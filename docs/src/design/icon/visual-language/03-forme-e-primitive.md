# Visual Language — Forme e primitive

> **Statuto**: sorgente di design, non canone. Vedi [`01-principi.md`](01-principi.md).
>
> Questo documento è l'**asse grammatica**. Le primitive qui elencate **non hanno un `IconId`** e non si
> risolvono a runtime: sono il vocabolario con cui si disegna un glifo. Le chiavi risolvibili stanno in
> [`08-catalogo-v0.1.md`](08-catalogo-v0.1.md).

## 1. Dove è finita ogni famiglia

Le tredici famiglie minime richieste dal brief si distribuiscono sui due assi. Non è una riduzione di scope:
è la separazione che rende il sistema verificabile da una macchina.

| Famiglia richiesta | Asse | Dove |
|---|---|---|
| Target | grammatica | §2 di questo documento |
| Geometry | grammatica | §3 |
| Damage | grammatica (effetto) | §4 |
| Defense | grammatica (effetto) | §4 |
| Movement | grammatica | §5 |
| Timing | grammatica (modificatore) | §6 + [`05-certainty-states.md`](05-certainty-states.md) |
| Action | **catalogo** | `ERTIconCategory::Action` |
| Reaction | **catalogo** | `ERTIconCategory::Reaction` |
| Status | **catalogo** | `ERTIconCategory::Status` |
| Environment | **catalogo** | `ERTIconCategory::Environment` |
| Information | **catalogo** | `ERTIconCategory::Information` |
| Warning | **catalogo** | `ERTIconCategory::Warning` |
| Objective | **catalogo** | `ERTIconCategory::Objective` |

Le cinque categorie canoniche che il brief non nominava — `Identity`, `Phase`, `MapInteraction`,
`Coordination`, `Certainty` — esistono comunque: sono in `ERTIconCategory`, e di queste la v0.1 popola
`Identity` e `Phase`. `Certainty` **non è una categoria di catalogo**: i suoi tre stati sono modificatori di
stile, e la ragione è in [`05-certainty-states.md`](05-certainty-states.md) §4. Vedi
[`08-catalogo-v0.1.md`](08-catalogo-v0.1.md).

## 2. Target

Chi o che cosa subisce l'azione.

| Primitiva | Silhouette |
|---|---|
| `Self` | unit marker con **inward** focus ring, distinto da Ally |
| `Ally` | unit marker rounded + connection tabs. Nessun `+` |
| `Enemy` | unit marker angular + reticolo/notch minimo |
| `Cell` | esagono 2D pulito, **nessuna prospettiva** |
| `Object` | blocco geometrico con base/anchor |
| `Direction` | chevron singolo o sector arrow — **non** confondere con `Move` |

`Ally` ed `Enemy` devono restare distinguibili in monocromia: la differenza è rounded contro angular, non la
tinta.

## 3. Geometry

La forma che l'azione occupa nello spazio.

| Primitiva | Silhouette |
|---|---|
| `Line` | `● ─── ►` — origine, segmento, punta. **Senza nodi intermedi** |
| `Circle` | anello esterno + punto centrale |
| `Cone` | origine + due bordi divergenti + boundary curvo |
| `Chain` | nodo origine + 2–3 nodi collegati con salto segmentato |
| `Arc` | sector boundary curvo + anchor di origine/facing |

`Chain` **non** si disegna come un fulmine solitario: il fulmine appartiene a `Electric`, e una catena senza
nodi visibili perde ciò che la distingue da una linea.

## 4. Effect

Che cosa accade al bersaglio. Include le famiglie `Damage` e `Defense` del brief.

