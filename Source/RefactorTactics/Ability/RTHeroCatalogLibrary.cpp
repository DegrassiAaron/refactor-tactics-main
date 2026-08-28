#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Map/RTHexCellData.h" // ERTHexSurface: `Phase.MistVeil` nomina la superficie che crea

namespace
{
	/**
	 * Nome mostrabile di un'azione d'eroe, dal suo `ActionId` (issue **#892**).
	 *
	 * Perche' una tabella e non un parametro dei costruttori: le tre vie che creano un'azione
	 * (`MakeHeroAction`, `MakeHeroBasicAttack`, `MakeHeroReactionFromCoreAction`) convergono tutte sulla
	 * prima, quindi un solo punto basta a coprirle. Un parametro obbligatorio impedirebbe il difetto *alla
	 * radice* — e' la scelta piu' forte — ma tocca venti chiamate e tre firme: si valuta a parte, con il test
	 * gia' in piedi a fare da rete.
	 *
	 * ⚠️ Un `ActionId` **non presente** qui restituisce testo vuoto, e
	 * `RefactorTactics.Heroes.EveryActionHasADisplayName` diventa rosso. E' voluto: e' il modo in cui
	 * un'azione aggiunta domani senza nome si fa notare subito invece di comparire in partita come
	 * `[RT] Piano: X usa  su Y`.
	 *
	 * Le chiavi sono `Hero.<Nome>.<Abilita>` (**#754**); i valori sono player-facing e seguono il roster
	 * canonico di **D-120**, quindi nessuno dei venti nomina un eroe. Le due meta' della riga rispondono a
	 * due domande diverse: *come si chiama l'azione nel codice* e *come si legge a schermo*.
	 *
	 * ⚠️ **Questo capoverso ha detto il falso in tre modi diversi, e vale la pena dire quali.** Diceva:
	 * *«le chiavi restano i nomi legacy perche' sono `ActionId` serializzati e si redirigono, non si
	 * rinominano (D-130)»*.
	 *   1. La **regola** era gia' morta: **D-134** ha rimosso il redirect degli ID ritirati — lo dichiara
	 *      `RTCatalogLibrary.h`, «un ID che il catalogo non conosce non risolve». Senza redirect,
	 *      «si redirigono» non descriveva niente.
	 *   2. Il **rename di #753** l'ha reso autocontraddittorio: ha sostituito i nomi dentro l'elenco che
	 *      li dichiarava legacy, cosi' la frase diceva «i nomi legacy sono `Gadget.`, `Phase.` …», che sono
	 *      i nomi NUOVI. Una sostituzione di testo puo' invertire il senso di una frase senza toccarne la
	 *      forma, e nessun gate lo vede perche' il file compila.
	 *   3. E la **premessa** e' caduta comunque: #754 ha rinominato le chiavi.
	 * Un commento che spiega una scelta va riletto quando la scelta cambia, o resta a difendere il
	 * contrario di cio' che il codice fa.
	 *
	 * I quattro con variante ereditano il lessico che le varianti gia' usano — *Scarica concentrata* e
	 * *ramificata* stanno sotto *Scarica lineare*, *Marea curativa* e *d'urto* sotto *Marea circolare* —
	 * cosi' il nome base e le sue varianti si leggono come una famiglia.
	 */
	FText HeroActionDisplayName(const FName& Id)
	{
		static const TMap<FName, FString> Names = {
			// Gadget (`Hero.Gadget`)
			{ TEXT("Hero.Gadget.ArcPulse"),           TEXT("Impulso ad arco") },
			{ TEXT("Hero.Gadget.LinearDischarge"),    TEXT("Scarica lineare") },
			{ TEXT("Hero.Gadget.ConductiveNode"),     TEXT("Nodo conduttivo") },
			{ TEXT("Hero.Gadget.Overload"),           TEXT("Sovraccarico") },
			{ TEXT("Hero.Gadget.ReactiveCapacitor"),  TEXT("Condensatore reattivo") },

			// Phase (`Hero.Phase`)
			{ TEXT("Hero.Phase.PressureJet"),        TEXT("Getto in pressione") },
			{ TEXT("Hero.Phase.CircularTide"),       TEXT("Marea circolare") },
			{ TEXT("Hero.Phase.FluidTrail"),         TEXT("Scia fluida") },
			{ TEXT("Hero.Phase.MistVeil"),           TEXT("Velo di nebbia") },
			{ TEXT("Hero.Phase.FlowReaction"),       TEXT("Reazione di flusso") },
			{ TEXT("Hero.Phase.TideGuard"),          TEXT("Guardia di marea") },

			// Riktor (`Hero.Riktor`)
			{ TEXT("Hero.Riktor.ImpactShot"),      TEXT("Colpo d'impatto") },
			{ TEXT("Hero.Riktor.KineticPanel"),    TEXT("Pannello cinetico") },
			{ TEXT("Hero.Riktor.Reconfigure"),     TEXT("Riconfigurazione") },
			{ TEXT("Hero.Riktor.Ram"),             TEXT("Carica d'ariete") },
			{ TEXT("Hero.Riktor.Interposition"),   TEXT("Interposizione") },

			// Wraith (`Hero.Wraith`)
			{ TEXT("Hero.Wraith.PulseShot"),        TEXT("Colpo a impulsi") },
			{ TEXT("Hero.Wraith.InterceptShot"),    TEXT("Intercetto") },
			{ TEXT("Hero.Wraith.PassingBlade"),     TEXT("Lama di passaggio") },
			{ TEXT("Hero.Wraith.Deflection"),       TEXT("Deviazione") },
			{ TEXT("Hero.Wraith.Feint"),            TEXT("Finta") },
			{ TEXT("Hero.Wraith.PhaseGuard"),       TEXT("Guardia di fase") },
		};

		if (const FString* Found = Names.Find(Id))
		{
			return FText::FromString(*Found);
		}
		return FText::GetEmpty();
	}

	/**
	 * Un'azione d'eroe completa: popola SIA `Def` (modello E4, letto dai validator e dai test) SIA i campi
	 * legacy specchiati (`RangeCells`/`Power`/`Shape`/`AreaRadius`/`CooldownTurns`) che `ARTTurnManager` legge
	 * ancora oggi in partita (vedi il ponte a `RTTurnManager.cpp` riga ~793: si attiva SOLO se `Def.ActionId`
	 * e' vuoto, quindi per un'azione catalogata come questa il campo legacy dev'essere gia' coerente da solo).
	 * Stessa disciplina di `URTCatalogLibrary::ShippedAction`, estesa ai campi che l'oggetto porta in piu'.
	 */
	URTActionData* MakeHeroAction(const FName& Id, ERTResolutionPhase Phase, int32 Priority, int32 Range,
		int32 Cooldown, ERTActionFallback Fallback, const TArray<FRTActionEffectSpec>& Effects,
		ERTAbilityShape Shape = ERTAbilityShape::Single, int32 AreaRadius = 0,
		ERTActionSlot Slot = ERTActionSlot::Main, bool bInterruptible = true)
	{
		URTActionData* Action = NewObject<URTActionData>();

		Action->Def.ActionId = Id;
		Action->Def.ResolutionPhase = Phase;
		Action->Def.Priority = Priority;
		Action->Def.RangeCells = Range;
		Action->Def.CooldownTurns = Cooldown;
		Action->Def.Fallback = Fallback;
		Action->Def.Effects = Effects;
		Action->Def.Slot = Slot;
		Action->Def.bCanBeInterrupted = bInterruptible;

		Action->RangeCells = Range;
		Action->CooldownTurns = Cooldown;
		Action->Shape = Shape;
		Action->AreaRadius = AreaRadius;
		Action->Power = 0;
		for (const FRTActionEffectSpec& Spec : Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage) { Action->Power = Spec.Amount; break; }
		}

		Action->DisplayName = HeroActionDisplayName(Id);

