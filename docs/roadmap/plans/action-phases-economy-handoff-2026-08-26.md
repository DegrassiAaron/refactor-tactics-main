# Fasi delle azioni, Dodge, Guard, Brace, Interact, Overwatch — handoff riconciliato

> 🔁 **Riscrittura del handoff ricevuto il 2026-08-26** (`CLAUDE_ActionPhases_Dodge_Guard_Brace_Overwatch_Epics_v1.0`).
> L'originale prescriveva un audit e poi lo dava per svolto: metà delle sue tesi descrive cose che il
> repository fa già, e la sua roadmap `AE-*` fino alla `v1.0` è **la seconda roadmap che il documento stesso
> vieta** — la ladder `v0.2 → v1.0` esiste, con le sue epic, in
> [`../roadmap-post-v0.1.md`](../roadmap-post-v0.1.md). Questo file è quel documento **dopo** l'audit.
>
> **Misurato su** `c2bbfb7` (2026-08-26), branch `claude/greybox-work-wg4w85`. Ogni riga marcata *misurato*
> ha un file e una funzione dietro; ogni riga marcata *da decidere* non ne ha, e non se ne inventa una.
>
> ⛔ **Non autorizza a implementare niente.** Un handoff non è autorità
> ([`../../../CLAUDE.md`](../../../CLAUDE.md) §4): le tre tesi che confliggono con decisioni consolidate
> passano da una Decision Issue, non da questo file.
>
> 📌 **Ultimo `D-nnn` assegnato: `D-195`** (2026-08-26). Le decisioni che nascono da qui partono da `D-196`
> e si **riverificano prima del merge**.

---

## 0. Verdetto dell'audit, tesi per tesi

| # | Tesi del handoff | Esito misurato | Cosa resta |
|---|---|---|---|
| 4.1 | `Dash` è fase, `Dodge` è l'azione | ⚠️ **il problema è reale**: `ERTMatchPhase::Dash` e `Action.Dash` convivono | §4 — rinomina, con i nomi già occupati sul tavolo |
| 4.2 | Il movimento in Dash non è un Move gratis | ✅ **già così**, e non per policy: lo scatto **occupa lo slot movimento** (`D-028`, `D-191`) | §2 — niente `PostDashMovePolicy` da inventare |
| 4.3 | Overwatch standard guarda solo il Move | ✅ **già così, per costruzione**: il trigger vive dentro `ResolveMovement` | §2 — un test che lo pinni |
| 4.4 | Brace è il counter anti-Dash | 🔴 **conflitto**: oggi `Action.Brace` è mitigazione danno + `Root` | §3 — Decision Issue |
| 4.5 | Guard nega Move e Reaction | 🔴 **falso in entrambe le metà**, e rovesciato: è **Brace** che nega il movimento | §3 — Decision Issue |
| 4.6 | Interact consente Move, nega Reaction | ⚠️ metà vera: il Move è libero; la Reaction **non** è negata da nessuno | §3 — stessa Decision Issue di 4.5/4.7 |
| 4.7 | La Reaction non è gratuita | 🔴 **conflitto frontale**: lo slot Reaction è **indipendente** per decisione (CP 5.1, E5) | §3 — è *la* domanda del dominio |
| 4.8 | Prep ospita Guard/Brace/Overwatch | ✅ **misurato**: tutte e tre in `ERTResolutionPhase::Preparation` | niente |
| 4.9 | `Sprint` non è `Dodge` | ✅ deciso (`D-015`, `D-116`) ⏳ **non implementato**: debito dichiarato di E38 | §2 — non è lavoro nuovo |
| 9 | Ladder `AE-*` fino a v1.0 | ⛔ **duplicato**: `E38` e `E40`–`E45` possiedono già quelle release | §6 — mapping, non epic nuove |

---

## 1. Lo stato misurato

### 1.1 Le due nozioni di «fase», che il handoff fondeva in una

```text
ERTMatchPhase      Planning · Prep · Dash · Blast · Move · Cleanup · MatchEnded   (RTTurnRules.h)
ERTResolutionPhase Snapshot · Preparation · FastMovement · NormalMovement ·
                   Control · Attack · Environment · Cleanup                       (RTActionDef.h)
```

