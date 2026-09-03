// Scrittura degli scenari di test nel formato JSON che `RTScenarioLoader.cpp` rilegge.
//
// Il verso che mancava. `URTScenarioLoader` sapeva leggere uno scenario e non sapeva scriverlo, e finche' e'
// stato cosi' qualunque authoring visuale avrebbe dovuto conoscere il formato da se' — diventando una
// **seconda** autorita' sul formato accanto al loader. Due autorita' sullo stesso formato divergono al primo
// campo aggiunto, e il file che ne esce e' illeggibile per una delle due.
//
// Per questo i metodi implementati qui appartengono a `URTScenarioLoader` e non a una classe nuova: lettura e
// scrittura sono due meta' della stessa regola. Vive in un `.cpp` separato solo perche' quello del loader ha
// gia' 1769 righe.
//
// ⚠️ **Due invarianti, e nessuna delle due e' cosmetica.**
//
// 1. **Determinismo.** I campi si scrivono in ordine ESPLICITO, uno per uno. Non si costruisce un
//    `FJsonObject` per poi serializzarlo: le sue chiavi vivono in una `TMap`, e l'ordine di iterazione di una
//    `TMap` non e' l'ordine di inserimento. Un writer costruito cosi' produrrebbe due file diversi per lo
//    stesso scenario, e un diff di PR diventerebbe rumore puro.
// 2. **Simmetria.** Ogni campo scritto qui e' un campo che il loader legge, e i nomi degli enum si ricavano
//    per RIFLESSIONE (`StaticEnum<>`), mai da una tabella scritta a mano. Il loader documenta gia' che una
//    tabella parallela diverge dall'enum che descrive — «e' gia' costato due test rossi in questo stesso
//    file». Una seconda tabella qui sarebbe la terza volta.

#include "ScenarioHarness/RTScenarioLoader.h"

#include "Ability/RTCatalogLibrary.h" // le risposte di profilo: decidono la versione minima del file
#include "Map/RTCellId.h"
#include "Turn/RTTurnLog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonWriter.h"

namespace
{
	using FRTScenarioJsonWriter = TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>;

	/**
	 * Il nome di un valore di enum, come lo scrive il JSON: `NE`, `UnitAtCell`, `CoverExpired`.
	 *
	 * Per riflessione, non per tabella. Aggiungere un valore all'enum lo rende scrivibile senza toccare
	 * questo file — che e' precisamente la proprieta' che una tabella parallela non ha.
	 */
	FString EnumValueName(const UEnum* Enum, int64 Value)
	{
		return Enum ? Enum->GetNameStringByValue(Value) : FString();
	}

	/** Una cella come `[q, r, layer]`. Sempre tre componenti: il loader ne accetta due, ma una forma sola e' una forma canonica. */
	void WriteCellArray(const TSharedRef<FRTScenarioJsonWriter>& W, const TCHAR* Key, const FRTCellId& Cell)
	{
		W->WriteArrayStart(Key);
		W->WriteValue(Cell.X);
		W->WriteValue(Cell.Y);
		W->WriteValue(Cell.Layer);
		W->WriteArrayEnd();
	}

	/**
	 * La versione minima del formato che le chiavi effettivamente usate richiedono.
	 *
	 * Serve a impedire un file che il loader rifiuterebbe. Uno scenario **caricato** non puo' trovarsi in
	 * quello stato — il loader lo avrebbe gia' respinto — ma uno **costruito in memoria** si': ed e'
	 * esattamente cio' che fa un editor di scenari. Senza questo controllo il round-trip si romperebbe solo
	 * per quel percorso, cioe' proprio per il caso d'uso per cui il writer esiste.
	 */
	int32 MinimumVersionFor(const FRTTestScenario& Scenario, FString& OutWhy)
	{
		if (Scenario.bFreeRun || Scenario.MaxTurns != 0 || Scenario.RepeatCount != 1
			|| Scenario.Requires.Num() > 0)
		{
			OutWhy = TEXT("'freeRun'/'maxTurns'/'repeatCount'/'requires'");
			return 4;
		}

		for (const FRTScenarioTurn& Turn : Scenario.Turns)
		{
			for (const FRTScenarioDecision& Decision : Turn.Decisions)
			{
				const bool bFire = Decision.Respond.Equals(TEXT("FIRE"), ESearchCase::CaseSensitive);
				const bool bHold = Decision.Respond.Equals(TEXT("HOLD"), ESearchCase::CaseSensitive);
				if (!bFire && !bHold && URTCatalogLibrary::IsKnownReactionProfileResponse(Decision.Respond))
				{
					OutWhy = FString::Printf(TEXT("la risposta di profilo '%s'"), *Decision.Respond);
					return 3;
				}
			}
		}

		for (const FRTScenarioTurn& Turn : Scenario.Turns)
		{
			if (Turn.Decisions.Num() > 0)
			{
				OutWhy = TEXT("'decisions'");
				return 2;
			}
		}

		OutWhy.Reset();
		return 1;
	}

