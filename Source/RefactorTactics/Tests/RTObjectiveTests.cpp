#include "Misc/AutomationTest.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Core/RTTypes.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexMapCustomVersion.h"
#include "Turn/RTMatchFormatData.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnRules.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti dagli helper degli altri file di test: nella unity build i namespace anonimi si fondono,
	// e due `MakeWorld` nello stesso blocco di traduzione sono un errore che compare a caso.
	UWorld* MakeObjectiveWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyObjectiveWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Un'arena piatta in cui UNA cella e' dichiarata obiettivo. `Objective` invalida -> arena senza obiettivi. */
	ARTHexMapActor* SpawnObjectiveMap(UWorld* World, const FRTCellId& Objective, bool bWithObjective, int32 Radius = 5)
	{
		URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		if (Map && bWithObjective)
		{
			if (const FRTHexCellData* Existing = Map->FindCell(Objective))
			{
				FRTHexCellData Marked = *Existing;
				Marked.bIsObjective = true;
				Map->AddOrUpdateCell(Marked);
			}
		}

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = Map;
		return Actor;
	}

	/**
	 * Un'unita' viva e FERMA sulla cella indicata: questi test verificano chi controlla l'obiettivo, non come
	 * ci si arriva. Nessuna pianificazione, quindi nessuno si muove e nessuno spara.
	 */
	ARTUnit* SpawnObjectiveUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeRiktor());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Un round completo senza pianificazione: nessuno agisce, il turno arriva fino al Cleanup e passa. */
	void PlayObjectiveRound(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/**
	 * Le sole voci di categoria `Objective` del TURNO APPENA RISOLTO.
	 *
	 * ⚠️ `ARTTurnManager::GetTurnLog()` non e' un archivio: `LockInAndResolve` fa `TurnLog.Reset()` a ogni
	 * lock-in, quindi porta l'ultimo turno e non la partita. Un test che lo leggesse come cumulativo
	 * conterebbe sempre uno — ed e' esattamente l'errore che ha fatto fallire la prima stesura di questi due.
	 */
	TArray<FRTTurnLogEntry> ObjectiveEntries(const TArray<FRTTurnLogEntry>& Log)
	{
		TArray<FRTTurnLogEntry> Out;
		for (const FRTTurnLogEntry& E : Log)
		{
			if (E.Category == ERTLogCategory::Objective)
			{
				Out.Add(E);
			}
		}
		return Out;
	}

	/** Regole di partita che NON possono chiudere la partita: qui si misura il progresso, non la vittoria. */
	FRTMatchRules OpenEndedRules()
	{
		FRTMatchRules Rules;
		Rules.FormatId = FName(TEXT("Format.ObjectiveControlTest"));
		Rules.RoundLimit = 50;   // lontano: nessun test qui deve chiudere per scadenza
		Rules.ScoreToWin = 0;    // via disattivata, come il formato reale della v0.1
		return Rules;
	}
}

