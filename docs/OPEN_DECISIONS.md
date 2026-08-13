# Decisioni aperte

> `OPEN` · **Stato**: vivo · **Ultimo aggiornamento**: 2026-08-13
> **Cosa è**: l'elenco di ciò che **aspetta una persona**. Nessuna di queste voci può essere chiusa
> deducendola dai documenti: o mancano i dati, o due fonti si contraddicono senza gerarchia.
> **Cosa non è**: il registro delle decisioni prese — quello è il
> [Decision Log](decisions/RT_PDR_00_Decision_Log.md), che resta l'**owner**. Quando una voce qui si chiude,
> diventa una `D-0xx` lì e **qui resta barrata**, con l'esito e l'istruttoria che l'ha prodotta.
>
> 🔴 **Corretto il 2026-08-12**: questa riga diceva *«e sparisce da qui»*, e **non è mai stato vero**. Sette
> sezioni di questo file conservano le voci chiuse — archivio replay, `GEO-*`, `MAP-1`/`STA-*`, `OD-*`,
> `FAC-*`, `MED-1`, i radar — perché una domanda barrata col suo perché **è** il valore: il prossimo kit la
> riproporrà, e trovarla già risposta costa meno che ridiscuterla. La regola scritta contraddiceva la
> pratica di ogni sezione, e fra le due si è corretta la regola.

---

## Aperta — la versione di formato che non viaggia, e la mappa che dipende da lei

Origine: [`#687`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/687) e
[`roadmap/plans/mappe-generate-o-dipinte-2026-08-12.md`](roadmap/plans/mappe-generate-o-dipinte-2026-08-12.md) §6.

Il [referto del 2026-08-12](roadmap/plans/roadmap-reconciliation-2026-08-12.md) §5 aveva **rinviato**
questa decisione, e con una ragione precisa: *«finché il meccanismo non è verificato su asset serializzato
con binario vecchio/nuovo — ed è ciò che fa la PR #688, aperta»*. **La PR #688 è stata mergiata il
2026-08-12T18:13:55Z.** La condizione che teneva sospesa la decisione non esiste più: il rinvio era
motivato, la motivazione è scaduta.

Cosa ha portato quella PR: **la verifica, non la correzione**. Ha aggiunto
`RefactorTactics.HexMap.SerializedAssetMigratesWithoutGainingData`
(`Source/RefactorTactics/Tests/RTHexMapTests.cpp:671`) e il commento a `:664` scrive l'esito —
*«la migrazione e' inerte: `FormatVersion` non e' nei byte serializzati»*. Il difetto è ora **dimostrato
sul binario**, e nessuna delle quattro direzioni di `#687` è stata scelta.

⚠️ **Le due domande stanno insieme perché il piano lo dice**, non per comodità di redazione: la §6 di
`mappe-generate-o-dipinte` conclude che *«le due cose vanno decise insieme, o si costruisce un secondo
controllo sopra un meccanismo inerte»*. Un test che lega asset e generatore via `ComputeHash` funziona
comunque — `ComputeHash` non passa dalla serializzazione delta — ma direbbe «l'asset corrisponde al codice»
senza poter dire **con quale versione di formato** è stato scritto.

| ID | Domanda | Perché non si deduce |
|---|---|---|
| `FMT-1` | Quale delle quattro direzioni di `#687` si prende: `SaveGame`/serializzazione esplicita su `FormatVersion`, `Serialize()` custom fuori dal delta, **custom object version** di UE (`FCustomVersionRegistry`), oppure **accettare** che le migrazioni restino dichiarative e ritirare la promessa scritta in `MigrateToCurrentFormat`? | `#687` chiude dicendo *«Non ne propongo una: la scelta ha implicazioni sul formato e va fatta da chi lo possiede»*, ed è la posizione giusta — le quattro direzioni non differiscono per costo ma per **cosa promettono al prossimo lettore**. Oggi il difetto è **innocuo**, e questo è il problema: tutte le migrazioni fatte finora sono dichiarative, quindi non partire non ha conseguenze e il meccanismo *sembra* sano. Il giorno in cui una migrazione dovrà **trasformare** dati, l'asset arriverà al gioco con dati interpretati secondo regole nuove, e **nessun test lo prenderebbe**: tutti i test di migrazione fanno `NewObject` e impostano `FormatVersion` a mano (`RTHexMapTests.cpp:310`), cosa che nella realtà non accade mai. ⚠️ Lo stesso meccanismo morde `MapClass` e `HexSize`: un'arena salvata come `Skirmish` — il default — diventerebbe **silenziosamente** un'altra classe il giorno in cui il default cambiasse, e la classe serve al validator per rifiutare l'accoppiata sbagliata prima dell'allestimento. La quarta direzione è legittima ma **va scritta**, o il prossimo che apre quel file crederà a una rete che non c'è. Innesco: **scaduto** — la verifica su asset serializzato esiste dal 2026-08-12 |
| `FMT-2` | Se le mappe si **generano** invece di dipingersi, quante se ne committano: **una** che faccia da rivelatore per la serializzazione, o le **nove** che l'allowlist ammette oggi? | Non si deduce perché la risposta dipende da `FMT-1`. L'asset committato ha **una sola funzione che il codice non può svolgere**: essere un binario scritto *ieri* da rileggere *oggi*. Se `FormatVersion` viaggia, un rivelatore basta e le altre otto sono codice; se non viaggia, nemmeno quello rivela nulla e committarne nove è **nove volte lo stesso silenzio**. ⚠️ Il piano segnala anche un buco già presente e indipendente: `MakeArenaV01` non è nel registry `MakeFixtureArena`, quindi **nessuno scenario può riferirla per nome**. Innesco: la prima mappa generata che entri in `Content/` |

---

## Aperte — le due aree davvero senza owner delle proposte §10, dal secondo passaggio del 2026-08-13

Origine: [`archive/src/handoff/2026-08-12-roadmap-reconciliation.md`](archive/src/handoff/2026-08-12-roadmap-reconciliation.md) §10.
Il banner d'archivio di quel sorgente dichiara: *«❌ Non recepito: le cinque epic proposte in §10 […] il
posto di una proposta senza decisione è `docs/OPEN_DECISIONS.md`»*. **Non ci sono mai arrivate.** È la
stessa forma del difetto che quel referto stava correggendo: una prescrizione scritta, e nessuno che la
esegua.

🔴 **Ma cinque non è il numero giusto, e trascriverle tutte avrebbe canonizzato tre affermazioni false.**
Il sorgente §10 premette *«Le ricerche live non trovano un owner dedicato completo per queste aree»*.
Rimisurato contro i **105 `feature_id`** di `feature-registry.yaml`, per tre aree su cinque è **falso**:

| Area §10 | Il sorgente dice | Il registry dice | Esito |
|---|---|---|---|
| **A. Super Actions** | «Epic da creare post-v0.1» | `RT-FEAT-ACTION-SUPERS` — v0.2, P2, **`IMPLEMENTING`**, cinque gate `partial` | **owner esiste**, ed è già in lavorazione |
| **B. Modular Effects + Presentation/VFX** | pipeline da proporre | nessuna feature per il mapping *outcome logico → presentazione*. `RT-FEAT-CHAR-PRESENTATION` ha per soggetto mesh/animazioni/anelli dei personaggi, non gli effetti | **gap reale** → `FX-1` |
| **C. Seeded Map Generation** | prototipo v0.3 | nessuna feature di generazione. `RT-FEAT-TOOL-MAP-GEOMETRY` e `RT-FEAT-TOOL-MAP-EDITOR` sono **authoring**, non generazione da seed | **gap reale** → `GEN-1` |
| **D. Production Map Generator / Level Designer** | v0.5 da proporre | `RT-FEAT-TOOL-MAP-EDITOR` è **`INTEGRATED`** in v0.1 | **coperto per la parte di authoring**; il resto è il seguito di `GEN-1`, non un'area propria |
| **E. Networking / Dedicated server** | proposta v0.6 | **tre** feature: `RT-FEAT-NET-PRIVATE-PLANNING`, `RT-FEAT-NET-AUTHORITY`, `RT-FEAT-NET-DEDICATED` (`IDEA`, `future`) | **owner esiste**, perimetro già scritto |

La lezione è la stessa di `REP-1` e di `MSE-1`: una proposta si registra **dopo** aver cercato chi
possiede già l'area, non prima. Le tre righe «owner esiste» restano qui apposta — il prossimo kit le
riproporrà, e trovarle già risposte costa meno che ridiscuterle.

