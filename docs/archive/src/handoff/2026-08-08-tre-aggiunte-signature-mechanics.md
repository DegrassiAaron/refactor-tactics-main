# TASK — Consolidare tre estensioni al framework delle Signature Mechanics

> `HISTORICAL` · **Handoff eseguito il 2026-08-08** · **Sorgente non normativo**
>
> | Aggiunta | Esito | Dove vive ora |
> |---|---|---|
> | `Misplay / Failure State` | ✅ **adottata** — era davvero assente | Campo dello schema in [`../../../characters/_Template.md`](../../../characters/_Template.md), compilato sulle 4 schede v0.1 · [D-032](../../../decisions/RT_PDR_00_Decision_Log.md) |
> | `GenericActionModifier` | ⛔ **respinta come nome, adottata come concetto** — il repository lo chiama già **profilo** | [`../../../gameplay/brief-azioni-generiche-overwatch.md`](../../../gameplay/brief-azioni-generiche-overwatch.md) §4-bis · [D-033](../../../decisions/RT_PDR_00_Decision_Log.md) |
> | `ConditionalIntent` | 📅 **post-v0.1**, come chiedeva il documento | [`../../../gameplay/brief-delayed-actions.md`](../../../gameplay/brief-delayed-actions.md) §7 · [D-034](../../../decisions/RT_PDR_00_Decision_Log.md) · epic **E33** |
>
> **Autorità**: nessuna. Un handoff non è una fonte e non autorizza da solo a implementare ciò che contiene.

## Correzioni applicate il 2026-08-08

Testo originale **intatto** (convenzione 4 di [`../../../src/README.md`](../../../src/README.md)). Errori di fatto rilevati
eseguendo la task:

| § | Affermazione originale | Correzione |
|---|---|---|
| 4 | Cerca in `docs/design/`, `docs/actions/`, `docs/adr/` | **Nessuno dei tre esiste.** I path reali sono `docs/gameplay/` (regole), `docs/decisions/` (ADR + Decision Log), `docs/balance/` (numeri), `docs/characters/` (schede). Il documento lo prevede — «i path reali prevalgono» — ma l'elenco induceva a creare cartelle nuove |
| 5 | La wiki è nel repository separato `refactor-tactics-main.wiki` | **Vero ma incompleto**: quel repository esiste ed è la wiki *pubblicata*. Il **sorgente** è in-repo, in [`../../../wiki/`](../../../wiki/), e le due non vanno modificate in parallelo. Questo consolidamento ha toccato **solo** il sorgente; la pubblicazione è un passo separato e non è stata eseguita |
| 2 | Propone `GenericActionModifier` come nome nuovo | Il repository ha già il concetto e il nome: **profilo**. `MoveProfile = Sneak \| Normal \| Sprint` ([D-015](../../../decisions/RT_PDR_00_Decision_Log.md)) e «Overwatch è universale, il **profilo** dipende dall'eroe» ([D-014](../../../decisions/RT_PDR_00_Decision_Log.md)). Il documento stesso impone la regola che lo esclude: «*salvo che il repository abbia già un nome migliore*», «*NON duplicarlo*» |
| 1 | Presenta `ConditionalIntent` come concetto senza precedenti | Ha già un precursore **deciso**: il regime `Conditional` dell'Overwatch, cioè «`AllowedResponses ≥ 2` **+ condizione dichiarata in planning**» ([D-012](../../../decisions/RT_PDR_00_Decision_Log.md)). Non è un sistema nuovo da inventare: è la stessa condizione spostata dal profilo di reazione all'intento |
| 6 | «Roster corrente» senza definirlo | Il roster attivo è a tre livelli: **v0.1 canonico** (Flux · Riva · Bastion · Vektor), **v0.2** (Aurora · Kwang · Murdock · Steel) e **34 candidati** Paragon. L'audit dei gap è stato fatto sul solo v0.1, come chiede §6 («dare precedenza al roster realmente attivo») |
| 8 | Ipotizza «ADR solo se cambia il modello runtime» | Confermato: **nessun ADR**. Nessuna delle tre aggiunte cambia l'architettura — due sono schema documentale, la terza è rinviata. Sono quattro voci di Decision Log, che è la forma prevista dal documento stesso |

Stai lavorando sul progetto **RefactorTactics**.

Esistono due repository/cartelle distinte da considerare:

- repository principale: `refactor-tactics-main`
- wiki: `refactor-tactics-main.wiki`

Obiettivo: consolidare nella documentazione di progetto tre aggiunte al framework di caratterizzazione dei personaggi, senza introdurre implementazioni premature e senza duplicare concetti già presenti.

Le tre aggiunte sono:

1. `ConditionalIntent`
2. `GenericActionModifier`
3. `Misplay / Failure State`

---

# 0. Regole di lavoro

Prima di modificare qualsiasi file:

1. leggi `CLAUDE.md`, `AGENTS.md` e qualunque file di istruzioni presente nelle due repository;
2. analizza la documentazione corrente relativa a:
   - character design;
   - roster;
   - signature mechanics;
   - generic actions;
   - planning/intents;
   - action economy;
   - reactions;
   - delayed/predictive actions;
   - decision boundaries;
   - simulator/resolver;
   - TurnLog;
   - wiki dei personaggi;
3. cerca esplicitamente se i tre concetti esistono già sotto altri nomi;
4. identifica la fonte canonica più recente;
5. segnala conflitti o sovrapposizioni prima di introdurre nuove definizioni;
6. non creare documentazione parallela se è sufficiente aggiornare quella esistente;
7. non modificare codice salvo che sia strettamente necessario per mantenere documentazione o schema coerente. Questa task è principalmente di **consolidamento design/documentazione**;
8. non inventare API Unreal o strutture runtime che non esistono nel progetto;
9. preserva le decisioni già consolidate:
   - simulazione deterministica;
   - planning privato;
   - `Planning -> Prep -> Dash -> Blast -> Move`;
   - Move normale come ultima fase volontaria;
   - Decision Boundary separato dai Phase Boundary;
   - stessa simulazione per giocatore, bot, test e replay;
   - client propone, server valida;
   - niente leak di intenti nemici.

Se trovi materiale storico incompatibile con lo stato corrente:
- non cancellarlo automaticamente;
- valuta `archive`, `historical`, `superseded` oppure aggiungi riferimenti alla nuova fonte canonica.

---

# 1. Aggiunta A — ConditionalIntent

## Intento di design

Formalizzare una possibile estensione futura del planning in cui un personaggio può dichiarare **un intento con una singola biforcazione condizionale**.

Non deve diventare un editor di scripting.

Esempio concettuale:

```text
IF TargetVisibleAtDecisionBoundary
    -> Attack
ELSE
    -> Guard
```

oppure:

```text
IF PreferredCellOccupied
    -> UseFallbackCell
ELSE
    -> UsePreferredCell
```

## Principio

`ConditionalIntent` deve essere trattato come **framework avanzato e futuro**, non come requisito immediato della v0.1, salvo che la documentazione corrente dimostri il contrario.

Vincoli iniziali da documentare:

```text
ConditionalIntent
├── Condition
├── TrueBranch
├── FalseBranch
└── EvaluationBoundary
```

Regole:

- massimo **1 condizione**;
- massimo **2 branch**;
- nessun nesting;
- niente loop;
- niente catene di `else-if`;
- nessuna lettura di informazioni che il giocatore non avrebbe legalmente al boundary;
- la condizione viene valutata solo su stato autorizzato e deterministico;
- il resolver deve poter registrare quale branch è stato selezionato;
- il branch non selezionato non produce effetti;
- la UI deve poter spiegare chiaramente entrambi i possibili esiti;
- non deve diventare un modo per “spiare” lo stato futuro dell'avversario.

## Rapporto con sistemi esistenti

Claude deve distinguere chiaramente `ConditionalIntent` da:

- fallback di validazione;
- `Delayed Action`;
- `Predictive Action`;
- `Prepared Reaction`;
- `Fast Reaction`;
- `Fast Action`;
- bot policy;
- moving-target policy.

In particolare:

### ConditionalIntent
Entrambi i branch sono dichiarati durante Planning.

### Fast Action
La scelta viene fatta live a un Decision Boundary.

### Prepared Reaction
Il trigger è esterno e la risposta è preconfigurata o selezionata tramite reaction opportunity.

### Predictive Action
Il giocatore scommette su uno stato/azione futura dell'avversario.

### Fallback
È una policy di validazione/esecuzione e non una nuova decisione tattica.

## Scope

Per questa task:

- documentare il concetto;
- indicarlo come **future framework / post-v0.1** se non già pianificato;
- aggiungere eventuale roadmap/issue se coerente;
- NON implementare runtime complesso;
- NON aggiungere UI complessa;
- NON introdurre schema definitivo se il modello Intent non è ancora stabile.

---

# 2. Aggiunta B — GenericActionModifier

## Intento di design

Promuovere a concetto esplicito una regola già implicita nel progetto:

> Un personaggio può essere caratterizzato non soltanto da abilità uniche, ma anche dal modo in cui modifica una Generic Action condivisa.

Esempi concettuali:

```text
Generic Overwatch
        ↓
Murdock
Patient Overwatch
```

```text
Generic Move
        ↓
Feng Mao
Flow Movement
```

```text
Generic Guard / Brace
        ↓
Steel
Guard Meter / Interposition
```

```text
Generic Interact
        ↓
Engineer
Remote Interaction
```

## Obiettivo

Fare in modo che la Signature Mechanic possa vivere anche dentro azioni universali, senza obbligare ogni personaggio ad avere sempre una ability separata per esprimere identità.

## Definizione proposta

Usare `GenericActionModifier` come nome concettuale salvo che il repository abbia già un nome migliore.

Deve rappresentare:

```text
GenericAction
+
CharacterModifier
=
CharacterSpecificVariant
```

Non deve duplicare una nuova action class se basta variare regole/configurazione.

## Proprietà da documentare

Ogni `GenericActionModifier` dovrebbe poter dichiarare almeno:

- Generic Action di base;
- personaggio o profilo che la modifica;
- regola modificata;
- condizioni;
- vantaggio;
- costo/trade-off;
- telegraphing;
- counterplay;
- interazioni con fase e action economy;
- eventuali effetti su targeting, range, facing, charges o reaction policy.

## Guardrail

Il modifier:

- non deve trasformare una Generic Action in un'abilità totalmente diversa senza più relazione con la base;
- non deve introdurre upgrade puri;
- deve avere trade-off leggibile;
- deve preservare la semantica della fase;
- non deve aggirare costi, cooldown o limiti di action economy senza regola esplicita;
- deve rimanere data-driven dove sensato;
- deve essere compatibile con TurnLog/explainability;
- deve essere validabile;
- deve essere leggibile nella UI.

## Cosa verificare nella documentazione corrente

Cerca riferimenti già esistenti a:

- generic actions;
- universal actions;
- action modifiers;
- Overwatch profilato per personaggio;
- Brace / Guard;
- Sprint / Dash / Move;
- Interact / Activate;
- action economy.

Se il concetto è già presente come campo della Character Matrix o schema personaggio:
- NON duplicarlo;
- rafforzane la definizione;
- aggiungi esempi e regole;
- collega il concetto alle pagine di azioni universali e ai personaggi.

---

# 3. Aggiunta C — Misplay / Failure State

## Intento di design

Aggiungere alla definizione di ogni Signature Mechanic una proprietà distinta da `Counterplay`:

> Cosa succede quando il giocatore usa correttamente la meccanica dal punto di vista delle regole, ma prende una decisione tattica sbagliata?

Questa proprietà deve chiamarsi preferibilmente:

`Misplay / Failure State`

oppure adattarsi alla nomenclatura già presente nel repository.

## Perché è distinta da Counterplay

### Counterplay
Descrive come l'avversario può rispondere o neutralizzare la Signature.

### Misplay / Failure State
Descrive la conseguenza di una lettura sbagliata del giocatore che usa la Signature.

Esempi:

```text
Vektor
Prediction sbagliata
-> azione a vuoto
```

```text
Bastion
chiude il percorso sbagliato
-> ostacola anche gli alleati
```

```text
Flux
costruisce la rete elettrica nella zona sbagliata
-> carica poco sfruttabile
```

```text
Riva
allaga un'area non vantaggiosa
-> crea una superficie utile anche al nemico
```

```text
Murdock
presidia il settore sbagliato
-> perde valore posizionale nel turno
```

## Regola di design

Ogni Signature Mechanic importante dovrebbe poter rispondere a:

```text
Player Question
State / Resource
Trigger
Payoff
Misplay / Failure State
Counterplay
Telegraphing
Team Synergies
Environment Interaction
```

## Guardrail

Il Failure State:

- non deve essere semplicemente “fai meno danno” in tutti i casi;
- deve essere collegato alla decisione tattica specifica del personaggio;
- non deve creare punishment arbitrario;
- deve essere leggibile ex ante dove possibile;
- deve produrre un risultato spiegabile nel TurnLog quando rilevante;
- non deve dipendere da RNG nascosto;
- non deve essere così severo da rendere il personaggio inutilizzabile per un singolo errore;
- deve differenziare personaggi che condividono lo stesso framework tecnico.

## Obiettivo progettuale

Usare `Misplay / Failure State` come criterio anti-clone:

Due personaggi possono condividere:

```text
PersonalResource
PredictionIntent
ReactionProfile
ControlledTerritory
GenericActionModifier
```

ma dovrebbero avere:

- player question diversa;
- payoff diverso;
- failure state diverso;
- counterplay diverso.

---

# 4. Aggiornamenti richiesti nella repository principale

Dopo l'audit, modifica la documentazione canonica più adatta.

Cerca in particolare file relativi a:

```text
docs/gameplay/
docs/design/
docs/characters/
docs/actions/
docs/roadmap/
docs/adr/
docs/decisions/
```

I path reali della repository prevalgono su questi esempi.

Obiettivi:

## Character / Signature documentation

Integrare:

```text
ConditionalIntent
GenericActionModifier
Misplay / Failure State
```

nella tassonomia delle Signature Mechanics.

## Character schema / matrix

Se esiste una scheda standard personaggio, aggiungere o consolidare:

```text
Primary Signature
Player Question
Framework
State / Resource
Trigger
Payoff
Misplay / Failure State
Counterplay
Telegraphing
Generic Action Modifiers
Environment Interaction
Team Synergies
```

Non aggiungere campi duplicati.

## Generic Actions

Aggiornare la documentazione delle azioni universali per chiarire che possono avere:

```text
Base Generic Action
Character-specific modifier/profile
```

senza perdere la loro identità di azione universale.

## Planning / Intent

Inserire `ConditionalIntent` nella tassonomia degli Intent, marcandolo correttamente come futuro/post-v0.1 se questa è la situazione reale.

## Roadmap

Se non già presente, aggiungere un item futuro per:

- Conditional Intent framework;
- validation/UI/explainability;
- test deterministici.

`GenericActionModifier` e `Misplay / Failure State` sono invece soprattutto consolidamento del design e possono non richiedere milestone tecnica autonoma.

---

# 5. Aggiornamenti richiesti nella Wiki

La Wiki si trova nella cartella/repository separata:

`refactor-tactics-main.wiki`

Prima di modificarla:

1. analizza struttura e naming;
2. identifica pagine player-facing vs technical/design;
3. evita di esporre dettagli implementativi inutili nella wiki giocatore.

Aggiornamenti desiderati:

## Wiki personaggi

Nelle pagine dedicate ai personaggi, quando appropriato, mostra:

- cosa rende unico il personaggio;
- quale Generic Action modifica;
- qual è la sua domanda tattica;
- quale rischio corre se legge male il turno;
- qual è il counterplay avversario.

Formato player-facing suggerito:

```text
### Come pensa questo personaggio

Domanda tattica:
"Quale settore controllerà davvero il nemico?"

Signature:
Predizione / controllo traiettorie

Se la lettura è corretta:
...

Se la lettura è sbagliata:
...

Come contrastarlo:
...
```

Non usare necessariamente questi titoli se la wiki ha già uno stile consolidato.

## Wiki delle azioni universali

Se esiste una pagina:

```text
Azioni universali
Generic Actions
Overwatch
Guard / Brace
Move
Interact
```

aggiungere una breve nota:

> Alcuni personaggi modificano una Generic Action condivisa tramite il proprio profilo, senza trasformarla in un'abilità completamente separata.

Aggiungere 1-2 esempi reali dal roster corrente, non personaggi storici/superseded.

## ConditionalIntent

NON presentarlo come feature disponibile al giocatore se è ancora futuro.

Se la wiki include sezioni Technical / Design / Roadmap, può essere documentato lì.

Altrimenti non aggiungerlo alla guida giocatore.

---

# 6. Validazione con roster corrente

Dopo aver consolidato il framework, applicalo come audit leggero ai personaggi correnti.

NON riscrivere tutto il roster.

