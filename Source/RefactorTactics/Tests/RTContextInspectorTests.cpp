// Il Context Inspector (#2485) — cio' che si verifica senza aprire il gioco.
//
// ⚠️ Dichiarato: questi test NON provano che un pannello compaia a schermo. Provano che la vista non si
// COSTRUISCE con cio' che l'osservatore non ha diritto di sapere, che e' dove vive l'invariante.

#include "Debug/RTContextInspector.h"
#include "Map/RTHexCellData.h"
#include "Misc/AutomationTest.h"
#include "Perception/RTTeamKnowledge.h"
#include "PieSession/RTPieVerdictOverlay.h" // la posa con cui questa non deve collidere
#include "Replay/RTReplayPrivacyLibrary.h"
#include "Tests/RTReflectedFieldsForTest.h"
#include "Tests/RTHudGeometryForTest.h" // il keep-out centrale, e l'intersezione di due rettangoli
#include "Styling/CoreStyle.h"             // lo stile da cui l'altezza di riga e' derivata
#include "Styling/SlateTypes.h"            // FTextBlockStyle
#include "Turn/RTHexSim.h"
#include "Turn/RTIntentPrivacyLibrary.h" // FRTPlannedIntent completo: il test ne costruisce un array vuoto
#include "Turn/RTTurnLog.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** La cella ispezionata, uguale in tutti i test di questo file. */
	FRTHexCellData CellaInEsame()
	{
		FRTHexCellData Cella;
		Cella.Id = FRTCellId(2, -1, 0);
		Cella.Surface = ERTHexSurface::Floor;
		return Cella;
	}

	/** Uno snapshot costruito PER un osservatore, cosi' che `DescribeCell` componga invece di rifiutare. */
	FRTHexSnapshot SnapshotPer(int32 ObserverTeamId)
	{
		FRTHexSnapshot S;
		S.ObserverTeamId = ObserverTeamId;
		S.Revision = 11;
		return S;
	}
}

