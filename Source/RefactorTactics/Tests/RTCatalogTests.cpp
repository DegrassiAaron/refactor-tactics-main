#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTEquipmentData.h"
#include "Ability/RTActionData.h"
#include "Core/RTGameplayTags.h"
#include "Unit/RTUnit.h"
#include "Turn/RTTurnRules.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"
#include "Engine/DataAsset.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Perception/RTVeilTransition.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Definizione minima valida, da alterare nei singoli test. Nome distinto per file (unity build). */
	FRTActionDef MakeCatalogAction(const FName& Id, ERTResolutionPhase Phase, int32 Priority,
		ERTActionFallback Fallback = ERTActionFallback::Cancel)
	{
		FRTActionDef Def;
		Def.ActionId = Id;
		Def.ResolutionPhase = Phase;
		Def.Priority = Priority;
		Def.Fallback = Fallback;
		Def.RangeCells = 1;
		Def.CostMP = 0;
		Def.CooldownTurns = 0;
		Def.InterruptPolicy = ERTInterruptPolicy::InterruptBeforeEffect;
		return Def;
	}
}

// ---------------------------------------------------------------------------------------------------------
// Rimappatura delle fasi: il catalogo numera 0/10/20/30/40/50/60, il gioco risolve sulle macro-fasi di Atlas
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogPhaseMappingTest,
	"RefactorTactics.Catalog.PhaseMappingIsTotal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogPhaseMappingTest::RunTest(const FString&)
{
	// TOTALE: ogni codice del catalogo ha una macro-fase, nessuno cade in un default silenzioso.
	const ERTResolutionPhase All[] = {
		ERTResolutionPhase::Snapshot,
		ERTResolutionPhase::Preparation,
		ERTResolutionPhase::FastMovement,
		ERTResolutionPhase::NormalMovement,
		ERTResolutionPhase::Control,
		ERTResolutionPhase::Attack,
		ERTResolutionPhase::Environment,
		ERTResolutionPhase::Cleanup
	};
	for (const ERTResolutionPhase Phase : All)
	{
		const ERTMatchPhase Mapped = URTCatalogLibrary::MapResolutionPhase(Phase);
		TestTrue(TEXT("ogni codice mappa su una macro-fase reale (mai MatchEnded)"), Mapped != ERTMatchPhase::MatchEnded);
	}

	// La rimappatura che distingue questo progetto dal catalogo (ADR-0003 §3).
	TestEqual(TEXT("Preparazione -> Prep"),
		URTCatalogLibrary::MapResolutionPhase(ERTResolutionPhase::Preparation), ERTMatchPhase::Prep);
	TestEqual(TEXT("Movimento rapido -> Dash (prima del Blast)"),
		URTCatalogLibrary::MapResolutionPhase(ERTResolutionPhase::FastMovement), ERTMatchPhase::Dash);
	TestEqual(TEXT("Movimento normale -> Move (DOPO il Blast: qui il catalogo divergeva)"),
		URTCatalogLibrary::MapResolutionPhase(ERTResolutionPhase::NormalMovement), ERTMatchPhase::Move);
	TestEqual(TEXT("Controllo -> Blast (non e' una macro-fase separata)"),
		URTCatalogLibrary::MapResolutionPhase(ERTResolutionPhase::Control), ERTMatchPhase::Blast);
	TestEqual(TEXT("Attacco -> Blast"),
		URTCatalogLibrary::MapResolutionPhase(ERTResolutionPhase::Attack), ERTMatchPhase::Blast);
	TestEqual(TEXT("Ambiente -> Cleanup (dopo il Move: colpisce chi e' appena entrato)"),
		URTCatalogLibrary::MapResolutionPhase(ERTResolutionPhase::Environment), ERTMatchPhase::Cleanup);
	TestEqual(TEXT("Cleanup -> Cleanup"),
		URTCatalogLibrary::MapResolutionPhase(ERTResolutionPhase::Cleanup), ERTMatchPhase::Cleanup);
	TestEqual(TEXT("Snapshot -> Planning (congelamento a fine pianificazione)"),
		URTCatalogLibrary::MapResolutionPhase(ERTResolutionPhase::Snapshot), ERTMatchPhase::Planning);

	// I codici numerici del catalogo restano leggibili: sono la chiave di lettura dei due PDF.
	TestEqual(TEXT("il codice numerico e' conservato"),
		URTCatalogLibrary::ResolutionPhaseCode(ERTResolutionPhase::Attack), 40);
	TestEqual(TEXT("movimento rapido e normale condividono il codice 20"),
		URTCatalogLibrary::ResolutionPhaseCode(ERTResolutionPhase::FastMovement),
		URTCatalogLibrary::ResolutionPhaseCode(ERTResolutionPhase::NormalMovement));
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Validazione del catalogo: un catalogo incoerente deve fallire QUI, non in partita
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogIdsUniqueTest,
	"RefactorTactics.Catalog.IdsAreUnique",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogIdsUniqueTest::RunTest(const FString&)
{
	TArray<FRTActionDef> Good;
	Good.Add(MakeCatalogAction(TEXT("Action.Move"), ERTResolutionPhase::NormalMovement, 50, ERTActionFallback::Stop));
	Good.Add(MakeCatalogAction(TEXT("Action.BasicAttack"), ERTResolutionPhase::Attack, 50));
	TestEqual(TEXT("ID distinti: nessun errore"), URTCatalogLibrary::ValidateActions(Good).Num(), 0);

	TArray<FRTActionDef> Duplicated = Good;
	Duplicated.Add(MakeCatalogAction(TEXT("Action.Move"), ERTResolutionPhase::FastMovement, 30, ERTActionFallback::Stop));
	const TArray<FString> Errors = URTCatalogLibrary::ValidateActions(Duplicated);
	TestTrue(TEXT("ID duplicato: almeno un errore"), Errors.Num() > 0);
	bool bMentionsId = false;
	for (const FString& E : Errors) { bMentionsId |= E.Contains(TEXT("Action.Move")); }
	TestTrue(TEXT("l'errore dice QUALE id e' duplicato"), bMentionsId);
	return true;
}

/**
 * **Una riduzione di guardia negativa e' un ERRORE di catalogo, non un numero strano** ([D-408]).
 *
 * 🔴 **Senza questa convalida sarebbe un no-op SILENZIOSO**: `RTTurnManager` fa
 * `-FMath::Max(0, GuardReduction)`, quindi un valore negativo diventa delta `0`, `ApplyEligibleHitDelta`
 * salta il colpo, e `Action.Guard` smette di proteggere quell'eroe senza una riga da nessuna parte —
 * nessun log, nessun test rosso.
 *
 * 🔑 **`URTHeroData::GuardReduction` e' arrivato con [D-408] accanto a `PushResistance`**, che il
 * validator controlla dal principio; il trattamento parallelo si era fermato prima del validator. Trovato
 * da una code review.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogRejectsNegativeGuardTest,
	"RefactorTactics.Catalog.RejectsNegativeGuardReduction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogRejectsNegativeGuardTest::RunTest(const FString&)
{
	URTHeroData* Eroe = URTHeroCatalogLibrary::MakeBranth();
	if (!TestNotNull(TEXT("l'eroe di prova esiste"), Eroe)) { return false; }

	// PREMESSA: com'e' a catalogo passa. Senza, la riga sotto non distinguerebbe «il campo e' rifiutato»
	// da «questo eroe era gia' invalido per altro».
	TestEqual(TEXT("premessa: il roster com'e' non produce errori"),
		URTHeroCatalogLibrary::ValidateHeroes({ Eroe }).Num(), 0);

	Eroe->GuardReduction = -5;
	const TArray<FString> Errori = URTHeroCatalogLibrary::ValidateHeroes({ Eroe });
	TestTrue(TEXT("riduzione guardia negativa: almeno un errore"), Errori.Num() > 0);

	// ⚠️ E l'errore dice QUALE campo, come fanno gli altri di questo validator: un elenco che non lo dice
	// costringe a cercarlo, ed e' la ragione per cui `ValidateActions` nomina l'id duplicato.
	bool bNominaLaGuardia = false;
	for (const FString& E : Errori) { bNominaLaGuardia |= E.Contains(TEXT("guardia")); }
	TestTrue(TEXT("l'errore dice che si tratta della guardia"), bNominaLaGuardia);

	// ✅ E zero e' LECITO: un eroe che non mitiga e' una scelta di taratura, non un dato rotto.
	Eroe->GuardReduction = 0;
	TestEqual(TEXT("zero e' un valore legittimo, non un errore"),
		URTHeroCatalogLibrary::ValidateHeroes({ Eroe }).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogRejectsInvalidTest,
	"RefactorTactics.Catalog.ValidatorRejectsInvalidAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogRejectsInvalidTest::RunTest(const FString&)
{
	// Ogni caso e' verificato da solo: un validator che segnalasse sempre lo stesso errore passerebbe
	// un test cumulativo senza distinguere i casi.
	{
		TArray<FRTActionDef> NoId;
		NoId.Add(MakeCatalogAction(NAME_None, ERTResolutionPhase::Attack, 50));
		TestTrue(TEXT("ID mancante: rifiutato"), URTCatalogLibrary::ValidateActions(NoId).Num() > 0);
	}
	{
		TArray<FRTActionDef> NegativePriority;
		NegativePriority.Add(MakeCatalogAction(TEXT("Action.X"), ERTResolutionPhase::Attack, -1));
		TestTrue(TEXT("priorita' negativa: rifiutata"), URTCatalogLibrary::ValidateActions(NegativePriority).Num() > 0);
	}
	{
		TArray<FRTActionDef> NegativeCost;
		FRTActionDef Def = MakeCatalogAction(TEXT("Action.Y"), ERTResolutionPhase::NormalMovement, 50, ERTActionFallback::Stop);
		Def.CostMP = -3;
		NegativeCost.Add(Def);
		TestTrue(TEXT("costo negativo: rifiutato"), URTCatalogLibrary::ValidateActions(NegativeCost).Num() > 0);
	}
	{
		// Un'azione di movimento DEVE dichiarare se e' rapida (Dash) o normale (Move): la fase 20 si sdoppia,
		// e "in mezzo" non esiste. Il fallback di un movimento deve essere Stop (regola del vertical slice).
		TArray<FRTActionDef> WrongFallback;
		WrongFallback.Add(MakeCatalogAction(TEXT("Action.Move"), ERTResolutionPhase::NormalMovement, 50, ERTActionFallback::Cancel));
		TestTrue(TEXT("movimento con fallback diverso da Stop: rifiutato"),
			URTCatalogLibrary::ValidateActions(WrongFallback).Num() > 0);
	}
	{
		TArray<FRTActionDef> Snapshot;
		Snapshot.Add(MakeCatalogAction(TEXT("Action.Z"), ERTResolutionPhase::Snapshot, 10));
		TestTrue(TEXT("nessuna azione puo' risolvere nello Snapshot: rifiutata"),
			URTCatalogLibrary::ValidateActions(Snapshot).Num() > 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogPropagationTest,
	"RefactorTactics.Catalog.ValidatorRejectsUnboundedPropagation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogPropagationTest::RunTest(const FString&)
{
	// «Consentire propagazione elettrica senza limite» e' fra gli errori da evitare del catalogo: una
	// propagazione illimitata su una mappa d'acqua colpisce tutti e rende il turno impredicibile.
	FRTActionDef Unbounded = MakeCatalogAction(TEXT("Action.Electrify"), ERTResolutionPhase::Environment, 30);
	Unbounded.PropagationLimit = -1; // -1 = nessun limite

	TArray<FRTActionDef> Actions;
	Actions.Add(Unbounded);
	TestTrue(TEXT("propagazione illimitata: rifiutata"), URTCatalogLibrary::ValidateActions(Actions).Num() > 0);

	Actions[0].PropagationLimit = 3; // il valore del catalogo
	TestEqual(TEXT("propagazione limitata: accettata"), URTCatalogLibrary::ValidateActions(Actions).Num(), 0);

	Actions[0].PropagationLimit = 0; // azione che non propaga affatto
	TestEqual(TEXT("nessuna propagazione: accettata"), URTCatalogLibrary::ValidateActions(Actions).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogEquipmentTest,
	"RefactorTactics.Catalog.ValidatorRejectsEquipmentWithoutDrawback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogEquipmentTest::RunTest(const FString&)
{
	// Asset di prova VOLUTAMENTE invalido, costruito in memoria: non finisce in Content/RT di produzione.
	URTEquipmentData* NoDrawback = NewObject<URTEquipmentData>();
	NoDrawback->EquipmentId = TEXT("Weapon.Overcharge");
	NoDrawback->Slot = ERTEquipmentSlot::WeaponVariant;
	NoDrawback->Advantage = FText::FromString(TEXT("+6 danni"));
	// Drawback lasciato vuoto: e' esattamente il caso che il catalogo vieta.

	TArray<const URTEquipmentData*> Invalid;
	Invalid.Add(NoDrawback);
	const TArray<FString> Errors = URTCatalogLibrary::ValidateEquipment(Invalid);
	TestTrue(TEXT("equipaggiamento senza svantaggio: rifiutato"), Errors.Num() > 0);
	bool bNamesIt = false;
	for (const FString& E : Errors) { bNamesIt |= E.Contains(TEXT("Weapon.Overcharge")); }
	TestTrue(TEXT("l'errore dice quale equipaggiamento"), bNamesIt);

	// ⚠️ **Il contratto si è esteso con CP 7.1 (`#60`), e questo test lo registra.**
	//
	// Fino a qui bastava dichiarare lo svantaggio a parole, ed era tutto ciò che si poteva chiedere: `Drawback`
	// è una `FText` e i delta numerici non esistevano. Ora esistono, e per una VARIANTE D'ARMA la prosa da sola
	// non basta più — perché nessuna regola la legge, quindi una variante potrebbe raccontare «cooldown +1»
	// mentre i suoi numeri non fanno pagare niente. Sarebbe potere verticale con una didascalia rassicurante.
	//
	// Quindi il caso «accettato» ora dichiara il costo in entrambe le lingue, come le sei varianti vere.
	NoDrawback->Drawback = FText::FromString(TEXT("cooldown +1"));
	TestTrue(TEXT("lo svantaggio SOLO a parole non basta piu' per una variante d'arma"),
		URTCatalogLibrary::ValidateEquipment(Invalid).Num() > 0);

	NoDrawback->CooldownDeltaTurns = 1; // lo stesso costo, ora in una forma che il resolver sa applicare

	// ⚠️ **Il contratto si e' esteso di nuovo con #509**: i delta di danno sono PER FASCIA ([D-087]), e una
	// fascia non dichiarata non vale zero — varrebbe «questa variante non fa niente su quegli attacchi»,
	// cioe' una scelta morta travestita da omissione. Il validator la rifiuta, e qui si registra.
	TestTrue(TEXT("le fasce non dichiarate: rifiutato anche col costo misurabile"),
		URTCatalogLibrary::ValidateEquipment(Invalid).Num() > 0);

	NoDrawback->DamageDeltaByBand.Add(ERTAttackDamageBand::Low, 0);
	NoDrawback->DamageDeltaByBand.Add(ERTAttackDamageBand::Medium, 0);
	TestTrue(TEXT("due fasce su tre non bastano: manca `High`"),
		URTCatalogLibrary::ValidateEquipment(Invalid).Num() > 0);

	NoDrawback->DamageDeltaByBand.Add(ERTAttackDamageBand::High, 0);
	TestEqual(TEXT("con lo svantaggio dichiarato, misurabile E le tre fasce: accettato"),
		URTCatalogLibrary::ValidateEquipment(Invalid).Num(), 0);

	// Id duplicato fra due equipaggiamenti diversi.
	URTEquipmentData* Clone = NewObject<URTEquipmentData>();
	Clone->EquipmentId = TEXT("Weapon.Overcharge");
	Clone->Advantage = FText::FromString(TEXT("altro"));
	Clone->Drawback = FText::FromString(TEXT("altro svantaggio"));
	Invalid.Add(Clone);
	TestTrue(TEXT("id duplicato: rifiutato"), URTCatalogLibrary::ValidateEquipment(Invalid).Num() > 0);
	return true;
}


/**
 * 🔑 **Il budget di movimento ha una sede sola, e questo test e' il modo in cui resta sola** ([D-427]).
 *
 * La regola vale per **ogni** azione a `ERTMovementStyle::Budget`, non per le tre che si chiamano cosi'
 * oggi: pinnare `Sprint` e `Withdraw` per nome lascerebbe passare la quarta, che chi la aggiunge domani
 * scriverebbe con un `RangeCells` per abitudine — ricostruendo la seconda sede senza che nessun test cada.
 *
 * ⛔ **Il gate anti-vacuita' non e' decorativo**: un catalogo che smettesse di dichiarare azioni a budget
 * renderebbe il ciclo vero **misurando zero**, ed e' esattamente la forma con cui un gate muore. Percio' si
 * asserisce anche che ne abbia vista almeno una.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBudgetActionsDeclareNoRangeTest,
	"RefactorTactics.Catalog.BudgetActionsDeclareNoRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBudgetActionsDeclareNoRangeTest::RunTest(const FString&)
{
	int32 AzioniABudget = 0;
	for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
	{
		if (Def.MovementStyle != ERTMovementStyle::Budget) { continue; }
		++AzioniABudget;
		TestEqual(
			*FString::Printf(
				TEXT("%s e' a budget, quindi il suo RangeCells e' zero: il numero vive nel profilo (D-427)"),
				*Def.ActionId.ToString()),
			Def.RangeCells, 0);
	}

	TestTrue(TEXT("il catalogo dichiara almeno un'azione a stile Budget"), AzioniABudget > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogCoreActionsTest,
	"RefactorTactics.Catalog.ValidatorAcceptsCoreActions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogCoreActionsTest::RunTest(const FString&)
{
	// Le azioni generiche (`Action.*`) passano dallo stesso validator delle abilita' d'eroe: nessuna scorciatoia
	// per il fatto che sono "di sistema".
	const TArray<FRTActionDef> Core = URTCatalogLibrary::GetCoreActionCatalog();
	TestTrue(TEXT("il catalogo delle azioni generiche non e' vuoto"), Core.Num() > 0);

	const TArray<FString> Errors = URTCatalogLibrary::ValidateActions(Core);
	for (const FString& E : Errors) { AddError(E); }
	TestEqual(TEXT("le azioni generiche sono valide"), Errors.Num(), 0);

	// `Action.Sprint` come lo descrive il catalogo v0.1 §2: slot movimento, e dal 2026-09-12 PROFILO del
	// movimento normale invece che mobilita' rapida ([D-116] voce 1, `#641`).
	const FRTActionDef Sprint = URTCatalogLibrary::FindCoreAction(TEXT("Action.Sprint"));
	TestTrue(TEXT("Action.Sprint e' nel catalogo"), Sprint.ActionId == FName(TEXT("Action.Sprint")));
	TestTrue(TEXT("Sprint risolve nella fase Move, dopo il Blast (D-116)"),
		URTCatalogLibrary::MapResolutionPhase(Sprint.ResolutionPhase) == ERTMatchPhase::Move);
	// 🔑 **Il budget NON e' qui, ed e' questa asserzione a dirlo** ([D-427]). Valeva `8`; da quando il
	// budget e' un moltiplicatore del movimento dell'unita' ([D-412], `Sprint` ×2) un assoluto sull'azione
	// sarebbe una seconda sede — e divergeva gia': `8` contro `10 · 10 · 8 · 12` sul roster spedito.
	// ⛔ Un test che si limitasse a non guardare il campo lascerebbe rientrare il numero senza che nessuno
	// se ne accorga: qui si asserisce che e' **zero**, e il moltiplicatore lo pinna `RTMovementProfileTests`.
	TestEqual(TEXT("il budget dello Sprint non vive sull'azione: RangeCells e' zero"), Sprint.RangeCells, 0);
	// D-028: il solo slot movimento. Prima era `MovementAndMain`, e il costo dello scatto lungo era
	// strutturale; ora il prezzo e' tutto nei dati (`Exposed` e nessuna reazione) — vedi `BAL-1`.
	TestTrue(TEXT("Sprint consuma il solo slot movimento"), Sprint.Slot == ERTActionSlot::Movement);
	TestEqual(TEXT("Sprint dichiara un solo effetto: lo stato"), Sprint.Effects.Num(), 1);
	// ⚠️ **DUE turni, e il numero e' legato alla fase qui sopra** ([D-116] voce 4): con lo Sprint dopo il
	// Blast nessuna fase legge `Exposed` prima del Cleanup, quindi `1` lo renderebbe inerte. Le due
	// asserzioni cadono insieme se qualcuno migra la fase e lascia indietro il prezzo — che e' il solo caso
	// che la decisione vuole impedire.
	TestTrue(TEXT("e quell'effetto e' Status.Exposed per DUE turni (D-116)"),
		Sprint.Effects.Num() == 1 && Sprint.Effects[0].Effect == ERTActionEffect::Status
		&& Sprint.Effects[0].StatusTag == TAG_Status_Exposed && Sprint.Effects[0].StatusDuration == 2);

	// Un ID non catalogato non inventa un'azione: torna una definizione vuota.
	TestTrue(TEXT("ID sconosciuto -> definizione vuota"),
		URTCatalogLibrary::FindCoreAction(TEXT("Action.NonEsiste")).ActionId.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogMatchesAbilitiesTest,
	"RefactorTactics.Catalog.ShippedCatalogMatchesAbilities",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogMatchesAbilitiesTest::RunTest(const FString&)
{
	// Il catalogo non deve poter divergere dalle abilita' che il gioco assegna davvero: qui si confronta
	// definizione e abilita' campo per campo, su TUTTO IL ROSTER.
	//
	// Prima girava sui due archetipi legacy, cioe' su unita' che nessuna partita schierava piu': verificava
	// l'allineamento del catalogo su un percorso morto. Ora gira sui quattro eroi che il GameMode schiera
	// davvero — due unita' in meno da cui dipendere, due in piu' che contano.
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		ARTUnit* Unit = NewObject<ARTUnit>();
		if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
		Unit->ConfigureFromHeroData(Hero);

		TestTrue(TEXT("l'eroe ha abilita'"), Unit->NumAbilities() > 0);

		// Solo le azioni DELL'EROE, non le generiche che `ConfigureFromHeroData` accoda (D-025).
		// L'invariante qui e' «la definizione di catalogo e i campi specchio dell'abilita' non divergono»,
		// e vale per le azioni che il catalogo eroi costruisce con `MakeHeroAction`. Le sette generiche
		// arrivano da `MakeGenericActions` e lasciano i campi legacy a zero: includerle non misurerebbe una
		// divergenza del catalogo, misurerebbe che sono due costruttori diversi — cosa gia' vera per
		// disegno. Il test degli archetipi guardava le loro quattro abilita' e nient'altro: stesso perimetro.
		const int32 NumHeroActions = Hero ? Hero->Actions.Num() : 0;
		for (int32 i = 0; i < NumHeroActions; ++i)
		{
			const URTActionData* Ability = Unit->GetAbility(i);
			if (!Ability) { continue; }

			const FString Name = Ability->DisplayName.ToString();
			TestFalse(FString::Printf(TEXT("%s ha un ActionId di catalogo"), *Name), Ability->Def.ActionId.IsNone());
			TestEqual(FString::Printf(TEXT("%s: portata coerente col catalogo"), *Name),
				Ability->Def.RangeCells, Ability->RangeCells);
			TestEqual(FString::Printf(TEXT("%s: ricarica coerente col catalogo"), *Name),
				Ability->Def.CooldownTurns, Ability->CooldownTurns);

			// La fase dichiarata deve corrispondere alla natura dell'abilita': uno scatto risolve nel Dash,
			// un supporto su se stessi nel Prep, un attacco nel Blast.
			const ERTMatchPhase Macro = URTCatalogLibrary::MapResolutionPhase(Ability->Def.ResolutionPhase);

			// Stile di movimento e fase sono due campi INDIPENDENTI, e la verifica va fatta nelle due
			// direzioni. Asserire «fase Dash» dentro un ramo scelto da `IsFastMovement` sarebbe invece una
			// tautologia: quel predicato E' definito come «macro-fase == Dash».
			const bool bDeclaresMovement = Ability->Def.MovementStyle != ERTMovementStyle::None;
			if (URTCatalogLibrary::IsFastMovement(Ability->Def))
			{
				// Risolve nel Dash: deve dire COME si sposta, o il resolver ricade sul pathfinding (#142).
				TestTrue(FString::Printf(TEXT("%s risolve nel Dash: dichiara COME si sposta"), *Name), bDeclaresMovement);
			}
			else if (bDeclaresMovement)
			{
				// Dichiara di spostare ma non e' fase Dash: l'unico caso legittimo e' il movimento NORMALE
				// (`Action.Move`, stile Budget). E' la meta' che scopre uno scatto catalogato con la fase
				// sbagliata — un `LinearDash` in fase Attack non si muoverebbe mai.
				TestEqual(FString::Printf(TEXT("%s si sposta fuori dal Dash: puo' solo essere il Move"), *Name),
					Macro, ERTMatchPhase::Move);
			}

			// Cio' che NON e' mobilita' si classifica dalla sua natura: un supporto su se stessi si prepara,
			// tutto il resto colpisce.
			// Le REAZIONI sono una terza categoria e non si classificano cosi': non sono mobilita' e non
			// sono supporto su se stessi, ma nemmeno attacchi — risolvono quando il loro trigger scatta, non
			// nella fase in cui colpisce chi le ha dichiarate. I due archetipi legacy non ne avevano
			// (quattro slot: attacchi, barriera, carica), quindi «tutto il resto colpisce» reggeva; il roster
			// ne ha, e senza questa esclusione il test chiederebbe la fase Blast a `Branth.Interposition`.
			if (!URTCatalogLibrary::IsFastMovement(Ability->Def)
				&& Ability->Def.Slot != ERTActionSlot::Reaction)
			{
				if (Ability->bSelfTarget)
				{
					TestEqual(FString::Printf(TEXT("%s e' un supporto -> fase Prep"), *Name), Macro, ERTMatchPhase::Prep);
				}
				else if (!bDeclaresMovement)
				{
					// «Tutto il resto colpisce» descriveva i quattro slot degli archetipi legacy. Un kit
					// d'eroe ha almeno quattro categorie, e le ultime due non colpiscono affatto:
					//   - si PREPARA senza essere supporto su se' — `Branth.Reconfigure`, `Phase.FlowReaction`,
					//     `Ivrin.InterceptShot` (fase Prep);
					//   - agisce sull'AMBIENTE — `Aevik.ConductiveNode`, `Phase.FluidTrail`, `Phase.MistVeil`,
					//     `Branth.KineticPanel`, che ereditano la fase dalle azioni core d'ambiente e
					//     risolvono nel Cleanup, dopo il Move, per colpire anche chi e' appena entrato.
					// La proprieta' che regge tutte e' che l'azione risolva in una fase in cui si GIOCA:
					// `Snapshot`, `Planning` e `MatchEnded` non sono destinazioni per un'azione dichiarata
					// da un'unita' — ci finirebbe senza che nessuno la risolva.
					//
					// L'`ActionId` nel messaggio e non il `DisplayName`: le azioni d'eroe non lo valorizzano,
					// e un fallimento diceva «' risolve in una fase giocabile'» senza dire di chi.
					const FString Who = Ability->Def.ActionId.ToString();
					TestTrue(FString::Printf(TEXT("%s risolve in una fase giocabile"), *Who),
						Macro == ERTMatchPhase::Prep || Macro == ERTMatchPhase::Blast
						|| Macro == ERTMatchPhase::Move || Macro == ERTMatchPhase::Cleanup);
				}
			}
		}
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// G7 — IL PERIMETRO DEI DATI DI GAMEPLAY, ENUMERATO INVECE CHE IMPLICITO (#3370)
//
// Fino a `#3370` l'invariante #4 era verificato su UNA struct, `FRTActionDef`, e ogni altra classe di dati
// era fuori senza che nessuno l'avesse deciso. Questo blocco ribalta l'onere: il perimetro si SCOPRE per
// reflection, ogni float che ci cade dentro va DICHIARATO, e una radice nuova non entra in silenzio.
//
// 🔑 **La domanda e' sulla FORMA del dato, non sul valore**, e la ragione e' in `#3370`: una `UPROPERTY
// int32` non puo' ospitare `1.5` — l'editor tronca — quindi cercare valori frazionari in un campo intero e'
// una ricerca che non si distingue da una che non guarda. La domanda ponibile e' *«esiste un campo in
// virgola mobile, raggiungibile da un asset versionato, che alimenti un costo, una priorita', un danno o una
// soglia?»*, e a quella risponde la reflection.
//
// ⛔ **Una macchina non sa dire se un float «alimenta un costo».** Percio' la regola implementata e' la sua
// forma conservativa — *nessun float, in nessun punto del perimetro* — e ogni eccezione porta la ragione per
// cui quel float NON e' un costo. Il giudizio umano sta scritto una volta, in `EccezioniVirgolaMobile`,
// invece di essere rifatto a ogni lettura.
// ---------------------------------------------------------------------------------------------------------
namespace RTG7Perimetro
{
	/** Un campo trovato, con il tipo che lo dichiara. I nomi sono quelli della reflection, senza prefisso C++. */
	struct FCampo
	{
		FString Tipo;
		FString Campo;
	};

	/**
	 * Un tipo appartiene al progetto? La scoperta guarda QUI e non nell'Engine, e non e' pigrizia:
	 * `FLinearColor` ha quattro float e `FVector` tre, e nessuno dei due e' un posto dove un autore scrive
	 * un costo. Scendervi produrrebbe un elenco di eccezioni lungo quanto l'Engine e vuoto di significato.
	 */
	static bool AppartieneAlProgetto(const UStruct* Tipo)
	{
		return Tipo != nullptr && Tipo->GetPathName().StartsWith(TEXT("/Script/RefactorTactics"));
	}

	/**
	 * LE RADICI ATTESE: ogni `UDataAsset` dichiarato dai moduli del progetto, cioe' ogni classe che un
	 * `.uasset` versionato puo' istanziare.
	 *
	 * ⛔ **Non e' una lista da consultare: e' una lista da CONFRONTARE.** Le radici si scoprono per
	 * reflection e `GameplayDataPerimeterIsDeclared` pretende che i due insiemi coincidano — una classe di
	 * dati nuova FA FALLIRE il test finche' qualcuno non la dichiara qui, che e' il criterio di `#3370`
	 * *«una classe nuova che non vi compare fa fallire il test invece di essere saltata in silenzio»*.
	 */
	static const TCHAR* const RadiciAttese[] = {
		TEXT("RTActionData"),
		TEXT("RTEquipmentData"),
		TEXT("RTHeroData"),
		TEXT("RTHexMapAsset"),
		TEXT("RTIconCatalogData"),
		TEXT("RTMatchFormatData"),
	};

	/**
	 * I FLOAT AMMESSI, uno per riga, con la ragione per cui non sono un costo.
	 *
	 * ⚠️ Una riga che non trova piu' il suo campo e' un ERRORE, non un'innocuita': un'eccezione stantia e'
	 * un permesso che sopravvive al proprio soggetto, e il giorno in cui un campo omonimo ricomparisse
	 * sarebbe gia' autorizzato.
	 */
	struct FEccezione
	{
		const TCHAR* Tipo;
		const TCHAR* Campo;
		const TCHAR* Ragione;
	};
	static const FEccezione EccezioniVirgolaMobile[] = {
		{ TEXT("RTHexMapAsset"), TEXT("HexSize"),
			TEXT("geometria: lato dell'esagono in unita' di mondo, alimenta la conversione cella->posizione") },
		{ TEXT("RTHexMapAsset"), TEXT("LayerHeight"),
			TEXT("geometria: altezza del piano in unita' di mondo, non entra in nessuna regola") },
	};

	/**
	 * COPERTURA MINIMA: i tipi che la visita DEVE raggiungere.
	 *
	 * 🔴 Senza questo elenco il verde sarebbe ambiguo: una visita che si rompesse — un `TArray` rimosso, un
	 * contenitore nuovo che lo srotolatore non conosce — ispezionerebbe meno e resterebbe verde lo stesso.
	 * Qui il restringimento del perimetro diventa un fallimento con un nome.
	 */
	static const TCHAR* const DeveRaggiungere[] = {
		TEXT("RTActionDef"),        // costi, priorita', range, cooldown
		TEXT("RTAbilityVariant"),   // varianti di workbench
		TEXT("RTActionEffectSpec"), // danni, cure, scudi, durate
		TEXT("RTHexCellData"),      // MoveCost, OccupancySurcharge
		TEXT("RTHexCover"),         // integrita' della copertura
		TEXT("RTHexDoor"),
		TEXT("RTHexEdge"),          // costo di transizione, integrita'
		TEXT("RTCellId"),
		TEXT("RTHexInteriorWall"),
		TEXT("RTGeometrySegment"),
		TEXT("RTAnchorRef"),
		TEXT("RTNoWalkArea"),
		TEXT("RTBoxVolume"),
		TEXT("RTInteractionBinding"),
		TEXT("RTIconDef"),
	};

	/** Srotola i contenitori fino alle proprieta' foglia: un `TArray<float>` nasconde un float, e va visto. */
	static void Foglie(FProperty* Prop, TArray<FProperty*>& Out)
	{
		if (Prop == nullptr) { return; }
		if (FArrayProperty* Arr = CastField<FArrayProperty>(Prop)) { Foglie(Arr->Inner, Out); return; }
		if (FSetProperty* Set = CastField<FSetProperty>(Prop)) { Foglie(Set->ElementProp, Out); return; }
		if (FMapProperty* Map = CastField<FMapProperty>(Prop))
		{
			Foglie(Map->KeyProp, Out);
			Foglie(Map->ValueProp, Out);
			return;
		}
		Out.Add(Prop);
	}

	/**
	 * Visita transitiva del grafo delle `UPROPERTY` a partire da un tipo, raccogliendo i campi in virgola
	 * mobile e i riferimenti a classe.
	 *
	 * ⚠️ **`ExcludeSuper` piu' risalita esplicita**, e non e' un dettaglio: iterando con `IncludeSuper` la
	 * visita di un `UDataAsset` vedrebbe anche le proprieta' dell'Engine — fra cui `UDataAsset::NativeClass`,
	 * che e' un riferimento a classe — e le attribuirebbe alla nostra radice. Qui si risale solo finche' il
	 * padre appartiene al progetto.
	 */
	static void Attraversa(UStruct* Tipo, TArray<UStruct*>& Visitati, TArray<FCampo>& Virgola,
		TArray<FCampo>& RiferimentiAClasse)
	{
		if (Tipo == nullptr || Visitati.Contains(Tipo)) { return; }
		Visitati.Add(Tipo);

		for (TFieldIterator<FProperty> It(Tipo, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			TArray<FProperty*> Leaves;
			Foglie(*It, Leaves);
			for (FProperty* Leaf : Leaves)
			{
				if (Leaf->IsA<FFloatProperty>() || Leaf->IsA<FDoubleProperty>())
				{
					Virgola.Add(FCampo{ Tipo->GetName(), It->GetName() });
					continue;
				}
				if (CastField<FClassProperty>(Leaf) != nullptr || CastField<FSoftClassProperty>(Leaf) != nullptr)
				{
					RiferimentiAClasse.Add(FCampo{ Tipo->GetName(), It->GetName() });
					continue;
				}
				if (FStructProperty* AsStruct = CastField<FStructProperty>(Leaf))
				{
					UScriptStruct* Inner = AsStruct->Struct;
					if (AppartieneAlProgetto(Inner)) { Attraversa(Inner, Visitati, Virgola, RiferimentiAClasse); }
					continue;
				}
				if (FObjectPropertyBase* AsObject = CastField<FObjectPropertyBase>(Leaf))
				{
					UClass* Target = AsObject->PropertyClass;
					if (AppartieneAlProgetto(Target)) { Attraversa(Target, Visitati, Virgola, RiferimentiAClasse); }
				}
			}
		}

		if (UStruct* Padre = Tipo->GetSuperStruct())
		{
			if (AppartieneAlProgetto(Padre)) { Attraversa(Padre, Visitati, Virgola, RiferimentiAClasse); }
		}
	}

	/** Le radici REALI, scoperte per reflection: ogni `UDataAsset` concreto dichiarato dai moduli. */
	static void ScopriRadici(TArray<UClass*>& Out)
	{
		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* Candidata = *It;
			if (!AppartieneAlProgetto(Candidata)) { continue; }
			if (!Candidata->IsChildOf(UDataAsset::StaticClass())) { continue; }
			if (Candidata->HasAnyClassFlags(CLASS_Abstract)) { continue; }
			Out.Add(Candidata);
		}
		Out.Sort([](const UClass& A, const UClass& B) { return A.GetName().Compare(B.GetName()) < 0; });
	}

	/** Il tipo e' stato raggiunto dalla visita? */
	static bool Raggiunto(const TArray<UStruct*>& Visitati, const TCHAR* Nome)
	{
		return Visitati.ContainsByPredicate([Nome](const UStruct* T) { return T != nullptr && T->GetName() == Nome; });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogNoFloatTest,
	"RefactorTactics.Catalog.NoFloatInIntegerFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogNoFloatTest::RunTest(const FString&)
{
	using namespace RTG7Perimetro;

	// CONTROLLO POSITIVO DEL RILEVATORE — `#3370`: un verde deve poter essere rosso.
	// `FRTVeilTransitionParams` porta tre float di TEMPO (apertura, chiusura, epsilon di arrivo): sta fuori
	// dal perimetro per costruzione — nessun asset versionato lo istanzia e nessuna regola lo legge — ed e'
	// per questo che serve qui. Se questa riga cadesse, la visita non vede piu' i float e ogni verde sotto
	// sarebbe vacuo.
	{
		TArray<UStruct*> Prova;
		TArray<FCampo> ProvaVirgola;
		TArray<FCampo> ProvaClassi;
		Attraversa(FRTVeilTransitionParams::StaticStruct(), Prova, ProvaVirgola, ProvaClassi);
		TestTrue(TEXT("controllo positivo: il rilevatore vede i float dove ci sono (FRTVeilTransitionParams)"),
			ProvaVirgola.Num() > 0);
	}

	TArray<UClass*> Radici;
	ScopriRadici(Radici);
	if (!TestTrue(TEXT("la reflection trova almeno una classe di dati del progetto"), Radici.Num() > 0))
	{
		return false;
	}

	// Si visitano le radici SCOPERTE, non quelle dichiarate: una classe nuova viene ispezionata gia' al
	// primo giro, e la sua mancata dichiarazione la segnala l'altro test invece di farla saltare.
	TArray<UStruct*> Visitati;
	TArray<FCampo> Virgola;
	TArray<FCampo> Classi;
	for (UClass* Radice : Radici)
	{
		Attraversa(Radice, Visitati, Virgola, Classi);
	}

	for (const TCHAR* Atteso : DeveRaggiungere)
	{
		if (!Raggiunto(Visitati, Atteso))
		{
			AddError(FString::Printf(
				TEXT("il perimetro si e' RISTRETTO: %s non e' piu' raggiungibile da nessuna classe di dati. ")
				TEXT("O il campo che lo portava e' stato rimosso, o la visita non sa piu' srotolare il suo ")
				TEXT("contenitore — in entrambi i casi il verde di questo test coprirebbe meno di prima."),
				Atteso));
		}
	}

	const int32 NumEccezioni = UE_ARRAY_COUNT(EccezioniVirgolaMobile);
	TArray<bool> EccezioneUsata;
	EccezioneUsata.Init(false, NumEccezioni);

	for (const FCampo& Trovato : Virgola)
	{
		int32 Indice = INDEX_NONE;
		for (int32 i = 0; i < NumEccezioni; ++i)
		{
			if (Trovato.Tipo == EccezioniVirgolaMobile[i].Tipo && Trovato.Campo == EccezioniVirgolaMobile[i].Campo)
			{
				Indice = i;
				break;
			}
		}
		if (Indice == INDEX_NONE)
		{
			AddError(FString::Printf(
				TEXT("campo in virgola mobile NON dichiarato dentro il perimetro dei dati di gameplay: %s::%s. ")
				TEXT("Se alimenta un costo, una priorita', un danno o una soglia e' una violazione dell'invariante ")
				TEXT("#4 e va tolto; se e' presentazione, geometria o tempo va dichiarato in ")
				TEXT("RTG7Perimetro::EccezioniVirgolaMobile con la sua ragione."),
				*Trovato.Tipo, *Trovato.Campo));
		}
		else
		{
			EccezioneUsata[Indice] = true;
		}
	}

	for (int32 i = 0; i < NumEccezioni; ++i)
	{
		if (!EccezioneUsata[i])
		{
			AddError(FString::Printf(
				TEXT("eccezione stantia: %s::%s e' dichiarato ammesso ma non esiste piu' nel perimetro. ")
				TEXT("Una riga che sopravvive al proprio campo e' un permesso gia' concesso a un omonimo futuro."),
				EccezioniVirgolaMobile[i].Tipo, EccezioniVirgolaMobile[i].Campo));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGameplayDataPerimeterDeclaredTest,
	"RefactorTactics.Catalog.GameplayDataPerimeterIsDeclared",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGameplayDataPerimeterDeclaredTest::RunTest(const FString&)
{
	using namespace RTG7Perimetro;

	TArray<UClass*> Radici;
	ScopriRadici(Radici);
	if (!TestTrue(TEXT("la reflection trova almeno una classe di dati del progetto"), Radici.Num() > 0))
	{
		return false;
	}

	// Scoperte ma non dichiarate: e' il caso che `#3370` chiede di far FALLIRE.
	for (const UClass* Trovata : Radici)
	{
		bool bDichiarata = false;
		for (const TCHAR* Attesa : RadiciAttese)
		{
			if (Trovata->GetName() == Attesa) { bDichiarata = true; break; }
		}
		if (!bDichiarata)
		{
			AddError(FString::Printf(
				TEXT("classe di dati NON dichiarata nel perimetro: %s. Dichiararla in ")
				TEXT("RTG7Perimetro::RadiciAttese, oppure — se i suoi numeri non sono di gameplay — dichiararlo ")
				TEXT("per iscritto accanto alla riga."),
				*Trovata->GetName()));
		}
	}

	// Dichiarate ma non piu' esistenti: la lista non puo' invecchiare in silenzio.
	for (const TCHAR* Attesa : RadiciAttese)
	{
		const bool bEsiste = Radici.ContainsByPredicate(
			[Attesa](const UClass* C) { return C != nullptr && C->GetName() == Attesa; });
		if (!bEsiste)
		{
			AddError(FString::Printf(
				TEXT("radice dichiarata ma assente dalla reflection: %s. O e' stata rinominata o rimossa, e ")
				TEXT("in entrambi i casi il perimetro copre meno di quanto dichiara."),
				Attesa));
		}
	}

	// IL BUCO CHE RESTEREBBE MUTO: un riferimento a classe dentro un dato versionato.
	// La visita non segue `TSubclassOf`/soft class perche' puntano a un Blueprint, i cui default sono
	// authoring che la reflection sul nativo non vede. Oggi nessun tipo del perimetro ne dichiara uno; il
	// giorno in cui ne comparisse uno, questo errore obbliga a decidere invece di lasciare un ramo cieco.
	TArray<UStruct*> Visitati;
	TArray<FCampo> Virgola;
	TArray<FCampo> Classi;
	for (UClass* Radice : Radici)
	{
		Attraversa(Radice, Visitati, Virgola, Classi);
	}
	for (const FCampo& Riferimento : Classi)
	{
		AddError(FString::Printf(
			TEXT("riferimento a classe dentro il perimetro: %s::%s. Punta a un Blueprint, i cui default ")
			TEXT("sono dati authorati che questa visita NON vede: va deciso se seguirlo o escluderlo per iscritto."),
			*Riferimento.Tipo, *Riferimento.Campo));
	}

	TestTrue(TEXT("la visita ha ispezionato piu' tipi delle sole radici"), Visitati.Num() > Radici.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitGameplayNumbersAreIntegersTest,
	"RefactorTactics.Catalog.PlacedUnitGameplayNumbersAreIntegers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitGameplayNumbersAreIntegersTest::RunTest(const FString&)
{
	// ⛔ `ARTUnit` NON entra nel perimetro per intero, ed e' una scelta dichiarata: e' un Actor che tiene
	// insieme stato canonico e presentazione, e i suoi float — `VisualZOffset`, `VisualRunSpeed`,
	// `MeshYawOffset` — sono esattamente l'audit di presentazione che `#3370` mette fuori scope. Ispezionarlo
	// tutto produrrebbe un elenco di eccezioni di presentazione, cioe' rumore.
	//
	// ✅ Ma i numeri competitivi di un'unita' PIAZZATA in una `.umap` versionata vivono qui, quindi la
	// domanda si pone per NOME invece che per forma: questi campi devono essere interi.
	static const TCHAR* const CampiCompetitivi[] = {
		TEXT("MaxHealth"), TEXT("Health"), TEXT("Shield"), TEXT("TemporaryShield"),
		TEXT("AttackRange"), TEXT("AttackPower"), TEXT("MoveRange"), TEXT("GuardReduction"),
		TEXT("PushResistance"), TEXT("VisionRange"), TEXT("HearingThreshold"),
		TEXT("UltimateMultiplier"), TEXT("UltimateRadius"),
	};

	UClass* Unita = ARTUnit::StaticClass();
	if (!TestNotNull(TEXT("ARTUnit e' una UCLASS riflessa"), Unita)) { return false; }

	for (const TCHAR* Nome : CampiCompetitivi)
	{
		FProperty* Campo = Unita->FindPropertyByName(FName(Nome));
		if (Campo == nullptr)
		{
			// Un campo sparito NON e' un pass: sarebbe un asserto che non guarda piu' niente.
			AddError(FString::Printf(
				TEXT("ARTUnit::%s non esiste piu': l'asserto su di esso sarebbe vacuo. Rinominato? ")
				TEXT("Aggiornare l'elenco, non rimuovere la riga."),
				Nome));
			continue;
		}
		if (!Campo->IsA<FIntProperty>())
		{
			AddError(FString::Printf(
				TEXT("ARTUnit::%s non e' un intero (%s): un numero competitivo in virgola mobile viola ")
				TEXT("l'invariante #4."),
				Nome, *Campo->GetCPPType()));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionDefDerivedFromIsEmptyByDefaultTest,
	"RefactorTactics.Catalog.DerivedFromIsEmptyByDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionDefDerivedFromIsEmptyByDefaultTest::RunTest(const FString&)
{
	// Il default DEVE essere vuoto: il gate legge l'assenza come «non deriva da nulla», e un default
	// diverso da `NAME_None` darebbe a ogni azione una derivazione che nessuno ha dichiarato.
	const FRTActionDef Vuoto;
	TestTrue(TEXT("una definizione appena costruita non dichiara derivazione"),
		Vuoto.DerivedFromActionId.IsNone());

	// E non e' `BaseActionId`: due campi, due domande (D-033 contro «da dove vengono i numeri»). Un'azione
	// del catalogo core non deriva da se stessa — se qualcuno fondesse i due campi, questo cadrebbe.
	const FRTActionDef Core = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"));
	TestEqual(TEXT("l'azione core esiste, altrimenti l'asserto sotto sarebbe vacuo"),
		Core.ActionId, FName(TEXT("Action.Charge")));
	TestTrue(TEXT("un'azione del catalogo core non dichiara di derivare da qualcosa"),
		Core.DerivedFromActionId.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogReachableOrDeclaredTest,
	"RefactorTactics.Catalog.EveryCoreActionIsReachableOrDeclared",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogReachableOrDeclaredTest::RunTest(const FString&)
{
	// Un'azione del catalogo core arriva in partita per quattro vie. Tre sono interrogabili qui; la
	// quarta — il motore che la scrive da se', come `Action.Move` in `MakePlanFor` — e' una proprieta' del
	// SORGENTE e non del dato, quindi non si misura: vive nell'elenco sotto con la sua ragione.
	TSet<FName> Raggiungibili;

	for (const FName& Id : URTCatalogLibrary::GetGenericActionIds())
	{
		Raggiungibili.Add(Id);
	}
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Hero) { continue; }
		for (const URTActionData* A : Hero->Actions)
		{
			if (!A) { continue; }
			// Due campi, due vie diverse e ugualmente valide: «un eroe porta un'abilita' che eredita da X»
			// e «un eroe porta un profilo di X» (D-033, che e' il caso degli attacchi base).
			if (!A->Def.DerivedFromActionId.IsNone()) { Raggiungibili.Add(A->Def.DerivedFromActionId); }
			if (!A->Def.BaseActionId.IsNone())        { Raggiungibili.Add(A->Def.BaseActionId); }
		}
	}

	// Terza via: l'EQUIPAGGIAMENTO DI DEFAULT. Ogni eroe del roster entra in campo con un loadout —
	// `SetupHexMatch` lo consegna, e `Heroes.SpawnedUnitCarriesItsDefaultLoadout` lo pinna — e i pezzi che
	// concedono un'azione la portano con se'.
	//
	// ⚠️ La prima stesura di questo gate NON aveva questa via, e dichiarava che «nessuna unita' riceve
	// equipaggiamento in partita». Era falso, ed e' stato un `grep` cercato nel posto sbagliato: il canale
	// non nomina mai `EquipmentId`, passa da `DefaultLoadoutFor` e `GrantedActionId`. Due azioni finivano
	// dichiarate irraggiungibili mentre un eroe le portava.
	//
	// Si misura sul loadout di DEFAULT, non sul catalogo equipaggiamento: un pezzo che esiste e che nessun
	// eroe porta non e' raggiungibile in partita piu' di quanto lo sia un'azione senza portatore.
	for (const FName& HeroId : URTHeroCatalogLibrary::GetHeroIds())
	{
		for (const FName& PieceId : URTCatalogLibrary::DefaultLoadoutFor(HeroId))
		{
			const URTEquipmentData* Piece = URTCatalogLibrary::FindEquipment(PieceId);
			if (!Piece) { continue; }
			// Si passa da `MakeEquipmentAction` invece di leggere `GrantedActionId` a mano, perche' e' il
			// percorso che il gioco esegue: se un giorno smettesse di concedere l'azione, il gate lo vedrebbe.
			if (const URTActionData* Granted = URTCatalogLibrary::MakeEquipmentAction(Piece, nullptr))
			{
				if (!Granted->Def.DerivedFromActionId.IsNone())
				{
					Raggiungibili.Add(Granted->Def.DerivedFromActionId);
				}
			}
		}
	}

	// Le eccezioni, ognuna con la sua ragione. Le categorie invecchiano in modo diverso, e il prefisso della
	// ragione dice quale sia:
	//   `Motore:`               cade se il gameplay smette di produrre quell'azione;
	//   `Pezzo non assegnato:`  cade quando un eroe riceve quel pezzo nel proprio loadout;
	//   `Aspetta il suo eroe`   cade quando entra in un kit;
	//   `Fuori dalla v0.1 ...`  NON cade da sola — la tiene una DECISIONE, e solo una decisione la toglie.
	// ⚠️ **La quarta e' diversa in natura dalle altre tre**, e il verso 2 qui sotto lo dice nel proprio
	// messaggio: per le prime tre la raggiungibilita' e' la buona notizia, per la quarta e' il difetto.
	const TMap<FName, FString> Dichiarate = {
		// Scritte dal motore: il gameplay le produce senza passare da un kit.
		{ TEXT("Action.Move"),            TEXT("Motore: MakePlanFor la aggiunge quando l'unita' si muove") },
		{ TEXT("Action.Heal"),            TEXT("Motore: RTTurnManager la scrive come voce di cura") },
		{ TEXT("Action.Interrupt"),       TEXT("Motore: raccolta e applicata dal TurnManager") },
		{ TEXT("Action.ModifyArc"),       TEXT("Motore: scritta dal TurnManager") },
		// Fuori dalla v0.1 per DECISIONE, non per un portatore che manca. ⛔ Se una voce di QUESTO gruppo
		// diventasse raggiungibile, il difetto e' la raggiungibilita': si toglie quella, non la riga.
		//
		// 🔴 **Migrata il 2026-09-22** ([D-264], `#1403`): la riga resta, e cambiano CATEGORIA e RAGIONE.
		// Stava fra le «scritte dal motore» qui sopra e citava `#1389` — CHIUSA dal 2026-08-27 — mentre la
		// decisione che governa davvero questa esclusione e' [D-264] (2026-08-30): l'`Action.Cleanse` ATTIVA
		// esce dalla v0.1, e `PlannedCleansePriority` non si implementa per tenerla in vita.
		// E la categoria era sbagliata al contrario: il motore non la PRODUCE — `ResolveCleanseActions` la
		// CERCA fra le abilita' dell'unita' e nessuno gliela mette in mano. E' un consumatore senza
		// produttore, cioe' l'opposto di `Action.Move` qui sopra.
		// ⛔ **Il Cleanse REATTIVO non e' questa riga e non si tocca**: e' il rischio di lettura che [D-264]
		// dichiara. `Reaction.Cleanse` (base `Action.Purge`) e' il modulo di default di `Hero.Branth`, resta in
		// campo, ed e' pinnato da `Equipment.Cleanse.CancelsControl` — non da `Reactions.Cleanse.*`, che a
		// dispetto del nome esercitano l'ATTIVA.
		// Esce di qui solo alle condizioni che [D-264] nomina — ruolo tattico distinto piu' contratto esplicito
		// di UI e di produttore — non come ripristino di cio' che e' stato tolto.
		{ TEXT("Action.Cleanse"),         TEXT("Fuori dalla v0.1 per DECISIONE D-264: l'attiva esce, il reattivo resta") },
		// Concesse da un pezzo che ESISTE ma che nessun eroe porta di default: il canale funziona, manca
		// l'assegnazione. Escono da qui il giorno in cui un eroe riceve quel pezzo nel suo loadout.
		{ TEXT("Action.Anchor"),          TEXT("Pezzo non assegnato: base di Reaction.Anchor, default di nessuno") },
		{ TEXT("Action.CreateSmoke"),     TEXT("Pezzo non assegnato: Gadget.SmokeEmitter esiste, default di nessuno") },
		// `Action.Purge` e' USCITA da questo elenco il 2026-08-27 ([D-218], `#1403`): `Reaction.Cleanse` e'
		// il modulo di default di Branth, quindi la base e' raggiungibile. La riga la toglie il gate stesso,
		// che dice «ORA e' raggiungibile: togli la riga» invece di lasciarla marcire fra le esclusioni.
		// 🔴 **Migrata il 2026-09-13** ([D-116], `#641`): la riga resta, e cambia CATEGORIA. Non e' piu'
		// «bloccata da una migrazione decisa e non fatta» — e' scritta dal motore, come `Action.Move` qui
		// sopra, perche' `MakePlanFor` la aggiunge quando il giocatore sceglie quel profilo di movimento.
		// Esce da questo elenco il giorno in cui un eroe la portasse nel proprio kit, non prima.
		{ TEXT("Action.Sprint"),          TEXT("Motore: MakePlanFor la aggiunge quando il profilo scelto e' Sprint") },
		// Stessa via, e un vincolo in piu': non la SCEGLIE il giocatore — la impone l'Overwatch ([D-070]),
		// che riserva lo slot movimento al solo ripiegamento. Offrirla fra le scelte sarebbe una seconda
		// verita' sullo stesso vincolo (`#1410` `AC-4`).
		{ TEXT("Action.Withdraw"),        TEXT("Motore: imposta da chi arma l'Overwatch, D-070") },
		// Contenuto che aspetta il suo portatore: diventeranno raggiungibili quando entrera' l'eroe che le
		// usa, ed e' la ragione per cui sono dichiarate invece che corrette. Non sono difetti (E6).
		{ TEXT("Action.CircularAoE"),     TEXT("Aspetta il suo eroe") },
		{ TEXT("Action.HeavyAttack"),     TEXT("Pezzo non assegnato: Gadget.BreachCharge esiste, default di nessuno") },
		{ TEXT("Action.Leap"),            TEXT("Aspetta il suo eroe") },
		{ TEXT("Action.LineAttack"),      TEXT("Aspetta il suo eroe") },
		{ TEXT("Action.MarkTarget"),      TEXT("Aspetta il suo eroe") },
		{ TEXT("Action.PrecisionAttack"), TEXT("Aspetta il suo eroe") },
		{ TEXT("Action.Pull"),            TEXT("Aspetta il suo eroe") },
		{ TEXT("Action.Push"),            TEXT("Aspetta il suo eroe") },
		{ TEXT("Action.Reposition"),      TEXT("Aspetta il suo eroe") },
		{ TEXT("Action.Root"),            TEXT("Aspetta il suo eroe") },
		// `Action.Shield` e' USCITA da questo elenco: la portano `Hero.Muiren.TideGuard` e
		// `Hero.Ivrin.PhaseGuard`, uno per squadra. Ci si era provato il 2026-08-28 con [D-224] e il
		// tentativo era stato RITIRATO — un sesto slot d'eroe portava il kit a 11 voci contro i 10 tasti
		// numerici, e l'azione sarebbe stata raggiungibile per QUESTO gate e impremibile per il giocatore.
		// Il vincolo e' caduto spostando le generiche su tasti propri (`GenericHotkeys()`), non ignorandolo.
		{ TEXT("Action.Slow"),            TEXT("Aspetta il suo eroe") },
		{ TEXT("Action.SuppressiveLine"), TEXT("Aspetta il suo eroe") },
	};

	// Anti-vacuita', e non e' teorica: se il roster tornasse vuoto ogni azione risulterebbe non
	// raggiungibile, e con un elenco lungo abbastanza il test resterebbe VERDE raccontando che va tutto
	// bene. Queste tre righe fanno cadere quel caso.
	TestTrue(TEXT("almeno tredici azioni sono raggiungibili"), Raggiungibili.Num() >= 13);
	TestTrue(TEXT("Guard e' raggiungibile: e' generica"),
		Raggiungibili.Contains(FName(TEXT("Action.Guard"))));
	TestTrue(TEXT("Charge e' raggiungibile: la porta Hero.Branth.Ram"),
		Raggiungibili.Contains(FName(TEXT("Action.Charge"))));

	// Il catalogo si costruisce UNA volta: `GetCoreActionCatalog()` istanzia un `FRTActionDef` per voce, per
	// valore (quante siano lo dice `grep -c "Catalog.Add(" RTCatalogLibrary.cpp`, e cambia da solo),
	// ognuno coi suoi `TArray` annidati, a ogni chiamata — e `FindCoreAction` non fa che scorrerlo. Il
	// verso 3 qui sotto lo interrogava una volta per riga dichiarata: ventidue ricostruzioni per rispondere
	// a domande che questo ciclo ha gia' in mano.
	const TArray<FRTActionDef> Catalogo = URTCatalogLibrary::GetCoreActionCatalog();
	TSet<FName> Esistenti;
	Esistenti.Reserve(Catalogo.Num());

	for (const FRTActionDef& Def : Catalogo)
	{
		Esistenti.Add(Def.ActionId);
		const bool bRaggiungibile = Raggiungibili.Contains(Def.ActionId);
		const FString* Ragione = Dichiarate.Find(Def.ActionId);

		// Verso 1 — un'azione nuova senza portatore non passa in silenzio.
		if (!bRaggiungibile && !Ragione)
		{
			AddError(FString::Printf(
				TEXT("%s non e' raggiungibile da nessun kit e non e' dichiarata: assegnala a un eroe, ")
				TEXT("oppure aggiungila all'elenco di questo test con la ragione per cui non lo e'."),
				*Def.ActionId.ToString()));
		}
		// Verso 2 — l'elenco non si fossilizza: chi assegna un'azione toglie la sua riga.
		if (bRaggiungibile && Ragione)
		{
			AddError(FString::Printf(
				TEXT("%s ORA e' raggiungibile ma e' ancora dichiarata come «%s»: togli la riga. ")
				TEXT("⛔ Se quella ragione e' una DECISIONE e non un portatore che manca, e' la ")
				TEXT("RAGGIUNGIBILITA' il difetto: si toglie quella, e la riga resta."),
				*Def.ActionId.ToString(), **Ragione));
		}
	}

	// Verso 3 — una voce che non corrisponde a nessuna azione del catalogo e' un residuo.
	for (const TPair<FName, FString>& Voce : Dichiarate)
	{
		TestTrue(*FString::Printf(TEXT("%s dichiarata esiste ancora nel catalogo"), *Voce.Key.ToString()),
			Esistenti.Contains(Voce.Key));
	}

	// Sul `Num()` e non su un contatore incrementato nel ciclo: quello poteva solo ripetere la stessa
	// cosa, e si sarebbe scollato dal suo soggetto al primo `continue` che qualcuno aggiunge sopra.
	TestTrue(TEXT("il catalogo core non e' vuoto"), Catalogo.Num() > 30);
	return true;
}

/**
 * **Ogni azione generica entra nel kit con un nome.** Il gemello di
 * `RefactorTactics.Heroes.EveryActionHasADisplayName` per le cinque che `MakeGenericActions` accoda.
 *
 * 🔴 **Il difetto che copre e' stato reale fino al 2026-08-26**: le generiche entravano nel kit con
 * `DisplayName` vuoto, e nessuno se ne accorgeva perche' **nessun tasto le raggiungeva**. Dato loro un tasto
 * (#1439), la prima pressione avrebbe stampato `[RT] RTUnit_0: abilita' attiva -> ` — la stessa forma di
 * #892, che aveva coperto le venti abilita' d'eroe e non queste, perche' allora non si vedevano.
 *
 * ⚠️ Si controlla cio' che il KIT riceve, non la tabella dei nomi: il difetto stava nel percorso —
 * `MakeGenericActions` copiava `Def`, portata, `bSelfTarget` e `Power`, e il nome no — non nei dati.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGenericActionDisplayNameTest,
	"RefactorTactics.Actions.EveryGenericHasADisplayName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGenericActionDisplayNameTest::RunTest(const FString&)
{
	const TArray<URTActionData*> Generiche = URTCatalogLibrary::MakeGenericActions(GetTransientPackage());

	// Anti-vacuita': se `MakeGenericActions` tornasse vuota il ciclo non asserirebbe nulla, e il test
	// resterebbe verde raccontando che ogni nome c'e'.
	if (!TestEqual(TEXT("le generiche accodate sono quante il catalogo ne dichiara"),
		Generiche.Num(), URTCatalogLibrary::GetGenericActionIds().Num()))
	{
		return false;
	}
	TestTrue(TEXT("e ce n'e' almeno una"), Generiche.Num() > 0);

	for (const URTActionData* Azione : Generiche)
	{
		if (!TestNotNull(TEXT("l'istanza esiste"), Azione)) { continue; }
		TestTrue(*FString::Printf(TEXT("`%s` ha un nome leggibile"), *Azione->Def.ActionId.ToString()),
			!Azione->DisplayName.IsEmpty());
	}

	return true;
}

/**
 * **La cascata del nome di un'azione equipaggiata, provata senza costruire oggetti** (`#3275`).
 *
 * 🔑 **E' la meta' che sa fallire in silenzio.** Il gemello qui sotto guarda il catalogo spedito, dove
 * il nome c'e' per tutti: se la cascata ricadesse SEMPRE sul ripiego, quel gemello resterebbe verde lo
 * stesso, perche' anche il ripiego produce una stringa non vuota. Qui si prova QUALE ramo risponde.
 *
 * ⚠️ Il caso «soli spazi» non e' teorico: una `FText` costruita da `"   "` non e' `IsEmpty()`, e senza
 * `IsEmptyOrWhitespace` il dock comporrebbe `"6.    "` — a schermo indistinguibile dal difetto originale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEquipmentActionDisplayNameTest,
	"RefactorTactics.Catalog.EquipmentActionDisplayName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEquipmentActionDisplayNameTest::RunTest(const FString&)
{
	// 1. Il nome dichiarato vince, ed e' il caso di ogni pezzo che il catalogo spedisce.
	TestEqual(TEXT("il nome dichiarato passa intatto"),
		URTCatalogLibrary::EquipmentActionDisplayName(
			FText::FromString(TEXT("Sprinkler")), TEXT("Gadget.Sprinkler")).ToString(),
		FString(TEXT("Sprinkler")));

	// 2. Vuoto, con un id: l'ultimo segmento. Una parola invece di un buco.
	TestEqual(TEXT("vuoto ricade sull'ultimo segmento dell'id"),
		URTCatalogLibrary::EquipmentActionDisplayName(
			FText::GetEmpty(), TEXT("Gadget.Sprinkler")).ToString(),
		FString(TEXT("Sprinkler")));

	// 3. Soli spazi: vale come non dichiarato. `IsEmpty()` direbbe di no, e il dock mostrerebbe spazi.
	TestEqual(TEXT("soli spazi valgono come non dichiarato"),
		URTCatalogLibrary::EquipmentActionDisplayName(
			FText::FromString(TEXT("   ")), TEXT("Reaction.Cleanse")).ToString(),
		FString(TEXT("Cleanse")));

	// 4. Un id senza punto resta intero: la cascata non produce vuoto per una convenzione cambiata.
	TestEqual(TEXT("un id senza punto resta intero"),
		URTCatalogLibrary::EquipmentActionDisplayName(
			FText::GetEmpty(), TEXT("Sprinkler")).ToString(),
		FString(TEXT("Sprinkler")));

	// 5. Niente nome e niente id: vuoto, perche' non c'e' niente da cui ricavare una parola. E' l'unico
	// caso in cui questa funzione torna vuoto, ed e' irraggiungibile dal catalogo: `MakeEquipmentAction`
	// non costruisce nulla per un pezzo senza `GrantedActionId`, e un pezzo senza `EquipmentId` non esiste.
	TestTrue(TEXT("senza nome e senza id resta vuoto"),
		URTCatalogLibrary::EquipmentActionDisplayName(FText::GetEmpty(), NAME_None).IsEmpty());

	return true;
}

/**
 * **Ogni azione concessa da un pezzo arriva nel kit col nome del PEZZO** (`#3275`). Il terzo della
 * famiglia, dopo `RefactorTactics.Heroes.EveryActionHasADisplayName` e
 * `RefactorTactics.Actions.EveryGenericHasADisplayName`.
 *
 * 🔴 **Il difetto che copre era reale, e in ogni partita di default.** `MakeEquipmentAction` non
 * scriveva mai `DisplayName`: l'azione entrava in `Abilities` con una `FText` costruita per default, e
 * `ARTHUD::ComposeAbilityLine` componeva **`"6. "`** — tasto, punto, ricarica, e nient'altro. Lo vedeva
 * meta' roster, perche' `DefaultLoadoutFor` da' il loadout a Muiren e Branth; e fra le voci mute c'era
 * `Reaction.Cleanse`, che [D-218] mette li' come unica risposta allo `Status.Slow` dell'attacco di Branth.
 *
 * ⛔ **I due gate esistenti non potevano vederlo, per costruzione.** `Heroes.EveryActionHasADisplayName`
 * itera `GetHeroRoster()` → `Hero->Actions`; le azioni equipaggiate non stanno in quell'insieme — entrano
 * in `Abilities` a runtime, con `EquipLoadout`. L'asserzione era vera e non copriva il caso: la forma di
 * falso verde in cui il gate guarda una popolazione diversa da quella del difetto.
 *
 * 🔑 **Si asserisce l'UGUAGLIANZA col nome del pezzo, non che il nome sia non-vuoto.** Un `TestFalse`
 * su `IsEmpty()` resterebbe verde anche se `Item->DisplayName` smettesse di essere letto: il ripiego
 * della cascata ricava `Sprinkler` da `Gadget.Sprinkler` e passerebbe. L'uguaglianza distingue «legge il
 * catalogo» da «ricava dall'id», che e' esattamente la differenza da pinnare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEveryEquipmentActionHasADisplayNameTest,
	"RefactorTactics.Catalog.EveryEquipmentActionHasADisplayName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEveryEquipmentActionHasADisplayNameTest::RunTest(const FString&)
{
	TArray<URTEquipmentData*> Pezzi;
	Pezzi.Append(URTCatalogLibrary::MakeWeaponVariants());
	Pezzi.Append(URTCatalogLibrary::MakeGadgets());
	Pezzi.Append(URTCatalogLibrary::MakeReactionModules());

	// Anti-vacuita' primaria: coi tre cataloghi vuoti nessun ciclo qui sotto asserirebbe niente, e il test
	// resterebbe verde raccontando che ogni nome c'e'. E' la forma contro cui i due gemelli si difendono.
	if (!TestTrue(TEXT("i tre cataloghi dichiarano dei pezzi"), Pezzi.Num() > 0))
	{
		return false;
	}

	int32 Dichiaranti = 0;
	int32 Prodotte = 0;
	for (const URTEquipmentData* Pezzo : Pezzi)
	{
		if (!TestNotNull(TEXT("il pezzo esiste"), Pezzo)) { continue; }

		// ⚠️ Le varianti d'arma non concedono un'azione: MODIFICANO l'attacco base, via `ApplyWeaponVariant`.
		// Non e' un caso saltato in silenzio — in fondo si asserisce QUANTE sono, perche' se un giorno una
		// variante cominciasse a concedere, questo `continue` la escluderebbe dal gate senza dirlo.
		if (Pezzo->GrantedActionId.IsNone())
		{
			continue;
		}
		++Dichiaranti;

		const URTActionData* Azione =
			URTCatalogLibrary::MakeEquipmentAction(Pezzo, GetTransientPackage());
		if (!TestNotNull(
			*FString::Printf(TEXT("`%s` produce un'azione"), *Pezzo->EquipmentId.ToString()), Azione))
		{
			continue;
		}
		++Prodotte;

		TestEqual(
			*FString::Printf(TEXT("`%s` arriva nel kit col nome del pezzo"), *Pezzo->EquipmentId.ToString()),
			Azione->DisplayName.ToString(), Pezzo->DisplayName.ToString());

		// E quel nome e' davvero leggibile: senza questo, un catalogo che dichiarasse `""` per tutti
		// renderebbe l'uguaglianza qui sopra vera e vuota.
		TestFalse(
			*FString::Printf(TEXT("`%s` non arriva muto"), *Pezzo->EquipmentId.ToString()),
			Azione->DisplayName.IsEmptyOrWhitespace());
	}

	// Anti-vacuita' secondaria: il ciclo ha esaminato dei soggetti, e ogni pezzo che dichiara un'azione ne
	// ha prodotta una — un core mancante la farebbe sparire dal gate invece che farlo fallire.
	TestTrue(TEXT("almeno un pezzo concede un'azione"), Dichiaranti > 0);
	TestEqual(TEXT("ogni pezzo che dichiara un'azione la produce"), Prodotte, Dichiaranti);

	// E i saltati sono ESATTAMENTE le varianti d'arma, non un insieme che cresce in silenzio.
	TestEqual(TEXT("i pezzi che non concedono sono le varianti d'arma"),
		Pezzi.Num() - Dichiaranti, URTCatalogLibrary::MakeWeaponVariants().Num());

	return true;
}

/**
 * L'insieme ESATTO delle azioni core che si dichiarano aggressione ([`INT-8`], [D-221]).
 *
 * 🔴 **Si pinna l'INSIEME e non il conteggio**, e la differenza e' il difetto che questo test nasce per
 * prendere: una riga `Catalog.Last().bCountsAsAttack = true` finita dopo il blocco SBAGLIATO dichiara
 * l'azione precedente e non quella voluta -- `Catalog.Last()` punta all'ultima aggiunta e non protesta.
 * Un conteggio sopravvive a uno scambio; un insieme no.
 *
 * ⚠️ **Ed e' esattamente cosi' che e' andata**: `Action.Brace` (self-target di Prep) e `Action.ModifyArc`
 * (intercettata per nome prima della raccolta) sono state dichiarate aggressioni per errore, e **1233 test
 * verdi non se ne sono accorti** -- perche' nessuna delle due raggiunge `CollectHexAttacks`, quindi la
 * dichiarazione sbagliata era INERTE. Sbagliata e invisibile: la combinazione peggiore, e l'unica che un
 * gate sull'insieme prende.
 *
 * ⚠️ **Chi NON e' nell'elenco e potrebbe sorprendere**: `Action.Counter`, `Action.Deflect` (reazioni: il
 * loro danno passa da `URTActionEffectLibrary::ProduceEvents`, mai dai colpi) e `Action.Electrify`
 * (`Environment`, risolve nel Cleanup). Sono aggressioni nel senso comune, ma non producono colpi -- e il
 * campo dichiara quello. Il giorno in cui una di esse venisse instradata nel Blast, il fail-closed la
 * rende muta e il primo test che la esercita diventa rosso: e' li' che si dichiarera'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoreActionsDeclareAggressionTest,
	"RefactorTactics.Actions.OnlyAggressionsDeclareThemselves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoreActionsDeclareAggressionTest::RunTest(const FString&)
{
	static const TSet<FName> Attese = {
		TEXT("Action.BasicAttack"), TEXT("Action.PrecisionAttack"), TEXT("Action.HeavyAttack"),
		TEXT("Action.LineAttack"),  TEXT("Action.CircularAoE"),     TEXT("Action.SuppressiveLine"),
		TEXT("Action.Mortar"), // `#2890`: colpisce senza vedere, ma COLPISCE — l'aggressione la dichiara
		TEXT("Action.Charge"),      TEXT("Action.MarkTarget"),
		TEXT("Action.Push"),        TEXT("Action.Pull"),            TEXT("Action.Root"),
		TEXT("Action.Slow"),        TEXT("Action.Interrupt") };

	TSet<FName> Dichiarate;
	int32 Totali = 0;
	for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
	{
		++Totali;
		if (Def.bCountsAsAttack) { Dichiarate.Add(Def.ActionId); }
	}

	// ANTI-VACUITA': un catalogo vuoto farebbe passare entrambi i cicli senza asserire niente.
	if (!TestTrue(TEXT("il catalogo core non e' vuoto"), Totali > 20)) { return false; }

	for (const FName& Id : Attese)
	{
		TestTrue(FString::Printf(TEXT("%s deve dichiararsi aggressione"), *Id.ToString()),
			Dichiarate.Contains(Id));
	}
	for (const FName& Id : Dichiarate)
	{
		TestTrue(FString::Printf(TEXT("%s NON deve dichiararsi aggressione"), *Id.ToString()),
			Attese.Contains(Id));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
