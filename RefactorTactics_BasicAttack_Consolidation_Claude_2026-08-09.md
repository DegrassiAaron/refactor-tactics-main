# REFACTORTACTICS — BASIC ATTACK SYSTEM CONSOLIDATION PACK FOR CLAUDE CODE

**Data:** 2026-08-09  
**Progetto:** RefactorTactics  
**Scopo:** integrare nel repository le decisioni emerse nel focus sugli **attacchi base**, consolidando prodotto, design, dati, roadmap, Wiki, Feature Map/Registry, Scenario Map/Registry, Epic/Issue e test senza creare fonti concorrenti.

> Questo file è un **handoff operativo per Claude Code**.  
> Non è prova che una feature sia già implementata.  
> Claude deve prima verificare repository, codice, test, issue GitHub e documentazione canonica reale.

---

# 0. MISSIONE

Integrare nel progetto il seguente principio di design:

> **Basic Attack è una categoria universale del linguaggio di azioni, ma il suo valore tattico, il payload e il modo corretto di usarlo possono essere radicalmente diversi da personaggio a personaggio.**

Non tutti i personaggi devono usare il Basic Attack come fonte primaria di DPS.

Per alcuni può essere:

- la principale arma di pressione;
- il motore che alimenta la Signature Mechanic;
- uno strumento di setup;
- uno strumento di utilità;
- una fallback action;
- una scelta situazionale rara ma corretta.

L'obiettivo non è uniformare i Basic Attack.

L'obiettivo è che:

> **anche usando una sola azione universale, Vektor, Flux, Riva e Bastion continuino a sentirsi personaggi completamente diversi.**

Il consolidamento deve aggiornare, dove esistono già:

1. Product Map / piano prodotto;
2. documentazione gameplay e tecnica;
3. Decision Log / ADR appropriati;
4. `RT_ActionCatalog_v0.1`;
5. `RT_HeroCatalog_v0.1`;
6. eventuale Character Definition / Action Definition authoring;
7. Feature Map e Feature Registry canonico;
8. Roadmap;
9. Scenario Map / Scenario Registry;
10. Wiki;
11. test Automation / Scenario Harness / Functional;
12. Epic / Issue GitHub e relative relazioni;
13. showcase v0.1, se può mostrare questa differenziazione senza allargarne inutilmente lo scope.

NON creare:

- un secondo Feature Registry;
- una seconda Roadmap;
- una seconda Scenario Map;
- una nuova Action Engine parallela;
- un sottosistema C++ per ogni personaggio;
- issue duplicate;
- nuove feature fuori scope solo per rendere il Basic Attack più elaborato.

---

# 1. AUDIT OBBLIGATORIO PRIMA DI MODIFICARE

Prima di qualsiasi modifica:

```bash
git status
git branch --show-current
git rev-parse HEAD
```

Leggere prima, se presenti:

```text
CLAUDE.md
AGENTS.md
README.md
CONTEXT_INDEX.md

docs/product/piano-canonico-mvp.md

docs/decisions/RT_PDR_00_Decision_Log.md
docs/decisions/adr-0003-modello-azioni-v01.md
docs/decisions/adr-0004-finestre-di-reazione.md
docs/decisions/adr-0005-orientamento.md
docs/DOC_CONFLICT_MATRIX.md
docs/OPEN_DECISIONS.md

docs/gameplay/spec-sequenza-turno.md
docs/gameplay/spec-motore-azioni-e4.md
docs/gameplay/*
docs/technical/brief-planning-visuale.md

docs/balance/RT_ActionCatalog_v0.1.md
docs/balance/RT_HeroCatalog_v0.1.md
docs/balance/RT_TerrainCatalog_v0.1.md
docs/balance/RT_EquipmentCatalog_v0.1.md

docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/v0.1-definition-of-done.md
docs/roadmap/feature-registry.*

docs/product/showcase-v0.1.md

Scenarios/
Source/RefactorTactics/Turn/
Source/RefactorTactics/Tests/
Source/RefactorTactics/ScenarioHarness/
```

Cercare repository-wide:

```text
BasicAttack
Basic Attack
Action.BasicAttack
Generic Actions
Base Action Signature
Character Action Profile
Attack OR Ability OR Overwatch

Flux
Riva
Bastion
Vektor

Charge
Charged
Wet
Water
Cover
Stagger
Guard
Facing
Noise
MovingTarget
Cooldown
Charges
Reload

E4
E8
E9
E13
E15
E16
```

Aprire anche la Wiki reale e verificare:

- pagina Generic Actions;
- pagina Basic Attack, se esiste;
- pagine Flux / Riva / Bastion / Vektor;
- pagine Feature;
- pagine Scenario;
- eventuali pagine collegate alla Roadmap.

---

# 2. ORDINE DI PREVALENZA

Usare questa gerarchia:

```text
1. Decisioni esplicite più recenti del progetto
2. Codice corrente + test automatici realmente presenti
3. Piano canonico MVP
4. ADR / Decision Log correnti
5. Cataloghi balance correnti
6. Feature Registry / Roadmap correnti
7. Specifiche gameplay recenti
8. Handoff recenti
9. PDR / PDF / workbook storici
```

Se una proposta di questo documento entra in conflitto con una decisione canonica già più recente:

- NON sostituirla silenziosamente;
- registrare il conflitto;
- mantenere il canone esistente;
- creare `Decision Required` solo se necessario.

---

# 3. CANONE GENERALE DA PRESERVARE

## 3.1 Ordine del turno

Preservare:

```text
Planning
  -> Commit
  -> Prep
  -> Dash
  -> Blast
  -> Move
  -> Cleanup
```

Vincolo:

> Il normale `Move` resta l'ultima fase volontaria standard.

Basic Attack appartiene alla normale azione offensiva / fase prevista dal sistema corrente e NON deve reintrodurre sequenze arbitrarie tipo:

```text
Move -> Basic Attack
```

---

## 3.2 Generic Actions

La tassonomia operativa recente comprende almeno:

```text
Wait
BasicAttack
Interact
Brace
Move
Overwatch
```

con Dash/special movement trattato separatamente secondo il modello attuale.

Preservare:

```text
Attack OR Ability OR Overwatch
```

come competizione dello slot offensivo/principale se è ancora il modello canonico del repository.

---

# 4. DECISIONE CENTRALE — BASIC ATTACK NON È “LA SKILL DEBOLE”

Da consolidare come principio di prodotto e character design:

> **Basic Attack non significa automaticamente “abilità debole senza cooldown”.**

Il Basic Attack è una action category universale.

Il personaggio può però assegnargli un ruolo tattico differente.

Quattro famiglie di riferimento:

```text
1. PRIMARY WEAPON
2. ENGINE ATTACK
3. SETUP ATTACK
4. UTILITY / EMERGENCY ATTACK
```

Queste famiglie sono una tassonomia di design, NON necessariamente quattro classi C++.

---

# 5. PRIMARY WEAPON

Definizione:

> Il personaggio usa spesso il Basic Attack perché è una parte centrale del suo output e della sua pressione.

Il kit speciale può creare occasioni migliori per il Basic Attack.

Pattern desiderato:

```text
special ability / positioning / ally setup
        ->
good firing opportunity
        ->
Basic Attack payoff
```

Quindi il flusso non deve essere sempre:

```text
Basic Attack
  ->
aspetto che torni la vera skill
```

Può essere:

```text
Skill
  ->
crea geometria / finestra
  ->
Basic Attack
```

---

# 6. ENGINE ATTACK

Definizione:

> Il Basic Attack non è interessante soltanto per il danno immediato: genera, modifica o alimenta una meccanica persistente del personaggio.

Può per esempio:

- generare risorsa;
- applicare Charge;
- alimentare nodi;
- preparare uno stato;
- modificare una rete;
- creare una condizione consumabile da un'altra skill.

Il valore deve essere misurato su:

```text
ImmediateDamage
+
FutureSetupValue
+
TeamComboValue
```

non sul danno grezzo isolato.

---

# 7. SETUP ATTACK

Definizione:

> Il Basic Attack è principalmente uno strumento per preparare terreno, status, posizione o combo.

Esempi di payoff possibili:

- `Wet`;
- superficie modificata;
- Fire attenuato;
- target predisposto a Shock;
- Mark;
- esposizione;
- displacement leggero.

Il giocatore che usa questo Basic Attack cercando solo DPS dovrebbe essere chiaramente meno efficiente rispetto a chi comprende il setup.

---

# 8. UTILITY / EMERGENCY ATTACK

Definizione:

> Il Basic Attack ha output offensivo modesto, ma resta una scelta contestualmente corretta quando le azioni signature non sono necessarie o disponibili.

Può servire per:

- finire un bersaglio a pochi HP;
- rompere una cover leggera;
- applicare un piccolo controllo;
- cambiare Facing;
- generare Guard;
- interrompere un oggetto;
- effettuare un'azione affidabile senza spendere risorse importanti.

Regola fondamentale:

> **Danno basso non deve significare pulsante finto.**

Se il Basic Attack di un personaggio è letteralmente sempre una scelta sbagliata, va riprogettato o eliminato come scelta player-facing.

---

# 9. ROSTER v0.1 — MATRICE DI IDENTITÀ DA PROTOTIPARE

Roster operativo:

```text
Flux
Riva
Bastion
Vektor
```

Showcase:

```text
Flux + Riva
vs
Bastion + Vektor
```

Stato canonico verificato (`main` @ `ea26c0f` · `RTHeroCatalogLibrary.cpp` · `RT_HeroCatalog_v0.1.md`):

| Personaggio | Attacco base reale | Range | Danno | Effetti dichiarati |
|---|---|---:|---:|---|
| Vektor | `Vektor.PulseShot` | 4 | 21 | solo danno |
| Flux | `Flux.ArcPulse` | 4 | 22 | solo danno (fascia generica, `MakeBasicAttack(4)`) |
| Riva | `Riva.PressureJet` | 5 | 16 | danno + `Status.Wet` (1 turno) + `Push 1`, forma linea |
| Bastion | `Bastion.ImpactShot` | 3 | 24 | solo danno |

Famiglia assegnata e **delta effettivamente richiesto** — decisioni chiuse il 2026-08-09 e registrate in
[ADR-0007](docs/decisions/adr-0007-attacco-base-per-eroe.md), che chiude per `BasicAttack` i «profili
concreti dei 4 eroi» lasciati aperti da **D-033**:

| Personaggio | Famiglia | Conforme oggi? | Decisione |
|---|---|---|---|
| Riva | Setup Attack | **sì** | `KEEP` — nessun lavoro su dati o codice |
| Vektor | Primary Weapon | non distinguibile | `KEEP` — «Primary» è profilo d'uso, non potenza |
| Flux | Engine Attack | no | `DEFERRED` — resta damage-only, bloccata da `RT-FEAT-ENV-ELECTRIC` |
| Bastion | Utility / Emergency | **no, invertito** | `CHANGE` — `ImpactShot` **24 → 8** danni **+ `Status.Slow` 1 turno**, range 3 invariato |

**Una sola riga di catalogo cambia.** Che quattro identità diverse reggano con un solo delta è la verifica
della tesi di questo documento, non un suo effetto collaterale.

> ⚠️ **Bastion è il caso che rovescia la matrice originale.** `ImpactShot` fa **24 danni: il valore più alto
> del roster**. La formulazione iniziale lo classificava «potenza immediata molto bassa». Non è un dettaglio
> di bilanciamento: è la premessa dell'intero §13, che chiede di non fare del Basic Attack la sua fonte
> principale di DPS. Oggi lo è per costruzione.

> **Formulazione originale (2026-08-09), superata:** la matrice dichiarava «potenza immediata»
> alta / medio-bassa / bassa / molto bassa rispettivamente per Vektor, Flux, Riva, Bastion. **Tre valori su
> quattro** contraddicono il catalogo canonico. Registrata qui invece che cancellata, secondo la regola del §2.

Questa tabella è **falsificabile**: ogni riga dice se il lavoro esiste e quale. La direzione di design
(le quattro famiglie, §5–§8) resta approvata e invariata — cambia solo il suo aggancio ai dati reali.

> Regola operativa che sostituisce «prototipi da validare»: **nessuna sezione da §10 a §13 autorizza una
> modifica di dato che non sia esplicitamente marcata `CHANGE` o `DECISION REQUIRED` qui sopra.**

---

# 10. VEKTOR — BASIC ATTACK COME PRIMARY WEAPON

## Direzione

Vektor deve essere il personaggio per cui il giocatore può pensare:

> “Non devo per forza usare una skill speciale. Se creo una buona linea, il Basic Attack è già una vera minaccia.”

### Nome — REJECTED

Il nome canonico è **`Vektor.PulseShot`**, già spedito in codice, catalogo eroi, roadmap, showcase,
`docs/gameplay/brief-conoscenza-parziale.md` e negli scenari `.json` che invocano l'abilità **per stringa**.

> **Proposta originale, respinta:** `Vector Shot`. Rinominare costerebbe un rename cross-repo su sei
> documenti più i file di scenario eseguibili, e violerebbe il gate `[ ] no stale roster names reintroduced`
> del §38 di questo stesso documento.

### Stato canonico

```text
Vektor.PulseShot
Range:    4
Damage:   21        # non la fascia generica (a range 4 darebbe 22): -1 pagato in mobilita'
Cooldown: 0
Fallback: Cancel
Effects:  solo danno
```

### Il problema aperto: «Primary Weapon» non è oggi distinguibile

Con 21 danni a range 4, `PulseShot` è il **terzo** attacco base del roster per danno e **non** il più lungo
(Riva arriva a 5). Nessun dato lo rende «l'arma primaria»: la famiglia è oggi un'etichetta senza correlato
osservabile, né da un test né da un giocatore.

Le tre strade sono alternative, non cumulabili:

| Opzione | Cosa cambia | Costo | Rischio |
|---|---|---|---|
| **A — nessun cambiamento** | «Primary» descrive la *frequenza d'uso*, non i numeri: Vektor usa spesso il base perché il suo kit crea linee | zero | l'etichetta resta non verificabile da un test |
| **B — payoff condizionale** | un bonus quando il bersaglio è in una geometria dichiarata (linea controllata, esposto dopo setup) | serve un consumer runtime della condizione | è una feature con dipendenze, non un numero |
| **C — delta numerico** | portare `PulseShot` sopra gli altri attacchi base | modifica di balance sul roster | tocca test e scenari che asseriscono 21 |

> ✅ **DECISO — opzione A** (2026-08-09, [ADR-0007](docs/decisions/adr-0007-attacco-base-per-eroe.md)).
> `PulseShot` resta 21 / r4: «Primary» è un **profilo d'uso**, non una promessa di potenza. B non è respinta,
> è rinviata: richiede un consumer runtime della condizione, quindi un checkpoint proprio.

La famiglia va quindi riformulata al §14 come profilo d'uso. B resterebbe la versione più interessante del
design, ma alzare il danno (C) contraddirebbe la motivazione già scritta nel catalogo («-1 pagato in
mobilità»).

Se si sceglie B, il payoff va scelto **uno solo** — e nessuno dei quattro ha oggi un consumer:

```text
Precision · IgnoreLightCover · Mark · Focus/Momentum
```

### Moving target

Il framework **ha già** una policy di moving target, esercitata dallo scenario
`Visual.Combat.FallbackTargetMoved`. Non introdurne una nuova: se serve un comportamento diverso per
`PulseShot`, va dichiarato come **dato dell'azione** (`ERTActionFallback`), non come policy parallela.

> **Proposta originale:** `LockCell`. Non è un valore di `ERTActionFallback`: il fallback che `PulseShot`
> dichiara oggi è `Cancel`. Va verificato contro l'enum reale prima di essere citato.

---

# 11. FLUX — BASIC ATTACK COME ENGINE ATTACK

## Direzione

Flux può infliggere meno danno immediato, ma usare molto il Basic Attack perché alimenta la Signature.

### Nome — REJECTED

Il nome canonico è **`Flux.ArcPulse`**. Vale la stessa ragione del §10.

> **Proposta originale, respinta:** `Charge Bolt`.

### Stato canonico

```text
Flux.ArcPulse
Range:    4
Damage:   22        # fascia "medio raggio" di URTCatalogLibrary::MakeBasicAttack(4)
Cooldown: 0
Effects:  solo danno
```

È l'**unico** attacco base del roster che deriva i suoi numeri dalla tabella a fasce condivisa; gli altri tre
sono dell'eroe. Nota che 22 è il **secondo valore più alto** del roster: la formulazione originale lo dava
per «medio-bassa potenza immediata».

### `Charged` non esiste — la famiglia Engine è BLOCKED

```text
status:     BLOCKED
blocked_by: RT-FEAT-ENV-ELECTRIC
```

Verificato su `main` @ `ea26c0f`:

- **nessuno stato `Charged`** esiste nel codice. L'unica occorrenza della stringa è `bChargedIntoTarget`, un
  bool locale della carica di *movimento* (`Source/RefactorTactics/Turn/RTTurnManager.cpp:1621`);
- **nessun modello di conduttività di cella** esiste. `Flux.ConductiveNode` è stato spedito con `Effects`
  **vuoto**, e la motivazione è scritta nel codice: non si dichiara un effetto che nessuno legge
  (`Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp:157-165`).

Quindi «Apply Charged 1» **non è un payload da configurare**: è `RT-FEAT-ENV-ELECTRIC` sotto un altro nome.
Aggiungerlo come dato riprodurrebbe esattamente il difetto che `ConductiveNode` ha scelto di evitare.

### Scelta richiesta per la v0.1

| Opzione | Effetto |
|---|---|
| **A — Flux resta damage-only in v0.1** | la famiglia Engine slitta; il registry dichiara `Engine (payload pending)`; nessuno scenario nuovo |
| **B — il Charge entra come checkpoint proprio** | dipendenza esplicita da `RT-FEAT-ENV-ELECTRIC`: prima lo stato e il suo consumer, poi il payload dell'attacco base |

> ✅ **DECISO — opzione A** (2026-08-09, [ADR-0007](docs/decisions/adr-0007-attacco-base-per-eroe.md)).
> `ArcPulse` resta 22 / r4 damage-only. La famiglia Engine **slitta**: il Charge è una feature ambientale con
> un costo che va deciso a parte, non un campo dell'attacco base.

> ⚠️ Finché vale A, **non pubblicare «Flux = Engine Attack» in Wiki o Feature Registry**: sarebbe uno stato
> che il codice non sostiene, cioè la seconda fonte di verità che il §31.4 vieta.

Backlog, non v0.1 — nessuno di questi bersagli ha oggi una semantica di targeting sull'attacco base:

```text
conductive cell · device · conductive cover · node · water/wet network
```

### Il pattern desiderato esiste già, ma passa da un'altra azione

La coordinazione acqua+elettricità **è eseguibile oggi**, e il suo motore è `Flux.LinearDischarge` (+8 su
bersaglio `Wet`, via `URTCombatLibrary::FluxWetDischargeBonus`), non l'attacco base. Scenari reali:
`Visual.Combat.WaterElectricCoordinated` e `Visual.Combat.WaterElectric`. Vincolo di fase: vedi §12.

### Pattern desiderato — obiettivo, non scope v0.1

```text
Flux Basic
  ->
Charged target / charged node        # BLOCKED: nessuno stato Charged esiste
  ->
Riva modifies Wet/Water
  ->
electric payoff / overload / chain
```

Resta la direzione giusta: il Basic Attack di Flux va valutato come **setup engine**, non come `low damage
shot`. Ma nella v0.1 la catena sopra **non è eseguibile in nessun punto**, perché il secondo anello non
esiste. Chi legge questa sezione come una specifica da implementare costruirebbe il dato prima del consumer.

---

# 12. RIVA — BASIC ATTACK COME SETUP ATTACK

## Direzione

Riva deve poter usare il Basic Attack pensando:

> “Dove voglio creare la condizione utile?”

più che:

> “A chi tolgo più HP?”

### Nome — REJECTED

Il nome canonico è **`Riva.PressureJet`**.

> **Proposta originale, respinta:** `Water Pulse`.

### Stato canonico — già conforme

```text
Riva.PressureJet
Range:    5
Damage:   16        # nessuna fascia generica: l'attacco base di Riva e' TEMATICO
Cooldown: 0
Shape:    Line
Fallback: AttackCell
Effects:
  Damage 16
  Status.Wet (1 turno)
  Push 1
```

**Riva è l'unico eroe del roster già conforme a questo documento.** Il Setup Attack descritto al §7 è
spedito: danno basso, stato applicato, spinta, forma a linea, fallback su cella.

```text
Delta richiesto: NESSUNO
```

Questo è il riferimento contro cui misurare gli altri tre: mostra che la tesi del documento è
implementabile con le primitive esistenti, senza un sistema per eroe.

### Multi-target semantics — fuori scope v0.1

Far accettare allo stesso attacco base tre semantiche di bersaglio (unità / cella / cella in fiamme) richiede
targeting per cella sull'azione base, una UI che distingua i tre casi, e un resolver che scelga il ramo.
**Nessuno dei tre esiste oggi per l'attacco base.**

`PressureJet` ha già forma a linea e applica `Wet` **all'unità**. Estendere a «crea o rinforza `Wet` sulla
cella» appartiene a `RT-FEAT-ENV-WATER`, non a questo consolidamento:

```text
backlog: PressureJet su cella          -> crea / rinforza Wet
backlog: PressureJet su cella in fiamme -> attenua / spegne Fire
```

### Vincolo di timing da rispettare — D-036

Il `Wet` di `PressureJet` **dura 1 turno e `TickStatuses` lo rimuove nel Cleanup dello stesso turno in cui è
stato applicato**: fra un turno e il successivo non sopravvive. D-036 ha scelto l'**ordine** invece della
durata, quindi la coordinazione acqua+elettricità si fa **dentro lo stesso Blast**, dove `PressureJet`
(priorità 50) risolve prima di `LinearDischarge` (55).

> ⚠️ Qualunque scenario, testo di Wiki o esempio che descriva «Riva bagna, il turno dopo Flux scarica» è
> **sbagliato**: è il difetto già corretto in `#242`, che aveva reso la combo firma della v0.1 non eseguibile
> in nessuna forma. Le due forme che funzionano hanno entrambe uno scenario — vedi §27.

### Pattern squadra — verificato, con il nome reale dell'anello elettrico

```text
Riva.PressureJet     -> danno 16 + Wet          (attacco base)
Flux.LinearDischarge -> 24 + 8 sul bagnato      (NON l'attacco base di Flux)
                        stesso Blast, priorita' 50 prima di 55
```

La combo è coerente con l'identità sistemica acqua + elettricità **ed è già eseguibile**: la prova è
`Visual.Combat.WaterElectricCoordinated`, che asserisce `100 − 16 − 32 = 52` sul bersaglio.

> Nota che l'anello elettrico è una **abilità fondamentale**, non l'attacco base di Flux. È la dimostrazione
> più pulita della tesi di questo documento — un attacco base a basso danno che vale per ciò che prepara — e
> vale **oggi**, senza il Charge del §11.

---

# 13. BASTION — BASIC ATTACK COME UTILITY / EMERGENCY

## Direzione

Bastion non dovrebbe vincere una partita facendo spam del Basic Attack.

La sua identità principale deve restare:

- struttura;
- protezione;
- controllo archi;
- Brace / reaction;
- cover;
- choke.

### Nome — REJECTED

Il nome canonico è **`Bastion.ImpactShot`**.

> **Proposta originale, respinta:** `Kinetic Bash`.

### Stato canonico — contraddice la famiglia assegnata

```text
Bastion.ImpactShot
Range:    3
Damage:   24        # il PIU' ALTO del roster
Cooldown: 0
Fallback: Cancel
Effects:  solo danno
```

La §9 originale classificava Bastion «potenza immediata molto bassa». È l'opposto: `ImpactShot` è l'attacco
base **più forte dei quattro**, e non porta alcun effetto di utilità.

Questa sezione chiede che il Basic Attack «non diventi la sua fonte principale di DPS». Con 24 danni, costo
zero e nessun cooldown, oggi **lo è per costruzione**.

### ✅ DECISO — opzione A, cambiano i numeri

Decisione del 2026-08-09, registrata in [ADR-0007](docs/decisions/adr-0007-attacco-base-per-eroe.md).

```text
Bastion.ImpactShot
Range:    3          # invariato
Damage:   24 -> 8
Cooldown: 0          # invariato
Fallback: Cancel     # invariato
Effects:
  Damage 8
  Status.Slow (1 turno)     <- la utility scelta
```

**Perché `Slow` e non le altre.** Delle cinque utility candidate, **tre non sono esprimibili** con le
primitive esistenti:

| Candidata | Verdetto |
|---|---|
| + danno contro cover leggera | ❌ `ERTStructureOp` ha solo `None / CreateCover / MoveCover`: nessuna operazione che danneggi una copertura |
| rimozione di oggetto fragile | ❌ stessa ragione |
| generazione di Guard | ❌ applicherebbe lo stato a **sé**, ma `ERTActionEffect::Status` lo applica al **bersaglio** |
| Stagger | ⚠️ non esiste come stato — `Status.Slow` è il suo equivalente reale |
| displacement condizionale | ✅ esprimibile (`Push`/`Pull`), ma `Push` è già la firma di Riva |

