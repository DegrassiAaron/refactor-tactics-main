# HUD Planning / Prediction · doppia roadmap — spec panel sul brief esterno

> `CURRENT` · **Stato**: revisione chiusa. Il sorgente è **recensito, non applicato** ·
> **Data**: 2026-09-05
> **HEAD della revisione**: `b063a60f` (branch `fix/1793-2167-posa-e-offset`)
> **`origin/main` al momento della misura**: `026850c0` (2026-09-05 07:58)
> **Oggetto**: brief operativo *«Refactor Tactics — HUD Planning / Prediction, doppia roadmap sincronizzata
> Code/Architecture + Unreal Editor/MCP/User»* (fornito in chat, **senza sha né data**)
> **Panel**: Wiegers (lead) · Fowler · Nygard · Crispin · Adzic · Cockburn
> **Modo**: critique

---

## 0. Come è stato misurato, e i limiti della misura

Il brief è stato letto **contro il repository**, non contro sé stesso: ogni sua asserzione su owner, issue,
file e capability è stata verificata. Comandi rilevanti: `gh issue view` sulle nove issue citate, `grep`
sugli assi semantici in `Source/`, lettura degli header di statuto dei quattro documenti owner, probe HTTP
su `127.0.0.1:8765`.

⚠️ **Limite dichiarato.** La misura è su `b063a60f`, che è **due giorni dietro** `origin/main`
(`026850c0`). Le conclusioni su codice e issue reggono; un merge di `main` può spostare gli stati. Nessuna
build, nessun test, nessuna `rt-suite`, nessun PIE è stato eseguito — vedi §8.

---

## 1. Verdetto

Il brief è **tecnicamente competente e ben scritto**, e la sua critica architetturale (§1.2, §1.5, §1.6) è
corretta e vale più della roadmap che propone. Ma **non è eseguibile come mandato**, per tre ragioni
misurate e indipendenti:

1. il suo obiettivo dichiarato — *«senza creare una seconda source of truth»* — è **violato dai suoi stessi
   due deliverable**, che duplicano owner esistenti e documentati (F-01);
2. la Roadmap B ha **zero capability** oggi: il ponte MCP è giù, e non è un ponte di sola lettura (F-02);
3. il primo milestone di codice (A2) **non è lavoro di codice**: il view model esiste ed è testato, e il
   blocco è su sei `.uasset` che, per il piano già accettato, *«nessun agente può fare»* (F-03).

**Raccomandazione del panel**: non aprire nulla. Riusare `#1941` come sede di A1, e trattare A2 come una
seduta d'Editor umana, non come una fetta di roadmap.

---

## 2. Le premesse del brief, misurate

| Asserzione del brief | Misura | Esito |
|---|---|---|
| `#25` E11 possiede HUD/log/debug | OPEN · `v0.1 · Leggibilità` | ✅ |
| `#172` possiede Ghost Timeline | OPEN · CP 11.5 | ✅ |
| `#173` possiede scrubbing fasi | OPEN · CP 11.6 | ✅ |
| `#613` possiede Screen HUD UMG | OPEN · CP 11.7 | ✅ |
| `#705` possiede Pointer Contract | OPEN · CP 11.8 | ✅ |
| `#1859` possiede PC Gym | OPEN | ✅ |
| `#2184` estrae decisioni da `DrawHUD` | OPEN · **E50, non E11** · nessuna milestone | ⚠️ parziale |
| `#1712` «verificare stato» | **CLOSED 2026-09-02** | ⚠️ decaduta |
| `#2358` countdown Ready | **CLOSED 2026-09-04 23:57** | ⚠️ decaduta |
| Semantic Area Overlay «sotto E49» | E49 = `#1769` *Tactical Camera*; la famiglia è `#1941`; `#1898` (CLOSED) esisteva proprio perché *«E49 significa due cose»* | ❌ eredita un'ambiguità già chiusa |
| `.mcp.json` dichiara `unreal-mcp` | vero: `http://127.0.0.1:8765/mcp` | ✅ dichiarato |
| entry point suite `./scripts/rt-suite.ps1` | esiste, 1328 righe | ✅ |
| «NON ESISTE ALCUN FOCUS da introdurre» | `Focus` non compare in `Source/RefactorTactics/UI/` in nessuna accezione di risorsa | ⚠️ divieto contro un fantasma |

