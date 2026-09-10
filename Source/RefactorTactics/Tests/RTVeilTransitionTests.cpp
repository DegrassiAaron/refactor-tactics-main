#include "Misc/AutomationTest.h"

#include "Perception/RTVeilTransition.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * `#2874` — **il filtro temporale della presentazione del velo**, misurato senza mondo e senza board.
 *
 * ⚠️ **Nessuno di questi test monta un `UWorld`, e non e' una comodita': e' la garanzia.** Se un giorno
 * `URTVeilTransitionLibrary` avesse bisogno di un Actor, di una mappa o della conoscenza per rispondere,
 * questi test non compilerebbero — ed e' l'unico modo automatico di accorgersi che il filtro ha smesso di
 * essere presentazione pura ed e' diventato una seconda autorita'.
 */

namespace
{
	/** I parametri di default, nominati una volta: ogni test che li cambia lo fa in modo visibile. */
	FRTVeilTransitionParams Default()
	{
		return FRTVeilTransitionParams();
	}

	/**
	 * Quanti passi da `Dt` servono perche' `Current` raggiunga `Target` **esattamente**, o `Limite` se non
	 * ci arriva. Restituire il limite invece di ciclare per sempre e' cio' che rende il test una misura:
	 * «non converge» diventa un numero, non un blocco della suite.
	 */
	int32 PassiPerConvergere(float Current, float Target, float Dt, const FRTVeilTransitionParams& P,
		int32 Limite, float& OutFinale)
	{
		for (int32 I = 0; I < Limite; ++I)
		{
			Current = URTVeilTransitionLibrary::Advance(Current, Target, Dt, P);
			if (Current == Target)
			{
				OutFinale = Current;
				return I + 1;
			}
		}
		OutFinale = Current;
		return Limite;
	}
}

/**
 * La transizione ARRIVA, in entrambi i versi, e ci arriva con un'uguaglianza e non con una distanza.
 *
 * 🔴 **E' il test che rende vero il vincolo «a fine transizione lo stato grafico coincide col target».** Un
 * esponenziale non tocca mai l'asintoto: senza lo snap di `SnapEpsilon` questo test girerebbe fino al limite
 * e fallirebbe, e la cella resterebbe in transizione per sempre pagando una riscrittura per frame.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilTransitionConvergesExactlyTest,
	"RefactorTactics.VeilTransition.ConvergesExactlyToBothEnds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilTransitionConvergesExactlyTest::RunTest(const FString&)
{
	const FRTVeilTransitionParams P = Default();
	const float Dt = 1.f / 60.f;
	const int32 Limite = 600; // dieci secondi a 60 fps: molto oltre entrambe le costanti

	// ⚠️ **`TestTrue` con `==` e non `TestEqual`, e la differenza e' il punto di tutto il test.**
	// `TestEqual(float, float)` ha una `Tolerance` che vale `KINDA_SMALL_NUMBER` per default: passerebbe a
	// `0.99999`, cioe' proprio nel caso che questo test esiste per escludere.
	float FinaleSu = 0.f;
	const int32 PassiSu = PassiPerConvergere(/*Current=*/ 0.f, /*Target=*/ 1.f, Dt, P, Limite, FinaleSu);
	TestTrue(*FString::Printf(TEXT("l'apertura arriva al target ESATTO (%.9f)"), FinaleSu), FinaleSu == 1.f);
	TestTrue(TEXT("l'apertura converge entro il limite"), PassiSu < Limite);

	float FinaleGiu = 0.f;
	const int32 PassiGiu = PassiPerConvergere(/*Current=*/ 1.f, /*Target=*/ 0.f, Dt, P, Limite, FinaleGiu);
	TestTrue(*FString::Printf(TEXT("la chiusura arriva al target ESATTO (%.9f)"), FinaleGiu), FinaleGiu == 0.f);
	TestTrue(TEXT("la chiusura converge entro il limite"), PassiGiu < Limite);

	// 🔑 **La chiusura deve metterci di PIU'.** Se i due numeri fossero uguali le due costanti non
	// starebbero facendo niente, e il test sopra passerebbe lo stesso: e' la guardia anti-vacuita' di questo.
	TestTrue(*FString::Printf(TEXT("la chiusura e' piu' lenta dell'apertura (%d passi contro %d)"),
			PassiGiu, PassiSu),
		PassiGiu > PassiSu);

	AddInfo(FString::Printf(TEXT("convergenza a 60 fps: apertura %d passi (%.0f ms), chiusura %d passi (%.0f ms)"),
		PassiSu, PassiSu * Dt * 1000.f, PassiGiu, PassiGiu * Dt * 1000.f));
	return true;
}

