# Brief — Stati del personaggio e trasformazioni

> `CURRENT` · **Stato**: brief di scope · **Data**: 2026-08-08 · **Release**: 📅 **post-v0.1**, epic **E34**
> **Decisione abilitante**: [D-035](../decisions/RT_PDR_00_Decision_Log.md)
> **Origine**: [`../archive/src/design/trasformazioni-e-stati-personaggio.md`](../archive/src/design/trasformazioni-e-stati-personaggio.md)
> (24 §, `RESEARCH` — **input, non autorità**)
> **Autorità**: subordinato al canone, ad [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) per le fasi e
> ad [ADR-0006](../decisions/adr-0006-ownership-abilita-sinergie.md) per l'ownership dei kit.
> **Nessuna implementazione**: questo brief fissa la forma e i vincoli, non apre lavoro.

## 1. Perché un brief, se non entra nella v0.1

Perché la domanda è già arrivata due volte da direzioni diverse — la trasformazione come meccanica identitaria,
e lo *stance* come modo di caratterizzare un eroe senza aggiungergli un'abilità — e senza un owner la seconda
verrebbe costruita come caso particolare della prima, o viceversa.

La tesi del documento sorgente, che questo brief adotta:

> **Una buona trasformazione deve aumentare le decisioni strategiche più di quanto aumenti le informazioni da
> ricordare.**

È un criterio di accettazione, non uno slogan: se una forma non cambia *quale domanda si pone il giocatore*,
sta pagando carico cognitivo per un bonus numerico.

## 2. Fuori dalla v0.1 — e perché

Il sorgente (§14) proponeva **quattro** stati sui quattro eroi della vertical slice. La proposta è respinta:

- lo scope della v0.1 è chiuso — il totale sta in [`../roadmap/roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) §3 — e il rischio di scope è già registrato come **alto**
  in [`../roadmap/roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) §8;
- due dei quattro stati proposti **dipendono da sistemi non ancora costruiti**: `Charged` dal canale ambientale
  e acustico di **E13**, `Bulwark` dal sistema strutture che oggi regge solo in parte `KineticPanel`;
- il terzo, `Wraith: Mobile ↔ Siege`, **contraddice l'eroe**: `Slancio` recupera muovendosi e la player
  question è «dove passerà il nemico?». Una forma che toglie il Dash non sospende una statistica, spegne la
  meccanica firma.

Se e quando il tema si apre, i banchi di prova coerenti sono **Howitzer** e **Murdock**, il cui kit è già
costruito sul trade-off mobilità/precisione, e **GRIM.exe**, che è modulare per concezione.

## 3. Un'infrastruttura, cinque presentazioni

Non si costruisce un sistema chiamato `Transformation`. Si costruisce uno **stato del personaggio**, e lo si
presenta in modi diversi perché il giocatore percepisca sistemi diversi:

| Famiglia | Cos'è | Peso indicativo |
|---|---|---:|
| `Stance` | Postura commutabile, leggera e leggibile — *Guard ↔ Assault* | 2–4 / 10 |
| `Form` | Cambio sostanziale di comportamento — *Mobile ↔ Siege* | 4–7 / 10 |
| `Overdrive` | Potenziamento forte ma **temporaneo**, spesso a soglia | 4–7 / 10 |
| `Environmental` | Stato causato o alimentato dal terreno — *Charged*, *Frostbound* | 3–7 / 10 |
| `Configuration` | Riconfigurazione tecnologica, **una attiva alla volta** — *Kernel Override* | 3–6 / 10 |

I pesi sono **indicativi e non misurati**: servono a confrontare due proposte, non a validarne una.

### Complexity budget

Il sorgente (§3) tratta la complessità di un eroe come un budget finito, e la regola che ne discende è l'unica
parte davvero vincolante: **un personaggio con una trasformazione importante deve essere più semplice
altrove.** Trasformazione *più* summon *più* trappole *più* risorse a stack sullo stesso eroe è il modo
documentato di rendere un personaggio ingiocabile senza che nessuna singola scelta sia sbagliata.

## 4. Il ponte con ciò che esiste già: lo *Stance* è un profilo commutabile

Il repository ha già il meccanismo «stesso comando, comportamento diverso per eroe»: il **profilo** di azione
generica ([`brief-azioni-generiche-overwatch.md`](brief-azioni-generiche-overwatch.md) §4-bis,
[D-033](../decisions/RT_PDR_00_Decision_Log.md)). Oggi un profilo è **fisso per eroe**.

```text
profilo fisso per eroe          →  identità
profilo commutabile in partita  →  Stance
```

È la via più economica per la prima metà del framework: `Stance` e `Configuration` non chiedono un sistema
nuovo, chiedono di rendere il profilo **selezionabile in Planning**. Solo `Form`, `Overdrive` ed
`Environmental` richiedono davvero override di abilità e movimento.