**Lettura di Cockburn.** Due issue su nove sono chiuse, una delle due **31 ore prima** della revisione, e un
divieto è scritto contro una risorsa che non esiste. Il brief non porta sha né data. È il profilo di un
handoff corretto quando è stato scritto e decaduto subito: va **consumato**, non eseguito alla lettera.

---

## 3. Findings

### F-01 · CRITICO · I due deliverable del brief creano la seconda source of truth che il brief vieta

**Crispin.** Il brief chiede due matrici. Entrambe hanno già un owner, e il confine fra gli owner è
**scritto ed esplicito**.

`docs/technical/tooling/scenario-map.md` si dichiara `CURRENT` e **owner di una sola domanda**:

> *«per ogni cosa da verificare, chi la verifica — una macchina, un occhio umano, o nessuno dei due perché
> non esiste ancora»*

Ed espone in testa una tabella di confine a quattro documenti (`scenario-index-e-tag.md`,
`scenari-validazione-visiva.md`, `test-manuali-pie.md`, sé stesso), con la frase *«Non duplica nessuno dei
tre documenti che già esistono, e il confine è netto»*.

Ora si confrontino:

| Brief | Owner esistente | Collisione |
|---|---|---|
| §1.8 — matrice con `Tipo: AUTO \| MCP \| USER \| MCP→USER` | `scenario-map.md`, owner di *chi esegue cosa* | **la stessa domanda**, con un valore in più (`MCP`) |
| §7 — matrice evidenza con colonne `Expected` + `Status` | `test-manuali-pie.md`, *«il registro: esito atteso e stato»* | **lo stesso dato** |

E c'è di più: `docs/roadmap/editor-sessions.yaml` codifica una regola chiamata **R-6**:

> *«qui si citano gli ID delle voci PIE, mai il loro esito atteso. L'esito atteso vive in
> `test-manuali-pie.md`, che ne resta l'unico owner. Se ti trovi a scrivere qui cosa dovrebbe succedere,
> stai scrivendo nel file sbagliato.»*

La matrice §7 del brief, se scritta come proposta, è **esattamente** ciò che R-6 vieta — in un file diverso,
ma con la stessa conseguenza.

**Wiegers.** Un requisito la cui esecuzione fedele viola il vincolo dichiarato del documento stesso non è un
requisito ambiguo: è un requisito **contraddittorio**. Si risolve a monte, non in implementazione.

**Rimedio.** La matrice AUTO/USER **non si scrive**: si aggiorna `scenario-map.md`, che ha già l'asse, se
serve un valore `MCP`. Gli esiti attesi **non si scrivono qui**: vanno in `test-manuali-pie.md`. Questo
referto può contenere il *piano* e i *gap*, mai lo stato di verifica.

---

### F-02 · CRITICO · La Roadmap B non ha capability: il ponte MCP è giù, e non è di sola lettura

**Nygard.** Il brief costruisce metà del lavoro su un protocollo MCP e — correttamente — ordina un
capability probe prima di usarlo. Il probe è stato eseguito:

```
curl -m 4 http://127.0.0.1:8765/mcp   ->  HTTP 000 in 2.04s   (nessuna risposta)
Get-Process *Unreal*                  ->  nessun processo
```

Il server è **giù**. Con esso cade tutto ciò che il brief marca `MCP`: B0 baseline, B1 smoke, l'ispezione
widget tree di B2, la raccolta evidenza. Nessuna di quelle voci può essere `PASS`; sono `BLOCKED` — e il
brief ha ragione a pretendere che un'istruzione data all'utente non valga `PASS`.

⚠️ **Il rischio non è solo l'indisponibilità.** Questo ponte, quando è su, **scrive** — non è un ispettore
passivo — e non è garantito che stia guardando *questo* checkout. Un `B9 — Cold Start & Asset Integrity`
eseguito via MCP potrebbe salvare `.uasset` in un albero diverso da quello che si sta dichiarando verde. Il
brief non contempla questo modo di fallire: la sua §1.9 distingue MCP da USER sul **giudizio**, mai
sull'**ambito**.

