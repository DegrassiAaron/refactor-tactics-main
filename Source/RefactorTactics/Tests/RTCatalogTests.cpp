#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTEquipmentData.h"
#include "Ability/RTActionData.h"
#include "Core/RTGameplayTags.h"
#include "Unit/RTUnit.h"
#include "Turn/RTTurnRules.h"
#include "UObject/UnrealType.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"

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
		Def.bCanBeInterrupted = true;
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

	// `Action.Sprint` come lo descrive il catalogo v0.1 §2: mobilita' rapida, 8 MP, slot movimento.
	const FRTActionDef Sprint = URTCatalogLibrary::FindCoreAction(TEXT("Action.Sprint"));
	TestTrue(TEXT("Action.Sprint e' nel catalogo"), Sprint.ActionId == FName(TEXT("Action.Sprint")));
	TestTrue(TEXT("Sprint risolve nella fase Dash (mobilita' rapida)"),
		URTCatalogLibrary::MapResolutionPhase(Sprint.ResolutionPhase) == ERTMatchPhase::Dash);
	TestEqual(TEXT("Sprint vale 8 punti movimento"), Sprint.RangeCells, 8);
	// D-028: il solo slot movimento. Prima era `MovementAndMain`, e il costo dello scatto lungo era
	// strutturale; ora il prezzo e' tutto nei dati (`Exposed` e nessuna reazione) — vedi `BAL-1`.
	TestTrue(TEXT("Sprint consuma il solo slot movimento"), Sprint.Slot == ERTActionSlot::Movement);
	TestEqual(TEXT("Sprint dichiara un solo effetto: lo stato"), Sprint.Effects.Num(), 1);
	TestTrue(TEXT("e quell'effetto e' Status.Exposed per un turno"),
		Sprint.Effects.Num() == 1 && Sprint.Effects[0].Effect == ERTActionEffect::Status
		&& Sprint.Effects[0].StatusTag == TAG_Status_Exposed && Sprint.Effects[0].StatusDuration == 1);

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
			// ne ha, e senza questa esclusione il test chiederebbe la fase Blast a `Riktor.Interposition`.
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
					//   - si PREPARA senza essere supporto su se' — `Riktor.Reconfigure`, `Phase.FlowReaction`,
					//     `Wraith.InterceptShot` (fase Prep);
					//   - agisce sull'AMBIENTE — `Gadget.ConductiveNode`, `Phase.FluidTrail`, `Phase.MistVeil`,
					//     `Riktor.KineticPanel`, che ereditano la fase dalle azioni core d'ambiente e
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogNoFloatTest,
	"RefactorTactics.Catalog.NoFloatInIntegerFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogNoFloatTest::RunTest(const FString&)
{
	// Invariante #4 verificato per REFLECTION, non a occhio: se qualcuno aggiungesse un float a
	// FRTActionDef (un moltiplicatore di danno, un costo frazionario) il test lo scopre subito.
	const UScriptStruct* Struct = FRTActionDef::StaticStruct();
	if (!TestNotNull(TEXT("FRTActionDef e' una USTRUCT riflessa"), Struct)) { return false; }

	int32 FloatFields = 0;
	int32 Inspected = 0;
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		++Inspected;
		if (It->IsA<FFloatProperty>() || It->IsA<FDoubleProperty>())
		{
			++FloatFields;
			AddError(FString::Printf(TEXT("campo in virgola mobile in FRTActionDef: %s"), *It->GetName()));
		}
	}
	TestTrue(TEXT("la struct ha campi riflessi da ispezionare"), Inspected > 0);
	TestEqual(TEXT("nessun float/double fra costo, priorita', range, cooldown"), FloatFields, 0);
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

	// Le eccezioni, ognuna con la sua ragione. Le categorie invecchiano in modo diverso: `Motore` cade se
	// il gameplay smette di produrre quell'azione, `Modulo` quando l'equipaggiamento diventa consegnabile,
	// `NonAssegnata` quando un eroe la porta.
	const TMap<FName, FString> Dichiarate = {
		// Scritte dal motore: il gameplay le produce senza passare da un kit.
		{ TEXT("Action.Move"),            TEXT("Motore: MakePlanFor la aggiunge quando l'unita' si muove") },
		{ TEXT("Action.Cleanse"),         TEXT("Motore: il Blast la CERCA fra le abilita'; produttore assente, #1389") },
		{ TEXT("Action.Heal"),            TEXT("Motore: RTTurnManager la scrive come voce di cura") },
		{ TEXT("Action.Interrupt"),       TEXT("Motore: raccolta e applicata dal TurnManager") },
		{ TEXT("Action.ModifyArc"),       TEXT("Motore: scritta dal TurnManager") },
		// Concesse da un pezzo che ESISTE ma che nessun eroe porta di default: il canale funziona, manca
		// l'assegnazione. Escono da qui il giorno in cui un eroe riceve quel pezzo nel suo loadout.
		{ TEXT("Action.Anchor"),          TEXT("Pezzo non assegnato: base di Reaction.Anchor, default di nessuno") },
		// `Action.Purge` e' USCITA da questo elenco il 2026-08-27 ([D-218], `#1403`): `Reaction.Cleanse` e'
		// il modulo di default di Riktor, quindi la base e' raggiungibile. La riga la toglie il gate stesso,
		// che dice «ORA e' raggiungibile: togli la riga» invece di lasciarla marcire fra le esclusioni.
		// Bloccata da una migrazione decisa e non fatta.
		{ TEXT("Action.Sprint"),          TEXT("E38: forma canonica profilo Move (D-015/D-116), il codice ha FastMovement") },
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
		{ TEXT("Action.Shield"),          TEXT("Aspetta il suo eroe: adr-0003 la da' per arrivata") },
		{ TEXT("Action.Slow"),            TEXT("Aspetta il suo eroe") },
		{ TEXT("Action.SuppressiveLine"), TEXT("Aspetta il suo eroe") },
	};

	// Anti-vacuita', e non e' teorica: se il roster tornasse vuoto ogni azione risulterebbe non
	// raggiungibile, e con un elenco lungo abbastanza il test resterebbe VERDE raccontando che va tutto
	// bene. Queste tre righe fanno cadere quel caso.
	TestTrue(TEXT("almeno tredici azioni sono raggiungibili"), Raggiungibili.Num() >= 13);
	TestTrue(TEXT("Guard e' raggiungibile: e' generica"),
		Raggiungibili.Contains(FName(TEXT("Action.Guard"))));
	TestTrue(TEXT("Charge e' raggiungibile: la porta Hero.Riktor.Ram"),
		Raggiungibili.Contains(FName(TEXT("Action.Charge"))));

	// Il catalogo si costruisce UNA volta: `GetCoreActionCatalog()` istanzia 37 `FRTActionDef` per valore,
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
				TEXT("%s ORA e' raggiungibile ma e' ancora dichiarata come «%s»: togli la riga."),
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
