# Multi-Hero, Decision Time Bank e Preferred Response — spec panel

> `CURRENT` · **Stato**: revisione chiusa, consolidamento eseguito · **Data**: 2026-08-17
> **HEAD della revisione**: `94575ef4` · branch `docs/consolidamento-multihero-timebank` · worktree `D:/rt-reaction`
> **Sorgente revisionata**: il kit `RefactorTactics_Claude_MultiHero_TimeBank_PreferredReaction_2026-08-17.md`,
> archiviato in [`../../archive/src/`](../../archive/src/RefactorTactics_Claude_MultiHero_TimeBank_PreferredReaction_2026-08-17.md)
> alla fine di questo giro — venti sezioni, tre decisioni proposte, quindici criteri di accettazione.
> **Scopo**: misurare ogni premessa del kit su `main` **prima** di scriverla dentro un owner. Dove il codice
> smentisce il kit, prevale il codice — è la regola che il kit stesso pone al §20.

---

## 1. Il verdetto in una riga

Il kit ha ragione su quasi tutto ciò che **vieta** e sbaglia il verso della cosa più importante che
**chiede**: presenta il modello «un Player può controllare più Hero» come un guardrail architetturale per un
futuro lontano, mentre la misura dice che è un **prerequisito già scaduto** — `#319` dichiara un bank *per
giocatore* e nel percorso di decisione di `main` non esiste nessun giocatore a cui attaccarlo.

Il resto si divide in tre: sette affermazioni verificate esatte, cinque nomi che duplicano vocabolario già
esistente, e due numeri che non sono baseline ma incognite moltiplicate fra loro.

---

## 2. Cosa il kit misura giusto

Verificato riga per riga, senza sconti. Queste sono le premesse su cui il consolidamento si appoggia.

| Affermazione del kit | Esito | Misura |
|---|---|---|
| «`#319` copre bank per Player, `Grace`, `ExhaustedGrace`, `FastReactionDuration`, timeout, fallback preselezionato, replay/TurnLog, privacy, bot, test» | ✅ **esatta** | il body di `#319` contiene tutte e dieci le voci |
| «L'owner è `docs/gameplay/spec-decision-time-bank.md`» | ✅ | dichiarato in `#319` e nel Feature Registry (`owner_specs`) |
| «La specifica corrente deriva `InitialBank` soprattutto da `RoundLimit`» | ✅ | §3.2 della spec: `RoundLimit × (MaxWindow − Grace)` |
| «`FastReactionDuration = 3,0 s`» | ✅ | `spec-durata-partita-e-scala-mappe.md` §8 · ADR-0004 §8 |
| «Il principio *reaction immediatamente confermabile* esiste già» | ✅ | §4.2 della spec: fallback preselezionato, un solo input, mouse e controller, dentro la grace |
| «`#314` copre già il modello dove una Reaction può avere più risposte legali» | ✅ | `spec-reaction-clash-e14.md` §2.4 · `FRTReactionOpportunity::AllowedResponses` |
| «Non creare un secondo Time Bank / un secondo sistema di Reaction» | ✅ | corretto, ed è il vincolo che ha guidato dove sono finite le sezioni nuove |

E una che il kit dà per aperta mentre è **chiusa**: «il fallback è raggiungibile con un solo input» non è una
proposta da introdurre, è un **requisito vincolante** già scritto (§4.2, *«come requisito e non come
raccomandazione»*). La Preferred Response non lo crea: lo **specializza**.

---

## 3. I difetti misurati

### F1 · Il bank di `#319` non ha un soggetto — e il kit lo classifica come lavoro futuro

`D-050` dice *«un bank per **giocatore**, non per abilità e non per finestra»*. Nel percorso che apre e chiude
una finestra non c'è nessun giocatore:

```
ARTTurnManager::AskReactionDecision(const FRTReactionOpportunity&, int32 OwnerUnitId, bool bOwnerIsBot)
```

`OwnerUnitId` è un **indice di unità**; `bOwnerIsBot` è letto da `ARTUnit::bIsBotControlled`, cioè una
proprietà dell'unità. L'unico raggruppamento sopra l'unità è `ARTUnit::TeamId`, e la Decision Window non lo
legge mai. Fuori dal resolver la situazione è la stessa: `grep` su `ControlledHero`, `ControlGroup` e
`ControlledUnits` in `Source/` e `docs/` restituisce **zero**.

