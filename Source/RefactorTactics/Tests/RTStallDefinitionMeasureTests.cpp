// `BOT-STALL-1` — misurare l'uscita (c) senza adottarla (`#1551`, `docs/OPEN_DECISIONS.md`).
//
// La voce ha quattro uscite e una raccomandata — la **(c)**, esenzione condizionata all'AVANZAMENTO — che e'
// **l'unica per cui non esiste nessuna misura**. Questo file la produce, su ENTRAMBE le board su cui girano i
// due oracoli di parcheggio, e non la adotta da nessuna parte.
//
// ⛔ **Qui non si decide.** Nessun oracolo cambia: ne' soglia, ne' esenzione, ne' rango del warning.
// `NobodyParksOnTheAuthoredMap` e `EngagesOnTheGeneratedTestArena` non sono toccati, e nemmeno letti: questo
// file allestisce le proprie partite sulle stesse due sorgenti. L'owner della decisione e' `PDR-00`, e
// unificare i due oracoli distruggerebbe la prova che uno dei due porta — e' cio' che `#1551` mette fuori
// scope e che `OPEN_DECISIONS` avverte di non fare.
//
// ⛔ **Nessuna soglia nuova.** Le sequenze si stampano accanto ai limiti che le due board gia' usano, per
// rendere leggibile il numero; nessuna asserzione le confronta. Una soglia introdotta di lato sarebbe la
// decisione presa qui.
//
// ## LA MISURA — 2026-08-29, `HEAD c2f694dc`, worktree `wt-dir-c-v02`, run dichiarata VALIDA
//
//     definizione                     mappa d'autore   arena generata
//     (b) immobilita'                       4                4        <- CONTROLLO
//     (a) immobilita' sterile               3                2
//     (c) salute netta                      3                2
//     (c) pool netto (salute+scudo)         3                2
//     (c) eliminazione                      4                4
//     (c) salute o eliminazione             3                2
//     turni giocati                        12               11
//     limite in uso                         4                4
//
// ✅ **Il controllo valida il banco.** La (b) sull'arena generata da' **4 su soglia 4**, cioe' esattamente
// il margine ZERO che `OPEN_DECISIONS` registra per quella board. Senza questo riscontro i numeri (c) non
// varrebbero niente: descriverebbero un'altra partita.
//
// 🔴 **E la prima stesura ne descriveva davvero un'altra.** Con 12 turni e senza `PlanBotsForTest()` la (b)
// sull'arena generata dava **10**. I due oracoli pilotano diversamente — `EngagesOnTheGeneratedTestArena`
// chiama `PlanBotsForTest()` e tiene `MaxTurns = 40` come tetto di SICUREZZA (la partita finisce per
// regola), `NobodyParksOnTheAuthoredMap` non lo chiama e si ferma a 12 — e specchiarli non e' un dettaglio
// di allestimento: e' la condizione perche' il numero parli della board.
//
// 🔴 **COSA DICE ALLA DECISIONE, e non e' cio' che la raccomandazione si aspetta.** Su queste due board la
// **(c) coincide con la (a)** in tre operativizzazioni su quattro: `salute netta`, `pool netto` e
// `salute o eliminazione` danno gli stessi numeri della (a), su ENTRAMBE le board. La quarta —
// `eliminazione` — coincide invece con la **(b)**: l'esenzione non scatta quasi mai, perche' un nemico che
// cade DENTRO una finestra ferma e' raro.
//
// ∴ La (c) raccomandata costa **una finestra di HP per unita' e una soglia nuova** — che `OPEN_DECISIONS`
// dichiara materia di `D-184` e non di un test — e su questi dati **non compra nessun verdetto diverso da
// quello che la (a) gia' da'**. Non e' un argomento a favore della (a): e' il costo della (c) messo accanto
// a cio' che rende, che e' precisamente il dato che mancava a chi deve decidere.
//
// ⚠️ **LIMITE DICHIARATO, e va letto prima di usare questi numeri.** NON e' misurato se la (c) conservi il
// potere discriminante che la (a) perde — cioe' se sul difetto di `#1088` (*«sta ferma e spara»*: Riktor
// parcheggiata dieci turni mentre il campo produceva 19 voci `Combat`) la (c) resterebbe rossa la' dove la
// (a) diventa cieca. Quel difetto oggi non si riproduce su nessuna delle due board, quindi la domanda **non
// ha soggetto** qui. E' l'unico argomento che potrebbe rimettere la (c) davanti alla (a), e questo lavoro
// non lo tocca.

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Misc/CommandLine.h"
#include "RTAuthoredArenaForTest.h"
#include "RTGameMode.h"
#include "RTStallDefinitionProbeForTest.h"
#include "RTWorldFixtures.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h" // #1150: «inflitto» si chiede al predicato, non alla categoria
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

