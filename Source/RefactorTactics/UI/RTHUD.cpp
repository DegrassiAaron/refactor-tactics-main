#include "UI/RTHUD.h"
#include "UI/RTHudViewModel.h"
#include "RTGameMode.h"
#include "Unit/RTUnit.h"
#include "Turn/RTIntentPrivacyLibrary.h"
#include "Ability/RTActionData.h"
#include "Player/RTPlayerController.h"
#include "Player/RTPlayerState.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Turn/RTTurnRules.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Pathfinding/RTHexPathLibrary.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Engine/Canvas.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

/**
 * L'interruttore dei pannelli **screen-space** di questo Canvas HUD.
 *
 * 🔴 **Taglia esattamente lungo il confine della spec**, e non e' una scorciatoia di comodo:
 * `progettazione-hud.md` separa il **§4.1 Screen HUD** — turno, fase, timer, combat log, barra abilita',
 * terna degli slot — dal **§4.2 Tactical World Overlay** — barre sopra le unita', path, waypoint, AoE,
 * fuoco amico. Questa variabile spegne il primo e **non tocca** il secondo.
 *
 * Serve perche' CP 11.7 sta ricostruendo il §4.1 in UMG (`WBP_RT_*`): finche' i due coesistono, le stesse
 * informazioni compaiono due volte a schermo, e i pannelli Canvas coprono le zone dove i widget nuovi
 * devono stare. Con `rt.HUD.CanvasPanels 0` si lavora sul layer nuovo senza il vecchio sotto.
 *
 * ⚠️ **Non e' una decisione di architettura**: quale dei due layer debba disegnare cosa resta aperto — il
 * piano lo chiama «Task 7-bis» — e questa variabile serve a poterlo *decidere guardando*, invece che a
 * deciderlo adesso cancellando codice coperto da `RefactorTactics.HUD.*`.
 *
 * Default `1`: il comportamento di prima resta quello che si ottiene senza fare nulla.
 */
static TAutoConsoleVariable<int32> CVarHudCanvasPanels(
	TEXT("rt.HUD.CanvasPanels"),
	1,
	TEXT("Pannelli screen-space del Canvas HUD (progettazione-hud.md §4.1).\n")
	TEXT("  1 = accesi (default)  |  0 = spenti\n")
	TEXT("Spegne: intestazione di turno, combat log, barra abilita', terna degli slot.\n")
	TEXT("NON tocca il §4.2 world-space (barre sopra le unita', path, AoE, fuoco amico) ne' il banner\n")
	TEXT("di scenario, che spiega perche' una mappa senza partita non mostra nulla."),
	ECVF_Default);

namespace
{
	// Centro-mondo di una cella esagonale, con la quota del suo layer (per disegnare i path sul ponte).
	// Stessa conversione usata da risoluzione e playback: l'anteprima non puo' divergere dal percorso reale.
	FVector HexCellWorld(const FRTCellId& Cell, const FVector& Origin, float HexSize, float LayerHeight)
	{
		return URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerHeight);
	}

	/** Cella in coordinate assiali, come compaiono nel TurnLog. */
	FString HexCellText(const FRTCellId& Cell)
	{
		return FString::Printf(TEXT("(q=%d,r=%d,L=%d)"), Cell.X, Cell.Y, Cell.Layer);
	}
}

FVector2D ARTHUD::ClampOverlayAnchor(const FVector2D& Anchor, float HalfWidth,
	float AboveAnchor, float BelowAnchor, const FVector2D& Viewport, float Margin)
{
	const float MinX = Margin + HalfWidth;
	const float MaxX = Viewport.X - Margin - HalfWidth;
	const float MinY = Margin + AboveAnchor;
	const float MaxY = Viewport.Y - Margin - BelowAnchor;

	// Blocco piu' largo/alto del viewport: `Min` supera `Max` e un `Clamp` normale restituirebbe il bordo
	// sbagliato (l'ordine degli argomenti decide, non la geometria). Si sceglie esplicitamente il minimo —
	// bordo superiore/sinistro — perche' il nome sta in alto ed e' cio' che identifica l'unita'.
	const float X = (MinX <= MaxX) ? FMath::Clamp(Anchor.X, MinX, MaxX) : MinX;
	const float Y = (MinY <= MaxY) ? FMath::Clamp(Anchor.Y, MinY, MaxY) : MinY;
	return FVector2D(X, Y);
}

void ARTHUD::ComputePlannedHitMarks(const TArray<ARTUnit*>& Units, int32 PlayerTeamId,
	TSet<FRTCellId>& OutHitCells, TSet<FRTCellId>& OutAllyHitCells)
{
	OutHitCells.Reset();
	OutAllyHitCells.Reset();

	for (const ARTUnit* Attacker : Units)
	{
		// Solo i piani DELLE PROPRIE unita': quelli avversari non si leggono, nemmeno per dedurne una cella
		// (invariante #6). Non e' prudenza eccessiva — e' la stessa regola per cui l'HUD non disegna gli
		// intenti nemici, e va rispettata anche dove il risultato sarebbe «solo» un colore.
		if (!Attacker || !Attacker->IsAlive() || Attacker->TeamId != PlayerTeamId)
		{
			continue;
		}
		const URTActionData* Ability = Attacker->GetAbility(Attacker->PlannedAbilityIndex);
		const ARTUnit* Target = Attacker->PlannedAttackTarget;
		if (!Ability || !Target || !Target->IsAlive())
		{
			continue;
		}

		// Le stesse celle che decideranno l'esito: `HexHitCells` e' la funzione del resolver, non una copia.
		const TArray<FRTCellId> Hit = URTHexCombatLibrary::HexHitCells(
			Ability->Shape, Attacker->Cell, Target->Cell, Ability->RangeCells, Ability->AreaRadius);
		OutHitCells.Append(TSet<FRTCellId>(Hit));

		// Fuoco amico solo se l'azione puo' DAVVERO colpire i propri: segnalare un alleato che non subirebbe
		// nulla insegna a ignorare il segnale.
		if (!Ability->Def.bFriendlyFire)
		{
			continue;
		}
		for (const ARTUnit* Other : Units)
		{
			if (Other && Other != Attacker && Other->IsAlive() && Other->TeamId == Attacker->TeamId
				&& Hit.Contains(Other->Cell))
			{
				OutAllyHitCells.Add(Other->Cell);
			}
		}
	}
}

bool ARTHUD::ShouldDrawUnitOverlay(const FRTKnowledgeEntry* Entry, bool bIsOwnTeam)
{
	if (bIsOwnTeam)
	{
		return true;
	}

	// 🔴 «Esiste una voce» NON e' «la posizione e' attuale». Un `Remembered` ha una voce per costruzione
	// (`ViewForTeam`: `CellOnly` -> contatto -> voce), e disegnarlo significherebbe disegnarlo DUE volte —
	// il personaggio vero dov'e' davvero, piu' la sagoma dove lo si ricordava. Questo predicato e'
	// COMPLEMENTARE a `ContactGhostTargetForUnit`: o si vede l'unita', o si vede il suo ricordo.
	return Entry != nullptr && Entry->Visibility == ERTKnowledgeVisibility::Live;
}

TOptional<FRTContactGhostTarget> ARTHUD::ContactGhostTargetForUnit(const FRTKnowledgeEntry* Entry, bool bIsOwnTeam)
{
	if (bIsOwnTeam || Entry == nullptr || Entry->Visibility != ERTKnowledgeVisibility::Remembered)
	{
		return TOptional<FRTContactGhostTarget>();
	}
	FRTContactGhostTarget Target;
	Target.Cell = Entry->Cell;
	Target.ContactTurn = Entry->ContactTurn;
	return Target;
}

namespace
{
	/**
	 * Compone una riga della terna.
	 *
	 * `BusyWithoutName` arriva da fuori invece di essere una costante qui dentro perche' il ripiego di uno
	 * slot occupato SENZA nome non e' lo stesso per tutti e tre: sul movimento e' un percorso tracciato a
	 * waypoint — il caso piu' comune del gioco — mentre su principale e reazione un'occupazione senza azione
	 * non ha un nome proprio, e chiamarla «percorso» sarebbe una riga che mente su cosa sta succedendo.
	 *
	 * «libero» invece resta costante: uno slot vuoto e' vuoto allo stesso modo su tutti e tre.
	 */
	FRTSlotLine ComposeOneSlotLine(const TCHAR* SlotLabel, const TCHAR* BusyWithoutName,
		const FRTPlannedSlotView& Slot)
	{
		FRTSlotLine Line;
		Line.bOccupied = Slot.bOccupied;

		if (!Slot.bOccupied)
		{
			Line.Text = FString::Printf(TEXT("%s: libero"), SlotLabel);
		}
		else if (!Slot.DisplayName.IsEmpty())
		{
			Line.Text = FString::Printf(TEXT("%s: %s"), SlotLabel, *Slot.DisplayName.ToString());
		}
		else
		{
			Line.Text = FString::Printf(TEXT("%s: %s"), SlotLabel, BusyWithoutName);
		}

		return Line;
	}
}

