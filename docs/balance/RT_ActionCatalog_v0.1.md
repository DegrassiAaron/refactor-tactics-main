# RT — Catalogo azioni v0.1

> **Fonte**: catalogo di bilanciamento v0.1 §§1–3, §12 · PDR-12 — oggi in
> [`prd-personaggi-azioni-e-bilanciamento.md`](../research/prd/prd-personaggi-azioni-e-bilanciamento.md) e
> [`RT_PDR_v0.1_consolidato.md`](../archive/pdr-v0.1/RT_PDR_v0.1_consolidato.md)
> **Decisione abilitante**: [`adr-0003-modello-azioni-v01.md`](../decisions/adr-0003-modello-azioni-v01.md) · **Checkpoint**: CP 1.2 (issue `#28`)
> **Stato**: catalogo di riferimento per la release v0.1. Questi sono i **numeri vigenti**; le *decisioni* stanno
> nel canone ([`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md)), lo *stato di avanzamento* nella
> [roadmap](../roadmap/roadmap-checkpoint.md).

## Come si legge

Ogni azione dichiara:

| Campo | Significato |
|---|---|
| **ActionId** | ID stabile (`FName`, es. `Action.Move`). Non cambia mai: è la chiave del data asset e del TurnLog |
| **Macro-fase** | Fase di Atlas in cui l'azione risolve davvero: `Prep` · `Dash` · `Blast` · `Move` · `Cleanup` |
| **Cod.** | Codice di fase del catalogo (0/10/20/30/40/50/60), conservato come **attributo dell'azione** |
| **Prio** | Priorità intera intra-fase: **valore minore risolve prima** |
| **Range / Dist.** | Portata in celle, o in MP dove indicato |
| **CD** | Cooldown in turni completi |
| **Fallback** | Comportamento quando l'azione non è più eseguibile al momento della risoluzione |
| **Interr.** | `bCanBeInterrupted`: se falso, `Action.Interrupt` non ha effetto su di essa |

**Ordine totale di risoluzione** (mai l'ordine di una `TMap`):
`Macro-fase → Priority → ActionDefinitionId → SourceUnitId → EventSequence`.

⚠️ **Rimappatura delle fasi**: il catalogo numera le fasi `Snapshot(0) → Preparazione(10) → Movimento(20) →
Controllo(30) → Attacco(40) → Ambiente(50) → Cleanup(60)`, con il movimento **prima** dell'attacco. Le
macro-fasi del progetto restano quelle di Atlas — `Prep → Dash → Blast → Move` — quindi **la fase 20 si
sdoppia**: mobilità rapida in `Dash` (prima del Blast), percorso normale in `Move` (dopo il Blast). Ogni azione
di movimento qui sotto dichiara esplicitamente quale delle due. Motivazione in [ADR-0003 §3](../decisions/adr-0003-modello-azioni-v01.md).

## Slot per turno

| Slot | Quantità | Esempi |
|---|---|---|
| Movimento | 1 | `Move` nei profili `Sneak`/`Move`/`Sprint` (§2.1) · `Dash`, `Leap`, `Reposition` (§2.2) |
| Azione principale | 1 | `BasicAttack`, `Charge`, `Guard`, `Heal`, **`Overwatch`** — mai due insieme |
| Reazione | 1 | `Counter`, `Intercept`, `Deflect` |
| Comunicazione | — | Ping, label |
| Conferma | illimitata (rate limit) | Ready |

> **Un movimento e un'azione principale** — e si sceglie **quando** ci si muove
> ([D-028](../decisions/RT_PDR_00_Decision_Log.md)):
>
> ```text
> schivo e sparo   ->  Dash (movimento, fase Dash)  +  Attacco (principale, Blast)
> sparo e muovo    ->  Attacco (principale, Blast)  +  Move (movimento, fase Move)
> ```
>
> Stessi due slot, ordine diverso: ci si muove **prima** dei colpi per schivare, o **dopo** per ripararsi.
> Nessuna delle due domina l'altra, ed è il motivo per cui lo scatto **non** occupa la principale.
>
> ✅ **Migrato nel codice il 2026-08-08.** A far valere la regola è il **resolver**: dopo uno scatto la
> destinazione pianificata *diventa* la cella d'arrivo, quindi il movimento è speso comunque sia stato
> pianificato — non un controllo che il bot potrebbe aggirare (invariante #1). Vale anche per le mobilità
> d'eroe: `Hero.Phase.FluidTrail` è passata a **Movimento**, e l'invariante `Heroes.MobilityWithoutDamageIsNotMain`
> impedisce che la prossima nasca sulla principale, dove `MakeHeroAction` la metterebbe per default.

> **Slot ≡ Action Points** (consolidato il 2026-08-07). Il workbook `RefactorTactics_Balance_Matrices_v0.1.xlsx`
> — che è **`RESEARCH`**, non una fonte ([D-023](../decisions/RT_PDR_00_Decision_Log.md)): qui vale solo come
> nota terminologica — modella gli stessi slot come **risorse**: `RES_ACTION`
> (Action Points, cap **2**, nessun riporto fra turni) copre Movimento + Azione principale, `RES_REACTION`
> (cap **1**) è lo slot Reazione dell'ADR-0003. **Sono lo stesso sistema con due nomi**: la tabella qui sopra
> resta la formulazione canonica, «AP» è ammesso come sinonimo nei documenti di bilanciamento.
>
> **Risorsa firma** — ciò che l'MVP chiamava *energia* è per-personaggio, con nome e trigger di ricarica
> propri: `Gadget` Carica Conduttiva · `Phase` Riserva Idrica · `Riktor` Integrità Strutturale · `Wraith`
> Slancio. Cap **4** per tutte (valore più basso fra i candidati), ricarica **1** sul trigger d'affinità.
> Cambia il nome e cosa la ricarica, **non** la regola.

Un piano completo dichiara: percorso di movimento · azione principale · reazione (se disponibile) · **facing
finale** · fallback.

> 🧭 **Come i quattro budget si tengono insieme** — slot, Movement Point, budget di pivot, cooldown/risorsa —
> è di [`../gameplay/spec-economia-del-turno.md`](../gameplay/spec-economia-del-turno.md), owner dal
> 2026-08-12. Questa sezione resta l'owner dei **numeri**; quella pagina risponde alla domanda *«questa cosa
> cosa consuma, e chi le dice di no?»* e registra la proposta — **aperta**, non canonica — che il profilo di
> movimento cambi anche la *legalità* delle azioni (`AE-1`, `AE-2` in
> [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md)).

---

## 1. Azioni generiche (universali)

Le **sette** azioni che ogni eroe possiede, indipendentemente dal kit
([D-014](../decisions/RT_PDR_00_Decision_Log.md) · [D-025](../decisions/RT_PDR_00_Decision_Log.md)):

```text
Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch
```

| ActionId | Azione | Slot | Macro-fase | Cod. | Prio | Range | CD | Rumore | Fallback | Interr. |
|---|---|---|---|---:|---:|---|---:|---:|---|---|
| `Action.Wait` | Attesa | — | Move | 20 | 100 | — | 0 | 0 | — | no |
| `Action.Move` | Movimento | Movimento | **Move** | 20 | 50 | 5 MP | 0 | — | `Fallback.Stop` | sì |
| `Action.BasicAttack` | Attacco base | Principale | Blast | 40 | 50 | arma | 0 | — | `Fallback.Cancel` | sì |
| `Action.Guard` | Guardia | Principale | **Prep** | 10 | 40 | self | 0 | — | `Fallback.Cancel` | no |
| `Action.Brace` | Irrigidimento | Principale | **Prep** | 10 | 30 | 0 | 1 | — | `Fallback.Cancel` | no |
| `Action.Interact` | Interagisci | Principale | Blast | 40 | 80 | 1 | 0 | — | `Fallback.Cancel` | sì |
| `Action.Overwatch` | Guardia reattiva | Principale | **Prep** *(arma)* | 10 | 45 | cono da facing | 0 | — | `Fallback.Cancel` | no |
| `Action.Activate` | ~~Attiva~~ | — | — | 40 | 70 | 1 | 0 | — | — | — |

> 🔊 **`Rumore` è l'intensità dell'evento sonoro sulla scala `0-10` di
> [D-041](../decisions/RT_PDR_00_Decision_Log.md)**, la stessa su cui vive la `Soglia d'udito` degli eroi
> ([`RT_HeroCatalog_v0.1.md`](RT_HeroCatalog_v0.1.md) §5.1): è ciò che rende confrontabili i due lati di
> `IsAudible(ReceivedNoise, HearingThreshold)`.
>
> ⚠️ **Un `—` non è «silenzioso»: è «non ancora deciso».** D-041 dichiara **tre** valori — `Wait 0`,
> `Sprint 5`, `Dash 6` — più «esplosione 10», che non è un'azione. Le altre intensità **non esistono in
> nessuna fonte**, e non si inventano qui: la domanda ha un ID, **`AE-8`** in
> [`OPEN_DECISIONS.md`](../OPEN_DECISIONS.md), accanto ad `AE-5` che copre il profilo `Sneak`.
>
> 🔴 **Le abilità firma non ricevono un'intensità propria, ed è una scelta, non una dimenticanza**
> ([#690](https://github.com/DegrassiAaron/refactor-tactics-main/issues/690)). Sarebbero **24** numeri
> nuovi, nessuno derivabile da una fonte corrente, per un canale che **oggi non ha un produttore**:
> `FRTNoiseEvent::Intensity` non è assegnata da nessuna parte fuori dai test. Inventarli sarebbe
> ribilanciare, e questa colonna **sposta dati esistenti, non ne crea**. Se un'abilità dovrà suonare
> diversamente dall'azione che la porta, la domanda si apre allora, con la sua evidenza.

**`Overwatch` è universale e compete con l'azione offensiva.** L'economia è
`Attack` **oppure** `Ability` **oppure** `Overwatch`, mai sommate, salvo eccezione dichiarata da un'abilità
([D-012](../decisions/RT_PDR_00_Decision_Log.md)). Il **profilo** — cosa scatta, con quale effetto — dipende
dall'eroe e dall'equipaggiamento; l'**azione** no: non è la skill di qualcuno.
Si arma in pianificazione, apre una finestra di **3,0 s** con `FIRE`/`HOLD` e `Timeout → HOLD`, e il suo cono
**è** il facing dell'unità ([ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) ·
[ADR-0005](../decisions/adr-0005-orientamento.md) §4c). Epic **E14**, dipende da E13.

> **Da non confondere**: armare l'Overwatch costa l'**azione principale**; lo **slot reazione** preparato è
> un'altra cosa e resta indipendente. Un eroe può avere entrambi.

**`Action.Activate` è assorbita da `Action.Interact`** ([D-014](../decisions/RT_PDR_00_Decision_Log.md),
confermata da [D-025](../decisions/RT_PDR_00_Decision_Log.md)): erano la stessa cosa con due nomi — «attiva un
dispositivo» è un'interazione.

> ✅ **Migrazione eseguita il 2026-08-10 (`#199`).** Il catalogo **non spedisce più** `Action.Activate`: da
> quella data il codice ha una sola azione di interazione.
>
> ⚠️ **Aggiornato il 2026-08-13 ([D-134](../decisions/RT_PDR_00_Decision_Log.md)): lo Stable ID è stato
> cancellato del tutto.** Fino a quella data era **reindirizzato in lettura** a `Action.Interact` da
> `URTCatalogLibrary::ResolveLegacyActionId`, perché D-014 vietava di cancellare un ID che entra nel TurnLog
> serializzato. Il redirect è stato rimosso quando si è misurato che **non proteggeva nulla**: nessuna traccia
> versionata contiene `Action.Activate` — il corpus golden porta solo `Action.Move` — e il gioco non è ancora
> uscito. `FindCoreAction("Action.Activate")` oggi risponde con una definizione **vuota**, ed è quello che
> verifica `RefactorTactics.Actions.RetiredStableIdIsGoneEntirely`.
>
> La riga qui sopra **resta barrata invece di sparire**: la tabella è il catalogo *storico* delle identità, e
> un ID ritirato che scompare dal documento è un ID che nessuno saprà più leggere quando lo incontra in un
> replay vecchio.

**`Action.Guard` resta fra le universali** ([D-025](../decisions/RT_PDR_00_Decision_Log.md)): ha già tre
consumatori — questo catalogo, l'interazione con `Status.Root`, e la difesa direzionale di
[ADR-0005](../decisions/adr-0005-orientamento.md) §4a, dove fuori dall'arco frontale la sua riduzione decade.
Toglierla dalle fondamentali avrebbe lasciato tre regole appese a un'azione non più garantita a nessuno.

**Wait** — non si muove e non usa l'azione principale. Può comunque: impostare il facing · preparare una
reazione · mantenere una stance già attiva · contestare un obiettivo.

**Move** — percorso di celle adiacenti. Budget **5 MP**; cella normale 1 MP, terreno difficile 2 MP, salita via
rampa 2 MP. Una cella occupata da un'unità solida non è attraversabile. Il percorso **non** viene ricalcolato
globalmente durante la risoluzione: se si blocca, l'unità si ferma nell'ultima cella valida (`Fallback.Stop`, la
regola standard del vertical slice).

**Basic Attack** — dipende dall'eroe e dalla variante arma:

| Tipo | Danno | Range |
|---|---:|---:|
| Corpo a corpo | 28 | 1 |
| Corto raggio | 25 | 3 |
| Medio raggio | 22 | 4 |
| Lungo raggio | 20 | 6 |

**Guard** — riduce di **15** il primo danno diretto ricevuto · resiste a una spinta di 1 cella · termina nel
Cleanup · **non** protegge dagli hazard ambientali già presenti.

> In v0.1 «una spinta di 1 cella» significa **ogni** spinta del gioco: il catalogo non ha valori maggiori di 1.
> È il motivo per cui sulla spinta `Guard` e `Brace` non si distinguono — [D-074](../decisions/RT_PDR_00_Decision_Log.md).

**Interact** — agisce su un oggetto adiacente: porta · consolle · ascensore · generatore · sprinkler · ponte ·
obiettivo. Assorbe ciò che il catalogo chiamava `Activate`: il bersaglio cambia, il gesto no.

---

## 2. Azioni di movimento

Il movimento si divide in **due famiglie**, e la distinzione è strutturale
([D-015](../decisions/RT_PDR_00_Decision_Log.md)):

### 2.1 Profili del movimento normale — slot **Movimento**, fase **Move**

`Sneak` · `Move` · `Sprint` **non sono tre azioni concorrenti**: sono tre **profili** della stessa famiglia.
Occupano tutti il **solo slot movimento** e risolvono tutti nella macro-fase **`Move`**, cioè **dopo** il
Blast. Cambiano distanza, rumore ed esposizione — non l'economia del turno.

| Profilo | Budget | Note |
|---|---|---|
| `MovementMode.Sneak` | **non specificato** | Costo, portata e rumore **non sono definiti da nessuna fonte corrente**. Non si inventano: la domanda è **[`AE-5`](../OPEN_DECISIONS.md)** — *«Con quali numeri esiste il profilo `Sneak`?»*, l'unico dei quattro profili senza budget |
| `MovementMode.Move` | **5 MP** | il profilo neutro |
| `MovementMode.Sprint` | **8 MP** | conserva un trade-off reale, vedi sotto |
| `MovementMode.Withdraw` | **2 MP** | **non si sceglie**: lo impone l'`Overwatch` ([D-070](../decisions/RT_PDR_00_Decision_Log.md)) |

**`Withdraw` è il ripiegamento dopo la sorveglianza**, e sta qui perché è un profilo del movimento normale —
stesso slot, stessa macro-fase — non una mobilità rapida. Tre cose lo distinguono dagli altri tre:

- **non è una scelta libera**: chi arma l'`Overwatch` riserva lo slot movimento a questo profilo, e lo dichiara
  in Planning insieme a settore e facing. È anche la ragione per cui armare l'Overwatch **esclude il `Dash`**:
  lo slot è già impegnato, non serve una regola apposta;
- **risolve nello Stage B della `Move`**, cioè **dopo** che tutti gli altri si sono mossi. La priorità spaziale
  tardiva è parte del prezzo: una cella occupata nel frattempo **non** si libera, il percorso **non** si
  ricalcola, e il ripiegamento si ferma all'ultima cella valida;
- **2 MP** è ancorato ad `Action.Reposition` (2 celle, §2.2) — l'unica altra mobilità breve del catalogo —
  invece di essere scelto a intuito. Resta da playtest come ogni valore di questa tabella.

> **Perché non si chiama `Reposition`.** Quel nome è già di un'azione viva: scatto lineare di 2 celle in
> macro-fase **`Dash`** (§2.2), concesso anche da `Hero.Phase.FlowReaction` e `Hero.Wraith.Feint`. Due entità con lo
> stesso nome in due fasi diverse si pagano a ogni lettura del TurnLog, non una volta sola.

**`Sprint` non è un `Dash`.** È il profilo lungo del movimento normale, quindi risolve dopo il Blast: non
permette di sparare da un'altra posizione nello stesso turno, che è precisamente ciò che un `Dash` fa.

> ⚠️ **Divergenza documento/codice, misurata il 2026-08-08.** Nel codice `Action.Sprint` è ancora in
> `ERTResolutionPhase::FastMovement` (fase `Dash`) e consuma **movimento più azione principale**. La decisione
> è presa, la migrazione no: lo Stable ID compare in **15 file** fra codice e test, e cancellarlo in una PR
> documentale romperebbe test e replay. Tracciato in
> [`../DOC_CONFLICT_MATRIX.md`](../DOC_CONFLICT_MATRIX.md) riga 41 → issue di refactor.
>
> 🔄 **Questa riga è stata vera, poi falsa, e dal 2026-08-12 è di nuovo vera.** In mezzo,
> [D-068](../decisions/RT_PDR_00_Decision_Log.md) aveva **rovesciato** la decisione — «Sprint resta pre-Blast,
> ed è una decisione, non un arretrato» — lasciando però il paragrafo qui sopra al suo posto: per quattro
> giorni il catalogo ha dichiarato una regola che il canone aveva appena abbandonato, e nessun gate poteva
> accorgersene. [D-116](../decisions/RT_PDR_00_Decision_Log.md) ha rovesciato di nuovo, e per un motivo che
> non era sul tavolo nel 2026-08-10: restando pre-Blast lo Sprint **spara da una posizione nuova**, cioè fa
> ciò che questa stessa sezione attribuisce al `Dash`. La migrazione della sola `ResolutionPhase` è ora
> lavoro di **E38**.
>
> ⚠️ **E non si migra da sola.** Portare lo Sprint dopo il Blast rende `Status.Exposed` **inerte** — verrebbe
> applicato quando tutti hanno già sparato, e scadrebbe nel Cleanup subito dopo. Per questo D-116 lo porta a
> **`Turni 2`** nello stesso momento, e introduce la compatibilità con il profilo di movimento
> ([`../gameplay/spec-compatibilita-azioni-movimento.md`](../gameplay/spec-compatibilita-azioni-movimento.md)):
> senza le due contropartite, lo Sprint diventerebbe **un `Move` più lungo che costa solo la reazione**.
>
> **Il trade-off dello Sprint va migrato, non perso.** Oggi paga con `Status.Exposed` e con la rinuncia alla
> reazione. Nel modello a profili non può diventare un potenziamento gratuito del `Move`: se perde il costo di
> slot deve conservare un costo, altrimenti nessuno sceglierebbe più `Move`.

### 2.2 Mobilità speciali — fase **Dash**

`Dash` · `Charge` · `Leap` · `Reposition` risolvono in macro-fase **Dash**, prima del Blast: riposizionarsi in
fretta è ciò che permette di sparare da un'altra parte nello stesso turno.

**Ma non occupano tutte lo stesso slot** ([D-028](../decisions/RT_PDR_00_Decision_Log.md)): `Dash`, `Leap` e
`Reposition` sono **movimento** — schivi e ti resta l'azione principale, ma non ti muovi ancora. `Charge` è
**un attacco** che ti porta addosso al bersaglio: occupa la **principale**, e il movimento ti resta.

| ActionId | Azione | Slot | Macro-fase | Cod. | Prio | Distanza | CD | Rumore | Fallback | Interr. |
|---|---|---|---|---:|---:|---|---:|---:|---|---|
| `Action.Sprint` *(vedi §2.1)* | Scatto lungo | Movimento + Principale ⚠️ | **Dash** ⚠️ | 20 | 60 | 8 MP | 0 | 5 | `Fallback.Stop` | sì |
| `Action.Dash` | Scatto | **Movimento** | **Dash** | 20 | 30 | 3 celle | 1 | 6 | `Fallback.Stop` | sì |
| `Action.Charge` | Carica | Principale | **Dash** | 20/30 | 35 | 3 celle | 2 | — | `Fallback.Stop` | sì |
| `Action.Leap` | Balzo | **Movimento** | **Dash** | 20 | 25 | 3 celle | 2 | — | `Fallback.Stop` | sì |
| `Action.Reposition` | Riposizionamento | **Movimento** | **Dash** | 20 | 40 | 2 celle | 1 | — | `Fallback.Stop` | sì |

**Sprint** — fornisce 8 MP · occupa il **solo slot movimento** ([D-028](../decisions/RT_PDR_00_Decision_Log.md),
coerente con D-015) · non permette di preparare una reazione · applica `Status.Exposed` (**+5** al primo danno diretto ricevuto). ⚠️ **Durata: da `1` a `2` turni** con [D-116](../decisions/RT_PDR_00_Decision_Log.md) — non è un ribilanciamento, è la contropartita della migrazione di fase: con lo Sprint dopo il Blast, un `Exposed` che scade nel Cleanup dello stesso turno non incontrerebbe mai un attacco.

> ⚠️ **Il prezzo dello Sprint ora regge tutto sui dati.** Finché consumava anche l'azione principale il costo
> era strutturale; adesso è `Exposed` (+5 al primo danno diretto) più la rinuncia alla reazione, contro 3 MP
> in più di un `Move`. Se non basta, `Sprint` è un `Move` più lungo e basta — l'**upgrade puro** che D-015
> vieta. Tracciato in [`../roadmap/plans/showcase-v01-audit.md`](../roadmap/plans/showcase-v01-audit.md)
> §`BAL-1`, che dice **come misurarlo** (quante volte il bot lo sceglie quando era disponibile) invece di
> proporre un numero nuovo.

**Dash** — movimento lineare lungo una delle **sei** direzioni · non consuma il percorso `Move` (quindi è
compatibile con esso) · non attraversa muri o coperture alte · non può terminare in una cella occupata.

**Charge** — lineare, massimo 3 celle · infligge **20** danni al primo nemico incontrato e applica `Push 1` ·
si ferma dopo l'impatto · interrotta da coperture alte, muri e porte chiuse. Il codice `20/30` è dovuto
all'impatto (controllo), che nel progetto resta dentro il **Blast** per priorità.

**Leap** — ignora unità e coperture basse · richiede una cella finale valida · non attraversa soffitti né cambia
`Layer` arbitrariamente · ignora gli effetti delle celle intermedie, **subisce** quelli della cella d'atterraggio.

**Reposition** — tattico, massimo 2 celle · **non** applica `Exposed` · non attraversa unità.

---

## 3. Azioni offensive

| ActionId | Azione | Macro-fase | Cod. | Prio | Danno | Targeting | CD | Fallback | Interr. |
|---|---|---|---:|---:|---:|---|---:|---|---|
| `Action.PrecisionAttack` | Attacco di precisione | Blast | 40 | 60 | 24 | bersaglio | 1 | `Fallback.Cancel` | sì |
| `Action.HeavyAttack` | Attacco pesante | Blast | 40 | 80 | 35 | bersaglio | 2 | `Fallback.Cancel` | sì |
| `Action.LineAttack` | Attacco lineare | Blast | 40 | 55 | 22 | linea | 1 | `Fallback.AttackCell` | sì |
| `Action.CircularAoE` | Area circolare | Blast | 40 | 65 | 18 | cella, raggio 1 | 2 | `Fallback.AttackCell` | sì |
| `Action.SuppressiveLine` | Linea di soppressione | **Prep** | 10/20 | 30 | 16 | linea / reazione | 2 | — | no |
| `Action.MarkTarget` | Marchia bersaglio | Blast | 40 | 40 | 0 | bersaglio | 1 | `Fallback.Cancel` | sì |

> **`SuppressiveLine` non si fonde con `Overwatch`.** Si somigliano — entrambe si armano in `Prep` e reagiscono
> a un passaggio — ma non sono la stessa cosa: `Overwatch` è l'**azione universale** e l'infrastruttura di
> controllo reattivo, `SuppressiveLine` è un **contenuto specifico** con effetti propri (linea, 16 danni,
> cooldown 2). Se un giorno il codice dimostrasse che è solo un duplicato nominale dell'Overwatch, la
> conclusione sarebbe una **issue di refactor**, non una cancellazione durante un riordino documentale.

**Precision Attack** — range dell'arma **+1** · ignora la copertura bassa · **non** utilizzabile dopo Sprint.

**Heavy Attack** — priorità bassa (risolve tardi) · infligge **20** danni alle coperture distruttibili · se
interrotto prima della fase d'attacco non produce alcun effetto.

**Line Attack** — direzione esagonale, colpisce il **primo** bersaglio valido · range standard 5 celle · una
copertura alta interrompe la linea.

**Circular AoE** — centro selezionabile entro 4 celle, raggio 1 · **friendly fire attivo** · la copertura non
riduce il danno se il centro dell'esplosione è dalla stessa parte della copertura del bersaglio.

**Suppressive Line** — si *prepara* (per questo risolve in `Prep`) e si attiva sul trigger «un nemico entra in una
cella controllata»: 16 danni, interruzione del movimento, **una sola attivazione**; il nemico resta nella cella
appena raggiunta.

**Mark Target** — applica `Status.Marked` per un turno: il prossimo attacco alleato contro il bersaglio infligge
**+6** danni e consuma il marchio. Non aumenta il danno ambientale.

---

## 4. Azioni difensive e reazioni

⚠️ **Solo tre di queste sei sono reazioni.** La colonna «Slot» è la distinzione che conta: `Counter`,
`Intercept` e `Deflect` occupano lo slot **Reazione** (0-1 per turno, trigger valutato sullo snapshot del Blast);
`Brace`, `Shield` e `Cleanse` sono azioni **Principali** che si dichiarano e basta, senza trigger. Stare nella
stessa sezione del catalogo non le rende lo stesso tipo di cosa.

> **Precisazione 2026-08-09 — [D-047](../decisions/RT_PDR_00_Decision_Log.md)**: `Brace` resta un'azione
> **Principale** di `Prep` con questi numeri, ma non è più «si dichiara e basta»: **arma un Reaction Profile**.
> La risposta universale è `Hold Ground` e **coincide con l'effetto qui sotto** — anti-spinta e riduzione —
> quindi la riga della tabella non cambia. Un profilo d'eroe che dichiarasse una seconda risposta legale
> aprirebbe una finestra, ma nessun eroe della v0.1 lo fa. Owner:
> [`../gameplay/spec-reaction-clash-e14.md`](../gameplay/spec-reaction-clash-e14.md) §2.

| ActionId | Azione | Slot | Macro-fase | Cod. | Prio | Range | Effetto | CD |
|---|---|---|---|---:|---:|---:|---|---:|
| `Action.Counter` | Contrattacco | Reazione | Blast | 30/40 | 20 | 0 | contrattacco 16 | 2 |
| `Action.Intercept` | Interposizione | Reazione | Blast | 30 | 10 | 2 | protezione alleato | 2 |
| `Action.Deflect` | Deviazione | Reazione | Blast | 30 | 15 | 0 | riduzione danno 20 | 2 |
| `Action.Brace` | Irrigidimento | Principale | **Prep** | 10 | 30 | 0 | anti-spinta | 1 |
| `Action.Shield` | Scudo | Principale | **Prep** | 10 | 35 | 0 | scudo temporaneo 25 | 2 |
| `Action.Cleanse` | Purifica | Principale | Blast | 30 | 25 | 0 | rimozione stato | 2 |

> La colonna **Range** non era nella tabella originale (il PDF dice «entro il range consentito», senza numero):
> i valori sono decisi in CP 5.2. `0` = su se stessi, che è il caso di tutte tranne `Intercept` (2 celle, questo
> sì dichiarato dal testo). Per `Counter`, `0` significa che il contrattacco raggiunge chi ha colpito, chiunque
> sia: inventare una portata cambierebbe *quali* attacchi si possono punire, e il catalogo non la fornisce.

**Counter** — trigger: l'eroe è colpito da un attacco **diretto** entro il range consentito. Esegue un attacco da
**16** danni *dopo* l'attacco ricevuto · non si attiva contro danni ambientali · una sola attivazione.
Il contrattacco entra **in coda** agli attacchi del Blast: non consuma i modificatori "primo colpo"
(Guard/Exposed/Deflect) di chi lo subisce, che restano per l'attacco pianificato.

**Intercept** — trigger: un alleato entro **2** celle è bersagliato da un attacco diretto. L'intercettore
**diventa** il bersaglio; la traiettoria deve essere compatibile · non intercetta AoE né hazard · una sola attivazione.

> La traiettoria che deve essere libera è quella dall'attaccante **all'intercettore**, non alla vittima: ci si
> mette in mezzo a un colpo, non lo si teletrasporta addosso. La priorità **10** — la più bassa fra le reazioni —
> non è un dettaglio di bilanciamento: cambiando il bersaglio dei colpi, Intercept deve risolvere prima che le
> altre reazioni valutino chi è stato colpito, altrimenti il protetto contrattaccherebbe per un colpo mai ricevuto.

**Deflect** — riduce il danno diretto di **20**. Se il danno arriva a zero l'attacco è comunque considerato
avvenuto (conta per trigger e marchi) · non riflette · non funziona contro AoE ambientali.

**Brace** — impedisce la prima spinta · riduce di **10** tutti i danni diretti fino al Cleanup · **blocca il
movimento volontario** dell'eroe.

> Tre precisazioni di implementazione (CP 5.2). Il **-10 vale su ogni colpo**, non solo sul primo: è un
> meccanismo diverso da quello di `Guard`/`Deflect`, e usa una funzione diversa. L'anti-spinta non ha limite
> di distanza *nel codice*, ma **in v0.1 questo non è osservabile** — vedi la nota qui sotto. Il **blocco del
> movimento** riusa `Status.Root`, quindi ferma anche lo scatto: chi si pianta per incassare non si riposiziona.

> ⚠️ **L'anti-spinta non distingue `Brace` da `Guard` in v0.1** — [D-074](../decisions/RT_PDR_00_Decision_Log.md),
> uscita **(B)** di [#400](https://github.com/DegrassiAaron/refactor-tactics-main/issues/400).
> Il catalogo ha **un solo valore di spinta, `1`**: `Action.Push`, `Action.Charge` (da cui `Hero.Riktor.Ram`
> eredita i suoi `20 danni + Push 1`), `Hero.Phase.PressureJet` e la variante `Hero.Phase.CircularTide.Impact`.
> L'elenco è **esaustivo e senza eccezioni in tutto il progetto**: l'ultimo `Push 2` era `Guardian.Sweep`, ed
> è sparito insieme agli archetipi legacy (`#426`, 2026-08-10) — quando questa nota è stata scritta esisteva
> ancora, fuori dal roster, e la riga lo dichiarava come eccezione. Ora nessuna azione, in nessun catalogo,
> spinge di più di una cella. `Guard` resiste fino a 1 cella, quindi copre per intero lo spazio degli
> spostamenti prodotti **dalle azioni**.
>
> 🔵 **La spinta forte esiste nei DATI, e non da qui: dall'equipaggiamento** (corretto il 2026-08-16).
> `Weapon.Impact` porta lo spostamento di `Hero.Phase.PressureJet` a **2** ([D-085](../decisions/RT_PDR_00_Decision_Log.md))
> ed è il **default dichiarato** di Phase ([D-089](../decisions/RT_PDR_00_Decision_Log.md)). L'elenco delle
> azioni qui sopra resta esatto — `Weapon.Impact` non è un'azione — ma la conclusione che se ne traeva no:
> contro una spinta di 2 **`Guard` cede e `Brace` regge**, misurato da
> `Equipment.PushTwoSeparatesGuardFromBrace`.
>
> 🔴 **Ma quel pezzo non arriva addosso a nessuno, ed è la parte che conta** (misurato il 2026-08-16,
> correggendo una prima stesura di questa stessa nota che diceva *«il mestiere anti-spinta di `Brace` è
> osservabile in v0.1»*). La catena si ferma al primo anello:
>
> · **in partita** nessuno equipaggia — `DefaultWeaponVariantFor` ha chiamanti **solo nei test**, e né
>   `RTGameMode.cpp` né `RTUnit.cpp` nominano equipaggiamento o loadout;
> · **nell'harness** il loadout viene *validato* e la variante d'arma **ignorata**: `RTScenarioSession`
>   applica gadget e moduli via `MakeEquipmentAction`, mentre le varianti passano da `EquipWeaponVariant`,
>   che fuori dai test non ha un solo chiamante.
>
> Quindi la spinta di 2 è **dichiarata e mai prodotta**: in ogni partita e in ogni scenario lo spostamento
> è `1`, e sulla spinta le due difese danno davvero lo stesso esito. Lo pinna
> `Spec.Brace.PushBeyondGuardThreshold`, dichiarato `expected-fail`: descrive ciò che dovrebbe succedere e
> **fallisce apposta** finché la variante non viene applicata. È anche la ragione per cui la seduta `U20`
> non riesce a distinguere le due difese — non è presentazione e non sono i numeri.
>
> **Ciò che distingue le due sul colpo singolo resta il danno**: `Guard` −15 sul solo primo colpo,
> `Brace` −10 su ogni colpo — cioè *primo colpo pesante* contro *colpi ripetuti*. Sul colpo singolo senza
> `Weapon.Impact` `Guard` domina, ed è il trade-off pinnato da `Spec.Brace.GuardAndBraceOnMixedHit` e
> `Spec.Brace.BraceWinsOnSecondHit` (12 contro 17 su due colpi).
>
> ✅ **Il confine fra le due è DECISO**: [D-121](../decisions/RT_PDR_00_Decision_Log.md) (2026-08-12) ha
> chiuso `BAL-1` scegliendo lo **status quo** — nessuna separazione fra danno e spinta, nessuna magnitudine
> nuova. ⏳ Resta il solo **gate umano**, la seduta `U20` / `PIE-BAL1` di
> [#403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/403), che verifica se la differenza è
> *leggibile a schermo* — non *quale sia*. Se non lo fosse, D-121 vincola l'ordine: prima feedback e
> presentazione, i numeri solo se quello non basta.
>
> 🔴 **Questo riquadro ha detto il contrario per quattro giorni**, ed è la ragione per cui va letto con la
> data accanto: dichiarava *«sulla spinta le due difese danno lo stesso esito, **sempre**»*, *«il confine
> fra le due resta **aperto** come `BAL-1`»* e *«la clausola torna osservabile solo se la **v0.2** introduce
> una spinta `≥ 2`»*. La prima era falsa dal 2026-08-11 (D-085), le altre due dal 2026-08-12 (D-121).

**Shield** — applica **25** punti scudo, consumati prima della salute · scade nel Cleanup del turno · non protegge
dagli effetti di controllo privi di danno.

**Cleanse** — rimuove **un solo** stato fra `Burning`, `Electrified`, `Rooted`, `Marked`, `Exposed`. La priorità
di rimozione è scelta dal giocatore **durante il planning** (non a runtime: nessuna scelta implicita).

> **Limite v0.1**: `Burning` ed `Electrified` non esistono ancora come stato di unità (sono ambiente, epic E8 /
> CP 8.2), quindi oggi `Cleanse` opera sui soli `Rooted`/`Marked`/`Exposed`. Il meccanismo scorre una lista di
> tag dichiarata nel piano: in E8 basterà rendere pianificabili i due nuovi stati, senza toccare il resolver.
> Senza un ordine dichiarato **non rimuove nulla** (fail-closed): "nessuna scelta implicita" significa che il
> resolver non sceglie al posto del giocatore neppure quando il candidato sarebbe uno solo.

---

## 5. Azioni di controllo

Tutte le azioni di controllo risolvono dentro il **Blast**, prima del danno, per priorità (il controllo non è una
macro-fase separata: ADR-0003 §3).

| ActionId | Azione | Macro-fase | Cod. | Prio | Range | Effetto | Durata | CD |
|---|---|---|---:|---:|---:|---|---|---:|
| `Action.Push` | Spinta | Blast | 30 | 40 | 1 | spinta 1 | istantanea | 1 |
| `Action.Pull` | Trazione | Blast | 30 | 40 | 2 | trazione 1 | istantanea | 1 |
| `Action.Root` | Radicamento | Blast | 30 | 25 | 1 | blocca il movimento | 1 turno | 2 |
| `Action.Interrupt` | Interruzione | Blast | 30 | 20 | 1 | annulla azione compatibile | istantanea | 2 |
| `Action.Slow` | Rallentamento | Blast | 30 | 50 | 1 | +1 costo movimento | 1 turno | 1 |

**Range — decisa in CP 4.7, non nel PDF**: questa è l'unica sezione del catalogo la cui tabella non dichiarava
una portata. 1 per quattro azioni su cinque; **Pull è l'eccezione (2)**: con targeting a 1 (adiacenza) il
bersaglio, tirato di 1 cella verso chi tira, finirebbe sempre sulla sua stessa cella — sempre occupata — e la
trazione si annullerebbe per costruzione, in ogni caso. Serve poter agganciare un bersaglio a 2 celle per
tirarlo a 1 senza finirgli addosso.

**Push / Pull** — un'unità non può terminare dentro un'altra · una copertura alta blocca lo spostamento (oggi
letta da `bBlocksMovement`) · se la destinazione è bloccata, lo spostamento termina lì · le collisioni **non**
producono danno nella v0.1 · le cadute arrivano con le mappe multilivello · la resistenza di `Action.Guard` (-1
cella) vale solo per **Push**: il testo del catalogo dice «spinta», non «trazione», e la v0.1 non estende
implicitamente la resistenza a Pull.

**Root** — cancella i micro-step di movimento non ancora risolti · non impedisce attacchi, `Guard` o `Activate` ·
non annulla un teletrasporto già risolto. Implementato tramite `GetEffectiveMoveRange` (azzera il budget per chi
è radicato), letto FRESCO a ogni fase di movimento — così un Root applicato nel Blast si riflette già sulla fase
Move dello stesso turno, anche su un percorso a waypoint già pianificato prima del radicamento
(`URTHexSimLibrary::TruncatePathToBudget`, CP 4.7).

**Interrupt** — un'azione è interrompibile **solo** se dichiara `bCanBeInterrupted = true`. Non tutte lo sono.
Cancella l'intera azione bersaglio (danno ed effetti collaterali insieme), non solo i suoi effetti: si applica
filtrando i colpi già raccolti nel Blast, prima che diventino danno o eventi.

**Slow** — +1 al costo di **ogni cella** attraversata, non un dimezzamento del raggio totale (il meccanismo che
`Ranger.Burst` applicava allo stesso stato prima di CP 4.7). Vale per il movimento a budget (`Action.Move`,
`Action.Sprint`); non riduce le mobilità lineari (Dash/Charge/Leap/Reposition), dichiarato fuori scope v0.1.

---

## 6. Azioni di supporto e ambiente

Le azioni che creano o modificano l'ambiente hanno un doppio codice (`40/50`): l'azione risolve nel **Blast**, la
sua **propagazione** ambientale nel `Cleanup` (fase 50 del catalogo), dopo il Move — così colpisce anche chi è
appena entrato nella cella.

| ActionId | Azione | Macro-fase | Cod. | Prio | Range | Effetto | CD |
|---|---|---|---:|---:|---:|---|---:|
| `Action.Heal` | Cura | Blast | 40 | 70 | 3 | cura 20 | 1 |
| `Action.CreateWater` | Crea acqua | Blast (+Cleanup) | 40/50 | 60 | 4 | acqua raggio 1 | 2 |
| `Action.Ignite` | Incendia | Blast (+Cleanup) | 40/50 | 60 | 4 | fuoco su cella | 2 |
| `Action.Electrify` | Elettrifica | **Cleanup** | 50 | 30 | 4 | 20 danni, propagazione elettrica 12 | 2 |
| `Action.CreateCover` | Crea copertura | **Prep** | 10 | 75 | 3 | copertura bassa | 2 |
| `Action.ModifyArc` | Modifica arco | Blast | 40 | 75 | 3 | modifica collegamento | 2 |

> **Allineamento 2026-08-09 — `Action.CreateCover` risolve in `Prep`, non nel Blast**
> ([D-040](../decisions/RT_PDR_00_Decision_Log.md), E9.5).
>
> Questa riga diceva Blast mentre il [catalogo eroi](RT_HeroCatalog_v0.1.md) e il codice davano
> `Hero.Riktor.KineticPanel` in Prep. Prevale **Prep**, e la ragione è che eretta nel Blast la copertura
> arriverebbe **dopo** aver incassato i colpi di quel Blast — nel turno in cui la si paga non servirebbe a
> nulla. Il precedente opposto di `ModifyArc` (portata *nel* Blast a E9.4) non si applica: riguarda la
> **topologia**, e una copertura bassa non tocca né grafo né vista.
>
> Resta la portata **3** di questa riga: è il catalogo eroi ad essersi allineato, portando `KineticPanel` da 1
> a 3. Il prezzo è che il **bordo** va dichiarato nel piano, perché a portata 3 non è più derivabile dalla
> coppia (chi erige, cella bersaglio). Dettaglio in
> [`spec-coperture-temporanee-cp95.md`](../gameplay/spec-coperture-temporanee-cp95.md).

**Heal** — cura **20** HP, non supera la salute massima, non rimuove stati, può bersagliare se stessi.

**Create Cover** — copertura **bassa** su un bordo dichiarato, integrità **30**, durata **2 turni**, non
sovrapponibile. La variante di `Hero.Riktor.KineticPanel` sostituisce integrità e durata (rinforzato 45/1 turno ·
adattivo 25 e non scade). Fuori portata, bordo non dichiarato o già riparato → `Cancel`, con la sua voce di
TurnLog.

**Create Water** — acqua superficiale, raggio 1, durata **2 turni**, applica `Wet` alle unità presenti.

**Ignite** — cella in fiamme, durata base **2 turni** · non incendia automaticamente acqua o metallo · può
incendiare vegetazione, olio e gas.

**Electrify** — colpisce un bersaglio o una cella conduttiva · propagazione massima **3 celle** · danno iniziale
**20**, danno propagato **12** — *ora anche in colonna nella tabella, per
[D-115](../decisions/RT_PDR_00_Decision_Log.md): la rubrica dei radar li legge di lì e non deve estrarli da
questa frase* · ogni unità è colpita **una sola volta** dallo stesso evento.

**Create Cover** — copertura bassa su un **bordo** esagonale · integrità **30** · durata 2 turni · non può
sovrapporsi a una copertura esistente.

**Modify Arc** — apre/chiude una porta · crea un ponte temporaneo · blocca temporaneamente un collegamento ·
rende un arco conduttivo. **Ogni modifica incrementa la revisione del chunk della mappa** (invalidazione delle
cache di percorso: mai path fantasma).

---

## 7. Fallback

| FallbackId | Comportamento |
|---|---|
| `Fallback.Stop` | Si ferma all'ultima posizione valida |
| `Fallback.Wait` | Sostituisce l'azione con `Wait` |
| `Fallback.AttackCell` | Colpisce la cella pianificata |
| `Fallback.AttackTarget` | Segue il bersaglio, se ancora valido |
| `Fallback.BasicAttack` | Usa `BasicAttack` sul bersaglio valido più vicino |
| `Fallback.Cancel` | Non esegue nulla |

Assegnazione per il vertical slice: `Move` usa sempre **Stop** · gli AoE usano **AttackCell** · gli attacchi
diretti usano **Cancel** · le cure usano **Cancel** · **le reazioni non hanno fallback**.

> Il targeting automatico del «nemico più vicino» va evitato all'inizio: produce risultati poco leggibili, e la
> leggibilità tattica è un pilastro di prodotto.

---

## 8. Divergenze rispetto al PDF (dichiarate, non silenziose)

| # | PDF | Qui | Motivo |
|---|---|---|---|
| 1 | Fasi `Snapshot→Preparazione→Movimento→Controllo→Attacco→Ambiente→Cleanup`, movimento **prima** dell'attacco | Macro-fasi di Atlas `Prep → Dash → Blast → Move`, movimento **dopo** l'attacco; i codici restano come attributo | [ADR-0003 §1/§3](../decisions/adr-0003-modello-azioni-v01.md): l'attacco da fermo è l'identità tattica del gioco e la premessa della logica esistente (bot compreso) |
| 2 | Fase 20 unica per tutte le azioni di movimento | Fase 20 **sdoppiata**: mobilità rapida → `Dash`, percorso normale → `Move` | Conseguenza diretta di #1: non esistono azioni che risolvono «in mezzo» |
| 3 | Fase 30 «Controllo» come fase a sé | Controllo dentro il **Blast**, ordinato per priorità (10–50) prima del danno | Una macro-fase in più cambierebbe il TurnLog e il playback senza aggiungere espressività: la priorità intera basta |
| 4 | Fase 50 «Ambiente» come fase a sé | Propagazione ambientale nel **Cleanup**, prima dei KO | Stesso motivo di #3; dopo il Move, così colpisce anche chi è appena entrato |
| 5 | UE 5.6.x | UE **5.8.1** | Versione bloccata dal canone |
| 6 | Cooldown di `Action.Wait`, `Move`, `BasicAttack` non esplicitati per ogni riga (tabella disallineata nel PDF) | Ricostruiti per posizione e verificati contro le descrizioni testuali | Le tabelle del PDF sono estratte con colonne sfalsate; dove il numero era ambiguo si è preferita la descrizione a parole |
| 7 | `Fallback` e `bCanBeInterrupted` non dichiarati per **ogni** azione | Compilati per tutte, seguendo le regole generali del §12 del PDF | Il DoD del catalogo richiede che «ogni azione dichiari fase, priorità e fallback» |

**Non ancora deciso** (assente nel PDF, da fissare quando servirà): l'elenco puntuale delle azioni con
`bCanBeInterrupted = false` oltre a `Wait`, `Guard` e `SuppressiveLine`; qui sono marcate «no» solo dove il testo
lo implica.

---

## 9. Dove finisce questo catalogo

- **Data asset**: `PDA_Action_<Nome>` sotto `Content/RT/…` **feature-first** (le
  [convenzioni contenuti](../technical/tooling/convenzioni-contenuti-ue.md) prevalgono sul `Content/RefactorTactics/Data/` del PDF).
- **Validator**: CP 1.4 (issue `#30`) confronta i data asset con questo documento.
- **Motore azioni**: epic **E4** (`#41`–`#45`); reazioni: epic **E5** (`#50`–`#53`).