extern TAutoConsoleVariable<int32> CVarRTAutobattle;
extern TAutoConsoleVariable<float> CVarRTPlanningSeconds;

namespace RTStallMisura
{
	using EDef = FRTStallDefinitionProbe::EDefinizione;

	/**
	 * Stato di processo che sopravvive al test, salvato e ripristinato.
	 *
	 * ⚠️ Duplica le guardie di `RTMatchAutobattleTests.cpp` invece di riusarle: la' vivono in un namespace
	 * anonimo, e tirarle fuori significherebbe modificare il file di un oracolo che questo lavoro ha il
	 * mandato esplicito di non toccare. La duplicazione e' il costo dichiarato di quel vincolo.
	 */
	struct FScopedCVars
	{
		int32 SavedMode;
		float SavedPlanning;
		FString SavedCmdLine;
		FScopedCVars()
			: SavedMode(CVarRTAutobattle.GetValueOnGameThread())
			, SavedPlanning(CVarRTPlanningSeconds.GetValueOnGameThread())
			, SavedCmdLine(FCommandLine::Get()) {}
		~FScopedCVars()
		{
			CVarRTAutobattle->Set(SavedMode, ECVF_SetByCode);
			CVarRTPlanningSeconds->Set(SavedPlanning, ECVF_SetByCode);
			FCommandLine::Set(*SavedCmdLine);
		}
	};

