// Writer degli scenari di test: ROUND-TRIP, identita', forma canonica.
//
// La domanda che questi test fanno non e' «il writer produce JSON?» ma «cio' che rientra e' cio' che era
// uscito?». Per questo il confronto e' **campo per campo sul modello**, mai fra due stringhe: un confronto di
// stringhe passerebbe anche con un writer che scrive un formato che il loader non sa rileggere, e fallirebbe
// per una virgola spostata che non cambia niente. Nessuna delle due e' la domanda.
//
// I 78 file sotto `Scenarios/` sono scritti a mano. Un writer che perdesse `scenarioId`, i tag o gli Stable
// Unit ID li romperebbe in blocco al primo salvataggio, e questi test esistono per rendere quel giorno rosso
// invece che silenzioso.

#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h" // ON_SCOPE_EXIT: la cartella temporanea sparisce anche se il test esce prima
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"            // il guardiano sul corpus conta le note nel TESTO, non nel modello
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	//
	// Lo scenario copre di proposito i campi che un round-trip ingenuo perde: i `tags` (che il loader
	// ignorava), un `facing` non-default, `health`/`shield` dichiarati, un `loadout`, una cella modificata,
	// un intent con `move` multi-passo e uno con `ability` + `target`, e due assertion di tipo diverso.
	// 🔑 **I QUATTRO SLOT DELL'INTENT, TUTTI POPOLATI — `#1626`.** Prima di questa slice il JSON di prova
	// esercitava `unit`, `move` e `facing`: il confronto semantico guardava **quattordici** campi e ne
	// provava tre, quindi undici confronti erano vacui e una regressione su `target`, `dash`, `edge`,
	// `reaction` o `condition` non avrebbe fatto cadere niente. E' la stessa cecita' che questo file aveva
	// gia' avuto sulle pareti interne e sugli status.
	//
	// ⚠️ **Ogni campo sta su una unita' che puo' portarlo davvero**, e non e' pedanteria: `B1` e' affidata
	// al bot e `ValidateScenarioTurns` rifiuta un intent dichiarato per lei — *«il suo piano lo produce
	// l'utility scoring, non il file»*. Percio' `C1` (Ivrin) esiste: possiede `PassingBlade` e
	// `Deflection`, mentre Aevik non ha ne' un dash ne' quella reazione. Una fixture che nominasse una
	// mobilita' inesistente girerebbe lo stesso — il possesso del dash non e' validato — e sarebbe un dato
	// finto che sembra una prova.
	const TCHAR* ScenarioWriterRichJson = TEXT(R"JSON(
	{
	  "scenarioId": "Movement.WriterRoundTrip",
	  "version": 1,
	  "tags": ["movement", "Aevik", "regressione"],
	  "mapRadius": 4,
	  "cells": [
	    { "cell": [0, 0, 0], "blocksMovement": true, "moveCost": 2 },
	    { "cell": [1, 0, 0], "blocksLineOfSight": true, "occupancySurcharge": 3 },
	    { "cell": [-1, 0, 0], "moveCost": 1 }
	  ],
	  "interiorWalls": [
	    { "cell": [-1, 0, 0], "axis": "Deg90", "alongStart": -12, "alongEnd": 12, "type": "High" },
	    { "cell": [-1, 0, 0], "axis": "Deg0", "offset": 6, "alongStart": 0, "alongEnd": 12,
	      "type": "Low", "stableId": "Muretto" }
	  ],
	  "units": [
	    { "id": "A1", "hero": "Hero.Aevik", "team": 0, "cell": [-2, 0, 0], "facing": "SW", "health": 12,
	      "statuses": [ { "tag": "Status.Guarded", "turns": 3 }, { "tag": "Status.Wet", "turns": 1 } ] },
	    { "id": "B1", "hero": "Hero.Branth", "team": 1, "cell": [2, 0, 0], "shield": 4, "visionRange": 6, "bot": true },
	    { "id": "C1", "hero": "Hero.Ivrin", "team": 0, "cell": [-2, 1, 0] }
	  ],
	  "turns": [
	    { "intents": [ { "unit": "A1", "move": [[-1, 0, 0], [0, -1, 0]] } ] },
	    { "intents": [ { "unit": "A1", "facing": "NE" } ], "requires": ["Movement"] },
	    { "intents": [
	        { "unit": "A1", "move": [[0, 0, 0]], "ability": "Action.BasicAttack", "target": "B1",
	          "facing": "SE", "edge": "NW" },
	        { "unit": "C1", "ability": "Action.Guard" }
	    ] },
	    { "intents": [
	        { "unit": "C1", "dash": "Hero.Ivrin.PassingBlade", "dashTo": [1, -1, 0] },
	        { "unit": "A1", "ability": "Action.Interact", "targetCell": [0, 0, 0] }
	    ] },
	    { "intents": [
	        { "unit": "C1", "reaction": "Hero.Ivrin.Deflection",
	          "condition": { "id": "TargetHealthAtOrBelowPercent", "param": 10 } }
	    ] }
	  ],
	  "expect": [
	    { "type": "UnitAtCell", "unit": "A1", "cell": [0, -1, 0] },
	    { "type": "TurnsCompleted", "value": 5 },
	    { "type": "UnitAlive", "unit": "B1", "value": true },
	    { "type": "UnitFacing", "unit": "A1", "value": "NE" }
	  ],
	  "variants": [
	    { "name": "Spostata", "units": [ { "id": "B1", "cell": [3, 0, 0] } ] },
	    { "name": "Arretrata", "units": [ { "id": "B1", "cell": [4, 0, 0] } ] }
	  ]
	}
	)JSON");

	/** Confronto SEMANTICO fra due scenari: campo per campo. Riporta la prima differenza, non un booleano. */
	/**
	 * Le note `_*` di UN oggetto, confrontate chiave per chiave e valore per valore.
	 *
	 * ⚠️ **Confronta anche il VALORE, non solo la chiave.** Un writer che riscrivesse `_nota` con il testo
	 * sbagliato — o troncato — supererebbe un confronto sulle sole chiavi, e la prosa che l'utente non puo'
	 * riscrivere a memoria sarebbe persa lo stesso.
	 */
	bool NotesEquivalent(const TArray<FRTScenarioNote>& A, const TArray<FRTScenarioNote>& B,
		const FString& Where, FString& OutDiff)
	{
		if (A.Num() != B.Num())
		{
			OutDiff = FString::Printf(TEXT("%s: %d note contro %d"), *Where, A.Num(), B.Num());
			return false;
		}
		for (int32 I = 0; I < A.Num(); ++I)
		{
			if (A[I].Key != B[I].Key)
			{
				OutDiff = FString::Printf(TEXT("%s[%d]: chiave '%s' contro '%s'"),
					*Where, I, *A[I].Key, *B[I].Key);
				return false;
			}
			if (A[I].RawJson != B[I].RawJson)
			{
				OutDiff = FString::Printf(TEXT("%s['%s']: il valore cambia nel round-trip"), *Where, *A[I].Key);
				return false;
			}
		}
		return true;
	}

	bool ScenariosEquivalent(const FRTTestScenario& L, const FRTTestScenario& R, FString& OutDiff)
	{
		auto Fail = [&OutDiff](const FString& What) { OutDiff = What; return false; };

		if (L.ScenarioId != R.ScenarioId) { return Fail(FString::Printf(TEXT("scenarioId: '%s' vs '%s'"), *L.ScenarioId, *R.ScenarioId)); }
		if (L.Version != R.Version) { return Fail(FString::Printf(TEXT("version: %d vs %d"), L.Version, R.Version)); }
		if (L.Seed != R.Seed) { return Fail(TEXT("seed")); }
		if (L.PreviewUnit != R.PreviewUnit) { return Fail(TEXT("previewUnit")); }
		if (L.Fixture != R.Fixture) { return Fail(TEXT("fixture")); }
		if (L.MapRadius != R.MapRadius) { return Fail(FString::Printf(TEXT("mapRadius: %d vs %d"), L.MapRadius, R.MapRadius)); }
		// La PRESENZA della chiave e' dato quanto il valore: un writer che la materializzasse dove il file
		// taceva — o che la perdesse dove il file la dichiarava — passerebbe il confronto sul solo intero.
		if (L.bHasMapRadius != R.bHasMapRadius)
		{
			return Fail(FString::Printf(TEXT("mapRadius dichiarato: %s vs %s"),
				L.bHasMapRadius ? TEXT("si'") : TEXT("no"), R.bHasMapRadius ? TEXT("si'") : TEXT("no")));
		}
		if (!NotesEquivalent(L.Notes, R.Notes, TEXT("note di radice"), OutDiff)) { return false; }
		if (L.bExpectSameAcrossVariants != R.bExpectSameAcrossVariants) { return Fail(TEXT("expectSameAcrossVariants")); }
		if (L.bFreeRun != R.bFreeRun) { return Fail(TEXT("freeRun")); }
		if (L.MaxTurns != R.MaxTurns) { return Fail(TEXT("maxTurns")); }
		if (L.RepeatCount != R.RepeatCount) { return Fail(TEXT("repeatCount")); }
		if (L.Requires != R.Requires) { return Fail(TEXT("requires")); }
		if (L.Tags != R.Tags) { return Fail(FString::Printf(TEXT("tags: [%s] vs [%s]"), *FString::Join(L.Tags, TEXT(",")), *FString::Join(R.Tags, TEXT(",")))); }

		if (L.Cells.Num() != R.Cells.Num()) { return Fail(TEXT("numero di celle")); }
		for (int32 I = 0; I < L.Cells.Num(); ++I)
		{
			const FRTScenarioCell& A = L.Cells[I];
			const FRTScenarioCell& B = R.Cells[I];
			if (A.Cell != B.Cell) { return Fail(FString::Printf(TEXT("cells[%d].cell"), I)); }
			if (A.bBlocksMovement != B.bBlocksMovement) { return Fail(FString::Printf(TEXT("cells[%d].blocksMovement"), I)); }
			if (A.bBlocksLineOfSight != B.bBlocksLineOfSight) { return Fail(FString::Printf(TEXT("cells[%d].blocksLineOfSight"), I)); }
			if (A.MoveCost != B.MoveCost) { return Fail(FString::Printf(TEXT("cells[%d].moveCost"), I)); }
			if (A.OccupancySurcharge != B.OccupancySurcharge) { return Fail(FString::Printf(TEXT("cells[%d].occupancySurcharge"), I)); }
			if (!NotesEquivalent(A.Notes, B.Notes, FString::Printf(TEXT("cells[%d] note"), I), OutDiff)) { return false; }
		}

		// LE PORTE e i BINDING D'INTERAZIONE, che il confronto non guardava: e' la stessa cecita' che
		// `interiorWalls` qui sotto documenta, e ha lasciato passare un writer che li perdeva ENTRAMBI per
		// intero. Misurato il 2026-09-20: zero occorrenze di `doors` e `interactionBindings` nel writer,
		// sette scenari versionati che li dichiarano, e questo test verde (`#3118`).
		if (L.Doors.Num() != R.Doors.Num())
		{
			return Fail(FString::Printf(TEXT("doors: %d contro %d"), L.Doors.Num(), R.Doors.Num()));
		}
		for (int32 I = 0; I < L.Doors.Num(); ++I)
		{
			const FRTScenarioDoor& A = L.Doors[I];
			const FRTScenarioDoor& B = R.Doors[I];
			if (!(A.Cell == B.Cell)) { return Fail(FString::Printf(TEXT("doors[%d].cell"), I)); }
			if (A.Door.Edge != B.Door.Edge) { return Fail(FString::Printf(TEXT("doors[%d].edge"), I)); }
			if (A.Door.State != B.Door.State) { return Fail(FString::Printf(TEXT("doors[%d].state"), I)); }
			if (A.Door.DoorId != B.Door.DoorId) { return Fail(FString::Printf(TEXT("doors[%d].doorId"), I)); }
			if (A.Door.StableId != B.Door.StableId) { return Fail(FString::Printf(TEXT("doors[%d].stableId"), I)); }
			if (!NotesEquivalent(A.Notes, B.Notes, FString::Printf(TEXT("doors[%d] note"), I), OutDiff)) { return false; }
		}

		if (L.InteractionBindings.Num() != R.InteractionBindings.Num())
		{
			return Fail(FString::Printf(TEXT("interactionBindings: %d contro %d"),
				L.InteractionBindings.Num(), R.InteractionBindings.Num()));
		}
		for (int32 I = 0; I < L.InteractionBindings.Num(); ++I)
		{
			if (L.InteractionBindings[I].SourceId != R.InteractionBindings[I].SourceId)
			{
				return Fail(FString::Printf(TEXT("interactionBindings[%d].source"), I));
			}
			// L'ORDINE dei bersagli e' dato: `ApplyInteraction` li applica come sono scritti.
			if (L.InteractionBindings[I].TargetIds != R.InteractionBindings[I].TargetIds)
			{
				return Fail(FString::Printf(TEXT("interactionBindings[%d].targets"), I));
			}
		}

		// I MURI INTERNI, che stanno alla RADICE e non nella cella — `#1830` li porta, `#2031` li fa
		// sopravvivere alla riscrittura.
		//
		// ⚠️ Senza queste righe il confronto e' CIECO al campo: misurato su `main` il 2026-09-01, il
		// writer non li scriveva affatto e questo test era verde lo stesso. Un gate che smette di guardare
		// non lo dice.
		if (L.InteriorWalls.Num() != R.InteriorWalls.Num())
		{
			return Fail(FString::Printf(TEXT("interiorWalls: %d contro %d"),
				L.InteriorWalls.Num(), R.InteriorWalls.Num()));
		}
		for (int32 W = 0; W < L.InteriorWalls.Num(); ++W)
		{
			const FRTHexInteriorWall& WA = L.InteriorWalls[W];
			const FRTHexInteriorWall& WB = R.InteriorWalls[W];
			if (!(WA.Cell == WB.Cell) || WA.Segment.Axis != WB.Segment.Axis
				|| WA.Segment.Offset != WB.Segment.Offset
				|| WA.Segment.AlongStart != WB.Segment.AlongStart
				|| WA.Segment.AlongEnd != WB.Segment.AlongEnd
				|| WA.Segment.Layer != WB.Segment.Layer
				|| WA.Segment.WallType != WB.Segment.WallType
				|| WA.StableId != WB.StableId)
			{
				return Fail(FString::Printf(TEXT("interiorWalls[%d] su %s"), W, *WA.Cell.ToString()));
			}
		}

		if (L.Units.Num() != R.Units.Num()) { return Fail(TEXT("numero di unita'")); }
		for (int32 I = 0; I < L.Units.Num(); ++I)
		{
			const FRTScenarioUnit& A = L.Units[I];
			const FRTScenarioUnit& B = R.Units[I];
			if (A.Id != B.Id) { return Fail(FString::Printf(TEXT("units[%d].id: '%s' vs '%s'"), I, *A.Id, *B.Id)); }
			if (A.HeroId != B.HeroId) { return Fail(FString::Printf(TEXT("units[%d].hero"), I)); }
			if (A.TeamId != B.TeamId) { return Fail(FString::Printf(TEXT("units[%d].team"), I)); }
			if (A.Cell != B.Cell) { return Fail(FString::Printf(TEXT("units[%d].cell: %s vs %s"), I, *A.Cell.ToString(), *B.Cell.ToString())); }
			if (A.Facing != B.Facing) { return Fail(FString::Printf(TEXT("units[%d].facing"), I)); }
			if (A.bBotControlled != B.bBotControlled) { return Fail(FString::Printf(TEXT("units[%d].bot"), I)); }
			if (A.Health != B.Health) { return Fail(FString::Printf(TEXT("units[%d].health: %d vs %d"), I, A.Health, B.Health)); }
			if (A.Shield != B.Shield) { return Fail(FString::Printf(TEXT("units[%d].shield"), I)); }
			if (A.VisionRange != B.VisionRange) { return Fail(FString::Printf(TEXT("units[%d].visionRange"), I)); }
			// GLI STATUS INIZIALI — `#1629`. ⚠️ Senza queste righe il round-trip sarebbe CIECO al campo:
		// il writer potrebbe non scriverli e il test resterebbe verde.
		if (A.Statuses.Num() != B.Statuses.Num())
		{
			return Fail(FString::Printf(TEXT("units[%d].statuses: %d contro %d"),
				I, A.Statuses.Num(), B.Statuses.Num()));
		}
		for (int32 S = 0; S < A.Statuses.Num(); ++S)
		{
			if (A.Statuses[S].Tag != B.Statuses[S].Tag || A.Statuses[S].Turns != B.Statuses[S].Turns)
			{
				return Fail(FString::Printf(TEXT("units[%d].statuses[%d]"), I, S));
			}
		}

		if (A.bLoadoutDeclared != B.bLoadoutDeclared) { return Fail(FString::Printf(TEXT("units[%d].loadout dichiarato"), I)); }
			if (A.Loadout != B.Loadout) { return Fail(FString::Printf(TEXT("units[%d].loadout"), I)); }
			if (!NotesEquivalent(A.Notes, B.Notes, FString::Printf(TEXT("units[%d] note"), I), OutDiff)) { return false; }
		}

		if (L.Turns.Num() != R.Turns.Num()) { return Fail(TEXT("numero di turni")); }
		for (int32 T = 0; T < L.Turns.Num(); ++T)
		{
			const FRTScenarioTurn& A = L.Turns[T];
			const FRTScenarioTurn& B = R.Turns[T];
			if (A.Requires != B.Requires) { return Fail(FString::Printf(TEXT("turns[%d].requires"), T)); }
			if (!NotesEquivalent(A.Notes, B.Notes, FString::Printf(TEXT("turns[%d] note"), T), OutDiff)) { return false; }
			if (A.Decisions.Num() != B.Decisions.Num()) { return Fail(FString::Printf(TEXT("turns[%d]: numero di decisioni"), T)); }
			for (int32 D = 0; D < A.Decisions.Num(); ++D)
			{
				if (A.Decisions[D].Unit != B.Decisions[D].Unit) { return Fail(FString::Printf(TEXT("turns[%d].decisions[%d].unit"), T, D)); }
				if (A.Decisions[D].Respond != B.Decisions[D].Respond) { return Fail(FString::Printf(TEXT("turns[%d].decisions[%d].respond"), T, D)); }
				if (A.Decisions[D].Target != B.Decisions[D].Target) { return Fail(FString::Printf(TEXT("turns[%d].decisions[%d].target"), T, D)); }
				// Il SELETTORE entra nel confronto come ogni altro campo del formato: una chiave che il writer
				// dimenticasse si noterebbe solo qui, e il file salvato tornerebbe all'abbinamento per ordine
				// senza che nulla lo dica.
				if (A.Decisions[D].bHasSelector != B.Decisions[D].bHasSelector) { return Fail(FString::Printf(TEXT("turns[%d].decisions[%d].on (presenza)"), T, D)); }
				if (A.Decisions[D].On.Reactor != B.Decisions[D].On.Reactor) { return Fail(FString::Printf(TEXT("turns[%d].decisions[%d].on.reactor"), T, D)); }
				if (A.Decisions[D].On.Reaction != B.Decisions[D].On.Reaction) { return Fail(FString::Printf(TEXT("turns[%d].decisions[%d].on.reaction"), T, D)); }
				if (A.Decisions[D].On.TriggerUnit != B.Decisions[D].On.TriggerUnit) { return Fail(FString::Printf(TEXT("turns[%d].decisions[%d].on.triggerUnit"), T, D)); }
				if (!NotesEquivalent(A.Decisions[D].Notes, B.Decisions[D].Notes,
					FString::Printf(TEXT("turns[%d].decisions[%d] note"), T, D), OutDiff)) { return false; }
			}
			if (A.Intents.Num() != B.Intents.Num()) { return Fail(FString::Printf(TEXT("turns[%d]: numero di intent"), T)); }
			for (int32 N = 0; N < A.Intents.Num(); ++N)
			{
				const FRTScenarioIntent& X = A.Intents[N];
				const FRTScenarioIntent& Y = B.Intents[N];
				const FString Where = FString::Printf(TEXT("turns[%d].intents[%d]"), T, N);
				if (X.UnitId != Y.UnitId) { return Fail(Where + TEXT(".unit")); }
				if (X.Move != Y.Move) { return Fail(Where + TEXT(".move")); }
				if (X.Ability != Y.Ability) { return Fail(Where + TEXT(".ability")); }
				if (X.Target != Y.Target) { return Fail(Where + TEXT(".target")); }
				if (X.Dash != Y.Dash) { return Fail(Where + TEXT(".dash")); }
				if (!X.Dash.IsNone() && X.DashCell != Y.DashCell) { return Fail(Where + TEXT(".dashTo")); }
				if (X.bTargetsCell != Y.bTargetsCell) { return Fail(Where + TEXT(": targetCell dichiarata")); }
				if (X.bTargetsCell && X.TargetCell != Y.TargetCell) { return Fail(Where + TEXT(".targetCell")); }
				if (X.bHasCoverEdge != Y.bHasCoverEdge) { return Fail(Where + TEXT(": edge dichiarato")); }
				if (X.bHasCoverEdge && X.CoverEdge != Y.CoverEdge) { return Fail(Where + TEXT(".edge")); }
				if (X.bDeclaresFacing != Y.bDeclaresFacing) { return Fail(Where + TEXT(": facing dichiarato")); }
				if (X.bDeclaresFacing && X.Facing != Y.Facing) { return Fail(Where + TEXT(".facing")); }
				if (X.Reaction != Y.Reaction) { return Fail(Where + TEXT(".reaction")); }
				if (X.Condition.Id != Y.Condition.Id) { return Fail(Where + TEXT(".condition.id")); }
				if (X.Condition.Param != Y.Condition.Param) { return Fail(Where + TEXT(".condition.param")); }
				if (!NotesEquivalent(X.Notes, Y.Notes, Where + TEXT(" note"), OutDiff)) { return false; }
			}
		}

		if (L.Expect.Num() != R.Expect.Num()) { return Fail(TEXT("numero di assertion")); }
		for (int32 I = 0; I < L.Expect.Num(); ++I)
		{
			const FRTTestExpectation& A = L.Expect[I];
			const FRTTestExpectation& B = R.Expect[I];
			if (A.Kind != B.Kind) { return Fail(FString::Printf(TEXT("expect[%d].type"), I)); }
			if (A.UnitId != B.UnitId) { return Fail(FString::Printf(TEXT("expect[%d].unit"), I)); }
			if (A.Cell != B.Cell) { return Fail(FString::Printf(TEXT("expect[%d].cell"), I)); }
			if (A.Value != B.Value) { return Fail(FString::Printf(TEXT("expect[%d].value: %d vs %d"), I, A.Value, B.Value)); }
			if (A.LogCategory != B.LogCategory) { return Fail(FString::Printf(TEXT("expect[%d].category"), I)); }
			if (A.LogOutcome != B.LogOutcome) { return Fail(FString::Printf(TEXT("expect[%d].outcome"), I)); }
			if (A.ThenCategory != B.ThenCategory) { return Fail(FString::Printf(TEXT("expect[%d].thenCategory"), I)); }
			if (A.ThenOutcome != B.ThenOutcome) { return Fail(FString::Printf(TEXT("expect[%d].thenOutcome"), I)); }
			// IL CONFINE (`#2867`). ⚠️ Senza queste righe il confronto sarebbe CIECO al campo, che e' la
			// cecita' che ha lasciato passare `interiorWalls`, `statuses`, `doors` e `interactionBindings`:
			// un writer che perdesse `at` produrrebbe assertion valutate a fine turno e il round-trip
			// resterebbe verde — cioe' il difetto peggiore, un verde che misura un altro momento.
			if (A.bHasCheckpoint != B.bHasCheckpoint) { return Fail(FString::Printf(TEXT("expect[%d]: 'at' dichiarato"), I)); }
			if (A.bHasCheckpoint && A.At != B.At) { return Fail(FString::Printf(TEXT("expect[%d].at"), I)); }
			if (A.bHasAfterEvent != B.bHasAfterEvent) { return Fail(FString::Printf(TEXT("expect[%d]: 'afterEvent' dichiarato"), I)); }
			if (A.bHasAfterEvent)
			{
				const FRTScenarioEventSelector& X = A.AfterEvent;
				const FRTScenarioEventSelector& Y = B.AfterEvent;
				if (X.bHasCategory != Y.bHasCategory) { return Fail(FString::Printf(TEXT("expect[%d].afterEvent: category dichiarata"), I)); }
				if (X.bHasCategory && X.Category != Y.Category) { return Fail(FString::Printf(TEXT("expect[%d].afterEvent.category"), I)); }
				if (X.bHasOutcome != Y.bHasOutcome) { return Fail(FString::Printf(TEXT("expect[%d].afterEvent: outcome dichiarato"), I)); }
				if (X.bHasOutcome && X.Outcome != Y.Outcome) { return Fail(FString::Printf(TEXT("expect[%d].afterEvent.outcome"), I)); }
				if (X.ActionId != Y.ActionId) { return Fail(FString::Printf(TEXT("expect[%d].afterEvent.actionId"), I)); }
				if (X.Unit != Y.Unit) { return Fail(FString::Printf(TEXT("expect[%d].afterEvent.unit"), I)); }
			}
			if (!NotesEquivalent(A.Notes, B.Notes, FString::Printf(TEXT("expect[%d] note"), I), OutDiff)) { return false; }
		}

		if (L.Variants.Num() != R.Variants.Num()) { return Fail(TEXT("numero di varianti")); }
		for (int32 I = 0; I < L.Variants.Num(); ++I)
		{
			if (L.Variants[I].Name != R.Variants[I].Name) { return Fail(FString::Printf(TEXT("variants[%d].name"), I)); }
			if (L.Variants[I].Units.Num() != R.Variants[I].Units.Num()) { return Fail(FString::Printf(TEXT("variants[%d]: numero di unita'"), I)); }
			for (int32 U = 0; U < L.Variants[I].Units.Num(); ++U)
			{
				if (L.Variants[I].Units[U].Id != R.Variants[I].Units[U].Id) { return Fail(FString::Printf(TEXT("variants[%d].units[%d].id"), I, U)); }
				if (L.Variants[I].Units[U].Cell != R.Variants[I].Units[U].Cell) { return Fail(FString::Printf(TEXT("variants[%d].units[%d].cell"), I, U)); }
			}
		}

		OutDiff.Reset();
		return true;
	}

	/**
	 * I PERCORSI di ogni chiave `_*` presenti in un testo JSON: `turns[].intents[]::_intento`.
	 *
	 * 🔑 **Esiste per rispondere a una domanda che il confronto sul modello non puo' fare.**
	 * `ScenariosEquivalent` verifica che le note del modello tornino identiche dopo un round-trip, ma se un
	 * LIVELLO del formato non fosse coperto dalla raccolta, le due parti sarebbero vuote entrambe e il
	 * confronto direbbe verde — la stessa cecita' che ha lasciato passare `interiorWalls`, `statuses`,
	 * `doors` e `interactionBindings`. Questo conta cio' che il FILE dichiara, e lo confronta con cio' che il
	 * modello ha raccolto: una nota scritta a un livello nuovo rende rosso il corpus, nominando il livello.
	 *
	 * Le chiavi si visitano ORDINATE perche' `FJsonObject::Values` e' una `TMap`: senza, il messaggio d'errore
	 * cambierebbe fra due esecuzioni identiche.
	 */
	void CollectJsonNotePaths(const TSharedPtr<FJsonValue>& Value, const FString& Path, TArray<FString>& OutPaths)
	{
		if (!Value.IsValid()) { return; }

		if (Value->Type == EJson::Array)
		{
			for (const TSharedPtr<FJsonValue>& Item : Value->AsArray())
			{
				CollectJsonNotePaths(Item, Path + TEXT("[]"), OutPaths);
			}
			return;
		}
		if (Value->Type != EJson::Object) { return; }

		const TSharedPtr<FJsonObject> Obj = Value->AsObject();
		if (!Obj.IsValid()) { return; }

		// Ordinate, e convertite una volta sola: dalla 5.8 le chiavi di `FJsonObject::Values` non sono
		// `FString` ma `UE::FSharedString`, e l'ordine di una `TMap` non e' quello d'inserimento.
		TArray<TPair<FString, TSharedPtr<FJsonValue>>> Fields;
		Fields.Reserve(Obj->Values.Num());
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : Obj->Values)
		{
			Fields.Emplace(Field.Key, Field.Value);
		}
		Fields.Sort([](const TPair<FString, TSharedPtr<FJsonValue>>& A,
			const TPair<FString, TSharedPtr<FJsonValue>>& B) { return A.Key < B.Key; });

		for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : Fields)
		{
			const FString& Key = Field.Key;
			if (Key.StartsWith(TEXT("_")))
			{
				// Non si scende DENTRO una nota: cio' che contiene e' prosa, e il writer la restituisce come
				// valore unico. Contarne le chiavi interne farebbe divergere i due conteggi per costruzione.
				OutPaths.Add(FString::Printf(TEXT("%s::%s"), Path.IsEmpty() ? TEXT("<radice>") : *Path, *Key));
			}
			else
			{
				CollectJsonNotePaths(Field.Value, Path.IsEmpty() ? Key : Path + TEXT(".") + Key, OutPaths);
			}
		}
	}

	/** Come sopra, a partire dal testo. Ritorna `false` se il testo non e' JSON. */
	bool CollectJsonNotePathsFromText(const FString& JsonText, TArray<FString>& OutPaths)
	{
		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(JsonText);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) { return false; }

		CollectJsonNotePaths(MakeShared<FJsonValueObject>(Root.ToSharedRef()), FString(), OutPaths);
		OutPaths.Sort();
		return true;
	}

	/** Quante note porta il MODELLO, sommate su tutte le sedi che `FRTTestScenario` conosce. */
	int32 CountModelNotes(const FRTTestScenario& Scenario)
	{
		int32 Total = Scenario.Notes.Num();
		for (const FRTScenarioCell& Cell : Scenario.Cells) { Total += Cell.Notes.Num(); }
		for (const FRTScenarioDoor& Door : Scenario.Doors) { Total += Door.Notes.Num(); }
		for (const FRTScenarioUnit& Unit : Scenario.Units) { Total += Unit.Notes.Num(); }
		for (const FRTTestExpectation& Exp : Scenario.Expect) { Total += Exp.Notes.Num(); }
		for (const FRTScenarioTurn& Turn : Scenario.Turns)
		{
			Total += Turn.Notes.Num();
			for (const FRTScenarioIntent& Intent : Turn.Intents) { Total += Intent.Notes.Num(); }
			for (const FRTScenarioDecision& Decision : Turn.Decisions) { Total += Decision.Notes.Num(); }
		}
		return Total;
	}

	/** Cartella temporanea di questo file di test. Il writer scrive su disco, quindi serve un disco. */
	FString ScenarioWriterTempDir()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Tests"), TEXT("ScenarioWriter"));
	}
}