		// [`INT-8`]: un'abilita' d'eroe che dichiara DANNO e' un'aggressione. Il campo resta comunque
		// dichiarato e sovrascrivibile dal chiamante -- serve a chi avra' l'equivalente d'eroe di
		// `Action.MarkTarget`, ostile e da zero danni, che questa riga da sola non riconoscerebbe.
		for (const FRTActionEffectSpec& Spec : Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage) { Action->Def.bCountsAsAttack = true; break; }
		}

		return Action;
	}

	/**
	 * L'attacco base di un eroe: `MakeHeroAction` piu' la dichiarazione di CHE COSA e' un profilo.
	 *
	 * Esiste per un motivo solo, ed e' che la relazione stia in UN posto: senza, ogni eroe nuovo dovrebbe
	 * ricordarsi di scrivere `BaseActionId` a mano, e chi se lo dimentica non rompe niente — l'azione
	 * funziona, e solo il TurnLog smette di poter dire che quello e' un attacco base (D-033).
	 * `RefactorTactics.Heroes.BasicAttackDeclaresItsBaseAction` lo fa valere sul roster.
	 */
	/** Aggiunge al kit solo se l'abilita' esiste: entrambe le fabbriche `...FromCore` sono fail-closed e
	 *  restituiscono `nullptr` quando l'azione core manca o non e' del tipo giusto.
	 *
	 *  ⚠️ Sta qui, e non in un `if` a ogni chiamata, perche' l'invariante e' UNO: **nel kit non entra un
	 *  `nullptr`**. Ripeterlo a ogni call site lo rende una cosa da ricordarsi, ed e' gia' successo — tre
	 *  reazioni d'eroe aggiungevano il risultato senza guardarlo, e un elemento nullo in `Actions` fa
	 *  contare 5 a `ValidateHeroes` mentre ogni consumatore che dereferenzia crasha.
	 */
	void AddAbility(URTHeroData* Hero, URTActionData* Action)
	{
		if (Hero && Action)
		{
			Hero->Actions.Add(Action);
		}
	}

	URTActionData* MakeHeroBasicAttack(const FName& Id, ERTResolutionPhase Phase, int32 Priority, int32 Range,
		int32 Cooldown, ERTActionFallback Fallback, const TArray<FRTActionEffectSpec>& Effects,
		ERTAbilityShape Shape = ERTAbilityShape::Single)
	{
		URTActionData* Action = MakeHeroAction(Id, Phase, Priority, Range, Cooldown, Fallback, Effects, Shape);
		Action->Def.BaseActionId = TEXT("Action.BasicAttack");
		return Action;
	}

}

TArray<FString> URTHeroCatalogLibrary::ValidateHeroes(const TArray<const URTHeroData*>& Heroes)
{
	TArray<FString> Errors;
	TSet<FName> Seen;

	// Numero di azioni: 1 attacco base + 4 abilita' fondamentali (catalogo v0.1 §"Struttura di un eroe"),
	// piu' al massimo UNA generica del catalogo core portata nel kit.
	//
	// ⚠️ **Era `== 5` esatte, ed e' diventato un intervallo con un costo dichiarato**: il validatore non dice
	// piu' *«questo eroe e' completo»* ma *«e' nell'intervallo»*. Un eroe a cui mancasse una fondamentale e
	// che ne portasse una generica passerebbe. Si accetta perche' l'alternativa — alzare il minimo a 6 —
	// renderebbe **invalidi** Gadget e Riktor, che di azioni ne hanno cinque: sposterebbe il rosso invece di
	// toglierlo.
	//
	// ⛔ **Il tetto e' 6 e non "quante ne vuoi"**: oltre, il kit supera le posizioni che l'input raggiunge.
	// Con dieci tasti numerici e le generiche su tasti propri, sei azioni d'eroe sono il massimo premibile —
	// `PlayerInput.EveryKitEntryIsReachable` e' il gate che lo misura, e questo numero senza quel gate
	// tornerebbe a essere un'opinione.
	constexpr int32 MinActionCount = 5;
	constexpr int32 MaxActionCount = 6;

	for (int32 i = 0; i < Heroes.Num(); ++i)
	{
		const URTHeroData* Hero = Heroes[i];
		if (Hero == nullptr)
		{
			Errors.Add(FString::Printf(TEXT("eroe #%d: riferimento nullo"), i));
			continue;
		}

		const FString Where = Hero->HeroId.IsNone()
			? FString::Printf(TEXT("eroe #%d"), i)
			: Hero->HeroId.ToString();

		if (Hero->HeroId.IsNone())
		{
			Errors.Add(FString::Printf(TEXT("%s: HeroId mancante (l'ID e' la chiave stabile dell'asset)"), *Where));
		}
		else if (Seen.Contains(Hero->HeroId))
		{
			Errors.Add(FString::Printf(TEXT("%s: HeroId duplicato"), *Where));
		}
		else
		{
			Seen.Add(Hero->HeroId);
		}

		if (Hero->MaxHealth <= 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: salute non positiva (%d)"), *Where, Hero->MaxHealth));
		}
		if (Hero->MovePoints <= 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: movimento non positivo (%d)"), *Where, Hero->MovePoints));
		}
		if (Hero->VisionRange < 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: range visivo negativo (%d)"), *Where, Hero->VisionRange));
		}
		if (Hero->PushResistance < 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: resistenza push negativa (%d)"), *Where, Hero->PushResistance));
		}
		// Soglia d'udito sulla scala 0-10 di D-041, la stessa dell'intensita' del rumore. Fuori scala non e'
		// «un valore estremo»: sotto 0 l'eroe sentirebbe il silenzio, sopra 10 non sentirebbe un'esplosione —
		// due modi di scollegare la statistica dal canale che la consuma.
		if (Hero->HearingThreshold < 0 || Hero->HearingThreshold > 10)
		{
			Errors.Add(FString::Printf(TEXT("%s: soglia d'udito fuori scala 0-10 (%d)"), *Where, Hero->HearingThreshold));
		}
		if (Hero->Affinity.IsNone())
		{
			Errors.Add(FString::Printf(TEXT("%s: affinita' non dichiarata"), *Where));
		}
		// La debolezza non si inventa (catalogo v0.1 §5): un eroe senza debolezza dichiarata resta
		// strutturalmente incompleto, e il validator lo dice prima che arrivi in partita.
		if (Hero->Weakness.IsNone())
		{
			Errors.Add(FString::Printf(TEXT("%s: debolezza non dichiarata"), *Where));
		}

		if (Hero->Actions.Num() < MinActionCount || Hero->Actions.Num() > MaxActionCount)
		{
			Errors.Add(FString::Printf(
				TEXT("%s: attese da %d a %d azioni (attacco base + quattro fondamentali, piu' al piu' una "
				     "generica), trovate %d"),
				*Where, MinActionCount, MaxActionCount, Hero->Actions.Num()));
			continue; // struttura gia' rotta: contare le varianti su un array della forma sbagliata non aiuta
		}

		// Indice 0 = attacco base, escluso dal conteggio delle varianti: la variante e' di un'abilita'
		// FONDAMENTALE (catalogo v0.1 §6), mai dell'attacco base (quello lo modificano le varianti d'arma,
		// equipaggiamento E7).
		int32 VariantCount = 0;
		for (int32 a = 1; a < Hero->Actions.Num(); ++a)
		{
			if (Hero->Actions[a] && Hero->Actions[a]->Variants.Num() > 0)
			{
				++VariantCount;
			}
		}
		if (VariantCount != 1)
		{
			Errors.Add(FString::Printf(
				TEXT("%s: %d abilita' fondamentali con variante (attesa esattamente 1)"), *Where, VariantCount));
		}
	}

	return Errors;
}

