# Match Lab — referto di discovery

> `DISCOVERY` · **Data**: 2026-09-09 · **Stato**: nessuna issue aperta, nessun codice scritto
> **Nato da**: `CLAUDE_CODE_CREATE_SCENARIO_LAB_ISSUES.md`, un prompt di generazione backlog sottoposto a
> spec panel e **non eseguito** — vedi §2.
> ⚠️ **Questo file non dichiara stati di avanzamento.** Se una riga qui dice «fatto» o «manca», è una
> misura del 2026-09-09 con accanto il comando che la produce. Rimisurala prima di fidartene.
>
> 🔴 **RETTIFICA del 2026-09-09, dopo la prima stesura.** Le §1–§7 sono state scritte **prima** di leggere
> [`../roadmap-balance.md`](../roadmap-balance.md) e le issue `CR-BALANCE`. Quella lettura cambia la
> conclusione: **Match Lab non va aperto come capability** — vedi §8, che governa. Ciò che resta valido
> delle sezioni precedenti è l'inventario tecnico (§3, §4, §5), non la proposta di §6.

---

## 1. Che cos'è

Il terzo membro della famiglia `Lab`, che nel repository ha già una semantica precisa —
`SRTLabPanel.h`: *«un'ability canonica si sceglie, si esegue, e si legge cosa è successo»*.

| Lab | Misura | Owner |
|---|---|---|
| Ability Lab | un'ability su bersaglio nudo | `#2599` |
| Hero Lab | un kit | `#2600` |
| **Match Lab** | **un ingaggio sulla mappa, un turno alla volta** | — questo file |

`#2568` (BAL 0.3) pone il problema che Match Lab esiste per risolvere:

> «la variante si misura **nella mappa**, non su un bersaglio nudo»

### Il loop

```text
carica una situazione
        │
        ▼
dichiara chi guida cosa ──── team A a me · team B al bot · o entrambi a me
        │
        ▼
do gli ordini alle unità che guido io
        │
        ▼
il motore risolve            ← LockInAndResolve, la porta di sempre
        │
        ▼
leggo cosa è successo        ← TurnLog
        │
        └──── il risultato È la nuova situazione ──► ricomincio
```

**Il turno singolo è il modo di procedere, non l'oggetto della misura.** L'oggetto è l'arco: come
evolve un ingaggio quando cambio un numero, un eroe, un terreno o una composizione.

---

## 2. Perché non è «Scenario Lab»

Il prompt di partenza chiedeva di creare dieci milestone, un'epic ombrello, dieci release epic e le
relative issue figlie su `refactor-tactics-main`, sotto il nome *Scenario Lab*. Lo spec panel
(Wiegers, Adzic, Cockburn, Fowler, Nygard, Crispin, Hightower) ha raccomandato di **non eseguirlo**.
Le tre ragioni che restano valide anche ora che lo scopo è cambiato:

1. **La baseline non è ferma.** Il prompt definisce la parità contro «la baseline v0.1» e chiede di
   congelarla. Misurato: `gh api repos/DegrassiAaron/refactor-tactics-main/milestones?state=all` dà
   `v0.1 — Offline Vertical Slice` ancora aperta. Non esiste una v0.1 da congelare.
2. **Il controllo duplicati proposto non trova il duplicato.** Cercava `Scenario Lab`, `map editor`,
   `parity matrix`. Nessuno di questi termini trova `#1105`, il cui titolo è *«Tactical Designer — un
   solo loop fra mappa, skill e scenario»*, che copre `v0.1`–`v0.4` e `v0.6` del prompt.
3. **Sarebbe il terzo nome per la stessa superficie**, dopo *Tactical Designer* e *Scenario Composer*.
   `spec-tactical-designer.md` §2 mette in guardia esattamente su questo.

**Match Lab non è il rebranding di quel prompt.** È un oggetto diverso: il prompt descriveva
l'*authoring* di scenari multi-turno; Match Lab è l'*esecuzione osservata* di un ingaggio.

---

## 3. Cosa esiste già

Tre dei quattro pezzi sono in repository. Misurato il 2026-09-09.

### 3.1 La pausa a turno indefinita

