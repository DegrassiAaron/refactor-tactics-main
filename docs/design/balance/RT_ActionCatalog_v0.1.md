# RT — Catalogo azioni v0.1

> **Fonte**: `docs/src/RefactorTactics — Catalogo e bilanciamento v0.1.pdf` §§1–3, §12 · `docs/PDR/RT_PDR_12_Catalog_v0.1.pdf`
> **Decisione abilitante**: [`adr-0003-modello-azioni-v01.md`](../adr-0003-modello-azioni-v01.md) · **Checkpoint**: CP 1.2 (issue `#28`)
> **Stato**: catalogo di riferimento per la release v0.1. Questi sono i **numeri vigenti**; le *decisioni* stanno
> nel canone ([`piano-canonico-mvp.md`](../piano-canonico-mvp.md)), lo *stato di avanzamento* nella
> [roadmap](../roadmap-checkpoint.md).

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
di movimento qui sotto dichiara esplicitamente quale delle due. Motivazione in [ADR-0003 §3](../adr-0003-modello-azioni-v01.md).

## Slot per turno

| Slot | Quantità | Esempi |
|---|---|---|
| Movimento | 1 | `Move` |
| Azione principale | 1 | `BasicAttack`, `Dash`, `Guard`, `Heal` |
| Reazione | 1 | `Counter`, `Intercept`, `Deflect` |
| Comunicazione | — | Ping, label |

> **Slot ≡ Action Points** (consolidato il 2026-08-07). Il workbook di bilanciamento
> `docs/RefactorTactics_Balance_Matrices_v0.1.xlsx` modella gli stessi slot come **risorse**: `RES_ACTION`
> (Action Points, cap **2**, nessun riporto fra turni) copre Movimento + Azione principale, `RES_REACTION`
> (cap **1**) è lo slot Reazione dell'ADR-0003. **Sono lo stesso sistema con due nomi**: la tabella qui sopra
> resta la formulazione canonica, «AP» è ammesso come sinonimo nei documenti di bilanciamento.
>
> **Risorsa firma** — ciò che l'MVP chiamava *energia* è per-personaggio, con nome e trigger di ricarica
> propri: `Flux` Carica Conduttiva · `Riva` Riserva Idrica · `Bastion` Integrità Strutturale · `Vektor`
> Slancio. Cap **4** per tutte (valore più basso fra i candidati), ricarica **1** sul trigger d'affinità.
> Cambia il nome e cosa la ricarica, **non** la regola.
| Conferma | illimitata (rate limit) | Ready |

Un piano completo dichiara: percorso di movimento · azione principale · reazione (se disponibile) · **facing
finale** · fallback.

---

## 1. Azioni fondamentali

| ActionId | Azione | Slot | Macro-fase | Cod. | Prio | Range | CD | Fallback | Interr. |
|---|---|---|---|---:|---:|---|---:|---|---|
| `Action.Wait` | Attesa | — | Move | 20 | 100 | — | 0 | — | no |
| `Action.Move` | Movimento | Movimento | **Move** | 20 | 50 | 5 MP | 0 | `Fallback.Stop` | sì |
| `Action.BasicAttack` | Attacco base | Principale | Blast | 40 | 50 | arma | 0 | `Fallback.Cancel` | sì |
| `Action.Guard` | Guardia | Principale | **Prep** | 10 | 40 | self | 0 | `Fallback.Cancel` | no |
| `Action.Activate` | Attiva | Principale | Blast | 40 | 70 | 1 | 0 | `Fallback.Cancel` | sì |
| `Action.Interact` | Interagisci | Principale | Blast | 40 | 80 | 1 | 0 | `Fallback.Cancel` | sì |

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

**Activate** — attiva un oggetto adiacente: porta · consolle · ascensore · generatore · sprinkler · ponte · obiettivo.

---

## 2. Azioni di movimento

Tutte le mobilità rapide risolvono in macro-fase **Dash** (prima del Blast): riposizionarsi in fretta è ciò che
permette di sparare da un'altra parte nello stesso turno. Solo `Action.Move` risolve in **Move**.

| ActionId | Azione | Slot | Macro-fase | Cod. | Prio | Distanza | CD | Fallback | Interr. |
|---|---|---|---|---:|---:|---|---:|---|---|
| `Action.Sprint` | Scatto lungo | Movimento + Principale | **Dash** | 20 | 60 | 8 MP | 0 | `Fallback.Stop` | sì |
| `Action.Dash` | Scatto | Principale | **Dash** | 20 | 30 | 3 celle | 1 | `Fallback.Stop` | sì |
| `Action.Charge` | Carica | Principale | **Dash** | 20/30 | 35 | 3 celle | 2 | `Fallback.Stop` | sì |
| `Action.Leap` | Balzo | Principale | **Dash** | 20 | 25 | 3 celle | 2 | `Fallback.Stop` | sì |
| `Action.Reposition` | Riposizionamento | Principale | **Dash** | 20 | 40 | 2 celle | 1 | `Fallback.Stop` | sì |

**Sprint** — fornisce 8 MP · consuma movimento **e** azione principale · non permette di preparare una reazione ·
applica `Status.Exposed` fino al Cleanup (**+5** al primo danno diretto ricevuto).

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
> meccanismo diverso da quello di `Guard`/`Deflect`, e usa una funzione diversa. L'**anti-spinta non ha limite
> di distanza**, a differenza di `Guard` che regge solo 1 cella — è ciò che distingue le due. Il **blocco del
> movimento** riusa `Status.Root`, quindi ferma anche lo scatto: chi si pianta per incassare non si riposiziona.

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
| `Action.Electrify` | Elettrifica | **Cleanup** | 50 | 30 | 4 | propagazione elettrica | 2 |
| `Action.CreateCover` | Crea copertura | Blast | 40 | 75 | 3 | copertura bassa | 2 |
| `Action.ModifyArc` | Modifica arco | Blast | 40 | 75 | 3 | modifica collegamento | 2 |

**Heal** — cura **20** HP, non supera la salute massima, non rimuove stati, può bersagliare se stessi.

**Create Water** — acqua superficiale, raggio 1, durata **2 turni**, applica `Wet` alle unità presenti.

**Ignite** — cella in fiamme, durata base **2 turni** · non incendia automaticamente acqua o metallo · può
incendiare vegetazione, olio e gas.

**Electrify** — colpisce un bersaglio o una cella conduttiva · propagazione massima **3 celle** · danno iniziale
**20**, danno propagato **12** · ogni unità è colpita **una sola volta** dallo stesso evento.

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
| 1 | Fasi `Snapshot→Preparazione→Movimento→Controllo→Attacco→Ambiente→Cleanup`, movimento **prima** dell'attacco | Macro-fasi di Atlas `Prep → Dash → Blast → Move`, movimento **dopo** l'attacco; i codici restano come attributo | [ADR-0003 §1/§3](../adr-0003-modello-azioni-v01.md): l'attacco da fermo è l'identità tattica del gioco e la premessa della logica esistente (bot compreso) |
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
  [convenzioni contenuti](../convenzioni-contenuti-ue.md) prevalgono sul `Content/RefactorTactics/Data/` del PDF).
- **Validator**: CP 1.4 (issue `#30`) confronta i data asset con questo documento.
- **Motore azioni**: epic **E4** (`#41`–`#45`); reazioni: epic **E5** (`#50`–`#53`).
