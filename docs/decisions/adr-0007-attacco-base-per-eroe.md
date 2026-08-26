# ADR-0007 — Attacco base: categoria universale, payload dell'eroe

- **Stato:** Accepted
- **Data:** 2026-08-09
- **Decision Log:** [D-058](RT_PDR_00_Decision_Log.md) *(assegnata il 2026-08-09 verificando il massimo su **tutti** i branch remoti, non solo su `main`: il progetto ha già avuto sei collisioni di contatore, e due branch aperti rivendicavano ID non ancora mergiati)*
- **Owner spec:** da creare (`../gameplay/spec-attacco-base-per-eroe.md`)
- **Issue:** [#315](https://github.com/DegrassiAaron/refactor-tactics-main/issues/315)
- **Provenienza:** consolidamento `RefactorTactics_BasicAttack_Consolidation_Claude_2026-08-09.md`, rivisto contro il codice

## Contesto

`BasicAttack` è una delle sette azioni generiche ([D-025](RT_PDR_00_Decision_Log.md)), e
[D-033](RT_PDR_00_Decision_Log.md) ha già fissato **la forma**: il modificatore di un'azione generica si
chiama **`profilo`**, vale per tutte e sette, è fisso per eroe in v0.1, dichiara trade-off e non solo
vantaggio. Quella decisione si è chiusa lasciando esplicitamente aperta una domanda: *«i profili concreti dei
4 eroi restano domande aperte»*.

**Questo ADR chiude quella domanda per `BasicAttack`.** Non introduce un framing nuovo e non conia un secondo
nome: usa `profilo`, come impone D-033.

Il catalogo v0.1 ha **già** quattro attacchi base distinti — indice 0 del kit di ogni eroe — ma nessun
documento diceva *perché* debbano differire, né con quale regola. La conseguenza è che ogni nuovo eroe
rischiava di ereditare un attacco base scelto per analogia invece che per ruolo.

Il consolidamento del 2026-08-09 proponeva quattro famiglie tattiche e una matrice di identità. La verifica
contro il catalogo ha mostrato che **tre valori su quattro** della matrice contraddicevano il canone: Riktor
era classificato «potenza molto bassa» pur avendo l'attacco base **più forte del roster** (24), Gadget
«medio-bassa» con il secondo più forte (22), Wraith «alta» con il terzo (21). Solo Phase era conforme.

Serve quindi fissare **la regola**, non i quattro numeri: la regola sopravvive al roster, i numeri no.

## Decisione

**1. `BasicAttack` è una categoria universale; il payload è dell'eroe.**
L'identità del personaggio deve restare leggibile anche quando non usa un'abilità fondamentale.

**2. Nessun ramo per eroe.** Il payload si compone di primitive condivise già esistenti (`Damage`, `Status`,
`Push`, `Pull`, …). Nessun `if` sull'`HeroId` nel resolver o nell'orchestratore — riafferma
[ADR-0006](adr-0006-ownership-abilita-sinergie.md).

**3. Quattro famiglie di ruolo tattico** — Primary Weapon · Engine · Setup · Utility/Emergency — sono
**tassonomia di design e documentazione**, non tipi C++ né statistiche runtime. Descrivono *quanto e perché*
l'attacco base entra nel decision making dell'eroe, **non** la sua potenza.

**4. Assegnazione v0.1:**

| Eroe | Famiglia | Decisione | Effetto sui dati |
|---|---|---|---|
| Phase | Setup | già conforme | **nessuno** — `PressureJet` 16 / r5 / `Wet` / `Push 1` resta |
| Riktor | Utility / Emergency | **cambiano i numeri** | `ImpactShot` **24 → 8** danni, **+ `Status.Slow` 1 turno**, range 3 invariato |
| Wraith | Primary Weapon | **nessun cambiamento ai dati** | `PulseShot` 21 / r4 resta: «Primary» è profilo d'uso, non potenza |
| Gadget | Engine | **rinviata** | `ArcPulse` 22 / r4 resta damage-only |

**5. Gadget resta damage-only in v0.1.** Nessuno stato `Charged` esiste nel codice — verificato, l'unica
occorrenza della stringa è `bChargedIntoTarget`, un bool locale della carica di *movimento*.

Il motore elettrico di Gadget **esiste e ha già un owner**: è `ConductiveNode`, cablata su `Action.Electrify`
da [D-046](RT_PDR_00_Decision_Log.md). Spostare la generazione di carica sull'attacco base le darebbe un
**secondo produttore**, cioè il contrario di quello che D-046 ha appena messo in ordine. La famiglia Engine
descrive quindi il **kit** di Gadget, e in v0.1 non passa dal suo attacco base.

**6. Il ruolo di «attacco base» resta la convenzione posizionale `URTHeroData::Actions[0]`**, resa esplicita
da un test invece che da un commento. Un campo dedicato entra **solo** quando esiste il primo consumer
runtime (metriche, UI, tutorial) — non prima.

## Punto aperto ereditato da D-033 — il TurnLog

D-033 richiede che un'azione generica con profilo sia **spiegabile nel TurnLog come *azione base + profilo***.
Oggi i quattro attacchi base non passano da `Action.BasicAttack`: hanno `ActionId` propri
(`Hero.Gadget.ArcPulse`, `Hero.Phase.PressureJet`, `Hero.Riktor.ImpactShot`, `Hero.Wraith.PulseShot`), quindi il TurnLog registra
**il nome dell'eroe**, non la coppia.

Questo ADR **non risolveva** quel punto e non lo dichiarava risolto: lo registrava come conseguenza da
verificare. Era anche la ragione per cui il punto 6 era una scelta *per la v0.1* e non definitiva — se il
TurnLog deve esprimere la coppia, serve rendere il profilo un dato esplicito, e quello sarebbe stato il primo
consumer reale.

> ✅ **Risolto il 2026-08-09** ([#354](https://github.com/DegrassiAaron/refactor-tactics-main/issues/354)).
> Il consumer **era questo**: `FRTActionDef::BaseActionId` dichiara di quale generica un'azione è profilo, la
> voce di TurnLog lo porta (formato **v5**) e `DescribeActionIdentity` rende la coppia
> `Action.BasicAttack · Hero.Riktor.ImpactShot`.
>
> **La regola del punto 6 non è stata violata — è stata soddisfatta.** Diceva che un campo entra *solo quando
> esiste il consumer*; D-033 era il consumer, e finché non è stato costruito il campo non c'era. Questo è il
> modo in cui quella regola è pensata per funzionare: non «mai un campo», ma «nessun campo senza qualcuno che
> lo legga».
>
> Resta invece vero **per il ruolo**: «attacco base» è ancora la convenzione posizionale `Actions[0]`, tenuta
> da `BasicAttackIsIndexZeroForEveryHero`. `BaseActionId` dice *di cosa* un'azione è profilo, non *che ruolo*
> ha nel kit: sono due domande diverse, e solo la prima aveva un consumer.

> ➕ **Aggiunta il 2026-08-26 ([D-195](RT_PDR_00_Decision_Log.md)): le domande diverse sono TRE.** Accanto a
> `BaseActionId` esiste ora `FRTActionDef::DerivedFromActionId`, che dice **da quale azione core un'abilità
> eredita i suoi valori** — fase, priorità, portata, fallback, effetti.
>
> | Campo | Domanda | Chi lo legge |
> |---|---|---|
> | `Actions[0]` | che **ruolo** ha nel kit | `BasicAttackIsIndexZeroForEveryHero` |
> | `BaseActionId` | di quale delle **sette generiche** è il profilo (D-033) | `DescribeActionIdentity` |
> | `DerivedFromActionId` | da quale azione core prende i **numeri** | `Catalog.EveryCoreActionIsReachableOrDeclared` |
>
> ⛔ **I due campi non si fondono**, e la ragione è che le sorgenti non coincidono: `Hero.Riktor.Ram` eredita
> da `Action.Charge`, che fra le sette generiche **non c'è** — un «profilo di Charge» non esiste. Scriverlo
> in `BaseActionId` avrebbe reso indistinguibili due relazioni diverse nella stessa traccia, ed è ciò che
> `BasicAttackDeclaresItsBaseAction` vietava: quel test resta valido parola per parola, e non è stato
> toccato. La regola del punto 6 vale anche per il campo nuovo — è entrato **con** il suo consumatore.

## Perché `Slow` per Riktor

Delle cinque utility proposte, tre non sono esprimibili con le primitive esistenti: `ERTStructureOp` ha solo
`None` / `CreateCover` / `MoveCover`, quindi «danno contro cover leggera» e «rimozione di oggetto fragile»
non hanno operazione; «generazione di Guard» applicherebbe lo stato a sé, mentre `ERTActionEffect::Status`
lo applica al **bersaglio**; e uno stato `Stagger` non esiste.

`Status.Slow` ha un consumer verificato (`MoveCostModifier`) ed è la scelta tematicamente coerente: la
debolezza dichiarata di Riktor è `Affinity.Movement`, quindi il suo attacco base diventa la sua piccola
risposta a chi si muove di mestiere. Resta distinto da Phase, che usa `Push`.

Il valore **8** è ancorato a un numero già canonico invece che scelto a caso: il documento colloca Phase (16)
un gradino sopra Riktor, e 8 ne è la metà esatta. Lascia possibile il *finish* su bersaglio a pochi HP, che
è l'uso dichiarato della famiglia.

## Conseguenze

- **Una sola riga di catalogo cambia.** Il modello regge quattro identità diverse senza codice nuovo: è la
  verifica della tesi, non un suo effetto collaterale.
- **Rimisurati** — 4 scenari e 1 file di test citavano `Hero.Riktor.ImpactShot` con il vecchio numero:
  `Combat.CounterStrikesBack` (81 → 90), `Combat.NoCounterWhenUnarmed` (66 → 82), `Visual.Combat.Defeat`
  (2 → 4 turni), `Visual.Map.LowCoverEdge` (55 → 71, con i tiratori scambiati), `RTHeroRiktorTests.cpp`.
  `RTHeroReactionTests.cpp` **non** è stato toccato: deriva l'atteso da `HeroReactDeclaredDamage(Actions[0])`
  invece di replicare il numero, ed è il modo giusto di scrivere quell'assert.
- **Rischio dichiarato:** Riktor è già l'eroe più incompleto del roster — solo `ImpactShot` e `Ram` sono
  interamente rappresentabili. Portarlo da 24 a 8 lo lascia con poco che funzioni finché le sue altre azioni
  non hanno un consumer. È il costo accettato di questa decisione, non un effetto imprevisto.
- **Gadget non va pubblicato come «Engine Attack»** in Wiki o Feature Registry finché il payload non esiste:
  sarebbe uno stato che il codice non sostiene.
- La coordinazione acqua+elettricità continua a passare da `Hero.Gadget.LinearDischarge`, non dall'attacco base, e
  resta vincolata a [D-036](RT_PDR_00_Decision_Log.md): stesso Blast, non turni consecutivi.
- Feature registry: la capability è `RT-FEAT-ACTION-BASIC-ATTACK-PROFILES`, modellata su
  `RT-FEAT-ACTION-MOVE-PROFILES`. **Non** si crea `RT-FEAT-CHAR-BASE-ACTION-SIGNATURE`: sarebbe una gerarchia
  parallela sopra sette azioni.

## Non decisioni

- **Non** rinomina nessuna azione. `ArcPulse`, `PressureJet`, `ImpactShot`, `PulseShot` restano i nomi
  canonici; i nomi candidati del consolidamento (`Charge Bolt`, `Water Pulse`, `Kinetic Bash`, `Vector Shot`)
  sono respinti.
- **Non** introduce cooldown, charges, reload o combo counter sugli attacchi base: nessun eroe attuale li
  richiede.
- **Non** apre `RT-FEAT-ENV-ELECTRIC`, né decide quando Gadget otterrà il suo payload.
- **Non** decide il formato definitivo del profilo: la convenzione `Actions[0]` è la scelta per la v0.1, non
  per sempre.
- **Non** rende `BasicAttackUsageProfile` (§14 del consolidamento) una statistica runtime: resta rubrica di
  design e di playtest.

## Alternative scartate

- **Riktor: riclassificare la famiglia invece dei numeri.** Avrebbe avuto costo zero sui dati, e il 24/r3 è
  un numero motivato (la fascia corto raggio darebbe 25, il catalogo ne toglie uno «in cambio della stazza»).
  Scartata perché avrebbe lasciato la famiglia Utility/Emergency senza alcun rappresentante in v0.1, cioè
  senza la dimostrazione che un attacco base a basso danno può essere una scelta corretta.
- **Wraith: payoff condizionale sulla geometria.** È il design più interessante, ma richiede un consumer
  runtime della condizione: è un checkpoint proprio, non un campo dati. Rinviato senza essere respinto.
- **Wraith: alzare il danno.** Avrebbe reso «Primary» osservabile al prezzo di contraddire la motivazione già
  scritta nel catalogo («-1 pagato in mobilità»).
