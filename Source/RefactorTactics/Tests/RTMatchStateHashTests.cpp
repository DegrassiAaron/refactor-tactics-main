#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMatchStateHash.h"
#include "Unit/RTUnit.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "ScenarioHarness/RTTestResult.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il checksum di fine partita copre l'AMBIENTE, non solo le unità (CP 12.1, issue #81).
 *
 * Il DoD chiedeva che coprisse «stati, terreni modificati, strutture e progresso degli obiettivi (non solo le
 * posizioni)». Non era così: `RTScenarioSession` mescolava, per ogni unità, cella / HP / scudo / energia /
 * vivo-morto e nient'altro. Due partite che finivano con le stesse unità nelle stesse condizioni davano lo
 * stesso hash **anche se una lasciava il campo in fiamme e l'altra no** — e un corpus golden costruito su quel
 * checksum (CP 12.6, #178) sarebbe nato con quel punto cieco dentro.
 *
 * L'ordine è la parte delicata: gli stati vivono in `TMap`/`TSet`, la cui iterazione non è deterministica
 * (invariante #4). Entrano ordinati, e l'ultimo test di questo file è quello che lo dimostra.
 */
namespace
{
	/** Mappa minima: esagono di raggio 1, tutte celle `Floor`. */
	URTHexMapAsset* MakeStateHashMap()
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), /*Radius*/ 1))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	/** Una sola unità viva, senza stati: la base da cui ogni caso si discosta di UNA cosa sola. */
	TArray<FRTUnitStateDigest> BaseUnits()
	{
		FRTUnitStateDigest U;
		U.UnitId = 1;
		U.Cell = FRTCellId(0, 0);
		U.Health = 100;
		U.Shield = 0;
		U.Energy = 25;
		U.bAlive = true;
		return { U };
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChecksumCoversEnvironmentTest,
	"RefactorTactics.Simulation.ChecksumCoversEnvironment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChecksumCoversEnvironmentTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeStateHashMap();
	const TArray<FRTUnitStateDigest> Units = BaseUnits();
	const TArray<int32> NoScore = { 0, 0 };

	const uint32 Baseline = URTMatchStateHashLibrary::HashMatchState(Map, Units, NoScore);

	// Riferimento: lo stesso stato dà lo stesso hash. Senza, tutto il resto del test non significherebbe nulla.
	TestEqual(TEXT("stesso stato -> stesso hash"),
		URTMatchStateHashLibrary::HashMatchState(Map, Units, NoScore), Baseline);

	// 1. TERRENO MODIFICATO: la stessa cella prende fuoco. Le unità non cambiano di una virgola.
	{
		URTHexMapAsset* Burning = MakeStateHashMap();
		FRTHexCellData Cell = *Burning->FindCell(FRTCellId(1, 0));
		Cell.Surface = ERTHexSurface::Fire;
		Burning->AddOrUpdateCell(Cell);
		Burning->SortCells();

		TestNotEqual(TEXT("una cella in fiamme cambia il checksum"),
			URTMatchStateHashLibrary::HashMatchState(Burning, Units, NoScore), Baseline);
	}

	// 2. STRUTTURE: una copertura eretta su un bordo. È il caso di CP 9.5 — le coperture si erigono e si
	//    spostano in partita, quindi due finali con e senza riparo non sono lo stesso finale.
	{
		URTHexMapAsset* Covered = MakeStateHashMap();
		URTHexCoverLibrary::AddCover(Covered, FRTCellId(0, 0), ERTHexDirection::E, ERTHexCoverType::Low, 30);

		TestNotEqual(TEXT("una copertura eretta cambia il checksum"),
			URTMatchStateHashLibrary::HashMatchState(Covered, Units, NoScore), Baseline);
	}

	// 3. STATI TEMPORANEI sull'unità: stessa cella, stessi HP, ma una è rallentata.
	{
		TArray<FRTUnitStateDigest> Slowed = Units;
		Slowed[0].Statuses.Add(FName(TEXT("Status.Slow")));

		TestNotEqual(TEXT("uno stato temporaneo cambia il checksum"),
			URTMatchStateHashLibrary::HashMatchState(Map, Slowed, NoScore), Baseline);
	}

	// 4. PROGRESSO OBIETTIVI: nessuno si è mosso, ma una squadra ha segnato.
	{
		const TArray<int32> Scored = { 1, 0 };
		TestNotEqual(TEXT("il progresso obiettivo cambia il checksum"),
			URTMatchStateHashLibrary::HashMatchState(Map, Units, Scored), Baseline);
	}

	// 5. INVARIANTE #4: gli stati arrivano da `TMap`/`TSet`, la cui iterazione non è deterministica. Due
	//    esecuzioni identiche non devono dare hash diversi solo perché i tag sono stati enumerati in un altro
	//    ordine — è esattamente il difetto che un checksum dovrebbe scoprire, non introdurre.
	{
		TArray<FRTUnitStateDigest> OneOrder = Units;
		OneOrder[0].Statuses = { FName(TEXT("Status.Slow")), FName(TEXT("Status.Burning")) };

		TArray<FRTUnitStateDigest> OtherOrder = Units;
		OtherOrder[0].Statuses = { FName(TEXT("Status.Burning")), FName(TEXT("Status.Slow")) };

		TestEqual(TEXT("permutare gli stati non cambia il checksum"),
			URTMatchStateHashLibrary::HashMatchState(Map, OneOrder, NoScore),
			URTMatchStateHashLibrary::HashMatchState(Map, OtherOrder, NoScore));

		// E due stati DIVERSI restano distinguibili: l'ordinamento non deve degenerare in «tutti uguali».
		TArray<FRTUnitStateDigest> Different = Units;
		Different[0].Statuses = { FName(TEXT("Status.Burning")), FName(TEXT("Status.Rooted")) };
		TestNotEqual(TEXT("stati diversi -> hash diversi"),
			URTMatchStateHashLibrary::HashMatchState(Map, OneOrder, NoScore),
			URTMatchStateHashLibrary::HashMatchState(Map, Different, NoScore));
	}

	return true;
}

