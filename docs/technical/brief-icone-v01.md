# Brief icone della v0.1 — cosa disegnare per CP 20.2

> **Tipo**: brief di produzione asset · **Creato**: 2026-08-12 · **Owner della domanda**: *quali icone servono,
> e come si chiamano*.
>
> **Cosa è**: l'elenco delle chiavi che il catalogo della v0.1 deve coprire, con il nome file atteso.
> **Cosa non è**: la fonte. La fonte è `URTIconLibrary::RequiredIconIds()`, che le **deriva dai dati di gioco**
> — catalogo azioni, tag di stato, fasi del turno, roster eroi. L'elenco qui sotto è una **fotografia**: se
> diverge, ha ragione il codice.

Regola di D-031: nessun widget referenzia una texture. Il gameplay produce una **chiave**, la UI la risolve nel
catalogo. Rinominare una chiave costa quanto rinominare un'azione a catalogo.

## Come rimisurare l'elenco

Le chiavi non si contano a mano. Il test `RefactorTactics.IconCatalog.V01CategoriesPopulated` le pretende, e
`RefactorTactics.IconCatalog.RequiredIdsFollowGameData` verifica che seguano i dati di gioco. Se aggiungi
un'azione al catalogo o un tag `Status.`, la copertura cade finché l'icona non esiste — che è il solo modo
perché questo elenco resti vivo.

## Convenzione

| | |
|---|---|
| Chiave | `UI.Icon.<Categoria>.<Nome>` |
| Cartella | `/Game/RT/UI/Icons/` (`convenzioni-contenuti-ue.md` §UI) |
| Nome file | `T_UI_Icon_<Categoria>_<Nome>` — i punti diventano underscore |

Il prefisso `UI.Icon.` **non è decorazione**: senza, `Status.Wet` come icona e `Status.Wet` come Gameplay Tag
sarebbero la stessa stringa in un log, e D-031 li vuole concetti distinti.

## Colore

> **Recepito qui il 2026-08-12** da `docs/src/design/icon/visual-language/02-color-system.md`, che dichiara
> di essere «sorgente di design, non canone». `docs/src/` diventa vincolante solo quando un owner
> documentale lo consuma: questa sezione è quell'atto per i token **semantici**. I token **chrome**
> (`RT_UI_*`) restano di [`progettazione-hud.md`](progettazione-hud.md) §32 e non si riscrivono qui.

### La regola che governa tutte le altre

**Il colore è il secondo canale. La silhouette è il primo.** Ogni icona deve restare distinguibile in
grayscale: il grayscale non è un tema, è il test di accettazione. Un'icona che funziona solo a colori non è
accettata, e `Critical` (`#FF4D4D`) è **solo rinforzo** — non porta mai un segnale da solo.

### Token semantici

Base **Okabe-Ito**, scelta perché distinguibile per costruzione sotto protanopia, deuteranopia e tritanopia.

| Token | HEX | Usato da |
|---|---|---|
| `RT_Sem_Movement` | `#009E73` | `Action.Move` · `Action.Sprint` · `Action.Dash` · `Action.Leap` |
| `RT_Sem_Attack` | `#D55E00` | `Action.BasicAttack` · `Action.Charge` |
| `RT_Sem_Utility` | `#0072B2` | `Action.Interact` |
| `RT_Sem_Defense` | `#009E73` | `Action.Guard` · `Status.Guarded` |
| `RT_Sem_Reaction` | `#CC79A7` | `Status.Braced` |
| `RT_Sem_Hazard` | `#E69F00` | `Status.Exposed` · `Status.Root` · `Status.Slow` · `Status.Marked` |
| `RT_Sem_Electric` | `#F0E442` | `Status.Electrified` — **outline scuro obbligatorio** |
| Water | `#0072B2` | `Status.Wet` |
| Fire | `#D55E00` | `Status.Burning` |
| Ally / Enemy | `#56B4E9` / `#E69F00` | `Identity.Ally` · `Identity.Enemy` |

### Perché non verde e rosso puri

La richiesta originale era `Movement → green` e `Attack → red`. **Le associazioni sono mantenute, gli hex
no**: `#009E73` è un verde virato al blu, `#D55E00` un rosso virato all'arancio. Un verde puro e un rosso
puro convergono sotto deuteranopia e protanopia, e Movement e Attack sono **adiacenti nella stessa skill
bar** — il punto in cui la confusione costa di più.

Non basta da solo: Movement e Attack restano distinti soprattutto per **forma** — percorso con nodi contro
reticolo con impatto.

### Le due collisioni di hex

Due coppie condividono lo stesso valore. La prima è dichiarata e accettata, la seconda no.

**`Movement` e `Defense` = `#009E73`** — accettabile perché non competono mai nella stessa decisione: un
profilo di movimento e una postura difensiva stanno in **gruppi diversi** della skill bar. Se il playtest
mostra confusione, si separa **`Defense` prima di `Movement`**. Sono due token distinti che oggi hanno lo
stesso valore, non un token preso in prestito.

⚠️ **`Ice` e `Ally` = `#56B4E9`** — questa collisione **non è dichiarata** nei vincoli della sorgente, ed è
più esposta della prima: un'unità alleata su una superficie ghiacciata mette i due colori **nella stessa
vista**, mentre Movement e Defense sono separati dal layout. Non blocca la v0.1 — **non esiste uno stato
`Ice`**: i tag sono undici (`Root, Slow, Reveal, Exposed, Guarded, Marked, Wet, Braced, Burning, Obscured,
Electrified`) e nessuno è ghiaccio. Va risolta **prima** che un'icona `Ice` venga disegnata, non dopo.

### Cosa il colore non decide

