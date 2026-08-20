# Brief icone della v0.1 — cosa disegnare per CP 20.2

> **Tipo**: brief di produzione asset · **Creato**: 2026-08-12 · **Owner della domanda**: *quali icone servono,
> come si chiamano e di che colore sono*.
>
> ⚠️ **Le due metà di questo documento hanno statuto diverso, e confonderle è un errore.**
>
> | | **Le chiavi** (§Le chiavi richieste) | **Il colore** (§Colore) |
> |---|---|---|
> | La fonte è | `URTIconLibrary::RequiredIconIds()`, che le **deriva dai dati di gioco** — catalogo azioni, tag di stato, fasi del turno, roster eroi | **questo documento**: nessun `RequiredColorTokens()` esiste in codice |
> | Se diverge | ha ragione **il codice**: l'elenco qui è una fotografia | ha ragione **questa pagina**, ma vedi il vincolo qui sotto |
>
> ⚠️ **Il colore è provvisorio finché la sua sorgente non è versionata.** I token `RT_Sem_*` sono recepiti da
> un file che non è in nessun branch, e la DoD dell'issue di riconciliazione lo dice: un sorgente non
> versionato «non può essere citato come specifica». Questa sezione lo cita comunque — consapevolmente,
> perché il batch della v0.1 non può aspettare — ma i valori si rileggono quando quella issue chiude.

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

