// La riga d'attivazione del Registro e la sua fusione con le righe d'esito (#4033, D-600 (1), regole R5, R6 e R7b).
//
// Tutto puro: nessun mondo, nessun manager. `URTHudViewModel::MergeActivationLines` riceve le righe d'esito GIA' tagliate
// come oggi e le attivazioni GIA' composte, e decide soltanto il pubblico (la squadra), lo spazio e l'ordine.
//
// ⚠️ Nomi `RigaAtt…` apposta: sotto unity build le funzioni dei namespace anonimi di file diversi finiscono nello stesso
// blocco, e due helper omonimi non compilano.

#include "Misc/AutomationTest.h"
#include "UI/RTActivationLine.h"
#include "UI/RTHudViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Una riga d'esito come la produce `BuildPlayerEventFeed`: il numero e' la posizione, da 1. */
	FRTPlayerEventLineView RigaAttEsito(int32 UnitId, const TCHAR* Testo, int32 Sequenza, int32 Turno = 0)
	{
		FRTPlayerEventLineView Riga;
		Riga.Text = FText::FromString(Testo);
		Riga.PrimaryStableUnitId = UnitId;
		Riga.Sequence = Sequenza;
		Riga.TurnNumber = Turno;
		return Riga;
	}

	FRTActivationLine RigaAttAttivazione(int32 UnitId, const TCHAR* Testo, int32 Squadra = 0,
		ERTMatchPhase Fase = ERTMatchPhase::Prep)
	{
		FRTActivationLine Attivazione;
		Attivazione.SourceStableUnitId = UnitId;
		Attivazione.ActionId = FName(TEXT("Hero.Prova.Azione"));
		Attivazione.Phase = Fase;
		Attivazione.ViewerTeamId = Squadra;
		Attivazione.Text = FText::FromString(Testo);
		return Attivazione;
	}

	/** I testi in ordine, separati da « | »: un confronto solo per l'ordine e per l'insieme. */
	FString RigaAttTesti(const TArray<FRTPlayerEventLineView>& Righe)
	{
		TArray<FString> Testi;
		for (const FRTPlayerEventLineView& Riga : Righe)
		{
			Testi.Add(Riga.Text.ToString());
		}
		return FString::Join(Testi, TEXT(" | "));
	}

	/** I numeri delle righe in ordine, «1,2,3». */
	FString RigaAttNumeri(const TArray<FRTPlayerEventLineView>& Righe)
	{
		TArray<FString> Numeri;
		for (const FRTPlayerEventLineView& Riga : Righe)
		{
			Numeri.Add(FString::FromInt(Riga.Sequence));
		}
		return FString::Join(Numeri, TEXT(","));
	}

	const TCHAR* const RigaAttUnoColpita = TEXT("Colpita — 13");
	const TCHAR* const RigaAttDueCurata = TEXT("Curata — 5");
}

/** Una riga d'attivazione si inserisce SUBITO PRIMA della riga d'esito della stessa unita' (R6). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLineInsertsAboveTest,
	"RefactorTactics.ActivationLine.InsertsAboveTheSameUnitResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLineInsertsAboveTest::RunTest(const FString&)
{
	const TArray<FRTPlayerEventLineView> Esiti = {
		RigaAttEsito(1, RigaAttUnoColpita, 1), RigaAttEsito(2, RigaAttDueCurata, 2) };
	const TArray<FRTActivationLine> Attivazioni = { RigaAttAttivazione(2, TEXT("Kallari — Lama d'ombra")) };

	const TArray<FRTPlayerEventLineView> Righe =
		URTHudViewModel::MergeActivationLines(Esiti, Attivazioni, TArray<int32>{ 0 }, URTHudViewModel::MaxFeedLines);

	TestEqual(TEXT("l'attivazione sta sopra l'esito della sua unita', non in fondo e non sopra tutto"),
		RigaAttTesti(Righe), FString(TEXT("Colpita — 13 | Kallari — Lama d'ombra | Curata — 5")));
	return true;
}

/** La riga d'attivazione porta importanza, unita', fase e il turno del feed; non ha ostacolo e non e' una reazione (R5, R6). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLineRowFieldsTest,
	"RefactorTactics.ActivationLine.RowCarriesItsFieldsAndTheFeedTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLineRowFieldsTest::RunTest(const FString&)
{
	const TArray<FRTPlayerEventLineView> Esiti = { RigaAttEsito(1, RigaAttUnoColpita, 1, /*Turno*/ 5) };
	const TArray<FRTActivationLine> Attivazioni = {
		RigaAttAttivazione(1, TEXT("Kallari — Lama d'ombra"), 0, ERTMatchPhase::Blast) };

	const TArray<FRTPlayerEventLineView> Righe =
		URTHudViewModel::MergeActivationLines(Esiti, Attivazioni, TArray<int32>{ 0 }, URTHudViewModel::MaxFeedLines);
	if (!TestEqual(TEXT("premessa: esito piu' attivazione"), Righe.Num(), 2)) { return false; }

	const FRTPlayerEventLineView& Riga = Righe[0];
	TestEqual(TEXT("il testo e' quello composto"), Riga.Text.ToString(), FString(TEXT("Kallari — Lama d'ombra")));
	TestTrue(TEXT("importanza Important (#1936 §D: uso di abilita')"),
		Riga.Importance == ERTPlayerEventImportance::Important);
	TestEqual(TEXT("l'unita' della riga e' la sorgente"), Riga.PrimaryStableUnitId, 1);
	TestTrue(TEXT("la fase e' quella dell'attivazione"), Riga.Phase == ERTMatchPhase::Blast);
	TestFalse(TEXT("nessun ostacolo da marcare"), Riga.bHasBlocker);
	TestFalse(TEXT("non e' una reazione"), Riga.bReaction);
	// Il widget legge il turno del feed dalla PRIMA riga: una attivazione in testa con turno 0 spegnerebbe l'intestazione.
	TestEqual(TEXT("il turno e' quello delle righe d'esito"), Riga.TurnNumber, 5);
	return true;
}

