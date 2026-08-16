// CP 46.2 (#937) — gli esiti d'avvio e le fasi di caricamento.
//
// Questi test provano il **vocabolario**, che e' la parte istruita prima di scrivere il codice: quali
// esiti esistono, quali sono fatali, e come si comporta un report che ne accumula piu' d'uno.
//
// La forma segue quella di `RTFrontendNavigationTests.cpp`: tipi puri, nessun mondo, nessun widget.

#include "Misc/AutomationTest.h"
#include "Frontend/RTStartupReport.h"
#include "Frontend/RTFrontendWidgets.h"
#include "RTWorldFixtures.h"
#include "RTGameMode.h"
#include "Map/RTHexMapActor.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/**
	 * Tutti i valori dell'enum, scritti a mano **di proposito**.
	 *
	 * ⚠️ E' l'unico modo per accorgersi che qualcuno ne ha aggiunto uno: `StaticEnum()` conta i valori
	 * reali, questa lista conta quelli che i test conoscono, e il confronto fra i due numeri e' cio' che
	 * rende il «nessun esito resta non classificato» una misura invece di una speranza.
	 */
	const ERTStartupOutcome AllOutcomes[] = {
		ERTStartupOutcome::Ok,
		ERTStartupOutcome::FormatAssetInvalid,
		ERTStartupOutcome::ShippedFormatInvalid,
		ERTStartupOutcome::FormatMapMismatch,
		ERTStartupOutcome::UsingTestArena,
		ERTStartupOutcome::UsingDemoArena,
		ERTStartupOutcome::LevelMapMissing,
		ERTStartupOutcome::UsingFallbackFormat,
		ERTStartupOutcome::NoTurnManager
	};
}