/**
 * Il checksum della PARTITA GIOCATA vede la mappa (CP 12.1, issue #81) — copertura del CABLAGGIO.
 *
 * `ChecksumCoversEnvironment` prova la libreria; questo prova che la sessione gliela passi davvero. La
 * distinzione non è accademica: con il cablaggio mutato in `HashMatchState(nullptr, ...)` la suite restava
 * **interamente verde**. È il difetto ricorrente di questo repository — libreria coperta, cablaggio scoperto.
 *
 * `Action.CreateCover` è l'azione giusta per dimostrarlo perché cambia la MAPPA lasciando identiche le
 * UNITÀ: nessuno si muove, nessuno perde vita. Se l'hash ignorasse la mappa, i due finali sarebbero
 * indistinguibili — ed è esattamente ciò che succedeva.
 */
namespace
{
	UWorld* MakeWiringWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyWiringWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Un turno, due unità ferme. Con `bErectCover` Riktor erige un pannello sul proprio bordo E. */
	FRTTestScenario MakeCoverWiringScenario(bool bErectCover)
	{
		FRTTestScenario S;
		S.ScenarioId = TEXT("Internal.ChecksumMapWiring");
		S.MapRadius = 3;

		FRTScenarioUnit Riktor;
		Riktor.Id = TEXT("B1");
		Riktor.HeroId = FName(TEXT("Hero.Riktor"));
		Riktor.TeamId = 0;
		Riktor.Cell = FRTCellId(0, 0);
		S.Units.Add(Riktor);

		// Un avversario lontano e inerte: serve solo perché la partita non finisca per eliminazione prima di
		// arrivare al digest.
		FRTScenarioUnit Foe;
		Foe.Id = TEXT("V1");
		Foe.HeroId = FName(TEXT("Hero.Wraith"));
		Foe.TeamId = 1;
		Foe.Cell = FRTCellId(3, 0);
		S.Units.Add(Foe);

		FRTScenarioTurn Turn;
		if (bErectCover)
		{
			FRTScenarioIntent Erect;
			Erect.UnitId = TEXT("B1");
			Erect.Ability = FName(TEXT("Bastion.KineticPanel"));
			Erect.TargetCell = FRTCellId(0, 0);
			Erect.bTargetsCell = true;
			Erect.CoverEdge = ERTHexDirection::E;
			Erect.bHasCoverEdge = true;
			Turn.Intents.Add(Erect);
			Turn.Requires.Add(TEXT("CreateCover"));
		}
		S.Turns.Add(Turn);

		// L'harness RIFIUTA uno scenario senza assertion — «passerebbe sempre» — ed e' una guardia giusta.
		// Questa e' vera in entrambe le varianti: nessuno si muove, ed e' esattamente il punto del test. Se un
		// giorno smettesse di esserlo, i due hash differirebbero per le UNITA' e non per la mappa, e il test
		// direbbe di aver verificato una cosa che non ha verificato.
		FRTTestExpectation Still;
		Still.Kind = ERTAssertionKind::UnitAtCell;
		Still.UnitId = TEXT("B1");
		Still.Cell = FRTCellId(0, 0);
		S.Expect.Add(Still);

		FRTTestExpectation FoeStill;
		FoeStill.Kind = ERTAssertionKind::UnitAtCell;
		FoeStill.UnitId = TEXT("V1");
		FoeStill.Cell = FRTCellId(3, 0);
		S.Expect.Add(FoeStill);

		return S;
	}

