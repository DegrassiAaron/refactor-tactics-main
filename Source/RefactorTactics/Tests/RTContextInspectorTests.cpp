// Il Context Inspector (#2485) — cio' che si verifica senza aprire il gioco.
//
// ⚠️ Dichiarato: questi test NON provano che un pannello compaia a schermo. Provano che la vista non si
// COSTRUISCE con cio' che l'osservatore non ha diritto di sapere, che e' dove vive l'invariante.

#include "Debug/RTContextInspector.h"
#include "Map/RTHexCellData.h"
#include "Misc/AutomationTest.h"
#include "Perception/RTTeamKnowledge.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
