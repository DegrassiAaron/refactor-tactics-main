# Lane 7 — VFX / AssetFab

> `SNAPSHOT` · **Data**: 2026-08-12 · **HEAD**: `8b27afab`
> **Cosa è**: la sequenza di lavoro della lane *VFX / AssetFab*.
> **Cosa non è**: una fonte di stato — e, oggi, nemmeno una lane piena: vedi §1.
> **Fonte comune delle lane**: [`roadmap-lane-index.md`](roadmap-lane-index.md).

---

## 1. ⚠️ Questa lane è quasi vuota, ed è un fatto misurato

Prima di usarla, va detto cosa **non** c'è. Misurato il 2026-08-12 su `2d18422`:

| Cerchi | Trovi |
|---|---|
| una cartella `VFX/`, `Niagara/`, `Particles/` in `Content/` | ⛔ **nessuna**, a nessuna profondità |
| una feature con `VFX` nel `feature_id` | ⛔ **nessuna** in `feature-registry.yaml` |
| una issue aperta su VFX/effetti/particelle | ⛔ **nessuna** |
| gli asset di `Content/FabAsset/` | 🚫 **fuori da git per decisione** — `.gitignore:67` |
| `Content/RT/Characters/` | **2** file tracciati (`M_SelectionRing`, `M_TeamRing`); il resto **untracked** |

**Non è una lacuna da riempire: è la conseguenza di due scelte già prese.** La prima è `.gitignore:67` —
i 26 pack Paragon vivono in `Content/FabAsset/` e non entrano nel repository (≈44 GB; ⚠️ `git clean -fdx`
li cancella). La seconda è che il VFX **non è un gate della v0.1**: il canone assegna al resolver
l'autorità (*«le animazioni/VFX non decidono nulla»*) e la presentazione arriva con **E21**.

**Cosa la renderebbe una lane vera**, in ordine di costo: una decisione su cosa dei Fab entra in git
(§3), poi una feature nel registry, poi delle issue. Finché nessuna delle tre esiste, questo file è
**un perimetro dichiarato, non un piano** — e va letto sapendolo, altrimenti diventa il registro
parallelo che la [revisione delle lane](five-lane-roadmap-spec-panel-2026-08-11.md) ha respinto.

---

## 2. Perimetro

**Di cosa sono fatte le cose**: import e retarget degli asset Fab, materiali, effetti visivi, suono.

| | Lane 6 — Character | Lane 7 — VFX / AssetFab |
|---|---|---|
| Domanda | «chi è, e come si presenta?» | «di cosa è fatto, e da dove arriva?» |
| Oggetto | l'unità sulla cella | mesh, materiali, `NS_*`, `SFX_*` |
| Esempio | l'anello di squadra sotto i piedi | `M_TeamRing` che lo disegna |

Convenzione di naming già canonica (`piano-canonico-mvp.md` §242):
`M_`/`MI_` Material · `T_` Texture · **`NS_` Niagara** · `SFX_`/`MUS_` Suono/Musica · `L_` Level.

⚠️ **Il confine che questa lane non può attraversare** è lo stesso della lane 5, e viene dal canone: la
presentazione **non decide nulla**. Un VFX legge un evento già risolto; se un effetto influenzasse
l'esito, il difetto non sarebbe nel VFX ma nel fatto che qualcuno lo ha reso autorità.

---

## 3. Cosa esiste davvero, ed è il punto di partenza

### Gli agganci ci sono già, e sono in C++

`ARTTurnManager` espone delegate per la presentazione Blueprint:

```cpp
// Delegate per la presentazione in Blueprint (camera/VFX/SFX): il playback riproduce eventi gia' risolti.
/** Un'unita' viene eliminata VISIVAMENTE nel playback (per VFX/SFX di morte in Blueprint). */
```

Cioè il **contratto** fra simulazione e presentazione esiste; mancano i consumatori. È lo schema noto
del progetto — un meccanismo senza chi lo usa — ma **al contrario**, ed è la variante benigna: qui
l'assenza è dichiarata, non nascosta da un valore neutro.

### I due materiali tracciati

`Content/RT/Characters/Shared/Materials/M_SelectionRing.uasset` e `M_TeamRing.uasset` sono in git e
servono E21 (`#289`, leggibilità tattica).

### L'import in corso

`Content/RT/Characters/{Gadget,Phase,Riktor,Wraith}/` esistono sul disco e **non sono tracciati**: sono
personaggi Paragon in corso di import da un'altra sessione, per la seduta **U7**.

⚠️ **Qui c'è una domanda aperta e non registrata**: *cosa, dei Fab, entra in git?* `Content/FabAsset/` è
ignorato per intero, ma `Content/RT/Characters/` no — quindi un asset importato è tracciato o no a
seconda di **dove** finisce, e la regola non è scritta da nessuna parte. Con 44 GB da un lato e il
budget LFS già esaurito una volta su questo repo, non è una domanda da rimandare all'importazione
successiva.

---

## 4. Sequenza

Nessuna issue aperta appartiene a questa lane. Quello che c'è è **a monte**:

| # | Cosa | Dove vive oggi | Perché è qui |
|---|---|---|:--|
| 1 | **Decidere cosa dei Fab entra in git** | ⛔ non registrata | senza, ogni import è una scelta implicita e irreversibile a 44 GB |
| 2 | `#593` — il root di `ARTUnit` deforma ogni mesh | lane 6 | ogni asset importato prima della fix va rivisto dopo |
| 3 | Import/retarget dei Paragon per **U7** | seduta, lane 4 | in corso, untracked |
| 4 | Consumatori dei delegate VFX/SFX | ⛔ non registrato | il contratto esiste, nessuno lo usa |

⚠️ **La voce 1 è la sola che valga la pena aprire adesso**, e non è codice: è una domanda per
`OPEN_DECISIONS.md`. Le altre tre appartengono a lane che esistono già, e spostarle qui produrrebbe il
doppione che la revisione ha respinto.

---

## 5. Quando questa lane diventa reale

Tre condizioni, in ordine:

1. **la domanda del §3 ha una risposta** — cosa entra in git, e con quale meccanismo (LFS? blob? niente?);
2. **E21 consegna** (`#287`–`#289`, lane 6): prima non c'è niente su cui applicare un effetto;
3. **una feature entra nel registry** — a quel punto ha un `feature_id`, gate derivati e uno stato che
   non va scritto a mano.

Finché le tre non sono soddisfatte, il lavoro visivo della v0.1 vive nella **lane 6** e nelle **sedute**
della lane 4, che è dove sta già.

## Dipendenze fuori lane

| Da | Verso | Natura |
|---|---|---|
| tutta la lane | E21 (lane 6) | senza personaggi non c'è cosa vestire |
| import Paragon | seduta **U7** (lane 4) | è lavoro d'editor, non di codice |
| delegate VFX/SFX | `ARTTurnManager` (lane 2) | il contratto è C++, i consumatori Blueprint |
| `Client FPS` | `#41` (lane 1) | il KPI si misura **dopo** che mesh e materiali esistono |
