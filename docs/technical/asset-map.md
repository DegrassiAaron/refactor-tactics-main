# Asset map — quali asset servono, chi li produce, quali esistono

> `CURRENT` · **Creato**: 2026-08-13 · **Owner**: questo file — è il **registro degli asset di contenuto**
> attesi dal progetto, release per release.
>
> **Cosa non è.** Non è l'owner di percorsi e naming: quello è
> [`convenzioni-contenuti-ue.md`](convenzioni-contenuti-ue.md), che è normativo e da cui questo file **deriva**
> ogni path. Non è l'owner dei principi di pipeline (presentazione-only, riferimenti soft con fallback,
> licenze): quello è [`spec-asset-pipeline.md`](spec-asset-pipeline.md). Non è l'owner dello stato delle
> sedute in editor: quello è [`../roadmap/editor-sessions.yaml`](../roadmap/editor-sessions.yaml), reso in
> [`../roadmap/editormap.shortlist.md`](../roadmap/editormap.shortlist.md).
>
> Nasce perché quelle tre fonti, insieme, **non rispondono a una domanda**: *quali asset servono e quanti ne
> mancano*. `convenzioni-contenuti-ue.md` §4 lo dichiara esplicitamente — «questo documento non è un
> tracker». Questo lo è.

---

## 1. Come si legge lo stato, e come si rimisura

Un asset ha tre stati possibili, e **due dei tre si misurano da soli**:

| Stato | Significa | Come si verifica |
|---|---|---|
| ✅ **committato** | è nel repository, chi clona lo ottiene | `git ls-files <path>` |
| 🟡 **su disco** | esiste nel progetto locale ma **non** è committato | esiste nel filesystem, non in `git ls-files` |
| ⏳ **assente** | non esiste ancora | nessuna delle due |

**La lista degli asset attesi non è un'opinione**: è l'allowlist di `.gitignore`. Il repository ignora tutto
`Content/**/*.uasset` e riammette per **path esplicito** ciò che deve entrare — quindi una riga `!Content/…`
è al tempo stesso il permesso e la dichiarazione d'intenti. Chi aggiunge un asset senza toccarla scopre che
`git add` non lo vede.

```bash
# stato di tutti gli asset attesi — si esegue dalla radice del repository
python - <<'PY'
import subprocess, os
root = subprocess.run(['git','rev-parse','--show-toplevel'],
                      capture_output=True, text=True).stdout.strip()
os.chdir(root)
allow = [l[1:].strip() for l in open('.gitignore', encoding='utf-8').read().splitlines()
         if l.startswith('!Content/') and not l.rstrip().endswith('/')]
# -z: i path con spazi o accenti non si spezzano e git non li quota
tracked = set(subprocess.run(['git','ls-files','-z','Content'],
                             capture_output=True, text=True).stdout.split('\0')) - {''}
for a in sorted(allow):
    print(('OK      ' if a in tracked else ('DISCO   ' if os.path.exists(a) else 'ASSENTE ')) + a)
extra = sorted(tracked - set(allow))
print(f"
{len(allow)} attesi · {sum(a in tracked for a in allow)} committati")
print(f"tracciati FUORI allowlist: {len(extra)}")
for e in extra:
    print('  ' + e)
PY
```

⚠️ La riga `tracciati FUORI allowlist` non c'era nella prima stesura, e la sua assenza aveva prodotto
un'affermazione falsa (§2). Un comando che itera solo la lista attesa non può scoprire ciò che la
lista non prevede: le due direzioni si controllano separatamente.

**Misurato il 2026-08-13** su `HEAD` `515c5c88`: **17 attesi · 13 committati · 4 non nel repository**.

⚠️ **Dei quattro mancanti, uno esiste sul disco di chi ha aperto la PR** (`ABP_Gadget.uasset`) e tre no.
Quella distinzione **non è riproducibile**: chi clona ed esegue il comando qui sopra legge
`0 su disco · 4 assenti`, e non è cambiato niente nel repository. Lo stato 🟡 è un fatto di una
macchina, non del progetto — utile a chi sviluppa, inutile a chiunque altro, e per questo la cifra che
conta è **13 su 17**.

