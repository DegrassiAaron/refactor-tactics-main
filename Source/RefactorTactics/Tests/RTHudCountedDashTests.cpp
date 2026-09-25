// Il tratteggio a conteggio fisso del tiro rifiutato: quanti tratti, dove cadono, e perché è DISPARI.
//
// Perché esiste (#2184, reperto n. 3): `ARTHUD::DrawHUD` portava `constexpr int32 Tratti = 7` e il ciclo
// che ne derivava. Una politica di disegno che **nessuna enumerazione dei rami poteva vedere** — non sta
// in un ramo — e che nessun test raggiungeva, perché `DrawHUD` non ha copertura headless.
//
// ⌫ **Era stato archiviato come «non si fa», e per la ragione sbagliata.** Il referto diceva che
// unificarlo con `ComposeDashSegments` avrebbe cambiato i pixel. È vero, ed è dimostrabile: quella
// funzione divide per un periodo in **pixel**, e chiedendole lo stesso disegno —
// `ComposeDashSegments(A, B, 0.5f, Len * 2.f / 7.f)` — dà `Steps = RoundToInt(3.5) = 4` e con
// `T1 = (s + 0.5) / 4` l'ultimo tratto finisce a **0,875** invece che sul bersaglio.
//
// ⛔ Ma il verdetto era sull'UNIFICAZIONE, non sul reperto. Estrarre il tratteggio a conteggio fisso con
// la **stessa identica aritmetica** non sposta un pixel, e lo rende interrogabile. Giudicare la via che
// non si può prendere e scriverne l'esito sul reperto è il difetto che questo file chiude.
//
// 🔑 **Il perché del DISPARI, che è la sola regola qui dentro.** Si accendono gli indici pari; con
// `Spans` dispari l'ultimo indice pari è `Spans - 1`, e il suo tratto arriva a `Spans / Spans = 1`, cioè
// esattamente sul punto d'arrivo. Con un conteggio pari il tratteggio finisce **spento**, e la linea
// sembra interrompersi prima dell'ostacolo — che è il contrario di ciò che deve comunicare.
//
// ── TABELLA DELLE ATTESE DI MUTAZIONE ────────────────────────────────────────────────────────────────
// Scritta prima di lanciare ed eseguibile: ogni riga nomina un test che esiste e una mutazione che
// compila, a partire dal codice spedito. Gli esiti sono misurati sull'insieme di test FINALE.
//
//   # | mutazione                                                | rosso atteso                  | esito
//  ---|------------------------------------------------------------|-------------------------------|------
//   1 | `I += 2` → `I += 1`                                         | …LightsFourAndEndsOnTarget…   | 2 rossi
//   2 | `static_cast<float>(I + 1) / Spans` → `(I) / Spans`         | …LightsFourAndEndsOnTarget…   | 2 rossi
//   3 | `BlockedShotDashSpans = 7` → `= 6`                          | …TheBlockedShotUsesSeven…     | 1, esatto
//   4 | `if (Spans <= 0)` → `if (Spans < 0)`                        | ⛔ **SOPRAVVIVE**, e va detto  | sopravvissuta
//   5 | `Segmenti.Reserve((Spans + 1) / 2)` → `Reserve(0)`          | ⛔ **SOPRAVVIVE**, e va detto  | sopravvissuta
//
// ⌫ **La 2 diceva «1, esatto» e adesso ne fa 2**, perché il caso pari è passato da una soglia a valori
// esatti dopo la revisione. Non è un esito cambiato da solo: è la conferma che una tabella di mutazioni
// è una misura sull'INSIEME dei test, e che cambiare un test obbliga a rigirarla tutta. Rigirate tutte
// e cinque, non solo la riga nuova.
//
// ⛔ **La 4 è dichiarata come sopravvivente, non omessa.** Con `Spans == 0` il ciclo non parte comunque
// (`0 < 0` è falso), quindi la distinzione `<= 0` contro `< 0` **non è osservabile dall'esito**: la
// guardia esiste per `Reserve((Spans + 1) / 2)`, che con un valore negativo chiederebbe un numero
// negativo. Una riga di tabella che promettesse un rosso qui sarebbe una promessa vuota — il difetto
// che gli AC di questa issue vietano dopo la fetta 5.
//
// ⛔ **E la 5 è la stessa forma, dichiarata dopo la revisione della PR #3355.** `TArray::Reserve` è un
// suggerimento di capacità: non tocca `Num()` né i valori, e nessun test della suite legge `Max()`. Vale
// la pena scriverla perché il commento accanto a quella riga parla di `Reserve`, e un lettore potrebbe
// dedurne che sia coperta. Non lo è, e l'unico caso in cui conta davvero — un argomento negativo, che
// in UE 5.8 non è un no-op ma un `Fatal` — lo chiude la guardia della riga 4, non questa costante.
//
// 🔑 **Ed è sopravvissuta davvero, il che è il punto.** Una guardia dichiarata non osservabile che
// morisse sotto la propria mutazione direbbe che l'analisi era sbagliata, non il codice. Una riga di
// tabella che prevede un VERDE vale quanto una che prevede un rosso: entrambe sono falsificabili.
//
// ⚠️ **La 2 è stata rigirata.** La prima corsa ha prodotto un log che non dichiarava `TEST COMPLETE`, e
// un conteggio letto su un log in corso non distingue «zero rossi» da «non ancora arrivato al rosso».
// Lo script di mutazione rifiuta quel caso invece di riportarne il numero, ed è la ragione per cui qui
// c'è un esito e non un verde preso per buono.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Nomi lunghi apposta: a livello di file, sotto unity build, diventano globali che nascondono le
	// locali omonime degli altri file del blocco — e `C4459` qui è un errore.
	const FVector2D PartenzaDiProva(0.f, 0.f);
	const FVector2D ArrivoDiProva(70.f, 0.f);
}

