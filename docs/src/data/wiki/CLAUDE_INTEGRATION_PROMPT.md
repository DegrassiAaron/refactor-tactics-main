# TASK — INTEGRARE LE INFOGRAFICHE OLD-STYLE NELLA WIKI DI REFACTORTACTICS

Stai lavorando nella repository `refactor-tactics-main`.

Questo pacchetto contiene un set di **10 infografiche già approvate visivamente**.
Sono volutamente le infografiche precedenti, con pseudo-personaggi, scene tattiche,
griglia esagonale e visualizzazione concreta delle meccaniche.

## REGOLA FONDAMENTALE

NON rigenerare, ridisegnare, reinterpretare o sostituire queste immagini.

NON usare il successivo set `v0.8` / immagini schematiche generate dopo queste.

Le immagini di questo pacchetto sono gli asset visuali da integrare.

Le immagini NON sono fonte normativa delle regole.
Se un testo dentro una figura differisce dal canone corrente della repository:
- NON modificare di nascosto la documentazione canonica per adeguarla alla figura;
- NON riscrivere il gameplay per far coincidere la figura;
- segnala il mismatch nel report finale;
- mantieni il testo Wiki aderente alle fonti normative correnti;
- se il mismatch rende l'immagine fuorviante in modo grave, fermati su QUELLA immagine e segnalalo.

Prima di modificare:
1. leggi `AGENTS.md`, `CLAUDE.md` se presenti;
2. leggi `docs/wiki/README.md` e `docs/wiki/index.md`;
3. leggi il Decision Log / ADR rilevanti;
4. leggi le pagine target indicate in `wiki-image-manifest.json`;
5. controlla che i file immagine del pacchetto siano presenti nei path previsti.

---

# OBIETTIVO EDITORIALE

Voglio una Wiki a tre livelli:

1. **Indice / guida principale**
   - leggero;
   - niente muri di immagini;
   - orienta il lettore.

2. **Pagina di dettaglio del sistema**
   - UNA infografica principale pertinente;
   - testo sotto che spiega e aggiorna la regola;
   - link alle meccaniche granulari.

3. **Pagine `meccaniche/`**
   - regole specifiche;
   - evitare di ripetere la stessa infografica panoramica.

Principio:

> una immagine visuale forte per concetto principale,
> non la stessa informazione ripetuta in cinque pagine.

---

# ASSET DA INTEGRARE

Usa `wiki-image-manifest.json` come mapping machine-readable.

## 1 — RefactorTactics in 60 secondi

Asset:
`docs/wiki/images/overview/01_refactortactics-in-60-secondi-v0.1.png`

Pagina:
`docs/wiki/game/che-cose-refactortactics.md`

Inserimento:
subito dopo `## In breve`, prima della spiegazione lunga.

Markdown suggerito:

```md
![RefactorTactics in 60 secondi](../images/overview/01_refactortactics-in-60-secondi-v0.1.png)
```

NON duplicare questa immagine nella Home Wiki.

La Home può linkare alla pagina `che-cose-refactortactics.md`.

---

## 2 — Anatomia di un turno

Asset:
`docs/wiki/images/gameplay/02_anatomia-turno-v0.1.png`

Pagina:
`docs/wiki/game/struttura-del-round.md`

Inserimento:
dopo il diagramma testuale della sequenza completa.

Markdown:

```md
![Anatomia di un turno di RefactorTactics](../images/gameplay/02_anatomia-turno-v0.1.png)
```

La pagina resta normativa a livello testuale:
Planning / Ready-Commit / Prep / Dash / Blast / Move / Cleanup.

L'immagine serve a rendere intuitiva la sequenza.

---

## 3 — Azioni universali e movimento

Asset:
`docs/wiki/images/gameplay/03_azioni-universali-movimento-v0.1.png`

Pagina:
`docs/wiki/game/azioni-e-movimento.md`

Inserimento:
vicino all'inizio, dopo metadata/stato e prima delle spiegazioni puntuali.

Markdown:

```md
![Azioni universali, Action Economy e mobilità speciale](../images/gameplay/03_azioni-universali-movimento-v0.1.png)
```

IMPORTANTE:
questa immagine è stata scelta perché rende finalmente visibili:
- Wait;
- Move;
- Basic Attack;
- Guard;
- Brace;
- Interact;
- Overwatch;
- Action Economy;
- Dash;
- Leap;
- Reposition;
- Charge;
- distinzione Sprint vs Dash.

Prima dell'integrazione confronta questi termini con:
- decision log corrente;
- action catalog corrente;
- `docs/wiki/game/azioni-e-movimento.md`.

Se ci sono differenze di migrazione legacy, lascia il testo Wiki corretto e segnala il gap.

---

## 4 — Reazioni e Decision Boundary

Asset:
`docs/wiki/images/gameplay/04_reazioni-decision-boundary-v0.1.png`

