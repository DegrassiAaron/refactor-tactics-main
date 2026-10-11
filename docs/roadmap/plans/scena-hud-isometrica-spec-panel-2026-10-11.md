# La scena fuori dall'HUD e la vista strategica isometrica: il panel del 2026-10-11

> `SNAPSHOT` · fotografia del 2026-10-11: vale finché è l'ultima misura del suo oggetto.
>
> - **Data**: 2026-10-11.
> - **Misurato su** `origin/main` = `1c58a69e4`, rimisurato dalla prima review su `15ad54d73` e dalla seconda su `11e628c85`.
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
- **La camera misura il viewport intero**: `ARTCameraPawn::RefreshViewportMetrics` legge `Viewport->GetSizeXY()`. L'aspetto che ne ricava lo usa solo `ComputeEffectivePivotBounds`, cioè il limite del pivot di [D-251](../../decisions/RT_PDR_00_Decision_Log.md). La larghezza ortografica è `OrthoWidthForArm(TargetArmLength, FieldOfView)` e non riceve l'aspetto; l'estensione resa, sì (sotto).
- **La deproiezione segue già il rettangolo del giocatore**: `UGameplayStatics::DeprojectScreenToWorld` passa da `ULocalPlayer::GetProjectionData`, che legge `Origin` e `Size`. Lo ha trovato la seconda review.
- **UE 5.8 sa rendere un giocatore in un rettangolo.** Verificato sugli header dell'engine:
  - `UGameViewportClient::SplitscreenInfo` e `LayoutPlayers` in `Engine/GameViewportClient.h`;
  - `FPerPlayerSplitscreenData` in `Engine/ViewportSplitScreen.h`;
  - `ULocalPlayer::Origin/Size` in `Engine/LocalPlayer.h`.
- **Il vincolo d'aspetto è `AspectRatio_MaintainYFOV`** (`BaseEngine.ini`; il progetto non lo cambia). Letto su `CameraStackTypes.cpp` dalla seconda review, e riletto:
  - la prospettica tiene fisso il semiangolo verticale `atan(tan(FOV/2) / 1,78)`, e l'orizzontale segue l'aspetto del quadro. Un rettangolo più piccolo conserva da sé l'estensione verticale;
  - l'ortografica usa `OrthoWidth` come estensione **verticale**. `OrthoWidthForArm` la calcola col FOV orizzontale, quindi l'ortografica di #3631 inquadra circa 1,78× la prospettica. Il test di #3631 confronta la formula, non la proiezione.
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
| Quanto stringere | **Sulle misure dei contenuti veri**, non verso il 70% del canone, che chiederebbe di ridisegnare gli inquilini. Allora la proposta era ~61%×66%, il 40% (§4.1) | D-497 (2) |
| Che cosa si conserva nel rettangolo | **L'estensione**: si vede la stessa porzione di mondo in verticale, tutto più piccolo | D-497 (4) |
| La finestra di reazione | **Resta un'esenzione dichiarata** | D-497 (5) |
| L'angolo dell'isometrica | **Dimetrico 2:1**, pitch −30°, proiezione ortografica | [D-498](../../decisions/RT_PDR_00_Decision_Log.md) (1) |
| La rotazione in strategica | **45°, come in tattica** (D-142 vale); pitch fisso e orbita verticale inerte | D-498 (4) |

### 4.1 La seconda review

La review delle correzioni ha trovato due misure sbagliate. Correggerle applica il metodo che l'autore ha scelto, senza cambiarlo:
- **la fascia alta era misurata sulla sola colonna centrale.** La griglia ha un taglio solo per riga, e in `Zone_TopLeft` il roster finisce a y=200: la fascia va al 19%, non al 17%;
- **il margine di 4 px della griglia non era contato.** La colonna destra va al 19,2%, perché il registro e il badge accanto a `Conferma` cominciano a x=1556. `Conferma` vincola la colonna, al contrario di quanto diceva la prima correzione.

La scena passa da ~61%×66% (il 40% dello schermo) a ~61%×64% (il 39%). La cella centrale scartata ne copre il 36%: con le misure corrette le due strade differiscono di circa tre punti. L'autore lo ha saputo dopo la scelta.

Ha trovato anche i due fatti dell'engine del §2: l'estensione verticale si conserva da sé, e l'ortografica di #3631 non è continua con la prospettica.

## 5. La specifica

### 5.1 La scena fuori dall'HUD — [#3661](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3661)