/**
 * **La vista non porta tipi grezzi** — l'AC *«verificabile per assenza in code review»* di #2485, resa
 * meccanica invece che affidata a un paio d'occhi.
 *
 * 🔑 **Allowlist e non blocklist.** Vietare `FRTTurnLogEntry` e `FRTPlannedIntent` per nome lascerebbe
 * passare il prossimo tipo grezzo che qualcuno aggiunge — e sarebbe il difetto nella sua forma piu'
 * comune: un gate che copre gli esempi citati nella issue e nulla oltre. Qui si dichiara cosa **puo'**
 * stare nella vista, e tutto il resto e' rosso finche' qualcuno non lo dichiara.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTContextViewCarriesNoRawTypesTest,
	"RefactorTactics.Debug.ContextViewCarriesNoRawTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTContextViewCarriesNoRawTypesTest::RunTest(const FString&)
{
	const UStruct* Tipo = FRTContextInspectorView::StaticStruct();

	// Le sole struct ammesse dentro la vista. `FRTCellId` e' una COORDINATA: non porta ne' intenti ne'
	// voci di traccia, e senza di lei il pannello non saprebbe di quale esagono parla.
	const TSet<FName> StructAmmesse = { FName(TEXT("RTCellId")) };

	int32 Esaminate = 0;
	for (TFieldIterator<FProperty> It(Tipo); It; ++It)
	{
		++Esaminate;
		const FProperty* Prop = *It;

		// Dentro un array conta l'elemento, non il contenitore: `TArray<FRTTurnLogEntry>` deve essere rosso.
		const FProperty* Elemento = Prop;
		if (const FArrayProperty* Array = CastField<FArrayProperty>(Prop))
		{
			Elemento = Array->Inner;
		}

		if (const FStructProperty* AsStruct = CastField<FStructProperty>(Elemento))
		{
			TestTrue(
				*FString::Printf(
					TEXT("`%s` porta la struct `%s`, che non e' fra quelle ammesse nella vista"),
					*Prop->GetName(), *AsStruct->Struct->GetFName().ToString()),
				StructAmmesse.Contains(AsStruct->Struct->GetFName()));
		}
	}

	// Anti-vacuita': una vista senza campi renderebbe il ciclo verde per assenza di soggetto.
	TestTrue(TEXT("la vista dichiara almeno un campo"), Esaminate > 0);
	return true;
}

/**
 * **Nessun campo `AuditOnly` entra nel `PLAYER VIEW`** — l'AC di #2485, verificata **iterando**
 * `FieldVisibility()` e non elencando i campi una seconda volta.
 *
 * 🔑 **Due meta', e la prima da sola non basterebbe.** Che il DTO pubblico non DICHIARI quei campi e'
 * struttura; che i loro VALORI non finiscano nelle righe composte e' comportamento, e un compositore che
 * leggesse la voce d'audit invece del DTO passerebbe la prima meta' e fallirebbe la seconda.
 *
 * ⛔ **E' la ragione per cui questo pannello non riusa `URTTurnLogLibrary::DescribeEntry`**, che pure e'
 * l'owner della prosa: quel traduttore legge `Entry.ReactionResponse` — classificato `AuditOnly` — e lo
 * stampa alla lettera. ⚠️ Svuotare il campo non sarebbe una redazione ma una bugia: la voce `Chosen`
 * cadrebbe nel ramo del vuoto e si leggerebbe *«tiene il colpo, resta armata»*.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTContextViewHasNoAuditOnlyFieldTest,
	"RefactorTactics.Debug.ContextViewHasNoAuditOnlyField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTContextViewHasNoAuditOnlyFieldTest::RunTest(const FString&)
{
	// I nomi `AuditOnly`, presi ITERANDO la tabella: nessun elenco scritto qui.
	TSet<FName> SoloAudit;
	for (const TPair<FName, ERTReplayFieldVisibility>& Row : URTReplayPrivacyLibrary::FieldVisibility())
	{
		if (Row.Value == ERTReplayFieldVisibility::AuditOnly) { SoloAudit.Add(Row.Key); }
	}
	// Anti-vacuita': con zero campi audit ogni controllo sotto sarebbe verde per assenza di soggetto.
	TestTrue(TEXT("la tabella dichiara almeno un campo AuditOnly"), SoloAudit.Num() > 0);

	// —— (1) struttura: nessuno di quei nomi e' un campo del DTO pubblico.
	const TSet<FName> Pubblici =
		RTTestReflection::ReflectedNames(FRTPublicReplayEntry::StaticStruct());
	TestTrue(TEXT("il DTO pubblico dichiara dei campi"), Pubblici.Num() > 0);
	for (const FName& Nome : SoloAudit)
	{
		TestFalse(*FString::Printf(TEXT("`%s` e' AuditOnly e non sta nel DTO pubblico"), *Nome.ToString()),
			Pubblici.Contains(Nome));
	}

	// —— (2) comportamento: i loro VALORI non compaiono nelle righe composte.
	const FRTHexCellData Cella = CellaInEsame();
	constexpr int32 Squadra0 = 0;

	FRTTurnLogEntry Voce;
	// Il verdetto lascia passare la squadra 0: senza, `FilterEntriesForObserver` scarta la voce e questo
	// test sarebbe verde su zero righe — il falso verde piu' facile da ottenere qui.
	Voce.Verdict = FRTKnowledgeVerdict::Everyone();
	Voce.TurnNumber = 3;
	// 🔴 **Categoria e outcome ESATTI, e la prima stesura li aveva sbagliati.** Il ramo di
	// `URTTurnLogLibrary::DescribeEntry` che stampa `ReactionResponse` alla lettera vive sotto
	// `ERTLogCategory::ReactionDecision` **e** `ERTReactionDecisionOutcome::Chosen`. Con una voce
	// `Reaction` generica la mutazione che compone dalla traccia di audit restava **verde**, e questo
	// sarebbe stato un gate nato verde: asseriva l'assenza di un token che quel percorso non avrebbe
	// stampato comunque. ⚠️ Se ne e' accorta la MUTAZIONE, non la lettura — ed e' esattamente il lavoro
	// per cui la mutazione esiste.
	Voce.Category = ERTLogCategory::ReactionDecision;
	Voce.Outcome = static_cast<uint8>(ERTReactionDecisionOutcome::Chosen);
	Voce.SrcCell = Cella.Id;
	Voce.TgtCell = Cella.Id;
	Voce.UnitId = 4;
	Voce.ActionId = TEXT("Action.Counter");
	// I tre campi `AuditOnly` che portano un valore leggibile, con token irripetibili.
	Voce.OpportunityId = TEXT("SEGRETOOPP4242");
	Voce.ReactionResponse = TEXT("SEGRETORESP7777");
	Voce.ReactionInstanceId = 31337;

	const FRTContextInspectorView Vista = URTContextInspectorLibrary::Compose(
		Squadra0, Cella, SnapshotPer(Squadra0), {}, { Voce }, ERTContextView::Player);

	// Anti-vacuita' del secondo controllo: la voce DEVE aver prodotto una riga, o i `TestFalse` qui sotto
	// sarebbero verdi su una vista vuota.
	if (!TestTrue(TEXT("la voce autorizzata ha prodotto una riga di evento"), Vista.EventLines.Num() > 0))
	{
		return false;
	}

	const FString Tutto = FString::Join(URTContextInspectorLibrary::AllLines(Vista), TEXT("|"));
	TestTrue(TEXT("e la riga porta cio' che e' PUBBLICO"), Tutto.Contains(TEXT("Action.Counter")));

	for (const TCHAR* Token : { TEXT("SEGRETOOPP4242"), TEXT("SEGRETORESP7777"), TEXT("31337") })
	{
		TestFalse(*FString::Printf(TEXT("il token audit `%s` non compare nella vista"), Token),
			Tutto.Contains(Token));
	}

	return true;
}

/**
 * **Il pannello mostra gli eventi DI QUESTA CELLA, e non quelli delle altre.**
 *
 * ⚠️ Senza questo, i due test qui sopra sarebbero verdi anche su un compositore che non mostra **mai**
 * un evento: la forma di falso verde piu' facile da ottenere riparando troppo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTContextInspectorShowsOnlyThisCellTest,
	"RefactorTactics.Debug.ContextInspectorShowsOnlyThisCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTContextInspectorShowsOnlyThisCellTest::RunTest(const FString&)
{
	const FRTHexCellData Cella = CellaInEsame();
	constexpr int32 Squadra0 = 0;

	auto Voce = [](const FRTCellId& Src, const FRTCellId& Tgt, const TCHAR* Azione)
	{
		FRTTurnLogEntry E;
		E.Verdict = FRTKnowledgeVerdict::Everyone();
		E.TurnNumber = 1;
		E.Category = ERTLogCategory::Move;
		E.SrcCell = Src;
		E.TgtCell = Tgt;
		E.UnitId = 2;
		E.ActionId = Azione;
		return E;
	};

	const FRTCellId Altrove(9, 9, 0);
	const TArray<FRTTurnLogEntry> Traccia = {
		Voce(Cella.Id, Altrove,   TEXT("Action.Parte")),   // parte da qui
		Voce(Altrove,  Cella.Id,  TEXT("Action.Arriva")),  // arriva qui
		Voce(Altrove,  Altrove,   TEXT("Action.Altrove")), // non la tocca
	};

	const FRTContextInspectorView Vista = URTContextInspectorLibrary::Compose(
		Squadra0, Cella, SnapshotPer(Squadra0), {}, Traccia, ERTContextView::Player);

	const FString Tutto = FString::Join(URTContextInspectorLibrary::AllLines(Vista), TEXT("|"));
	TestTrue(TEXT("un evento che PARTE da questa cella compare"), Tutto.Contains(TEXT("Action.Parte")));
	TestTrue(TEXT("un evento che ARRIVA su questa cella compare"), Tutto.Contains(TEXT("Action.Arriva")));
	// ⛔ E uno che non la tocca resta fuori: un pannello che mostrasse tutta la traccia non sarebbe un
	// ispettore di contesto, e mostrerebbe movimenti altrui su celle che l'osservatore non sta guardando.
	TestFalse(TEXT("un evento che NON la tocca resta fuori"), Tutto.Contains(TEXT("Action.Altrove")));
	TestEqual(TEXT("due eventi, non tre"), Vista.EventLines.Num(), 2);

	// Il pannello: consumatore sottile, nessun filtro proprio. Mostra cio' che riceve.
	URTContextInspectorWidgetBase* Pannello = NewObject<URTContextInspectorWidgetBase>();
	if (TestNotNull(TEXT("il widget base si costruisce"), Pannello))
	{
		TestFalse(TEXT("un pannello non armato non ha contenuto"), Pannello->HasContent());
		Pannello->ShowFor(Vista);
		TestTrue(TEXT("armato, ha contenuto"), Pannello->HasContent());
		TestEqual(TEXT("e mostra tutte le righe della vista"),
			Pannello->GetLines().Num(), URTContextInspectorLibrary::AllLines(Vista).Num());
		// L'intestazione dichiara PER CHI e' composta: una verita' parziale che non lo dicesse si
		// leggerebbe come la verita'.
		TestTrue(TEXT("l'intestazione nomina l'osservatore"),
			Pannello->GetHeaderText().ToString().Contains(TEXT("squadra 0")));
	}

	return true;
}

/**
 * **La posa del pannello non collide con l'overlay del verdetto** — e i due sono a schermo INSIEME,
 * perche' e' durante una seduta PIE che questo pannello viene giudicato.
 *
 * 🔴 **E' il difetto di #3242, reso meccanico.** Li' l'overlay del verdetto nasceva senza allineamento
 * esplicito, si posava in alto a sinistra — sopra i nomi degli eroi — e il verdetto d'autore alla prima
 * seduta reale fu *«la scritta sta sotto i nomi degli eroi e non e' visualizzabile»*. Una posa dentro
 * `RebuildWidget` non ha modo di essere rossa senza uno schermo; una posa che e' un **dato** si'.
 *
 * ⚠️ Questo test NON prova che il pannello si veda: prova che non sia stato messo dove qualcun altro c'e'
 * gia'. Il resto e' giudizio umano, ed e' la seduta `U59`.
 *
 * ⛔ Questa riga diceva `U55`, corretta il 2026-09-23. `U55` e' «I residui del mode Hex Map, e la versione
 * che il salvataggio scrive» — corsia `asset`, e il suo `verifies` porta `PIE-HEX-MODE-P/Q/R` e
 * `PIE-FMTVER`, non questa voce. Chi leggeva il test dal codice convocava una seduta di authoring asset.
 * La seduta che convoca `PIE-DEBUG-CONTEXT` e' `U59` — `editor-sessions.yaml`, `verifies:
 * [PIE-DEBUG-CONTEXT]`, e la cella di stato del registro lo ripete.
 * ⚠️ Il titolo va citato com'e' scritto: la prima stesura di questa correzione diceva «I residui
 * dell'authoring hex», una parafrasi che nel registro non esiste — `grep` non la trova, quindi chi
 * cercava quella seduta per nome non la trovava. Un puntatore che non risolve e' il difetto che questa
 * riga stessa doveva riparare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTContextInspectorPlacementTest,
	"RefactorTactics.Debug.ContextInspectorPlacementDoesNotCollide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTContextInspectorPlacementTest::RunTest(const FString&)
{
	URTContextInspectorWidgetBase* Pannello = NewObject<URTContextInspectorWidgetBase>();
	if (!TestNotNull(TEXT("il pannello si costruisce"), Pannello)) { return false; }
	const FRTContextInspectorPlacement Posa = Pannello->Placement();

	// `URTPieVerdictOverlay::Placement()` e' STATICA: si legge senza costruire il widget. E leggerla e'
	// il punto — un confronto contro due letterali scritti qui proverebbe solo che so copiare, e
	// resterebbe verde il giorno in cui qualcuno sposta quello invece di questo.
	const FRTPieOverlayPlacement PosaVerdetto = URTPieVerdictOverlay::Placement();

	// ⛔ **Non due costanti confrontate fra loro**: si legge la posa REALE di entrambi i widget, cosi' che
	// spostare uno dei due faccia diventare rosso questo test invece di lasciarlo verde su numeri scritti
	// qui. Un gate che confrontasse due letterali proverebbe solo che so copiare.
	TestFalse(
		TEXT("il pannello e l'overlay del verdetto non occupano lo STESSO angolo"),
		Posa.Horizontal == PosaVerdetto.Horizontal && Posa.Vertical == PosaVerdetto.Vertical);

	// E la colonna sinistra resta dell'overlay del verdetto: non basta differire in verticale, perche' due
	// pannelli nella stessa colonna si contendono comunque la larghezza.
	TestNotEqual(TEXT("e nemmeno la stessa COLONNA"),
		static_cast<int32>(Posa.Horizontal), static_cast<int32>(PosaVerdetto.Horizontal));

	// Il centro non si copre: e' il contratto dello Screen HUD, e un pannello centrato lo violerebbe
	// qualunque cosa dica il resto.
	TestNotEqual(TEXT("non e' centrato in orizzontale"),
		static_cast<int32>(Posa.Horizontal), static_cast<int32>(HAlign_Center));
	// ⌫ Il messaggio diceva «cosi' la board resta libera», e #3319 ha misurato che non e' vero: il
	// pannello copre le celle dietro di se', ed e' stato giudicato accettabile a schermo il 2026-09-24.
	TestTrue(TEXT("la larghezza ha un tetto"), Posa.MaxWidth > 0.f);

	// ⛔ **E il tetto ha un limite SUPERIORE**, che fino al 2026-09-24 non era pinnato da niente: `460`
	// viveva come solo default di struct, e `> 0.f` lascia passare qualunque allargamento. Il widget
	// gemello ha lo stesso asserto due file piu' in la' — `RTPieSessionOverlayTests.cpp`,
	// `Posa.MaxWidth <= 640.f`, col messaggio *«e il tetto lascia libero il centro»*.
	//
	// ⚠️ **Ma non e' la stessa asserzione, e vale dirlo**: li' il tetto asserito lascia 220 px di
	// margine sul valore corrente (`420.f`), cioe' e' una soglia di design; qui pinna il valore esatto,
	// margine zero. Serve a rendere rosso un allargamento di un pixel, non a dichiarare una soglia.
	//
	// ⛔ **E l'allargamento NON e' la direzione verso il centro**, come una prima stesura di questo
	// commento diceva: il pannello e' `VAlign_Bottom`, quindi vive sotto il keep-out centrale
	// (`RTCenterFree::CenterKeepOut`, Y 216..864 a 1920x1080) finche' la sua ALTEZZA resta contenuta.
	// A portarlo nel centro sarebbe l'altezza, e `FRTContextInspectorPlacement` non ha nessun tetto
	// d'altezza: resta da decidere in #3319.
	TestTrue(TEXT("e il tetto non cresce senza che qualcuno lo decida"), Posa.MaxWidth <= 460.f);

	return true;
}

/**
 * **`0` e' la squadra 0, non l'interruttore** — e nessun argomento valido spegne per sbaglio.
 *
 * 🔴 **Il difetto era reale ed e' arrivato fino a una seduta.** La prima stesura del comando trattava
 * `0` come lo spegnimento, mentre in ogni altro `rt.Debug.*` il primo argomento e' il **TeamId**:
 * `rt.Debug.DrawIntent 1` mostra i piani del team 1. La precondizione che avevo scritto nella voce
 * `PIE-DEBUG-CONTEXT` diceva *«console `rt.Debug.ContextInspector 0` per la squadra 0»*, cioe' il comando
 * che SPEGNE: chi avesse eseguito la seduta alla lettera avrebbe registrato un ❌ su un pannello che
 * funziona, e avrebbe cercato il difetto nella resa.
 *
 * ⚠️ **Il parsing e' una funzione PURA proprio per questo.** Dentro il comando era un `if` fra un mondo,
 * un widget e un viewport: niente di tutto cio' e' costruibile headless, quindi l'unico modo di
 * accorgersene era guardare uno schermo. Qui si prova senza nulla di tutto quello.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTContextInspectorArgsTest,
	"RefactorTactics.Debug.ContextInspectorArgsDoNotCollide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTContextInspectorArgsTest::RunTest(const FString&)
{
	// `TArray<FString>` si costruisce gia' da una initializer list: nessuna impalcatura per leggere
	// quattro casi, e una dipendenza in meno da `<initializer_list>`.
	auto Leggi = [](const TArray<FString>& Args)
	{
		return URTContextInspectorLibrary::ParseCommandArgs(Args);
	};

	// Nessun argomento: mostra, squadra 0. E il default non e' lo spegnimento.
	const FRTContextInspectorRequest Vuoto = Leggi({});
	TestFalse(TEXT("senza argomenti NON spegne"), Vuoto.bOff);
	TestEqual(TEXT("senza argomenti e' la squadra 0"), Vuoto.ObserverTeamId, 0);

	// 🔑 Il cuore del test: `0` e' una SQUADRA.
	const FRTContextInspectorRequest Zero = Leggi({ TEXT("0") });
	TestFalse(TEXT("`0` NON spegne: e' la squadra 0"), Zero.bOff);
	TestEqual(TEXT("e vale proprio 0"), Zero.ObserverTeamId, 0);

	const FRTContextInspectorRequest Uno = Leggi({ TEXT("1") });
	TestFalse(TEXT("`1` non spegne"), Uno.bOff);
	TestEqual(TEXT("`1` e' la squadra 1"), Uno.ObserverTeamId, 1);

	// `-1` e' l'onnisciente, ed e' una posizione nominata: non un valore speciale inventato qui.
	const FRTContextInspectorRequest Onni = Leggi({ TEXT("-1") });
	TestFalse(TEXT("`-1` non spegne"), Onni.bOff);
	TestEqual(TEXT("`-1` e' l'osservatore onnisciente"), Onni.ObserverTeamId,
		static_cast<int32>(RTObserver::Omniscient));

	// L'interruttore e' una PAROLA, che nessun TeamId puo' essere. Cassa indifferente: chi digita in
	// console non ha motivo di ricordarsela.
	TestTrue(TEXT("`off` spegne"), Leggi({ TEXT("off") }).bOff);
	TestTrue(TEXT("`OFF` spegne lo stesso"), Leggi({ TEXT("OFF") }).bOff);

	// ⛔ Anti-vacuita': se `bOff` fosse sempre falso i controlli qui sopra sarebbero verdi per meta',
	// e il comando non avrebbe modo di spegnersi. Almeno un argomento DEVE spegnere.
	TestNotEqual(TEXT("lo spegnimento esiste e si distingue dalla squadra 0"),
		Leggi({ TEXT("off") }).bOff, Zero.bOff);

	return true;
}

namespace
{
	/**
	 * La riga tecnica, nella forma che `URTDebugReportLibrary::DescribeContext` compone davvero —
	 * `[tecnico] vista per %s · snapshot costruito per %s · autorizza=%s · rev=%d`.
	 *
	 * ⚠️ **I quattro campi non si abbreviano**, ed e' il punto di #3320: `vista per` duplica
	 * l'intestazione, ma gli altri tre no. `snapshot costruito per` puo' DIVERGERE da `vista per` — e'
	 * la divergenza per cui `DescribeCell` stampa `NON-COMPOSTO(...)` — mentre `autorizza` dice con
	 * quale titolo e `rev` su quale stato. Una fixture che li omettesse renderebbe il test verde su una
	 * riga piu' facile di quella vera.
	 */
	FString RigaTecnica(int32 Revisione)
	{
		return FString::Printf(
			TEXT("[tecnico] vista per onnisciente · snapshot costruito per onnisciente · ")
			TEXT("autorizza=si · rev=%d"), Revisione);
	}

	/** Una vista con corpo: `Intenti` intenti e `Eventi` eventi, una cella, una riga tecnica. */
	FRTContextInspectorView VistaCon(int32 Intenti, int32 Eventi)
	{
		FRTContextInspectorView V;
		V.CellLine = TEXT("(q=0,r=0,L=0) Floor cost=1 occupante=1 rev=1");
		for (int32 i = 0; i < Intenti; ++i)
		{
			V.IntentLines.Add(FString::Printf(TEXT("[RT] alleata intento-%d"), i));
		}
		for (int32 i = 0; i < Eventi; ++i)
		{
			V.EventLines.Add(FString::Printf(TEXT("T1 Blast/Combat evento-%d"), i));
		}
		V.TechnicalLines.Add(RigaTecnica(1));
		return V;
	}

	/** Il corpo nell'ordine in cui `AllLines` lo compone: prima gli intenti, poi gli eventi. */
	TArray<FString> CorpoAtteso(const FRTContextInspectorView& V)
	{
		TArray<FString> C;
		C.Append(V.IntentLines);
		C.Append(V.EventLines);
		return C;
	}

	/** La riga della provenienza, cercata per il suo marcatore e non per il testo intero. */
	bool PortaLaProvenienza(const TArray<FString>& Righe)
	{
		return Righe.ContainsByPredicate(
			[](const FString& R) { return R.StartsWith(TEXT("[tecnico]")); });
	}
}

