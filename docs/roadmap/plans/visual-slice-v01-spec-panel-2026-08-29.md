# Visual Slice v0.1 — spec panel sul pacchetto a quattordici nodi

> `CURRENT` · **Stato**: revisione chiusa. Il kit è **consumato e archiviato**, non applicato ·
> **Data**: revisione 2026-08-29 · §15–§17 il 2026-08-30 · §18 il 2026-08-30, sulla **seconda consegna** dello stesso kit
> **HEAD della revisione**: `20d59973` (`diag/1665-istanze-board`) — ⚠️ un branch che al momento della
> revisione era **71 commit dietro `origin/main`**: le misure su `Source/` e `Content/` reggono (quelle aree
> non divergono), ma vedi §17 per la sola in cui questo ha prodotto un numero sbagliato
> **Oggetto**: la directory untracked `RefactorTactics_Claude_v0.1_VisualSlice/` — **20 file, 2163 righe**,
> più uno zip byte-identico alla radice — letta contro `Source/`, `Content/`, le dieci issue lato server,
> il Decision Log, `docs/OPEN_DECISIONS.md` e i due kit fratelli consumati il giorno prima.
> **Panel**: Wiegers (lead) · Cockburn · Fowler · Nygard · Crispin · Adzic
> **Modo**: critique
> **Archiviato in**: [`../../archive/src/handoff/2026-08-29-visual-slice-v01-master.md`](../../archive/src/handoff/2026-08-29-visual-slice-v01-master.md)
> **Seconda consegna**, 2026-08-30: lo stesso kit rimpaginato in un file solo,
> `RefactorTactics_Claude_v0.1_VisualSlice_ALL_IN_ONE.md` — **743 righe uniche su 768 sono verbatim** del
> sorgente qui sopra. Misurata in §18; **non** archiviata a parte, per la ragione di §18.6

---

## 1. Il verdetto in una riga

Il kit è **metodologicamente corretto e cronologicamente scaduto**: il suo preambolo prescrive *«misura lo
stato corrente, non assumere che sia tutto da fare»* — la regola giusta, ripetuta quindici volte — e poi
costruisce una **sequenza dichiarata vincolante** i cui primi tre nodi lavorano su una issue chiusa da dieci
giorni.

| | Voci |
|---|---:|
| 🔴 Critico | **3** |
| 🟠 Alto | **4** |
| 🟡 Medio | **4** |

**Raccomandazione operativa**: **non eseguire il kit come sequenza.** Delle sedici unità di lavoro che
propone, **una** non ha equivalente nel repository (`NEW-02`), **due** sono già atterrate integralmente,
**una** è per tre quarti costruita, e le restanti dodici sono versioni *più povere* di issue aperte che le
descrivono già meglio. Il valore netto del pacchetto è una issue da aprire e una checklist di gate; il costo
di eseguirlo alla lettera è ripercorrere `#1094` e `#956`.

⚠️ Nessuna suite eseguita, nessuna build. Issue lette lato server con `gh` a `20d59973`; `Source/`,
`Content/` e `docs/` con `git grep` e `git ls-files` sullo stesso `HEAD`. **Una sola scrittura su GitHub**,
autorizzata dall'autore dopo il referto: l'apertura di `#1712` — §15.

---

## 2. 🔴 C1 — Tre dei quattordici nodi lavorano su `#1094`, chiusa il 2026-08-19

La §«Sequenza vincolante» del master dice: *«Esegui i file in quest'ordine»*, e i primi tre sono

1. `01_PRECHECK_GBX_1094.md` — *«Sbloccare ciò che serve prima di creare il primo asset»*;
2. `02_GRAYBOX_VALIDATION_1095.md`;
3. `03_CLOSE_GBX_1094.md` — *«Riapri `#1094` sullo stato corrente»*.

Misurato lato server:

```
1094 [CLOSED] closed=2026-08-19T04:51:28Z reason=COMPLETED
```

**Dieci giorni prima della generazione del kit**, che si data 2026-08-29 nella sua prima riga.

Il master aggiunge una nota che rende il difetto peggiore, non migliore: *«`#1094 → #1095 → #1094` è
volutamente un loop. Non cercare di chiudere tutte le decisioni graybox prima della scena di validazione»*.
La cautela è ragionata — `GBX-1` e `GBX-5` si misurano guardando, non deducendo — ma è **la cautela di un
mondo in cui `#1094` è aperta**, e in quel mondo il kit non vive più.

> **WIEGERS**: un piano che si apre con due nodi il cui esito è già scritto non è impreciso, è **non
> falsificabile**: chi lo esegue produce un handoff che dice «verificato, era già chiuso» e lo registra come
> avanzamento. Il primo criterio di una sequenza vincolante è che ogni nodo possa fallire.

⚠️ **E il fratello lo aveva già trovato.** Il referto
[`graykit-v01-consolidation-spec-panel-2026-08-28.md`](graykit-v01-consolidation-spec-panel-2026-08-28.md)
§3 porta il rilievo `C2` — *«Due dei sei nodi della roadmap sono issue chiuse»* — e nomina esattamente
`#1094` fra le tre. Il kit di oggi è stato generato **il giorno dopo** quel referto e ripete lo stesso
difetto su scala maggiore.

---

## 3. 🔴 C2 — Il nodo 5 riapre `#956`, e la grammatica che chiede è in `Source/` con il suo gate

`05_BOARD_VISUAL_GRAMMAR_956.md` dice: *«Implementare il delta reale ancora aperto nella `#956`»*, e
prescrive *«Mantieni `RingCount` derivato dalla superficie, non come nuovo dato serializzato»*.

Misurato:

```
956 [CLOSED] closed=2026-08-23T13:25:21Z reason=COMPLETED
```

E il meccanismo che il nodo chiede di implementare esiste, con il nome che il nodo stesso ipotizza:

| Cosa il nodo chiede | Dove è già | Prova |
|---|---|---|
| `RingCount` **derivato**, non serializzato | `URTHexLibrary::SurfaceRingCount(ERTHexSurface)` | `Map/RTHexLibrary.cpp:249` · `.h:250` — è una funzione della superficie, esattamente la forma richiesta |
| Il glifo ad anelli concentrici | `ARTHexMapActor::GetCellGlyphMesh(int32 RingCount)` | `Map/RTHexMapActor.cpp:137`, con cache per `RingCount` e `RTGlyphMaxRings` |
| *«Riusa/estendi il test owner `Hex.SurfaceColorsAreDistinguishable` o nome corrente»* | **quel nome esatto** | `Tests/RTHexTests.cpp:646`, e alle righe 724/731 confronta `SurfaceRingCount` fra coppie di superfici — cioè il gate copre già il canale non cromatico |