⚠️ **Tre file tracciati stanno fuori dall'allowlist**: `Content/.gitkeep`,
`Content/RT_UI_AssetPack_FromHUD/README.md` e `.../manifest.json`. Non sono `.uasset` — le regole di
`.gitignore` che li lasciano passare sono altre — ma la prima stesura di questo documento dichiarava
«nessun asset è tracciato fuori dall'allowlist» perché il controllo escludeva `.md`, `.json` e
`.gitkeep`: aveva filtrato via esattamente i casi che lo smentivano.

---

## 2. v0.1 — misurata

I 17 path che il repository dichiara di volere. La colonna **Seduta** dice chi lo produce, secondo
`editor-sessions.yaml`; `—` significa che nessuna seduta lo rivendica (esisteva prima che le sedute
fossero un dato).

| Asset (sotto `Content/RT/`) | Famiglia | Seduta | Stato |
|---|---|:--:|---|
| `Characters/Gadget/Blueprints/BP_Unit_Gadget.uasset` | Unità giocabile | **U7** | ✅ committato |
| `Characters/Phase/Blueprints/BP_Unit_Phase.uasset` | Unità giocabile | **U7** | ✅ committato |
| `Characters/Riktor/Blueprints/BP_Unit_Riktor.uasset` | Unità giocabile | **U7** | ✅ committato |
| `Characters/Wraith/Blueprints/BP_Unit_Wraith.uasset` | Unità giocabile | **U7** | ✅ committato |
| `Characters/Gadget/Animation/ABP_Gadget.uasset` | Animazione | **U8** | 🟡 su disco, non committato |
| `Characters/Phase/Animation/ABP_Phase.uasset` | Animazione | **U8** | ⏳ assente |
| `Characters/Riktor/Animation/ABP_Riktor.uasset` | Animazione | **U8** | ⏳ assente |
| `Characters/Wraith/Animation/ABP_Wraith.uasset` | Animazione | **U8** | ⏳ assente |
| `Characters/Shared/Materials/M_SelectionRing.uasset` | Condiviso | — | ✅ committato |
| `Characters/Shared/Materials/M_TeamRing.uasset` | Condiviso | — | ✅ committato |
| `Maps/Dev/L_HexArena/L_HexArena.umap` | Mappa | **U1** | ✅ committato |
| `Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.uasset` | Mappa | **U1** | ✅ committato |
| `Maps/Dev/L_DevSandbox/L_DevSandbox.umap` | Mappa | — | ✅ committato |
| `Maps/Dev/L_DevSandbox/Data/DA_HexMap_Sandbox.uasset` | Mappa | — | ✅ committato |
| `Maps/Dev/L_Prototype/L_Prototype.umap` | Mappa | — | ✅ committato |
| `Core/Framework/BP_GameMode.uasset` | Framework | — | ✅ committato |
| `Art/GlobalMaterials/M_Global_Tint.uasset` | Materiale globale | — | ✅ committato |

**Quel che resta della v0.1 è una sola cosa: le quattro animazioni.** `ABP_Gadget` esiste sul disco di chi
sviluppa e non è committato — il caso peggiore dei tre, perché il gioco funziona in locale e si rompe per
chiunque cloni. Le altre tre non esistono. È esattamente il perimetro della seduta **U8**, ed è l'unica
famiglia della v0.1 con lavoro aperto.

### 2.1 Famiglie attese che non hanno ancora un path

Due cose che la v0.1 richiede e che **non stanno né nell'allowlist né in una seduta**. Non sono dimenticanze
di questo file: sono buchi delle fonti, e vanno chiusi lì.

