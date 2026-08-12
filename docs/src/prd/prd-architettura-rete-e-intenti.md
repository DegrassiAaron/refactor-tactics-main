# PRD — Architettura, rete, dati e intenti condivisi

> **Non è fonte normativa.** Livello **8** della gerarchia — *visione north-star*. In caso di conflitto
> prevalgono [`../../product/piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md),
> [`../../decisions/RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md) e gli ADR
> applicabili. L'inventario **vero** delle classi è
> [`../../technical/architettura-codice.md`](../../technical/architettura-codice.md).
>
> **Testo estratto dai PDF originari, non riscritto.**

## Da dove viene

| Sorgente (rimosso il 2026-08-12) | Pagine | Cosa contribuisce qui |
|---|---|---|
| `prd-intenti-condivisi.pdf` | 30 (tutte) | PRD di feature completo: 14 requisiti funzionali con ID e stima, NFR, flussi, modello dati, messaggi di rete, codice UE, piano di test, criteri di accettazione |
| `prd-stampabile.pdf` | 40 (di cui 11 qui) | Architettura tecnica e networking · modding e contratti dati |
| `prd-e-piano-di-sviluppo.pdf` | 45 (di cui 20 qui) | Architettura dei sistemi e modelli dati · stack tecnologico e configurazione |
| `prd-roadmap-e-percorso-didattico.pdf` | 35 (di cui 3 qui) | Architettura e scelte tecnologiche |

## Cosa resta vero, cosa no

