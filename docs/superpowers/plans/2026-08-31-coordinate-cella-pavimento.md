# Coordinate della cella sul pavimento — piano di implementazione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ogni cella della mappa aperta nell'editor mostra sul pavimento la propria terna `(x, y, layer)`, in tre direzioni, senza che si attivi nulla.

**Architecture:** Due pezzi separati. Nel modulo runtime `RTHexLabelLibrary` decide *dove cade ogni carattere* — geometria pura, nessun `UWorld`, nessun Actor, tutta testabile. Nel modulo editor un `ULineBatchComponent` sull'`ARTHexMapActor` consuma quelle pose e traccia i segmenti; si ripopola quando la mappa cambia, non a ogni frame.

**Tech Stack:** C++ / Unreal Engine 5.8.1 · `IMPLEMENT_SIMPLE_AUTOMATION_TEST` · `ULineBatchComponent` (`Engine/Classes/Components/LineBatchComponent.h`) · `./scripts/rt-suite.ps1` per la misura.

**Spec:** `docs/superpowers/specs/2026-08-31-coordinate-cella-pavimento-design.md`

**Issue:** #1920 · sub-issue di #1861

## Global Constraints

- **Unreal Engine 5.8.1.** L'engine sta in `D:\EpicGames\UE_5.8`.
- **La geometria dell'esagono ha un'unica autorità**: `URTHexLibrary::CellCorners(Cell, Origin, HexSize, LayerHeight)`. I punti medi dei lati si **derivano** da quei vertici, mai da una seconda formula.
- **Convenzione pointy-top**: `HexCorners` mette il primo vertice a `-30°`; i vertici cadono a `-30/30/90/150/210/270`, i punti medi dei lati a `0/60/120/180/240/300`.
- **Le tre direzioni sono `0° / 120° / 240°`** — punti medi di lati alternati, **non vertici**.
- **La terna è `(x, y, layer)`**, disposta **dal bordo verso il centro**; il `layer` a **metà** della scala di `x` e `y`.
- **Nessun segmento esce dall'esagono.** Caso peggiore da prevedere: `-10,-10,1`, dieci caratteri.
- **Nessun asset nuovo nel gray kit**, nessun ottavo `InstancedStaticMeshComponent`, nessun interruttore.
- **Commenti implementativi in italiano**, come il resto del repository. I nomi dei test in inglese, nel namespace `RefactorTactics.<Area>.<Nome>`.
- **Build**: `Build.bat RefactorTacticsEditor Win64 Development -Project=<uproject> -WaitMutex -NoHotReloadFromIDE`.
- **Misura**: `./scripts/rt-suite.ps1 -Filter "<filtro>" -WaitMinutes 40`. Se il motore è occupato lo script attende: non aggirarlo lanciando a mano senza dichiararlo.

---

## Struttura dei file

| File | Responsabilità |
|---|---|
| `Source/RefactorTactics/Map/RTHexLabel.h` | I tre tipi di dato: `FRTLabelStroke`, `FRTLabelGlyph`, `FRTCellLabel`. Nessuna funzione. |
| `Source/RefactorTactics/Map/RTHexLabelLibrary.h/.cpp` | `GlyphStrokes` (forma dei caratteri) e `BuildCellLabel` (dove cadono). Pure. |
| `Source/RefactorTactics/Tests/RTHexLabelTests.cpp` | I test delle due funzioni. |
| `Source/RefactorTactics/Map/RTHexMapActor.h/.cpp` | Il `ULineBatchComponent` e la funzione che lo ripopola, sotto `WITH_EDITOR`. |
| `docs/technical/test-manuali-pie.md` | La voce `PIE-*` per il giudizio di leggibilità. |
| `docs/roadmap/editor-sessions.yaml` | La seduta che la ospita. |

Task 1 e 2 stanno nel runtime e sono interamente misurabili. Task 3 è il guscio, che nessun automation test vede. Task 4 apre la verifica a schermo.

---

### Task 1: il font a segmenti

**Files:**
- Create: `Source/RefactorTactics/Map/RTHexLabel.h`
- Create: `Source/RefactorTactics/Map/RTHexLabelLibrary.h`, `Source/RefactorTactics/Map/RTHexLabelLibrary.cpp`
- Test: `Source/RefactorTactics/Tests/RTHexLabelTests.cpp`

**Interfaces:**
- Consumes: niente.
- Produces: `struct FRTLabelStroke { FVector2D From; FVector2D To; };` e
  `static TArray<FRTLabelStroke> URTHexLabelLibrary::GlyphStrokes(TCHAR Character);`
  Il set di caratteri è chiuso: `'0'`–`'9'`, `','`, `'-'`. Ogni coordinata sta in `[0,1]`.

- [ ] **Step 1: scrivi il test che fallisce**

In `Source/RefactorTactics/Tests/RTHexLabelTests.cpp`:

```cpp
#include "Misc/AutomationTest.h"
#include "Map/RTHexLabel.h"
#include "Map/RTHexLabelLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il font a segmenti delle coordinate (#1920).
 *
 * ⛔ Non e' un font: e' il set CHIUSO di dodici caratteri che una terna di coordinate puo' contenere. Un
 * font vero e' l'approccio C della spec, e questa architettura lo lascia possibile senza anticiparlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelGlyphSetTest,
	"RefactorTactics.HexLabel.GlyphSetIsClosedAndNormalised",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelGlyphSetTest::RunTest(const FString&)
{
	const FString Alphabet = TEXT("0123456789,-");

	for (const TCHAR Ch : Alphabet)
	{
		const TArray<FRTLabelStroke> Strokes = URTHexLabelLibrary::GlyphStrokes(Ch);
		TestTrue(FString::Printf(TEXT("'%c' ha almeno un segmento"), Ch), Strokes.Num() > 0);

		for (const FRTLabelStroke& S : Strokes)
		{
			const bool bInside =
				S.From.X >= 0.f && S.From.X <= 1.f && S.From.Y >= 0.f && S.From.Y <= 1.f &&
				S.To.X   >= 0.f && S.To.X   <= 1.f && S.To.Y   >= 0.f && S.To.Y   <= 1.f;
			TestTrue(FString::Printf(TEXT("'%c' resta nel quadrato unitario"), Ch), bInside);
		}
	}

	// 🔴 Un carattere fuori set NON produce segmenti inventati: meglio niente che un glifo che nessuno
	// ha disegnato, e che a schermo sembrerebbe una cifra sbagliata invece che un dato mancante.
	TestEqual(TEXT("un carattere fuori set non disegna nulla"),
		URTHexLabelLibrary::GlyphStrokes(TEXT('Z')).Num(), 0);
	return true;
}

/**
 * Le dieci cifre sono DISTINTE fra loro. Senza questo, un font in cui `6` e `8` condividono i segmenti
 * passerebbe ogni altro test di questo file: sono entrambi non vuoti e dentro il quadrato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelDigitsDifferTest,
	"RefactorTactics.HexLabel.EveryDigitLooksDifferent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelDigitsDifferTest::RunTest(const FString&)
{
	TSet<FString> Shapes;
	for (int32 D = 0; D <= 9; ++D)
	{
		FString Shape;
		for (const FRTLabelStroke& S : URTHexLabelLibrary::GlyphStrokes(TEXT('0') + D))
		{
			Shape += FString::Printf(TEXT("%.2f,%.2f-%.2f,%.2f;"), S.From.X, S.From.Y, S.To.X, S.To.Y);
		}
		Shapes.Add(Shape);
	}
	TestEqual(TEXT("dieci cifre, dieci forme diverse"), Shapes.Num(), 10);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 2: crea i tipi e lo stub, perché il test compili e FALLISCA**

`Source/RefactorTactics/Map/RTHexLabel.h`:

```cpp
#pragma once

#include "CoreMinimal.h"

/**
 * Un segmento di un carattere, in coordinate NORMALIZZATE dentro il quadrato unitario.
 *
 * 🔑 Normalizzate e non in centimetri: la forma del carattere non sa quanto sara' grande, e chi la posa
 * non sa che forma ha. E' il confine che permette di cambiare il disegnatore senza toccare la geometria.
 */
struct FRTLabelStroke
{
	FVector2D From = FVector2D::ZeroVector;
	FVector2D To   = FVector2D::ZeroVector;
};
```

`Source/RefactorTactics/Map/RTHexLabelLibrary.h`:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTHexLabel.h"

#include "RTHexLabelLibrary.generated.h"

/**
 * Le coordinate della cella incise sul pavimento (#1920): dove cade ogni carattere, e che forma ha.
 *
 * ⛔ **Non disegna niente.** Restituisce pose; chi le traccia e' il guscio d'editor. Se un giorno questo
 * file includesse un componente o un PDI, sarebbe la presentazione entrata nella regola.
 */
UCLASS()
class REFACTORTACTICS_API URTHexLabelLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * I segmenti che disegnano un carattere dentro il quadrato unitario.
	 *
	 * Set CHIUSO: `0`-`9`, `,` e `-`. Un carattere fuori set restituisce un array vuoto — meglio niente
	 * che un glifo inventato, che a schermo sembrerebbe una cifra sbagliata invece che un dato mancante.
	 */
	static TArray<FRTLabelStroke> GlyphStrokes(TCHAR Character);
};
```

`Source/RefactorTactics/Map/RTHexLabelLibrary.cpp`:

```cpp
#include "Map/RTHexLabelLibrary.h"

TArray<FRTLabelStroke> URTHexLabelLibrary::GlyphStrokes(TCHAR Character)
{
	return {};
}
```

- [ ] **Step 3: compila e osserva il rosso**

```powershell
& "D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development `
  -Project="<repo>\RefactorTactics.uproject" -WaitMutex -NoHotReloadFromIDE
./scripts/rt-suite.ps1 -Filter "RefactorTactics.HexLabel" -WaitMinutes 40
```

Atteso: **2 test, 2 falliti**. `GlyphSetIsClosedAndNormalised` cade su *«'0' ha almeno un segmento»*; `EveryDigitLooksDifferent` su *«dieci cifre, dieci forme diverse»* (ne trova 1: tutte vuote).

⚠️ Se cade con un errore di compilazione invece che con quelle asserzioni, non è il rosso giusto: sistema la compilazione e rilancia.

- [ ] **Step 4: implementa il font**

Sostituisci il corpo in `RTHexLabelLibrary.cpp`. Le cifre sono a **sette segmenti** — la forma da display digitale — perché è il minimo che distingue dieci cifre e resta leggibile a pochi pixel:

```cpp
#include "Map/RTHexLabelLibrary.h"

