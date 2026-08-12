# Brief — Conoscenza parziale (visibilità, contatto, ultimo avvistamento)

> **Stato**: brief di requisiti, **non** spec implementativa · **Data**: 2026-08-07 · **Origine**: `/sc:brainstorm`
> **Cosa è**: il perimetro deciso in sessione per portare la *conoscenza parziale* nella v0.1.
> **Cosa non è**: stealth. Nessuna firma visiva, nessun punteggio di rilevamento, nessun sensore — vedi §9.
> **Autorità**: subordinato a [`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) e [`adr-0003-modello-azioni-v01.md`](../decisions/adr-0003-modello-azioni-v01.md).
> Diventa vincolante solo quando i CP di §7 entrano in [`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md).

> ⚠️ **`D1`…`D15` qui sotto sono ID LOCALI di questo brief, non del [Decision Log](../decisions/RT_PDR_00_Decision_Log.md).**
> Le sigle **collidono**: il `D14` di questo brief è la propagazione a flood fill, il `D-014` globale sono le
> azioni generiche canoniche. Nel dubbio, il trattino distingue: `D-0xx` è globale e vincolante, `Dxx` è locale
> e vale solo dentro il brief che lo contiene.

> 🧭 **[ADR-0005](../decisions/adr-0005-orientamento.md) è prerequisito di questo slice** (2026-08-07). La
> vista **non** è omnidirezionale: piena nell'**arco frontale** fino a `Vista`, più **consapevolezza a 360°
> entro 2 celle** (lo stesso cap del fumo). La primitiva è una sola — `URTHexLibrary::HexCone` — condivisa da
> difesa, percezione e Overwatch. Dove il testo qui sotto assume una vista a 360° piena, va letto come il
> comportamento *precedente* ad ADR-0005: la catena è **E16 → E13 → E14**.

## 1. Le tre nozioni, e quale entra adesso

Il design di partenza separa correttamente tre cose che di solito vengono confuse:

| Nozione | Domanda | Stato |
|---|---|---|
| **Linea di vista** | la cella è geometricamente osservabile? | ✅ esiste: `URTHexVisionLibrary::HasLineOfSight` |
| **Rilevamento** | il personaggio percepisce che qualcosa c'è? | 🟡 **entra ora**, su due canali: vista e **udito** |
| **Identificazione** | capisce chi è e cosa sta facendo? | ⏳ rimandata (§9) |

Lo slice adotta **tre stati** invece di cinque: `Nascosto`, `ContattoIncerto`, `Rilevato` — più la memoria
`UltimoContatto`. `Identificato` richiede la firma visiva e resta fuori.

> **Revisione del 2026-08-07**: il livello `ContattoIncerto`, inizialmente escluso, **rientra** perché il
> [sistema di rumore](../archive/src/design/rumore-e-percezione-acustica.md) lo richiede per costruzione — vedi §12. Un
> rumore che rivela la cella esatta non è rumore. La conoscenza smette quindi di essere binaria, e il canale
> acustico costa poco proprio perché condivide questo modello con la vista.

## 2. Decisioni prese in sessione

