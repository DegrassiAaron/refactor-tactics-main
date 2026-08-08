# RefactorTactics — Piano Canonico

> **Stato**: canone di progetto · **Ultimo aggiornamento**: 2026-08-05
> **Scopo**: raccogliere le decisioni operative vincolanti del gioco — invarianti, architettura, regole
> numeriche — in un'unica specifica coerente. Questo documento è la **fonte di verità** per
> l'implementazione. In caso di conflitto con i PDF in `docs/src/`, prevale questo file.
>
> ⚠️ **Fase tutorial chiusa (2026-08-05)**: nato come riconciliazione dei due corsi (`02-Tutorial`,
> `03-TutorialToMVP`) per costruire l'MVP, questo piano è ora il canone di un progetto di **prodotto**.
> I corsi sono storico; l'MVP quadrato M0–M5 è archiviato. Roadmap corrente:
> [`roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md).

---

## 1. Gerarchia delle fonti

| Livello | Documenti | Ruolo |
|---|---|---|
| **Canonico (vincolante)** | *questo file* | Decisioni operative del progetto |
| **Esecuzione** | [`roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md) | Milestone, checkpoint, DoD misurabili, stato |
| **Requisiti di lungo periodo** | [`../PDR/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md`](../roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md) | Fasi F0–F6, QA, rischi — direzione, non scope |
| **Visione (north-star)** | i 3 PRD + `Intenti condivisi.pdf` | Prodotto a lungo termine, **non** scope attuale |
| **Storico / superato** | `00-Intro.pdf`, `01-StrutturaTutorial.pdf`, `02-Tutorial.pdf`, `03-TutorialToMVP.pdf` — **rimossi da `docs/src/` il 2026-08-07**, recuperabili dallo storico git | Brief e corsi da cui è nato il progetto (fase chiusa) |

I 4 PRD descrivono un prodotto molto più ambizioso (4v4 competitivo, GAS, netcode avanzato,
modding). Sono la **direzione futura**, non l'obiettivo attuale. Vedi §8.

---

## 2. Identità del progetto

- **Nome**: **RefactorTactics** (coincide col repository; i tutorial usano
  `ReactorTactics`/`Reactor Tactics` — allineati a questo nome).
- **.uproject**: `RefactorTactics.uproject` nella **radice del repository**
  (`D:\Repositories\refactor-tactics-main`), così il progetto UE è versionato col resto.
- **Natura**: gioco tattico PvP a **turni simultanei** ispirato ad *Atlas Reactor*, sviluppato su
  Unreal Engine 5.8 da un dev singolo. *(Fino al 2026-08-05 il progetto era anche un percorso didattico
  UE partendo da C#: quella fase è chiusa, vedi `roadmap-checkpoint.md`.)*
- **Nota IP**: il riferimento ad Atlas Reactor è solo meccanico. Per un'eventuale pubblicazione
  servono nomi, personaggi, lore e asset **originali** (decisione aperta, vedi §9).

---

## 3. Decisioni canoniche — riconciliazione `02` ↔ `03`

> ⚠️ **Revisione 2026-08-03 — PIVOT A GRIGLIA ESAGONALE** ([`adr-0002-griglia-esagonale.md`](../decisions/adr-0002-griglia-esagonale.md)):
> per decisione dell'utente il progetto abbandona la griglia **quadrata** (righe "Griglia" e "Coord. cella" della
> tabella sotto) a favore di una griglia **esagonale** (assiale/cubica, `FRTCellId`) con editor mappa data-driven.
> Quelle righe sono **superate**; il rifacimento procede per milestone H0–H9 ([`hex-map-roadmap.md`](../roadmap/hex-map-roadmap.md))
> e invalida M1–M5 (griglia/combat/LOS/bot/pathfinding). Gli **invarianti** (determinismo, no float, dati autorevoli in
> C++) restano. Il sistema quadrato resta su `feat/skeletal-units`/`main` come base di rollback.

> ⚠️ **Revisione 2026-08-05 — MODELLO AZIONI v0.1** ([`adr-0003-modello-azioni-v01.md`](../decisions/adr-0003-modello-azioni-v01.md)):
> per la release **v0.1** ([`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md)) si adotta il modello del catalogo di bilanciamento —
> ID azione stabili, **priorità intera** intra-fase, fallback dichiarati, budget **5 MP**, reazioni, 8 terreni,
> coperture direzionali, obiettivi dinamici, limite di round *(oggi parametro di formato, non più «12» fisso:
> [`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md) §6)*. **Le macro-fasi NON cambiano**: resta
> `ERTMatchPhase{ Planning, Prep, Dash, Blast, Move, Cleanup, MatchEnded }` di Atlas Reactor — in particolare il
> **Move resta dopo il Blast**. I codici di fase del catalogo (0/10/20/30/40/50/60) diventano un attributo
> dell'azione, rimappato sulle macro-fasi (ADR-0003 §3). Sono **superati**: i valori di movimento di §6 e la
> vittoria per sola eliminazione. L'allineamento completo di questo documento è il checkpoint **CP 1.1**
> della v0.1 (issue `#27`).

