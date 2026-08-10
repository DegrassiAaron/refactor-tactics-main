# Decisioni aperte

> `OPEN` · **Stato**: vivo · **Ultimo aggiornamento**: 2026-08-10
> **Cosa è**: l'elenco di ciò che **aspetta una persona**. Nessuna di queste voci può essere chiusa
> deducendola dai documenti: o mancano i dati, o due fonti si contraddicono senza gerarchia.
> **Cosa non è**: il registro delle decisioni prese — quello è il
> [Decision Log](decisions/RT_PDR_00_Decision_Log.md), che resta l'**owner**. Quando una voce qui si chiude,
> diventa una `D-0xx` lì e sparisce da qui.

---

## Aperte — geometria, acqua e strutture, dal quinto sorgente del 2026-08-10

Origine: [triage `FULL CHAT CONSOLIDATION`](roadmap/plans/triage-grid-geometry-water-2026-08-10.md).
Tre domande, tutte da rispondere **prima** della spec del blocco corrispondente.

| ID | Domanda | Perché serve una risposta |
|---|---|---|
| [`GEO-1`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/429) | `WaterDepth` è un **campo della cella accanto a `Surface`**, oppure una **superficie composta**? | Il sorgente lo dà come asse ortogonale — una cella può essere `ShallowWater` e cambiare profondità durante il match. Il repository ha **otto superfici piatte** (CP 8.1) che non cambiano mai. La scelta decide se CP 8.1 si **estende** o si **riscrive**, e con essa i costi di movimento, la conduzione elettrica e il formato della mappa |
| [`GEO-2`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/430) | Come si chiama lo **slot strutturale** della §40, dato che `Gadget.BreachCharge` esiste già? | Il sorgente lo chiama `Breach`, ma nel repository `BreachCharge` è l'**oggetto che lo aprirebbe** (35 danni a struttura, E7). Due entità con lo stesso nome, una che agisce sull'altra, si pagano a ogni lettura del TurnLog — è lo stesso costo che ha fatto scartare `Reposition` in [D-070](decisions/RT_PDR_00_Decision_Log.md) |
| [`GEO-3`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/431) | Le §22–§27 (move end semantics, grafo causale, provenance, credit) come si riconciliano col **TurnLog v6**? | Non è una domanda di design ma di **sequenza**: il TurnLog v6 e i reason code sono atterrati lo stesso giorno ([D-062](decisions/RT_PDR_00_Decision_Log.md), [D-063](decisions/RT_PDR_00_Decision_Log.md), [D-067](decisions/RT_PDR_00_Decision_Log.md)). Va guardata da chi ha quel codice in mano, non consolidata al buio da un triage documentale |

---

## ✅ Chiuse il 2026-08-10 — footprint della cella e derivazione degli status

Tre voci decise dall'autore. Restano qui **solo come indice**: il contenuto vive nel Decision Log.

| Era | Decisione presa | Dove vive ora |
|---|---|---|
| `MAP-1` | Il footprint standard è il **cerchio inscritto** nell'esagono (raggio = apotema), e la *swept clearance* della transizione usa **lo stesso raggio**. **Zero numeri nuovi**: l'apotema si deriva dal lato, già fissato. Si misura in **esagoni**, non in metri | [`D-071`](decisions/RT_PDR_00_Decision_Log.md) — **E23.6/23.7** |
| `STA-1` | Le sei primitive **non sono un enum**: si derivano da ciò che lo status dichiara. Stesso argomento con cui è stato respinto l'enum di policy dell'Overwatch — un campo accanto al comportamento è una seconda verità | [`D-072`](decisions/RT_PDR_00_Decision_Log.md) |
| `STA-2` | La severity `C0`–`C3` si **conta** dalle capability toccate. Così la regola anti-stun-lock diventa **un test** invece di una revisione | [`D-072`](decisions/RT_PDR_00_Decision_Log.md) |

> ⚠️ **Chiudere `STA-1` e `STA-2` ha aperto ciò che serviva a entrambe.** Nessuna delle due derivazioni è
> possibile senza una **tassonomia esplicita delle capability** — l'elenco di cosa un effetto può togliere e
> con quale granularità. Non esiste, e non è un dettaglio implementativo: è il dato da cui *entrambe* leggono.
> È il primo lavoro del framework, prima delle primitive e prima dei due status nuovi. Tracciata come
> `STA-4` qui sotto.
>
> `MAP-1` invece non ne ha aperte: ha **chiuso** anche il rischio che qualcuno introducesse un secondo
> numero per il corridoio.

---

## ✅ Chiuse il 2026-08-07 — sessione `/sc:brainstorm`

Le cinque voci `OD-1`…`OD-5` aperte dalla revisione documentale sono state decise. Restano qui **solo come
indice**, per chi cerca la sigla vecchia: il contenuto vive nel Decision Log.

| Era | Decisione presa | Dove vive ora |
|---|---|---|
| `OD-1` | Formato principale **non deciso**: D-001 declassata da *Consolidata* ad *Assunzione da bloccare*. Il 3v3 resta baseline di lavoro, il 4v4 solo scenario di stress | [`D-011`](decisions/RT_PDR_00_Decision_Log.md) |
| `OD-2` | Unità ausiliarie: **concetto unico**, solo vincoli architetturali, gameplay fuori dalla v0.1 | [`gameplay/brief-unita-ausiliarie.md`](gameplay/brief-unita-ausiliarie.md) |
| `OD-3` | L'Overwatch **compete** con l'azione offensiva; le tre policy entrano nel DoD di **CP 14.3** | [`D-012`](decisions/RT_PDR_00_Decision_Log.md) · [`gameplay/brief-azioni-generiche-overwatch.md`](gameplay/brief-azioni-generiche-overwatch.md) |
| `OD-4` | Il trigger su transizione è **possesso della trap**, non della mappa: `FRTHexEdge` resta per i soli salti di layer | [`D-013`](decisions/RT_PDR_00_Decision_Log.md) |
| `OD-5` | Scenario **4v4 di stress** in roadmap dopo E15, come validazione e non come produzione | epic **E17**, [`roadmap/roadmap-v0.1.md`](roadmap/roadmap-v0.1.md) |

**Nota di metodo, che vale più delle singole risposte.** Due delle cinque domande erano **mal poste**, e lo si è
scoperto guardando il codice invece dei documenti:

- `OD-4` chiedeva «gli archi portano trigger?». Gli archi degli adiacenti **non esistono**: `GraphNeighbors`
  li calcola da `URTHexLibrary::Neighbors`, e solo le transizioni fra layer sono dati. La domanda vera —
  *dove vive la coppia `(From→To)`* — ha una risposta che non tocca la mappa.
- `OD-1` era etichettata **bloccante**. La dimensione della squadra è un `TArray<FName>` sul GameMode, non un
  campo di `FRTMatchRules`: in v0.1 non si costruisce né un 3v3 né un 4v4. Bloccava la *documentazione*, non
  il codice.

Etichettare l'urgenza dai documenti, senza verificarla sul codice, aveva prodotto l'ordine di priorità
sbagliato: l'unica decisione che bloccava lavoro costruibile era `OD-3`.

---

## Aperte — facing e azioni base, dal consolidamento del 2026-08-08

Origine: l'handoff [`archive/src/handoff/2026-08-08-azioni-base-e-facing.md`](archive/src/handoff/2026-08-08-azioni-base-e-facing.md), che chiudeva con
quindici «decisioni ancora aperte». **Cinque erano già decise**, e una sesta era una domanda diversa da come
era posta. Non compaiono fra le voci aperte, perché una domanda già chiusa che resta scritta come aperta
invita a ridecidere ciò che è stato deciso:

| Domanda dell'handoff | Perché non è aperta |
|---|---|
| §38.4 — quanto può girarsi un'unità **ferma** | [ADR-0005](decisions/adr-0005-orientamento.md) §1: stile `None` → **sei** direzioni, rotazione libera, **non consuma slot** |
| §38.7 — quali Basic Attack usano `FaceTarget` | [D-020](decisions/RT_PDR_00_Decision_Log.md): **tutte** le azioni con bersaglio o direzione orientano prima di risolvere. Non è una proprietà per azione |
| §38.10 — quali forced movement preservano il facing | [ADR-0005](decisions/adr-0005-orientamento.md) §3: verso l'origine dell'**ultimo** spostamento subito nell'ordine canonico; **ambientale senza sorgente** → invariato; un Move volontario successivo vince |
| §38.14 — un'azione che ruota torna al facing precedente? | No: la timeline di D-020 va in avanti e non ha punto di ripristino. `FacingFinalAfterMove` **persiste** nel round dopo |
| §38.15 — «Dash azione base» player-facing vs tassonomia tecnica | [D-015](decisions/RT_PDR_00_Decision_Log.md) + [D-028](decisions/RT_PDR_00_Decision_Log.md) + [`balance/RT_ActionCatalog_v0.1.md`](balance/RT_ActionCatalog_v0.1.md) §2.2: `Dash` è **mobilità speciale in fase Dash**, slot movimento. Che la Wiki lo presenti come azione base non è un conflitto: la Wiki è **dichiaratamente non normativa** (invariante #12) |
| §38.5/§38.6 — valori di rotazione per personaggio | Non sono valori mancanti: sono un **modello diverso**. Vedi `FAC-1` |

### ✅ Chiuse il 2026-08-10 — [ADR-0008](decisions/adr-0008-rotazione-e-policy-di-facing.md)

Quattro delle dieci voci sono state decise dall'autore e non sono più aperte. Restano qui, barrate, perché
il registro deve dire **come** è andata a finire, non solo cosa manca.

| ID | Esito | Dove vive ora |
|---|---|---|
| ~~`FAC-1`~~ | **Accettata**: la rotazione è una **capacità del personaggio** in step (0–3). Otto numeri nuovi (2 × 4 eroi), valori iniziali da §23.1 dell'handoff, dichiarati **non** bilanciati | [ADR-0008](decisions/adr-0008-rotazione-e-policy-di-facing.md) §1 — supera ADR-0005 §1 |
| ~~`FAC-2`~~ | **Accettata**: policy dichiarative per azione ed effetto, **con default** che riproducono D-020 e ADR-0005 §3 — un'azione che non dichiara nulla si comporta come oggi | [ADR-0008](decisions/adr-0008-rotazione-e-policy-di-facing.md) §3 — supera ADR-0005 §3 |
| ~~`FAC-4`~~ | **Decisa**: il facing al boundary `k` è la direzione dell'**ultimo passo compiuto**, cioè `FacingFromPath` sul prefisso del percorso. Il pivot finale si applica dopo e **non** retroattivamente | [ADR-0008](decisions/adr-0008-rotazione-e-policy-di-facing.md) §2 |
| ~~`FAC-10`~~ | **Risolta** distinguendo i due termini: **pivot** = la capacità di ruotare, **rotazione dichiarata** = l'atto di sceglierla in planning. Il codice aveva già scelto `Declared*` per il secondo | [ADR-0008](decisions/adr-0008-rotazione-e-policy-di-facing.md) §4 |

> **L'opzione scartata su `FAC-4` merita di essere ricordata**, perché il motivo non è di gusto: «facing =
> direzione del **prossimo** passo» è stata esclusa perché il facing assunto è **pubblico** (ADR-0005 §5), quindi
> un avversario che lo osservasse a metà movimento dedurrebbe il percorso futuro — contro l'invariante **#6** e
> contro ADR-0004 §7-bis.
>
> ⚠️ *Rettifica del 2026-08-10*: qui era scritto «il test `Overwatch.OpportunityLeaksNoFuture` esiste per
> vietare». Quel test **non esiste**: è pianificato in ADR-0004 per E14, che non è implementata. Il requisito
> è dichiarato, la verifica no.

### Il modello — la proposta residua che cambierebbe ADR-0005

Riga **52** di [`DOC_CONFLICT_MATRIX.md`](DOC_CONFLICT_MATRIX.md). Oggi prevale l'ADR; la voce
esiste perché la proposta è coerente e nessun documento può accettarla al posto dell'autore.

| ID | Domanda | Cosa cambierebbe |
|---|---|---|
| `FAC-3` | `Brace` deve diventare **direzionale**? | ADR-0005 §4a dice il contrario in modo esplicito: `Deflect`, `Brace`, `Shield` proteggono la **persona**, non un lato. Cambiarlo è una modifica di §4a, e il test `Combat.ShieldWorksFromAnyDirection` (CP 16.2) esiste per impedire che accada per deriva. ⚠️ **Dal 2026-08-10**: con `FAC-2` accettata, `FAC-3` avrebbe una sede naturale in cui esprimersi — una policy di facing dichiarata su `Brace` — il che ne abbassa il **costo**, non ne cambia il **merito** |

### Le lacune — cose che il canone non dice affatto

Queste **non** contraddicono nessuna decisione: sono buchi. Vanno decise prima che E16 le incontri in codice.

| ID | Domanda | Perché serve una risposta |
|---|---|---|
| `FAC-5` | Una **reazione** può ruotare l'unità che reagisce? | D-020 nomina `FacingUsedByOverwatch` come valore **letto**, mai scritto. Se una Return Fire ruota verso l'attaccante, il facing cambia a metà round e i consumatori successivi lo ereditano: è una regola nuova, non una precisazione. ⚠️ **Dal 2026-08-10**: ADR-0008 §2 stabilisce che il pivot finale **non è retroattivo**, quindi una rotazione da reazione non riscriverebbe i boundary già passati — il che circoscrive il danno, ma non risponde alla domanda |
| `FAC-6` | `Interact` **richiede** un facing verso l'oggetto, oppure lo **impone**? | `Interact` è universale ([D-025](decisions/RT_PDR_00_Decision_Log.md)) e assorbe `Activate`: la risposta tocca porte, console e valvole di E9, non un solo dispositivo. ⏳ **Dal 2026-08-09 ha un consumatore concreto**: [`gameplay/spec-interazioni-mappa-cp101.md`](gameplay/spec-interazioni-mappa-cp101.md) §12, dove «girarsi verso la porta» smette di essere ipotetico — la domanda resta qui, l'owner resta [ADR-0005](decisions/adr-0005-orientamento.md) |
| `FAC-7` | Quali **status** limitano la rotazione, separatamente dal movimento? | Oggi nessuno: `Status.Root` interagisce con `Guard`, non col facing. Distinguere «non può muoversi» da «non può girarsi» è una scelta di design, non un dettaglio implementativo |
| `FAC-8` | Il **terreno** può limitare la rotazione (ghiaccio, condotti, scale)? | Fuori v0.1 — ma il ghiaccio esiste già in E8 e in `Visual/Environment/IceSlide`, quindi la domanda va registrata prima che qualcuno la risolva localmente in una spec di terreno |
| `FAC-9` | Il pathfinding deve diventare **orientation-aware** — stato `(CellId, Facing)` invece di `CellId`? | Fuori v0.1, e va tenuto fuori finché non c'è una misura: moltiplica per sei lo spazio degli stati di A\*, contro i budget già dichiarati in [`technical/spec-pathfinding.md`](technical/spec-pathfinding.md). Il ripiego dichiarato dall'handoff — path geometrico, facing derivato, pivot validato alla fine — **è** già il modello di ADR-0005. ⚠️ **Dal 2026-08-10 la pressione aumenta**: con `FAC-1` accettata la cella d'arrivo vale diversamente a seconda del lato da cui la si raggiunge, quindi la **preview** deve mostrare il facing ottenibile. Resta fuori v0.1, ma non più «senza motivo» |

### Aggiunte il 2026-08-10 dal pacchetto `Facing_Claude_Consolidation`

Quattro domande che **nessun documento poneva**, estratte da un pacchetto di 69 sezioni di cui 38 erano già
canoniche. Triage completo in
[`roadmap/plans/facing-consolidation-triage-2026-08-10.md`](roadmap/plans/facing-consolidation-triage-2026-08-10.md).

| ID | Domanda | Cosa cambierebbe |
|---|---|---|
| `FAC-11` | I **sei lati** devono diventare la primitiva, con gli archi **derivati** per abilità? | La fonte dichiara che un arco è una raccolta di lati scelta dalla singola abilità (`{FrontLeft, Front, FrontRight}`), mai una primitiva globale. Il canone dice l'opposto due volte: ADR-0005 §4 e [ADR-0008](decisions/adr-0008-rotazione-e-policy-di-facing.md) §5 — *«l'arco frontale resta `HexCone` e resta **uno solo** per difesa, percezione e reazioni»*. ⚠️ **Non è vocabolario**: `IsInFrontalArc` costruisce un cono **profondo quanto la distanza dell'attaccante**, e per un attaccante lontano un cono e un insieme di tre lati **non coincidono**. Cambiarlo tocca `Guard`, la copertura e l'Overwatch insieme, e riapre `FAC-3` da un'altra parte. Oggi prevale l'ADR |
| `FAC-12` | Il pivot **si paga** in punti movimento, o resta solo un **tetto**? | ADR-0008 §1 misura la rotazione in step e la tratta come un tetto (`MoveEndPivotMaxSteps`): ruotare fin dove è consentito è **gratis**. La fonte §10 propone un prezzo — `Move 2 celle + Pivot 60° = 3 MP` — che mette *quanto mi muovo* contro *quanto ruoto*, e farebbe pagare 3 MP anche a chi ruota da fermo (oggi libero e universale). Sono due assi di scelta diversi, non due formulazioni. Da guardare alla **prima revisione dei numeri di ADR-0008**, cioè alla chiusura di CP 16.2 |
| `FAC-13` | Da dove «arriva» un colpo che **non ha una sorgente puntuale**? | Oggi la direzione d'impatto è implicitamente la cella dell'attaccante (`IsInFrontalArc(…, OriginCell)`), e non c'è risposta per proiettile con traiettoria, esplosione con centro d'area o terreno che brucia — `grep` di `ImpactDirection`/`FromTrajectory` in `Source/` dà **zero**. La fonte propone una policy esplicita (`FromSource`, `FromTrajectory`, `FromImpactCenter`, `NonDirectional`). **Non è un difetto attivo**: il danno ambientale non passa da `Plan.Hits` e un'area azzera già la copertura per costruzione. Diventa un caso da correggere quando **E8/E9** daranno una direzione agli effetti d'area |
| `FAC-14` | La **rotazione forzata** è un effetto di controllo a catalogo? | `ERTActionEffect` ha `Damage · Heal · Shield · Push · Pull · Status · DamageReduction · DamageStructure`: **nessuna rotazione**. Girare un avversario è geometricamente equivalente a spostarlo — apre un lato — e `Push`/`Pull` esistono già. ⚠️ Si tiene con `FAC-3` e `FAC-11`: se una difesa diventasse direzionale, la rotazione forzata sarebbe **l'unico modo di aggirarla senza spostare nessuno**, quindi decidere l'una senza l'altra lascia il modello sbilanciato in un verso o nell'altro |

> **Nota di metodo.** Filtrare le quindici domande contro il canone ne ha chiuse cinque senza discussione, e
> ne ha riqualificata una sesta: «quali valori di rotazione per ogni eroe» non è un valore mancante, è la
> domanda `FAC-1` travestita da tabella da compilare. Vale la stessa lezione di `OD-1`/`OD-4`: un elenco di
> domande aperte redatto senza verificare il repository misura ciò che l'estensore non sapeva, non ciò che il
> progetto non ha deciso.
>
> ⚠️ **La stessa misura, una seconda volta e su scala maggiore.** Il pacchetto del 2026-08-10 porta **69
> sezioni**: 38 erano già canoniche, 6 già aperte, 3 rovesciavano una decisione presa, 18 erano procedura. Le
> domande nuove sono **quattro**, cioè il **6%**. Il rapporto non è un difetto della fonte — è la firma di un
> handoff scritto senza il repository davanti — ma dice qual è il lavoro: **leggere fino in fondo e filtrare**,
> non applicare. Applicato alla lettera, quel pacchetto avrebbe creato un'epic duplicata di una **chiusa**,
> quattordici issue per codice che esiste, e sedici feature al posto di una che ha già nove gate misurati.
>
> Le **dieci** voci erano il residuo dopo la verifica — sette dall'elenco §38, tre (`FAC-1`…`FAC-3`)
> promosse da proposte che l'handoff dava per acquisite.
>
> **Aggiornamento 2026-08-10.** Quattro sono state decise ([ADR-0008](decisions/adr-0008-rotazione-e-policy-di-facing.md)):
> `FAC-1`, `FAC-2`, `FAC-4` e `FAC-10`. **Ne restano sei** — `FAC-3` e `FAC-5`…`FAC-9`. Con `FAC-4` chiusa non
> c'è più una voce che blocchi lavoro costruibile: `CP 14.2`, `CP 14.4` e `CP 14.7` hanno la definizione che
> aspettavano. Le sei rimaste possono attendere, ma la scadenza dichiarata non cambia — **non oltre l'apertura
> di E16 in codice**, perché a quel punto il codice risponderà per conto suo.

---

## Aperte — interazioni con la mappa, dal consolidamento del 2026-08-09

Origine: [`gameplay/spec-interazioni-mappa-cp101.md`](gameplay/spec-interazioni-mappa-cp101.md) §12, owner della
grammatica delle interazioni ambientali (**CP 10.1**). Nessuna è decidibile dai documenti, e vanno chiuse
prima che E10 le incontri in codice.

| ID | Domanda | Perché serve una risposta |
|---|---|---|
| `INT-1` | Quali **capability di interazione** porta ciascun eroe della v0.1? | È un **asse di bilanciamento**, non un dettaglio di contenuto: se un solo eroe porta `Interaction.Force` e la mappa mette una porta rinforzata sull'unica rotta buona, quell'eroe non è una scelta — è una tassa. Le assegnazioni discusse (Flux → `Electric`/`Tech`, Riva → `Fluid`, Bastion → `Engineering`/`Force`, Vektor → `Precision`/`Sensor`) sono **coerenti** con le identità canoniche, e proprio per questo è facile scambiarle per decise: non compaiono né nel [catalogo eroi](balance/RT_HeroCatalog_v0.1.md) né nel Decision Log |
| `INT-2` | Un **verbo** può risolvere in una fase diversa dal Blast? | Il pacchetto propone `OpenDoor → Prep`. Una porta aperta in Prep è attraversabile **col Dash**; una aperta nel Blast solo col Move. Sono due economie del turno diverse — la seconda fa costare un turno intero al piano di sfondamento, la prima no. Va decisa con un dato in mano, non per analogia |
| `INT-4` | Il **costo** di un `Interact` dipende dal verbo? | Se sì, il verbo entra nell'action economy e smette di essere solo un payload dell'elemento (§6 della spec). Nessun numero va inventato prima della risposta |

> **`INT-3` non esiste come voce separata.** La domanda «`Interact` **richiede** un facing verso l'elemento,
> oppure lo **impone**?» era già registrata come **`FAC-6`** dal consolidamento del 2026-08-08, e resta lì:
> è una domanda sull'**orientamento**, e il suo owner è [ADR-0005](decisions/adr-0005-orientamento.md), non la
> grammatica delle interazioni. La spec di CP 10.1 ne è però il **primo consumatore concreto** — è lì che
> «girarsi verso la porta» smette di essere un caso ipotetico.
>
> Registrarla due volte sarebbe costato più della duplicazione: due ID per la stessa domanda si chiudono in
> momenti diversi, e il secondo resta aperto a mentire.

---

<<<<<<< HEAD
## Aperte — profili d'eroe di `Brace` e `Overwatch`, dal consolidamento del 2026-08-10

Origine: [`roadmap/plans/baseaction-signatures-spec-panel-2026-08-10.md`](roadmap/plans/baseaction-signatures-spec-panel-2026-08-10.md),
triage del sorgente omonimo. Il **meccanismo** è deciso da entrambi i lati — [D-047](decisions/RT_PDR_00_Decision_Log.md)
per `Brace`, [D-012](decisions/RT_PDR_00_Decision_Log.md)/[D-014](decisions/RT_PDR_00_Decision_Log.md) e
**CP 14.4** per `Overwatch`, con il profilo dichiarato «dato per eroe, non ramo nel resolver». Quello che manca
è il **contenuto**: quali siano i quattro profili. Vanno decise prima che E14 li incontri in codice.

| ID | Domanda | Perché serve una risposta |
|---|---|---|
| `BAS-1` | I quattro profili `Brace` — Flux `Grounding`, Riva `Flow`, Bastion `Anchor`, Vektor `Deflection` — entrano nel canone come contenuto di **CP 14.7**? | D-047 stabilisce che un profilo con **≥ 2 risposte legali** apre la finestra: questi ne dichiarano due o tre, quindi non sono contenuto neutro — decidono **quante finestre si aprono** in una resolution, che è il rischio di pacing dichiarato di E14 |
| `BAS-2` | I quattro profili `Overwatch` — `Conductive`, `Pressure`, `Frontline`, `Predictive` — entrano come contenuto di **CP 14.4**? | Le geometrie proposte (settore medio · medio-corto · corto-largo frontale · stretto-lungo) sono esprimibili con `FRTSuppressiveZone` e col facing come impone CP 14.4, ma **nessun catalogo le contiene**. Senza, `Overwatch.ProfileIsDataNotBranch` non ha un secondo profilo da confrontare col primo |
| `BAS-3` | `Vektor.Deflection`: un nome, **due semantiche**. Si rinomina il profilo `Brace`, si rinomina la reazione, o si unificano? | `Vektor.Deflection` esiste già cablata (`RTHeroCatalogLibrary.cpp:557`, CP 6.7) ed è **−20 sul colpo diretto**, cioè riduzione danno — esattamente ciò che il §10 del sorgente vieta al `Brace`. Due entità con lo stesso nome e semantiche opposte non si separano da sole al momento dell'implementazione |
| `BAS-4` | `Riva.Flow`: il profilo `Brace` è la **promozione** di `Riva.FlowReaction`, o una terza cosa? | «Flow» per Riva è già scritto **due volte, in due accezioni**: `Riva.FlowReaction` (`RTHeroCatalogLibrary.cpp:351`) è `Reposition 1` **dopo un attacco subito**, rinviata a E14 con slot `None`; `State.Riva.Flow` è un candidato *stance* post-v0.1 (`RT-FEAT-CHARACTER-STATE`, E34, `PROPOSED`). Il profilo proposto risponde invece al **Forced Movement**. «Riva colpita ma non spostata» distingue il primo caso dal terzo, e oggi nessun documento dice quale dei due accade |
| ~~`BAS-5`~~ | ~~Dopo l'`Overwatch`: **Move con budget ridotto**, o **Watch Stage + Reposition** pianificato?~~ **Chiusa come domanda il 2026-08-10**: prevale il modello **Watch → EndWatchStage → Reposition**. Non per la data — è la stessa — ma perché il sorgente gemello si dichiara superato su questo punto (§34 e §48 elencano «vecchio post-Overwatch Normal/Sneak Move» fra ciò da correggere), e perché è l'unico dei due modelli che dice **dove si trova** il personaggio quando l'Overwatch finisce. Restano aperti il suo **costo** (`OW-1`) e il suo **nome** (`OW-2`): vedi [`roadmap/plans/overwatch-runtime-lifecycle-triage-2026-08-10.md`](roadmap/plans/overwatch-runtime-lifecycle-triage-2026-08-10.md) |

> **Le affinità di interazione per eroe non aprono una voce nuova.** Il §17 del sorgente (Flux → generatori e
> pannelli, Riva → valvole e pompe, Bastion → cover, porte e barricate, Vektor → standard) **ricalca** le
> assegnazioni già registrate in **`INT-1`**. Non è una domanda nuova: è una **proposta di risposta** a una
> già aperta, e va valutata lì. Vale la stessa regola che tiene fuori `INT-3`.
>
> Il §17 la fonda però su `Activate` come azione distinta da `Interact` — che [D-014](decisions/RT_PDR_00_Decision_Log.md)
> e [D-025](decisions/RT_PDR_00_Decision_Log.md) hanno già escluso. Letta come affinità di `Interact`, la
> proposta regge senza modifiche di sostanza.
=======
## Aperte — geometria, clearance e confine Guard/Brace, dal consolidamento del 2026-08-10

Origine: [conflict report dell'handoff geometria](roadmap/plans/handoff-geometry-reazioni-conflict-report-2026-08-10.md).
Delle **17** domande che il sorgente elencava, **sette non erano aperte** — tre già decise, due già tracciate
come `CLASH-1`/`CLASH-2`, due con risposta in un altro sorgente non ancora consumato. Queste cinque sono il
residuo.

| ID | Domanda | Perché serve una risposta |
|---|---|---|
| `MAP-2` | Che succede quando la **linea di tiro sfiora l'angolo** di un muro? | È il caso che la geometria arbitraria crea e la griglia allineata non aveva: con muri a 90° che tagliano le celle, `HasLineOfSight` incontra tangenze che oggi non esistono. La risposta decide anche i corner case di proiettile e copertura, che sono lo stesso problema visto dal lato del danno. Va decisa **con una fixture in mano**, non per principio |
| `MAP-3` | La **cottura non è invertibile**: cosa succede se qualcuno modifica a mano il dato cotto? | Registrato come rischio il 2026-08-09 e ancora aperto. Se si edita `bBlocksMovement` su una cella cotta, il prossimo ricalcolo cancella la modifica **in silenzio** — stessa classe di problema dei prefab. Le uscite sono tre (vietare l'edit, marcare la cella come «sganciata», o rinunciare al ricalcolo automatico) e nessuna è deducibile dai documenti |
| `BAL-1` | `Guard` e `Brace` devono separarsi in **danno contro spinta**? | [D-066](decisions/RT_PDR_00_Decision_Log.md) ha misurato il modello in vigore: entrambi fanno entrambe le cose, e differiscono per *forma* (primo colpo forte vs ogni colpo; spinta di 1 cella vs spinta qualsiasi). È bilanciamento: si chiude con una partita, non con un documento. ✅ **La Fase 0 è decisa** ([D-074](decisions/RT_PDR_00_Decision_Log.md), 2026-08-10, issue [#400](https://github.com/DegrassiAaron/refactor-tactics-main/issues/400)): si accetta che in v0.1 ogni spinta valga 1 e si **riscrive** la clausola «senza limite di distanza» invece di introdurre una spinta `≥ 2`. Conseguenza sulle opzioni ancora in campo: restano lo **status quo** e l'**ibrido** (separare le magnitudini); l'opzione *«`Guard` solo danno, `Brace` solo spostamento»* è **preclusa**, perché senza spinta forte lascerebbe `Brace` senza mestiere. ✅ Gli scenari che servono a decidere esistono e sono verdi ([#401](https://github.com/DegrassiAaron/refactor-tactics-main/issues/401)): `Spec.Brace.GuardAndBraceOnMixedHit` e `Spec.Brace.BraceWinsOnSecondHit` pinnano il trade-off reale — *primo colpo pesante* (`Guard` 1 danno) contro *colpi ripetuti* (`Brace` 12 contro 17 su due colpi). ⏳ **Resta l'unica parte che richiede l'autore**: la seduta editor **U20** (voce `PIE-BAL1`) e la scelta fra le due opzioni superstiti. Roadmap e numeri: [`bal-1-guard-brace-roadmap-2026-08-10.md`](roadmap/plans/bal-1-guard-brace-roadmap-2026-08-10.md). Issue [#403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/403) (decisione) |
| `ECO-1` | `Guard` e `Brace` competono con il **Main Commitment**, o hanno un'altra economia? | [D-012](decisions/RT_PDR_00_Decision_Log.md) copre `Attack \| Ability \| Overwatch` e **non** dice nulla di `Guard` e `Brace`, che a catalogo occupano l'azione principale ma non compaiono in quella regola. La domanda si porta dietro la matrice Sprint/Sneak proposta dal sorgente (`Brace` e `Overwatch` senza Sprint), che **non è canonica** e non va resa tale senza playtest |
>>>>>>> origin/main

---

## Il ciclo Watch/Withdraw — tre voci chiuse il 2026-08-10, una aperta

Origine: [`roadmap/plans/overwatch-runtime-lifecycle-triage-2026-08-10.md`](roadmap/plans/overwatch-runtime-lifecycle-triage-2026-08-10.md).
Il **modello** era già deciso da `BAS-5` sopra — `Watch → EndWatchStage → Withdraw` — e gran parte del sorgente
era già canone-compatibile: la cadence *once-per-target* ha persino uno scenario che la esprime da prima
(`Spec.Overwatch.HoldThenFire`, dove Vektor fa `HOLD` su Flux e `FIRE` su Riva). Restavano aperti **quanto
costa** e **come si chiama**: entrambi decisi dall'autore il 2026-08-10 in [D-070](decisions/RT_PDR_00_Decision_Log.md).

### ✅ Chiuse il 2026-08-10 — [D-070](decisions/RT_PDR_00_Decision_Log.md), decise dall'autore in sessione

Tre voci su quattro. Restano qui, barrate, perché il registro deve dire **come** è andata a finire.

| ID | Esito | Dove vive ora |
|---|---|---|
| ~~`OW-1`~~ | **Sì, costa anche il movimento** — ma non con `ERTActionSlot::MovementAndMain`: l'Overwatch occupa lo slot **principale** e **riserva** quello di movimento al solo `Withdraw`, dichiarato in Planning. Il divieto di `Dash` diventa una **conseguenza** dello slot impegnato, non una regola da scrivere e testare | [D-070](decisions/RT_PDR_00_Decision_Log.md) |
| ~~`OW-2`~~ | **`Withdraw`**. Scartati `Fallback` (collide con `ERTActionFallback`), `Disengage` (evoca attacchi di opportunità inesistenti), `Retreat` (implica una direzione). `Action.Reposition` resta dov'è: nessuna rinumerazione di catalogo, codice, kit e test | idem |
| ~~`OW-3`~~ | **2 MP**, ancorati ad `Action.Reposition` — l'unica altra mobilità breve del catalogo — invece di un numero scelto a intuito. Owner: [`balance/RT_ActionCatalog_v0.1.md`](balance/RT_ActionCatalog_v0.1.md) §2.1 | idem |

> **L'opzione scartata su `OW-1` merita di essere ricordata**, perché il motivo non è estetico. `Action.Sprint`
> sembrava il precedente perfetto — è l'unica azione che consuma movimento **e** principale — ma
> [D-028](decisions/RT_PDR_00_Decision_Log.md) glielo sta **togliendo**, e il catalogo lo dichiara già
> «occupa il **solo slot movimento**» con ⚠️ sul codice non ancora allineato. Adottare `MovementAndMain` per
> l'Overwatch l'avrebbe resa l'unica utente di uno slot che il canone sta svuotando, riaprendo di fatto una
> decisione chiusa. La formulazione scelta ottiene lo stesso effetto di gioco — niente Dash, movimento corto e
> tardivo — **senza** che un'azione si prenda due slot.

### Ancora aperta

| ID | Domanda | Perché serve una risposta |
|---|---|---|
| `OW-4` | Gli objective che dipendono dalla **posizione finale** si valutano dopo lo Stage B? | Il sorgente (§17) chiede di verificarlo contro il canone, e il canone **non dice nulla**: è una lacuna, non un conflitto. Con due stage di movimento nella stessa fase, «posizione finale» smette di essere ovvia. ⏳ **Non è di E14 e non è urgente**: misurato il 2026-08-10, gli objective oggi sono **solo** un motivo di fine partita (`ERTMatchEndReason::Objective`) e il punto d'ingresso per il progresso è dichiarato per **CP 10.2**. Nessun objective di posizione esiste, quindi la domanda non ha ancora un consumatore |

> **Quattro punti del sorgente NON aprono una voce: sono già compatibili** e possono entrare nel DoD di E14
> senza altre decisioni — la cadence `OncePerTargetPerReactionInstance` (§21), `MaxPrompts` che conta
> opportunity distinte e non passi (§24, precisa [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) §8),
> l'eligibility valutata **post-transition** (§20), e la distinzione fra **hard cancel** e **soft eligibility
> block** (§9) — che CP 14.6 oggi non fa, e che serve perché `NoLOS` e `Stun` non possono produrre lo stesso
> reason code terminale.

---

## Aperte — framework degli status, dal quarto sorgente del 2026-08-10

Origine: [triage dell'handoff Status/Control](roadmap/plans/handoff-status-control-triage-2026-08-10.md).
Il repository ha **undici** status implementati e **nessun framework** che li governi. Queste tre vanno
decise **prima** di scrivere la spec owner, perché ognuna cambia la forma del dato, non un valore.

| ID | Domanda | Perché serve una risposta |
|---|---|---|
| `STA-3` | `Suppressed` e `Dazed` entrano nella **v0.1** o restano nel framework? | Il sorgente li mette nel set ridotto del vertical slice, ma **nessuno dei quattro kit li produce oggi**: sarebbero due status senza consumatore, il difetto ricorrente di questo repository. Entrano quando un'abilità li applica, non prima |
| `STA-4` | Qual è la **tassonomia delle capability** — l'elenco di cosa un effetto può togliere, e con quale granularità? | Aperta **da** [`D-072`](decisions/RT_PDR_00_Decision_Log.md), ed è il prerequisito di tutto il resto: senza, non si deriva né la primitiva (`STA-1`) né la severity (`STA-2`), che sono le due cose appena decise. La granularità è la parte non ovvia — «movimento» è una capability sola o si divide nelle azioni e nei profili reali (`Move` · `Sneak` · `Sprint` · `Dash` · `Leap` · `Charge` · `Reposition`)? Da questa scelta dipende se `Suppressed` conta come `C1` o `C2`, cioè **il gate anti-stun-lock cambia di significato**. Va decisa guardando le azioni che esistono, non a tavolino. ⚠️ **Precisata dallo spec panel del 2026-08-10** su [#436](https://github.com/DegrassiAaron/refactor-tactics-main/issues/436), che ha corretto tre cose in questa riga. **(1)** La formulazione originale citava `Climb`, che **non esiste come azione**: cambiare layer è `Move` attraverso un arco di transizione (`RefactorTactics.HexMove.ClimbsOnlyThroughTransition`), e ometteva `Sneak`, che esiste come profilo ma senza numeri. **(2)** La tassonomia grossolana **non è da inventare**: `ERTActionSlot` (`None` · `Movement` · `Main` · `MovementAndMain` · `Reaction`) e `ERTMovementStyle` girano già, e i tre nomi della regola anti-stun-lock **sono** tre suoi valori — quindi la domanda vera è se `ERTActionSlot` basti, vada raffinato o affiancato. Aggiungere un `capability:` accanto a `Slot` sarebbe la **seconda verità** che `D-072` ha appena respinto. **(3)** Serve un **secondo asse**: `Exposed`, `Obscured` e `Marked` non tolgono un'azione — tolgono uno step di copertura, cambiano l'eleggibilità di targeting, abilitano un payoff altrui. Tre status su undici non si esprimono su uno slot. Da decidere anche se la tassonomia eredita `MovementAndMain`, che oggi **non ha produttori** (`D-028` gli ha tolto `Action.Sprint`, `D-070` ha rifiutato di adottarlo) ma è la definizione operativa di un `C3` |

---

## ✅ Chiusa il 2026-08-09 — `MED-1`, mal posta

Aperta e chiusa lo stesso giorno. Resta qui **solo come indice**: il contenuto vive in
[`D-059`](decisions/RT_PDR_00_Decision_Log.md).

| Era | Perché non era una domanda | Dove vive ora |
|---|---|---|
| `MED-1` — «`Fire` e `Smoke` restano superfici o diventano stati temporanei?» | Presupponeva che `Fire`(cella) e `Burning`(unità) fossero lo **stesso asse**. Non lo sono: sono una coppia **produttore/consumatore** — la superficie applica lo status a chi entra. E la separazione base/transitorio che la domanda chiedeva di introdurre **esiste già**, su un terzo strato: `ARTTurnManager::DynamicSurfaces{Original, TurnsRemaining}` | [`D-059`](decisions/RT_PDR_00_Decision_Log.md) |

> **Nota di metodo, la quarta della serie.** Come `OD-1`, `OD-4` e `PER-3`, la domanda era stata redatta
> confrontando due **documenti** — l'elenco delle otto superfici di CP 8.1 e quello degli stati temporanei di
> CP 8.2 — senza guardare **dove ciascun valore è memorizzato**. Il codice risponde in tre righe di header
> (`RTHexCellData.h`, `RTTurnManager.h:246`, `RTUnit`), e per giunta argomenta *contro* la correzione che la
> domanda implicava: un campo `BaseSurface` nella cella sarebbe «un secondo modello di verità», e la scadenza
> è stato **di partita**, non dato di mappa.
>
> La lezione non è «verificare meglio»: è che un elenco di domande aperte redatto sui documenti misura ciò che
> l'estensore non sapeva, non ciò che il progetto non ha deciso. Vale anche quando l'estensore sono io.

## Aperte — livello prodotto

Owner: [`product/piano-canonico-mvp.md`](product/piano-canonico-mvp.md) §9. Non bloccano l'MVP tecnico.

| Tema | Stato |
|---|---|
| Budget e modello commerciale | non specificati in nessun documento |
| Composizione del team | assunzioni discordanti; il progetto assume **dev singolo** |
| Direzione artistica | inesistente; si usano placeholder e asset Paragon |
| Hardware target | mai definito → i budget KPI restano **da misurare**, non garanzie |
| Identità originale (nomi, lore) | necessaria per una pubblicazione |
| ~~Mapping visuale Paragon → roster~~ | **chiusa il 2026-08-08** da [`D-037`](decisions/RT_PDR_00_Decision_Log.md): Flux → `Paragon.Gadget`, Riva → `Paragon.Phase`, Bastion → `Paragon.Riktor`, Vektor → `Paragon.Wraith`. Tabella owner in [`characters/paragon.md`](characters/paragon.md). Resta aperto solo il **nome retail** dei quattro slot v0.2, che è la riga «Identità originale» qui sopra |

## ✅ Chiuse il 2026-08-09 — sessione `/sc:brainstorm` su E13

Le tre voci `PER-1`…`PER-3`, aperte dalla spec panel dello stesso giorno, sono state decise dall'autore.
Restano qui **solo come indice**: il contenuto vive nel [Decision Log](decisions/RT_PDR_00_Decision_Log.md).

| Era | Decisione presa | Dove vive ora |
|---|---|---|
| `PER-1` | La soglia d'udito e' una statistica **per eroe** e **compensa** la vista: Flux 5 · Riva 3 · Bastion 3 · Vektor 5 | [`D-041`](decisions/RT_PDR_00_Decision_Log.md) |
| `PER-2` | Acqua bassa **`+2`**, non `+3`: ostacolo tattico, non allarme | [`D-042`](decisions/RT_PDR_00_Decision_Log.md) |
| `PER-3` | Non sai di essere nell'arco di un avversario; `TeamKnowledge` resta un **insieme per squadra** | [`D-043`](decisions/RT_PDR_00_Decision_Log.md) |

**Nota di metodo, di nuovo.** Come per `OD-1`/`OD-4` nel 2026-08-07, una delle tre era **mal posta**: `PER-3`
chiedeva «se A vede B, B vede A?», che presuppone una relazione fra **unita'** — mentre il DoD dice
`TeamKnowledge`, cioe' conoscenza **di squadra**. Riformulata, non aveva bisogno di una decisione di modello:
l'asimmetria c'e' gia' per costruzione. Restava una domanda vera, ma di **presentazione** — «il giocatore
vede di essere osservato?» — ed e' quella che e' stata decisa.

E meta' di `PER-2` si e' sciolta guardando il codice invece dei documenti: la divergenza di segno riguardava
la **vegetazione**, che non e' fra le otto superfici della v0.1. Una domanda senza terreno su cui atterrare.

## Aperte — livello regole

| Tema | Stato |
|---|---|
| ~~**Che ne è del Move pianificato se l'unità viene spostata prima della fase Move**~~ | ✅ **Chiusa il 2026-08-10 da [D-045](decisions/RT_PDR_00_Decision_Log.md)**: `Model A` — se l'origine effettiva differisce da quella pianificata, **il Move decade**. `B` (ricalcolo verso la stessa destinazione) resta **escluso** perché contraddice *«mai auto-reroute»*; `C` (riesecuzione delle direzioni dalla nuova origine) è l'alternativa da provare **dopo**. **Baseline rivedibile**, con criterio di uscita quantificato: Move annullato più di **una volta ogni due round** → si prova `C`. Owner: [`gameplay/spec-tassonomia-movimento.md`](gameplay/spec-tassonomia-movimento.md) §5 |
| **Con quali valori si tara il Decision Time Bank?** | ⏳ **aperta il 2026-08-09**. *Non* è più aperto **se** costruirlo: entra in **v0.1** come **CP 14.8**, senza gate — owner [`gameplay/spec-decision-time-bank.md`](gameplay/spec-decision-time-bank.md), audit di provenienza [`roadmap/plans/decision-time-bank-conflict-report-2026-08-09.md`](roadmap/plans/decision-time-bank-conflict-report-2026-08-09.md). Il bank è un cap aggregato per un costo che [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) §8 aveva deciso di **misurare prima di contenere** (`D20`, nessun cap): quel rischio è ora **assunto in senso opposto e dichiarato** (spec §2.1), e i due rientri di ADR-0004 §Revisione — *cap aggregato condiviso* e `MaxPromptsPerReaction = 1` — restano validi e compatibili. Resta aperta la **taratura**: `InitialBank` è derivato (`RoundLimit × (MaxWindow − Grace)` → 24 s in 2v2), `Grace` 1,0 s ed `ExhaustedGrace` 0,75 s sono `PROPOSED FOR PLAYTEST` con i criteri di uscita di §3.2. Prima misura utile **CP 14.6**, che CP 14.8 non precede. Metrica che decide: `ReactionDecisionSeconds`, separata da `ResolutionPlaybackSeconds`. Restano aperte anche `TB-5` e `TB-7` (policy di rete, M10): vivono nella spec §17, **non si duplicano qui** |

## Assunzioni da bloccare

Voci del Decision Log che **aspettano una misura**, non una discussione. Si consolidano da sole quando il dato
arriva; finché non arriva, nessun documento deve trattarle come vincolanti.

| ID | Assunzione | Cosa la consolida |
|---|---|---|
| [`D-001`](decisions/RT_PDR_00_Decision_Log.md) | Formato principale **3v3** | La prima misura reale su una partita ≥3v3. Oggi non esiste: la v0.1 misura la banda **2v2** |

`D-007` (baseline motore) **non è più qui**: chiusa il 2026-08-08 come
[`D-022`](decisions/RT_PDR_00_Decision_Log.md) — **UE 5.8.1**, upgrade solo fra milestone e con migrazione
esplicita. Era il caso opposto a `D-001`: non mancava la misura, mancava solo la formalizzazione di un vincolo
che il repository applicava già.

## Da playtestare — non decisioni, tarature

Numeri già in vigore che nessuno ha ancora misurato sul campo. Vivono nei documenti che li possiedono; sono
elencati qui perché **sembrano decisi e non lo sono**.

| Parametro | Valore in vigore | Owner |
|---|---|---|
| `FastReactionDuration` = **3,0 s** | baseline | [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) §8 |
| `MaxPromptsPerReaction` = **3**, nessun cap aggregato | scelta esplicita, rischio dichiarato | [`gameplay/brief-overwatch-reazioni.md`](gameplay/brief-overwatch-reazioni.md) §5 |
| Effetto esatto di `Brace`, numeri `Sneak/Move/Sprint` | non definiti | [`gameplay/brief-azioni-generiche-overwatch.md`](gameplay/brief-azioni-generiche-overwatch.md) |
| `RoundLimit` 2v2 **10–14**, 3v3 **16–20** | bande, non costanti | [D-010](decisions/RT_PDR_00_Decision_Log.md) · [`gameplay/spec-durata-partita-e-scala-mappe.md`](gameplay/spec-durata-partita-e-scala-mappe.md) |
| Durata della resolution con 1/2/3 unità armate | **mai misurata**; soglia d'allarme 20 s | CP 14.5 |
| Grammatica `STAND · READ · SHIFT` del Reaction Clash | **`PROPOSED FOR PLAYTEST`**, non canonica ([D-049](decisions/RT_PDR_00_Decision_Log.md)) | [`gameplay/spec-reaction-clash-e14.md`](gameplay/spec-reaction-clash-e14.md) §4 |
| Payoff `Win/Tie/Lose` delle maneuver dei 4 eroi | **inesistenti**: i nomi nella spec sono fixture, nessun dato d'eroe li contiene | idem §5 |
| Costo di un Clash: `Charges` proprio o quello della reaction | non deciso | idem §14 (`CLASH-3`) |

> **Il Reaction Clash incontra due domande già aperte, e non le risolve.** `FAC-3` (*`Brace` diventa
> direzionale?*) e `FAC-5` (*una reazione può ruotare chi reagisce?*) tornano rilevanti perché maneuver come
> `Pivot Step` e `Sidestep` presuppongono la risposta «sì». Restano `PROPOSED` finché quelle due non sono
> decise — vedi la §6 della spec, che lo dichiara invece di risolverle di lato.
