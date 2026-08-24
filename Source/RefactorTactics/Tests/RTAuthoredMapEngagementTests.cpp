// #1287 — sulla mappa d'autore i bot si fermano a due celle e non sparano.
//
// Il test di ingaggio che esisteva quando questo file e' nato imposta `MapSource = GeneratedTestArena`:
// misura l'arena generata, non la mappa che la partita carica. Dopo #1069 il `MapSource` spedito e'
// `LevelAsset`, quindi quel test guardava una configurazione che non e' piu' quella spedita — ed e' il
// buco da cui questo difetto e' passato.
//
// ✅ **Chiuso il 2026-08-23, in due mosse.** Quel test si chiamava `EngagesOnTheShippedMapSource` e ora
// si chiama `EngagesOnTheGeneratedTestArena`: il contenuto resta — le sue sei asserzioni su quella
// geometria non sono coperte da nessun altro — ed e' il NOME che era diventato falso. E l'ingaggio sulla
// mappa d'autore, che non aveva nessun oracolo, ce l'ha qui sotto: `EngagesOnTheAuthoredMap`.
//
// Misurato prima del fix, 12 turni su `L_HexArena`: 42 voci `Stayed` su 48, zero `Combat`, unita' ferme
// nove e dieci turni sulla stessa cella. La mappa ha un ostacolo al centro che blocca vista e passo, e il
// punteggio del bot misura la distanza in LINEA D'ARIA: a due celle era gia' al minimo, e aggirare
// l'ostacolo avrebbe solo peggiorato il punteggio.

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "RTGameMode.h"
#include "RTWorldFixtures.h"
#include "Turn/RTTurnLogLibrary.h" // #1150: «inflitto» si chiede al predicato, non si deduce dalla categoria
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTAuthoredEngagement
{
	TArray<ARTUnit*> LiveUnits(UWorld* World)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Found);
		TArray<ARTUnit*> Units;
		for (AActor* A : Found)
		{
			ARTUnit* U = Cast<ARTUnit>(A);
			if (U && U->IsAlive()) { Units.Add(U); }
		}
		return Units;
	}
}