URTHeroData* URTHeroCatalogLibrary::MakeGadget()
{
	URTHeroData* Gadget = NewObject<URTHeroData>();
	Gadget->HeroId = TEXT("Hero.Gadget");
	// ⚠️ La variabile dice `Gadget` e il nome dice `Gadget`, e NON e' un refuso: D-120 separa i due piani.
	// `Hero.Gadget` e' lo Stable ID — chiave di codice, scenari e replay, che non si rinomina finche' #716 non
	// scioglie la collisione di namespace. `Gadget` e' il nome canonico/player-facing del personaggio.
	// Da qui il nome raggiunge l'unita' (`ConfigureFromHeroData`) e poi la HUD; il gate del confine e'
	// `RefactorTactics.Unit.HeroDataCrossesTheBoundary`.
	Gadget->DisplayName = FText::FromString(TEXT("Gadget"));
	Gadget->MaxHealth = 90;
	Gadget->MovePoints = 5;
	// 6 -> 7 (#131, [D-073]). E' la seconda meta' del lavoro cominciato con Wraith 100->90: quel calo aveva
	// tolto la dominanza su Phase e lasciato quella su **Gadget**, dove a parita' di salute e vista Wraith
	// restava avanti di un punto movimento.
	//
	// La leva e' la VISTA e non il movimento, per due ragioni misurate. Dare 6 MP a Gadget (o toglierne uno a
	// Wraith) renderebbe i due profili IDENTICI sulle quattro statistiche base, e `RosterIsBalanced` verifica
	// che nessuna coppia li condivida: si sarebbe rotto un test per ripararne un altro. E una resistenza alla
	// spinta NEGATIVA per Wraith sarebbe stata un numero senza effetto — `PushResistance` e' una SOGLIA, e le
	// spinte del catalogo valgono almeno 1, quindi -1 e 0 si comportano allo stesso modo.
	//
	// Con 7, Gadget diventa l'unico che vede oltre l'esagono di raggio 6: identita' vera, non compensazione.
	Gadget->VisionRange = 7;
	Gadget->HearingThreshold = 5;  // D-041: vede piu' lontano di tutti (7), quindi sente meno. L'udito COMPENSA la vista.
	Gadget->PushResistance = 0;
	Gadget->Affinity = TEXT("Affinity.Electricity");
	// Debolezza acqua: stesso identificatore che Phase (CP 6.3) usera' come sua affinita', cosi' la combo
	// "Gadget su bersaglio Wet" e "l'affinita' di Phase e' l'acqua" restano lo stesso concetto, non due nomi.
	Gadget->Weakness = TEXT("Affinity.Water");
	// E14.7 [D-047]: il profilo che il `Brace` arma. Il token NON porta il prefisso d'eroe — `Profile.Grounding`
	// e non `Gadget.Grounding` — cosi' il profilo si riassegna quando il roster cresce, senza rename.
	Gadget->ReactionProfileId = TEXT("Profile.Grounding");

	// Indice 0 — ArcPulse, attacco base. 22 danni / range 4 e' ESATTAMENTE la fascia "medio raggio" del
	// catalogo azioni v0.1 §1: non e' una coincidenza da verificare a mano, e' la stessa tabella.
	const FRTActionDef ArcPulseDef = URTCatalogLibrary::MakeBasicAttack(4);
	Gadget->Actions.Add(MakeHeroBasicAttack(TEXT("Hero.Gadget.ArcPulse"), ArcPulseDef.ResolutionPhase, ArcPulseDef.Priority,
		ArcPulseDef.RangeCells, ArcPulseDef.CooldownTurns, ArcPulseDef.Fallback, ArcPulseDef.Effects));

	// Indice 1 — LinearDischarge. 24 danni in linea, range 5 (stessa portata di `Action.LineAttack`: nessuna
	// azione lineare del catalogo ne dichiara una diversa). Il bonus "+8 su bersaglio Wet" NON e' nella lista
	// Effects: e' condizionale al bersaglio, non un danno fisso, e passa da `EffectiveAttackPower` +
	// `URTCombatLibrary::GadgetWetDischargeBonus` (vedi `Heroes.Gadget.WetBonus`) — un chiamante che applica
	// SOLO Effects vede 24, corretto finche' non controlla anche lo status del bersaglio.
	Gadget->Actions.Add(MakeHeroAction(TEXT("Hero.Gadget.LinearDischarge"), ERTResolutionPhase::Attack, /*Priority*/ 55,
		/*Range*/ 5, /*Cooldown*/ 2, ERTActionFallback::AttackCell,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 24) }, ERTAbilityShape::Line));

	// Indice 2 — ConductiveNode. Cablata su `Action.Electrify` da D-046 (issue #282): numeri dal CORE, non
	// nuovi — portata 4, propagazione 3, fase Environment. Il cooldown resta quello dell'eroe (2), come per
	// `Riktor.Ram`.
	//
	// Fino al 2026-08-10 qui c'era scritto che «nessun modello di conduttivita' di cella esiste» e che
	// `Effects` vuoto era la dichiarazione onesta. Era vero quando fu scritto e ha smesso di esserlo con E8
	// (chiusa il 2026-08-07): il terreno dinamico esiste, `ARTTurnManager::ApplyDynamicSurface` cambia la
	// superficie di una cella per N turni, ed `ERTHexSurface::Conductive` e' gia' dichiarata
	// `bConductsElectricity` nel catalogo terreni. Anche `Range 0` non e' piu' un segnaposto: la portata
	// arriva dal core insieme all'effetto, come il commento vecchio prometteva.
	//
	// D-064 (2026-08-10) chiude la domanda che restava: il PDF descriveva l'azione come «rende conduttiva una
	// cella per 2 turni», e quel PDF e' OBSOLETO. Le due letture non erano una completa e una parziale — erano
	// due azioni diverse, e l'autore ha scelto questa: la scarica LEGGE il grafo conduttivo, non lo crea.
	//
	// Il corollario che vale per chi passera' di qui: nessun campo di durata per-azione entra nel catalogo per
	// rappresentare l'altra lettura, e `Range 0` non e' piu' un segnaposto da sostituire.
	const FRTActionDef ElectrifyDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Electrify"));
	URTActionData* ConductiveNode = MakeHeroActionFromCore(TEXT("Hero.Gadget.ConductiveNode"),
		TEXT("Action.Electrify"), /*Cooldown*/ 2);
	// Il `nullptr` va guardato, altrimenti il fail-closed dell'helper diventa un crash sul percorso del
	// roster: `GetHeroRoster()` gira all'avvio e in decine di test. Un eroe con un'abilita' in meno lo
	// prendono `ValidateHeroes` e i test di struttura; un dereferenziamento nullo no.
	if (ConductiveNode)
	{
		// La propagazione e' IL comportamento, non un dettaglio: senza questa riga l'azione elettrificherebbe
		// una cella sola e CP 8.3 resterebbe non innescabile pur avendo un owner.
		ConductiveNode->Def.PropagationLimit = ElectrifyDef.PropagationLimit;
		Gadget->Actions.Add(ConductiveNode);
	}

	// Indice 3 — Overload. AoE 18 danni, raggio 1 (riuso il raggio di `Action.CircularAoE`, non un numero
	// nuovo), portata 3. "Interrupt sui dispositivi" non e' rappresentabile: non esistono dispositivi/gadget
	// (E7). Solo il danno e' un effetto dichiarato.
	Gadget->Actions.Add(MakeHeroAction(TEXT("Hero.Gadget.Overload"), ERTResolutionPhase::Attack, /*Priority*/ 65,
		/*Range*/ 3, /*Cooldown*/ 3, ERTActionFallback::AttackCell,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 18) }, ERTAbilityShape::Area, /*AreaRadius*/ 1));
	// La variante (vincolo v0.1: una sola abilita' fondamentale per eroe) sta su LinearDischarge, non qui:
	// vedi sotto.

	// Indice 4 — ReactiveCapacitor (CP 6.7). REAZIONE cablata sulla semantica di `Action.Counter`: stesso
	// trigger («sono stato colpito da un attacco diretto») e stesso modo di raggiungere chi ha colpito. Gli
	// EFFETTI sono di Gadget e sono DUE — scudo 15 a se' **e** 10 danni all'attaccante: e' la reazione per cui
	// CP 5.5 ha reso il motore componibile, e prima di allora ne sarebbe arrivata solo meta'.
	// Cooldown 3, dal catalogo eroi: piu' lungo dei 2 di `Action.Counter`, perche' fa anche da scudo.
	AddAbility(Gadget, MakeHeroReactionFromCoreAction(TEXT("Hero.Gadget.ReactiveCapacitor"), TEXT("Action.Counter"),
		/*Cooldown*/ 3,
		{ FRTActionEffectSpec(ERTActionEffect::Shield, 15),
		  FRTActionEffectSpec(ERTActionEffect::Damage, 10) }));

	// Variante di LinearDischarge (catalogo v0.1 §6, vincolo: una sola abilita' fondamentale con variante).
	// Concentrata: +6 danni (30 totali), un solo bersaglio — "non si propaga" e' vero per costruzione, dato
	// che LinearDischarge base non propaga (nessun sistema di propagazione elettrica esiste, E8).
	FRTAbilityVariant Concentrated;
	Concentrated.VariantId = TEXT("Hero.Gadget.LinearDischarge.Concentrated");
	Concentrated.DisplayName = FText::FromString(TEXT("Scarica concentrata"));
	Concentrated.Tradeoff = FText::FromString(TEXT("+6 danni (30 totali), ma non si propaga a un secondo bersaglio"));
	Concentrated.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 30));

	// Ramificata: -6 danni per bersaglio (18 ciascuno) MA un bersaglio aggiuntivo. Due eventi di danno
	// separati rappresentano i due colpi: QUALE secondo nemico venga colpito e' targeting non ancora cablato
	// (`ProduceEvents` legge oggi un solo `TargetUnitId` per istanza) — il numero c'e', il "come" arriva
	// quando la geometria multi-bersaglio delle azioni lineari lo richiedera' davvero.
	FRTAbilityVariant Branched;
	Branched.VariantId = TEXT("Hero.Gadget.LinearDischarge.Branched");
	Branched.DisplayName = FText::FromString(TEXT("Scarica ramificata"));
	Branched.Tradeoff = FText::FromString(TEXT("un bersaglio aggiuntivo, ma -6 danni per bersaglio (18 ciascuno)"));
	Branched.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 18));
	Branched.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 18));

	Gadget->Actions[1]->Variants.Add(Concentrated);
	Gadget->Actions[1]->Variants.Add(Branched);

	return Gadget;
}