Chi costruirà E34 dovrebbe partire da lì: se il primo prototipo è un cambio di profilo, valida metà del
framework senza toccare il kit.

## 5. Regole di design

1. **Lo stato è sempre visibile.** HUD, icona, silhouette, colore, e soprattutto **Action Ghost**: la forma
   deve essere leggibile in Planning, non scoperta in Resolution
   ([`../technical/systems/brief-planning-visuale.md`](../technical/systems/brief-planning-visuale.md)).
2. **Serve commitment.** Se `A → B → A` è gratuito ogni turno, il sistema è micro-ottimizzazione: costo in
   `Prep`, durata minima, cooldown o costo di risorsa.
3. **Le fasi non si piegano.** Dichiarazione in Planning, risoluzione in `Prep`, effetti sulle fasi successive.
   La sequenza resta `Planning → Prep → Dash → Blast → Move → Cleanup`, e **il `Cleanup` è dove scade** una
   forma a durata fissa — la stessa fase in cui Riktor recupera `Integrità Strutturale`.
4. **Niente GAS** ([D-005](../decisions/RT_PDR_00_Decision_Log.md)): i dati vivono in
   `URTActionData`/`URTHeroData`/`URTEquipmentData`.
5. **Niente branch per eroe** nel core (ADR-0006). Uno stato può sostituire le abilità **del proprio**
   personaggio; non può introdurre quelle di un altro.
6. **Spiegabile nel TurnLog**: quale stato era attivo e quale regola ha modificato, non solo l'esito.

## 6. Anti-pattern

| Anti-pattern | Perché fallisce |
|---|---|
| `Transform → +20% danno` | Costo di sistema alto, decisione nuova zero |
| Doppio kit completo (4+4 skill, 2 passive, 2 reaction) | Raddoppia le informazioni da ricordare su un solo eroe |
| Toggle gratuito ogni turno | Senza commitment diventa aritmetica, non lettura |
| Stato poco visibile | Il giocatore non deve **ricordare** in che configurazione è un'unità |
| Trasformazioni per tutti | Il framework può essere comune; la meccanica **percepita** non deve esserlo |

## 7. Come si misura, se si costruisce

Il sorgente (§23) propone metriche osservabili negli scenari: tempo aggiuntivo di Planning · frequenza di
Transform/Revert · percentuale di turni in cui una forma domina tutte le alternative · azioni annullate perché
incompatibili con lo stato · utilizzo reale delle abilità mutate.

Il segnale che conta è qualitativo, e i due esiti sono distinguibili:

- ✅ «devo scegliere quale modalità mi conviene» — la forma ha aggiunto una decisione;
- ❌ «non ricordo cosa cambia in questa modalità» — ha aggiunto solo informazioni.

## 8. Cosa resta aperto

> 📊 **Le candidature vivono in [`../characters/matrici-stati-personaggio.md`](../characters/matrici-stati-personaggio.md)**:
> tutte le alternative L/M/S per ~40 personaggi, il costo sistemico di ognuna, il budget di complessità e lo
> stato di validazione. Nessuna riga supera oggi `PROPOSED`.

Nessun eroe ha uno stato assegnato: le proposte del sorgente (§7–§13) restano **alternative di design**, tre per
personaggio, deliberatamente non risolte. Una Signature scartata come trasformazione può tornare come ability,
come passiva o come profilo — ed è il motivo per cui il sorgente non è stato potato.

Non decisi, e da non dedurre: quali famiglie entrano davvero · quanti stati contemporanei la partita tollera ·
se la trasformazione possa essere una `Fast Reaction` (§17 del sorgente: possibile, «da usare con moderazione»,
e comunque dipendente da **E14**) · il costo in action economy.

## 9. Scenari di validazione — definiti, non scritti

Cinque scenari, **da scrivere quando E34 apre**: oggi descriverebbero un sistema che non esiste, e uno scenario
che non gira è peggio di uno che manca — sembra copertura. Gli ID seguono la convenzione del corpus
([`../technical/runbooks/scenari-validazione-visiva.md`](../technical/runbooks/scenari-validazione-visiva.md)).

