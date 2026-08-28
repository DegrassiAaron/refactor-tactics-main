# Reaction Outcome Preview · E14 — spec panel sull'handoff v0.2

> `CURRENT` · **Stato**: revisione chiusa. Il sorgente è **consumato e archiviato**, non applicato ·
> **Data**: 2026-08-28
> **HEAD della revisione**: `e3911eed` (`main`)
> **Oggetto**: `CLAUDE_RefactorTactics_ReactionOutcome_Roadmap_Handoff_2026-08-28_v0.2.md` (untracked,
> 1436 righe), letto contro `Source/`, la DoD lato server di
> [`#166`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166), il Decision Log e le due viste
> di esecuzione scritte **lo stesso giorno**.
> **Panel**: Wiegers (lead) · Adzic · Cockburn · Fowler · Nygard · Crispin
> **Modo**: critique
> **Archiviato in**: [`../../archive/src/handoff/2026-08-28-reaction-outcome-preview-e14.md`](../../archive/src/handoff/2026-08-28-reaction-outcome-preview-e14.md)

---

## 1. Il verdetto in una riga

L'handoff è **disciplinato sui divieti e scaduto sulle premesse**: i suoi venti «errori da non
reintrodurre» reggono quasi tutti e la sua sezione side-effect-free è la meglio scritta del documento, ma
**quattro delle premesse su cui poggia la roadmap R0–R9 sono state falsificate prima che il documento
fosse scritto** — e due di esse lo sono state il 2026-08-28, in questo repository, da documenti che
l'handoff non cita.

| | Voci |
|---|---:|
| 🔴 Critico | **4** |
| 🟠 Alto | **5** |
| 🟡 Medio | **5** |

**Raccomandazione operativa**: **non eseguire la roadmap R0–R9 come scritta.** Il suo primo passo di
lavoro apre una issue chiusa; il secondo costruisce un'invariante che una revisione di oggi ha già
dichiarato non falsificabile; il terzo poggia su un tipo che non esiste e che il documento presenta come
riuso. Ciò che regge — i divieti, la privacy, il contratto side-effect-free, la correzione di ownership del
Time Bank — vale la pena di essere trasferito negli owner reali, che sono quattro file già esistenti.

⚠️ **Questa revisione non ha eseguito né suite né build**, e non ha scritto su GitHub. Le issue sono state
lette lato server con `gh` a `e3911eed`; `Source/` con `git grep` sullo stesso `HEAD`.

---

## 2. 🔴 C1 — Il primo passo di lavoro apre una issue chiusa

§18 mette `R1 — Replay seam prima della finestra umana (#886)` come **primo lavoro dopo l'audit**; §5 la
disegna come nodo attivo del dependency graph; §16 la dichiara `**Replay prerequisite**: #886` nel body
della child issue proposta; §29 la elenca sotto `TODAY / FIRST PASS`.

**Misurato**:

```text
#886  CLOSED  «Le decisioni di reazione tornano dal TurnLog: il Verifier non le richiede»
```

**Cockburn**: quattro sezioni indipendenti assegnano lavoro a un attore che non esiste più. Non è un
refuso: `R1` porta cinque criteri di uscita (`lookup per OpportunityId`, `collapsed window ricalcolata`,
`orphan/illegal/missing`, `live decider non interrogato`) e una sessione che li esegue riapre un percorso
già chiuso, misurando come lacuna ciò che è verde.

🔴 **E l'handoff possiede già la domanda giusta, nel posto sbagliato.** §22 chiede: *«#886 è ancora aperta
al momento del lavoro?»*.

**Wiegers**: una domanda aperta che contraddice il piano scritto quindici sezioni più sopra non è una
domanda — è un difetto non risolto, spostato a valle. La regola è che la roadmap si scriva **dopo** aver
risposto, non che porti la risposta in appendice: chi legge in ordine ha già cominciato `R1` quando arriva
alla §22.

**Correzione**: togliere `R1` dalla sequenza, registrare `#886` come **acquisito**, e ripuntare i suoi
cinque criteri come *verifiche di non regressione* sul seam esistente.

---

## 3. 🔴 C2 — L'invariante centrale è già stata dichiarata non falsificabile, oggi

§7 pone come **invariante principale**:

```text
Confirmed preview(Response R, Boundary B) = Committed outcome(Response R, Boundary B)
```

e §16 ne deriva il primo test del corpus, `ReactionPreview.ConfirmedMatchesCommittedOutcome`.

**Fowler**: la stessa formulazione è stata smontata il **2026-08-28** in
[`dir-b-core-gameplay-directive-spec-panel-2026-08-28.md`](dir-b-core-gameplay-directive-spec-panel-2026-08-28.md)
§7, su tre assi che valgono qui identici — **uguaglianza di quali campi**, **come si osserva un commit
senza commettere**, **chi marca un input `Uncertain`**. Il terzo è quello che uccide il test: senza un
predicato definito, l'esenzione assorbe qualunque divergenza e l'invariante non fallisce mai.

L'handoff **peggiora** il difetto rispetto alla direttiva che lo precede, perché §7 aggiunge una seconda
clausola di esenzione — *«se esistono input autorizzati non ancora risolti, il risultato NON va etichettato
Confirmed»* — senza dire chi decide che un input è «non ancora risolto». Due esenzioni non definite invece
di una.

**Correzione**, già scritta e non recepita: **una funzione pura, due chiamanti.** Non esiste «la formula del
preview»; esiste la formula del commit, e il preview la chiama. L'invariante diventa strutturale:

- stesso snapshot + stessa response → stesso valore (test di purezza);
- nessuna seconda occorrenza della formula nel percorso di commit (verifica per lettura).

**Adzic**: e le quarantasette righe della §7 non contengono **un solo numero verificabile**. L'unico
esempio — `HIT · 14 DAMAGE / Cover: -2` — non dice da quale danno base i 14 derivino, quindi non è
eseguibile: è un mockup di UI presentato come specifica.

---

## 4. 🔴 C3 — `AppliedDamage` non esiste, e §8 lo presenta come riuso

§8 ordina di mostrare `AppliedDamage` e non `CatalogDamage`/`BaseDamage`, e prescrive di *«riusare il
percorso puro/autorevole delle mitigazioni già determinabili»*. §15 attenua: *«Prima cercare tipi
equivalenti a HEAD»*.

**Misurato**:

```text
git grep -c AppliedDamage -- Source   ->  0
git grep -c AppliedDamage -- docs     ->  9 occorrenze, 2 file
```

E i due file sono **entrambi del 2026-08-28**, entrambi in questo repository, ed entrambi dicono la stessa
cosa:

| File | Cosa dice |
|---|---|
| [`dir-b-core-gameplay-directive-spec-panel-2026-08-28.md`](dir-b-core-gameplay-directive-spec-panel-2026-08-28.md) §8 | *«`AppliedDamage` non esiste, quindi non è un riuso»* |
| [`../roadmap-main-v0.1.md`](../roadmap-main-v0.1.md) **P4** | *«`B1` · `AppliedDamage` è un tipo su cui appoggiarsi» → ⛔ **non esiste*** |

**Wiegers**: la clausola *«cercare tipi equivalenti»* ha, nel testo, la funzione di far sembrare la sezione
un allineamento a ciò che c'è; copre invece **l'introduzione di una struttura nuova** in un checkpoint v0.1
la cui DoD non la chiede (vedi `A5`). Il documento deve scegliere: o dichiara `AppliedDamage` come dato
nuovo con l'autorizzazione di scope che lo copre, o lo toglie.

🔴 **Il rilievo che conta non è il tipo mancante: è che la ricerca non è stata fatta.** §3 dell'handoff
prescrive la gerarchia *codice → issue → ADR → roadmap → handoff*, e §20 consegna dieci `git grep` fra cui
esattamente `git grep -n "AppliedDamage"`. Se quel comando fosse stato eseguito su questo repository, la §8
non esisterebbe nella forma in cui è scritta.

### C3 risolto — il valore esiste, il nome no, e la formula è duplicata

> Misurato a `483e031a` dopo la stesura del referto, per rispondere alla domanda che `C3` lasciava aperta:
> *«dato nuovo da autorizzare, o da togliere?»* La dicotomia era **falsa**.

Dentro `ApplyReactionDecision` (`Turn/RTTurnManager.cpp`) il danno effettivo è già calcolato e ha già un
nome:

```cpp
const int32 Reduction = BoundaryCoverReduction(Map, WatchOwner, Target, Target->Cell);
const int32 Dealt     = FMath::Max(0, Armed.Damage - Reduction);
const FRTDamageResult Result = URTCombatLibrary::ApplyDamage(Dealt, ERTDamageSource::Direct, …);
Target->ApplyCombatState(Result.Health, Result.Shield);
Entry.Amount = Dealt;  // il danno EFFETTIVO: con la copertura diverge da quello dichiarato (#888)
```

Le quattro domande aperte della §22 dell'handoff hanno quindi risposta **nel codice**:

| Domanda §22 | Risposta misurata |
|---|---|
| quale funzione produce il danno effettivo di un boundary shot? | `BoundaryCoverReduction` → `Armed.Damage - Reduction` → `URTCombatLibrary::ApplyDamage` |
| Shield/Armor sono già nel percorso di commit? | **sì**: `ApplyDamage` è statica e pura, prende `Shield` · `TemporaryShield` · `Health` |
| `HealthDamage`/`ShieldDamage` separati o aggregato? | la struttura esiste già: `FRTDamageResult { Health, Shield, TemporaryShield }` |
| la regola è autorizzata? | **decisa e chiusa**: `D-189` · ADR-0004 §8-bis · [`#888`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/888) **CLOSED** |

**Decisione: `AppliedDamage` si TOGLIE come nome/dato nuovo.** Sarebbe un **terzo** termine per un numero
che ne ha già due — `Dealt` in codice, `Amount` nel TurnLog — e `#1392` lo dichiara a parole: *«`Entry.Amount`
porta ora il danno effettivo su entrambi i percorsi»*. Nessuna meccanica da autorizzare: la regola è chiusa.

⚠️ **Ma togliere il nome non chiude il lavoro, e questa è la parte che `C3` non vedeva.** La formula è
**copiata in due punti** — `ResolvePredictiveBoundary` (con `Armed.LockedCell`) e `ApplyReactionDecision`
(con `Target->Cell`) — e `BoundaryCoverReduction` vive in un **anonymous namespace**: **zero** occorrenze in
`RTTurnManager.h`, quindi non è chiamabile fuori dal `.cpp`. La preview sarebbe il **terzo** chiamante di una
formula che non si può chiamare. L'estrazione è quindi «una funzione pura, N chiamanti» — la correzione di
`C2` — e **riduce** duplicazione esistente invece di aggiungere codice.

⛔ **E l'estrazione non è libera: `#1392` porta una decisione a monte, non presa.**

> *«Con `EffectiveCoverReduction` pura, chi chiama deve poter sapere **se c'era una riduzione che è stata
> annullata** — oggi non può, e cambiare quella funzione tocca ogni consumatore: la forma va scelta, non
> assunta.»*

Estrarre ora e scegliere la forma dopo significa rifare l'estrazione.

**Ordine operativo**:

1. togliere `AppliedDamage` dal DTO e dal vocabolario; la preview espone `Amount`, e `FRTDamageResult` se
   serve la decomposizione;
2. **prima o insieme** — chiudere la **parte 1** di [`#1392`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1392)
   (la forma che distingue «nessuna copertura» da «copertura annullata dal facing»): è l'unica delle sue tre
   parti lavorabile senza Editor, e la issue stessa lo dichiara;
3. **poi** estrarre la formula dei due boundary in una funzione pura, con i due chiamanti esistenti come
   primo test di parity; la preview diventa il terzo chiamante senza costo aggiuntivo.

