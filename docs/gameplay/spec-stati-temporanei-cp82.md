# Spec — Stati temporanei (E8, CP 8.2)

> 👁️ **`Obscured` non è più solo osservabilità futura** (2026-08-08): è un **input reale** della conoscenza
> parziale (**E13**, [`brief-conoscenza-parziale.md`](brief-conoscenza-parziale.md)) e concorre con
> [ADR-0005](../decisions/adr-0005-orientamento.md) a decidere il livello di contatto. Il cap di targeting
> definito qui resta; la **semantica di detection** è posseduta da E13, non da questa spec.
>
> 🧹 **Ordine del Cleanup — una sola fonte.** L'ordine definitivo è quello della spec ambientale più recente
> ([`spec-fuoco-acqua-cp84.md`](spec-fuoco-acqua-cp84.md), che lo ha esteso con il terreno dinamico). Se le
> due divergono, **prevale quella**: qui resta la parte sugli **stati**, non l'ordine complessivo. Due
> documenti che definiscono lo stesso ordine sono due fonti normative, cioè nessuna.

> 📌 **Stato di implementazione storico al 2026-08-07 (CP 8.2).** Numeri, conteggi di test ed esiti qui sotto fotografano
> la chiusura del checkpoint. Lo **stato corrente** è posseduto da
> [`../roadmap/roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md): questa spec non compete con la roadmap come
> fonte di stato.

> **Issue**: `#65` · **Epic**: `#22` (E8) · **Dipende da**: `#64` (CP 8.1, chiusa) · **Data**: 2026-08-07
> **Branch**: `feat/65-stati-temporanei` · **Baseline misurata**: 325 test, 0 fallimenti
> Fonti: [`RT_TerrainCatalog_v0.1.md`](../balance/RT_TerrainCatalog_v0.1.md) §2 ·
> [`RT_ActionCatalog_v0.1.md`](../balance/RT_ActionCatalog_v0.1.md) §3 · [`spec-terreni-e8.md`](spec-terreni-e8.md) §6-bis ·
> [`adr-0003-modello-azioni-v01.md`](../decisions/adr-0003-modello-azioni-v01.md) §3

## 1. Obiettivo

Gli otto stati del catalogo smettono di essere **tag dichiarati** e diventano **regole che decidono l'esito**:
durata, scadenza deterministica nel Cleanup ed effetto osservabile in partita.

Il checkpoint non inventa numeri: costanti ed effetti sono già a catalogo. Quello che manca è, in tre casi su
otto, **il cablaggio** — il difetto ricorrente già visto in `#137` (`Marked`) e al CP 4.7 (`Action.Slow`): la
formula è coperta da un test sulla funzione pura, e nessuno in partita gliene passa mai un valore.

## 2. Stato verificato (2026-08-07, sul codice)

| Stato | Tag | Chi lo applica | Effetto in partita | Verdetto |
|---|---|---|---|---|
| `Root` | ✅ `RTGameplayTags.h:6` | `Action.Root`, `Guardian.Quake` | ✅ azzera movimento e scatto (`RTUnit.cpp:267,274`) | **completo** |
| `Slow` | ✅ `:7` | `Action.Slow`, `Ranger.Burst` | ✅ +1 costo per cella (`RTTurnManager.cpp:1782`) | **completo** |
| `Exposed` | ✅ `:9` | `Action.Sprint` (`RTCatalogLibrary.cpp:245`) | ✅ +5 al primo danno diretto (`RTTurnManager.cpp:1399`) | **completo** |
| `Marked` | ✅ `:11` | `Action.MarkTarget` (`RTCatalogLibrary.cpp:381`) | ❌ **mai letto**: il loop `FirstHitDelta` legge solo `Exposed`/`Guarded` | **issue `#137`** |
| `Wet` | ✅ `:16` | `ShallowWater` con durata **0** (`RTTerrainLibrary.cpp:33`), Phase con durata 1 | ❌ doppio buco: la durata 0 è scartata da `ApplyStatus`, e `GadgetWetDischargeBonus` compare **solo nei test** | **inerte** |
| `Burning` | ✅ `:20` | `Fire`, durata 2 (`RTTerrainLibrary.cpp:36`) | ❌ il Cleanup non infligge alcun danno (`RTTurnManager.cpp:558-578`) | **inerte** |
| `Obscured` | ✅ `:21` | `Smoke` con durata **0** (`RTTerrainLibrary.cpp:39`) | 🟡 il cap di targeting a 2 celle **esiste e funziona**, ma deriva dal terreno (`EffectiveTargetingRange`), non dallo stato | **stato inerte, regola viva** |
| `Electrified` | ❌ assente | — | ⏳ propagazione = CP 8.3 (`#66`) | **da dichiarare** |

**Conteggio**: 3 stati su 8 funzionano, 1 è a metà, 3 sono inerti, 1 non esiste. Il difetto centrale è uno solo:
`ARTUnit::ApplyStatus` rifiuta silenziosamente ogni durata `<= 0` (`RTUnit.cpp:228`), e il catalogo usa proprio
`0` per dire «finché sulla cella».

## 3. Decisioni

### D1 — «Finché sulla cella» = durata sentinella, revocata nel Cleanup

`ApplyStatus` accetta una durata sentinella `ARTUnit::PersistentWhileOnCell` (`-1`) che significa: **non
decrementa, non scade da sola**. Nel Cleanup, *prima* del tick delle durate, ogni stato persistente viene
**revocato** se la cella su cui l'unità ha terminato il turno non lo dichiara più fra i propri `OnEnterEffects`.

*Perché*: `StatusTurns` resta l'**unica verità** e `HasStatus` l'unica API. Gadget, l'HUD, il bot e il combat log
continuano a leggere lo stato senza dover conoscere la mappa — che è il costo dell'alternativa «stato derivato
al volo». Non introduce latenza: la revoca guarda la posizione **finale** del turno, quindi chi esce dall'acqua
nel Move di T non è più `Wet` nel Blast di T+1.

*Convivenza con le durate esplicite*: `ApplyStatus` mantiene la regola «non accorciare» — un `Wet` di Phase
(1 turno) applicato a chi sta già in acqua non degrada il persistente, e un persistente non cancella una durata
in corso. La regola di precedenza è: **il persistente vince finché la cella lo sostiene**, poi resta l'eventuale
durata residua.

### D2 — `Marked`: pass a priorità nel Blast, consumato dal primo colpo alleato

Fedele al catalogo azioni §3 («il **prossimo attacco alleato** contro il bersaglio infligge +6 e **consuma** il
marchio»). Tre conseguenze di progetto:

- il delta non può più essere solo *per bersaglio*: `Marked` ha bisogno della **squadra del marcatore**, perché
  un colpo che non viene dalla squadra di chi ha marcato non deve beneficiarne né consumarlo;
- il tag viene **rimosso nel Blast** quando un colpo lo consuma, non lasciato scadere nel Cleanup: così l'HUD e
  `Action.Cleanse` non mostrano un marchio già speso;
- **il marchio si applica prima del calcolo dei danni** dei colpi a priorità più alta dello stesso Blast.

> ⚠️ **Scoperta durante l'implementazione (2026-08-07)**: la issue `#137` ipotizzava «un fix da una riga più un
> test». **Non è così.** Gli effetti di stato del Blast si applicano *dopo* il calcolo dei danni
> (`RTTurnManager.cpp`: danni ~1486-1592, stati ~1789) e `Marked` ha durata 1, quindi scade nel Cleanup dello
> stesso turno in cui viene applicato: **non esiste alcun turno in cui il marchio sia spendibile**, e
> aggiungere la riga mancante al loop `FirstHitDelta` non lo renderebbe utilizzabile.
>
> Il catalogo dice *perché* deve valere subito: `MarkTarget` ha priorità **40**, «la più bassa delle offensive,
> perché un marchio che arrivasse dopo i colpi non servirebbe a nulla». Si introduce quindi un **pass a
> priorità**: i marchi dichiarati dagli intenti si applicano prima, e il +6 va al primo colpo qualificato con
> priorità **strettamente maggiore** (`PrecisionAttack` 60, `BasicAttack` 50). Resta «raccogli poi applica» —
> si aggiustano i `Power` di colpi già congelati, non si ricalcolano bersagli. Il precedente nel codice è
> `Action.Interrupt` (priorità 20), che filtra i colpi del piano prima che diventino danno.
>
> L'ordine di spesa è totale (`priorità → ActionId → AttackerId → IntentIndex`): «il *prossimo* attacco
> alleato» deve avere una sola risposta anche quando due alleati colpiscono lo stesso bersaglio nello stesso
> turno.

### D3 — `Electrified`: tag dichiarato, consumatore assente e dichiarato

Il catalogo terreni §2 lo definisce **istantaneo** («una sola volta per evento»), quindi non è uno stato con
durata e **non entra in `StatusTurns`**. Qui si consegna il tag e la semantica documentata; l'applicazione reale
nasce con la propagazione di CP 8.3. È lo stesso pattern di `PushResistance` di Riktor: dato pronto,
consumatore dichiarato assente — non un effetto finto con durata inventata.

### D4 — `Obscured` non diventa un secondo gate del targeting *(motivata, non opzionale)*

Il cap a 2 celle resta calcolato da `URTTerrainLibrary::EffectiveTargetingRange` sulla **linea** dell'attacco:
è la stessa primitiva `HexLine` su cui si spara, ed è già coperta da `Terrain.Smoke.CapAgreesAcrossGates`.
Lo stato `Obscured` diventa reale (applicato e revocato come `Wet`), ma il suo ruolo è **osservabilità**:
HUD, combat log e — più avanti — E13. Aggiungere un secondo criterio produrrebbe due verità che divergono
appena una delle due cambia.

### D5 — Il danno di `Burning` erode lo scudo temporaneo e precede i KO *(motivata)*

Il danno di fine turno passa da `URTCombatLibrary::ApplyDamage` + `ApplyCombatState`, come il fuoco
all'ingresso: è già il comportamento coperto da `Terrain.Fire.ErodesTemporaryShieldFirst`, e scrivere
`Health` a mano farebbe sottrarre due volte lo scudo temporaneo (la trappola documentata a
`RTTurnManager.cpp:83`). L'ordine del Cleanup diventa quello di §4.

### D6 — Durata di `Wet` fuori dall'acqua = 1 turno *(chiude un «non specificato» del catalogo)*

Il catalogo terreni §4 lascia aperta la durata di `Wet` applicato lontano dall'acqua. Il catalogo eroi la
dichiara già **1 turno** per `Hero.Phase.PressureJet` e `Hero.Phase.CircularTide`: si adotta quel valore e si aggiorna il
catalogo terreni, invece di tenere aperta una domanda a cui il repository ha già risposto.

## 4. Ordine del Cleanup (nuovo)

Oggi (`RTTurnManager.cpp:558-578`): energia → scadenza scudo temporaneo → tick stati → tick cooldown → conteggio
vivi. Il conteggio produce `PendingOutcome`, cioè decide la partita.

Nuovo ordine, con la regola dell'ADR-0003 §3 («gli effetti ambientali risolvono nel Cleanup **prima dei KO**»)
e la DoD di `#65` («scadenza applicata **dopo** gli effetti ambientali»):

1. **Effetti ambientali**: danno di `Burning` (8), erosione dello scudo temporaneo inclusa;
2. **Revoca degli stati legati alla cella**: `Wet`/`Obscured` persistenti che la cella finale non sostiene più;
3. **Scadenza**: `TickStatuses` sulle sole durate `> 0`;
4. Energia, scadenza dello scudo temporaneo, cooldown;
5. **Conteggio delle unità vive** → `PendingOutcome`.

Chi muore bruciato muore **in questo turno**: il KO da hazard conta per l'esito, non slitta al turno dopo.

## 5. Fette di lavoro

Ogni fetta chiude con build + suite verde. L'ordine non è negoziabile: la fetta 1 è il modello su cui poggiano
tutte le altre.

| # | Contenuto | Test nuovi | Stato |
|---|---|---|---|
| **1** | Durata sentinella in `ARTUnit` + revoca nel Cleanup; il catalogo terreni smette di essere inerte | `Status.PersistsWhileOnCell`, `Status.RevokedOnLeavingCell`, `Status.ExpiresInCleanup` | ✅ |
| **2** | `Burning`: 8 danni nel Cleanup, 2 turni, prima dei KO | `Status.Burning.DamagesInCleanup`, `Status.Burning.ExpiresAfterTwoTurns`, `Status.Burning.DefeatCountsThisTurn` | ✅ |
| **3** | `Wet` rimuove `Burning` + cablaggio del +8 di Gadget | `Status.WetRemovesBurning`, `Status.Wet.AmplifiesFluxDischarge` (integrazione) | ✅ |
| **4** | `Marked`: pass a priorità e consumo dal primo colpo alleato (chiude `#137`) | `Status.Marked.AllyHitConsumesBonus`, `Status.Marked.EnemyHitDoesNotConsume` | ✅ |
| **5** | `Obscured` reale + `Electrified` dichiarato | `Status.Obscured.AppliedBySmokeWithoutChangingGate` | ✅ |
| **6** | Documentazione: cataloghi, `spec-terreni-e8.md` §6-bis, roadmap (entrambe le viste), PR | — | ✅ |

**Misura finale**: suite da **325** (baseline sul branch) a **338** test, 0 fallimenti. Build Editor **e** Game verdi.
Gli ultimi 2 test sono i due difetti di cablaggio corretti su richiesta (§6.1).

### 5.1 Verifiche di mutazione eseguite

| Mutazione | Test attesi | Esito |
|---|---|---|
| `RevokeCellBoundStatusesNotIn` resa un no-op | `Status.RevokedOnLeavingCell`, `Terrain.Status.LogMatchesState` | ✅ cadono esattamente quei due |
| Controllo di squadra del marchio disattivato | `Status.Marked.EnemyHitDoesNotConsume` | ✅ cade solo quello |

Le altre fette sono coperte dal ciclo RED osservato: ogni test è stato eseguito e **visto fallire** con il
valore atteso prima dell'implementazione (es. `Burning`: 70 HP invece di 62, cioè il solo danno d'ingresso).