| ID | Cosa valida | Perché è questo, e non un altro |
|---|---|---|
| `State.Phase.Flow` | Trigger su sequenza, stato leggero, nessun override | **Il primo da scrivere.** È l'unico che non tocca cover, LOS, collisione o pathing: se fallisce, il difetto è nel framework, non nell'integrazione |
| `State.Gadget.Charged` | Trigger ambientale, mutazione di skill, interazione con `Wet` | Valida la pipeline `ambiente → stato → abilità → ambiente`. Il caso interessante è che il bonus legge `Status.Wet` **senza sapere chi l'ha applicato** ([D-029](../decisions/RT_PDR_00_Decision_Log.md)) |
| `State.Riktor.Bulwark` | Pseudo-cover, LOS, protezione dell'alleato, pathing | Il più invasivo: quattro sistemi condivisi in un colpo solo. Va scritto **dopo** i due leggeri |
| `State.Howitzer.Siege` | Alternate Form completa: override movimento + skill, revert a costo di `Prep` | Il banco di prova di `Mobile ↔ Siege`. **Non Wraith**: vedi §2 |
| `State.MultiState.Stress` | Più unità in stati diversi nello stesso turno | Il solo che può rompere il determinismo. Deve dimostrare **permutazione-invarianza**: stessi stati, ordine di inserimento diverso, `TurnLog` e hash identici |

Nessun bypass del gameplay: percorso reale `Intent → Planning → Snapshot → Resolver → TurnLog`, niente
`SetActorLocation` né scorciatoie. Le verifiche interattive corrispondenti sono le voci `PIE-STATE-*` di
[`../technical/test-manuali-pie.md`](../technical/test-manuali-pie.md).

## 10. Logging — cosa deve poter ricostruire

Uno stato deterministico ma **non spiegabile** è un mezzo risultato: il replay torna uguale e nessuno sa
perché. Il TurnLog deve registrare le transizioni con gli stessi reason code che già usa, non con un canale
parallelo.

Gli eventi minimi, da nominare secondo le convenzioni del TurnLog esistente
([`../technical/architecture/spec-turnlog.md`](../technical/architecture/spec-turnlog.md)):

```text
StateRequested · StateActivated · StateRejected
StateExpired   · StateReverted
StateAbilityOverrideApplied · StateMovementOverrideApplied
StateEnvironmentTrigger
```

Ogni voce deve bastare a ricostruire, **da sola**: personaggio · stato precedente · stato nuovo · trigger ·
fase · round · motivo · bersaglio o cella rilevante.

Due vincoli che non sono dettagli:

1. **`StateRejected` è obbligatorio quanto `StateActivated`.** Una transizione rifiutata in silenzio è il
   difetto che rende impossibile diagnosticare un playtest: il giocatore vede che «non è successo niente».
2. **L'hash del TurnLog resta permutazione-invariante.** Le voci di stato entrano nel log *come dato*, con la
   stessa disciplina delle Fast Decision — altrimenti due partite identiche con ordine di inserimento diverso
   producono hash diversi, e il KPI `replay divergence 0` cade.

## 11. Terminologia — e la parola già occupata

```text
Character State
├── Stance          postura commutabile, leggera
├── Form            cambio sostanziale di comportamento
├── Overdrive       potenziamento forte e temporaneo
├── Environmental   causato o alimentato dal terreno
└── Configuration   riconfigurazione, una attiva alla volta
```

⚠️ **«Stato» è già preso.** Il progetto lo usa per gli **stati temporanei** — `Status.Wet`, `Status.Marked`,
`Burning` — che sono un'altra cosa: appartengono alla cella o al bersaglio, li applica qualcun altro, e il
giocatore non li sceglie. Un *Character State* è invece una **configurazione scelta dal proprio giocatore**.

La distinzione non è accademica: è esattamente il caso di
[D-029](../decisions/RT_PDR_00_Decision_Log.md), dove `Wet` non sa chi l'ha applicato. Se i due concetti
scivolano in una parola sola, la prima implementazione avrà ragione a metterli nello stesso sistema — e sarà
sbagliata.

Nei documenti, quindi: **stato del personaggio** o `Character State` per questo framework; **stato temporaneo**
per `Status.*`, di cui l'owner resta [`spec-stati-temporanei-cp82.md`](spec-stati-temporanei-cp82.md).
Il glossario player-facing **non** cambia: non contiene ancora il framework, e non deve — nessuna riga supera
`PROPOSED`.

## 12. Rapporto con gli altri documenti

| Documento | Relazione |
|---|---|
| [`brief-azioni-generiche-overwatch.md`](brief-azioni-generiche-overwatch.md) | §4-bis: il **profilo**, di cui lo `Stance` è la versione commutabile |
| [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) | **Prevale** sull'ordine delle fasi: nessuno stato lo altera |
| [ADR-0006](../decisions/adr-0006-ownership-abilita-sinergie.md) | **Prevale** sull'ownership: nessuna abilità condivisa fra eroi |
| [`brief-unita-ausiliarie.md`](brief-unita-ausiliarie.md) | Il caso «summon + trasformazione sullo stesso eroe» è esattamente ciò che il complexity budget vieta |
| [`spec-stati-temporanei-cp82.md`](spec-stati-temporanei-cp82.md) | Owner degli **stati temporanei di gioco** (`Wet`, `Marked`): sono un'altra cosa — stati della cella o del bersaglio, non configurazioni scelte dal giocatore |
