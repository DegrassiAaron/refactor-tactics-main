// Cosa conta `FRTOrbitProbe`, e cosa NON conta.
//
// La sonda e' condivisa da due oracoli — `Match.Autobattle.NobodyOscillatesOnTheAuthoredMap` e
// `Match.Autobattle.EngagesOnTheGeneratedTestArena` — e fino a questo file la sua correttezza era provata
// solo **di rimbalzo**: una mutazione del bot faceva cadere il primo, quindi il rilevatore «funzionava».
//
// 🔴 **Quella prova non basta**, e la ragione non e' che il rilevatore sia inerte altrove — misurato il
// 2026-08-28, togliere `*Prev != Cell` da `Observe` fa cadere anche `EngagesOnTheGeneratedTestArena`, quindi
// entrambi gli oracoli lo esercitano. E' che una prova di rimbalzo dice «qualcosa e' cambiato», non **cosa**:
// un rilevatore che contasse la meta' dei ritorni, o che contasse anche il parcheggio, farebbe cadere quegli
// stessi test e nessuno saprebbe distinguere il difetto dalla correzione.
//
// ⚠️ Cio' che resta davvero non dimostrato e' un percorso di COMPORTAMENTO sull'arena generata: nessuna delle
// tre mutazioni del BOT provate quel giorno vi produce un'orbita vera. E' scritto in
// `RTMatchAutobattleTests.cpp`, accanto all'asserzione, e non e' quello che questo file va a coprire.
//
// Qui le sequenze sono SINTETICHE: niente bot, niente mappa, niente turni. Si scrive la storia di celle e
// si controlla il numero. E' l'unico posto in cui «conta le alternanze e non i percorsi» e' un'affermazione
// verificabile invece di una speranza.

#include "Misc/AutomationTest.h"
#include "RTOrbitProbeForTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTOrbitProbeSpec
{
	// Nome distinto da ogni altro file di test: la unity build condivide la translation unit.

	/** Celle su una riga, per leggere le sequenze come lettere invece che come coordinate. */
	FRTCellId C(int32 X)
	{
		return FRTCellId(X, 0, 0);
	}

	/** Alimenta la sonda con una storia di celle per una sola chiave, e restituisce il peggior contatore. */
	int32 ReturnsFor(const TArray<int32>& Storia)
	{
		FRTOrbitProbe Probe;
		for (int32 X : Storia)
		{
			Probe.Observe(/*Key*/ 1, C(X));
		}
		return Probe.WorstReturns();
	}
}

