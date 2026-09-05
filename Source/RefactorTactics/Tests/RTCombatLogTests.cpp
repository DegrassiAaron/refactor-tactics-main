// CP 11.3 (#79) — il log leggibile DERIVA dal TurnLog, e i due dicono la stessa cosa.
//
// ## Il difetto che questi test misurano, misurato in partita
//
// Dodici turni di partita non presidiata su `L_HexArena`, 151 righe `LogRT` (2026-08-23):
//
//     utility -> ...   (cosa ha deciso il bot)     0 righe
//     ... danni da ... (chi ha colpito chi)        0 righe
//     arma <reazione> / reazione pronta            tutte le altre
//
// Con quel log in mano non si puo' dire se i bot si siano colpiti — che e' esattamente cio' che il DoD
// vieta: *«ogni esito deve essere spiegabile leggendo il log, senza aprire il debugger»*.
//
// ## Perche' un produttore solo
//
// Le righe leggibili nascevano da 59 `AddLogEvent` sparse nella risoluzione, il `TurnLog` nasceva altrove.
// Due produttori indipendenti coincidono per abitudine, non per costruzione: nessun test poteva dire se
// avessero smesso. Ora la sequenza leggibile si DERIVA dal TurnLog, che e' l'autorita'.

#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Core/RTGameplayTags.h"      // TAG_Status_Burning · TAG_Status_Guarded: la guardia si applica al difensore
#include "Perception/RTTeamKnowledge.h" // ClassifyTarget: la premessa «e' un ricordo» si asserisce, non si spera
#include "Turn/RTActionFallbackLibrary.h" // ERTActionInvalidReason nel motivo del fallback
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTCombatLog.h" // URTCombatLogLibrary: il filtro, che non vive piu' nell'Actor
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTCombatLogFixture
{
	UWorld* MakeWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnMap(UWorld* World, int32 Radius = 6)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi: qui si prova il log, non il bot
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeWraith());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	void RunTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/**
	 * Mondo, mappa, TurnManager e due unita' affiancate: il montaggio che tre test di questo file avevano
	 * copiato riga per riga. Estratto invece di farne una quarta copia — e' la stessa duplicazione che
	 * `#1415` ha appena tolto ai builder di arena.
	 *
	 * Restituisce `false` senza aver registrato errori solo se il chiamante ha gia' asserito: i controlli
	 * stanno qui dentro, cosi' un montaggio rotto e' RUMOROSO. Un'uscita muta verrebbe riportata come
	 * Success, perche' l'automation ignora il `bool` di `RunTest`.
	 */
	struct FTwoUnitTurn
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* A = nullptr;
		ARTUnit* B = nullptr;
	};

	bool BuildTwoUnitTurn(FAutomationTestBase& Test, FTwoUnitTurn& Out,
		const FRTCellId& CellA = FRTCellId(0, 0, 0), const FRTCellId& CellB = FRTCellId(1, 0, 0))
	{
		Out.World = MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), Out.World)) { return false; }

		ARTHexMapActor* Map = SpawnMap(Out.World);
		Out.TM = Out.World->SpawnActor<ARTTurnManager>();
		Out.A = SpawnUnit(Out.World, /*TeamId=*/ 0, CellA);
		Out.B = SpawnUnit(Out.World, /*TeamId=*/ 1, CellB);

		return Test.TestNotNull(TEXT("mappa"), Map)
			&& Test.TestNotNull(TEXT("turn manager"), Out.TM)
			&& Test.TestNotNull(TEXT("unita' A"), Out.A)
			&& Test.TestNotNull(TEXT("unita' B"), Out.B);
	}

	/**
	 * Le righe che il TurnLog produce, con la STESSA risoluzione dei nomi che usa `ConcludeTurn` (`#1932`).
	 *
	 * ⚠️ Non `DescribeTurnLog`: da quando le voci `Move` portano il soggetto nel testo, quella forma le rende
	 * con `u<id>` — non avendo la mappa — e il confronto con cio' che e' stato emesso cadrebbe su
	 * `Gadget: resta` contro `u3: resta`, che e' la stessa riga scritta da due risoluzioni diverse. Il
	 * produttore resta uno solo: qui si passa la mappa, non si riscrive il testo.
	 */
	TArray<FString> RigheAttese(const ARTTurnManager* TM)
	{
		TArray<FString> Righe;
		for (const FRTDescribedLine& Line
			: URTTurnLogLibrary::DescribeTurnLogWithSubjects(TM->GetTurnLog(), TM->SubjectNamesForLog()))
		{
			Righe.Add(Line.Text);
		}
		return Righe;
	}
}

/**
 * **Ogni riga porta il reason code e le coordinate.**
 *
 * E' la voce 1 del DoD: `ActionId`, coordinate assiali `(q,r,L)`, bersaglio, esito. Il test costruisce le
 * voci a mano perche' interroga il FORMATO della riga, non chi la produce — quello e' l'altro test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLogContainsReasonAndCoordsTest,
	"RefactorTactics.UI.LogContainsReasonAndCoords",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLogContainsReasonAndCoordsTest::RunTest(const FString&)
{
	FRTTurnLogEntry Colpo;
	Colpo.TurnNumber = 1;
	Colpo.UnitId = 7;
	Colpo.Phase = ERTMatchPhase::Blast;
	Colpo.Category = ERTLogCategory::Combat;
	Colpo.Outcome = static_cast<uint8>(ERTCombatOutcome::Lethal);
	Colpo.SrcCell = FRTCellId(0, 0, 0);
	Colpo.TgtCell = FRTCellId(2, -1, 1);
	Colpo.Amount = 40;
	Colpo.ActionId = FName(TEXT("Hero.Wraith.PiercingShot"));

	const TArray<FString> Righe = URTTurnLogLibrary::DescribeTurnLog({ Colpo });
	if (!TestEqual(TEXT("una riga per voce"), Righe.Num(), 1)) { return false; }

	const FString& R = Righe[0];
	// Le coordinate di ENTRAMBI i capi: senza il bersaglio, «40 danni» non dice a chi.
	TestTrue(*FString::Printf(TEXT("la riga porta la cella di partenza: %s"), *R), R.Contains(TEXT("(q=0,r=0,L=0)")));
	TestTrue(*FString::Printf(TEXT("e quella del bersaglio: %s"), *R), R.Contains(TEXT("(q=2,r=-1,L=1)")));
	// Il reason code tradotto, non il numero dell'enum.
	TestTrue(*FString::Printf(TEXT("e l'esito in chiaro: %s"), *R), R.Contains(TEXT("eliminata")));
	// L'ActionId: senza, due colpi diversi dello stesso eroe sono indistinguibili nel log.
	TestTrue(*FString::Printf(TEXT("e l'azione che l'ha prodotto: %s"), *R), R.Contains(TEXT("Hero.Wraith.PiercingShot")));

	return true;
}

/**
 * **La sequenza leggibile e' quella del TurnLog, nello stesso ordine.**
 *
 * Passa dal percorso vero — `LockInAndResolve` in un mondo di prova — perche' e' l'unica forma che cade se
 * qualcuno rimette un secondo produttore: un test che chiamasse `DescribeTurnLog` a mano resterebbe verde
 * mentre a schermo compaiono righe che il TurnLog non conosce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLogMatchesTurnLogOrderTest,
	"RefactorTactics.UI.LogMatchesTurnLogOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLogMatchesTurnLogOrderTest::RunTest(const FString&)
{
	UWorld* World = RTCombatLogFixture::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* Map = RTCombatLogFixture::SpawnMap(World);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	ARTUnit* A = RTCombatLogFixture::SpawnUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0, 0));
	ARTUnit* B = RTCombatLogFixture::SpawnUnit(World, /*TeamId=*/ 1, FRTCellId(1, 0, 0));
	if (!TestNotNull(TEXT("turn manager"), TM) || !TestNotNull(TEXT("mappa"), Map)
		|| !TestNotNull(TEXT("unita' A"), A) || !TestNotNull(TEXT("unita' B"), B))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	RTCombatLogFixture::RunTurn(TM);

	const TArray<FRTTurnLogEntry>& Log = TM->GetTurnLog();
	if (!TestTrue(TEXT("premessa: il turno ha prodotto almeno una voce di TurnLog"), Log.Num() > 0))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	const TArray<FString> Attese = RTCombatLogFixture::RigheAttese(TM);
	const TArray<FString>& Emesse = TM->GetRecentEvents();

	// ⚠️ **Non la coda: una sottosequenza CONSECUTIVA.** Dopo le righe del turno il log annuncia il turno
	// successivo (`Turno N - pianificazione`), che e' informazione legittima e che il TurnLog non contiene —
	// il DoD chiede «stesse informazioni, stesso ordine», non che il TurnLog sia l'ultima cosa detta. La
	// prima stesura asseriva la coda e cadeva proprio li': l'asserzione era sbagliata, non il codice.
	//
	// `RecentEvents` e' inoltre una finestra (`MaxLogLines`), quindi si cerca il blocco dentro cio' che resta.
	int32 Inizio = INDEX_NONE;
	for (int32 I = 0; I + Attese.Num() <= Emesse.Num(); ++I)
	{
		bool bTutte = true;
		for (int32 J = 0; J < Attese.Num(); ++J)
		{
			if (Emesse[I + J] != Attese[J]) { bTutte = false; break; }
		}
		if (bTutte) { Inizio = I; break; }
	}

	if (!TestTrue(*FString::Printf(
		TEXT("le %d righe del TurnLog compaiono nel log, consecutive e in ordine (log: %s)"),
		Attese.Num(), *FString::Join(Emesse, TEXT(" | "))), Inizio != INDEX_NONE))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}
	RTCombatLogFixture::DestroyWorld(World);
	return true;
}


