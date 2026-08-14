> 📦 `HISTORICAL` · **Sorgente archiviato il 2026-08-12** · **Revisionato, recepito in parte.**
>
> Il testo originale **non è stato riscritto**: quanto segue è l'esito della revisione. Referto completo:
> [`../../roadmap-plans/teleport-instant-movement-2026-08-12.md`](../../roadmap-plans/teleport-instant-movement-2026-08-12.md).
>
> | Esito | Sezioni |
> |---|---|
> | ✅ **Recepito** | §20 (la contraddizione, risolta: **nessuna `D-nnn` fissa l'assenza del Teleport** — è un fatto misurato, non una decisione) · §6 e §8 registrati come argomenti validi ma senza consumatore |
> | **Già canone** | §5, §13, §14, §15, §16 — e §13 in modo notevole: la policy `SameLayerOnly` che il kit *consiglia* è **già quella in vigore** (`LinearLeap` non cambia layer) |
> | ⚠️ **Premessa falsa** | La tesi «un movimento molto veloce non è un teletrasporto» è giusta; la premessa che nel repository esista solo il primo **no**: `ERTMovementStyle::LinearLeap` fa `Result.Entered = { destinazione }`, quindi `Action.Leap` **non prende gli hazard intermedi**. È già un trasferimento, archiviato sotto Dash |
> | ✂️ **Filtrato** | 6 scenari proposti → **uno solo**, e **esisteva già**: `Scenarios/Spec/Movement/TeleportSkipsIntermediateCells.json`, specifica eseguibile scritta prima dell'implementazione, con i 90 HP attesi dentro |
> | 🔁 **Diventa domanda** | `MOV-1` (famiglia o policy del Dash?) e `MOV-2` (un Blink entra in v0.1?) in `OPEN_DECISIONS.md` |
>
> ⚠️ **Ciò che il kit ha fatto emergere vale più di ciò che proponeva**: due righe della colonna Dash della
> matrice erano false di uno dei suoi quattro stili, e `Action.Leap` è la **quarta** capacità del motore
> irraggiungibile dal roster dopo le tre di [#425](https://github.com/DegrassiAaron/refactor-tactics-main/issues/425).

---

# RefactorTactics — Teletrasporto e movimenti istantanei
## Prompt operativo per Claude Code — consolidamento, issue audit, roadmap e specifiche

> Data handoff: 2026-08-12  
> Repository: `DegrassiAaron/refactor-tactics-main`  
> Scopo: consolidare il focus di design su **Teleport / Blink / Spatial Transfer** senza duplicare feature, issue, tassonomie o sistemi già presenti.

---

# 0. Regola operativa principale

**NON partire creando nuove issue. NON partire implementando un nuovo subsystem.**

Prima:

1. aggiorna `main`;
2. lavora in un worktree/branch dedicato;
3. leggi `CLAUDE.md`, `AGENTS.md`, `README.md` e gli indici/owner documentali;
4. misura lo stato reale di codice, test, documenti e GitHub;
5. cerca **issue aperte e chiuse**, PR e decisioni già esistenti;
6. identifica il delta reale;
7. modifica issue esistenti quando coprono già il lavoro;
8. crea una nuova issue **solo** per un delta che nessuna issue esistente possiede.

Non duplicare una feature solo perché viene chiamata con un nome diverso.

---

# 1. Audit GitHub obbligatorio PRIMA di qualsiasi nuova issue

Eseguire ricerche su issue **open + closed** e PR con almeno:

```text
Teleport
teletrasporto
Blink
Spatial Transfer
Instant Movement
Instantaneous Movement
Phase
Phase Dash
Reposition
Dash
Forced Movement
Reaction Movement
MovementStyle
ERTMovementStyle
Slip Between
movement taxonomy
tassonomia movimento
```

Per ogni risultato pertinente classificare:

```text
EXISTING-OPEN
EXISTING-CLOSED
PARTIAL-OVERLAP
SUPERSEDED
NOT-RELEVANT
MISSING
```

Prima di aprire una issue nuova, scrivere nel referto:

```text
Search performed:
...
Closest existing issues:
...
Why none owns this delta:
...
```

Se non è possibile spiegare in modo concreto perché le issue esistenti non coprono il lavoro, **non crearne una nuova**.

---

# 2. Snapshot già verificato da ChatGPT — da RIVERIFICARE live

Questo è uno snapshot del repository/GitHub al 2026-08-12. Non fidarti ciecamente: riconferma sul branch corrente.

## Owner di movimento già esistente

`docs/gameplay/spec-tassonomia-movimento.md` è già l'owner del confronto tra famiglie.

La specifica contiene già:

```text
Move
Dash
Forced
Teleport
Reaction
```

e dichiara esplicitamente:

```text
Teleport = comparsa senza attraversamento
Teleport NON ESISTE in v0.1
Reaction Movement non è una quinta meccanica:
usa la policy di una delle altre famiglie.
```

La matrice attuale dice già, per Teleport:

- nessun micro-step;
- nessuna cella intermedia attraversata;
- nessun MoveBudget;
- nessun costo terreno;
- collisione solo all'arrivo;
- nessun hazard intermedio;
- trigger spaziali solo all'arrivo;
- facing secondo policy dell'abilità;
- nessun rumore da passi.

**Quindi non creare una seconda “spec del movimento istantaneo” che ridica questa tabella.**
Estendere l'owner esistente oppure creare una spec subordinata soltanto se serve dettaglio che l'owner non deve possedere.

## Issue già rilevanti

### #307 — CLOSED
`La causa di uno spostamento non è leggibile nel TurnLog`

Ha già consolidato la distinzione di causa:

```text
Move / Dash / Forced / Reaction
```

e il TurnLog/replay.

Non riaprire o duplicare salvo regressione misurata.

### #308 — CLOSED
`Una spinta attraverso il fuoco genera davvero gli eventi di ogni cella attraversata?`

Ha già fissato Forced Movement:

```text
attraversa celle
applica hazard intermedi
non spende MoveBudget della vittima
```

Serve come contrasto diretto col Teleport, non come nuovo lavoro.

### #165 — OPEN
`CP 14.5 — Finestra, commit e cablaggio di Vektor.InterceptShot`

Possiede la prima Fast Reaction interattiva e il Decision Boundary:
3 s, FIRE/HOLD, replay, resume, stop del movimento.

Qualunque **Reactive Blink** deve coordinarsi con questa infrastruttura.
Non creare una seconda macchina di reaction o un secondo orchestratore.

### #159 — OPEN
`CP 13.4 — Rumore → contatto incerto, vista filtrata per squadra`

Possiede rumore → conoscenza team-filtered.
Se Teleport produce rumore di partenza/arrivo, deve riusare questo dominio.
Non creare un sistema parallelo di detection.

### #605 — OPEN
`CP 38.2 — Validazione del piano in Planning, con reason code deterministico`

Possiede il punto unico:

```text
LEGALE
oppure
ILLEGALE + reason code
```

prima del commit.

Una futura azione Teleport volontaria deve passare dallo stesso validatore.
Non creare un validator speciale per teleport.

### #606 / #609 / #641 — OPEN, post-v0.1
Riguardano profili di movimento, compatibilità azione↔movimento e migrazione dello Sprint.

Il Teleport non deve diventare accidentalmente un quinto “profilo” se semanticamente è una **famiglia di trasferimento**.
Verificare l'owner e gli enum reali prima di modificarli.

### #436 — OPEN, post-v0.1
`CP 36.1 · Tassonomia delle capability`

Sta decidendo la granularità delle capability di movimento.
Se Teleport deve essere bloccabile da `Root`, `Suppressed`, jammer o altre capability, coordinare qui invece di aggiungere una tassonomia parallela.

---

# 3. Decisioni di design emerse in questa sessione

Queste sono **proposte/decisioni d'autore della sessione** da confrontare con il canone corrente.
Se confliggono con una decisione già accettata, NON sovrascriverla in silenzio: aprire/aggiornare la relativa open decision e registrare la decisione corretta.

## 3.1 Distinzione fondamentale

```text
Move         = percorre spazio a micro-step
Dash         = percorre spazio con policy speciale
Phase Move   = percorre/valida uno spazio con eccezioni dichiarate
Teleport     = cambia posizione senza occupare le celle intermedie
```

Regola centrale:

> Un movimento molto veloce non è un teletrasporto.

---

# 4. Semantica proposta del Teleport

Un Teleport standard non genera `MoveStep` intermedi.

Flusso logico:

```text
ValidateTransfer
    ↓
TransferStarted
    ↓
LeaveOrigin
    ↓
Relocate
    ↓
EnterDestination
    ↓
Recompute:
    Occupancy
    LOS
    Visibility
    Cover
    Detection
    Reactions
    Objectives / zone control se applicabile
    ↓
TransferCompleted
```

Non deve esistere una falsa sequenza:

```text
A → B → C → D
```

se l'unità teletrasporta direttamente `A → D`.

Le celle B/C:

- non sono occupate;
- non applicano hazard;
- non generano trigger “crossed”;
- non producono passi;
- non consumano costo.

---

# 5. Overwatch e trigger spaziali

La tassonomia esistente distingue trigger geografici e semantici: conservarla.

Baseline proposta:

```text
Move attraversa Overwatch       → può triggerare
Dash attraversa Overwatch       → può triggerare
Forced attraversa Overwatch     → può triggerare se entra nell'area
Teleport "attraversa" il cono   → NO, perché non attraversa
Teleport ARRIVA nel cono        → può generare EnteredArea / Appeared
```

Quindi:

```text
A -------- [killzone] -------- B

Teleport A → B
```

non genera trigger lungo la linea geometrica A-B.

Se B è dentro la zona controllata:

```text
TransferCompleted
→ EnemyEnteredControlledArea
→ Reaction Opportunity
```

Verificare che ciò sia compatibile con ADR/brief di Overwatch e con #165.

---

# 6. Teleport ≠ Portal

Separare due concetti.

## Ability Teleport / Blink

È un trasferimento dell'unità:

```text
SourceCell → DestinationCell
```

senza arco attraversato.

## Portal / Teleporter della mappa

È topologia:

```text
Cell A
  ↕ special graph transition
Cell B
```

Un portal persistente o temporaneo può essere un **edge del grafo**, quindi:

- entra nella GraphRevision se cambia topologia;
- invalida cache pertinenti;
- viene considerato dal pathfinding quando legalmente attraversabile.

Non modellare un Blink personale come edge temporaneo del grafo.

---

# 7. Destinazione e Fog of War

Baseline prudente proposta per un primo Blink:

```text
Range corto
DestinationCell valida
Destination libera
Destination attualmente visibile all'utilizzatore
No blind teleport
```

Motivazione:

un blind teleport apre subito casi su:

- occupanti nascosti;
- muri/porte modificati;
- hazard sconosciuti;
- layer sconosciuti;
- leak di informazione.

Blind/Known-cell teleport è estensione futura, non default.

Se il canone corrente prescrive diversamente, registrare il conflitto.

---

# 8. Collisioni simultanee / occupazione

Caso:

```text
Unit A → X ← Unit B
```

con due teleport risolti nello stesso boundary.

Baseline proposta:

```text
ConflictPolicy = FailAll
```

cioè entrambi falliscono.

Motivo:

- preserva simultaneità;
- non dipende dall'ordine di iterazione;
- evita che un `TMap/TSet` decida il vincitore;
- è leggibile nel TurnLog.

Non hardcodare però una policy universale se l'architettura già ha un modello di conflitto riusabile.

Possibili policy future, data-driven solo se servono davvero:

```text
FailAll
PriorityWins
Swap
Displace
NearestFallback
```

Per il primo caso usare la minima superficie possibile.

---

# 9. Swap

Uno Swap deve essere **atomico**.

Non implementarlo come:

```text
Teleport A → B
Teleport B → A
```

perché l'occupazione intermedia produce un conflitto artificiale.

Concetto futuro:

```text
SpatialTransfer.Swap
```

validato come operazione unica.

Non implementare adesso se non esiste una issue/feature che lo richiede.

---

# 10. Recall / Anchor

Pattern futuro interessante:

```text
Place Anchor at A
Move/Fight
Recall → A
```

È forte perché la destinazione viene dichiarata prima e può diventare counterplay:

- anchor distruttibile;
- anchor bloccabile;
- zona controllabile;
- hazard sull'anchor;
- Overwatch sull'arrivo.

Da documentare come estensione futura, non scope automatico.

---

# 11. Forced Teleport

Tenere distinto da Push/Pull.

```text
Push/Pull:
percorre celle
collisioni intermedie
hazard intermedi

Forced Teleport:
source → destination
nessun percorso
solo validazione/effetti di arrivo
```

Può diventare una forte meccanica ambientale, ma richiede una policy esplicita su:

- target validi;
- occupazione;
- hazard d'arrivo;
- zone proibite;
- layer;
- boss/anchor resistance.

Non introdurla nel vertical slice solo per completezza.

---

# 12. Rumore

Riutilizzare il sistema rumore esistente.

Un teleport può produrre due eventi separati:

```text
DepartureNoise
ArrivalNoise
```

Mai “rumore lungo il percorso”.

Esempi data-driven futuri:

```text
Stealth Blink:
Departure = 0
Arrival   = 1

Violent Warp:
Departure = 4
Arrival   = 7
```

Il sistema di conoscenza deve ricevere solo gli eventi che la simulazione produce realmente e filtrare per squadra tramite il dominio di #159.

Nessun client nemico deve ricevere destinazioni future o intenti di teleport privati.

---

# 13. Multilivello

Non assumere automaticamente che un Teleport attraversi i Layer.

Verificare il modello reale di `FRTCellId`/grafo.

Separare almeno:

```text
SameLayerOnly
VisibleDestinationAcrossLayer
ExplicitLayerTransitionAllowed
```

Non serve necessariamente un enum se la prima implementazione ha una sola policy.

Per un primo Blink, preferire **SameLayerOnly** se il multilivello non è già stabile.

---

# 14. LOS e traiettoria

Pathfinding, LOS, Targeting e Trajectory sono già servizi distinti.

Il Teleport:

- non deve chiedere un path A* per simulare il transito;
- può chiedere Targeting per la cella candidata;
- può chiedere LOS/Visibility se la policy richiede destinazione visibile;
- deve ricalcolare LOS/cover dopo l'arrivo;
- non deve inventare una “trajectory” attraversata se il trasferimento non la possiede.

---

# 15. Data model: NON creare una seconda tassonomia a priori

La spec corrente dice che `ERTMovementStyle` possiede già le mobilità data-driven:

```text
Budget
LinearDash
LinearCharge
LinearLeap
LinearPass
...
```

Prima di introdurre:

```text
ERTSpatialTransferType
URTSpatialTransferService
FRTTeleportSpec
```

misurare se servono davvero.

La soluzione preferita è estendere i tipi/dati esistenti **se mantengono una semantica pulita**.

Creare un nuovo dominio soltanto se:

1. l'owner corrente non può esprimere la semantica senza branching per abilità;
2. esistono almeno due consumer reali;
3. il nuovo tipo elimina duplicazione concreta;
4. test e TurnLog possono osservarlo.

No “framework preventivo” senza consumer.

---

# 16. Eventi e TurnLog

Prima verificare il formato corrente e le decisioni su serializzazione/replay.

Il log deve consentire di distinguere almeno:

```text
movement family
movement cause
origin
destination
outcome
source action / reaction dove applicabile
```

Non aggiungere enum o campi duplicati se #307 ha già reso ricostruibile l'informazione.

Per il Teleport deve essere verificabile che:

- non esistono MoveStep intermedi;
- il trasferimento è distinguibile da Move/Dash/Forced;
- una collisione di arrivo ha reason code;
- replay e hash restano deterministici.

Se un nuovo valore entra in un enum serializzato, **aggiungerlo in coda** e trattare il cambio come migrazione di formato.

---

# 17. Scenario map / test richiesti

Prima cercare scenari equivalenti.

Se il Teleport entra davvero nel backlog, proporre il minimo set che dimostri la semantica, ad esempio:

```text
Spec.Movement.TeleportSkipsIntermediateHazard
Spec.Movement.TeleportDoesNotCrossOverwatch
Spec.Movement.TeleportArrivalCanTriggerZone
Spec.Movement.TeleportDestinationOccupiedFails
Spec.Movement.TeleportConflictIsPermutationInvariant
Spec.Movement.TeleportRecomputesLOSAfterArrival
```

Non creare tutti questi file se il relativo runtime/capability non esiste.

Nel Feature Registry possono essere `planned:` soltanto se questa è la convenzione corrente e il validator lo accetta.

Separare:

- test puri della libreria;
- scenario che prova il cablaggio reale;
- PIE solo per presentazione/VFX/UI.

---

# 18. UI/UX

Durante Planning, per un Blink:

- mostra origine;
- mostra esagoni di destinazione validi;
- **non** disegnare un path che sembri attraversato;
- distinguere chiaramente `Teleport destination` da `Move path`;
- warning solo con stato pubblico/proprio/team-only;
- se la destinazione è incerta/non valida, usare il vocabolario corrente `Confermato / Previsto / Incerto`.

Durante Resolution:

- VFX può mostrare dissolvenza/warp;
- la durata visiva non crea micro-step logici;
- il TurnLog è la sorgente della presentazione.

---

# 19. Scope consigliato

Non trasformare questo focus in una mega-feature.

## Primo incremento, se il repository decide di adottarlo

Uno solo:

```text
Short Blink
```

con:

- range corto;
- destinazione valida/libera;
- visibilità richiesta;
- nessuna cella intermedia;
- collisione solo all'arrivo;
- trigger di area solo all'arrivo;
- same-layer se il multilivello non è ancora stabile;
- deterministic conflict policy;
- TurnLog e test.

## Reactive Blink

Solo DOPO o INSIEME all'infrastruttura reale di #165, senza una seconda finestra di reaction.

## Futuro

```text
Swap
Recall / Anchor
Portal
Forced Teleport
Blind Teleport
Teleport blockers / jammer
Chain teleport
```

---

# 20. Contraddizione importante da NON nascondere

La conversazione propone di usare Blink/Reactive Blink come primo caso di test.

**Il canone corrente di `spec-tassonomia-movimento.md` dichiara però che Teleport NON ESISTE in v0.1.**

Questa è una vera decisione di scope, non una correzione editoriale.

Claude deve:

1. trovare se esiste una Decision `D-xxx` che fissa l'assenza dalla v0.1;
2. trovare se il roster/catalogo corrente ha già una skill che semanticamente è un teleport;
3. verificare se “Phase Dash / Reposition / EmergencyDash” sono traversal oppure teleport;
4. NON rinominare una skill per farla combaciare;
5. se l'autore vuole davvero portare un Blink in v0.1, aprire/aggiornare una **decisione di scope** prima dell'implementazione;
6. se non c'è questa decisione, mantenere Teleport come **post-v0.1 design-ready**.

---

# 21. Deliverable documentale

Produrre un referto in:

```text
docs/roadmap/plans/
```

con nome coerente con le convenzioni correnti, ad esempio:

```text
teleport-instant-movement-spec-panel-2026-08-12.md
```

Il referto deve avere:

1. sorgenti lette;
2. stato reale del codice;
3. stato issue/PR;
4. decisioni già canoniche;
5. nuove proposte;
6. conflitti;
7. delta reale;
8. issue riutilizzate;
9. eventuali issue create;
10. scenari esistenti/nuovi;
11. file aggiornati;
12. comandi di validazione eseguiti;
13. cosa NON è stato fatto e perché.

---

# 22. Aggiornamenti repository

Aggiornare **solo gli owner corretti**.

Verificare almeno:

```text
docs/gameplay/spec-tassonomia-movimento.md
docs/gameplay/spec-dash.md
docs/gameplay/brief-azioni-generiche-overwatch.md
docs/decisions/adr-0004-finestre-di-reazione.md
docs/technical/spec-turnlog.md
docs/roadmap/feature-registry.yaml
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-post-v0.1.md
docs/technical/scenario-map.md
Wiki pertinente
docs/OPEN_DECISIONS.md
docs/decisions/RT_PDR_00_Decision_Log.md
```

**Non significa modificarli tutti.**
Significa controllare quali sono owner/consumer e cambiare solo quelli che devono cambiare.

### CLAUDE.md / AGENTS.md / README.md

Aggiornarli **solo** se emerge una nuova regola globale del repository o un nuovo entrypoint operativo.
Non duplicare una regola di gameplay dentro i file di istruzioni agli agenti.

### Feature map / milestone map / scenario map / editor map

Seguire la pipeline corrente:

- editare la sorgente canonica;
- rigenerare le viste generate;
- non modificare manualmente file marcati `GENERATED`.

### Workbook / Excel

Non aggiornare il workbook di bilanciamento “per completezza”.
Verificare la ownership corrente (`docs/balance/README.md`, decisioni D-023/D-106 o successive).
Se il workbook è research/derived, non usarlo come canone e non scriverci una regola nuova.

### `.umap`

Nessuna modifica CLI a `.umap`.
Se serve una verifica visuale/editoriale, registrarla come seduta/PIE secondo la convenzione del repository.

---

# 23. Gestione issue

Per ciascun delta:

## Se un'issue esistente lo copre

Aggiorna:

- body;
- DoD;
- riferimenti owner;
- dipendenze;
- scenari;
- note sulla nuova semantica.

Non creare un duplicato.

## Se un'issue chiusa lo ha già consegnato

Non riaprirla per aggiungere scope nuovo.
Citala come prerequisito/precedente.

## Se serve davvero una nuova issue

La issue deve dichiarare:

```text
Why this is not #307 / #308 / #165 / #159 / #605 / #606 / #436
Owner spec
Feature ID
Release target
Dependencies
Scope
Out of scope
DoD misurabile
Automation tests
Scenario
Replay/TurnLog impact
Privacy impact
```

Se appartiene a un'epic esistente, collegarla come sub-issue secondo la convenzione GitHub già usata.

Non creare una nuova epic se un'epic esistente possiede già il dominio.

---

# 24. Acceptance criteria del consolidamento

Il lavoro è finito solo se:

- [ ] la parola `Teleport` ha un solo significato canonico;
- [ ] Blink ≠ Dash è scritto nell'owner corretto;
- [ ] traversal e transfer non vengono confusi nel resolver;
- [ ] nessun nuovo subsystem duplica `ERTMovementStyle`, reaction infrastructure, noise o plan validation senza prova;
- [ ] issue open/closed sono state auditate prima di crearne altre;
- [ ] le issue esistenti pertinenti sono citate e aggiornate dove necessario;
- [ ] eventuali nuove issue descrivono solo delta non posseduti;
- [ ] roadmap/registry/scenario map sono allineati;
- [ ] nessuna vista generata è stata editata a mano;
- [ ] replay/TurnLog e determinismo sono coperti;
- [ ] privacy degli intenti resta intatta;
- [ ] la contraddizione “Teleport assente v0.1 vs eventuale Blink v0.1” è esplicitamente risolta o resta aperta, non nascosta;
- [ ] suite e validator pertinenti sono verdi.

---

# 25. Comandi/gate finali

Usare i comandi reali del repository; almeno verificare gli equivalenti correnti di:

```text
python scripts/check-docs-links.py
python scripts/check-docs-symbols.py
python scripts/feature_registry.py validate
python scripts/feature_registry.py generate --check
python scripts/feature_registry.py shortlist --check
```

Se sono stati modificati file `Source/`:

- build Development Editor;
- test area mirata;
- suite completa se richiesta dal rischio;
- mutation test sui punti di determinismo/conflitto;
- scenario runner se è stato aggiunto uno scenario.

Se cambiano log serializzati/replay:
- verificare versioning;
- dichiarare eventuale rebaseline;
- non aggiornare golden “per farli tornare verdi” senza spiegare il perché.

---

# 26. Output finale richiesto a Claude

Chiudi il lavoro con una risposta breve ma misurata:

```text
1. Stato trovato
2. Decisioni già esistenti
3. Cosa ho cambiato
4. Issue riusate/modificate
5. Nuove issue create (se davvero necessarie)
6. Issue deliberatamente NON create
7. File aggiornati
8. Test/gate eseguiti
9. Conflitti/open decisions rimasti
10. NEXT ISSUE consigliata
```

La `NEXT ISSUE` deve essere una issue **esistente** se ce n'è una che rappresenta il prossimo passo.
Crearne una nuova solo se il delta non ha davvero un owner.

---

# Principio finale

> Teleport non è “un Dash che salta i trigger”.  
> È una famiglia semantica diversa: **nessun attraversamento, solo partenza e arrivo**.

Ma l'obiettivo di questo task non è inventare un nuovo framework: è fare in modo che questa semantica entri nel **sistema che RefactorTactics ha già**, con una sola fonte di verità, issue non duplicate, test osservabili e roadmap coerente.
