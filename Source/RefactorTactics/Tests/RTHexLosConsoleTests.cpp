#include "Misc/AutomationTest.h"
#include "HAL/IConsoleManager.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLosConsole.h"
#include "Map/RTHexVisionLibrary.h"

// La guardia: senza, questi test finiscono nel binario Shipping. Vedi `#923`.
#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Il verdetto come lo produce la primitiva, montato a mano per interrogare la sola RESA. */
	FRTLineOfSightResult LosConsoleVerdict(ERTLineOfSightBlock Block, const FRTCellId& At,
		const FRTCellId& FromCell, int32 Step)
	{
		FRTLineOfSightResult R;
		R.Block = Block;
		R.BlockedAt = At;
		R.BlockedFrom = FromCell;
		R.StepIndex = Step;
		return R;
	}

	FString LosConsoleJoined(const TArray<FString>& Lines) { return FString::Join(Lines, TEXT(" | ")); }
}

/**
 * OGNI CAUSA HA UN NOME, E NESSUNA CADE IN UN RAMO MUTO — `#1712`, `#1830`.
 *
 * 🔴 **E' il difetto che `RTHexLos::Describe` ha gia' dovuto correggere una volta**: aveva un `default:` che
 * scriveva `unavailable`, e quando `InteriorGeometry` e' arrivata quel ramo taceva **proprio sulla causa
 * nuova**. Un comando di debug che non nomina cio' che ha bloccato non serve a niente: e' la domanda per cui
 * lo si apre.
 *
 * ⚠️ Il test itera sui valori dell'enum invece di elencarne tre a mano: cosi' una causa aggiunta domani lo
 * fa cadere finche' non ha una riga sua, che e' esattamente cio' che si vuole.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLosConsoleNamesEveryCauseTest,
	"RefactorTactics.Debug.LosConsoleNamesEveryCause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLosConsoleNamesEveryCauseTest::RunTest(const FString&)
{
	const FRTCellId From(0, 0, 0);
	const FRTCellId To(4, 0, 0);
	const FRTCellId At(2, 0, 0);
	const FRTCellId Prev(1, 0, 0);

	// Le tre cause di blocco devono nominare se stesse in modo distinguibile.
	const TArray<TPair<ERTLineOfSightBlock, FString>> Attese = {
		{ ERTLineOfSightBlock::EdgeBlocker,      TEXT("BORDO") },
		{ ERTLineOfSightBlock::CellBlocker,      TEXT("CELLA") },
		{ ERTLineOfSightBlock::InteriorGeometry, TEXT("GEOMETRIA INTERNA") }
	};

	for (const TPair<ERTLineOfSightBlock, FString>& Caso : Attese)
	{
		const FString Testo = LosConsoleJoined(URTHexLosConsoleLibrary::DescribeVerdict(
			LosConsoleVerdict(Caso.Key, At, Prev, /*Step*/ 2), From, To));

		TestTrue(*FString::Printf(TEXT("la causa %d si nomina («%s»): %s"),
			static_cast<int32>(Caso.Key), *Caso.Value, *Testo), Testo.Contains(Caso.Value));

		// ANTI-VACUITA': non basta che il nome ci sia, deve esserci anche DOVE. Un verdetto che dice
		// «bloccata» senza la cella manda a cercare a mano cio' che il comando esiste per dire.
		TestTrue(*FString::Printf(TEXT("e dice dove: %s"), *Testo), Testo.Contains(TEXT("q=2,r=0")));
	}

	// E il caso libero non deve nominare nessuna causa.
	const FString Libera = LosConsoleJoined(URTHexLosConsoleLibrary::DescribeVerdict(
		LosConsoleVerdict(ERTLineOfSightBlock::None, FRTCellId(), FRTCellId(), INDEX_NONE), From, To));
	TestTrue(*FString::Printf(TEXT("una linea libera lo dice: %s"), *Libera), Libera.Contains(TEXT("libera")));
	TestFalse(TEXT("e non nomina un blocco"), Libera.Contains(TEXT("BLOCCATA")));
	return true;
}

