# Issue Tracking Completeness — il Tracking Impact Pass

> **Owner della regola.** `CLAUDE.md` §Tracking ne porta la forma breve e `AGENTS.md` §Git vi rimanda:
> qui sta il dettaglio, lì il richiamo. Una regola scritta tre volte diverge alla prima correzione.
>
> Recepita il **2026-08-16** da un documento di consolidamento esterno
> ([`docs/archive/src/`](../archive/src/README.md) ne conserva la provenienza).

## Il principio

Ogni volta che una GitHub Issue viene **creata, suddivisa o modificata nella sostanza**, si esegue un
**Tracking Impact Pass**. Una Issue non è un elemento isolato: rappresenta il lavoro eseguibile, e gli altri
tracking rappresentano ciò che quel lavoro implementa, modifica, richiede, verifica, documenta, dimostra,
configura a mano o introduce come contenuto.

> **CREATE OR LINK, NEVER IGNORE.**

Per ogni categoria applicabile, in quest'ordine:

1. **cerca** un elemento esistente;
2. **collegalo** se appropriato;
3. **aggiornalo** se la Issue ne cambia scope o stato;
4. **crealo** solo se non ne esiste uno corretto;
5. dichiara **`N/A`** quando la categoria non si applica.

⚠️ **Non creare duplicati per soddisfare formalmente la regola.** Una entry inventata per riempire un campo
costa più di un campo vuoto: sporca le viste generate, e il prossimo che cerca quella cosa ne trova due.

## Il blocco `## Tracking`

Ogni Issue creata deve contenerlo. Il template di
[`.github/ISSUE_TEMPLATE/`](../../.github/ISSUE_TEMPLATE/) lo precompila, così non dipende dalla memoria.

```text
## Tracking

Milestone:            LINK / CREATE / N/A
Epic:                 LINK / CREATE / N/A
Feature Map:          LINK / CREATE / UPDATE / N/A
Scenario Map:         LINK / CREATE / UPDATE / N/A
Test:                 LINK / CREATE / UPDATE / N/A
Editor Map:           LINK / CREATE / UPDATE / N/A
Assets:               LINK / CREATE / ACQUIRE / N/A
Content/Data:         LINK / CREATE / UPDATE / N/A
Wiki/Docs:            LINK / CREATE / UPDATE / N/A
ADR/Decision:         LINK / CREATE / UPDATE / N/A
UI/UX:                LINK / CREATE / UPDATE / N/A
Debug/Observability:  LINK / CREATE / UPDATE / N/A
Dependencies:         …
```

**`N/A` è valido. Un campo mancante no.** La differenza è che `N/A` è una decisione presa, l'assenza è una
domanda che nessuno si è posto.

## Le dodici categorie

### 1 · Roadmap / Milestone / Epic

Milestone di appartenenza, Epic o parent, dipendenze, Issue bloccanti e bloccate, ordine raccomandato. Ogni
Issue deve stare su un percorso verso una milestone identificabile. Se appartiene a una Epic esistente:
`LINK`. Se apre un gruppo di lavoro consistente: `CREATE EPIC`. Se è un task tecnico locale: **nessuna Epic
artificiale**.

⚠️ In questo repository i numeri di **epic** (`Enn`) sono un contatore condiviso non coperto
dall'allocatore: si verificano sul remote (`gh issue list --search "EPIC in:title"`) subito prima del merge
— vedi `AGENTS.md` §*`D-nnn` non si sceglie a mano*.

### 2 · Feature Map

> La Issue introduce, completa o modifica una feature percepibile in gioco o un sistema riutilizzabile?

Se sì: collega la feature, aggiornane lo stato, indica quali acceptance criteria copre. **Una feature è
implementata da molte Issue** — non creare una feature per ogni task tecnico.

⚠️ Owner dello stato: [`docs/roadmap/feature-registry.yaml`](../roadmap/feature-registry.yaml), e **solo**
quello. `status` è **derivato dai gate**, non dichiarato: la Issue referenzia il `feature_id`, non copia lo
stato. Le viste (`feature-registry.json`, `project-graph.json`, le `*.shortlist.md`) si **rigenerano**.

### 3 · Scenario Map

> Esiste uno scenario giocabile o dimostrativo che deve mostrare questa funzionalità?

Collega, estendine gli acceptance criteria, o creane uno **solo se rappresenta una situazione tattica
davvero distinta**. Lo scenario descrive il **comportamento osservabile**, mai l'implementazione.

### 4 · Test

Ogni Issue che cambia comportamento competitivo ha una strategia di test: automation, unit/core, functional,
golden, determinismo, network, privacy/canary, packaged, performance, regressione.

> **Una Issue non è completa perché «provata in PIE».**

Per i sistemi deterministici vale sempre `same snapshot + rules + seed → same result`; per il networking,
`unauthorized client receives zero private information`.

⚠️ `Test: N/A` su una Issue che tocca simulazione, networking o regole competitive **richiede una
motivazione esplicita**. Non servono tre test se uno copre il rischio.

### 5 · Editor Map

> La Issue richiede lavoro dentro Unreal Editor che non si completa modificando file sorgente?