	void WriteScenarioCells(const TSharedRef<FRTScenarioJsonWriter>& W, const FRTTestScenario& Scenario)
	{
		if (Scenario.Cells.Num() == 0) { return; }

		W->WriteArrayStart(TEXT("cells"));
		for (const FRTScenarioCell& Cell : Scenario.Cells)
		{
			W->WriteObjectStart();
			WriteCellArray(W, TEXT("cell"), Cell.Cell);
			// I default non si scrivono: una cella che non modifica niente e' rumore in un diff.
			if (Cell.bBlocksMovement) { W->WriteValue(TEXT("blocksMovement"), true); }
			if (Cell.bBlocksLineOfSight) { W->WriteValue(TEXT("blocksLineOfSight"), true); }
			if (Cell.MoveCost != 0) { W->WriteValue(TEXT("moveCost"), Cell.MoveCost); }
			if (Cell.OccupancySurcharge != 0) { W->WriteValue(TEXT("occupancySurcharge"), Cell.OccupancySurcharge); }
			W->WriteObjectEnd();
		}
		W->WriteArrayEnd();
	}

	/**
	 * I MURI INTERNI, che il writer fino a qui PERDEVA in silenzio — `#2031` sopra `#1830`.
	 *
	 * 🔑 **Il campo esisteva nel loader e non nel writer, e nessun test se ne accorgeva** perche'
	 * `ScenariosEquivalent` non lo guardava: uno scenario con geometria interna, letto e riscritto, tornava
	 * senza muri e il round-trip restava verde. Misurato su `main` il 2026-09-01: zero occorrenze di
	 * `interiorWalls` qui dentro, zero di `InteriorWalls` nel confronto.
	 *
	 * ⚠️ Il vocabolario e' quello del loader e non un secondo dialetto: `axis` e `type` sono NOMI
	 * d'enum, non indici, e `layer` si omette quando coincide con quello della cella — e' cio' che il
	 * parser assume come default.
	 */
	void WriteScenarioInteriorWalls(const TSharedRef<FRTScenarioJsonWriter>& W, const FRTTestScenario& Scenario)
	{
		if (Scenario.InteriorWalls.Num() == 0)
		{
			return;
		}

		const UEnum* AxisEnum = StaticEnum<ERTTacticalAxis>();
		const UEnum* TypeEnum = StaticEnum<ERTHexCoverType>();

		W->WriteArrayStart(TEXT("interiorWalls"));
		for (const FRTHexInteriorWall& Wall : Scenario.InteriorWalls)
		{
			W->WriteObjectStart();
			WriteCellArray(W, TEXT("cell"), Wall.Cell);
			if (AxisEnum)
			{
				W->WriteValue(TEXT("axis"),
					AxisEnum->GetNameStringByValue(static_cast<int64>(Wall.Segment.Axis)));
			}
			if (Wall.Segment.Offset != 0) { W->WriteValue(TEXT("offset"), Wall.Segment.Offset); }
			W->WriteValue(TEXT("alongStart"), Wall.Segment.AlongStart);
			W->WriteValue(TEXT("alongEnd"), Wall.Segment.AlongEnd);
			if (Wall.Segment.Layer != Wall.Cell.Layer) { W->WriteValue(TEXT("layer"), Wall.Segment.Layer); }
			if (TypeEnum)
			{
				W->WriteValue(TEXT("type"),
					TypeEnum->GetNameStringByValue(static_cast<int64>(Wall.Segment.WallType)));
			}
			if (!Wall.StableId.IsNone()) { W->WriteValue(TEXT("stableId"), Wall.StableId.ToString()); }
			W->WriteObjectEnd();
		}
		W->WriteArrayEnd();
	}

