# Reazioni, Overwatch e predizione

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-REACTION-FAST-ACTION -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-REACTION-FAST-ACTION` · Release: `v0.1` · Roadmap: `E14 · CP 14.6`  
> Stato: **DESIGNED** · Gate: `0/9`  
> Scenario: `—`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-REACTION-FAST-ACTION -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-REACTION-OVERWATCH -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-REACTION-OVERWATCH` · Release: `v0.1` · Roadmap: `E14 · CP 14.4`  
> Stato: **SPECIFIED** · Gate: `1/9`  
> Scenario: `Spec.Overwatch.HoldThenFire (pianificato)`  
> Le reazioni **preparate** esistono e funzionano in partita; l'**Overwatch interattivo** di questa pagina non e' implementato.  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-REACTION-OVERWATCH -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-REACTION-FAST -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-REACTION-FAST` · Release: `v0.1` · Roadmap: `E14 · CP 14.5, 14.6`  
> Stato: **SPECIFIED** · Gate: `1/9`  
> Scenario: `—`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-REACTION-FAST -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-REACTION-OPPORTUNITY -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-REACTION-OPPORTUNITY` · Release: `v0.1` · Roadmap: `E14 · CP 14.3`  
> Stato: **SPECIFIED** · Gate: `1/8`  
> Scenario: `—`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-REACTION-OPPORTUNITY -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-REACTION-PREPARED -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-REACTION-PREPARED` · Release: `v0.1` · Roadmap: `E5 · CP 5.1, 5.2, 5.3, 5.4, 5.5`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Combat.CounterStrikesBack`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-REACTION-PREPARED -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ACTION-PREDICTIVE -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ACTION-PREDICTIVE` · Release: `v0.1` · Roadmap: `E18 · CP 18.1, 18.2`  
> Stato: **SPECIFIED** · Gate: `1/8`  
> Scenario: `Spec.Predictive.WhiffOnEmptyCell (pianificato)`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ACTION-PREDICTIVE -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-CORE-DECISION-BOUNDARY -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-CORE-DECISION-BOUNDARY` · Release: `v0.1` · Roadmap: `E14 · CP 14.1, 14.2, 14.3`  
> Stato: **SPECIFIED** · Gate: `1/8`  
> Scenario: `—`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-CORE-DECISION-BOUNDARY -->

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
