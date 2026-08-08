# REFACTORTACTICS — PDR-00 · Decision Log

> **Sorgente Markdown canonica** del Decision Log (PDR-00 §4), residente in Git per la regola di manutenzione
> PDR-00 §6 #5: *«I PDF sono snapshot di consultazione: le sorgenti testuali devono vivere nel repository Git.»*
> Trascrive fedelmente il Decision Log dallo snapshot PDF `RT_PDR_00_Indice_Governance_v0.1.pdf` (pag. 3) e vi
> aggiunge le decisioni successive. **Owner**: PDR-00. **Regola di prevalenza** (PDR-00): *decisioni esplicite
> del progetto > requisiti consolidati > proposte PDR > ricerca web di supporto.*

## Stati delle decisioni (PDR-00 §3 «Principi di consolidamento»)

Gli stati **non vengono mescolati**: `Consolidato · Assunzione · Proposta · Open question`.
Regola di manutenzione #1: *ogni modifica a un requisito aggiunge o aggiorna una voce in questo log.*

## Decision Log

| ID | Decisione | Stato | Impatto |
|---|---|---|---|
| D-001 | Formato principale 3v3; vertical slice 2v2 | **Assunzione da bloccare** *(declassata il 2026-08-07, vedi D-011)* | Scope, UI, rete, bilanciamento |
| ~~D-002~~ | ~~Massimo 12 turni; planning 30 s; resolution 6-12 s~~ | **Superata da D-010** | Tempo partita e UX |
| D-003 | Server authoritative; client propone | Consolidata | Rete, validazione, anti-cheat |
| D-004 | C++ per simulazione/rete; Blueprint per contenuti e presentazione | Consolidata | Ownership del codice |
| D-005 | GAS non è l'autorità del simulatore | Consolidata | Confine abilities/resolver |
| D-006 | Mappa come grafo tattico 3D con `FRTCellId` | Consolidata | Pathfinding, targeting, ambienti |
| D-007 | UE 5.8 baseline per questa edizione PDR | Assunzione da bloccare | Build, API, toolchain |
| D-008 | Gameplay Framework legacy replication come primo target; Iris valutato dopo vertical slice | Proposta semplice | Riduce rischio iniziale |
| **D-009** | **Le sorgenti canoniche dei PDR vivono in Git (Markdown); i PDF `v0.1` restano snapshot di consultazione. Prima applicazione: PDR-10 → [`RT_PDR_10_Roadmap_QA_Rischi_v0.2.md`](../roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md)** | **Consolidata** | **Governance/manutenzione (attua PDR-00 §6 #5)** |
| **D-010** | **Durata, budget del round e scala delle mappe sono parametri di formato, non costanti.** Target 3v3 Standard **25–30 min** (tetto ~45); `RoundLimit` **16–20** in 3v3 e **10–14** in 2v2; planning max **40–45 s** in 3v3 (30 s nel 2v2 corrente); **Ready anticipato + countdown annullabile 3 s**; **Fast Reaction 3 s**; resolution ~**12–20 s** (8–15 s in 2v2). Principio: **«compatto nel tempo, non necessariamente piccolo nello spazio»** — la scala della mappa si misura in **Move per raggiungere una zona rilevante**, non in celle. Dettaglio: [`../design/spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md) | **Consolidata** *(i valori numerici restano baseline da playtestare)* | **Supera D-002.** Tempo partita, UX, level design, formati |

| **D-011** | **Il formato principale non è deciso: D-001 è declassata da *Consolidata* ad *Assunzione da bloccare*.** Il 3v3 resta la baseline di lavoro e le bande di D-010 restano coerenti **come ipotesi dichiarate**. Il **4v4** è ammesso in roadmap **solo** come scenario di validazione di stress (epic **E17**), **non** come formato candidato, finché non esiste una misura. La decisione si consolida con il primo dato reale su una partita ≥3v3 | **Consolidata** *(la decisione di non decidere è essa stessa una decisione)* | Nessun effetto su v0.1 (che è 2v2). Toglie autorità normativa alle bande 3v3 di D-010 |
| **D-012** | **L'Overwatch è universale e compete con l'azione offensiva del turno**: `Attack` **oppure** `Ability` **oppure** `Overwatch`, mai sommati, salvo eccezione dichiarata da un'abilità. Non tutte le opportunity aprono una finestra: i tre regimi *Automatic · Conditional · FastSelect* **emergono dai dati** (`AllowedResponses` di ADR-0004 §2 + una **condizione dichiarata in planning**), **non** da un enum di policy parallelo. L'unica aggiunta rispetto ad ADR-0004 è la condizione dichiarata, e il suo gate è il **DoD di CP 14.3** | **Consolidata** *(l'effetto esatto e i numeri restano da playtestare)* | E14, action economy, durata della resolution. Owner: [`../gameplay/brief-azioni-generiche-overwatch.md`](../gameplay/brief-azioni-generiche-overwatch.md) |
| **D-013** | **Un trigger su transizione è possesso della definizione predittiva, non della mappa.** Una trap/tripwire dichiara la propria coppia `(From → To)` e i tipi di transizione validi; il resolver la confronta col micro-step del movimento. `FRTHexEdge` resta riservato alle **sole transizioni fra layer**: gli adiacenti sullo stesso layer restano **calcolati** da `URTHexLibrary::Neighbors`, non memorizzati | **Consolidata** | Il formato e l'hash di `URTHexMapAsset` **non cambiano**; nessun vincolo di precedenza rispetto a E9. Owner: [`../gameplay/brief-delayed-actions.md`](../gameplay/brief-delayed-actions.md) |

| **D-014** | **Azioni generiche canoniche**: `Wait · BasicAttack · Interact · Brace · Move · Overwatch`. `Activate` è **assorbita semanticamente da `Interact`**; `Guard` **non è più una fondamentale universale** (resta stance/capacità specifica dove il kit la richiede); `Overwatch` è universale come framework, ma il **profilo dipende dall'eroe**. Economia standard: `Attack` **oppure** `Ability` **oppure** `Overwatch`, salvo eccezione dichiarata (≡ D-012) | **Consolidata** *(i costi restano balance data)* | Catalogo azioni, E4/E7/E14. Owner: [`../gameplay/brief-azioni-generiche-overwatch.md`](../gameplay/brief-azioni-generiche-overwatch.md). **Gli Stable ID legacy non si cancellano**: migrazione tracciata, vedi Note |
| **D-015** | **`Sneak · Normal · Sprint` sono profili della famiglia `Move`**, non azioni distinte. **`Sprint` non è un Dash**: `Dash/Charge/Leap/Blink/Reposition` restano mobilità speciali **pre-Blast**, il Move normale resta l'ultima fase volontaria. Distanza, rumore, exposure e costi sono **data-driven** | **Consolidata** *(l'eventuale costo extra di slot dello Sprint resta tuning da playtest)* | Catalogo azioni, `MovementStyle`, E13 (il rumore è ciò che distingue i profili) |
| **D-016** | **Un solo thin slice di Predictive Action nella v0.1**, preferibilmente `Vektor.InterceptShot`: decisione **completa in Planning** → trigger/boundary deterministico → risoluzione automatica se la previsione è corretta → whiff/fallback dichiarato → **nessun input umano durante la Resolution**. Il framework completo di trap/tactical gambit resta **fuori** dalla v0.1 | **Consolidata** | **Separa** `Delayed/Predictive Action` da `Fast Action` e da `Fast Reaction`. Owner: [`../gameplay/brief-delayed-actions.md`](../gameplay/brief-delayed-actions.md) |
| **D-017** | **`Intercept` rivalida la geometria sul bersaglio effettivo.** Se il colpo destinato ad A viene intercettato da B: l'identità dell'azione e i dati **indipendenti dal target** restano invariati; LOS, traiettoria, copertura applicabile, facing e difese geometriche si rivalidano **su B**. La rivalidazione **non apre** una nuova Reaction Opportunity e **non** introduce nested reaction. Il TurnLog spiega il redirect e il contesto difensivo effettivamente usato | **Consolidata** | Supera il limite dichiarato in [`../gameplay/spec-copertura-cp91.md`](../gameplay/spec-copertura-cp91.md). Serve un test discriminante con A e B a copertura diversa |
| **D-018** | **`HighGround` non dà un bonus numerico piatto alla vista nella v0.1**: nessun `+1 VisionRange` di default. La quota vale già attraverso geometria, LOS, occlusione, copertura e topologia/layer. Un bonus numerico futuro richiede **playtest e decisione separata** | **Consolidata** | Chiude la domanda aperta di [`../gameplay/brief-conoscenza-parziale.md`](../gameplay/brief-conoscenza-parziale.md) §10.1 e la nota non quantificata di `spec-terreni-e8.md` |
| **D-019** | **`Fast Action` e `Fast Reaction` sono categorie semantiche distinte**, sulla stessa infrastruttura `DecisionWindow`: *Fast Action* = scelta live limitata come **continuazione esplicita di una propria azione**; *Fast Reaction* = scelta live provocata da un **evento esterno**. Una Fast Action **non** è una Delayed Action. La v0.1 **non** inventa una Fast Action concreta finché nessuna ability reale la richiede. Baseline comune di finestra: **3,0 s** | **Consolidata** | Glossario temporale, ADR-0004, `spec-durata-partita-e-scala-mappe.md` (che usava «Fast Action» per l'azione dichiarata in Planning: **uso errato**, corretto) |

## Note

- **D-014…D-019** (2026-08-08): approvate in sessione e recepite dal consolidamento di `docs/gameplay/`
  (handoff `../src/RefactorTactics_DocsGameplay_Audit_Consolidamento_Claude_2026-08-08.md`). Gli ID proposti
  dall'handoff erano liberi e sono stati usati così com'erano.
- **Migrazione degli Stable ID legacy (D-014, D-015)**: `Action.Guard`, `Action.Activate` e `Action.Sprint`
  **esistono e sono consumati** — misurato al 2026-08-08: `Action.Sprint` in **9** file di codice e **6** di
  test, `Action.Guard` in **4** e **4**, `Action.Activate` in **1** e **1**. La nuova tassonomia è **semantica
  di gameplay**, non un rename immediato: cancellarli in una PR documentale romperebbe test e replay. La
  migrazione è tracciata come issue, con Stable ID/replay safety e validator come requisiti.
- **D-016 e `Vektor.InterceptShot`**: oggi l'azione è a catalogo con `ERTActionSlot::None` e **nessun trigger**,
  con il rinvio a **E14** dichiarato *nei dati* (`RTHeroCatalogLibrary.cpp`) perché il suo trigger è d'ingresso
  su movimento. Trattarla come **Predictive Action** precommitted la **sgancia da E14**: non le serve una
  finestra interattiva, le serve un boundary deterministico. È una semplificazione, non uno scope creep — ma
  resta una **migrazione di classificazione** da tracciare, non da fare in una PR documentale.
- **D-001…D-008**: trascritti verbatim dallo snapshot `RT_PDR_00_Indice_Governance_v0.1.pdf` (pag. 3). Se il PDF
  viene aggiornato, questa sorgente Git prevale (regola #5) e il PDF va rigenerato di conseguenza.
- **D-001 → D-011** (2026-08-07): la voce **non** è barrata perché il suo contenuto non è stato sostituito —
  è cambiato il suo **stato**. D-001 dichiarava *Consolidata* una scelta mai misurata: nessun 3v3 e nessun 4v4
  sono mai stati giocati, e il KPI di [`../roadmap/roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md) lo
  dice esplicitamente («il 3v3 non esiste: in v0.1 si misura la banda 2v2»).
  È la **stessa forma di D-002** — tre numeri scelti e mai misurati, poi superati da D-010 nel momento in cui
  una misura è arrivata (`HexMatch.PlaysToCompletion`: 25 round contro i 12 previsti). Sostituire «3v3» con
  «4v4» oggi avrebbe rifatto lo stesso errore con un numero diverso; declassare registra ciò che davvero
  sappiamo. Precedente di forma: **D-007**, *Assunzione da bloccare* dal principio.
- **D-012, D-013** (2026-08-07): chiudono `OD-3` e `OD-4` della sessione `/sc:brainstorm`. D-013 nasce da una
  verifica sul codice che ha **riformulato la domanda**: si chiedeva «gli archi portano trigger?», ma gli archi
  degli adiacenti **non esistono** — `GraphNeighbors` li calcola. La domanda vera era dove vivesse la coppia,
  e la risposta che non tocca la mappa è anche la più economica.
- **D-002 → D-010** (2026-08-07): la voce non è cancellata ma **barrata**, perché la regola di manutenzione #1
  chiede che ogni modifica a un requisito *aggiorni* il log, non che ne riscriva la storia. I tre numeri di
  D-002 erano scelti, mai misurati; D-010 li sostituisce con **bande per formato** e li marca come baseline da
  playtestare. Il PDF snapshot resta indietro fino alla prossima rigenerazione.
- **D-007** resta *Assunzione da bloccare*: nel repo la patch è di fatto bloccata a **UE 5.8.1**
  (vedi `CLAUDE.md` e `piano-canonico-mvp.md`); la formalizzazione a *Consolidata* è una decisione futura.
- Divergenze note MVP↔PDR (segnalate, prevale il canone MVP): **rete/privacy** anticipata dal PDR (F1) vs
  differita nell'MVP; **GAS** previsto a F2 vs **No-GAS nell'MVP**. Dettaglio in
  [`../design/roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md) §«Allineamento con i PDR».
