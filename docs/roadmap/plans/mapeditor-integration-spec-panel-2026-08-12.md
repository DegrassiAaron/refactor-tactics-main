# Map Editor — audit issue e integrazione roadmap — spec panel

> `CURRENT` · **Stato**: revisione chiusa, **applicata in parte** · **Data**: 2026-08-12
> **HEAD della revisione**: `4fce92ca`
> **Sorgente revisionato**: `RefactorTactics_MapEditor_Roadmap_Issue_Integration_2026-08-12.md` (275 righe,
> untracked), archiviato a fine sessione in
> [`../../archive/src/handoff/2026-08-12-mapeditor-roadmap-issue-integration.md`](../../archive/src/handoff/2026-08-12-mapeditor-roadmap-issue-integration.md)
> **Scopo**: classificare ogni voce dell'audit contro il repository **misurato**, prima che qualcuno la
> applichi a issue, registry, mappe o wiki.
> **Regola applicata**: un handoff AI è l'ultima fonte della gerarchia.
> **Quarto della sua famiglia**: vedi §1.1.

---

## 1. Il verdetto in una riga

Il sorgente è il **primo della serie map-editor che non chiede di ricostruire l'editor**: non ha duplicati di
tool, camera o toolbar, e il suo §4 — *«niente secondo pathfinder, reachability system, edge helper o
authority geometry»* — è la regola di questo repository scritta da chi non sapeva che fosse già scritta. In
compenso sbaglia il fatto centrale attorno a cui costruisce metà del documento: **dichiara aperta una issue
chiusa un'ora e quarantaquattro minuti prima che lui fosse scritto**, e ne deriva una roadmap, un percorso
critico e un ordine pratico.

| | Voci | Significato |
|---|---:|---|
| `CURRENT` | **17** | riporta correttamente lo stato, spesso senza sapere che è canone |
| `STALE` | **5** | si rivolge a uno stato che non esiste più al momento in cui il documento è scritto |
| `PROPOSED` | **2** | idea nuova, nessun conflitto: si apre |
| `CONFLICT` | **2** | contraddice una regola di processo già in vigore |
| `DUPLICATE` | **1** | chiede una cosa che ha già un owner e un check |

**Su 27 voci classificate, 2 sopravvivono come lavoro nuovo.** Le altre 25 sono conferme, correzioni di
riferimenti, o cose che il repository fa già.

### 1.1 Il quarto della famiglia — e stavolta paga un debito invece di crearne uno

| Sorgente | Referto | Esito |
|---|---|---|
| `2026-08-09-map-editor-roadmap.md` | [`map-editor-brief-spec-panel-2026-08-09.md`](map-editor-brief-spec-panel-2026-08-09.md) | ⛔ **Non applicato** — 9 duplicati, 5 conflitti. Sopravviveva **una** proposta |
| `2026-08-10-full-grid-geometry-walls-water.md` | [`triage-grid-geometry-water-2026-08-10.md`](triage-grid-geometry-water-2026-08-10.md) | 55+ sezioni `LOCKED`; `GEO-1`…`GEO-3` |
| `2026-08-12-map-sketch-editor.md` | [`map-sketch-editor-spec-panel-2026-08-12.md`](map-sketch-editor-spec-panel-2026-08-12.md) | **Applicato in parte**: `#619`…`#623`, `MSE-1` |
| *questo* | *questo file* | **applicato in parte**: due issue nuove, cinque corpi corretti, i derivati di `#554` |

✅ **La differenza che conta.** Il referto del 2026-08-12 chiudeva dichiarando **una** eredità scoperta e
scriveva perché non la apriva:

> «**`P7` non è aperta da questo referto** ed è la sola eredità che resta scoperta: la sonda di movimento del
> panel 2026-08-09. Non la apro qui perché non appartiene a questo prompt e aprirla di straforo ripeterebbe il
> difetto al contrario — *una issue senza il documento che la motiva*.»

Il §R6 di questo sorgente **è quel documento**. Descrive la sonda con start, profilo, budget, `reason` e
divieto di pathfinder parallelo: la condizione che il referto precedente aveva posto è soddisfatta, e `P7`
si apre qui. È la prima volta in quattro prompt che un handoff chiude una prescrizione invece di aggiungerne.

---

## 2. Il panel

Sette revisori, un focus ciascuno. Le citazioni sono ricostruzioni della metodologia, non attribuzioni reali.

### 📋 WIEGERS — qualità dei requisiti

