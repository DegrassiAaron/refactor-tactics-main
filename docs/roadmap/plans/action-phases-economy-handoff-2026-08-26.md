# Fasi delle azioni, Dodge, Guard, Brace, Interact, Overwatch — handoff riconciliato

> `CURRENT` · **Creato**: 2026-08-26 · **Base misurata**: `c2bbfb7`
> **Sorgente esaminato**: `CLAUDE_ActionPhases_Dodge_Guard_Brace_Overwatch_Epics_v1.0_20260826.md`, archiviato in
> [`../../archive/src/handoff/2026-08-26-action-phases-dodge-guard-brace-overwatch.md`](../../archive/src/handoff/2026-08-26-action-phases-dodge-guard-brace-overwatch.md).
>
> **Cosa possiede**: il verdetto tesi per tesi del sorgente e le misure che lo sostengono. Da oggi è la
> **lettura corrente del dominio** — fasi delle azioni, Dash/Dodge, Guard, Brace, Interact, Overwatch,
> reaction economy — e supera il sorgente come documento di riferimento.
> **Cosa non possiede**: nessuna regola, e questa cartella non ha owner ([`README.md`](README.md)). Le regole
> restano di [`../../decisions/`](../../decisions/RT_PDR_00_Decision_Log.md),
> [`../../gameplay/spec-economia-del-turno.md`](../../gameplay/spec-economia-del-turno.md) e
> [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md); lo stato di E38 è di
> [`../roadmap-post-v0.1.md`](../roadmap-post-v0.1.md).
>
> ✅ **Il §3 è stato deciso il 2026-08-27**, in sessione socratica con l'autore:
> [D-204](../../decisions/RT_PDR_00_Decision_Log.md) (`Hold Ground` risponde a un evento) e
> [D-205](../../decisions/RT_PDR_00_Decision_Log.md) (la `Guard` è la difesa piantata), che **riaprono
> `BAL-1`**. Le regole vivono là, non qui: questa pagina conserva il verdetto sul sorgente e le misure.
>
> 🔴 **Due affermazioni della prima stesura erano false, ed erano mie.** Dicevo che il `Brace` «è mitigazione,
> non una reazione» e che il suo runtime non esisteva: `D-047` lo classifica come azione che **arma un
> profilo di reazione** dal 2026-08-09, ed è **implementato**. Avevo letto il testo di una decisione come una
> descrizione dello stato — due volte. Corretto nel §3.1, che tiene l'errore invece di cancellarlo.
>
> 🔁 **Perché esiste**: il sorgente prescriveva un audit e poi lo dava per svolto. Metà delle sue tesi
> descrive cose che il repository fa già, e la sua roadmap `AE-*` fino alla `v1.0` è **la seconda roadmap che
> il documento stesso vieta** al §9. Questo file è quel documento **dopo** l'audit.
>
> Ogni riga marcata *misurato* ha un file e una funzione dietro; ogni riga marcata *da decidere* non ne ha, e
> non se ne inventa una.
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
| 4.4 | Brace è il counter anti-Dash | 🔴 **la premessa era mia e sbagliata**: il `Brace` **è già una reazione**, per `D-047` e in codice. Il *payload* anti-Dash resta un'altra azione | §3 — chiuso da `D-204` |
| 4.5 | Guard nega Move e Reaction | ✅ **il sorgente aveva ragione sul movimento**: era il **catalogo** a essere invertito | §3 — `D-205` la pianta (`Slow`); la reazione è `BAL-4` |
| 4.6 | Interact consente Move, nega Reaction | ⚠️ metà vera: il Move è libero; la Reaction **non** è negata da nessuno | §3 — stessa forma di `BAL-4`, non ancora posta |
| 4.7 | La Reaction non è gratuita | ⚠️ **il meccanismo esiste già**: `bAllowsReaction` è un dato, applicato, e lo usa **una** azione (`Sprint`) | §3 — ridotta a `BAL-4`: un booleano, non una regola |
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

## 3. Le tesi in conflitto: decise il 2026-08-27

> 🔁 **Questa sezione è stata riscritta.** Nella prima stesura dichiarava tre conflitti *«da portare a una
> Decision Issue»*. Due di quei tre **non erano conflitti col repository**: erano conflitti con
> l'implementazione, che avevo descritto come se fosse la decisione. La sessione socratica del 2026-08-27 li
> ha sciolti e ha prodotto [D-204](../../decisions/RT_PDR_00_Decision_Log.md) e
> [D-205](../../decisions/RT_PDR_00_Decision_Log.md).