`Status.Slow` ha un consumer verificato (`MoveCostModifier`, `RTTurnManager.cpp:3261`) e chiude un cerchio
tematico: la debolezza dichiarata di Bastion è `Affinity.Movement`, quindi il suo attacco base diventa la sua
piccola risposta a chi si muove di mestiere.

**Perché 8.** È ancorato a un numero già canonico invece che scelto dentro la forbice: il §9 colloca Riva
(16) un gradino sopra Bastion, e 8 ne è la metà esatta. Lascia possibile il *finish*, che è l'uso dichiarato
della famiglia — con 6 il finish diventerebbe raro, cioè la falsa scelta che il §22 vuole evitare.

> ⚠️ **Costo accettato, non effetto imprevisto.** 24/r3 era un numero **motivato**: la fascia corto raggio
> darebbe 25, e il catalogo ne toglie uno «in cambio della stazza»
> (`Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp:326-329`). Inoltre Bastion è già l'eroe più
> incompleto del roster — solo `ImpactShot` e `Ram` sono interamente rappresentabili. Portarlo a 8 lo lascia
> con poco che funzioni finché le sue altre azioni non hanno un consumer. L'alternativa B (riclassificare la
> famiglia) è stata scartata perché avrebbe lasciato Utility/Emergency **senza alcun rappresentante in v0.1**.

**Da rimisurare, non da adattare finché passano** — 4 scenari e 2 file di test citano `Bastion.ImpactShot`:

```text
Scenarios/Combat/CounterStrikesBack.json
Scenarios/Combat/NoCounterWhenUnarmed.json
Scenarios/Visual/Combat/Defeat.json
Scenarios/Visual/Map/LowCoverEdge.json
Source/RefactorTactics/Tests/RTHeroBastionTests.cpp
Source/RefactorTactics/Tests/RTHeroReactionTests.cpp
```

Uso desiderato che la scelta rende possibile: finish · risposta affidabile · emergenza. La *rimozione di un
oggetto fragile* esce dagli usi dichiarati: non ha primitiva.

---

# 14. BASIC ATTACK DEPENDENCY / “MASTERY”

Durante il focus è stata proposta una metrica qualitativa:

| Personaggio | Basic dependency |
|---|---:|
| Vektor | ★★★★★ |
| Flux | ★★★★☆ |
| Riva | ★★★☆☆ |
| Bastion | ★☆☆☆☆ |

Interpretazione:

> NON è una statistica di potenza.

Significa:

- quanto spesso il Basic Attack entra nel normale gameplay del personaggio;
- quanto è centrale nel suo decision making;
- quanto è integrato con la Signature.

Esempio:

- Flux può avere molte stelle pur facendo poco danno;
- Vektor molte stelle perché il Basic è un'arma primaria;
- Riva media dipendenza per setup;
- Bastion bassa dipendenza perché normalmente preferisce controllo/protezione.

Usare questa metrica come:

- documentazione;
- UX di onboarding;
- matrice di character design;
- playtest rubric;

NON introdurla automaticamente come runtime stat.

Nome consigliato interno al documento:

```text
BasicAttackUsageProfile
```

o equivalente già presente.

---

# 15. BASIC ATTACK ≠ BASIC ACTION

Chiarire la terminologia.

Player-facing:

```text
Generic / Basic Actions
  - Wait
  - Move
  - Interact
  - Brace
  - Overwatch
  - Basic Attack
```

Ma:

> **Basic Attack è una categoria universale; il contenuto concreto dell'attacco è character-specific.**

Quindi evitare di modellare:

```text
BasicAttack = stessa pistola per tutti con danno +/- 3
```

Preferire:

```text
Action.BasicAttack
    +
Character Basic Attack Profile / Definition
    =
Resolved Basic Attack
```

ATTENZIONE:

NON decidere in anticipo se tecnicamente debba essere:

- `URTAbilityDefinition`;
- `URTActionDefinition`;
- campo di `URTCharacterDefinition`;
- riferimento a un ActionId;
- wrapper GAS.

Verificare prima l'architettura corrente e riusare la fonte di verità esistente.

> **Nota di verifica (2026-08-09).** Nessuno dei cinque nomi elencati sopra esiste nel progetto.
> I tipi reali sono **`URTActionData`**, **`URTHeroData`**, **`URTEquipmentData`** (pin di canone: nessun GAS
> nella v0.1). Il §15-bis chiude la domanda che questa sezione lascia aperta.

---

# 15-bis. COME SI RICONOSCE L'ATTACCO BASE A RUNTIME

Questa è la vera domanda architetturale del consolidamento, e nessuna delle 41 sezioni originali la poneva.

## Architettura reale — diversa da quella descritta al §15

Il §15 propone `Action.BasicAttack + Character Profile = Resolved Basic Attack`. Il codice **non fa così**:

- gli eroi **non** usano `Action.BasicAttack` come `ActionId`. Usano un ID proprio (`Flux.ArcPulse`,
  `Riva.PressureJet`, `Bastion.ImpactShot`, `Vektor.PulseShot`);
- `Action.BasicAttack` esiste nel catalogo core, ma **senza danno né portata**, e con una motivazione scritta:
  quei due numeri dipendono dall'eroe, «mettere qui un numero significherebbe sceglierne uno arbitrario per
  tutti» (`Source/RefactorTactics/Ability/RTCatalogLibrary.cpp:311-316`);
- solo Flux deriva i suoi numeri dalla tabella a fasce (`MakeBasicAttack(4)`). Gli altri tre sono definiti
  a mano, ciascuno con la sua motivazione nel commento.

Non esiste quindi un *profilo sopra un'azione condivisa*: esiste **un'azione per eroe che facoltativamente
riusa una tabella di fasce**. È un modello più semplice di quello proposto, e già allineato al §16
(«non creare `FluxBasicAttackSystem`…»): va recepito, non sostituito.

## Il buco

L'attacco base è oggi identificato da una **convenzione posizionale**: `URTHeroData::Actions[0]`, dichiarata
nel commento del tipo (`Source/RefactorTactics/Ability/RTHeroData.h:19-21`) — indice 0 l'attacco base, indici
1-4 le quattro abilità fondamentali.

Funziona benissimo finché nessuno deve **interrogare** quel ruolo. Ma tre richieste di questo documento lo
interrogano:

```text
§14  BasicAttackUsageProfile      -> serve una chiave per eroe
§28.2 "resolved from stable character/action data" -> serve qualcosa di piu' di una posizione
§29  BasicAttackPickRate, ...     -> serve una chiave di aggregazione
```

Su cosa si aggrega `BasicAttackPickRate`, se «essere l'attacco base» non è un dato ma un indice?

## Le tre opzioni

| | Opzione | Costo | Quando è giusta |
|---|---|---|---|
| **A** | **Tenere la convenzione `Actions[0]`**, documentarla come contratto e aggiungere **un test** che la faccia valere per tutti e quattro gli eroi | ~1 test | finché il ruolo serve solo alla documentazione |
| **B** | **Campo esplicito di ruolo** su `URTActionData` (enum `ERTActionRole` o flag) | dato + validator + migrazione dei quattro asset | quando esiste un consumer runtime reale (metriche, UI, tutorial) |
| **C** | Convenzione sul nome dell'`ActionId` | zero | mai: i quattro ID non condividono alcun pattern (`ArcPulse`, `PressureJet`, `ImpactShot`, `PulseShot`) |

## Raccomandazione

**A adesso, B quando arriva il primo consumer.** È esattamente la regola che il §16 di questo documento si
dà da solo — *«aggiungere un campo al modello solo quando esiste un caso reale che ne ha bisogno»* — e non
c'è ragione di violarla proprio qui.

Concretamente, la tranche v0.1 aggiunge **un solo test**:

```text
RefactorTactics.Heroes.BasicAttackIsIndexZeroForEveryHero
  per ogni eroe del roster:
    Actions.Num() == 5
    Actions[0] esiste, ha ResolutionPhase Attack e Cooldown 0
    Actions[0] dichiara almeno un effetto Damage
```

Senza questo test la convenzione è un commento, e §14/§28.2/§29 non sono implementabili.

---

# 16. MODELLO DATA-DRIVEN CONCETTUALE

Il sistema deve poter descrivere un Basic Attack con proprietà come:

