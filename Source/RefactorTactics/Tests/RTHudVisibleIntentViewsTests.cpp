// Da quale prospettiva si chiedono le viste d'intento — cioè QUALI piani compaiono a schermo.
//
// Perché esiste (#2184): la decisione viveva dentro `ARTHUD::DrawHUD`, che il motore chiama ogni
// fotogramma e che **non ha copertura headless**. È una pura funzione di tre cose — i piani autorevoli, la
// squadra dell'osservatore, e il dato di sessione — e nessuna tocca `Project()`, `Canvas` o una coordinata.
//
// 🔴 **Ed è una decisione di PRIVACY, non di grafica.** Invertendo i rami, in partita presidiata il
// giocatore riceverebbe ogni intento filtrato dalla prospettiva di chi lo possiede: vedrebbe i piani
// completi dei nemici NON rivelati, waypoint e reazioni compresi. Non è un difetto visivo — è
// l'invariante #6 che cade, e cade **sul client**, dove il dato arriverebbe davvero.
//
// ⛔ **La reticenza che nessuno misurava sono i DOPPIONI.** `URTIntentPrivacyLibrary::FilterForTeam`
// concede all'osservatore gli alleati **e** gli avversari `bRevealed`. Nel ramo non presidiato la domanda
// è quindi **una per unità**, dalla prospettiva di chi la possiede: due domande di squadra sull'insieme
// intero farebbero tornare due volte ogni unità rivelata — una come alleata della propria squadra, una
// come nemica rivelata dell'altra. È la forma di difetto che non si vede in una schermata, perché due
// righe identiche sovrapposte sembrano una.
//
// ⚠️ **`FilterForTeam` non è sotto test qui.** Quella decide *cosa* un osservatore ha diritto di sapere e
// ha i suoi test in `RTIntentPrivacyTests`. Qui si verifica *chi chiede*, che è l'altra metà.
//
// ⚠️ **`TestEqualSensitive` dove si pinna testo**: su stringhe `TestEqual` passa da `FCString::Stricmp`
// (`AutomationTest.cpp:2163`) ed è case-insensitive — dieci asserzioni della fetta 1 di #2184 non
// fissavano le maiuscole, e l'ha trovato la code review, non la suite.
//
// ── TABELLA DELLE ATTESE DI MUTAZIONE, E CIÒ CHE HANNO DAVVERO PRODOTTO ──────────────────────────────
// Scritta **prima** di lanciare, come le fette 2 e 3 di #2184 hanno stabilito: mutare la regola più ovvia
// misura la regola più ovvia, e nella fetta 1 quattro reticenze sopravvissero perché ne fu mutata una sola.
//
// ⌫ **La prima stesura di questa tabella era imprecisa in tre punti, e la code review li ha misurati.**
// Nominava test che non esistono (etichette descrittive invece dei nomi registrati), descriveva la
// mutazione 1 a partire da uno stato che non è quello spedito, e la 3 come `PlayerTeamId → Intent.TeamId`,
// che **non compila**: nel ramo presidiato `Intent` non è dichiarato. Una tabella che non si può rieseguire
// non è evidenza — è una promessa. Qui sotto c'è ciò che è stato eseguito davvero.
//
//   # | mutazione su `ARTHUD::ComposeVisibleIntentViews`            | rosso atteso              | esito
//  ---|-------------------------------------------------------------|---------------------------|------
//   1 | togliere il `!` da `if (!bUnattendedSession)`                | HideUnrevealedEnemy…      | 4/4 ⚠️
//     |   → i due rami si scambiano                                  |  + ShowBothTeams…         |
//   2 | sostituire il ciclo per unità con due domande di squadra     | ShowBothTeams…            | 3 rossi
//     |   `FilterForTeam(0, Authoritative)` + `FilterForTeam(1, …)`  |  (incluso l'atteso)       |
//   3 | ramo presidiato: `PlayerTeamId` → letterale `1`              | HideUnrevealedEnemy…      | 1, esatto
//   4 | `Views.Insert(…, 0)` invece di `Views.Append(…)`             | KeepInputOrder            | 1, esatto
//   5 | ramo presidiato: `PlayerTeamId` → letterale `0`              | FollowTheObserverTeamNot… | 1, esatto
//
// 🔑 **La 5 è quella che la code review ha reso necessaria, e misura il buco che aveva trovato**: prima
// del test `FollowTheObserverTeamNotZero`, cablare `0` al posto del parametro non faceva cadere NIENTE —
// tutti i test passavano `PlayerTeamId = 0`. Una mutazione che sopravvive è una regola che nessuno tiene.
//
// I nomi per esteso sono `RefactorTactics.HUD.VisibleIntentViews…` — quelli passati a
// `IMPLEMENT_SIMPLE_AUTOMATION_TEST` qui sotto, non parafrasi.
//
// ⚠️ La mutazione 1 ha prodotto **più** rossi dell'atteso (tutti e quattro): l'attesa era un minimo, non
// un'uguaglianza, e scambiare i due rami rompe ogni proprietà del file. Registrato perché una sorpresa in
// eccesso va scritta quanto una in difetto.
//
// ⛔ Ogni build di mutazione ha dato `Result: Succeeded` prima della run. Senza quel controllo la suite
// girerebbe sul binario precedente e il rosso — o il verde — direbbe il falso.
//
// Le quattro sono reticenze o precedenze, cioè la categoria che entrambe le fette precedenti hanno
// mostrato essere la più fragile. Nessuna è un conteggio: sono tutte proprietà.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "Turn/RTIntentPrivacyLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ **L'unita' si identifica con `OwnerCell`, perche' un `UnitId` in queste struct NON ESISTE** —
	// verificato nei campi di `FRTPlannedIntent` e `FRTIntentView`, non assunto. La cella e' univoca per
	// unita' (un solo occupante autorevole per `FRTCellId`), quindi regge come identificatore nel test.

	/** Un piano autorevole minimo: quel che basta a `FilterForTeam` per decidere. */
	FRTPlannedIntent PianoDi(int32 CellaX, int32 TeamId, bool bRevealed, const TCHAR* Azione)
	{
		FRTPlannedIntent Intent;
		Intent.OwnerCell = FRTCellId(CellaX, 0, 0);
		Intent.TeamId = TeamId;
		Intent.bRevealed = bRevealed;
		Intent.bAlive = true;
		Intent.ActionName = FText::FromString(Azione);
		return Intent;
	}

	/** Le `OwnerCell.X` delle viste, nell'ordine in cui tornano: la forma in cui si leggono le attese. */
	TArray<int32> IdDi(const TArray<FRTIntentView>& Views)
	{
		TArray<int32> Out;
		Out.Reserve(Views.Num());
		for (const FRTIntentView& V : Views) { Out.Add(V.OwnerCell.X); }
		return Out;
	}
}