Pagina:
`docs/wiki/game/reazioni-overwatch-e-previsioni.md`

Inserimento:
prima di `## Tre concetti diversi`.

Markdown:

```md
![Reazioni, Overwatch e Decision Boundary](../images/gameplay/04_reazioni-decision-boundary-v0.1.png)
```

Il testo Wiki deve continuare a distinguere chiaramente:
- Prepared Reaction;
- Fast Reaction;
- Predictive Action;
- Overwatch;
- decision boundary;
- trigger/opportunity/commit/resolution.

Se la baseline Fast Reaction corrente è 3,0 s, il testo Wiki resta 3,0 s anche se una figura storica non mostra il numero.

---

## 5 — La mappa è un'arma

Asset:
`docs/wiki/images/gameplay/05_mappa-meccaniche-ambientali-v0.1.png`

Pagina:
`docs/wiki/game/mappa-terreni-e-ambiente.md`

Inserimento:
prima di `## La mappa è un grafo tattico`.

Markdown:

```md
![Mappa e meccaniche ambientali di RefactorTactics](../images/gameplay/05_mappa-meccaniche-ambientali-v0.1.png)
```

Usa l'immagine come panoramica di:
- griglia esagonale;
- facing;
- LOS;
- cover;
- acqua;
- elettricità;
- fuoco;
- fumo;
- porte/leve;
- cover distruttibili;
- movimento e costi;
- combo ambientali.

NON duplicarla nelle pagine `coperture.md`, `porte.md`, `acqua-e-elettricita.md`, ecc.
Quelle pagine devono restare approfondimenti granulari.

---

## 6 — Combo ambientali v0.1

Asset:
`docs/wiki/images/gameplay/06_combo-ambientali-v0.1.png`

Pagina:
`docs/wiki/game/sinergie-e-combinazioni.md`

Inserimento:
dopo i paragrafi introduttivi che spiegano che le abilità appartengono ai personaggi
e le interazioni appartengono ai sistemi, prima di `## Tre livelli`.

Markdown:

```md
![Esempi di combo ambientali e sinergie v0.1](../images/gameplay/06_combo-ambientali-v0.1.png)
```

NON trasformare le combinazioni illustrate in:
- abilità di coppia;
- bonus hard-coded;
- FactionSetBonus;
- dipendenze `if HeroA && HeroB`.

La pagina deve mantenere il principio:
regola sistemica prima, personaggi come esempio dopo.

---

## 7 — Facing, LOS, Cover e Percezione

Asset:
`docs/wiki/images/gameplay/07_facing-los-cover-percezione-v0.1.png`

Pagina:
`docs/wiki/game/visibilita-rumore-e-informazione.md`

NON creare una pagina duplicata `facing-los-cover-percezione.md`
a meno che la struttura della repository sia cambiata dopo questo prompt.