| Famiglia | Chi la richiede | Perché manca un path |
|---|---|---|
| **Icone dell'HUD** | E20 · E11 | L'**insieme** richiesto è derivato e cresce da solo — `URTIconLibrary::RequiredIconIds()` lo compone dalle fasi volontarie e dal catalogo azioni *realmente in codice*, gate `RTIconCatalogTests` — ma **path e naming sono già decisi**: `/Game/RT/UI/Icons/`, `T_UI_Icon_<Categoria>_<Nome>` ([`brief-icone-v01.md`](brief-icone-v01.md) §33–34). Manca solo la riga d'allowlist, ed è un problema concreto: chi importa le texture al path già deciso fa `git add`, git tace, e le icone restano locali |
| **Sorgenti icona già sul disco** | E20 | `Content/RT_UI_AssetPack_FromHUD/` contiene **30 PNG** (`icons/I_Guard.png`, `I_Overwatch.png`, …) più `buttons/`, `panels/`, `tiles/`, `warnings/` e un `manifest.json` con box e margini 9-slice. Di tutto il kit **il repository traccia due file**: `README.md` e `manifest.json`. I trenta PNG no. È la famiglia più vicina a essere pronta, e la sola che nessuna riga d'allowlist prevede |
| **Livello illuminato del graybox** | seduta **U21** | U21 dichiara di produrre «il livello illuminato **committato**», ma ha `artifacts: []`: nessuno sa quale file sarà, quindi non può entrare nell'allowlist prima della seduta |
| **Kit graybox degli oggetti di mappa** | `RT-FEAT-UI-GRAYBOX-KIT` · `D-152` | La famiglia è **decisa e senza percorso**, ed è il caso che questa colonna descrive meglio. Il contratto esiste ([`spec-graybox-placement-contract.md`](spec-graybox-placement-contract.md)) e dice *che forma* devono avere gli asset; le convenzioni §5 non hanno una riga per un kit graybox di **oggetti** — coprono la griglia (`/Game/RT/World/Grid/`, dove `Generation/` sono i *generatori*) e le mappe, non le primitive riusabili che ci stanno sopra. Il percorso è `GBX-4` in [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md), e si chiude **prima** della seduta che produce il primo asset, non dopo: è la riga 1 di §6 |

---

## 3. v0.2 — derivata dalle epic, non dai file

Nessun asset della v0.2 esiste, e nessuno ha ancora un path: quello che segue è il **fabbisogno** che le epic
dichiarano, tradotto nei percorsi che le convenzioni impongono. Serve a dimensionare il lavoro, non a
committare niente.

| Famiglia | Epic | Quanti | Percorso previsto (§5 delle convenzioni) |
|---|:--:|--:|---|
| Unità giocabili dei 4 eroi nuovi | **E35** | 4 | `Characters/<Pack>/Blueprints/BP_Unit_<Pack>.uasset` |
| Animazioni dei 4 eroi nuovi | **E35** | 4 | `Characters/<Pack>/Animation/ABP_<Pack>.uasset` |
| Mappa di classe Standard 3v3 | **E24** | 1 + dati | `Maps/<Categoria>/<Nome>/` con il suo `DA_HexMap_*` |
| Icone: catalogo completo | **E25** | derivato | ancora senza path — vedi §2.1 |
| Muri e porte come oggetti | **E23** | ignoto | l'epic definisce il **modello logico**; se servano mesh dedicate non è deciso |

**Gli eroi sono già scelti e già speccati**: Steel e Murdock (Sentinel Directorate), Aurora e Kwang
(Resonance), in [`../characters/v0.2/`](../characters/v0.2/) — E35 li porta a runtime, non li inventa. Il
`<Pack>` è il **nome del pack Paragon**, non quello dell'eroe di gioco (§5b delle convenzioni) — e per i
quattro nuovi **è già dichiarato**: ogni scheda in `docs/characters/v0.2/` porta `Asset base: Paragon —
<Nome>` e `Hero_Key`. I percorsi sono quindi già risolvibili (`Characters/Steel/Blueprints/BP_Unit_Steel.uasset`
e simili), e le righe d'allowlist si possono scrivere **prima** che gli asset esistano: è la pratica già
adottata per U1, U7 e U8.

> ⚠️ **Oggi i dati dell'eroe non sono un asset committato, e la ragione va verificata prima di farne una
> regola.** In `Content/` non esiste nessun `DA_Hero_*`, e l'esempio in §5 delle convenzioni descrive dove
> *starebbe*, non un file che esiste. Ma [`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) descrive
> `URTHeroData` come `UPrimaryDataAsset` con asset `PDA_*` sotto `Content/RT/`: **le due fonti divergono**,
> e questo registro non è l'owner che può chiudere la divergenza.
> *(Una prima stesura citava `#375` a sostegno di «spedito da C++». È un'attribuzione sbagliata: #375 è il
> checksum di determinismo (CP 12.1). Il precedente vero riguarda `Format.Skirmish2v2`, non gli eroi.)*