	void WriteScenarioUnits(const TSharedRef<FRTScenarioJsonWriter>& W, const FRTTestScenario& Scenario)
	{
		const UEnum* DirectionEnum = StaticEnum<ERTHexDirection>();

		W->WriteArrayStart(TEXT("units"));
		for (const FRTScenarioUnit& Unit : Scenario.Units)
		{
			W->WriteObjectStart();
			// `id` per primo, e mai derivato: e' lo **Stable Unit ID** che intent, decisioni e assertion
			// nominano. Rigenerarlo al salvataggio scollegherebbe in blocco tutto cio' che lo cita.
			W->WriteValue(TEXT("id"), Unit.Id);
			W->WriteValue(TEXT("hero"), Unit.HeroId.ToString());
			W->WriteValue(TEXT("team"), Unit.TeamId);
			WriteCellArray(W, TEXT("cell"), Unit.Cell);

			// `E` e' il default della struct: ometterlo rilegge identico, dichiararlo ovunque no.
			if (Unit.Facing != ERTHexDirection::E)
			{
				W->WriteValue(TEXT("facing"), EnumValueName(DirectionEnum, static_cast<int64>(Unit.Facing)));
			}
			if (Unit.bBotControlled) { W->WriteValue(TEXT("bot"), true); }
			// -1 e' il «non dichiarato» di queste tre: scriverlo lo trasformerebbe in una dichiarazione.
			if (Unit.Health != -1) { W->WriteValue(TEXT("health"), Unit.Health); }
			if (Unit.Shield != -1) { W->WriteValue(TEXT("shield"), Unit.Shield); }
			if (Unit.VisionRange != -1) { W->WriteValue(TEXT("visionRange"), Unit.VisionRange); }

			// `bLoadoutDeclared` distingue «nessun loadout» da «loadout vuoto dichiarato», e il loader lo
			// ricava dalla PRESENZA della chiave. Un loadout vuoto ma dichiarato va quindi scritto vuoto.
			if (Unit.bLoadoutDeclared)
			{
				W->WriteArrayStart(TEXT("loadout"));
				for (const FName& Piece : Unit.Loadout) { W->WriteValue(Piece.ToString()); }
				W->WriteArrayEnd();
			}

			// Gli status si scrivono NELL'ORDINE DICHIARATO e non si riordinano: `ApplyStatus` li applica
			// uno per uno, e un riordino cambierebbe cio' che il round-trip restituisce — `#1629`.
			if (Unit.Statuses.Num() > 0)
			{
				W->WriteArrayStart(TEXT("statuses"));
				for (const FRTScenarioStatus& Status : Unit.Statuses)
				{
					W->WriteObjectStart();
					W->WriteValue(TEXT("tag"), Status.Tag.ToString());
					// `turns` si scrive sempre: e' meta' del dato, e ometterlo al valore di default
					// costringerebbe chi legge il file a sapere qual e' quel default.
					W->WriteValue(TEXT("turns"), Status.Turns);
					W->WriteObjectEnd();
				}
				W->WriteArrayEnd();
			}
			W->WriteObjectEnd();
		}
		W->WriteArrayEnd();
	}

	void WriteScenarioIntent(const TSharedRef<FRTScenarioJsonWriter>& W, const FRTScenarioIntent& Intent)
	{
		const UEnum* DirectionEnum = StaticEnum<ERTHexDirection>();

		W->WriteObjectStart();
		W->WriteValue(TEXT("unit"), Intent.UnitId);
		if (!Intent.Ability.IsNone()) { W->WriteValue(TEXT("ability"), Intent.Ability.ToString()); }
		if (!Intent.Target.IsEmpty()) { W->WriteValue(TEXT("target"), Intent.Target); }
		if (Intent.bTargetsCell) { WriteCellArray(W, TEXT("targetCell"), Intent.TargetCell); }
		if (!Intent.Dash.IsNone())
		{
			W->WriteValue(TEXT("dash"), Intent.Dash.ToString());
			// Il loader RIFIUTA un `dash` senza `dashTo`: i due viaggiano insieme o non viaggiano.
			WriteCellArray(W, TEXT("dashTo"), Intent.DashCell);
		}
		if (!Intent.Reaction.IsNone()) { W->WriteValue(TEXT("reaction"), Intent.Reaction.ToString()); }
		if (Intent.Move.Num() > 0)
		{
			W->WriteArrayStart(TEXT("move"));
			for (const FRTCellId& Step : Intent.Move)
			{
				W->WriteArrayStart();
				W->WriteValue(Step.X);
				W->WriteValue(Step.Y);
				W->WriteValue(Step.Layer);
				W->WriteArrayEnd();
			}
			W->WriteArrayEnd();
		}
		if (Intent.bHasCoverEdge)
		{
			W->WriteValue(TEXT("edge"), EnumValueName(DirectionEnum, static_cast<int64>(Intent.CoverEdge)));
		}
		if (Intent.bDeclaresFacing)
		{
			W->WriteValue(TEXT("facing"), EnumValueName(DirectionEnum, static_cast<int64>(Intent.Facing)));
		}
		if (Intent.Condition.IsDeclared())
		{
			W->WriteObjectStart(TEXT("condition"));
			W->WriteValue(TEXT("id"), Intent.Condition.Id.ToString());
			W->WriteValue(TEXT("param"), Intent.Condition.Param);
			W->WriteObjectEnd();
		}
		W->WriteObjectEnd();
	}