| ID | Domanda | Perché non si deduce |
|---|---|---|
| `FX-1` | Fra l'esito logico di un'azione e il suo effetto visivo esiste un **livello di mapping dichiarativo**, o la presentazione legge direttamente lo stato? Cioè: la catena proposta `Action Definition → Effect Specs → Logical Outcome Events → Presentation Mapping → Niagara/SFX/UI` ha un anello che oggi manca — quale? | I due estremi esistono già: `FRTActionEffectSpec` è reale e diffuso (**32 file** in `Source/`, da `RTActionData.h` a `RTActionEffectLibrary.cpp`), e `RT-FEAT-CHAR-PRESENTATION` è `IMPLEMENTING`. Quello che **nessun documento nomina** è l'anello centrale: un *outcome event* logico che la presentazione consuma senza poter influire. Non si deduce dal codice perché oggi la domanda non morde — la presentazione della v0.1 è ferma a mesh e anelli — ma l'invariante che protegge è già canone e vale la pena scriverla prima che esista un consumatore: **le animazioni e i VFX non decidono mai il risultato**. È lo stesso schema di `D-083`, perimetro deciso adesso e costruzione rinviata. Innesco: `#288` (locomotion/cast/hit/death), che è il primo consumatore reale |
| `GEN-1` | La relazione **numero di celle → `RoundLimit`** è una formula canonica o una taratura da misurare? E un generatore da seed deve **validare** connettività, macro-rotte, choke, raggiungibilità degli obiettivi, densità di copertura, alture, transizioni e equità degli spawn — o è il playtest a dirlo? | Il sorgente stesso marca la relazione come *«ipotesi da misurare, non formula canonica»*, ed è la parte che va conservata: è una **taratura**, e questo file ha una sezione apposta per le tarature. Non si deduce perché il generatore non esiste e non c'è nulla da misurare: `RT-FEAT-TOOL-MAP-GEOMETRY` e `RT-FEAT-TOOL-MAP-EDITOR` producono geometria **d'autore**, con un umano che decide. ⚠️ Il vincolo che sopravvive alla proposta è di **formato**, non di algoritmo, e va rispettato dal primo giorno: un generatore deve riusare i dati canonici del filone editor (`#619`–`#623`, `#695`) — **niente secondo formato di mappa**. È lo stesso vincolo che `MSE-1` sta perimetrando dal lato della cottura, ed è il motivo per cui `GEN-1` non può essere decisa senza guardarla. Innesco: v0.3, o la prima mappa non disegnata a mano |

---

## ✅ Chiuse — archivio replay, dal conflict report del 2026-08-10

Origine: [conflict report replay](roadmap/plans/replay-system-conflict-report-2026-08-10.md) §9.
Le tre domande erano già issue su GitHub ma **non erano elencate qui**, dove vive ciò che aspetta una
persona: questa sezione colma il buco.

✅ **Tutte e tre decise il 2026-08-10**, e con esse **R1** (Replay Archive e Recorder), **R2**
(serializzazione e compatibilità) e **R3** (Replay Player) diventano specificabili. La sezione resta come
**indice**: il contenuto vive nel Decision Log.

### Le tre risposte