```text
Stable Attack/Profile ID
Version

Targeting
Range
Shape

Damage
DamageType

Requirements
LOS
Cover / trajectory policy

MovingTargetPolicy

Effects
Status application
Resource gain / spend
Surface interaction
Cover interaction

FacingPolicy
NoiseProfile

Cooldown
Charges
Reload / recovery policy

TurnLog / reason metadata
```

Non tutti i campi devono essere presenti nella v0.1.

Regola:

> aggiungere un campo al modello solo quando esiste un caso reale che ne ha bisogno.

Preferire primitive condivise già esistenti:

```text
Damage
Status
Resource
SurfaceChange
CoverEffect
FacingEffect
NoiseEvent
Cooldown
Charge
```

Non creare:

```text
FluxBasicAttackSystem
RivaBasicAttackSystem
BastionBasicAttackSystem
VektorBasicAttackSystem
```

---

# 17. BASIC ATTACK E COOLDOWN / CHARGES

Decisione di design generale:

> Non è obbligatorio che tutti i Basic Attack futuri siano “infinite use, zero cooldown”.

Il framework dovrebbe poter supportare in futuro:

```text
Heavy Basic
  30 damage
  cooldown 1 turn

Weapon
  2 charges
  shot
  shot
  reload

Combo Basic
  every third hit -> empowered

Support Basic
  0 damage
  mark target
```

Questi sono esempi di **possibilità future**, non scope obbligatorio v0.1.

Regola di design:

> Se un Basic Attack ha cooldown, reload o limite di charge, il personaggio deve avere comunque decisioni interessanti quando non è disponibile.

Evitare turni morti.

Non implementare oggi un sistema Reload/Combo Counter se nessun personaggio corrente lo richiede.

---

# 18. INTERAZIONI SISTEMICHE DA SUPPORTARE COME GRAMMATICA

Un Basic Attack può potenzialmente:

```text
deal damage
generate resource
consume resource
apply status
remove status
change Facing
prepare Reaction
reduce cooldown
mark unit/cell
interact with terrain
interact with cover
generate noise
modify a Signature Mechanic
```

Questo elenco descrive **capacità del framework**, non il payload di ogni personaggio.

La regola fondamentale è:

> l'identità del Basic Attack deve emergere combinando poche primitive leggibili.

---

# 19. FACING

Integrare con il sistema Facing corrente.

Il Basic Attack può avere una `FacingPolicy` equivalente alle policy già previste per le azioni.

Esempi concettuali:

```text
KeepFacing
FaceTarget
FaceAttackDirection
LimitedTurn(N)
```

Usare i nomi reali del progetto.

TurnLog deve poter spiegare:

```text
FacingBefore
FacingChangeReason
FacingAfter
```

solo se il modello corrente registra già questi dettagli o se sono necessari all'explainability.

Dipendenza:

```text
Basic Attack
  -> E16 / Facing
```

se l'azione cambia orientamento o la validità dipende dall'orientamento.

---

# 20. RUMORE

Il Basic Attack deve poter produrre un Noise Event / Noise Profile quando il sistema acustico è attivo.

Esempi qualitativi:

- Vektor: arma evidente / forte firma;
- Flux: scarica elettrica;
- Riva: getto meno rumoroso;
- Bastion: impatto cinetico.

NON inventare valori di Noise se non sono già approvati.

Feature relation:

```text
Basic Attack
  -> Noise Generation
  -> Team Knowledge / Perception
```

Se Noise/TeamKnowledge non sono runtime nella milestone corrente:

- mantenere la relazione nella Feature Map;
- scenario `BLOCKED` / `FUTURE`;
- non bloccare il Basic Attack MVP.

---

# 21. COVER / TERRAIN / ENVIRONMENT

Il Basic Attack può interagire con:

```text
Cover
Water
Wet
Conductive
Fire
Objects / devices
```

ma NON deve aggirare servizi canonici.

Richiesto:

```text
BasicAttack intent
  ->
snapshot
  ->
targeting / trajectory / LOS
  ->
resolver
  ->
effects
  ->
environment / cover mutation
  ->
TurnLog
```

Vietato:

```text
if Character == Riva:
    SetCellWet()

if Character == Bastion:
    DestroyCover()
```

dentro TurnManager o codice orchestration.

---

# 22. FALSE CHOICE TEST

Aggiungere una regola di design/playtest:

> Un Basic Attack con danno basso deve avere almeno una situazione ripetibile in cui è una scelta sensata.

Per ogni personaggio v0.1 verificare:

```text
When is Basic Attack correct?
When is it inferior to a Signature Ability?
What resource/opportunity does it save?
What counterplay exists?
What does the player learn from using it?
```

Se non esiste una risposta concreta, il design è una falsa scelta.

Questo è un test di **design/playtest**, non necessariamente un assert C++.

---

# 23. PRODUCT MAP

Aggiornare la Product Map nel punto reale in cui vive.

Inserire/collegare una capability equivalente a:

```text
Character Identity
  ->
Base Action Signature
      ->
Basic Attack Profiles
```

Messaggio prodotto:

> I personaggi non differiscono soltanto per quattro skill speciali. Anche le azioni universali contribuiscono all'identità del kit.

Il Product Map deve linkare:

```text
Generic Actions
Character Identity
Basic Attack
Signature Mechanics
Environment Interaction
Facing
Noise / Perception
Scenario Validation
```

Non trasformare Product Map in una tabella di numeri di balance.

---

# 24. FEATURE MAP / FEATURE REGISTRY

## 24.0 Stato verificato del registry

Dal 2026-08-08 la fonte canonica è **`docs/roadmap/feature-registry.yaml`**. `.json`, `.md` e la pagina
`docs/wiki/feature-status.md` sono **generati**: non si modificano a mano. Il validator è
`scripts/feature_registry.py`, e impone tre cose che questa sezione originariamente ignorava:

```text
GATE_VALUES  = done | partial | todo | na          # "na" conta come soddisfatto, "partial" mai
STATUS_ORDER = IDEA DESIGNED SPECIFIED IMPLEMENTING TESTABLE INTEGRATED RELEASE_READY DONE
STATUS_OFF_SCALE = DEFERRED | BLOCKED              # dichiarano una decisione, non un grado di completezza
```

> **`status` è DERIVATO dai gate** (`derive_status`) e il validator verifica la coerenza. Non si sceglie a
> mano: si compilano i gate e si accetta ciò che ne esce.

Feature esistenti pertinenti — **verificate**, non supposte:

```text
RT-FEAT-ACTION-GENERIC          esiste
RT-FEAT-ACTION-ENGINE           esiste
RT-FEAT-ACTION-MOVE-PROFILES    esiste, status RELEASE_READY   <- il precedente da copiare
RT-FEAT-CHAR-V01-ROSTER         esiste
RT-FEAT-CORE-DETERMINISM        esiste
RT-FEAT-MAP-LOS                 esiste
RT-FEAT-MAP-FACING              esiste
RT-FEAT-ENV-WATER               esiste
RT-FEAT-ENV-ELECTRIC            esiste
```

**ID citati che NON esistono** (0 occorrenze nel registry): `RT-FEAT-MAP-TARGETING`,
`RT-FEAT-MAP-TRAJECTORY`. Le feature `RT-FEAT-MAP-*` reali sono HEXGRAPH, PATHFINDING, LOS, FACING, COVER,
DYNAMIC-COVER, INTERACTIVE-EDGES, SPECIAL-TRANSITIONS, HIGH-GROUND. **Non crearle**: sono un artefatto della
stesura, non una lacuna del registry.

## 24.1 La scelta 24.2-vs-24.3 è già fatta dal precedente

`RT-FEAT-ACTION-MOVE-PROFILES` («Profili di movimento — Move, Sprint, Charge») è nato dallo stesso tipo di
consolidamento, è `RELEASE_READY`, e ha esattamente la forma che serve qui. Quindi:

- **niente `RT-FEAT-CHAR-BASE-ACTION-SIGNATURE`**: sarebbe una gerarchia nuova sopra sette azioni, cioè
  proprio la gerarchia parallela che il §24.2 originale voleva evitare;
- **sì a `RT-FEAT-ACTION-BASIC-ATTACK-PROFILES`**, gemello di MOVE-PROFILES, stessa area `Actions`, stesso
  `kind: gameplay`, stessa epic `E4`.

## 24.2 Voce da aggiungere — bozza da verificare prima del commit