TArray<FRTSlotLine> ARTHUD::ComposeSlotLines(const FRTUnitSlotsView& Slots)
{
	return {
		ComposeOneSlotLine(TEXT("Movimento"),  TEXT("percorso"),  Slots.Movement),
		ComposeOneSlotLine(TEXT("Principale"), TEXT("occupata"),  Slots.Main),
		ComposeOneSlotLine(TEXT("Reazione"),   TEXT("armata"),    Slots.Reaction),
	};
}

FRTIntentCertaintyStyle ARTHUD::ComposeIntentCertaintyStyle(const FRTIntentView& View)
{
	FRTIntentCertaintyStyle Style;

	// La grammatica visiva di CP 11.2, fissata il 2026-08-07 e non rinegoziabile qui. I tre livelli arrivano
	// gia' decisi da `URTIntentPrivacyLibrary::ClassifyPlan`: questo `switch` traduce, non classifica.
	//
	// 🔴 **Ogni ramo assegna TUTTI i campi, e non e' verbosita'.** Quando i default della struct sono passati
	// a quelli del livello incerto — la correzione di un finding di review — i due rami certi smisero di
	// riportare `bUncertaintyMark` a `false` e ogni intento a schermo si prese il `?`: `Confirmed` e
	// `Predicted` ereditavano il default che nessuno li obbligava a sovrascrivere. La suite l'ha preso al
	// primo giro (`IntentCertaintyRendering` e `IntentLabelGrammar`, quattro assert), ma la lezione resta:
	// un ramo che assegna solo *alcuni* campi dipende in silenzio dal valore di costruzione, e quel valore
	// e' cambiato una volta e puo' cambiare ancora.
	switch (View.Certainty)
	{
	case ERTIntentCertainty::Confirmed:
		// «Linea piena · ghost pienamente leggibile · nessun `?`». Niente da alleggerire: l'unita' sta ferma
		// e non punta niente, quindi non c'e' un avversario che possa smentirla entro questo turno.
		// ⚠️ **Questi valori non raggiungono nessun `DrawLine`, ed e' un fatto della matrice, non un difetto.**
		// `Confirmed` e' fermo, senza bersaglio e senza scatto: non entra in `if (bMoving)` ne' in
		// `if (bHasTarget)` ne' in `if (bDashing)`, quindi non ha geometria da disegnare. Restano valori
		// **sicuri** per il caso in cui un giorno un elemento nuovo comparisse a questo livello — non una
		// promessa che oggi si veda qualcosa. Cio' che distingue `Confirmed` a schermo e' il CONTENUTO
		// dell'etichetta: non nomina nessun bersaglio, e non porta il `?`.
		Style.LineThickness = 2.5f;
		Style.bDashedLine = false;
		Style.DashDutyCycle = 1.f;
		Style.DashPeriodPx = 18.f;   // irrilevante con la linea piena: tenuto coerente con `Predicted`
		Style.bUncertaintyMark = false;
		break;

	case ERTIntentCertainty::Predicted:
		// «Linea tratteggiata · ghost attenuato». Il collegamento al bersaglio vale nello snapshot corrente:
		// il tratteggio dice «adesso e' valido», non «andra' cosi'».
		// 🔺 **Linea PIENA, ed e' il cambiamento che questa revisione porta.** Prima era tratteggiata come
		// `Uncertain`, e i due livelli restavano separati da un pixel di spessore: la verifica PIE li ha
		// bocciati. Il collegamento al bersaglio e' l'**unico** elemento su cui questi due livelli
		// coesistono, e ora e' pieno per uno e tratteggiato per l'altro — il canale che un occhio umano ha
		// confermato di vedere, speso sull'87 % dei casi reali.
		// «Linea tratteggiata», §16 alla lettera. 🔺 **Torna TRATTEGGIATA il 2026-08-19, e la stesura
		// precedente la dava PIENA — cioe' invertiva una regola normativa in silenzio**, trovato dalla code
		// review. L'inversione era nata per separarla da `Uncertain`, che allora era tratteggiata anche lei;
		// non serve piu', perche' `Uncertain` ha ora lo stile **puntinato** che §16 le assegna e che nessuna
		// stesura aveva mai reso. Trattini lunghi: periodo `18` px con `0,6` acceso.
		Style.LineThickness = 2.f;
		Style.bDashedLine = true;
		Style.DashDutyCycle = 0.6f;
		Style.DashPeriodPx = 18.f;
		Style.bUncertaintyMark = false;
		break;

	default:
		// `Uncertain`, e con lui `Unknown`, per la ragione scritta sulla dichiarazione: un livello mai
		// calcolato riceve la resa che promette meno, mai quella che promette di piu'.
		//
		// ⚠️ **`default:` invece dei due `case` espliciti, ed e' una scelta contro l'abitudine.** Con
		// `Unknown` scritto per nome, un quinto enumeratore aggiunto domani cadrebbe fuori dallo `switch`
		// senza che nessuno se ne accorga; cosi' invece riceve la resa che promette meno.
		// ⚠️ Le tre righe qui sotto **ripetono** i default della struct, e stavolta di proposito: quei
		// default sono anch'essi i valori del livello incerto, per la ragione scritta sulla dichiarazione.
		// La ridondanza e' voluta perche' questo ramo dica cosa disegna anche a chi non risale all'header —
		// e se un giorno divergessero, e' `IntentCertaintyRendering` a cadere, non lo schermo in silenzio.
		// Tratteggiata: ogni linea di questo livello dipende da una scelta che l'avversario puo' ancora fare.
		// ⚠️ Vale sia per il collegamento al bersaglio — dove **decide**, contro la linea piena di
		// `Predicted` — sia per la rotta, dove non distingue niente perche' le rotte sono tutte incerte. La
		// seconda non e' una gradazione mascherata: e' un'affermazione vera su una classe intera, e non
		// toglie leggibilita' a un confronto che non esiste.
		// «Linea puntinata/fading», §16 — **il terzo stile, reso qui per la prima volta**. Punti fitti:
		// periodo `7` px con `0,35` acceso, contro i trattini da `18`/`0,6` di `Predicted`. E' la
		// distinzione che la grammatica prevedeva dall'inizio e che nessuna stesura aveva implementato,
		// costringendole a cercare un secondo canale — prima l'opacita' (inerte), poi lo spessore
		// (bocciato in PIE), poi il colore (gia' occupato dall'identita' di squadra).
		Style.LineThickness = 1.25f;
		Style.bDashedLine = true;
		Style.DashDutyCycle = 0.35f;
		Style.DashPeriodPx = 7.f;
		Style.bUncertaintyMark = true;
		break;
	}

	// La reazione e' un secondo asse e non tocca il primo: un'unita' ferma che tiene pronto un contrattacco
	// ha un piano `Confirmed` — lo pinna `RefactorTactics.UI.IntentCertaintyClassification` — e insieme una
	// reazione che attende un trigger deciso dall'avversario. Il nome vuoto e' l'unico segnale disponibile.
	Style.bReactionArmed = !View.ReactionName.IsEmpty();

	return Style;
}