/**
 * 🔴 **In partita presidiata il nemico non rivelato non arriva affatto.**
 *
 * Non «arriva e non si disegna»: non compare fra le viste. La differenza è l'invariante #6 — un dato
 * presente sul client e nascosto a schermo è leggibile con qualunque strumento, ed è insostenibile
 * quando arriverà la rete (`M10`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVisibleIntentsAttendedTest,
	"RefactorTactics.HUD.VisibleIntentViewsHideUnrevealedEnemyWhenAttended",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVisibleIntentsAttendedTest::RunTest(const FString&)
{
	TArray<FRTPlannedIntent> Autorevoli;
	Autorevoli.Add(PianoDi(/*CellaX=*/ 1, /*TeamId=*/ 0, /*bRevealed=*/ false, TEXT("Avanza")));
	Autorevoli.Add(PianoDi(/*CellaX=*/ 2, /*TeamId=*/ 1, /*bRevealed=*/ false, TEXT("Attacca")));
	Autorevoli.Add(PianoDi(/*CellaX=*/ 3, /*TeamId=*/ 1, /*bRevealed=*/ true,  TEXT("Scatta")));

	const TArray<FRTIntentView> Viste =
		ARTHUD::ComposeVisibleIntentViews(Autorevoli, /*PlayerTeamId=*/ 0, /*bUnattendedSession=*/ false);

	const TArray<int32> Id = IdDi(Viste);

	// L'alleata c'è, il nemico rivelato c'è, il nemico NON rivelato manca. È la regola di `FilterForTeam`,
	// e qui si verifica che la prospettiva da cui la si interroga sia quella di chi gioca.
	TestTrue(TEXT("la propria unita' c'e'"), Id.Contains(1));
	TestTrue(TEXT("il nemico RIVELATO c'e'"), Id.Contains(3));
	TestFalse(TEXT("⛔ il nemico NON rivelato non compare fra le viste"), Id.Contains(2));

	// ⛔ **Asserzione di controllo, e misura gli ECCESSI**: le tre righe sopra vedono le assenze, non le
	// presenze di troppo — un doppione di `1` o di `3`, o una quarta voce comparsa dal nulla, le
	// passerebbe tutte e tre. È il conteggio che le chiude.
	//
	// ⌫ La prima stesura diceva «un ritorno sempre vuoto passerebbe per due terzi»: è aritmeticamente
	// falso — con `Viste` vuoto le due `TestTrue` cadono e passa solo la `TestFalse`, cioè una su tre, e il
	// test è già rosso senza questa riga. Il caso da cui difende è l'opposto.
	TestEqual(TEXT("e le viste concesse sono esattamente due"), Viste.Num(), 2);

	return true;
}

