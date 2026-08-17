# Lane 6 — Character

> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> `SNAPSHOT` · **Data**: 2026-08-12 · **HEAD**: `8b27afab`
> **Cosa è**: la sequenza di lavoro della lane *Character*, letta sul backlog **già aperto**.
> **Cosa non è**: una fonte di stato. Aperto/chiuso vive su GitHub; il `🟢`/`⏳`/`✅` qui dentro è la
> fotografia di una data. In caso di divergenza **vince GitHub**.
> **Fonte comune delle lane**: [`roadmap-lane-index.md`](roadmap-lane-index.md).

---

## Perimetro

**L'unità come oggetto nel mondo**: scheletro, mesh, animazioni, anelli team/selezione, dati del roster,
profili e radar di personaggio, stati e trasformazioni.

Il confine con la **lane 3** non è «personaggi vs interfaccia» ma **dove vive il pixel**:

| | Lane 3 — Client / UX | Lane 6 — Character |
|---|---|---|
| Spazio | **schermo**: HUD, camera, input, ghost, log | **mondo**: ciò che sta sulla cella |
| Domanda | «il giocatore capisce cosa succede?» | «chi è, e come si presenta?» |
| Esempio | barra HP nel widget | l'anello di squadra sotto i piedi |

⚠️ **E21 si sposta qui.** `#287`–`#289` erano elencati nella lane 3 alla stesura del 2026-08-12 mattina:
mesh, animazioni e leggibilità tattica sono l'unità nel mondo, non l'interfaccia. La lane 3 conserva il
legame perché **E21 le serve**: finché i personaggi sono cilindri, il KPI `FPS client` di `#41` (lane 1)
è un valore pre-presentazione.

---

## Il primo anello è un bug, e blocca tutto il resto

### `#593` · Il cilindro segnaposto è il root di `ARTUnit` 🟢 **pronta** · **P2** · `bug`

```cpp
Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
SetRootComponent(Mesh);                              // RTUnit.cpp:19
Mesh->SetRelativeScale3D(BaseMeshScale);             // RTUnit.cpp:22
FVector BaseMeshScale = FVector(1.2f, 1.2f, 1.8f);   // RTUnit.h:606
```

Il segnaposto **è il root**, e porta una scala non uniforme: qualunque componente aggiunto in Blueprint la
eredita, e una Skeletal Mesh viene stirata in altezza di `1.8 / 1.2 = **1,5×**`. Riscontrato in PIE il
2026-08-12 sui quattro `BP_Unit_*` della seduta **U7**: i personaggi appaiono «stretti e allungati».

⚠️ **Va preso per primo, e non per gravità ma per posizione**: è a monte di ogni voce di questa lane. Ogni
mesh importata prima della correzione va rivista dopo, e ogni giudizio di leggibilità dato su un personaggio
deformato non vale.

---

## Sequenza

### Catena A — Presentazione (E21, `#286`)

| Issue | Checkpoint | Stato | Dipende da | Prio |
|---|---|---|---|:--:|
| `#287` | CP E21.1 — Personaggi sui centri esagonali | 🟢 pronta | ⚠️ **`#593` di fatto** | P1 |
| `#288` | CP E21.2 — Animazioni di locomozione e impatto | ⏳ | `#287` *(non dichiarata)* | P1 |
| `#289` | CP E21.3 — Leggibilità tattica | ⏳ | `#288` *(non dichiarata)* | P1 |

⚠️ **Nessuna delle tre dichiara `#593` come dipendenza, e nessuna dichiara le altre due.** L'ordine è
ovvio a chi legge i titoli e invisibile a qualsiasi strumento. Se conta, va scritto nelle issue.

**Seduta collegata**: **U7** «Personaggi Paragon» in [`../../roadmap/editor-sessions.yaml`](../../roadmap/editor-sessions.yaml) —
è lì che il difetto di `#593` è stato visto.

### Catena B — Roster e dati

| Feature / Issue | Cosa | Stato | Release |
|---|---|---|---|
| `RT-FEAT-CHAR-V01-ROSTER` | Gadget, Phase, Riktor, Wraith | `INTEGRATED` | v0.1 |
| `RT-FEAT-CHAR-V02-ROSTER` | Steel, Aurora, Murdock, Kwang | `DESIGNED` | v0.2 |
| `#322` | `[EPIC v0.2] E35` · Roster 8: Sentinel Directorate e Resonance | ⏳ **P0** post-v0.1 | v0.2 |
| `#645` | `Action.Leap` è la quarta capacità del motore irraggiungibile dal roster | 🟢 `question` P2 | v0.1 |

