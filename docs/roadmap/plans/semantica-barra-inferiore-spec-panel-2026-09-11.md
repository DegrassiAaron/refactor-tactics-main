# Spec panel — La semantica della barra inferiore, e i contratti che non ha

**Data**: 2026-09-11 · **Modo**: `critique` · **Fuoco**: `requirements` + `architecture` + `testing`

**Misurato su** `aa0b08fa` (HEAD del clone `refactor-tactics-designer`), **riverificato su `origin/main` =
`f11b7b1a`** per i soli simboli toccati dal delta fra i due — `RTScreenHudWidgets.h/.cpp`,
`RTScreenHudWidgetTests.cpp`, `WBP_RT_ActionSlot.uasset`. Il delta **non è cosmetico**: aggiunge
`URTActionSlotWidget::GetActionLine()`, che è una delle risposte di questo referto (§2.5).

> ⚠️ **Convenzione di questo documento**: non compare nessun totale che invecchia da solo. Dove serve una
> misura, c'è il **comando** che la produce; dove serve un elenco, ci sono i **nomi**.

---

# 1. La domanda, e la risposta misurata

> *«Determina nel repository se esistono distintamente: Skill/Ability Palette · Plan Slots · Context Actions ·
> Reaction choices. Non assumere che siano un unico widget.»*

**Non sono un unico widget.** Sono **tre** classi C++ distinte, con tre `.uasset` distinti, e la separazione è
dichiarata nelle firme. Ma **due dei quattro concetti coincidono in una sola lista piatta**, e un terzo non ha
nessuna rappresentazione nella barra.

| Concetto richiesto | Esiste come? | Dove |
|---|---|---|
| **Skill/Ability Palette** | ⚠️ **non distinta** — è il kit intero | `URTActionDockWidget::GetActions()` → `URTHudViewModel::BuildAbilityCooldowns` |
| **Plan Slots** | ✅ **distinti**, e in un widget **diverso** | `URTSelectedUnitPanelWidget::GetSlots()` → `FRTUnitSlotsView{Movement, Main, Reaction}` |
| **Context Actions** | ⚠️ **fuse nella palette** — e **due delle sette** non ci sono affatto | `URTCatalogLibrary::GetGenericActionIds()`, accodate al kit |
| **Reaction choices** | ✅ **distinte**, widget proprio, porta propria | `URTFastDecisionWidget` + `URTFastDecisionOptionWidget::Choose()` |

## 1.1 Le due economie NON sono in conflitto — la documentazione lo aveva già risolto

L'ipotesi di partenza («la doc dice tre slot, il comportamento dice `1–N` skill») **non regge alla misura**, e
va detto perché è la premessa che avrebbe orientato male tutte le issue.

* `progettazione-hud.md` §6.7 dichiara esplicitamente che la divisione **Universal Actions / Hero Kit** è
  *«una **classificazione UI**, non una doppia economia d'azione»*, e mette in guardia contro il layout che
  faccia pensare *«scegli una Universal Action + una Hero Ability»*.
* `FRTUnitSlotsView` porta i **tre slot del piano** — `Movement · Main · Reaction` — e il commento della sua
  dichiarazione chiude il caso: *«Non sono tre booleani indipendenti»*, chi decide è
  `URTCatalogLibrary::TakesMovementSlot`/`TakesMainSlot`, *«qui non si riscrive la regola, la si interroga»*.
* `FRTAbilityCooldownView::Slot` porta, per **ogni voce del kit**, quale slot quella voce consumerebbe.

∴ **`1–N` è la palette; `3` è l'economia.** Una voce della palette *dichiara* quale dei tre slot consuma. Il
modello è coerente, ed è già scritto.

🔴 **Ciò che manca è che la barra lo mostri.** `FRTAbilityCooldownView::Slot` ha **un solo sito di scrittura e
nessun lettore fuori dai test**:

```bash
grep -rn "\.Slot\b" --include=*.cpp Source/RefactorTactics/UI/
# → RTHudViewModel.cpp:335:  View.Slot = Action->Def.Slot;     (la scrittura, e basta)
```

Il campo esiste, il commento che lo accompagna promette che *«il pannello raggruppa per slot, non per ordine
nel kit»* — e nessun pannello lo fa: `GetActions()` documenta l'ordine opposto, *«nell'ordine del kit:
l'indice è quello che l'hotkey arma»*. **Le due frasi sono in contraddizione dentro lo stesso modulo**, e la
seconda è quella che il codice esegue.