// --- round-trip semantico ------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioWriterRoundTripTest,
	"RefactorTactics.Scenario.WriterRoundTripPreservesEveryField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioWriterRoundTripTest::RunTest(const FString&)
{
	FRTTestScenario Original;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza accettato"),
		URTScenarioLoader::LoadFromString(ScenarioWriterRichJson, Original, Error)))
	{
		AddError(FString::Printf(TEXT("motivo del rifiuto: %s"), *Error));
		return false;
	}

	FString Json;
	if (!TestTrue(TEXT("scenario serializzato"), URTScenarioLoader::SaveToString(Original, Json, Error)))
	{
		AddError(FString::Printf(TEXT("motivo del rifiuto in scrittura: %s"), *Error));
		return false;
	}

	FRTTestScenario Reloaded;
	if (!TestTrue(TEXT("il JSON prodotto e' rileggibile dal loader"),
		URTScenarioLoader::LoadFromString(Json, Reloaded, Error)))
	{
		AddError(FString::Printf(TEXT("il writer ha prodotto un file che il loader rifiuta: %s\n--- JSON ---\n%s"),
			*Error, *Json));
		return false;
	}

	FString Diff;
	if (!TestTrue(TEXT("scenario ricaricato equivalente all'originale"),
		ScenariosEquivalent(Original, Reloaded, Diff)))
	{
		AddError(FString::Printf(TEXT("primo campo divergente: %s\n--- JSON ---\n%s"), *Diff, *Json));
		return false;
	}

	// Le tre identita' che l'issue nomina, asserite una per una invece che dentro il confronto generale:
	// se cadono, il messaggio deve dire QUALE, non «gli scenari differiscono».
	TestEqual(TEXT("ScenarioId preservato"), Reloaded.ScenarioId, TEXT("Movement.WriterRoundTrip"));
	TestEqual(TEXT("tag preservati come scritti"), Reloaded.Tags.Num(), 3);
	if (Reloaded.Tags.Num() == 3)
	{
		// `Gabget` maiuscolo resta maiuscolo: la normalizzazione appartiene all'indice, non al writer.
		TestEqual(TEXT("tag non normalizzato dal writer"), Reloaded.Tags[1], TEXT("Aevik"));
	}
	TestNotNull(TEXT("Stable Unit ID 'A1' ancora risolvibile"), Reloaded.FindUnit(TEXT("A1")));
	TestNotNull(TEXT("Stable Unit ID 'B1' ancora risolvibile"), Reloaded.FindUnit(TEXT("B1")));

	return true;
}