/**
 * **Le due superfici accreditano la STESSA unita'.**
 *
 * `RearHitBypassedCover` scriveva l'evento a due destinatari attribuendolo a due unita' diverse: il TurnLog
 * all'attaccante, il combat log a chi il colpo l'aveva subito (`#1418`). Un consumatore che aggrega per
 * `UnitId` e un umano che legge il log rispondevano diversamente a «chi l'ha fatto» — e nessuno dei due
 * poteva accorgersene, perche' guardava una superficie sola.
 *
 * Chi ha ragione lo dicono i campi che la voce riempie: `Amount` porta il `Facing` del DIFENSORE, `TgtCell`
 * la sua cella, la categoria e' `Facing`. La voce descrive l'orientamento che non ha retto, quindi il
 * soggetto e' chi era in guardia. La riga leggibile aveva ragione dall'inizio.
 *
 * Il test guarda ENTRAMBE le superfici sullo stesso evento: e' l'unica forma che cade se tornano a
 * divergere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRearHitCreditsSameUnitTest,
	"RefactorTactics.UI.RearHitCreditsTheSameUnitInBothLogs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRearHitCreditsSameUnitTest::RunTest(const FString&)
{
	UWorld* World = RTCombatLogFixture::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* Map = RTCombatLogFixture::SpawnMap(World);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	// Il difensore guarda a EST; l'attaccante sta a OVEST, cioe' fuori dall'arco frontale.
	ARTUnit* Difensore = RTCombatLogFixture::SpawnUnit(World, /*TeamId=*/ 1, FRTCellId(0, 0, 0));
	ARTUnit* Attaccante = RTCombatLogFixture::SpawnUnit(World, /*TeamId=*/ 0, FRTCellId(-1, 0, 0));
	if (!TestNotNull(TEXT("turn manager"), TM) || !TestNotNull(TEXT("mappa"), Map)
		|| !TestNotNull(TEXT("difensore"), Difensore) || !TestNotNull(TEXT("attaccante"), Attaccante))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	Difensore->Facing = ERTHexDirection::E;
	Difensore->ApplyStatus(TAG_Status_Guarded, 1);
	Attaccante->PlannedAbilityIndex = 0; // attacco base
	Attaccante->PlannedAttackTarget = Difensore;

	RTCombatLogFixture::RunTurn(TM);

	const TArray<FRTTurnLogEntry>& Log = TM->GetTurnLog();
	const FRTTurnLogEntry* Bypassed = Log.FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Facing
			&& E.Outcome == static_cast<uint8>(ERTFacingOutcome::RearHitBypassedGuard);
	});
	if (!TestNotNull(TEXT("premessa: il colpo alle spalle ha annullato la guardia"), Bypassed))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	// Una voce sola: con l'arena piatta non c'e' copertura, quindi puo' averla prodotta solo il ramo della
	// Guard. Senza questo conteggio il test potrebbe finire a misurare l'ALTRO produttore il giorno in cui
	// la fixture guadagnasse una copertura — e continuerebbe a dire di misurare questo.
	TestEqual(TEXT("una sola voce, dal ramo della Guard"),
		Log.FilterByPredicate([](const FRTTurnLogEntry& E)
		{
			return E.Category == ERTLogCategory::Facing
				&& E.Outcome == static_cast<uint8>(ERTFacingOutcome::RearHitBypassedGuard);
		}).Num(), 1);

	// 🔴 **E nessuna voce dell'ALTRO esito** (`#1430`, [D-199]). E' l'asserzione che cade se qualcuno
	// riunifica i due: prima della separazione questo scenario — arena piatta, nessuna copertura — produceva
	// una voce `RearHitBypassedCover` il cui `Amount` era una DIREZIONE, non i punti scavalcati che quel nome
	// promette. Senza questa riga, riunificarli tornerebbe verde.
	TestEqual(TEXT("e nessuna voce di copertura scavalcata: qui non c'e' copertura"),
		Log.FilterByPredicate([](const FRTTurnLogEntry& E)
		{
			return E.Category == ERTLogCategory::Facing
				&& E.Outcome == static_cast<uint8>(ERTFacingOutcome::RearHitBypassedCover);
		}).Num(), 0);

	// `Amount` porta la DIREZIONE del difensore, ed e' cio' che distingue questo esito dall'altro.
	TestEqual(TEXT("Amount porta il facing del difensore"),
		Bypassed->Amount, static_cast<int32>(ERTHexDirection::E));

	// Il TurnLog dichiara chi ha SUBITO.
	TestEqual(TEXT("il TurnLog accredita il difensore"), Bypassed->UnitId, Difensore->StableUnitId);
	TestTrue(TEXT("e lo dichiara anche alla tassonomia, non solo a chi ha letto il commento"),
		URTTurnLogLibrary::IsSubjectTheSufferer(*Bypassed));

	// 🔴 L'invariante vero: le due superfici nominano la STESSA unita'. Si risolve l'unita' che il TurnLog
	// accredita e si cerca IL SUO nome nella riga — non quello del difensore.
	//
	// Confrontare direttamente col difensore sarebbe tautologico: la riga leggibile nasce da
	// `AddLogEvent("%s: %s", Units[i]->GetName(), ...)` dove `Units[i]` E' il difensore per costruzione del
	// loop, quindi conterrebbe il suo nome comunque — anche col `UnitId` sbagliato. Cosi' invece il test
	// cade appena i due tornano a divergere, ed e' la forma per cui esiste.
	const ARTUnit* Accreditata =
		Bypassed->UnitId == Difensore->StableUnitId ? Difensore :
		Bypassed->UnitId == Attaccante->StableUnitId ? Attaccante : nullptr;
	if (!TestNotNull(TEXT("il TurnLog accredita una delle due unita' in campo"), Accreditata))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	// ⚠️ `RecentEvents` e' una finestra (`MaxLogLines`): con due unita' e un turno la riga ci sta, ma un
	// giorno che questa fixture crescesse andrebbe cercata prima che venga sfrattata.
	//
	// 🔴 **Questo test DIPENDE da una riga che `#1412` punto 2 vuole togliere**: la riga col nome davanti
	// nasce dall'`AddLogEvent` scritto a mano di `RTTurnManager.cpp:3996`, uno dei sette duplicati. Il
	// giorno in cui spariscono, l'invariante «le due superfici accreditano la stessa unita'» va riformulato
	// sulla superficie derivata — che a quel punto dovra' saper nominare l'attore, ed e' esattamente la
	// domanda che tiene aperto quel punto. Detto qui perche' chi toglie quella riga trova questo test rosso
	// e deve sapere che non e' una regressione.
	const FString Descrizione = URTTurnLogLibrary::DescribeEntry(*Bypassed);
	const TArray<FString>& Emesse = TM->GetRecentEvents();
	const FString* Riga = Emesse.FindByPredicate([&Descrizione](const FString& L)
	{
		// La riga NOMINATA, non quella derivata da `ConcludeTurn`: quella e' `DescribeEntry` e basta, e non
		// dice chi. E' proprio la sua mancanza di nome a rendere utile il confronto qui.
		return L.Contains(Descrizione) && L.Len() > Descrizione.Len();
	});
	if (!TestNotNull(TEXT("l'evento compare anche nel combat log, con un nome davanti"), Riga))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}
	TestTrue(*FString::Printf(TEXT("e il combat log nomina l'unita' che il TurnLog accredita: %s"), **Riga),
		Riga->Contains(Accreditata->GetName()));

	RTCombatLogFixture::DestroyWorld(World);
	return true;
}

/**
 * **L'altro annullamento accredita la stessa unita', e porta un `Amount` di natura diversa.**
 *
 * ⚠️ Fino al 2026-08-27 i due rami emettevano lo STESSO esito (`#1430`, [D-199]): la guardia scavalcata e la
 * copertura scavalcata. Ora sono `RearHitBypassedGuard` e `RearHitBypassedCover`, e questo test copre il
 * secondo — quello che mette in `Amount` i **punti di riduzione** invece della direzione.
 *
 * Il ramo della copertura accreditava l'attaccante come l'altro, e la issue `#1418` non lo citava — quindi
 * senza questo test la meta' meno visibile di quella correzione tornerebbe indietro senza che niente diventi
 * rosso: il test qui sopra gira su un'arena piatta, dove questo ramo non si esegue mai.
 *
 * ⚠️ Il difensore NON e' in guardia, apposta: con la guardia si attiverebbero entrambi i produttori e non si
 * saprebbe quale delle due voci si sta guardando.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRearHitOnCoverCreditsSameUnitTest,
	"RefactorTactics.UI.RearHitOnCoverCreditsTheDefender",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRearHitOnCoverCreditsSameUnitTest::RunTest(const FString&)
{
	UWorld* World = RTCombatLogFixture::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* Map = RTCombatLogFixture::SpawnMap(World);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	ARTUnit* Difensore = RTCombatLogFixture::SpawnUnit(World, /*TeamId=*/ 1, FRTCellId(0, 0, 0));
	ARTUnit* Attaccante = RTCombatLogFixture::SpawnUnit(World, /*TeamId=*/ 0, FRTCellId(-1, 0, 0));
	if (!TestNotNull(TEXT("turn manager"), TM) || !TestNotNull(TEXT("mappa"), Map)
		|| !TestNotNull(TEXT("difensore"), Difensore) || !TestNotNull(TEXT("attaccante"), Attaccante))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	// Copertura bassa sul lato da cui arriva il colpo: senza, non c'e' riduzione da scavalcare.
	if (URTHexMapAsset* Asset = Map->MapAsset)
	{
		const FRTHexCellData* Existing = Asset->FindCell(FRTCellId(0, 0, 0));
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(FRTCellId(0, 0, 0));
		Data.Covers.Add(FRTHexCover(ERTHexDirection::W, ERTHexCoverType::Low,
			FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low)));
		Asset->AddOrUpdateCell(Data);
		Asset->SortCells();
	}

	Difensore->Facing = ERTHexDirection::E; // guarda dall'altra parte: il colpo arriva alle spalle
	Attaccante->PlannedAbilityIndex = 0;
	Attaccante->PlannedAttackTarget = Difensore;

	RTCombatLogFixture::RunTurn(TM);

	const TArray<FRTTurnLogEntry>& Log = TM->GetTurnLog();
	const FRTTurnLogEntry* Bypassed = Log.FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Facing
			&& E.Outcome == static_cast<uint8>(ERTFacingOutcome::RearHitBypassedCover);
	});
	if (!TestNotNull(TEXT("premessa: la copertura e' stata scavalcata"), Bypassed))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	// ⚠️ `Amount` qui porta i PUNTI di riduzione scavalcati, non una direzione: e' la divergenza fra i due
	// produttori dello stesso esito, dichiarata e aperta in `#1430`. Serve anche a distinguere il ramo: una
	// direzione sta in [0,5].
	TestTrue(TEXT("e' la voce del ramo COPERTURA, non quella della Guard"), Bypassed->Amount > 5);

	TestEqual(TEXT("accredita il difensore, come l'altro produttore"),
		Bypassed->UnitId, Difensore->StableUnitId);
	TestTrue(TEXT("e la tassonomia lo conferma"), URTTurnLogLibrary::IsSubjectTheSufferer(*Bypassed));

	RTCombatLogFixture::DestroyWorld(World);
	return true;
}

