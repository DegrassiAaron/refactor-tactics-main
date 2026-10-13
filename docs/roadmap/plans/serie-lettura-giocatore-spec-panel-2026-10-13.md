# La serie «ciò che il giocatore legge» — spec panel su sei issue della v0.1

> **Stato**: `CURRENT` · **Creato**: 2026-10-13 · **Tipo**: referto di spec panel — **vista di esecuzione**, non owner.
>
> **Oggetto**: le sei issue proposte come serie il 2026-10-13. Quattro toccano `Source/`:
> [#3081](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3081),
> [#3400](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3400),
> [#3629](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3629) e
> [#3667](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3667). Due toccano solo `tools/hud-assets/`:
> [#3626](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3626) e
> [#3627](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3627).
>
> **Base di misura**: `origin/main` = `68f9bd391`, in un worktree separato dal clone principale.
> **Nessuna build, nessuna Automation, nessun PIE**: il motore era in uso da un'altra sessione. Ogni affermazione
> sul codice è marcata **letto** (dal sorgente) o **eseguito** (un comando ha risposto). §9 dice che cosa
> falsificherebbe le letture.
>
> **Modalità**: `critique`. **Panel**: Wiegers (requisiti e criteri), Adzic (esempi falsificabili), Fowler
> (struttura e fonte unica), Nygard (modi di guasto), Crispin (evidenza e test).
>
> **Cosa non è**: una decisione, né una riscrittura delle issue. I corpi delle sei issue non sono stati toccati.
> Le quattro scelte di §7 sono dell'autore: qui si rendono prendibili. Dove questo referto e un owner divergono,
> ha ragione l'owner.
>
> **Convenzione sui numeri** ([`AGENTS.md`](../../../AGENTS.md) §14): nessun totale che cambi da solo. Gli insiemi
> sono scritti per nome; i numeri rimasti sono valori di gioco letti dal sorgente o esiti di questo passaggio.

---

**Indice** · [0. Il verdetto](#0-il-verdetto-in-breve) · [1. #3081](#1-3081--i-sei-esiti-di-movimento-che-il-feed-non-racconta) ·
[2. #3400](#2-3400--la-destinazione-di-scatto-non-è-un-ordine) · [3. #3629](#3-3629--i-nomi-delle-azioni-del-core) ·
[4. #3667](#4-3667--titolo-sottotitolo-e-bersaglio-nella-finestra-di-reazione) ·
[5. #3626](#5-3626--color_metricspy-non-gira) · [6. #3627](#6-3627--la-copia-python-della-palette) ·
[7. Decisioni](#7-decisioni-richieste) · [8. Ordine](#8-lordine-e-i-vincoli) · [9. Limiti](#9-limiti-dichiarati) ·
[10. Gate](#10-gate-di-questo-passaggio)

---

## 0. Il verdetto in breve

Le sei issue descrivono fatti veri, ma **quattro di esse hanno una premessa o un criterio che non regge più**. Due
si possono chiudere rispettando alla lettera i propri criteri senza che il giocatore veda niente di diverso: è il
rilievo più importante del panel.

| Issue | Il corpo regge? | Che cosa cambia rispetto al corpo | Aspetta l'autore? |
|---|---|---|---|
| #3081 | in parte | uno dei sei esiti non ha più un produttore; mappare le cadute su `Moved` non cambia una riga del feed, perché il danno da caduta la copre | sì, `P1` — solo per il testo |
| #3400 | sì, con una correzione | un turno di solo scatto oggi conta un ordine, non zero; la riga di log dell'arm esiste già | sì, `P2` — una conferma |
| #3629 | il fatto sì, l'effetto no | il ripiego sull'ultimo segmento non ha un caso raggiungibile nel catalogo spedito | sì, `P3` |
| #3667 | sì | i riferimenti di riga sono slittati; la vista ora si costruisce prima e si consegna dopo | no |
| #3626 | sì | manca la causa: lo script fallisce forte dal 2026-09-04 e nessun comando lo lancia | no |
| #3627 | no, sul punto centrale | `Disabled` non diverge: sono due token di due vocabolari diversi | sì, `P4` |

Tre lavori non aspettano nessuna risposta e possono partire: #3626, la parte di #3081 che non riguarda il testo, e
#3667.

---

## 1. #3081 — I sei esiti di movimento che il feed non racconta

### 1.1 Il fatto, rimisurato

**Confermato** (eseguito): nel ramo `Move` di `ClassifyEntry` nessuno dei sei esiti ha un `case`.

```sh
for o in BlockedByCycle Fell FellToAlternative FellToLastStable FellWithoutLanding StoppedByEdgeGuard; do
  grep -cE "case ERTMoveOutcome::$o\b" Source/RefactorTactics/UI/RTPlayerEventProjector.cpp   # 0 per ciascuno
done
grep -cE "case ERTMoveOutcome::Slid\b" Source/RefactorTactics/UI/RTPlayerEventProjector.cpp   # 1: il comando sa colpire
```

Tre premesse del corpo sono invece cadute.

1. **«Oggi è inerte» non è più vero.** Il corpo si appoggia a
   [#2697](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697) per dire che il feed non ha
   consumatori. #2697 è chiusa dal 2026-09-12 e il feed scrive a schermo dalla seduta `U54` del 2026-09-18
   ([#2964](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2964)). Il difetto è sulla superficie
   che `G16` giudica al passo 4.
2. **`BlockedByCycle` non è «il caso più netto»: non accade più.** Il commento dell'enum in `Turn/RTTurnLog.h` lo
   dichiara: *«nessuno emette più questo valore, ed è deliberato»* (`D-453`, che chiude `MOV-13`; il produttore
   l'ha spento `D-445`). Fuori dai test l'unico uso in codice è il ramo che ne compone la stringa in
   `Turn/RTTurnLogLibrary.cpp` (letto). La issue è del 2026-09-12, le due decisioni sono di ottobre.
3. **Mappare le cadute su `Moved` non le porta nel feed.** È la premessa implicita del primo criterio, ed è falsa
   per come funziona la dominanza (letto):
   - una caduta scrive, per la stessa unità, anche una voce `Combat` con `ActionId` `Fall.Fall` e cinque danni
     (`ARTTurnManager::ApplyFallEffects`, `RTFallDamage`), e applica `Status.Exposed`;
   - la proiezione tiene **una riga per unità** e sostituisce solo se il rango è più alto
     (`DominanceRank`): `Attacked` vale 80, `StatusChanged` 50, `MoveBlocked` 40, `Moved` 20;
   - il testo dipende dal solo tipo (`URTHudViewModel::ComposePlayerEventText`): `Attacked` scrive
     «Colpita — N», `Moved` scrive «Spostata».

   ∴ dopo la correzione chiesta dal corpo, chi cade ha nel feed la riga «Colpita — 5», identica a quella di prima.
   La voce di movimento è classificata e subito coperta.

### 1.2 Il panel

**WIEGERS** — Il primo criterio (*«i sei esiti hanno un tipo e un'importanza decisi»*) misura la funzione, non ciò
che il giocatore legge. Si spunta con sei `case` e il feed resta com'è. Il secondo criterio (*«le quattro cadute
restano distinguibili nel feed, oppure la issue dichiara perché no»*) è l'unico che parla dello schermo, e oggi la
risposta onesta è che nel feed non si distingue nemmeno **che** una caduta sia avvenuta.

**FOWLER** — La causa viaggia già nell'evento. `FRTPlayerEvent` porta `ActionId`, e per il danno da caduta vale
`Fall.Fall` per chi cade e `Fall.Impact` per l'occupante colpito dall'atterraggio; il prefisso è dichiarato in
`URTTurnLogLibrary::FallCausePrefix()`. Non servono un tipo nuovo né un cambio di rango: basta che il testo della
riga superstite legga la causa. Un tipo `Fell` sopra `Attacked` rovescerebbe invece l'ordine «KO > Danno > Colpo»,
che è una regola del feed e non un dettaglio di questa issue.

**NYGARD** — `StoppedByEdgeGuard` è l'unico dei sei la cui riga sopravvive, perché il parapetto non infligge danni.
Qui il tipo conta davvero. `MoveBlocked` scriverebbe «Movimento bloccato», cioè che il piano del giocatore è
fallito: è falso, ed è l'obiezione che [#2314](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2314) ha già fatto valere per `SlideBlocked`. Lo spostamento è **subito**
e il parapetto l'ha accorciato. Prima che il valore esistesse lo stesso fatto era `Displaced` e produceva
«Spostata» (lo dice il commento dell'enum): mapparlo con `Displaced` restituisce ciò che il feed diceva già.

**ADZIC** — Gli esempi che la issue deve portare stanno in §1.4. Il primo è quello che oggi manca del tutto: una
caduta completa, con le sue tre voci, e una sola riga in uscita.

**CRISPIN** — Il test di esaustività è giusto e va scritto nella forma di
`RefactorTactics.TurnLog.EveryMoveOutcomeIsDescribed`. Da solo però è il test della funzione. Senza l'esempio della
caduta completa la suite resterebbe verde mentre il feed tace, che è la situazione di oggi con un `case` in più.

### 1.3 La specifica

- **R1.1** `BlockedByCycle` → `MoveBlocked`, `Important`, nel blocco della sua famiglia. Un commento dice che nessuno
  lo emette (`D-453`) e che il ramo esiste perché riaccenderlo non renda muto il feed.
- **R1.2** `StoppedByEdgeGuard` → `Moved`, `Important`, con `Displaced` e `DisplacementResisted`.
- **R1.3** `Fell`, `FellToAlternative`, `FellToLastStable`, `FellWithoutLanding` → `Moved`, `Important`. Sono
  spostamenti subiti. Servono per il caso in cui l'osservatore è autorizzato alla voce di movimento e non a quella
  di danno.
- **R1.4** *(dipende da `P1`)* Il testo della riga di danno nomina la causa quando è una caduta.
- **R1.5** Un test di esaustività sul ramo `Move`: ogni valore di `ERTMoveOutcome` produce un evento, oppure sta in
  un elenco di esclusi **dichiarato nel test**. Oggi gli esclusi sono `Stayed` e `SupersededByDash`.
- **R1.6** Il `default` resta, come in [#2628](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2628).
- **R1.7** La issue dichiara che i quattro esiti di atterraggio **non** si distinguono nel feed: il feed dice che
  l'unità è caduta, la board mostra dove è finita, il log diagnostico tiene i quattro testi (`DescribeEntry`).

R1.1, R1.2, R1.3, R1.5 e R1.6 non dipendono da nessuna decisione.

### 1.4 Gli esempi

1. **Caduta su atterraggio libero.** Voci dello stesso turno per l'unità 7: `Move`/`Fell`; `Combat`/`Hit` con
   `ActionId` `Fall.Fall` e `Amount` 5; il cambio di stato `Exposed`. Proiettate per la sua squadra danno **una**
   riga per l'unità 7, ed è quella del danno. Con `P1` = (b) il testo è «Caduta — 5»; con (a) resta «Colpita — 5».
2. **Occupante colpito dall'atterraggio.** `Combat`/`Hit` con `ActionId` `Fall.Impact` sull'unità 9. Con `P1` = (b)
   il testo è «Urto — 5».
3. **Spinta fermata dal parapetto.** `Move`/`StoppedByEdgeGuard` per l'unità 7, nessuna voce di danno di cui lei
   sia il soggetto. Una riga, «Spostata», `Important`.
4. **Ogni esito.** Per ogni valore di `ERTMoveOutcome` la classificazione produce un evento, tranne `Stayed` e
   `SupersededByDash`.

### 1.5 Definition of Done

- [ ] I sei esiti sono classificati come R1.1–R1.3, e togliere uno qualunque dei `case` fa cadere il test di R1.5.
- [ ] L'esempio 1 è un test, e asserisce **una** riga e il suo testo: è ciò che distingue «la funzione classifica»
      da «il feed racconta».
- [ ] L'esempio 3 è un test.
- [ ] `P1` è scritta nella issue, con il testo scelto.
- [ ] Confine di privacy: nessun campo nuovo attraversa la proiezione. La causa sta in `ActionId`, che l'evento
      porta già, su una voce che `IsAuthorized` ha già esaminato.

---

## 2. #3400 — La destinazione di scatto non è un ordine

### 2.1 Il fatto, rimisurato

**Confermato** (letto): in `ARTPlayerController::HandleClickOnCell` i due rami della mobilità rapida — quello
**lineare** e quello **a budget** (`Action.Sprint`) — scrivono `PlannedDashAbility` e `PlannedDashCell`, loggano
`[RT] Piano: … SCATTO -> …` ed escono con `return`. L'unica chiamata a `RecordPlanningInput(Order)` della funzione
sta dopo, in coda al ramo del waypoint accettato.

Due affermazioni della issue vanno corrette.

1. **Un turno di solo scatto oggi non conta zero ordini: ne conta uno** (letto). Per pianificare uno scatto bisogna
   armare l'abilità, e l'arm passa da `SelectAbilityForCurrent`, che registra già un `Order`. I siti che
   registrano un ordine sono oggi: `HandleClickOnUnit`, `HandleClickOnCell` (solo il ramo del waypoint),
   `SelectAbilityForCurrent`, `HandleTargetCell`, `HandleTargetEdge`, `HandleFacingSector`.
2. **La casella «l'arm lascia una riga di log» è già soddisfatta** (letto): `SelectAbilityForCurrent` scrive
   `[RT] <richiesta>: armata su '<unità>'`.

### 2.2 Il panel

**WIEGERS** — L'ultimo commento chiede di decidere *«una volta»* se uno scatto vale un `Order` come un waypoint o
di più, perché tre waypoint contano tre e uno scatto equivalente uno. Il contratto risponde già. `Turn/RTPacing.h`
definisce `Order` come *«il giocatore ha impartito un ordine (abilità o destinazione)»* e dichiara lo scopo del
tipo: distinguere *«sto pensando»* da *«sto litigando con l'interfaccia»*. Il contatore misura **gesti accettati**,
non lunghezza del piano. Tre waypoint sono tre gesti. Uno scatto sono due: armare e indicare la cella, come
un'abilità con bersaglio.

**ADZIC** — Con questa lettura gli esempi sono quattro e non lasciano spazio: §2.4.

**NYGARD** — La posizione della chiamata è la regola. Il test esistente
`RefactorTactics.PlayerInput.MovementDestinationCountsAsAnOrder` lo dice per il waypoint: un tentativo respinto
non è un ordine. Per lo scatto vale lo stesso, e i rami di rifiuto sono più d'uno (ricarica, portata nulla, non
allineata, terreno, fuori budget): la chiamata va **dopo** l'accettazione e **prima** del `return`, in entrambi i
rami.

**CRISPIN** — Due rami, due chiamate, due casi di prova. Con un solo caso, una delle due chiamate resterebbe senza
un test che cada quando viene tolta. Il controllo positivo dell'allestimento è `PlannedDashCell`: senza, un
conteggio rimasto fermo non distingue «la chiamata manca» da «lo scatto non è stato accettato».

### 2.3 La specifica

- **R2.1** I due rami dello scatto registrano `ERTPlanningInput::Order` dopo l'accettazione e prima del `return`.
- **R2.2** *(conferma: `P2`)* Un gesto accettato vale un ordine. Lo scatto ne produce due: l'arm e la destinazione.
- **R2.3** Una seconda destinazione valida sostituisce la prima e conta un altro ordine, come il
  ri-bersagliamento di un'abilità.
- **R2.4** Il commento sopra la chiamata del waypoint si aggiorna: non è più l'unica della funzione.

### 2.4 Gli esempi

| Gesti | `PlannedDashCell` | `OrderCount` del turno |
|---|---|---|
| arm di uno scatto lineare, poi cella allineata e in portata | scritta | 2 |
| arm di uno scatto lineare, poi cella non allineata | non scritta | 1 |
| arm di `Action.Sprint`, poi cella dentro il budget | scritta | 2 |
| arm di `Action.Sprint`, poi cella fuori dal budget | non scritta | 1 |

### 2.5 Definition of Done

- [ ] I quattro esempi sono un test, nella forma di `MovementDestinationCountsAsAnOrder`. Togliere una delle due
      chiamate fa cadere il solo esempio accettato del suo ramo.
- [ ] La casella sull'arm si spunta con la riga di log citata sopra.
- [ ] `P2` è scritta nella issue.
- [ ] `PIE-PACING-1` si rigiudica in seduta su un binario che contiene la correzione: resta aperta fino ad allora.

---

## 3. #3629 — I nomi delle azioni del core

### 3.1 Il fatto, rimisurato

**Confermato** (letto): `GenericActionDisplayName` copre `Action.Wait`, `Action.Guard`, `Action.Brace`,
`Action.Overwatch` e `Action.Interact`. Le altre azioni di `GetCoreActionCatalog()` non hanno un nome chiavato
sull'id.

**Non confermato**: che il giocatore possa vedere un'azione senza nome. Il secondo criterio dice *«gli avvisi di
piano non ripiegano più sull'ultimo segmento per un'azione del catalogo»*. La cascata `NomeDi` di
`URTHudViewModel::BuildPlanWarnings` ripiega sull'ultimo segmento solo al terzo passo, e per arrivarci serve una
voce del piano che non stia nel kit con un nome e che non sia il movimento. `URTPlanValidationLibrary::MakePlanFor`
(letto) costruisce il piano da tre voci del kit dell'unità — scatto, azione principale, reazione — più la voce di
movimento, che il secondo passo chiama già «Movimento». I nomi delle voci di kit risultano presidiati da gate:
`Catalog.EveryEquipmentActionHasADisplayName` per l'equipaggiamento, e il radar `action-descriptions.ts` per le
colonne *Nome* di eroi e generiche.

∴ nel catalogo spedito il terzo passo non ha un caso raggiungibile. Il criterio si spunta senza che una riga a
schermo cambi.

### 3.2 Il panel

**WIEGERS** — Una tabella con un nome per ogni azione di `GetCoreActionCatalog()`, che nessuna superficie mostra,
chiede all'autore di approvare testo che non leggerà nessuno. È lavoro di contenuto senza lettore, e la issue stessa dice che i nomi sono una
decisione d'autore.

**NYGARD** — Il rischio vero non è l'assenza della tabella: è il ripiego. `Counter` al posto di un nome è il modo
in cui un kit senza nome arriverebbe a schermo **senza errore**. Ciò che va protetto è la proprietà che rende il
ripiego irraggiungibile.

**CRISPIN** — I gate esistenti guardano le **fabbriche**. Il piano legge il **kit assemblato**: eroe, generiche,
equipaggiamento, varianti. Il presidio che corrisponde a ciò che il giocatore legge è un test sul kit assemblato
di ogni eroe del roster col suo loadout di default.

**FOWLER** — Se la tabella si fa, la forma è decisa dal precedente: `RTActionDescriptions`, con la tabella chiavata
per `ActionId`, la copia in [`RT_ActionDescriptions_v0.1.md`](../../balance/RT_ActionDescriptions_v0.1.md) e il
confronto del radar. Il momento giusto è quando nasce un lettore che **non ha un kit in mano**: una riga del feed
che nomini l'azione di un avversario, per esempio.

### 3.3 La specifica, nelle due forme che `P3` separa

**Forma ridotta — raccomandata.**
- **R3.1** Un test sul kit assemblato: per ogni eroe del roster col loadout di default, ogni abilità del kit ha un
  `DisplayName` non vuoto. Prima di scriverlo si verifica che un gate esistente non copra già il kit assemblato;
  se lo copre, R3.1 è un rimando e non un test nuovo.
- **R3.2** Il commento di `NomeDi` dice che il terzo passo è una rete, e nomina il test che la rende irraggiungibile.
- **R3.3** #3629 si chiude come ridotta, con il rimando alla tabella rinviata e al lettore che la farà nascere.

**Forma piena — quella del corpo.**
- **R3.4** Tabella `Action.*` → nome nella forma di `RTActionDescriptions`, documento e radar compresi.
- **R3.5** Gate su `GetCoreActionCatalog()`: nessuna azione senza nome.
- **R3.6** I nomi li propone chi implementa e li approva l'autore **prima** del merge.

### 3.4 Definition of Done

- [ ] `P3` è scritta nella issue.
- [ ] Forma ridotta: R3.1–R3.3. Forma piena: R3.4–R3.6, con l'elenco dei nomi approvato in un commento della issue.

---

## 4. #3667 — Titolo, sottotitolo e bersaglio nella finestra di reazione

### 4.1 Il fatto, rimisurato

La specifica è del panel dell'11 ottobre
([`finestra-di-reazione-seguiti-spec-panel-2026-10-11.md`](finestra-di-reazione-seguiti-spec-panel-2026-10-11.md)
§4) e **regge**. Questo passaggio non la rifà: rilegge le premesse due giorni dopo.

- `FRTReactionWindowView` porta oggi `bOpen`, `Key`, `WindowSeconds`, `Options`, `SafeResponse` (letto). Nessuno
  dei campi di R2.1 esiste ancora.
- I simboli che la specifica nomina esistono tutti: `TeamAwarenessOfCell`, `FilterWindowForTeam`,
  `BuildRouteObserverTeams`, `TargetSnapshotIndex`, il test `FastDecisionLookFollowsTheBoard` e lo scenario
  `Visual.Reaction.OverwatchWindow` (eseguito: `grep` e `ls`).
- La dipendenza [#3661](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3661) è chiusa dal
  2026-10-11.
- I numeri di riga citati dal referto sono slittati. Chi implementa cerchi le funzioni per nome.

Due cose sono cambiate.

1. **La vista si costruisce prima e si consegna dopo.** Da
   [#3666](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3666) i due siti passano la vista a
   `SuspendReactionWindowForPresentation`: *«costruito qui, consegnato dopo»*, quando il playback ha mostrato
   l'ingresso o la spinta (letto, nei commenti dei due siti).
2. **`ReactionName` esiste già in un altro DTO.** La vista degli **intenti** ha un campo con lo stesso nome
   (`Turn/RTIntentPrivacyLibrary.cpp`, riempito da `URTHudViewModel` con il `DisplayName` della reazione). Non è
   quello della finestra. La ricognizione di questa serie li aveva confusi.

### 4.2 Il panel

**NYGARD** — R2.2 dice che un nome avversario entra *«solo se la squadra del proprietario la ha `Detected` in quel
momento»*. Da #3666 i momenti sono due. Vale la **costruzione**: è l'istante in cui l'indice di snapshot è valido
e in cui il resolver conosce la rilevazione. Fra costruzione e consegna i nomi sono testo già scritto, e non si
ricalcolano.

**FOWLER** — `ReactionName` della finestra ha una fonte già usata: il `DisplayName` della voce di kit della
reazione, come fa la vista degli intenti. Stessa fonte, due DTO distinti. Non vanno fusi: hanno confini di privacy
diversi.

**ADZIC** — Il secondo esempio della specifica (*«indice spostato da un KO»*) va esteso all'intervallo nuovo:
l'unità nominata cade **dopo** la costruzione e **prima** della consegna, e il titolo non cambia.

**CRISPIN** — I tre test headless restano quelli. Il secondo copre ora due finestre di tempo.

### 4.3 Integrazioni alla specifica

- **I4.1** «In quel momento» è il momento della costruzione della vista in `MakeReactionWindowView`.
- **I4.2** `ReactionName` è il `DisplayName` della voce di kit della reazione. Se manca, il titolo resta «Reazione».
- **I4.3** Un quarto esempio: l'unità nominata cade fra costruzione e consegna; titolo e sottotitolo restano
  quelli costruiti.
- **I4.4** Revisione del confine di privacy, confermata: nessun campo nuovo nella vista dell'avversario;
  `FilterWindowForTeam` resta l'unico filtro.

La Definition of Done resta quella del referto dell'11 ottobre, con I4.3 aggiunto ai test headless.

---

## 5. #3626 — `color_metrics.py` non gira

### 5.1 Il fatto, rimisurato

**Confermato** (eseguito):

```sh
python tools/hud-assets/color_metrics.py    # exit 1
# RuntimeError: `const FLinearColor Color = bOwn ? ...` non trovato in RTHUD.cpp
```

La riga in `UI/RTHUD.cpp` oggi è `Out.Color = bOwn ? … : …;`. Il secondo ancoraggio, `const FLinearColor DashColor`,
è intatto (letto). La forma è cambiata con `26370f3c7`, del 2026-09-04.

### 5.2 Il panel

**NYGARD** — Lo script ha fatto ciò per cui era scritto: ha fallito forte. Fallisce dal 2026-09-04, e non
l'ha sentito nessuno. `color_metrics.py` è citato nei documenti e lanciato da nessun comando: non dal
generatore, non dal radar (`grep -rl color_metrics tools/ docs/` lo mostra). Un fallimento forte in uno script che nessuno esegue è un fallimento silenzioso.
Correggere l'espressione regolare ripara il sintomo; la prossima riscrittura di `RTHUD.cpp` lo romperà allo stesso
modo.

**FOWLER** — `overlay_colors()` legge il file e lo interpreta nella stessa funzione. Il secondo criterio della
issue chiede una prova *«sul C++ vecchio e su uno inventato»*: se l'interpretazione prende il testo come argomento,
la prova non deve toccare `RTHUD.cpp`.

**CRISPIN** — Il terzo criterio è il più importante: se la rimisura trova un criterio violato, si dichiara nella
PR. Dal 2026-09-04 quel controllo non ha misurato niente, e nel frattempo i colori dell'overlay possono essere
cambiati.

### 5.3 La specifica

- **R5.1** `overlay_colors()` riconosce la forma attuale e continua a fallire forte su una forma sconosciuta.
- **R5.2** L'interpretazione è una funzione sul testo. La prova la esercita su tre testi: la forma vecchia, la forma
  attuale, una forma inventata che deve dare errore.
- **R5.3** Il referto ha **un chiamante dichiarato**: un comando che qualcuno lancia già lo esegue, e la PR dice
  quale e perché.
- **R5.4** I criteri sull'overlay si rimisurano, e ogni violazione va nella PR.

### 5.4 Definition of Done

- [ ] `python tools/hud-assets/color_metrics.py` esce con 0 e stampa la sezione «World overlay».
- [ ] La prova di R5.2 è eseguibile, e cade se la forma inventata viene accettata.
- [ ] R5.3: il chiamante è nominato nella PR, e una riga di documentazione dice come si lancia.
- [ ] R5.4: l'esito della rimisura è scritto, anche se è «nessuna violazione».

---

## 6. #3627 — La copia Python della palette

### 6.1 Il fatto, rimisurato

Uno script di questo passaggio ha confrontato le due tabelle del generatore con i documenti che il generatore
stesso dichiara come owner (*«Chrome: `progettazione-hud.md` §32. Semantica: `02-color-system.md` §2»*). Esito
(eseguito):

- **`CHROME` coincide con §32** su tutti i suoi token: `BG_Deep`, `BG_Panel`, `BG_Raised`, `Frame_Deep`,
  `Frame_Mid`, `Cyan`, `Violet`, `Amber`, `Red`, `White`.
- **`SEMANTIC` coincide con
  [`02-color-system.md`](../../research/design/icon/visual-language/02-color-system.md) §2** su tutte le sue voci:
  `Movement`, `Attack`, `Utility`, `Hazard`, `Reaction`, `Electric`, `Defense`, `Disabled`.

**La divergenza che la issue dichiara non c'è.** `SEMANTIC["Disabled"]` vale `#6B7280` perché è `RT_Sem_Disabled`,
il grigio semantico delle icone (*«non disponibile, esaurito»*). `#6B7684` è `RT_UI_Text_Disabled`, il token di
[`progettazione-hud.md`](../../technical/systems/progettazione-hud.md) §32 (*«testo e icone spenti»*), entrato con
`D-482` e con sede a runtime in `URTUIPalette` (`D-489`). Sono due token di due vocabolari, e
`02-color-system.md` dichiara il confine: la palette del chrome è di §32 *«e non si riscrive qui»*.

Resta vero il resto: nessun controllo confronta le copie, quindi la coincidenza di oggi non è presidiata.

E resta un punto che la issue non nomina. Il gate `T9` del generatore misura la distinguibilità dei colori di fase
contro gli stati dell'HUD, e per lo stato spento usa `SEMANTIC["Disabled"]`. Da `D-482` l'HUD disegna il testo
spento con `RT_UI_Text_Disabled`. `T9` misura un grigio che l'HUD non usa più per quello stato.

### 6.2 Il panel

**FOWLER** — È un conflitto apparente fra tassonomie omonime. Il rimedio non è scegliere quale valore *«vince»*: è
dichiarare che sono due, e far leggere a ogni consumatore il token giusto.

**WIEGERS** — Il secondo criterio (*«per `Disabled` c'è una scelta documentata»*) presuppone una divergenza fra
copie. Va riscritto: la scelta da documentare è se i due token restano distinti, e quale dei due misura `T9`.

**NYGARD** — I due grigi differiscono di poche unità su due canali. È probabile che il verdetto di `T9` non cambi,
ma non è stato misurato: chi sposta `T9` sul token di §32 lo esegue con entrambi i valori e scrive i due esiti.

**CRISPIN** — Il controllo confronta **per nome**, con la corrispondenza dichiarata: `CHROME` contro i token
`RT_UI_*` di §32, `SEMANTIC` contro i token `RT_Sem_*` del suo documento. Una voce senza corrispondente deve dare
errore, non essere saltata.

### 6.3 La specifica

- **R6.1** Un controllo eseguibile confronta `CHROME` con §32 e `SEMANTIC` con `02-color-system.md` §2, per nome.
  Esce diverso da zero su un valore divergente o su una voce senza corrispondente.
- **R6.2** Il generatore dichiara accanto a `SEMANTIC["Disabled"]` che è `RT_Sem_Disabled` e **non**
  `RT_UI_Text_Disabled`.
- **R6.3** *(dipende da `P4`)* `T9` legge il grigio dello stato spento dal token che l'HUD disegna.
- **R6.4** Il controllo ha un chiamante dichiarato, come R5.3: conviene che sia lo stesso.

### 6.4 Definition of Done

- [ ] R6.1 è eseguibile, e cade cambiando un valore di una delle due tabelle.
- [ ] `P4` è scritta nella issue, e il secondo criterio del corpo è riformulato di conseguenza.
- [ ] Se `T9` cambia fonte, i due esiti (prima e dopo) sono nella PR.

Fuori perimetro, da non risolvere qui: il documento che il generatore dichiara owner della palette semantica si
presenta come *«sorgente di design, non canone»*. Chi sia l'owner normativo dei token `RT_Sem_*` è una domanda a sé.

---

## 7. Decisioni richieste

`BLOCKED — DECISION REQUIRED` solo per le parti indicate. Nessuna si deduce dai documenti.

| ID | Issue | Domanda | Opzioni e conseguenze | Raccomandazione del panel |
|---|---|---|---|---|
| **P1** | #3081 | Come il feed nomina una caduta? | **(a)** resta «Colpita — N», e la issue dichiara che nel feed una caduta non si distingue da un colpo. **(b)** la riga di danno nomina la causa: «Caduta — N» per chi cade, «Urto — N» per chi è colpito dall'atterraggio. Tocca solo `ComposePlayerEventText`. | **(b)**. È l'unica forma in cui la issue cambia ciò che il giocatore legge |
| **P2** | #3400 | Uno scatto vale un ordine per gesto accettato, cioè due? | **(a)** sì: arm e destinazione, come un'abilità con bersaglio. **(b)** un peso diverso, da definire: cambia il significato di `OrderCount` per tutti i gesti, e il criterio di `PIE-PACING-1`. | **(a)**. È la lettura del contratto di `RTPacing.h` |
| **P3** | #3629 | La tabella dei nomi del core si fa adesso? | **(a)** forma ridotta: il test sul kit assemblato, tabella rinviata al primo lettore senza kit. **(b)** forma piena: tabella, documento, radar e gate, con i nomi da approvare. | **(a)**. Oggi la tabella non avrebbe un lettore |
| **P4** | #3627 | `RT_Sem_Disabled` e `RT_UI_Text_Disabled` restano due token? | **(a)** restano due, e `T9` legge quello che l'HUD disegna. **(b)** si unificano su `#6B7684`: cambiano le icone che oggi usano il grigio semantico, e va rimisurato `T9`. | **(a)**. Sono due vocabolari con owner diversi |

---

## 8. L'ordine e i vincoli

```text
subito, senza motore e senza decisioni   #3626
subito, col motore libero                #3081 (R1.1–R1.3, R1.5, R1.6)  ->  #3667
dopo la risposta                         #3081 R1.4 (P1) · #3400 (P2) · #3629 (P3) · #3627 (P4)
```

- **Il freeze.** #3081, #3400 e #3667 toccano `Source/`. Per `D-483` entrano **prima** del candidate v2
  ([`sequenza-chiusura-v01-spec-panel-2026-10-09.md`](sequenza-chiusura-v01-spec-panel-2026-10-09.md) §4, Onda 2).
  #3626 e #3627 toccano solo `tools/` e possono entrare anche dopo.
- **Il motore è uno.** Al momento di questo referto un'altra sessione compilava nel clone principale. La serie si
  lavora in un worktree separato; le suite si lanciano con `-abslog` nello scratchpad della sessione e si
  dichiarano `NOT RUN` se il motore è occupato.
- **File condivisi.** La stessa sessione tocca in chiusura `CHANGELOG_DOCUMENTATION.md`, `capability-map.yaml`,
  `test-manuali-pie.md` ed `editor-sessions.yaml`. Chi mergia per secondo integra, non sovrascrive.
- **Sedute.** `PIE-PACING-1` (#3400) e la cattura sul banco `Visual.Reaction.OverwatchWindow` (#3667) restano
  verdetti a schermo: le due issue restano aperte dopo il merge del codice.

---

## 9. Limiti dichiarati

Niente di ciò che segue è stato eseguito col motore. Ogni voce dice che cosa la smentirebbe.

| Affermazione | Come è stata ottenuta | Che cosa la falsifica |
|---|---|---|
| chi cade ha nel feed una sola riga, ed è quella del danno | lettura di `ApplyFallEffects`, `DominanceRank` e della sostituzione per `UnitId` | l'esempio 1 di §1.4, scritto come test **prima** dei `case`: se dà due righe o un'altra superstite, §1 va riscritta |
| un turno di solo scatto conta oggi un ordine | lettura dei siti che registrano `Order` | il test di §2.4 eseguito senza la correzione: il primo e il terzo esempio devono dare 1 |
| il terzo passo di `NomeDi` non è raggiungibile nel catalogo spedito | lettura di `MakePlanFor`, e fiducia nei gate sui nomi, non rieseguiti | il test di R3.1: se trova un'abilità di kit senza nome, il caso esiste e `P3` cambia |
| spostare `T9` sul token di §32 non ne cambia il verdetto | stima sulla distanza fra i due grigi | l'esecuzione di `T9` con entrambi i valori |

Il panel è stato condotto in linea da una sola sessione, senza agenti separati: le voci degli esperti sono lenti
distinte applicate agli stessi fatti, non revisioni indipendenti. Una verifica avversaria non è stata fatta.

---

## 10. Gate di questo passaggio

I due comandi del criterio di `G14`, eseguiti sul ramo di questo referto: l'esito è nel corpo della PR che lo
porta, col commit su cui è stato misurato.

```sh
node tools/radar/doc-links.ts --check
node tools/radar/doc-coherence.ts --check
```