Inserimento:
dopo l'introduzione sotto `## Perché l'informazione conta`,
prima dei livelli di conoscenza.

Markdown:

```md
![Facing, LOS, Cover e Percezione](../images/gameplay/07_facing-los-cover-percezione-v0.1.png)
```

Aggiungi/controlla link verso:
- `../meccaniche/facing-e-direzionalita.md`;
- `../meccaniche/coperture.md`;
- eventuale pagina rumore/percezione se esiste.

---

## 8 — Planning e coordinazione di squadra

Asset:
`docs/wiki/images/gameplay/08_planning-coordinazione-v0.1.png`

Pagina da creare se non esiste:
`docs/wiki/game/planning-e-coordinazione.md`

Obiettivo pagina:
spiegare la UI/UX del planning senza trasformare `come-si-gioca.md` in una specifica HUD.

Contenuti minimi:
- Path Ghost;
- AoE Ghost;
- label / intent;
- ping / draw;
- Ready;
- Confermato;
- Previsto;
- Incerto;
- privacy team-only.

Markdown:

```md
![Planning e coordinazione di squadra](../images/gameplay/08_planning-coordinazione-v0.1.png)
```

Aggiorna `docs/wiki/game/come-si-gioca.md`:
nella sezione Planning aggiungi un link alla nuova pagina.

Aggiorna `docs/wiki/README.md` e/o `docs/wiki/index.md`
solo quanto basta per renderla raggiungibile.

PRIVACY:
non descrivere mai il planning nemico come dato presente ma nascosto dalla UI.
Gli intenti canonici completi restano server-only e non devono essere replicati agli avversari.

---

## 9 — Roster v0.1

Asset:
`docs/characters/images/09_roster-v0.1-overview.png`

Pagina:
`docs/characters/index.md`

Inserimento:
immediatamente sotto:

```md
## Roster v0.1 — RefactorTactics
```

e prima dei link:
- Flux;
- Riva;
- Bastion;
- Vektor.

Markdown:

```md
![Roster v0.1: Flux, Riva, Bastion e Vektor](images/09_roster-v0.1-overview.png)
```

NON duplicare questa panoramica dentro `flux.md`, `riva.md`, `bastion.md`, `vektor.md`.

Le singole pagine personaggio in futuro possono ricevere una scheda visuale specifica per eroe.

---

## 10 — TurnLog e determinismo

Asset:
`docs/wiki/images/technical/10_turnlog-determinismo-v0.1.png`

Pagina da creare se non esiste:
`docs/wiki/technical/turnlog-e-determinismo.md`

Questa pagina appartiene alla **Technical Wiki**, non al tutorial principale.

Markdown:

```md
![TurnLog e determinismo](../images/technical/10_turnlog-determinismo-v0.1.png)
```

Contenuti minimi:
- snapshot;
- accepted intents;
- resolver;
- TurnLog;
- nuovo stato;
- StateHash;
- LogHash;
- replay;
- audit;
- automated/golden tests;
- presentazione come consumer del risultato.

Deriva il testo dalle specifiche tecniche correnti.
NON usare la figura come fonte normativa.

Aggiungi un piccolo link dalla Wiki principale alla sezione Technical Wiki,
senza mischiare TurnLog con il percorso tutorial del giocatore.

---

# HOME WIKI

NON mettere tutte e 10 le infografiche nella Home.

Mantieni `docs/wiki/index.md` leggero.

La Home deve restare un percorso di navigazione:
- Che cos'è;
- Come si gioca;
- Struttura del round;
- Esempio;
- Personaggi;
- Manuale delle meccaniche;
- Planning e coordinazione, se opportuno.

Puoi aggiungere piccoli link testuali.
Non creare una dashboard grafica ridondante.

---

# GESTIONE DELLE IMMAGINI

Non spostare arbitrariamente gli asset dal layout del pacchetto.

Struttura desiderata:

```text
docs/wiki/images/
  overview/
    01_refactortactics-in-60-secondi-v0.1.png
  gameplay/
    02_anatomia-turno-v0.1.png
    03_azioni-universali-movimento-v0.1.png
    04_reazioni-decision-boundary-v0.1.png
    05_mappa-meccaniche-ambientali-v0.1.png
    06_combo-ambientali-v0.1.png
    07_facing-los-cover-percezione-v0.1.png
    08_planning-coordinazione-v0.1.png
  technical/
    10_turnlog-determinismo-v0.1.png

docs/characters/images/
  09_roster-v0.1-overview.png
```

Le immagini sono binarie:
- assicurati che siano tracciate con Git LFS se le regole della repo lo richiedono;
- verifica `.gitattributes`;
- non convertirle automaticamente in JPG;
- non ricomprimerle con perdita.

---

# AUDIT ANTI-RIDONDANZA

Dopo l'integrazione cerca nella Wiki:
- immagini precedenti equivalenti;
- vecchi poster panoramici;
- riferimenti al vecchio roster;
- immagini con vecchie reaction window;
- copie multiple della stessa infografica.

Non cancellare materiale storico automaticamente.

Se trovi materiale obsoleto:
- segnala;
- proponi archive/remove;
- modifica solo se le convenzioni della repository lo rendono sicuro.

---

# VALIDAZIONE

Prima di chiudere:

1. verifica che tutti i path Markdown delle 10 immagini siano validi;
2. verifica che ogni immagine appaia in UNA pagina principale;
3. verifica che nessuna pagina giocatore dipenda dalla figura per una regola fondamentale;
4. verifica link relativi da `docs/wiki/game/`;
5. verifica link da `docs/characters/index.md`;
6. verifica che le due eventuali nuove pagine siano raggiungibili dalla navigazione;
7. cerca riferimenti a roster v0.1 diversi da Flux/Riva/Bastion/Vektor nelle pagine modificate;
8. cerca finestre Fast Reaction obsolete nelle pagine modificate;
9. esegui gli eventuali validator/documentation checks già presenti nella repository.

---

# OUTPUT FINALE RICHIESTO

Restituisci:

## Files changed
Lista dei file Markdown e degli asset aggiunti.

## Placement
Per ognuna delle 10 immagini:
- pagina;
- sezione;
- path usato.

## New pages
Specificare se sono state create:
- `planning-e-coordinazione.md`;
- `technical/turnlog-e-determinismo.md`.

## Mismatches found
Qualsiasi differenza fra testo dentro le immagini e canone corrente.

## Redundancy audit
Vecchie immagini/pagine duplicate individuate, senza cancellazioni non autorizzate.

## Validation
- link relativi;
- immagini presenti;
- eventuali test/validator;
- Git LFS / `.gitattributes`.

## Suggested commit
Usa un messaggio simile:

`docs(wiki): integrate v0.1 gameplay infographics`