	uint32 RunAndHash(FAutomationTestBase& Test, bool bErectCover, bool& bOutOk)
	{
		bOutOk = false;
		UWorld* World = MakeWiringWorld();
		if (!World)
		{
			return 0;
		}
		const FRTTestResult Result = URTScenarioRunner::Run(World, MakeCoverWiringScenario(bErectCover));
		DestroyWiringWorld(World);

		// Uno scenario BLOCKED o in errore darebbe due hash uguali per il motivo sbagliato, e il test
		// passerebbe da verde a verde senza aver verificato nulla.
		if (Result.Outcome == ERTTestOutcome::Error || Result.Outcome == ERTTestOutcome::Blocked)
		{
			Test.AddError(FString::Printf(TEXT("scenario non eseguibile (%s): %s%s"),
				*Result.OutcomeString(), *Result.ErrorMessage, *Result.BlockedReason));
			return 0;
		}
		bOutOk = true;
		return Result.StateHash;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChecksumSeesMapWiringTest,
	"RefactorTactics.Simulation.ChecksumSeesMapInPlayedScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChecksumSeesMapWiringTest::RunTest(const FString&)
{
	bool bPlainOk = false;
	bool bCoverOk = false;

	const uint32 Plain = RunAndHash(*this, /*bErectCover*/ false, bPlainOk);
	const uint32 WithCover = RunAndHash(*this, /*bErectCover*/ true, bCoverOk);

	if (!TestTrue(TEXT("entrambe le partite sono eseguibili"), bPlainOk && bCoverOk))
	{
		return false;
	}

	// Le unità finiscono identiche in entrambe: l'unica differenza è il pannello sulla mappa. Se il digest
	// non ricevesse la mappa, i due hash coinciderebbero.
	TestNotEqual(TEXT("una copertura eretta in partita cambia il checksum"), WithCover, Plain);
	return true;
}

// `BuildUnitDigests` e' la SOLA costruzione del digest, usata dall'harness e dalla partita (D-084). Due
// costruzioni divergenti darebbero hash diversi per lo stesso stato, e nessuno se ne accorgerebbe finche'
// qualcuno non prova a confrontare un corpus con una partita vera.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDigestUsesStableUnitIdTest,
	"RefactorTactics.Simulation.DigestUsesStableUnitIdAndKeepsTheDead",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDigestUsesStableUnitIdTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	// Il contesto va registrato come fa ogni altro test del repository: qui sarebbe probabilmente inerte —
	// si spawna e si legge, senza toccare il mondo — ma «probabilmente» e' il motivo per cui una convenzione
	// seguita da venticinque file non si rompe in uno solo.
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(World);

	ARTUnit* Viva = World->SpawnActor<ARTUnit>();
	ARTUnit* Caduta = World->SpawnActor<ARTUnit>();
	if (!Viva || !Caduta) { World->DestroyWorld(false); return false; }

	Viva->StableUnitId = 3;
	Viva->Health = 40;
	Caduta->StableUnitId = 7;
	Caduta->Health = 0; // caduta, ma ancora presente: e' il caso che `bAlive` esiste per rappresentare

	const TArray<ARTUnit*> Unita = { Viva, Caduta };
	const TArray<FRTUnitStateDigest> Digests = URTMatchStateHashLibrary::BuildUnitDigests(Unita);

	if (!TestEqual(TEXT("un digest per unita', morte comprese"), Digests.Num(), 2))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}