	void WriteScenarioTurns(const TSharedRef<FRTScenarioJsonWriter>& W, const FRTTestScenario& Scenario)
	{
		if (Scenario.Turns.Num() == 0) { return; }

		W->WriteArrayStart(TEXT("turns"));
		for (const FRTScenarioTurn& Turn : Scenario.Turns)
		{
			W->WriteObjectStart();

			W->WriteArrayStart(TEXT("intents"));
			for (const FRTScenarioIntent& Intent : Turn.Intents) { WriteScenarioIntent(W, Intent); }
			W->WriteArrayEnd();

			if (Turn.Requires.Num() > 0)
			{
				W->WriteArrayStart(TEXT("requires"));
				for (const FString& Capability : Turn.Requires) { W->WriteValue(Capability); }
				W->WriteArrayEnd();
			}
			if (Turn.Decisions.Num() > 0)
			{
				W->WriteArrayStart(TEXT("decisions"));
				for (const FRTScenarioDecision& Decision : Turn.Decisions)
				{
					W->WriteObjectStart();
					W->WriteValue(TEXT("unit"), Decision.Unit);
					W->WriteValue(TEXT("respond"), Decision.Respond);
					// Il loader rifiuta un `target` su una risposta che non sia `FIRE`: si scrive se c'e'.
					if (!Decision.Target.IsEmpty()) { W->WriteValue(TEXT("target"), Decision.Target); }
					W->WriteObjectEnd();
				}
				W->WriteArrayEnd();
			}
			W->WriteObjectEnd();
		}
		W->WriteArrayEnd();
	}

	/** Un evento del TurnLog come coppia di NOMI. Gli stessi che `ParseScenarioLogEvent` rilegge. */
	void WriteLogEvent(const TSharedRef<FRTScenarioJsonWriter>& W, const TCHAR* CategoryKey, const TCHAR* OutcomeKey,
		ERTLogCategory Category, uint8 Outcome)
	{
		W->WriteValue(CategoryKey, EnumValueName(StaticEnum<ERTLogCategory>(), static_cast<int64>(Category)));
		W->WriteValue(OutcomeKey, EnumValueName(URTScenarioLoader::OutcomeEnumForCategory(Category),
			static_cast<int64>(Outcome)));
	}

	/**
	 * Il filtro per `ActionId`, **solo quando c'e'** (`#170`).
	 *
	 * ⚠️ **`NAME_None` non si scrive**, ed e' la stessa regola con cui il loader lo legge: una chiave assente
	 * significa «nessun filtro», mentre un `"actionId": "None"` sul disco sarebbe un filtro su un'azione che
	 * si chiama `None` — un round-trip che cambia il significato del file invece di preservarlo. E scriverlo
	 * come stringa vuota sarebbe peggio: il loader lo RIFIUTA di proposito.
	 */
	void WriteLogActionId(const TSharedRef<FRTScenarioJsonWriter>& W, const TCHAR* Key, FName ActionId)
	{
		if (!ActionId.IsNone())
		{
			W->WriteValue(Key, ActionId.ToString());
		}
	}