## 6. Fuori scope dichiarato

- **Propagazione elettrica** e applicazione reale di `Electrified` → CP 8.3 (`#66`).
- **Spegnimento del fuoco da parte dell'acqua sulla *cella*** (`Environment.WaterExtinguishesFire`) → CP 8.4
  (`#67`). Qui `Wet` rimuove `Burning` **dall'unità**: è l'altra metà, e il catalogo le distingue.
- **`Action.Ignite` / `CreateWater`** (modifica dinamica della superficie) → CP 8.5 (`#68`).
- **Conduttività di cella** per `Hero.Gadget.ConductiveNode` → nessun modello di cella conduttiva esiste ancora
  (`RTHeroCatalogLibrary.cpp:167`); resta a CP 8.3.
- **HUD**: `RTHUD.cpp:89` mostra un solo stato per unità (ROOT/SLOW). Con otto stati serve una decisione di
  presentazione che appartiene a E11 (CP 11.1), non a questo checkpoint.
- **TurnLog**: il danno di `Burning` compare nel **combat log**, non nel TurnLog. Le categorie sono fisse
  (`Move/Combat/Fallback/Reaction`, `RTTurnLog.h:17`) e il formato è serializzato e versionato: aggiungere una
  categoria ambientale è un lavoro con la propria DoD (migrazione + checksum), e serve a CP 8.3/8.4, dove gli
  eventi ambientali diventano molti.

