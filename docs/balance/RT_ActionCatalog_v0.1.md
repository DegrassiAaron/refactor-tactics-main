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
| **Interr.** | `InterruptPolicy` ([D-298](../decisions/RT_PDR_00_Decision_Log.md)): quanto dell'azione sopravvive a un `Action.Interrupt` — `None` (non la tocca) · `InterruptBeforeEffect` (non produce nulla) · `SuppressSecondary` (resta il primo effetto dichiarato) · `CancelChannel` (**riservata**, il validator la rifiuta) |

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
| Movimento | 1 | `Move` nei profili `Sneak`/`Move`/`Sprint` (§2.1) · `Dash`, `Charge`, `Leap`, `Reposition` (§2.2) |
| Azione principale | 1 | `BasicAttack`, `Guard`, `Heal`, **`Overwatch`** — mai due insieme |
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
> d'eroe: `Hero.Muiren.FluidTrail` è passata a **Movimento**, e l'invariante `Heroes.MobilityWithoutDamageIsNotMain`
> impedisce che la prossima nasca sulla principale, dove `MakeHeroAction` la metterebbe per default.

> **Slot ≡ Action Points** (consolidato il 2026-08-07). Il workbook `RefactorTactics_Balance_Matrices_v0.1.xlsx`
> — che è **`RESEARCH`**, non una fonte ([D-023](../decisions/RT_PDR_00_Decision_Log.md)): qui vale solo come
> nota terminologica — modella gli stessi slot come **risorse**: `RES_ACTION`
> (Action Points, cap **2**, nessun riporto fra turni) copre Movimento + Azione principale, `RES_REACTION`
> (cap **1**) è lo slot Reazione dell'ADR-0003. **Sono lo stesso sistema con due nomi**: la tabella qui sopra
> resta la formulazione canonica, «AP» è ammesso come sinonimo nei documenti di bilanciamento.
>
> **Risorsa firma** — ⛔ **voce di disegno, non di regola.** L'MVP chiamava *energia* un contatore per unità;
> [`D-265`](../decisions/RT_PDR_00_Decision_Log.md) ha deciso che **non esiste una risorsa firma universale** e
> [`D-324`](../decisions/RT_PDR_00_Decision_Log.md) ha tolto `Energy` dal gameplay ([#610](https://github.com/DegrassiAaron/refactor-tactics-main/issues/610)).
> Un kit **può** dichiarare una risorsa propria, con contratto di dati e validazione suoi: allora è quel kit a
> definirne nome, cap e trigger. I nomi per eroe — `Gadget` Carica Conduttiva · `Phase` Riserva Idrica ·
> `Riktor` Integrità Strutturale · `Wraith` Slancio — restano nel catalogo eroi come **disegno**, e il `Cap 4`
> con ricarica `1` è il parametro del modello scartato: va rimotivato, non ereditato.

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

> 🔵 **Sette è l'elenco canonico; a entrare nel kit di ogni unità sono cinque** — `Wait`, `Guard`, `Brace`,
> `Overwatch`, `Interact` — e le due che mancano non mancano davvero: `Move` passa da `PlannedPath` e non da
> uno slot azione, `BasicAttack` è **l'indice 0** del kit di ogni eroe. Le accoda
> `URTCatalogLibrary::MakeGenericActions`, sempre **in coda**, perché `PlannedAbilityIndex` è un indice.
>
> `Interact` è entrata il **2026-08-26**: era fuori perché *«nessun codice risolve un'interazione»*, e quel
> motivo è scaduto con [D-148](../decisions/RT_PDR_00_Decision_Log.md)/[D-151](../decisions/RT_PDR_00_Decision_Log.md)
> — agisce sulle porte, e solo su quelle. ⏱️ *Diceva «apre le porte» fino al 2026-09-05: da
> [D-331](../decisions/RT_PDR_00_Decision_Log.md) (`#2380`, chiude `INT-7`) l'azione **commuta** — apre una porta
> chiusa e chiude una aperta — mentre `Locked` e `Destroyed` sono rifiuti con reason code distinti.* Il criterio
> d'ammissione è sempre lo stesso: *si aggiunge la generica quando l'altra metà esiste*. Storia e riserve in
> [`../gameplay/brief-azioni-generiche-overwatch.md`](../gameplay/brief-azioni-generiche-overwatch.md).
>
> ⌨️ **E dal 2026-08-26 il giocatore le raggiunge tutte**: i tasti abilità sono `1`–`9` più `0`, dieci
> posizioni contro le dieci voci del kit. Prima erano quattro, e `Overwatch`, `Guard`, `Brace`, `Wait` e la
> reazione di tre eroi su quattro le usava solo il bot.

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
| `MovementMode.Sneak` | **×0,5** | Cadenza **1 passo ogni 2 tick**, **sempre silenzioso** indipendentemente dal terreno. ✅ La domanda `AE-5` — *«Con quali numeri esiste il profilo `Sneak`?»* — è **chiusa dal 2026-09-13** da [D-412](../decisions/RT_PDR_00_Decision_Log.md), che risponde a tutte e tre: da allora il profilo è anche **pianificabile** |
| `MovementMode.Move` | **×1** | il profilo neutro |
| `MovementMode.Sprint` | **×2** | conserva un trade-off reale, vedi sotto |
| `MovementMode.Withdraw` | **×0,25** | **non si sceglie**: lo impone l'`Overwatch` ([D-070](../decisions/RT_PDR_00_Decision_Log.md)) |

> 🔄 **I quattro budget erano assoluti — `Move` 5 · `Sprint` 8 · `Withdraw` 2 — fino a
> [D-412](../decisions/RT_PDR_00_Decision_Log.md) (2026-09-13), che li rende MOLTIPLICATORI del budget base
> dell'unità**, arrotondati per difetto: con un eroe da 5, `Sprint` vale 10 e `Withdraw` **1**, non 2. La
> decisione nomina queste righe fra ciò che sostituisce, e il codice dei profili le applica già
> (`RTMovementProfileLibrary.cpp`, `/*×2*/ 200` · `/*×0,25*/ 25` · `/*×0,5*/ 50`). ⚠️ Gli assoluti
> sopravvivono in `Action.Sprint.RangeCells` e `Action.Withdraw.RangeCells`, che sono una **seconda sede** e
> un debito aperto: [#3198](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3198).

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
> macro-fase **`Dash`** (§2.2), concesso anche da `Hero.Muiren.FlowReaction` e `Hero.Wraith.Feint`. Due entità con lo
> stesso nome in due fasi diverse si pagano a ogni lettura del TurnLog, non una volta sola.

**`Sprint` non è un `Dash`.** È il profilo lungo del movimento normale, quindi risolve dopo il Blast: non
permette di sparare da un'altra posizione nello stesso turno, che è precisamente ciò che un `Dash` fa.

> ✅ **Codice migrato il 2026-09-12, divergenza chiusa il 2026-09-18** — due date, e tenerle distinte è il
> punto: fra l'una e l'altra il documento ha dichiarato il falso. La storia resta scritta perché è la ragione
> per cui questa sezione ha la forma che ha. Fino ad allora il codice teneva `Action.Sprint` in
> `ERTResolutionPhase::FastMovement` (fase `Dash`): divergenza misurata il **2026-08-08**, tracciata in
> [`../DOC_CONFLICT_MATRIX.md`](../DOC_CONFLICT_MATRIX.md) riga 41. Oggi il codice dichiara
> `ERTResolutionPhase::NormalMovement`, cioè la fase `Move` che questa sezione descrive
> ([D-116](../decisions/RT_PDR_00_Decision_Log.md) voce 1, [#641](https://github.com/DegrassiAaron/refactor-tactics-main/issues/641)).
>
> ⚠️ **Il documento è rimasto indietro sei giorni, e nessuno se n'era accorto.** §2.2 ha continuato a
> dichiarare `Dash` fino al **2026-09-18**, quando il gate catalogo↔C++ di
> [#2578](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2578) ha confrontato i due lati per
> la prima volta e ha trovato il ritardo ([#3186](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3186)).
> È il difetto che quel gate esiste per vedere: una decisione eseguita nel codice e non arrivata
> all'autorità che [D-023](../decisions/RT_PDR_00_Decision_Log.md) le assegna.
>
> 🔄 **Questa riga è stata vera, poi falsa, e dal 2026-08-12 è di nuovo vera.** In mezzo,
> [D-068](../decisions/RT_PDR_00_Decision_Log.md) aveva **rovesciato** la decisione — «Sprint resta pre-Blast,
> ed è una decisione, non un arretrato» — lasciando però il paragrafo qui sopra al suo posto: per quattro
> giorni il catalogo ha dichiarato una regola che il canone aveva appena abbandonato, e nessun gate poteva
> accorgersene. [D-116](../decisions/RT_PDR_00_Decision_Log.md) ha rovesciato di nuovo, e per un motivo che
> non era sul tavolo nel 2026-08-10: restando pre-Blast lo Sprint **spara da una posizione nuova**, cioè fa
> ciò che questa stessa sezione attribuisce al `Dash`. La migrazione della sola `ResolutionPhase` era lavoro
> di **E38**, ed è stata eseguita il 2026-09-12 con #641.
>
> ⚠️ **E non si è migrata da sola.** Portare lo Sprint dopo il Blast rende `Status.Exposed` **inerte** —
> verrebbe applicato quando tutti hanno già sparato, e scadrebbe nel Cleanup subito dopo. Per questo D-116 lo
> porta a **`Turni 2`** nello stesso momento — ed è il valore che il codice dichiara oggi
> (`RTCatalogLibrary.cpp`, `FRTActionEffectSpec(… TAG_Status_Exposed, /*Turni*/ 2)`).
>
> ⛔ **La terza voce di D-116 NON è arrivata, e va detto qui invece che scoperto dopo**: la compatibilità
> fra azioni e profilo di movimento
> ([`../gameplay/spec-compatibilita-azioni-movimento.md`](../gameplay/spec-compatibilita-azioni-movimento.md)):
> è ancora **da implementare** — la sua spec lo dichiara di sé (*«nessuna riga di codice la esprime oggi»*) e
> `RTMovementProfileLibrary.cpp:83` misura che *«nessuna azione dichiara oggi un `MinStability`»*. Senza di
> essa lo Sprint conserva due prezzi su tre, e la riserva di D-015 — **un `Move` più lungo che costa solo la
> reazione** — non è ancora del tutto scongiurata.
>
> ✅ **Il divieto di reazione invece regge, ma per una via nuova**: uscendo da `FastMovement` lo Sprint non
> passava più dal punto che lo applicava, e [D-405](../decisions/RT_PDR_00_Decision_Log.md) lo ha spostato sul
> **piano validato**. Senza quella voce il divieto non sarebbe arrivato tardi: non sarebbe arrivato.
>
> **Il trade-off dello Sprint va migrato, non perso.** Oggi paga con `Status.Exposed` e con la rinuncia alla
> reazione. Nel modello a profili non può diventare un potenziamento gratuito del `Move`: se perde il costo di
> slot deve conservare un costo, altrimenti nessuno sceglierebbe più `Move`.

`Action.Sprint` è l'azione che il resolver costruisce per il profilo lungo, e la sua riga sta qui — non fra
le mobilità rapide di §2.2, dove è stata fino al 2026-09-18. ⚠️ **Non è l'unica azione di questa famiglia**:
`Action.Move` è dichiarata fra le generiche (§1), perché ogni unità la possiede.

| ActionId | Azione | Slot | Macro-fase | Cod. | Prio | Range | CD | Rumore | Fallback | Interr. |
|---|---|---|---|---:|---:|---|---:|---:|---|---|
| `Action.Sprint` | Scatto lungo | **Movimento** | **Move** | 20 | 60 | 8 MP ⚠️ | 0 | 5 | `Fallback.Stop` | sì |
| `Action.Withdraw` | Ripiegamento | **Movimento** | **Move** | 20 | 50 | 2 MP ⚠️ | 0 | — | `Fallback.Stop` | sì |

> 🔄 **La riga di `Action.Sprint` stava in §2.2 fino al 2026-09-18**, dove dichiarava macro-fase `Dash` con
> una ⚠️ che rimandava a D-116. La migrazione è stata eseguita il 2026-09-12 e la riga è tornata dove la sua
> fase la colloca: `Sprint` è un profilo del movimento normale, non una mobilità rapida
> ([#3186](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3186)).
>
> 🔴 **`Action.Withdraw` è entrata il 2026-09-18** ([#3187](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3187)):
> `GetCoreActionCatalog()` la costruisce dal **2026-09-13**, e nessuna tabella di questo catalogo la
> dichiarava. Il **profilo** `MovementMode.Withdraw` era già qui sopra — è l'**azione** che lo nomina a
> mancare, cioè il modo di metterlo in un piano ([D-070](../decisions/RT_PDR_00_Decision_Log.md)). I valori
> sono quelli che il codice usa oggi: questa riga trasferisce un'autorità, non cambia un bilanciamento.
>
> 🔴 **La storia della cella «Slot» di `Sprint`, che viaggia con la riga.** Fino al 2026-08-26 diceva «Movimento +
> Principale ⚠️», contraddicendo [D-028](../decisions/RT_PDR_00_Decision_Log.md) e il dato: in
> `URTCatalogLibrary::GetCoreActionCatalog` lo scatto lungo entra con `ERTActionSlot::Movement`, e
> `RefactorTactics.Actions.PrecisionAttack.WeaponRangePlusOne` verifica che la principale resti libera.
>
> ⚠️ **La ⚠️ sulla cella «Range» dice che quel numero è vivo nel codice e morto nel modello.** `8 MP` è
> `Action.Sprint.RangeCells`, l'assoluto che il resolver del Dash leggeva. Il budget effettivo è ora un
> **moltiplicatore** del budget dell'unità — `Sprint` **×2** — da
> [D-412](../decisions/RT_PDR_00_Decision_Log.md), che nomina proprio queste righe fra gli assoluti che
> sostituisce. La cella riporta l'assoluto perché è ciò che il C++ dichiara, e il confronto catalogo↔codice
> misura quello; togliere il numero morto è lavoro di
> [#3198](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3198). ⚠️ **Vale per entrambe le
> righe**: `Withdraw` è **×0,25**, cioè `1` per un eroe da 5, non i `2 MP` che `RangeCells` conserva.

### 2.2 Mobilità speciali — fase **Dash**

`Dash` · `Charge` · `Leap` · `Reposition` risolvono in macro-fase **Dash**, prima del Blast: riposizionarsi in
fretta è ciò che permette di sparare da un'altra parte nello stesso turno.

**E occupano tutte lo stesso slot: il movimento** ([D-191](../decisions/RT_PDR_00_Decision_Log.md)). Chi
risolve nella fase Dash si è mosso, e ha speso per questo lo slot movimento — schivi e ti resta l'azione
principale, ma non ti muovi ancora. Che una mobilità faccia danno a chi trapassa, a chi raggiunge o a nessuno
non cambia *che cosa* ha speso.

> 🔴 **Corretto il 2026-08-25.** Questo capoverso diceva che `Charge` è *«un attacco che ti porta addosso al
> bersaglio: occupa la principale, e il movimento ti resta»*, seguendo la clausola di
> [D-028](../decisions/RT_PDR_00_Decision_Log.md) che [D-191](../decisions/RT_PDR_00_Decision_Log.md) ha
> superato. La misura che ha deciso: con la carica sulla principale, Riktor pianificava `Ram` **e** l'attacco
> base e il resolver eseguiva entrambe — 28 danni, due azioni principali. E il movimento *non* restava:
> `ResolveDash` lo toglie a ogni mobilità rapida, quindi la carica costava già il movimento senza dichiararlo.
>
> Chi vuole che una mobilità costi **anche** la principale lo dichiara con `MovementAndMain`.

| ActionId | Azione | Slot | Macro-fase | Cod. | Prio | Distanza | CD | Rumore | Fallback | Interr. |
|---|---|---|---|---:|---:|---|---:|---:|---|---|
| `Action.Dodge` | Scatto | **Movimento** | **Dash** | 20 | 30 | 3 celle | 1 | 6 | `Fallback.Stop` | sì |
| `Action.Charge` | Carica | **Movimento** | **Dash** | 20/30 | 35 | 3 celle | 2 | — | `Fallback.Stop` | sì |
| `Action.Leap` | Balzo | **Movimento** | **Dash** | 20 | 25 | 3 celle | 2 | — | `Fallback.Stop` | sì |
| `Action.Reposition` | Riposizionamento | **Movimento** | **Dash** | 20 | 40 | 2 celle | 1 | — | `Fallback.Stop` | sì |

> 🔄 **`Action.Sprint` non è più in questa tabella, dal 2026-09-18**: risolve in macro-fase `Move` da
> [D-116](../decisions/RT_PDR_00_Decision_Log.md)/[#641](https://github.com/DegrassiAaron/refactor-tactics-main/issues/641)
> (codice, 2026-09-12) e la sua riga è in **§2.1**, con gli altri profili del movimento normale. Qui restano
> `Dodge`, `Charge`, `Leap` e `Reposition`, che sono le mobilità rapide; lo scatto lungo non è una di esse —
> è la distinzione che §2.1 chiama strutturale.
>
> La storia della sua cella «Slot» si legge ora accanto alla riga, in §2.1.

**Sprint** — fornisce 8 MP · occupa il **solo slot movimento** ([D-028](../decisions/RT_PDR_00_Decision_Log.md),
coerente con D-015) · non permette di preparare una reazione · applica `Status.Exposed` (**+5** al primo danno diretto ricevuto) per **2 turni**, come [D-116](../decisions/RT_PDR_00_Decision_Log.md) prescrive e come il codice dichiara dal 2026-09-12 — non è un ribilanciamento, è la contropartita della migrazione di fase: con lo Sprint dopo il Blast, un `Exposed` che scade nel Cleanup dello stesso turno non incontrerebbe mai un attacco. ⚠️ **E lo Sprint risolve in `Move`**: la sua riga di tabella è in §2.1, questo capoverso resta qui perché il confronto col `Dash` è ciò che lo spiega.

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
| `Action.Mortar` | Mortaio | Blast | 40 | 65 | 12 | cella, raggio 1, tiro indiretto | 3 | `Fallback.AttackCell` | sì |
| `Action.SuppressiveLine` | Linea di soppressione | **Prep** | 10/20 | 30 | 16 | linea / reazione | 2 | — | no |
| `Action.MarkTarget` | Marchia bersaglio | Blast | 40 | 40 | 0 | bersaglio | 1 | `Fallback.Cancel` | sì |

> **`SuppressiveLine` non si fonde con `Overwatch`.** Si somigliano — entrambe si armano in `Prep` e reagiscono
> a un passaggio — ma non sono la stessa cosa: `Overwatch` è l'**azione universale** e l'infrastruttura di
> controllo reattivo, `SuppressiveLine` è un **contenuto specifico** con effetti propri (linea, 16 danni,
> cooldown 2). Se un giorno il codice dimostrasse che è solo un duplicato nominale dell'Overwatch, la
> conclusione sarebbe una **issue di refactor**, non una cancellazione durante un riordino documentale.

**Mortar** — **tiro indiretto**: **non richiede linea di vista**
(`ERTLineOfSightPolicy::NotRequired`, [D-380](../decisions/RT_PDR_00_Decision_Log.md)) · portata **4** dal
centro d'area, la stessa di `CircularAoE` · **raggio 1** come lei · conta come **aggressione dichiarata**
(`INT-8`), quindi rompe una tregua come un attacco diretto. È l'unica **azione core** con quella politica;
`Hero.Branth.MortarShot` la eredita, e `Hero.Muiren.MistVeil` ce l'ha per conto suo.

> 🔴 **Entrata in questo catalogo il 2026-09-18** ([#3187](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3187)),
> dove il codice la costruiva già. ⚠️ **La portata resta 4 e non è una svista**: un mortaio è un'arma di
> distanza, e accorciarlo per punirlo contraddirebbe ciò che l'azione è — il tiro indiretto si paga con la
> **potenza** (12: meno di ogni altra offensiva che infligge danno, `MarkTarget` a parte, che ne dichiara 0
> perché marchia e basta) e con l'**attesa** (cooldown 3, il più lungo di §3), non con l'avvicinamento.
> ⛔ Non rende blind fire nessun'altra azione: `CircularAoE`, `LineAttack`, `Hero.Aevik.Overload` e
> `Hero.Muiren.CircularTide` restano `Required`.
>
> 🔑 **E il kit di Branth la porta a 3, non a 4 — una taratura d'eroe, con una misura dietro.** Branth
> ingaggia a 3 (`ImpactShot`), e con un mortaio da 4 il bot smetteva di chiudere: misurato il 2026-09-10,
> cadevano tre gate anti-stallo — `Bot.StallDefinitionsOnTheGeneratedTestArena`,
> `Match.Autobattle.EngagesOnTheGeneratedTestArena` e `Match.Autobattle.NobodyParksOnTheAuthoredMap` — tutti
> e tre verdi di nuovo con la portata 3. Il catalogo dice **cosa l'azione è**, il kit **come quell'eroe la
> porta**: chi domani desse il mortaio a un eroe che ingaggia a 4 non ha bisogno di quella riduzione.

**Precision Attack** — range dell'arma **+1** · ignora la copertura bassa · **utilizzabile dopo lo Sprint**.

> 🔴 **Questa riga diceva «*non* utilizzabile dopo Sprint» fino al 2026-08-26**, ed era la regola di
> prima di [D-028](../decisions/RT_PDR_00_Decision_Log.md): quando lo scatto lungo occupava anche la
> principale, l'attacco non trovava più lo slot. Con lo Sprint sul **solo** movimento il piano è legale —
> *corro e sparo* — e non per un'eccezione sull'`ActionId`: lo decide la stessa regola di slot che vieta due
> principali. Lo pinna `RefactorTactics.Actions.PrecisionAttack.WeaponRangePlusOne`, che verifica
> `ValidateActionSlots({ Sprint, PrecisionAttack })` **senza errori**.

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

⚠️ **Non tutte le righe di questa sezione sono reazioni.** La colonna «Slot» è la distinzione che conta:
`Counter`, `Intercept`, `Deflect`, `Anchor`, `Purge` ed `Evade` occupano lo slot **Reazione** (0-1 per turno,
trigger valutato su un punto di passaggio della risoluzione); `Brace`, `Shield` e `Cleanse` sono azioni
**Principali** che si dichiarano e basta, senza trigger. Stare nella stessa sezione del catalogo non le rende
lo stesso tipo di cosa.

⚠️ **E il punto di passaggio non è lo stesso per tutte.** Cinque si valutano dentro il Blast; `Evade` no —
il suo trigger è la cella che diventa pericolosa, e quella nasce nel **Cleanup**
(`URTReactionLibrary::PassPointFor` → `CleanupSurfaceBirth`). La **fase dell'azione** resta `Control` come
per le altre: è il punto di valutazione a essere diverso, e le due cose non vanno confuse.

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
| `Action.Anchor` | Ancoraggio | Reazione | Blast | 30 | 5 | 0 | annulla lo spostamento | 2 |
| `Action.Purge` | Epurazione | Reazione | Blast | 30 | 5 | 0 | annulla lo stato di controllo | 2 |
| `Action.Evade` | Elusione | Reazione | Blast (valutata nel Cleanup) | 30 | 5 | 0 | si sposta di una cella | 2 |
| `Action.Brace` | Irrigidimento | Principale | **Prep** | 10 | 30 | 0 | anti-spinta | 1 |
| `Action.Shield` | Scudo | Principale | **Prep** | 10 | 35 | 0 | scudo temporaneo 25 | 2 |
| `Action.Cleanse` | Purifica | Principale | Blast | 30 | 25 | 0 | rimozione stato | 2 |

> La colonna **Range** non era nella tabella originale (il PDF dice «entro il range consentito», senza numero):
> i valori sono decisi in CP 5.2. `0` = su se stessi, che è il caso di tutte tranne `Intercept` (2 celle, questo
> sì dichiarato dal testo). Per `Counter`, `0` significa che il contrattacco raggiunge chi ha colpito, chiunque
> sia: inventare una portata cambierebbe *quali* attacchi si possono punire, e il catalogo non la fornisce.

> 🔴 **`Anchor`, `Purge` ed `Evade` sono entrate in questo catalogo il 2026-09-18**
> ([#3187](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3187)): `GetCoreActionCatalog()` le
> costruisce già (CP 7.5, [#505](https://github.com/DegrassiAaron/refactor-tactics-main/issues/505)) e
> nessuna tabella le dichiarava. I valori sono quelli che il codice usa oggi — queste righe trasferiscono
> un'autorità, non cambiano un bilanciamento.
>
> 🔑 **Esistono come azioni core, e non solo come moduli di equipaggiamento**, perché è dall'azione core che
> un modulo eredita ciò che lo rende una reazione: fase, priorità e soprattutto il **trigger**.

**Anchor** — trigger: l'unità **sta per essere spostata** da una spinta o da una trazione di questo Blast
(`AboutToBeDisplaced`). È la reazione core dello spostamento: chi la dichiara non viene spostato.

**Purge** — trigger: l'unità **sta per ricevere uno stato di controllo** (`AboutToReceiveControl`), che
viene annullato. ⚠️ **Non è `Cleanse`, e la differenza non è cosmetica**: `Action.Cleanse` è un'azione
**Principale** che rimuove uno stato **già addosso** e ne dichiara la priorità in pianificazione; qui lo
stato lo determina l'evento, e con più controlli si annulla il più grave.

**Evade** — trigger: la cella sotto l'unità **diventa pericolosa** (`CellBecameHazardous`), e l'unità si
sposta di una cella. È la reazione core dell'ambiente.

> **Priorità 5, la stessa per tutte e tre, e più bassa di ogni altra reazione.** Non dichiara una
> precedenza: dichiara che risolvono in un punto del turno tutto loro, dove le altre non arrivano — non
> producono colpi e non riducono danno — quindi non contendono niente a nessuno. La fase resta `Control`
> come le altre reazioni: a portare `Evade` nel Cleanup è il **punto di valutazione**
> (`URTReactionLibrary::PassPointFor`), non la fase dell'azione, e sono due cose diverse.

**Counter** — trigger: l'eroe è colpito da un attacco **diretto** entro il range consentito. Esegue un attacco da
**16** danni *dopo* l'attacco ricevuto · non si attiva contro danni ambientali · una sola attivazione.
Il contrattacco entra **in coda** agli attacchi del Blast: non consuma i modificatori "primo colpo"
(Guard/Exposed/Deflect) di chi lo subisce, che restano per l'attacco pianificato.

**Intercept** — trigger: un alleato entro **2** celle è bersagliato da un attacco diretto. L'intercettore
**diventa** il bersaglio; la traiettoria deve essere compatibile · non intercetta AoE né hazard · una sola attivazione.

> La traiettoria che deve essere libera è quella dall'attaccante **all'intercettore**, non alla vittima: ci si
> mette in mezzo a un colpo, non lo si teletrasporta addosso. La priorità **10** — la più bassa fra le reazioni che producono un effetto sul colpo, sotto la sola terna `Anchor`/`Purge`/`Evade` che annulla —
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
> eredita i suoi `20 danni + Push 1`), `Hero.Muiren.PressureJet` e la variante `Hero.Muiren.CircularTide.Impact`.
> L'elenco è **esaustivo e senza eccezioni in tutto il progetto**: l'ultimo `Push 2` era `Guardian.Sweep`, ed
> è sparito insieme agli archetipi legacy (`#426`, 2026-08-10) — quando questa nota è stata scritta esisteva
> ancora, fuori dal roster, e la riga lo dichiarava come eccezione. Ora nessuna azione, in nessun catalogo,
> spinge di più di una cella. `Guard` resiste fino a 1 cella, quindi copre per intero lo spazio degli
> spostamenti prodotti **dalle azioni**.
>
> 🔵 **La spinta forte esiste nei DATI, e non da qui: dall'equipaggiamento** (corretto il 2026-08-16).
> `Weapon.Impact` porta lo spostamento di `Hero.Muiren.PressureJet` a **2** ([D-085](../decisions/RT_PDR_00_Decision_Log.md))
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

**Cleanse** — rimuove **un solo** stato, e soltanto fra quelli che **il piano ha elencato**. La priorità
di rimozione è scelta dal giocatore **durante il planning** (non a runtime: nessuna scelta implicita).

> **Limite v0.1, riscritto il 2026-08-27** ([D-211](../decisions/RT_PDR_00_Decision_Log.md)) — la riga precedente diceva
> *«oggi `Cleanse` opera sui soli `Rooted`/`Marked`/`Exposed`, perché `Burning` ed `Electrified` non esistono
> come stato di unità»*, e sbagliava due volte: `Burning` **esiste** come stato di unità, e il limite non è un
> insieme fisso di stati.
>
> **L'insieme rimovibile lo dichiara il piano**, tag per tag: `ARTUnit::PlannedCleansePriority` è insieme
> l'**ordine** e il **filtro** — un tag che l'unità possiede ma che il piano non elenca **non si toglie**
> (`Reactions.Cleanse.NoImplicitChoice`). Senza lista non rimuove nulla (fail-closed): *«nessuna scelta
> implicita»* significa che il resolver non sceglie al posto del giocatore neppure quando il candidato
> sarebbe uno solo.
>
> 🔴 **E quella lista non ha produttori**: la scrivono solo i test. In partita `Cleanse` non rimuove niente,
> paga il cooldown ([D-200](../decisions/RT_PDR_00_Decision_Log.md)) e lascia una voce `NoEffect`. L'argomento sta in
> [D-211](../decisions/RT_PDR_00_Decision_Log.md) e nella riga **78** di [`DOC_CONFLICT_MATRIX.md`](../DOC_CONFLICT_MATRIX.md), e **non si
> duplica qui**.

---

## 5. Azioni di controllo

Tutte le azioni di controllo risolvono dentro il **Blast**, prima del danno, per priorità (il controllo non è una
macro-fase separata: ADR-0003 §3).

| ActionId | Azione | Macro-fase | Cod. | Prio | Range | Effetto | Durata | CD |
|---|---|---|---:|---:|---:|---|---|---:|
| `Action.Push` | Spinta | Blast | 30 | 40 | 1 | spinta 1 | istantanea | 1 |
| `Action.Pull` | Trazione | Blast | 30 | 40 | 2 | trazione 1 | istantanea | 1 |
| `Action.Root` | Radicamento | Blast | 30 | 25 | 1 | blocca il movimento | 1 turno | 2 |
| `Action.Interrupt` | Interruzione | Blast | 30 | 20 | 1 | annulla — o **degrada** — un'azione compatibile, secondo la sua `InterruptPolicy` ([D-300](../decisions/RT_PDR_00_Decision_Log.md)) | istantanea | 2 |
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

**Interrupt** — un'azione è interrompibile **solo** se la sua `InterruptPolicy` non è `None`. Non tutte lo sono, e da [D-300](../decisions/RT_PDR_00_Decision_Log.md) «interrompibile» non significa più «cancellabile»: `Action.Charge` dichiara `SuppressSecondary` e, interrotta, colpisce senza spingere.
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
| `Action.CreateSmoke` | Crea fumo | **Cleanup** | 50 | 60 | 4 | fumo raggio 1, durata 2 turni | 2 |
| `Action.Electrify` | Elettrifica | **Cleanup** | 50 | 30 | 4 | 20 danni, propagazione elettrica 12 | 2 |
| `Action.CreateCover` | Crea copertura | **Prep** | 10 | 75 | 3 | copertura bassa | 2 |
| `Action.ModifyArc` | Modifica arco | Blast | 40 | 75 | 3 | modifica collegamento | 2 |

> 🔴 **`Action.CreateSmoke` è entrata il 2026-09-18**
> ([#3187](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3187)), dove il codice la
> costruiva già: portata, priorità e cooldown di `Ignite` e `CreateWater`, le altre due ambientali, e **la
> stessa superficie** che `MistVeil` dichiara — non c'è un secondo fumo. La **durata 2 turni** è quella che
> `ARTTurnManager::ApplyDynamicSurface` applica a ogni superficie creata, la stessa di `Create Water` e
> `Ignite`.
>
> ⚠️ **La sua fase dice `50` e non `40/50`, a differenza delle due sorelle, e la differenza è voluta**: il
> C++ costruisce tutte e tre — `CreateSmoke`, `Ignite`, `CreateWater` — con `ERTResolutionPhase::Environment`
> e nient'altro, esattamente come `Electrify`, che infatti in questa tabella dichiara `50`. Il doppio codice
> `40/50` descritto qui sopra è un modello che il codice **non** implementa: allinearvi anche la riga nuova
> avrebbe aggiunto una terza dichiarazione a un'intenzione che nessuno ha ancora deciso di realizzare. La
> domanda — catalogo o codice — è in
> [#3200](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3200).
>
> ⚠️ **Il fumo NON blocca la linea di vista**, e chi legge «fumo» può aspettarselo: il catalogo terreni gli
> dà `bBlocksLineOfSight = false`, portata di targeting **2** attraverso, e `Status.Obscured` a chi entra.
> Quella regola vale per **ogni** cella di fumo, da qualunque azione venga, ed è là che vive.

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
| 7 | `Fallback` e `InterruptPolicy` non dichiarati per **ogni** azione | Compilati per tutte, seguendo le regole generali del §12 del PDF | Il DoD del catalogo richiede che «ogni azione dichiari fase, priorità e fallback» |

**Non ancora deciso** (assente nel PDF, da fissare quando servirà): l'elenco puntuale delle azioni con
`InterruptPolicy = None` oltre a `Wait`, `Guard` e `SuppressiveLine`; qui sono marcate «no» solo dove il testo
lo implica.

---

## 9. Dove finisce questo catalogo

- **Data asset**: `PDA_Action_<Nome>` sotto `Content/RT/…` **feature-first** (le
  [convenzioni contenuti](../technical/tooling/convenzioni-contenuti-ue.md) prevalgono sul `Content/RefactorTactics/Data/` del PDF).
- **Validator**: CP 1.4 (issue `#30`) confronta i data asset con questo documento.
- **Motore azioni**: epic **E4** (`#41`–`#45`); reazioni: epic **E5** (`#50`–`#53`).