Per ogni personaggio attivo rilevante, verifica se sono chiaramente identificabili:

```text
Player Question
Primary Signature
Generic Action Modifier
Misplay / Failure State
Counterplay
```

Se mancano:

- segnala il gap;
- proponi un'aggiunta minimale;
- non inventare una nuova meccanica senza necessità.

Dare precedenza al roster realmente attivo nella repository.

Non reintrodurre automaticamente roster storici o superseded.

---

# 7. Test e implicazioni tecniche

Questa task non richiede implementazione completa.

Documentare però i test futuri necessari.

## ConditionalIntent

Prevedere test per:

- TrueBranch;
- FalseBranch;
- stato identico -> stesso branch;
- boundary corretto;
- nessun accesso a hidden enemy intent;
- replay deterministico;
- TurnLog registra condition + selected branch;
- nessun nesting non supportato;
- invalid branch -> validation error esplicito.

## GenericActionModifier

Prevedere test per:

- azione base senza modifier;
- azione con modifier;
- trade-off applicato;
- phase invariants preservati;
- action economy preservata;
- TurnLog spiega base action + modifier;
- data validation.

## Misplay / Failure State

Non richiede necessariamente codice dedicato.

Deve però essere verificabile tramite scenari/golden test quando produce uno stato logico concreto.

---

# 8. Decision Log / ADR

Non creare un ADR per qualunque dettaglio.

Valuta un ADR soltanto se una di queste aggiunte introduce una decisione architetturale cross-system difficile da invertire.

Probabile impostazione:

- `Misplay / Failure State` -> design guideline, niente ADR;
- `GenericActionModifier` -> design/schema guideline, ADR solo se cambia il modello runtime;
- `ConditionalIntent` -> possibile ADR futuro quando si decide di implementarlo realmente.

Se esiste un Decision Log più adatto:
- aggiungi una decisione breve lì.

---

# 9. Output richiesto da Claude

Al termine restituisci un report strutturato.

## A. Audit

```text
Files inspected:
Existing overlapping concepts:
Conflicts found:
Historical/superseded material:
```

## B. Decisioni consolidate

Per ognuna:

```text
ConditionalIntent
Status:
Canonical definition:
Scope:
Docs updated:
Wiki updated:

GenericActionModifier
Status:
Canonical definition:
Scope:
Docs updated:
Wiki updated:

Misplay / Failure State
Status:
Canonical definition:
Scope:
Docs updated:
Wiki updated:
```

## C. File modificati

Separare:

```text
refactor-tactics-main
- ...

refactor-tactics-main.wiki
- ...
```

Per ogni file spiegare in una riga cosa è cambiato.

## D. Gap nel roster

Tabella:

```text
Character | Player Question | GenericActionModifier | Failure State | Counterplay | Status
```

Usare solo roster corrente.

## E. Roadmap / issue

Elencare eventuali item aggiunti o proposti.

## F. Test

Elencare i test futuri richiesti.

## G. Conflitti non risolti

Se una decisione non può essere dedotta dalla documentazione, NON scegliere arbitrariamente.

Segnalare:

```text
BLOCKED DECISION:
Context:
Options:
Recommended option:
Reason:
Files affected:
```

---

# 10. Commit Git proposti

Preferire commit piccoli e separati.

Esempio:

```text
docs(characters): formalize signature failure states
docs(actions): define character generic action modifiers
docs(planning): document future conditional intents
docs(wiki): align character mechanics and generic actions
```

Adatta i messaggi alla convenzione reale del repository.

---

# 11. Definition of Done

La task è completata quando:

- i tre concetti hanno una definizione canonica non duplicata;
- `ConditionalIntent` è chiaramente separato da Fast Action, Reaction, Prediction e fallback;
- `GenericActionModifier` è collegato alle Generic Actions e allo schema dei personaggi;
- `Misplay / Failure State` è distinto da Counterplay;
- la scheda standard dei personaggi li riflette quando appropriato;
- la roadmap non presenta ConditionalIntent come già implementato se non lo è;
- la wiki non pubblicizza feature future come disponibili;
- le pagine personaggio attive sono coerenti con il framework;
- documentazione tecnica e wiki non si contraddicono;
- non sono stati reintrodotti concetti o roster superseded;
- Claude restituisce il report finale con file modificati e gap residui.