/**
 * IL LAYER SI DICHIARA SEMPRE, ANCHE QUANDO E' ZERO — il caveat esplicito di `#1712`.
 *
 * `HasLineOfSight` **non guarda il layer**: la linea resta su quello del TIRATORE, ed e' la regola
 * d'elevazione ereditata dalla LOS quadrata (`RTHexVisionLibrary.h:27`). Su una mappa multilivello una
 * risposta che tace sul layer si legge come se valesse per tutti i piani — e non e' vero.
 *
 * 🔑 **E' il difetto piu' facile da introdurre e il piu' difficile da vedere**: l'output *sembra* corretto,
 * perche' su una mappa mono-layer lo e'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLosConsoleDeclaresLayerTest,
	"RefactorTactics.Debug.LosConsoleDeclaresLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLosConsoleDeclaresLayerTest::RunTest(const FString&)
{
	// Mono-layer: il layer si dichiara lo stesso.
	{
		const FString Testo = LosConsoleJoined(URTHexLosConsoleLibrary::DescribeVerdict(
			LosConsoleVerdict(ERTLineOfSightBlock::None, FRTCellId(), FRTCellId(), INDEX_NONE),
			FRTCellId(0, 0, 0), FRTCellId(3, 0, 0)));
		TestTrue(*FString::Printf(TEXT("il layer 0 e' dichiarato: %s"), *Testo),
			Testo.Contains(TEXT("layer 0")));
		TestTrue(TEXT("e si dice di CHI e' quel layer"), Testo.Contains(TEXT("TIRATORE")));
	}

	// Layer diversi: oltre a dichiararlo, avverte che la linea non ci sale.
	{
		const FString Testo = LosConsoleJoined(URTHexLosConsoleLibrary::DescribeVerdict(
			LosConsoleVerdict(ERTLineOfSightBlock::None, FRTCellId(), FRTCellId(), INDEX_NONE),
			FRTCellId(0, 0, /*Layer*/ 1), FRTCellId(3, 0, /*Layer*/ 2)));

		TestTrue(*FString::Printf(TEXT("ragiona sul layer del tiratore, 1: %s"), *Testo),
			Testo.Contains(TEXT("layer 1")));
		TestTrue(TEXT("e avverte che il bersaglio sta su un altro piano"),
			Testo.Contains(TEXT("layer 2")) && Testo.Contains(TEXT("NON ci sale")));
	}
	return true;
}

/**
 * IL COMANDO E' REGISTRATO, E STA NEL NAMESPACE `rt.Debug.*`.
 *
 * ⚠️ **Non va aggiunto agli otto di `Debug.NamespaceDeclaresAllCommands`**, e non e' una dimenticanza: la
 * riga 259 di quel test fissa la regola — *«il DoD elenca cio' che deve esserci, non tutto cio' che c'e'»* —
 * e `rt.Debug.Pacing` e' li' a dimostrarlo. Gli otto appartengono al DoD di `#80`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLosConsoleIsRegisteredTest,
	"RefactorTactics.Debug.LosConsoleIsRegistered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLosConsoleIsRegisteredTest::RunTest(const FString&)
{
	IConsoleObject* Cmd = IConsoleManager::Get().FindConsoleObject(TEXT("rt.Debug.Los"));
	if (!TestNotNull(TEXT("rt.Debug.Los e' registrato"), Cmd))
	{
		return false;
	}

	// ⛔ Il nome NON promette un disegno, ed e' la meta' del difetto che `RTDebugConsole.cpp:163` registra:
	// tre comandi chiamati `Draw*` che stampano. Questo stampa e si chiama `Los`.
	const FString Help = Cmd->GetHelp();
	TestTrue(*FString::Printf(TEXT("l'aiuto dichiara che stampa: %s"), *Help),
		Help.Contains(TEXT("non disegna")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
