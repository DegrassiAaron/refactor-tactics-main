# La specifica consolidata della skill bar (2026-09-13) — spec panel sul documento come sorgente d'autore

> `CURRENT` · **Stato**: revisione chiusa. Le decisioni che ne discendono sono `D-407` … `D-415`. Del codice
> è migrata **una sola** delle nove — `D-412`, §10-bis — e §10 dice perché le altre no.
> ✅ **Compile e Tests `PASS`** su `HEAD e537b4aa`: 657 test, 657 `Success`, 0 `Fail` — §12.
> **Data**: 2026-09-13
> **HEAD della revisione**: `origin/main` = `a2d23509`. Le citazioni `file:riga` sono state **rimisurate**
> su questo commit — vedi la nota di metodo in §1 — in un worktree isolato (`spec/skill-bar-2026-09-13`),
> perché il clone principale era occupato da un'altra sessione su
> [#1410](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1410).
> **Oggetto**: `RefactorTactics-Specifica-Skill-Bar-2026-09-13.md`, consegnato dall'autore in `docs/` e
> **consumato** da questa revisione. Il documento sorgente è stato rimosso: il suo contenuto vive ora nelle
> voci di registro di §8, nei documenti owner di §9 e nelle issue di §11.
> **Panel**: Wiegers (lead) · Adzic · Cockburn · Fowler · Nygard · Crispin
> **Modo**: critique · **Focus**: requirements, architecture, testing

---

## 1. Il verdetto in una riga

**Non è un consolidamento: è una riprogettazione dell'economia dell'azione della v0.1**, e il documento
sorgente lo dichiara esso stesso senza saperlo — §15 punto 3: *«Confrontare questa specifica con gli owner
vivi nel repository e le issue esistenti… Questo confronto completo **non è stato eseguito** in questa
consegna.»*

Questa revisione è quel confronto. Su **237** affermazioni normative estratte dalle sezioni 2–12:

| Esito | Voci | Che cosa significa |
|---|---:|---|
| `ALREADY` | **111** | il repository dice già la stessa cosa — la specifica ratifica |
| `CONFLICT` | **72** | il repository dice qualcosa di **diverso**, e quasi sempre in una decisione accettata |
| `NEW` | **45** | nessuna fonte corrente si esprime — si può scrivere senza superare nulla |
| `OPEN` | **9** | la specifica stessa la marca `DOMANDA` / `PROPOSTA` non approvata |

Misurato su 43 documenti owner, 124 siti di codice e 178 test citati.

> 🔴 **Una nota di metodo, perché la misura è stata rifatta e sarebbe stato più comodo tacerlo.** La
> ricognizione è girata nel **clone principale**, che al momento portava il lavoro **non mergiato** di
> [#1410](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1410) e
> [#641](https://github.com/DegrassiAaron/refactor-tactics-main/issues/641): `ReservesMovementProfileId`,
> `Action.Withdraw` e lo `Sprint` già a `NormalMovement`, che su `a2d23509` **non esistono**. ∴ i suoi
> numeri di riga erano di un altro albero. Ogni citazione `file:riga` di questo referto e delle pagine che
> ne discendono è stata **rimisurata** con `git show origin/main:<file>`, e diciassette sono state corrette
> — `Action.Interact` da `:1209` a `:1181`, `Action.Dodge` da `:1264` a `:1233`, `Action.Brace` da `:1536`
> a `:1505`, e così le altre. ⚠️ **I FATTI reggono tutti**: a spostarsi erano le righe, non ciò che c'è
> scritto — ed è la differenza fra una misura sbagliata e una misura fatta sull'albero sbagliato.

🔴 **Il rapporto che conta non è 72 su 237: è che i 72 non sono lacune, sono decisioni pagate.** Fra le voci
di registro che cadrebbero: [`D-028`](../../decisions/RT_PDR_00_Decision_Log.md) ·
[`D-045`](../../decisions/RT_PDR_00_Decision_Log.md) · [`D-090`](../../decisions/RT_PDR_00_Decision_Log.md) ·
[`D-116`](../../decisions/RT_PDR_00_Decision_Log.md) · [`D-149`](../../decisions/RT_PDR_00_Decision_Log.md) ·
[`D-169`](../../decisions/RT_PDR_00_Decision_Log.md) · [`D-191`](../../decisions/RT_PDR_00_Decision_Log.md) ·
[`D-200`](../../decisions/RT_PDR_00_Decision_Log.md) · [`D-209`](../../decisions/RT_PDR_00_Decision_Log.md) ·
[`D-292`](../../decisions/RT_PDR_00_Decision_Log.md) · [`D-324`](../../decisions/RT_PDR_00_Decision_Log.md) ·
[`D-365`](../../decisions/RT_PDR_00_Decision_Log.md) · [`D-367`](../../decisions/RT_PDR_00_Decision_Log.md) ·
[`D-380`](../../decisions/RT_PDR_00_Decision_Log.md) · [`D-381`](../../decisions/RT_PDR_00_Decision_Log.md).
Diverse di esse **esistono perché una misura le ha prodotte**, non perché sembravano giuste.

## 2. La premessa che scioglie un terzo dei conflitti: lo stordimento non esiste

**Misurato**: `grep -rniE '\bstun|stordi' Source/RefactorTactics/` → **1** occorrenza, ed è un sottotesto
dentro un'altra parola.

Non è una lacuna d'implementazione: è [`D-169`](../../decisions/RT_PDR_00_Decision_Log.md) —
*«`Stun` e `Disarm` **escono dalla v0.1** perché non esistono»* — che per questo motivo riscrisse la prima
riga di DoD del CP 14.6.

La specifica usa lo stordimento come strumento portante in **sei** sezioni: §4 (simultaneità), §6 (decadenza
della Guardia), §7 (Schivata disabilitata in Planning), §8 (Reflect), §9 (Overwatch interrotto), §11
(Interact impedito). ∴ **quelle regole non sono false: sono senza soggetto.** Vanno riscritte sugli stati di
controllo che il gioco ha davvero — `Unbalanced`, `Prone`, `Root`, `Slow` — oppure la v0.1 riapre `Stun`,
che è una decisione a sé e non un dettaglio di queste pagine.

⚠️ **È la correzione più economica dell'intero referto**: non chiede di scegliere fra due design, chiede di
nominare il soggetto giusto.

## 3. I sette conflitti strutturali

Ordinati per costo di migrazione decrescente. Ciascuno è stato **verificato in prima persona** dopo la
segnalazione: un conflitto asserito è un'ipotesi finché non se ne leggono le due formulazioni.

### 3.1 🔴 La Guardia: pool contro riduzione per colpo

| | |
|---|---|
| **Spec §6** | *«Mitigazione: riduzione **fissa per colpo**, valore diverso secondo il personaggio»* · *«Numero di colpi: **tutti** i colpi validi durante la durata»* |
| **Repo** | [`D-292`](../../decisions/RT_PDR_00_Decision_Log.md): *«La `Guard` smette di essere "-15 al primo colpo" e diventa un **POOL di 15 danni assorbibili**»* — `RTCombatResolver.cpp:145-161`, valore unico `URTCombatLibrary::GuardFirstHitReduction = 15` (`RTCombatLibrary.h:184`) |

🔑 **`D-292` non è un'opinione di bilanciamento: è la chiusura di un difetto misurato.** Il test
`Combat.NegativeFirstHitDeltaIsPermutationInvariant` era **rosso**, e il difetto era che con un delta
negativo più grande del colpo che lo riceveva *«la riduzione che avanza si perde — e quanta se ne perda
dipende da quale colpo è arrivato prima»*. A scegliere era l'indice dell'attaccante.

✅ **La buona notizia**: la riduzione per colpo **è** permutation-invariant, quindi non riapre quel difetto.
Ciò che cambia è il tetto — un pool limita la mitigazione totale, una riduzione per colpo no — e con esso il
prezzo della Guardia contro sequenze di colpi piccoli, che la spec §13.8 riconosce e dichiara voluto.

⛔ **Costo**: la pinnano i test qui sotto, in `RTCombatResolverTests.cpp` · `RTCoreActionTests.cpp` · `RTDamageBreakdownTests.cpp` · `RTFacingDefenseTests.cpp` · `RTHexCombatIntegrationTests.cpp`. `Combat.GuardPoolIsPermutationInvariant` ·
`GuardPoolRemainderIsNotWasted` · `GuardPoolIsNotConsumedFromBehind` · `SingleHitAgainstGuardIsUnchanged` ·
`DeflectPoolAbsorbsBeforeGuardPool` · `GuardAndDeflectAbsorbInDeclaredOrder` · `AreaGuardUsesImpactCenter` ·
`AreaGuardIsBypassedWhenImpactCenterIsBehind` · `BackAttackIgnoresGuard` · `Actions.Guard.FirstHitOnly` ·
`Actions.Guard.ReducesFirstDirectDamageInMatch` · `Actions.Guard.ResistsSinglePush`. E
[`D-309`](../../decisions/RT_PDR_00_Decision_Log.md) ha esteso **la stessa forma** a `Deflect`: cambiare solo
la Guardia lascia due modelli difensivi diversi dove oggi ce n'è uno.

⚠️ *L'elenco è la misura di questo passaggio, non un totale: si conta con*
`grep -rlE "GuardPool|Guard\\.(FirstHitOnly|Resists|Reduces)" Source/RefactorTactics/Tests/`.

⚠️ **E due sotto-voci della stessa sezione vanno nel verso opposto al repository, con un test ciascuna.**
*«Un'esplosione centrata sull'esagono del difensore supera Guardia»*: il codice dice l'opposto e lo motiva
(`RTHexCombatLibrary.cpp:202`, test `Combat.AreaGuardUsesImpactCenter`). *«Se il danno scende a zero
anche gli effetti associati sono bloccati»*: oggi gli effetti collaterali **non** dipendono dal danno passato
(`RTTurnManager.cpp:5607`).

### 3.2 🔴 La Schivata: azione principale contro slot movimento

| | |
|---|---|
| **Spec §7** | *«Occupa l'**azione principale**»* + *«**Consente Withdraw** nella fase Move»* |
| **Repo** | `Action.Dodge` è `ERTActionSlot::Movement` — *«chi scatta si è mosso per questo turno e **non prosegue col Move**, ma l'azione principale gli resta — schivo e sparo»* (`RTCatalogLibrary.cpp:1233`) |

🔴 **Ciò che cade non è una riga, è una scelta di turno.** [`D-028`](../../decisions/RT_PDR_00_Decision_Log.md)
diede allo scatto lo slot movimento **apposta**, e `AE-7` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) lo
riformula: *«lo scatto occupa lo slot movimento, e quella impossibilità **è** la scelta fra schivo e sparo e
sparo e muovo»*. Con la Schivata sull'azione principale, *schivo e sparo* smette di esistere per tutti.

⚠️ [`D-191`](../../decisions/RT_PDR_00_Decision_Log.md) lo ribadisce in generale: *«Una mobilità rapida
occupa il MOVIMENTO. Il danno non conta, lo stile non conta»*. La Schivata della specifica sarebbe la prima
eccezione.

🔑 **Possibile scioglimento, e va deciso invece che dedotto**: la «Schivata» della specifica è l'abilità
difensiva **caratteristica** di un personaggio, non la generica `Action.Dodge` che ogni eroe ha come quarta
abilità. Se è così è una **collisione di nome** — la stessa forma di
[`D-230`](../../decisions/RT_PDR_00_Decision_Log.md) (`Dash` → `Dodge`) e di
[`D-082`](../../decisions/RT_PDR_00_Decision_Log.md) (`Bulkhead`) — e si risolve battezzando la nuova, non
riscrivendo la vecchia.

⛔ **Un dettaglio smentisce la spec a prescindere da come si scioglie**: *«la versione standard è
silenziosa»*, ma `Action.Dodge` porta **rumore 6**, il massimo della scala delle azioni
([`RT_ActionCatalog_v0.1.md`](../../balance/RT_ActionCatalog_v0.1.md):290).

### 3.3 🔴 `Brace` è già un'azione, non uno slot

La specifica usa `Brace` come **nome dello slot/famiglia** della difesa caratteristica, e lo dichiara:
*«non una riduzione di danno universale»*. Nel repository `Action.Brace` **è** esattamente una riduzione di
danno universale — una delle sette generiche di [`D-025`](../../decisions/RT_PDR_00_Decision_Log.md):
`Braced` (−10 a ogni colpo, **da ogni lato**) più `Root` (`RTCatalogLibrary.cpp:1505` su `origin/main`).

E `ERTActionSlot` non ha un valore `Brace`: è `{ None, Movement, Main, MovementAndMain, Reaction }`
(`RTActionDef.h:71-88`).

⛔ **`Root` contraddice direttamente la regola di §2** per cui le difese di quello slot consentono
`Withdraw`: `Status.Root` blocca il movimento volontario. Chi si irrigidisce **non ripiega**.

### 3.4 🔴 L'Overwatch: una richiesta per nemico contro una per micro-step

| | |
|---|---|
| **Spec §9** | *«Una sola richiesta per ciascun nemico durante il turno. Dopo HOLD, quel nemico non viene riproposto nello stesso turno»* |
| **Repo** | `RefactorTactics.Overwatch.TriggersPerMicroStep` — l'occasione nasce **a ogni micro-step**, e un `HOLD` perde l'*opportunity*, non la *reaction* |

E la geometria non regge: *«ampiezza e portata dipendono dal personaggio»*, ma
[`D-169`](../../decisions/RT_PDR_00_Decision_Log.md) dichiara testualmente che **la zona non ha ampiezza** —
è una **linea larga una cella**. *«Decade a uno spostamento forzato»*: `D-169` dice l'opposto con parole sue
— *«movimento forzato: **RILOCALIZZA**, non invalida»*, il watcher si ricostruisce (ed è
[#2367](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2367)). *«La rotazione sul posto ruota
il settore»*: il facing letto è quello **dichiarato all'armamento**, non quello corrente
(`RTTurnManager.h:123`), che è [`D-367`](../../decisions/RT_PDR_00_Decision_Log.md).

✅ **Ciò che invece regge, alla lettera**: l'Overwatch riserva lo slot movimento al solo `Withdraw`
([`D-070`](../../decisions/RT_PDR_00_Decision_Log.md)), e `Timeout → HOLD`, mai `FIRE`
([`spec-sequenza-turno.md`](../../gameplay/spec-sequenza-turno.md):163).

### 3.5 🔴 Interact: fase `Move` contro fase `Blast`

La specifica lo colloca *«**sempre in Move**, mai in Prep»*. Il repository lo risolve nel **Blast**, e lo
dice in tre sedi che concordano: `RTCatalogLibrary.cpp:1181` (`ERTResolutionPhase::Attack`),
[`RT_ActionCatalog_v0.1.md`](../../balance/RT_ActionCatalog_v0.1.md):122, e
[`spec-interazioni-mappa-cp101.md`](../../gameplay/spec-interazioni-mappa-cp101.md):181 — *«`Action.Interact`
è **Blast**»*.

🔑 **La collocazione ha una ragione architetturale, e non è inerzia**: la topologia muta nel Blast **perché
il Move dello stesso turno la veda**. ⚠️ **E il test che lo pinnerebbe non esiste**: `Interaction.TopologyChangesInBlast` è un nome dichiarato nel DoD di [`spec-interazioni-mappa-cp101.md`](../../gameplay/spec-interazioni-mappa-cp101.md), e `grep -rn` in `Source/` risponde **0** —
*«la mutazione avviene nel Blast e la revisione si incrementa **prima** del Move»*. Spostare Interact nel Move
significa decidere che cosa vede chi cammina attraverso una porta aperta nello stesso turno.

⚠️ **La specifica contrappone Move a *Prep*, non a Blast** (§13.1: *«Interact prima del movimento in Prep»* →
*«entrambe le sequenze in Move»*). ∴ è verosimile che il Blast non fosse sul tavolo quando la regola è stata
scritta: il conflitto potrebbe essere **apparente**, e la domanda vera è solo *«prima o dopo il proprio
percorso»* — che il Blast di oggi non impedisce.

⛔ **E l'adiacenza è invertita**: la spec dice *«normalmente richiede la **stessa cella**, i bordi sono
l'eccezione»*; [`D-149`](../../decisions/RT_PDR_00_Decision_Log.md) dice che la portata è `1` e
**l'adiacenza è la norma** — *«il giocatore punta la sorgente, che è adiacente»*.

### 3.6 🟡 Il movimento: moltiplicatori, cadenza, primo passo

Tre conflitti distinti in una sezione sola, e non si migrano insieme.

**(a) Budget.** Spec §10.1: moltiplicatori del budget base — `Withdraw` ×0,25 · `Sneak` ×0,5 · `Move` ×1 ·
`Sprint` ×2. Repo: valori **assoluti** per profilo — `Move` eredita da `ARTUnit::MoveRange`, `Sprint` 8,
`Withdraw` 2, `Sneak` non definito (`RTMovementProfileLibrary.cpp:39-88`). Con base 5 i moltiplicatori danno
`Sprint` 10 e `Withdraw` 1: **non coincidono** con 8 e 2.
✅ La forma però è già pronta: `FRTMovementProfile::InheritFromUnit` è esattamente «il budget lo dichiara
l'unità», cioè il posto dove un moltiplicatore atterra senza inventare un campo.

**(b) Cadenza.** Spec: *«Sprint fino a 2 passi per tick»* e *«il terreno aumenta il costo, non rallenta la
cadenza»*. Repo: `MaxGraphTransitionsPerUnitPerMicroStep = 1`, *«vale per **tutti** i profili di Move»*
([`spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md):98-104); e
[`D-381`](../../decisions/RT_PDR_00_Decision_Log.md) ha deciso il contrario del secondo punto — *«la durata
di un attraversamento è un dato proprio del passo»*, cioè il terreno **cambia** la cadenza.
🔑 Il primo dei due può essere apparente: la spec dice *«risolti **separatamente**»*, compatibile con «1 arco
per micro-step, 2 micro-step per tick». Il secondo no: è una decisione contro una decisione.

**(c) Primo passo garantito.** *«Tutte le abilità di movimento garantiscono un primo passo… anche quando il
costo supera il budget»*. Nessuna eccezione al budget esiste oggi, e
[`spec-pathfinding.md`](../../technical/architecture/spec-pathfinding.md):36 la esclude. È **`NEW`**
sostanziale, col rischio che la spec stessa nomina in §13.5: rende un `Withdraw` corto molto efficiente su
terreno costoso.

### 3.7 🟡 Il consumo al piano definitivo contro «la portata decide»

Spec §3.2: *«Quando termina la pianificazione e il piano diventa definitivo, le skill selezionate consumano
il cooldown»*, e resta consumato **comunque**.

Repo [`D-200`](../../decisions/RT_PDR_00_Decision_Log.md): *«La **portata** decide se un'azione parte. Tutto
il resto è un esito, e un esito si paga»* — fuori portata **non** paga, perché è geometria nota in
pianificazione. E [`D-209`](../../decisions/RT_PDR_00_Decision_Log.md): il cooldown si scrive in
**risoluzione**, non al commit; un'azione annullata da `Action.Interrupt` **non** paga
(`RTTurnManager_Blast.cpp:1337`).

⚠️ Le due regole coincidono nel caso comune e divergono in tre: l'azione interrotta, l'azione fuori portata,
e il momento in cui il numero si scrive — che è un fatto di replay e digest, non di bilanciamento.

⛔ **Due voci della stessa sezione hanno perso il soggetto**: *«anche energia … è consumata al piano
definitivo»* — [`D-324`](../../decisions/RT_PDR_00_Decision_Log.md) ha tolto `Energy` dal gameplay; e
*«un CD di N indica N turni successivi»* è la traduzione letterale che
[`D-090`](../../decisions/RT_PDR_00_Decision_Log.md) ha **misurato e scartato**.

## 4. I conflitti dell'attacco base, che meritano una riga a parte

Tre affermazioni di §5 contraddicono catene di decisioni **con un prezzo già pagato**.

| Spec §5 | Repository |
|---|---|
| *«non dipende dall'equipaggiamento»* | `URTCatalogLibrary::EquipWeaponVariant(Abilities[0], Piece)` — `RTUnit.cpp:1661`. `Abilities[0]` **è** l'attacco base (ADR-0007 §6). Le varianti arma sono `D-086`…`D-100` |
| *«la mira è fissata in pianificazione; non insegue il bersaglio»* | `const FRTCellId AimCell = bTargetsUnit ? Units[Intent.TargetId].Cell : Intent.TargetCell;` — `RTHexCombatLibrary.cpp:352`. `Units` è lo stato **al Blast**: con bersaglio-unità la mira **insegue** |
| *«si può sparare alla cieca verso un esagono non visibile»* | `ERTLineOfSightPolicy::Required` è il default (`RTActionDef.h:787`). Il tiro indiretto è una **licenza dichiarata**, e [`D-380`](../../decisions/RT_PDR_00_Decision_Log.md) l'ha **pagata**: `Action.Mortar` scende a 12 danni da 18 e sale a ricarica 3 da 2 |

🔴 **Il terzo è il più caro**: la specifica concederebbe gratis, a quattro attacchi base senza costo né
cooldown, ciò per cui una singola abilità d'eroe ha pagato un terzo del proprio danno.

## 5. La parola «rivela» ha già tre significati, e la specifica ne aggiunge un quarto

Segnalato dal panel come rischio di privacy, non come refuso.

| Senso | Dove |
|---|---|
| il colpo a segno promuove la **vittima** a contatto per la squadra dell'attaccante | [`D-380`](../../decisions/RT_PDR_00_Decision_Log.md) |
| `Status.Reveal` — l'**intento** diventa visibile all'avversario | invariante #6, `RTGameplayTags.h:8` |
| `Observe` — la **conoscenza di squadra** si aggiorna sulla linea visiva | `RTTeamKnowledge.cpp:6` |
| *(nuovo)* l'**attaccante** diventa noto ai nemici per il fatto di aver agito | spec §5, §7, §8, §9, §11 |

⚠️ Quattro sensi sulla stessa parola sono il modo in cui una regola di privacy si rompe in silenzio.
`CLAUDE.md` §7 chiede una revisione esplicita del boundary per qualunque modifica alla projection degli
eventi: il quarto senso ne è una, e va **battezzato** prima di essere implementato.

## 6. Ciò che la specifica chiude davvero

Non tutto è conflitto. Tre risultati netti, e sono i più immediatamente spendibili.

### 6.1 ✅ `AE-5` si chiude — il profilo `Sneak` ha i suoi numeri

La domanda era: *«Con quali numeri esiste il profilo `Sneak`? Costo, portata e rumore non sono definiti da
nessuna fonte corrente»*. La specifica risponde a tutte e tre: budget **×0,5**, cadenza **1 passo ogni 2
tick**, **sempre silenzioso** indipendentemente dal terreno.

∴ `FRTMovementProfile::bPlannable` di `Sneak` può diventare `true` (`RTMovementProfileLibrary.cpp:68-69`),
che è la sola ragione per cui oggi il profilo esiste nel catalogo senza essere scegliibile.

### 6.2 ✅ `spec-attacco-base-per-eroe.md` esisteva come promessa e ora ha un contenuto

L'intestazione di [`ADR-0007`](../../decisions/adr-0007-attacco-base-per-eroe.md) dichiara
**«Owner spec: da creare (`../gameplay/spec-attacco-base-per-eroe.md`)»**, e il file non esisteva. La sezione
5 della specifica è precisamente quel documento: è la sede naturale, e riempie una lacuna dichiarata invece
di aprire una sede nuova.

### 6.3 ✅ Centoundici affermazioni ratificano il canone

Fra le più significative: l'ordine `Prep → Dash → Blast → Move`; nessuna gerarchia `Instant/Fast`
([`D-293`](../../decisions/RT_PDR_00_Decision_Log.md) ritira APNAP); l'attacco base proprietà dell'eroe
(ADR-0007); il settore di 120° come `HexCone`; il fuoco amico attivo di default (`bFriendlyFire = true`);
l'assenza di RNG ([`D-096`](../../decisions/RT_PDR_00_Decision_Log.md)); l'Overwatch che riserva il
`Withdraw` ([`D-070`](../../decisions/RT_PDR_00_Decision_Log.md)); `Timeout → HOLD`; `Wait` che non concede
nulla di gratuito.

🔑 **Che una specifica scritta senza guardare il repository ne ratifichi 111 su 237 è il dato più
incoraggiante del referto**: il modello mentale dell'autore e il codice non hanno divergito.

## 7. La scala in Punti Movimento: che cosa è entrato, e con quale autorità

⚠️ **Va letto prima di usare i numeri.** La specifica §10.2 dichiara la scala **`PROPOSTA NUMERICA NON
APPROVATA COME CALIBRAZIONE DEFINITIVA`**: 12 PM base, costo 2,4 su terreno agevole, 3 su standard, 4 su
difficile → 5, 4 e 3 esagoni. §13.5 conferma che *«la scala 12/2,4/3/4 è soltanto un esempio»* mentre
*«PM, moltiplicatori e garanzia del primo passo sono approvati»*.

🔴 **Quei numeri entrano nel repository per decisione esplicita della sessione del 2026-09-13, non come
calibrazione d'autore.** È registrato in `D-412` e ripetuto qui perché è il tipo di provenienza che si
perde: chi leggerà «12 PM» fra sei mesi deve poter sapere che l'autore l'aveva marcata *non approvata* e che
a promuoverla è stata una scelta di sessione, per rendere la regola eseguibile invece di lasciarla sospesa.
**È taratura, non contratto**: si cambia senza toccare nessuna decisione.

## 8. Le decisioni prese — `D-407` … `D-415`

Numeri presi con la misura a tre posti su `origin/main` = `a2d23509`: registro massimo **`D-405`**; **12**
ref remoti, massimo **`D-406`** (rivendicato da `origin/issue/641-sprint-post-blast`, non mergiato); GitHub
**0** assegnazioni letterali per `D-407`…`D-413` — i nove risultati full-text sono tokenizzazione, verificati
uno per uno con `grep` su titolo e corpo, **0** occorrenze della stringa; **3** PR aperte, nessuna delle
quali tocca il registro (verificato sui `files`, non sui titoli).

| ID | Che cosa registra | Rapporto col canone |
|---|---|---|
| `D-407` | La barra ha undici comandi, e la difesa caratteristica ha uno slot proprio | estende `D-025` |
| `D-408` | La Guardia torna a **riduzione per colpo**, con valore per personaggio | supera `D-292` per la sola `Guard` |
| `D-409` | La difesa caratteristica occupa l'azione principale e consente `Withdraw`; si chiama `Evasion`, non `Dodge` | deroga a `D-028`/`D-191` per il solo slot |
| `D-410` | `Reflect` è una famiglia nuova, e **non** è `Deflect` | nomina, come `D-082` |
| `D-411` | L'occasione di Overwatch è **per nemico e per turno** | supera `D-169` in parte |
| `D-412` | Il budget di movimento è un **moltiplicatore** del budget base; chiude `AE-5`; la scala PM entra come taratura | supera `D-117` in parte |
| `D-413` | `Interact` dichiara la propria sequenza rispetto al percorso, e resta nel Blast | precisa `D-149` |
| `D-414` | Costi e cooldown si consumano al **piano definitivo** | supera `D-200`/`D-209` in parte |
| `D-415` | L'attacco base non dipende dall'arma, non insegue il bersaglio, e può sparare alla cieca | supera `D-086`…`D-100` nel loro unico consumatore; svuota il prezzo di `D-380` |

🔴 **`D-415` è la più cara delle nove, e va letta prima delle altre.** Le sue tre voci non sono argomentate
dalla sorgente — che non nomina mai né le varianti arma né il tiro indiretto — e ciascuna costa più di quanto
la riga che la enuncia lasci vedere. In particolare il punto (4): concedere il tiro alla cieca a quattro
attacchi base **gratuiti e senza cooldown** significa che i 6 danni e il turno di ricarica con cui
[`D-380`](../../decisions/RT_PDR_00_Decision_Log.md) ha pagato `Action.Mortar` **non compravano una
capacità, compravano un'esclusiva**. La voce lo scrive dentro di sé e apre le due code — *su cosa agiscono
adesso le varianti arma*, *come si riprezza `Action.Mortar`* — invece di dedurle.

## 9. I documenti aggiornati

| Documento | Che cosa cambia |
|---|---|
| [`gameplay/spec-attacco-base-per-eroe.md`](../../gameplay/spec-attacco-base-per-eroe.md) | **creato** — la sede che `ADR-0007` dichiarava da creare |
| [`gameplay/spec-barra-comandi.md`](../../gameplay/spec-barra-comandi.md) | **creato** — gli undici comandi e la tabella azione↔movimento |
| [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) | `AE-5` chiusa da `D-412`; aperte le domande di §13 senza soggetto deciso |
| [`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) | ogni `SUPERSEDED` di §8, con la fonte che prevale |
| [`gameplay/spec-economia-del-turno.md`](../../gameplay/spec-economia-del-turno.md) | §3.1 rimanda a `D-412` per i budget |
| [`CHANGELOG_DOCUMENTATION.md`](../../CHANGELOG_DOCUMENTATION.md) | la voce di questo consolidamento |

## 10. Perché otto migrazioni su nove non sono state fatte in questo passaggio

⛔ **Non è prudenza: è che una migrazione parziale produce due autorità**, che è precisamente ciò che
`CLAUDE.md` §16 mette sopra la velocità d'implementazione.

✅ **Una delle nove è stata migrata, ed è l'unica autonoma**: `D-412` — vedi §10-bis. Le altre no,
e questo è il perché.

I sette conflitti di §3 toccano il combat resolver, il catalogo delle azioni, il pathfinding, il modello di
conoscenza e le finestre di reazione. Ciascuno ha una famiglia di test che diventa rossa **per progetto**, e
un test rosso per progetto va riscritto insieme al codice che lo rende tale, non dopo. Le voci di §8 rendono
la specifica **autorevole**; le issue di §11 la rendono **eseguibile**, una migrazione per volta.

⚠️ **E il verde di §12 copre `D-412`, non le altre otto.** `CLAUDE.md` §6: chi scrive una correzione non ne
emette da solo il verdetto sui sistemi che tocca. La suite è verde — 657 su 657 — su un perimetro scelto per
**questa** migrazione: dice che il moltiplicatore non ha rotto nulla di ciò che esisteva, e non dice niente
sulle sette che restano. Ciascuna dovrà allargare il proprio filtro, e `D-414` dovrà aggiungere il corpus
golden che qui manca.

## 10-bis. L'unica migrazione fatta: `D-412`

`D-412` è l'unica delle nove che si chiude **dentro il proprio perimetro**: tocca il tipo del profilo, il
suo catalogo e i test che lo pinnano, e non attraversa né il combat resolver né le finestre di reazione.

| File | Che cosa cambia |
|---|---|
| `Ability/RTMovementProfile.h` | `StepBudget`/`MoveBudget` diventano `StepBudgetPercent`/`MoveBudgetPercent`; `InheritFromUnit` → `NeutralPercent` (100); `ScaleBudget` fa il troncamento in aritmetica intera |
| `Ability/RTMovementProfileLibrary.cpp` | `Sprint` 200 · `Withdraw` 25 · `Sneak` 50 e **pianificabile** · `Move`/`Still` 100 |
| `Ability/RTCatalogLibrary.cpp` | solo una **nota**: perché `Action.Sneak` NON entra — vedi sotto |
| `Turn/RTTurnManager.cpp` | solo il commento di `MakeSimUnit`, che dichiarava `InheritFromUnit` |
| `Tests/RTMovementProfileTests.cpp` | tre test riscritti sul modello nuovo |
| `Tests/RTCatalogTests.cpp` | tolto un totale volatile da un commento (`CLAUDE.md` §15) |

⛔ **`Action.Sneak` NON entra, e la revisione avversariale ha trovato perché.** Una voce del catalogo core
rende obbligatoria la propria chiave icona — `URTIconLibrary::RequiredIconIds()` la deriva da **ogni** azione
— e `DA_IconCatalog` non ha `UI.Icon.Action.Sneak`: il gate
`RefactorTactics.IconCatalog.RealCatalogCoversRequiredIds` sarebbe diventato **rosso**.

🔴 **E la prima misura non l'avrebbe visto**: il filtro dichiarato in §12 contiene `RefactorTactics.Catalog`,
che **non** è un prefisso di `RefactorTactics.IconCatalog.*`. Misurato: su `origin/main` le azioni core senza
icona sono **zero**; con l'aggiunta ne sarebbe stata una. ∴ il verde di 649 test era vero e **cieco su
quel gate**.

🔑 **Il rinvio non è una scorciatoia: è il confine giusto.** `D-412` chiude `AE-5` dando i numeri al
**profilo**, e il profilo non ha bisogno dell'azione finché non esiste chi la sceglie. L'azione — col suo
glifo e la sua voce di catalogo icone — appartiene a
[#1410](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1410), che porta il selettore e che
sta già aggiungendo `Action.Withdraw` per la stessa ragione.

🔴 **I numeri si muovono, e per quasi tutto il roster.** Il budget base non è il `MoveRange = 4` del
default di classe: è `ARTUnit::GetEffectiveMoveRange()`, che parte da `Hero->MovePoints`
(`Source/RefactorTactics/Unit/RTUnit.cpp:1600`), e il roster spedito dichiara **5 · 5 · 4 · 6**.

| Profilo | Prima | Ora | Sul roster |
|---|---|---|---|
| `Sprint` | **8** per tutti | ×2 | **10 · 10 · 8 · 12** |
| `Withdraw` | **2** per tutti | ×0,25 | **1** per tutti |
| `Sneak` | non definito | ×0,5 | **2 · 2 · 2 · 3** |

⚠️ **È un cambiamento di bilanciamento, non una riscrittura a parità di numeri**, e va detto così: lo
Sprint cambia per tre eroi su quattro, il `Withdraw` dimezza per tutti. Ciò che il moltiplicatore
*corregge* è un difetto che l'assoluto aveva — con `8` cablato, un eroe da 9 aveva lo scatto **più corto
del passo**.

🔑 **E il moltiplicatore corregge un difetto che l'assoluto aveva.** Con `Sprint` cablato a `8`, un eroe da
`MoveRange 9` aveva uno **scatto più corto del proprio passo**. Il nuovo test lo pinna su sette valori di
`MoveRange` invece di asserire un numero solo.

🔴 **Un pezzo di `D-412` NON è implementato, e il caso si raggiunge in partita**: il **primo passo
garantito** vive nel pathfinding — `RTHexSimLibrary.cpp` lo gaterebbe in quattro punti — e sotto un budget
di `4` il quarto del `Withdraw` è **zero**.

⛔ **E non è un caso teorico**, come una prima stesura di questa sezione sosteneva: `GetEffectiveMoveRange()`
applica lo `StandUp` di [`D-319`](../../decisions/RT_PDR_00_Decision_Log.md), quindi l'eroe che spedisce
`MovePoints = 4` scende a `3` quando si rialza — e con l'Overwatch armato **non ripiega affatto**.
`MovementProfile.CatalogDeclaresTheProfiles` asserisce quello zero con la ragione accanto, ma l'asserzione
non è la correzione: la correzione è il primo passo garantito, e non c'è.

⛔ **E la scala 12 PM non è entrata nel codice**, benché la sessione l'abbia adottata (§7): il denominatore
che la rende sensata — il costo per cella per terreno — è
[#666](https://github.com/DegrassiAaron/refactor-tactics-main/issues/666) e **non esiste**. Con ogni cella a
costo `1`, portare la base a 12 darebbe **12 celle** di movimento invece di 4. La regola è registrata; la
sua base resta `ARTUnit::MoveRange` finché `#666` non consegna il costo.

## 11. Le issue

Mappate leggendo il **corpo** di trentuno issue aperte, non i titoli.

### 11.1 Da aggiornare — una decisione ne cambia il contenuto

| Issue | Che cosa cambia |
|---|---|
| [#606](https://github.com/DegrassiAaron/refactor-tactics-main/issues/606) `CP 38.3` | riceve l'**assegnazione** dei valori alla soglia: [`spec-barra-comandi.md`](../../gameplay/spec-barra-comandi.md) §3 |
| [#1410](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1410) | l'insieme offribile diventa `{Sneak, Move, Sprint}`, e `ReservesMovementProfileId` acquista **due utenti in più** — `Evasion` e `Reflect` |
| [#666](https://github.com/DegrassiAaron/refactor-tactics-main/issues/666) `CP 38.8` | i moltiplicatori valgono su **entrambi** i budget di `D-117`: la funzione di costo li eredita |
| [#2589](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2589) · [#2586](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2586) | 🔴 **contraddette**: allineano i nomi al vocabolario del **pool**, che `D-408` ritira per la `Guard` |
| [#924](https://github.com/DegrassiAaron/refactor-tactics-main/issues/924) | 🔴 **contraddetta**: il suo DoD conserva *«ResolveDash scarta il piano»*, che `D-409` rimuove per lo slot `SignatureDefense` |
| [#314](https://github.com/DegrassiAaron/refactor-tactics-main/issues/314) `CP 14.7` | 🔴 **contraddetta**: fa di `Action.Brace` l'accesso al Reaction Profile, mentre `D-407` gli lascia il ruolo di riduzione e battezza `SignatureDefense` lo slot nuovo |
| [#2367](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2367) | `D-411` **non** la risolve: la sorgente voleva la decadenza a spostamento forzato, e `D-169` — *rilocalizza, non invalida* — resta |
| [#617](https://github.com/DegrassiAaron/refactor-tactics-main/issues/617) `ECO-1` · [#403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/403) `BAL-1` | il confronto Guard/Brace che devono giudicare cambia sotto: `D-408` toglie il tetto alla Guardia |
| [#1408](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1408) `E48` | conta il kit in **dieci** voci; `D-407` ne dichiara **undici** e aggiunge lo slot della difesa caratteristica |
| [#608](https://github.com/DegrassiAaron/refactor-tactics-main/issues/608) `CP 38.5` | §14 della sorgente propone scenari da **confrontare** con quelli già dichiarati, non da aggiungere sopra |

### 11.2 Confermate — la specifica le ratifica senza cambiarle

[#641](https://github.com/DegrassiAaron/refactor-tactics-main/issues/641) (Sprint dopo il Blast) ·
[#607](https://github.com/DegrassiAaron/refactor-tactics-main/issues/607) (la preview dice *perché*) ·
[#2501](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2501) (`Model A`: niente reroute) ·
[#2795](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2795) (Overwatch su visibilità, non rumore) ·
[#2341](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2341) (il facing che la Guardia legge) ·
[#2827](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2827) ·
[#339](https://github.com/DegrassiAaron/refactor-tactics-main/issues/339) ·
[#2826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2826) ·
[#2988](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2988).

### 11.3 Aperte — le migrazioni che rendono eseguibili le decisioni

Una per decisione, perché una per decisione è la granularità a cui si può tornare indietro.

| Issue | Decisione | Blocca / è bloccata |
|---|---|---|
| [#3128](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3128) | `SKB-1` — lo stordimento non esiste: si riapre o si riscrive? | ⛔ **blocca #3129, #3130, #3131, #3132, #3133** |
| [#3136](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3136) | `SKB-2` — il calendario dei sotto-passi (`CRITICO`) | ⛔ blocca ogni cadenza diversa da 1 passo per tick |
| [#3129](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3129) | `D-408` — Guardia per colpo, coi dodici test | bloccata da #3128 |
| [#3130](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3130) | `D-407`/`D-409` — slot `SignatureDefense` e `Evasion` | bloccata da #3128 |
| [#3131](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3131) | `D-410` — `Reflect` | bloccata da #3128 e #3130 |
| [#3132](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3132) | `D-411` — occasione per nemico e per turno | bloccata da #3128 |
| [#3133](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3133) | `D-413` — sequenza di `Interact` | bloccata da #3128 |
| [#3134](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3134) | `D-414` — consumo al piano definitivo | 🔴 misura sul corpus golden **prima** |
| [#3135](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3135) | `D-415` — attacco base | ⛔ **dopo** `SKB-4` e `SKB-5` |

⛔ **`SKB-1` viene prima di cinque delle sette migrazioni**: ciascuna incontra lo stordimento nel proprio
perimetro, e finché non si sa se la v0.1 lo riapre non hanno un soggetto su cui scrivere il proprio DoD.

### 11.4 Commentate — dove una decisione ne cambia il contenuto

[#606](https://github.com/DegrassiAaron/refactor-tactics-main/issues/606) ·
[#1410](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1410) ·
[#666](https://github.com/DegrassiAaron/refactor-tactics-main/issues/666) ·
[#2589](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2589) ·
[#2586](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2586) ·
[#924](https://github.com/DegrassiAaron/refactor-tactics-main/issues/924) ·
[#314](https://github.com/DegrassiAaron/refactor-tactics-main/issues/314) ·
[#2367](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2367) ·
[#617](https://github.com/DegrassiAaron/refactor-tactics-main/issues/617) ·
[#403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/403) ·
[#1408](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1408).

## 12. Verifica

Misurato su `HEAD e537b4aa` — dopo le correzioni della revisione avversariale — con albero pulito
verificato **prima e dopo**.

| Gate | Esito |
|---|---|
| Compile | ✅ **`PASS`** — `Result: Succeeded`, 0 errori, 14,39 s |
| Tests | ✅ **`PASS`** — **657** test, **657** `Success`, **0** `Fail` |
| Determinism | `NOT RUN` — nessuna famiglia golden nel filtro; vedi sotto |
| Replay | `N/A` — nessun formato di traccia cambia in questo passaggio |
| Privacy | `N/A` per il codice; ⚠️ **rilevata** una questione di boundary in §5, aperta come `SKB-3` |
| PIE | `N/A` |
| Packaged | `N/A` |

### Le due misure, e cosa le rende valide

**Compile.** `Build.bat RefactorTacticsEditor Win64 Development` sul worktree, `-MaxParallelActions=6` per
non saturare la macchina mentre un'altra sessione girava la propria suite: **`Result: Succeeded`** in 53,63 s.
I `warning C4996` sono deprecazioni UE preesistenti in `RTMatchWidgetAssetTests.cpp`, un file che questo
passaggio non tocca.

**Tests.** Le sette famiglie che `AGENTS.md` §9 impone a chi tocca il resolver del movimento —
`HexSim` · `HexMatch` · `HexOccupancy` · `Movement` · `HexMove` · `ForcedMovement` · `Scenario` — più le
cinque che questa migrazione tocca (`MovementProfile` · `Actions` · `Catalog` · `Bot` · `Combat`),
**`Reactions`** — aggiunta dopo il merge di `#641`, perché [`D-406`](../../decisions/RT_PDR_00_Decision_Log.md)
tocca il divieto di reazione dello `Sprint` — e **`IconCatalog`**.

🔴 **`IconCatalog` c'è perché la revisione avversariale ha mostrato che mancava, ed è la lezione di questo
referto.** `RefactorTactics.Catalog` **non** è un prefisso di `RefactorTactics.IconCatalog.*`: il filtro lo
escludeva senza che nulla lo segnalasse, e il verde precedente era vero e cieco proprio sul gate che la
prima stesura della PR avrebbe reso rosso. ✅ Ora `IconCatalog.RealCatalogCoversRequiredIds` è nel perimetro
ed è `Success`.

⚠️ **Il criterio generale, per chi allarga un filtro**: una famiglia si include verificando il **match di
sottostringa**, non l'intuizione sul nome — `Catalog` sembra coprire `IconCatalog` e non lo fa.

⚠️ **I test che `D-412` ha davvero riscritto sono TRE**, e sono questi:
`MovementProfile.CatalogDeclaresTheProfiles` · `MovementProfile.MoveInheritsUnitBudget` ·
`MovementProfile.SneakIsPlannableWithItsNumbers` (rovesciato da `SneakIsDeclaredButNotPlannable`).

Gli altri della stessa famiglia — `StillKeepsUnitCapacity`, `SnapshotCarriesBothBudgets`,
`PlanDeclaresTheProfile`, `CoreActionsNameTheirProfile`, `StabilityIsOrdered`, `OnlySprintIsARun` — la PR
**non li tocca**, e sono verdi come prova che la migrazione non li ha rotti. ⛔ Contarli fra i propri
gonfierebbe il lavoro fatto, ed è la ragione per cui la riga è stata riscritta.

### ✅ Il merge di `#641`, e la prova che i due modelli convivono

`#641` è stato mergiato su `main` **mentre questa PR era aperta**, e tocca gli stessi tre file: porta
[`D-406`](../../decisions/RT_PDR_00_Decision_Log.md), il campo `bIsRun` che dà un soggetto a
[`D-319`](../../decisions/RT_PDR_00_Decision_Log.md) — *«chi ha perso l'equilibrio non corre»*.

🔑 **I due cambiamenti sono ortogonali, e il conflitto era solo testuale**: `bIsRun` dice **che cosa** il
profilo è, la percentuale di `D-412` dice **quanto** concede. `MakeProfile` ha ora una firma fusa, e lo
`Sprint` è `200` **e** una corsa.

✅ **A dirlo non è il ragionamento, è il test di qualcun altro**: `MovementProfile.OnlySprintIsARun` — scritto
da `#641`, non da qui — è `Success` sull'albero mergiato. Se la fusione avesse perso `bIsRun` o l'avesse
attribuito al profilo sbagliato, quello sarebbe rosso.

⚠️ **E una cosa è stata misurata invece che assunta**: `#641` ha consegnato il **solo prerequisito**.
`Action.Sprint` resta a `FastMovement` e il test si chiama ancora
`Actions.SprintIsAMoveProfileResolvedPreBlast`. La migrazione di fase è ancora davanti, e le righe di
`D-412` le restano coerenti.

### ⚠️ Tre limiti dichiarati, perché un verde senza perimetro non dice niente

1. **`Determinism` resta `NOT RUN`**: il filtro non include `Simulation.GoldenCorpus*`. Per `D-412` non è
   il gate che conta — nessun formato di traccia cambia — ma per [`D-414`](../../decisions/RT_PDR_00_Decision_Log.md)
   lo sarà, e quella misura va fatta **prima** di spostare la sede di scrittura del cooldown.
2. **`Content/FabAsset` non esiste**, e il log porta warning su animazioni Paragon mancanti. ✅ **Non è un
   artefatto del worktree**: è assente **anche nel clone principale**, quindi il perimetro coperto è lo
   stesso di qualunque altro checkout. Verificato invece di assunto.
3. **Alcune famiglie restano fuori dal filtro** — fra cui `Knowledge`, `Overwatch`, `Veil`. `AGENTS.md`
   §9 lo prescrive: *«se tocchi anche reazioni, conoscenza o presentazione, aggiungi le loro»*, e
   `Reactions` è stata aggiunta per questo dopo il merge di `#641`. `D-412` non tocca le altre; le otto
   migrazioni di §11.3 sì, e ciascuna dovrà allargare il proprio.

### ⏱️ La prima misura è stata scartata, e la seconda ha trovato un errore vero

*Fino a poche ore fa questa sezione diceva `Compile: NOT RUN`, e il motivo era che il Live Coding di un
Editor aperto su `D:\Repositories\refactor-tactics-main` faceva uscire `Build.bat` con **6** —* «Unable to
build while Live Coding is active» *— e il mutex è sull'eseguibile dell'engine condiviso, non sul checkout.*

🔴 **Poi la finestra si è aperta, e il compilatore ha trovato un difetto che nessuna rilettura aveva
visto**: `RTMovementProfileTests.cpp(194)` chiamava `URTMovementProfileLibrary::OfferableProfiles()`, che
**non esiste su `origin/main`** — la porta [#1410](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1410)
insieme al selettore. È la stessa causa delle diciassette citazioni corrette in §1: un simbolo visto nel
clone principale, che porta lavoro non mergiato. ∴ **la nota di metodo di §1 vale anche per il codice, non
solo per i numeri di riga** — ed è la ragione per cui il gate va eseguito e non dedotto.

⚠️ **Una misura intermedia è stata scartata da sé stessa**: l'albero è cambiato mentre girava (una nota di
commento), e `AGENTS.md` §9 dice che una misura che non osserva lo stesso working tree dall'inizio alla fine
è `NON VALIDA`. È servita a trovare l'errore, non a firmare un verde.
