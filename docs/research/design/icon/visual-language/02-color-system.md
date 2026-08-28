# Visual Language — Color System

> **Statuto**: sorgente di design, non canone. Vedi [`01-principi.md`](01-principi.md).
>
> **Confine**: la palette del **chrome** dell'HUD — fondi, pannelli, bordi, accenti di stato — è di
> [`progettazione-hud.md`](../../../../technical/systems/progettazione-hud.md) §32 e non si riscrive qui. Questo
> documento definisce la palette **semantica**: i colori che portano un significato di gioco.

## 1. La regola che governa tutte le altre

Il colore è il **secondo** canale. Ogni informazione codificata a colore deve restare leggibile senza,
attraverso silhouette, bordo, pattern, simbolo, direzione o forma.

La verifica è meccanica: **portare la tavola in scala di grigi e rileggerla**. Se un'informazione sparisce,
manca il secondo canale. È lo stesso test di
[`progettazione-hud.md`](../../../../technical/systems/progettazione-hud.md) §47-bis.1, applicato all'asset invece
che alla schermata.

## 2. Palette semantica

Base **Okabe-Ito**, scelta perché è distinguibile per costruzione sotto protanopia, deuteranopia e
tritanopia — non perché sia gradevole.

> 🔴 **SUPERATA dal 2026-08-28 — [D-232](../../../../decisions/RT_PDR_00_Decision_Log.md): il colore dice la
> FASE, non la famiglia.** La tabella qui sotto resta come **provenienza** e per i suoi valori Okabe-Ito, ma
> non governa più il colore delle icone d'azione. Il colore ha ora **quattro** valori — le macro-fasi
> `Prep · Dash · Blast · Move` che `RequiredIconIds()` deriva dalle `VoluntaryPhases` — non otto.
>
> ✅ **La palette nuova è [D-233](../../../../decisions/RT_PDR_00_Decision_Log.md)** e riusa quattro HEX di
> questa stessa tabella: `Prep #56B4E9` (era `Defense`) · `Dash #009E73` (era `Movement`) ·
> `Blast #D55E00` (era `Attack`) · `Move #0072B2` (era `Utility`). Nessun colore nuovo entra nel sistema.
> ⛔ Il giallo `#F0E442` è stato **escluso**: dista `ΔE 18.3` dall'ambra di `Selected`, e un colore di fase
> non deve somigliare a uno stato.
>
> ⚠️ **Costo misurato del cambio**: **119** icone del generatore portano oggi un token di famiglia, e la
> mappatura non è iniettiva — sulla fase di risoluzione, `Control` raccoglie quattro famiglie (`Hazard` 6,
> `Reaction` 3, `Defense` 2, `Utility` 1) che diventano dello stesso colore, mentre `Attack` come famiglia si
> sparge su tre fasi. **La famiglia deve ora vivere nella silhouette**, che è la regola «pattern prima del
> colore» portata alle sue conseguenze. La ricolorazione è lavoro di **E25**.
>
> ✅ Le sezioni **Elemento**, **Relazione** e i temi CVD/High Contrast **non** sono toccate.

| Famiglia | Token | HEX | Uso |
|---|---|---|---|
| Movement | `RT_Sem_Movement` | `#009E73` | `Move`, `Sprint`, `Dash`, reposition |
| Attack | `RT_Sem_Attack` | `#D55E00` | `BasicAttack`, damage, offensive |
| Utility | `RT_Sem_Utility` | `#0072B2` | `Interact`, support, informazione |
| Control / Hazard | `RT_Sem_Hazard` | `#E69F00` | controllo, pericolo, `Status.Exposed` |
| Reaction | `RT_Sem_Reaction` | `#CC79A7` | `Overwatch`, ciclo di reazione |
| Electric / Follow-up | `RT_Sem_Electric` | `#F0E442` | `Electric`, catene, effetti a seguire |
| Defense | `RT_Sem_Defense` | `#56B4E9` | `Guard`, `Brace`, `Shield`, `Cover` |
| Disabled | `RT_Sem_Disabled` | `#6B7280` | non disponibile, esaurito |

Elementi con colore proprio, indipendente dalla squadra:

| Elemento | HEX |
|---|---|
| Water | `#0072B2` |
| Fire | `#D55E00` |
| Electric | `#F0E442` |
| Ice | `#56B4E9` |

Relazione di squadra:

| Relazione | HEX |
|---|---|
| Ally | `#56B4E9` |
| Enemy | `#E69F00` |

### 2.1 Perché non verde e rosso puri

La richiesta originale era `Movement → green` e `Attack → red`. Le **associazioni** sono mantenute; gli
**hex** no. Un verde puro e un rosso puro convergono sotto deuteranopia e protanopia, e Movement e Attack
sono adiacenti nella stessa skill bar: è il punto in cui la confusione costa di più.

`#009E73` è un verde virato al blu e `#D55E00` un rosso virato all'arancio. Restano leggibili come «verde» e
«rosso» a vista normale, e conservano separazione di **hue e luminanza** sotto CVD.

Questo non basta da solo. Movement e Attack restano distinti soprattutto per forma: percorso con nodi contro
reticolo con impatto.

### 2.2 Vincoli

> ⚠️ **Corretto il 2026-08-12.** La prima stesura assegnava `#009E73` **sia** a `Movement` sia a `Defense`,
> motivandolo con «non competono mai nella stessa decisione: stanno in gruppi diversi della skill bar». La
> premessa è **falsa**, ed è falsificata da [`progettazione-hud.md`](../../../../technical/systems/progettazione-hud.md)
> §6.7 — dove `Movement` (`Move` · `Sprint` · `Dash`) e `Defense` (`Guard` · `Brace`) sono **due corsie della
> stessa Action Dock**, viste nello stesso momento. `Defense` passa a `#56B4E9`.

- `Defense` (`#56B4E9`) e `Ally` (`#56B4E9`) condividono lo hex, e questa volta la separazione regge davvero:
  una relazione di squadra tinge un **marker di unità**, una famiglia d'azione tinge uno **slot**. Non
  compaiono mai sulla stessa superficie. Se un giorno lo faranno, il primo a spostarsi è `Defense`.
- `Brace` è **difesa**, non reazione: è un'azione generica di D-025, non un ramo del ciclo di reazione. La
  prima stesura la elencava sotto `Reaction` per associazione col nome della corsia HUD.
- `Electric` (`#F0E442`) ha luminanza alta: richiede **outline scuro obbligatorio** su fondo chiaro.
- `Fire`, `Water`, `Electric` e `Ice` **non cambiano** in base alla squadra. Un fuoco alleato è dello stesso
  colore di un fuoco nemico: a cambiare è chi lo ha causato, non che cosa è.
- Il colore di **fazione** non indica mai `Ally`/`Enemy`. Sono due assi distinti, e il badge di fazione è
  secondario nell'HUD di combattimento.
- `Critical` (`#FF4D4D`, da §32 dell'HUD) è **solo rinforzo**: non porta mai un segnale da solo.

## 3. Temi

Tre temi, stessa geometria e stessa semantica. Cambia il rendering, mai il layout.

| Tema | Che cosa cambia |
|---|---|
| Default Accessible | la palette qui sopra |
| CVD | rinforza **pattern, outline e luminanza**; non si limita a ruotare la tinta |
| High Contrast | separato da CVD: alza il contrasto di bordo e fondo, riduce le mezze tinte |

Il **grayscale** non è un tema: è il test di accettazione di tutti e tre.
