// CP 11.4 (#80) — gli otto comandi `rt.Debug.*`, e cio' che di essi si verifica senza aprire il gioco.
//
// Un comando console non e' testabile: legge un `UWorld` e scrive su un `FOutputDevice`. Testabile e' la
// FUNZIONE PURA che compone le righe, e ogni comando qui e' un wrapper sottile sopra una di quelle. E' la
// stessa forma di `ARTHUD::ComputePlannedHitMarks` e `ComposeSlotLines`, che il repository usa gia' per
// rendere verificabile la HUD.
//
// ⚠️ Dichiarato: questi test NON provano che una linea compaia a schermo. Il disegno resta `PIE-V01-DEBUG`,
// seduta U15. Qui si prova cosa lo strumento **dice**, che e' dove vive l'invariante #6.

#include "Debug/RTDebugReportLibrary.h"
#include "HAL/IConsoleManager.h"
#include "Map/RTHexCellData.h"
#include "Misc/AutomationTest.h"
#include "Turn/RTIntentPrivacyLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Piano completo: movimento, azione, bersaglio e reazione pronta — tutto cio' che si puo' esporre. */
	FRTPlannedIntent MakeDebugIntent(int32 TeamId, bool bRevealed, const FRTCellId& Owner)
	{
		FRTPlannedIntent I;
		I.OwnerCell = Owner;
		I.TeamId = TeamId;
		I.bAlive = true;
		I.bRevealed = bRevealed;
		I.bMoving = true;
		I.PlannedCell = FRTCellId(7, -3);
		I.ActionName = FText::FromString(TEXT("ScaricaLineare"));
		I.bHasTarget = true;
		I.TargetCell = FRTCellId(9, -3);
		I.ReactionName = FText::FromString(TEXT("CondensatoreReattivo"));
		I.PlannedPath = { Owner, FRTCellId(6, -3), FRTCellId(7, -3) };
		I.PlannedWaypoints = { FRTCellId(7, -3) };
		return I;
	}

	/** `true` se una qualunque riga contiene il frammento. Le righe sono prosa: si cerca dentro, non uguale. */
	bool AnyLineContains(const TArray<FString>& Lines, const TCHAR* Needle)
	{
		for (const FString& L : Lines)
		{
			if (L.Contains(Needle)) { return true; }
		}
		return false;
	}

	/** Una traccia breve ma non banale: due turni, due categorie, coordinate e importi distinti. */
	TArray<FRTTurnLogEntry> MakeTrace()
	{
		auto Entry = [](int32 Turn, ERTLogCategory Category, const TCHAR* ActionId, int32 Amount)
		{
			FRTTurnLogEntry E;
			E.TurnNumber = Turn;
			E.Phase = ERTMatchPhase::Blast;
			E.Category = Category;
			E.ActionId = FName(ActionId);
			E.Amount = Amount;
			E.UnitId = 3;
			E.SrcCell = FRTCellId(1, -1, 0);
			E.TgtCell = FRTCellId(2, -1, 0);
			E.Priority = 40;
			return E;
		};
		return {
			Entry(1, ERTLogCategory::Move,   TEXT("Action.Move"),        0),
			Entry(1, ERTLogCategory::Combat, TEXT("Action.BasicAttack"), 12),
			Entry(2, ERTLogCategory::Combat, TEXT("Action.BasicAttack"), 9),
		};
	}
}

