# #3869 - il prompt del dock: misure e catture (2026-10-27)

Evidenza **dell'assistente**, non un giudizio dell'autore. Riguarda [#3869](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3869).

## Che cosa e' stato catturato, e come

- **PIE nell'Editor** (`NetMode: Standalone`), finestra flottante, partita avviata, nessuna selezione: il prompt mostrato e' quello di `IdleSelection`.
- Cattura con `PrintWindow` della finestra per titolo, dal clone principale, **senza inviare input** alla PIE. Il client include la barra del titolo (40 px).
- **Non e' il pacchetto cotto** dove l'issue ha visto il difetto: la resa del pacchetto non e' stata misurata.

## Le catture

- `pie-1600x900-prima-2026-10-27.jpg`: `main`, finestra 1600x900. Il prompt in Roboto Bold 24 bianco e' **tagliato dal bordo inferiore**, se ne vede la parte alta.
- `pie-1600x900-dopo-2026-10-27.jpg` e `pie-1920x1080-dopo-2026-10-27.jpg`: layout finale. Il prompt, in Exo 2 SemiBold 12 con la tinta del testo corrente dell'HUD, e' **intero**, a circa 20 px dal bordo, allineato all'etichetta COMUNI, con «unità» accentato.
- **Non c'e' un "prima" a 1920x1080**: la misura qui sotto dice che a quella scala il difetto e' piu' grande, non l'ho catturato.

## La misura che spiega il taglio, e che ha corretto il mio primo ritocco

Il dock sta in `Zone_Bottom`, alta `Bands().Bottom - 2 * ZoneMargin` = 175,6 unita' Slate (`RTHudLayout::ZoneLayout`). Il test `RefactorTactics.ScreenHud.ActionDockWithItsPromptFitsTheBottomZone` costruisce il dock vero con gli slot, scrive il testo del prompt e misura con Slate **a scala 1,0**:

| blob del dock | barra | prompt | dock | zona |
|---|---|---|---|---|
| `main` (Roboto Bold 24) | 163 | 39 | 202 | 175,6 |
| primo ritocco (Exo 2 12, padding barra 20+22) | 163 | 20 | 185 | 175,6 |
| finale (Exo 2 12, padding barra 12+12) | 145 | 20 | 167 | 175,6 |

- **Il primo ritocco non bastava a 1920x1080**, e la PIE a 1600x900 lo nascondeva: a scala 0,833 il carattere si arrotonda e il dock torna nella zona (barra 130 px, cioe' 156 unita'). **Il bilancio in unita' Slate NON e' indipendente dalla risoluzione**: la mia prima frase, "a 1920x1080 il taglio e' lo stesso", era un'inferenza dalla sola misura a 1600x900, ed era sbagliata.
- Il margine finale e' di circa 8 unita' a scala 1,0, e e' maggiore a scale minori.

## Limiti

- Un solo prompt catturato. Gli altri hanno lo stesso widget e lo stesso stile, e il test di bilancio li misura tutti.
- La misura headless non e' la resa: il carattere reale e l'arrotondamento a ogni scala sono della PIE e del pacchetto.
- Il padding verticale della barra (20+22 → 12+12) e' una scelta di layout dell'assistente: era l'unica altezza libera. Alternative non prese: alzare la fascia (`Bands().Bottom`, decisione del layout) o far stare il prompt dentro il pannello.
