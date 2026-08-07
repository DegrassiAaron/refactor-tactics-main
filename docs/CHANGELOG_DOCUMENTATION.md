# Changelog della documentazione

---

## 2026-08-07 (2) — Chiusura delle cinque decisioni aperte

**Origine**: sessione `/sc:brainstorm`. Chiude `OD-1`…`OD-5` aperte poche ore prima dalla revisione documentale.
La matrice dei conflitti passa a **0 `OPEN`, 0 `CONFLICT`**.

| Decisione | Esito |
|---|---|
| **D-011** | Formato principale **non deciso**: `D-001` declassata da *Consolidata* ad *Assunzione da bloccare*. Il 3v3 resta baseline, il 4v4 è **solo** stress test |
| **D-012** | L'Overwatch è **universale** e **compete** con l'azione offensiva. I tre regimi emergono dai dati, **non** da un enum di policy |
| **D-013** | Un trigger su transizione è **possesso della trap**, non della mappa. `FRTHexEdge` resta per i soli salti di layer |
| **E17** | Nuova epic: validazione di stress 4v4 (3 CP), dopo E15. Totale **17 epic, 85 checkpoint** |
| Nuovi brief | [`gameplay/brief-unita-ausiliarie.md`](gameplay/brief-unita-ausiliarie.md) · [`gameplay/brief-azioni-generiche-overwatch.md`](gameplay/brief-azioni-generiche-overwatch.md) |

### Due domande erano mal poste — e lo ha detto il codice, non i documenti

- **`OD-4`** chiedeva «gli archi del grafo portano trigger?», ed era etichettata come gate di **E9**. Ma gli
  archi degli adiacenti **non esistono**: `URTHexPathLibrary::GraphNeighbors` li calcola da
  `URTHexLibrary::Neighbors`, e solo le transizioni fra layer sono `FRTHexEdge`. La domanda vera — *dove vive
  la coppia `(From→To)`* — ha una risposta che non tocca la mappa, non ne versiona il formato e **non ha
  scadenze**.
- **`OD-1`** era etichettata **bloccante**. La dimensione della squadra è un `TArray<FName>` su `ARTGameMode`,
  non un campo di `FRTMatchRules`: in v0.1 non si costruisce né un 3v3 né un 4v4. Bloccava la *documentazione*.

Conseguenza di metodo: l'urgenza era stata dedotta **dai documenti** e l'ordine di priorità ne era uscito
sbagliato. L'unica decisione che bloccava lavoro costruibile era `OD-3`.

### Una contraddizione evitata

Il brief sulle azioni generiche, come scritto la prima volta, introduceva
`enum ERTOverwatchResolutionPolicy { Automatic, Conditional, FastSelect }` copiandolo dal documento sorgente.
[`roadmap/roadmap-v0.1.md`](roadmap/roadmap-v0.1.md) §E14 rischio (b) aveva **già deciso il contrario**: un enum
di policy affiancato ad `AllowedResponses` sarebbe una seconda verità sullo stesso comportamento.
Riformulato: i tre regimi **emergono** da `AllowedResponses` più una **condizione dichiarata in planning**, che
è l'unica aggiunta reale rispetto ad ADR-0004.

### Deriva corretta nel passaggio

`brief-overwatch-reazioni.md` conteneva una tabella di checkpoint **duplicata** e divergente da quella della
roadmap: 5 checkpoint contro 6, perché la rinumerazione che ha inserito `CP 14.2` non era mai tornata sul brief.
Le issue `#161`–`#166` seguono la roadmap. La tabella è stata **rimossa** dal brief, che ora possiede le
decisioni e non il piano.

---

> **Cosa registra**: le modifiche **strutturali** alla documentazione — spostamenti, nuovi owner, correzioni di
> fatti sbagliati. **Cosa non registra**: le revisioni di contenuto di una singola regola, che vivono accanto
> alla regola stessa come blocco `⚠️ Revisione <data>` nel documento che la possiede. Il changelog di una
> decisione sta dove sta la decisione; qui sta solo la storia dell'impalcatura.

