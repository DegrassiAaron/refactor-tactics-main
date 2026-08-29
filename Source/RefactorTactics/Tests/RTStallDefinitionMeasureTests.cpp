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
// ## LA MISURA — 2026-08-29, worktree `wt-dir-c-v02`, run dichiarata VALIDA
//
//     definizione                     mappa d'autore   arena generata
//     (b) immobilita'                       4                4        <- CONTROLLO
//     (a) immobilita' sterile               3                2
//     (c) salute netta                      3                2
//     (c) pool netto (salute+scudo)         3                2        <- IDENTITA' di salute netta
//     (c) salute o eliminazione             3                2        <- IDENTITA' di salute netta
//     (c) salute per turno                  3                2        <- coincidenza MISURATA
//     (c) eliminazione                      4                4        <- coincide con la (b)
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
// regola), `NobodyParksOnTheAuthoredMap` non lo chiama e si ferma a 12 — e specchiarli e' la condizione
// perche' il numero parli della board.
//
// 🔴 **DUE RIGHE SU SETTE SONO IDENTITA', NON MISURE — e la prima stesura le contava come conferme.**
// `pool netto` e `salute o eliminazione` non possono differire da `salute netta`: al punto di campionamento
// il Cleanup ha appena rimesso `Shield = BaseShield` su ogni unita' viva, quindi
// `Pool == Salute + BaseShield * Vivi` e `bPoolCalato ⟺ bSaluteCalata`. L'identita' e' **asserita a ogni
// osservazione** dai due test di board, non lasciata all'algebra. `salute per turno` invece **non** e'
// forzata a coincidere: coincide, e quello e' un dato. Trovato in code review su `#1645`.
//
// 🔴 **PERCHE' coincidono, ed e' cio' che vale piu' dei numeri.** La (c) puo' separarsi dalla (a) SOLO
// quando un colpo e' interamente assorbito — armata, ma senza far calare la salute. In v0.1 non accade
// mai: l'attacco base fa **20-28** (`BasicAttackDamageForRange`) contro uno scudo base di **5**. L'unico
// assorbitore e' lo scudo temporaneo **25** di `Action.Shield`, che il difensore paga con l'azione
// principale ogni due turni, e nessuna delle due partite lo produce.
//
// ∴ questi numeri non sono una proprieta' delle due board: sono una proprieta' dei **numeri di
// bilanciamento**, e si muovono con `#149`. La (c) costa una finestra di HP per unita' e **una soglia
// nuova** — materia di `D-184`, non di un test — e su questi dati non compra nessun verdetto diverso da
// quello che la (a) gia' da'.
//
// ⚠️ **LIMITE, e per scelta.** Non e' misurata la (c) in uno scenario COSTRUITO dove puo' separarsi
// (bersaglio che si scuda, attaccante fermo che spara dentro lo scudo). Non e' una lacuna del mandato: il
// condizionale di `BOT-STALL-1` chiedeva la misura la' dove il bot gioca, e questa e' quella.

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Combat/RTCombatLibrary.h" // BaseShield: l'identita' del pool si asserisce, non si assume
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
	 *
	 * ⚠️ **Cosa NON puo' cogliere, e va detto perche' altrimenti si legge come una verifica dei numeri**:
	 * l'annidamento dei predicati in `Avanza` rende questa disuguaglianza vera per **qualunque** input,
	 * quindi nessuna board — nessun bot, nessuna mappa, nessun bilanciamento — puo' farla diventare rossa.
	 * Cade in un caso solo: se qualcuno cambia un predicato di `Avanza` rompendo l'annidamento. E' una
	 * guardia sulle DEFINIZIONI, non una misura sui dati (#1655).
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
	/**
	 * Le guardie che la code review di `#1645` ha chiesto, e che senza di lei mancavano.
	 *
	 * 🔴 `ViolazioniPool` e' la piu' importante: al punto di campionamento vale `Pool == Salute +
	 * BaseShield * Vivi` per costruzione (Cleanup: `ExpireTemporaryShield` poi `RechargeBaseShield`), ed e'
	 * la ragione per cui `PoolNetto` e `SaluteOEliminazione` NON possono differire da `SaluteNetta`.
	 * Asserirla rende quel collasso un fatto misurato invece che un'algebra riportata a mano — e se una
	 * meccanica futura tocca lo scudo fuori dal Cleanup, questo diventa rosso e il collasso va rifatto.
	 *
	 * ⚠️ `ChiaviDistinte` sorveglia il collasso delle chiavi: `StableUnitId` vale 0 finche' il roster non lo
	 * assegna, e quattro unita' con la stessa chiave darebbero numeri plausibili e falsi con tutto verde.
	 */
	struct FGuardie
	{
		int32 Osservazioni = 0;
		int32 ViolazioniPool = 0;
		int32 MinChiaviDistinte = TNumericLimits<int32>::Max();
		int32 MinVive = TNumericLimits<int32>::Max();
		int32 UmaneViste = 0;
	};

	int32 Gioca(FAutomationTestBase& Test, UWorld* World, ARTTurnManager* TM,
		FRTStallDefinitionProbe& Sonda, int32 MaxTurni, bool bPianificaEsplicito, int32& OutTurniArmati,
		FGuardie& G)
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
			TSet<int32> ChiaviViste;
			for (const ARTUnit* U : Vive)
			{
				ChiaviViste.Add(U->StableUnitId);
				if (!U->bIsBotControlled) { ++G.UmaneViste; }
			}
			G.MinChiaviDistinte = FMath::Min(G.MinChiaviDistinte, ChiaviViste.Num());
			G.MinVive = FMath::Min(G.MinVive, Vive.Num());
			const TMap<int32, FTotali> Per = TotaliPerSquadra(Vive);
			for (const ARTUnit* U : Vive)
			{
				const bool bArmato = ChiHaColpito.Contains(U->StableUnitId);
				if (bArmato) { ++OutTurniArmati; }
				const FRTStallDefinitionProbe::FStatoNemico Nemici = NemiciDi(Per, U->TeamId);
				++G.Osservazioni;
				if (Nemici.Pool != Nemici.Salute + URTCombatLibrary::BaseShield * Nemici.Vivi)
				{
					++G.ViolazioniPool;
				}
				Sonda.Observe(U->StableUnitId, U->Cell, bArmato, Nemici);
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
	RTStallMisura::FGuardie Guardie;
	const int32 MaxTurni = 12;
	const int32 Turni = RTStallMisura::Gioca(*this, World, TM, Sonda, MaxTurni,
		/*bPianificaEsplicito=*/ false, TurniArmati, Guardie);
	if (Turni < 0) { RTWorldFixtures::DestroyWorld(World); return false; }

	// `RoundLimit / 3`: la soglia che l'oracolo di questa board usa. Stampata, non asserita.
	RTStallMisura::Riporta(*this, TEXT("mappa d'autore"), Sonda, Turni, FMath::Max(1, MaxTurni / 3));

	TestTrue(FString::Printf(TEXT("premessa: la partita ha prodotto turni e unita' (%d turni, %d unita')"),
		Turni, Sonda.UnitaOsservate()), Turni > 0 && Sonda.UnitaOsservate() > 0);
	TestTrue(FString::Printf(
		TEXT("il classificatore del danno risponde: %d turni-unita' armati in %d turni"),
		TurniArmati, Turni), TurniArmati > 0);

	// 🔴 **L'identita' del pool, asserita e non assunta** (code review di `#1645`). Al punto di
	// campionamento il Cleanup ha appena rimesso `Shield = BaseShield` su ogni unita' viva, quindi
	// `Pool == Salute + BaseShield * Vivi`. E' cio' che rende `PoolNetto` e `SaluteOEliminazione`
	// IDENTITA' di `SaluteNetta` invece che conferme indipendenti. Se cade, il collasso va rimisurato.
	// ⚠️ Verificato per mutazione: sommando 1 al `Pool` in `TotaliPerSquadra` questa riga cade.
	TestEqual(FString::Printf(TEXT("Pool == Salute + %d * Vivi su tutte le %d osservazioni"),
		URTCombatLibrary::BaseShield, Guardie.Osservazioni), Guardie.ViolazioniPool, 0);

	// Le chiavi non collassano: quattro unita' con lo stesso `StableUnitId` darebbero numeri plausibili e
	// falsi, con ogni asserzione verde.
	TestEqual(FString::Printf(TEXT("chiavi distinte quante le unita' vive (%d su %d)"),
		Guardie.MinChiaviDistinte, Guardie.MinVive), Guardie.MinChiaviDistinte, Guardie.MinVive);

	// 🔴 **L'allestimento e' quello previsto, asserito e non assunto** (#1655). `UmaneViste` era raccolto a
	// ogni turno e non guardato da nessuno: un contatore che esiste e che nessuno controlla e' un difetto
	// della stessa famiglia del numero riportato a mano. Se una sola unita' restasse al giocatore, il bot
	// non pianificherebbe per lei e ogni sequenza ferma qui sotto descriverebbe una partita a meta'.
	TestEqual(FString::Printf(TEXT("nessuna unita' umana in campo (%d viste in %d osservazioni)"),
		Guardie.UmaneViste, Guardie.Osservazioni), Guardie.UmaneViste, 0);
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
	RTStallMisura::FGuardie Guardie;
	// **40 come l'oracolo**: e' un tetto di SICUREZZA, non una regola — la partita finisce per regola, e
	// arrivarci sarebbe il difetto. Con 12 la partita veniva troncata a meta' e la misura era di un'altra.
	const int32 MaxTurni = 40;
	const int32 Turni = RTStallMisura::Gioca(*this, World, TM, Sonda, MaxTurni,
		/*bPianificaEsplicito=*/ true, TurniArmati, Guardie);
	if (Turni < 0) { RTWorldFixtures::DestroyWorld(World); return false; }

	const int32 RoundLimit = TM->GetMatchRules().RoundLimit;
	const int32 LimiteInUso = FMath::Max(2, RoundLimit / 3);
	RTStallMisura::Riporta(*this, TEXT("arena generata"), Sonda, Turni, LimiteInUso);

	TestTrue(FString::Printf(TEXT("premessa: la partita ha prodotto turni e unita' (%d turni, %d unita')"),
		Turni, Sonda.UnitaOsservate()), Turni > 0 && Sonda.UnitaOsservate() > 0);

	// 🔴 **IL CONTROLLO DEL BANCO, che l'intestazione dichiarava e nessuno asseriva** (#1655).
	//
	// La (b) su questa board e' il **margine zero** che `OPEN_DECISIONS` registra per `BOT-STALL-1`: la
	// sequenza ferma piu' lunga tocca esattamente la soglia in uso. E' cio' che rende significativi tutti i
	// numeri (c) stampati qui sopra — se la (b) misurata qui non fosse quella che l'oracolo di questa board
	// vede, questo test starebbe descrivendo un'altra partita e il referto sarebbe plausibile e falso.
	//
	// ⚠️ **Un rosso qui NON e' un difetto del bot, ed e' il punto**: significa che il margine registrato in
	// `OPEN_DECISIONS` e' scaduto. Si **rimisura** e si aggiorna la riga — non si ritara la soglia, che e'
	// `D-184` e non materia di un test. Stessa disciplina del grilletto
	// `Bot.ShippedRosterStaysAboveTheBackstepBudget`: una premessa che si muove deve farlo sapere a qualcuno.
	// `EDef` e' un alias locale ai corpi che lo usano piu' volte (vedi i due Meta test): per due sole
	// occorrenze si qualifica, invece di introdurne un terzo.
	const int32 ImmobilitaMisurata =
		Sonda.Peggiore(FRTStallDefinitionProbe::EDefinizione::Immobilita);
	TestEqual(FString::Printf(
		TEXT("controllo: la (b) tocca la soglia in uso (margine zero) — %d su %d"),
		ImmobilitaMisurata, LimiteInUso),
		ImmobilitaMisurata, LimiteInUso);
	TestTrue(FString::Printf(
		TEXT("il classificatore del danno risponde: %d turni-unita' armati in %d turni"),
		TurniArmati, Turni), TurniArmati > 0);

	// 🔴 **L'identita' del pool, asserita e non assunta** (code review di `#1645`). Al punto di
	// campionamento il Cleanup ha appena rimesso `Shield = BaseShield` su ogni unita' viva, quindi
	// `Pool == Salute + BaseShield * Vivi`. E' cio' che rende `PoolNetto` e `SaluteOEliminazione`
	// IDENTITA' di `SaluteNetta` invece che conferme indipendenti. Se cade, il collasso va rimisurato.
	// ⚠️ Verificato per mutazione: sommando 1 al `Pool` in `TotaliPerSquadra` questa riga cade.
	TestEqual(FString::Printf(TEXT("Pool == Salute + %d * Vivi su tutte le %d osservazioni"),
		URTCombatLibrary::BaseShield, Guardie.Osservazioni), Guardie.ViolazioniPool, 0);

	// Le chiavi non collassano: quattro unita' con lo stesso `StableUnitId` darebbero numeri plausibili e
	// falsi, con ogni asserzione verde.
	TestEqual(FString::Printf(TEXT("chiavi distinte quante le unita' vive (%d su %d)"),
		Guardie.MinChiaviDistinte, Guardie.MinVive), Guardie.MinChiaviDistinte, Guardie.MinVive);

	// 🔴 **L'allestimento e' quello previsto, asserito e non assunto** (#1655). `UmaneViste` era raccolto a
	// ogni turno e non guardato da nessuno: un contatore che esiste e che nessuno controlla e' un difetto
	// della stessa famiglia del numero riportato a mano. Se una sola unita' restasse al giocatore, il bot
	// non pianificherebbe per lei e ogni sequenza ferma qui sotto descriverebbe una partita a meta'.
	TestEqual(FString::Printf(TEXT("nessuna unita' umana in campo (%d viste in %d osservazioni)"),
		Guardie.UmaneViste, Guardie.Osservazioni), Guardie.UmaneViste, 0);
	RTStallMisura::VerificaOrdinamento(*this, TEXT("arena generata"), Sonda);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