## 1.2 Le due universali che nella barra non ci sono

[D-025] dichiara **sette** generiche: `Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch`.
`URTCatalogLibrary::GetGenericActionIds()` ne accoda al kit **cinque**: mancano `Move` e `BasicAttack`.

* **`Move`** non è una voce di kit: si pianifica cliccando le celle (`HandleClickOnCell` → waypoint), e
  `Action.Move` compare solo come `Timeline.MoveActionId`. Nella barra **non ha uno slot**, mentre §6.7 lo
  elenca nella corsia `Movement`.
* **`BasicAttack`** non è generica: è **dell'eroe**, all'indice `[0]` del kit, dichiarato da
  `MakeHeroBasicAttack` con `Def.BaseActionId = Action.BasicAttack`.

∴ la corsia `Movement` di §6.7 e la corsia `Attack` **non hanno un produttore nella lista che il dock
disegna**. Non è un difetto di rendering: è un contratto che il documento dichiara e il codice non serve.

---

# 2. Dove i concetti si mescolano — con l'evidenza

## 2.1 🔴 L'identità dello slot è **posizionale**, e la posizione è stabile solo finché nessuno filtra

Il contratto A chiede un `SlotId` stabile e vieta *«l'indice momentaneo dell'array come identità persistente
senza verificarne la stabilità»*. Qui la verifica dà un risultato **misto**, e la metà rotta è silenziosa.

✅ **L'indice di kit *è* progettato come identità stabile**, e la scelta è dichiarata:
`GetGenericActionIds()` avverte che l'ordine *«conta: sono accodate al kit, quindi diventano indici stabili.
Cambiarlo sposta gli indici di ogni unità — e `PlannedAbilityIndex` è un indice»*. `FRTAbilityCooldownView`
porta **sia** `ActionId` **sia** `AbilityIndex`, e quest'ultimo esiste apposta: *«è ciò che l'hotkey arma,
quindi il widget ne ha bisogno»*.

🔴 **Ma `BuildAbilityCooldowns` RINUMERA, e `SelectAbilityForCurrent` no.**

```cpp
// RTHudViewModel.cpp — dentro il ciclo su NumAbilities()
const URTActionData* Action = Unit->GetAbility(Index);
if (!Action)
{
    continue;              // ⛔ la riga non viene emessa: l'array esce PIÙ CORTO del kit
}
```

Il canale tastiera non ha questo ramo: `OnAbility6` chiama `SelectAbilityForCurrent(5)`, cioè la **posizione
di kit** `5`, comunque. ∴ una posizione nulla fra due popolate fa divergere **la posizione visiva** dalla
**posizione di kit** — il tasto `6` e il sesto riquadro smettono di essere la stessa cosa.