**Rimedio.** Prima di qualunque uso: verificare a quale checkout il ponte è agganciato, e trattare ogni sua
scrittura come una modifica al working tree — con write-set dichiarato, come CLAUDE.md §5 pretende per gli
asset.

---

### F-03 · CRITICO · A2 non è un milestone di codice, ed è già pianificato altrove

**Fowler.** Il brief mette A2 *«Screen HUD Shell / ViewModel»* in `PRIORITÀ: ALTO` con gate automatici
(*«view model testabile senza UMG»*). Ma il piano già accettato per `#613` —
`docs/roadmap/plans/screen-hud-umg-2026-08-26.md` — dice che quel lavoro **è fatto**:

> *«chiudendo l'anello letto che oggi rende invisibile un view model **già scritto e già testato**»*

Ciò che manca sono **sei `WBP_RT_*`**, e lo stesso piano è esplicito su chi può produrli:

> *«I task 2–7 sono lavoro d'Editor su `.uasset`: **nessun agente li può fare**, e la loro verifica è a
> occhio dentro l'Editor.»*

Quindi A2 è già scomposto, già assegnato, e il suo collo di bottiglia è **umano e in Editor** — non
architetturale. Il gate `A2 → B2` del brief inverte la dipendenza reale: non è il codice che abilita la
verifica in Editor, è la seduta d'Editor che sblocca tutto il resto.

**Combinato con F-02** questo è il punto più bloccante dell'intero brief: l'unico strumento che avrebbe
potuto ridurre il lavoro d'Editor è proprio quello che oggi non risponde.

---

### F-04 · ALTO · «Predicted» significa già qualcosa, e non è quello che il brief intende

**Fowler.** Il brief §1.3 propone quattro assi e li battezza. L'asse B (*Certainty*) **esiste già**, è
canonico ed è testato: `ERTIntentCertainty` in `Source/RefactorTactics/Turn/RTIntentPrivacyLibrary.h`, con i
valori `Unknown = 0 · Confirmed · Predicted · Uncertain`.

Il problema è una **collisione di vocabolario su un asse diverso**:

| Termine | Nel brief | Nel codice |
|---|---|---|
| `Predicted` | asse **A** — *«outcome previsto dalla simulazione»* | asse **B** — livello di certezza: *«valido nello snapshot corrente: c'è un bersaglio, ma l'unità non si sposta»* |
| `Uncertain` | asse **B** — certezza | asse **B** — certezza ✅ |

Un contratto che usa `Predicted` per la **provenienza** mentre il dominio lo usa per la **certezza** produce
ViewModel in cui `Meaning: Predicted, Certainty: Confirmed` è scrivibile e significa qualcosa di diverso da
ciò che chiunque leggerà.

**Nygard — contro-evidenza da leggere prima di toccare quell'enum.** Il docstring di `ERTIntentCertainty`
documenta un difetto reale del 2026-08-16: lo zero dell'enum **era** `Confirmed`, cioè la garanzia più forte
del dominio, e un initializer C++ non bastava a proteggerlo (`Memzero`, `SetNumZeroed`,
`WithZeroConstructor` leggevano tutti «collegamento certo» su un campo mai calcolato). E il catalogo icone ne
pretende **tre** e non quattro, perché `Unknown` *«non ha una resa»*. Chi estende quell'asse eredita entrambi
i vincoli.

**Rimedio.** Non introdurre `Predicted` come valore di provenienza. E soprattutto: l'asse mancante ha già una
issue — **`#1941` OVL-01**, il cui titolo è *«cinque significati disegnati, zero modello che li dichiari»*,
che è letteralmente il problema di §1.3. **A1 non è un gap: è `#1941`.**

---

### F-05 · ALTO · `DrawHUD` non disegna soltanto: applica il velo, e l'invariante è un ordine

**Nygard.** Il brief tratta `#2184` come alleato (*«sta già estraendo decisioni di presentazione»*) e
costruisce A2/A7 sull'idea che spostare roba fuori da `DrawHUD` sia presentazione pura. La misura dice che
dentro quel ciclo passa una mutazione **critica per la privacy**, in `RTHUD.cpp` (~riga 519):

