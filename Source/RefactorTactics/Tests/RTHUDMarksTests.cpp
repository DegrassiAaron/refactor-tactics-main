// Marcatori sopra la testa: chi verrebbe colpito dai piani delle PROPRIE unita'.
//
// Il difetto che questi test chiudono e' stato trovato in PIE: l'avviso di fuoco amico spariva appena si
// selezionava un'altra unita' — per esempio per muoverla, cioe' proprio mentre si finisce il turno. Leggeva
// l'anteprima, che appartiene all'unita' SELEZIONATA, invece dei piani.
//
// `ComputePlannedHitMarks` non ha accesso alla selezione: l'indipendenza e' una proprieta' della firma, non
// una disciplina da ricordare. Questi test verificano il resto — che legga i piani giusti e ignori quelli
// che non deve leggere.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "Unit/RTUnit.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	UWorld* MakeMarksWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyMarksWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTUnit* SpawnMarksUnit(UWorld* World, FName HeroId, int32 TeamId, const FRTCellId& Cell)
	{
		const URTHeroData* Hero = nullptr;
		for (const URTHeroData* H : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (H && H->HeroId == HeroId) { Hero = H; break; }
		}
		if (!World || !Hero) { return nullptr; }

		ARTUnit* Unit = World->SpawnActor<ARTUnit>();
		if (!Unit) { return nullptr; }
		Unit->ConfigureFromHeroData(Hero);
		Unit->TeamId = TeamId;
		Unit->Cell = Cell;
		return Unit;
	}

	/** Indice dell'abilita' con quell'ActionId nel kit dell'unita', o INDEX_NONE. */
	int32 MarksAbilityIndex(const ARTUnit* Unit, const TCHAR* ActionId)
	{
		for (int32 I = 0; Unit && I < Unit->NumAbilities(); ++I)
		{
			const URTActionData* A = Unit->GetAbility(I);
			if (A && A->Def.ActionId == FName(ActionId)) { return I; }
		}
		return INDEX_NONE;
	}
}

/**
 * L'alleato dentro l'area viene marcato, e il marchio nasce dal PIANO.
 *
 * Nessun parametro di questa funzione dice chi e' selezionato: e' il punto. Prima la stessa informazione
 * passava per `IsPreviewAllyHitCell`, cioe' per lo stato dell'anteprima, e cambiare selezione la spegneva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHUDAllyMarkFromPlanTest,
	"RefactorTactics.HUD.AllyInBlastIsMarkedFromThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHUDAllyMarkFromPlanTest::RunTest(const FString&)
{
	UWorld* World = MakeMarksWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	// Stessa geometria di `Combat.FriendlyFire`: Phase adiacente al bersaglio, dentro l'area r1.
	ARTUnit* Gadget    = SpawnMarksUnit(World, TEXT("Hero.Gadget"),    0, FRTCellId(-1, 0, 0));
	ARTUnit* Phase    = SpawnMarksUnit(World, TEXT("Hero.Phase"),    0, FRTCellId( 1, 0, 0));
	ARTUnit* Riktor = SpawnMarksUnit(World, TEXT("Hero.Riktor"), 1, FRTCellId( 2, 0, 0));
	if (!TestNotNull(TEXT("Gadget"), Gadget) || !TestNotNull(TEXT("Phase"), Phase) || !TestNotNull(TEXT("Riktor"), Riktor))
	{
		DestroyMarksWorld(World);
		return false;
	}

	const int32 Overload = MarksAbilityIndex(Gadget, TEXT("Flux.Overload"));
	if (!TestTrue(TEXT("Gadget ha Overload"), Overload != INDEX_NONE)) { DestroyMarksWorld(World); return false; }
	Gadget->PlannedAbilityIndex = Overload;
	Gadget->PlannedAttackTarget = Riktor;

	TSet<FRTCellId> Hit, Ally;
	ARTHUD::ComputePlannedHitMarks({ Gadget, Phase, Riktor }, /*PlayerTeamId=*/ 0, Hit, Ally);
	DestroyMarksWorld(World);

	// L'area di raggio 1 accende il bersaglio piu' i suoi vicini: piu' di una cella distingue «area» da
	// «bersaglio singolo».
	TestTrue(FString::Printf(TEXT("l'area e' accesa (celle: %d)"), Hit.Num()), Hit.Num() > 1);
	TestTrue(TEXT("il bersaglio e' nella zona"), Hit.Contains(Riktor->Cell));
	TestTrue(TEXT("anche la cella di Phase e' nella zona"), Hit.Contains(Phase->Cell));

	// Il punto del test: l'ALLEATA e' segnalata come fuoco amico.
	TestEqual(TEXT("una sola cella di fuoco amico"), Ally.Num(), 1);
	TestTrue(TEXT("ed e' quella di Phase"), Ally.Contains(Phase->Cell));
	// Chi lancia non si segnala mai da solo.
	TestFalse(TEXT("Gadget non e' marcato"), Ally.Contains(Gadget->Cell));
	return true;
}