URTHeroData* URTHeroCatalogLibrary::MakePhase()
{
	URTHeroData* Phase = NewObject<URTHeroData>();
	Phase->HeroId = TEXT("Hero.Phase");
	Phase->DisplayName = FText::FromString(TEXT("Phase")); // D-120: nome canonico; `Hero.Phase` resta lo Stable ID
	Phase->MaxHealth = 95;
	Phase->MovePoints = 5;
	Phase->VisionRange = 5;
	Phase->HearingThreshold = 3;  // D-041: orecchio fine (soglia bassa) a compensare una vista corta.
	Phase->PushResistance = 0;
	Phase->Affinity = TEXT("Affinity.Water");
	// Simmetrica a Gadget (Affinity.Water e' gia' la sua debolezza): la rivalita' fra i due eroi legati dalla
	// combo Wet e' un solo identificatore condiviso in entrambe le direzioni, non due nomi da sincronizzare.
	Phase->Weakness = TEXT("Affinity.Electricity");
	Phase->ReactionProfileId = TEXT("Profile.Sidestep"); // E14.7 [D-047]

	// Indice 0 — PressureJet, attacco base. 16 danni non corrisponde a NESSUNA fascia di
	// `BasicAttackDamageForRange` (28/25/22/20): a differenza di `Gadget.ArcPulse`, l'attacco base di Phase e'
	// TEMATICO (linea, Wet, spinta), non generico per portata. Non si forza `MakeBasicAttack` su un numero
	// che non gli appartiene. Range 5: stessa portata di `Gadget.LinearDischarge` (stessa forma, stesso riuso).
	Phase->Actions.Add(MakeHeroBasicAttack(TEXT("Hero.Phase.PressureJet"), ERTResolutionPhase::Attack, /*Priority*/ 50,
		/*Range*/ 5, /*Cooldown*/ 0, ERTActionFallback::AttackCell,
		{
			FRTActionEffectSpec(ERTActionEffect::Damage, 16),
			FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Wet, /*Turni*/ 1),
			FRTActionEffectSpec(ERTActionEffect::Push, 1),
		}, ERTAbilityShape::Line));

	// Indice 1 — CircularTide. Cura 18 ad area, e **solo quello** (#1006).
	//
	// 🔵 Il `Wet` ai nemici e' USCITO dalla dichiarazione. Viene da #995: Phase e' **abilitata** a Water,
	// non padrona — grado `Access`, cioe' UNA sola capability elementale — e il catalogo ne dichiarava
	// tre. Resta `PressureJet`. Copertura: `RefactorTactics.Heroes.Phase.TideHealsWithoutWetting`, che
	// sostituisce `...TideHealsAlliesWetsEnemies` — il nome e' cambiato col contratto, perche' un test
	// che dice `WetsEnemies` e non verifica piu' nessun `Wet` resta verde e racconta un kit che non c'e'.
	//
	// ⚠️ Costo accettato con l'opzione C di #1006: questo `Wet` ad area era il preparatore della combo con
	// Gadget (`LinearDischarge` fa +8 su bersaglio `Wet`). La combo passa ora solo per la linea di
	// `PressureJet`, che copre meno bersagli.
	//
	// Il limite dichiarato di prima resta e non c'entra col cambio: `bFriendlyFire` decide SE colpire un
	// alleato, non CON QUALE effetto. Portata 4 e raggio 1: stessi numeri di `Gadget.Overload`.
	Phase->Actions.Add(MakeHeroAction(TEXT("Hero.Phase.CircularTide"), ERTResolutionPhase::Attack, /*Priority*/ 60,
		/*Range*/ 4, /*Cooldown*/ 2, ERTActionFallback::AttackCell,
		{
			FRTActionEffectSpec(ERTActionEffect::Heal, 18),
		}, ERTAbilityShape::Area, /*AreaRadius*/ 1));

	// Indice 2 — FluidTrail. `Dash 3` che crea acqua lungo il percorso: la mobilita' e' rappresentabile
	// (fase Dash, stile lineare — stessa famiglia di `Action.Dodge`), la creazione di terreno no (nessun
	// effetto di cella dinamica esiste: E8/E9). Nessun Effects dichiarato: il movimento non passa da li', e
	// l'acqua lasciata dietro non ha un modello da consumare.
	// Lo slot va dichiarato: `MakeHeroAction` mette `Main` di default, e per una mobilita' che non fa danno
	// sarebbe quello sbagliato (D-028). Chi lascia la scia si e' mosso, ma puo' ancora agire.
	// 🔵 **TORNA a essere uno scatto (#1006), e D-046 e' superata su questo punto.**
	//
	// D-046 l'aveva cablata su `Action.CreateWater` per dare all'acqua un owner nel roster, e il suo
	// argomento era la vetrina: *«senza un produttore la combo acqua+elettricita' di Conflux, vetrina
	// dichiarata della v0.1, era giocabile solo su mappe che l'acqua ce l'avevano gia'»*. Quell'argomento
	// non e' stato ignorato — e' stato **pagato**: l'owner dell'acqua e' ora l'EQUIPAGGIAMENTO, perche'
	// `Gadget.Sprinkler` porta gia' `Action.CreateWater` («acqua raggio 1, che e' esattamente cio' che
	// `Action.CreateWater` gia' fa», `RTCatalogLibrary.cpp`). Chi vuole la combo la equipaggia.
	//
	// La ragione del cambio viene da #995: Phase e' **abilitata** a Water, non padrona — grado `Access`,
	// una sola capability elementale. Questa era la terza, e l'unica che *generava* la superficie.
	// Per la grammatica di #995 un Generic Equipment e' `External Access` e non fa proficiency, quindi
	// montare lo Sprinkler non riporta Phase sopra `Access`.
	//
	// ⚠️ **Costo accettato, scritto qui perche' e' dove qualcuno lo cerchera'**: il roster perde l'unico
	// produttore INNATO di superficie acqua, quindi `Gadget.ConductiveNode` — che propaga sul grafo
	// conduttivo — dipende ora dalla mappa o dallo Sprinkler.
	//
	// ✅ E D-028 riacquista un soggetto: `RTHeroCatalogTests.cpp` registra che dopo D-046 la regola era
	// «vera, e oggi senza nessuno a cui applicarsi», perche' `Wraith.PassingBlade` e' FastMovement e fa
	// 20 danni. Questa e' di nuovo mobilita' senza danno.
	//
	// 🔴 Questa riga chiamava `PassingBlade` **«una carica»** per via del danno. Dopo [D-191] la
	// distinzione non ha piu' conseguenze sullo slot — carica e lama sono mobilita' entrambe e occupano il
	// movimento — ma resta vera come descrizione: una carica si FERMA addosso al bersaglio
	// (`LinearCharge`), la lama lo ATTRAVERSA (`LinearPass`). E' lo stesso equivoco che, dieci righe piu'
	// sotto, lasciava alla lama il `Main` di default.
	//
	// Numeri dal CORE: portata 3, fase FastMovement, stile lineare. Il cooldown resta 2 — e' dell'eroe, non
	// del core (`Action.Dodge` ha 1).
	// ⚠️ Lo SLOT va dichiarato: `MakeHeroAction` mette `Main` di default, e per una mobilita' sarebbe
	// quello sbagliato (D-028). Chi scatta si e' mosso, ma puo' ancora agire.
	//
	// 🔴 **Questa riga diceva «per una mobilita' che NON FA DANNO», e la clausola era il difetto**: ha
	// lasciato `Hero.Wraith.PassingBlade` — che fa danno — col `Main` di default, e con esso due azioni
	// principali in un turno. [D-191] toglie la clausola e fissa il criterio: conta se la mobilita' si
	// FERMA sul bersaglio (`LinearCharge` -> attacco, slot principale) o lo ATTRAVERSA (`LinearPass` ->
	// mobilita', slot movimento), non se fa danno.
	const FRTActionDef DodgeDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Dodge"));
	URTActionData* FluidTrail = MakeHeroActionFromCore(TEXT("Hero.Phase.FluidTrail"),
		TEXT("Action.Dodge"), /*Cooldown*/ 2);
	if (FluidTrail)
	{
		// Lo SLOT non si eredita dall'helper (vedi il suo docstring): qui e' `Movement` perche' la mobilita'
		// ATTRAVERSA il bersaglio invece di fermarcisi ([D-191]), ed e' una scelta d'eroe, non del core.
		FluidTrail->Def.Slot = ERTActionSlot::Movement;
		// Lo STILE va copiato a mano, come prima ci andava la superficie: `MakeHeroAction` prende identita',
		// fase, portata, ricarica, fallback ed effetti — non i campi di comportamento. Senza questa riga
		// l'azione risolve e non muove nessuno, cioe' fallisce nel modo piu' silenzioso possibile.
		FluidTrail->Def.MovementStyle = DodgeDef.MovementStyle;
		Phase->Actions.Add(FluidTrail);
	}

	// Indice 3 — MistVeil. Issue #353: dichiarava «crea fumo raggio 1» e non lo faceva. `Smoke` era l'unica
	// delle otto superfici che nessuna azione sapeva creare, e il meccanismo esisteva gia' — mancava il
	// collegamento, esattamente come per `MovementStyle` su `Riktor.Ram`.
	//
	// **Passa a `Environment`**, e non e' un dettaglio di implementazione: `ResolveEnvironment` — l'unico posto
	// che crea superfici — processa solo le azioni la cui fase mappa su `Cleanup`, e in `Preparation` MistVeil
	// non ci arrivava mai. Conseguenza di GIOCO dichiarata: il fumo si alza nel Cleanup, quindi copre il turno
	// SEGUENTE. Si prepara un attraversamento, non si rompe una linea nell'istante. Stesso movimento che D-046
	// ha fatto su `FluidTrail`, che ha adottato fase e portata del core.
	//
	// I numeri vengono dal CORE, non sono inventati qui: portata 4 e priorita' 60 sono quelle di entrambe le
	// azioni ambientali (`Ignite`, `CreateWater`). La priorita' e' un ordinamento INTERNO alla fase — si
	// confronta solo fra azioni della stessa — quindi il 35 scelto per il Prep non aveva piu' significato
	// dove l'azione e' andata a vivere. Il cooldown 3 resta: e' del catalogo eroi.
	URTActionData* MistVeil = MakeHeroActionFromCore(TEXT("Hero.Phase.MistVeil"),
		TEXT("Action.Ignite"), /*Cooldown*/ 3, ERTAbilityShape::Area, /*AreaRadius*/ 1);
	if (MistVeil)
	{
		// ⚠️ **Non serve azzerare effetti o fallback**, e la prima stesura di questo passaggio lo faceva
		// scrivendo che senza quelle righe MistVeil avrebbe ereditato «il danno da fuoco» di `Ignite`.
		// E' falso: `Action.Ignite` e' dichiarata `ShippedAction(..., Cancel, {})` — nessun effetto, e il
		// fallback e' gia' `Cancel`. Le due righe erano no-op con una motivazione inventata, e sono uscite.
		// Chi domani desse a `Ignite` un effetto di danno deve invece guardare `URTActionData::Power`, che
		// `MakeHeroAction` calcola dagli effetti: svuotare `Def.Effects` dopo non lo aggiorna.
		//
		// La superficie va copiata a mano, come per `FluidTrail`: l'helper prende identita', fase, portata,
		// ricarica, fallback ed effetti — non i campi di comportamento.
		MistVeil->Def.bCreatesSurface = true;
		MistVeil->Def.SurfaceCreated = ERTHexSurface::Smoke;
		MistVeil->Def.SurfaceRadius = 1; // «crea fumo raggio 1», catalogo eroi
		Phase->Actions.Add(MistVeil);
	}

	// Indice 4 — FlowReaction. `Reposition 1` dopo un attacco subito: e' una reazione che produce MOVIMENTO
	// dentro un boundary di risoluzione, quindi il suo aggancio e' **E14** (ADR-0004, finestre di reazione),
	// non E5 — il motore di E5 valuta i trigger sullo snapshot dei colpi e non sposta nessuno.
	// Il rinvio e' dichiarato come DATO: slot `None` e nessun trigger. Darle lo slot `Reaction` la farebbe
	// raccogliere dal pass delle reazioni, che la registrerebbe come attivata senza che accada nulla — un
	// esito falso nel TurnLog e' peggio di un'abilita' dichiaratamente incompleta.
	Phase->Actions.Add(MakeHeroAction(TEXT("Hero.Phase.FlowReaction"), ERTResolutionPhase::Preparation, /*Priority*/ 36,
		/*Range*/ 0, /*Cooldown*/ 3, ERTActionFallback::Cancel, {}, ERTAbilityShape::Single, /*AreaRadius*/ 0,
		ERTActionSlot::None, /*bInterruptible*/ false));

	// `Hero.Phase.TideGuard` — lo scudo PROATTIVO, derivato da `Action.Shield`: Preparation, 25 punti di
	// scudo temporaneo, cooldown 2. E' l'unico scudo del gioco che si sceglie PRIMA di sapere se sarai
	// colpito: `Gadget.ReactiveCapacitor` e `Reaction.ReactiveShield` rispondono a un colpo gia' partito.
	//
	// Uno per squadra — il gemello e' `Hero.Wraith.PhaseGuard` — perche' le formazioni sono fisse
	// (`ARTGameMode::Team0Heroes`/`Team1Heroes`). Non va a Gadget, che porta gia' `ReactiveCapacitor`:
	// sarebbe la terza fonte di scudo sullo stesso eroe, il difetto che [D-218] ha corretto altrove.
	//
	// ⚠️ **E' la SESTA azione di Phase, e prima non ci sarebbe stata**: fino a quando le generiche
	// occupavano la fila dei numeri, un kit da undici voci ne lasciava una impremibile.
	Phase->Actions.Add(MakeHeroActionFromCore(TEXT("Hero.Phase.TideGuard"), TEXT("Action.Shield"),
		/*Cooldown*/ 2));

	// Variante di CircularTide (vincolo v0.1: una sola abilita' fondamentale con variante per eroe).
	// Curativa: cura 24 (invece di 18), MA non applica Wet ai nemici — rinuncia al setup della combo con
	// Gadget per curare di piu'.
	FRTAbilityVariant Healing;
	Healing.VariantId = TEXT("Hero.Phase.CircularTide.Healing");
	Healing.DisplayName = FText::FromString(TEXT("Marea curativa"));
	Healing.Tradeoff = FText::FromString(TEXT("cura 24 invece di 18, ma non applica Wet ai nemici"));
	Healing.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Heal, 24));

	// Urto: cura 10 (meno della base) MA applica Push 1 ai nemici — meno supporto, piu' controllo.
	FRTAbilityVariant Impact;
	Impact.VariantId = TEXT("Hero.Phase.CircularTide.Impact");
	Impact.DisplayName = FText::FromString(TEXT("Marea d'urto"));
	Impact.Tradeoff = FText::FromString(TEXT("cura solo 10, ma applica Push 1 ai nemici"));
	Impact.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Heal, 10));
	Impact.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Push, 1));

	Phase->Actions[1]->Variants.Add(Healing);
	Phase->Actions[1]->Variants.Add(Impact);

	return Phase;
}

