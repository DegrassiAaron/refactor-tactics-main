# La scena fuori dall'HUD e la vista strategica isometrica: il panel del 2026-10-11

> `SNAPSHOT` · fotografia del 2026-10-11: vale finché è l'ultima misura del suo oggetto.
>
> - **Data**: 2026-10-11.
> - **Misurato su** `origin/main` = `1c58a69e4`.
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

## 2. Che cosa c'era, misurato prima di discutere

- **La griglia delle zone** è 3×3 meno il centro: fasce del 20% / 60% / 20% su entrambi gli assi ([`guida-screen-hud-umg.md`](../../technical/runbooks/guida-screen-hud-umg.md) §3). Il centro libero è il 60%×60%, che ha lo stesso aspetto dello schermo e ne copre il 36% della superficie.
- **Il centro libero vale per i pannelli, non per la scena.** La 3D si rende a tutto schermo e le zone le stanno sopra. Anche le tavole A–F disegnano la scena a tutto schermo, con la griglia di esagoni che continua sotto i pannelli ([`A-pianificazione-pulita.png`](../../research/design/hud/hud-screens-2026-10/immagini/A-pianificazione-pulita.png)).
- **Il 60% non ha una sorgente di runtime.** Sta in `Source/RefactorTactics/Tests/RTHudGeometryForTest.h` (`CenterFraction = 0.6f`) e negli anchor di `WBP_RT_TacticalHUD`.
- **La camera misura il viewport intero**: `ARTCameraPawn::RefreshViewportMetrics` legge `GetViewportSize` (`Camera/RTCameraPawn.cpp`).
- **UE 5.8 può rendere un giocatore in un rettangolo.** Verificato sugli header dell'engine:
  - `UGameViewportClient::SplitscreenInfo` e `LayoutPlayers` in `Engine/GameViewportClient.h`;
  - `FPerPlayerSplitscreenData` in `Engine/ViewportSplitScreen.h`;
  - `ULocalPlayer::Origin/Size` in `Engine/LocalPlayer.h`.