	TArray<ARTUnit*> UnitaVive(UWorld* World)
	{
		TArray<AActor*> Trovati;
		UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Trovati);
		TArray<ARTUnit*> Vive;
		for (AActor* A : Trovati)
		{
			ARTUnit* U = Cast<ARTUnit>(A);
			if (U && U->IsAlive()) { Vive.Add(U); }
		}
		return Vive;
	}

	/** I totali per squadra, a fine turno. La sonda non conosce le squadre: glieli si passa gia' fatti. */
	struct FTotali { int32 Salute = 0; int32 Pool = 0; int32 Vivi = 0; };

	TMap<int32, FTotali> TotaliPerSquadra(const TArray<ARTUnit*>& Vive)
	{
		TMap<int32, FTotali> Per;
		for (const ARTUnit* U : Vive)
		{
			FTotali& T = Per.FindOrAdd(U->TeamId);
			T.Salute += U->Health;
			T.Pool += U->Health + U->Shield;
			T.Vivi += 1;
		}
		return Per;
	}

	/** Tutto cio' che non e' della squadra `Mia`. Generico sul numero di squadre, non solo su due. */
	FRTStallDefinitionProbe::FStatoNemico NemiciDi(const TMap<int32, FTotali>& Per, int32 Mia)
	{
		FRTStallDefinitionProbe::FStatoNemico N;
		for (const TPair<int32, FTotali>& P : Per)
		{
			if (P.Key == Mia) { continue; }
			N.Salute += P.Value.Salute;
			N.Pool += P.Value.Pool;
			N.Vivi += P.Value.Vivi;
		}
		return N;
	}

	/** Il referto: una riga per definizione, nell'ordine in cui l'enum le dichiara. */
	void Riporta(FAutomationTestBase& Test, const TCHAR* Board, const FRTStallDefinitionProbe& Sonda,
		int32 Turni, int32 Limite)
	{
		Test.AddInfo(FString::Printf(TEXT("%s — %d turni giocati, %d unita' osservate (limite in uso: %d)"),
			Board, Turni, Sonda.UnitaOsservate(), Limite));
		for (int32 I = 0; I < FRTStallDefinitionProbe::NumDefinizioni; ++I)
		{
			const EDef D = static_cast<EDef>(I);
			Test.AddInfo(FString::Printf(TEXT("%s — %-32s sequenza ferma piu' lunga: %d"),
				Board, FRTStallDefinitionProbe::NomeDi(D), Sonda.Peggiore(D)));
		}
	}

	/**
	 * L'invariante che rende leggibile il referto, e che non decide niente.
	 *
	 * Ogni `(c)` esenta un SOTTOINSIEME di cio' che esenta la `(a)` — «armata **e** avanzata» implica
	 * «armata» — quindi la sua sequenza non puo' essere piu' corta di quella della (a), ne' piu' lunga di
	 * quella della (b), che non esenta nulla. E' una proprieta' delle definizioni, non del bot: se cade, il
	 * numero pubblicato non descrive la definizione che dice di descrivere.
	 */
	void VerificaOrdinamento(FAutomationTestBase& Test, const TCHAR* Board, const FRTStallDefinitionProbe& S)
	{
		const int32 B = S.Peggiore(EDef::Immobilita);
		const int32 A = S.Peggiore(EDef::Sterile);
		for (int32 I = static_cast<int32>(EDef::SaluteNetta); I < FRTStallDefinitionProbe::NumDefinizioni; ++I)
		{
			const EDef D = static_cast<EDef>(I);
			const int32 C = S.Peggiore(D);
			Test.TestTrue(FString::Printf(TEXT("%s — %s (%d) sta fra la (a) (%d) e la (b) (%d)"),
				Board, FRTStallDefinitionProbe::NomeDi(D), C, A, B), A <= C && C <= B);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------
// La sonda, senza mondo: dove le definizioni si separano davvero
// ---------------------------------------------------------------------------------------------------------

/**
 * **Le sei letture danno numeri diversi sulla stessa sequenza, e si sa QUALI e PERCHE'.**
 *
 * Una sonda che desse lo stesso numero a tutte sarebbe verde e inutile: il referto delle due board non
 * distinguerebbe nulla, e nessuno se ne accorgerebbe. Qui la separazione e' costruita a mano.
 *
 * Lo scenario: un'unita' ferma per quattro turni sulla stessa cella, armata a ogni turno, contro un nemico
 * il cui `Health` non cala mai — tutto il danno finisce nello scudo, che `D-224` ricarica nel Cleanup.
 *
 * ⚠️ **Verificato per mutazione il 2026-08-29**: sostituendo `bArmato && bSaluteCalata` con il solo
 * `bArmato` in `FRTStallDefinitionProbe::Observe` — cioe' collassando la (c) sulla (a) — la riga
 * `SaluteNetta` cade, perche' il 4 atteso diventa 0.
 * ⚠️ **Misurato: cadono ENTRAMBI i test Meta, non solo questo** — anche
 * `StallDefinitionProbeWindowIsTheStillRun` asserisce `SaluteNetta`. La nota prevedeva un rosso solo, e la
 * misura ne ha dati due: e' scritto qui perche' chi vedra' quel doppio rosso non lo legga come due difetti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStallDefinitionSeparatesTest,
	"RefactorTactics.Meta.StallDefinitionProbeSeparatesTheDefinitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStallDefinitionSeparatesTest::RunTest(const FString&)
{
	using EDef = FRTStallDefinitionProbe::EDefinizione;
	FRTStallDefinitionProbe Sonda;

	const FRTCellId Ferma(0, 0, 0);
	// Il pool cala di 5 a turno — lo scudo eroso — mentre la salute resta 100 e nessuno cade.
	int32 Pool = 100;
	for (int32 T = 0; T < 5; ++T)
	{
		FRTStallDefinitionProbe::FStatoNemico N;
		N.Salute = 100;
		N.Pool = Pool;
		N.Vivi = 2;
		Sonda.Observe(/*UnitKey=*/ 1, Ferma, /*bArmato=*/ true, N);
		Pool -= 5;
	}

	// Cinque osservazioni sulla stessa cella = quattro turni FERMI (il primo apre la finestra).
	TestEqual(TEXT("(b) immobilita': quattro turni fermi"), Sonda.Peggiore(EDef::Immobilita), 4);
	TestEqual(TEXT("(a) sterile: zero, l'unita' e' armata a ogni turno"), Sonda.Peggiore(EDef::Sterile), 0);
	// 🔴 Il punto di tutto il lavoro: armata e senza avanzamento in salute, la (c) NON esenta.
	TestEqual(TEXT("(c) salute netta: quattro — il danno non fa calare gli Health"),
		Sonda.Peggiore(EDef::SaluteNetta), 4);
	TestEqual(TEXT("(c) eliminazione: quattro — nessuno cade"), Sonda.Peggiore(EDef::Eliminazione), 4);
	TestEqual(TEXT("(c) salute o eliminazione: quattro"), Sonda.Peggiore(EDef::SaluteOEliminazione), 4);
	// E il pool invece cala: qui la (c) esenta, e collassa sulla (a). E' la lettura scartata, resa visibile.
	TestEqual(TEXT("(c) pool netto: zero — lo scudo eroso conta come avanzamento"),
		Sonda.Peggiore(EDef::PoolNetto), 0);

	return true;
}