URTHeroData* URTHeroCatalogLibrary::MakeRiktor()
{
	URTHeroData* Riktor = NewObject<URTHeroData>();
	Riktor->HeroId = TEXT("Hero.Riktor");
	Riktor->DisplayName = FText::FromString(TEXT("Riktor")); // D-120: nome canonico; `Hero.Riktor` resta lo Stable ID
	Riktor->MaxHealth = 120;
	Riktor->MovePoints = 4;
	Riktor->VisionRange = 5;
	Riktor->HearingThreshold = 3;  // D-041: come Phase — il piu' lento del roster sente arrivare.
	// Era `1`, l'unico del roster, fino al 2026-08-10. Portato a `0` da D-075 (#402): siccome ogni spinta
	// del gioco vale 1 e `PushResistance` e' una SOGLIA (D-038), il valore `1` non comprava "stabilita'" ma
	// **immunita' totale a ogni spostamento, sempre, senza spendere un'azione** — e quella nessuno l'aveva
	// decisa: era una conseguenza del catalogo, non una scelta. Per Riktor svuotava meta' di `Guard` e
	// `Brace` (l'asse dello spostamento era gia' coperto dalla statistica) proprio sull'eroe su cui la
	// scelta difensiva dovrebbe pesare di piu'.
	// Cio' che il commento vecchio motivava — "compra HP e stabilita' con movimento e vista" — resta vero
	// sugli HP (120, il piu' alto) e sui 4 MP: e' il prezzo, e non dipendeva da questo campo.
	// ATTENZIONE: con questo `0` il roster v0.1 e' interamente a `0`, quindi `PushResistance` e' una
	// meccanica DORMIENTE — modello e resolver (`RTTurnManager.cpp`, ramo `ERTActionEffect::Push`) la
	// implementano, nessun contenuto la esercita. E' dichiarato, non dimenticato: si risveglia da sola il
	// giorno in cui una v0.2 introduce una spinta >= 2 (rinviata con l'uscita (B) di #400, D-074).
	Riktor->PushResistance = 0;
	Riktor->Affinity = TEXT("Affinity.Structures");
	// Simmetrica a Wraith (CP 6.5), come Gadget/Phase fra loro: il roster chiude in due coppie. Il piu' lento
	// del roster e' vulnerabile a chi il movimento lo fa di mestiere.
	Riktor->Weakness = TEXT("Affinity.Movement");
	// ⛔ **Riktor NON ha un profilo, e il campo resta `None` di proposito** ([D-047], §2.5). La proposta gli
	// assegnava `ANCHOR` — «annulla lo spostamento» — ma `Hold Ground` lo fa gia' e con la stessa ampiezza:
	// il ramo `Braced` del resolver non controlla `KnockDist`, a differenza di `Guarded`. Una seconda
	// risposta identica alla prima lascerebbe la cardinalita' a 1 senza aprire nulla, e sarebbe la terza
	// scrittura della stessa regola dopo `Reaction.Anchor` e `Gadget.Anchor`.
	// ∴ il roster tiene **un eroe senza finestra sul `Brace`**, ed e' la baseline con cui CP 14.6 confronta
	// gli altri tre quando misura il pacing. La riga non si aggiunge: l'assenza e' il contenuto.

	// Indice 0 — ImpactShot, attacco base. **8 danni / range 3** (ADR-0007). Come `Phase.PressureJet`, non e'
	// una fascia di `BasicAttackDamageForRange`: l'attacco base e' dell'eroe, non della tabella generica.
	//
	// Fino al 2026-08-09 erano 24, ed era un numero motivato — la fascia corto raggio da' 25, e il catalogo
	// ne toglieva uno «in cambio della stazza». Il problema non era il numero in se': era che rendeva
	// `ImpactShot` l'attacco base PIU' FORTE del roster mentre il ruolo dichiarato di Riktor e'
	// Utility/Emergency. 8 e' ancorato a `Phase.PressureJet` (16), che sta un gradino sopra: ne e' la meta'
	// esatta. Basso, ma non finto: deve restare corretto premerlo per finire un bersaglio a pochi HP.
	//
	// Lo `Slow` e' la utility della famiglia, e delle cinque candidate era l'unica sia esprimibile sia
	// coerente: `ERTStructureOp` non sa danneggiare una copertura, e uno `Status` si applica al BERSAGLIO,
	// quindi «genera Guard su di se'» non e' rappresentabile. Durata 1 basta perche' il modificatore e'
	// letto FRESCO a ogni snapshot (CP 4.7): applicato nel Blast si riflette gia' sulla fase Move dello
	// STESSO turno, poi scade nel Cleanup. E' la risposta di Riktor a chi si muove di mestiere — cioe' alla
	// sua stessa debolezza dichiarata, `Affinity.Movement`.
	Riktor->Actions.Add(MakeHeroBasicAttack(TEXT("Hero.Riktor.ImpactShot"), ERTResolutionPhase::Attack, /*Priority*/ 50,
		/*Range*/ 3, /*Cooldown*/ 0, ERTActionFallback::Cancel,
		{
			FRTActionEffectSpec(ERTActionEffect::Damage, 8),
			FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Slow, /*Turni*/ 1),
		}));

	// Indice 1 — KineticPanel (CP 9.5). E' `Action.CreateCover` con un nome d'eroe: stesso riuso che `Ram` fa
	// di `Action.Charge` e `Interposition` di `Action.Intercept`. Fase, priorita', portata, fallback e
	// soprattutto `StructureOp` vengono dal CORE — cablarli a mano qui significherebbe due cataloghi da tenere
	// allineati, e il primo a cambiare romperebbe il secondo in silenzio.
	//
	// La fase resta `Preparation`, ed e' il core ad essersi allineato a questa (D-a, 2026-08-09): si costruisce
	// prima che i colpi partano, altrimenti la copertura arriverebbe dopo aver incassato.
	//
	// **Portata 3, non piu' 1**: il numero e' quello che il catalogo azioni dichiara per `CreateCover`. Il
	// prezzo e' un dato in piu' nel piano — a portata 1 il bordo si deduceva dalla coppia di celle, a 3 no —
	// ed e' `PlannedCoverEdge`.
	{
		const FRTActionDef CoverDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.CreateCover"));
		URTActionData* Panel = MakeHeroActionFromCore(TEXT("Hero.Riktor.KineticPanel"),
			TEXT("Action.CreateCover"), /*Cooldown*/ 2);
		if (Panel)
		{
			Panel->Def.StructureOp = CoverDef.StructureOp; // erige: e' il dato che il resolver legge
			Riktor->Actions.Add(Panel);
		}
	}

	// Indice 2 — Reconfigure (CP 9.5). Sposta o ruota una copertura ESISTENTE. A differenza del pannello NON
	// eredita da un'azione core, e la ragione e' che oggi non ce n'e' una: spostare una copertura e' semantica
	// di questa sola identita' in tutta la v0.1. Il dato (`StructureOp`) e' comunque quello condiviso, quindi
	// il giorno in cui un gadget o uno scenario vorranno spostare, l'azione core si aggiunge senza toccare il
	// resolver.
	//
	// **Portata 3** come il pannello: ruotare solo cio' che si ha addosso renderebbe l'abilita' un ripiego, e
	// il catalogo non dichiara una portata diversa fra le due. Fase Preparation per lo stesso motivo del
	// pannello: il campo si sistema PRIMA che i colpi partano.
	{
		URTActionData* Reconfigure = MakeHeroAction(TEXT("Hero.Riktor.Reconfigure"), ERTResolutionPhase::Preparation,
			/*Priority*/ 76, /*Range*/ 3, /*Cooldown*/ 2, ERTActionFallback::Cancel, {});
		Reconfigure->Def.StructureOp = ERTStructureOp::MoveCover;
		Riktor->Actions.Add(Reconfigure);
	}

	// Indice 3 — Ram. E' una CARICA: 20 danni + Push 1, gli stessi numeri di `Action.Charge` del catalogo
	// azioni v0.1 §2 — non una coincidenza, e' la stessa azione con un nome d'eroe. Riuso identico di fase,
	// stile di movimento e portata: l'impatto risolve nel Blast (codice 20/30), come per ogni carica.
	const FRTActionDef ChargeDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"));
	// Lo SLOT si eredita dal core come tutto il resto ([D-191]): `Ram` **e'** `Action.Charge` con un nome
	// d'eroe, e scriverne uno a mano qui creerebbe la seconda verita' che il commento sopra nega. Senza
	// questo argomento prendeva il `Main` di default di `MakeHeroAction` — che era anche lo slot del core
	// fino al 2026-08-25, quindi il difetto non si vedeva: coincidevano per caso, non per costruzione.
	URTActionData* Ram = MakeHeroActionFromCore(TEXT("Hero.Riktor.Ram"), TEXT("Action.Charge"),
		/*Cooldown*/ 2);
	if (Ram)
	{
		Ram->Def.Slot = ChargeDef.Slot; // lo slot si eredita dal core come tutto il resto ([D-191])
		Ram->Def.MovementStyle = ChargeDef.MovementStyle; // LinearCharge: si ferma ADDOSSO al primo nemico
		Riktor->Actions.Add(Ram);
	}

	// Indice 4 — Interposition (CP 6.7). REAZIONE cablata sulla semantica di `Action.Intercept`: Riktor
	// DIVENTA il bersaglio di un colpo diretto a un alleato entro 2 celle. Nessun effetto proprio, ed e'
	// corretto: interporsi non aggiunge danno ne' stati, cambia CHI subisce un colpo altrui — cosa che
	// `FRTActionEffectSpec` non sa esprimere e che il pass delle reazioni fa gia' per l'azione core.
	// La portata (2) e la priorita' (la piu' bassa fra le reazioni, cosi' la redirezione precede le altre)
	// vengono dal core; il cooldown 3 dal catalogo eroi. L'eroe piu' resistente del roster e' quello che si
	// mette in mezzo: e' la sua identita', non un numero in piu'.
	AddAbility(Riktor, MakeHeroReactionFromCoreAction(TEXT("Hero.Riktor.Interposition"), TEXT("Action.Intercept"),
		/*Cooldown*/ 3));

	// Variante di KineticPanel (vincolo v0.1: una sola abilita' fondamentale con variante per eroe).
	// I due compromessi sono fatti di INTEGRITA' e DURATA, non di effetti, e vivono in `Parameters`.
	// **Da CP 9.5 qualcuno li legge**: `ResolveCoverStructures` prende integrita' e durata dalla variante
	// attiva dell'unita' (`ARTUnit::ActiveVariantId`) invece che dai valori base. Erano stati scritti qui
	// dichiarando che sarebbero serviti a E9: e' successo.
	FRTAbilityVariant Reinforced;
	Reinforced.VariantId = TEXT("Hero.Riktor.KineticPanel.Reinforced");
	Reinforced.DisplayName = FText::FromString(TEXT("Pannello rinforzato"));
	Reinforced.Tradeoff = FText::FromString(TEXT("integrita' 45 invece di 30, ma dura un solo turno"));
	Reinforced.Parameters.Add(TEXT("Integrity"), 45);
	Reinforced.Parameters.Add(TEXT("DurationTurns"), 1);
	Reinforced.Parameters.Add(TEXT("FreeRotations"), 0);

	FRTAbilityVariant Adaptive;
	Adaptive.VariantId = TEXT("Hero.Riktor.KineticPanel.Adaptive");
	Adaptive.DisplayName = FText::FromString(TEXT("Pannello adattivo"));
	Adaptive.Tradeoff = FText::FromString(TEXT("integrita' 25 invece di 30, ma una rotazione gratuita"));
	Adaptive.Parameters.Add(TEXT("Integrity"), 25);
	// Durata 0 = "non scade da sola" (il pannello base del catalogo terreni non dichiara una durata): il
	// rinforzato la compra con la fragilita' del tempo, l'adattivo con quella dell'integrita'.
	Adaptive.Parameters.Add(TEXT("DurationTurns"), 0);
	Adaptive.Parameters.Add(TEXT("FreeRotations"), 1);

	// ⚠️ Per **ActionId**, non per indice. `Actions[1]` era il pannello finche' ogni `Add` era
	// incondizionato; da quando `KineticPanel` puo' non entrare — l'helper e' fail-closed e restituisce
	// `nullptr` su un'azione core assente — l'indice 1 diventerebbe `Reconfigure`, e le due varianti si
	// attaccherebbero all'abilita' sbagliata senza che niente lo dica: `ValidateHeroes` conta le azioni,
	// non guarda a chi appartengono le varianti.
	if (TObjectPtr<URTActionData>* Panel = Riktor->Actions.FindByPredicate(
			[](const TObjectPtr<URTActionData>& A)
			{ return A && A->Def.ActionId == FName(TEXT("Hero.Riktor.KineticPanel")); }))
	{
		(*Panel)->Variants.Add(Reinforced);
		(*Panel)->Variants.Add(Adaptive);
	}

	return Riktor;
}