- **Oggi un click fuori dalla board pianifica lo stesso.** Il ripiego sul piano del layer risolve una cella ovunque: nella seduta PIE `U78` del 2026-10-11 un click sullo sfondo nero ha prodotto due waypoint.
- **La vista strategica ha due leve consegnate**, nate per una scelta che l'autore doveva fare in PIE:
  - una rampa del pitch con la distanza (`StrategicPitchDelta`, #3630);
  - un interruttore ortografico (`bStrategicOrthographic`, #3631).

## 3. Il panel

**Wiegers (requisiti).**
- La prima richiesta non diceva che cosa è «UI». Le famiglie sono tre, e solo la prima esce dalla mappa:
  - i pannelli delle zone;
  - la UI nel mondo: targhe ([D-320](../../decisions/RT_PDR_00_Decision_Log.md)), anelli, anteprima del percorso, sagome, danno fluttuante (Z13), conoscenza parziale (Z15);
  - gli stati a schermo intero (pausa, fine partita).
- La seconda richiesta non dava un angolo: «isometrica» copre il vero isometrico (−35,26°), il dimetrico 2:1 (−30°) e un'ortografica qualunque.

**Cockburn (obiettivo).** Il giocatore pianifica guardando il campo. Con la griglia di oggi, «nessuna sovrapposizione» lascia alla scena il 36% dello schermo. Le strade erano tre:
- tenere la cella centrale;
- stringere l'HUD;
- pannelli opachi sopra una scena a tutto schermo, che però soddisfa solo l'apparenza della richiesta.

**Fowler (architettura).**
- Il rettangolo della scena deve derivare dalla stessa sorgente delle zone. Se la scena legge un terzo numero, i tre divergono.
- Tutto ciò che misura il viewport deve misurare il rettangolo del giocatore:
  - limiti del pivot;
  - larghezza ortografica;
  - ancoraggio dello zoom;
  - deproiezione del click;
  - proiezioni mondo→schermo.

**Nygard (casi di guasto).**
- Un click fuori dal rettangolo non deve raggiungere il mondo.
- Il rettangolo si ricalcola a ogni cambio di dimensione: finestra, PIE nel viewport, 16:10, 21:9.
- `WBP_RT_FastDecision`, unica esenzione al centro libero, starebbe sopra la mappa: va dichiarata o spostata.

**Wiegers e Fowler sulla seconda richiesta (`CONTRACT CONFLICT`).**
- Un'isometrica fissa il pitch, mentre #3630 ha appena consegnato una rampa che lo fa variare con la distanza: le due cose non convivono.
- In strategica il pitch manuale va bloccato, e l'imbardata va decisa: gli esagoni hanno simmetria di 60°, `Q`/`E` girano a 45°.

**Crispin (verifica).**
- I criteri si scrivono come test prima del codice: rettangolo a più risoluzioni, deproiezione, click fuori dal rettangolo, pitch e proiezione per stato, ritorno con `Tab`.
- Poi una voce PIE per ciascuna richiesta.

## 4. Le decisioni dell'autore

| Domanda | Risposta | Registrata in |
|---|---|---|
| Come separare scena e HUD | **Stringere l'HUD**: le fasce si riducono ai loro contenuti e il rettangolo della scena cresce | [D-497](../../decisions/RT_PDR_00_Decision_Log.md) |
| L'angolo dell'isometrica | **Dimetrico 2:1**, pitch −30°, proiezione ortografica | [D-498](../../decisions/RT_PDR_00_Decision_Log.md) |
| La rotazione in strategica | **`Q`/`E` a scatti di 60°, pitch fisso**: l'orbita verticale non agisce | [D-498](../../decisions/RT_PDR_00_Decision_Log.md) |
| La finestra di reazione | **Resta un'esenzione dichiarata**: è l'unica UI a schermo ammessa sopra la scena | [D-497](../../decisions/RT_PDR_00_Decision_Log.md) |

## 5. La specifica

### 5.1 La scena fuori dall'HUD — [#3661](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3661)

1. **Una sorgente sola per la griglia.** Le quattro fasce sono un dato di runtime: colonna sinistra, colonna destra, fascia alta, fascia bassa. Lo leggono:
   - il rettangolo della scena;
   - gli anchor delle zone, o un test che li confronta;
   - il keep-out di `PanelsLeaveTheCenterFree`, che oggi usa una costante sua.
2. **Le fasce si stringono ai loro inquilini.** Proposta, misurata sulla tavola A a 1920×1080:

   | Fascia | Frazione proposta | Perché |
   |---|---|---|
   | Colonna sinistra | 20% | roster e scheda dell'unità finiscono a x≈380 |
   | Colonna destra | 16% | `Conferma` comincia a x≈1620 |
   | Fascia alta | 17% | header e celle di fase finiscono a y≈180 |
   | Fascia bassa | 17% | il dock comincia a y≈900 |

   La scena diventa circa 64%×66%, contro il 60%×60% della cella centrale. Le misure si riprendono sui widget veri, e l'autore conferma le frazioni in PIE. Stringere oltre il contenuto vuol dire ridisegnare gli inquilini, ed è fuori da qui.
3. **La 3D si rende solo nel rettangolo** fra le fasce. Fuori c'è lo sfondo dell'HUD, e il rettangolo si ricalcola a ogni cambio di dimensione.
4. **Ogni misura della camera, del click e delle proiezioni usa il rettangolo del giocatore.**
5. **Un click fuori dal rettangolo non raggiunge il mondo.**
6. **Esenzione**: `WBP_RT_FastDecision`, finché la finestra è aperta.
7. **La UI nel mondo resta sopra la mappa**: è parte della scena.

### 5.2 La vista strategica isometrica — [#3662](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3662)

1. **Lo stato strategico implica proiezione ortografica e pitch −30°**, a ogni distanza oltre la soglia. Lo stato resta derivato dalla distanza ([D-252](../../decisions/RT_PDR_00_Decision_Log.md)), e il passaggio resta un taglio netto ([D-495](../../decisions/RT_PDR_00_Decision_Log.md) (4)).
2. **La rampa di #3630 si ritira** dal codice, con i suoi test. Resta la correzione di `AddOrbit`.
3. **La proiezione ortografica è la regola**: l'interruttore di #3631 esce. Restano la larghezza ortografica derivata dal braccio e la continuità della scala sul pivot.
4. **In strategica il pitch manuale non agisce**, e al ritorno torna quello della tattica.
5. **`Q`/`E` ruotano di 60° in strategica**, di 45° in tattica. Sono proposte, rovesciabili in PIE:
   - all'ingresso l'imbardata si aggancia al multiplo di 60° più vicino;
   - all'uscita resta dov'è;
   - in strategica anche l'orbita orizzontale è inerte.
6. **La separazione dei piani** ([#3632](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3632)) resta, con il valore ancora da scegliere.

## 6. Che cosa succede alle issue aperte

| Issue | Che cosa cambia |
|---|---|
| [#3630](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3630) | La sua scelta pendente («l'autore sceglie il valore») è presa da D-498: niente rampa, angolo fisso. Il codice della rampa lo ritira #3662 |
| [#3631](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3631) | La sua scelta pendente («l'autore vede le due proiezioni») è presa da D-498: ortografica. Il click ai tre punti del viewport si rimisura con #3662 sul rettangolo di #3661 |
| [#3632](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3632) | Invariata: il valore della separazione si sceglie ancora in PIE |

## 7. Fuori da questo lavoro

| Che cosa | Classe | Dove resta |
|---|---|---|
| Ridisegnare gli inquilini delle zone per stringerle oltre il contenuto | `DEFERRED` | una issue di design, se l'autore la vuole |
| Le soglie della vista strategica | `DEFERRED` | [#1780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1780) |
| Split-screen multigiocatore | `DEFERRED` | nessuna issue: fuori dalla v0.1 |