namespace
{
	/**
	 * I sette segmenti, nel quadrato unitario. Nomi da display digitale: A in alto, poi in senso orario,
	 * G di mezzo.
	 *
	 *      A          (0,1) ---- (1,1)
	 *    F   B          |    G     |
	 *      G          (0,.5) --- (1,.5)
	 *    E   C          |          |
	 *      D          (0,0) ---- (1,0)
	 */
	const FRTLabelStroke SegA{ FVector2D(0.f, 1.f),  FVector2D(1.f, 1.f)  };
	const FRTLabelStroke SegB{ FVector2D(1.f, 1.f),  FVector2D(1.f, 0.5f) };
	const FRTLabelStroke SegC{ FVector2D(1.f, 0.5f), FVector2D(1.f, 0.f)  };
	const FRTLabelStroke SegD{ FVector2D(0.f, 0.f),  FVector2D(1.f, 0.f)  };
	const FRTLabelStroke SegE{ FVector2D(0.f, 0.5f), FVector2D(0.f, 0.f)  };
	const FRTLabelStroke SegF{ FVector2D(0.f, 1.f),  FVector2D(0.f, 0.5f) };
	const FRTLabelStroke SegG{ FVector2D(0.f, 0.5f), FVector2D(1.f, 0.5f) };
}

TArray<FRTLabelStroke> URTHexLabelLibrary::GlyphStrokes(TCHAR Character)
{
	switch (Character)
	{
	case TEXT('0'): return { SegA, SegB, SegC, SegD, SegE, SegF };
	case TEXT('1'): return { SegB, SegC };
	case TEXT('2'): return { SegA, SegB, SegG, SegE, SegD };
	case TEXT('3'): return { SegA, SegB, SegG, SegC, SegD };
	case TEXT('4'): return { SegF, SegG, SegB, SegC };
	case TEXT('5'): return { SegA, SegF, SegG, SegC, SegD };
	case TEXT('6'): return { SegA, SegF, SegG, SegE, SegD, SegC };
	case TEXT('7'): return { SegA, SegB, SegC };
	case TEXT('8'): return { SegA, SegB, SegC, SegD, SegE, SegF, SegG };
	case TEXT('9'): return { SegA, SegB, SegC, SegD, SegF, SegG };

	// Il meno e' il segmento di mezzo: stessa altezza della barra del `4`, quindi non si confonde con
	// una cifra e si legge alla stessa quota.
	case TEXT('-'): return { SegG };

	// La virgola vive nella meta' bassa e sporge a sinistra: senza la coda si leggerebbe come un punto,
	// e la terna `0.0.0` non e' la terna `0,0,0`.
	case TEXT(','): return {
		FRTLabelStroke{ FVector2D(0.45f, 0.2f), FVector2D(0.35f, 0.f) } };

	default:
		// ⛔ Nessun glifo di ripiego: vedi la doc dell'header.
		return {};
	}
}
```

- [ ] **Step 5: compila, rilancia, verifica il verde**

Atteso: **2/2 passati**.

- [ ] **Step 6: commit**

```bash
git add Source/RefactorTactics/Map/RTHexLabel.h Source/RefactorTactics/Map/RTHexLabelLibrary.h \
        Source/RefactorTactics/Map/RTHexLabelLibrary.cpp Source/RefactorTactics/Tests/RTHexLabelTests.cpp
git commit -m "feat(1920): il set chiuso dei caratteri della terna, e la forma che li distingue"
```

---

### Task 2: il layout dentro l'esagono

**Files:**
- Modify: `Source/RefactorTactics/Map/RTHexLabel.h` (aggiunge due struct)
- Modify: `Source/RefactorTactics/Map/RTHexLabelLibrary.h/.cpp` (aggiunge `BuildCellLabel`)
- Test: `Source/RefactorTactics/Tests/RTHexLabelTests.cpp` (aggiunge i test del layout)

**Interfaces:**
- Consumes: `FRTLabelStroke` e `GlyphStrokes` dal Task 1. `URTHexLibrary::CellCorners(const FRTCellId&, const FVector&, float, float)` dal runtime esistente.
- Produces:
  ```cpp
  struct FRTLabelGlyph { TCHAR Character; FVector Origin; FVector Right; FVector Up; };
  struct FRTCellLabel  { TArray<FRTLabelGlyph> Glyphs; };
  static FRTCellLabel URTHexLabelLibrary::BuildCellLabel(
      const FRTCellId& Cell, const FVector& Origin, float HexSize, float LayerHeight);
  ```
  `Origin` è l'angolo in basso a sinistra del quadrato del carattere, nel mondo; `Right` e `Up` sono i due
  assi **già scalati**, così il consumatore ottiene il punto mondo di uno stroke con
  `Glyph.Origin + Glyph.Right * S.From.X + Glyph.Up * S.From.Y`.

- [ ] **Step 1: scrivi i test che falliscono**

Aggiungi in `RTHexLabelTests.cpp`, prima di `#endif`:

```cpp
namespace
{
	/** Il punto mondo di un estremo di segmento. Un'unica formula, usata da ogni test di questo file. */
	FVector StrokePoint(const FRTLabelGlyph& G, const FVector2D& Local)
	{
		return G.Origin + G.Right * Local.X + G.Up * Local.Y;
	}

	/** Dentro il poligono convesso, sul piano XY. I sei vertici arrivano in ordine da `CellCorners`. */
	bool InsideHex(const TArray<FVector>& Corners, const FVector& P)
	{
		for (int32 I = 0; I < Corners.Num(); ++I)
		{
			const FVector& A = Corners[I];
			const FVector& B = Corners[(I + 1) % Corners.Num()];
			const double Cross = (B.X - A.X) * (P.Y - A.Y) - (B.Y - A.Y) * (P.X - A.X);
			if (Cross < -0.01) // tolleranza: il confronto e' su centimetri, non su bit
			{
				return false;
			}
		}
		return true;
	}
}

/**
 * 🔴 **Il test che DIMENSIONA le cifre.** Nessun segmento esce dall'esagono — e il caso e' il peggiore
 * possibile, non `(0,0,0)`: `-10,-10,1` sono dieci caratteri, e una taratura fatta sulla terna corta
 * sborda alla prima mappa grande senza che nessuno se ne accorga prima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelStaysInsideTest,
	"RefactorTactics.HexLabel.NothingLeavesTheHexagon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelStaysInsideTest::RunTest(const FString&)
{
	constexpr float HexSize = 150.f;    // il default di `URTHexMapAsset`
	constexpr float LayerHeight = 250.f;
	const FVector MapOrigin = FVector::ZeroVector;

	// La terna piu' lunga che una mappa possa produrre: dieci caratteri, due segni meno.
	const FRTCellId Worst(-10, -10, 1);
	const FRTCellLabel Label = URTHexLabelLibrary::BuildCellLabel(Worst, MapOrigin, HexSize, LayerHeight);
	const TArray<FVector> Corners = URTHexLibrary::CellCorners(Worst, MapOrigin, HexSize, LayerHeight);

	TestTrue(TEXT("l'etichetta non e' vuota"), Label.Glyphs.Num() > 0);

	int32 Checked = 0;
	for (const FRTLabelGlyph& G : Label.Glyphs)
	{
		for (const FRTLabelStroke& S : URTHexLabelLibrary::GlyphStrokes(G.Character))
		{
			TestTrue(TEXT("l'inizio del segmento e' dentro l'esagono"), InsideHex(Corners, StrokePoint(G, S.From)));
			TestTrue(TEXT("la fine del segmento e' dentro l'esagono"),  InsideHex(Corners, StrokePoint(G, S.To)));
			++Checked;
		}
	}
	TestTrue(TEXT("qualche segmento e' stato davvero controllato"), Checked > 0);
	return true;
}

/**
 * Tre run a 120 gradi esatti, verificato sulle POSE e non a occhio: e' la differenza fra «sembrano
 * ruotate» e «sono ruotate».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelThreeDirectionsTest,
	"RefactorTactics.HexLabel.ThreeRunsAtOneHundredTwentyDegrees",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelThreeDirectionsTest::RunTest(const FString&)
{
	const FRTCellId Cell(0, 0, 0);
	const FRTCellLabel Label = URTHexLabelLibrary::BuildCellLabel(Cell, FVector::ZeroVector, 150.f, 250.f);

	// Gli angoli distinti dell'asse `Right`, arrotondati al grado: devono essere tre.
	TSet<int32> Angles;
	for (const FRTLabelGlyph& G : Label.Glyphs)
	{
		const double Deg = FMath::RadiansToDegrees(FMath::Atan2(G.Right.Y, G.Right.X));
		Angles.Add(FMath::RoundToInt(((Deg < 0.0) ? Deg + 360.0 : Deg)));
	}

	TestEqual(TEXT("tre direzioni, non una ne' sei"), Angles.Num(), 3);

	TArray<int32> Sorted = Angles.Array();
	Sorted.Sort();
	TestEqual(TEXT("la prima e' a 0 gradi: il punto medio del primo lato"), Sorted[0], 0);
	TestEqual(TEXT("la seconda a 120"), Sorted[1], 120);
	TestEqual(TEXT("la terza a 240"),   Sorted[2], 240);
	return true;
}

/**
 * La terna corre DAL BORDO AL CENTRO, e il layer e' a META' scala. Due criteri della spec che, sbagliati,
 * danno un'etichetta plausibile: le cifre ci sono, stanno dentro, e dicono la cosa nell'ordine sbagliato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelOrderAndScaleTest,
	"RefactorTactics.HexLabel.RunsInwardAndLayerIsHalfSize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelOrderAndScaleTest::RunTest(const FString&)
{
	const FRTCellId Cell(2, -3, 0);
	const FVector MapOrigin = FVector::ZeroVector;
	const FRTCellLabel Label = URTHexLabelLibrary::BuildCellLabel(Cell, MapOrigin, 150.f, 250.f);
	const FVector Centre = URTHexLibrary::AxialToWorld(Cell, MapOrigin, 150.f, 250.f);

	// La run a 0 gradi, nell'ordine in cui e' stata costruita.
	TArray<FRTLabelGlyph> Run;
	for (const FRTLabelGlyph& G : Label.Glyphs)
	{
		if (FMath::IsNearlyZero(G.Right.Y, 0.01) && G.Right.X > 0.0)
		{
			Run.Add(G);
		}
	}
	TestTrue(TEXT("la run a 0 gradi ha almeno tre caratteri"), Run.Num() >= 3);

	const double FirstDist = FVector::Dist2D(Run[0].Origin, Centre);
	const double LastDist  = FVector::Dist2D(Run.Last().Origin, Centre);
	TestTrue(TEXT("il primo carattere e' piu' vicino al BORDO dell'ultimo"), FirstDist > LastDist);

	// L'ultimo carattere della terna e' il layer: alto meta' del primo.
	const double FirstHeight = Run[0].Up.Size();
	const double LastHeight  = Run.Last().Up.Size();
	TestTrue(TEXT("il layer e' alto meta' delle altre componenti"),
		FMath::IsNearlyEqual(LastHeight, FirstHeight * 0.5, 0.5));
	return true;
}

/**
 * Le coordinate negative si leggono: il segno meno e' nella terna, non sottinteso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelNegativeTest,
	"RefactorTactics.HexLabel.NegativeCoordinatesShowTheirSign",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelNegativeTest::RunTest(const FString&)
{
	const FRTCellLabel Negative = URTHexLabelLibrary::BuildCellLabel(
		FRTCellId(-2, 1, 0), FVector::ZeroVector, 150.f, 250.f);

	int32 Minus = 0;
	for (const FRTLabelGlyph& G : Negative.Glyphs) { if (G.Character == TEXT('-')) { ++Minus; } }

	// Una `x` negativa, tre direzioni: tre segni meno, uno per run.
	TestEqual(TEXT("un segno meno per ciascuna delle tre run"), Minus, 3);
	return true;
}
```