```yaml
  - feature_id: RT-FEAT-ACTION-BASIC-ATTACK-PROFILES
    title: Attacchi base caratterizzati per eroe
    area: Actions
    kind: gameplay
    release: v0.1
    priority: P1
    status: <DERIVATO dai gate — non scriverlo a mano>
    gates:
      spec: todo             # done quando questo documento e' recepito in docs/gameplay
      data: done             # i quattro attacchi base esistono nel catalogo eroi
      runtime: done          # risolvono dal resolver condiviso, senza branch per eroe
      log_debug: todo        # da misurare: il TurnLog distingue l'attacco base?
      automation: partial    # esistono i test di catalogo; manca il ruolo (§15-bis, §28)
      scenario: partial      # Combat.BasicAttack esiste; mancano i casi per famiglia
      ui_wiki: todo
      packaged: todo
      network_privacy: na    # v0.1 offline-first
    roadmap:
      epic: E4
    dependencies:
      - RT-FEAT-ACTION-ENGINE
      - RT-FEAT-ACTION-GENERIC
      - RT-FEAT-CHAR-V01-ROSTER
      - RT-FEAT-CORE-DETERMINISM
      - RT-FEAT-MAP-LOS
      - RT-FEAT-ENV-WATER        # solo per Riva.PressureJet (Wet)
    owner_specs:
      - <la spec gameplay dove questo documento viene recepito>
    issues: [<numeri GitHub reali — non "E4">]
    tests:
      - RefactorTactics.Heroes.BasicAttackByRangeBand
      - <i nuovi test del §28>
    scenarios:
      - Combat.BasicAttack
      - <i nuovi scenari del §27>
    wiki_refs:
      - docs/wiki/game/<pagina reale>
    last_verified:
      date: <data>
      commit: <sha>
    notes: >
      Categoria universale, payload dell'eroe. Riva e' l'unico gia' conforme; Flux e' BLOCKED da
      RT-FEAT-ENV-ELECTRIC (nessuno stato Charged esiste); Bastion ha una decisione aperta sui numeri.
```

**`RT-FEAT-ENV-ELECTRIC` non va messa fra le dipendenze** finché vale l'opzione A del §11: una dipendenza
dichiarata e non usata è essa stessa un dato senza consumatore.

## 24.3 Cosa la feature deve dichiarare

```text
categoria d'azione universale
payload caratterizzato per eroe (dato, non codice)
resolver condiviso e deterministico
policy di moving target dichiarata dal dato
copertura scenario per ciascuna famiglia rappresentata
copertura Wiki
```

## 24.4 Gate

Vale il vocabolario del §24.0. Due regole che il documento originale non esplicitava:

- **`na` è una risposta, non un buco** — `network_privacy: na` è corretto in v0.1 offline-first;
- **`partial` non è mai soddisfatto**: è il modo in cui una feature dichiara di non aver finito.

Non marcare `DONE` finché i gate applicabili non sono verificati, e **non scrivere `status` a mano**:
eseguire il validator e rigenerare le viste.

---

# 25. ROADMAP

Non riaprire automaticamente E4 se l'Action Engine generic è già chiusa.

La roadmap deve distinguere:

## Framework

```text
Generic Basic Attack action already available?
    -> verify

Character-specific profile/payload support?
    -> verify

Shared effect primitives?
    -> verify

Validation / TurnLog?
    -> verify
```

## v0.1

Obiettivo minimo:

```text
Vektor -> Primary Weapon
Flux    -> Engine Attack
Riva    -> Setup Attack
Bastion -> Utility/Emergency
```

con almeno un comportamento riconoscibile per ciascuno.

## v0.2+

Estensione tramite nuovi character profile senza nuovo action engine.

## Future

Solo se realmente necessario:

```text
reload
ammo/charges
combo basic
empowered basic
multi-mode basic
advanced noise signature
```

---

# 26. EPIC / ISSUE RELATIONS

## Regola primaria

PRIMA:

- leggere Epic/Issue reali;
- cercare duplicate;
- verificare stato;
- verificare milestone;
- verificare parent/child.

NON inventare numeri GitHub.

## 26.1 Epic relation desiderata

Il Basic Attack deve essere collegato almeno concettualmente a:

```text
E4  Generic Action Engine
  ->
Character Base Action Signature / Character Profiles
  ->
v0.1 character prototypes
  ->
E15 Showcase integration
```

Dipendenze trasversali possibili:

```text
E8  Environment
E9  Cover / Structures
E13 Team Knowledge / Perception
E16 Facing
```

NON rendere E13 blocker del Basic Attack core se serve soltanto per Noise.

## 26.2 Se esiste già una Epic Character / Ability / Base Actions

Aggiornarla.

NON crearne una nuova.

## 26.3 Se manca un contenitore appropriato

Epic candidata:

```text
Character Base Action Signatures
```

Obiettivo:

> Rendere le azioni universali character-specific tramite profili data-driven, preservando una grammatica comune e un resolver condiviso.

Possibili child issue:

```text
1. Basic Attack profile framework / data integration
2. Vektor Primary Weapon prototype
3. Flux Engine Attack prototype
4. Riva Setup Attack prototype
5. Bastion Utility Attack prototype
6. Basic Attack scenario + automation coverage
7. Wiki / onboarding / explainability
8. Balance + false-choice playtest
```

Se le issue hero esistono già:

- aggiornare quelle;
- NON crearne quattro nuove per duplicare il lavoro.

## 26.4 Acceptance criteria delle issue

Ogni issue deve contenere:

```text
Goal
Scope
Non-goals
Dependencies
Data changes
Runtime changes
TurnLog/debug
Scenario
Automation
Wiki/docs
Packaged gate
Target release
```

---

# 27. SCENARIO MAP / SCENARIO REGISTRY

## 27.0 Convenzione reale — nessuna delle categorie proposte esisteva

Lo `scenarioId` **rispecchia il percorso su disco**: `Scenarios/<Percorso>/<Nome>.json` → `<Percorso>.<Nome>`.

```text
Scenarios/Combat/BasicAttack.json                  -> Combat.BasicAttack
Scenarios/Spec/Facing/FrontAttackKeepsGuard.json   -> Spec.Facing.FrontAttackKeepsGuard
Scenarios/Visual/Combat/WaterElectric.json         -> Visual.Combat.WaterElectric
```

Radici reali: `Combat/`, `Movement/`, `Spec/<Cat>`, `Visual/<Cat>`. **Non esistono** le categorie `Actions`,
`Characters`, `Team Combos`, `Regression`. Documenti owner: `docs/technical/scenario-index-e-tag.md` (come si
identifica uno scenario) e `docs/technical/scenario-map.md` (chi lo verifica: macchina, occhio umano, o
nessuno).

> **Vincolo che riscrive metà di questa sezione.** Il runner supporta **cinque** tipi di assertion, e basta
> (`Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp:348-400`):
>
> ```text
> UnitAtCell · UnitHpEquals · UnitAlive · UnitFacing · TurnsCompleted
> ```
>
> **Non esiste `UnitHasStatus`.** Uno stato applicato — `Wet`, `Marked`, qualunque — **non è osservabile
> direttamente**: si dimostra solo con un *delta di HP che vale soltanto se lo stato è stato applicato*.
> È il metodo che gli scenari reali già usano: `Spec.Facing.FrontAttackKeepsGuard` prova la guardia con
> `120 - (22 - 15) = 113`.

## 27.1 Baseline — ESISTE GIÀ, non crearlo

`Combat.BasicAttack` è spedito: Flux colpisce Bastion a distanza 2, `UnitHpEquals 98` (120 − 22), e il suo
`_nota` dichiara di essere legato ai numeri del catalogo eroi — se qualcuno li cambia senza aggiornare il
catalogo, diventa rosso.

> **Proposta originale, respinta:** `Actions.BasicAttack.Baseline` — sarebbe un duplicato.

Delta utile su questo scenario: **nessuno**. Se serve coprire LOS bloccata o moving target, esistono già
`Combat.BlockedByWall` e `Visual.Combat.FallbackTargetMoved`.

## 27.2 Riva — il Setup Attack è GIÀ dimostrato

`Visual.Combat.WaterElectricCoordinated` esegue esattamente ciò che il §27.4 e il §27.6 originali chiedevano,
**nello stesso turno** come impone D-036:

```text
intents:  R1 Riva.PressureJet   -> V1
          F1 Flux.LinearDischarge -> V1
expect:   UnitHpEquals V1 = 52
```

Il conto è la prova del `Wet`: `100 − 16 (PressureJet) − 32 (24 + 8 sul bagnato) = 52`. Senza il `Wet` il
risultato sarebbe 60. **Lo scenario fallisce se il setup non avviene** — è un oracolo vero, non una
descrizione.

> **Proposte originali, respinte perché duplicate:** `Character.Riva.BasicAttack.WetSetup` e
> `Team.Conflux.FluxRiva.BasicAttackSetup`.

