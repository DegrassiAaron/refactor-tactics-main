# Lane 2 — Simulation / Turn

> `SNAPSHOT` · **Data**: 2026-08-12 · **HEAD**: `59fa6f8a` (riallineato al merge)
> **Cosa è**: la sequenza di lavoro della lane *Simulation / Turn*, letta sul backlog **già aperto**.
> **Cosa non è**: una fonte di stato. Aperto/chiuso vive su GitHub; il `⏳`/`✅` qui dentro è la
> fotografia di una data. In caso di divergenza **vince GitHub**.
> **Fonte comune delle cinque lane**: [`roadmap-lane-index.md`](roadmap-lane-index.md).

---

## Perimetro

Ciò che il resolver **fa**: pipeline del turno, azioni, reazioni, combat, obiettivi, percezione,
bot, determinismo della risoluzione. Non lo spazio come dato (lane 1), non la presentazione (lane 3).

È la lane più carica del backlog v0.1, e la sua catena più lunga comincia con una **decisione**,
non con del codice.

---

## Il primo anello si è sciolto mentre questo file veniva scritto

⚠️ **`#501` e `#163` sono state chiuse il 2026-08-12**, poche ore dopo la stesura. La prima stesura
di questa sezione le indicava come il collo di bottiglia della lane; **non lo sono più**.

- `#501` (OW-5) è decisa → **D-109**: la v0.1 ammette **una** condizione dichiarata,
  `TargetHealthAtOrBelowPercent(N)`.
- `#163` (CP 14.3, modello unificato opportunity → commit) è chiusa.

**Il collo di bottiglia è ora `#583`**: D-109 ha dichiarato la condizione e CP 14.3 ne implementa la
**valutazione**, ma **nessuno la produce** — non esiste comando, planning UI o via del bot che la
scriva. È la firma del difetto ricorrente di questo repository, e adesso è il primo anello vero.

La catena che resta: `#512 → #170 → #171`, con `#512` **sbloccata** dalla chiusura di `#163`.

---

## Sequenza

### Catena A — Reazioni interattive (E14, `#152`)

```text
#501 ✅ ──> #163 ✅ ──> #512 ──> [lane 5] #170 ──> #171
                        CP 15.3

#165 ──┬──> #166
CP 14.5│    CP 14.6
       └──> #314
            CP 14.7
```

| Issue | Checkpoint | Stato | Dipende da | Prio |
|---|---|---|---|:--:|
| ~~`#163`~~ | CP 14.3 — Modello unificato opportunity → commit | ✅ **chiusa 2026-08-12** | — | P2 |
| `#165` | CP 14.5 — Finestra, commit e cablaggio di `Vektor.InterceptShot` | 🟢 **pronta** | CP 14.4 = `#164` ✅ | P2 |
| `#166` | CP 14.6 — Counterplay, UI della finestra, misura del pacing | ⏳ bloccata | `#165` | P2 |
| `#314` | CP 14.7 — Reaction Profile e Reaction Clash | ⏳ bloccata | `#165` · *segue* `#166` | P3 |
| `#319` | CP 14.8 — Decision Time Bank | 🟢 **pronta** (nessuna dipendenza dichiarata) | — | P3 |
| `#512` | CP 15.3 metà B — decisioni di finestra come dato, `DecisionProvider` iniettabile | 🟢 **sbloccata** dalla chiusura di `#163` | — | P1 |
| `#583` | La condizione dichiarata di **D-109** ha bisogno di un produttore | 🟢 **pronta — primo anello** | *consumatore*: `#163` ✅ · *gemella*: `#582` ✅ | P1 |

⚠️ **`#583` è la stessa famiglia di difetto che questo repository continua a pagare: il dato che
nessuno produce.** `D-109` (2026-08-12) ammette **una** condizione dichiarata in v0.1 —
`TargetHealthAtOrBelowPercent(N)` — e CP 14.3 ne implementerà la **valutazione**; ma *nessun*
checkpoint implementa la **dichiarazione**: non esiste comando, interfaccia di planning o via del
bot che la scriva. Va presa **insieme** a `#163`, non dopo, altrimenti il predicato nasce già orfano.