```cpp
const bool bIsKnownToObserver = ShouldDrawUnitOverlay(Entry, bIsOwnTeam);

// Applica lo stato di conoscenza PRIMA del filtro sottostante: altrimenti l'unita' saltata dal
// `continue` qui sotto non riceverebbe mai il comando e resterebbe visibile.
Unit->SetKnownToObserver(bIsKnownToObserver);
```

Le due **regole** (`ShouldDrawUnitOverlay`, `ContactGhostTargetForUnit`) sono già pure e testate senza
montare un HUD — quel lavoro è fatto. Ma l'invariante che protegge il velo **non è una funzione pura**: è un
**ordine di esecuzione** (*«PRIMA del filtro sottostante»*). Una migrazione che conserva le funzioni e perde
la posizione lascia un nemico visibile, e nessun test puro se ne accorge, perché le funzioni continuano a
rispondere giusto.

`#2184` lo sa e lo dichiara: *«`DrawHUD` non ha copertura headless, e non l'avrà… la rete è media, non densa:
13 file di test nominano `ARTHUD`, 8 `ScreenHud`. Va guardata prima di aprire ogni fetta.»*

**Rimedio.** Qualunque fetta che tocchi quel ciclo dichiari l'invariante d'ordine come criterio esplicito, e
lo verifichi in PIE — non solo con i test delle funzioni pure.

---

### F-06 · ALTO · «Da quale cella parte l'azione» ha due risposte, e il brief ne assume una

**Adzic.** §1.4 è il paragrafo migliore del brief: insiste che la preview segua la semantica reale delle fasi
e non una timeline UI inventata. Ha ragione. Ma poi comprime la domanda in una sola voce di checklist (B3:
*«Blast/azione parte dalla cella corretta rispetto al phase order»*), e quella voce non è decidibile.

Il repository ha **due** livelli di fase, entrambi canonici e **riconciliati esplicitamente** in
`Source/RefactorTactics/Ability/RTActionDef.h:17`:

| Livello | Enum | Valori |
|---|---|---|
| macro | `ERTMatchPhase` | `Planning · Prep · Dash · Blast · Move · Cleanup · MatchEnded` |
| micro | `ERTResolutionPhase` | `Snapshot · Preparation · FastMovement · NormalMovement · Control · Attack · Environment · Cleanup` |

E la riconciliazione dice la cosa che rompe la checklist:

> *«`FastMovement` (Dash, **PRIMA** del Blast) e `NormalMovement` (`Action.Move` → macro-fase Move, **DOPO**
> il Blast)»*

Quindi **due movimenti stanno a cavallo del Blast**. La cella d'origine di un attacco è:

- la cella **post-Dash**, se l'unità ha pianificato una mobilità rapida;
- la cella **pre-Move**, se ha pianificato un movimento normale.

*«La cella corretta»* non è una: è funzione di quale azione di movimento è nel piano. Un tester a cui si
chiede *«da dove partirà l'azione?»* (B3) può rispondere correttamente in due modi diversi, e il brief non
dice quale conta.

**Rimedio — nella forma che il brief stesso preferisce, l'esempio concreto:**

```
Dato    un'unità in A con piano [Dash A->B, Attack su T]
Quando  si ispeziona la preview della fase Blast
Allora  l'origine mostrata è B        (FastMovement ha già risolto)

Dato    un'unità in A con piano [Move A->B, Attack su T]
Quando  si ispeziona la preview della fase Blast
Allora  l'origine mostrata è A        (NormalMovement risolve DOPO)
```

Due esempi rendono falsificabile ciò che una riga di checklist lasciava all'intuito. **È la verifica più
preziosa dell'intero brief**, ed è quella che il brief stava per perdere.

---

### F-07 · MEDIO · La sequenza referto ↔ suite invalida la suite

**Crispin.** Il brief chiede (§2) di scrivere un referto sotto `docs/roadmap/plans/`, e (A9) di eseguire
`./scripts/rt-suite.ps1` e *«verificare che il verdetto sia valido»*. I due ordini interagiscono.

`rt-suite.ps1` calcola il suo digest d'albero come `git diff HEAD` **più gli untracked** via
`git hash-object --no-filters --stdin-paths` — cioè sul **contenuto dell'albero intero**, non sui soli
sorgenti. Un file nuovo in `docs/` che compare fra le due letture del digest fa cambiare l'albero **senza un
solo test fallito**, e il referto è `NON REGISTRABILE`.

