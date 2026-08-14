# Lane 1 — Spatial / Map

> `SNAPSHOT` · **Data**: 2026-08-12 · **HEAD**: `59fa6f8a` (riallineato al merge)
> **Cosa è**: la sequenza di lavoro della lane *Spatial / Map*, letta sul backlog **già aperto**.
> **Cosa non è**: una fonte di stato. Aperto/chiuso vive su GitHub; il `⏳`/`✅` qui dentro è la
> fotografia di una data e invecchia da sola. In caso di divergenza **vince GitHub**.
> **Fonte comune delle lane**: [`roadmap-lane-index.md`](roadmap-lane-index.md).

**Come si rilegge lo stato senza fidarsi di questo file:**

```bash
gh issue list --state open --label v0.1 --limit 100 \
  --json number,title,body --jq '.[] | "\(.number)\t\(.title)"'
```

---

## Perimetro

Lo spazio **come dato e come query**: celle, grafo, LOS, pathfinding, superfici, coperture, porte,
ponti, asset di mappa. Non ciò che il resolver ne fa (lane 2), non come si vede (lane 3).

⚠️ **Questa lane è quasi finita per la v0.1, ed è un fatto, non un'impressione**: le epic **E2**
(parità hex), **E8** (terreni, stati, ambiente) e **E9** (coperture e strutture) sono **chiuse**.
Restano due voci, e una è una misura, non una costruzione.

---

## Sequenza

### 1. `#41` · CP 3.3 — Misurazione dei budget su hex 🟢 **pronta** · **P0**

**Epic**: `#17` (E3 — Dismissione del quadrato) · **Dipende da**: `#40` ✅ chiusa

Sostituisce i target mitici con numeri reali, **prima** che entri il contenuto della v0.1.

**DoD** — quattro misure su mappa `r=4` multilivello con 4 unità, in Development:

- [ ] FPS client
- [ ] path mediana
- [ ] preview completa
- [ ] tempo del resolver per turno
- [ ] valori nella tabella KPI di [`../../roadmap/v0.1-definition-of-done.md`](../../roadmap/v0.1-definition-of-done.md) §4, con data e metodo
- [ ] un valore **fuori target** si registra come tale, non si nasconde né si arrotonda

⚠️ **Tre KPI su quattro sono misurabili adesso; `FPS client` no** — o meglio: il numero che misuri
oggi **cambierà** quando atterra **E21** (`#286`–`#289`, personaggi e animazioni), perché oggi la
scena sono cilindri. Misurarlo resta giusto — il gate chiede di **avere** i numeri, non di centrarli
— ma va registrato come valore **pre-presentazione**, altrimenti a E21 chiuso nessuno saprà se la
regressione è reale.

**Sblocca**: `#84` (CP 12.4, KPI misurati) → `#85` (CP 12.5, release interna) — la catena P0 della
lane 5.

### 2. ~~`#570`~~ · Una superficie che nasce sotto un'unità non le fa niente ✅ **CHIUSA il 2026-08-12**

**Epic**: `#22` (E8, chiusa) · ⚠️ **Chiusa poche ore dopo la stesura di questo file** — resta qui
per il legame con `#625`, non come lavoro da fare.

Difetto di applicazione ambientale: l'effetto di una superficie si applica a **chi entra**, non a
chi ci si trova già quando la superficie nasce sotto di lui.

⚠️ **Tocca la lane 5 senza sembrarlo**: se la correzione fa applicare uno status in più durante la
risoluzione, produce una voce di TurnLog in più e gli hash di quei turni cambiano.

**Misurato il 2026-08-12: oggi non romperebbe niente.** Il corpus golden pinnato sono **due** file
`.rttl` di soli scenari di movimento; `ShowcaseRelay` confronta run contro run **nella stessa
esecuzione**; nessun test pinna un conteggio di voci. Quindi non è un rischio da gestire ma una
**finestra**: dopo `#170` (lane 5), che pinna otto turni, la stessa correzione costerà una
rigenerazione motivata. Stessa situazione di `#625`, e prese insieme si rigenera una volta sola.

---

## Dipendenze fuori lane

| Da | Verso | Natura |
|---|---|---|
| `#41` | `#84` → `#85` (lane 5) | i KPI di release leggono questa misura |
| `#41` | **E21** (lane 3) | `FPS client` non è stabile finché la presentazione non c'è |
| `#570` | `#625` (lane 5) | stessa famiglia: applicazione ambientale che aggiunge una voce |

## Niente altro è in coda

Se questa lane si svuota, il lavoro spaziale successivo è **v0.2**: `E23` (muri, porte, interaction
graph, `#324`) e le tre feature `IDEA` del registry — acqua dinamica, strutture, verticalità. Non
vanno anticipate: non sono nello scope della release.
