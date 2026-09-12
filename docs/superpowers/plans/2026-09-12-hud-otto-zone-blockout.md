# Le otto zone dell'HUD — piano di implementazione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rendere le otto zone dello Screen HUD oggetti che i test possono nominare, disegnarle a schermo con un blockout da cantiere, e rimontarci dentro i cinque widget esistenti.

**Architecture:** Una classe C++ `URTHudZoneWidget` porta `ZoneId` (enum `ERTHudZone`) e un flag di blockout; il Blueprint `WBP_RT_HudZone` le dà il bordo e un `NamedSlot Content`. `WBP_RT_TacticalHUD` monta otto istanze sulla griglia 20/60/20, e i widget attuali migrano nei rispettivi `Content`. Due gate headless nuovi chiedono *ci sono tutte le zone?* e *stanno dove devono?* — domande che oggi nessuno può porre.

**Tech Stack:** Unreal Engine 5.8, C++ (modulo `RefactorTactics`), UMG, Automation Test framework.

**Spec:** [`docs/superpowers/specs/2026-09-11-hud-otto-zone-blockout-design.md`](../specs/2026-09-11-hud-otto-zone-blockout-design.md)

## Global Constraints

- **Engine**: UE 5.8, installata in `D:/EpicGames/UE_5.8`. Non aggiornare engine, plugin o toolchain.
- **Clone di sviluppo** (Task 1–4): `D:/Repositories/refactor-tactics-refactor`, branch `design/hud-otto-zone-blockout`. Qui si scrive C++ e si eseguono i test headless.
- **Clone principale** (Task 5–7): `D:/Repositories/refactor-tactics-main`. ⛔ **L'authoring degli asset Unreal appartiene solo a questo clone** — il bridge MCP è uno solo, e un clone senza i file gitignorati salva asset i cui riferimenti duri leggono `None`, **azzerandoli senza errore** (`CLAUDE.md` §10).
- **Prima di compilare o misurare**: verificare che nessun'altra sessione tenga il motore, leggendo la `CommandLine` e non contando i processi:
  ```
  Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine
  ```
- **Lingua**: commenti e messaggi di test in italiano, come il resto di `Source/RefactorTactics/UI/` e `Tests/`.
- **Niente totali volatili** nei commit, nelle issue e nei documenti (`AGENTS.md` §14). Un conteggio che è *evidenza misurata* resta, col comando che lo produce.
- **Palette**: ⛔ i colori del blockout **non** usano le tinte Okabe-Ito del gioco (`#009E73` Movement, `#D55E00` Attack, `#0072B2` Utility, `#56B4E9` Defense): sono già impegnate semanticamente.
- **Griglia**: fasce 20% / 60% / 20% su entrambi gli assi; offset `L4 T4 R4 B4` su tutte le zone.

---

## Struttura dei file

| File | Responsabilità | Task |
|---|---|---|
| `Source/RefactorTactics/UI/RTHudZoneWidget.h` (nuovo) | `ERTHudZone` + dichiarazione `URTHudZoneWidget` | 1 |
| `Source/RefactorTactics/UI/RTHudZoneWidget.cpp` (nuovo) | `BlockoutColor`, l'unica logica della classe | 1 |
| `Source/RefactorTactics/Tests/RTHudZoneTests.cpp` (nuovo) | I test **puri** sulla classe: colori distinti, copertura dell'enum | 2 |
| `Source/RefactorTactics/Tests/RTMatchWidgetAssetTests.cpp` (modifica) | I due gate **sull'asset**: le otto zone ci sono, e stanno dove devono; poi il vocabolario di zone in `EveryZoneOwnerIsMountedByClass` e la pulizia del commento «sette widget» | 3, 4, 7 |
| `Content/RT/UI/Match/WBP_RT_HudZone.uasset` (nuovo) | Il bordo e il `NamedSlot Content` | 5 |
| `Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset` (modifica) | Le otto istanze e la migrazione degli inquilini | 6 |
| `docs/technical/runbooks/guida-screen-hud-umg.md` (modifica) | §3 riscritto sulle otto zone | 7 |

🔑 **Perché i test stanno in due file e non in uno.** `RTHudZoneTests.cpp` prova la **classe** e gira senza caricare nessun asset; i due gate di `RTMatchWidgetAssetTests.cpp` provano l'**albero** e falliscono finché il `.uasset` non è rimontato. Tenerli insieme significherebbe che i Task 2–4 non possono chiudere verdi prima del Task 6.

⚠️ **I Task 3 e 4 lasciano la suite ROSSA di proposito**, ed è il loro scopo: sono i gate che il Task 6 deve far passare. Chi esegue non deve "aggiustarli" — vanno committati rossi, con il rosso dichiarato nel messaggio.

---

### Task 1: `ERTHudZone` e `URTHudZoneWidget`

**Files:**
- Create: `Source/RefactorTactics/UI/RTHudZoneWidget.h`
- Create: `Source/RefactorTactics/UI/RTHudZoneWidget.cpp`

**Interfaces:**
- Consumes: niente (è il primo task).
- Produces: `enum class ERTHudZone : uint8` con gli otto valori `TopLeft, TopCenter, TopRight, MiddleLeft, MiddleRight, BottomLeft, BottomCenter, BottomRight`; `URTHudZoneWidget` con `ZoneId` (`ERTHudZone`), `bBlockoutVisible` (`bool`), e `static FLinearColor BlockoutColor(ERTHudZone)`.

- [ ] **Step 1: Scrivere l'header**

Crea `Source/RefactorTactics/UI/RTHudZoneWidget.h`:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTHudZoneWidget.generated.h"

/**
 * Le otto celle della griglia 3x3 che circonda il centro tattico.
 *
 * 🔑 **Il centro NON e' un valore di questo enum**, e l'assenza e' deliberata: e' definito da cio' che non
 * contiene (`progettazione-hud.md` §3.1), e un `ERTHudZone::Center` sarebbe un invito a riempirlo. Il layer
 * §4.2 disegna li' path, AoE e barre ancorate sopra la mappa.
 *
 * ⚠️ **L'ordine dei valori e' significativo**: `BlockoutColor` deriva la tinta dall'indice, quindi
 * riordinarli rimescola i colori del blockout. Non e' un difetto — sono colori da cantiere — ma chi
 * riordina se ne accorga invece di sorprendersi.
 */
UENUM(BlueprintType)
enum class ERTHudZone : uint8
{
	TopLeft      UMETA(DisplayName = "Alto sinistra"),
	TopCenter    UMETA(DisplayName = "Alto centro"),
	TopRight     UMETA(DisplayName = "Alto destra"),
	MiddleLeft   UMETA(DisplayName = "Mezzo sinistra"),
	MiddleRight  UMETA(DisplayName = "Mezzo destra"),
	BottomLeft   UMETA(DisplayName = "Basso sinistra"),
	BottomCenter UMETA(DisplayName = "Basso centro"),
	BottomRight  UMETA(DisplayName = "Basso destra"),

	/** Sentinella per il conteggio. Non e' una zona: non assegnarla mai a un `ZoneId`. */
	Count        UMETA(Hidden)
};

