# GreyKit Background Geometry & Camera Boundary — referto del consumo (2026-09-05)

> `CURRENT` · **HEAD della revisione**: `origin/main` = **`215b23d2`** (2026-09-05 08:52 +0200), misurato
> alle **09:05 locali**. Albero di lavoro `b063a60f` (`fix/1793-2167-posa-e-offset`), **409 commit dietro**
> `origin/main`: tutte le misure di codice sono prese su `origin/main` con `git grep`/`git show`, **non**
> sull'albero locale. Verificato che i tre owner letti (`spec-domini-spaziali-mappa.md`,
> `spec-tactical-camera.md`, `docs/archive/src/README.md`) sono **identici** fra i due alberi — è la ragione
> per cui il worktree sarebbe stato costo puro.
>
> **Cosa è**: il referto di consumo del kit esterno *«GreyKit Background Geometry & Camera Boundary — Dual
> Roadmap»* (1048 righe, Roadmap A `A0`–`A9` + Roadmap B `B0`–`B21`).
> **Cosa non è**: una specifica, e nessuna sua prescrizione è stata applicata al codice.
> Gli owner restano [`spec-domini-spaziali-mappa.md`](../../technical/systems/spec-domini-spaziali-mappa.md),
> [`spec-tactical-camera.md`](../../technical/systems/spec-tactical-camera.md) e il
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md).
>
> **Archiviato in**: [`../../archive/src/handoff/2026-09-05-greykit-background-camera-dual-roadmap.md`](../../archive/src/handoff/2026-09-05-greykit-background-camera-dual-roadmap.md)
> (verbatim, `md5 4820a59b26cf7b21ec0510319a24443d`).

---

## 1. Il verdetto in una riga

Il kit chiede di separare tre concetti — sfondo visivo, spazio giocabile, limiti camera — che il
repository ha già separato il **2026-08-30** in **quattro** domini (`D-250`), e la sua premessa tecnica
centrale è **falsa qui**: questa camera non è vincolata da volumi di collisione, lo è da una **funzione
pura** (`D-251`, `#1778`, implementata e coperta headless), con `bDoCollisionTest = false` promosso a
vincolo da `D-253`.

---

## 2. Il precedente, cercato prima di revisionare

Questo è il **terzo** kit esterno sulla camera tattica, e i due precedenti sono in repository:

| Quando | Sorgente | Referto |
|---|---|---|
| 2026-08-14 | [`RefactorTactics_Camera_Roadmap_v1.0_Claude_Consolidation_2026-08-14.md`](../../archive/src/RefactorTactics_Camera_Roadmap_v1.0_Claude_Consolidation_2026-08-14.md) | [`camera-roadmap-v1-triage-2026-08-14.md`](camera-roadmap-v1-triage-2026-08-14.md) |
| 2026-08-30 | prompt *«Tactical Camera, Epic e GitHub Issues»* | [`tactical-camera-consolidamento-spec-panel-2026-08-30.md`](tactical-camera-consolidamento-spec-panel-2026-08-30.md) |
| 2026-09-05 | *questo* | questo file |

🔴 **La premessa della collisione SpringArm è alla terza ripetizione.** Il triage del 2026-08-14 l'aveva
già misurata falsa; il consolidamento del 2026-08-30 l'ha ritrovata e l'ha chiusa promuovendola a
decisione (`D-253`) *proprio perché* si ripresentava; questo kit la ripropone una terza volta al §1.4
(*«camera boom / spring arm penetration»*), al `A2` e al `B11`. Tre kit indipendenti hanno posto la stessa
domanda a cui il costruttore risponde da `#864`.

---

## 3. Il conto