/**
 * **Anche la riga di un'azione ANNULLATA dice quale azione era.**
 *
 * Il ramo `Fallback` era l'unico di `DescribeEntry` a non chiamare `DescribeActionIdentity`: rendeva celle,
 * esito e motivo — pura geometria. Due azioni annullate dalla stessa unita' nello stesso turno producevano
 * righe identiche byte a byte, e [D-063] vieta di dedurre l'unita' da `SrcCell`: non c'era modo di dire
 * quale delle due fosse (`#1412`).
 *
 * ⚠️ Dei tre produttori della categoria, DUE l'azione la scrivono gia' (`RTTurnManager_Blast.cpp:294`,
 * `RTTurnManager.cpp:3457`): da oggi si leggono. Il terzo — `FallbackEntry` — no, e riempirlo cambia l'hash
 * delle tracce, quindi e' un cambio d'identita' da decidere a parte. Le sue righe restano senza suffisso,
 * come fanno `Move` e `Combat` quando l'azione non c'e': un «non dichiarata» in coda a ogni annullamento
 * direbbe al giocatore una lacuna interna, che non e' informazione di gioco.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLogFallbackNamesTheActionTest,
	"RefactorTactics.UI.FallbackLineNamesTheAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLogFallbackNamesTheActionTest::RunTest(const FString&)
{
	FRTTurnLogEntry Annullata;
	Annullata.Phase = ERTMatchPhase::Blast;
	Annullata.Category = ERTLogCategory::Fallback;
	Annullata.Outcome = static_cast<uint8>(ERTFallbackOutcome::Stopped);
	Annullata.SrcCell = FRTCellId(1, 0, 0);
	Annullata.TgtCell = FRTCellId(3, 0, 0);
	Annullata.Amount = static_cast<int32>(ERTActionInvalidReason::TargetGone);

	// Senza nessuna azione: nessun suffisso — e soprattutto nessun `None`, che si leggerebbe come un id
	// d'azione vero.
	const FString Muta = URTTurnLogLibrary::DescribeEntry(Annullata);
	TestFalse(*FString::Printf(TEXT("non scrive 'None': %s"), *Muta), Muta.Contains(TEXT("None")));
	TestTrue(*FString::Printf(TEXT("e il motivo c'e' comunque: %s"), *Muta),
		Muta.Contains(TEXT("bersaglio assente")));

	// Il motivo `TargetUnknown` non cadeva piu' nel generico «non eseguibile».
	{
		FRTTurnLogEntry Ignoto = Annullata;
		Ignoto.Amount = static_cast<int32>(ERTActionInvalidReason::TargetUnknown);
		TestTrue(TEXT("un bersaglio ignoto si dice, non si generalizza"),
			URTTurnLogLibrary::DescribeEntry(Ignoto).Contains(TEXT("bersaglio ignoto")));
	}

	// Con `ActionId`: la riga lo nomina, come ogni altra categoria.
	Annullata.ActionId = FName(TEXT("Hero.Wraith.PulseShot"));
	const FString Nominata = URTTurnLogLibrary::DescribeEntry(Annullata);
	TestTrue(*FString::Printf(TEXT("nomina l'azione: %s"), *Nominata),
		Nominata.Contains(TEXT("Hero.Wraith.PulseShot")));

	// Due azioni annullate dalla stessa unita' nello stesso posto NON producono piu' la stessa riga: e' il
	// difetto per cui questo ramo esisteva senza identita'.
	FRTTurnLogEntry Altra = Annullata;
	Altra.ActionId = FName(TEXT("Action.BasicAttack"));
	TestNotEqual(TEXT("due azioni annullate dalla stessa cella si distinguono"),
		URTTurnLogLibrary::DescribeEntry(Altra), Nominata);

	// E il profilo, quando c'e', si legge come nelle altre categorie. Token canonico `Hero.<Nome>.<Abilita>`
	// (D-130): i nomi legacy sono usciti dal repository, e il gate che li cercava e' uscito con D-182.
	Altra.BaseActionId = FName(TEXT("Action.BasicAttack"));
	Altra.ActionId = FName(TEXT("Hero.Riktor.ImpactShot"));
	TestTrue(TEXT("azione base e profilo, come altrove"),
		URTTurnLogLibrary::DescribeEntry(Altra).Contains(TEXT("Action.BasicAttack · Hero.Riktor.ImpactShot")));

	// Solo il PROFILO, senza l'azione: si legge il profilo, non `Action.BasicAttack · None`.
	{
		FRTTurnLogEntry SoloBase = Annullata;
		SoloBase.ActionId = FName();
		SoloBase.BaseActionId = FName(TEXT("Action.BasicAttack"));
		const FString Riga = URTTurnLogLibrary::DescribeEntry(SoloBase);
		TestTrue(*FString::Printf(TEXT("il profilo si legge: %s"), *Riga),
			Riga.Contains(TEXT("Action.BasicAttack")));
		TestFalse(*FString::Printf(TEXT("e senza 'None' accanto: %s"), *Riga), Riga.Contains(TEXT("None")));
	}

	return true;
}

/**
 * **Ogni riga derivata compare UNA volta sola.**
 *
 * `UI.LogMatchesTurnLogOrder` qui sopra cerca il blocco derivato come sottosequenza CONSECUTIVA, e resta
 * verde con un duplicato prima o dopo: e' il difetto di `#1412`, dove sette punti della risoluzione
 * chiamano `AddLogEvent(... DescribeEntry(X))` subito dopo aver appeso `X` al TurnLog, e la stessa
 * informazione arriva al giocatore due volte.
 *
 * ⚠️ **Non si confrontano righe uguali**: quelle scritte a mano portano davanti `Unit->GetName()`, quindi
 * `==` non le vede. Il duplicato e' la stessa informazione in due FORMATI — ed e' anche il motivo per cui
 * nessuno se n'era accorto prima.
 *
 * ⚠️ Ma nemmeno `Contains`: una riga derivata puo' essere PREFISSO di un'altra (`«resta (q=0,r=0,L=0)»` e
 * `«resta (q=0,r=0,L=0) (Action.Move, p50)»`), e `FString::Contains` e' per giunta case-insensitive di
 * default. Si conta per corrispondenza esatta, o esatta preceduta da `«Nome: »`.
 *
 * ⚠️ E si conta per riga UNICA contro le sue occorrenze ATTESE: due voci diverse possono rendere la stessa
 * stringa — due azioni annullate senza `ActionId` lo fanno — e in quel caso il combat log deve emetterla
 * due volte. Un `1` fisso fallirebbe proprio sul caso di `#1412`.
 *
 * ⚠️ Copertura di questo percorso: due unita' che non attaccano. Le sette righe doppie vivono in rami che
 * questa fixture non attraversa (fallback, reazioni, colpi senza linea di tiro), quindi il test protegge
 * dall'**ottavo** duplicato piu' che misurare i sette — che restano aperti in `#1412` e non si tolgono
 * finche' `DescribeTurnLog` non sa nominare l'attore. Esercitarli QUI renderebbe il test rosso su un
 * difetto noto e non ancora correggibile, che e' un modo per farlo ignorare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLogDoesNotRepeatDerivedLinesTest,
	"RefactorTactics.UI.LogDoesNotRepeatTheDerivedLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLogDoesNotRepeatDerivedLinesTest::RunTest(const FString&)
{
	RTCombatLogFixture::FTwoUnitTurn Fixture;
	if (!RTCombatLogFixture::BuildTwoUnitTurn(*this, Fixture))
	{
		RTCombatLogFixture::DestroyWorld(Fixture.World);
		return false;
	}

	RTCombatLogFixture::RunTurn(Fixture.TM);

	const TArray<FRTTurnLogEntry>& Log = Fixture.TM->GetTurnLog();
	if (!TestTrue(TEXT("premessa: il turno ha prodotto almeno una voce"), Log.Num() > 0))
	{
		RTCombatLogFixture::DestroyWorld(Fixture.World);
		return false;
	}

	const TArray<FString> Attese = RTCombatLogFixture::RigheAttese(Fixture.TM);
	const TArray<FString>& Emesse = Fixture.TM->GetRecentEvents();

	// Quante volte ogni riga e' ATTESA. Due voci che rendono la stessa stringa vanno emesse due volte.
	TMap<FString, int32> Previste;
	for (const FString& Riga : Attese)
	{
		++Previste.FindOrAdd(Riga);
	}

	for (const TPair<FString, int32>& Prevista : Previste)
	{
		const FString Nominata = FString(TEXT(": ")) + Prevista.Key;
		int32 Conta = 0;
		for (const FString& Emessa : Emesse)
		{
			// Esatta (la riga derivata) oppure esatta preceduta dal nome unita' (quella scritta a mano).
			if (Emessa.Equals(Prevista.Key, ESearchCase::CaseSensitive)
				|| Emessa.EndsWith(Nominata, ESearchCase::CaseSensitive))
			{
				++Conta;
			}
		}
		// ⚠️ `RecentEvents` e' una finestra (`MaxLogLines`): un `Conta` a ZERO qui vuol dire che la riga e'
		// stata sfrattata, non che manchi un produttore. Con questa fixture non succede, e il messaggio lo
		// dice a chi ci arrivasse dopo averla fatta crescere.
		TestEqual(*FString::Printf(
			TEXT("emessa tante volte quante attesa (0 = finestra del log troppo corta): %s"), *Prevista.Key),
			Conta, Prevista.Value);
	}

	RTCombatLogFixture::DestroyWorld(Fixture.World);
	return true;
}

/**
 * **Un esito che il log non sa tradurre non deve somigliare a uno che sa.**
 *
 * `Stayed` cadeva nel `default` insieme a ogni valore non ancora tradotto: le due cose producevano la
 * riga identica, quindi un esito nuovo entrava nel log travestito da «resta» e nessuno se ne accorgeva.
 * E' lo stesso difetto che il ramo `DisplacementResisted` evita di proposito poco sopra — «il valore di
 * questa voce sta tutto nel dire QUALE dei sei modi di non muoversi si e' verificato».
 *
 * Misurato in partita il 2026-08-23: dodici turni di autobattle producevano solo righe `resta`, e non
 * c'era modo di sapere se fosse una scelta del bot o un esito che il log non conosceva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLogDistinguishesUntranslatedOutcomeTest,
	"RefactorTactics.UI.LogDistinguishesUntranslatedOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLogDistinguishesUntranslatedOutcomeTest::RunTest(const FString&)
{
	auto MoveEntry = [](uint8 Outcome)
	{
		FRTTurnLogEntry E;
		E.Category = ERTLogCategory::Move;
		E.Outcome = Outcome;
		E.SrcCell = FRTCellId(0, 0, 0);
		return E;
	};

	const FString Fermo = URTTurnLogLibrary::DescribeEntry(MoveEntry(static_cast<uint8>(ERTMoveOutcome::Stayed)));
	// Un valore che l'enum non ha: sta per l'esito che qualcuno aggiungera' domani senza tradurlo qui.
	const FString Ignoto = URTTurnLogLibrary::DescribeEntry(MoveEntry(200));

	TestTrue(*FString::Printf(TEXT("l'unita' ferma si legge: %s"), *Fermo), Fermo.Contains(TEXT("resta")));
	TestNotEqual(TEXT("e un esito non tradotto NON si confonde con lei"), Ignoto, Fermo);

	return true;
}

/**
 * `SlideBlocked` HA UNA FRASE PROPRIA, E DICE CHE IL PIANO E' RIUSCITO — `#2314`.
 *
 * 🔴 **Un esito nuovo non tradotto non fallisce a compilazione**: cade nel `default` di `DescribeEntry`,
 * che stampa *«esito di movimento non tradotto»*. La riga si legge — e' la difesa che
 * `UI.LogDistinguishesUntranslatedOutcome` ha messo — ma non dice al giocatore niente di cio' che e'
 * successo, e nessun test la vedrebbe se non ce ne fosse uno che chiede la traduzione per nome.
 *
 * ⚠️ **La frase non puo' aprire con «fermo»**, come tutti i suoi vicini nel vocabolario dei blocchi:
 * `SlideBlocked` e' l'unico esito in cui il movimento CHIESTO dal giocatore e' riuscito, e dirgli «fermo»
 * lo manderebbe a cercare un errore nel proprio piano invece di una lastra di ghiaccio contro un muro.
 *
 * ⚠️ **E deve stampare la destinazione.** E' un arrivo: nascondere `TgtCell` lascerebbe «impedito» senza
 * la meta' che rende la riga una buona notizia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLogRendersSlideBlockedTest,
	"RefactorTactics.UI.LogRendersSlideBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLogRendersSlideBlockedTest::RunTest(const FString&)
{
	auto SlideEntry = [](ERTMoveOutcome Outcome)
	{
		FRTTurnLogEntry E;
		E.Category = ERTLogCategory::Move;
		E.Outcome = static_cast<uint8>(Outcome);
		E.SrcCell = FRTCellId(0, 0, 0);
		E.TgtCell = FRTCellId(2, -1, 0);
		E.Amount = 2;
		return E;
	};

	const FString Impedito = URTTurnLogLibrary::DescribeEntry(SlideEntry(ERTMoveOutcome::SlideBlocked));

	TestFalse(*FString::Printf(TEXT("non e' un esito ignoto: %s"), *Impedito),
		Impedito.Contains(TEXT("non tradotto")));
	TestTrue(TEXT("nomina lo scivolamento impedito"), Impedito.Contains(TEXT("scivolamento impedito")));
	// «arriva», non «fermo»: il Move chiesto dal giocatore e' riuscito. `StartsWith` e non `Contains`,
	// perche' la regola parla della parola d'APERTURA: un `Contains(TEXT("fermo"))` negativo passerebbe
	// anche per una riga che dice «fermo» piu' avanti, e fallirebbe per una cella o un `ActionId` che
	// contenessero quella sequenza di lettere per caso.
	TestTrue(*FString::Printf(TEXT("apre con «arriva»: %s"), *Impedito), Impedito.StartsWith(TEXT("arriva")));
	// La forma LUNGA — `partenza -> destinazione (N celle)` — e non la breve, che stampa la sola `SrcCell`.
	// La freccia e' cio' che la distingue, e senza di lei la riga direbbe «impedito» e basta.
	TestTrue(*FString::Printf(TEXT("e stampa la destinazione raggiunta: %s"), *Impedito),
		Impedito.Contains(TEXT(" -> ")));

	// DISTINTO dai tre vicini con cui potrebbe confondersi. Tre confronti e non uno: lo scivolamento
	// avvenuto e quello impedito sono la coppia che l'esito esiste per separare, e `Moved` e' cio' che il
	// log diceva prima di `#2314` a chi arrivava senza scivolare.
	TestNotEqual(TEXT("non si confonde con lo scivolamento AVVENUTO"),
		Impedito, URTTurnLogLibrary::DescribeEntry(SlideEntry(ERTMoveOutcome::Slid)));
	TestNotEqual(TEXT("non si confonde con un movimento riuscito qualunque"),
		Impedito, URTTurnLogLibrary::DescribeEntry(SlideEntry(ERTMoveOutcome::Moved)));
	TestNotEqual(TEXT("non si confonde con una cella occupata"),
		Impedito, URTTurnLogLibrary::DescribeEntry(SlideEntry(ERTMoveOutcome::BlockedByUnit)));

	return true;
}

/**
 * 🔴 **Il combat log di un turno VERO non nomina un nemico che la squadra non vede piu'.**
 *
 * Test di PIPELINE, non di sito: percorre `LockInAndResolve -> TurnLog -> ConcludeTurn -> RecentEvents ->
 * GetRecentEventsForTeam` e asserisce sull'USCITA. Un test per sito invecchia il giorno in cui qualcuno
 * aggiunge il sito successivo; questo no — e' l'unico asserto che vede tutti i canali insieme, compresa la
 * derivazione dal TurnLog, che e' il canale primario (CP 11.3, `#79`) e che nessun test toccava.
 *
 * Lo scenario e' quello vero della feature: si vede un nemico, poi lo si perde di vista. Da quel momento la
 * sua posizione ATTUALE — che `DescribeEntry` stampa per ogni voce `Move`, `SrcCell` **e** `TgtCell` — e'
 * precisamente cio' che la squadra ha smesso di sapere.
 *
 * ⚠️ **Anti-vacuita' doppia**, perche' un test di sola assenza e' verde anche quando non succede niente:
 * si asserisce che la premessa regga (il nemico e' davvero un `CellOnly`, cioe' un ricordo) e che la vista
 * NON filtrata contenga davvero la riga incriminata. Senza queste due, il test passerebbe su un log vuoto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLogOmitsRememberedEnemyEndToEndTest,
	"RefactorTactics.UI.LogOmitsRememberedEnemyEndToEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLogOmitsRememberedEnemyEndToEndTest::RunTest(const FString&)
{
	UWorld* World = RTCombatLogFixture::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* Map = RTCombatLogFixture::SpawnMap(World);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	ARTUnit* Mia = RTCombatLogFixture::SpawnUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0, 0));
	ARTUnit* Nemica = RTCombatLogFixture::SpawnUnit(World, /*TeamId=*/ 1, FRTCellId(1, 0, 0));
	if (!TestNotNull(TEXT("turn manager"), TM) || !TestNotNull(TEXT("mappa"), Map)
		|| !TestNotNull(TEXT("unita' mia"), Mia) || !TestNotNull(TEXT("unita' nemica"), Nemica))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	// Turno 1 — adiacenti: la nemica si VEDE, e la squadra registra il contatto.
	RTCombatLogFixture::RunTurn(TM);

	// Poi la si perde di vista: si sposta lontano e la propria unita' smette di arrivarci.
	// `VisionRange = 0` lascia comunque attivo il canale ravvicinato (`CloseAwarenessRange` = 2), quindi la
	// distanza 5 e' oltre entrambi i canali.
	Nemica->PlaceOnCell(FRTCellId(5, 0, 0), FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
	Mia->VisionRange = 0;

	// Turno 2 — la nemica agisce dove nessuno la vede: le sue voci di TurnLog portano (5,0,0).
	RTCombatLogFixture::RunTurn(TM);

	// ── Premessa 1: e' davvero un RICORDO, non un'ignota e non una vista. Senza questa riga il test
	// potrebbe passare per la ragione sbagliata (nessuna voce affatto).
	const ERTTargetKnowledge Conoscenza = URTTeamKnowledgeLibrary::ClassifyTarget(
		TM->KnowledgeForTeamPublic(0), Nemica->StableUnitId, Nemica->TeamId, Nemica->Cell);
	if (!TestTrue(TEXT("premessa: la nemica e' un RICORDO per la squadra 0 (CellOnly)"),
		Conoscenza == ERTTargetKnowledge::CellOnly))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	// Le due forme in cui una cella compare nel log: `DescribeEntry` scrive `L=0`, i siti a mano scrivono `L0`.
	const FString CellaAttualeA = TEXT("(q=5,r=0,L=0)");
	const FString CellaAttualeB = TEXT("(q=5,r=0,L0)");
	const FString NomeNemica = Nemica->GetName();

	// ── Premessa 2: la vista NON filtrata contiene davvero la riga incriminata. Se il turno non l'avesse
	// prodotta non ci sarebbe niente da nascondere, e l'assenza sotto non proverebbe nulla.
	const TArray<FString>& Complete = TM->GetRecentEvents();
	bool bLeakNelCanaleCompleto = false;
	for (const FString& L : Complete)
	{
		if (L.Contains(CellaAttualeA) || L.Contains(CellaAttualeB)) { bLeakNelCanaleCompleto = true; break; }
	}
	if (!TestTrue(*FString::Printf(
		TEXT("premessa: il log completo nomina la posizione attuale della nemica (log: %s)"),
		*FString::Join(Complete, TEXT(" | "))), bLeakNelCanaleCompleto))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	// ── 🔴 Il cuore: la vista del giocatore non la nomina, ne' per coordinate ne' per nome.
	const TArray<FString> Visibili = TM->GetRecentEventsForTeam(0);
	for (const FString& L : Visibili)
	{
		TestFalse(*FString::Printf(TEXT("nessuna riga porta la cella attuale della nemica: %s"), *L),
			L.Contains(CellaAttualeA) || L.Contains(CellaAttualeB));
		TestFalse(*FString::Printf(TEXT("nessuna riga porta il nome della nemica: %s"), *L),
			L.Contains(NomeNemica));
	}

	// ── Anti-vacuita' finale: il filtro non ha svuotato il log. Le righe di mondo restano.
	bool bRigaDiMondo = false;
	for (const FString& L : Visibili)
	{
		if (L.Contains(TEXT("pianificazione"))) { bRigaDiMondo = true; break; }
	}
	TestTrue(*FString::Printf(TEXT("il log filtrato non e' vuoto (%d righe)"), Visibili.Num()),
		Visibili.Num() > 0);
	TestTrue(TEXT("e conserva le righe di mondo"), bRigaDiMondo);

	RTCombatLogFixture::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **Nemmeno l'ECO scritto a mano accanto alla voce nomina un nemico che la squadra non vede.**
 *
 * Il gemello qui sopra passava per la ragione sbagliata, e la re-review lo ha misurato: nel suo scenario la
 * nemica produce **solo** una voce `Move` con esito `Stayed`, e l'unico canale che la racconta e' la
 * derivazione dal TurnLog. Il verde dimostrava «nessun leak su *quel tipo* di evento», non «nessun leak».
 *
 * Sette siti della risoluzione rieccheggiano **verbatim** la voce che hanno appena scritto nel TurnLog,
 * chiamando lo stesso `URTTurnLogLibrary::DescribeEntry`. Sono una **seconda porta** verso `RecentEvents`:
 * l'evento entra nel TurnLog con la sua identita' e usciva di qui senza, quindi le coordinate che il filtro
 * sopprimeva sulla copia derivata arrivavano comunque a schermo dalla copia scritta a mano.
 *
 * Questo test costruisce l'evento piu' economico che ne attraversa uno — una **mossa bloccata**, l'eco di
 * `ResolveMovement` — e asserisce sull'uscita filtrata.
 *
 * ⚠️ **La firma dell'eco e' il NOME.** La copia derivata da `ConcludeTurn` e' il solo `DescribeEntry`
 * (*«fermo: cella occupata (q=5,r=0,L=0)»*); l'eco antepone `%s: ` col nome dell'unita'. Una riga che porta
 * **entrambi** puo' venire solo dalla seconda porta: e' cosi' che la premessa 3 distingue i due canali senza
 * contare le righe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLogOmitsRememberedEnemyBlockedMoveTest,
	"RefactorTactics.UI.LogOmitsRememberedEnemyBlockedMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLogOmitsRememberedEnemyBlockedMoveTest::RunTest(const FString&)
{
	UWorld* World = RTCombatLogFixture::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* Map = RTCombatLogFixture::SpawnMap(World);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	ARTUnit* Mia = RTCombatLogFixture::SpawnUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0, 0));
	ARTUnit* Nemica = RTCombatLogFixture::SpawnUnit(World, /*TeamId=*/ 1, FRTCellId(1, 0, 0));
	// ⚠️ L'ostacolo e' della squadra AVVERSARIA, non della mia: un alleato piazzato li' vedrebbe la nemica
	// da un passo di distanza e la terrebbe `Live`, cioe' smonterebbe la premessa del test.
	ARTUnit* Muro = RTCombatLogFixture::SpawnUnit(World, /*TeamId=*/ 1, FRTCellId(4, 0, 0));
	if (!TestNotNull(TEXT("turn manager"), TM) || !TestNotNull(TEXT("mappa"), Map)
		|| !TestNotNull(TEXT("unita' mia"), Mia) || !TestNotNull(TEXT("unita' nemica"), Nemica)
		|| !TestNotNull(TEXT("l'ostacolo"), Muro))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	// Turno 1 — adiacenti: la nemica si VEDE, e la squadra registra il contatto.
	RTCombatLogFixture::RunTurn(TM);

	// Poi la si perde di vista. `VisionRange = 0` lascia attivo il canale ravvicinato
	// (`CloseAwarenessRange` = 2): la distanza 5 e' oltre entrambi.
	Nemica->PlaceOnCell(FRTCellId(5, 0, 0), FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
	Mia->VisionRange = 0;

	// Turno 2 — la nemica prova a muoversi e trova la cella occupata. Il percorso si scrive a mano dritto
	// dentro l'ostacolo: passando per `PlannedCell` l'A* lo aggirerebbe e non ci sarebbe mossa bloccata.
	// Bloccata al PRIMO passo, quindi partenza e arrivo coincidono: la cella che la riga stampa
	// (`SrcCell`, `Paths[i][0]`) e' anche quella in cui la nemica si trova a fine turno.
	Nemica->PlannedPath = { FRTCellId(5, 0, 0), FRTCellId(4, 0, 0), FRTCellId(3, 0, 0) };
	Nemica->PlannedCell = FRTCellId(3, 0, 0);
	Muro->PlannedCell = Muro->Cell; // fermo: e' l'ostacolo
	RTCombatLogFixture::RunTurn(TM);

	// ── Premessa 1: e' davvero un RICORDO, non un'ignota e non una vista.
	const ERTTargetKnowledge Conoscenza = URTTeamKnowledgeLibrary::ClassifyTarget(
		TM->KnowledgeForTeamPublic(0), Nemica->StableUnitId, Nemica->TeamId, Nemica->Cell);
	if (!TestTrue(TEXT("premessa: la nemica e' un RICORDO per la squadra 0 (CellOnly)"),
		Conoscenza == ERTTargetKnowledge::CellOnly))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	// ── Premessa 2: il turno ha davvero prodotto l'evento che attraversa l'eco. Senza, il test verificherebbe
	// l'assenza di una riga che nessuno ha scritto — e resterebbe verde anche a filtro rimosso.
	bool bMossaBloccata = false;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Move
			&& E.UnitId == Nemica->StableUnitId
			&& (E.Outcome == static_cast<uint8>(ERTMoveOutcome::BlockedByUnit)
				|| E.Outcome == static_cast<uint8>(ERTMoveOutcome::BlockedContested)))
		{
			bMossaBloccata = true;
			break;
		}
	}
	if (!TestTrue(TEXT("premessa: la nemica ha una voce Move BLOCCATA nel TurnLog (l'eco scatta solo su quelle)"),
		bMossaBloccata))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	const FString CellaAttuale = TEXT("(q=5,r=0,L=0)");
	const FString NomeNemica = Nemica->GetName();

	// ── Premessa 3: la riga con la posizione della nemica esiste davvero nel canale completo. Senza,
	// il cuore verificherebbe l'assenza di qualcosa che nessuno ha scritto.
	//
	// ⚠️ **Cercava nome E coordinate, e la ragione scritta qui non regge piu'.** Diceva *«la copia
	// derivata dal TurnLog non porta il nome, quindi solo l'eco puo' produrla»*: da `#1932` le voci
	// `Move` portano **anche il soggetto**, e da `#1412` l'eco scritto a mano non c'e' piu' — era un
	// duplicato che nominava la stessa unita' in un altro modo (`RTUnit_0` contro `Wraith`). La
	// premessa si regge ora sulla **cella**, che e' anche cio' che il cuore verifica.
	//
	// ⛔ Il nome non e' un criterio utilizzabile qui: le tre unita' della fixture escono tutte da
	// `MakeWraith`, quindi *«Wraith»* compare anche nelle righe della squadra che guarda. Cio' che
	// identifica la nemica in una riga e' la sua **posizione**, ed e' quella che non deve trapelare.
	const TArray<FString>& Complete = TM->GetRecentEvents();
	bool bEcoNelCanaleCompleto = false;
	for (const FString& L : Complete)
	{
		if (L.Contains(CellaAttuale)) { bEcoNelCanaleCompleto = true; break; }
	}
	if (!TestTrue(*FString::Printf(
		TEXT("premessa: la riga con la posizione della nemica e' nel log completo (log: %s)"),
		*FString::Join(Complete, TEXT(" | "))), bEcoNelCanaleCompleto))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	// ── 🔴 Il cuore: nella vista del giocatore quella riga non c'e', per nessuno dei due canali.
	const TArray<FString> Visibili = TM->GetRecentEventsForTeam(0);
	for (const FString& L : Visibili)
	{
		TestFalse(*FString::Printf(TEXT("nessuna riga porta la cella della nemica: %s"), *L),
			L.Contains(CellaAttuale));
		// ⚠️ Questo secondo controllo e' VACUO per costruzione, e resta perche' lo dica: `GetName()`
		// rende `RTUnit_N`, che da `#1412` nessun produttore scrive piu'. Cade solo se qualcuno
		// reintroducesse una riga che nomina l'Actor — cioe' il duplicato appena tolto. Il criterio che
		// MORDE e' quello sopra, sulla posizione.
		TestFalse(*FString::Printf(TEXT("nessuna riga porta il nome dell'Actor nemico: %s"), *L),
			L.Contains(NomeNemica));
	}

	// ── Anti-vacuita' finale: il filtro non ha svuotato il log.
	bool bRigaDiMondo = false;
	for (const FString& L : Visibili)
	{
		if (L.Contains(TEXT("pianificazione"))) { bRigaDiMondo = true; break; }
	}
	TestTrue(*FString::Printf(TEXT("il log filtrato non e' vuoto (%d righe)"), Visibili.Num()),
		Visibili.Num() > 0);
	TestTrue(TEXT("e conserva le righe di mondo"), bRigaDiMondo);

	RTCombatLogFixture::DestroyWorld(World);
	return true;
}
/**
 * 🔴 **Il giocatore legge il turno della PROPRIA squadra.**
 *
 * Sembra ovvio, e nessun test lo diceva: tutta la copertura del combat log era di sola ASSENZA — nessun
 * leak, nessuna cella di nemico — quindi un filtro che avesse soppresso *tutto* sarebbe stato verde.
 *
 * ⚠️ **E' il gemello obbligatorio del filtro**, e l'ha reso necessario [D-223]: da quando il verdetto si
 * congela alla scrittura, un canale che non riesce a calcolarlo produce righe che nessuno legge — in
 * silenzio, perche' il fail-closed non ha voce. Il canale derivato dal TurnLog e' proprio quello che genera
 * la maggior parte delle righe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLogShowsOwnTeamTurnTest,
	"RefactorTactics.UI.LogShowsOwnTeamTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLogShowsOwnTeamTurnTest::RunTest(const FString&)
{
	UWorld* World = RTCombatLogFixture::MakeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	RTCombatLogFixture::SpawnMap(World);

	// Due unita' della squadra 0, vicine: si vedono fra loro, e la squadra 1 non esiste in campo.
	ARTUnit* Mia = RTCombatLogFixture::SpawnUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Compagna = RTCombatLogFixture::SpawnUnit(World, 0, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mia || !Compagna) { RTCombatLogFixture::DestroyWorld(World); return false; }

	Mia->PlannedCell = FRTCellId(2, -1);
	Compagna->PlannedCell = Compagna->Cell;

	RTCombatLogFixture::RunTurn(TM);

	const TArray<FString> Visibili = TM->GetRecentEventsForTeam(0);
	const TArray<FString> Tutte    = TM->GetRecentEvents();

	// Anti-vacuita': il turno ha davvero prodotto righe. Senza, le asserzioni sotto sarebbero vuote.
	if (!TestTrue(TEXT("il turno ha prodotto righe di log"), Tutte.Num() > 0))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	// 🔴 Il cuore: in campo ci sono SOLO unita' della squadra 0, e una squadra conosce sempre i propri.
	// Quindi non c'e' una sola riga che la squadra 0 non debba poter leggere: visibili == totali.
	//
	// ⚠️ Il criterio e' il CONTEGGIO e non il nome dell'unita': `DescribeEntry` — che produce le righe del
	// canale derivato — stampa `ActionId` e coordinate, mai `GetName()`. Una prima stesura cercava il nome
	// e falliva su entrambe le asserzioni, controprova inclusa: era rossa per lo scenario, non per il
	// difetto. La controprova ha fatto il suo lavoro.
	if (Visibili.Num() != Tutte.Num())
	{
		for (const FString& L : Tutte)
		{
			if (!Visibili.Contains(L))
			{
				AddError(FString::Printf(TEXT("riga soppressa alla squadra che possiede il soggetto: %s"), *L));
			}
		}
	}
	TestEqual(TEXT("la squadra 0 legge ogni riga del proprio turno"), Visibili.Num(), Tutte.Num());

	RTCombatLogFixture::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **La morte e' pubblica: l'eliminazione la legge anche chi non vedeva la vittima cadere.**
 *
 * [D-223], decisione d'autore del 2026-08-28. E' l'unica eccezione dichiarata alla regola del verdetto
 * congelato, e senza un test non farebbe rumore: rimettere `Unit(...)` su quei siti produrrebbe un log che
 * a prima vista sembra giusto — le righe ci sono, per chi vedeva — e nessuna asserzione se ne accorgerebbe.
 *
 * ⚠️ **La verifica sta nell'ASIMMETRIA, non nella presenza.** Il test costruisce una vittima che la squadra
 * osservatrice non ha mai visto, e chiede due cose insieme: che l'annuncio della morte arrivi, e che le
 * righe ordinarie sulla stessa unita' **no**. Un test che chiedesse solo la prima passerebbe anche se il
 * filtro fosse spento del tutto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLogDeathIsPublicTest,
	"RefactorTactics.UI.DeathIsPublicEvenToWhoNeverSawIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLogDeathIsPublicTest::RunTest(const FString&)
{
	UWorld* World = RTCombatLogFixture::MakeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	RTCombatLogFixture::SpawnMap(World);

	// L'osservatore sta da una parte; la vittima e la sua compagna dall'altra, lontane e fuori vista.
	ARTUnit* Osservatore = RTCombatLogFixture::SpawnUnit(World, 0, FRTCellId(-5, 0));
	ARTUnit* Vittima     = RTCombatLogFixture::SpawnUnit(World, 1, FRTCellId(5, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Osservatore || !Vittima) { RTCombatLogFixture::DestroyWorld(World); return false; }

	Osservatore->PlannedCell = Osservatore->Cell;
	Vittima->PlannedCell = Vittima->Cell;
	RTCombatLogFixture::RunTurn(TM);   // un turno perche' il roster assegni gli StableUnitId

	// La premessa si ASSERISCE, non si spera: per la squadra 0 la vittima e' davvero ignota.
	const FRTTeamKnowledge K = TM->KnowledgeForTeamPublic(0);
	const ERTTargetKnowledge Classe =
		URTTeamKnowledgeLibrary::ClassifyTarget(K, Vittima->StableUnitId, Vittima->TeamId, Vittima->Cell);
	if (!TestTrue(TEXT("premessa: la vittima e' ignota alla squadra 0"),
		Classe == ERTTargetKnowledge::Rejected))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	// Due righe sulla stessa unita': una ordinaria, col verdetto congelato dalla conoscenza vera; e
	// l'annuncio della morte, che porta `Everyone()` come i cinque siti di eliminazione.
	//
	// ⚠️ **Costruite qui e non via `AddLogEvent`**, che e' `protected` — ed e' giusto che lo sia: quella
	// protezione e' cio' che rende gli 80 call site della risoluzione l'insieme COMPLETO. Il test verifica
	// quindi la regola, non il cablaggio dei cinque siti: quello lo tiene il compilatore, perche' `World()`
	// e `Unit()` sono tipi diversi e nessuno dei due ha una conversione implicita.
	const FString NomeVittima = Vittima->GetName();

	FRTKnowledgeSubject SubjVittima;
	SubjVittima.StableUnitId = Vittima->StableUnitId;
	SubjVittima.TeamId = Vittima->TeamId;
	SubjVittima.Cell = Vittima->Cell;
	SubjVittima.bAlive = true;

	FRTCombatLogLine Ordinaria;
	Ordinaria.Text = FString::Printf(TEXT("%s: riga ordinaria"), *NomeVittima);
	Ordinaria.SubjectStableUnitId = Vittima->StableUnitId;
	Ordinaria.Verdict = URTTeamKnowledgeLibrary::FreezeVerdict({ K }, SubjVittima);

	FRTCombatLogLine Morte;
	Morte.Text = FString::Printf(TEXT("Eliminata: %s (team %d)"), *NomeVittima, Vittima->TeamId);
	Morte.SubjectStableUnitId = INDEX_NONE;
	Morte.Verdict = FRTKnowledgeVerdict::Everyone();

	const TArray<FString> Visibili =
		URTCombatLogLibrary::ComposeVisibleLogLines({ Ordinaria, Morte }, /*ObserverTeamId*/ 0);

	bool bVedeLaMorte = false, bVedeLOrdinaria = false;
	for (const FString& L : Visibili)
	{
		if (L.StartsWith(TEXT("Eliminata: ")) && L.Contains(NomeVittima)) { bVedeLaMorte = true; }
		if (L.Contains(TEXT(": riga ordinaria")))                        { bVedeLOrdinaria = true; }
	}

	TestTrue (TEXT("la morte si legge anche senza aver visto la vittima"), bVedeLaMorte);

	// 🔴 L'altra meta': il filtro NON e' spento. Senza questa, la prima asserzione passerebbe comunque.
	TestFalse(TEXT("ma una riga ordinaria sulla stessa unita' resta nascosta"), bVedeLOrdinaria);

	RTCombatLogFixture::DestroyWorld(World);
	return true;
}