/**
 * **La finestra e' la sequenza FERMA, non quella gia' esentata** — e la differenza si vede.
 *
 * Un'unita' che si muove riapre la finestra: i totali nemici d'inizio si riazzerano, quindi un avanzamento
 * ottenuto PRIMA dello spostamento non esenta i turni che vengono dopo. Senza questa regola un colpo
 * andato a segno al turno uno terrebbe esente un parcheggio di venti turni.
 *
 * ⚠️ **Verificato per mutazione il 2026-08-29**: congelando `InizioSalute` al primo valore mai visto —
 * cioe' togliendo il riazzeramento che riapre la finestra — questa asserzione cade, perche' il 3 atteso
 * diventa 0. ✅ E cade **solo questo**: `StallDefinitionProbeSeparatesTheDefinitions` resta verde, quindi i
 * due test Meta non sono due nomi dello stesso controllo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStallDefinitionWindowTest,
	"RefactorTactics.Meta.StallDefinitionProbeWindowIsTheStillRun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStallDefinitionWindowTest::RunTest(const FString&)
{
	using EDef = FRTStallDefinitionProbe::EDefinizione;
	FRTStallDefinitionProbe Sonda;

	const FRTCellId A(0, 0, 0);
	const FRTCellId B(1, 0, 0);

	auto Osserva = [&Sonda](const FRTCellId& C, int32 Salute)
	{
		FRTStallDefinitionProbe::FStatoNemico N;
		N.Salute = Salute;
		N.Pool = Salute;
		N.Vivi = 2;
		Sonda.Observe(/*UnitKey=*/ 7, C, /*bArmato=*/ true, N);
	};

	Osserva(A, 100);   // apre la finestra su A
	Osserva(A, 90);    // ferma, armata, salute calata -> esente, sequenza 0
	Osserva(B, 90);    // si sposta: nuova finestra, riferimento 90
	Osserva(B, 90);    // ferma, armata, ma NIENTE avanzamento da 90 -> conta
	Osserva(B, 90);    // conta
	Osserva(B, 90);    // conta

	TestEqual(TEXT("(b) immobilita': tre turni fermi su B"), Sonda.Peggiore(EDef::Immobilita), 3);
	// 🔴 Se la finestra non si riaprisse, il calo del turno due terrebbe esenti anche i tre su B.
	TestEqual(TEXT("(c) salute netta: tre — l'avanzamento su A non vale su B"),
		Sonda.Peggiore(EDef::SaluteNetta), 3);
	TestEqual(TEXT("(a) sterile: zero — armata a ogni turno"), Sonda.Peggiore(EDef::Sterile), 0);

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Le due board
// ---------------------------------------------------------------------------------------------------------

