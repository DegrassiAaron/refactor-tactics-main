# Visual Language — Accessibilità

> **Statuto**: sorgente di design, non canone. Vedi [`01-principi.md`](01-principi.md).
>
> **Owner dei requisiti**: [`progettazione-hud.md`](../../../../technical/progettazione-hud.md) §47-bis
> definisce i vincoli di accessibilità dell'interfaccia — colore non unico canale, finestra di reazione,
> leggibilità, scala UI, rimappatura, reduced motion. Qui c'è **solo la parte che riguarda l'asset**: come si
> produce e come si verifica un glifo.

## 1. Il test è meccanico

Un glifo è accettato quando **supera il grayscale**, non quando piace a colori. L'ordine di produzione lo
riflette: si consegna prima in monocromia, e solo dopo l'approvazione della silhouette si aggiungono i temi.

> Se i glifi non sono distinguibili in grayscale, **non si corregge il problema col colore**.

È la regola di review del pack, e resta la più importante del documento.

## 2. Le quattro rese

Ogni batch si verifica in quattro rese, senza cambiare geometria né layout.

| Resa | Che cosa verifica |
|---|---|
| Default Accessible | la palette di [`02-color-system.md`](02-color-system.md) |
| Grayscale | che il **primo canale** (silhouette, pattern) basti da solo |
| CVD | protanopia, deuteranopia, tritanopia |
| High Contrast | separazione bordo/fondo, riduzione delle mezze tinte |

CVD non significa ruotare la tinta: significa rafforzare **forma, pattern, outline e luminanza**. High
Contrast è un tema **separato**, non una versione più satura di CVD.

## 3. Dimensioni reali

Si verifica a dimensione reale, non ingrandito. Un glifo che funziona al 400% e sparisce a 24 px è un glifo
che non funziona.

`16 px` · `20 px` · `24 px` · `32 px` · `48 px` · slot ability `56–60 px` · strip HUD a `1920×1080`.

**1080p è la baseline, non il caso limite.**

## 4. Fondi di tortura

Un'icona vive sopra il campo di battaglia, non sopra un artboard bianco. Ogni glifo va verificato su:

nero · bianco · grigio medio · erba · cemento · metallo · acqua · fuoco · ghiaccio · fumo · screenshot reale
di gioco affollato.

Il fondo peggiore è di solito acqua o fuoco: hanno luminanza alta e struttura, e mangiano gli stroke sottili.

## 5. Densità

L'accessibilità di un sistema iconografico non si misura su un'icona sola. Si misura sulla schermata piena:

8 unità visibili · 2 status per unità più `+N` · più intenti alleati · 3 warning · un obiettivo attivo · una
reazione armata · un prompt di Fast Reaction.

Se in quella condizione il giocatore non distingue **selected da planned**, **planned da cooldown**,
**cooldown da unavailable**, **reaction armed da reaction opportunity**, **predicted da uncertain** entro
circa un secondo, il problema è il sistema, non la singola icona.

## 6. Che cosa verifica una macchina, e che cosa no

Aggiunto il **2026-08-12** dopo una revisione che ha trovato il difetto opposto a quello atteso: la
specifica descriveva un sistema validabile da una macchina, e nessuna macchina lo validava.

`scripts/build-icon-assets.py --check` verifica **tre** cose, e fallisce con exit 1:

| Criterio | Come |
|---|---|
| ogni `IconId` prodotto è richiesto da una fonte del gioco | ricostruisce `RequiredIconIds()` dal catalogo azioni, dai tag registrati e dalle fasi |
| il catalogo dichiara `MissingIcon` | senza, il validator Unreal rifiuta il catalogo |
| gli asset sono rigenerabili dal sorgente | confronto byte a byte |

Il primo è quello che conta: senza, `UI.Icon.Status.Rooted` — il tag registrato è `Status.Root` — passerebbe
sia questo gate sia il validator Unreal, e **non risolverebbe mai** a runtime. Entrambi i controlli sono
stati verificati per mutazione: rompendoli uno per volta, il gate cade.

Restano **giudizio umano**, e non c'è modo onesto di automatizzarli: la leggibilità a 24 px, la
distinguibilità di una coppia senza label, il numero di componenti percepiti come dominanti, e se un glifo
evochi per caso una convenzione estranea — è così che sono emerse le somiglianze con *refresh*,
*equalizzatore* e *papillon*, tutte trovate guardando una tavola, nessuna deducibile dal codice.

Per questi vale la calibration sheet, non il gate.

## 7. Che cosa questo documento non promette

Non è una dichiarazione di conformità WCAG né un audit. È l'insieme dei vincoli che il progetto si dà per la
produzione degli asset. **Nessuno di questi punti è stato misurato su un giocatore reale**: la verifica di
leggibilità appartiene al playtest di **E21**, insieme al resto della presentazione.
