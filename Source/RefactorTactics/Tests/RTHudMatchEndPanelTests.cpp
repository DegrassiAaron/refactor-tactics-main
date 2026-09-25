// Dove cadono le due righe del pannello di fine partita, e le tre coppie che dovevano coincidere.
//
// Perché esiste (#2184): il blocco finale di `ARTHUD::DrawHUD` portava sette letterali, e **tre di loro
// comparivano due volte ciascuna**:
//
//     0.4f    l'ancora verticale dell'intestazione  E  quella da cui l'istruzione scende
//     0.5f    la mezzeria su cui si centra l'intestazione  E  quella dell'istruzione
//     2.f     la scala con cui l'intestazione è MISURATA  E  quella con cui è DISEGNATA  (e 1.2f per l'istruzione)
//
// ⛔ **Una divergenza sarebbe muta e visibile solo a partita finita.** Con due ancore diverse le due
// righe si separano di mezzo schermo; con due mezzerie diverse smettono di condividere l'asse e il
// pannello si legge storto. Nessun test le raggiungeva: `RTHudEndAndSlotTests` copre la funzione pura
// che compone il TESTO, mai la posa.
//
// 🔑 **L'ancora adesso si legge una volta sola**, e l'istruzione sta sotto l'intestazione **per
// costruzione**. È la differenza fra «due numeri che finora coincidono» e «un numero solo».
//
// ⚠️ **Le due SCALE restano protette dal nome e non da un test**, e va detto invece di lasciarlo
// dedurre: le loro occorrenze sono `GetTextSize` e `DrawText`, membri di `AHUD` che vogliono canvas e
// font. Nessuna domanda headless le interroga. È lo stesso argomento già scritto per `IntentLabelScale`,
// e la riga 6 della tabella lo rende una previsione falsificabile invece che una nota.
//
// ⛔ **Il pannello NON è vincolato al viewport, e questa fetta non ce lo mette.** Con un'intestazione più
// larga del canvas la X diventa negativa e il testo esce dal bordo — come oggi. Aggiungere
// `ClampOverlayAnchor` mentre si estrae sarebbe cambiare ciò che si vede, che l'Out of scope vieta. Il
// caso è pinnato **com'è**, così il prossimo lo trova documentato invece di scoprirlo.
//
// ── TABELLA DELLE ATTESE DI MUTAZIONE ────────────────────────────────────────────────────────────────
// Scritta prima di lanciare ed eseguibile; esiti misurati sull'insieme di test FINALE.
//
//   # | mutazione                                                     | rosso atteso                | esito
//  ---|-----------------------------------------------------------------|-----------------------------|------
//   1 | `AncoraY + …` → `Viewport.Y * 0.45f + …` (DIVERGENZA)            | …StacksUnderTheHeadline…    | 1, esatto
//   2 | `MatchEndAnchorY = 0.4f` → `0.45f` (CONCORDE)                    | …PinsTheAnchor… · Stacks VERDE | 1, esatto
//   3 | `MatchEndLineGapPx = 8.f` → `10.f`                               | …StacksUnderTheHeadline…    | 1, esatto
//   4 | `(Viewport.X - RestartWidth) * MatchEndCentre` → `* 0.45f`       | …ShareTheSameAxis…          | 1, esatto
//   5 | `MatchEndRestartPrompt` → `TEXT("premi R per ricominciare")`     | …NamesTheKeyItPromises…     | 1, esatto
//   6 | `MatchEndHeadlineScale = 2.f` → `1.8f`                           | ⛔ **SOPRAVVIVE**, va detto  | sopravvissuta
//   7 | `MatchEndRestartScale = 1.2f` → `1.0f`                           | ⛔ **SOPRAVVIVE**, va detto  | sopravvissuta
//
// ⛔ **Le righe 6 e 7 sono le due che valgono.** Sopravvivono come dichiarato: le due occorrenze di
// OGNI scala sono `GetTextSize` e `DrawText`, e non esiste una domanda headless che le interroghi. Una
// tabella che le avesse omesse avrebbe lasciato credere alla copertura; una che avesse promesso un rosso
// sarebbe stata falsificata dalla prima corsa.
//
// ⌫ **La 7 non c'era, e il buco era della stessa forma del quinto letterale della fetta 9.** Il commento
// in testa a questo file nomina le due scale come **una** coppia — «`2.f` … e `1.2f` per l'istruzione» —
// e la tabella ne dichiarava sopravvivente una sola. Chi leggeva poteva dedurne che l'altra fosse
// coperta. Non lo era: mutare `MatchEndRestartScale` cambia la dimensione del testo a schermo e nessuno
// dei cinque test lo vede. Trovato dalla revisione della PR #3358, non da chi ha scritto la tabella.
//
// 🔑 **La 1 e la 2 sono la coppia che dà senso al file.** La 1 rompe la coincidenza fra le due letture
// dell'ancora e uccide l'impilamento; la 2 sposta l'ancora restando concorde e lascia l'impilamento
// **verde**, perché quel test chiede una differenza e non una posizione. Se la 2 lo facesse cadere,
// starebbe misurando il numero invece dell'invariante.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Nomi lunghi apposta: a livello di file, sotto unity build, diventano globali che nascondono le
	// locali omonime degli altri file del blocco, e `C4459` qui è un errore.
	//
	// 🔑 I numeri sono scelti per NON essere esatti in binario: `1080 * 0.4` e `37.3` non lo sono, quindi
	// una catena calcolata in doppia precisione e arrotondata alla fine non darebbe lo stesso `float`.
	// Con valori tondi il test non distinguerebbe le due implementazioni.
	constexpr float AltezzaSchermoDiProva = 1080.f;
	constexpr float LarghezzaSchermoDiProva = 1920.f;
	constexpr float AltezzaIntestazioneDiProva = 37.3f;
	constexpr float LarghezzaIntestazioneDiProva = 401.7f;
	constexpr float LarghezzaIstruzioneDiProva = 233.1f;

	ARTHUD::FRTMatchEndPanelPlacement PosaDiProva()
	{
		return ARTHUD::ComposeMatchEndPanelPlacement(
			FVector2f(LarghezzaSchermoDiProva, AltezzaSchermoDiProva),
			FVector2f(LarghezzaIntestazioneDiProva, AltezzaIntestazioneDiProva),
			LarghezzaIstruzioneDiProva);
	}
}

