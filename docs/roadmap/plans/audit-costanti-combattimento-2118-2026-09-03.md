# Quali costanti di combattimento si possono cambiare senza che niente diventi rosso

> **Issue**: [#2118](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2118) ·
> **Origine**: [#2105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2105) ·
> **Strumento**: `tools/mutation/costanti-combattimento.py` ·
> **Misurato**: 2026-09-03, su `88da4a1c` (`origin/main`), suite `1852/1852`

## La risposta

**Nessuna.** Ognuna delle 11 `static constexpr int32` di `RTCombatLibrary.h` ha almeno un test che
cade quando il valore cambia, **in entrambe le direzioni**. Il buco che `#2105` ha chiuso era
l'eccezione, non la regola.

## La tabella, a due direzioni

Ogni cella è il numero di test che **cadono** mutando quella costante, con la baseline sottratta.
Ogni riga `−3` è una misura **VALIDA** su un build `Result: Succeeded`.

| costante | valore | `+3` | `−3` |
|---|---:|---:|---:|
| `BaseShield` | 5 | 19 | **18** |
| `DeflectDamageReduction` | 20 | 9 | **5** |
| `GuardFirstHitReduction` | 15 | 7 | **7** |
| `BurningCleanupDamage` | 8 | 5 | **6** |
| `ExposedFirstHitBonus` | 5 | 3 | **3** |
| `GuardResistedPushDistance` | 1 | 3 | **3** |
| `LowCoverDamageReduction` | 10 | 3 | **3** |
| `PropagatedElectricDamage` | 12 | 2 | **2** |
| `GadgetWetDischargeBonus` | 8 | 2 | **2** |
| `MarkedFirstHitBonus` | 6 | 1 | **1** |
| `BraceDamageReduction` | 10 | 1 | **1** |

### ⚠️ Il confronto è pareggiato su una riga sola

La colonna `+3` viene dall'audit del **2026-09-02**, su un albero che oggi è **10 commit** indietro.
Per la riga che porta il verdetto — `DeflectDamageReduction`, la costante che ha motivato l'intera
issue — la `+3` è stata **rimisurata sullo stesso `88da4a1c`** della `−3`, e ha dato **9**: lo stesso
numero di allora. ∴ La colonna `+3` non è invecchiata, ma per le altre dieci righe resta un confronto
**indicativo**, non pareggiato.

### La direzione conta davvero, e solo dove la teoria diceva

`DeflectDamageReduction` è l'unica riga con uno scarto grande fra le direzioni: **9** contro **5**.
È il meccanismo che `D-224` rende possibile — alzare una riduzione spinge il danno residuo sotto i 5
dello scudo base, dove ogni valore dà lo stesso esito. ⛔ **Ma lo scarto non arriva mai a zero**: nella
direzione che nasconde, `Deflect` conserva 5 test. Il caso di `#2105` — 2 test — era un difetto di
quello scenario, non una proprietà della costante.

### `GuardResistedPushDistance` misura un'altra domanda

`1 → −2` non è «tre di meno»: l'uso è `KnockDist[T] <= GuardResistedPushDistance`
(`RTTurnManager_Blast.cpp:1789`) con `KnockDist ≥ 0`, quindi **ogni valore ≤ 0 dà lo stesso esito**.
Quella riga risponde a *«e se la guardia non reggesse mai una spinta?»*. Satura sul **dominio**, non
sullo scudo — ed è comunque coperta, da 3 test.

## 🔴 Un solo test regge nove costanti su undici

`RefactorTactics.Scenario.EveryShippedScenarioRuns` compare in **9** delle 11 righe `−3` (erano 8 su 11
in `+3`). È il singolo test che regge più costanti dell'intero perimetro.

⚠️ E per `BraceDamageReduction` è **l'unico**: se quel test venisse indebolito, non si perderebbe una
copertura, se ne perderebbero nove — e una costante resterebbe del tutto scoperta.

## I due punti singoli

| costante | l'unico test che se ne accorge | livello |
|---|---|---|
| `MarkedFirstHitBonus` | `Actions.MarkTarget.ConsumedOnce` | C++, nessuno scenario |
| `BraceDamageReduction` | `Scenario.EveryShippedScenarioRuns` | scenario, nessun test C++ |

Sono gli **stessi due** della `+3`, confermati nella direzione opposta. Entrambi pinnano davvero — sono
caduti sotto mutazione, quindi non sono tautologici. ⛔ Non aprono una issue: un test che cade è
copertura, e «ce n'è uno solo» sarebbe rumore. È scritto qui perché chi tocca quei due test sappia cosa
regge.

## 🔴 Lo strumento era rotto, e falliva dalla parte giusta

La `−3` non era «in misura» dal 2026-09-02: **non era misurabile**. La guardia che verifica *«la
mutazione è atterrata?»* rileggeva l'header in modalità **testo** e lo confrontava con una stringa
derivata dai **byte**. `RTCombatLibrary.h` è CRLF, la rilettura applica le *universal newlines*, e i due
valori non erano uguali **mai**:

| | caratteri |
|---|---:|
| `BYTE_ORIGINALI.decode("utf-8")` | 17603 |
| `io.open(H, encoding="utf-8").read()` | 17276 |
| differenza | **327** = le righe del file |

Nato da `af1083b2`, che portò la **scrittura** in byte — giustamente, perché il ripristino sporcava i
fine riga — e lasciò la **verifica** in testo. Corretto in `8aab99bc`: entrambe in byte, il che è anche
più forte, perché confronta il disco con esattamente ciò che si voleva scrivere.

Provata sulle undici costanti prima di spendere un'altra ora di motore: verifica vecchia **0/11**,
nuova **11/11**, e il ripristino torna byte-identico.

⚠️ **Il difetto non produceva numeri falsi**: dichiarava `NON MISURATA`. Ma la suite girava lo stesso,
verde, su un binario **non** mutato — un'ora di motore per zero righe di conoscenza, su una macchina
dove il motore è conteso da più sessioni.

## Cosa questa misura NON dice

- ⛔ Nulla sulle `static constexpr int32` **fuori** da `RTCombatLibrary.h`: stessa domanda, altro perimetro.
- ⛔ Nulla sulla **qualità** della copertura: 18 test che cadono su `BaseShield` dicono che è intrecciata
  ovunque, non che sia ben misurata.
- ⛔ Nulla sui valori che non sono `constexpr int32` — cataloghi, `Parameters`, dati degli eroi.
- ⛔ Nulla su scarti **diversi** da 3. `+3` e `−3` non esauriscono lo spazio: una costante potrebbe essere
  muta a `±1` e coperta a `±3`.