## 27.3 Flux — BLOCKED, e va dichiarato tale

```text
scenario:   Character.Flux.BasicAttack.ChargeSetup
status:     BLOCKED
blocked_by: RT-FEAT-ENV-ELECTRIC   (nessuno stato `Charged` esiste — §11)
```

Non implementare hack: la §27.3 originale aveva già ragione su questo punto, e resta invariata.

## 27.4 Bastion — dipende dalla decisione del §13

Non è scrivibile finché la decisione A/B del §13 è aperta:

- **se B** (cambia la famiglia): nessuno scenario nuovo, `Combat.BasicAttack` copre già il colpo pesante;
- **se A** (cambia i numeri): serve uno scenario in cui l'attacco base a basso danno è la scelta corretta —
  e l'unico caso esprimibile con i cinque tipi di assertion disponibili è il **finish**:

```text
bersaglio a pochi HP  ->  ImpactShot  ->  UnitAlive = false
```

«rimuovere una cover leggera» e «applicare un piccolo controllo» **non sono asseribili oggi**: non esiste un
tipo di expectation per lo stato della copertura né per uno status.

## 27.5 Vektor — dipende dalla decisione del §10

Con l'opzione A (raccomandata) non c'è nulla da dimostrare che `Combat.BasicAttack` non dimostri già.
Con l'opzione B, lo scenario diventa una **coppia** sul modello di `Spec.Facing.*`:

```text
Spec.<Cat>.VektorLineUpRewarded    geometria favorevole  -> HP atteso col payoff
Spec.<Cat>.VektorLineUpDenied      stessa scena, angolo rotto -> HP atteso senza payoff
```

Una sola metà della coppia non prova niente: un'implementazione che applica **sempre** il bonus passerebbe la
prima. È la lezione già scritta nel `_nota` di `Spec.Facing.FrontAttackKeepsGuard`.

## 27.6 Interaction scenarios

Non creare fixture nuove per Facing, Cover, MovingTarget, Environment e Determinism: **esistono già** come
corpus (`Spec/Facing/`, `Spec/Cover/`, `Spec/Environment/`, `Visual/Combat/FallbackTargetMoved`). Se
l'attacco base deve entrarci, si **estende** un caso esistente, non se ne apre un decimo quasi identico.

`Actions.BasicAttack.Noise` resta `D — dichiarato` finché la percezione non è runtime (§20).

---

# 28. TEST AUTOMATICI

Convenzione reale dei nomi: `RefactorTactics.<Area>.<Nome>` — es. `RefactorTactics.Heroes.StatsFromData`,
`RefactorTactics.Actions.Sprint`.

## 28.0 Copertura già esistente — non riscriverla

```text
RefactorTactics.Heroes.BasicAttackByRangeBand        la fascia danno/portata
RefactorTactics.Heroes.ValidateHeroesStructure       struttura del roster
RefactorTactics.Heroes.StatsFromData                 statistiche dal dato
RefactorTactics.Heroes.ExactlyOneVariantPerHero      una sola abilita' con varianti
RefactorTactics.Heroes.MobilityWithoutDamageIsNotMain
```

Più il corpus scenario del §27. Il §28 aggiunge **solo** ciò che manca.

## 28.1 Il difetto della formulazione originale

> **Assert originale, respinto:** `Flux BasicAttack != Riva BasicAttack payload`.
>
> **Passa già oggi**, su `main`, prima che venga scritta una riga: 22 danni contro 16 + `Wet` + `Push`.
> Un test verde prima della feature non misura la feature — misura che il repository non è vuoto.
>
> Peggio: resterebbe verde anche **cancellando il `Wet` da `PressureJet`**, perché `22 ≠ 16` basta a
> soddisfarlo. Il test non protegge la cosa che questo documento dichiara di voler proteggere.

Regola che lo sostituisce:

> **Asserire il RUOLO, non la disuguaglianza.** Ogni test deve poter fallire per una ragione sola e nominabile.

## 28.2 Ruolo e struttura

```text
RefactorTactics.Heroes.BasicAttackIsIndexZeroForEveryHero
  per ogni eroe: Actions.Num() == 5
                 Actions[0].ResolutionPhase == Attack
                 Actions[0].CooldownTurns == 0
                 Actions[0] dichiara almeno un effetto Damage

RefactorTactics.Heroes.BasicAttackIdsAreUniqueAndStable
  i quattro ActionId sono distinti e non derivati dal DisplayName
```

Il primo è il test che il §15-bis richiede: senza, la convenzione `Actions[0]` resta un commento.

## 28.3 Payload per famiglia — un assert per identità

Uno per eroe, ciascuno falsificabile da solo:

```text
RefactorTactics.Heroes.RivaBasicAttackAppliesWet
  Riva.PressureJet dichiara Status.Wet, durata 1, e forma Line

RefactorTactics.Heroes.BastionBasicAttackDeclaresNoSideEffect
  Bastion.ImpactShot dichiara SOLO Damage        # da riscrivere se il §13 sceglie A

RefactorTactics.Heroes.FluxBasicAttackUsesRangeBand
  Flux.ArcPulse == MakeBasicAttack(4)            # gia' coperto da BasicAttackByRangeBand: verificare
                                                 # se estenderlo invece di duplicarlo
```

**Vektor non ha un test di famiglia** finché la decisione del §10 è aperta: scriverne uno significherebbe
canonizzare l'opzione B senza averla scelta.

## 28.4 Verifica di mutazione — obbligatoria prima del commit

Un test nuovo non vale finché non si è visto **fallire per la ragione giusta**. Rompere una mutazione per
volta e controllare che cadano **esattamente** i test attesi:

```text
togliere Status.Wet da PressureJet
  -> RivaBasicAttackAppliesWet                    rosso
  -> Visual.Combat.WaterElectricCoordinated       rosso (52 diventa 60)
  -> tutto il resto                                verde

cambiare ImpactShot da 24 a 6
  -> BastionBasicAttackDeclaresNoSideEffect       verde   <- corretto: non asserisce il numero
  -> gli scenari che asseriscono HP               rossi
```

Una mutazione che non fa cadere nulla è un segnale: o il test è debole, o la build non è arrivata al binario.
Confrontare il totale di `Test Completed` con la suite misurata sul branch: una run **parziale** somiglia a
un esito valido e non lo è.

## 28.5 Data validation

`ValidateHeroes` esiste già. Estensioni pertinenti, **solo per campi che esistono**:

```text
ogni eroe ha Actions[0] valido
Range e CooldownTurns non negativi
gli Status referenziati esistono come tag reale
```

Non asserire `MovingTargetPolicy`, `Charges`, `Reload`, `NoiseProfile`: **non sono campi del modello**
(§16 — un campo entra quando esiste il caso che lo richiede, e un test non è quel caso).

## 28.6 Determinismo

Coperto da `RT-FEAT-CORE-DETERMINISM` e dal replay hash già esistente. Non introdurre un impianto nuovo:
aggiungere l'attacco base al corpus già esercitato, se non c'è.

## 28.7 Packaged

Smoke test sul percorso reale `Intent → Planning → Snapshot → Resolver → TurnLog`, riusando
`Combat.BasicAttack`. Il gate `packaged` resta `todo` finché la run non è stata **eseguita e misurata**, non
dedotta.

---

# 29. PLAYTEST / BALANCE TESTS

Aggiungere una mini matrice di playtest.

Per ciascun personaggio:

```text
How often is Basic Attack selected?
Why?
Was it correct?
Was the tactical value understood?
Did the player mistake low damage for uselessness?
Did the Basic Attack overshadow Signature Abilities?
Did Signature Abilities make the Basic Attack irrelevant?
```

Metriche candidate:

```text
BasicAttackPickRate
BasicAttackDamageContribution
BasicAttackSetupContribution
BasicAttackResourceContribution
BasicAttackFollowupConversion
BasicAttackWhiffRate
```

Non trasformare automaticamente queste metriche in telemetry production.

Possono essere inizialmente raccolte da Scenario Harness / TurnLog / playtest notes.

---

# 30. “NO FALSE CHOICE” ACCEPTANCE

Per v0.1 documentare almeno una situazione:

| Character | Basic Attack correct when... | Not correct when... |
|---|---|---|
| Vektor | ha una linea/occasione favorevole | deve creare/prevedere una geometria migliore |
| Flux | deve generare setup/Charge | serve payoff immediato o controllo più forte |
| Riva | serve Wet/setup | serve danno immediato |
| Bastion | serve utility/fallback/finish | può ottenere più valore da struttura/protezione |