	// L'identita' e' `StableUnitId`, non una stringa di scenario: e' l'intero fatto di D-084.
	const bool bTreCiSta = Digests.ContainsByPredicate(
		[](const FRTUnitStateDigest& D) { return D.UnitId == 3 && D.bAlive; });
	const bool bSetteCiSta = Digests.ContainsByPredicate(
		[](const FRTUnitStateDigest& D) { return D.UnitId == 7 && !D.bAlive; });

	TestTrue(TEXT("l'unita' viva porta il proprio StableUnitId"), bTreCiSta);
	TestTrue(TEXT("la caduta c'e', e dichiara di non essere viva"), bSetteCiSta);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	return true;
}

/**
 * L'IDENTITA' DI STRUTTURA entra nel checksum (CP 23.3, #832) — l'ultima casella di quel DoD.
 *
 * `DoorId` c'era gia': distingue i gruppi DENTRO una mappa, ed e' un `int32` locale all'asset. `StableId`
 * (v9 del formato) e' un'altra cosa: e' il nome che sopravvive al `cook` e che uno scenario o un replay
 * possono citare. Se non entrasse nel checksum, due partite in cui la stessa porta e' stata rinominata —
 * cioe' due partite che uno scenario descrive in modo diverso — darebbero lo stesso hash, e il corpus
 * golden nascerebbe con quel punto cieco dentro. E' lo stesso argomento con cui l'ambiente ci e' entrato
 * sopra, applicato all'anello che #832 aggiunge.
 *
 * ⚠️ Il nome entra come TESTO, via `MixName`, mai come indice della name table: l'indice dipende
 * dall'ordine di creazione dei nomi nel processo, e due esecuzioni della stessa partita darebbero due hash
 * diversi (invariante #4). Quella proprieta' **non e' verificabile da qui** — vedi il punto 4 sotto, che
 * dice perche' e dove lo e' invece: fra esecuzioni separate.
 */