---

## 2026-08-07 — Riorganizzazione in `product/gameplay/technical/balance/roadmap/decisions/`

**Origine**: revisione `/sc:spec-panel` su `src/RefactorTactics_Consolidamento_PRD_SourceOfTruth_Claude.md`,
registrata in [`brief-consolidamento-documentale.md`](brief-consolidamento-documentale.md).
**HEAD di partenza**: `50159c6`.

### Struttura

**101 file spostati.** `docs/design/` (72 file) si è dissolta nelle cartelle per dominio; `docs/HUD/`,
`docs/guides/`, `docs/Data/` e `docs/PDR/` sono state assorbite.

| Da | A | Criterio |
|---|---|---|
| `design/piano-canonico-mvp.md`, `showcase-v0.1.md` | `product/` | visione e canone |
| `design/spec-*` di regole di gioco, `brief-*` di gameplay | `gameplay/` | cosa fa il gioco |
| `design/spec-*` di implementazione, `HUD/`, `guides/`, `Data/use-case-list.md` | `technical/` | come è costruito |
| `design/balance/`, `Data/*.xlsx` | `balance/` | i numeri, in un posto solo |
| `design/roadmap-*`, `v0.1-*`, `PDR/RT_PDR_10_*.md` | `roadmap/` | pianificazione ed esecuzione |
| `design/h5*`, `cp6-*`, `plan-*`, `pacing-turno-plan`, `handoff-*` | `roadmap/plans/` | piani consegnati, storico |
| `design/adr-000*`, `PDR/RT_PDR_00_Decision_Log.md` | `decisions/` | decisioni con motivazione |
| `PDR/*.pdf` | `archive/pdr-v0.1/` | snapshot di consultazione |
| `HUD/*.png` | `technical/img/` | riferimenti visuali |
| `guides/*.docx` | `src/` | sorgente orfano, non normativo |

**Link riscritti in modo programmatico**, non a mano: 474 link relativi verificati, **0 rotti**. Aggiornati
anche i 3 file di radice (`CLAUDE.md`, `AGENTS.md`, `README.md`) e i **13 file C++** che citano un percorso di
`docs/` in un commento.

**Due deviazioni dichiarate** rispetto alla struttura di riferimento: i nomi dei file restano in italiano
kebab-case invece di `UPPER_SNAKE` inglese (convenzione consolidata del repository), e `src/` sopravvive come
casella dei sorgenti grezzi, esplicitamente non normativa.

### Nuovi documenti

| File | Ruolo |
|---|---|
| [`README.md`](README.md) | **Punto d'ingresso**: gerarchia delle fonti, tabella *concetto → owner*, le risposte brevi |
| [`DOC_CONFLICT_MATRIX.md`](DOC_CONFLICT_MATRIX.md) | 26 conflitti con stato e fonte che prevale |
| [`OPEN_DECISIONS.md`](OPEN_DECISIONS.md) | `OD-1`…`OD-5`: cosa aspetta una persona |
| *questo file* | storia strutturale della documentazione |

### Fatti sbagliati corretti

Otto difetti **misurati**, non ipotizzati. Il pattern è uno solo: **i documenti normativi non vengono riletti
dopo un refactor**, e restano leggibili e falsi.