| Oggetto | Quanti | Esito |
|---|--:|---|
| Voci Roadmap A (`A0`–`A9`) | 10 | 1 eseguita · 4 già canone/fatte · 2 senza oggetto · 2 gap reali · 1 bloccata |
| Voci Roadmap B (`B0`–`B21`) | 22 | **0 verificate** · 3 senza oggetto · 19 `NOT VERIFIED` |
| Premesse tecniche del kit falsificate | 4 | §4 |
| Domini spaziali proposti dal kit | 3 | il canone ne ha **4** — adottare i 3 sarebbe una **regressione** |
| Decisioni nuove | **0** | `D-250`, `D-251`, `D-253` coprono già la materia |
| Issue nuove | **0** | l'owner esiste ed è **aperta**: [#1777](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1777) (`CAM-08`) |
| Codice modificato | **0** | §7 — tre blocchi indipendenti |
| Test camera esistenti | **34** | `RefactorTactics.Camera.*`, nomi distinti in `RTCameraPawnTests.cpp` |

---

## 4. Le premesse del kit, misurate

Marcatura richiesta dal kit stesso (§0.8): `FACT` · `INFERENCE` · `PROPOSAL` · `QUESTION`.

### 4.1 🔴 `FACT` — la camera non è vincolata dalla collisione

Il kit costruisce §1.4, `A2` e `B11` sull'ipotesi di un *«blocking volume»* che ferma la camera, e teme
*«spring arm penetration»*, *«camera boom»*, *«near-plane clipping»*.

Misurato su `origin/main` `215b23d2`:

```
Source/RefactorTactics/Camera/RTCameraPawn.cpp:39:  SpringArm->bDoCollisionTest = false;
Source/RefactorTactics/Camera/RTCameraPawn.h:631:   ⚠️ Non e' collisione: bDoCollisionTest resta false e #864 lo dichiara fuori scope.
```

E `D-253` lo ha promosso da scelta di implementazione a **vincolo**: *«la collisione della SpringArm resta
al più una safety, mai il sistema primario»*.

∴ Il vincolo camera è **analitico**, non fisico: `ARTCameraPawn::ComputeEffectivePivotBounds(AllowedArea,
ArmLength, Pitch, Yaw, FOV, AspectRatio, AllowedOutsideFraction)` — funzione **pura**, che prende le
metriche come parametri invece di leggerle, *«ed è la ragione per cui è verificabile headless»*.

### 4.2 🔴 `FACT` — zero canali di collisione custom, e nessuna traccia camera

`A2` chiede di *«auditare i profili di collisione correnti»* e di verificare le risposte per camera,
pawn, LOS, targeting e navigazione.

| Cercato su `origin/main` | Risultato |
|---|---|
| sezione collisione in `Config/DefaultEngine.ini` | **assente** |
| `ECC_GameTraceChannel` in `Source/` | **0** |
| `SetCollisionResponseToChannel` in `Source/` | **0** |
| `ECC_Camera` in `Source/` | **0** |

∴ `A2` non ha oggetto: non esiste un'architettura di canali da auditare, e crearne una per lo sfondo
sarebbe il primo canale del progetto — cioè introdurre il sistema che `A2` credeva di trovare.

### 4.3 🔴 `FACT` — il pathfinding non usa NavMesh

`B12` chiede di visualizzare la navigazione, e prevede la propria esenzione: *«If tactical pathfinding is
entirely custom … document that and skip NavMesh-specific checks»*.

`RecastNavMesh`, `NavigationSystem`, `UNavigationSystemV1`, `ANavMeshBoundsVolume` in `Source/` e
`Plugins/`: **zero occorrenze**. ✅ Esenzione applicata: `B12` **senza oggetto**.

### 4.4 🔴 `FACT` — nessuna replicazione: `B15`/`B16` non hanno oggetto

`B15` chiede due client PIE, `B16` il reconnect. `GetLifetimeReplicatedProps` in `Source/` compare in
**un solo file**, ed è una fixture di test (`RTServerOnlyGuardFixturesForTest.h`). La v0.1 è **2v2 offline
contro bot** (`AGENTS.md` §1); l'online è milestone successiva.

∴ `B15`, `B16` e i requisiti di rete del §7 del kit: **senza oggetto in v0.1**. Restano validi come regola
— gli invarianti di authority/privacy (`AGENTS.md` §4) già la scrivono più stretta del kit.