/**
 * La contesa PARITARIA non fa progredire nessuno — prima riga della DoD di CP 10.2 (#75).
 *
 * Due meta': la regola pura, e la stessa regola in una partita vera. Senza la seconda, la funzione potrebbe
 * essere corretta e non essere chiamata da nessuno — che e' il difetto ricorrente di questo repository, e la
 * ragione per cui `AddTeamScore` e' rimasto per mesi con un solo chiamante, un test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTObjectiveContestedNoProgressTest,
	"RefactorTactics.Objectives.ContestedNoProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTObjectiveContestedNoProgressTest::RunTest(const FString&)
{
	// --- la regola pura -----------------------------------------------------
	TestTrue(TEXT("uno contro uno e' conteso"),
		URTTurnRules::ResolveObjectiveControl(1, 1) == ERTObjectiveOutcome::Contested);
	TestTrue(TEXT("due contro due e' conteso: conta la PARITA', non il numero"),
		URTTurnRules::ResolveObjectiveControl(2, 2) == ERTObjectiveOutcome::Contested);
	TestTrue(TEXT("nessuno presente non e' una contesa"),
		URTTurnRules::ResolveObjectiveControl(0, 0) == ERTObjectiveOutcome::Unclaimed);
	TestTrue(TEXT("chi e' solo controlla"),
		URTTurnRules::ResolveObjectiveControl(1, 0) == ERTObjectiveOutcome::Team0Scores);
	TestTrue(TEXT("e vale per entrambe le squadre"),
		URTTurnRules::ResolveObjectiveControl(0, 3) == ERTObjectiveOutcome::Team1Scores);
	// La maggioranza controlla: due contro uno non e' una contesa, e' un vantaggio.
	TestTrue(TEXT("chi e' in piu' controlla"),
		URTTurnRules::ResolveObjectiveControl(2, 1) == ERTObjectiveOutcome::Team0Scores);

	// --- la stessa regola, in partita ---------------------------------------
	UWorld* World = MakeObjectiveWorld();
	if (!TestNotNull(TEXT("world di prova"), World))
	{
		return false;
	}

	const FRTCellId Objective(0, 0);
	ARTHexMapActor* MapActor = SpawnObjectiveMap(World, Objective, /*bWithObjective=*/ true);

	// 🔴 **DUE celle obiettivo, una per unita' — e non e' un dettaglio di comodo** (#1970). Questa fixture
	// metteva entrambe le unita' sulla STESSA cella, che e' uno stato che il gioco vieta: `MakeSnapshot`
	// tiene una sola unita' viva per cella e da oggi lo **dice**, quindi il test cadeva su un `Error` del
	// resolver. La contesa non ha bisogno della stessa cella: `ResolveObjectiveControl` conta le presenze
	// su TUTTE le celle marcate, quindi 1v1 su due celle obiettivo e' la stessa identica regola con uno
	// stato legale.
	const FRTCellId SecondObjective(1, 0);
	if (MapActor && MapActor->MapAsset)
	{
		if (const FRTHexCellData* Existing = MapActor->MapAsset->FindCell(SecondObjective))
		{
			FRTHexCellData Marked = *Existing;
			Marked.bIsObjective = true;
			MapActor->MapAsset->AddOrUpdateCell(Marked);
		}
	}

	// Una per squadra, su celle obiettivo diverse: e' la presenza che conta, e nessuna delle due agisce.
	SpawnObjectiveUnit(World, 0, Objective);
	SpawnObjectiveUnit(World, 1, SecondObjective);

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM)
	{
		DestroyObjectiveWorld(World);
		return false;
	}
	TM->SetMatchRules(OpenEndedRules());

	PlayObjectiveRound(TM);

	TestEqual(TEXT("la squadra 0 non progredisce"), TM->GetTeamScore(0), 0);
	TestEqual(TEXT("e nemmeno la squadra 1"), TM->GetTeamScore(1), 0);

	const TArray<FRTTurnLogEntry> Entries = ObjectiveEntries(TM->GetTurnLog());
	if (TestEqual(TEXT("il turno conteso lascia comunque una voce"), Entries.Num(), 1))
	{
		// Il silenzio non basterebbe: un turno conteso e un turno in cui l'obiettivo non esiste sarebbero
		// indistinguibili, e la contesa e' precisamente cio' che questo checkpoint aggiunge alla partita.
		TestTrue(TEXT("e la voce dice CONTESO"),
			Entries[0].Outcome == static_cast<uint8>(ERTObjectiveOutcome::Contested));
		TestEqual(TEXT("con zero punti assegnati"), Entries[0].Amount, 0);
	}

	DestroyObjectiveWorld(World);
	return true;
}