TArray<TPair<FVector2D, FVector2D>> ARTHUD::ComposeDashSegments(const FVector2D& A, const FVector2D& B,
	float DutyCycle, float PeriodPx)
{
	// Linea piena: un segmento solo, e nessun costo aggiunto rispetto a prima di CP 11.2.
	if (DutyCycle >= 1.f)
	{
		return { TPair<FVector2D, FVector2D>(A, B) };
	}

	const float Len = FVector2D::Distance(A, B);

	// Il periodo arriva dallo stile ed e' in pixel di **schermo**: un segno calcolato in world space si
	// infittirebbe con la distanza fino a tornare pieno, e i due stili — trattini e punti — convergerebbero
	// proprio sulle unita' lontane, dove servono di piu'.
	const float Period = FMath::Max(2.f, PeriodPx);

	// 🔴 **Il tetto e' la ragione per cui questa funzione esiste.** `UCanvas::Project` divide per una `W` solo
	// *clampata* a `UE_KINDA_SMALL_NUMBER`: una cella pochi centimetri davanti al piano della camera passa il
	// test `Z > 0` e proietta a coordinate dell'ordine di `1e6`, che senza limite diventano decine di migliaia
	// di `DrawLine` per segmento, ogni frame. Il valore non e' arbitrario: nessuno schermo ha piu' di qualche
	// migliaio di pixel di diagonale, quindi oltre questo numero di tratti non c'e' piu' niente da vedere —
	// il tetto toglie lavoro invisibile, non dettaglio.
	const int32 MaxSteps = 512;
	const int32 Steps = FMath::Clamp(FMath::RoundToInt(Len / Period), 1, MaxSteps);

	TArray<TPair<FVector2D, FVector2D>> Segments;
	Segments.Reserve(Steps);
	for (int32 s = 0; s < Steps; ++s)
	{
		const float T0 = static_cast<float>(s) / Steps;
		const float T1 = (static_cast<float>(s) + FMath::Clamp(DutyCycle, 0.05f, 1.f)) / Steps;
		Segments.Emplace(FMath::Lerp(A, B, T0), FMath::Lerp(A, B, T1));
	}
	return Segments;
}

FString ARTHUD::ComposeIntentLabel(const FRTIntentView& View, const FRTIntentCertaintyStyle& Style)
{
	FString Label;
	if (!View.ActionName.IsEmpty() && View.bHasTarget)
	{
		Label = FString::Printf(TEXT("%s -> %s"), *View.ActionName.ToString(), *HexCellText(View.TargetCell));
	}
	else if (!View.ActionName.IsEmpty())
	{
		Label = View.ActionName.ToString();
	}
	else if (View.bMoving)
	{
		Label = FString::Printf(TEXT("-> %s"), *HexCellText(View.PlannedCell));
	}
	else if (View.bDashing)
	{
		// 🔴 **Questo ramo mancava, e senza di esso lo scatto finiva nel ramo «fermo».** Prima era solo
		// incompleto; col `?` di CP 11.2 appeso subito dopo diventava **auto-contraddittorio** — «fermo ?» su
		// un'unita' che sta per attraversare la mappa, con la preview magenta dello scatto disegnata accanto.
		// Le due meta' della stessa etichetta affermavano cose opposte. Trovato dalla code review.
		Label = FString::Printf(TEXT("scatto -> %s"), *HexCellText(View.DashCell));
	}
	else
	{
		Label = TEXT("fermo");
	}

	// Il `?` del livello incerto qualifica il PIANO, e la reazione qui sotto ha il proprio.
	if (Style.bUncertaintyMark)
	{
		Label += TEXT(" ?");
	}

	// ⚠️ **Il ramo si apre su `bReactionArmed`, che e' `!ReactionName.IsEmpty()` e nient'altro** — mai su un
	// livello della reazione, che il DTO non porta piu'. Porta sempre il `?`: una reazione armata attende per
	// definizione un trigger che decide l'avversario, quindi e' incerta anche quando il piano che
	// l'accompagna e' `Confirmed`. Per un avversario `ReactionName` e' vuota per costruzione, quindi questo
	// ramo non si apre mai su una vista nemica.
	if (Style.bReactionArmed)
	{
		Label += FString::Printf(TEXT("  (reazione: %s ?)"), *View.ReactionName.ToString());
	}
	return Label;
}

float ARTHUD::NextViewerPlaybackSpeed(float Current)
{
	// La scala di CP 47.2 (#955). Ordinata crescente: la regola qui sotto ne dipende.
	static const float Scale[] = { 1.f, 2.f, 4.f };
	const float Tol = 1e-3f;

	// Un valore non positivo vale «non scelto», esattamente come lo tratta `EffectivePlaybackSpeed`:
	// leggerlo come x1 tiene una sola convenzione fra il modello e il controllo.
	const float From = (Current > 0.f) ? Current : 1.f;

	// La piu' piccola legale STRETTAMENTE maggiore. Il giro (`x4 -> x1`) e il rientro da fuori scala
	// (`x3 -> x4`) sono lo stesso caso, non due rami: sopra il massimo non esiste nessuna legale, e si
	// torna in testa.
	for (const float Speed : Scale)
	{
		if (From < Speed - Tol)
		{
			return Speed;
		}
	}
	return Scale[0];
}

FString ARTHUD::ComposePlaybackSpeedLabel(float ViewerSpeed)
{
	// ⚠️ INTERROGA la normalizzazione, non la rifa'. E' la stessa funzione che `TickPlayback` usa per
	// scorrere: se un giorno la convenzione sul non-positivo cambiasse, l'etichetta segue senza che
	// nessuno se ne ricordi. Non positivo = «non scelto» = x1; un'etichetta «x0» direbbe che la
	// riproduzione e' ferma, che e' un'altra cosa e non e' vera.
	const float Effective = URTPlaybackLibrary::EffectivePlaybackSpeed(ViewerSpeed);

	// Interi quando lo sono — la scala offre `x1 · x2 · x4` — ma il campo e' `EditAnywhere` e puo' portare
	// un valore fuori scala scritto a mano: un `x3` arrotondato da `2.6` sarebbe un numero inventato.
	return FMath::IsNearlyEqual(Effective, FMath::RoundToFloat(Effective), 0.05f)
		? FString::Printf(TEXT("x%d"), FMath::RoundToInt(Effective))
		: FString::Printf(TEXT("x%.1f"), Effective);
}

void ARTHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	// Il team del giocatore, UNA sola costante per tutta la funzione: prima erano due letterali
	// indipendenti (l'argomento di `ComputePlannedHitMarks` e un `PlayerTeam` locale piu' sotto), e la
	// conoscenza di squadra introdotta con la porta ne avrebbe fatto un terzo.
	//
	// 🔴 Da un letterale a una LETTURA: era `= 0`, e con quello alimentava sei consumatori di cui quattro
	// sono filtri di privacy. Ora la fonte e' la stessa unica porta che [D-242] ha scelto per il velo e per
	// tutti gli altri lettori di squadra. Vedi `ARTPlayerState::TeamIdOf`, che porta la ragione per esteso.
	const int32 PlayerTeamId = ARTPlayerState::TeamIdOf(GetOwningPlayerController());

	// Barre HP/scudo sopra ogni unita' viva.
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);

	// Chi verrebbe colpito dai PIANI, non dall'anteprima dell'unita' selezionata: cosi' l'avviso di fuoco
	// amico resta acceso anche mentre si seleziona qualcun altro per muoverlo, che e' esattamente il momento
	// in cui prima spariva.
	TArray<ARTUnit*> AllUnits;
	AllUnits.Reserve(Actors.Num());
	for (AActor* A : Actors)
	{
		if (ARTUnit* U = Cast<ARTUnit>(A)) { AllUnits.Add(U); }
	}
	TSet<FRTCellId> PlannedHitCells;
	TSet<FRTCellId> PlannedAllyHitCells;
	ComputePlannedHitMarks(AllUnits, PlayerTeamId, PlannedHitCells, PlannedAllyHitCells);

	// Recuperato QUI, PRIMA del ciclo delle unita': la vista di conoscenza sotto ha bisogno del
	// TurnManager, e recuperarlo dopo il ciclo (come accadeva prima di questo task) lascerebbe la vista
	// sempre vuota — un filtro che non filtra nulla.
	const ARTTurnManager* TurnManager =
		Cast<ARTTurnManager>(UGameplayStatics::GetActorOfClass(this, ARTTurnManager::StaticClass()));

	// Costruita UNA volta prima del ciclo: `ViewForTeam` e' pura ma il ciclo gira su ogni unita' a ogni
	// frame, e ricostruirla per ognuna sarebbe lavoro ripetuto per un risultato identico.
	FRTKnowledgeView KnowledgeView;
	if (TurnManager)
	{
		TArray<FRTKnowledgeSubject> Subjects;
		Subjects.Reserve(AllUnits.Num());
		for (ARTUnit* U : AllUnits)
		{
			if (!U) { continue; }
			FRTKnowledgeSubject S;
			S.StableUnitId = U->StableUnitId;
			S.TeamId = U->TeamId;
			S.Cell = U->Cell;
			S.HeroId = U->HeroId;
			S.HeroDisplayName = U->HeroDisplayName;
			S.bAlive = U->IsAlive();
			Subjects.Add(S);
		}
		KnowledgeView = URTKnowledgeViewLibrary::ViewForTeam(
			TurnManager->KnowledgeForTeamPublic(PlayerTeamId), Subjects, PlayerTeamId);
	}

	// Geometria della mappa ESAGONALE: unica fonte di scala per ogni conversione cella -> schermo di questa
	// HUD (traccia, anteprime, waypoint, e ora anche la sagoma dell'ultimo contatto). La stessa che usano
	// risoluzione e playback (ARTHexMapActor).
	//
	// ⚠️ Recuperata QUI, PRIMA del ciclo delle unita' — spostata da dopo il ciclo, dov'era finche' solo la
	// visualizzazione degli intenti (sotto) ne aveva bisogno: il ciclo ora chiama `UpdateContactGhost`, che
	// richiede la stessa conversione cella -> mondo per posizionare la sagoma di un ricordo (CP 13.5).
	//
	// ⚠️ **Non e' l'unico punto del file che interroga `ARTHexMapActor`**, e la riga che lo affermava era
	// falsa gia' quando e' stata scritta. Il secondo e' il pannello della terna piu' sotto in questa stessa
	// funzione (la riga «TIRO: N celle»), che lo raggiunge con un meccanismo DIVERSO —
	// `Cast<ARTHexMapActor>(UGameplayStatics::GetActorOfClass(...))` invece di `ARTHexMapActor::FindInWorld`.
	// Cio' che e' vero e' piu' stretto: e' l'unico punto che ne ricava la GEOMETRIA (origine, dimensione
	// della cella, altezza del layer), quindi ogni conversione cella -> schermo di questa HUD nasce da qui.
	// Unificare i due meccanismi non e' compito di questa riga — ma nominarli entrambi si', perche' chi
	// cercasse «dove si prende la mappa» seguendo la vecchia frase ne troverebbe uno solo.
	FVector Origin = FVector::ZeroVector;
	float HexSize = 150.f;
	float LayerH = 250.f;
	const URTHexMapAsset* Map = nullptr;
	if (const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld()))
	{
		Map = HexMap->GetHexContext(Origin, HexSize, LayerH);
	}

	for (AActor* Actor : Actors)
	{
		ARTUnit* Unit = Cast<ARTUnit>(Actor);
		if (!Unit || !Unit->IsAlive())
		{
			continue;
		}

		// La voce di conoscenza si cerca UNA volta per unita' e alimenta ENTRAMBE le decisioni sotto
		// (`ShouldDrawUnitOverlay` e `ContactGhostTargetForUnit`) — non due `FindEntry` separate per la
		// stessa domanda (review). Per la propria squadra non si cerca nemmeno: entrambe le funzioni
		// decidono da `bIsOwnTeam` prima di guardare `Entry`, esattamente come faceva prima questo ramo.
		const bool bIsOwnTeam = (Unit->TeamId == PlayerTeamId);
		const FRTKnowledgeEntry* Entry = bIsOwnTeam
			? nullptr
			: URTKnowledgeViewLibrary::FindEntry(KnowledgeView, Unit->StableUnitId);

		// `ShouldDrawUnitOverlay` e `ContactGhostTargetForUnit` sono statiche e PURE (dichiarate in
		// `RTHUD.h`, testate senza montare un HUD): ricalcolare una qualunque delle due regole inline qui
		// sarebbe una seconda definizione, e le due potrebbero divergere (review).
		const bool bIsKnownToObserver = ShouldDrawUnitOverlay(Entry, bIsOwnTeam);

		// Applica lo stato di conoscenza PRIMA del filtro sottostante: altrimenti l'unita' saltata dal
		// `continue` qui sotto non riceverebbe mai il comando e resterebbe visibile.
		Unit->SetKnownToObserver(bIsKnownToObserver);

		// Sagoma dell'ultimo contatto (Task 6b, CP 13.5): SPENTA per default, e accesa SOLO per un
		// ricordo (`Remembered`) di un nemico. Gira per OGNI unita' viva — prima del filtro sottostante —
		// perche' spegnerla vale anche per chi quel filtro sta per saltare: un nemico senza voce nella vista
		// (`Rejected`, ricordo scaduto) non ha nemmeno una sagoma, e resterebbe accesa dall'ultima volta se
		// il `continue` la saltasse prima di arrivarci.
		if (const TOptional<FRTContactGhostTarget> GhostTarget = ContactGhostTargetForUnit(Entry, bIsOwnTeam))
		{
			// `GhostTarget->Cell` e' quella del CONTATTO (Task 2), mai la posizione attuale dell'attore:
			// `ContactGhostTargetForUnit` non riceve nemmeno `Unit`, quindi non puo' leggerla per sbaglio.
			const int32 CurrentTurn = TurnManager ? TurnManager->GetTurnNumber() : 0;
			Unit->UpdateContactGhost(HexCellWorld(GhostTarget->Cell, Origin, HexSize, LayerH),
				GhostTarget->ContactTurn, CurrentTurn);
		}
		else
		{
			// Niente da ricordare: propria squadra, nemico `Live`, o nemico `Rejected`.
			Unit->HideContactGhost();
		}

		// Filtro di conoscenza (CP 13.5): un'unita' avversaria si disegna solo se la squadra del giocatore
		// la VEDE ORA (`Live`). Un ricordo (`Remembered`) non si disegna qui — lo disegna la sagoma qui
		// sopra, alla cella del contatto: le due strade sono complementari, mai contemporanee.
		// La propria squadra si disegna sempre.
		if (!bIsKnownToObserver)
		{
			continue;
		}

		const FVector Head = Unit->GetActorLocation() + FVector(0.f, 0.f, WorldHeadOffset);
		const FVector Screen = Project(Head);
		if (Screen.Z <= 0.f)
		{
			continue; // dietro la camera
		}

		// Il NOME si compone qui, prima di disegnare, perche' la sua larghezza serve al vincolo orizzontale:
		// l'etichetta e' spesso piu' larga della barra, e vincolare sulla sola barra la lascerebbe uscire.
		FString HeroName = ARTUnit::DisplayLabel(Unit->HeroDisplayName, Unit->HeroId, Unit->GetName());
		FLinearColor NameColor = ARTUnit::TeamColorFor(Unit->TeamId,
			FLinearColor(0.55f, 0.75f, 1.f, 1.f), FLinearColor(1.f, 0.62f, 0.55f, 1.f));
		if (PlannedAllyHitCells.Contains(Unit->Cell))
		{
			// Fuoco amico: l'avviso deve essere piu' forte del colore di squadra, perche' e' l'unico caso
			// in cui chi guarda potrebbe voler cambiare idea. E deve restare finche' il piano esiste, non
			// finche' l'unita' e' selezionata.
			HeroName = TEXT("! ") + HeroName;
			NameColor = FLinearColor(1.f, 0.6f, 0.12f, 1.f);
		}
		else if (PlannedHitCells.Contains(Unit->Cell))
		{
			HeroName = TEXT("* ") + HeroName;
			NameColor = FLinearColor(1.f, 0.35f, 0.3f, 1.f);
		}

		float NameW = 0.f;
		float NameH = 0.f;
		GetTextSize(HeroName, NameW, NameH, nullptr, 0.9f);

		// L'ancora si vincola al viewport PRIMA di disegnare: senza, un'unita' vicina alla camera perde
		// l'intera sovrapposizione — nome e barre insieme, che condividono questa Y (#729).
		// Il blocco va da `Y - 36` (riga del nome) a `Y + BarHeight + 4` (fondo della barra energia).
		const FVector2D Anchor = ClampOverlayAnchor(
			FVector2D(Screen.X, Screen.Y - BarHeight),
			FMath::Max(BarWidth, NameW) * 0.5f,
			/*AboveAnchor=*/ 36.f,
			/*BelowAnchor=*/ BarHeight + 4.f,
			FVector2D(Canvas->SizeX, Canvas->SizeY),
			/*Margin=*/ 4.f);

		const float CenterX = Anchor.X;
		const float X = CenterX - BarWidth * 0.5f;
		const float Y = Anchor.Y;

		// Sfondo.
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X - 1.f, Y - 1.f, BarWidth + 2.f, BarHeight + 2.f);

		// HP: verde (pieno) -> rosso (vuoto).
		const float HpFrac = Unit->MaxHealth > 0 ? FMath::Clamp((float)Unit->Health / Unit->MaxHealth, 0.f, 1.f) : 0.f;
		DrawRect(FLinearColor(1.f - HpFrac, HpFrac, 0.15f, 1.f), X, Y, BarWidth * HpFrac, BarHeight);

		// Scudo: barretta ciano sopra la barra HP (proporzionale a MaxHealth).
		if (Unit->Shield > 0 && Unit->MaxHealth > 0)
		{
			const float ShieldFrac = FMath::Clamp((float)Unit->Shield / Unit->MaxHealth, 0.f, 1.f);
			DrawRect(FLinearColor(0.2f, 0.8f, 1.f, 1.f), X, Y - 4.f, BarWidth * ShieldFrac, 3.f);
		}

		// Energia: barretta sotto la barra HP (oro se ultimate pronta, giallo scuro se in carica).
		if (Unit->MaxEnergy > 0)
		{
			const float EnergyFrac = FMath::Clamp((float)Unit->Energy / Unit->MaxEnergy, 0.f, 1.f);
			const bool bReady = Unit->Energy >= Unit->MaxEnergy;
			const FLinearColor EColor = bReady ? FLinearColor(1.f, 0.85f, 0.1f, 1.f) : FLinearColor(0.5f, 0.45f, 0.1f, 1.f);
			DrawRect(EColor, X, Y + BarHeight + 1.f, BarWidth * EnergyFrac, 3.f);
		}

		// Marker di status sopra la barra HP.
		FString StatusStr;
		if (Unit->HasStatus(TAG_Status_Root)) { StatusStr = TEXT("ROOT"); }
		else if (Unit->HasStatus(TAG_Status_Slow)) { StatusStr = TEXT("SLOW"); }
		if (!StatusStr.IsEmpty())
		{
			DrawText(StatusStr, FLinearColor(1.f, 0.6f, 0.2f, 1.f), X, Y - 20.f, nullptr, 0.8f);
		}

		// NOME dell'eroe, sopra a tutto e centrato sulla barra. Senza, quattro cilindri identici rendono
		// impossibile dire chi sta facendo cosa — e un giudizio sul bot o sul ritmo della partita, che e' cio'
		// che il playtest deve dare, non varrebbe nulla. Posizione FISSA (non sotto lo status): un'etichetta che
		// salta quando arriva un ROOT si legge peggio di una ferma.
		// Il nome CANONICO del catalogo (D-120), non l'ID stabile: `Hero.Gadget` si legge `Gadget`. Il ripiego
		// sull'ID resta dentro `DisplayLabel` per le unita' che nessun eroe ha configurato.
		// CHI viene colpito, marcato sull'UNITA' e non solo sulla cella — il prefisso e il colore sono stati
		// decisi sopra, insieme al nome, perche' la larghezza serviva al vincolo.
		//
		// L'anteprima a terra dice quali CELLE entrano nella zona; la domanda che ci si fa guardando lo schermo
		// e' un'altra — «questo cilindro lo prendo o no?». Sono due informazioni diverse, e finche' c'era solo
		// la prima l'anteprima si vedeva e non si capiva (osservato in PIE il 2026-08-08: «non capisco se sto
		// facendo un tiro e se nel tiro si interseca con un cilindro»).
		//
		// Il nome sopra la testa e' il posto giusto: c'e' gia', l'occhio ci va gia' per sapere chi e' chi, e
		// non aggiunge un elemento nuovo da imparare.
		//
		// Ombra di 1px: il testo chiaro su cielo chiaro sparirebbe, e la camera tattica guarda spesso il vuoto.
		DrawText(HeroName, FLinearColor(0.f, 0.f, 0.f, 0.75f),
			CenterX - NameW * 0.5f + 1.f, Y - 36.f + 1.f, nullptr, 0.9f);
		DrawText(HeroName, NameColor, CenterX - NameW * 0.5f, Y - 36.f, nullptr, 0.9f);
	}

	// Traccia post-lock: il percorso realmente eseguito nell'ultima risoluzione (grigio, sotto le preview).
	if (TurnManager && TurnManager->GetPhase() == ERTMatchPhase::Planning)
	{
		// Il filtro di conoscenza di [D-223], e **non si decide qui**: la rotta porta gia' un verdetto per
		// cella, congelato quando e' stata percorsa. Questo ciclo consuma e non costruisce nulla — niente
		// `ViewForTeam`, niente `GetAllActorsOfClass`, nessuna seconda rilettura della regola.
		//
		// 🔴 **`VisibleTrailFor` TRONCA**, e per questo il ciclo scorre il suo risultato invece di
		// `Route.Cells`: la traccia mostra il tratto che l'osservatore ha visto e finisce dove ha perso il
		// soggetto. Saltare le celle non ammesse tenderebbe un segmento fra due celle non adiacenti, proprio
		// sopra il tratto da nascondere.
		//
		// ⚠️ La regola vive in una statica PURA di `ARTTurnManager`, gemella di `ComposeVisibleLogLines`:
		// `DrawHUD` non ha copertura headless, quindi cio' che si puo' sbagliare deve stare dove i test
		// arrivano.
		const FLinearColor TrailColor(0.6f, 0.6f, 0.6f, 0.5f);
		for (const FRTMoveRoute& Route : TurnManager->GetLastMoveRoutes())
		{
			const TArray<FRTCellId> Trail = ARTTurnManager::VisibleTrailFor(Route, PlayerTeamId);
			for (int32 i = 1; i < Trail.Num(); ++i)
			{
				const FVector A = Project(HexCellWorld(Trail[i - 1], Origin, HexSize, LayerH));
				const FVector B = Project(HexCellWorld(Trail[i], Origin, HexSize, LayerH));
				if (A.Z > 0.f && B.Z > 0.f)
				{
					DrawLine(A.X, A.Y, B.X, B.Y, TrailColor, 1.5f);
				}
			}
		}
	}

	// Visualizzazione degli INTENTI di pianificazione (fase Planning, non durante il playback).
	//
	// Invariante #6 (privacy dell'intento), esteso alle reazioni con CP 5.4. La UI NON legge piu' lo stato di
	// pianificazione delle unita': costruisce i piani autorevoli, li fa filtrare per squadra da
	// `URTIntentPrivacyLibrary::FilterForTeam` e disegna SOLO le viste che tornano indietro.
	//
	// La differenza non e' stilistica. Prima il ciclo scorreva tutte le unita', leggeva il piano completo anche
	// dei nemici e decideva di non disegnarlo: un occultamento GRAFICO, cioe' un dato presente sul client e
	// nascosto a schermo — leggibile con qualunque strumento, e insostenibile quando arrivera' la rete (M10).
	// Ora un piano avversario non rivelato non compare proprio fra le viste, e la reazione di un alleato non
	// viene mai copiata in una vista avversaria.
	if (TurnManager && TurnManager->GetPhase() == ERTMatchPhase::Planning && !TurnManager->IsResolving())
	{
		// 1. RACCOGLI i piani autorevoli (in rete: lato server, mai spediti cosi' come sono).
		// ⚠️ Il ciclo che li costruiva stava qui fino al 2026-08-24 ed e' ora in `URTHudViewModel`: lo
		// condivide con `rt.Debug.DrawIntent` (CP 11.4, #80), che deve mostrare **gli stessi** intenti che
		// questa HUD disegna. Con due costruzioni separate, un campo aggiunto a `FRTPlannedIntent` finirebbe
		// in una sola delle due.
		const TArray<FRTPlannedIntent> Authoritative = URTHudViewModel::BuildAuthoritativeIntents(Actors);

		// 2. FILTRA per l'osservatore. Da qui in giu' lo stato completo non si tocca piu'.
		const TArray<FRTIntentView> Views = URTIntentPrivacyLibrary::FilterForTeam(PlayerTeamId, Authoritative);

		// Disegna cio' che `ComposeDashSegments` ha gia' deciso. Qui non resta nessuna scelta: il conteggio
		// dei tratti, il rapporto acceso/spento e il tetto vivono nella statica, dove un test li raggiunge.
		auto DrawIntentLine = [this](const FVector2D& A, const FVector2D& B, const FLinearColor& C,
			const FRTIntentCertaintyStyle& S)
		{
			// ⚠️ **Il colore arriva INTATTO, ed e' una scelta.** In questa HUD il colore e' gia' l'identita'
			// di squadra — ciano contro giallo — e una stesura precedente lo sbiadiva secondo la certezza,
			// togliendo croma a ogni unita' in movimento per un confronto che su quell'elemento non esiste.
			// Due semantiche sullo stesso canale, e la seconda pagata dalla prima. Qui la certezza parla col
			// tratteggio, che e' libero.
			const float Duty = S.bDashedLine ? S.DashDutyCycle : 1.f;
			for (const TPair<FVector2D, FVector2D>& Seg : ComposeDashSegments(A, B, Duty, S.DashPeriodPx))
			{
				DrawLine(Seg.Key.X, Seg.Key.Y, Seg.Value.X, Seg.Value.Y, C, S.LineThickness);
			}
		};

		// 3. DISEGNA le sole viste ricevute.
		for (const FRTIntentView& View : Views)
		{
			const bool bOwn = View.bIsAlly;
			const bool bHasPlan = View.bMoving || View.bHasTarget || !View.ActionName.IsEmpty()
				|| View.bDashing || !View.ReactionName.IsEmpty();
			if (bOwn && !bHasPlan)
			{
				continue; // unita' propria senza ordine: niente da mostrare
			}

			const FLinearColor Color = bOwn
				? FLinearColor(0.2f, 0.9f, 1.f, 1.f)   // ciano: le tue unita'
				: FLinearColor(1.f, 0.9f, 0.2f, 1.f);  // giallo: nemico rivelato

			// CP 11.2 — la resa arriva dal livello che la vista PORTA gia' calcolato. Nessun `View.bMoving`
			// da qui in giu' per decidere lo stile: quella e' la regola, e vive in `ClassifyPlan`.
			const FRTIntentCertaintyStyle Style = ComposeIntentCertaintyStyle(View);

			// Descrizione testuale dell'intento, dalla sola vista. Composta da una statica pura: il `?` del
			// livello e quello della reazione armata sono grammatica visiva, e in una format string qui
			// dentro nessun test headless li raggiungerebbe.
			const FString Intent = ComposeIntentLabel(View, Style);

			// Etichetta sopra la testa, posizionata dalla CELLA (identita' stabile), non da un pointer all'Actor.
			const FVector Head = HexCellWorld(View.OwnerCell, Origin, HexSize, LayerH) + FVector(0.f, 0.f, WorldHeadOffset);
			const FVector HeadScreen = Project(Head);
			if (HeadScreen.Z > 0.f)
			{
				const TCHAR* Prefix = bOwn ? TEXT("[PIANO] ") : TEXT("[REVEAL] ");
				const FString Label = FString(Prefix) + Intent;

				// Stesso vincolo della sovrapposizione dell'unita' (#729): l'ancora nasce dallo stesso offset
				// world space, quindi soffriva dello stesso difetto — l'intento di un'unita' vicina alla
				// camera finiva sopra il bordo. Qui il blocco e' una riga sola.
				float LabelW = 0.f;
				float LabelH = 0.f;
				GetTextSize(Label, LabelW, LabelH, nullptr, 0.85f);
				const FVector2D LabelAnchor = ClampOverlayAnchor(
					FVector2D(HeadScreen.X, HeadScreen.Y),
					FMath::Max(BarWidth, LabelW) * 0.5f,
					/*AboveAnchor=*/ 36.f,
					/*BelowAnchor=*/ 0.f,
					FVector2D(Canvas->SizeX, Canvas->SizeY),
					/*Margin=*/ 4.f);

				DrawText(Label, Color, LabelAnchor.X - LabelW * 0.5f, LabelAnchor.Y - 36.f, nullptr, 0.85f);
			}

			// Percorso pianificato: la rotta composita se la vista la porta, altrimenti lo stesso A* dell'autorita'.
			if (View.bMoving)
			{
				const TArray<FRTCellId> PathCells = (View.PlannedPath.Num() >= 2)
					? View.PlannedPath
					: URTHexPathLibrary::FindPath(Map, View.OwnerCell, View.PlannedCell).Path;

				for (int32 i = 1; i < PathCells.Num(); ++i)
				{
					const FVector A = Project(HexCellWorld(PathCells[i - 1], Origin, HexSize, LayerH));
					const FVector B = Project(HexCellWorld(PathCells[i], Origin, HexSize, LayerH));
					if (A.Z > 0.f && B.Z > 0.f)
					{
						DrawIntentLine(FVector2D(A.X, A.Y), FVector2D(B.X, B.Y), Color, Style);
					}
				}

				const FVector DestScreen = Project(HexCellWorld(View.PlannedCell, Origin, HexSize, LayerH));
				if (DestScreen.Z > 0.f)
				{
					// 🔴 **La destinazione NON e' graduata, e la prima stesura la graduava — sbagliando due
					// volte.** Questo blocco vive dentro `if (View.bMoving)`, e `ClassifyPlan` restituisce
					// `Uncertain` ogni volta che `bMoving`: il livello qui e' **sempre** lo stesso, quindi
					// attenuare non distingue niente e toglie soltanto leggibilita' — il rettangolo passava da
					// alpha `0.35` a `0.105`, in permanenza, per ogni unita' in movimento. E' lo stesso
					// argomento con cui la preview dello scatto e' esentata poche righe piu' sotto, che non era
					// stato applicato qui. Trovato dalla code review.
					DrawRect(FLinearColor(Color.R, Color.G, Color.B, 0.35f),
						DestScreen.X - 12.f, DestScreen.Y - 12.f, 24.f, 24.f);
				}
			}

			// Marker sui waypoint cliccati: la vista li porta solo per le unita' proprie.
			// ⚠️ Non graduati, per la stessa ragione della destinazione: i waypoint appartengono a un piano di
			// movimento, e un piano di movimento e' `Uncertain` per costruzione.
			for (const FRTCellId& WP : View.PlannedWaypoints)
			{
				const FVector WPScreen = Project(HexCellWorld(WP, Origin, HexSize, LayerH));
				if (WPScreen.Z > 0.f)
				{
					DrawRect(Color, WPScreen.X - 5.f, WPScreen.Y - 5.f, 10.f, 10.f);
				}
			}

			// Preview dello SCATTO pianificato (fase Dash): percorso e destinazione in MAGENTA.
			//
			// La traiettoria si disegna come la fase Dash la ESEGUIRA' (#142): una mobilita' lineare va dritta
			// e non gira gli angoli, una a budget (`Action.Sprint`) segue il grafo. Disegnare l'A* per uno
			// scatto lineare mostrerebbe un percorso curvo attorno a un ostacolo che in realta' lo ferma — e
			// la leggibilita' tattica e' un pilastro, non un dettaglio estetico.
			//
			// ⚠️ **La preview dello scatto NON e' graduata dalla certezza, e non e' una svista.** `bDashing`
			// implica `Uncertain` per costruzione (`ClassifyPlan` guarda `bMoving || bDashing`), quindi
			// applicarle lo stile la lascerebbe *sempre* allo stesso livello: un simbolo che non varia non
			// informa, ed e' il difetto esatto per cui `ReactionCertainty` e' uscito dal DTO. Il colore
			// di FASE distingue gia' lo scatto dal movimento normale, che e' l'informazione che serve qui.
			if (View.bDashing && Map)
			{
				const TArray<FRTCellId> DPath = URTMovementActionLibrary::IsLinear(View.DashStyle)
					? URTHexLibrary::HexLine(View.OwnerCell, View.DashCell)
					: URTHexPathLibrary::FindPath(Map, View.OwnerCell, View.DashCell).Path;
				// **D-234**: la fase `Dash` prende in PRESTITO `#009E73` da D-233. L'overlay tiene un
				// vocabolario proprio — il colore ci dice l'IDENTITA' di squadra — ma questa riga era gia'
				// un'eccezione: `DashColor` e' costruito FUORI da ogni ramo su `bOwn`, quindi la linea di
				// scatto aveva gia' perso la squadra ed era gia' colorata per fase. Per caso, pero'.
				//
				// 🔴 Il magenta che stava qui non apparteneva a NESSUNA palette, e non era neutro: misurato,
				// distava `dE 20.7` dal ciano di squadra in deuteranopia e `20.9` dal giallo in tritanopia —
				// due dicromazie a meno di un punto dalla soglia `20`. Dopo il cambio i due peggiori casi
				// sono `43.9` e `51.7`. Il gate `T9` ora misura questa coppia e la rilegge DA QUI.
				//
				// ⚠️ `FromSRGBColor` NON e' opzionale: il costruttore prende valori LINEARI, e passargli
				// `0/158/115` diviso 255 darebbe una tinta slavata — lo stesso errore documentato in
				// `Map/RTHexMapActor.cpp:909`, che e' anche il precedente della forma usata qui.
				const FLinearColor DashColor = FLinearColor::FromSRGBColor(FColor(0, 158, 115));
				for (int32 i = 1; i < DPath.Num(); ++i)
				{
					const FVector DA = Project(HexCellWorld(DPath[i - 1], Origin, HexSize, LayerH));
					const FVector DB = Project(HexCellWorld(DPath[i], Origin, HexSize, LayerH));
					if (DA.Z > 0.f && DB.Z > 0.f) { DrawLine(DA.X, DA.Y, DB.X, DB.Y, DashColor, 2.5f); }
				}
				const FVector DDest = Project(HexCellWorld(View.DashCell, Origin, HexSize, LayerH));
				if (DDest.Z > 0.f) { DrawRect(FLinearColor(DashColor.R, DashColor.G, DashColor.B, 0.4f), DDest.X - 10.f, DDest.Y - 10.f, 20.f, 20.f); }
			}

			// Linea verso il bersaglio d'attacco pianificato (dalla CELLA del bersaglio, non dal suo Actor).
			//
			// ⚠️ **E' l'UNICO elemento grafico su cui il livello varia davvero, e quindi l'unico che una
			// distinzione informa.** Un bersaglio senza movimento e' `Predicted`, un bersaglio mentre ci si
			// sposta e' `Uncertain`: qui la linea passa da **piena** a **tratteggiata**, ed e' il canale che
			// la seduta PIE del 2026-08-19 ha confermato visibile. La rotta e la destinazione, invece,
			// esistono solo quando l'unita' si muove — cioe' sempre allo stesso livello.
			// 🔴 **Questo commento diceva «il tratto passa da 2,0 a 1,25 e il tratteggio da mezzo a un terzo
			// acceso, e la differenza si vede»**: e' esattamente la tesi che quella seduta ha BOCCIATO, ed e'
			// rimasta scritta accanto al codice che descriveva mentre l'intestazione della struct, 600 righe
			// piu' su, gia' diceva il contrario. Trovato dalla code review. Chi cercava se lo spessore possa
			// portare un confronto trovava per primo la risposta sbagliata.
			// I due livelli qui coprono l'**87 %** dei casi reali (`Predicted` 51,1 % + `Uncertain` 36,1 %).
			if (View.bHasTarget && HeadScreen.Z > 0.f)
			{
				const FVector TgtScreen = Project(HexCellWorld(View.TargetCell, Origin, HexSize, LayerH) + FVector(0.f, 0.f, WorldHeadOffset));
				if (TgtScreen.Z > 0.f)
				{
					DrawIntentLine(FVector2D(HeadScreen.X, HeadScreen.Y), FVector2D(TgtScreen.X, TgtScreen.Y),
						Color, Style);
				}
			}
		}
	}

	// Letta UNA volta per frame, non a ogni pannello: tre `GetValueOnGameThread()` nello stesso `DrawHUD`
	// potrebbero in teoria vedere valori diversi se la console cambiasse a meta' — e mezzo HUD acceso
	// sarebbe piu' difficile da diagnosticare di entrambi gli stati interi.
	const bool bCanvasPanels = CVarHudCanvasPanels.GetValueOnGameThread() != 0;

	// Barra di stato in alto: turno, fase e timer/avanzamento. (§4.1 — vedi `rt.HUD.CanvasPanels`)
	if (bCanvasPanels && TurnManager)
	{
		// Contatore del turno: con un formato in vigore mostra anche il limite, altrimenti resta il solo
		// numero — un "su 0" direbbe che la partita e' gia' scaduta, che non e' cio' che accade.
		// «Round», non «Turno»: nel progetto il TURNO e' la sequenza di fasi DENTRO il round
		// (`Planning -> Prep -> Dash -> Blast -> Move -> Cleanup`), e il contatore qui e' quello che si
		// confronta con `RoundLimit` del formato. Il DoD di CP 11.1 lo chiede esplicitamente, e la riga
		// diceva «Turno %d/%d» — cioe' nominava una cosa e ne mostrava un'altra.
		const int32 RoundLimit = TurnManager->GetMatchRules().RoundLimit;
		const FString TurnCounter = RoundLimit > 0
			? FString::Printf(TEXT("Round %d/%d"), TurnManager->GetTurnNumber(), RoundLimit)
			: FString::Printf(TEXT("Round %d"), TurnManager->GetTurnNumber());

		FString Status;
		if (TurnManager->IsResolving())
		{
			// Durante il playback: fase in riproduzione + avanzamento + come saltare.
			const int32 Pct = FMath::RoundToInt(TurnManager->GetPlaybackProgress01() * 100.f);
			Status = FString::Printf(TEXT("%s  -  Risoluzione: %s  [%d%%]  (Spazio: salta)"),
				*TurnCounter, *TurnManager->GetPlaybackPhaseName(), Pct);
		}
		else
		{
			const TCHAR* PhaseName = TEXT("");
			switch (TurnManager->GetPhase())
			{
			case ERTMatchPhase::Planning:   PhaseName = TEXT("Pianificazione"); break;
			case ERTMatchPhase::MatchEnded: PhaseName = TEXT("Fine"); break;
			default:                        PhaseName = TEXT("Risoluzione"); break;
			}
			Status = FString::Printf(TEXT("%s  -  %s"), *TurnCounter, PhaseName);
			const float Remaining = TurnManager->GetPlanningTimeRemaining();
			if (TurnManager->GetPhase() == ERTMatchPhase::Planning && Remaining > 0.f)
			{
				Status += FString::Printf(TEXT("  -  %.0fs"), FMath::CeilToFloat(Remaining));
			}
		}

		// Il progresso sull'OBIETTIVO contendibile (`CP 10.2`, `#75`). Il punto lo assegna il Cleanup e lo
		// registra nel TurnLog da tempo; la DoD lo voleva anche «nell'HUD», e fino a qui nessuno lo mostrava.
		//
		// ⚠️ **Solo se la mappa DICHIARA un obiettivo.** Su una mappa che non ne ha, `0-0` non sarebbe una
		// partita in parita': sarebbe un punteggio inventato per una gara che non si sta correndo. E' la
		// stessa reticenza che il resolver ha gia' — `RTTurnManager` non scrive la voce di log senza
		// `HasObjectiveCell()`, e `Objectives.SilentWithoutObjectiveCell` la misura.
		if (const ARTHexMapActor* ObjectiveMapActor = ARTHexMapActor::FindInWorld(GetWorld()))
		{
			FVector IgnoredOrigin = FVector::ZeroVector;
			float IgnoredSize = 0.f;
			float IgnoredLayerH = 0.f;
			const URTHexMapAsset* ObjectiveMap = ObjectiveMapActor->GetHexContext(IgnoredOrigin, IgnoredSize, IgnoredLayerH);
			if (ObjectiveMap && ObjectiveMap->HasObjectiveCell())
			{
				// Interi, come nel resolver e nel log: la riga mostrata dev'essere confrontabile con la
				// colonna `Amount` del TurnLog, non somigliarle.
				Status += FString::Printf(TEXT("  -  Obiettivo %d-%d"),
					TurnManager->GetTeamScore(0), TurnManager->GetTeamScore(1));

				// La soglia viene dal FORMATO, come `RoundLimit` qui sopra, e si tace quando e' `0`: la via
				// per obiettivo e' disattivata in v0.1, e «a 0» leggerebbe come «gia' vinta».
				const int32 ScoreToWin = TurnManager->GetMatchRules().ScoreToWin;
				if (ScoreToWin > 0)
				{
					Status += FString::Printf(TEXT(" (a %d)"), ScoreToWin);
				}
			}
		}

		// Il controllo di velocita' (CP 47.7, #1015). Sta nella riga di stato e non in un pannello suo
		// perche' quella riga e' l'unico elemento sempre visibile durante la risoluzione — che e' quando
		// serve — e perche' `progettazione-hud.md` §31 mette turn/phase/timer fra i persistenti.
		//
		// ⚠️ **Mostrato anche fuori dalla risoluzione, di proposito.** Chi guarda una partita non
		// presidiata sceglie il ritmo PRIMA che il round parta: una manopola che compare solo mentre
		// scorre costringe a inseguirla. Il tetto fuori dal playback non morde, quindi li' l'etichetta e'
		// un numero solo.
		//
		// ⚠️ Il tasto e' nominato accanto al valore, come `(Spazio: salta)` due righe sopra: un HUD in
		// Canvas non ha nulla su cui passare il mouse, quindi una scorciatoia non scritta e' una
		// scorciatoia che non esiste.
		Status += FString::Printf(TEXT("  -  Velocita': %s (V)"),
			*ComposePlaybackSpeedLabel(TurnManager->ViewerPlaybackSpeed));
		float TW = 0.f, TH = 0.f;
		GetTextSize(Status, TW, TH, nullptr, 1.2f);
		DrawText(Status, FLinearColor::White, (Canvas->SizeX - TW) * 0.5f, 16.f, nullptr, 1.2f);
	}

	// Banda «questa non e' una partita»: quando il GameMode sta eseguendo uno scenario, la partita normale non
	// viene allestita e mancano unita' proprie, selezione e barra abilita'. Senza questa riga il sintomo non
	// punta alla causa — la spiegazione esiste, ma solo nell'Output Log.
	//
	// Fondo scuro dietro il testo e non solo testo colorato: e' l'unico elemento dell'HUD che deve farsi
	// leggere anche sopra la mappa, e chi lo legge sta gia' cercando di capire perche' non vede nulla.
	if (const ARTGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ARTGameMode>() : nullptr)
	{
		const FString Banner = GM->GetScenarioBannerText();
		if (!Banner.IsEmpty())
		{
			float BW = 0.f, BH = 0.f;
			GetTextSize(Banner, BW, BH, nullptr, 1.1f);
			const float BX = (Canvas->SizeX - BW) * 0.5f;
			const float BY = 44.f;
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.7f), BX - 8.f, BY - 4.f, BW + 16.f, BH + 8.f);
			DrawText(Banner, FLinearColor(1.f, 0.75f, 0.2f, 1.f), BX, BY, nullptr, 1.1f);
		}
	}

	// Combat log in basso a sinistra (dal piu' vecchio in alto al piu' recente in basso).
	// (§4.1 — vedi `rt.HUD.CanvasPanels`)
	if (bCanvasPanels && TurnManager)
	{
		const TArray<FString> Events = TurnManager->GetRecentEventsForTeam(PlayerTeamId);
		const float LineH = 16.f;
		float Y = Canvas->SizeY - 24.f - LineH * (Events.Num() - 1);
		for (const FString& Line : Events)
		{
			DrawText(Line, FLinearColor(0.85f, 0.85f, 0.85f, 1.f), 16.f, Y, nullptr, 1.f);
			Y += LineH;
		}
	}

	// Barra abilita' dell'unita' selezionata (in basso al centro) e terna degli slot (in basso a destra).
	// (§4.1 — vedi `rt.HUD.CanvasPanels`)
	//
	// ⚠️ Le due sezioni condividono una guardia sola perche' condividono il blocco della selezione: la
	// barra abilita' e la terna leggono entrambe `Sel`, e separarle vorrebbe dire duplicare il `Cast` e la
	// `GetSelectedUnit()`. Se in futuro servisse spegnerne una sola, il punto dove dividerle e' qui.
	if (const ARTPlayerController* RTPC = Cast<ARTPlayerController>(GetOwningPlayerController());
		bCanvasPanels && RTPC)
	{
		if (const ARTUnit* Sel = RTPC->GetSelectedUnit())
		{
			const float LineH = 18.f;
			float Y = Canvas->SizeY - 24.f - LineH * (Sel->NumAbilities() - 1);
			const float X = Canvas->SizeX * 0.45f;

			// COSA sto per fare, in una riga sopra la barra. La barra dice quali abilita' HO; questa dice se
			// una zona e' davvero puntata adesso e quanto e' larga — la differenza fra «sto scegliendo» e «sto
			// per tirare», che dai soli contorni a terra non si legge.
			if (const ARTHexMapActor* HexMap = Cast<ARTHexMapActor>(
					UGameplayStatics::GetActorOfClass(this, ARTHexMapActor::StaticClass())))
			{
				const int32 NumHit = HexMap->NumPreviewHitCells();
				if (NumHit > 0)
				{
					const int32 NumAlly = HexMap->NumPreviewAllyHitCells();
					FString Zona = FString::Printf(TEXT("TIRO: %d celle"), NumHit);
					if (NumAlly > 0)
					{
						Zona += FString::Printf(TEXT("  -  %d ALLEATO%s NELLA ZONA"),
							NumAlly, NumAlly > 1 ? TEXT("/I") : TEXT(""));
					}
					// Arancione quando c'e' fuoco amico: stesso codice colore del nome marcato sopra la testa,
					// cosi' le due informazioni si riconoscono come la stessa cosa detta in due posti.
					DrawText(Zona, NumAlly > 0 ? FLinearColor(1.f, 0.6f, 0.12f, 1.f) : FLinearColor(1.f, 0.35f, 0.3f, 1.f),
						X, Y - LineH - 6.f, nullptr, 1.f);
				}
			}
			for (int32 A = 0; A < Sel->NumAbilities(); ++A)
			{
				const URTActionData* Ability = Sel->GetAbility(A);
				if (!Ability)
				{
					continue;
				}
				const bool bActive = (A == Sel->SelectedAbilityIndex);
				const bool bUsable = Sel->CanUseAbility(A);
				const int32 CD = Sel->GetAbilityCooldown(A);

				FString Line = FString::Printf(TEXT("%d. %s"), A + 1, *Ability->DisplayName.ToString());
				if (CD > 0) { Line += FString::Printf(TEXT("  (ricarica %d)"), CD); }
				else if (Ability->EnergyCost > 0 && Sel->Energy < Ability->EnergyCost) { Line += TEXT("  (energia)"); }
				if (bActive) { Line = TEXT("> ") + Line; }

				const FLinearColor Color = bActive ? FLinearColor::White
					: (bUsable ? FLinearColor(0.8f, 0.8f, 0.8f, 1.f) : FLinearColor(0.45f, 0.45f, 0.45f, 1.f));
				DrawText(Line, Color, X, Y, nullptr, 1.f);
				Y += LineH;
			}

			// La terna movimento / principale / reazione (CP 11.1). La barra qui sopra dice quali abilita'
			// HO; questa dice quali dei tre slot del turno ho gia' speso, e da cosa.
			//
			// Le tre righe si disegnano SEMPRE, anche a piano vuoto: la riga d'intento sopra le teste salta
			// le unita' senza ordini, quindi uno slot libero non si vedeva da nessuna parte — ed e' meta'
			// della domanda che il giocatore si pone in pianificazione.
			//
			// In basso a DESTRA perche' e' l'unica zona libera: il combat log tiene il basso a sinistra, le
			// abilita' il centro. L'ingombro non e' un dettaglio estetico, e' meta' del giudizio di
			// `PIE-V01-HUD`.
			const TArray<FRTSlotLine> SlotLines = ComposeSlotLines(URTHudViewModel::BuildUnitSlots(Sel));
			float SlotY = Canvas->SizeY - 24.f - LineH * (SlotLines.Num() - 1);
			for (const FRTSlotLine& SlotLine : SlotLines)
			{
				float SW = 0.f, SH = 0.f;
				GetTextSize(SlotLine.Text, SW, SH, nullptr, 1.f);

				// Grigio per lo slot libero, bianco per quello speso: la stessa scala che la barra abilita'
				// usa gia' per «c'e' ma non e' attivo», cosi' le due letture non chiedono due convenzioni.
				DrawText(SlotLine.Text,
					SlotLine.bOccupied ? FLinearColor::White : FLinearColor(0.55f, 0.55f, 0.55f, 1.f),
					Canvas->SizeX - SW - 24.f, SlotY, nullptr, 1.f);
				SlotY += LineH;
			}
		}
	}

	// Esito, VIA che l'ha determinato e istruzione di riavvio a partita conclusa (CP 10.3). "Vince il team 0"
	// da solo non distingue un'eliminazione da un punto di vantaggio allo scadere dei round.
	if (TurnManager && TurnManager->GetPhase() == ERTMatchPhase::MatchEnded)
	{
		const FRTMatchResult Result = TurnManager->GetMatchResult();
		const FString Headline = FString::Printf(TEXT("%s - %s"),
			*URTTurnRules::DescribeOutcome(Result.Outcome),
			*URTTurnRules::DescribeEndReason(Result.Reason));

		float TW = 0.f, TH = 0.f;
		GetTextSize(Headline, TW, TH, nullptr, 2.f);
		DrawText(Headline, FLinearColor::White, (Canvas->SizeX - TW) * 0.5f, Canvas->SizeY * 0.4f, nullptr, 2.f);

		const FString Restart = TEXT("premi R per rigiocare");
		float RW = 0.f, RH = 0.f;
		GetTextSize(Restart, RW, RH, nullptr, 1.2f);
		DrawText(Restart, FLinearColor(0.85f, 0.85f, 0.85f, 1.f),
			(Canvas->SizeX - RW) * 0.5f, Canvas->SizeY * 0.4f + TH + 8.f, nullptr, 1.2f);
	}
}