/**
 * L'ALTERNANZA si conta, ed e' il caso per cui la sonda esiste.
 *
 * `A B A B A B` — sei campioni, quattro ritorni: il primo e' osservabile al terzo campione, e da li' ognuno
 * ne aggiunge uno. E' la firma misurata su `L_HexArena` (#1287), dove Riktor alternava fra `(1,-1,L0)` e la
 * piattaforma `(3,-3,L1)`.
 *
 * ⚠️ **Il numero esatto conta, non solo che sia `> 0`.** La soglia degli oracoli e' `TurnsPlayed / 3`:
 * un rilevatore che contasse la meta' o il doppio passerebbe un test «maggiore di zero» e sposterebbe di
 * fatto la soglia di entrambi senza che nessuno lo veda.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOrbitProbeCountsAlternationTest,
	"RefactorTactics.Meta.OrbitProbeCountsTheAlternation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOrbitProbeCountsAlternationTest::RunTest(const FString&)
{
	TestEqual(TEXT("A B A B A B: quattro ritorni di periodo due"),
		RTOrbitProbeSpec::ReturnsFor({0, 1, 0, 1, 0, 1}), 4);

	// La forma minima che produce un ritorno: tre campioni. Con due non e' osservabile, e un oracolo che
	// girasse su due turni non potrebbe cadere — e' la ragione di `MinTurnsToFalsify`.
	TestEqual(TEXT("A B A: un solo ritorno"), RTOrbitProbeSpec::ReturnsFor({0, 1, 0}), 1);
	TestEqual(TEXT("A B: nessuno, non e' ancora osservabile"), RTOrbitProbeSpec::ReturnsFor({0, 1}), 0);

	return true;
}

/**
 * Lo STARE FERMO non si conta, e non e' una svista: e' l'altra meta' della divisione del lavoro.
 *
 * Il punto fisso ha gia' il suo oracolo — la sequenza di turni sulla stessa cella — e contarlo anche qui
 * farebbe scattare due rossi per un difetto solo, con due soglie diverse e nessuna che dice quale ha
 * ragione. `Cell[t] != Cell[t-1]` e' precisamente la condizione che tiene separate le due domande.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOrbitProbeIgnoresStandingStillTest,
	"RefactorTactics.Meta.OrbitProbeIgnoresStandingStill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOrbitProbeIgnoresStandingStillTest::RunTest(const FString&)
{
	TestEqual(TEXT("A A A A A: zero ritorni, il parcheggio non e' un'orbita"),
		RTOrbitProbeSpec::ReturnsFor({0, 0, 0, 0, 0}), 0);

	// E un'unita' che si ferma DOPO essersi mossa non ne produce comunque: `A B B B` non ha alternanza.
	TestEqual(TEXT("A B B B: zero"), RTOrbitProbeSpec::ReturnsFor({0, 1, 1, 1}), 0);

	return true;
}

/**
 * Un PERCORSO non e' un'orbita, e questo e' il test che impedisce alla sonda di diventare severa.
 *
 * ⚠️ **Contare le celle ripetute punirebbe un bot che aggira un ostacolo** — cioe' esattamente il
 * comportamento che #1287 e' andato a comprare, e che #1296 ha reso possibile misurando l'avvicinamento in
 * passi sul grafo. Un rilevatore che segnalasse `A B C A` renderebbe rosso l'aggiramento e spingerebbe a
 * disattivarlo, che e' il modo in cui un oracolo troppo severo finisce per non proteggere piu' niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOrbitProbeIgnoresAPathThatComesBackTest,
	"RefactorTactics.Meta.OrbitProbeIgnoresAPathThatComesBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOrbitProbeIgnoresAPathThatComesBackTest::RunTest(const FString&)
{
	TestEqual(TEXT("A B C D: avanzare non e' orbitare"),
		RTOrbitProbeSpec::ReturnsFor({0, 1, 2, 3}), 0);

	// 🔴 **E' anche il LIMITE DICHIARATO della sonda**, non solo la sua prudenza: `A B C A` e' un'orbita di
	// periodo TRE, e qui vale zero. Un difetto futuro che oscillasse su tre celle non lo vedrebbe nessuno
	// dei due oracoli. Sta scritto sulla sonda, e questo test lo rende una proprieta' misurata invece che
	// una nota: chi estendera' al periodo tre vedra' cadere questa riga, ed e' il punto.
	TestEqual(TEXT("A B C A: periodo tre, NON coperto — e' il limite dichiarato"),
		RTOrbitProbeSpec::ReturnsFor({0, 1, 2, 0}), 0);

	return true;
}

/**
 * Le unita' non si mescolano, e una chiave condivisa FABBRICA un'orbita che non esiste.
 *
 * 🔴 **E' la ragione per cui entrambi gli oracoli asseriscono che gli id siano distinti**, e la nota sulla
 * sonda diceva la cosa sbagliata: sosteneva che con una chiave condivisa l'oscillante «non farebbe crescere
 * nessun contatore». Misurato qui: e' il contrario. Due unita' FERME su celle diverse che scrivono la stessa
 * chiave producono `A B A B ...`, cioe' quattro ritorni su sei campioni — un'oscillazione perfetta in cui
 * nessuno si e' mosso. La guardia non evita un falso NEGATIVO, evita un falso POSITIVO.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOrbitProbeKeepsUnitsApartTest,
	"RefactorTactics.Meta.OrbitProbeKeepsUnitsApart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOrbitProbeKeepsUnitsApartTest::RunTest(const FString&)
{
	// Chiavi distinte: l'oscillante conta, la compagna che avanza no, e il peggiore e' quello dell'oscillante.
	{
		FRTOrbitProbe Probe;
		const TArray<int32> Oscilla = {0, 1, 0, 1};
		const TArray<int32> Avanza  = {10, 11, 12, 13};
		for (int32 T = 0; T < 4; ++T)
		{
			Probe.Observe(/*Key*/ 1, RTOrbitProbeSpec::C(Oscilla[T]));
			Probe.Observe(/*Key*/ 2, RTOrbitProbeSpec::C(Avanza[T]));
		}
		TestEqual(TEXT("con chiavi distinte il peggiore e' l'oscillante: due ritorni"), Probe.WorstReturns(), 2);
	}

	// Chiave condivisa: due unita' FERME, e la sonda vede un'orbita. E' il falso positivo che la guardia
	// sugli `StableUnitId` esiste per impedire — e la prova che quella guardia non e' cerimoniale.
	{
		FRTOrbitProbe Probe;
		for (int32 T = 0; T < 3; ++T)
		{
			Probe.Observe(/*Key*/ 7, RTOrbitProbeSpec::C(0));   // unita' ferma in A
			Probe.Observe(/*Key*/ 7, RTOrbitProbeSpec::C(1));   // unita' ferma in B
		}
		TestTrue(TEXT("con una chiave condivisa due unita' FERME sembrano oscillare"), Probe.WorstReturns() > 0);
	}

	return true;
}