/**
 * **A pannello PIENO la provenienza resta, e il taglio si dichiara** — #3320.
 *
 * 🔴 **Il difetto che questo test rende rosso, misurato nella seduta `U59` del 2026-09-24.**
 * `[tecnico]` e' l'ULTIMA riga di `AllLines`, quindi la prima a cadere sotto il tetto: sulla cella
 * `(q=0,r=0,L=0)` del banco `Visual.Combat.GuardVsBraceUnderSmallHits` la vista componeva **17** righe
 * contro un tetto di 12, e a schermo la riga che dice *per chi* la vista e' composta non c'era.
 *
 * ⚠️ **E il taglio era MUTO**, che e' la meta' piu' grave: chi guarda un pannello pieno conclude che su
 * quella cella non sia successo altro. E' lo stesso ragionamento che `RTDebugConsole.cpp` rifiuta di
 * permettere due funzioni piu' in la', dove preferisce **dire** che non c'e' una cella valida invece di
 * mostrare un pannello vuoto — *«una cella non valida e un contesto vuoto si vedono uguali»*.
 *
 * 🔑 **Il tetto si legge, non si riscrive**: `MaxRighe` e' pubblica apposta. Un `12` letterale qui
 * sarebbe il numero che invecchia da solo, verde il giorno in cui qualcuno cambia il tetto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTContextInspectorKeepsProvenanceWhenFullTest,
	"RefactorTactics.Debug.ContextInspectorKeepsProvenanceWhenFull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTContextInspectorKeepsProvenanceWhenFullTest::RunTest(const FString&)
{
	constexpr int32 Tetto = URTContextInspectorWidgetBase::MaxRighe;

	// —— Anti-vacuita' PRIMA del caso pieno: sotto il tetto non si taglia e non si dichiara niente.
	// Senza questo, un `VisibleLines` che tagliasse SEMPRE passerebbe la meta' interessante del test.
	const FRTContextInspectorView Corta = VistaCon(1, 1);
	const TArray<FString> RigheCorte = URTContextInspectorLibrary::VisibleLines(Corta, Tetto);
	TestEqual(TEXT("sotto il tetto entrano tutte"),
		RigheCorte.Num(), URTContextInspectorLibrary::AllLines(Corta).Num());
	TestTrue(TEXT("e la provenienza c'e'"), PortaLaProvenienza(RigheCorte));
	const FString CorteUnite = FString::Join(RigheCorte, TEXT("|"));
	TestFalse(TEXT("nessun marcatore quando non si taglia"),
		CorteUnite.Contains(URTContextInspectorLibrary::TruncationMarkerPrefix()));

	// —— Il caso che il difetto produceva: piu' righe del tetto, e con intenti E eventi, cosi' che il
	// ramo `Corpo.Append(View.IntentLines)` sia esercitato invece di restare invisibile.
	const FRTContextInspectorView Piena = VistaCon(3, Tetto * 2);
	const TArray<FString> Tutte = URTContextInspectorLibrary::AllLines(Piena);
	// Anti-vacuita': se la vista non eccedesse, ogni controllo qui sotto sarebbe verde per assenza di
	// soggetto — la forma di falso verde che questo file combatte in ogni altro test.
	if (!TestTrue(TEXT("la vista ECCEDE davvero il tetto"), Tutte.Num() > Tetto))
	{
		return false;
	}

	const TArray<FString> Viste = URTContextInspectorLibrary::VisibleLines(Piena, Tetto);
	TestEqual(TEXT("si rende esattamente il tetto"), Viste.Num(), Tetto);

	// ⛔ Il difetto vero: la provenienza cade per ULTIMA in `AllLines`, quindi per prima sotto il taglio.
	// E si asserisce che sia l'ULTIMA riga, non solo che ci sia: appesa altrove sarebbe verde qui e
	// sbagliata a schermo.
	TestTrue(TEXT("la provenienza sopravvive al pannello pieno"), PortaLaProvenienza(Viste));
	TestEqual(TEXT("ed e' l'ultima riga, come in AllLines"), Viste.Last(), Piena.TechnicalLines[0]);

	// E la cella resta in testa: senza, non si sa di quale esagono il pannello stia parlando.
	TestEqual(TEXT("la riga della cella resta per prima"), Viste[0], Piena.CellLine);

	// 🔴 **Il CORPO, e non solo la cornice.** Senza questi tre asserti una mutazione che riempisse gli
	// slot di mezzo con stringhe vuote — o con `Corpo[0]` ripetuto, o col corpo all'incontrario —
	// passerebbe tutto il resto del test: dodici righe, provenienza presente, cella in testa, e un
	// pannello che non mostra niente.
	const TArray<FString> Corpo = CorpoAtteso(Piena);
	const int32 Entrano = Tetto - (1 /*cella*/ + Piena.TechnicalLines.Num() + 1 /*marcatore*/);
	if (TestTrue(TEXT("qualche riga di corpo entra"), Entrano > 0))
	{
		TestEqual(TEXT("il corpo comincia dalla prima riga della vista"), Viste[1], Corpo[0]);
		TestEqual(TEXT("e finisce sulla riga che il tetto consente"),
			Viste[Entrano], Corpo[Entrano - 1]);
		// Il marcatore sta fra il corpo e la provenienza: e' li' che dichiara cosa manca.
		TestTrue(TEXT("il marcatore segue il corpo"),
			Viste[Entrano + 1].StartsWith(URTContextInspectorLibrary::TruncationMarkerPrefix()));
	}

	// —— La CUCITURA: che il widget consumi `VisibleLines` e non `AllLines`. Senza questo asserto,
	// rimettere `GetLines()` dentro `RebuildWidget` lascerebbe tutta la suite verde — cioe' il modulo
	// puro provato e il pannello invariato, che e' proprio il difetto che #3320 denuncia altrove.
	URTContextInspectorWidgetBase* Pannello = NewObject<URTContextInspectorWidgetBase>();
	if (TestNotNull(TEXT("il widget si costruisce"), Pannello))
	{
		Pannello->ShowFor(Piena);
		TestEqual(TEXT("il widget rende cio' che entra nel tetto"),
			Pannello->GetVisibleLines().Num(), Tetto);
		TestNotEqual(TEXT("e non tutte le righe composte"),
			Pannello->GetVisibleLines().Num(), Pannello->GetLines().Num());
	}
	// ⚠️ **Limite dichiarato**: resta scoperta la sola lambda di `RebuildWidget`, che e' Slate e non si
	// costruisce headless. L'asserto qui sopra sposta il buco da due funzioni a una riga.

	return true;
}

