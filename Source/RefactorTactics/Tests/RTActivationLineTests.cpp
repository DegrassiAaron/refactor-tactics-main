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

	/** Una riga d'esito con la sua fase (#4052): senza, vale `Planning`, che viene prima di ogni fase del playback. */
	FRTPlayerEventLineView RigaAttEsitoInFase(int32 UnitId, const TCHAR* Testo, int32 Sequenza, ERTMatchPhase Fase)
	{
		FRTPlayerEventLineView Riga = RigaAttEsito(UnitId, Testo, Sequenza);
		Riga.Phase = Fase;
		return Riga;
	}

	FRTActivationLine RigaAttAttivazione(int32 UnitId, const TCHAR* Testo, int32 Squadra = 0,
		ERTMatchPhase Fase = ERTMatchPhase::Prep, int32 Turno = 0)
	{
		FRTActivationLine Attivazione;
		Attivazione.SourceStableUnitId = UnitId;
		Attivazione.ActionId = FName(TEXT("Hero.Prova.Azione"));
		Attivazione.Phase = Fase;
		Attivazione.ViewerTeamId = Squadra;
		Attivazione.Text = FText::FromString(Testo);
		Attivazione.TurnNumber = Turno;
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

/**
 * La riga d'attivazione porta importanza, unita', fase e il PROPRIO turno; non ha ostacolo e non e' una reazione (R5, R6).
 * ⚠️ Qui il turno della riga e quello degli esiti coincidono, come in partita (stessa vita, R7): da dove la fusione lo prenda
 * lo pinna `OnlyActivationsCarryTheirTurn`, dove un esito da cui copiarlo non c'e'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLineRowFieldsTest,
	"RefactorTactics.ActivationLine.RowCarriesItsFieldsAndItsTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLineRowFieldsTest::RunTest(const FString&)
{
	const TArray<FRTPlayerEventLineView> Esiti = { RigaAttEsito(1, RigaAttUnoColpita, 1, /*Turno*/ 5) };
	const TArray<FRTActivationLine> Attivazioni = {
		RigaAttAttivazione(1, TEXT("Kallari — Lama d'ombra"), 0, ERTMatchPhase::Blast, /*Turno*/ 5) };

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
	TestEqual(TEXT("il turno e' quello della riga d'attivazione"), Riga.TurnNumber, 5);
	return true;
}

/**
 * Un feed di SOLE attivazioni porta il loro turno (#4033, Ruling sul `TurnNumber`): il widget legge l'intestazione da
 * `Feed[0].TurnNumber`, e un turno in cui nessuna riga d'esito passa (soli movimenti, che il proiettore scarta) non deve
 * dire «turno 0». Copiarlo dalla prima riga d'esito, che qui non c'e', darebbe 0.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLineOnlyActivationsTurnTest,
	"RefactorTactics.ActivationLine.OnlyActivationsCarryTheirTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLineOnlyActivationsTurnTest::RunTest(const FString&)
{
	const TArray<FRTActivationLine> Attivazioni = {
		RigaAttAttivazione(1, TEXT("Kallari — Lama d'ombra"), 0, ERTMatchPhase::Prep, /*Turno*/ 7),
		RigaAttAttivazione(2, TEXT("Ivrin — Scarica"), 0, ERTMatchPhase::Blast, /*Turno*/ 7) };

	const TArray<FRTPlayerEventLineView> Righe = URTHudViewModel::MergeActivationLines(
		TArray<FRTPlayerEventLineView>(), Attivazioni, TArray<int32>{ 0 }, URTHudViewModel::MaxFeedLines);
	if (!TestEqual(TEXT("premessa: le due attivazioni, nient'altro"), Righe.Num(), 2)) { return false; }

	TestEqual(TEXT("🔴 la prima riga, da cui il widget legge l'intestazione, porta il turno 7"), Righe[0].TurnNumber, 7);
	TestEqual(TEXT("e anche la seconda"), Righe[1].TurnNumber, 7);
	return true;
}