Conseguenza, ed è il capovolgimento di questo referto: chi implementerà `#319` dovrà **comunque** inventare il
soggetto «giocatore», perché senza non c'è dove tenere un bank condiviso fra finestre. Il kit propone di
scrivere il modello di controllo *dopo*, come guardrail; la misura dice che va scritto *prima*, o `#319` lo
improvviserà nel modo più stretto possibile — un bank per unità — che è esattamente ciò che `D-050` vieta.

> È la forma già vista con la condizione dichiarata di `D-109`, registrata nelle `notes` di
> `RT-FEAT-REACTION-OPPORTUNITY`: la regola c'è, è corretta, e resta **inerte** finché nessuno possiede
> l'azione che la farebbe mordere — lì `Action.Overwatch` fuori dal catalogo, qui il giocatore.

### F2 · `SafeTimeoutResponse` è un secondo nome per una funzione che esiste già

Il kit usa `SafeTimeoutResponse` **7** volte come se fosse un tipo da introdurre. La cosa esiste, è pura, ed è
documentata:

```
URTReactionOpportunityLibrary::DecisionOnTimeout(const FRTReactionOpportunity&)
    → «La decisione allo scadere della finestra: sempre HOLD (ADR-0004 §3). Funzione PURA.»
```

Introdurre un nome nuovo per la stessa regola produce due vocaboli e, al primo refactor, due verità. Il kit lo
prevede al §6 (*«i nomi finali vanno allineati al vocabolario già presente»*) e poi non lo applica.

**Nel consolidamento**: si usa `DecisionOnTimeout`. L'**unico** nome nuovo ammesso è `PreferredResponse`,
perché quella cosa non esiste.

### F3 · Space è già occupato, e il kit chiedeva di verificarlo

Il kit propone la barra spaziatrice per il Quick Confirm e mette in lista *«verificare conflitti con Space»*.
La verifica ha una risposta:

```
Source/RefactorTactics/Player/RTPlayerController.cpp:256
    MappingContext->MapKey(LockInAction, EKeys::SpaceBar);
```

Space chiude il **planning** (`LockInAndResolve`, `:897`). Il conflitto non è fatale — planning e Decision
Boundary non sono mai aperti insieme — ma riusare il tasto diventa allora una **decisione sul contesto di
input**, non un default naturale, e chi la prende deve dire perché non confonde chi gioca. Il kit ha anche
ragione sul resto: `IA_LockIn` è costruito in C++ con `NewObject<UInputAction>`, non da asset, e `Config/DefaultInput.ini`
non nomina `Space` — quindi «evitare input hard-coded nel widget» è un requisito reale, non teorico.

### F4 · Il Decision Boundary serializza già, e non per pigrizia

Il kit (§11) chiede di non serializzare una finestra da 3 s per Hero. Il codice serializza, e la ragione è una
regola:

```
Source/RefactorTactics/Turn/RTTurnManager.cpp:5261
    for (const FRTOverwatchTrigger& Trigger : Triggers)
    {
        ...
        const FRTReactionDecision Decision = AskReactionDecision(...);
        ApplyReactionDecision(Units, State, Opportunity, Decision, ArmedIndex);
    }
```

Applicare **prima** di chiedere la successiva è ciò che rende corretto il caso «due watcher nello stesso
micro-step con `Charges = 1`»: il secondo non trova più la charge e viene saltato (`:5288`, commento
esplicito). Un *Player Decision Batch* che raccolga le risposte e applichi dopo cambia quella semantica.