| Primitiva | Silhouette |
|---|---|
| `Damage` | impact/crack astratto, **3–4 diramazioni max**. No spada, proiettile, teschio, fiamma |
| `Heal` | pulse/croce soft geometrica. Usato **solo** per Heal, mai per `Ally` |
| `Push` | `» ● ─►` — impulso esterno, unit dot, uscita |
| `Pull` | `◄─ ● «` — impulso inverso |
| `Shield` | scudo semplice con ampio negative space. **Non** Cover, **non** Brace |
| `Cover` | segmento/barriera tattica con cue di protezione laterale |
| `Interrupt` | break/notch che spezza una linea o un'azione |
| `Reposition` | freccia breve origine→destinazione, distinta dal path di `Move` |
| `Reveal` | sensor/radar mark, distinto dall'occhio+settore di `Overwatch` |

### 4.1 Le quattro difese

`Cover`, `Guard`, `Brace` e `Shield` sono quattro concetti separati, tutti presenti nel gioco. È la collisione
semantica più probabile dell'intero sistema.

| Concetto | Che cos'è | Cue visivo |
|---|---|---|
| `Cover` | proprietà della **mappa** | barriera direzionale, ancorata al terreno |
| `Guard` | **azione** generica: difesa direzionale, decade fuori dall'arco frontale | postura + arco frontale |
| `Brace` | **azione** generica: prepararsi a ricevere l'impatto | body/anchor + cuneo di contrasto |
| `Shield` | **risorsa/effetto**: un pool che assorbe | scudo geometrico, negative space |

`Brace` non riusa il glifo di `Shield`. `Guard` non riusa quello di `Cover`.

## 5. Movement

| Primitiva | Silhouette | Note |
|---|---|---|
| `Move` | `●──•──•──►` | i **nodi intermedi** sono ciò che la definisce |
| `Sprint` | stessa famiglia di `Move`, stride più lungo, doppia speed trail, endpoint aggressivo | mai `Move` con un `x2` |
| `Dash` | `●──»──»──►` — a 16 px: `● » ►` | movimento speciale, evasivo, rapido |
| `Forced Movement` | unit dot + impulso **esterno** | usa la grammatica `Push`/`Pull`, **non** `Dash` |

La distinzione che conta di più: `Dash` è una scelta dell'unità, `Forced Movement` è subìto. Se leggono
uguale, il giocatore non capisce chi ha deciso.

`Sprint` merita una nota di gameplay perché la sua icona può mentire: `Action.Sprint` consuma **entrambi** gli
slot e **nega la reazione** per il turno. Non è «Move ma più veloce» — è «Move al posto di tutto il resto».
La grammatica lo suggerisce con un endpoint che chiude, non con una freccia più lunga.

## 6. Modifier

Modificatori di stile applicati sopra una primitiva, non icone separate.

| Modificatore | Reso |
|---|---|
| `Confirmed` | tratto solido, fill normale |
| `Predicted` | tratteggio, fill hollow, opacità ghost |
| `Uncertain` | tratto puntinato/discontinuo, fade, `?` secondario |
| `Invalid` | slash / cross-hatch / `⊘`, neutro muted |
| `Disabled` | desaturazione + frame disabled, silhouette **ancora leggibile** |

Il dettaglio è in [`05-certainty-states.md`](05-certainty-states.md). `Disabled` non si ottiene abbassando
tutto al 10–15% di opacità: un'icona illeggibile non comunica «non disponibile», comunica «rotto».

## 7. Collisioni da testare senza label

Ogni coppia deve restare distinguibile in grayscale, a 24 px, senza testo.

- `Line` ↔ `Move`
- `Move` ↔ `Sprint` ↔ `Dash`
- `Electric` ↔ `Reaction`
- `Shield` ↔ `Brace` ↔ `Cover` ↔ `Guard`
- `BasicAttack` ↔ `Overwatch`
- `Wait` ↔ `Hold`
- `Water` ↔ `Fire`
- `Status.Wet` ↔ superficie d'acqua
- `Status.Electrified` ↔ payload `Electric`
- `Status.Burning` ↔ superficie di fuoco
- `Ally` ↔ `Enemy`
- `Confirmed` ↔ `Predicted` ↔ `Uncertain`
- `Push` ↔ `Dash`
- `Interact` ↔ interazione con obiettivo