> «§R8 elenca dodici voci di DoD e una è **non soddisfabile come scritta**: *"`editor-sessions.yaml` senza PIE
> orfane"*. L'ho misurata. Il registro porta **132 voci `PIE-*`**; le sedute ne citano **72**; ne restano
> **60 orfane** — `PIE-VIS-*`, `PIE-STATE-*`, `PIE-MUT-*`, roba di combat, status e mutazione che con l'editor
> non c'entra niente. Un DoD di una issue map-editor che si chiude solo collocando sessanta voci di
> altre aree non è un criterio: è un blocco permanente travestito da checklist.»

> 🔴 **Nota di metodo, e vale più del numero.** La prima misura di questo paragrafo diceva `123 · 72 · 57`, ed
> era **sbagliata**: il pattern usato per estrarre i nomi era `PIE-[A-Z0-9-]+`, che scarta ogni voce con una
> lettera minuscola — `PIE-AS4a`, `PIE-AS4b`, `PIE-BU2b`, `PIE-BU3c`, `PIE-HEXPLAY-3b`, `-4b`, `-6b`, `-6c` —
> e con un punto: `PIE-CP1.4`. **Nove voci su 132 invisibili al criterio**, cioè il difetto che questo
> repository ha già a catalogo: *un gate che copre meno righe di quante ne esistano*. Il registro PIE ha un
> comando canonico (`awk -F'|' '/^\| \*\*PIE-/…'`), scritto in `test-manuali-pie.md` proprio perché nessuno
> lo reinventi — e reinventarlo è esattamente ciò che è successo. La misura sopra è quella del comando.

> «Il sottoinsieme utile però esiste, ed è **piccolo e falsificabile**: nel perimetro editor le voci orfane
> **aperte** sono **cinque** — le `PIE-HEX-VIZ-*` nate il 2026-08-12 con la serie viz. Le altre `PIE-HEX-*`
> orfane (`MODE-A`, `-B`, `-C`, `-D`, `-I`, `-J`, `-K`, `-M`, e `PIE-HEX`) sono orfane **perché già ✅**: non
> vanno collocate, vanno lasciate in pace. Questo è il numero da mettere in un DoD — *da 5 a 0*, non
> *"senza orfane"*.»

> «§7 elenca quattordici voci di DoD dell'Editor v0.1 aperte tutte con un verbo di capacità: *"l'autore può
> vedere / creare / leggere / lavorare / usare"*. Con quale osservazione dimostro che *"l'autore può leggere
> costo, movement block e LOS block senza Details"* è **falso**? Quella riga ha già la sua risposta e non è in
> §7: è `PIE-HEX-VIZ-BLOCCHI` e `PIE-HEX-VIZ-COSTO`, con precondizione e criterio scritti. §7 non è sbagliata,
> è **già derivabile** — e una lista derivabile scritta a mano è la quindicesima vista da tenere allineata.»

### 🎯 COCKBURN — attore primario e obiettivo

> «L'attore è lo stesso dei tre prompt precedenti — il level designer — e §R6 è il punto in cui finalmente
> gli si chiede *cosa vuole sapere* invece di *cosa vuole vedere*. La distinzione che il documento fa fra
> `#554` e la sonda è la cosa migliore che contiene, e vale citarla: `#554` risponde *«questa zona è
> strutturalmente raggiungibile dagli spawn?»*, la sonda risponde *«dove arriva **questa unità** con **questo**
> budget?»*. Due domande, due strumenti, nessuna sovrapposizione. È un confine per **soggetto**, che è il tipo
> di confine che regge.»

> «§R5 supera lo stesso esame. Ho verificato le tre issue che citerebbe: `#620` mette *"il rendering del ghost
> e dello snap in viewport"* esplicitamente **fuori scope**, `#621` possiede la cottura, `#622` la griglia di
> lavoro. Il **gesto** — disegnare, vedere il ghost, annullare — non ha padrone in nessuna delle tre. Non è una
> issue inventata per completezza: è un buco che si vede guardando i tre bordi.»

### 🏗️ FOWLER — confini e responsabilità

> «§R8 propone una issue *"M9 Integration Gate"* che *"non è una nuova fonte di stato: chiude solo il wiring"*.
> La frase si contraddice da sola: se chiude il wiring di otto issue, allora fino a quel momento otto issue
> **possono chiudersi senza il proprio wiring** — ed è esattamente il difetto che questo repository ha
> catalogato e pagato. Il registry, le shortlist e la Wiki non sono un passo finale, sono la **condizione di
> chiusura di ogni singola issue**.»

