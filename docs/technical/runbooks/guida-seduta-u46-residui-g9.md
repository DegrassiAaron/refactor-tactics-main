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
| **`PIE-HEXPLAY-6`** | **`RELEASE-V01`** | ✅ | **niente: la seduta è stata fatta** — il 2026-09-12, convocata da [#2697](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697), `runId 20260912-141110`, e per la prima volta il verdetto d'autore è `PASS`. ⌫ *Diceva «un occhio — ed è l'unica ❌ che blocca `G9`»: non blocca più niente, e `G9` è ✅* |
| **`PIE-VIS-SIGHTWALL`** | — *(fuori subset)* | ❌ **e resta l'unica ❌ di questa tabella** | **un occhio** — non blocca `G9` (che è ✅ dal 2026-09-12), ma è la DoD di [#2534](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2534) |
| `PIE-HEXPLAY-8` | `RELEASE-V01` | ✅ | **niente: la decisione è stata presa** — [`D-402`](../../decisions/RT_PDR_00_Decision_Log.md) il 2026-09-12 ha ridotto il criterio alla metà osservabile, e su quello la voce è verde. ⛔ Non convocare la seduta per questa voce |
| `PIE-V01-DEBUG` *(opportunistica)* | — | 🟡 | un occhio, su un altro allestimento |

⚠️ **`PIE-VIS-SIGHTWALL` NON è nel subset di release, e la distinzione è quella che la stessa
`grep -c RELEASE-V01` sbaglia**: quella stringa compare anche nella **prosa** di celle che non portano il
marcatore. Il conteggio si fa sul marcatore in testa alla riga, ed è la ragione per cui il registro
prescrive il comando che prescrive:

```bash
grep -c '^| \*\*PIE-[A-Za-z0-9.-]*\*\* `RELEASE-V01`' docs/technical/test-manuali-pie.md   # 17

grep '^| \*\*PIE-[A-Za-z0-9.-]*\*\* `RELEASE-V01`' docs/technical/test-manuali-pie.md \
| awk -F'|' '{s=$(NF-1); if (match(s, /✅|🟡|❌|⏳/)) c[substr(s, RSTART, RLENGTH)]++}
  END {printf "verde=%d parziale=%d fallita=%d aperta=%d\n", c["✅"], c["🟡"], c["❌"], c["⏳"]}'
# verde=15 parziale=1 fallita=1 aperta=0     ← 2026-09-10, main = 5d44e38e
```

⌫ **Questa riga diceva *«resta una sola ❌, `PIE-HEXPLAY-6`, più `PIE-HEXPLAY-8` 🟡 che aspetta una decisione»*, e non vale più per entrambe le voci.** `PIE-HEXPLAY-6` è ✅ dal 2026-09-12 (seduta convocata da #2697, `runId 20260912-141110`, verdetto d'autore `PASS`); `PIE-HEXPLAY-8` è ✅ dalla stessa data per riduzione di criterio (`D-402`, #2911). ∴ del subset **non resta nessuna voce non verde**: `17 ✅ · 0 · 0`, e `G9` è ✅. Trovato in code review.
`PIE-VIS-SIGHTWALL` si giudica **nello stesso Play e gratis**, e vale per #2534.

🔑 **Le due voci si giudicavano sullo STESSO banco, nello STESSO Play — e una è già stata giudicata.**
`PIE-HEXPLAY-6` è ✅ dal 2026-09-12 (seduta convocata da [#2697](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697), `runId 20260912-141110`,
verdetto d'autore `PASS`). ⛔ **Resta `PIE-VIS-SIGHTWALL`**, ancora ❌ e **fuori** dal subset
`RELEASE-V01`: non tocca `G9`, che è ✅, ma è una voce della DoD di [#2534](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2534). Il banco non
cambia — `Visual.Map.SightWallIsWalkable`, la stessa scena — e la voce registra il verdetto della vista
di gioco sulla coppia lastra/colonna.

∴ **questa seduta è una apertura, un banco, un verdetto** — più `PIE-V01-DEBUG`, che vuole un secondo
allestimento e non entra nel `done_when`. ⌫ *Diceva «due verdetti», e il primo dei due è stato emesso il
2026-09-12.*

⛔ **Il passo ③ non produce niente e va saltato.** `PIE-HEXPLAY-8` non aspetta una seduta: il suo residuo —
che il crollo del ponte si veda — **non è osservabile in v0.1** perché `Action.ModifyArc` è senza owner nel
roster ([D-046](../../decisions/RT_PDR_00_Decision_Log.md)). Chi apre l'Editor per quella voce non troverà
nulla da guardare. La domanda è passata a #2911, **che l'ha chiusa il 2026-09-12** con `D-402`: il criterio si riduce alla metà osservabile e `D-046` non si riapre.

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

### 3.2 La via pulita per lanciare uno scenario — che serve al **passo ②**, non al ①

⚠️ **Leggi prima §3.3.** Questa sezione dice come si lancia bene uno scenario; **non** dice che il passo ①
si giudichi su uno scenario, perché non è così. Serve al passo ② (`PIE-V01-DEBUG`), e a chiunque debba
lanciare un banco per altre ragioni.

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

### 3.3 🔴 `ScenarioTurnPauseSeconds` NON è la via, ed è l'errore che questa guida conteneva

```
ARTGameMode::ScenarioTurnPauseSeconds   // RTGameMode.h:309, default 1.5f
UPROPERTY(EditAnywhere, Category = "RefactorTactics|Test")
```

⌫ **La prima stesura di questa guida prescriveva di portarlo a `15` e giudicare sul banco in auto-run.
È sbagliato, e sarebbe la QUINTA ripetizione dello stesso errore** — il registro ne conta già quattro su
questa voce: *«un verdetto che descrive il banco invece del prodotto»*, e l'auto-run scambiato per la
pianificazione è la quarta.

**Il fatto strutturale**, misurato e scritto in [`test-manuali-pie.md`](../test-manuali-pie.md):

> *il banco in auto-run non può giudicare la leggibilità della pianificazione, perché **non ha una fase di
> pianificazione**. Ciò che vi si osserva è la scena **fra** due risoluzioni, con tutti i piani azzerati.*

Il meccanismo, verificabile nel codice:

* durante la pausa `FRTScenarioSession::Step` esegue **solo** `PauseElapsed += DeltaSeconds`
  (`ScenarioHarness/RTScenarioSession.cpp:1567`) — nessun input, nessun piano;
* `BeginTurn()` azzera i piani, applica gli intenti, chiama i bot e committa **nello stesso frame**;
* la guardia della linea d'intento ha **tre** condizioni — `GetPhase() == Planning` **e** `!IsResolving()`
  **e** `View.bHasTarget` — e la terza è **falsa** per tutta la pausa, perché il resolver ha appena azzerato
  il piano.

∴ alzare la pausa dà **più tempo per guardare una scena in cui nessuno sta pianificando**. Il 2026-09-09 ha
prodotto esattamente questo: un verdetto ❌ la cui causa è stata **ritirata lo stesso giorno**.

⛔ **E l'harness non ha una modalità che ceda il controllo**: `git grep` per
`bHumanPlanning|WaitForPlayer|Interactive|PauseForPlanning` in `ScenarioHarness/` dà **0** occorrenze
(misurato 2026-09-10 su `main = d62c5dca`). Non è una via da configurare meglio: non c'è.

✅ **La via corretta è una partita con pianificazione umana**, ed è ciò che prescrivono sia il registro —
*«in una partita con pianificazione umana, o su un banco che lasci un intento vivo nella finestra»* — sia
`S2` del piano consolidato: *«non usare l'auto-run come oracolo della planning UI»*.

---

## 4. La seduta

### Passo ① — `PIE-VIS-SIGHTWALL`, un solo Play

⌫ **Fino al 2026-09-12 questo passo serviva due voci.** `PIE-HEXPLAY-6` è stata giudicata ✅ quel giorno
([#2697](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697), `runId 20260912-141110`, verdetto d'autore `PASS`): l'allestimento qui sotto resta
invariato, ma la domanda ora è una sola.

🔴 **Allestimento: una partita con PIANIFICAZIONE UMANA. Non l'auto-run di uno scenario** — §3.3.

Due vie, entrambe legittime, e nessuna delle due è il banco lasciato girare da solo:

| | Setup | Come |
|---|---|---|
| **A** | partita normale su `L_HexArena` | `rt.Test.Scenario ""` → Play → **`Home`** → si comanda un'unità e si pianifica |
| **B** | `Visual.Map.SightWallIsWalkable` con un **intento vivo** nella finestra | il banco isola la lastra, ma va tenuto in pianificazione: senza questo è la via ⌫ del §3.3 |

⚠️ **La via B non ha oggi un modo di ottenere quella finestra** — l'harness non cede il controllo, misurato
(§3.3). Finché non esiste, **la via A è l'unica eseguibile**, e va dichiarata come tale accanto al verdetto.

⚠️ **Che cosa la via A NON può isolare, e perché oggi non importa.** Le celle di barriera di `L_HexArena`
portano **entrambi** i flag — `bBlocksMovement` e `bBlocksLineOfSight` — quindi `RebuildInstances` dà loro
due volumi sovrapposti e la **lastra** non si giudica da sola:

| forma | misura | flag di origine |
|---|---|---|
| **lastra** | `0,75 × 10 cm` | `bBlocksLineOfSight` |
| **colonna** | `0,40 × 55 cm` | `bBlocksMovement` |

Non importa perché **quella domanda è caduta**: [D-340](../../decisions/RT_PDR_00_Decision_Log.md) ha scelto
la via *(b)* — far nominare la causa al log — e ha rinunciato **per iscritto** ad alzare `RTSightSlabHeight`.
La domanda residua non è più *«la lastra si vede?»* ma *«il giocatore collega la riga al segno?»*, e quella
non chiede di isolare le due forme.

⌫ *Fino al 2026-09-10 questo passo diceva «perché questo banco e non `L_HexArena`»: era corretto quando la
domanda era la lastra, ed è stato superato da D-340 e dal montaggio del feed.*

**Che cosa guardare, in quest'ordine:**

1. 🔑 **Guarda se la riga arriva a schermo PRIMA di aprire l'Output Log.** È la novità del 2026-09-10 ed è il
   punto di tutta la seduta: se apri il log prima, hai già saputo cosa cercare e la domanda è bruciata.
2. **Pianifica un tiro contro un bersaglio dietro una cella che blocca la vista.** Il click non deve creare
   un piano — `ARTPlayerController::HandleClickOnUnit` rifiuta **a monte** — e a schermo la cella che ha
   fermato il colpo porta un segno.
3. **La domanda, e va posta mentre pianifichi il turno seguente**: *chi guarda, senza sapere in anticipo
   cosa cercare, collega la riga del feed al segno sulla cella?*
4. Per `PIE-VIS-SIGHTWALL`, nello stesso Play: la coppia lastra/colonna si legge nella vista di gioco? Si
   distingue «non ci passo» da «non ci vedo attraverso»?
5. **La geometria**, che è ciò che #2870 ha lasciato a questa voce: spostandosi di **una cella di lato** il
   tiro va a segno, e con un ostacolo su un **altro layer** il tiro passa (regola di elevazione).

⛔ **Il segno vive fino al commit del turno, non un turno solo.** Sul banco in auto-run durava `1,5 s` perché
quella pausa *era* la pianificazione; in partita resta acceso per tutta la pianificazione vera. Presidiato da
`RefactorTactics.HUD.BlockerMarkLivesUntilNextLockIn`.

⚠️ **Il velo entra nella misura.** `VeilInstances` porta a scala **zero** i volumi delle celle mai viste
([D-225](../../decisions/RT_PDR_00_Decision_Log.md)): una cella non osservata non mostra nulla **per
progetto**. Non è un difetto di resa.

⛔ **Si giudica in PIE, alla camera del giocatore.** Chi cattura dal viewport dell'Editor vede gli anelli di
`DrawCellOverlay` — che però nascono **spenti** (`bCellOverlay = false`) — e crede di aver giudicato la vista
del giocatore.

✅ **E la via A porta un vantaggio che il banco non ha**: in partita entrano i quattro `BP_Unit_*`, mentre
**ogni** banco di scenario spawna il cilindro base — `RTScenarioSession.cpp:849` usa `ARTUnit::StaticClass()`.

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