/**
 * **Ogni esito appartiene a esattamente una categoria: `Ok`, fatale o degradato.**
 *
 * E' la proprieta' da cui dipende tutto il resto — `IsFatal` sceglie fra modale e banner, e un esito che
 * non cadesse in nessuna delle tre non verrebbe mostrato **da nessuna parte**: il difetto peggiore
 * possibile per un vocabolario che esiste per rendere visibile cio' che prima stava solo nel log.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStartupOutcomeIsAlwaysClassifiedTest,
	"RefactorTactics.Startup.EveryOutcomeIsExactlyOneCategory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStartupOutcomeIsAlwaysClassifiedTest::RunTest(const FString&)
{
	for (const ERTStartupOutcome Outcome : AllOutcomes)
	{
		const bool bOk = (Outcome == ERTStartupOutcome::Ok);
		const bool bFatal = URTStartupReportLibrary::IsFatal(Outcome);
		const bool bDegraded = URTStartupReportLibrary::IsDegraded(Outcome);

		const int32 Categories = (bOk ? 1 : 0) + (bFatal ? 1 : 0) + (bDegraded ? 1 : 0);
		if (!TestEqual(*FString::Printf(TEXT("esito %d in una sola categoria"), (int32)Outcome),
			Categories, 1))
		{
			return false;
		}
	}

	// 🔑 **Il controllo che rende vero il precedente anche domani.** Senza, aggiungere un nono esito e
	// dimenticarlo in `AllOutcomes` lascerebbe il test verde: proverebbe otto casi su nove e direbbe
	// «ogni esito», che e' la classe di bugia che questo repository paga piu' spesso.
	const UEnum* EnumType = StaticEnum<ERTStartupOutcome>();
	if (TestNotNull(TEXT("l'enum e' riflesso"), EnumType))
	{
		// `NumEnums()` include il `_MAX` sintetico che UHT aggiunge: si sottrae.
		TestEqual(TEXT("i test conoscono TUTTI i valori dell'enum"),
			EnumType->NumEnums() - 1, (int32)UE_ARRAY_COUNT(AllOutcomes));
	}

	return true;
}

/**
 * I tre fatali sono **esattamente** quelli in cui `ApplyMatchFormat` restituisce `false`.
 *
 * Il numero non e' arbitrario: e' misurato su `RTGameMode.cpp`, dove i `return false` sono tre. Se un
 * giorno diventassero quattro, questo test cade e chiede di aggiornare il vocabolario — che e' il suo
 * lavoro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStartupFatalSetTest,
	"RefactorTactics.Startup.FatalOutcomesAreTheThreeThatRefuseToStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStartupFatalSetTest::RunTest(const FString&)
{
	TestTrue(TEXT("formato assegnato invalido"),
		URTStartupReportLibrary::IsFatal(ERTStartupOutcome::FormatAssetInvalid));
	TestTrue(TEXT("formato spedito invalido"),
		URTStartupReportLibrary::IsFatal(ERTStartupOutcome::ShippedFormatInvalid));
	TestTrue(TEXT("formato e mappa non combaciano"),
		URTStartupReportLibrary::IsFatal(ERTStartupOutcome::FormatMapMismatch));

	// I ripieghi NON sono fatali: e' la meta' che il DoD originale non vedeva, e che un modale
	// nasconderebbe dietro un pulsante `BACK` invece di mostrarla in partita.
	TestFalse(TEXT("arena di prova"), URTStartupReportLibrary::IsFatal(ERTStartupOutcome::UsingTestArena));
	TestFalse(TEXT("arena demo"), URTStartupReportLibrary::IsFatal(ERTStartupOutcome::UsingDemoArena));
	TestFalse(TEXT("mappa assente"), URTStartupReportLibrary::IsFatal(ERTStartupOutcome::LevelMapMissing));
	TestFalse(TEXT("formato di ripiego"), URTStartupReportLibrary::IsFatal(ERTStartupOutcome::UsingFallbackFormat));
	TestFalse(TEXT("nessun turn manager"), URTStartupReportLibrary::IsFatal(ERTStartupOutcome::NoTurnManager));

	TestFalse(TEXT("Ok non e' fatale"), URTStartupReportLibrary::IsFatal(ERTStartupOutcome::Ok));
	TestFalse(TEXT("ne' degradato"), URTStartupReportLibrary::IsDegraded(ERTStartupOutcome::Ok));

	return true;
}

/**
 * **Un avvio accumula piu' ripieghi, e il banner deve mostrarli tutti.**
 *
 * E' il caso reale delle due riserve di `G13`: arena di PROVA **e** formato di RIPIEGO nella stessa
 * partita. Mostrarne uno solo nasconderebbe l'altro — ed e' esattamente il motivo per cui il 2026-08-10
 * nessuno se n'era accorto guardando lo schermo: le due cause stavano in due righe di log diverse.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStartupCollectsEveryDegradationTest,
	"RefactorTactics.Startup.ReportKeepsEveryDegradation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStartupCollectsEveryDegradationTest::RunTest(const FString&)
{
	FRTStartupReport Report;

	TestFalse(TEXT("un report vuoto non ha degradazioni"),
		URTStartupReportLibrary::HasDegradation(Report));
	TestEqual(TEXT("ne' fatali"),
		URTStartupReportLibrary::FindFatal(Report), ERTStartupOutcome::Ok);

	// Il caso di `G13`, quello vero.
	Report.Add(ERTStartupOutcome::UsingTestArena, TEXT("120 celle"));
	Report.Add(ERTStartupOutcome::UsingFallbackFormat, TEXT("Format.Fallback"));

	TestEqual(TEXT("due note, non una"), Report.Notes.Num(), 2);
	TestTrue(TEXT("il banner ha qualcosa da dire"), URTStartupReportLibrary::HasDegradation(Report));
	TestEqual(TEXT("ma nessun modale"),
		URTStartupReportLibrary::FindFatal(Report), ERTStartupOutcome::Ok);

	// L'ordine e' quello in cui le condizioni si sono presentate: la mappa prima del formato, come
	// nell'allestimento. Un banner che le riordinasse racconterebbe una sequenza che non e' avvenuta.
	TestEqual(TEXT("prima la mappa"), Report.Notes[0].Outcome, ERTStartupOutcome::UsingTestArena);
	TestEqual(TEXT("poi il formato"), Report.Notes[1].Outcome, ERTStartupOutcome::UsingFallbackFormat);

	// Il dettaglio viaggia con la nota: e' prodotto da chi rileva, non ricomposto da chi mostra.
	TestEqual(TEXT("il dettaglio e' trasportato"), Report.Notes[0].Detail, FString(TEXT("120 celle")));

	return true;
}

/**
 * `Add(Ok)` non produce una nota: `Ok` e' l'assenza di condizioni, non una condizione.
 *
 * Senza questa guardia ogni passo riuscito lascerebbe una riga, e il banner mostrerebbe un elenco di
 * successi — cioe' rumore esattamente dove serve il segnale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStartupOkIsNotANoteTest,
	"RefactorTactics.Startup.OkDoesNotBecomeANote",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStartupOkIsNotANoteTest::RunTest(const FString&)
{
	FRTStartupReport Report;
	Report.Add(ERTStartupOutcome::Ok);
	Report.Add(ERTStartupOutcome::Ok, TEXT("tutto bene"));

	TestEqual(TEXT("nessuna nota"), Report.Notes.Num(), 0);
	TestFalse(TEXT("niente banner"), URTStartupReportLibrary::HasDegradation(Report));

	return true;
}

/**
 * Un fatale si trova anche se **non e' il primo**: l'ordine delle note e' quello dell'allestimento, e la
 * mappa si risolve prima del formato — quindi un ripiego di mappa precede quasi sempre il rifiuto del
 * formato.
 *
 * Se `FindFatal` guardasse solo la prima nota, la partita mostrerebbe il **banner** di un ripiego mentre
 * in realta' non e' partita affatto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStartupFatalFoundAfterDegradationTest,
	"RefactorTactics.Startup.FatalIsFoundEvenAfterADegradation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStartupFatalFoundAfterDegradationTest::RunTest(const FString&)
{
	FRTStartupReport Report;
	Report.Add(ERTStartupOutcome::LevelMapMissing, TEXT("0 celle"));
	Report.Add(ERTStartupOutcome::FormatMapMismatch, TEXT("Standard su mappa 2v2"));

	TestEqual(TEXT("il fatale si trova"),
		URTStartupReportLibrary::FindFatal(Report), ERTStartupOutcome::FormatMapMismatch);
	TestTrue(TEXT("e la degradazione resta registrata"),
		URTStartupReportLibrary::HasDegradation(Report));

	return true;
}

/**
 * Ogni esito non-`Ok` ha un testo: e' la riga che il banner o il modale mostrano.
 *
 * ⚠️ Il testo nasce nella libreria, **non nel widget**. E' la ragione per cui questo vocabolario esiste:
 * un widget libero di comporre la frase sarebbe una seconda autorita' su *perche'* qualcosa non e'
 * partito, e potrebbe dire una cosa diversa dal log a parita' di causa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStartupEveryOutcomeHasTextTest,
	"RefactorTactics.Startup.EveryOutcomeHasReadableText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStartupEveryOutcomeHasTextTest::RunTest(const FString&)
{
	for (const ERTStartupOutcome Outcome : AllOutcomes)
	{
		const FText Text = URTStartupReportLibrary::DescribeOutcome(Outcome);

		if (Outcome == ERTStartupOutcome::Ok)
		{
			TestTrue(TEXT("Ok non ha niente da dire"), Text.IsEmpty());
			continue;
		}

		if (!TestFalse(*FString::Printf(TEXT("l'esito %d ha un testo"), (int32)Outcome), Text.IsEmpty()))
		{
			return false;
		}
	}

	return true;
}

/**
 * Le fasi di caricamento sono un **ordine**, non un insieme: `Idle` precede `Map`, che precede
 * `Scenario`, che precede `Bots`, che precede `Ready`.
 *
 * L'ordine e' quello dell'allestimento reale in `BeginPlay` — mappa, formato, unita' — ed e' cio' che
 * permette a un test di asserire che le fasi siano state emesse **nella sequenza giusta** invece che solo
 * emesse.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLoadPhaseOrderTest,
	"RefactorTactics.Startup.LoadPhasesAreOrdered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLoadPhaseOrderTest::RunTest(const FString&)
{
	TestTrue(TEXT("Idle < Map"), (uint8)ERTLoadPhase::Idle < (uint8)ERTLoadPhase::Map);
	TestTrue(TEXT("Map < Scenario"), (uint8)ERTLoadPhase::Map < (uint8)ERTLoadPhase::Scenario);
	TestTrue(TEXT("Scenario < Bots"), (uint8)ERTLoadPhase::Scenario < (uint8)ERTLoadPhase::Bots);
	TestTrue(TEXT("Bots < Ready"), (uint8)ERTLoadPhase::Bots < (uint8)ERTLoadPhase::Ready);

	FRTStartupReport Report;
	TestEqual(TEXT("un report nasce Idle"), Report.Phase, ERTLoadPhase::Idle);

	Report.Phase = ERTLoadPhase::Ready;
	Report.Reset();
	TestEqual(TEXT("e Reset lo riporta a Idle"), Report.Phase, ERTLoadPhase::Idle);
	TestEqual(TEXT("svuotando le note"), Report.Notes.Num(), 0);

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Il PRODUTTORE. I test qui sopra provano il vocabolario; questi provano che qualcuno lo riempia —
// che e' la meta' che il repository ha gia' pagato piu' volte: un dato dichiarato, trasportato e
// mai prodotto.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * **Un GameMode appena costruito non dichiara niente.**
 *
 * Il rapporto nasce vuoto e in `Idle`: se nascesse `Ready`, o con una nota, il banner comparirebbe prima
 * che l'allestimento sia successo — cioe' mentirebbe nel verso peggiore, quello che fa perdere tempo a
 * cercare una causa che non c'e'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStartupGameModeStartsSilentTest,
	"RefactorTactics.Startup.FreshGameModeReportsNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStartupGameModeStartsSilentTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("game mode"), GameMode)) { RTWorldFixtures::DestroyWorld(World); return false; }

	const FRTStartupReport& Report = GameMode->GetStartupReport();
	TestEqual(TEXT("nessuna nota"), Report.Notes.Num(), 0);
	TestEqual(TEXT("fase Idle"), Report.Phase, ERTLoadPhase::Idle);
	TestFalse(TEXT("niente banner"), URTStartupReportLibrary::HasDegradation(Report));
	TestEqual(TEXT("niente modale"),
		URTStartupReportLibrary::FindFatal(Report), ERTStartupOutcome::Ok);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **Due ripieghi nella stessa partita, entrambi dichiarati.**
 *
 * E' la proprieta' per cui le note sono una **lista**: un allestimento accumula condizioni, e mostrarne
 * una sola nasconderebbe l'altra. Qui la coppia e' *mappa assente* + *nessun TurnManager*, entrambe
 * raggiunte davvero da `SetupHexMatch` in un mondo di prova.
 *
 * 🔴 **La prima stesura di questo test usava una coppia diversa e la premessa era falsa**: credevo che
 * senza `MatchFormat` si arrivasse al **formato di ripiego**, e il test lo asseriva. Non ci si arriva —
 * `FindShippedFormat` trova `Format.Skirmish2v2`, **spedito da C++** dal commit `9f44570d`, quindi il
 * ramo `UsingFallbackFormat` richiede *anche* che non esista un formato spedito. Il test rosso l'ha
 * detto; la premessa era plausibile e sbagliata.
 *
 * ⚠️ Questo test non passa per l'HUD: prova che il **dato** esista. Che il banner lo disegni e' di
 * `PIE-V01-FRONTEND-ERROR`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStartupReportsBothG13ReservesTest,
	"RefactorTactics.Startup.BothFallbacksAreReportedTogether",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStartupReportsBothG13ReservesTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("game mode"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Mappa senza asset -> ripiego sull'arena demo. E nel mondo di prova non c'e' nessun `ARTTurnManager`,
	// quindi il formato risolve ma non ha destinatario: due condizioni distinte, entrambe reali.
	HexMap->MapAsset = nullptr;

	GameMode->SetupHexMatch(HexMap);

	const FRTStartupReport& Report = GameMode->GetStartupReport();

	TestTrue(TEXT("il banner ha qualcosa da dire"), URTStartupReportLibrary::HasDegradation(Report));
	TestEqual(TEXT("ma la partita e' partita: nessun fatale"),
		URTStartupReportLibrary::FindFatal(Report), ERTStartupOutcome::Ok);

	// Le DUE condizioni, non una. E' il punto del test.
	bool bMapFallback = false;
	bool bNoTurnManager = false;
	for (const FRTStartupNote& Note : Report.Notes)
	{
		bMapFallback |= (Note.Outcome == ERTStartupOutcome::LevelMapMissing
			|| Note.Outcome == ERTStartupOutcome::UsingTestArena
			|| Note.Outcome == ERTStartupOutcome::UsingDemoArena);
		bNoTurnManager |= (Note.Outcome == ERTStartupOutcome::NoTurnManager);
	}

	TestTrue(TEXT("il ripiego di MAPPA e' dichiarato"), bMapFallback);
	TestTrue(TEXT("e anche l'assenza del TurnManager"), bNoTurnManager);
	TestTrue(TEXT("due note distinte, non una"), Report.Notes.Num() >= 2);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **Le fasi avanzano, e `Ready` non significa «senza problemi».**
 *
 * E' la distinzione che rende il rapporto utile: un allestimento puo' arrivare in fondo **e** portare due
 * ripieghi. Se `Ready` implicasse «tutto a posto», il banner non avrebbe motivo di esistere proprio nel
 * caso per cui e' stato costruito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStartupPhaseReachesReadyWithNotesTest,
	"RefactorTactics.Startup.ReadyDoesNotMeanClean",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStartupPhaseReachesReadyWithNotesTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("game mode"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	HexMap->MapAsset = nullptr;
	GameMode->SetupHexMatch(HexMap);

	const FRTStartupReport& Report = GameMode->GetStartupReport();

	TestEqual(TEXT("l'allestimento e' arrivato in fondo"), Report.Phase, ERTLoadPhase::Ready);
	TestTrue(TEXT("**e** porta condizioni degradate"), URTStartupReportLibrary::HasDegradation(Report));

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **Un secondo allestimento non eredita le note del primo.**
 *
 * `Play Again` ripercorre `SetupHexMatch` nella stessa sessione: senza il reset, il banner mostrerebbe
 * condizioni della partita precedente — che e' un dato falso con l'aria di essere aggiornato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStartupSecondSetupResetsTest,
	"RefactorTactics.Startup.SecondSetupDoesNotInheritNotes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStartupSecondSetupResetsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("game mode"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	HexMap->MapAsset = nullptr;
	GameMode->SetupHexMatch(HexMap);
	const int32 FirstCount = GameMode->GetStartupReport().Notes.Num();
	TestTrue(TEXT("il primo allestimento ha prodotto note"), FirstCount > 0);

	GameMode->SetupHexMatch(HexMap);

	// ⚠️ **Si asserisce «non accumula», non «stesso numero»**, e la differenza l'ha trovata il test rosso:
	// la prima chiamata **crea** l'arena di ripiego e la lascia su `HexMap->MapAsset`, quindi la seconda
	// trova una mappa valida e non ripiega piu'. Il conteggio *deve* scendere — pretendere che resti
	// uguale sarebbe asserire che il mondo non cambia fra due allestimenti, che e' falso e non e' cio' che
	// questo test difende.
	TestTrue(TEXT("il secondo NON accumula sopra il primo"),
		GameMode->GetStartupReport().Notes.Num() <= FirstCount);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Le CLASSI BASE dei widget. Provano la superficie: cosa un Blueprint puo' leggere, e cosa non trova
// perche' non esiste. Il layout sta nel `.uasset` e resta a `PIE-V01-FRONTEND-ERROR`, per costruzione.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * La schermata d'attesa **si spegne da sola** su `Idle` e su `Ready`.
 *
 * `IsLoading()` e' la condizione con cui il Blueprint si mostra: se fosse vera anche a `Ready`, la
 * schermata di caricamento resterebbe sopra la partita — il dead-end piu' banale e piu' facile da
 * introdurre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLoadingWidgetHidesWhenIdleOrReadyTest,
	"RefactorTactics.Frontend.LoadingScreenIsSilentWhenThereIsNoWait",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLoadingWidgetHidesWhenIdleOrReadyTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	URTLoadingScreenWidgetBase* Widget = NewObject<URTLoadingScreenWidgetBase>(World);
	if (!TestNotNull(TEXT("widget"), Widget)) { RTWorldFixtures::DestroyWorld(World); return false; }

	TestFalse(TEXT("appena costruito non sta caricando"), Widget->IsLoading());
	TestTrue(TEXT("e non ha niente da dire"), Widget->GetPhaseText().IsEmpty());

	Widget->SetPhase(ERTLoadPhase::Map);
	TestTrue(TEXT("durante la mappa sta caricando"), Widget->IsLoading());
	TestFalse(TEXT("e ha una riga"), Widget->GetPhaseText().IsEmpty());

	Widget->SetPhase(ERTLoadPhase::Ready);
	TestFalse(TEXT("a partita pronta si spegne"), Widget->IsLoading());
	TestTrue(TEXT("e torna muto"), Widget->GetPhaseText().IsEmpty());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Ogni fase d'attesa ha una riga, e le due fasi che **non** sono attese non ce l'hanno.
 *
 * E' la stessa proprieta' di `EveryOutcomeHasReadableText` applicata alle fasi: un widget che ricevesse
 * una fase senza testo mostrerebbe una schermata vuota, che e' peggio di nessuna schermata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLoadingWidgetHasTextForEveryWaitTest,
	"RefactorTactics.Frontend.EveryWaitingPhaseHasText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLoadingWidgetHasTextForEveryWaitTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	URTLoadingScreenWidgetBase* Widget = NewObject<URTLoadingScreenWidgetBase>(World);
	if (!TestNotNull(TEXT("widget"), Widget)) { RTWorldFixtures::DestroyWorld(World); return false; }

	const ERTLoadPhase Waiting[] = { ERTLoadPhase::Map, ERTLoadPhase::Scenario, ERTLoadPhase::Bots };
	for (const ERTLoadPhase Phase : Waiting)
	{
		Widget->SetPhase(Phase);
		if (!TestFalse(*FString::Printf(TEXT("la fase %d ha un testo"), (int32)Phase),
			Widget->GetPhaseText().IsEmpty()))
		{
			RTWorldFixtures::DestroyWorld(World);
			return false;
		}
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **Il modale rifiuta di armarsi su un ripiego.**
 *
 * E' il difetto del DoD originale reso impossibile: un modale che dicesse «non si e' potuto partire»
 * sopra una partita che sta girando. Il filtro vive nella classe base, non nel Blueprint — un Blueprint
 * puo' dimenticarlo, una firma no.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTErrorModalRefusesDegradationTest,
	"RefactorTactics.Frontend.ErrorModalOnlyArmsOnFatal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTErrorModalRefusesDegradationTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	URTErrorModalWidgetBase* Modal = NewObject<URTErrorModalWidgetBase>(World);
	if (!TestNotNull(TEXT("widget"), Modal)) { RTWorldFixtures::DestroyWorld(World); return false; }

	TestFalse(TEXT("nasce disarmato"), Modal->IsArmed());

	// Un ripiego non lo arma.
	Modal->ShowFor(FRTStartupNote(ERTStartupOutcome::UsingTestArena, TEXT("120 celle")));
	TestFalse(TEXT("un ripiego NON arma il modale"), Modal->IsArmed());
	TestTrue(TEXT("e non lascia una causa"), Modal->GetReasonText().IsEmpty());

	// Un fatale si'.
	Modal->ShowFor(FRTStartupNote(ERTStartupOutcome::FormatMapMismatch, TEXT("Standard su mappa 2v2")));
	TestTrue(TEXT("un fatale lo arma"), Modal->IsArmed());
	TestEqual(TEXT("con l'esito giusto"), Modal->GetOutcome(), ERTStartupOutcome::FormatMapMismatch);
	TestFalse(TEXT("e una causa leggibile"), Modal->GetReasonText().IsEmpty());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **Il banner elenca tutte le degradazioni e nessun fatale.**
 *
 * Le due meta' della stessa regola: mostrarne una sola nasconderebbe l'altra, e mostrare un fatale
 * significherebbe disegnare un banner sopra una partita che non esiste.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBannerListsEveryDegradationTest,
	"RefactorTactics.Frontend.BannerListsEveryDegradationAndNoFatal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBannerListsEveryDegradationTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	URTFallbackBannerWidgetBase* Banner = NewObject<URTFallbackBannerWidgetBase>(World);
	if (!TestNotNull(TEXT("widget"), Banner)) { RTWorldFixtures::DestroyWorld(World); return false; }

	FRTStartupReport Report;
	Banner->SetFromReport(Report);
	TestFalse(TEXT("un report pulito non accende il banner"), Banner->HasAnything());

	Report.Add(ERTStartupOutcome::UsingTestArena, TEXT("120 celle"));
	Report.Add(ERTStartupOutcome::NoTurnManager, TEXT("Format.Skirmish2v2"));
	Report.Add(ERTStartupOutcome::FormatMapMismatch, TEXT("non deve comparire"));

	Banner->SetFromReport(Report);

	TestTrue(TEXT("il banner si accende"), Banner->HasAnything());
	TestEqual(TEXT("DUE righe: i due ripieghi, non il fatale"), Banner->GetLines().Num(), 2);

	// Il fatale non deve comparire fra le righe.
	for (const FText& Line : Banner->GetLines())
	{
		if (!TestFalse(TEXT("nessuna riga viene dal fatale"),
			Line.ToString().Contains(TEXT("non deve comparire"))))
		{
			RTWorldFixtures::DestroyWorld(World);
			return false;
		}
	}

	// Un secondo riempimento sostituisce, non accumula: e' lo stesso difetto del `Reset()` del report,
	// un gradino piu' in la'.
	Banner->SetFromReport(Report);
	TestEqual(TEXT("ancora due righe, non quattro"), Banner->GetLines().Num(), 2);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