URTHeroData* URTHeroCatalogLibrary::MakeWraith()
{
	URTHeroData* Wraith = NewObject<URTHeroData>();
	Wraith->HeroId = TEXT("Hero.Wraith");
	Wraith->DisplayName = FText::FromString(TEXT("Wraith")); // D-120: nome canonico; `Hero.Wraith` resta lo Stable ID
	// 100 -> 90 (#131). A 100 HP Wraith DOMINAVA Gadget (90/5/6/0) e Phase (95/5/5/0) sulle quattro statistiche
	// base: migliore o pari ovunque, strettamente migliore in salute e movimento. Il catalogo §5 scriveva
	// «Wraith compra mobilita' con l'assenza di difese», che sui numeri era falso — non pagava nulla.
	//
	// ⚠️ 90 toglie la dominanza su **Phase** (-5 HP), NON quella su Gadget: a parita' di HP e vista resta +1 MP.
	// Non e' una svista, e' il perimetro della decisione presa: il costo di Wraith diventa visibile, ma il
	// confronto Gadget/Wraith resta da chiudere e vive in `#131`, che non si chiude qui.
	Wraith->MaxHealth = 90;
	Wraith->MovePoints = 6; // il piu' mobile del roster: e' cio' che compra con l'assenza di difese
	Wraith->VisionRange = 6;
	Wraith->HearingThreshold = 5;  // D-041: mobilita' e vista si pagano sull'udito.
	Wraith->PushResistance = 0;
	Wraith->Affinity = TEXT("Affinity.Movement");
	// Simmetrica a Riktor: chi si muove di mestiere e' neutralizzato da chi gli chiude le traiettorie.
	// Il roster chiude in due coppie — Gadget↔Phase sull'acqua, Riktor↔Wraith sullo spazio.
	Wraith->Weakness = TEXT("Affinity.Structures");
	// E14.7 [D-047]: `Profile.Glance` porta DUE risposte extra, quindi cardinalita' 3 — l'unico del roster.
	Wraith->ReactionProfileId = TEXT("Profile.Glance");

	// Indice 0 — PulseShot, attacco base. 21 danni / range 4: come per Phase e Riktor, non e' la fascia
	// generica (a range 4 darebbe 22). Un punto in meno del medio raggio, pagato in mobilita'.
	Wraith->Actions.Add(MakeHeroBasicAttack(TEXT("Hero.Wraith.PulseShot"), ERTResolutionPhase::Attack, /*Priority*/ 50,
		/*Range*/ 4, /*Cooldown*/ 0, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 21) }));

	// Indice 1 — InterceptShot. 16 danni + stop del movimento a chi ENTRA nella cella controllata.
	//
	// E' la thin slice della **Predictive Action** (E18, D-016). Fino al 2026-08-10 era dichiarata «rinviata a
	// E14» come DATO — slot `None`, nessun trigger — perche' il suo trigger e' d'ingresso su movimento e
	// cablarla sul motore delle reazioni di E5 avrebbe duplicato `FRTSuppressiveZone` (CP 4.6). Quel rinvio
	// e' caduto per la ragione opposta a quella che lo aveva prodotto: **non le serve una finestra**. La cella
	// si dichiara in Planning e si verifica a un boundary deterministico, quindi non c'e' niente da chiedere a
	// nessuno durante la Resolution — che e' esattamente cio' che una Fast Reaction fa e questa no.
	//
	// Lo slot torna `Main`: e' un'azione dichiarata in pianificazione come le altre, non una reazione tenuta
	// pronta. Portata 1 e' la cella ADIACENTE controllata; il cooldown 2 non cambia — chi scommette paga il
	// cooldown anche quando sbaglia, ed e' la meta' del costo che rende il whiff una scelta.
	URTActionData* InterceptShot = MakeHeroAction(TEXT("Hero.Wraith.InterceptShot"), ERTResolutionPhase::Preparation,
		/*Priority*/ 30, /*Range*/ 1, /*Cooldown*/ 2, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 16) }, ERTAbilityShape::Single, /*AreaRadius*/ 0,
		ERTActionSlot::Main, /*bInterruptible*/ false);
	// I due campi che la rendono predittiva stanno nei DATI e non in un ramo del resolver: `LockCell` dice che
	// la cella non si rivaluta, `MovementEntry` quando si guarda chi ci e' passato.
	InterceptShot->Def.PredictiveTargeting = ERTPredictiveTargeting::LockCell;
	InterceptShot->Def.PredictionBoundary = ERTPredictionBoundary::MovementEntry;
	Wraith->Actions.Add(InterceptShot);

	// Indice 2 — PassingBlade. `Dash 3` che colpisce per 20 le unita' ATTRAVERSATE: l'unica abilita'
	// non-base di Wraith interamente rappresentabile. Stile `LinearPass` e non `LinearCharge`: la carica si
	// FERMA sul primo nemico, questa gli passa attraverso — e' la differenza che `ERTMovementStyle` esiste
	// per rendere un dato invece che un `if` sull'ActionId (CP 4.5).
	//
	// ⚠️ Questa riga diceva `LinearDash` e contraddiceva quella dodici righe piu' sotto, che dal 2026-08-08
	// dichiara `LinearPass`: due commenti adiacenti sulla STESSA abilita' con due stili diversi. Corretta con
	// [D-191], che su quella distinzione fonda il criterio dello slot — un commento sbagliato li' non e' piu'
	// solo impreciso, manda il prossimo kit dalla parte sbagliata.
	// 🔴 Lo SLOT e' `Movement` e va DICHIARATO ([D-191]): una mobilita' rapida occupa il movimento, e il
	// danno non cambia che cosa ha speso. Vale per tutte e tre quelle del roster — questa, `Phase.FluidTrail`
	// e `Riktor.Ram`, carica compresa.
	//
	// ⚠️ Senza la dichiarazione prendeva il `Main` di default di `MakeHeroAction`, e la conseguenza era
	// misurabile: il bot pianificava lama + attacco base e il resolver eseguiva ENTRAMBE — due azioni
	// principali, 41 danni in un turno. Il difetto e' rimasto invisibile finche' nessuno ha composto il
	// piano e chiesto a `URTPlanValidationLibrary::ValidatePlan` se fosse legale.
	Wraith->Actions.Add(MakeHeroAction(TEXT("Hero.Wraith.PassingBlade"), ERTResolutionPhase::FastMovement,
		/*Priority*/ 30, /*Range*/ 3, /*Cooldown*/ 2, ERTActionFallback::Stop,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 20) },
		ERTAbilityShape::Single, /*AreaRadius*/ 0, ERTActionSlot::Movement));
	// `LinearPass` e non `LinearDash`: fino al 2026-08-08 questo commento diceva «passa attraverso» e il
	// codice si fermava sul primo nemico come uno scatto qualsiasi, quindi i 20 danni dichiarati sopra non
	// avevano un momento in cui applicarsi. Lo stile e' un DATO proprio per questo (CP 4.5): la differenza
	// fra fermarsi, fermarsi addosso, scavalcare e attraversare non e' un `if` sull'ActionId.
	Wraith->Actions[2]->Def.MovementStyle = ERTMovementStyle::LinearPass;

	// Indice 3 — Deflection (CP 6.7). REAZIONE cablata sulla semantica di `Action.Deflect`: -20 sul colpo
	// diretto che l'ha innescata. La riduzione arriva dagli effetti del core (`ERTActionEffect::DamageReduction`,
	// CP 5.5) e resta distinta dallo scudo: uno scudo ASSORBE e si consuma, questa toglie punti al colpo.
	// Stessa famiglia di `Action.Guard` (-15 al primo colpo) ma con un trigger invece di una stance.
	// Cooldown 2, uguale al core: il catalogo eroi non ne dichiara uno diverso.
	AddAbility(Wraith, MakeHeroReactionFromCoreAction(TEXT("Hero.Wraith.Deflection"), TEXT("Action.Deflect"),
		/*Cooldown*/ 2));

	// Indice 4 — Feint. Marca una CELLA e concede un `Reposition`: nessuna delle due meta' e' dichiarabile.
	// `Status` si applica alle UNITA', non alle celle; il movimento passa da `ERTMovementStyle`, non da
	// Effects. Fase Control (codice 30 del catalogo, macro-fase Blast): la finta precede il danno per
	// priorita', com'e' giusto per un'azione che sposta e disorienta.
	//
	// La durata della marcatura e' **1 turno**, decisa esplicitamente perche' il PDF non la dava. Non sta nei
	// `Parameters` perche' quelli sono della VARIANTE (dove il catalogo differenzia due configurazioni), e
	// qui non c'e' niente da differenziare: e' un numero dell'azione base, e finche' non esiste un effetto
	// che lo porti resta dichiarato nel catalogo eroi, non simulato in un campo che nessuno leggerebbe.
	Wraith->Actions.Add(MakeHeroAction(TEXT("Hero.Wraith.Feint"), ERTResolutionPhase::Control, /*Priority*/ 40,
		/*Range*/ 3, /*Cooldown*/ 2, ERTActionFallback::Cancel, {}));

	// `Hero.Wraith.PhaseGuard` — gemello di `Hero.Phase.TideGuard`, uno per squadra. Su Wraith costa una
	// scelta vera: la Preparation spesa qui e' quella che non arma `InterceptShot`.
	Wraith->Actions.Add(MakeHeroActionFromCore(TEXT("Hero.Wraith.PhaseGuard"), TEXT("Action.Shield"),
		/*Cooldown*/ 2));

	// Variante di InterceptShot (vincolo v0.1: una sola abilita' fondamentale con variante per eroe).
	// Preciso: 20 danni ma controlla UNA cella. Esteso: 14 danni ma una LINEA di 3 celle. Il danno e' un
	// effetto; il numero di celle controllate non lo e' (non esiste il concetto di "zona di controllo" fuori
	// da `FRTSuppressiveZone`, che nessuno collega ancora a un eroe): sta nei `Parameters`, come l'integrita'
	// del pannello di Riktor.
	FRTAbilityVariant Precise;
	Precise.VariantId = TEXT("Hero.Wraith.InterceptShot.Precise");
	Precise.DisplayName = FText::FromString(TEXT("Intercetto preciso"));
	Precise.Tradeoff = FText::FromString(TEXT("20 danni invece di 16, ma controlla una sola cella"));
	Precise.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 20));
	Precise.Parameters.Add(TEXT("ControlledCells"), 1);

	FRTAbilityVariant Extended;
	Extended.VariantId = TEXT("Hero.Wraith.InterceptShot.Extended");
	Extended.DisplayName = FText::FromString(TEXT("Intercetto esteso"));
	Extended.Tradeoff = FText::FromString(TEXT("controlla una linea di 3 celle, ma solo 14 danni"));
	Extended.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 14));
	Extended.Parameters.Add(TEXT("ControlledCells"), 3);

	Wraith->Actions[1]->Variants.Add(Precise);
	Wraith->Actions[1]->Variants.Add(Extended);

	return Wraith;
}