/**
 * 🔴 **In sessione non presidiata si vedono entrambe le squadre — e ogni unità UNA VOLTA SOLA.**
 *
 * Nessuno gioca, quindi chi guarda è autorizzato a vedere tutto (`#2386`). Ma l'autorizzazione non
 * ammorbidisce il filtro: si fanno più domande a cui il filtro risponde di sì.
 *
 * ⛔ Il doppione è la parte che nessuna schermata rivelerebbe: due righe identiche sovrapposte sembrano
 * una. Qui l'unità `3` è **rivelata**, quindi due domande di squadra la restituirebbero due volte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVisibleIntentsUnattendedTest,
	"RefactorTactics.HUD.VisibleIntentViewsShowBothTeamsWithoutDuplicatesWhenUnattended",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVisibleIntentsUnattendedTest::RunTest(const FString&)
{
	TArray<FRTPlannedIntent> Autorevoli;
	Autorevoli.Add(PianoDi(/*CellaX=*/ 1, /*TeamId=*/ 0, /*bRevealed=*/ false, TEXT("Avanza")));
	Autorevoli.Add(PianoDi(/*CellaX=*/ 2, /*TeamId=*/ 1, /*bRevealed=*/ false, TEXT("Attacca")));
	Autorevoli.Add(PianoDi(/*CellaX=*/ 3, /*TeamId=*/ 1, /*bRevealed=*/ true,  TEXT("Scatta")));

	const TArray<FRTIntentView> Viste =
		ARTHUD::ComposeVisibleIntentViews(Autorevoli, /*PlayerTeamId=*/ 0, /*bUnattendedSession=*/ true);

	const TArray<int32> Id = IdDi(Viste);

	// Entrambe le squadre, compreso il nemico che in partita presidiata sarebbe stato taciuto.
	TestTrue(TEXT("la squadra 0 c'e'"), Id.Contains(1));
	TestTrue(TEXT("la squadra 1 c'e' anche NON rivelata"), Id.Contains(2));
	TestTrue(TEXT("e il rivelato pure"), Id.Contains(3));

	// ⛔ **La reticenza**: una vista per unità, non una per (unità × squadra che ha diritto di vederla).
	TestEqual(TEXT("⛔ una vista per unita', nessun doppione"), Viste.Num(), 3);

	int32 Occorrenze3 = 0;
	for (const int32 X : Id) { if (X == 3) { ++Occorrenze3; } }
	TestEqual(TEXT("⛔ l'unita' RIVELATA compare una volta sola, non due"), Occorrenze3, 1);

	return true;
}

/**
 * 🔴 **La prospettiva è il PARAMETRO, non il letterale `0`.**
 *
 * ⌫ Aggiunto dopo la code review, che ha misurato il buco: i primi test passavano tutti
 * `PlayerTeamId = 0`, e nel ramo non presidiato il parametro è ignorato per costruzione. Un'implementazione
 * che avesse scritto `FilterForTeam(0, Authoritative)` — ignorando l'argomento — li avrebbe passati tutti.
 * La mutazione che sostituiva `PlayerTeamId` con `1` cadeva, quindi il parametro era usato; ma la **suite**
 * non lo diceva, e una regola che regge solo sotto mutazione non è pinnata.
 *
 * Qui l'osservatore è la squadra `1`: la simmetria si rovescia, e ciò che prima era nascosto è visibile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVisibleIntentsOtherTeamTest,
	"RefactorTactics.HUD.VisibleIntentViewsFollowTheObserverTeamNotZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVisibleIntentsOtherTeamTest::RunTest(const FString&)
{
	TArray<FRTPlannedIntent> Autorevoli;
	Autorevoli.Add(PianoDi(/*CellaX=*/ 1, /*TeamId=*/ 0, /*bRevealed=*/ false, TEXT("Avanza")));
	Autorevoli.Add(PianoDi(/*CellaX=*/ 2, /*TeamId=*/ 1, /*bRevealed=*/ false, TEXT("Attacca")));

	// Osservatore = squadra 1. L'unità 2 è la SUA, l'unità 1 è il nemico non rivelato.
	const TArray<int32> Id = IdDi(
		ARTHUD::ComposeVisibleIntentViews(Autorevoli, /*PlayerTeamId=*/ 1, /*bUnattendedSession=*/ false));

	TestTrue(TEXT("la squadra 1 vede la PROPRIA unita'"), Id.Contains(2));
	TestFalse(TEXT("⛔ e NON vede il nemico non rivelato della squadra 0"), Id.Contains(1));

	// ⛔ È l'asserzione che distingue il parametro dal letterale: con `FilterForTeam(0, …)` cablato,
	// l'esito sarebbe esattamente rovesciato.
	TestEqual(TEXT("una sola vista concessa"), Id.Num(), 1);

	return true;
}

