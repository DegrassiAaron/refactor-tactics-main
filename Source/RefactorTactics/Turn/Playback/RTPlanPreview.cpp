#include "Turn/Playback/RTPlanPreview.h"

#include "Turn/Resolution/RTHexSim.h"
#include "Turn/Resolution/RTHexSimLibrary.h"  // BuildCompositeHexPath: lo STESSO A* del resolver
#include "Turn/Resolution/RTFacingLibrary.h"  // FacingFromPath: la derivazione pura, senza TurnLog
#include "Ability/Catalog/RTCatalogLibrary.h" // MapResolutionPhase: la fase dell'azione decide il posto della voce ([D-470])
#include "Turn/Resolution/RTMovementActionLibrary.h" // ResolveLinearMove: la rotta di uno scatto lineare e' quella del resolver ([D-524])
#include "Turn/Actions/RTActionQueueLibrary.h"        // NumberInstancesByContent, MainVisitLess: l'ordine delle pile e' quello del resolver (HUD-AP.5b)

namespace
{
	/** L'unita' nello snapshot, o `nullptr`. Identita' per indice, come ovunque nel simulatore. */
	const FRTHexSimUnit* UnitOf(const FRTHexSnapshot& Snapshot, int32 UnitId)
	{
		for (const FRTHexSimUnit& U : Snapshot.Units)
		{
			if (U.UnitId == UnitId)
			{
				return &U;
			}
		}
		return nullptr;
	}

	/**
	 * Il facing che guarda da `From` a `To`, derivato dalla regola e non inventato.
	 *
	 * ⚠️ **`DirectionTowards` e non `ReadFacingForConsumer`**: la seconda scrive una voce di TurnLog, cioe'
	 * dichiara che il RESOLVER ha letto quel facing. Un'anteprima che la chiamasse inciderebbe nel registro
	 * canonico una lettura mai avvenuta in partita. Vedi l'header.
	 *
	 * 🔑 **Il PRIMO passo verso il bersaglio, come il resolver** (`TargetingReoriented`, il Blast gira chi attacca con
	 * `DirectionTowards(Unit->Cell, AimCell)`). ⛔ **Non `FacingFromPath`**, che dal [D-524] risponde con l'ULTIMO passo
	 * per due celle non adiacenti: e' la direzione con cui si ARRIVA, e questa e' quella verso cui si GUARDA. Sulle celle
	 * di pareggio le due differiscono (`(0,0) -> (1,1)`: `E` e `SE`). ⏱️ *Fino a [D-524] le due coincidevano perche' il
	 * fallback di `FacingFromPath` era `DirectionTowards`; la coincidenza non era una regola* (review di #3810).
	 */
	ERTHexDirection FacingVerso(const FRTCellId& From, const FRTCellId& To, ERTHexDirection Corrente)
	{
		ERTHexDirection Verso = Corrente;
		return URTHexLibrary::DirectionTowards(From, To, Verso) ? Verso : Corrente; // celle uguali: il facing resta
	}
}