Aggiungi in cima al file gli include che i nuovi test usano:

```cpp
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"
```

- [ ] **Step 2: aggiungi i tipi e lo stub**

In `RTHexLabel.h`, dopo `FRTLabelStroke`:

```cpp
/**
 * Un carattere POSATO nel mondo: dove sta il suo quadrato unitario e quanto e' grande.
 *
 * `Right` e `Up` sono gia' scalati, quindi il consumatore ottiene il punto mondo di uno stroke con
 * `Origin + Right * S.X + Up * S.Y` — una moltiplicazione, nessuna trigonometria a valle. E' cio' che
 * permette a un secondo disegnatore (mesh, font atlas) di consumare le stesse pose.
 */
struct FRTLabelGlyph
{
	TCHAR   Character = TEXT(' ');
	FVector Origin    = FVector::ZeroVector;
	FVector Right     = FVector::ZeroVector;
	FVector Up        = FVector::ZeroVector;
};

/** Le tre run di una cella, tutte insieme: l'ordine e' run per run, carattere per carattere. */
struct FRTCellLabel
{
	TArray<FRTLabelGlyph> Glyphs;
};
```

In `RTHexLabelLibrary.h`, dopo `GlyphStrokes`:

```cpp
	/**
	 * Dove cade ogni carattere della terna `(x, y, layer)` di questa cella.
	 *
	 * Tre run a `0/120/240` gradi — i punti medi di tre lati alternati, **non i vertici**: con la
	 * convenzione pointy-top di `HexCorners` i vertici stanno a `-30/30/90/150/210/270`, e `0/120/240`
	 * cade sui punti medi. Ogni run corre **dal bordo verso il centro**, e il `layer` e' a meta' scala.
	 *
	 * ⚠️ I sei vertici li da' `URTHexLibrary::CellCorners`, e i punti medi si DERIVANO da quelli: una
	 * seconda formula per la stessa forma e' il difetto visto in `U22` — celle piene tonde e contorno
	 * esagonale.
	 */
	static FRTCellLabel BuildCellLabel(const FRTCellId& Cell, const FVector& Origin,
		float HexSize, float LayerHeight);
```

In `RTHexLabelLibrary.cpp` lo stub:

```cpp
FRTCellLabel URTHexLabelLibrary::BuildCellLabel(const FRTCellId& Cell, const FVector& Origin,
	float HexSize, float LayerHeight)
{
	return FRTCellLabel();
}
```

Aggiungi in cima al `.cpp`:

```cpp
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"
```

e in `RTHexLabelLibrary.h`:

```cpp
#include "Map/RTCellId.h"
```

- [ ] **Step 3: compila e osserva il rosso**

```powershell
./scripts/rt-suite.ps1 -Filter "RefactorTactics.HexLabel" -WaitMinutes 40
```

Atteso: i due test del Task 1 restano **verdi**, i quattro nuovi **falliscono** — su *«l'etichetta non e' vuota»*, *«tre direzioni, non una ne' sei»* (ne trova 0), *«la run a 0 gradi ha almeno tre caratteri»* e *«un segno meno per ciascuna delle tre run»*.

- [ ] **Step 4: implementa il layout**