// --- forma canonica ------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioWriterIdempotentTest,
	"RefactorTactics.Scenario.WriterIsIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioWriterIdempotentTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("scenario caricato"),
		URTScenarioLoader::LoadFromString(ScenarioWriterRichJson, Scenario, Error)))
	{
		AddError(Error);
		return false;
	}

	FString First;
	FString Second;
	if (!TestTrue(TEXT("prima scrittura"), URTScenarioLoader::SaveToString(Scenario, First, Error))
		|| !TestTrue(TEXT("seconda scrittura"), URTScenarioLoader::SaveToString(Scenario, Second, Error)))
	{
		AddError(Error);
		return false;
	}

	// Qui il confronto di stringhe e' la domanda giusta, ed e' l'unico posto in cui lo e': la forma canonica
	// esiste perche' un diff di PR mostri le modifiche del contenuto e nient'altro.
	TestEqual(TEXT("due scritture consecutive producono lo stesso testo"), First, Second);

	// E il giro completo non deve derivare: salva -> ricarica -> risalva.
	FRTTestScenario Reloaded;
	if (!TestTrue(TEXT("riletto"), URTScenarioLoader::LoadFromString(First, Reloaded, Error)))
	{
		AddError(Error);
		return false;
	}
	FString Third;
	if (!TestTrue(TEXT("terza scrittura dopo un giro completo"),
		URTScenarioLoader::SaveToString(Reloaded, Third, Error)))
	{
		AddError(Error);
		return false;
	}
	TestEqual(TEXT("load->save->load->save e' stabile"), Third, First);

	return true;
}

