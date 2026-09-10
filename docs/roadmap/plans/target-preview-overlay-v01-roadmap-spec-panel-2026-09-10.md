# Target Preview & Semantic Tactical Overlay v0.1 — panel di specifica sulla roadmap · 2026-09-10

> **Oggetto**: la roadmap *«Target Preview & Semantic Tactical Overlay v0.1»* proposta dall'autore (Wave 0–8 + tre GATE MACHINE).
> **Comando**: `/sc:spec-panel`, modalità **critique**, focus `requirements · architecture · testing`.
> **Panel**: Wiegers (requisiti misurabili) · Adzic (esempi eseguibili) · Cockburn (attore e goal) · Fowler (dove vive il confine) · Nygard (modi di fallimento e costo) · Crispin (testabilità).
> **Base della misura**: `main = 89fbb24e`, working tree pulito. Ogni riga marcata 🔴 o ⚠️ cita il sorgente o la issue da cui viene; nessuna viene dalla roadmap in review.
> **Convenzione**: dove serve un conteggio, questo referto porta il **comando** che lo produce. I nomi sostituiscono i totali (`AGENTS.md` §14).

---

## §0 — Il verdetto in una riga

> **La roadmap ha ragione sulla strategia di macchina e sulla grammatica, e sbaglia il punto di partenza: la Wave 2 chiede di costruire un modello che è già spedito con i suoi test, mentre il lavoro che manca davvero — sostituire `DrawDebugLine` con un canale che sappia disegnare l'alpha — non ha nessuna wave.**

∴ Applicata così com'è, la sequenza spenderebbe la sua fase costosa su una fondazione già posata e arriverebbe alla PIE d'accettazione con i parametri di Wave 6 privi di consumatore.

### Punteggio

| Dimensione | Voto | Perché |
|---|---:|---|
| **Chiarezza** | 8.5/10 | la catena *da dove → dove → come → cosa → quali → perché* è il pezzo migliore del documento: è un requisito, non un'aspirazione |
| **Completezza** | 5.0/10 | manca la wave del canale di rendering (§C2); `InteractionContext` è richiesto e non esiste (§C4) |
| **Testabilità** | 6.0/10 | la tabella PIE è un vero oracolo umano, ma due righe non sono eseguibili come scritte (§C3, §C7) |
| **Coerenza col repository** | 3.5/10 | Wave 2 e Wave 1 ridichiarano tipi esistenti; nessun piano precedente sullo stesso scope è citato (§C1, §C9, §C11) |

---

## §1 — Cosa la roadmap prende giusto, e va detto prima

Un referto che elencasse solo difetti darebbe l'impressione sbagliata: quattro scelte di questo documento sono **verificate dal sorgente**, e due di esse chiudono conflitti reali.

| Affermazione della roadmap | Verifica | Esito |
|---|---|---|
| *«il tratteggio resta riservato alla certezza, perché è già spedito nel codice della preview»* | `RTOverlayArea.h`: *«Il canale non cromatico della certezza esiste già ed è il tratteggio»*; `RTOverlayPalette.h`: *«la certezza a schermo passa oggi dal tratteggio»* | ✅ **corretta** — e il divieto ai Secondary di usarlo previene una collisione vera |
| *«non derivare automaticamente lo stile da `ERTAbilityShape`»* | `Ability/RTActionData.h:63`: `Single · Area · Line · Cone` — è la **forma dell'area colpita**, cioè gameplay | ✅ **corretta**: forma ≠ traiettoria, e dedurre l'una dall'altra legherebbe la presentazione al ruleset |
| *«event/state driven, non full redraw a Tick»* | `OnLockInCommitted` esiste già ed è il segnale che spegne le anteprime di pianificazione | ✅ **corretta**, e il segnale da consumare c'è |
| *«una sola LOS: azione armata → target corrente»* | è la stessa conclusione del panel del 2026-09-09, che aveva misurato il costo dell'alternativa: `O(unità × nemici)`, *«fino a 16 linee simultanee»* | ✅ **corretta** |
| *«non usare alla cieca i vecchi riferimenti a `scripts/rt-suite.ps1`»* | `ls scripts/` → `No such file or directory`. La cartella non esiste più, non solo il file | ✅ **corretta, e più forte di come è scritta** |
| *«non deve nascere un secondo ruleset»* | è l'invariante `RefactorTactics.AreaOverlay.DrawingMutatesNothing`, già pinnata | ✅ allineata a un test esistente |
| *«creare l'owner che oggi manca per il Target Preview System»* | `grep -ril "TargetPreview" .` → **nessun file**, nemmeno nelle directory di build | ✅ **corretta**: qui il foglio è davvero bianco, e la Wave 1 è l'unica che lo è |

🔑 **La strategia di macchina singola è la parte da non toccare.** Concentrare build, authoring, automation e PIE in una finestra invece di alternare `compile → editor → test` per issue è coerente con `CLAUDE.md` §10 — Unreal è uno per macchina — e con il principio già registrato di *engine time concentrato*. Le critiche che seguono riguardano **cosa** entra nella finestra, non la scelta di averne una.

---

## §2 — Il panel

### C1 · 🔴 CRITICO — WIEGERS: la Wave 2 chiede di implementare ciò che è già in `Source/`

Il difetto non è di stima: è che il documento descrive come lavoro futuro artefatti che esistono su `main` con i loro test.

```
$ ls Source/RefactorTactics/Map/RTOverlay* Source/RefactorTactics/Map/RTRegionBoundary*
Source/RefactorTactics/Map/RTOverlayArea.h
Source/RefactorTactics/Map/RTOverlayPalette.h
Source/RefactorTactics/Map/RTOverlayPalette.cpp
Source/RefactorTactics/Map/RTRegionBoundary.h
Source/RefactorTactics/Map/RTRegionBoundary.cpp
```

