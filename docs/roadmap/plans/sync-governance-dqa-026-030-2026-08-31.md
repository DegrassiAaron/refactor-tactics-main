# Sincronizzazione governance `DQA-026`…`DQA-030` — Drive ↔ Decision Log

> **Referto di sincronizzazione**, non owner. Consuma l'handoff ChatGPT conservato in
> [`../../archive/src/handoff/2026-08-31-chatgpt-governance-handoff.md`](../../archive/src/handoff/2026-08-31-chatgpt-governance-handoff.md)
> e le righe **live** del tab `Open Questions` del Google Sheet *RT — Knowledge Index & Consolidation Log*
> (`1GOd_Hi3bZBM0NMXQ7oAKV8XxlzPdywtpgCjWtsZsoeo`, modificato `2026-08-31T09:55Z`).
>
> **Data**: 2026-08-31 · **Base**: `origin/main` @ `7c48ce63`, **ribasato su `d2e94a62`** prima del push
> · **Modo**: critique · **Focus**: requirements
> **Esito documentale**: [`D-294`](../../decisions/RT_PDR_00_Decision_Log.md) + due escalation in
> [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md). **Nessuna riga di `Source/` toccata.**

---

## 1. Il verdetto in una riga

> **Delle cinque decisioni d'autore, una sola è sincronizzabile e non è quella che l'handoff riteneva più
> urgente: `DQA-030` ratifica un comportamento che il runtime ha già e che nessun documento dichiarava.
> Le altre quattro erano state consumate tre ore prima da [`D-291`](../../decisions/RT_PDR_00_Decision_Log.md),
> e su due di esse la riga Drive dice l'opposto di ciò che il repository ha misurato.**

Quel «opposto» non si scioglie qui. Una decisione d'autore e una misura sul codice non stanno sulla stessa
scala, e sceglierne una sarebbe stato inventare la gerarchia che
[`D-282`](../../decisions/RT_PDR_00_Decision_Log.md) vieta di inventare.

---

## 2. Ciò che è stato misurato

Ogni riga è un comando eseguito su `origin/main` @ `7c48ce63`, non una lettura. Le tre righe segnate
🔁 sono state **rimisurate su `d2e94a62`** prima del push, e due di esse la seconda volta dicono altro.