/**
 * Nome vincolante della DoD di #80.
 *
 * Uno strumento di debug e' precisamente il posto in cui la tentazione di «stampare tutto» e' massima, e
 * l'invariante #6 non ammette eccezioni per gli strumenti: il DoD lo dice a parole («anche negli strumenti
 * di debug»). Il test ha quattro parti, e la prima esiste perche' senza di essa sarebbe VACUO — una
 * funzione che non stampa mai niente lo supererebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDebugDrawIntentHidesEnemyIntentTest,
	"RefactorTactics.Debug.DrawIntentHidesEnemyIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDebugDrawIntentHidesEnemyIntentTest::RunTest(const FString&)
{
	const FRTCellId Owner(5, -3);
	const TArray<FRTPlannedIntent> Hidden = { MakeDebugIntent(/*Team*/ 0, /*bRevealed*/ false, Owner) };

	// 1 — CONTROPROVA DI NON VACUITA': per il proprio team lo strumento dice qualcosa. Se questa cade, le
	// tre parti sotto non provano piu' niente, perche' passerebbero anche con un output sempre vuoto.
	{
		const TArray<FString> Own = URTDebugReportLibrary::DescribeIntents(/*Observer*/ 0, Hidden);
		if (!TestTrue(TEXT("per il proprio team lo strumento produce righe"), Own.Num() > 0)) { return false; }
		TestTrue(TEXT("e la riga nomina l'azione pianificata"), AnyLineContains(Own, TEXT("ScaricaLineare")));
		TestTrue(TEXT("e la reazione armata, che l'alleato deve poter leggere"),
			AnyLineContains(Own, TEXT("CondensatoreReattivo")));
	}

	// 2 — L'avversario di un'unita' NON rivelata non riceve nessuna riga. Non una riga con i campi
	// oscurati: nessuna riga. Non deve sapere nemmeno che un piano esiste.
	{
		const TArray<FString> Enemy = URTDebugReportLibrary::DescribeIntents(/*Observer*/ 1, Hidden);
		TestEqual(TEXT("nessuna riga per l'avversario non rivelato"), Enemy.Num(), 0);
	}

	// 3 — Su un'unita' RIVELATA l'avversario legge l'azione, mai la reazione. E' la regola di
	// `FilterForTeam`: chi riscrivesse la composizione ignorandola dovrebbe reimplementare questa
	// distinzione, e non la indovinerebbe.
	{
		const TArray<FRTPlannedIntent> Revealed = { MakeDebugIntent(0, /*bRevealed*/ true, Owner) };
		const TArray<FString> Enemy = URTDebugReportLibrary::DescribeIntents(/*Observer*/ 1, Revealed);
		if (!TestTrue(TEXT("l'avversario riceve la riga di un'unita' rivelata"), Enemy.Num() > 0)) { return false; }
		TestTrue(TEXT("e vi legge l'azione"), AnyLineContains(Enemy, TEXT("ScaricaLineare")));
		TestFalse(TEXT("ma MAI la reazione, nemmeno su un'unita' rivelata"),
			AnyLineContains(Enemy, TEXT("CondensatoreReattivo")));
	}

	// 4 — Il canale laterale. Due scene che differiscono SOLO per i piani nemici devono dare a un
	// osservatore lo stesso identico output: se cambiasse, la differenza sarebbe una deduzione affidabile
	// sulla posizione altrui, cioe' l'esposizione che l'invariante vieta anche senza mostrare un campo.
	{
		TArray<FRTPlannedIntent> SceneA = { MakeDebugIntent(0, false, Owner) };
		TArray<FRTPlannedIntent> SceneB = SceneA;
		SceneB.Add(MakeDebugIntent(/*Team*/ 1, /*bRevealed*/ false, FRTCellId(-2, 4)));

		const TArray<FString> FromA = URTDebugReportLibrary::DescribeIntents(/*Observer*/ 0, SceneA);
		const TArray<FString> FromB = URTDebugReportLibrary::DescribeIntents(/*Observer*/ 0, SceneB);
		TestEqual(TEXT("un piano nemico in piu' non cambia il numero di righe viste"), FromB.Num(), FromA.Num());
		for (int32 i = 0; i < FMath::Min(FromA.Num(), FromB.Num()); ++i)
		{
			TestEqual(TEXT("ne' il loro contenuto"), FromB[i], FromA[i]);
		}
	}
	return true;
}

