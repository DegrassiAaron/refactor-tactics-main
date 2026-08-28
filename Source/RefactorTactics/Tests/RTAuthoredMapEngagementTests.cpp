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
#include "RTOrbitProbeForTest.h" // il ritorno di periodo due, condiviso con `EngagesOnTheGeneratedTestArena`
#include "RTWorldFixtures.h"
#include "Turn/RTTurnLog.h"
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
 * Stesso oracolo di #1088 e di `D-184` — nessuna unita' viva **inerte** sulla stessa cella oltre
 * `RoundLimit / 3` — applicato alla mappa d'autore invece che all'arena generata. Il pareggio allo
 * scadere resta un esito legittimo: cio' che non lo e' e' stare fermi senza fare niente.
 *
 * ⚠️ **«Fermi» vuol dire INERTI, e la precisazione e' del 2026-08-23.** La prima stesura contava i turni
 * sulla stessa cella e basta, quindi accusava di stallo anche un kiter che presidia la propria distanza di
 * tiro e spara — il comportamento che `ScorePlan` dichiara corretto («resta a standoff e spara e'
 * esattamente il punto in cui entrambi i termini si annullano»). #1088 definisce il difetto come «soli
 * spostamenti, **zero combattimento**»: il colpo e' meta' della definizione, e mancava.
 * ⛔ **Non e' un indebolimento**, ed e' verificato per mutazione: rimettendo il difetto di #1287 — la
 * distanza in linea d'aria al posto dei passi — questo test torna rosso, perche' li' le unita' stanno
 * ferme **e** non sparano.
 *
 * 🔵 **E IL GEMELLO SULL'ARENA GENERATA HA DECISO IL CONTRARIO, APPOSTA** (#1551).
 * `Match.Autobattle.EngagesOnTheGeneratedTestArena` rifiuta questa stessa esenzione **per iscritto**, e con
 * la propria misura: il difetto di #1088 e' esattamente *«sta ferma e spara»* — Riktor parcheggiata dieci
 * turni mentre il campo produceva 19 voci `Combat` — quindi li' l'esenzione rende l'oracolo cieco. Misurato
 * su quella board: senza esenzione la sequenza e' 9 turni e il test **falsifica**; con esenzione globale
 * scende a 2 e passa verde; con esenzione per unita' a 3, e passa lo stesso.
 *
 * ⛔ **Le due risposte non sono una giusta e una sbagliata**: sono due definizioni di stallo — immobilita'
 * contro immobilita' STERILE — e ciascuna e' quella che rende falsificabile il proprio oracolo sulla propria
 * board. Qui l'esenzione **difende** il potere discriminante (senza, il rosso arriverebbe su un kiter
 * legittimo); la', **lo distrugge**. Allineare i due «per coerenza» toglie a uno dei due la prova che porta,
 * e il rosso che ne segue si legge come un difetto del bot invece che come una soglia spostata.
 *
 * ∴ chi vuole unificarli legga prima l'istruttoria: `BOT-STALL-1` in `docs/OPEN_DECISIONS.md`. DIR-C non
 * l'ha presa, perche' e' una decisione sul significato di «stallo» e il suo owner e' PDR-00.
 *
 * 🔴 **NON e' il rilevatore di OSCILLAZIONE, benche' lo dichiarasse.** Questa riga diceva che un bot che
 * alterna fra «cerca» e «avvicinati» «tornerebbe sulle stesse celle». Il suo oracolo conta i turni
 * **consecutivi sulla stessa cella** — il contatore si azzera appena la cella cambia — quindi un'alternanza
 * `A->B->A->B` lo lascia a zero e passa sempre. Misurato il 2026-08-23 su `L_HexArena`: Riktor alterna fra
 * `(1,-1,L0)` e la piattaforma `(3,-3,L1)` **otto volte in dodici turni**, e questo test resta verde.
 * L'oscillazione ha il suo oracolo in `NobodyOscillatesOnTheAuthoredMap`, qui sotto.
 *
 * ⚠️ Che faccia girare una partita resta necessario — entrambi i difetti sono di sequenza, non di singola
 * decisione — ma non e' sufficiente: e' l'ORACOLO a decidere cosa una sequenza rivela.
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
		// Il tetto di 400 tick non e' una fine turno: esaurito il budget le celle verrebbero lette a turno
		// mezzo applicato, e il `LockInAndResolve` successivo si impilerebbe sopra. Si asserisce.
		if (!TestFalse(*FString::Printf(TEXT("il turno %d ha finito di risolvere entro 400 tick"), Turni + 1),
			TM->IsResolving()))
		{
			RTWorldFixtures::DestroyWorld(World);
			return false;
		}
		++Turni;

		for (const ARTUnit* U : RTAuthoredEngagement::LiveUnits(World))
		{
			const int32 Id = U->GetUniqueID();
			const FRTCellId* Prev = Ultima.Find(Id);
			// ⚠️ **Inerte, non solo fermo.** Un turno conta solo se l'unita' non ha nemmeno colpito, e il
			// predicato lo chiede a `#1150`: la categoria `Combat` porta anche il fuoco e il terreno, dove
			// `UnitId` e' chi SUBISCE, quindi un conteggio per categoria direbbe «ha agito» di chi brucia fermo.
			bool bHaColpito = false;
			for (const FRTTurnLogEntry& E : TM->GetTurnLog())
			{
				if (E.UnitId == U->StableUnitId && URTTurnLogLibrary::IsDamageInflictedByActor(E))
				{
					bHaColpito = true;
					break;
				}
			}
			if (Prev && *Prev == U->Cell && !bHaColpito) { Ferma.FindOrAdd(Id) += 1; }
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

/**
 * **Sulla mappa che si gioca, nessuno fa avanti-e-indietro.**
 *
 * Il difetto che questo test pinna e' il **gemello** del parcheggio, e il fix di #1287 lo ha prodotto
 * mentre chiudeva l'altro. Il livello 2 di quel fix (`BuildCandidates`) restringe il dominio alle celle
 * **da cui si vede** quando il bot non puo' colpire e non vede gia' nessuno. Il suo commento dichiara che
 * restringere il dominio «spezza l'oscillazione fra cerca e avvicinati senza introdurre stato», con la
 * motivazione «uscire dalla ricerca non puo' riportare su una cella cieca, perche' quelle non sono piu'
 * candidate».
 *
 * 🔴 **La motivazione non regge, ed e' il difetto.** Uscire dalla ricerca significa `bVedeGia == true`,
 * e con quella condizione il filtro **e' spento**: le celle cieche tornano candidate immediatamente. La
 * sequenza e' un ciclo di periodo due — cella cieca (filtro acceso) -> cella che vede (filtro spento) ->
 * la cieca torna la migliore per punteggio -> cella cieca — e non si ferma mai.
 *
 * Misurato su `L_HexArena` il 2026-08-23: Riktor alterna fra `(1,-1,L0)` e la piattaforma `(3,-3,L1)`
 * **otto volte in dodici turni** in partita, e **trentasette in quaranta** nello scenario
 * `AutoBattle.ArenaV01`, dove la partita non si decide affatto.
 *
 * ⚠️ **L'oracolo e' il RITORNO DI PERIODO DUE**, non i turni fermi: `Cell[t] == Cell[t-2]` con
 * `Cell[t] != Cell[t-1]`. E' la forma minima che distingue un'alternanza da un percorso che ripassa —
 * contare le celle ripetute punirebbe un bot che aggira un ostacolo, che e' esattamente il comportamento
 * che #1287 e' andato a comprare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAuthoredMapNoOscillationTest,
	"RefactorTactics.Match.Autobattle.NobodyOscillatesOnTheAuthoredMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAuthoredMapNoOscillationTest::RunTest(const FString&)
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

	// Il rilevatore sta in `RTOrbitProbeForTest.h`, e non e' un'estrazione di comodo: la configurazione
	// SPEDITA — `MapSource = GeneratedTestArena` — non aveva alcun oracolo di oscillazione, e riscriverne
	// una seconda copia la' avrebbe messo due implementazioni della stessa firma a divergere in silenzio.
	// I limiti dichiarati (periodo due si', periodo tre no) sono nella sonda, in un posto solo.
	FRTOrbitProbe Orbite;

	const int32 MaxTurni = 12;
	int32 Turni = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Turni < MaxTurni)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
		// 🔴 **Il tetto di 400 tick non e' una fine turno, e prima usciva in silenzio.** Esaurito il budget
		// il ciclo terminava lo stesso, le celle venivano lette a turno mezzo applicato e il
		// `LockInAndResolve` successivo si impilava sopra: l'oracolo misurava posizioni che non sono mai
		// state uno stato di fine turno. Che il budget si esaurisca non e' teorico — lo scenario
		// `AutoBattle.ArenaV01` lo documenta da riga di comando. Trovato in code review.
		if (!TestFalse(*FString::Printf(TEXT("il turno %d ha finito di risolvere entro 400 tick"), Turni + 1),
			TM->IsResolving()))
		{
			RTWorldFixtures::DestroyWorld(World);
			return false;
		}
		++Turni;

		for (const ARTUnit* U : RTAuthoredEngagement::LiveUnits(World))
		{
			Orbite.Observe(U->GetUniqueID(), U->Cell);
		}
	}

	const int32 Peggiore = Orbite.WorstReturns();

	// La soglia si deriva dai turni GIOCATI, non da un tetto costante — la ragione, e il difetto per
	// aritmetica che quella forma produceva, stanno su `FRTOrbitProbe::LimitForTurns`.
	const int32 Limite = FRTOrbitProbe::LimitForTurns(Turni);

	// E la premessa va asserita, non sperata: senza abbastanza turni l'oracolo non puo' cadere, e un verde
	// li' non dice niente.
	const int32 TurniMinimi = FRTOrbitProbe::MinTurnsToFalsify;
	if (!TestTrue(*FString::Printf(
		TEXT("la partita e' durata abbastanza perche' l'oracolo possa cadere (%d turni, minimo %d)"),
		Turni, TurniMinimi), Turni >= TurniMinimi))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	AddInfo(FString::Printf(TEXT("turni giocati: %d  |  piu' ritorni di periodo due su una unita': %d (limite %d)"),
		Turni, Peggiore, Limite));

	TestTrue(*FString::Printf(
		TEXT("nessuna unita' oscilla fra due celle: %d ritorni di periodo due (limite %d)"),
		Peggiore, Limite), Peggiore <= Limite);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
