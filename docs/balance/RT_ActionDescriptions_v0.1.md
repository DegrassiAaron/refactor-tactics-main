# RT — Frasi dei tooltip delle azioni v0.1

> **Owner**: [#3499](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3499) · **Stato**: **accettate dall'autore per la v0.1** il 2026-10-06 — *«vanno bene così, le rivedremo in un secondo momento»*. Scritte da chi implementa a partire dai dati; la revisione è [#3526](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3526)
>
> La frase che il tooltip di uno slot mostra **sopra i numeri**. Decisione d'autore del 2026-10-05: il tooltip è
> *testo d'autore più numeri dal gioco*.
>
> - **Una frase non porta numeri.** Fase, slot, portata, ricarica e danno li compone il gioco dal catalogo
>   (`URTHudViewModel::BuildActionTooltip`): una frase che li ripetesse invecchierebbe al primo ribilanciamento.
> - **Il testo è quello che il gioco mostra**, carattere per carattere, compreso l'apostrofo al posto
>   dell'accento (`piu'`, `e'`), che è la convenzione del testo a schermo del progetto.
> - **Le copie sono due**, questa e la tabella in
>   [`RTActionDescriptions.cpp`](../../Source/RefactorTactics/Ability/RTActionDescriptions.cpp). Il radar le
>   confronta, e non dice quale delle due correggere:
>
>   ```sh
>   node tools/radar/action-descriptions.ts --check   # exit 1 se una frase o un nome divergono
>   ```
>
> - **Il roster è coperto per intero**: `RefactorTactics.Catalog.EveryRosterActionHasADescription` fallisce se
>   un'azione resta senza frase, o se una frase non ha più la sua azione.
>
> ⛔ **Fuori**: le azioni d'equipaggiamento. Il loro tooltip mostra i soli numeri, finché non avranno una frase.
>
> **Come rivederle**: correggi la colonna *Frase*. Il cambio va riportato nella tabella C++ nella stessa PR; il
> radar lo pretende. La colonna *Nome* viene dal gioco (`HeroActionDisplayName`, `GenericActionDisplayName`), e
> il radar controlla anche quella.

## Generiche (D-025)

| Azione | Nome | Frase | Da rivedere |
|---|---|---|---|
| `Action.Wait` | Attesa | Non agisci in questo turno. |  |
| `Action.Guard` | Guardia | Ti metti in guardia: ogni colpo che arriva di fronte perde una parte del danno. |  |
| `Action.Brace` | Irrigidimento | Ti irrigidisci: attutisci un po' i colpi da ogni direzione e resisti alle spinte. |  |
| `Action.Overwatch` | Guardia reattiva | Sorvegli la direzione in cui guardi e spari al nemico che vi passa; puoi muoverti solo per ritirarti. |  |
| `Action.Interact` | Interagisci | Usi un oggetto su una cella vicina, per esempio una porta. |  |

## Aevik

| Azione | Nome | Frase | Da rivedere |
|---|---|---|---|
| `Hero.Aevik.ArcPulse` | Impulso ad arco | Una scarica elettrica a distanza su un bersaglio. |  |
| `Hero.Aevik.LinearDischarge` | Scarica lineare | Una scarica in linea retta, piu' dannosa su chi e' bagnato. |  |
| `Hero.Aevik.ConductiveNode` | Nodo conduttivo | Elettrizza una cella: la scarica corre lungo l'acqua e le superfici conduttive vicine. |  |
| `Hero.Aevik.Overload` | Sovraccarico | Un'esplosione elettrica ad area, che interrompe anche i dispositivi. |  |
| `Hero.Aevik.ReactiveCapacitor` | Condensatore reattivo | Reazione: quando vieni colpito ti scherma e restituisce una scarica a chi ti attacca. |  |

## Muiren

| Azione | Nome | Frase | Da rivedere |
|---|---|---|---|
| `Hero.Muiren.PressureJet` | Getto in pressione | Un getto d'acqua in linea: bagna il bersaglio e lo spinge indietro. |  |
| `Hero.Muiren.CircularTide` | Marea circolare | Un'onda ad area attorno a un punto: cura gli alleati o colpisce, secondo la variante scelta. |  |
| `Hero.Muiren.FluidTrail` | Scia fluida | Uno scatto rapido verso una cella vicina. |  |
| `Hero.Muiren.MistVeil` | Velo di nebbia | Crea una nube di nebbia che ostacola la vista. |  |
| `Hero.Muiren.FlowReaction` | Reazione di flusso | Reazione pensata per riposizionarti dopo un attacco: in questa versione non ha ancora effetto. | ⚠️ l'azione oggi non ha effetti in partita, e la frase lo dice |
| `Hero.Muiren.TideGuard` | Guardia di marea | Uno scudo temporaneo su di te, da scegliere prima di sapere se sarai colpito. |  |

## Branth

| Azione | Nome | Frase | Da rivedere |
|---|---|---|---|
| `Hero.Branth.ImpactShot` | Colpo d'impatto | Un colpo cinetico che rallenta il bersaglio. |  |
| `Hero.Branth.KineticPanel` | Pannello cinetico | Erige un pannello di copertura sul lato di una cella. |  |
| `Hero.Branth.Reconfigure` | Riconfigurazione | Sposta o ruota una copertura gia' in campo. |  |
| `Hero.Branth.Ram` | Carica d'ariete | Carichi contro un nemico: lo colpisci e lo spingi. |  |
| `Hero.Branth.Interposition` | Interposizione | Reazione: ti metti in mezzo e incassi il colpo diretto a un alleato vicino. |  |
| `Hero.Branth.MortarShot` | Colpo di mortaio | Un tiro ad arco su un'area, che arriva anche dove non vedi. |  |

## Ivrin

| Azione | Nome | Frase | Da rivedere |
|---|---|---|---|
| `Hero.Ivrin.PulseShot` | Colpo a impulsi | Un colpo a impulsi a distanza su un bersaglio. |  |
| `Hero.Ivrin.InterceptShot` | Intercetto | Miri una cella vicina e colpisci chi ci passa durante il movimento. |  |
| `Hero.Ivrin.PassingBlade` | Lama di passaggio | Scatti in avanti e colpisci chi attraversi. |  |
| `Hero.Ivrin.Deflection` | Deviazione | Reazione: devii parte del danno che ricevi. |  |
| `Hero.Ivrin.Feint` | Finta | Pensata per marcare una cella e riposizionarti: in questa versione non ha ancora effetto. | ⚠️ l'azione oggi non ha effetti in partita, e la frase lo dice |
| `Hero.Ivrin.PhaseGuard` | Guardia di fase | Uno scudo temporaneo su di te, da scegliere prima di sapere se sarai colpito. |  |
