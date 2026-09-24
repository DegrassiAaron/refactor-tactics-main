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
// ── TABELLA DELLE ATTESE DI MUTAZIONE ────────────────────────────────────────────────────────────────
// Scritta **prima** di lanciare, come le fette 2 e 3 di #2184 hanno stabilito: mutare la regola più ovvia
// misura la regola più ovvia, e nella fetta 1 quattro reticenze sopravvissero perché ne fu mutata una sola.
//
//   # | mutazione su `ARTHUD::ComposeVisibleIntentViews`          | test atteso ROSSO
//  ---|-----------------------------------------------------------|------------------------------------
//   1 | invertire il ramo: `if (bUnattendedSession)` → `if (!…)`   | PresidiataNascondeIlNemicoNonRivelato
//     |                                                           | + NonPresidiataMostraEntrambeLeSquadre
//   2 | ramo non presidiato: `FilterForTeam(Intent.TeamId, …)`     | NonPresidiataNonDuplicaIlRivelato
//     | → due domande di squadra sull'insieme intero               |
//   3 | ramo presidiato: `PlayerTeamId` → `Intent.TeamId`          | PresidiataNascondeIlNemicoNonRivelato
//   4 | accodare in ordine inverso (`Insert(…, 0)`)                | NonPresidiataConservaLOrdineDIngresso
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

	// ⛔ **Asserzione di controllo**: se la funzione tornasse sempre vuoto, le tre righe sopra passerebbero
	// per due terzi e il test direbbe che la privacy funziona su una lista che non esiste.
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
