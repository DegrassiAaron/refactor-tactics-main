# Piano — la derivazione di un'abilità d'eroe diventa un dato, e la raggiungibilità un gate

**Stato**: piano approvato, non implementato · **Data**: 2026-08-26 · **Origine**:
[#1403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1403)

⚠️ **Questo piano non tocca la semantica di `BaseActionId`**, che resta di
[`adr-0007`](../decisions/adr-0007-attacco-base-per-eroe.md) e D-033: ne **affianca** una seconda, per una
domanda diversa. La prima stesura le sovrapponeva su un campo solo, e §2 dice perché era sbagliato.

## 1. Il problema, misurato

Il catalogo core dichiara **37** azioni. Un'azione arriva in partita per quattro vie, e solo tre sono
interrogabili a runtime:

| # | Via | Come si riconosce | Interrogabile |
|---|---|---|---|
| 1 | generica | è in `GetGenericActionIds()`, quindi nel kit di ogni unità | ✅ |
| 2 | kit eroe | un `Hero.*` ne deriva i parametri | ❌ **oggi no**: nessun campo la porta |
| 3 | base di modulo | è la base di un `Reaction.*` in `MakeReactionModules()` | ✅ |
| 4 | scritta dal motore | il gameplay la produce da sé, senza kit | ❌ mai |

🔴 **La via 2 non è nel dato, e non esiste un campo che possa portarla.** `MakeHeroAction` copia i
parametri da un `FRTActionDef` locale e nessuno registra da dove vengano. La relazione «`Hero.Riktor.Ram`
eredita i suoi valori da `Action.Charge`» esiste **nel sorgente e in nessun altro posto**.

⛔ **E `BaseActionId` non è quel campo.** [D-033](../decisions/RT_PDR_00_Decision_Log.md) gli dà un
significato preciso — *«di quale delle sette generiche questa è il profilo»* — e `Action.Charge` non è una
generica. Usarlo per la derivazione renderebbe indistinguibili due relazioni diverse nella stessa traccia:
`Action.BasicAttack · ImpactShot` («profilo di una generica») e `Action.Charge · Ram` («eredita i
parametri»). Il test `RefactorTactics.Heroes.BasicAttackDeclaresItsBaseAction` **vieta** oggi la seconda —
asserisce che ogni abilità diversa da `Actions[0]` abbia `BaseActionId` vuoto — e ha ragione a vietarla.

⚠️ **`MakeHeroReactionFromCoreAction` riceve `CoreActionId` come parametro e non lo scrive da nessuna
parte.** Il dato attraversa la funzione e viene buttato.

Conseguenza misurata il 2026-08-26 su `main`: delle 37 azioni core, **16** non hanno nessuna delle quattro
vie, e nessun gate se ne accorge. Fra queste `Action.Shield`, che
[`adr-0003`](../decisions/adr-0003-modello-azioni-v01.md) dà per arrivata, e `Action.Sprint`, che 19 righe
di commento nominano e zero righe producono.

## 2. La decisione di semantica

Un campo **nuovo**, `FRTActionDef::DerivedFromActionId`, si popola per **derivazione meccanica**:
l'abilità costruisce i suoi valori a partire da quell'azione core. Non per parentela concettuale — un
attacco a linea inventato non dichiara `Action.LineAttack` solo perché gli somiglia.

**Perché il criterio è la derivazione**: è verificabile e ha un solo lettore possibile. La parentela
semantica darebbe un portatore a più azioni orfane, ma nessun test potrebbe dire se è giusta, e due
persone la scriverebbero diversa.

### Perché un campo nuovo e non `BaseActionId`

Sono due relazioni diverse, e sovrapporle su un campo solo renderebbe la traccia ambigua:

| Campo | Dice | Owner |
|---|---|---|
| `BaseActionId` | di quale delle **sette generiche** questa è il profilo | [D-033](../decisions/RT_PDR_00_Decision_Log.md), consumato da `DescribeActionIdentity` |
| `DerivedFromActionId` | da quale azione **core** eredita i parametri | questo piano, consumato dal gate di §4 |

Per gli attacchi base i due coincidono — `Hero.Gadget.ArcPulse` è profilo di `Action.BasicAttack` **ed**
eredita i suoi valori da lì (`URTCatalogLibrary::MakeBasicAttack`) — e non è un caso: un profilo di una
generica è anche una derivazione da essa. Il contrario non vale, ed è tutta la differenza: `Ram` deriva da
`Action.Charge`, che generica non è.

✅ **Guadagni della separazione**: [D-033](../decisions/RT_PDR_00_Decision_Log.md) non si rovescia, il test
`RTHeroCatalogTests.cpp:225` resta valido **e non si tocca**, la traccia di una partita non cambia di un
carattere, e con essa non cambiano né i golden né ciò che `RTScenarioRunner` stampa.

La regola del punto 6 di [`adr-0007`](../decisions/adr-0007-attacco-base-per-eroe.md) — un campo entra
solo quando esiste il consumer — è soddisfatta: il consumer è il gate di §4, e nasce nello stesso lavoro.

## 3. L'API che produce il dato

`MakeHeroActionFromCore(HeroActionId, CoreActionId, …)` prende l'**ID** invece del `Def`: fa lui la
`FindCoreAction`, eredita ciò che il chiamante non passa e scrive `DerivedFromActionId`. La dichiarazione
smette di essere una cosa da ricordarsi e diventa il modo stesso di derivare — la forma che
`MakeHeroBasicAttack` ha già per `BaseActionId`.

⛔ **Fail-closed**, come il fratello che già lo fa: un `CoreActionId` che il catalogo non conosce
restituisce `nullptr`. Un'abilità con parametri di default e una base falsa sarebbe peggio di nessuna
abilità.

`MakeHeroAction` resta, e serve alle abilità che **non** derivano da nulla: `LinearDischarge`, `Overload`,
`CircularTide`, `Reconfigure`, `FlowReaction`.

### Le otto derivazioni da dichiarare

| Abilità | Deriva da | Oggi |
|---|---|---|
| `Hero.Gadget.ConductiveNode` | `Action.Electrify` | `Def` locale, `RTHeroCatalogLibrary.cpp:307` |
| `Hero.Phase.FluidTrail` | `Action.Dash` | `Def` locale, `:453` |
| `Hero.Phase.MistVeil` | `Action.Ignite` | `Def` locale, `:477` |
| `Hero.Riktor.KineticPanel` | `Action.CreateCover` | `Def` locale, `:589` |
| `Hero.Riktor.Ram` | `Action.Charge` | `Def` locale, `:619` |
| `Hero.Gadget.ReactiveCapacitor` | `Action.Counter` | riceve l'ID e lo butta, `:800` |
| `Hero.Riktor.Interposition` | `Action.Intercept` | idem |
| `Hero.Wraith.Deflection` | `Action.Deflect` | idem |

Più i **quattro attacchi base**, che derivano anch'essi — `URTCatalogLibrary::MakeBasicAttack(fascia)`
parte da `FindCoreAction("Action.BasicAttack")` e ne modifica portata e danno secondo la fascia. Per loro
non si cambia il modo di costruire i parametri, che passano dalla fascia: `MakeHeroBasicAttack` guadagna
**una riga**, `DerivedFromActionId = Action.BasicAttack`, accanto al `BaseActionId` che già scrive.

∴ dodici abilità dichiarano una derivazione, cinque restano senza — `LinearDischarge`, `Overload`,
`CircularTide`, `Reconfigure`, `FlowReaction` — e il gate legge **un campo solo**.

✅ **Controllo incrociato che dà l'inventario per completo**: a lavoro finito le azioni raggiungibili per
via 2 e misurabili a runtime sono `BasicAttack · Electrify · Dash · Ignite · CreateCover · Charge ·
Counter · Intercept · Deflect` — **nove**, esattamente quante la misura statica sui sorgenti ne aveva
contate citate nei kit. Due metodi indipendenti, stesso numero.

## 4. Il gate

`RefactorTactics.Catalog.EveryCoreActionIsReachableOrDeclared`, in `RTCatalogTests.cpp`.

Misura le vie 1, 2 e 3 **via API** — nessun grep, nessuna euristica sul sorgente — e confronta il
risultato con un elenco dichiarato nel test, dove ogni azione che il gate **non vede raggiungibile** porta
la sua ragione. La via 2 si legge da `DerivedFromActionId` su ogni abilità di ogni eroe del roster: è il
campo di §2, ed è tutto ciò che il gate ha bisogno di sapere sui kit.

⚠️ **L'elenco tiene insieme tre casi diversi, e le ragioni servono a distinguerli.** Sono **24** voci:

- **16 orfane vere**, senza nessuna delle quattro vie: le 15 `NonAssegnata` più `Sprint`;
- **5 raggiungibili per una via che un test non può vedere** (`ScrittaDalMotore`): la via 4 è una
  proprietà del codice sorgente, non del dato, e nessuna API la espone;
- **3 raggiungibili per una via che non porta in partita** (`ModuloNonConsegnabile`): `Anchor`, `Evade` e
  `Purge` la via 3 **ce l'hanno** — il paragrafo qui sotto spiega perché non basta.

Le tre categorie invecchiano in modo diverso, ed è la ragione per cui non è un elenco solo.

| Ragione | Quante il 2026-08-26 | Significato |
|---|---|---|
| `ScrittaDalMotore` | 5 — `Move` `Cleanse` `Heal` `Interrupt` `ModifyArc` | il gameplay le produce senza kit: non sono contenuto mancante |
| `ModuloNonConsegnabile` | 3 — `Anchor` `Evade` `Purge` | basi di moduli reazione, e nessuna unità riceve equipaggiamento |
| `MigrazioneE38` | 1 — `Sprint` | la forma canonica (D-015, riaffermata da D-116) non è implementata |
| `NonAssegnata` | 15 | contenuto che nessun eroe porta, E6 |

### La via 3 non basta, ed è una scelta

`Action.Purge` **è** la base di `Reaction.Cleanse`. Ma i moduli si consegnano come equipaggiamento, e
l'equipaggiamento non arriva mai a un'unità: `EquipmentId` vive in cinque file — i cataloghi, il data
asset e lo `ScenarioHarness` — e `ARTUnit` non ha nessun campo che lo porti. Un gate che contasse la via 3
sufficiente direbbe il vero sul catalogo e il falso sulla partita.

➕ **Condizione di riapertura**: il giorno in cui un'unità riceve equipaggiamento in partita, quelle tre
voci escono dall'elenco e il gate cambia significato. È la ragione per cui questa scelta va nel Decision
Log e non solo qui.

### Fallisce in tre versi

1. un'azione core **non raggiungibile e non dichiarata** → *«assegnala a un kit o dichiara perché no»*;
2. un'azione **dichiarata che è diventata raggiungibile** → *«toglila dall'elenco»*. È il verso che
   impedisce all'elenco di fossilizzarsi, ed è la ragione per cui un residuo si asserisce invece di
   commentarlo;
3. una **voce dell'elenco che non è più nel catalogo** → *«l'azione non esiste più»*.

### E si difende dal proprio verde

Se `GetHeroRoster()` tornasse vuoto, ogni azione risulterebbe non raggiungibile — e con un elenco
abbastanza lungo il test resterebbe verde raccontando che va tutto bene. Quindi asserisce anche il verso
positivo: **almeno tredici** azioni raggiungibili, `Guard` per via generica, `Charge` per via kit — che è
`Ram` a portarla. Sono le asserzioni-oracolo della misura statica, portate dentro il test.

## 5. Verifica

Ordine: test mirati → suite → mutazioni.

```
UnrealEditor-Cmd.exe <progetto> -ExecCmds="Automation RunTests RefactorTactics+Quit" -nullrhi
```

Una run per volta, e `Test Completed` si legge **prima** del verdetto: se non coincide col totale, la run
non vale. Baseline statica di partenza: **1180** test dichiarati in 128 file — dichiarati, non eseguiti.

⚠️ **Mutazioni con l'implementazione già committata.** Tre, una per volta:

| Mutazione | Atteso |
|---|---|
| togliere `DerivedFromActionId` da `Ram` | rosso: `Charge` non raggiungibile e non dichiarata |
| togliere una voce dall'elenco | rosso sul verso 1 |
| dichiarare un'azione che è raggiungibile | rosso sul verso 2 |

## 6. Rischi

1. **Collisione con [#1400](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1400)**, in volo:
   tocca `RTTurnLog.h`, `RTTurnManager.*` e `spec-turnlog.md`. Questo piano tocca `RTHeroCatalogLibrary.*`,
   `RTActionDef.h` e i test — nessun file in comune **tranne il Decision Log**, dove serve una voce nuova:
   lì ci sono un conflitto di merge sulla tabella e un `D-nnn` da riverificare prima del merge.
2. **`FRTActionDef` guadagna un campo**, ed è una struct che il catalogo costruisce a ogni chiamata. Il
   costo è una `FName` per definizione; nessun percorso la serializza — la voce di TurnLog è una struct
   sua, e i data asset non portano `FRTActionDef` su disco. Da verificare compilando, non deducendo.

✅ **Cosa NON è più un rischio**, dopo la scelta di §2: la traccia non cambia, quindi `RTScenarioRunner`
stampa esattamente ciò che stampa oggi, `DescribeActionIdentity` non si muove, e i due golden — che
contengono solo `Action.Move` — restano fuori discussione per costruzione.

## 7. Documenti

- [`adr-0007`](../decisions/adr-0007-attacco-base-per-eroe.md): una nota che accanto a `BaseActionId`
  esiste ora `DerivedFromActionId`, e che le due domande sono diverse — *«di quale generica sono il
  profilo»* contro *«da quale azione core eredito i valori»*. **D-033 non si tocca**, e nemmeno il test
  `RefactorTactics.Heroes.BasicAttackDeclaresItsBaseAction`, che continua a valere parola per parola.
- [`RT_PDR_00_Decision_Log.md`](../decisions/RT_PDR_00_Decision_Log.md): una voce per la scelta di §4 — la
  via 3 non basta finché l'equipaggiamento non arriva a un'unità — con la sua condizione di riapertura.
- [#1403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1403): il gate chiude la sua
  domanda «per scelta o difetto?» trasformandola in una dichiarazione per riga.

## 8. Cosa NON è in scope

⛔ **Assegnare `Action.Cleanse` a un eroe.** È il passo successivo, ed è una scelta di bilanciamento: qui
si costruisce lo strumento che la rende verificabile.

⛔ **`Action.Sprint`.** La sua forma canonica è profilo `Move`, solo slot movimento — voce **41** di
[`DOC_CONFLICT_MATRIX`](../DOC_CONFLICT_MATRIX.md), riaperta da D-116 — e il codice la tiene in
`FastMovement` con `MovementAndMain`. È lavoro di **E38**, e *«non si migra da sola: porta con sé `Exposed`
a 2 turni»*. Consegnarla ora significherebbe consegnare la Sprint sbagliata.

⛔ **La parentela semantica** di §2, e con essa l'idea di dare una base alle abilità inventate.

⛔ **Rendere l'equipaggiamento consegnabile** per salvare `Purge`: è la condizione di riapertura, non il
lavoro di oggi.

## 9. Definition of Done

I criteri sono comandi, e i numeri di oggi sono la misura di partenza, non una soglia da ricopiare.

- [ ] `FRTActionDef::DerivedFromActionId` esiste, con un commento che lo distingue da `BaseActionId`
- [ ] `MakeHeroActionFromCore` esiste, è fail-closed su ID sconosciuto, e ha un test che lo dimostra
- [ ] Le otto derivazioni di §3 dichiarano la loro origine; le cinque `const FRTActionDef …Def =` locali
      sono sparite dal catalogo eroi
- [ ] I quattro attacchi base dichiarano `DerivedFromActionId`, e `BasicAttackDeclaresItsBaseAction` resta
      verde **senza essere modificato** — è la prova che le due semantiche non si sono sovrapposte
- [ ] `git grep -c "FindCoreAction" -- Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp` scende da
      **6** a **1** (resta quella dentro l'helper)
- [ ] Il gate di §4 esiste, con l'elenco dichiarato e i tre versi di fallimento
- [ ] Il gate asserisce il verso positivo di §4, e una mutazione che svuota il roster lo fa cadere
- [ ] Le tre mutazioni di §5 producono i tre rossi attesi, con l'implementazione committata prima
- [ ] Suite `RefactorTactics` verde, con `Test Completed` confrontato col totale e non con zero
- [ ] `node tools/radar/doc-links.ts --check` e `node tools/radar/doc-tables.ts --check` escono `0`
- [ ] La voce del Decision Log esiste, e il suo `D-nnn` è stato riverificato contro le PR aperte
