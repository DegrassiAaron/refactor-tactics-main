# #3881 - la fascia bassa a 18%: misure e catture (2026-10-27)

Evidenza **dell'assistente**, non un giudizio dell'autore. Riguarda [#3881](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3881) e la decisione [D-549](../../../decisions/RT_PDR_00_Decision_Log.md).

## Che cosa e' stato catturato, e come

- **PIE nell'Editor** (`NetMode: Standalone`), finestra flottante, partita avviata, nessuna selezione: il prompt mostrato e' quello di `IdleSelection`, «Seleziona un'unita'» con l'accento.
- Cattura con `PrintWindow` della finestra per titolo, dal clone principale, **senza inviare input** alla PIE. Il client include la barra del titolo (40 px).
- Blob del dock: padding verticale di `BarPanel` **20 sopra e 22 sotto**, padding sinistro del prompt **32**; C++ con `FRTHudBands::Bottom` a 194,4.
- **Non e' il pacchetto cotto**: la resa del pacchetto non e' stata misurata.
- I valori di finestra della PIE (`LevelEditorPlaySettings`) sono stati riportati per byte a Editor chiuso (1914x1080, posizione 0,-32): il ripristino via bridge non arriva al file.

## Le catture

| file | finestra | che cosa si vede |
|---|---|---|
| `pie-1600x900-fascia-18-2026-10-27.jpg` | 1600x900 | prompt **intero** sotto il dock, a circa 10 px dal bordo inferiore del client |
| `pie-1920x1080-fascia-18-2026-10-27.jpg` | 1920x1080 | prompt **intero** sotto il dock, a circa 10 px dal bordo inferiore del client |

La distanza dal bordo e' una lettura **a occhio** dalla cattura. E' minore di quella del layout 12/12 di [#3880](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3880) (circa 20 px, [`../3869/`](../3869/prompt-del-dock-misure-2026-10-27.md)): il dock torna alto come nel mockup e la fascia lo contiene, invece di stringerlo.

## La misura: il contratto «gli inquilini entrano nelle loro fasce»

Gate `RefactorTactics.ScreenHud.TenantsFitTheirBands` (D-497 (2)), headless, **riga intera** del testo, misurato sul blob finale e sul C++ finale. La zona `Bottom` e' alta `Bottom - 2 * ZoneMargin` = 186,4 unita' Slate. Dock col prompt piu' alto, contro lo spazio:

| finestra | scala | dock | spazio | margine |
|---|---|---|---|---|
| 1920x1080 e 2560x1080 | 1,000 | 185,0 | 186,4 | 1,4 |
| 1600x900 | 0,833 | 182,8 | 186,4 | 3,6 |
| 1280x800 | 0,740 | 182,4 | 186,4 | 4,0 |
| 1920x1200 | 1,111 | 185,5 | 186,4 | 0,9 |

- I due scenari del gate (`Spec.Hud.MatchWithTwoAllies` e `Visual.Map.LayerStack`) danno gli stessi numeri a ogni finestra.
- **La misura headless e' un limite superiore, non la resa**: conta la riga intera del testo, non l'inchiostro, e supera la PIE di circa il 3-4% ([`../3869/`](../3869/prompt-del-dock-misure-2026-10-27.md)). Il margine piu' stretto, 0,9 a scala 1,111, e' sulla riga intera: in PIE e' piu' largo.
- Le altre zone non cambiano: roster 259, scheda 250, testata 100.

## Limiti

- Un solo prompt catturato per finestra, e nessuna con una selezione: gli altri testi li misura il gate.
- Il padding sinistro del prompt (32) l'ho misurato dalla cattura: l'etichetta COMUNI sta a 32 unita' dal bordo del pannello.
- Gli altri inquilini delle fasce sono misurati dal gate a scala 1,0, non a ogni risoluzione: e' un follow-up gia' nominato in [#3880](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3880).
- Il pacchetto cotto: `NOT RUN`.