Data Asset, mesh/material, Blueprint, collision, Niagara, Widget, Enhanced Input, livelli, Actor, GameMode,
Gameplay Tag, verifiche visive. Se sì, collega o crea una voce in
[`editor-sessions.yaml`](../roadmap/editor-sessions.yaml) — la vista è
[`editormap.shortlist.md`](../roadmap/editormap.shortlist.md), **generata**.

La voce dice: cosa fare · dove nell'Editor · asset coinvolti · prerequisiti · risultato atteso · come si
verifica.

> **Non nascondere lavoro manuale dentro un acceptance criterion ordinario.** Il codice generato non implica
> che la feature sia usabile: se restano passi in Editor, la Issue non è completa finché non li ha tracciati.

⚠️ Le verifiche PIE hanno un owner proprio,
[`test-manuali-pie.md`](test-manuali-pie.md): la Editor Map ne cita gli **ID**, mai l'esito atteso.

### 6 · Asset

> La Issue richiede asset che non sono codice?

Non scrivere `serve un'icona`. Scrivi:

```text
Asset:                 Overwatch action icon
Type:                  UI/Icon
Required by:           #<issue>
Status:                Missing
Strategy:              Acquire | Create | Placeholder
Source:                <sorgente o percorso, se noto>
License:               <se esterno>
Target path:           Content/RT/UI/Icons/…
Final required for:    <milestone>
Placeholder acceptable: Yes/No
```

**Non bloccare una Issue di gameplay su un asset finale quando basta un graybox** — dichiaralo.

⚠️ La destinazione segue [`convenzioni-contenuti-ue.md`](tooling/convenzioni-contenuti-ue.md), ed è normativa. E un
`.uasset` nuovo richiede la **riga di allowlist prima che l'asset esista**: `.gitignore` esclude
`Content/**/*.uasset` e riammette per file. Nell'ordine inverso `git add` non fa nulla e non lo dice.

### 7 · Content / Data

Contenuto competitivo data-driven: Stable ID, Definition Version, Gameplay Tag, Data Asset, catalogo,
dipendenze, validator, manifest/hash. **Non hardcodare come soluzione permanente ciò che appartiene alla
pipeline dati.**

### 8 · Wiki / Documentation

> La Issue introduce o modifica una regola, un sistema, un contratto o una decisione consultabile in futuro?

Destinazioni: Wiki · `docs/` · PDR · ADR/Decision Log · specifica tecnica o di gameplay.

> **Le decisioni architetturali e di game design non vivono solo nel testo di una Issue.**

⚠️ La Wiki è un **repository separato** e il clone **è** la fonte (`D-076`).

### 9 · Decision / ADR

Se durante la Issue si decide qualcosa che cambia un'invariante, sceglie fra due architetture, modifica una
regola competitiva o la resolution order, tocca networking/privacy o una convenzione dati, o influenza più
feature: **crea o aggiorna la Decision/ADR**. Nessun ADR per dettagli implementativi banali.

⚠️ `D-nnn` **non si sceglie a mano**: `python scripts/rt_shared_id.py reserve D`.

### 10 · UI / UX

Se la feature diventa percepibile o controllabile: HUD, icone, tooltip, warning, targeting, feedback, debug
visualization, combat log, stato Confermato/Previsto/Incerto, accessibilità. Se serve lavoro visuale, collega
**anche** Editor Map e Asset.

### 11 · Debug / Observability

Per i sistemi core: log category, debug draw, console command, evento TurnLog, reason code, scope Insights,
metriche, dump di stato.

> **Una feature competitiva deve poter spiegare *perché* ha prodotto un certo risultato.**

### 12 · Dependencies

Issue bloccanti e bloccate, con il numero. Una dipendenza scritta a parole non si può interrogare.

## Prima di creare

Cerca, in quest'ordine: Issue esistenti · Epic · Feature Map · Scenario Map · Editor Map · asset · test ·
Wiki/docs · ADR/Decision Log. Serve a evitare duplicati e frammentazione, ed è la parte che si salta quando
si ha fretta.

## Dopo aver creato

Verifica i link · verifica parent e dipendenze · aggiorna le map interessate · **rigenera le viste** ·
controlla che nessun elemento creato sia orfano · riepiloga cosa è stato creato e cosa riusato:

```text
Created:  #412 · Automation Test RT.Reaction.Overwatch.EnemyEnterArea
Linked:   Feature FEAT-REACTION-001 · Scenario SCN-OW-003 · Epic #366
Updated:  Feature Map · Scenario Map
N/A:      Assets · Editor Map · ADR
```

## Definition of Ready

Scope e acceptance criteria · milestone/parent dove servono · dipendenze collegate · **Tracking Impact Pass
completo** · asset mancanti tracciati · lavoro Editor tracciato · test identificato · Feature/Scenario
collegati · documentazione interessata identificata.

## Definition of Done

**Prima di chiudere si riesegue il Tracking Impact Pass.** Feature Map allineata · Scenario Map allineata ·
Editor Task completati o tracciati separatamente · asset disponibili o rinviati per iscritto · test presenti
e verdi · documentazione allineata · Decision Log aggiornato · dipendenze aggiornate · nessun tracking
orfano.

> **Una Issue chiusa con tracking incoerenti è una Issue incompleta.**

⚠️ E il DoD si consuntiva **nel commento di chiusura**, non spuntando il body: le spunte nel corpo di una
Issue chiusa non sono un segnale — su questo repository 122 Issue chiuse ne hanno zero.