∴ Il «delta reale ancora aperto» è **vuoto**. Il nodo 5 non è una semplificazione: è un invito a estendere
un gate che già verifica ciò che il nodo vuole garantire.

> **CRISPIN**: quando un piano nomina il test che gli serve e quel test esiste con quel nome e quell'asserto,
> il lavoro non è «estenderlo» — è cancellare il nodo. Riaprirlo produce o un no-op o una seconda garanzia
> sullo stesso invariante.

---

## 4. 🔴 C3 — Il nodo 1 manda a verificare `GBX-2` e `GBX-4`: `D-171` e `D-173` le hanno chiuse

`01_PRECHECK_GBX_1094.md` §«Priorità» ordina:

> *1. verifica stato `GBX-4` — percorso Content e allowlist;
> 2. verifica stato `GBX-2` — grammatica Closed vs Locked prima di modellare porte*

e §«Passi» aggiunge le clausole *«Se `GBX-4` è ancora realmente aperta e impedisce l'asset: NON creare
asset»* e *«Se `GBX-2` è ancora aperta: non creare la porta»*.

Entrambe sono **chiuse il 2026-08-18**, e `docs/OPEN_DECISIONS.md:460` lo dichiara in grassetto:

| Voce | Chiusa da | Esito |
|---|---|---|
| `GBX-2` — quale canale **non cromatico** distingue `Closed` da `Locked` | **`D-171`** | *«Una traversa in rilievo modellata sul pannello: il marcatore è geometria»*. `Locked` è una **mesh diversa** |
| `GBX-3` — soglie di lettura dell'integrità | **`D-172`** | frazioni del catalogo: `critico ⟺ Integrity * 3 <= DefaultIntegrity(Type)` |
| `GBX-4` — percorso `Content/` del kit | **`D-173`** | `/Game/RT/World/Graybox/` con `Cover/ · Doors/ · Surfaces/ · Volumes/` |
| `GBX-6` — quale scala governa | **`D-163`** | scala d'arte, `HexSize = 150`; atterrata con `#1155` il 2026-08-25 |

E le due che restano — `GBX-1` (Safe Placement inset) e `GBX-5` (ingombro unità) — sono esattamente quelle
che il kit dice di **non** chiudere per ragionamento. Su questo il kit ha ragione, e `OPEN_DECISIONS.md:552`
dice la stessa cosa con la stessa motivazione: *«ciò che le separava dalle due rimaste non è l'importanza,
è l'oracolo»*.

🔴 **Il difetto non è che il kit sbagli la teoria: è che le sue clausole condizionali sono tutte già
risolte, e nel verso che rende i nodi vuoti.** ⚠️ E c'è un dettaglio che rende `C3` più grave di `C1`:
`SM_Graybox_Door_Locked` **esiste già come asset** (`Content/RT/World/Graybox/Doors/`), prodotto secondo
`D-171`. Chi esegue il passo *«non creare la porta»* omette un asset che è già in repository.

> **NYGARD**: il modo di fallire di questo nodo è silenzioso e ottimista. Non rompe niente, e produce un
> referto che dice «bloccato in attesa di decisione d'autore» su decisioni prese undici giorni fa.

---

## 5. 🟠 A1 — `NEW-01` propone un kit graybox di cui sei mesh esistono, prodotte da un commandlet

`ISSUE_NEW01_GRAYBOX_KIT.md` elenca lo scope: *Wall Full · Wall Broken · Low Cover · High Cover · Pillar ·
Door Frame · Door states · placeholder acqua/ghiaccio · Material Master · Material Instance*.

Misurato con `git ls-files Content/RT/World/Graybox/`:

| Voce dello scope | Stato |
|---|---|
| Low Cover · High Cover | ✅ `SM_Graybox_Cover_Low` · `SM_Graybox_Cover_High` |
| Door states | ✅ `SM_Graybox_Door_Panel` · `SM_Graybox_Door_Locked` (`D-171`) |
| Acqua / ghiaccio | ✅ `SM_Graybox_Surface_Water` · `SM_Graybox_Surface_Ice` |
| *(non nello scope, ma è il nodo 2)* Cell Placement Volume | ✅ `BP_Graybox_CellPlacementVolume` |
| Wall Full · Wall Broken · Pillar/Block · Door Frame | ❌ **assenti** |
| `M_GBX_Master` + Material Instance | ❌ **assenti** |

E non sono asset autorati a mano: `Source/RefactorTacticsEditor/Private/Content/RTBuildGrayboxMeshesCommandlet.cpp`
li **genera**, con una tabella (righe 373–378) che mappa nome → sottocartella → builder, e un commento a
riga 359 che cita `D-173` come autorità sulla struttura. Esiste anche il suo gate,
`Source/RefactorTacticsEditor/Private/Tests/RTGrayboxMeshTests.cpp`.

∴ `NEW-01` non è da scartare — **è da riscrivere come delta di quattro mesh più il materiale**, e il suo
primo passo non è «crea asset via Editor» ma «aggiungi quattro builder alla tabella del commandlet». Il kit
dice *«non scrivere `.uasset` manualmente»* e ha ragione; ignora che il repository ha già risolto il
problema nel modo che quella cautela implica.

> **FOWLER**: c'è un produttore canonico con una tabella di registrazione. Un'issue che descrive lo stesso
> output senza nominarlo invita a costruire il secondo produttore accanto al primo.

---

## 6. 🟠 A2 — Il nodo 10 è `#1535` riscritta più corta, e perde le tre uscite che la issue valuta

`10_KNOWLEDGE_VEIL_1535.md` dice cose corrette: *«`ApplyKnowledgeVeil` riceve una conoscenza già scelta»*,
*«non hardcodare `0`»*, *«`AddUniqueDynamic`»*, *«nessun Tick»*, *«prima inquadratura»*. Sono **le stesse
cose** che `#1535` dice, e che il kit ha evidentemente letto.

Ciò che il kit lascia fuori è ciò per cui la issue vale:

| `#1535` porta | Il nodo 10 porta |
|---|---|
| La misura datata e ancorata: *«2026-08-28 su `9018b5c3`»*, tre buchi numerati con i comandi che li producono | *«Verifica che `ApplyKnowledgeVeil` esista e che non abbia consumer reali»* |
| Una **tabella di tre uscite** — A `MakeCurrentSnapshot` (già pubblica, già usata a `RTPlayerController.cpp:66`), B accessore stretto, C payload nel delegate — con **C scartata e la ragione scritta**: sceglierebbe il team per conto di tutti, cioè ciò che `D-227` vieta alla firma | *«Scegli la via minima: snapshot pubblico già esistente, oppure accessor stretto se il costo è misurato»* — due delle tre, senza la ragione dello scarto |
| Il **precedente da non ripetere**, misurato: `ARTHUD` chiama `ComputePlannedHitMarks(AllUnits, /*PlayerTeamId=*/ 0, …)` a `UI/RTHUD.cpp:391` con lo zero cablato, mentre `ARTPlayerController::PlayerTeamId` esiste a `Player/RTPlayerController.h:29` | — |
| La correzione di una propria prima stesura: *«aveva cercato un `GetSnapshot` che non esiste e concluso dall'assenza del nome l'assenza della cosa»* | — |
| Il pattern da riusare con la sua ragione: `AddUniqueDynamic` a `Frontend/RTFrontendGameMode.cpp:108`, *«perché `BeginPlay` può correre più di una volta sullo stesso GameMode in editor»* | *«Usa `AddUniqueDynamic` se il delegate è dinamico e il pattern esistente lo richiede»* |

Il fatto architetturale su cui il nodo poggia è **vero e verificato**: `git grep ApplyKnowledgeVeil -- Source`
fuori da `Map/RTHexMapActor.{h,cpp}` dà solo `Tests/RTVeilTests.cpp`, più due menzioni **in commento** in
`RTKnowledgeDebugConsole.cpp` e `RTTurnManager.h:577`. Zero chiamanti di produzione, come la issue dichiara.

∴ `SUPERSEDED` in senso stretto no — il lavoro è aperto — ma il nodo **non va usato al posto della issue**:
eseguirlo significa lavorare con una specifica strettamente meno informata di quella che il repository ha.

---

## 7. 🟠 A3 — I sei WBP che il nodo 12 cita come «storicamente» esistono tutti, e due issue ci hanno già lavorato

`12_HUD_UMG_613.md` §«Widget target» dice *«Verifica i nomi owner correnti; storicamente: `WBP_RT_TacticalHUD`,
`WBP_RT_TurnHeader`, `WBP_RT_TeamRoster`, `WBP_RT_SelectedUnitPanel`, `WBP_RT_ActionDock`,
`WBP_RT_ActionSlot`»*.

`git ls-files Content/RT/UI/Match/` restituisce **tutti e sei**, più `WBP_RT_UnitCard`. Non sono nomi
storici: sono i file versionati.

E il nodo 13 (`#220`, icone) presuppone un mondo anteriore a due issue chiuse:

- **`#1545`** *«CP 11.7 · `WBP_RT_ActionSlot` non ha `IconImage`: la chiave dell'icona non ha dove atterrare»* — CLOSED;
- **`#1608`** *«main è rosso: `WBP_RT_ActionDock` cerca `IconCatalog`, rinominata in `ReceivedCatalog`»* — CLOSED.

Il wiring che il nodo 13 chiama «mancante» ha una storia di regressioni, il che significa che è stato
costruito. `Source/RefactorTactics/UI/RTIconLibrary.{h,cpp}`, `RTIconCatalogData.h`,
`Content/RT/UI/DA_IconCatalog.uasset`, `Tests/RTIconCatalogTests.cpp` e `Tests/RTScreenHudWidgetTests.cpp`
esistono tutti; esiste anche `RTBuildIconCatalogCommandlet`.

⚠️ Il nodo 13 nomina il test `ScreenHud.WidgetApiExposesNoTexture` *«o nome corrente»*. Il file
`RTScreenHudWidgetTests.cpp` esiste e va letto prima di aggiungere qualsiasi cosa — la clausola *«o nome
corrente»* è corretta e non è stata risolta da nessuno.

---

## 8. 🟠 A4 — Il nodo 2 chiede di implementare il Cell Placement Volume «se ancora mancante»: non manca

`02_GRAYBOX_VALIDATION_1095.md` passo 4: *«Implementa il Cell Placement Volume solo se ancora mancante»*.

`Content/RT/World/Graybox/Volumes/BP_Graybox_CellPlacementVolume.uasset` è versionato, sotto la
sottocartella `Volumes/` che `D-173` prescrive.

✅ **Ma qui la clausola condizionale ha funzionato**, ed è la differenza fra questo nodo e i nodi 1/3/5:
`#1095` è **OPEN**, e la parte che resta aperta — la seduta U25, le tre distanze di camera, la misura di
`GBX-1` e `GBX-5` in scala di grigi — non è codice, è **una seduta con un umano davanti allo schermo**.
Nessun asset la sostituisce.

∴ Il nodo 2 è l'unico dei primi cinque che sopravvive alla misura, ridotto al suo scopo vero: *allestire la
scena e guardarla*.

> **COCKBURN**: l'attore primario di questo nodo non è Claude — è chi apre l'editor. Il nodo scritto così è
> corretto proprio perché smette di essere un task di implementazione a metà.

---

## 9. 🟡 I quattro rilievi medi

| | Rilievo |
|---|---|
| **M1** | Il launcher prescrive *«incolla il percorso, es. `docs/tasks/visual-slice/01_...`»*. **`docs/tasks/` non esiste** nel repository. Il kit propone implicitamente una cartella nuova per documenti che sono temporanei per costruzione — cioè un secondo `docs/archive/src/` senza la disciplina del banner |
| **M2** | Il preambolo di 66 righe è **ripetuto identico in tutti e 15 i file** (`md5` delle righe 1–66: `361e2b82` per tutti). 990 righe su 2163 — il **46%** del pacchetto — sono la stessa pagina quindici volte, e ogni copia è un posto in cui la regola può divergere dalle altre quattordici |
| **M3** | Il nodo 14 prescrive di produrre `VISUAL_SLICE_V01_REPORT.md` *«nel working tree»* senza dire dove. `AGENTS.md` e la convenzione di `docs/` assegnano i referti a `docs/roadmap/plans/`; un file di referto alla radice è esattamente ciò che questa sessione sta rimuovendo |
| **M4** | Il nodo 6 (`#289`) e il nodo 11 (`#77`) non contengono **nessuna** affermazione che le rispettive issue non facciano meglio. Non sono sbagliati: sono ridondanti, e la ridondanza in un piano vincolante costa un giro di lettura per scoprire che non c'era delta |

---

## 10. Cosa regge, misurato

