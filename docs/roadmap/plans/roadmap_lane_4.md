# Lane 4 — Editor / Tooling

> `SNAPSHOT` · **Data**: 2026-08-12 · **HEAD**: `3c4e48e`
> **Cosa è**: la sequenza di lavoro della lane *Editor / Tooling*, letta sul backlog **già aperto**.
> **Cosa non è**: una fonte di stato. Aperto/chiuso vive su GitHub, e lo stato delle **sedute**
> vive in [`../../technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md) + `editor-sessions.yaml`.
> In caso di divergenza **vince la fonte**, non questa fotografia.
> **Fonte comune delle cinque lane**: [`roadmap-lane-index.md`](roadmap-lane-index.md).

---

## Perimetro

Gli strumenti: editor mode della mappa, harness degli scenari, validator, e le **sedute** — il
lavoro che solo una persona davanti a Unreal può fare.

⚠️ **Questa lane non implementa regole di gioco.** Un tool che simulasse un risultato diverso dal
resolver sarebbe una seconda verità; la validazione competitiva resta runtime.

⚠️ **La `editormap` non si aggiorna a mano**: è `GENERATA` da
`python scripts/feature_registry.py shortlist`, che legge `editor-sessions.yaml`. Le sedute qui sotto
si citano per **ID**, mai per contenuto.

---

## Sequenza

### 1. `#38` · CP 2.8 — Playtest della partita hex (sessione D) 🟢 **pronta** · **P0**

**Epic**: `#16` (E2) · **Dipende da**: `#33` ✅ `#35` ✅ `#36` ✅ `#37` ✅ — tutte chiuse

⚠️ **Sblocca un'intera epic**: `#17` (E3, Dismissione del quadrato) dichiara nel proprio corpo
*«non iniziare prima che `#38` sia chiuso»*. È il P0 più economico della release — è una sessione,
non del codice.

### 2. `#451` · U1-C — Seduta: costruire `L_HexArena` e verificare le sette voci 🟢 **pronta** · **P1**

**Dipende da**: `#449` (U1-A, allowlist `.gitignore`) ✅ · `#450` (U1-B, criteri misurabili) ✅

⚠️ **Gli artefatti sono già committati** (`2e2caff`): `DA_HexMap_Arena.uasset` e `L_HexArena.umap`
sono tracciati, e per questo la seduta **U1** è passata da ⏳ a 🟡 nella `editormap`. Resta la
verifica delle sette voci — 0/7 al momento della fotografia.

### 3. `#82` · CP 12.2 — Matrice dei test manuali v0.1 (sessione E) 🟢 **pronta** · **P1**

**Epic**: `#26` (E12) · **Dipende da**: `#81` ✅ (CP 12.1, replay deterministico)

**Sblocca `#85`** (CP 12.5, release interna) insieme a `#84`. Il gate `G9` della Definition of Done
chiede il subset **`RELEASE-V01`**: **17 voci** marcate, contate col comando scritto nel gate — non
con un `grep` che conta anche la prosa.

### 4. Viz editor — la serie 1/4 → 4/4

**Brief**: [`../../technical/brief-editor-map-viz.md`](../../technical/brief-editor-map-viz.md)

| Issue | Cosa | Stato | Prio |
|---|---|---|:--:|
| `#551` | 1/4 · superficie e costo di movimento leggibili senza aprire un pannello | 🟢 **pronta**, **rivista il 2026-08-12** | P2 |
| `#552` | 2/4 · dare volume ai blocchi, distinguere «non si passa» da «non si vede» | 🟢 pronta | P2 |
| `#553` | 3/4 · coperture, porte e transizioni stanno sui **bordi**, e l'overlay non li mostra | 🟢 pronta | P2 |
| `#554` | 4/4 · le transizioni e la raggiungibilità — capire come si collegano i piani | 🟢 pronta | P2 |

⚠️ **Sono ordinate per frequenza sulla mappa, non da una dipendenza dichiarata**: nessuna delle
quattro blocca formalmente le altre. `#551` è stata riscritta dopo il merge di `#565` (vista a
livelli `Focus`) — vincolo tecnico e acceptance sono aggiornati, il resto della serie no.

### 5. `#582` · Harness: il perimetro delle capability di reazione 🟢 **pronta** · **P2**

*«…è superato da CP 14.4, e D-109 sta per aggiungere un campo senza produttore.»*

⚠️ Il titolo nomina da solo il difetto ricorrente del progetto — **un campo che nessuno legge**.
Vale la pena prenderla **prima** che D-109 atterri, non dopo.

### 6. Coda lunga

| Issue | Cosa | Stato | Prio |
|---|---|---|:--:|
| `#214` | `[EPIC] E19` · Classe di mappa e composizione | ⏳ | P2 |
| `#221` | `[EPIC] E17` · Validazione di stress 4v4 | ⏳ — **non è un gate di release** | P3 |

---

## Ordine consigliato

1. **`#38`** — P0, è una sessione, e sblocca l'epic `#17`.
2. **`#82`** — P1, alimenta la catena di release della lane 5.
3. **`#451`** — P1, gli artefatti ci sono già.
4. **`#582`** — prima che D-109 aggiunga il campo.
5. `#551` → `#552` → `#553` → `#554`.

## Dipendenze fuori lane

| Da | Verso | Natura |
|---|---|---|
| `#38` | epic `#17` (lane 1/2) | E3 non comincia prima |
| `#82` | `#85` (lane 5) | la release interna chiede la matrice manuale |
| seduta **U20** | `#403` (lane 2) | `BAL-1` si decide giocando, non leggendo |
| `#582` | `D-109` | il campo senza produttore sta per entrare |