| La roadmap chiede a #1941/#1942 | Stato reale su `89fbb24e` |
|---|---|
| `Meaning` | ✅ `ERTOverlayMeaning`: `Movement` · `PathTrace` · `AttackOriginAim` · `Attack` · `FriendlyFire` · `Hover` |
| `Source` | ✅ `FRTOverlayArea::Source`, con l'invariante *non cambia il colore* pinnata da `AreaOverlay.SourceDoesNotChangeColor` |
| `Certainty` | ✅ `ERTOverlayCertainty`: `Confirmed` · `Predicted` · `Uncertain`, con `AreaOverlay.CertaintyChangesOnlyOpacity` |
| `Priority` | ✅ `URTOverlayPalette::PriorityFor` — **deliberatamente non un campo dell'area**: è proprietà del significato |
| `Layer` | ✅ `FRTOverlayArea::Layer()` **derivato** dalle celle, con `AreaOverlay.AreaIsSingleLayer` |
| *«una sola sede configurabile per la palette»* | ✅ `URTOverlayPalette`: `ColorFor` · `AlphaFor` · `ColorForArea` · `PriorityFor` · `ScaleFor` · `DrawsThroughUnits` |
| *«non creare `M_GreenMovement`, `M_RedAttack`…»* | ✅ già impossibile: `DrawPlanningPreview` chiama `URTOverlayPalette::ColorFor` (`RTHexMapActor.cpp:1203,1227,1250`) |
| #1942 *«estrazione del vero perimetro»* | ✅ `ExtractBoundaryEdges` (tre overload) · `ConnectedComponents` · `CellLess`, ordine stabile per `Layer,X,Y,Dir` |
| #1942 *«concavità, buchi, regioni disconnesse e multilayer inclusi»* | ✅ e il multilayer **per costruzione**: `URTHexLibrary::Neighbor` conserva il `Layer`, quindi il vicinato non attraversa un piano |

⚠️ **Il rischio non è sprecare tempo: è produrre il secondo modello.** Una wave che riparte da `FRTTargetPreview → Meaning/Certainty/Layer` senza sapere che `FRTOverlayArea` esiste li ridichiara, e da quel momento la palette ha due sedi. È esattamente il difetto che #1941 esiste per chiudere.

📝 **RACCOMANDAZIONE** — riscrivere la Wave 2 come **residuo misurato**, non come fondazione. Il residuo di #1941/#1942 non è il modello: sono (a) il consumatore dell'alpha, (b) la ribbon a schermo, (c) la scelta sul depth test — e `RTRegionBoundary.h` dichiara esplicitamente che le ultime due *«si decide guardando, e la voce PIE di questa issue è l'oracolo»*.

---

### C2 · 🔴 CRITICO — FOWLER: manca la wave che sostituisce il canale di rendering, ed è il lavoro portante

La roadmap salta dal modello (Wave 2) ai materiali (Wave 6). Fra i due c'è un fatto che nessuna wave nomina:

```
$ grep -n "DrawDebugLine" Source/RefactorTactics/Map/RTHexMapActor.cpp
1124: DrawDebugLine(...)   1194: DrawDebugLine(...)   1227: DrawDebugLine(...)
1260: DrawDebugLine(...)   1265: DrawDebugLine(...)
```

E `RTOverlayPalette.h` dichiara la conseguenza per iscritto:

> *«Il renderer di oggi usa `DrawDebugLine`, che **ignora l'alpha**; la certezza a schermo passa oggi dal tratteggio. Il consumatore dell'alpha è l'*area fill* aggregata, che è scope di #1944.»*

∴ `AlphaFor` è **dichiarata e disegnata da nessuno**. E la Wave 6 definisce `M_RT_SemanticAreaOverlay` con i parametri `Opacity`, `Fade`, `PulseAmount` — tutti senza consumatore, perché il codice che li applicherebbe non esiste e nessuna wave lo crea.

🔴 **La conseguenza cade sulla Wave 8, che è la seduta d'accettazione.** La roadmap promette: *«In questa stessa seduta si tarano opacity, emissive, width, boundary height, fade, dash scale, arc height, pulse»* e *«niente rebuild per cambiare un numero visuale»*. Con `DrawDebugLine` quei numeri non sono in un materiale: sono argomenti di funzione in C++. Tararli **è** un rebuild — cioè precisamente ciò che la strategia di macchina singola vuole evitare, e lo scoprirebbe a GATE C.

📝 **RACCOMANDAZIONE** — inserire una wave, prima della Wave 6:

> **Wave 2.5 — Il canale di rendering: da `DrawDebugLine` al fill aggregato**
> Owner: **#1944** (già suo: *«il consumatore dell'alpha … è scope di #1944»*).
> Consegna: un renderer che (a) legge `AlphaFor`, (b) disegna un'area come **regione** e non come contorni annidati, (c) risolve il depth test che `DrawsThroughUnits` oggi approssima con `SDPG_Foreground`.
> ⛔ È il prerequisito reale di Wave 6: un parametro di materiale senza consumatore non è un parametro, è un nome.

---

### C3 · ⚠️ MAGGIORE — ADZIC: `BallisticArc` non ha un produttore, e la sua riga PIE non è eseguibile

```
$ grep -rilE "grenade|\blob\b|ballistic|parabol" Source/ --include=*.h --include=*.cpp
   (nessun risultato)
```

Nel catalogo azioni non esiste un'azione che descriva un lob: `ERTAbilityShape` è `Single · Area · Line · Cone`, e i tag vanno da `Action.BasicAttack` a `Action.CircularAoE` a `Action.CreateSmoke` senza mai una traiettoria alta.

⚠️ **Quindi la riga d'accettazione *«Ballistic — la parabola comunica chiaramente il lob»* è `NOT RUN` per costruzione**, e lo si scoprirebbe **dentro** la seduta PIE — cioè nel momento più caro, trasformando una decisione di scope in un buco d'evidenza.

🔑 E il precedente su come trattare questo caso è già scritto, nello stesso repository, in `RTOverlayArea.h`:

> *«Le voci della spec v0.2 che qui NON compaiono … non sono dimenticate: non hanno un produttore. Dichiararle qui senza produttore sarebbe un placeholder, e un enum con voci che nessuno emette non è un vocabolario: è una promessa.»*

📝 **RACCOMANDAZIONE** — scegliere, in Wave 0 e per iscritto:
- **(a)** nominare l'azione che fa il lob e portarla nello scope v0.1 — allora `BallisticArc` ha un produttore e la riga PIE è eseguibile; oppure
- **(b)** togliere `BallisticArc` da `TrajectoryStyle` v0.1 **e** la sua riga dalla tabella d'accettazione.

Non è ammissibile la terza via — tenere la voce e la riga sperando che qualcuno produca l'azione: è il placeholder che #1941 ha già rifiutato.

---

