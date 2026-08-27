# Conoscenza parziale — Fase A: la porta e le unità · Piano di implementazione

> **Per esecutori agentici:** SUB-SKILL RICHIESTA — usa `superpowers:subagent-driven-development`
> (consigliata) oppure `superpowers:executing-plans` per eseguire questo piano task per task. Gli step usano
> la sintassi checkbox (`- [ ]`) per il tracciamento.

**Obiettivo:** far sì che il giocatore veda solo ciò che la sua squadra conosce — e capisca perché, quando
un'azione viene rifiutata.

**Architettura:** nasce una **porta filtrata** (`FRTKnowledgeView`), nucleo puro e headless più un
adattatore sottile, sul modello letterale di `FRTPlannedIntent → FilterForTeam → FRTIntentView` che nel
repository è già vivo e verde. Quattro consumatori la attraversano: il testo del rifiuto, l'HUD, il combat
log, la visibilità dell'unità. La sagoma dell'ultimo contatto chiude la fase.

**Stack:** UE 5.8.1 · C++ · Unreal Automation Framework (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`) · nessuna
dipendenza nuova.

**Spec:** [`docs/technical/systems/conoscenza-parziale-visibile-spec.md`](../../technical/systems/conoscenza-parziale-visibile-spec.md)

**Fasi B e C:** hanno piani propri, da scrivere **dopo** il merge di questa fase — i loro task consumano
l'interfaccia che questa produce, e dettagliarli oggi significherebbe pianificare contro una firma che non
esiste ancora.

---

## Stato di esecuzione — aggiornato il 2026-08-27

| Task | Stato | Commit | Evidenza |
|---|---|---|---|
| **1** — `TargetUnknown` leggibile | ✅ | `7f38ba11` | RED 1/1 col default, GREEN 1/1, regressione 71/71 |
| **2** — la porta `FRTKnowledgeView` | ✅ | `a2b2225f` | RED per **link** (`LNK2019`), GREEN 3/3, **due mutazioni** con rebuild fra l'una e l'altra |
| **3** — `ARTHUD` consuma la porta | ✅ | `06c8c3c1` | RED per **compilazione** (`C3861` ×4), GREEN 4/4, regressione 27/27 |
| **4** — il combat log passa dal filtro | ✅ | `e3e61936` · corretto da due giri di fix del 2026-08-27 | 14 siti `AddLogEvent` convertiti. ⚠️ **Il canale primario era rimasto fuori**: `ConcludeTurn` deriva l'intero log dal TurnLog e passava ogni riga senza soggetto — chiuso nel primo giro di fix con `DescribeTurnLogWithSubjects`. ⚠️ **E sette siti lo disfacevano**: rieccheggiavano *verbatim*, senza soggetto, la voce che avevano appena scritto nel TurnLog — chiusi nel secondo giro. Censimento aggiornato e sue trappole nel Task 4 qui sotto |
| **5** — l'unità ignota sparisce | ✅ | `678cc8fc` · corretto da giro di fix del 2026-08-27 | `RefreshComponentVisibility` + `SetActorEnableCollision`. ⚠️ Due difetti trovati alla review finale e chiusi nel giro di fix: il predicato non distingueva `Live` da `Remembered` (**C1**), e la funzione sovrascriveva la visibilità decisa da altri owner (**I3**) |
| **6** — la sagoma del ricordo | ✅ | `ebfee2a9` · `d8fdaed4` · `ad49b166` · `a52df0db` | Sagoma sganciata dal transform del padre, `ContactTurn` sulla voce, dispatch da `DrawHUD` con la sua rete di test, e il materiale `M_LastContactGhost` entrato nel repository. ✅ **M8 chiuso** dal secondo giro di fix del 2026-08-27: `GhostOpacityForContact` valeva `1.0` nel turno del contatto — cioè opaca nel caso **più frequente**, contro S4 che dichiara una sagoma *semitrasparente*. Ora `0.75` a contatto fresco, `0.45` al turno dopo, `0` oltre; il test asserisce l'intervallo aperto, non il numero |

**Suite completa sull'albero del giro di fix (2026-08-27)**: `Found 1215 automation tests`, **1215 eseguiti,
1215 successi, 0 fallimenti**. Dichiarati ed eseguiti coincidono, misurato con **due** metodi: i nomi
`RefactorTactics.*` estratti dalle macro `IMPLEMENT_*_AUTOMATION_TEST` nei sorgenti e i `Path={...}` del log
di esecuzione danno lo stesso insieme di **1215**, con differenza vuota in entrambi i versi.

*(La riga diceva `1204`, misurato prima dei commit `d49c5dad`/`a52df0db` e dei sei test aggiunti dal giro di
fix. Un conteggio letterale invecchia da solo: si rimisura, non si copia.)*

⏳ **Verifiche manuali pendenti**: `PIE-KNOW1` (overlay, Task 3), `PIE-KNOW2` (modello e collisione, Step
5.7), `PIE-KNOW3` (sagoma monocroma e dissolvenza, Step 6.8) e `PIE-KNOW4` (il ricordo si vede **una volta
sola**, il difetto C1), registrate in
[`test-manuali-pie.md`](../../technical/test-manuali-pie.md). Sono l'unica prova che il **cablaggio**
funzioni: `DrawHUD` non ha test automatici, e i `Knowledge.*` esercitano statiche pure.

> ⚠️ **Le 53 caselle qui sotto restano vuote, e questa tabella è la ragione per cui va bene.** Il progresso
> per-task vive nel workspace di esecuzione, che è **gitignorato** (`.gitignore:187`): senza questo blocco,
> dopo un merge nessun file versionato direbbe quali task sono fatti. Le caselle sono la lista operativa di
> chi esegue; questa tabella è il consuntivo di chi legge dopo.

---

## Vincoli globali

Valgono per **ogni** task. Non si ripetono nei singoli step.

- **UE 5.8.1.** Non inventare API: verifica le firme realmente presenti.
- **Indentazione a TAB**, in tutti i file toccati. Un diff con spazi non applica.
- **`RefactorTactics.Build.cs` non si tocca.** Non contiene alcun elenco di sorgenti: UBT raccoglie
  ricorsivamente sotto `ModuleDirectory`. Misurato in due modi — il file non nomina nessun `.cpp`, e i
  commit che hanno aggiunto file di test non l'hanno modificata.
- **Ogni file di test** inizia con `#include "Misc/AutomationTest.h"` come **prima riga**, apre
  `#if WITH_DEV_AUTOMATION_TESTS` dopo gli include, e **finisce** con la riga esatta
  `#endif // WITH_DEV_AUTOMATION_TESTS` **senza nulla dopo**. Lo verifica un oracolo meta,
  `RefactorTactics.Meta.TestGuardClosesAtEndOfFile`: un carattere di coda rompe la suite.
- **Macro unica**: `IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRT<Soggetto>Test, "RefactorTactics.<Gruppo>.<Asserzione>",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)`. È l'unica forma nel
  repository, 1197 usi su 1197.
- **Firma di RunTest**: `bool FNomeClasse::RunTest(const FString&)` — parametro **senza nome**, in tutto il
  repository. Il corpo termina con `return true;`.
- **Gli helper nei namespace anonimi devono avere nomi unici per file**: la unity build condivide la
  translation unit, e due helper omonimi in file diversi non compilano.
- ⚠️ **`RTKnowledgeViewTests.cpp` è condiviso da quattro task.** Il Task 2 lo crea; i Task 3, 4, 5 e 6 vi
  aggiungono test **prima** dell'`#endif` finale, e i loro `#include` vanno **prima** di
  `#if WITH_DEV_AUTOMATION_TESTS`, mai dopo: un include dentro la guardia sparisce in Shipping.
- **Niente `SetActorLocation`, `ApplyDamage` o `if (IsTest)`** che aggirino il gameplay nei test.
- **Nessun commit, push o merge** senza richiesta esplicita dell'autore.

### Comando dei test

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests <FiltroTest>; Quit" \
  -unattended -nopause -nullrhi -NoSound
```

⚠️ **Una run per volta.** Non lanciare l'automation con `Start-Process` né in parallelo: due istanze si
strozzano a vicenda e il risultato non è leggibile.
⚠️ **Leggi `Found N tests` nel log**: una run può eseguirne meno di quanti ne dichiari, e «N eseguiti su M
dichiarati» sono **due** numeri.

### Precondizione bloccante — da verificare PRIMA del Task 1

```bash
git status --porcelain
```

Il working tree deve essere **pulito** per i binari. Al momento della stesura conteneva
`Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset` modificato e `WBP_RT_TurnHeader.uasset` untracked, sopra
CP 11.7 (#613, aperta). Il **Task 6 crea un materiale nuovo**: due `.uasset` non si fondono, e D-178
prescrive una sessione, una working directory, un branch. I Task 1–5 non toccano binari e possono procedere
comunque.

---

## Struttura dei file

| File | Responsabilità | Task |
|---|---|---|
| `Source/RefactorTactics/Turn/RTTurnLogLibrary.cpp` | *modifica* — la tabella dei motivi di rifiuto guadagna la voce mancante | 1 |
| `Source/RefactorTactics/Perception/RTKnowledgeView.h` | *nuovo* — `FRTKnowledgeSubject`, `FRTKnowledgeEntry`, `FRTKnowledgeView`, firme della porta | 2 |
| `Source/RefactorTactics/Perception/RTKnowledgeView.cpp` | *nuovo* — il nucleo puro: da conoscenza + soggetti a vista osservabile | 2 |
| `Source/RefactorTactics/UI/RTHUD.h` / `.cpp` | *modifica* — una statica pura decide cosa disegnare; `DrawHUD` la consuma | 3 |
| `Source/RefactorTactics/Turn/RTTurnManager.cpp` | *modifica* — le righe leggibili del combat log passano dal filtro | 4 |
| `Source/RefactorTactics/Unit/RTUnit.h` / `.cpp` | *modifica* — `SetKnownToObserver`, e la visibilità come funzione di *vivo* **e** *noto* | 5, 6 |
| `Source/RefactorTactics/Tests/RTKnowledgeViewTests.cpp` | *nuovo* — la suite della porta | 2 |

La porta vive in `Perception/`, accanto a `RTTeamKnowledge`. **Mai** in `UI/`: la conoscenza è regola, non
presentazione. **Mai** in `RTTurnLog*`: il log è uno solo e non deve conoscere osservatori.

---

## Task 1: Il rifiuto per bersaglio ignoto diventa leggibile

**File:**
- Modifica: `Source/RefactorTactics/Turn/RTTurnLogLibrary.cpp` — `URTTurnLogLibrary::DescribeInvalidReason`
- Test: `Source/RefactorTactics/Tests/RTTurnLogLibraryTests.cpp` (esistente: aggiungi **prima** dell'`#endif` finale)

**Interfacce:**
- Consuma: nulla. È il task indipendente.
- Produce: nulla di programmatico. Cambia una stringa che due chiamanti già usano —
  `URTTurnLogLibrary::DescribeEntry` (voci di categoria `Fallback`) e
  `ARTTurnManager::ValidatePlansAtLockIn` (combat log al lock-in). Il commento in
  `RTTurnLogLibrary.h` dichiara l'invariante: *«UNA tabella sola … Due tabelle divergerebbero al primo
  motivo aggiunto — ed è successo»*.

**Contesto misurato.** `ERTActionInvalidReason` ha **11** valori; `DescribeInvalidReason` ne copre **8**.
`TargetUnknown` non ha un `case` e cade nel `default`, che dice `"non eseguibile"`. Il valore viaggia come
`int32` in `FRTTurnLogEntry::Amount`, quindi **non si può riordinare l'enum**: le tracce già scritte
cambierebbero significato.

- [ ] **Step 1.1 — Scrivi il test che fallisce**

Inserisci in `Source/RefactorTactics/Tests/RTTurnLogLibraryTests.cpp`, subito **prima** della riga
`#endif // WITH_DEV_AUTOMATION_TESTS`:

```cpp
/**
 * `TargetUnknown` deve avere un testo PROPRIO. Cade nel `default` -> il giocatore legge «non eseguibile»,
 * che e' esattamente cio' che la conoscenza parziale NON sta dicendo: non «non si puo'», ma «per la tua
 * squadra quel bersaglio non c'e'».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogTargetUnknownIsDescribedTest,
	"RefactorTactics.TurnLog.TargetUnknownIsDescribed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogTargetUnknownIsDescribedTest::RunTest(const FString&)
{
	const FString Fallback = URTTurnLogLibrary::DescribeInvalidReason(
		ERTActionInvalidReason::InsufficientMovementPoints);
	const FString Unknown = URTTurnLogLibrary::DescribeInvalidReason(
		ERTActionInvalidReason::TargetUnknown);

	// ⚠️ Anti-vacuita': senza questa riga il test passerebbe anche cambiando il TESTO DEL DEFAULT invece di
	// aggiungere il case. `InsufficientMovementPoints` non ha un case e per D-190 non ne avra' mai uno,
	// quindi e' la sonda giusta per dimostrare che il default e' ancora raggiungibile e ancora quello.
	TestEqual(TEXT("il default esiste ancora ed e' invariato"), Fallback, TEXT("non eseguibile"));

	TestNotEqual(TEXT("TargetUnknown non cade piu' nel default"), Unknown, Fallback);
	TestEqual(TEXT("e dice di CONOSCENZA, non di geometria"),
		Unknown, TEXT("bersaglio ignoto alla squadra"));
	return true;
}
```

- [ ] **Step 1.2 — Esegui il test e verifica che FALLISCA**

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics.TurnLog.TargetUnknownIsDescribed; Quit" \
  -unattended -nopause -nullrhi -NoSound
```

Atteso: **FAIL** su `TargetUnknown non cade piu' nel default` — entrambe le stringhe valgono
`"non eseguibile"`. È il rosso giusto: prova che il buco esiste davvero.

- [ ] **Step 1.3 — Aggiungi il `case`**

In `Source/RefactorTactics/Turn/RTTurnLogLibrary.cpp`, dentro `DescribeInvalidReason`, inserisci la riga
**dopo** `NoMap` e **prima** di `SlotOccupied` (l'ordine dei `case` segue l'enum):

```cpp
	case ERTActionInvalidReason::NoMap:          return TEXT("nessuna mappa autorevole");
	case ERTActionInvalidReason::TargetUnknown:  return TEXT("bersaglio ignoto alla squadra");
	case ERTActionInvalidReason::SlotOccupied:   return TEXT("lo slot e' gia' occupato");
```

- [ ] **Step 1.4 — Esegui il test e verifica che PASSI**

Stesso comando dello Step 1.2. Atteso: **PASS**.

- [ ] **Step 1.5 — Regressione sui due chiamanti**

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics.TurnLog+RefactorTactics.Actions.Fallback+RefactorTactics.PlayerInteraction; Quit" \
  -unattended -nopause -nullrhi -NoSound
```

Atteso: **tutti PASS**. Quattro test indiretti asseriscono stringhe di `DescribeInvalidReason`
(`Actions.Fallback.LoggedOutcome`, `PlayerInteraction.LockInDescribesTheCooldownReason`,
`Actions.Fallback.CancelIsLoggedInMatch`, `Actions.KitDeclaringBothSlotsDeclaresTheDiscard`): nessuno di
essi usa `TargetUnknown`, quindi devono restare verdi. Se uno diventa rosso, hai toccato la riga sbagliata.

- [ ] **Step 1.6 — Commit**

```bash
git add Source/RefactorTactics/Turn/RTTurnLogLibrary.cpp Source/RefactorTactics/Tests/RTTurnLogLibraryTests.cpp
git commit -m "fix(turnlog): il rifiuto per bersaglio ignoto smette di dire 'non eseguibile'"
```

---

## Task 2: La porta filtrata

**File:**
- Crea: `Source/RefactorTactics/Perception/RTKnowledgeView.h`
- Crea: `Source/RefactorTactics/Perception/RTKnowledgeView.cpp`
- Test: `Source/RefactorTactics/Tests/RTKnowledgeViewTests.cpp` (nuovo)

**Interfacce:**
- Consuma: `FRTTeamKnowledge`, `FRTLastKnownContact`, `URTTeamKnowledgeLibrary::ClassifyTarget` e
  `::LastKnownCell` da `Perception/RTTeamKnowledge.h`; `FRTCellId` da `Map/RTCellId.h`.
- Produce, e i task 3, 4, 5 e 6 ci si appoggiano:
  - `struct FRTKnowledgeSubject { int32 StableUnitId; int32 TeamId; FRTCellId Cell; FName HeroId; FText HeroDisplayName; bool bAlive; }`
  - `enum class ERTKnowledgeVisibility : uint8 { Live, Remembered }`
  - `struct FRTKnowledgeEntry { int32 StableUnitId; ERTKnowledgeVisibility Visibility; FRTCellId Cell; FName HeroId; FText HeroDisplayName; }`

  ⚠️ **I tipi sono misurati su `ARTUnit`, non dedotti dal nome**: `HeroId` è `FName`, `HeroDisplayName` è
  **`FText`** (`RTUnit.h:157`), `IsAlive()` è `Health > 0`. Una prima stesura di questo piano dichiarava
  `FName HeroDisplayName` e interrogava `IsNone()`: non compilava.
  - `struct FRTKnowledgeView { int32 ObserverTeamId; TArray<FRTKnowledgeEntry> Entries; }`
  - `static FRTKnowledgeView URTKnowledgeViewLibrary::ViewForTeam(const FRTTeamKnowledge& Knowledge, const TArray<FRTKnowledgeSubject>& Subjects, int32 ObserverTeamId);`
  - `static const FRTKnowledgeEntry* URTKnowledgeViewLibrary::FindEntry(const FRTKnowledgeView& View, int32 StableUnitId);`

**Le tre regole che il nucleo realizza**, e sono la ragione per cui esiste:

1. Un soggetto della **propria** squadra entra sempre, `Live`, con la cella attuale.
2. Un avversario `Allowed` entra `Live` con la cella attuale.
3. Un avversario `CellOnly` entra `Remembered` con la **cella del ricordo**, mai quella attuale.
4. Un avversario `Rejected` **non entra**. Nessuna voce, nessun flag.

⚠️ **`FRTKnowledgeEntry` non porta la condizione** (HP, scudo): la squadra conosce l'identità, non lo stato.
È il confine che #160 ha già fissato per il bot (*«`CellOnly` → HP massimi»*), e portare qui un campo HP
costringerebbe il consumatore a inventarne un valore.

- [ ] **Step 2.1 — Scrivi l'header**

Crea `Source/RefactorTactics/Perception/RTKnowledgeView.h`:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Perception/RTTeamKnowledge.h" // FRTTeamKnowledge, ERTTargetKnowledge
#include "RTKnowledgeView.generated.h"

/**
 * Un soggetto ridotto a cio' che serve per decidere SE SI SA. NON e' un'unita': prendere `ARTUnit`
 * legherebbe una regola pura al mondo di gioco, come `FRTPerceiver` evita di fare per la percezione.
 */
USTRUCT(BlueprintType)
struct FRTKnowledgeSubject
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Knowledge")
	int32 StableUnitId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Knowledge")
	int32 TeamId = 0;

	/** Cella ATTUALE. E' informazione autorevole: non deve attraversare la porta per un ignoto. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Knowledge")
	FRTCellId Cell;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Knowledge")
	FName HeroId;

	/** ⚠️ `FText`, non `FName`: e' il tipo che `ARTUnit::HeroDisplayName` ha davvero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Knowledge")
	FText HeroDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Knowledge")
	bool bAlive = true;

	FRTKnowledgeSubject() = default;
};

/** Se cio' che si sa e' presente ORA, oppure e' un ricordo. */
UENUM(BlueprintType)
enum class ERTKnowledgeVisibility : uint8
{
	/** La squadra lo vede: cella attuale, rappresentazione normale. */
	Live,
	/** Solo un ricordo: cella dell'ULTIMO CONTATTO, sagoma. Mai la posizione vera. */
	Remembered
};

/**
 * Cosa un osservatore puo' sapere di UN soggetto.
 *
 * ⚠️ Non porta la CONDIZIONE (HP, scudo). La squadra conosce l'identita', non lo stato: e' il confine che
 * il bot rispetta gia' (`CellOnly` -> HP massimi). Un campo qui costringerebbe a inventarne il valore.
 */
USTRUCT(BlueprintType)
struct FRTKnowledgeEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	int32 StableUnitId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	ERTKnowledgeVisibility Visibility = ERTKnowledgeVisibility::Live;

	/** Attuale se `Live`, del CONTATTO se `Remembered`. Chi legge non deve sapere quale delle due. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	FRTCellId Cell;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	FName HeroId;

	/** ⚠️ `FText`, come su `ARTUnit`. `FText` non ha `IsNone()`: si interroga con `IsEmpty()`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	FText HeroDisplayName;

	FRTKnowledgeEntry() = default;
};

/**
 * Il mondo come un osservatore puo' vederlo.
 *
 * 🔴 Un soggetto IGNOTO non e' una voce con un flag: **non c'e' nessuna voce**. Un flag si puo' leggere per
 * sbaglio; un campo che non esiste no. E' la stessa disciplina di `FRTPlannedIntent -> FilterForTeam ->
 * FRTIntentView`, che nel progetto e' gia' viva e verde.
 */
USTRUCT(BlueprintType)
struct FRTKnowledgeView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	int32 ObserverTeamId = 0;

	/** Ordinate per `StableUnitId`: ordine canonico, mai quello di scoperta. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	TArray<FRTKnowledgeEntry> Entries;

	FRTKnowledgeView() = default;
};

/**
 * La porta fra lo stato autorevole e la presentazione (CP 13.5).
 *
 * Pura e headless: nessun Actor, nessun `UWorld`, nessuno snapshot. E' anche la ragione per cui NON prende
 * `FRTHexSnapshot`: `MakeCurrentSnapshot` fa `GetAllActorsOfClass` e due `Sort`, e il disegno gira a ogni
 * frame; inoltre `FRTHexSimUnit` non porta `TeamId`, quindi non basterebbe.
 */
UCLASS()
class REFACTORTACTICS_API URTKnowledgeViewLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Knowledge")
	static FRTKnowledgeView ViewForTeam(const FRTTeamKnowledge& Knowledge,
		const TArray<FRTKnowledgeSubject>& Subjects, int32 ObserverTeamId);

	/** La voce di un soggetto, o `nullptr` se l'osservatore non ne sa nulla. */
	static const FRTKnowledgeEntry* FindEntry(const FRTKnowledgeView& View, int32 StableUnitId);
};
```

- [ ] **Step 2.2 — Scrivi i test, che non compileranno**

Crea `Source/RefactorTactics/Tests/RTKnowledgeViewTests.cpp`:

```cpp
#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Perception/RTKnowledgeView.h"
#include "Perception/RTTeamKnowledge.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 13.5 — la porta fra lo stato autorevole e la presentazione.
 *
 * Il test che conta e' `ViewIsIndependentOfHiddenState`: e' il gemello umano di
 * `HexBotPlay.HiddenEnemyFairness`, ed e' il debito che D-143 assegna al primo consumatore che introduca un
 * overlay di conoscenza. Questa fase e' quel consumatore.
 */

namespace
{
	/** Nome distinto per file: la unity build condivide la translation unit. */
	FRTKnowledgeSubject KvSubject(int32 StableId, int32 TeamId, const FRTCellId& Cell)
	{
		FRTKnowledgeSubject S;
		S.StableUnitId = StableId;
		S.TeamId = TeamId;
		S.Cell = Cell;
		S.HeroId = FName(*FString::Printf(TEXT("Hero%d"), StableId));
		S.HeroDisplayName = FText::FromString(FString::Printf(TEXT("Eroe %d"), StableId));
		S.bAlive = true;
		return S;
	}

	/** Conoscenza della squadra 0 al turno 5, con i contatti passati esplicitamente. */
	FRTTeamKnowledge KvKnowledge(const TArray<FRTLastKnownContact>& Contacts)
	{
		FRTTeamKnowledge K;
		K.TeamId = 0;
		K.TurnNumber = 5;
		K.Contacts = Contacts;
		return K;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeViewOmitsHiddenTest,
	"RefactorTactics.Knowledge.ViewOmitsHidden",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeViewOmitsHiddenTest::RunTest(const FString&)
{
	// Squadra 0: un alleato (id 1) e due avversari — uno visto ora (id 2), uno ignoto (id 3).
	const FRTCellId AllyCell(0, 0, 0);
	const FRTCellId SeenCell(3, 0, 0);
	const FRTCellId HiddenCell(7, 0, 0);

	FRTTeamKnowledge K = KvKnowledge({ FRTLastKnownContact(2, SeenCell, 5) });
	K.VisibleCells.Add(SeenCell);

	const TArray<FRTKnowledgeSubject> Subjects = {
		KvSubject(1, 0, AllyCell),
		KvSubject(2, 1, SeenCell),
		KvSubject(3, 1, HiddenCell)
	};

	const FRTKnowledgeView View = URTKnowledgeViewLibrary::ViewForTeam(K, Subjects, /*ObserverTeamId*/ 0);

	TestEqual(TEXT("l'alleato e il nemico visto: due voci, non tre"), View.Entries.Num(), 2);
	TestNotNull(TEXT("l'alleato c'e'"), URTKnowledgeViewLibrary::FindEntry(View, 1));
	TestNotNull(TEXT("il nemico visto c'e'"), URTKnowledgeViewLibrary::FindEntry(View, 2));

	// 🔴 Il cuore: NESSUNA voce, non una voce con un flag.
	TestNull(TEXT("l'ignoto non ha una voce"), URTKnowledgeViewLibrary::FindEntry(View, 3));

	// E la sua cella non compare da nessuna parte nella vista.
	for (const FRTKnowledgeEntry& E : View.Entries)
	{
		TestFalse(TEXT("la cella dell'ignoto non trapela"), E.Cell == HiddenCell);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeViewIsIndependentOfHiddenStateTest,
	"RefactorTactics.Knowledge.ViewIsIndependentOfHiddenState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeViewIsIndependentOfHiddenStateTest::RunTest(const FString&)
{
	// Lo stesso osservatore, due stati autoritativi DIVERSI: il nemico ignoto sta in A oppure in B.
	// Se la vista differisce, l'informazione e' passata.
	const FRTCellId SeenCell(3, 0, 0);
	FRTTeamKnowledge K = KvKnowledge({ FRTLastKnownContact(2, SeenCell, 5) });
	K.VisibleCells.Add(SeenCell);

	const TArray<FRTKnowledgeSubject> WorldA = {
		KvSubject(1, 0, FRTCellId(0, 0, 0)),
		KvSubject(2, 1, SeenCell),
		KvSubject(3, 1, FRTCellId(7, 0, 0))
	};
	const TArray<FRTKnowledgeSubject> WorldB = {
		KvSubject(1, 0, FRTCellId(0, 0, 0)),
		KvSubject(2, 1, SeenCell),
		KvSubject(3, 1, FRTCellId(-7, 2, 0)) // stesso ignoto, dall'altra parte della mappa
	};

	const FRTKnowledgeView A = URTKnowledgeViewLibrary::ViewForTeam(K, WorldA, 0);
	const FRTKnowledgeView B = URTKnowledgeViewLibrary::ViewForTeam(K, WorldB, 0);

	TestEqual(TEXT("stesso numero di voci"), A.Entries.Num(), B.Entries.Num());
	for (int32 i = 0; i < A.Entries.Num() && i < B.Entries.Num(); ++i)
	{
		TestEqual(TEXT("stessa identita'"), A.Entries[i].StableUnitId, B.Entries[i].StableUnitId);
		TestTrue(TEXT("stessa cella"), A.Entries[i].Cell == B.Entries[i].Cell);
		TestTrue(TEXT("stessa visibilita'"), A.Entries[i].Visibility == B.Entries[i].Visibility);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeLastContactCarriesIdentityNotConditionTest,
	"RefactorTactics.Knowledge.LastContactCarriesIdentityNotCondition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeLastContactCarriesIdentityNotConditionTest::RunTest(const FString&)
{
	// Il nemico 2 e' stato visto in (3,0), poi si e' spostato in (6,0) senza essere visto.
	const FRTCellId Remembered(3, 0, 0);
	const FRTCellId Actual(6, 0, 0);

	const FRTTeamKnowledge K = KvKnowledge({ FRTLastKnownContact(2, Remembered, 5) });
	// `VisibleCells` VUOTA: nessuno lo vede ora. Resta solo il ricordo.

	const TArray<FRTKnowledgeSubject> Subjects = { KvSubject(2, 1, Actual) };
	const FRTKnowledgeView View = URTKnowledgeViewLibrary::ViewForTeam(K, Subjects, 0);

	const FRTKnowledgeEntry* E = URTKnowledgeViewLibrary::FindEntry(View, 2);
	if (!TestNotNull(TEXT("il ricordo produce una voce"), E))
	{
		return false;
	}
	TestTrue(TEXT("e' un ricordo, non un contatto vivo"), E->Visibility == ERTKnowledgeVisibility::Remembered);
	TestTrue(TEXT("porta la cella del CONTATTO"), E->Cell == Remembered);
	TestFalse(TEXT("e NON quella attuale"), E->Cell == Actual);
	TestFalse(TEXT("l'identita' c'e'"), E->HeroDisplayName.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 2.3 — Esegui e verifica che NON COMPILI**

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics.Knowledge; Quit" \
  -unattended -nopause -nullrhi -NoSound
```

Atteso: **errore di compilazione** — `RTKnowledgeView.cpp` non esiste, quindi `ViewForTeam` e `FindEntry`
non hanno definizione (link error). È il rosso giusto: un test che compilasse proverebbe che la porta
esisteva già.

- [ ] **Step 2.4 — Scrivi l'implementazione minima**

Crea `Source/RefactorTactics/Perception/RTKnowledgeView.cpp`:

```cpp
#include "Perception/RTKnowledgeView.h"

FRTKnowledgeView URTKnowledgeViewLibrary::ViewForTeam(const FRTTeamKnowledge& Knowledge,
	const TArray<FRTKnowledgeSubject>& Subjects, int32 ObserverTeamId)
{
	FRTKnowledgeView View;
	View.ObserverTeamId = ObserverTeamId;

	for (const FRTKnowledgeSubject& S : Subjects)
	{
		if (!S.bAlive)
		{
			continue; // un morto non e' un soggetto di conoscenza: lo tratta la presentazione della sconfitta
		}

		FRTKnowledgeEntry E;
		E.StableUnitId = S.StableUnitId;
		E.HeroId = S.HeroId;
		E.HeroDisplayName = S.HeroDisplayName;

		if (S.TeamId == ObserverTeamId)
		{
			// La propria squadra si conosce sempre: non passa da `ClassifyTarget`, che risponde alla domanda
			// «posso bersagliarlo?» e per un alleato non e' la domanda giusta.
			E.Visibility = ERTKnowledgeVisibility::Live;
			E.Cell = S.Cell;
			View.Entries.Add(E);
			continue;
		}

		switch (URTTeamKnowledgeLibrary::ClassifyTarget(Knowledge, S.StableUnitId, S.TeamId, S.Cell))
		{
		case ERTTargetKnowledge::Allowed:
			E.Visibility = ERTKnowledgeVisibility::Live;
			E.Cell = S.Cell;
			View.Entries.Add(E);
			break;

		case ERTTargetKnowledge::CellOnly:
		{
			// 🔴 La cella del RICORDO, mai `S.Cell`. Se il ricordo non si legge, non si inventa: nessuna voce.
			FRTCellId Remembered;
			if (URTTeamKnowledgeLibrary::LastKnownCell(Knowledge, S.StableUnitId, Remembered))
			{
				E.Visibility = ERTKnowledgeVisibility::Remembered;
				E.Cell = Remembered;
				View.Entries.Add(E);
			}
			break;
		}

		default:
			break; // Rejected: NESSUNA voce. E' il cuore della porta.
		}
	}

	// Ordine canonico per `StableUnitId`: mai quello di scoperta, che dipenderebbe dall'ordine dei soggetti.
	View.Entries.Sort([](const FRTKnowledgeEntry& A, const FRTKnowledgeEntry& B)
		{ return A.StableUnitId < B.StableUnitId; });
	return View;
}

const FRTKnowledgeEntry* URTKnowledgeViewLibrary::FindEntry(const FRTKnowledgeView& View, int32 StableUnitId)
{
	for (const FRTKnowledgeEntry& E : View.Entries)
	{
		if (E.StableUnitId == StableUnitId)
		{
			return &E;
		}
	}
	return nullptr;
}
```

- [ ] **Step 2.5 — Esegui i test e verifica che PASSINO**

Stesso comando dello Step 2.3. Atteso: **3 test PASS**. Verifica nel log che dica `Found 3 tests`: se ne
trova meno, un nome è sbagliato e stai leggendo un verde che non copre nulla.

- [ ] **Step 2.6 — Verifica di mutazione**

Rompi **una** cosa per volta, riesegui, rimetti a posto:

1. Nel ramo `default:` sostituisci `break;` con `View.Entries.Add(E); break;` →
   `ViewOmitsHidden` **deve** fallire. Se resta verde, il test non misura niente.
2. Nel ramo `CellOnly` sostituisci `E.Cell = Remembered;` con `E.Cell = S.Cell;` →
   `LastContactCarriesIdentityNotCondition` **deve** fallire.

⚠️ Dopo ogni mutazione **ripristina il sorgente e ricompila**: ripristinare senza rebuild fa misurare alla
seconda mutazione il binario della prima.

- [ ] **Step 2.7 — Commit**

```bash
git add Source/RefactorTactics/Perception/RTKnowledgeView.h Source/RefactorTactics/Perception/RTKnowledgeView.cpp Source/RefactorTactics/Tests/RTKnowledgeViewTests.cpp
git commit -m "feat(perception): la porta fra lo stato autorevole e la presentazione"
```

> ⚠️ **Questo task ha una scadenza.** Una porta senza consumatori è un dato che nessuno legge. Se il Task 3
> non chiude nella stessa PR, questa fase non è consegnabile: dichiaralo invece di aprirne un'altra.

---

## Task 3: L'HUD consuma la porta

**File:**
- Modifica: `Source/RefactorTactics/UI/RTHUD.h` — dichiara la statica pura
- Modifica: `Source/RefactorTactics/UI/RTHUD.cpp` — `DrawHUD` la consuma
- Test: `Source/RefactorTactics/Tests/RTKnowledgeViewTests.cpp` (aggiungi prima dell'`#endif`)

**Interfacce:**
- Consuma: `FRTKnowledgeView`, `FRTKnowledgeEntry`, `URTKnowledgeViewLibrary::FindEntry` (Task 2).
- Produce: `static bool ARTHUD::ShouldDrawUnitOverlay(const FRTKnowledgeView& View, int32 StableUnitId, bool bIsOwnTeam);`

**Contesto misurato.** `ARTHUD::DrawHUD` fa `GetAllActorsOfClass(ARTUnit)` e il **solo** filtro del ciclo è
`if (!Unit || !Unit->IsAlive()) continue;`. Nome eroe, barra HP e barretta scudo vengono disegnati per ogni
unità viva di **ogni** squadra. E `DrawHUD` **non ha nessun test**: tutti e 14 i test della HUD esercitano
statiche pure (`ComputePlannedHitMarks`, `ClampOverlayAnchor`, `ComposeSlotLines`, …). Questo task segue la
stessa strada — la decisione va in una statica pura, e quella si testa.

⚠️ Il team del giocatore è un **letterale, due volte**: `RTHUD.cpp:391` passa `/*PlayerTeamId=*/ 0` a
`ComputePlannedHitMarks`, e più sotto un `const int32 PlayerTeam = 0;`. Non aggiungerne un terzo: questo
task introduce **una** costante locale e la riusa.

- [ ] **Step 3.1 — Scrivi il test che fallisce**

Aggiungi in `RTKnowledgeViewTests.cpp`, prima dell'`#endif`, e aggiungi `#include "UI/RTHUD.h"` in cima
al file insieme agli altri include:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeHudDrawsOnlyKnownUnitsTest,
	"RefactorTactics.Knowledge.HudDrawsOnlyKnownUnits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeHudDrawsOnlyKnownUnitsTest::RunTest(const FString&)
{
	const FRTCellId SeenCell(3, 0, 0);
	FRTTeamKnowledge K = KvKnowledge({ FRTLastKnownContact(2, SeenCell, 5) });
	K.VisibleCells.Add(SeenCell);

	const TArray<FRTKnowledgeSubject> Subjects = {
		KvSubject(1, 0, FRTCellId(0, 0, 0)),  // alleato
		KvSubject(2, 1, SeenCell),            // nemico visto
		KvSubject(3, 1, FRTCellId(7, 0, 0))   // nemico ignoto
	};
	const FRTKnowledgeView View = URTKnowledgeViewLibrary::ViewForTeam(K, Subjects, 0);

	TestTrue(TEXT("l'alleato si disegna"), ARTHUD::ShouldDrawUnitOverlay(View, 1, /*bIsOwnTeam*/ true));
	TestTrue(TEXT("il nemico visto si disegna"), ARTHUD::ShouldDrawUnitOverlay(View, 2, false));
	TestFalse(TEXT("il nemico ignoto NON si disegna"), ARTHUD::ShouldDrawUnitOverlay(View, 3, false));

	// Anti-vacuita': un'unita' della propria squadra si disegna anche se, per un difetto della porta, non
	// avesse una voce. Il proprio schieramento non si nasconde mai a se stessi.
	TestTrue(TEXT("la propria squadra non si nasconde mai"),
		ARTHUD::ShouldDrawUnitOverlay(View, 99, /*bIsOwnTeam*/ true));
	return true;
}
```

- [ ] **Step 3.2 — Esegui e verifica che NON COMPILI**

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics.Knowledge.HudDrawsOnlyKnownUnits; Quit" \
  -unattended -nopause -nullrhi -NoSound
```

Atteso: **errore di compilazione** — `ShouldDrawUnitOverlay` non esiste.

- [ ] **Step 3.3 — Dichiara la statica nell'header**

In `Source/RefactorTactics/UI/RTHUD.h`, nel blocco `public:` accanto alle altre statiche pure
(`ComputePlannedHitMarks` e sorelle), aggiungi:

```cpp
	/**
	 * Se l'overlay di un'unita' (nome, barra HP, scudo) va disegnato per questo osservatore.
	 *
	 * Statica e PURA: `DrawHUD` non ha test, quindi la decisione vive qui dove si puo' interrogare — e' la
	 * stessa strada di `ComputePlannedHitMarks`.
	 *
	 * ⚠️ La propria squadra si disegna SEMPRE, anche senza una voce nella vista: nascondere il proprio
	 * schieramento a se' stessi non e' conoscenza parziale, e' un difetto.
	 */
	static bool ShouldDrawUnitOverlay(const FRTKnowledgeView& View, int32 StableUnitId, bool bIsOwnTeam);
```

E aggiungi in cima all'header, fra gli include:

```cpp
#include "Perception/RTKnowledgeView.h" // FRTKnowledgeView: l'HUD legge la vista, non lo stato
```

- [ ] **Step 3.4 — Definisci la statica**

In `Source/RefactorTactics/UI/RTHUD.cpp`, accanto a `ComputePlannedHitMarks`:

```cpp
bool ARTHUD::ShouldDrawUnitOverlay(const FRTKnowledgeView& View, int32 StableUnitId, bool bIsOwnTeam)
{
	if (bIsOwnTeam)
	{
		return true;
	}
	return URTKnowledgeViewLibrary::FindEntry(View, StableUnitId) != nullptr;
}
```

- [ ] **Step 3.5 — Esegui il test e verifica che PASSI**

Stesso comando dello Step 3.2. Atteso: **PASS**.

- [ ] **Step 3.6 — Cabla `DrawHUD` sulla statica**

Nel ciclo di `DrawHUD`, subito **dopo** `if (!Unit || !Unit->IsAlive()) { continue; }`, aggiungi il secondo
filtro. La vista si costruisce **una volta sola prima del ciclo**, non per unità:

```cpp
	// Costruita UNA volta prima del ciclo: `ViewForTeam` e' pura ma il ciclo gira su ogni unita' a ogni frame.
	const int32 PlayerTeamId = 0; // il giocatore e' la squadra 0 (stesso letterale gia' usato due volte qui)
	FRTKnowledgeView KnowledgeView;
	if (TurnManager)
	{
		TArray<FRTKnowledgeSubject> Subjects;
		Subjects.Reserve(AllUnits.Num());
		for (ARTUnit* U : AllUnits)
		{
			if (!U) { continue; }
			FRTKnowledgeSubject S;
			S.StableUnitId = U->StableUnitId;
			S.TeamId = U->TeamId;
			S.Cell = U->Cell;
			S.HeroId = U->HeroId;
			S.HeroDisplayName = U->HeroDisplayName;
			S.bAlive = U->IsAlive();
			Subjects.Add(S);
		}
		KnowledgeView = URTKnowledgeViewLibrary::ViewForTeam(
			TurnManager->KnowledgeForTeamPublic(PlayerTeamId), Subjects, PlayerTeamId);
	}
```

e dentro il ciclo:

```cpp
		if (!ShouldDrawUnitOverlay(KnowledgeView, Unit->StableUnitId, Unit->TeamId == PlayerTeamId))
		{
			continue;
		}
```

⚠️ **`ARTHUD` recupera il `TurnManager` DOPO il ciclo delle unità.** Sposta quel recupero **prima**, o la
vista sarà sempre vuota e il filtro non filtrerà nulla — un verde che non nasconde niente.

- [ ] **Step 3.7 — Esponi la conoscenza sul TurnManager**

`ARTTurnManager::KnowledgeForTeam` non è pubblica. Aggiungi in `RTTurnManager.h`, nel blocco `public:`:

```cpp
	/**
	 * La conoscenza di UNA squadra, per la presentazione. Copia piccola: NON e' `MakeCurrentSnapshot`, che
	 * fa `GetAllActorsOfClass` e due `Sort` ed e' proibitiva a ogni frame.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Perception")
	FRTTeamKnowledge KnowledgeForTeamPublic(int32 TeamId) const { return KnowledgeForTeam(TeamId); }
```

- [ ] **Step 3.8 — Verifica manuale in PIE, e dichiarala**

Avvia una partita, avvicinati e allontanati da un nemico. Atteso: nome e barra HP **compaiono e
scompaiono** col contatto. Registra l'esito come verifica manuale: `DrawHUD` non ha test automatici e
questo passo è l'unica prova che il cablaggio funziona.

- [ ] **Step 3.9 — Commit**

```bash
git add Source/RefactorTactics/UI/RTHUD.h Source/RefactorTactics/UI/RTHUD.cpp Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Tests/RTKnowledgeViewTests.cpp
git commit -m "feat(hud): l'overlay delle unita' passa dalla conoscenza di squadra"
```

---

## Task 4: Il combat log smette di stampare ciò che la squadra non sa

**File:**
- Modifica: `Source/RefactorTactics/Turn/RTTurnManager.h` / `.cpp` — `AddLogEvent`, `RecentEvents`, l'accessor
- Modifica: `Source/RefactorTactics/UI/RTHUD.cpp` — legge l'accessor filtrato
- Test: `Source/RefactorTactics/Tests/RTKnowledgeViewTests.cpp`

**Interfacce:**
- Consuma: `FRTKnowledgeView`, `URTKnowledgeViewLibrary::FindEntry` (Task 2); `KnowledgeForTeamPublic` (Task 3).
- Produce:
  - `struct FRTCombatLogLine { FString Text; int32 SubjectStableUnitId = INDEX_NONE; }`
  - `void ARTTurnManager::AddLogEvent(const FString& Message, int32 SubjectStableUnitId = INDEX_NONE);`
  - `TArray<FString> ARTTurnManager::GetRecentEventsForTeam(int32 ObserverTeamId) const;`

### Contesto misurato — e correzione a una premessa sbagliata di questo piano

⚠️ **Una stesura precedente diceva che le righe leggibili nascono da un'unica derivazione in
`ConcludeTurn`, e che bastava estrarla.** È **falso a metà**, e la metà mancante cambia il lavoro.
Misurato su `RTTurnManager.cpp`:

- `ConcludeTurn` **deriva** davvero dal TurnLog:
  `for (const FString& Line : URTTurnLogLibrary::DescribeTurnLog(TurnLog)) { AddLogEvent(Line); }`
  — introdotto da CP 11.3 (#79).
- **Ma i produttori sparsi sono rimasti.** Il commento accanto alla derivazione lo dice: *«prima le righe
  nascevano da 59 `AddLogEvent` sparse nella risoluzione, e il TurnLog nasceva altrove: due produttori
  indipendenti coincidono per abitudine, non per costruzione»*.

**Questo però regala la forma giusta**, invece di complicarla: **tutto passa da `AddLogEvent`**, comprese
le righe derivate. È l'imbuto, e il filtro va lì.

#### 🔴 Il censimento dei siti `AddLogEvent`, e i due modi in cui era sbagliato

⚠️ **Una stesura precedente di questo blocco contava con `grep -c "AddLogEvent("` e dichiarava `61` siti,
`35` dei quali nominano un'unità.** Sono numeri da non riusare, per **due** ragioni indipendenti:

1. **Un grep per riga sbaglia sulle chiamate multilinea** e conta anche la definizione della funzione: il
   `61` include `void ARTTurnManager::AddLogEvent(...)`, che non è un chiamante.
2. **Il censimento era scopato a `RTTurnManager.cpp` senza dirlo.** Esiste un secondo file, tracciato e
   compilato, che chiama la stessa funzione: `Source/RefactorTactics/Turn/RTTurnManager_Blast.cpp`
   (83 KB). Nessuno dei suoi siti era stato guardato. ⚠️ Lo stesso difetto è finito nel corpo del commit
   `44c016b1`, che dichiara **26** siti scoperti e li presenta come «il numero da portare al passo
   successivo»: quel corpo è storia e non si riscrive, ma il numero **vale per un file solo**.

Misura con un parser che **bilancia le parentesi** e conta gli argomenti di primo livello, ripetuta da due
persone sull'albero pre-fix e riprodotta dopo la chiusura dei sette echi:

| File | call site | nominano | passano un soggetto | nominano e restano scoperti |
|---|---|---|---|---|
| `RTTurnManager.cpp` | 60 | 40 | 15 → **18** | 26 → **23** |
| `RTTurnManager_Blast.cpp` | 20 | 16 | 0 → **4** | 16 → **12** |
| **totale** | **80** | **56** | 15 → **22** | **42 → 35** |

*(«prima → dopo» = albero di `44c016b1` → albero che chiude i sette echi. La differenza è esattamente
**7**, i sette siti che duplicavano verbatim una voce di TurnLog.)*

⚠️ **`passano un soggetto` non è un sottoinsieme di `nominano`**, e il disaccordo `15` contro `14` della
prima stesura veniva da qui: esiste **un** sito convertito che non nomina alcuna unità —
`RTTurnManager.cpp:2385`, cioè la derivazione da `ConcludeTurn` aggiunta dal fix stesso, che passa
`Line.Value` e stampa il solo `DescribeEntry`. Non erano due definizioni contate in modo diverso.

🔴 **Non copiare `35` da questa tabella**: è una misura, e si rimisura sui **due** file prima di aprire il
lavoro che li chiude — che è una **issue separata**, insieme alla rimozione del default di `AddLogEvent` da
cui dipende. I sette echi chiusi qui erano speciali perché duplicavano *verbatim* ciò che il filtro
sopprimeva: lasciarli era uno stato contraddittorio, non un residuo.

🔴 **Ma non su tutt'e due le cose che fa.** `AddLogEvent` scrive in **due** canali:

```cpp
void ARTTurnManager::AddLogEvent(const FString& Message)
{
	UE_LOG(LogRT, Log, TEXT("[RT] %s"), *Message);   // <- diagnosi per SVILUPPATORE
	RecentEvents.Add(Message);                        // <- cio' che il GIOCATORE vede
	...
}
```

Il `UE_LOG` **resta completo**: è uno strumento di diagnosi, e mutilarlo renderebbe impossibile capire
una partita andata storta. A essere filtrato è **`RecentEvents`**, che è presentazione.

### Dove vive il filtro

`RecentEvents` conserva **testo + soggetto**; a filtrare è l'**accessor**, non lo scrittore. Così il
`TurnManager` non deve sapere chi guarda, e la decisione resta al confine della presentazione — la stessa
disciplina di `FilterForTeam`.

- [ ] **Step 4.1 — Scrivi il test che fallisce**

Aggiungi in cima a `RTKnowledgeViewTests.cpp`, **prima** di `#if WITH_DEV_AUTOMATION_TESTS` (un include
dentro la guardia sparisce in Shipping e non compila):

```cpp
#include "Turn/RTTurnManager.h" // ARTTurnManager::ComposeVisibleLogLines
```

E il test, prima dell'`#endif` finale:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeCombatLogOmitsUnknownTest,
	"RefactorTactics.Knowledge.CombatLogOmitsUnknown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeCombatLogOmitsUnknownTest::RunTest(const FString&)
{
	const FRTCellId SeenCell(3, 0, 0);
	FRTTeamKnowledge K = KvKnowledge({ FRTLastKnownContact(2, SeenCell, 5) });
	K.VisibleCells.Add(SeenCell);

	const TArray<FRTKnowledgeSubject> Subjects = {
		KvSubject(1, 0, FRTCellId(0, 0, 0)),  // alleato
		KvSubject(2, 1, SeenCell),            // nemico visto
		KvSubject(3, 1, FRTCellId(7, 0, 0))   // nemico ignoto
	};
	const FRTKnowledgeView View = URTKnowledgeViewLibrary::ViewForTeam(K, Subjects, /*Observer*/ 0);

	TArray<FRTCombatLogLine> Raw;
	Raw.Add({ TEXT("Turno 5 - pianificazione"),       INDEX_NONE }); // riga di mondo, senza soggetto
	Raw.Add({ TEXT("Alleato: passo -> (0,0,L0)"),     1 });
	Raw.Add({ TEXT("Nemico visto: passo -> (3,0,L0)"), 2 });
	Raw.Add({ TEXT("Ignoto: passo -> (7,0,L0)"),      3 });

	const TArray<FString> Visible = ARTTurnManager::ComposeVisibleLogLines(Raw, View);

	TestEqual(TEXT("tre righe su quattro"), Visible.Num(), 3);
	TestTrue (TEXT("la riga di mondo resta"),  Visible.Contains(TEXT("Turno 5 - pianificazione")));
	TestTrue (TEXT("l'alleato resta"),         Visible.Contains(TEXT("Alleato: passo -> (0,0,L0)")));
	TestTrue (TEXT("il nemico visto resta"),   Visible.Contains(TEXT("Nemico visto: passo -> (3,0,L0)")));

	// 🔴 Il cuore: la riga sparisce INTERA. Non una riga oscurata, non una riga vuota.
	TestFalse(TEXT("la riga dell'ignoto sparisce"), Visible.Contains(TEXT("Ignoto: passo -> (7,0,L0)")));

	// E la sua cella non trapela in NESSUNA delle righe superstiti.
	for (const FString& L : Visible)
	{
		TestFalse(TEXT("nessuna riga nomina la cella dell'ignoto"), L.Contains(TEXT("(7,0,L0)")));
	}

	// Anti-vacuita': l'ORDINE si conserva. Un filtro che riordinasse renderebbe il combat log illeggibile,
	// e nessuna delle asserzioni sopra lo prenderebbe.
	if (Visible.Num() == 3)
	{
		TestEqual(TEXT("l'ordine e' quello di produzione"), Visible[0], TEXT("Turno 5 - pianificazione"));
		TestEqual(TEXT("secondo"), Visible[1], TEXT("Alleato: passo -> (0,0,L0)"));
	}
	return true;
}
```

- [ ] **Step 4.2 — Esegui e verifica che NON COMPILI**

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics.Knowledge.CombatLogOmitsUnknown; Quit" \
  -unattended -nopause -nullrhi -NoSound
```

Atteso: **errore di compilazione** — `FRTCombatLogLine` e `ComposeVisibleLogLines` non esistono.

- [ ] **Step 4.3 — Dichiara il tipo e le due funzioni**

In `Source/RefactorTactics/Turn/RTTurnManager.h`, prima della classe:

```cpp
/**
 * Una riga di combat log, col SOGGETTO accanto al testo.
 *
 * Il soggetto e' `ARTUnit::StableUnitId` — l'identita' che attraversa fasi e turni — oppure `INDEX_NONE`
 * per le righe che parlano del MONDO e non di un'unita' («Turno 3 - pianificazione», una superficie che
 * scade). Senza questo campo il filtro dovrebbe cercare coordinate dentro una stringa gia' formattata.
 */
USTRUCT()
struct FRTCombatLogLine
{
	GENERATED_BODY()

	UPROPERTY()
	FString Text;

	UPROPERTY()
	int32 SubjectStableUnitId = INDEX_NONE;
};
```

Nella classe, in `public:`:

```cpp
	/**
	 * Le righe che un osservatore puo' leggere. Statica e PURA: la si interroga senza montare una partita.
	 *
	 * 🔴 Una riga il cui soggetto e' ignoto **sparisce intera**, non viene oscurata: una riga oscurata
	 * dice comunque che qualcosa e' successo, e quando e' successo.
	 * L'ORDINE di produzione si conserva: un combat log riordinato non e' un log.
	 */
	static TArray<FString> ComposeVisibleLogLines(const TArray<FRTCombatLogLine>& Lines,
		const FRTKnowledgeView& View);

	/** Le righe recenti gia' filtrate per una squadra. E' cio' che l'HUD deve chiamare. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	TArray<FString> GetRecentEventsForTeam(int32 ObserverTeamId) const;
```

E cambia la firma dello scrittore, con il default che tiene compilanti tutti i 61 chiamanti esistenti:

```cpp
	void AddLogEvent(const FString& Message, int32 SubjectStableUnitId = INDEX_NONE);
```

⚠️ `RecentEvents` cambia tipo: da `TArray<FString>` a `TArray<FRTCombatLogLine>`.

- [ ] **Step 4.4 — Implementa**

In `RTTurnManager.cpp`:

```cpp
void ARTTurnManager::AddLogEvent(const FString& Message, int32 SubjectStableUnitId)
{
	// 🔴 Il log di SVILUPPO resta COMPLETO. E' diagnosi, non un canale del giocatore: mutilarlo renderebbe
	// impossibile capire una partita andata storta, e nessun avversario lo legge.
	UE_LOG(LogRT, Log, TEXT("[RT] %s"), *Message);

	RecentEvents.Add(FRTCombatLogLine{ Message, SubjectStableUnitId });
	while (RecentEvents.Num() > MaxLogLines)
	{
		RecentEvents.RemoveAt(0);
	}
}

TArray<FString> ARTTurnManager::ComposeVisibleLogLines(const TArray<FRTCombatLogLine>& Lines,
	const FRTKnowledgeView& View)
{
	TArray<FString> Out;
	Out.Reserve(Lines.Num());
	for (const FRTCombatLogLine& L : Lines)
	{
		// Riga di mondo: nessun soggetto da conoscere, quindi nessuna ragione per nasconderla.
		if (L.SubjectStableUnitId == INDEX_NONE
			|| URTKnowledgeViewLibrary::FindEntry(View, L.SubjectStableUnitId) != nullptr)
		{
			Out.Add(L.Text);
		}
	}
	return Out; // ordine di produzione, mai riordinato
}

TArray<FString> ARTTurnManager::GetRecentEventsForTeam(int32 ObserverTeamId) const
{
	TArray<FRTKnowledgeSubject> Subjects;
	for (const ARTUnit* U : GetLiveUnitsForKnowledge())
	{
		if (!U) { continue; }
		FRTKnowledgeSubject S;
		S.StableUnitId = U->StableUnitId;
		S.TeamId       = U->TeamId;
		S.Cell         = U->Cell;
		S.HeroId       = U->HeroId;
		S.HeroDisplayName = U->HeroDisplayName;
		S.bAlive       = U->IsAlive();
		Subjects.Add(S);
	}
	const FRTKnowledgeView View = URTKnowledgeViewLibrary::ViewForTeam(
		KnowledgeForTeam(ObserverTeamId), Subjects, ObserverTeamId);
	return ComposeVisibleLogLines(RecentEvents, View);
}
```

⚠️ **`GetLiveUnitsForKnowledge()` non esiste**: è il nome che do al modo in cui il `TurnManager` raggiunge
le unità vive. **Prima di scrivere questa funzione, cerca come il `TurnManager` le raggiunge già** — c'è
più di un modo nel file (un `TArray<ARTUnit*>` membro, oppure `GetAllActorsOfClass`). Riusa quello che
c'è, e se non c'è un accessor riusabile, dillo nel report invece di aggiungerne uno terzo.

- [ ] **Step 4.5 — Esegui il test e verifica che PASSI**

Stesso comando dello Step 4.2. Atteso: **PASS**.

- [ ] **Step 4.6 — Passa il soggetto ai siti che possono nominare un nemico**

Con il default a `INDEX_NONE` tutti i 61 chiamanti compilano, ma **quelli non toccati continuano a
mostrare tutto**. I siti che possono nominare un'unità avversaria sono **quattordici**, e vanno toccati
uno per uno passando `Unit->StableUnitId` (o `Bot->StableUnitId`) come secondo argomento:

| Cosa | Riga circa | Perché perde |
|---|---|---|
| danno da terreno | 214 | `%s: %d danni da terreno (q,r,L)` — nomina e localizza |
| status da terreno | 242 | nomina l'unità |
| arma di reazione | 516 | nomina l'unità |
| scatto difensivo | 891 | nomina e localizza |
| arretramento | 897 | nomina e localizza |
| utility del bot — carica | 1062 | cella d'impatto **e** bersaglio |
| utility del bot — scatto+attacco | 1072 | cella **e** bersaglio |
| utility del bot — attacco | 1081 | cella **e** bersaglio |
| utility del bot — scatto | 1089 | cella |
| utility del bot — mossa | 1096 | cella |
| danno da `Status.Burning` | 1359 | nomina e localizza |
| eliminazione dalle fiamme | 1373 | nomina l'unità |
| **movimento** | 1477 | `%s: %s -> (q,r,L)` — è la perdita principale |
| cura | 1627 | nomina l'unità |

⚠️ **Il soggetto è chi la riga RIVELA, non chi la produce.** Le righe di utility del bot ne nominano
**due** — il bot e il suo bersaglio — e quella che va protetta è **il bot**: è la sua posizione e la sua
intenzione che stanno trapelando. Il bersaglio è già filtrato dalla riga del bersaglio stesso.

⚠️ **Le righe di terreno senza unità** (superficie che scade, ponte scaduto, «non prende fuoco») restano
con `INDEX_NONE`: parlano del mondo, e la mappa statica resta nota per canone.

- [ ] **Step 4.7 — L'HUD legge l'accessor filtrato**

In `RTHUD.cpp`, il blocco del combat log sostituisce `GetRecentEvents()` con
`GetRecentEventsForTeam(PlayerTeamId)`, riusando la costante introdotta al Task 3 — **senza aggiungere un
terzo letterale `0`**.

- [ ] **Step 4.8 — Regressione, e in particolare i golden**

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics.TurnLog+RefactorTactics.Match+RefactorTactics.Simulation.Golden+RefactorTactics.Replay.Manifest.GoldenV1+RefactorTactics.HexOccupancy.GoldenExample; Quit" \
  -unattended -nopause -nullrhi -NoSound
```

⚠️ **`RefactorTactics.Golden` non esiste, e la prima stesura di questo comando lo usava.** Misurato il
2026-08-27: **zero** test cominciano con quel prefisso. I golden veri sono **cinque**, sotto tre prefissi
diversi — `Simulation.GoldenCorpus{Matches,DetectsDivergence,RejectsFormatMismatch}`,
`Replay.Manifest.GoldenV1StaysReadable`, `HexOccupancy.GoldenExampleFromTheSource`. Il filtro sbagliato
avrebbe eseguito `TurnLog` e `Match` e **saltato i golden in silenzio**, dichiarando una copertura che non
aveva: esattamente il difetto che il DoD del Task 4 usa i golden per escludere. È il terzo comando di questo
piano che dichiarava più di quanto eseguisse — dopo il `+Quit` e il filtro di regressione del Task 1.

Atteso: **tutti PASS**. 🔴 Se un golden diverge hai toccato il **TurnLog** invece del combat log: annulla
e rileggi. Il TurnLog non cambia in questo task — solo `RecentEvents` e chi lo legge.

- [ ] **Step 4.9 — Commit**

```bash
git add Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/UI/RTHUD.cpp Source/RefactorTactics/Tests/RTKnowledgeViewTests.cpp
git commit -m "fix(hud): il combat log smette di raccontare i movimenti che nessuno ha visto"
```

> ⚠️ **Fuori scope, dichiarato.** Le righe di utility del bot restano visibili per un nemico **noto**, e
> mostrano il suo `score`. Quella è la sua valutazione privata, non la sua posizione: filtrarla per
> conoscenza non basta, andrebbe spostata dietro `rt.Debug.*` come già è la riga della velocità di
> playback. È lavoro proprio, con la sua issue — questo task chiude la perdita **posizionale**, non
> quella del ragionamento.

---

## Task 5: L'unità ignota sparisce

**File:**
- Modifica: `Source/RefactorTactics/Unit/RTUnit.h` — campo, statica pura, funzione di comando
- Modifica: `Source/RefactorTactics/Unit/RTUnit.cpp` — implementazione
- Modifica: `Source/RefactorTactics/UI/RTHUD.cpp` — applica lo stato dopo aver costruito la vista
- Test: `Source/RefactorTactics/Tests/RTKnowledgeViewTests.cpp`

**Interfacce:**
- Consuma: `FRTKnowledgeView`, `URTKnowledgeViewLibrary::FindEntry` (Task 2).
- Produce:
  - `static bool ARTUnit::ShouldBeRendered(bool bAlive, bool bKnownToObserver);`
  - `void ARTUnit::SetKnownToObserver(bool bKnown);`

**Contesto misurato, e la trappola.** `ARTUnit::HideForDefeat` fa **due** cose:
`SetActorHiddenInGame(true)` e `SetActorEnableCollision(false)`. Meccanicamente sono reversibili, ma
**semanticamente** quella funzione significa *morte*: riusarla per «ignoto» produrrebbe il difetto in cui un
morto ridiventa visibile perché la conoscenza lo «rivela».

🔴 **La visibilità è funzione di DUE variabili**: `vivo` **e** `noto`. Va calcolata in un posto solo.

⚠️ `SetActorHiddenInGame` **non** tocca la collisione, e l'unico proxy di click è il componente `Mesh`
(`QueryOnly` + `ECR_Block` su tutti i canali). Il click risolve sull'**Actor**
(`GetHitResultUnderCursor(ECC_Visibility)` poi `Cast<ARTUnit>(Hit.GetActor())`), quindi qualunque componente
collidente lo intercetta: serve `SetActorEnableCollision`, non un `SetCollisionEnabled` sul solo `Mesh`.

- [ ] **Step 5.1 — Scrivi il test che fallisce**

Aggiungi in `RTKnowledgeViewTests.cpp`, e aggiungi `#include "Unit/RTUnit.h"` fra gli include:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeUnitRenderingCombinesAliveAndKnownTest,
	"RefactorTactics.Knowledge.UnitRenderingCombinesAliveAndKnown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeUnitRenderingCombinesAliveAndKnownTest::RunTest(const FString&)
{
	TestTrue (TEXT("vivo e noto: si vede"),        ARTUnit::ShouldBeRendered(true,  true));
	TestFalse(TEXT("vivo ma ignoto: sparisce"),    ARTUnit::ShouldBeRendered(true,  false));

	// 🔴 Le due righe che impediscono il difetto: un MORTO non torna visibile perche' la conoscenza lo
	// «rivela». La morte vince sempre sulla conoscenza, in entrambi i versi.
	TestFalse(TEXT("morto e noto: resta nascosto"), ARTUnit::ShouldBeRendered(false, true));
	TestFalse(TEXT("morto e ignoto: nascosto"),     ARTUnit::ShouldBeRendered(false, false));
	return true;
}
```

- [ ] **Step 5.2 — Esegui e verifica che NON COMPILI**

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics.Knowledge.UnitRenderingCombinesAliveAndKnown; Quit" \
  -unattended -nopause -nullrhi -NoSound
```

Atteso: **errore di compilazione** — `ShouldBeRendered` non esiste.

- [ ] **Step 5.3 — Dichiara nell'header**

In `Source/RefactorTactics/Unit/RTUnit.h`, blocco `public:`:

```cpp
	/**
	 * Se l'unita' va renderizzata, date le DUE variabili che lo decidono.
	 *
	 * 🔴 Pura, statica e in un posto solo perche' il difetto naturale e' calcolarla in due: un morto che
	 * «diventa noto» tornerebbe visibile. La morte vince sempre sulla conoscenza.
	 */
	static bool ShouldBeRendered(bool bAlive, bool bKnownToObserver);

	/**
	 * Dichiara se l'osservatore locale conosce questa unita'. REVERSIBILE, a differenza di `HideForDefeat`,
	 * che significa morte ed e' a senso unico per SEMANTICA.
	 */
	void SetKnownToObserver(bool bKnown);

protected:
	/** Vero finche' l'osservatore locale non dichiara il contrario: un'unita' nasce nota. */
	bool bKnownToObserver = true;

	/**
	 * Applica ai COMPONENTI VISIVI l'esito di `ShouldBeRendered`.
	 *
	 * 🔴 **Non usa `SetActorHiddenInGame`, ed e' il punto di questa funzione.** Quella propaga a TUTTI i
	 * componenti dell'actor, inclusa la sagoma dell'ultimo contatto (Task 6), che vive su questo stesso
	 * actor e deve vedersi **proprio quando l'unita' non si vede**. Nascondere l'actor renderebbe la sagoma
	 * inerte, e nessun test automatico lo prenderebbe: si vedrebbe solo in PIE.
	 */
	void ApplyObserverVisibility();
```

- [ ] **Step 5.4 — Implementa**

In `Source/RefactorTactics/Unit/RTUnit.cpp`, accanto a `HideForDefeat`:

```cpp
bool ARTUnit::ShouldBeRendered(bool bAlive, bool bKnownToObserver)
{
	return bAlive && bKnownToObserver;
}

void ARTUnit::SetKnownToObserver(bool bKnown)
{
	if (bKnownToObserver == bKnown)
	{
		return; // niente churn di stato render a ogni frame
	}
	bKnownToObserver = bKnown;
	ApplyObserverVisibility();
}

void ARTUnit::ApplyObserverVisibility()
{
	const bool bRender = ShouldBeRendered(IsAlive(), bKnownToObserver);

	// 🔴 Si nascondono i COMPONENTI, non l'actor. `SetActorHiddenInGame` propaga a tutti i componenti,
	// sagoma dell'ultimo contatto compresa (Task 6) — che deve vedersi proprio quando l'unita' non si vede.
	if (Mesh)          { Mesh->SetVisibility(bRender, /*bPropagateToChildren*/ false); }
	if (TeamRing)      { TeamRing->SetVisibility(bRender, false); }
	if (SelectionRing) { SelectionRing->SetVisibility(bRender, false); }

	// La freccia di facing ha gia' un interruttore proprio: qui si fa l'AND, non la si sovrascrive.
	if (FacingArrow)   { FacingArrow->SetVisibility(bRender && bShowFacingArrow, false); }

	// La skeletal arriva dal Blueprint `BP_Unit_*`, non dal C++: si cerca fra i componenti.
	if (USkeletalMeshComponent* Skeletal = FindComponentByClass<USkeletalMeshComponent>())
	{
		Skeletal->SetVisibility(bRender, false);
	}

	// La collisione si spegne sull'ACTOR: `SetVisibility` non la tocca, e l'unico proxy di click e' `Mesh`
	// (QueryOnly + ECR_Block su tutti i canali). Un'unita' invisibile ma cliccabile e' peggio di una
	// visibile: il giocatore selezionerebbe qualcosa che non vede. La sagoma e' `NoCollision`, quindi
	// spegnere la collisione dell'actor non la riguarda.
	SetActorEnableCollision(bRender);
}
```

⚠️ `ApplyObserverVisibility` cerca la skeletal con `FindComponentByClass`: aggiungi
`#include "Components/SkeletalMeshComponent.h"` in `RTUnit.cpp` se non c'è già.

- [ ] **Step 5.5 — Esegui il test e verifica che PASSI**

Stesso comando dello Step 5.2. Atteso: **PASS**.

- [ ] **Step 5.6 — Cabla in `DrawHUD`, dopo la costruzione della vista**

Nel ciclo di `DrawHUD`, **prima** del `continue` introdotto al Task 3 (altrimenti l'unità saltata non
riceverebbe mai il comando):

```cpp
		const FRTKnowledgeEntry* Entry = bIsOwnTeam
			? nullptr
			: URTKnowledgeViewLibrary::FindEntry(KnowledgeView, Unit->StableUnitId);
		Unit->SetKnownToObserver(ARTHUD::ShouldDrawUnitOverlay(Entry, bIsOwnTeam));
```

🔴 **Questo Step prescriveva un predicato SBAGLIATO, ed è stato corretto il 2026-08-27.** Diceva alla
lettera:

```cpp
		Unit->SetKnownToObserver(
			Unit->TeamId == PlayerTeamId
			|| URTKnowledgeViewLibrary::FindEntry(KnowledgeView, Unit->StableUnitId) != nullptr);
```

`FindEntry(...) != nullptr` chiede **«esiste una voce»**, che non è **«la posizione è attuale»**. Una voce
`Remembered` esiste per costruzione — `ViewForTeam` la crea apposta dal ramo `CellOnly` — quindi il
predicato prescritto lasciava disegnato, cliccabile e con nome e barra HP alla sua **posizione vera** un
nemico che la squadra non vede più, **mentre** la sagoma del Task 6 lo disegnava una seconda volta alla
cella ricordata. Due copie della stessa unità: l'opposto di ciò per cui la sagoma esiste, e una
**regressione di leggibilità** rispetto alla base, dove le copie erano una.

Il predicato giusto è `bIsOwnTeam || (Entry && Entry->Visibility == Live)`, ed è quello che
`ARTHUD::ShouldDrawUnitOverlay` implementa: così l'overlay e `ContactGhostTargetForUnit` diventano
**complementari** — o si vede l'unità, o si vede il suo ricordo, mai entrambi.

L'errore era già visibile nel piano: la **spec §4 A2** dichiara che A3/A4 sarebbero stati «la cura» del leak
di posizione, e con `!= nullptr` non curano niente — il leak resta intero sul modello vero.

⚠️ Il nome è cambiato: la funzione applicata dallo Step 5.4 si chiama ora `RefreshComponentVisibility`, e
**deriva** la visibilità di tutti i componenti dallo stato invece di assegnarla componente per componente
(vedi il giro di fix del 2026-08-27).

- [ ] **Step 5.7 — Verifica manuale in PIE, e dichiarala**

Atteso: un nemico che esce dalla conoscenza **sparisce**, e cliccando dove stava **non lo si seleziona**.
Rientrando in contatto ricompare. Registra l'esito: è l'unica prova che collisione e visibilità tornano
insieme.

- [ ] **Step 5.8 — Commit**

```bash
git add Source/RefactorTactics/Unit/RTUnit.h Source/RefactorTactics/Unit/RTUnit.cpp Source/RefactorTactics/UI/RTHUD.cpp Source/RefactorTactics/Tests/RTKnowledgeViewTests.cpp
git commit -m "feat(unit): un'unita' ignota alla squadra non si vede e non si clicca"
```

---

## Task 6: La sagoma dell'ultimo contatto

**File:**
- Crea: `Content/RT/Characters/Shared/Materials/M_LastContactGhost.uasset` *(passo Editor, manuale)*
- Modifica: `Source/RefactorTactics/Unit/RTUnit.h` / `.cpp` — il componente della sagoma
- Modifica: `Source/RefactorTactics/Map/RTMapVisuals.h` — la quota del marker a terra
- Test: `Source/RefactorTactics/Tests/RTKnowledgeViewTests.cpp`

**Interfacce:**
- Consuma: `FRTKnowledgeEntry::Visibility == ERTKnowledgeVisibility::Remembered` e `::Cell` (Task 2).
- Produce: nulla per i task successivi. È l'ultimo.

🔴 **PRECONDIZIONE BLOCCANTE.** Questo task crea un `.uasset`. Non iniziarlo finché `git status --porcelain`
non mostra il working tree **pulito dai binari**: due `.uasset` non si fondono, e D-178 prescrive un lavoro
per volta su un binario.

**Contesto misurato, e la sorpresa.** **La mesh dell'eroe non viene dal C++.** `ConfigureFromHeroData` non
tocca alcuna mesh: copia statistiche, identità e abilità. Gli unici tre `SetStaticMesh` del file sono nel
costruttore e mettono il **cilindro engine** segnaposto. L'aspetto dell'eroe arriva da
`TMap<FName, TSubclassOf<ARTUnit>> HeroUnitClasses` sul GameMode — cioè dai Blueprint `BP_Unit_*`, che
portano uno `USkeletalMeshComponent`, e le cui mesh vivono in **`Content/FabAsset`, non versionata**.

Quindi la sagoma **non può** essere un asset nuovo: va derivata **a runtime** dal componente skeletal che il
Blueprint porta — che il C++ già sa localizzare fra i componenti.

⚠️ **La sagoma non deve seguire l'unità.** Sta alla cella del **ricordo** mentre l'unità si muove altrove.
Un componente figlio ereditando il transform la trascinerebbe: serve
`SetUsingAbsoluteLocation(true)` (e rotazione) sul componente della sagoma, così il suo transform è nel
mondo e non relativo al padre.

- [ ] **Step 6.1 — Misura prima di modificare**

Trova nel C++ il punto in cui la skeletal del Blueprint viene localizzata fra i componenti
(`GetComponents`/`FindComponentByClass` su `USkeletalMeshComponent` in `RTUnit.cpp`) e riportane la firma
esatta nel corpo della PR. È da lì che la sagoma prende la mesh, ed è l'unica strada che non richiede un
asset versionato.

- [ ] **Step 6.2 — Crea il materiale in Editor**

In Unreal, duplica il materiale base della mesh eroe e crea
`Content/RT/Characters/Shared/Materials/M_LastContactGhost`:
- **Blend Mode**: `Translucent`
- **Shading Model**: `Unlit`
- **Emissive**: colore **monocromo** (grigio), **nessun** colore di squadra
- **Opacity**: costante scalare, esposta come parametro `GhostOpacity` per la dissolvenza

⚠️ È un passo **manuale**: registralo come verifica manuale, non come «fatto» automatico.

- [ ] **Step 6.3 — Scrivi il test che fallisce**

La dissolvenza è presentazione e non ha effetti logici; ciò che si testa è la **funzione di opacità**:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeGhostFadesWithContactAgeTest,
	"RefactorTactics.Knowledge.GhostFadesWithContactAge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeGhostFadesWithContactAgeTest::RunTest(const FString&)
{
	// `ContactLifetimeTurns` vale 1: il ricordo dura il turno successivo a quello dell'avvistamento.
	// Turno del contatto == turno corrente -> sagoma piena; un turno dopo -> gia' in dissolvenza; oltre -> nulla.
	TestEqual(TEXT("appena visto: opaca"),
		ARTUnit::GhostOpacityForContact(/*ContactTurn*/ 5, /*CurrentTurn*/ 5), 1.0f);
	TestTrue(TEXT("un turno dopo: dissolve ma c'e' ancora"),
		ARTUnit::GhostOpacityForContact(5, 6) > 0.0f && ARTUnit::GhostOpacityForContact(5, 6) < 1.0f);
	TestEqual(TEXT("oltre la scadenza: sparita"),
		ARTUnit::GhostOpacityForContact(5, 7), 0.0f);

	// Anti-vacuita': un ricordo dal FUTURO (turno maggiore del corrente) non e' un ricordo. Fail-closed.
	TestEqual(TEXT("un contatto dal futuro non disegna nulla"),
		ARTUnit::GhostOpacityForContact(9, 5), 0.0f);
	return true;
}
```

- [ ] **Step 6.4 — Esegui e verifica che NON COMPILI**

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics.Knowledge.GhostFadesWithContactAge; Quit" \
  -unattended -nopause -nullrhi -NoSound
```

Atteso: **errore di compilazione** — `GhostOpacityForContact` non esiste.

- [ ] **Step 6.5 — Implementa la funzione di opacità**

⚠️ `RTUnit.cpp` deve includere `#include "Perception/RTTeamKnowledge.h"` per `ContactLifetimeTurns`:
la costante è di `URTTeamKnowledgeLibrary`, e ricopiarne il valore qui creerebbe la seconda verità sulla
durata del ricordo.

In `RTUnit.h` (`public:`) e `RTUnit.cpp`:

```cpp
	/**
	 * Opacita' della sagoma del ricordo. Pura: la dissolvenza e' PRESENTAZIONE e non ha effetti logici, ma
	 * la sua REGOLA e' testabile e va tenuta fuori dal Tick.
	 *
	 * `URTTeamKnowledgeLibrary::ContactLifetimeTurns` vale 1: il ricordo vive il turno successivo, poi basta.
	 * Un contatto con turno maggiore di quello corrente e' incoerente -> zero (fail-closed).
	 */
	static float GhostOpacityForContact(int32 ContactTurn, int32 CurrentTurn);
```

```cpp
float ARTUnit::GhostOpacityForContact(int32 ContactTurn, int32 CurrentTurn)
{
	const int32 Age = CurrentTurn - ContactTurn;
	if (Age < 0 || Age > URTTeamKnowledgeLibrary::ContactLifetimeTurns)
	{
		return 0.0f;
	}
	return (Age == 0) ? 1.0f : 0.45f;
}
```

- [ ] **Step 6.6 — Esegui il test e verifica che PASSI**

Stesso comando dello Step 6.4. Atteso: **PASS**.

- [ ] **Step 6.7 — Aggiungi il componente della sagoma**

Un `USkeletalMeshComponent` figlio di `SceneRoot`, `NoCollision`, `CastShadow = false`, con
`SetUsingAbsoluteLocation(true)` e `SetUsingAbsoluteRotation(true)` — così **non segue** l'unità. La mesh e
la posa si prendono dal componente skeletal del Blueprint (Step 6.1); il materiale è
`M_LastContactGhost`, con `GhostOpacity` guidato da `GhostOpacityForContact`.

⚠️ **La quota del marker a terra si dichiara in `Map/RTMapVisuals.h`**, non si ricopia: le quote 0,3 / 0,5 /
1,5 / 2,5 sopra la faccia sono già assegnate, e tutto ciò che sta sotto `RTCellTopZ` (2,5 uu) finisce dentro
un cilindro opaco — è successo due volte.

- [ ] **Step 6.8 — Verifica manuale in PIE, e dichiarala**

Atteso: perdendo contatto con un nemico, nella cella dell'ultimo avvistamento resta la sua sagoma
**monocroma, senza freccia di facing**; l'unità vera non è visibile; la sagoma **non si muove**; dopo un
turno svanisce. Registra l'esito.

- [ ] **Step 6.9 — Commit**

```bash
git add Content/RT/Characters/Shared/Materials/M_LastContactGhost.uasset Source/RefactorTactics/Unit/RTUnit.h Source/RefactorTactics/Unit/RTUnit.cpp Source/RefactorTactics/Map/RTMapVisuals.h Source/RefactorTactics/Tests/RTKnowledgeViewTests.cpp
git commit -m "feat(hud): il ricordo di un nemico resta dove l'avevi visto"
```

---

## Chiusura della fase

- [ ] **Suite completa**

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics; Quit" \
  -unattended -nopause -nullrhi -NoSound
```

Leggi `Found N tests` e confronta gli **eseguiti** con i **dichiarati**: sono due numeri, non uno.

- [ ] **Issue.** I Task 1–6 non hanno una issue: apri **una issue nuova** che li copra (è metà porta e metà
  *difetto* — i due leak non erano nel DoD di nessuno), e **non** allargare #160 in silenzio.
- [ ] **Decisione nuova.** Registra nel Decision Log la grammatica visiva — sagoma volumetrica sia per *Last
  Contact* sia per *Action Ghost*, distinte su due canali — che emenda `progettazione-hud.md` §9 e §25.
  ✅ **Riverificato il 2026-08-27**: `D-196` era gia' preso su `origin/main` (corpus golden), che era arrivata
  a **`D-212`** con la PR aperta **#1481** su `D-213`. La voce di questo branch e' quindi **`D-221`**, e la
  fog of war in v0.1 e' **`D-222`**.
- [ ] **PR** con base il branch padre (`git config branch.<current>.parent` o `git merge-base`), **non**
  automaticamente `main`.
- [ ] **Verifiche PIE** registrate in `docs/technical/test-manuali-pie.md`: sono tre (Step 3.8, 5.7, 6.8) e
  nessuna ha un equivalente automatico.