// --- identita' indipendente dal percorso ---------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioWriterIdentityIsNotPathTest,
	"RefactorTactics.Scenario.WriterKeepsIdentityAcrossPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioWriterIdentityIsNotPathTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("scenario caricato"),
		URTScenarioLoader::LoadFromString(ScenarioWriterRichJson, Scenario, Error)))
	{
		AddError(Error);
		return false;
	}

	const FString Dir = ScenarioWriterTempDir();
	const FString PathA = FPaths::Combine(Dir, TEXT("Origine"), TEXT("Scenario.json"));
	const FString PathB = FPaths::Combine(Dir, TEXT("AltraCartella"), TEXT("NomeCompletamenteDiverso.json"));

	ON_SCOPE_EXIT{ IFileManager::Get().DeleteDirectory(*Dir, false, true); };

	if (!TestTrue(TEXT("salvato nel primo percorso"), URTScenarioLoader::SaveToFile(Scenario, PathA, Error))
		|| !TestTrue(TEXT("salvato nel secondo percorso"), URTScenarioLoader::SaveToFile(Scenario, PathB, Error)))
	{
		AddError(Error);
		return false;
	}

	FRTTestScenario FromA;
	FRTTestScenario FromB;
	if (!TestTrue(TEXT("riletto dal primo percorso"), URTScenarioLoader::LoadFromFile(PathA, FromA, Error))
		|| !TestTrue(TEXT("riletto dal secondo percorso"), URTScenarioLoader::LoadFromFile(PathB, FromB, Error)))
	{
		AddError(Error);
		return false;
	}

	// L'identita' e' DICHIARATA dal file, non dedotta dalla cartella: due percorsi, un solo `scenarioId`.
	TestEqual(TEXT("il percorso non cambia lo scenarioId"), FromA.ScenarioId, Scenario.ScenarioId);
	TestEqual(TEXT("nemmeno un nome di file diverso lo cambia"), FromB.ScenarioId, Scenario.ScenarioId);

	// E i tag continuano a essere quelli che l'INDICE legge — non solo quelli che il loader conserva.
	FString FileText;
	if (TestTrue(TEXT("file rileggibile come testo"), FFileHelper::LoadFileToString(FileText, *PathB)))
	{
		FString HeaderId;
		TArray<FString> HeaderTags;
		FString HeaderError;
		if (TestTrue(TEXT("l'indice sa leggere l'intestazione del file scritto"),
			URTScenarioIndex::ReadHeader(FileText, HeaderId, HeaderTags, HeaderError)))
		{
			TestEqual(TEXT("l'indice legge lo stesso scenarioId"), HeaderId, Scenario.ScenarioId);
			TestEqual(TEXT("l'indice ritrova tutti i tag"), HeaderTags.Num(), 3);
			// `ReadHeader` normalizza e ordina: e' il suo mestiere, e il writer non glielo ha tolto.
			//
			// 🔑 **Il tag dichiarato e' `"Aevik"` con la maiuscola, e l'atteso e' minuscolo**: e' proprio
			// la normalizzazione che questa riga verifica, non un confronto letterale. ⚠️ Cercava
			// `gadget` fino al 2026-09-10 — il rename di `#2491` ha aggiornato il tag dentro
			// `ScenarioWriterRichJson` e non l'assertion che lo interroga, e il test e' rimasto rosso su
			// `main`. Se un giorno cambia il tag nel JSON, cambia anche questa riga: sono una cosa sola.
			TestTrue(TEXT("il tag 'aevik' e' filtrabile"), HeaderTags.Contains(TEXT("aevik")));
		}
		else
		{
			AddError(HeaderError);
		}
	}

	return true;
}

