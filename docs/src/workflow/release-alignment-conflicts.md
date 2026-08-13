# Release alignment — problemi reali da risolvere

## 1. Registry non esprime v0.3/v0.4

Oggi i valori ammessi sono:

`v0.1 | v0.2 | future`

La roadmap post-v0.1 è già owner di:

`v0.2 | v0.3 | v0.4`.

Quindi il registry non è ancora capace di rappresentare la roadmap che già esiste.

---

## 2. E34 ha due Feature che si sovrappongono

### `RT-FEAT-CHARACTER-STATE`
- current: `future`
- shortlist: “Epic E34, #244”
- allineamento sicuro: `v0.4`

### `RT-FEAT-CHAR-TRANSFORMATION`
- current: `v0.2`
- descrizione: stati/stance/trasformazioni
- PIE-STATE come controparte umana

Non spostare entrambe alla cieca.
Fare un audit di:
- owner specs;
- gate;
- scenarios;
- PIE refs;
- issues.

Esito atteso: un solo concetto canonico, oppure due scope chiaramente distinti.

---

## 3. v0.3: reassignment chiari

| Feature | Da | A | Epic |
|---|---|---|---|
| RT-FEAT-BOT-BELIEF | future | v0.3 | E27 |
| RT-FEAT-BOT-PREDICTIVE | future | v0.3 | E28 |
| RT-FEAT-ACTION-TRAPS | future | v0.3 | E29 |
| RT-FEAT-ACTION-DELAYED | future | v0.3 | E29 |
| RT-FEAT-INTENT-CONDITIONAL | future | v0.3 | E33 |

Le Feature `RT-FEAT-PERCEPTION-*` restano v0.1: E27 le estende.

---

## 4. v0.4: gap di feature

E30/E31/E32 esistono come Epic e issue, ma il Feature Map corrente non mostra Feature dedicate
inequivocabili per quei tre scope.

Non usare questi falsi equivalenti:

- E30 != semplicemente “sposta MATCH-FORMAT a v0.4”
- E31 != sposta OBJECTIVE-SYSTEM a v0.4
- E32 != sposta STRESS-4V4 a v0.4

Quelle Feature sono fondazioni già consegnate in v0.1.

Il graph deve supportare:
`Epic v0.4 extends Feature v0.1`

e diagnosticare:
`Epic planned with no dedicated Feature`

finché non si decide se serve un nuovo Feature ID.

---

## 5. E37

E37 compare nel documento post-v0.1 ed è già completata, ma la tabella canonica delle release
non la assegna a v0.2/v0.3/v0.4.

Non inserirla in v0.4 soltanto perché la sua sezione appare dopo il titolo v0.4.

Lasciarla `future/supporting` finché l'owner delle release non cambia esplicitamente la tabella.

---

## 6. roadmap.shortlist §3 è stale

La sua sezione post-v0.1:
- non è generata;
- omette E36, E38, E39;
- mostra E25 senza issue anche se #265 esiste.

La migrazione deve far nascere una vista post-v0.1 generata.
Non correggere una riga manualmente per poi farla divergere di nuovo.