| ID | Domanda | Risposta |
|---|---|---|
| [`#412`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/412) | Quale artefatto è **autorevole**, e dove passa il confine `ReplayPlayer`/`ReplayVerifier`? | [**ADR-0009**](decisions/adr-0009-replay-logico-canonico.md): **due prodotti, perimetri disgiunti**. Il **Player** ha per autorità la *traccia* e non calcola nulla; il **Verifier** ha per autorità il *resolver*, ri-simula e produce un verdetto, mai una presentazione. Il confine è reso **impossibile dalla struttura** (il Player vive dove il resolver non è raggiungibile — è già così in `#415`), con un test negativo come rete. A runtime il Player **non verifica**: rifiuta in apertura ciò che non sa leggere, e la verifica vive offline |
| [`#414`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/414) | L'archivio è per **partita** o per **turno**, e cosa identifica una partita? | [**D-077**](decisions/RT_PDR_00_Decision_Log.md): **entrambe le cose, a due livelli** — un **manifest per partita** più una **traccia per turno**. Le tracce restano come sono (`SaveTurnLogToFile` ne salva già una per file); il manifest è la casa che [D-062](decisions/RT_PDR_00_Decision_Log.md) aveva già assegnato a `HashTurnLogOrdered`, ed è lo stesso artefatto che l'indice di [#416](https://github.com/DegrassiAaron/refactor-tactics-main/issues/416) chiede. L'identità è un **`FGuid` generato all'avvio**, **fuori da ogni hash**: identifica la registrazione, non il contenuto |
| [`#413`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/413) | `ContentManifestHash` e `RulesVersion` si costruiscono **ora o alla v0.2**? E cosa entra nel manifest? | [**D-083**](decisions/RT_PDR_00_Decision_Log.md): **alla v0.2**, ma con il **perimetro deciso ora**. ⚠️ Misurando, il rischio del rinvio si è rivelato più piccolo di come la domanda lo poneva: il corpus golden vive **nello stesso repository delle regole**, quindi un ritocco di bilanciamento lo fa diventare **rosso**, con turno, fase e `ActionId` già nominati da `CompareSerializedTraces`. Ciò che manca non è il **rilevamento**, è l'**attribuzione**: un rebalance legittimo e una regressione si presentano identici. Perimetro: **entra ciò che il resolver legge** — catalogo azioni, catalogo eroi, costanti di combat, config del resolver; **fuori** presentazione, HUD, icone e la mappa, che ha già il suo hash |

---

## Aperte — economia delle azioni e accoppiamento col movimento, dal consolidamento del 2026-08-12

Origine: [referto del kit action economy](roadmap/plans/action-economy-consolidamento-2026-08-12.md).
Il kit d'autore proponeva 39 sezioni; **la maggior parte è già canone** e tre contraddicono decisioni prese
fra il 2026-08-07 e il 2026-08-10. Ciò che resta sono queste sette domande, di cui **cinque aperte**: `AE-1` e `AE-2` sono state chiuse il
2026-08-12 — da [D-114](decisions/RT_PDR_00_Decision_Log.md) e [D-116](decisions/RT_PDR_00_Decision_Log.md) — e
restano qui barrate, perché erano le due voci da cui dipendevano le altre. Le due risposte vanno lette insieme:
l'economia **resta a slot** e il peso si paga in drawback (`D-114`); il **profilo di movimento è uno di quei
drawback** (`D-116`).

🔴 **Una ottava non è qui perché esiste già.** «Il pivot consuma Movement Point» (§15 del kit) **è
`FAC-12`**, aperta il 2026-08-10 dal pacchetto `Facing_Claude_Consolidation` e registrata alla riga 68 di
[`DOC_CONFLICT_MATRIX.md`](DOC_CONFLICT_MATRIX.md). Aprirla di nuovo con un ID nuovo avrebbe prodotto due
voci che si chiudono a vicenda senza saperlo. Il fatto che **due sorgenti indipendenti** la chiedano in due
giorni diversi è però un dato: vedi la nota su `FAC-12`.

Owner della regola in vigore: [`gameplay/spec-economia-del-turno.md`](gameplay/spec-economia-del-turno.md).
Epic che le raccoglie: **E38** (v0.2).

| ID | Domanda | Perché serve una risposta |
|---|---|---|
| ~~`AE-1`~~ | ~~L'economia del turno resta a **slot** o diventa una **capacità numerica** con un costo per azione?~~ | ✅ **Chiusa il 2026-08-12** da [D-114](decisions/RT_PDR_00_Decision_Log.md), decisa dall'autore: **restano gli slot**, e due azioni leggere **non** valgono una pesante. Il peso di un'azione si paga in **drawback** — cooldown, status auto-inflitto, rinuncia alla reazione — non in costo, perché uno slot non ha una grandezza da sommare. La decisione non introduce il meccanismo: lo **nomina**, perché il catalogo già lo applica (`HeavyAttack` 35 danni / cooldown 2 contro `PrecisionAttack` 24 / 1). Issue [#604](https://github.com/DegrassiAaron/refactor-tactics-main/issues/604). ⚠️ **`ECO-1` non si dissolve**: con gli slot confermati è l'unica via rimasta per il piano «mi preparo e agisco» |
| ~~`AE-2`~~ | ~~Il **profilo di movimento** può cambiare la **legalità** e l'**efficacia** di un'azione?~~ | ✅ **Chiusa il 2026-08-12** da [D-116](decisions/RT_PDR_00_Decision_Log.md), decisa dall'autore: **sì**, con il modello a **soglia** — `MinStability` sull'azione contro `Stability` sul profilo — e le categorie da negare allo `Sprint` nominate: **precisione, preparazione, azioni pesanti**. La giustificazione adottata è l'**impegno del turno**, non la fisica del gesto. ⚠️ Non è arrivata da sola: la stessa decisione riporta lo `Sprint` **dopo il Blast** (superando `D-068`) e porta `Exposed` a **2 turni**, perché ciascuna delle tre presa da sola rompe il bilanciamento. Owner: [`gameplay/spec-compatibilita-azioni-movimento.md`](gameplay/spec-compatibilita-azioni-movimento.md) · issue [#606](https://github.com/DegrassiAaron/refactor-tactics-main/issues/606) |
| `AE-3` | I **fatti del percorso** (celle percorse, dislivello, superfici attraversate) modificano l'azione, oltre al profilo? | Va decisa **separatamente** da `AE-2`, e la ragione è di determinismo: il profilo è noto in Planning e si previsualizza esatto, i fatti del percorso sono noti solo dopo la risoluzione — e un Move può essere troncato, contestato o annullato ([D-045](decisions/RT_PDR_00_Decision_Log.md)). Una preview che promette «+1 spinta perché avrai percorso 4 celle» promette ciò che il turno può smentire. Fuori dallo scope di E38 |
| `AE-4` | Qual è il **contratto della risorsa firma**: nome player-facing, costo per azione, e quale dei tre valori vive? | Metà è **già decisa** e il kit non lo sapeva: la risorsa è **per personaggio** (Flux `Carica Conduttiva` · Riva `Riserva Idrica` · Bastion `Integrità Strutturale` · Vektor `Slancio`), cap **4**, ricarica **1** sul trigger d'affinità. ⚠️ Ciò che non è deciso è più concreto: `ARTUnit` dichiara `MaxEnergy = 100`, `EnergyPerTurn = 25`, `EnergyOnHit = 15` — parametri dell'MVP quadrato mai rivisti — e il **costo** in risorsa non è un campo del catalogo azioni (`EnergyCost` sta su `URTActionData`, il data asset legacy, non su `FRTActionDef`). Quindi un'azione del catalogo v0.1 **non sa dichiarare quanto costa** |
| `AE-5` | Con quali numeri esiste il profilo **`Sneak`**? | Costo, portata e rumore non sono definiti **da nessuna fonte corrente**: il catalogo lo dichiara apertamente e non si inventano. È l'unico dei quattro profili senza budget, quindi oggi non è pianificabile. Già elencato fra le assunzioni da bloccare in fondo a questo file; qui prende un ID perché `AE-2` lo userebbe come colonna |
| `AE-6` | `Wait` restituisce risorsa firma? | Il kit chiede di **non** concederlo di default, e il canone concorda: `Wait` non dà armatura, precisione, furtività né reazione gratis (`Actions.Wait.AllowsFacingAndReaction` verifica che conservi ciò che aveva, non che guadagni). Ma la ricarica «sul trigger d'affinità» non dice cosa succede a chi non innesca nulla per tre turni, e con cap 4 la differenza è grande |
| `AE-7` | Le **eccezioni per eroe** alla compatibilità e al vincolo dell'`Overwatch` sono contenuto di kit o regole? | Il pattern è già fissato da [D-014](decisions/RT_PDR_00_Decision_Log.md)/[D-028](decisions/RT_PDR_00_Decision_Log.md) — un'eccezione si dichiara **nel kit dell'eroe**, mai nella regola generale — ma nessun kit ne dichiara una, quindi il meccanismo che le renderebbe esprimibili non esiste. Da decidere **dopo** `AE-2`: prima non si sa a cosa si fa eccezione. ➕ **Il caso concreto è arrivato il 2026-08-12**, dal brainstorming su [#604](https://github.com/DegrassiAaron/refactor-tactics-main/issues/604): l'autore vuole poter giocare **`Dash` + attacco + `Move` normale**. Come **regola generale** contraddice [D-028](decisions/RT_PDR_00_Decision_Log.md) — lo scatto occupa lo slot movimento, e quella impossibilità *è* la scelta fra *schivo e sparo* e *sparo e muovo* — ma come **eccezione di kit** è più forte della regola: un turno che tutti possono fare non caratterizza nessuno. Resta da decidere **di chi è**, e il candidato ovvio è quello che complica: `Vektor` ha già `PassingBlade` in `FastMovement` — una mobilità che **attraversa e colpisce** per 20 danni, cioè già una forma di *muoviti e colpisci* nella fase `Dash`. Concedergli anche `Dash` + attacco + `Move` renderebbe `PassingBlade` dominata dalla combinazione generica, che è il difetto che [D-028](decisions/RT_PDR_00_Decision_Log.md) ha appena corretto fra `Dash+attacco` e `Charge`. L'eccezione va quindi **assegnata guardando cosa rende ridondante**, non solo cosa caratterizza |

> **Tre voci del kit non sono qui perché non sono domande.** *«Il divieto di `Dash` con l'`Overwatch`»* è
> una **conseguenza** già decisa ([D-070](decisions/RT_PDR_00_Decision_Log.md): lo slot movimento è riservato
> a `Withdraw`) e non una regola da scrivere · *«le azioni possono orientare senza pagare due volte»* è
> ADR-0008 §3, decisa e da implementare · *«aggiornare il workbook di bilanciamento»* è vietato per iscritto
> da [`balance/README.md`](balance/README.md) e da [D-023](decisions/RT_PDR_00_Decision_Log.md).

---

## ✅ Chiuse il 2026-08-12 — traversal contro transfer

> Aperte dal consolidamento Teleport del mattino, chiuse la sera dall'autore in sessione sul secondo
> handoff ([`roadmap/plans/spatial-transfer-epic-2026-08-12.md`](roadmap/plans/spatial-transfer-epic-2026-08-12.md)).
> Le due domande sono state poste **insieme** perché la seconda non ha senso senza la prima: decidere se un
> Blink entra in v0.1 prima di sapere se il trasferimento è una famiglia significa decidere il calendario di
> una cosa senza nome.

| ID | Domanda | Esito, e l'istruttoria che ci è arrivata sotto |
|---|---|---|
| ~~`MOV-1`~~ | ~~**`LinearLeap` è un'eccezione del Dash o il primo membro di una famiglia «trasferimento»?**~~ | ✅ **Chiusa il 2026-08-12** — [D-118](decisions/RT_PDR_00_Decision_Log.md), decisa dall'autore: **famiglia propria**. La tassonomia si partiziona in `Traversal` (percorre lo spazio, e ogni cella percorsa è un fatto) e `Transfer` (cambia posizione senza percorrerla), e `LinearLeap` **è già** il primo membro della seconda. Owner aggiornato: [`gameplay/spec-tassonomia-movimento.md`](gameplay/spec-tassonomia-movimento.md) §1–§2. Epic **E39**. ⚠️ La decisione **non** introduce un tipo nuovo oggi e **non** sceglie fra le due strade sotto: sceglie che quella scelta è una **migrazione di formato serializzato** e appartiene a un checkpoint con un test di compatibilità, non a chi scriverà il primo Blink. **L'istruttoria che l'ha prodotta**: Il kit d'autore porta una tesi giusta — *«un movimento molto veloce non è un teletrasporto»* — e una premessa falsa: che nel repository esista solo il primo. `ERTMovementStyle::LinearLeap` produce `Result.Entered = { destinazione }` e nient'altro, e poiché `ApplyTerrainOnEnterEffects` legge **esattamente** `Entered`, un `Action.Leap` non prende gli hazard intermedi, non genera micro-step intermedi e collide solo all'arrivo: **le tre righe che la matrice attribuisce al Teleport**. Le due strade non sono equivalenti. *(a)* **Eccezione del Dash**: un Blink diventa un valore nuovo di `ERTMovementStyle`, zero tipi nuovi, e la matrice tiene una colonna che descrive tre stili su quattro. *(b)* **Famiglia propria**: la matrice guadagna una colonna vera, ma `ERTMovementStyle` è serializzato negli asset e la migrazione va progettata. ⚠️ Da decidere **prima** che qualcuno scriva un Blink, non dopo: è la scelta che determina se il Blink è lavoro nuovo o un dato in più. 🔗 Incrocia `STA-4`/[#436](https://github.com/DegrassiAaron/refactor-tactics-main/issues/436), che possiede la **bloccabilità** (un `Root` ferma un trasferimento?) ma non la famiglia |
| ~~`MOV-2`~~ | ~~**Un Blink entra in v0.1**, o Teleport resta post-v0.1 design-ready?~~ | ✅ **Chiusa il 2026-08-12** — [D-119](decisions/RT_PDR_00_Decision_Log.md), decisa dall'autore: **post-v0.1**, epic **E39** in **v0.2**. La v0.1 resta a quattro eroi che non saltano. ⚠️ È la **prima** decisione sul perimetro del Teleport: l'assenza precedente era un fatto misurato, non una scelta — quindi qui non si conferma niente, si decide. 🔴 **`#645` non è rinviata da questa chiusura**: il salto è irraggiungibile dal roster qualunque sia la release, e la sua domanda si risponde prima e altrove. **L'istruttoria che l'ha prodotta**: Il kit propone un `Short Blink` come primo caso, e il canone dice che il Teleport non esiste in v0.1. ⚠️ **Non c'è nessuna `D-nnn` che fissi quell'assenza**: `git grep -i teleport docs/decisions/` non restituisce nulla. L'assenza è un **fatto misurato** («nessuna azione del catalogo dichiara quella famiglia»), non una decisione presa — quindi portare un Blink in v0.1 non contraddirebbe una decisione, ma ne richiede una. La domanda vera non è tecnica: la semantica esiste già in `LinearLeap` e lo scenario che la prova è già scritto (§sotto). È di **scope**: la v0.1 ha quattro eroi e nessuno di loro salta |

> **Lo scenario esiste già, ed è deliberatamente inerte.**
> [`Scenarios/Spec/Movement/TeleportSkipsIntermediateCells.json`](../Scenarios/Spec/Movement/TeleportSkipsIntermediateCells.json)
> è una specifica eseguibile scritta **prima** dell'implementazione: due celle di fuoco fra origine e
> destinazione, Flux a 90 HP, e l'assertion che dopo il trasferimento sia **ancora 90**. Il turno 2 dichiara
> `requires: ["Teleport"]`, capability che non esiste, quindi resta `BLOCKED` e i 90 HP passano perché Flux
> non si muove mai. Il file porta dentro anche le istruzioni per chi lo completerà.
> **Il kit ne proponeva sei di nuovi: cinque sono premature e il sesto è questo.**

---

## Aperta — chi può leggere una traccia, dallo spec panel del 2026-08-12

Origine: [`roadmap/plans/five-lane-roadmap-spec-panel-2026-08-11.md`](roadmap/plans/five-lane-roadmap-spec-panel-2026-08-11.md) §7C.
Il sorgente revisionato proponeva un intero modello di privacy del replay come lavoro della v0.1: è **fuori
scope** — la v0.1 è 2v2 **offline contro un bot**, e [D-078](decisions/RT_PDR_00_Decision_Log.md) lo scrive
(«nessun avversario, nessun server»). Ma la **domanda** sopravvive alla proposta, e va registrata prima che
esista un consumatore: è lo stesso schema di [D-083](decisions/RT_PDR_00_Decision_Log.md), perimetro deciso
adesso e costruzione rinviata.

⚠️ `REPLAY-01`…`REPLAY-09` sono i **rischi** del §32 del kit di consolidamento, non domande: da qui il
prefisso `REP-`, che era libero.

➕ **`REP-2` si è aggiunta il 2026-08-12**, da una domanda diversa: assegnando `replay_representable` alle
feature del bot è emerso che la traccia non distingue un bot equo da un bot onnisciente. È lo stesso oggetto
di `REP-1` — *cosa contiene una traccia* — preso dal lato opposto: `REP-1` chiede chi può leggerla, `REP-2`
chiede se debba contenere abbastanza da falsificare una proprietà del bot. Le due si vincolano a vicenda, e
per questo stanno nella stessa tabella.

| ID | Domanda | Perché non si deduce |
|---|---|---|
| `REP-1` | Una traccia archiviata può essere **letta da chiunque**, o esistono profili di lettura distinti (audit · squadra · pubblico)? | Oggi non ha consumatori — nessun avversario e nessun server — quindi **non è urgente**, ma non è nemmeno neutra: `D-077` mette un `FGuid` di partita nel manifest e il TurnLog porta `UnitId`/`TurnNumber` ([D-063](decisions/RT_PDR_00_Decision_Log.md)), cioè la traccia sa già **chi** ha fatto **cosa**. Il momento per decidere il perimetro è prima che un archivio esca dalla macchina che l'ha prodotto — lo stesso innesco che `D-083` ha già dichiarato per `ContentManifestHash`. ⏳ **Default prudente in vigore fino alla decisione**: non pubblicare automaticamente intenti storici. «Partita finita» **non** implica «tutto il planning diventa pubblico»: è una scelta di prodotto, e nessun documento l'ha mai fatta. Innesco: la prima delle tre condizioni fra multiplayer (`RT-FEAT-NET-PRIVATE-PLANNING` oltre `TESTABLE`), condivisione di archivi fra macchine, o uno spettatore |
| `REP-2` | Una traccia deve contenere abbastanza da **falsificare l'equita' del bot**, cioe' lo stato di conoscenza su cui ha pianificato? | Oggi no, e non per una svista: un bot che pianifica su `FRTTeamKnowledge` e un bot onnisciente che fa la stessa mossa scrivono voci **identiche** — la traccia registra l'azione, non cio' che il decisore sapeva. `RT-FEAT-BOT-FAIRNESS` e' `TESTABLE` e la sua proprieta' la tiene solo la verifica di mutazione su `HiddenEnemyFairness`; nessun gate del registry la protegge, e infatti il suo `replay_representable` e' `na` nel senso di «non e' possibile». `RT-FEAT-BOT-PREDICTIVE` dichiara lo stesso caso come proprio test da scrivere: «dedurre» e «sapere» sono indistinguibili dall'esterno. ⚠️ **Non e' gratis**: registrare la conoscenza del bot significa mettere nella traccia lo stato privato di un lato, cioe' esattamente cio' che `REP-1` sta perimetrando e che il default prudente vieta di pubblicare. Con un avversario umano sarebbe un vantaggio informativo, non un audit. Innesco: quando l'equita' del bot diventa un requisito da **dimostrare a terzi** invece che da verificare in casa — un torneo, un replay condiviso, un bot di terze parti |

---

## Aperta — l'invertibilità della cottura, dallo spec panel del 2026-08-12

Origine: [`roadmap/plans/map-sketch-editor-spec-panel-2026-08-12.md`](roadmap/plans/map-sketch-editor-spec-panel-2026-08-12.md) §4.2.
Il sorgente revisionato ([Map Sketch Editor v0.1](archive/src/handoff/2026-08-12-map-sketch-editor.md)) separa
correttamente *authoring geometry* da *runtime spatial data* (§20) e non dice cosa accade **dopo** la cottura,
se qualcuno tocca a mano un campo cotto.

Due decisioni dell'autore sono state prese nella stessa sessione e **non** sono questa: la geometria
quantizzata si anticipa in v0.1 come strumento d'editor (`#619`–`#621`, anticipazione dichiarata su **E23**
[`#324`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324), che non si apre), e layout
generato e geometria disegnata **convivono** perché hanno soggetti diversi. È proprio la seconda a rendere la
domanda urgente: due produttori sullo stesso artefatto.