| Tema | `02-Tutorial` | `03-TutorialToMVP` | **Canonico** | Motivazione |
|---|---|---|---|---|
| Nome / `.uproject` | ReactorTactics | Reactor Tactics | **RefactorTactics** | Deciso; = repo |
| Prefissi classi | `AT` / `UAT` | `RT` / `URT` | **`RT` / `URT`** | Coerente col nome; `03` è la spina dorsale |
| Scope MVP | Multiplayer 1v1 | Offline 2v2 vs bot | **Offline 2v2 vs bot** | Minor rischio per dev singolo; multiplayer rimandato |
| Split linguaggi | ~60% C++ | ~70% Blueprint | **Regole in C++, presentazione in Blueprint** (nessuna % fissa) | Principio condiviso; la % è conseguenza |
| Griglia | 12×12 @ 100 cm | 10×10 @ 200 cm | **10×10 @ 200 cm** | Base `03`; celle da 200 cm adatte alla scala UE. Tunable |
| Coord. cella | `FGridCoord{X,Y}` | `FIntPoint` | **`FRTGridCoord{X,Y}`** (`Layer` riservato) | USTRUCT estendibile verso mappa multilivello (visione PRD) |
| Enum fasi | `EATMatchPhase` + `EATActionPhase` | `ERTCombatPhase` | **`ERTMatchPhase{ Planning, Prep, Dash, Blast, Move, Cleanup, MatchEnded }`** | Un solo enum; fasi Prep/Dash/Blast/Move condivise |
| Resolver | `UATTurnResolver : UObject` | `ARTTurnManager : Actor` | **`URTTurnResolver : UObject`** (logica pura) orchestrato da **`ARTTurnManager`** | Testabile, senza dipendenze da Actor |
| Combat math | `FATCombatMath` (struct) | funzione libera `CalculateDamage` | **`URTCombatLibrary`** (BlueprintFunctionLibrary, funzioni pure) | Testabile e richiamabile da Blueprint |
| Griglia visuale | `BP_CellHighlight` | Instanced Static Mesh | **Instanced Static Mesh** | Più efficiente |
| Timer pianificazione | 30 s | 15–30 s | **30 s (configurabile)** | Entrambi ~30 s |
| Versione UE | 5.8.1 | 5.8.x | **5.8.1** | Deciso; bloccata per l'intero corso |
| Ability system | `UPrimaryDataAsset`, no GAS | `UPrimaryDataAsset`, no GAS | **`URTAbilityData : UPrimaryDataAsset` (no GAS)** | Concordi; GAS post-MVP |
| Energia / Ultimate / Status / Tag | assenti | presenti | **Presenti** | Parte dello slice `03` |
| Targeting a forme | assente | Self/Single/Line/Cone/Circle | **Presente** | Feature dello slice |
| Bot AI | assente | utility scoring | **Presente** (utility scoring) | Necessario per 2v2 offline |
| Vittoria | 3 elim. o miglior punteggio/10 turni | squadra eliminata | **Squadra eliminata** (niente punteggio nell'MVP) | Il "punteggio" non è mai definito in `02` → escluso |
| Test | Automation Framework | Automation Framework + CI | **Automation Framework** (CI = milestone successiva) | CI self-hosted è avanzata |
| Networking | core (lez. 9) | roadmap P0 post-tutorial | **Rimandato post-MVP**, ma architettura *server-authority-ready* | Si mantiene la disciplina di `02` come target |
| Percorso progetto | `C:\Dev\...` | `D:\Dev\...` | **Radice del repo** | Il progetto UE vive nel repo versionato |

> ⚠️ **Revisione 2026-08-08 — SECONDO PASSAGGIO DOCUMENTALE.** Il canone recepisce qui, come richiede la regola
> di gerarchia («un ADR accettato si recepisce nel canone nello stesso commit»), sei punti che vivevano solo
> negli ADR o nel Decision Log:
>
> | Punto | Cosa vale ora | Fonte |
> |---|---|---|
> | **Facing** | È **stato di gioco**, non presentazione, e ha tre consumatori: difesa, percezione, reazioni direzionali. Un'azione con bersaglio **orienta l'unità prima di risolvere**; il `Move`, ultimo, fissa il facing finale, che persiste nel round successivo | [ADR-0005](../decisions/adr-0005-orientamento.md) · [D-020](../decisions/RT_PDR_00_Decision_Log.md) |
> | **Conoscenza parziale** | Non è «fog of war»: la mappa statica è nota. È incompleta l'informazione sulle **unità e sugli eventi** — LOS geometrica + rilevamento + rumore + ultimo contatto ⇒ **Team Knowledge** a tre livelli. Il rumore è un **secondo canale**, propagato con interi sul grafo | `gameplay/brief-conoscenza-parziale.md` · E13 |
> | **Overwatch** | **Universale**: azione di pianificazione per tutti, che **compete con l'azione offensiva principale** (`Attack` **oppure** `Ability` **oppure** `Overwatch`). Il profilo cambia per eroe/equipaggiamento; l'azione no | [D-012](../decisions/RT_PDR_00_Decision_Log.md) · [D-014](../decisions/RT_PDR_00_Decision_Log.md) |
> | **Quota / High Ground** | Vale per **geometria** — LOS, occlusione, copertura, accessibilità. **Nessun `+Damage` e nessun `+VisionRange` globali.** Un eroe, tratto, abilità o equipaggiamento può dichiarare un bonus da altura, in modo data-driven | [D-018](../decisions/RT_PDR_00_Decision_Log.md) · [D-024](../decisions/RT_PDR_00_Decision_Log.md) |
> | **Formato di partita** | **Non è deciso.** 2v2 è la vertical slice corrente, 3v3 la baseline di lavoro, 4v4 solo scenario di stress (E17). Nessun documento tratti il 3v3 come formato di prodotto scelto: si consolida con la **prima misura** reale | [D-011](../decisions/RT_PDR_00_Decision_Log.md) |
> | **Verifica automatica** | Gli scenari di test passano dalla **stessa pipeline di gioco** della partita reale: JSON versionato sotto `Scenarios/` → percorso di gioco → `result.json`, `PASS`/`FAIL`/`ERROR`. **Nessun bypass** del resolver, nessun Actor di test | `technical/test-automatico-unreal.md` |
>
> Restano fermi UE **5.8.1** ([D-022](../decisions/RT_PDR_00_Decision_Log.md), ora *Consolidata*) e il **no-GAS**
> per la v0.1.

### 3.0 Stato vigente delle decisioni superate (2026-08-06)

La tabella sopra è la **riconciliazione dei due corsi** e resta leggibile come storia del progetto: alcune sue
righe sono state superate da decisioni successive. Questa tabella dice, riga per riga, **cosa vale oggi** — così
non serve ricostruirlo dai blocchi di revisione.

| Riga superata (§3) | Valore vigente | Decisione |
|---|---|---|
| Griglia `10×10 @ 200 cm` | Griglia **esagonale** pointy-top, dimensione della cella dall'asset mappa | [ADR-0002](../decisions/adr-0002-griglia-esagonale.md) |
| Coord. cella `FRTGridCoord{X,Y}` | **`FRTCellId{X=q, Y=r, Layer}`** (assiale/cubica); `FRTGridCoord` **rimosso** dal codice (CP 6.1) | [ADR-0002](../decisions/adr-0002-griglia-esagonale.md) |
| Vittoria `Squadra eliminata` | Eliminazione **oppure** obiettivi **oppure** `RoundLimit` raggiunto (fine partita a più vie). Il `RoundLimit` è un **parametro del formato di partita**, non la costante «12»: 10–14 in 2v2, 16–20 in 3v3 Standard | [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) · [`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md) §6, §12 |
| Movimento `4 celle` / Dash `3` (§6) | **5 MP**, costo intero per cella (1 normale, 2 difficile/rampa); Sprint 8 MP; Dash/Charge/Leap a distanza fissa | [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) |
| Reazioni «north-star, escluse» (§8.2) | **In scope** per la v0.1: slot Reazione con trigger, **1 attivazione per turno** | [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) |
| Roster `2 archetipi` | **4 eroi** (Flux, Riva, Bastion, Vektor) con varianti d'equipaggiamento | [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) |

**Non** superato, e vale la pena ripeterlo perché il catalogo v0.1 dice altro: le **macro-fasi restano quelle di
Atlas Reactor** — `Planning → Prep → Dash → Blast → Move → Cleanup`, con il **Move dopo il Blast**. I codici di
fase del catalogo (0/10/20/30/40/50/60) sono un *attributo dell'azione*, rimappato sulle macro-fasi
(ADR-0003 §3). Restano fermi anche UE **5.8.1** e il **no-GAS**.

I valori numerici vigenti della v0.1 (azioni, terreni, equipaggiamento, eroi) vivono nei cataloghi versionati
in [`balance/`](../balance), non in questo documento: qui restano le **decisioni**, lì i **numeri**.

### 3.1 Riconciliazione fonti PDF — path finding (2026-08-02)

I 4 PDF (visione north-star) divergono su elementi *load-bearing* del path finding. Decisioni canoniche
(prevalgono sui PDF); dettaglio e gate di implementazione in
[`spec-pathfinding-pf3-pf4.md`](../technical/spec-pathfinding-pf3-pf4.md).

- **Fasi del turno** — prevale lo schema già canonico (§3, `ERTMatchPhase`):
  `Planning → Prep → Dash → Blast → Move → Cleanup`. Gli schemi del "piano completo"
  (`Preparation→Movement→Actions→…`) e del PRD (`…Mobilità rapida→Movimento→…`) sono elaborazioni
  **mappabili** su questo, non sostituzioni. Il path finding serve a **Move** (movimento normale) e
  **Dash** (mobilità rapida, `TraversalProfile` distinto).
- **Coordinata** — `FRTGridCoord{X, Y, Layer}` (§3): il campo verticale si chiama **`Layer`** (default 0),
  **non `Level`** (divergenza PRD scartata). Il 2D corrisponde a `Layer = 0`; `GetTypeHash` includerà `Layer`.
- **Costo di traversata** — modello **additivo intero**: `TraversalCost = Σ costi interi dei provider`
  (piano completo). **Niente float nel resolver/hash** (invariante determinismo #4); i `MovementMultiplier`
  float dei Data Asset si convertono a intero al caricamento. (Scartato il modello moltiplicativo-float del PRD.)

---

## 4. Definizione dell'MVP (vertical slice) — *consegnato, storico*

> ✅ **Chiusa il 2026-08-03** con M5. La checklist resta come **definizione di «partita completa»**: è il
> comportamento di riferimento che **M6** deve riprodurre su griglia esagonale (parità funzionale). Non è più
> una lista di lavoro da spuntare.

Checklist di completamento — l'MVP era "fatto" quando **tutte** queste voci erano vere.
**Revisione al 2026-08-02** (✅ fatto · 🟡 fatto in parte); le voci 🟡 sono state chiuse dagli incrementi
post-MVP (utility scoring, Dash attiva, forme Line/Cone, Reveal, build Shipping) — vedi roadmap § *Archivio*:

- [x] ✅ Il gioco si avvia direttamente nell'arena *(GlobalDefaultGameMode + `L_Prototype`)*
- [x] ✅ **2v2**: 2 unità alleate (giocatore) + 2 nemiche (bot)
- [x] ✅ Selezione delle proprie unità
- [x] ✅ Pianificazione: **abilità (tasti 1/2/3) + bersaglio (click) + movimento (click cella)**
- [x] ✅ Lock-in del turno con **timer 30 s** (Spazio o scadenza)
- [~] 🟡 Il **bot** pianifica le unità nemiche — sceglie nemico più vicino + abilità migliore per danno; *utility scoring multi-fattore (minaccia, kiting) da fare*
- [~] 🟡 Risoluzione a 4 fasi **Prep → Dash → Blast → Move**, deterministica — la macchina attraversa tutte le fasi e Blast/Move risolvono in modo deterministico ("raccogli poi applica"); *Prep/Dash sono pass-through (nessuna abilità le usa ancora)*
- [~] 🟡 Combattimento: danni ✅, **scudi** ✅, **energia/ultimate** ✅, **status** Root/Slow ✅ *(Shield/Reveal ⏳)*, **LOS/copertura** ✅
- [~] 🟡 **Targeting a forme**: Single ✅, area/Circle ✅ *(Self/Line/Cone ⏳)*
- [x] ✅ Vittoria: la partita termina quando una squadra è eliminata (+ riavvio con R)
- [x] ✅ UI: salute, energia, abilità, **timer**, **fase**, **combat log** (tutti a schermo)
- [x] ✅ Test automatici: griglia, danno-dopo-scudo, ordine fasi, conflitti di movimento *(50 test)*
- [~] 🟡 Build Windows: **Development** ✅ (verificata, si avvia senza editor) *(Shipping ⏳)*

**Verdetto**: MVP **sostanzialmente completo e giocabile end-to-end**. I punti 🟡 sono soddisfatti nel
loro nucleo; restano rifiniture/estensioni dichiarate (utility scoring del bot, abilità di fase Prep/Dash,
forme Line/Cone, status Shield/Reveal, build Shipping), non bloccanti per il vertical slice.

---

## 5. Architettura & invarianti

Principi non negoziabili (valgono anche in offline, per preparare il multiplayer futuro) — numerazione allineata a `CLAUDE.md` § *Invarianti architetturali* (7 voci):

1. **Autorità delle regole in C++**: il resolver decide l'esito; le animazioni/VFX non decidono nulla.
2. **Griglia logica autoritativa**: la posizione vera è **`FRTCellId{X=q, Y=r, Layer}`** (assiale/cubica); il `FVector` serve solo al rendering. *(Corretto il 2026-08-07: questa riga citava `FRTGridCoord`, **rimosso dal codice al CP 6.1** — vedi §3.0.)*
3. **Resolver "raccogli poi applica"**: snapshot a inizio **segmento di risoluzione**, nessun `Delay`/timeline/montage dentro il segmento; l'ordine dell'array **non** deve cambiare il risultato. Quando l'esito dipende dall'ordine (scudo/buff/reazione prima del danno), l'ordine segue la regola deterministica di **§5.1** (APNAP + tie-break assoluto), non l'inserimento.
   > **Riformulato il 2026-08-07 da [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md)** — *indebolito no, composto sì*. Un **segmento** è delimitato dall'inizio di una macro-fase **oppure** da un *decision boundary* (finestra di reazione). Il turno è una **sequenza** di segmenti, ciascuno dei quali è un «raccogli poi applica» completo con snapshot proprio. Il resolver non attende **mai** dentro un segmento: lo termina e restituisce il controllo. Prima di ADR-0004 il segmento coincideva sempre con la macro-fase, quindi la formulazione precedente resta valida per tutte le fasi senza finestre.
4. **Determinismo**: niente `DeltaTime` non controllato nella logica dei turni; niente dipendenza dall'ordine di container non ordinati; ogni RNG usa seed/stream espliciti; ogni formato serializzato è versionato.
5. **Server autoritativo** per ogni decisione di gameplay; il client calcola solo preview. Nell'MVP offline l'autorità è già isolata in `ARTTurnManager` (predisposizione al multiplayer).
6. **Privacy dell'intento**: le intenzioni di pianificazione non raggiungono i client avversari — stato server + replica filtrata per squadra + autorizzazione server-side (invariante di `Intenti condivisi`). Nell'MVP offline: nessuna mossa avversaria mostrata/replicata durante la pianificazione.
   > **Esteso il 2026-08-08 da [D-021](../decisions/RT_PDR_00_Decision_Log.md)** — *anche il tempo è un canale*. La formulazione «payload filtrato per squadra» non copre le finestre di reazione: una **pausa osservabile** al decision boundary dice all'avversario che una finestra si è aperta, su quale micro-step e per quanto si è pensato, senza che un solo byte lo attraversi. La sospensione **logica** resta globale (serve al determinismo, [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) §5); la **presentazione avversaria** non deve avere una pausa variabile correlata alla scelta altrui. Zero leak comprende ora payload **e** timing.
7. **Combat math = funzioni pure** in `URTCombatLibrary`, coperte da test.

### Classi principali (prefissi `RT`/`URT`)

> ⚠️ **Riallineata al codice il 2026-08-07.** La versione precedente elencava **quattro classi su dieci che non
> esistono**: `ARTGameState` e `URTTurnResolver` (mai realizzate — l'autorità vive in `ARTTurnManager`),
> `URTGridLibrary` (rimossa col substrato quadrato al **CP 7.2**) e `URTAbilityData` (sostituita dal catalogo
> data-driven `URTActionData`/`URTHeroData` in **E1**). Un documento vincolante che descrive codice inesistente
> è peggio di un documento assente: chi lo legge agisce sul dato falso. Elenco riproducibile con
> `grep -rhoE "^class REFACTORTACTICS_API [AU]RT[A-Za-z]+ : public [A-Za-z]+" Source/RefactorTactics/ | sort -u`.

| Classe | Tipo | Responsabilità |
|---|---|---|
| `ARTGameMode` | GameMode | Regole autorevoli, allestimento della partita, avanzamento fasi |
| `ARTPlayerController` | PlayerController | Input, selezione, pianificazione |
| `ARTCameraPawn` | Pawn (BP `BP_TacticalCamera`) | Camera tattica pan/zoom |
| `ARTHUD` | HUD | Barre, timer, fase, combat log, anteprime dei piani |
| `ARTTurnManager` | Actor | Orchestrazione del turno; **unico punto di autorità** (invariante #5) |
| `ARTUnit` | Actor | Unità: team, HP, scudo, energia, cella (`FRTCellId`), kit |
| `ARTHexMapActor` | Actor | Rendering della mappa via ISM — **nessun Actor per cella** |
| `URTHexMapAsset` | PrimaryDataAsset | Mappa esagonale multilivello + hash stabile |
| `URTActionData` · `URTHeroData` · `URTEquipmentData` | PrimaryDataAsset | Catalogo azioni · eroi · equipaggiamento (**no GAS**) |
| `URTMatchFormatData` | PrimaryDataAsset | Parametri di formato: `RoundLimit`, timer ([D-010](../decisions/RT_PDR_00_Decision_Log.md)) |
| `URTHexSimLibrary` | BlueprintFunctionLibrary | Snapshot, budget di movimento, collisioni simultanee |
| `URTHexPathLibrary` | BlueprintFunctionLibrary | A\* esagonale multilivello, costi interi |
| `URTHexVisionLibrary` | BlueprintFunctionLibrary | LOS e forme di targeting (Line/Cone/Area) |
| `URTHexCombatLibrary` · `URTCombatResolver` | BlueprintFunctionLibrary | Risoluzione del combattimento su hex |
| `URTCombatLibrary` | BlueprintFunctionLibrary | Combat math: **funzioni pure** (invariante #7) |
| `URTActionQueueLibrary` · `URTActionEffectLibrary` · `URTActionFallbackLibrary` | BlueprintFunctionLibrary | Motore azioni: ordine per priorità intera, effetti, fallback (E4) |
| `URTReactionLibrary` | BlueprintFunctionLibrary | Reazioni componibili e Intercept (E5) |
| `URTHexBotLibrary` | BlueprintFunctionLibrary | Utility scoring del bot |
| `URTTurnLogLibrary` | BlueprintFunctionLibrary | TurnLog: hash, serializzazione versionata, reason code |
| `URTIntentPrivacyLibrary` | BlueprintFunctionLibrary | `FilterForTeam` → `FRTIntentView` (invariante #6) |
| `URTHexCoverLibrary` | BlueprintFunctionLibrary | Copertura direzionale bassa/alta, bordi, danno a struttura (E9) |
| `URTTerrainLibrary` | BlueprintFunctionLibrary | Superfici, stati temporanei, propagazione (E8) |
| `URTMatchSetupLibrary` · `URTMatchFormatLibrary` | BlueprintFunctionLibrary | Allestimento e parametri di formato; fine partita a tre vie |
| `URTScenarioLoader` · `URTScenarioRunner` · `URTTestReportWriter` | BlueprintFunctionLibrary | **Scenario Test Harness**: JSON versionato → percorso di gioco reale → `result.json` |

> Questa tabella elenca le classi **load-bearing** citate dal canone, non tutte: al 2026-08-08 il comando qui
> sopra ne restituisce **40**. La mappa completa e aggiornata è di
> [`../technical/architettura-codice.md`](../technical/architettura-codice.md), che ne è l'owner — il canone
> non deve diventare un secondo inventario da tenere sincronizzato.

### Convenzioni asset

Naming `<Tipo>_<Feature>_<Nome>_<Variante>`: `BP_` Blueprint · `BPC_` Component · `WBP_` Widget ·
`ABP_` AnimBlueprint · `DA_` DataAsset · `DT_` DataTable · `Curve_` Curve · `SM_`/`SK_` Static/Skeletal Mesh ·
`M_`/`MI_` Material/Instance · `T_` Texture · `NS_` Niagara · `SFX_`/`MUS_` Suono/Musica · `L_` Level ·
`IA_` InputAction · `IMC_` InputMappingContext.

Gli asset proprietari vivono sotto **`/Game/RT/`** con organizzazione **feature-first** (un asset sta vicino
alla feature che lo possiede; niente cartelle globali per tipo). Regole vincolanti — struttura, posizionamento,
dipendenze consentite fra cartelle, procedura di spostamento e checklist —
in **[`convenzioni-contenuti-ue.md`](../technical/convenzioni-contenuti-ue.md)**.

### Layout del progetto (radice repo)

```
RefactorTactics.uproject
Source/RefactorTactics/        # codice C++ autorevole (Core, Map, Grid, Unit, Turn, Combat, Bot, UI)
Source/RefactorTacticsEditor/  # modulo editor-only (Hex Editor Mode)
Content/RT/                     # asset proprietari, feature-first (vedi convenzioni-contenuti-ue.md)
Content/FabAsset/Paragon/       # asset di terze parti (Fab/Epic), non versionati
Config/                         # DefaultEngine.ini, DefaultGame.ini, ...
docs/                           # documentazione (design, guide, PDR, sorgenti PDF)
SourceAssets/                   # sorgenti non importati (.blend/.psd/...) — creata alla prima necessità
```

### 5.1 Ordinamento deterministico degli effetti simultanei (APNAP) — 2026-08-02

Recepito da [`spec-sequenza-turno.md`](../gameplay/spec-sequenza-turno.md) §3 (panel `/sc:spec-panel`). Estende
l'invariante #3: quando più effetti risolvono nello stesso istante e **l'ordine conta** (es. scudo/buff/reazione
prima del danno), l'ordine è dato da una **regola totale deterministica**, mai dall'ordine di inserimento nel
container.

**Ordine di gruppo (APNAP-adattato):** 1) sistema/State (morti, scadenze) → 2) unità attiva → 3) alleati
dell'attivo → 4) avversari → 5) terreno/oggetti → 6) globali di scenario. Intra-gruppo:
**velocità → priorità (intera) → tie-break assoluto** (id unità / coord stabile, come lo `StableTieBreak` del
path finding) → così due effetti pari non dipendono mai dall'ordine del container.

**Requisiti vincolanti** (SMART; esempi Given/When/Then e test plan in `spec-sequenza-turno.md` §3):
- **`FR-RESOLVE-01`** — ordine totale deterministico per effetti simultanei (gruppi APNAP + tie-break
  assoluto). Generalizza l'attuale "danni sommati per bersaglio" ai casi ordine-dipendenti. *Verifica:
  permutare l'array di input non cambia il log eventi.*
- **`FR-RESOLVE-02`** — **State-Based Actions** (morte a HP≤0, scadenza status) controllate **fra un effetto e
  il successivo**; un bersaglio morto invalida gli effetti pendenti che lo riguardano.
- **`FR-RESOLVE-03`** — nessun **float** nell'ordinamento/hash; priorità intere (coerente con l'invariante #4, **Determinismo**).

**Scope:** regola del *resolver puro* (offline e futuro server-authority). **Non** introduce finestre di
reazione live né categorie di velocità/`EndOfPhase` (north-star, `spec-sequenza-turno.md` §4). Implementazione
**gated**: si esercita quando esistono effetti ordine-dipendenti (buff/scudo/reazioni); il combat attuale a
"danni sommati" resta un caso particolare già conforme.

---

## 6. Regole di gioco numeriche (valori iniziali, da bilanciare)

> ⚠️ **Superata in parte dall'[ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) (2026-08-05)** per la release v0.1:
> il movimento passa da «4 celle / Dash 3» a **5 MP** con costo intero per cella (1 normale, 2 difficile,
> 2 salita via rampa; Sprint 8 MP), la griglia è esagonale (ADR-0002) e la vittoria non è più solo per
> eliminazione (obiettivi dinamici + limite di round). I valori qui sotto restano il riferimento
> **storico dell'MVP quadrato**; quelli vigenti per la v0.1 sono nei cataloghi di
> `docs/balance/` (creati in CP 1.2, issue `#28`).
>
> ⚠️ **Revisione 2026-08-07 — DURATA, ROUND E SCALA DELLE MAPPE**
> ([`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md)): il «limite di **12 turni**»
> smette di essere una regola universale e diventa un **parametro di formato** (`RoundLimit`), insieme a timer
> di planning, countdown del Ready, durata della Fast Reaction e banda della resolution. Il principio guida è
> **«compatto nel tempo, non necessariamente piccolo nello spazio»**: la scala delle mappe si misura in **Move
> necessari per raggiungere una zona rilevante**, non in celle, e non è vincolata a quella di Atlas Reactor.
> Le macro-fasi **non cambiano**.

**Valori vigenti** (v0.1, [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md); i dettagli per azione/eroe/terreno sono
nei cataloghi di [`balance/`](../balance)):

| Parametro | Valore vigente |
|---|---|
| Griglia | **esagonale** pointy-top, dimensione della cella dall'asset mappa (ADR-0002) |
| Occupazione | max 1 unità/cella |
| Movimento normale | **5 MP**, costo intero per cella (1 normale, 2 difficile/rampa) |
| Mobilità rapida | Sprint 8 MP; Dash/Charge/Leap a **distanza fissa** dichiarata dall'azione |
| Fine partita | eliminazione **oppure** obiettivi **oppure** `RoundLimit` (parametro di formato — v0.1 2v2: **10–14**, hard cap 14–16) |
| Reazioni | slot Reazione con trigger, **1 attivazione per turno**; finestra interattiva **3,0 s** ([ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) §8) |
| Timer pianificazione | **30 s** in 2v2 (configurabile, da tarare sul misurato); baseline 3v3 Standard **40–45 s** |
| Ready | Ready anticipato + **countdown annullabile di 3 s** quando tutti sono Ready ⏳ *non ancora implementato* |
| Durata della partita | target 3v3 Standard **25–30 min**; **45 min** è il tetto da evitare, non un obiettivo |

**Valori storici dell'MVP quadrato** (tabella conservata come riferimento di *parità*, non come regola vigente):

| Parametro | Valore (storico) |
|---|---|
| Griglia | 10×10, cella 200 cm |
| Occupazione | max 1 unità/cella |
| Movimento normale | 4 celle *(superato: 5 MP)* |
| Dash | 3 celle *(superato: distanza fissa per azione)* |
| HP unità | 100 |
| Scudo base | 20 (assorbito prima degli HP) |
| Attacco base | 30 |
| Energia | max 100 (guadagno per turno / per colpo: **da bilanciare**) |
| Copertura | riduce il danno (valore da bilanciare; `02` cita 50%) |
| Timer pianificazione | 30 s |
| Conflitto: destinazione contesa | entrambe le unità restano ferme |
| Conflitto: scambio diretto | consentito |
| Conflitto: cella occupata da unità ferma | movimento bloccato |
| Morte | applicata dopo l'intero batch della fase Blast |

---

## 7. Roadmap

La roadmap vive in **[`roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md)** (unica vista di esecuzione: milestone,
checkpoint, DoD misurabili, stato). Sintesi:

- **M0–M5** — MVP quadrato della fase tutorial: **archiviate** (consegnate, non si riaprono).
- **H0–H6.5** — fondamenta della griglia esagonale: consegnate ([`hex-map-roadmap.md`](../roadmap/hex-map-roadmap.md)).
- **M6 Parità hex** → **M7 Dismissione del quadrato** → **M8 Presentazione** → **M9 Ambienti/editor** →
  **M10 Rete e privacy** → **M11 Production readiness**.

Non si duplicano qui stati o date: se le due fonti divergono, la roadmap è quella autorevole per lo *stato*,
questo file per le *decisioni*.

---

## 8. North-star (post-MVP, dai PRD)

Esplicitamente **fuori** dall'MVP, in ordine di priorità indicativa (P0 → P3):

- **P0**: pulizia resolver deterministico, test conflitti, **multiplayer server-authoritative** (replica azioni/GameState, timeout/reconnect), lobby privata.
- **P1**: 4v4, 6+ eroi, draft, fog of war, replay, spectator, matchmaking, ranking, **Intenti condivisi** completo.
- **P2**: loadout/moduli, **migrazione a GAS**, tutorial interattivo, accessibilità, localizzazione, controller, Steam/EOS, mappa multilivello.
- **P3**: console, cosmetici, anti-cheat, **modding** (Blueprint sandbox *vs* Lua/UnLua — **decisione aperta** nei PRD).

### 8.1 Riferimento dettagliato (north-star)

Il documento più completo per il post-MVP è **`docs/RefactorTactics_ Product Requirements Document e piano
completo di sviluppo.pdf`** (45 pagine). Rispetto ai 3 PRD aggiunge decisioni/specifiche non altrove presenti,
da usare come riferimento quando le relative feature entrano in scope:

- **Modalità "Relay Control"**: relay da controllare a fine round, ruota ogni 2 round, vittoria a punteggio, knockout con rientro, max 12 round.
- **Economia del round + 7 intent label**: Focus, Protezione, Scout, Controllo, Fuga, Trappola, Attesa (1 movimento + 1 azione + 0-1 reazione + 0-1 interazione, 1 label).
- **Config build competitiva**: chassis fisso, 1 spec su 3, 2 modificatori abilità, 2 gadget, 2 tratti a budget, 1 ultimate su 2-3 + tabella varianti.
- **Formule**: costo traversal, euristica A* multilivello, utility IA (nel PDF).
- **Modello dati ricco**: 12 struct (`FRTGridCellId`, `FRTCellStaticData/RuntimeState`, `FRTTraversalEdge/Profile`, `FRTPlannedAction/Turn`, `FRTTeamIntentView`, `FRTResolvedEvent`, `FRTModManifest`, `FRTRunState`) + 12 moduli `RTCore…RTEditor`.
- **Roguelike cooperativo**: 1-4 giocatori, 3 atti, deck 8-12, energia, reliquie, save versionati, "stanze curate + validator".
- **Modding**: 4 livelli (Data/Content/Game Feature/Native), 3 schemi JSON, 16 operazioni autorizzate, handshake su hash del manifest.
- **Analytics/Test/Rilascio**: 15 eventi analytics, test plan (unit/deterministico con golden hash/rete/funzionale) + log per-round, 13 gate di release, risk register (15 rischi).
- **Toolchain/hardware** e tabella migrazione C#→Unreal.

**Conflitti col MVP** (restano distinzioni north-star, NON cambiano l'MVP): il PDF è **4v4**, mette **GAS** nello
stack, vittoria **a punteggio**. L'MVP resta 2v2, no-GAS, con la vittoria a più vie dell'ADR-0003.
Nota naming: il PDF usa `FRTGridCellId` (modello ricco); l'MVP usa `FRTGridCoord{X,Y}` (semplice) —
da riconciliare se in futuro si adotta il modello a chunk multilivello.

> **Non è più un conflitto**: la **pianificazione a 40-60 s** del PDF. La baseline del formato principale
> 3v3 è ora **40–45 s** ([`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md) §7);
> i **30 s** restano il valore del **2v2 corrente**, da tarare sul misurato, non da alzare per analogia.
> Anche il «max 12 round» della modalità *Relay Control* elencata sopra va letto come il valore del **suo**
> formato, non come limite del gioco.

### 8.2 Sequenza di risoluzione ricca — reazioni/reveal/timeline (north-star)

Il design esplorativo [`sequenza-turno.md`](../archive/gameplay/sequenza-turno-exploratory.md) (trascrizione del PDF omonimo) propone un
modello di risoluzione molto più ricco: pianificazione segreta → **reveal progressivo** → **finestre di
reazione** (stack LIFO stile *Magic*) → risoluzione con ordinamento **APNAP** → cleanup, con 5 categorie di
velocità e budget di reazione. Consolidamento, conflitti con gli invarianti #3/#4 e la parte adottata per prima
— **ordinamento deterministico degli effetti simultanei** (APNAP + tie-break totale) — in
[`spec-sequenza-turno.md`](../gameplay/spec-sequenza-turno.md).

> **Aggiornamento 2026-08-06 ([ADR-0003](../decisions/adr-0003-modello-azioni-v01.md), CP 1.1)**: le **reazioni non sono più
> fuori scope**. La v0.1 adotta uno **slot Reazione** con trigger dichiarato e **1 attivazione per turno**
> (epic **E5**). Resta north-star tutto il resto di questo modello: stack LIFO, finestre di reazione *live*,
> reveal progressivo e le 5 categorie di velocità — che confliggerebbero con la risoluzione "raccogli poi
> applica" (invariante #3). La reazione della v0.1 è **deterministica e senza finestre**: il trigger si valuta
> nello snapshot della fase, non in una finestra interattiva.
>
> ⚠️ **Aggiornamento 2026-08-07 — le finestre di reazione rientrano in scope.** Il documento
> `docs/src/RefactorTactics_Overwatch_FastReaction_Claude.md` propone l'Overwatch come primo caso di un modello
> generale di reazione con *decision boundary* e finestra di 3 s. La riconciliazione adottata è la via **(b)**
> già prevista da [`spec-sequenza-turno.md`](../gameplay/spec-sequenza-turno.md) §3 C1: **l'invariante #3 si compone, non
> si deroga** — il turno diventa una sequenza di sotto-risoluzioni, ciascuna «raccogli poi applica» con
> snapshot proprio, e l'input del giocatore entra nel TurnLog **come dato** (il timeout è una funzione pura).
> Tutte le reazioni passano a un modello unico *opportunity → commit*, di cui quello di E5 è il caso degenere
> (`AllowedResponses ≤ 1` → commit immediato, nessuna finestra).
> Decisioni **D16–D22** e checkpoint in [`brief-overwatch-reazioni.md`](../gameplay/brief-overwatch-reazioni.md) (epic
> **E14**). ✅ **Formalizzato in [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md)** (2026-08-07, = CP 14.1): questo
> paragrafo **è canone**, e l'invariante #3 di §5 è riformulato di conseguenza. Restano north-star lo stack
> LIFO interattivo, gli interrupt annidati, il reveal progressivo e le 5 categorie di velocità.

---

## 9. Decisioni ancora aperte (business & prodotto)

Non bloccano l'MVP tecnico, ma vanno decise crescendo:

- **Budget** e **modello commerciale**: non specificati in nessun documento.
- **Team**: assunzioni discordanti (single-dev / 3–5 FTE / 4–6). L'MVP assume **dev singolo**.
- **Direzione artistica**: inesistente. L'MVP usa asset placeholder / Starter Content.
- **PC target hardware**: mai definito → i target di performance (60 FPS, A* <2 ms, turn <100 ms) restano **budget da misurare**, non garanzie.
- **Piattaforme**: MVP solo Windows; controller/console rimandati.
- **Identità originale**: nomi eroi, lore e naming definitivo per la pubblicazione.