✅ **Il passo 2 è deciso: la parte 1 di `#1392` ENTRA PRIMA** *(decisione d'autore, 2026-08-28)*. L'ordine
non è più condizionale: la forma del ritorno che distingue «nessuna copertura» da «copertura annullata dal
facing» si sceglie **prima** dell'estrazione, e l'estrazione la eredita già decisa. Il costo evitato è
rifare l'estrazione su ogni consumatore di `EffectiveCoverReduction` — che è precisamente ciò che `#1392`
avverte quando dice *«cambiare quella funzione tocca ogni consumatore»*.

⚠️ **Conseguenza sulla sequenza**: il primo lavoro di codice della Outcome Preview **non** è la query pura,
è la parte 1 di `#1392` — voce di TurnLog con il precedente di `#649` (`RearHitBypassedCover`), lavorabile
senza Editor. `R2` dell'handoff va quindi dopo, non prima.

**Nessun `D-nnn` assegnato**: la decisione applica `D-189` e `#888`, non ne introduce una. Il dedup è stato
fatto — nessuna issue traccia oggi questa estrazione.

---

## 5. 🔴 C4 — R0–R9 è una terza numerazione per un lavoro che ne ha già due

§18 si apre dichiarando: *«Questa NON è una nuova roadmap parallela. È l'ordine proposto da riportare negli
owner reali.»* Poi produce dieci fasi con un vocabolario proprio — `R0` … `R9` — che non ha corrispondenza
con nessuno dei due sistemi che il repository usa per lo stesso lavoro:

| Sistema | Dove vive | Come chiama questo lavoro |
|---|---|---|
| Checkpoint | `roadmap-v0.1.md`, issue | `CP 14.6` · `CP 14.7` · `CP 14.8` |
| Wave | [`../roadmap-main-v0.1.md`](../roadmap-main-v0.1.md), **creata il 2026-08-28** | `B0` reaction outcome preview · `B1` reaction query API / reason code / preview read-only · `B2` replay decision verifier |
| **Handoff v0.2** | untracked | `R0` … `R9` |

**Cockburn**: la vista `B0`–`B6` è stata scritta lo stesso giorno, dichiara la propria base di misura
(`HEAD f20c94d9`, issue lette lato server), e porta per ogni wave le **sedi verificate con `gh`**. È già
l'«ordine da riportare negli owner reali» che la §18 dice di voler produrre. Due numerazioni per lo stesso
lavoro non sono ridondanza: sono la condizione in cui due sessioni si dichiarano a metà di fasi diverse e
nessuna delle due può leggere lo stato dell'altra.

⚠️ **E la vista che esiste è più severa di quella proposta.** `roadmap-main-v0.1.md` **P3** misura che il
titolo di `#166` è *«Counterplay, UI della finestra e misura del pacing»* e conclude: *«una wave che
consegna la sola preview lascia il checkpoint aperto»*. `R4` dell'handoff consegna esattamente la sola
preview più la UI, e dichiara come exit *«PIE human can arm + receive + answer live reaction»* — che non
include il pacing p50/p90 su ≥ 10 partite che la DoD chiede.

**Correzione**: sostituire §18 con un rinvio a `roadmap-main-v0.1.md` §4 lane `DIR-B`, e portare negli owner
solo i contenuti che quella vista non ha — che sono i divieti della §13 e il test di privacy della §11.

---

## 6. 🟠 A1 — Il seam del decisore è atterrato, e l'handoff non nomina nessuno dei suoi tre termini

§1.4 conclude: *«E48 registra che `ReactionDecider` è ancora legato dallo scenario/harness e il default
runtime umano ricade su `HoldNoDecider` finché #166 non chiude il percorso»*, e §6 ne deriva *«non iniziare
dalla UMG isolata: prima chiudere il seam runtime input/decisione»*.

**Misurato in `Source/`**:

| Simbolo | Dove | Cosa prova |
|---|---|---|
| `#512` | issue, lato server | **CLOSED** — *«CP 15.3 metà B — le decisioni di finestra come dato, e il `DecisionProvider` iniettabile»* |
| `RefactorTactics.ShowcaseRelay.DecisionProviderIsInjectable` | `RTShowcaseScenarioTests.cpp` | il seam del decisore **esiste** |
| commento `#512` fase B | `RTScenarioCorpusTests.cpp` | *«`Spec.Overwatch.HoldThenFire` non è più una specifica in attesa: si esegue»* |
| `TM->ReactionDecider.BindRaw(...)` | `RTScenarioSession.cpp` | il binding è codice vivo, non un progetto |

**Cockburn**: la frase è vera sul **runtime di partita** e falsa su ciò che ne segue. Il seam non è da
costruire: è da **collegare a un input umano**. La differenza è l'intero contenuto di `R2`, che l'handoff
dimensiona come lavoro di estrazione quando è lavoro di binding.

⚠️ **È il secondo documento consecutivo a mancare gli stessi tre termini.** Né `#512`, né `HoldThenFire`, né
`DecisionProvider` compaiono nell'handoff — esattamente come non comparivano nella direttiva DIR-B, dove il
rilievo è già registrato come `A1`. Due kit d'autore scritti a un giorno di distanza hanno lo stesso punto
cieco, e sono i tre termini con cui questo repository parla del problema.

---

## 7. 🟠 A2 — `Certainty` a tre valori riapre un difetto pagato il 2026-08-16

§10 propone il vocabolario `CONFERMATO | PREVISTO | INCERTO`, §15 lo mette nel DTO come campo `Certainty`,
e §22 chiede: *«esiste già un tipo condiviso per Certainty?»*.

**La risposta è nel codice, ed è doppia.**

`ERTIntentCertainty` esiste (`Turn/RTIntentPrivacyLibrary.h`, CP 11.2) e ha **quattro** valori, non tre —
`Unknown` a **zero** più i tre disegnabili. Il commento dichiara perché, ed è una lezione già pagata:

> *«Fino al 2026-08-16 il valore zero era `Confirmed`, cioè la garanzia PIÙ FORTE del dominio, e un
> initializer C++ non bastava a difenderlo: […] un `FMemory::Memzero`, un `SetNumZeroed`, una struct che UHT
> marca `WithZeroConstructor`: tutti leggevano «collegamento certo» per un campo che nessuno aveva
> calcolato.»*

**Nygard**: un `Certainty` a tre valori nel DTO di preview rimette `Confirmed` a zero, e qui il costo è più
alto che sugli intenti. `CONFERMATO` non è un'etichetta: è la promessa di parity col commit che la §7 pone
come invariante principale. Un campo azzerato per costruzione promette parity su un valore mai calcolato —
e il test `ConfirmedMatchesCommittedOutcome` non lo vedrebbe, perché confronta il valore, non la sua
provenienza.

**E la seconda metà della risposta**, in `UI/RTIconLibrary.h`: il progetto ha **già deciso** di non creare
un tipo generico —

> *«È l'unica categoria senza un tipo da cui derivare: un `ERTCertainty` creato adesso sarebbe un enum che
> nessuno legge. Tre costanti che il catalogo deve coprire valgono più di un tipo senza consumatori.»*

Quella decisione ha una condizione d'innesco — «finché nessuno legge» — e un DTO di preview con un campo
`Certainty` **è** il consumatore che mancava. La domanda §22 è quindi legittima, ma va posta come *«la
condizione di `RTIconLibrary.h` è scattata?»*, non come *«esiste un tipo?»*: la prima ha una risposta
azionabile, la seconda produce un enum nuovo accanto a uno esistente.

---

## 8. 🟠 A3 — La categoria di test proposta sarebbe la settima per lo stesso sottosistema

§16 e §19 propongono nove test sotto `RefactorTactics.ReactionPreview.*`.

**Crispin**: la convenzione è rispettata — `RefactorTactics.<Categoria>.<CamelCase>`, zero underscore — ed è
un miglioramento netto rispetto al kit precedente, che usava `ReactionPreview_Hit`. Il problema è la
**categoria**. Misurate in `Source/RefactorTactics/Tests/`:

| Categoria esistente | Occorrenze |
|---|---:|
| `RefactorTactics.Scenario.` | 116 |
| `RefactorTactics.Replay.` | 54 |
| `RefactorTactics.Reactions.` | 52 |
| `RefactorTactics.Overwatch.` | 17 |
| `RefactorTactics.Predictive.` | 8 |
| `RefactorTactics.Preview.` | 4 |
| `RefactorTactics.Reaction.` | 2 |
| **`RefactorTactics.ReactionPreview.`** | **0 — sarebbe nuova** |

`Reaction.` e `Reactions.` si distinguono già per una `s`; `Preview.` esiste e porta quattro test
(`HitCellsMatchCombatShape`, `ClearedWhenPlanIsCancelled`, `AllyInAreaIsFlagged`,
`ReachableCellsArePassedThrough`) che sono **la stessa famiglia semantica**. Aggiungerne una che concatena
due categorie esistenti rende il corpus non selezionabile per prefisso: `-Filter RefactorTactics.Reaction`
non prenderebbe i nuovi, e `RefactorTactics.Preview` nemmeno.

**E tre voci sono già coperte**, contro la regola *search → reuse → create* che l'handoff stesso prescrive:

| Voce proposta | Stato reale |
|---|---|
| `ReactionPreview.NoFutureIntentLeak` | `Overwatch.OpportunityLeaksNoFuture` — esistente, `RTReactionOpportunityTests.cpp` |
| `ReactionPreview.ClashDoesNotRevealHiddenChoice` | forma già in uso: `UI.NoEnemyIntentExposed`, *«due scene che differiscono SOLO per i piani nemici»* — è **letteralmente** il test che §11 descrive |
| `ReactionPreview.StalePlanningPreviewDoesNotGrantOutcome` | §19 la duplica sotto `Cover.StalePreviewDoesNotGrantTheShot` per `#1562` — due nomi, un oracolo |

---

## 9. 🟠 A4 — §12 riscrive l'owner del Time Bank senza nominarlo, e nel farlo perde i numeri

§12 detta sei regole su Bank, Grace, `ExhaustedGrace`, `PreferredResponse` e subject del bank. **L'owner
esiste**: [`../../gameplay/spec-decision-time-bank.md`](../../gameplay/spec-decision-time-bank.md), che
l'handoff non cita **mai** — e che possiede già, con più precisione:

| Cosa | Dove |
|---|---|
| `InitialBank = RoundLimit × (MaxWindow − Grace) × LoadFactor(ControlledHeroes)` | §3.4, decisione `D-156` |
| `ControlledHeroes` e `InitialBankMs` **nell'header** del TurnLog, registrati e non ricalcolati | §10 |
| Finestre dello stesso giocatore **in serie**, cap aggregato al bank | §17, decisione `D-167` |
| Checklist di chiusura | §checklist, riga 812 |

**Wiegers**: e la perdita è misurabile. `D-156` deriva per la v0.1 — due Hero, `LoadFactor(2) = 1,75`,
`RoundLimit = 12` — un bank di **31,5 s**, con `Grace 1,50 s` e timeout `1,50 s`. L'esempio §7
dell'handoff mostra `Time Bank: 18.4 s`, un numero che non appartiene a nessuna derivazione del progetto e
che nessun lettore può ricondurre a una fonte.

✅ **Ma la §12 ha ragione sul punto che conta, ed è il contributo migliore del documento.** La correzione di
ownership — `UnitsPerPlayer` è formato, il **subject runtime** è di `#319` e non va rinviato a CP 19.3 — è
esatta, corrisponde a [`#1206`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1206)
(OPEN), e il nome che propone, `ControlledHeroes`, **è già il nome della spec owner**. Non è un termine da
inventare: è da portare in `Source/`, dove le sue occorrenze sono **zero** — la premessa negativa di
`D-155` regge ancora sul codice, ed è scaduta solo sui documenti.

---

## 10. 🟠 A5 — La preview non è nella DoD di #166, e §17 propone di aggiungercela

§17 prescrive di *«estendere la Decision Window»* di `#166` con `Target`, `HitState`, `AppliedDamage`,
`Certainty` e breakdown.

**DoD reale di `#166`, letta lato server** — le caselle aperte rilevanti:

```text
[ ] UI FIRE/HOLD con countdown e bersaglio, DTO sanitizzato: nessuna logica di gioco nel widget
[ ] countdown = FastReactionDuration 3,0 s, server-authoritative; allo scadere HoldTimeout, mai FIRE
[ ] slow-motion sola presentazione
[ ] la finestra è visibile in sola lettura alla squadra; l'avversario non riceve nulla
[ ] ReactionDecisionSeconds misurata con 1, 2 e 3 unità armate — DELLO STESSO GIOCATORE (D-167)
[ ] p50 e p90, campione >= 10 partite
[ ] i numeri sono consegnati come taratura di InitialBank e Grace
```

**Nygard**: `HIT`, `MISS`, danno, `AppliedDamage` e `Certainty` **non compaiono**. Quattro delle caselle
aperte riguardano la **misura del pacing**, che l'handoff nomina una volta sola (`R8`) e colloca dopo cinque
fasi di costruzione.

Il rischio operativo è preciso: `#166` è un checkpoint **v0.1**, e la §17 gli aggiunge scope mentre la sua
DoD è aperta sulle voci di misura. Chi esegue l'handoff consegna una preview e lascia il checkpoint dov'era
— che è la conclusione già scritta come **P3** in `roadmap-main-v0.1.md`.

**Correzione**: la Outcome Preview non estende `#166`. O sta nella child issue che §16 propone — con la sua
DoD e senza rivendicare la chiusura di CP 14.6 — o non sta.

---

## 11. 🟡 I cinque rilievi medi

| # | Sezione | Rilievo | Evidenza | Correzione |
|---|---|---|---|---|
| **M1** | intestazione | Dichiara di superare `CLAUDE_..._2026-08-27.md`, che **non è mai entrato nel repository**: la catena di supersessione non è verificabile e il predecessore non è recuperabile | `git log --all` sul path: vuoto | Dichiarare che il predecessore era untracked; è la stessa lezione di `M5` del panel DIR-B, pagata due volte |
| **M2** | §20 | I dieci comandi di audit usano il pathspec `-- Source Tests docs`, e **`Tests/` non è una directory di questo repository**: i test stanno in `Source/RefactorTactics/Tests/` | `ls -d Tests` → *No such file* | Togliere il pathspec morto. Non rompe il comando (`Source` matcha), ma è il segnale che i comandi non sono stati eseguiti qui — che è ciò che `C3` misura |
| **M3** | §25 | `preview < 50 ms` è ereditato da `RT_PDR_v0.1_consolidato.md`, dove è definito come *«input → server → ally render»*: un percorso di rete, non una query pura locale | `docs/archive/pdr-v0.1/RT_PDR_v0.1_consolidato.md` | Se la soglia serve, ri-derivarla per la query. Applicare un budget end-to-end a una funzione pura è un cambio di soggetto, e l'unica fonte è **in archivio** |
| **M4** | §1.7 | Sul write GitHub la cautela è giusta e va confermata: la **lettura** funziona (dodici issue lette con `gh` in questa sessione, a `e3911eed`), la **scrittura** resta non testata | questa revisione | Nessuna. È l'unica sezione del documento che tratta correttamente una premessa come scaduta invece che come fatto |
| **M5** | §16 | Il body della child issue porta cinque numeri di issue come prerequisiti (`#886`, `#319`, `#314`, `#1118`, `#1392`): un body con numeri letterali invecchia da solo, e uno dei cinque è già chiuso | `C1` | Sostituire i prerequisiti con criteri verificabili; il numero resta come link, non come stato |

---

## 12. Cosa regge, misurato

Voci controllate che **non** hanno prodotto rilievi, elencate perché una revisione che nomina solo i difetti
non dice quanto ha guardato:

| Proprietà | Come regge |
|---|---|
| §13 **preview side-effect-free** | Undici divieti (`mutare HP`, `spendere charge`, `cambiare seed`, `aprire nuove Reaction Window`…), ciascuno falsificabile **singolarmente**, più la prescrizione giusta: *«la suite deve confrontare hash/stato prima e dopo»*. È la sezione migliore del documento |
| §11 **no side channel** | Il test di privacy è formulato come oracolo eseguibile — *«stesso stato pubblico, hidden choice A vs B ⇒ payload identico»* — ed è esattamente la forma di `UI.NoEnemyIntentExposed`, già in `Source/` |
| §1.3 **correzione #1206** | Misurata: `#1124` è CP 19.3 e porta `UnitsPerPlayer` (vivo in `RTMatchFormatData.h`); `#1206` è **OPEN** e dice ciò che l'handoff riporta. La premessa che smonta — *«#319 è bloccata perché CP 19.3 deve inventare il soggetto»* — è effettivamente falsa |
| §1.5 **#1158 CLOSED** | Confermato lato server, e l'uso che ne fa è quello giusto: precedente di correttezza sulle opportunity seriali, non backlog |
| §1.1 / §17 **#1562 resta post-v0.1** | Confermato: la issue porta la label `post-v0.1`. E §17 *«NON spostarla in v0.1 e NON trasformarla in prerequisite»* è la disciplina corretta verso una issue appena aperta |
| §14 **multi-target** | `preview = current opportunity + current boundary state`, con `#1158` come precedente e il divieto di precomputare. Regge, ed è argomentato dal caso reale |
| §23 **errori da non reintrodurre** | Venti divieti; quelli verificabili reggono. `NO CP14.9 inventato`, `NO Reaction as fifth phase` e `NO Feature Registry/parallel-batch resurrected` sono allineati al canone (`D-178`, `D-181`, `D-182`) |
| §1.6 **dedup** | ✅ **Riverificato in questa sessione**: `gh issue list --search` su *reaction preview outcome*, *"outcome preview"* e *"damage preview"* non restituisce una sede equivalente. La conclusione dell'handoff è corretta |

---

## 13. Cosa fare, in ordine

| # | Azione | Dove | Blocca l'esecuzione? |
|---|---|---|---|
| 1 | Registrare `#886` come acquisito; togliere `R1` dalla sequenza (`C1`) | §5, §16, §18, §29 | **sì** |
| 2 | Sostituire l'invariante con «una funzione pura, due chiamanti» più il predicato di `Uncertain` (`C2`) | §7, §16 | **sì** |
| 3 | Dichiarare `AppliedDamage` come dato **nuovo**, o toglierlo (`C3`) | §8, §15 | **sì** |
| 4 | Sostituire R0–R9 con il rinvio a `roadmap-main-v0.1.md` §4 lane `DIR-B` (`C4`) | §18, §29 | **sì** |
| 5 | Registrare `#512`, `DecisionProvider` e `HoldThenFire`: il seam è da collegare, non da costruire (`A1`) | §1.4, §6, §18 | no, ma cambia il residuo |
| 6 | Portare `Certainty` a quattro valori con lo zero non disegnabile, o riusare `ERTIntentCertainty` (`A2`) | §10, §15 | no |
| 7 | Rinominare il corpus sotto `RefactorTactics.Preview.*` o `.Reactions.*`; togliere le tre voci già coperte (`A3`) | §16, §19 | no |
| 8 | Rinviare §12 a `spec-decision-time-bank.md` §3.4/§10/§17; conservare la sola correzione di subject (`A4`) | §12 | no |
| 9 | Togliere la preview dall'estensione di `#166`; lasciarla alla child issue (`A5`) | §17 | no |
| 10 | I cinque medi (`M1`–`M5`) | intestazione, §16, §20, §25 | no |

⚠️ **Nessuna azione tocca il codice**, per la stessa ragione del panel fratello: il documento non ha difetti
di implementazione perché non è stato implementato. Correggerlo ora costa dieci edit di testo; eseguirlo
come scritto costa una sessione che riapre `#886`, introduce un tipo non autorizzato e lascia `CP 14.6`
dov'era.

---

## 14. Nota di regime

**Perché quasi nessun riferimento porta un numero di riga.** Le ancore di questo referto sono **simboli e
ID**, che sopravvivono a un commit altrui; `D-222` è il regime, e in questa working directory `HEAD` si è
mosso quattro volte durante la stesura del referto fratello.

**Nessun `D-nnn` riservato.** La revisione non introduce una regola né supera una decisione: applica
`D-155`, `D-156`, `D-167` e la DoD di `#166` a un documento che le ignora o le riscrive con meno precisione.

**Nessuna scrittura su GitHub.** La child issue proposta dalla §16 **non è stata creata**: il mandato di
questa sessione era consumare e archiviare, e `CLAUDE.md` §9 vieta l'apertura non richiesta. Il dedup della
§1.6 è però stato rifatto e regge, quindi la proposta resta valida per chi la eseguirà — con il body
corretto secondo `C1` e `M5`.

⛔ **Il sorgente non è stato modificato.** È stato archiviato verbatim in
[`../../archive/src/handoff/2026-08-28-reaction-outcome-preview-e14.md`](../../archive/src/handoff/2026-08-28-reaction-outcome-preview-e14.md),
sotto un preambolo di verdetto. È la lezione di `M1` applicata a sé stessa: un documento di regime che
nessuno traccia non ha nessuno che lo difenda, e questo è il **secondo** della serie ad arrivare senza un
predecessore recuperabile.