### C4 · ⚠️ MAGGIORE — WIEGERS: `InteractionContext` non esiste, e #1943 è chiavata su di esso

```
$ grep -ril "InteractionContext" .
   (nessun file)
```

La Wave 2 lo elenca fra ciò che **#1941 deve consegnare**. Ma #1941 è già spedita, e il suo enum esclude deliberatamente ciò che non ha un produttore. E #1943 lo prende come **input**: *«Interaction Context → Primary → Secondary 1 → Secondary 2»*, con priorità diverse per `AbilityTargeting` e per `Movement`.

∴ Il resolver di #1943 è chiavato su un tipo che nessuno emette. `ERTOverlayMeaning::Hover` è la cosa più vicina che esiste — ed è documentata come *«Interaction Context, non un'area semantica di gameplay»*, cioè un **significato**, non il contesto che seleziona la priorità.

📝 **RACCOMANDAZIONE** — nominare il produttore prima di #1943. Il candidato naturale è lo stato di pianificazione che già distingue *azione armata* da *movimento in selezione* (`ARTPlayerController::HandleClickOnCell` lo discrimina di fatto). Serve una riga che dica **chi** costruisce il contesto e **quando** cambia; senza, #1943 non è implementabile e la Wave 2 la programmerebbe comunque.

---

### C5 · ⚠️ MAGGIORE — COCKBURN: il grafo delle dipendenze mette P2 a monte di P1

Misurato issue per issue:

| Issue | Label | Milestone |
|---|---|---|
| #1941 · #1942 · #1943 · #1944 | `enhancement` `v0.1` **`P2`** | `v0.1 — Offline Vertical Slice` |
| #172 · #173 | `v0.1` `checkpoint` **`P1`** | `v0.1 — Offline Vertical Slice` |
| #2742 | `enhancement` **`P2`** | ⚠️ **nessuna** |
| #2741 | `v0.1` `P1` | nessuna — **CLOSED** |

Il grafo della roadmap è `#1941 → #1942 → #1943 → #1944 → #2742 → #172/#173`. Quindi due checkpoint **P1** sono a valle di quattro **P2** e di una issue **senza milestone**.

⚠️ **Il secondo punto è il difetto che il repository ha già misurato.** `triage-perimetro-v01-residuo-2026-09-03.md` §3.1: *«Chi pianifica dalla milestone non vede un quarto del proprio residuo, e la vista di release è esattamente lo strumento con cui si decide se spedire.»* Mettere #2742 nello scope minimo v0.1 lasciandola senza milestone riproduce quel difetto sull'elemento più visibile della release.

⚠️ **E l'inversione non è solo di etichetta: è di dipendenza reale.** La Wave 5 chiede che il renderer sia *«alimentato dal `PreviewPhaseViewModel`»*, ma

```
$ grep -ril "PreviewPhaseViewModel" .
docs/technical/systems/progettazione-hud.md
Content/RT_UI_AssetPack_FromHUD/manifest.json
```

∴ esiste **in un documento e in un manifest, non in codice**. Chi lo costruisce sono #172/#173 — cioè le due **P1** che il grafo mette a valle di tutto. La Wave 5 dipende dunque da un tipo che le sue proprie dipendenze devono ancora produrre.

📝 **RACCOMANDAZIONE** — tre scritture, tutte piccole: (1) assegnare a #2742 la milestone `v0.1 — Offline Vertical Slice`, dato che questa roadmap la mette nello scope di release; (2) **dichiarare** l'inversione di priorità nel corpo della roadmap, o correggerla — un ordinamento che contraddice le label senza dirlo si legge come un errore di trascrizione, e qualcuno lo «aggiusterà» nella direzione sbagliata; (3) dire nella Wave 5 **chi** consegna `PreviewPhaseViewModel`, perché oggi la roadmap lo consuma senza nominarne il produttore — lo stesso difetto di §C4.

---

### C6 · ⚠️ MAGGIORE — NYGARD: la giustificazione dello sblocco di #2742 è sbagliata, e questo dice che il grafo non è verificato

La roadmap, Wave 4:

> *«A questo punto #2742 smette di essere bloccata. Il produttore del rifiuto esiste già grazie a #2741, che è chiusa.»*

Il corpo di **#2741** dice un'altra cosa:

> *«**Metà (B)** della richiesta «visualizzare le linee di tiro e quelle bloccate». La metà (A) — l'overlay — è un'altra issue, ed è **bloccata da #1941**.»*

∴ #2741 non è mai stata il bloccante di #2742: il bloccante è **#1941**. #2741 ha consegnato una cosa **diversa** — il rifiuto di bersaglio come ritorno visibile al click, con `OutOfRange` e `NoLineOfSight` mantenuti distinti.

🔑 **La conclusione della roadmap è giusta e la sua ragione è falsa**, e la seconda parte è quella che conta: un grafo di dipendenze la cui motivazione non sopravvive alla lettura della issue è un grafo che non è stato controllato. Le altre frecce vanno riverificate con lo stesso metodo.

⚠️ **E ha una conseguenza sullo scope**: *«invalid feedback»* compare nella lista *«Per dichiarare Target Preview v0.1 DONE»*. Per il caso del click rifiutato **è già consegnato da #2741**. La lista non separa ciò che è spedito da ciò che va costruito, e una DoD che elenca lavoro già fatto non misura più niente.

---

### C7 · ⚠️ MAGGIORE — FOWLER: la riga PIE «Multilayer» contraddice il modello di LOS

Wave 8 chiede al giudizio umano: *«Multilayer — overlay sul piano corretto»*. Per il canale LOS, «il piano corretto» non è definito:

> `RTHexLosTool.h:76` — *«La LOS **non guarda il layer del bersaglio**: la linea resta su quello del tiratore — «da terra si spara sotto un ponte, da un piano superiore si spara oltre le coperture basse».»*

∴ Verso un bersaglio su un altro piano l'autorità **non ha valutato** una linea su quel piano. Disegnarla comunque farebbe affermare alla preview qualcosa che il ruleset non dice — cioè violerebbe il non-goal che la roadmap stessa mette in apertura (*«non deve nascere un secondo ruleset»*).

🔑 Il precedente corretto esiste e non disegna: `RTHexLosConsole` **dichiara il caveat** invece di tacerlo, perché *«una risposta che tace sul layer si legge come se valesse per tutti, e non è vero»*.