## 6.1 Difetti di cablaggio scoperti — **corretti su richiesta (2026-08-07)**

Trovati mentre si cablavano gli stati, tutti dello stesso schema («la formula è testata, il chiamante non
esiste»). Segnalati come fuori scope e poi corretti su richiesta esplicita, con test d'integrazione propri.

| Difetto | Evidenza | Correzione |
|---|---|---|
| Il **fuoco amico non era mai attivo in partita**: `bFriendlyFire` è dichiarato dal catalogo e rispettato dal resolver puro, ma l'intento nasceva sempre a `false` | `RTCatalogLibrary.cpp:361` (dato) · `RTHexCombatLibrary.cpp:124` (resolver) · l'intento non lo riceveva | `Intent.bFriendlyFire = Instance.Def.bFriendlyFire` + `Actions.AoE.FriendlyFireInMatch` |
| Le azioni del catalogo con **`Range 0`** si validavano a portata nulla e degradavano sempre al fallback | `Action.MarkTarget` e `Action.PrecisionAttack` dichiarano entrambe 0 | vedi sotto: il valore era giusto, sbagliato era il ponte |

### La diagnosi iniziale era sbagliata, e come

Avevo riportato «`MarkTarget` ha Range 0, serve decidere un valore». **Non era così.** `Action.PrecisionAttack`
dichiara anch'essa `Range 0`, e il catalogo documentale ne spiega il motivo: «range dell'**arma** +1». Lo zero è
la **convenzione del catalogo per “portata del portatore”**, non un numero mancante.

