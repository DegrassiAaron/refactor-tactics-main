#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Regola 6.1 di `docs/roadmap/roadmap-v0.1.md`: **nessun nome di test e' prefisso gerarchico di un altro**.
 *
 * Un nome che e' anche il padre di altri crea un nodo nell'albero dell'Automation Framework, e il test che
 * porta quel nome **non viene raccolto**: non fallisce, sparisce, e `trovati = avviati = completati` resta
 * vero. E' successo a `RefactorTactics.Reactions.Intercept`, uno dei dieci test vincolanti del catalogo (`G3`),
 * quando `Reactions.Intercept.NotAcrossLayers` l'ha reso padre. La regola era scritta in prosa, con proprio
 * questo nome come esempio, e nessun test la applicava.
 *
 * Il controllo legge il registro dei test e non il log di una run: un test saltato dal controller e' comunque
 * nel registro, ed e' li' che si vede. Vale per i nomi `RefactorTactics.*`: i test del motore non sono nostri.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoTestNameIsAPrefixOfAnotherTest,
	"RefactorTactics.Naming.NoTestNameIsAPrefixOfAnother",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoTestNameIsAPrefixOfAnotherTest::RunTest(const FString&)
{
	TArray<FAutomationTestInfo> Tests;
	FAutomationTestFramework::Get().GetValidTestNames(Tests);

	TSet<FString> Names;
	for (const FAutomationTestInfo& Info : Tests)
	{
		// `GetTestName()` e' il nome della CLASSE (`FRTInterceptTest`): il percorso con i punti e' `GetFullTestPath()`.
		// Il registro contiene anche i test del motore, che non sono nostri da rinominare: contano solo i nostri.
		if (Info.GetFullTestPath().StartsWith(TEXT("RefactorTactics.")))
		{
			Names.Add(Info.GetFullTestPath());
		}
	}

	// Premessa: il registro che sto leggendo contiene questo stesso test. Se il filtro in corso lo escludesse,
	// il controllo qui sotto non vedrebbe niente e passerebbe a vuoto.
	if (!TestTrue(TEXT("il registro dei test contiene questo stesso test (altrimenti il controllo e' vacuo)"),
			Names.Contains(GetBeautifiedTestName())))
	{
		return false;
	}

	// Per ogni nome, ogni suo prefisso gerarchico proprio: "A.B.C" -> "A.B", poi "A".
	TArray<FString> Padri;
	for (const FString& Name : Names)
	{
		FString Prefix = Name;
		int32 LastDot = INDEX_NONE;
		while (Prefix.FindLastChar(TEXT('.'), LastDot))
		{
			Prefix.LeftInline(LastDot);
			if (Names.Contains(Prefix))
			{
				Padri.AddUnique(Prefix);
			}
		}
	}

	Padri.Sort();
	for (const FString& Padre : Padri)
	{
		AddError(FString::Printf(TEXT("'%s' e' un test ed e' anche prefisso gerarchico di un altro test: l'Automation Framework "
			"lo tratta come nodo padre e puo' non eseguirlo (regola 6.1 di roadmap-v0.1.md). Rinomina il figlio nella forma "
			"dei fratelli, 'Gruppo.NomeDelFiglio' invece di 'Gruppo.Padre.NomeDelFiglio'."), *Padre));
	}
	return Padri.Num() == 0;
}

#endif // WITH_DEV_AUTOMATION_TESTS
