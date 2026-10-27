# #3869 - il prompt del dock: misura e catture (2026-10-27)

Evidenza **dell'assistente**, non un giudizio dell'autore. Riguarda [#3869](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3869).

## Che cosa e' stato catturato, e come

- **PIE nell'Editor** (`NetMode: Standalone`), finestra flottante **1600x900**, partita avviata, nessuna selezione: il prompt mostrato e' quello di `IdleSelection`.
- Cattura con `PrintWindow` della finestra per titolo, dal clone principale, **senza inviare input** alla PIE. Il client e' 1606x940: il viewport e' 1600x900, il resto e' la barra del titolo.
- **Non e' il pacchetto cotto** dove l'issue ha visto il difetto: la resa del pacchetto non e' stata misurata.

## Le catture

- `pie-1600x900-prima-2026-10-27.jpg`: il prompt in Roboto Bold 24 bianco e' **tagliato dal bordo inferiore**, se ne vede la parte alta.
- `pie-1600x900-dopo-2026-10-27.jpg`: Exo 2 SemiBold 12, tinta del testo corrente dell'HUD, **intero** («Seleziona un'unità», con l'accento che il font disegna).

## La misura che spiega il taglio

Il dock sta in `Zone_Bottom`, alta `Bands().Bottom - 2 * ZoneMargin` unita' Slate (`RTHudLayout::ZoneLayout`). A scala UI 0,833 (900 / 1080):

- la barra delle azioni e' alta 130 px, cioe' 156 unita' Slate;
- restano circa 17 px, cioe' circa 20 unita', sotto la barra;
- il prompt a 24 ne voleva circa 28; a 12, circa 14.

Il bilancio e' in unita' Slate e non dipende dalla risoluzione: a 1920x1080 il taglio e' lo stesso.

## Limiti

- Un solo prompt catturato. Gli altri hanno lo stesso widget e lo stesso stile.
- Il margine dopo la correzione e' di circa mezza riga: se la barra delle azioni cresce, il prompt torna a sforare e nessun test lo dice.
