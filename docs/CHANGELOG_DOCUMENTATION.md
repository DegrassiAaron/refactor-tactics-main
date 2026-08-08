# Changelog della documentazione

---

## 2026-08-08 — Consolidamento dei documenti **non**-Gameplay (secondo passaggio)

**Origine**: audit `src/RefactorTactics_Audit_Docs_NonGameplay_Consolidamento_Claude_2026-08-08_v2.md`.
73 file in scope (tutto `docs/` **escluse** `gameplay/`, `src/`, `archive/`). Baseline: `HEAD 3335e36`,
**419 test unici in 64 file**.

Il primo passaggio aveva sistemato la **struttura**; questo trova il problema opposto — documenti nella
struttura giusta che contengono ancora stato storico, snapshot intermedi e correzioni stratificate.

### Decisioni registrate

| | |
|---|---|
| **D-020** | Un'azione con bersaglio **orienta l'unità prima di risolvere**; timeline del facing a sei punti. Emenda [ADR-0005](decisions/adr-0005-orientamento.md) §1–§2 |
| **D-021** | Una Decision Window privata **non è deducibile nemmeno dal tempo**: la sospensione logica resta globale, la presentazione avversaria no. Estende l'invariante #6 |
| **D-022** | **UE 5.8.1** consolidata; chiude `D-007` |
| **D-023** | Il workbook `.xlsx` è **`RESEARCH`**; i cataloghi `.md` sono i dati canonici |
| **D-024** | La quota **non aumenta il danno**; completa D-018 sul lato danno |
| **D-025** | **Sette** azioni generiche: `Guard` torna universale, `Activate` resta assorbita da `Interact`. Emenda D-014 |

### Il paradosso di governance

`README.md` dichiarava «il canone prevale su tutto», ma ADR-0004 e ADR-0005 *correggevano* il canone. Ora la
regola è esplicita: **un ADR accettato si recepisce nel canone nello stesso commit**, e un ADR accettato ma non
recepito è un difetto registrabile. Aggiunta la tassonomia dei tag — `CANONICAL`, `CURRENT`, `AS-BUILT`,
`DELIVERED PLAN`, `HISTORICAL`, `RESEARCH`, `OPEN` — con la regola che uno storico «sbagliato» non si corregge:
il difetto è lo storico *senza etichetta*.

### Contraddizioni interne trovate, non ereditate

- **ADR-0004** diceva due cose incompatibili: la §5 sospende la simulazione «così chi guarda vede il mondo
  fermarsi», la §7 che l'avversario non riceve nulla. Una pausa globale osservabile **è** il dato. → §7-bis.
- **`roadmap-v0.1.md`** conteneva due viste dello stesso stato, ognuna correttiva dell'altra: nella stessa
  pagina E8 risultava «da costruire» **e** «chiusa», con due «stato in una riga» diversi. → una sola tabella.
- **`test-manuali-pie.md`** dichiarava 34 voci aperte in testa e «le 31 aperte, somma verificata
  `2+9+9+4+4+1+2 = 31`» venti righe sotto. La somma tornava **con sé stessa**.
- **D-014 vs l'handoff** si contraddicevano sulle azioni generiche, chiusi lo stesso giorno: unico `CONFLICT`
  vero del passaggio, risolto dall'autore → D-025.

### Documenti che non descrivevano più il progetto

`test-automatico-unreal.md` era ancora il **prompt di implementazione** dell'harness (1114 righe di «TASK —
progettare e implementare»), e proponeva un `ARTTestDirector` che non è mai stato costruito → prompt in `src/`,
spec as-built al suo posto. `debug-vs-unreal.md` era una guida **M1 quadrata**. `spec-pathfinding-pf3-pf4.md` e
`spec-mappa-multilivello.md`, indicate come owner correnti, descrivevano `FRTGridCoord` a 4 vicini → riscritte
dal codice, corpi originali in `archive/technical/`. `architettura-codice.md` elencava 25 classi su 40 e dava
per «north-star da costruire» PF.4 e le reazioni, entrambe fatte.

### Divergenze documento↔codice registrate, non nascoste

Tre restano aperte come issue, perché la decisione è presa e la migrazione no:
`Action.Sprint` è in `FastMovement` e consuma due slot mentre D-015 lo vuole profilo del `Move`;
`OccupantDamageBonus` è generico e ogni call site passa `0`, ma il test si chiama `...WithTerrainBonus`;
`Overwatch` è deciso da D-012 e compariva **zero volte** in `balance/`.

### Riclassificazioni

20 piani in `roadmap/plans/` con header uniforme (corpi invariati) · `roadmap-editor.md` **ritirata** (terza
vista di stato da allineare a mano) · `hex-map-roadmap.md` congelata · `v0.1-issue-plan.md` a snapshot ·
`use-case-list.md` e l'handoff in `archive/session-notes/` · `plan-turnlog.md` e
`brief-consolidamento-documentale.md` in `roadmap/plans/`.

### Verifiche

`python scripts/check-docs-symbols.py` ✅ · **916 link relativi, 0 rotti** · test count rimisurato ·
ricerca dei termini obsoleti sui soli documenti `CURRENT`/`CANONICAL`.

