# La scena fuori dall'HUD e la vista strategica isometrica: il panel del 2026-10-11

> `SNAPSHOT` · fotografia del 2026-10-11: vale finché è l'ultima misura del suo oggetto.
>
> - **Data**: 2026-10-11.
> - **Misurato su** `origin/main` = `1c58a69e4`, rimisurato dalla review su `15ad54d73`.
> - **Oggetto**: due modifiche chieste dall'autore in una sessione `/sc:spec-panel`. (1) La UI non sta sopra il render della camera: UI e mappa non si sovrappongono. (2) La vista strategica usa una proiezione isometrica.
> - **Issue**: [#3661](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3661) (la scena nel rettangolo libero) · [#3662](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3662) (CAM-05c, l'isometrica).
> - **Decisioni registrate**: [D-497](../../decisions/RT_PDR_00_Decision_Log.md) e [D-498](../../decisions/RT_PDR_00_Decision_Log.md).
>
> ⛔ Nessuna riga di `Source/` e di `Content/` cambia con questo referto.
>
> Convenzione: niente totali che cambiano da soli. Dove un numero serve, sta accanto alla sua fonte.

---

## 1. Le due richieste, come sono state scritte

1. «La UI non deve stare sopra il render della telecamera. Far in modo che la UI e la mappa di gioco non si sovrappongano.»
2. «Nella visuale strategica, usiamo una proiezione isometrica.»

## 2. Che cosa c'era, misurato prima di decidere

- **La griglia delle zone** è 3×3 meno il centro, a fasce del 20% / 60% / 20% su entrambi gli assi ([`guida-screen-hud-umg.md`](../../technical/runbooks/guida-screen-hud-umg.md) §3). Il centro libero è il 60%×60%, che ha lo stesso aspetto dello schermo e ne copre il 36%.
  - [D-456](../../decisions/RT_PDR_00_Decision_Log.md) (7) dice che quei tagli restano.
  - Il canone di [`progettazione-hud.md`](../../technical/systems/progettazione-hud.md) §3 chiede un «70–80% centrale: viewport 3D libero».
- **Il centro libero vale per i pannelli, non per la scena.** La 3D si rende a tutto schermo e le zone le stanno sopra. Le tavole A–F disegnano la scena a tutto schermo, con la griglia di esagoni che continua sotto i pannelli ([`A-pianificazione-pulita.png`](../../research/design/hud/hud-screens-2026-10/immagini/A-pianificazione-pulita.png)).
- **Il 60% vive in tre posti, nessuno di runtime**:
  - `CenterFraction` in `Source/RefactorTactics/Tests/RTHudGeometryForTest.h`, letta da `PanelsLeaveTheCenterFree` e dal test del Context Inspector;
  - `RTGrigliaZone` in `Source/RefactorTactics/Tests/RTMatchWidgetAssetTests.cpp`, che pilota `ScreenHud.ZoneRectanglesMatchTheThreeByThreeGrid`;
  - gli anchor di `WBP_RT_TacticalHUD`.
- **La camera misura il viewport intero**: `ARTCameraPawn::RefreshViewportMetrics` legge `Viewport->GetSizeXY()`. L'aspetto che ne ricava lo usa solo `ComputeEffectivePivotBounds`, cioè il limite del pivot di [D-251](../../decisions/RT_PDR_00_Decision_Log.md). La larghezza ortografica non dipende dall'aspetto: è `OrthoWidthForArm(TargetArmLength, FieldOfView)`.
- **UE 5.8 sa rendere un giocatore in un rettangolo.** Verificato sugli header dell'engine:
  - `UGameViewportClient::SplitscreenInfo` e `LayoutPlayers` in `Engine/GameViewportClient.h`;
  - `FPerPlayerSplitscreenData` in `Engine/ViewportSplitScreen.h`;
  - `ULocalPlayer::Origin/Size` in `Engine/LocalPlayer.h`.
- **La vista strategica ha due leve consegnate**, nate per una scelta che l'autore doveva fare in PIE: una rampa del pitch con la distanza (`StrategicPitchDelta`, #3630) e un interruttore ortografico (`bStrategicOrthographic`, #3631).
- **La rotazione ha una decisione sua**: [D-142](../../decisions/RT_PDR_00_Decision_Log.md) tiene lo scatto a 45° perché non divide 60. Allineandosi agli assi degli esagoni, un cilindro copre la cella dietro di sé.

## 3. Il panel

**Wiegers (requisiti).**
- La prima richiesta non diceva che cosa è «UI». Le famiglie sono tre:
  - i pannelli delle zone, l'HUD di partita;
  - la UI nel mondo: targhe ([D-320](../../decisions/RT_PDR_00_Decision_Log.md)), anelli, anteprima del percorso, sagome, danno fluttuante (Z13), conoscenza parziale (Z15);
  - gli strumenti di sviluppo: Context Inspector, `RTPieVerdictOverlay`, console.

  Solo la prima esce dalla mappa.
- La seconda richiesta non dava un angolo: «isometrica» copre il vero isometrico (−35,26°), il dimetrico 2:1 (−30°) e un'ortografica qualunque.

**Cockburn (obiettivo).** Il giocatore pianifica guardando il campo, e «nessuna sovrapposizione» toglie spazio alla scena. Le strade erano tre:
- la cella centrale, al 36% dello schermo;
- stringere l'HUD;
- pannelli opachi sopra una scena a tutto schermo, che soddisfa solo l'apparenza della richiesta.

**Fowler (architettura).**
- Il rettangolo della scena deve derivare dalla stessa sorgente delle zone. Oggi le copie del 60% sono già tre.
- Il limite del pivot e la deproiezione devono misurare il rettangolo del giocatore, non il viewport.

**Nygard (casi di guasto).**
- Un click fuori dal rettangolo non deve raggiungere il mondo.
- Il rettangolo si ricalcola a ogni cambio di dimensione.
- La scala dell'interfaccia di UE segue il lato corto, mentre le fasce sono frazioni dello schermo: a 1280×800 o a 1920×1200 un inquilino può non entrare nella sua fascia.
- `WBP_RT_FastDecision`, unica esenzione al centro libero, starebbe sopra la mappa.

**Wiegers e Fowler sulla seconda richiesta (`CONTRACT CONFLICT`).**
- Un'isometrica fissa il pitch, mentre #3630 ha appena consegnato una rampa che lo fa variare con la distanza.
- Il panel aveva proposto `Q`/`E` a 60° in strategica, ma la review ha trovato D-142, che lo esclude: è un secondo conflitto, e l'autore l'ha risolto tenendo i 45°.

**Crispin (verifica).**
- I criteri vanno scritti come test prima del codice: rettangolo a più risoluzioni, deproiezione come funzione pura, click fuori dal rettangolo, pitch e proiezione per stato nei due versi dell'isteresi, ritorno con `Tab`.
- Poi una voce PIE per ciascuna richiesta.

## 4. Le decisioni dell'autore

Due giri di domande. Il secondo nasce dalla review di [#3663](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3663), che ha trovato D-142, la colonna destra misurata sull'inquilino sbagliato e il canone del 70–80%.

| Domanda | Risposta | Registrata in |
|---|---|---|
| Come separare scena e HUD | **Stringere l'HUD**: le fasce si riducono ai loro contenuti e il rettangolo della scena cresce | [D-497](../../decisions/RT_PDR_00_Decision_Log.md) (2) |
| Quanto stringere | **Sulle misure dei contenuti veri** (~61%×66%), non verso il 70% del canone, che chiederebbe di ridisegnare gli inquilini | D-497 (2) |
| Che cosa si conserva nel rettangolo | **L'estensione**: si vede la stessa porzione di mondo, tutto più piccolo | D-497 (4) |
| La finestra di reazione | **Resta un'esenzione dichiarata** | D-497 (5) |
| L'angolo dell'isometrica | **Dimetrico 2:1**, pitch −30°, proiezione ortografica | [D-498](../../decisions/RT_PDR_00_Decision_Log.md) (1) |
| La rotazione in strategica | **45°, come in tattica** (D-142 vale); pitch fisso e orbita verticale inerte | D-498 (4) |

## 5. La specifica

### 5.1 La scena fuori dall'HUD — [#3661](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3661)

1. **Una sorgente sola per la griglia.** Le quattro fasce sono un dato di runtime. Lo leggono:
   - gli anchor delle zone, o un test che li confronta;
   - il rettangolo della scena;
   - i tre lettori di oggi: `PanelsLeaveTheCenterFree`, il test del Context Inspector e `ScreenHud.ZoneRectanglesMatchTheThreeByThreeGrid`.
2. **Le fasce si stringono ai loro inquilini.** Proposta, misurata sulle tavole a 1920×1080:

   | Fascia | Frazione proposta | Perché |
   |---|---|---|
   | Colonna sinistra | 20% | roster e scheda dell'unità della tavola A finiscono a x≈380 |
   | Colonna destra | 19% | il registro della tavola D comincia a x≈1556; `Conferma` sta nella fascia alta |
   | Fascia alta | 17% | header e celle di fase finiscono a y≈180 |
   | Fascia bassa | 17% | il dock comincia a y≈900 |

   La scena diventa circa 61%×66%. Le misure si riprendono sui widget veri, a più risoluzioni e con la scala dell'interfaccia di UE, e l'autore conferma le frazioni in PIE.
3. **La 3D si rende solo nel rettangolo**, ricalcolato a ogni cambio di dimensione. Fuori c'è lo sfondo dell'HUD.
4. **A parità di braccio il rettangolo inquadra la stessa porzione di mondo di oggi.** Celle e unità appaiono più piccole; soglie e zoom non si ritarano.
5. **Limite del pivot e deproiezione usano il rettangolo del giocatore.** Un click fuori dal rettangolo non raggiunge il mondo.
6. **Esenzione**: `WBP_RT_FastDecision`, ricentrata sul rettangolo.
7. **Fuori dalla regola**: la UI nel mondo, che è parte della scena, e gli strumenti di sviluppo. Il budget del Context Inspector sotto la nuova fascia bassa si decide in #3661, perché il suo test oggi misura contro il keep-out del 60%.

### 5.2 La vista strategica isometrica — [#3662](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3662)

1. **Finché lo stato è strategico la proiezione è ortografica e il pitch vale −30°**, anche fra le due soglie per l'isteresi di [D-252](../../decisions/RT_PDR_00_Decision_Log.md). Finché è tattico, restano prospettiva e pitch di oggi. Il passaggio è un taglio netto ([D-495](../../decisions/RT_PDR_00_Decision_Log.md) (4)).
2. **La rampa di #3630 si ritira** dal codice, con i suoi test. Resta la correzione di `AddOrbit`.
3. **La proiezione ortografica è la regola**: l'interruttore di #3631 esce.
4. **In strategica l'orbita verticale non agisce**, e al ritorno torna il pitch manuale della tattica. `Q`/`E` e l'orbita orizzontale restano quelle di oggi (D-142).
5. **La separazione dei piani** ([#3632](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3632)) resta.

## 6. Che cosa succede alle issue aperte

| Issue | Che cosa cambia |
|---|---|
| [#3630](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3630) | La sua scelta pendente («l'autore sceglie il valore») è presa da D-498: niente rampa, angolo fisso. Il codice della rampa lo ritira #3662 |
| [#3631](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3631) | La sua scelta pendente («l'autore vede le due proiezioni») è presa da D-498: ortografica. Il click ai tre punti del viewport si rimisura con #3662, sul rettangolo di #3661 |
| [#3632](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3632) | Invariata da questo referto |

## 7. Fuori da questo lavoro

| Che cosa | Classe | Dove resta |
|---|---|---|
| Ridisegnare gli inquilini delle zone per arrivare al 70% del canone | `DEFERRED` | una issue di design, se l'autore la vuole |
| Le soglie della vista strategica | `DEFERRED` | [#1780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1780) |
| Split-screen multigiocatore | `DEFERRED` | nessuna issue: fuori dalla v0.1 |
