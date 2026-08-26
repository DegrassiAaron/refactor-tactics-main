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
#include "Core/RTGameplayTags.h" // TAG_Status_Guarded: la guardia si applica al difensore
#include "Turn/RTActionFallbackLibrary.h" // ERTActionInvalidReason nel motivo del fallback
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnManager.h"
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

	const TArray<FString> Attese = URTTurnLogLibrary::DescribeTurnLog(Log);
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
			&& E.Outcome == static_cast<uint8>(ERTFacingOutcome::RearHitBypassedCover);
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
				&& E.Outcome == static_cast<uint8>(ERTFacingOutcome::RearHitBypassedCover);
		}).Num(), 1);

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
 * **L'ALTRO produttore dello stesso esito accredita la stessa unita'.**
 *
 * `RearHitBypassedCover` non nasce solo dal ramo della Guard: `RTTurnManager.cpp:4112` lo emette anche
 * quando a essere scavalcata e' una COPERTURA. Anche quello accreditava l'attaccante, e la issue non lo
 * citava — quindi senza questo test la meta' meno visibile della correzione tornerebbe indietro senza che
 * niente diventi rosso: il test qui sopra gira su un'arena piatta, dove quel ramo non si esegue mai.
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
 * ⚠️ Il produttore oggi NON riempie `ActionId` su quelle voci, e la riga lo dichiara invece di mascherarlo.
 * Riempirlo cambia l'hash delle tracce, quindi e' un cambio d'identita' che va deciso a parte — e fino ad
 * allora la lacuna la vede chi legge il log.
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

	// Senza `ActionId`: la riga lo DICHIARA. Non `None`, che si leggerebbe come un id di azione vero.
	const FString Muta = URTTurnLogLibrary::DescribeEntry(Annullata);
	TestTrue(*FString::Printf(TEXT("dichiara che l'azione non c'e': %s"), *Muta),
		Muta.Contains(TEXT("azione non dichiarata")));
	TestFalse(*FString::Printf(TEXT("e non scrive 'None': %s"), *Muta), Muta.Contains(TEXT("None")));

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

	// E il profilo, quando c'e', si legge come nelle altre categorie.
	Altra.BaseActionId = FName(TEXT("Action.BasicAttack"));
	Altra.ActionId = FName(TEXT("Riktor.ImpactShot"));
	TestTrue(TEXT("azione base e profilo, come altrove"),
		URTTurnLogLibrary::DescribeEntry(Altra).Contains(TEXT("Action.BasicAttack · Riktor.ImpactShot")));

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
 * `==` non le vede. Il duplicato e' la stessa informazione in due FORMATI, e si trova per contenuto — che
 * e' anche il motivo per cui nessuno se n'era accorto prima.
 *
 * ⚠️ Copertura di questo percorso: due unita' che non attaccano. Le sette righe doppie vivono in rami che
 * questa fixture non attraversa (fallback, reazioni, colpi senza linea di tiro), quindi il test protegge
 * dall'**ottavo** duplicato piu' che misurare i sette — che restano aperti in `#1412` e non si tolgono
 * finche' `DescribeTurnLog` non sa nominare l'attore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLogDoesNotRepeatDerivedLinesTest,
	"RefactorTactics.UI.LogDoesNotRepeatTheDerivedLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLogDoesNotRepeatDerivedLinesTest::RunTest(const FString&)
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
	if (!TestTrue(TEXT("premessa: il turno ha prodotto almeno una voce"), Log.Num() > 0))
	{
		RTCombatLogFixture::DestroyWorld(World);
		return false;
	}

	const TArray<FString> Attese = URTTurnLogLibrary::DescribeTurnLog(Log);
	const TArray<FString>& Emesse = TM->GetRecentEvents();

	for (const FString& Riga : Attese)
	{
		int32 Conta = 0;
		for (const FString& Emessa : Emesse)
		{
			if (Emessa.Contains(Riga))
			{
				++Conta;
			}
		}
		TestEqual(*FString::Printf(TEXT("una volta sola nel combat log: %s"), *Riga), Conta, 1);
	}

	RTCombatLogFixture::DestroyWorld(World);
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
#endif // WITH_DEV_AUTOMATION_TESTS