/**
 * Piu' righe d'esito della STESSA unita': l'attivazione compare una volta sola, sopra la PRIMA (R6). Il proiettore ne tiene
 * di regola una per unita' (dominanza), ma la fusione non lo presume.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLineSeveralResultsTest,
	"RefactorTactics.ActivationLine.SeveralResultsOfOneUnitGetItOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLineSeveralResultsTest::RunTest(const FString&)
{
	const TArray<FRTPlayerEventLineView> Esiti = {
		RigaAttEsito(1, RigaAttUnoColpita, 1), RigaAttEsito(2, RigaAttDueCurata, 2), RigaAttEsito(1, TEXT("Eliminata"), 3) };
	const TArray<FRTActivationLine> Attivazioni = { RigaAttAttivazione(1, TEXT("Kallari — Lama d'ombra")) };

	TestEqual(TEXT("una volta, sopra la prima riga di U1, e non anche sopra la seconda"),
		RigaAttTesti(URTHudViewModel::MergeActivationLines(
			Esiti, Attivazioni, TArray<int32>{ 0 }, URTHudViewModel::MaxFeedLines)),
		FString(TEXT("Kallari — Lama d'ombra | Colpita — 13 | Curata — 5 | Eliminata")));
	return true;
}

/**
 * Un'attivazione senza sorgente nota (`INDEX_NONE`) accanto a una riga di mondo (`PrimaryStableUnitId == INDEX_NONE`) non
 * le si aggancia: «nessuna unita'» non e' un'unita'. Senza la riga d'esito della sua unita' va in fondo alla sua fase
 * ([D-603] (2), #4052). ⛔ Agganciata starebbe SOPRA la riga di mondo; con R6 com'era prima di D-603, in fondo all'elenco,
 * sotto la BLAST. A runtime non nasce (`AddActivationLine` scarta una sorgente nulla): e' il caso limite della funzione pura.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLineWorldSourceTest,
	"RefactorTactics.ActivationLine.WorldSourceIsNotAttachedToAWorldRow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLineWorldSourceTest::RunTest(const FString&)
{
	const TArray<FRTPlayerEventLineView> Esiti = {
		RigaAttEsitoInFase(INDEX_NONE, TEXT("Ambiente — 2"), 1, ERTMatchPhase::Prep),
		RigaAttEsitoInFase(1, RigaAttUnoColpita, 2, ERTMatchPhase::Blast) };
	const TArray<FRTActivationLine> Attivazioni = { RigaAttAttivazione(INDEX_NONE, TEXT("Senza sorgente")) }; // Prep

	TestEqual(TEXT("dopo la riga di mondo e non sopra, e prima della BLAST"),
		RigaAttTesti(URTHudViewModel::MergeActivationLines(
			Esiti, Attivazioni, TArray<int32>{ 0 }, URTHudViewModel::MaxFeedLines)),
		FString(TEXT("Ambiente — 2 | Senza sorgente | Colpita — 13")));
	return true;
}

/**
 * Senza la riga d'esito della sua unita', l'attivazione chiude la sua fase ([D-603] (2), #4052). Qui righe d'esito e
 * attivazione sono tutte della Prep, quindi chiude l'elenco. Senza nessuna riga d'esito e' l'unica riga, e porta il numero 1.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLineWithoutResultTest,
	"RefactorTactics.ActivationLine.WithoutAResultClosesItsPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLineWithoutResultTest::RunTest(const FString&)
{
	const TArray<FRTPlayerEventLineView> Esiti = {
		RigaAttEsitoInFase(1, RigaAttUnoColpita, 1, ERTMatchPhase::Prep),
		RigaAttEsitoInFase(2, RigaAttDueCurata, 2, ERTMatchPhase::Prep) };
	const TArray<FRTActivationLine> Attivazioni = { RigaAttAttivazione(3, TEXT("Ivrin — Scarica")) }; // Prep

	TestEqual(TEXT("U3 non ha una riga d'esito: la sua attivazione chiude la Prep, l'ultima fase dell'elenco"),
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

/**
 * 🔴 **IL CASO DELLA SEDUTA DI #4051** ([D-603] (2), #4052). La riga del gadget, la cui unita' non ha una riga d'esito, sta
 * in fondo alle PREP e non sotto la BLAST; l'attivazione BLAST di Aevik resta subito sopra la sua riga d'esito (R6, che
 * D-603 non tocca). E' la lista dell'anteprima che l'autore ha scelto. ⛔ Prima di #4052 «Branth — Copertura portatile»
 * chiudeva l'elenco. Le attivazioni sono nell'ordine in cui il playback le ha mostrate nella seduta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLineBeforeNextPhaseTest,
	"RefactorTactics.ActivationLine.WithoutAResultGoesBeforeTheNextPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLineBeforeNextPhaseTest::RunTest(const FString&)
{
	const TArray<FRTPlayerEventLineView> Esiti = {
		RigaAttEsitoInFase(INDEX_NONE, TEXT("L'ambiente cambia"), 1, ERTMatchPhase::Prep),
		RigaAttEsitoInFase(2, TEXT("Stato cambiato"), 2, ERTMatchPhase::Prep),
		RigaAttEsitoInFase(3, TEXT("Colpita — 170"), 3, ERTMatchPhase::Blast) };
	const TArray<FRTActivationLine> Attivazioni = {
		RigaAttAttivazione(4, TEXT("Branth — Copertura portatile")), // Prep, senza riga d'esito
		RigaAttAttivazione(2, TEXT("Muiren — Irrigidimento")),       // Prep, sopra «Stato cambiato»
		RigaAttAttivazione(3, TEXT("Aevik — Impulso ad arco"), 0, ERTMatchPhase::Blast) };

	const TArray<FRTPlayerEventLineView> Unite = URTHudViewModel::MergeActivationLines(
		Esiti, Attivazioni, TArray<int32>{ 0 }, URTHudViewModel::MaxFeedLines);
	TestEqual(TEXT("🔴 l'ordine dell'anteprima scelta in D-603: il gadget chiude le PREP, prima delle BLAST"),
		RigaAttTesti(Unite),
		FString(TEXT("L'ambiente cambia | Muiren — Irrigidimento | Stato cambiato | Branth — Copertura portatile | "
			"Aevik — Impulso ad arco | Colpita — 170")));
	TestEqual(TEXT("e i numeri seguono la lista unita"), RigaAttNumeri(Unite), FString(TEXT("1,2,3,4,5,6")));
	return true;
}

/**
 * 🔴 **«IN FONDO ALLA SUA FASE» QUANDO LE RIGHE D'ESITO NON SONO IN ORDINE DI FASE** ([D-603] (3) e le letture (4)(b) e
 * (4)(c), #4052). Una riga d'esito tiene il posto del primo evento della sua unita' e porta la fase della voce che la domina
 * (`URTPlayerEventProjector::Project`, «Sostituisce sul posto»): una BLAST puo' stare fra le PREP. L'attivazione va subito
 * dopo l'ultima riga d'esito la cui fase non viene dopo la sua, e in cima se non ce n'e' nessuna; due attivazioni nello
 * stesso posto restano nell'ordine in cui sono nate.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLineOutOfPhaseOrderTest,
	"RefactorTactics.ActivationLine.WithoutAResultFollowsTheLastRowNotAfterItsPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLineOutOfPhaseOrderTest::RunTest(const FString&)
{
	const auto Unisci = [](const TArray<FRTPlayerEventLineView>& Esiti, const TArray<FRTActivationLine>& Attivazioni)
	{
		return RigaAttTesti(URTHudViewModel::MergeActivationLines(
			Esiti, Attivazioni, TArray<int32>{ 0 }, URTHudViewModel::MaxFeedLines));
	};

	// 1. Fuori ordine: la BLAST di U1 sta prima della PREP di U2. Due attivazioni PREP senza esito, nate in quest'ordine.
	TestEqual(TEXT("🔴 dopo l'ultima PREP, non dopo la prima BLAST, e nell'ordine in cui sono nate"),
		Unisci(
			{ RigaAttEsitoInFase(1, RigaAttUnoColpita, 1, ERTMatchPhase::Blast),
			  RigaAttEsitoInFase(2, TEXT("Stato cambiato"), 2, ERTMatchPhase::Prep),
			  RigaAttEsitoInFase(3, TEXT("Colpita — 9"), 3, ERTMatchPhase::Blast) },
			{ RigaAttAttivazione(4, TEXT("Branth — Copertura portatile")),
			  RigaAttAttivazione(5, TEXT("Kallari — Scatto")) }),
		FString(TEXT("Colpita — 13 | Stato cambiato | Branth — Copertura portatile | Kallari — Scatto | Colpita — 9")));

	// 2. In cima: nessuna riga d'esito di una fase che non venga dopo la Prep.
	TestEqual(TEXT("🔴 senza righe d'esito della Prep o di prima, l'attivazione PREP apre l'elenco"),
		Unisci(
			{ RigaAttEsitoInFase(1, RigaAttUnoColpita, 1, ERTMatchPhase::Blast),
			  RigaAttEsitoInFase(2, RigaAttDueCurata, 2, ERTMatchPhase::Blast) },
			{ RigaAttAttivazione(4, TEXT("Branth — Copertura portatile")) }),
		FString(TEXT("Branth — Copertura portatile | Colpita — 13 | Curata — 5")));

	// 3. Due fasi senza esito: la PREP resta sopra la BLAST d'esito, la BLAST chiude l'elenco.
	TestEqual(TEXT("ognuna in fondo alla sua fase"),
		Unisci(
			{ RigaAttEsitoInFase(1, TEXT("Stato cambiato"), 1, ERTMatchPhase::Prep),
			  RigaAttEsitoInFase(2, RigaAttUnoColpita, 2, ERTMatchPhase::Blast) },
			{ RigaAttAttivazione(4, TEXT("Branth — Copertura portatile")),
			  RigaAttAttivazione(5, TEXT("Ivrin — Scarica"), 0, ERTMatchPhase::Blast) }),
		FString(TEXT("Stato cambiato | Branth — Copertura portatile | Colpita — 13 | Ivrin — Scarica")));

	// 4. Il tetto toglie le attivazioni piu' vecchie PRIMA dell'ordine: con due posti liberi restano le due piu' giovani,
	// ciascuna in fondo alla sua fase.
	TestEqual(TEXT("col tetto escono le piu' vecchie, e le rimaste vanno ognuna in fondo alla sua fase"),
		RigaAttTesti(URTHudViewModel::MergeActivationLines(
			{ RigaAttEsitoInFase(1, TEXT("Stato cambiato"), 1, ERTMatchPhase::Prep),
			  RigaAttEsitoInFase(2, RigaAttUnoColpita, 2, ERTMatchPhase::Blast) },
			{ RigaAttAttivazione(4, TEXT("Branth — Copertura portatile")),
			  RigaAttAttivazione(5, TEXT("Kallari — Scatto")),
			  RigaAttAttivazione(6, TEXT("Ivrin — Scarica"), 0, ERTMatchPhase::Blast) },
			TArray<int32>{ 0 }, /*MaxLines=*/ 4)),
		FString(TEXT("Stato cambiato | Kallari — Scatto | Colpita — 13 | Ivrin — Scarica")));
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