### 4.5 ⚖️ `FACT` — il modello a tre livelli è più grossolano del canone

Il §2 del kit propone:

```text
Visual Background Layer  ·  Camera Constraint Layer  ·  Gameplay Layer
```

`D-250` (accettata il 2026-08-30) ne dichiara **quattro**, e la differenza non è cosmetica:

| `D-250` | Il kit | Perché la fusione perde |
|---|---|---|
| `PlayableMapBounds` | Gameplay Layer | ≡ |
| `ScenicBufferArea` | ⤵ fuso in *Visual Background* | è **vicino, guardato e attraversabile dal pivot**: LOD aggressivi lo rovinerebbero |
| `VisualBackgroundBounds` | ⤴ | è **lontano, mai guardato**: deve essere economico |
| `CameraTravelBounds` | Camera Constraint Layer | ≡ |

Il buffer scenico e lo sfondo distante hanno **budget opposti** e un confine diverso col pivot: `D-250`(3)
dice che il pivot **deve** poter entrare nel buffer, *«altrimenti il bordo non si porta al centro dello
schermo e non si vede cosa c'è lì»*. Fonderli rimetterebbe la stessa decisione in un solo campo.

⛔ **Adottare il modello a tre livelli sarebbe una regressione documentale**, e creerebbe la seconda source
of truth vietata da `AGENTS.md` §8.

### 4.6 ✅ Cosa il kit indovina, e che non è nuovo

- §1.2 *«il fallimento peggiore è una mesh che è insieme scenario, blocco del path e bordo giocabile»* →
  è `D-250`(2) alla lettera, con l'aggravante che il canone nomina la decisione che si rompe: `D-143`.
- §1.3 *«false affordance»* → è il *Boundary Language* di `spec-domini-spaziali-mappa.md` §3, con la
  tabella per ambiente (città, industriale, montagna, foresta, porto, tunnel, tetti, zona militare).
- §1.5 *«non rappresentare lo sfondo come celle ad alto costo»* → `D-250`(2): *«senza celle, senza spawn,
  senza bersagli»*. Il kit e il canone concordano, e il canone porta il test decisivo che il kit non ha:
  **«se un'unità sparasse attraverso, cosa deve succedere?»** — se la risposta è *deve fermarsi*, quella
  struttura non appartiene al buffer.
- §10 *Success criterion* → è la Definition of Done di `#1777`, scritta prima.

---

## 5. Roadmap A — voce per voce

| Voce | Esito | Evidenza |
|---|---|---|
| `A0` Repository audit | ✅ **ESEGUITO** — questo documento | §4, §6 |
| `A1` Define responsibilities | 🔄 **GIÀ CANONE** | `D-250` · [`spec-domini-spaziali-mappa.md`](../../technical/systems/spec-domini-spaziali-mappa.md) · `#1777` |
| `A2` Collision model | ⛔ **SENZA OGGETTO** | §4.2 — zero canali, zero risposte, camera non fisica |
| `A3` GreyKit background primitives | 🟡 **GAP REALE, owner esistente** | §6.3 |
| `A4` Camera constraint implementation | ✅ **GIÀ FATTO** | `#1778` chiusa · `ComputeEffectivePivotBounds` · `D-251` |
| `A5` Gameplay isolation | 🔄 **GIÀ INVARIANTE** | `AGENTS.md` §3 · `D-143` · `D-250`(2) |
| `A6` Debug visualization | 🟡 **GAP REALE** | §6.2 — la famiglia `rt.Debug.Draw*` esiste, i **bounds no** |
| `A7` Automated tests | 🟡 **PARZIALE** | 34 test `Camera.*`, fra cui `EffectivePivotBoundsShrinkWithZoomPitchAndAspect` |
| `A8` Performance sanity | ⛔ **NOT RUN** | motore occupato, §7.3 |
| `A9` Implementation gate | 🔴 **BLOCCATO** | `origin/main` **non compila** — §7.1 |

---

## 6. I due gap reali, e chi li possiede