| Domanda | Comando / sede | Esito |
|---|---|---|
| L'HEAD dell'handoff esiste ancora? | `git log -1 e30361e9` | ✅ Sì, ed è **antenato**: `git rev-list --count e30361e9..HEAD` = **10**. L'osservazione ChatGPT era esatta quando fu presa |
| Qual è il massimo `D-nnn` reale? | `grep -oE '^\| \*\*D-[0-9]+\*\*'` | 🔁 **`D-291`** alla prima misura, **`D-293`** alla seconda: `D-292` (la `Guard` come pool) e `D-293` (APNAP ritirato) sono arrivate **durante** questa sessione, e la decisione è stata rinumerata `D-292` → **`D-294`** |
| Quali ID sono liberi? | stesso comando + `count()` per ID mancante | 🔁 **`286`, `287`, `289`, `290`** — zero occorrenze nel file, invariati alla seconda misura. Gli altri buchi (`44`, `159`, `161`, `164`, `165`, `174`, `213`) sono citati altrove e **non** sono liberi |
| Il Drive coincide con l'handoff? | rilettura live del tab `Open Questions` | ✅ **Verbatim su tutte e cinque.** Nessuna divergenza fra istruzione e foglio |
| Il tab finisce a `DQA-030`? | conteggio righe | ⛔ **No: arriva a `DQA-044`.** L'handoff si ferma a 030 perché lì si era fermata la chat |
| `DQA-026`…`029` sono già state consumate? | `grep 'DQA-02' docs/` | 🔴 **Sì**, da `D-291` (merge PR [#1899](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1899), **12:01** del 2026-08-31) |
| `DQA-030` è stata consumata? | `grep -rn 'DQA-030\|KO Occupancy' docs/` | ⛔ **No.** `D-291` non la nomina: non era nel kit che ha consumato |
| L'enum `ResolutionLayer` esiste? | `grep -c ResolutionLayer -- Source/` | ⛔ **0** — invariato rispetto alla misura di `D-291` |
| `PreInterrupt` · `EnvironmentPropagation` | idem | ⛔ **0** ciascuno |
| `Armor` · `DamageResistance` · `DamagePacket` · `Shred` | `grep -c … -- Source/` | ⛔ **0** ciascuno — invariato dal 2026-08-28 ([`D-238`](../../decisions/RT_PDR_00_Decision_Log.md)) |
| `TemporaryShield` | idem | ✅ **89 occorrenze** — la *catena di assorbimento* di `DQA-028` esiste, la *mitigazione* no (§4.2) |
| `Piercing` | idem | ⚠️ **2 occorrenze**, nessuna nella pipeline del danno |
| Un'unità KO blocca ancora la cella? | `RTHexSimLibrary.cpp:66` · `RTTurnManager.cpp:4975` | ✅ **No, e il codice lo dichiara**: *«i morti (es. nel Blast) non si muovono e non bloccano»* (§4.5) |
| L'occupancy si muta durante la resolution? | `grep -c 'Occupancy.Remove' -- Source/` | **0** — l'occupancy è **congelata** per snapshot e **ricostruita a ogni fase** (`RTUnit.h:41`) |
| Esiste un test che pinni KO → cella libera? | `grep -E '"RefactorTactics\.[A-Za-z.]*(Occupan\|Block\|KO\|Defeat)'` | ⛔ **Nessuno.** Il più vicino è `Combat.NewlyDefeatedDetectsFreshDeaths`, che misura altro (§5) |
| `#1733` è pertinente a `DQA-026`? | `gh issue view 1733` | ⛔ **No.** Alla misura era `OPEN` e riguarda **due unità VIVE sulla stessa cella** — occupancy, non resolution ordering. È invece l'owner reale di `DQA-040`/`DQA-041`. 🔁 **Rimisurata il 2026-08-31**: `CLOSED COMPLETED` alle `14:47Z`, e la sovrapposizione **non era mai avvenuta** — le tre righe citate come prova erano della **stessa** unità, e a produrre la diagnosi è stata l'assenza del soggetto nel TurnLog |
| `#1897` è ancora aperta? | `gh issue view 1897` | 🔁 **`CLOSED COMPLETED`** alle `09:41Z`, cioè **prima** che `D-291` la citasse come aperta — e alla seconda misura si sa da chi: [`D-293`](../../decisions/RT_PDR_00_Decision_Log.md) (§4.3) |

---

## 3. Mappatura DQA → repository

| DQA | Titolo | Sede nel repository | Esito di questa sessione |
|---|---|---|---|
| `DQA-026` | Resolution Ordering | `D-291` punto (1) | ⚡ **Conflitto** → `AUTHOR-RESOLUTION-001` in `OPEN_DECISIONS.md` |
| `DQA-027` | Same-Boundary Reactions | `D-291` punto (2) · `spec-sequenza-turno.md` §2.3, §3.1 | ✅ **Nessuna azione** — compatibile con l'esistente (§4.3) |
| `DQA-028` | Damage Contract | [`D-238`](../../decisions/RT_PDR_00_Decision_Log.md) · `D-291` punto (3) | ⚡ **Conflitto** → `AUTHOR-DAMAGE-001` in `OPEN_DECISIONS.md` |
| `DQA-029` | Same-Layer KO | `spec-sequenza-turno.md` §3.3 (`FR-RESOLVE-02`) · `D-291` punto (4) | ✅ **Nessuna azione** — già canonica |
| `DQA-030` | KO Occupancy | **nessuna** | ✅ **Sincronizzata** → [`D-294`](../../decisions/RT_PDR_00_Decision_Log.md) |

---

## 4. Le cinque voci, una per una

### 4.1 `DQA-026` — l'autore ha risposto alla domanda che `D-291` aveva lasciato aperta, e ne ha aperta un'altra

`D-291` chiude su `DQA-026` con due verdetti distinti, e **solo uno dei due** è toccato dalla riga Drive.

**Il verdetto sciolto.** `D-291` segnala che il §11.6 del kit *«rovescerebbe una policy spedita senza
saperlo»*: `RTTurnManager_Blast.cpp:1366` (CP 9.2) dichiara che *«chi ha sparato in questo Blast non guadagna
la linea perche' il muro e' caduto»*, e lascia la scelta come **decisione umana**. Lo scenario divergente che
`D-291` costruisce — due attaccanti nello stesso Blast, il primo abbatte il muro, il secondo spara — è
`ActionEffects` contro `ActionEffects`, cioè **lo stesso layer**. La riga Drive dice *«Same-layer
contributions read one State N»* e *«derived-state refresh is a checkpoint, not a gameplay layer»*: applicata
a quello scenario, il secondo attaccante legge `State N` e **non** guadagna la linea. ✅ **La decisione
d'autore conferma la policy spedita.**

⚠️ **Con una riserva che va dichiarata**: il testo Drive **non nomina** CP 9.2, né il Blast, né quello
scenario. La conferma è una *conseguenza* della regola generale, non una ratifica esplicita. Chi la userà
per chiudere il punto deve saperlo.

**Il verdetto NON sciolto, e per cui si escala.** `D-291` misura che i dieci `ResolutionLayer` proposti
duplicherebbero **sei valori su dieci** di quattro vocabolari già implementati e con contratto scritto
(`ERTMatchPhase`, `ERTResolutionPhase`, `ERTReactionPassPoint`, `ERTPredictionBoundary`) e conclude ⛔
*«L'enum non si introduce»*. La riga Drive **congela quei dieci nomi**. Le due fonti non si compongono: una
dice «congela il vocabolario», l'altra ha misurato che il vocabolario è ridondante. 🔴 **E un layer è
vietato per nome**: `PreInterrupt` è la forma degli *interrupt annidati*, che `spec-sequenza-turno.md` §4
elenca fra le voci **north-star — non costruire**, e §2.3 chiude con *«nessun interrupt annidato nella
v0.1»*.

### 4.2 `DQA-028` — la stessa formula che `D-238` ha rifiutato di congelare, e un difetto in più

La riga Drive è **la proposta di `D-238`**, parola per parola fino al `min(Armor, 0)` sotto `Piercing`.
`D-238` non l'aveva respinta per prudenza: *«`Armor` e `BaseShield` occupano già lo stesso asse»*, e
[`D-224`](../../decisions/RT_PDR_00_Decision_Log.md) ha spedito `BaseShield = 5` con **la stessa**
condizione che il kit assegna ad `Armor > 0` — ferma solo il danno `Direct`. La misura di `D-238`: con
`Armor 3`, due colpi `Direct` da 10 nello stesso turno tolgono **9 HP invece di 15**, cioè una modifica di
bilanciamento del **40%** presa per omissione. ⛔ **La riga Drive non nomina `BaseShield`**: non risponde
all'obiezione, la scavalca.

🔴 **Difetto che nessuna delle due fonti aveva registrato, misurato qui.** `URTCombatLibrary::ApplyDamage`
(`RTCombatLibrary.cpp:6`) implementa una catena di assorbimento **dipendente dalla sorgente**: il temporaneo
assorbe qualunque danno, ma lo scudo **base** assorbe *solo* il `Direct` — l'ambientale lo attraversa intero,
ed è il punto (2) di `D-224`. La riga Drive scrive la catena come **uniforme**: *«then TemporaryShield →
Shield → Health»*, senza condizione sulla sorgente, perché mette tutta la dipendenza dalla sorgente nello
step di *mitigazione* (`Environmental: ApplicableArmor = 0`). Adottare `DQA-028` alla lettera renderebbe il
danno ambientale **assorbibile dallo scudo base**, che è esattamente ciò che `D-224` ha superato pagando
**35 test rossi**. Non è un dettaglio di redazione: è una seconda collisione sullo stesso asse, e sta nella
metà della formula che `D-238` aveva dichiarato *«internamente coerente»*.

### 4.3 `DQA-027` — compatibile, e nessuna azione

La riga Drive chiede di valutare l'intero `Contribution Set` prima di scegliere le reazioni, di ordinare i
conflitti per `ReactionPriority` poi ID stabili, e di **non** far rientrare gli output nello stesso boundary.
Il repository ha già: la regola dei trigger simultanei (`spec-sequenza-turno.md` §2.3, *«una sola opportunity
multi-bersaglio, mai prompt in sequenza»*), l'ordine totale a **cinque chiavi** (§3.1) verificato da **sei**
test di permutazione, e il divieto di ricorsione (§2.3, *«nessun interrupt annidato»*). ✅ **Le tre clausole
coincidono.** Il solo elemento nuovo è la distinzione `consuming` / `shareable` degli input, che però non ha
ancora un produttore: nessun campo la esprime. Non si apre una voce per questo.

🔁 **E la ragione per cui non si apre è cambiata fra la prima e la seconda misura.** `D-291` registrava che
la domanda viva di quest'area fosse **l'ordine APNAP a sei gruppi**, con owner `piano-canonico-mvp.md` §5.1,
e apriva [#1897](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1897) per farla decidere. Alla
rimisura su `d2e94a62` quella domanda **è chiusa**: [`D-293`](../../decisions/RT_PDR_00_Decision_Log.md)
ritira `FR-RESOLVE-01` perché *«APNAP è Active Player, Non-Active Player»* e RefactorTactics è a **turni
simultanei** — nessuno «ha il turno», e i sei gruppi poggiano su un'unità attiva che non esiste. Resta in
vigore l'ordine a cinque chiavi, cioè esattamente ciò con cui `DQA-027` è compatibile.

Ne segue che `#1897` risultava `CLOSED COMPLETED` dalle `09:41Z` — **prima** del merge di `D-291` alle
`12:01` — non per una svista ma perché il lavoro che l'ha chiusa era già in volo. La riga di `D-291` è vera
del momento in cui fu scritta e obsoleta in quello in cui è stata mergiata: è la stessa finestra di poche ore
che ha prodotto la collisione di ID del §2.

### 4.4 `DQA-029` — già canonica, e nessuna azione

`spec-sequenza-turno.md` §3.3 dichiara `FR-RESOLVE-02`, ed è esercitata da
`RefactorTactics.Match.Autobattle.SimultaneousKOFollowsDeclaredPolicy`
(`RTMatchAutobattleTests.cpp:1400`), che permuta l'ordine di spawn e verifica che due unità che si uccidono
a vicenda producano `Draw` e non un vincitore deciso dall'ordine di visita. ✅ È esattamente l'invariante
`DQA-029`.

⚠️ **Riserva conservata, non risolta**: la riga **66** di [`../../DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md)
registra che `FR-RESOLVE-02` non ha *«nessuna sede esplicita nel codice: nessun `StateBased`, nessun
`CheckDeaths`»*. La regola è dichiarata e il suo esito è testato **alla scala della partita**; il controllo
fra un effetto e il successivo non ha una funzione che lo nomini. Questo referto non cambia quella riga.

### 4.5 `DQA-030` — comportamento osservato, e l'unica sincronizzazione di questo pass

La domanda: al KO l'unità smette **subito** di bloccare il proprio `FRTCellId`, o continua fino al Cleanup?
La riga Drive sceglie **A** — libera all'`Atomic Commit`.

Il repository **fa già A**, e in tre sedi indipendenti:

1. `ARTUnit::IsAlive()` è `Health > 0` (`RTUnit.h:539`) — **calcolata**, non un flag alzato in una fase.
   Un'unità portata a zero nel Blast è non-viva nell'istante in cui il danno si applica.
2. `ARTTurnManager::CollectLivingUnits` (`RTTurnManager.cpp:4964`) filtra sui vivi, e il commento lo
   dichiara: *«i morti (es. nel Blast) non si muovono e non bloccano»*.
3. `URTHexSimLibrary::MakeSnapshot` popola `Occupancy` **solo** con `Unit.bAlive`
   (`RTHexSimLibrary.cpp:66`), e `URTMatchSetupLibrary::BuildOccupancy` ripete la regola con lo stesso
   commento (`RTMatchSetupLibrary.cpp:141`).

Lo snapshot è **ricostruito a ogni fase** (`RTUnit.h:41`), quindi un'unità caduta nel `Blast` non compare
nell'occupancy del `Move` che segue: la cella è libera per chi si muove dopo. ✅ **Policy A, spedita.**

🔎 **Perché allora non è già decisa.** Nessun documento la dichiara, e un documento dice il contrario per
implicazione: [`adr-0003-modello-azioni-v01.md`](../../decisions/adr-0003-modello-azioni-v01.md) §3 mappa il
codice catalogo **60** — *«KO, verifica obiettivi, decremento cooldown, TurnLog»* — sulla macro-fase
**`Cleanup`**. Chi legge quella riga deduce **Policy B**: che il KO si materializzi nel Cleanup e che fino a
lì l'unità occupi. La riga è vera di *cosa* il Cleanup elabora (la rimozione dell'attore, il TurnLog), e
falsa di *quando* la cella si libera. È lo stesso difetto che `D-291` ha ratificato per l'Environment: un
comportamento corretto nel runtime che nessun owner scrive, e che il primo lettore deduce al contrario.

⚠️ **Ciò che `D-294` NON dichiara.** L'`Atomic Commit` della riga Drive e il confine di fase del repository
**coincidono oggi per costruzione**, non per regola: il danno da terreno del `Move`
(`RTTurnManager.cpp:5930`) si applica **dopo** che `ResolveMoves` ha risolto l'intera fase, quindi nessun
micro-step può osservare un occupante stantio. Se un giorno un effetto uccidesse **dentro** la risoluzione
di una fase, l'occupancy di quella fase resterebbe congelata (`Occupancy.Remove` = **0** occorrenze) e il
comportamento diventerebbe **B** senza che nessuno lo decida. È la condizione che il test del §5 deve
pinnare.

---

## 5. Test

**Eseguiti: nessuno.** Il pass non tocca `Source/`, e un `UnrealEditor-Cmd` di un'altra sessione era attivo
sulla macchina: avviare una seconda run automation la manda in stallo senza errore. Nessun numero di test è
dichiarato verde da questo referto.

**Il test discriminante manca, ed è uno solo.** `RefactorTactics.Match.KODoesNotBlockItsCellNextPhase`:
due unità adiacenti, `A` uccide `B` nel `Blast`, una terza unità pianifica un `Move` che **termina sulla
cella di `B`**. Con Policy A il movimento riesce; con Policy B esce `BlockedByUnit`. Oggi passerebbe al
primo colpo — ed è il punto: pinna un comportamento non protetto, non ne aggiunge uno. Il difetto che
intercetta è la regressione descritta al §4.5, e cadrebbe da sé il giorno in cui `Occupancy` diventasse
mutabile durante la fase.

Sede naturale: `Source/RefactorTactics/Tests/RTMatchAutobattleTests.cpp`, accanto a
`SimultaneousKOFollowsDeclaredPolicy`, che allestisce già lo scambio letale fra due adiacenti.

---

## 6. Cosa NON è stato fatto, e perché

| Non fatto | Perché |
|---|---|
| Sincronizzare `DQA-026` e `DQA-028` nel Decision Log | Contraddicono una decisione consolidata (`D-238`) e una misura sul codice (`D-291`). Sintetizzarle sarebbe stato inventare la gerarchia che `D-282` vieta di inventare |
| Introdurre l'enum `ResolutionLayer` | `D-291` ha misurato che duplica sei valori su dieci; `PreInterrupt` è north-star vietata |
| Aprire una issue per `DQA-027` | La domanda viva dell'area ha già un owner (`piano-canonico-mvp.md` §5.1) e `D-291` ha già rifiutato di duplicarla |
| Toccare `DQA-031`…`DQA-044` | Fuori dallo scope dell'handoff, e hanno già un `Repo ID` che punta a issue esistenti |
| Riempire i buchi `D-286`/`D-287`/`D-289`/`D-290` | Il log non ha mai riempito un buco (`44`, `159`, `161`, `164`, `165`, `174`, `213` sono lì da sempre) e `build_decision_view.py` li dichiara a ogni rigenerazione, quindi non sono voci perse. 🔴 **E il primo-oltre-il-massimo non ha protetto**: `D-292` era il primo libero quando questa decisione è stata scritta, e alla rimisura prima del push `origin/main` aveva preso `D-292` **e** `D-293`. Rinumerata a `D-294`. È la terza collisione dello stesso pomeriggio — `D-288` due volte, poi questa — e nessun ID sarà sicuro finché più sessioni scrivono nello stesso file: l'unica difesa è **rimisurare subito prima del push**, che è ciò che ha funzionato |
| Scrivere sul Google Sheet | Il `Repo ID` va aggiornato **dopo** il merge, non prima (§7) |
| Eseguire la suite | §5 |

---

## 7. Righe Drive da aggiornare dopo il merge

Il tab `Open Questions` **non va toccato prima** che `D-294` sia su `main`. Le righe sono identificate per
`DQA ID` e non per numero di riga: la numerazione osservata dall'handoff (27…31) era già sfalsata perché
l'intestazione del tab non è la riga 1 del foglio.

| `DQA ID` | Colonna `B` (`Repo ID`) — valore atteso dopo il merge | Colonna `K` (`Repo Owner`) |
|---|---|---|
| `DQA-030` | `D-294` | rimuovere il `pending repo sync`: la decisione è nel Decision Log |
| `DQA-026` | invariato (`NEW — Resolution Ordering`) | ⚡ aggiungere il rinvio a `AUTHOR-RESOLUTION-001` in `OPEN_DECISIONS.md`: **non** è sincronizzata, è **in conflitto** |
| `DQA-028` | invariato (`NEW — Damage Contract`) | ⚡ idem, `AUTHOR-DAMAGE-001`. La decisione corrente del repository resta `D-238` |
| `DQA-027` | `D-291` (riferimento, non sincronizzazione) | l'area ha owner: `piano-canonico-mvp.md` §5.1 |
| `DQA-029` | `D-291` (riferimento) · `spec-sequenza-turno.md` §3.3 | già canonica, nessun `pending` |

⛔ **Nessuna riga `RESOLVED — AUTHOR DECISION` va riscritta**: la colonna `M` (`Answer / Resolution`) è
dell'autore, e questo referto non la tocca.

---

> 🔁 **Nota aggiunta il 2026-08-31 (secondo passaggio) — `DQA-040` e `DQA-041` hanno entrambe un owner, e nessuna delle due ne aspetta uno nuovo.**
>
> - **`DQA-040`** (la carica termina sulla cella adiacente, mai su quella del bersaglio) è consumata da [`D-296`](../../decisions/RT_PDR_00_Decision_Log.md), già implementata e pinnata da un test.
> - **`DQA-041`** (attraversare celle occupate intermedie, mai terminarci sopra) è **già canonica, e misurata nel codice**: `URTHexSimLibrary::StepHexMovement` calcola `bCrossesStationary = PassesThrough(i) && !bFinalStep`, cioè chi attraversa passa **solo** se quella cella non è la sua ultima — e il commento in loco lo dice per esteso: *«si transita dentro qualcuno, non ci si ferma … due unità nella stessa cella a fine turno non sono rappresentabili»*. Il *«solo se lo stile lo permette»* è il flag `bPassThrough`, ed è la riga *«attraversa le celle intermedie: **policy**»* di [`spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md). Una destinazione occupata **ferma** l'unità sull'ultima cella libera raggiunta: è il punto fisso monotono, non un'eccezione scritta a parte.
>
> ⛔ **Nessuna nuova `D-nnn` per queste due**: sarebbe un numero speso su una regola già posseduta. ⚠️ E `#1733` **non è la prova** che `DQA-041` sia rotta — quell'issue è stata chiusa proprio perché la sovrapposizione non era mai avvenuta.