/**
 * `WBP_RT_HudZone` — UNA zona dello Screen HUD: un contenitore geometrico con un `NamedSlot Content`.
 *
 * 🔑 **`UUserWidget` e non `URTScreenHudWidgetBase`, e la differenza e' il punto.** Una zona e' geometria:
 * non interroga il ViewModel, non sa cosa sia un turno, non ha niente da aggiornare quando la partita
 * cambia. Derivarla dalla base le darebbe un accesso al ViewModel che non le serve — e un accesso che
 * esiste, prima o poi qualcuno lo usa. E' la stessa disciplina di `URTActionSlotWidget` e
 * `URTFastDecisionOptionWidget`.
 *
 * 🔑 **`ZoneId` esiste per rendere la zona MISURABILE.** Prima di questa classe una zona era un
 * `UCanvasPanelSlot` con un nome scelto nel Designer: per un test, indistinguibile da qualunque altro
 * nodo. E' la condizione in cui `cc5ca967` ha risalvato l'albero allo stato precedente al fix di `#2760`
 * con la suite verde.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTHudZoneWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Quale cella della griglia e' questa istanza.
	 *
	 * ⚠️ `EditAnywhere` e non `EditDefaultsOnly`: le otto istanze condividono la stessa classe e si
	 * distinguono **per istanza**. E' l'opposto di `bShowDebug` su `URTScreenHudWidgetBase`, che e'
	 * `EditDefaultsOnly` proprio per non restare acceso in una schermata dimenticata.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HUD")
	ERTHudZone ZoneId = ERTHudZone::TopLeft;

	/**
	 * Accende i bordi spessi colorati da cantiere. Si spegne quando il contenuto della zona arriva.
	 *
	 * ⛔ **Non e' uno stile di produzione**: vedi `BlockoutColor`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bBlockoutVisible = true;

	/**
	 * Il colore del bordo da cantiere, DERIVATO da `ZoneId`.
	 *
	 * 🔑 **Pura e statica**: stesso ingresso, stesso colore, e la mappa zona -> tinta sta in un punto solo
	 * invece che in otto istanze da tenere allineate a mano.
	 *
	 * ⛔ **Queste tinte NON vengono dalla palette di gioco, e non passano i criteri di accessibilita' del
	 * progetto.** La palette e' Okabe-Ito e ogni tinta e' impegnata (`spec-icon-card-grammar.md`):
	 * `#009E73` e' Movement, `#D55E00` Attack, `#0072B2` Utility, `#56B4E9` Defense. Un bordo verde
	 * verrebbe letto come «movimento». Qui si usano tonalita' HSV equispaziate che nella palette non
	 * esistono: stonano di proposito, perche' un bordo da cantiere che sembra design finito e' un bordo che
	 * resta montato. Non raggiungono il giocatore; se ci arrivassero, il difetto sarebbe quello.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	static FLinearColor BlockoutColor(ERTHudZone Zone);

	/** L'etichetta diagnostica di una zona, per i messaggi di test e i log. Mai a schermo. */
	static FString ZoneName(ERTHudZone Zone);
};
```

- [ ] **Step 2: Scrivere l'implementazione**

Crea `Source/RefactorTactics/UI/RTHudZoneWidget.cpp`:

```cpp
#include "UI/RTHudZoneWidget.h"

FLinearColor URTHudZoneWidget::BlockoutColor(ERTHudZone Zone)
{
	const uint8 Indice = static_cast<uint8>(Zone);
	if (Indice >= static_cast<uint8>(ERTHudZone::Count))
	{
		// Magenta pieno: il colore che non appartiene a nessuna zona, per un valore che non e' una zona.
		return FLinearColor(1.f, 0.f, 1.f, 1.f);
	}

	// Otto tonalita' equispaziate sul cerchio, saturazione e valore pieni. `FLinearColor::MakeFromHSV8`
	// vuole la tinta su 0-255, non su 0-360.
	//
	// ⚠️ **L'aritmetica e' in `int32` per scelta, non per caso.** `Indice` e' un `uint8` e la promozione
	// intera farebbe comunque il lavoro, ma `Indice * 256` scritto su `uint8` sembra un overflow a chi
	// legge: reso esplicito, non c'e' niente da dedurre. Le tinte che ne escono sono 0, 32, 64 ... 224.
	const int32 Quante = static_cast<int32>(ERTHudZone::Count);
	const int32 Tinta = (static_cast<int32>(Indice) * 256) / Quante;
	return FLinearColor::MakeFromHSV8(static_cast<uint8>(Tinta), 255, 255);
}

FString URTHudZoneWidget::ZoneName(ERTHudZone Zone)
{
	switch (Zone)
	{
	case ERTHudZone::TopLeft:      return TEXT("TopLeft");
	case ERTHudZone::TopCenter:    return TEXT("TopCenter");
	case ERTHudZone::TopRight:     return TEXT("TopRight");
	case ERTHudZone::MiddleLeft:   return TEXT("MiddleLeft");
	case ERTHudZone::MiddleRight:  return TEXT("MiddleRight");
	case ERTHudZone::BottomLeft:   return TEXT("BottomLeft");
	case ERTHudZone::BottomCenter: return TEXT("BottomCenter");
	case ERTHudZone::BottomRight:  return TEXT("BottomRight");
	default:                       return TEXT("<non e' una zona>");
	}
}
```

- [ ] **Step 3: Compilare**

Verificare prima che il motore sia libero:

```powershell
Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine
```

Poi, da `D:/Repositories/refactor-tactics-refactor`:

```powershell
& "D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development `
  -Project="D:\Repositories\refactor-tactics-refactor\RefactorTactics.uproject" -WaitMutex
```

Atteso: `Build succeeded`. Un nuovo `UCLASS` in un file nuovo richiede la rigenerazione dell'header — la fa UBT da sé.

- [ ] **Step 4: Commit**

```bash
git add Source/RefactorTactics/UI/RTHudZoneWidget.h Source/RefactorTactics/UI/RTHudZoneWidget.cpp
git commit -m "feat(hud): una zona dello Screen HUD diventa una cosa che i test possono nominare

Prima di questa classe una zona era un UCanvasPanelSlot con un nome scelto nel
Designer: per un test, indistinguibile da qualunque altro nodo. E' la condizione
in cui cc5ca967 ha potuto risalvare l'albero allo stato precedente al fix di
#2760 lasciando la suite verde.

ZoneId porta l'identita' in C++, e BlockoutColor la deriva invece di farla
scegliere a otto istanze da tenere allineate a mano.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

### Task 2: I test puri sulla classe

**Files:**
- Create: `Source/RefactorTactics/Tests/RTHudZoneTests.cpp`

**Interfaces:**
- Consumes: `ERTHudZone`, `URTHudZoneWidget::BlockoutColor`, `URTHudZoneWidget::ZoneName` dal Task 1.
- Produces: niente che altri task consumino.

⚠️ **Questi test devono passare VERDI** alla fine del task — a differenza dei Task 3 e 4.

- [ ] **Step 1: Scrivere i test**

Crea `Source/RefactorTactics/Tests/RTHudZoneTests.cpp`:

```cpp
// Le otto zone dello Screen HUD, provate come CLASSE e non come albero.
//
// 🔑 **La divisione con `RTMatchWidgetAssetTests.cpp` e' deliberata**: questi test girano senza caricare
// nessun asset e sono verdi appena il Task 1 compila. I due gate sull'albero vivono nell'altro file e
// restano rossi finche' il `.uasset` non e' rimontato — tenerli insieme significherebbe non poter chiudere
// verde nessun task prima dell'authoring.

#include "Misc/AutomationTest.h"
#include "UI/RTHudZoneWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudZoneColorsAreAllDistinctTest,
	"RefactorTactics.ScreenHud.HudZoneColorsAreAllDistinct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudZoneColorsAreAllDistinctTest::RunTest(const FString&)
{
	// 🔑 Due zone dello stesso colore renderebbero il blockout inutile proprio dove serve: a dire QUALE
	// riquadro si sta guardando.
	const int32 Quante = static_cast<int32>(ERTHudZone::Count);

	for (int32 A = 0; A < Quante; ++A)
	{
		const ERTHudZone ZonaA = static_cast<ERTHudZone>(A);
		const FLinearColor ColoreA = URTHudZoneWidget::BlockoutColor(ZonaA);

		AddInfo(FString::Printf(TEXT("  %-14s -> R%.2f G%.2f B%.2f"),
			*URTHudZoneWidget::ZoneName(ZonaA), ColoreA.R, ColoreA.G, ColoreA.B));

		for (int32 B = A + 1; B < Quante; ++B)
		{
			const ERTHudZone ZonaB = static_cast<ERTHudZone>(B);
			const FLinearColor ColoreB = URTHudZoneWidget::BlockoutColor(ZonaB);

			// `Equals` con tolleranza: due tinte adiacenti sul cerchio non devono coincidere, e un
			// confronto esatto su float direbbe «diverse» anche per una differenza invisibile.
			if (ColoreA.Equals(ColoreB, 0.05f))
			{
				AddError(FString::Printf(
					TEXT("le zone `%s` e `%s` hanno lo stesso colore di blockout (R%.2f G%.2f B%.2f): ")
					TEXT("a schermo non si distinguono, ed e' l'unica cosa che il blockout deve fare."),
					*URTHudZoneWidget::ZoneName(ZonaA), *URTHudZoneWidget::ZoneName(ZonaB),
					ColoreA.R, ColoreA.G, ColoreA.B));
			}
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudZoneNamesCoverEveryValueTest,
	"RefactorTactics.ScreenHud.HudZoneNamesCoverEveryValue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudZoneNamesCoverEveryValueTest::RunTest(const FString&)
{
	// 🔑 **Il default dello switch e' una trappola silenziosa**: chi aggiunge un valore all'enum e dimentica
	// `ZoneName` non rompe la compilazione, ottiene «<non e' una zona>» dentro i messaggi degli ALTRI test —
	// che diventano illeggibili proprio mentre qualcosa non va.
	for (int32 I = 0; I < static_cast<int32>(ERTHudZone::Count); ++I)
	{
		const ERTHudZone Zona = static_cast<ERTHudZone>(I);
		const FString Nome = URTHudZoneWidget::ZoneName(Zona);

		TestFalse(
			*FString::Printf(TEXT("la zona di indice %d ha un nome (ne ha reso `%s`)"), I, *Nome),
			Nome.Contains(TEXT("non e' una zona")));
	}

	// E la sentinella NON deve avere un nome: se lo avesse, sarebbe diventata una zona senza che nessuno
	// lo decidesse.
	TestTrue(TEXT("`Count` non e' una zona, e il suo nome lo dice"),
		URTHudZoneWidget::ZoneName(ERTHudZone::Count).Contains(TEXT("non e' una zona")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 2: Compilare**

```powershell
& "D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development `
  -Project="D:\Repositories\refactor-tactics-refactor\RefactorTactics.uproject" -WaitMutex
```

Atteso: `Build succeeded`.

- [ ] **Step 3: Eseguire i due test**

```powershell
& "D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "D:\Repositories\refactor-tactics-refactor\RefactorTactics.uproject" `
  -ExecCmds="Automation RunTests RefactorTactics.ScreenHud.HudZone+Quit" `
  -unattended -nopause -nullrhi -NoSound
```

Gli esiti stanno in `Saved/Logs/RefactorTactics.log`, **non** nello stdout:

```bash
grep -E "HudZone|Test Completed|Failed" Saved/Logs/RefactorTactics.log | tail -20
```

Atteso: entrambi `Success`. Il report elenca le otto zone con i rispettivi RGB.

⚠️ **Se `HudZoneColorsAreAllDistinct` fallisce**, la causa più probabile è che `MakeFromHSV8` renda tinte troppo vicine agli estremi del cerchio (0 e 256 sono lo stesso rosso). In quel caso la correzione è nel Task 1 — `(Indice * 224) / Count`, che lascia il cerchio aperto — non alzare la tolleranza del test.

- [ ] **Step 4: Commit**

```bash
git add Source/RefactorTactics/Tests/RTHudZoneTests.cpp
git commit -m "test(hud): il blockout deve distinguere le zone, quindi i colori non possono coincidere

Due test puri, che girano senza caricare nessun asset: le otto tinte sono
distinte a vista, e ogni valore dell'enum ha un nome diagnostico.

Il secondo esiste per una trappola silenziosa: chi aggiunge una zona e dimentica
ZoneName non rompe la compilazione, ottiene un segnaposto dentro i messaggi degli
ALTRI test, che diventano illeggibili proprio mentre qualcosa non va.

Verification:
- Compile: PASS
- Tests: PASS (RefactorTactics.ScreenHud.HudZone*)

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

### Task 3: Il gate «le otto zone ci sono, una volta ciascuna»

**Files:**
- Modify: `Source/RefactorTactics/Tests/RTMatchWidgetAssetTests.cpp` (aggiunta in coda, prima di `#endif // WITH_DEV_AUTOMATION_TESTS`)

**Interfaces:**
- Consumes: `ERTHudZone`, `URTHudZoneWidget` dal Task 1; `RTWidgetAssetTest::LoadWidgetTree` dall'header già esistente; `TacticalHudPath` dal namespace anonimo in cima al file.
- Produces: niente.

🔴 **Questo test DEVE fallire alla fine del task**, e va committato rosso. È il gate che il Task 6 deve far passare: un gate scritto dopo l'asset non prova nulla, perché non lo si è mai visto fallire.

- [ ] **Step 1: Aggiungere l'include**

In cima a `RTMatchWidgetAssetTests.cpp`, accanto agli altri include di `UI/`:

```cpp
#include "UI/RTHudZoneWidget.h"
```

- [ ] **Step 2: Scrivere il test**

In fondo al file, **prima** di `#endif // WITH_DEV_AUTOMATION_TESTS`:

```cpp
// =====================================================================================================
// Le otto zone: ci sono tutte, una volta ciascuna
// =====================================================================================================
//
// 🔴 **E' la domanda che prima di `URTHudZoneWidget` nessuno poteva porre**, e la ragione per cui quella
// classe esiste. Una zona era un `UCanvasPanelSlot` con un nome scelto nel Designer: indistinguibile, per
// un test, da qualunque altro nodo. E' anche la domanda che `cc5ca967` ha eluso — un salvataggio che ha
// riportato l'albero allo stato precedente al fix di `#2760`, con la suite verde perche' nessun gate
// guardava.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTheEightZonesAreDeclaredExactlyOnceTest,
	"RefactorTactics.ScreenHud.TheEightZonesAreDeclaredExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTTheEightZonesAreDeclaredExactlyOnceTest::RunTest(const FString&)
{
	const UWidgetTree* Tree = RTWidgetAssetTest::LoadWidgetTree(*this, TacticalHudPath,
		TEXT("WBP_RT_TacticalHUD"));
	if (Tree == nullptr)
	{
		return false;
	}

	// Quante istanze per ogni valore dell'enum. Zero e due sono due difetti diversi, e il report li deve
	// distinguere: «manca» si corregge aggiungendo, «doppia» si corregge cambiando un `ZoneId`.
	TArray<int32> Conteggio;
	Conteggio.Init(0, static_cast<int32>(ERTHudZone::Count));

	int32 FuoriEnum = 0;

	Tree->ForEachWidget([&Conteggio, &FuoriEnum, this](UWidget* Widget)
	{
		const URTHudZoneWidget* Zona = Cast<URTHudZoneWidget>(Widget);
		if (!Zona)
		{
			return;
		}

		const int32 Indice = static_cast<int32>(Zona->ZoneId);
		if (Conteggio.IsValidIndex(Indice))
		{
			++Conteggio[Indice];
			AddInfo(FString::Printf(TEXT("  %-24s ZoneId=%s"),
				*Widget->GetName(), *URTHudZoneWidget::ZoneName(Zona->ZoneId)));
		}
		else
		{
			++FuoriEnum;
			AddError(FString::Printf(
				TEXT("la zona `%s` porta un `ZoneId` che non e' una zona (indice %d). ")
				TEXT("`ERTHudZone::Count` e' una sentinella per il conteggio, non un valore assegnabile."),
				*Widget->GetName(), Indice));
		}
	});

	for (int32 I = 0; I < Conteggio.Num(); ++I)
	{
		const FString Nome = URTHudZoneWidget::ZoneName(static_cast<ERTHudZone>(I));

		if (Conteggio[I] == 0)
		{
			AddError(FString::Printf(
				TEXT("`WBP_RT_TacticalHUD` non dichiara nessuna zona `%s`. Le otto zone sono la griglia ")
				TEXT("3x3 meno il centro (`guida-screen-hud-umg.md` §3): una che manca e' un buco nel ")
				TEXT("layout, non uno spazio libero."), *Nome));
		}
		else if (Conteggio[I] > 1)
		{
			AddError(FString::Printf(
				TEXT("`WBP_RT_TacticalHUD` dichiara %d zone `%s`. Due istanze con lo stesso `ZoneId` si ")
				TEXT("sovrappongono a schermo, e il blockout smette di dire quale riquadro si guarda."),
				Conteggio[I], *Nome));
		}
	}

	// 🔴 **Senza questa riga il test sarebbe verde su un albero SENZA zone** — cioe' misurando zero, il modo
	// in cui un gate diventa decorativo. Questo file lo ha gia' imparato una volta, in
	// `PanelsLeaveTheCenterFree`.
	const int32 Totale = Algo::Accumulate(Conteggio, 0) + FuoriEnum;
	TestTrue(
		*FString::Printf(TEXT("l'albero contiene delle zone da misurare (ne ha %d)"), Totale),
		Totale > 0);

	return true;
}
```

⚠️ `Algo::Accumulate` richiede `#include "Algo/Accumulate.h"` — aggiungerlo in cima al file se non c'è già.

- [ ] **Step 3: Compilare**

```powershell
& "D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development `
  -Project="D:\Repositories\refactor-tactics-refactor\RefactorTactics.uproject" -WaitMutex
```

Atteso: `Build succeeded`.

- [ ] **Step 4: Eseguire il test e VERIFICARE CHE FALLISCA**

```powershell
& "D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "D:\Repositories\refactor-tactics-refactor\RefactorTactics.uproject" `
  -ExecCmds="Automation RunTests RefactorTactics.ScreenHud.TheEightZonesAreDeclaredExactlyOnce+Quit" `
  -unattended -nopause -nullrhi -NoSound
```

```bash
grep -E "TheEightZones|Test Completed|Failed" Saved/Logs/RefactorTactics.log | tail -20
```

🔴 **Atteso: FAIL**, con otto errori «non dichiara nessuna zona …» e il fallimento di «l'albero contiene delle zone da misurare (ne ha 0)». L'asset non è ancora rimontato.

⛔ **Se passa, qualcosa non va nel test, non nell'asset** — fermarsi e capire perché prima di proseguire.

- [ ] **Step 5: Commit (rosso, dichiarato)**

```bash
git add Source/RefactorTactics/Tests/RTMatchWidgetAssetTests.cpp
git commit -m "test(hud): il gate che chiede se le otto zone ci siano, e oggi risponde di no

ROSSO DI PROPOSITO: l'asset non e' ancora rimontato, e questo test e' il gate che
il rimontaggio dovra' far passare. Un gate scritto DOPO l'asset non prova niente,
perche' non lo si e' mai visto fallire.

Misurato ora: otto errori 'non dichiara nessuna zona', e zero zone da misurare.

Verification:
- Compile: PASS
- Tests: FAIL atteso (TheEightZonesAreDeclaredExactlyOnce) — chiude col Task 6

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

### Task 4: Il gate «le zone stanno dove devono»

**Files:**
- Modify: `Source/RefactorTactics/Tests/RTMatchWidgetAssetTests.cpp` (aggiunta in coda)

**Interfaces:**
- Consumes: `ERTHudZone`, `URTHudZoneWidget` dal Task 1; il namespace `RTCenterFree` già presente nel file, con `RefWidth = 1920.f`, `RefHeight = 1080.f`, `FRect { Left, Top, Right, Bottom }`, `RettangoloDellaZona(const FAnchorData&)`, `Descrivi(const FRect&)`.
- Produces: niente.

🔴 **Anche questo va committato ROSSO.**

🔑 **Riusa `RTCenterFree::RettangoloDellaZona` invece di riscriverla.** È la formula di `SConstraintCanvas::OnArrangeChildren`, e gli offset di un `UCanvasPanelSlot` **non sono un rettangolo**: cambiano significato con gli anchor. Una seconda copia divergerebbe alla prima correzione.

- [ ] **Step 1: Scrivere il test**

In fondo a `RTMatchWidgetAssetTests.cpp`, prima di `#endif`:

```cpp
// =====================================================================================================
// Le otto zone: stanno dove devono
// =====================================================================================================
//
// 🔑 **`PanelsLeaveTheCenterFree` e' un gate NEGATIVO**: dice che nessuna zona invade il centro, e
// passerebbe con tutte e otto schiacciate in un angolo. Questo dice dove sono.
//
// La griglia e' 20% / 60% / 20% su entrambi gli assi, e non e' una scelta: il keep-out del centro e'
// `RTCenterFree::CenterFraction` = 0.6 centrato, quindi i tagli cadono a 0.2 e 0.8.

namespace RTGrigliaZone
{
	/** I tagli della griglia, in frazione di schermo. */
	constexpr float TaglioBasso = 0.2f;
	constexpr float TaglioAlto = 0.8f;

	/** Gli offset uniformi di ogni zona: distacco visivo, e margine dal keep-out. */
	constexpr float Margine = 4.f;

	/** La cella attesa di una zona, in frazione di schermo: `Min` e `Max` degli anchor. */
	void CellaAttesa(ERTHudZone Zona, FVector2D& Min, FVector2D& Max)
	{
		const int32 I = static_cast<int32>(Zona);

		// Colonna: 0 = sinistra, 1 = centro, 2 = destra. Riga: 0 = alto, 1 = mezzo, 2 = basso.
		// L'ordine dell'enum salta la cella centrale, quindi la mappa e' esplicita invece che calcolata.
		static const int32 Colonne[] = { 0, 1, 2,  0, 2,  0, 1, 2 };
		static const int32 Righe[]   = { 0, 0, 0,  1, 1,  2, 2, 2 };

		static const float Bordi[] = { 0.f, TaglioBasso, TaglioAlto, 1.f };

		Min.X = Bordi[Colonne[I]];
		Max.X = Bordi[Colonne[I] + 1];
		Min.Y = Bordi[Righe[I]];
		Max.Y = Bordi[Righe[I] + 1];
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTZoneRectanglesMatchTheThreeByThreeGridTest,
	"RefactorTactics.ScreenHud.ZoneRectanglesMatchTheThreeByThreeGrid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTZoneRectanglesMatchTheThreeByThreeGridTest::RunTest(const FString&)
{
	const UWidgetTree* Tree = RTWidgetAssetTest::LoadWidgetTree(*this, TacticalHudPath,
		TEXT("WBP_RT_TacticalHUD"));
	if (Tree == nullptr)
	{
		return false;
	}

	// Un pixel di tolleranza: le geometrie sono float, e un arrotondamento non e' un difetto di layout.
	constexpr float Tolleranza = 1.f;

	int32 Misurate = 0;

	Tree->ForEachWidget([this, &Misurate, Tolleranza](UWidget* Widget)
	{
		const URTHudZoneWidget* Zona = Cast<URTHudZoneWidget>(Widget);
		if (!Zona)
		{
			return;
		}

		const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot);
		if (!Slot)
		{
			AddError(FString::Printf(
				TEXT("la zona `%s` non e' in un `Canvas Panel`: la sua geometria non e' dichiarata dal ")
				TEXT("layout, e il centro libero smette di essere una proprieta' verificabile."),
				*Widget->GetName()));
			return;
		}

		FVector2D Min, Max;
		RTGrigliaZone::CellaAttesa(Zona->ZoneId, Min, Max);

		const RTCenterFree::FRect Atteso{
			static_cast<float>(Min.X) * RTCenterFree::RefWidth + RTGrigliaZone::Margine,
			static_cast<float>(Min.Y) * RTCenterFree::RefHeight + RTGrigliaZone::Margine,
			static_cast<float>(Max.X) * RTCenterFree::RefWidth - RTGrigliaZone::Margine,
			static_cast<float>(Max.Y) * RTCenterFree::RefHeight - RTGrigliaZone::Margine };

		const RTCenterFree::FRect Reale = RTCenterFree::RettangoloDellaZona(Slot->GetLayout());
		++Misurate;

		AddInfo(FString::Printf(TEXT("  %-14s atteso %s   reale %s"),
			*URTHudZoneWidget::ZoneName(Zona->ZoneId),
			*RTCenterFree::Descrivi(Atteso), *RTCenterFree::Descrivi(Reale)));

		const bool bCombacia =
			FMath::IsNearlyEqual(Reale.Left, Atteso.Left, Tolleranza)
			&& FMath::IsNearlyEqual(Reale.Top, Atteso.Top, Tolleranza)
			&& FMath::IsNearlyEqual(Reale.Right, Atteso.Right, Tolleranza)
			&& FMath::IsNearlyEqual(Reale.Bottom, Atteso.Bottom, Tolleranza);

		if (!bCombacia)
		{
			AddError(FString::Printf(
				TEXT("la zona `%s` non occupa la sua cella della griglia 20/60/20: atteso %s, reale %s. ")
				TEXT("Gli anchor devono essere STIRATI su entrambi gli assi — con anchor a punto il ")
				TEXT("rettangolo dipende dall'Alignment, ed e' il difetto che porto' `ZoneBottom` a ")
				TEXT("`Y 1080..1280`, fuori schermo."),
				*URTHudZoneWidget::ZoneName(Zona->ZoneId),
				*RTCenterFree::Descrivi(Atteso), *RTCenterFree::Descrivi(Reale)));
		}
	});

	TestTrue(
		*FString::Printf(TEXT("l'albero contiene delle zone da misurare (ne ha %d)"), Misurate),
		Misurate > 0);

	return true;
}
```

- [ ] **Step 2: Compilare**

```powershell
& "D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development `
  -Project="D:\Repositories\refactor-tactics-refactor\RefactorTactics.uproject" -WaitMutex
```

Atteso: `Build succeeded`.

- [ ] **Step 3: Eseguire e verificare che fallisca**

```powershell
& "D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "D:\Repositories\refactor-tactics-refactor\RefactorTactics.uproject" `
  -ExecCmds="Automation RunTests RefactorTactics.ScreenHud.ZoneRectanglesMatchTheThreeByThreeGrid+Quit" `
  -unattended -nopause -nullrhi -NoSound
```

```bash
grep -E "ZoneRectangles|Test Completed|Failed" Saved/Logs/RefactorTactics.log | tail -20
```

🔴 **Atteso: FAIL** su «l'albero contiene delle zone da misurare (ne ha 0)».

- [ ] **Step 4: Eseguire l'INTERA suite ScreenHud, per la baseline**

```powershell
& "D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "D:\Repositories\refactor-tactics-refactor\RefactorTactics.uproject" `
  -ExecCmds="Automation RunTests RefactorTactics.ScreenHud+Quit" `
  -unattended -nopause -nullrhi -NoSound
```

```bash
grep -E "Test Completed|Success|Fail" Saved/Logs/RefactorTactics.log | tail -30
```

🔑 **Annotare quali test sono rossi ORA**, prima dell'authoring. È la baseline che dice, dopo il Task 6, quali rossi sono stati chiusi e quali eventuali nuovi sono stati introdotti. Attesi rossi: i due nuovi. Tutti gli altri `ScreenHud.*` devono essere verdi — se uno non lo è, **non è causato da questo lavoro** e va annotato come preesistente.

- [ ] **Step 5: Commit (rosso, dichiarato)**

```bash
git add Source/RefactorTactics/Tests/RTMatchWidgetAssetTests.cpp
git commit -m "test(hud): il gate che chiede DOVE stanno le zone, non solo se ci sono

ROSSO DI PROPOSITO, come il precedente.

PanelsLeaveTheCenterFree e' un gate negativo: dice che nessuna zona invade il
centro, e passerebbe con tutte e otto schiacciate in un angolo. Questo confronta
ogni rettangolo con la sua cella della griglia 20/60/20 — che non e' una scelta,
ma la conseguenza del keep-out al 60% centrato.

Riusa RTCenterFree::RettangoloDellaZona invece di riscriverla: e' la formula di
SConstraintCanvas::OnArrangeChildren, e gli offset di un UCanvasPanelSlot non
sono un rettangolo — cambiano significato con gli anchor.

Verification:
- Compile: PASS
- Tests: FAIL atteso (ZoneRectanglesMatchTheThreeByThreeGrid) — chiude col Task 6

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

### Task 5: `WBP_RT_HudZone` — l'asset della zona

⛔ **DA QUI IN POI SI LAVORA NEL CLONE PRINCIPALE**: `D:/Repositories/refactor-tactics-main`.

**Files:**
- Create: `Content/RT/UI/Match/WBP_RT_HudZone.uasset`

**Interfaces:**
- Consumes: `URTHudZoneWidget` dal Task 1 (deve essere compilata: senza la classe non c'è da cosa derivare).
- Produces: `WBP_RT_HudZone` con un `NamedSlot` chiamato **esattamente** `Content`, consumato dal Task 6.

- [ ] **Step 1: Portare il C++ nel clone principale**

```bash
cd D:/Repositories/refactor-tactics-main
git fetch origin
git checkout design/hud-otto-zone-blockout   # oppure il branch in cui i Task 1-4 sono stati mergiati
git log --oneline -4                          # deve mostrare i quattro commit
```

- [ ] **Step 2: Verificare che il motore sia libero, poi compilare**

```powershell
Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine
```

Se un Editor è aperto sul `.uproject` di un **altro** clone, aspettare o dichiarare `NOT RUN` col nome di quel clone. Se è aperto su questo, chiuderlo prima di compilare — altrimenti tiene il DLL.

```powershell
& "D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development `
  -Project="D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" -WaitMutex
