# #3869 - il prompt del dock: misure e catture (2026-10-27)

Evidenza **dell'assistente**, non un giudizio dell'autore. Riguarda [#3869](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3869).

> **Aggiornamento 2026-10-27 ([#3881](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3881), [D-549](../../../decisions/RT_PDR_00_Decision_Log.md)).** Il padding 12/12 di questa nota e' stato **sostituito**: l'autore ha scelto di alzare la fascia bassa e tornare a 20/22. Le catture «dopo» e le misure del blob «finale» qui sotto descrivono il layout di #3880 (12/12, fascia bassa a 183,6) e restano come storia. Il layout in vigore e' in [`../3881/`](../3881/misure-fascia-bassa-2026-10-27.md).

## Che cosa e' stato catturato, e come

- **PIE nell'Editor** (`NetMode: Standalone`), finestra flottante, partita avviata, nessuna selezione: il prompt mostrato e' quello di `IdleSelection`.
- Cattura con `PrintWindow` della finestra per titolo, dal clone principale, **senza inviare input** alla PIE. Il client include la barra del titolo (40 px).
- **Non e' il pacchetto cotto** dove l'issue ha visto il difetto: la resa del pacchetto non e' stata misurata.
- Le catture "prima" a 1920x1080 hanno il **testo con l'accento** perche' il C++ e' quello nuovo e l'asset quello di `main`; quella a 1600x900 e' di prima del cambio di testo e dice `unita'`.

## Le catture

| file | blob del dock | finestra | che cosa si vede |
|---|---|---|---|
| `pie-1600x900-prima-2026-10-27.jpg` | `main` | 1600x900 | prompt Roboto Bold 24 bianco, **tagliato** dal bordo inferiore: se ne vede la parte alta |
| `pie-1920x1080-prima-2026-10-27.jpg` | `main` | 1920x1080 | lo stesso difetto: **tagliato**, se ne vede la parte alta |
| `pie-1920x1080-primo-ritocco-2026-10-27.jpg` | primo ritocco (Exo 2 12, padding barra 20+22) | 1920x1080 | **intero** ma a circa 4 px dal bordo del client |
| `pie-1600x900-dopo-2026-10-27.jpg` | finale | 1600x900 | **intero**, a circa 20 px dal bordo, allineato all'etichetta COMUNI |
| `pie-1920x1080-dopo-2026-10-27.jpg` | finale | 1920x1080 | **intero**, a circa 20 px dal bordo |

Il primo ritocco a 1600x900 non ha un file proprio: la cattura (testo intero, circa 4 px dal bordo) e' stata sostituita da quella del layout finale.

## La misura: il contratto "gli inquilini entrano nelle loro fasce"

Il dock sta in `Zone_Bottom`, alta `Bands().Bottom - 2 * ZoneMargin` = 175,6 unita' Slate (`RTHudLayout::ZoneLayout`). Il gate `RefactorTactics.ScreenHud.TenantsFitTheirBands` (D-497 (2)) costruisce l'HUD vero, da' il contesto di partita, fa girare il Tick del dock (che costruisce gli slot), scrive ogni testo che `BuildTargetPrompt` puo' produrre e misura la zona con Slate a ogni risoluzione **con la scala di quella risoluzione**. Zona `Bottom` col prompt piu' alto, in unita' Slate, contro 175,6:

| blob del dock | scala 0,740 (1280x800) | 0,833 (1600x900) | 1,000 (1920x1080, 2560x1080) | 1,111 (1920x1200) |
|---|---|---|---|---|
| `main` | 199,3 | 200,0 | 202,0 | 201,5 |
| primo ritocco | 182,4 | 182,8 | 185,0 | 185,5 |
| finale | 164,4 | 164,8 | 167,0 | 167,5 |

- **Il primo ritocco violava il contratto a ogni scala misurata, anche a 1600x900**, di 7-10 unita': la riga del prompt usciva dalla fascia. Ma **a schermo il testo si vedeva intero** (circa 4 px dal bordo, a 900p e a 1080p). La mia prima versione di questo documento e di questa PR sosteneva che il primo ritocco "non bastava a 1920x1080" e che a 1600x900 "il carattere si arrotonda e il dock torna nella zona": **entrambe le affermazioni erano sbagliate**, la seconda e' smentita dalla misura headless a 0,833 (182,8 contro 175,6).
- **La misura headless e' un limite superiore, non la resa**: conta la riga intera del testo, non l'inchiostro, e supera la PIE di circa il 3-4% (la barra misura 161,6 unita' a scala 0,833 in headless contro 156 in PIE). Per questo il primo ritocco e' rosso nel gate e visibile in PIE: non c'e' contraddizione, ci sono due misure di due cose.
- Il margine finale sulla riga intera e' fra 8,1 e 11,2 unita' alle scale misurate (175,6 meno 167,5 e meno 164,4).

## IMPLEMENTATION DRIFT (CLAUDE.md §3) - RISOLTO da D-549

> **Risolto il 2026-10-27**: la fascia bassa e' stata alzata a 194,4 e il padding e' tornato a 20/22, quindi `tokens.json`, la sua copia e il runbook U65 sono di nuovo coerenti con l'asset e non sono stati modificati. Il testo seguente e' quello di quando il drift era aperto.

Il padding verticale di `BarPanel` e' passato da **20 sopra e 22 sotto a 12 e 12**. Due fonti dicono ancora 20/22 e **non le ho riconciliate**:

- `docs/research/skill-bar-pacchetto/dati/tokens.json` e la sua copia `docs/research/design/hud/skill-bar-2026-10/dati/tokens.json`: `"dock_padding": [20, 24, 22, 24]` (un JSON non porta un commento: non li ho toccati);
- `docs/technical/runbooks/guida-seduta-u65-barra-fedele-al-mockup.md` §3: "padding sopra 20, ai lati 24, sotto 22". **Questo l'ho annotato** con un avviso che rimanda qui, senza cambiare la prescrizione.

Serve una decisione dell'autore: aggiornare token e runbook al nuovo padding, oppure scegliere un'altra soluzione (alzare la fascia `Bands().Bottom`, o far stare il prompt dentro il pannello). **Se una seduta futura riapplica 20/22 dal runbook, il gate diventa rosso** (e' il blob del primo ritocco): non torna in silenzio.

## Stato misurato

- Le misure del gate sono state prese con i test del commit `1e86e37e8` e i blob di `main`, del commit `9985dfd21` (primo ritocco) e del commit `f92b7752b` (finale). Per il blob finale i due scenari del gate (`Spec.Hud.MatchWithTwoAllies` e `Visual.Map.LayerStack`) danno gli stessi numeri; per `main` e per il primo ritocco ho letto lo scenario `LayerStack`.
- La suite Automation intera e' stata eseguita sul commit `eb160df21`, con zero rossi; i commit successivi di questo ramo toccano solo test e documenti, e vanno riletti nella PR.

## Limiti

- Un solo prompt catturato. Gli altri hanno lo stesso widget e lo stesso stile, e il gate li misura tutti.
- Il padding sinistro del prompt (32) l'ho misurato dalla cattura: l'etichetta COMUNI sta a 32 unita' dal bordo del pannello; 24 e' il padding di `BarPanel`, gli altri 8 non li ho attribuiti a un elemento.
- Un dock con piu' righe di quelle che lo scenario costruisce non e' misurato.
