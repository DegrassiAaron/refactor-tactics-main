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
	TestTrue(TEXT("la larghezza e' limitata, cosi' la board resta libera"), Posa.MaxWidth > 0.f);

	// ⛔ **E il tetto ha un limite SUPERIORE**, che fino al 2026-09-24 non era pinnato da niente: `460`
	// viveva come solo default di struct, e `> 0.f` lascia passare qualunque allargamento. Il widget
	// gemello lo asserisce da sempre — `RTPieSessionOverlayTests.cpp`, `Posa.MaxWidth <= 640.f`, col
	// messaggio *«e il tetto lascia libero il centro»* — quindi non e' un meccanismo da inventare.
	// 🔑 Il numero e' il valore corrente, non una soglia di design: serve a rendere ROSSO un
	// allargamento, che e' la direzione in cui il pannello invaderebbe il centro. La condivisione della
	// fascia bassa con `§6.7`/`§6.8` e' invece stata giudicata accettabile a schermo (#3319).
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
	/** Una vista che ECCEDE il tetto: una cella, `Eventi` eventi, una riga tecnica. */
	FRTContextInspectorView VistaConEventi(int32 Eventi)
	{
		FRTContextInspectorView V;
		V.CellLine = TEXT("(q=0,r=0,L=0) Floor cost=1 occupante=1 rev=1");
		for (int32 i = 0; i < Eventi; ++i)
		{
			V.EventLines.Add(FString::Printf(TEXT("T1 Blast/Combat evento-%d"), i));
		}
		V.TechnicalLines.Add(TEXT("[tecnico] vista per onnisciente · autorizza=si · rev=1"));
		return V;
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
	const FRTContextInspectorView Corta = VistaConEventi(2);
	const TArray<FString> RigheCorte = URTContextInspectorLibrary::VisibleLines(Corta, Tetto);
	TestEqual(TEXT("sotto il tetto entrano tutte"),
		RigheCorte.Num(), URTContextInspectorLibrary::AllLines(Corta).Num());
	TestTrue(TEXT("e la provenienza c'e'"), PortaLaProvenienza(RigheCorte));
	const FString CorteUnite = FString::Join(RigheCorte, TEXT("|"));
	TestFalse(TEXT("nessun marcatore quando non si taglia"),
		CorteUnite.Contains(URTContextInspectorLibrary::TruncationMarkerPrefix()));

	// —— Il caso che il difetto produceva: piu' righe del tetto.
	const FRTContextInspectorView Piena = VistaConEventi(Tetto * 2);
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
	TestTrue(TEXT("la provenienza sopravvive al pannello pieno"), PortaLaProvenienza(Viste));

	// E la cella resta in testa: senza, non si sa di quale esagono il pannello stia parlando.
	TestTrue(TEXT("la riga della cella resta per prima"),
		Viste.Num() > 0 && Viste[0] == Piena.CellLine);

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

	const FRTContextInspectorView Piena = VistaConEventi(Tetto * 2);
	const TArray<FString> Tutte = URTContextInspectorLibrary::AllLines(Piena);
	const TArray<FString> Viste = URTContextInspectorLibrary::VisibleLines(Piena, Tetto);

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
	TestTrue(TEXT("il marcatore porta il numero delle righe tolte"),
		Marcatore->Contains(FString::Printf(TEXT("%d"), Attese)));

	// ⛔ Anti-vacuita': con zero righe tolte il controllo sopra sarebbe verde su un marcatore che dice
	// «0», cioe' un pannello che dichiara un taglio che non c'e' stato.
	TestTrue(TEXT("e le righe tolte sono almeno una"), Attese > 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