Il difetto vero era il ponte che traduce quello zero: copriva le sole azioni **senza `ActionId`** («abilità non
ancora catalogate»), quindi per le azioni catalogate la validazione usava `0` mentre l'intento leggeva il campo
dell'asset — **due verità sulla stessa portata**, e l'azione non arrivava mai a bersaglio. La correzione estende
il ponte a `RangeCells <= 0` e non inventa nessun valore di bilanciamento.

Verifica: `Actions.MarkTarget.ReachesTarget` monta l'azione **dal catalogo senza toccarne la portata** e misura
l'effetto (il +6 sul colpo dell'alleato), perché il tag ha durata 1 e non è osservabile dopo il Cleanup.

**Effetto collaterale positivo**: con il fuoco amico attivo, `Status.Marked.EnemyHitDoesNotConsume` è stato
riscritto nella forma naturale — un AoE della squadra del bersaglio lo colpisce senza prendere né consumare il
marchio — eliminando il setup artificiale che era stato dichiarato come limite.

## 7. File coinvolti

`Unit/RTUnit.{h,cpp}` (sentinella, revoca, tick) · `Turn/RTTurnManager.cpp` (Cleanup, `FirstHitDelta`,
`ApplyTerrainOnEnterEffects`) · `Combat/RTCombatResolver.{h,cpp}` (delta per squadra del marcatore) ·
`Core/RTGameplayTags.{h,cpp}` (`Electrified`) · `Terrain/RTTerrainLibrary.cpp` (durate sentinella) ·
`Tests/RTStatusTests.cpp` (nuovo) · `docs/balance/RT_TerrainCatalog_v0.1.md` ·
`docs/gameplay/spec-terreni-e8.md` · `docs/roadmap/roadmap-v0.1.md` · `docs/roadmap/roadmap-checkpoint.md`.

## 8. Rischi

| Rischio | Mitigazione |
|---|---|
| La sentinella `-1` attraversa codice che assume «durata = interi positivi» (serializzazione, TurnLog, HUD) | Censimento dei lettori di `StatusTurns` prima della fetta 1; il tick ignora esplicitamente i valori `< 0` |
| Il delta per coppia (D2) cambia una funzione pura già coperta da test | La firma esistente resta e continua a valere per `Exposed`/`Guarded`/`Deflect`: il caso «marchio» è un secondo passaggio, non una riscrittura |
| Il danno di Cleanup allunga ancora le partite o le accorcia troppo | `#96` (25 turni contro i 12 del catalogo) è aperta: `Burning` toglie 8 HP/turno, quindi spinge nella direzione giusta. Nessun ribilanciamento qui: si misura e si riporta nella issue |
| Un test che replica la formula invece di chiamare il gioco | Le fette 3 e 4 chiudono con test **d'integrazione** in `UWorld`, come chiede `#137` |