| ID | Domanda | Perché non si deduce |
|---|---|---|
| `MSE-1` | Quando la geometria disegnata è **cotta** in `bBlocksMovement` / `FRTHexCover`, il sorgente resta autorevole? Cioè: se qualcuno modifica a mano un campo cotto, vince la modifica o il prossimo rebake la cancella? | È la stessa classe di problema dei **prefab**, e il panel del 2026-08-09 l'aveva già segnalata come costo da *decidere, non scoprire* — senza deciderla. Non si deduce dal codice perché oggi **la cottura non esiste**: `FRTHexCellData` ha un solo produttore, il pennello, e la domanda non si pone. Si porrà al primo segmento cotto. ⚠️ La decisione **4.2** la rende più stretta: se il layout è generato da codice e la geometria è disegnata, l'asset ha **due** produttori, e un test che confrontasse il suo hash con quello del solo generatore diventerebbe rosso al primo muro disegnato — quel test, se verrà scritto, deve avere per soggetto la parte **generata**. Innesco: [`#621`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/621), che è dove la cottura nasce |

> 🔎 **Ristretta il 2026-08-12: `#619` cuoce, ma non innesca.** La revisione spec panel di `#619` ha preso due
> decisioni che tolgono a `MSE-1` la sua metà più vicina. `D1`: il confine `#619`/`#621` è **per campo** — il
> **costo** è di `#619`, i **bordi** di `#621` — quindi la prima cottura in ordine di serie nasce già in
> `#619`. `D2`: il sovrapprezzo di `Constrained` **non** si scrive in `MoveCost`, ma in un campo suo, proprio
> perché `MoveCost` ha già un produttore che lo ricalcola dalla `Surface` ogni turno
> (`ARTTurnManager::ApplyDynamicSurface` e `TickDynamicSurfaces`).
>
> Effetto su questa domanda: per il **costo** non si pone più, perché `D2` gli dà **un produttore solo** — e
> `MSE-1` chiede esattamente cosa succede quando i produttori sono due. La cottura di `#619` quindi **non è un
> innesco**, e `Innesco: #621` nella riga sopra resta corretto: `FRTHexCover` e `bBlocksMovement` restano a
> produttore condiviso col pennello, ed è lì che la domanda va decisa.
>
> ⚠️ Nota di metodo: `D2` è la stessa forma di risposta che `MSE-1` cerca — *separare i produttori invece di
> arbitrarli*. Se regge per il costo, è il primo candidato da provare sui bordi.

