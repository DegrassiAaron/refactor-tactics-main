# CLAUDE — RefactorTactics · Animation Issue Orchestrator

> ## 📸 `HISTORICAL` — SORGENTE CONSUMATO, NON NORMATIVO
>
> Work order arrivato in radice come `CLAUDE_RefactorTactics_Animation_Issue_Orchestrator.md`, **untracked**,
> e consumato il **2026-08-30** dal referto
> [`../../../roadmap/plans/animazioni-paragon-issue-orchestrator-spec-panel-2026-08-30.md`](../../../roadmap/plans/animazioni-paragon-issue-orchestrator-spec-panel-2026-08-30.md).
>
> **Non è una fonte, e la sua premessa centrale era già scaduta di cinque giorni**: chiede i quattro
> `ABP_Gadget`/`ABP_Phase`/`ABP_Riktor`/`ABP_Wraith`, che è la via misurata e scartata da #288 il 2026-08-25
> spostando il grafo di locomozione in C++ (`URTUnitAnimInstance`). Conservato per la **provenienza** — e
> perché la sua §7 e la sua §19 sono la ragione per cui la risposta a «serve una nuova Epic?» è **no**. Ciò
> che di normativo sopravvive sta nel §9 del referto.

## Missione

Lavori sul repository **RefactorTactics** e devi organizzare correttamente il lavoro GitHub necessario per integrare e configurare le animazioni dei personaggi **Paragon** nella v0.1.

Il tuo compito NON è creare issue per simmetria o riscrivere la roadmap da zero.

Devi:

1. misurare lo stato reale del repository e delle issue GitHub;
2. verificare cosa è già posseduto da Epic / checkpoint / issue esistenti;
3. aggiornare le issue esistenti quando sono l'owner corretto;
4. creare nuove issue solo per lavoro realmente separato e verificabile;
5. collegare le nuove issue alle Epic esistenti;
6. creare una nuova Epic SOLO se nessuna Epic corrente possiede correttamente un insieme coerente di lavoro;
7. mantenere allineati roadmap, issue e stato reale del codice/asset;
8. usare Unreal MCP quando serve per verificare asset e configurazione reale nell'Editor.

Non iniziare modificando asset o gameplay. Prima sistema ownership e tracking.

---

# 1. Preflight obbligatorio

Prima di modificare issue:

- leggi `AGENTS.md`;
- leggi `CLAUDE.md`;
- verifica branch e HEAD correnti;
- verifica lo stato della roadmap v0.1;
- leggi il Decision Log pertinente;
- cerca issue ed Epic esistenti sul tema animazioni / Paragon / presentation / locomotion / montage / cook / packaging;
- verifica lo stato reale del codice e degli asset;
- se Unreal Editor è aperto e MCP è disponibile, usa Unreal MCP per l'audit degli asset.

NON fidarti di numeri, path o stati copiati in questo file se GitHub o il repository dicono qualcosa di diverso.

Questo file descrive il PROCESSO, non è una fonte normativa del gameplay.

---

# 2. Punto di partenza da riverificare

Al momento della stesura esistono almeno questi candidati ownership:

- **#286 — `[EPIC v0.1] E21 — Presentazione e leggibilità`**
- **#288 — `CP E21.2 — Animazioni di locomozione e impatto`**
- **#1663 — packaged: asset Paragon presenti ma clip animazione mancanti dal cook**

NON assumere che siano ancora aperti, completi o corretti.

Aprili e leggi:

- body;
- commenti;
- stato;
- dipendenze;
- Acceptance Criteria;
- issue collegate;
- PR collegate;
- riferimenti alla roadmap.

Cerca inoltre issue più recenti che possano averne superseduto una parte.

---

# 3. Scope funzionale da coprire

Roster v0.1 / pack Paragon:

- Gadget
- Phase
- Riktor
- Wraith

L'obiettivo complessivo della track animazioni deve coprire, se realmente necessario:

## Asset / integrazione

- Skeletal Mesh corretta;
- Skeleton corretto;
- Animation Sequence esistenti;
- Blend Space esistenti;
- Animation Montage esistenti;
- Animation Blueprint originali Paragon;
- Animation Blueprint RefactorTactics;
- assegnazione `AnimClass` alle unità;
- eventuali Data Asset / profili di presentation;
- dipendenze fra asset.

## Locomotion