/**
 * Nome vincolante della DoD di #80: «introduce una divergenza e verifica che il comando la rilevi».
 *
 * Le tre parti misurano tre proprieta' diverse, e la prima e la terza esistono perche' la seconda da sola
 * sarebbe soddisfatta da uno strumento che risponde SEMPRE «divergono» — l'errore piu' facile da scrivere
 * e il piu' difficile da notare, perche' il caso che si prova a mano e' sempre quello divergente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDebugVerifyReplayDetectsDivergenceTest,
	"RefactorTactics.Debug.VerifyReplayDetectsDivergence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDebugVerifyReplayDetectsDivergenceTest::RunTest(const FString&)
{
	const FName Format(TEXT("Format.Skirmish2v2"));
	const TArray<FRTTurnLogEntry> Golden = MakeTrace();
	const TArray<uint8> GoldenBytes = URTTurnLogLibrary::SerializeTurnLog(Golden, ERTLogTopology::Hex, Format);

	// 1 — CONTROPROVA: due tracce identiche NON divergono. Senza questa, «rileva la divergenza» sarebbe
	// soddisfatto anche da uno strumento che non guarda niente e risponde sempre di si'.
	{
		const FRTDebugReplayVerdict V = URTDebugReportLibrary::VerifyReplay(
			GoldenBytes, Golden, Golden, ERTLogTopology::Hex, Format);
		TestEqual(TEXT("due tracce identiche coincidono"),
			static_cast<int32>(V.Comparison), static_cast<int32>(ERTTraceComparison::Identical));
		TestTrue(TEXT("e non si nomina nessuna divergenza"), V.FirstDivergence.IsEmpty());
		TestTrue(TEXT("lo strumento dice comunque qualcosa: un verdetto muto non e' un verdetto"),
			V.Lines.Num() > 0);
	}

	// 2 — Il cuore del DoD: una sola voce cambiata, e lo strumento la trova. Cambio `Amount` sull'ULTIMA
	// voce, non la prima, cosi' un confronto che si fermasse presto non passerebbe per fortuna.
	{
		TArray<FRTTurnLogEntry> Actual = Golden;
		Actual.Last().Amount = Golden.Last().Amount + 1;

		const FRTDebugReplayVerdict V = URTDebugReportLibrary::VerifyReplay(
			GoldenBytes, Golden, Actual, ERTLogTopology::Hex, Format);
		TestEqual(TEXT("una voce diversa e' una DIVERGENZA"),
			static_cast<int32>(V.Comparison), static_cast<int32>(ERTTraceComparison::Divergence));
		TestFalse(TEXT("e la divergenza viene LOCALIZZATA, non solo annunciata"),
			V.FirstDivergence.IsEmpty());
	}

	// 3 — Due formati diversi non sono due tracce in disaccordo: sono due tracce non confrontabili.
	// Uno strumento che qui dicesse `Divergence` manderebbe a cercare un difetto di simulazione che non
	// c'e'. `ERTTraceComparison` distingue gia' i due casi, e il comando riporta la distinzione invece di
	// appiattirla su un booleano.
	{
		const FRTDebugReplayVerdict V = URTDebugReportLibrary::VerifyReplay(
			GoldenBytes, Golden, Golden, ERTLogTopology::Hex, FName(TEXT("Format.Altro")));
		TestNotEqual(TEXT("formati diversi non si dichiarano «divergenti»"),
			static_cast<int32>(V.Comparison), static_cast<int32>(ERTTraceComparison::Divergence));
	}
	return true;
}

/**
 * Il gate di chiusura di **E11** dice *«gli 8 comandi `rt.Debug.*` funzionano in PIE e in Development»*, e
 * fino a questo test la sola risposta possibile era guardare una cartella. Una cartella non e' un gate:
 * nessuno la esegue, e un comando registrato con un refuso nel nome ci starebbe dentro identico.
 *
 * Qui si interroga il **namespace a runtime**, che e' cio' che il gate intende davvero — e per questo il
 * test conta anche i due comandi che vivono in `Map/` e `Turn/`, dove il precedente del repository li
 * mette. Dove stia il file non e' piu' una proprieta' verificabile, e non deve esserlo.
 *
 * ⚠️ **Non prova che «funzionano»**: prova che sono registrati e raggiungibili. Che facciano la cosa giusta
 * lo provano gli altri test di questo file per la parte pura, e `PIE-V01-DEBUG` per il resto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDebugNamespaceDeclaresAllCommandsTest,
	"RefactorTactics.Debug.NamespaceDeclaresAllCommands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDebugNamespaceDeclaresAllCommandsTest::RunTest(const FString&)
{
	// I nomi che il DoD di #80 pretende. `DrawGrid` del testo originale e' `DrawCells`, che esisteva gia'
	// e si dichiarava parte di questa issue: deciso il 2026-08-24, la ragione e' nel commento della issue.
	static const TCHAR* const Required[] = {
		TEXT("rt.Debug.DrawCells"),
		TEXT("rt.Debug.DrawPaths"),
		TEXT("rt.Debug.DrawCover"),
		TEXT("rt.Debug.DrawIntent"),
		TEXT("rt.Debug.DrawResolution"),
		TEXT("rt.Debug.DumpSnapshot"),
		TEXT("rt.Debug.DumpTurnLog"),
		TEXT("rt.Debug.VerifyReplay"),
	};

	TSet<FString> Registered;
	IConsoleManager::Get().ForEachConsoleObjectThatStartsWith(
		FConsoleObjectVisitor::CreateLambda([&Registered](const TCHAR* Name, IConsoleObject*)
		{
			Registered.Add(FString(Name));
		}),
		TEXT("rt.Debug."));

	// CONTROPROVA: il visitor trova davvero qualcosa, e non trova cio' che non esiste. Senza, un visitor
	// mai invocato darebbe `Registered` vuoto e le asserzioni sotto fallirebbero per la ragione sbagliata;
	// un visitor che accettasse tutto le farebbe passare tutte.
	TestTrue(TEXT("il namespace rt.Debug. non e' vuoto"), Registered.Num() > 0);
	TestFalse(TEXT("e non contiene un nome inventato"), Registered.Contains(TEXT("rt.Debug.NonEsiste")));

	for (const TCHAR* Name : Required)
	{
		TestTrue(FString::Printf(TEXT("%s e' registrato"), Name), Registered.Contains(FString(Name)));
	}

	// `rt.Debug.Pacing` sta nel namespace e NON e' fra gli otto: il DoD elenca cio' che deve esserci, non
	// tutto cio' che c'e'. Pinnarlo qui evita che un domani qualcuno "allinei" il namespace all'elenco
	// cancellando un comando buono.
	TestTrue(TEXT("rt.Debug.Pacing resta, benche' fuori dagli otto del DoD"),
		Registered.Contains(TEXT("rt.Debug.Pacing")));
	return true;
}

/**
 * Il DoD di #80 elenca sette campi che «le celle mostrano». Il test li pretende **tutti**, uno per
 * asserzione, perche' un dump a cui ne manca uno e' esattamente lo strumento che non risponde alla
 * domanda per cui e' stato scritto — e la mancanza si nota solo il giorno in cui serve.
 *
 * ⚠️ Due dei sette non stanno nella cella, e il DoD non poteva saperlo: `OccupantId` vive nello snapshot e
 * `HazardTags` si deriva dalla superficie. La firma li prende quindi separati, invece di fingere che
 * `FRTHexCellData` li porti. La mappatura completa e' nel commento del 2026-08-24 sulla issue.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDebugCellReportCarriesEveryFieldTest,
	"RefactorTactics.Debug.CellReportCarriesEveryDeclaredField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDebugCellReportCarriesEveryFieldTest::RunTest(const FString&)
{
	FRTHexCellData Cell;
	Cell.Id = FRTCellId(4, -2, 1);
	Cell.Surface = ERTHexSurface::ShallowWater;
	Cell.MoveCost = 2;
	Cell.OccupancySurcharge = 1;             // costo totale 3: il campo e' la SOMMA, non `MoveCost`

	FRTHexCover Cover;
	Cover.Edge = ERTHexDirection::NE;
	Cover.Type = ERTHexCoverType::Low;
	Cover.Integrity = 20;
	Cell.Covers.Add(Cover);

	const FString Line = URTDebugReportLibrary::DescribeCell(Cell, /*OccupantUnitId*/ 7, /*Revision*/ 42);

	TestTrue(TEXT("CellId — le coordinate assiali, con il layer"), Line.Contains(TEXT("(q=4,r=-2,L=1)")));
	TestTrue(TEXT("TerrainId — la superficie, che nel modello si chiama Surface"),
		Line.Contains(TEXT("ShallowWater")));
	TestTrue(TEXT("TraversalCost — il costo TOTALE (2+1), non il solo MoveCost"), Line.Contains(TEXT("cost=3")));
	TestTrue(TEXT("OccupantId — che viene dallo snapshot, non dalla cella"), Line.Contains(TEXT("occupante=7")));
	TestTrue(TEXT("CoverEdges — il bordo coperto e il tipo"), Line.Contains(TEXT("NE")));
	TestTrue(TEXT("ChunkRevision — la revisione dell'asset"), Line.Contains(TEXT("rev=42")));
	// HazardTags: `ShallowWater` lega `Status.Wet` a chi ci sta sopra. E' l'unico dei sette che sia
	// DERIVATO, e mostrarlo e' cio' che rende il dump utile a capire perche' un'unita' e' bagnata.
	TestTrue(TEXT("HazardTags — gli stati che la superficie impone"), Line.Contains(TEXT("Status.Wet")));

	// Una cella libera non deve stampare un occupante finto: `0` e' un UnitId valido, e usarlo come
	// sentinella confonderebbe «cella vuota» con «ci sta l'unita' 0».
	const FString Empty = URTDebugReportLibrary::DescribeCell(Cell, /*OccupantUnitId*/ INDEX_NONE, 42);
	TestFalse(TEXT("una cella libera non dichiara un occupante"), Empty.Contains(TEXT("occupante=")));
	return true;
}

