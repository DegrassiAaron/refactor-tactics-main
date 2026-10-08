# Il profilo FX per abilità — attivazione e colpo, una tabella `ActionId → profilo` sopra la forma

> **Statuto**: design **accettato** il 2026-10-08 sulle decisioni d'autore D1–D5 (§0, prese lo stesso giorno: D1–D4 tutte le
> raccomandate, D5 dopo la stesura). ➕ rev. **Rivista dal panel il 2026-10-08** (`.superpowers/sp4-spec-panel.md`, verdetto *APPROVATA CON
> MODIFICHE*, finding F1–F22): le modifiche sono incorporate e marcate `➕ rev.`, quelle che ribaltano un `Ruling` della
> prima stesura lo dicono col `⌫`. ➕ rev2. Re-review mirata dello stesso giorno (`.superpowers/sp4-spec-rereview.md`, OK per
> il piano): gli ultimi ritocchi sono marcati `➕ rev2.`. **Implementato** in [#3578](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3578) sul branch
> `issue/3578-profilo-fx-per-abilita` (blocco «Implementato» qui sotto), PR [#3585](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3585) verso `main`, aperta il 2026-10-08; la voce
> `PIE-FX-ABILITA` resta da eseguire. È il **quarto di quattro sotto-progetti** della richiesta d'autore
> *«associare animazioni e FX alle skill e vederle in azione»*: il banco Ability Lab → PIE
> ([#3532](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3532)), il momento `AbilityActivated`
> ([#3549](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3549), spec
> [`2026-10-07-momento-ability-activated-design.md`](2026-10-07-momento-ability-activated-design.md)) e la clip per
> abilità ([#3563](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3563), spec
> [`2026-10-07-clip-per-abilita-design.md`](2026-10-07-clip-per-abilita-design.md)) sono in `main`. Owner della
> grammatica che questa spec **consuma**: [#2454](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454)
> (*Basic Combat Cues*, aperta), epic [#2453](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2453). La
> issue di lavoro, aperta col piano, è [#3578](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3578).
>
> **Stato misurato**: 2026-10-08, `origin/main` = `ef49b454d`. Ogni `file:riga` è stato letto su quel commit, in sola
> lettura, sul checkout `rt-wt-sp4-fx`; chi lo rilegge più tardi lo **rimisura**. Nessun totale volatile: dove serve
> una misura c'è il comando, dove serve un elenco ci sono i nomi. Le scelte di chi scrive sono marcate `Ruling`, ognuna
> col suo costo se è sbagliata; sono revocabili dall'autore. Le parti di specifiche sorelle che questa supera sono
> elencate in §2.9 con `⌫`.
>
> 🔴 **Una tensione fra le regole d'ingresso, da far giudicare all'autore** (§2.2, R3): D1 chiede *«tracer diversi per
> eroe e abilità»*, e la regola di sessione vuole il tracer dell'**attacco base** identico a schermo (regressione zero su
> `PIE-V01-TRACER`). Gli attacchi base sono gli unici colpi `Single`/`Line` a distanza del roster: con la regola, la
> varietà per eroe del tracer ha **un** consumatore (`Hero.Aevik.LinearDischarge`). La spec segue la regola e mette
> l'alternativa nella tabella.
>
> ✅ **Sciolta dall'autore il 2026-10-08** (AskUserQuestion): **tracer identico** sugli attacchi base (R3 confermata); la varietà per eroe vive sugli override; gli attacchi base ricevono comunque il marcatore d'impatto all'arrivo (grammatica `Single` di #2454). La mappa degli override di §2.2 è **approvata così** (D5).
>
> ✅ **Implementato il 2026-10-08** sul branch `issue/3578-profilo-fx-per-abilita`, da `ef49b454d`, col piano
> [`2026-10-08-profilo-fx-per-abilita.md`](../plans/2026-10-08-profilo-fx-per-abilita.md): un commit per task, più i giri di
> review. Spec e piano `a2dd733b6`; tipi, default per forma, override e ripiego `530b3ef38`; il tracer che segue il
> profilo `70f86b4a9`, con i due fix della sua review `f788d1724` (puntatori, `Action.Push` senza proiettile) e `fb7c0d47f`
> (R15, qui sotto); il segnale di attivazione — `Ev.Origin`, la cue dietro `SourceVerdict`, il canale di mappa —
> `17d83a223`; le cue di colpo (`Marker`, `AreaPulse`, `ConeSweep`, R14) `379c0c87f`; il disegno col line batcher e D-278
> `e835c5fdd`; lo scenario `Visual.Ability.FxProfile` `ecb008548`; il merge di `origin/main` `867a4d0a3` nel branch
> `b70faca87` (nessun file in comune); registro PIE, seduta `U71`, capability map, conoscenza parziale, spec superate, piano
> e questo statuto nel commit `docs(3578)` che porta questo blocco. Le correzioni emerse in esecuzione sono marcate
> `➕ esecuzione` (§2.1, §2.2, §5.1, §5.2, §6, §7); il piano porta le sue con lo stesso marcatore.
>
> **Gate**, ciascuno sul commit del proprio task, con il log nei report della sessione (non versionati), tutti con
> `**** TEST COMPLETE`, nessun `Result={Fail}` e nessun `Ensure condition failed`: compile `PASS` su ogni commit di
> codice; `RefactorTactics.Fx` e `Presentation` + `Playback.Tracer` `PASS` su `530b3ef38`; le famiglie di mondo il cui
> ritmo può cambiare (`Playback`, `Privacy`, `Reactions`, `Turn`, `HexMatch`, `HexBotPlay`, `Actions`, `HexBlast`,
> `HexMapActor`) `PASS` **prima** (su `530b3ef38`) e **dopo** (sul contenuto di `70f86b4a9`), con lo stesso insieme di nomi più
> i test nuovi e nessuna differenza d'esito; `Fx` + `Playback` + `Privacy`, i gate del tracer e
> `Unit.BlueprintSurfaceIsCensused` `PASS` su `70f86b4a9`, `f788d1724` e `fb7c0d47f`; `Playback` + `Determinism` +
> `HexMapActor` + `Privacy` + `Fx` + `Unit.BlueprintSurfaceIsCensused` e `Turn` + `Replay` `PASS` su `17d83a223`; `Playback` +
> `Privacy` + `Determinism` + `HexMapActor` + `Presentation` + `Fx` + `Unit.BlueprintSurfaceIsCensused` `PASS` su `379c0c87f`;
> `Fx` + `Preview` + `Presentation` + `HexMapActor` + `Playback` + `Unit.BlueprintSurfaceIsCensused` `PASS` su `e835c5fdd`;
> `Scenario.EveryShippedScenarioRuns` `PASS` su `ecb008548`, con `Visual.Ability.FxProfile` verde; `Fx` +
> `Playback.TracerFollowsTheProfile` `PASS` sull'albero del commit `docs(3578)` (solo commenti di sorgente). Ogni test nuovo
> è stato visto rosso prima del codice (di compilazione, o `Result={Fail}` per i test di disegno e di binding del Task 5), e
> ciascuna mutazione di §5.1 e del piano è caduta sul proprio asserto. Determinismo: `PASS` limitato a `Determinism.*`;
> `Determinism.FxFieldsStayOutOfHashes` è un **tripwire** — oggi nessun hash legge `ResolvedTimeline`, e il controllo
> positivo prova che il gancio agisce. Replay: `RefactorTactics.Replay` `PASS` su `17d83a223`. Privacy: la famiglia
> `RefactorTactics.Privacy` `PASS`. ⚠️ Ogni log porta due `Condition failed` all'avvio del motore, al fotogramma `[0]`,
> prima di qualunque test: c'erano già sulla base (`530b3ef38`), nessun `Result={Fail}` li accompagna. ➕ esecuzione (Task 8). **Suite intera**
> `RefactorTactics` su `f9e9a08dc` (albero con `origin/main` `867a4d0a3` fuso): `Result={Success}` 3042, `Result={Fail}` 1 —
> il solo `Packaging.RequiredActionClipsAreCooked`, rosso ereditato da `main` (#3563, owner #3562) —, nessun
> `Ensure condition failed`, con `**** TEST COMPLETE`; i commit successivi toccano solo documenti. `NOT RUN`: la PIE
> (`PIE-FX-ABILITA`, seduta `U71`); il pacchetto.
>
> **Eccezioni dichiarate**, ognuna con la sua ragione: (a) `Playback.TracerStyleFollowsShapeForBasicAttack` è l'unico test
> esistente riscritto (§5.1); (b) la mutazione (2) è eseguita in forma compilabile (§5.1); (c) `Fx.ConeSweepAxisIsTheAim`
> usa letterali `double`, perché `TestEqual(double, float, float)` non compila con `FVector` in doppia precisione (`C2666`),
> asserti e tolleranze invariati; (d) al Task 4 le mutazioni hanno un **solo** verde finale, sul commit pulito, invece di
> un verde dopo ogni ripristino: la review lo ha accettato con tre misure — l'albero finale è il commit e la build finale è
> posteriore all'ultima scrittura, il binario finale contiene i sorgenti ripristinati, e ogni log di mutazione mostra solo
> le cadute della propria mutante; (e) al Task 3 il parametro di `PushPlaybackCues` è `InPhase`, perché `Phase` è un membro
> di `ARTTurnManager` (`C4458`), e le due manopole non hanno `ClampMin`, come le vicine (§2.1).
>
> ➕ esecuzione. `Ruling` **R15** (controller, review del Task 2): un'azione **core** che conta come attacco ed è **a
> contatto** non disegna un proiettile, come `Ram`; il criterio è la portata, le righe restano esplicite (§2.2). Costo se
> sbagliato: una riga per azione. ➕ esecuzione. **F1 è chiusa dal fatto, non dalla dipendenza**: `PIE-V01-TRACER` è stata
> eseguita su `main` (seduta `U68`) **prima** di #3578 (§2.2). I follow-up emersi in esecuzione sono in §7.

---

## 0. Le decisioni d'autore (2026-10-08) — non si riaprono

| # | Domanda | Decisione |
|---|---|---|
| D1 | Canale | **Primitive del line batcher** ([D-467](../../decisions/RT_PDR_00_Decision_Log.md), `DisegnaLineaAnteprima`): ring/pulse di attivazione sulla sorgente; tracer diversi per eroe e abilità; `Area` = pulse radiale sull'`AimCell`; `Cone` = sweep. Nessun asset, niente Niagara/Cascade ([D-124](../../decisions/RT_PDR_00_Decision_Log.md), #2453). |
| D2 | Supporto del dato | **Tabella C++ accanto a `URTPresentationBindingLibrary`** ([D-278](../../decisions/RT_PDR_00_Decision_Log.md)): default derivati da `Shape`, override per `ActionId`; ogni cambio ricompila; migrazione ad asset possibile senza cambiare il gate. |
| D3 | Granularità | Ogni `AbilityActivated` dà il **segnale di attivazione** sulla sorgente; ogni `Attack` dà la **cue di default della sua `Shape`** e l'override per `ActionId` dove l'autore lo giudica; le abilità di sola Prep hanno solo l'attivazione. |
| D4 | Perimetro vs #2454 | SP4 implementa **attivazione + colpo per le quattro `Shape`**, consumando la grammatica di #2454; `ReactionResolved` e `Defeated` restano a #2454. Il tracer esistente diventa **un caso** del profilo. |
| D5 | La mappa e gli attacchi base (dopo la stesura) | **Tracer identico sugli attacchi base** (R3 confermata, pinnata come regola da `Playback.BasicAttackTracersEqualShapeDefault`, §5.1); la mappa degli override di §2.2 è **approvata così**; gli attacchi base ricevono il marcatore d'impatto. |

---

## 1. Il problema, misurato

**Nessun segnale visivo di attivazione esiste.** `ShowActivation` (`Turn/RTTurnManager.cpp:8096-8106`) scrive la riga
`Attiva:` e suona la clip `Cast` (`:8104`); null'altro. La voce D-278 di `AbilityActivated` dichiara solo
`PlayCastMontage` (`Turn/RTPresentationBinding.cpp:271-272`) e il commento accanto (`:265-267`) lascia il ring/pulse a
#2454. Il momento esiste (sotto-progetto 2), la clip per abilità esiste (sotto-progetto 3): sul terreno non si vede
niente.

**Il tracer è l'unico FX del colpo, ed è cablato sugli attacchi base.** `IsTracerEligible`
(`Turn/RTPlaybackLibrary.cpp:130-137`) pretende `ActionId` o `BaseActionId` == `Action.BasicAttack`, `Shape` `Single`
o `Line` e `HitGeometry.bResolved`; il suo commento la dichiara *«PROVVISORIA … la sostituisce la tabella `ActionId ->
profilo` del sotto-progetto 4»* (`Turn/RTPlaybackLibrary.h:277-282`). `TracerStyleFor` (`.cpp:139-148`) sceglie lo stile
**solo dalla forma** (`:147`). `ERTTracerStyle` ha `None`, `Projectile`, `Jet` (`Map/RTPlaybackTracer.h:13-22`). Il
disegno vive in `ARTHexMapActor` (`Map/RTHexMapActor.cpp:1659-1679`), col colore
`URTOverlayPalette::ColorFor(ERTOverlayMeaning::Attack)`, costanti graybox a `:176-179`, canale
`SetPlaybackTracers`/`ClearPlaybackTracers` (`:1178-1188`, dichiarato in `Map/RTHexMapActor.h:917-930`).

**Il colpo non ha un momento d'impatto disegnato.** All'arrivo (`ArrivePlaybackAttack`, `RTTurnManager.cpp:8296-8322`)
suonano `Hit`, il numero e `OnAttackResolved`; sulla cella non resta nulla. `Area` e `Cone` non hanno nessuna cue di
colpo oltre all'impronta (`AddPlaybackFootprint`, `:8239-8244`).

**Il tempo c'è già, e c'è un cursore solo.** Nel Blast, `BlastBeatsDone` (`Turn/RTTurnManager.h:3479`) percorre la
sequenza per intento: il battito `2k` rivela l'elemento `k` (per un colpo, il **lancio**, `:8255-8259`), il `2k+1` è
l'**arrivo** (`:8207-8230`), con i voli in `PlaybackBlastFlights` (`.h:3490`) calcolati in `BeginPlayback`
(`.cpp:7951-7975`) da `IsTracerEligible` e `TracerFlightFor` (≤ `A/2`, `RTPlaybackLibrary.cpp:79-86`). In Prep e Dash le
attivazioni escono una per volta con `AttacksToShow` sullo stesso `A` (`:8853-8862`). Le manopole sono
`PhaseBeatSeconds = 0.30`, `AttackShowSeconds = 0.50`, `TracerFlightSeconds = 0.25` (➕ rev. F14: `RTTurnManager.h:1305-1306`,
`:1309-1310`, `:1317-1318`, `UPROPERTY` e dichiarazione).

**I dati dell'evento.** `AbilityActivated` porta `ActionId`, `BaseActionId`, `AimCell`, `Shape` e `SourceVerdict`
(emissione `RTTurnManager.cpp:333-346`; `SourceVerdict` a `:344`), **non** la cella della sorgente. `Attack` porta
`Shape` dall'intento e `HitGeometry` (`:6378-6392`), con `From` da `ResolveImpactOrigin` — che per un'`Area` rende il
**centro** d'impatto, `Footprint->AimCell` (`:2566-2579`; avvertenza in `Turn/RTResolvedEvent.h:196-197` e `:205-207`) —
**non** l'`AimCell`: la nota di `RTResolvedEvent.h:407-408` dichiara `AimCell` solo per `AttackFootprint` e
`AbilityActivated`. `FRTResolvedEvent` e `FRTHitGeometry` sono `RTServerOnly` (`RTResolvedEvent.h:185`, `:241`).
➕ rev. (F5) **L'impronta li porta già**: ogni `AttackFootprint`, uno per intento, scrive `Origin = Footprint.Origin` e
`AimCell = Footprint.AimCell` (`RTTurnManager.cpp:5605-5611`), e nella sequenza del Blast precede i colpi del proprio
atto (spec del momento §2.4). `ResolvedTimeline` non entra in `StateHash` né nel replay (panel: `RTTurnManager.cpp:2866`,
`:2952`, `:3019`, `:3111`).

**La forma di ogni abilità del roster**, misurata su `Ability/RTHeroCatalogLibrary.cpp` (righe in §2.2): attacchi base
`Single` tranne `Hero.Muiren.PressureJet` (`Line`); `Hero.Aevik.LinearDischarge` `Line`; `Hero.Aevik.Overload`,
`Hero.Muiren.CircularTide`, `Hero.Muiren.MistVeil`, `Hero.Branth.MortarShot` `Area` raggio 1; ogni altra `Single` (il
default di `MakeHeroAction`, `:99`, e di `MakeHeroActionFromCore`, `Ability/RTHeroCatalogLibrary.h:223-224`).
🔴 **Nessuna azione dichiara `Cone`**: `git grep -n "ERTAbilityShape::Cone" -- Source ':!Source/RefactorTactics/Tests'`
risponde solo `Combat/RTHexCombatLibrary.cpp:42` (la geometria) e `UI/RTHudViewModel.cpp:464` (l'etichetta). Lo zero è
il difetto, non un totale: `Cone` non ha contenuto in partita (§3, §6).

**Le regole del canale.** D-124 (`docs/decisions/RT_PDR_00_Decision_Log.md:178`): dentro *«eventi `Cast / Hit /
Death`; … ring leggibili»*, fuori *«Niagara dedicato a ogni abilità»*. D-278 (`:327`): evento → presentazione è dato
dichiarativo. D-467 (`:530`): line batcher, mai `DrawDebug*`. D-297 (`:345`): la sorgente Paragon di un'abilità è
presentazione, e la fedeltà non è un criterio. D-287 (`:336`): il ritmo è presentazione, i tempi sono proposte da
playtest.

---

## 2. Il disegno

### 2.1 Il profilo

```cpp
// Map/RTPlaybackTracer.h — lo stesso file del tracer: e' la presentazione di playback IN CELLE.

UENUM(BlueprintType)
enum class ERTTracerStyle : uint8
{
	None, Projectile, Jet,
	/** ➕ SP4. Un getto spezzato: ancorato all'origine come `Jet`, vertici funzione di (From, To) soltanto. */
	Zigzag,
};

/** Il segnale di attivazione sulla sorgente (D1, D3). */
UENUM(BlueprintType)
enum class ERTActivationFxStyle : uint8 { None, Ring, Pulse, Flash };

/** ➕ rev. (F5, F6) La cue di OGNI `Attack`, all'arrivo, sulla sua vittima. */
UENUM(BlueprintType)
enum class ERTImpactFxStyle : uint8 { None, Marker };

/** ➕ rev. (F5, F6) La cue dell'IMPRONTA dell'atto: una per `AttackFootprint`, all'arrivo del primo colpo che la segue. */
UENUM(BlueprintType)
enum class ERTFootprintFxStyle : uint8 { None, AreaPulse, ConeSweep };

/** Il profilo FX di un'azione (D2). Presentazione pura: non entra in snapshot, TurnLog, StateHash, replay. */
USTRUCT()
struct FRTAbilityFxProfile
{
	GENERATED_BODY()
	// ➕ rev. (F17) Default TUTTI `None`: una voce si costruisce solo con `MakeFxProfile(A, T, I, F)`, quattro argomenti
	// obbligatori, cosi' una riga che dimentica un campo non eredita in silenzio.
	UPROPERTY() ERTActivationFxStyle Activation = ERTActivationFxStyle::None;
	UPROPERTY() ERTTracerStyle Tracer = ERTTracerStyle::None;
	UPROPERTY() ERTImpactFxStyle Impact = ERTImpactFxStyle::None;
	UPROPERTY() ERTFootprintFxStyle Footprint = ERTFootprintFxStyle::None;
};
```

⌫ ➕ rev. *La prima stesura aveva un solo enum di colpo (`Marker`, `AreaPulse`, `ConeSweep`) e una regola fuori dal
profilo (R8: pulse sul primo colpo dell'atto, `Marker` sui successivi), che il panel (F6) ha mostrato non definita per un
override `Impact = None` su un'`Area`.* Ora ogni campo ha **un** evento e **un** momento, scritti qui e da nessun'altra
parte:

| Campo | Evento che lo porta | Momento | Cella |
|---|---|---|---|
| `Activation` | `AbilityActivated` | rivelazione | `Ev.Origin` della sorgente (§2.3) |
| `Tracer` | `Attack` | dal lancio (`2k`) all'arrivo (`2k+1`) | `HitGeometry.From → Impact` (invariato) |
| `Impact` | **ogni** `Attack` (ogni vittima è un evento) | arrivo | `HitGeometry.Impact` |
| `Footprint` | l'`AttackFootprint` dell'atto | arrivo del **primo** `Attack` che la segue nel suo atto | `Origin`/`AimCell` dell'impronta (§2.4) |

⛔ **Il canale è la geometria, mai il solo colore** (#2453). ➕ rev. (F7) Le scale sono numeri, in frazioni di
`s = HexSize`; sono graybox (D-287 punto 7) e si tarano in PIE, ma **restano diverse a coppie**, e lo pinna
`Fx.CueStylesDifferByGeometry` (§5.1):

| Stile | Segmenti | Ancoraggio | a `α = 0` | a `α = 1` | Verso | Spessore |
|---|---|---|---|---|---|---|
| `Ring` | 6 (un esagono) | centro della sorgente, nel piano | raggio `0.55 s` | `0.95 s` | si allarga | 3 |
| `Pulse` | 12 (due esagoni concentrici) | centro della sorgente, nel piano | `1.00 s` e `0.75 s` | `0.60 s` e `0.35 s` | si stringono | 3 |
| `Flash` | 6 raggi | dai vertici a `0.5 s`; ➕ rev. (F21) **inclinati di 45°** verso l'alto e l'esterno, non verticali, perché dalla camera tattica un segmento verticale si proietta quasi in un punto | lunghezza `0.4 s` | `0.4 s` (fissi) | nessuno: un lampo | 4 |
| `Marker` | 4 raggi a X | centro della vittima, nel piano | `0` | `0.35 s` | crescono | 3 |
| `AreaPulse` | 12 (un esagono + 6 raggi dal centro ai vertici) | `AimCell` dell'impronta | `0.3 s` | `1.7 s` | si allarga | 3 |
| `ConeSweep` | 3 (due bordi fissi a `±60°` lunghi `0.3 L`, un braccio lungo `L`) | `Origin` dell'impronta, `L = \|Origin → AimCell\|` | braccio a `−60°` | braccio a `+60°` | ruota | 4 |
| `Zigzag` | 8, scarto laterale `±0.12 s` alternato | `HitGeometry.From`, ancorato | prefisso lungo `α` | intero fino a `Impact` | cresce | ➕ rev. (F11) 5 |

`Projectile` e `Jet` non cambiano (`RTHexMapActor.cpp:176-179`, `:1676-1677`). Colore da `URTOverlayPalette::ColorFor`:
`Attack` per tutto, come oggi (`RTHexMapActor.cpp:1667`). `Ruling` R1: l'attivazione usa **lo stesso**
`ERTOverlayMeaning::Attack`, nessun significato nuovo: un ottavo significato per una variante di resa è il difetto che
#1941 esiste per chiudere (`RTHexMapActor.cpp:1621-1623`). Costo se sbagliato: chi vuole un colore d'attivazione lo
aggiunge alla palette con il suo test di priorità.

**Le durate**, `Ruling` R2: due manopole su `ARTTurnManager`, accanto a `TracerFlightSeconds`, ➕ rev. (F18) con gli
stessi specificatori delle vicine — `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback",
meta = (ClampMin = "0.0"))` — ⛔ niente su `ARTUnit`:

| Manopola | Default | Durata effettiva | Perché il tetto |
|---|---|---|---|
| `ActivationCueSeconds` | `0.35` | `D_act = A > 0 ? Min(ActivationCueSeconds, A) : 0` | l'elemento dopo esce a `(k+1)·A` |
| `ImpactCueSeconds` | `0.20` | `D_imp(k) = A > 0 ? Min(ImpactCueSeconds, A − F_eff(k)) : 0` | l'arrivo cade a `k·A + F_eff(k)`, il lancio dopo a `(k+1)·A` |

Con `A = AttackShowSeconds` e `F_eff` di `TracerFlightFor`. `Impact` e `Footprint` usano `D_imp`. Le durate **non**
sono nel profilo: sono ritmo (D-287 punto 7). `PhaseBeatSeconds` non entra: è lo `Slack` della Prep.
➕ esecuzione (Task 3). Le vicine **non** hanno `ClampMin` (`TracerFlightSeconds`, `ARTTurnManager`): applicata la regola
«gli stessi specificatori delle vicine», le due manopole sono `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category =
"RefactorTactics|Playback")`, senza `meta`.

🔑 **Conseguenza dei tetti**: nel Blast le cue dell'elemento `k` vivono dentro `[k·A, (k+1)·A)`, quindi le cue di
elementi diversi non si sovrappongono, e il tracer finisce dove comincia la cue d'arrivo. Al primo arrivo di un atto
`Area` si vedono **due** segni — l'`AreaPulse` dell'impronta sul centro e il `Marker` del colpo sulla vittima — e sono di
**due** eventi: «un evento → un segnale» di #2453 resta vero per evento. ⛔ `PhaseTime` non cambia firma né valore.

### 2.2 La tabella e la mappa degli override (giudizio dell'autore)

In `Turn/RTPresentationBinding.{h,cpp}`, accanto a `DeclaredBindings`, funzioni **statiche C++, non `UFUNCTION`**:

```cpp
static FRTAbilityFxProfile MakeFxProfile(ERTActivationFxStyle, ERTTracerStyle, ERTImpactFxStyle, ERTFootprintFxStyle);
static FRTAbilityFxProfile DefaultFxProfileFor(ERTAbilityShape Shape);
static const TArray<TPair<FName, FRTAbilityFxProfile>>& DeclaredFxOverrideRows();  // ➕ rev. (F17) le righe, in ordine
static const TMap<FName, FRTAbilityFxProfile>& DeclaredFxOverrides();               // costruita una volta dalle righe
static FRTAbilityFxProfile FxProfileForIn(const TMap<FName, FRTAbilityFxProfile>& Overrides,
	FName ActionId, FName BaseActionId, ERTAbilityShape Shape);                     // pura: i test le passano una tabella
static FRTAbilityFxProfile FxProfileFor(FName ActionId, FName BaseActionId, ERTAbilityShape Shape);
```

**Default per forma** (D3, grammatica di #2454):

| `Shape` | `Activation` | `Tracer` | `Impact` | `Footprint` |
|---|---|---|---|---|
| `Single` | `Ring` | `Projectile` | `Marker` | `None` |
| `Line` | `Ring` | `Jet` | `Marker` | `None` (l'impronta resta quella di oggi) |
| `Area` | `Ring` | `None` | `Marker` | `AreaPulse` |
| `Cone` | `Ring` | `None` | `Marker` | `ConeSweep` |

**Ripiego**, lo stesso delle clip: `Overrides[ActionId]` → `Overrides[BaseActionId]` → default della `Shape`. Un id
vuoto salta il proprio livello; `BaseActionId` si legge dall'evento, mai derivato (`RTResolvedEvent.h:393-396`).
⛔ `DerivedFromActionId` **non** è un livello: non è sull'evento. Il profilo è **intero** per livello (`Ruling` R4:
nessuna fusione per campo; costo: una voce ripete i campi che non cambia).

➕ rev. (F8) `Ruling` R12 — **nessuna azione, nessun profilo.** Con `ActionId` **e** `BaseActionId` entrambi vuoti,
`FxProfileFor` rende il profilo tutto `None` senza passare dalla forma: nessun tracer, nessuna cue, nessun volo. Sono gli
attacchi legacy di `ARTUnit::MakeAbility` (`Unit/RTUnit.cpp:1606-1615`, che non scrive `ActionId` e dà `Single` o
`Area`): oggi non hanno volo perché `IsTracerEligible` vuole `Action.BasicAttack`, e restano così. Il ritmo del Blast dei
test di mondo con unità di default non cambia. Costo: le unità legacy non hanno FX; un'azione nuova li ottiene
dichiarando un id.

➕ rev. (F9) `Ruling` R13 — **il volo lo decide la forma, il disegno l'override.** ⌫ *La prima stesura faceva decidere il
volo al profilo, e il ritardo fra lancio e numero avrebbe rivelato, di un attaccante non visto, l'override della sua
azione.*

```cpp
// Il VOLO, quindi il ritmo: la sola forma di default di un'azione con un id. Non legge l'override ne' chi guarda.
IsTracerEligible(Ev) = Ev.Type == Attack && Ev.HitGeometry.bResolved
                       && !(Ev.ActionId.IsNone() && Ev.BaseActionId.IsNone())                       // R12
                       && DefaultFxProfileFor(Ev.Shape).Tracer != ERTTracerStyle::None;
// Il DISEGNO: lo stile dell'override, oppure None = nessun disegno con lo STESSO volo.
TracerStyleFor(Ev, V)  = !IsTracerEligible(Ev) || !FromVerdict.AllowsTeam(V) || !ImpactVerdict.AllowsTeam(V)
                         ? None : FxProfileFor(Ev.ActionId, Ev.BaseActionId, Ev.Shape).Tracer;      // :147
```

Ne discende che `Hero.Branth.Ram` (override `Tracer = None`) ha **lo stesso volo** di `Hero.Branth.ImpactShot`, e non lo
disegna. Un override **non può aggiungere** un tracer a una forma che non ne ha di default (`Area`, `Cone`): non avrebbe
volo, e `Fx.DeclaredOverridesMatchTheProposal` lo asserisce su ogni riga. Costo: un colpo senza disegno ha un ritardo di
`F_eff` fra clip `Attack` e numero; il giudizio è a schermo (§5.2).

🔑 **Per un attacco base il risultato è identico a oggi**: `Single` → `Projectile`, `Line` → `Jet`, stesso volo. Lo
pinna **come regola**, non come riga, `Playback.BasicAttackTracersEqualShapeDefault` (➕ rev. F10, §5.1). Cambia per i
colpi **non** base con un id e forma `Single`/`Line`: da oggi hanno un volo, e un tracer salvo override.
➕ esecuzione. Con il volo si allarga anche il **disegno**: ogni azione non base di forma `Single`/`Line` senza riga
d'override riceve il tracer di default della sua forma (D3), dove prima non c'era nulla — per esempio le azioni core a
distanza che contano come attacco, `Action.Pull` fra queste. È ciò che D3 prescrive; dichiarato in §6, la PIE lo giudica.

**La mappa.** `Ruling` R3, confermata dall'autore (D5): **nessun override di tracer sugli attacchi base**. Le ragioni
sono il nome dell'abilità e la sua natura, non la fedeltà al pack (D-297). «fase» = quella del suo `AbilityActivated`;
«nessun beat» = non emette `AbilityActivated` in v0.1. Ogni riga si cambia con un commit di una riga più la sua gemella
nel test (§5.1). Notazione: `Activation · Tracer · Impact · Footprint`.

| Eroe | `ActionId` (riga di catalogo) | `Shape` | natura · fase | default | **override** | ragione (una riga) |
|---|---|---|---|---|---|---|
| Aevik | `Hero.Aevik.ArcPulse` (`:353`) | `Single` | attacco base · Blast | Ring · Projectile · Marker · None | — | R3 |
| | `Hero.Aevik.LinearDischarge` (`:361-363`) | `Line` | linea · Blast | Ring · Jet · Marker · None | **Ring · Zigzag · Marker · None** | «scarica lineare»: elettricità che si spezza; il getto liscio è acqua (`PressureJet`). ➕ rev. (F1) cambia la scena (2) di `PIE-V01-TRACER`, qui sotto |
| | `Hero.Aevik.ConductiveNode` (`:383-384`) | `Single` | Environment | — | — | nessun beat in v0.1 |
| | `Hero.Aevik.Overload` (`:405-407`) | `Area` r1 | area · Blast | Ring · None · Marker · AreaPulse | **Flash · None · Marker · AreaPulse** | «sovraccarico»: la carica si scarica in un istante sulla sorgente prima dell'onda |
| | `Hero.Aevik.ReactiveCapacitor` (`:416`) | `Single` | reazione | Ring · Projectile · Marker · None | — | non si attiva. ➕ rev. (F22) ➕ rev3. **Misurato il 2026-10-08: il contrattacco non produce un evento `Attack`** — l'unico `Type = Attack` è a `RTTurnManager.cpp:6357`, nel ciclo sui colpi del piano; i contrattacchi entrano in `Attacks` dopo (`:6460-6468`, commento `:6470`); `Action.Counter` ha `Range 0` e trigger `HitByDirectAttack`. Quindi nessuna cue di colpo e nessuna riga d'override; il piano lo conferma con un grep al Task 4 Step 0 |
| Muiren | `Hero.Muiren.PressureJet` (`:496-502`) | `Line` | attacco base · Blast | Ring · Jet · Marker · None | — | R3 |
| | `Hero.Muiren.CircularTide` (`:518-522`) | `Area` r1 | cura ad area · Blast | Ring · None · Marker · AreaPulse | **Pulse · None · None · None** | «marea circolare» cura: due anelli che si stringono si leggono come sostegno. ➕ rev. Se la cura non emette `Attack` né `AttackFootprint`, `Impact` e `Footprint` non hanno consumatore: il piano lo misura ⌫ #3593: `Pulse · None · None · None` — la cura passa dalle cure e non emette `Attack`: `Marker` e `AreaPulse` erano dato morto (spec SP5 F4, R7) |
| | `Hero.Muiren.FluidTrail` (`:573-574`) | `Single` | scatto · Dash | Ring · Projectile · Marker · None | — | lo scatto ha già la rotta |
| | `Hero.Muiren.MistVeil` (`:601-602`) | `Area` r1 | Environment | — | — | nessun beat in v0.1 |
| | `Hero.Muiren.FlowReaction` (`:642-644`) | `Single` | inerte (slot `None`) | Ring · Projectile · Marker · None | — | nessun effetto da segnalare |
| | `Hero.Muiren.TideGuard` (`:657-658`) | `Single` | scudo su sé · Prep | Ring · Projectile · Marker · None | **Pulse · None · None · None** | «guardia»: si chiude sulla sorgente; nessun colpo |
| Branth | `Hero.Branth.ImpactShot` (`:740`) | `Single` | attacco base · Blast | Ring · Projectile · Marker · None | — | R3 |
| | `Hero.Branth.KineticPanel` (`:760-761`) | `Single` | copertura · Prep | Ring · Projectile · Marker · None | — | il pannello che compare è già il segnale |
| | `Hero.Branth.Reconfigure` (`:779-782`) | `Single` | sposta copertura · Prep | Ring · Projectile · Marker · None | — | come `KineticPanel` |
| | `Hero.Branth.Ram` (`:793-794`) | `Single` | carica (Dash) + impatto | Ring · Projectile · Marker · None | **Ring · None · Marker · None** | «ariete»: colpo a contatto; un proiettile dall'adiacente mentirebbe. Volo invariato (R13) |
| | `Hero.Branth.Interposition` (`:809`) | — | reazione | — | — | non si attiva |
| | `Hero.Branth.MortarShot` (`:828-829`) | `Area` r1 | area · Blast | Ring · None · Marker · AreaPulse | — | «mortaio»: l'arco è #2825; l'onda sul centro è la lettura onesta |
| Ivrin | `Hero.Ivrin.PulseShot` (`:928`) | `Single` | attacco base · Blast | Ring · Projectile · Marker · None | — | R3 |
| | `Hero.Ivrin.InterceptShot` (`:944-947`) | `Single` | predittiva · Prep | Ring · Projectile · Marker · None | **Flash · None · None · None** | «intercetto»: un lampo di mira; il colpo predittivo non emette `Attack` |
| | `Hero.Ivrin.PassingBlade` (`:971-974`) | `Single` | scatto che attraversa (Dash) + colpo | Ring · Projectile · Marker · None | **Ring · None · Marker · None** | «lama di passaggio»: il colpo è il passaggio. Volo invariato (R13) |
| | `Hero.Ivrin.Deflection` (`:990`) | — | reazione | — | — | non si attiva |
| | `Hero.Ivrin.Feint` (`:1002-1003`) | `Single` | controllo · Blast, nessun effetto | Ring · Projectile · Marker · None | **Flash · None · None · None** | «finta»: un lampo senza seguito |
| | `Hero.Ivrin.PhaseGuard` (`:1011-1012`) | `Single` | scudo su sé · Prep | Ring · Projectile · Marker · None | **Pulse · None · None · None** | gemello di `TideGuard` (`:1005`) |
| *fuori roster* | `Action.Charge` (core di `Ram`) | `Single` | carica generica | Ring · Projectile · Marker · None | **Ring · None · Marker · None** | come `Ram`: `DerivedFromActionId` non è un livello del ripiego |
| *core, a contatto* | `Action.Push` (`URTCatalogLibrary`, `RangeCells` 1) | `Single` (default del campo) | controllo a contatto: spinge | Ring · Projectile · Marker · None | **Ring · None · Marker · None** | ➕ esecuzione, R15: a contatto, come `Ram`; volo invariato (R13) |
| *core, a contatto* | `Action.Root` (`URTCatalogLibrary`, `RangeCells` 1) | `Single` (default del campo) | controllo a contatto: radica | Ring · Projectile · Marker · None | **Ring · None · Marker · None** | ➕ esecuzione, R15: a contatto, come `Ram`; volo invariato (R13) |
| *core, a contatto* | `Action.Slow` (`URTCatalogLibrary`, `RangeCells` 1) | `Single` (default del campo) | controllo a contatto: rallenta | Ring · Projectile · Marker · None | **Ring · None · Marker · None** | ➕ esecuzione, R15: a contatto, come `Ram`; volo invariato (R13) |
| *core, a contatto* | `Action.Interrupt` (`URTCatalogLibrary`, `RangeCells` 1) | `Single` (default del campo) | controllo a contatto: interrompe | Ring · Projectile · Marker · None | **Ring · None · Marker · None** | ➕ esecuzione, R15: a contatto, come `Ram`; volo invariato (R13) |

➕ esecuzione. `Ruling` **R15** (controller, 2026-10-08, review del Task 2): la logica della riga `Ram` vale per le azioni
**core** che contano come attacco (`bCountsAsAttack`) ed entrano a **contatto**. Il criterio applicato è
`bCountsAsAttack && RangeCells == 1`: ⚠️ `RangeCells == 0` **non** è contatto, è *«la portata del portatore»*
(`FRTActionDef::RangeCells`), e contarlo toglierebbe il proiettile a `Action.BasicAttack`, cioè a ogni attacco base per
ripiego su `BaseActionId` (contro R3). Le righe restano esplicite e la misura si ripete: `Fx.DeclaredOverridesMatchTheProposal`
scorre `URTCatalogLibrary::GetCoreActionCatalog()` filtrato su `bCountsAsAttack` e scrive una riga «colpo core …» nel
log per ognuna (`Select-String "colpo core" <log>`). Esito misurato il 2026-10-08: `Action.Push`, `Action.Root`,
`Action.Slow`, `Action.Interrupt` a contatto; `Action.Pull` ha portata 2 (aggancia a distanza) e tiene il proiettile di
default; `Action.Charge` resta la riga d'autore. La forma dei core è `Single` perché `FRTActionDef` non porta `Shape` e
l'azione core vive in un `URTActionData` col default del campo. ⚠️ Nessuna unità porta oggi questi controlli (`Action.Push`, `Action.Root`, `Action.Slow`, `Action.Interrupt`) —
non sono in un kit né fra le generiche di `URTCatalogLibrary::GetGenericActionIds` —, quindi le righe hanno consumatori
solo nei test e la PIE ne giudica la regola su `Ram` (§5.2, scena (8)). Costo se sbagliato: una riga per azione.

➕ rev. (F1) 🔴 **`LinearDischarge` cambia a schermo una scena di `PIE-V01-TRACER`.** `Visual.Combat.WaterElectricCoordinated`
(`Scenarios/Visual/Combat/WaterElectricCoordinated.json:22-31`, misura del panel) fa colpire lo stesso bersaglio ad Aevik
con `LinearDischarge` e a Muiren con `PressureJet`. Da SP4 lo `Zigzag` di Aevik è **collineare** al getto e passa sopra
la cella di Muiren, proprio dove la scena (2) chiede *«una linea … ancorata a Muiren?»*. La regola di regressione zero
vale sugli **attacchi base del roster** (➕ rev2.: quelli con `BaseActionId == Action.BasicAttack`, lista dal catalogo; D5) e la pinna il test, non questa scena. La scena cambia, e si dichiara: il Task 7
del piano scrive **in coda** alla cella di `PIE-V01-TRACER` nel registro (`test-manuali-pie.md:1670`) la nota *«da
eseguire su `main` prima del merge di SP4, altrimenti la domanda della scena (2) si riformula: la scarica di Aevik viaggia
a zigzag»*. Costo: la voce del tracer resta senza verdetto, con una dipendenza dichiarata.
➕ esecuzione. **Chiusa dal fatto, non dalla dipendenza.** `PIE-V01-TRACER` è stata eseguita su `main` nella seduta `U68`
(2026-10-08), prima di #3578. La nota in coda alla sua cella dice quindi un'altra cosa: quel verdetto vale per il tracer
degli attacchi base, che #3578 non cambia; dopo il merge la scena (2) cambia a schermo, e la scarica di Aevik e le cue
le giudica `PIE-FX-ABILITA`. ⌫ *«la voce del tracer resta senza verdetto»*.

⚖️ Le varianti di tracer sugli attacchi base (zigzag, proiettile pieno, a tratti) restano fuori per D5. ⛔ `Slug` e
`Dashed` **non** si aggiungono a `ERTTracerStyle`: un valore che nessuna riga usa è un dato senza consumatore.

### 2.3 Il segnale di attivazione

**Dove.** Sulla cella della sorgente **nell'istante in cui agisce**, mai dalla posizione dell'attore. `Ruling` R5:
l'emettitore di `AbilityActivated` (`RTTurnManager.cpp:333-346`) scrive `Ev.Origin`, il campo che sull'impronta significa
già *«da dove il colpo è partito»* (`RTResolvedEvent.h:436-444`). ➕ rev. (F16) **Il soggetto si legge una volta**, come
`FRTLogSubject::GetFactCell` chiede (`Turn/RTCombatLog.h:82`, *«un solo accesso»*):

```cpp
const FRTLogSubject Soggetto = FRTLogSubject::Unit(Source);
Ev.Origin = Soggetto.GetFactCell();
Ev.SourceVerdict = FreezeVerdictFor(Soggetto);   // oggi :344 costruisce il soggetto dentro la chiamata
```

➕ rev. Cella e verdetto coincidono **per costruzione** (panel, incertezza 2): `Unit()` non dichiara una cella, quindi
`GetFactCell()` rende quella dell'Actor, ed è su quella che `FreezeVerdictFor` congela. Nel Dash l'emissione avviene sulle
celle di inizio fase: la cella da cui lo scatto parte. La nota `RTResolvedEvent.h:407-408` si aggiorna. ⌫ ➕ rev. (F5)
*È l'unico tocco al produttore: R9 (l'`AimCell` sul colpo) è caduta, §2.4.* Costo: un campo in più su un evento
`RTServerOnly` fuori dagli hash, confermato da `Determinism.FxFieldsStayOutOfHashes`.

**Quando.** Lo stesso istante della rivelazione, in tutte e tre le fasi:

| Fase | Rivelazione (esistente) | Finestra della cue |
|---|---|---|
| Prep, Dash | attivazione `k` della coda a `k·A` (`AttacksToShow`, `RTTurnManager.cpp:8857-8858`) | `[k·A, k·A + D_act)`, se `k < ActivationsShown` |
| Blast | elemento `k` al battito `2k` (`:8236-8238`) | `[k·A, k·A + D_act)`, se `BlastBeatsDone > 2k` |

Alpha = `(t − k·A) / D_act`, con `t = PlaybackPhaseElapsed`: funzione dell'orologio e del cursore, mai di un anim notify
né della durata della clip (`Ruling` R6: la clip vive in `Content/FabAsset/`, non versionato). Costo: cue e clip possono
finire in istanti diversi.

**Privacy — revisione esplicita del confine** (`CLAUDE.md` §7; ➕ rev. F9). Le code sono già filtrate a monte: Prep e
Dash in `BeginPlayback` (`RTTurnManager.cpp:7923-7926`), il Blast in `BuildBlastSequence` (`:7947-7949`). `Ruling` R7: la
funzione pura della cue **ricontrolla** `SourceVerdict` (costo: un confronto per attivazione). Il confine, canale per
canale:

| Canale | Cosa rivela | Verdetto (fail-closed) |
|---|---|---|
| cue d'attivazione | la sorgente nella sua cella | `SourceVerdict` |
| tracer | origine e impatto | `FromVerdict` ∧ `ImpactVerdict` (invariato, P1 del tracer) |
| `Marker` | la vittima nella sua cella | `ImpactVerdict` |
| `AreaPulse` | il centro dell'area | `FromVerdict` del colpo (§2.4) |
| `ConeSweep` | l'attaccante e la direzione | `FromVerdict` del colpo |
| **il ritmo** (volo) | che un colpo con un id ha forma di default `Single`/`Line` | nessuno: ➕ rev2. **stesso volo, a parità di indice nella sequenza** — l'indice dipende da chi guarda (`RTTurnManager.cpp:7952-7956`, §6) |

La riga del ritmo è la sola informazione che passa senza verdetto. È **la classe** che la spec del tracer accettava
(*«rivela che un attaccante, anche non visto, ha usato un attacco base `Single` o `Line`»*, tracer §2.1), allargata da
«attacco base» a «azione con un id»: per un attaccante non visto chi guarda può dedurre dal ritardo fra elemento e
numero la classe di forma del colpo, **mai** il suo override (R13), cioè ciò che distingue un'abilità da un'altra della
stessa forma. Il numero sulla vittima dice già che c'è stato un colpo. Costo se sbagliato: un bit di forma per colpo a
chi non vede la sorgente; chiuderlo vorrebbe dire un volo uguale per ogni colpo, cioè un ritmo che allunga ogni `Area`.
Il viewer è il giocatore locale (`:7856`), limite di v0.1 già dichiarato. La riga va nella tabella dei canali di
`conoscenza-parziale-visibile-spec.md` §1.3 (§2.8).

### 2.4 Le cue di colpo

**`Impact` — ogni `Attack`.** All'**arrivo**, battito `2k+1` (`AdvanceBlastSequence`, `RTTurnManager.cpp:8207-8230`),
finestra `[k·A + F_eff(k), k·A + F_eff(k) + D_imp(k))`, sulla `HitGeometry.Impact`, se il profilo dice `Marker` e
`ImpactVerdict` ammette chi guarda. ➕ rev. (F6) ⌫ *R8 è caduta*: **la prima vittima di un atto ha il `Marker` come le
altre**, `Line` e `Area` comprese; nessuna regola fuori dal profilo. Per un colpo senza volo `F_eff = 0` e l'arrivo
coincide col lancio, come oggi. Su una `Line` il `Marker` è ciò che si aggiunge all'impronta e al getto.

**`Footprint` — una volta per impronta.** ➕ rev. (F5) ⌫ *R9 è caduta: la prima stesura scriveva l'`AimCell` sul colpo dal
produttore ed estraeva un helper da `ResolveImpactOrigin`, per una forma senza contenuto.* La geometria viene
dall'**`AttackFootprint` dello stesso atto**, che porta già `Origin` e `AimCell` (`RTTurnManager.cpp:5605-5611`) e nella
sequenza precede i suoi colpi. **Associazione**, in `BeginPlayback`, accanto ai voli (`:7961-7975`), funzione pura della
sequenza: si percorrono gli elementi del Blast in ordine; un `AttackFootprint` apre la chiave
`(SourceStableUnitId, ActionId)` del suo atto — la stessa chiave che raggruppa la sequenza (spec del momento §2.4) — e il
**primo** `Attack` successivo con la stessa chiave la consuma e porta la cue. Risultato:
`PlaybackBlastFootprintFx[k]` = indice di timeline dell'impronta, o `INDEX_NONE`.

➕ rev2. `Ruling` R14 — **due impronte con la stessa chiave.** L'unico `Footprints.Add` è `RTHexCombatLibrary.cpp:620`,
nel ciclo per intento (`:349`): un intento ha al più un'impronta. Se due `AttackFootprint` con la stessa
`(SourceStableUnitId, ActionId)` arrivassero nello stesso Blast (due intenti con la stessa chiave), **l'ultima aperta
sostituisce** la precedente non ancora consumata, e un log a `Verbose` lo dice; nessun `ensure`. Pinnato da
`Playback.SecondFootprintReplacesTheFirst`, su eventi costruiti dal test. Costo: nessuno in v0.1 (un intento per
abilità per turno); se un giorno un'unità avrà due intenti della stessa azione, il primo atto perde la sua cue d'impronta.

| Cue | Celle (dall'impronta) | Momento | Privacy |
|---|---|---|---|
| `AreaPulse` | centro = `AimCell` | arrivo del colpo che la consuma | `FromVerdict` di quel colpo |
| `ConeSweep` | `At = Origin`, `Toward = AimCell`, braccio lungo `L = \|Origin → AimCell\|` | idem | `FromVerdict` di quel colpo |

- ➕ rev. **Il verdetto è del colpo, le celle dell'impronta, e coincidono per costruzione**: per un'`Area`
  `HitGeometry.From` **è** `Footprint->AimCell` (`RTTurnManager.cpp:2578`); per le altre forme `From` è la cella
  dell'attaccante (`:2586`), di cui l'impronta porta l'`Origin`. L'impronta in sé non ha verdetto: è terreno, disegnata
  senza filtro (spec del tracer §2.1, P1).
- ➕ rev. (incertezza 1, sciolta dal panel) **`FromVerdict` di un'`Area`** è congelato su `UnitAt(Attaccante, centro)`
  (`RTTurnManager.cpp:6390`): è **aperto alla squadra dell'attaccante** sempre (ramo alleato di `ClassifyTarget`,
  `Perception/RTTeamKnowledge.cpp:191-196`) e, per le altre squadre, **solo se vedono la cella del centro**
  (`AwarenessOfUnit`, `:145-150`); un contatto soltanto non basta. Il pulse si vede quindi se chi guarda vede il centro,
  che veda o no chi l'ha lanciato.
- **Degrado** (nessun errore, nessun `ensure`): impronta assente dalla sequenza, chiave con sorgente `0` (D-063) o già
  consumata → nessuna cue d'impronta; i `Marker` dei colpi restano. Un'area **senza vittime** non ha `Attack`, quindi non
  ha pulse: resta l'impronta (§6).
- **La scala del pulse non è l'estensione dell'area**: la dice l'impronta; il pulse dice il centro e l'istante.
- ➕ rev. (F4) **`ConeSweep` senza la cella della vittima**: il braccio è lungo quanto l'asse dichiarato
  `Origin → AimCell`, il raggio del cono come l'impronta lo dichiara. Nessun campo nuovo in `FRTPlaybackCue`. `Cone`
  resta nel profilo (D1, D4) **senza contenuto nel catalogo** (§1): si prova headless con un evento costruito dal test.

**Come viaggiano verso la mappa.** `Ruling` R10: **un canale nuovo, uno solo**, accanto a quello del tracer, che
**resta com'è** (`HexMapActor.PlaybackTracerIsItsOwnChannel`, `Tests/RTHexMapActorTests.cpp:1846`). Costo: due consegne
per tick nel Blast. ➕ rev. (panel, Fowler c) Il record ha **un** campo di tipo, non un'unione con un invariante:

```cpp
// Map/RTPlaybackTracer.h
UENUM() enum class ERTPlaybackCueKind : uint8 { Ring, Pulse, Flash, Marker, AreaPulse, ConeSweep };

USTRUCT()
struct FRTPlaybackCue
{
	GENERATED_BODY()
	UPROPERTY() ERTPlaybackCueKind Kind = ERTPlaybackCueKind::Ring;
	UPROPERTY() FRTCellId At;      // sorgente | vittima | centro | origine del ventaglio
	UPROPERTY() FRTCellId Toward;  // solo ConeSweep: AimCell dell'impronta
	UPROPERTY() float Alpha = 0.f;
};

// ARTHexMapActor
void SetPlaybackCues(const TArray<FRTPlaybackCue>& Cues);   // SOSTITUISCE in blocco, come SetPlaybackTracers
void ClearPlaybackCues();
const TArray<FRTPlaybackCue>& GetPlaybackCues() const;       // oracolo headless
```

Il disegno sta subito dopo quello del tracer (`RTHexMapActor.cpp:1659-1679`), con `DisegnaLineaAnteprima`
(`:1422-1435`) in `SDPG_Foreground`; il canale entra in `HasAnythingToDraw` (`:1093-1108`). La geometria è **pura** in
`URTPlaybackLibrary`: `CueSegments(Kind, …, Alpha)` per i sei tipi e ➕ rev. (F11) `TracerPolyline` **solo per
`Zigzag`**; `Projectile` e `Jet` restano su `TracerSegment` (`RTPlaybackLibrary.cpp:56-77`), invariata. ➕ rev. (F12)
I vertici dello `Zigzag` sono funzione di `(From, To)` soltanto; `Alpha` taglia la polilinea, quindi la linea a `α = 0.3`
è un **prefisso** di quella a `α = 0.6`: cresce, non tremola. Niente `FMath::Rand`, niente orologio di parete.

**Chi consegna.** `ARTTurnManager::PushPlaybackCues()`, gemella di `PushPlaybackTracers` (`RTTurnManager.cpp:8324-8366`),
con lo stesso flag «consegna solo se c'è qualcosa da dire» (`bPlaybackCueChannelFull`):

- Blast: nello stesso `ON_SCOPE_EXIT` di `:8874`, dopo `PushPlaybackTracers`.
- Prep e Dash: un `ON_SCOPE_EXIT` nel ramo di `:8853-8862`. ⚠️ Oggi quel ramo non consegna nulla alla mappa: è il primo.
- Il canale si spegne alla finalizzazione di **ogni** fase (accanto a `ClearPlaybackTracers`, `:8963-8967`, e nei rami
  Prep/Dash di `:8932-8940`) e in `FinishPlayback` (blocco `:9186-9199`), che è anche il percorso di `SkipPlayback`
  (`:9219`).

I dati per elemento si calcolano in `BeginPlayback` accanto ai voli: `PlaybackBlastFootprintFx` (sopra). Si ricalcolano
anche estendendo con `bPreserveClock`, come i voli: sono funzione pura degli eventi, e il prefisso congelato porta gli
stessi. ⚠️ Un'impronta già consumata nel prefisso resta consumata: la ricostruzione percorre l'intera sequenza.

### 2.5 D-278

| Voce | Oggi | Dopo |
|---|---|---|
| `Attack` (`RTPresentationBinding.cpp:36-42`) | `PlayAttackMontage`, `PlayHitMontage`, `ShowDamageToken`, `PulseHealthBar`, `SetPlaybackTracers` | + `SetPlaybackCues`; il commento `:36` («solo per gli attacchi base `Single`/`Line`») diventa «secondo il profilo FX» |
| `AbilityActivated` (`:265-272`) | `PlayCastMontage`; «ring/pulse resta di #2454» | + `SetPlaybackCues`; il commento dice che il ring è il profilo d'attivazione di SP4, grammatica di #2454 |

➕ rev. `AttackFootprint` **non** guadagna una cue nella tabella: la cue d'impronta la consegna il colpo che la consuma, ed
è la voce `Attack` a dichiarare `SetPlaybackCues`. ⚠️ **Il gate non vede una cue mai chiamata**: `FindMissingBindings`
(`:292`) conta i nomi non vuoti (`:269`). Perciò i test di §5.1 asseriscono **quale** cue, **su quale cella** e **a quale
battito**, leggendo `GetPlaybackCues()` dopo ogni tick. ⛔ Nessun tipo di evento nuovo, nessuna `UFUNCTION`/`UPROPERTY`
su `ARTUnit` (`Unit.BlueprintSurfaceIsCensused` invariato), `Play*Montage` invariati, nessuna autorità in Blueprint.

### 2.6 Seek, pausa, passo, fermata

La politica dei one-shot che #2453 chiede di **scrivere**: *una cue è funzione di `(PlaybackPhaseElapsed, cursore)`* —
`BlastBeatsDone` nel Blast, `ActivationsShown` in Prep e Dash — e di nient'altro.

| Gesto | Effetto |
|---|---|
| Un tick lungo (velocità ×4, un fotogramma lento) | lo stesso insieme di cue, con lo stesso `Alpha`, di molti tick corti allo stesso `t`: **lineare fino a X == salto a X** per lo stato disegnato |
| Pausa (`bPlaybackPaused`) | il tick esce prima del ramo: la cue resta ferma dove l'ultima consegna l'ha lasciata |
| `Step` (`StepMicroStep`, `:8493`) | nel Blast senza spinte non avanza (spec del tracer §2.3): la cue resta ferma |
| `Next Action` (`RequestPlaybackStopAt`, `:8533`) | la fermata cade dopo l'arrivo (`:8224-8228`): la cue d'arrivo è nella sua finestra se il tick non l'ha già oltrepassata (§6) |
| `SkipPlayback` / `FinishPlayback` | **nessuna cue rigiocata**; il canale si spegne |
| Rete di finalizzazione (`:8941-8959`, `:8932-8940`) | rivela ciò che resta; le finestre sono già passate, quindi non disegnano — la rete è senza casi |
| Estensione con `bPreserveClock` | cursore e orologio sopravvivono: nessuna cue ripetuta |

### 2.7 Ability Lab

Nessun codice di UI nuovo: il banco (#3532) lancia la PIE con una fixture e il profilo si giudica a schermo. `Ruling`
R11: **uno scenario nuovo**, `Scenarios/Visual/Ability/FxProfile.json` (`Visual.Ability.FxProfile`), e `CastBeat.json`
**non si tocca**: è la fixture di `PIE-CAST-BEAT` (`test-manuali-pie.md:1944`), voce ➕ rev. (F19) ancora senza verdetto.
➕ rev. (F1, panel c) Lo `Zigzag` di `LinearDischarge` si guarda in `Visual.Combat.WaterElectricCoordinated`, che lo
contiene già: `FxProfile.json` **non** lo ripete (un consumatore in meno da mantenere). Contenuto: Branth con
`Hero.Branth.MortarShot` (`Area`) centrato in modo da colpire **due** nemici adiacenti (un `AreaPulse`, due `Marker`);
Aevik con `Hero.Aevik.Overload` (`Flash` e `AreaPulse`). ⛔ Nessuna `Cone`: nessuna azione la dichiara. Celle e `expect`
si fissano **misurando** il run headless dello scenario (la fixture vuole `units` **e** `expect`): `NOT RUN` qui.

### 2.8 Gate e documenti

- **Automation**: §5.1, sotto i prefissi esistenti `RefactorTactics.Playback.*`, `Privacy.*`, `Presentation.*`,
  `HexMapActor.*`, `Preview.*`, `Determinism.*`, più `RefactorTactics.Fx.*` per le funzioni pure del profilo.
- ➕ rev. (F8) **Prima** di dichiarare un gate «verde senza modifiche», il piano misura quali test di mondo toccano il
  ritmo del Blast: `git grep -ln "EnsureDefaultAbilities\|AttackBeatTraceForTest\|NumPlaybackTracers" -- Source/RefactorTactics/Tests`,
  ➕ rev2. **e** i nomi delle abilità il cui volo cambia:
  `git grep -n "Hero.Branth.Ram\|Action.Charge\|Hero.Aevik.LinearDischarge\|Hero.Ivrin.PassingBlade" -- Source/RefactorTactics/Tests`
  (alla re-review: `RTPlaybackActivationTests.cpp:363`, `:497`, `:637`, `:650` e `RTPlaybackStopPredicateTests.cpp:975`;
  ogni occorrenza si rilegge per sapere se asserisce un istante del Blast).
  Con R12 le unità legacy non cambiano ritmo; i colpi non base con un id e forma `Single`/`Line` sì.
- **Radar**: tutti i controlli di `tools/radar/` (l'elenco è `ls tools/radar/*.test.ts`); `doc-coherence` A1 misurato
  **prima** e **dopo** il registro PIE, come le voci sorelle (`test-manuali-pie.md:94-102`). ➕ rev. (F19) Nessun glifo
  di stato accanto a un nome di voce PIE nel testo che finirà nel corpo della issue (A3, `tools/radar/doc-coherence.ts:209`).
- **Capability map** (`docs/technical/architecture/capability-map.yaml`):
  - `RT-CAP-VFX` (`:1707-1735`): `name` «VFX (line batcher in v0.1; Niagara fuori, D-124)»; `status.implementation`
    `partial`, `validation` `partial`, `presentation` `missing` finché `PIE-FX-ABILITA` non ha un verdetto;
    `implementation.files` = `Map/RTPlaybackTracer.h`, `Turn/RTPresentationBinding.{h,cpp}`,
    `Turn/RTPlaybackLibrary.{h,cpp}`, `Map/RTHexMapActor.cpp`; `issues.open` + `2454` e la issue di SP4;
    `tests.automation` i prefissi qui sopra; `verification.pie` `[PIE-V01-TRACER, PIE-FX-ABILITA]`. `owner.epic` **resta
    `null`**: assegnarla è una decisione di ownership (il rischio `OWNER GAP` si restringe, non si chiude).
  - `RT-CAP-COMBAT-FEEDBACK` (`:1610-1641`): `issues.open` + `2454` e la issue di SP4; `verification.pie` le stesse due
    voci.
- **Registro PIE e sedute**: voce `PIE-FX-ABILITA` in `docs/technical/test-manuali-pie.md`, seduta **`U71`** in
  `docs/roadmap/editor-sessions.yaml` (l'ultima è `U70`, `:5138`), scritte **con** la feature; ➕ rev. (F1) nel Task 7 la
  nota **in coda** alla cella di `PIE-V01-TRACER` (§2.2).
- **Conoscenza parziale**: la tabella dei canali di `docs/technical/systems/conoscenza-parziale-visibile-spec.md` §1.3
  riceve le righe *«cue di attivazione: solo con sorgente osservata; cue di colpo: col verdetto della cella che disegna»*
  e ➕ rev. *«ritmo del Blast: rivela la classe di forma di un colpo con un id, mai l'override»* (`NOT RUN`: il piano
  verifica che la tabella sia ancora in quella forma).

### 2.9 Cosa questa spec supera

- ⌫ Spec del tracer §2.1, condizione 1 (*«`ActionId == Action.BasicAttack` oppure …»*) e il suo ⚠️ *«idoneità
  provvisoria»*: il volo è la forma di default di un'azione con un id (R12, R13). Le condizioni 3 e 4 restano.
- ⌫ Spec del tracer §2.1, condizione 2 (*«`Shape ∈ { Single, Line }`»*): resta vera **come forma di default**; un override
  può togliere il disegno, mai il volo, e non può aggiungerli.
- ⌫ ➕ rev. (F9) Spec del tracer §2.1, *«Non è privo di informazione — rivela che un attaccante … ha usato un attacco base
  `Single` o `Line`»*: allargato a «un'azione con un id», con la revisione del confine di §2.3.
- ⌫ Spec del tracer §2.2, *«`Single` = proiettile, `Line` = getto»* come tabella chiusa: è la riga di default di §2.2.
- ⌫ Spec del tracer §7, *«Per eroe …»* e *«Area e Cone, e ogni azione che non sia attacco base»*; §8, ultimo punto:
  consegnati qui, come cue d'impronta e non come tracer.
- ⌫ Commenti nel codice: `RTPlaybackLibrary.h:277-282` («PROVVISORIA»), `RTTurnManager.cpp:6374-6377` e
  `RTResolvedEvent.h:196-197` («irrilevante finché `Area` non è idonea»: da oggi `FromVerdict` del centro **serve** al
  pulse), `RTPresentationBinding.cpp:36` e `:265-267`.
- ⌫ Spec del momento, D2: *«Il segnale visivo di attivazione (ring, pulse) resta di #2454»* — lo consegna SP4.
- **Non** superato: la spec del tracer §2.3 (tempo, cursore unico, confine `Next Action` all'arrivo), §3 (dati: ➕ rev.
  con R9 caduta nessun campo nuovo sul colpo, e il divieto di riusare `AimCell`/`Origin` resta intatto), §5 e P1.

---

## 3. Fuori scope

- `ReactionResolved` e `Defeated`: grammatica di #2454 (D4).
- Arco balistico ([#2825](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2825)): tracer e pulse sono retti.
- `ArcHit` ([#3293](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3293)): voce D-278 in attesa, fuori
  dalla sequenza del Blast.
- Suono: `RT-CAP-AUDIO` non ha implementazione.
- Asset Niagara o Cascade (D-124), materiali, decal, mesh: nessun `.uasset`, nessun authoring nel clone principale.
- I profili come dati esterni (DataAsset, JSON): la migrazione di D-278 resta possibile e non si fa.
- Un `CastShowSeconds` o una durata per abilità: le durate sono ritmo (R2).
- Le varianti di tracer sugli attacchi base (D5).
- Un'azione `Cone` nel catalogo: non si crea contenuto per far vedere una cue.
- ➕ rev. FX per gli attacchi legacy senza id (R12).

---

## 4. Errori e degrado

| Caso | Comportamento |
|---|---|
| Azione senza voce in tabella | Default della sua `Shape`. |
| ➕ rev. `ActionId` **e** `BaseActionId` vuoti (legacy) | Nessun profilo: nessun volo, nessun tracer, nessuna cue (R12). |
| Uno solo dei due id vuoto | Il livello si salta; mai derivato. |
| `HitGeometry.bResolved = false` | Nessun tracer, nessun `Marker`, nessuna cue d'impronta, `F_eff = 0`: il ritmo di oggi. |
| ➕ rev. Override `Tracer = None` su una forma con volo | Stesso volo, nessun disegno (R13). |
| Sorgente fuori da `SourceVerdict` | Nessuna cue d'attivazione (la coda non la porta, e la funzione pura la rifiuta: R7). |
| Vittima fuori da `ImpactVerdict` | Nessun `Marker`. |
| Colpo fuori da `FromVerdict` | Nessun tracer, nessuna cue d'impronta; l'impronta resta, senza filtro com'è oggi (terreno). |
| ➕ rev. Impronta assente, sorgente `0` o già consumata | Nessuna cue d'impronta, nessun errore; i `Marker` restano. |
| Area senza vittime | Nessun `Attack`, nessun pulse: resta l'impronta. |
| `AttackShowSeconds <= 0` | `D_act = D_imp = 0`: nessuna cue, tutto in un frame, come il volo. |
| `ActivationCueSeconds`/`ImpactCueSeconds` oltre il tetto | Tagliate (§2.1). |
| Tick più lungo della finestra di una cue | La cue può non comparire in nessun fotogramma (§6). |
| Clip assenti (`FabAsset` non presente) | Le cue funzionano: non dipendono da alcuna animazione. |
| Server dedicato | `DisegnaLineaAnteprima` non disegna (`RTHexMapActor.cpp:1425-1428`). |
| Fermata, `SkipPlayback`, fine fase | Canale spento; nessuna cue rigiocata (§2.6). |

---

## 5. Verifica

### 5.1 Automation, headless

| Test | Asserisce |
|---|---|
| `Fx.DefaultProfileFollowsShape` | `DefaultFxProfileFor` dà la tabella dei default di §2.2 per ogni valore di `ERTAbilityShape`. Mutazione (1). |
| `Fx.ProfileFallsBackActionThenBaseThenShape` | Su una tabella **iniettata** in `FxProfileForIn` (non quella giudicata, come R11 della spec della clip): `ActionId` vince; senza, `BaseActionId`; senza, la forma; `BaseActionId = NAME_None` non indovina la generica. Mutazioni (2), (3). |
| ➕ rev. `Fx.NoActionNoProfile` (F8) | `ActionId` e `BaseActionId` vuoti, forma `Single` e `Area`: profilo tutto `None`; `IsTracerEligible` falso anche con geometria risolta. Mutazioni (18) e (22): ➕ rev2. due asserti distinti, perché `IsTracerEligible` ha la **propria** clausola R12 e legge `DefaultFxProfileFor`, non `FxProfileFor`. |
| `Fx.DeclaredOverridesMatchTheProposal` | Per ogni abilità del roster (lista attesa = **funzione** di `URTHeroCatalogLibrary`) il profilo è quello della tabella di §2.2, ricopiata nel test come seconda copia dichiarata; ➕ rev. nessuna riga dà `Tracer != None` a una forma che non ne ha di default (R13). Mutazione (4). ➕ esecuzione (R15): scorre anche il catalogo core filtrato su `bCountsAsAttack` — a contatto (`RangeCells == 1`) la riga gemella e nessun proiettile, a distanza senza riga il tracer di default — e scrive «colpo core …» nel log per ognuna; validato togliendo la riga `Action.Root` (cade «🔴 Action.Root (core, a contatto): nessun proiettile»). |
| ➕ rev. `Fx.DeclaredOverridesHaveNoDuplicateKeys` (F17) | `DeclaredFxOverrideRows()` non ha due righe con la stessa chiave: in un `TMap` letterale una duplicata vincerebbe in silenzio. |
| ➕ rev. `Fx.CueStylesDifferByGeometry` (F7) | Per ogni coppia di stili distinti fra `Ring`, `Pulse`, `Flash`, `Marker`, `AreaPulse`, `ConeSweep`, a `α = 0.5` su una cella di prova, `CueSegments` dà insiemi che differiscono per **numero** di segmenti, oppure di almeno `0.1 s` nella distanza massima o minima dall'ancora, oppure nell'estensione verticale. Mutazione (20). |
| ➕ rev. `Playback.BasicAttackTracersEqualShapeDefault` (F10) | Per ogni azione del roster con `BaseActionId == Action.BasicAttack` (lista **funzione** del catalogo: `MakeHeroBasicAttack` la scrive, `Ability/RTHeroCatalogLibrary.cpp:165-168`), `FxProfileFor(...).Tracer == DefaultFxProfileFor(Shape).Tracer` e il volo è quello di oggi. È R3/D5 come **regola**. Mutazione (6). |
| `Playback.TracerStyleFollowsShapeForBasicAttack` *(esistente, `Tests/RTPlaybackLibraryTests.cpp:421-455`)* | ➕ rev. **Cambia, dichiarato.** L'evento di prova è `Hero.Ivrin.PulseShot` (`:407-408`), senza override, e quegli asserti restano verdi. L'asserto su `PassingBlade` (`:436-438`, `IsTracerEligible == false`) **cade** con R13: `PassingBlade` è idonea al volo (forma `Single`) e il suo stile è `None` per override. Il piano lo riscrive su `TracerStyleFor(...) == None` e lo dichiara nella PR. ⚠️ Da SP4 quell'asserto regge su una riga **giudicata** della tabella (panel). ➕ rev2. Perciò gli si affianca la **variante a tabella iniettata**, come R11 della spec della clip: con `FxProfileForIn` e una tabella di prova che dà `Tracer = None` a un'azione `Single` sintetica, il volo resta quello della forma e lo stile è `None`. Il meccanismo è provato senza dipendere da una riga giudicata. |
| `Playback.TracerFollowsTheProfile` | `Hero.Aevik.LinearDischarge` (`Line`, non base) è idonea e dà `Zigzag`; `Hero.Branth.Ram` e `Action.Charge` sono idonee al volo e danno `None`. Mutazione (5). |
| ➕ rev. `Privacy.FlightDependsOnShapeNotOverride` (F9) | `PlaybackBlastFlights` di un `Ram` e di un `ImpactShot` con la stessa `A`: uguali. Mutazione (17). |
| `Playback.BasicAttackTracerIsUnchanged` | Sulla fixture di `RTPlaybackActivationTests.cpp` (`ImpactShot`), a metà volo, `GetPlaybackTracers()` ha **un** tracer con `From`, `To`, `Style = Projectile` e `Alpha` uguali a quelli delle formule di oggi. ➕ esecuzione: pin d'istanza, nessuna mutazione dell'elenco qui sotto lo fa cadere; lo valida la mutazione del fix del Task 2, `T.Style = ERTTracerStyle::Jet` in `ARTTurnManager::PushPlaybackTracers` (cade «🔴 Style = Projectile», e con lui `Playback.TracerIsInFlightBetweenLaunchAndArrival`). |
| ➕ rev. `Playback.TracerZigzagGrowsAsAPrefix` (F12) | `TracerPolyline(Zigzag, …)` a `α = 0.3` è un prefisso di quella a `α = 0.6`; due chiamate uguali danno gli stessi punti; ancorata all'origine; nessun punto oltre l'impatto. Mutazione (19). |
| `Playback.ActivationCueAtTheSourceCell` | Fixture del momento: dopo il tick della rivelazione, `GetPlaybackCues()` ha `Pulse` su `Ev.Origin` dello Scudo in Prep e `Ring` sul Tiratore nel Blast; dopo `D_act`, nessuna. ➕ rev. (F15) **Premessa asserita prima**: per lo Scudo `Ev.Origin != Src->Cell` a fine risoluzione; se la fixture non lo sposta, il piano la estende finché la premessa vale. Mutazione (7). |
| `Privacy.ActivationCueNeedsTheSource` | Pura: `SourceVerdict` che esclude la squadra → nessuna cue; che la include → lo stile del profilo. Mutazione (8). |
| `Privacy.HiddenSourceDeliversNoActivationCue` | Fixture di `Playback.HiddenSourceHasNoActivationBeat` (`RTPlaybackActivationTests.cpp:182`), viewer `7`: nessuna cue d'attivazione su nessun tick; controllo positivo col viewer `0`. |
| `Playback.ImpactCueComesAtTheArrival` | Al battito `2k` il canale non ha il `Marker`; al `2k+1` sì, sulla `HitGeometry.Impact`. ➕ rev2. **Premessa asserita prima**: il colpo ha `F_eff > 0`, quindi `2k` e `2k+1` cadono in tick diversi. Mutazione (9). |
| ➕ rev. `Playback.EveryAttackGetsItsProfileMarker` (F6) | Un'area con due vittime e una `Line` con due vittime: **ogni** `Attack`, il primo compreso, ha il suo `Marker` sulla sua vittima. Mutazione (12). |
| `Privacy.ImpactMarkerNeedsTheVictim` | Pura: `ImpactVerdict` che esclude → nessun `Marker`. Mutazione (10). |
| ➕ rev. `Fx.AreaPulseIsOnTheFootprintAim` (F5) | Un atto `Area` costruito dal test (impronta, poi due `Attack`): l'`AreaPulse` è sull'`AimCell` dell'impronta, non su un `Impact`. ➕ rev2. **Premessa asserita prima**: l'`AimCell` è diversa da **ogni** `Impact` dell'atto. Mutazione (11). |
| ➕ rev. `Playback.FootprintCueOncePerFootprint` (F5, F6) | Stesso atto: **un** `AreaPulse`, portato dal primo `Attack` e ➕ rev2. contato su **tutti i tick** del Blast, non su uno; con l'impronta tolta, nessun pulse, nessun errore, i `Marker` restano. Mutazione (21). |
| ➕ rev2. `Playback.SecondFootprintReplacesTheFirst` (R14) | Due `AttackFootprint` costruiti dal test, con la stessa chiave, prima del colpo: la cue d'impronta usa le celle della **seconda**. |
| `Fx.ConeSweepAxisIsTheAim` | Un atto `Cone` costruito dal test: lo sweep è ancorato all'`Origin` dell'impronta, simmetrico attorno a `Origin → AimCell`, ➕ rev. (F4) con braccio lungo `\|Origin → AimCell\|`. |
| `Playback.FxCuesNeverOverlapInTheBlast` | ➕ rev. (panel, Wiegers) Sulla griglia dichiarata `A ∈ {0.1, 0.5, 1.0}`, `F ∈ {0, A/4, A/2, A}`, `ActivationCueSeconds` e `ImpactCueSeconds ∈ {0, A/2, 2A}`, con sequenze miste: le finestre di elementi diversi sono disgiunte e ordinate, l'ultima finisce entro `N·A`. Mutazione (13). |
| `Playback.FxCuesAreAFunctionOfTheClock` | Lo stesso turno con un tick unico fino a `t` e con tick da `1/60 s` fino a `t`, con `t` dentro la finestra di una cue: stesse cue, `Alpha` uguale entro `1e-3`. Mutazione (14). |
| `HexMapActor.PlaybackCueIsItsOwnChannel` | `SetPlaybackCues` **sostituisce**, `ClearPlaybackCues` spegne, non tocca tracer, impronta né anteprima. Mutazione (15). |
| `Preview.FxCuesDrawWithDebugDrawingOff` | Col debug spento ogni cue scrive le sue linee nel batcher `Foreground` col colore `Attack` (gemello di `Preview.TracerDrawsWithDebugDrawingOff`, `Tests/RTPreviewLineBatcherTests.cpp:143-146`). |
| ➕ rev. `Playback.CueChannelClearsOnSkip` (F2) | `SkipPlayback` chiamato a metà della finestra di un `Marker` (premessa asserita: canale **non** vuoto un tick prima): dopo, canale vuoto; e a fine Prep, Dash e Blast il canale è vuoto. Mutazione (16). |
| `Determinism.FxFieldsStayOutOfHashes` | Gemello di `Determinism.HitGeometryStaysOutOfHashes` (`RTAttackTracerTests.cpp:214`). ➕ rev. (F20) Gancio `bool bSkipFxFieldsForTest`, semplice e non `UPROPERTY`, accanto a `bSkipHitGeometryForTest` (`RTTurnManager.h:636-640`): lascia vuoto `Origin` di `AbilityActivated`, l'unico campo nuovo. Con e senza, `StateHash` e `HashTurnLog` dello stesso turno sono identici; controllo positivo che il gancio agisca. |
| `Presentation.FxCueIsDeclaredForAttackAndActivation` | Le voci `Attack` e `AbilityActivated` dichiarano `SetPlaybackCues`. |
| ➕ esecuzione `Privacy.AreaPulseNeedsTheCenter` | Dal piano: pura, `FromVerdict` del colpo che esclude la squadra → nessun `AreaPulse`; controllo positivo con chi vede il centro. Mutazione (P3) del piano. |
| ➕ esecuzione `Playback.UnresolvedGeometryHasNoFxButPlaysTheClip` | Dal piano: con `HitGeometry.bResolved = false` nessun `Marker`, anche coi verdetti aperti (parte pura), e la clip suona (parte di mondo). Mutazione (P4) del piano. |

🔴 **Controlli di mutazione** (ognuno visto rosso sull'asserto nominato, poi ripristinato; il codice nuovo si nomina per
funzione, quello esistente per `file:riga`):

1. In `DefaultFxProfileFor`, la riga `Line` dà `Tracer = Projectile` → cade `Fx.DefaultProfileFollowsShape` («`Line` → `Jet`»), ➕ rev2. e anche `Playback.TracerStyleFollowsShapeForBasicAttack` (`Tests/RTPlaybackLibraryTests.cpp:428-429`, «Line -> getto»).
2. In `FxProfileForIn`, la forma letta prima di `ActionId` → cade `Fx.ProfileFallsBackActionThenBaseThenShape` («`ActionId` vince»). ➕ esecuzione: spostare il `return` lascia codice irraggiungibile, e `C4702` qui è un errore; la forma eseguita è una guardia sempre vera subito dopo la guardia R12, `if (Overrides.Num() >= 0) { return DefaultFxProfileFor(Shape); }`. Cadono **tre** test, perché leggono la forma al posto della riga: `Fx.ProfileFallsBackActionThenBaseThenShape`, `Fx.BaseActionOverrideWinsOverShapeDefault` e `Fx.DeclaredOverridesMatchTheProposal`.
3. In `FxProfileForIn`, il livello `BaseActionId` saltato → cade lo stesso test («senza `ActionId`, vince `BaseActionId`»).
4. Una riga tolta da `DeclaredFxOverrideRows` → cade `Fx.DeclaredOverridesMatchTheProposal` (la riga attesa dalla copia).
5. `IsTracerEligible` riportata al corpo di `RTPlaybackLibrary.cpp:130-137` → cade `Playback.TracerFollowsTheProfile` («`LinearDischarge` idonea»), ➕ rev2. e anche `Privacy.FlightDependsOnShapeNotOverride` (`Ram` senza volo, `ImpactShot` col volo).
6. ➕ rev. (F10, F15) Override `Zigzag` su `Hero.Muiren.PressureJet` in `DeclaredFxOverrideRows` **e** nella copia del test di (4) → cade `Playback.BasicAttackTracersEqualShapeDefault` («tracer == default della forma»), mentre (4) resta verde.
7. ➕ rev. (F15) In `PushPlaybackCues`, la cella d'attivazione presa da `Src->Cell` invece che da `Ev.Origin`, **dopo** la premessa `Ev.Origin != Src->Cell` → cade `Playback.ActivationCueAtTheSourceCell` («`Pulse` su `Ev.Origin`»).
8. Nella funzione pura della cue d'attivazione, il controllo di `SourceVerdict` tolto → cade `Privacy.ActivationCueNeedsTheSource`.
9. In `PushPlaybackCues`, la finestra del `Marker` aperta al battito `2k` invece che al `2k+1` → cade `Playback.ImpactCueComesAtTheArrival` («al `2k` nessun `Marker`»). ➕ rev2. **Premessa**: `F_eff > 0`; con volo nullo `2k` e `2k+1` cadono nello stesso tick (`RTTurnManager.cpp:8207-8262`) e la mutante sopravvive. ➕ esecuzione: per la stessa ragione cadono solo gli elementi che volano (la `Line`), e `Fx.AreaPulseIsOnTheFootprintAim` resta verde (i colpi `Area` hanno volo nullo).
10. Nella funzione pura del `Marker`, `ImpactVerdict` ignorato → cade `Privacy.ImpactMarkerNeedsTheVictim`.
11. ➕ rev. L'`AreaPulse` centrato su `HitGeometry.Impact` del colpo invece che sull'`AimCell` dell'impronta → cade `Fx.AreaPulseIsOnTheFootprintAim`. ➕ rev2. **Premessa**: `AimCell` diversa da ogni `Impact` dell'atto.
12. ➕ rev. (F6) Il `Marker` saltato sul primo `Attack` di un atto (la R8 caduta) → cade `Playback.EveryAttackGetsItsProfileMarker` («la prima vittima ha il `Marker`»), ➕ rev2. e anche ogni asserto sul `Marker` di un atto con **una** vittima (`Playback.ImpactCueComesAtTheArrival`).
13. I tetti di `D_act`/`D_imp` tolti (i `Min` rimossi) → cade `Playback.FxCuesNeverOverlapInTheBlast` (finestre disgiunte con `ImpactCueSeconds = 2A`).
14. ➕ rev. (F3) In `PushPlaybackCues`, `Alpha` letta da un accumulatore `CueElapsed` **azzerato nel tick in cui l'elemento si rivela** e aumentato di `Dt` solo nei tick successivi, invece che da `PlaybackPhaseElapsed − k·A` → cade `Playback.FxCuesAreAFunctionOfTheClock`: col tick unico l'accumulatore vale `0`, quindi `Alpha = 0`; coi tick da `1/60 s` vale circa `t − k·A`. Il piano mostra prima che la mutante dà i due valori diversi, o la mutazione è vacua. ➕ esecuzione: i due `Alpha` misurati sono `0.000` e `0.417`, non «circa `t − k·A`» (l'accumulatore parte dal primo tick che vede l'arrivo, non dall'istante dell'arrivo); la caduta non cambia.
15. In `ARTHexMapActor::SetPlaybackCues`, `Append` invece dell'assegnazione → cade `HexMapActor.PlaybackCueIsItsOwnChannel` («sostituisce»).
16. ➕ rev. (F2) `ClearPlaybackCues` tolto da `FinishPlayback` (blocco di spegnimento, `RTTurnManager.cpp:9186-9199`) → cade `Playback.CueChannelClearsOnSkip` («canale vuoto dopo `SkipPlayback` a metà `Marker`»). ⌫ *La prima stesura mutava la pulizia di fine Prep: vacua, perché con i tetti di R2 lì il canale è già vuoto.*
17. ➕ rev. (F9) In `IsTracerEligible`, `DefaultFxProfileFor(Shape).Tracer` sostituito con `FxProfileFor(…).Tracer` → cade `Privacy.FlightDependsOnShapeNotOverride` («volo di `Ram` == volo di `ImpactShot`»), ➕ rev2. e anche `Playback.TracerFollowsTheProfile` («`Ram` e `Action.Charge` idonee al volo»).
18. ➕ rev. (F8) In `FxProfileForIn`, la guardia «nessun id, nessun profilo» tolta → cade `Fx.NoActionNoProfile` sull'asserto ➕ rev2. **«profilo tutto `None`»** (il profilo torna quello della forma). ⌫ *La prima stesura nominava «`IsTracerEligible` falso»: quella funzione ha la propria clausola R12 e legge `DefaultFxProfileFor`, quindi questa mutante non la tocca.*
19. ➕ rev. (F12) In `TracerPolyline`, lo scarto laterale calcolato da `Alpha` invece che dall'indice del vertice → cade `Playback.TracerZigzagGrowsAsAPrefix` («prefisso»).
20. ➕ rev. (F7) In `CueSegments`, `AreaPulse` disegnato come `Ring` → cade `Fx.CueStylesDifferByGeometry` (coppia `Ring`/`AreaPulse`).
21. ➕ rev. In `BeginPlayback`, l'impronta non consumata dal primo colpo (ogni `Attack` dell'atto la porta) → cade `Playback.FootprintCueOncePerFootprint` («un solo `AreaPulse`», ➕ rev2. contato su tutti i tick).
22. ➕ rev2. In `IsTracerEligible`, la clausola R12 (`!(ActionId.IsNone() && BaseActionId.IsNone())`) tolta → cade `Fx.NoActionNoProfile` sull'asserto «`IsTracerEligible` falso anche con geometria risolta»: la forma `Single` di default ha un tracer, quindi il legacy riprenderebbe il volo.

⛔ **Gate che devono restare verdi senza modifiche** (se uno cambia, la ragione va nella PR; ➕ rev. F8: dopo la misura
di §2.8): i test del tracer (`Tests/RTPlaybackLibraryTests.cpp:287`, `:319`, `:346`, `:376`, `:457`, `:484`, `:1331`;
`Tests/RTAttackTracerPlaybackTests.cpp`; `Tests/RTAttackTracerTests.cpp`), `HexMapActor.PlaybackTracerIsItsOwnChannel`,
`Preview.TracerDrawsWithDebugDrawingOff`, `Tests/RTPlaybackActivationTests.cpp`, `Tests/RTPlaybackStopPredicateTests.cpp`,
`Tests/RTPlaybackBudgetIntegrationTests.cpp`, `Presentation.*`, `Privacy.ServerOnlyTypesAreNotReplicated`,
`Unit.BlueprintSurfaceIsCensused`, la suite di determinismo e replay. L'eccezione dichiarata è
`Playback.TracerStyleFollowsShapeForBasicAttack` (`:421`), qui sopra. ➕ rev. (F13)
`Privacy.TracerRhythmIsTheSameForEveryViewer` (`Tests/RTPlaybackLibraryTests.cpp:484-485`) resta verde: l'idoneità non
legge chi guarda. Il ritmo **cambia** per i colpi non base con un id e forma `Single`/`Line` (`LinearDischarge`, `Ram`,
`PassingBlade`): il loro arrivo cade prima del lancio successivo per costruzione (`F_eff ≤ A/2`).

### 5.2 Seduta PIE — `PIE-FX-ABILITA`, seduta `U71`

Dal banco Ability Lab (#3532) o da `L_DevSandbox` con `rt.Test.Scenario <Id>`, a velocità 1×. Una scena, una domanda
binaria; il verdetto è dell'autore.

0. ➕ rev. (F1) **Regressione sugli attacchi base.** Le scene (1) e (3) di `PIE-V01-TRACER` danno le **stesse** risposte
   di quella voce. La scena (2) **cambia** con SP4: se `PIE-V01-TRACER` è ancora senza verdetto, le sue tre domande si
   eseguono su `main` **prima** del merge (nota in coda alla sua cella, §2.2); dopo il merge la scena (2) si giudica con
   la domanda riformulata *«il getto di Muiren è una linea liscia ancorata a Muiren, e la scarica di Aevik viaggia a
   zigzag?»*. ➕ esecuzione: `PIE-V01-TRACER` ha il verdetto della seduta `U68`, dato su `main` prima di #3578; il
   confronto si fa con quel verdetto, e la scena (2) con la domanda riformulata.
1. `Visual.Ability.CastBeat` — *Durante la Prep vedi due anelli che si stringono sulla cella di Muiren, e nessun
   proiettile?* (`Pulse` di `TideGuard`)
2. `Visual.Ability.CastBeat` — *Quando il proiettile di Branth arriva su Muiren, compare una X sulla cella di Muiren
   dopo il proiettile e non prima?* (`Marker`; il proiettile è quello di `PIE-V01-TRACER`)
3. ➕ rev. `Visual.Combat.WaterElectricCoordinated` — *La scarica di Aevik è una linea spezzata che cresce da Aevik fino
   al bersaglio, distinguibile dal getto liscio di Muiren?* (`Zigzag` di `LinearDischarge`)
4. `Visual.Ability.FxProfile` — *Al primo numero del mortaio di Branth vedi un esagono a raggi che si allarga dal centro
   dell'area, una sola volta, e una X su **ciascuna** delle due vittime?* (`AreaPulse`, `Marker`)
5. ➕ rev. `Visual.Ability.FxProfile` — *Prima dell'onda di `Overload`, sulla cella di Aevik vedi dei raggi inclinati,
   diversi dall'anello di un'altra attivazione?* (`Flash`, F21)
6. `Visual.Combat.TracerHiddenFromUnseenAttacker` — *Vedi un anello, una X o un'onda su una cella che non vedi?* —
   atteso **no**.
7. `Cone`: **`N/A`** — forma senza contenuto in v0.1. Non si giudica.
8. ➕ esecuzione (R15). `Visual.Movement.Charge` — *Vedi l'anello d'attivazione sulla cella da cui Branth parte,
   **nessun** proiettile, e una X sulla cella colpita quando la carica arriva?* (`Hero.Branth.Ram`). È la regola dei
   controlli core a contatto, che non hanno una scena: nessuna unità li porta (§2.2).

---

## 6. Limiti dichiarati

- **La varietà per eroe del tracer ha un consumatore solo** (D5): gli attacchi base restano `Projectile`/`Jet`.
- ➕ rev. (F1) **La scena (2) di `PIE-V01-TRACER` cambia a schermo** con lo `Zigzag` di `LinearDischarge`: la regressione
  zero vale sugli attacchi base del roster (➕ rev2., lista dal catalogo) ed è pinnata da `Playback.BasicAttackTracersEqualShapeDefault`; la scena si esegue
  su `main` prima del merge, oppure si riformula (§2.2, §5.2). ➕ esecuzione: eseguita su `main` (seduta `U68`) prima
  del merge; il limite resta per la scena (2) dopo il merge, che giudica `PIE-FX-ABILITA`.
- ➕ esecuzione. **Il disegno si allarga col volo** (R13): ogni azione non base di forma `Single`/`Line` senza riga
  d'override riceve il tracer di default della forma (D3), anche dove prima non c'era nulla (§2.2).
- ➕ esecuzione. **La tabella è per `ActionId`, non per istanza**: un'azione con `RangeCells == 0` eredita la portata
  dell'arma, quindi un `Action.HeavyAttack` portato con un'arma a portata 1 colpisce a contatto e disegna un proiettile.
  R15 legge la portata del catalogo (§2.2, §7).
- ➕ esecuzione. **I controlli core a contatto non hanno una scena**: nessuna unità li porta in v0.1, e le loro righe hanno
  consumatori solo nei test (§2.2).
- ➕ rev. (F4) **`Cone`, forma senza contenuto in v0.1**: profilo, cue e test esistono; nessuna azione la dichiara, e la
  PIE la dichiara `N/A`. Nessun campo nuovo e nessun tocco al produttore la servono.
- ➕ rev. (F9) **Il ritmo rivela la classe di forma** di un colpo con un id a chi non vede l'attaccante (`Single`/`Line`
  contro `Area`/`Cone`), mai l'override (§2.3).
- ➕ rev. (F8) **Le unità legacy senza id** non hanno FX e non cambiano ritmo (R12).
- **Area senza vittime**: nessun pulse.
- **Il pulse non dice l'estensione dell'area**: la dice l'impronta.
- **Tick più lungo di una finestra**: una cue breve (`D_imp` fino a `0.20`) può non comparire in nessun fotogramma a
  velocità alta o con un fotogramma lento, e una fermata `Next Action` dopo un tick lungo può cadere oltre la sua
  finestra. Lo stato disegnato resta funzione dell'orologio; la visibilità di un one-shot breve a velocità alta no. Lo
  stesso vale oggi per il tracer.
- **Il ritmo per viewer** di D6 del momento vale anche per le cue (`RTTurnManager.cpp:7952-7956`).
- **Il viewer è il giocatore locale** (`:7856`).
- **Le durate e le scale sono graybox** (D-287 punto 7): si tarano a schermo, ma devono restare diverse a coppie (§2.1).
- **L'impronta `Line`/`Cone` di un attaccante ignoto** ne rivela la cella adiacente: preesistente,
  [#3551](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3551). Le cue di questa spec non la peggiorano:
  lo sweep è dietro `FromVerdict`.

---

## 7. Follow-up candidates

- Il pulse per un'area **senza vittime**, portato dall'`AttackFootprint` alla sua rivelazione: decisione d'autore,
  perché oggi la cue d'impronta la consegna il colpo.
- Le varianti di tracer degli attacchi base, se l'autore riapre D5.
- ➕ rev. (F22) ➕ rev3. `Hero.Aevik.ReactiveCapacitor`, **misurato il 2026-10-08: il contrattacco non produce un evento
  `Attack`** (`RTTurnManager.cpp:6357` è l'unico `Type = Attack`, nel ciclo sui colpi del piano; i contrattacchi entrano in
  `Attacks` dopo, `:6460-6468`, commento `:6470`; `Action.Counter` ha `Range 0` e trigger `HitByDirectAttack`). Quindi
  nessuna cue di colpo e nessuna riga d'override; il piano lo conferma con un grep al Task 4 Step 0. ➕ esecuzione:
  confermato al Task 4 Step 0 (l'unica assegnazione `Type = Attack` resta nel ciclo sui colpi del piano).
- ➕ rev. Se `Hero.Muiren.CircularTide` non emette `Attack`, il suo `AreaPulse` non ha consumatore: o la cura guadagna una
  cue d'impronta propria, o la riga perde `Footprint`. ➕ esecuzione: **confermato** al Task 4 Step 0 — la cura ha
  `bCountsAsAttack` falso ed esce in `URTHexCombatLibrary` prima dell'impronta e dei colpi; in v0.1 il suo `AreaPulse` e il
  suo `Marker` non hanno consumatore. ✅ Deciso in #3593: nessun consumatore, riga riscritta.
- ➕ rev. (F9) Un volo uguale per ogni colpo, se l'autore vuole chiudere anche il bit di forma del ritmo.
- `ReactionResolved` e `Defeated` come profilo, quando #2454 ne avrà la grammatica.
- I profili come dati esterni, se un autore non programmatore dovrà toccarli (migrazione di D-278).
- Una cue minima garantita per un fotogramma, se la PIE la chiede.
- `FindMissingBindings` che verifichi i nomi delle cue contro funzioni reali (già candidato della spec del tracer §8).
- `owner.epic` di `RT-CAP-VFX`: decisione di ownership.
- ➕ esecuzione. **Dai giri di review**, nessuno bloccante:
  - `Determinism.FxFieldsStayOutOfHashes` senza la premessa «TurnLog non vuoto» (due log vuoti hanno lo stesso hash), e
    non copre snapshot e replay.
  - Un asserto per `SourceVerdict` vuoto con il viewer `0` → nessuna cue.
  - La mutazione (7) copre solo Prep e Dash: nel Blast nessuna fixture ha una sorgente che si sposta.
  - `URTPlaybackLibrary::FootprintCueFor` assume che il `From` del colpo stia sulle celle dell'impronta: con due impronte a
    stessa chiave (R14) un colpo mostrerebbe l'impronta dell'altro intento col verdetto del proprio centro (nessun caso in
    v0.1). Correzione candidata: la cue solo se `From == AimCell` (`Area`) o `From == Origin` (`Cone`). La sua guardia
    `bResolved` non ha test né mutazione.
  - `Playback.FxCuesAreAFunctionOfTheClock` confronta i due `Alpha` fra loro, non col valore atteso.
  - `Fx.ConeSweepAxisIsTheAim` prova solo un asse orizzontale e `α = 0.5`: servono un asse obliquo e `α = 0`, `α = 1`.
  - `Preview.FxCuesDrawWithDebugDrawingOff` non asserisce lo spessore né il colore dello `Zigzag`; nessun test conta le
    linee di un `Jet` **disegnato** (la regressione zero sul `Jet` poggia sul diff di `RTHexMapActor.cpp`).
  - La portata d'istanza (§6): una riga per `ActionId` non vede l'arma che la porta.
  - L'atteso di `Fx.DeclaredOverridesMatchTheProposal` ignora il livello `BaseActionId` per le voci senza riga.
  - `MakeFxProfile` obbligatorio è una convenzione, non un vincolo di tipo.
  - `UENUM(BlueprintType)` sui tre enum nuovi, senza consumatori Blueprint.
  - Le varianti di tracer sugli attacchi base: D5 le ha sciolte «identiche», e restano riapribili dall'autore.