| Proprietà | Come regge |
|---|---|
| **`NEW-02` — LOS debug overlay** | ✅ **L'unico contributo genuinamente nuovo.** Il censimento dei console command in `Source/` dà quindici `rt.*`, fra cui `rt.Debug.DrawCells`, `DrawCover`, `DrawIntent`, `DrawPaths`, `DrawResolution`, `rt.Debug.Knowledge` — e **nessuno per la LOS**. Nessuna delle 15 issue aperte pertinenti lo copre. E i vincoli che l'issue proposta si dà sono quelli giusti: *«nessun algoritmo LOS duplicato nel renderer»*, *«nessun targeting dentro il debug»*, *«debug spento di default»*, *«preferire refresh/eventi esistenti»* |
| **Il nodo 14 come gate** | ✅ Quindici passi di scenario manuale end-to-end più il gate packaged. Nessun documento del repository tiene oggi la catena intera in un posto solo; `docs/technical/test-manuali-pie.md` tiene le voci singole |
| **La disciplina delle clausole condizionali** | ✅ *«solo se ancora mancante»*, *«verifica il branch: non rifare task già chiusi»*, *«non fidarti dei numeri storici: misura il branch»*, *«se l'issue è parzialmente già implementata, riduci il delta invece di rifare il lavoro»*. È la forma giusta — e come nel kit fratello, **le condizioni erano quasi tutte già risolte**: le clausole hanno funzionato e la prosa attorno no |
| **I divieti architetturali** | ✅ *«niente FoW parallelo: usare TeamKnowledge/Knowledge Veil esistenti»* · *«niente target logic dentro il renderer LOS»* · *«niente mega-widget che assorbe i World Overlay»* · *«le mesh sono presentazione, non decidono passabilità/cover/LOS/costo»* · *«non creare un secondo substrato: usare `FRTCellId`»*. Tutti coerenti con il canone, nessuno inventato |
| **La separazione §4.1 / §4.2** | ✅ Il nodo 12 elenca correttamente cosa è Screen HUD e cosa resta World Overlay, e vieta la migrazione del secondo dentro il primo |
| **La `Binary asset rule`** | ✅ Ripetuta in quattro nodi: *«prepara nomi, directory, parametri, Material graph spec e checklist; non scrivere `.uasset` manualmente»*. È la regola di `CLAUDE.md` §7, scritta in modo operativo |
| **Non chiudere `GBX-1`/`GBX-5` per ragionamento** | ✅ Coincide con `OPEN_DECISIONS.md:552`, e per la stessa ragione: l'oracolo è visivo |

---

## 11. Il triage completo — sedici unità di lavoro

| Nodo | Issue | Classe | Motivo |
|---|---|---|---|
| 01 Precheck GBX | `#1094` | 🔴 `SUPERSEDED` | Issue CLOSED 2026-08-19; `GBX-2/3/4/6` chiuse da `D-171/172/173/163` |
| 02 Scena U25 | `#1095` | ✅ `CURRENT` | Issue OPEN; il volume esiste già, resta la **seduta** |
| 03 Close GBX | `#1094` | 🔴 `SUPERSEDED` | *«Riapri `#1094`»* su issue chiusa |
| 04 `NEW-01` kit + materiali | nuova | 🟠 `DUPLICATE` parziale | 6 mesh + volume esistono via commandlet. Delta: Wall Full/Broken, Pillar, Door Frame, `M_GBX_Master` |
| 05 Grammatica board | `#956` | 🔴 `SUPERSEDED` | CLOSED 2026-08-23; `SurfaceRingCount` + `GetCellGlyphMesh` + il gate che li confronta |
| 06 Leggibilità tattica | `#289` | 🟡 `DUPLICATE` | OPEN, ma il nodo non aggiunge nulla alla issue |
| 07 `NEW-02` LOS debug | nuova | ✅ **`PROPOSED`** | **Assente dal repository e dalle issue aperte** |
| 08 Conoscenza parziale | `#1466` | 🟡 `DUPLICATE` | OPEN; issue più dettagliata del nodo |
| 09 Privacy traccia | `#1497` | ✅ `CURRENT` | OPEN; `FRTMoveRoute.StableUnitId` c'è (`Debug/RTDebugConsole.cpp:212`), il troncamento no |
| 10 Knowledge Veil | `#1535` | 🟠 `DUPLICATE` impoverito | OPEN; il nodo perde la tabella A/B/C e il precedente dello `0` cablato |
| 11 Contratto HUD | `#77` | 🟡 `DUPLICATE` | OPEN; nessun delta rispetto alla issue |
| 12 Screen HUD UMG | `#613` | 🟠 `DUPLICATE` | OPEN; i sei WBP «storici» esistono tutti |
| 13 Catalogo icone | `#220` | 🟠 `DUPLICATE` | OPEN; catalogo, library, commandlet e due test esistono; `#1545`/`#1608` già chiuse |
| 14 Gate finale | — | ✅ `PROPOSED` | Nessun equivalente: è una checklist di gate, non una feature |
| `ISSUE_NEW01` | — | 🟠 riscrivere come delta | vedi nodo 04 |
| `ISSUE_NEW02` | — | ✅ **aprire** | vedi nodo 07 |

**Conteggio**: 3 `SUPERSEDED` · 7 `DUPLICATE` (di cui 4 impoveriti) · 3 `CURRENT` · 2 `PROPOSED` · 1 delta
da riscrivere.

---

## 12. Cosa fare, in ordine

| # | Azione | Dove | Chi decide |
|---|---|---|---|
| 1 | ✅ **FATTA** — `NEW-02` aperta come [`#1712`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1712), `v0.1` `P1` `enhancement`, milestone *v0.1 · Leggibilità*. **Non** è il corpo del kit: è riscritto sulle misure — vedi §15 | GitHub | l'autore, che l'ha autorizzata |
| 2 | 🔴 **RITIRATA — era sbagliata, vedi §16.** Diceva *«riscrivere `NEW-01` come delta di quattro mesh + materiale»*. Delle quattro mesh, **una esiste già** come presentazione derivata, **una è `DEFER`** per decisione della spec owner, e **due non sono nel catalogo dei 19**. Il delta reale è **il solo materiale** | — | misurato, non più una scelta |
| 3 | **Non aprire nulla per i nodi 01/03/05**: `#1094` e `#956` sono chiuse e i loro esiti sono in `D-163/171/172/173` e in `Source/` | — | — |
| 4 | Se il nodo 14 serve come gate, **portarlo in `docs/technical/test-manuali-pie.md`** come voce `PIE-VSLICE-01`, non come referto alla radice | owner PIE | l'autore |
| 5 | Per i nodi 06/08/09/10/11/12/13: **lavorare dalle issue**, non dai nodi. Le issue sono più recenti e in tre casi strettamente più informate | — | — |