/**
 * **Il taglio dice QUANTE righe ha tolto** — #3320, seconda meta'.
 *
 * ⚠️ Che la provenienza sopravviva non basta: se le righe di mezzo sparissero in silenzio, chi guarda
 * concluderebbe che su quella cella non sia successo altro. Il numero e' la differenza fra un pannello
 * che tace e uno che dichiara il proprio limite.
 *
 * 🔑 **Il conteggio si DERIVA, non si scrive**: e' `AllLines` meno cio' che entra, e cosi' resta vero se
 * qualcuno cambia il tetto o l'ordine delle righe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTContextInspectorDeclaresTheTruncationTest,
	"RefactorTactics.Debug.ContextInspectorDeclaresTheTruncation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTContextInspectorDeclaresTheTruncationTest::RunTest(const FString&)
{
	constexpr int32 Tetto = URTContextInspectorWidgetBase::MaxRighe;

	const FRTContextInspectorView Piena = VistaCon(3, Tetto * 2);
	const TArray<FString> Tutte = URTContextInspectorLibrary::AllLines(Piena);
	const TArray<FString> Viste = URTContextInspectorLibrary::VisibleLines(Piena, Tetto);
	TestEqual(TEXT("si rende esattamente il tetto"), Viste.Num(), Tetto);

	const FString* Marcatore = Viste.FindByPredicate([](const FString& R)
	{
		return R.StartsWith(URTContextInspectorLibrary::TruncationMarkerPrefix());
	});
	if (!TestNotNull(TEXT("il pannello pieno dichiara di aver tagliato"), Marcatore))
	{
		return false;
	}

	// Quante righe di MEZZO non sono entrate: il totale meno quelle rese, meno il marcatore stesso che
	// occupa uno slot. Derivato, cosi' non invecchia con il tetto.
	const int32 Attese = Tutte.Num() - (Viste.Num() - 1);

	// ⛔ **Uguaglianza sulla stringa INTERA, non `Contains` sul numero.** Una sottostringa lascerebbe
	// passare un marcatore che dicesse `150` dove ne servono `15`, e non pinnerebbe il suffisso —
	// che oggi nessun altro asserto guarda.
	TestEqual(TEXT("il marcatore dice esattamente quante righe ha tolto"), *Marcatore,
		FString::Printf(TEXT("%s%d righe"),
			URTContextInspectorLibrary::TruncationMarkerPrefix(), Attese));

	// ⛔ Anti-vacuita': con zero righe tolte il controllo sopra sarebbe verde su un marcatore che dice
	// «0», cioe' un pannello che dichiara un taglio che non c'e' stato.
	TestTrue(TEXT("e le righe tolte sono almeno una"), Attese > 0);

	return true;
}

/**
 * **Le guardie di `VisibleLines`** — i rami che la vista di prova non attraversa, e che senza un caso
 * proprio resterebbero mutabili senza che niente diventi rosso.
 *
 * ⚠️ Nessuno di questi e' teorico: `MaxLines` arriva da `MaxRighe`, che un `WBP_` derivato o una
 * revisione del layout possono cambiare, e `TechnicalLines` e' un array — oggi ne porta una, domani
 * puo' portarne due.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTContextInspectorTruncationGuardsTest,
	"RefactorTactics.Debug.ContextInspectorTruncationGuards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTContextInspectorTruncationGuardsTest::RunTest(const FString&)
{
	const FRTContextInspectorView V = VistaCon(2, 20);

	// —— Tetto nullo o negativo: niente entra. Restituire `Tutte` qui riempirebbe un layout che non ha
	// slot, cioe' righe composte e mai rese.
	TestEqual(TEXT("con tetto 0 non entra niente"),
		URTContextInspectorLibrary::VisibleLines(V, 0).Num(), 0);
	TestEqual(TEXT("e con tetto negativo nemmeno"),
		URTContextInspectorLibrary::VisibleLines(V, -1).Num(), 0);

	// —— Il ramo DEGRADATO: quando nemmeno le riservate entrano (cella + tecnica + marcatore = 3), non
	// c'e' spazio per dichiarare niente e si taglia secco. Un pannello che usasse i suoi ultimi slot per
	// parlare del proprio troncamento direbbe meno di uno che mostra le prime righe e basta.
	for (int32 Tetto : { 1, 2, 3 })
	{
		const TArray<FString> R = URTContextInspectorLibrary::VisibleLines(V, Tetto);
		TestEqual(*FString::Printf(TEXT("con tetto %d si rende esattamente %d"), Tetto, Tetto),
			R.Num(), Tetto);
		TestEqual(*FString::Printf(TEXT("e con tetto %d si taglia secco dall'inizio"), Tetto),
			R[0], V.CellLine);
	}

	// —— Piu' di una riga tecnica: tutte devono sopravvivere, perche' sono la provenienza e il motivo
	// per cui questa funzione esiste.
	FRTContextInspectorView Due = VistaCon(0, 20);
	Due.TechnicalLines.Add(RigaTecnica(2));
	const TArray<FString> RDue = URTContextInspectorLibrary::VisibleLines(
		Due, URTContextInspectorWidgetBase::MaxRighe);
	TestEqual(TEXT("con due righe tecniche si rende comunque il tetto"),
		RDue.Num(), static_cast<int32>(URTContextInspectorWidgetBase::MaxRighe));
	TestEqual(TEXT("e ci sono entrambe"),
		RDue.FilterByPredicate([](const FString& R) { return R.StartsWith(TEXT("[tecnico]")); }).Num(),
		2);

	// —— Vista senza riga di cella: non si inventa uno slot vuoto in testa.
	FRTContextInspectorView SenzaCella = VistaCon(0, 20);
	SenzaCella.CellLine.Empty();
	const TArray<FString> RSenza = URTContextInspectorLibrary::VisibleLines(
		SenzaCella, URTContextInspectorWidgetBase::MaxRighe);
	TestEqual(TEXT("senza riga di cella si rende comunque il tetto"),
		RSenza.Num(), static_cast<int32>(URTContextInspectorWidgetBase::MaxRighe));
	TestFalse(TEXT("e la prima riga non e' vuota"), RSenza[0].IsEmpty());

	return true;
}

namespace
{
	/**
	 * L'altezza di una riga di testo alla risoluzione di riferimento, in pixel.
	 *
	 * ⚠️ **E' una DERIVAZIONE dichiarata, non una misura su una resa**, e il test qui sotto la difende
	 * invece di fidarsene: lo stile `NormalText` porta Roboto Regular a **10 pt**
	 * (`FStarshipCoreStyle`, che e' cio' che `FCoreStyle::Get()` restituisce davvero — `Get()` inoltra a
	 * `FAppStyle::Get()`). Da li': `10 pt` a 96 DPI → `((10*64)*96+36)/72 = 853` in 26.6 → **13 px** di
	 * ppem; Roboto ha `unitsPerEm 2048` e `hhea` ascender 1900 / descender -500 → `height = 2400`;
	 * `FT_MulFix(2400, 13*64*65536/2048) = 975` → `(975+32)>>6` = **15 px**.
	 *
	 * 🔑 L'anello fragile e' il **punto di partenza**, non l'aritmetica: se qualcuno cambia lo stile, i
	 * 15 px non valgono piu'. Per questo il test asserisce la dimensione del font **letta a runtime**,
	 * cosi' un cambio di stile diventa rosso qui invece che a schermo.
	 */
	constexpr float AltezzaRigaPx = 15.f;

	/** I punti da cui `AltezzaRigaPx` e' derivata. Se cambiano, la derivazione va rifatta. */
	constexpr float PuntiFontAttesi = 10.f;

	/** L'altezza del pannello, calcolata dai DATI della posa — non da una resa. */
	float AltezzaPannello(const FRTContextInspectorPlacement& Posa, int32 Righe, int32 CapoRiga)
	{
		const float Bordo = Posa.BorderPaddingY * 2.f;
		const float Intestazione = AltezzaRigaPx + Posa.HeaderSpacing;
		return Bordo + Intestazione + AltezzaRigaPx * static_cast<float>(Righe + CapoRiga);
	}
}

