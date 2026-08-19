# Visual Language — Regole di composizione

> **Statuto**: sorgente di design, non canone. Vedi [`01-principi.md`](01-principi.md).

## 1. La formula

```text
TARGET + GEOMETRY → EFFECT + MODIFIER / STATE
```

Non tutti e quattro i termini sono sempre presenti. La formula dice **in che ordine** si legge un glifo, non
quanti pezzi deve avere.

| Esempio | Lettura |
|---|---|
| `Enemy + Line → Damage + Electric` | una linea che parte verso un nemico e scarica elettricità |
| `Cell + Circle → Water` | un'area circolare su celle che diventa acqua |
| `Self + Dash → Reposition` | l'unità si sposta rapidamente |
| `Enemy + Cone → Overwatch` | un settore controllato in cui un nemico attiverà una reazione |
| `Ally + Circle → Heal + Water` | cura ad area su alleati, con payload acqua |

## 2. Il limite dei tre componenti

A 24 px un glifo regge **tre** componenti dominanti. A 16 px ne regge **due**.

Quando una skill ne richiederebbe quattro, non si riduce lo stroke: si **sacrifica il termine meno
distintivo**. L'ordine di sacrificio è:

1. `Modifier` — spesso è già comunicato dallo stato dello slot;
2. `Target` — spesso è già comunicato dal contesto di targeting;
3. `Geometry` — solo se l'effetto è più identitario della forma;
4. `Effect` — **mai**: è ciò che distingue l'azione.

Un glifo che perde l'effetto non è più leggibile come quell'azione.

## 3. Slot art delle ability

Le ability degli eroi possono essere più caratterizzate del glifo semantico, ma restano dentro la grammatica.
Non sono mini-illustrazioni.

Vincoli: 2–3 elementi principali, silhouette forte, primitive semanticamente corrette, leggibile a **32–36 px
dentro uno slot da 56–60 px**.

Le venti ability della v0.1, con la composizione derivata dal loro comportamento reale:

| Ability | Composizione |
|---|---|
| `Hero.Gadget.ArcPulse` | BasicAttack + Electric — pulse breve |
| `Hero.Gadget.LinearDischarge` | Line + Electric |
| `Hero.Gadget.ConductiveNode` | Cell/Object + Electric — setup, cue di nodo/rete |
| `Hero.Gadget.Overload` | Circle + Electric + Damage |
| `Hero.Gadget.ReactiveCapacitor` | Reaction + Shield + Electric |
| `Hero.Phase.PressureJet` | Line + Water + Push |
| `Hero.Phase.CircularTide` | Circle + Water + Heal |
| `Hero.Phase.FluidTrail` | Dash + Water — scia |
| `Hero.Phase.MistVeil` | Circle + Water→Smoke, cue di occultamento |
| `Hero.Phase.FlowReaction` | Reaction + Water + Reposition |
| `Hero.Riktor.ImpactShot` | BasicAttack + impatto cinetico |
| `Hero.Riktor.KineticPanel` | Cover creato + pannello direzionale |
| `Hero.Riktor.Reconfigure` | Cover + cue di rotazione |
| `Hero.Riktor.Ram` | Dash + Push + Damage |
| `Hero.Riktor.Interposition` | Reaction + Ally + redirect |
| `Hero.Wraith.PulseShot` | BasicAttack + pulse cinetico |
| `Hero.Wraith.InterceptShot` | Reaction + Line/Cell controllata + stop movimento |
| `Hero.Wraith.PassingBlade` | Dash + Line + Damage |
| `Hero.Wraith.Deflection` | Reaction + deflessione — **non** un pool di Shield |
| `Hero.Wraith.Feint` | Predicted + Cell mark + Reposition |

Non si disegna un fulmine per ogni ability di Gadget, né un'onda per ogni ability di Phase. L'elemento è il
payload; a distinguere le cinque ability sono geometria e funzione.

### 3.1 La chiave di una ability

Le ability prendono un `IconId` regolare sotto la categoria `Action`:

```text
UI.Icon.Action.Hero.Gadget.LinearDischarge
```

Il validator (`URTIconLibrary::ValidateIconCatalog`) confronta il **prefisso** `UI.Icon.<Categoria>.` con
`StartsWith`, quindi il nome dopo la categoria può contenere punti e l'ID dell'ability resta riconoscibile.
Non serve un namespace separato, e non se ne inventa uno.

## 4. Payload, stato e superficie

Tre concetti vicini che il giocatore deve distinguere a colpo d'occhio. La differenza la fa il **frame**, non
il colore.

| Concetto | Che cos'è | Frame |
|---|---|---|
| Payload | l'elemento in sé (`Water`, `Fire`, `Electric`, `Ice`) | nessun frame — il glifo nudo |
| Superficie | una **cella** ha quella proprietà | il glifo dentro una **cella esagonale** |
| Status | una **unità** ha quello stato | il glifo dentro un **anello di stato** |

Così `Water` payload, superficie d'acqua e `Status.Wet` restano tre cose diverse anche in monocromia, e
nessuna delle tre è la ricolorazione di un'altra.

### 4.1 Quando il payload non entra nel frame

> Aggiunto il **2026-08-12**. La regola «cambia il frame, non il payload» è stata falsificata dalle prime
> due coppie prodotte, e va corretta invece che aggirata.

Un anello di stato lascia dentro **~11 px effettivi** a dimensione reale: metà della superficie di un glifo
nudo. Un payload da due o tre componenti che funziona da solo lì dentro diventa una macchia — verificato su
`Guarded`, `Braced` ed `Exposed`, tutte e tre riscritte a un solo elemento.

La regola corretta è quindi:

> Il payload di uno stato è una **riduzione dichiarata** del payload dell'azione che lo causa, e la parentela
> si mantiene sulla **famiglia di forma** — curvo contro angolare, aperto contro chiuso, pieno contro
> contorno — non sull'identità del disegno.

Le coppie azione → stato della v0.1, con la parentela esplicita:

| Azione | Stato | Che cosa si conserva |
|---|---|---|
| `Action.Guard` arco + corpo | `Status.Guarded` arco | la **curva aperta ai lati**: coperto sopra, scoperto di fianco |
| `Action.Brace` postura puntellata | `Status.Braced` cuneo | la **base larga**: peso in basso, stabile |
| `Action.Root` T rovesciata | `Status.Root` T rovesciata | identici: il payload regge in entrambi |
| `Action.Slow` chevron + freno | `Status.Slow` chevron + freno | identici |
| `Action.MarkTarget` rombo + mirino | `Status.Marked` rombo | il **rombo** |

Dove il payload regge in entrambi i contesti — `Root`, `Slow` — resta identico, e non c'è ragione di
inventare due disegni. Dove non regge, la riduzione va **scritta qui**, altrimenti diventa una divergenza
che nessuno riconosce come deliberata.

## 5. Che cosa non si compone

Alcune cose non sono glifi e non entrano in questa grammatica: la loro geometria dipende dallo stato di
gioco, e rasterizzarla produce un asset sbagliato appena la partita cambia.

Percorsi, celle raggiungibili, AoE di dimensione arbitraria, coni di facing e di Overwatch, linee di
targeting, catene, aree di incertezza, ghost del personaggio, riempimenti percentuali, numeri di cooldown e
di timer.

Per questi il design system fornisce **tile, mask, pattern, node, arrowhead, end-cap, border e marker** — i
pezzi con cui il sistema runtime costruisce la geometria finale. L'elenco completo di ciò che non va
rasterizzato è in [`progettazione-hud.md`](../../../../technical/progettazione-hud.md) §41.2, che resta
l'owner della pipeline verso Unreal.
