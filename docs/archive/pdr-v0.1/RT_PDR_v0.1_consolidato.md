# RefactorTactics — Corpus PDR `v0.1`, consolidato in Markdown

> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> ⚠️ **Archivio — materiale NON autorevole.** Livello 9 della gerarchia in
> [`../../README.md`](../../README.md). Questo documento **non** descrive il gioco di oggi: è il corpus
> documentale del **3 agosto 2026**, scritto prima del pivot esagonale, prima del modello azioni v0.1 e prima
> che il no-GAS diventasse una decisione. Serve a ricostruire **da dove viene** una regola, mai a stabilirla.
>
> Le fonti vincolanti sono [`../../product/piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md),
> [`../../decisions/`](../../decisions/) e i cataloghi di [`../../balance/`](../../balance/README.md).

## Perché questo file esiste

PDR-00 §6 regola di manutenzione **#5** dice: *«I PDF sono snapshot di consultazione: le sorgenti testuali
devono vivere nel repository Git.»* È la regola registrata come
[**D-009**](../../decisions/RT_PDR_00_Decision_Log.md), e fino al 2026-08-12 era stata applicata a **due**
pezzi soli — il [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md) (PDR-00 §4) e
[PDR-10 v0.2](../../roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md). Per gli altri undici documenti il PDF **era
l'unica copia**: dodici PDF binari, nessun testo diffabile, nessun `grep` possibile.

Questo file chiude D-009 per l'intero corpus. Il testo è **estratto dai PDF, non riscritto**: mantiene errori,
ripetizioni e tabelle spezzate dell'originale — sono parte della prova di provenienza. I PDF sono stati rimossi
dal working tree e restano nella storia Git (vedi [Recuperare i PDF](#recuperare-i-pdf-originali)).

## Cosa vale oggi, documento per documento

Non «cosa dice»: **cosa ne è rimasto**. La colonna a destra è il verdetto contro il canone corrente.

| PDR | Cosa contiene | Stato rispetto al canone |
|---|---|---|
| **00** Indice e governance | Mappa degli undici documenti, decision log D-001…D-008, glossario operativo, 6 regole di manutenzione | **Già recepito.** Il Decision Log vive in [`../../decisions/RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md) con la **stessa numerazione**, proseguita fino a oggi. Il glossario (Intento · Preview · Commit · Snapshot · Resolution · TurnLog · Cella) è ancora il vocabolario del progetto |
| **01** Visione e game design | 3v3, 20-30 min, max 12 turni, cinque pilastri, scope/non-scope, sei metriche di successo | **Superato in parte.** Il formato principale **non è deciso** ([D-011](../../decisions/RT_PDR_00_Decision_Log.md)) e il «massimo 12 turni» è diventato `RoundLimit`, parametro di formato ([D-010](../../decisions/RT_PDR_00_Decision_Log.md)). Pilastri e metriche restano leggibili come intenzione |
| **02** Turni simultanei | Macchina a stati del turno, planning privato, preview 8-12 Hz, Ready/commit, snapshot immutabile, sei fasi di resolution, stati UI, edge case | **Nucleo canonico.** Preview non-reliable + commit reliable + snapshot che chiude gli input sono gli invarianti #3/#5/#6. Le sei fasi `0…5` **non** sono le macro-fasi: sono un attributo dell'azione rimappato su `Prep → Dash → Blast → Move` ([ADR-0003](../../decisions/adr-0003-modello-azioni-v01.md) §3) |
| **03** Architettura UE5 | Moduli C++, Gameplay Framework, struttura `Source/` e `Content/`, plugin e `Build.cs`, setup Fondazioni | **Struttura sì, nomi no.** `ARTGameState`, `URTPathfindingService`, `URTAbilityDefinition` e compagnia **non esistono nel codice**: l'inventario vivo è [`../../technical/architettura-codice.md`](../../technical/architettura-codice.md). Restano validi il partizionamento per dominio e la regola che il grafo delle dipendenze punti al core. `Content/` è cambiato: feature-first sotto `/Game/RT` |
| **04** Networking e privacy 🟢[#589](https://github.com/DegrassiAaron/refactor-tactics-main/issues/589) | Threat model a cinque leak, classificazione dati a cinque classi, flusso preview, sanitizzazione a whitelist, rate limit, **test privacy con canary ID** | 🟢 **Il più recuperabile del corpus.** M10 (Rete e privacy) non è ancora costruita e l'invariante #6 dichiara *cosa* garantire, non *come* verificarlo. Il canary ID, il «PIE da solo non è sufficiente, serve packaged» e il test che fallisce se un tipo server-only acquisisce `replicated` sono procedure che oggi non esistono da nessuna parte |
| **05** Simulazione deterministica 🟢[#578](https://github.com/DegrassiAaron/refactor-tactics-main/issues/578) | Schema `FRTTurnSnapshot`, ordine stabile, seed per stream nominati, event model, replay, hash, **sei test di determinismo** | **Canone nel principio** (invariante #4), più specifico nella verifica. `Hash(TurnSeed, ActionId, RollKind)` — «aggiungere un VFX casuale non sposta le estrazioni di hit/crit» — è una regola operativa che il canone non scrive. I sei test (golden · permutation · repeat ×1000 · seed · frame-rate · packaged) sono un DoD pronto |
| **06** Mappa e pathfinding | `FRTCellId`, cell data, archi come dati di prima classe, A\* con `FGraphAStar`, cost model, cache per revisione, LOS/targeting separati | ⚠️ **Una trappola di naming.** Il `FRTCellId{X, Y, Layer}` di questo PDR è **planare quadrato**; il tipo omonimo di oggi è **assiale** `{X=q, Y=r, Layer}` ([ADR-0002](../../decisions/adr-0002-griglia-esagonale.md)). Stesso nome, semantica diversa. Il resto — archi come dati, costi interi, cache invalidata da `GraphRevision`, path/LOS/targeting come servizi distinti — è canone |
| **07** Abilità, personaggi e GAS | Confine GAS/resolver, schema ability definition, quattro targeting policy, roster MARA/IVO/NYX/SOL, combo ambientali, bilanciamento | **Superato nell'impianto.** Niente GAS nella v0.1; il roster è **Gadget · Phase · Riktor · Wraith**. Sopravvivono le **quattro moving-target policy** (`LockCell · TrackUnit · Retarget · Fizzle`), che sono nel catalogo azioni, e la regola «potere = azione + setup + rischio, non danno grezzo» |
| **08** UI/UX e coordinazione | Camera, planning HUD, intenti alleati, **warning model**, Confermato/Previsto/Incerto, playback, combat log | **Misto.** La tripartizione Confermato/Previsto/Incerto è recepita. Il **warning model** — sei warning con severità `Warning`/`Error`/`Block`/`Info` — non ha un owner documentale: è recuperabile per M8 |
| **09** Dati, validazione e modding | Primary Data Assets, ID stabili e versioni, Gameplay Tags per dominio, manifest + hash, **validator a dieci regole**, percorso modding | **Fondamenta canoniche** (`URTActionData`/`URTHeroData`/`URTEquipmentData`), verifica no. Le dieci regole del validator e il `ContentManifestHash` bloccato prima del match non hanno owner. §10 prescrive `/Docs/PDR/*.md` come sorgente e i PDF come artefatto di release: **è esattamente ciò che questo commit fa** |
| **10** Roadmap, QA e rischi | Fasi F0-F6, acceptance criteria, test pyramid, performance budget, risk register, DoD | **Ha già una v0.2 canonica**: [`../../roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md`](../../roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md). Qui resta lo snapshot v0.1 per confronto |
| **11** Demo | Scenario 2v2 di **otto turni completo**: mappa esagonale con coordinate, quattro schede personaggio, formato intent JSON, `ExpectedTurnLog` turno per turno, undici assert | 🟢 **Recuperabile nella forma, non nei contenuti.** Roster Aegis/Nyx/Drift/Vex mai adottato, ma la struttura *intent → ExpectedTurnLog → assert* è **letteralmente** quella del [Scenario Test Harness](../../technical/test-automatico-unreal.md). È l'antenato dello showcase [Relay Basin](../../src/showcase/relay-v0.1-scenario-spec.md): stesso relay neutrale al centro, stesso generatore a Est |
| **12** Catalog | Azioni, fasi, movimento, targeting, fallback, reazioni e stati, otto terreni, coperture, equipaggiamenti, eroi e loadout, matrice di test | **Antenato diretto** di `catalogo-e-bilanciamento-v0.1` e quindi dei [cataloghi di bilanciamento](../../balance/README.md), che lo **superano sui numeri**. Nomina già `Riktor` — è qui che il roster corrente comincia a esistere |

### Le tre cose da non prendere alla lettera

1. **`FRTCellId` di PDR-06 è quadrato.** Lo stesso identificatore oggi significa una coordinata assiale. Un
   `grep` sul nome non distingue i due mondi.
2. **GAS.** PDR-03, PDR-07 e PDR-09 lo danno per acquisito. La v0.1 **non lo usa**: `URTActionData` /
   `URTHeroData` / `URTEquipmentData`, nessun `GameplayAbilities` nel `Build.cs`.
3. **I codici di fase `0/10/20/30/40/50/60`** di PDR-02 e PDR-12 non sostituiscono
   `Planning → Prep → Dash → Blast → Move → Cleanup`. In particolare il **Move resta dopo il Blast**.

### Rumore di estrazione, lasciato dov'è

PDR-11 e PDR-12 portano marcatori di citazione della forma `【38†L170-L178】`: sono residui del generatore che
li ha prodotti e **non puntano a nulla**. Restano nel testo perché dicono qualcosa di vero sulla provenienza di
quei due documenti. Le tabelle molto larghe dell'originale a volte si spezzano su più righe: dove il PDF
impaginava su due colonne, l'estrazione non può ricomporre.

## Recuperare i PDF originali

I tredici PDF sono stati rimossi dal working tree, non dalla storia:

```bash
P=docs/archive/pdr-v0.1/RT_PDR_04_Networking_Privacy_v0.1.pdf
git show "$(git rev-list -1 HEAD -- "$P")^:$P" > RT_PDR_04.pdf
```

Il comando funziona senza conoscere lo SHA: `rev-list -1` trova il commit che ha cancellato il file, `^` ne
prende il genitore.

---

## PDR-00 — Indice, governance e decision log

> Il **Decision Log** di §4 non vive più qui: la sua sorgente canonica è
> [`../../decisions/RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md), che ne
> trascrive le otto voci e vi aggiunge tutte le decisioni successive. La tabella qui sotto è lo snapshot
> del 3 agosto 2026.

**REFACTORTACTICS Indice, governance e decision log** Mappa ufficiale della documentazione consolidata

Documento: PDR-00 Versione: 0.1 - Consolidato iniziale Data: 3 agosto 2026 Baseline tecnica: Unreal Engine 5.8 Documento guida del corpus PDR. Non sostituisce i documenti specialistici.

### Controllo del documento
|**Campo**|**Valore**|
|---|---|
|Stato|Bozza consolidata per revisione tecnica|
|Versione|0.1 - Consolidato iniziale|
|Data|3 agosto 2026|
|Autore|RefactorTactics / documentazione consolidata con supporto AI|
|Regola di prevalenza|Decisioni esplicite del progetto > requisiti consolidati > proposte PDR > ricerca web di supporto|

#### Sommario esecutivo
Il corpus è diviso in undici PDR che coprono prodotto, turni simultanei, architettura UE5, rete, simulazione, mappa, abilità, UI, dati e roadmap.

##### Decisioni consolidate

Formato 3v3, vertical slice 2v2, server authoritative, C++ core, Blueprint presentation, privacy team-only e determinismo sono invarianti di progetto.

Assunzioni operative

Baseline UE 5.8 per la documentazione; il repository dovrà bloccare una patch esatta prima dell’implementazione.

#### Indice del documento
1. Scopo del corpus documentale

2. Mappa dei documenti

3. Principi di consolidamento

4. Decision log

5. Glossario

6. Regole di manutenzione

### 1. Scopo del corpus documentale
Questo corpus trasforma il brief, le decisioni tecniche e le conversazioni disponibili in un set di PDR modulari. Ogni documento è leggibile da solo, ma usa gli stessi termini, gli stessi target e la stessa gerarchia decisionale.

L’obiettivo non è produrre una descrizione generica di un gioco tattico, ma una baseline operativa per il vertical slice: ciò che è in scope, chi possiede l’autorità, quali dati possono circolare in rete, come viene risolto un turno e quali prove rendono una feature realmente completata.

### 2. Mappa dei documenti
|**Codice**|**Documento**|**Scopo primario**|**Dipendenze**|
|---|---|---|---|
|PDR-00|Indice e governance|Navigazione, decision log, glossario|Nessuna|
|PDR-01|Visione e game design|Prodotto, pubblico, pilastri e vertical slice|PDR-00|
|PDR-02|Turno simultaneo|Planning, commit, snapshot, resolution|PDR-01|
|PDR-03|Architettura UE5|Moduli, classi, cartelle, dipendenze|PDR-01/02|
|PDR-04|Networking e privacy|Authority, RPC, team-only, anti-leak|PDR-02/03|
|PDR-05|Simulazione deterministica|Stato, seed, resolver, log e replay|PDR-02/03|
|PDR-06|Mappa e pathfinding|Grafo 3D, celle, A*, LOS e cache|PDR-03/05|
|PDR-07|Personaggi e abilità|Kit, GAS, targeting, combo, bilanciamento|PDR-01/05/06|
|PDR-08|UI/UX e coordinazione|Camera, preview, warning, explainability|PDR-02/04/06|
|PDR-09|Dati e validazione|Asset Manager, ID, hash, validator, mod future|PDR-03/07|
|PDR-10|Roadmap, QA e rischi|Milestone, test, KPI e Definition of Done|Tutti|

### 3. Principi di consolidamento
- Una sola fonte logica per concetto. I dettagli ripetuti vengono definiti nel documento owner e referenziati altrove.

- Stato delle decisioni visibile. Consolidato, assunzione, proposta e open question non vengono mescolati.

- Nessuna API inventata. Le API Unreal citate sono verificate contro la documentazione UE 5.8; le integrazioni progettuali sono comunque da compilare e testare nel branch milestone.

- Privacy come requisito di architettura. Il planning avversario non può essere “nascosto dalla UI”: non deve proprio raggiungere la connessione nemica.

- Determinismo verificabile. Stesso snapshot, regole, versione e seed devono produrre lo stesso TurnLog.

### 4. Decision log
|**ID**|**Decisione**|**Stato**|**Impatto**|
|---|---|---|---|
|D-001|Formato principale 3v3; vertical slice 2v2|Consolidata|Scope, UI, rete,<br>bilanciamento|
|D-002|Massimo 12 turni; planning 30 s; resolution 6-12 s|Consolidata|Tempo partita e UX|
|D-003|Server authoritative; client propone|Consolidata|Rete, validazione, anti-cheat|
|D-004|C++ per simulazione/rete; Blueprint per contenuti e presentazione|Consolidata|Ownership del codice|
|D-005|GAS non è l’autorità del simulatore|Consolidata|Confine abilities/resolver|
|D-006|Mappa come grafo tattico 3D con FRTCellId|Consolidata|Pathfinding, targeting,<br>ambienti|
|D-007|UE 5.8 baseline per questa edizione PDR|Assunzione da bloccare|Build, API, toolchain|
|D-008|Gameplay Framework legacy replication come primo target; Iris<br>valutato dopo vertical slice|Proposta semplice|Riduce rischio iniziale|

### 5. Glossario
|**Termine**|**Definizione operativa**|
|---|---|
|Intento|Piano proposto da un giocatore per una unità: percorso, destinazione, abilità, bersaglio, AoE,<br>direzione, label e stato Ready.|
|Preview|Aggiornamento frequente e non definitivo dell’intento, condiviso solo con server e alleati.|
|Commit|Invio reliable della versione finale dell’intento per il turno.|
|Snapshot|Copia immutabile dello stato autorevole usata per tutta la resolution.|
|Resolution|Esecuzione deterministica, per fasi e micro-step, degli intenti validati.|
|TurnLog|Sequenza ordinata e versionata degli eventi prodotti dal resolver.|
|Cella|Posizione logica compatta del grafo, identificata da X, Y e Layer.|
|Pubblico / team-only / server-only|Classificazione minima dei dati per stabilire quali connessioni possono riceverli.|

### 6. Regole di manutenzione
- 1 Ogni modifica a un requisito aggiunge o aggiorna una voce nel Decision Log.

- 2 Ogni modifica alle regole classificate incrementa RulesVersion e invalida replay incompatibili.

- 3 Il documento owner viene aggiornato per primo; gli altri mantengono solo riferimenti sintetici.

- 4 Una milestone non può essere chiusa senza test packaged, test rete e verifica assenza leak.

- 5 I PDF sono snapshot di consultazione: le sorgenti testuali devono vivere nel repository Git.

---

## PDR-01 — Visione di prodotto e game design

**REFACTORTACTICS Visione di prodotto e game design** Pilastri, formato partita e perimetro del vertical slice

Documento: PDR-01 Versione: 0.1 - Consolidato iniziale Data: 3 agosto 2026 Baseline tecnica: Unreal Engine 5.8

Documento di prodotto per allineare game design, engineering e produzione.

### Controllo del documento
|**Campo**|**Valore**|
|---|---|
|Stato|Bozza consolidata per revisione tecnica|
|Versione|0.1 - Consolidato iniziale|
|Data|3 agosto 2026|
|Autore|RefactorTactics / documentazione consolidata con supporto AI|
|Regola di prevalenza|Decisioni esplicite del progetto > requisiti consolidati > proposte PDR > ricerca web di supporto|

#### Sommario esecutivo
RefactorTactics è un tattico competitivo 3v3, PC-first, con planning simultaneo, mappa attiva e risoluzione server-authoritative. Il vertical slice 2v2 concentra la prova su coordinazione, privacy, determinismo e leggibilità.

##### Decisioni consolidate

Partite da 20-30 minuti, massimo 12 turni, planning 30 s, resolution 6-12 s; progressione orizzontale e contenuti originali.

##### Assunzioni operative

Obiettivo dinamico e roster proposto devono essere validati con playtest; nessun sistema di matchmaking o progressione account nella milestone Fondazioni.

#### Indice del documento
1. Visione di prodotto

2. Pilastri e principi

3. Formato partita

4. Vertical slice

5. Mappa come sistema

6. Progressione e competitività

7. Scope e non-scope

8. Metriche di successo

### 1. Visione di prodotto
RefactorTactics è un tattico competitivo PC-first basato su turni simultanei. Due squadre pianificano in parallelo, condividono il piano internamente e scoprono l’esito quando il server risolve uno snapshot comune. Il valore distintivo non è solo “indovinare la mossa avversaria”, ma leggere uno spazio multilivello che può cambiare durante il turno.

##### Posizionamento

Ispirazione nei principi generali dei turni simultanei, ma personaggi, mappe, abilità, UI, lore e asset devono essere originali. Nessuna dipendenza da IP, nomi o contenuti di Atlas Reactor.

### 2. Pilastri e principi
|**Pilastro**|**Promessa al giocatore**|**Conseguenza tecnica**|
|---|---|---|
|Mappa strategica|Quota, coperture, visibilità e ambienti generano<br>scelte reali.|Grafo 3D, servizi separati per path/LOS/targeting,<br>stato ambientale.|
|Simultaneità leggibile|Tutti pianificano sullo stesso intervallo; l’esito è<br>spiegabile.|Snapshot immutabile, ordine data-driven, TurnLog.|
|Coordinazione sicura|Gli alleati vedono il piano; i nemici no.|Relay team-only, nessun Actor globale con intenti.|
|Roster modulare|Ogni personaggio resta riconoscibile ma<br>supporta varianti laterali.|Data Assets versionati, trade-off, niente upgrade<br>puri.|
|Riproducibilità|Stesso input produce stesso risultato.|ID stabili, seed per turno, interi/fixed-point,<br>ordinamenti espliciti.|

### 3. Formato partita
|**Parametro**|**Target consolidato**|**Nota di design**|
|---|---|---|
|Formato principale|3v3|Sei giocatori o combinazioni umano/bot.|
|Durata|20-30 minuti|Ridurre tempi morti tramite planning simultaneo.|
|Turni|Massimo 12|La pressione deve aumentare verso la conclusione.|
|Planning|30 secondi|Countdown annullabile finché tutti non sono Ready.|
|Resolution|6-12 secondi|Animazione breve, leggibile, non autoritativa.|
|Obiettivo|Dinamico|Deve forzare rotazioni e uso della mappa.|

### 4. Vertical slice
Il vertical slice dimostra il rischio principale del progetto: che planning privato, risoluzione deterministica e mappa attiva funzionino insieme in una partita comprensibile.

- 2v2, per ridurre il numero di combinazioni durante la prima validazione.

- Una mappa multilivello con almeno ponte o tetto, tunnel, porte e un’interazione ambientale significativa.

- Quattro personaggi con quattro abilità ciascuno e ruoli complementari.

- Un obiettivo dinamico che cambia posizione o valore durante la partita.

- Planning alleato visibile con path, destinazione, target, AoE, direzione, label e Ready.

- Replay di turno basato sul TurnLog, non sulle animazioni registrate.

### 5. Mappa come sistema
La mappa non è uno sfondo. Ogni cella e transizione può incidere su movimento, attacchi e reazioni. Il design deve valorizzare combinazioni emergenti ma controllabili: acqua conduce elettricità, fuoco altera aree o visibilità, porte e ponti cambiano il grafo, ascensori e tunnel creano timing e rischio.

|**Elemento**|**Decisione tattica**|**Output del simulatore**|
|---|---|---|
|Quota|Vantaggio di linea, accesso, esposizione|Modificatori di LOS/targeting e transizioni<br>consentite|
|Copertura direzionale|Da quale lato ingaggiare|Riduzione/blocco secondo direzione dell’attacco|
|Acqua|Zona di rischio e combo|Stato bagnato, propagazione elettrica|
|Fuoco|Controllo area e trasformazione|Hazard, danno/status, modifica temporanea celle|
|Porte/ponti|Controllo delle rotte|Archi abilitati/disabilitati e revisione grafo|
|Tunnel/ascensori|Mobilità multilivello|Transizioni speciali con costo e regole|

### 6. Progressione e competitività
La progressione è orizzontale: il giocatore amplia possibilità e identità, non accumula potenza obbligatoria. Varianti, talenti, gadget, specializzazioni, tratti e affinità ambientali devono cambiare il profilo di rischio.

##### Regola di bilanciamento

Ogni variante deve dichiarare esplicitamente cosa guadagna e cosa perde. Un’opzione che conserva tutto il kit base e aggiunge solo vantaggi è fuori standard.

### 7. Scope e non-scope
|**In scope Fondazioni / vertical slice**|**Fuori scope iniziale**|
|---|---|
|Progetto C++ compilabile, sandbox, camera, griglia, A*, unità,|Matchmaking, progressione account, monetizzazione, modding|
|planning, Ready, snapshot, movimento, TurnLog e test.|pubblico, backend live.|
|Listen server per test e progettazione server-authoritative.|Infrastruttura produzione completa e anti-cheat esterno.|
|Data-driven fin dal primo giorno.|Editor pubblico di mod o codice nativo non fidato.|

### 8. Metriche di successo
- 60 FPS client sul target PC del vertical slice.

- Path query mediana inferiore a 2 ms; preview completa inferiore a 50 ms.

- Resolution server inferiore a 100 ms per match MVP, esclusa presentazione.

- Zero divergenze del replay sul corpus di test.

- Zero pacchetti contenenti intenti avversari nelle catture di rete.

- Partite interne completabili senza spiegazione verbale dell’ordine di resolution.

---

## PDR-02 — Gameplay loop e turni simultanei

**REFACTORTACTICS Gameplay loop e turni simultanei** Planning, commit, snapshot e resolution deterministica

Documento: PDR-02 Versione: 0.1 - Consolidato iniziale Data: 3 agosto 2026 Baseline tecnica: Unreal Engine 5.8 Specifica funzionale del ciclo autorevole del turno.

### Controllo del documento
|**Campo**|**Valore**|
|---|---|
|Stato|Bozza consolidata per revisione tecnica|
|Versione|0.1 - Consolidato iniziale|
|Data|3 agosto 2026|
|Autore|RefactorTactics / documentazione consolidata con supporto AI|
|Regola di prevalenza|Decisioni esplicite del progetto > requisiti consolidati > proposte PDR > ricerca web di supporto|

#### Sommario esecutivo
Il turno è una macchina a stati server-controlled: planning privato, preview team-only, Ready/unready, commit reliable, validazione, snapshot immutabile, resolution per fasi, cleanup e broadcast.

##### Decisioni consolidate

Preview 8-12 Hz sequenziata e non reliable; Ready/commit reliable; lo snapshot chiude definitivamente gli input del turno.

##### Assunzioni operative

La durata esatta del countdown finale, le politiche di collisione e le fallback su disconnect sono proposte da validare in playtest.

#### Indice del documento
1. Macchina a stati del turno

2. Planning privato

3. Preview e sequencing

4. Ready e commit

5. Validazione

6. Snapshot

7. Resolution

8. Cleanup e broadcast

9. Stati UI

10. Edge case

### 1. Macchina a stati del turno
```
TurnStart
   |
   v
PlanningOpen -----> CountdownArmed
   |  ^                  |
   |  | Unready          | tutti Ready / timeout
   |  +------------------+
   v
```

```
CommitClosed -> Validate -> SnapshotBuilt -> Resolving -> Cleanup -> TurnStart/MatchEnd
```

Le transizioni sono controllate dal server. Il client può proporre Ready/unready e inviare intenti, ma non decide l’inizio della resolution.

### 2. Planning privato
Ogni giocatore pianifica per le unità controllate. Il server mantiene il CanonicalIntentStore completo. Gli alleati ricevono una rappresentazione sanitizzata sufficiente alla coordinazione; gli avversari non ricevono alcun dato di planning.

|**Campo intento**|**Preview team**|**Commit server**|**Pubblico**|
|---|---|---|---|
|UnitId|Sì|Sì|Solo dopo resolution, se necessario|
|Path / destination|Sì|Sì|No durante planning|
|AbilityId / target|Sì|Sì|No durante planning|
|AoE / direction|Sì|Sì|No durante planning|
|Label / ping|Sì|Opzionale|No|
|Sequence / revision|Sì|Sì|No|
|Ready|Sì per squadra|Sì|Solo stato aggregato opzionale|

### 3. Preview e sequencing
- Frequenza target: 8-12 Hz, adattabile a bandwidth e input change.

- Trasporto: aggiornamenti non reliable, perché una preview vecchia può essere sostituita dalla più recente.

- Ogni stream usa una sequence crescente per giocatore/unità; i pacchetti fuori ordine vengono scartati.

- Il server applica rate limit, verifica ownership e rimuove campi non consentiti prima del relay.

- La preview non modifica lo stato autorevole del turno e non può consumare risorse.

### 4. Ready e commit
Ready/unready è reliable. Quando tutti i membri attivi della squadra e della partita sono Ready, il server arma un countdown breve e annullabile. Un unready valido prima della chiusura riapre il planning. Alla scadenza, il server chiude gli ingressi e usa l’ultima revisione committata o una fallback policy esplicita.

##### Fallback proposta

Intento mancante o invalido: Hold/NoAction per l’unità, con evento di validazione nel TurnLog. Mai usare automaticamente una preview non committata senza dichiararlo nelle regole.

### 5. Validazione
- 1 Autenticare connessione, giocatore e ownership dell’unità.

- 2 Confermare fase, numero turno, revisioni e RulesVersion.

- 3 Risolvere AbilityId e riferimenti tramite cataloghi autorevoli.

- 4 Validare costi, cooldown, status, range, targeting e path rispetto allo stato pre-snapshot.

- 5 Normalizzare ordine e dati non canonici; rifiutare NaN, valori fuori limite e ID sconosciuti.

- 6 Produrre un esito strutturato: Accepted, Sanitized o Rejected con reason code.

### 6. Snapshot
Dopo la chiusura del commit il server costruisce una copia immutabile contenente mappa, unità, effetti, obiettivi, intenti accettati, regole, seed e revisioni. Nessun input tardivo può modificare lo snapshot in corso.

### 7. Resolution
|**Fase**|**Responsabilità**|**Esempi**|
|---|---|---|
|0. Effetti e reazioni|Preparare trigger già attivi|Counter armati, status all’inizio turno|
|1. Transizioni e movimento|Micro-step ordinati|Dash, porte, collisioni, swap consentiti|
|2. Controllo/difesa/interruzioni|Alterare o negare azioni|Guard, interrupt, displacement|
|3. Attacchi e abilità|Calcolare impatti|Lineare, AoE, projectile logico|
|4. Ambiente|Propagare hazard e modifiche|Acqua-elettricità, fuoco, archi disabilitati|
|5. Cleanup|KO, obiettivi, cooldown|Punteggio, scadenze, next turn|

L’ordine è data-driven, ma la playlist classificata usa un ruleset bloccato e hashato. All’interno di ogni fase si ordinano gli elementi con chiavi stabili, mai con iterazione casuale di mappe/hash.

### 8. Cleanup e broadcast
- Completare KO, obiettivi, cooldown, durate e modifiche del grafo.

- Persistenza del nuovo authoritative state e del TurnLog.

- Broadcast reliable dei risultati necessari; la presentazione ricostruisce animazioni dal log.

- Azzeramento di preview e ready, incremento TurnNumber, controllo MatchEnd.

### 9. Stati UI
|**Stato**|**Definizione**|**Esempio UI**|
|---|---|---|
|Confermato|Deriva da stato pubblico o regola deterministica senza<br>input nascosti.|Path valido, costo, cooldown proprio.|
|Previsto|Dipende dagli intenti della squadra e dallo stato noto.|Arrivo alleato, AoE pianificata.|
|Incerto|Dipende da intenti avversari, priorità o collisioni non<br>risolte.|Linea che può essere bloccata, cella<br>contesa.|

### 10. Edge case
|**Caso**|**Policy iniziale proposta**|
|---|---|
|Disconnect in planning|Mantieni ultimo commit valido; bot/hold secondo modalità.|
|Disconnect in resolution|La resolution continua dal server; il client recupera stato e log.|
|Due unità stessa cella|Resolver applica priorità/tie-break e produce Blocked/Displaced.|
|Target si sposta|Comportamento dichiarato per abilità: lock cell, track unit, retarget o fizzle.|
|Mappa cambia durante movimento|Le modifiche entrano nella fase stabilita; nessuna modifica retroattiva.|

---

## PDR-03 — Architettura software Unreal Engine 5

### REFACTORTACTICS Architettura software Unreal Engine 5
Moduli, classi, struttura progetto e milestone Fondazioni

Documento: PDR-03 Versione: 0.1 - Consolidato iniziale Data: 3 agosto 2026 Baseline tecnica: Unreal Engine 5.8

PDR tecnico per implementazione del vertical slice in UE 5.8.

#### Controllo del documento
|**Campo**|**Valore**|
|---|---|
|Stato|Bozza consolidata per revisione tecnica|
|Versione|0.1 - Consolidato iniziale|
|Data|3 agosto 2026|
|Autore|RefactorTactics / documentazione consolidata con supporto AI|
|Regola di prevalenza|Decisioni esplicite del progetto > requisiti consolidati > proposte PDR > ricerca web di supporto|

##### Sommario esecutivo
L’architettura separa stato logico, autorità e presentazione. Un game module iniziale contiene domini chiari; C++ governa ciò che è possibile, Data Assets e Blueprint selezionano varianti e presentazione.

###### Decisioni consolidate

Il resolver non dipende da frame, animazioni o UI. Le celle sono dati compatti; il GameState non ospita intenti privati.

Assunzioni operative

La baseline UE 5.8 e la scelta di mantenere un singolo module sono assunzioni pragmatiche; una patch precisa va bloccata nel repository.

##### Indice del documento
1. Baseline e principi

2. Diagramma architetturale

3. Moduli C++

4. Gameplay Framework

5. Struttura Source

6. Struttura Content

7. Plugin e Build.cs

8. Dipendenze

9. Setup Fondazioni

10. Commit proposto

#### 1. Baseline e principi
Baseline documentale: Unreal Engine 5.8. Il progetto deve bloccare una patch e toolchain esatte nel file README della milestone. La soluzione più semplice scalabile è un singolo game module iniziale, separato internamente per dominio; plugin di progetto solo quando un confine è stabile o riutilizzabile.

- C++: simulazione, rete, serializzazione, pathfinding, validazione, regole competitive e test core.

- Blueprint/Data: configurazione, UI, animazioni, VFX, prototipi e varianti di contenuto.

- GAS: abilità, costi, cooldown, attributi ed effetti; il resolver resta autorità.

- Actor: rappresentazione, collisioni e interazioni visibili; non una cella Actor per ogni nodo.

- Subsystem/servizi: sistemi senza presenza fisica e con ownership esplicita.

#### 2. Diagramma architetturale
|`Client Presentation                         Authoritative Server`|
|---|
|`+---------------------------+              +-----------------------------+`|
|`| UMG / ViewModels          |<--results----| GameMode / TurnManager      |`|
|`| Tactical Camera           |              | CanonicalIntentStore        |`|
|`| Ghost Path / AoE          |--proposal--->| Intent Validation           |`|
|`| Team Intent Relay View    |<--team only--| SnapshotBuilder             |`|
|`| Animation/VFX Playback    |<--TurnLog----| ActionResolver              |`|
|`+---------------------------+              | Map/Path/LOS/Targeting      |`<br>`+--------------+--------------+`<br>`|`<br>`+--------------v--------------+`<br>`| Data Assets / Rules / Tags   |`<br>`+-----------------------------+`|

#### 3. Moduli C++
|**Area**|**Classi/servizi iniziali**|**Responsabilità**|
|---|---|---|
|Framework|ARTGameMode, ARTGameState, ARTPlayerController,<br>ARTPlayerState|Lifecycle match, stato pubblico, RPC ownership,<br>identità team.|
|Unità|ARTTacticalUnit, URTHealthComponent,<br>URTStatusComponent|Rappresentazione unità e stato runtime.|
|Mappa|URTMapSubsystem, FRTMapState, FRTCellId|Lookup celle, revisioni, occupazione, transizioni.|
|Query|URTPathfindingService, URTLOSService,<br>URTTargetingService|Servizi separati e testabili.|
|Planning|URTPlanningComponent, FRTIntent, FRTIntentStore|Proposte client e store server.|
|Turno|URTTurnManager, FRTSnapshotBuilder,<br>FRTActionResolver|State machine, snapshot, resolution.|
|Log|FRTTurnLog, FRTTurnEvent|Replay, debug, explainability.|
|Dati|URTAbilityDefinition, URTCharacterDefinition,<br>URTRuleset|Contenuti versionati e validabili.|

#### 4. Gameplay Framework
Il GameMode esiste solo sul server e guida le regole; il GameState contiene esclusivamente lo stato che può essere replicato a tutti. Il PlayerController è il punto naturale per RPC dirette alla singola connessione, mentre il PlayerState conserva identità e dati pubblici persistenti nel match. Le unità possono essere Pawn o Actor controllati logicamente; nel vertical slice non serve forzare CharacterMovement.

#### 5. Struttura Source
```
Source/RefactorTactics/
  RefactorTactics.Build.cs
```

```
  Public/
    Core/          RTTypes.h, RTIds.h, RTRulesVersion.h
    Framework/     RTGameMode.h, RTGameState.h, RTPlayerController.h
    Map/           RTCellId.h, RTMapState.h, RTMapSubsystem.h
    Query/         RTPathfindingService.h, RTLOSService.h
    Planning/      RTIntent.h, RTPlanningComponent.h
    Turn/          RTTurnManager.h, RTSnapshot.h, RTActionResolver.h
    Unit/          RTTacticalUnit.h, RTStatusComponent.h
    Data/          RTAbilityDefinition.h, RTCharacterDefinition.h
    Log/           RTTurnLog.h
  Private/
    ... mirrored implementation folders ...
    Tests/         RTCellIdTests.cpp, RTPathTests.cpp, RTResolverTests.cpp
```

#### 6. Struttura Content
```
Content/RefactorTactics/
  Maps/Dev/L_DevSandbox
  Blueprints/Framework/
  Blueprints/Units/
  Data/Characters/
  Data/Abilities/
  Data/Rulesets/
  Input/IA_*, IMC_Tactical
  UI/HUD/, UI/Planning/, UI/Debug/
  Art/Graybox/, Art/Materials/
  VFX/, Audio/
```

```
  Tests/Functional/
```

#### 7. Plugin e Build.cs
|**Modulo/plugin**|**Milestone Fondazioni**|**Motivo**|
|---|---|---|
|Core, CoreUObject, Engine, InputCore|Required|Base runtime.|
|EnhancedInput|Required|Camera e azioni contestuali.|
|AIModule|Required|FGraphAStar / GraphAStar.h.|
|GameplayTags|Required|Vocabolario governato.|
|UMG, Slate, SlateCore|Required|HUD e debug UI.|
|GameplayAbilities, GameplayTasks|Dopo path/turn proof|GAS entra quando il resolver base è stabile.|
|FunctionalTesting|Dev/Test|Test di livello.|
|CommonUI|Dopo proof of concept|Riduce rischio di integrazione prematura.|

##### Build.cs indicativo
```
PublicDependencyModuleNames.AddRange(new[] {
```

```
  "Core", "CoreUObject", "Engine", "InputCore",
  "EnhancedInput", "AIModule", "GameplayTags"
```

- `});`

```
PrivateDependencyModuleNames.AddRange(new[] {
  "UMG", "Slate", "SlateCore"
```

- `});`

```
// Aggiungere GameplayAbilities e GameplayTasks nella milestone Abilities.
```

#### 8. Dipendenze
Il grafo delle dipendenze deve puntare verso il core logico: Presentation -> Planning/Queries -> Snapshot/Resolver -> Core Types/Data. Il resolver non dipende da UMG, animazioni o asset caricati in modo implicito. I dati runtime vengono materializzati in strutture compatte prima della resolution.

#### 9. Setup Fondazioni
- 1 Creare progetto Games > Blank > C++, Desktop, Maximum Quality, Starter Content off.

- 2 Impostare L_DevSandbox come Editor Startup Map e Game Default Map.

- 3 Creare ARTGameMode e ARTPlayerController; assegnarli tramite World Settings o DefaultGame.ini.

- 4 Abilitare Enhanced Input e creare IA_CameraPan, IA_CameraZoom, IA_CameraRotate, IA_Select, IA_Confirm, IA_Ready.

- 5 Creare graybox 2D generato da dati compatti e un overlay debug delle coordinate.

- 6 Aggiungere due unità, planning movimento locale, Ready simultaneo locale, snapshot e resolver movimento.

- 7 Abilitare Automation/Functional Testing solo nelle configurazioni appropriate e aggiungere test core.

#### 10. Commit proposto
```
feat(foundation): bootstrap tactical sandbox and deterministic move turn
```

- Progetto C++ e Build.cs

- L_DevSandbox e camera tattica

- FRTCellId, mappa graybox e primo A*

- Due unità, planning, Ready, snapshot, movement resolver

- TurnLog, debug overlay e Automation Tests

Fonti tecniche verificate: Epic Games, Gameplay Framework in Unreal Engine, UE 5.8.; Epic Games, Enhanced Input in Unreal Engine, UE 5.8.; Epic Games, FGraphAStar C++ API, UE 5.8.

---

## PDR-04 — Networking e privacy degli intenti

### REFACTORTACTICS Networking e privacy degli intenti
Authority, relay team-only, RPC e test anti-leak

Documento: PDR-04 Versione: 0.1 - Consolidato iniziale Data: 3 agosto 2026 Baseline tecnica: Unreal Engine 5.8

Specifica di sicurezza e trasporto per il planning competitivo.

#### Controllo del documento
|**Campo**|**Valore**|
|---|---|
|Stato|Bozza consolidata per revisione tecnica|
|Versione|0.1 - Consolidato iniziale|
|Data|3 agosto 2026|
|Autore|RefactorTactics / documentazione consolidata con supporto AI|
|Regola di prevalenza|Decisioni esplicite del progetto > requisiti consolidati > proposte PDR > ricerca web di supporto|

##### Sommario esecutivo
Il server conserva l’intento canonico; le preview vengono relayate solo alla squadra mediante DTO sanitizzati. Gli avversari non devono ricevere proprietà o RPC contenenti planning, nemmeno se la UI non lo mostra.

###### Decisioni consolidate

Preview unreliable e sequenziate; ready/commit reliable; risultati autorevoli pubblici dopo la resolution.

###### Assunzioni operative

La replication standard è proposta per il vertical slice; Iris viene valutato dopo la stabilizzazione del modello e dei test.

##### Indice del documento
1. Threat model

2. Classificazione dati

3. Topologia rete

4. Flusso preview

5. Commit e risultati

6. Ownership RPC

7. Sanitizzazione

8. Rate limit

9. Test privacy

10. Dedicated server path

#### 1. Threat model
L’avversario è un client legittimo ma non fidato: può ispezionare memoria, pacchetti, proprietà replicate, RPC ricevute e timing. La sicurezza richiesta non è segretezza crittografica contro il server, ma non consegna dei dati di planning alla connessione non autorizzata.

- Leak diretto: proprietà o RPC contengono path, target o AbilityId avversari.

- Leak indiretto: array globali con campi “hidden”, subobject replicati o log prematuri.

- Leak temporale: dimensione/frequenza pacchetti differente rivela cambi di piano sensibili.

- Abuso client: intenti per unità non possedute, spam preview, sequence rollback, payload fuori limite.

- Divergenza: client mostra un piano mai accettato o esito calcolato localmente come definitivo.

#### 2. Classificazione dati
|**Classe**|**Esempi**|**Destinatari**|
|---|---|---|
|Server-only|CanonicalIntentStore, validazione grezza, seed non ancora<br>pubblicabile|Solo server|
|Team-only|Preview alleati, label, ping tattico, Ready individuale|Connessioni della squadra|
|Owner-only|Errori dettagliati di validazione, UI input locale|Singola connessione|
|Public|TurnNumber, fase, tempo, obiettivo, risultati risolti|Tutti i client|
|Derived local|Ghost, warning, interpolazioni, animazioni|Solo client locale|

#### 3. Topologia rete
```
Client A1 --Server RPC--> PlayerController A1 --validate--> CanonicalIntentStore
```

```
                                              |
                                              +--Client RPC--> A1 (ack/error)
                                              +--Client RPC--> A2 (sanitized team preview)
                                              X-- no route --> B1/B2
```

```
After resolution:
ActionResolver -> Public TurnLog/Result DTO -> A1/A2/B1/B2
```

###### Divieto architetturale

Non memorizzare intenti completi in GameState, PlayerState replicati globalmente, componenti AlwaysRelevant o Actor globali. “Nascondere il widget” non è una misura di privacy.

#### 4. Flusso preview
- 1 Il client costruisce un FRTIntentPreviewDTO con UnitId, revisione e campi ammessi.

- 2 Invia ServerSubmitIntentPreview tramite il proprio PlayerController/owned component.

- 3 Il server valida ownership, fase, dimensione, sequence e rate limit.

- 4 Aggiorna lo store server e costruisce un TeamIntentPreviewDTO sanitizzato.

- 5 Invia ClientReceiveTeamIntentPreview solo ai PlayerController della stessa TeamId.

- 6 I client scartano sequence vecchie e aggiornano esclusivamente il view model.

#### 5. Commit e risultati
Il commit è reliable e idempotente rispetto a TurnId + PlayerId + Revision. Il server risponde con ack e reason code. I risultati della resolution sono reliable o rappresentati da stato replicato pubblico; il TurnLog può essere segmentato per playback ma non deve contenere campi privati non più necessari.

#### 6. Ownership RPC
Unreal usa un modello client-server server-authoritative; owner e owning connection determinano quale client può invocare remote function su un Actor. Per questo il PlayerController o un Actor posseduto è il punto di ingresso corretto per le proposte del client. Il server deve comunque rivalidare ogni campo: ownership RPC non equivale ad autorizzazione semantica.

#### 7. Sanitizzazione
|**Controllo**|**Azione**|
|---|---|
|Unit ownership|Reject se TeamId/Controller non corrisponde.|
|Turn/phase|Reject o ignore se stale/future.|
|Sequence|Ignore se <= LastAcceptedSequence.|
|Array length|Clamp/reject path, targets, points, labels.|
|IDs|Risoluzione contro catalogo; mai fidarsi di soft path client.|
|Coordinate|Normalizzare a FRTCellId valido e grafo corrente.|
|Text label|Lunghezza, charset, profanity policy eventuale.|
|Fields relay|Whitelist, non blacklist. Creare DTO distinto dal canonical.|

#### 8. Rate limit
- Token bucket per connessione e per unità, target 8-12 update/s con burst ridotto.

- Hard cap sulla dimensione serializzata e sul numero di punti path.

- Metriche server: accepted, dropped stale, rate-limited, invalid ownership, invalid ID.

- Ping e disegno tattico hanno canali e limiti separati; scadono automaticamente.

#### 9. Test privacy
- 1 Avviare dedicated/listen server con due squadre e catturare traffico per ogni client.

- 2 Inserire canary ID riconoscibili negli intenti Team A.

- 3 Assert che i canary non compaiano in RPC/proprietà/log ricevuti da Team B durante planning.

- 4 Provare relevancy, reconnect, late join, spectator e replay.

- 5 Eseguire test packaged; PIE da solo non è sufficiente.

- 6 Aggiungere un test che fallisce se un tipo server-only acquisisce proprietà replicated.

#### 10. Dedicated server path
Il listen server è utile per iterare, ma la produzione richiede dedicated server authoritative. La migrazione deve essere prevista evitando dipendenze da LocalPlayer sul server, input nel resolver o accesso a widget. Il primo proof di rete può usare la replication standard; Iris, production-ready in UE 5.8, va valutato solo dopo avere test privacy e modello dati stabili.

Fonti tecniche verificate: Epic Games, Networking Overview for Unreal Engine, UE 5.8.; Epic Games, Actor Owner and Owning Connection, UE 5.8.; Epic Games, Unreal Engine 5.8 Release Notes (Iris production-ready).

---

## PDR-05 — Simulatore deterministico, snapshot e TurnLog

**REFACTORTACTICS Simulatore deterministico, snapshot e TurnLog** Stato logico, resolver, seed, replay e verifica divergenze

Documento: PDR-05 Versione: 0.1 - Consolidato iniziale Data: 3 agosto 2026 Baseline tecnica: Unreal Engine 5.8

Specifica del nucleo autorevole indipendente dalla presentazione.

### Controllo del documento
|**Campo**|**Valore**|
|---|---|
|Stato|Bozza consolidata per revisione tecnica|
|Versione|0.1 - Consolidato iniziale|
|Data|3 agosto 2026|
|Autore|RefactorTactics / documentazione consolidata con supporto AI|
|Regola di prevalenza|Decisioni esplicite del progetto > requisiti consolidati > proposte PDR > ricerca web di supporto|

#### Sommario esecutivo
Il resolver usa uno snapshot immutabile e ordinamenti stabili per produrre un nuovo stato e un TurnLog canonico. La presentazione riproduce eventi, ma non decide collisioni, danni, reazioni o obiettivi.

##### Decisioni consolidate

Stesso snapshot, RulesVersion, ContentManifestHash, ResolverConfigHash e seed devono produrre gli stessi hash finali.

##### Assunzioni operative

I formati di serializzazione e hash esatti sono da scegliere durante Fondazioni; devono essere versionati prima dei replay persistenti.

#### Indice del documento
1. Obiettivi determinismo

2. Stato logico

3. Snapshot schema

4. Ordine stabile

5. Seed e random

6. Resolver

7. Event model

8. Replay

9. Hash e divergenze

10. Test

### 1. Obiettivi determinismo
Il determinismo richiesto è applicativo: stessa build/rules version, stesso snapshot e stesso seed devono produrre lo stesso stato finale e lo stesso log canonico. Non si richiede che ogni effetto visivo sia identico né che la fisica real-time decida risultati competitivi.

### 2. Stato logico
|**Dominio**|**Contenuto minimo**|**Vincoli**|
|---|---|---|
|MapState|Celle, archi, hazard, occupazione, revisioni|Nessun riferimento a componenti visuali.|
|Units|ID, cella, facing, HP, risorse, status|Ordinabili per StableUnitId.|
|Effects|ID, source, target, durata, stack|Ordine e trigger espliciti.|
|Objectives|stato, posizione, punteggio|Aggiornati in fase definita.|
|Turn|numero, fase, rules version|Monotono e serializzabile.|
|RNG|seed turno e stream ID|Nessun FMath::Rand globale.|
|Log|eventi, reason, before/after essenziali|Replay ed explainability.|

### 3. Snapshot schema
```
FRTTurnSnapshot
  RulesVersion
  ContentManifestHash
  MatchId / TurnNumber
  TurnSeed
  MapSnapshot
    GraphRevision, Cells[], Edges[], Hazards[]
  Units[]       // sorted StableUnitId
  Effects[]     // sorted Phase, Priority, EffectId
  Objectives[]
  AcceptedIntents[] // sorted Team, Player, Unit
  ResolverConfigHash
```

Lo snapshot è un value object logico. Può essere costruito da UObject/Actor, ma il resolver preferisce strutture pure e compatte per ridurre dipendenze e facilitare test headless.

### 4. Ordine stabile
- Non dipendere dall’ordine di TMap/TSet per risultati competitivi.

- Ordinare esplicitamente per fase, priorità, iniziativa se prevista e StableId come tie-break finale.

- Definire regole per conflitti di movimento, target multipli e reazioni simultanee.

- Registrare nel log la chiave di ordinamento o il reason code quando influisce sull’esito.

### 5. Seed e random
Ogni turno riceve un seed autorevole. Le estrazioni casuali devono usare stream nominati o derivati in modo stabile, ad esempio Hash(TurnSeed, ActionId, RollKind). In questo modo aggiungere un VFX casuale non sposta le estrazioni di hit/crit.

##### Regola

Le abilità competitive non accedono direttamente a random globale. Ogni roll significativo produce un evento con stream key, range e risultato.

### 6. Resolver
Il resolver consuma snapshot e intenti, produce eventi e un nuovo stato. L’implementazione iniziale può essere sincrona sul server; la separazione in fasi consente successiva parallelizzazione solo dove l’ordine osservabile resta invariato.

```
Validate snapshot invariants
```

- `-> Build ordered action queue`

- `-> For each phase`

- `-> Resolve triggers/reactions`

- `-> Resolve micro-steps or impacts`

- `-> Append canonical events`

- `-> Apply events to working state`

- `-> Finalize objectives/cooldowns/KO`

- `-> Compute StateHash + LogHash`

- `-> Publish result`

### 7. Event model
|**Evento**|**Dati essenziali**|
|---|---|
|MoveStarted|UnitId, from, intended destination|
|MoveStep|UnitId, from, to, step index, cost|
|MoveBlocked|UnitId, attempted cell, reason, blocker|
|AbilityDeclared|ActionId, source, AbilityId, target policy|
|DamageApplied|source, target, amount, damage type, modifiers|
|StatusChanged|target, StatusId, stacks/duration, reason|
|EnvironmentChanged|cell/edge, before, after, source|
|UnitKO|UnitId, source/reason|
|ObjectiveUpdated|ObjectiveId, before, after|

### 8. Replay
Il replay di turno ricostruisce la presentazione dal TurnLog e dallo stato iniziale. Per audit competitivo si conserva anche lo snapshot o un riferimento verificabile. Le animazioni sono consumatori del log: se un montage termina tardi, il risultato non cambia.

### 9. Hash e divergenze
- ContentManifestHash identifica definizioni usate dal match.

- ResolverConfigHash identifica ordine fasi e parametri classificati.

- StateHash normalizza e serializza campi logici in ordine stabile.

- LogHash copre gli eventi canonici, non timestamp real-time.

- In caso di mismatch il client scarta simulazioni locali e applica il risultato server; telemetry registra versione e hash.

### 10. Test
- 1 Golden test: snapshot fixture -> log e state hash attesi.

- 2 Permutation test: inserire unità/effects in ordine diverso e ottenere output uguale.

- 3 Repeat test: 1.000 esecuzioni con stesso seed, zero differenze.

- 4 Seed test: cambiare seed modifica solo eventi random correlati.

- 5 Frame-rate test: playback a 30/60/144 FPS non modifica stato finale.

- 6 Packaged server test: confrontare hash tra build Development e Shipping compatibile.

---

## PDR-06 — Mappa tattica 3D e pathfinding

### REFACTORTACTICS Mappa tattica 3D e pathfinding
FRTCellId, grafo multilivello, A*, LOS e revisioni

Documento: PDR-06 Versione: 0.1 - Consolidato iniziale Data: 3 agosto 2026 Baseline tecnica: Unreal Engine 5.8 Specifica dati e servizi spaziali autorevoli.

#### Controllo del documento
|**Campo**|**Valore**|
|---|---|
|Stato|Bozza consolidata per revisione tecnica|
|Versione|0.1 - Consolidato iniziale|
|Data|3 agosto 2026|
|Autore|RefactorTactics / documentazione consolidata con supporto AI|
|Regola di prevalenza|Decisioni esplicite del progetto > requisiti consolidati > proposte PDR > ricerca web di supporto|

##### Sommario esecutivo
La mappa è un grafo tattico 3D compatto. FRTCellId usa X, Y e Layer; celle e archi contengono stato logico, mentre rendering e collisione sono aggregati. A* è autorevole, NavMesh è supporto visivo.

Decisioni consolidate

Costi interi/fixed-point, servizi separati per path/LOS/targeting e cache legata alla revisione del grafo.

Assunzioni operative

Il modello di serializzazione compatta e la tassonomia completa dei Layer vengono bloccati dopo il graybox 2D e il primo ponte.

##### Indice del documento
1. Modello spaziale

2. FRTCellId

3. Cell data

4. Graph transitions

5. Graybox generator

6. A* autorevole

7. Cost model

8. Cache e revisioni

9. LOS/targeting

10. Multilivello roadmap

11. Test e debug

#### 1. Modello spaziale
La mappa è un grafo tattico 3D discreto. Un nodo rappresenta una posizione valida; un arco rappresenta una transizione consentita. X/Y descrivono il piano logico, Layer distingue posizioni sovrapposte come terreno, ponte, tetto e tunnel.

#### 2. FRTCellId
```
USTRUCT(BlueprintType)
struct FRTCellId
```

```
{
```

```
    GENERATED_BODY()
```

```
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 X = 0;
```

```
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Y = 0;
```

```
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int16 Layer = 0;
```

```
    bool IsValid() const;
    friend bool operator==(const FRTCellId&, const FRTCellId&) = default;
```

```
};
```

Per usarlo come chiave di TMap serve GetTypeHash stabile. Per serializzazione di rete si può aggiungere NetSerialize solo quando i range sono fissati; durante Fondazioni la serializzazione standard riduce rischio.

#### 3. Cell data
|**Categoria**|**Campi suggeriti**|
|---|---|
|Identità|CellId, chunk ID, local index, revision|
|Geometria logica|elevation, world anchor, surface type, normal/facing hints|
|Movimento|base cost, blocked flags, unit profile restrictions|
|Combattimento|directional cover, opacity, visibility modifiers|
|Ambiente|water/fire/electric state, hazard intensity/duration|
|Runtime|occupant StableUnitId, interactions, enabled tags|

#### 4. Graph transitions
Gli archi sono dati di prima classe: una porta, un ponte o un ascensore modifica una transizione, non solo una mesh. Ogni arco dichiara From, To, transition type, physical cost, requirements, enabled state e revision. Le transizioni speciali possono produrre eventi nella resolution.

#### 5. Graybox generator
- 1 Input: width, height, cell size, origin, default Layer=0.

- 2 Generare array compatto di celle e adiacenze 4-way iniziali.

- 3 Creare una sola istanza renderer per chunk o InstancedStaticMesh, non Actor per cella.

- 4 Line trace/select converte hit world -> candidate cell -> lookup valido.

- 5 Debug overlay mostra X,Y,Layer, cost, blockers e graph revision.

- 6 Salvare la mappa come Data Asset o dati generati riproducibili per il test.

#### 6. A* autorevole
UE 5.8 espone FGraphAStar in AIModule/GraphAStar.h. Il progetto fornisce un graph adapter e un query filter. Il graph adapter espone validità e vicini; il filter decide costo, euristica e traversabilità.

```
using FRTGraphSearch = FGraphAStar<FRTGraphAdapter>;
```

```
FRTGraphAdapter Graph(MapState);
FRTPathQueryFilter Filter(UnitProfile, MoveBudget, MapState);
FRTGraphSearch Search(Graph);
EGraphAStarResult Result = Search.FindPath(Start, Goal, Filter, OutPath);
```

###### Nota API

Lo snippet è architetturale e va adattato alla firma/template della patch UE 5.8 bloccata. L’API ufficiale richiede TGraph e TQueryFilter con metodi specifici; verificare la compilazione nel branch Fondazioni.

#### 7. Cost model
|**Componente**|**Tipo**|**Uso**|
|---|---|---|
|PhysicalCost|Intero/fixed-point|Distanza, salita, superficie, transizione.|
|TacticalPreference|Intero, solo preview/bot|Cover, esposizione, formazione.|
|RiskCost|Intero, conoscenza consentita|Hazard pubblico e intenti alleati.|
|Validity|Boolean/reason|Blocco, profilo unità, budget, porta.|

Il path autorevole per il commit deve usare regole competitive; la UI può offrire varianti di preferenza senza cambiare quali archi siano legalmente percorribili.

#### 8. Cache e revisioni
Chiave cache proposta: GraphRevision + UnitMovementProfileId + Start + Goal + Budget + QueryPolicyId. Una modifica a porta, ponte, hazard bloccante o costo incrementa la revisione del chunk e una revisione aggregata. La cache non deve restituire path costruiti su un grafo precedente.

#### 9. LOS e targeting
Pathfinding, line of sight, targeting e trajectory sono servizi distinti. Un arco percorribile non implica visibilità; una cella visibile non implica target valido. Separare i servizi evita di codificare eccezioni delle abilità dentro A*.

|**Servizio**|**Input**|**Output**|
|---|---|---|
|Path|start, goal, unit profile, budget, map rev|path, cost, reason|
|LOS|source cell/height, target cell/height, opacity|visible, blockers, confidence|
|Targeting|ability definition, source, candidate, state|valid, reason, affected cells|
|Trajectory|source, direction/arc, blockers|segments, impact, obstruction|

#### 10. Multilivello roadmap
- Fondazioni: Layer 0, griglia 2D, ostacoli e A* locale.

- Passo 2: archi speciali tra Layer 0 e 1 per scale/rampe/ponte.

- Passo 3: tunnel sovrapposti e selezione livello camera/UI.

- Passo 4: porte/ponte runtime con revisioni e invalidazione cache.

- Passo 5: ascensori e transizioni temporizzate integrate nella resolution.

#### 11. Test e debug
Suite minima: equality/hash e conversione world-grid di FRTCellId; A* su mappa vuota, ostacolo, no path, budget e costi superficie; tie-break stabile con adiacenze permutate; invalidazione cache su GraphRevision; debug draw di nodi, archi, costi e path; profiling CSV/Insights con nodi espansi, cache hit e microsecondi per query.

Fonti tecniche verificate: Epic Games, FGraphAStar C++ API, UE 5.8.

---

## PDR-07 — Personaggi, abilità e Gameplay Ability System

**REFACTORTACTICS**

### **Personaggi, abilità e Gameplay Ability System** Kit modulari, targeting, combo ambientali e confine con il resolver

Documento: PDR-07 Versione: 0.1 - Consolidato iniziale Data: 3 agosto 2026 Baseline tecnica: Unreal Engine 5.8

PDR di gameplay tecnico per il roster del vertical slice.

#### Controllo del documento
|**Campo**|**Valore**|
|---|---|
|Stato|Bozza consolidata per revisione tecnica|
|Versione|0.1 - Consolidato iniziale|
|Data|3 agosto 2026|
|Autore|RefactorTactics / documentazione consolidata con supporto AI|
|Regola di prevalenza|Decisioni esplicite del progetto > requisiti consolidati > proposte PDR > ricerca web di supporto|

##### Sommario esecutivo
GAS gestisce abilità, costi, cooldown, attributi, tag ed effetti, mentre il resolver determina gli esiti da snapshot. Il roster proposto copre attacchi lineari, AoE, dash, difesa/counter, acqua-elettricità e modifiche a cover/archi.

Decisioni consolidate

Quattro personaggi con quattro abilità ciascuno; varianti sempre laterali e moving-target policy dichiarata.

Assunzioni operative

Nomi, valori numerici e fantasy del roster sono proposte non ancora approvate e richiedono playtest.

##### Indice del documento
1. Confine GAS/simulatore

2. Schema ability definition

3. Targeting policies

4. Roster vertical slice

5. Combo ambientali

6. Varianti e progressione

7. Resolution ability

8. Bilanciamento

9. Editor setup

10. Test

#### 1. Confine GAS/simulatore
GAS gestisce ownership delle abilità, costi, cooldown, attributi, tag ed effetti. Durante il planning può contribuire alla validazione e alla UI. Al commit, l’intento viene convertito in un comando logico; durante la resolution è il resolver a decidere esiti e produrre eventi. GAS applica o riflette il risultato, ma non usa timing di montage o prediction come fonte finale.

```
Ability Data/GAS ----> Planning validation ----> FRTAbilityIntent
                                               |
                                               v
Snapshot + Resolver rules ---> ResolveAbility(ActionId) ---> TurnEvents
                                               |
                           +-------------------+-------------------+
                           v                                       v
                     Apply logical state                     GAS/UI/VFX mirror
```

#### 2. Schema ability definition
|**Campo**|**Esempio/uso**|
|---|---|
|AbilityId + Version|Ability.VoltRunner.ChainArc v1|
|Tags|Ability.Attack.Line, Damage.Electric|
|Cost/Cooldown|valori interi e turni|
|Category/Priority|Attack, Defense, Reaction; ordine resolver|
|Targeting|Unit, Cell, Direction, Line, Circle, Self|
|Range/AoE|celle o fixed-point; shape policy|
|Requirements|LOS, tags, surface, occupancy|
|Effects|damage, status, displacement, graph/cover change|
|Duration|turni/micro-step/fase|
|Moving target policy|LockCell, TrackUnit, Retarget, Fizzle|

#### 3. Targeting policies
- LockCell: colpisce la cella dichiarata anche se il bersaglio si muove.

- TrackUnit: segue l’ID unità entro vincoli dichiarati al momento dell’impatto.

- Retarget: seleziona un nuovo bersaglio con algoritmo stabile e visibile nelle regole.

- Fizzle: l’azione fallisce o produce effetto ridotto con reason code.

#### 4. Roster vertical slice - proposta PDR
###### Stato

I quattro personaggi seguenti sono una proposta originale per soddisfare il vertical slice. Nomi, numeri e fantasy visivo non risultano ancora approvati.

##### MARA “BREAKPOINT” - Vanguard / controllo copertura
|**Abilità**|**Tipo**|**Effetto e trade-off**|
|---|---|---|
|Rail Punch|Attacco lineare|Linea corta, spinge di 1 cella; danno ridotto se non avviene displacement.|
|Breach Dash|Dash|Attraversa un arco porta e termina in guardia leggera; non può curvare.|
|Hard Reset|Difesa/counter|Riduce il primo impatto e interrompe un attacker adiacente; spreco se non<br>attivata.|
|Deploy Barricade|Modifica archi/cover|Crea cover direzionale o blocco leggero per 2 turni; limita anche rotte alleate.|

##### IVO “UNDERTOW” - Controller / acqua
|**Abilità**|**Tipo**|**Effetto e trade-off**|
|---|---|---|
|Pressure Jet|Linea|Danno leggero e spinta; aumenta su cella bagnata.|
|Flood Marker|AoE circolare|Bagna un’area senza danno immediato; setup visibile.|
|Slipstream|Dash support|Muove sé o alleato lungo celle bagnate; range scarso su asciutto.|
|Sluice Gate|Mappa|Apre/chiude una transizione idrica o cambia costo di un’area; effetto<br>simmetrico.|

##### NYX “VOLT RUNNER” - Skirmisher / elettricità
|**Abilità**|**Tipo**|**Effetto e trade-off**|
|---|---|---|
|Arc Lance|Attacco lineare|Danno elettrico; salta a bersagli bagnati con ordine stabile.|
|Static Bloom|AoE circolare|Campo elettrico ritardato; alleati inclusi se entrano.|
|Phase Dash|Dash|Ignora occupazione intermedia ma non muri/archi disabilitati; difesa ridotta<br>dopo arrivo.|
|Grounding Stance|Difesa/counter|Assorbe elettrico e risponde; vulnerabile a displacement.|

**SOL “PARALLAX” - Marksman / quota e visibilità**

|**Abilità**|**Tipo**|**Effetto e trade-off**|
|---|---|---|
|Vector Shot|Attacco lineare|Linea lunga, bonus quota; penalità attraverso opacity.|
|Prism Zone|AoE circolare|Rivela/illumina celle e riduce cover in una direzione.|
|Anchor Step|Dash corto|Riposition su quota/cover; non attraversa hazard.|
|Return Fire|Counter|Reagisce al primo attacco visibile; telegraphed e aggirabile.|

#### 5. Combo ambientali
|**Setup**|**Payoff**|**Counterplay**|
|---|---|---|
|Undertow bagna celle -> Volt Runner usa<br>Arc Lance|Propagazione elettrica ordinata su rete di celle<br>bagnate|Uscire dall’acqua, grounding,<br>interrompere setup.|
|Breakpoint crea barricata -> Parallax usa<br>linea/angolo|Canalizza movimento e crea tiro prevedibile|Distruggere/aggirare cover; usare<br>tunnel.|
|Jet spinge su Static Bloom|Displacement forza impatto AoE|Difesa anti-push, cambiare<br>direzione.|
|Sluice Gate cambia arco -> Breach Dash|Nuova rotta improvvisa per engage|Controllo porta/ponte e preview<br>alleata.|

#### 6. Varianti e progressione
Ogni slot può offrire varianti con budget di potenza equivalente. Esempio: Arc Lance può avere più range ma niente chain, oppure chain più ampia ma danno base inferiore. Talenti e gadget non devono rimuovere i counter fondamentali del kit.

#### 7. Resolution ability
- 1 Convertire AbilityId in definizione e versione snapshot.

- 2 Rivalidare precondizioni dipendenti dalla fase di impatto.

- 3 Calcolare affected cells/targets con servizi targeting/LOS.

- 4 Ordinare impatti e chain con chiavi stabili.

- 5 Produrre eventi; applicare stato logico; registrare fizzle/partial outcome.

- 6 Mappare eventi a Gameplay Effects/Cues e presentazione.

#### 8. Bilanciamento
- Potere misurato su azione + setup + rischio, non solo danno grezzo.

- Ogni combo forte richiede almeno un segnale osservabile e un counter accessibile.

- Friendly fire e collisioni devono essere espliciti per abilità.

- La mappa non deve rendere un personaggio obbligatorio: ogni affordance ambientale ha più utenti o alternative.

- La priorità di resolution è parte del balance e resta bloccata in ranked.

#### 9. Editor setup
- 1 Creare classi native URTAbilityDefinition e URTCharacterDefinition derivate da UPrimaryDataAsset.

- 2 Creare cataloghi Data/Abilities e Data/Characters con ID governati.

- 3 Configurare Gameplay Tags in Config/Tags separando Ability, Damage, Status, Surface e Event.

- 4 Abilitare GameplayAbilities/GameplayTasks quando entra la milestone Abilities.

- 5 Creare Blueprint presentation per animazioni, targeting decals e cues senza logica decisiva.

#### 10. Test
- Validator: ID duplicati, range negativo, cooldown invalido, tag ignoti, target policy mancante.

- Golden tests per ogni abilità su snapshot minimi.

- Combo acqua-elettricità con ordine di chain deterministico.

- Test moving target per tutte e quattro le policy.

- Test friendly fire, KO simultaneo e modifica cover/arco.

- Test GAS mirror: stato/logico e attributi/tag runtime restano coerenti.

Fonti tecniche verificate: Epic Games, Gameplay Ability System Overview, UE 5.8.; Epic Games, Gameplay Attributes and Attribute Sets, UE 5.8.; Epic Games, Using Gameplay Tags in Unreal Engine, UE 5.8.

---

## PDR-08 — UI/UX e coordinazione di squadra

###### REFACTORTACTICS
### UI/UX e coordinazione di squadra
Camera, planning, warning, playback ed explainability

Documento: PDR-08 Versione: 0.1 - Consolidato iniziale Data: 3 agosto 2026 Baseline tecnica: Unreal Engine 5.8

Specifica di esperienza utente del vertical slice PC-first.

#### Controllo del documento
|**Campo**|**Valore**|
|---|---|
|Stato|Bozza consolidata per revisione tecnica|
|Versione|0.1 - Consolidato iniziale|
|Data|3 agosto 2026|
|Autore|RefactorTactics / documentazione consolidata con supporto AI|
|Regola di prevalenza|Decisioni esplicite del progetto > requisiti consolidati > proposte PDR > ricerca web di supporto|

##### Sommario esecutivo
La UI rende leggibile la simultaneità senza violare la privacy. Camera isometrica, ghost path/AoE, intenti alleati, warning e combat log si appoggiano a view model sanitizzati e TurnLog autorevole.

Decisioni consolidate

Distinzione obbligatoria tra Confermato, Previsto e Incerto; warning solo da stato pubblico e intenti della propria squadra.

Assunzioni operative

Rotazione libera, CommonUI e styling finale restano successivi al proof of concept.

##### Indice del documento
1. Principi UX

2. Camera

3. Selezione e input

4. Planning HUD

5. Intenti alleati

6. Warning model

7. Confermato/Previsto/Incerto

8. Resolution playback

9. Combat log

10. Accessibilità e performance

11. Editor setup

#### 1. Principi UX
- La UI deve ridurre l’incertezza lecita, non rivelare informazione nascosta.

- Ogni previsione visuale distingue dati certi, alleati e dipendenze avversarie.

- La mappa resta leggibile con più path, AoE, livelli e hazard tramite filtri/focus.

- La resolution deve spiegare “perché”, non solo mostrare animazioni spettacolari.

- Input PC-first: mouse/tastiera primari, remapping e controller considerati via Enhanced Input.

#### 2. Camera
|**Azione**|**Comportamento**|
|---|---|
|Pan|WASD/edge drag/middle drag, velocità scalata con zoom.|
|Zoom|Verso cursor focus, limiti e smoothing solo presentazione.|
|Rotazione|Step 90° iniziale; rotazione libera solo se non degrada leggibilità.|
|Livelli|Filtro layer: all, current, below/above ghosted.|
|Focus|Unità selezionata, evento TurnLog, ping alleato.|

#### 3. Selezione e input
Enhanced Input usa Input Actions e Mapping Contexts. La modalità camera, targeting e UI può cambiare contesto senza sovraccaricare i binding. L’input genera richieste al controller/view model, non modifica direttamente lo stato del simulatore.

#### 4. Planning HUD
|`+---------------------------------------------------------------+`<br>`| Turn 04 | Planning 00:21 | Objective | Team Ready 1/2        |`|
|---|
|`+--------------------+------------------------------------------+`|
|`| Unit roster        | Tactical map                            |`|
|`| HP / resources     | path ghosts, AoE, cover, hazards        |`|
|`| ability slots      | teammate intent labels and pings        |`|
|`+--------------------+------------------------------------------+`|
|`| Selected action: target policy | warnings | Undo | READY      |`<br>`+---------------------------------------------------------------+`|

#### 5. Intenti alleati
|**Visuale**|**Contenuto**|**Filtro**|
|---|---|---|
|Path ghost|Percorso e destinazione|Unità/team/focus|
|Ability marker|Icona, target, direzione|Hover o selected ally|
|AoE ghost|Area prevista e friendly fire|Opacity per certainty|
|Label|Breve intento: “blocca porta”|Rate/length limited|
|Ready badge|Stato personale/alleato|Sempre visibile|
|Ping/drawing|Comunicazione temporanea|TTL e mute controls|

#### 6. Warning model
I warning usano solo stato pubblico, dati del giocatore e intenti della propria squadra. Non devono derivare da simulazioni con planning avversario disponibile sul client.

|**Warning**|**Condizione**|**Severità**|
|---|---|---|
|Collisione alleata|Path/destinazioni alleate incompatibili|Warning|

|**Warning**|**Condizione**|**Severità**|
|---|---|---|
|Friendly fire|AoE pianificata include alleato previsto|Warning/Block secondo<br>regola|
|Risorsa/cooldown|Costo non disponibile al commit|Error|
|Target incerto|Dipende da posizione avversaria futura|Info|
|Path invalidato|GraphRevision o stato proprio cambiato|Error e recompute|
|Piano non committato|Ready con draft più recente del commit|Block|

#### 7. Confermato, Previsto, Incerto
|**Classe**|**Stile proposto**|**Regola**|
|---|---|---|
|Confermato|Linea solida / colore pieno|Stato pubblico + regola deterministica.|
|Previsto|Linea tratteggiata / icona team|Include intenti della squadra.|
|Incerto|Gradient/fade + simbolo ?|Dipende da avversario o conflitto non risolto.|

#### 8. Resolution playback
- Playback guidato da TurnLog con timeline di eventi e gruppi simultanei.

- Camera evita salti eccessivi; priorità a eventi che coinvolgono unità selezionate o obiettivo.

- Fast-forward/skip consentito solo dopo aver ricevuto risultato autorevole completo.

- La durata visuale può cambiare senza cambiare ordine logico o stato.

- Evento fallito mostra reason: target moved, blocked, interrupted, insufficient state at impact.

#### 9. Combat log
Il combat log deve collegare dichiarazione, modificatori e risultato. Una riga espandibile può mostrare: “Arc Lance -> 18 danni: base 20, cover -4, wet chain +2”. Le spiegazioni derivano dagli eventi e reason code, non da ricalcolo client.

#### 10. Accessibilità e performance
- Non affidarsi solo al colore; icone, pattern e testo supportano gli stati.

- Scale UI e font leggibili a 1080p; key remapping; opzioni riduzione movimento/camera shake.

- Pooling di decals/linee; aggiornare preview a 8-12 Hz, non ogni Tick per tutti gli elementi.

- Virtualizzare combat log e liste; evitare binding UMG costosi per frame.

- Profilare GPU/Slate e numero di primitive di overlay sulla mappa completa.

#### 11. Editor setup
- 1 Creare IMC_Tactical e Input Actions per camera, select, confirm, cancel, ready e ping.

- 2 Creare WBP_TacticalHUD, WBP_UnitPanel, WBP_AbilityBar, WBP_TurnTimer e WBP_CombatLog.

- 3 Introdurre un view model C++/UObject che riceve stato pubblico e team-only già sanitizzato.

- 4 Implementare path/AoE preview come renderer dedicato, non come logica nei widget.

- 5 CommonUI solo dopo proof of concept con Enhanced Input e flow modale stabile.

- Fonti tecniche verificate: Epic Games, Enhanced Input in Unreal Engine, UE 5.8.

---

## PDR-09 — Pipeline contenuti, Data Assets e validazione

**REFACTORTACTICS Pipeline contenuti, Data Assets e validazione** ID stabili, Asset Manager, tag, hash e percorso modding-ready

Documento: PDR-09 Versione: 0.1 - Consolidato iniziale Data: 3 agosto 2026 Baseline tecnica: Unreal Engine 5.8

PDR della supply chain dei contenuti competitivi.

### Controllo del documento
|**Campo**|**Valore**|
|---|---|
|Stato|Bozza consolidata per revisione tecnica|
|Versione|0.1 - Consolidato iniziale|
|Data|3 agosto 2026|
|Autore|RefactorTactics / documentazione consolidata con supporto AI|
|Regola di prevalenza|Decisioni esplicite del progetto > requisiti consolidati > proposte PDR > ricerca web di supporto|

#### Sommario esecutivo
I contenuti sono Primary Data Assets catalogati con ID e versioni stabili. Prima di un match vengono scoperti, validati, risolti in un manifest e hashati. Il modding pubblico resta fuori scope, ma le fondamenta evitano hard-code e riferimenti fragili.

##### Decisioni consolidate

Gameplay Tags governati; validator per ID, dipendenze, mappe, limiti e cooking; nessun codice nativo non fidato nelle mod retail.

##### Assunzioni operative

Lo schema JSON mod e il formato esatto del manifest vengono introdotti dopo la stabilizzazione dei Data Assets interni.

#### Indice del documento
1. Obiettivi data-driven

2. ID e versioni

3. Primary Data Assets

4. Cataloghi

5. Gameplay Tags

6. Manifest e hash

7. Validator

8. Serialization/JSON

9. Modding-ready

10. Git LFS e repository

11. Test

### 1. Obiettivi data-driven
C++ definisce invarianti e possibilità; asset e Blueprint scelgono la variante. Ogni contenuto competitivo deve essere identificabile, versionato, validabile e incluso in un manifest del match.

### 2. ID e versioni
|**Concetto**|**Regola**|
|---|---|
|Stable ID|Stringa/Name governata, mai derivata solo dal display name localizzato.|
|Definition Version|Incremento quando cambia semantica o serializzazione.|
|RulesVersion|Versione del resolver e ordine fasi.|
|ContentManifestHash|Hash delle definizioni effettivamente caricate per la partita.|
|Redirect|Mappatura esplicita per rename; vietato silenziosamente in ranked.|

### 3. Primary Data Assets
UPrimaryDataAsset fornisce un PrimaryAssetId e supporto Asset Bundle, permettendo al progetto di scoprire e caricare definizioni tramite Asset Manager. Le classi native proposte includono CharacterDefinition, AbilityDefinition, RulesetDefinition, MapDefinition e ObjectiveDefinition.

### 4. Cataloghi
```
URTContentCatalog
  Characters: Map<CharacterId, SoftObjectPtr<CharacterDefinition>>
  Abilities:  Map<AbilityId, SoftObjectPtr<AbilityDefinition>>
  Rulesets:   Map<RulesetId, SoftObjectPtr<RulesetDefinition>>
  Maps:       Map<MapId, SoftObjectPtr<MapDefinition>>
```

```
Build/Match startup:
```

```
Discover -> Validate -> Resolve dependencies -> Build manifest -> Hash -> Lock
```

### 5. Gameplay Tags
Gameplay Tags sono label gerarchiche definite in un dizionario governato. RefactorTactics separa sorgenti Config/Tags per dominio e limita la creazione libera di tag in asset.

|**Root**|**Esempi**|
|---|---|
|Ability|Ability.Attack.Line, Ability.Defense.Counter|
|Target|Target.Cell, Target.Unit, Target.Direction|
|Damage|Damage.Kinetic, Damage.Electric, Damage.Fire|
|Status|Status.Wet, Status.Burning, Status.Guarded|
|Surface|Surface.Water, Surface.Metal, Surface.HighGround|
|Event|Event.Move.Blocked, Event.Ability.Interrupted|
|Phase|Phase.Movement, Phase.Attack, Phase.Environment|

### 6. Manifest e hash
- Il server risolve tutte le definizioni richieste prima del match.

- Il manifest include ID, versioni, dipendenze e hash normalizzati.

- I client confrontano il manifest per compatibilità; il server resta fonte di verità.

- La playlist ranked blocca ruleset e contenuti ammessi.

- Il TurnLog registra almeno RulesVersion e ContentManifestHash.

### 7. Validator
|**Regola**|**Severità**|
|---|---|
|ID duplicato o vuoto|Error|
|Tag ignoto/restricted non autorizzato|Error|
|Dipendenza mancante/ciclo non consentito|Error|
|Valori fuori limite, range/AoE negativi|Error|
|Ability senza moving-target policy|Error|
|Map edge verso cella inesistente|Error|
|Cover direction incoerente o layer invalido|Error|
|Soft reference non cookata nel bundle|Error|
|Display metadata mancante|Warning|
|Hash non riproducibile tra due scansioni|Error|

### 8. Serialization e JSON
Il formato runtime Unreal può usare USTRUCT/archives; per future mod data-only si prevede un JSON versionato con schema esplicito. Non esporre subito JSON come fonte primaria del vertical slice: prima stabilizzare le definizioni native e i validator, poi aggiungere import/export deterministico.

### 9. Modding-ready
- Fin dal giorno uno: ID stabili, riferimenti indiretti, versioni, hash e validator.

- Fino alla beta: nessun modding pubblico; contenuti interni usano la stessa disciplina.

- Prima fase pubblica: data-only/asset-only, sandbox e whitelist di tipi/tag.

- No codice nativo non fidato nelle mod retail.

- Manifest e compatibilità di rete impediscono match tra contenuti divergenti.

### 10. Git LFS e repository
```
/Source                 Git normale
/Config                 Git normale
/Content/*.uasset       Git LFS
/Content/*.umap         Git LFS
/Docs/PDR/*.md           Git normale (sorgente)
/Docs/PDR/Exports/*.pdf  opzionale LFS o release artifact
```

```
Naming: RT_<Area>_<AssetName> e cartelle per dominio, non per autore.
```

### 11. Test
- 1 Eseguire validator in Editor e commandlet CI.

- 2 Due scansioni del catalogo producono stesso manifest/hash.

- 3 Cook test verifica che tutti i Primary Assets richiesti siano inclusi.

- 4 Test rename/redirect esplicito e incompatibilità di versione.

- 5 Test JSON round-trip solo quando lo schema mod viene introdotto.

- 6 Test packaged server/client con manifest mismatch e messaggio diagnostico chiaro.

Fonti tecniche verificate: Epic Games, Asset Management in Unreal Engine, UE 5.8.; Epic Games, UPrimaryDataAsset C++ API, UE 5.8.; Epic Games, Using Gameplay Tags in Unreal Engine, UE 5.8.

---

## PDR-10 — Roadmap tecnica, QA e rischi

> Questo PDR ha già una **sorgente Markdown canonica**, aggiornata e con la colonna di stato:
> [`../../roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md`](../../roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md)
> (D-009, prima applicazione). Il testo qui sotto è lo **snapshot v0.1**, conservato per confronto: se i
> due divergono, vale la v0.2.

**REFACTORTACTICS**

### **Roadmap tecnica, QA e rischi** Milestone, acceptance criteria, performance e Definition of Done

Documento: PDR-10 Versione: 0.1 - Consolidato iniziale Data: 3 agosto 2026 Baseline tecnica: Unreal Engine 5.8

Piano operativo dal bootstrap al vertical slice e dedicated server.

#### Controllo del documento
|**Campo**|**Valore**|
|---|---|
|Stato|Bozza consolidata per revisione tecnica|
|Versione|0.1 - Consolidato iniziale|
|Data|3 agosto 2026|
|Autore|RefactorTactics / documentazione consolidata con supporto AI|
|Regola di prevalenza|Decisioni esplicite del progetto > requisiti consolidati > proposte PDR > ricerca web di supporto|

##### Sommario esecutivo
La roadmap affronta prima il loop deterministico locale, poi privacy di rete, abilità, multilivello e dedicated server. Ogni milestone ha un exit gate misurabile e packaged.

Decisioni consolidate

Feature Done solo con rete corretta, privacy, log/debug, test automatico e build packaged.

Assunzioni operative

Stime temporali e staffing non sono ancora specificati; il piano è sequenziale per rischio, non un calendario.

##### Indice del documento
1. Strategia milestone

2. Milestone Fondazioni

3. Acceptance criteria

4. Roadmap tecnica

5. Test pyramid

6. Performance budgets

7. Risk register

8. Definition of Done

9. Build e debug

10. Commit sequence

11. Prossimo passo

#### 1. Strategia milestone
Le milestone riducono rischio in ordine: prima determinismo locale e modello mappa, poi rete/privacy, poi abilities/GAS, quindi multilivello e dedicated server. Ogni incremento deve essere giocabile e osservabile.

#### 2. Milestone Fondazioni
|**#**|**Deliverable**|**Output verificabile**|
|---|---|---|
|1|Architettura vertical slice|Diagramma e ownership classi.|
|2|Struttura Source/Content|Cartelle e naming nel repository.|
|3|Plugin e Build.cs|Build Development Editor pulita.|
|4|Progetto e L_DevSandbox|Mappa avviabile e GameMode corretto.|
|5|Camera, selezione, graybox 2D|Pan/zoom/rotate, cell hover/select.|
|6|FRTCellId, lookup, grafo, A*|Path visibile e testato.|
|7|Due unità e planning movimento|Draft path per unità.|
|8|Ready simultaneo locale|Countdown annullabile.|
|9|Snapshot e movement resolution|Esito indipendente dal frame.|
|10|TurnLog + Automation Test|Log visibile e test automatico.|
|11|Roadmap successiva|Backlog rete, GAS, multilivello, dedicated.|

#### 3. Acceptance criteria Fondazioni
- Editor e packaged Development build avviano L_DevSandbox.

- Due unità selezionabili pianificano path validi su griglia 2D.

- Quando entrambe sono Ready, viene creato uno snapshot immutabile.

- Il movimento viene risolto per micro-step con policy collisione esplicita.

- Il TurnLog descrive move, block e fine turno; UI riproduce il log.

- Automation Test passa da command line e Session Frontend.

- Nessun sistema GAS, networking completo o modding viene introdotto prematuramente.

#### 4. Roadmap tecnica
|**Milestone**|**Obiettivo**|**Exit gate**|
|---|---|---|
|F0 Fondazioni|Loop locale movimento deterministico|Golden tests + packaged demo|
|F1 Rete privata|Listen server, preview team-only, commit e privacy test|Zero canary leak|
|F2 Abilities|4x4 kit, GAS mirror, target/LOS|Golden test per ability|
|F3 Mappa multilivello|Layer, porte, ponte, tunnel, acqua/elettrico|Revision/cache/LOS tests|
|F4 Vertical slice|2v2, objective, complete UI, bots base|Internal playtest 20-30 min|
|F5 Dedicated|Server target, reconnect, telemetry, replay audit|Packaged soak test|
|F6 Beta systems|Content pipeline hardening, balance, accessibility|Release checklist|

#### 5. Test pyramid
|**Livello**|**Esempi**|**Frequenza**|
|---|---|---|
|Core automation|CellId, costs, A*, resolver, hash|Ogni commit|
|Feature tests|Planning->snapshot->resolution|PR/CI|
|Network tests|team relay, stale sequence, disconnect|PR critiche/nightly|
|Functional maps|L_DevSandbox scripted scenarios|Nightly|
|Packaged tests|client/server, cook, privacy capture|Milestone/release|
|Playtest|leggibilità, durata, counterplay|Ogni incremento giocabile|

L’Automation Framework UE supporta test unit/feature/smoke/content stress; i test core devono essere indipendenti dallo stato globale e ripulire artefatti generati.

#### 6. Performance budgets
|**Budget**|**Target**|**Misura**|
|---|---|---|
|Client|60 FPS|Unreal Insights, stat unit/gpu/slate|
|Path|mediana < 2 ms|CSV per query, nodi espansi, cache hit|
|Preview completa|< 50 ms|input->server->ally render|
|Resolver server|< 100 ms / match MVP|scope timer per fase|
|Intent updates|8-12 Hz|packet/second e dropped stale|
|Replay divergence|0|state/log hash corpus|
|Intent leak|0|network canary test|

#### 7. Risk register
|**Rischio**|**P/I**|**Mitigazione**|
|---|---|---|
|Resolver difficile da spiegare|H/H|TurnLog reason codes e UI certainty dall’inizio.|
|Leak di planning|M/H|DTO team-only, canary test packaged, no global replication.|
|GAS invade autorità|M/H|Confine documentato; resolver puro prima di GAS.|
|Mappa Actor-heavy|M/H|Dati compatti + instancing/chunk rendering.|
|Path/LOS accoppiati|M/M|Servizi separati e contratti testati.|
|Scope roster e ambienti|H/M|Vertical slice 4 personaggi, una combo primaria.|
|Upgrade UE durante milestone|M/H|Patch lock; upgrade solo tra milestone.|
|Modding prematuro|M/M|Solo fondamenta ID/validator, niente pubblico.|

#### 8. Definition of Done
- 1 Funziona server/client nel modello previsto, non solo Standalone.

- 2 Non espone dati oltre la classificazione autorizzata.

- 3 Produce log e strumenti debug sufficienti a spiegare l’esito.

- 4 Include Automation/Functional Test pertinente.

- 5 Rispetta budget o registra una deviazione con owner.

- 6 È verificata in build packaged.

- 7 Ha documentazione, changelog e commit focalizzato.

#### 9. Build e debug

- Configurazioni iniziali: Development Editor per iterazione, Development Client/Server quando entra F1/F5.

- Comandi debug proposti: rt.Turn.DumpSnapshot, rt.Turn.DumpLog, rt.Map.Debug, rt.Net.IntentStats.

- Log categories dedicate: LogRTTurn, LogRTMap, LogRTNet, LogRTData.

- Unreal Insights scopes su path query, snapshot build e ciascuna resolver phase.

- Salvare fixture di snapshot e log in directory test, non nel Content di shipping.

#### 10. Sequenza commit proposta
- 1 `chore(project): bootstrap UE 5.8 C++ project`

- 2 `feat(camera): add tactical camera and cell selection`

- 3 `feat(map): add compact grid graph and cell lookup`

- 4 `feat(path): add authoritative A-star query`

- 5 `feat(planning): add local move intents and ready state`

- 6 `feat(turn): add immutable snapshot and move resolver`

- 7 `test(turn): add deterministic movement golden tests`

#### 11. Prossimo passo
###### Sprint immediato

Implementare esclusivamente F0 Fondazioni fino al golden test del movimento. Non introdurre GAS o rete completa prima che snapshot, collisioni e TurnLog siano stabili e testati.

Fonti tecniche verificate: Epic Games, Automation Test Framework in Unreal Engine, UE 5.8.; Epic Games, Run Automation Tests in Unreal Engine, UE 5.8.

---

## PDR-11 — Demo 2v2 automatizzata

### Executive Summary
In questo PRD definiamo una demo tattica 2v2 “vertical slice”【38†L170-L178】 su mappa piatta esagonale con i quattro personaggi Aegis (Guardiano), Nyx (Infiltrator), Drift (Controller) e Vex (Striker). L’obiettivo è allineare tutti i membri del team su che cosa stiamo costruendo e perché【17†L72-L80】. La demo copre tutte le componenti chiave del combattimento: movimento, coperture, interazioni ambientali e interrupt. È concepita come una _prova di concetto_ giocabile (vertical slice) che mostra un segmento completo del gioco【38†L170-L178】. Include meccaniche funzionali (richieste di gioco) e vincoli tecnici (requisiti non funzionali) con precisione. Puntiamo a un risultato deterministico: data la stessa sequenza di input (intent), la simulazione produce sempre lo stesso log【36†L173-L180】. La demo dura 8 turni (ogni turno 30s di pianificazione più 5s di interrupt) e ogni unità ha 100 HP di base. Di seguito descriviamo obiettivi, requisiti, dati della mappa, schede personaggi, formato di input, script e log atteso, regole di risoluzione, flussi UI/UX, metriche di successo, casi di test e requisiti tecnici.

#### Obiettivi della Demo
- **Mostrare le meccaniche core** : comprovare in situ le abilità dei personaggi, il movimento esagonale, il sistema di coperture e la propagazione degli effetti (acqua, elettricità, fuoco).

- **Testare l’automazione di gioco** : eseguire uno scenario predefinito 2v2 in modo deterministico, con intent definiti per ogni turno e log di eventi che consentano verifica automatica.

- **Validare l’UX demo** : implementare interfaccia che supporti il gioco a turni, finestre di interrupt (Reaction Charge di 5s) e pannello log degli eventi.

- **Misurare prestazioni** : assicurare che la demo giri fluentemente in Unreal Engine 5.x con snapshot serializzabili di stato e logging deterministico.

Questa demo funge da prova verticale del sistema, cioè di tutte le componenti integrate in un segmento giocabile【38†L170-L178】. Garantiamo documentazione chiara per tutti gli stakeholder (sviluppo, design, test)【17†L72-L80】. Vogliamo un’esperienza pulita, senza ambiguità, dove funzionalità e regole sono ben definite prima dell’implementazione finale.

#### Requisiti Funzionali
I requisiti funzionali descrivono **cosa** il gioco deve fare【32†L66-L75】. In questa demo includiamo:

- **Mappa di combattimento 2v2** : griglia esagonale piatta con elementi di copertura (cover direzionali) e ostacoli (muro, Acqua iniziale). Dimensione e coordinate definite.

- **Personaggi giocanti** : 4 unità (Aegis, Nyx, Drift, Vex), ciascuna con 100 HP, movimento basico (es. 3 esagoni) e abilità uniche. Ogni abilità ha portata, danno ed effetti precisi (tabella “Schede personaggi” sotto).

- **Combattimento a turni** : ogni turno ogni unità pianifica un movimento e un’azione. L’ordine di esecuzione è sincronizzato in micro-step (risoluzione simultanea di movimenti e azioni).

- **Interrupt (Reaction Charge)** : Azioni _offensive_ selezionate da nemici possono innescare reazioni immediate. Ad es., **Guardian Protocol** di Aegis può attivarsi se Drift viene colpito, per intercettare l’attacco; oppure **Slip Between** di Nyx permette di teletrasportarsi come contrattacco. Queste meccaniche richiedono una timeline degli eventi in tempo reale per gestire i delay e le interruzioni【23†L66-L69】.

- **Elementi ambientali e interazioni** :

- **Coperture distruttibili** : Alcune abilità generano barriere (Hardlight Barricade di Aegis) o distruggono barriere (espansione di gatto d’acqua o colpi esplosivi).

- **Propagazione effetti** : Acqua e fuoco si propagano in tile adiacenti (Acqua estende su esagono + Battito d’acqua di Drift sprema fuoco, Shock di Vex elettrizza l’area, ecc).

- **Line of Sight (LOS)** : Tiro e abilità colpiscono solo se la linea di vista è libera. Null Veil di Nyx ne riduce temporaneamente.

- **Logging** : Generazione di un _turn log_ evento-per-evento con timestamp. Include movimenti, attivazioni abilità, danni, morti, cambi di stato (es. stato Slow, coperture distrutte, ecc).

Notiamo che i requisiti funzionali si focalizzano su MVP del sistema di combattimento【18†L60-L69】: puntiamo alle meccaniche chiave, tralasciando dettagli opzionali secondari. Ogni requisito funzionale sarà verificabile tramite test (per esempio, “lanciare Floodgate dovrebbe spostare X e infliggere Y a un bersaglio” dovrà avere un caso di test che asserisca quell’effetto).

#### Requisiti Non-Funzionali
I requisiti non funzionali descrivono **come** il sistema deve essere costruito【32†L79-L87】. Per la demo:

- **Piattaforma** : Unreal Engine 5.x (versione specifica UE5.3+).

- **Determinismo** : La simulazione deve essere riproducibile: dati gli stessi input, il log degli eventi è identico (determinismo bitwise). Per garantire ciò, **registrare tutti gli input** (intent di movimento/azione) e **niente fonti reali di casualità** 【36†L173-L180】. Test di deterministicità dovranno essere eseguiti per ogni build.

- **Performance** :

- Pianificazione turni: 30s per turno (max) affinchè il giocatore (o sistema automatico) selezioni intent.

- Interrupt: finestra 5s per reazione.

- Il gioco deve girare stabile a **60 FPS** durante la risoluzione dei turni (uso di snapshot serializzabili in memoria per rollback/pause).

- **Formato dati** : Log e snapshot devono essere serializzabili (ad es. JSON/YAML). Documentazione chiede output leggibile per tool di test automatico.

- **Logging avanzato** : Ogni micro-step e collisione va registrato. La risoluzione è _deterministica su micro-step_ , quindi collisioni e movimenti devono seguire regole fisse (v. sotto).

- **Architettura** : Usa modelli di dati JSON per intent: per turno fornire un elenco di comandi per ciascuna unità (movimento + abilità/scelta), policy fallback (es. se abilità non valida, usa attacco base). È un flusso strutturato simile a uno user story completo【18†L60-L69】.

- **UI/UX** : Interfaccia intuitiva ma minimale – tempo di pianificazione visibile (countdown 30s), icona “interrupt in corso” durante i 5s di reazione, pannello log laterale con eventi in tempo reale.

Questi requisiti non funzionali vincolano il design dell’implementazione: per esempio, l’engine deve supportare replay deterministico (UE5 offre “record demo” ma va usato con RNG controllato), e la serializzazione in JSON va prevista dal day-1. In riassunto, richiediamo performance target coerenti con un’esperienza fluida, logistica dei dati robusta e un’esperienza utente snella.

#### Snapshot iniziale della Mappa
La mappa è un piano esagonale regolare, semplice e privo di altitudine, con coperture statiche e punti di interesse. La Fig. 1 (immagine generica di un board game a griglia) illustra il tipo di disposizione che utilizzeremo【56†embed_image】.

```
               [F]
        [C]  [M]  [M]  [C]
[Aegis] [M] [M] [R] [M] [M] [Vex]
[Drift] [W] [W] [M] [C] [M] [Nyx]
        [V]  [M]  [G]  [M]
```

- **C = Copertura** (blocca LOS in un verso); **M = Piastre metalliche** (suolo libero); **W = Acqua iniziale** ; **F = Serbatoio infiammabile** ; **V = Valvola controllo acqua** ; **R = Relay neutrale** ; **G = Generatore orientale** .

- **Coordinate principali (sistema assiale [q,r,layer])** :

- **Relay (R)** : (0, 0, 0), al centro.

- **Generatore (G)** : (2, 1, 0), obiettivo Est.

- **Serbatoio infiammabile (F)** : (0, -2, 0), punto alto Nord.

- **Valvola acqua (V)** : (0, -1, 0), distribuisce o interrompe flusso.

- **Acqua iniziale** : celle (-2,1,0) e (-1,1,0) (estremità Sud-Ovest), come corso d’acqua statica.

- **Piastre metalliche** (cammino centrale): (-2,0),( -1,0),(0,0),(1,0),(2,0).

- **Coperture direzionali** : (-1,-1,0), (2,-1,0) e (1,1,0) (riduzione LOS in verso indicato dal simbolo C).

Questa disposizione fissa è lo _snapshot iniziale_ : tutte le posizioni e condizioni (coperture integre, acqua corrente in [W], nessuna barriera creata) sono predefinite. Eventi come _propagazione di fuoco o distruzione di coperture_ aggiorneranno dinamicamente questo stato durante la simulazione.

#### Schede Personaggi
|Personaggio|Ruolo|HP|Movimento base (tiles)|
|---|---|---|---|
|**Aegis**|Guardian|100|3|
|**Nyx**|Infiltrator|100|3|
|**Drift**|Controller|100|3|
|**Vex**|Striker|100|3|

##### Aegis (Guardian)
- **HP/Mob** : 100 HP; movimento 3.

- **Abilità** :

- _Bastion Bolt_ : Raggio 5 (lineare). Danno 20 al primo nemico colpito o alla copertura pesante intermedia (10 danno a copertura leggera). Se il bersaglio si muove dopo il fuoco, il proiettile continua la sua traiettoria.

- _Shield Advance_ : Corsa (dash) fino a 3 caselle. Riduce del 50% il danno subito dal prossimo colpo. Spinge il nemico incontrato di 1 casella se presente (scudo che respinge); la spinta non oltrepassa altre unità o muri. (Uso micro-step per collisione spinta.)

- _Hardlight Barricade_ : Crea una barriera semitrasparente su 2 archi frontali (quelle che bloccano il fuoco avversario). Dura 2 turni o finché distrutta. Blocca proiettili nemici, ma non impedisce movimento. Le coperture possono interagire: per esempio, Fuoco o Ghiaccio su di esse danneggiano l’ostacolo.

- _Guardian Protocol_ (Interrupt): Se Drift subisce un attacco (o si muove entro 2 caselle da un attaccante), Aegis può scegliere una reazione: intercettare il colpo, ridurlo del 50% o annullare la

spinta subita da Drift. Se Aegis è adiacente al nemico, può eseguire lo scudo in risposta; questo consuma il suo turno reattivo ma non quello di Drift.

- **Build consigliato** : _Anchor_ . Esempio: +1 portata Barricade, Protocollo annulla spinta, aumentare resistenza. (Nessun gadget specifico applicato.)

##### Nyx (Infiltrator)
- **HP/Mob** : 100 HP; movimento 3.

- **Abilità** :

- _Shadow Needle_ : Raggio 6 (semidir.). Danno base 18. Ignora la prima copertura leggera sul percorso. +10 danno se Nyx parte invisibile (Shadowed) o fuori LOS avversaria.

- _Phase Cut_ : Corsa 4; può attraversare alleati e coperture (ma non nemici vivi). Infligge 24 danni al nemico colpito e colpisce tutti entro 1 esagono di raggio dal punto finale (è un attacco ad area localizzato). Se colpisce un alleato per errore, ferma Nyx sul confine; in ogni caso interrompe lo scudo di droni entro 1.

- _Null Veil_ : Campo d’ombra radiale (raggio 1 intorno a Nyx). Nyx diventa “not in LOS” agli occhi dei nemici per 1 turno (parziale invisibilità) e la sua posizione finale non è mostrata nell’anteprima avversaria. Nessun danno, ma innesca interazioni su quello stato.

- _Slip Between_ (Interrupt): Se Nyx viene bersagliata, consuma il suo turno reattivo per teletrasportarsi 1 casella (blink) in una destinazione libera alla stessa distanza dalla posizione originaria. Richiede che la destinazione sia visibile dal suo POV; dopo il blink, i calcoli di copertura/traettoria si ricalcolano. L’effetto è intercettivo: Nyx può annullare colpi e infliggere contro-danno al nemico.

- **Build consigliato** : _Ghostblade_ . Esempio: Phase Cut marca (enable mark), Shadow Needle +15% contro bersaglio marcato, Null Veil area minorata, bonus ai danni AoE. (Nessun gadget specifico.)

##### Drift (Controller)
- **HP/Mob** : 100 HP; movimento 3.

###### **Abilità** :

- _Pressure Wave_ : Raggio 4; dardo energetico lineare che taglia trasversalmente a una linea di 3 caselle in linea, infligge 30 danni e spinge i nemici colpiti (1 casella all’indietro). Blocca proiettili dietro di sé. Perfetto per controllare lo spiazzo.

- _Floodgate_ : Campo d’acqua 1 (raggio 1). Crea un tile d’acqua al centro dell’area (al livello di Drift) e infligge 10 danni agli alleati infradito lì dentro. Ripristina 1 HP per ogni alleato (include sé). Può spegnere fiamme; conversamente, abilità elettriche innescano Steam (vapore).

- _Vector Shift_ : Velocità aumentata; Drift può muovere 2 caselle addizionali (totale 5), ma perde 20 HP una volta terminato (overcharge meccanica).

- _Reversal Current_ (Interrupt): Se Drift subisce danno, attiva un contraccolpo: crea un piccolo getto d’acqua offensivo (raggio 2, 15 danni agli nemici entro un arco di 90° davanti) nella fase di reazione, poi finisce sopraffuso (Rush debuff: perde 10 HP).

- **Build consigliato** : _Aquasurge_ . Esempio: Floodgate +1 casella, Pressure Wave danno extra area, Vector Shift range ridotto. Gadget possibile: _Water Amplifier_ (es. potenzia Water grid generation).

##### Vex (Striker)
- **HP/Mob** : 100 HP; movimento 3.

- **Abilità** :

- _Arc Carbine_ : Raggio 5; fucile al plasma leggero. 20 danni a colpo singolo. Doppia carica base (non consecutivi).

- _Overdrive Dash_ : Corsa 4; al termine del movimento, Vex può piazzare un Generatore di Scarica (campo elettrico) su un’area di 2 esagoni (danno 15 a chi vi finisce dentro la prossima volta). Penetrazione LOS limitata.

- _Capacitor Burst_ : Colpo in salto (attacco verticale). Raggio 3 a bersaglio singolo, ignorando coperture tranne muri totali. Danno 35 + rallentamento (Rage debuff: il bersaglio subisce +10% danno dal prossimo colpo entro 2 turni).

- _Retaliation Spark_ (Interrupt): Se Vex viene bersagliato, consuma il turno reattivo per roteare e sparare con il Carbine potenziato (+50% danno) al nemico attaccante. Dà al contempo a Vex uno status “Overcharged” (arma carica, prossimo attacco +10 range).

- **Build consigliato** : _Ion Burst_ . Esempio: Overdrive area +1, Retaliation Spark +70% danno, Arc Carbine proiettili esplosivi (aggiunge AoE lieve). Gadget: _Capacitor Core_ (incrementa riserve elettriche).

Queste schede riassumono HP, mobilità, abilità con parametri (range, danno, effetti, cooldown), trigger di interrupt e build esemplificativa. I valori esatti (e.g. danni, range) sono stati scelti per bilanciamento interno e per testare tutte le meccaniche.

#### Formato di Input per l’Automazione
L’automazione prende in input (per ogni turno) una struttura serializzata (JSON/YAML) con le intenzioni ( **intent** ) di ogni unità. Il formato prevede:

- **Intent di movimento** : percorso (lista di coordinate) o destinazione finale desiderata.

- **Intent di azione** : abilità scelta e bersaglio (coordinate o direzione), o scelta di “nessuna azione” se si passa.

- **Policy di fallback** : se un intent risulta impossibile (es. obiettivo morto), indicare azione alternativa (es. attacco base).

- **Timing** : il sistema assume 30s di “planning phase” per inviare questo blocco di intent. Se scade, si attiva policy di default (es. nessuna azione).

- **Interrupt window** : durante l’esecuzione del turno, al verificarsi di un trigger (attacco nemico, o condizione), il client può fornire un _intent reattivo_ entro 5s. L’input specifica quale interrupt/ reazione eseguire. Se il giocatore non risponde, si usa la reazione di default (es. non fare nulla).

In pratica, per ogni turno formiamo un array di oggetti intent:

`[{unit_id, move: [...], ability: x, target: {...}, fallback: y}, ...]` . L’ordine degli intent nel file non determina la sequenza di esecuzione reale (che è parallela), ma i dati contengono tutte le scelte necessarie. Questo schema corrisponde a un flusso “user story” completo per ogni turno【18†L60-L69】. Ad es., un intent potrebbe essere `{ “unit”: “Aegis”, “move”: [[-3,0],[ -2,0] ], “ability”: “Hardlight Barricade”, “targetDir”: 0 }` .

#### Script Automatizzato e Expected Turn Log
Per 8 turni simuliamo lo scenario con intent predefiniti (come mostrato nel dettaglio di seguito). In ogni turno elenchiamo le intenzioni di Blu (Aegis+Drift) e Rosso (Nyx+Vex), poi il log evento-per-evento. Le transizioni includono movimenti, uso abilità, collisioni, danni e morti.

<!-- Start of picture text -->
Timeline Demo 2v2<br>Turno 1 Turno 2 Turno 3 Turno 4 Turno 5 Turno 6 Turno 7 Turno 8<br>0, 1 1, 1 2, 1 3, 1 4, 1 5, 1 6, 1 7, 1<br><!-- End of picture text -->

|Turno|Intent (Blu)|Intent (Rosso)|ExpectedTurnLog (eventi chiave)|
|---|---|---|---|
|**1**|Aegis: muovi<br>(0,0)→(−2,0),<br>_Barricade_verso<br>(2,1) (blocca nella<br>direzione di Vex).|Nyx: muovi<br>(1,1)→(−1,1)<br>(sotto<br>copertura),_Null_<br>_Veil_.|-_Aegis_si posiziona su (−2,0).<br>-_Nyx_si sposta<br>dietro le coperture a (−1,1). Attiva Null Veil,<br>diventa “non in LOS”.<br>-_Barricade_eretta su<br>arene (-3,2) e (-2,2).<br>- Nessun danno inflitto.|
||Drift: muovi<br>(−1,1)→(0,−1)<br>(attiva valvola),<br>_Floodgate_su<br>(0,−1).|Vex: muovi<br>(2,0)→(2,0) (si<br>avvicina, resta su<br>(2,0)), prepara<br>_Arc Carbine_.|-_Drift_sposta l’acqua: distribuisce nuovo tile W in<br>(−1,0) e (0,0), rimuovendo V. Attiva Floodgate:<br>posiziona acqua in (0,−1) riducendo 1HP a<br>sé.<br>-_Vex_resta (2,0). Pronto a sparare.<br>-<br>Log: acqua ora in (−2,1),(−1,1),(0,0),(−1,0),(0,−1).|
|**2**|Aegis:_Shield_<br>_Advance_<br>muovendo di 3<br>verso est fino<br>(0,0) (scudo<br>attivo).|Nyx:_Shadow_<br>_Needle_da (−1,1)<br>verso Vex (2,0).|-_Aegis_scatta da (−2,0) a (0,0), scudo pronto.<br>Incontra_Vex_: collisione;_Aegis_(Guardian) è più<br>pesante, non viene spinto.<br>-_Nyx_spara:<br>Shadow Needle (20 danni) colpisce_Vex_a (2,0)<br>oltre coperture._Vex HP_100→80.<br>- Log: Vex<br>-20 HP.|
||Drift: resto fermo<br>(0,−1),_Vector Shift_<br>su sé (attivo).|Vex:_Arc Carbine_<br>da (2,0) verso<br>Aegis (0,0).|-_Drift_resta (0,−1). Attiva Vector Shift (velocità 5,<br>perdendo 20HP subito: 100→80).<br>-_Vex_spara<br>Arc Carbine a Aegis: prima Aegis ha Shield<br>attivo, subisce solo 10 danni (dam.50→25<br>ridotto). HP Aegis 100→90. (Shield finito.)<br>-<br>Log: Aegis -10 HP.|
|**3**|Aegis:_Bastion Bolt_<br>linea lunga verso<br>Nord (colpisce<br>generatore).|Nyx:_Phase Cut_<br>corri<br>(−1,1)→(3,1),<br>attacca Vex che<br>sorprende.|-_Aegis_spara: Riktor Bolt (20dmg) alla cella (2,1)<br>con Generatore (blocco). Distrugge generatore!<br>Stato: Generatore distrutto (HP 100→0).<br>-<br>_Nyx_scatta: bypassa Drift/coperture, finisce su<br>(3,1) dietro_Vex_, infliggendo 24dmg._Vex_80→56.<br>(Inoltre, segna Vex con marcatura<br>Shadowblade.)<br>- Log: Gen. -100 HP,<br>Generatore spento; Vex -24 HP,_Shadowblade_.|

|Turno|Intent (Blu)|Intent (Rosso)|ExpectedTurnLog (eventi chiave)|
|---|---|---|---|
||Drift:_Pressure_<br>_Wave_dardo<br>(raggio 4) verso<br>(2,1) (centrato su<br>Vex).|Vex:_Capacitor_<br>_Burst_su Nyx<br>(non riesce, Nyx<br>out of LOS)|-_Drift_lancia Pressure Wave verso est: colpisce<br>_Vex_e_Nyx_nell’area._Vex_riceve 30dmg (56→26).<br>_Nyx_(posizione (3,1)) riceve 30dmg (100→70).<br>Entrambi spinti di 1 verso est.<br>-_Vex_basso a<br>26HP,_Nyx_70HP. Nyx subisce_Rage_(maggiore<br>danno prossimo colpo).<br>-_Vex_tenta<br>Capacitor su Nyx ma Null Veil impedisce la mira,<br>l’azione fallisce. <br>- Log: Vex -30 HP, Nyx -30<br>HP, status_Rage_su Nyx.|
|**4**|Aegis:_Guardian_<br>_Protocol_<br>(Intercetta il<br>prossimo attacco<br>su Drift).|Nyx:_Slip Between_<br>in reazione (non<br>necessario, non<br>bersagliato)|-_Aegis_pronto a reagire. (Nessun evento<br>immediato; protocollo in stand-by.)<br>-_Nyx_<br>non è bersagliata, non usa Slip.|
||Drift: muovi<br>(0,−1)→(0,0),<br>_Floodgate_su (0,0)<br>(genera acqua).|Vex:_Retaliation_<br>_Spark_(intercetta<br>Drift con<br>Counter-Shot<br>3x).|-_Drift_salta su (0,0) (tile ora Acqua, grazie a<br>turno 2), attiva Floodgate: crea ulteriore tile W in<br>(1,0).<br>-_Vex_viene bersagliato da Pressure<br>Wave di Drift, attiva Retaliation: spara 3 colpi<br>rapidi (20dmg ciascuno) a Drift. Drift subisce 60<br>danni._Drift HP_100→40. Vex guadagna 10 range<br>(Overcharged).<br>- Log: Drift -60 HP, Vex<br>+range.|
|**5**|Aegis:_Bastion Bolt_<br>verso (2,2)<br>(colpisce Valvola<br>in (0,−1)).|Nyx:_Shadow_<br>_Needle_da (3,1)<br>verso Drift (0,0).|-_Aegis_spara: colpisce la valvola d’acqua (V) in<br>(0,−1), distruggendola. L’acqua cessa flusso<br>residuo.<br>-_Nyx_spara a Drift: colpo medio<br>(24dmg) ma Drift era su Acqua. Drift<br>40→16.<br>- Log: Valvola distrutta; Drift -24 HP<br>(eora 16).|
||Drift:_Vector Shift_<br>su sé (attivo, per<br>velocità<br>aggiuntiva).|Vex:_Capacitor_<br>_Burst_da (2,0)<br>verso Drift (0,0).|-_Drift_attiva Vector Shift: +2 mov, perde altri<br>20HP (16→−4). Drift muore. Battle over per<br>Drift. (No status perché spara vicino alla<br>morte.)<br>-_Vex_spara Capacitor Burst in salto:<br>35dmg a Drift, che era già KO. Nessun<br>effetto.<br>- Log: Drift muore (tutti intent/abilità<br>dopo vengono ignorati).|
|**6**|Aegis: muovi<br>(0,0)→(1,0),_Shield_<br>_Advance_.|Nyx: (non ha<br>avversari né<br>spazio di<br>movimento<br>importante)|-_Aegis_si sposta in (1,0) e attiva lo scudo. Fine<br>mappa, solo Vex rimane a 26HP.|
||(nessuna altra<br>azione, Drift<br>morto)|Vex: (può anche<br>saltare, ma non<br>necessario)|- Log: nessun combattimento rilevante (2vs1).|

|Turno|Intent (Blu)|Intent (Rosso)|ExpectedTurnLog (eventi chiave)|
|---|---|---|---|
|**7**|Aegis:_Bastion Bolt_<br>finale su Vex (2,0).|Nyx:_Slip Between_<br>(ora possibile:<br>evade &<br>contrattacca su<br>Vex).|-_Aegis_spara: 20dmg a Vex (26→6). Non KO, ma<br>quasi.<br>-_Vex_sopravvive con 6HP.|
|||Vex:_Retaliation_<br>_Spark_(non<br>attivabile perché<br>Vex non era<br>bersaglio<br>diretto)|- Log: Vex -20 HP.|
|**8**|Aegis:_Bastion Bolt_<br>colpo finale (2,0).|Nyx:_Slip Between_<br>a (1,1),_Shadow_<br>_Needle_su Vex<br>(2,0).|-_Aegis_spara: 20dmg a Vex (6→ −14). Vex<br>muore.<br>-_Nyx_atterra accanto a Vex (1,1) e<br>spara Shadow Needle mortale (+10 bonus se<br>applicato) su Vex già stordito. Conferma la<br>morte.<br>- Log: Vex -20 HP→morto;_victoire_<br>Blu.|
|||Vex: (KO, non<br>reagisce)|-**Evento finale**: Team Blu controlla il relay e<br>l’area. Vittoria Blu.|

Gli eventi chiave (in ordine cronologico) sono mostrati nel log di ogni turno. Le asserzioni di test (vedi sezione successiva) verificano HP finali, stati (es. _Dead_ , _Rage_ ), e stato del generatore. Notare che il protocollo di Aegis e gli interrupt di Vex rientrano nel flusso: sono trattati come micro-eventi in questi log. Ad esempio, nel Turno 2, l’Aegis ha uno scudo attivo e riduce il danno del 50%【23†L66-L69】; nel Turno 4 Vex risponde con Retaliation Spark al dardo di Drift. Tutte le collisioni (Aegis vs Vex in Turno 2) seguono una regola fissa (il Guardian non viene spinto) – questo è un caso di risoluzione con priorità difensiva.

#### Regole di Risoluzione
- **Micro-step Simultanei** : I movimenti e le azioni pianificate vengono computati simultaneamente in piccoli step. Ad esempio, se due unità convergono sulla stessa casella, la collisione è risolta in base a priorità (es. Guardian più pesante non viene respinto). Le regole di risoluzione contano sugli stati all’inizio del passo e sugli intenti di movimento.

- **Collisioni e coperture** : Un agente spinto (da abilità come _Push_ ) viene spostato se la casella di destinazione è libera; altrimenti rimane fermo. I proiettili si bloccano su muri e possono danneggiare o distruggere coperture leggere. Le coperture strutturali hanno direzione: proteggono da fuoco laterale ma possono essere colpite dalla parte scoperta.

- **Propagazione effetti** :

- _Acqua/Gas_ : l’acqua si propaga a tiles adiacenti (fino a 3 mosse massime); l’abilità di Drift genera sempre un tile W temporaneo. L’acqua amplifica gli effetti elettrici (conduce elettricità a una cella in più) e produce vapore se combinata con fonte di calore (non usata qui).

- _Elettricità_ : abilità elettriche (Vector Shift, Overdrive) colpiscono in linee dritte per X esagoni. Lo Shock finale (arcobaleno Vex) danneggia in linea.

_Fuoco_ : incendi (cleared dalla Floodgate, non rilevante qui).

- **Line of Sight (LOS)** : Gli attacchi volanti e a raggio seguono una linea che può essere interrotta da coperture o muri. Null Veil (Nyx) e Shield (Aegis) alterano temporaneamente la percezione del bersaglio. Il sistema calcola la LOS dinamicamente ad ogni tiro.

- **Distruzione coperture** : Colpi su coperture applicano danni (Barricade richiede 2 turni per scomparire, fuoco o Overdrive possono degradarle). Una copertura distrutta apre nuove linee di tiro per quella direzione.

In sintesi, le regole di risoluzione rispecchiano un motore tattico a turni con risoluzione deterministica. Ogni sotto-evento è registrato nel log, così da fornire tracciabilità completa delle dinamiche di combattimento.

#### UI/UX Flows
- **Avvio demo** : Al caricamento, mostra briefing rapido (obiettivo: neutralizzare Generatore Est) e pulsante “Start Demo”. All’inizio di ogni turno viene avviato il timer di pianificazione (barra di progresso 30s). Durante il turno, l’utente inserisce intent; scorciatoie di tastiera/mouse per spostarsi su esagoni e selezionare abilità.

- **Finestra di interrupt** : Quando un trigger (attacco o condizione) si verifica, il gioco entra in modalità “Interrupt” visualizzando una notifica (“Choose Reaction – 5s remaining”). L’avatar coinvolto lampeggia. Il giocatore può selezionare un interrupt (se disponibile) entro 5s; in caso di timeout, si applica la reazione predefinita.

- **Log di combat** : A lato schermo, scorrono in tempo reale le azioni eseguite con timestamp (es. “[T2.2s] Aegis ha sparato Riktor Bolt a Vex, -20 HP”). Il log è filtrabile per unità e salvabile in file. Tutti gli eventi critici (danni, morti, reset di turni) appaiono evidenziati. Questa UI di log aiuta nel debug e nel testing automatico (ogni voce può essere confrontata con l’ExpectedTurnLog atteso).

Questi flussi UI seguono best practice di giochi tattici: timeline chiara, feedback immediato e log consultabile. Ad esempio, molti GDR moderni mostrano le azioni su timeline attiva con notifiche, e un pannello log dettagliato è standard nei giochi da tavolo digitalizzati. L’utente può interrompere la demo in qualsiasi momento per consultare lo stato o ripetere il turno se necessario (modalità debug).

#### Metriche di Successo e Test Cases
- **Victory Condition** : Blue vince se neutralizza il Generatore Est entro 8 turni senza perdere tutti i personaggi.

- **Nessun errore** : La demo deve eseguire senza crash o eccezioni; log deterministico verificabile.

- • **Precisione del combat** : Tutti i danni e stati calcolati devono corrispondere alle specifiche abilità. • **Copertura dei casi** : Ogni abilità e interazione chiave deve comparire almeno in un turno (es. fusione acqua-elettricità, barricate, spinta).

Tabella test (asserts) – esempi di eventi chiave:

|Evento chiave|Asserzione attendibile|
|---|---|
|**Turno 1**: Barricade eretta|Esistere copertura in (−3,2),(−2,2) con durabilità=2 turni.|
|**Turno 2**: Aegis vs Vex colpo|HP_Vex = 80 (100−20); Aegis ha Shield che riduce 10 danni.|
|**Turno 2**: Drift Vector Shift|HP_Drift = 80 (100−20) dopo auto-danno.|
|**Turno 2**: Arc Carbine|HP_Aegis = 90 (100−10 dal Carbine). Shield consumato.|

|Evento chiave|Asserzione attendibile|
|---|---|
|**Turno 3**: Generatore distr.|Generatore HP = 0 dopo Riktor Bolt; segnale vittoria anticipata.|
|**Turno 3**: Floodgate|Nuovo tile Acqua in (0,−1) e (−1,0); Drift HP ricaricato (da 90→91).|
|**Turno 4**: Retaliation Spark|HP_Drift = 40 (100−60) dopo counter 3 colpi di Vex.|
|**Turno 5**: Valvola distrutta|Valvola (0,−1) non più esistente; Acqua residua ferma.|
|**Turno 5**: Drift KO|Drift HP <= 0 (muore). Nessuna azione dopo.|
|**Turno 8**: Vex KO|HP_Vex <= 0 (muore con Riktor Bolt). Dead flag su Vex.|

Ogni riga di test confronta lo stato calcolato con quello atteso. Ad es.: dopo il Turno 2, assert `Vex.HP == 80` e `Aegis.Shield == false` ; dopo Turno 3, assert `Generator.active == false` e così via. I test garantiscono che ogni chiave di evento (danno, morte, status) rispetti le regole.

#### Requisiti Tecnici
- **Engine** : Unreal Engine 5.x (UE5.3 o superiore).

- **Snapshot serializzabile** : Stato di gioco (posizioni, HP, coperture) salvabile ogni micro-step in formato JSON/YAML per replay/debug.

- **Performance target** : 60 FPS costanti durante risoluzione; 30s CPU per pianificazione (multithread non necessario, turni di solo 4 unità).

- **Logging deterministico** : Ogni esecuzione salva log completo ed è riproducibile (“replay deterministico”). Come consigliato dalla letteratura, bisogna **registrare tutti gli input** e **evitare casualità imprevedibili** 【36†L173-L180】. Effettuare test di breakage di determinismo ad ogni modifica del codice.

- **Compatibilità** : Pubblico di riferimento è PC Windows; interfaccia e input mouse/keyboard.

- **Framework dati** : Uso di strutture dati JSON/ YAML per intent e log, in linea con standard Agile di requisiti facilmente testabili【18†L60-L69】.

- **Database test** : Nessuno, dati caricati in memoria.

- **Strumenti di sviluppo** : Git per versioning e tracking, Unreal Insights per profiling CPU/GPU, sistema di build CI che esegue test automatici di validazione log.

#### Asset e Milestones di Implementazione
- **Asset richiesti** : modelli base 3D (o sprite 2D) per i 4 personaggi, tileset mappa esagonale (coperture, acqua), effetti particellari per abilità (spari, fiamme, elettricità), interfaccia HUD (icone abilità, timer, log).

- **Milestones** :

- **Setup progetto UE5** : import caricamenti, plancia esagonale, sistema input mosse. (1 settimana)

- **Implementazione movimento e cover** : script per camminare su griglia, test collsione. (1 settimana)

- **Aggiunta abilità base** : coding Riktor Bolt, Arc Carbine, Pressure Wave, Shadow Needle. (2 settimane)

- **Interrupt e micro-step** : flusso di reazione in tempo reale; test _Guardian Protocol_ , _Retaliation Spark_ . (2 settimane)

- **Ambiente interattivo** : acqua, fuoco, valves. (1 settimana)

- **UI/UX e log** : timer, popup interrupt, log panel. (1 settimana)

- **Test integrato** : esecuzione script 8 turni, verifica ExpectedTurnLog, debugging. (1 settimana)

Il risultato deve essere una demo automizzata che rispetti tutti i requisiti elencati e possa essere usata come scenario di test automatico completo. Le strutture dati (intent JSON e ExpectedTurnLog) sono pronte per integrarsi in un framework di testing CI.

**Riferimenti:** I requisiti sono basati su best practice di documentazione (un PRD deve “allineare il team su cosa e perché”【17†L72-L80】) e metodologia Agile (siamo focalizzati su un MVP di funzionalità 【18†L60-L69】). La meccanica di interrupt riflette soluzioni già adottate in titoli moderni【23†L66L69】. La necessità di determinismo e logging è coerente con la letteratura su replay di giochi 【36†L173-L180】. Le tabelle e i diagrammi offrono una chiarezza ottimale per la fase di sviluppo e testing.

---

## PDR-12 — Catalogo del vertical slice

### Executive Summary
Questo documento definisce in modo chiaro scopo, obiettivi e funzionalità principali del **vertical slice 2v2** di _RefactorTactics_ , sviluppato in Unreal Engine 5.6 su mappa esagonale【13†L1495-L1502】. Il PRD organizza e allinea il team di design e sviluppo sui requisiti, come indicato da Atlassian: scopo, funzionalità chiave e criteri di successo devono essere una sola fonte di verità【13†L1529-L1532】. Abbiamo quindi descritto tutte le meccaniche di gioco (azioni, fasi di risoluzione, movimento, targeting, fallback, reazioni, stati, terreni, coperture, equipaggiamento, eroi e loadout), oltre ai requisiti tecnici (determinismo, performance, networking) e modello dati. L’implementazione è suddivisa in milestone progressive (dal setup all’integrazione finale), affiancata da strumenti di debug. Il documento include inoltre esempi di JSON per _PrimaryDataAsset_ , diagrammi mermaid (timeline e ER), e matrici di test manuali/automatici per la convalida del vertical slice. Tutti i requisiti sono espressi in termini concreti, e ove manchino dettagli espliciti abbiamo indicato “non specificato” secondo le linee guida.

### 1. Obiettivi di Prodotto e Scope
- **Ambito di gioco:** _RefactorTactics_ è un gioco strategico a turni su **griglia esagonale** , 2 squadre da 2 eroi (2v2). Il vertical slice comprende tutte le meccaniche base di combattimento tattico, senza livelli di progressione o contenuti casuali.

- **Motivazione:** Validare il concept di gameplay (movimento, combattimento, abilità, ambientazione) in un prototipo giocabile. Dimostrare a stakeholder e sviluppatori che i sistemi funzionano insieme in modo divertente ed efficiente【13†L1529-L1532】.

- **Tecnologia:** Unreal Engine 5.6 (C++) con approccio autoritario server-side per sincronizzazione. Il sistema deve essere deterministico (stessa simulazione su server e client) tramite seed fissi per i generatori di numeri casuali【8†L27-L34】.

- **Obiettivi chiave:** Implementare il “nucleo” (core) con 4 eroi distinti, 6–8 azioni per eroe, tipi di terreno e coperture, equipaggiamenti iniziali, regole di collisione e interazioni ambientali. Tutte le funzionalità devono essere testabili e riproducibili (replay deterministico)【8†L27-L34】 【15†L115-L123】.

- **Esclusioni:** Non si include IA avanzata né campagne narrative. Non ci sono upgrade o progressione tra partite. _Out of scope:_ multiplayer massivo, grafica avanzata, editor di mappe dinamico.

- **Stakeholder:** Team di design (impostazione regole e bilanciamento), team di engine (implementazione UE5, networking), QA (test manuale/automatizzato), producer.

- **Criteri di successo (DoD):** Il gioco è uno slice giocabile completo: tutte le azioni hanno ID e parametri stabili, i terreni e le coperture funzionano secondo specifiche, le collisioni simultanee sono regolate, la propagazione di effetti ambientali è deterministica. In particolare, la sequenza di gioco ripetuta con lo stesso seed deve dare sempre lo stesso risultato【8†L27-L34】 【15†L115-L123】.

### 2. Requisiti Funzionali
#### 2.1 Azioni Gioco
- **Tipi di azioni:** Il sistema include le azioni fondamentali (es. _Move, BasicAttack, Guard, Wait, Activate, Interact_ ), azioni di movimento speciali (Sprint, Dash, Charge, Leap, Reposition), azioni offensive (PrecisionAttack, HeavyAttack, LineAttack, CircularAoE, SuppressiveLine, MarkTarget), azioni difensive/reazione (Counter, Intercept, Deflect, Brace, Shield, Cleanse) e azioni di supporto/ambiente (Heal, CreateWater, Ignite, Electrify, CreateCover, ModifyArc). Ogni azione è definita da un ID univoco, fase di risoluzione, priorità, portata, costo in risorse (es. punti movimento, MP) e durata/cooldown.

- **Azioni selezionabili:** All’inizio del turno ogni eroe sceglie un percorso di movimento (se Move) e un’Azione Principale, oltre a eventuali reazioni preparate. Come tabella riassuntiva, l’azione base _Move_ (MP standard 5) consuma tempo nella fase 20 con priorità 50 ed è fallback ‘Stop’ se bloccata. _Attack base_ dipende dall’eroe e arma e viene risolta in fase 40 con priorità 50. _Guard_ ha fase 10 (preparation) e riduce il prossimo danno di 15, resiste a uno spinta di 1. _Wait_ (fase 20, priorità 100) non fa nulla e può solo impostare facing o preparare reazioni.

|**ID Azione**|**Tipo**|**Fase**|**Priorità**|**Portata /**<br>**Costo**|**Descrizione**|
|---|---|---|---|---|---|
|Action.Wait|Fondamentale|20|100|—|Nessuna azione; solo<br>facing/reazione|
|Action.Move|Movimento|20|50|5 MP|Movimento su percorso<br>adiacente|
|Action.BasicAttack|Offensive|40|50|1 cella (CC)|Attacco base (danno<br>variabile per eroe)|
|Action.Guard|Difensiva|10|40|Self (buff)|Riduce primo danno di<br>15, blocca push 1|
|Action.Activate|Principale|40|70|1|Attiva oggetto/obiettivo<br>adiacente|
|Action.Interact|Principale|40|80|1|Interagisce (es.<br>addobbare gadget)|
|**...**|_..._|**...**|**...**|**...**|**Altre azioni: Sprint,**<br>**Dash, ecc.**|

(Dettagli completi per ciascuna azione sono definiti nel catalogo di bilanciamento; qui si ribadiscono i più critici).

#### 2.2 Fasi di Risoluzione
La partita è a turni simultanei (planning + resolution). Ad ogni turno si segue la sequenza di fasi **fisse** : | Fase (Nome) | Codice | Contenuto principale |

|------------------|-------:|------------------------------------------------------| | **0. Snapshot** | 0 | Congelamento dello stato iniziale, intenti e semi RNG | | **1. Preparazione** | 10 | Applicazione scudi/Stance, trappole, setup reazioni | | **2. Movimento** | 20 | Movimento simultaneo (micro-step) | | **3. Controllo** | 30 | Root/Push/

Interrupt/Interposizione | | **4. Attacco** | 40 | Risoluzione attacchi ed abilità offensive e cure | | **5. Ambiente** | 50 | Trigger hazard (acqua, fuoco, elettricità, fumo) | | **6. Cleanup** | 60 | KO, verifica obiettivi, decremento cooldown, log |

L’ordine interno rispetta fase↘priorità↘ID (in modo stabile), evitando dipendenze non determinate da id statici o dall’ordine di un TMap【11†L29-L37】【15†L115-L123】. Tutte le azioni dichiarano fase e priorità: priorità minore significa risolto prima nella stessa fase. Per esempio, _Dash_ (fase 20, priorità 30) si risolve prima di _Move_ (20, priorità 50).

#### 2.3 Regole di Movimento
- **Budget:** 5 MP standard. Costo per cella: 1 MP (terreno normale), 2 MP (terreno difficile o salire di un livello con rampa). _Sprint_ consuma 8 MP totali (compenso Move+Azione), _Reposition_ e _Dash_ hanno portata fissa (2 e 3 celle).

- **Vincoli:** Non si può attraversare una cella occupata da unità solida; nemmeno termina in cella occupata. Il percorso viene scelto all’inizio del turno e non è ricalcolato durante la resolution.

- **Fallback Movimento:** Se il percorso si blocca (ad es. unità o ostacolo nuovo), l’unità si ferma nell’ultima cella valida (comportamento _Fallback.Stop_ )【11†L29-L37】. Non sono previste azioni di bypass automatico; il giocatore deve pianificare diversamente.

- **Collisioni Multiple:** In caso di conflitto (es. 2 unità puntano a stessa cella), le regole decise sono:

- Se pari priorità/MP, entrambe si fermano prima della cella contesa.

- Se un’unità fa _Charge_ e l’altra _Move_ , _Charge_ prevale: l’unità in movimento si ferma indietro.

- Se una cella è già occupata da un’unità ferma, chi entra si ferma prima.

- Due _Charge_ opposte si fermano entrambe. (Vedere schema di regolazione collisioni in _Catalogo Balancing v0.1_ ).

- **Movimenti Speciali:** _Dash_ e _Charge_ ignorano il normale percorso Move (movimento lineare); _Leap_ salta unità e coperture basse; _Push/Pull_ (come azione di controllo) sposta target di 1 cella se libera. Coperture alte o porte chiuse bloccano spostamenti lineari e subentrano regole di interruzione.

#### 2.4 Targeting e Forme
- Ogni azione definisce un tipo di targeting (bersaglio singolo, cella, area, linea, cone, ecc.). Esempi: _LineAttack_ colpisce la prima unità in una direzione esagonale (range 5), _CircularAoE_ prende come centro una cella scelta entro 4 caselle.

- Esistono forme standard:

- **Self:** bersaglio se stessi (es. Guard, Shield).

- **Cell/Point:** selezione di una cella nel range (es. Ignite sul terreno).

- **Direzione:** linea in una delle 6 direzioni (es. LineAttack, Charge).

- **Area:** raggio di celle attorno a un punto (es. AoE raggio 1).

- Il **targeting** verifica collisioni con ostacoli (coperture alte bloccano linee, coperture basse riducono danno se da dietro) e visibilità (LOS, fumo riduce visibilità a 2).

#### 2.5 Fallback delle Azioni
Ogni azione principale (dash, attacchi, AoE, cura, ecc.) dichiara un comportamento di fallback se il bersaglio non è valido al momento dell’effettiva risoluzione: ad esempio _BasicAttack_ può usare

`Fallback.BasicAttack` (cerca nuovo bersaglio vicino) o `Fallback.Cancel` (nulla). Per il vertical slice:

- _Move:_ sempre `Fallback.Stop` .

_AoE:_ `Fallback.AttackCell` (colpisce comunque la cella pianificata).

- _Attacchi diretti e cure:_ `Fallback.Cancel` (nulla).

• _Reazioni:_ non hanno fallback (si attivano o non succede nulla). Questo assicura comportamenti prevedibili in caso di movimento dei bersagli o modifiche di piano.

#### 2.6 Reazioni e Stati
- **Reazioni:** Azioni che scattano su trigger durante il turno altrui. Es. _Counter_ (contrattacco da 16d dopo essere colpiti), _Intercept_ (prende l’attacco diretto destinato a un alleato entro 2 celle), _Deflect_ (riduce 20 danni del primo attacco), _FlowReaction_ (Reposition dopo un attacco subito), ecc. Le reazioni sono di slot Reazione e spesso una sola attivazione per turno.

- **Stati Temporanei:** Gli eroi e le cellule possono avere stati come **Burning, Wet, Electrified, Obscured (fumo)** , **Rooted, Exposed, Marked** , ecc. I triggering di questi sono dettagliati nel catalogo: p.es. entrare in Fuoco infligge 10 immediati + _Burning_ (8 danni Cleanup per 2 turni), entrare in acqua applica _Wet_ (annulla _Burning_ , aumenta danni elettrici). Gli stati vengono aggiornati nelle fasi appropriate (Prep/Attacco/Ambiente). La reazione _Cleanse_ rimuove uno stato alla scelta del giocatore.

#### 2.7 Terreni di Gioco
La mappa esagonale contiene vari tipi di terreno con effetti specifici. Viene fornita la seguente tabella riassuntiva:

|ID Terreno|Nome|Costo<br>MP|Movimento|LOS|Effetto<br>Principale|
|---|---|---|---|---|---|
|`Terrain.Floor`|Pavimento|1|Normale|Libero|–|
|`Terrain.Rough`|Accidentato|2|Sì (no Dash)|Libero|Rallenta<br>(costo+2 MP)|
|`Terrain.ShallowWater`|Acqua<br>bassa|2|Normale|Libero|Applica_Wet_,<br>conduce<br>elettricità,<br>spegne_Burning_|
|`Terrain.Fire`|Fuoco|2|Normale|Parziale|Dà 10 danni +<br>_Burning_|
|`Terrain.Conductive`|Metallica|1|Normale|Libero|Conduce<br>elettricità|
|`Terrain.Smoke`|Fumo|1|Normale|Ridotta|Obscured<br>(riduce<br>visibilità)|

|ID Terreno|Nome|Costo<br>MP|Movimento|LOS|Effetto<br>Principale|
|---|---|---|---|---|---|
||||||Se entri con≥2|
|`Terrain.Ice`|Ghiaccio|1|_Scivoloso_¹|Libero|MP scivoli una|
||||||cella|
|`Terrain.HighGround`|Quota alta|1|Dipende<br>dagli archi|Libero|Bonus visivo|

1. _Scivoloso:_ se un’unità entra con ≥2 MP rimaste, scivola di 1 cella nella direzione di ingresso (sospendibile per test iniziale).

Ogni terreno può interagire con abilità ambientali: es. l’acqua può essere gelata o attraversata da elettricità【8†L27-L34】, il fuoco può propagarsi su vegetazione, il ghiaccio può essere rotto. La fase _Ambiente (50)_ gestisce propagazioni (in ordine di distanza) in modo deterministico.

#### 2.8 Coperture e Strutture
Le coperture forniscono protezione direzionale o totale. Riepilogo degli elementi principali:

|ID Struttura|Tipo|Movimento|LOS|Integrità|Protezione|
|---|---|---|---|---|---|
|`LowCover`|Copertura<br>bassa|Blocca proiettile<br>sotto|Parziale<br>(offuscato)|30|Riduce danno<br>10 da una<br>direzione|
|`HighCover`|Copertura<br>alta|Blocca<br>completamente|Bloccato|50|Protezione<br>totale (0<br>danni oltre)|
|`Structure.Door`|Porta|Variabile<br>(dipende stato)|Variabile|40|Variabile (es.<br>10 se<br>bloccata)|
|`Bridge`|Ponte|Permette arco<br>sopra|Libero|40|Nessuna (aria<br>aperta)|
|`KineticPanel`|Pannello<br>cinetico|Blocca proiettile<br>basso|Parziale|30|Protegge per<br>10 danni|

- **LowCover:** associata a un bordo di cella, riduce di 10 danni frontali, ma da lato diverso è inefficace; non blocca completamente linea di vista.

- **HighCover:** blocca movimento e linea di tiro; assorbe interamente i colpi fino a distruzione.

- **Porta:** può essere _Open/Closed/Locked/Destroyed_ . Cambio di stato aggiorna il grafo di connessione (es. una porta chiusa blocca il cammino e la LOS).

- **Ponte (Bridge):** collegamento tra due celle. Se disattivo o distrutto, non è attraversabile.

- **Pannello cinetico:** gadget di Riktor, crea una copertura temporanea con specifiche.

Le coperture interagiscono con abilità: es. _CreateCover_ posiziona un LowCover; _Breccia/Gadget_ possono danneggiare strutture (colpo, granata, BreachCharge); _LineAttack_ o _Charge_ possono essere interrotte da cover alte.

#### 2.9 Equipaggiamenti
Ogni eroe inizia la partita configurato con: **1 Variante d’Arma** , **1 Gadget** , **1 Modulo Reazione** (non si trovano o ottengono durante il turno). Non ci sono livelli o upgrade in partita.

- **Varianti di Arma:** modificano l’attacco base dell’eroe. Ad esempio:

| ID Variant | Vantaggio | Svantaggio | |-------------------|------------------|-------------------| | `Weapon.Precision` | +1 range | -4 danni | | `Weapon.Impact` | Push 1 | -1 range | | `Weapon.Overcharge` | +6 danni | +1 cooldown | | `Weapon.Split` | +1 bersaglio | -6 danni | | `Weapon.Suppressive` | Applica Slow | -5 danni | | `Weapon.Environmental` | Potenzia hazard | -5 danni |

- **Gadget:** elementi d’uso singolo con effetto attivo, es. _Medkit_ (cura 18), _Sprinkler_ (crea acqua), _SmokeEmitter_ (crea fumo), _Anchor_ (blocca push), ecc. Ogni gadget ha un cooldown in turni. (Vedi tabella dettagliata nel catalogo).

- **Moduli Reazione:** potenziamenti passivi che scattano su trigger. Esempi: _Reaction.CounterShot_ (contrattacco 14d quando colpiti), _Reaction.ReactiveShield_ (+15 scudo quando subisci danno), _Reaction.AllyIntercept_ (intercetta attacco alleato), ecc. Ogni modulo si attiva al massimo una volta per turno.

In fase di planning il giocatore sceglie la combinazione Weapon/Gadget/Reaction; un modulo avversario pre-assegnato può essere noto o meno (parte del loadout visibile pre-partita).

#### 2.10 Eroi e Loadout Iniziali
Abbiamo definito **4 eroi** base con ruoli distinti. Tabella riepilogativa (HP, movimento, affinità):

|Eroe|Ruolo|Salute|Movimento|Range<br>Visivo|Affinità|Note|
|---|---|---|---|---|---|---|
|**Gadget**|Attacco/<br>Controllo|90|5|6|Elettricità|Stun/Effetti +<br>elettricità|
|**Phase**|Supporto/<br>Controllo|95|5|5|Acqua|Debuff nemici<br>(Wet) e cura|
|**Riktor**|Difesa/Area/<br>Protezione|120|4|5|Strutture|Pannelli, scudi|
|**Wraith**|Assalto/Duello|100|6|6|Movimento|Reazioni per<br>punire avversari|

Ogni eroe ha 4 abilità fondamentali uniche (vedi catalogo per effetti), di cui una variante potenziabile. L’ **equipaggiamento iniziale consigliato** (WeaponVar, Gadget, Reazione) è: Gadget con Precisione+Isolante+Scudo Reattivo, Phase con Cura+Sprinkler+Fuga hazard, Riktor con Pannello Adattivo+Copertura Portatile+Interposizione, Wraith con Intercetto Esteso+Sensore+Dash d’emergenza. Il loadout determina tattiche come push+hazard e combo acqua/elettrico.

### 3. Requisiti Non Funzionali
• **Determinismo:** Tutta la simulazione deve essere deterministica dato uno stesso seed iniziale. Si adotterà un approccio lockstep/client-server: il server calcola e invia eventi essenziali, i client riflettono la scena. I generatori di numeri casuali (FRandomStream) usano semi sincronizzati replicando un seed in fase di Snapshot【8†L27-L34】. Questo garantisce che muovendo l’input (turni, azioni) con lo stesso seme si ottengano identici risultati. (Nota: Unreal non è deterministico per default a causa di virgola mobile e tick asincroni【15†L115-L123】; si seguirà tick a step fisso o simile, e si eviteranno funzioni non deterministiche).

- **Performance:** Il gioco deve girare fluido con minime risorse. Si minimizzerà il carico di rete disabilitando la _replicazione_ degli attori non necessari ( `SetReplicates(false)` ) e riducendo il `NetUpdateFrequency` 【 21†L19-L26】. Si useranno tipi di rete quantizzati (es. `FVector_NetQuantize` ) per ridurre banda【21†L34-L41】. I comandi di debug (vtune,

- profiling) saranno impiegati per individuare colli di bottiglia, e il codice C++ sarà ottimizzato (nessun uso eccessivo di float costosi).

- **Networking:** Topologia client-server con server autoritario. Tutti i dati di gioco (posizioni, stati, HP) vengono replicati dal server ai client. I comandi dei giocatori sono in input e conferme. Si eviterà replicare dati ridondanti: per esempio, le azioni pianificate non sono replicate in realtime (si trasmettono solo intenti finali prima del turno e risultati al Cleanup). È richiesta prevedibilità di risposte anche con lag moderato: si implementeranno rollback limitati, e si sincronizzerà lo stato intermittente tramite checkpoint/semi.

- **Seed/Replay:** Il sistema deve supportare il _salvataggio del seed_ di ogni turno per poter riprodurre l’esecuzione (deterministic replay). Durante il debugging si può utilizzare

`rt.Debug.DumpSnapshot/TurnLog` e `rt.Debug.VerifyReplay` per confrontare ripetizioni. Il _TurnLog_ deve registrare tutti gli eventi del turno (input, collisioni, esiti abilità) in modo che, con lo stesso seed e snapshot, si ottenga identico risultato ad ogni esecuzione automatica【8†L27-L34】【15†L115-L123】.

### 4. API / Modello Dati Proposto
Per flessibilità e caricamento dati ottimale, useremo **PrimaryDataAsset** in C++/Blueprint per definire Actions, Terrains, Eroi ed Equip. Ogni asset ha un ID primario univoco e campi con

`UPROPERTY(EditAnywhere)` 【 1†L72-L81】. Di seguito un modello minimo (esempi di campi):

|**ActionDataAsset (USTRUCT)**|Tipo|Descrizione|
|---|---|---|
|ActionId (FName)|stringa|ID univoco (es. "Action.Move")|
|Phase (uint8)|intero|Fase di risoluzione (es. 20)|
|Priority (int)|intero|Priorità interna|
|Range (int)|intero|Portata dell’azione (celle)|
|Cost (int)|intero|Costo in risorse (es. MP)|
|Cooldown (int)|intero|Turni di ricarica|
|Effects (TArray<Effect>)|struct|Effetti applicati (danno, status)|

|**TerrainDataAsset**|Tipo|Descrizione|
|---|---|---|
|TerrainId (FName)|stringa|ID (es. "Terrain.Fire")|
|TraversalCost (int)|intero|Costo di movimento della cella|
|BlocksVision (bool)|bool|Se blocca completamente la LOS|
|HazardTags (array<FName>)|array|Tag di effetto (Wet, Burning, ecc.)|
|**HeroDataAsset**|Tipo|Descrizione|
|HeroId (FName)|stringa|ID eroe (es. "Hero.Gadget")|
|Health (int)|intero|Salute massima|
|MoveSpeed (int)|intero|Punti movimento base|
|SightRange (int)|intero|Raggio visivo|
|WeaponVariantId (FName)|stringa|Variante d’arma assegnata|
|GadgetId (FName)|stringa|Gadget assegnato|
|ReactionModuleId (FName)|stringa|Modulo reazione assegnato|

Questo schema dati ci consente di caricare in runtime via Asset Manager (grazie a _PrimaryDataAsset_ ) le definizioni di ogni entità【1†L72-L81】. I campi possono essere estesi (ad esempio aggiungere `Damage` , `AreaShape` , `StatusEffect` ) , mantenendo ID costanti.

**Diagramma ER (Entità/Relazioni):** Le entità principali e relazioni possono essere rappresentate come segue:

<!-- Start of picture text -->
HERO<br>può usare possiede<br>ACTION EQUIPMENT<br>colpisce/illumina<br>TERRAIN<br><!-- End of picture text -->

(In pratica: un eroe può eseguire molte azioni; un eroe possiede vari equipaggiamenti (arma, gadget, moduli); un’azione può interessare uno o più terreni).

#### 4.1 Esempi JSON di PrimaryDataAsset
Di seguito alcuni esempi semplificati dei dati asset salvati come JSON (solo campi minimi):

```
// Esempi di Action DataAsset
{
"ActionId":"Action.Move",
"Phase":20,
"Priority":50,
"Range":0,
"Cost":5,
"Cooldown":0,
"Effects":[]
}
{
"ActionId":"Action.Dash",
"Phase":20,
"Priority":30,
"Range":3,
"Cost":0,
"Cooldown":1,
"Effects":[]
```

```
}
```

```
{
"ActionId":"Action.BasicAttack",
"Phase":40,
"Priority":50,
"Range":1,
"Cost":0,
"Cooldown":0,
"Effects":["Damage:28"]
}
```

```
// Esempi di Terrain DataAsset
{
"TerrainId":"Terrain.Floor",
"TraversalCost":1,
"BlocksVision":false
}
{
"TerrainId":"Terrain.Fire",
"TraversalCost":2,
"BlocksVision":true
}
{
"TerrainId":"Terrain.ShallowWater",
"TraversalCost":2,
"BlocksVision":false
}
```

```
// Esempi di Hero DataAsset
{
"HeroId":"Hero.Gadget",
"Health":90,
"MoveSpeed":5,
"SightRange":6,
"WeaponVariantId":"Weapon.Overcharge",
"GadgetId":"Gadget.Insulator",
"ReactionModuleId":"Reaction.ReactiveShield"
}
{
"HeroId":"Hero.Phase",
"Health":95,
"MoveSpeed":5,
"SightRange":5,
"WeaponVariantId":"Weapon.Precision",
"GadgetId":"Gadget.Sprinkler",
"ReactionModuleId":"Reaction.HazardEscape"
```

```
}
```

```
// Esempi di Equipment DataAsset (Weapon/Gadget/Reaction)
```

```
{
"WeaponVariantId":"Weapon.Precision",
"Benefit":"+Range",
"Drawback":"-Damage"
}
{
"GadgetId":"Gadget.Medkit",
"Effect":"Cura 18 HP",
"Cooldown":3
}
{
"ReactionModuleId":"Reaction.CounterShot",
"Trigger":"Sotto attacco",
"Effect":"14 danni al nemico"
}
```

### 5. Flusso di Implementazione e Milestone
**Timeline di sviluppo:** Il vertical slice sarà realizzato per fasi iterativamente. Ecco una roadmap esemplificativa:

<!-- Start of picture text -->
Milestone Vertical Slice<br>Setup e Infrastruttura Definizione Assets (USTRUCT)Configurazione Network (AuthSrv)<br>Implementa Movimento base<br>Meccaniche Base Aggiungi Azioni Fondamentali<br>Prototipo Interazione Terreni<br>Combattimento e Azioni Attacchi Base e Targeting Azioni Speciali e Reazioni<br>Ambiente e Supporto Hazard (Acqua, Fuoco, Elettrico)Abilità Supporto (Heal, Cover)<br>Strumenti Debug (Log, UI)<br>Debug e Test Test Manuali & Automatici<br>PRD/Refinement Final<br>Release Packaged Build Vertical Slice<br>-09-07 -09-14 -09-21 -09-28 -10-05 -10-12 -10-19 -10-26 -11-02 -11-09 -11-16 -11-23 -11-30 -12-07<br><!-- End of picture text -->

- **Milestone principali:** Finire il motore del movimento e pathfinding, completare tutte le azioni primarie e le interazioni (Milestones _a3–a7_ ), poi iterare su controlli ambientali e supporto ( _a8–a9_ ). Entro ogni milestone, eseguire test di regressione per garantire determinismo.

- **Debug Tools:** Verranno sviluppati comandi console `rt.Debug` per visualizzare la griglia, cammini, coperture, intenti, e fare dump di snapshot e turn log (come riportato nel catalogo). Questi aiutano a tracciare esattamente come i dati cambiano in ogni fase.

### 6. Matrice di Test
**Test Manuali:** Coprono i casi critici delle regole di gioco. Alcuni esempi:

|**Scenario**|**Risultato atteso**|
|---|---|
|Due unità tentano stessa cella|Entrambe si fermano (regole collisione)|
|Move su terreno accidentato|Consuma 2 MP (costo extra)|
|Dash incontra copertura alta|Si ferma prima (non attraversa ostacolo)|

|**Scenario**|**Risultato atteso**|
|---|---|
|Push verso cella occupata|Nessuno spostamento illegale (si ferma)|
|Elettrificazione dell’acqua|Propagazione deterministica (stesso in tutti i test)|
|Fuoco spegnuto dall’acqua|Terreno “Fuoco” rimosso (brucia non si propaga)|
|Attacco base su copertura bassa|Danno ridotto di 10 (copertura schermata)|
|Intercept intercetta attacco alleato|Attaccante mira all’intercettore|
|AoE colpisce alleato|Si applica danno amico (friendly fire on)|
|Bersaglio si sposta prima dell’attacco|Viene applicato fallback (es._Cancel_)|
|Porta si chiude durante il turno|Ricostruzione del grafo aggiornata (collision avviene)|
|Replay stesso turno|TurnLog identico, risultato identico (determinismo)|

**Test Automatici:** Ogni build deve eseguire una suite di test unitari/integrati per validare regole chiave. Per esempio:

|**Test**|**Scopo**|
|---|---|
|`RT_Move_PathBlocked`|Verifica fallback in movimento bloccato|
|`RT_Move_CellConflict`|Conflitti di ingresso in cella|
|`RT_Dash_BlockedArc`|Dash interrotto da ostacolo|
|`RT_Push_InvalidDestination`|Spinta verso cella invalida|
|`RT_Env_WaterElectricPropagation`|Propagazione elettrica su acqua|
|`RT_Env_WaterExtinguishesFire`|Acqua spegne fuoco|
|`RT_Cover_DirectionalDamageReduction`|Danno ridotto da copertura bassa|
|`RT_Reaction_Intercept`|Intercept intercetta attacco|
|`RT_Reaction_SingleActivation`|Reazione si attiva al massimo 1 volta|
|`RT_Simulation_DeterministicReplay`|Replay deterministico e replay identico|

Ogni test automatico è replicato più volte con seed fissi per garantire non-variabilità. Questi test vengono eseguiti sia in editor sia nelle build finali. Il checksum finale del mondo simulato deve essere identico in ogni replay【8†L27-L34】【15†L115-L123】.

### 7. Criteri di Accettazione (Definition of Done)
Il vertical slice è approvato quando:

- **Asset e ID:** Tutte le azioni, terreni, coperture, eroi ecc. hanno ID stabili e dichiarati (es. _Action.Move_ ). Ogni asset dati (PrimaryDataAsset) ha un getID primario unico.

- **Requisiti funzionali:** Tutti i requisiti sopra elencati sono implementati in modo conforme (movimento, attacchi, reazioni, interazioni ambientali, ecc.). Il comportamento di ogni azione (fase, priorità, targeting, fallback) rispetta le tabelle di design.

- **Regole ambientali:** Ogni terreno e stato applica effetti come specificato. Ad esempio, _Wet_ e _Burning_ si alternano correttamente con l’acqua/fuoco, e l’elettricità si propaga solo su celle conduttive.

- **Reazioni e collisioni:** Tutte le reazioni sono triggerate correttamente (Counter, Intercept, ecc.) e applicabili una sola volta per turno. Collisioni e push/pull seguono le regole di sezione 2.3 e tabella.

- **Determinismo garantito:** Esiste un TurnLog verificabile che produce risultati identici per replay con stesso seed e snapshot【8†L27-L34】【15†L115-L123】.

- **Performance e stabilità:** La simulazione rimane entro limiti di latenza previsti e senza crash. I test di performance minima passano.

- **Documentazione:** Tutti i file di design (in Docs/Design/Balance) e gli asset vanno creati come da sezione 3 del catalogo. Un commit Git finale includerà `docs: add PRD vertical slice v1.0` con file PRD e qualsiasi asset di esempio.

### 8. Rischi e Mitigazioni
• **Mancato determinismo UE:** Unreal non è deterministico per default (float, tick asincroni) 【15†L115-L123】. _Mitigazione:_ utilizzare timestep fisso o controllo esplicito del delta-time; sincronizzare i semi dei RandomStream come discusso【8†L27-L34】; limitare l’uso di funzioni dipendenti dall’hardware. Se necessario, adottare server autoritario (rollbacks di sicurezza). • **Lag di rete / Bandwidth:** Replicazione pesante può causare hitches. _Mitigazione:_ disattivare la replicazione per attori statici o non essenziali【21†L19-L26】; usare net quantization (FVector_NetQuantize) per vettori e ridurre frequenza di aggiornamento【21†L34-L41】; testare su connessioni lente.

- **Ordine di Tick / Stato non sincrono:** L’ordine di ticking UE non è garantito e può variare 【11†L80-L89】. _Mitigazione:_ usare ordini di ticking espliciti o processare tutti gli attori in elenchi ordinati; evitare dipendenze dall’ordine di un TMap; imporre update deterministico (bEnableEnhancedDeterminism in fisica, se utile).

- **Input dei giocatori:** Race condition tra movimenti. _Mitigazione:_ la fase Snapshot congela gli input, e le interazioni (Interrupt, Counter) sono basate su regole fisse. Uso di `EventSequence` stabile per ordinare azioni parallele.

- **Ambiente e Hazard:** Molte interazioni possibili (acqua + elettrico + fuoco). _Mitigazione:_ testare tutte le combo possibili con tool di simulazione automatica; UI indicatori di stato ( _Wet_ , _Burning_ , ecc.) per chiarezza utente; log di debug per propagazioni.

- **Crash UI/UX:** Il vertical slice non include UI definitiva. _Mitigazione:_ focalizzarsi su logica backend, usare strumenti console di debug ( `rt.Debug` ) per visualizzare griglia, intenzioni e risolvere bug senza UI finale.

### 9. File e Commit Proposti
- **Doc di design:** `Docs/Design/PRD_RefactorTactics_v1.0.md` (questo documento in formato Markdown, sezione Agile/PRD).

- **Cataloghi di bilanciamento:** Seguire struttura del catalogo: `Docs/Design/Balance/ RT_ActionCatalog_v0.1.md` , `RT_TerrainCatalog_v0.1.md` ,

- `RT_EquipmentCatalog_v0.1.md` , `RT_HeroCatalog_v0.1.md` , ecc.

- **Asset di gioco:** Directory `Content/RefactorTactics/Data/Actions/` , `/Terrains/` , `/` `Equipment/` , `/Heroes/` per i vari PrimaryDataAsset.

- **Codice C++:** Includere USTRUCT e UPrimaryDataAsset corrispondenti: ad es.

- **Codice C++:** Includere USTRUCT e UPrimaryDataAsset corrispondenti: ad es. `URTActionDataAsset` , `URTTerrainDataAsset` , `URTHeroDataAsset` , `URTEqDataAsset` .

- **Commit Git:** Esempio di messaggio finale:

```
docs: add PRD per vertical slice RefactorTactics (v1.0)
```

Questo commit include il file PRD Markdown, eventuali aggiornamenti ai cataloghi esistenti e placeholder di esempio JSON in Content/. Il seguente ticket/product backlog item può riferirsi a questo commit.

**Fonti e best practice:** Per la stesura di questo PRD ci si è basati su linee guida Atlassian【13†L1495L1502】【13†L1529-L1532】 e documentazione Unreal su Data Asset【1†L72-L81】 e rete【21†L19L27】【21†L34-L41】, integrando le regole di gioco definite nel catalogo (es. gestione dei semi RNG 【8†L27-L34】) e nei forum tecnici (es. importanza di simulazione deterministica【15†L115-L123】).
