# Consumo del kit *Create GrayKit Combat Feedback Issues*

> **HEAD della revisione**: `a254da94` (worktree `D:/rt-wt-eatdoc`, 0 commit di scarto da `origin/main`)
> · **Ingresso**: `/eat-doc` — **nessun panel**, quindi questo non è un referto di `/sc:spec-panel`: è il
> verbale di un consumo · **Sorgente**: 748 righe, letto integralmente
> · **Archiviato in**: [`../../archive/src/handoff/2026-09-05-graykit-combat-feedback-issues.md`](../../archive/src/handoff/2026-09-05-graykit-combat-feedback-issues.md)

## Il verdetto in una riga

**Il mandato era già stato eseguito, integralmente e fedelmente, 16 minuti prima che io lo committassi in
`docs/` dichiarando il contrario.** Nulla è stato creato oggi: la STOP condition che il kit stesso impone al
§1 — *«If an equivalent issue already exists, do not create a duplicate»* — è scattata su **tutte e cinque**
le voci della gerarchia che chiedeva.

## Il controllo dell'orologio, fatto per primo

| Reperto | Ora (locale) |
|---|---|
| Epic `#2453` creata | 2026-09-05 **15:39:28** |
| Leaf `#2454`–`#2457` create | 2026-09-05 **15:41:58 → 15:42:02** |
| Commenti additivi su `#1990` e `#286` | 2026-09-05 **15:43:55 / 15:43:56** |
| Il kit entra in `docs/roadmap/plans/` (`6f6a9997`) | 2026-09-05 **15:58:16** |

Cinque delle nove issue che il kit ordina di ispezionare (`#2245`, `#2274`, `#2288`, `#2336`, `#2272`) erano
**già `CLOSED` il 2026-09-04**, il giorno prima. Non è un difetto del kit — chiede esplicitamente di
verificarle contro `main` — ma è la misura di quanto il repository si muova sotto un handoff.

## Copertura: cosa il mandato chiede, cosa esiste

| § | Richiesta | Stato | Reperto |
|---|---|---|---|
| §4 | Una Epic cross-release, **senza** milestone di release | ✅ REUSED | `#2453`, `enhancement`+`epic`+`P2`, milestone **nessuna** |
| §4 | L'Epic linka `#1990` `#286` `#288` `#2288` `#2245` | ✅ | tutti e cinque presenti nel corpo |
| §7 | Esattamente **4** leaf v0.1 | ✅ REUSED | `#2454` cue · `#2455` danno/HP · `#2456` status · `#2457` diagnostica |
| §12 | Solo label esistenti; milestone `v0.1 · Leggibilità` sui leaf | ✅ | tutti e quattro sulla milestone; `#2457` a `P3` |
| §13 | Relazione leggibile nei **due** versi | ✅ | checklist dei 4 leaf nell'Epic (righe 132-135) + `#2453` citata in ognuno |
| §14 | Commenti **additivi** su `#1990` e `#286` | ✅ | 15:43:55Z e 15:43:56Z, nessun corpo riscritto |
| ISSUE A | `AttackFootprint` consuma le celle congelate; Stable ID `0` = NONE | ✅ | in `#2454` |
| ISSUE B | `Amount == 0` **deciso e documentato**, non lasciato accidentale | ✅ | in `#2455` |
| ISSUE C | Riusa `IsStatusBirth`; set finale coerente con `BuildStatusBadges` | ✅ | in `#2456` |
| ISSUE D | Nessun contatore di playback locale; solo coordinata canonica | ✅ | `MicroStep` citato in `#2457` |
| §9 | Nessun requisito Niagara in v0.1; mai il solo colore come canale | ✅ | in `#2453` `#2454` `#2455` `#2456` |
| §15 | Nessuna issue speculativa, nessun duplicato di HP/status/animazione | ✅ | 5 voci in totale, nessuna oltre il perimetro |

**Zero gap reali.** Non c'è stato niente da creare, e per §15 crearlo sarebbe stato issue spam.

## 🔴 Falsificato — e la premessa falsa è mia, non del kit

Il commit `6f6a9997` e il corpo della PR **#2464** affermano:

> *«al 2026-09-05 nessuna issue GrayKit / Combat Feedback esiste, né aperta né chiusa»*

**È falso.** Alle 15:58, quando l'ho scritto, ne esistevano cinque da sedici minuti.

Il meccanismo, misurato ripetendo la query: **`gh issue list --search` interroga l'indice di ricerca di
GitHub, che è asincrono**. La stessa query che oggi risponde `5` rispondeva `[]` allora, e l'altra misura
che avevo — `gh issue list --limit 8`, che usa REST e non ha lag — era stata eseguita **prima** delle
15:39, quindi era vera quando l'ho letta. Due misure entrambe corrette al proprio istante, una conclusione
falsa al momento della scrittura.

∴ **Un `--search` che risponde vuoto su issue appena create non è una prova di assenza.** Il controllo che
non ha lag è `gh issue list` senza `--search` (REST), oppure `gh issue view <n>` su un numero atteso.

La riga d'indice in archivio porta questa correzione, così chi ritrova il kit non lo riesegue.

## ✅ Sopravvive

L'architettura che il kit dichiara — presentazione che **consuma** `FRTResolvedEvent` e non ricalcola mai
targeting, LOS, danno o pathfinding; `one event → one signal` in v0.1 senza aggregazione; scala di maturità
v0.2→v1.0 documentata come gate invece che come issue aperte — è quella che le cinque issue implementano, ed
è coerente con i guardrail di `CLAUDE.md`.

## ⛔ Non applicato

Nulla, per assenza di gap. Un dato **più fresco del kit** vive già nella checklist dell'Epic: la metà
`HazardDamage` di `#2455` è dichiarata bloccata da `#2460`, dipendenza che il mandato non conosceva.

## Rischi

- Il kit resta un documento **non autorevole** in archivio: descrive una gerarchia da creare che oggi
  esiste. Chi lo legge senza la riga d'indice può rieseguirlo. È esattamente il difetto che questo consumo
  chiude.
- ✅ **Chiuso il 2026-09-05, poche ore dopo questo referto**: `#2457` era `P3` mentre gli altri tre leaf
  erano `P2`, ed è stata **promossa a `P2`** — i quattro leaf sono ora allineati. Il rilievo era che la
  diagnostica non è un lavoro parallelo agli altri tre ma lo **strumento con cui si diagnosticano**:
  distingue evento gameplay assente, evento presente con consumer mai chiamato, cue sbagliata e timing/seek
  di playback sbagliato. A `P3` sarebbe arrivata **dopo** i suoi consumatori. Non era una deviazione dal
  mandato, che fissa `P2` per la sola Epic e tace sui leaf: era una scelta implicita, ora esplicita e
  motivata in un commento su `#2457`. Scope, milestone e dipendenze invariati.
