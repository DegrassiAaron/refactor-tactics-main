// Il contesto del puntatore come funzione pura sui fatti (#4065, S2).
//
// Fino a #4065 l'ordine dei contesti viveva dentro `ARTPlayerController::GetPointerContext`, insieme alla lettura del
// mondo, e per provarlo serviva un controller, un TurnManager e un'unita'. Ora il controller raccoglie i fatti e
// `URTPointerLibrary::DeriveContext` decide: qui lo si prova senza mondo, su tutti i fatti.
//
// ⚠️ **Cosa NON copre.** Che il controller raccolga i fatti giusti dal mondo lo provano i test che passano dal
// controller (`PlayerInput.*`, `Reactions.ASuspendedResolutionIsReadOnlyForInput`, `Frontend.*` per la pausa).

#include "Misc/AutomationTest.h"
#include "Player/RTPointerInteraction.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FString PointerContextName(ERTPointerContext Context)
	{
		return StaticEnum<ERTPointerContext>()->GetNameStringByValue(static_cast<int64>(Context));
	}

	FRTPointerFacts PointerFacts(bool bModal, bool bResolution, bool bSelection, bool bFacing, bool bArmed, int32 Waypoints)
	{
		FRTPointerFacts Facts;
		Facts.bModalOpen = bModal;
		Facts.bResolutionActive = bResolution;
		Facts.bHasSelection = bSelection;
		Facts.bDeclaringFacing = bFacing;
		Facts.bArmedActionWantsTarget = bArmed;
		Facts.WaypointCount = Waypoints;
		return Facts;
	}

	/** Ogni combinazione dei fatti: cinque booleani e due quantita' di waypoint (nessuno, alcuni). */
	TArray<FRTPointerFacts> PointerEveryFacts()
	{
		TArray<FRTPointerFacts> Tutti;
		for (int32 Bits = 0; Bits < 32; ++Bits)
		{
			for (const int32 Waypoints : { 0, 2 })
			{
				Tutti.Add(PointerFacts((Bits & 1) != 0, (Bits & 2) != 0, (Bits & 4) != 0, (Bits & 8) != 0, (Bits & 16) != 0, Waypoints));
			}
		}
		return Tutti;
	}

	FString PointerDescribe(const FRTPointerFacts& F)
	{
		return FString::Printf(TEXT("modale=%d risoluzione=%d selezione=%d verso=%d armata=%d waypoint=%d"),
			F.bModalOpen, F.bResolutionActive, F.bHasSelection, F.bDeclaringFacing, F.bArmedActionWantsTarget, F.WaypointCount);
	}
}

