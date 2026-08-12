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