⚠️ **Il produttore può atterrare nella lane 3**: se la dichiarazione passa dall'interfaccia di
planning invece che dal bot, il lavoro è client. Da decidere prima di aprire il codice.

⚠️ `#512` è etichettata E15 (showcase) ma costruisce un **seam di simulazione** — per questo sta
qui e non nella lane 5, che la consuma. Vedi anche `#542` (D-101, «chi decide restituisce decisioni,
mai esiti»), che è `post-v0.1` ma descrive lo stesso confine.

### Catena B — Percezione (E13, `#151`)

| Issue | Checkpoint | Stato | Dipende da | Prio |
|---|---|---|---|:--:|
| `#159` | CP 13.4 — Rumore → contatto incerto, vista filtrata per squadra | 🟢 **pronta** | CP 13.2 `#157` ✅ · CP 13.3 `#158` ✅ | P2 |
| `#160` | CP 13.5 — Bot e HUD sulla conoscenza parziale | ⏳ bloccata | `#159` | P2 |

⚠️ `#160` è **mezza lane 3**: la sua metà HUD appartiene al client, e la issue lo dichiara.

### Catena C — Oggetti e obiettivi (E10, `#24`)

| Issue | Checkpoint | Stato | Dipende da | Prio |
|---|---|---|---|:--:|
| `#74` | CP 10.1 — `Activate` e `Interact` sugli oggetti | 🟢 **pronta** | `#71` ✅ · `#72` ✅ | P2 |
| `#75` | CP 10.2 — Obiettivo contestabile | ⏳ bloccata | `#74` | P2 |

**`#75` è una dipendenza della lane 5**: `#170` (golden replay) la aspetta.

### Catena D — Equipaggiamento e combat (E7, `#21`)

| Issue | Checkpoint | Stato | Dipende da | Prio |
|---|---|---|---|:--:|
| `#61` | CP 7.2 — Gadget | 🟢 **pronta** | `#60` ✅ | P2 |
| `#63` | CP 7.4 — Loadout, la regola 1+1+1 | 🟢 **pronta** | `#62` ✅ | P2 |
| ~~`#505`~~ | CP 7.5 — Trigger di reazione oltre il colpo diretto | ✅ **chiusa 2026-08-12** | — | P2 |
| `#509` | CP 7.6 — I delta di danno delle varianti passano alle fasce (D-087) | 🟢 **pronta** | — | P2 |

### Catena E — Bilanciamento Guard/Brace

| Issue | Stato | Dipende da | Prio |
|---|---|---|:--:|
| `#403` | `BAL-1`: decidere il confine fra Guard e Brace — 🟡 **serve l'autore** | seduta editor **U20** (lane 4) | P2 |
| `#404` | Applicare la decisione BAL-1 alle costanti di combat — ⏳ bloccata | `#403` | P2 |

⚠️ `#403` non si chiude con un documento: `D-074` ha già deciso la Fase 0 e gli scenari che servono
a scegliere **esistono e sono verdi**. Resta la seduta `PIE-BAL1` e la scelta fra le due opzioni
superstiti — status quo o ibrido.

---

## Ordine consigliato

1. **`#583`** — il produttore che manca a D-109: prenderlo tardi significa un predicato già orfano.
2. **`#512`** — P1, appena sbloccata, e apre la catena `#170 → #171` della lane 5.
3. **`#165`** — pronta, P2, e apre `#166`/`#314`.
4. **`#159`** — pronta, P2, chiude metà E13.
5. **`#74`** — pronta, e la lane 5 la aspetta via `#75`.
6. `#509`, `#61`, `#63` — pronte, indipendenti fra loro.

## Dipendenze fuori lane

| Da | Verso | Natura |
|---|---|---|
| `#512` | `#170` (lane 5) | il golden replay consuma le decisioni di finestra |
| `#583` | `#163` ✅ | il consumatore c'è già: manca solo chi scrive la condizione |
| `#75` | `#170` (lane 5) | idem |
| `#160` | HUD (lane 3) | metà del checkpoint è client |
| `#403` | seduta **U20** (lane 4) | la decisione si prende giocando |