> «La prova che è già così sta nel repository, non in un'opinione: `feature_registry.py validate` **è** il
> gate di integrazione, gira a mano, e oggi dà `errori: 0 · warning: 33`. Una issue che promette di far girare
> alla fine un comando che deve essere verde **adesso** non aggiunge un controllo: aggiunge un permesso di
> rimandare. E M9 ha già i suoi tre checkpoint in `roadmap-checkpoint.md` — `M9.1` è letteralmente *"residuo
> editor mappa (H5)"*. Una issue omonima sarebbe la quinta vista dello stesso stato.»

### 🛡️ NYGARD — modi di guasto e invarianti

> «§R2 è il guasto, e va descritto con precisione perché la sua forma è istruttiva. Il documento è timbrato
> **2026-08-12 20:10 CEST**. La PR `#707` — `feat/554-transizioni-visibili` → `main` — è mergiata alle
> **18:26:29**, e `#554` è **CLOSED**. Il documento non è *datato*: è stato scritto **un'ora e quarantaquattro
> minuti dopo** il fatto che nega, e su quella negazione costruisce cinque azioni numerate, una riga del
> percorso critico, il punto 2 dell'ordine pratico e una riga di §5.»

> «Il modo in cui si sbaglia è più interessante dell'errore. `#694` **è** chiusa unmerged, e questo è vero: era
> impilata su `#688` e puntava a `test/migrazione-asset-reale`, non a `main`. Chi ha scritto ha guardato la PR
> giusta e ha concluso che il codice non fosse atterrato — ma il branch è stato **ricollocato** e rimergiato
> come `#707`. È il difetto che questo repository ha già a catalogo: *un branch mergiato che sembra orfano*.
> Lo stato di una PR non è lo stato del lavoro; `gh pr list --head <branch>` lo è.»

> «Una conseguenza operativa che nessuno ha ancora tratto, e che vale più della correzione: `#554` è chiusa e
> **i suoi derivati no**. Il registry dichiara `log_debug: partial` e la nota che lo motiva dice, oggi, su
> `main`: *"Il gate NON avanza a `done` perche' `#554` … e' ancora aperta"*. Il test che `#554` ha portato —
> `RefactorTactics.Arena.CriterionAndOverlayCountTheSameCells`, `RTArenaCriteriaTests.cpp:469` — **non è
> nell'elenco `tests:`** della feature. Il sorgente ha ragione sul fatto che qualcosa di `#554` è rimasto
> aperto: si è sbagliato solo su **cosa**. Non il codice: la sua contabilità.»

### 🔗 HOHPE — flusso e legami fra registri

> «Ho controllato se il legame *feature ↔ voce PIE* fosse da costruire, come §R8 lascia intendere. **Non lo è,
> ed è pagato a monte**: `editor-sessions.yaml` porta `verifies:` per seduta, `U18` ospita le
> `PIE-HEX-LAYER-*` e `U1` le `PIE-HEX-MODE-*`, e la EditorMap si genera da lì. Chi volesse "collegare le PIE
> alla feature" aggiungerebbe un secondo legame accanto a uno che funziona.»

> «⚠️ Va detto anche il contrario, perché è la trappola simmetrica. Il registry **ha** un campo `pie_refs`, e
> `RT-FEAT-TOOL-MAP-EDITOR` non lo dichiara. Sembra una lacuna. Non lo è: quel campo ha **un solo consumatore**
> — il controllo sul gate `packaged` — e per l'editor `packaged` è `na`, perché non entra nella build di gioco.
> Compilarlo lì produrrebbe un campo che nessun controllo legge, cioè il *dato senza consumatore* che questo
> repository ha già pagato quattro volte. **Si lascia vuoto, e si scrive perché.**»

### 🧪 CRISPIN — strategia di test

> «§R6 chiede *"hover → best path + costo"* e §R5 *"una gesture = una transaction Undo"*. Il primo aveva un
> prerequisito noto e **non ce l'ha più**: il referto del 2026-08-09 osservava che `FRTHexReachableCell` era
> `{Cell, Cost}` e che senza predecessore ogni hover avrebbe richiesto un pathfinding suo. Rimisurato oggi:
> `RTHexSim.h:88` porta **`FromCell`**, *"il predecessore nel percorso più economico"*, aggiunto per il facing
> di CP 13.5. La sonda non deve chiederlo — deve **consumarlo**, e se lo ricalcolasse sarebbe esattamente il
> pathfinder parallelo che §R6 vieta due righe più sotto.»

