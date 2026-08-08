# Overwatch e Fast Reactions

> **Stato v0.1:** modello deciso · reazioni preparate di base esistono · Overwatch interattivo E14 non ancora implementato
> **Tipo:** guida giocatore, non normativa

## Il concetto

Overwatch significa **rinunciare a colpire adesso** per preparare una risposta a ciò che pensi farà il nemico.

La regola economica è:

```text
Attack  OPPURE  Ability  OPPURE  Overwatch
```

Salvo eccezioni esplicite del kit, non ottieni attacco e Overwatch insieme.

## Perché ha un costo

Se nessuno entra nella tua zona o attiva il trigger, hai speso la tua scelta offensiva senza ottenere un colpo. Questo è intenzionale: Overwatch deve essere una **scommessa**, non il pulsante sicuro da premere quando non sai cosa fare.

## Overwatch è universale, il profilo no

La struttura è comune, ma ogni personaggio può sorvegliare in modo diverso:

| Profilo | Esempio di trigger/risposta |
|---|---|
| Marksman | movimento nemico → `FIRE / HOLD` |
| Tank | nemico minaccia un alleato → `INTERCEPT / HOLD` |
| Controller | attraversamento di un bordo → spinta a sinistra/destra/HOLD |
| Assassin | ingresso in prossimità → `AMBUSH / HOLD` |
| Engineer | interazione con dispositivo → `HACK / HOLD` |

Questi sono **profili di design**, non la promessa che tutti siano presenti nella v0.1.

## Tre modi di risolvere una opportunity

Il comportamento emerge dalle risposte legali:

### Automatic

Resta una sola risposta possibile. Nessuna finestra live.

### Conditional

Una condizione scelta nel Planning riduce le risposte quando il trigger avviene. Se ne resta una sola, il resolver procede senza fermarsi.

### FastSelect

Restano almeno due risposte: si apre un **decision boundary**.

Baseline decisa:

- finestra **3,0 s**;
- `Timeout → HOLD`;
- `HOLD` non consuma la charge;
- trigger simultanei nello stesso micro-step vengono aggregati;
- nessun interrupt annidato nella v0.1.

## Prepared Reaction vs Fast Reaction

Una reazione come l'Interposition di Bastion può essere **preparata** e avere una sola risposta legale: non apre una finestra.

Una Fast Reaction invece chiede una decisione live, ma solo nel punto previsto dalle regole. Non trasforma la Resolution in un secondo Planning.

## Facing

Il modello deciso fa derivare la zona di Overwatch dal facing. Questa dipendenza entra con E16/E13/E14 ed è ancora da implementare.

## Fonti normative

- `docs/gameplay/brief-azioni-generiche-overwatch.md`
- `docs/gameplay/brief-overwatch-reazioni.md`
- `docs/gameplay/spec-sequenza-turno.md`
- `docs/decisions/adr-0004-finestre-di-reazione.md`
- `docs/decisions/adr-0005-orientamento.md`
