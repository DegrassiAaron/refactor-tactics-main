# Visual Language — Export e naming

> **Statuto**: sorgente di design, non canone. Vedi [`01-principi.md`](01-principi.md).
>
> **Owner della pipeline Unreal**: [`progettazione-hud.md`](../../../../technical/systems/progettazione-hud.md)
> §41–§44 — che cosa produrre come asset 2D, che cosa no, struttura di `Content/RT/UI/`. Qui c'è il naming
> **semantico** e i formati di consegna.

## 1. Due nomi, due scopi

Un'icona ha due identificatori e non sono intercambiabili.

| | `IconId` | Nome asset Unreal |
|---|---|---|
| Forma | `UI.Icon.<Categoria>.<Nome>` | `RT_UI_<Category>_<Name>_<State>` |
| Esempio | `UI.Icon.Status.Wet` | `RT_UI_Icon_Status_Wet` |
| Chi lo usa | il gameplay, i widget, il combat log, la wiki | il content browser, il packaging |
| Chi lo risolve | `URTIconLibrary` | Unreal |

Il gameplay produce **sempre** un `IconId`, mai un percorso di asset. È il punto di
[D-031](../../../../decisions/RT_PDR_00_Decision_Log.md): il giorno in cui `Status.Wet` cambia disegno, o
serve una variante ad alto contrasto, la modifica è una riga di dato invece di un refactor di ogni widget.

### 1.1 Perché il prefisso `UI.Icon.`

Senza, `Status.Wet` come **icona** e `Status.Wet` come **Gameplay Tag** sarebbero la stessa stringa dentro un
log. Il prefisso non è decorazione: tiene separati due concetti che D-031 vuole distinti.

### 1.2 Il vincolo verificato dalla macchina

`URTIconLibrary::ValidateIconCatalog` confronta la categoria **dichiarata** nel dato con il segmento dentro
l'`IconId`, usando `StartsWith` sul prefisso `UI.Icon.<Categoria>.`.

Ne discendono tre conseguenze operative:

1. `<Categoria>` deve essere **esattamente** un valore di `ERTIconCategory` — l'enum a dodici voci. `Map` non
   vale: il valore è `MapInteraction`. `Intel` non vale: il valore è `Information`.
2. `<Nome>` **può contenere punti**, quindi `UI.Icon.Action.Hero.Gadget.LinearDischarge` è valido.
3. Una chiave duplicata è un **errore di validazione**, non un'icona che ne sovrascrive un'altra in silenzio.

Una chiave senza icona è a sua volta un errore di validazione, non un widget vuoto.

## 2. Export

### Preferito

- **SVG / master vettoriale** per ogni glifo;
- **PNG RGBA** a `16 / 20 / 24 / 32 / 48 px`;
- file sorgente di design;
- artboard PNG di review a 2x/4x.

### Regole PNG

Alpha pulito, nessun matte, crop e padding coerenti, tintabile dove possibile, glow su layer separato se
serve.

**Niente testo incorporato.** Mai rasterizzare numeri di cooldown, keybind, costi o percentuali dentro un
glifo: sono testo dinamico, e un'icona che li contiene diventa una texture per ogni valore possibile.

Il master è **monocromatico** e tintabile. I temi si applicano sopra, non si esportano come glifi separati.

## 3. Scheda di consegna

Ogni asset statico dichiara:

```text
AssetName
Category
NativeSize
9SliceMargins        (se applicabile)
Tintable             yes / no
Alpha mode
Expected UMG usage
Expected world-space usage
CVD-safe             yes / no
HighContrast variant yes / no
```

## 4. Vocabolario di stato

Quando un asset esiste in più stati, i suffissi sono questi e non sinonimi:

`Available` · `Hover` · `Selected` · `Planned` · `Cooldown` · `Unavailable` · `Invalid` · `Warning` ·
`Armed`

`Armed` vale solo per gli slot di reazione. `Unavailable` e `Invalid` non sono la stessa cosa: il primo dice
«non ora», il secondo «non così».