> «Il vincolo strutturale resta quello di sempre e va ripetuto nelle due issue nuove: in
> `Source/RefactorTacticsEditor/` **non esiste alcun test**. Ne segue che il *reachable set*, il *reason* di
> esclusione e la validazione del gesto vivono nel modulo runtime; l'editor li **chiama**. Se nascono
> nell'editor nascono non verificabili — ed è la stessa regola per cui `#619` e `#621` mettono maschera e
> cottura nel runtime.»

> «Sul gesto: *"una gesture = una transaction Undo"* è falsificabile e va scritto così — si disegna un
> segmento, si preme `Ctrl+Z` **una volta**, e la mappa torna com'era. Se ne servono due, il criterio è rosso.
> Ma la verifica è a schermo: è una voce `PIE-*`, non un test.»

### 📐 ADZIC — esempi eseguibili

> «Il documento non porta un solo esempio concreto — nessuna maschera, nessun costo, nessun `reason`. È il suo
> limite rispetto al predecessore, che aveva
> `Occupied Sectors: 5 / 12 · Mask: 001111000100 · Classification: Constrained` e da lì si scriveva un test
> senza chiedere niente a nessuno.»

> «Per §R6 l'esempio manca dove serve di più: *"cella esclusa → reason"*. Quali `reason`? Il repository ha già
> il vocabolario — `BlockedByTopology` è il reason code che `TruncatePathToTopology` produce quando un percorso
> incontra una porta chiusa. La sonda deve **riusare quel vocabolario**, non inventarne uno d'editor: due
> elenchi di ragioni per la stessa esclusione divergono, ed è la ragione per cui la tavolozza delle superfici
> è una sola.»

### 💬 DOUMONT — struttura del messaggio

> «§6 disegna il percorso critico in ASCII e §2 lo racconta in prosa: due rappresentazioni dello stesso ordine,
> e già divergono — il diagramma fa passare la sonda da `#622`, l'ordine pratico la mette dopo il Geometry
> Tool. Con `#554` rimosso dal percorso il diagramma va **ridisegnato**, non corretto: la sua prima colonna
> esiste solo per lei.»

---

## 3. `STALE` — le cinque voci che si rivolgono a uno stato che non esisteva più

Tutte discendono da un fatto solo, e vanno corrette insieme.

| # | § | Il sorgente dice | Misurato su `4fce92ca` |
|---|---|---|---|
| **S1** | §1 | `#554` · `OPEN` · *«recuperare PR #694, chiusa unmerged»* | `#554` **CLOSED**. `#707` `feat/554-transizioni-visibili` → `main` **MERGED** `2026-08-12T18:26:29` |
| **S2** | §R2 | Cinque azioni per «portare il diff proprio di `#694` su `main`» | Il diff **è** su `main`: `2f3c53cf` (transizioni sempre visibili, celle isolate in magenta) + `7ae2ce44` (raggiungibilità pigra) |
| **S3** | §5 | *«`#554` resta OPEN perché `#694` non è mergiata»* | La premessa è vera di `#694` e falsa del lavoro: il branch è stato ricollocato su `main` e rimergiato |
| **S4** | §6 | Il percorso critico parte da `#554 recover PR694 ──► #622 ──► Movement Probe` | Quel nodo non esiste. `#622` e la sonda non hanno più un predecessore aperto |
| **S5** | §6 | Ordine pratico: `#687 → #554 → #620 → …` | Il punto 2 cade; l'ordine scala |

> ⚠️ **Il rilievo che sopravvive alla correzione.** `#554` è chiusa *e la sua contabilità no*. Il sorgente
> chiedeva la cosa giusta per la ragione sbagliata, e va fatta comunque — vedi §5.

---

## 4. `CONFLICT` — due voci contro una regola di processo in vigore