TArray<URTHeroData*> URTHeroCatalogLibrary::GetHeroRoster()
{
	return { MakeGadget(), MakePhase(), MakeRiktor(), MakeWraith() };
}

TArray<FName> URTHeroCatalogLibrary::GetHeroIds()
{
	// Stesso ordine di `GetHeroRoster()`: `Heroes.HeroIdsMatchRoster` confronta le due liste posizione per
	// posizione, quindi riordinare qui senza riordinare la' e' un rosso, non una svista che passa.
	return { TEXT("Hero.Gadget"), TEXT("Hero.Phase"), TEXT("Hero.Riktor"), TEXT("Hero.Wraith") };
}

URTActionData* URTHeroCatalogLibrary::MakeHeroActionFromCore(const FName& HeroActionId,
	const FName& CoreActionId, int32 Cooldown, ERTAbilityShape Shape, int32 AreaRadius)
{
	const FRTActionDef Core = URTCatalogLibrary::FindCoreAction(CoreActionId);
	// ⚠️ `IsNone()` PRIMA del confronto, e non e' ridondanza: con `CoreActionId` vuoto il confronto
	// `Core.ActionId != CoreActionId` mette a paragone `NAME_None` con `NAME_None`, e' falso, e la guardia
	// lascia passare esattamente l'abilita' coi default che il docstring promette di impedire. E' la forma
	// che `URTCatalogLibrary::MakeEquipmentAction` usa da sempre, e che qui mancava.
	if (Core.ActionId.IsNone() || Core.ActionId != CoreActionId)
	{
		return nullptr;
	}

	URTActionData* Action = MakeHeroAction(HeroActionId, Core.ResolutionPhase, Core.Priority,
		Core.RangeCells, Cooldown, Core.Fallback, Core.Effects, Shape, AreaRadius);

	Action->Def.DerivedFromActionId = CoreActionId;
	return Action;
}