// --- rifiuto dell'invalido -----------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioWriterRejectsInvalidTest,
	"RefactorTactics.Scenario.WriterRefusesToWriteInvalidScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioWriterRejectsInvalidTest::RunTest(const FString&)
{
	FRTTestScenario Valid;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"),
		URTScenarioLoader::LoadFromString(ScenarioWriterRichJson, Valid, Error)))
	{
		AddError(Error);
		return false;
	}

	const FString Dir = ScenarioWriterTempDir();
	const FString Path = FPaths::Combine(Dir, TEXT("NonDeveEsistere.json"));
	ON_SCOPE_EXIT{ IFileManager::Get().DeleteDirectory(*Dir, false, true); };

	// (1) Uno scenario senza assertion passerebbe sempre: `Validate` lo rifiuta, e il writer non lo scrive.
	{
		FRTTestScenario NoExpect = Valid;
		NoExpect.Expect.Reset();

		FString Json = TEXT("SENTINELLA");
		FString Why;
		TestFalse(TEXT("scenario senza assertion rifiutato"),
			URTScenarioLoader::SaveToString(NoExpect, Json, Why));
		TestEqual(TEXT("l'uscita non viene toccata da una scrittura rifiutata"), Json, TEXT("SENTINELLA"));
		// L'errore deve nominare il CAMPO, non dire «scenario invalido»: chi legge deve sapere dove guardare.
		TestTrue(FString::Printf(TEXT("l'errore nomina 'expect' (era: %s)"), *Why), Why.Contains(TEXT("expect")));

		TestFalse(TEXT("nemmeno su disco"), URTScenarioLoader::SaveToFile(NoExpect, Path, Why));
		TestFalse(TEXT("il file non e' stato creato"), FPaths::FileExists(Path));
	}

	// (2) Due unita' con lo stesso Stable Unit ID: un salvataggio le renderebbe indistinguibili.
	{
		FRTTestScenario DuplicateId = Valid;
		DuplicateId.Units[1].Id = DuplicateId.Units[0].Id;

		FString Json;
		FString Why;
		TestFalse(TEXT("id unita' duplicato rifiutato"),
			URTScenarioLoader::SaveToString(DuplicateId, Json, Why));
		TestTrue(FString::Printf(TEXT("l'errore nomina l'id duplicato (era: %s)"), *Why),
			Why.Contains(TEXT("A1")));
	}

	// (3) Versione insufficiente per le chiavi usate. E' il caso che NON puo' arrivare da un file — il loader
	//     lo rifiuterebbe — ma arriva da uno scenario costruito in memoria, cioe' da un editor. Senza questo
	//     controllo il writer produrrebbe un file che il loader poi rifiuta, e il round-trip si romperebbe
	//     solo lungo il percorso per cui il writer esiste.
	{
		FRTTestScenario FreeRunAtV1 = Valid;
		FreeRunAtV1.Turns.Reset();
		FreeRunAtV1.bFreeRun = true;
		FreeRunAtV1.MaxTurns = 5;
		FreeRunAtV1.Version = 1;
		for (FRTScenarioUnit& Unit : FreeRunAtV1.Units) { Unit.bBotControlled = true; }
		FreeRunAtV1.Expect.Reset();
		FRTTestExpectation Turns;
		Turns.Kind = ERTAssertionKind::TurnsCompleted;
		Turns.Value = 1;
		FreeRunAtV1.Expect.Add(Turns);

		FString Json;
		FString Why;
		TestFalse(TEXT("free-run dichiarato version 1 rifiutato"),
			URTScenarioLoader::SaveToString(FreeRunAtV1, Json, Why));
		TestTrue(FString::Printf(TEXT("l'errore nomina la version (era: %s)"), *Why),
			Why.Contains(TEXT("version")));
	}

	return true;
}