⚠️ **Il caso non è teorico, e lo dichiarano tre difese scritte apposta**: `SelectAbilityForCurrent` ha
`if (!Ability) return;` (che [#2986](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2986) sta
correggendo perché è **muto**), `MakeGenericActions` fa `continue` su un ID che il catalogo non conosce, e
`GetAbility` fa bounds-check. Tre punti del progetto ammettono che una posizione possa non produrre
un'azione; **nessuno di loro concorda su cosa succede dopo**.

⛔ **E il test che sembrerebbe coprirlo, non lo copre.** `HudViewModel.CooldownsMirrorTheSimulator` asserisce
`Cds.Num() == Unit->NumAbilities()` e `Cds[i].AbilityIndex == i` — su `Hero.Aevik`, cioè su un kit **senza
buchi**. Il ramo `continue` non viene mai eseguito: l'asserzione pinna il contratto *«una riga per azione»*
esattamente nel caso in cui è vero, e tace in quello in cui il codice lo viola.

## 2.2 🔴 Il binding visualizzato è **derivato**, non **letto** — e per l'ultimo slot è falso

Su `origin/main` = `f11b7b1a`, `ARTHUD::ComposeAbilityLine` compone la riga dello slot così:

```cpp
// Il numero e' 1-based: e' la scorciatoia che il giocatore preme, non l'indice del kit.
Line.Text = FString::Printf(TEXT("%d. %s"), Ability.AbilityIndex + 1, *Ability.DisplayName.ToString());
```

Il commento dichiara l'intenzione giusta — *«la scorciatoia che il giocatore preme»* — e la implementa con
un'**aritmetica sull'indice** invece che con una lettura della tabella dei binding. Le due cose divergono in
tre punti già presenti nel codice:

| Caso | La riga dice | Il tasto è | Fonte |
|---|---|---|---|
| `AbilityIndex == 9` | `10.` | **`0`** | `AbilityHotkeys()` chiude con `EKeys::Zero` |
| le generiche `[5..9]` | solo il numero | **anche** `G B C X Z` | `GenericHotkeys()` |
| kit più lungo dei tasti | un numero qualunque | **nessuno** | è il difetto storico che `GenericHotkeys()` racconta: *«il kit faceva undici voci contro dieci tasti… impremibile in partita. Un verde che mente»* |

⚠️ **E il test lo afferma senza misurarlo.** Il gemello di
`ScreenHud.ActionSlotLineIsTheSameComposerAsTheHud` asserisce *«la riga nomina il tasto che arma lo slot»*
con `Action.AbilityIndex = 3` → cerca `"4. "`. L'indice centrale passa; **il primo e l'ultimo non sono
provati**, ed è precisamente l'ultimo a essere sbagliato. Il nome del test fa una promessa che l'oracolo non
mantiene.

## 2.3 ⚠️ Lo slot non sa di **quale unità** è

`URTActionSlotWidget` porta `Action`, `bArmed`, `ReceivedCatalog`. **Non porta l'unità proprietaria.** Il
click risolve così:

`grafo di WBP_RT_ActionSlot` → `ArmKitAbility(int32)` → `GetSelectedUnit()` → l'unità selezionata **adesso**.

∴ fra il `SetAction` che ha popolato il riquadro e il click che lo attiva, **la selezione può essere
cambiata**, e l'indice viene applicato al kit di un'altra unità. Il contratto A chiede l'unità proprietaria
fra i campi dello slot; qui non c'è, e il percorso non la confronta.

📊 **Il precedente corretto è nello stesso file.** `URTFastDecisionOptionWidget` porta `OptionIndex` **e**
l'owner, e `SetOption(URTFastDecisionWidget* InOwner, …, int32 InIndex, bool bInIsSafe)` li tiene in C++.
La guida UMG dichiara il perché, per la stessa famiglia di difetti: il grafo non deve decidere *«il
proprietario — e con esso una seconda porta su `Choose Option`, con un indice non suo»*. **Il dock non ha
l'equivalente**: non esiste un `MakeSlotWidget`, e lo slot non ha un `Arm()` proprio.

## 2.4 ⚠️ Il ciclo di vita della selezione non ha un owner, e nessuno lo azzera

`SelectedAbilityIndex` vive su `ARTUnit` ed è scritto da **un solo** sito di produzione
(`ARTUnit::SelectAbility`, chiamato da `SelectAbilityForCurrent`). La conseguenza va dichiarata invece che
scoperta:

```bash
grep -rn "SelectedAbilityIndex" --include=*.cpp --include=*.h Source/RefactorTactics/ | grep -v "Tests/"
```

Nessun sito lo azzera a **cambio fase**, a **fine round**, o quando l'unità viene **invalidata**. Non è
necessariamente sbagliato — un armamento per-unità che sopravvive al cambio di selezione può essere il
comportamento voluto — ma **non è dichiarato da nessuna parte**, quindi non è nemmeno verificabile. Il
contratto B chiede che *«nessuna selezione precedente lasci input mode, highlight, target o callback
residui»*: oggi la risposta è «dipende», e dipende da un campo che nessun documento possiede.

⚠️ **`SelectUnit` non tocca l'armamento**, né della vecchia né della nuova: ripristina l'anteprima del piano
(`SetPreviewPath`, `RefreshPlanningPreview`) e basta. Il residuo che il contratto B teme non è un bug
osservato: è un **caso non specificato**.

## 2.5 ✅ Ciò che il delta di `origin/main` ha appena chiuso — e va sottratto dalle issue

`URTActionSlotWidget::GetActionLine()` (nuovo in `f11b7b1a`) inoltra a `ComposeAbilityLine` e porta allo slot
**tasto, nome, stato armato e motivo d'indisponibilità** come **testo**, col prefisso `> ` per l'armata e
`(ricarica N)` per il motivo. ∴ una parte degli scope 3 · 4 · 6 di
[#2826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2826) **è consegnata via canale
testuale**, e resta scoperto ciò che il testo non dà: gli stati `Hover`, `Pressed`, `Targeting`,
`Planned/Committed`, `Empty`, `Disabled by phase` del contratto E, e il `0` dell'ultimo slot di §2.2.

⚠️ **E la seduta `U49` del 2026-09-11 ha misurato a schermo l'altra metà**: *«gli slot non portano nessun
segno visivo che separi disponibile / selezionato / cooldown»*, col criterio (3) di `PIE-V01-SCREENHUD`
dichiarato **non eseguibile** per la guardia sulle icone di ripiego
([#2963](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2963)).

---

# 3. Il panel — critique

**KARL WIEGERS** — *qualità dei requisiti*
> ❌ **CRITICO.** Il requisito *«la riga nomina il tasto che arma lo slot»* è **affermato da un nome di test**
> e misurato su un solo valore. Un criterio di accettazione che vale per `AbilityIndex = 3` e non per `9` non
> è un criterio: è un esempio. Il criterio corretto è un'**uguaglianza con la tabella dei binding** —
> `AbilityHotkeys()` e `GenericHotkeys()` — per **ogni** posizione producibile, non per una.

**ALISTAIR COCKBURN** — *attore e goal*
> ❓ **Chi è l'attore di uno slot?** Oggi la risposta è «l'unità selezionata nell'istante del click», e
> l'attore non compare fra i dati dello slot. Finché il caso d'uso non nomina il proprio attore, «arma lo
> slot 3» ha due letture — *quel* riquadro, o la terza posizione di *qualunque* unità — e la seconda è
> quella che il codice esegue.

**MARTIN FOWLER** — *interfacce*
> ⚠️ `URTActionDockWidget` espone **una** lista per **due** responsabilità — palette d'eroe e azioni
> universali — e §6.7 ne chiede **cinque** raggruppamenti. La lista piatta non è sbagliata: è
> **sotto-specificata**. E l'asimmetria con `FastDecisionOption` è il segnale che manca un pezzo: là owner e
> indice sono in C++ e testati, qui il grafo li ricompone.

**MICHAEL NYGARD** — *modi di guasto*
> 🔴 **Il modo di guasto è già scritto nel codice, tre volte, con tre risposte diverse.** `continue` che
> rinumera, `return` che rifiuta, bounds-check che tace. In produzione questo è *«alcune skill rispondono e
> altre no»* — il sintomo esatto che ha aperto questa indagine — e il log non lo distingue. Il fail-closed
> dev'essere **uno**: o la vista emette un segnaposto e le posizioni restano allineate, o il contratto
> dichiara che l'array è più corto e **chi legge deve usare `AbilityIndex`**, mai la posizione.

**GOJKO ADZIC** — *esempi eseguibili*
> ⚠️ Gli edge case del contratto G non sono casi limite: sono **gli esempi che definiscono il contratto**.
> `slot 1 · slot intermedio · ultimo slot` non è una terna di cortesia — l'ultimo è quello che oggi mente.
> «Slot vuoto fra due popolati» non è ipotetico: `continue` lo produce.

**LISA CRISPIN** — *strategia di test*
> ✅ **La linea fra headless e PIE è già tracciata, e va rispettata invece che ridiscussa.**
> `RTMatchWidgetAssetTests.cpp` carica i `.uasset` (`LoadWidgetTree`, `ForEachWidget`, `Class->Bindings`) e
> misura geometrie, gerarchia e binding a design-time: **il contratto D è headless**, non è materia da PIE.
> ⚠️ Ma la lezione scritta in quel file va applicata prima: *«`ActionDockConsumesArmedIndex` cercava property
> binding dove il dock usa il **grafo**, e sbagliava meccanismo»*. Un gate sul `ForEach` del dock deve
> misurare **il grafo**, o nasce verde per il motivo sbagliato.

---

# 4. Le issue — come i contratti si distribuiscono

⛔ **Nessuna issue per edge case**, come richiesto: i casi del contratto G entrano come **criteri** dentro le
tre issue di lavoro.

| Issue | Possiede | Contratti |
|---|---|---|
| [#2987](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2987) — **input routing e identità dello slot** | la catena `input → slot → skill → validazione → targeting`, l'identità (`AbilityIndex`, unità proprietaria), il binding **letto** e non derivato, la parità dei due canali | **A** · **C** · parte di **G** |
| [#2988](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2988) — **ViewModel: ciclo di vita e stato dello slot** | cosa succede fra due selezioni, due fasi, due round; lo **stato** dello slot come dato del ViewModel invece che come deduzione del grafo | **B** · **E** · parte di **G** |
| [#2989](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2989) — **verifica: il cablaggio del dock ha un oracolo** | il gate headless sul `.uasset` (hit test, `Visibility`, focusability, delegate, quale valore il `ForEach` passa), e la voce PIE per ciò che resta giudizio umano | **D** · **G** · la coda di **F** |
| [#2990](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2990) — **`BLOCKED · DECISION REQUIRED`** | la relazione fra palette, plan slots, universali, hotkey e reaction window | **H** |

**Già aperte, e non vanno duplicate:**

* [#2986](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2986) possiede il contratto **F** —
  *«l'esito della richiesta di armamento, fra la pressione e il modello»*. ⛔ Nessuna issue nuova tocca la
  verbosità dei log né lo stato incoerente della reazione in ricarica.
* [#2826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2826) possiede la **leggibilità**
  dello slot (scope 3 · 4 · 6 · 7) e il prompt di targeting. La parte consegnata da `f11b7b1a` è §2.5.
* [#2963](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2963) possiede le chiavi icona
  mancanti; [#2964](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2964) il feed vuoto.
  Entrambe misurate nella seduta `U49`.

---

# 5. La decisione aperta

Il repository **non possiede** una decisione sulla relazione fra `Skill Palette`, `Action Dock`, `Plan
Slots`, le hotkey `1–N`, le azioni universali e la `Reaction Window`. Quello che possiede:

* **[D-025]** — quali sono le sette generiche;
* **[D-028] / [D-114]** — i tre slot del turno;
* **[D-128]** — lo stato neutro dell'armamento;
* **`progettazione-hud.md` §6.7 / §7** — la *classificazione UI* in corsie, e gli stati di uno slot;
* **`GetGenericActionIds()`** — l'ordine del kit come identità stabile.

⛔ **Ciò che nessuna di queste dice**, e che serve prima di rendere definitiva l'architettura del widget:

1. Se il dock possa **riordinare o raggruppare** (§6.7 lo chiede) mentre `AbilityIndex` resta l'identità —
   e quindi se il numero mostrato sia un **binding** o una **posizione**;
2. Se `Move` e `BasicAttack`, universali per [D-025], debbano avere uno slot nella barra o restare
   contestuali;
3. Se un'azione armata **sopravviva** al cambio di unità, al cambio di fase e al cambio di round;
4. Se le generiche debbano mostrare **il proprio tasto** (`G B C X Z`) invece del numero di posizione.

✅ **Nessuno di questi quattro blocca il lavoro delle issue A · B · C**: cliccabilità, routing, identità,
diagnostica e gate si correggono sul modello attuale. Ciò che la decisione blocca è il **layout definitivo**.

---

# 6. Verifica di questo referto

| Gate | Esito |
|---|---|
| Compile | `N/A` — nessun file di `Source/` toccato |
| Automation | `NOT RUN` — nessun test aggiunto; le asserzioni di §2.1 e §2.2 sono **lette**, non eseguite |
| Determinism · Replay · Privacy | `N/A` |
| PIE | `N/A` — la seduta `U49` è citata, non rieseguita |

⚠️ **Ciò che questo referto NON ha misurato**, e che va detto perché due delle issue vi appoggiano:

* il **grafo** di `WBP_RT_ActionDock` e `WBP_RT_ActionSlot` — un `.uasset` non si legge come testo, e il
  delta `f11b7b1a` ne ha toccato uno. Quale valore il `ForEach` passi ad `ArmKitAbility` — `AbilityIndex` o
  la posizione del ciclo — **è la domanda aperta di §2.1**, e la risposta sta dentro l'asset;
* la `Visibility`, la focusability e lo Z-order effettivi dei riquadri: stessa ragione;
* se `ArmKitAbility` sia **davvero** collegato nel grafo: `RefactorTactics.PlayerInput.TheDockPortArmsAndDisarms`
  prova la **porta**, non il cablaggio.