| # | § | Il sorgente chiede | Il canone dice | Esito |
|---|---|---|---|---|
| **C1** | §R8 | Una issue `M9 Integration Gate` che «chiude il wiring» di otto issue: registry, EditorMap, ScenarioMap, MilestoneMap, Wiki, PIE | Il wiring **è la condizione di chiusura di ogni issue**, non un passo finale. `feature_registry.py validate` è il gate e gira a mano; `M9.1` in `roadmap-checkpoint.md` è già *«residuo editor mappa (H5)»* | **Non si apre.** Il suo contenuto è il lavoro di **questo** commit; la parte ricorrente è già la checklist in vigore |
| **C2** | §7 | Quattordici voci di DoD «Editor v0.1» da tenere in un elenco | Le stesse quattordici condizioni sono già i gate del registry + le voci `PIE-*` con precondizione e criterio | **Vista derivabile.** Non nasce un elenco a mano: si citano gli owner. È la stessa ragione per cui `roadmap-editor.md` è `HISTORICAL` |

---

## 5. `CURRENT` — ciò che il sorgente riporta bene, e le cinque correzioni che chiede

Le richieste di §3 sono **verificate e corrette**: i corpi di `#620` e `#622` citano davvero uno stato scaduto.

| Issue | Il corpo dice oggi | Misurato | Azione |
|---|---|---|---|
| `#620` | *«`#588` — **aperta**, e la PR `#598` … **non è mergiata**: su `main` `RTHexEditorClick.cpp:84` porta ancora `Result.GetActor()`»* | `#588` **CLOSED**; `#598` **MERGED** `07:07:42`; il confronto è sul componente | ✅ corretto |
| `#622` | *«il suo irrobustimento è **ancora aperto**: `#588` e la PR `#598` non sono mergiate»* · Out of scope: *«La raggiungibilità → `#554`»* | idem; e `#554` è chiusa | ✅ corretto |
| `#621` | dipendenze | già aggiornata il 2026-08-12 (`D1`/`D2`); manca il legame esplicito con `#687` | ✅ aggiunto il rischio di formato |
| `#623` | seduta + comando | A/B/C non esplicitati | ✅ esplicitati |
| `#695` | label `v0.1` sola | manca `P2` e la dichiarazione di follow-up | ✅ label + riga |

Sono corrette anche, e vanno riconosciute perché sono la parte che regge: §4 (**niente** secondo pathfinder,
reachability, edge helper o authority geometry), il divieto di `Walls[]` runtime e di geometria nel `.umap`,
la conferma che `#324`/E23 non si apre, e che `#687` protegge qualunque schema serializzato che richieda una
migrazione trasformativa.

---

## 6. `PROPOSED` — le due che si aprono

| # | Issue | Cosa | Perché regge |
|---|---|---|---|
| **N1** | [`#712`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/712) | **Geometry Authoring Tool** — il gesto: disegno quantizzato, ghost/snap, una gesture = una transaction, che **chiama** il validator di `#620` e la cottura di `#621` | I tre bordi sono verificati: `#620` mette ghost/snap fuori scope, `#621` possiede la cottura, `#622` la griglia. Nessuno possiede il gesto. La decisione sulla persistenza **non si duplica**: è `MSE-1` |
| **N2** | [`#711`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/711) | **Movement Probe** — start, profilo, budget, `reason`, sopra `ReachableCells` autorevole | È `P7`, ereditata dal panel 2026-08-09 e mai aperta. Il documento che la motiva ora esiste. Il prerequisito è **già pagato**: `FromCell` (`RTHexSim.h:88`) |

**Non si aprono**: `M9 Integration Gate` (`C1`) e la DoD-elenco di §7 (`C2`).

---

## 7. `DUPLICATE` — una

| § | Il sorgente chiede | Esiste già come |
|---|---|---|
| §R8 | *«`editor-sessions.yaml` senza PIE orfane»* come DoD di una issue | Il legame *seduta ↔ voce PIE* è `verifies:` in `editor-sessions.yaml`, e la EditorMap **conta già** le orfane: fu così che le tre `PIE-HEX-LAYER-*` furono collocate in `U18` il 2026-08-12. Non serve una issue: serve applicare lo stesso precedente alle cinque `PIE-HEX-VIZ-*` |

---

## 8. Cosa si fa — e cosa non si fa

**Non fare**: aprire `M9 Integration Gate`; scrivere l'elenco DoD di §7 a mano; recuperare `#694`; riaprire
`#554`; aggiungere `pie_refs` a `RT-FEAT-TOOL-MAP-EDITOR` (`packaged: na` — sarebbe inerte); collocare le
le sessanta voci PIE orfane fuori perimetro; aprire `#324`/E23.

**Le issue aperte da questo referto** — due, nell'ordine in cui pagano.