```cpp
FRTCellLabel URTHexLabelLibrary::BuildCellLabel(const FRTCellId& Cell, const FVector& Origin,
	float HexSize, float LayerHeight)
{
	FRTCellLabel Out;

	const TArray<FVector> Corners = URTHexLibrary::CellCorners(Cell, Origin, HexSize, LayerHeight);
	if (Corners.Num() != 6)
	{
		return Out; // geometria inattesa: meglio nessuna etichetta che una posata su un'ipotesi
	}
	const FVector Centre = URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerHeight);

	// 🔑 I punti medi si DERIVANO dai vertici (vedi la doc): il lato `k` va da `Corners[k]` a
	// `Corners[k+1]`, e il suo punto medio cade a `60k` gradi. Prendendo i lati pari si ottengono
	// esattamente 0, 120 e 240.
	const FString Text = FString::Printf(TEXT("%d,%d,%d"), Cell.X, Cell.Y, Cell.Layer);
	const int32 LayerDigits = FString::FromInt(Cell.Layer).Len();

	// L'altezza piena e la larghezza di un carattere, tarate sull'APOTEMA e sul caso peggiore. Non sono
	// numeri scelti a occhio: sono il piu' grande valore per cui `NothingLeavesTheHexagon` resta verde
	// su `-10,-10,1`, cioe' dieci caratteri lungo una direzione.
	constexpr int32 WorstCaseChars = 10;
	const double Apothem   = HexSize * 0.8660254; // cos(30°)
	const double Margin    = HexSize * 0.06;      // il bordo non si tocca: la cella ha gia' un contorno
	const double Usable    = Apothem - Margin;
	const double CharWidth = Usable / WorstCaseChars;
	const double FullHeight = CharWidth * 1.4;    // rapporto di un display a sette segmenti

	for (int32 Side = 0; Side < 6; Side += 2)
	{
		const FVector Mid = (Corners[Side] + Corners[(Side + 1) % 6]) * 0.5;

		// Verso il CENTRO: la prima cifra sta al bordo e l'ultima al centro, come chiesto.
		const FVector Inward = (Centre - Mid).GetSafeNormal();
		const FVector Right  = -Inward;              // il testo si legge venendo dal bordo
		const FVector Up     = FVector::CrossProduct(FVector::UpVector, Right).GetSafeNormal();

		double Travelled = Margin;
		for (int32 I = 0; I < Text.Len(); ++I)
		{
			const TCHAR Ch = Text[I];

			// ⚠️ Il layer a META' scala: sono gli ULTIMI `LayerDigits` caratteri, non «tutto dopo la
			// seconda virgola» — contare le virgole rompeva su coordinate negative, dove il meno non e'
			// un separatore ma fa parte del numero.
			const bool bIsLayer = I >= Text.Len() - LayerDigits;
			const double Scale  = bIsLayer ? 0.5 : 1.0;

			const double W = CharWidth * Scale;
			const double H = FullHeight * Scale;

			// L'angolo in basso a sinistra: si parte dal bordo e si cammina verso il centro, e il
			// carattere e' centrato sull'asse della run.
			const FVector Base = Mid + Inward * (Travelled + W) + Up * (-H * 0.5);

			FRTLabelGlyph Glyph;
			Glyph.Character = Ch;
			Glyph.Origin    = Base;
			Glyph.Right     = Right * W;
			Glyph.Up        = Up * H;
			Out.Glyphs.Add(Glyph);

			Travelled += W * 1.15; // il 15% e' la spaziatura fra caratteri
		}
	}

	return Out;
}
```

- [ ] **Step 5: rilancia e verifica**

Atteso: **6/6 verdi**. Se `NothingLeavesTheHexagon` cade, **non allargare la tolleranza**: riduci `WorstCaseChars` o alza `Margin`. È il test che dimensiona le cifre, e ammorbidirlo toglie l'unica garanzia che la spec dà.

- [ ] **Step 6: verifica di mutazione**

Cambia `const bool bIsLayer = ...` in `const bool bIsLayer = false;`, ricompila, rilancia.
Atteso: cade **esattamente** `RunsInwardAndLayerIsHalfSize`, e nessun altro.
Poi **ripristina e ricompila**, e riverifica il verde: un binario mutato lasciato sul disco fa dichiarare verde codice che non esiste.

- [ ] **Step 7: commit**

```bash
git add Source/RefactorTactics/Map/RTHexLabel.h Source/RefactorTactics/Map/RTHexLabelLibrary.h \
        Source/RefactorTactics/Map/RTHexLabelLibrary.cpp Source/RefactorTactics/Tests/RTHexLabelTests.cpp
git commit -m "feat(1920): le tre run della terna, e il contenimento che ne dimensiona le cifre"
```

---

### Task 3: il guscio che le traccia

**Files:**
- Modify: `Source/RefactorTactics/Map/RTHexMapActor.h` (un componente e una funzione, sotto `WITH_EDITOR`)
- Modify: `Source/RefactorTactics/Map/RTHexMapActor.cpp`

**Interfaces:**
- Consumes: `URTHexLabelLibrary::BuildCellLabel` e `GlyphStrokes` dai Task 1-2.
- Produces: niente per i task successivi.

- [ ] **Step 1: dichiara il componente**

In `RTHexMapActor.h`, accanto alle altre famiglie di componenti:

```cpp
#if WITH_EDITOR
	/**
	 * Le coordinate incise sul pavimento (#1920). **Solo editor**, e non e' un ottavo ISM.
	 *
	 * 🔑 **Perche' un `ULineBatchComponent` e non un disegno per frame.** Le linee si posano una volta e
	 * restano finche' non si svuota il batch: si ripopola quando la mappa cambia, come `RebuildInstances`.
	 * Un `DrawDebugLine` per frame su tutte le celle sarebbe il difetto che #711 ha gia' pagato — un costo
	 * per evento che nessun test misura e che si scopre solo quando il viewport smette di seguire il mouse.
	 *
	 * ⚠️ Le sette famiglie di ISM sono canali di lettura DELLA PARTITA. Questo no: in una build di gioco
	 * il componente non esiste, e si verifica per assenza.
	 */
	UPROPERTY(VisibleAnywhere, Category = "RefactorTactics|HexMap")
	TObjectPtr<class ULineBatchComponent> CoordinateLabels;

	/** Ridisegna le terne di tutte le celle. Nessuna decisione: consuma `BuildCellLabel`. */
	void RebuildCoordinateLabels();
#endif
```

- [ ] **Step 2: costruisci il componente e popolalo**

In `RTHexMapActor.cpp`, nel costruttore, dopo gli altri componenti:

```cpp
#if WITH_EDITOR
	CoordinateLabels = CreateDefaultSubobject<ULineBatchComponent>(TEXT("CoordinateLabels"));
	CoordinateLabels->SetupAttachment(RootComponent);
	// Come le altre famiglie di lettura: non e' scenografia e non deve intercettare il raycast di
	// selezione, che valida il componente colpito.
	CoordinateLabels->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoordinateLabels->SetCastShadow(false);
#endif
```

e la funzione:

```cpp
#if WITH_EDITOR
void ARTHexMapActor::RebuildCoordinateLabels()
{
	if (!CoordinateLabels)
	{
		return;
	}
	CoordinateLabels->Flush();

	if (!MapAsset)
	{
		return; // nessuna mappa, nessuna coordinata: non si inventa una griglia
	}

	FVector MapOrigin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerHeight = 0.f;
	GetHexContext(MapOrigin, HexSize, LayerHeight);

	// Il colore del segno inciso, lo stesso registro dei glifi e dei bordi ([D-183]): le coordinate sono
	// un segno d'autore, non un canale di colore in piu'.
	// ⚠️ `FLinearColor`, non `FColor`: e' il tipo che `FBatchedLine` prende. Un `FColor` compilerebbe per
	// conversione implicita e passerebbe per lo spazio sbagliato.
	const FLinearColor Ink(0.08f, 0.08f, 0.08f, 1.f);
	constexpr float Lift = 2.0f;      // sopra il disco, per la ragione che `RTMapVisuals.h` documenta
	constexpr float Thickness = 1.0f;

	TArray<FBatchedLine> Lines;
	for (const FRTHexCellData& Cell : MapAsset->Cells)
	{
		const FRTCellLabel Label = URTHexLabelLibrary::BuildCellLabel(Cell.Id, MapOrigin, HexSize, LayerHeight);
		for (const FRTLabelGlyph& Glyph : Label.Glyphs)
		{
			for (const FRTLabelStroke& S : URTHexLabelLibrary::GlyphStrokes(Glyph.Character))
			{
				const FVector A = Glyph.Origin + Glyph.Right * S.From.X + Glyph.Up * S.From.Y + FVector(0, 0, Lift);
				const FVector B = Glyph.Origin + Glyph.Right * S.To.X   + Glyph.Up * S.To.Y   + FVector(0, 0, Lift);
				// Firma: (Start, End, FLinearColor, LifeTime, Thickness, DepthPriority).
				// `LifeTime` negativo = la linea resta finche' non si chiama `Flush()`, che e' il punto:
				// si posa quando la mappa cambia, non a ogni frame.
				Lines.Emplace(A, B, Ink, /*LifeTime*/ -1.f, Thickness, /*DepthPriority*/ uint8(SDPG_World));
			}
		}
	}
	CoordinateLabels->DrawLines(Lines);
}
#endif
```

Include da aggiungere in cima al `.cpp`:

```cpp
#if WITH_EDITOR
#include "Components/LineBatchComponent.h"
#include "Map/RTHexLabel.h"
#include "Map/RTHexLabelLibrary.h"
#endif
```

- [ ] **Step 3: chiamala dove la mappa cambia**

Trova `RebuildInstances()` in `RTHexMapActor.cpp` e aggiungi in coda al corpo:

```cpp
#if WITH_EDITOR
	// Le coordinate seguono la mappa: stesso innesco delle istanze, quindi nessuna regola di
	// invalidazione nuova da tenere allineata.
	RebuildCoordinateLabels();
#endif
```

- [ ] **Step 4: compila e verifica che nulla sia rotto**

```powershell
./scripts/rt-suite.ps1 -WaitMinutes 40
```

Atteso: la suite intera **verde**, con i sei test di `HexLabel` dentro.

⚠️ Se la suite era già rossa su qualcosa prima di questo task, **misura la baseline** (`git stash`, ricompila, rilancia) invece di attribuirsi il rosso o di scartarlo.