> 🔎 **Ristretta di nuovo il 2026-08-12: `MSE-1` resta a DUE campi, e non diventerà tre.**
> Origine: [revisione spec panel dei due handoff Level Designer](roadmap/plans/level-designer-handoff-spec-panel-2026-08-12.md).
>
> L'handoff proponeva `footprint void/cliff → ERTHexSurface::Void`, che avrebbe creato un **terzo** campo a
> produttore condiviso — e nessuno l'aveva contato. **Deciso: il bake non scrive `Surface`.** Tre ragioni,
> la seconda è quella che decide:
>
> 1. `Void` è una **superficie dipinta**, membro di un enum di nove valori accanto a `Floor`,
>    `ShallowWater`, `Rough`, `Fire`, `Conductive`, `Ice`, `Smoke`, `HighGround`: nessuna regola
>    geometrica sa scegliere fra nove.
> 2. **`Fill` propaga sulla contiguità di superficie.** Una `Surface` cotta non cambierebbe una cella:
>    cambierebbe il confine di *ogni futuro flood fill* che la attraversa. È un effetto sullo **strumento**,
>    non sul dato — categoria peggiore di `MSE-1`, non uguale. È il motivo per cui non basta estendere
>    la domanda: bisogna non porla.
> 3. Il precipizio è già esprimibile: `bBlocksMovement = true` + `bBlocksLineOfSight = false` dice «non ci
>    si sta sopra, ma ci si vede attraverso», e lo distingue da un muro. Entrambi i campi sono già di
>    `#621` per `D1`.
>
> È la forma di `D2` nella sua versione più economica: non separare i produttori — **non creare il secondo
> produttore**. La `D-0xx` la prende la PR che implementa `#621`.
>
> ⚠️ Collisione di terminologia registrata qui perché continuerà a mordere: `VoidFootprint()` in
> `Source/RefactorTactics/Tests/RTOccupancyFixtures.h` significa «contorno chiuso che **non contiene il
> centro**», cioè il gemello di controllo del solido. **Non** significa `ERTHexSurface::Void`.

---

## Aperta — le soglie di occupancy contro la grammatica di #620, dallo spec panel del 2026-08-12

Origine: [revisione spec panel dei due handoff Level Designer](roadmap/plans/level-designer-handoff-spec-panel-2026-08-12.md) §D0-bis.
Non nasce da una contraddizione fra documenti, ma da una **collisione fra due decisioni entrambe corrette**,
prese a settimane di distanza e mai messe una accanto all'altra.

I confini radiali dei dodici settori di `#619` stanno a `-30 + 30k` gradi
(`RTHexOccupancyLibrary.cpp:99`). Gli assi tattici che `#620` vuole imporre sono `0/30/60/90/120/150`.
**Sono lo stesso insieme di angoli.** E il contatto su un confine conta come occupazione di *entrambi* i
settori adiacenti — scelta deliberata e commentata (`RTHexOccupancyLibrary.cpp:38`), giusta presa da sola.

Ne segue che ogni segmento canonico di `#620` nascerà nel caso collineare: quello che le **quattro fixture
evitano di proposito** — usano `-20`, `-10`, `10`, `40`, `-15` gradi, e il commento della prima lo dichiara
— e che **nessuno dei diciassette test `HexOccupancy.*` copre**. Le soglie `ConstrainedFrom = 4` e
`BlockedFrom = 6` sono state calibrate contro geometria fuori asse e stanno per ricevere solo geometria in
asse.

| ID | Domanda | Perché non si deduce |
|---|---|---|
| `MSE-2` | Le soglie di occupancy vanno **ritarate** ora che la grammatica di `#620` produce solo geometria collineare ai confini di settore, o la regola conservativa va resa esclusiva, o si accetta il conteggio più alto come semantica voluta? | Non si deduce dal codice perché **oggi la geometria in asse non esiste**: `#620` è aperta, e nessuna fixture esercita il caso. I numeri invece **ora esistono** — misurati il 2026-08-13, vedi il blocco qui sotto — e quello che resta aperto è una scelta di semantica, che nessuna misura può fare al posto dell'autore. ⚠️ E non è più un'ipotesi: **due** muri su sei lati portano la cella a `Blocked` mentre **quattro lati restano aperti**, e `#621` cuocerebbe quel `Blocked` in `bBlocksMovement` rendendo **impassabile una cella attraversabile**. Cinque uscite, e la quinta è emersa dalla misura: ritarare le soglie (non tocca codice chiuso, solo default) · rendere esclusiva la regola collineare (cambia una regola chiusa, va motivata) · **scartare il contatto di misura nulla** — un settore toccato in un solo punto non è invaso — che è più fine della precedente e lascia le soglie dove sono · contare i **lati murati** invece dei settori quando la geometria è perimetrale (è il conteggio che il designer ha in testa) · accettare e dichiararlo. Innesco: [`#620`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/620), prima che la grammatica fissi la forma dei segmenti. **Tracciata in [`#717`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/717)** insieme a `MSE-3` |

> 🔎 **La suite è girata il 2026-08-13, e il margine NON regge.** 19 test `HexOccupancy.*` dichiarati,
> 19 eseguiti, 0 falliti (build `RefactorTacticsEditor` Development, `-nullrhi`). I numeri che
> `PerimeterWallsOccupancyIsRecorded` registra:
>
> | Muri su lati consecutivi | Settori occupati | Quali | Classificazione | Lati ancora aperti |
> |---:|---:|---|---|---:|
> | 1 | 4 / 12 | `{0,1,2,11}` | `Constrained` | 5 |
> | 2 | 6 / 12 | `{0,1,2,3,4,11}` | 🔴 **`Blocked`** | 4 |
> | 3 | 8 / 12 | `{0,1,2,3,4,5,6,11}` | `Blocked` | 3 |
>
> **Due muri sono l'angolo di una stanza**: la geometria più comune che un designer disegni, e già oggi
> supera `BlockedFrom`.
>
> 🔑 **La causa è isolata, e non sono le soglie.** Lo stesso muro rientrato del 5% agli estremi — stessa
> giacitura, stesso lato, ma senza toccare i due vertici — occupa **2 settori invece di 4**. Metà del
> conteggio è **contatto sul solo vertice**, cioè area invasa nulla: un vertice dell'esagono è il punto in
> comune fra quattro triangoli di settore, e un estremo che ci cade sopra ne accende due che la geometria
> non invade. Scartando il contatto di misura nulla il conteggio diventa `2N` — 1 muro `Free`, 2
> `Constrained`, 3 `Blocked` — che è esattamente la lettura del designer, **con le soglie attuali intatte**.
>
> ⚠️ Questa quinta uscita **non contraddice** la regola conservativa che `SegmentOnSectorBoundary...`
> protegge: là il contatto è lungo un *segmento*, e resterebbe occupazione. La distinzione è fra contatto
> di misura nulla e contatto esteso, non fra collineare e non collineare. Cambia comunque una regola di
> `#619` già chiusa, quindi resta una **decisione**, non una correzione: va presa prima di `#621`.

---

## Aperta — due modelli di calpestabilità, dallo spec panel del 2026-08-13

Origine: [revisione spec panel del brief HexGeometry e del bundle `grid/`](roadmap/plans/hexgeometry-editor-spec-panel-2026-08-13.md) §C1.
Come `MSE-2`, non nasce da una contraddizione fra documenti ma da **due decisioni entrambe corrette, prese a
settimane di distanza e mai messe una accanto all'altra**. Questa volta però i due modelli sono entrambi
documentati, e uno dei due è già **implementato**.

Il repository risponde alla domanda «questa cella è calpestabile?» in due modi:

| | Modello | Dove vive | Stato |
|---|---|---|---|
| **A** | **Cerchio inscritto**: l'ingombro dell'unità è il cerchio più grande dentro l'esagono; se tocca il muro la cella non è valida. **Binario** | [`D-071`](decisions/RT_PDR_00_Decision_Log.md) · Wiki `Meccaniche/griglia-e-geometria.md` · `RT-FEAT-MAP-STANDABILITY` | `DESIGNED`, gate 0/8, **v0.2** |
| **B** | **Dodici settori con soglie**: `≥4` → `Constrained`, `≥6` → `Blocked`. **Ternario** | `#619` · `RTHexOccupancyLibrary` | **implementato**, 19 test verdi |