namespace
{
	/** La mappa minima, piu' UNA porta sul bordo `E` di `(0,0)`, con l'identita' che il caso vuole. */
	URTHexMapAsset* MapWithDoorNamed(FName StableId)
	{
		URTHexMapAsset* M = MakeStateHashMap();
		FRTHexCellData Cell = *M->FindCell(FRTCellId(0, 0));

		FRTHexDoor Door;
		Door.Edge = ERTHexDirection::E;
		Door.State = ERTHexDoorState::Closed;
		Door.DoorId = 7;              // costante in tutti i casi: la variabile e' SOLO `StableId`
		Door.StableId = StableId;
		Cell.Doors.Add(Door);

		M->AddOrUpdateCell(Cell);
		M->SortCells();
		return M;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChecksumSeesStructureIdentityTest,
	"RefactorTactics.Simulation.ChecksumSeesStructureIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChecksumSeesStructureIdentityTest::RunTest(const FString&)
{
	const TArray<FRTUnitStateDigest> Units = BaseUnits();
	const TArray<int32> NoScore = { 0, 0 };

	auto HashOf = [&Units, &NoScore](const URTHexMapAsset* M)
	{
		return URTMatchStateHashLibrary::HashMatchState(M, Units, NoScore);
	};

	const uint32 Anonima = HashOf(MapWithDoorNamed(NAME_None));

	// Riferimento. Senza questo, i `TestNotEqual` sotto non distinguerebbero un difetto da un rumore.
	TestEqual(TEXT("stessa mappa -> stesso hash"), HashOf(MapWithDoorNamed(NAME_None)), Anonima);

	// 1. NOMINARE una struttura anonima cambia il checksum. E' la casella del DoD, nella sua forma minima.
	const uint32 Nominata = HashOf(MapWithDoorNamed(FName(TEXT("Door.Atrio"))));
	TestNotEqual(TEXT("dare un'identita' a una porta anonima cambia il checksum"), Nominata, Anonima);

	// 2. Due identita' DIVERSE danno hash diversi: non basta che il campo «ci sia», deve contare il nome.
	//    Senza questo caso passerebbe anche un'implementazione che mescola solo `StableId.IsNone()`.
	const uint32 Altra = HashOf(MapWithDoorNamed(FName(TEXT("Door.Cortile"))));
	TestNotEqual(TEXT("due identita' diverse danno checksum diversi"), Altra, Nominata);

	// 3. La stessa identita' da' lo stesso hash: e' un checksum, non un contatore di chiamate.
	TestEqual(TEXT("la stessa identita' da' lo stesso checksum"),
		HashOf(MapWithDoorNamed(FName(TEXT("Door.Atrio")))), Nominata);

	// 4. ⚠️ QUI NON C'E' UN CASO, ED E' UNA SCELTA — non una dimenticanza.
	//
	//    La proprieta' che conta per l'invariante #4 e' che il nome entri come TESTO e non come indice
	//    della name table. Non e' verificabile DENTRO un processo: `FName(TEXT("X"))` restituisce lo
	//    stesso indice per tutta la vita del processo, comunque lo si costruisca, quindi anche un
	//    `Mix(GetTypeHash(Name))` — l'implementazione sbagliata — passerebbe qualunque confronto scritto
	//    qui. Una prima stesura di questo test costruiva gli stessi `FName` in ordine inverso credendo di
	//    falsificarlo: era un caso vacuo, verde per costruzione.
	//
	//    🔴 **E la prima stesura di questa nota rimandava a due coperture che NON esistono.** Diceva
	//    che la proprieta' e' verificata da `RTGoldenCorpusTests` e dal gate `G12`. Misurato dopo, su
	//    segnalazione di una code review: i golden `.rttl` sono tracce di TurnLog e **non contengono
	//    alcuno state hash** (`grep StateHash` sui due file: zero), e `G12` e' `RunUAT BuildCookRun ->
	//    BUILD SUCCESSFUL`, un gate di **packaging** che non confronta hash. Cioe' avevo sostituito un
	//    caso vacuo con un puntatore a una copertura inesistente: lo stesso difetto, spostato dal
	//    codice al commento.
	//
	//    Lo stato vero, oggi: **nessuno verifica che il nome entri come testo e non come indice.** Un
	//    `Mix(GetTypeHash(Name))` passerebbe l'intera suite — anche dopo #986, e vale la pena dire
	//    **perche'**: quella mutazione non e' intercettata da nessuno dei casi nuovi. Il comparison index
	//    di `FName` e' gia' case-insensitive, quindi passerebbe il caso sull'ortografia; ed e' diverso per
	//    nomi diversi, quindi passerebbe anche il confine `{"AB"}` vs `{"A","B"}`. Resta non falsificabile
	//    dentro un processo, per la ragione scritta due paragrafi sopra.
	//
	//    🔵 **Il SECONDO buco invece e' chiuso, e questa riga e' stata riscritta perche' diceva il
	//    contrario.** Fino al merge di #986 qui si leggeva *«entrambi sono aperti su #986 … finche' quella
	//    issue e' aperta, questa riga dice cosa manca»*: la issue e' stata chiusa e la riga e' rimasta a
	//    dichiarare scoperto cio' che nello stesso commit era stato coperto. E' lo stesso difetto che #986
	//    era venuta a chiudere — un commento che sopravvive al lavoro che descrive — trovato su se' stesso
	//    un'ora dopo. `MixName` ora normalizza con `ToLower()` e mescola la lunghezza:
	//    `ChecksumTreatsStructureIdentityCaseInsensitively` e `ChecksumSeesStatusBoundaries` lo pinnano,
	//    ed entrambi cadono se si tocca la funzione (verifica di mutazione in #986).

	// 5. Gli ARCHI, non le sole porte: lo scope di #832 dice «archi e non solo porte, sono lo stesso
	//    problema di identita'». Un ponte nominato e uno anonimo non sono lo stesso stato di mappa.
	//
	//    ⚠️ L'arco ATTRAVERSA I LIVELLI, e non e' un dettaglio di comodo: `(0,0) -> (1,0)` sullo stesso
	//    layer e' classificato **ridondante** dalla validazione dell'asset (`RTHexMapAsset.cpp:461`),
	//    perche' le due celle sono gia' adiacenti. Una prima stesura di questo caso usava proprio quella
	//    forma, cioe' pinnava una geometria che una mappa valida non contiene; i ponti di CP 9.4
	//    attraversano i livelli, ed e' quella la forma da difendere.
	{
		auto ArcoVerso = [](FName StableId)
		{
			URTHexMapAsset* M = MakeStateHashMap();
			FRTHexEdge Arco;
			Arco.From = FRTCellId(0, 0, 0);
			Arco.To = FRTCellId(0, 0, 1);
			Arco.StableId = StableId;
			M->Transitions.Add(Arco);
			return M;
		};

		TestNotEqual(TEXT("dare un'identita' a un arco cambia il checksum"),
			HashOf(ArcoVerso(FName(TEXT("Arc.PonteBasso")))), HashOf(ArcoVerso(NAME_None)));

		// 5-bis. DUE NOMI DIVERSI, che e' la cosa che il caso sopra da solo non prova.
		//        `Mix(Arc.StableId.IsNone() ? 0 : 1)` — l'implementazione debole che il caso 2 esclude
		//        esplicitamente per le porte — passerebbe il `TestNotEqual` qui sopra e cadrebbe qui.
		//        Senza questa riga la verifica di mutazione su `MixName(Arc.StableId)` non ha un test
		//        capace di falsificarla, che e' il difetto misurato da #986.
		TestNotEqual(TEXT("due archi con nomi diversi danno checksum diversi"),
			HashOf(ArcoVerso(FName(TEXT("Arc.PonteAlto")))),
			HashOf(ArcoVerso(FName(TEXT("Arc.PonteBasso")))));
	}

	return true;
}

/**
 * Il checksum tratta l'identita' di una struttura come la trattano i suoi CONSUMATORI: senza distinguere
 * maiuscole e minuscole (#986, difetto 2).
 *
 * `FName::operator==` e' case-insensitive, e `URTStructureIdentityLibrary` risolve i bersagli proprio con
 * quello (`RTStructureIdentityLibrary.cpp:21` e `:42`): per ogni consumatore `Door.Atrio` e `door.atrio`
 * sono **la stessa porta**. Se il checksum le distingue, due mappe che si giocano identiche hanno hash
 * diversi — un falso positivo contro il KPI `replay divergence = 0`.
 *
 * ⚠️ **E il difetto e' peggiore in packaged che nell'editor.** `MixName` mescolava `FName::ToString()`, e
 * `WITH_CASE_PRESERVING_NAME` vale `WITH_EDITORONLY_DATA` (`NameTypes.h:33`): in una build cotta quel
 * metodo restituisce il case della **prima ortografia registrata nel processo**, che dipende dall'ordine
 * di caricamento dei package. Cioe' la stessa identica partita puo' dare due hash diversi in due
 * esecuzioni — precisamente cio' che il commento accanto a `MixName` prometteva di escludere.
 *
 * Questo test lo falsifica **nell'editor**, dove il case invece e' preservato: e' l'unico ambiente in cui
 * la differenza e' osservabile in modo deterministico, ed e' la ragione per cui il caso si scrive cosi'
 * invece di provare a simulare l'ordine di caricamento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChecksumIsCaseInsensitiveTest,
	"RefactorTactics.Simulation.ChecksumTreatsStructureIdentityCaseInsensitively",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChecksumIsCaseInsensitiveTest::RunTest(const FString&)
{
	const TArray<FRTUnitStateDigest> Units = BaseUnits();
	const TArray<int32> NoScore = { 0, 0 };

	// La PREMESSA del test, asserita invece che assunta: se i due nomi non fossero la stessa `FName` per
	// il gioco, pretendere lo stesso hash sarebbe sbagliato, non giusto.
	TestTrue(TEXT("premessa: per il gioco i due nomi sono la stessa struttura"),
		FName(TEXT("Door.Atrio")) == FName(TEXT("door.atrio")));

	auto HashOf = [&Units, &NoScore](const URTHexMapAsset* M)
	{
		return URTMatchStateHashLibrary::HashMatchState(M, Units, NoScore);
	};

	TestEqual(TEXT("l'ortografia dell'identita' non cambia il checksum"),
		HashOf(MapWithDoorNamed(FName(TEXT("door.atrio")))),
		HashOf(MapWithDoorNamed(FName(TEXT("Door.Atrio")))));

	// E il caso NEGATIVO nello stesso test: normalizzare non deve appiattire nomi davvero diversi. Senza
	// questa riga passerebbe anche un `MixName` che ignora del tutto il testo.
	TestNotEqual(TEXT("due nomi diversi restano diversi dopo la normalizzazione"),
		HashOf(MapWithDoorNamed(FName(TEXT("Door.Cortile")))),
		HashOf(MapWithDoorNamed(FName(TEXT("Door.Atrio")))));

	return true;
}

/**
 * Gli stati di un'unita' entrano nel checksum con il proprio CONFINE, non come una sequenza di caratteri
 * concatenati (#986, difetto 3).
 *
 * `MixName` mescolava i caratteri senza prefisso di lunghezza ne' separatore, quindi un'unita' con
 * `{"AB"}` e una con `{"A","B"}` producevano la stessa sequenza — due stati di gioco diversi, un solo
 * checksum. Non e' un caso di laboratorio: gli `Statuses` arrivano da `ARTUnit::GetActiveStatusNames()`,
 * e i tag del catalogo condividono i prefissi (`Status.Burn`, `Status.Burning`).
 *
 * ⚠️ Il difetto era **preesistente** a #978 — `MixName` serviva gia' gli stati — ma la PR #978 l'ha esteso
 * a `Door.StableId` e `Arc.StableId` asserendone la correttezza, che e' il modo in cui una lacuna
 * sopravvive a una revisione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChecksumSeesStatusBoundariesTest,
	"RefactorTactics.Simulation.ChecksumSeesStatusBoundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChecksumSeesStatusBoundariesTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeStateHashMap();
	const TArray<int32> NoScore = { 0, 0 };

	TArray<FRTUnitStateDigest> Unito = BaseUnits();
	Unito[0].Statuses = { FName(TEXT("AB")) };

	TArray<FRTUnitStateDigest> Diviso = BaseUnits();
	Diviso[0].Statuses = { FName(TEXT("A")), FName(TEXT("B")) };

	TestNotEqual(TEXT("uno stato \"AB\" non e' due stati \"A\" e \"B\""),
		URTMatchStateHashLibrary::HashMatchState(Map, Diviso, NoScore),
		URTMatchStateHashLibrary::HashMatchState(Map, Unito, NoScore));

	// Lo stesso confine sull'identita' delle strutture, che usa la stessa funzione: due porte adiacenti
	// nominate `Door.A` e `Door.B` non devono collidere con una sola porta `Door.AB`.
	TestNotEqual(TEXT("il confine vale anche per l'identita' di una struttura"),
		URTMatchStateHashLibrary::HashMatchState(MapWithDoorNamed(FName(TEXT("Door.A"))), Unito, NoScore),
		URTMatchStateHashLibrary::HashMatchState(MapWithDoorNamed(FName(TEXT("Door.AB"))), Unito, NoScore));

	return true;
}

/**
 * I DUE hash vedono l'identita' di un ARCO, non solo quella di una porta (#986, difetto 1).
 *
 * `ChecksumSeesStructureIdentity` copre `HashMatchState`; `HexMap.DoorHashDeterminism` copre
 * `URTHexMapAsset::ComputeHash` per le **porte**. Restava scoperto l'incrocio: `ComputeHash` sugli
 * **archi**, che e' esattamente il tipo di buco che #986 e' venuta a chiudere — #832 aveva aggiunto
 * l'identita' a un hash e spuntato il DoD con un test che guardava l'altro.
 *
 * Il test sta qui e non in `RTHexArcTests.cpp` perche' la proprieta' che verifica e' la **coerenza fra i
 * due hash**, cioe' il tema di questo file, e perche' quel file non appartiene al write-set di questa
 * track (D-139). Chi lo legge cercando i ponti trovera' il rimando dal caso 5 di sopra.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapHashSeesArcIdentityTest,
	"RefactorTactics.Simulation.MapHashSeesArcIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapHashSeesArcIdentityTest::RunTest(const FString&)
{
	auto MappaConArco = [](FName StableId)
	{
		URTHexMapAsset* M = MakeStateHashMap();
		FRTHexEdge Ponte;
		Ponte.From = FRTCellId(0, 0, 0);
		Ponte.To = FRTCellId(0, 0, 1);
		Ponte.StableId = StableId;
		M->Transitions.Add(Ponte);
		return M;
	};

	const uint32 Anonimo = MappaConArco(NAME_None)->ComputeHash();

	// Riferimento: senza, i confronti sotto non distinguerebbero un difetto da un rumore.
	TestEqual(TEXT("stesso arco -> stesso hash di mappa"), MappaConArco(NAME_None)->ComputeHash(), Anonimo);

	TestNotEqual(TEXT("nominare un arco cambia l'hash della mappa"),
		MappaConArco(FName(TEXT("Arc.PonteBasso")))->ComputeHash(), Anonimo);

	TestNotEqual(TEXT("due archi con nomi diversi danno hash di mappa diversi"),
		MappaConArco(FName(TEXT("Arc.PonteAlto")))->ComputeHash(),
		MappaConArco(FName(TEXT("Arc.PonteBasso")))->ComputeHash());

	TestEqual(TEXT("l'ortografia dell'identita' di un arco non cambia l'hash"),
		MappaConArco(FName(TEXT("arc.pontebasso")))->ComputeHash(),
		MappaConArco(FName(TEXT("Arc.PonteBasso")))->ComputeHash());

	return true;
}

/**
 * La CONDUTTIVITA' di un arco entra nel checksum (#986, difetto 4).
 *
 * Il giro degli archi mescolava `Cost`, `Kind`, `State`, `Integrity` e `StableId` — non
 * `bConductsElectricity`. `URTHexMapAsset::ComputeHash` invece lo mescola gia'
 * (`RTHexMapAsset.cpp:314`), chiamandolo «dato autorevole quanto il costo»: i due hash divergevano su un
 * campo senza che nessuno l'avesse deciso.
 *
 * ⚠️ **Non e' teorico.** `ARTTurnManager` crea ponti IN PARTITA con `bConductsElectricity = true`
 * (`RTTurnManager.cpp:3802-3806`) e `URTHexArcLibrary` legge quel flag per far risalire la scarica lungo
 * l'arco (`RTHexArcLibrary.cpp:75`). Due finali che differiscono solo per la conduttivita' di un ponte
 * sono due stati di gioco diversi con esiti futuri diversi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChecksumSeesArcConductivityTest,
	"RefactorTactics.Simulation.ChecksumSeesArcConductivity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChecksumSeesArcConductivityTest::RunTest(const FString&)
{
	const TArray<FRTUnitStateDigest> Units = BaseUnits();
	const TArray<int32> NoScore = { 0, 0 };

	// Un ponte fra due livelli: e' la forma che CP 9.4 produce, e l'unica che la validazione non
	// classifica ridondante.
	auto PonteConduttivo = [](bool bConduce)
	{
		URTHexMapAsset* M = MakeStateHashMap();
		FRTHexEdge Ponte;
		Ponte.From = FRTCellId(0, 0, 0);
		Ponte.To = FRTCellId(0, 0, 1);
		Ponte.StableId = FName(TEXT("Arc.PonteBasso"));
		Ponte.bConductsElectricity = bConduce;
		M->Transitions.Add(Ponte);
		return M;
	};

	const uint32 Isolante = URTMatchStateHashLibrary::HashMatchState(PonteConduttivo(false), Units, NoScore);

	// Riferimento: senza, il `TestNotEqual` sotto non distinguerebbe un difetto da un rumore.
	TestEqual(TEXT("stesso ponte -> stesso checksum"),
		URTMatchStateHashLibrary::HashMatchState(PonteConduttivo(false), Units, NoScore), Isolante);

	TestNotEqual(TEXT("un ponte conduttivo non e' lo stesso stato di uno isolante"),
		URTMatchStateHashLibrary::HashMatchState(PonteConduttivo(true), Units, NoScore), Isolante);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
