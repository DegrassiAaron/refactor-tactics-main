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
# stato di tutti gli asset attesi, in una passata
python - <<'PY'
import subprocess, os
allow = [l[1:].strip() for l in open('.gitignore', encoding='utf-8').read().splitlines()
         if l.startswith('!Content/') and not l.rstrip().endswith('/')]
tracked = set(subprocess.run(['git','ls-files','Content'], capture_output=True, text=True).stdout.split())
for a in sorted(allow):
    print(('OK      ' if a in tracked else ('DISCO   ' if os.path.exists(a) else 'ASSENTE ')) + a)
print(f"\n{len(allow)} attesi · {sum(a in tracked for a in allow)} committati")
PY
```

**Misurato il 2026-08-13** su `HEAD` `515c5c88`: **17 attesi · 13 committati · 1 su disco · 3 assenti**.
Nessun asset è tracciato fuori dall'allowlist: il gate regge in entrambi i versi.

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
| **Icone dell'HUD** | E20 · E11 | L'insieme richiesto è **derivato**, non fisso: `URTIconLibrary::RequiredIconIds()` lo compone dalle quattro fasi volontarie e dal catalogo azioni *realmente in codice*, quindi cresce da solo. Il gate è `RTIconCatalogTests`. Finché le icone restano ID senza texture, non c'è un `.uasset` da elencare — ma nemmeno un `T_Icon_*` proposto |
| **Livello illuminato del graybox** | seduta **U21** | U21 dichiara di produrre «il livello illuminato **committato**», ma ha `artifacts: []`: nessuno sa quale file sarà, quindi non può entrare nell'allowlist prima della seduta |

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
`<Pack>` è il **nome del pack Paragon**, non quello dell'eroe di gioco: la regola è §5b delle convenzioni, e
per i quattro nuovi va deciso quando si scelgono i pack.

> ⚠️ **I dati dell'eroe non sono un asset.** `URTHeroData` e le istanze di azioni sono **spedite da C++**
> (`#375`): in `Content/` finisce solo la **presentazione**. Un `DA_Hero_*` non compare in questa map, e non
> deve comparire nell'allowlist — l'esempio in §5 delle convenzioni descrive dove *starebbe*, non un file che
> esiste.

---

## 4. v0.3, v0.4, v1.0 — quello che le fonti dicono davvero

Qui la map si assottiglia, e dirlo è più utile che riempirla.

| Release | Asset dichiarati | Perché così pochi |
|---|---|---|
| **v0.3** *Informazione* | **nessuno** | Le sue quattro epic — percezione completa, Expert Bot v2, predictive avanzato, Conditional Intent — sono simulazione e bot. L'unico contenuto plausibile è la presentazione dell'incertezza nell'HUD, che però appartiene alle icone di §2.1 |
| **v0.4** *Operations* | **1 famiglia**: mappa di classe Operations (**E30**) | Le altre epic della release sono formato di gioco e obiettivi. ⚠️ Nel registry la v0.4 ha **zero feature**, mentre la sua milestone GitHub ne ha 29: qui si vede il vuoto dal lato asset |
| **v1.0** *proposta* | **nessuno dichiarato** | Delle sei feature `future` non tracciate, tre toccherebbero il mondo — acqua dinamica, strutture e crolli, verticalità — e richiederebbero asset ambientali. Nessuna ha una issue, quindi nessuna ha un fabbisogno: metterlo qui sarebbe inventarlo |

**La conseguenza pratica**: il costo in asset del progetto è quasi tutto nella **v0.2**, ed è il roster. Le
quattro unità della v0.1 sono costate quattro `BP_Unit` più quattro `ABP`; il roster a 8 raddoppia quel
lavoro in una release sola, e l'epic E35 lo dichiara come rischio («la matrice di interazioni da testare — il
costo non è lineare»), ma dal lato *contenuti* il conto è lineare e noto: **8 asset**.

---

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
sono decine di GB e chi clona se li riscarica. ⚠️ `git clean -fdx` li cancella — si usa
`git clean -fdx -e Content/FabAsset`.

---

## 6. Come si aggiunge un asset

Tre righe, in quest'ordine. Saltarne una produce un difetto silenzioso, e per ognuna è già successo:

1. **`.gitignore`** — la riga `!Content/RT/…` con il path esatto. Senza, `git add` tace e l'asset resta
   locale: è lo stato di `ABP_Gadget` oggi.
2. **`editor-sessions.yaml`** — l'asset va fra gli `artifacts` della seduta che lo produce, con `tracked`.
   Senza, nessuna vista sa che quell'asset è atteso: è il caso di **U21**.
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
  esegue davvero. Il comando di §1 esiste per accorgersene.
- **Le righe di §3 e §4 non sono impegni.** Sono fabbisogno derivato dalle epic; diventano impegni quando
  entrano in una seduta e nell'allowlist.