### 3.1 Il `Brace` era già una reazione — e l'errore era mio

Scrivevo: *«oggi `Action.Brace` è mitigazione danno + `Root`»*, e classificavo la tesi del sorgente come
conflitto con una decisione consolidata. **Falso in due modi.**

[D-047](../../decisions/RT_PDR_00_Decision_Log.md) (2026-08-09, `Consolidata`) dice:

> *«`Brace` prepara una reazione; **come** si reagisce lo dice il Reaction Profile del personaggio. La
> risposta universale è `Hold Ground`.»*

E non è solo scritta: **è implementata**. `Reactions.Brace.ProfileDecidesInPlay`,
`...SidestepLeavesThePushLine`, `...DecisionRoundTripsThroughTrace`, `...BotAnswerIsLegal` girano oggi, e
[D-132](../../decisions/RT_PDR_00_Decision_Log.md) ha già assegnato i tre profili d'eroe.

Avevo letto la nota di D-047 *«nessun runtime prima di E13»* come se descrivesse il presente — lo stesso
errore, due volte nella stessa sezione: **prendere il testo di una decisione per una descrizione dello
stato**. È esattamente ciò contro cui questa cartella esiste.

Il payload *anti-Dash* che il sorgente propone resta invece un'altra cosa, e resta non adottato: `Reaction.Anchor`
ha già il trigger `AboutToBeDisplaced` e lo scenario `Spec.Reaction.AnchorCancelsPush`, e due entità che
annullano la stessa spinta si pagano a ogni lettura del TurnLog — il criterio di `D-070` e `D-082`.

### 3.2 Cosa è stato deciso

**[D-204] — `Hold Ground` risponde a un evento, non copre il turno.** Cade una sola clausola di D-047,
quella che diceva *«nessun numero di bilanciamento cambia»*: la risposta universale mitiga **il colpo a cui
risponde** e blocca **quella** spinta. La ragione non è il numero ma la categoria — una reazione che risponde
a un evento e protegge per l'intero turno è uno **stato travestito da risposta**.

**[D-205] — la `Guard` è la difesa piantata**: mitigazione sostenuta, `Status.Slow` auto-inflitto. Supera
[D-121](../../decisions/RT_PDR_00_Decision_Log.md), che aveva chiuso `BAL-1` come *status quo*.

> **Perché la supersessione è legittima, e non un ripensamento.** D-047 e D-121 sono state prese
> **nell'ordine sbagliato l'una rispetto all'altra**: D-121 ha fissato la divisione del lavoro fra le due
> difese *come se fossero due azioni dello stesso tipo*, tre giorni dopo che D-047 aveva cambiato **che cosa
> è** il `Brace`. Quando il runtime del profilo è atterrato, lo «status quo» che D-121 conferma ha smesso di
> descrivere qualcosa che esiste — e qualcuno deve restare il difensore piantato.

Sul movimento della Guardia **il sorgente aveva ragione e il catalogo torto**: `Move + Guard + reazione
d'eroe` era legale, ed è la combinazione che il sorgente chiamava inaccettabile. La prima stesura di questo
referto diceva che il sorgente aveva *«Guard e Brace scambiati»* — misurato contro il catalogo era esatto, ma
attribuiva l'errore alla parte sbagliata.

### 3.3 Cosa resta aperto, e non è stato dedotto di passaggio

Tre domande, registrate in [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) invece che risolte di passaggio.
Una è stata chiusa lo stesso giorno:

| ID | Domanda | Stato |
|---|---|---|
| ~~`BAL-2`~~ | La Guardia piantata copre a 360° o resta frontale? | ✅ **chiusa da [D-206](../../decisions/RT_PDR_00_Decision_Log.md)**: resta **frontale** (120°, 3 direzioni su 6), e il controllo diventa **per-colpo** |
| `BAL-3` | Con quali numeri, e quanto dura `Hold Ground`? | **aperta** — `balance/README.md` e `D-023` vietano di correggerli a tavolino |
| ~~`BAL-4`~~ | La Guardia piantata nega anche la reazione? | ✅ **chiusa da [D-207](../../decisions/RT_PDR_00_Decision_Log.md)**: **sì** — assorbire o rispondere, mai entrambi |
| ~~`BAL-5`~~ | E le altre tre azioni di `Prep` (`Brace`, `Shield`, `Overwatch`)? | ✅ **chiusa da [D-208](../../decisions/RT_PDR_00_Decision_Log.md)**, con **tre risposte diverse**: `Brace` **no**, `Overwatch` e `Shield` **sì** |
| `BAL-1` | *(riformulata)* La differenza fra le due difese è leggibile? | **aperta** — `Guard` e `Brace` hanno ora lo **stesso prezzo**: la scelta poggia solo sulla forma della mitigazione |

