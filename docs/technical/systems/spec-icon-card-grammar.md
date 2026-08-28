# Skill Card Grammar — core esagonale e satelliti tipizzati

> **Owner** di *come si compone* una Skill Card: quali forme esistono, che cosa significano, quante ne
> stanno e dove si agganciano. Autorità: **[D-231](../../decisions/RT_PDR_00_Decision_Log.md)**.
>
> **Non è** l'owner di: la risoluzione runtime delle icone (**D-031** + `ERTIconCategory`, procedura in
> [`../runbooks/guida-catalogo-icone.md`](../runbooks/guida-catalogo-icone.md)); *dove e quando* un elemento
> compare a schermo ([`progettazione-hud.md`](progettazione-hud.md) §7/§31/§49); l'alfabeto di fase e
> conseguenza cotto nel master
> ([`09-alfabeto-fase-e-conseguenza.md`](../../research/design/icon/visual-language/09-alfabeto-fase-e-conseguenza.md)).
> Nessuna regola vive in due posti.
>
> **Stato misurato**: 2026-08-28. Nessuna Skill Card esiste ancora nel repository. Questo documento è la
> grammatica *prima* della produzione: serve a che la prima card nasca già leggibile, non a validarne una.

---

## 1. Scope e non-scope

| | |
|---|---|
| ✅ **In scope** | forme dei satelliti e loro significato · tetti di quantità · reticolo di ancoraggio · gerarchia di lettura · formula di lettura della skill · esempi falsificabili |
| ⛔ **Fuori scope** | `ERTIconCategory` e le sue 12 categorie · `RequiredIconIds()` · GameplayTag · qualunque calcolo di gameplay. Il **canale colore** è deciso ([D-232](../../decisions/RT_PDR_00_Decision_Log.md)) ma la sua **palette HEX** resta aperta (§8) |

**La regola che tiene separati i due assi**, ed è la ragione per cui questo documento esiste:

> La **grammatica compositiva** dice come un glifo è *disegnato*.
> Il **catalogo semantico** dice come un asset è *risolto*: `UI.Icon.<Category>.<Name>`.