```

- [ ] **Step 3: Creare il Blueprint**

Aprire l'Editor su `D:/Repositories/refactor-tactics-main/RefactorTactics.uproject`, con `-abslog` nel proprio scratchpad di sessione così il processo è attribuibile:

```powershell
& "D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" `
  "D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" `
  -abslog="<scratchpad>\editor-hud-zone.log"
```

In `Content/RT/UI/Match/`, creare un **Widget Blueprint** derivato da `URTHudZoneWidget` (nella finestra di scelta della classe genitore, cercare `RTHudZoneWidget`), e chiamarlo `WBP_RT_HudZone`.

Albero del widget:

```
Root: Overlay  (nome: ZoneRoot)
 ├── Border      (nome: ZoneBorder)   ← decorativo, si spegne col blockout
 └── NamedSlot   (nome: Content)      ← il nome DEVE essere esattamente `Content`
```

🔴 **Radice `Overlay`, non `Border` che avvolge il `NamedSlot`.** Un `Border` collassato collassa anche i
suoi figli: se `Content` fosse figlio di `ZoneBorder`, spegnere il blockout su `Zone_TopCenter`
(`bBlockoutVisible = false`) farebbe sparire anche `WBP_RT_TurnHeader` — ed è il contratto della classe a
dirlo, non un'ipotesi: `RTHudZoneWidget.h` documenta `bBlockoutVisible` come *«si spegne quando il contenuto
della zona arriva»*, cioè proprio per le zone che un inquilino ce l'hanno già. Con `Border` e `NamedSlot`
**fratelli** dentro l'`Overlay`, spegnere il primo non tocca il secondo.

Impostazioni del `Border` `ZoneBorder`:
- `Brush` → `Draw As: Border`, con un materiale o una texture di bordo. ⛔ **Niente ripiego su `Draw As: Box`**: un `Margin` a 0.1 produce comunque un riquadro PIENO a saturazione e valore massimi, dietro `WBP_RT_TurnHeader`, `WBP_RT_TeamRosterLeft`, `WBP_RT_EventLogRight`, `WBP_RT_SelectedUnitPanelLeft` e `WBP_RT_ActionDockBottom` — la richiesta era «bordi spessi», non un fondo a tinta unita, e il giudizio PIE cadrebbe su un HUD illeggibile. Se manca la risorsa di bordo, procurarsene una (anche un 9-slice minimale) prima di continuare.
- `Padding`: 8 su tutti i lati.
- `Brush Color`: **binding** — non a una funzione parametrica (UMG non passa argomenti a un property binding), ma alla funzione generata dal binding stesso: senza parametri, che al suo interno legge `ZoneId` e lo passa a `BlockoutColor`. Nel grafo: `Get ZoneId` → `BlockoutColor` → return value.
- `Visibility`: **binding**, solo su questo `Border`, su `bBlockoutVisible` → `SelfHitTestInvisible` / `Collapsed`. ⛔ **Non `Visible`**: è hit-testable, e `Zone_TopRight`, `Zone_BottomLeft` e `Zone_BottomRight` nascono con `Content` vuoto — oggi lì non c'è niente che intercetti un click, con `Visible` ci sarebbe un rettangolo opaco che lo mangia. `SelfHitTestInvisible` mostra il colore ma lascia passare l'input.

⚠️ **Il colore va in binding, non impostato a mano.** Un colore fissato nel Designer si scollegherebbe da `ZoneId` alla prima istanza in cui qualcuno cambia zona, ed è esattamente ciò che la funzione statica esiste per impedire.

- [ ] **Step 4: Salvare e RILEGGERE dal disco**

```bash
cd D:/Repositories/refactor-tactics-main
python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_HudZone.uasset --filtro Content --unici
python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_HudZone.uasset --filtro HudZone --unici
```

Atteso: il primo nomina `Content`; il secondo nomina `RTHudZoneWidget` (la classe genitore) e `WBP_RT_HudZone`.

🔴 **Questo passo non è una formalità.** L'Editor può risalvare una copia in memoria anteriore, ed è successo tre volte su questo repository il 2026-09-10. Rileggere il blob è l'unico modo di accorgersene.

- [ ] **Step 5: Commit**

```bash
git add Content/RT/UI/Match/WBP_RT_HudZone.uasset
git commit -m "feat(hud): l'asset di una zona — un bordo da cantiere e un NamedSlot

Il colore e' in BINDING su BlockoutColor(ZoneId), non impostato a mano: un colore
fissato nel Designer si scollegherebbe dalla zona alla prima istanza in cui
qualcuno cambia ZoneId.

Riletto dal disco dopo il salvataggio:
  names.py WBP_RT_HudZone.uasset --filtro Content  -> Content
  names.py WBP_RT_HudZone.uasset --filtro HudZone  -> RTHudZoneWidget, WBP_RT_HudZone

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

### Task 6: Rimontare `WBP_RT_TacticalHUD`

**Files:**
- Modify: `Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset`

**Interfaces:**
- Consumes: `WBP_RT_HudZone` dal Task 5, col suo `NamedSlot Content`.
- Produces: l'albero che i gate dei Task 3 e 4 misurano.

🔑 **È il task che chiude i due rossi.** Alla fine, entrambi devono essere verdi.

- [ ] **Step 1: Registrare lo stato PRIMA**

```bash
cd D:/Repositories/refactor-tactics-main
python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset --filtro Zone --unici
python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset --filtro WBP_RT_ --unici
git rev-parse --short HEAD
```

Atteso prima: `ZoneBottom, ZoneBottomContainer, ZoneLeft, ZoneRight, Zone_Top`. Annotare l'elenco dei `WBP_RT_*` — dopo il rimontaggio devono esserci **gli stessi**, più `WBP_RT_HudZone`.

- [ ] **Step 2: Aggiungere le otto zone al Canvas radice**

Nell'Editor, aprire `WBP_RT_TacticalHUD`. La radice è già un `Canvas Panel` e **non va cambiata**: `PanelsLeaveTheCenterFree` fallisce esplicitamente se non lo è.

Aggiungere otto istanze di `WBP_RT_HudZone` come figli **diretti** del Canvas, con questi nomi, `ZoneId` e geometria:

| Nome del nodo | `ZoneId` | Anchors Min | Anchors Max | Offsets |
|---|---|---|---|---|
| `Zone_TopLeft` | `TopLeft` | (0.0, 0.0) | (0.2, 0.2) | L4 T4 R4 B4 |
| `Zone_TopCenter` | `TopCenter` | (0.2, 0.0) | (0.8, 0.2) | L4 T4 R4 B4 |
| `Zone_TopRight` | `TopRight` | (0.8, 0.0) | (1.0, 0.2) | L4 T4 R4 B4 |
| `Zone_MiddleLeft` | `MiddleLeft` | (0.0, 0.2) | (0.2, 0.8) | L4 T4 R4 B4 |
| `Zone_MiddleRight` | `MiddleRight` | (0.8, 0.2) | (1.0, 0.8) | L4 T4 R4 B4 |
| `Zone_BottomLeft` | `BottomLeft` | (0.0, 0.8) | (0.2, 1.0) | L4 T4 R4 B4 |
| `Zone_BottomCenter` | `BottomCenter` | (0.2, 0.8) | (0.8, 1.0) | L4 T4 R4 B4 |
| `Zone_BottomRight` | `BottomRight` | (0.8, 0.8) | (1.0, 1.0) | L4 T4 R4 B4 |

⛔ **`Size To Content` (AutoSize) deve restare SPENTO su tutte.** `PanelsLeaveTheCenterFree` lo rifiuta esplicitamente: una zona che si dimensiona sul contenuto può invadere il centro quando il contenuto cresce, e il centro libero smette di essere una proprietà del layout per diventare una coincidenza dei dati.

⚠️ **Gli anchor devono essere STIRATI su entrambi gli assi** (`Minimum != Maximum` su X e su Y). Con anchor a punto gli offset cambiano significato — `Right` diventa la larghezza — ed è il difetto che portò `ZoneBottom` a `Y 1080..1280`, fuori schermo.

- [ ] **Step 3: Migrare i cinque inquilini**

Spostare ogni widget esistente dentro il `Content` della sua zona nuova:

| Widget | Da | A |
|---|---|---|
| `WBP_RT_TurnHeader` | `Zone_Top` | `Zone_TopCenter` ▸ `Content` |
| `WBP_RT_TeamRosterLeft` | `ZoneLeft` | `Zone_TopLeft` ▸ `Content` |
| `WBP_RT_EventLogRight` | `ZoneRight` | `Zone_MiddleRight` ▸ `Content` |
| `WBP_RT_SelectedUnitPanelBottom` | `ZoneBottom` | `Zone_MiddleLeft` ▸ `Content`, **rinominato `WBP_RT_SelectedUnitPanelLeft`** |
| `WBP_RT_ActionDockBottom` | `ZoneBottomContainer` | `Zone_BottomCenter` ▸ `Content` |

⚠️ **Il rename non è cosmetico**: è l'unica istanza che cambia fascia, e `…Bottom` in `Zone_MiddleLeft` direbbe il falso. [#1896](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1896) è stata diagnosticata leggendo il nome di un'istanza in un log PIE — un nome che mente devia la prossima lettura.

Poi **cancellare** i contenitori vuoti: `Zone_Top`, `ZoneLeft`, `ZoneRight`, `ZoneBottom`, `ZoneBottomContainer`.

✅ Nessun C++ li nomina, quindi la rimozione non rompe codice — misurato:
```
grep -rn "ZoneBottomContainer\|ZoneLeft\|ZoneRight\|Zone_Top\|ZoneBottom" \
  Source/RefactorTactics/ --include=*.cpp --include=*.h | grep -v /Tests/   → nessuna riga
```

`Zone_TopRight`, `Zone_BottomLeft` e `Zone_BottomRight` restano col `Content` **vuoto**: sono le zone i cui inquilini non esistono ancora.

- [ ] **Step 4: Salvare, poi RILEGGERE dal disco**

```bash
cd D:/Repositories/refactor-tactics-main
python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset --filtro Zone --unici
```

🔴 **Atteso: gli otto nomi nuovi (`Zone_TopLeft` … `Zone_BottomRight`) e NESSUNO dei vecchi.** Se compare anche solo `ZoneBottomContainer`, l'Editor ha risalvato una copia anteriore — rifare il passo 3, non proseguire.

```bash
python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset --filtro WBP_RT_ --unici
```

Atteso: gli stessi widget dello Step 1 (con `SelectedUnitPanelLeft` al posto di `…Bottom`), più `WBP_RT_HudZone`.

- [ ] **Step 5: Eseguire l'intera suite `ScreenHud`**

```powershell
& "D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" `
  -ExecCmds="Automation RunTests RefactorTactics.ScreenHud+Quit" `
  -unattended -nopause -nullrhi -NoSound
```

```bash
grep -E "Test Completed|Success|Fail" Saved/Logs/RefactorTactics.log | tail -40
```

Atteso, confrontando con la baseline del Task 4 Step 4:

| Test | Atteso |
|---|---|
| `TheEightZonesAreDeclaredExactlyOnce` | 🟢 **da rosso a verde** |
| `ZoneRectanglesMatchTheThreeByThreeGrid` | 🟢 **da rosso a verde** |
| `PanelsLeaveTheCenterFree` | verde, e ora misura **otto** zone invece di quattro |
| `TheHudMountsTheFeedThatExplainsTheTurn` | verde |
| `NoNodeWearsTheNameOfAWidgetWithoutBeingOne` | verde |
| `EveryZoneOwnerIsMountedByClass` | verde (già lo era: verifica per classe, non per posizione) |
| `HudZoneColorsAreAllDistinct`, `HudZoneNamesCoverEveryValue` | verdi (non dipendono dall'asset) |

⚠️ **`TheHudMountsTheFeedThatExplainsTheTurn` è il rischio dichiarato nella spec §6.2.** Dipende dal fatto che il contenuto di un `NamedSlot` finisca nel `WidgetTree` del contenitore — comportamento atteso da UMG, **mai misurato su questo albero**. Se cade qui, il fallimento è previsto: la correzione è nel test (deve scendere anche nei `NamedSlot`), non nell'asset.

- [ ] **Step 6: Commit**

```bash
git add Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset
git commit -m "feat(hud): le otto zone della griglia, e i cinque inquilini dentro

Zone_Top, ZoneLeft, ZoneRight, ZoneBottom e ZoneBottomContainer escono; le otto
celle della griglia 3x3 meno il centro entrano, con anchor stirati su entrambi
gli assi — con anchor a punto gli offset cambiano significato, ed e' il difetto
che porto' ZoneBottom fuori schermo a Y 1080..1280.

SelectedUnitPanelBottom e' rinominato ...Left perche' cambia fascia: e' l'unica
istanza il cui suffisso direbbe il falso, e #1896 fu diagnosticata leggendo il
nome di un'istanza in un log PIE.

Riletto dal disco dopo il salvataggio:
  names.py --filtro Zone -> gli otto nomi nuovi, nessuno dei vecchi

Verification:
- Compile: PASS
- Tests: PASS — i due gate dei Task 3 e 4 passano da rosso a verde
- PIE: NOT RUN
- Determinism / Replay / Privacy / Packaged: N/A

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

### Task 7: Allineare la guida e il vocabolario dei gate, e aprire il follow-up

**Files:**
- Modify: `docs/technical/runbooks/guida-screen-hud-umg.md` (§3)
- Modify: `Source/RefactorTactics/Tests/RTMatchWidgetAssetTests.cpp` (vocabolario zone in `EveryZoneOwnerIsMountedByClass`; pulizia del commento «sette widget»)

**Interfaces:** nessuna.

🔴 **Questo task va nello STESSO giro del rimontaggio (Task 5-6), non dopo.** Il gate nuovo
(`TheEightZonesAreDeclaredExactlyOnce`) cita già «guida-screen-hud-umg.md §3» in ciascuno degli otto errori
del rosso voluto, e `EveryZoneOwnerIsMountedByClass` — che resta verde prima e dopo il rimontaggio — cita la
stessa §3 col vocabolario `TOP`/`LEFT`/`RIGHT`/`BOTTOM`. Finché §3 descrive cinque zone col vocabolario
vecchio, ogni messaggio che la cita rimanda a un documento che non corrisponde più all'albero appena
rimontato: chiudere il Task 6 e rimandare questo lascerebbe quella finestra aperta sul branch padre.

- [ ] **Step 1: Riscrivere §3 sulle otto zone, e il vocabolario di `EveryZoneOwnerIsMountedByClass`**

Sostituire il diagramma a quattro zone e la tabella «Contiene oggi» con la griglia 3×3 e la tabella del Task 6 Step 2. Tenere:
- il paragrafo su `CENTER` come zona a contratto negativo;
- il riquadro 🔴 sul centro libero come requisito;
- l'avvertenza sul `Canvas Panel` invece della `Vertical Box`;
- **i riquadri storici** su `cc5ca967` e `#2760`: sono la ragione per cui i gate esistono, e cancellarli renderebbe i test inspiegabili.

Aggiungere una riga che nomina i due gate nuovi e cosa chiedono.

Nello stesso passo, riscrivere l'array `Attesi[]` di `EveryZoneOwnerIsMountedByClass`
(`RTMatchWidgetAssetTests.cpp`) sul vocabolario a otto zone: `TOP` → `TopCenter` (TurnHeader), `LEFT` →
`TopLeft` (TeamRoster), `RIGHT` → `MiddleRight` (EventLog), `BOTTOM` → `MiddleLeft` (SelectedUnit, che
cambia fascia) e `BOTTOM` → `BottomCenter` (ActionDock). Il test resta verde prima e dopo — cambia solo
l'etichetta nel messaggio d'errore — ma lasciato al vocabolario vecchio continuerebbe a rimandare a una §3
che nel frattempo è cambiata sotto di lui.

- [ ] **Step 2: Correggere la riga su `FastDecision`**

§3 dichiara che `BOTTOM` contiene *«`WBP_RT_FastDecision` a runtime»*. È falso, misurato:

```
python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset --filtro FastDecision  → 0 nomi
grep -rn "URTFastDecisionWidget" Source/RefactorTactics/ --include=*.cpp | grep -v /Tests/
#   → solo i metodi della classe: nessun CreateWidget, nessun AddChild
```

Sostituire l'affermazione con la misura e il rimando alla issue del follow-up.

- [ ] **Step 3: Aprire la issue di follow-up**

```bash
gh issue create --repo DegrassiAaron/refactor-tactics-main \
  --title "La finestra di reazione non e' montata da nessuna parte" \
  --body "Trovato durante il rimontaggio delle otto zone dell'HUD, e non causato da quel lavoro.

\`guida-screen-hud-umg.md\` §3 dichiara che la zona BOTTOM contiene \`WBP_RT_FastDecision\` a runtime.
L'asset non lo referenzia e nessun C++ lo crea:

\`\`\`
python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset --filtro FastDecision
#   -> 0 nomi
grep -rn 'URTFastDecisionWidget' Source/RefactorTactics/ --include=*.cpp | grep -v /Tests/
#   -> solo i metodi della classe: nessun CreateWidget, nessun AddChild
\`\`\`

La finestra di reazione di #166 non puo' comparire in partita, e nessun gate se ne accorge perche'
nessuno chiede se sia montata — la stessa classe di difetto di #2697, su un widget diverso.

Il montaggio e' a runtime, quindi un gate sull'albero non basterebbe: serve una prova che la finestra
compaia quando la reazione si apre."
```

- [ ] **Step 4: Pulizia di un totale volatile preesistente in `RTMatchWidgetAssetTests.cpp`**

Il file dice in tre punti «i sette widget di `Content/RT/UI/Match/`» (il commento in cima al file, il
docstring di `FRTMatchWidgetsLoadTest`, e il messaggio del suo `TestEqual`), mentre l'array `Paths[]` dello
stesso test ne conta di più — il numero esatto lo dà `UE_ARRAY_COUNT(Paths)`, che il codice stesso usa già
nel `TestEqual` invece di ripeterlo in prosa. È un totale volatile **preesistente**, non causato da questo
lavoro — ma il Task 5 aggiunge un altro asset a quella stessa cartella (`WBP_RT_HudZone.uasset`), quindi è
il momento di smettere di fissare un numero in prosa invece di lasciarlo invecchiare ulteriormente.

Sostituire «i sette widget» con una formulazione che non conti a mano — ad esempio «i widget di
`Content/RT/UI/Match/` elencati in `Paths[]`» — nei tre punti sopra.

- [ ] **Step 5: Commit**

```bash
git add docs/technical/runbooks/guida-screen-hud-umg.md Source/RefactorTactics/Tests/RTMatchWidgetAssetTests.cpp
git commit -m "docs(hud): la guida descrive le otto zone, e i gate parlano lo stesso vocabolario

§3 diceva che il BOTTOM contiene WBP_RT_FastDecision a runtime. L'asset non lo
referenzia e nessun C++ lo crea — misurato con names.py e grep, entrambi nel
testo. La riga e' sostituita dalla misura e dal rimando alla issue.

EveryZoneOwnerIsMountedByClass passava dal vocabolario TOP/LEFT/RIGHT/BOTTOM a
quello a otto zone: restava verde con un'etichetta che rimandava a una sezione
gia' cambiata sotto di lui.

Rimosso anche un totale volatile preesistente ('i sette widget di
Content/RT/UI/Match/'): l'array Paths[] ne contava di piu' gia' prima di questo
lavoro, che ne aggiunge un altro.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

## Verifica finale

Prima di aprire la PR:

- [ ] Suite completa `RefactorTactics.ScreenHud` verde, confrontata con la baseline del Task 4 Step 4
- [ ] `names.py --filtro Zone` rende gli otto nomi nuovi e nessuno dei vecchi
- [ ] **PIE**: aprire una partita e guardare il blockout — otto riquadri colorati distinti, centro sgombro, nessuna zona fuori schermo. È l'unico gate che vede l'aspetto; se non viene eseguito, si dichiara `NOT RUN` col nome del clone che tiene il motore.

PR con base il **branch padre**, non `main` a prescindere:

```bash
git config branch.design/hud-otto-zone-blockout.parent   # oppure: git merge-base --fork-point main HEAD
```

## Cosa resta fuori, e diventa il lavoro successivo

Le zone `TopRight`, `BottomLeft` e `BottomRight` restano vuote, e `MiddleRight` contiene solo il feed. I quattro contenuti che §6 descrive e che **non esistono** — Objective, Team Intent, Ghost Timeline, Confirm/Undo — sono il passo dopo, e ciascuno merita il proprio giro di design: hanno sorgenti dati diverse e vincoli di privacy diversi (Team Intent tocca `URTIntentPrivacyLibrary::FilterForTeam`, gli altri no).
