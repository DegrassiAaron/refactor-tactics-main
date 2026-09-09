# #2766 — «Fuori portata (max 5)» a distanza 3 · panel di specifica · 2026-09-09

> **Comando**: `/issue-run 2766` → `sc:spec-panel`, modalità **critique**, focus `requirements · architecture`.
> **Panel**: Wiegers (lead) · Cockburn · Adzic · Nygard · Crispin.
> **Base**: `main = 6de2a58c`, worktree isolato `rt-wt-2766`.

## §0 — ⛔ Il rilievo centrale della prima stesura era falso

La prima stesura di questo panel sosteneva che il caso del fumo fosse un difetto di **classificazione**: che `OutOfRange` col cap del terreno *«si corregge spostandosi di lato, non avvicinandosi»*, e quindi violasse il criterio scritto in `RefusalDistinguishesCoverFromRange`.

🔴 **Falsificato da una domanda dell'autore**: *«ma se ti avvicini e la skill porta il raggio oltre la portata iniziale?»*

`EffectiveTargetingRange` restituisce `Min(RangeCells, cap)` e il rifiuto scatta su `HexDistance > Effective`. Col fumo a `2`, **avvicinarsi a distanza ≤ 2 fa passare il tiro**: il cap è un limite di *distanza*, non un blocco della traiettoria — il fumo ha `bBlocksLineOfSight = false`. La linea continua ad attraversarlo, e il colpo parte lo stesso.

∴ **avvicinarsi funziona.** Il messaggio *«Troppo lontano per questa abilità»* suggerisce l'azione **giusta**, anche col fumo. Il panel aveva dedotto il contrario senza misurare il predicato.

⚠️ La stesura precedente non è conservata: non era una misura poi superata, era un errore di lettura del `Min`.

## §1 — Che cosa resta, misurato

| Fatto | Dove |
|---|---|
| il classificatore confronta con la portata **effettiva** | `EffectiveTargetingRange(Map, From, To, RangeCells)` |
| il cap si applica per cella della linea, come minimo | `RTTerrainLibrary.cpp:117` — `Min(Effective, Def.MaxTargetingRangeThrough)`, sotto la guardia `> 0` |
| 🔴 il log stampa la portata **dichiarata** | `RTPlayerController.cpp:1480` — `Ability->RangeCells` |
| il messaggio al giocatore **non contiene numeri** | `RTHUD.cpp:78` — *«Troppo lontano per questa abilità»* |
| la portata numerica **non raggiunge lo schermo** | unico uso in `UI/`: `RTHUD.cpp:133`, che passa `RangeCells` a `HexHitCells` per l'anteprima di un piano **già accettato** |
| una sola superficie cappa | `Smoke`, a `2`; le altre sette del catalogo hanno `0` |

🔑 **Il difetto è quindi uno solo, e sta nel canale diagnostico.** Il log dichiara di *«dire il vero per intero»* (`RTPlayerController.cpp:1473`) e stampa un numero che non è il limite applicato. Chi lo legge vede `3 ≤ 5` e conclude che il classificatore è rotto: è successo davvero, in una code review su #2754, ed è il sintomo che ha aperto questa issue.

## §2 — I rilievi

### 🔴 WIEGERS — il DoD chiede due cose, e la seconda si chiude con una dichiarazione

Il primo punto (*il log nomina la portata effettiva*) è una correzione senza scelte.

Il secondo (*il messaggio distingue, **oppure è dichiarato per iscritto perché non deve***) trova la sua risposta nel §0: **non deve**, perché il suggerimento è già corretto. La issue offriva entrambe le uscite, e la misura sceglie la seconda.

### 🟡 COCKBURN — resta un'informazione che manca, e non la introduce il fumo

Il giocatore sa *che* è troppo lontano, non *di quanto*. ⚠️ Questo vale **anche senza fumo**: il messaggio non ha mai contenuto numeri. Il cap non peggiora la classe di informazione, la rende solo più sorprendente — chi conosce la portata dell'abilità dalla scheda si aspetta 5 e ne ottiene 2.

📝 → `FOLLOW-UP CANDIDATE`, non lavoro di questa issue: *«il rifiuto per distanza dice di quanto»* è una feature dell'HUD, non una correzione.

### 🟢 NYGARD — il TurnLog non eredita l'inganno, e la ragione è misurata

`RTTurnManager.cpp:4108` confronta con `Def.RangeCells` **direttamente**, senza passare da `EffectiveTargetingRange`: il cap non entra in quel ramo, quindi non può stamparne una versione sbagliata.

⚠️ Ma la misura ne apre un'altra che questa issue **non** deve risolvere: esistono **due regole di portata**, e solo una legge il terreno. Può essere legittimo — quel ramo tratta bordi dichiarati, dove la linea di tiro non è in gioco — ma nessun documento lo dichiara. → `FOLLOW-UP CANDIDATE`.

### 🟢 ADZIC — l'esempio, e le due sponde che il test deve fissare

```
Fumo sulla linea (cap 2) · abilità RangeCells = 5 · bersaglio a distanza 3
Oggi   → log «[RT] Branth fuori portata (max 5)»    ← il numero non è il limite applicato
Atteso → il log nomina 2, e dice che 5 era la dichiarata
```

⚠️ Il test deve fissare **entrambe**: `3 ≤ RangeCells` (quindi non è un fuori portata ordinario) **e** `3 > EffectiveRange` (quindi il rifiuto è legittimo). Asserire solo la seconda passerebbe anche con `RangeCells = 2`, dove non c'è nulla da correggere.

### 🟢 CRISPIN — la mutazione, e il verso opposto

*«Rimettere `RangeCells` al posto della portata effettiva deve far cadere quel test»* è il controllo giusto.

⚠️ Serve anche il caso **senza** cap: portata dichiarata ed effettiva coincidono, e il messaggio non deve guadagnare una precisazione che non ha ragione d'essere. Senza, un'implementazione che stampasse sempre due numeri passerebbe.

## §3 — Consenso

1. ✅ **Si corregge il log, e solo il log.** È l'unico canale che afferma un numero falso.
2. ✅ **Il messaggio al giocatore resta invariato**, e la ragione va **scritta**: l'azione che suggerisce è corretta anche col cap, perché il cap limita la distanza e non la traiettoria.
3. ⛔ **Nessun valore nuovo negli enum.** La prima stesura ne proponeva uno (`Obscured`): sarebbe nato per un difetto che non esiste, e ogni consumatore avrebbe dovuto trattarlo.
4. ⚠️ **Due test**: il caso col cap attivo, e il caso senza cap che deve restare invariato.
5. 📝 **Due follow-up**, entrambi fuori scope: il rifiuto che dice *di quanto*, e le due regole di portata che divergono sul terreno.

## §4 — Raccomandazione

Il log del ramo `OutOfRange` nomina la portata **effettiva**, e quando differisce dalla dichiarata dice entrambe. Nessuna modifica a `ERTHexTargetReason`, a `ERTTargetRefusal`, né a `RefusalText`.