| # | Decisione | Motivo |
|---|---|---|
| **D1** | **Slice minimo dentro la v0.1**, non epic completa | La v0.1 ha già 59 CP col rischio scope dichiarato H/H. Entra ciò che dà significato a una statistica **già a catalogo** (range visivo), non un sistema nuovo |
| **D2** | La visibilità è **vista derivata**: funzione pura di `(stato unità, mappa)` | Non entra nel checksum né nel formato serializzato; ricalcolabile e testabile headless. Coerente con l'invariante #4 |
| **D3** | L'**ultimo contatto** è memoria, e vive in un campo **per squadra nello snapshot**, con versione di formato incrementata | L'unica cosa non deducibile dallo stato corrente. Esplicito e nel TurnLog invece che in un componente a lato che potrebbe divergere dal replay |
| **D4** | La visibilità si ricalcola ai **confini di fase**, non a ogni micro-step | Il Dash precede il Blast: riposizionarsi *può* ancora aprire o chiudere una linea prima degli attacchi — il 90% del valore tattico, senza toccare «raccogli poi applica» (invariante #3) |
| **D5** | Il **targeting** è limitato alla conoscenza **di squadra**, non individuale | È la «copertura informativa»: chi vede più lontano estende la portata utile di chi vede meno. Emerge dai numeri esistenti, senza una regola nuova |
| **D6** | Il **bot** pianifica sulla **stessa** conoscenza parziale | In un 2v2 offline, nascondersi da un avversario onnisciente è teatro. È la voce più costosa dello slice ed è deliberata |
| **D7** | ~~Nessuna finestra~~ → ~~finestra come presentazione~~ → **finestra interattiva vera** (rivista **due volte** il 2026-08-07) | **Superata da D16–D22** in [`brief-overwatch-reazioni.md`](brief-overwatch-reazioni.md). Il documento sorgente sull'Overwatch, emerso dopo, mostra che il bait/bluff non è recuperabile con condizioni dichiarate: se dichiaro «spara al primo che entra», il tank brucia sempre la reaction. La via (b) di `spec-sequenza-turno.md` §3 riconcilia la finestra con l'invariante #3 **per composizione** (sequenza di sotto-risoluzioni). ✅ Formalizzata in [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) |
| **D8** | Il vertical slice è **Flux · Riva · Bastion · Vektor**; i 4 archetipi Paragon del workbook tornano `Candidate` | I cataloghi `.md` sono il canone (regola «`.md` > `.pdf`»); il workbook allinea roster e scale |
| **D9** | I valori numerici partono dalla **scala più bassa**: statistiche dai cataloghi `.md`, parametri nuovi al **minimo di colonna**, uguali per tutti | Si parte piatti e conservativi; la differenziazione la produce il playtest, non una stima a tavolino |
| **D10** | **AP ≡ slot**: `RES_ACTION` (cap 2) copre Movimento + Principale, `RES_REACTION` (cap 1) è lo slot Reazione | Non erano due sistemi in conflitto, erano due nomi dello stesso. Nessuna regola cambia |
| **D11** | «Energia» è una **risorsa firma per personaggio**: nome e trigger di ricarica cambiano, la regola no (cap 4, ricarica 1) | Dà identità senza moltiplicare le meccaniche — coerente col pilastro «scelta orizzontale» |

## 3. Perimetro

**In scope**

- Raggio di vista per eroe (`Vista` del catalogo: Flux **7**, Riva 5, Bastion 5, Vektor 6) che **decide qualcosa**.
- Celle visibili per unità = `HexDistance ≤ Vista` **e** `HasLineOfSight`; conoscenza di squadra = unione.
- Tre stati di contatto: `Nascosto`, `ContattoIncerto`, `Rilevato`, più `UltimoContatto` (persistenza **1 turno**).
- **Canale acustico**: eventi di rumore, propagazione intera sul grafo, soglia d'udito, contatto `Incerto` (§12).
- Il targeting delle azioni offensive richiede che il bersaglio sia noto alla squadra.
- Perdita di contatto durante la pianificazione → fallback **già esistenti** di CP 4.3, nessun enum nuovo.
- Bot sulla stessa conoscenza.
- HUD: marker «ultima posizione», scadenza a fine turno successivo.

**Fuori scope, dichiarato**

Firma visiva · punteggio di rilevamento e soglie · identificazione parziale · sensori e droni · coni
direzionali · tracking per eroe · `RevealRecovery` · densità del fumo · illuminazione · elettricità che
rivela · zona di raggiungibilità prevista nell'HUD · jamming e relay.

## 4. Verifica sui numeri (perché non rompe il bilanciamento)

| Azione | Range | Vista di chi la usa |
|---|---:|---:|
| `Flux.ArcPulse` | 4 | 7 |
| `Vektor.PulseShot` | 4 | 6 |
| `Bastion.ImpactShot` | 3 | 5 |
| `Action.LineAttack` | 5 | ≥ 5 |

Nessuna azione **verificata** supera la vista del proprietario: il vincolo `range ≤ vista` è già soddisfatto,
quindi D5 **non riduce la gittata di nessuno**. Morde solo su bersagli non ancora noti, che è il suo scopo.

Il delta di vista vale **informazione**, non danno: oltre il raggio 5 nessuna azione del roster arriva, quindi
la vista lunga concede *anticipo*, non un colpo gratis. L'argomento non cambia con la distanza — vale a 6 come
a 7, perché dipende dal fatto che **nessuna** azione ci arrivi.

> ⚠️ **Aggiornato il 2026-08-10.** Questo paragrafo diceva «il delta **vista 6 vs 5**» e chiudeva con
> *«questo evita di aggravare la dominanza di Vektor su Flux e Riva già registrata in CP 6.5»*. Entrambe le
> affermazioni sono superate: dopo [D-073](../decisions/RT_PDR_00_Decision_Log.md) Flux ha **vista 7**, quindi
> lo scarto massimo è di **due** punti, e la dominanza non è più «da evitare di aggravare» — **non esiste
> più** (`#131` chiusa: nessun eroe domina nessun altro sulle quattro statistiche base).
>
> Vale però registrare che la vista è diventata **la leva** con cui quella dominanza è stata tolta. Finché
> E13 non esiste la vista non decide nulla, quindi il cambio è stato a costo zero; **quando E13 arriverà,
> questo brief è il posto in cui ricontrollare** che due punti di scarto informativo siano un vantaggio
> proporzionato e non un secondo asse di dominanza per un'altra strada.

> **Assunzione dichiarata**: i range di `PrecisionAttack`, `HeavyAttack`, `CircularAoE`, `SuppressiveLine`,
> `MarkTarget`, `Push`, `Pull` e `Heal` **non** sono esposti nelle tabelle del catalogo azioni e non sono stati
> verificati. Prima dell'implementazione serve un controllo sistematico, o un test che lo renda un invariante.

## 5. Impatti sui documenti esistenti

| Documento | Impatto |
|---|---|
| [`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) §8 | «fog of war» è classificata north-star P1. Lo slice **non** è fog of war (la mappa statica resta nota), ma la distinzione va scritta, altrimenti sembra scope creep |
| [`h6-4-hex-vision-spec.md`](../technical/h6-4-hex-vision-spec.md) §6 | dichiara `VisibleCells` fuori scope. Va aggiornata: lo slice la introduce |
| [`RT_TerrainCatalog_v0.1.md`](../balance/RT_TerrainCatalog_v0.1.md) | il fumo ha già una regola (`Obscured`, cap targeting a **2**). **Prevale il catalogo**: niente densità progressiva |
| [`RT_HeroCatalog_v0.1.md`](../balance/RT_HeroCatalog_v0.1.md) | la frase «Bastion compra HP e resistenza con movimento **e vista**» diventa vera solo con questo slice; oggi è falsa |
| `Core/RTGameplayTags.h` | `Status.Reveal` è già dichiarato e mai implementato: è il gancio naturale per «rivelato per un turno» |
| [`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) | +3 CP → **62** invece di 59. Va dichiarato, non assorbito in silenzio |
| `docs/RefactorTactics_Balance_Matrices_v0.1.xlsx` | ⚠️ **fonte numerica parallela**: contiene già il sistema di visione completo (foglio `04_Visione_Stealth` + `04b_Visione_Parametri`), ma su un **roster e una scala diversi** dai cataloghi in `balance/` — vedi §11 |

## 6. Matrice Visibilità × Terreno × Azione (ridotta allo scope)

Solo le righe che lo slice può davvero verificare. Le altre appartengono a §9.

| Fattore | Effetto sulla conoscenza | Origine del valore |
|---|---|---|
| Distanza > vista | nessun contatto | `Vista` del catalogo eroi (5–6) |
| Cella che blocca la LOS | nessun contatto | `bBlocksLineOfSight` (dato d'asset) |
| Layer diverso | regola D3 di H6.4: blocca solo sul layer dell'osservatore | eredità della LOS esistente |
| Fumo (`Terrain.Smoke`) | contatto solo entro **2** celle, dentro o attraverso | catalogo terreni, invariato |
| Copertura alta | blocca la vista (blocca già LOS e proiettili) | catalogo terreni §3 |
| Copertura bassa | **non** blocca la vista (protegge, non nasconde) | distinzione copertura ≠ occultamento |
| Quota elevata | «bonus visuale» dichiarato ma **non quantificato** nel catalogo | ⚠️ numero mancante, §10 |
| Attacco eseguito | l'attaccante diventa noto per il turno (`Status.Reveal`) | §4 del design di partenza |
| Perdita di contatto in planning | fallback di CP 4.3, nessun enum nuovo | `Fallback.{Cancel, AttackCell, Stop}` |

## 7. Checkpoint proposti — epic **E13 · Conoscenza parziale** (P2)

Epic separata invece che innesto in E6/E11: resta tracciabile e **tagliabile** se il tempo stringe.
Dipende da **E4** (fallback), **E6** (statistiche eroe), **E8** (fumo). Si colloca accanto a E9/E11.

| CP | Obiettivo | DoD misurabile | Test |
|---|---|---|---|
| **13.1** | `VisibleCells` e conoscenza di squadra | Funzione **pura** e headless: celle visibili per unità (`HexDistance ≤ Vista` ∧ LOS), unione per squadra, ordine stabile; **nessun consumatore ancora** | `Vision.VisibleCellsRespectsSight`, `Vision.TeamKnowledgeIsUnion`, `Vision.SmokeCapsContactAtTwo`, `Vision.PermutationInvariant` |
| **13.2** | Il targeting consuma la conoscenza + memoria del contatto | Le azioni offensive rifiutano bersagli ignoti alla squadra; `FRTLastKnownContact` per squadra nello snapshot, **versione di formato incrementata** e migrazione; persistenza 1 turno | `Vision.CannotTargetUnknown`, `Vision.AllySpottingExtendsTargeting`, `Vision.LastContactExpiresAfterOneTurn`, `Vision.SerializationRoundTrip` |
| **13.3** | Bot e HUD sulla stessa conoscenza | `URTHexBotLibrary` pianifica sulla conoscenza della **propria** squadra, nessuna mossa che presupponga informazioni non note; HUD mostra il marker d'ultimo contatto | `Bot.PlansOnPartialKnowledge`, `Bot.DoesNotTargetUnknown`; verifica PIE `PIE-V01-VISION` |

**Opzionale, primo a cadere**: trigger `OnFirstContact` sullo slot Reazione (D7). Richiede E5 chiusa, ed E5 è
già il punto di rischio dell'ADR-0003. Se E5 degrada alle sole difensive di Prep, questo cade con lei.

## 8. Rischi

| Rischio | P/I | Mitigazione |
|---|---|---|
| I test del bot (smoke/panic/support/tuning) cambiano **premessa**, non solo valori | **H/M** | CP 13.3 li riscrive esplicitamente; un bot che perde il contatto e sbaglia è il comportamento *atteso*, non una regressione |
| «Vista derivata» ma l'ultimo contatto è stato: il confine si sfuma con l'uso | M/H | D3 lo fissa in un solo campo versionato; tutto il resto resta funzione pura, verificato da `Vision.PermutationInvariant` |
| Il giocatore non capisce *perché* non può bersagliare | M/M | Il combat log deve dare il reason code (CP 11.3 lo prevede già con `ValidationResult`) |
| Lo slice cresce verso la §9 durante l'implementazione | M/H | §3 elenca il fuori scope per nome; ogni aggiunta è una modifica a questo brief, non una decisione in corsa |
| Il bonus visuale della quota non ha un numero | M/M | §10: va deciso o dichiarato assente prima di CP 13.1 |

## 9. Espansioni rimandate (il design completo)

Il documento d'origine descrive un sistema molto più ampio, che resta valido come direzione: cinque stati di
conoscenza · firma visiva come somma di modificatori · punteggio di rilevamento con soglie · identificazione
separata dal rilevamento · quattro archetipi percettivi (Ricognitore, Guardiano, Controllore, Infiltratore) ·
densità del fumo · acqua/fuoco/elettricità come rivelatori · `ELossOfContactPolicy` per abilità · zona di
raggiungibilità prevista · sensori, droni, coni direzionali · relevancy per connessione e rappresentazioni
sanitizzate lato rete.

Due punti restano **obbligatori** quando arriva M10 (rete e privacy), non prima:

- un'unità non nota **non deve essere replicata** e poi nascosta lato client (invariante #6);
- il replay del giocatore va filtrato durante il match.

Lo slice li rispetta per costruzione, essendo offline e senza replica.

## 10. Domande aperte

1. **Bonus visuale della quota** — ⚠️ **riaperta e chiusa nel verso opposto da
   [D-018](../decisions/RT_PDR_00_Decision_Log.md)** (2026-08-08): nella v0.1 la quota **non** dà alcun bonus
   numerico a `VisionRange`.
   > La chiusura del 2026-08-07 assegnava `Sight_Mod = +1` alla quota alta, `+2` al tetto, `−1` a tunnel e
   > acqua profonda, con la regola `Sight_Mod = Height`. Il numero veniva dal **workbook**, non da un playtest,
   > ed è caduto per la stessa ragione di D-002 e D-001: un valore scelto a tavolino che nessuno ha misurato.
   >
   > La quota **conta già** — attraverso geometria, LOS, occlusione, copertura e topologia dei layer. Sommarci
   > un bonus numerico gratuito raddoppierebbe lo stesso vantaggio in due modi, e renderebbe la posizione alta
   > l'unica giocabile. Un bonus futuro richiede playtest e una decisione nuova, non il ripristino di questa.
2. **Auto-visibilità**: un'unità vede sempre sé stessa e i propri alleati, ovunque siano? (Assunto sì, ma non è
   ovvio con la mappa multilivello: un alleato nel tunnel resta noto?)
3. ~~**Contatto mutuo**: se A vede B, B vede A?~~ ✅ **Chiusa il 2026-08-09** — ed era **mal posta**.
   Presupponeva una relazione fra **unità**, mentre il DoD dice `TeamKnowledge`: conoscenza **di squadra**.
   Esistono due conoscenze separate, una per squadra, ognuna unione dei coni dei propri membri:
   l'asimmetria non va decisa, **c'è già per costruzione**, e con la vista a cono (CP 16.1) è la norma
   invece del caso di bordo. La domanda vera che vi si nascondeva dentro era di *presentazione*, ed è
   stata decisa: il giocatore **non** sa di essere nell'arco frontale di un avversario
   ([D-043](../decisions/RT_PDR_00_Decision_Log.md)) — sapere di essere osservati è informazione
   sull'avversario (invariante #6), e aggirare deve pagare due volte.
4. **Range delle azioni non verificate** (§4): controllo sistematico, o test che rende `range ≤ vista` un
   invariante del catalogo?

## 11. ⚠️ Conflitto di fonti: workbook vs cataloghi `balance/`

Registrato il **2026-08-07** analizzando `docs/RefactorTactics_Balance_Matrices_v0.1.xlsx` (25 fogli).
Il workbook contiene già l'intero sistema di visione — inclusa la parte che §9 dichiara rimandata — ma
**diverge dai cataloghi versionati in `balance/` su quasi ogni numero**. Non è una differenza di dettaglio:

| Dimensione | Cataloghi `balance/` (repo) | Workbook `.xlsx` |
|---|---|---|
| Roster vertical slice | Flux · Riva · Bastion · Vektor | **Aurora · Kwang · Murdock · Steel** (su 38 eroi Paragon) |
| Salute | 90 – 120 | **780 – 1150** |
| Raggio visivo | 5 – 6 | **6 – 11** |
| Movimento | 5 MP, costi 1/2 | **6 hex** + Sprint bonus + Dash 5 |
| Danno abilità | 16 – 35 | **55 – 180** |
| Risorse | energia / ultimate | **Action Points (cap 2)** + risorse firma |
| Fasi | Prep → Dash → Blast → Move | `Planning` / `Resolution` con **priorità 1–75** |
| Reazioni | dichiarate in planning, deterministiche | **finestre interattive di 5–7 secondi** |

Le due scale **non sono convertibili l'una nell'altra** con un fattore: sono due giochi diversi sulla stessa
idea. In particolare le finestre di reazione a tempo del foglio `07_Fast_Reactions` (`Finestra_sec`) e
`ACT_FASTSELECT` (fase `Resolution`) confliggono frontalmente con l'invariante #3 e con la decisione **D7**.

> ✅ **Risolto il 2026-08-07** con la **via 2**: i cataloghi `.md` restano il canone e il workbook è stato
> riallineato (roster, scale, risorse, reazioni) — decisioni **D8–D11**. La tabella sotto resta come
> registrazione di *cosa* divergeva; la colonna `Valore precedente` del foglio `22_Changelog` conserva i valori
> sostituiti. Le tre vie sono conservate perché la scelta va rispiegata se qualcuno riapre il tema.

**Le tre vie considerate:**

1. **Il workbook diventa la fonte numerica** e i cataloghi `balance/` vengono rigenerati da lì. Richiede un
   ADR: cambia roster, scala dei danni, sistema di risorse e modello di reazione, e invalida parte di E5/E6.
2. **I cataloghi restano il canone** e il workbook resta uno strumento di esplorazione del bilanciamento, da
   riallineare (roster e scale) prima di usarne i numeri.
3. **Convivenza dichiarata**: il workbook resta autorevole per la *struttura* (quali colonne, quali relazioni)
   e i cataloghi per i *valori* della v0.1. È la modifica minima, ed è ciò che la colonna `Scope_v0.1`
   aggiunta ai fogli rende già leggibile.

**Dopo il consolidamento**: workbook e cataloghi concordano su roster, statistiche, risorse e modello di
reazione. Restano nel workbook — e **solo** lì — i parametri che i `.md` non hanno ancora: percezione
dettagliata (Detection, Identification, Stealth, firme), modificatori di visione per terreno/cover/hazard/status
e i 22 parametri globali di `04b_Visione_Parametri`. Sono tutti marcati `Scope_v0.1 = Espansione` tranne quelli
elencati in §3, e nessuno di essi è ancora un valore da portare in codice: il gate resta CP 13.1.

~~⚠️ Il workbook **non è ancora versionato in git** (`docs/*.xlsx` risulta untracked). Finché non lo è, la sua
autorità è di fatto quella di un file di lavoro locale.~~

> ✅ **Superata il 2026-08-07**: il workbook **è versionato** e vive in
> [`../balance/RefactorTactics_Balance_Matrices_v0.1.xlsx`](../balance/RefactorTactics_Balance_Matrices_v0.1.xlsx),
> accanto ai cataloghi `.md` di cui è lo strumento di esplorazione. La gerarchia non cambia: i `.md` restano
> il canone (via **2**, D8–D11).

## 12. Il canale acustico — rumore

Fonte: `docs/archive/src/design/rumore-e-percezione-acustica.md` (33 sezioni). Brainstorming del **2026-08-07**.

**Perché costa poco.** Il rumore è il gemello acustico della visione e riusa *tutto*: `TeamKnowledge`, la
privacy dell'invariante #6, i tre livelli UI confermato/previsto/incerto, la memoria dell'ultimo contatto
(`LastHeard` ≡ `LastKnownContact`), il TurnLog e il filtro che lo precede all'uscita. Non è un secondo
sistema: è un **secondo canale che alimenta lo stesso modello**.

> ⚠️ **Corretto il 2026-08-09** ([#295](https://github.com/DegrassiAaron/refactor-tactics-main/issues/295)):
> questa riga diceva «il TurnLog **sanitizzato per squadra**». Il TurnLog è **uno solo**
> (`TArray<FRTTurnLogEntry> TurnLog` in `ARTTurnManager`) ed è la sorgente di
> `URTTurnLogLibrary::HashTurnLog`: filtrarlo avrebbe reso il checksum del replay dipendente da **chi
> guarda**, cioè non più un checksum. Ciò che si sanitizza è la **vista** che raggiunge un osservatore, con
> la stessa disciplina degli intenti — `FRTPlannedIntent → FilterForTeam → FRTIntentView`, dove il DTO non
> riceve i campi proibiti invece di nasconderli dopo averli spediti.

**I dati esistono già.** Il workbook ha `Noise_Mod` su tutti i 17 terreni e `EQ_BREACH_CHARGE` dichiara
«Rumore 10». Regola di derivazione proposta, coerente con quelle della visione:

```
NoiseDelta = Noise_Mod − 1        (terreno libero = 0, come il «cemento» del documento sorgente)
```

Verifica incrociata: ghiaccio `Noise_Mod 2` → **+1**, che è esattamente il valore del documento (§8). Acqua
bassa esce **+3** contro il +2 del documento, e la vegetazione **+1** contro il −2 (il documento la vuole
silenziosa, il workbook rumorosa): due divergenze da chiudere prima di CP 13.3.

> ✅ **Chiuse il 2026-08-09.** L'acqua bassa vale **`+2`** — vince il documento sorgente, il workbook resta
> `RESEARCH` ([D-042](../decisions/RT_PDR_00_Decision_Log.md)): sprintarci fa 7 su 10, come un Dash su
> terreno libero, e l'acqua costa già 2 MP, bagna e conduce. La divergenza sulla **vegetazione non è stata
> decisa e non blocca nulla**: quel terreno non esiste nella v0.1 — le superfici sono otto
> (`Floor · ShallowWater · Rough · Fire · Conductive · Ice · Void · Smoke`) e non lo comprendono.
> Si deciderà quando la superficie esisterà.

> ✅ **Le due superfici senza riga, e un terzo valore, decisi il 2026-08-11**
> ([D-091](../decisions/RT_PDR_00_Decision_Log.md)), implementando CP 13.3. `Rough` **+1** — accidentato, come
> il ghiaccio; `Conductive` **0** — ha già un owner ed è elettrico, il rumore non è il suo mestiere.
> Il terzo non era in nessun elenco: l'**attenuazione per arco** (`2`), che la DoD di CP 13.3 nomina nella
> formula senza che alcun documento normativo la quantificasse. Il numero viene dal documento sorgente, come
> il `+1` del ghiaccio.
>
> ⚠️ **Il verso, che questa sezione non diceva**: `NoiseDelta` **amplifica alla sorgente**, non attenua in
> transito. Lo dice D-042 alla lettera — *«l'acqua bassa **aggiunge** +2 al rumore»* — e la scala lo conferma,
> perché il fuoco (`+4`) crepita e non ovatta. Il documento sorgente ha anche un `CellSoundAttenuation` in
> transito, ma senza valori: resta fuori.

> ✅ **Soglia d'udito, decisa lo stesso giorno** ([D-041](../decisions/RT_PDR_00_Decision_Log.md)):
> è una statistica **per eroe** che **compensa** la vista invece di seguirla — Flux 5 · Riva 3 ·
> Bastion 3 · Vektor 5, soglia bassa = orecchio fine. Chi vede lontano sente meno. Va **aggiunta** a
> `URTHeroData`, che oggi ha quattro statistiche e nessuna è l'udito.

### Decisioni

| # | Decisione | Motivo |
|---|---|---|
| **D12** | Il rumore entra nella **v0.1**, dentro E13, non come epic separata | Condivide modello, privacy e UI con la vista: separarli produrrebbe due `TeamKnowledge` |
| **D13** | La conoscenza passa a **tre livelli**; il rumore produce `ContattoIncerto`, **mai** una cella esatta | I cinque livelli di precisione del §13 sorgente sono il cuore del sistema. Un rumore che localizza con precisione è solo un secondo raggio di rilevamento |
| **D14** | Propagazione = **flood fill intero** limitato dall'intensità sul grafo tattico, **non** `SphereOverlap` | §7 e §31 del sorgente; è la stessa query che serviranno propagazione elettrica (CP 8.3) e calore. Si costruisce **una volta** |
| ~~**D15**~~ | ~~La Fast Reaction acustica **non apre finestre**: è un trigger dichiarato in planning~~ · ⚠️ **superata da [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md)** (2026-08-07): poggiava su **D7**, che vietava le finestre — e D7 è caduto. **Regola vigente**: un evento acustico **può** generare una `ReactionOpportunity`, ma **solo se una reaction o un profilo lo dichiara**; se poi si apra davvero una finestra lo decide il regime (`AllowedResponses` + eventuale condizione), non il canale. **Nessuna Fast Reaction acustica di default**: la v0.1 non ne definisce nessuna | Il divieto assoluto non ha più una premessa. Il suo posto lo prende un permesso **condizionato**, che è cosa diversa da un obbligo |

### Perimetro

> **Revisione del 2026-08-12** ([D-112](../decisions/RT_PDR_00_Decision_Log.md)): il contatto acustico passa da **una** ampiezza a **due** (raggio `4` e `2`), più la direzione dovuta all'attacco, che resta una regola a parte. ⚠️ A scegliere l'ampiezza è il **margine sopra soglia**, mai il tipo di evento: riconoscere il tipo di un rumore è il Livello 5 del §13 sorgente, che richiede un personaggio specializzato. Il «Fuori» resta invariato: i **cinque** livelli del §13 sorgente, l'identificazione della sorgente e le abilità sonore restano fuori dalla v0.1. La modifica è registrata qui perché questo brief è **vincolante** — lo diventa *«quando i CP di §7 entrano in `roadmap-v0.1.md`»*, e CP 13.1–13.5 ci sono già: allargare il perimetro senza toccarlo avrebbe lasciato il documento a dire una cosa e il codice a farne un'altra.

**Dentro**: evento sonoro (`SourceId`, `OriginCell`, `NoiseType`, `Intensity`, `TurnIndex`, `MicroStepIndex`) ·
intensità per azione (Wait 0 → Sprint 5 → Dash 6 → esplosione 10) · attenuazione per superficie e per arco ·
soglia d'udito per eroe · contatto incerto con area a **due ampiezze** — larga `4`, stretta `2`, scelte dal **margine sopra soglia** ([D-112](../decisions/RT_PDR_00_Decision_Log.md)) · l'attacco rivela almeno la direzione · TurnLog filtrato.

**Fuori** (§9): decoy e passi fantasma · mascheramento acustico · rumore ambientale persistente · eco dei
tunnel · cinque livelli di precisione · identificazione della sorgente · abilità sonore (Sonar Pulse, Resonance
Shot, Sonic Grenade) · attributi acustici completi (si usa il solo `HearingThreshold`).

### Il punto architetturale che vale più del rumore

Visione, rumore, propagazione elettrica (CP 8.3) e le interazioni termiche del ghiaccio chiedono **la stessa
cosa**: una query di propagazione sul grafo tattico, distinta dal pathfinding. Il documento sorgente lo dice
esplicitamente (§28):

```
Grafo tattico ──┬── Movimento     (esiste: RTHexPathLibrary)
                ├── LOS           (esiste: RTHexVisionLibrary)
                ├── Targeting     (esiste)
                └── Propagazione  (manca — serve a rumore, elettricità, calore)
```

Costruirla una volta è la differenza fra un'epic e tre. **CP 13.3 è il posto giusto** per farla nascere
generica, con il rumore come primo cliente e la propagazione elettrica come secondo.