URTActionData* URTHeroCatalogLibrary::MakeHeroReactionFromCoreAction(const FName& HeroActionId,
	const FName& CoreActionId, int32 CooldownTurns, const TArray<FRTActionEffectSpec>& Effects,
	int32 RangeCells)
{
	const FRTActionDef Core = URTCatalogLibrary::FindCoreAction(CoreActionId);
	// `IsNone()` per prima, come nel fratello: con `CoreActionId` vuoto il confronto paragona `NAME_None`
	// con se stesso ed e' falso. Qui la guardia reggeva **per caso** — il default di `FRTActionDef::Slot`
	// e' `Main`, quindi la seconda clausola salvava la prima — e un caso che regge per caso e' un caso che
	// smette di reggere il giorno in cui qualcuno cambia quel default.
	if (Core.ActionId.IsNone() || Core.ActionId != CoreActionId || Core.Slot != ERTActionSlot::Reaction)
	{
		// Fail-closed: un'azione core assente, o che reazione non e', darebbe un'abilita' che il pass delle
		// reazioni non guarda — inerte in partita e silenziosa nel TurnLog.
		return nullptr;
	}

	URTActionData* Action = MakeHeroAction(HeroActionId, Core.ResolutionPhase, Core.Priority,
		RangeCells >= 0 ? RangeCells : Core.RangeCells, CooldownTurns, Core.Fallback,
		Effects.Num() > 0 ? Effects : Core.Effects, ERTAbilityShape::Single, /*AreaRadius*/ 0,
		Core.Slot, Core.bCanBeInterrupted);

	// Il trigger viene dalla semantica core: e' la domanda a cui la reazione risponde («sono stato colpito?»,
	// «un alleato e' stato colpito?»), non un numero di bilanciamento dell'eroe.
	Action->Def.ReactionTrigger = Core.ReactionTrigger;

	// `CoreActionId` attraversava questa firma da sempre e non finiva da nessuna parte: la relazione
	// «questa reazione d'eroe e' `Action.Counter` con un nome proprio» viveva nel solo sorgente. Ora resta
	// nel `Def`, dove il gate della raggiungibilita' la legge.
	Action->Def.DerivedFromActionId = CoreActionId;
	return Action;
}
