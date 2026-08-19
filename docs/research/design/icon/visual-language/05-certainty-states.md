# Visual Language — Certainty states

> **Statuto**: sorgente di design, non canone. Vedi [`01-principi.md`](01-principi.md).
>
> **Owner della semantica**: [`progettazione-hud.md`](../../../../technical/progettazione-hud.md) §16
> definisce che cosa significano i tre stati e quando si applicano. Qui c'è **solo come si disegnano**.

## 1. Tre assi separati

`Validity`, `Certainty` e `Knowledge` sono assi indipendenti e non vanno collassati su un unico canale
visivo. Un bersaglio può essere **valido** e **incerto**; un percorso può essere **certo** e **invalido**.

| Asse | Domanda | Valori |
|---|---|---|
| Validity | l'azione si può fare? | valida · invalida |
| Certainty | quanto è certo ciò che vedo? | `Confirmed` · `Predicted` · `Uncertain` |
| Knowledge | che cosa la squadra sa? | `Allowed` · `CellOnly` · `Rejected` |

`Knowledge` non è documentazione: è `ERTTargetKnowledge` in `Source/RefactorTactics/Perception/RTTeamKnowledge.h`,
e `CellOnly` significa che si può mirare alla **cella**, mai all'unità. Un'icona che mostra una silhouette
nemica su una cella esatta quando la conoscenza è `CellOnly` **mente**, e la bugia non è estetica: suggerisce
un bersaglio che il gioco rifiuterà.

## 2. I quattro renderer

Sono **style modifier** applicati sopra un glifo, non icone duplicate per ogni oggetto.

| Stato | Bordo | Fill | Opacità | Marca |
|---|---|---|---|---|
| `Confirmed` | solido | normale | piena | nessuna |
| `Predicted` | tratteggiato | hollow / basso | ghost | micro-marker di intento, opzionale |
| `Uncertain` | puntinato / discontinuo | fade | ridotta | `?` secondario |
| `Invalid` | slash / cross-hatch / `⊘` | muted neutro | — | accento critical opzionale |

La distinzione deve reggere **senza colore**. È il test più stretto del sistema, perché i tre stati compaiono
spesso nella stessa schermata e sullo stesso tipo di oggetto.

### 2.1 La regola dell'area

Se la posizione è incerta, si rappresenta un'**area**, non una cella esatta con un bordo tratteggiato. Un
marker preciso reso «incerto» dallo stile comunica comunque una precisione che il dato non ha — e il
giocatore legge la geometria prima dello stile.

## 3. Disabled non è Uncertain

`Disabled` dice «questa cosa esiste ma non è disponibile ora». `Uncertain` dice «non so dove o se».
Confonderli produce un HUD in cui il giocatore non capisce se deve aspettare o cercare.

`Disabled`: desaturazione, frame dedicato, glifo di stato — **silhouette ancora leggibile**. Non si ottiene
abbassando l'opacità al 10–15%: a quel punto non è disabilitato, è illeggibile.

## 4. Nel catalogo

`Certainty` è una delle dodici categorie canoniche e le sue voci sono chiavi regolari:

```text
UI.Icon.Certainty.Confirmed
UI.Icon.Certainty.Predicted
UI.Icon.Certainty.Uncertain
```

Il manifest sorgente del pack le colloca sotto `UI.Style.Certainty.*`: quella forma **non passa il
validator**, che pretende il prefisso `UI.Icon.<Categoria>.`. Vedi
[`08-catalogo-v0.1.md`](08-catalogo-v0.1.md) §4.

`Invalid` non è una `Certainty`: appartiene all'asse `Validity` e nella v0.1 si esprime come mask
riutilizzabile, non come voce di catalogo.