---

## 2026-08-08 — Consolidamento di `docs/gameplay/`

**Origine**: audit `src/RefactorTactics_DocsGameplay_Audit_Consolidamento_Claude_2026-08-08.md`, con
`D-014`…`D-019` approvate. 23 file passati in rassegna.

### Decisioni registrate

| | |
|---|---|
| **D-014** | Generiche canoniche `Wait · BasicAttack · Interact · Brace · Move · Overwatch`; `Activate`→`Interact`; `Guard` non più universale |
| **D-015** | `Sneak · Normal · Sprint` sono **profili di `Move`**; `Sprint` **non è un Dash** |
| **D-016** | **Un** thin slice di Predictive Action nella v0.1 (`Vektor.InterceptShot`) |
| **D-017** | `Intercept` **rivalida la geometria sul bersaglio effettivo** |
| **D-018** | `HighGround` **senza bonus numerico** alla vista in v0.1 |
| **D-019** | `Fast Action` ≠ `Fast Reaction` — categorie distinte sulla stessa `DecisionWindow` |

### `spec-sequenza-turno.md` riscritta come **spec canonica del round**

Diceva due cose opposte: §2 che le finestre live erano in scope (C1 chiuso da ADR-0004), §4 e §5 di «non
implementare finestre live nell'MVP» perché «serve il multiplayer». La seconda era in forma di **divieto**, la
più facile da prendere per buona. Il gate a due condizioni è caduto il 2026-08-07 — la ragione di gameplay
esisteva (bait/bluff non recuperabili con condizioni dichiarate) e la riconciliazione **(b)**, già scritta lì
come ipotesi, si è rivelata sufficiente.

Ora il documento è normativo e corto: sequenza, tassonomia temporale, ordine deterministico, confine
corrente/north-star. La storia è compressa in fondo.

### Il difetto trovato guardando il codice

**APNAP a sei gruppi non esiste.** Il canone §5.1 lo dichiara vincolante come `FR-RESOLVE-01`, ma
`grep -rn "APNAP\|FR-RESOLVE" Source/` trova **un solo commento**. L'ordine reale è quello della coda azioni —
`MacroPhase → Priority → ActionId → SourceUnitId → EventSequence` — che assorbe la parte *intra-gruppo* di
APNAP ma non la partizione per appartenenza.

Non è urgente: il canone dichiara l'implementazione *gated*. Ma è la solita forma — **una regola normativa che
nessun consumer legge** — e ora è scritta, non da dedurre. Registrata come riga `OPEN` nella matrice.

### Altre correzioni di fatto

| | |
|---|---|
| `spec-anima-risoluzione.md` | Assumeva «lock-in calcola tutto una volta sola». Con ADR-0004 al lock-in **il futuro del round non è ancora scritto**: riscritta a segmenti, con la durata del round che smette di essere calcolabile in anticipo |
| `spec-dash.md` | Elencava «dash = pathfinding (aggira ostacoli)» fra le **decisioni prese**, smentito quattro righe sotto. Rimosso dall'elenco, non barrato |
| `spec-propagazione-elettrica-cp83.md` | Dichiarava `Action.Electrify` **assente dal catalogo core**. Oggi **esiste** (`RTCatalogLibrary.cpp`); nessun eroe la usa |
| `brief-conoscenza-parziale.md` | §10.1 dichiarava **chiusa** la vista da quota con `Sight_Mod = +1/+2/−1`. **D-018 la chiude nel verso opposto**: il numero veniva dal workbook, non da un playtest |
| `brief-conoscenza-parziale.md` | Il veto `D15` sulle finestre acustiche poggiava su `D7`, caduto. Sostituito da un permesso **condizionato**, non da un obbligo |
| `spec-motore-azioni-e4.md` | Diceva ancora «proposta da approvare» a epic chiusa. Ora **AS-BUILT**, con la sezione delle decisioni che l'hanno superata |

### Collisione di ID risolta

I brief usano ID **locali** `D1`…`D22`, il Decision Log usa `D-001`…`D-019`. Le sigle si sovrappongono: il
`D14` di `brief-conoscenza-parziale` è la propagazione a flood fill, il `D-014` globale sono le azioni
generiche. Regola scritta in testa a entrambi i brief: **il trattino distingue** — `D-0xx` globale e
vincolante, `Dxx` locale.

### Archiviati

`spec-bot-utility.md` · `spec-knockback.md` · `spec-terreni.md` · `sequenza-turno.md` → `archive/gameplay/`.
L'ultimo rinominato `sequenza-turno-exploratory.md`: due file quasi omonimi in cartelle diverse erano una
trappola di lettura.

### Cosa NON è stato fatto, deliberatamente

Nessuna migrazione di Stable ID. `Action.Guard`, `Action.Activate` e `Action.Sprint` **esistono e sono
consumati** — `Action.Sprint` in 9 file di codice e 6 di test. La tassonomia di D-014/D-015 è **semantica di
gameplay**, non un rename già avvenuto: farlo in una PR documentale romperebbe test e replay. Tracciato come
issue.

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
registrata in [`brief-consolidamento-documentale.md`](roadmap/plans/brief-consolidamento-documentale.md).
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
