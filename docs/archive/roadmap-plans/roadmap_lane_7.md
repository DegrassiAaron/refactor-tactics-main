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
| gli asset di `Content/FabAsset/` | 🚫 **fuori da git per decisione** — `.gitignore:59-67` |
| `Content/RT/Characters/` | **2** file tracciati (`M_SelectionRing`, `M_TeamRing`); il resto **untracked** |

**Il vuoto riguarda il VFX, non la pipeline.** Il VFX **non è un gate della v0.1**: il canone assegna
l'autorità al resolver (*«le animazioni/VFX non decidono nulla»*) e la presentazione arriva con **E21**.
La pipeline degli asset, invece, esiste, è **decisa** e ha già prodotto lezioni — vedi §3.

**Cosa renderebbe questa lane una lane piena**: una feature nel registry e delle issue. Finché non
esistono, il lavoro asset vive dove sta già — le **sedute** della lane 4 e la documentazione tecnica —
e questo file è **un perimetro dichiarato, non un piano**.

> 🔴 **Correzione del 2026-08-12, e vale più del resto del file.** La prima stesura diceva che «cosa dei
> Fab entra in git» era *«una domanda aperta e non registrata»* e la proponeva come primo lavoro della
> lane. **È falso, tre volte**: la regola sta in `.gitignore` con venti righe di motivazione, in
> [`../../technical/tooling/convenzioni-contenuti-ue.md`](../../technical/tooling/convenzioni-contenuti-ue.md) §B.2/§B.2a,
> e nel README del magazzino. Non l'avevo cercata: avevo misurato l'**assenza** di VFX e dedotto
> l'assenza di una **regola**, che è un'altra domanda.

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

### La pipeline, che è la vera sostanza di questa lane

**Gli asset non entrano nel progetto: entrano in un magazzino, e da lì si importa solo ciò che serve.**

```text
Fab ──> refactor-tactics-main.vault ──> Migrate ──> refactor-tactics-main
        (RTVault.uproject, 3,4 GB,          solo l'asset
         NON un repository)                 e le sue dipendenze
```

Il magazzino è un progetto Unreal separato in `D:/Repositories/refactor-tactics-main.vault`, con un
`README.md` che ne è l'owner operativo. Esiste per una ragione verificata sulla documentazione ufficiale
(2026-08-10): **Fab non lascia scegliere la cartella** — installa sempre in `Content/<NomePack>/`, e
l'unica leva configurabile è *quale progetto*. Il magazzino è quel punto.

Il guadagno è concreto: `ParagonGideon` pesa **2,74 GB**, ma a un `BP_Unit` servono mesh, scheletro,
poche animazioni e i materiali. Il `Migrate` porta l'asset **e solo le sue dipendenze**.

**Cosa entra in git: quasi niente, e per scelta scritta.** `.gitignore` ignora i binari **per
estensione** (`*.uasset`, `*.umap`, `*.fbx`, `*.png`, `*.wav`…) e `Content/FabAsset/` **come directory**
— quest'ultima non per estensione, perché con un pattern per file git attraverserebbe ~40.000 file /
48 GB a ogni `git status`. Le **eccezioni sono nominate una per una** con la motivazione accanto:
`BP_GameMode` («un Blueprint è codice, non un asset d'arte»), la mappa di sviluppo e il suo data asset,
i due materiali degli anelli. Cinque file, 62 KB.

⚠️ **`git clean -fdx` cancella i pack** (`-x` include gli ignorati): si usa `git clean -fd`, oppure
`-e Content/FabAsset`.

Procedura completa e insidie: [`../../technical/tooling/convenzioni-contenuti-ue.md`](../../technical/tooling/convenzioni-contenuti-ue.md)
§B.2a, e il README del magazzino per il lato operativo.

### Le lezioni già pagate, che questa lane eredita

Portando `ParagonGadget` l'11-08-2026 sono emerse cinque cose che non si deducono:

1. un asset di classe **`Rig`** — rimossa in UE 5.8 — **blocca l'intero rename**, che fallisce in blocco;
   si mette in quarantena, e il fallimento è innocuo;
2. un tentativo fallito **lascia la destinazione creata e vuota**, e il secondo giro si ferma perché
   «esiste già»;
3. `duplicate_directory` **non è l'alternativa che sembra**: perde gli stessi file *e in più* lascia
   riferimenti al path vecchio su 19 asset su 40 (contro 9 su 1229 del rename);
4. **la perdita di audio dipende dal pack** — su Gadget i `DialogueWave` `_Engage_` sono passati da 25 a
   13, su Gideon niente: non si prevede, si misura;
5. lo **skeleton si legge dalla mesh**, non si cerca nella cartella.

> 🔴 E la regola che le riassume, già scritta nel README del magazzino: **un campione casuale non prova
> l'assenza di un difetto raro.** Per i riferimenti si scansiona **tutto** il pack; il campione conferma,
> non esclude.

### L'import in corso

`Content/RT/Characters/{Gadget,Phase,Riktor,Wraith}/` esistono sul disco e **non sono tracciati** — ed è
il comportamento voluto, non un'omissione: sono i personaggi Paragon della seduta **U7**, e i binari non
entrano in git.

---

## 4. Sequenza

Nessuna issue aperta appartiene a questa lane. Quello che c'è è **a monte**:

| # | Cosa | Dove vive oggi | Perché è qui |
|---|---|---|:--|
| 1 | `#593` — il root di `ARTUnit` deforma ogni mesh | lane 6 | ogni asset importato prima della fix va rivisto dopo |
| 2 | Import/Migrate dei Paragon per **U7** | seduta, lane 4 + magazzino | in corso; procedura in §B.2a |
| 3 | Consumatori dei delegate VFX/SFX | ⛔ non registrato | il contratto C++ esiste, nessuno lo usa |

⚠️ **Nessuna delle tre va spostata qui**: le prime due appartengono a lane che esistono già, e la terza
non ha ancora un soggetto — E21 deve consegnare prima che un effetto abbia qualcosa su cui attaccarsi.
Spostarle produrrebbe il doppione che la revisione ha respinto.

**La voce 3 è la sola candidata a diventare la prima issue di questa lane**, e non prima di E21.

---

## 5. Quando questa lane diventa reale

Due condizioni, in ordine — la terza che questo file elencava *(«decidere cosa entra in git»)* era già
soddisfatta prima che la lane esistesse:

1. **E21 consegna** (`#287`–`#289`, lane 6): prima non c'è niente su cui applicare un effetto;
2. **una feature entra nel registry** — a quel punto ha un `feature_id`, gate derivati e uno stato che
   non va scritto a mano.

Finché le due non sono soddisfatte, il lavoro visivo della v0.1 vive nella **lane 6** e nelle **sedute**
della lane 4, che è dove sta già.

## Dipendenze fuori lane

| Da | Verso | Natura |
|---|---|---|
| tutta la lane | E21 (lane 6) | senza personaggi non c'è cosa vestire |
| import Paragon | seduta **U7** (lane 4) | è lavoro d'editor, non di codice |
| import Paragon | `refactor-tactics-main.vault` | il magazzino è il passo obbligato: Fab non sceglie la cartella |
| delegate VFX/SFX | `ARTTurnManager` (lane 2) | il contratto è C++, i consumatori Blueprint |
| `Client FPS` | `#41` (lane 1) | il KPI si misura **dopo** che mesh e materiali esistono |