/**
 * **Sulla mappa che si gioca, nessuno si parcheggia.**
 *
 * Stesso oracolo di #1088 e di `D-184` — nessuna unita' viva ferma sulla stessa cella oltre
 * `RoundLimit / 3` — applicato alla mappa d'autore invece che all'arena generata. Il pareggio allo
 * scadere resta un esito legittimo: cio' che non lo e' e' stare fermi.
 *
 * ⚠️ **E' anche il rilevatore di OSCILLAZIONE.** Un bot che alternasse fra «cerca» e «avvicinati»
 * tornerebbe sulle stesse celle: la sequenza per unita' lo mostra, e una singola chiamata a `PlanUnit`
 * no. E' la ragione per cui questo test fa girare una partita invece di interrogare la libreria.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAuthoredMapNobodyParksTest,
	"RefactorTactics.Match.Autobattle.NobodyParksOnTheAuthoredMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAuthoredMapNobodyParksTest::RunTest(const FString&)
{
	const TCHAR* AssetPath = TEXT("/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.DA_HexMap_Arena");
	URTHexMapAsset* Authored = Cast<URTHexMapAsset>(StaticLoadObject(URTHexMapAsset::StaticClass(), nullptr, AssetPath));
	if (!TestNotNull(TEXT("la mappa d'autore si carica"), Authored)) { return false; }

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("TurnManager"), TM)
		|| !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// La mappa D'AUTORE, non una generata: e' il punto del test.
	HexMap->MapAsset = Authored;
	GameMode->MapSource = ERTMapSource::LevelAsset;
	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);

	TArray<ARTUnit*> Units = RTAuthoredEngagement::LiveUnits(World);
	if (!TestEqual(TEXT("quattro unita' in campo"), Units.Num(), 4))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Sequenza di celle per unita': la memoria che rende visibile sia il parcheggio sia l'oscillazione.
	TMap<int32, FRTCellId> Ultima;
	TMap<int32, int32> Ferma;
	TMap<int32, int32> PiuLunga;

	const int32 MaxTurni = 12;
	int32 Turni = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Turni < MaxTurni)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
		++Turni;

		for (const ARTUnit* U : RTAuthoredEngagement::LiveUnits(World))
		{
			const int32 Id = U->GetUniqueID();
			const FRTCellId* Prev = Ultima.Find(Id);
			if (Prev && *Prev == U->Cell) { Ferma.FindOrAdd(Id) += 1; }
			else { Ferma.FindOrAdd(Id) = 0; }
			int32& Record = PiuLunga.FindOrAdd(Id);
			Record = FMath::Max(Record, Ferma[Id]);
			Ultima.FindOrAdd(Id) = U->Cell;
		}
	}

	int32 Peggiore = 0;
	for (const TPair<int32, int32>& P : PiuLunga) { Peggiore = FMath::Max(Peggiore, P.Value); }

	// `RoundLimit / 3`: la soglia di D-184, la stessa che l'oracolo di #1088 usa sull'arena di prova.
	const int32 Limite = FMath::Max(1, MaxTurni / 3);
	AddInfo(FString::Printf(TEXT("turni giocati: %d  |  piu' lunga sequenza ferma: %d (limite %d)"),
		Turni, Peggiore, Limite));

	TestTrue(*FString::Printf(
		TEXT("nessuna unita' si parcheggia sulla mappa d'autore: piu' lunga sequenza ferma %d turni (limite %d)"),
		Peggiore, Limite), Peggiore <= Limite);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **Sulla mappa che si gioca, i bot si COLPISCONO.**
 *
 * 🔴 **Era il buco piu' grande dei tre, e nessuno lo copriva.** L'unico oracolo dell'ingaggio del
 * repository e' `Match.Autobattle.EngagesOnTheGeneratedTestArena`, che forza `GeneratedTestArena`: dopo
 * `#1069` quella non e' piu' la sorgente che il giocatore ottiene. Gli altri due test di questo file
 * misurano il **parcheggio** e l'**oscillazione**, cioe' due modi di non concludere — nessuno dei due dice
 * se qualcuno abbia colpito. Sulla mappa d'autore si poteva quindi tornare a zero `Combat` con tutta la
 * suite verde, che e' esattamente lo stato di `#1088`.
 *
 * ⚠️ **L'oracolo e' «qualcuno ha INFLITTO danno», non «esiste una voce `Combat`».** La categoria porta
 * anche il danno da terreno e il tick di `Status.Burning`, in cui `UnitId` e' chi SUBISCE (`#1150`): un
 * conteggio per categoria direbbe «ingaggiano» di quattro unita' che bruciano ferme. Si chiede a
 * `URTTurnLogLibrary::IsDamageInflictedByActor`, che e' il posto dove quella tassonomia vive.
 *
 * 🔴 **Cosa questo test NON e', e va detto perche' il nome invita a crederlo.** NON e' la rete di `#1287`.
 * Misurato il 2026-08-23 riportando `RTHexBotLibrary.cpp` a PRIMA di quel fix: `NobodyParksOnTheAuthoredMap`
 * diventa rosso (dieci turni fermi) e **questo resta verde**, con dodici colpi inflitti. Il consuntivo di
 * `#1287` riporta «zero `Combat`» su dodici turni, ma quella misura viene dalla partita in gioco
 * (`-RTAutobattle`, con timer e formato); qui il mondo e' di prova e l'esito e' un altro. Chi cerchera' il
 * guardiano dello stallo di `#1287` lo trova nel parcheggio, non qui.
 *
 * ✅ **Cio' che difende davvero, verificato per mutazione**: togliendo le candidate d'attacco da
 * `BuildCandidates` — un bot che non propone mai un colpo — questo test cade con `0 colpi in 12 turni`, e
 * con lui il gemello sull'arena generata. E' il confine fra «si colpiscono» e «non si colpiscono», che
 * sulla mappa che si gioca non aveva **nessun** oracolo: parcheggio e oscillazione misurano due modi di non
 * concludere, non se qualcuno abbia colpito.
 *
 * ⚠️ **La soglia e' `> 0` e non di piu', deliberatamente.** Il test gemello sull'arena generata pretende
 * anche il primo colpo entro il primo terzo del formato; qui il primo colpo misurato cade **oltre** quel
 * terzo, e pinnarlo sarebbe scrivere in un test un numero di bilanciamento che nessuno ha deciso. Cio' che
 * questa voce difende e' il confine fra «si colpiscono» e «non si colpiscono»: il turno del primo colpo sta
 * nell'`AddInfo`, dove invecchia senza rompere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAuthoredMapEngagesTest,
	"RefactorTactics.Match.Autobattle.EngagesOnTheAuthoredMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAuthoredMapEngagesTest::RunTest(const FString&)
{
	const TCHAR* AssetPath = TEXT("/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.DA_HexMap_Arena");
	URTHexMapAsset* Authored = Cast<URTHexMapAsset>(StaticLoadObject(URTHexMapAsset::StaticClass(), nullptr, AssetPath));
	if (!TestNotNull(TEXT("la mappa d'autore si carica"), Authored)) { return false; }

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("TurnManager"), TM)
		|| !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	HexMap->MapAsset = Authored;
	GameMode->MapSource = ERTMapSource::LevelAsset;
	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);

	if (!TestEqual(TEXT("quattro unita' in campo"), RTAuthoredEngagement::LiveUnits(World).Num(), 4))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	int32 Colpi = 0;
	int32 PrimoColpoAlTurno = 0;
	int32 DannoInflitto = 0;
	TSet<int32> Attaccanti;

	const int32 MaxTurni = 12;
	int32 Turni = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Turni < MaxTurni)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
		// Il tetto di 400 tick non e' una fine turno: esaurito il budget, le voci lette sarebbero di un turno
		// mezzo applicato. Si asserisce invece di uscire in silenzio.
		if (!TestFalse(*FString::Printf(TEXT("il turno %d ha finito di risolvere entro 400 tick"), Turni + 1),
			TM->IsResolving()))
		{
			RTWorldFixtures::DestroyWorld(World);
			return false;
		}
		++Turni;

		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (!URTTurnLogLibrary::IsDamageInflictedByActor(E)) { continue; }
			++Colpi;
			DannoInflitto += E.Amount;
			Attaccanti.Add(E.UnitId);
			if (PrimoColpoAlTurno == 0) { PrimoColpoAlTurno = Turni; }
		}
	}

	AddInfo(FString::Printf(
		TEXT("turni giocati: %d · colpi inflitti: %d (%d danni) · primo colpo al turno %d · attaccanti distinti: %d"),
		Turni, Colpi, DannoInflitto, PrimoColpoAlTurno, Attaccanti.Num()));

	TestTrue(*FString::Printf(
		TEXT("sulla mappa d'autore i bot si colpiscono: %d colpi inflitti in %d turni (attesi > 0)"),
		Colpi, Turni), Colpi > 0);

	// Un solo attaccante sarebbe un ingaggio a senso unico, e passerebbe la riga qui sopra: e' il caso in
	// cui una squadra spara e l'altra non risponde mai. Misurato, rispondono entrambe.
	TestTrue(*FString::Printf(TEXT("e a colpire non e' una sola unita': %d attaccanti distinti"),
		Attaccanti.Num()), Attaccanti.Num() >= 2);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