/**
 * L'altro elenco del DoD: gli otto campi che «le azioni mostrano».
 *
 * La parte narrativa viene da `URTTurnLogLibrary::DescribeEntry` e non e' riscritta qui: la traduzione di
 * un esito in italiano esiste gia' ed e' l'owner. Duplicarla darebbe due frasi diverse per lo stesso
 * evento — nel combat log una, nel dump di debug un'altra — e chi le confronta non saprebbe quale credere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDebugActionReportCarriesEveryFieldTest,
	"RefactorTactics.Debug.ActionReportCarriesEveryDeclaredField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDebugActionReportCarriesEveryFieldTest::RunTest(const FString&)
{
	FRTTurnLogEntry E;
	E.TurnNumber = 5;
	E.Phase = ERTMatchPhase::Blast;
	E.Category = ERTLogCategory::Fallback;
	E.Outcome = static_cast<uint8>(ERTFallbackOutcome::AttackedCell);
	E.ActionId = FName(TEXT("Hero.Wraith.PulseShot"));
	E.UnitId = 3;
	E.Priority = 40;
	E.SrcCell = FRTCellId(1, -1, 0);
	E.TgtCell = FRTCellId(2, -1, 0);

	const FString Line = URTDebugReportLibrary::DescribeLogEntry(E, /*SequenceIndex*/ 11);

	TestTrue(TEXT("ActionId"),                       Line.Contains(TEXT("Hero.Wraith.PulseShot")));
	TestTrue(TEXT("SourceUnitId — nel modello e' UnitId"), Line.Contains(TEXT("unita=3")));
	TestTrue(TEXT("fase"),                           Line.Contains(TEXT("Blast")));
	TestTrue(TEXT("priorita'"),                      Line.Contains(TEXT("p40")));
	TestTrue(TEXT("bersaglio"),                      Line.Contains(TEXT("(q=2,r=-1,L=0)")));
	TestTrue(TEXT("fallback — la categoria della voce"), Line.Contains(TEXT("Fallback")));
	// `ValidationResult` non esiste come campo: e' `Outcome`, un uint8 la cui enum dipende dalla
	// categoria. Il dump deve mostrare l'esito TRADOTTO, non il numero: `3` non dice niente a nessuno.
	TestTrue(TEXT("ValidationResult — l'esito tradotto, non il suo uint8"),
		Line.Contains(TEXT("cella")) || Line.Contains(TEXT("bersaglio")));
	TestTrue(TEXT("EventSequence — l'ordine, che nel modello e' l'indice"), Line.Contains(TEXT("#11")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