```text
Turn/RTTurnManager.h:388   void PausePrepWindow();
Turn/RTTurnManager.h:392   void ResumePrepWindow();      // riprende DAL RESIDUO
Turn/RTTurnManager.h:396   bool IsPrepWindowPaused()
Player/RTPlayerController.h:240   PrepWindowPauseAction  // già un input bindabile
```

È il «avanza quando dico io». Con un vincolo dichiarato nel sorgente: la finestra si arma **solo in
sessione non presidiata** (`bUnattendedSession`) — *«in una partita con una mano umana il ritmo è
quello del giocatore, e non si tocca»*. Match Lab è una sessione con una mano umana **e** un ritmo da
controllare: è la prima configurazione che chiede entrambe le cose, e va deciso come.

### 3.2 La camera di misura

```text
Camera/…:250  /** Vista di MISURA a picco sull'intera board (`rt.Camera.TopDown`). */
              bool ApplyTopDownView(const ARTHexMapActor*, const FVector&, float, float);
              void ScheduleTopDownShot();   // scatto di confronto
```

Il commento la chiama già «vista di misura». Non va costruita: va accesa.

### 3.3 Il bot pianifica per singola unità

```text
Bot/RTHexBotLibrary.h:518   FRTHexBotPlan PlanUnit(const FRTHexSnapshot&, int32 UnitId, const FRTHexBotContext&);
Bot/RTHexBotLibrary.h:514   TArray<FRTHexBotPlan> BuildCandidates(...);
Bot/RTHexBotLibrary.h:507   FRTHexBotPlan ChooseBestPlan(...);
Bot/RTHexBotLibrary.h:437   int32 ScorePlan(...);        // e dice anche PERCHÉ ha scelto
```

La granularità è già l'unità, non la partita. `ScorePlan` è ciò che rende leggibile una scelta del
bot, e quindi misurabile.

### 3.4 Le situazioni di partenza

`Scenarios/AutoBattle/` contiene `ArenaV01`, `Hazard`, `Objective`, `Obstacles`, `OpenField` — già
caricabili dal Loader, già usate dai test. **Il primo taglio non authora nulla di nuovo**: parte da
queste.

---

## 4. Cosa manca

### A. La dichiarazione «chi guida cosa»

Oggi il controllo è la composizione di tre flag globali, risolta in **una riga**:

```cpp
// Match/RTMatchBootstrapper.cpp:322
Unit->bIsBotControlled = (TeamId == 1) || Config.bAutobattle || bBotAlly;
```

- `TeamId == 1` è cablato: la squadra avversaria è sempre del bot;
- `bAutobattle` toglie il giocatore da tutto;
- `bBotAlly` (`BotAllyCount`) è dichiarato in `RTGameMode.h` *«il fratello minore di `bAutobattle`,
  non un suo caso particolare»*: riduce le unità comandabili contandole **dal fondo** di `Team0Heroes`.

🔑 **La granularità che serve esiste già nel dato.** `bIsBotControlled` è un campo per-unità su
`ARTUnit`, e la regola che lo legge è pura e statica:

```cpp
// Combat/RTCombatLibrary.h:344
static bool CanPlayerControlUnit(int32 UnitTeamId, int32 PlayerTeamId, bool bUnitIsBotControlled = false);
```

Manca solo il modo di **dichiarare** quel campo dall'esterno invece di derivarlo da tre flag globali.
È configurazione, non motore — e il repository lo dice già di sé, in `RTGameMode.h`:

> ⛔ *«il delta è configurazione e non motore [...] Cambia **chi** è segnato come bot, non **come**
> decide (invariante #10).»*

Questa è la diagnosi che `#2745` ha già scritto: *«i seam esistono sparsi, nessun manifesto»*.

### B. Il confronto fra run

Il TurnLog dice cosa è successo in **una** esecuzione. Non esiste il diff fra due esecuzioni, che è
ciò che trasforma una misura in un bilanciamento. È il buco che `#2566` (BAL 0.1) nomina: *«cambio un
numero e so quali scenari cambiano esito»*.

Sonde a vuoto, per delimitare cosa non c'è (2026-09-09):

```sh
grep -ril ParentHash Source/ docs/     # 0 file
grep -ril Outdated   Source/           # 0 file
grep -rn "ConfirmResult\|AsNextTurn\|PromoteResult" Source/   # nessun hit
```

⚠️ **Nota sul modello.** Il prompt di partenza proponeva una catena di turni con `parent hash` e
invalidazione dei discendenti. Match Lab **non ne ha bisogno**: il turno successivo non è authorato in
anticipo, è generato dall'esecuzione. Non c'è un antenato da invalidare, perché non c'è un discendente
già scritto. Il modello a catena resta un'idea valida per l'authoring — non è il problema di Match Lab.

---

## 5. Strategia build e test

**Vincolo dichiarato dal committente**: concentrare build, test e tempo occupato dall'engine.

Il progetto cade bene rispetto a questo vincolo, e la ragione è nel sorgente. `RTTurnManager.h`
documenta perché la prep window sta su `OnPlanningTimeout` e non dentro `LockInAndResolve`:

> 🔑 *«`LockInAndResolve` è il punto di CONFLUENZA di tre percorsi, e lo Scenario Harness lo chiama
> **diretto** (`RTScenarioRunner.cpp`, `RTScenarioSession.cpp`). Una finestra lì dentro metterebbe tre
> secondi di attesa in ogni run headless. Qui l'harness non passa: non la vede **per costruzione**, non
> per disciplina.»*

**Conseguenza**: il loop di Match Lab è provabile headless senza attese, perché l'harness entra dalla
stessa porta della partita saltando la finestra. Con `PrepWindowSeconds <= 0` risolve subito.

### La piramide, applicata a questo progetto

| Livello | Cosa ci sta | Engine? |
|---|---|---|
| **pure/domain** | il modello «chi guida cosa» e la sua risoluzione; il diff fra due TurnLog | **no** |
| **headless** | il loop turno-per-turno via `RTScenarioRunner` / `RTScenarioSession`; le run di confronto | **no** |
| **engine integration** | il bootstrap che scrive `bIsBotControlled` dalla dichiarazione | sì, in suite |
| **Editor / PIE** | vista top-down, input di pausa, giudizio visivo | sì, in gate |
| **packaged** | — nessuna evidenza che questo progetto possa dare solo lì | — |

**Le due cose che mancano (§4.A e §4.B) stanno entrambe nei due livelli senza engine.** Il grosso di
Match Lab si costruisce e si prova senza mai avviare l'Editor; l'engine serve per la superficie di
osservazione, non per la logica.

### Regole operative

- la regola «chi guida questa unità» ha **un solo produttore** — `CanPlayerControlUnit`. Non
  ricomporla: `RTPlayerController.h:905` avverte già che un secondo produttore *«divergerebbe il
  giorno del bot alleato»*;
- una verifica che il livello pure dimostra non sale di livello;
- le verifiche engine si aggregano in una suite, non un avvio per caso;
- prima di ottimizzare build e bootstrap, **misurarli**: nessun numero di baseline è stato preso oggi.

---

## 6. Primo taglio proposto

Deciso il 2026-09-09: situazione iniziale dai file già in `Scenarios/AutoBattle/`, **zero authoring
nuovo**. Il taglio si concentra sul loop e sulla manopola.

1. **La dichiarazione.** Una configurazione che dice, per unità o per squadra, chi la guida —
   sostituendo la derivazione da tre flag globali senza rimuoverli. Verifica: pure.
2. **Il loop.** Carica una delle situazioni, applica la dichiarazione, risolvi un turno, esponi lo
   stato risultante come nuovo punto di partenza. Verifica: headless.
3. **La superficie.** Vista top-down accesa, pausa della finestra comandabile, il TurnLog leggibile
   dopo ogni turno. Verifica: Editor/PIE, in un solo gate.

**Fuori dal primo taglio**: il confronto fra run (§4.B), il piazzamento di unità in-game, il
salvataggio di varianti, qualunque metrica aggregata.

---

## 7. Confini — cosa Match Lab non è

| Non è | Owner reale |
|---|---|
| un editor di scenari | Scenario Composer, `#1114`–`#1117` (chiuse) |
| il workflow d'authoring | Tactical Designer, `#1105` |
| bot-vs-bot da guardare | AutoBattle Viewer, `#2748` |
| la misura di un'ability isolata | Ability Lab `#2599`, Hero Lab `#2600` |
| una seconda autorità sull'esito | il resolver, e nessun altro |

L'invariante di `spec-tactical-designer.md` §3 vale identica qui: **uno strumento non è mai
un'autorità di gioco**. Match Lab non calcola danno, portata, percorsi o esiti: dichiara chi comanda,
chiede al motore, e legge il TurnLog.

---

## 8. Rettifica — cosa dice `CR-BALANCE`, e perché Match Lab non va aperto

La domanda aperta n. 3 della prima stesura era *«Match Lab è lo strumento che le issue BAL cercano, o è
un percorso parallelo?»*. È stata risolta leggendo, ed è la sezione che governa questo file.

### 8.1 Quello che esisteva già, e che la prima stesura non aveva letto

[`roadmap-balance.md`](../roadmap-balance.md) — capability `CR-BALANCE`, epic ancora `#2565` — è stata
**aperta il 2026-09-06**, tre giorni prima di questo referto. Contiene:

- la stessa vision, in una riga: *«Il designer cambia un parametro d'abilità e sa, con evidenza
  riproducibile, quali scenari cambiano esito e di quanto»*;
- la stessa invariante di §2.1, *«Nessun secondo simulatore»*, con lo stesso diagramma;
- gli stadi `BAL 0.1`…`BAL 1.0`, issue `#2566`…`#2575`, più i gap `#2576`…`#2579`;
- un percorso critico dichiarato e una prima issue da implementare.

**Le quattro misure richieste dal committente hanno già uno stadio**, e tre non hanno owner:

| Misura chiesta | Stadio | Owner del lavoro |
|---|---|---|
| un'abilità nel contesto | `BAL 0.3` — `#2568` | ⚠️ nessuno |
| un eroe contro un altro | `BAL 0.2` — `#2567` | ⚠️ nessuno |
| mappa e terreno | `BAL 0.3` / outcome `O5` | ⚠️ nessuno per la misura |
| composizione di squadra | `BAL 0.7` — `#2572` | ⚠️ *«nessuno oggi per la parte strumento»* |

### 8.2 Il rischio che questo referto stava per realizzare

`roadmap-balance.md` §11 elenca come **primo** rischio aperto:

> **Backlog parallelo** — *«quarta ricomparsa della stessa proposta: milestone 2026-08-13 (superata),
> `TD 0.x` rinumerata ⛔, `SW-E1…SW-E9` ⛔»*

`CLAUDE_CODE_CREATE_SCENARIO_LAB_ISSUES.md` sarebbe stata la quinta. **Aprire `Match Lab` come
capability, con una propria scala e propri stadi, sarebbe la sesta** — e la prima commessa da chi aveva
appena scritto un referto per evitarla.

⛔ **Match Lab non va aperto come capability, scala o milestone.** Il nome resta utile come etichetta
descrittiva di un modo d'uso; non deve diventare un contenitore di lavoro.

### 8.3 Il percorso già tracciato, che risponde alla stessa domanda

`roadmap-balance.md` §8 dichiara il percorso critico e la prima issue:

```text
#2577 pannello  →  #2576 diff  →  BAL 0.1 verificato
```

> **Prima issue da implementare: `#2576` — `TD 0.4`, il diff baseline↔variante.**
> *«Le sue due dipendenze (`TD 0.2` e `TD 0.3`) sono chiuse, **non tocca l'Editor**, e senza di essa il
> pannello avrebbe un ingresso e nessuna uscita.»*

🔑 **«Non tocca l'Editor» soddisfa il vincolo di §5 meglio di qualunque cosa proposta in §6.** Il lavoro
che serve per iniziare a bilanciare è già identificato, già senza duplicati, e già nel livello più
economico della piramide.

⚠️ **Ma il gate `catalog-code.ts` è rosso** (`cpp: 21/28`, misurato il 2026-09-06 su `origin/main` =
`7c1af4c4`): `D-334` ha rinominato `Hero.Riktor` → `Hero.Branth` nel codice e il catalogo dichiara
ancora *Riktor*. Owner `#2491`, riparazione in `PR #2582`. `#2578` **presuppone** quel gate verde. Da
rimisurare prima di sequenziare qualsiasi cosa:

```sh
node tools/radar/catalog-code.ts
```

### 8.4 Cosa resta davvero non posseduto

Un solo pezzo di questo referto non ha un owner in nessuna delle viste esistenti: **la dichiarazione
"chi guida quale unità"** di §4.A — la sostituzione di
`(TeamId == 1) || Config.bAutobattle || bBotAlly` con una configurazione esplicita.

Non è di `CR-BALANCE` (che esegue in modo automatico, non interattivo), non è di `#1105` (authoring),
non è di `#2748` (bot-vs-bot). Ha però già una diagnosi scritta: **`#2745`** — *«i seam esistono
sparsi, nessun manifesto»*.

**Se un giorno servirà, il posto è `#2745`, non una capability nuova.**

### 8.5 Domande che restano

1. **La prep window in sessione presidiata.** Si arma solo con `bUnattendedSession`; il sorgente motiva
   la scelta. Resta un nodo di design, indipendente da chi lo affronterà.
2. **Nessuna misura di baseline** di build e bootstrap è stata presa. Il vincolo di §5 non ha ancora un
   numero contro cui verificarsi.
3. **Serve davvero un modo interattivo?** `CR-BALANCE` misura in modo automatico e riproducibile. Il
   valore di un giro giocato a mano sarebbe la **scoperta** — trovare la situazione che non si sapeva
   di dover cercare — non la misura. È una domanda per il committente, non per un referto.

---

## 9. Cosa è stato fatto in questa sessione

- clonato `DegrassiAaron/refactor-tactics-main` in `refactor-tactics-refactor`, branch `main`;
- audit read-only di repository, issue, milestone e label;
- spec panel su `CLAUDE_CODE_CREATE_SCENARIO_LAB_ISSUES.md`, con raccomandazione di non eseguirlo;
- discovery del perimetro reale, che ha prodotto questo file.

**Non è stato fatto**: nessuna issue, milestone o epic creata; nessun commit, branch o PR; nessun file
del repository modificato oltre alla creazione di questo referto.


---

## 10. COSA NE È USCITO — quattro issue invece di ~81 artefatti

> Aggiunto il 2026-09-09, a valle delle due chiusure.

La §8 concludeva che **Match Lab non andasse aperto come capability**. Quella conclusione è stata
rispettata: non esiste una milestone `Match Lab`, non esiste una scala `ML 0.x`, e il nome non è entrato
in nessun documento owner. È rimasto quello che la §8.2 diceva dovesse restare — un'etichetta
descrittiva.

Al suo posto, quattro issue mirate, tutte sub-issue di
[#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105):

| | Esito |
|---|---|
| [#2786](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2786) — il pannello non authora | ✅ **chiusa** · PR #2809 |
| [#2789](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2789) — il Composer è irraggiungibile e vuoto | ✅ **chiusa** · PR #2812 |
| [#2788](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2788) — `Esegui` muto sui `freerun` | aperta |
| [#2802](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2802) — il ponte viewport → pannello | aperta |

### La §8.4 aveva individuato il pezzo giusto

Diceva che l'unico elemento non posseduto da nessuna vista era **la dichiarazione «chi guida quale
unità»**. Quella resta vera e resta di `#2745`. Ma la discovery ha trovato qualcosa di più immediato: il
pannello **non authorava affatto** — `grep -cE "AddUnit|MoveUnit|RemoveUnit|SetUnit"` sul file dava `0`,
mentre i mutatori erano in `main` da `#1115`. Il dato c'era, mancava l'ingresso. Ora c'è.

### Cosa di questo referto è invecchiato

- **§3.4** dichiarava `Scenarios/AutoBattle/` come punto di partenza «zero authoring nuovo». Regge, ma il
  primo taglio è finito altrove: il piazzamento è entrato nel pannello, non in una modalità di gioco.
- **§8.5 domanda 3** — *«serve davvero un modo interattivo?»* — **non ha ancora risposta**. `CR-BALANCE`
  misura in batch; il valore di un giro giocato a mano sarebbe la scoperta, non la misura. Resta una
  domanda per il committente.

### Un effetto collaterale emerso e non risolto

`SaveInPlace` riscrive il file scenario in **forma canonica**: `74+/24−` su `Movement.Basic` per una sola
unità aggiunta. È del writer (`#1114`), ma con `DEC-1` di `#2786` ogni gesto salva — quindi il primo
piazzamento su uno scenario del corpus lo riformatta. Segnalato in #2786, senza owner.