	void WriteScenarioExpectations(const TSharedRef<FRTScenarioJsonWriter>& W, const FRTTestScenario& Scenario)
	{
		const UEnum* KindEnum = StaticEnum<ERTAssertionKind>();
		const UEnum* DirectionEnum = StaticEnum<ERTHexDirection>();

		W->WriteArrayStart(TEXT("expect"));
		for (const FRTTestExpectation& Exp : Scenario.Expect)
		{
			W->WriteObjectStart();
			// I nomi di `ERTAssertionKind` SONO i `type` del JSON — per questo la riflessione basta e una
			// tabella `Kind -> stringa` sarebbe solo un secondo posto da tenere allineato.
			W->WriteValue(TEXT("type"), EnumValueName(KindEnum, static_cast<int64>(Exp.Kind)));

			switch (Exp.Kind)
			{
			case ERTAssertionKind::UnitAtCell:
				W->WriteValue(TEXT("unit"), Exp.UnitId);
				WriteCellArray(W, TEXT("cell"), Exp.Cell);
				break;

			case ERTAssertionKind::TurnsCompleted:
				W->WriteValue(TEXT("value"), Exp.Value);
				break;

			case ERTAssertionKind::UnitHpEquals:
				W->WriteValue(TEXT("unit"), Exp.UnitId);
				W->WriteValue(TEXT("value"), Exp.Value);
				break;

			case ERTAssertionKind::UnitAlive:
				W->WriteValue(TEXT("unit"), Exp.UnitId);
				// Qui `value` e' un BOOLEANO nel file, non l'intero che la struct conserva.
				W->WriteValue(TEXT("value"), Exp.Value != 0);
				break;

			case ERTAssertionKind::UnitFacing:
				W->WriteValue(TEXT("unit"), Exp.UnitId);
				// E qui e' il NOME di una direzione. Lo stesso campo, tre tipi diversi: il `switch` non e'
				// evitabile, ed e' il motivo per cui una serializzazione generica non funzionerebbe.
				W->WriteValue(TEXT("value"), EnumValueName(DirectionEnum, static_cast<int64>(Exp.Value)));
				break;

			case ERTAssertionKind::LogEventCount:
			case ERTAssertionKind::LogEventAmount:
				WriteLogEvent(W, TEXT("category"), TEXT("outcome"), Exp.LogCategory, Exp.LogOutcome);
				WriteLogActionId(W, TEXT("actionId"), Exp.LogActionId);
				W->WriteValue(TEXT("value"), Exp.Value);
				break;

			case ERTAssertionKind::LogEventOrder:
				WriteLogEvent(W, TEXT("category"), TEXT("outcome"), Exp.LogCategory, Exp.LogOutcome);
				WriteLogActionId(W, TEXT("actionId"), Exp.LogActionId);
				WriteLogEvent(W, TEXT("thenCategory"), TEXT("thenOutcome"), Exp.ThenCategory, Exp.ThenOutcome);
				WriteLogActionId(W, TEXT("thenActionId"), Exp.ThenActionId);
				break;

			case ERTAssertionKind::OriginalTargetEquals:
			case ERTAssertionKind::EffectiveTargetEquals:
				W->WriteValue(TEXT("unit"), Exp.UnitId);
				break;
			}
			W->WriteObjectEnd();
		}
		W->WriteArrayEnd();
	}

	void WriteScenarioVariants(const TSharedRef<FRTScenarioJsonWriter>& W, const FRTTestScenario& Scenario)
	{
		if (Scenario.Variants.Num() == 0) { return; }

		W->WriteArrayStart(TEXT("variants"));
		for (const FRTScenarioVariant& Variant : Scenario.Variants)
		{
			W->WriteObjectStart();
			W->WriteValue(TEXT("name"), Variant.Name);
			W->WriteArrayStart(TEXT("units"));
			for (const FRTScenarioVariantUnit& Unit : Variant.Units)
			{
				W->WriteObjectStart();
				W->WriteValue(TEXT("id"), Unit.Id);
				WriteCellArray(W, TEXT("cell"), Unit.Cell);
				W->WriteObjectEnd();
			}
			W->WriteArrayEnd();
			W->WriteObjectEnd();
		}
		W->WriteArrayEnd();
	}
}

