#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTEquipmentData.h"
#include "Ability/RTActionData.h"
#include "Core/RTGameplayTags.h"
#include "Unit/RTUnit.h"
#include "Turn/RTTurnRules.h"
#include "UObject/UnrealType.h"

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

	NoDrawback->Drawback = FText::FromString(TEXT("cooldown +1"));
	TestEqual(TEXT("con lo svantaggio dichiarato: accettato"),
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogShippedTest,
	"RefactorTactics.Catalog.ValidatorAcceptsShippedCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogShippedTest::RunTest(const FString&)
{
	// Il catalogo REALE del gioco: le azioni che ARTUnit crea per i due archetipi. Se una di esse perde
	// l'ActionId, o un'azione di movimento cambia fallback, questo test diventa rosso — il documento e il
	// codice non possono divergere in silenzio.
	TArray<FRTActionDef> Shipped = URTCatalogLibrary::GetShippedActionCatalog();
	TestTrue(TEXT("il catalogo spedito non e' vuoto"), Shipped.Num() > 0);

	const TArray<FString> Errors = URTCatalogLibrary::ValidateActions(Shipped);
	for (const FString& E : Errors) { AddError(E); }
	TestEqual(TEXT("il catalogo spedito e' valido"), Errors.Num(), 0);

	// Ogni azione del catalogo spedito deve avere una macro-fase sensata per il suo tipo.
	for (const FRTActionDef& Def : Shipped)
	{
		const ERTMatchPhase Macro = URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase);
		TestTrue(FString::Printf(TEXT("%s risolve in una fase giocabile"), *Def.ActionId.ToString()),
			Macro == ERTMatchPhase::Prep || Macro == ERTMatchPhase::Dash
			|| Macro == ERTMatchPhase::Blast || Macro == ERTMatchPhase::Move);
	}
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

	// `Action.Sprint` come lo descrive il catalogo v0.1 §2: mobilita' rapida, 8 MP, entrambi gli slot.
	const FRTActionDef Sprint = URTCatalogLibrary::FindCoreAction(TEXT("Action.Sprint"));
	TestTrue(TEXT("Action.Sprint e' nel catalogo"), Sprint.ActionId == FName(TEXT("Action.Sprint")));
	TestTrue(TEXT("Sprint risolve nella fase Dash (mobilita' rapida)"),
		URTCatalogLibrary::MapResolutionPhase(Sprint.ResolutionPhase) == ERTMatchPhase::Dash);
	TestEqual(TEXT("Sprint vale 8 punti movimento"), Sprint.RangeCells, 8);
	TestTrue(TEXT("Sprint consuma movimento E azione principale"), Sprint.Slot == ERTActionSlot::MovementAndMain);
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
	// definizione e abilita' campo per campo, sui due archetipi.
	const ERTArchetype Archetypes[] = { ERTArchetype::Ranger, ERTArchetype::Guardian };
	for (const ERTArchetype Archetype : Archetypes)
	{
		ARTUnit* Unit = NewObject<ARTUnit>();
		if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
		Unit->ConfigureAsArchetype(Archetype);

		TestTrue(TEXT("l'archetipo ha abilita'"), Unit->NumAbilities() > 0);
		for (int32 i = 0; i < Unit->NumAbilities(); ++i)
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
			if (Ability->bDash)
			{
				TestEqual(FString::Printf(TEXT("%s e' uno scatto -> fase Dash"), *Name), Macro, ERTMatchPhase::Dash);
			}
			else if (Ability->bSelfTarget)
			{
				TestEqual(FString::Printf(TEXT("%s e' un supporto -> fase Prep"), *Name), Macro, ERTMatchPhase::Prep);
			}
			else
			{
				TestEqual(FString::Printf(TEXT("%s e' un attacco -> fase Blast"), *Name), Macro, ERTMatchPhase::Blast);
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

#endif // WITH_DEV_AUTOMATION_TESTS