⚠️ **Nessuna azione tocca il codice.** Il mandato era *consumare e rimuovere*; l'autore ha poi autorizzato
l'azione 1, eseguita — vedi §15. Le altre quattro restano.

---

## 13. Nota di regime

**Nessun `D-nnn` riservato.** La revisione applica `D-163`, `D-171`, `D-172`, `D-173`, `D-225` e `D-227` a
un documento che li ignora; non prende posizione nuova. L'ultimo assegnato in locale è **`D-240`** — non
copiato qui come valore vivo, per la ragione che `CLAUDE.md` §7 dichiara.

**Le ancore sono simboli e ID**, non numeri di riga, tranne dove il numero è la prova (`RTHexTests.cpp:646`,
`RTBuildGrayboxMeshesCommandlet.cpp:373`, `RTHexLibrary.cpp:249`).

⛔ **Il sorgente non è stato modificato**: archiviato verbatim sotto un preambolo di verdetto in
[`../../archive/src/handoff/2026-08-29-visual-slice-v01-master.md`](../../archive/src/handoff/2026-08-29-visual-slice-v01-master.md),
con il preambolo condiviso riprodotto una volta sola invece di quindici.

✅ **Il secondo esemplare è stato rimosso.** `RefactorTactics_Claude_v0.1_VisualSlice.zip` era
**byte-identico** alla directory — venti file, stesse dimensioni una per una — e stava alla radice. Rimuoverlo
non era nel mandato iniziale; l'autore l'ha autorizzato dopo il referto. Lasciarlo avrebbe prodotto la
condizione che il `PASSO 10` del mandato fratello chiama *«due source of truth»*: il kit disponibile in due
copie, una archiviata con un verdetto e una no.

---

## 14. La famiglia, per chi arriva dopo

Tre pacchetti d'autore consumati in due giorni, tutti sulla stessa area:

| Consumato | Sorgente | Referto | Rilievo ricorrente |
|---|---|---|---|
| 2026-08-28 | `RefactorTactics_GrayKit_v0.1_Roadmap_CONSOLIDATED` | [`graykit-v01-consolidation-spec-panel-2026-08-28.md`](graykit-v01-consolidation-spec-panel-2026-08-28.md) | `C2`: due dei sei nodi sono issue chiuse — fra cui `#1094` e `#956` |
| 2026-08-28 | `CLAUDE_Apply_GrayKit_v0.1_Consolidation` | *(stesso referto)* | `PASSO 0` elenca quindici issue, tre chiuse |
| 2026-08-29 | `RefactorTactics_Claude_v0.1_VisualSlice` | **questo** | `C1`+`C2`: tre nodi su `#1094`, uno su `#956` — **le stesse due issue** |

⚠️ **Il difetto non è casuale: è la stessa coppia di issue, tre volte.** Un pacchetto generato da una
conversazione che non rilegge lo stato lato server riproduce la fotografia del momento in cui quella
conversazione è cominciata. La contromisura non è un referto migliore — è che il primo passo di ogni kit
futuro sia `gh issue view` sulle issue che nomina, prima della sua §1.

---

## 15. L'azione 1, eseguita — e perché il corpo del kit non è stato copiato

`NEW-02` è aperta come [`#1712`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1712)
(`v0.1` · `P1` · `enhancement`, milestone *v0.1 · Leggibilità*, dipende da
[`#80`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/80)).

⚠️ **Il corpo proposto dal kit sarebbe stato una issue debole**, e le tre cose che lo rendono debole si
vedono solo misurando. La stesura pubblicata le corregge:

| Il kit dice | Misurato su `20d59973` | Cosa cambia nella issue |
|---|---|---|
| *«blocker/reason code **se il servizio corrente lo espone**»* — clausola condizionale, lasciata irrisolta | **Non lo espone**: `URTHexVisionLibrary::HasLineOfSight` ritorna un `bool` nudo (`RTHexVisionLibrary.cpp:7`). ⚠️ Ma il corpo distingue **due cause su due domini**: `BlocksTraversal` sul **bordo** attraversato, estremi **inclusi** (`:27`) e `bBlocksLineOfSight` sulla **cella**, estremi **esclusi** (`:38`) | La reason è derivabile **senza toccare l'algoritmo**, con una funzione pura accanto che ripercorre `HexLine`. E poiché sarebbe una seconda implementazione della stessa regola, la DoD chiede il test che la ancora a `HasLineOfSight`: se divergono, il debug mente |
| *«un toggle debug **mostra** le celle visibili»* — presume che i comandi `Draw*` disegnino | 🔴 **Cinque su sei non disegnano.** `RTDebugConsole.cpp:163` lo dichiara con la ragione e il debito: *«L'unico comando che disegna davvero è `rt.Debug.DrawCells` […] per `DrawPaths`, `DrawCover` e `DrawResolution` quel lavoro non è fatto»*, e serve un consumatore in `ARTHexMapActor` sul modello di `DrawCellOverlay` | La issue **non presume**: chiede di **scegliere** fra il precedente che disegna (`SetCellOverlayEnabled`, `RTHexMapActor.cpp:806`/`:812`) e quello che stampa, e di scrivere il costo dell'alternativa scartata |
| *«Trova il sistema debug esistente (`Debug/`, console commands, overlay)»* | `RTDebugConsole.cpp:7`: *«`rt.Debug.DrawCells` e `rt.Debug.Pacing` **NON stanno qui**, e non è una dimenticanza: vivono […] dov'è il dominio che ispezionano — il precedente del repository, **cinque file su cinque**»* | Il comando va in `Map/` o `Perception/`, **non** in `Debug/` |

➕ **Tre cose che il kit non aveva e la misura ha prodotto:**

1. **La catena completa dal produttore alla conoscenza** — `HasLineOfSight` → `VisibleCells`
   (`RTPerceptionLibrary.cpp:32`, chiama a `:28`) → `TeamVisibleCells` (`:104`) →
   `FRTTeamKnowledge::VisibleCells` (`RTTeamKnowledge.cpp:14`). Da cui il criterio che rende il vincolo
   *«nessuna divergenza fra debug e Team Knowledge»* **verificabile invece che augurale**: un overlay che
   consuma quell'array non può divergere, uno che rifà la query può.