- [ ] **Step 5: verifica per assenza che in gioco non esista**

```bash
grep -n "CoordinateLabels" Source/RefactorTactics/Map/RTHexMapActor.h Source/RefactorTactics/Map/RTHexMapActor.cpp
```

Atteso: ogni occorrenza è dentro un blocco `#if WITH_EDITOR`. Nessun riferimento fuori.

- [ ] **Step 6: commit**

```bash
git add Source/RefactorTactics/Map/RTHexMapActor.h Source/RefactorTactics/Map/RTHexMapActor.cpp
git commit -m "feat(1920): il guscio d'editor traccia le terne, e si ripopola quando la mappa cambia"
```

---

### Task 4: la verifica che nessun test può dare

**Files:**
- Modify: `docs/technical/test-manuali-pie.md`
- Modify: `docs/roadmap/editor-sessions.yaml`

**Interfaces:**
- Consumes: il comportamento dei Task 1-3.
- Produces: niente per i task successivi.

- [ ] **Step 1: apri la voce**

In `docs/technical/test-manuali-pie.md`, crea una sezione nuova prima di `### Scenario Test Harness`:

```markdown
### Le coordinate sul pavimento (#1920, aggiunte il 2026-08-31)

> **Una voce, e nessun test può darla**: che una cifra a sette segmenti si legga su una cella è un
> giudizio d'autore. Il contenimento nell'esagono, le tre direzioni e la scala del layer sono misurati da
> `RefactorTactics.HexLabel.*`; **quanto siano leggibili** no.
>
> **Precondizione**: Editor aperto su `L_DevSandbox` illuminato da `U21`, un `ARTHexMapActor` con asset.
> Nessun tool da attivare: le coordinate ci sono appena la mappa è aperta.

| ID | Cosa verificare | Precondizione | Esito atteso | Stato |
|----|-----------------|---------------|--------------|-------|
| **PIE-HEX-COORD-LEGGIBILITA** | La terna si legge sulla cella, da tre lati | una mappa con celle su due layer | Girando attorno alla cella, **almeno una** delle tre terne è dritta e leggibile da dove si guarda. Le cifre non si confondono fra loro — `6` e `8`, `1` e `7` — e il `layer` si distingue da `x` e `y` per la metà scala invece di sembrare una cifra staccata. ⚠️ La riserva: che la terna non competa con il glifo di superficie e col bordo della cella, che occupano lo stesso pavimento | ⏳ |
| **PIE-HEX-COORD-COSTO** | Il viewport non rallenta su una mappa grande | una mappa da almeno 200 celle | Navigando la mappa il viewport resta fluido. ⚠️ È la voce che guarda il rischio §8 della spec: le linee si posano una volta e non per frame, ma **quante siano** non lo dice nessun test — 200 celle sono ~6000 segmenti | ⏳ |
```

- [ ] **Step 2: ricalcola il conteggio canonico**

```bash
awk -F'|' '/^\| \*\*PIE-/ {s=$(NF-1); if (match(s, /✅|🟡|❌|⏳/)) c[substr(s, RSTART, RLENGTH)]++; else c["nessuno"]++ }
  END {printf "totale=%d verde=%d parziale=%d fallita=%d aperta=%d\n", c["✅"]+c["🟡"]+c["❌"]+c["⏳"]+c["nessuno"], c["✅"], c["🟡"], c["❌"], c["⏳"]}' \
  docs/technical/test-manuali-pie.md
```

Il totale deve salire **di due**, e le aperte di due. Riporta i numeri nel messaggio di commit: la regola di quel file è che il conteggio si ricalcola, non si aggiorna a mente.

- [ ] **Step 3: colloca le voci in una seduta**

In `docs/roadmap/editor-sessions.yaml`, aggiungi le due voci a `verifies` della seduta d'editor esistente più adatta, oppure aprine una nuova con un `id` **misurato** come massimo su `main` **e su tutti i branch remoti** più uno.

⚠️ *«Una voce che non sta in una seduta non viene eseguita mai»* — è la ragione per cui `U26` esiste.

- [ ] **Step 4: valida il YAML**

```bash
python -c "import yaml,io; yaml.safe_load(io.open('docs/roadmap/editor-sessions.yaml',encoding='utf-8')); print('YAML valido')"
```

- [ ] **Step 5: commit**

```bash
git add docs/technical/test-manuali-pie.md docs/roadmap/editor-sessions.yaml
git commit -m "docs(1920): due voci PIE per cio' che nessun test misura, e la seduta che le ospita"
```

---

## Chiusura

- [ ] `./scripts/rt-suite.ps1 -WaitMinutes 40` sul commit che verrà davvero mergiato, **dopo** aver portato `origin/main` dentro il branch.
- [ ] PR verso `main` (il parent dichiarato di questo branch), con il referto di misura nel corpo.
- [ ] Aggiorna #1920: quali criteri sono soddisfatti e quali restano alle voci PIE.
- [ ] ⛔ **Non chiudere #1920 al merge**: i suoi acceptance criteria includono la voce PIE aperta e collocata — che questo piano fa — ma la leggibilità resta un verdetto d'autore da dare in seduta.