| # | Issue | Cosa | Gate d'uscita |
|---|---|---|---|
| 1 | [`#711`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/711) | **Movement Probe** | Una cella esclusa dà un `reason` del vocabolario esistente, e il percorso mostrato viene da `FromCell` — non da una seconda ricerca |
| 2 | [`#712`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/712) | **Geometry Authoring Tool** | Un segmento fuori grammatica è rifiutato dal validator di `#620`, e una gesture si annulla con **un** `Ctrl+Z` |

`#711` è indipendente — usa un servizio che esiste — mentre `#712` è a valle di `#620` e `#621`, entrambe
aperte. Per questo l'ordine è invertito rispetto al §6 del sorgente.

**Il lavoro di contabilità che questo commit fa**, e che nessuna issue deve ereditare:

1. `#554` esce dalle note come «aperta»; il suo test entra in `tests:`; `log_debug` si rimisura.
2. Le sei `PIE-HEX-VIZ-*` entrano in `U18` — **da 5 orfane aperte a 0** nel perimetro editor. Sul totale del
   registro l'effetto è `60 → 57` orfane su `132 → 135` voci *(rimisurato **sull'albero mergiato**: nel
   frattempo il reconciliation di roadmap ne ha aggiunte due, e i due rami partivano dalla stessa base 132 —
   nessuno dei due numeri pre-merge sarebbe rimasto vero)*. Il resto è di altre aree e resta dov'è.
3. Nasce la voce PIE che `#554` non ha lasciato: la sua acceptance è **visiva** e non è verificabile headless.
4. I cinque corpi issue di §3 si allineano allo stato reale.

---

## 9. Punteggi

Misurati sul sorgente **come istruzione per questo repository**.

| Dimensione | Voto | Evidenza |
|---|---:|---|
| **Chiarezza** | 8/10 | §R5/§R6 separano due strumenti per **soggetto**; §4 e §5 dicono cosa non fare, che è la parte difficile |
| **Completezza** | 5/10 | Zero esempi eseguibili; nessun `reason` nominato; §7 non cita gli owner che già la contengono |
| **Testabilità** | 4/10 | Un DoD non soddisfabile (`PIE orfane`), quattordici criteri «l'autore può» senza osservazione che li falsifichi |
| **Coerenza** | 6/10 | §4 vieta i duplicati e §R8 ne propone uno; §2 e §6 divergono sull'ordine |
| **Allineamento al canone** | 4/10 | Il fatto centrale è **falso al momento della scrittura**, e cinque voci ne discendono — ma i divieti di §4 sono canone esatto |

Il voto di allineamento sarebbe stato il più alto della serie senza `#554`: non c'è un solo duplicato di
tool, camera o toolbar, che è ciò su cui i tre predecessori avevano perso. **Il difetto non è di conoscenza
del dominio: è di verifica dello stato.**

---

## 10. Rapporto con gli altri documenti

| Documento | Ruolo rispetto a questa revisione |
|---|---|
| [`../../archive/src/handoff/2026-08-12-mapeditor-roadmap-issue-integration.md`](../../archive/src/handoff/2026-08-12-mapeditor-roadmap-issue-integration.md) | Il sorgente revisionato — **provenienza, non regola** |
| [`map-sketch-editor-spec-panel-2026-08-12.md`](map-sketch-editor-spec-panel-2026-08-12.md) | **Il predecessore diretto**: apre `#619`…`#623` e lascia `P7` scoperta. §R6 di qui la chiude |
| [`map-editor-brief-spec-panel-2026-08-09.md`](map-editor-brief-spec-panel-2026-08-09.md) | Dove `P7` — la sonda di movimento — fu proposta la prima volta |
| [`../feature-registry.yaml`](../feature-registry.yaml) | Owner dello stato di ogni feature citata |
| [`../editor-sessions.yaml`](../editor-sessions.yaml) · [`../editormap.shortlist.md`](../editormap.shortlist.md) | Owner delle sedute e vista **generata** — §R8 non ne crea una terza |
| [`../roadmap-checkpoint.md`](../roadmap-checkpoint.md) | Owner di **M9**, che ha già `M9.1`…`M9.3` |
| [`../../technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md) | Owner delle voci `PIE-*` e dei loro esiti |
| [`../../technical/brief-editor-map-viz.md`](../../technical/brief-editor-map-viz.md) | Owner della visualizzazione in editor |
| [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) | `MSE-1`, che il Geometry Authoring Tool **cita** invece di riaprire |