| ID | Domanda | Perché non si deduce |
|---|---|---|
| `MSE-3` | I due modelli sono **lo stesso modello a due livelli di dettaglio** — e allora quale produce `bBlocksMovement` — oppure sono **due regole concorrenti**, e una va ritirata? | Non si deduce dal codice perché **il modello A non è implementato**: esiste come decisione e come pagina Wiki, non come funzione. Non si deduce dai documenti perché **nessuno dei due li cita insieme**: il brief HexGeometry §7 presenta solo B come «baseline corrente», il bundle `grid/` §1 dichiara solo che «prevale D-071». ⚠️ Danno **risposte diverse sulla stessa geometria**: un muro appoggiato a un lato è *tangente* al cerchio inscritto — caso limite per A — mentre B gli assegna 4/12 settori, cioè `Constrained`; con due muri B dice `Blocked` mentre quattro lati restano aperti. Tre uscite: A è il **gate** e B è il **dettaglio del costo** (i due si compongono, e `Blocked` di B smette di produrre impassabilità) · B assorbe A e `D-071` diventa la *motivazione* delle soglie invece di una regola separata · restano separati per **domande diverse** — A per «ci sta un'unità», B per «quanto è stretta» — e allora va detto quale scrive `bBlocksMovement`. Innesco: [`#621`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/621), la cottura, che è il primo codice che deve **scegliere**. **Tracciata in [`#717`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/717)** insieme a `MSE-2` |

> 🔎 **Perché non la chiude la revisione che l'ha trovata.** La prima uscita è tecnicamente attraente — `D-071`
> come gate binario e le soglie come modulazione del costo — e risolverebbe anche `MSE-2`, perché toglierebbe a
> `Blocked` il compito di rendere impassabile. Ma `D-071` è una decisione d'autore registrata nel Decision Log
> e spiegata al giocatore in una pagina Wiki: ridefinirne il ruolo è una scelta di design, non un refactor.
>
> ⚠️ **`MSE-2` e `MSE-3` vanno decise insieme**: la seconda uscita di `MSE-2` (scartare il contatto di misura
> nulla) e la prima di `MSE-3` risolvono lo stesso sintomo — celle dichiarate `Blocked` con lati aperti — da
> due lati diversi. Prenderne una sola rischia di lasciare l'altra senza problema da risolvere, o di risolverlo
> due volte.

---

## ✅ Chiuse il 2026-08-10 — geometria, acqua e strutture, dal quinto sorgente

Origine: [triage `FULL CHAT CONSOLIDATION`](roadmap/plans/triage-grid-geometry-water-2026-08-10.md).
Erano **tre**, e nessuna bloccava la v0.1. **Tutte e tre chiuse lo stesso giorno**, e nessuna ha richiesto
l'autore: due si sono decise **misurando il repository** invece di ragionare a tavolino, la terza rispondendo
col codice in mano — che era la condizione che il triage stesso poneva. Restano qui **solo come indice**: il
contenuto vive nel Decision Log.

