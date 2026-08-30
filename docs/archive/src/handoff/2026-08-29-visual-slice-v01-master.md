# RefactorTactics — Visual Slice v0.1 (pacchetto Claude Code)

> `HISTORICAL` · **Kit d'autore consumato**, non una fonte. · **Consumato**: 2026-08-29 · **Base**:
> `20d59973` (`diag/1665-istanze-board`).
>
> Archiviato da [`docs/archive/`](../../README.md): vale per la **provenienza** e il rationale, mai per la
> regola. Il kit stava alla radice del repository come directory untracked
> `RefactorTactics_Claude_v0.1_VisualSlice/` — **20 file, 2163 righe** — accompagnata da uno zip
> byte-identico. I 15 file di task condividevano lo **stesso preambolo di 66 righe**, riprodotto qui una
> volta sola; tutto il resto è verbatim.
>
> **Cosa possiede**: la sequenza a quattordici nodi della Visual Slice, le due issue proposte e il template
> di handoff, verbatim. **Cosa non possiede**: nessuna autorità, e nessuna esecuzione.
> Il referto completo — voce per voce, misura per misura — è
> [`../../../roadmap/plans/visual-slice-v01-spec-panel-2026-08-29.md`](../../../roadmap/plans/visual-slice-v01-spec-panel-2026-08-29.md).
>
> ⛔ **Nessuno dei suoi quattordici task è stato eseguito.** Il mandato della sessione era *consumare e
> rimuovere*. Nessun asset e nessun sorgente toccati; nessuna issue chiusa o commentata.
>
> ➕ **Una sola azione applicata, su richiesta esplicita dell'autore**: `NEW-02` è stata aperta come
> [`#1712`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1712), **non** copiando il corpo
> del kit ma riscrivendolo sulle misure — vedi §15 del referto. E `NEW-01` come
> [`#1714`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1714), **ridotta al solo
> materiale**: delle sue cinque voci quattro non vanno fatte, e §16 del referto misura perché.
>
> 🔴 **È la terza fotografia della stessa famiglia, e ripete l'errore delle prime due.** I fratelli sono
> [`2026-08-28-graykit-v01-roadmap-consolidated.md`](2026-08-28-graykit-v01-roadmap-consolidated.md) e
> [`2026-08-28-graykit-v01-apply-mandate.md`](2026-08-28-graykit-v01-apply-mandate.md), consumati il giorno
> prima: già lì il rilievo `C2` era *«due dei sei nodi della roadmap sono issue chiuse»*. Qui i primi
> **tre** nodi su quattordici — e la sequenza è dichiarata **vincolante** — lavorano su `#1094`, chiusa il
> 2026-08-19, e il quinto su `#956`, chiusa il 2026-08-23.
>
> ✅ **Ciò che il kit ha di suo**: `NEW-02` (LOS debug overlay) non ha un equivalente nel repository né
> fra le issue aperte, e il nodo 14 è l'unico gate visivo end-to-end scritto in qualche posto.
>
> ➕ **Una quarta fotografia è arrivata il 2026-08-30, e non è archiviata qui accanto: è questa.**
> `RefactorTactics_Claude_v0.1_VisualSlice_ALL_IN_ONE.md` — 1284 righe, un file solo, consegnato in
> `refactor-tactics-technical-designer/` alle `2026-08-29T22:16:40Z`, cioè **cinque minuti prima** che
> [PR #1717](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1717) portasse in `main` il referto
> che consuma questo sorgente. Normalizzando i due testi: **743 righe uniche su 768 sono verbatim qui**, e le
> 71 che mancano sono questo banner e i separatori `<!-- NN_*.md -->`. Contenuto tecnico aggiunto: **zero** —
> le 25 righe nuove sono un cappello d'uso e un prompt di lancio, riprodotte **verbatim e per intero** in
> [§18.2 del referto](../../../roadmap/plans/visual-slice-v01-spec-panel-2026-08-29.md).
>
> ⛔ **Non è stata archiviata a parte per la ragione che il referto ha già scritto** chiudendo lo zip
> byte-identico (§13): due copie dello stesso kit, una con un verdetto e una senza, sono **due source of
> truth** — e queste due nemmeno coinciderebbero, divergendo per venticinque righe. Nulla è andato perso: il
> 96,7 % era già qui dal 29, il resto è in §18.2.

---

# RefactorTactics — Claude Code Task

> Pacchetto Visual Slice v0.1 — generato 2026-08-29.
>
> Repository: `DegrassiAaron/refactor-tactics-main`
>
> Baseline progetto: Unreal Engine 5.8.1 · v0.1 2v2 offline vs bot · hex multilivello · no GAS.

## Regole operative obbligatorie

Prima di modificare codice o contenuti:

1. Leggi `AGENTS.md`.
2. Leggi `CLAUDE.md`.
3. Verifica `git status`, branch e `git rev-parse HEAD`.
4. Leggi l'issue indicata in questo task e i suoi commenti più recenti.
5. Cerca nel repository prima di creare classi/helper/asset nuovi.
6. Quando pertinente, consulta:
   - `docs/product/piano-canonico-mvp.md`
   - `docs/decisions/RT_PDR_00_Decision_Log.md`
   - `docs/DOC_CONFLICT_MATRIX.md`
   - `docs/OPEN_DECISIONS.md`
   - `docs/roadmap/roadmap-checkpoint.md`
   - `docs/roadmap/roadmap-v0.1.md`
7. Non usare PDF come source of truth.
8. Non introdurre GAS nella v0.1.
9. La simulazione decide; la presentazione mostra.
10. LOS, targeting e traiettoria restano servizi distinti.
11. Non creare un secondo substrato di coordinate: usare `FRTCellId`.
12. Non fare refactor laterali “già che ci siamo”.
13. Non toccare `.uasset`/`.umap` a mano dal filesystem.
14. Se un task richiede una modifica binaria Unreal e non puoi farla in sicurezza tramite Editor, prepara il codice/supporto necessario e fermati con una checklist precisa per l'operazione umana.
15. Non dichiarare “completo” senza evidenza di build/test/PIE richiesta.

### Test

Usa il percorso ufficiale del repository:

```powershell
./scripts/rt-suite.ps1
```

Quando possibile usa un filtro ristretto durante lo sviluppo, poi esegui la suite prevista dalla issue prima della chiusura.

Per la build Editor, con Unreal Editor chiuso:

```powershell
& "<engine>/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development `
    -project="<repo>/RefactorTactics.uproject" -waitmutex
```

### Formato dell'handoff finale

Rispondi sempre con:

- **Risultato**
- **File modificati**
- **Decisioni prese / decisioni NON prese**
- **Build eseguita**
- **Test eseguiti e risultati**
- **Verifiche PIE/manuali ancora necessarie**
- **Rischi o limiti rimasti**
- **Commit suggerito**
- **Prossimo task della sequenza**



---

# Il corpo dei task, verbatim


---

<!-- 00_MASTER_VISUAL_SLICE.md -->

# Visual Slice v0.1 — Master execution plan per Claude Code

## Obiettivo

Portare il progetto dal suo stato visivo attuale a una **vertical slice leggibile a schermo** nel minor tempo possibile:

`esagoni + unità`  
→ `graybox arena`  
→ `materiali/superfici leggibili`  
→ `LOS osservabile`  
→ `conoscenza parziale/Last Contact`  
→ `Knowledge Veil visibile`  
→ `HUD UMG`  
→ `icone`  
→ `packaged visual gate`.

Non espandere la v0.1 con GAS, multiplayer, rumore avanzato, stealth avanzato, ambiente completo o final art.

## Sequenza vincolante

Esegui i file in quest'ordine:

1. `01_PRECHECK_GBX_1094.md`
2. `02_GRAYBOX_VALIDATION_1095.md`
3. `03_CLOSE_GBX_1094.md`
4. `04_NEW01_GRAYBOX_KIT_MATERIALS.md`
5. `05_BOARD_VISUAL_GRAMMAR_956.md`
6. `06_TACTICAL_READABILITY_289.md`
7. `07_NEW02_LOS_DEBUG_OVERLAY.md`
8. `08_PARTIAL_KNOWLEDGE_1466.md`
9. `09_MOVE_TRACE_PRIVACY_1497.md`
10. `10_KNOWLEDGE_VEIL_1535.md`
11. `11_HUD_CONTRACT_77.md`
12. `12_HUD_UMG_613.md`
13. `13_ICON_CATALOG_220.md`
14. `14_VISUAL_PACKAGE_GATE.md`

Le issue nuove sono descritte anche in:

- `ISSUE_NEW01_GRAYBOX_KIT.md`
- `ISSUE_NEW02_LOS_DEBUG.md`

## Regola importante sulla sequenza

`#1094 → #1095 → #1094` è volutamente un loop.

Non cercare di chiudere tutte le decisioni graybox prima della scena di validazione: alcune vanno **misurate guardando** la scena U25.

## Definition of Done della Visual Slice

La slice è pronta quando:

- `L_DevSandbox` mostra una piccola arena graybox leggibile;
- low/high cover, muri, porte e superfici sono distinguibili;
- la camera tattica legge la board alla scala corrente;
- la LOS esistente è visualizzabile con debug overlay;
- un nemico ignoto non espone nome/HP/modello/path reali;
- Last Contact è coerente con Team Knowledge;
- il Knowledge Veil è applicato dalla prima inquadratura e ai refresh previsti;
- il viewer del velo non è hardcoded;
- il Canvas HUD soddisfa il contratto informativo;
- il layer Screen HUD esiste in UMG mantenendo il centro mappa libero;
- i widget consumano il catalogo icone;
- test automatici richiesti sono verdi;
- le verifiche PIE previste sono eseguite;
- una packaged Development build non presenta regressioni visuali bloccanti.

## Cosa NON fare durante questa catena

- niente GAS;
- niente refactor del resolver se non richiesto dall'issue;
- niente sistema FoW parallelo: usare TeamKnowledge/Knowledge Veil esistenti;
- niente nuova LOS se quella esistente basta;
- niente target logic dentro il renderer LOS;
- niente mega-widget che assorbe i Tactical World Overlay;
- niente texture hardcoded nei WBP se il catalogo le possiede;
- niente final art;
- niente catalogo graybox completo: il nuovo kit è solo il minimo per la visual slice.

## Modalità consigliata

Per ogni file:

1. fai il preflight;
2. misura lo stato corrente;
3. se l'issue è parzialmente già implementata, riduci il delta invece di rifare il lavoro;
4. implementa un cambiamento piccolo;
5. build;
6. test mirati;
7. test di regressione;
8. verifica PIE/manuale se applicabile;
9. commit singolo coerente;
10. passa al file successivo.

Se trovi una decisione aperta che blocca il task, **non inventarla**: documenta lo STOP e indica esattamente quale decisione serve.

---

<!-- 01_PRECHECK_GBX_1094.md -->

## Task 01 — Precheck decisioni Graybox

**Issue:** #1094 — “Cinque domande aperte del contratto graybox…”

### Obiettivo

Sbloccare ciò che serve **prima** di creare il primo asset del Graybox Kit, senza tentare di chiudere prematuramente le decisioni che richiedono la scena U25.

### Stato noto da verificare sul branch corrente

L'issue contiene decisioni `GBX-*` su:

- Safe Placement inset;
- distinzione visiva porte Closed/Locked;
- soglie di integrità;
- percorso `Content/` del Graybox Kit;
- ingombro unità rispetto alla cella.

La scala esagonale è già stata decisa da D-163 e va verificato che l'atterraggio corrispondente sia presente nel codice corrente.

### Scope di questo task

Chiudere o rendere eseguibili **solo le decisioni che bloccano la creazione degli asset**.

Priorità:

1. verifica stato `GBX-4` — percorso Content e allowlist;
2. verifica stato `GBX-2` — grammatica Closed vs Locked prima di modellare porte;
3. verifica che la scala corrente sia quella canonica;
4. prepara `GBX-1` e `GBX-5` per essere misurate in U25, senza inventare valori.

### Passi

1. Leggi tutta #1094 e i commenti.
2. Cerca `GBX-1`…`GBX-6` in:
   - `docs/OPEN_DECISIONS.md`
   - Decision Log
   - spec graybox placement contract
   - roadmap.
3. Verifica `.gitignore` e convenzioni contenuti.
4. Verifica la scala esagonale nel codice.
5. Se `GBX-4` è ancora realmente aperta e impedisce l'asset:
   - NON creare asset;
   - prepara la modifica documentale minima coerente con le decisioni correnti oppure segnala STOP se richiede decisione d'autore.
6. Se `GBX-2` è ancora aperta:
   - non creare la porta;
   - registra quali opzioni sono già ammesse dalla spec;
   - non scegliere in autonomia una soluzione estetica non autorizzata.
7. Non chiudere `GBX-1`/`GBX-5` per ragionamento: vanno misurate nella scena U25.

### Output richiesto

Alla fine deve esistere una risposta netta:

```text
READY FOR U25: YES/NO
Blocker:
- ...
Decisioni da misurare in U25:
- GBX-1
- GBX-5
```

### Test / verifica

Questo task è prevalentemente decisionale/documentale. Esegui comunque i gate documentali applicabili se tocchi docs.

### Stop condition

Se manca una decisione d'autore necessaria a fissare `GBX-2` o `GBX-4`, fermati. Non modellare asset “temporanei” che cristallizzano una decisione implicita.

### Commit suggerito

`docs(graybox): unblock U25 authoring decisions`

---

<!-- 02_GRAYBOX_VALIDATION_1095.md -->

## Task 02 — Scena di validazione Graybox U25

**Issue:** #1095 — “Seduta U25 — il volume di posa della cella, e la scena che dice se il graybox si legge”

### Obiettivo

Creare/chiudere la scena di validazione che permetta di giudicare il Graybox Kit a tre distanze di camera.

Questa issue **non è il catalogo graybox completo**.

### Risultato atteso

In `L_DevSandbox` deve essere possibile confrontare, senza dipendere dall'HUD:

- unità;
- low cover vs high cover;
- muro vs muro sfondato;
- porta Open/Closed/Locked/Destroyed secondo lo scope realmente autorizzato;
- acqua vs ghiaccio;
- intatto vs distrutto;
- CellBound e EdgeBound rispetto al volume di posa.

### Vincoli

- il volume è EditorOnly;
- non entra nell'authority;
- non influenza collisioni o resolver;
- `CellBound` si ancora alla cella senza offset correttivi;
- `EdgeBound` segue `URTHexLibrary::EdgeRotation`;
- nessuna geometria di authoring deve diventare dato competitivo.

### Passi

1. Misura lo stato corrente di #1095: non assumere che sia tutto da fare.
2. Individua la classe/strumento editor più piccolo coerente col repository.
3. Riusa `URTHexLibrary` per geometria e orientamento.
4. Implementa il Cell Placement Volume solo se ancora mancante.
5. Prepara la scena U25 in `L_DevSandbox`.
6. Verifica tre distanze:
   - ravvicinata;
   - gameplay;
   - tattica.
7. Cattura la verifica anche in scala di grigi.
8. Usa la scena per raccogliere i dati necessari a `GBX-1` e `GBX-5`.
9. Non chiudere decisioni estetiche non richieste dall'issue.

### Binary asset rule

Se la scena richiede `.umap`/`.uasset` e la sessione non può modificarli in Unreal Editor in sicurezza:

- implementa solo il supporto C++/Editor necessario;
- consegna istruzioni Editor passo-passo;
- NON generare o manipolare binari da filesystem.

### Test

- build `RefactorTacticsEditor`;
- test editor applicabili;
- verifica PIE/manuale prevista dall'issue;
- conferma che il volume non compaia nel gioco/runtime.

### Evidenza richiesta

Riporta:

- screenshot/checklist alle tre distanze;
- valore osservato per `GBX-1`;
- valore osservato/target per `GBX-5`;
- eventuali categorie non distinguibili.

### Commit suggerito

`feat(editor): add graybox placement validation scene`

---

<!-- 03_CLOSE_GBX_1094.md -->

## Task 03 — Chiudi le decisioni Graybox misurabili

**Issue:** #1094

### Dipendenza

Eseguire **dopo** Task 02 / #1095.

### Obiettivo

Usare l'evidenza U25 per chiudere le decisioni che non andavano inventate prima della scena.

### Focus

- `GBX-1` — Safe Placement inset;
- `GBX-5` — ingombro unità rispetto alla cella;
- eventuali altre `GBX-*` che U25 rende finalmente misurabili.

### Passi

1. Riapri #1094 sullo stato corrente.
2. Usa soltanto misure/evidenze prodotte da U25.
3. Registra le decisioni nel loro owner canonico.
4. Aggiorna `docs/OPEN_DECISIONS.md`.
5. Se richiesto dal processo del repo, aggiungi la voce D-nnn nel Decision Log.
6. Non cambiare gameplay per far “tornare” il graybox.
7. Se il target visuale dell'unità richiede successivo lavoro di presentazione, registra la decisione ma lascia l'implementazione all'owner corretto.

### Definition of Done

- nessuna decisione necessaria al nuovo Graybox Kit resta implicita;
- percorso Content e scala sono non ambigui;
- l'inset è documentato;
- il target di ingombro unità è documentato;
- nessun asset nuovo è stato creato in un percorso non autorizzato.

### Test

Gate documentali applicabili + eventuale test già esistente se viene modificata una costante C++.

### Commit suggerito

`docs(graybox): close U25 measured decisions`

---

<!-- 04_NEW01_GRAYBOX_KIT_MATERIALS.md -->

## Task 04 — NEW-01: Graybox Kit minimo + materiale/polytexture

**Nuova issue:** vedi `ISSUE_NEW01_GRAYBOX_KIT.md`.

### Obiettivo

Fare il salto visivo più importante della v0.1: trasformare `L_DevSandbox` da board di esagoni a **piccola arena tattica graybox**.

### Scope stretto

Creare soltanto il minimo necessario:

- Wall Full;
- Wall Broken/variant se richiesto dalla grammatica già decisa;
- Low Cover;
- High Cover;
- Pillar/Block;
- Door Frame;
- Door panel/stati solo se `GBX-2` è risolta;
- water/ice placeholder se servono alla scena;
- materiale master graybox;
- poche material instance / polytexture placeholder.

Non creare il catalogo completo dei 19 elementi.

### Principio architetturale

Le mesh sono **presentazione**.

Non devono decidere:

- passabilità;
- cover;
- LOS;
- costo movimento;
- integrità;
- stato porta.

Queste proprietà derivano dal modello dati esistente.

### Passi

1. Verifica i percorsi Content autorizzati.
2. Verifica asset esistenti riusabili prima di crearne di nuovi.
3. Definisci un kit modulare con pivot/scale coerenti col placement contract.
4. Crea o specifica `M_GBX_Master` solo se non esiste una soluzione già owner.
5. Parametri minimi:
   - tiling;
   - roughness;
   - normal strength se necessario;
   - tint/variant solo per authoring/presentazione.
6. Crea MI minime per Concrete/Metal/Wall/Cover/Water/Ice se utili.
7. Popola una piccola arena in `L_DevSandbox` con:
   - area aperta;
   - corridoio;
   - angolo cieco;
   - choke;
   - low/high cover;
   - porta;
   - superficie speciale.
8. Valida dalla camera tattica.
9. Mantieni la board leggibile anche senza colore quando richiesto dalla grammatica.

### Polytexture

Se con “polytexture” il progetto non ha un termine canonico equivalente, NON inventare un subsystem.

Implementa soltanto un pass materiale placeholder economico che:
- rompa l'aspetto da primitive UE;
- resti facilmente sostituibile;
- non introduca dipendenze gameplay.

### Binary asset rule

Gli asset UE si creano/modificano via Editor. Se Claude non ha un ponte affidabile per farlo:
- prepara nomi, directory, parametri, Material graph spec e checklist;
- non scrivere `.uasset` manualmente.

### Test / verifica

- `L_DevSandbox` leggibile a camera tattica;
- nessuna collisione/authority dipende dal materiale;
- nessun asset fuori allowlist;
- build verde se viene aggiunto supporto C++;
- verifica visiva a colori e scala di grigi.

### Stop condition

Se il kit richiede una decisione ancora aperta in #1094, fermati e riporta l'ID `GBX-*`.

### Commit suggerito

`feat(graybox): add minimal visual slice kit`

---

<!-- 05_BOARD_VISUAL_GRAMMAR_956.md -->

## Task 05 — Grammatica visiva della board

**Issue:** #956 — “CP 47.3 · Grammatica visiva della board: colore E forma, mai solo il colore”

### Obiettivo

Rendere leggibili le superfici previste dall'issue anche quando il colore non basta.

### Stato noto da verificare

L'issue ha già convergito su una famiglia di segni basata su **anelli concentrici** per un quartetto di superfici. Non sostituire questa scelta con una nuova grammatica “più bella” senza una nuova decisione.

### Scope

Implementare il **delta reale** ancora aperto nella #956.

Non generalizzare a tutte le superfici se l'issue limita esplicitamente il DoD a un quartetto.

### Passi

1. Misura lo stato corrente dell'issue e dei test.
2. Verifica `ERTHexSurface` corrente.
3. Verifica `M_HexCell`, ISM e custom data esistenti.
4. Riusa il pattern geometrico deciso dall'issue.
5. Mantieni `RingCount` derivato dalla superficie, non come nuovo dato serializzato.
6. Non migrare il formato mappa.
7. Estendi il gate esistente invece di creare un secondo test owner.
8. Esegui la voce PIE prevista.
9. Valida la lettura a picco e in scala di grigi.

### Cose da NON fare

- niente enum nuovo;
- niente campo nuovo sulla cella;
- niente seconda tassonomia “Normal/Rough/Cover/Hazard/Objective” come dato;
- non confondere cover di bordo con superficie.

### Test

Riusa/estendi il test owner indicato dalla issue (`Hex.SurfaceColorsAreDistinguishable` o nome corrente verificato sul branch).

Esegui suite mirata + PIE della board.

### Commit suggerito

`feat(board): add redundant surface visual grammar`

---

<!-- 06_TACTICAL_READABILITY_289.md -->

## Task 06 — Leggibilità tattica

**Issue:** #289 — “CP E21.3 — Leggibilità tattica”

### Dipendenza

#287 è indicata come prerequisito storico e risulta chiusa; verificare sul branch.

### Obiettivo

Fare in modo che, durante la partita:

- team ring sia leggibile;
- selection ring sia leggibile;
- superfici siano leggibili senza console debug;
- la camera sia tarata sulla scala esagonale corrente.

### Passi

1. Misura cosa è già implementato.
2. Verifica materiali degli anelli e loro scala.
3. Verifica che la skeletal mesh non nasconda gli anelli.
4. Verifica la superficie a runtime senza `rt.Debug.DrawCells`.
5. Taratura camera:
   - pitch;
   - arm length;
   - apertura iniziale.
6. Non cambiare la scala esagonale per far entrare meglio la camera.
7. Non spostare l'Icon Language dentro questa issue.

### Verifiche

- team/selection ring distinguibili anche non solo per colore;
- board radius dello scenario previsto leggibile interamente;
- camera tattica non schiaccia cover/muri;
- PIE citate dalla issue riconfermate sulla skeletal.

### Test

Build + suite UI/camera applicabile + PIE.

### Commit suggerito

`feat(presentation): complete tactical board readability`

---

<!-- 07_NEW02_LOS_DEBUG_OVERLAY.md -->

## Task 07 — NEW-02: LOS debug overlay

**Nuova issue:** vedi `ISSUE_NEW02_LOS_DEBUG.md`.

### Obiettivo

Rendere **osservabile** la LOS esistente senza implementare una seconda LOS.

Questo è uno strumento di debug/presentazione, non una nuova regola competitiva.

### Vincoli

- riusare il produttore corrente di `VisibleCells` / LOS;
- nessuna query LOS duplicata nel renderer;
- nessun targeting dentro il sistema LOS;
- nessun Tick se non strettamente giustificato: preferire refresh/evento già esistente;
- debug spento di default.

### Esperienza desiderata

Con una unità selezionata, un toggle debug mostra:

- celle visibili;
- celle non visibili / fuori vista quando utile;
- cella puntata;
- esito `Visible/Blocked`;
- blocker/reason code se il servizio corrente lo espone;
- opzionalmente una linea sorgente→target solo come visualizzazione.

### Passi

1. Trova il produttore canonico della LOS.
2. Trova il punto in cui `VisibleCells` entra già in Team Knowledge.
3. Trova il sistema debug esistente (`Debug/`, console commands, overlay).
4. Aggiungi il consumer più piccolo possibile.
5. Non memorizzare una seconda copia persistente della verità LOS.
6. Mantieni i dati del debug fuori dalla simulazione.
7. Se serve una funzione pura per preparare DTO/segmenti di overlay, testala headless.
8. Aggiungi comando/toggle coerente col naming del progetto.
9. Documenta una voce PIE per:
   - muro blocca;
   - porta aperta lascia vedere;
   - porta chiusa blocca, se già supportato dal modello.

### Test

Minimo:
- test puro del mapping `LOS result -> overlay primitive` se introduci logica;
- regressione dei test LOS esistenti;
- PIE visiva in `L_DevSandbox`.

### Stop condition

Se scopri che il produttore LOS non espone una reason/blocker informazione, NON modificare l'algoritmo solo per il debug senza prima verificare l'owner. Puoi limitare la v0.1 a `Visible/Blocked`.

### Commit suggerito

`feat(debug): visualize existing line of sight`

---

<!-- 08_PARTIAL_KNOWLEDGE_1466.md -->

## Task 08 — Conoscenza parziale visibile

**Issue:** #1466 — “E13.6 · La conoscenza parziale diventa visibile…”

### Obiettivo

Chiudere i leak e rendere percepibile al giocatore la differenza tra:

- soggetto conosciuto;
- soggetto ricordato;
- soggetto ignoto.

### Stato noto da verificare

L'issue dichiara già implementati almeno:
- traduzione `TargetUnknown`;
- `FRTKnowledgeView` / porta filtrata.

Il delta storico ancora aperto include:
- HUD filtrato;
- combat log filtrato;
- unità ignota nascosta;
- sagoma Last Contact.

Verifica il branch: non rifare task già chiusi.

### Regola principale

Un soggetto ignoto non deve diventare “una entry con flag hidden” se l'owner corrente ha già deciso che la porta filtrata **omette** il soggetto.

### Passi

1. Leggi issue + spec owner.
2. Verifica i test `Knowledge.*` esistenti.
3. Misura i task T3–T6 ancora realmente aperti.
4. HUD:
   - usa la porta filtrata;
   - niente nome/HP di unità non note.
5. Combat log:
   - conserva diagnostica completa nel canale dev se previsto;
   - filtra il canale giocatore nel punto owner deciso dalla spec.
6. Rendering unità:
   - combina `alive` e `known`;
   - non nascondere indiscriminatamente l'actor se questo spegne anche la sagoma/overlay necessari.
7. Last Contact:
   - posizione del contatto, non posizione vera aggiornata;
   - identità sì se conosciuta;
   - condizione/HP no se non autorizzata;
   - niente facing se la grammatica corrente lo vieta.
8. Esegui le verifiche PIE della issue.

### Privacy gate

A fine task, un nemico fuori conoscenza NON deve essere ricostruibile da:
- unit overlay;
- combat log giocatore;
- modello 3D.

La traccia di movimento è trattata nel task successivo (#1497).

### Test

Esegui i test nominati dalla issue quando presenti sul branch e aggiungi soltanto quelli mancanti richiesti dal DoD.

### Commit suggerito

`feat(knowledge): complete partial-knowledge presentation`

---

<!-- 09_MOVE_TRACE_PRIVACY_1497.md -->

## Task 09 — Privacy della traccia di movimento

**Issue:** #1497 — “La traccia post-lock disegna il percorso di ogni unità, di entrambe le squadre”

### Obiettivo

Impedire che la traccia del movimento riveli la vera posizione/percorso di un nemico oltre ciò che la squadra ha osservato.

### Stato noto da verificare

L'issue riporta che `FRTMoveRoute` ha già ricevuto `StableUnitId`, ma il verdetto per cella e il rendering troncato erano ancora aperti.

### Regola decisa

La traccia deve mostrare **solo il tratto osservato** e interrompersi dove il soggetto non era più osservato.

Non filtrare semplicemente per `TeamId`: la regola è Team Knowledge/percezione.

### Passi

1. Verifica lo stato corrente di `FRTMoveRoute`.
2. Individua raccolta Move e Dash.
3. Mantieni una rotta per ogni unità mossa se i test owner lo richiedono.
4. Trasporta un verdetto per cella/segmento secondo la decisione corrente.
5. Metti la regola in una funzione pura testabile.
6. `DrawHUD` deve consumare il verdetto, non ricostruirlo arbitrariamente.
7. Rispondi anche al consumer `rt.Debug.DrawPaths`.
8. Aggiorna commenti/spec/PIE che descrivono il vecchio leak.
9. Usa un caso **Move**, non Dash, per evitare test vacuo se Dash viene azzerato prima del Planning.
10. Non risolvere in questa issue i problemi del playback/timeline se l'owner è un'altra issue.

### Test obbligatori dalla issue

- caso percorso osservato vs non osservato;
- mutazione: senza troncamento il test deve fallire;
- anti-vacuità;
- compatibilità con sagoma/Last Contact;
- regressione test identità rotta.

### Evidenza PIE

La traccia non deve entrare nella zona non osservata e non deve contraddire la sagoma Last Contact.

### Commit suggerito

`fix(knowledge): truncate enemy move traces to observed cells`

---

<!-- 10_KNOWLEDGE_VEIL_1535.md -->

## Task 10 — Aggancia il Knowledge Veil alla presentazione

**Issue:** #1535 — “E13.8 · Il velo si aggancia alla presentazione…”

### Obiettivo

Fare finalmente apparire a schermo il velo/conoscenza della squadra usando il sistema già implementato.

### Fatto architetturale

`ApplyKnowledgeVeil(const FRTTeamKnowledge&)` riceve una conoscenza **già scelta**.

Il renderer NON deve decidere autonomamente il team.

### Decisione da rendere esplicita

Per il 2v2 offline:

- quale team è il viewer;
- da quale owner si legge quel valore;
- cosa accade in spectator/replay (può restare fuori scope, ma deve essere scritto).

Non hardcodare `0`.

### Passi

1. Verifica che `ApplyKnowledgeVeil` esista e che non abbia consumer reali.
2. Verifica `OnTeamKnowledgeRefreshed` e i refresh point.
3. Individua il luogo di presentazione più coerente:
   - GameMode o PlayerController secondo la decisione corrente.
4. Leggi il viewer dal dato canonico (`PlayerTeamId` o owner deciso), non da literal.
5. Scegli la via minima per ottenere Team Knowledge:
   - snapshot pubblico già esistente, oppure
   - accessor stretto solo se il costo è misurato e giustifica nuova API.
6. Usa `AddUniqueDynamic` se il delegate è dinamico e il pattern esistente lo richiede.
7. Applica il velo:
   - alla prima inquadratura;
   - ai refresh di planning/perception previsti.
8. Nessun Tick.
9. Non spostare la scelta del viewer dentro `ApplyKnowledgeVeil`.
10. Aggiorna `PIE-HEX-VIZ-VELO`.

### Definition of Done

- niente primo frame completamente rivelato;
- viewer esplicito e testabile;
- velo segue refresh points, non tempo reale;
- integrazione testata dal percorso reale, non chiamando a mano `ApplyKnowledgeVeil`.

### Test

- integrazione percorso reale;
- prima inquadratura;
- riuso del test esistente che impedisce il Tick/real-time follow;
- PIE `PIE-HEX-VIZ-VELO`.

### Commit suggerito

`feat(perception): bind knowledge veil to player presentation`

---

<!-- 11_HUD_CONTRACT_77.md -->

## Task 11 — Chiudi il contratto informativo HUD

**Issue:** #77 — “CP 11.1 — HUD di partita completo”

### Obiettivo

Chiudere il delta del Canvas HUD corrente **prima** di costruire/finire il layer UMG.

### Stato noto da verificare

Storicamente l'issue riportava 5 voci su 6 già presenti.

Delta indicato:
- terna `MOVEMENT / MAIN / REACTION`;
- test headless;
- vocabolario `Round` invece di `Turno` dove richiesto.

Verifica il codice attuale: potrebbe essere avanzato.

### Passi

1. Misura il DoD reale della #77.
2. Non trasformare questa issue in migrazione UMG: quella è #613.
3. Implementa solo informazioni mancanti.
4. Deriva slot da intento/simulatore, non da copia tenuta nel widget.
5. `RoundLimit` deve venire dal formato.
6. Cooldown dal simulatore.
7. Nessuna informazione avversaria oltre quanto autorizzato da Team Knowledge.
8. Integra con i fix di #1466, non reintroducendo overlay ignoti.
9. Aggiungi/chiudi i test headless richiesti.
10. Esegui `PIE-V01-HUD`.

### Test

- slot derivati;
- cooldown interi/non negativi;
- RoundLimit data-driven;
- DTO/visibilità filtrata;
- regressione `RefactorTactics.HUD.*`.

### Commit suggerito

`feat(hud): close first-playable HUD information contract`

---

<!-- 12_HUD_UMG_613.md -->

## Task 12 — Screen HUD UMG

**Issue:** #613 — “CP 11.7 — Screen HUD in UMG (layer §4.1)”

### Obiettivo

Costruire/chiudere il layer **Screen HUD UMG** mantenendo il centro della mappa libero.

### Confine da rispettare

UMG §4.1:
- round/fase/timer;
- objective;
- roster;
- selected unit;
- action dock;
- warning;
- combat log;
- conferma piano.

World overlay §4.2:
- path;
- waypoint;
- AoE;
- friendly fire;
- facing;
- coni/indicatori spaziali.

**Non migrare §4.2 dentro un mega-widget.**

### Widget target

Verifica i nomi owner correnti; storicamente:

- `WBP_RT_TacticalHUD`
- `WBP_RT_TurnHeader`
- `WBP_RT_TeamRoster`
- `WBP_RT_SelectedUnitPanel`
- `WBP_RT_ActionDock`
- `WBP_RT_ActionSlot`

### Architettura

- Widget = presentazione;
- ViewModel/DTO = dati sanitizzati;
- nessuna formula competitiva nel Blueprint;
- nessuna decisione LOS/visibility nel widget;
- nessun accesso diretto a stato avversario non filtrato.

### Passi

1. Misura asset/classi già esistenti.
2. Definisci/riusa ViewModel C++.
3. Crea/chiudi layout Top/Left/Bottom/Right.
4. Mantieni il centro trasparente/libero.
5. TurnHeader dal ViewModel.
6. TeamRoster solo team autorizzato.
7. SelectedUnitPanel coerente con la selezione.
8. ActionDock con stati:
   - available;
   - selected;
   - cooldown.
9. Non referenziare texture direttamente: #220 gestisce il catalogo.
10. Debug spento di default.

### Binary asset rule

Se Claude non può creare/aggiornare WBP via Unreal Editor:
- implementa le classi C++/ViewModel necessarie;
- produci una checklist Editor precisa per ogni WBP;
- non simulare asset binari.

### Test / PIE

- regressione `RefactorTactics.HUD.*`;
- test ViewModel puri dove possibile;
- `PIE-V01-HUD` estesa:
  - leggibilità;
  - centro non coperto;
  - playback coerente.

### Commit suggerito

`feat(ui): add tactical screen HUD layer`

---

<!-- 13_ICON_CATALOG_220.md -->

## Task 13 — I widget consumano il catalogo icone

**Issue:** #220 — “CP 20.3 · I widget consumano il catalogo”

### Obiettivo

Eliminare riferimenti diretti a texture nei widget e cablare il catalogo icone esistente.

### Stato noto da verificare

Storicamente:
- `DA_IconCatalog` esisteva;
- catalogo con decine di chiavi/texture;
- `WBP_RT_ActionSlot` aveva `IconImage`;
- mancava il wiring via `URTIconLibrary`.

Non fidarti dei numeri storici: misura il branch.

### Passi

1. Verifica catalogo e chiavi richieste.
2. Verifica `URTIconLibrary`.
3. Verifica API dei widget C++ e Blueprint.
4. Collega gli slot al catalogo tramite ID/chiave.
5. Nessun `UTexture2D` diretto nelle API dei widget se il gate lo vieta.
6. Aggiungi/chiudi il test che intercetta anche proprietà Blueprint dirette se previsto dall'issue corrente.
7. Non creare un secondo catalogo.
8. Non mettere path asset hardcoded nei widget.
9. Esegui `PIE-ICON-01`.

### Test

- `ScreenHud.WidgetApiExposesNoTexture` o nome corrente;
- test catalogo;
- test Blueprint asset se già previsto;
- PIE icone distinguibili.

### Commit suggerito

`feat(ui): resolve widget icons through catalog`

---

<!-- 14_VISUAL_PACKAGE_GATE.md -->

## Task 14 — Gate finale Visual Slice v0.1

### Obiettivo

Verificare che la catena implementata funzioni insieme e in packaged Development.

Questo task non deve aprire nuove feature.

### Scenario manuale minimo

In `L_DevSandbox`:

1. avvia partita 2v2 offline;
2. camera apre sulla squadra del giocatore;
3. graybox arena leggibile;
4. seleziona una unità;
5. verifica team/selection ring;
6. attiva LOS debug:
   - muro blocca;
   - apertura permette LOS;
7. disattiva debug;
8. verifica che la conoscenza parziale nasconda un nemico ignoto;
9. verifica Last Contact quando applicabile;
10. verifica che la traccia di movimento non riveli celle non osservate;
11. verifica Knowledge Veil dal primo frame e dopo refresh;
12. verifica HUD:
   - round;
   - fase;
   - timer;
   - selected unit;
   - action slots;
   - cooldown;
   - ready/conferma se presente nello scope;
13. verifica icone dal catalogo;
14. risolvi un turno;
15. riconferma tutto dopo la resolution.

### Regression gate

Esegui:

```powershell
./scripts/rt-suite.ps1
```

Se la suite completa è troppo costosa durante iterazione, i filtri mirati sono ammessi, ma **prima della chiusura** usa il gate richiesto dal repository.

### Packaged Development

Crea/esegui una build packaged Development secondo il runbook del progetto.

Verifica almeno:

- board non nera;
- materiali graybox presenti;
- WBP caricati;
- icone risolte;
- knowledge veil presente;
- nessun debug overlay acceso di default;
- nessun asset mancante;
- niente riferimenti locali fuori repo.

### Performance sanity

Non fare ottimizzazione prematura, ma segnala:

- hitch evidente quando cambia il velo;
- ricostruzioni per-frame non necessarie;
- allocazioni/actor explosion;
- HUD o overlay che scalano con Tick senza motivo.

### Deliverable finale

Produci `VISUAL_SLICE_V01_REPORT.md` nel working tree o come output dell'handoff con:

- HEAD;
- build;
- suite;
- PIE eseguite;
- packaged result;
- screenshot richiesti;
- bug residui classificati:
  - BLOCKER;
  - SHOULD FIX;
  - POST-v0.1.

### Commit suggerito

`test(v0.1): validate visual slice packaged gate`

---

<!-- ISSUE_NEW01_GRAYBOX_KIT.md -->

# GitHub Issue proposta — NEW-01

## Titolo

`Visual Slice v0.1 — Graybox Kit minimo giocabile + material/polytexture pass`

## Labels suggerite

`v0.1`, `P1`, `enhancement`

## Body

### Obiettivo

Portare `L_DevSandbox` da esagoni/primitivi a una piccola arena tattica graybox leggibile a camera di gioco.

### Dipende da

- #1094 per le decisioni graybox necessarie;
- #1095 per Cell Placement Volume e scena di validazione.

### Scope

Solo kit minimo:

- Wall Full;
- Wall Broken/variant se autorizzato;
- Low Cover;
- High Cover;
- Pillar/Block;
- Door Frame;
- Door states solo se la grammatica è già decisa;
- placeholder acqua/ghiaccio;
- Material Master graybox;
- poche Material Instance / texture placeholder.

### Vincoli

- mesh/materiali non sono authority;
- nessuna passabilità/LOS/cover derivata dalla collisione della mesh;
- asset sotto il percorso Content autorizzato;
- pivot/scala conformi al placement contract;
- niente catalogo completo;
- niente final art.

### DoD

- [ ] una piccola arena graybox esiste in `L_DevSandbox`
- [ ] muro, low/high cover, porta e superfici richieste sono distinguibili a camera tattica
- [ ] il kit rispetta i safe placement bounds
- [ ] Material Master/istanze sono sostituibili e non contengono gameplay
- [ ] nessun asset fuori allowlist
- [ ] verifica a colori e in scala di grigi
- [ ] packaged Development carica gli asset senza missing reference

### Out of scope

GAS · final art · catalogo completo · multilivello completo · ambiente sistemico · networking.

---

<!-- ISSUE_NEW02_LOS_DEBUG.md -->

# GitHub Issue proposta — NEW-02

## Titolo

`Visual Slice v0.1 — LOS debug overlay su VisibleCells e blocker`

## Labels suggerite

`v0.1`, `P1`, `enhancement`

## Body

### Obiettivo

Rendere osservabile la LOS già esistente durante lo sviluppo senza creare un secondo sistema LOS.

### Dipende da

- produttore LOS/VisibleCells esistente;
- graybox leggibile (#1095 + NEW-01);
- #289 consigliata prima della verifica finale.

### Scope

Toggle debug che può mostrare:

- celle visibili;
- cella puntata;
- esito Visible/Blocked;
- blocker/reason quando già disponibile;
- linea sorgente-target opzionale.

### Vincoli

- nessun algoritmo LOS duplicato nel renderer;
- nessun targeting dentro il debug LOS;
- nessuna authority nella presentazione;
- debug spento di default;
- preferire refresh/eventi esistenti, nessun Tick non necessario.

### DoD

- [ ] il debug consuma il risultato LOS canonico
- [ ] muro/blocker cambia chiaramente il risultato
- [ ] porta aperta/chiusa è osservabile se già supportata dal modello
- [ ] nessuna divergenza fra debug e Team Knowledge
- [ ] regressione test LOS verde
- [ ] verifica PIE in `L_DevSandbox`
- [ ] debug assente di default in packaged/player view

### Out of scope

Nuova LOS · targeting · projectile trajectory · Fog of War · perception rewrite.

---

<!-- 99_HANDOFF_TEMPLATE.md -->

# Template handoff Claude — Visual Slice v0.1

## Risultato

...

## Stato Git

- Branch:
- HEAD iniziale:
- HEAD finale:
- Working tree:

## File modificati

- ...

## Decisioni prese

- ...

## Decisioni NON prese

- ...

## Build

Comando:
```powershell
...
```

Esito:
...

## Test

| Test / filtro | Esito | Evidenza |
|---|---|---|
| ... | PASS/FAIL | ... |

## PIE / Editor

- [ ] ...
- [ ] ...

## Binary assets

- Asset modificati:
- Sessione Editor:
- Operazioni manuali richieste:

## Limiti / rischi aperti

- ...

## Commit suggerito

`type(scope): message`

## Prossimo task

`NN_....md`

---

<!-- PROMPT_LAUNCHER_CLAUDE.md -->

# Prompt launcher per Claude Code

Copia questo prompt in Claude Code dalla root di `refactor-tactics-main`:

```text
Stiamo implementando la Visual Slice v0.1 di RefactorTactics.

Leggi prima AGENTS.md e CLAUDE.md. Poi apri il file task che ti indico sotto e trattalo come piano operativo, non come fonte normativa superiore al repository.

Prima di implementare:
- misura branch/HEAD/git status;
- leggi l'issue GitHub correlata e i commenti correnti;
- verifica il codice e i test reali;
- segnala eventuali differenze fra il task e lo stato corrente.

Non lavorare dalla memoria e non espandere lo scope.

TASK DA ESEGUIRE:
<INCOLLA QUI IL PERCORSO, es. docs/tasks/visual-slice/01_PRECHECK_GBX_1094.md>

Completa un solo task per volta.
Alla fine usa esattamente il formato handoff indicato nel task.
Non iniziare il task successivo finché non ti do conferma.
```

---

<!-- README.md -->

# File inclusi

- `00_MASTER_VISUAL_SLICE.md`
- `01_PRECHECK_GBX_1094.md`
- `02_GRAYBOX_VALIDATION_1095.md`
- `03_CLOSE_GBX_1094.md`
- `04_NEW01_GRAYBOX_KIT_MATERIALS.md`
- `05_BOARD_VISUAL_GRAMMAR_956.md`
- `06_TACTICAL_READABILITY_289.md`
- `07_NEW02_LOS_DEBUG_OVERLAY.md`
- `08_PARTIAL_KNOWLEDGE_1466.md`
- `09_MOVE_TRACE_PRIVACY_1497.md`
- `10_KNOWLEDGE_VEIL_1535.md`
- `11_HUD_CONTRACT_77.md`
- `12_HUD_UMG_613.md`
- `13_ICON_CATALOG_220.md`
- `14_VISUAL_PACKAGE_GATE.md`
- `99_HANDOFF_TEMPLATE.md`
- `ISSUE_NEW01_GRAYBOX_KIT.md`
- `ISSUE_NEW02_LOS_DEBUG.md`
- `PROMPT_LAUNCHER_CLAUDE.md`