Ne segue la conseguenza pratica — vale come nota per
[#637](https://github.com/DegrassiAaron/refactor-tactics-main/issues/637): `Target`, `Shape`, `Delivery`,
`HitRule` ed `Effect` sono **primitive compositive**. Possono comporre un'icona senza che nasca una
`ERTIconCategory` per ciascuna. Un'abilità resta `UI.Icon.Action.Hero.Gadget.LinearDischarge` anche se il
suo glifo è composto da `Line + Electric`: non deve esistere `UI.Icon.Geometry.Line`.

---

## 2. Due lettori, due vincoli — non confonderli

Questa distinzione previene la revisione più probabile: «questa card è leggibile ma viola un cap».

| Chi | Che cosa gli serve | Che cosa lo vincola |
|---|---|---|
| **Chi autora** | reticolo, anchor, tetti | i **caps** del §5: sono un vincolo di *authoring*, e servono a impedire una card satura |
| **Chi legge in partita** | capire in un colpo d'occhio | la **gerarchia** del §5: è un vincolo di *resa*, e serve all'ordine di lettura |

Il reticolo è una regola di authoring: **non** è una texture da mostrare in HUD.

---

## 3. Le macro-fasi

Il round è `Planning → Prep → Dash → Blast → Move → Cleanup`. La card però porta un marker solo per le fasi
in cui una skill risolve, e sono **quattro**: `Prep · Dash · Blast · Move`. È ciò che `RequiredIconIds()`
deriva, dalla variabile che si chiama `VoluntaryPhases` (`RTIconLibrary.cpp:35-36`).

- ⚠️ `Cleanup` **ospita azioni** — `Action.Evade` è valutata lì (**D-230**). Se una reazione avrà una card,
  il suo marker sarà il quinto, e nascerà insieme al proprio consumatore. Non prima.
- `Planning` non ospita risoluzioni: nessun marker.
- `Dodge` **non è una fase**: è l'azione generica dentro `Dash` (**D-230**). `Dash` è il momento.
- `Reaction` **non è una fase**. `Overwatch` **non è un tipo di attacco**: è una reazione *preparata* che
  osserva un trigger e poi **genera** un attacco, con `Shape`, `Delivery` ed `Effect` propri (§6.3).

### 3.1 La palette delle quattro fasi

Decisa da [D-233](../../decisions/RT_PDR_00_Decision_Log.md). Sono quattro valori Okabe-Ito **già presenti**
nel repository: non entra un colore nuovo nel sistema.

| Fase | HEX | Era il token | Contrasto su `BG_Panel` | Luminanza |
|---|---|---|---:|---:|
| **Prep** | `#56B4E9` | `Defense` | 7.56 | 0.405 |
| **Dash** | `#009E73` | `Movement` | 5.10 | 0.257 |
| **Blast** | `#D55E00` | `Attack` | 4.51 | 0.222 |
| **Move** | `#0072B2` | `Utility` | 3.36 | 0.152 |

**Perché proprio questi quattro**, misurato sui 35 sottoinsiemi possibili: il peggior caso è **ΔE ≥ 19.6** in
ogni tipo di daltonismo (normale 26.4 · deuteranopia 25.1 · protanopia 25.7 · tritanopia 19.6). ⚠️ **La
combinazione intuitiva è quella che fallisce**: con `Move` rosa `#CC79A7`, `Blast` e `Move` collassano a
**ΔE = 1.0** in tritanopia — diventano lo stesso colore. Il giallo `#F0E442` è escluso perché dista **18.3**
dall'ambra di `Selected`, e un colore di fase non deve somigliare a uno **stato**.

⚠️ **Tre limiti dichiarati:**

- in **grayscale** `Blast` e `Dash` distano solo `ΔL = 0.035`. Non si corregge spostando i colori: è la
  ragione per cui il **marker resta obbligatorio** e il colore è ridondanza;
- `Move #0072B2` ha contrasto **2.89** su `BG_Raised` — sotto la soglia 3:1. Su superficie rialzata il glifo
  va bordato o schiarito, e va **verificato a schermo**;
- `Prep #56B4E9` è la coppia più stretta col sistema: `ΔE = 26.2` dal `Cyan`.


---

## 4. La formula di lettura

```text
PHASE → TARGET + SHAPE + DELIVERY + HIT RULE → EFFECT + VALUE → RANGE / DURATION + MODIFIERS
```

Non tutti i termini compaiono insieme.

| Asse | Valori |
|---|---|
| **Target** | `Self · Unit · Cell · Direction · Path` — e per `Unit`: `Ally · Enemy · Any` |
| **Shape / AoE** | `Single · Line · Ray · Cone · Circle · Ring · Cross · Arc · Rectangle · Wall · Chain · ConnectedRegion` |
| **Delivery** | `Direct · Projectile · Beam · Lob · Ground` |
| **Hit Rule** | `First · All · StopOnUnit · StopOnBlocker · PierceUnits · Chain` |
| **Modifier** | `Pierce · StopOnFirstTarget · HitsAll · Bounce · FriendlyFirePossible · IgnoreCover · DestroyCover · ArmorPiercing · ArmorShred` |
| **Effect** | `Damage · Shield · Armor · Heal · Push · Pull · Displace` · `Water/Wet · Electric/Shock · Fire · Status` · `CoverModification · TerrainModification · GraphTraversalModification · Utility` |

**Distinzioni obbligatorie** — ognuna è una collisione già pagata da qualcuno:

- `Line` ≠ `Move`: la prima è una geometria, il secondo un movimento.
- `Chain` mostra **nodi e salti**; un fulmine da solo non è una catena.
- `ConnectedRegion` è la propagazione su celle collegate, non un cerchio grande.
- `Cover` ≠ `Guard` ≠ `Brace` ≠ `Shield` — quattro cose, quattro glifi.
- `Electric` possiede il fulmine. `Reaction` **non** usa il fulmine come simbolo primario.
- `Dash` = movimento scelto · `ForcedMovement` = movimento subìto.
- `ArmorPiercing` ≠ `ArmorShred`.

---

## 5. La card: forme, posizioni, tetti

Il core è un **esagono `pointy-top`**. Non flat-top: il gioco è pointy-top ovunque la griglia sia scritta
(`RTCellId.h:7,22`; `RTHexLibrary.cpp:379`, «primo vertice a −30 gradi»), il test
`RefactorTactics.Hex.CellCornersFormPointyTopHexagon` lo verifica, e l'asset di riferimento lo è
(rapporto altezza/larghezza **1.1547** = 2/√3).

> **Centro = informazione fondamentale della skill. Periferia = proprietà, condizioni, costi, modificatori.**
> **La forma del satellite dice di che tipo è l'informazione.**

| Forma | Significato | Posizione | Tetto |
|---|---|---|---:|
| **Esagono grande** | core / effetto principale | centro | 1 |
| **Marker di fase** | timing reale (§3) | sopra | 1 |
| **Cerchio** | costo, cooldown, cariche, limite d'uso | alto-sinistra | 1 |
| **Piccolo esagono** | proprietà intrinseca: Target, elemento, Shape, Delivery | sinistra · destra · alto-destra | 3 |
| **Quadrato** | modificatore esterno su unità / cella / contesto | basso-destra | 2 |
| **Rombo** | modificatore applicato alla skill | basso-centro | 1 |
| **Triangolo** | condizione, trigger, requisito, warning | alto-destra o basso-sinistra | 1 |

Gerarchia di lettura: `CORE > PHASE > proprietà native > condizione > modificatori`.

### Il reticolo

Esagono pointy-top, centro come origine, un raggio ogni **30°**: sei assi completi, dodici raggi. I satelliti
si agganciano a questa geometria, **mai a occhio**.

⚠️ **Il reticolo non è nuovo, e per questo non se ne inventa un secondo.** `FRTOccupancyMask`
(`RTHexOccupancyLibrary.h:76-99`) è già «dodici bit, uno per settore da 30 gradi», ancorati al primo vertice
a **−30°** con una ragione scritta nel codice: *«senza, due implementazioni entrambe corrette producono
maschere diverse dalla stessa geometria»*. I dodici raggi della card sono lo **stesso insieme** di direzioni.

⛔ **Perciò gli slot della card non si numerano**: si nominano per posizione (`sopra`, `alto-sinistra`,
`basso-centro`). Se un indice servisse davvero, si usa l'ancoraggio a **−30°** del codice — non un secondo
sfasato di uno.

---

## 6. Esempi

I primi tre sono didattici e non appartengono al roster; il quarto sì, ed è quello che può diventare un test
percettivo in [#269](https://github.com/DegrassiAaron/refactor-tactics-main/issues/269).

### 6.1 Rail Shot

```text
Blast · Enemy/Direction · Line · Beam · Damage 5 · Pierce · Range 7
```

- **core**: `Damage 5` · **fase**: `Blast`
- **piccoli esagoni** (3/3): `Enemy/Direction` · `Line` · `Beam`
- **rombo**: `Pierce` · **valore**: `Range 7` col proprio simbolo, mai un numero nudo

### 6.2 Chain Lightning

```text
Blast · Enemy Unit · Chain · Beam · Electric Damage 3 · Max Targets 3 · Range 5
```

- **core**: `Electric Damage 3` · **fase**: `Blast`
- **piccoli esagoni** (3/3): `Enemy` · `Chain` · `Beam`
- **valori**: `Targets 3`, `Range 5`, entrambi contestualizzati
- ⚠️ `Chain` deve mostrare i **nodi**: senza, è indistinguibile da `Electric` + `Line`

### 6.3 Overwatch — tre cose, non una

```text
Reaction → Trigger: enemy movement/entry → attacco generato
```

L'attacco generato ha una propria grammatica, per esempio `Cone · Projectile · Damage 3 · Range 5`. La
documentazione e la UI devono distinguere **tre** stati, che non sono sinonimi:

1. **armata** — la reazione è pronta;
2. **trigger / opportunity** — la condizione è scattata;
3. **attacco prodotto** — l'effetto risolto.

### 6.4 `Hero.Wraith.InterceptShot` — l'esempio reale

È la thin slice Predictive della v0.1 (`CLAUDE.md` §3) ed è l'unico dei quattro che attraversa il roster
spedito. Va composto con la grammatica di §5 e usato come caso di prova quando la prima card viene autorata:
un esempio che non esiste nel gioco non può falsificare nulla.

---

## 7. Vincoli percettivi

Ereditati da [`01-principi.md`](../../research/design/icon/visual-language/01-principi.md) e
[`06-accessibilita.md`](../../research/design/icon/visual-language/06-accessibilita.md), e qui vincolanti:

- **silhouette e pattern prima del colore**; test **grayscale** obbligatorio;
- distinguibili **senza colore**: `Ally`/`Enemy` · `Line`/`Move` · `Electric`/`Reaction` ·
  `Cover`/`Guard`/`Brace`/`Shield` · `Dash`/`Sprint`;
- nessun glow indispensabile; niente 3D o prospettiva nei glifi base; **nessun testo** nel glifo runtime;
- flat/vector, sci-fi tattico, minimal geometrico; **max 2–3 colori** in esplorazione;
- leggibilità reale **20–24 px** a 1080p; glifo ability **32–36 px** dentro slot **56–60 px**;
- **un numero non compare mai senza contesto o simbolo**.

---

## 8. Open points — da non chiudere per simmetria

✅ **1. A che cosa serve il colore — CHIUSO il 2026-08-28 da [D-232](../../decisions/RT_PDR_00_Decision_Log.md).**

> **Il colore dice la FASE, non la famiglia semantica.**

E dice la **macro-fase del round** — `Prep · Dash · Blast · Move`, i quattro valori che `RequiredIconIds()`
deriva dalle `VoluntaryPhases` — non la fase di risoluzione. La distinzione non è pedanteria: il repository
ha **due** nozioni di fase, e il binario di
[`09-alfabeto-fase-e-conseguenza.md`](../../research/design/icon/visual-language/09-alfabeto-fase-e-conseguenza.md)
porta l'**altra**, quella di `RTActionDef.h` (`Preparation` · `FastMovement` · `NormalMovement` · `Control` ·
`Attack` · `Environment`). Le due convivono su due canali diversi e non si contraddicono: il binario dice *in
che ordine risolve dentro il turno*, il colore *in quale fase del round si gioca*.

⚠️ **Il colore NON è l'unico canale della fase, e il marker resta obbligatorio.** Il vincolo grayscale del §7
impone che la fase sia leggibile senza tinta: il colore è **ridondanza deliberata**. Chi togliesse il marker
«perché ora c'è il colore» romperebbe l'accessibilità, e nessun gate lo direbbe.

⚠️ **Conseguenza per chi disegna**: la **famiglia** (Movement, Attack, Defense…) non ha più un colore proprio
e deve vivere **interamente nella silhouette**. È la regola «pattern prima del colore» portata alle sue
conseguenze. Costo misurato: 119 icone portano oggi un token di famiglia, e la ricolorazione è lavoro di E25.

✅ **Il gate `T8` verifica questa regola** (`tools/hud-assets/generate_hud_assets.py`). I cinque gate
preesistenti controllavano geometria e derivazione — banda del binario, riquadro di superficie, fase derivata
dal C++ — **mai la tinta**. `T8` deriva la macro-fase dal C++ due volte: `action_axes()` legge la
`ResolutionPhase`, `match_phase_map()` legge `URTCatalogLibrary::MapResolutionPhase`.

⚠️ **Il debito è dichiarato e contato**: **19** icone già conformi, **33** in `COLOR_DEBT`, **5** su una fase
senza colore. Il gate fallisce sia su una violazione nuova (regressione) sia su una voce di `COLOR_DEBT` già
ricolorata (lista stantia). La ricolorazione resta lavoro di **E25**; il gate impedisce che peggiori.

Restano aperti, dall'handoff §12:

2. ✅ ~~palette HEX finale dei marker di fase~~ — **CHIUSA** da [D-233](../../decisions/RT_PDR_00_Decision_Log.md), vedi §3.1;
3. marker geometrico finale di ciascuna fase;
4. posizione esatta dei piccoli esagoni quando ne compaiono 1, 2 o 3;
5. resa densa di `Range` / `Radius` / `Duration` / `Targets`;
6. ingresso di `Object` nel Target set — oggi è una primitiva del research, non un valore congelato;
7. mapping Hit Rule → triangolo/rombo/mini-glifo quando più regole coesistono;
8. dimensione finale della card composita nell'Action Dock;
9. se esisterà uno schema data-driven per generare la card. **Nessun campo va introdotto «perché il modello
   lo prevede»**: senza un consumatore reale, non nasce.

---

## 9. Puntatori

| Cosa | Dove |
|---|---|
| Questa grammatica | **D-231** |
| Catalogo runtime, 12 categorie | **D-031** · `RTIconCatalogData.h` |
| `Dash` fase / `Action.Dodge` | **D-230** |
| Procedura di catalogo | [`../runbooks/guida-catalogo-icone.md`](../runbooks/guida-catalogo-icone.md) |
| Dove e quando in HUD | [`progettazione-hud.md`](progettazione-hud.md) |
| Minimo v0.1 | **E20** [#217](https://github.com/DegrassiAaron/refactor-tactics-main/issues/217) |
| Governance e authoring estesi | **E25** [#265](https://github.com/DegrassiAaron/refactor-tactics-main/issues/265) · **CP 25.1** [#266](https://github.com/DegrassiAaron/refactor-tactics-main/issues/266) |
| Revisione che ha prodotto questa spec | [`../../roadmap/plans/icon-card-grammar-spec-panel-2026-08-28.md`](../../roadmap/plans/icon-card-grammar-spec-panel-2026-08-28.md) |