---

## 4. v0.3 e oltre — quello che le fonti dicono davvero

⚠️ **Questa sezione è stata riscritta dopo il merge di #818 (D-136), che l'aveva smentita mentre
veniva scritta.** La prima stesura diceva «nel registry la v0.4 ha zero feature» e «sei feature
`future` non tracciate»: misure vere su `515c5c88`, false sull'albero in cui il documento è atterrato.
Il modello di release ora arriva alla **v1.0** e le release intermedie **v0.5–v0.8 esistono**.

| Release | Feature nel registry | Asset dichiarati | Perché |
|---|--:|---|---|
| **v0.3** *Informazione* | 5 | **nessuno** | percezione e bot: simulazione, non contenuto |
| **v0.4** *Operations* | **4** | 1 famiglia: mappa Operations (**E30**) | le altre epic sono formato e obiettivi |
| **v0.5 – v0.8** | 1 ciascuna | **non ancora esaminati** | release introdotte da D-136: il loro fabbisogno di asset non è stato misurato da questo registro |
| **v1.0** | — | **nessuno dichiarato** | — |
| `future` | **4** | acqua dinamica · strutture e crolli · verticalità · Control Center | le uniche rimaste senza release dopo D-136; le prime tre toccherebbero il mondo e richiederebbero asset ambientali |

**Il costo in asset resta quasi tutto nella v0.2**, ed è il roster: le quattro unità della v0.1 sono
costate quattro `BP_Unit` più quattro `ABP`, e il roster a 8 raddoppia quel lavoro in una release
sola. Dal lato contenuti il conto è lineare e noto: **8 asset**.

⚠️ **Le release v0.5–v0.8 sono un buco dichiarato di questo documento**, non del registry: sono nate
lo stesso giorno in cui è nato lui, e nessuno ha ancora chiesto quali asset richiedano.

> 🔵 **Aggiornamento 2026-08-17 — il buco si è ristretto da un lato solo, ed è quello di sopra.** Il
> consolidamento del Graybox Kit (`D-153`) ha misurato il fabbisogno di **oggetti di mappa** per tutte le
> release e la risposta è che **non atterra nella v0.5–v0.8**: da lì i temi canonici sono rete, GAS,
> dedicated server e hardening, e nessuno di essi è un tema di contenuto. Dei **sette** elementi `DEFER` del
> catalogo, **quattro** si appoggiano alla riga **`future`** qui sopra — `RT-FEAT-MAP-STRUCTURAL` (macerie,
> muri sfondati) e `RT-FEAT-MAP-VERTICALITY` (rampe, piattaforme) sono le due feature `IDEA` che li
> possiedono, e finché restano `IDEA` quegli asset non hanno un committente. Gli altri tre non c'entrano con
> `future` e hanno ciascuno una ragione propria: la **valvola** è fuori scope v0.1 dichiarato, generatore e
> serbatoio hazard sono proxy di elementi che nessuno produce ancora. **Non è un ritardo della pipeline**: è
> che sette voci su diciannove descrivono sistemi che il progetto non ha ancora deciso di costruire.
> La sola release che acquisisce un impegno nuovo è la **v1.0**, e non in asset: `E45` diventa il punto in
> cui il contratto di ingombro **si congela** perché l'arte finale possa sostituire il graybox senza
> cambiare le regole competitive.

## 5. Da dove vengono gli asset

Import da Fab attraverso un **magazzino** e `Migrate` — procedura vigente dal 2026-08-10,
[`convenzioni-contenuti-ue.md`](convenzioni-contenuti-ue.md) §B.2a. In sintesi, perché la scorciatoia non
esiste: Fab non lascia scegliere la cartella di destinazione dentro `Content/`, quindi si installa il pack in
un progetto vuoto fuori dal repository, **lì** si rinomina, e da lì si migra solo ciò che serve.