1. **Una sorgente sola per la griglia.** Le quattro fasce sono un dato di runtime. Lo leggono:
   - gli anchor delle zone, o un test che li confronta;
   - il rettangolo della scena;
   - i tre lettori di oggi: `PanelsLeaveTheCenterFree`, il test del Context Inspector e `ScreenHud.ZoneRectanglesMatchTheThreeByThreeGrid`.
2. **Le fasce si stringono ai loro inquilini.** Proposta, misurata sul sorgente delle tavole (`sorgente-mockup/RefactorTactics-HUD.dc.html`) a 1920×1080, con il margine di 4 px della griglia:

   | Fascia | Frazione proposta | Perché |
   |---|---|---|
   | Colonna sinistra | 20% | roster e scheda dell'unità finiscono a x=380 |
   | Colonna destra | 19,2% | il registro e il badge accanto a `Conferma` cominciano a x=1556 |
   | Fascia alta | 19% | il roster, in `Zone_TopLeft`, finisce a y=200; header e celle di fase a y=180 |
   | Fascia bassa | 17% | il dock comincia a y=903 |

   La scena diventa circa 61%×64%, il 39% dello schermo. Le misure si riprendono sui widget veri, a più risoluzioni e con la scala dell'interfaccia di UE, e l'autore conferma le frazioni in PIE.
3. **La 3D si rende solo nel rettangolo**, ricalcolato a ogni cambio di dimensione. Fuori c'è lo sfondo dell'HUD.
4. **A parità di braccio il rettangolo inquadra la stessa estensione verticale di oggi**, che `MaintainYFOV` conserva da sé. L'orizzontale segue l'aspetto del rettangolo (circa il 5% in meno). Celle e unità appaiono circa 0,64×; soglie e zoom non si ritarano.
5. **Il limite del pivot usa il rettangolo del giocatore**, con il modello di `MaintainYFOV`: semiasse verticale fisso, orizzontale dall'aspetto. La deproiezione lo segue già; un click fuori dal rettangolo non raggiunge il mondo.
6. **Esenzione**: `WBP_RT_FastDecision`, ricentrata sul rettangolo.
7. **Fuori dalla regola**: la UI nel mondo, che è parte della scena, e gli strumenti di sviluppo. Il Context Inspector oggi rispetta il riquadro del 60%, e sotto la fascia bassa al 17% non ci sta più (174 px contro ~160). Se riduce le righe a 7, si sposta o perde la soglia si decide in #3661.

### 5.2 La vista strategica isometrica — [#3662](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3662)

1. **Finché lo stato è strategico la proiezione è ortografica e il pitch vale −30°**, anche fra le due soglie per l'isteresi di [D-252](../../decisions/RT_PDR_00_Decision_Log.md). Finché è tattico, restano prospettiva e pitch di oggi. Il passaggio è un taglio netto ([D-495](../../decisions/RT_PDR_00_Decision_Log.md) (4)).
2. **La rampa di #3630 si ritira** dal codice, con i suoi test. Resta la correzione di `AddOrbit`.
3. **La proiezione ortografica è la regola**: l'interruttore di #3631 esce. La scala sul pivot va resa continua misurando la proiezione, non la formula: oggi l'ortografica inquadra circa 1,78× la prospettica.
4. **In strategica l'orbita verticale non agisce**, e al ritorno torna il pitch manuale della tattica. `Q`/`E` e l'orbita orizzontale restano quelle di oggi (D-142).
5. **La separazione dei piani** ([#3632](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3632)) resta.

## 6. Che cosa succede alle issue aperte

| Issue | Che cosa cambia |
|---|---|
| [#3630](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3630) | La sua scelta pendente («l'autore sceglie il valore») è presa da D-498: niente rampa, angolo fisso. Il codice della rampa lo ritira #3662 |
| [#3631](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3631) | La sua scelta pendente («l'autore vede le due proiezioni») è presa da D-498: ortografica. Il click ai tre punti del viewport e la scala al cambio di proiezione passano a #3662 |
| [#3632](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3632) | Invariata da questo referto: il valore della separazione (3) lo registra la seduta `U78` ([#3665](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3665)) |

## 7. Fuori da questo lavoro

| Che cosa | Classe | Dove resta |
|---|---|---|
| Ridisegnare gli inquilini delle zone per arrivare al 70% del canone | `DEFERRED` | una issue di design, se l'autore la vuole |
| Le soglie della vista strategica | `DEFERRED` | [#1780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1780) |
| Split-screen multigiocatore | `DEFERRED` | nessuna issue: fuori dalla v0.1 |