### 6.1 Il punto d'innesto è già marcato nel codice

`ScenicBufferArea` non esiste come dato, e lo spec lo dichiara con un `⛔` invece di lasciarlo dedurre. Il
codice porta il segnaposto:

```cpp
// RTCameraPawn.cpp:499
// La zona che si puo' mostrare: celle piu' il margine. ⏳ Quando `ScenicBufferArea` esistera' come dato
// (`D-250`, `#1777`) e' **questo** il punto che deve leggerlo — il buffer allarga l'area consentita, non
// il margine in celle.
const FBox2D Allowed(FVector2D(Min.X - Margin, Min.Y - Margin), FVector2D(Max.X + Margin, Max.Y + Margin));
```

∴ La *«minimum system that can be tested»* che il kit chiede al §0.3 esiste già come **una riga da
sostituire**, non come un sistema da progettare.

### 6.2 `A6` — non esiste debug dei bounds

`rt.Debug.DrawCells`, `DrawCover`, `DrawIntent`, `DrawPaths`, `DrawResolution`, `Los`, `Knowledge`,
`Pacing` esistono. **Nessuna** CVar disegna i limiti — né `PlayableMapBounds`, né `CameraTravelBounds`, né
l'`EffectivePivotBounds` effettivo a un dato zoom/pitch/aspect.

⚠️ Ed è il difetto che rende impossibile eseguire onestamente `B2` e `B13`: entrambi chiedono di *vedere*
che i tre domini sono distinti, e oggi non c'è niente da accendere. Il kit lo dice bene al `A6`:
*«do not depend on visual guesswork»*.

`PROPOSAL` — appartiene alla DoD di `#1777` (*«i quattro domini esistono come dati interrogabili»*): un
dominio non interrogabile non è disegnabile.

### 6.3 `A3` — il gray kit esiste, lo sfondo no

`Content/RT/World/Graybox/` ha `SM_Graybox_Cover_High/Low`, `Door_Panel/Locked`, `Surface_Water/Ice`,
`BP_Graybox_CellPlacementVolume`, `BP_Graybox_UnitFacingFixture` e i materiali. È il kit **giocabile**:
nessuna primitiva di sfondo, e la scelta di dove viva la geometria scenica sotto `/Game/RT/` è una casella
esplicita della DoD di `#1777` (*«`convenzioni-contenuti-ue.md` dice dove va la geometria scenica»*).

⚠️ `Scenarios/Visual/Map/GrayKitYard.json` dichiara già il limite gemello: *«La fixture porta il DATO, non
le mesh del kit. `SM_Graybox_*` sono asset da posare in scena e nessuna fixture li posa»*.

---

## 7. Perché Roadmap A non è stata implementata — tre blocchi indipendenti

Ognuno da solo basterebbe. Nessuno dipende dagli altri.

### 7.1 🔴 `origin/main` non compila

