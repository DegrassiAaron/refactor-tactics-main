# Spec — L'attacco base per eroe

> `CURRENT` · **Owner** della semantica dell'attacco base. **Data**: 2026-09-13.
> **Nasce da una promessa dichiarata**: l'intestazione di
> [`ADR-0007`](../decisions/adr-0007-attacco-base-per-eroe.md) dice **«Owner spec: da creare
> (`../gameplay/spec-attacco-base-per-eroe.md`)»** dal 2026-08-09, e il file non esisteva. Il contenuto
> arriva dalla specifica consolidata della skill bar del 2026-09-13, consumata dallo spec panel in
> [`../roadmap/plans/skill-bar-consolidamento-spec-panel-2026-09-13.md`](../roadmap/plans/skill-bar-consolidamento-spec-panel-2026-09-13.md).
>
> ⚠️ **Questa pagina descrive due stati diversi, e li tiene separati.** §1–§4 sono **in vigore**: il codice
> le esegue. §5 è **decisa e non implementata** — nessuna riga di codice la esprime oggi, e il repository fa
> tuttora il contrario. Chi legge §5 come descrizione del comportamento corrente la legge male.
>
> 🔁 **Aggiornata il 2026-09-14, dopo l'istruttoria sui punti aperti.** Delle tre voci di
> [`D-415`](../decisions/RT_PDR_00_Decision_Log.md), la §5.1 è stata **ritirata**
> ([`D-417`](../decisions/RT_PDR_00_Decision_Log.md)), la §5.2 **precisata**
> ([`D-419`](../decisions/RT_PDR_00_Decision_Log.md)) e la §5.3 confermata con il prezzo del mortaio
> **riqualificato** ([`D-418`](../decisions/RT_PDR_00_Decision_Log.md)).
>
> **Decisioni**: [`D-058`](../decisions/RT_PDR_00_Decision_Log.md) (ADR-0007) ·
> [`D-096`](../decisions/RT_PDR_00_Decision_Log.md) (nessun RNG) ·
> [`D-221`](../decisions/RT_PDR_00_Decision_Log.md) (`bCountsAsAttack`) ·
> [`D-415`](../decisions/RT_PDR_00_Decision_Log.md) (§5) ·
> [`D-417`](../decisions/RT_PDR_00_Decision_Log.md) (§5.1 ritirata) ·
> [`D-418`](../decisions/RT_PDR_00_Decision_Log.md) (§5.3) ·
> [`D-419`](../decisions/RT_PDR_00_Decision_Log.md) (§5.2).
> **Issue**: [#315](https://github.com/DegrassiAaron/refactor-tactics-main/issues/315).

## Perché esiste

`BasicAttack` è una delle sette azioni generiche di
[`D-025`](../decisions/RT_PDR_00_Decision_Log.md), e `ADR-0007` ne ha deciso l'**ownership** — categoria
universale, payload dell'eroe — senza descriverne il comportamento. La conseguenza si vedeva: chi cercava
*«la mira insegue il bersaglio?»* trovava la risposta solo leggendo `RTHexCombatLibrary.cpp`, cioè trattando
il codice come specifica. Questa pagina è la sede che mancava.

## 1. Identità e costo

| Proprietà | Regola |
|---|---|
| Appartenenza | **Dell'eroe**, non di una tabella generica. `ADR-0007` §6: il ruolo è la convenzione posizionale `URTHeroData::Actions[0]` |
| Costo | **Gratuito**: nessuna risorsa, nessun cooldown |
| Slot | Azione **principale** (`ERTActionSlot::Main`) |
| Fase | **Sempre** il Blast (`ERTResolutionPhase::Attack`) |
| Dichiarazione di aggressione | `bCountsAsAttack = true` ([`D-221`](../decisions/RT_PDR_00_Decision_Log.md)) |

⚠️ **Non esiste un `Action.BasicAttack` giocato**: i quattro attacchi base del roster hanno `ActionId`
propri, ed è ADR-0007 §7 a registrarlo. La voce di catalogo `Action.BasicAttack` è la **categoria**, e il
suo danno per fascia di portata lo assegna `MakeBasicAttack`.

## 2. Determinismo

**Danno ed effetti sono deterministici.** Non esiste probabilità casuale di mancare, e non esistono critici
casuali.

Non è una scelta di questa pagina: è [`D-096`](../decisions/RT_PDR_00_Decision_Log.md) — *«nel progetto
**non esiste un RNG**»* — e vale per tutta la simulazione. Un attacco base che sbagliasse *a volte* romperebbe
il replay prima ancora del bilanciamento.

## 3. Geometria e impatti

- **Ostacoli, intercettazione da altre unità, traiettoria e area dipendono dal tipo di attacco**, e sono un
  **dato** e non un ramo di codice: `ERTAbilityShape` più `ERTLineResolution`
  ([`D-386`](../decisions/RT_PDR_00_Decision_Log.md)). `None` perfora, `StopAtFirstTarget` si ferma sul primo.
- Uno **spostamento forzato prima del Blast cambia l'origine del colpo**: portata, linea di tiro, strutture e
  footprint si valutano dalla posizione dell'attaccante **al Blast**.
- Se un controllo blocca l'attacco, il **movimento pianificato resta eseguibile** se legalmente consentito e
  non bloccato a sua volta: gli slot sono indipendenti.

## 4. Fuoco amico

| | |
|---|---|
| Attivo? | **Sì, a danno pieno.** `FRTActionDef::bFriendlyFire = true` è il **default**, non l'eccezione di una singola abilità |
| L'alleato è selezionabile? | **No, normalmente.** `ValidateInstance` rifiuta ogni bersaglio-unità della stessa squadra con `ERTActionInvalidReason::TargetFriendly` |
| Allora come lo si colpisce? | **Mirando a un esagono**, e secondo la geometria e gli impatti dell'attacco |

⛔ **L'eccezione — un attacco che ammette esplicitamente alleati come bersaglio — non è esprimibile oggi**:
il rifiuto è incondizionato e non consulta `bFriendlyFire`. Misurato: **0** occorrenze di
`bAllowsAllyTarget`, `AllyTargetable`, `bCanTargetAllies` in `Source/`. È una lacuna dichiarata, non un
divieto: il primo kit che ne abbia bisogno la apre.

## 5. 🔴 Deciso e NON implementato — [`D-415`](../decisions/RT_PDR_00_Decision_Log.md)

⛔ **Il codice fa oggi il contrario di quelle che restano**, e nessuna delle righe qui sotto descrive il
comportamento corrente.

✅ **Le due code che `D-415` apriva sono chiuse**: `SKB-4` da [`D-417`](../decisions/RT_PDR_00_Decision_Log.md)
(la §5.1 si ritira) e `SKB-5` da [`D-418`](../decisions/RT_PDR_00_Decision_Log.md) (il mortaio non si
riprezza). Restano **due** voci da implementare, non tre.

### 5.1 ~~L'attacco base non dipende dall'equipaggiamento~~ — ✅ **RITIRATA il 2026-09-14**

⏱️ *Questa sezione dichiarava «decisa e non implementata» la regola per cui il payload dell'attacco base è
dell'eroe e l'arma non lo modifica.* 🔁 **[`D-417`](../decisions/RT_PDR_00_Decision_Log.md) ritira il punto
(2) di [`D-415`](../decisions/RT_PDR_00_Decision_Log.md)**: l'attacco base **continua a dipendere dall'arma
equipaggiata**, e `URTCatalogLibrary::EquipWeaponVariant(Abilities[0], Piece)`
(`Source/RefactorTactics/Unit/RTUnit.cpp:1661`) resta il canale. La catena `D-086`…`D-100` **non è
superata**.

🔑 **Non è stata ritirata per costo: è stata ritirata perché la domanda che apriva ha ricevuto una risposta
diversa da quella che si aspettava.** `SKB-4` chiedeva *su cosa* agiscano le varianti staccate dall'attacco
base; la misura ha risposto che **la ragione per cui esistono non è esercitata da nessuno**: la loro
giustificazione è la **scelta orizzontale** ([`D-086`](../decisions/RT_PDR_00_Decision_Log.md)), e nessuna
UI di loadout la offre — `SelectLoadout`, `LoadoutWidget`, `EquipmentWidget`, `PreMatch` danno **zero**.
Spostarle avrebbe costruito un meccanismo per una scelta che il giocatore non compie.

⚠️ **La scelta orizzontale resta senza sede.** Se un'UI di loadout arriverà, `SKB-4` si riapre con una
domanda diversa: non «dove metterle» ma «quante e quali».

✅ **Le altre due voci di `D-415` restano accettate**: §5.2 (la mira, ora precisata da
[`D-419`](../decisions/RT_PDR_00_Decision_Log.md)) e §5.3 (il tiro alla cieca, con il prezzo del mortaio
riqualificato da [`D-418`](../decisions/RT_PDR_00_Decision_Log.md)).

### 5.2 La mira è fissata in pianificazione e non insegue

**Regola decisa**: la mira si fissa in Planning. Se il bersaglio si sposta prima del Blast, **non viene
seguito**. Può ancora subire un impatto se la nuova posizione ricade nella traiettoria o nell'area effettiva
— e questo è **già vero** oggi.

🔁 **Precisata il 2026-09-14 da [`D-419`](../decisions/RT_PDR_00_Decision_Log.md): è una regola UNIVERSALE**,
non una proprietà dichiarata per azione. Un'azione che voglia agganciare lo dichiara, e oggi nessuna lo fa.

⏱️ *Questa sezione diceva «oggi insegue, e per una riga sola: `RTHexCombatLibrary.cpp:352`».* 🔴 **La misura
dice altro, ed è la correzione che conta.** Il Blast **precede** il Move (`RTTurnRules.cpp:7-12`),
`HexUnits[].Cell` nasce a inizio Blast e nessuno la riscrive prima di `CollectHexAttacks`: quella riga e
`Intent.TargetCell` portano oggi **lo stesso valore**. Toccarla costa **34 test rossi** e cambia **zero
esiti**. L'inseguimento vero vive in **`RTTurnManager_Blast.cpp:770`**, e **il piano non ha da dove leggere una cella
di pianificazione nel ramo a bersaglio-unità**: `ARTUnit::PlannedAttackCell` esiste, ma la scrive solo
`DeclareAttackOnCell`.

∴ **il costo non è una riga: è un campo di schema del piano**, più due voci del corpus golden da rigenerare.

🔑 **E il difetto che chiude non è nel resolver.** `MakeBlastPreview` legge la cella di **pianificazione**
(`RTHexCombatLibrary.cpp:886-889`) e il bot punteggia sulla stessa (`RTHexBotLibrary.cpp:281-295`):
anteprima e bot modellano **già** una mira ferma, e il resolver ne usa una che insegue. È la classe di
divergenza che [`D-378`](../decisions/RT_PDR_00_Decision_Log.md) esiste per impedire — il giocatore vede una
cosa e il turno ne esegue un'altra.

⛔ **Il costo di gioco, che `D-419` accetta esplicitamente.** Con `ERTAbilityShape::Single` l'area colpita è
**una cella**: chi si sposta di **un** esagono prima del Blast diventa **immune a ogni attacco a bersaglio
singolo**. E `Single` è il **default**: `FRTActionDef` non porta uno `Shape` (`RTActionDef.h:801`), quindi
lo evita solo un'azione che dichiari `Area`, `Line` o `Cone`. ⚠️ **Il corollario
qui sopra — «può comunque essere colpito se la nuova posizione ricade nell'area» — è vero e NON copre il
caso principale**: vale per `Area`, `Line` e `Cone`, non per `Single`.

⛔ Cade `ERTActionFallback::AttackTarget` come default sensato per un attacco base; resta legittimo per le
abilità che **dichiarano** di agganciare.

⚠️ Corollario deciso e privo di implementazione: uno spostamento forzato cambia l'origine **ma non l'esagono
di mira** — l'attacco parte comunque verso di esso, fino alla gittata massima, se ora è fuori portata.
Misurato: **0** occorrenze di una regola «fino alla gittata massima» in `Source/`.

### 5.3 Si può sparare verso un esagono non visibile

**Regola decisa**: l'attacco base può mirare a un esagono che l'attaccante non vede.

**Stato**: `ERTLineOfSightPolicy::Required` è il default (`Source/RefactorTactics/Ability/RTActionDef.h:787`),
e il tiro indiretto è una **licenza dichiarata** che ogni azione deve chiedere.

🔴 **È la più cara delle tre, e il prezzo è documentato.**
[`D-380`](../decisions/RT_PDR_00_Decision_Log.md) ha **pagato** quella licenza per `Action.Mortar`: **12**
danni invece di 18, ricarica **3** invece di 2. Concederla a quattro attacchi base gratuiti e senza cooldown
significa che quel prezzo non comprava una capacità, comprava un'**esclusiva**.

✅ **Risolto il 2026-09-14 da [`D-418`](../decisions/RT_PDR_00_Decision_Log.md): il mortaio NON si
riprezza.** I suoi numeri restano, e comprano **area, gittata e traiettoria** invece dell'esclusiva — una
**riqualificazione** del prezzo, non una sua conferma passiva. 🔑 La misura che ha deciso: a **18 / cd 2** il
mortaio eguaglia `Action.CircularAoE` su tutti e cinque i valori (`Attack` · 65 · **portata** 4 · cd 2 · 18)
**e conserva in più il tiro indiretto**, che quella non dichiara: restituirgli i sei danni produrrebbe
un'abilità **strettamente superiore** a una che esiste già.

⛔ **E non è a costo zero**: **tre** test di `RTBlindFireOffensiveTests.cpp` identificano il mortaio dalla
proprietà che questa sezione rende comune. Vanno riscritti **insieme** all'implementazione, non dopo.

## 6. La rivelazione dell'attaccante — decisa nella sostanza, **non** nel nome

**Regola decisa**: l'attacco pianificato rende il personaggio noto ai nemici **all'inizio del Blast**, anche
quando un controllo ne impedisce l'esecuzione. È un caso esplicitamente diverso da `Interact`, dove una
mancata esecuzione **non** rivela.

⚠️ **E qui il vocabolario è già occupato tre volte.** «Rivelare» significa oggi:

| Senso | Dove |
|---|---|
| il colpo a segno promuove la **vittima** a contatto per la squadra dell'attaccante | [`D-380`](../decisions/RT_PDR_00_Decision_Log.md) |
| `Status.Reveal` — l'**intento** diventa visibile all'avversario | invariante #6 |
| `Observe` — la **conoscenza di squadra** si aggiorna sulla linea visiva | `RTTeamKnowledge.cpp` |

Il senso nuovo — l'**attaccante** diventa noto per il fatto di aver agito — è il quarto, e quattro sensi
sulla stessa parola sono il modo in cui una regola di privacy si rompe in silenzio.
[`CLAUDE.md`](../../CLAUDE.md) §7 chiede una revisione esplicita del boundary per ogni modifica alla
projection degli eventi: **va battezzato prima di essere implementato**, e finché non lo è questa sezione
non produce lavoro.

## 7. Attacco base e furtività

✅ **Attacco + `Sneak` è consentito**, e dopo l'attacco il personaggio può tornare nascosto uscendo dalla
linea visiva dei nemici. Non è una regola nuova: `URTTeamKnowledgeLibrary::Observe` ricalcola la conoscenza
sulla linea visiva a ogni passo, quindi «tornare nascosti» è già il comportamento del modello.

⛔ **`Sneak` non nasconde chi resta nella linea visiva nemica**, e non lo ha mai fatto.

⚠️ **Il rumore fuori vista non ha numeri, e non se ne inventano.**
[`D-267`](../decisions/RT_PDR_00_Decision_Log.md) ha deciso che il rumore appartiene al **produttore
concreto** e che non esistono valori fissi per azione generica. Ciò che `D-412` aggiunge riguarda il solo
`Sneak`, ed è che è **sempre silenzioso**: un profilo silenzioso non ha bisogno di un raggio.

⛔ **Il `Withdraw` NON è coperto da `D-412`.** La sorgente §10.4 lo dice silenzioso insieme allo `Sneak`, ma
`D-412` risponde a `AE-5`, che poneva la domanda sul solo `Sneak`: attribuirgli anche il ripiegamento
sarebbe estendere una decisione oltre ciò che ha deliberato.

## Rapporto con gli altri documenti

| Documento | Rapporto |
|---|---|
| [`../decisions/adr-0007-attacco-base-per-eroe.md`](../decisions/adr-0007-attacco-base-per-eroe.md) | **Prevale** sull'ownership: categoria universale, payload dell'eroe, `Actions[0]`. Questa pagina è l'owner spec che l'ADR dichiarava da creare |
| [`../balance/RT_ActionCatalog_v0.1.md`](../balance/RT_ActionCatalog_v0.1.md) | Possiede i **numeri** entro le regole di questa pagina ([`D-282`](../decisions/RT_PDR_00_Decision_Log.md)) |
| [`spec-sequenza-turno.md`](spec-sequenza-turno.md) | Possiede la collocazione della fase e la simultaneità |
| [`spec-barra-comandi.md`](spec-barra-comandi.md) | Possiede quale movimento è compatibile con l'attacco base |
| [`../technical/systems/conoscenza-parziale-visibile-spec.md`](../technical/systems/conoscenza-parziale-visibile-spec.md) | Possiede la conoscenza e la visibilità; §6 di questa pagina vi si appoggia e **non** la estende |