⚠️ Due punti su cui si sbaglia, entrambi già pagati: `Migrate` **preserva il path virtuale** (rinominare dopo
non basta, va fatto nel magazzino), e non si portano `SoundCue` né `DialogueWave`.

Il **vault con junction** è storia (§B.2b, 2026-08-05 → 06): fu abbandonato perché il progetto finiva per
dipendere da un percorso esterno. Non va riproposto.

I pack scaricati vivono in `Content/FabAsset/` e `Content/Paragon*/`, **fuori dal repository per scelta**:
sono decine di GB e chi clona se li riscarica.

🔴 **`git clean -fdx` li cancella, e l'esclusione a una sola cartella non basta.** Il comando che gira
nel progetto — `git clean -fdx -e Content/FabAsset` — protegge la prima e **non** la seconda, mentre
`.gitignore` le dichiara entrambe ignorate (`/Content/FabAsset/`, `/Content/Paragon*/`). Servono
entrambe le esclusioni:

```bash
git clean -fdx -e Content/FabAsset -e 'Content/Paragon*'
```

*(Oggi `Content/Paragon*/` non esiste sul disco di chi sviluppa — i pack sono stati consolidati sotto
`FabAsset/` — quindi il difetto non morde. Morde alla prima reimportazione che segue la regola
`.gitignore` invece del comando. La forma monca vive anche in `convenzioni-contenuti-ue.md` §B: là è
un difetto preesistente che questo registro non può correggere da solo, ed è segnalato qui perché è
lo stesso comando.)*

---

## 6. Come si aggiunge un asset

Tre righe, in quest'ordine. Saltarne una produce un difetto silenzioso, e per ognuna è già successo:

1. **`.gitignore`** — la riga `!Content/RT/…` con il path esatto. Senza, `git add` tace e l'asset resta
   locale: è lo stato di `ABP_Gadget` oggi.
2. **`editor-sessions.yaml`** — l'asset va fra gli `artifacts` della seduta che lo produce, **come
   stringa di path e nient'altro**. Senza, nessuna vista sa che quell'asset è atteso: è il caso di
   **U21**.
   🔴 **Non scrivere `tracked`**: non è un campo del sorgente, è un **derivato** che
   `scripts/feature_registry.py` calcola confrontando il path con `git ls-files` e scrive solo in
   `project-graph.json`. Trasformare le voci in mapping `{path:…, tracked:…}` fa confrontare dict
   contro stringhe dentro `tracked_artifacts()` e `session_state()`: l'artefatto non risulta mai
   `done`, la seduta resta bloccata, **e nessun gate fallisce**.
3. **questo file** — la riga nella tabella della sua release.

⚠️ **Un rename tocca tutte e tre.** Le convenzioni lo dichiarano già per il caso `<CharacterId>`: gli otto
path degli artefatti sono elencati **per esteso** nell'allowlist, quindi un rename li rende muti senza che
niente fallisca.

---

## 7. Quello che questa map non sa

- **Non conosce gli asset non committabili.** I pack Paragon importati stanno fuori dal repository: qui si
  vede il `BP_Unit` che li usa, non le mesh e le animazioni sorgente da cui dipende.
- **Non misura le dipendenze.** Che `BP_Unit_Gadget` referenzi una `SkeletalMesh` di un pack è vero e non
  verificabile da qui: lo dice l'editor, e il fallback al cilindro è ciò che tiene in piedi il gioco quando
  il riferimento soft non risolve (`spec-asset-pipeline.md`).
- **Non sostituisce l'allowlist.** Se le due divergono, **vince `.gitignore`**: è il gate che il repository
  esegue davvero. Il comando di §1 se ne accorge **in entrambe le direzioni** — attesi che mancano, e
  tracciati che nessuno ha dichiarato — ma solo dalla sua seconda stesura: la prima iterava la sola
  allowlist, e una lista che interroga sé stessa non può trovare ciò che non prevede.
- **Le righe di §3 e §4 non sono impegni.** Sono fabbisogno derivato dalle epic; diventano impegni quando
  entrano in una seduta e nell'allowlist.