namespace RTStallMisura
{
	/** Gioca la partita gia' allestita e riempie la sonda. Ritorna i turni giocati, o -1 se un turno pende. */
	int32 Gioca(FAutomationTestBase& Test, UWorld* World, ARTTurnManager* TM,
		FRTStallDefinitionProbe& Sonda, int32 MaxTurni, bool bPianificaEsplicito, int32& OutTurniArmati)
	{
		OutTurniArmati = 0;
		int32 Turni = 0;
		while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Turni < MaxTurni)
		{
			// ⚠️ **Si specchia l'oracolo della board, non si sceglie.** `EngagesOnTheGeneratedTestArena`
			// chiama `PlanBotsForTest()` prima di risolvere, `NobodyParksOnTheAuthoredMap` no. Misurato il
			// 2026-08-29: pilotare entrambe senza la chiamata esplicita porta la (b) sull'arena generata a
			// **10**, cioe' una partita in cui i bot non decidono — e ogni numero (c) letto li' descrive
			// quella, non la partita che l'oracolo sorveglia.
			if (bPianificaEsplicito) { TM->PlanBotsForTest(); }
			TM->LockInAndResolve();
			for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
			if (!Test.TestFalse(*FString::Printf(TEXT("il turno %d ha finito di risolvere entro 400 tick"),
				Turni + 1), TM->IsResolving()))
			{
				return -1;
			}
			++Turni;

			// Una passata sola sul log, non una per unita'. `LockInAndResolve` ha gia' fatto `Reset()`,
			// quindi il log e' del turno appena risolto.
			TSet<int32> ChiHaColpito;
			for (const FRTTurnLogEntry& Voce : TM->GetTurnLog())
			{
				if (URTTurnLogLibrary::IsDamageInflictedByActor(Voce)) { ChiHaColpito.Add(Voce.UnitId); }
			}

			const TArray<ARTUnit*> Vive = UnitaVive(World);
			const TMap<int32, FTotali> Per = TotaliPerSquadra(Vive);
			for (const ARTUnit* U : Vive)
			{
				const bool bArmato = ChiHaColpito.Contains(U->StableUnitId);
				if (bArmato) { ++OutTurniArmati; }
				Sonda.Observe(U->StableUnitId, U->Cell, bArmato, NemiciDi(Per, U->TeamId));
			}
		}
		return Turni;
	}
}