/**
 * **Un caso per contesto, e i casi in cui due fatti si contendono il puntatore.**
 *
 * Le righe di contesa sono quelle che un riordino rompe: l'azione armata vince sui waypoint, il verso vince
 * sull'azione armata, la risoluzione vince su tutto il piano, la pausa vince sulla risoluzione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerContextOrderTest,
	"RefactorTactics.Pointer.ContextFollowsTheContractOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerContextOrderTest::RunTest(const FString&)
{
	struct FRiga { const TCHAR* Nome; FRTPointerFacts Fatti; ERTPointerContext Atteso; };
	const FRiga Righe[] = {
		{ TEXT("nessun fatto"),                          PointerFacts(false, false, false, false, false, 0), ERTPointerContext::IdleSelection },
		{ TEXT("selezione, niente armato"),              PointerFacts(false, false, true,  false, false, 0), ERTPointerContext::Planning },
		{ TEXT("selezione e waypoint"),                  PointerFacts(false, false, true,  false, false, 2), ERTPointerContext::Pathing },
		{ TEXT("selezione e azione armata"),             PointerFacts(false, false, true,  false, true,  0), ERTPointerContext::Targeting },
		{ TEXT("azione armata e waypoint"),              PointerFacts(false, false, true,  false, true,  2), ERTPointerContext::Targeting },
		{ TEXT("selettore del verso aperto"),            PointerFacts(false, false, true,  true,  false, 0), ERTPointerContext::Facing },
		{ TEXT("verso e azione armata"),                 PointerFacts(false, false, true,  true,  true,  2), ERTPointerContext::Facing },
		{ TEXT("verso senza selezione"),                 PointerFacts(false, false, false, true,  true,  2), ERTPointerContext::IdleSelection },
		{ TEXT("risoluzione su un piano in corso"),      PointerFacts(false, true,  true,  true,  true,  2), ERTPointerContext::ResolutionPlayback },
		{ TEXT("risoluzione senza selezione"),           PointerFacts(false, true,  false, false, false, 0), ERTPointerContext::ResolutionPlayback },
		{ TEXT("pausa durante la risoluzione"),          PointerFacts(true,  true,  true,  true,  true,  2), ERTPointerContext::Modal },
		{ TEXT("pausa senza selezione"),                 PointerFacts(true,  false, false, false, false, 0), ERTPointerContext::Modal },
	};
	for (const FRiga& Riga : Righe)
	{
		const ERTPointerContext Esito = URTPointerLibrary::DeriveContext(Riga.Fatti);
		TestEqual(FString::Printf(TEXT("%s: %s (atteso %s)"), Riga.Nome, *PointerContextName(Esito), *PointerContextName(Riga.Atteso)),
			Esito, Riga.Atteso);
	}
	return true;
}

/**
 * **La precedenza vale per OGNI combinazione dei fatti**, non solo per quelle scritte sopra: la pausa domina tutto,
 * la risoluzione domina il piano, senza selezione nient'altro conta, e `ReactionWindow` non esce mai (E14).
 *
 * Le proprieta' sono scritte una per volta, non come una seconda copia della funzione: una copia passerebbe con lo
 * stesso errore dell'originale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerContextPrecedenceTest,
	"RefactorTactics.Pointer.ContextPrecedenceHoldsForEveryFact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerContextPrecedenceTest::RunTest(const FString&)
{
	const TArray<FRTPointerFacts> Tutti = PointerEveryFacts();
	TestEqual(TEXT("premessa: tutte le combinazioni"), Tutti.Num(), 64);
	for (const FRTPointerFacts& F : Tutti)
	{
		const ERTPointerContext C = URTPointerLibrary::DeriveContext(F);
		const FString Chi = PointerDescribe(F);
		TestTrue(FString::Printf(TEXT("%s: mai ReactionWindow"), *Chi), C != ERTPointerContext::ReactionWindow);
		TestEqual(FString::Printf(TEXT("%s: Modal se e solo se la pausa e' aperta"), *Chi), C == ERTPointerContext::Modal, F.bModalOpen);
		if (!F.bModalOpen)
		{
			TestEqual(FString::Printf(TEXT("%s: senza pausa, ResolutionPlayback se e solo se la risoluzione e' attiva"), *Chi),
				C == ERTPointerContext::ResolutionPlayback, F.bResolutionActive);
		}
		if (!F.bModalOpen && !F.bResolutionActive)
		{
			TestEqual(FString::Printf(TEXT("%s: a mondo scrivibile, IdleSelection se e solo se non c'e' selezione"), *Chi),
				C == ERTPointerContext::IdleSelection, !F.bHasSelection);
			if (F.bHasSelection)
			{
				TestEqual(FString::Printf(TEXT("%s: con la selezione, Facing se e solo se il selettore e' aperto"), *Chi),
					C == ERTPointerContext::Facing, F.bDeclaringFacing);
				TestEqual(FString::Printf(TEXT("%s: Pathing solo con waypoint, niente armato e verso chiuso"), *Chi),
					C == ERTPointerContext::Pathing, F.WaypointCount > 0 && !F.bArmedActionWantsTarget && !F.bDeclaringFacing);
			}
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