| Era | Decisione presa | Dove vive ora |
|---|---|---|
| `GEO-1` | La profondità dell'acqua è una **superficie**, non un asse. ⚠️ La domanda poggiava su una premessa falsa: `ShallowWater` **cambia** — `Action.CreateWater` la crea a runtime da CP 8.4, quindi il *flooding* esiste già come cambio di superficie. L'asse costerebbe una **versione del formato mappa**, per un'espressività che nessuna regola consuma | [`D-081`](decisions/RT_PDR_00_Decision_Log.md) · owner [`spec-terreni-e8.md`](gameplay/spec-terreni-e8.md) §6-ter |
| `GEO-2` | Lo slot strutturale si chiama **`Bulkhead`**, verificato libero con `git grep`: zero occorrenze. Scartati `StructuralSlot` (`Slot` è l'economia del turno), `Panel` (collide con `Bastion.KineticPanel`, che è *copertura*) e `Section` (termine Unreal per le mesh) | [`D-082`](decisions/RT_PDR_00_Decision_Log.md) · owner [triage](roadmap/plans/triage-grid-geometry-water-2026-08-10.md) §3 |
| `GEO-3` | Il modello causale delle §22–§27 **non entra** nel v6: presuppone un `EventId` per voce, che non esiste. Tre rinvii con l'innesco dichiarato — cause contribuenti, dedup, provenance — e **una risposta piena**: un `EventId` sarebbe **identità**, quindi resterebbe fuori dall'hash come `UnitId` e `TurnNumber` | [`D-080`](decisions/RT_PDR_00_Decision_Log.md) · owner [`spec-turnlog.md`](technical/spec-turnlog.md) §12-bis |

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
>
> ✅ *Aggiornamento dello stesso giorno (CP 14.3, PR #494)*: il test **è atterrato** — difende però la **forma
> del DTO** (nessun campo fuori da un elenco chiuso, via reflection), non il facing intermedio. Per questa
> voce il requisito resta dichiarato e la verifica specifica ancora no.

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
| `FAC-12` | Il pivot **si paga** in punti movimento, o resta solo un **tetto**? | ADR-0008 §1 misura la rotazione in step e la tratta come un tetto (`MoveEndPivotMaxSteps`): ruotare fin dove è consentito è **gratis**. La fonte §10 propone un prezzo — `Move 2 celle + Pivot 60° = 3 MP` — che mette *quanto mi muovo* contro *quanto ruoto*, e farebbe pagare 3 MP anche a chi ruota da fermo (oggi libero e universale). Sono due assi di scelta diversi, non due formulazioni. Da guardare alla **prima revisione dei numeri di ADR-0008**, cioè alla chiusura di CP 16.2. 🔄 **Riproposta il 2026-08-12** dal kit dell'action economy (§15), che la presenta come «*latest explicit direction*» senza sapere di ADR-0008. Non cambia la risposta e **non cambia la data della revisione**: cambia il peso della domanda, perché due sorgenti indipendenti hanno chiesto la stessa cosa a due giorni di distanza. Il secondo aggiunge un argomento che il primo non aveva — *«viaggio più lontano o arrivo orientato bene?»* è la scelta che il **tetto** non produce, perché un tetto non si spende |
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
## Aperte — geometria, clearance e confine Guard/Brace, dal consolidamento del 2026-08-10

Origine: [conflict report dell'handoff geometria](roadmap/plans/handoff-geometry-reazioni-conflict-report-2026-08-10.md).
Delle **17** domande che il sorgente elencava, **sette non erano aperte** — tre già decise, due già tracciate
come `CLASH-1`/`CLASH-2`, due con risposta in un altro sorgente non ancora consumato. Queste cinque sono il
residuo.

| ID | Domanda | Perché serve una risposta |
|---|---|---|
| `MAP-2` | Che succede quando la **linea di tiro sfiora l'angolo** di un muro? | È il caso che la geometria arbitraria crea e la griglia allineata non aveva: con muri a 90° che tagliano le celle, `HasLineOfSight` incontra tangenze che oggi non esistono. La risposta decide anche i corner case di proiettile e copertura, che sono lo stesso problema visto dal lato del danno. Va decisa **con una fixture in mano**, non per principio |
| `MAP-3` | La **cottura non è invertibile**: cosa succede se qualcuno modifica a mano il dato cotto? | Registrato come rischio il 2026-08-09 e ancora aperto. Se si edita `bBlocksMovement` su una cella cotta, il prossimo ricalcolo cancella la modifica **in silenzio** — stessa classe di problema dei prefab. Le uscite sono tre (vietare l'edit, marcare la cella come «sganciata», o rinunciare al ricalcolo automatico) e nessuna è deducibile dai documenti |
| `BAL-1` | `Guard` e `Brace` devono separarsi in **danno contro spinta**? | [D-066](decisions/RT_PDR_00_Decision_Log.md) ha misurato il modello in vigore: entrambi fanno entrambe le cose, e differiscono per *forma* (primo colpo forte vs ogni colpo; spinta di 1 cella vs spinta qualsiasi). È bilanciamento: si chiude con una partita, non con un documento. ✅ **La Fase 0 è decisa** ([D-074](decisions/RT_PDR_00_Decision_Log.md), 2026-08-10, issue [#400](https://github.com/DegrassiAaron/refactor-tactics-main/issues/400)): si accetta che in v0.1 ogni spinta valga 1 e si **riscrive** la clausola «senza limite di distanza» invece di introdurre una spinta `≥ 2`. Conseguenza sulle opzioni ancora in campo: restano lo **status quo** e l'**ibrido** (separare le magnitudini); l'opzione *«`Guard` solo danno, `Brace` solo spostamento»* era **preclusa**, perché senza spinta forte lasciava `Brace` senza mestiere. 🔄 **Non più, dal 2026-08-11**: `Weapon.Impact` su `Riva.PressureJet` produce una spinta di **2** ([D-085](decisions/RT_PDR_00_Decision_Log.md)) ed è il default di Riva ([D-089](decisions/RT_PDR_00_Decision_Log.md)), quindi la spinta forte che `D-074` aveva scartato è arrivata dall'**equipaggiamento** invece che dal catalogo azioni. Misurato da `Equipment.PushTwoSeparatesGuardFromBrace`: contro una spinta di 2 **`Guard` cede e `Brace` regge**. Le opzioni tornano **tre**, con una domanda nuova — quel mestiere dipende da un equipaggiamento equipaggiato, non da una regola del turno. ✅ Gli scenari che servono a decidere esistono e sono verdi ([#401](https://github.com/DegrassiAaron/refactor-tactics-main/issues/401)): `Spec.Brace.GuardAndBraceOnMixedHit` e `Spec.Brace.BraceWinsOnSecondHit` pinnano il trade-off reale — *primo colpo pesante* (`Guard` 1 danno) contro *colpi ripetuti* (`Brace` 12 contro 17 su due colpi). ⏳ **Resta l'unica parte che richiede l'autore**: la seduta editor **U20** (voce `PIE-BAL1`) e la scelta fra le due opzioni superstiti. Roadmap e numeri: [`bal-1-guard-brace-roadmap-2026-08-10.md`](roadmap/plans/bal-1-guard-brace-roadmap-2026-08-10.md). Issue [#403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/403) (decisione) |
| `ECO-1` | `Guard` e `Brace` competono con il **Main Commitment**, o hanno un'altra economia? | [D-012](decisions/RT_PDR_00_Decision_Log.md) copre `Attack \| Ability \| Overwatch` e **non** dice nulla di `Guard` e `Brace`, che a catalogo occupano l'azione principale ma non compaiono in quella regola. La domanda si porta dietro la matrice Sprint/Sneak proposta dal sorgente (`Brace` e `Overwatch` senza Sprint), che **non è canonica** e non va resa tale senza playtest. 🔄 **Generalizzata il 2026-08-12 da `AE-1`**: il kit dell'action economy pone la stessa domanda per *tutte* le azioni, non per due. Resta qui perché è la sua istanza concreta — se `AE-1` conferma gli slot, `ECO-1` va comunque risposta; se li sostituisce, `ECO-1` si dissolve. ➕ **Ha una proposta e quattro opzioni misurate**: [#617](https://github.com/DegrassiAaron/refactor-tactics-main/issues/617). ⚠️ **E la domanda è mal posta in un punto**: tratta `Guard` e `Brace` come un caso solo, mentre il catalogo dice che **`Brace` paga due prezzi e `Guard` uno** — `Action.Brace` applica a sé `Braced` **e `Root`**, e `Root` porta `EffectiveMoveRange` a zero già nel `Move` dello stesso turno. Da qui l'opzione più economica: `Brace` esce dalla principale perché il radicamento *è già* il suo prezzo, `Guard` resta. La via che sembrava ovvia — spostarli sullo slot `Reaction` — **è più cara di così**: quello slot è l'ingresso alla macchina dei trigger, e nessuna delle due azioni ne ha uno. Da tenere separata da `BAL-1`/[#403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/403), che chiede *cosa fanno*: se costo ed effetto cambiano insieme, il playtest non sa quale delle due sta misurando |

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
✅ **`OW-5` chiusa il 2026-08-12** → [D-109](decisions/RT_PDR_00_Decision_Log.md): la v0.1 ammette **una sola**
condizione dichiarata, `TargetHealthAtOrBelowPercent(N)`, con l'elenco **nel codice** e la soglia intera. Resta
qui come indice, perché la voce aveva una premessa che si è rivelata falsa e vale segnarlo: diceva che nessuna
opportunity a due risposte esistesse nel gioco, mentre `BuildOverwatchTriggers` (CP 14.4, **chiusa**) ne produce
da prima che la domanda fosse posta. Una decisione può restare ferma per un ostacolo che non c'è più.

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

## Aperte — varianti d'arma, dal consolidamento del 2026-08-11

Origine: [`RefactorTactics_WeaponVariants_Claude_Consolidation.md`](archive/src/RefactorTactics_WeaponVariants_Claude_Consolidation.md)
(archiviato). Quattro decisioni del sorgente erano `Locked` e sono diventate
[D-085](decisions/RT_PDR_00_Decision_Log.md)–[D-088](decisions/RT_PDR_00_Decision_Log.md). Queste sono le
`Provisional` e le `Open`, che il sorgente stesso vieta di promuovere senza conferma.

| ID | Domanda | Perché serve una risposta |
|---|---|---|
| ~~`WV-1`~~ | ~~Che cosa significa **`+1 turno di ricarica`** per `Weapon.Overcharge`?~~ | ✅ **Chiusa il 2026-08-11** — [D-090](decisions/RT_PDR_00_Decision_Log.md): è una variante **burst**, bonus per fascia (+18/+14/+8) e costo `CooldownDeltaTurns = +2`. La traduzione letterale «+1» è stata **misurata e scartata**: vale zero, perché `TickCooldowns()` gira nel Cleanup dello stesso turno. Implementabile solo dopo [#509](https://github.com/DegrassiAaron/refactor-tactics-main/issues/509), e i numeri restano `PROPOSED FOR PLAYTEST` con `WV-2` |
| `WV-2` | Le **soglie delle fasce** di danno e i **delta per fascia** | [D-087](decisions/RT_PDR_00_Decision_Log.md) ha deciso il **principio**; i numeri sono `PROPOSED FOR PLAYTEST`. Baseline proposta: `Low 1–10` · `Medium 11–18` · `High 19+`, con `Precisione −2/−3/−4`, `Sovraccarico +3/+5/+6`, `Soppressione −2/−4/−5`. Si chiudono con una partita, non con un documento — e vanno misurati **dopo** che le fasce esistono nel codice |
| ~~`WV-3`~~ | ~~Il **default** di variante per ciascun eroe~~ | ✅ **Chiusa il 2026-08-11** — [D-089](decisions/RT_PDR_00_Decision_Log.md): `Flux → Precisione`, `Riva → Impatto`, `Vektor → Soppressione`, `Bastion → Impatto`. Criterio: il default **rinforza l'identità**. Nessun default usa `Sovraccarico`, il cui costo è ancora `WV-1` |
| ~~`WV-4`~~ | ~~Che cosa modifica **`Weapon.Environmental`**~~ | ✅ **Chiusa il 2026-08-11** — [D-100](decisions/RT_PDR_00_Decision_Log.md): resta fuori dalla v0.1, e non perché manchi **un parametro** ma perché manca un **produttore** — *nessun attacco base crea ambiente*. Su Flux e Vektor non avrebbe niente da migliorare. Si riapre da sé il giorno in cui un attacco base dichiarerà `bCreatesSurface` |
| `WV-5` | Il modello di **selezione dei bersagli** per `Weapon.Split` | Prima del campo serve il modello: chi seleziona, in che ordine, automatico o manuale, distanza, prevenzione dei duplicati, ordinamento stabile, serializzazione, intento, ghost preview, bot, TurnLog, replay. ⚠️ E la domanda che viene prima di tutte: la cardinalità dei bersagli serve **anche al sistema abilità**, o si introdurrebbe per salvare una sola variante? Nel secondo caso il costo non vale il ritorno |

---

## Aperte — conoscenza e facing, trovata da uno scenario il 2026-08-11

Non nasce da un documento ma da uno **scenario portato avanti**: `Spec.Perception.CannotShootWhatYouCannotSee`,
scritto durante CP 13.2 e mai mergiato, è stato riscritto sull'API attuale ed **eseguito**. Il gate della
conoscenza regge (il bersaglio ignoto non subisce danno), ma è caduta l'assertion sul facing.

> ⚠️ **Da sapere prima di leggere la riga sotto**: il harness degli scenari **non deriva** il facing
> iniziale. `RTScenarioSession` fa `Unit->Facing = Spec.Facing` — la posa dichiarata nel JSON, `E` di
> default — e [D-044](decisions/RT_PDR_00_Decision_Log.md), «ci si schiera guardando il nemico più
> vicino», **non viene applicata**. Ne segue che nessuno scenario oggi esercita lo schieramento, e che una
> geometria può asserire un facing iniziale che una partita vera non produrrebbe mai. Gli scenari di questa
> coppia restano onesti perché la loro configurazione è **raggiungibile** sotto D-044 (il secondo nemico a
> est catturerebbe lo sguardo), ma è una proprietà scritta a mano, non verificata dal harness.

| ID | Domanda | Perché serve una risposta |
|---|---|---|
| `PER-4` | Un'azione **rifiutata dal gate della conoscenza** deve comunque **orientare** l'attaccante verso il bersaglio? | Misurato: sì, oggi lo orienta. Il codice segue [D-020](decisions/RT_PDR_00_Decision_Log.md) — «un'azione con bersaglio orienta l'unità **prima** di risolvere» — e infatti la rotazione (`RTTurnManager.cpp:2577`, `TargetingReoriented`) precede il gate (`:2632`). ⚠️ Ma **D-020 è anteriore a CP 13.2** e non poteva prevedere un gate che *rifiuta* l'azione: il facing è **osservabile dall'avversario**, quindi girarsi verso un nemico che la squadra non conosce fa trapelare che lo si conosce — cioè tocca l'invariante #6 (privacy dell'intento) per una strada che nessuna delle due decisioni aveva davanti. Le uscite sono tre: (a) D-020 vince e si accetta il tell; (b) la rotazione si sposta **dopo** il gate, e allora va deciso cosa fa un'azione rifiutata *a metà* della timeline dei facing di D-020; (c) la rotazione avviene ma verso la **cella**, non verso l'unità, se il contatto è solo `Incerto`. ⚠️ Finché è aperta, lo scenario **non asserisce sul facing**: pinnare `E` accuserebbe il gioco di un difetto non deciso, pinnare `W` renderebbe canone per inerzia un possibile leak |

---

## ✅ Chiuse il 2026-08-12 — radar di personaggio

Origine: [`RefactorTactics_Character_Radar_Wiki_Generator_Claude.md`](archive/src/RefactorTactics_Character_Radar_Wiki_Generator_Claude.md)
(archiviato). Il modello era stato consolidato in [D-105](decisions/RT_PDR_00_Decision_Log.md); **tutte e
cinque** le voci residue sono state decise dall'autore in sessione il 2026-08-12. La sezione resta come
**indice**: il contenuto vive nel Decision Log e nell'owner
[`spec-radar-profilo-personaggio.md`](characters/spec-radar-profilo-personaggio.md).

> ⚠️ **`RAD-1` era mal posta, e il repository aveva già risposto.** Chiedeva quale dei due workbook fosse
> autorità sui rating `*_1_10`. La risposta è **nessuno dei due**: [D-023](decisions/RT_PDR_00_Decision_Log.md)
> aveva già declassato `RefactorTactics_Balance_Matrices_v0.1.xlsx` a `RESEARCH` e spostato l'autorità dei
> numeri sui cataloghi `balance/RT_*Catalog_v0.1.md`, e [`balance/README.md`](balance/README.md) vieta perfino
> la riparazione che stavo per proporre — *«non correggerlo cella per cella: un workbook rattoppato
> diventerebbe una falsa fonte corrente»*. Il conflitto non è stato risolto: **si è dissolto**.

| Era | Decisione presa | Dove vive ora |
|---|---|---|
| ~~`RAD-1`~~ | I rating **non si scrivono da nessuna parte: si calcolano** dai cataloghi a ogni generazione. Nessun file di rating esiste, quindi nessuna seconda fonte può nascere né divergere | [`D-106`](decisions/RT_PDR_00_Decision_Log.md) · owner §4 |
| ~~`RAD-2`~~ | La rubrica è **codice**, non una tabella compilata: una formula si rivede in PR, un numero copiato in un foglio no. ⚠️ Diventa il **prerequisito di tutto** — senza rubrica i rating non esistono affatto, e non c'è il ripiego «intanto li mettiamo a mano» | [`D-106`](decisions/RT_PDR_00_Decision_Log.md) · owner §4.3, §7 |
| ~~`RAD-3`~~ | I sei assi si **modellano**, non si riduce il radar a ciò che era già derivabile. ⚠️ `information` nasce con un solo ingrediente — la **Vista** — perché stealth e detection vivono solo nel workbook che D-106 esclude; si arricchisce derivando il rumore dal **kit** ([D-042](decisions/RT_PDR_00_Decision_Log.md)), non ripescando il foglio | [`D-107`](decisions/RT_PDR_00_Decision_Log.md) · owner §5, §5.1 |
| ~~`RAD-4`~~ | Il generatore è **Node/TypeScript**, contro la raccomandazione registrata nel consolidamento (Python accanto a `scripts/`). Costo accettato: package manager e build step in un repository che li aveva evitati | [`D-108`](decisions/RT_PDR_00_Decision_Log.md) · owner §8 |
| ~~`RAD-5`~~ | Gli SVG si **committano**, e il `--check` che li verifica entra **nell'MVP**. ⚠️ Combinato con D-106 il gate diventa un **test di regressione sui dati competitivi**: cambiare `Salute` in un catalogo lo fa diventare rosso finché i grafici non sono rigenerati nello stesso commit | [`D-108`](decisions/RT_PDR_00_Decision_Log.md) · owner §8.2 |

---

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

> ✅ **Il resto di `PER-2` chiuso il 2026-08-11** con [`D-091`](decisions/RT_PDR_00_Decision_Log.md),
> implementando CP 13.3. Restavano **due** superfici della v0.1 senza riga nel workbook — `Rough` e
> `Conductive` — e implementando e' emerso un **terzo** valore che nessuno aveva contato: l'attenuazione per
> **arco**, che la DoD del checkpoint mette nella formula ma che nessun documento normativo quantificava.
> Decisi rispettivamente **`+1`**, **`0`** e **`2`**.
>
> Nota di metodo: il terzo si e' visto solo scrivendo il test che doveva pinnarlo. Una formula puo' nominare
> un termine — *«Intensita' − costo acustico − **occlusione**»* — senza che nessuno si accorga che quel
> termine non ha un numero, finche' qualcuno non deve scriverlo.

## Aperte — livello regole

| Tema | Stato |
|---|---|
| **Il bot dichiara condizioni sulle reazioni?** ([`#657`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/657)) | ⏳ **aperta il 2026-08-12**, e non blocca un checkpoint: blocca una **misura**. [D-109](decisions/RT_PDR_00_Decision_Log.md) ha reso la condizione dichiarata il mitigatore di pacing del regime `Conditional`, e il giocatore ha ora un modo di dichiararla (PR #639). Il bot no — quindi in 2v2 offline **meta' dei trigger resta senza mitigazione per costruzione**. Se il primo campione di pacing si raccoglie cosi', misura un gioco piu' lento di quello che la v0.1 consegna, e quel numero diventa la baseline che nessuno rimette in discussione. Le righe KPI toccate sono «Decisione del giocatore (p90)», «Durata playback per round» e la banda «round per partita». ⚠️ **Va decisa prima di raccogliere il campione**: deciderla dopo significa scegliere la regola che giustifica il numero gia' ottenuto |
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
| Effetto esatto di `Brace`, numeri `Sneak/Move/Sprint` | non definiti — ⚠️ `Move` **5 MP**, `Sprint` **8** e `Withdraw` **2** sono invece **in vigore** a catalogo: l'indefinito è il solo `Sneak` (`AE-5`) | [`gameplay/brief-azioni-generiche-overwatch.md`](gameplay/brief-azioni-generiche-overwatch.md) · [`balance/RT_ActionCatalog_v0.1.md`](balance/RT_ActionCatalog_v0.1.md) §2.1 |
| `RoundLimit` 2v2 **10–14**, 3v3 **16–20** | bande, non costanti | [D-010](decisions/RT_PDR_00_Decision_Log.md) · [`gameplay/spec-durata-partita-e-scala-mappe.md`](gameplay/spec-durata-partita-e-scala-mappe.md) |
| Durata della resolution con 1/2/3 unità armate | **mai misurata**; soglia d'allarme 20 s | CP 14.5 |
| Grammatica `STAND · READ · SHIFT` del Reaction Clash | **`PROPOSED FOR PLAYTEST`**, non canonica ([D-049](decisions/RT_PDR_00_Decision_Log.md)) | [`gameplay/spec-reaction-clash-e14.md`](gameplay/spec-reaction-clash-e14.md) §4 |
| Payoff `Win/Tie/Lose` delle maneuver dei 4 eroi | **inesistenti**: i nomi nella spec sono fixture, nessun dato d'eroe li contiene | idem §5 |
| Costo di un Clash: `Charges` proprio o quello della reaction | non deciso | idem §14 (`CLASH-3`) |

> **Il Reaction Clash incontra due domande già aperte, e non le risolve.** `FAC-3` (*`Brace` diventa
> direzionale?*) e `FAC-5` (*una reazione può ruotare chi reagisce?*) tornano rilevanti perché maneuver come
> `Pivot Step` e `Sidestep` presuppongono la risposta «sì». Restano `PROPOSED` finché quelle due non sono
> decise — vedi la §6 della spec, che lo dichiara invece di risolverle di lato.