📝 **RACCOMANDAZIONE** — spezzare la riga in due giudizi, che sono davvero due:
- **aree** semantiche multilayer → vero per costruzione (`FRTCellId::Layer`, `AreaOverlay.AreaIsSingleLayer`): la PIE verifica solo che il fill stia sul piano che dichiara;
- **linea di tiro** cross-layer → la preview deve **dichiarare** il piano del tiratore, non insinuare una linea non valutata. Il giudizio da dare è *«si capisce che la linea vale sul piano del tiratore?»*, non *«è sul piano corretto?»*.

---

### C8 · ⚠️ MODERATO — WIEGERS: la Wave 0 chiede una decisione palette senza dichiarare il gate misurabile che esiste già

La Wave 0 dice: *«Non congelare i colori attualmente proposti finché non passano: distinguibilità colore + luminanza/grayscale + distinzione dalle superfici della mappa»*. Il principio è giusto. Ciò che manca è che **il criterio è già eseguibile e tre collisioni sono già misurate**.

Formula e soglia in uso in `RefactorTactics.Hex.SurfaceColorsAreDistinguishable`: **Manhattan RGB, `>= 60`**. Con quella:

| Coppia | Distanza | Dove è registrata |
|---|---:|---|
| `Attack (230,60,50)` vs `URTHexLibrary::BlockedCellColor() (230,40,40)` | **30** | pinnata da `AreaOverlay.PaletteRatchet` |
| `FriendlyFire (255,150,30)` vs superficie `Fire (255,130,40)` | **30** | pinnata da `AreaOverlay.PaletteRatchet` |
| v0.2 assegna il ciano-blu a Vision/LOS, ma `FColor(40,220,220)` **è già** `PathTrace` | — | `RTHexLosConsole.cpp`, e il panel del 2026-09-09 |

⚠️ **E la decisione è già stata rimandata a chi scrive questa roadmap**: `RTOverlayPalette.h` registra che il maintainer, il 2026-09-02, ha dichiarato che la collisione ciano *«resta interamente vostra da decidere»*, e che #1941 ha consolidato **i valori in produzione** proprio per non risolvere in silenzio la collisione nel codice.

∴ La Wave 0 tratta la palette come una scelta di gusto da chiudere in una riga di tabella, mentre è un `D-nnn` con tre voci nominate e un test che le sorveglia.

📝 **RACCOMANDAZIONE** — la Wave 0 consegna un **`D-nnn` nel Decision Log** che risolve le tre coppie citando la formula esistente, e che estende `AreaOverlay.PaletteRatchet` ai valori decisi. Non un'altra tabella di colori proposti: le tabelle proposte esistono già ed è la loro collisione il problema.

---

### C9 · ⚠️ MODERATO — FOWLER: `FRTTargetPreview` ridichiara due vocabolari che esistono

| Campo proposto | Cosa esiste già |
|---|---|
| `TargetReason` | `ERTHexTargetReason`: `Ok` · `OutOfRange` · `NoLineOfSight` · `NoMap`, da `URTCombatLibrary::ClassifyHexTargeting`, con `Combat.HexTargetingReasonDistinguishesRangeFromCover` e `Combat.HexTargetingIsFailClosed` a pinnare la distinzione |
| `Blocker` | `FRTLineOfSightResult`: `Block` · `BlockedAt` · `BlockedFrom` · `StepIndex`. E la regola su **cosa** si indica è già scritta: `URTTurnLogLibrary::SightBlockerForLog` indica la cella quando il blocco è `CellBlocker` e **non** indica nulla quando è `EdgeBlocker`, *«perché `BlockedAt` su un bordo non è il muro»* |
| `AffectedCells` / `TargetCells` | arrivano da `FRTOverlayArea::Cells`, già presentation-only |

🔑 `FRTTargetPreview` deve **portare** questi tipi, non riformularli. Una proiezione che ridichiara il vocabolario delle sue sorgenti è il punto in cui le due divergono — e qui divergerebbero sulla regola `EdgeBlocker`, che è controintuitiva e quindi la prima a essere riscritta male.

📝 **RACCOMANDAZIONE** — nel modello di Wave 1: `ERTHexTargetReason TargetReason` e `FRTLineOfSightResult Los` come campi, con un commento `⛔` che vieti di derivare il blocker altrove. Così la regola `CellBlocker`/`EdgeBlocker` resta in un posto solo.

---

### C10 · ⚠️ MODERATO — CRISPIN: *«test scritti ma non eseguiti»* per sei wave è un rischio reale, e la roadmap non lo dichiara

La scelta è difendibile su una workstation sola, ed è la stessa logica di *engine time concentrato*. Ma il documento non dice il prezzo: la **prima esecuzione** di qualunque test avviene a **GATE B**, cioè dopo la build (GATE A) **e dopo la seduta di authoring** (Wave 7). Un errore di contratto nel modello di Wave 1 si manifesta quando i `.uasset` sono già stati creati e salvati.

⚠️ E l'asimmetria è gratuita: i test di Wave 1 e Wave 2 sono **headless e puri** — è la stessa forma di `RTOverlayModelTests` e `RTRegionBoundaryTests`, che girano senza PIE. A GATE A la build **c'è già**. Eseguirli lì non è il churn `issue → build → issue → build` che la roadmap giustamente rifiuta: è usare una finestra già pagata.

📝 **RACCOMANDAZIONE** — aggiungere a GATE A un passo `7. eseguire i test puri (nessuna PIE, nessun asset)` e spostare a GATE B solo ciò che ha bisogno di asset o di Editor. La suite completa resta una sola volta alla fine, come scritto.

---

### C11 · ℹ️ MINORE — COCKBURN: il documento non cita nessun piano precedente sullo stesso scope

`CLAUDE.md` §4 (`SEARCH → REUSE → EXTEND → CREATE`) vale anche per i documenti. In `docs/roadmap/plans/` esistono già, sullo stesso perimetro:

- **`linee-di-tiro-visibili-spec-panel-2026-09-09.md`** — di **ieri**, è l'origine di #2741 e #2742, e **pone già** le tre domande della Wave 0 (cardinalità, durata, forma della linea bloccata), rispondendo alle prime due;
- `hud-planning-prediction-dual-roadmap-spec-panel-2026-09-05.md`;
- `tactical-grid-overlay-spec-panel-2026-08-30.md` + `tactical-grid-overlay-roadmap-2026-08-30.md`;
- `combat-shot-presentation-spec-panel-2026-08-31.md`;
- `visual-slice-v01-spec-panel-2026-08-29.md`;
- `triage-perimetro-v01-residuo-2026-09-03.md` — il perimetro v0.1 autorevole.

∴ La Wave 0 riderivera decisioni prese ieri. Citarle costa una riga e cambia quali domande restano davvero aperte.

---

### C12 · ℹ️ MINORE — DOUMONT: il documento introduce due nomi che collidono col vocabolario già in uso

Due divergenze piccole, in un repository che tiene molto al fatto che un termine significhi una cosa sola.

| Nome nella roadmap | Cosa dice già il codice |
|---|---|
| `M_RT_SemanticAreaOverlay` · `M_RT_SemanticBoundary` | il namespace spedito è **`RTOverlay*`** / **`RTRegionBoundary`**: `grep -ril "SemanticOverlay\|SemanticArea" Source/` → **nessun file**. Asset che non portano il nome del tipo che li configura rendono più difficile l'invariante di #1941, *«git grep di un valore della palette dà UNA occorrenza»* |
| `TrajectoryStyle` · `TrajectoryPoints` · `M_RT_TargetTrajectory` | **`trajectory` è già occupato**: `RefactorTactics.Reactions.InterceptRequiresCompatibleTrajectory` (`RTInterceptTests.cpp:376`) lo usa per il **percorso di movimento** di un'unità, ai fini della compatibilità di intercetto. Sono due cose diverse — un tragitto di gameplay e una curva di presentazione |

📝 **RACCOMANDAZIONE** — allineare i nomi degli asset al namespace `RTOverlay*`, e scegliere per la curva di presentazione un termine che non sia `Trajectory` (`Arc`, `Flight`, `Shot`), riservando `Trajectory` al significato che il resolver delle reaction gli dà già. Costa nulla adesso e molto dopo la Wave 7, quando i `.uasset` sono salvati.

---

## §3 — Sintesi: la sequenza corretta

Il panel converge su una riorganizzazione che **conserva** la strategia di macchina singola e cambia il contenuto delle prime due wave.

```text
WAVE 0   Grammatica → consegna un D-nnn (palette: le tre coppie nominate)
         + la decisione BallisticArc: produttore o fuori scope
         + cita il panel del 2026-09-09 invece di riderivarlo
   ↓
WAVE 1   Target Preview Model — PORTA ERTHexTargetReason e FRTLineOfSightResult,
         non li ridichiara. PreviewStyle esplicito per azione.
   ↓
WAVE 2   RESIDUO di #1941/#1942 (il modello è spedito):
         InteractionContext + il suo produttore → poi #1943
   ↓
WAVE 2.5 ⭐ NUOVA — canale di rendering: DrawDebugLine → fill aggregato che
         legge AlphaFor. Owner #1944. Prerequisito reale della Wave 6.
   ↓
WAVE 3   Trajectory Renderer (event-driven)
   ↓
WAVE 4   LOS — una linea sola, col caveat del piano del tiratore dichiarato
   ↓
WAVE 5   Ghost Timeline (#172/#173) — e la loro P1 va riconciliata col grafo
   ↓
WAVE 6   Asset spec — ora i parametri hanno un consumatore
   ↓
GATE A   build + ⭐ test puri headless (la build c'è già)
   ↓
WAVE 7   Editor authoring, una volta
   ↓
GATE B   Automation batch
   ↓
WAVE 8   PIE d'accettazione — con «Multilayer» spezzata in due giudizi
         e senza la riga Ballistic, se la Wave 0 ha scelto (b)
   ↓
GATE C   Release gate
```

### Le scritture che sbloccano il resto

| Azione | Costo | Chiude |
|---|---|---|
| `D-nnn` sulla palette: le tre coppie, contro la formula Manhattan `>= 60` già in uso | una decisione | §C8 |
| Decidere `BallisticArc`: produttore nominato **o** fuori dalla v0.1 (con la sua riga PIE) | una riga | §C3 |
| Nominare il produttore di `InteractionContext` | una riga | §C4 — sblocca #1943 |
| Nominare chi consegna `PreviewPhaseViewModel` | una riga | §C5 — sblocca Wave 5 |
| `gh issue edit 2742 --milestone "v0.1 — Offline Vertical Slice"` | un comando | §C5 |
| Riscrivere Wave 2 come residuo e inserire Wave 2.5 | due sezioni | §C1 · §C2 |
| Separare, nello «Scope minimo v0.1», ciò che è **già spedito** da ciò che va costruito | una tabella | §C6 |
| Rileggere ogni freccia del grafo contro il corpo della issue, col metodo di §C6 | una passata | §C6 |
| Allineare i nomi asset a `RTOverlay*` e liberare `Trajectory` — **prima** della Wave 7 | due rinomine su carta | §C12 |

---

## §4 — Verification

| Gate | Esito |
|---|---|
| Compile | `N/A` — nessuna riga di `Source/` toccata da questo referto |
| Tests | `NOT RUN` — nessun test eseguito; i nomi citati (`AreaOverlay.*`, `Combat.HexTargeting*`, `Hex.SurfaceColorsAreDistinguishable`) sono **letti** dal sorgente, non misurati qui |
| Determinism · Replay · Privacy | `N/A` — documento |
| PIE · Packaged | `N/A` |

⛔ **Nessuna evidenza fabbricata.** Ogni verifica di §1 e ogni misura di §2 riporta il file, la riga o il comando. Le due affermazioni sull'assenza — `InteractionContext` e il produttore ballistico — sono **zeri misurati col loro `grep`**, ed è quello il difetto, non un totale.

## §5 — Known limitations

- Non è stato letto il corpo integrale di #1941–#1944 e #172/#173: la review misura la roadmap contro il **sorgente** e contro le issue #2741/#2742, non contro le DoD complete delle sei issue coinvolte;
- il residuo esatto di #1944 non è misurato: `RTOverlayPalette.h` gli attribuisce il consumatore dell'alpha, ma quanta parte del fill aggregato la issue già specifichi non è stato verificato;
- nessun giudizio su leggibilità visiva: appartiene alla PIE, e questo documento non ne ha aperta nessuna.