// --- verifica di mutazione -----------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioWriterMutationTest,
	"RefactorTactics.Scenario.WriterRoundTripDetectsMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioWriterMutationTest::RunTest(const FString&)
{
	// Un test di round-trip che confronta uno scenario con se stesso passa anche se il writer non scrive
	// niente. Questo verifica il verso opposto: che il confronto **morda**. Se una di queste mutazioni
	// passasse inosservata, il campo corrispondente non sarebbe coperto — e il round-trip direbbe verde su un
	// writer che lo perde.
	FRTTestScenario Base;
	FString Error;
	if (!TestTrue(TEXT("scenario caricato"),
		URTScenarioLoader::LoadFromString(ScenarioWriterRichJson, Base, Error)))
	{
		AddError(Error);
		return false;
	}

	auto MutationIsDetected = [this, &Base](const TCHAR* What, TFunctionRef<void(FRTTestScenario&)> Mutate)
	{
		FRTTestScenario Mutated = Base;
		Mutate(Mutated);

		FString Diff;
		const bool bStillEquivalent = ScenariosEquivalent(Base, Mutated, Diff);
		TestFalse(FString::Printf(TEXT("il confronto rileva la mutazione di %s"), What), bStillEquivalent);
	};

	MutationIsDetected(TEXT("units[0].cell"), [](FRTTestScenario& S) { S.Units[0].Cell = FRTCellId(9, 9, 1); });
	MutationIsDetected(TEXT("units[0].id"), [](FRTTestScenario& S) { S.Units[0].Id = TEXT("Rinominata"); });
	MutationIsDetected(TEXT("units[0].facing"), [](FRTTestScenario& S) { S.Units[0].Facing = ERTHexDirection::W; });
	MutationIsDetected(TEXT("units[0].health"), [](FRTTestScenario& S) { S.Units[0].Health = 99; });
	MutationIsDetected(TEXT("units[1].bot"), [](FRTTestScenario& S) { S.Units[1].bBotControlled = false; });
	MutationIsDetected(TEXT("tags"), [](FRTTestScenario& S) { S.Tags[0] = TEXT("altro"); });
	MutationIsDetected(TEXT("scenarioId"), [](FRTTestScenario& S) { S.ScenarioId = TEXT("Altro.Id"); });
	MutationIsDetected(TEXT("mapRadius"), [](FRTTestScenario& S) { S.MapRadius = 7; });
	MutationIsDetected(TEXT("cells[0].blocksMovement"), [](FRTTestScenario& S) { S.Cells[0].bBlocksMovement = false; });
	MutationIsDetected(TEXT("cells[0].moveCost"), [](FRTTestScenario& S) { S.Cells[0].MoveCost = 5; });
	MutationIsDetected(TEXT("il percorso di un move"), [](FRTTestScenario& S) { S.Turns[0].Intents[0].Move[0] = FRTCellId(4, 4, 0); });
	MutationIsDetected(TEXT("il facing dichiarato da un intent"), [](FRTTestScenario& S) { S.Turns[1].Intents[0].Facing = ERTHexDirection::SE; });
	MutationIsDetected(TEXT("turns[1].requires"), [](FRTTestScenario& S) { S.Turns[1].Requires[0] = TEXT("Altro"); });
	MutationIsDetected(TEXT("expect[0].cell"), [](FRTTestScenario& S) { S.Expect[0].Cell = FRTCellId(8, 8, 0); });
	MutationIsDetected(TEXT("expect[1].value"), [](FRTTestScenario& S) { S.Expect[1].Value = 42; });
	MutationIsDetected(TEXT("variants[0].units[0].cell"), [](FRTTestScenario& S) { S.Variants[0].Units[0].Cell = FRTCellId(5, 5, 0); });

	// E la mutazione deve sopravvivere anche al giro su JSON: se il writer la perdesse, lo scenario riletto
	// tornerebbe uguale alla base — cioe' il writer starebbe scrivendo un campo che ignora.
	{
		FRTTestScenario Mutated = Base;
		Mutated.Units[0].Cell = FRTCellId(1, -1, 0);

		FString Json;
		FString Why;
		if (!TestTrue(TEXT("scenario mutato serializzato"),
			URTScenarioLoader::SaveToString(Mutated, Json, Why)))
		{
			AddError(Why);
			return false;
		}

		FRTTestScenario Reloaded;
		if (!TestTrue(TEXT("scenario mutato riletto"), URTScenarioLoader::LoadFromString(Json, Reloaded, Why)))
		{
			AddError(Why);
			return false;
		}

		FString Diff;
		TestFalse(TEXT("la mutazione sopravvive al round-trip (non torna uguale alla base)"),
			ScenariosEquivalent(Base, Reloaded, Diff));
		TestTrue(TEXT("e il round-trip la riporta esatta"),
			ScenariosEquivalent(Mutated, Reloaded, Diff));
	}

	return true;
}