/**
 * 🔑 **L'indipendenza dal frame rate, misurata come IDENTITA' e non come «abbastanza simile».**
 *
 * `Advance` e' `Target + (Current - Target) * exp(-Dt/Tau)`: due passi consecutivi moltiplicano i due
 * esponenziali, e `exp(-a/T) * exp(-b/T) = exp(-(a+b)/T)`. ∴ un passo lungo e molti corti devono dare lo
 * **stesso numero**, a meno dell'arrotondamento in virgola mobile.
 *
 * ⚠️ **Anti-vacuita' in due punti**, perche' senza il test passerebbe per ragioni sbagliate: si verifica
 * che il valore NON sia gia' al target (altrimenti confronterebbe due volte `1.0`) e che si sia mosso dallo
 * zero di partenza.
 *
 * 🔴 **E' il test che un `Lerp` a coefficiente fisso non puo' passare.** Con `Lerp(C, T, 0.2f)` per passo,
 * un frame lungo e sessanta corti danno velocita' visuali completamente diverse — che e' il difetto che
 * questa forma esiste per non avere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilTransitionStepSizeTest,
	"RefactorTactics.VeilTransition.IsIndependentOfStepSize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilTransitionStepSizeTest::RunTest(const FString&)
{
	const FRTVeilTransitionParams P = Default();

	// 60 ms = mezza costante di apertura: lontano dal target e lontano dalla soglia di snap, quindi il
	// confronto misura il filtro e non il ramo che lo chiude.
	const float Totale = 0.06f;
	const int32 N = 60;

	const float UnPasso = URTVeilTransitionLibrary::Advance(0.f, 1.f, Totale, P);

	float MoltiPassi = 0.f;
	for (int32 I = 0; I < N; ++I)
	{
		MoltiPassi = URTVeilTransitionLibrary::Advance(MoltiPassi, 1.f, Totale / N, P);
	}

	TestTrue(TEXT("il valore si e' mosso dallo zero di partenza"), UnPasso > 0.01f);
	TestTrue(TEXT("e non e' gia' al target, altrimenti il confronto sarebbe vacuo"), UnPasso < 0.99f);

	const float Scarto = FMath::Abs(UnPasso - MoltiPassi);
	TestTrue(*FString::Printf(TEXT("un passo da %.0f ms e %d da %.2f ms coincidono (scarto %.3e)"),
			Totale * 1000.f, N, Totale / N * 1000.f, Scarto),
		Scarto < 1.e-4f);

	AddInfo(FString::Printf(TEXT("un passo: %.6f · %d passi: %.6f · scarto %.3e"),
		UnPasso, N, MoltiPassi, Scarto));
	return true;
}

/**
 * Il VERSO sceglie la costante: a parita' di tempo trascorso, l'apertura copre piu' strada della chiusura.
 *
 * ⚠️ Il confronto e' sulla **distanza percorsa**, non sul valore assoluto: i due partono da estremi opposti,
 * e confrontare `0.57` con `0.72` direbbe il contrario di quel che si intende.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilTransitionRevealFasterTest,
	"RefactorTactics.VeilTransition.RevealIsFasterThanHide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilTransitionRevealFasterTest::RunTest(const FString&)
{
	const FRTVeilTransitionParams P = Default();
	const float Dt = 0.1f;

	const float DopoApertura = URTVeilTransitionLibrary::Advance(/*Current=*/ 0.f, /*Target=*/ 1.f, Dt, P);
	const float DopoChiusura = URTVeilTransitionLibrary::Advance(/*Current=*/ 1.f, /*Target=*/ 0.f, Dt, P);

	const float StradaApertura = DopoApertura - 0.f;
	const float StradaChiusura = 1.f - DopoChiusura;

	TestTrue(*FString::Printf(TEXT("in %.0f ms l'apertura copre piu' strada (%.3f contro %.3f)"),
			Dt * 1000.f, StradaApertura, StradaChiusura),
		StradaApertura > StradaChiusura);

	// Le due costanti dichiarate restano dentro le bande del prototipo: 80-150 ms e 250-400 ms.
	TestTrue(TEXT("la costante di apertura resta nella banda 80-150 ms"),
		P.RevealSeconds >= 0.080f && P.RevealSeconds <= 0.150f);
	TestTrue(TEXT("la costante di chiusura resta nella banda 250-400 ms"),
		P.HideSeconds >= 0.250f && P.HideSeconds <= 0.400f);
	return true;
}