/**
 * Il controllo si verifica NEL CLEANUP, dopo gli effetti ambientali e i KO — seconda riga della DoD.
 *
 * Il verso che morde e' l'ordine: la voce nasce in fase `Cleanup`, e chi e' stato eliminato durante il turno
 * non tiene l'obiettivo. La seconda meta' si misura uccidendo l'occupante: se il controllo fosse valutato
 * prima dei KO, un morto continuerebbe a fare punto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTObjectiveCheckedInCleanupTest,
	"RefactorTactics.Objectives.CheckedInCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTObjectiveCheckedInCleanupTest::RunTest(const FString&)
{
	UWorld* World = MakeObjectiveWorld();
	if (!TestNotNull(TEXT("world di prova"), World))
	{
		return false;
	}

	const FRTCellId Objective(0, 0);
	SpawnObjectiveMap(World, Objective, /*bWithObjective=*/ true);

	ARTUnit* Holder = SpawnObjectiveUnit(World, 0, Objective);
	SpawnObjectiveUnit(World, 1, FRTCellId(4, -2)); // lontana: non contende

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Holder)
	{
		DestroyObjectiveWorld(World);
		return false;
	}
	TM->SetMatchRules(OpenEndedRules());

	PlayObjectiveRound(TM);

	const TArray<FRTTurnLogEntry> Entries = ObjectiveEntries(TM->GetTurnLog());
	if (TestEqual(TEXT("una voce per turno con obiettivo"), Entries.Num(), 1))
	{
		TestTrue(TEXT("scritta in fase Cleanup"), Entries[0].Phase == ERTMatchPhase::Cleanup);
		TestTrue(TEXT("chi era solo ha segnato"),
			Entries[0].Outcome == static_cast<uint8>(ERTObjectiveOutcome::Team0Scores));
		// [D-063]: le voci senza unita' portano `UnitId = 0`. Il punto lo fa la squadra, non chi ci stava sopra.
		TestEqual(TEXT("e non nomina nessuna unita'"), Entries[0].UnitId, 0);
	}
	TestEqual(TEXT("il punteggio e' salito"), TM->GetTeamScore(0), 1);

	// Il presidio muore: dal turno dopo l'obiettivo non e' piu' suo, ed e' la meta' «dopo i KO» della DoD.
	//
	// ⚠️ `ApplyCombatState` e' l'API con cui il resolver scrive l'esito di un colpo, e qui prepara lo STATO
	// INIZIALE del turno successivo — non salta la regola in prova, che e' il conteggio dei presenti nel
	// Cleanup. Uccidere per via di gioco richiederebbe di pianificare un attacco, cioe' di far dipendere
	// questo test dal resolver di combattimento: un secondo soggetto in un test che ne ha gia' uno.
	Holder->ApplyCombatState(/*NewHealth=*/ 0, /*NewShield=*/ 0);
	TestFalse(TEXT("il presidio e' caduto"), Holder->IsAlive());

	PlayObjectiveRound(TM);

	const TArray<FRTTurnLogEntry> After = ObjectiveEntries(TM->GetTurnLog());
	if (TestEqual(TEXT("il secondo turno scrive la sua voce"), After.Num(), 1))
	{
		TestTrue(TEXT("un morto non tiene l'obiettivo"),
			After[0].Outcome == static_cast<uint8>(ERTObjectiveOutcome::Unclaimed));
	}
	TestEqual(TEXT("e il punteggio non cresce piu'"), TM->GetTeamScore(0), 1);

	DestroyObjectiveWorld(World);
	return true;
}

/**
 * Il progresso e' un INTERO, e cresce di un passo per turno controllato — terza riga della DoD.
 *
 * Il test che discrimina non e' «e' un int32» (lo dice il tipo): e' che tre turni di controllo diano
 * ESATTAMENTE tre, cioe' che il progresso sia contabile e non una frazione accumulata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTObjectiveProgressIsIntegerTest,
	"RefactorTactics.Objectives.ProgressIsInteger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTObjectiveProgressIsIntegerTest::RunTest(const FString&)
{
	UWorld* World = MakeObjectiveWorld();
	if (!TestNotNull(TEXT("world di prova"), World))
	{
		return false;
	}

	const FRTCellId Objective(0, 0);
	SpawnObjectiveMap(World, Objective, /*bWithObjective=*/ true);
	SpawnObjectiveUnit(World, 0, Objective);
	SpawnObjectiveUnit(World, 1, FRTCellId(4, -2));

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM)
	{
		DestroyObjectiveWorld(World);
		return false;
	}
	TM->SetMatchRules(OpenEndedRules());

	// La colonna `Amount` si somma turno per turno, perche' il TurnLog porta un turno solo: e' la stessa
	// somma che un consumatore farebbe leggendo l'archivio, fatta man mano invece che alla fine.
	int32 Sum = 0;
	for (int32 Round = 1; Round <= 3; ++Round)
	{
		PlayObjectiveRound(TM);
		TestEqual(*FString::Printf(TEXT("dopo %d turni di controllo il progresso e' %d"), Round, Round),
			TM->GetTeamScore(0), Round);

		const TArray<FRTTurnLogEntry> Entries = ObjectiveEntries(TM->GetTurnLog());
		if (TestEqual(*FString::Printf(TEXT("il turno %d scrive una voce sola"), Round), Entries.Num(), 1))
		{
			// `Amount` porta i punti EFFETTIVI: chi somma questa colonna ottiene il punteggio senza dover
			// reinterpretare l'esito, ed e' la forma in cui l'HUD lo leggera'.
			TestEqual(TEXT("e vale un punto intero"), Entries[0].Amount, 1);
			Sum += Entries[0].Amount;
		}
	}
	TestEqual(TEXT("la somma della colonna e' il punteggio"), Sum, TM->GetTeamScore(0));

	DestroyObjectiveWorld(World);
	return true;
}

