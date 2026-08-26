# Piano — la derivazione di un'abilità d'eroe diventa un dato, e la raggiungibilità un gate

**Stato**: **implementato** in [#1406](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1406) · **Data**: 2026-08-26 · **Origine**:
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
| 3 | equipaggiamento | un pezzo del **loadout di default** di un eroe la concede (`GrantedActionId`) | ✅ |
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

Conseguenza misurata il 2026-08-26 su `main`: delle 37 azioni core, **15** non hanno nessuna delle quattro
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
`Heroes.BasicAttackDeclaresItsBaseAction` resta valido **e non si tocca**, la traccia di una partita non cambia di un
carattere, e con essa non cambiano né i golden né ciò che `RTScenarioRunner` stampa.

La regola del punto 6 di [`adr-0007`](../decisions/adr-0007-attacco-base-per-eroe.md) — un campo entra
solo quando esiste il consumer — è soddisfatta: il consumer è il gate di §4, e nasce nello stesso lavoro.

## 3. L'API che produce il dato

`MakeHeroActionFromCore(HeroActionId, CoreActionId, …)` prende l'**ID** invece del `Def`: fa lui la
`FindCoreAction`, eredita ciò che il chiamante non passa e scrive `DerivedFromActionId`. La dichiarazione
smette di essere una cosa da ricordarsi e diventa il modo stesso di derivare — la forma che
`MakeHeroBasicAttack` ha già per `BaseActionId`.

⛔ **Fail-closed**: un `CoreActionId` vuoto o che il catalogo non conosce restituisce `nullptr`, e chi
chiama **deve** guardare il risultato — `GetHeroRoster()` gira all'avvio, e lì un dereferenziamento nullo
non è un fail-closed ma un crash.

⚠️ **La prima stesura sbagliava la guardia**, e diceva di averla presa «dal fratello». Il fratello
`MakeHeroReactionFromCoreAction` ha una clausola in più sullo slot; senza quella, `Core.ActionId !=
CoreActionId` con un ID vuoto confronta `NAME_None` con `NAME_None`, è falso, e lascia passare proprio
l'abilità coi default che il fail-closed esiste per impedire. La forma giusta — `IsNone()` prima del
confronto — è quella che `URTCatalogLibrary::MakeEquipmentAction` usa da sempre.

`MakeHeroAction` resta, e serve alle otto abilità che **non** derivano da nulla: `LinearDischarge`,
`Overload`, `CircularTide`, `Reconfigure`, `FlowReaction`, `InterceptShot`, `PassingBlade`, `Feint`.

### Le otto derivazioni da dichiarare

| Abilità | Deriva da | Oggi |
|---|---|---|
| `Hero.Gadget.ConductiveNode` | `Action.Electrify` | `Def` locale in `MakeGadget` |
| `Hero.Phase.FluidTrail` | `Action.Dash` | `Def` locale in `MakePhase` |
| `Hero.Phase.MistVeil` | `Action.Ignite` | `Def` locale in `MakePhase` |
| `Hero.Riktor.KineticPanel` | `Action.CreateCover` | `Def` locale in `MakeRiktor` |
| `Hero.Riktor.Ram` | `Action.Charge` | `Def` locale in `MakeRiktor` |
| `Hero.Gadget.ReactiveCapacitor` | `Action.Counter` | riceve l'ID e lo butta, in `MakeHeroReactionFromCoreAction` |
| `Hero.Riktor.Interposition` | `Action.Intercept` | idem |
| `Hero.Wraith.Deflection` | `Action.Deflect` | idem |

⛔ **Gli attacchi base restano fuori, e `MakeHeroBasicAttack` non si tocca.** Misurato durante
l'implementazione: dei quattro, solo `Hero.Gadget.ArcPulse` deriva davvero i parametri
(`URTCatalogLibrary::MakeBasicAttack(4)` parte da `FindCoreAction`); `PressureJet`, `ImpactShot` e
`PulseShot` li hanno **letterali**. Scrivere `DerivedFromActionId` su tutti e quattro trasformerebbe «i
parametri vengono da lì» in «gli somiglia», che è la parentela semantica scartata in §2.

∴ **il gate legge due campi**, `BaseActionId` ∪ `DerivedFromActionId`: sono due vie di raggiungibilità
diverse e ugualmente valide — «un eroe porta un profilo di X» e «un eroe porta un'abilità che eredita da
X». `Action.BasicAttack` resta raggiungibile per la prima, che è quella vera per lui.

Otto abilità dichiarano una derivazione e **otto** restano senza — `LinearDischarge`, `Overload`,
`CircularTide`, `Reconfigure`, `FlowReaction`, `InterceptShot`, `PassingBlade`, `Feint` — mentre i quattro
attacchi base continuano a dichiarare solo il profilo, come D-033 vuole. Otto più otto più quattro fa
**venti**, che sono le cinque abilità di ciascuno dei quattro eroi.

⚠️ **La prima stesura ne contava cinque**, dimenticando le tre di Wraith, e lo faceva sotto un titolo che
prometteva un «controllo incrociato». Il controllo tornava lo stesso perché verificava le *derivate*, che
erano giuste: la somma delle non-derivate non la controllava nessuno.

✅ **Controllo incrociato che dà l'inventario per completo**: a lavoro finito le azioni raggiungibili per
via 2 e misurabili a runtime sono `BasicAttack · Electrify · Dash · Ignite · CreateCover · Charge ·
Counter · Intercept · Deflect` — **nove**, esattamente quante la misura statica sui sorgenti ne aveva
contate citate nei kit. Due metodi indipendenti, stesso numero.

## 4. Il gate

`RefactorTactics.Catalog.EveryCoreActionIsReachableOrDeclared`, in `RTCatalogTests.cpp`.

Misura le vie 1, 2 e 3 **via API** — nessun grep, nessuna euristica sul sorgente — e confronta il
risultato con un elenco dichiarato nel test, dove ogni azione che il gate **non vede raggiungibile** porta
la sua ragione. La via 2 si legge da **`DerivedFromActionId` e `BaseActionId`** su ogni abilità di ogni
eroe del roster: il primo copre le otto derivazioni di §3, il secondo gli attacchi base — che di
`Action.BasicAttack` sono il profilo, non una derivazione di parametri.

⚠️ **L'elenco tiene insieme tre casi diversi, e le ragioni servono a distinguerli.** Sono **22** voci —
il numero si conta sul codice, non qui: `awk '/Dichiarate/,/};/' RTCatalogTests.cpp | grep -c 'TEXT("Action.'`.

- **14 aspettano il loro eroe**: contenuto che diventerà raggiungibile quando entrerà chi lo usa. Non
  sono difetti, ed è la ragione per cui sono dichiarate invece che corrette;
- **5 sono scritte dal motore**: la via 4 è una proprietà del codice sorgente e nessuna API la espone,
  quindi un test non può vederla;
- **3 hanno un pezzo che le concede** e nessun eroe che lo porta — `Anchor`, `Purge`, `HeavyAttack`.

⚠️ **La prima stesura di questo blocco ne contava 24 ed elencava `Evade` fra le non consegnabili.**
Entrambe sbagliate: `Action.Evade` è la base di `Reaction.HazardEscape`, che è il modulo di default di
Phase, quindi è raggiungibile — dichiararla oggi farebbe scattare il **verso 2** del gate.

Le categorie invecchiano in modo diverso, ed è la ragione per cui non è un elenco solo.

| Ragione | Quante il 2026-08-26 | Significato |
|---|---|---|
| `ScrittaDalMotore` | 5 — `Move` `Cleanse` `Heal` `Interrupt` `ModifyArc` | il gameplay le produce senza kit: non sono contenuto mancante |
| `PezzoNonAssegnato` | 3 — `Anchor` `Purge` `HeavyAttack` | il pezzo che le concede esiste, e non è il default di nessun eroe |
| `MigrazioneE38` | 1 — `Sprint` | la forma canonica (D-015, riaffermata da D-116) non è implementata |
| `AspettaIlSuoEroe` | 13 | contenuto che diventerà raggiungibile quando entrerà l'eroe che lo usa |

### La raggiungibilità si misura sul roster ATTUALE

⚠️ **Questa sezione diceva il falso, e la correzione è arrivata da una code review.** Sosteneva che i
moduli non arrivassero mai a un'unità perché «`EquipmentId` vive in cinque file e `ARTUnit` non ha il
campo». Il canale esiste e non nomina mai `EquipmentId`: `DefaultLoadoutFor` prescrive un gadget e un
modulo per ogni eroe, `SetupHexMatch` li consegna, e `Heroes.SpawnedUnitCarriesItsDefaultLoadout` —
che era fra i test verdi citati come prova — asserisce proprio che «i pezzi che CONCEDONO un'azione
sono in campo».

⚠️ **Ma copre metà roster, non tutto**, e la prima correzione di questa sezione diceva «tutti e quattro»:
anche quella era sbagliata, in senso opposto. `DefaultLoadoutFor` è **all-or-nothing** — se un pezzo
prescritto non è spedito restituisce `{}` invece di un loadout parziale, perché `ValidateLoadout`
rifiuterebbe l'insieme incompleto tre livelli più in là. §4 assegna `Gadget.Insulator` a Gadget e
`Gadget.Sensor` a Wraith, e `MakeGadgets` non costruisce né l'uno né l'altro: **Gadget e Wraith non hanno
loadout**, e la terza via copre solo Phase e Riktor. Lo dicono `RTGameMode.cpp` e il commento di
`RTHeroSpawnTests.cpp` — *«Metà roster non ha un loadout»* — che sono le due fonti che avrei dovuto
leggere prima di scrivere «tutti e quattro».

Ne seguivano due errori di classificazione: `Action.CreateWater` è portata da `Gadget.Sprinkler`, default
di Phase — e il catalogo lo chiama «l'unico produttore d'acqua che il roster può portare in campo» — e
`Action.Evade` è la base di `Reaction.HazardEscape`, modulo di default di Phase.

**La regola che resta**, e che il gate ora calcola invece di dichiarare: un'azione è raggiungibile se il
roster **attuale** la porta in campo — nel kit o nel loadout di default. Un pezzo che esiste e che nessun
eroe porta non conta, come non conta un'abilità che nessun eroe ha.

➕ **Condizione di riapertura**: un'azione esce dall'elenco il giorno in cui **un eroe la porta**, e il
verso 2 del gate lo pretende invece di lasciare la riga a invecchiare. Le azioni che restano non sono
difetti: sono contenuto che aspetta il suo portatore.

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

- [x] `FRTActionDef::DerivedFromActionId` esiste, con un commento che lo distingue da `BaseActionId`
- [x] `MakeHeroActionFromCore` esiste, è fail-closed su ID sconosciuto, e ha un test che lo dimostra
- [x] Le otto derivazioni di §3 dichiarano la loro origine
- [x] `MakeHeroBasicAttack` **non è stata toccata**, e `BasicAttackDeclaresItsBaseAction` resta verde
      senza essere modificato — è la prova che le due semantiche non si sono sovrapposte
- [x] `git grep -c "FindCoreAction" -- Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp` resta
      **6**, e il criterio che diceva «scende a 5» era **sbagliato**: `IgniteDef` è sparita come previsto,
      ma `MakeHeroActionFromCore` ha una `FindCoreAction` propria, quindi il totale non si muove. Il
      numero giusto da guardare sono le letture *nei chiamanti*, che scendono da **5** a **4** —
      `git grep -c "const FRTActionDef .*Def = URTCatalogLibrary::FindCoreAction"`.
      ⚠️ Quattro di quelle quattro cercano la stessa azione che l'helper cerca di nuovo: è una
      **doppia lookup** dichiarata e non risolta, perché toglierla vuol dire far restituire all'helper il
      `Def` risolto, e quella è una firma diversa da quella approvata
- [x] Il gate di §4 esiste, con l'elenco dichiarato e i tre versi di fallimento
- [x] Il gate asserisce il verso positivo di §4, e una mutazione che svuota il roster lo fa cadere
- [x] Le tre mutazioni di §5 producono i tre rossi attesi, con l'implementazione committata prima
- [x] Suite `RefactorTactics` verde, con `Test Completed` confrontato col totale e non con zero
- [x] `node tools/radar/doc-links.ts --check` e `node tools/radar/doc-tables.ts --check` escono `0`
- [x] La voce del Decision Log esiste, e il suo `D-nnn` è stato riverificato contro le PR aperte