La `ResolvePhase` del handoff **esiste già** ed è `ERTResolutionPhase` — ma non è la fase in cui l'azione
risolve davvero: la conversione è `URTCatalogLibrary::MapResolutionPhase`, e i codici `0/10/20/…/60` restano
perché sono la chiave di lettura del catalogo. Il codice **20 si sdoppia** (`FastMovement` → macro-fase
`Dash`, `NormalMovement` → macro-fase `Move`): è l'unica divergenza strutturale dal catalogo, decisa in
[ADR-0003](../../decisions/adr-0003-modello-azioni-v01.md) §3.

Il vincolo «una sola `ResolvePhase` per azione concreta» che il handoff chiede come lavoro **è già la forma
del dato**: `FRTActionDef` porta un campo, non una maschera. Non c'è un validatore da scrivere, c'è un
invariante che nessuno può violare senza cambiare il tipo.

### 1.2 L'economia, misurata sul catalogo spedito

`ERTActionSlot` = `None · Movement · Main · MovementAndMain · Reaction`. La riga che decide tutto il §3 di
questo file è il commento dello slot `Reaction` in `RTActionDef.h`:

> *«0-1 per turno, indipendente da Movimento e Principale — un eroe può muoversi, agire **E** tenere una
> reazione pronta nello stesso turno.»*

| Azione | `ResolutionPhase` → macro | Slot | Effetti dichiarati | Move dopo | Reaction |
|---|---|---|---|---|---|
| `Action.Guard` | `Preparation` → Prep | `Main` | `Status.Guarded` 1t | **sì** | **sì** |
| `Action.Brace` | `Preparation` → Prep | `Main` | `Status.Braced` + **`Status.Root`** 1t | **no** (per `Root`) | **sì** |
| `Action.Overwatch` | `Preparation` → Prep | `Main` | — (arma una zona) | solo `Withdraw` (`D-070`) | **sì** |
| `Action.Interact` | `Attack` → **Blast** | `Main` | `SetDoorState(Open)`, portata 1 | **sì** | **sì** |
| `Action.Dash` | `FastMovement` → Dash | `Movement` | `LinearDash`, 3 celle, cd 1 | **no** (slot speso) | **sì** |
| `Action.Sprint` | `FastMovement` → Dash ⏳ | `MovementAndMain` ⏳ | `Budget`, 8 MP | — | — |

⏳ Le due celle dello `Sprint` sono **debito dichiarato**, non lo stato voluto: `D-015` e `D-116` lo vogliono
profilo del `Move` risolto **dopo** il Blast. Vedi §2.3.

> ⚠️ **Il handoff aveva Guard e Brace scambiati.** Chiede `Guard: Move unavailable` e lascia il Brace da
> «riconciliare»; il catalogo dice l'opposto — la `Guard` non tocca il movimento, ed è il `Brace` a
> inchiodare chi lo pianifica con `Status.Root`. Chi avesse eseguito il handoff alla lettera avrebbe tolto
> il movimento alla Guard e non si sarebbe accorto che al Brace era già tolto.

### 1.3 Le sette generiche, e le quattro che il catalogo consegna