`BAL-1` è **riaperta**, e `#403` / `U20 · PIE-BAL1` non cade: si **ripunta**, perché verificava la
leggibilità di una differenza che ora è diversa.

> **Perché `BAL-2` si è potuta chiudere subito, e le altre due no.** Non era una domanda di bilanciamento
> travestita: la geometria difensiva del gioco è **una sola** — `IsInFrontalArc` per la guardia ed
> `EffectiveCoverReduction` per la copertura implementano la stessa regola CP 16.2 — e una Guardia a 360° ne
> avrebbe creata una seconda. La risposta stava nella struttura, non in un numero.
>
> ➕ **Le eccezioni per eroe sono ammesse, e nel kit** (`D-014`/`D-028`), con la forma già in uso di
> `URTHeroData::ReactionProfileId`. Ma devono passare il test di `D-132` — *«non era contenuto, era un
> nome»*: un eroe che «para da ogni lato» senza portare altro sarebbe esattamente ciò che quella voce ha
> rifiutato a Riktor. Nessun kit ne dichiara una oggi, e il campo si scrive quando un kit lo chiede.

> ⚠️ **`BAL-4` si è chiusa contro la finzione, e la voce lo dichiara.** L'unica azione che nega la reazione
> oggi è lo `Sprint`, e la nega perché *«chi corre a perdifiato non para»* — mentre *piantato e pronto* è la
> definizione di pronto. `D-207` toglie dal gioco `Guard + Counter`, il turno difensivo più evocativo che il
> gioco sappia esprimere, e lo fa perché `D-205` ha dato alla Guardia una protezione **che dura**: una
> protezione che dura senza rinunciare a niente di reattivo non è una scelta. L'argomento contrario è
> registrato nel Decision Log invece che scartato, e `U20`/`PIE-BAL1` è dove si misura se aveva ragione.
>
> 🔴 **Dove la sessione è arrivata, e non era il punto di partenza.** Chiuse `BAL-2`, `BAL-4` e `BAL-5`, la
> `Guard` e il `Brace` hanno **lo stesso identico prezzo**: azione principale, `Status.Slow`, niente slot
> reazione. Nessun **costo** le separa più — le separa solo la **forma** di ciò che danno: la Guardia
> assorbe in modo **sostenuto** dentro un arco **frontale** di 120°, il `Brace` risponde **a un evento** da
> **ogni lato**, con un profilo d'eroe sopra.
>
> È una differenza di **genere**, non di numeri, e questo riformula `BAL-1` alla radice: la domanda non è più
> *«si separano abbastanza?»* ma **«la differenza di genere è leggibile senza un costo che la annunci?»** —
> che è una domanda per `U20` / `PIE-BAL1`, non per un documento.

> 🔴 **Misurato il 2026-08-27, e rivede al rialzo il costo di `D-206`.** Il *«controllo direzionale
> per-colpo»* non è un ramo da spostare: **il dato non c'è dove serve.**
>
> ```cpp
> struct FRTAttack { int32 TargetIndex = INDEX_NONE; int32 Power = 0; };
> ```
>
> `FRTAttack` **non porta l'attaccante**, e le due funzioni che applicano le mitigazioni —
> `ApplyFirstHitDelta` (gate «una volta sola») e `ApplyDamageDelta` (ogni colpo) — prendono un delta
> **per bersaglio**, non per colpo. Oggi la direzione si valuta a monte in `RTTurnManager`, dove
> `Plan.Hits` porta `AttackerId`, e viene **collassata in un booleano per bersaglio** prima della
> chiamata: è quel collasso che `D-206` scioglie, non un `if`.
>
> Le due vie, entrambe più care di un ramo: portare l'attaccante dentro `FRTAttack` — una `USTRUCT`
> esposta a Blueprint che attraversa il resolver — oppure una **terza** funzione con delta parallelo ai
> colpi invece che ai bersagli. La seconda evita di toccare una struct serializzata, ed è la sola che
> non chiede di rimisurare cosa legge `FRTAttack`.
>
> ✅ **La buona notizia, dallo stesso giro**: `D-204` e `D-205` sono per il resto **uno scambio di
> chiamata**. Le due funzioni esistono, sono documentate come *«non intercambiabili»* proprio per quel
> gate, e le due difese si scambiano di posto fra loro — `Guard` passa a `ApplyDamageDelta`, il `Brace`
> a `ApplyFirstHitDelta`. ⛔ **Ma non prima di `BAL-3`**: fatto senza rinumerare dà la `Guard` a −15 su
> **ogni** colpo e il `Brace` a −10 su **uno**, cioè una coppia peggiore di quella che sostituisce.