/**
 * **Le sei definizioni sulla MAPPA D'AUTORE** — la board di `NobodyParksOnTheAuthoredMap`, che usa la (a).
 *
 * ⛔ Nessuna asserzione confronta le sequenze con un limite: il limite si stampa perche' il numero si legga,
 * e sceglierlo e' `PDR-00`. Le due asserzioni qui sono guardie di NON-VACUITA' e un invariante fra
 * definizioni — nessuna delle due dice quanti turni fermi siano accettabili.
 *
 * ⚠️ **Verificato per mutazione**: forzando `bArmato` a `false` nel ciclo di `Gioca` — cioe' spegnendo il
 * classificatore del danno — la guardia sui turni armati cade. E' la stessa forma di guardia che
 * `EngagesOnTheGeneratedTestArena` porta per `#1602`: senza, un classificatore bloccato su falso renderebbe
 * ogni definizione identica alla (b) **senza che nulla suoni**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStallDefinitionsAuthoredMapTest,
	"RefactorTactics.Bot.StallDefinitionsOnTheAuthoredMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStallDefinitionsAuthoredMapTest::RunTest(const FString&)
{
	URTHexMapAsset* Authored = RTAuthoredArena::Load();
	if (!TestNotNull(TEXT("la mappa d'autore si carica"), Authored)) { return false; }

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	RTStallMisura::FScopedCVars Guard;

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

	FRTStallDefinitionProbe Sonda;
	int32 TurniArmati = 0;
	const int32 MaxTurni = 12;
	const int32 Turni = RTStallMisura::Gioca(*this, World, TM, Sonda, MaxTurni,
		/*bPianificaEsplicito=*/ false, TurniArmati);
	if (Turni < 0) { RTWorldFixtures::DestroyWorld(World); return false; }

	// `RoundLimit / 3`: la soglia che l'oracolo di questa board usa. Stampata, non asserita.
	RTStallMisura::Riporta(*this, TEXT("mappa d'autore"), Sonda, Turni, FMath::Max(1, MaxTurni / 3));

	TestTrue(FString::Printf(TEXT("premessa: la partita ha prodotto turni e unita' (%d turni, %d unita')"),
		Turni, Sonda.UnitaOsservate()), Turni > 0 && Sonda.UnitaOsservate() > 0);
	TestTrue(FString::Printf(
		TEXT("il classificatore del danno risponde: %d turni-unita' armati in %d turni"),
		TurniArmati, Turni), TurniArmati > 0);
	RTStallMisura::VerificaOrdinamento(*this, TEXT("mappa d'autore"), Sonda);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **Le sei definizioni sull'ARENA GENERATA** — la board di `EngagesOnTheGeneratedTestArena`, che usa la (b),
 * e la configurazione su cui `OPEN_DECISIONS` registra il margine **zero**: 4 su soglia 4.
 *
 * ⛔ Come sopra: si stampa, non si giudica. Il rango del `AddWarning` di quell'oracolo non e' toccato —
 * alzarlo sarebbe una soglia nuova introdotta di lato, che `#1551` mette esplicitamente fuori scope.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStallDefinitionsGeneratedArenaTest,
	"RefactorTactics.Bot.StallDefinitionsOnTheGeneratedTestArena",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStallDefinitionsGeneratedArenaTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	RTStallMisura::FScopedCVars Guard;
	CVarRTAutobattle->Set(-1, ECVF_SetByCode);
	// Modalita' dalla PROPRIETA' del GameMode, non da CVar o riga di comando: e' cio' che si misura.

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("TurnManager"), TM)
		|| !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// I due Warning dell'allestimento sono la scelta dichiarata dell'oracolo gemello, non un difetto — e
	// vanno attesi PRIMA che l'azione li produca.
	AddExpectedError(TEXT("MapSource=GeneratedTestArena"), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedError(TEXT("AUTOBATTLE"), EAutomationExpectedErrorFlags::Contains, 1);

	GameMode->MapSource = ERTMapSource::GeneratedTestArena;
	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);

	if (!TestNotNull(TEXT("il GameMode ha prodotto una mappa"), HexMap->MapAsset.Get()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	FRTStallDefinitionProbe Sonda;
	int32 TurniArmati = 0;
	// **40 come l'oracolo**: e' un tetto di SICUREZZA, non una regola — la partita finisce per regola, e
	// arrivarci sarebbe il difetto. Con 12 la partita veniva troncata a meta' e la misura era di un'altra.
	const int32 MaxTurni = 40;
	const int32 Turni = RTStallMisura::Gioca(*this, World, TM, Sonda, MaxTurni,
		/*bPianificaEsplicito=*/ true, TurniArmati);
	if (Turni < 0) { RTWorldFixtures::DestroyWorld(World); return false; }

	const int32 RoundLimit = TM->GetMatchRules().RoundLimit;
	RTStallMisura::Riporta(*this, TEXT("arena generata"), Sonda, Turni, FMath::Max(2, RoundLimit / 3));

	TestTrue(FString::Printf(TEXT("premessa: la partita ha prodotto turni e unita' (%d turni, %d unita')"),
		Turni, Sonda.UnitaOsservate()), Turni > 0 && Sonda.UnitaOsservate() > 0);
	TestTrue(FString::Printf(
		TEXT("il classificatore del danno risponde: %d turni-unita' armati in %d turni"),
		TurniArmati, Turni), TurniArmati > 0);
	RTStallMisura::VerificaOrdinamento(*this, TEXT("arena generata"), Sonda);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