/**
 * Una mappa SENZA obiettivi non produce nessuna voce, e non e' un dettaglio di pulizia.
 *
 * Tutte le mappe versionate oggi sono senza obiettivi: se il Cleanup scrivesse comunque una voce «non e'
 * successo niente», ogni partita gia' archiviata ne guadagnerebbe una per turno e il corpus golden
 * divergerebbe su scenari che con l'obiettivo non hanno niente a che vedere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTObjectiveSilentWithoutObjectiveTest,
	"RefactorTactics.Objectives.SilentWithoutObjectiveCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTObjectiveSilentWithoutObjectiveTest::RunTest(const FString&)
{
	UWorld* World = MakeObjectiveWorld();
	if (!TestNotNull(TEXT("world di prova"), World))
	{
		return false;
	}

	SpawnObjectiveMap(World, FRTCellId(0, 0), /*bWithObjective=*/ false);
	SpawnObjectiveUnit(World, 0, FRTCellId(0, 0));
	SpawnObjectiveUnit(World, 1, FRTCellId(4, -2));

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM)
	{
		DestroyObjectiveWorld(World);
		return false;
	}
	TM->SetMatchRules(OpenEndedRules());

	PlayObjectiveRound(TM);
	PlayObjectiveRound(TM);

	TestEqual(TEXT("nessuna voce di obiettivo"), ObjectiveEntries(TM->GetTurnLog()).Num(), 0);
	TestEqual(TEXT("e nessun punteggio: stare al centro non basta se il centro non e' un obiettivo"),
		TM->GetTeamScore(0), 0);

	DestroyObjectiveWorld(World);
	return true;
}

