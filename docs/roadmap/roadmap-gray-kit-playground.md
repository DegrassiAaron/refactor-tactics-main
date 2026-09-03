# Roadmap Gray Kit Playground

> `CURRENT` · **Stato**: vivo · **Aperta**: 2026-08-31 · **Misurata su**: `origin/main` = `23c0af3a`
> **Epic**: [#1990](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1990) · **Epic padre**: [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105) — Tactical Designer
> **Decisione**: [`D-304`](../decisions/RT_PDR_00_Decision_Log.md) · **Release**: `out_of_release_scope`
> **Confine**: [#1861](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1861) Map Editor possiede l'**authoring del dato**; questo documento possiede il **laboratorio che lo guarda**.

---

## 0. Il gap che questa capability chiude, misurato e non supposto

Il referto [`graykit-asset-roadmap-v10-spec-panel-2026-08-30.md`](plans/graykit-asset-roadmap-v10-spec-panel-2026-08-30.md)
§2.4 lo dichiara come difetto **critico C1**, con due misure indipendenti:

```bash
grep -rn "SM_Graybox" Source/ Plugins/ Config/ | grep -v Commandlet | grep -v Tests   # -> 0
# nessun .uasset/.umap versionato cita SM_Graybox, tranne i sei SM_Graybox_*.uasset stessi
```

> **Il GrayKit v0.1 esiste, è generato, è testato e non lo usa nessuno.**

Il progetto ha pagato un commandlet (`RTBuildGrayboxMeshesCommandlet`, 518 righe, `D-229`), sette binari,
cinque test `RefactorTactics.Graybox.*` e quattro decisioni d'autore per produrre asset che **nessuna scena
posa**. `L_DevSandbox.umap` è versionato e non li nomina.

Il Gray Kit Playground è il **consumer che manca**: una scena permanente e versionata dove la grammatica
visuale tattica si guarda, si confronta e si giudica — invece di essere riallestita a mano a ogni seduta.

⚠️ **Non è un secondo Map Editor.** Il confine è una riga sola:

> **Map Editor costruisce il dato. Playground costruisce un laboratorio per vedere e validare il dato.**

---

## 1. Invarianti — nessuna issue di questa roadmap può violarli

Il Playground **non**:

1. crea un secondo pathfinder, un secondo LOS, un secondo targeting, un secondo Cover Resolver;
2. crea un secondo sistema di `Facing` né una seconda tassonomia di settori;
3. definisce regole di gioco in `Source/RefactorTacticsEditor/`;
4. modifica `MapState`, snapshot, `TurnLog` o `StateHash` per mostrare qualcosa;
5. usa Actor-per-cella come autorità;
6. interpreta autonomamente il JSON di uno scenario;
7. duplica dati eroe o abilità.

Consuma sempre query, DTO, facade, replay o dati canonici **esistenti**.

### Le sei direzioni, e i dodici settori, restano quelli che sono

| Concetto | Owner | Cardinalità |
|---|---|---|
| direzione di `Facing` | `ERTHexDirection` (`Source/RefactorTactics/Map/RTCellId.h:11`) | **6** — `E · NE · NW · W · SW · SE`, pointy-top, ordine stabile `0..5` |
| occupazione tattica | `FRTOccupancyMask` (#619, invariante 4 di #1861) | **12** settori da 30° |

⛔ **Non si introducono 8 direzioni e non nasce un terzo concetto di «settore».** Sono due assi diversi e
il repository li tiene separati da prima di questa roadmap.

### La guida da 1 metro è un righello, non una griglia

La guida metrica del Playground è **presentation-only**. Non corrisponde a `FRTCellId`, hex grid, nodo A\*,
topologia, distanza di gameplay, occupancy o pathfinding.

🔴 **Una misura che sembrava rinforzare la regola la indebolisce, ed è meglio saperlo.** La prima
stesura di questa sezione diceva che la cella da `1,5 m` (`HexSize = 150.f`, [`D-163`](../decisions/RT_PDR_00_Decision_Log.md))
rende la guida da 1 m **incommensurabile** con la griglia. Il test scritto per pinnarlo
([#1991](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1991)) ha misurato i due passi
chiamando `URTHexLibrary::AxialToWorld`, e dice il contrario:

| Asse | Passo | Rapporto con una guida da 1 m |
|---|---:|---|
| **X** | `HexSize · √3` = `2,598076 m` | non coincide **mai** — `√3` è irrazionale — ma a `13 m` dista **0,96 cm**: a occhio è allineato |
| **Y** | `HexSize · 1,5` = `2,250000 m` | coincide **esattamente** a `9 · 18 · 27 · 36 m` — quattro volte su un floor lungo 40 |

∴ **L'aritmetica non protegge dalla confusione.** Ciò che la impedisce è `D-304`, più l'obbligo che la
**scena dichiari** la guida presentation-only — che è un acceptance criterion di #1991, non il corollario
di un calcolo. Il guardiano è `RefactorTactics.Playground.MetreGuideIsNotTheHexPitch`, che misura
entrambi gli assi e **dichiara** la coincidenza verticale invece di negarla.

---

## 2. La planimetria — 40 m × 24 m

```text
X = -20 .. +20 m        Y = -12 .. +12 m
```

| Elemento | Estensione |
|---|---|
| corridoio centrale | `Y = -2 .. +2` (larghezza 4 m) |
| service strip nord | `Y = +2 .. +3` |
| service strip sud | `Y = -3 .. -2` |

Le otto stazioni sono **8 × 8 m**, con 2 m di separazione fra le colonne.

| # | Nome | X | Y | Stato in GKP 0.1 |
|---|---|---|---|---|
| **01** | Unit + Facing | `-19 .. -11` | `+3 .. +11` | 🟢 **LIVE** |
| 02 | GrayKit primitives + scale | `-19 .. -11` | `-11 .. -3` | ⬜ `PLANNED` |
| 03 | Movement / Path / Destination / Dash | `-9 .. -1` | `+3 .. +11` | ⬜ `PLANNED` |
| 04 | Target / Range / Line / AoE / Cone | `-9 .. -1` | `-11 .. -3` | ⬜ `PLANNED` |
| 05 | Defense / Shield / Cover | `+1 .. +9` | `+3 .. +11` | ⬜ `PLANNED` |
| 06 | LOS / Visibility / Knowledge | `+1 .. +9` | `-11 .. -3` | ⬜ `PLANNED` |
| 07 | Water / Ice / Fire / Electricity / Hazard | `+11 .. +19` | `+3 .. +11` | ⬜ `PLANNED` |
| 08 | Scenario / Resolution / Playback | `+11 .. +19` | `-11 .. -3` | ⬜ `PLANNED` |

Le stazioni devono essere riconoscibili **senza colore** — numero, nome, forma/signage e boundaries visuali.
È [`D-146`](../decisions/RT_PDR_00_Decision_Log.md) («mai solo il colore»), la stessa regola che governa la
seduta `U25`.

⚠️ In `GKP 0.1` **solo la Station 01 è funzionale**. Le altre sette esistono come pad + signage `PLANNED`,
e questo **non finanzia** i loro sistemi.

---

## 3. Dove vivono gli asset — riconciliato contro il repository

🔴 **I percorsi candidati del mandato d'origine sono stati emendati, e l'oracolo dice perché.**
`git check-ignore -q <path>` su `origin/main` = `23c0af3a` — exit **0** significa *ignorato*, cioè
`git add` **tace e non segnala nulla**:

| Path candidato (mandato) | exit | Esito |
|---|---:|---|
| `Content/RT/Editor/GrayKit/Maps/L_GrayKitPlayground.umap` | **0** | ⛔ non entrerebbe nel repository |
| `Content/RT/Editor/GrayKit/Fixtures/BP_GK_UnitFacingFixture.uasset` | **0** | ⛔ idem |
| `Content/RT/Editor/GrayKit/UI/EUW_RT_GrayKitPlayground.uasset` | **0** | ⛔ idem |

| Path riconciliato | exit | Perché |
|---|---:|---|
| `Content/RT/Maps/Dev/L_GrayKitPlayground/L_GrayKitPlayground.umap` | **1** | ✅ §5 di [`convenzioni-contenuti-ue.md`](../technical/tooling/convenzioni-contenuti-ue.md) — *«Mappe → `/Game/RT/Maps/<Category>/<MapName>/`»* — e stessa categoria `Dev/` di `L_DevSandbox`, `L_HexArena`, `L_Prototype`. Già ammessa dal glob `!Content/RT/Maps/**/*.umap` |
| `Content/RT/World/Graybox/Fixtures/BP_Graybox_UnitFacingFixture.uasset` | **1** | ✅ [`D-173`](../decisions/RT_PDR_00_Decision_Log.md) / `GBX-4` — il kit graybox degli **oggetti posabili** vive lì. Già ammesso dal glob `!Content/RT/World/Graybox/**/*.uasset` |
| `Content/RT/Editor/GrayKit/UI/WBP_RT_GrayKitPlayground.uasset` | **0 → 1** | ✅ [`D-280`](../decisions/RT_PDR_00_Decision_Log.md) — gli **strumenti** di authoring solo-Editor vivono in `/Game/RT/Editor/…`. ⚠️ Richiede la riga d'allowlist, scritta **prima** dell'asset (§6 di [`asset-map.md`](../technical/tooling/asset-map.md)) |

### Perché la mappa non va sotto `/Game/RT/Editor/`

`D-280` parla di **strumenti** di authoring, non di **scene**. Una mappa di laboratorio è una mappa di
sviluppo, e la sua categoria esiste già: `Maps/Dev/` ospita `L_DevSandbox`, `L_HexArena` e `L_Prototype`,
che sono nella stessa condizione — versionate, non di gioco, escluse dal pacchetto.

### Il nome del pannello è `WBP_`, non `EUW_`

§6 di `convenzioni-contenuti-ue.md` elenca i prefissi ammessi: `WBP_` = Widget Blueprint. **`EUW_` non
esiste nella tabella.** Il precedente è già in casa: `WBP_RT_ScenarioComposer` **è** un
`EditorUtilityWidget` — [`D-244`](../decisions/RT_PDR_00_Decision_Log.md), uscita (a) di `TD-COMP-1` — e
porta il prefisso `WBP_`. Un secondo prefisso per la stessa classe base sarebbe una convenzione nuova
introdotta senza deciderla.

⚠️ **La scelta `EditorUtilityWidget` vs Slate/C++ non merita una voce del Decision Log**: `TD-COMP-1` l'ha
già decisa per gli strumenti di authoring di questo progetto, e il Playground Panel ne è il secondo caso.

### L'esclusione dal cook — il «meccanismo canonico esistente» non esiste, e va detto

> ✅ **Superato il 2026-09-03 da [#1804](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1804): il meccanismo ORA esiste.**
> `Config/DefaultGame.ini:114` dichiara `+DirectoriesToNeverCook=(Path="/Game/RT/Editor")`, presidiata da
> `RefactorTactics.Packaging.EditorNamespaceIsNeverCooked` e validata su pacchetto vero. La misura qui
> sotto resta come **istruttoria datata**: era corretta quando fu presa, e non descrive piu' il repository.
> 🔴 **E l'esecuzione ha aggiunto qualcosa che nessuna delle due versioni prevedeva**: cotti due
> pacchetti, con e senza quella riga, i container sono **identici**. Il pannello e il Composer sono
> `EditorUtilityWidget`, quindi esclusi **per classe** — la riga vale per il primo asset **non**
> editor-only che finisse sotto `/Game/RT/Editor/`, non per loro. Dettagli in `PIE-PKG-EDITOR-NAMESPACE`.

🔴 **Misurato**: `git grep -n "DirectoriesToNeverCook\|NeverCook" Config Source docs` su `origin/main` dà
**zero**. `Config/DefaultGame.ini:95` fa l'**opposto**:

```ini
+DirectoriesToAlwaysCook=(Path="/Game/RT")
+MapsToCook=(FilePath="/Game/RT/Maps/Dev/L_HexArena/L_HexArena")
```

Lo stesso file dichiara la regola che ne discende, misurata su un pacchetto vero il 2026-08-29:

> *«una mappa entra nel cook solo se è la `GameDefaultMap`, se è referenziata, o se è elencata qui»*

∴ **per una mappa, l'esclusione dal cook è l'assenza da `MapsToCook` più l'assenza di riferimenti.** Non
serve un meccanismo nuovo, serve **l'oracolo che lo dimostra**, ed è già dichiarato in quel commento:

```powershell
UnrealPak.exe <...>/RefactorTactics-Windows.utoc -List | findstr ".umap"
```

⚠️ **E non basta cercare `L_GrayKitPlayground` nel `.utoc`**: quella stringa è presente comunque, perché è
il path della cartella. *Cercare il nome trova un riferimento; cercare il `.umap` trova la mappa.*

✅ **La prova è stata eseguita il 2026-09-01, su un pacchetto vero.** Il container porta **tre** `.umap`
— `L_HexArena` (elencata in `MapsToCook`), `Engine/Maps/Entry` e `L_Frontend` (`GameDefaultMap`) — e
nessuno è il Playground. `L_GrayKitPlayground` non compare nemmeno come stringa.

🔴 **Lo zero è stato validato per mutazione, non letto e basta.** Aggiungendo la mappa a `MapsToCook`
e ricuocendo, **compare**; togliendola e ricuocendo, **sparisce**. Senza quel giro un conteggio a zero
poteva significare *«l'oracolo non sa guardare»* invece di *«la mappa non c'è»*. Nella stessa misura il
container porta **108** asset di `Content/RT/`: il cook ha lavorato su quella cartella, quindi lo zero non
è il prodotto di un cook mai partito.

⚠️ **Non serviva `DirectoriesToNeverCook`, e non sarebbe bastato il percorso.** Per una **mappa**
l'esclusione è l'assenza da `MapsToCook` più l'assenza di riferimenti; il presidio è il commento scritto
accanto a quella lista, perché è lì che il difetto si introdurrebbe e nessun test se ne accorgerebbe.

⛔ **Resta di altri**: la *configurazione* dell'esclusione per gli **asset** di authoring — il pannello di
[#1993](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1993), che è un `.uasset` sotto
`/Game/RT/Editor/` e non una mappa — è di
[#1804](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1804), e l'accettazione packaged del
Map Editor di [#1872](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1872).

---

## 4. Maturity ladder — `GKP 0.1` → `GKP 1.0`

⚠️ **`GKP 0.1` NON è la release `v0.1` del gioco**, e non è nemmeno `TD 0.1`. Sono tre numerazioni
distinte: la release ladder (`v0.1`…`v1.0`, con gli `E<n>`), la scala del Tactical Designer
(`TD 0.1`…`TD 1.0`, #1105) e questa. È lo stesso avvertimento che #1105 porta già nel proprio corpo.

| Stadio | Il designer può | Consumer canonico richiesto | Stato |
|---|---|---|---|
| **GKP 0.1** | aprire `L_GrayKitPlayground`, vedere una Unit graybox col suo Facing, cambiarlo dal pannello fra le sei direzioni canoniche | nessuno oltre `ERTHexDirection` | 🔄 **in corso** — [#1991](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1991) · [#1992](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1992) · [#1993](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1993) |
| **GKP 0.2** | vedere Station 02: le primitive GrayKit **esistenti** e la loro scala/posa | le sette `Content/RT/World/Graybox/*` già generate | ⬜ `PLANNED` — ⛔ si riusano, non si ricreano |
| **GKP 0.3** | navigare fra le station dal pannello: catalogo, preset, reset, camera management | nessuno | ⬜ `PLANNED` |
| **GKP 0.4** | vedere Station 03: movimento, path, destinazione, dash | la sonda di #711 e le query di movimento canoniche | ⬜ `PLANNED` — ⛔ nessun A\* d'editor |
| **GKP 0.5** | vedere Station 04: target, range, line, AoE, cone | `URTHexCombatLibrary::HexHitCells` e l'evento `AttackFootprint` di [`D-301`](../decisions/RT_PDR_00_Decision_Log.md) / [#1945](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1945) | ⬜ `PLANNED` — ⛔ nessun `HexHitCells` duplicato nella presentazione |
| **GKP 0.6** | vedere Station 05: defense, shield, cover | gli owner `E23` / `D-285` (#1826–#1833) e il cluster copertura | ⬜ `PLANNED` |
| **GKP 0.7** | vedere Station 06: LOS, visibility, knowledge | `URTHexVisionLibrary::DescribeLineOfSight` e #1712 | ⬜ `PLANNED` — ⛔ nessuna seconda LOS |
| **GKP 0.8** | vedere Station 07: water, ice, fire, electricity, hazards | i runtime hazard, **quando** esistono come consumer reali | ⬜ `PLANNED` |
| **GKP 0.9** | vedere Station 08: scenario bootstrap e resolution playback | #1753, #1625, #1881 | ⬜ `PLANNED` — ⛔ nessun secondo simulator |
| **GKP 1.0** | vedere la **stessa** grammatica visuale in Playground, Planning, Scenario e Playback | tutti i precedenti | ⬜ `PLANNED` — visual acceptance matrix · cook/package gate · owner documentati |

### 🔴 Perché queste righe NON sono issue oggi

Il precedente è scritto in #1105 e va applicato qui:

> *«il vincolo reale non è "prima chiudi lo stadio precedente", è **"prima esiste il dato che la UI
> compila"**»*

Una riga diventa una issue **solo se** valgono tutte e quattro:

1. il consumer canonico esiste già;
2. non esiste un owner equivalente;
3. la DoD è falsificabile **oggi**;
4. la issue non nasce con la prima riga *«serve un dato che ancora non esiste»*.

Altrimenti resta `PLANNED` in questo documento e nell'Epic.

Applicando il criterio a `GKP 0.2`…`GKP 1.0`: **nessuna** delle nove supera oggi tutti e quattro i punti —
`0.2` e `0.3` per il punto 3 (non c'è ancora la mappa su cui essere falsificabili), le altre per il punto 1
o per il punto 2.

---

## 5. Percorso critico verso `GKP 0.1`

```text
R0  governance / reconciliation        ← questo documento, D-304, l'Epic e le tre issue
     │
     ├─► #1991  map shell 40×24 + otto station pad
     │        │
     │        └─► #1993  Playground Panel
     │                 ▲
     └─► #1992  Station 01 — fixture Unit + Facing
              │        │
              └────────┘
                       │
                       ▼
              U25 / visual acceptance
                       │
                       ▼
                   GKP 0.1
```

**[#1991](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1991)** e la parte di **contratto geometrico** di [#1992](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1992) possono procedere in parallelo — ma solo finché
non contendono lo stesso `.uasset`. La mappa (`L_GrayKitPlayground.umap`) e il fixture
(`BP_Graybox_UnitFacingFixture.uasset`) sono due binari distinti; **posare** il fixture nella mappa tocca il
`.umap`, quindi quel gesto è lavoro-uno-per-volta.

⚠️ **Un `.uasset` non si fonde**: chi li tocca lo fa sapendo che l'ultimo che committa vince
(`.gitignore:119-122`). #1095 dichiara come dipendenza *«una Binary Asset Lease su `Content/RT/`, che non
esiste ancora»*, e questa roadmap eredita quel vincolo invece di aggirarlo.

### Stato misurato — 2026-09-01

| Pezzo | Stato |
|---|---|
| #1991 — map shell | ✅ **CLOSED**; `L_GrayKitPlayground.umap` è su `main`, Station 01 è `bLive = true` |
| #1992 — contratto geometrico | ✅ in `URTHexLibrary`: `FacingRotation` e `FacingMarkerOrigin`, `BlueprintPure`, con sei test `RefactorTactics.Hex.Facing*` |
| #1992 — `BP_Graybox_UnitFacingFixture` | ⏳ **da creare** |
| #1992 — posa in Station 01 | ⏳ **da fare**, e tocca il `.umap` |
| Seduta di verdetto | ✅ `U40` in [`editor-sessions.yaml`](editor-sessions.yaml) |

🔑 **Il contratto geometrico è atterrato in `URTHexLibrary` e non nel Blueprint, e la ragione è misurata**:
`EdgeRotation` — l'unico derivatore della direzione-mondo dei sei lati — è una `static` **nuda**, quindi
un asset in `Content/` non poteva chiamarla e sarebbe stato costretto a incidersi sei angoli. Le due
`UFUNCTION(BlueprintPure)` **delegano** a `EdgeRotation` e non aggiungono trigonometria: la convenzione dei
sei lati resta scritta in un posto solo.

⚠️ **Cio' che i sei test NON provano: che il Blueprint le chiami.** Un fixture che calcolasse l'origine per
conto proprio li lascerebbe tutti verdi. Quel legame è verifica d'editor, ed è la ragione per cui `U40`
esiste invece di essere assorbita dalla suite.

⛔ **Non si anticipano le Station 02–08 prima che il gate `GKP 0.1` sia visibile.** La priorità è

> **prima qualcosa da vedere**, non prima il framework che potrebbe un giorno mostrarlo.

---

## 6. Owner matrix — chi possiede cosa

| Ambito | Owner | Il Playground |
|---|---|---|
| authoring del dato di mappa, otto strumenti del kit | **#1861** Map Editor 0.1 | ne è **consumer**, non lo duplica |
| semantica runtime di posa, copertura, traversata | **#324** / `D-285` (#1826–#1833) | la **chiede**, non la ridefinisce |
| leggibilità del graybox, `GBX-1` / `GBX-5` | **#1095** seduta `U25` | **ospita** la scena permanente di quella seduta |
| presentazione delle unità in scena | **#286** `E21` | ne consuma la grammatica |
| contratto ingombro/pivot | [`spec-graybox-placement-contract.md`](../technical/systems/spec-graybox-placement-contract.md) | lo **mostra**, non lo riscrive |
| collocazione authoring + cook exclusion | **#1804** / `D-280` | vi **dipende** |
| scenario, playback, LOS d'editor | **#1753** · **#1625** · **#1881** · **#1712** | futuri consumer delle Station 06 e 08 |
| generazione delle mesh del kit | `RTBuildGrayboxMeshesCommandlet` / `D-229` | le **posa**, non le rigenera |

---

## 7. Rapporto con la fixture `GrayKitYard` — due cose diverse con nomi vicini

`GrayKitYard` **esiste già** ed è mergiata in `main`:
`Scenarios/Visual/Map/GrayKitYard.json` + `URTMatchSetupLibrary::MakeGrayKitYardArena`, con la
[guida di seduta](../technical/runbooks/guida-seduta-u25-u35-graykit-e-griglia.md) che la usa per `U25`+`U35`.

| | `GrayKitYard` | `L_GrayKitPlayground` |
|---|---|---|
| **cos'è** | una **fixture di dato** generata da codice | una **scena** versionata |
| **porta** | celle, coperture, porte, superfici | attori, mesh, guide, signage |
| **dove vive** | `Source/RefactorTactics/Turn/RTMatchSetupLibrary.cpp` | `Content/RT/Maps/Dev/` |
| **limite dichiarato** | *«porta il DATO, non le mesh del kit»* | è precisamente ciò che posa le mesh |

🔑 **Sono complementari, e la seconda chiude il buco che la prima dichiara.** La fixture ha tolto
l'allestimento a mano del *dato*; resta manuale l'allestimento delle *mesh*, ed è ciò che una scena
permanente elimina. ⛔ **Il Playground non sostituisce `GrayKitYard`** e non rigenera la sua geometria: la
board di `U25` resta quella, per la ragione che la fixture dichiara — *«due letture su scene diverse non
sono confrontabili»*.

---

## 8. Gate `GKP 0.1`

- [ ] `L_GrayKitPlayground` esiste, è versionata, e chi clona la ottiene — oracolo `git ls-files`
- [ ] la guida da 1 m è dichiarata presentation-only **nella scena**, non solo in questo documento
- [ ] otto station pad nelle coordinate di §2, leggibili **senza colore**
- [ ] solo la Station 01 è marcata `LIVE`
- [ ] una Unit graybox e il suo Facing si vedono **senza PIE**
- [ ] il Facing si cambia dal pannello fra le sei `ERTHexDirection` e il marker segue
- [ ] chiusura e riapertura dell'Editor conservano il setup
- [ ] nessuna regola di gioco definita in `Source/RefactorTacticsEditor/`
- [x] il packaged non contiene `L_GrayKitPlayground.umap` — oracolo `UnrealPak -List`, e si cerca il `.umap` — ✅ **provato il 2026-09-01, e validato per mutazione**
- [ ] nessuna relazione fra la guida metrica e `FRTCellId` è stata introdotta

⚠️ **Non si dichiara verde ciò che non è stato eseguito.** Le voci a schermo appartengono a una seduta di
[`editor-sessions.yaml`](editor-sessions.yaml); quelle sul packaged a un pacchetto vero.

---

## 9. Cosa questa roadmap NON fa

- ⛔ non apre issue per le Station 02–08;
- ⛔ non decide `GBX-1` né `GBX-5` — restano di [#1094](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1094) e si chiudono a `U25`;
- ⛔ non incide numeri canonici nuovi: i default di presentazione del fixture si derivano dal CDO, come fanno già i test `RefactorTactics.Graybox.*`;
- ⛔ non crea un `E<n>` — gli `E<n>` sono la release ladder, e questa capability è cross-release;
- ⛔ non assorbe nessuna issue di #1861, #1105 o #286.