⚠️ **`#645` è di questa lane anche se sembra di azioni**: la capacità esiste nel motore e **nessun eroe la
raggiunge**. È la firma del difetto ricorrente del progetto — il dato che nessuno consuma — visto dal lato
del roster. Vedi anche `#464` (il ramo difensivo del bot che nessun eroe attraversa).

### Catena C — Radar e profili (E37, `#555`)

| Issue | Cosa | Stato | Prio |
|---|---|---|:--:|
| ~~`#556`~~ | CP 37.1 · Quale workbook è **autorità** sui rating 1..10 (`RAD-1`) | ✅ **chiusa** — `RAD-1` decisa | P3 |
| `#557` | CP 37.2 · Rubrica di conversione kit/stats → rating (`RAD-2`) | ⏳ | P3 |
| `#558` | CP 37.3 · Rating canonici Balance del roster v0.1 | ⏳ | P3 |
| `#559` | CP 37.4 · Schema dei rating e validator | ⏳ | P3 |
| `#560` | CP 37.5 · Generatore SVG deterministico | ⏳ | P3 |
| `#561` | CP 37.6 · Radar di confronto A vs B | ⏳ | P3 |
| `#562` | CP 37.7 · Assi Profile senza fonte (`RAD-3`) | 🟡 `question` | P3 |
| `#563` | CP 37.8 · Integrazione dei radar nella Wiki (`RAD-5`) | ⏳ | P3 |

Feature: `RT-FEAT-CHAR-RADAR-MODEL`, `RT-FEAT-CHAR-RADAR-RATINGS-V01` (`SPECIFIED`),
`RT-FEAT-WIKI-CHART-GENERATOR`.

⚠️ **Tutta v0.4**, e la catena si è già mossa: `RAD-1` — *quale workbook è autorità* — **è decisa**
(`#556` chiusa), quindi `#557` è il primo anello vero. Resta aperta `RAD-3` (`#562`, gli assi Profile
senza fonte): costruire il generatore prima di averla chiusa significa generare da una fonte che
potrebbe non essere autorità.

### Catena D — Stati e unità ausiliarie

| Feature / Issue | Cosa | Stato | Release |
|---|---|---|---|
| `#244` | `[EPIC] E34` · Character State / Configuration System | ⏳ P3 | post-v0.1 |
| `RT-FEAT-CHARACTER-STATE` | — | `SPECIFIED` · 5 scenari `planned` | v0.2 |
| `RT-FEAT-CHAR-TRANSFORMATION` | stance e trasformazioni | `IDEA` | v0.2 |
| `RT-FEAT-CHAR-AUXILIARY-UNITS` | pet, evocazioni, gadget | `DESIGNED` | v0.2 |

### Catena E — Fazioni

`RT-FEAT-FACTION-SYSTEM` e `RT-FEAT-FACTION-SCENARIOS`, entrambe `DESIGNED`, con **4 scenari `planned`**.
Perimetro condiviso con la lane 3 per l'iconografia (`#217`/`#219`/`#220`, HUD Icon Language).

---

## Ordine consigliato

1. **`#593`** — sblocca di fatto tutta la catena A, ed è un bug di dieci righe con una conseguenza visibile.
2. **`#287`** subito dopo, non prima.
3. **`#645`** — è una domanda, non codice: risponderla evita di costruire un eroe attorno a una capacità
   che nessuno userà.
4. `#288` → `#289`.

Il resto (radar, stati, fazioni, roster 8) è **post-v0.1** e non va anticipato.

## Dipendenze fuori lane

| Da | Verso | Natura |
|---|---|---|
| `#593` | seduta **U7** (lane 4) | il difetto si vede in PIE, non in un test |
| E21 (`#287`–`#289`) | `#41` (lane 1) | `FPS client` non è stabile finché la scena è cilindri |
| E21 | lane 7 | senza mesh importate e retargettate non c'è niente da mostrare |
| fazioni | `#219`/`#220` (lane 3) | l'iconografia è un catalogo semantico condiviso |