/**
 * L'obiettivo entra nell'HASH della mappa: due mappe identiche in tutto tranne dove sta l'obiettivo non si
 * giocano allo stesso modo, quindi non possono avere lo stesso hash.
 *
 * E' il criterio dichiarato di `ComputeHash` — «ci entra cio' che puo' cambiare un esito» — applicato al
 * campo che cambia **chi vince**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTObjectiveChangesMapHashTest,
	"RefactorTactics.Objectives.ObjectiveChangesMapHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTObjectiveChangesMapHashTest::RunTest(const FString&)
{
	URTHexMapAsset* Bare = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 3);
	URTHexMapAsset* WithObjective = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 3);
	if (!TestNotNull(TEXT("arena di prova"), Bare) || !TestNotNull(TEXT("arena gemella"), WithObjective))
	{
		return false;
	}

	TestEqual(TEXT("due arene identiche hashano uguale"), Bare->ComputeHash(), WithObjective->ComputeHash());
	TestFalse(TEXT("e nessuna delle due dichiara obiettivi"), Bare->HasObjectiveCell());

	const FRTCellId Objective(1, 0);
	if (const FRTHexCellData* Existing = WithObjective->FindCell(Objective))
	{
		FRTHexCellData Marked = *Existing;
		Marked.bIsObjective = true;
		WithObjective->AddOrUpdateCell(Marked);
	}

	TestTrue(TEXT("la mappa sa di avere un obiettivo"), WithObjective->HasObjectiveCell());
	TestTrue(TEXT("e la cella dichiarata e' quella"), WithObjective->FirstObjectiveCell() == Objective);
	TestNotEqual(TEXT("l'obiettivo cambia l'hash"), WithObjective->ComputeHash(), Bare->ComputeHash());

	// La versione del formato che porta il campo, dichiarata dove i lettori la cercano.
	//
	// ⚠️ Pinnava `CurrentFormatVersion == 11`, che diceva la cosa giusta solo finche' 11 ERA la corrente:
	// col bump a v12 (#1864, muro interno) quell'asserzione sarebbe diventata «il formato e' v12» sotto un
	// commento che parla del campo obiettivo — arrivato in v11 e non in v12. Ora nomina la costante che
	// porta davvero quel campo, quindi resta vera a ogni bump futuro.
	//
	// ⛔ Non indebolisce il gate sui bump: a farlo cadere sono i quattro pin su `CurrentFormatVersion` in
	// `RTHexArcTests` · `RTHexDoorTests` · `RTHexMapTests` · `RTHexOccupancyTests`, che restano.
	TestEqual(TEXT("l'obiettivo e' arrivato col formato v11"),
		static_cast<int32>(FRTHexMapCustomVersion::ObjectiveCell), 11);

	return true;
}

/**
 * La MAPPA D'AUTORE dichiara il suo obiettivo, ed e' l'unica cosa che un `.uasset` non puo' dire da solo.
 *
 * ⚠️ **Senza questo test l'obiettivo vive dentro un binario che nessuno controlla.** Il diff di una PR non
 * mostra cosa cambia dentro un `.uasset`: chi lo aprisse in Editor e togliesse la spunta senza accorgersene
 * non farebbe cadere nulla — la regola resterebbe verde su una mappa che non ha piu' un obiettivo, e
 * `Objectives.SilentWithoutObjectiveCell` continuerebbe a passare perche' quel caso e' legittimo altrove.
 *
 * Verifica anche che la cella sia PERCORRIBILE: un obiettivo su cui nessuno puo' salire resterebbe
 * `Unclaimed` per sempre, e la partita non lo direbbe mai.
 *
 * 🔴 **La cella e' `(0,-3,0)`, la PORTA NORD, e il numero non e' arbitrario**: la barriera su `q=0` ha due
 * sole aperture — `(0,-3,0)` e `(0,3,0)` — e sono le uniche celle percorribili equidistanti dai due spawn,
 * che `PickStartCells` deriva su `(-4,0)` e `(4,0)`. Fra le due, la nord e' la via VELOCE e SCOPERTA
 * (`D-241`): chi tiene l'obiettivo e' visibile, e la via sud coperta resta l'aggiramento per scacciarlo.
 * Metterlo sulla sud premierebbe chi arriva primo e si pianta dietro lo schermo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTObjectiveAuthoredArenaDeclaresItTest,
	"RefactorTactics.Objectives.AuthoredArenaDeclaresItsObjective",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTObjectiveAuthoredArenaDeclaresItTest::RunTest(const FString&)
{
	const TCHAR* Path = TEXT("/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena");
	URTHexMapAsset* Arena = LoadObject<URTHexMapAsset>(nullptr, Path);
	if (!TestNotNull(TEXT("DA_HexMap_Arena si carica"), Arena))
	{
		return false;
	}

	if (!TestTrue(TEXT("l'arena d'autore dichiara un obiettivo"), Arena->HasObjectiveCell()))
	{
		// Senza la premessa, le righe sotto sarebbero verdi per il motivo sbagliato.
		return false;
	}

	const FRTCellId Objective = Arena->FirstObjectiveCell();
	TestTrue(TEXT("ed e' la porta nord (0,-3,0)"), Objective == FRTCellId(0, -3, 0));

	const FRTHexCellData* Cell = Arena->FindCell(Objective);
	if (TestNotNull(TEXT("la cella esiste"), Cell))
	{
		TestFalse(TEXT("e si puo' salirci: un obiettivo bloccato non e' contendibile"), Cell->bBlocksMovement);
	}

	// Uno solo: piu' obiettivi simultanei sono CP 31.1 (post-v0.1), e il TurnLog oggi ne nomina uno.
	int32 Quanti = 0;
	for (const FRTHexCellData& C : Arena->Cells)
	{
		if (C.bIsObjective)
		{
			++Quanti;
		}
	}
	TestEqual(TEXT("e ne dichiara esattamente uno"), Quanti, 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