**Recepito nel canone.** Regole in C++ e presentazione in Blueprint/Data; server autoritativo per ogni
decisione di gameplay; nessun `NetMulticast` con intenti privati e nessuno store globale replicato — la privacy
è **non consegnare il dato**, non nasconderlo alla UI (invariante #6); `UPrimaryDataAsset` come formato dei
contenuti competitivi; ID stabili, versioni e formati serializzati versionati (invariante #4).

**Superato.** `FRTGridCellId` e il **modello a chunk** con `FRTCellStaticData`/`FRTCellRuntimeState`: la
coordinata di oggi è `FRTCellId{X=q, Y=r, Layer}` assiale
([ADR-0002](../../decisions/adr-0002-griglia-esagonale.md)). I **dodici moduli** `RTCore · RTGrid ·
RTGridTypes · RTTurn · RTAI · RTNetworking · RTMod · RTModRuntime · RTUI · RTEditor …`: il repository ha
**due** moduli, `RefactorTactics` e `RefactorTacticsEditor`. Il costo di traversata **moltiplicativo float**:
il canone §3.1 adotta il modello additivo intero, perché nessun float può entrare nel resolver o nell'hash.
Il **GAS** nello stack: fuori dalla v0.1.

**Recuperabile, e non ancora recepito da nessuno.**

- 🟢 **Il PRD «Intenti condivisi» per intero.** È l'unico documento del corpus scritto come *specifica di
  feature consegnabile*: quattordici requisiti con ID (`FR-TRAJ`, `FR-AOE`, `FR-TARGET`, `FR-LABEL`,
  `FR-STATE`, `FR-CONFIRM`, `FR-NOTIFY`, `FR-GHOST`, `FR-CONFLICT`, `FR-DRAW`, `FR-HISTORY`, `FR-ROLLBACK`,
  `FR-RECONNECT`, `FR-FILTER`, `FR-MOD`), priorità, stime in giorni-persona, dodici requisiti non funzionali
  con **numeri** (mediana < 100 ms, p95 < 200 ms a RTT ≤ 120 ms, ≤ 5 KB/s per client, 10-12 update/s), piano
  di test e criteri di accettazione. Il canone lo elenca come **P1 north-star**; M10 lo costruirà, e questo è
  il capitolato che oggi manca.
- **La regola del rollback monotono**: ripristinare una revisione **non riusa il vecchio numero**, ne crea uno
  nuovo che ne copia il contenuto — così un pacchetto in ritardo non può sovrascrivere il piano ripristinato.
  È una regola sottile e non è scritta da nessun'altra parte.
- **La distinzione avvisi consultivi ↔ errori bloccanti**: due unità sulla stessa cella è una *strategia
  possibile*, quindi si segnala; un'abilità senza linea di tiro è un errore, quindi si blocca.
- **Modding a livelli** (`Data-only → Content → Game Feature → Native`) con schemi JSON, operazioni
  autorizzate e handshake sull'hash del manifest. Nessun documento corrente lo copre; il canone lo tiene a
  **P3** con la scelta *Blueprint sandbox vs Lua/UnLua* ancora **aperta**.
- **Le formule**: euristica ammissibile per A\* multilivello e funzione di utility del bot. Il bot corrente usa
  utility scoring (`URTHexBotLibrary`) senza una derivazione documentata.

**Attenzione a due assunzioni datate.** `prd-intenti-condivisi.pdf` assume **4v4**, **dedicated server** e
pianificazione **20-30 s**: la v0.1 è 2v2 offline. E assume «un programmatore principale, forte in C# e in
apprendimento su C++» — la fase didattica è **chiusa dal 2026-08-05**.

---

## Intenti condivisi — PRD di feature completo

### Product Requirements Document — Intenti condivisi
#### Executive summary
**Intenti condivisi** è il livello collaborativo della fase di pianificazione: mentre ogni giocatore prepara movimento, abilità e interazioni, i compagni vedono in tempo reale una rappresentazione privata del piano; gli avversari non ricevono né visualizzano questi dati. La feature comprende traiettorie, aree d’effetto, bersagli, ghost preview, etichette tattiche, stato di preparazione, notifiche, disegno temporaneo, cronologia, rollback e avvisi di conflitto.

La feature ha un valore centrale per il gioco, non puramente cosmetico. La ricerca sui workspace collaborativi mostra che informazioni visive condivise aiutano a mantenere consapevolezza dell’attività altrui, stabilire un contesto comune e coordinare le azioni; ritardare gli aggiornamenti visivi riduce parte 1 di questi benefici. Il pattern è inoltre coerente con giochi tattici basati sulla pianificazione: _Frozen Synapse_ permette di costruire e provare ordini prima dell’esecuzione simultanea, mentre _Phantom Brigade_ usa una timeline predittiva per orchestrare azioni coordinate. _Atlas Reactor_ basava il proprio ritmo su turni simultanei nei quali i giocatori dovevano coordinarsi e bloccare rapidamente le mosse. 2

La soluzione raccomandata è:

1. **Preview locale immediata** , senza attendere il server.

2. **Server autoritativo** , che valida ogni intenzione e ne assegna la revisione canonica.

3. **Distribuzione esclusivamente agli alleati** , tramite Client RPC sui rispettivi `PlayerController` o tramite componenti replicati owner-only.

4. **RPC unreliable per gli aggiornamenti frequenti** e **reliable per conferma, rollback e cambio di stato** .

5. **Snapshot periodici o Fast Array owner-only** per recuperare aggiornamenti persi.

6. **Nessun** **`NetMulticast` contenente intenzioni private** , perché un multicast viene eseguito sulle macchine per le quali l’attore è rilevante e non rappresenta, da solo, una separazione sicura fra squadre. 3

7. **Modding data-driven come ultima feature funzionale** , dopo che protocollo, schema dati e regole di sicurezza sono stabilizzati.

Per un team da quattro contro quattro, la feature non richiede netcode deterministico completo o rollback della simulazione. Il “rollback” previsto dal PRD riguarda le revisioni del piano durante la fase decisionale. Sistemi come GGPO o il Network Prediction Plugin salvano, ripristinano e risimulano lo stato del gioco e sarebbero eccessivi per il solo scambio degli intenti; potranno essere valutati separatamente per la risoluzione del turno. 4

###### Assunzioni di progetto

|Voce|Assunzione|
|---|---|
|Modalità principale|PvP quattro contro quattro|
|Modello di rete|Dedicated server autoritativo|
|Durata<br>pianificazione|Circa 20–30 secondi, configurabile|
|Engine|Unreal Engine 5.7 o 5.8, versione bloccata per ogni release|
|Piattaforma iniziale|PC|
|Sviluppo|Un programmatore principale, forte in C# e in apprendimento su C++|
|Persistenza|Nessun database nel percorso critico del match|
|Modding|Data-only inizialmente; codice nativo solo per mod curate|
|Stime|Giorni-persona netti, esclusi asset grafici definitivi e localizzazione<br>completa|

#### Obiettivi, benefici e requisiti
Gli obiettivi primari sono rendere leggibile la strategia degli alleati, ridurre la dipendenza da chat vocale, prevenire errori evitabili e conservare la tensione derivante dall’incertezza sulle mosse nemiche. Il sistema non deve trasformarsi in un simulatore perfetto del futuro: mostra ciò che gli alleati **intendono** fare, non garantisce ciò che accadrà dopo la risoluzione simultanea.

La distinzione è importante. Una traiettoria alleata deve essere presentata come piano provvisorio; l’interfaccia non deve suggerire che collisioni, spostamenti forzati, distruzione della mappa o azioni nemiche siano già risolti. La preview di _Frozen Synapse_ , per esempio, permette di testare piani ipotetici senza conoscere il piano reale dell’avversario; _Phantom Brigade_ usa invece la previsione come meccanica esplicita del gioco. Il nostro sistema segue il primo approccio: **condivisione dell’intento alleato, non previsione certa del turno** . 5

###### Obiettivi misurabili
- Un alleato deve capire in meno di due secondi dove un compagno vuole muoversi, quale area vuole colpire e se ha confermato.

- I giocatori devono poter correggere un piano senza interrompere il flusso degli altri.

- Gli errori più comuni — stessa cella finale, fuoco amico previsto, risorsa condivisa duplicata — devono essere segnalati prima del lock.

- Nessun dato degli intenti avversari deve raggiungere il client nemico.

- Il sistema deve restare utile anche senza chat vocale.

- La struttura dati deve supportare nuovi tipi di intento aggiunti tramite mod.

###### Fuori ambito iniziale
Non rientrano nel primo rilascio la previsione delle mosse nemiche, un replay deterministico completo del combattimento, testo libero nelle etichette, scripting arbitrario scaricato da server non affidabili e risoluzione automatica dei conflitti tattici.

##### Requisiti funzionali
|ID<br>FR-TRAJ|Requisito<br>Traiettorie|Comportamento richiesto<br>Mostrare spline o sequenza di celle,<br>cambi di livello, punti di transizione e<br>destinazione finale|Priorità<br>P0|Stima<br>3–5<br>giorni|
|---|---|---|---|---|
|FR-AOE|Aree d’effetto|Visualizzare cerchio, cono, linea, anello<br>o forma custom dell’abilità dalla<br>posizione prevista|P0|3–5<br>giorni|
|FR-TARGET|Bersagli|Evidenziare personaggio, cella,<br>oggetto o elemento della mappa<br>scelto|P0|2–3<br>giorni|
|FR-LABEL|Etichette di<br>intento|Associare tag predefiniti come Focus,<br>Protezione, Scout, Escape, Trappola|P0|1–2<br>giorni|
|FR-STATE|Stato giocatore|Mostrare Editing, Ready, Locked,<br>Disconnected e AFK/Timeout|P0|2–3<br>giorni|
|FR-CONFIRM|Conferma e<br>lock|Permettere conferma, annullamento<br>prima del lock e blocco immutabile<br>allo scadere|P0|3–5<br>giorni|
|FR-NOTIFY|Notifiche|Segnalare cambio bersaglio,<br>annullamento ultimate, conferma e<br>rollback senza spam|P1|2–4<br>giorni|
|FR-GHOST|Simulazione<br>ghost|Mostrare posizione prevista,<br>orientamento, stance e volume<br>dell’azione senza muovere l’attore<br>reale|P0|5–8<br>giorni|
|FR-CONFLICT|Avvisi conflitto|Rilevare collisioni, fuoco amico, celle<br>contese, risorse duplicate e azioni<br>incompatibili|P0|8–12<br>giorni|
|FR-DRAW|Disegno sulla<br>mappa|Tratti e frecce temporanei team-only<br>con limiti, timeout e mute|P1|4–6<br>giorni|
|FR-HISTORY|Cronologia<br>modifiche|Conservare una breve sequenza di<br>revisioni e una timeline sintetica dei<br>cambiamenti importanti|P1|3–5<br>giorni|
|FR-<br>ROLLBACK|Rollback del<br>piano|Ripristinare una revisione precedente<br>generando una nuova revisione<br>canonica|P1|4–6<br>giorni|
|FR-<br>RECONNECT|Riconnessione|Inviare snapshot completo degli<br>intenti visibili e stato del turno dopo<br>reconnect|P1|3–5<br>giorni|

|ID|Requisito|Comportamento richiesto|Priorità|Stima|
|---|---|---|---|---|
|FR-FILTER|Filtri e<br>accessibilità|Nascondere singoli overlay, cambiare<br>intensità, usare pattern oltre al colore<br>e supportare color blindness|P1|3–5<br>giorni|
|FR-MOD|Intenti<br>moddabili|Registrare etichette, renderer e tipi di<br>intento data-driven, con schema<br>versionato|P2, ultima<br>feature|10–15<br>giorni|

La stima netta dei requisiti è circa **58–89 giorni-persona** , prima del buffer di integrazione. Alcune attività procedono in parallelo: per esempio traiettorie, AOE e ghost condividono gran parte del renderer.

##### Requisiti non funzionali
|Area|Requisito|
|---|---|
|Latenza|Risposta locale entro un frame; aggiornamento alleato mediano sotto 100 ms e<br>p95 sotto 200 ms quando l’RTT è inferiore o uguale a 120 ms|
|Sicurezza|Tutti i payload client devono essere validati dal server; target, turno, ownership,<br>dimensione, frequenza e coordinate devono essere controllati|
|Privacy<br>tattica|Il server non deve inviare intenzioni, revisioni, target o identificatori nascosti ai<br>client nemici|
|Affidabilità|Conferma, lock, rollback e snapshot devono convergere anche con perdita di<br>pacchetti|
|Scalabilità|Target iniziale di otto giocatori; design compatibile con sedici senza cambiare il<br>protocollo|
|Banda|Massimo tipico 5 KB/s per client per la feature; picco consigliato inferiore a 10 KB/s|
|Frequenza|Preview limitate a 10–12 aggiornamenti al secondo per giocatore; rendering locale<br>a frame rate pieno|
|Osservabilità|Telemetria di latenza, dimensione payload, revisioni scartate, conflitti e fallimenti<br>di validazione|
|Modding|Schema versionato, whitelist server, hash dei pacchetti e fallback per tipi<br>sconosciuti|
|Privacy<br>utente|Disegni e cronologia non persistenti oltre il match salvo replay o telemetria<br>esplicitamente configurata|
|Accessibilità|Informazioni non codificate esclusivamente tramite colore; supporto a opacità,<br>spessore, icone e pattern|
|Compatibilità|Client e server devono negoziare versione del protocollo e manifest delle mod|

Unreal utilizza un modello client-server nel quale il server conserva lo stato autoritativo; le proprietà replicate vengono inviate dal server ai client e le Server RPC possono essere validate secondo una politica “trust and verify”. Questi meccanismi sono appropriati per impedire che il client dichiari autonomamente una mossa valida o decida a chi distribuirla. 6

#### Esperienza utente e flussi
Durante la pianificazione, ogni giocatore vede quattro livelli distinti:

|Livello|Esempi|Visibilità|
|---|---|---|
|Piano locale non inviato|Hover, mirino in movimento, path<br>incompleto|Solo autore|
|Intento condiviso<br>provvisorio|Percorso, AOE, bersaglio, etichetta|Autore e alleati|
|Intento confermato|Stesso overlay con stato Ready|Autore e alleati|
|Piano bloccato|Snapshot immutabile usato dal<br>resolver|Server e squadra; nessuna<br>modifica|

Questa separazione evita di trasmettere ogni movimento del mouse. L’utente può esplorare localmente; l’intento viene condiviso quando esiste una forma valida o dopo un breve debounce, per esempio 80– 100 ms.

L’overlay di ogni compagno deve avere un’identità visiva stabile: icona del personaggio, nome abbreviato, pattern, spessore della linea e colore. Il colore da solo non è sufficiente. Quando più traiettorie si sovrappongono, la UI deve applicare offset laterali, priorità all’intento selezionato e dissolvenza degli intenti non rilevanti.

<!-- Start of picture text -->
Inizio fase di<br>pianificazione<br>Il giocatore modificalocalmente la mossa<br>No Ghost preview immediatasul client Conferma<br>Intento abbastanzastabile? RPC reliable al server<br>Sì<br>Invio delta unreliable alserver Piano ancora valido?<br>Sì<br>No Validazione server Stato Ready e ack reliable<br>Non valido Valido<br>Reject o correzione al soloautore Nuova revisione canonica Tutti ready o timerscaduto?<br>No<br>Sì<br>Distribuzione ai soli alleati Calcolo conflitti Possibile modifica orollback Snapshot immutabile delturno<br>Overlay e ghost aggiornatisui client alleati Avvisi contestuali allasquadra Fase di risoluzione<br><!-- End of picture text -->

**Flusso di pianificazione di squadra.** Il giocatore seleziona una destinazione. Il client calcola il path, disegna immediatamente la spline e invia una versione compressa al server. Gli alleati vedono il percorso e la cella finale; se viene scelta un’abilità, appaiono bersaglio e AOE calcolati dalla posizione prevista.

**Modifica in tempo reale.** Ogni cambiamento incrementa `Revision` . Gli update possono arrivare fuori ordine perché sono unreliable: il ricevente applica solo revisioni maggiori di quella già visualizzata. Un update perso non richiede ritrasmissione immediata, perché il successivo contiene lo stato aggiornato o un delta riferito a una base esplicita.

**Conferma.** Premendo “Pronto”, il client invia una richiesta reliable. Il server ricostruisce o ricontrolla il percorso, valida abilità e bersaglio, assegna lo stato `Ready` e invia un acknowledgement. L’icona passa da “editing” a “ready”; il piano può restare modificabile fino al lock, ma una successiva modifica riporta automaticamente il giocatore in `Editing` .

**Rollback.** La UI presenta “Annulla modifica” e una breve cronologia. Il rollback non riutilizza il vecchio numero: crea una nuova revisione che copia il contenuto della revisione selezionata. In questo modo la sequenza resta monotona e i pacchetti ritardati non possono sovrascrivere il piano ripristinato.

**Timeout.** Alla scadenza, il server blocca l’ultima revisione confermata. Se non esiste, applica una regola esplicita: mantenere posizione, usare una mossa salvata localmente e già accettata dal server, oppure selezionare `Hold` . Il client non può inviare modifiche dopo il lock.

**Conflitti.** Gli avvisi sono consultivi, salvo invalidità formali. Due personaggi che raggiungono la stessa cella possono essere un errore oppure una strategia intenzionale: la UI segnala il problema, ma la risoluzione segue le regole del gioco. Un’abilità senza linea di tiro o un percorso fuori mappa è invece un errore bloccante.

Gli avvisi devono essere aggregati:

⚠ `Collisione prevista`

```
Echo e Nova terminano nella stessa cella, livello 2.
```

⚠ `Fuoco amico possibile L'area di Nova interseca il percorso di Echo al passo 3.`

```
ℹ Piano modificato
```

```
Echo ha cambiato bersaglio da Guardia A a Consolle.
```

Il disegno sulla mappa deve usare tratti temporanei, non testo libero. Limiti proposti: massimo otto tratti attivi per giocatore, 64 punti per tratto, durata massima pari al turno, rate limit e possibilità di silenziare un singolo compagno. Questo riduce spam, abuso e costi di moderazione.

#### Specifica tecnica e architettura
La feature deve essere organizzata come sistema indipendente dal movimento e dalle abilità concrete. Il dominio “intent” descrive un piano; pathfinder, targeting system e conflict engine lo interpretano attraverso interfacce.

<!-- Start of picture text -->
Mod system<br>Primary Data Assets<br>Intent Type Registry<br>Gameplay Tags<br>Renderer e Conflict Rules<br>Client alleato<br>Dedicated server Team Overlay<br>Intent Validator Intent Authority Store Client RPC o owner-onlyreplication Owner-only NetworkComponent Visible Intent Cache<br>Remote Ghost Renderer<br>Conflict Engine Team Visibility Router Client nemico<br>Gameplay normale<br>nessun invio Nessun payload alleato<br>Storage non critico<br>Turn Lock e Snapshot Telemetry DB<br>Replay e TelemetryAdapter<br>RPC Gateway e RateLimiter Client RPC o owner-onlyreplication Replay/Event Store<br>Client autore<br>Local Ghost Preview Server RPC<br>Planning Input Intent NetworkComponent Intent Cache Team Overlay<br><!-- End of picture text -->

Il database non partecipa alla pianificazione live. Il server mantiene gli intenti in memoria; al termine del turno può scrivere eventi aggregati per replay, analytics e debugging. Una dipendenza sincrona da database introdurrebbe un punto di latenza e fallimento non necessario.

##### Modello dati
|Campo|Tipo concettuale|Note|
|---|---|---|
|`IntentId`|`uint32` o GUID<br>compatto|Identità stabile del piano|
|`TurnId`|`uint32`|Impedisce modifiche a un turno diverso|
|`Revision`|`uint32`|Sequenza monotona assegnata o validata dal<br>server|
|`BaseRevision`|`uint32`|Base del delta; zero per snapshot completo|
|`SourcePlayerId`|ID server-side|Il client non può impostarlo autorevolmente|
|`TeamId`|Server-only|Usato per il routing, non considerato<br>attendibile dal client|
|`IntentType`|`FGameplayTag`|Esempio<br>`Intent.Move.Standard`|
|`AbilityId`|`FPrimaryAssetId`|Identifica definizione data-driven|
|`Path`|Celle o waypoint<br>compressi|Preferire<br>`CellId` al world-space definitivo|
|`Target`|Actor ID, cella o oggetto|Nessun riferimento a entità nascoste non<br>visibili|
|`Area`|Shape + parametri|Cerchio, cono, linea, custom|
|`LabelTag`|`FGameplayTag`|Esempio<br>`Intent.Label.Focus`|
|`PlanningState`|Enum|Editing, Ready, Locked|
|`FieldMask`|Bitmask|Campi modificati nel delta|

|Campo|Tipo concettuale|Note|
|---|---|---|
|`CustomPayload`|Struct versionata|Solo per intenti registrati|
|`ServerTimestamp`|Tempo match|Debug e ordinamento secondario|
|`ContentHash`|Hash breve|Verifica snapshot e mod manifest|

I Gameplay Tag sono etichette gerarchiche definite dall’utente e adatte a classificare oggetti e concetti di gameplay. I Primary Data Asset possono essere identificati e caricati tramite Asset Manager; i Data Registry forniscono un archivio globale efficiente per dati `USTRUCT` prevalentemente read-only. Questi strumenti sono una base naturale per tipi, label e renderer moddabili. 7

Per il path multilivello, il payload di produzione dovrebbe contenere riferimenti alle celle:

```
structFCellRef
{
uint16X;
uint16Y;
uint8Layer;
};
```

Il client può ricostruire la spline usando il centro delle celle e i collegamenti fra livelli. La conferma non deve fidarsi del path proposto: il server deve ricalcolarlo o validare ogni arco rispetto alla mappa corrente, all’unità, agli effetti ambientali e alle capacità di movimento.

##### Messaggi di rete
|Messaggio|Direzione|Payload<br>principale|Frequenza|Affidabilità|
|---|---|---|---|---|
|`C2S_IntentPreview`|Client→<br>server|Turno, revisione,<br>field mask, path/<br>target/area<br>modificati|0–12 Hz|Unreliable|
|`S2C_IntentPreview`|Server→<br>alleati|Snapshot o delta<br>canonico|0–12 Hz per<br>autore|Unreliable|
|`C2S_IntentConfirm`|Client→<br>server|Intent ID,<br>revisione, hash|Evento|Reliable|
|`S2C_IntentConfirmed`|Server→<br>alleati|Intento canonico<br>e stato Ready|Evento|Reliable|
|`S2C_IntentRejected`|Server→<br>autore|Revisione, reason<br>code, eventuale<br>correzione|Evento|Reliable|
|`C2S_SetPlanningState`|Client→<br>server|Ready o Editing|Raro|Reliable|

|Messaggio|Direzione|Payload<br>principale|Frequenza|Affidabilità|
|---|---|---|---|---|
|`S2C_PlayerPlanningState`|Server→<br>squadra|Player ID e stato|Raro|Replicated/<br>reliable|
|`C2S_RollbackIntent`|Client→<br>server|Turno e revisione<br>sorgente|Raro|Reliable|
|`S2C_IntentSnapshot`|Server→<br>client|Tutti gli intenti<br>visibili|Join,<br>reconnect,<br>healing|Reliable|
|`C2S_MapStrokeChunk`|Client→<br>server|Stroke ID,<br>sequenza, punti<br>compressi|5–10 Hz<br>durante il<br>tratto|Unreliable|
|`S2C_MapStrokeChunk`|Server→<br>alleati|Stroke validato|5–10 Hz|Unreliable|
|`S2C_ConflictSnapshot`|Server→<br>squadra|Conflitti attivi e<br>revisioni correlate|2–10 Hz|Unreliable<br>latest-wins|
|`S2C_TurnLocked`|Server→<br>tutti|Turn ID e<br>timestamp di<br>risoluzione|Una volta|Reliable|

Le RPC di Unreal sono chiamate unidirezionali e non restituiscono un valore, quindi conferma e rifiuto richiedono un messaggio separato. Epic indica inoltre le RPC unreliable per eventi transitori e la property replication per stato che deve convergere; una proprietà replicata arriva in modo affidabile al valore finale, ma non garantisce che il client osservi ogni valore intermedio. Questo comportamento è utile per una preview “latest wins”. 8

###### Strategia di sincronizzazione consigliata
- Il client disegna immediatamente la propria modifica.

- Ogni 80–100 ms invia l’ultimo delta disponibile.

- Il server ignora revisioni vecchie o duplicate.

- Gli alleati applicano solo revisioni superiori.

- Ogni secondo, oppure dopo inattività, il server replica lo snapshot corrente attraverso una proprietà owner-only o una Fast Array.

- Conferma e lock usano messaggi reliable.

- Dopo reconnect viene inviato uno snapshot completo.

`FFastArraySerializer` è progettato per la replica delta di array e dispone anche di integrazione con Iris; è adatto a una lista di intenti visibili sul componente owner-only del destinatario. 9

##### Autorità e visibilità selettiva
Il server deve derivare da solo `SourcePlayerId` e `TeamId` . Il client invia il contenuto della proposta, non l’identità autorevole né la lista dei destinatari.

Per quattro contro quattro, la soluzione più semplice e sicura è:

1. Ogni `PlayerController` possiede un `USharedIntentComponent` .

2. Il client invia una Server RPC sul proprio componente.

3. Il server valida e salva l’intento.

- Il server individua i `PlayerController` della stessa squadra.

- Chiama una Client RPC sul componente di ciascun destinatario.

6. I controller nemici non ricevono alcuna chiamata.

Un’alternativa più data-oriented è conservare, su ogni `PlayerController` , una Fast Array owner-only contenente gli intenti che quel client può vedere. I `PlayerController` sono particolarmente comodi perché ogni client possiede il proprio controller; GameState e PlayerState, invece, sono normalmente pensati per replicare informazioni condivise fra i client e non devono contenere direttamente dati tattici segreti. 10

`NetCullDistanceSquared` non deve essere trattato come controllo di privacy: è un’ottimizzazione spaziale della relevancy. Un avversario vicino potrebbe comunque diventare rilevante. Per attori teamscoped persistenti servono `bOnlyRelevantToOwner` , `IsNetRelevantFor` custom o un Replication Graph personalizzato. 11

**Regola fondamentale:** i ghost, le spline, i decal AOE e le frecce devono essere attori o componenti **locali non replicati** . Si replica il dato compatto; ogni client genera la propria rappresentazione grafica.

##### Gestione dei conflitti
Il conflict engine riceve l’insieme delle revisioni canoniche e produce record come:

```
ConflictId
RuleTag
Severity
ParticipantIntentIds
TimeSlice
CellIds
MessageKey
Blocking
```

Le regole iniziali sono:

|Regola|Tipo|
|---|---|
|Stessa cella finale nello stesso time slice|Warning|
|Percorsi che attraversano una cella esclusiva nello stesso istante|Warning|
|AOE alleata che interseca ghost o path alleato|Warning|
|Due interazioni su oggetto monouso|Warning o blocking|
|Target non più valido|Blocking|
|Path non percorribile|Blocking|

|Regola|Tipo|
|---|---|
|Abilità in cooldown o senza risorsa|Blocking|
|Cambio della mappa che invalida un arco|Blocking|
|Portale, ascensore o cover richiesti da più utenti<br>calcolo può essere debounced di 50–100 ms. Ogni war<br>lcolato; se una di esse cambia, il warning viene elimina|Rule-specific<br>ning deve dichiarare le revisioni su cui<br>to o ricalcolato.|
|**mplementazione in Unreal Engine**<br>divisione consigliata è netta:<br>Layer|Tecnologia consigliata|
|Struct di rete, validazione, rate limit, routing e<br>revisioni|C++|
|Input di pianificazione e prototipazione|Blueprint|
|Ghost, spline, decal, widget e animazioni|Blueprint/UMG/Niagara|
|Definizioni di intenti e label|Primary Data Asset, Gameplay Tags|
|Conflict rules comuni|C++ con parametri data-driven|
|Mod data-only|Plugin di contenuto, Data Asset, JSON<br>importato|
|Tool editor per creare intenti|Blueprint o C++ Editor Module|

Il calcolo può essere debounced di 50–100 ms. Ogni warning deve dichiarare le revisioni su cui è stato calcolato; se una di esse cambia, il warning viene eliminato o ricalcolato.

#### Implementazione in Unreal Engine
La divisione consigliata è netta:

Epic consiglia Blueprints per logica ad alto livello e casi specifici, mentre il C++ offre un controllo maggiore quando servono prestazioni, matematica complessa o gestione precisa della replica. Per questo progetto il compromesso migliore è un piccolo core C++ stabile, esposto a Blueprint con `BlueprintCallable` , `BlueprintAssignable` e `BlueprintNativeEvent` . 12

##### Configurazione del sistema
Su Windows:

1. Installare Unreal Engine tramite Epic Games Launcher.

2. Installare Visual Studio compatibile con la versione di Unreal scelta e il workload **Game development with C++** . La documentazione Epic mantiene una matrice fra versione di Unreal e versione di Visual Studio; per UE 5.7 sono supportate versioni recenti di Visual Studio 2022, mentre la documentazione di UE 5.8 include anche Visual Studio 2026. 13

3. Creare un progetto **Games → Blank → C++** , con Starter Content opzionale.

4. Aprire `Tools` → `New C++ Class` almeno una volta se si parte da un progetto Blueprint; il wizard converte il progetto in un code project e genera modulo, `.h` e `.cpp` . 14

5. Aggiungere a `YourGame.Build.cs` :

```
PublicDependencyModuleNames.AddRange(
newstring[]
{
"Core",
"CoreUObject",
"Engine",
"InputCore",
"NetCore",
"GameplayTags",
"UMG"
}
);
```

1. In `Edit` → `Plugins` , verificare o abilitare:

2. Gameplay Tags;

- Data Registry, più avanti;

4. Gameplay Abilities solo se già previsto per il sistema delle abilità;

- Functional Testing per i test.

6. Creare `BP_IntentPlayerController` , derivato dalla classe C++ del controller, e aggiungere `SharedIntentComponent` .

- Creare `BP_IntentPlayerState` , derivato dal PlayerState C++.

- Assegnare controller e player state nel GameMode.

9. In Play In Editor configurare quattro o più player e, quando possibile, un dedicated server. Unreal permette di testare più client, server separati, latenza e packet loss direttamente dalle opzioni PIE. 15

Per chi arriva da C#, Rider può risultare più familiare, ma resta necessario il toolchain C++ di Unreal. 16 Epic mantiene una guida specifica per configurare Rider.

##### Codice C++ minimo
Il seguente scheletro usa RPC dirette. È intenzionalmente semplice; nella beta, lo snapshot autoritativo può essere migrato a Fast Array. Sostituire `YOURGAME_API` con il macro del progetto, per esempio `TACTICALGAME_API` .

###### SharedIntentTypes.h
```
#pragma once
#include"CoreMinimal.h"
#include"Engine/NetSerialization.h"
#include"SharedIntentTypes.generated.h"
```

```
UENUM(BlueprintType)
enumclassEPlanningState:uint8
{
Editing,
```

```
Ready,
Locked
```

```
};
```

```
USTRUCT(BlueprintType)
structFSharedIntentData
{
```

```
GENERATED_BODY()
UPROPERTY(EditAnywhere,BlueprintReadWrite)
int32TurnId=0;
```

```
UPROPERTY(EditAnywhere,BlueprintReadWrite)
int32Revision=0;
```

```
// Il server sovrascrive sempre questo campo.
UPROPERTY(VisibleAnywhere,BlueprintReadOnly)
int32SourcePlayerId=-1;
```

```
UPROPERTY(EditAnywhere,BlueprintReadWrite)
FNameIntentType=TEXT("Move");
```

```
// Semplice per il prototipo.
// In produzione usare ID di celle compressi.
UPROPERTY(EditAnywhere,BlueprintReadWrite)
TArray<FVector_NetQuantize10>PathPoints;
```

```
UPROPERTY(EditAnywhere,BlueprintReadWrite)
FVector_NetQuantize10TargetLocation=FVector::ZeroVector;
```

```
UPROPERTY(EditAnywhere,BlueprintReadWrite)
int32TargetActorId=-1;
```

```
UPROPERTY(EditAnywhere,BlueprintReadWrite)
FNameLabel=TEXT("Intent.Label.None");
```

```
UPROPERTY(EditAnywhere,BlueprintReadWrite)
EPlanningStatePlanningState=EPlanningState::Editing;
UPROPERTY(VisibleAnywhere,BlueprintReadOnly)
boolbConfirmed=false;
```

```
};
```

###### IntentPlayerState.h
```
#pragma once
#include"CoreMinimal.h"
#include"GameFramework/PlayerState.h"
#include"IntentPlayerState.generated.h"
```

```
UCLASS()
classYOURGAME_APIAIntentPlayerState:publicAPlayerState
{
GENERATED_BODY()
public:
UPROPERTY(Replicated,EditAnywhere,BlueprintReadOnly,Category="Team")
uint8TeamId=0;
virtualvoidGetLifetimeReplicatedProps(
TArray<FLifetimeProperty>&OutLifetimeProps
)constoverride;
};
```

###### IntentPlayerState.cpp
```
#include"IntentPlayerState.h"
#include"Net/UnrealNetwork.h"
voidAIntentPlayerState::GetLifetimeReplicatedProps(
TArray<FLifetimeProperty>&OutLifetimeProps
)const
{
Super::GetLifetimeReplicatedProps(OutLifetimeProps);
DOREPLIFETIME(AIntentPlayerState,TeamId);
}
```

###### SharedIntentComponent.h
```
#pragma once
#include"CoreMinimal.h"
#include"Components/ActorComponent.h"
#include"SharedIntentTypes.h"
#include"SharedIntentComponent.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
FSharedIntentReceived,
constFSharedIntentData&,
Intent
);
UCLASS(
ClassGroup=(Network),
meta=(BlueprintSpawnableComponent)
)
classYOURGAME_APIUSharedIntentComponent:publicUActorComponent
```

```
{
GENERATED_BODY()
public:
USharedIntentComponent();
UPROPERTY(BlueprintAssignable,Category="Shared Intent")
FSharedIntentReceivedOnIntentReceived;
UFUNCTION(BlueprintCallable,Category="Shared Intent")
voidSubmitPreview(FSharedIntentDataIntent);
UFUNCTION(BlueprintCallable,Category="Shared Intent")
voidConfirmIntent(FSharedIntentDataIntent);
protected:
UFUNCTION(Server,Unreliable,WithValidation)
voidServerSubmitPreview(FSharedIntentDataIntent);
UFUNCTION(Server,Reliable,WithValidation)
voidServerConfirmIntent(FSharedIntentDataIntent);
UFUNCTION(Client,Unreliable)
voidClientReceivePreview(FSharedIntentDataIntent);
UFUNCTION(Client,Reliable)
voidClientReceiveConfirmed(FSharedIntentDataIntent);
private:
boolIsStructurallyValid(constFSharedIntentData&Intent)const;
voidBroadcastToTeam(
FSharedIntentDataIntent,
boolbReliable
);
int32LastAcceptedRevision=-1;
doubleLastPreviewServerTime=0.0;
};
```

###### SharedIntentComponent.cpp
```
#include"SharedIntentComponent.h"
#include"IntentPlayerState.h"
#include"GameFramework/PlayerController.h"
#include"Engine/World.h"
USharedIntentComponent::USharedIntentComponent()
{
```

```
SetIsReplicatedByDefault(true);
```

```
}
```

```
voidUSharedIntentComponent::SubmitPreview(FSharedIntentDataIntent)
{
// La UI deve già aver disegnato la preview locale.
ServerSubmitPreview(Intent);
}
voidUSharedIntentComponent::ConfirmIntent(FSharedIntentDataIntent)
{
ServerConfirmIntent(Intent);
}
boolUSharedIntentComponent::IsStructurallyValid(
constFSharedIntentData&Intent
)const
{
if(Intent.TurnId<0||Intent.Revision<0)
{
returnfalse;
}
if(Intent.PathPoints.Num()>32)
{
returnfalse;
}
for(constFVector_NetQuantize10&Point:Intent.PathPoints)
{
if(FVector(Point).ContainsNaN())
{
returnfalse;
}
}
return!FVector(Intent.TargetLocation).ContainsNaN();
}
boolUSharedIntentComponent::ServerSubmitPreview_Validate(
FSharedIntentDataIntent
)
{
// Solo controlli strutturali.
// Non disconnettere il client per una normale revisione vecchia.
returnIsStructurallyValid(Intent);
}
voidUSharedIntentComponent::ServerSubmitPreview_Implementation(
FSharedIntentDataIntent
)
```

```
{
constdoubleNow=FPlatformTime::Seconds();
// Massimo circa 12 update al secondo.
if(Now-LastPreviewServerTime<0.08)
{
return;
}
LastPreviewServerTime=Now;
// Ignora pacchetti vecchi o duplicati.
if(Intent.Revision<=LastAcceptedRevision)
{
return;
}
// TODO:
// - verificare TurnId contro il GameState;
// - validare path e target;
// - ricalcolare il path canonico.
Intent.bConfirmed=false;
Intent.PlanningState=EPlanningState::Editing;
LastAcceptedRevision=Intent.Revision;
BroadcastToTeam(Intent,false);
```

- `}`

```
boolUSharedIntentComponent::ServerConfirmIntent_Validate(
FSharedIntentDataIntent
)
{
returnIsStructurallyValid(Intent);
```

- `}`

```
voidUSharedIntentComponent::ServerConfirmIntent_Implementation(
FSharedIntentDataIntent
)
{
// È permesso confermare la stessa revisione già inviata come preview.
if(Intent.Revision<LastAcceptedRevision)
{
return;
}
// TODO:
// - verificare il turno;
// - ricalcolare il path;
// - validare abilità, risorse, target e mappa;
// - salvare lo snapshot canonico.
```

```
Intent.bConfirmed=true;
Intent.PlanningState=EPlanningState::Ready;
LastAcceptedRevision=Intent.Revision;
BroadcastToTeam(Intent,true);
}
voidUSharedIntentComponent::BroadcastToTeam(
FSharedIntentDataIntent,
boolbReliable
)
{
APlayerController*SenderController=
Cast<APlayerController>(GetOwner());
if(!SenderController||!GetWorld())
{
return;
}
AIntentPlayerState*SenderState=
SenderController->GetPlayerState<AIntentPlayerState>();
if(!SenderState)
{
return;
}
// Non fidarsi mai del SourcePlayerId mandato dal client.
Intent.SourcePlayerId=SenderState->GetPlayerId();
for(
FConstPlayerControllerIteratorIt=
GetWorld()->GetPlayerControllerIterator();
It;
++It
)
{
APlayerController*RecipientController=It->Get();
if(!RecipientController)
{
continue;
}
AIntentPlayerState*RecipientState=
RecipientController->GetPlayerState<AIntentPlayerState>();
if(!RecipientState||
RecipientState->TeamId!=SenderState->TeamId)
```

```
{
continue;
}
```

```
USharedIntentComponent*RecipientComponent=
RecipientController
->FindComponentByClass<USharedIntentComponent>();
```

```
if(!RecipientComponent)
```

- `{ continue;`

- `} if (bReliable)`

- `{`

```
RecipientComponent->ClientReceiveConfirmed(Intent);
```

- `} else`

- `{`

```
RecipientComponent->ClientReceivePreview(Intent);
```

      - `}`

   - `}`

- `}`

- `void USharedIntentComponent::ClientReceivePreview_Implementation( FSharedIntentData Intent`

- `)`

- `{`

   - `OnIntentReceived.Broadcast(Intent);`

- `}`

- `void USharedIntentComponent::ClientReceiveConfirmed_Implementation( FSharedIntentData Intent`

- `)`

- `{`

   - `OnIntentReceived.Broadcast(Intent);`

- `}`

Il codice applica la proprietà più importante: **il filtro di squadra avviene sul server** . Il client nemico non riceve l’RPC. La validazione delle Server RPC è supportata direttamente dagli specifier `Server` , `Reliable` / `Unreliable` e `WithValidation` di Unreal. 17

Per una build reale, aggiungere:

- controllo del `TurnId` contro un GameState autoritativo;

- rate limiter a token bucket;

- limite in byte del payload;

- path canonico server-side;

- reason code di rifiuto;

- acknowledgement all’autore;

- snapshot di healing;

- confronto fra hash del contenuto e revisione;

- log di sicurezza;

- filtri sulle entità visibili;

- test automatici di assenza di leakage.

##### Blueprint pseudo-code
###### Aggiornamento della traiettoria
```
Event IA_PlanMove Triggered
    → Get Mouse World Position
    → Find Path For Selected Character
    → Build SharedIntentData
        TurnId = CurrentTurn
        Revision = Revision + 1
        IntentType = "Move"
        PathPoints = Path
        TargetLocation = LastPathPoint
    → Render Local Ghost Immediately
    → Store as PendingIntent
```

###### Invio con throttle
```
Timer ogni 0.10 secondi
    → Branch: PendingIntent è cambiato?
        True
            → SharedIntentComponent.SubmitPreview(PendingIntent)
            → Set PendingIntentChanged = False
```

###### Ricezione di un intento alleato
```
Event OnIntentReceived(Intent)
```

- `→ Find or Spawn Local IntentGhost by SourcePlayerId`

- `→ Set Ghost Path`

- `→ Set Target Marker`

- `→ Set AOE Shape`

- `→ Set Label Icon`

- `→ Set Ready Visual from Intent.PlanningState`

###### Conferma
```
On Confirm Button
```

- `→ PendingIntent.PlanningState = Ready`

- `→ SharedIntentComponent.ConfirmIntent(PendingIntent)`

- `→ Show "Conferma in corso"`

La UI non deve applicare un ulteriore filtro di sicurezza per squadra: può farlo come controllo difensivo, ma la garanzia reale resta il routing server-side.

##### NetMulticast, NetCull e Replication Graph
`NetMulticast` può essere usato per eventi pubblici, come l’animazione di inizio risoluzione, ma non per inviare dati tattici privati. Le RPC multicast vengono eseguite dal server e dai client ai quali l’attore viene replicato; trasformare la relevancy in un sistema di autorizzazione tattica aumenta il rischio di errori. 18

`NetCullDistanceSquared` è utile per elementi pubblici vicini, non per gli intenti. Le intenzioni di un alleato devono essere visibili anche dall’altro lato della mappa e invisibili a un nemico adiacente.

Un Replication Graph custom è giustificato soltanto se il progetto cresce verso molte squadre, spettatori con permessi diversi o decine di giocatori. Per un quattro contro quattro, Client RPC mirate e componenti owner-only sono molto più semplici da verificare.

##### C# e plugin bridge
UnrealCLR integra un runtime .NET e dichiara supporto a C# e .NET; UnrealSharp è un altro progetto open source attivo che permette di creare classi Unreal in C# e supporta hot reload. Entrambi sono 19 strumenti di terze parti, non parte del toolchain ufficiale di Epic.

La raccomandazione è:

|Uso|C++ minimo|C# bridge|
|---|---|---|
|RPC e autorità server|Raccomandato|Evitare nel core iniziale|
|Struct replicate|Raccomandato|Solo dopo spike tecnico|
|Renderer di ghost|Blueprint|Possibile|
|Tool editor|Blueprint/C++|Buon candidato|
|Utility di import JSON|C++ o Blueprint|Buon candidato|
|Dedicated server|Baseline ufficiale|Verificare packaging e piattaforme|
|ABI pubblica per mod|Interfacce Unreal|Non dipendere dal bridge|

La scelta safe è imparare il sottoinsieme di C++ necessario a Unreal: `UCLASS` , `USTRUCT` , `UPROPERTY` , `UFUNCTION` , puntatori gestiti da `UPROPERTY` , componenti e RPC. La logica visiva può restare in Blueprint. Un bridge C# può essere sperimentato in un branch separato, senza renderlo una dipendenza del protocollo multiplayer.

#### Test, metriche e criteri di accettazione
Unreal fornisce Network Emulation per simulare latenza e perdita di pacchetti, Networking Insights per analizzare il traffico e Automation/Functional Testing per test di basso e alto livello. Gauntlet può avviare sessioni che includono server e più client ed è adatto a test multiplayer automatizzati. 20

##### Piano di test
|Livello|Test principali|
|---|---|
|Unit test|Serializzazione, confronto revisioni, validazione payload, compressione path,<br>regole di conflitto|
|Component<br>test|Throttle, rate limiter, creazione snapshot, rollback, eliminazione cronologia|
|Integration<br>test|Client→server→soli alleati, conferma, reject, reconnect, timeout|
|Security test|Client modifica SourcePlayerId, TeamId, TurnId, target, dimensione array o<br>frequenza|
|Visibility test|Packet capture su client nemico; nessun payload o actor reference degli intenti<br>avversari|
|Network test|50–250 ms RTT, jitter, 1–10% packet loss, riordinamento e disconnect|
|Load test|Otto e sedici client che aggiornano simultaneamente path e disegni|
|Mod test|Manifest compatibile, incompatibile, hash errato, intento sconosciuto, renderer<br>mancante|
|UX test|Comprensione piano, tempo per risolvere conflitti, leggibilità con overlay<br>sovrapposti|
|Accessibility<br>test|Deuteranopia/protanopia, scala UI, overlay senza colori, controller e mouse|
|Soak test|Match ripetuti per almeno 30–60 minuti senza crescita non controllata di cache o<br>ghost|

La Network Emulation di Unreal consente di configurare separatamente lag e packet loss in entrata e uscita, anche nelle opzioni multiplayer di Play In Editor. Networking Insights permette di ispezionare 21 pacchetti, RPC, proprietà e traffico nel tempo.

##### Target tecnici
|Metrica|Target alpha|Target release|
|---|---|---|
|Risposta della preview locale|< 33 ms|< 16,7 ms a 60 FPS|
|Autore→alleato, mediana|< 150 ms|< 100 ms|
|Autore→alleato, p95 con RTT≤120 ms|< 250 ms|< 200 ms|
|Frequenza preview client|≤10 Hz|≤12 Hz|

|Metrica|Target alpha|Target release|
|---|---|---|
|Banda tipica per client|< 8 KB/s|< 5 KB/s|
|Banda di picco per client|< 15 KB/s|< 10 KB/s|
|Elaborazione server per update, otto player|< 2 ms|< 1 ms|
|Convergenza dopo perdita pacchetto|< 1,5 s|< 500 ms|
|Payload massimo preview|2 KB|1 KB|
|Path massimo|48 punti|32 punti o celle compresse|
|Leakage verso nemici|0|0|
|Crash/desync durante soak|0|0|
|Conflitti hard non rilevati|< 5%|< 1%|

I target di latenza non derivano da un limite imposto da Unreal: sono obiettivi di prodotto. La ricerca sul contesto visivo condiviso indica però che aggiornamenti ritardati riducono i benefici collaborativi, quindi la latenza dell’overlay deve essere trattata come metrica UX, non solo di rete. 22

##### Metriche di gameplay e UX
Durante playtest controllati, confrontare una build con intenti condivisi e una build con soli ping:

|Metrica|Segnale desiderato|
|---|---|
|Conflitti fra alleati per turno|Riduzione|
|Fuoco amico non intenzionale|Riduzione|
|Doppie interazioni sulla stessa risorsa|Riduzione|
|Tempo medio per raggiungere Ready|Stabile o inferiore|
|Modifiche negli ultimi tre secondi|Riduzione|
|Ping generici per turno|Riduzione|
|Messaggi chat necessari per coordinare un<br>focus|Riduzione|
|Accuratezza nel descrivere il piano alleato|Aumento|
|Percezione di clutter|Non oltre soglia definita|
|Utilizzo dei filtri overlay|Individuazione delle fonti di clutter|
|Rollback per turno|Misurare, senza assumere che meno sia sempre<br>meglio|
|Win-rate gruppi con voice vs senza voice|Riduzione del divario, senza annullarlo|

##### Criteri di accettazione
La feature è accettata per la beta quando:

- tutti gli alleati vedono traiettoria, target, AOE, label e stato Ready;

- un nemico non riceve alcuna intenzione avversaria, verificato tramite log e packet capture;

- pacchetti preview fuori ordine non riportano l’overlay a una revisione precedente;

- la perdita di un update non impedisce la convergenza allo stato più recente;

- conferma e lock arrivano in modo affidabile;

- un client non può cambiare `SourcePlayerId` , `TeamId` o destinatari;

- un client non può confermare un turno già chiuso;

- path e target vengono validati dal server;

- modificare una mossa dopo Ready riporta correttamente il giocatore in Editing;

- il rollback crea una nuova revisione monotona;

- al reconnect il client ricostruisce tutti gli intenti alleati correnti;

- i warning di conflitto indicano partecipanti, area e revisione;

- l’utente può nascondere disegni, AOE o singoli compagni senza influenzare il gameplay;

- lo stato bloccato usato dal resolver è immutabile;

- il sistema regge otto client con 5% packet loss e 150 ms RTT senza desync permanente;

- le mod non approvate dal server non possono registrare payload o regole di rete.

#### Roadmap, modding, rischi e risorse
Le stime seguenti assumono un programmatore full-time, UI provvisoria e disponibilità dei sistemi dipendenti. Con sviluppo part-time, il calendario si estende in proporzione.

##### Roadmap
|Milestone|Deliverable|Dipendenze|Stima|
|---|---|---|---|
|Fondazioni|Turn state machine, team ID, PlayerController/<br>PlayerState custom, struct intent, test PIE a<br>quattro client|Sessione<br>multiplayer e<br>GameMode|7–10<br>giorni|
|Alpha tecnica|Traiettoria, target, AOE semplice, RPC team-<br>only, preview locale, conferma e Ready|Pathfinder e ability<br>targeting minimi|18–24<br>giorni|
|Alpha<br>giocabile|Ghost, overlay squadra, lock del turno,<br>snapshot autoritativo, reject e timeout|Resolver del turno|10–15<br>giorni|
|Beta rete|Delta/revisioni, snapshot healing, reconnect,<br>rate limiting, Networking Insights|Dedicated server<br>stabile|10–15<br>giorni|
|Beta<br>gameplay|Conflict engine, notifiche, cronologia, rollback,<br>disegno mappa|Timeline del<br>movimento e shape<br>AOE|15–22<br>giorni|
|Feature mod<br>finale|Registry pubblico, Gameplay Tags, Data Asset,<br>import JSON, manifest e whitelist|Schema protocollo<br>congelato|10–15<br>giorni|
|Polish|Accessibilità, performance, anti-spam,<br>telemetria, test Gauntlet, UX pass|Tutte le milestone<br>precedenti|10–15<br>giorni|

**Totale realistico:** circa **80–116 giorni-persona** , equivalenti a **16–23 settimane full-time** per una singola persona. Un MVP ridotto senza disegno, cronologia avanzata, rollback e modding può essere completato in circa **7–10 settimane** .

Le dipendenze critiche sono:

- identificazione affidabile delle squadre;

- state machine Planning → Locked → Resolution;

- pathfinder multilivello;

- sistema di targeting e shape AOE;

- ID stabili delle celle e degli attori;

- regole di visibilità del fog of war;

- resolver temporale per valutare collisioni e fuoco amico;

- dedicated server e reconnect.

##### API di modding
Unreal supporta plugin contenenti codice e dati; i Primary Data Asset possono essere caricati tramite Asset Manager e i Gameplay Tag possono essere definiti anche da file di configurazione dei plugin. Game Features e Modular Gameplay permettono di attivare funzionalità modulari, ma la documentazione corrente li segnala ancora come sistemi da utilizzare con cautela nelle build distribuite. 23

L’API pubblica dovrebbe esporre:

```
classIIntentTypeProvider;
classIIntentRenderer;
classIIntentConflictRule;
classIIntentPayloadValidator;
classIIntentDescriptionProvider;
```

Ogni intento registrato deve dichiarare:

- ID e versione;

- tag gerarchico; • payload ammesso; • dimensione massima; • renderer;

- validatore server; • regole di conflitto;

- frequenza massima;

- policy di visibilità;

- icona e chiavi di localizzazione;

- compatibilità con replay e spectator.

###### Esempio JSON: nuova etichetta
```
{
"schemaVersion":1,
```

```
"contentType":"IntentLabel",
"id":"mod.rescue.label.cover_me",
"tag":"Intent.Label.CoverMe",
"displayNameKey":"label.cover_me.name",
"descriptionKey":"label.cover_me.description",
"icon":"/RescueMod/UI/T_Intent_CoverMe",
"sortOrder":40,
"allowedModes":[
"Casual",
"Custom"
]
}
```

###### Esempio JSON: nuovo tipo di intento
```
{
"schemaVersion":1,
"contentType":"IntentType",
"id":"mod.breach.intent.remote_detonation",
"version":"1.0.0",
"tag":"Intent.Interact.RemoteDetonation",
"renderer":"IntentRenderer.TargetAndRadius",
"validator":"IntentValidator.RegisteredInteractable",
"visibility":"TeamOnly",
"maxUpdateHz":8,
"maxPayloadBytes":256,
"payload":{
"fields":[
{
"name":"targetObjectId",
"type":"NetObjectId",
"required":true
},
{
"name":"radiusCells",
"type":"UInt8",
"min":1,
"max":12
}
]
},
"conflictRules":[
"Conflict.ExclusiveInteractable",
"Conflict.FriendlyFireArea"
]
}
```

In ranked multiplayer, il server deve accettare soltanto manifest firmati o inclusi nella propria whitelist. Tutti i client devono possedere la stessa versione delle mod che influenzano simulazione, intenti o

conflitti. Una mod puramente cosmetica può essere client-side solo se non modifica dimensione, posizione o semantica dell’overlay in modo competitivo.

I plugin di codice nativo hanno accesso ampio al processo e non devono essere scaricati ed eseguiti automaticamente da server sconosciuti. La prima versione del modding dovrebbe quindi consentire:

- nuove label;

- icone;

- colori e pattern;

- shape basate su renderer già approvati;

- nuovi intenti composti da payload dichiarativi;

- conflict rule parametrizzate già presenti nel gioco.

Il codice C++ arbitrario rappresenta una fase successiva e separata.

##### Rischi e mitigazioni
|Rischio|Impatto|Mitigazione|
|---|---|---|
|Leakage degli intenti ai<br>nemici|Critico|Routing server-side per destinatario, packet capture<br>automatica, nessun multicast privato|
|Client che falsifica identità o<br>team|Critico|Identità derivata dal PlayerController/PlayerState<br>server-side|
|Spam di RPC|Alto|Token bucket, frequenza massima, payload cap, mute<br>temporaneo|
|Coda reliable congestionata|Alto|Preview unreliable; reliable solo per eventi finali|
|Overlay illeggibile|Alto|Filtri, opacità, focus mode, offset delle spline, pattern|
|Preview interpretata come<br>risultato certo|Alto|Linguaggio visivo “ghost”, warning e animazioni non<br>definitive|
|Conflitti con falsi positivi|Medio|Warning non bloccanti e rule ID spiegabile|
|Path server diverso dal<br>client|Alto|Canonical path nell’ack; correzione visuale breve|
|Pacchetto finale preview<br>perso|Medio|Snapshot healing e conferma reliable|
|Rollback troppo complesso|Medio|Revision rollback, non rollback completo della<br>simulazione|
|Disegni tossici o spam|Medio|Nessun testo libero, limiti, TTL, mute e report|
|Mod che altera il vantaggio<br>competitivo|Critico|Manifest hash, whitelist, server authority, mode<br>separation|
|Dipendenza da bridge C#|Alto|Core ufficiale C++/Blueprint; bridge isolato e opzionale|
|Schema mod instabile|Alto|Modding sviluppato per ultimo, dopo protocol freeze|
|Database indisponibile|Basso|DB fuori dal percorso critico|

|Rischio|Impatto|Mitigazione|
|---|---|---|
|Reconnect durante il lock|Medio|Snapshot immutabile server-side e modalità spettatore<br>temporanea|

##### Risorse prioritarie
|Priorità<br>Essenziale|Risorsa<br>Networking Overview di Unreal<br>24|Utilità<br>Modello autoritativo client-server|
|---|---|---|
|Essenziale|Remote Procedure Calls<br>25|Server, Client, NetMulticast, reliability e<br>validazione|
|Essenziale|Replicate Actor Properties<br>26|`Replicated` ,<br>`ReplicatedUsing` e<br>condizioni|
|Essenziale|Actor Relevancy<br>27|Owner relevancy, distanza e<br>personalizzazione|
|Essenziale|Multiplayer Programming Quick Start<br>28|Primo progetto multiplayer C++|
|Essenziale|Testing Multiplayer e Network<br>Emulation<br>29|Test multi-client, lag e packet loss|
|Essenziale|Networking Insights<br>30|Profilazione di RPC, proprietà e banda|
|Alta|Automation e Gauntlet<br>31|Test automatici con server e client|
|Alta|Gameplay Tags<br>32|Classificazione data-driven di intenti e label|
|Alta|Data Assets e Asset Manager<br>33|Definizioni caricabili e moddabili|
|Alta|Data Registries<br>34|Registro globale dei tipi di intento|
|Alta|Plugins e Game Features<br>35|Packaging modulare delle mod|
|Utile|Lyra Sample Game<br>36|Esempio Epic di architettura modulare<br>multiplayer|
|UX|Frozen Synapse<br>37|Path, ordini e preview nella pianificazione|
|UX|Phantom Brigade<br>38|Timeline, ghost e leggibilità delle azioni|
|UX|Workspace awareness research<br>39|Principi per presenza e coordinamento<br>visuale|
|Futuro|Network Prediction e Mover<br>40|Rollback e resimulazione della simulazione|
|Futuro|GGPO SDK<br>41|Pattern di save, load e rollback<br>deterministico|
|Opzionale|UnrealCLR<br>42|Esperimento di integrazione .NET|
|Opzionale|UnrealSharp<br>43|Alternativa C# attiva, non ufficiale|

La decisione architetturale più importante è mantenere separati **dato autoritativo** , **trasporto di rete** e **rappresentazione grafica** . Il server conserva e filtra gli intenti; la rete trasporta revisioni compatte;

ogni client costruisce localmente spline, AOE e ghost. Questa separazione protegge la privacy tattica, limita la banda, semplifica il modding e permette di migliorare la UI senza cambiare il protocollo del match.

> 1 Awareness and coordination in shared workspaces https://dl.acm.org/doi/pdf/10.1145/143457.143468?utm_source=chatgpt.com

2 5 37 Frozen Synapse - Game

https://www.matrixgames.com/product/frozen-synapse?utm_source=chatgpt.com

3 6 8 25 Remote Procedure Calls in Unreal Engine

https://dev.epicgames.com/documentation/unreal-engine/remote-procedure-calls-in-unreal-engine? utm_source=chatgpt.com

> 4 GGPO | Rollback Networking SDK for Peer-to-Peer Games

https://www.ggpo.net/?utm_source=chatgpt.com

7 32 Using Gameplay Tags in Unreal Engine

https://dev.epicgames.com/documentation/unreal-engine/using-gameplay-tags-in-unreal-engine?utm_source=chatgpt.com

###### Unreal Python 5.6 (Experimental) documentation

https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/FastArraySerializer? application_version=5.6&utm_source=chatgpt.com

###### Gameplay Framework in Unreal engine

https://dev.epicgames.com/documentation/unreal-engine/gameplay-framework-in-unreal-engine?utm_source=chatgpt.com

11 27

###### Actor Relevancy in Unreal Engine

https://dev.epicgames.com/documentation/unreal-engine/actor-relevancy-in-unreal-engine?utm_source=chatgpt.com

###### Balancing Blueprint and C++ | Unreal Engine 4.27 ...

https://dev.epicgames.com/documentation/en-us/unreal-engine/balancing-blueprint-and-cplusplus? application_version=4.27&utm_source=chatgpt.com

###### 13

###### Setting Up Visual Studio

https://dev.epicgames.com/documentation/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplusprojects-in-unreal-engine?utm_source=chatgpt.com

###### 14

###### CPP Only Example | Unreal Engine 5.8 Documentation

https://dev.epicgames.com/documentation/unreal-engine/cpp-only-example?utm_source=chatgpt.com

15 29

###### Testing Multiplayer in Unreal Engine

https://dev.epicgames.com/documentation/unreal-engine/testing-multiplayer-in-unreal-engine?utm_source=chatgpt.com

###### Rider Setup Guide | Unreal Engine 5.8 Documentation

https://dev.epicgames.com/documentation/unreal-engine/rider-setup-guide?utm_source=chatgpt.com

###### UFunctions in Unreal Engine

https://dev.epicgames.com/documentation/unreal-engine/ufunctions-in-unreal-engine?utm_source=chatgpt.com

###### 18

###### Networking Overview | Unreal Engine 4.27 Documentation

https://dev.epicgames.com/documentation/unreal-engine/networking-overview? application_version=4.27&utm_source=chatgpt.com

19 42 nxrighthere/UnrealCLR: Unreal Engine .NET 6 integration

https://github.com/nxrighthere/UnrealCLR?utm_source=chatgpt.com

###### 20 Using Network Emulation in Unreal Engine

https://dev.epicgames.com/documentation/unreal-engine/using-network-emulation-in-unreal-engine? utm_source=chatgpt.com

> 21 Play In Editor Multiplayer Options in Unreal Engine https://dev.epicgames.com/documentation/unreal-engine/play-in-editor-multiplayer-options-in-unreal-engine? utm_source=chatgpt.com

###### The Use of Visual Information in Shared Visual Spaces

> 22 The Use of Visual Information in Shared Visual Spaces https://sfussell.hci.cornell.edu/pubs/Manuscripts/p318-kraut.pdf?utm_source=chatgpt.com

###### 23 35 Plugins in Unreal Engine

https://dev.epicgames.com/documentation/unreal-engine/plugins-in-unreal-engine?utm_source=chatgpt.com

###### Networking Overview for Unreal Engine

> 24 Networking Overview for Unreal Engine https://dev.epicgames.com/documentation/unreal-engine/networking-overview-for-unreal-engine?utm_source=chatgpt.com

###### 26 Replicate Actor Properties in Unreal Engine

https://dev.epicgames.com/documentation/unreal-engine/replicate-actor-properties-in-unreal-engine? utm_source=chatgpt.com

###### 28 Multiplayer Programming Quick Start for Unreal Engine

https://dev.epicgames.com/documentation/unreal-engine/multiplayer-programming-quick-start-for-unreal-engine? utm_source=chatgpt.com

###### 30 Networking Insights in Unreal Engine

https://dev.epicgames.com/documentation/unreal-engine/networking-insights-in-unreal-engine?utm_source=chatgpt.com

###### 31 Automation Test Framework in Unreal Engine

https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unreal-engine? utm_source=chatgpt.com

> 33 Data Assets in Unreal Engine https://dev.epicgames.com/documentation/unreal-engine/data-assets-in-unreal-engine?utm_source=chatgpt.com

###### 34

###### Data Registries in Unreal Engine

https://dev.epicgames.com/documentation/unreal-engine/data-registries-in-unreal-engine?utm_source=chatgpt.com

###### 36 Lyra Sample Game in Unreal Engine

https://dev.epicgames.com/documentation/unreal-engine/lyra-sample-game-in-unreal-engine?utm_source=chatgpt.com

###### 38

###### Phantom Brigade

https://braceyourselfgames.com/phantom-brigade/?utm_source=chatgpt.com

###### A Descriptive Framework of Workspace Awareness for ...

> 39 A Descriptive Framework of Workspace Awareness for ... https://collablab.northwestern.edu/CollabolabDistro/nucmc/GutwinGreenberg_FrameworkWorkspaceAwareness.pdf? utm_source=chatgpt.com

###### NetworkPrediction | Unreal Engine 5.8 Documentation

###### 40

https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/NetworkPrediction?utm_source=chatgpt.com

> 41 pond3r/ggpo: Good Game, Peace Out Rollback Network SDK

https://github.com/pond3r/ggpo?utm_source=chatgpt.com

###### 43

###### UnrealSharp is a plugin to Unreal Engine 5, which enables ...

https://github.com/UnrealSharp/UnrealSharp?utm_source=chatgpt.com

---

## PRD stampabile — architettura tecnica, networking, modding e contratti dati

### Architettura tecnica e networking
#### Stack raccomandato.
|Livello|Tecnologia|
|---|---|
|Motore|Unreal Engine 5, release stabile|

|Livello|Tecnologia|
|---|---|
|Core simulazione|C++|
|Prototipi e presentazione|Blueprints|
|Abilità e status|Gameplay Ability System|
|Tassonomia|Gameplay Tags|
|Input|Enhanced Input|
|UI|UMG; CommonUI dopo una prova tecnica|
|Dati|Primary Data Assets, Data Registry, JSON per import/mod|
|Multiplayer|Replication Unreal, RPC e server autorevole|
|Servizi online|Interfaccia astratta; EOS o piattaforma in una fase successiva|
|Build|Unreal Build Tool e Automation Tool|
|Version control|Git LFS per team piccolo; valutare Perforce con crescita del team|
|Test|Automation Tests, functional tests e simulatori headless|

Enhanced Input supporta azioni astratte, mapping context prioritari, modificatori, trigger e rimappatura; è quindi adatto a distinguere input di mappa, targeting, disegno e UI. 8

UMG utilizza Widget Blueprint per layout e logica UI. CommonUI offre strumenti per interfacce multilivello, routing degli input e supporto multipiattaforma, ma deve essere adottato soltanto dopo un piccolo proof of concept, evitando di introdurlo in tutte le schermate prima di averne validato il 9 workflow.

#### C++ contro Blueprint.
|Area|C++|Blueprint|Scelta|
|---|---|---|---|
|Simulatore del<br>turno|Controllo, testabilità,<br>determinismo|Difficile da revisionare su<br>grafi grandi|C++|
|Pathfinding|Migliore per algoritmi e<br>profiling|Utile per debug e parametri|C++ con API<br>Blueprint|
|Sistemi<br>ambientali|Core in codice|Configurazione ed effetti<br>visivi|Ibrido|
|Abilità|Base e validazione|Composizione e<br>presentazione|GAS ibrido|
|UI|View model e servizi|Layout e animazioni|Ibrido|
|Personaggi|Contratti e componenti<br>base|Asset e varianti|Ibrido|
|Mod content|Loader e validator|Asset data-only|Core C++,<br>contenuti dati|

|Area|C++|Blueprint|Scelta|
|---|---|---|---|
|Networking|Replication e sicurezza|Prototipi limitati|C++|
|VFX e audio|Solo API essenziali|Iterazione rapida|Blueprint/Niagara|
|Regole<br>classificate|Più controllabili|Rischio di logica dispersa|C++ o dati validati|

Epic evidenzia che C++ fornisce maggiore controllo su prestazioni, rete, serializzazione e API, mentre Blueprint facilita creazione e iterazione. RefactorTactics adotterà quindi la regola: **C++ definisce ciò che** 10 **è possibile; dati e Blueprint definiscono quale variante viene utilizzata** .

**C# nel progetto.** Unreal non usa C# come linguaggio gameplay principale. L’esperienza C# resta utile per classi, interfacce, generics, eventi e architettura, ma occorre imparare reflection Unreal, macro, garbage collection, moduli e lifecycle degli UObject. La guida ufficiale per sviluppatori provenienti da Unity è un riferimento utile per la traduzione concettuale. 11

|C#|Unreal C++|
|---|---|
|`class` serializzabile|`UCLASS()`|
|`struct` dati|`USTRUCT()`|
|attributo/proprietà editor|`UPROPERTY()`|
|metodo esposto|`UFUNCTION()`|
|`List<T>`|`TArray<T>`|
|`Dictionary<K,V>`|`TMap<K,V>`|
|evento/delegate|Delegate Unreal|
|riferimento managed|`TObjectPtr<T>` o soft reference|
|interface|`UINTERFACE` + interfaccia C++|
|assembly|Module Unreal|
|reflection runtime|Unreal Header Tool|

#### Struttura dei moduli C++.
#### `Source/`

- `├── RefactorTactics/`

- `│   ├── Core/`

- `│   ├── Characters/`

- `│   ├── Map/`

- `│   ├── Planning/`

- `│   ├── Simulation/`

- `│   └── UI/`

- `├── RefactorTacticsEditor/`

- `│   ├── Validators/`

```
│   ├── MapTools/
│   └── ModTools/
└── RefactorTacticsTests/
```

Per il primo anno può essere mantenuto un singolo modulo runtime con cartelle interne. La suddivisione in più moduli avviene quando le dipendenze e i tempi di compilazione lo giustificano.

#### Plugin Unreal raccomandati.
|Plugin|Utilizzo|Fase|
|---|---|---|
|GameplayAbilities|Abilità, costi, cooldown, effetti|Vertical slice|
|GameplayTags|Tassonomia e query|Da subito|
|GameplayTasks|Task asincroni di abilità|Vertical slice|
|EnhancedInput|Input e context|Da subito|
|TargetingSystem|Valutare preset di targeting|Alpha|
|CommonUI|UI e routing cross-input|Dopo proof of concept|
|DataRegistry|Cataloghi di dati|Alpha|
|GameFeatures|Pacchetti modulari e mod|Beta/mod release|
|OnlineSubsystemEOS o Online Services|Sessioni e servizi|Alpha|
|Niagara|VFX ambientali e abilità|Vertical slice|

Il Gameplay Ability System richiede l’attivazione del plugin e le dipendenze `GameplayAbilities` , `GameplayTags` e `GameplayTasks` . Il sistema supporta ability, task, attributi ed effetti, inclusi costi e cooldown. 12

Esempio `Build.cs` :

```
PublicDependencyModuleNames.AddRange(
newstring[]
{
"Core",
"CoreUObject",
"Engine",
"InputCore",
"EnhancedInput",
"UMG",
"AIModule",
"GameplayAbilities",
"GameplayTags",
"GameplayTasks"
});
```

Questo file è scritto in C# perché Unreal Build Tool utilizza script di build C#, ma il gameplay continua a essere implementato in Unreal C++.

**Simulatore autorevole.** Lo stato logico deve essere separato dalla rappresentazione visuale.

```
Authoritative Match State
```

- `├── MapState`

- `├── Units`

- `├── Effects`

- `├── Objectives`

- `├── TurnState`

- `├── CommandSnapshot`

- `├── RandomSeed`

- `└── TurnLog`

Le animazioni non decidono quando un colpo avviene. Il simulatore produce eventi temporizzati; il client riproduce animazioni e VFX corrispondenti.

<!-- Start of picture text -->
UI e input client<br>Preview locale Comandi verso il server<br>Validazione Cataloghi dati validati<br>Snapshot del turno Map State Unit State e GAS<br>Simulatore C++<br>Turn Log autorevole<br>Replica e RPC ai client<br>Animazioni, VFX, UI<br><!-- End of picture text -->

<u>Scarica il diagramma vettoriale dell’architettura</u>

#### Modello di rete.

|Modello|Vantaggi|Svantaggi|Decisione|
|---|---|---|---|
|Peer-to-peer con<br>host|Costi infrastrutturali<br>iniziali ridotti|Host advantage,<br>migrazione e sicurezza<br>complesse|Solo prototipo<br>locale|
|Listen server|Semplice per test e<br>partite private|Un giocatore è autorità e<br>host|Supporto<br>custom|
|Dedicated<br>authoritative|Equità, anti-cheat e stato<br>centrale|Costo operativo e<br>deployment|Produzione<br>competitiva|
|Lockstep peer|Poco traffico per<br>simulazioni<br>deterministiche|Ritardo del peer peggiore e<br>gestione disconnessioni|Non principale|
|Server simulation<br>con snapshot|Controllo completo e<br>replay|Maggiore lavoro backend|Scelta|

In Unreal gli Actor possono replicare proprietà e chiamate di funzione; la property replication e gli RPC sono i meccanismi fondamentali per distribuire stato dal server ai client. 13

**Separazione della visibilità.** Non è sufficiente nascondere graficamente gli intenti avversari: tali intenti non devono essere replicati al loro processo.

Architettura proposta:

#### `Server`

- `├── CanonicalIntentStore`

- `│   ├── Team A intents`

- `│   └── Team B intents`

- `├── TeamIntentRelay A`

- `│   └── snapshot sanitizzati inviati soltanto ai client A`

- `└── TeamIntentRelay B`

- `└── snapshot sanitizzati inviati soltanto ai client B`

Una semplice proprietà replicata su un Actor globale è vietata per gli intenti. Il server invia snapshot di squadra tramite:

- Client RPC indirizzati ai singoli compagni; oppure • subobject/Actor relay specifici per ogni client, contenenti la sola vista autorizzata.

Il server conserva sempre il dato canonico completo.

#### Affidabilità dei messaggi.
|Messaggio|Direzione|Affidabilità|
|---|---|---|
|Preview percorso|Client→server|Unreliable, sequenziato|
|Preview abilità|Client→server|Unreliable, sequenziato|

|Messaggio|Direzione|Affidabilità|
|---|---|---|
|Broadcast preview al team|Server→client|Unreliable, sequenziato|
|Ping|Client→server→team|Reliable con rate limit|
|Ready/unready|Client→server|Reliable|
|Commit finale|Client→server|Reliable|
|Snapshot del turno|Server interno|Immutabile|
|Risultato autorevole|Server→client|Reliable o stato replicato|
|Disegno tattico|Unreliable, con chunk e scadenza|Unreliable|

I pacchetti preview includono un `SequenceNumber` ; i client ignorano aggiornamenti più vecchi.

<!-- Start of picture text -->
Giocatore Client Server autorevole Client alleati Client nemici<br>Modifica percorso o abilità<br>PreviewIntent(sequence)<br>Valida e sanitizza<br>TeamIntentPreview<br>Nessun dato dell'intento ricevuto<br>Ready<br>CommitIntent<br>Crea snapshot quando il turno si chiude<br>Risolve il turno<br>TurnResult<br>TurnResult<br>TurnResult pubblico<br>Giocatore Client Server autorevole Client alleati Client nemici<br><!-- End of picture text -->

**Determinismo e replay.** Il progetto deve utilizzare:

- ordine stabile degli ID;

- costi interi;

- seed registrato per ogni turno;

- niente iterazione dipendente dall’ordine non garantito di mappe hash;

- niente risultato gameplay deciso da frame rate o animazione;

- versioni esplicite di regole e contenuti;

- command log per ricostruire una partita.

Un replay registra `MatchConfig` , hash dei contenuti, seed iniziale, snapshot degli intenti e risultati per turno. Durante le prime milestone è sufficiente un replay logico senza registrazione video.

#### Entity relationship.
<!-- Start of picture text -->
MATCH<br>contiene<br>utilizza TEAM<br>contiene<br>MAP_STATE PLAYER<br>contiene controlla<br>MOD_PACKAGE CELL crea UNIT<br>possiede<br>connette fornisce possiede equipaggia<br>EDGE DATA_DEFINITION EFFECT_INSTANCE PLANNED_INTENT ABILITY_LOADOUT<br>estende seleziona riferisce<br>ABILITY_DEFINITION<br><!-- End of picture text -->

#### <u>Scarica il diagramma vettoriale delle entità</u>

#### Requisiti non funzionali iniziali.
|Requisito|Target|
|---|---|
|Frame rate client|60 FPS su hardware target medio|
|Tick simulazione|Non legato al frame rate|
|Path query singola|Mediana inferiore a 2 ms su mappa MVP|
|Preview completa|Inferiore a 50 ms lato client|
|Update intenti|8–12 Hz|
|Riconnessione|Entro due turni, target alpha|
|Turn resolution|Inferiore a 100 ms server per match MVP|
|Crash-free sessions|Oltre 99% in beta|
|Divergenze replay|Zero nei test deterministici|
|Intent leak test|Zero pacchetti non autorizzati|

### Modding e contratti dati
Il supporto alle mod viene pubblicato dopo MVP, alpha e beta. Tuttavia tre decisioni devono essere applicate dal primo giorno:

1. ogni contenuto possiede un ID stabile;

- le regole configurabili risiedono in asset o strutture dati;

- il caricamento passa da cataloghi e validator, non da riferimenti hard-coded.

Unreal consente di gestire contenuti attraverso Asset Manager, Primary Data Asset, Primary Asset Label e Data Registry. I Gameplay Tags forniscono classificazione gerarchica, mentre il Game Features subsystem gestisce plugin di feature installabili e disinstallabili. Questi sistemi costituiscono una base utile, ma RefactorTactics necessita comunque di manifest, schema, validazione, dipendenze e policy proprie. 14

#### Livelli di modding.
|Livello|Contenuto|Supporto|
|---|---|---|
|Data mod|Terreni, valori, tag, tabelle|Prima release mod|
|Asset mod|Mesh, materiali, audio, VFX|Prima release mod, con limiti|
|Blueprint sandbox|Abilità basate su nodi approvati|Release successiva|
|Map mod|Celle, layer, oggetti e obiettivi|Prima o seconda release mod|
|Mode mod|Regole e scoring data-driven|Seconda release mod|
|Native plugin|C++ arbitrario|Non supportato nel client standard|

Il codice nativo di terze parti non viene caricato nel client retail per ragioni di sicurezza, stabilità e compatibilità. Server e client devono avere lo stesso catalogo di mod, verificato tramite hash.

#### Manifest.
```
{
"schemaVersion":1,
"modId":"aaron.refactortactics.volcanic_pack",
"displayName":"Volcanic Pack",
"version":"1.0.0",
"gameVersion":"0.8.x",
"type":"Content",
"dependencies":[
{
"modId":"refactortactics.core",
"version":">=0.8.0"
}
],
"content":[
"Terrains/Lava.json",
"Abilities/MagmaWall.json"
```

```
],
"networkPolicy":"ServerRequired"
}
```

#### Definizione di terreno.
```
{
"schemaVersion":1,
"id":"Terrain.Lava",
"displayName":"Lava",
"tags":[
"Terrain.Liquid",
"Hazard.Fire",
"Surface.Hot"
],
"movement":{
"baseCost":5,
"blockedByDefault":false,
"requiredTags":[],
"forbiddenTags":[
"Unit.Trait.Flammable"
]
},
"visibility":{
"blocksLineOfSight":false,
"opacity":0.1
},
"turnEffects":[
{
"effectId":"Effect.Burning",
"magnitude":20
}
]
}
```

#### Definizione di abilità.
```
{
"schemaVersion":1,
"id":"Ability.MagmaWall",
"abilityFamily":"Control.Wall",
"range":5,
"cost":{
"energy":2,
"cooldownTurns":3
},
"targeting":{
"type":"LineOfCells",
"length":3,
```

```
"requiresLineOfSight":true
},
"effects":[
{
"type":"AddCellTag",
"tag":"Hazard.Fire",
"durationTurns":2
},
{
"type":"BlockEdges",
"durationTurns":2
}
]
}
```

#### API concettuale.
```
IModContentProvider
├── GetManifest()
├── GetDefinitions()
├── GetDependencies()
└── GetContentHash()
IDataDefinitionValidator
├── ValidateSchema()
├── ValidateReferences()
├── ValidateGameplayTags()
├── ValidateNetworkPolicy()
└── ValidateBalanceLimits()
ITraversalCostProvider
├── CanTraverse(Context, Edge)
└── GetAdditionalCost(Context, Edge)
```

```
IMapEffectProcessor
├── CanProcess(Event)
└── Process(Event, ChangeSet)
```

Le interfacce di runtime sono implementate dal gioco. Una mod data-only seleziona provider già approvati e passa loro configurazioni. Non può inserire funzioni arbitrarie.

#### Versionamento.
|Cambiamento|Strategia|
|---|---|
|Aggiunta campo opzionale|Minor schema|
|Ridenominazione campo|Redirect e migration|

|Cambiamento|Strategia|
|---|---|
|Cambio semantico|Major schema|
|Rimozione di un ID|Deprecation per almeno una release|
|Ridenominazione tag|Gameplay Tag redirect|
|Mod non compatibile|Disabilitazione con errore leggibile|
|Save vecchio|Pipeline di migration|
|Match online|Hash esatto dei contenuti|

**Validator.** Il toolkit mod deve verificare:

- ID duplicati;

- dipendenze mancanti o circolari;

- tag sconosciuti;

- riferimenti ad asset inesistenti;

- costi negativi;

- loop infiniti dichiarativi;

- aree e durate oltre limiti;

- path impossibili;

- mappe senza spawn o obiettivi validi;

- asset oltre budget;

- contenuti non consentiti nelle playlist classificate.

**Distribuzione.** La prima versione può utilizzare una cartella locale e pacchetti importabili. L’integrazione con Steam Workshop o altri cataloghi viene trattata come adapter di distribuzione, non come fondamento del formato.

```
RefactorTactics/
└── Mods/
    └── VolcanicPack/
        ├── manifest.json
        ├── Content/
        ├── Data/
        └── Localization/
```

**Networking delle mod.** All’ingresso in una sessione:

- `Client invia ContentCatalogHash → server confronta hash → se uguale: accesso`

- `→ se mancano mod: elenco dipendenze`

- `→ se versione incompatibile: rifiuto motivato`

- `→ se catalogo classificato: ignora contenuti non approvati`

Le partite classificate accettano soltanto contenuti firmati e inclusi nel catalogo ufficiale della stagione. Le partite custom possono usare mod locali, ma devono mostrare chiaramente che progressione, ranking e statistiche competitive sono disattivati.

---

## PRD e piano di sviluppo — architettura dei sistemi, modelli dati e stack tecnologico

### Architettura dei sistemi e modelli dati
#### Architettura generale
<!-- Start of picture text -->
Griglia e grafo semantico<br>Resolver deterministico Resolved event log Animazioni e UI<br>Abilità e Gameplay Ability<br>System<br>Turn orchestrator Replay e test<br>Input e planning UI Preview locale Submit plan Networking autoritativo Personaggi e loadout<br>Solo alleati Intenti condivisi<br>Planner IA<br><!-- End of picture text -->

Versione adatta alla stampa:

```
CLIENT
  Input + Planning UI
          |
          v
  Preview locale
          |
     ServerSubmitPlan
          v
SERVER AUTORITATIVO
  Networking
      |
      v
  Turn Orchestrator
      |
      +---- Grid e Pathfinding
      +---- Abilità e Status
      +---- Personaggi
      +---- IA
      |
      v
  Resolver deterministico
      |
      +---- Intenti sanificati solo agli alleati
      +---- Stato pubblico a tutti
      +---- Event log di risoluzione a tutti
      |
      v
  Replay + Test + Analytics
```

La separazione segue il Gameplay Framework di Unreal: `GameMode` vive soltanto sul server, mentre `GameState` rappresenta stato pubblico replicato; `PlayerState` contiene informazioni del giocatore che devono sopravvivere alla sostituzione del Pawn. Questo rende essenziale non inserire il piano privato nel `GameState` . 4

#### Moduli di codice
|Modulo|Responsabilità|
|---|---|
|`RTCore`|Identificatori, tag, risultati, errori e tipi condivisi|
|`RTGrid`|Celle, chunk, coordinate e conversioni world-grid|
|`RTPathfinding`|Grafo, A*, provider di costo e cache|
|`RTTurn`|Stato round, validazione, lock e risoluzione|
|`RTAbilities`|Abilità, effetti, targeting e status|
|`RTCharacters`|Chassis, moduli, loadout e affinità|
|`RTNetworking`|RPC, replica, privacy e riconnessione|
|`RTUI`|HUD, preview, timeline e feedback|
|`RTAI`|Generazione candidati e utility|
|`RTAnalytics`|Eventi, metriche e adapter provider|
|`RTModRuntime`|Manifest, schema, caricamento e registrazione|
|`RTEditor`|Tool mappe, validator, heatmap e simulazione|

Le modalità dovrebbero essere plugin separati:

- `Plugins ├── RTMode_PvP ├── RTMode_Roguelike ├── RTContent_Core ├── RTContent_Maps └── RTModRuntime`

Game Features e Modular Gameplay sono stati progettati da Epic proprio per incapsulare feature attivabili e disattivabili; i Data Registries possono inoltre integrare dati provenienti dalle Game Features. 5

#### Modelli fondamentali
|Tipo|Campi principali|Scopo|
|---|---|---|
|`FRTGridCellId`|`X` ,<br>`Y` ,<br>`Layer`|Identità stabile della cella|
|`FRTCellStaticData`|terreno, quota, cover, tag|Dati immutabili della mappa|
|`FRTCellRuntimeState`|effetti, occupazione,<br>revision|Stato dinamico|
|`FRTTraversalEdge`|origine, destinazione, tipo,<br>costo|Collegamento tra celle|

|Tipo|Campi principali|Scopo|
|---|---|---|
|`FRTTraversalProfile`|capacità, immunità, regole<br>costo|Interpretazione della mappa da parte<br>dell’unità|
|`FRTPathRequest`|unità, partenza, arrivo,<br>snapshot|Richiesta deterministica|
|`FRTPlannedAction`|tipo, target, abilità,<br>parametri|Singola parte del piano|
|`FRTPlannedTurn`|giocatore, revisione, azioni|Piano privato completo|
|`FRTTeamIntentView`|geometria e label sanificate|Informazione per gli alleati|
|`FRTResolvedEvent`|fase, ordine, sorgente,<br>payload|Risultato atomico|
|`FRTModManifest`|ID, versione, dipendenze,<br>hash|Compatibilità mod|
|`FRTRunState`|nodo, seed, mazzi, reliquie|Stato roguelike|

#### Sistema di celle
Una decisione tecnica importante è non trasformare ogni cella in un Actor con molti `UActorComponent` .

Il termine “componente della cella” deve descrivere un **frammento dati** , non necessariamente un componente Unreal allocato separatamente. Una mappa con migliaia di celle e decine di UObject per cella avrebbe costi inutili di memoria, garbage collection e aggiornamento.

Struttura proposta:

- `MAP ASSET ├── Chunk`

- `│   ├── Static cell array`

- `│   ├── Traversal edges`

- `│   ├── Cover edges`

- `│   └── Render instances ├── Sparse runtime overrides ├── Environment systems └── Graph revision`

Frammenti semantici:

|Frammento|Dati|
|---|---|
|Movimento|costo base, blocchi, direzioni|
|Superficie|acqua, ghiaccio, metallo, vegetazione|
|Altezza|quota, gradino, caduta|

|Frammento|Dati|
|---|---|
|Copertura|lato, altezza e distruttibilità|
|Visibilità|opacità, fumo, luce|
|Pericolo|fuoco, veleno, elettricità|
|Interazione|porte, terminali, leve|
|Trigger|ingresso, uscita, fine round|
|Suono|rumore prodotto e propagazione|
|Meteo|vento, pioggia, temperatura|

I dati statici possono vivere in array contigui organizzati per chunk. Gli effetti temporanei vengono registrati come override sparsi. La geometria ripetuta dovrebbe essere visualizzata tramite instancing, lasciando gli Actor alle sole entità realmente interattive.

#### Grafo semantico multilivello
La cella è un nodo. Scale, rampe, portali, ascensori e salti sono archi.

<!-- Start of picture text -->
FTraversalEdge<br>├── From<br>├── To<br>├── EdgeType<br>├── BaseCost<br>├── RequiredTags<br>├── BlockedTags<br>├── Capacity<br>└── RuntimeState<br><!-- End of picture text -->

Esempio:

```
Cella (10, 5, Piano 0)
       |
       | Scala
       | Costo 2
       | Richiede Unit.CanClimb
       v
Cella (10, 5, Piano 1)
```

Un ascensore potrebbe avere costo basso ma richiedere energia. Un portale può collegare coordinate lontane. Un ponte distrutto disabilita l’arco. Un jump pad crea un arco temporaneo.

#### Formula del costo
Il pathfinder non deve conoscere direttamente lava, fumo o robot. Interroga provider di costo.

- `TraversalCost = EdgeBaseCost`

- `+ TerrainCost`

- `+ UnitProfileCost`

- `+ StatusCost`

- `+ DynamicHazardCost`

- `+ ReservationCost`

- `+ ScenarioCost`

Ogni provider può:

1. accettare l’attraversamento;

- rifiutarlo;

- aggiungere un costo intero;

- aggiungere una conseguenza prevista;

5. indicare che il risultato dipende dalla revisione corrente.

Esempio:

```
Unità umana
  Acqua     +2
  Fuoco     bloccato
  Scala     +1
Unità robotica
  Acqua     bloccato se elettrificata
  Fuoco     +0
  Scala     +2
Unità volante
  Acqua     +0
  Fuoco     +1
  Scala     ignorata
```

#### Strategia A*
L’algoritmo raccomandato è A* custom su grafo esplicito.

```
OpenSet
ClosedSet
CostSoFar
Parent
Heuristic
StableTieBreak
```

La funzione euristica deve restare ammissibile per il percorso di movimento. Una baseline è:

```
H =
  distanza Manhattan orizzontale
  + numero minimo stimato di cambi layer
```

Le regole tattiche come esposizione o qualità della copertura **non devono alterare il percorso mostrato automaticamente al giocatore** , altrimenti la UI può sembrare imprevedibile. Devono invece essere usate dall’IA o da un comando esplicito come “percorso più sicuro”.

|Ricerca|Ottimizza|
|---|---|
|Path preview giocatore|Legalità e costo movimento|
|Path sicuro opzionale|Movimento più rischio|
|IA offensiva|Posizione, tiro, obiettivo|
|IA difensiva|Copertura, distanza, fuga|
|Map validator|Connettività e requisiti|

#### Aggiornamenti dinamici
Ogni modifica topologica incrementa una `GraphRevision` .

##### `Ponte distrutto`

- `-> edge disabilitato`

- `-> GraphRevision + 1`

- `-> invalidazione path interessati`

```
Portale creato
```

- `-> nuovo edge -> GraphRevision + 1`

```
Cella incendiata
```

- `-> aggiornamento runtime state -> CostRevision + 1`

Una cache deve includere:

```
Start
Goal
TraversalProfileId
GraphRevision
CostRevision
Path
```

Non è necessario ricalcolare tutta la mappa quando cambia una sola cella. I primi tutorial possono invalidare l’intera cache; l’invalidazione selettiva arriverà soltanto dopo il profiling.

#### Linea di vista e tiro
La linea di vista è un servizio separato dal pathfinding:

```
Movement Graph
Line-of-Sight Service
Projectile Service
Cover Service
Visibility Service
```

Un’abilità interroga:

```
CanTarget(Source, Target)
GetCover(Source, Target)
TraceProjectile(Path)
GetVisibilityModifier(Source, Target)
```

Questa separazione consente a una cella di essere attraversabile ma opaca, oppure non attraversabile ma trasparente.

#### Risoluzione simultanea
<!-- Start of picture text -->
Planning<br>tutti pronti o timer<br>Locked<br>Preparation<br>Movement<br>match continua<br>Actions<br>Projectiles<br>Environment<br>Aftermath<br>vittoria<br><!-- End of picture text -->

Versione stampabile:

```
PLANNING
  Modifica piano, preview, undo, intenti alleati
        |
        v
LOCK
  Snapshot e validazione finale
        |
        v
PREPARATION
  Stance, scudi, reazioni preventive
        |
        v
MOVEMENT
  Microstep e conflitti
        |
        v
ACTIONS
  Abilità e interazioni
        |
        v
PROJECTILES
  Impatti, spostamenti, danni e status
        |
        v
ENVIRONMENT
  Fuoco, acqua, elettricità, trigger
        |
        v
AFTERMATH
  Obiettivi, KO, vittoria e nuovo round
```

Il resolver deve produrre una sequenza di eventi, non comandare direttamente animazioni.

```
FResolvedEvent
├── Phase
├── StableOrder
├── SourceEntity
├── TargetEntities
├── TargetCells
├── EffectId
├── NumericPayload
└── ResultingStateHash
```

L’animazione riproduce questi eventi. Il replay salva questi eventi. I test confrontano questi eventi.

#### Conflitti di movimento
Regole iniziali proposte:

|Situazione|Risultato|
|---|---|
|Una unità entra in una cella libera|Movimento riuscito|
|Due unità vogliono la stessa cella|Vince la priorità deterministica|
|Due unità alleate si scambiano|Permesso solo se la regola di swap è attiva|
|Due nemici si scambiano|Bloccati o gestiti da regola esplicita|
|Un bersaglio viene spinto|Risoluzione nella fase di impatto|
|La cella diventa invalida|L’unità resta nell’ultima posizione valida|
|Un arco viene distrutto durante il turno|Gli step successivi che lo usano falliscono|
|Una unità viene eliminata|Libera la cella nella fase stabilita|

La priorità deve usare valori deterministici:

```
PhasePriority
Initiative
ActionPriority
StableEntityId
```

Non usare l’ordine casuale di iterazione di un container.

#### Privacy della pianificazione
Flusso raccomandato:

```
Client locale
  crea preview
      |
      v
ServerSubmitPlan(Plan, Revision)
      |
      v
Server valida e conserva FPlannedTurn
      |
      +--> genera FTeamIntentView
      |       |
      |       +--> replica soltanto agli alleati
      |
      +--> conserva il piano completo soltanto sul server
```

La replica delle proprietà in Unreal è server-to-client; il server resta la fonte autorevole e i client non 6 devono modificare direttamente lo stato replicato.

Implementazione proposta:

1. `ARTPlayerController` invia `ServerSubmitPlan` .

2. `ARTTurnSubsystem` valida il piano.

3. Il piano completo rimane in memoria server.

- `ARTTeamPlanningChannel` , uno per squadra, contiene soltanto intenti sanificati.

5. `ARTTeamPlanningChannel::IsNetRelevantFor` autorizza i client del team.

6. A ogni riconnessione il server invia uno snapshot degli intenti correnti.

- Alla risoluzione il server elimina o archivia i piani privati.

8. Un test automatico controlla che il client avversario non possieda l’oggetto o il payload.

`COND_OwnerOnly` da solo non risolve il problema, perché un Actor ha normalmente un singolo owner mentre l’informazione deve raggiungere più compagni. Per un quattro contro quattro, un canale replicato per squadra o RPC mirate risultano più semplici da verificare.

#### UX della pianificazione
La UI deve distinguere tre livelli di informazione:

|Livello|Contenuto|
|---|---|
|Piano proprio|Dettaglio completo, costi, errori, timeline|
|Piano alleato|Path, destinazione, AoE, target, label e ready state|
|Nemico|Posizione e stato pubblicamente visibili, mai il piano|

Codifica visuale:

|Informazione|Forma|
|---|---|
|Movimento|Linea segmentata con frecce|
|Cella finale|Contorno pieno|
|Tiro|Linea sottile con punto target|
|Area d’effetto|Reticolo o pattern|
|Difesa|Arco o cupola|
|Interazione|Icona contestuale|
|Piano non confermato|Trasparenza e tratteggio|
|Piano confermato|Forma più stabile e lucchetto|
|Conflitto|Icona, testo e pattern di avviso|

Il colore non deve essere l’unico canale: ogni giocatore riceve anche un numero, una forma e un pattern. Common UI fornisce un’architettura per UI stratificate e input multipiattaforma; Enhanced Input consente Mapping Context dinamici, trigger e modificatori, utili per passare tra camera, selezione, targeting e UI. 7

#### IA tattica
Pipeline proposta:

```
Snapshot pubblico
    |
    v
Celle raggiungibili
    |
    v
Azioni candidate
    |
    v
Filtro legalità
    |
    v
Utility individuale
    |
    v
Coordinamento di squadra
    |
    v
Piano scelto
```

Utility:

- `Score = ObjectiveValue + ExpectedDamage + AllySynergy + PositionValue + CoverValue - Exposure`

- `HazardRisk`

- `ResourceCost`

- `CollisionRisk`

L’IA non deve leggere i piani privati dei giocatori umani. Il server deve fornire al planner uno snapshot che esclude tali informazioni, anche se tecnicamente le possiede.

StateTree è adatto agli stati macro, come avanzare, difendere, ritirarsi o perseguire un obiettivo. Behavior Tree e Blackboard sono utili per comportamenti gerarchici, mentre EQS offre un modello per raccogliere e pesare posizioni candidate. Il planner simultaneo specifico deve comunque restare nel core C++, perché deve produrre piani deterministici e coordinati. 8

### Stack tecnologico e configurazione del sistema
#### Requisiti consigliati
Epic indica per Unreal Engine una configurazione raccomandata con **32 GB di RAM** , GPU compatibile DirectX 11 o 12 e circa **8 GB di memoria grafica** . Per un progetto C++ multiplayer con editor, IDE e più client PIE, 32 GB rappresentano il minimo pratico consigliato; 64 GB migliorano la qualità di vita ma non sono un requisito formale. 9

|Componente|Raccomandazione|
|---|---|
|Sistema operativo|Windows 11 a 64 bit|
|CPU|Almeno otto core moderni|
|RAM|32 GB minimo raccomandato|
|GPU|DirectX 12, almeno 8 GB VRAM|
|Disco|SSD NVMe con almeno 200 GB liberi|
|Motore|Unreal Engine 5.8 stabile|
|IDE|Visual Studio compatibile con UE 5.8|
|Version control|Git più Git LFS|
|Account|Epic Games e GitHub collegati|
|Testing multiplayer|Due o più processi PIE o standalone|

Per UE 5.8, la tabella di compatibilità Epic indica Visual Studio 2022 dalla versione 17.14 e Visual Studio 2026 dalla versione 18.0; la stessa documentazione suggerisce Visual Studio 2026 per lo sviluppo generale, mantenendo Visual Studio 2022 in alcuni toolchain specifici. Verificare sempre la matrice relativa alla versione esatta del motore installata. 10

#### Installazione passo passo
##### Installare Unreal Engine
1. Scaricare e installare <u>Epic Games Launcher.</u>

2. Aprire **Unreal Engine → Library** .

3. Aggiungere **Unreal Engine 5.8** .

4. Aprire **Options** prima dell’installazione.

5. Selezionare:

6. Core Components;

7. Templates and Feature Packs;

8. Starter Content, facoltativo;

9. Engine Source, utile per consultazione;

10. Editor Symbols for Debugging, utile ma pesante;

11. Windows target platform.

12. Non usare una build Preview per il branch principale.

13. Avviare l’editor una volta per completare shader e configurazione.

Epic documenta l’installazione tramite Launcher, la selezione dei componenti e il fatto che le build Preview non siano destinate allo sviluppo di produzione. 11

##### Installare Visual Studio
Nel Visual Studio Installer selezionare:

1. **Game development with C++** ;

2. Windows SDK;

3. Visual Studio Tools for Unreal Engine;

4. Visual Studio debugger tools for Unreal Engine Blueprints;

5. Unreal Engine Test Adapter;

6. C++ profiling tools.

Gli strumenti Microsoft integrano visualizzazione delle macro Unreal, log, test e debugging Blueprint/C+ +. 12

##### Installare Git e Git LFS
1. Installare Git.

2. Installare Git LFS.

3. Aprire PowerShell.

4. Eseguire:

```
gitlfsinstall
```

1. Nel repository:

```
gitlfstrack"*.uasset"
gitlfstrack"*.umap"
gitlfstrack"*.fbx"
gitlfstrack"*.wav"
gitadd.gitattributes
```

Git LFS richiede l’installazione dell’estensione e il comando iniziale `git lfs install` ; i tipi di file vengono poi registrati nel repository. 13

`.gitignore` minimo:

```
Binaries/
DerivedDataCache/
Intermediate/
Saved/
.vs/
```

```
*.VC.db
*.sln
```

Il file `.uproject` , `Config` , `Content` , `Plugins` e `Source` devono essere versionati.

##### Collegare Epic e GitHub
L’accesso al repository sorgente Unreal su GitHub richiede il collegamento tra account Epic e GitHub e l’accettazione dell’invito all’organizzazione Epic. 14

Guida ufficiale:

##### <u>Unreal Engine on GitHub</u>

Non è necessario compilare subito il motore dal sorgente. La build del Launcher è più adatta alle prime fasi. La compilazione del motore ha senso quando occorre modificare il motore, diagnosticare problemi 15 profondi o mantenere patch specifiche. Epic fornisce una procedura ufficiale per la build sorgente.

#### Creazione del progetto
1. Aprire Unreal Engine.

2. Selezionare **Games** .

3. Scegliere **Top Down** .

4. Selezionare **C++** .

5. Target: **Desktop** .

6. Quality preset: **Maximum** inizialmente.

7. Ray tracing: disattivato.

8. Starter Content: facoltativo.

9. Nome: `RefactorTactics` .

10. Percorso senza spazi o caratteri speciali.

Struttura iniziale:

```
RefactorTactics
├── Config
├── Content
│   ├── Core
│   ├── Grid
```

- `│   ├── Characters`

- `│   ├── Abilities │   ├── UI │   ├── Maps │   └── Tests ├── Plugins ├── Source`

- `│   ├── RefactorTactics`

- `│   ├── RTCore`

- `│   ├── RTGrid`

```
│   └── RTTurn
└── RefactorTactics.uproject
```

##### Plugin da attivare progressivamente:

|Plugin|Quando|
|---|---|
|Enhanced Input|Subito|
|Gameplay Tags|Subito|
|Gameplay Abilities|Quando iniziano le abilità|
|Gameplay Tasks|Con Gameplay Ability System|
|Common UI|Durante la planning UI|
|Modular Gameplay|Prima delle modalità multiple|
|Game Features|Prima della vertical slice completa|
|Functional Testing|Dalla prima regola deterministica|
|Online Subsystem|Durante la multiplayer alpha|
|Model-View-ViewModel|Solo se scelto per la UI|
|Gameplay Message Router|Valutabile per eventi di presentazione|

#### Confronto tra C++, Blueprint e C
|Criterio|Unreal C++|Blueprint|C# tramite plugin|
|---|---|---|---|
|Supporto Epic|Completo|Completo|No|
|Networking nativo|Completo|Buono|Dipende dal plugin|
|Test unitari|Ottimo|Limitato|Dipende dal bridge|
|Performance|Alta|Adeguata per molte<br>funzioni|Variabile|
|Iterazione|Più lenta|Molto rapida|Rapida con hot<br>reload|
|Diff e merge|Testuale|Più difficile|Testuale|
|Accesso API nuove|Immediato|Esposte da Epic/C++|Dipende dai<br>binding|
|Piattaforme|Tutte quelle<br>supportate|Tutte|Dipende dal plugin|
|Rischio upgrade<br>motore|Gestibile|Gestibile|Maggiore|
|Uso consigliato|Core|Presentazione e<br>authoring|Tool o esperimenti|

**UnrealSharp** dichiara supporto per UE 5.6–5.8, .NET 10, hot reload e binding generati, ma resta un progetto open source esterno a Epic e il supporto piattaforme non coincide necessariamente con quello completo del motore. **UnrealCLR** integra .NET 6, mentre MonoUE richiede una versione modificata del motore ed espone limitazioni ancora dichiarate dal progetto. Queste soluzioni sono interessanti come esperimento o per tool interni, non come dipendenza centrale di RefactorTactics. 16

Decisione consigliata:

```
C++       Regole, dati runtime, rete, resolver, test
Blueprint UI, VFX, audio, level scripting, configurazione
C#        Eventuali tool isolati, mai requisito per avviare il gioco
```

#### Migrazione mentale da C
|C# o Unity|Unreal C++|
|---|---|
|`MonoBehaviour`|`AActor` ,<br>`UActorComponent` ,<br>`UObject` ,<br>`Subsystem`|
|`GameObject`|`AActor`|
|Component|Actor Component o Scene Component|
|Prefab|Blueprint Class|
|ScriptableObject|Data Asset|
|`List<T>`|`TArray<T>`|
|`Dictionary<K,V>`|`TMap<K,V>`|
|`HashSet<T>`|`TSet<T>`|
|Attribute|Macro di reflection|
|`Task`|Task UE, async operation o delegate|
|Event C#|Delegate Unreal|
|Garbage collector .NET|Garbage collector UObject|
|NuGet|Moduli, plugin e file<br>`.Build.cs`|
|Scene|Level o Map|
|Singleton|GameInstance Subsystem, World Subsystem o Engine Subsystem|

Epic mantiene una guida per sviluppatori provenienti da Unity che mette in relazione Scene e Map, GameObject e Actor, componenti e Blueprint Class. 17

##### Regole pratiche:

1. Non creare un UObject con `new` .

- Non distruggere un UObject con `delete` .

- Usare `UPROPERTY` o `TObjectPtr` per riferimenti UObject persistenti.

4. Preferire `TArray` , `TMap` , `TSet` , `FString` , `FName` e `FText` .

5. Separare `.h` e `.cpp` .

6. Non mettere ogni funzione in un Actor.

7. Usare classi e struct C++ pure per gli algoritmi.

8. Esporre a Blueprint soltanto API intenzionali.

9. Evitare Tick quando un evento può fare lo stesso lavoro.

- Compilare spesso, con modifiche piccole.

#### Primo esempio copia-incolla
Creare `Source/RefactorTactics/Public/Grid/RTGridTypes.h` :

```
#pragma once
#include"CoreMinimal.h"
#include"RTGridTypes.generated.h"
USTRUCT(BlueprintType)
structFRTGridCellId
{
GENERATED_BODY()
UPROPERTY(EditAnywhere,BlueprintReadWrite)
int32X=0;
UPROPERTY(EditAnywhere,BlueprintReadWrite)
int32Y=0;
UPROPERTY(EditAnywhere,BlueprintReadWrite)
int32Layer=0;
booloperator==(constFRTGridCellId&Other)const
{
returnX==Other.X
&&Y==Other.Y
&&Layer==Other.Layer;
}
};
FORCEINLINEuint32GetTypeHash(constFRTGridCellId&Cell)
{
returnHashCombine(
HashCombine(GetTypeHash(Cell.X),GetTypeHash(Cell.Y)),
GetTypeHash(Cell.Layer)
);
}
```

Cosa impari:

|Elemento|Significato|
|---|---|
|`USTRUCT(BlueprintType)`|Rende la struct visibile a Blueprint|
|`GENERATED_BODY()`|Aggiunge codice generato da Unreal Header Tool|
|`UPROPERTY`|Espone e traccia i campi|
|`FRT`|Prefisso del progetto|
|`GetTypeHash`|Permette l’uso come chiave in<br>`TMap` e<br>`TSet`|

#### Funzione C++ richiamabile da Blueprint
Creare una classe **Blueprint Function Library** dall’Editor:

```
#pragma once
#include"CoreMinimal.h"
#include"Kismet/BlueprintFunctionLibrary.h"
#include"Grid/RTGridTypes.h"
#include"RTGridBlueprintLibrary.generated.h"
UCLASS()
classREFACTORTACTICS_APIURTGridBlueprintLibrary
:publicUBlueprintFunctionLibrary
{
GENERATED_BODY()
public:
UFUNCTION(
BlueprintPure,
Category="RefactorTactics|Grid"
)
staticint32ManhattanDistance(
constFRTGridCellId&A,
constFRTGridCellId&B)
{
returnFMath::Abs(A.X-B.X)
+FMath::Abs(A.Y-B.Y)
+FMath::Abs(A.Layer-B.Layer);
}
};
```

Passi:

1. Chiudere l’Editor se la compilazione live crea problemi.

2. Compilare la soluzione.

3. Riaprire l’Editor.

4. Creare un Blueprint Actor.

5. Cercare il nodo `Manhattan Distance` .

6. Creare due `RTGridCellId` .

7. Stampare il risultato.

- Provare coordinate su layer differenti.

La guida ufficiale C++ Quick Start copre la creazione di classi, compilazione ed esposizione dei tipi Unreal. 18

#### Azione pianificata minima
```
#pragma once
#include"CoreMinimal.h"
#include"GameplayTagContainer.h"
#include"Grid/RTGridTypes.h"
#include"RTPlannedAction.generated.h"
UENUM(BlueprintType)
enumclassERTActionKind:uint8
{
Move,
Ability,
Interact,
Wait
};
USTRUCT(BlueprintType)
structFRTPlannedAction
{
GENERATED_BODY()
UPROPERTY(EditAnywhere,BlueprintReadWrite)
ERTActionKindKind=ERTActionKind::Wait;
UPROPERTY(EditAnywhere,BlueprintReadWrite)
FRTGridCellIdTargetCell;
UPROPERTY(EditAnywhere,BlueprintReadWrite)
FGameplayTagAbilityId;
};
```

Nel file `.Build.cs` aggiungere:

```
PublicDependencyModuleNames.AddRange(
newstring[]
{
"Core",
"CoreUObject",
"Engine",
"GameplayTags"
```

```
}
```

```
);
```

Il file `.Build.cs` usa sintassi C#, ma non contiene gameplay C#: descrive dipendenze e configurazione del modulo Unreal.

#### Sample ufficiali consigliati
|Risorsa|Utilità|
|---|---|
|Lyra Sample Game|Architettura modulare, multiplayer, GAS e Common UI|
|Action RPG Sample|Gameplay Ability System e interazione Blueprint/C++|
|Content Examples|Esempi isolati delle funzionalità engine|
|C++ Quick Start|Primo progetto C++|
|Blueprint Visual Scripting|Fondamenti Blueprint|
|Epic GitHub|Codice sorgente del motore|

Lyra è una risorsa particolarmente utile per modularità, multiplayer e Gameplay Ability System, ma non deve essere copiato indiscriminatamente. Epic segnala inoltre che la migrazione di un progetto Lyra fra major release può richiedere confronto manuale del codice e delle release note. 19

---

## PRD, roadmap e percorso didattico — architettura e scelte tecnologiche

### Architettura e scelte tecnologiche
Architettura logica

#### Architettura logica
<!-- Start of picture text -->
UMG e MVVM<br>Preview planning locale Lua Sandbox Asset Manager e pak<br>PlayerController e RPC Mod Registry<br>Server autoritativo e Turn Data Assets e Gameplay<br>Orchestrator Tags<br>Character System<br>Semantic Grid<br>A* e Cost Providers<br>LOS, Cover e Projectile<br>Queries<br><!-- End of picture text -->

#### Responsabilità dei moduli
|Modulo|Responsabilità|
|---|---|
|`RefactorTacticsCore`|Tipi di base, tag, ID, event log|
|`RefactorTacticsMap`|Tile graph, layer, archi e revisioni|

|Modulo|Responsabilità|
|---|---|
|`RefactorTacticsPath`|A*, provider, query di raggiungibilità|
|`RefactorTacticsCombat`|Abilità, danni, stati, LOS e cover|
|`RefactorTacticsTurn`|Planning, commit, resolve e aftermath|
|`RefactorTacticsNet`|RPC, validazione, privacy team e replay|
|`RefactorTacticsUI`|HUD, planning preview, notifiche e accessibilità|
|`RefactorTacticsContent`|Data Assets di personaggi, terreni e abilità|
|`RefactorTacticsMod`|Registry, manifest, JSON, sandbox e packaging|
|`RefactorTacticsEditor`|Tool per mappe, validazione e mod kit|

Unreal organizza il gameplay attorno a classi come `Actor` , `Pawn` , `Controller` , `GameMode` , `GameState` e `PlayerState` ; per chi proviene da Unity e C#, la documentazione ufficiale fornisce una corrispondenza tra GameObject/MonoBehaviour e Actor/Component, oltre al rapporto fra Blueprint e C++. 7

#### Blueprints, C++ e C
|Tecnologia|Uso raccomandato|Vantaggi|Limiti|
|---|---|---|---|
|Blueprint|UI, animazioni, effetti,<br>configurazione,<br>prototipi|Feedback rapido,<br>visuale, integrato con<br>l’Editor|Grafi grandi difficili da<br>mantenere e revisionare|
|C++ Unreal|Core tattico, rete,<br>pathfinding, test, editor<br>tool|Supporto nativo,<br>performance, reflection<br>e packaging|Sintassi e build più<br>complesse rispetto a C#|
|C# con<br>UnrealCLR|Spike o laboratorio<br>isolato|.NET, C# moderno,<br>debugger managed|Plugin di terze parti;<br>compatibilità con UE 5.8<br>da verificare|
|Lua con<br>UnLua|Mod scripting e<br>iterazione controllata|Dinamico, accesso<br>tramite reflection|Plugin community;<br>necessaria sandbox e<br>versione bloccata|
|AngelScript<br>Hazelight|Possibile gameplay<br>scripting avanzato|Esperienza d’uso in<br>produzioni Hazelight|Richiede modifiche<br>dirette al source<br>dell’engine|
|MonoUE|Studio storico|Approccio C#/F#|Progetto e workflow non<br>adatti come base<br>moderna|
|VaRest|Non raccomandato|Accesso REST semplice<br>da Blueprint|Repository archiviato e<br>baseline engine datata|

UnrealCLR dichiara integrazione .NET 6, C# 10, caricamento dinamico degli assembly e supporto desktop, ma la documentazione del repository usa prerequisiti generici e non garantisce

automaticamente la compatibilità con la versione 5.8. La scelta corretta è una spike time-boxed e non l’adozione immediata nel core. 8

MonoUE è rimasto a lungo un progetto sperimentale con workflow e limitazioni non adatti a una nuova produzione. 9

UnLua dichiara supporto a diverse versioni UE 4 e 5 e a più piattaforme, ma il suo issue tracker mostra 10 che la compatibilità con nuove release deve essere verificata e fissata per ogni progetto.

L’integrazione AngelScript di Hazelight è attivamente documentata ed è stata usata nei giochi dello studio, ma richiede modifiche dirette al source di Unreal Engine; comporta quindi il mantenimento di un engine fork. 11

VaRest è stato archiviato nel novembre 2024 e non dovrebbe essere scelto come nuova dipendenza. Per eventuali API web sono preferibili i moduli HTTP e JSON nativi di Unreal o una libreria mantenuta. 12

#### Decisione raccomandata
La baseline è:

```
Gameplay core       → C++
Contenuti            → Blueprint + Primary Data Assets
Classificazione      → Gameplay Tags
UI                    → UMG, eventualmente MVVM
Networking           → Replicazione Unreal e RPC
Test                  → Automation Framework + Gauntlet
Mod dati              → JSON + registry
Mod contenuti         → Pak/chunk
Mod scripting         → Lua sandboxata
C#                    → Spike opzionale, non dipendenza core
```

#### Struttura del progetto
```
RefactorTactics/
├── Config/
├── Content/
```

- `│   ├── Characters/`

- `│   ├── Abilities/`

- `│   ├── Maps/`

- `│   ├── Tiles/`

- `│   ├── UI/`

- `│   └── Developer/ ├── Plugins/`

- `│   ├── RefactorTacticsCore/`

- `│   ├── RefactorTacticsMap/`

- `│   ├── RefactorTacticsCombat/`

- `│   ├── RefactorTacticsMod/`

- `│   └── ThirdParty/`

```
├── Source/
```

- `│   ├── RefactorTactics/`

- `│   ├── RefactorTacticsEditor/`

- `│   └── RefactorTacticsServer/ ├── Mods/ ├── Scripts/ └── RefactorTactics.uproject`

Per le prime lezioni può essere utilizzato un solo modulo. La separazione in plugin va introdotta soltanto quando i confini diventano stabili. I plugin Unreal possono contenere codice, contenuti ed estensioni dell’Editor. 13

#### Modello dati principale
<!-- Start of picture text -->
UNIT<br>string unitId<br>int teamId<br>string currentTile<br>istanza_di<br>MOD_MANIFEST<br>CHARACTER_DEFINITION<br>string modId<br>string characterId<br>string version<br>string specialization<br>string apiVersion<br>possiede registra include<br>TILE_NODE<br>int x ABILITY_DEFINITION<br>int y string abilityId<br>CONTENT_DEFINITION<br>int layer int actionPhase<br>int baseMoveCost int cooldown<br>int graphRevision<br>possiede dichiara usa<br>classificato_da<br>TILE_EDGE<br>string destination<br>GAMEPLAY_TAG<br>int baseCost<br>string traversalType<br><!-- End of picture text -->
