# Reazioni, Overwatch e predizione

> **Stato nel gioco:** reazioni base presenti; Overwatch interattivo e finestre E14 ancora in sviluppo
> **Tipo:** guida giocatore, non normativa

## Tre concetti diversi

### Prepared Reaction

La prepari durante il Planning. Quando il trigger avviene, la risposta è già determinata oppure esiste una sola risposta legale. Non richiede una nuova scelta live.

Esempio: un'interposizione che devia un attacco verso il difensore.

### Fast Reaction

È una scelta **live** provocata da un evento esterno durante la Resolution.

Esempio tipico: Overwatch con `FIRE` oppure `HOLD`.

### Predictive Action

È decisa **interamente nel Planning** e risolve più tardi. Non ricevi nuova informazione quando arriva il momento di risolverla.

È la vera scommessa del tipo:

> «Penso che entrerai proprio lì.»

## Overwatch

Overwatch è una postura/comando disponibile come concetto a tutti, ma ogni personaggio può avere un profilo diverso per area, trigger e risposta.

La scelta fondamentale è:

```text
Attack  OPPURE  Ability  OPPURE  Overwatch
```

Se prepari Overwatch e nessuno attiva il trigger, hai perso quell'opportunità offensiva. È intenzionale: la previsione deve avere un costo.

## Quando si apre una finestra

Il modello usa il numero di risposte legali:

- **0–1 risposta:** commit immediato, nessuna pausa;
- **2+ risposte:** può aprirsi un decision boundary.

Per la baseline di Overwatch:

- finestra: **3,0 s**;
- `Timeout → HOLD`;
- `HOLD` non consuma automaticamente la charge;
- trigger simultanei dello stesso micro-step vengono aggregati in una sola opportunity;
- niente interrupt annidati nella v0.1.

## Automatic, Conditional, FastSelect

Non sono tre sistemi diversi:

- **Automatic:** resta una sola risposta valida;
- **Conditional:** una condizione scelta nel Planning riduce le risposte al trigger;
- **FastSelect:** restano più risposte e il giocatore sceglie live.

## Fonti normative

- `docs/gameplay/spec-sequenza-turno.md`
- `docs/gameplay/brief-azioni-generiche-overwatch.md`
- `docs/decisions/adr-0004-finestre-di-reazione.md`