2. **Il gate non si oppone.** `RefactorTactics.Debug.NamespaceDeclaresAllCommands`
   (`RTDebugConsoleTests.cpp:222`) ha otto nomi letterali, e la riga 259 fissa la regola: *«`rt.Debug.Pacing`
   sta nel namespace e NON è fra gli otto: il DoD elenca ciò che deve esserci, non ciò che può esserci»*. Un
   nono comando non lo rompe — e non va aggiunto alla lista, che appartiene al DoD di `#80`.
3. 🔴 **Un caveat che il kit non nomina e che avrebbe prodotto un overlay bugiardo**: `HasLineOfSight`
   **non guarda il layer** — la linea resta su quello del tiratore (`RTHexVisionLibrary.h:27`,
   `RTPerceptionLibrary.cpp:53`). Su mappa multilivello, colorare celle senza dichiarare il layer è una
   lettura falsa presentata come misura.

✅ **Controprova prima di aprire**: `gh issue list --state all --limit 800` filtrato su
`LOS|line of sight|linea di vista|linea di tiro|HasLineOfSight|VisibleCells` restituisce **una** issue,
[`#1562`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1562) — che decide **quando** la
linea di tiro si rivaluta, è `v0.2`/`post-v0.1`, e non si sovrappone. Il vincolo *«non creare una seconda
LOS»* del kit vale anche per le issue, e questa è la misura che lo verifica.

⚠️ **Ogni ancora numerica di `#1712` è stata verificata con `grep -n` prima della pubblicazione**, e una era
sbagliata: il commento sull'elevazione è a `RTHexVisionLibrary.h:27`, non `:26`. Una issue che denuncia
riferimenti stantii non può portarne.

---

## 16. 🔴 L'azione 2 era sbagliata, e la correzione riduce il lavoro a un quinto

Questo referto raccomandava di riscrivere `NEW-01` come *«delta di quattro mesh più il materiale»* — Wall
Full, Wall Broken, Pillar/Block, Door Frame. **La misura che l'ha prodotta era incompleta**: contava i file
in `Content/RT/World/Graybox/` e la tabella dei builder, e non incrociava la **classificazione §8 della spec
owner**, che è il documento che decide quali dei diciannove elementi si costruiscono e quando.

Incrociata, delle cinque voci ne resta **una**:

| Voce di `NEW-01` | Spec `spec-graybox-placement-contract.md` §8 | Verdetto |
|---|---|---|
| **Wall Full** | Elemento **#5** «Muro» — `EdgeBound`, **PARTIAL**, azione `UPDATE`, *«`FRTGeometrySegment` è l'authority; la presentazione esiste come volume derivato»* | 🔴 **Non manca.** `ARTHexMapActor.cpp:1272` itera `MapAsset->InteriorWalls` e disegna il pannello con `URTGeometryGrammarLibrary::ToPolyline`, filtro layer incluso. Modellare una mesh `SM_Graybox_Wall` accanto creerebbe **due** presentazioni per lo stesso dato |
| **Wall Broken** | Elemento **#10** «Muro sfondato» — **`DEFER`**, dipende da `RT-FEAT-MAP-STRUCTURAL` (`IDEA`, release `future`; nei sorgenti archiviati è **target v0.2+**) | 🔴 **Differito per decisione**, non per dimenticanza. È uno dei sette `DEFER` che §8 conta e motiva |
| **Pillar / Block** | — | 🔴 **Non è nel catalogo dei diciannove.** Il kit lo introduce senza che nessun owner lo preveda |
| **Door Frame** | Elemento **#8** «Porta» — `AS_BUILT`, quattro stati, gate `DoorPanelsShowWhetherYouCanPass` | 🔴 Il *frame* separato non esiste nella classificazione: la porta è già costruita, e `D-171` ha deciso che `Locked` è una **mesh diversa** invece che un pannello ricolorato |
| **Material master + istanze** | — | ✅ **L'unica reale.** Le sei mesh escono dal commandlet con `Mesh->GetStaticMaterials().Add(FStaticMaterial())` (`RTBuildGrayboxMeshesCommandlet.cpp:426`) — cioè uno slot **vuoto**, e a valle il default grigio dell'engine |

### 💡 Il fatto strutturale che nessuno dei tre kit ha nominato

**Nessuna mesh del kit usa un inset.** `CellPlan(Budget)` restituisce il footprint **esterno** della cella
(`RTBuildGrayboxMeshesCommandlet.cpp:238`, *«il footprint esterno della cella, in pianta»*) e le due
superfici lo riempiono intero — giustamente, sono pavimenti. Tutto il resto è `EdgeBound` e si misura in
frazioni di `Side`: `0.92` di lunghezza pannello, `0.20`/`0.10` di spessore, `0.85`/`0.28` di altezza su `H`.

∴ **`GBX-1` — il safe placement inset, aperta e da misurare a U25 — non ha mai bloccato nulla, perché il kit
non ha ancora un solo volume `CellBound`.** Il Pillar sarebbe stato il primo, ed è esattamente la voce che il
catalogo non prevede. Il che ribalta anche la lettura di `C3`: il nodo 1 del kit non è solo scaduto sulle tre
decisioni chiuse — è scaduto anche sulla **quarta**, perché tratta `GBX-1` come un blocco all'authoring
mentre nessun asset autorabile oggi la interroga.

> **FOWLER**: il conto dei file in una cartella dice cosa esiste, non cosa manca. Cosa manca lo dice il
> documento che classifica il dominio — e qui lo classifica per intero, con l'azione per ciascuno dei
> diciannove.

### L'azione 2, come è stata eseguita

Aperta [`#1714`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1714) sul **solo materiale**,
con la tabella qui sopra nel corpo — perché la ragione per cui le altre quattro voci non ci sono vale più del
lavoro che resta, ed è l'unica cosa che impedisce al prossimo kit di riproporle.

⚠️ **Questa riga ha portato `#1713` per qualche minuto, ed era il numero di qualcun altro**: una PR di
un'altra sessione — *«feat(1118): la risposta di reazione e la sua ragione smettono di essere lo stesso
`uint8`»* — l'ha preso mentre scrivevo. È la stessa classe di difetto che `CLAUDE.md` §7 registra per i
`D-nnn`, applicata ai numeri di issue: **un numero non si predice, si legge dalla risposta del server**. Qui
non è costato nulla perché il riferimento era in un documento non ancora committato; in un corpo di issue già
pubblicato sarebbe stato un puntatore a lavoro altrui.


---

## 17. La misura che l'albero sbagliato ha falsato — e come è stata presa