/**
 * I piani AVVERSARI non si leggono, nemmeno per dedurne una cella.
 *
 * E' l'invariante #6 (privacy dell'intento) applicata a un caso in cui il risultato sarebbe «solo» un
 * colore: se il marcatore comparisse per un attacco nemico, il giocatore saprebbe che sta per essere
 * bersagliato senza che nessuno glielo abbia rivelato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHUDEnemyPlansAreNotReadTest,
	"RefactorTactics.HUD.EnemyPlansDoNotProduceMarks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHUDEnemyPlansAreNotReadTest::RunTest(const FString&)
{
	UWorld* World = MakeMarksWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	// Stavolta e' l'avversario a pianificare, su un bersaglio del giocatore.
	ARTUnit* NemicoFlux = SpawnMarksUnit(World, TEXT("Hero.Gadget"),    1, FRTCellId(-1, 0, 0));
	ARTUnit* MioRiva    = SpawnMarksUnit(World, TEXT("Hero.Phase"),    0, FRTCellId( 1, 0, 0));
	ARTUnit* MioBastion = SpawnMarksUnit(World, TEXT("Hero.Riktor"), 0, FRTCellId( 2, 0, 0));
	if (!TestNotNull(TEXT("unita'"), NemicoFlux) || !MioRiva || !MioBastion)
	{
		DestroyMarksWorld(World);
		return false;
	}

	NemicoFlux->PlannedAbilityIndex = MarksAbilityIndex(NemicoFlux, TEXT("Flux.Overload"));
	NemicoFlux->PlannedAttackTarget = MioBastion;

	TSet<FRTCellId> Hit, Ally;
	ARTHUD::ComputePlannedHitMarks({ NemicoFlux, MioRiva, MioBastion }, /*PlayerTeamId=*/ 0, Hit, Ally);
	DestroyMarksWorld(World);

	TestEqual(TEXT("nessuna cella dal piano avversario"), Hit.Num(), 0);
	TestEqual(TEXT("nessun marcatore di fuoco amico"), Ally.Num(), 0);
	return true;
}

/**
 * Un'abilita' che NON dichiara fuoco amico non produce l'avviso.
 *
 * Un allarme su un evento impossibile insegna a ignorare gli allarmi: era gia' il difetto corretto
 * nell'anteprima, e va tenuto anche qui — sono due posti che dicono la stessa cosa e devono dirla uguale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHUDNoFriendlyFireNoMarkTest,
	"RefactorTactics.HUD.NoFriendlyFireMeansNoAllyMark",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHUDNoFriendlyFireNoMarkTest::RunTest(const FString&)
{
	UWorld* World = MakeMarksWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTUnit* Gadget    = SpawnMarksUnit(World, TEXT("Hero.Gadget"),    0, FRTCellId(-1, 0, 0));
	ARTUnit* Phase    = SpawnMarksUnit(World, TEXT("Hero.Phase"),    0, FRTCellId( 1, 0, 0));
	ARTUnit* Riktor = SpawnMarksUnit(World, TEXT("Hero.Riktor"), 1, FRTCellId( 2, 0, 0));
	if (!Gadget || !Phase || !Riktor) { DestroyMarksWorld(World); return false; }

	const int32 Overload = MarksAbilityIndex(Gadget, TEXT("Flux.Overload"));
	if (!TestTrue(TEXT("Gadget ha Overload"), Overload != INDEX_NONE)) { DestroyMarksWorld(World); return false; }
	Gadget->PlannedAbilityIndex = Overload;
	Gadget->PlannedAttackTarget = Riktor;

	// Si spegne il flag sulla COPIA dell'unita', non nel catalogo: il test non deve lasciare il roster sporco
	// per chi gira dopo di lui.
	URTActionData* Ability = const_cast<URTActionData*>(Gadget->GetAbility(Overload));
	if (!TestNotNull(TEXT("abilita'"), Ability)) { DestroyMarksWorld(World); return false; }
	const bool bSaved = Ability->Def.bFriendlyFire;
	Ability->Def.bFriendlyFire = false;

	TSet<FRTCellId> Hit, Ally;
	ARTHUD::ComputePlannedHitMarks({ Gadget, Phase, Riktor }, /*PlayerTeamId=*/ 0, Hit, Ally);

	Ability->Def.bFriendlyFire = bSaved;
	DestroyMarksWorld(World);

	// La zona c'e' comunque — l'attacco parte lo stesso — ma l'alleata non e' segnalata.
	TestTrue(TEXT("la zona resta accesa"), Hit.Num() > 1);
	TestEqual(TEXT("nessun avviso di fuoco amico"), Ally.Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