/** Senza la riga d'esito della sua unita', l'attivazione va in fondo (R6). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLineWithoutResultTest,
	"RefactorTactics.ActivationLine.WithoutAResultGoesLast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLineWithoutResultTest::RunTest(const FString&)
{
	const TArray<FRTPlayerEventLineView> Esiti = {
		RigaAttEsito(1, RigaAttUnoColpita, 1), RigaAttEsito(2, RigaAttDueCurata, 2) };
	const TArray<FRTActivationLine> Attivazioni = { RigaAttAttivazione(3, TEXT("Ivrin — Scarica")) };

	TestEqual(TEXT("U3 non ha una riga d'esito: la sua attivazione chiude l'elenco"),
		RigaAttTesti(URTHudViewModel::MergeActivationLines(
			Esiti, Attivazioni, TArray<int32>{ 0 }, URTHudViewModel::MaxFeedLines)),
		FString(TEXT("Colpita — 13 | Curata — 5 | Ivrin — Scarica")));

	// Nessuna riga d'esito in assoluto: l'attivazione e' l'unica riga, e porta il numero 1.
	const TArray<FRTPlayerEventLineView> Sole = URTHudViewModel::MergeActivationLines(
		TArray<FRTPlayerEventLineView>(), Attivazioni, TArray<int32>{ 0 }, URTHudViewModel::MaxFeedLines);
	TestEqual(TEXT("senza esiti, la sola attivazione"), RigaAttTesti(Sole), FString(TEXT("Ivrin — Scarica")));
	TestEqual(TEXT("e comincia da 1"), RigaAttNumeri(Sole), FString(TEXT("1")));
	return true;
}

/** Piu' attivazioni della stessa unita' stanno tutte sopra la sua riga, nell'ordine della lista (R6). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLineKeepsShowOrderTest,
	"RefactorTactics.ActivationLine.KeepsShowOrderForOneUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLineKeepsShowOrderTest::RunTest(const FString&)
{
	const TArray<FRTPlayerEventLineView> Esiti = {
		RigaAttEsito(1, RigaAttUnoColpita, 1), RigaAttEsito(2, RigaAttDueCurata, 2) };
	// In mezzo, un'attivazione di un'altra unita': l'ordine relativo delle due di U1 non deve dipendere da lei.
	const TArray<FRTActivationLine> Attivazioni = {
		RigaAttAttivazione(1, TEXT("Kallari — Prima")),
		RigaAttAttivazione(2, TEXT("Ivrin — Altra")),
		RigaAttAttivazione(1, TEXT("Kallari — Seconda")) };

	TestEqual(TEXT("le due di U1 sopra U1, nell'ordine in cui sono nate"),
		RigaAttTesti(URTHudViewModel::MergeActivationLines(
			Esiti, Attivazioni, TArray<int32>{ 0 }, URTHudViewModel::MaxFeedLines)),
		FString(TEXT("Kallari — Prima | Kallari — Seconda | Colpita — 13 | Ivrin — Altra | Curata — 5")));
	return true;
}

/** Quando lo spazio non basta escono le attivazioni PIU' VECCHIE, e le righe d'esito restano (R6). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLineBudgetTest,
	"RefactorTactics.ActivationLine.BudgetDropsOldestActivationsFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLineBudgetTest::RunTest(const FString&)
{
	const TArray<FRTPlayerEventLineView> Esiti = {
		RigaAttEsito(1, RigaAttUnoColpita, 1), RigaAttEsito(2, RigaAttDueCurata, 2) };
	const TArray<FRTActivationLine> Attivazioni = {
		RigaAttAttivazione(1, TEXT("Vecchia")),
		RigaAttAttivazione(2, TEXT("Media")),
		RigaAttAttivazione(1, TEXT("Nuova")) };

	// Quattro righe in tutto: due sono gia' esiti, restano due posti per tre attivazioni.
	const TArray<FRTPlayerEventLineView> Righe =
		URTHudViewModel::MergeActivationLines(Esiti, Attivazioni, TArray<int32>{ 0 }, 4);

	TestEqual(TEXT("esce la piu' vecchia; restano le due recenti e i due esiti"),
		RigaAttTesti(Righe), FString(TEXT("Nuova | Colpita — 13 | Media | Curata — 5")));
	TestEqual(TEXT("il totale rispetta il tetto"), Righe.Num(), 4);
	return true;
}

/** Con il tetto gia' pieno di esiti non entra nessuna attivazione, e gli esiti non si toccano (R6). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLineFullBudgetTest,
	"RefactorTactics.ActivationLine.FullBudgetKeepsEveryResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLineFullBudgetTest::RunTest(const FString&)
{
	// Numeri 7 e 8: un feed tagliato da `MaxFeedLines` conserva il numero delle righe che restano, e la fusione non
	// le rinumera se non aggiunge niente.
	const TArray<FRTPlayerEventLineView> Esiti = {
		RigaAttEsito(1, RigaAttUnoColpita, 7), RigaAttEsito(2, RigaAttDueCurata, 8) };
	const TArray<FRTActivationLine> Attivazioni = { RigaAttAttivazione(2, TEXT("Kallari — Lama d'ombra")) };

	const TArray<FRTPlayerEventLineView> Pieno = URTHudViewModel::MergeActivationLines(Esiti, Attivazioni, TArray<int32>{ 0 }, 2);
	TestEqual(TEXT("le due righe d'esito, nient'altro"), RigaAttTesti(Pieno),
		FString(TEXT("Colpita — 13 | Curata — 5")));
	TestEqual(TEXT("e con il loro numero"), RigaAttNumeri(Pieno), FString(TEXT("7,8")));

	// Un tetto PIU' BASSO degli esiti non li taglia: il taglio degli esiti e' di chi li ha costruiti, e lo spazio non
	// diventa mai negativo.
	const TArray<FRTPlayerEventLineView> Stretto = URTHudViewModel::MergeActivationLines(Esiti, Attivazioni, TArray<int32>{ 0 }, 1);
	TestEqual(TEXT("sotto il numero degli esiti, gli esiti restano tutti"), RigaAttTesti(Stretto),
		FString(TEXT("Colpita — 13 | Curata — 5")));
	return true;
}

/** Una riga appartiene alla squadra per cui il playback l'ha mostrata, e solo ai suoi osservatori (R7b). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLineTeamKeyTest,
	"RefactorTactics.ActivationLine.BelongsToTheTeamItWasShownFor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLineTeamKeyTest::RunTest(const FString&)
{
	const TArray<FRTPlayerEventLineView> Esiti = { RigaAttEsito(1, RigaAttUnoColpita, 1) };
	const TArray<FRTActivationLine> Della1 = { RigaAttAttivazione(1, TEXT("Kallari — Lama d'ombra"), 1) };

	TestEqual(TEXT("osservatori {0}: l'attivazione mostrata per la squadra 1 non c'e'"),
		RigaAttTesti(URTHudViewModel::MergeActivationLines(Esiti, Della1, TArray<int32>{ 0 }, URTHudViewModel::MaxFeedLines)),
		FString(TEXT("Colpita — 13")));
	TestEqual(TEXT("osservatori {0,1}: c'e'"),
		RigaAttTesti(URTHudViewModel::MergeActivationLines(Esiti, Della1, TArray<int32>{ 0, 1 }, URTHudViewModel::MaxFeedLines)),
		FString(TEXT("Kallari — Lama d'ombra | Colpita — 13")));
	TestEqual(TEXT("nessun osservatore: fail-closed, niente righe d'attivazione"),
		RigaAttTesti(URTHudViewModel::MergeActivationLines(
			Esiti, Della1, TArray<int32>(), URTHudViewModel::MaxFeedLines)),
		FString(TEXT("Colpita — 13")));

	// Il pubblico si sceglie PRIMA dello spazio: un'attivazione di un'altra squadra non occupa un posto. Un solo posto,
	// due attivazioni: la vecchia e' della squadra 0, la nuova della 1. Contando prima si butterebbe la vecchia e il
	// filtro toglierebbe la nuova: non resterebbe niente.
	const TArray<FRTActivationLine> Miste = {
		RigaAttAttivazione(1, TEXT("Mia"), 0), RigaAttAttivazione(1, TEXT("Altrui"), 1) };
	TestEqual(TEXT("l'attivazione altrui non consuma lo spazio di quella mia"),
		RigaAttTesti(URTHudViewModel::MergeActivationLines(Esiti, Miste, TArray<int32>{ 0 }, 2)),
		FString(TEXT("Mia | Colpita — 13")));
	return true;
}

/** `Sequence` numera la lista unita, con la stessa convenzione delle righe d'esito: la posizione, da 1 (R6). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLineSequenceTest,
	"RefactorTactics.ActivationLine.SequenceCoversTheMergedList",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLineSequenceTest::RunTest(const FString&)
{
	const TArray<FRTPlayerEventLineView> Esiti = {
		RigaAttEsito(1, RigaAttUnoColpita, 1), RigaAttEsito(2, RigaAttDueCurata, 2),
		RigaAttEsito(3, TEXT("Eliminata"), 3) };
	const TArray<FRTActivationLine> Attivazioni = {
		RigaAttAttivazione(2, TEXT("Ivrin — Scarica")),
		RigaAttAttivazione(3, TEXT("Muiren — Scudo")),
		RigaAttAttivazione(9, TEXT("Senza esito")) };

	const TArray<FRTPlayerEventLineView> Righe =
		URTHudViewModel::MergeActivationLines(Esiti, Attivazioni, TArray<int32>{ 0 }, URTHudViewModel::MaxFeedLines);

	if (!TestEqual(TEXT("premessa: l'ordine unito"), RigaAttTesti(Righe),
			FString(TEXT("Colpita — 13 | Ivrin — Scarica | Curata — 5 | Muiren — Scudo | Eliminata | Senza esito"))))
	{
		return false;
	}
	TestEqual(TEXT("il numero e' la posizione nella lista unita, senza buchi e senza doppioni"),
		RigaAttNumeri(Righe), FString(TEXT("1,2,3,4,5,6")));
	return true;
}

/** Il nome dell'azione e' quello del kit; senza un nome leggibile, l'ultimo segmento dell'id cosi' com'e' (R5). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLineTextTest,
	"RefactorTactics.ActivationLine.TextNamesUnitAndAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLineTextTest::RunTest(const FString&)
{
	const FName Id(TEXT("Hero.Kallari.ShadowBlade"));

	TestEqual(TEXT("unita' e nome del kit, col trattino lungo"),
		URTHudViewModel::ComposeActivationLineText(
			TEXT("Kallari"), FText::FromString(TEXT("Lama d'ombra")), Id).ToString(),
		FString(TEXT("Kallari — Lama d'ombra")));

	TestEqual(TEXT("senza nome del kit: l'ultimo segmento dell'id, senza maiuscolo"),
		URTHudViewModel::ComposeActivationLineText(TEXT("Kallari"), FText::GetEmpty(), Id).ToString(),
		FString(TEXT("Kallari — ShadowBlade")));

	TestEqual(TEXT("un nome di soli spazi non e' un nome"),
		URTHudViewModel::ComposeActivationLineText(TEXT("Kallari"), FText::FromString(TEXT("   ")), Id).ToString(),
		FString(TEXT("Kallari — ShadowBlade")));

	TestEqual(TEXT("un id senza punti e' il segmento intero, e non si altera"),
		URTHudViewModel::ComposeActivationLineText(
			TEXT("Kallari"), FText::GetEmpty(), FName(TEXT("shadowBlade"))).ToString(),
		FString(TEXT("Kallari — shadowBlade")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