Il nodo 14 è stato portato nel registro come voce `PIE-VSLICE-01`
([`docs/technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md)), che è l'azione 4 di §12.
Il registro impone una disciplina esplicita — *«il numero si ricalcola, non si aggiorna a mente»* — con un
comando canonico da eseguire **prima** di toccare il file e **di nuovo** dopo.

🔴 **Eseguirlo non basta: va eseguito sulla base su cui il commit atterra.** Il primo conteggio di questa
sessione diede `174 · 63/22/2/87`, ed era una misura **corretta di un albero sbagliato** — il working tree di
`diag/1665-istanze-board`, 71 commit dietro. Sulla base vera:

| | voci | ✅ | 🟡 | ❌ | ⏳ |
|---|--:|--:|--:|--:|--:|
| Riga di stato dichiarata (2026-08-28) | 173 | 63 | 21 | 2 | 87 |
| Misurato sul **working tree** (71 commit dietro) | 174 | 63 | 22 | 2 | 87 |
| Misurato su **`origin/main`** — la base vera | **176** | **65** | **24** | 2 | **85** |
| Dopo `PIE-VSLICE-01` | **177** | 65 | 24 | 2 | **86** |

Lo scarto fra la riga dichiarata e la base vera è di **tre voci, due verdi e tre parziali** — il più grande
delle tre passate che il registro documenta. Un numero misurato su un albero stantio ha esattamente l'aspetto
di un numero giusto.

### 🔴 E lo stesso albero stava per cancellare lavoro altrui

`git diff origin/main -- docs/archive/src/README.md` mostrava **−80 righe** su un file dove questa sessione
ne aggiungeva **una**. Non era un conflitto: era `origin/main` **avanti**, con due handoff del 2026-08-29
archiviati da altre sessioni — `cp2-8-e2-hex-playtest-reconciliation` e `cell-sector12-edge6-issues` — che il
branch non ha. Committare il working tree così li avrebbe **rimossi in silenzio**, senza conflitto, perché le
regioni non si toccano.

∴ I due file modificati sono stati **ricostruiti a partire da `origin/main`** (`git show origin/main:<path>`)
e le modifiche riapplicate lì: la riga dell'indice va **dopo** le due voci del 29, e il conteggio PIE è quello
di main. ⛔ Il working tree condiviso **non è stato toccato** per costruire il commit — indice temporaneo e
`commit-tree`, `HEAD` fermo dov'era, perché su questa directory lavora più di una sessione (`D-222`).

### La voce, e perché nasce ⏳ e non 🟡

`PIE-VSLICE-01` verifica che la catena visiva **coesista** in packaged Development, non che i suoi anelli
funzionino: quelli hanno già le loro voci. Nasce non eseguibile perché cinque delle sue dipendenze sono
aperte — `#1712` (appena aperta), `#1535`, `#1497`, `#613`, `#220` — e **non si esegue a pezzi**: metà dei
suoi quindici passi, eseguiti su metà catena, misurano ciò che è già coperto altrove.

### ➕ Un ritrovamento collaterale, nella stessa area

Il blockquote delle sei `PIE-GBX-*` dichiarava *«nessuna eseguibile oggi, per una causa sola e misurabile:
`git ls-files 'Content/RT/World/Graybox/*'` dà 0»*. Oggi dà **7** — le sei mesh più il volume, atterrate con
`D-229`. La causa dichiarata era caduta **senza che una riga cambiasse**, e sei voci restavano marcate
impossibili per una ragione che non esiste più. Corretto, con la precisazione che **toglie uno dei due
impedimenti e non entrambi**: resta la seduta **U21**, l'illuminazione di `L_DevSandbox`, che
`editor-sessions.yaml` mette in `unblocked_by` di U25 e che nessun asset sostituisce.

---

## 18. La quarta fotografia — `ALL_IN_ONE`, scritta cinque minuti prima che questo referto entrasse in `main`

> **Aggiunto**: 2026-08-30 · **Base**: `origin/main` @ `8693b635` · **Sorgente**:
> `RefactorTactics_Claude_v0.1_VisualSlice_ALL_IN_ONE.md`, arrivato in
> `D:/Repositories/refactor-tactics-technical-designer/` — **accanto** al checkout, non dentro.

Il mandato era lo stesso del 29 — *consumare e rimuovere*. Il consumo si è ridotto a un confronto, perché il
file **è lo stesso kit rimpaginato in un documento solo**.

### 18.1 La misura

| Voce | Valore |
|---|---|
| File | `RefactorTactics_Claude_v0.1_VisualSlice_ALL_IN_ONE.md` · **1284 righe** · 35 549 byte |
| Scritto | `2026-08-30 00:16:40 +0200` = **`2026-08-29T22:16:40Z`** |
| Righe uniche normalizzate | **768** |
| Già presenti nel master archiviato il 29 | **743** → **96,7 % verbatim** |
| Righe che il master ha e questo no | 71 — **tutte** banner d'archivio del 29 e separatori `<!-- NN_*.md -->` |
| Contenuto tecnico aggiunto | **zero** |

Riproducibile, e non è un giudizio a occhio:

```bash
norm(){ tr -d '\r' < "$1" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//' | grep -v '^$' \
        | sed 's/^[-*0-9.]*[[:space:]]*//' | sort -u; }
norm RefactorTactics_Claude_v0.1_VisualSlice_ALL_IN_ONE.md          > new.txt
norm docs/archive/src/handoff/2026-08-29-visual-slice-v01-master.md > old.txt
wc -l < new.txt                    # 768
comm -12 new.txt old.txt | wc -l   # 743  — condivise
comm -23 new.txt old.txt | wc -l   # 25   — sole del nuovo, riprodotte sotto
```

### 18.2 Le venticinque righe nuove, verbatim e per intero

Sono tutto ciò che questa consegna aggiunge, ed è la ragione per cui il sorgente **non** è stato archiviato
una seconda volta (§18.6). In ordine di documento: un cappello d'uso e un prompt di lancio.

```text
# RefactorTactics — Visual Slice v0.1 — Claude Code ALL-IN-ONE
Questo è il **solo file da dare a Claude Code**.
## Uso
Dalla root di `refactor-tactics-main`, chiedi a Claude:
Leggi AGENTS.md e CLAUDE.md.
Poi leggi interamente RefactorTactics_Claude_v0.1_VisualSlice_ALL_IN_ONE.md.
Esegui SOLO il primo task non completato.
Prima misura sempre branch, HEAD, git status, issue, codice e test reali.
Alla fine fai l'handoff e fermati. Non iniziare il task successivo senza mia conferma.
# Prompt operativo finale
Leggi AGENTS.md, CLAUDE.md e poi questo file ALL-IN-ONE.
Regole:
non lavorare dalla memoria;
verifica lo stato corrente prima di ogni task;
issue GitHub e owner canonici del repository prevalgono se questo file è invecchiato;
non espandere lo scope;
niente GAS nella v0.1;
non creare una seconda LOS;
non creare una seconda Fog of War;
simulazione e presentazione restano separate;
non manipolare .uasset/.umap a mano;
completa un solo task per volta;
usa il template Handoff;
fermati e aspetta conferma.
Inizia dal primo task della sequenza che non risulta già completato sul branch corrente.
```

### 18.3 🔴 Il contributo netto è **zero**, e stavolta si misura in minuti

La terza fotografia aveva due contributi propri — `NEW-02` e `NEW-01`, §11. Erano **già issue** quando questo
file è stato scritto:

| Evento | Istante (UTC) | Distanza dal file |
|---|---|---|
| [`#1712`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1712) — LOS debug overlay | `2026-08-29T19:50:05Z` | **−2 h 26 m** |
| [`#1714`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1714) — materiale del kit graybox | `2026-08-29T21:03:13Z` | **−1 h 13 m** |
| **Il file `ALL_IN_ONE` viene scritto** | `2026-08-29T22:16:40Z` | — |
| [PR #1717](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1717) — questo referto entra in `main` | `2026-08-29T22:21:56Z` | **+5 m 16 s** |

∴ Il triage di §11 vale riga per riga, riverificato lato server il 2026-08-30, con **due sole righe che
cambiano classe** — e cambiano perché il lavoro è stato fatto, non perché il kit sia migliorato:

| Riga di §11 | Il 29 | Il 30 |
|---|---|---|
| `ISSUE_NEW02` | ✅ **aprire** | ✅ **aperta** — `#1712`, `OPEN` |
| `ISSUE_NEW01` | 🟠 riscrivere come delta | ✅ **aperta ridotta** — `#1714`, `OPEN` |

Le tre `SUPERSEDED` reggono, riverificate il 2026-08-30: `#1094` `CLOSED` dal 2026-08-19, `#956` `CLOSED` dal
2026-08-23. Le otto issue dei nodi restanti sono tutte ancora `OPEN` — nessun verdetto si muove.

### 18.4 🔴 Il difetto non si limita a ripetersi: qui è **argomentato**

`C1` diceva che tre nodi su quattordici lavorano su una issue chiusa. In questa stesura la sequenza non è solo
ripetuta, è **difesa in una sezione propria**:

> *«## Sequenza vincolante»* … *«## Regola importante sulla sequenza — `#1094 → #1095 → #1094` è volutamente
> un loop. Non cercare di chiudere tutte le decisioni graybox prima della scena di validazione»*

Il ragionamento sul loop è **giusto in astratto** — alcune decisioni si misurano guardando, ed è esattamente
ciò che [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) dice di `GBX-1` e `GBX-5`. È il supporto a essere
sbagliato: il loop è appeso a una issue chiusa da undici giorni, e le decisioni che il nodo 01 manda a
*«verificare»* — `GBX-2` e `GBX-4` — sono chiuse da `D-171` e `D-173`. Restano `GBX-1` e `GBX-5`, che hanno
per innesco la seduta U25, cioè [`#1095`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1095),
`OPEN`, che è il nodo 02.

⛔ **E la nuova riga d'uso rende il difetto l'ingresso.** *«Esegui SOLO il primo task non completato»* più
*«Inizia dal primo task della sequenza che non risulta già completato»*: il primo task della sequenza è il
nodo 01, su `#1094`. Chi apre il file dalla prima riga e obbedisce alla lettera comincia da lì.

### 18.5 ✅ Una riga nuova che vale, e perché non basta

Fra le venticinque c'è la clausola migliore dell'intera famiglia:

> *«issue GitHub e owner canonici del repository prevalgono se questo file è invecchiato»*

È il kit che dichiara la propria subordinazione, ed è ciò che §14 chiedeva — ma **nel posto sbagliato**. §14
diceva: *«la contromisura non è un referto migliore — è che il primo passo di ogni kit futuro sia
`gh issue view` sulle issue che nomina, prima della sua §1»*. Quella è una regola di **generazione**; questa è
una regola di **esecuzione**. La differenza si vede nel risultato: la clausola ammette che il documento possa
essere vecchio, e il documento è vecchio lo stesso. Scaricare la rimisurazione sul lettore costa, ogni volta,
le stesse ore di misura che questo referto ha già speso il 29.

⚠️ **Nota operativa minore, ma reale**: *«Dalla root di `refactor-tactics-main`»*, mentre il file è stato
consegnato in `refactor-tactics-technical-designer/`, dove quella root è una **sottodirectory**. Sono due
copie distinte dello stesso repository, e chi obbedisce alla riga apre la root sbagliata.

### 18.6 ⛔ Perché il sorgente non è archiviato una seconda volta

§13 ha già nominato questo esatto fallimento, chiudendo lo zip byte-identico: *«il kit disponibile in due
copie, una archiviata con un verdetto e una no»* è la condizione **due source of truth**. Archiviare
`ALL_IN_ONE` verbatim la produrrebbe, con l'aggravante che le due copie **non** sarebbero byte-identiche —
divergono per venticinque righe, e sarebbe quella la domanda del prossimo lettore.

Nulla va perso: **743 righe su 768 sono già in git** dal 29, in
[`../../archive/src/handoff/2026-08-29-visual-slice-v01-master.md`](../../archive/src/handoff/2026-08-29-visual-slice-v01-master.md),
e le **25** che non c'erano stanno qui sopra in §18.2, verbatim e per intero. Il banner del master porta il
rimando a questa sezione.

### 18.7 Cosa è stato fatto, e cosa no

- ⛔ **Nessuno dei quattordici nodi eseguito**, nessuna issue aperta, chiusa o commentata, nessun sorgente e
  nessun asset toccato — `/sc:spec-panel` è task documentale (`CLAUDE.md` §6);
- ⛔ **Nessun `D-nnn` assegnato**: non c'è posizione nuova da prendere. L'ultimo su `main` è `D-245`;
- ⛔ **Suite non eseguita**: nessuna riga di codice toccata, quindi niente da misurare (`D-222`);
- ✅ **Il file è stato rimosso** dalla directory in cui era arrivato, come da mandato;
- ✅ Il lavoro è stato fatto in un **worktree** su `origin/main`: le due copie del repository restano dove
  sono, e nessuna delle due ha cambiato branch.