/**
 * 🔑 **Sette parti accendono quattro tratti, e l'ultimo finisce ESATTAMENTE sul bersaglio.**
 *
 * Le coordinate sono asserite esatte, non approssimate: lo Scope della issue vieta di spostare i pixel,
 * e questa funzione è nata da un blocco che nessun test misurava.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudCountedDashOddTest,
	"RefactorTactics.HUD.CountedDashLightsFourAndEndsOnTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudCountedDashOddTest::RunTest(const FString&)
{
	const TArray<TPair<FVector2D, FVector2D>> Tratti =
		ARTHUD::ComposeCountedDashSegments(PartenzaDiProva, ArrivoDiProva, 7);

	if (!TestEqual(TEXT("sette parti, quattro tratti accesi"), Tratti.Num(), 4)) { return false; }

	// 70 pixel in sette parti: ogni parte è 10. Accese le pari.
	TestEqual(TEXT("il primo tratto parte dall'origine"), Tratti[0].Key.X, 0.0);
	TestEqual(TEXT("e arriva a un settimo"), Tratti[0].Value.X, 10.0);
	TestEqual(TEXT("il secondo salta un settimo"), Tratti[1].Key.X, 20.0);
	TestEqual(TEXT("il terzo"), Tratti[2].Key.X, 40.0);

	// ⛔ L'asserzione che porta la regola: DISPARI significa che si finisce ACCESI, sul bersaglio.
	TestEqual(TEXT("⛔ e l'ultimo tratto finisce ESATTAMENTE sul punto d'arrivo"),
		Tratti[3].Value.X, 70.0);

	return true;
}

/**
 * ⛔ **Un conteggio PARI finisce spento, e la linea sembra fermarsi prima dell'ostacolo.**
 *
 * È il caso che dà senso alla parola «dispari» nel nome della costante: senza, quella parola sarebbe
 * un'affermazione non verificata accanto a un numero.
 *
 * ⌫ Qui c'era una soglia (`< Arrivo - 1`) invece di un valore. Reggeva solo perché il caso dispari pinna
 * la stessa formula a valori esatti — cioè per ridondanza, non per forza propria. Trovato dalla revisione
 * della PR #3355.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudCountedDashEvenTest,
	"RefactorTactics.HUD.CountedDashWithAnEvenCountStopsShort",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudCountedDashEvenTest::RunTest(const FString&)
{
	// ⚠️ **Arrivo a 60 e non a 70, e non e' una comodita'**: sei parti di 70 cadono su 11,666…, e
	// l'asserzione dovrebbe diventare una soglia. Una soglia lasca nasconde di non aver fatto il conto.
	// Con 60 le sei parti valgono 10 esatti e l'ultimo tratto acceso finisce a 50, dieci pixel prima.
	const FVector2D Arrivo(60.f, 0.f);
	const TArray<TPair<FVector2D, FVector2D>> Tratti =
		ARTHUD::ComposeCountedDashSegments(PartenzaDiProva, Arrivo, 6);

	if (!TestEqual(TEXT("sei parti, tre tratti accesi"), Tratti.Num(), 3)) { return false; }

	TestEqual(TEXT("l'ultimo tratto acceso parte dai due terzi"), Tratti[2].Key.X, 40.0);

	// ⛔ L'asserzione che porta la regola, ed e' ESATTA: con un conteggio pari si finisce SPENTI.
	TestEqual(TEXT("⛔ e finisce a 50, non sul punto d'arrivo"), Tratti[2].Value.X, 50.0);
	TestTrue(TEXT("⛔ cioe' la linea sembra fermarsi prima dell'ostacolo"),
		Tratti[2].Value.X < Arrivo.X);

	return true;
}

/**
 * Il tiro rifiutato usa sette parti, e il numero si pinna.
 *
 * ⚠️ Il valore è scritto a mano e non letto dalla costante: un test che rileggesse `BlockedShotDashSpans`
 * sarebbe soddisfatto da qualunque valore, e non direbbe niente sui pixel che lo Scope protegge.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudBlockedShotSpansTest,
	"RefactorTactics.HUD.TheBlockedShotUsesSevenSpans",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudBlockedShotSpansTest::RunTest(const FString&)
{
	TestEqual(TEXT("⛔ sette, e dispari: il tratteggio finisce sul bersaglio"),
		ARTHUD::BlockedShotDashSpans, 7);
	TestTrue(TEXT("🔑 ed e' la proprieta' che conta: DISPARI"),
		ARTHUD::BlockedShotDashSpans % 2 == 1);

	return true;
}

/**
 * Un conteggio non positivo rende un elenco vuoto, senza cadere.
 *
 * ⚠️ **Il ciclo non c'entra**: con `Spans == 0` non partirebbe comunque. Ciò che la guardia protegge è
 * `Reserve((Spans + 1) / 2)`, che con un valore negativo chiederebbe un numero negativo — ed è per
 * questo che il caso interrogato qui è `-3` e non solo `0`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudCountedDashNonPositiveTest,
	"RefactorTactics.HUD.CountedDashWithANonPositiveCountIsEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudCountedDashNonPositiveTest::RunTest(const FString&)
{
	TestEqual(TEXT("zero parti, nessun tratto"),
		ARTHUD::ComposeCountedDashSegments(PartenzaDiProva, ArrivoDiProva, 0).Num(), 0);
	TestEqual(TEXT("⛔ e un conteggio negativo non chiede una riserva negativa"),
		ARTHUD::ComposeCountedDashSegments(PartenzaDiProva, ArrivoDiProva, -3).Num(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