FRTPlanPreview URTPlanPreviewLibrary::MakePlanPreview(const FRTHexSnapshot& Snapshot,
	const FRTPlanPreviewInput& Plan, const TArray<FRTHexCombatUnit>& CombatUnits)
{
	FRTPlanPreview Out;

	const FRTHexSimUnit* Unit = UnitOf(Snapshot, Plan.UnitId);
	if (!Unit || !Unit->bAlive)
	{
		// Nessuna unita', nessuna timeline. ⛔ Non si restituisce una lista di fasi vuote: «non c'e' un piano»
		// e «il piano non fa niente» sono due cose diverse, e una lista vuota le distingue da sola.
		return Out;
	}

	const FRTCellId CellaCorrente = Unit->Cell;
	const ERTHexDirection FacingCorrente = Unit->Facing;

	// Lo stato che ogni fase consegna alla successiva. Le fasi risolvono in ordine, quindi la fase N vede
	// cio' che la N-1 ha lasciato: leggere sempre lo snapshot iniziale mostrerebbe quattro fasi che partono
	// tutte dallo stesso posto, cioe' l'errore che questo checkpoint esiste per togliere.
	FRTCellId CellaCorrenteDiFase = CellaCorrente;
	ERTHexDirection FacingCorrenteDiFase = FacingCorrente;

	// ── L'AZIONE PRINCIPALE: una voce, nella SUA fase e nel SUO posto ([D-470]) ───────────────────────────
	//
	// La fase la dichiara il catalogo (`FRTMainPreviewInput::Blast.Phase`), e la macro-fase in cui risolve decide dove sta la
	// voce: una `Preparation` prima dello scatto, una `Control` o un `Attack` fra lo scatto e il Move, una
	// `Environment` dopo il Move. ⏱️ *Fino a [D-470] la voce si chiamava sempre `Attack` e stava sempre fra lo
	// scatto e il Move*, contro le due promesse dell'header: la fase canonica, e l'ordine di risoluzione.
	//
	// ⛔ L'origine e l'area NON si ricalcolano qui: le produce `MakeBlastPreview`, che a sua volta chiama
	// `HexHitCells` — la forma canonica del combat. Una seconda derivazione sarebbe la seconda autorita' che
	// questo checkpoint vieta espressamente.
	//
	// ➕ **Le principali sono una lista** (E38 CP 38.14; HUD-AP.5b, #4159), e ognuna va nella colonna della sua fase.
	// ⚠️ Il Blast prende anche le fasi che un'azione principale non ha — `Snapshot` e i due movimenti: lo scatto ha
	// il suo slot e il percorso e' il Move. E' il posto in cui la voce stava per tutte prima di [D-470].
	TArray<const FRTMainPreviewInput*> PrincipaliNelPrep;
	TArray<const FRTMainPreviewInput*> PrincipaliNelBlast;
	TArray<const FRTMainPreviewInput*> PrincipaliNelCleanup;
	for (const FRTMainPreviewInput& Principale : Plan.Mains)
	{
		if (!Principale.Blast.bHasAction) { continue; }
		switch (URTCatalogLibrary::MapResolutionPhase(Principale.Blast.Phase))
		{
		case ERTMatchPhase::Prep:    PrincipaliNelPrep.Add(&Principale); break;
		case ERTMatchPhase::Cleanup: PrincipaliNelCleanup.Add(&Principale); break;
		default:                     PrincipaliNelBlast.Add(&Principale); break;
		}
	}

	// 🔑 **Dentro la colonna, l'ordine in cui il resolver le visita** (HUD-AP.5b, #4159): `MainVisitLess`, la regola di
	// `ARTTurnManager::MainTurnsOf`, mai l'ordine dell'ingresso ([D-615] (5)). Le chiavi che `Blast` porta si leggono da
	// li', come `MainTurnsOf` le legge dalla voce: la cella di mira e' quella dichiarata o quella del bersaglio.
	const auto ChiaveDiVisita = [&CombatUnits, &CellaCorrente](const FRTMainPreviewInput& Principale)
	{
		FRTMainVisitKey Chiave;
		Chiave.MacroPhase = static_cast<int32>(URTCatalogLibrary::MapResolutionPhase(Principale.Blast.Phase));
		Chiave.Priority = Principale.Priority;
		Chiave.ActionId = Principale.ActionId;
		Chiave.TargetStableUnitId = Principale.TargetStableUnitId;
		Chiave.AimCell = Principale.Blast.bTargetsCell ? Principale.Blast.TargetCell
			: CombatUnits.IsValidIndex(Principale.Blast.TargetId) ? CombatUnits[Principale.Blast.TargetId].Cell
			: CellaCorrente;
		Chiave.CoverEdge = Principale.CoverEdge;
		Chiave.DeclarationIndex = Principale.DeclarationIndex;
		return Chiave;
	};
	const auto OrdinaLeVisite = [&ChiaveDiVisita](TArray<const FRTMainPreviewInput*>& Principali)
	{
		Principali.StableSort([&ChiaveDiVisita](const FRTMainPreviewInput& A, const FRTMainPreviewInput& B)
		{
			return URTActionQueueLibrary::MainVisitLess(ChiaveDiVisita(A), ChiaveDiVisita(B));
		});
	};
	OrdinaLeVisite(PrincipaliNelBlast);
	OrdinaLeVisite(PrincipaliNelCleanup);

	/**
	 * La voce dell'azione principale, una sola costruzione per i tre posti in cui puo' stare.
	 *
	 * `DoveSara` e' la cella in cui l'unita' si trova quando l'azione risolve, cioe' dove va il ghost; `FacingPrima`
	 * e `FontePrima` sono l'orientamento che la fase riceve. `bRuotaVersoIlBersaglio` e' vero nel solo Blast: e' li'
	 * che `CollectAttackIntents` gira chi agisce, mentre `ResolvePrep` e `ResolveEnvironment` non lo girano.
	 * ⚠️ Nel Blast il ghost sta sull'origine di mira della voce, che conosce solo `MakeBlastPreview`: il chiamante passa
	 * una cella qualunque e copia l'origine nella destinazione dopo.
	 */
	const auto VoceAzionePrincipale = [&Plan, &CombatUnits](const FRTMainPreviewInput& Principale, const FRTCellId& DoveSara,
		ERTHexDirection FacingPrima, ERTPreviewFacingSource FontePrima, bool bRuotaVersoIlBersaglio)
	{
		// ⛔ L'origine e l'area NON si ricalcolano qui: le produce `MakeBlastPreview`, una per voce.
		const FRTBlastPreview Blast = URTHexCombatLibrary::MakeBlastPreview(Principale.Blast, CombatUnits);
		FRTPhasePreviewEntry Colpo;
		Colpo.Phase = Principale.Blast.Phase;
		Colpo.UnitId = Plan.UnitId;
		Colpo.ActionId = Principale.ActionId;
		// L'origine e' quella di MIRA ([D-464]); la destinazione e' dove sara' l'unita'. Coincidono salvo per
		// un'azione del Cleanup dopo un movimento: mira da dove e' stata pianificata, e risolve dopo il Move.
		Colpo.PreviewOrigin = Blast.Origin;
		Colpo.PreviewDestination = DoveSara;
		Colpo.AffectedCells = Blast.HitCells;
		Colpo.AllyCells = Blast.AllyCells;
		Colpo.TargetRefusal = Principale.TargetRefusal;

		// Cio' che il piano DICHIARA di bersagliare, distinto da cio' che l'azione toccherebbe.
		//
		// ⛔ **Un bersaglio CADUTO non ha una cella dichiarata, e la prima stesura gliela dava.**
		// `MakeBlastPreview` esige `Units[TargetId].bAlive` e lo motiva: *«un bersaglio caduto non degrada
		// alla propria ultima cella — quello sarebbe il FALLBACK del resolver»*. Leggendo il solo
		// `IsValidIndex` la timeline segnava un bersaglio sulla cella di un cadavere, che l'area lasciava
		// giustamente vuota: due campi della stessa voce che si contraddicevano.
		FRTCellId CellaMira;
		bool bMiraSuUnitaViva = false;
		bool bMiraNota = false;
		if (Principale.Blast.bTargetsCell)
		{
			CellaMira = Principale.Blast.TargetCell;
			bMiraNota = true;
		}
		else if (CombatUnits.IsValidIndex(Principale.Blast.TargetId) && CombatUnits[Principale.Blast.TargetId].bAlive)
		{
			CellaMira = CombatUnits[Principale.Blast.TargetId].Cell;
			bMiraNota = true;
			bMiraSuUnitaViva = true;
		}
		if (bMiraNota)
		{
			Colpo.TargetCells.Add(CellaMira);
		}

		// 🔴 **La rotazione verso il bersaglio vale SOLO per un'unita' viva, e la prima stesura la
		// prevedeva anche per una cella.** Il resolver la guarda con
		// `if (Unit->IsAlive() && Target && Target->IsAlive() && Target != Unit)`
		// (`RTTurnManager_Blast.cpp:715`): un'azione bersagliata su una CELLA — un'area lasciata cadere su un
		// varco vuoto — non orienta chi la esegue. Prevederla comunque faceva leggere al giocatore una postura
		// di copertura direzionale che al momento del colpo non sarebbe esistita.
		// ⚠️ **E vale solo nel Blast** ([D-470]): prima la si prevedeva per ogni fase, anche dove il resolver non
		// gira nessuno.
		if (bRuotaVersoIlBersaglio && bMiraSuUnitaViva)
		{
			// La stessa derivazione che il resolver registra come `TargetingReoriented`. Qui si PREVEDE
			// quella rotazione, non se ne inventa un'altra.
			Colpo.Facing = FacingVerso(Blast.Origin, CellaMira, FacingPrima);
			Colpo.FacingSource = ERTPreviewFacingSource::DerivedFromPath;
		}
		else
		{
			Colpo.Facing = FacingPrima;
			Colpo.FacingSource = FontePrima;
		}

		// 🔑 **Il livello segue le definizioni dell'enum, non un giudizio di questa funzione.**
		//  - bersaglio rifiutato o nessuna cella colpita: il piano non produrra' niente → `Uncertain`;
		//  - origine che viene da uno scatto: dipende da un movimento, e «muoversi basta» → `Uncertain`;
		//  - altrimenti: c'e' un bersaglio e l'unita' non si sposta → `Predicted`.
		const bool bBersaglioUtile =
			Principale.TargetRefusal == ERTTargetRefusal::None && Blast.HitCells.Num() > 0;
		Colpo.Certainty = (!bBersaglioUtile || Blast.bOriginFromPlannedDash)
			? ERTIntentCertainty::Uncertain
			: ERTIntentCertainty::Predicted;
		return Colpo;
	};

	// ── PREP ────────────────────────────────────────────────────────────────────────────────────────────
	//
	// Non sposta e non bersaglia: per la definizione di `ERTIntentCertainty::Confirmed` — *«niente puo'
	// cambiarlo: nessun movimento, nessun bersaglio»* — e' l'unica fase che puo' essere certa.
	if (Plan.bReactionArmed || !Plan.PrepActionId.IsNone())
	{
		FRTPhasePreviewEntry Prep;
		Prep.Phase = ERTResolutionPhase::Preparation;
		Prep.UnitId = Plan.UnitId;
		Prep.ActionId = Plan.PrepActionId;
		Prep.PreviewOrigin = CellaCorrenteDiFase;
		Prep.PreviewDestination = CellaCorrenteDiFase;
		Prep.Facing = FacingCorrenteDiFase;
		Prep.FacingSource = ERTPreviewFacingSource::Authoritative;
		Prep.Certainty = ERTIntentCertainty::Confirmed;
		Out.Phases.Add(Prep);
	}

	// ➕ **Le voci di Prep aggiunte (E38 CP 38.13) e la principale di `Preparation`, nell'ordine di `ResolvePrep`**
	// (HUD-AP.5b, #4159). Il resolver ne fa un'istanza per voce — chi agisce e' anche il bersaglio, la cella e' quella
	// dell'unita' o quella dichiarata — e le percorre nell'ordine di `NumberInstancesByContent`: qui si costruiscono le
	// stesse istanze e si chiede a lei. ⛔ Mai l'ordine dell'ingresso ([D-615] (5)).
	//
	// ⚠️ La voce qui sopra, la reazione armata, non entra fra le istanze: `ResolvePrep` non la ordina con le altre, e il suo
	// posto e' il primo della pila come prima di HUD-AP.5b. Le reazioni in una lista nella timeline sono un seguito di CP 38.15 (#4116).
	struct FVoceDelPrep { const FRTPrepPreviewInput* Prep = nullptr; const FRTMainPreviewInput* Principale = nullptr; };
	TArray<FVoceDelPrep> VociDelPrep;
	TArray<FRTActionInstance> IstanzeDelPrep;
	const auto IstanzaDelPrep = [&Plan, &CellaCorrente](FName ActionId, int32 Priorita, ERTResolutionPhase Fase,
		const FRTCellId& Cella, int32 Posizione)
	{
		FRTActionInstance Istanza;
		Istanza.Def.ActionId = ActionId;
		Istanza.Def.Priority = Priorita;
		Istanza.Def.ResolutionPhase = Fase;
		Istanza.SourceUnitId = Plan.UnitId;
		Istanza.TargetUnitId = Plan.UnitId;
		Istanza.TargetCell = Cella;
		Istanza.DeclarationIndex = Posizione;
		return Istanza;
	};
	for (const FRTPrepPreviewInput& VoceDiPrep : Plan.PrepEntries)
	{
		VociDelPrep.Add({ &VoceDiPrep, nullptr });
		IstanzeDelPrep.Add(IstanzaDelPrep(VoceDiPrep.ActionId, VoceDiPrep.Priority, ERTResolutionPhase::Preparation,
			CellaCorrente, VoceDiPrep.DeclarationIndex));
	}
	for (const FRTMainPreviewInput* Principale : PrincipaliNelPrep)
	{
		VociDelPrep.Add({ nullptr, Principale });
		IstanzeDelPrep.Add(IstanzaDelPrep(Principale->ActionId, Principale->Priority, Principale->Blast.Phase,
			Principale->Blast.bTargetsCell ? Principale->Blast.TargetCell : CellaCorrente, Principale->DeclarationIndex));
	}
	// Le istanze si ordinano in place, quindi ognuna si riporta alla sua voce: la prima non ancora presa con le stesse chiavi.
	// Due voci con le stesse chiavi sono copie uguali nel contenuto, e quale delle due vada prima non si vede.
	TArray<FRTActionInstance> InOrdine = IstanzeDelPrep;
	int32 Spareggio = 0;
	URTActionQueueLibrary::NumberInstancesByContent(InOrdine, Spareggio);
	TArray<bool> Presa;
	Presa.Init(false, IstanzeDelPrep.Num());
	for (const FRTActionInstance& Istanza : InOrdine)
	{
		int32 Voce = INDEX_NONE;
		for (int32 I = 0; I < IstanzeDelPrep.Num() && Voce == INDEX_NONE; ++I)
		{
			const FRTActionInstance& Candidata = IstanzeDelPrep[I];
			if (!Presa[I] && Candidata.Def.ActionId == Istanza.Def.ActionId && Candidata.Def.Priority == Istanza.Def.Priority
				&& Candidata.Def.ResolutionPhase == Istanza.Def.ResolutionPhase && Candidata.TargetCell == Istanza.TargetCell
				&& Candidata.DeclarationIndex == Istanza.DeclarationIndex)
			{
				Voce = I;
			}
		}
		if (Voce == INDEX_NONE) { continue; } // non succede: `NumberInstancesByContent` ordina, non crea istanze
		Presa[Voce] = true;
		if (const FRTPrepPreviewInput* VoceDiPrep = VociDelPrep[Voce].Prep)
		{
			// Non sposta e non bersaglia: e' certa come la voce della reazione qui sopra.
			FRTPhasePreviewEntry Prep;
			Prep.Phase = ERTResolutionPhase::Preparation;
			Prep.UnitId = Plan.UnitId;
			Prep.ActionId = VoceDiPrep->ActionId;
			Prep.PreviewOrigin = CellaCorrenteDiFase;
			Prep.PreviewDestination = CellaCorrenteDiFase;
			Prep.Facing = FacingCorrenteDiFase;
			Prep.FacingSource = ERTPreviewFacingSource::Authoritative;
			Prep.Certainty = ERTIntentCertainty::Confirmed;
			Out.Phases.Add(Prep);
		}
		else
		{
			// La principale di `Preparation` risolve qui, prima dello scatto ([D-470]): `ResolvePrep` la arma da dove
			// l'unita' e' adesso, e col facing di adesso — le azioni di Prep agiscono su chi le usa, e non lo girano.
			Out.Phases.Add(VoceAzionePrincipale(*VociDelPrep[Voce].Principale, CellaCorrenteDiFase, FacingCorrenteDiFase,
				ERTPreviewFacingSource::Authoritative, /*bRuotaVersoIlBersaglio=*/ false));
		}
	}

	// ⚠️ **Lo scatto si applica solo se e' stato pianificato E risolve.** I due flag sono indipendenti
	// nell'ingresso, e leggerne uno solo — come faceva la prima stesura nel blocco della reazione — lascia
	// passare `bDashResolves` senza `bDashPlanned`, cioe' una cella d'arrivo che nessuna fase ha prodotto.
	const bool bScattoEffettivo = Plan.bDashPlanned && Plan.bDashResolves;

	// ── DASH ────────────────────────────────────────────────────────────────────────────────────────────
	//
	// ⚠️ La voce si mostra anche quando lo scatto **non** si applica: nasconderla direbbe al giocatore che non ha
	// pianificato uno scatto, mentre lo ha pianificato.
	//
	// 🔑 **La differenza la porta il ghost, che sta dove sara' l'unita'** ([D-474]). Uno scatto che non si applica
	// non la porta da nessuna parte, e la voce si comporta come una rotta rifiutata: ghost sulla cella corrente,
	// facing di adesso, nessun percorso. La cella dello scatto resta nell'intento dell'HUD (`Intent.DashCell`).
	// ⏱️ *Fino a [D-474] il ghost stava sulla cella dello scatto in ogni caso, e questo commento diceva che la
	// differenza la portava la certezza*: ma la certezza era `Uncertain` in entrambi i casi. Da [D-475] la porta
	// anche lei: una voce Dash che non sposta e' `Confirmed`, vedi sotto.
	if (Plan.bDashPlanned)
	{
		// 🔴 **La rotta dello scatto la CALCOLA il resolver, e la prima stesura la inventava.**
		// Diceva `PreviewPath = { partenza, arrivo }` — due celle, una linea retta — mentre `ResolveDash`
		// fa `FindPathForUnit(Snapshot, i, PlannedDashCell).Path` (`RTTurnManager.cpp:5017`) e da quel
		// percorso ricava anche il facing. Su uno scatto che deve aggirare un ostacolo il ghost disegnava una
		// retta ATTRAVERSO l'ostacolo e dichiarava un orientamento che l'unita' non avrebbe mai avuto: lo
		// stesso difetto che la fase Move era stata scritta per evitare, sulla fase accanto.
		// [D-474]: la rotta si chiede al resolver solo per uno scatto che si applica. Per gli altri resta vuota,
		// cioe' la rotta rifiutata qui sotto: l'unita' non si sposta e non si gira.
		FRTHexPathResult RottaScatto;
		if (bScattoEffettivo && URTMovementActionLibrary::IsLinear(Plan.DashStyle))
		{
			// 🔑 **[D-524] Una mobilita' LINEARE percorre la retta, non l'A\*** — e la percorre con la STESSA funzione
			// del resolver. Lo scatto non paga il budget del movimento (`Unbalanced`, `Sneak` non lo riguardano) ne'
			// aggira gli occupanti: li' si ferma, e una carica urta il nemico. Dentro la portata l'A\* sceglieva fra
			// piu' percorsi minimi sulle celle fuori asse, con un ultimo passo che poteva non essere quello della
			// risoluzione, e davanti a un muro lo aggirava; oltre la portata era la retta, ma troncata a parte
			// (review della PR #3767). Ora e' una chiamata sola, e il troncamento della carica e' il suo `OutOfRange`.
			//
			// Gli ostili sono le unita' vive di un'altra squadra nello snapshot di CHI PIANIFICA, quindi quelle che
			// conosce ([D-371]): la fotografia e' la stessa su cui il controller ha accettato il piano.
			TSet<int32> Ostili;
			for (const FRTHexSimUnit& U : Snapshot.Units)
			{
				if (U.bAlive && U.TeamId != Unit->TeamId)
				{
					Ostili.Add(U.UnitId);
				}
			}
			const FRTLinearMoveResult Corsa = URTMovementActionLibrary::ResolveLinearMove(Snapshot.MapForPlanning(),
				CellaCorrenteDiFase, Plan.PlannedDashCell, Plan.DashRange, Plan.DashStyle, Snapshot.Occupancy, Ostili);
			// Una corsa che non muove e' una rotta rifiutata: vuota, e il fantasma resta dov'e'.
			if (Corsa.Entered.Num() > 0)
			{
				RottaScatto.Path.Add(CellaCorrenteDiFase);
				RottaScatto.Path.Append(Corsa.Entered);
			}
		}
		else if (bScattoEffettivo)
		{
			RottaScatto = URTHexSimLibrary::FindPathForUnit(Snapshot, Plan.UnitId, Plan.PlannedDashCell);
		}

		FRTPhasePreviewEntry Dash;
		Dash.Phase = ERTResolutionPhase::FastMovement;
		Dash.UnitId = Plan.UnitId;
		Dash.ActionId = Plan.DashActionId;
		Dash.PreviewOrigin = CellaCorrenteDiFase;
		Dash.PreviewPath = RottaScatto.Path;
		// Una rotta RIFIUTATA torna vuota: allora lo scatto non porta da nessuna parte, e dirlo e' l'esito
		// giusto — come per il Move.
		Dash.PreviewDestination = RottaScatto.Path.Num() > 0 ? RottaScatto.Path.Last() : CellaCorrenteDiFase;
		Dash.Facing = RottaScatto.Path.Num() >= 2
			? URTFacingLibrary::FacingFromPath(RottaScatto.Path, FacingCorrenteDiFase)
			: FacingCorrenteDiFase;
		Dash.FacingSource = RottaScatto.Path.Num() >= 2
			? ERTPreviewFacingSource::DerivedFromPath
			: ERTPreviewFacingSource::InheritedFromPreviousPhase;
		// «Muoversi basta»: le celle del percorso sono contendibili e il resolver puo' troncare la rotta.
		//
		// 🔑 **Ma una voce che non sposta e' certa** ([D-475]): uno scatto che non si applica, o una rotta rifiutata.
		// Prima della fine del Dash nessuno muove un'unita' ferma: la Prep non sposta, la carica spinge nel Blast, e le
		// reazioni scattano nel Blast e nel Cleanup. E una rotta rifiutata qui lo e' anche nel resolver: lo snapshot di
		// pianificazione porta la posizione vera delle unita' note ([D-371]), e il tetto di [D-425] non e' mai minore
		// della banda. ⛔ La voce Move resta `Uncertain`: una spinta nel Blast puo' spostare l'unita' prima del Move.
		// 🔑 La premessa la fissa `Actions.Charge.StandingUnitStaysPutUntilTheBlast` (#3576): se un giorno il Dash
		// spostasse un'unita' ferma, quel test diventerebbe rosso prima di questa riga.
		// ⏱️ *Fino a [D-475] la voce Dash era `Uncertain` in ogni caso.*
		Dash.Certainty = RottaScatto.Path.Num() >= 2 ? ERTIntentCertainty::Uncertain : ERTIntentCertainty::Confirmed;
		Out.Phases.Add(Dash);

		// 🔴 **Solo se si applica DAVVERO.** `bDashResolves` e' la stessa domanda che `ResolveDash` si pone,
		// e propagare una cella d'arrivo che lo scatto non raggiunge sposterebbe anche l'origine del Blast.
		if (bScattoEffettivo)
		{
			CellaCorrenteDiFase = Dash.PreviewDestination;
			FacingCorrenteDiFase = Dash.Facing;
		}
	}

	// 🔑 **La cella e il facing DOPO lo scatto, in una sede sola.** La prima stesura ne aveva tre — la
	// catena di fase, i campi `Plan.Blast.*` che `MakeBlastPreview` consuma, e una terza derivazione dentro il
	// blocco della reazione — e con un ingresso incoerente rispondevano cose diverse alla stessa domanda.
	const FRTCellId CellaDopoScatto = CellaCorrenteDiFase;
	const ERTHexDirection FacingDopoScatto = FacingCorrenteDiFase;

	// ── BLAST ───────────────────────────────────────────────────────────────────────────────────────────
	//
	// La principale di `Control` o di `Attack` risolve qui, fra lo scatto e il Move, ed e' la sola che il
	// resolver gira verso un bersaglio vivo (`CollectAttackIntents`, [D-020]). Non sposta chi la esegue: il
	// ghost sta sulla sua origine, che dopo uno scatto e' la cella d'arrivo ([D-464]).
	//
	// 🔑 **E il facing che il Move riceve e' quello DOPO il Blast** (#3566). Il resolver gira chi agisce verso il
	// bersaglio vivo, e un Move che non lo sposta non deriva un altro orientamento: *«chi non si e' mosso non deriva
	// nessun orientamento»* (`ResolveMovement`). ⏱️ *Fino a #3566 la voce Move senza percorso prendeva il facing
	// dopo lo scatto*, cioe' di prima del Blast, e mostrava l'unita' girata dove non sarebbe stata.
	ERTHexDirection FacingPrimaDelMove = FacingDopoScatto;
	ERTPreviewFacingSource FontePrimaDelColpo = bScattoEffettivo
		? ERTPreviewFacingSource::InheritedFromPreviousPhase
		: ERTPreviewFacingSource::Authoritative;
	for (const FRTMainPreviewInput* Principale : PrincipaliNelBlast)
	{
		// Il ghost sta sull'origine di mira della voce ([D-464]), che `VoceAzionePrincipale` legge da `MakeBlastPreview`.
		FRTPhasePreviewEntry Colpo = VoceAzionePrincipale(*Principale, FRTCellId(), FacingPrimaDelMove, FontePrimaDelColpo,
			/*bRuotaVersoIlBersaglio=*/ true);
		Colpo.PreviewDestination = Colpo.PreviewOrigin;
		Out.Phases.Add(Colpo);
		// ➕ Con piu' principali nel Blast ognuna riceve il facing che la precedente le lascia: il resolver le visita una
		// alla volta, e ognuna gira chi agisce verso il proprio bersaglio (HUD-AP.5b).
		FacingPrimaDelMove = Colpo.Facing;
		FontePrimaDelColpo = ERTPreviewFacingSource::InheritedFromPreviousPhase;
	}

	// ── MOVE ────────────────────────────────────────────────────────────────────────────────────────────
	//
	// 🔴 **Il percorso lo costruisce `BuildCompositeHexPath`, ed e' la parita' che `CP 11.5` chiede.** Non e'
	// una funzione «equivalente»: e' letteralmente quella che riempie `ARTUnit::PlannedPath`
	// (`RTPlayerController.cpp:1830`), e `ResolveMovement` consuma quel percorso quando parte ancora dalla
	// cella dell'unita'. Preview e resolver leggono la stessa uscita della stessa funzione.
	//
	// ⚠️ **E il Move parte da DOPO lo scatto, non dalla cella di adesso.** `ResolveMovement` prende uno
	// snapshot FRESCO (`MakeCurrentSnapshot`, `RTTurnManager_Movement.cpp:109`), cioe' vede l'unita' gia'
	// scattata; e riusa `PlannedPath` solo se quel percorso parte ancora dalla cella corrente — dopo uno
	// scatto non ci parte piu', e il resolver **ricalcola** verso `PlannedCell`. Ecco perche' la certezza di
	// questa fase scende quando lo scatto si applica: cio' che si disegna e' il percorso dichiarato, e il
	// resolver ne percorrera' un altro.
	// Dove sara' l'unita' a Move concluso, e come guardera': li' risolve la principale del Cleanup ([D-470]).
	// Senza un Move resta dove lo scatto l'ha lasciata.
	FRTCellId CellaDopoIlMove = CellaDopoScatto;
	ERTHexDirection FacingDopoIlMove = FacingPrimaDelMove;
	ERTPreviewFacingSource FonteDopoIlMove = bScattoEffettivo
		? ERTPreviewFacingSource::InheritedFromPreviousPhase
		: ERTPreviewFacingSource::Authoritative;

	if (Plan.PlannedWaypoints.Num() > 0)
	{
		// 🔴 **Spostare l'unita' vuol dire spostare anche l'OCCUPAZIONE, e la prima stesura riscriveva
		// solo `Units[i].Cell`.** `BuildCompositeHexPath` non guarda `Units` per sapere cosa e' bloccato:
		// `BlockedCellsFor` itera `Snapshot.Occupancy` (`RTHexSimLibrary.cpp:34`). Con la sola cella riscritta
		// lo snapshot restava internamente incoerente — la cella lasciata continuava a risultare occupata dal
		// mover, e quella d'arrivo non risultava sua. Se un'altra unita' ne occupasse la destinazione, il
		// percorso partirebbe da un nodo bloccato e il Move collasserebbe a vuoto, **in silenzio**: un
		// percorso rifiutato si legge come «il piano non porta da nessuna parte».
		//
		// ⚠️ **E la copia si paga solo quando serve.** Senza scatto lo snapshot si usa com'e': copiarlo
		// duplicherebbe `Units`, `Occupancy`, `Overlaps` e la conoscenza di squadra a ogni waypoint aggiunto o
		// tolto, per poi non cambiarne niente.
		FRTHexSnapshot SnapshotDiFase;
		const FRTHexSnapshot* SnapshotPerIlMove = &Snapshot;
		if (bScattoEffettivo)
		{
			SnapshotDiFase = Snapshot;
			for (FRTHexSimUnit& U : SnapshotDiFase.Units)
			{
				if (U.UnitId == Plan.UnitId)
				{
					SnapshotDiFase.Occupancy.Remove(U.Cell);
					U.Cell = CellaDopoScatto;
					break;
				}
			}
			SnapshotDiFase.Occupancy.Add(CellaDopoScatto, Plan.UnitId);
			SnapshotPerIlMove = &SnapshotDiFase;
		}

		// [D-569]: un Move che lo stordimento nega e' una rotta RIFIUTATA (vuota), come la rifiuta il resolver: la voce resta, il
		// ghost sta dove l'unita' si trova e non percorre niente. `bMoveResolves` e' la domanda del resolver, non un secondo giudizio.
		const FRTHexPathResult Percorso = Plan.bMoveResolves
			? URTHexSimLibrary::BuildCompositeHexPath(*SnapshotPerIlMove, Plan.UnitId, Plan.PlannedWaypoints)
			: FRTHexPathResult();

		FRTPhasePreviewEntry Move;
		Move.Phase = ERTResolutionPhase::NormalMovement;
		Move.UnitId = Plan.UnitId;
		Move.ActionId = Plan.MoveActionId;
		Move.PreviewOrigin = CellaDopoScatto;
		Move.PreviewPath = Percorso.Path;
		// ⚠️ Un percorso RIFIUTATO torna vuoto (vedi il contratto di `BuildCompositeHexPath`), e allora la
		// destinazione e' l'origine: il piano non porta l'unita' da nessuna parte, e dirlo e' l'esito giusto.
		Move.PreviewDestination = Percorso.Path.Num() > 0 ? Percorso.Path.Last() : CellaDopoScatto;
		// Senza un percorso vero l'unita' resta com'e' dopo il Blast (#3566): vedi `FacingPrimaDelMove`.
		Move.Facing = Percorso.Path.Num() >= 2
			? URTFacingLibrary::FacingFromPath(Percorso.Path, FacingPrimaDelMove)
			: FacingPrimaDelMove;
		Move.FacingSource = Percorso.Path.Num() >= 2
			? ERTPreviewFacingSource::DerivedFromPath
			: ERTPreviewFacingSource::InheritedFromPreviousPhase;
		Move.Certainty = ERTIntentCertainty::Uncertain;
		Out.Phases.Add(Move);

		CellaDopoIlMove = Move.PreviewDestination;
		FacingDopoIlMove = Move.Facing;
		FonteDopoIlMove = ERTPreviewFacingSource::InheritedFromPreviousPhase;
	}

	// ── CLEANUP ─────────────────────────────────────────────────────────────────────────────────────────
	//
	// La principale di `Environment` risolve dopo il Move ([D-470]), in `ResolveEnvironment`. Il ghost va dove
	// l'unita' sara' allora, cioe' dopo il Move; l'origine resta quella di mira, la cella da cui e' stata
	// pianificata ([D-464]). `ResolveEnvironment` non gira chi agisce: il facing e' quello che il Move le lascia.
	for (const FRTMainPreviewInput* Principale : PrincipaliNelCleanup)
	{
		Out.Phases.Add(VoceAzionePrincipale(*Principale, CellaDopoIlMove, FacingDopoIlMove, FonteDopoIlMove,
			/*bRuotaVersoIlBersaglio=*/ false));
	}

	// ── LA REAZIONE, che fase non e' ────────────────────────────────────────────────────────────────────
	//
	// ⛔ Fuori da `Phases` di proposito: vedi `FRTReactionPreview`. Il livello e' `Uncertain` per definizione
	// dell'enum — *«vale **sempre** per una reazione armata, che per definizione attende un trigger che non
	// decidiamo noi»* — quindi non e' una scelta di questa funzione ed e' scritto altrove.
	if (Plan.bReactionArmed)
	{
		Out.Reaction.bArmed = true;
		Out.Reaction.UnitId = Plan.UnitId;
		Out.Reaction.ReactionProfileId = Plan.ReactionProfileId;
		// La cella di FINE Dash: la reazione e' armata in Prep e il Move risolve dopo il Blast, quindi
		// sorveglia da dove l'unita' si trovera' quando un innesco potra' scattare.
		// ⚠️ **Cella E facing dello STESSO istante**, che e' la fine del Dash. La prima stesura prendeva la
		// cella dopo lo scatto e il facing dopo il BLAST: una reazione il cui innesco scatta durante il Dash —
		// o prima dell'attacco — veniva disegnata mentre guarda il bersaglio di un colpo non ancora partito.
		// L'argomento con cui `WatchOrigin` sceglie la cella del Dash sceglie anche il facing di quel momento.
		Out.Reaction.WatchOrigin = CellaDopoScatto;
		Out.Reaction.Facing = FacingDopoScatto;
		Out.Reaction.Certainty = ERTIntentCertainty::Uncertain;
	}

	// ── LE PILE (HUD-AP.5b, #4159) ──────────────────────────────────────────────────────────────────────────
	//
	// La colonna e' la macro-fase, e le voci di una colonna stanno qui sopra una dopo l'altra, gia' nell'ordine del
	// resolver: il posto nella pila e' il loro conto. La mappa serve solo a cercare, e non si itera.
	TMap<ERTMatchPhase, int32> VociNellaColonna;
	for (FRTPhasePreviewEntry& Voce : Out.Phases)
	{
		Voce.StackIndex = ++VociNellaColonna.FindOrAdd(URTCatalogLibrary::MapResolutionPhase(Voce.Phase));
	}

	return Out;
}