/**
 * La soglia scala coi turni giocati, e la premessa di falsificabilita' e' coerente con essa.
 *
 * ⚠️ **Il difetto che questa forma evita e' aritmetico**, e la code review di #1296 l'ha trovato sull'altro
 * oracolo: con una soglia fissa una partita che finisce presto non puo' produrre abbastanza ritorni per
 * superarla, quindi passa senza misurare niente — e il fix che fa decidere prima la partita rende vacuo il
 * proprio test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOrbitProbeThresholdFollowsTheTurnsTest,
	"RefactorTactics.Meta.OrbitProbeThresholdFollowsTheTurns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOrbitProbeThresholdFollowsTheTurnsTest::RunTest(const FString&)
{
	TestEqual(TEXT("12 turni: soglia 4"), FRTOrbitProbe::LimitForTurns(12), 4);
	TestEqual(TEXT("11 turni: soglia 3"), FRTOrbitProbe::LimitForTurns(11), 3);

	// Mai zero: una soglia a zero renderebbe rosso il primo ritorno, che su una partita cortissima e'
	// rumore e non un difetto.
	TestEqual(TEXT("0 turni: soglia 1, mai zero"), FRTOrbitProbe::LimitForTurns(0), 1);
	TestEqual(TEXT("2 turni: soglia 1"), FRTOrbitProbe::LimitForTurns(2), 1);

	// La premessa deve lasciare spazio alla soglia: con `MinTurnsToFalsify` turni il massimo dei ritorni
	// osservabili e' `turni - 2`, e deve poter SUPERARE il limite, altrimenti l'oracolo non cade mai.
	const int32 Minimo = FRTOrbitProbe::MinTurnsToFalsify;
	const int32 MassimoOsservabile = Minimo - 2;
	TestTrue(FString::Printf(
		TEXT("con %d turni un'oscillazione totale (%d ritorni) supera la soglia (%d)"),
		Minimo, MassimoOsservabile, FRTOrbitProbe::LimitForTurns(Minimo)),
		MassimoOsservabile > FRTOrbitProbe::LimitForTurns(Minimo));

	// 🔴 **E la MINIMALITA', che e' la meta' mancata dalla prima stesura.** Verificare solo che la premessa
	// funzioni AL valore scelto lascia passare qualunque numero piu' grande — ed e' cosi' che il 6 iniziale,
	// aritmeticamente sbagliato, e' stato pinnato senza che nessuno se ne accorgesse. Un minimo troppo alto
	// non e' prudenza: fa andare rosso il chiamante che ASSERISCE la premessa quando la partita finisce
	// prima, cioe' punisce il bot che decide in fretta.
	const int32 SottoIlMinimo = Minimo - 1;
	TestFalse(FString::Printf(
		TEXT("a %d turni la soglia NON e' esercitabile (%d ritorni al massimo contro limite %d): %d e' minimale"),
		SottoIlMinimo, SottoIlMinimo - 2, FRTOrbitProbe::LimitForTurns(SottoIlMinimo), Minimo),
		(SottoIlMinimo - 2) > FRTOrbitProbe::LimitForTurns(SottoIlMinimo));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