`GetGenericActionIds()` restituisce **quattro** ID — `Wait · Guard · Brace · Overwatch`. Le altre tre
dell'elenco canonico di [D-025](../../decisions/RT_PDR_00_Decision_Log.md) arrivano per altre strade
(`Move` da `PlannedPath`, `BasicAttack` dall'indice 0 del kit, `Interact` dal catalogo core). `Action.Dash`
**non è fra le sette**: è lo scatto generico di fallback del catalogo core, ed è esattamente la ragione per
cui §4 esiste.

---

## 2. Le tesi già vere: si pinnano, non si implementano

### 2.1 L'Overwatch guarda già solo il Move — per costruzione, non per regola

`ResolveDash()` e `ResolveMovement()` sono due funzioni distinte di `ARTTurnManager`, chiamate da rami
diversi del ciclo di fase. Il ciclo a micro-step in cui l'Overwatch valuta i suoi trigger —
`URTHexSimLibrary::BeginHexMovement` + `FRTMovementResolutionState` — vive **dentro `ResolveMovement`**.
Uno scatto non ci passa: non c'è nessun `position changed` generico da filtrare, perché il produttore di
opportunity non vede la fase Dash.

Vale anche per lo spostamento forzato, e per la stessa ragione: non attraversa quel ciclo.

`ERTReactionTrigger` conferma da un secondo lato — i suoi valori sono `HitByDirectAttack`,
`AllyHitByDirectAttack`, `AboutToBeDisplaced`, `AboutToReceiveControl`, `CellBecameHazardous`. **Nessun
trigger di movimento**: l'Overwatch non è una reazione di quell'enum, è armato a parte (`FRTArmedOverwatch`).

> **Quindi il lavoro non è filtrare, è impedire la regressione.** Il giorno in cui qualcuno unificasse i due
> percorsi di movimento — che è una semplificazione plausibile — l'Overwatch comincerebbe a scattare sugli
> scatti **in silenzio**. Un test che lo pinni vale più della regola scritta.

⚠️ **Un punto che il handoff non nomina e che invece è il vero rischio**: l'Overwatch pretende contatto
`Rilevato` (un contatto `Incerto` è informazione, non un bersaglio). Se un giorno l'eleggibilità si
allentasse, il filtro di fase resterebbe intatto e il difetto sarebbe altrove.

### 2.2 «Dash + Move» non esiste, e non serve una policy per dirlo

[D-028](../../decisions/RT_PDR_00_Decision_Log.md) (2026-08-08) ha spostato `Dash · Leap · Reposition` sullo
slot **Movimento**; [D-191](../../decisions/RT_PDR_00_Decision_Log.md) (2026-08-25) ci ha portato anche la
`Charge`, superando la clausola che la teneva sulla principale. Un turno dà un movimento e un'azione
principale: la scelta è **quando** muoversi — *schivo e sparo* oppure *sparo e muovo*.

L'implementazione è in `ResolveDash`, che **scarta il percorso pianificato** e porta `PlannedCell` sulla
cella d'arrivo dello scatto, con esito `ERTMoveOutcome::SupersededByDash`. È eseguibile:
`Scenarios/Spec/Movement/DashDiscardsPlannedMove.json` (`#924`), che dichiara scatto e waypoint verso due
destinazioni incompatibili e misura quale vince.

Il `PostDashMovePolicy: None | Reduced | Normal` del handoff non è quindi un modello mancante: è una
**leva di bilanciamento nuova** su una decisione chiusa. `Reduced` in particolare non ha oggi dove
appoggiarsi — i profili di movimento **non esistono come tipo** (`grep -rn MovementProfile Source/` → zero,
misurato il 2026-08-12 e ancora vero), ed è il prerequisito `#653` di E38.

Le eccezioni per eroe non vanno inventate qui: la domanda è già aperta come **`AE-7`**, e porta già il caso
concreto (*`Dash` + attacco + `Move` normale*) con l'analisi di chi renderebbe ridondante.

### 2.3 `Sprint` ≠ `Dodge`: deciso due volte, implementato zero

`D-015` lo dichiara profilo del `Move`; `D-068` rovesciò; `D-116` (2026-08-12) rovescia di nuovo e vince,
perché restando pre-Blast lo Sprint **spara da una posizione nuova** — cioè fa ciò che il catalogo
attribuisce al Dash. Nel codice `Action.Sprint` è ancora `FastMovement` + `MovementAndMain`: divergenza
**misurata e tracciata** ([`../../DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) riga 41), lavoro
di E38, bloccata su `#653`.

⚠️ **Non si migra da sola** — la stessa decisione porta `Status.Exposed` a 2 turni e la compatibilità col
profilo, e la migrazione presa da sola produce l'upgrade puro vietato da `D-015`. Chi tocca lo Sprint
«per allineare i nomi» rompe il bilanciamento.

---

## 3. Le tesi che confliggono: una Decision Issue, non un commit

Le tre voci sotto sono **una sola domanda** vista da tre lati, e vanno decise insieme o non decise.

### 3.1 La domanda: la Reaction resta indipendente?

Il handoff §4.7 chiede che la capacità di reazione sia una risorsa forte, spenta da Guard, da Interact e
in parte dalle azioni di Dash. Il repository ha deciso il contrario, e non per distrazione: lo slot
`Reaction` è **indipendente per progetto** (CP 5.1, epic E5), e la sua indipendenza è ciò che rende
pianificabile il turno «mi muovo, agisco e resto pronto».

Adottare il handoff significa **togliere un asse all'economia del turno**. Può essere giusto — è
esattamente il genere di leva che `D-114` cercava quando ha confermato gli slot e spostato il peso sui
*drawback* — ma è una decisione di dominio, con quattro consumatori (catalogo, validatore, HUD, pesi del
bot) e i golden replay da rifare.

> 📌 `D-114` dice che il peso di un'azione si paga in **drawback**, e nomina *«rinuncia alla reazione»*
> fra quelli. Il handoff sta chiedendo di **generalizzare quel drawback a tre azioni core**. Questa è la
> tesi più forte del documento, ed è l'unica che merita di diventare una decisione.

### 3.2 Il Brace: mitigazione o counter anti-Dash?

Oggi `Action.Brace` dà `Status.Braced` (−10 su **ogni** danno diretto fino al Cleanup, contro il −15 sul
**primo** della `Guard`) e `Status.Root`. È una difesa di durata contro un'identità di difesa di picco, e
la coppia è coerente: due prezzi diversi per due minacce diverse. Gli scenari esistono e girano
(`Scenarios/Spec/Brace/*`, `Scenarios/Spec/Facing/BraceHoldsFromBehind.json`).

Il handoff propone un Brace che **intercetta l'ingaggio della fase Dash** — arresto della carica,
resistenza al displacement, Stagger sull'attaccante. Non è un payload da aggiungere: è un'**altra azione**
che occuperebbe lo stesso ID.

⚠️ Va notato che una fetta di quell'intento esiste già altrove, e non nel Brace: `Reaction.Anchor` ha il
trigger `AboutToBeDisplaced` e lo scenario `Spec/Reaction/AnchorCancelsPush.json`. Chi decide deve dire se
il Brace **assorbe** Anchor o gli si affianca — due entità che annullano la stessa spinta si pagano a ogni
lettura del TurnLog, che è il criterio con cui `D-070` scartò `Reposition` e `D-082` scartò `Breach`.

### 3.3 La Guard: nega il movimento o no?

Il handoff la vuole `Move: unavailable`. Il catalogo non le dà `Root`, quindi oggi la Guard **non** nega
il movimento, e `Move + Guard + reazione d'eroe` — la combinazione che il handoff chiama esplicitamente
inaccettabile — è **legale e pianificabile**.

Se 3.1 si decide «sì», questa cade di conseguenza per la sola metà della reazione; la metà del movimento
resta una decisione a sé, e va guardata insieme al `Root` del Brace, o le due difese finiscono con lo
stesso prezzo.

> **Ciò che non è deciso e il handoff dà per fatto**: `Guard = anti-Blast`. Il documento lo marca come
> «direzione proposta» e poi lo usa nella matrice. Resta proposta.

---

## 4. La tesi nuova, e sostenibile: `Dash` fase contro `Dash` azione

Il problema è reale e misurabile: `ERTMatchPhase::Dash` è una macro-fase, `Action.Dash` è lo scatto generico
del catalogo core (`LinearDash`, 3 celle, ricarica 1, slot `Movement`). Lo stesso token nomina il contenitore
e uno dei contenuti, e il costo si paga a ogni lettura del TurnLog — lo stesso criterio con cui
[D-070](../../decisions/RT_PDR_00_Decision_Log.md) rifiutò `Reposition` per il movimento post-Watch e scelse
`Withdraw`, e con cui [D-082](../../decisions/RT_PDR_00_Decision_Log.md) scelse `Bulkhead` invece di `Breach`.

**Ma il nome proposto va verificato prima di adottarlo, non dopo.** Vincoli misurati:

- `Action.Evade` **esiste già** nel catalogo, ed è una reazione. `Dodge` ed `Evade` come due entità distinte
  sono la stessa ambiguità spostata di un sinonimo — chi legge il TurnLog non le distinguerà.
- Le mobilità linearI del catalogo §2 sono già nominate per stile (`Charge`, `Leap`, `Reposition`,
  `PassingBlade`): la rinomina tocca **una** riga di catalogo, non la famiglia.
- `Action.Dash` è uno **Stable ID**. `D-134` ha stabilito che si cancella soltanto dopo aver **misurato**
  che nessuna traccia versionata lo contiene — e i golden replay vanno controllati prima, non dopo.
- ⛔ Il gate che sorvegliava i nomi legacy nella documentazione — `scripts/check-docs-naming.py` — **è uscito
  con `D-182`**. Una rinomina fatta oggi non ha nessuno che segnali le occorrenze rimaste.

Prima di decidere il nome serve una misura, non un'opinione: quante occorrenze di `Action.Dash` esistono in
`Source/`, `Content/`, `Scenarios/`, `docs/` e nel corpus golden. È lavoro di un'ora, ed è il preflight della
Decision Issue.

---

## 5. Domande che il handoff riapre, e che sono chiuse

| Domanda del handoff | Dove è già decisa |
|---|---|
| Economia a slot o a capacità numerica | `AE-1` ✅ chiusa da `D-114`: **restano gli slot**, il peso si paga in drawback |
| Il profilo di movimento cambia la legalità di un'azione | `AE-2` ✅ chiusa da `D-116`: **sì**, modello a soglia `MinStability`/`Stability` |
| Vietare il `Dash` a chi arma l'Overwatch | `D-070`: **non serve una regola** — lo slot movimento è già riservato a `Withdraw`, è una conseguenza |
| Eccezioni per eroe alla compatibilità | `AE-7`, aperta, col caso concreto già istruito |
| I fatti del percorso modificano l'azione | `AE-3`, aperta e **tenuta separata** da `AE-2` per determinismo |
| Numeri dello `Sneak` | `AE-5` |
| Rumore delle sei generiche | `AE-8` |

Delle dodici domande del §15 del handoff, **sette hanno già un ID**. Aprirle di nuovo produce voci che si
chiudono a vicenda senza saperlo — il difetto che `OPEN_DECISIONS.md` documenta per `FAC-12`.

Restano genuinamente aperte e **senza ID**: la portata base di uno scatto generico rinominato, la formula
del Move ridotto (che presuppone `#653`), e i profili di Overwatch autorizzati a reagire fuori dal Move.

---

## 6. Tracking: nessuna ladder nuova

La ladder che il handoff propone **esiste**, con le stesse release e gli stessi nomi, in
[`../roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) — e ha già le epic:

| Release | Epic reale | Gruppo `AE-*` del handoff | Dove va il lavoro |
|---|---|---|---|
| v0.2 — Struttura e finestre | **`E38`** ([#609](https://github.com/DegrassiAaron/refactor-tactics-main/issues/609)) | `AE-PHASE-v0.1` + `AE-COMPAT-v0.2` | CP 38.2 (validatore), 38.3 (compatibilità), 38.4 (preview), 38.5 (scenari) |
| v0.3 — Informazione | `E27`–`E29` · `E33` | `AE-INFO-v0.3` | eleggibilità reazioni ↔ percezione |
| v0.4 — Operations | `E30`–`E32` · `E34` · `E37` | `AE-MAP-v0.4` | transizioni multilivello |
| v0.5 — Online Foundation | **`E40`** | `AE-ONLINE-v0.5` | intent server-authoritative, canary di leak |
| v0.6 — Ability Runtime | **`E41`** | `AE-ABILITY-v0.6` | profili d'azione per personaggio |
| v0.7 — Competitive Alpha | **`E42`** | `AE-COMPETITIVE-v0.7` | dedicated, reconnect, replay |
| v0.8 — Beta / Balance | **`E43`** | `AE-BETA-v0.8` | batch, metriche, false choice |
| v0.9 — Release Candidate | **`E44`** | `AE-RC-v0.9` | freeze, regressione, accessibilità |
| v1.0 — Launch | **`E45`** | `AE-LAUNCH-v1.0` | gate di produzione — **non porta feature nuove** |

> ⛔ **`E40`–`E45` non si riusano per questo dominio**: sono la vista di *release* di tutto il progetto. Le
> issue dell'economia delle azioni si **agganciano** a loro; non le sostituiscono e non le rinominano.
>
> ⚠️ E38 dichiara per iscritto i propri cross-link: *«E14 possiede l'`Overwatch`, E16 il facing, E11 la HUD —
> questa epic non ne riapre nessuna»*. Il handoff scriveva lavoro di Overwatch dentro il gruppo di
> compatibilità: quel lavoro è **di E14**.

E38 è `P2` e non apre lavoro finché i gate della v0.1 non sono verdi. Le decisioni del §3, invece, si possono
prendere subito: una decisione scritta costa meno che ridiscuterla.

---

## 7. Scenari: due esistono, e uno dei due non gira

Il handoff propone dodici `PHASE-AE-*`. Il corpus usa `Spec.<Dominio>.<Caso>`, e ne ha già alcuni:

| Caso del handoff | Nel corpus |
|---|---|
| `PHASE-AE-002` Dodge riduce il Move finale | ✅ `Spec.Movement.DashDiscardsPlannedMove` — lo **azzera**, e lo misura |
| `PHASE-AE-003` OW ignora il Dash | ⏳ `Spec.Movement.AntiDashTriggerIgnoresMove` esiste ma è **`planned`**: richiede la capability `SemanticTrigger`, che non esiste, e dipende da `#307`. Ha `TurnsCompleted: 0` — non prova ancora niente |
| `PHASE-AE-006/007` Brace vs Dash | ✅/❌ `Scenarios/Spec/Brace/*` coprono il Brace **che c'è**, non quello del §3.2 |
| `PHASE-AE-012` permutazione | ⏳ dichiarato in CP 38.5, insieme ai cinque `Spec.ActionEconomy.*` `planned` |

**I due che mancano davvero**, e sono i più utili perché pinnano ciò che oggi è vero per costruzione:

1. `Spec.Overwatch.IgnoresDashPhaseTransition` — un bersaglio attraversa il cono armato **durante la fase
   Dash**: nessuna opportunity. Oggi passa senza scrivere una riga di produzione, ed è il punto.
2. `Spec.Overwatch.CatchesMovePhaseTransition` — lo stesso bersaglio, lo stesso cono, nella fase Move:
   opportunity generata. **Vale solo in coppia col primo**: da solo, il primo lo passerebbe anche un
   Overwatch rotto che non scatta mai.

È la stessa disciplina già scritta in `AntiDashTriggerIgnoresMove` (*«il valore sta nella coppia di turni, e
va letta insieme»*) — qui però eseguibile subito, perché non dipende da `SemanticTrigger`.

---

## 8. Nomi reali, per chi implementerà

Il handoff propone `ActionDefinition`, `ReactionDefinition`, `TriggerPhaseMask`, `ActionConflict.*`.
**Non si inventano**: gli equivalenti esistono.

| Concetto del handoff | Nome reale |
|---|---|
| `ActionDefinition` | `FRTActionDef` |
| `ResolvePhase` | `ERTResolutionPhase` + `URTCatalogLibrary::MapResolutionPhase` |
| `MovementKind` | `ERTMovementStyle` (`Budget · LinearDash · LinearCharge · LinearLeap · LinearPass`) |
| `ConsumesMain` | `ERTActionSlot` |
| validator del piano | `URTPlanValidationLibrary` — **esiste**, e il buco è il *consumatore*: in partita non la chiama nessuno |
| reason code | famiglie **esistenti**, valori **in coda**: `ERTMoveOutcome` · `ERTCombatOutcome` · `ERTFallbackOutcome` |

⛔ **Un enum di reason code parallelo è già stato respinto** una volta
([`../../gameplay/spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md) §6): CP 38.2 lo
mette per iscritto come DoD. `ERTMoveOutcome` porta già `SupersededByDash` e `StoppedByOverwatch` — i due
esiti che il handoff voleva nominare da zero.

⚠️ **Un enum aggiunto in coda non rompe la compilazione**, e nessun consumatore fa uno `switch` esaustivo:
`ERTStructureOp` lo documenta come trappola già scattata. Chi aggiunge un valore aggiunge anche il suo ramo.

---

## 9. Consegna

Le voci del §16 del handoff che restano, senza le sedici che chiedevano di consuntivare lavoro non svolto:

- `HEAD` prima e dopo, e la versione di Unreal **verificata sul repository** (non attesa: `RefactorTactics.uproject`);
- epic e issue **riusate**, con URL reali — non numeri inventati;
- conflitti trovati e come sono stati riconciliati;
- `D-nnn` assegnati, **riverificati prima del merge** contro le PR aperte;
- test eseguiti, con **output reale**: un conteggio copiato dalla roadmap non è una misura.

Non si dichiara «funziona», «completo» o «deterministico» senza evidenza, e non si dichiara `PASS` senza
output. Una modifica a queste regole è *Done* quando è data-driven invece che ramificata sull'`ActionId`,
quando il TurnLog spiega l'esito, quando ha uno scenario riproducibile e quando il replay sullo stesso
snapshot non diverge.

---

## 10. Il prossimo passo, uno solo

**La Decision Issue del §3** — «la capacità di reazione resta indipendente dagli altri due slot?» — con le
tre voci (Reaction, Brace, Guard) nella stessa issue, perché decise separatamente si contraddicono.

Tutto il resto o è già vero (§2), o è già tracciato (§5, §6), o dipende da quella risposta. I due scenari
dell'Overwatch (§7) sono l'unico lavoro che si può fare **prima** della decisione, e non la anticipano.