/**
 * L'esempio eseguibile di `#1498`, sul percorso vero: **chi muore nel turno lascia comunque il racconto di
 * cio' che ha fatto prima di cadere.**
 *
 * Era il difetto che la issue descriveva: il filtro girava in LETTURA, e nell'istante in cui l'unita' veniva
 * distrutta ogni riga gia' scritta che la nominava perdeva la propria voce nella vista — retroattivamente
 * su tutto il buffer. Il giocatore perdeva la narrazione del turno che voleva rileggere.
 *
 * Con [D-223] il verdetto e' congelato quando la riga nasce, cioe' mentre il soggetto e' ancora vivo e
 * osservabile: la morte non lo tocca piu'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLogKeepsTheTurnOfTheFallenTest,
	"RefactorTactics.UI.LogKeepsTheTurnOfTheFallen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLogKeepsTheTurnOfTheFallenTest::RunTest(const FString&)
{
	UWorld* World = RTCombatLogFixture::MakeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	RTCombatLogFixture::SpawnMap(World);

	// Due unita' vicine che si colpiscono nello stesso Blast — i turni sono simultanei, quindi entrambi i
	// colpi partono. La mia ha vita bassa: cade nello stesso segmento in cui mette a segno il proprio.
	ARTUnit* Mia = RTCombatLogFixture::SpawnUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Nemica = RTCombatLogFixture::SpawnUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mia || !Nemica) { RTCombatLogFixture::DestroyWorld(World); return false; }

	Mia->Health = 1;
	Mia->Shield = 0;
	Mia->PlannedAbilityIndex = 0;
	Mia->PlannedAttackTarget = Nemica;
	Nemica->PlannedAbilityIndex = 0;
	Nemica->PlannedAttackTarget = Mia;

	const int32 VitaNemicaPrima = Nemica->Health + Nemica->Shield;

	RTCombatLogFixture::RunTurn(TM);

	// Le due premesse si ASSERISCONO: senza, il test misurerebbe uno scenario che non e' accaduto.
	if (!TestFalse(TEXT("premessa: la mia unita' e' caduta"), Mia->IsAlive()))
	{
		RTCombatLogFixture::DestroyWorld(World); return false;
	}
	if (!TestTrue(TEXT("premessa: prima di cadere ha messo a segno il colpo"),
		(Nemica->Health + Nemica->Shield) < VitaNemicaPrima))
	{
		RTCombatLogFixture::DestroyWorld(World); return false;
	}

	// La lettura avviene DOPO `ConcludeTurn`, che e' la finestra in cui il difetto si manifestava: l'attore
	// e' gia' distrutto e la vista costruita adesso non avrebbe piu' una voce per lui.
	const TArray<FString> Visibili = TM->GetRecentEventsForTeam(0);

	bool bAnnuncioMorte = false, bColpoInflitto = false;
	for (const FString& L : Visibili)
	{
		if (L.Contains(TEXT("Eliminata: ")) || L.Contains(TEXT("Morte mostrata"))) { bAnnuncioMorte = true; }
		// Il racconto del colpo: la riga derivata con l'esito, dove il soggetto e' l'ATTACCANTE — cioe' la
		// mia unita' caduta. E' precisamente la riga che il filtro in lettura faceva sparire.
		if (L.Contains(TEXT("danni"))) { bColpoInflitto = true; }
	}

	TestTrue(TEXT("l'annuncio della morte si legge"), bAnnuncioMorte);
	TestTrue(*FString::Printf(TEXT("e si legge anche il colpo che ha messo a segno prima di cadere (%d righe)"),
		Visibili.Num()), bColpoInflitto);

	RTCombatLogFixture::DestroyWorld(World);
	return true;
}

/**
 * Il gemello di `#1498`, e asserisce il MECCANISMO: **un nemico mai rilevato che muore lascia l'annuncio,
 * non il racconto.**
 *
 * 🔴 **Ed e' il meccanismo che conta, non l'esito.** Prima di [D-223] una riga su un'unita' morta spariva
 * perche' il soggetto non esisteva piu' — l'attore era distrutto e la vista non aveva una voce per lui.
 * Adesso l'annuncio resta (e' `World()`) e il racconto no, perche' il verdetto congelato di quella voce
 * dice `Rejected`. Stessa assenza, ragione opposta: un test che chiedesse solo «non si vede» passerebbe in
 * entrambi i mondi, compreso quello rotto.
 *
 * ⚠️ **La causa della morte e' scelta, non casuale.** `Status.Burning` produce una voce `Lethal` il cui
 * soggetto e' la VITTIMA (`RTTurnManager` accoda la voce all'unita' che brucia). Con un colpo del Blast il
 * soggetto sarebbe stato l'ATTACCANTE — cioe' un'unita' propria — e la riga si sarebbe vista di diritto:
 * il test avrebbe misurato la convenzione del soggetto invece del filtro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLogUnseenDeathLeavesOnlyTheAnnouncementTest,
	"RefactorTactics.UI.UnseenDeathLeavesOnlyTheAnnouncement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLogUnseenDeathLeavesOnlyTheAnnouncementTest::RunTest(const FString&)
{
	UWorld* World = RTCombatLogFixture::MakeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	RTCombatLogFixture::SpawnMap(World, /*Radius=*/ 12);

	// Lontani e voltati: la squadra 0 non ha mai visto la vittima.
	ARTUnit* Osservatore = RTCombatLogFixture::SpawnUnit(World, 0, FRTCellId(-10, 0));
	ARTUnit* Vittima     = RTCombatLogFixture::SpawnUnit(World, 1, FRTCellId(10, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Osservatore || !Vittima) { RTCombatLogFixture::DestroyWorld(World); return false; }

	// Brucia, e non ha abbastanza vita per arrivare al turno dopo.
	Vittima->Shield = 0;
	Vittima->Health = 1;
	Vittima->ApplyStatus(TAG_Status_Burning, /*Turns=*/ 3);

	Osservatore->PlannedCell = Osservatore->Cell;
	Vittima->PlannedCell = Vittima->Cell;

	RTCombatLogFixture::RunTurn(TM);

	if (!TestFalse(TEXT("premessa: la vittima e' caduta"), Vittima->IsAlive()))
	{
		RTCombatLogFixture::DestroyWorld(World); return false;
	}

	const TArray<FString> Visibili = TM->GetRecentEventsForTeam(0);
	const TArray<FString> Tutte    = TM->GetRecentEvents();

	// Anti-vacuita' sul RACCONTO: la riga con le celle deve esistere nel log completo, o l'assenza sotto
	// non significherebbe nulla. E' lo stesso errore che una prima stesura di `LogShowsOwnTeamTurn` ha
	// fatto: rossa per lo scenario, non per il difetto.
	bool bRaccontoEsiste = false;
	for (const FString& L : Tutte)
	{
		if (L.Contains(TEXT("eliminata")) && L.Contains(TEXT("q="))) { bRaccontoEsiste = true; break; }
	}
	if (!TestTrue(TEXT("anti-vacuita': il racconto col le celle esiste nel log completo"), bRaccontoEsiste))
	{
		RTCombatLogFixture::DestroyWorld(World); return false;
	}

	bool bAnnuncio = false, bRacconto = false;
	for (const FString& L : Visibili)
	{
		if (L.Contains(TEXT("eliminato dalle fiamme")))                { bAnnuncio = true; }
		if (L.Contains(TEXT("eliminata")) && L.Contains(TEXT("q=")))   { bRacconto = true; }
	}

	TestTrue (TEXT("l'annuncio della morte arriva anche a chi non l'ha mai vista"), bAnnuncio);
	TestFalse(TEXT("ma il racconto con le celle NON arriva"), bRacconto);

	// 🔴 E la ragione dell'assenza si asserisce, invece di dedurla dall'assenza stessa: il verdetto di
	// quella vittima, per la squadra 0, e' `Rejected` — non «il soggetto non esiste piu'».
	const FRTTeamKnowledge K = TM->KnowledgeForTeamPublic(0);
	const ERTTargetKnowledge Classe =
		URTTeamKnowledgeLibrary::ClassifyTarget(K, Vittima->StableUnitId, Vittima->TeamId, Vittima->Cell);
	TestTrue(TEXT("e l'assenza viene da Rejected, non dal soggetto sparito"),
		Classe == ERTTargetKnowledge::Rejected);

	RTCombatLogFixture::DestroyWorld(World);
	return true;
}

