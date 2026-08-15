# Le cinque lane — indice e regole di lettura

> `SNAPSHOT` · **Data**: 2026-08-12 · **HEAD**: `3c4e48e`
> **Cosa è**: la vista d'insieme dei cinque file di lane, e le regole che li rendono leggibili senza
> credere ai loro simboli.
> **Cosa non è**: una fonte di stato, né un modello di ownership. Vedi § *Cosa NON sono le lane*.

| Lane | File | Perimetro | Prossima |
|---|---|---|---|
| **1** | [`roadmap_lane_1.md`](roadmap_lane_1.md) | Spatial / Map | `#41` CP 3.3 — 🟢 P0 |
| **2** | [`roadmap_lane_2.md`](roadmap_lane_2.md) | Simulation / Turn | `#501` decisione OW-5 — 🟡 serve l'autore |
| **3** | [`roadmap_lane_3.md`](roadmap_lane_3.md) | Client / UX | `#77` CP 11.1 — 🟢 P1 |
| **4** | [`roadmap_lane_4.md`](roadmap_lane_4.md) | Editor / Tooling | `#38` CP 2.8 — 🟢 P0 |
| **5** | [`roadmap_lane_5.md`](roadmap_lane_5.md) | Replay / Audit | `#83` CP 12.3 — 🟢 P0 |

---

## 1. Cosa NON sono le lane

Il termine viene dal sorgente
[archiviato](../../archive/src/handoff/2026-08-11-five-lane-roadmap-editor-replay.md) e revisionato
in [`five-lane-roadmap-spec-panel-2026-08-11.md`](five-lane-roadmap-spec-panel-2026-08-11.md), che
proponeva molto di più e di cui **quasi nulla è stato adottato**. Va detto qui, perché è il posto in
cui qualcuno leggerà la parola «lane» per la prima volta:

- ❌ **non sono ownership di file** — l'impianto proposto assegnava 51 path alle lane e **45 non
  esistono**;
- ❌ **non sono branch né worktree** — l'attore primario è **una persona sola**, e cinque sessioni
  concorrenti moltiplicano un guasto già misurato (otto collisioni di contatore `D-nnn`);
- ❌ **non sono milestone** — `F0`–`F6` e `F4.5` aprirebbero un **quarto** asse di numerazione
  accanto a `M6`–`M11`, `E1`–`E36` e `CP x.y`;
- ❌ **non introducono gate** — `G0`–`G10` collidono tutti con `G1`–`G15` già in uso, tre con
  significato *quasi* uguale, che è peggio di una collisione netta;
- ❌ **non esistono label `lane:*`** su GitHub, e non sono state create.

✅ **Sono una chiave di lettura del backlog che esiste già.** Al 2026-08-12 le issue aperte con
label `v0.1` sono **58**: la domanda «qual è la prossima» si risponde **scegliendo**, non creando.

⚠️ **Quel numero è passato da 57 a 58 mentre questi file venivano scritti** — `#583` è nata a
`00:57`. Non è un aneddoto: è la ragione per cui l'intestazione di ogni file dice `SNAPSHOT` e
rimanda al comando `gh`. Un conteggio scritto a mano è già vecchio quando lo rileggi.

---

## 2. Come si rilegge lo stato senza fidarsi di questi file

I cinque file sono **fotografie datate**. Ogni `🟢`/`⏳`/`✅` è vero al 2026-08-12 su `3c4e48e` e
invecchia da solo — è precisamente il difetto che questo repository ha già pagato quattro volte, e
per questo è dichiarato invece che nascosto.

```bash
# lo stato reale, sempre
gh issue list --state open --label v0.1 --limit 100 \
  --json number,title --jq '.[] | "\(.number)\t\(.title)"'

# le dipendenze dichiarate nel corpo di una issue
gh issue view <N> --json body --jq '.body' | head -20
```

**In caso di divergenza vince GitHub**, e per le sedute vincono
[`../../technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md) e `editor-sessions.yaml`.

⚠️ **Le viste generate non si toccano**: `featuremap.shortlist.md`, `scenariomap.shortlist.md` ed
[`../editormap.shortlist.md`](../editormap.shortlist.md) portano l'intestazione `GENERATA` e si
riscrivono con `python scripts/feature_registry.py shortlist`. Questi cinque file **non** sono
generati, e per questo non sono autorità.

---

## 3. Il grafo fra le lane

Le dipendenze che attraversano i confini sono poche e vale la pena vederle insieme:

```text
        lane 1                lane 2               lane 4            lane 5
        ──────                ──────               ──────            ──────
         #41 ──────────────────────────────────────────────────────> #84 ──> #85
          ↑                                                           ↑       ↑
       E21 #287                                        #82 ───────────┼───────┘
      (lane 3)                                                        │
                                                                     #83
                          #501 ─> #163 ─> #512 ──────────────────────────> #170 ─> #171
                          #74 ──> #75 ─────────────────────────────────────┘

                          #403 <── seduta U20 (lane 4)
                          #160 ──> metà HUD (lane 3)
        #570 ─────────────────────────────────────────────────> corpus golden (lane 5)
```

**Tre cose che il grafo rende evidenti e le singole issue no:**

1. **`#38` è il P0 più economico della release**: è una sessione di playtest, non del codice, e
   `#17` (E3) dichiara di non cominciare prima che sia chiusa.
2. **La catena più lunga comincia con una decisione**, `#501`, che nessuna quantità di codice
   chiude. Dietro ci sono quattro checkpoint.
3. **Due correzioni in due lane diverse aprono e chiudono la stessa finestra**: `#570` (lane 1) e
   `#625` (lane 5) toccano entrambe l'applicazione ambientale e possono aggiungere una voce di
   TurnLog. ⚠️ **Misurato**: oggi non romperebbero niente — il corpus pinnato sono due `.rttl` di
   soli scenari di movimento — ma dopo `#170`, che pinna otto turni, costeranno una rigenerazione
   motivata. Il momento a costo zero è **adesso**, ed è lo stesso argomento di `D-084`.

---

## 4. Cosa manca a questa vista

⚠️ **Le dipendenze non dichiarate non compaiono.** `#287`–`#289` (E21) sono sequenziali per
chiunque legga i titoli e **nessuna issue lo scrive**; la serie `#551`–`#554` è ordinata per
frequenza sulla mappa e non da un vincolo. Se contano, vanno dichiarate nelle issue — non qui,
dove nessuno strumento le leggerà.

✅ **Nessuna voce senza numero.** La lane 5 conteneva l'unica proposta senza issue — il danno da
hazard fuori dalla traccia canonica — ed è stata **aperta come `#625`** il 2026-08-12, lo stesso
giorno in cui è emersa. È deliberato: il Decision Log ha già registrato **due** casi di prescrizioni
scritte in una colonna e mai diventate issue, e una riga di piano non è un lavoro tracciato.
