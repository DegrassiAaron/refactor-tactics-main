# ADR-0007 — Attacco base: categoria universale, payload dell'eroe

- **Stato:** Accepted
- **Data:** 2026-08-09
- **Decision Log:** `D-0xx` *(da assegnare al merge — il progetto ha già avuto sei collisioni di contatore fra sessioni parallele: l'ID si prende quando il branch entra, non ora)*
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
contro il catalogo ha mostrato che **tre valori su quattro** della matrice contraddicevano il canone: Bastion
era classificato «potenza molto bassa» pur avendo l'attacco base **più forte del roster** (24), Flux
«medio-bassa» con il secondo più forte (22), Vektor «alta» con il terzo (21). Solo Riva era conforme.

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
| Riva | Setup | già conforme | **nessuno** — `PressureJet` 16 / r5 / `Wet` / `Push 1` resta |
| Bastion | Utility / Emergency | **cambiano i numeri** | `ImpactShot` **24 → 8** danni, **+ `Status.Slow` 1 turno**, range 3 invariato |
| Vektor | Primary Weapon | **nessun cambiamento ai dati** | `PulseShot` 21 / r4 resta: «Primary» è profilo d'uso, non potenza |
| Flux | Engine | **rinviata** | `ArcPulse` 22 / r4 resta damage-only |

**5. Flux resta damage-only in v0.1.** Nessuno stato `Charged` esiste nel codice — verificato, l'unica
occorrenza della stringa è `bChargedIntoTarget`, un bool locale della carica di *movimento*.

Il motore elettrico di Flux **esiste e ha già un owner**: è `ConductiveNode`, cablata su `Action.Electrify`
da [D-046](RT_PDR_00_Decision_Log.md). Spostare la generazione di carica sull'attacco base le darebbe un
**secondo produttore**, cioè il contrario di quello che D-046 ha appena messo in ordine. La famiglia Engine
descrive quindi il **kit** di Flux, e in v0.1 non passa dal suo attacco base.

**6. Il ruolo di «attacco base» resta la convenzione posizionale `URTHeroData::Actions[0]`**, resa esplicita
da un test invece che da un commento. Un campo dedicato entra **solo** quando esiste il primo consumer
runtime (metriche, UI, tutorial) — non prima.

## Punto aperto ereditato da D-033 — il TurnLog

D-033 richiede che un'azione generica con profilo sia **spiegabile nel TurnLog come *azione base + profilo***.
Oggi i quattro attacchi base non passano da `Action.BasicAttack`: hanno `ActionId` propri
(`Flux.ArcPulse`, `Riva.PressureJet`, `Bastion.ImpactShot`, `Vektor.PulseShot`), quindi il TurnLog registra
**il nome dell'eroe**, non la coppia.

Questo ADR **non risolve** quel punto e non lo dichiara risolto: lo registra come conseguenza da verificare.
È anche la ragione per cui il punto 6 è una scelta *per la v0.1* e non definitiva — se il TurnLog deve
esprimere la coppia, servirà rendere il profilo un dato esplicito, e quello sarà il primo consumer reale.

> Da misurare prima di marcare il gate `log_debug`: il TurnLog distingue oggi un attacco base da un'abilità
> fondamentale? Se no, è un difetto di D-033 che questo ADR eredita, non ne crea uno nuovo.

## Perché `Slow` per Bastion

Delle cinque utility proposte, tre non sono esprimibili con le primitive esistenti: `ERTStructureOp` ha solo
`None` / `CreateCover` / `MoveCover`, quindi «danno contro cover leggera» e «rimozione di oggetto fragile»
non hanno operazione; «generazione di Guard» applicherebbe lo stato a sé, mentre `ERTActionEffect::Status`
lo applica al **bersaglio**; e uno stato `Stagger` non esiste.

`Status.Slow` ha un consumer verificato (`MoveCostModifier`) ed è la scelta tematicamente coerente: la
debolezza dichiarata di Bastion è `Affinity.Movement`, quindi il suo attacco base diventa la sua piccola
risposta a chi si muove di mestiere. Resta distinto da Riva, che usa `Push`.

Il valore **8** è ancorato a un numero già canonico invece che scelto a caso: il documento colloca Riva (16)
un gradino sopra Bastion, e 8 ne è la metà esatta. Lascia possibile il *finish* su bersaglio a pochi HP, che
è l'uso dichiarato della famiglia.

## Conseguenze

- **Una sola riga di catalogo cambia.** Il modello regge quattro identità diverse senza codice nuovo: è la
  verifica della tesi, non un suo effetto collaterale.
- **Rimisurati** — 4 scenari e 1 file di test citavano `Bastion.ImpactShot` con il vecchio numero:
  `Combat.CounterStrikesBack` (81 → 90), `Combat.NoCounterWhenUnarmed` (66 → 82), `Visual.Combat.Defeat`
  (2 → 4 turni), `Visual.Map.LowCoverEdge` (55 → 71, con i tiratori scambiati), `RTHeroBastionTests.cpp`.
  `RTHeroReactionTests.cpp` **non** è stato toccato: deriva l'atteso da `HeroReactDeclaredDamage(Actions[0])`
  invece di replicare il numero, ed è il modo giusto di scrivere quell'assert.
- **Rischio dichiarato:** Bastion è già l'eroe più incompleto del roster — solo `ImpactShot` e `Ram` sono
  interamente rappresentabili. Portarlo da 24 a 8 lo lascia con poco che funzioni finché le sue altre azioni
  non hanno un consumer. È il costo accettato di questa decisione, non un effetto imprevisto.
- **Flux non va pubblicato come «Engine Attack»** in Wiki o Feature Registry finché il payload non esiste:
  sarebbe uno stato che il codice non sostiene.
- La coordinazione acqua+elettricità continua a passare da `Flux.LinearDischarge`, non dall'attacco base, e
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
- **Non** apre `RT-FEAT-ENV-ELECTRIC`, né decide quando Flux otterrà il suo payload.
- **Non** decide il formato definitivo del profilo: la convenzione `Actions[0]` è la scelta per la v0.1, non
  per sempre.
- **Non** rende `BasicAttackUsageProfile` (§14 del consolidamento) una statistica runtime: resta rubrica di
  design e di playtest.

## Alternative scartate

- **Bastion: riclassificare la famiglia invece dei numeri.** Avrebbe avuto costo zero sui dati, e il 24/r3 è
  un numero motivato (la fascia corto raggio darebbe 25, il catalogo ne toglie uno «in cambio della stazza»).
  Scartata perché avrebbe lasciato la famiglia Utility/Emergency senza alcun rappresentante in v0.1, cioè
  senza la dimostrazione che un attacco base a basso danno può essere una scelta corretta.
- **Vektor: payoff condizionale sulla geometria.** È il design più interessante, ma richiede un consumer
  runtime della condizione: è un checkpoint proprio, non un campo dati. Rinviato senza essere respinto.
- **Vektor: alzare il danno.** Avrebbe reso «Primary» osservabile al prezzo di contraddire la motivazione già
  scritta nel catalogo («-1 pagato in mobilità»).