[#2397](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2397), aperta oggi alle 06:34Z:
due `StandStill` omonime in anonymous namespace (`RTStatusTests.cpp:97`, `RTUnbalancedProneTests.cpp:103`)
collidono sotto unity build. **Verificato ancora vero** su `215b23d2`: entrambe le definizioni sono al loro
posto. Il fix è in volo — PR [#2398](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2398),
worktree `D:/rt-wt-2387` su `fix/2409-standstill-unity-collision`.

> ✅ **SBLOCCATO alle 11:02 locali.** `#2398` mergiata — `origin/main` = **`4cc23af9`** — e la build misurata,
> non dedotta: `RefactorTacticsEditor Win64 Development` su un worktree pulito e **senza makefile
> preesistente**, `Result: Succeeded`, exit 0, 211,56 s.
>
> 🔑 **E non è un verde per raggruppamento fortunato**: i due file sono finiti nello **stesso** blob unity —
> `Module.RefactorTactics.19.cpp`, righe 5 e 16 su 15 include — cioè esattamente la condizione che
> falliva. Il blob ha prodotto un `.obj` da 10,3 MB. ⚠️ `#2410` era un duplicato dello stesso fix di
> un'altra sessione, e resta aperta.
>
> ✅ **E la suite intera è VALIDA sullo stesso commit**, misurata alle 11:14 sul binario di quella build:
> `HEAD 4cc23af9 · albero ae48caf4 · 2022/2022 completati, 0 fallimenti · 06:42`. Lo script ha atteso 254 s
> il lock del motore e **ha ridichiarato lo stato al risveglio**: HEAD, digest dell'albero e mtime di
> entrambi i moduli **identici** prima e dopo l'attesa.
>
> 🔑 **Il limite che `rt-suite` dichiara di non coprire qui non si applica**: *«un DLL compilato da un
> commit diverso resta identico dall'inizio alla fine, quindi passa»*. Il binario porta `11:00:08` ed è
> quello che ho costruito io da questo `HEAD` su albero pulito — la provenienza copre ciò che l'invariante
> non vede.
>
> ∴ Il §7.1 **non è più un blocco**, e i tre `USER CHECK` del §9 sono eseguibili. Restano `7.2` (nessun
> MCP) e `7.4` (`#1777` è `post-v0.1`).

∴ Il gate `A9` (*«project compiles; automated tests pass»*) **non è soddisfacibile**, e tutta la Roadmap B
si ferma a `B0`.

### 7.2 ⛔ Nessun MCP Unreal in questa sessione

`.mcp.json` dichiara `unreal-mcp` su `http://127.0.0.1:8765/mcp`. Misurato: **nessun tool `mcp__unreal*`
esposto**, e l'endpoint risponde `HTTP 000` (non raggiungibile).

Il kit prevede il proprio caso al §0.7: *«If a required check cannot be performed through MCP, create a
precise user-assisted checklist instead of pretending it was verified»* — §8.

### 7.3 ⚠️ Il motore è occupato da un'altra sessione

`UnrealEditor-Cmd` PID **80364**, avviato alle 09:02:58, su un albero **diverso**:

```
D:\Repositories\rt-wt-2184d\RefactorTactics.uproject
-ExecCmds=Automation RunTests RefactorTactics.HUD.SlotLine+RefactorTactics.HUD.MatchEndHeadline;Quit
```

⛔ **Non terminato, ed è una scelta.** È la misura di un'altra sessione, su lavoro non correlato a questo
kit: interromperla la renderebbe `NON VALIDA` senza dare niente a nessuno. `AGENTS.md` §11 —
*«un worktree separato non elimina il mutex globale»* — chiude anche la strada al build parallelo.

✅ Verificato che quell'albero **non è questo**: le scritture documentali di questo referto non entrano nel
suo digest e non invalidano la sua misura.

### 7.4 E il quarto motivo, che non è un blocco ma una scelta di scope

`#1777` è etichettata **`P2` · `post-v0.1`**, senza milestone. `D-286` ha promosso in v0.1 il lavoro
camera del 2026-08-30, ma **elenca cosa promuove** — `ZoomAlpha`, stato camera nell'HUD, dominio di
navigazione legato alla Team Knowledge, `Home` contestuale, pipeline di pan unificata — e `CAM-08` non è
fra quelli. `D-286`(4) è esplicita: *«non promuove l'intera E49»*.

∴ Implementare oggi i quattro domini **sposterebbe lo scope della release senza una decisione**, che è
esattamente ciò che `D-286` esiste per non far fare di lato.

---

## 8. Roadmap B — matrice, marcata come il kit richiede al §5

Nessuna voce è marcata su ispezione del codice: il kit lo vieta, e sarebbe falso.

| Voce | Metodo possibile | Esito | Perché |
|---|---|---|---|
| `B0` Editor startup | MCP / User | 🔴 `NOT VERIFIED` | §7.1 — non compila; §7.2 — MCP assente |
| `B1` Structure inspection | MCP / User | 🔴 `NOT VERIFIED` | idem |
| `B2` Visual separation | User | 🔴 `NOT VERIFIED` | §6.2 — **non c'è debug da accendere** |
| `B3` Hex/grid generation | User | 🔴 `NOT VERIFIED` | §7.1 |
| `B4` Pathfinding near edges | User | 🔴 `NOT VERIFIED` | §7.1 |
| `B5` Camera pan | User | 🔴 `NOT VERIFIED` | §9 — checklist pronta |
| `B6` Rotation/orbit | User | 🔴 `NOT VERIFIED` | §9 |
| `B7` Zoom | User | 🔴 `NOT VERIFIED` | §9 |
| `B8` Focus / camera command | User | 🔴 `NOT VERIFIED` | copertura headless esiste (`FocusIsClampedLikeEveryOtherPivotWrite`), la percettiva no |
| `B9` Resolution phase | User | 🔴 `NOT VERIFIED` | §7.1 |
| `B10` LOS / targeting | User | 🔴 `NOT VERIFIED` | ⚠️ la risposta di design è già data: `D-143` + `D-250`(2) — lo sfondo **non** occlude la LOS |
| `B11` Collision validation | — | ⛔ **SENZA OGGETTO** | §4.2 |
| `B12` Navigation visualization | — | ⛔ **SENZA OGGETTO** | §4.3 — esenzione prevista dal kit stesso |
| `B13` Debug bounds stress | User | 🔴 `NOT VERIFIED` | §6.2 |
| `B14` Save / reload / PIE | User | 🔴 `NOT VERIFIED` | §7.1 |
| `B15` Multi-client PIE | — | ⛔ **SENZA OGGETTO** | §4.4 |
| `B16` Disconnect / reconnect | — | ⛔ **SENZA OGGETTO** | §4.4 |
| `B17` Editor transform ergonomics | User | 🔴 `NOT VERIFIED` | niente da trasformare: le primitive non esistono (§6.3) |
| `B18` Extreme transforms | User | 🔴 `NOT VERIFIED` | idem |
| `B19` Performance editor sanity | User | 🔴 `NOT VERIFIED` | §7.3 |
| `B20` Packaging / cook | Automation | 🔴 `NOT VERIFIED` | §7.1 |
| `B21` Final regression matrix | — | 🔴 `NOT VERIFIED` | consolidamento delle precedenti |

**`MCP VERIFIED` 0 · `AUTOMATION VERIFIED` 0 · `USER VERIFIED` 0 · `NOT VERIFIED` 19 · senza oggetto 3.**

---

## 9. `USER CHECK` — le tre verifiche percettive che il canone aspetta da sei giorni

Non sono del kit: sono i `⏳` che `spec-domini-spaziali-mappa.md` §2 dichiara aperti dal 2026-08-30, e il
kit ha il merito di ricordarli. Si eseguono **dopo** che [#2398](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2398)
è mergiata e il progetto ricompila.

```text
USER CHECK 1 — dove la camera comincia a irrigidirsi
1. Apri L_HexArena (o L_DevSandbox) in PIE.
2. Porta il pivot a un angolo della mappa.
3. Zoom out continuo da MatchStartArmLength (450) a MaxArmLength (4000).
4. Attesa: il pivot si stringe verso il centro in modo progressivo.
5. Da registrare: la distanza a cui lo scorrimento smette di rispondere.
   È il numero che `AllowedOutsideFraction` sposta, e oggi non è mai stato misurato a schermo.
6. Evidenza: video breve, o due screenshot con il valore di zoom leggibile.
```

```text
USER CHECK 2 — il caso 32:9, che è quello che ha motivato D-251
1. Avvia PIE in una finestra ultrawide (o forza l'aspect ratio).
2. Pitch quasi orizzontale, distanza massima.
3. Ruota lo yaw per l'intero giro ai quattro angoli.
4. Attesa: nessun lato dello schermo mostra il vuoto esterno.
5. Fallimento atteso e da registrare: il buffer scenico NON esiste ancora come dato (§6.1),
   quindi oltre il bordo non c'è niente da vedere — il test misura quanto vuoto, non se.
6. Evidenza: screenshot per ogni angolo.
```

```text
USER CHECK 3 — la mappa piccola, dove un Clamp ingenuo sbaglia in silenzio
1. Mappa di raggio piccolo (o riduci DemoArenaRadius via rt.Match.DemoArenaRadius).
2. Zoom massimo.
3. Prova a spostare il pivot in ogni direzione.
4. Attesa: il pivot resta al CENTRO e non si incolla a un angolo.
5. È il caso che `Camera.EffectivePivotBoundsShrinkWithZoomPitchAndAspect` copre headless;
   qui si verifica che a schermo non si legga come «camera rotta».
6. Evidenza: screenshot ai quattro tentativi di pan.
```

---

## 10. Il formato di report che il kit chiede al §8

**IMPLEMENTED** — nulla. Zero file di `Source/`, zero `.uasset`, zero `Config/`. §7.

**ARCHITECTURE** — invariata e già scritta: quattro domini (`D-250`), limite del pivot come funzione del
viewport (`D-251`), occlusione in presentazione con collisione SpringArm spenta (`D-253`).

**AUTOMATED TESTS** — nessuno aggiunto. I 34 esistenti **non sono stati eseguiti**: §7.1 e §7.3.

**EDITOR CHECKS** — §8, tutte `NOT VERIFIED`.

**REGRESSIONS FOUND** — una, e non è di questo kit: `origin/main` non compila
([#2397](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2397)), già tracciata e con PR in
volo. Nessuna regressione introdotta da questo lavoro: il write-set è documentale.

**OPEN QUESTIONS**

1. `QUESTION` — `#1777` resta `post-v0.1`, o il kit è la richiesta d'autore di promuoverla? Serve una
   riga nel Decision Log come `D-286`, non un'inferenza.
2. `QUESTION` — dove vivono i domini nei dati: volume in mappa, campo di `URTHexMapAsset`, o convenzione
   di cartella? `D-250`(4) rifiuta esplicitamente di deciderlo a tavolino, e `#1777` possiede la scelta.
3. `QUESTION` — `AllowedOutsideFraction` non ha ancora un valore istruito. §9 lo misura.

**FOLLOW-UP** — §11.

---

## 11. Azioni raccomandate, **elencate e non eseguite**

Le scritture su GitHub restano una decisione dell'autore.

| # | Azione | Su |
|---|---|---|
| 1 | Commento su `#1777`: il punto d'innesto è `RTCameraPawn.cpp:499`, e il kit lo conferma dall'esterno | [#1777](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1777) |
| 2 | Aggiungere alla DoD di `#1777` la casella mancante: **debug dei bounds** (§6.2) — nessuna CVar li disegna | [#1777](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1777) |
| 3 | Decidere la milestone di `#1777` (§10, domanda 1) | Decision Log |
| 4 | Mergiare [#2398](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2398) **prima** di qualunque misura su `origin/main` | PR #2398 |
| 5 | Eseguire i tre `USER CHECK` del §9 e riportarli in `spec-domini-spaziali-mappa.md` §2 | spec owner |

⛔ **Nessuna issue nuova.** Il kit non nomina un solo requisito che `#1777`, `#1779`, `#1780` non
possiedano già.

---

## 12. Limiti di questo referto

- **Nessuna misura di runtime.** Niente build, niente suite, niente PIE, niente packaged: `NOT RUN`, per
  i motivi del §7. Un verde non è stato osservato e non è stato dichiarato.
- **Le misure di codice valgono su `origin/main` `215b23d2`**, non sull'albero locale `b063a60f`. Il
  repository è lavorato in parallelo: `origin/main` è avanzato di 409 commit in ~39 ore, e questo
  documento **scade quando `origin/main` avanza o quando `#1777`/`#2397` cambiano stato**.
- **I 34 test camera sono contati, non eseguiti**: `grep` di nomi distinti su `RTCameraPawnTests.cpp`.
- Il kit non porta sha né ora: non può accorgersi di invecchiare. Questo è il motivo per cui l'intestazione
  di questo file ne porta due.