bool URTScenarioLoader::SaveToString(const FRTTestScenario& Scenario, FString& OutJson, FString& OutError)
{
	OutError.Reset();

	// Validate PRIMA di scrivere un solo carattere. Un file valido a meta' e' peggio di un file assente:
	// il secondo si nota, il primo passa il parsing e mente.
	if (!Validate(Scenario, OutError))
	{
		return false;
	}

	if (Scenario.Version > SupportedVersion)
	{
		OutError = FString::Printf(
			TEXT("version: %d non e' scrivibile da questa build (massimo %d)"),
			Scenario.Version, SupportedVersion);
		return false;
	}

	FString WhyMinimum;
	const int32 Minimum = MinimumVersionFor(Scenario, WhyMinimum);
	if (Scenario.Version < Minimum)
	{
		OutError = FString::Printf(
			TEXT("version: dichiarata %d ma %s richiede \"version\": %d — scriverlo cosi' produrrebbe un file ")
			TEXT("che il loader rifiuta"),
			Scenario.Version, *WhyMinimum, Minimum);
		return false;
	}

	FString Json;
	const TSharedRef<FRTScenarioJsonWriter> Writer = TJsonWriterFactory<TCHAR,
		TPrettyJsonPrintPolicy<TCHAR>>::Create(&Json);

	// L'ordine che segue e' l'ordine canonico del formato, e coincide con quello di `FRTTestScenario`.
	// E' scritto a mano, campo per campo, ed e' cio' che rende il risultato riproducibile: vedi la nota
	// sull'ordine delle `TMap` in testa al file.
	Writer->WriteObjectStart();

	Writer->WriteValue(TEXT("scenarioId"), Scenario.ScenarioId);
	Writer->WriteValue(TEXT("version"), Scenario.Version);
	// I tag restano nell'intestazione, dove `URTScenarioIndex::ReadHeader` li cerca, e si riscrivono
	// **grezzi**: normalizzarli qui riscriverebbe i file di chi li ha scritti in maiuscolo.
	if (Scenario.Tags.Num() > 0)
	{
		Writer->WriteArrayStart(TEXT("tags"));
		for (const FString& Tag : Scenario.Tags) { Writer->WriteValue(Tag); }
		Writer->WriteArrayEnd();
	}
	if (Scenario.Seed != 0) { Writer->WriteValue(TEXT("seed"), Scenario.Seed); }
	if (!Scenario.PreviewUnit.IsEmpty()) { Writer->WriteValue(TEXT("previewUnit"), Scenario.PreviewUnit); }
	if (!Scenario.Fixture.IsEmpty()) { Writer->WriteValue(TEXT("fixture"), Scenario.Fixture); }
	Writer->WriteValue(TEXT("mapRadius"), Scenario.MapRadius);

	WriteScenarioCells(Writer, Scenario);
	WriteScenarioInteriorWalls(Writer, Scenario);
	WriteScenarioUnits(Writer, Scenario);
	WriteScenarioTurns(Writer, Scenario);
	WriteScenarioExpectations(Writer, Scenario);
	WriteScenarioVariants(Writer, Scenario);

	if (Scenario.bExpectSameAcrossVariants) { Writer->WriteValue(TEXT("expectSameAcrossVariants"), true); }
	if (Scenario.bFreeRun) { Writer->WriteValue(TEXT("freeRun"), true); }
	if (Scenario.MaxTurns != 0) { Writer->WriteValue(TEXT("maxTurns"), Scenario.MaxTurns); }
	if (Scenario.RepeatCount != 1) { Writer->WriteValue(TEXT("repeatCount"), Scenario.RepeatCount); }
	if (Scenario.Requires.Num() > 0)
	{
		Writer->WriteArrayStart(TEXT("requires"));
		for (const FString& Capability : Scenario.Requires) { Writer->WriteValue(Capability); }
		Writer->WriteArrayEnd();
	}

	Writer->WriteObjectEnd();
	Writer->Close();

	OutJson = MoveTemp(Json);
	return true;
}

bool URTScenarioLoader::SaveToFile(const FRTTestScenario& Scenario, const FString& FilePath, FString& OutError)
{
	OutError.Reset();

	if (FilePath.IsEmpty())
	{
		OutError = TEXT("percorso di destinazione vuoto");
		return false;
	}

	// Il testo si produce per intero prima di toccare il disco: se `SaveToString` rifiuta, il file
	// eventualmente gia' presente resta quello di prima.
	FString Json;
	if (!SaveToString(Scenario, Json, OutError))
	{
		return false;
	}

	if (!FFileHelper::SaveStringToFile(Json, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutError = FString::Printf(TEXT("scrittura fallita su '%s'"), *FilePath);
		return false;
	}

	return true;
}