- **`Certainty`** non usa il colore come canale: la grammatica è già in
  [`../roadmap/roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) §11.2 — confermato = linea piena · previsto =
  tratteggiata + icona di squadra · incerto = dissolto + `?`.
- **Gli elementi non cambiano per squadra.** Un fuoco alleato è dello stesso colore di un fuoco nemico: a
  cambiare è chi lo ha causato, non che cosa è.
- **Il colore di fazione non indica mai `Ally`/`Enemy`.** Sono due assi distinti.
- **`Ally` e `Enemy` differiscono per forma anche in monocromia**, non solo per tinta.

### Rimaste senza token

Non hanno un token nella sorgente e **non si inventano qui**: `Phase.*` (quattro), `Action.Wait`,
`Status.Reveal`, `Status.Obscured`, e i quattro eroi di `Identity`. Per la prima passata restano neutre
(`RT_UI_White` / testo secondario) e si distinguono per silhouette — che è comunque il primo canale.

## Le 33 chiavi

### Phase — 4

Le quattro fasi **volontarie**. `Planning` e `Cleanup` non sono fasi in cui il giocatore agisce, e la reaction
non è una quinta fase: darle un'icona qui la trasformerebbe in una.

`Prep` · `Dash` · `Blast` · `Move`

### Action — 9

Ogni azione che il catalogo generico dichiara davvero. Non la lista canonica dei documenti: se
`Action.Overwatch` non è ancora in codice, pretenderne il disegno significherebbe chiedere un'icona per
qualcosa che nessuno può pianificare.

`Sprint` · `Wait` · `Move` · `BasicAttack` · `Guard` · `Interact` · `Dash` · `Charge` · `Leap`

> ⚠️ **`Move` e `Dash` compaiono due volte, in categorie diverse, e sono due disegni distinti.**
> `UI.Icon.Phase.Move` è *il momento del turno in cui il movimento risolve*; `UI.Icon.Action.Move` è *la scelta
> che il giocatore dichiara*. Compaiono in posti diversi dell'interfaccia — la prima in una timeline, la
> seconda in una barra di azioni — e riusare lo stesso disegno renderebbe illeggibile la differenza fra «sto
> pianificando un movimento» e «siamo nella fase di movimento».

### Status — 11

I tag registrati sotto `Status.` in `Core/RTGameplayTags.cpp`: la stessa sorgente che il gameplay applica.

| Chiave | Cosa significa |
|---|---|
| `Root` | non può muoversi — ⚠️ **non è la radice dell'albero dei tag**, è l'immobilizzazione |
| `Slow` | range di movimento dimezzato |
| `Reveal` | intento visibile agli avversari |
| `Exposed` | scoperta: +5 al primo danno diretto |
| `Guarded` | in guardia |
| `Marked` | marcato |
| `Wet` | bagnato |
| `Braced` | ha armato una reazione |
| `Burning` | in fiamme |
| `Obscured` | oscurato |
| `Electrified` | elettrificato |

### Certainty — 3

I tre livelli di CP 11.2. La grammatica visiva è già decisa in `roadmap-v0.1.md` §11.2 e **le icone devono
accordarsi con essa**, non sostituirla: confermato = linea piena · previsto = tratteggiata + icona di squadra ·
incerto = dissolto + `?`.

`Confirmed` · `Predicted` · `Uncertain`

### Identity — 6

I quattro eroi del roster, più la relazione di squadra.

`Flux` · `Riva` · `Bastion` · `Vektor` · `Ally` · `Enemy`

> ⚠️ **Il prefisso è tradotto.** Gli `HeroId` in codice sono `Hero.Flux`, non `Flux`: la chiave dell'icona è
> `UI.Icon.Identity.Flux`, perché il validator confronta il segmento di categoria dentro l'ID con la categoria
> dichiarata, e `UI.Icon.Hero.Flux` verrebbe rifiutato.

`Ally` e `Enemy` sono **relazione, non personaggi**: restano due anche quando il roster cresce. Il consumatore
esiste già — `ARTHUD` colora gli intenti per squadra leggendo `View.bIsAlly`.

## Le sette categorie che NON si disegnano

`Environment` · `MapInteraction` · `Information` · `Reaction` · `Coordination` · `Warning` · `Objective`

Sono **dichiarate e vuote** di proposito: fissano la tassonomia senza produrre asset che nessuno consuma. Una
categoria senza chiavi è legale; una **chiave** senza asset è un errore di validazione. Il test
`V01CategoriesPopulated` verifica entrambe le direzioni, quindi aggiungere una chiave a una di queste sette lo
fa diventare rosso — è voluto: significa che sta arrivando un sistema che la consuma, e la sua icona va
decisa allora.

## Il missing-icon

Oltre alle 33 serve **un'icona in più**: `MissingIcon`, obbligatoria nel catalogo. È quella che `ResolveIcon`
restituisce quando una chiave non si risolve, e senza di lei il validator rifiuta il catalogo. Deve essere
riconoscibile a colpo d'occhio come *«qui manca qualcosa»*, non un quadrato neutro che si confonde con
un'icona vera.

## Cosa serve dopo il disegno

Due passi **in Editor**, che nessuno script del repository può fare al posto di una persona:

1. importare le texture in `/Game/RT/UI/Icons/`;
2. creare `DA_IconCatalog` (`URTIconCatalogData`), popolare `Icons` con le 33 voci e impostare `MissingIcon`.

Solo allora `FindMissingRequiredIcons` sul catalogo **spedito** è vuoto, che è la parte del DoD di CP 20.2 che
richiede l'asset. La parte in codice — le chiavi derivate e i test che le pretendono — non la aspetta.