Il kit lo sospetta e si ferma al posto giusto (*«non implementarlo automaticamente dentro #319»*). La misura
lo conferma: **resta fuori dal consolidamento**, e resta come domanda aperta con la sua misura allegata —
non come issue, perché non esiste ancora il caso che la produce (un solo umano in v0.1, e un Player controlla
un Hero).

### F5 · La categoria di log che il bank userà ha già un omonimo in coda

`spec-turnlog.md` §4.2, deciso il 2026-08-09 con `#361`, prescrive che `ERTLogCategory` guadagni `Decision`
**in coda**, e `RTTestScenario.h:92` lo ripete (*«il Decision Time Bank scrive `Decision/BankConsumed` e
`Decision/BankAfter`»*). Ciò che è atterrato con CP 14.5 è però:

```
Source/RefactorTactics/Turn/RTTurnLog.h:17
    enum class ERTLogCategory : uint8 { Move, Combat, Fallback, Reaction, Environment, Facing, Predictive, ReactionDecision };
```

`ReactionDecision`, non `Decision`, già serializzata in TurnLog v8 e letta da cinque call site. Aggiungerne una
seconda in coda darebbe **due categorie sulla stessa finestra**, che è il difetto che il commento di
`ERTReactionDecisionOutcome` argomenta di aver evitato. Il kit non lo vede perché guarda `#319` e non il
TurnLog.

Non è lavoro di questo consolidamento — nessuno ha ancora scritto una riga del bank — ma è una voce di DoD di
`#319` (*«TurnLog allineato a `spec-turnlog.md`, nomi confermati con l'owner»*) che oggi è **falsificabile**, e
va registrata prima che qualcuno implementi contro la spec.

### F6 · Due varianti di Grace su un valore che non è ancora tarato

Il kit propone `Grace = BaseGrace + 0,50 s × ExtraHeroes` come baseline e `+0,75 s` come variante da
playtest. Ma `GraceMs = 1,0 s` è **a sua volta** `PROPOSED`, e il suo criterio di promozione (§3.2 della spec)
è *«il p50 delle risposte in playtest cade sotto la grace»* — una misura che non esiste, e che `TB-8` lega alla
chiusura di CP 14.6.

Un coefficiente additivo su un valore non tarato non è una baseline: sono due incognite, e il playtest che
dovrebbe separarle ne misura una sola alla volta. Peggio: il kit stesso osserva che 1,75 s *«riducono molto la
pressione effettiva del Time Bank»* su una finestra da 3,0 s — cioè sa che la variante può annullare la
feature, e la propone lo stesso come alternativa paritaria.

**Nel consolidamento**: si registra la **forma** — la Grace scala col carico di controllo, per policy
data-driven — e il coefficiente resta una domanda aperta ancorata a `TB-8`, non due varianti concorrenti
scritte come se fossero già una scelta.

### F7 · Il fattore su `InitialBank` reintroduce il numero magico che `D-056` aveva tolto

`InitialBank = RoundLimit × (MaxWindow − Grace)` è **derivato** apposta: la spec lo dice
(*«toglie un numero magico a un progetto che ha appena finito di toglierne altri»*, §3.2). Scrivere

```
InitialBank = BaseInitialBank × (1 + 0,75 × ExtraHeroes)
```

lo moltiplica per una costante inventata, cioè rimette dentro esattamente ciò che la derivazione aveva
eliminato — e la mette **fuori** dalla formula, dove non scala più con niente.

La forma coerente con `D-056` tiene il carico **dentro** la derivazione:

```
InitialBank = RoundLimit × (MaxWindow − Grace) × LoadFactor(ControlledHeroes)
    LoadFactor(1) = 1                       per costruzione
    LoadFactor(n) ∈ [1, n]                  1,75 per n = 2 è UN punto dell'intervallo, da playtestare
```

Così il parametro libero è **uno solo** e ha un nome, il limite inferiore («il carico non riduce il budget») e
quello superiore («due Hero non costano più di due giocatori») sono argomentati invece che assunti, e
`RoundLimit` continua a far scalare il bank col formato da solo.

### F8 · `ControlledHeroCount` non è `UnitsPerTeam`, e in v0.1 i due numeri coincidono

Il kit dice bene: *«il conteggio usa le Hero realmente assegnate/controllate per quel match, non la dimensione
teorica del roster»*. Il dato che esiste oggi è però un altro:

```
Source/RefactorTactics/Turn/RTMatchFormatData.h:47
    int32 UnitsPerTeam = 0;   // unità per SQUADRA
```

In v0.1 — 2v2 offline, **un** umano che controlla una squadra — «unità per squadra» e «Hero per giocatore»
valgono entrambe `2`. Un'implementazione che legga il campo sbagliato passa ogni test esistente e sbaglia al
primo formato in cui una squadra è divisa fra due persone. È il caso in cui un campo errato è invisibile
finché non è tardi, e per questo il campo nuovo va **dichiarato accanto** a quello che gli somiglia, non
dedotto da lui.

### F9 · La Preferred Response ha già un precedente strutturale, e il kit non lo nomina

Il kit cerca dove mettere la preferenza e propone «policy/metadata sopra le `AllowedResponses`». Il posto
esiste, con un precedente esatto:

```
Source/RefactorTactics/Turn/RTTurnManager.h · struct FRTArmedOverwatch
    FRTDeclaredCondition Condition;   // «La condizione dichiarata in pianificazione (D-109). Vuota = nessuna.»
```

Una reaction armata porta già **una dichiarazione fatta in planning dal decisore**. La Preferred Response è la
seconda, e la differenza fra le due è precisamente l'invariante che il kit chiede:

| Dichiarazione | Quando agisce | Cosa fa alle `AllowedResponses` |
|---|---|---|
| `FRTDeclaredCondition` (`D-109`) | al trigger | le **riduce** — è un filtro di legalità |
| `PreferredResponse` (nuova) | all'apertura della finestra | **non le tocca** — ordina la presentazione |

Detto così, «la preferenza non cambia la cardinalità» smette di essere una raccomandazione da ricordare e
diventa la riga che distingue i due campi. E la Preferred Response **non** va su `FRTReactionOpportunity`, che
ha due soli campi (`Key`, `AllowedResponses`), è costruita dal server e viaggia verso il client: metterla lì
sarebbe informazione privata del decisore dentro il DTO che `Overwatch.OpportunityLeaksNoFuture` esiste per
tenere pulito.

---

## 4. Il panel

**WIEGERS** — *«Quali di questi criteri sono verificabili?»* Dei quindici criteri di accettazione del §19, **undici**
sono verificabili con un comando o un test e **quattro** no: «nessun documento CURRENT assume più
implicitamente `Player == Hero`» non ha un oracolo (implicito non si cerca con `grep`), e «i gate documentali
restano verdi» è vero ma non misura questo lavoro. Il primo va riscritto come una lista chiusa di documenti
misurati; il secondo va tenuto, perché costa poco ed è già la disciplina della casa.

**FOWLER** — *«Quanti di questi nomi nominano qualcosa che non ha già un nome?»* Il kit ne propone tredici
(quattro in §1, due in §6, uno in §9, sei in §16). **Cinque duplicano vocabolario esistente**, contati uno per
uno: `SafeTimeoutResponse` → `DecisionOnTimeout` (F2) · `MaxHeroesPerTeam` → `UnitsPerTeam`, che esiste in
`FRTMatchRules` dal CP 19.2 · `DecisionTimingPolicy` → è già il nome che la spec usa in §6.1 · `BaseGrace` e
`BaseExhaustedGrace` → `GraceMs` e `ExhaustedGraceMs` in §3.2. Altri due sono **sovra-specifica**:
`MinControlledHeroesPerPlayer` e `MaxControlledHeroesPerPlayer` sono due campi per un intervallo che in v0.1
ha cardinalità uno.

Restano `PreferredResponse` e il conteggio del controllo: **due** nomi davvero nuovi. La regola sta scritta nel
kit stesso — *«non introdurre questi campi letteralmente se il repository possiede già un modello
equivalente»* — ed è il kit a non eseguirla su sé stesso.

**COCKBURN** — *«Chi è l'attore, e qual è il suo obiettivo?»* Il kit passa da «Player» a «decisore» a «owner» senza
dire se sono la stessa cosa. Nel codice l'unico attore è l'**unità**. Finché il modello di controllo non
esiste, ogni frase del kit che dice «il Player» sta descrivendo un attore che il sistema non conosce — ed è
questo, non il 16v16, il motivo per cui il §1 va scritto adesso.

**NYGARD** — *«Cosa succede quando la preferenza è stale?»* Il kit risponde bene (`preselect SafeTimeoutResponse`,
il server rivalida al commit) e manca il caso peggiore: la preferenza **non è più fra le `AllowedResponses` ma
un'altra risposta le somiglia**. `FIRE:<UnitId>` porta il bersaglio dentro la stringa: `FIRE:7` illegale con
`FIRE:9` legale non è «illegale, degrada a HOLD» per tutti — è la domanda se la preferenza sia *sparare* o
*sparare a quello*. Va decisa, e la risposta sicura è la seconda: confronto **esatto** con `IsResponseAllowed`,
nessun matching parziale.

**ADZIC** — *«Mostrami l'esempio in cui il timeout non fa FIRE.»* Il kit lo scrive, ed è il suo pezzo migliore
(§7). Va reso eseguibile: `Reaction.TimeoutIgnoresPreferredResponse` con `PreferredResponse = FIRE:<n>`,
nessun input, e le tre asserzioni separate — risposta `HOLD`, charge **non** spesa, reaction ancora armata.
Una sola delle tre passerebbe anche se il codice sbagliasse le altre due.

**CRISPIN** — *«Quanti di questi test l'harness può davvero eseguire?»* Dei diciotto nomi del §13, quelli sul
bank sono esprimibili come `Spec.*` (l'harness ha `LogEventAmount` dal 2026-08-10); quelli sulla
preselezione **no**, perché la preselezione è uno stato di UI e il TurnLog registra la risposta committata,
non ciò che era evidenziato. Vanno separati: harness per la regola, PIE per ciò che si vede. Confonderli
produce test che non falliscono mai.

---

## 5. Cosa entra nel consolidamento, e cosa no

| Contenuto del kit | Esito | Dove atterra |
|---|---|---|
| Player non è 1:1 con Hero; conteggio dichiarato dal formato | ✅ **entra**, e **prima** di `#319` (F1) | spec del formato · Decision Log · issue nuova |
| Time Bank sensibile al carico di controllo | ✅ entra, riformulato dentro la derivazione (F7) | `spec-decision-time-bank.md` · `#319` |
| `FastReactionDuration` invariata col numero di Hero | ✅ entra come invariante esplicito | `spec-decision-time-bank.md` §1.2 |
| Coefficienti `+0,50` / `+0,75` / `×1,75` | ⚠️ entra la **forma**, non i due numeri concorrenti (F6) | domanda aperta ancorata a `TB-8` |
| `PreferredResponse` distinta dal timeout | ✅ entra, sul precedente `D-109` (F9) | `spec-decision-time-bank.md` §4.2 · `#319` |
| `SafeTimeoutResponse` come nome nuovo | ❌ **respinto** (F2) | si usa `DecisionOnTimeout` |
| Quick Confirm come azione semantica | ✅ entra, col conflitto Space dichiarato (F3) | `spec-decision-time-bank.md` §11 · verifica PIE |
| Player Decision Batch (più Hero, un boundary) | ⛔ **fuori**, con la misura che lo motiva (F4) | domanda aperta, nessuna issue |
| 16v16, networking, UI Grand Battle | ⛔ fuori, come il kit stesso chiede | — |
| Categoria di log `Decision` vs `ReactionDecision` | ➕ **aggiunto dal panel**, non era nel kit (F5) | nota in `spec-decision-time-bank.md` §10 · `#319` |

---

## 6. Le due cose che questo referto non ha potuto fare

- **La verifica PIE del Quick Confirm non è stata scritta in
  [`test-manuali-pie.md`](../../technical/test-manuali-pie.md).** Quel file non è nel `writable` di questa
  track: su `origin/main` (`94575ef4`) `docs/roadmap/parallel-batch.yaml` lo assegna a `playtest`, che è
  `IDLE` — quindi il path è *prenotato*, non *in uso*, ma prenotato da un'altra track — e `D-139` dice STOP.
  Le due voci richieste (preselezione visibile · decadimento dichiarato) sono descritte nel §11 della spec e
  nel DoD di `#319`, col testo pronto perché chi possiede il file le trascriva senza riprogettarle.

  > 🔴 **Questa riga ha dovuto correggersi prima del commit, e il difetto vale più del fatto.** La prima
  > stesura attribuiva il file a `playback` e argomentava che l'assegnazione fosse **scaduta**, perché
  > `#1015` è `CLOSED`. Era vero del file che avevo letto e falso del repository: leggevo
  > `parallel-batch.yaml` dal checkout principale, fermo su un `temp-detach` più vecchio di `origin/main`.
  > Rimisurato sull'albero della track, l'owner è `playtest` e la prenotazione è legittima — la conclusione
  > operativa non cambia (STOP in entrambi i casi), ma la ragione sì, e con essa chi va cercato. **Un
  > write-set si misura sulla base su cui si sta lavorando**, non su quella che si aveva aperta.
- **Nessun numero è stato promosso.** Tutti i coefficienti restano `PROPOSED FOR PLAYTEST`, e il criterio di
  promozione resta quello di §3.2, che dipende da una misura che CP 14.6 non ha ancora prodotto.