namespace RT1932
{
	/** Una voce `Move` minima, con il soggetto che si vuole provare. */
	FRTTurnLogEntry Movimento(int32 UnitId, ERTMoveOutcome Esito, ERTMatchPhase Fase)
	{
		FRTTurnLogEntry E;
		E.TurnNumber = 1;
		E.UnitId = UnitId;
		E.Phase = Fase;
		E.Category = ERTLogCategory::Move;
		E.Outcome = static_cast<uint8>(Esito);
		E.SrcCell = FRTCellId(-1, -1, 0);
		E.TgtCell = FRTCellId(1, -1, 0);
		E.Amount = 2;
		return E;
	}

	/** Il soggetto in testa alla riga, o stringa vuota se la riga non ne dichiara uno. */
	FString Soggetto(const FString& Riga)
	{
		int32 Colon = INDEX_NONE;
		return Riga.FindChar(TEXT(':'), Colon) ? Riga.Left(Colon) : FString();
	}
}

/**
 * 🔴 **Una riga di movimento dice CHI si e' mosso.**
 *
 * Il soggetto viaggiava gia' in `SubjectStableUnitId` — serviva al filtro di conoscenza — ma nel TESTO non
 * entrava mai. Chi leggeva il log non aveva modo di attribuire una riga, ed e' cosi' che
 * [#1733](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1733) e' stata aperta come bug di
 * gameplay su un comportamento corretto.
 *
 * ⚠️ Il prefisso vale solo dove `UnitId` e' anche il soggetto GRAMMATICALE: nelle voci di danno porta chi
 * **subisce** (#1150), e «Gadget: colpisce» direbbe il falso. Il test lo pinna, altrimenti la prossima
 * estensione lo scopre a schermo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveLineNamesItsSubjectTest,
	"RefactorTactics.UI.MoveLineNamesItsSubject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveLineNamesItsSubjectTest::RunTest(const FString&)
{
	using namespace RT1932;

	const FRTTurnLogEntry Mossa = Movimento(3, ERTMoveOutcome::Moved, ERTMatchPhase::Move);

	// --- 1. Col nome risolto dal chiamante -----------------------------------------------------------
	{
		TMap<int32, FString> Nomi;
		Nomi.Add(3, TEXT("Gadget"));
		const TArray<FRTDescribedLine> Righe = URTTurnLogLibrary::DescribeTurnLogWithSubjects({ Mossa }, Nomi);
		if (!TestEqual(TEXT("una riga per voce"), Righe.Num(), 1)) { return false; }
		TestEqual(TEXT("la riga nomina il soggetto"), Soggetto(Righe[0].Text), FString(TEXT("Gadget")));
		TestTrue(*FString::Printf(TEXT("e conserva il predicato: %s"), *Righe[0].Text),
			Righe[0].Text.Contains(TEXT("si muove")));
		// Il dato per il filtro di conoscenza non cambia: il testo si aggiunge, non sostituisce.
		TestEqual(TEXT("il soggetto resta anche come dato"), Righe[0].SubjectStableUnitId, 3);
	}

	// --- 2. Senza mappa: l'id stabile, che e' brutto ma vero -----------------------------------------
	{
		const TArray<FRTDescribedLine> Righe = URTTurnLogLibrary::DescribeTurnLogWithSubjects({ Mossa });
		if (!TestEqual(TEXT("una riga per voce"), Righe.Num(), 1)) { return false; }
		TestEqual(TEXT("ripiega sull'id stabile"), Soggetto(Righe[0].Text), FString(TEXT("u3")));
	}

	// --- 3. `UnitId == 0`: nessun soggetto finto ----------------------------------------------------
	{
		const FRTTurnLogEntry Senza = Movimento(0, ERTMoveOutcome::Moved, ERTMatchPhase::Move);
		const TArray<FRTDescribedLine> Righe = URTTurnLogLibrary::DescribeTurnLogWithSubjects({ Senza });
		if (!TestEqual(TEXT("una riga per voce"), Righe.Num(), 1)) { return false; }
		TestFalse(*FString::Printf(TEXT("nessun «u0» inventato: %s"), *Righe[0].Text),
			Righe[0].Text.StartsWith(TEXT("u0"), ESearchCase::CaseSensitive));
		TestEqual(TEXT("la riga e' il solo predicato"),
			Righe[0].Text, URTTurnLogLibrary::DescribeEntry(Senza));
	}

	// --- 4. 🔴 Una voce di DANNO non prende il prefisso: li' `UnitId` e' chi SUBISCE ------------------
	{
		FRTTurnLogEntry Colpo;
		Colpo.TurnNumber = 1;
		Colpo.UnitId = 7;
		Colpo.Phase = ERTMatchPhase::Blast;
		Colpo.Category = ERTLogCategory::Combat;
		Colpo.Outcome = static_cast<uint8>(ERTCombatOutcome::Hit);
		Colpo.SrcCell = FRTCellId(0, 0, 0);
		Colpo.TgtCell = FRTCellId(1, 0, 0);
		Colpo.Amount = 12;

		TMap<int32, FString> Nomi;
		Nomi.Add(7, TEXT("Gadget"));
		const TArray<FRTDescribedLine> Righe = URTTurnLogLibrary::DescribeTurnLogWithSubjects({ Colpo }, Nomi);
		if (!TestEqual(TEXT("una riga per voce"), Righe.Num(), 1)) { return false; }
		TestFalse(*FString::Printf(TEXT("il difensore non diventa il soggetto della frase: %s"), *Righe[0].Text),
			Righe[0].Text.StartsWith(TEXT("Gadget:"), ESearchCase::CaseSensitive));
		// Ma il soggetto come DATO resta: e' quello che il filtro di conoscenza usa.
		TestEqual(TEXT("e resta il soggetto per la conoscenza"), Righe[0].SubjectStableUnitId, 7);
	}

	// --- 5. `DescribeEntry` invariata: `DescribeReportLine` stampa gia' `unita=` ---------------------
	TestFalse(TEXT("il predicato non porta il soggetto"),
		URTTurnLogLibrary::DescribeEntry(Mossa).StartsWith(TEXT("u3"), ESearchCase::CaseSensitive));

	return true;
}

/**
 * 🔴 **Il caso che ha prodotto #1733: due righe, una unita' sola.**
 *
 * ```text
 * Turno 5:  si muove (-1,-1) -> (1,-1)  (Hero.Wraith.PassingBlade, p30)
 *           resta    (1,-1)             (Action.Move, p50)
 * ```
 *
 * Senza soggetto si leggono come *«uno arriva, un altro ci sta gia'»*, cioe' una sovrapposizione — che non
 * era avvenuta. Il referto dello spec panel l'ha misurato: quelle celle erano libere.
 *
 * ⚠️ Il test NON asserisce che il «resta» spieghi di essere il seguito di uno scatto: quello vorrebbe un
 * valore nuovo in `ERTMoveOutcome`, che e' **serializzato in v7** e riprodotto dal replay. Resta dichiarato
 * come lavoro separato: qui si prova solo che le due righe si attribuiscano alla stessa unita'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTwoLinesSameUnitSameSubjectTest,
	"RefactorTactics.UI.TwoLinesOfTheSameUnitCarryTheSameSubject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTwoLinesSameUnitSameSubjectTest::RunTest(const FString&)
{
	using namespace RT1932;

	TMap<int32, FString> Nomi;
	Nomi.Add(3, TEXT("Wraith"));
	Nomi.Add(5, TEXT("Riktor"));

	const TArray<FRTTurnLogEntry> Log = {
		Movimento(3, ERTMoveOutcome::Moved,  ERTMatchPhase::Dash),
		Movimento(3, ERTMoveOutcome::Stayed, ERTMatchPhase::Move),
		Movimento(5, ERTMoveOutcome::Stayed, ERTMatchPhase::Move),
	};

	const TArray<FRTDescribedLine> Righe = URTTurnLogLibrary::DescribeTurnLogWithSubjects(Log, Nomi);
	if (!TestEqual(TEXT("tre voci, tre righe"), Righe.Num(), 3)) { return false; }

	TArray<FString> DiWraith;
	TArray<FString> DiAltri;
	for (const FRTDescribedLine& Riga : Righe)
	{
		(Riga.SubjectStableUnitId == 3 ? DiWraith : DiAltri).Add(Soggetto(Riga.Text));
	}

	if (!TestEqual(TEXT("due righe sono della stessa unita'"), DiWraith.Num(), 2)) { return false; }
	TestEqual(TEXT("e portano lo stesso soggetto"), DiWraith[0], DiWraith[1]);
	TestEqual(TEXT("che e' il nome dell'unita'"), DiWraith[0], FString(TEXT("Wraith")));

	if (!TestEqual(TEXT("la terza e' di un'altra"), DiAltri.Num(), 1)) { return false; }
	TestNotEqual(TEXT("e si distingue dalle prime due"), DiAltri[0], DiWraith[0]);

	return true;
}


/**
 * NEMMENO UNA MOSSA BLOCCATA SI RIPETE — ed e' l'ottavo duplicato che `LogDoesNotRepeatTheDerivedLines`
 * dichiara di non coprire.
 *
 * 🔴 **Il difetto e' NUOVO, e nessuno lo ha introdotto sbagliando.** `RTTurnManager.cpp` scrive a mano
 * `«Nome: <predicato>»` per le mosse `BlockedContested` / `BlockedByUnit`, e fino a `#1932` non era un
 * duplicato: la riga derivata dal TurnLog non portava il soggetto, quindi le due erano *«la stessa
 * informazione in due formati»* — che e' la ragione per cui `#1412` non le ha tolte tutte.
 *
 * Poi `#1932` ha dato il soggetto alle voci `Move` — **l'unica categoria in cui `UnitId` e' anche il
 * soggetto grammaticale** — e da quel momento la derivazione rende esattamente la stessa stringa. Il
 * giocatore legge due volte che la sua unita' non e' passata.
 *
 * ⚠️ **La fixture di `LogDoesNotRepeatTheDerivedLines` non poteva vederlo**: due unita' che si muovono
 * senza contendersi una cella non producono nessun esito bloccato, e il suo commento di copertura lo
 * dichiara. Qui le due mosse puntano alla STESSA cella, che e' la sola configurazione che lo esercita.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlockedMoveIsNotRepeatedTest,
	"RefactorTactics.UI.BlockedMoveLineIsNotRepeated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlockedMoveIsNotRepeatedTest::RunTest(const FString&)
{
	// Due unita' affiancate che puntano alla stessa cella: contesa, quindi almeno una resta dov'e'.
	RTCombatLogFixture::FTwoUnitTurn Fixture;
	if (!RTCombatLogFixture::BuildTwoUnitTurn(*this, Fixture, FRTCellId(0, 0, 0), FRTCellId(2, 0, 0)))
	{
		RTCombatLogFixture::DestroyWorld(Fixture.World);
		return false;
	}

	const FRTCellId Conteso(1, 0, 0);
	// ⚠️ **I waypoint da soli non muovono nessuno**, e il primo giro di questo test lo ha misurato:
	// il resolver legge `PlannedPath` e `PlannedCell`, e con i soli waypoint la contesa non avveniva —
	// il test sarebbe stato verde su un turno in cui nessuno si era mosso. L'ha intercettato la riga
	// anti-vacuita' qui sotto.
	Fixture.A->PlannedWaypoints = { Conteso };
	Fixture.A->PlannedPath = { Fixture.A->Cell, Conteso };
	Fixture.A->PlannedCell = Conteso;
	Fixture.A->PlannedAbilityIndex = INDEX_NONE;
	Fixture.B->PlannedWaypoints = { Conteso };
	Fixture.B->PlannedPath = { Fixture.B->Cell, Conteso };
	Fixture.B->PlannedCell = Conteso;
	Fixture.B->PlannedAbilityIndex = INDEX_NONE;

	RTCombatLogFixture::RunTurn(Fixture.TM);

	// ANTI-VACUITA': senza un esito bloccato il test non esercita il ramo, e il conteggio sotto sarebbe
	// verde su un turno in cui nessuno si e' fermato — il modo piu' facile di non misurare niente.
	bool bQualcunoBloccato = false;
	for (const FRTTurnLogEntry& Entry : Fixture.TM->GetTurnLog())
	{
		if (Entry.Category != ERTLogCategory::Move) { continue; }
		const ERTMoveOutcome Esito = static_cast<ERTMoveOutcome>(Entry.Outcome);
		if (Esito == ERTMoveOutcome::BlockedContested || Esito == ERTMoveOutcome::BlockedByUnit)
		{
			bQualcunoBloccato = true;
			break;
		}
	}
	if (!TestTrue(TEXT("la contesa ha davvero bloccato qualcuno"), bQualcunoBloccato))
	{
		RTCombatLogFixture::DestroyWorld(Fixture.World);
		return false;
	}

	// Stessa regola di conteggio di `LogDoesNotRepeatTheDerivedLines`: corrispondenza esatta, o esatta
	// preceduta da `«Nome: »`, contata per riga unica contro le sue occorrenze attese.
	// Il PREDICATO nudo, non la riga intera: e' la chiave che vede il difetto. Le due copie portano
	// prefissi DIVERSI — `Units[i]->GetName()` rende `RTUnit_0`, mentre la derivata usa il nome risolto da
	// `SubjectNamesForLog()` e rende `Wraith` — quindi confrontare la riga intera, come fa
	// `LogDoesNotRepeatTheDerivedLines`, non le fa combaciare e il duplicato passa.
	//
	// 🔴 **E le due righe non sono solo doppie: si contraddicono.** Lo stesso evento arriva al
	// giocatore attribuito a due entita' che sembrano diverse.
	TArray<FString> Predicati;
	for (const FRTTurnLogEntry& Entry : Fixture.TM->GetTurnLog())
	{
		if (Entry.Category != ERTLogCategory::Move) { continue; }
		const ERTMoveOutcome Esito = static_cast<ERTMoveOutcome>(Entry.Outcome);
		if (Esito == ERTMoveOutcome::BlockedContested || Esito == ERTMoveOutcome::BlockedByUnit)
		{
			Predicati.Add(URTTurnLogLibrary::DescribeEntry(Entry));
		}
	}
	const TArray<FString> Emesse = Fixture.TM->GetRecentEvents();
	RTCombatLogFixture::DestroyWorld(Fixture.World);

	for (const FString& Predicato : Predicati)
	{
		int32 Conta = 0;
		for (const FString& Emessa : Emesse)
		{
			if (Emessa.EndsWith(Predicato, ESearchCase::CaseSensitive)) { ++Conta; }
		}
		TestEqual(*FString::Printf(TEXT("«%s» arriva al giocatore una volta sola"), *Predicato), Conta, 1);
	}

	return true;
}

/**
 * ✅ **`UsedByBlast` entra in una traccia REALE** (`#1933`) — il residuo si chiude, **per il ramo della
 * Guardia**.
 *
 * ⚠️ **Il perimetro va detto subito, perché è più stretto del nome del consumatore.** La lettura registrata
 * è quella di `ResolveCombatPasses`, che gira **solo per le unità con `Status.Guarded`**: è la Guardia a
 * chiedere se il colpo viene dall'arco frontale. L'altra lettura del Blast — la copertura generale, in
 * `EffectiveCoverReduction` → `IsInFrontalArc` — vive in una funzione **pura**, senza log: registrarla da
 * lì vorrebbe dire passare un `TArray<FRTTurnLogEntry>&` dentro il resolver, che è il confine che
 * `RTCombatResolver.h` dichiara di tenere (*«il resolver resta puro: nessun Actor, nessuna mappa»*).
 *
 * 🔴 **Il difetto che questo test chiude era invisibile a un test verde.**
 * `Facing.TurnLogNamesConsumerAndReason` muta **a mano** una voce `UsedByBlast` per verificare che l'hash
 * distingua i consumatori: misura il TurnLog, non chi lo produce. `ReadFacingForConsumer` aveva **zero
 * chiamanti in gioco** — `RTTurnLog.h` lo dichiarava — quindi quella voce non entrava in nessuna traccia
 * prodotta dal gioco, e nessun test se ne accorgeva perché nessuno la cercava in una traccia vera.
 *
 * ∴ qui la traccia è quella di un turno **giocato**: se il produttore sparisse, questo test diventerebbe
 * rosso e quello di `RTFacingTests` resterebbe verde.
 *
 * ⚠️ Cosa il test NON afferma: nulla su `UsedByOverwatch`. Quel consumatore **non ha oggetto** — l'Overwatch
 * decide con quattro condizioni e nessuna riguarda l'orientamento — e resta deliberatamente senza
 * produttore. Registrarlo dichiarerebbe una lettura mai avvenuta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatLogBlastRecordsFacingReadTest,
	"RefactorTactics.Facing.BlastRecordsTheFacingItRead",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatLogBlastRecordsFacingReadTest::RunTest(const FString&)
{
	UWorld* World = RTCombatLogFixture::MakeWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTHexMapActor* Map = RTCombatLogFixture::SpawnMap(World);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Difensore = RTCombatLogFixture::SpawnUnit(World, /*TeamId=*/ 1, FRTCellId(0, 0, 0));
	ARTUnit* Attaccante = RTCombatLogFixture::SpawnUnit(World, /*TeamId=*/ 0, FRTCellId(-1, 0, 0));
	if (!TestNotNull(TEXT("turn manager"), TM) || !TestNotNull(TEXT("mappa"), Map)
		|| !TestNotNull(TEXT("difensore"), Difensore) || !TestNotNull(TEXT("attaccante"), Attaccante))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	Difensore->Facing = ERTHexDirection::E;
	// 🔴 **La Guardia non e' un dettaglio della fixture: e' la CONDIZIONE del ramo che legge il facing.**
	// `ResolveCombatPasses` salta le unita' senza `Status.Guarded` — `if (!Units[i]->HasStatus(...)) continue`
	// — quindi senza questa riga nessuna lettura avviene e il test misurerebbe l'assenza di un produttore
	// che invece esiste. Misurato: la prima stesura non ce l'aveva ed e' fallita per questo.
	Difensore->ApplyStatus(TAG_Status_Guarded, 1);
	Attaccante->PlannedAbilityIndex = 0; // attacco base
	Attaccante->PlannedAttackTarget = Difensore;

	RTCombatLogFixture::RunTurn(TM);

	const TArray<FRTTurnLogEntry>& Log = TM->GetTurnLog();
	const TArray<FRTTurnLogEntry> Letture = Log.FilterByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Facing
			&& E.Outcome == static_cast<uint8>(ERTFacingOutcome::UsedByBlast);
	});

	if (!TestTrue(TEXT("il Blast ha registrato il facing che ha letto"), Letture.Num() > 0))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	// --- 🔴 il VALORE registrato e' quello letto, non un default ---------------------------------------
	// Senza questa asserzione una voce con `Amount` a zero passerebbe: e' il modo in cui un produttore
	// «presente» puo' comunque non dire nulla.
	const FRTTurnLogEntry& Lettura = Letture[0];
	TestEqual(TEXT("e il valore e' il facing del difensore, non uno zero"),
		Lettura.Amount, static_cast<int32>(ERTHexDirection::E));

	// --- la voce nomina l'unita' di cui racconta l'orientamento ----------------------------------------
	TestEqual(TEXT("e nomina il difensore, non l'attaccante"),
		Lettura.UnitId, Difensore->StableUnitId);
	TestEqual(TEXT("nella fase Blast"), Lettura.Phase, ERTMatchPhase::Blast);

	// --- ⛔ e NON registra il consumatore che non legge nulla ------------------------------------------
	// `UsedByOverwatch` resta senza produttore per una ragione misurata, non per dimenticanza: se un
	// giorno comparisse in una traccia senza che il cono pianificato esista, sarebbe un dato falso.
	TestEqual(TEXT("⛔ nessuna voce UsedByOverwatch: quel consumatore non legge nulla"),
		Log.FilterByPredicate([](const FRTTurnLogEntry& E)
		{
			return E.Category == ERTLogCategory::Facing
				&& E.Outcome == static_cast<uint8>(ERTFacingOutcome::UsedByOverwatch);
		}).Num(), 0);

	RTCombatLogFixture::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
