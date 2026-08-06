#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
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