/**
 * 🔑 **L'istruzione sta sotto l'intestazione, e il test NON dipende da dove sia l'ancora.**
 *
 * Chiede una differenza, non una posizione: è l'unica forma in cui la coincidenza fra le due letture
 * dell'ancora è osservabile da fuori. Cade se divergono di un pixel, e non cade se si spostano insieme.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudMatchEndStackTest,
	"RefactorTactics.HUD.MatchEndPanelStacksUnderTheHeadline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudMatchEndStackTest::RunTest(const FString&)
{
	const ARTHUD::FRTMatchEndPanelPlacement Posa = PosaDiProva();

	// ⚠️ Stessa associatività del codice — `((ancora + altezza) + scarto)` — perché in `float` l'ordine
	// degli arrotondamenti si vede, e una catena riassociata darebbe un altro numero.
	// Lo scarto `8.f` è scritto a mano: rileggere la costante renderebbe il test vacuo.
	TestEqual(TEXT("🔑 l'istruzione scende dall'ancora dell'intestazione, qualunque essa sia"),
		Posa.Restart.Y, Posa.Headline.Y + AltezzaIntestazioneDiProva + 8.f, 0.f);

	return true;
}

/**
 * L'ancora è a quattro decimi dell'altezza, al pixel.
 *
 * ⚠️ Il `0.4f` è scritto a mano e non letto da `MatchEndAnchorY`: un test che rileggesse la costante
 * sarebbe soddisfatto da qualunque valore, e lo Scope della issue vieta di spostare i pixel.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudMatchEndAnchorTest,
	"RefactorTactics.HUD.MatchEndPanelPinsTheAnchorAtFourTenths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudMatchEndAnchorTest::RunTest(const FString&)
{
	const ARTHUD::FRTMatchEndPanelPlacement Posa = PosaDiProva();

	TestEqual(TEXT("⛔ quattro decimi esatti, nella stessa catena in virgola mobile"),
		Posa.Headline.Y, AltezzaSchermoDiProva * 0.4f, 0.f);

	return true;
}

/**
 * ⛔ **Le due righe condividono l'asse**, ciascuna centrata sulla PROPRIA larghezza.
 *
 * Sono due mezzerie che devono restare la stessa: se una divergesse, il pannello si leggerebbe storto e
 * nessun errore lo direbbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudMatchEndAxisTest,
	"RefactorTactics.HUD.MatchEndPanelLinesShareTheSameAxis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudMatchEndAxisTest::RunTest(const FString&)
{
	const ARTHUD::FRTMatchEndPanelPlacement Posa = PosaDiProva();

	TestEqual(TEXT("l'intestazione e' centrata sulla propria larghezza"),
		Posa.Headline.X, (LarghezzaSchermoDiProva - LarghezzaIntestazioneDiProva) * 0.5f, 0.f);
	TestEqual(TEXT("⛔ e l'istruzione sulla SUA, con la stessa mezzeria"),
		Posa.Restart.X, (LarghezzaSchermoDiProva - LarghezzaIstruzioneDiProva) * 0.5f, 0.f);

	// 🔑 La proprieta' che le due asserzioni insieme esprimono: i due centri coincidono.
	TestEqual(TEXT("🔑 i due centri sono lo STESSO punto"),
		Posa.Headline.X + LarghezzaIntestazioneDiProva * 0.5f,
		Posa.Restart.X + LarghezzaIstruzioneDiProva * 0.5f, 0.001f);

	return true;
}

/**
 * ⛔ **Il pannello NON si vincola al viewport, ed è pinnato com'è.**
 *
 * Con un'intestazione più larga del canvas la X esce negativa e il testo sborda — a differenza
 * dell'etichetta d'intento, che passa da `ClampOverlayAnchor` (#729). Questo test non dice che sia
 * giusto: dice che è **così oggi**, perché la fetta che lo ha estratto non poteva cambiarlo senza
 * spostare i pixel. Chi deciderà di vincolarlo troverà qui il caso invece di scoprirlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudMatchEndNoClampTest,
	"RefactorTactics.HUD.MatchEndPanelDoesNotClampToTheViewport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudMatchEndNoClampTest::RunTest(const FString&)
{
	const ARTHUD::FRTMatchEndPanelPlacement Posa = ARTHUD::ComposeMatchEndPanelPlacement(
		FVector2f(200.f, 400.f), FVector2f(500.f, 30.f), 100.f);

	TestEqual(TEXT("⛔ intestazione piu' larga del canvas: la X esce NEGATIVA"),
		Posa.Headline.X, -150.f, 0.f);
	TestTrue(TEXT("e non viene riportata dentro"), Posa.Headline.X < 0.f);

	return true;
}

/**
 * L'istruzione nomina il tasto che promette.
 *
 * ⚠️ **La stringa è scritta per esteso e non riletta dalla costante**, o il test sarebbe vacuo. E
 * l'accoppiamento vero resta aperto: il tasto lo lega `ARTPlayerController` con `EKeys::R`, e se quella
 * legatura cambiasse questo testo mentirebbe senza che niente lo segnali. Il nome non chiude
 * l'accoppiamento — lo rende visibile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudMatchEndPromptTest,
	"RefactorTactics.HUD.MatchEndPanelNamesTheKeyItPromises",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudMatchEndPromptTest::RunTest(const FString&)
{
	TestEqual(TEXT("⛔ il testo dell'istruzione, parola per parola"),
		FString(ARTHUD::MatchEndRestartPrompt), FString(TEXT("premi R per rigiocare")));
	TestTrue(TEXT("🔑 e nomina il tasto R, che ARTPlayerController lega altrove"),
		FString(ARTHUD::MatchEndRestartPrompt).Contains(TEXT(" R ")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