/**
 * 🔴 **Un target nuovo a meta' strada RIPRENDE, non ricomincia.**
 *
 * E' il caso del movimento interrotto, di quello concatenato e della sequenza rapida di celle: tre scenari
 * che un filtro con «durata e progresso» avrebbe dovuto trattare uno per uno, e che qui sono lo stesso caso
 * perche' non c'e' nessuno stato di transizione — c'e' solo il valore corrente.
 *
 * ⚠️ **Il difetto che questo test prende ha una forma precisa**: un'implementazione che ricominciasse
 * dall'estremo farebbe partire il primo passo dopo il retarget da `1.0`, quindi il valore **salirebbe** da
 * `0.39` a `0.97` prima di scendere. La continuita' e' cio' che lo esclude.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilTransitionRetargetTest,
	"RefactorTactics.VeilTransition.RetargetResumesInsteadOfRestarting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilTransitionRetargetTest::RunTest(const FString&)
{
	const FRTVeilTransitionParams P = Default();

	// A meta' apertura, lontano da entrambi gli estremi: e' li' che «riprende» e «ricomincia» si distinguono.
	const float AMeta = URTVeilTransitionLibrary::Advance(0.f, 1.f, 0.06f, P);
	TestTrue(TEXT("il valore di partenza e' davvero a meta' strada"), AMeta > 0.2f && AMeta < 0.8f);

	// Target rovesciato, un passo breve.
	const float DopoRetarget = URTVeilTransitionLibrary::Advance(AMeta, 0.f, 1.f / 60.f, P);

	TestTrue(*FString::Printf(TEXT("il valore SCENDE verso il nuovo target (%.4f -> %.4f)"), AMeta, DopoRetarget),
		DopoRetarget < AMeta);
	TestTrue(TEXT("e non salta al nuovo target in un passo"), DopoRetarget > 0.f);
	TestTrue(*FString::Printf(TEXT("la transizione e' CONTINUA: nessun salto all'estremo (delta %.4f)"),
			FMath::Abs(DopoRetarget - AMeta)),
		FMath::Abs(DopoRetarget - AMeta) < 0.05f);
	return true;
}

/**
 * La PAUSA: un passo nullo o negativo lascia il valore identico, e ripeterlo non lo muove di un bit.
 *
 * 🔑 **E' il vincolo «pausa durante la dissolvenza» del prototipo**, e vive qui invece che nel consumatore:
 * un playback in pausa pompa il proprio passo a zero, e il velo deve fermarsi a meta' invece di concludere.
 *
 * ⚠️ **Il NaN cade nello stesso ramo**, ed e' deliberato: propagato nel valore disegnato spegnerebbe una
 * cella senza che nessun log dicesse perche'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilTransitionPauseTest,
	"RefactorTactics.VeilTransition.PausedStepDoesNotMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilTransitionPauseTest::RunTest(const FString&)
{
	const FRTVeilTransitionParams P = Default();
	const float AMeta = URTVeilTransitionLibrary::Advance(0.f, 1.f, 0.06f, P);

	float Fermo = AMeta;
	for (int32 I = 0; I < 100; ++I)
	{
		Fermo = URTVeilTransitionLibrary::Advance(Fermo, 1.f, 0.f, P);
	}
	// Uguaglianza ESATTA, non `TestEqual` con la sua tolleranza: «non si muove» significa nemmeno di un bit.
	TestTrue(TEXT("cento passi da zero secondi non muovono il valore"), Fermo == AMeta);
	TestTrue(TEXT("un passo negativo nemmeno"),
		URTVeilTransitionLibrary::Advance(AMeta, 1.f, -0.5f, P) == AMeta);

	// ⚠️ Il NaN si costruisce e **si verifica di averlo**: con certi modelli in virgola mobile la radice di
	// un negativo non lo produce, e un'asserzione scritta come se lo producesse sempre sarebbe verde per la
	// ragione sbagliata. Se il NaN non c'e', lo si dice invece di tacerlo.
	const float NonNumero = FMath::Sqrt(-1.f);
	if (FMath::IsNaN(NonNumero))
	{
		TestTrue(TEXT("un passo NaN cade nel ramo della pausa invece di propagarsi"),
			URTVeilTransitionLibrary::Advance(AMeta, 1.f, NonNumero, P) == AMeta);
	}
	else
	{
		AddInfo(TEXT("NaN non prodotto su questa piattaforma: il ramo non e' stato misurato qui"));
	}
	return true;
}

/**
 * Il filtro SPENTO riproduce il salto: e' il ramo di confronto che `#2875` usera' per dimostrare che, a
 * costante nulla, la board e' identica a quella di prima del filtro.
 *
 * 🔑 **Serve che sia NOMINATO.** Un ramo che si prova solo cancellando il codice non si prova in una suite.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilTransitionInstantTest,
	"RefactorTactics.VeilTransition.InstantParamsReproduceTheJump",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilTransitionInstantTest::RunTest(const FString&)
{
	const FRTVeilTransitionParams Istantaneo = FRTVeilTransitionParams::Instant();

	TestTrue(TEXT("un passo apre del tutto"),
		URTVeilTransitionLibrary::Advance(0.f, 1.f, 1.f / 60.f, Istantaneo) == 1.f);
	TestTrue(TEXT("un passo chiude del tutto, anche su un target intermedio"),
		URTVeilTransitionLibrary::Advance(1.f, 0.35f, 1.f / 60.f, Istantaneo) == 0.35f);

	// ⚠️ La pausa vince comunque sul filtro spento: in pausa non si salta, si sta fermi. Sono due domande
	// diverse — «quanto tempo e' passato?» e «quanto e' lento il filtro?» — e la prima risponde per prima.
	TestTrue(TEXT("ma in pausa non salta nemmeno il filtro spento"),
		URTVeilTransitionLibrary::Advance(0.f, 1.f, 0.f, Istantaneo) == 0.f);
	return true;
}

/**
 * Una sequenza rapida di target alternati non fa uscire il valore dall'intervallo dei target.
 *
 * E' la sagoma dell'unita' che oscilla sul bordo del cono, e del movimento concatenato che cambia idea
 * prima che la dissolvenza precedente sia finita.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilTransitionAlternatingTest,
	"RefactorTactics.VeilTransition.AlternatingTargetsStayInRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilTransitionAlternatingTest::RunTest(const FString&)
{
	const FRTVeilTransitionParams P = Default();
	float V = 0.f;
	float Minimo = 1.f;
	float Massimo = 0.f;

	for (int32 I = 0; I < 300; ++I)
	{
		const float Target = (I % 7 < 3) ? 1.f : 0.f; // periodo dispari: le fasi non si allineano al passo
		V = URTVeilTransitionLibrary::Advance(V, Target, 1.f / 60.f, P);
		Minimo = FMath::Min(Minimo, V);
		Massimo = FMath::Max(Massimo, V);
	}

	TestTrue(*FString::Printf(TEXT("il valore resta in [0, 1] (min %.4f, max %.4f)"), Minimo, Massimo),
		Minimo >= 0.f && Massimo <= 1.f);
	// Anti-vacuita': se il valore non si fosse mai mosso, l'intervallo sarebbe rispettato per niente.
	TestTrue(TEXT("e si e' davvero mosso"), Massimo - Minimo > 0.1f);
	return true;
}

/**
 * Il passo su un'intera board, e il conteggio che dice **quando smettere**.
 *
 * 🔑 **Lo zero e' il segnale con cui un consumatore puo' spegnere il proprio aggiornamento.** Senza,
 * «il velo sta lavorando» e «il velo ha finito e continua a pagarne il costo» sarebbero indistinguibili.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilTransitionAdvanceAllTest,
	"RefactorTactics.VeilTransition.AdvanceAllReportsCellsStillMoving",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilTransitionAdvanceAllTest::RunTest(const FString&)
{
	const FRTVeilTransitionParams P = Default();

	// Tre celle: una che si apre, una gia' a posto, una che si chiude.
	TArray<float> Display = { 0.f, 0.35f, 1.f };
	const TArray<float> Targets = { 1.f, 0.35f, 0.f };

	const int32 PrimoPasso = URTVeilTransitionLibrary::AdvanceAll(Display, Targets, 1.f / 60.f, P);
	TestEqual(TEXT("al primo passo si muovono le due celle che hanno un target diverso"), PrimoPasso, 2);
	TestTrue(TEXT("la cella gia' al target non viene toccata"), Display[1] == 0.35f);

	int32 Passi = 0;
	int32 Vive = PrimoPasso;
	while (Vive > 0 && Passi < 600)
	{
		Vive = URTVeilTransitionLibrary::AdvanceAll(Display, Targets, 1.f / 60.f, P);
		++Passi;
	}
	TestEqual(TEXT("alla fine nessuna cella e' in movimento"), Vive, 0);
	for (int32 I = 0; I < Display.Num(); ++I)
	{
		TestTrue(*FString::Printf(TEXT("la cella %d e' al proprio target ESATTO (%.9f contro %.9f)"),
				I, Display[I], Targets[I]),
			Display[I] == Targets[I]);
	}
	AddInfo(FString::Printf(TEXT("board di %d celle converse in %d passi"), Display.Num(), Passi + 1));

	// 🔴 Il disallineamento risponde `INDEX_NONE` e NON tocca niente: `0` direbbe «tutto converso» e
	// spegnerebbe l'aggiornamento proprio a chi ha l'ingresso rotto.
	TArray<float> Corta = { 0.f, 0.f };
	const TArray<float> Copia = Corta;
	const int32 Esito = URTVeilTransitionLibrary::AdvanceAll(Corta, Targets, 1.f / 60.f, P);
	TestEqual(TEXT("lunghezze diverse: INDEX_NONE, non zero"), Esito, INDEX_NONE);
	TestTrue(TEXT("e i valori restano intatti"), Corta == Copia);
	return true;
}

/**
 * ⛔ **Il filtro non conosce la conoscenza, e questo test lo mette per iscritto.**
 *
 * Non c'e' un modo automatico di asserire «questo header non include quell'altro» da dentro un test; cio'
 * che si puo' asserire e' che il filtro risponde **senza mondo**, il che e' anche la ragione per cui questo
 * file non monta un `UWorld`. La guardia vera e' il compilatore: se `RTVeilTransition.h` acquisisse una
 * dipendenza da `FRTTeamKnowledge` o da un Actor, questo file smetterebbe di compilare cosi' com'e'.
 *
 * ⚠️ Resta quindi un **tripwire dichiarato**, non un controllo vivo — la stessa onesta' con cui il progetto
 * registra gli altri.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilTransitionIsPureTest,
	"RefactorTactics.VeilTransition.AnswersWithoutAWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilTransitionIsPureTest::RunTest(const FString&)
{
	// Nessun `UWorld`, nessun Actor, nessuna mappa, nessuna conoscenza: solo numeri.
	const float V = URTVeilTransitionLibrary::Advance(0.2f, 0.8f, 1.f / 60.f, Default());
	TestTrue(TEXT("il filtro risponde senza mondo, e il valore avanza verso il target"),
		V > 0.2f && V < 0.8f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