## §6 — Follow-up candidates

- ⛔ Fuori scope qui, ma emerso: la voce PIE verde `PIE-PREVIEW-AREA` (2026-08-09) per `FriendlyFire` **non può** essere stata passata su una cella `Fire`, dove i due arancioni distano `30`. È una firma valida su uno scenario che non copre il caso peggiore — se ne dovrebbe accorgere il `D-nnn` di §C8;
- `ERTOverlayMeaning::Hover` è documentato come *«Interaction Context, non un'area semantica»*: se #1943 introduce un `InteractionContext` vero, va deciso se `Hover` resta un significato o diventa un contesto.

---

# 🔁 Aggiornamento del 2026-09-10, dopo il merge — e il referto sopra resta scritto

> Il corpo di questo documento misura `main = 89fbb24e`. Al momento del commit `main` è **`6c695731`**, e due delle sue raccomandazioni hanno già un esito. ⛔ **Il testo sopra non è stato riscritto**: un referto porta la data della sua misura, e correggerlo a posteriori toglierebbe la prova di cosa era vero quando è stato scritto.

## §C8 — **eseguita**, e nella forma che chiedeva

La raccomandazione era: *«la Wave 0 consegna un `D-nnn` nel Decision Log che risolve le tre coppie citando la formula esistente, e che estende `AreaOverlay.PaletteRatchet` ai valori decisi»*.

**[`D-368`](../../decisions/RT_PDR_00_Decision_Log.md)** fa entrambe le cose, e con la stessa formula che §C8 aveva estratto dal test — Manhattan RGB, soglia `60`:

| §C8 misurava | `D-368` |
|---|---|
| `Attack` vs `BlockedCellColor()` = **30** | passa a **72** |
| `FriendlyFire` vs superficie `Fire` = **30** | passa a **60** — ⚠️ *esattamente* sulla soglia, e la decisione dichiara perché non si può fare meglio senza confondere il fuoco amico con `Attack` |
| il ciano già occupato da `PathTrace` | `Vision #32ADE6` **riservato**, e non aggiunto: *«entra nell'enum quando #2742 gli darà un produttore»* |

Il gate ha cambiato nome e non ha più esenzioni: `AreaOverlay.PaletteRatchet` → **`AreaOverlay.PaletteIsDistinguishable`**, `grep` in `RTOverlayModelTests.cpp:182`. ∴ ogni occorrenza di `PaletteRatchet` nel corpo sopra si legge come il nome storico.

🔑 **E `D-368` raccoglie anche il §6**: dichiara che **`PIE-PREVIEW-AREA` va rimisurata**, perché era ✅ con un giudizio umano firmato sul fuoco amico e la decisione ne cambia il colore. Il follow-up sospettava che quella firma non coprisse il caso peggiore; la decisione la invalida per una ragione diversa — il colore non è più quello — e l'esito pratico coincide.

## §C5 — **non eseguita**

```bash
gh issue view 2742 --json milestone --jq '.milestone.title // "nessuna"'   # -> nessuna
```

L'inversione P1/P2 e l'assenza di milestone su **#2742** restano come misurate. La raccomandazione — *«assegnare a #2742 la milestone `v0.1 — Offline Vertical Slice`, dato che questa roadmap la mette nello scope di release»* — è una decisione di **perimetro di release**, non una correzione di forma: appartiene a chi possiede lo scope v0.1, e questo commit non la prende.

## §C3 — ha ora una issue, aperta lo stesso giorno e da un altro panel