**Questo file stesso è oggi untracked** e cambia il digest. Chi esegue la suite lo committi, o lo metta in
conto **prima** di misurare — non dopo.

**Nota di scoping.** `AGENTS.md` §9: *«Non esiste CI automatica per scelta corrente. Non introdurre CI,
package manager o nuovi build step senza una decisione esplicita.»* I «gate» della matrice A↔B restano quindi
**procedurali** — cose che una persona lancia a mano. Nessuna riga di questo brief autorizza a renderli
automatici.

---

### F-08 · MEDIO · Criteri non falsificabili

**Wiegers.** Alcuni requisiti non sono verificabili come scritti.

| Brief | Perché non è misurabile | Riscrittura |
|---|---|---|
| B2: *«centro della mappa libero»* | nessuna soglia, nessuna regione | *«nessun widget dello Screen HUD interseca il rettangolo centrale 60%×60% a 1920×1080»* |
| B8: *«nessun hitch evidente»* | «evidente» per chi | il brief lo ammette (*«non inventare una soglia FPS»*) → allora è **trend contro una baseline registrata**, non un `PASS` |
| B8: *«nessun Actor/Widget count che cresce senza tornare»* | «senza tornare» in quanto tempo | *«conteggio dopo 2 turni completi = conteggio dopo il primo, ±0»* |
| A8: *«niente Tick… se non misurato necessario»* | chi misura, con che metro | il brief ordina «prima misura» ma non dice cosa registrare |
| DoD: *«nessun golden/hash cambiato senza spiegazione»* | ✅ questo è buono, e va tenuto | — |

---

## 4. Cosa il brief prende giusto, e va conservato

Il panel è unanime: **queste parti valgono più della roadmap**, e sopravvivono alla revisione.

- **§1.2** — la gerarchia *simulazione decide / HUD mostra*, con la lista dei divieti (secondo pathfinder,
  seconda LOS, secondo targeting). È l'invariante #5 di `architettura-codice.md` detta bene.
- **§1.5** — i **tre** significati di «fin dove si vede» (corrente / da posizione pianificata / da posizione
  prevista) sono realmente distinti, e collassarli sarebbe un difetto. L'aggiunta *«un overlay LOS senza
  layer può mentire»* è esatta su mappa multilivello.
- **§1.6** — privacy **strutturale nel DTO**, non nascosta nel widget. Il dominio la applica già
  (`RefactorTactics.UI.NoEnemyIntentExposed` pinna due scene che differiscono SOLO per i piani nemici).
- **§1.7** — la **prima divergenza** con reason canonica, e il divieto di inventare la spiegazione nel widget
  se il reason code non esiste. È la richiesta più matura del documento.
- **§1.9** — MCP non giudica leggibilità. `test-manuali-pie.md` dice la stessa cosa con parole proprie: *«un
  log può dire "la cella evidenziata è (q,r,L)", non "la vedi"»*.
- **Le domande USER di B3 e B5** — porre la domanda *senza* aver prima spiegato la risposta, e trattare la
  confusione sistematica come `FAIL` di leggibilità anche a dati corretti. È un protocollo di test
  genuinamente buono, e riusabile ben oltre questo HUD.

---

## 5. Roadmap A, rimappata sugli owner reali