| # | Difetto | Correzione |
|---|---|---|
| D1 | `piano-canonico-mvp.md` §5 elencava **4 classi su 10 inesistenti** (`ARTGameState`, `URTTurnResolver`, `URTGridLibrary`, `URTAbilityData`) e l'invariante #2 citava `FRTGridCoord`, rimosso al CP 6.1 | tabella riallineata al codice con comando di verifica riproducibile; invariante #2 corretto a `FRTCellId`. Estesa a `CLAUDE.md`, `AGENTS.md` e alle due righe stantie di `technical/architettura-codice.md` |
| D2 | `docs/README.md` non esisteva | creato |
| D3 | `CLAUDE.md`, `AGENTS.md` e `archive/README.md` linkavano `docs/SuperClaude_...md`; il file è in `docs/src/` | link corretti |
| D4 | `brief-delayed-actions.md` affermava che `RTReactionLibrary` **non esiste**. Esiste: epic E5, 27 test, dichiarata ✅ in `roadmap-v0.1.md` lo stesso giorno | rettifica in linea, osservazione barrata |
| D5 | `brief-conoscenza-parziale.md` avvertiva che il workbook di bilanciamento non era versionato. Lo era | avviso superato; il workbook è ora in `balance/`, accanto ai cataloghi |
| D6 | 3 sorgenti in `src/` erano **untracked**, incluse le due che chiedevano il consolidamento | versionati |
| D7 | 8 documenti descrivevano il substrato **quadrato** senza dirlo | banner in testa, **distinti per natura**: ⚠️ *Superato* (4), 📦 *Piano consegnato* (1), ℹ️ *Regola vigente, esempi datati* (3). Non tutti erano superati: appiattirli sarebbe stato un errore opposto |
| D8 | `progettazione-hud.md` referenziava un PNG inesistente | corretto su `technical/img/UI-style-guide.png` |

### Conteggio dei test riallineato

Le due viste di roadmap dichiaravano **366 test in 55 file**; la misura al commit `50159c6` dà
**390 in 61 file** (+24, dal primo blocco dell'harness degli scenari) — e **397 in 62** dopo il merge con `main`,
dove sono atterrati i 7 test di CP 9.1. È la terza volta che questo scarto si
apre — 2026-08-05 (−3), 2026-08-07 (−145), ora (−24) — e ogni volta perché il numero è stato *citato* invece
che *misurato*. Il comando resta quello dichiarato nei due documenti:

```bash
grep -rhoE '"RefactorTactics\.[A-Za-z0-9_.]+"' Source/RefactorTactics/Tests/*.cpp | tr -d '"' | sort -u | wc -l
```

### Gate anti-deriva

Nuovo: [`scripts/check-docs-symbols.py`](../scripts/check-docs-symbols.py). Fallisce se un inventario di classi
in un documento normativo cita un simbolo non **dichiarato** in `Source/`. È la sola parte della «Fase G» del
documento sorgente che si possa automatizzare in modo affidabile: i link rotti si vedono, un simbolo inesistente
no.

Due decisioni prese costruendolo, entrambe emerse da un **test di mutazione** (reintrodurre D1 e verificare che
cada):

1. **Conta le dichiarazioni, non le occorrenze.** La prima versione considerava esistente qualunque simbolo
   apparisse in `Source/` — e lasciava passare `URTGridLibrary`, viva solo dentro un commento che ne spiegava
   la rimozione. Il gate si autoassolveva proprio sui simboli che gli interessano di più. I simboli noti sono
   passati da 542 (occorrenze) a **141** (dichiarazioni).
2. **Controlla solo gli inventari di classi**, non la prosa. Una prima versione più larga produceva 37
   segnalazioni, quasi tutte legittime: simboli *futuri* nei DoD dei checkpoint, modelli north-star dei PDF,
   frasi che documentano correttamente una rimozione. Tarare le espressioni regolari finché il gate diventava
   verde sarebbe stato l'antipattern peggiore — un gate verde perché ha smesso di guardare. Meglio stretto e
   affidabile che largo e ignorato.

Verifica di mutazione registrata: reintrodotti uno alla volta i quattro simboli di D1 nella tabella del canone
→ **4 su 4 rilevati**; ripristino → verde.

### Cosa **non** è stato fatto, deliberatamente

- **Nessuna epic nuova**: il rischio di scope della v0.1 è già `H/H` con 82 checkpoint.
- **Nessuna decisione al posto di una persona**: i 4 conflitti irrisolti sono in `OPEN_DECISIONS.md`, non
  chiusi per plausibilità.
- **Nessun brief per le tre aree scoperte** (unità ausiliarie, azioni generiche/Overwatch universale, trap
  persistenti): sono in `OPEN_DECISIONS.md` come `OD-2`, `OD-3`, `OD-4`.