// --- i 78 file versionati ------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioWriterShippedScenariosTest,
	"RefactorTactics.Scenario.WriterRoundTripsShippedScenarios",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioWriterShippedScenariosTest::RunTest(const FString&)
{
	// Il corpus vero. Uno scenario sintetico copre i campi che ho pensato di coprire; i file versionati
	// coprono quelli che qualcun altro ha usato — ed e' li' che un writer perde un campo senza dirlo.
	TArray<FString> Problems;
	const TArray<FRTScenarioEntry> Entries = URTScenarioIndex::Scan(Problems);

	if (Entries.Num() == 0)
	{
		AddWarning(TEXT("nessuno scenario trovato sotto Scenarios/: il round-trip sul corpus non ha girato"));
		return true;
	}

	int32 Checked = 0;
	int32 NotesSeen = 0;
	for (const FRTScenarioEntry& Entry : Entries)
	{
		FRTTestScenario Original;
		FString Error;
		if (!URTScenarioLoader::LoadFromFile(Entry.Path, Original, Error))
		{
			// Non e' un difetto del writer: lo copre gia' il test del loader sul corpus.
			continue;
		}

		FString Json;
		if (!URTScenarioLoader::SaveToString(Original, Json, Error))
		{
			AddError(FString::Printf(TEXT("'%s' si carica ma non si riscrive: %s"), *Entry.ScenarioId, *Error));
			continue;
		}

		FRTTestScenario Reloaded;
		if (!URTScenarioLoader::LoadFromString(Json, Reloaded, Error))
		{
			AddError(FString::Printf(TEXT("'%s': il writer ha prodotto un file che il loader rifiuta — %s"),
				*Entry.ScenarioId, *Error));
			continue;
		}

		FString Diff;
		if (!ScenariosEquivalent(Original, Reloaded, Diff))
		{
			AddError(FString::Printf(TEXT("'%s' cambia nel round-trip, primo campo divergente: %s"),
				*Entry.ScenarioId, *Diff));
			continue;
		}

		// LE NOTE `_*`, contate sul FILE e non sul modello — `#3118`.
		//
		// 🔑 Il confronto qui sopra verifica che le note del modello tornino identiche; non puo' accorgersi di
		// una nota che nel modello non e' MAI entrata, perche' allora i due lati sono vuoti entrambi ed e'
		// verde. Questo conta cio' che il file dichiara: se qualcuno scrive un `_nota` a un livello che la
		// raccolta non copre — dentro `variants`, dentro `statuses` — il corpus diventa rosso e il messaggio
		// nomina il percorso, invece di perdere la nota al primo salvataggio dal pannello.
		FString FileText;
		TArray<FString> FileNotePaths;
		if (FFileHelper::LoadFileToString(FileText, *Entry.Path)
			&& CollectJsonNotePathsFromText(FileText, FileNotePaths))
		{
			const int32 InModel = CountModelNotes(Original);
			if (FileNotePaths.Num() != InModel)
			{
				AddError(FString::Printf(
					TEXT("'%s': il file dichiara %d chiavi '_*' e il modello ne porta %d — una sede del ")
					TEXT("formato non e' coperta dalla raccolta, e un salvataggio le cancellerebbe. ")
					TEXT("Percorsi nel file: %s"),
					*Entry.ScenarioId, FileNotePaths.Num(), InModel,
					*FString::Join(FileNotePaths, TEXT(" · "))));
			}
			NotesSeen += InModel;
		}

		// I tag li legge l'INDICE, non il loader: e' l'unico modo di verificare che sopravvivano davvero
		// nella forma che il filtro usa.
		FString HeaderId;
		TArray<FString> HeaderTags;
		FString HeaderError;
		if (URTScenarioIndex::ReadHeader(Json, HeaderId, HeaderTags, HeaderError))
		{
			if (HeaderId != Entry.ScenarioId)
			{
				AddError(FString::Printf(TEXT("'%s': lo scenarioId cambia nel round-trip (ora '%s')"),
					*Entry.ScenarioId, *HeaderId));
			}
			if (HeaderTags != Entry.Tags)
			{
				AddError(FString::Printf(TEXT("'%s': i tag cambiano nel round-trip — [%s] diventa [%s]"),
					*Entry.ScenarioId, *FString::Join(Entry.Tags, TEXT(",")), *FString::Join(HeaderTags, TEXT(","))));
			}
		}
		else
		{
			AddError(FString::Printf(TEXT("'%s': l'indice non sa piu' leggere l'intestazione — %s"),
				*Entry.ScenarioId, *HeaderError));
		}

		++Checked;
	}

	AddInfo(FString::Printf(TEXT("round-trip verificato su %d scenari versionati, %d chiavi '_*' preservate"),
		Checked, NotesSeen));

	// ⚠️ Un corpus senza note renderebbe VACUO il controllo qui sopra: zero contro zero e' verde. La soglia e'
	// deliberatamente bassa — non e' un conteggio da tenere aggiornato, e' la prova che il gate ha visto
	// qualcosa. Il corpus del 2026-09-20 ne porta 900.
	if (Checked > 0 && NotesSeen == 0)
	{
		AddError(TEXT("nessuna chiave '_*' vista nel corpus: il controllo sulle note e' vacuo, non verde"));
	}
	return true;
}

