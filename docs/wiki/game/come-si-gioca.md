# Come si gioca

> **Stato nel gioco:** loop principale consolidato
> **Tipo:** guida giocatore, non normativa

## Una partita in una frase

Pianifica contemporaneamente agli avversari, conferma il piano, osserva la Resolution e usa ciò che hai imparato per preparare il round successivo.

## 1. Inizio del round

Il gioco entra in **Planning**. Lo stato pubblico della mappa è visibile e ogni giocatore prepara le decisioni delle proprie unità.

Un piano può comprendere, secondo il personaggio e le regole disponibili:

- azione principale;
- percorso o profilo di movimento;
- facing finale;
- una reazione preparata;
- eventuale azione predittiva;
- fallback quando un bersaglio o un percorso non è più valido.

## 2. Planning simultaneo

Le due squadre pianificano **nello stesso intervallo di tempo**. Non esiste un giro in cui prima agisce A e poi B.

Nel 2v2 della v0.1 il timer massimo di Planning parte da **30 secondi**, ma il giocatore può dichiararsi `Ready` prima.

## 3. Ready e Commit

`Ready` significa «ho terminato il piano». Quando gli intenti vengono **committati**, diventano canonici per quel round.

Da quel punto non si riapre un nuovo Planning durante la Resolution. Le sole scelte live consentite sono piccole decisioni esplicitamente previste da una finestra di reazione.

## 4. Resolution

Il gioco risolve le macro-fasi nell'ordine:

```text
Prep → Dash → Blast → Move
```

- **Prep:** preparazioni, stance e setup che devono esistere prima dello scontro.
- **Dash:** mobilità speciale pre-attacco.
- **Blast:** attacchi, abilità, controllo e interazioni della fase offensiva.
- **Move:** movimento normale, sempre dopo il Blast.

## 5. Cleanup

Alla fine del round vengono gestiti gli effetti di fine round, scadenze, cooldown, KO e aggiornamenti necessari prima del Planning successivo.

## 6. Fine della partita

Una partita può terminare per:

- eliminazione della squadra avversaria;
- condizione dell'obiettivo;
- raggiungimento del `RoundLimit` previsto dal formato/scenario.

## La regola mentale più importante

Non chiederti soltanto:

> «Qual è la mia azione migliore?»

Chiediti:

> «Quando questa azione risolverà, dove saranno tutti gli altri?»

## Fonti normative

- `docs/gameplay/spec-sequenza-turno.md`
- `docs/gameplay/spec-durata-partita-e-scala-mappe.md`
- `docs/product/piano-canonico-mvp.md`