> 🔴 **Due costi che nessuna delle tre voci può nascondere.** Il valore di `bAllowsReaction` lo assegna oggi
> un `if` sull'ActionId dentro `ShippedAction` — un secondo utente lo rende un **parametro**, non allunga il
> predicato. E `ResolvePrep` **non legge** il flag: senza quel ramo la decisione sarebbe dichiarata e mai
> applicata, che è peggio di non averla presa.

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
`Source/`, `Content/`, `Scenarios/` e `docs/`, e quante nel corpus golden. È lavoro di un'ora, ed è il
preflight della decisione sul nome — che resta **non presa**.

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

## 10. Il prossimo passo

Il §3 è deciso (`D-204`…`D-208`) e **nessuna riga di codice è stata scritta**: le cinque voci
cambiano il comportamento di gioco, e l'ordine in cui si eseguono non è indifferente.

1. **Il controllo direzionale per-colpo** (`D-206`), che è il prerequisito di `D-205` e non un dettaglio:
   il gate di oggi guarda *«il PRIMO dell'array»* perché la Guardia era front-loaded, e su una difesa
   sostenuta quel criterio lascerebbe la direzione di un colpo arbitrario a decidere la protezione contro
   tutti gli altri. ⏳ Serve **uno scenario nuovo** che oggi non esiste e prima di `D-205` non poteva
   esistere: due attaccanti nello stesso turno, uno nell'arco e uno alle spalle, guardia che regge sul primo
   e viene scavalcata sul secondo.
2. **Una sola rigenerazione del corpus golden**, per D-204 e D-205 insieme — è la disciplina che
   [D-196](../../decisions/RT_PDR_00_Decision_Log.md) ha appena imposto per tre cambi d'identità decisi
   insieme, e due voci che cambiano numeri di danno ricadono nella stessa regola.
3. **`#403` / `U20 · PIE-BAL1` si ripunta**, non si chiude: verificava la leggibilità di una differenza che
   ora è un'altra.

> ✅ **Un rischio sollevato in sessione e poi misurato, invece di restare un timore.** `RTReactionLibrary`
> classifica `Status.Root` e `Status.Slow` come stati **di controllo**, cioè quelli su cui scatta
> `AboutToReceiveControl` (`Reaction.Cleanse`). Poiché `D-205` e `D-208` fanno auto-infliggere `Slow` a
> `Guard` e `Brace`, sembrava che una difesa potesse aprire una finestra di reazione **contro sé stessa**.
>
> Non succede, e non per una guardia: `PassPointFor(AboutToReceiveControl)` vale `BlastStatus`, e quel pass
> gira **solo** dentro `RTTurnManager_Blast.cpp`. `Guard` e `Brace` risolvono in `Preparation`, che è
> un'altra funzione e un'altra fase — lo stato auto-inflitto non attraversa mai quel pass. L'esclusione è
> **strutturale**, quindi resta vera finché le due fasi restano due; chi un giorno le unificasse la perde in
> silenzio.
>
> ⚠️ **Resta però un uso nuovo**: oggi `Status.Slow` è uno stato **inflitto** (`Hero.Riktor.ImpactShot` lo
> mette addosso ai nemici), e nessuna azione se lo dà da sola. Chi implementa `D-205`/`D-208` è il primo a
> farlo, e vale la pena guardare gli altri consumatori dello stato prima di assumerlo innocuo.

I due scenari dell'Overwatch (§7) restano l'unico lavoro indipendente da tutto questo, e si possono fare
subito: pinnano ciò che oggi è vero **per costruzione**, quindi passano senza toccare produzione.