/**
 * L'ordine d'ingresso si conserva, ed è la proprietà su cui si appoggia chi disegna.
 *
 * ⚠️ È anche l'unica cosa che distingue «accodare» da «raccogliere»: un'implementazione che ordinasse per
 * squadra passerebbe ogni altra asserzione di questo file.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVisibleIntentsOrderTest,
	"RefactorTactics.HUD.VisibleIntentViewsKeepInputOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVisibleIntentsOrderTest::RunTest(const FString&)
{
	// Squadre alternate, così un raggruppamento per squadra darebbe un ordine diverso da quello d'ingresso.
	TArray<FRTPlannedIntent> Autorevoli;
	Autorevoli.Add(PianoDi(/*CellaX=*/ 10, /*TeamId=*/ 0, /*bRevealed=*/ false, TEXT("A")));
	Autorevoli.Add(PianoDi(/*CellaX=*/ 20, /*TeamId=*/ 1, /*bRevealed=*/ false, TEXT("B")));
	Autorevoli.Add(PianoDi(/*CellaX=*/ 30, /*TeamId=*/ 0, /*bRevealed=*/ false, TEXT("C")));
	Autorevoli.Add(PianoDi(/*CellaX=*/ 40, /*TeamId=*/ 1, /*bRevealed=*/ false, TEXT("D")));

	const TArray<int32> Id = IdDi(
		ARTHUD::ComposeVisibleIntentViews(Autorevoli, /*PlayerTeamId=*/ 0, /*bUnattendedSession=*/ true));

	TestEqual(TEXT("quattro viste"), Id.Num(), 4);
	if (Id.Num() == 4)
	{
		TestEqual(TEXT("l'ordine e' quello d'ingresso, non quello per squadra"), Id[0], 10);
		TestEqual(TEXT("secondo"), Id[1], 20);
		TestEqual(TEXT("terzo"),   Id[2], 30);
		TestEqual(TEXT("quarto"),  Id[3], 40);
	}

	return true;
}

/**
 * La vista che torna è quella ALLEATA, non quella da avversario — e porta quindi la forma piena.
 *
 * 🔑 È la ragione per cui la domanda si fa dalla prospettiva di chi POSSIEDE l'unità: uno spettatore
 * autorizzato deve vedere il piano come lo vede chi lo ha fatto, non la sua versione ridotta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVisibleIntentsAllyShapeTest,
	"RefactorTactics.HUD.VisibleIntentViewsUseOwnerPerspectiveWhenUnattended",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVisibleIntentsAllyShapeTest::RunTest(const FString&)
{
	TArray<FRTPlannedIntent> Autorevoli;
	Autorevoli.Add(PianoDi(/*CellaX=*/ 7, /*TeamId=*/ 1, /*bRevealed=*/ true, TEXT("Contrattacco")));

	const TArray<FRTIntentView> Viste =
		ARTHUD::ComposeVisibleIntentViews(Autorevoli, /*PlayerTeamId=*/ 0, /*bUnattendedSession=*/ true);

	if (!TestEqual(TEXT("una vista"), Viste.Num(), 1)) { return false; }

	// Chiesta dalla prospettiva della squadra 1, che la possiede: per lei è un'alleata.
	TestTrue(TEXT("🔑 e' la vista ALLEATA, cioe' la forma piena"), Viste[0].bIsAlly);
	TestEqualSensitive(TEXT("e porta il nome dell'azione intatto"),
		*Viste[0].ActionName.ToString(), TEXT("Contrattacco"));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