| Brief | Owner reale misurato | Gap? | Azione |
|---|---|---|---|
| A0 Audit | — | no | **fatto da questo referto** |
| A1 Contract / assi semantici | **`#1941` OVL-01** + `ERTIntentCertainty` (canonico, testato) | no | riusare `#1941`; **non** creare un enum di provenienza (F-04) |
| A2 Screen HUD / ViewModel | **`#613`** + piano `screen-hud-umg-2026-08-26.md` | no | ViewModel già fatto; restano 6 `.uasset` → **umano in Editor** (F-03) |
| A3 Planning preview per fase | **`#172`** CP 11.5 | no | aggiungere i due esempi di F-06 come criteri |
| A4 Visibility | **`#1712` CLOSED** (misurata, §10) + **`#1535`** OPEN per il velo | **sì, ristretto** | la primitiva e la ragione esistono; manca **solo** la resa runtime player-facing |
| A5 Divergence / explainability | `reaction-outcome-preview-handoff-spec-panel-2026-08-28.md` | da rimisurare | il territorio ha già un referto: leggerlo per primo |
| A6 Scrubbing | **`#173`** CP 11.6 | no | il brief stesso lo declassa; tenerlo declassato |
| A7 Pointer / cleanup | **`#705`** + **`#1859`** | no | ⚠️ vincolato da F-05 (invariante d'ordine del velo) |
| A8 Performance | **`#2184`** (E50) | no | `DrawHUD` è già misurato: 706 righe, 3ª funzione più costosa |
| A9 Suite | `scripts/rt-suite.ps1` | no | ⚠️ vedi F-07 sulla sequenza |

**Nessuna nuova issue è giustificata da questo referto.** Ogni requisito del brief ha trovato un owner vivo.
Il principio `SEARCH → REUSE → CREATE solo per gap reale` si ferma su `REUSE`.

---

## 6. Roadmap B — stato oggi

| Voce | Stato | Perché |
|---|---|---|
| B0 Baseline | `BLOCKED` | MCP giù, nessun processo Unreal (F-02) |
| B1 Smoke | `BLOCKED` | dipende da B0 |
| B2 Layout | `BLOCKED` | i 6 `WBP_RT_*` non esistono ancora (F-03) |
| B3 Preview | `NOT RUN` | eseguibile a mano via `rt.Test.Scenario <Id>` + Play |
| B4 LOS | `NOT RUN` | idem; rimisurare `#1712` prima |
| B5 Meaning | `NOT RUN` | il protocollo a domande è pronto e va usato |
| B6 Scrubbing | `N/A` | fuori scope corrente |
| B7 PC Gym | `NOT RUN` | owner `#1859`; **non duplicare il registro** |
| B8 Churn | `NOT RUN` | serve una baseline prima (F-08) |
| B9–B12 Final | `NOT RUN` | gate STOP-THE-LINE non raggiunto |

---

## 7. STOP conditions del brief effettivamente scattate

Il brief §9 elenca le condizioni per fermarsi. Ne sono scattate **tre**:

1. *«l'Editor è posseduto da un altro workflow»* → variante peggiore: **il ponte MCP non risponde**, e quando
   risponde non è garantito che guardi questo checkout (F-02);
2. *«due owner canonici si contraddicono»* → non fra loro, ma fra **il brief e gli owner**: §1.8 e §7
   riaprono domande già assegnate a `scenario-map.md` e `test-manuali-pie.md` (F-01);
3. *«serve modificare gameplay solo per far funzionare l'HUD»* → non scattata, ma **sfiorata** da F-04: un
   asse `Predicted` di provenienza spingerebbe a toccare un enum di dominio con due difetti storici
   documentati.

---

## 8. NOT RUN — dichiarazione esplicita

Nessuna di queste verifiche è stata eseguita, e **nessuna affermazione di questo referto ne dipende**:

- build `RefactorTacticsEditor` — non eseguita
- `./scripts/rt-suite.ps1` — non eseguita
- test Automation mirati — non eseguiti
- PIE / seduta Editor — non eseguita
- packaged / standalone — non eseguito
- MCP — **probe eseguito** (HTTP 000), nessuna operazione

Le conclusioni poggiano su: lettura di sorgenti, header di statuto dei documenti owner, stato issue via `gh`,
e un probe di rete.

---

## 9. Prossimo passo

**Uno solo**: aprire una console in PIE ed eseguire `rt.Debug.Los` su un muro noto, per scrivere la voce PIE
mancante (§10) sulla risposta reale del comando, non su un'aspettativa.

È il residuo dichiarato di `#1712`, costa cinque minuti, e chiude l'unico criterio che quella issue ha
lasciato aperto chiudendosi.

---

## 10. Addendum — cosa ha consegnato `#1712` (misurato 2026-09-05)

Misura richiesta dal §9 della prima stesura. Esito: **la premessa di A4 non era scaduta, era incompleta** —
ciò che manca è meno di quanto il brief assume, e in un punto solo.

### Consegna

| PR | Commit | Contenuto |
|---|---|---|
| `#1756` (2026-08-30) | `276d36f4` | primitiva della ragione in `Map/RTHexVisionLibrary.{h,cpp}` — +125 righe, +159 di test |
| `#2110` (2026-09-02) | `c77e0fd1` | `Map/RTHexLosConsole.{h,cpp}` + test — comando `rt.Debug.Los`, 338 righe |
| `#1755` (2026-08-30) | `869aac45` | `RTHexLosTool` + `RTHexLosReadout`, 714 righe — ma in `RefactorTacticsEditor/` |

### L'inversione, che è il pezzo importante

`HasLineOfSight` **non decide più da solo**: delega a `DescribeLineOfSight` e ne scarta la ragione.
L'autorità è diventata un caso degenere dell'osservabilità. La issue chiedeva una funzione pura *accanto*,
testata contro l'autorità su un corpus; l'implementazione ha invertito la dipendenza, e la divergenza è
**impossibile per costruzione** invece che improbabile e da sorvegliare.

∴ La STOP condition del brief *«un reason code necessario non esiste»* **non scatta** per la LOS.

### `FRTLineOfSightResult` è già la forma di §1.7

| Campo | Contenuto |
|---|---|
| `Block` | `None · EdgeBlocker · CellBlocker · InteriorGeometry` — **quattro** cause; `#1830` ha aggiunto la terza dopo la stesura della issue |
| `BlockedAt` | la cella in cui la linea stava **entrando** quando il blocco è scattato |
| `BlockedFrom` | la cella da cui si entrava — con `EdgeBlocker` il bordo colpevole è il lato `BlockedFrom → BlockedAt` |
| `StepIndex` | indice lungo `HexLine`; `INDEX_NONE` quando la vista passa |

`StepIndex` + `BlockedAt` **sono** la «prima divergenza» per il dominio LOS, già come dato.

### Il layer: la risposta canonica contraddice la formulazione del brief

Il brief chiede che l'overlay «dichiari il layer». Il codice spiega perché un campo dedicato sarebbe la
seconda verità:

> *«Il layer non ha un campo proprio, e non è una dimenticanza. `HexLine` tiene la linea sul layer del
> TIRATORE, quindi il layer su cui si sta ragionando è `From.Layer` — e lo portano anche `BlockedFrom` e
> `BlockedAt`. Un overlay che dichiara il layer lo legge da lì invece di riceverne una copia che potrebbe
> non coincidere.»*

Pinnato da `RefactorTactics.Debug.LosConsoleDeclaresLayer`.

### Il gap effettivo di A4

**Nessuna delle due superfici consegnate è runtime player-facing**:

- `rt.Debug.Los` **stampa**, per scelta scritta col costo dell'alternativa (`RTHexLosConsole.cpp:18`):
  *«Disegnare avrebbe voluto dire un sesto blocco dentro `ARTHexMapActor::DrawPlanningPreview`»*
  — funzione reale, `RTHexMapActor.cpp:1062`, oggi a cinque blocchi;
- `RTHexLosTool` è **modalità Editor**, strumento da tactical designer.

Ma il gap ha owner e canale: **`#1535`** (OPEN, `v0.1 · Percezione e reazioni`) per il velo in
presentazione, e un solo canale di disegno già esistente — `ARTHexMapActor::SetCellOverlayEnabled` /
`DrawCellOverlay`, lo stesso di `rt.Debug.DrawCells`.

E la regola di consumo è già decisa nel corpo di `#1712`:

> *«Un overlay che consuma `FRTTeamKnowledge::VisibleCells` **non può divergere** da ciò che il gioco crede:
> è lo stesso array. Un overlay che rifà la query può.»*

### Residuo

`rt.Debug.Los` **non ha voce PIE** (verificato: `grep` su `test-manuali-pie.md` non trova nulla). La DoD è
stata chiusa 8/9 con la motivazione scritta: il testo giusto dipende da cosa si vede *eseguendo* il comando,
e scriverlo prima produrrebbe *«una voce che descrive un'aspettativa invece di una verifica»* — la classe di
difetto che `PIE-GEO-CENTRO` aveva appena pagato con mezz'ora persa da chi l'ha eseguita.

⛔ **PIE e packaged su `#1712`: NOT RUN**, dichiarato dalla issue stessa alla chiusura.