/**
 * **Il pannello resta sotto il keep-out centrale** — il ⛔ che `PIE-DEBUG-CONTEXT` dichiara dal
 * 2026-09-24, reso misurabile invece che alluso.
 *
 * 🔴 **Il criterio precedente era inosservabile, e questo rischiava di esserlo uguale.** *«Non deve
 * coprire la board»* e' stato ritirato perche' non rispettabile senza cambiare la posa (#3319); il suo
 * sostituto — *«non deve invadere il centro»* — sarebbe stato la stessa cosa finche' «il centro» non
 * aveva un referente e l'altezza non era un dato. Ora il referente e' `RTCenterFree::CenterKeepOut()` e
 * l'altezza si calcola da `FRTContextInspectorPlacement`.
 *
 * ⛔ **E il pannello NON ci stava.** Con `MaxRighe = 12` l'altezza era `219 px` contro un budget di
 * `192`: dentro il keep-out **sempre**, perche' i dodici slot esistono anche a pannello quasi vuoto.
 * Il tetto e' sceso a **9**, e questo test e' la ragione per cui non puo' risalire in silenzio.
 *
 * 🔑 **Un capo-riga di tolleranza, e non e' teorico**: con l'osservatore onnisciente la riga
 * `[tecnico]` supera la larghezza utile e va a capo. Dieci righe sarebbero entrate senza margine e
 * sarebbero uscite al primo `rt.Debug.ContextInspector -1` — cioe' proprio l'argomento con cui la
 * seduta `U59` ha riempito il pannello.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTContextInspectorFitsUnderTheKeepOutTest,
	"RefactorTactics.Debug.ContextInspectorFitsUnderTheKeepOut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTContextInspectorFitsUnderTheKeepOutTest::RunTest(const FString&)
{
	URTContextInspectorWidgetBase* Pannello = NewObject<URTContextInspectorWidgetBase>();
	if (!TestNotNull(TEXT("il pannello si costruisce"), Pannello)) { return false; }
	const FRTContextInspectorPlacement Posa = Pannello->Placement();

	// —— L'assunzione su cui poggia tutta l'aritmetica, letta a runtime invece che creduta.
	const FTextBlockStyle& Stile = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>(TEXT("NormalText"));
	TestEqual(TEXT("il font e' quello su cui l'altezza di riga e' stata derivata"),
		Stile.Font.Size, PuntiFontAttesi);

	// —— Il budget: dal bordo inferiore del keep-out al fondo del pannello.
	const RTCenterFree::FRect KeepOut = RTCenterFree::CenterKeepOut();
	const float Budget = (RTCenterFree::RefHeight - KeepOut.Bottom) - Posa.Margin;
	// Anti-vacuita': con un budget nullo o negativo ogni `<=` qui sotto sarebbe una domanda diversa.
	if (!TestTrue(TEXT("il keep-out lascia una fascia bassa"), Budget > 0.f)) { return false; }

	const float Altezza = AltezzaPannello(Posa, URTContextInspectorWidgetBase::MaxRighe, 0);
	TestTrue(*FString::Printf(TEXT("il pannello (%.0f px) sta nella fascia sotto il keep-out (%.0f px)"),
		Altezza, Budget), Altezza <= Budget);

	// ⛔ E con UN capo-riga: e' la tolleranza che ha deciso 9 invece di 10, e senza questo asserto
	// qualcuno potrebbe risalire a 10 restando verde.
	const float ConCapoRiga = AltezzaPannello(Posa, URTContextInspectorWidgetBase::MaxRighe, 1);
	TestTrue(*FString::Printf(TEXT("e ci sta anche con un capo-riga (%.0f px)"), ConCapoRiga),
		ConCapoRiga <= Budget);

	// —— Il RETTANGOLO, che e' cio' che #3319 chiedeva: non un enum di allineamento.
	const RTCenterFree::FRect Rett{
		RTCenterFree::RefWidth - Posa.Margin - Posa.MaxWidth,
		RTCenterFree::RefHeight - Posa.Margin - Altezza,
		RTCenterFree::RefWidth - Posa.Margin,
		RTCenterFree::RefHeight - Posa.Margin };

	TestFalse(*FString::Printf(TEXT("il pannello %s non tocca il keep-out %s"),
		*RTCenterFree::Descrivi(Rett), *RTCenterFree::Descrivi(KeepOut)),
		RTCenterFree::SiToccano(Rett, KeepOut));

	// ⚠️ **Limite dichiarato, e va letto insieme al verde qui sopra**: `MaxWidth` e `MaxDesiredWidth`
	// sono tetti sulla dimensione DESIDERATA, e nessuno ritaglia. Questo rettangolo e' quindi il caso
	// peggiore che i dati consentono, non la resa. Un contenuto che eccedesse i tetti uscirebbe dal
	// bordo senza che questo test lo veda: quella meta' resta di PIE.

	return true;
}

/**
 * **Il pannello e l'overlay del verdetto non si toccano, misurato sulle COLONNE** — non su un enum.
 *
 * 🔑 **Perche' le colonne e non i rettangoli interi.** Nessuno dei due widget dichiara un'altezza
 * fissa: quella del Context Inspector si calcola dal tetto di righe, quella di `URTPieVerdictOverlay`
 * dipende dal testo del prompt e non e' un dato. Due rettangoli non si toccano se sono separati su
 * **anche un solo** asse, e qui l'asse separato e' X — quindi la non-collisione si prova per intero
 * senza inventare l'altezza che manca. ⌫ La prima stesura del gate confrontava
 * `Posa.Horizontal != PosaVerdetto.Horizontal`: vero anche per un pannello largo 2000 px.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTContextInspectorColumnsDoNotOverlapTest,
	"RefactorTactics.Debug.ContextInspectorColumnsDoNotOverlap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTContextInspectorColumnsDoNotOverlapTest::RunTest(const FString&)
{
	URTContextInspectorWidgetBase* Pannello = NewObject<URTContextInspectorWidgetBase>();
	if (!TestNotNull(TEXT("il pannello si costruisce"), Pannello)) { return false; }
	const FRTContextInspectorPlacement Posa = Pannello->Placement();
	const FRTPieOverlayPlacement PosaVerdetto = URTPieVerdictOverlay::Placement();

	// Il pannello e' ancorato a destra; l'overlay a sinistra. Le due colonne, in pixel di riferimento.
	const float PannelloSinistra = RTCenterFree::RefWidth - Posa.Margin - Posa.MaxWidth;
	const float PannelloDestra = RTCenterFree::RefWidth - Posa.Margin;
	const float VerdettoSinistra = PosaVerdetto.LeftMargin;
	const float VerdettoDestra = PosaVerdetto.LeftMargin + PosaVerdetto.MaxWidth;

	// Anti-vacuita': due colonne di larghezza nulla non si toccherebbero per costruzione.
	TestTrue(TEXT("entrambe le colonne hanno larghezza"),
		PannelloDestra > PannelloSinistra && VerdettoDestra > VerdettoSinistra);

	TestTrue(*FString::Printf(
		TEXT("la colonna del pannello (%.0f..%.0f) sta a destra di quella del verdetto (%.0f..%.0f)"),
		PannelloSinistra, PannelloDestra, VerdettoSinistra, VerdettoDestra),
		PannelloSinistra >= VerdettoDestra);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