- Idle;
- Walk/Jog;
- Run;
- Forward;
- Backward;
- Strafe;
- facing coerente sulle sei direzioni hex;
- start/stop se necessario;
- ritorno a Idle a fine movimento.

## Azioni discrete

- Dodge / Evade;
- Basic Attack;
- Cast / Ability;
- Hit Reaction;
- Stagger / Knockback;
- Death / KO;
- eventuali pose Prepared (Guard / Brace / Overwatch) se già richieste dalla v0.1.

## Playback

La Presentation deve consumare gli eventi autorevoli già prodotti dalla simulazione.

Vincolo:

`Resolver / TurnLog / eventi autorevoli -> Presentation -> Animation`

L'animazione NON decide:

- hit;
- danno;
- posizione finale;
- eliminazione;
- ordine delle azioni;
- cooldown;
- esito di una reazione.

AnimNotify, durata di Montage, Root Motion e timing visuale NON diventano autorità gameplay.

## Packaging

Deve essere verificato che il subset di animazioni richiesto dalla v0.1 venga incluso nel cook/package.

NON cuocere automaticamente tutto `Content/FabAsset/Paragon`.

La soluzione deve dichiarare esplicitamente:

- quali asset entrano nel cook;
- perché;
- come vengono referenziati;
- comportamento del clone che non possiede i pack Paragon;
- comportamento del packaged build.

---

# 4. Audit GitHub

Cerca almeno con questi concetti:

- animation
- animations
- AnimBP
- Paragon
- locomotion
- montage
- hit reaction
- death
- dodge
- presentation
- E21
- packaged
- cook
- FabAsset

Per ogni risultato rilevante costruisci internamente questa matrice:

| Issue | Tipo | Owner reale | Stato | Scope | Overlap | Azione |
|---|---|---|---|---|---|---|
| #... | Epic / CP / Bug / Task | ... | open/closed | ... | ... | KEEP / UPDATE / LINK / CLOSE DUPLICATE / SUPERSEDE |

NON creare nuove issue prima di aver completato questa matrice.

---

# 5. Audit Unreal MCP

Se Unreal MCP è disponibile, fai inizialmente un audit READ-ONLY.

Per ciascun pack:

- Gadget
- Phase
- Riktor
- Wraith

verifica:

1. Skeletal Mesh principale;
2. Skeleton;
3. AnimBlueprint;
4. AnimationSequence;
5. BlendSpace;
6. AnimMontage;
7. Root Motion;
8. dipendenze dagli asset/classi Paragon;
9. asset RT già esistenti;
10. reference mancanti.

Path RT attesi da verificare, non da assumere:

- `/Game/RT/Characters/Gadget/`
- `/Game/RT/Characters/Phase/`
- `/Game/RT/Characters/Riktor/`
- `/Game/RT/Characters/Wraith/`

Asset candidati:

- `BP_Unit_Gadget`
- `BP_Unit_Phase`
- `BP_Unit_Riktor`
- `BP_Unit_Wraith`
- `ABP_Gadget`
- `ABP_Phase`
- `ABP_Riktor`
- `ABP_Wraith`

Classifica le capability:

- READY
- CONFIGURATION ONLY
- CODE REQUIRED
- ASSET MISSING
- MANUAL ART REVIEW

Non modificare asset durante questa fase.

---

# 6. Regola di ownership

Per ogni pezzo di lavoro chiediti in quest'ordine:

### A. Esiste già una issue che possiede esattamente questo lavoro?

Se sì:

- aggiorna quella issue;
- non crearne una nuova.

### B. Esiste una issue più ampia che può essere estesa senza diventare incoerente?

Se sì:

- aggiorna quella issue;
- aggiungi Acceptance Criteria e collegamenti mancanti.

### C. Il lavoro è indipendente, ha un proprio test/gate e può essere completato separatamente?

Se sì:

- crea una nuova issue figlia;
- collegala all'Epic corretta.

### D. Nessuna Epic esistente possiede correttamente l'intero gruppo di lavoro?

Solo in questo caso valuta una nuova Epic.

---

# 7. Quando NON creare una nuova Epic

NON creare una nuova Epic se:

- #286 / E21 possiede già Presentation e animazioni;
- il problema è soltanto un checkpoint mancante;
- il problema è soltanto packaging/cook;
- il lavoro è una singola integrazione AnimBP;
- il lavoro è un bug;
- il lavoro è una verifica PIE;
- il lavoro è una verifica packaged;
- il lavoro è configurazione MCP;
- esiste già una Epic semantica corretta anche se il body va aggiornato.