// --- la documentazione incorporata (`#3118`) --------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioWriterKeepsNotesTest,
	"RefactorTactics.Scenario.WriterKeepsEmbeddedNotes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioWriterKeepsNotesTest::RunTest(const FString&)
{
	// Uno scenario che porta una nota a OGNI sede che il corpus usa, e che di proposito NON dichiara
	// `mapRadius`: sono le due meta' di `#3118`, il campo perso e il campo inventato.
	//
	// ⚠️ Le chiavi `_*` sono scritte in ordine NON alfabetico apposta — `_zeta` prima di `_alfa` — perche'
	// l'ordine che il modello restituisce e' dichiarato, non quello del file: `FJsonObject::Values` e' una
	// `TMap` e l'ordine originale il parsing lo ha gia' perso. Un test scritto in ordine alfabetico non
	// avrebbe potuto distinguere «ordinato» da «per caso».
	const TCHAR* WithNotes = TEXT(R"JSON(
	{
	  "scenarioId": "Movement.WriterKeepsNotes",
	  "version": 1,
	  "tags": ["movement", "note"],
	  "_zeta": "L'ultima in ordine alfabetico, scritta per prima nel file.",
	  "_oggetto": { "perche": "Una nota puo' essere un OGGETTO, e sei lo sono nel corpus.",
	                "_annidata": "E puo' contenerne un'altra." },
	  "_alfa": "La prima in ordine alfabetico, scritta per ultima nel file.",
	  "cells": [
	    { "_cella": "Perche' questa cella e' chiusa.", "cell": [1, 0, 0], "blocksMovement": true }
	  ],
	  "doors": [
	    { "_perche": "Aperta per costruzione.", "cell": [0, 0, 0], "edge": "E", "doorId": 1, "state": "Open" }
	  ],
	  "units": [
	    { "_unita": "Chi la porta, e perche' proprio lei.", "id": "A1", "hero": "Hero.Aevik", "team": 0,
	      "cell": [-2, 0, 0] },
	    { "id": "B1", "hero": "Hero.Branth", "team": 1, "cell": [2, 0, 0] }
	  ],
	  "turns": [
	    { "_turno": "T1 — un passo solo, e basta cosi'.",
	      "intents": [ { "_intento": "Perche' questo passo e non un altro.", "unit": "A1",
	                     "move": [[-1, 0, 0]] } ] }
	  ],
	  "expect": [
	    { "_assertion": "Cosa prova davvero, e cosa no.", "type": "UnitAtCell", "unit": "A1",
	      "cell": [-1, 0, 0] }
	  ]
	}
	)JSON");

	FRTTestScenario Original;
	FString Error;
	if (!TestTrue(FString::Printf(TEXT("lo scenario con le note si carica (%s)"), *Error),
		URTScenarioLoader::LoadFromString(WithNotes, Original, Error)))
	{
		return false;
	}

	// --- il file -> il modello ---------------------------------------------------------------------------
	TestEqual(TEXT("note di radice raccolte"), Original.Notes.Num(), 3);
	if (Original.Notes.Num() == 3)
	{
		// ORDINE DICHIARATO: alfabetico, non quello del file — che era `_zeta`, `_oggetto`, `_alfa`.
		TestEqual(TEXT("prima nota di radice"), Original.Notes[0].Key, TEXT("_alfa"));
		TestEqual(TEXT("seconda nota di radice"), Original.Notes[1].Key, TEXT("_oggetto"));
		TestEqual(TEXT("terza nota di radice"), Original.Notes[2].Key, TEXT("_zeta"));
		// La nota d'OGGETTO si conserva in forma canonica, con le chiavi interne ordinate: `_annidata`
		// prima di `perche`. E' cio' che rende due salvataggi identici.
		TestEqual(TEXT("la nota d'oggetto e' canonica e ordinata"), Original.Notes[1].RawJson,
			TEXT("{\"_annidata\":\"E puo' contenerne un'altra.\",")
			TEXT("\"perche\":\"Una nota puo' essere un OGGETTO, e sei lo sono nel corpus.\"}"));
	}

	TestEqual(TEXT("note raccolte su ogni sede"), CountModelNotes(Original), 9);

	// --- `mapRadius` non dichiarato non si materializza ---------------------------------------------------
	TestFalse(TEXT("il file non dichiarava mapRadius"), Original.bHasMapRadius);
	TestEqual(TEXT("e il loader gli applica il default"), Original.MapRadius,
		FRTTestScenario::DefaultMapRadius);

	FString Json;
	if (!TestTrue(FString::Printf(TEXT("lo scenario si riscrive (%s)"), *Error),
		URTScenarioLoader::SaveToString(Original, Json, Error)))
	{
		return false;
	}

	// 🔑 **La meta' che nessun confronto semantico coglie**: `mapRadius` al default rilegge uguale, quindi
	// scriverlo o ometterlo da' lo stesso modello. La differenza sta nel FILE — una chiave comparsa dal nulla
	// che contraddice cio' che il file dice di se' — e si asserisce sul testo.
	TestFalse(TEXT("il salvataggio non inventa 'mapRadius'"), Json.Contains(TEXT("\"mapRadius\"")));

	// --- il modello -> il file -> il modello -------------------------------------------------------------
	FRTTestScenario Reloaded;
	if (!TestTrue(FString::Printf(TEXT("il file riscritto si rilegge (%s)"), *Error),
		URTScenarioLoader::LoadFromString(Json, Reloaded, Error)))
	{
		AddError(FString::Printf(TEXT("--- JSON ---\n%s"), *Json));
		return false;
	}

	FString Diff;
	if (!TestTrue(FString::Printf(TEXT("il round-trip conserva tutto (primo divergente: %s)"), *Diff),
		ScenariosEquivalent(Original, Reloaded, Diff)))
	{
		AddError(FString::Printf(TEXT("--- JSON ---\n%s"), *Json));
		return false;
	}

	TestEqual(TEXT("nessuna nota persa nel round-trip"), CountModelNotes(Reloaded), 9);

	// Il testo di una nota si asserisce per intero, non per presenza della chiave: una nota troncata
	// supererebbe un controllo che guarda solo i nomi, e la prosa e' precisamente cio' che non si riscrive.
	if (Reloaded.Turns.Num() == 1 && Reloaded.Turns[0].Notes.Num() == 1)
	{
		TestEqual(TEXT("la nota del turno torna col suo testo"), Reloaded.Turns[0].Notes[0].RawJson,
			TEXT("\"T1 — un passo solo, e basta cosi'.\""));
	}
	else
	{
		AddError(TEXT("la nota del turno non e' tornata"));
	}

	// La porta: il writer la perdeva per intero, non solo la sua nota.
	TestEqual(TEXT("la porta sopravvive al round-trip"), Reloaded.Doors.Num(), 1);

	// --- idempotenza: due salvataggi danno lo stesso testo ------------------------------------------------
	// E' l'invariante 1 del writer applicata alle note: se l'ordine venisse da una `TMap`, questo cadrebbe.
	FString Json2;
	if (URTScenarioLoader::SaveToString(Reloaded, Json2, Error))
	{
		TestEqual(TEXT("due salvataggi producono lo stesso testo"), Json2, Json);
	}
	else
	{
		AddError(FString::Printf(TEXT("il secondo salvataggio fallisce: %s"), *Error));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
