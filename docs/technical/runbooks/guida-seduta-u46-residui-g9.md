# Guida di seduta — U46, i residui del gate `G9`

> `CURRENT` · **Creata**: 2026-09-10 · **Seduta**: `U46` in
> [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml) · **Issue**: [#2476](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2476) ·
> **Owner degli esiti**: [`test-manuali-pie.md`](../test-manuali-pie.md) — questa guida dice **come**
> osservare, non **cosa è risultato**.
> Preparata su `origin/main = 5d44e38e`.

> 📐 **Convenzione dei numeri.** Ogni conteggio in questo documento porta accanto il comando che lo
> produce e la data in cui è stato eseguito. Chi rilegge **ricalcola**: senza un gate che li rimisuri,
> i numeri qui sotto invecchiano da soli.

---

## 1. Questa seduta è molto più corta di quanto la sua issue dichiari

`U46` è nata il 2026-09-04 per raccogliere **quattro** residui sparsi in quattro sedute quasi esaurite —
convocarle dalle origini sarebbe costato quattro aperture di Editor. Da allora la maggior parte si è chiusa,
e il corpo di #2476 non ha seguito.

Stato al 2026-09-10, letto voce per voce con la **prima icona di stato per posizione** — cioè la prima fra
`✅ 🟡 ❌ ⏳`, ignorando `⚠️` e `🔴` che aprono molte celle come marcatori di prosa e non sono esiti:

| Voce | Subset | Stato | Che cosa vuole |
|---|---|---|---|
| `PIE-V01-LOG` | `RELEASE-V01` | ✅ | — chiusa |
| `PIE-V01-ROSTER` | `RELEASE-V01` | ✅ | — chiusa |
| `PIE-VIS-TWOLAYERS` | — | ✅ | — chiusa |
| **`PIE-HEXPLAY-6`** | `RELEASE-V01` | ❌ | **un occhio** |
| **`PIE-VIS-SIGHTWALL`** | `RELEASE-V01` | ❌ | **un occhio** |
| `PIE-HEXPLAY-8` | `RELEASE-V01` | 🟡 | **una decisione** — [#2911](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2911) |
| `PIE-V01-DEBUG` *(opportunistica)* | — | 🟡 | un occhio, su un altro allestimento |

🔑 **Le due voci `❌` si giudicano sullo STESSO banco, nello STESSO Play.** `PIE-HEXPLAY-6` e
`PIE-VIS-SIGHTWALL` guardano entrambe `Visual.Map.SightWallIsWalkable`: la prima chiede che il blocco-vista
sia comprensibile, la seconda registra il verdetto della vista di gioco sulla coppia lastra/colonna.

∴ **questa seduta è una apertura, un banco, due verdetti** — più `PIE-V01-DEBUG`, che vuole un secondo
allestimento e non entra nel `done_when`.

⛔ **Il passo ③ non produce niente e va saltato.** `PIE-HEXPLAY-8` non aspetta una seduta: il suo residuo —
che il crollo del ponte si veda — **non è osservabile in v0.1** perché `Action.ModifyArc` è senza owner nel
roster ([D-046](../../decisions/RT_PDR_00_Decision_Log.md)). Chi apre l'Editor per quella voce non troverà
nulla da guardare. La domanda è passata a #2911.

---

## 2. 🔴 La premessa è cambiata il 2026-09-10, e per questo non è una ripetizione

Le due voci `❌` sono state **già giudicate due volte il 2026-09-09** — `runId 20260909-085103` e
`20260909-123728` — ed entrambe le volte sono uscite rosse.

**Rigiudicarle non è rispendere una seduta sulla stessa cosa**, e la deroga va dichiarata invece che
sottintesa, come [`roadmap-pia.md`](../../roadmap/roadmap-pia.md) impone:

* il **2026-09-09** il feed testuale non arrivava a schermo. Il canale esisteva, l'asset esisteva, e
  `WBP_RT_EventLog` **non era montato in nessun albero**;
* il **2026-09-10** è stato montato — `WBP_RT_EventLogRight` nella `ZoneRight`
  ([#2697](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697), PR #2834), presidiato da
  `ScreenHud.TheHudMountsTheFeedThatExplainsTheTurn`.

∴ la domanda che il passo ② pone — *«chi guarda collega la riga al segno senza sapere in anticipo cosa
cercare?»* — **non era ponibile ieri**, perché la riga a schermo non c'era. Oggi lo è per la prima volta.

⚠️ **Un test prova che il widget è nell'albero, non che una frase sia leggibile.** È esattamente la metà che
questa seduta deve rispondere.

---

## 3. Allestimento — le due vie, e quale conviene

### 3.1 ⛔ La via da NON usare: `rt.Test.Run`

`rt.Test.Run` esegue lo scenario **nella partita in corso** e lascia i quattro personaggi del roster in
scena: due allestimenti sovrapposti, e ogni verdetto preso così è su una scena contaminata. Misurato il
2026-09-03 e scritto in [`guida-seduta-u42-corpus-visual.md`](guida-seduta-u42-corpus-visual.md) §1–2.

### 3.2 La via pulita

Lo scenario si esegue **al posto** della partita normale: il ramo `ERTScenarioStart::Started` accende il
Tick del GameMode e prosegue nell'harness invece di allestire la board
(`Source/RefactorTactics/RTGameMode.cpp:514-545`). Nessun roster, nessuna sovrapposizione.

Tre sorgenti, e **il più specifico vince** (`ARTGameMode::ResolveScenarioToRun`,
`RTGameMode.cpp:929`):

| | Sorgente | Come |
|---|---|---|
| 1 | riga di comando | `-dpcvars=rt.Test.Scenario=<Id>` |
| 2 | console | `rt.Test.Scenario <Id>` — CVar, `ScenarioHarness/RTTestConsole.cpp:25` |
| 3 | proprietà del GameMode | Details → categoria `RefactorTactics\|Test` → **`Scenario To Run`** |

✅ **Per una seduta a mano conviene la (3), e la ragione è misurata**: il menu a tendina fa **scegliere**
l'ID invece di scriverlo. Il 2026-09-03 un `Enviorment` per `Environment` non ha prodotto nessun errore
visibile — la partita è semplicemente partita normale, e chi guardava credeva di guardare lo scenario.

💡 Con PIE **fermo** lo scenario si cambia da console senza riaprire l'Editor: Stop → `rt.Test.Scenario …`
→ Play.

### 3.3 ⏱️ Il ritmo — è la leva che decide se il giudizio è possibile

```
ARTGameMode::ScenarioTurnPauseSeconds   // RTGameMode.h:309, default 1.5f
UPROPERTY(EditAnywhere, Category = "RefactorTactics|Test")
```

🔴 **Fra le due sedute del 2026-09-09 la build NON è cambiata: è cambiato questo numero, da `1,5` a `15`.**
La prima apertura misurava un secondo e mezzo di scena credendo di misurare il gioco. Per una voce che
chiede *«si legge?»*, un secondo e mezzo non è una misura.

**Portalo a `15` nei Details prima di premere Play.**

⚠️ **Cambialo nel pannello, MAI nel codice.** `RefactorTactics.Scenario.AutoRunPacingHasShortDefault`
(`Tests/RTScenarioAutoRunTests.cpp:145`) asserisce che il **default** stia sotto `10 s`: alzarlo nel sorgente
fa rosso quel test. La proprietà impostata a mano su un'istanza non lo tocca.

---

## 4. La seduta

### Passo ① — `PIE-HEXPLAY-6` **e** `PIE-VIS-SIGHTWALL`, un solo Play

**Banco**: `Visual.Map.SightWallIsWalkable` · **Ritmo**: `ScenarioTurnPauseSeconds = 15`

⚠️ **Perché questo banco e non `L_HexArena`**: le celle di barriera della mappa d'autore portano
**entrambi** i flag — `bBlocksMovement` e `bBlocksLineOfSight` — quindi lastra e colonna si sovrappongono e
la lastra non si giudica da sola. `SightWallIsWalkable` è l'unico caso in cui la cella **nega il tiro e si
attraversa**.

Le due forme che `RebuildInstances` popola nell'ISM persistente `Blockers`, indipendenti e senza guardia di
compilazione:

| forma | misura | flag di origine |
|---|---|---|
| **lastra** | `0,75 × 10 cm` | `bBlocksLineOfSight` |
| **colonna** | `0,40 × 55 cm` | `bBlocksMovement` |

**Che cosa guardare, in quest'ordine:**

1. 🔑 **Guarda se la riga arriva a schermo PRIMA di aprire l'Output Log.** È la novità del 2026-09-10, ed è
   il punto di tutta la seduta: se apri il log prima, hai già saputo cosa cercare e la domanda è bruciata.
2. Un tiro viene rifiutato per linea di vista. La riga nel feed nomina la cella che l'ha fermato, e a schermo
   quella cella porta un segno.
3. **La domanda**: *chi guarda, senza sapere in anticipo cosa cercare, collega la riga al segno?*
4. Per `PIE-VIS-SIGHTWALL`, nello stesso Play: la coppia **lastra/colonna** si legge nella vista di gioco?
   Si distingue «non ci passo» da «non ci vedo attraverso»?

⛔ **Non aprire una issue se la lastra da 10 cm si legge male.** [D-340](../../decisions/RT_PDR_00_Decision_Log.md)
ha scelto la via *(b)* — far nominare la causa al log — e ha rinunciato **per iscritto** ad alzare
`RTSightSlabHeight`. Un «no» sulla lastra è il risultato **atteso**, non un difetto da aprire.

⚠️ **Il velo entra nella misura.** `VeilInstances` porta a scala **zero** i volumi delle celle mai viste
([D-225](../../decisions/RT_PDR_00_Decision_Log.md)): una cella non osservata non mostra nulla **per
progetto**. Non è un difetto di resa.

⛔ **Si giudica in PIE, alla camera del giocatore.** Chi cattura dal viewport dell'Editor vede gli anelli di
`DrawCellOverlay` — che però nascono **spenti** (`bCellOverlay = false`) — e crede di aver giudicato la vista
del giocatore.

### Passo ② — *(opzionale)* `PIE-V01-DEBUG`

**Banco**: `Visual.Environment.Acceptance` · cinque turni, i cinque fenomeni di superficie.

Non è nel subset `RELEASE-V01` e **non entra nel `done_when`**. Ma chi ha già l'Editor aperto la chiude quasi
gratis: si legge in `Window → Output Log`, filtro `LogRT`.

⚠️ **La partita normale su `L_HexArena` non serve a questa voce**: misurato il 2026-09-04, dodici turni senza
un solo fallback né una modifica ambientale. È un fatto sulla mappa, non sul codice.

---

## 5. Che cosa NON guardare, perché ha già un oracolo

Queste tre metà sono coperte headless e rigiudicarle a schermo è tempo speso due volte:

| Domanda | Oracolo |
|---|---|
| la riga nomina la cella che ha fermato il tiro | `CombatLog.SightBlockerAppearsInTheLine` — `Tests/RTCombatLogTests.cpp:1848` |
| quella cella **e nessun'altra** è marcata | `HUD.BlockerMarksOnlyNameableCells` — `Tests/RTHUDMarksTests.cpp:256` |
| il feed è montato nell'albero della HUD | `ScreenHud.TheHudMountsTheFeedThatExplainsTheTurn` — `Tests/RTMatchWidgetAssetTests.cpp:822` |

Resta **una domanda sola** che vuole un occhio, ed è quella del passo ① punto 3.

---

## 6. ⛔ Perché nessuna di queste voci si delega al ponte MCP

Misurato il 2026-09-04: `CaptureViewport` renderizza il mondo dell'**Editor**, non quello PIE. La prova è
diretta — `find_actors` trova `BP_Unit_Gadget_C_0` a `(-519,-30,-200)`, e una cattura centrata esattamente
su quel punto mostra **la cella e non l'unità**.

Via ponte si giudicano mappa, celle, quote e archi. **Non** unità, HUD, VFX né playback — cioè precisamente
ciò che questa seduta chiede.

---

## 7. Come si chiude

* l'esito di ciascuna voce si scrive in [`test-manuali-pie.md`](../test-manuali-pie.md), che ne resta
  **l'unico owner** — non in questa guida, non in `editor-sessions.yaml` (regola `R-6`), non in due posti;
* ⛔ **dichiara commit e binario accanto al verdetto**, non solo lo scenario. La misura del 2026-09-04 diceva
  *«il muro si vede»* perché guardava la colonna da 55 cm credendo di guardare la lastra da 10: ciò che
  mancava accanto a quel verdetto era proprio l'oggetto misurato;
* dichiara anche `ScenarioTurnPauseSeconds`, per la stessa ragione — è parte delle condizioni in cui il
  giudizio è stato dato;
* **un verdetto negativo è un risultato**, non un fallimento della seduta. Se una voce risulta rossa per un
  difetto vero, quel difetto ha una issue propria;
* dopo il merge, la riga `G9` del DoD si **rimisura col comando che prescrive**, non si copia da qui.