In questi casi aggiorna/estendi l'Epic esistente.

---

# 8. Quando creare una nuova Epic

Crea una nuova Epic SOLO se tutte queste condizioni sono vere:

1. il lavoro contiene più task/CP indipendenti;
2. nessuna Epic corrente lo possiede semanticamente;
3. il lavoro ha un outcome di prodotto riconoscibile;
4. il lavoro attraversa più subsystem o milestone in modo coerente;
5. aggiungerlo a una Epic esistente ne romperebbe chiaramente lo scope;
6. hai cercato GitHub per evitare duplicati;
7. puoi definire un gate di chiusura falsificabile.

Prima di crearla, nel tuo report scrivi esplicitamente:

`NEW EPIC REQUIRED: YES`

seguito da:

- perché le Epic esistenti non bastano;
- quali issue diventeranno figlie;
- quale milestone/release possiede;
- gate di chiusura.

Se una di queste informazioni manca, NON creare l'Epic.

---

# 9. Candidati naturali a issue separate

Valuta questi gruppi, ma NON crearli automaticamente:

## A — Paragon Animation Audit

Solo se nessuna issue corrente possiede l'inventario/mapping delle clip.

Possibile outcome:

- matrice verificata `RT semantic -> Paragon asset` per i quattro eroi.

## B — AnimBP locomotion

Solo se #288 non copre già adeguatamente questo lavoro.

Possibile outcome:

- `ABP_*` con Idle / locomotion / facing verificati.

## C — Playback Blast

Possibile scope:

- attack;
- cast;
- hit;
- stagger;
- death.

Separalo solo se il codice/playback è abbastanza indipendente dalla locomotion da meritare un proprio gate.

## D — Dodge / movement presentation

Crea una issue separata solo se Dodge non è già incluso nel checkpoint corrente e richiede un proprio playback/mapping/test.

## E — Cook subset Paragon

Probabilmente deve aggiornare/estendere **#1663**, non duplicarlo.

## F — Verification

Evita una issue generica "test animations" se i test possono stare nei DoD delle issue che implementano la feature.

Crea una issue di validation solo se esiste una sessione/manual gate indipendente e sostanziale.

---

# 10. Come aggiornare una Epic esistente

Se #286 è ancora l'owner corretto, assicurati che il body dichiari chiaramente:

- ownership delle animazioni v0.1;
- link a checkpoint e bug attivi;
- dipendenze;
- stato dei checkpoint;
- gate PIE;
- gate packaged se necessario;
- relazione fra Presentation e simulazione;
- quali parti sono automatiche e quali richiedono giudizio umano.

Non riscrivere la storia inutilmente.

Preferisci modifiche additive e correzioni precise.

---

# 11. Template nuova issue

Ogni nuova issue deve usare questa struttura minima:

```md
## Why

Problema reale misurato e impatto sulla v0.1.

## Ownership

Epic: #...
Parent/checkpoint: #... se applicabile
Roadmap: ...
Decisioni: ...

## Stato misurato

- codice:
- asset:
- Editor/MCP:
- PIE:
- packaged:

## Scope

- ...

## Out of scope

- ...

## Implementazione prevista

- ...

## Asset interessati

- ...

## Acceptance Criteria

- [ ] criterio falsificabile 1
- [ ] criterio falsificabile 2
- [ ] criterio falsificabile 3

## Test automatici

- nome test previsto o `N/A` con motivazione

## Verifica PIE

- scenario / passaggi / risultato atteso

## Verifica packaged

- se applicabile

## Dipendenze

- #...

## Rischi

- ...
```

---

# 12. Template nuova Epic

Solo se realmente necessaria:

```md
# [EPIC <release>] <titolo>

## Outcome

Una frase player/developer-facing che definisce quando l'Epic è conclusa.

## Perché esiste

Gap non coperto dalle Epic correnti.

## Scope

- ...

## Fuori scope

- ...

## Issue / checkpoint

- [ ] #...
- [ ] #...
- [ ] #...

## Dipendenze

- ...

## Gate di chiusura

- automation;
- PIE;
- packaged;
- eventuale review visiva.

## Rischi

- ...
```

---

# 13. Collegamenti GitHub

Quando crei o aggiorni issue:

- linka sempre l'Epic nel body;
- linka issue parent/related;
- usa `Refs #...`, `Depends on #...`, `Blocks #...` solo quando semanticamente corretto;
- non dichiarare dipendenze per semplice affinità tematica;
- evita grafi circolari;
- se una issue è duplicata, collega l'owner e chiudi come duplicate solo dopo aver trasferito eventuali informazioni uniche;
- non perdere misure, log o Acceptance Criteria presenti nell'issue duplicata.

---

# 14. Stati e chiusure

Non chiudere un'issue perché "il codice sembra esserci".

Prima della chiusura verifica i gate dichiarati nel suo DoD.

Per animazioni/presentation considera almeno:

- Automation Test quando possibile;
- compilazione Editor;
- Blueprint compile senza errori;
- PIE;
- verifica visiva quando richiesta;
- packaged build quando riguarda cook/reference/asset;
- assenza di `SkipPackage` rilevanti;
- comportamento corretto senza asset Paragon nel clone, se previsto dal progetto.

---

# 15. Uso dell'Unreal MCP durante l'esecuzione successiva

Dopo aver sistemato le issue, il lavoro potrà essere eseguito tramite Unreal MCP.

Preferisci i tool Epic esistenti per:

- Asset Registry;
- search asset;
- object inspection;
- Blueprint inspection;
- Blueprint creation/configuration;
- asset duplication;
- property editing;
- compile;
- save;
- validation;
- Automation Tests.

Se un'operazione fondamentale non è coperta:

1. verifica davvero i toolset disponibili;
2. non assumere che manchi;
3. solo dopo proponi un'estensione minima di `RTDeveloperTools` tramite `UToolsetDefinition`;
4. non introdurre Python se non esiste una ragione nuova e verificata.

---

# 16. Regole per gli asset Paragon

Gli asset originali Paragon sono sorgenti di presentation.

Non modificarli direttamente salvo decisione esplicita.

Preferisci:

`Paragon asset read-only -> RT asset/configuration -> RT gameplay unit`

Gli ID e le regole gameplay RefactorTactics non devono dipendere dai nomi tecnici delle clip Paragon.

Esempio concettuale:

`RT.Action.Dodge -> profile RT -> specific Paragon animation`

non:

`Gameplay rule -> Gadget_Dodge_Fwd_Anim`

---

# 17. Output richiesto PRIMA delle mutazioni GitHub

Prima di creare o modificare issue, produci un report sintetico:

## Existing ownership

- Epic trovate;
- checkpoint trovati;
- bug/task trovati.

## Duplicazioni

- issue sovrapposte;
- owner raccomandato.

## Gap reali

- lavoro senza issue.

## Proposed GitHub changes

Per ogni modifica:

| Tipo | Issue | Azione | Motivazione |
|---|---|---|---|
| UPDATE | #... | ... | ... |
| CREATE | NEW | ... | ... |
| LINK | #... -> #... | ... | ... |
| CLOSE DUPLICATE | #... | owner #... | ... |

## Epic decision

Scrivi una delle due:

`NEW EPIC REQUIRED: NO`

oppure

`NEW EPIC REQUIRED: YES`

con motivazione.

Dopo questo report, procedi autonomamente con le mutazioni GitHub se non emergono conflitti o decisioni di prodotto non risolvibili dai dati.

Non chiedere conferma per semplici operazioni di tracking quando l'ownership è chiara.

Fermati e segnala invece una decisione d'autore quando esistono due strutture entrambe valide ma con significato di prodotto diverso.

---

# 18. Output finale

Al termine restituisci:

1. Epic utilizzate;
2. Epic create, se presenti;
3. issue aggiornate;
4. issue create;
5. issue collegate;
6. issue chiuse come duplicate/superseded;
7. lista ordinata di esecuzione;
8. blocker rimasti;
9. prossimo task concreto da eseguire con Unreal MCP;
10. URL GitHub di tutte le issue toccate.

Il risultato deve lasciare GitHub in uno stato in cui una persona nuova possa capire:

- chi possiede il lavoro;
- cosa manca;
- in quale ordine farlo;
- come verificarlo;
- quando è Done.

---

# 19. Principio finale

Non ottimizzare per il numero di issue.

Ottimizza per:

- ownership chiara;
- niente duplicati;
- scope verificabile;
- dipendenze leggibili;
- outcome misurabili;
- tracciabilità Epic -> issue -> test -> asset -> packaged build.

Una issue in meno con ownership chiara è migliore di cinque issue create per completezza apparente.