Questa tabella va raffinata con il payload effettivamente implementato.

---

# 31. WIKI

Aggiornare la Wiki reale senza creare duplicati.

## 31.1 Pagina Basic Attack

Deve spiegare:

```text
What it is
Why it is universal
Why it differs by character
Primary / Engine / Setup / Utility patterns
Relation with cooldown/resources
Relation with Facing
Relation with environment
Relation with Noise
Moving target behavior
Counterplay
Related scenarios
Related feature IDs
Roadmap status
```

## 31.2 Generic Actions

Aggiungere link:

```text
Generic Actions
  -> Basic Attack
  -> Character Base Action Signature
```

## 31.3 Character pages

Per Flux / Riva / Bastion / Vektor aggiungere una sezione equivalente a:

```text
Basic Attack
- tactical role
- targeting
- range
- immediate effect
- setup/resource effect
- environmental interaction
- facing/noise notes
- best use cases
- common mistake
- related scenarios
- feature / roadmap status
```

Non pubblicare numeri candidati come finali se il catalogo non li ha approvati.

## 31.4 Cross-links

Wiki:

```text
Character
  <-> Basic Attack
  <-> Signature Mechanic
  <-> Feature
  <-> Scenario
  <-> Roadmap
```

La Wiki NON deve diventare una seconda fonte dello status.

---

# 32. DOCUMENTAZIONE DA AGGIORNARE

Verificare e aggiornare almeno:

```text
piano-canonico-mvp
Action Catalog
Hero Catalog
character design/spec
generic action spec
ability/action data model docs
Feature Registry
Roadmap
Scenario Registry
Showcase
Decision Log
Open Decisions
Wiki
```

Aggiornare il conflict matrix se esistono vecchie affermazioni come:

```text
Basic Attack = same generic low-damage shot for every hero
```

oppure:

```text
Basic actions are identical between characters
```

Se queste frasi non esistono, non inventare un conflitto.

---

# 33. DECISION LOG / ADR

✅ **FATTO** — registrata come **[ADR-0007 — Attacco base: categoria universale, payload
dell'eroe](docs/decisions/adr-0007-attacco-base-per-eroe.md)** (2026-08-09), indicizzata in
`docs/README.md`.

L'ADR **non** apre un framing nuovo: chiude per `BasicAttack` i «profili concreti dei 4 eroi» che **D-033**
aveva lasciato esplicitamente aperti, e ne usa il vocabolario — il modificatore di un'azione generica si
chiama **`profilo`**, non se ne conia un secondo nome.

La sintesi qui sotto resta come traccia della formulazione originale.

## Decisione

```text
Basic Attack is a universal action category with character-specific profile/payload.
Its primary purpose can be damage, engine generation, setup, or utility.
Character identity must remain visible through Basic Attack without requiring hero-specific resolver branches.
```

## Conseguenze

```text
+ stronger identity during cooldown/resource downtime
+ better data-driven roster differentiation
+ reusable generic action language
+ systemic interactions with environment/resources

- more balance surface
- requires clear Wiki/UI explanation
- character profile data must be validated
```

**Decision ID ancora da confermare.** L'ADR porta `D-041` come segnaposto: il log arriva a `D-040`, ma
l'ID vero si prende **al merge**, non ora. Il progetto ha già avuto **cinque** collisioni di contatore
(`D-028`/`D-029`, `D-037`/`D-038`, `E21`/`E21`, `D-039`/`D-039`, `D-039`→`D-040`), tutte fra sessioni
parallele: assegnare adesso significherebbe candidarsi alla sesta. Lo stesso vale per il numero `0007`
dell'ADR, da riverificare prima del commit.

---

# 34. IMPLEMENTATION STRATEGY

Usare la soluzione più semplice compatibile con il codice attuale.

Ordine consigliato:

```text
1. Audit current BasicAttack implementation
2. Identify actual source of BasicAttack data
3. Make profile/payload character-specific using existing definition system
4. Keep generic intent/action path unchanged where possible
5. Add/extend resolver primitives only for real missing effects
6. Add TurnLog/reason
7. Add v0.1 data
8. Add scenarios/tests
9. Update Wiki/docs/registry
10. Showcase integration only after isolated scenarios pass
```

Target:

> **riusare E4**, non riscrivere E4.

---

# 35. NON-GOALS v0.1

Non introdurre automaticamente:

```text
full ammo system
reload animation authority
weapon inventory
random crit framework
complex combo counter
per-shot heat system
large Basic Attack talent tree
network replication redesign
GAS authority over resolver
hero-specific TurnManager branches
```

Se una di queste emerge come reale dipendenza, creare issue separata e motivata.

---

# 36. RELATION CON GAS

Preservare il confine corrente:

```text
GAS
-> ownership / costs / cooldowns / tags / effects mirror

Resolver
-> authoritative outcome from snapshot
```

Se Basic Attack usa cooldown/charges:

- GAS può rappresentare/mettere in UI lo stato;
- il resolver/autorevole data model deve restare fonte dell'esito competitivo secondo l'architettura vigente.

Non spostare la decisione dell'impatto nel montage o timing di animazione.

---

# 37. NETWORK / PRIVACY

Il focus v0.1 può restare offline-first secondo roadmap corrente.

Quando entra multiplayer:

```text
BasicAttack intent
```

segue le stesse regole di privacy degli altri intenti:

- team-only planning preview;
- enemy client non riceve target/path/attack planning;
- server valida;
- risultato diventa pubblico quando risolto secondo la policy di percezione.

Non creare un canale rete specifico per Basic Attack.

---

# 38. DEFINITION OF DONE

La feature Basic Attack Profile non è `DONE` soltanto perché quattro Data Asset hanno danni diversi.

Gate applicabili:

```text
[ ] spec
[ ] data
[ ] runtime
[ ] targeting / moving target
[ ] TurnLog + reason
[ ] automation
[ ] scenario
[ ] Wiki
[ ] Feature Registry / Roadmap
[ ] packaged
[ ] network/privacy when applicable
```

Inoltre:

```text
[ ] no hero-specific branch in TurnManager
[ ] no duplicate feature/registry source
[ ] no stale roster names reintroduced
[ ] no candidate balance values published as canonical by mistake
[ ] each v0.1 Basic Attack has a clear tactical reason to exist
```

---

# 39. REPORT FINALE RICHIESTO A CLAUDE

Al termine produrre:

```text
1. HEAD before / after
2. Files modified
3. Canonical decisions added
4. Conflicts found
5. BasicAttack architecture found in code
6. Reused existing systems
7. New runtime code, if any
8. Data changes
9. Feature Registry changes
10. Roadmap changes
11. Product Map changes
12. Scenario Map changes
13. Wiki changes
14. Epic / Issue updated
15. Epic / Issue created
16. Dependency relations added
17. Tests added/updated
18. Test results
19. Packaged verification
20. Open decisions
21. Follow-up backlog
```

Per Epic/Issue riportare URL/numero reale.

Non usare `E4`, `E15`, ecc. come se fossero automaticamente GitHub Issue number.

---

# 40. COMMIT SEQUENCE CONSIGLIATA

Adattare al repository reale.

```text
docs(actions): define character-specific basic attack roles
data(characters): configure v0.1 basic attack profiles
feat(actions): resolve character basic attack profiles
test(actions): add basic attack scenario and deterministic coverage
docs(wiki): link basic attacks to characters and scenarios
chore(roadmap): align basic attack feature and epic relations
```

Se non serve codice:

```text
docs(gameplay): consolidate basic attack character signatures
```

può essere sufficiente per la prima tranche.

---

# 41. RISULTATO ATTESO

Al termine il progetto deve esprimere chiaramente:

```text
BASIC ATTACK
    |
    +-- universal action language
    |
    +-- character-specific profile
            |
            +-- Vektor: Primary Weapon
            +-- Flux: Engine Attack
            +-- Riva: Setup Attack
            +-- Bastion: Utility / Emergency
    |
    +-- shared resolver primitives
    |
    +-- data-driven configuration
    |
    +-- TurnLog / tests / scenarios
    |
    +-- Wiki / Feature / Roadmap traceability
```

Principio finale da preservare:

> **Un personaggio di RefactorTactics deve continuare a essere riconoscibile anche quando non usa una delle sue skill speciali.**

E, nello specifico:

> **Il Basic Attack può essere forte, debole, preparatorio o situazionale: ciò che non deve mai essere è una scelta priva di identità o una falsa scelta senza ragione tattica.**