Il rilievo *«`BallisticArc` non ha un produttore»* è stato raggiunto **indipendentemente** da uno spec panel gemello, che ha aperto **[#2825](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2825)** sotto E49 con lo stesso vincolo come cardine: *«la ballistica nasce nel gameplay o non nasce»*, misurato con

```bash
git grep -niE "ballistic|parabol|trajector" -- 'Source/*' ':!*Tests*'   # -> 0
```

⚠️ **Due differenze da leggere insieme, non a sostituzione**:

- #2825 aggiunge un asse che questo referto non nomina — **`Trajectory` come dichiarazione dell'ability**, distinta da `Shape`, perché oggi `ERTAbilityShape::Line` risponde a due domande diverse con una voce sola;
- questo referto porta il rilievo **§C2**, che #2825 non ha: *manca la wave che sostituisce `DrawDebugLine` con un canale capace di alpha*. È il prerequisito reale della resa, e nessuna delle due issue lo possiede oggi.

∴ chi stima la resa della preview legge **entrambi**, e §C2 è il pezzo che sopravvive solo qui.

## ⛔ `CONTRACT CONFLICT` — `D-364` e `D-368` decidono il contrario l'una dell'altra

> Rilievo **nuovo**, non presente nel corpo: emerso rileggendo il Decision Log dopo l'aggiornamento su `D-368`. Non lo risolve questo referto — `CLAUDE.md` §13, *«se il conflitto resta reale: `BLOCKED — DECISION REQUIRED`»*.

Due voci **accettate lo stesso giorno**, a quattro righe di distanza nello stesso Log (`RT_PDR_00_Decision_Log.md`, righe `378` e `382`), dicono l'opposto su due voci dell'enum:

| | [`D-364`](../../decisions/RT_PDR_00_Decision_Log.md) | [`D-368`](../../decisions/RT_PDR_00_Decision_Log.md) |
|---|---|---|
| **`FriendlyFire`** | **non è un `Meaning`**: sottoinsieme marcato di `PreviewHitCells`, stesso produttore `MakeBlastPreview` — è un **modificatore reso come forma**, e *«va rinominata insieme, non dopo»* | **è un `Meaning`**, e prende il valore `#FA9B0A` |
| **`Hover`** | è **Interaction Context** (#1614), non un'area di gameplay | **è un `Meaning`**, e prende il valore `#FFD60A` |
| **L'effetto sulle collisioni** | toglierli *«fa sparire due collisioni invece di risolverle»* | le risolve **spostando i valori**, e converte il ratchet in gate |

⛔ **Nessuna delle due cita l'altra, e nessuna è marcata superata.** Verificato: `grep` di `D-364` dentro il corpo di `D-368` non dà nulla, e viceversa.

### Quale delle due è in vigore: lo dice il codice, non il Log

`D-368` è **implementata, con l'attribuzione scritta accanto ai valori**:

```
$ sed -n '11,16p' Source/RefactorTactics/Map/RTOverlayPalette.cpp
case ERTOverlayMeaning::Movement:        return FColor(53, 199, 89);   // #35C759 [D-368]
case ERTOverlayMeaning::PathTrace:       return FColor(40, 220, 220);
case ERTOverlayMeaning::AttackOriginAim: return FColor(220, 220, 255);
case ERTOverlayMeaning::Attack:          return FColor(255, 69, 58);   // #FF453A [D-368]
case ERTOverlayMeaning::FriendlyFire:    return FColor(250, 155, 10);  // #FA9B0A [D-368]
case ERTOverlayMeaning::Hover:           return FColor(255, 214, 10);  // #FFD60A [D-368]
```

`FriendlyFire` e `Hover` sono presenti in **tutti e quattro** gli switch di `URTOverlayPalette` — `ColorFor`, `PriorityFor`, `ScaleFor`, `DrawsThroughUnits` — e il gate è `AreaOverlay.PaletteIsDistinguishable` senza esenzioni (`RTOverlayModelTests.cpp:182`).

∴ **il punto (2) di `D-364` non è implementato, ed è contraddetto dal codice spedito.** Non è drift silenzioso — è una decisione accettata che un'altra decisione accettata ha scavalcato senza dichiararlo.

### Perché blocca proprio questa roadmap

È il rischio di **§C1**, reso concreto. La Wave 1 scrive `FRTTargetPreview` contro `ERTOverlayMeaning`:

- se governa **`D-368`** → l'enum è stabile, e la Wave 1 può partire;
- se governa **`D-364`** → l'enum perde due voci e una va **rinominata**, e un modello scritto prima nasce contro un vocabolario in movimento.

🔑 La decisione minima richiesta è **una riga**: quale delle due governa `FriendlyFire` e `Hover`, e se `D-364` va marcata superata nel punto (2) o `D-368` nel punto (1). La conseguenza di non prenderla non è estetica: è che nessuna wave di questa roadmap può essere sequenziata sull'enum.

### Due note di contorno, misurate insieme

- ⚠️ `RTOverlayPalette.h:40` cita ancora `RefactorTactics.AreaOverlay.PaletteRatchet`, che **non esiste più** sotto quel nome. Fuori da `Source/` il nome sopravvive dove è corretto che sopravviva — nel corpo di `D-368`, che racconta la conversione, e in questo referto — ma in `Source/` resta **un solo lettore**, ed è quell'header:

  ```
  $ grep -rn "PaletteRatchet" --include=*.h --include=*.cpp Source/
  Source/RefactorTactics/Map/RTOverlayPalette.h:40
  ```

  Lì non è una nota storica: è una citazione a un test, e chi la segue non trova niente;
- 🔁 **il Decision Log non era ancora stato censito** quando questa sezione è stata scritta: `D-364` era emersa da `git log`, `D-368` dall'appendice, entrambe per caso. **Il censimento è stato fatto subito dopo ed è la sezione seguente** — il conflitto sopra sopravvive alla verifica.

## ✅ Censimento del Decision Log — la limitazione dichiarata in §5 è chiusa

> §5 dichiarava: *«il Decision Log non è stato censito … altre decisioni accettate che toccano questo perimetro possono esistere e non essere citate qui»*. **Esistono, e sono molte.** Questa sezione le nomina.

**Metodo.** Indice costruito dai titoli, non dai corpi:

```bash
grep -oE "^\| \*\*D-[0-9]+\*\* \| \*\*[^*]{0,170}" docs/decisions/RT_PDR_00_Decision_Log.md
# al passaggio di oggi il Log va da D-009 a D-370; nessun ID duplicato:
grep -oE "^\| \*\*D-[0-9]+\*\*" … | grep -oE "[0-9]+" | sort -n | uniq -d   # -> vuoto
```

### 1 · Il conflitto `D-364`/`D-368` non è risolto da nulla che venga dopo

`D-369` e `D-370` esistono e riguardano **i bersagli non-unità**, non la palette. ∴ il `CONTRACT CONFLICT` della sezione precedente **resta aperto** — non era un artefatto di lettura parziale.

### 2 · 🔴 Il difetto di §C1 si ripete sulle decisioni: la Wave 0 propone di decidere ciò che è già deciso

La tabella *«Congelare la grammatica»* della Wave 0 presenta come scelte da prendere righe che hanno già una voce accettata:

| Riga della Wave 0 | Decisione che la governa già |
|---|---|
| *«Lifetime: da action armed fino a cancel / confirm / lock-in»* | **`D-128`** — *«in stato neutro il click su un nemico ISPEZIONA; per bersagliare bisogna aver armato un'azione»* |
| *«Certainty: linea piena = confirmed; tratteggiata = predicted»* | **`D-235`** — *«`Certainty` e validità del piano sono due assi GIÀ separati nel dominio; la v0.1 riceve lo STATO binario di legalità durante il Planning»* · **`D-177`** — *«la certezza si rende solo sugli elementi che esistono per più di un livello, e la matrice è parte della grammatica»* |
| *«Il colore non deve essere l'unico canale»* | **`D-146`** — *«l'encoding è ridondante: mai solo il colore»*. Non è un criterio da proporre: è la regola che `D-364` discute quanto pesi su un overlay |
| *«Invalid: stesso preview geometrico, ma semantica Invalid»* | **`D-235`**, che separa già legalità da certezza — sono i due assi che la roadmap fonde in una riga |
| Wave 4, il marcatore del tiro bloccato | **`D-359`** — *«IL MARCATORE D'OSTACOLO VIVE FINO AL LOCK-IN SUCCESSIVO, E IL SUO STATO STA NELLA PRESENTAZIONE»* |
| Wave 1, *«`PreviewStyle` = metadato di presentazione esplicito»* | **`D-278`** — *«il legame evento risolto → presentazione è DATO DICHIARATIVO: ogni `ERTResolvedEventType` risolve in una voce di mapping oppure dichiara …»*. Il pattern esiste: la Wave 1 ne inventerebbe un secondo |

🔑 **È lo stesso difetto di §C1, spostato di un livello**: là la roadmap ricostruiva codice spedito, qui ridecide decisioni accettate.

### 3 · ⚠️ La premessa *«un solo linguaggio grafico condiviso»* è già stata decisa al contrario

L'obiettivo in testa alla roadmap chiede *«un solo linguaggio grafico condiviso da movimento, attacchi, AoE, LOS, hazard, obiettivi e stati non validi»*. Il Log ne registra **due, deliberatamente separati**:

- **`D-232`** — *«il colore dice la FASE, non la famiglia semantica»*, con **`D-233`** che fissa `Prep #56B4E9 · Dash #009E73 · Blast #D55E00 · Move #0072B2`;
- **`D-234`** — *«il colore del world overlay ha un vocabolario PROPRIO, e la fase `Dash` ne prende in prestito `#009E73`»*.

∴ i due vocabolari si incontrano esattamente nella **Wave 5**, dove il Target Preview diventa consumer della fase selezionata. La roadmap non può unificarli per premessa: o dichiara di superare `D-232`/`D-234`, o dice come convivono.

### 4 · Ciò che il censimento **regala** alla roadmap

| Rilievo | La decisione che lo risolve o lo rafforza |
|---|---|
| **§C7**, la riga PIE «Multilayer» | **`D-255`** — *«il click e l'hover risolvono la cella sul PIANO ATTIVO del giocatore, non sulla quota del punto colpito»*. Il «piano corretto» **ha già un nome**: è il piano attivo, e vive nel `PlayerController` |
| **§C9**, `FRTTargetPreview` non deve ridichiarare | **`D-370`** — *«il puntatore NON acquista un secondo campo»*: il vincolo è già scritto, e vale anche per la preview |
| **§C2**, il canale di rendering | **`D-124`** — *«E21 finisce alla leggibilità tattica necessaria per giocare e misurare la v0.1, non a un presentation pass»*: giustifica lo scope minimo della roadmap, e va citata a suo favore |
| Il non-goal *«non deve nascere un secondo ruleset»* | **`D-143`** (camera presentation-only), **`D-243`** (gli spicchi sono presentazione), **`D-269`**/**`D-270`** (muri e geometria intra-cella occludono LoS e proiettili, e sono AUTOREVOLI): la traiettoria di Wave 3 non può contraddirli |
| Cosa la preview **non** può disegnare | **`D-225`** · **`D-249`** — *«l'ignoto non si disegna e non si raggiunge»*. Vincolo di privacy sul range di movimento e sulle linee di tiro, già accettato |
| L'area colpita | **`D-301`** — l'impronta a terra è **un evento proprio**, `ERTResolvedEventType::AttackFootprint`, non un campo del colpo |

### 5 · Uno **zero misurato**, ed è dove la roadmap è più esposta

```bash
grep -inE "preview|anteprima|ghost|timeline|fase selezionat" <indice del Log>   # -> 0
```

**Nessuna decisione accettata parla di Ghost Timeline o di preview per fase.** La Wave 5 — che la roadmap dà per la più semplice, *«non servono quattro renderer»* — è l'unica che opera in un'area **senza contratto**, ed è anche quella dove i due vocabolari di colore di §3 si incontrano. Non è la wave facile: è la wave scoperta.

### ⛔ Cosa questo censimento **non** ha fatto

Ha letto i **titoli**, non i corpi. Una decisione il cui titolo non nomina il perimetro ma il cui corpo lo tocca **non è stata vista** — e `D-364` insegna che i corpi contengono clausole che i titoli non annunciano. Le voci nominate qui vanno lette per intero prima di essere usate come vincolo.

### 🔁 E il censimento era già scaduto quando è stato scritto — `D-371`…`D-374`

Fra la stesura della sezione qui sopra e il suo push, `origin/main` è avanzato e ha portato **quattro decisioni nuove**, tutte sulla privacy del planning e sull'ignoto. Non è un aneddoto: è la dimostrazione che un censimento è una **misura datata**, non uno stato.

⚠️ **Il conflitto `D-364`/`D-368` resta intatto** — misurato, non supposto:

```bash
grep -oE "^\| \*\*D-37[1-4]\*\*.*" <Log su origin/main> | grep -ocE "FriendlyFire|Hover|D-364|D-368"   # -> 0
```

Ma due delle quattro cambiano il terreno sotto la roadmap:

| | Cosa dice | Cosa muove |
|---|---|---|
| **`D-371`** | *«IL PLANNING SMETTE DI ESSERE UN SIMULATORE FEDELE DEL RESOLVER: nasce la SECONDA CLASSE DI PRIVACY — quella della CONOSCENZA — e vale per TUTTI, bot compreso»*. **Emenda `D-249`** | la §4 di questo censimento cita `D-225`/`D-249` come *«l'ignoto non si disegna»*: quella riga è **emendata**, e va riletta lì |
| **`D-372`** | *«UN MOVE VERSO L'IGNOTO SI PIANIFICA SU MAPPA OTTIMISTICA — «ignoto = passabile» — E IL TRACCIATO OLTRE IL NOTO SI DICHIARA INCERTO»*, con un trilemma esplicito: l'anteprima non rivela ciò che il giocatore non sa · l'anteprima non mente · nessuna mappa ottimistica — **non si possono avere tutti e tre** | 🔴 **`Certainty` smette di essere una scelta di resa e diventa un meccanismo di privacy** |
| **`D-373`** | *«LA FORMA DELLA BOARD — QUALI CELLE ESISTONO — È INFORMAZIONE PUBBLICA; COSA CONTENGONO NO»* | delimita cosa un overlay può disegnare senza rivelare |
| **`D-374`** | hazard e blocker ignoti non hanno una policy propria: le regole d'arresto che il resolver già possiede li coprono | evita una policy in più nel canale della preview |

🔴 **La conseguenza per la Wave 0 è la più pesante di tutto questo referto.** La riga *«Certainty: linea piena = confirmed; tratteggiata = predicted»* è presentata come una scelta di grammatica visiva. Dopo `D-372` è **la metà visibile di un contratto di privacy**: se il canale della certezza non distingue, l'anteprima o mente o rivela. ∴ sbagliarlo non è un difetto di leggibilità — è un difetto di privacy.

⚠️ **E questo rende §C2 più grave, non meno.** Il canale che oggi porta la certezza è il **tratteggio su `DrawDebugLine`** — scelto perché quel renderer *ignora l'alpha*. Un contratto di privacy che poggia su un canale scelto per i limiti del debug draw è il punto in cui §C2 smette di essere una questione di resa.