> **Recepito qui il 2026-08-12** da `docs/research/design/icon/visual-language/02-color-system.md`, che dichiara
> di essere «sorgente di design, non canone». `docs/src/` diventa vincolante solo quando un owner
> documentale lo consuma: questa sezione è quell'atto per i token **semantici**. I token **chrome**
> (`RT_UI_*`) restano di [`progettazione-hud.md`](../systems/progettazione-hud.md) §32 e non si riscrivono qui.
>
> ⚠️ **La sorgente si è mossa durante il recepimento**, e la prima stesura di questa sezione ha recepito una
> versione superata **trentasei secondi** prima che venisse corretta. Quello che segue è lo stato del
> **2026-08-12 ore 11:06**, e il modo di accorgersi di una deriva è confrontare i valori con
> `progettazione-hud.md`.
>
> ✅ **La clausola «la sorgente non è versionata» è scaduta.** Diceva che `docs/src/design/icon/` risultava
> untracked e che ogni recepimento restava una fotografia «finché quel materiale non entra nel repository».
> Quel materiale **è** nel repository — misurato: `git ls-files docs/research/design/icon` elenca i
> documenti, e i 296 master iconografici stanno in `docs/generated/icons/` con il loro generatore
> (`scripts/build-icon-assets.py`). Riscritta il 2026-08-19 con la fase 2 di
> [#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165): una condizione d'innesco già
> scattata, che continuava a prescrivere un'attesa finita.

### La regola che governa tutte le altre

**Il colore è il secondo canale. La silhouette è il primo.** Ogni icona deve restare distinguibile in
grayscale: il grayscale non è un tema, è il test di accettazione. Un'icona che funziona solo a colori non è
accettata, e il rosso critico — `RT_UI_Red`, che [`progettazione-hud.md`](../systems/progettazione-hud.md) §32 possiede
— è **solo rinforzo**: non porta mai un segnale da solo. Il suo valore si legge lì, non qui.

### Token semantici

Base **Okabe-Ito**, scelta perché distinguibile per costruzione sotto protanopia, deuteranopia e tritanopia.

⚠️ **`RT_Sem_*` e le dodici categorie di D-031 sono due assi diversi che condividono qualche nome.**
`RT_Sem_Reaction` è una **famiglia di colore**; `Reaction` è una **categoria di chiave**, ed è una delle sette
che la v0.1 lascia vuote. Un token può tingere una chiave di un'altra categoria — `RT_Sem_Defense` tinge
`Status.Braced` — e la coincidenza di nome non implica nessun legame. Chi legge una tabella non deve dedurne
l'altra.

| Token | HEX | Usato da |
|---|---|---|
| `RT_Sem_Movement` | `#009E73` | `Action.Move` · `Action.Sprint` · `Action.Dash` · `Action.Leap` |
| `RT_Sem_Attack` | `#D55E00` | `Action.BasicAttack` · `Action.Charge` |
| `RT_Sem_Utility` | `#0072B2` | `Action.Interact` |
| `RT_Sem_Defense` | `#56B4E9` | `Action.Guard` · `Status.Guarded` · `Status.Braced` |
| `RT_Sem_Reaction` | `#CC79A7` | — *nessuna chiave della v0.1: il suo uso è `Overwatch`, che non è nel catalogo azioni* |
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

### Le collisioni di hex

**Tre token stanno su `#56B4E9`**: `Defense`, `Ally` e `Ice`. Delle tre coppie che ne derivano, una è
dichiarata e due no.

**`Defense` e `Ally` — dichiarata, e la separazione regge**: una relazione di squadra tinge un **marker di
unità**, una famiglia d'azione tinge uno **slot**. Non compaiono mai sulla stessa superficie. Se un giorno lo
faranno, il primo a spostarsi è `Defense`.

⚠️ **`Ice` con `Ally` e `Ice` con `Defense` — non dichiarate.** Un'unità alleata su una superficie ghiacciata
mette i primi due colori nella stessa vista; il terzo caso è più remoto ma non impossibile. Non blocca la
v0.1 per una ragione precisa: **non esiste uno stato `Ice`** — i tag sono undici (`Root, Slow, Reveal,
Exposed, Guarded, Marked, Wet, Braced, Burning, Obscured, Electrified`) e nessuno è ghiaccio. Va risolta
**prima** che un'icona `Ice` venga disegnata, non dopo.

> ⚠️ **`Movement` e `Defense` non condividono più `#009E73`, e la ragione vale più della correzione.** La
> prima stesura di questa sezione riportava quella collisione come «accettabile perché non competono mai
> nella stessa decisione: gruppi diversi della skill bar». La premessa è **falsa**, e a falsificarla è un
> documento **versionato**: [`progettazione-hud.md`](../systems/progettazione-hud.md) §6.7 mette `Move`, `Wait`,
> `Guard` e `Overwatch` nella **stessa lista** «Universal Actions» dell'Action Dock — cioè sotto gli occhi
> nello stesso momento. `Defense` è passato a `#56B4E9`.
>
> Il modo di non ripetere l'errore non è leggere meglio la sorgente di design: è **verificare la premessa
> contro il canone versionato** prima di accettarla. Qui la prova stava in un file del repository.

> ⚠️ **`Brace` è difesa, non reazione.** La prima stesura mappava `Status.Braced` su `RT_Sem_Reaction` per
> associazione col nome della corsia HUD. `Brace` è un'azione generica di D-025, non un ramo del ciclo di
> reazione — e D-047 lo conferma dal lato gameplay: *«`Brace` prepara una reazione; come si reagisce lo dice
> il Reaction Profile»*. Prepararla non è esserlo.

### Cosa il colore non decide

- **`Certainty`** non usa il colore come canale: la grammatica è già in
  [`../../roadmap/roadmap-v0.1.md`](../../roadmap/roadmap-v0.1.md) §11.2 — confermato = linea piena · previsto =
  tratteggiata + icona di squadra · incerto = dissolto + `?`.
- **Gli elementi non cambiano per squadra.** Un fuoco alleato è dello stesso colore di un fuoco nemico: a
  cambiare è chi lo ha causato, non che cosa è.
- **Il colore di fazione non indica mai `Ally`/`Enemy`.** Sono due assi distinti.
- **`Ally` e `Enemy` differiscono per forma anche in monocromia**, non solo per tinta.

### Rimaste senza token

Non hanno un token nella sorgente e **non si inventano qui**: `Phase.*` (quattro), `Action.Wait`,
`Status.Reveal`, `Status.Obscured`, e i quattro eroi di `Identity`. Per la prima passata restano neutre
(`RT_UI_White` / testo secondario) e si distinguono per silhouette — che è comunque il primo canale.

## Le chiavi richieste

> 🔁 **Si chiamava «Le 33 chiavi»**, rinominata il 2026-08-13. Il nome vecchio non è citato da nessun link né
> da nessun documento — verificato — e un titolo che porta un numero sbagliato è la copia più difficile da
> correggere, perché nessuno la legge come un dato.

> 🔴 **Non sono più 33: rimisurate il 2026-08-13, sono 60.** E il difetto non è nel numero — è nell'aver
> scritto **un numero** per un insieme che è una **funzione**.
>
> `URTIconLibrary::RequiredIconIds()` non legge questo documento: deriva l'insieme richiesto dalle sorgenti
> vive, e la sola che sia cresciuta è il catalogo azioni. Composizione misurata oggi:
>
> | Categoria | Qui sotto | Misurata | Da dove deriva |
> |---|---:|---:|---|
> | `Phase` | 4 | **4** | le quattro fasi volontarie, elencate in codice |
> | `Action` | 9 | **36** | `URTCatalogLibrary::GetCoreActionCatalog()` — `RTCatalogLibrary.cpp:709-1172` |
> | `Status` | 11 | **11** | i tag sotto `Status.` in `Core/RTGameplayTags.cpp` |
> | `Certainty` | 3 | **3** | tre costanti dichiarate in codice |
> | `Identity` | 6 | **6** | 4 eroi del roster + `Ally` + `Enemy` |
> | | **33** | **60** | |
>
> ```bash
> sed -n '709,1172p' Source/RefactorTactics/Ability/RTCatalogLibrary.cpp | grep -c 'Catalog.Add('   # 36
> ```
>
> ⚠️ **Conseguenza di scope, non nota a margine**: il gate di `#219` è `FindMissingRequiredIcons` **vuoto**, e
> quella funzione pretende **tutte** le chiavi derivate. Finché il catalogo azioni resta a 36, la v0.1 chiede
> **60 disegni + il missing-icon**, non 34. Se 60 sono troppi, la leva **non** è ritoccare questo elenco: è
> decidere quali azioni del catalogo entrano nell'insieme richiesto, e quella decisione va presa in `#219`.
>
> ⚠️ **Il titolo portava il numero, ed è stato rinominato.** L'argomento per tenerlo — *«serve a far
> riconoscere il documento a chi lo cita»* — è stato verificato e non regge: nessun link punta all'ancora e
> nessun documento cita la sezione per nome. L'unico riferimento interno, nella tabella in testa a questo
> file, è stato aggiornato con il rename. Un numero in un titolo è la copia peggiore: nessuno lo legge come
> un dato, quindi nessuno lo rimisura.

### Phase — 4

Le quattro fasi **volontarie**. `Planning` e `Cleanup` non sono fasi in cui il giocatore agisce, e la reaction
non è una quinta fase: darle un'icona qui la trasformerebbe in una.

`Prep` · `Dash` · `Blast` · `Move`

### Action — 9

Ogni azione che il catalogo generico dichiara davvero. Non la lista canonica dei documenti: se
`Action.Overwatch` non è ancora in codice, pretenderne il disegno significherebbe chiedere un'icona per
qualcosa che nessuno può pianificare.

`Sprint` · `Wait` · `Move` · `BasicAttack` · `Guard` · `Interact` · `Dash` · `Charge` · `Leap`

> ⚠️ **Queste nove erano il catalogo del giorno in cui la riga fu scritta; oggi ne dichiara 36.** La
> motivazione qui sopra — *«ogni azione che il catalogo generico dichiara davvero»* — è ancora quella giusta
> ed è la stessa del commento in `RTIconLibrary.cpp:46-48`. È l'**elenco** ad aver smesso di seguirla, perché
> è stato **trascritto** invece che derivato. L'elenco vivo:
>
> ```bash
> sed -n '709,1172p' Source/RefactorTactics/Ability/RTCatalogLibrary.cpp \
>   | grep -oE 'TEXT\("Action\.[A-Za-z0-9.]+"\)' | sort -u
> ```

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

`Gadget` · `Phase` · `Riktor` · `Wraith` · `Ally` · `Enemy`

> ⚠️ **Il prefisso è tradotto.** Gli `HeroId` in codice sono `Hero.Gadget`, non `Gadget`: la chiave dell'icona è
> `UI.Icon.Identity.Gadget`, perché il validator confronta il segmento di categoria dentro l'ID con la categoria
> dichiarata, e `UI.Icon.Hero.Gadget` verrebbe rifiutato.

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

Oltre alle chiavi richieste serve **un'icona in più**: `MissingIcon`, obbligatoria nel catalogo. È quella che `ResolveIcon`
restituisce quando una chiave non si risolve, e senza di lei il validator rifiuta il catalogo. Deve essere
riconoscibile a colpo d'occhio come *«qui manca qualcosa»*, non un quadrato neutro che si confonde con
un'icona vera.

## Cosa serve dopo il disegno

Due passi **in Editor**, che nessuno script del repository può fare al posto di una persona:

1. importare le texture in `/Game/RT/UI/Icons/`;
2. creare `DA_IconCatalog` (`URTIconCatalogData`), popolare `Icons` con **tutte le chiavi che
   `RequiredIconIds()` produce** — **60** al 2026-08-13 — e impostare `MissingIcon`.

⚠️ **Non fidarti di questo numero il giorno in cui esegui il passo 2.** L'insieme è derivato e cresce col
catalogo azioni: la lista vera si legge dal codice, e il gate che la pretende è `FindMissingRequiredIcons`
vuoto. Se il conteggio qui sopra non coincide con quello che il validator chiede, ha ragione il validator.

Solo allora `FindMissingRequiredIcons` sul catalogo **spedito** è vuoto, che è la parte del DoD di CP 20.2 che
richiede l'asset. La parte in codice — le chiavi derivate e i test che le pretendono — non la aspetta.
