# Le quattro decisioni d'autore sul movimento — sincronizzazione Drive ↔ repository

> **Referto di sincronizzazione**, non owner. Consuma le righe `AUTHOR-MOVE-001`, `AUTHOR-FACING-002`,
> `AUTHOR-PIVOT-001` e `AUTHOR-MOVETIME-001` del tab **Decisions** del Google Sheet
> *RT — Knowledge Index & Consolidation Log* (`1GOd_Hi3bZBM0NMXQ7oAKV8XxlzPdywtpgCjWtsZsoeo`), tutte
> marcate `Accepted — pending repo sync` e datate *«Decision session 2026-08-31»*.
>
> **Data**: 2026-08-31 · **Base**: `origin/main` @ `0eadc681` · **Modo**: critique · **Focus**: requirements
> **Esito**: [`D-295`](../../decisions/RT_PDR_00_Decision_Log.md) +
> [#1922](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1922). **Nessuna riga di `Source/`
> toccata.**

---

## 1. Il verdetto in una riga

> **Tre delle quattro erano gia' canone o gia' vere per costruzione. La quarta e' nuova solo per due delle
> sue sette clausole — e quelle due rovesciano un test verde, in un punto dove il repository aveva
> esplicitamente parcheggiato la domanda in attesa di una persona.**

---

## 2. Come ci si e' arrivati: il Drive dichiarava 31 pendenze e ne aveva 4

Il tab **Decisions** porta **31** righe `Accepted — pending repo sync`. Verificate una per una contro
`docs/` (archivio escluso): **27 sono gia' nel repository**. Il `pending` e' un residuo del foglio, non
lavoro arretrato — nessuna sincronizzazione aggiorna la colonna `Status` a valle del merge.

Le quattro davvero assenti sono queste, e appartengono tutte allo stesso dominio e alla stessa seduta.

⚠️ **Il conteggio del Drive non e' quindi un indicatore di debito.** Chi lo legge come tale pianifica
ventisette sincronizzazioni che non esistono e ne manca quattro che esistono. La misura utile e' il token
(`AUTHOR-*`) cercato in `docs/`, non lo stato dichiarato nel foglio.

---

## 3. Ciò che è stato misurato

Ogni riga è un comando eseguito su `origin/main` @ `0eadc681`, non una lettura.

| Clausola | Sede | Esito |
|---|---|---|
| micro-step da un solo `State N`, ordine irrilevante | `RTHexSimLibrary.cpp` `StepHexMovement` | ✅ **punto fisso monotono** — *«si puo' solo passare da "in movimento" a "fermo"»*. `HexSim.ResolveOrderIndependent` |
| destinazione contesa a parita' → fermi tutti | idem | ✅ `HexSim.ResolveContestedDestination` · `HexSim.ResolvePriorityTieStillContested` |
| coda che fallisce propaga il blocco all'indietro | idem, ramo *«bloccata da un'unita' che RESTA»* | ✅ `HexSim.ResolveBlockedByStationary` — e' il ciclo `while (bChanged)` a propagarlo |
| convoy a coda libera avanza | idem | ✅ per costruzione (blocca solo chi e' fermo) — ⚠️ **nessun test esplicito** |
| nessun danno da collisione | `RTHexSimLibrary.cpp` | ✅ **0 occorrenze** di `ApplyDamage`/`Health` in tutto il file |
| identita' di cella sul `FRTCellId` completo, Layer incluso | `RTCellId.h:47` | ✅ `operator==` confronta `X && Y && Layer` |
| **scambio diretto blocca** | idem | ⛔ **riesce** — `HexSim.ResolveSwapAllowed` ✅ verde, e `ERTMoveOutcome::Moved` documenta *«scambio incluso»* |
| **ciclo chiuso blocca** | idem | ⛔ **ruota**, e **nessun test** lo copre |
| facing dalla transizione riuscita | `adr-0008` §2, `FAC-4` | ✅ **gia' canone** — *«il facing al boundary `k` e' la direzione dell'ultimo passo compiuto»*, `FacingFinalAfterMove` di `D-020` |
| pivot finale non retroattivo | `adr-0008` | ✅ **gia' canone**, pinnato da `Facing.FinalPivotIsNotRetroactive` |
| costo MP ≠ durata del micro-step | firma di `ResolveHexPaths` | ✅ **piu' forte di come la decisione lo chiede**: il resolver **non riceve affatto** il costo (`Paths`, `Priorities`, `bLinearMovers`, `bPassThrough`), e un micro-step avanza di esattamente una cella (`Paths[i][Prog[i] + 1]`) |

---

## 4. Le quattro voci

### 4.1 `AUTHOR-FACING-002` e `AUTHOR-PIVOT-001` — gia' canone, si registra l'equivalenza

Nessuna azione oltre la registrazione. `ADR-0008` decide entrambe da prima, `FAC-4` e' barrata in
[`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) con la stessa formulazione, e il pivot ha un test che lo
pinna per nome. Il valore di sincronizzarle non e' cambiare qualcosa: e' che il prossimo kit che le
riproponga le trovi gia' risposte, con il proprio `AUTHOR-*` accanto.

### 4.2 `AUTHOR-MOVETIME-001` — vera per costruzione, e il repository e' piu' severo del testo

La decisione chiede che *«higher per-cell MP cost does not automatically create additional temporal
microsteps»*. Nel repository la separazione non e' una regola da rispettare: e' **strutturale**, perche' il
resolver non ha alcun modo di conoscere il costo — non compare nella firma. Il costo vive interamente nella
pianificazione (`ReachableCells`, budget), e la risoluzione consuma un percorso gia' calcolato, una cella per
micro-step.

⚠️ La clausola operativa resta comunque utile: *«any multi-microstep traversal-duration mechanic must be an
explicit separate deterministic rule with its own tests»*. E' un divieto per il futuro, non una correzione.

### 4.3 `AUTHOR-MOVE-001` — cinque clausole su sette gia' spedite, due che rovesciano un test verde

Le cinque implementate stanno nella tabella del §3 e non richiedono nulla. Le due che divergono sono
**lo scambio diretto** e **il ciclo chiuso**.

Il punto fisso blocca per due sole condizioni: destinazione contesa (2+ unita' in movimento verso la stessa
cella) e unita' **ferma** sulla destinazione. In uno scambio `A↔B` nessuna delle due si verifica — le celle
sono distinte e nessuna delle due unita' e' ferma — quindi entrambe si muovono. Lo stesso vale per un ciclo
di lunghezza qualsiasi. L'unica eccezione esistente e' lo **scontro frontale fra due mobilita' lineari**
(`BlockedByImpact`, CP 4.8), che e' una regola sua e resta.

🔑 **E la decisione arriva esattamente dove il repository l'aspettava.**
[`../../technical/runbooks/test-e-diagnosi.md`](../../technical/runbooks/test-e-diagnosi.md) registra gia'
lo scarto — il resolver consente, il planner rifiuta, *«insieme rendono la regola del resolver
**irraggiungibile**»* — e chiude con una consegna esplicita: *«Se lo scambio debba essere possibile e' una
**decisione di design**. Il difetto si fissa con una caratterizzazione e si segnala; chi decide, decide.»*

⛔ **Quella domanda non aveva owner**: nessuna voce in `OPEN_DECISIONS.md`, nessuna issue, solo un paragrafo
di runbook dove nessun audit la cerca. E' il motivo per cui `AUTHOR-MOVE-001` e' rimasta `pending repo sync`
mentre ventisette sue coetanee erano gia' dentro.

🔴 **Un owner scritto dice gia' l'opposto del codice.**
[`../../gameplay/spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md) §19 dichiarava
*«contesa e swap bloccano entrambi — gia' implementato»*: vero per la contesa, **falso per lo scambio**.
`D-295` corregge quella riga — non e' una decisione che cambia, e' una misura che era sbagliata.

---

## 5. Test

**Eseguiti: nessuno.** Il pass non tocca `Source/`. I test citati sono lo **stato dichiarato** del
repository, non una misura presa qui.

I tre test che [#1922](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1922) deve produrre:

| Test | Cosa distingue |
|---|---|
| `ResolveSwapAllowed` → **rinominato e invertito** | oggi asserisce l'opposto della decisione: e' il test che cade per primo |
| ciclo chiuso `A→B→C→A` | **non esiste**, e il punto fisso lo lascia ruotare in silenzio |
| convoy a coda libera `A→B→C→(libera)` | **non esiste**, ed e' il caso che la modifica non deve rompere — oggi passa solo per costruzione |

⛔ `ResolveHeadOnBlocksLinearSwap` **resta verde**: dopo la modifica lo scambio blocca in entrambi i casi, ma
con reason code diversi, e `ERTMoveOutcome` e' **serializzato nei replay**.

---

## 6. Cosa NON e' stato fatto, e perche'

| Non fatto | Perche' |
|---|---|
| Cambiare `StepHexMovement` | e' gameplay che rovescia un test verde in un enum serializzato nei replay: si istruisce in [#1922](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1922) con la sua caratterizzazione, non in un commit di governance |
| Aprire una voce in `OPEN_DECISIONS.md` | la domanda non e' piu' aperta: l'autore ha risposto. Il residuo e' implementazione, e ha una issue |
| Toccare `ADR-0008` | `AUTHOR-FACING-002` e `AUTHOR-PIVOT-001` non lo emendano: lo confermano |
| Aggiornare la colonna `Status` sul Drive | `update_file` dell'MCP accetta solo `title` e `parentId`: nessuna cella e' scrivibile (§7) |

---

## 7. Drive — da aggiornare a mano dopo il merge

Tab **Decisions**, colonna `Status`, identificando la riga per `Decision ID`:

| `Decision ID` | Nuovo `Status` |
|---|---|
| `AUTHOR-MOVE-001` | `Accepted — repo synced (D-295); implementation open in #1922` |
| `AUTHOR-FACING-002` | `Accepted — already canonical (ADR-0008 §2 / FAC-4 / D-020); registered by D-295` |
| `AUTHOR-PIVOT-001` | `Accepted — already canonical (ADR-0008); registered by D-295` |
| `AUTHOR-MOVETIME-001` | `Accepted — repo synced (D-295)` |

⚠️ **Le altre 27 righe `pending repo sync` sono da rileggere, non da sincronizzare**: il loro
`AUTHOR-*` e' gia' nel repository. Aggiornarne lo `Status` e' manutenzione del foglio, non lavoro sul
codice.

Tab **Open Questions**: `DQA-031` (`COV-1`) e `DQA-032` (`COV-2`) sono `RESOLVED` sul Drive ma
[#1833](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1833) porta ancora tutte e otto le
`COV-*` come aperte — recepite in un commento alla issue il 2026-08-31.
