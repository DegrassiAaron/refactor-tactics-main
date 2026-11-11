# L'audit per area della v0.1 giocabile — 2026-11-04

> `SNAPSHOT` · fotografia del 2026-11-04: vale finché è l'ultima misura del suo oggetto.
>
> - **Statuto**: referto di un audit **in sola lettura** della ridefinizione in [`v01-giocabile-brainstorm-2026-11-04.md`](v01-giocabile-brainstorm-2026-11-04.md), eseguito il 2026-11-04 da agenti per area e per gruppo di backlog, ciascuno con un tetto di chiamate e l'obbligo di dichiarare ciò che non ha verificato. Le affermazioni portano il comando o il `file:riga` che le prova. Le proposte (lacune, tagli, opzioni) sono di chi scrive e **non decidono nulla**.
> - **Misurato su** `origin/main` = `361e2225f`.
> - **Decisioni registrate**: nessuna da questo documento. [`D-576`](../../decisions/RT_PDR_00_Decision_Log.md) (PR #3987) registra la ridefinizione.
> - **Tracking**: [#3985](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3985).
>
> ⛔ Nessuna riga di `Source/` e di `Content/` cambia con questo referto. Nessuna issue è stata creata, spostata o chiusa.
>
> Convenzione: niente totali che cambiano da soli. Dove un numero serve, sta accanto al comando che lo produce.
>
> **Allegati**: [lavori delle ondate 1, 2a, 2b e 3](v01-giocabile-audit-lavori-ondate-1-3-2026-11-04.md) · [lavori delle ondate 4a, 4b e 5](v01-giocabile-audit-lavori-ondate-4-5-2026-11-04.md).

---

## 1. Come leggerlo

- **Ondate**: 1 Ambiente · 2a Personaggi e animazioni · 2b VFX, suoni e audio · 3 Scelte pre-partita · 4a Bot (E13 ed E26) · 4b Roster a 8 (E35) · 5 HUD, schermate e giro finale. La spaccatura di 2 in 2a e 2b e di 4 in 4a e 4b è di chi scrive.
- **Esistente**: `riusa` si usa com'è; `estende` va ampliato; `parziale` copre una parte; `obsoleto` la sua premessa è caduta.
- **Lavoro nuovo**: nei due allegati, con un DoD binario proposto. La taglia (`S` `M` `L` `XL`) è **relativa**: non è una stima in giorni, e il progetto non ne ha una.
- **Contraddizioni**: `STALE ROADMAP`, `IMPLEMENTATION DRIFT`, `CONTRACT CONFLICT`. Non sono riconciliate: ciascuna dice quale fonte governa.
- **Domande aperte**: sono dell'autore; stanno negli allegati, ondata per ondata, con le loro opzioni.

## 2. Che cosa l'audit e la revisione di D-576 correggono in ciò che è già scritto

Ognuna di queste voci contraddice il brief, `D-576`, o il testo di un'opzione o del rapporto che l'autore ha visto. Le correzioni sono già nel brief (PR #3986) e nella voce (PR #3987); qui è il perché, con la prova. **Ciò che l'autore ha visto non si riscrive**: si cita com'era e si aggiunge la correzione.

1. **La corrispondenza fra le schede e i nomi RT è già decisa.** [`docs/characters/paragon.md`](../../characters/paragon.md): Steel = Ward, Murdock = Vigil (Sentinel Directorate); Aurora = Rime, Kwang = Tethra (Resonance), «Chiuso il 2026-09-04» da [D-322](../../decisions/RT_PDR_00_Decision_Log.md), con i nomi della v0.1 (Aevik, Muiren, Branth, Ivrin) ridecisi da [D-334](../../decisions/RT_PDR_00_Decision_Log.md). Il brief (§6, §7) e `D-576` (7)(g) la davano per aperta. Le schede portano ancora `RT Character ID: TBD`: quel campo è stantio.
2. **La morte visiva non è differita.** `PIE-AS4b` è ✅ dal 2026-09-28 (seduta `U8`, verdetto d'autore sui quattro eroi): la clip `Death` parte alla caduta, e «differita» in E21.2 vale per la **sparizione** dell'unità. L'opzione che l'autore ha visto diceva: *«Paragon con i ruoli di presentazione Idle/Move/Cast/Attack/Hit/Death; la morte visiva è differita (E21.2).»* Resta un limite vero, la finestra zero per chi cade nell'ultima fase (§4.2).
3. **I pack non portano effetti sonori.** In ciascuno dei quattro pack gli unici asset che citano `SoundCue` o `SoundWave` stanno nelle cartelle di voce (`Sounds` per tre, `Audio` per Wraith), con famiglie di nomi vocali (`Effort`, `Lore`, `Emote`, `Kill`, `Death`…): `grep -rl -a -E 'SoundCue|SoundWave|SoundClass' <pack> --include='*.uasset'`, con controllo positivo sui suoni noti di Gadget. L'audit diceva che Wraith «non ha suoni»: ha la cartella `Audio`, ed è la stessa famiglia vocale. Per «Misto» (§3.3 del brief) significa che **gli effetti sonori non hanno una base nel progetto**.
4. **`#2291` non è stata differita da D-484.** [D-484](../../decisions/RT_PDR_00_Decision_Log.md) (2) differisce la sola `#2297`; `#2291` (CP 35.5) è già un checkpoint di E35 in milestone v0.2. Il rapporto che l'autore ha confermato le diceva insieme («`#2291` e `#2297`, che D-484 aveva differito»): la seconda metà era imprecisa. Inoltre il rename secco degli ID del roster v0.1 (fette B·C·D di `#2297`) è già in `main` con `#2491` (chiusa): `gh issue view 2491 --json state`.
5. **CP 46.4 non è in D-144.** La frase *«Match Setup v0.1 è **preconfigurato** — …»* sta nella tabella di E46 in [`roadmap-v0.1.md`](../roadmap-v0.1.md) §5; [D-144](../../decisions/RT_PDR_00_Decision_Log.md) ha aperto l'epic ma non contiene quella frase (`git grep -n preconfigurat -- docs/roadmap/roadmap-v0.1.md`). Vale anche per CP 46.3.
6. **Due decisioni accettate dicono il contrario di «anticipare E26», e il brief e `D-576` non le nominavano.**
   - [D-441](../../decisions/RT_PDR_00_Decision_Log.md) porta il **comportamento** dei bot fuori dalla v0.1 su una frase dell'autore del 2026-09-27: *«il comportamento dei bot al momento non mi interessa, mi servono solo per veder funzionare il gioco»*, e sposta `U5` e `#149` su «E26 · v0.2».
   - [D-338](../../decisions/RT_PDR_00_Decision_Log.md) (2) rifiuta di esercitare la via dell'obiettivo con *«oppure anticipare `E26`»*: il bot non la cerca, e il bot che la cercherebbe è E26 in v0.2. Il Match Setup scelto dall'autore comprende «obiettivo e durata».
   - La scelta di oggi, «Anticipare E26» e «Partite più vere», è più recente e specifica. Ma **la frase del 2026-09-27 non gli è stata riproposta quando ha scelto**, e le due decisioni restavano vive senza rimando. La voce le annota come superate in quella parte (lettura dell'assistente, ribaltabile).
7. **L'ordine delle ondate 2–5 è una bozza dell'assistente.** L'autore ha scelto solo il primo posto («Ambiente della mappa»); l'anteprima che ha visto era una *«bozza»* e il rapporto che ha confermato diceva *«Ondate e tetti sono bozze mie»*. «E26 ed E35 vanno per ultime» vale fra i contenuti, prima dell'ondata 5.

## 3. Le decisioni che toccano all'autore

Le domande aperte di ogni area sono negli allegati, con le loro opzioni. Qui sono raggruppate in quelle che **non si possono rimandare** a un'ondata e in quelle che si decidono con l'ondata.

**Prima di aprire le ondate**
- **Il tetto di E26 nella v0.1**: le opzioni A–D hanno un DoD binario (allegato 4–5, ondata 4a): A minimo, senza Team Planner · B coordinazione minima (CP 26.1 e 26.2) · C coordinazione di `#326` completa · D = C più sinergie ambientali e reaction policy. Fuori da tutte: belief, threat map, information value, predictive scoring e stress 4v4 (che la riga «Fuori scope v0.1» di `roadmap-v0.1.md` §1 dichiara fuori: finché non è deciso, vale la riga).
- **La forma di E35**: Aurora e Kwang entrano con le primitive grandi (oggetto persistente piazzato, ghiaccio prodotto da un'azione, risorse di kit) o con kit ridotti che usano solo le meccaniche di oggi, con il resto in v0.2 (ondata 4b). Dipende da dove viene la loro base visiva, perché i loro pack non sono nel progetto.
- **La sorte di D-441 e D-338** (punto 6 di §2): tornano `U5`, `#149`, `U19` e la via dell'obiettivo? Con E26 dentro, una parte del loro motivo è caduta; il resto è da decidere.
- **La forma di `G17`** e la sorte di `G13`, di PIA e di `G16` (allegato 4–5, ondata 5).
- **Il primary owner dell'arte d'ambiente**: E21 (`#286`), E49 (`#1769`, post-v0.1) o un'epic nuova (allegato 1–3, ondata 1).
- **Che cosa esce dalla v0.1**: le proposte di differimento del triage (§5.10), nessuna decisa.

**Con l'ondata a cui appartengono**
- Ondata 1: stile dell'ambiente, tabella dati superficie → look, luci autorate, seconda mappa.
- Ondata 2: sistema FX (Cascade dai pack, conversione a Niagara o asset nuovi; se Cascade è ammesso dal nuovo tetto), catalogo delle cue come dato o come tabella, fonte degli effetti sonori e della musica, pilota (quale eroe).
- Ondata 3: precedenza della sorgente-menu, doppioni, squadra avversaria, preset di obiettivo e durata, significato di «livello», PLAY AGAIN.
- Ondata 5: linguaggio visivo (fonte dei token, lingua, kit UI), risoluzioni di riferimento; che cosa intendeva l'autore con «menu».

## 4. Sintesi e contraddizioni per ondata

Il lavoro nuovo, l'esistente da riusare e le domande di ciascuna ondata sono nei due documenti allegati: [ondate 1–3](v01-giocabile-audit-lavori-ondate-1-3-2026-11-04.md) e [ondate 4–5](v01-giocabile-audit-lavori-ondate-4-5-2026-11-04.md).

### 4.1 · Ondata 1 — Ambiente

La board e resa interamente da C++: ARTHexMapActor monta istanze ISM (celle, rilievo, blocchi, bordi, glifi, griglia, volumi) con due soli materiali dati-driven (CellMaterial=M_HexCell, VolumeMaterial) piu CellMesh opzionale; i colori di superficie vengono da URTHexLibrary::SurfaceColor, non da dati. Lo scaffale per un look da materiali esiste (slot materiale, CellMesh, custom data per istanza, commandlet a ricetta), ma mancano una tabella dati superficie-materiale, un materiale lit, luci autorate su L_HexArena (la luce e uno spawn di fallback in RTGameMode) e uno stile definito. La leggibilita e protetta da molti test (velo, piani, colore/luminanza) ma da poche voci PIE eseguite: le sedute U25/U35 e PIE-V01-ARENADOOR sono ancora da eseguire, quindi la baseline graybox non e firmata. Due issue dell'elenco hanno il codice gia in main (#1758, #2330) e restano aperte solo per giudizi a schermo; #2492 non e di ambiente.

**Contraddizioni fra le fonti** *(nessuna è riconciliata in silenzio: ciascuna dice chi governa)*

- `STALE ROADMAP` — Il corpo di #1758 (e la formulazione di E21.3) dicono che in partita non esiste alcun confine fra celle; il codice ha CellBorders attivo di default dal commit 85b9935f0 e l'ultimo commento di stato della stessa issue dice che il residuo e solo PIE-GRID-CONFINE. Governa il codice in main piu il commento di stato. *(gh issue view 1758 (corpo e commento 2026-08-30); Source/RefactorTactics/Map/Actor/RTHexMapActor.h:1252-1269 e :1596; git log -S CellBorders -> 85b9935f0)*. **Governa**: codice in main e ultimo commento di stato di #1758
- `STALE ROADMAP` — Lo snapshot di E23 in roadmap-v0.1.md (2026-09-05) elenca #712 fra le issue aperte; GitHub la dichiara CLOSED. Lo stesso snapshot dichiara che prevale sulle formulazioni precedenti, ma e stato superato dallo stato live. *(docs/roadmap/roadmap-v0.1.md riga 1955+ (blocco 'Stato GitHub live'); gh issue view 712 -> CLOSED)*. **Governa**: GitHub live
- `CONTRACT CONFLICT` — D-124 limita E21 a leggibilita tattica, esclude VFX completi e dichiara che un confine 'leggibile' impedisce lo scivolamento in 'bello'; il brief del 2026-11-04 chiede arte d'ambiente 'da materiali'. D-228, richiamata dalla spec graybox, dichiara che la resa di sviluppo della board cadra con l'art pass, ma nessun owner descrive quell'art pass. Finche il D-nnn di brief sez. 8 non e in main, governa D-124. *(Decision Log riga 178 (D-124); docs/technical/systems/spec-graybox-placement-contract.md sez. 8.1; docs/roadmap/plans/v01-giocabile-brainstorm-2026-11-04.md sez. 8)*. **Governa**: D-124 fino al merge del D-nnn di scope (brief sez. 8 punti 2 e 6)
- `CONTRACT CONFLICT` — Il brief dice che la rimisura di G1-G14/G16 si fa una volta sola, alla fine (sez. 3.4), e nella tabella delle ondate fa uscire l'ondata 1 con 'Client FPS da rimisurare' (sez. 5). D-124 vincola l'FPS a dopo le mesh definitive. *(docs/roadmap/plans/v01-giocabile-brainstorm-2026-11-04.md sez. 3.4 e sez. 5; Decision Log riga 178 (D-124))*. **Governa**: D-124 per l'ordine; la distinzione fra misura preliminare e sign-off e una proposta da confermare
- `IMPLEMENTATION DRIFT` — #2330 e #1758 sono aperte nel tracker ma l'implementazione e in main (RTSetCellDoorCommandlet con porta posata nell'arena, CellBorders attivo). Il residuo e solo di giudizio a schermo (PIE-V01-ARENADOOR, PIE-GRID-CONFINE). *(git log RTSetCellDoorCommandlet.cpp (62bcebbbb); gh issue view 2330 (commento 2026-09-06); registro PIE: PIE-GRID-CONFINE stato eseguibile)*. **Governa**: codice in main
- `IMPLEMENTATION DRIFT` — L'illuminazione di partita non e un asset d'ambiente ma uno spawn di fallback in RTGameMode; il match packaged parte da una mappa sotto Maps/Dev (MatchLevel in DefaultGame.ini) senza luci autorate. Nessun owner documentale presentato l'ha scritto come scelta. *(Source/RefactorTactics/RTGameMode.cpp:646-658; Config/DefaultGame.ini (MatchLevel); grep -a su L_HexArena.umap)*. **Governa**: codice in main (da confermare con un owner)
- `STALE ROADMAP` — Il registro PIE per PIE-GBX-* dichiara l'asset presente con un conteggio storico del 2026-08-28 (git ls-files della cartella Graybox); la cartella contiene ora anche Materials, Fixtures e Volumes. Il numero in prosa del registro non e piu una misura corrente. *(docs/technical/test-manuali-pie.md righe 1616-1620; find Content/RT/World/Graybox -type f)*. **Governa**: filesystem corrente

### 4.2 · Ondata 2a — Personaggi e animazioni

Il meccanismo c'è ed è tutto C++ e dati, senza AnimBlueprint dei pack (D-248). ERTPresentationRole nomina 9 ruoli; il grafo URTUnitAnimInstance suona solo Idle e Move (2 sequence player, blend a due vie con alpha booleano bIsMovingVisually, slot DefaultSlot); Cast, Attack, Hit e Death passano da ARTUnit::PlayPresentationRole, che risolve la clip dal CDO e la suona con PlaySlotAnimationAsDynamicMontage. La risoluzione è (ActionId, ruolo) → (BaseActionId, ruolo) → ruolo, e solo Cast e Attack conoscono l'azione (spec D3). Le clip si assegnano nel costruttore di URTUnitAnimInstance (MakeClips per il ruolo, MakeActionClips per l'azione), non in Data/Anim: AnimCatalog.json ha cinque voci Unreviewed di Gadget e nessun binding, e _redirects.json non esiste. Il catalogo è l'override: AnimCatalog.json → -run=RTBuildAnimBindings → ABP_RTUnitAuthored → -run=RTSyncUnitAnimRefs scrive UnitAnimClass e i riferimenti duri in CookDeps_Anim dei BP_Unit_*. Copertura azioni: 17 chiavi PerAction su 23 abilità di kit; senza voce ConductiveNode, ReactiveCapacitor, Interposition, Deflection, FlowReaction, MistVeil (tutte reazioni o passive). La morte visiva NON è più differita: Death parte alla fase di caduta, HideForDefeat solo in FinishPlayback, e PIE-AS4b è ✅ dal 2026-09-28; il limite residuo è la finestra zero per chi cade nell'ultima fase. Posa e facing: #2167 è sostanzialmente chiusa a schermo (PR #3505), #1793 (offset di Phase/Muiren) è ancora aperta e richiede calibrazione visiva in Editor. I pack coprono tutte le categorie (Hit a 4 direzioni, Death 1-3 clip, decine di cast), ma Hit usa una sola direzione, nessuna clip Stun è collegata, Dash, Defend e Fall non hanno consumatore e Cascade resta non misurato in 5.8.1. Il lavoro vero dell'ondata è il giudizio dell'autore su un pilota di un eroe, non nuova infrastruttura.

**Contraddizioni fra le fonti** *(nessuna è riconciliata in silenzio: ciascuna dice chi governa)*

- `STALE ROADMAP` — La roadmap E21.2 e il brief (§2) dicono che la morte visiva resta differita. In realtà il montaggio Death parte alla fase di caduta, l'hide avviene solo in FinishPlayback, e PIE-AS4b è stata eseguita con esito positivo il 2026-09-28 sui 4 eroi. «Differita» vale per la sparizione, non per l'animazione. Resta il limite della finestra zero nell'ultima fase. *(docs/roadmap/roadmap-v0.1.md:1626; docs/roadmap/plans/v01-giocabile-brainstorm-2026-11-04.md:49; docs/technical/test-manuali-pie.md:1167-1168; Source/RefactorTactics/Turn/RTTurnManager.cpp:9960-9995)*. **Governa**: test-manuali-pie.md PIE-AS4b e il codice di RTTurnManager.cpp; D-124 esclude solo la «cinematic death».
- `CONTRACT CONFLICT` — D-124 mette fuori v0.1 locomotion set bespoke, Niagara per abilità e cinematic death; la ridefinizione del 2026-11-04 chiede clip per azione, FX e morte visiva dalle ondate 2a e 2b. Finché il nuovo D-nnn non è in main, #3985 dichiara che lo scope resta quello dell'owner. *(docs/decisions/RT_PDR_00_Decision_Log.md:178 (D-124); gh issue view 3985; brief §3)*. **Governa**: D-124 fino al merge del D-nnn che ridefinisce la v0.1.
- `IMPLEMENTATION DRIFT` — Il pointer del compito indicava Data/Anim/_redirects.json, che non esiste: Data/Anim contiene solo AnimCatalog.json. Inoltre le clip effettive non vivono nei dati ma nel costruttore C++ di URTUnitAnimInstance, mentre il catalogo ha 0 binding. Il contratto (spec clip-per-abilità D4) prevede esattamente questo: default C++ e catalogo come override. *(ls Data/Anim; Data/Anim/AnimCatalog.json; docs/superpowers/specs/2026-10-07-clip-per-abilita-design.md:77; Source/RefactorTactics/Unit/RTUnitAnimInstance.cpp:195-235)*. **Governa**: La spec clip-per-abilità (D4): il codice è conforme, il pointer era sbagliato.

### 4.3 · Ondata 2b — VFX, suoni e audio

Oggi la dorsale evento -> presentazione esiste, ma porta solo animazione, numeri di danno e disegni a line batcher: nessun VFX da asset e nessun suono (0 file di produzione con i simboli VFX o audio, controllo positivo con UAnimInstance: 3 file). La mappatura e' una tabella C++ senza slot asset. Cascade (UParticleSystem) e' nel modulo Engine runtime dell'Engine installato e il plugin Cascade e' di soli Editor, con default abilitato, ma il suo rendering in 5.8.1 non e' stato misurato qui. I quattro pack portano Cascade (P_*/PS_*, nessun NS_*) e soli suoni di VOCE; Wraith non ha nemmeno quelli. L'audio non ha owner ne' codice.

**Contraddizioni fra le fonti** *(nessuna è riconciliata in silenzio: ciascuna dice chi governa)*

- `CONTRACT CONFLICT` — D-124 lascia fuori dalla v0.1 'Niagara per ogni abilita'' e il VFX completo (e #2454, #3578 lo ripetono come fuori scope), mentre il brief del 2026-11-04 mette VFX, impatti e audio dentro la v0.1. Il brief dichiara che nessuna decisione e' ancora nel Decision Log. *(docs/decisions/RT_PDR_00_Decision_Log.md:178 (D-124); docs/roadmap/plans/v01-giocabile-brainstorm-2026-11-04.md §2, §8; gh issue view 2454 (Out of scope: Niagara); docs/superpowers/specs/2026-10-08-profilo-fx-…)*. **Governa**: D-124 resta il contratto accettato finche' non e' registrato un D-nnn che lo emenda; il brief e' il mandato d'autore ma non e' contratto.
- `IMPLEMENTATION DRIFT` — Il brief (§2) dice che 'animazioni, suoni e FX' stanno in ciascuno dei quattro pack. Su disco ParagonWraith (eroe Ivrin) non ha nessun file sotto /Sounds/ (0, con controllo positivo Phase: 564 file). Inoltre i suoni degli altri tre sono, dai nomi, voce e non effetti di combattimento. *(docs/roadmap/plans/v01-giocabile-brainstorm-2026-11-04.md §2 'Pack Paragon degli eroi'; scratchpad list_Wraith.txt (grep -c '/Sounds/' -> 0); scratchpad list_Phase.txt (grep -c '/Sounds/' -> 564))*. **Governa**: Il disco (stato live); il brief va corretto.
- `STALE ROADMAP` — Il corpo di #2454 dice che AttackFootprint, ReactionResolved e StatusChanged sono NoPresentation con la stessa clausola e che il gate resta verde se non si fa nulla. Nel codice: AttackFootprint e' Cues (AddPlaybackFootprint), AbilityActivated e Healed sono Cues, ReactionResolved e StatusChanged sono PendingPresentation (tipo introdotto da #2483) con owner nominato, e ArcHit/HazardDamage sono Pending. Il gate ora distingue Pen… *(gh issue view 2454; Source/RefactorTactics/Turn/Playback/RTPresentationBinding.cpp:99-283)*. **Governa**: Il codice (IMPLEMENTATION fa fede per lo stato); l'owner di ReactionResolved resta #2454.
- `STALE ROADMAP` — #2617 cita 'RTPresentationBinding.cpp:89' per HazardDamage Pending e la tabella di #2453 dichiara HazardDamage senza produttore: il produttore e' arrivato con #2460 e la voce e' alla riga 99. *(gh issue view 2617; gh issue view 2453; Source/RefactorTactics/Turn/Playback/RTPresentationBinding.cpp:99)*. **Governa**: Il codice; riferimenti per riga da sostituire con il nome della funzione.

### 4.4 · Ondata 3 — Scelte pre-partita

Quasi tutte le leve di allestimento esistono già come dato con precedenza proprietà < riga di comando < console, e il bootstrapper valida roster, formato e mappa. Mancano: la schermata, il trasporto della richiesta oltre OpenLevel, un ingresso per il formato, un registro di mappe selezionabili, la seconda mappa e i campi nel manifest di replay. Il punto di ingresso è un solo canale: URTFrontendNavigator porta già PendingMatchLevel oltre l'OpenLevel e ARTGameMode::SetupHexMatch compone FRTMatchBootstrapConfig. Un Match Setup si innesta lì, come quarta sorgente da dichiarare in ordine di precedenza. Il nodo duro è la seconda mappa: authoring binario nel clone principale, calibrazione di bot e soglia obiettivo sulla sua geometria.

**Contraddizioni fra le fonti** *(nessuna è riconciliata in silenzio: ciascuna dice chi governa)*

- `STALE ROADMAP` — CP 46.4 dice «Match Setup v0.1 è preconfigurato — non c'è nulla da scegliere», e roadmap-post-v0.1.md colloca scelte e Briefing in v0.2. L'autore ha confermato «Con scelte» in v0.1. *(docs/roadmap/roadmap-v0.1.md (E46, CP 46.4); docs/roadmap/roadmap-post-v0.1.md:1411; docs/roadmap/plans/v01-giocabile-brainstorm-2026-11-04.md §3.1 e §8)*. **Governa**: Fino al merge del D-nnn della ridefinizione governa roadmap-v0.1.md, come dice #3985. La scelta dell'autore è registrata solo nel brief.
- `CONTRACT CONFLICT` — «Manifest» indica due cose. Il brief vuole che le scelte «entrino nel manifest e nel replay». spec-manifesto-showcase.md non propone nessuna struct e scarta un asset-manifesto. L'artefatto vero è FRTReplayManifest (D-077), che oggi non porta nessuna scelta. *(docs/technical/tooling/spec-manifesto-showcase.md §4; Source/RefactorTactics/Replay/RTReplayManifest.h; docs/roadmap/plans/v01-giocabile-brainstorm-2026-11-04.md §5)*. **Governa**: Per la persistenza nel replay governa FRTReplayManifest e il suo schema additivo. Per la composizione delle scelte governa spec-manifesto-showcase.md (nessun nuovo contenitore, quarta sorge…
- `IMPLEMENTATION DRIFT` — La banda di spec-durata per il 2v2 è RoundLimit 14-16 con round attesi 10-14. Il codice spedisce RoundLimit 12 ed ExpectedRounds 10, e il commento riporta una partita reale decisa al round 21: la via normale di chiusura è il pareggio allo scadere, accettato da D-184. I preset di durata partono da qui. *(docs/gameplay/spec-durata-partita-e-scala-mappe.md §6; Source/RefactorTactics/Turn/Format/RTMatchFormatLibrary.cpp:183-198)*. **Governa**: D-184 e il valore spedito governano la v0.1; la spec dichiara i numeri da fissare con telemetria. Nessun preset si deriva da bot-contro-bot (D-102).
- `STALE ROADMAP` — La riga CP 46.5 dice che manca WBP_RT_ResultScreen (seduta U29), mentre la riga di stato di E46 dice che U24 ha consegnato tutti e cinque i WBP. Il file esiste e ha il binding in DefaultGame.ini. #940 è ancora OPEN. *(docs/roadmap/roadmap-v0.1.md (E46, CP 46.5 e riga di stato); Content/RT/UI/Framework/WBP_RT_ResultScreen.uasset; Config/DefaultGame.ini (ScreenId="Result"); gh issue view 940)*. **Governa**: Il repository (file e binding presenti) governa lo stato; la voce PIE-V01-FRONTEND-RESULT resta NOT RUN finché non eseguita.
- `IMPLEMENTATION DRIFT` — Il commento di Team0Heroes dice «Aevik + Phase contro Branth + Ivrin», mentre il valore è { Hero.Aevik, Hero.Muiren }. Phase è il pack di Muiren e a schermo il nome è D-120: lo stesso eroe ha tre nomi nel codice, nel commento e in UI. *(Source/RefactorTactics/RTGameMode.h:94-97 e il commento sopra; docs/decisions/RT_PDR_00_Decision_Log.md D-120; docs/roadmap/plans/v01-giocabile-brainstorm-2026-11-04.md §2 (riga Roster))*. **Governa**: Il valore nel codice (HeroId) è l'identità; D-120 governa il nome a schermo; il commento va corretto quando si tocca il file.

### 4.5 · Ondata 4a — Bot: E13 ed E26

Il bot di oggi è un utility planner per unità (URTBotPlanningLibrary::PlanTurn + URTHexBotLibrary) che pianifica già sulla conoscenza della propria squadra (ClassifyTarget, LastKnownCell) e porta l'obiettivo nel punteggio (CP 26.5 #2269, chiusa). Di E26 non c'è altro: TeamPlanner, TopK, BeliefMap e ThreatMap non hanno riscontri in Source (#531-#534 aperte, ancora in v0.2). E13 è chiusa sulla vista e sul velo (CP 13.1-13.3, 13.7, F1a-c) ma NON sul rumore: nessun sistema di gioco chiama URTAcousticPropagationLibrary, FRTAcousticContact non esiste, Status.Reveal è solo un tag, il ricordo del nemico scade ancora dopo 1 turno e la memoria del terreno (F2a) non esiste. Per F2, F3 e F4 del brief del 2026-10-14 non ho trovato issue proprie. La certificazione (bot-competence.yaml) è una fotografia del 2026-09-20 su 4 eroi, con FAIL per Environment e AdvancedCounter; con un roster a 8 la griglia deve essere piena, quindi il controllo check.py diventa rosso finché non si aggiungono le righe. Il tetto di E26 è una decisione d'autore: sotto sono elencate quattro opzioni con DoD binario.

**Contraddizioni fra le fonti** *(nessuna è riconciliata in silenzio: ciascuna dice chi governa)*

- `STALE ROADMAP` — L'elenco di E26 in roadmap-post-v0.1.md nomina ancora belief weights, predictive action scoring, threat/opportunity map, information value e stress 4v4, ma lo stesso testo dice che belief e predictive sono usciti in feature separate, e il corpo consolidato di #326 (2026-08-11) li esclude. Per spec-bot-tattico §8 threat projection e information gain sono belief. *(docs/roadmap/roadmap-post-v0.1.md (### E26); gh issue view 326; docs/gameplay/spec-bot-tattico.md §8; D-097, D-099)*. **Governa**: Decision Log (D-097, D-099) e il corpo consolidato di #326; roadmap-post-v0.1.md va riallineata. Per E26 vale il perimetro di #326 e spec-bot-tattico.
- `STALE ROADMAP` — #326 e le figlie #531-#534 sono nella milestone v0.2 con 'non si apre prima dei gate della v0.1', mentre il brief del 2026-11-04 sposta E26 nella v0.1. Inoltre CP 26.5 (#2269) è già CLOSED. *(gh issue view 326 531 532 533 534 2269 --json milestone,state; docs/roadmap/plans/v01-giocabile-brainstorm-2026-11-04.md §6; git grep -n WObjective -- Source ':!*/Tests/*')*. **Governa**: Il brief del 2026-11-04 (decisione d'autore confermata) governa l'intento, ma non c'è ancora un D-nnn: finché non è registrato, il Decision Log e le milestone sono quelli di v0.2.
- `STALE ROADMAP` — Il DoD di CP 13.4 e CP 13.5 cita test mai scritti: Noise.ProducesUncertainContact, Noise.AttackRevealsDirection, Noise.ObserverViewOmitsUnheard, Noise.HashIsIndependentOfObserver (assenti perché l'emissione manca) e Bot.PlansOnPartialKnowledge, Bot.DoesNotTargetUnknown, Bot.ConsidersExposedRearArc (assenti, la copertura reale è HexBotPlay.HiddenEnemyFairness e altri). Inoltre la riga 'Team Knowledge' della v0.1-definition-of-… *(docs/roadmap/roadmap-v0.1.md (tabella E13, CP 13.4 e 13.5); gh issue view 159 160; docs/roadmap/v0.1-definition-of-done.md riga 361; git grep -l '"RefactorTactics.<nome>"' -- Source → 0 per i sette nomi, 1 per…)*. **Governa**: Il codice e i test eseguibili governano lo stato; il DoD va riscritto con nomi di test esistenti.
- `IMPLEMENTATION DRIFT` — spec-bot-tattico §7 dice che il caso 'umano + bot nella stessa squadra' non esiste in v0.1, ma RTGameMode e il bootstrapper hanno BotAllyCount (rt.Match.BotAllies) e la modalità autobattle. *(docs/gameplay/spec-bot-tattico.md §7; Source/RefactorTactics/RTGameMode.h:132-165; Source/RefactorTactics/Match/RTMatchBootstrapper.h:81-105)*. **Governa**: Il codice è evidenza dell'implementazione; il contratto (spec) resta quello finché una decisione non lo cambia. Richiede una decisione, vedi domande aperte.
- `CONTRACT CONFLICT` — Il brief del 2026-10-14 (decisione F2b) vuole il ricordo del nemico fino a smentita, mentre il brief di gioco, CP 13.2 e il test Vision.LastContactExpiresAfterOneTurn fissano la persistenza di 1 turno; lo stesso brief dichiara che il test va riscritto. Nessun D-nnn lo ha ancora registrato (il brief dice 'nessuna decisione registrata'). *(docs/roadmap/plans/fog-of-war-ripensata-brainstorm-2026-10-14.md §4 e §7; Source/RefactorTactics/Perception/RTTeamKnowledge.h:239; Source/RefactorTactics/Tests/Perception/RTTeamKnowledgeTests.cpp:219; roadmap-v0.1.md CP…)*. **Governa**: Oggi vale la persistenza di 1 turno (codice e test); la modifica richiede un D-nnn d'autore registrato prima di riscrivere il test. Non ho verificato se un D-nnn successivo al brief l'abbia…
- `STALE ROADMAP` — Il corpo di #464 e la sua tabella di candidati citano Riva, Bastion, Flux e Vektor; il roster attuale è Aevik, Muiren, Branth, Ivrin. Il corpo di #151 cita ancora come attesi documenti uscendo dal repository (docs/src/). *(gh issue view 464 151; Source/RefactorTactics/Ability/Catalog/RTHeroCatalogLibrary.cpp (GetHeroIds); docs/roadmap/bot-competence.yaml (eroi))*. **Governa**: Il catalogo in codice governa il roster; i corpi delle issue vanno ripuliti.

### 4.6 · Ondata 4b — Roster a 8 (E35)

La corrispondenza schede-nomi RT è già scritta e chiusa (`docs/characters/paragon.md:24-27`, D-322): Steel=Ward, Murdock=Vigil (Sentinel Directorate); Aurora=Rime, Kwang=Tethra (Resonance). Il brief la dà per non scritta (STALE). Il rename degli ID del roster v0.1 è già eseguito (#2491 CLOSED): `Hero.Aevik/Muiren/Branth/Ivrin`; restano residui di presentazione (icone, nomi di test) sotto #2297. Il motore esprime quasi per intero Steel e Whiteout/Hoarfrost di Aurora. Mancano tre primitive grosse: l'oggetto persistente/piazzato (Frozen Simulacrum, Sword Anchor, Lightning Return, Concussive Mine, Ice Mirror), il ghiaccio prodotto da un'azione (nessuna lo produce) e le risorse di kit (D-265/D-324). Aurora e Kwang non hanno pack Paragon nel progetto. `RosterIsBalanced` pinna oggi 4 eroi, con indici fissi e 4 affinità distinte.

**Contraddizioni fra le fonti** *(nessuna è riconciliata in silenzio: ciascuna dice chi governa)*

- `STALE ROADMAP` — Il brief (§6 e §7) dice che la corrispondenza schede-nomi RT 'non è scritta nei file letti' e la tiene fra le domande aperte. È scritta ed è chiusa: `paragon.md` la dà come tabella normativa, deliberata il 2026-09-04 (D-322). *(docs/roadmap/plans/v01-giocabile-brainstorm-2026-11-04.md §6, §7; docs/characters/paragon.md:24-27,127-130,40)*. **Governa**: `docs/characters/paragon.md` + D-322
- `STALE ROADMAP` — Il brief (§2 riga 'Roster') e D-484 (2) trattano la migrazione dell'identità (#2297) come differita. Il rename secco degli ID del roster v0.1 (`Hero.Gadget` → `Hero.Aevik` ecc.) è già in `main` (#2491 CLOSED); restano la fetta E e i residui di presentazione. Il testo di #2297 elenca ancora misure (632 token) di prima del rename. *(docs/roadmap/plans/v01-giocabile-brainstorm-2026-11-04.md §2; docs/decisions/RT_PDR_00_Decision_Log.md riga 547 (D-484); Source/RefactorTactics/Ability/Catalog/RTHeroCatalogLibrary.cpp:314-325; Source/RefactorTactics/…)*. **Governa**: Il repository (codice + #2491 chiusa)
- `STALE ROADMAP` — CP 35.4 e il corpo di #322 citano `docs/wiki/fazioni/`. La cartella non esiste: `docs/wiki/` è vuota per D-076, la Wiki è un clone esterno. *(docs/roadmap/roadmap-post-v0.1.md:241-257 (E35.4); docs/wiki/README.md; gh issue view 322 (voce barrata nel corpo))*. **Governa**: D-076 (`docs/wiki/README.md`)
- `STALE ROADMAP` — Le quattro schede portano `RT Character ID: TBD` e 'il nome coincide, non è deciso'. La decisione è presa (Ward, Rime, Vigil, Tethra, D-322). *(docs/characters/v0.2/{aurora,kwang,murdock,steel}.md riga 9 e 12; docs/characters/paragon.md:40)*. **Governa**: `docs/characters/paragon.md`
- `CONTRACT CONFLICT` — Le schede assegnano a ogni abilità un 'Costo risorsa' (1-4) e a ogni eroe una risorsa firma. D-324 ha tolto la risorsa firma universale e `Energy` dal gameplay; D-265 ammette una risorsa solo come meccanica di un kit specifico, con contratto proprio. Lo stesso vale per gli ID `ABL_*`/`Ability.*`, che portano il nome Paragon (D-297: la sorgente Paragon è presentazione). *(docs/characters/v0.2/aurora.md:78-91 e tabelle abilità; D-324, D-265, D-297 (docs/decisions/RT_PDR_00_Decision_Log.md))*. **Governa**: D-324/D-265 (Decision Log accettato). Le schede sono DATA_SPEC e non sono contratto runtime.
- `IMPLEMENTATION DRIFT` — La DoD di CP 35.5 (#2291) vuole un gate sui campi HeroId, DisplayName, namespace ActionId, Gameplay Tag e voci di catalogo. Oggi c'è solo il ratchet di file per 4 token (`RTLegacyIdentityRatchetTests`), e il gate documentale è stato rimosso da D-182. Intanto IconId e file icona di `Gadget/Riktor/Wraith`, e i nomi di test `Heroes.Phase.*`, sopravvivono nel namespace RT. *(Source/RefactorTactics/Tests/Meta/RTLegacyIdentityRatchetTests.cpp; Content/Icons/manifest.json; Content/Icons/Icons/RT_UI_Icon_Identity_Gadget.svg; docs/roadmap/bot-competence.yaml:46,281; #2291 corpo (D-182))*. **Governa**: Roadmap E35.5 e #2291 (obiettivo); il repository descrive lo stato attuale
- `IMPLEMENTATION DRIFT` — La riga del brief e E35 trattano `Heroes.RosterIsBalanced` come test da 'estendere a 8'. Il test codifica 4 come costante, usa indici fissi e 'quattro affinità distinte'; il suo commento dice già che la forma per-coppia regge a 8. *(Source/RefactorTactics/Tests/Ability/RTHeroIvrinTests.cpp:223-322; docs/roadmap/roadmap-post-v0.1.md:255)*. **Governa**: Codice del test (stato) + roadmap E35.3 (obiettivo)

### 4.7 · Ondata 5 — HUD, schermate e gate

HUD in-match e shell del frontend esistono come C++ più WBP_RT_* e hanno i loro oracoli headless. Restano aperti i verdetti d'autore su dock e feed, il layout della schermata Result e il modale d'errore #1330. Mancano il Match Setup (nessun id schermata, nessun WBP), il kit UI pulito, un contratto scritto di «un solo linguaggio visivo» e le icone E20 (CP 20.2 e 20.3). La misura sugli asset indica due linguaggi: i WBP di partita referenziano i font F_RT_*, quelli del frontend no. I gate sono letti da un solo consumatore automatico, il test Meta RTGateCoherenceTests, più prosa che nomina il perimetro «G1–G14 più G16» in più documenti. G17 si aggiunge con poco rischio tecnico se nasce non-✅. Il lavoro vero è riscrivere G13 (oggi ✅ per la deroga D-544, con seduta pilotata dall'assistente), decidere la forma del verdetto e riallineare PIA, che non riporta su G16 e ha PIA-1…PIA-6 aperte.

**Contraddizioni fra le fonti** *(nessuna è riconciliata in silenzio: ciascuna dice chi governa)*

- `STALE ROADMAP` — roadmap-v0.1.md (E46, CP 46.5) e le righe PIE-V01-FRONTEND-RESULT e -PAUSE dicono che WBP_RT_ResultScreen e WBP_RT_PauseMenu mancano. I due asset sono tracciati da una data precedente alla riga di roadmap, e la schermata Result è stata osservata a schermo nella seduta del 2026-10-26. Anche l'osservazione sul layout della schermata Result è sul packaged. Governa il repository (stato live). *(docs/roadmap/roadmap-v0.1.md (E46, tabella CP); docs/technical/test-manuali-pie.md (PIE-V01-FRONTEND-RESULT, -PAUSE); git log -1 --format='%h %cs' -- Content/RT/UI/Framework/WBP_RT_ResultScreen.uasset → f6724a…)*. **Governa**: Repository e build packaged (stato live); le righe di roadmap e di registro vanno aggiornate
- `CONTRACT CONFLICT` — DoD §7 dichiara «nessuna direzione artistica… l'obiettivo è la leggibilità, non la qualità visiva» e E46 CP 46.3/46.4 dichiara il Match Setup preconfigurato e Settings «coming soon». La ridefinizione confermata dall'autore chiede arte d'ambiente, scelte pre-partita e un solo linguaggio visivo. Il brief stesso dice che fino al D-nnn lo scope resta quello di roadmap-v0.1.md. *(docs/roadmap/v0.1-definition-of-done.md §7; docs/roadmap/roadmap-v0.1.md (E46, CP 46.3 e 46.4; E21 con D-124); docs/roadmap/plans/v01-giocabile-brainstorm-2026-11-04.md (intestazione, §3, §8))*. **Governa**: Le owner specification e la DoD correnti fino al merge del D-nnn; poi il D-nnn
- `IMPLEMENTATION DRIFT` — G13 è ✅ nella tabella (deroga D-544) mentre un commento su #940 elenca, dalle schermate del v3, difetti visibili sulla schermata Result (sovrapposizione al roster, lingua mista, basso contrasto). Il commento dichiara di non essere un giudizio d'autore: non si tratta di una violazione della DoD di #940, ma il ✅ di G13 vale «meccanica misurata più occhio riportato dal v2», e l'occhio non ha guardato questo. *(docs/roadmap/v0.1-definition-of-done.md (cella G13); gh issue view 940 --json comments (ultimo commento); docs/decisions/RT_PDR_00_Decision_Log.md (D-544, riga 607))*. **Governa**: D-544 per il valore di G13 sul v3; non copre la nuova definizione
- `STALE ROADMAP` — L'intestazione di editor-sessions.yaml dice che nessuno script legge il file, mentre la cella di G16 descrive l'asserzione A2 di doc-coherence.ts (seduteOrfane) che lo legge e rende rosso G14 se un record critico non ha issues:. Chi aggiunge la seduta di G17 deve seguire la cella, non l'intestazione. *(docs/roadmap/editor-sessions.yaml:1-8; docs/roadmap/v0.1-definition-of-done.md (cella G16); tools/radar/doc-coherence.ts:354 e 439-466)*. **Governa**: Il codice del radar (doc-coherence.ts) per cosa è letto

## 5. Triage del backlog aperto della milestone v0.1

Una classe per issue, assegnata da due agenti e **non ancora confermata dall'autore**. `RESTA-FUORI-O-DIFFERIBILE` è una **proposta di taglio**: nessuna issue è stata toccata. Una issue `P2` o `P3` è tagliabile se nessun gate diventa rosso ([D-377](../../decisions/RT_PDR_00_Decision_Log.md)), e una `P0` aperta non rende rossa la release: lo fa un gate. Confidenza: `a` alta, `m` media, `b` bassa.

### 5.1 · `W1-ambiente`

| Issue | Priorità | Titolo | Motivo | Conf. |
|---|---|---|---|---|
| #289 | P1 | CP E21.3 — Leggibilità tattica | Anelli, colori delle superfici leggibili in partita e camera sulla scala esagonale; citata dalla DoD (riga 284). Si fonde con la leggibilità dell'ondata 1. | m |
| #324 | P1 | [EPIC v0.1] E23 · Muri, porte e interaction graph | Quattro figlie aperte: #2330 (porte, W1), #2827, #834, #1831 (editor, differibile). Ambiente interagibile della mappa.→ #2330, #2827, #834, #1831 | b |
| #695 | P2 | Viz editor: quattro stati di porta, due forme — «chi può aprirla» non si vede | Leggibilità delle porte sulla mappa: Locked scambiata per Closed cambia un piano. Rientra nella resa delle superfici della W1; follow-up di #553 chiusa. | b |
| #1095 | P2 | Seduta U25 — il volume di posa della cella, e la scena che dice se il graybox si legge | Il brief §5 la elenca tra ciò che la W1 riusa: contratto di ingombro del Graybox Kit e scena di verifica. | a |
| #1758 | P1 | CP E21.3 — La griglia si vede in partita | Confine fra celle visibile in partita con toggle non-debug: leggibilità tattica dell'ambiente, ondata 1. | m |
| #1941 | P2 | OVL-01 · Semantic Area Overlay Foundation: cinque significati disegnati, zero modello che li dichia… | Gli overlay oggi sono DrawDebugLine. La W1 chiede superfici riconoscibili senza console debug e leggibilità tattica; classe incerta, potrebbe essere W5. | b |
| #1942 | P2 | OVL-02 · Volumetric Region Boundary: una regione si legge dal perimetro | Perimetro di regione sulla griglia, dipende da #1941. Stessa incertezza: resa dei significati sulla mappa, non nelle scelte del brief. | b |
| #2330 | nessuna | Una porta del gray kit non si puo' posare su nessuna mappa | Manca l'anello di autoraggio delle porte su mappa Dev; il brief la nomina come riuso dell'ondata 1. | m |
| #2492 | nessuna | Visual.Environment.Acceptance promette cinque fenomeni e ne produce tre | Scenario di accettazione visiva dell'ambiente con buchi aperti; il brief lo cita come riuso dell'ondata 1. | m |

### 5.2 · `W2a-animazioni`

| Issue | Priorità | Titolo | Motivo | Conf. |
|---|---|---|---|---|
| #286 | P1 | [EPIC v0.1] E21 — Presentazione e leggibilità | Epic 'fuori dai cilindri colorati': le figlie aperte sono #289 (W1), #2554 e #2167 (animazione/posa). Copre W1 e W2a.→ #289, #2554, #2167 | b |
| #1750 | P2 | La sagoma dell'ultimo contatto si disegna in posa di riferimento: le catene di Riktor si stendono s… | Difetto di posa della mesh fantasma: nessuna classe di animazione assegnata. Va risolto insieme alla messa a punto delle pose dei personaggi. | m |
| #1793 | P2 | Phase guarda dalla parte sbagliata mentre si muove: MeshYawOffset e' -90 su tutti e quattro | Difetto di orientamento della mesh in movimento, solo presentazione. Il brief §5 lo elenca per la W2. | a |
| #2167 | P2 | La posa non segue piu' il facing: il facing logico cambia a ogni passo e la mesh no | Il brief §5 lo elenca per la W2. Parent #288 (E21.2), ordine obbligato con #1793. | a |

### 5.3 · `W2b-vfx-audio`

| Issue | Priorità | Titolo | Motivo | Conf. |
|---|---|---|---|---|
| #2454 | P2 | [v0.1] Combat Feedback — evento → cue: attivazione, impatto e sconfitta | Dorsale evento → cue. Tre voci NoPresentation dichiarate in DeclaredBindings(). Il brief §5 la elenca per la W2. | a |
| #2456 | P2 | [v0.1] Combat Feedback — ciclo di vita degli stati: pulse e fade delle icone su StatusChanged | Il brief §5 la elenca per la W2. Manca il momento (pulse e fade) delle icone di stato, già persistenti. | a |
| #2457 | P2 | [v0.1] Combat Feedback — diagnostica evento/animazione separata dal layer player-facing | Nominata dal brief §5 accanto a #2454 e #2456. Strumento di diagnosi di dove cade una cue, utile al collegamento di VFX e suoni. | m |

### 5.4 · `W3-setup`

| Issue | Priorità | Titolo | Motivo | Conf. |
|---|---|---|---|---|
| #2652 | P2 | [UI] Hero Profile — binding dei dati e verifica Editor/PIE | Le schede mostrano ancora 'Text Block': nessun binding alla view. Utile se la scheda eroe serve alla scelta degli eroi; uso non confermato nelle roadmap. | b |

### 5.5 · `W4a-bot`

| Issue | Priorità | Titolo | Motivo | Conf. |
|---|---|---|---|---|
| #151 | P2 | [EPIC v0.1] E13 — Conoscenza parziale: vista e udito | Il brief §5 chiede di completare E13 prima di E26. Restano aperti sotto di essa #159, #160 e #1466. | a |
| #159 | P2 | CP 13.4 — Rumore → contatto incerto, vista filtrata per squadra | Nominata dal brief §5 come coda di E13, che E26 richiede come dipendenza. | a |
| #160 | P2 | CP 13.5 — Bot e HUD sulla conoscenza parziale | Nominata dal brief §5. Il corpo dice che la metà bot è in main; resta la metà HUD, che potrebbe scorrere in W5. | m |
| #1466 | P2 | E13.6 · La conoscenza parziale diventa visibile: la porta filtrata, i due leak, la sagoma | Il brief §5 la nomina nel completamento di E13. Parte visiva della conoscenza parziale, la spec vive su un branch feat/conoscenza-parziale-fase-a. | m |

### 5.6 · `W4b-roster`

| Issue | Priorità | Titolo | Motivo | Conf. |
|---|---|---|---|---|
| #3065 | P2 | Il colpo al buio di D-380 ha un owner che il giocatore non controlla: MortarShot e' di Branth | Domanda di roster, non difetto: il tiro indiretto è di un eroe della squadra 1. Si risolve con scelta fra quattro uscite, che può spostare roster o catalogo. | b |

### 5.7 · `W5-hud`

| Issue | Priorità | Titolo | Motivo | Conf. |
|---|---|---|---|---|
| #25 | P1 | [EPIC v0.1] E11 — HUD, log e debug | Epic HUD: figlie aperte #77, #613, #705, #172, #173, #79, #80 e correlate. Prevalentemente schermate e interazione. | m |
| #77 | P1 | CP 11.1 — HUD di partita completo | HUD di partita (barre, round, informazioni per giocare un turno); dieci caselle aperte, nessuna spuntata. | m |
| #172 | P1 | CP 11.5 — Ghost Timeline: preview del piano per fase | Anteprima per fase del piano: 'vedere l'anteprima e capire l'esito'. Lavoro corposo (23 caselle aperte, 7 chiuse nel corpo). | m |
| #217 | P2 | [EPIC] E20 · HUD Icon Language | Catalogo di icone semantiche per l'HUD. Sotto di essa restano aperti #219 e #220. Serve al linguaggio visivo unico della W5. | m |
| #219 | P2 | CP 20.2 · Categorie della v0.1 | Popola cinque categorie del catalogo icone. Stato di avanzamento non letto nel dettaglio: la classe segue l'epic #217. | b |
| #220 | P2 | CP 20.3 · I widget consumano il catalogo | I widget leggono le icone dal dato. La DoD chiede anche PIE-ICON-01, lettura a schermo: icone distinguibili in HUD. | m |
| #613 | P1 | CP 11.7 — Screen HUD in UMG (layer §4.1) | Screen HUD in UMG, cuore delle schermate di partita; caselle aperte e 29 commenti, verdetto PIE-V01-SCREENHUD pendente. | a |
| #705 | P1 | CP 11.8 — Pointer Interaction Contract: Hover / LMB / RMB | Contratto del puntatore: hover, click, mira; possiede il cablaggio di #3063. Targeting 'si usa da solo'; molte caselle aperte, stato non misurato. | m |
| #934 | P1 | [EPIC v0.1] E46 · Frontend shell e ciclo di partita | Epic frontend; resta una figlia aperta (#940 o #1330). Il Match Setup di W3 emenda CP 46.4, ma l'epic qui chiude con la resa schermate.→ #940, #1330 | m |
| #940 | P1 | CP 46.5 · Result, e il ritorno al menu | Result implementato (PR #3139) ma resa non finita: overlay sul roster, lingua mista (commento 2026-10-26). Schermata finale da rifare in W5. | m |
| #1330 | nessuna | Il modale d'errore ha la forma e nessun filo | WBP_RT_ErrorModal mockup non cablato (mostra 'Text Block'); il brief lo nomina come riuso dell'ondata 5. | a |
| #1392 | P2 | Il colpo di boundary rispetta la copertura, e nessuno lo dice: traccia, log e rotta leggibile (E11/… | La regola c'è, al giocatore non arriva: mancano traccia e log che spieghino il bypass di copertura. Chiarezza dell'esito, con giudizio a schermo. | m |
| #1614 | P1 | CP 11.8 — L'hover si vede: overlay pointy-top maggiorato | Feedback visivo del puntatore in partita, parte della usabilità del targeting; nessuna casella spuntata. | m |
| #1615 | P1 | CP 11.8 — Settore sotto il cursore: dodici spicchi da 30° | Feedback direzionale locale del puntatore; rifinitura di mira, candidata a differimento se W5 stringe. | b |
| #1835 | P2 | CAM-14 · Il piano attivo non si vede: D-255 cambia quale cella si seleziona | Il giocatore ha solo il log per capire quale piano è attivo. Owner HUD #613, pattern di view-model già presente. Chiarezza di partita. | m |
| #2405 | P2 | La minaccia di caduta non e' leggibile in pianificazione, e nel caso saturo l'esito non si spiega | Leggibilità in pianificazione e spiegazione dell'esito di una meccanica già in v0.1 (D-332), riusa il Player Event Log. | m |
| #2757 | P2 | HUD v0.1 — Osservabilità di stato e binding: bShowDebug è dichiarato e non ha un solo uso | Diagnostica dello Screen HUD §4.1: zone, source reale, 0 distinto da non popolata. Qualità dell'HUD, ma lavoro soprattutto interno. | b |
| #3176 | P2 | D-426 · Il giocatore vede il segno e legge la riga, e non li unisce | Collegamento fra segno in mappa e riga del log: meccanismo da scegliere. È chiarezza per il giocatore tra feed e mappa. | m |

### 5.8 · `CODICE-IN-MAIN-MANCA-VERDETTO`

| Issue | Priorità | Titolo | Motivo | Conf. |
|---|---|---|---|---|
| #952 | P1 | [EPIC v0.1] E47 · Mini v0.1 Autobattle — la partita che si guarda | Figlie quasi tutte completate; la residua è il verdetto a schermo del workflow roster (#2748/#2744).→ #2748, #2744 | m |
| #1936 | P1 | Il log a schermo racconta le celle, non la partita: entra il feed del giocatore | Consuntivo 2026-10-25: codice in main; restano PIE-V01-SCREENHUD (criteri 3, 6, 8) e il pacchetto NOT RUN. | a |
| #1937 | P1 | [EPIC] Player Event Log & Explainability | Epic cross-release fino a v1.0; l'unica figlia v0.1 aperta è #1936, codice in main con verdetto d'autore pendente.→ #1936 | m |
| #2554 | P1 | Anteprima dell'Anim Browser non si anima e non inquadra | Fix mergiato (PR #2591); DoD chiede osservazione in Editor sui quattro pack. Strumento utile al lavoro clip di W2a. | m |
| #2744 | P1 | Screen HUD §4.1 sa di essere in autobattle | Dato, binding e privacy PASS in main; resta la seduta PIE-V01-SPECTATOR-ROSTER dell'autore. | a |
| #2748 | P1 | E47 AutoBattle Viewer 0.1: il roster si sceglie fuori dall'Editor | Codice in main (PR #3717); resta PIE-V01-VIEWER-ROSTER. Il brief la nomina come riuso per W3 (resolver sui roster). | a |
| #2764 | P2 | Il §4.1 ridecide nei Blueprint ciò che il C++ decide e testa | Consuntivo del 2026-10-25: PR #3847, #3849 e #3850 in main. Resta solo il verdetto d'autore di PIE-V01-SCREENHUD. | a |
| #2826 | P1 | L'Action Dock arma e si spiega: restano i verdetti di seduta | Tutti gli scope in main; restano DOCKKEYS/DOCKPROMPT/DOCKCLICK e stati Cooldown/Unavailable/Warning dal vivo. Parte del tema 'skill si usano da sole'. | a |
| #2951 | P1 | Colpo verso altro piano rifiutato con OtherLayer: restano verdetto e VERT-4 | Codice in main (PR #2961, #3060); resta PIE-V01-REFUSAL e la decisione VERT-4 sulla regola del piano. | a |
| #2964 | nessuna | Il feed e' montato ma a schermo non compare nessuna riga | #1936 (2026-10-25) la tiene aperta per verdetti d'autore; titolo è il sintomo del 2026-09-11, possibile STANTIA se la seduta lo smentisce. | m |
| #2989 | P1 | Cablaggio del dock: oracoli headless, restano verdetti di seduta | Consuntivo D-507: caselle spuntate, restano verdetti PIE-V01-DOCKTICK/DOCKREBUILD/DOCKCLICK dell'autore. | a |
| #3063 | P1 | Click su cella mai osservata muore nel trace di OnSelect | Fix in main (PR #3114), criterio 1 PASS in seduta 2026-09-13; restano criteri successivi non letti per intero. | m |
| #3073 | nessuna | Il feed si ricostruisce solo se cambia il NUMERO di righe | Commento 2026-10-18: gate EventLogRewritesLineWhenCountIsUnchanged in main, grafo con la guardia rimosso (#3637); resta il verdetto d'autore (#1936). | m |

### 5.9 · `GATE-verifica`

| Issue | Priorità | Titolo | Motivo | Conf. |
|---|---|---|---|---|
| #16 | P0 | [EPIC v0.1] E2 — Parità hex del substrato | Il corpo corregge: la premessa era falsa; resta solo il playtest #38. Dopo la correzione è un epic di verifica.→ #38 | m |
| #26 | P0 | [EPIC v0.1] E12 — Determinismo, QA e release | Epic dei gate (#81..#85, #178); nessun lavoro proprio, G1-G14 e G16 si rimisurano sul candidate finale e si aggiunge G17. | a |
| #38 | P0 | CP 2.8 — Playtest della partita hex (sessione D) | Owner del playtest da cui dipende G10; verifica, non implementazione. | a |
| #82 | P2 | CP 12.2 — Matrice dei test manuali v0.1 (sessione E) | Resta solo l'esecuzione registrata delle voci PIE-V01. Il corpo dice che non blocca G9 né la consegna (D-377). Tagliabile, assorbibile da G17. | m |
| #84 | P0 | CP 12.4 — KPI misurati e registrati | Owner di G11; da rimisurare dopo arte e VFX (Client FPS), come scrive il brief §6. | a |
| #85 | P0 | CP 12.5 — Release interna v0.1 | Attestazione della release su un candidate; la ridefinizione esclude di dichiararla sul v3 e rimanda a G17. | a |
| #403 | P2 | BAL-1: la decisione c'e' (D-121), resta il gate umano U20/PIE-BAL1 | Resta la casella PIE-BAL1 (seduta U20). Il corpo dice che oggi non è eseguibile: D-205/207/208 non sono nel codice. L'autore ha rinviato la scelta alla seduta. | m |
| #1408 | P1 | [EPIC v0.1] E48 · Il giocatore raggiunge il modello | Figlie completate; resta una casella: eseguire PIE-V01-REACTCOND. Il resto è verdetto di seduta. | m |
| #2407 | P3 | Evidenza packaged della caduta: il TurnLog si rilegge da processo pulito e il bordo sopravvive al c… | Evidenza da build packaged (persistenza, cook). Verifica, non produzione. P3, prima da tagliare. | a |
| #2408 | P2 | Seduta di accettazione della caduta: dieci scenari in una sola apertura di Editor | Seduta unica di accettazione umana. Prerequisiti #2401-#2406, build coerente con HEAD e suite VALIDA. | a |
| #2411 | P2 | v0.1 · Resolution playback — il corto che arriva prima e aspetta | Voce PIE di giudizio a schermo: i mover da 1 cella si leggono come arrivati o bloccati. Nessun codice da scrivere, serve un osservatore. | a |
| #2476 | P2 | Seduta U46 — resta la rigiudicazione di PIE-VIS-SIGHTWALL | Resta un verdetto PIE (PIE-VIS-SIGHTWALL, fuori subset); il corpo dice che il resto è cronaca. Il residuo A2/G14 è forse coperto da #3384 e #3385. | m |
| #2601 | P0 | [V0.1][TOOLS] Vertical Slice Labs & Playback — smoke gate integrato | Il corpo dice che è il gate G16: verifica, non implementazione; da rimisurare sul candidate finale. | a |
| #2916 | P2 | PIE: i comandi di playback di #2858 non hanno ancora una voce nel registro | Manca una voce nel registro PIE e la sua esecuzione, per un'ergonomia che nessun test headless vede. Procedura già scritta nel corpo. | a |
| #3384 | P2 | Convoca U10 — Data asset delle azioni: le abilita' sono hard-coded, e il prefisso non e' ancora dec… | Convoca la seduta critica U10 per l'asserzione A2 di G14. Chiede asset di dati e validator. Se le skill diventano dati può toccare W2/W4b. | m |
| #3385 | P2 | Convoca U14 — Ambiente in partita: nove voci su undici hanno un esito, e nessuna e' verde | Convoca la seduta critica U14 (A2 di G14). Mancano esiti reali di PIE-V01-ELEC e PIE-V01-FIREWATER; marcatore da scrivere solo a done_when soddisfatto. | a |
| #3424 | P3 | L'automation sul binario packaged uccide la GPU allo shutdown: exit 3 | Difetto dichiarato nella cella G2 e mai convocato: il verdetto va contato dai Result. Tocca la rimisura su packaged. P3, nessuna modifica proposta. | m |

### 5.10 · `RESTA-FUORI-O-DIFFERIBILE`

| Issue | Priorità | Titolo | Motivo | Conf. |
|---|---|---|---|---|
| #80 | P1 | CP 11.4 — Comandi rt.Debug.* | Strumenti di ispezione, non giocabilità; il brief chiede superfici leggibili senza console debug. #1758 lo riusa. PROPOSTA. | b |
| #153 | P1 | [EPIC v0.1] E15 — Showcase «Il Relè» e golden replay | Showcase/golden replay è fixture e demo; figlie aperte #171 e #2149. Il golden replay però può servire a G4: da verificare. PROPOSTA.→ #171, #2149 | b |
| #171 | P1 | CP 15.5 — Presentazione e playtest di leggibilità | Showcase guardabile in editor: sovrapposto a G10/G13 e al nuovo G17 (verdetto d'autore). PROPOSTA; da confermare con l'autore. | b |
| #173 | P1 | CP 11.6 — Scrubbing delle fasi e ramo condizionale della reaction | Strumento avanzato di pianificazione (scrubbing); nessuna casella spuntata e un solo commento. Dipende da #172. PROPOSTA. | b |
| #221 | P3 | [EPIC] E17 · Validazione di stress 4v4 | È una misura di stress con 8 unità, non formato di gioco (D-011). Il brief lascia il 4v4 fuori scope. P3, prima da tagliare. | a |
| #224 | P3 | CP 17.3 · Leggibilità con otto unità | Giudizio a schermo su partita 4v4 registrata. Nessuna scelta del brief lo nomina. Dipende da CP 17.2 e dalla Ghost Timeline. | a |
| #314 | P3 | CP 14.7 — Reaction Profile e Reaction Clash (opportunity contested) | Meccanica nuova di reazione contestata, P3. Nessuna scelta d'autore la richiede; aumenterebbe la matrice di interazioni da testare. | a |
| #319 | P3 | CP 14.8 — Decision Time Bank (budget di decisione per giocatore) | Risorsa temporale del giocatore, P3, segue CP 14.7. Non serve alla giocabilità definita nel brief. | a |
| #834 | P2 | S1 e D1: il giocatore non puo' sapere che cosa controlla quella struttura | Il corpo dichiara Release v0.2 (E23, CP 23.5), ma la milestone è la 6: divergenza da portare all'autore. Etichette di struttura, non serve alla giocabilità definita. | m |
| #995 | P2 | [DOCS/DESIGN] Consolidare Elemental Proficiency v0.1, accanto all'asse Affinity | Solo documentazione di design con grammatica dei gradi. Può informare il roster a 8 ma non è nel percorso della build giocabile. | m |
| #1774 | P2 | CAM-05 · La Strategic View e' una conseguenza dello zoom, con isteresi — e oggi non esiste | Il nucleo (Tab, segnalini) è in v0.1 per D-488, la taratura è post-v0.1 (#1780). Comfort di camera, non nelle scelte del brief. | b |
| #1831 | P2 | Editor: i dodici settori, le CoverSource/CoverOption e i due lati si vedono, con la ragione dell'in… | Strumento di Hex Map mode per chi disegna mappe (E23). Utile alla seconda mappa ma non necessario al giocatore. | m |
| #1838 | P2 | CAM-17 · L'edge pan non esiste (zero occorrenze) | Comodità di input della camera, la pipeline di pan esiste con un solo ingresso. Non è nelle scelte del brief. | m |
| #1881 | P1 | [EPIC] Resolution Playback & Inspection — bot, replay e debug fino alla v1.0 | Epic cross-release fino a v1.0 con una sola figlia aperta (#2411, P2); non serve alla giocabilità definita. PROPOSTA. | b |
| #1943 | P2 | OVL-03 · Priorita' dinamica e Primary/Secondary: l'ordine e' fisso | Rifinitura della famiglia OVL: ordine dinamico dei significati, dipende da #1941. Tagliabile senza perdere la base della leggibilità. | b |
| #1944 | P2 | OVL-04 · I sette significati, la privacy e il costo: quattro hanno un produttore e tre no | Integrazione finale della famiglia OVL: dipende da #1941-#1943 e da #75. Tre significati senza produttore. Si decide dopo OVL-01. | b |
| #2149 | nessuna | Le due battute del T8 dello showcase che la coreografia non produce | Coreografia della showcase golden replay, non giocabilità; il corpo stesso dice che le battute non sono ottenibili oggi. | m |
| #2597 | P2 | Enemy Inspection — click sul nemico osservato: MOVIMENTO e MINACCIA come regione | Dipende dalla famiglia OVL (#1941-#1944), l'affordance del click è già occupata. Funzione di ispezione, non nelle scelte del brief. | m |
| #2819 | P2 | Facing: playtest e taratura dei budget di pivot per eroe | Taratura dopo playtest, dipende da #291. L'autore potrebbe raccogliere i dati nelle partite di G17. | m |
| #2827 | P2 | Nessuna azione puo' chiedere una struttura: TargetKindForAction non produce mai Object | Targeting di strutture (E23): nessuna azione del roster di oggi lo richiede. Dipende da #2330, #705 e #1941. | m |

### 5.11 · `ALTRO`

| Issue | Priorità | Titolo | Motivo | Conf. |
|---|---|---|---|---|
| #14 | P0 | [EPIC v0.1] Vertical slice 2v2 su hex — release v0.1 | Tracker della release: corpo da riscrivere dopo il D-nnn (brief §8). Non un gate.→ #3985 | a |
| #79 | P1 | CP 11.3 — Combat log con reason code completi | Log diagnostico dettagliato, citato da G16 (DoD riga 95); stato non misurato, quattro caselle aperte, nessuna spuntata. | b |
| #152 | P0 | [EPIC v0.1] E14 — Overwatch e reazioni interattive | Restano #166 (P0) e #314, #319 (P3, differibili per D-377). Non è un gate; la parte differibile è nei P3.→ #166, #314, #319 | m |
| #166 | P0 | CP 14.6 — Counterplay, UI della finestra e misura del pacing | Mischia UI della finestra di reazione (profondità/counterplay) e misure p50/p90 del pacing; 35 commenti, 19 caselle aperte. Non assegnato a un'ondata. | m |
| #291 | P2 | Facing finale: il secondo click sulla destinazione sostituisce il tasto T | Interazione di input del facing da completare (budget 0-3, direzioni illegali non cliccabili). Pertiene a 'skill usabili', che il brief non assegna a un'ondata. | m |
| #2276 | P1 | [EPIC] Correzioni meccaniche — validazione gameplay e leggibilità decisionale | Epic cross-capability: caselle residue sono lavoro di mappatura CM-01..CM-10 e un figlio (#2819 facing). Tocca 'profondità' ma senza ondata. | b |
| #2388 | P1 | [EPIC] Verticalità tattica — Ledge, Fall e Forced Movement | Meccanica mergiata (PR #2500); restano #2402, #2404, #2405 (VERT-1 attende l'autore), #2407, #2408. Non nominata dal brief. | b |
| #2462 | P0 | [P0][ROADMAP] Reaction hardening | Tracker: caselle residue spostate in #2516 e #166 (misure pacing/CPU); nessun lavoro proprio. Non è un gate.→ #166, #2516 | m |
| #3420 | P2 | L'anteprima della visibilità si calcola dalla cella d'arrivo, non da quella attuale | Anteprima di pianificazione (celle che si aprirebbero dal percorso): usabilità del targeting e dell'anteprima, senza ondata nel brief. DoD emendata il 2026-10-06. | m |
| #3985 | P0 | [ROADMAP v0.1] La build giocabile — ridefinizione della release | Tracker della ridefinizione, non possiede scope né gate; segue il brief e il D-nnn di §8. | a |

## 6. Limiti dell'audit

- **Sola lettura, con un tetto.** Ogni agente aveva un tetto di chiamate di strumento e l'obbligo di scrivere in «non verificato» ciò che non aveva visto. Il non verificato di ciascuna ondata è nell'allegato corrispondente; qui il limite comune.
- **Nessun Blueprint e nessun `.uasset` è stato aperto**, perché l'Editor non era raggiungibile (`unreal-mcp` non connesso). Dove l'audit dice che un asset «non referenzia» qualcosa, intende i riferimenti forti leggibili nel binario; dove dice che il C++ non fa qualcosa, non dice che non lo faccia un Blueprint.
- **Cascade in 5.8.1 non è misurato.** L'audit trova il modulo runtime nell'Engine installato e il plugin Editor abilitato di default, ma nessuno ha visto un `P_*` renderizzare in un pacchetto.
- **Il triage del backlog è per classe, non per decisione.** È una lettura di titolo ed estratto del corpo, a confidenza dichiarata; solo le issue in cui non bastavano sono state aperte per intero.
- **Un errore dell'audit è già noto**, ed è stato corretto in §2: ha dato Wraith per privo di suoni, perché cercava la cartella `Sounds` e il pack usa `Audio`. Un controllo positivo sul perimetro sbagliato (Phase, 564 file) non rendeva la sua affermazione più vera.
- **Gli stati delle issue e le date sono quelli del 2026-11-04** e cambiano da soli. Dove una taglia compare è relativa.

## 7. Che cosa toccherebbe l'ingresso di `G17` e di E35 (dalla revisione di D-576)

Elenco **senza modifiche**: nessuna di queste cose è cambiata. Le liste del roster si riscrivono con l'ondata 4b, non prima.

- **`G17`**: `RTGateCoherenceTests.cpp` (`Meta.SatisfiedGatesAgreeWithTheDefinitionOfDone`) legge gli stati dei gate dalla tabella di §3 della DoD e li confronta con le righe di [`roadmap-pia.md`](../roadmap-pia.md) §4: una riga `G17` deve aprire la cella di stato col glifo giusto e non essere rivendicata da PIA prima del ✅. Il perimetro «`G1`–`G14` più `G16`» è scritto in prosa in più punti: la DoD (§3), `roadmap-pia.md`, `roadmap-v0.1-v1.0.md` (conto «7 + 4 + 3 = 14»), la descrizione della milestone v0.1, il corpo di #85 e `AGENTS.md`. `git grep -nIE '\bG17\b'` oggi colpisce solo il banner della DoD aggiunto da `D-576`.
- **Roster a 8**: `tools/radar/cli.ts` (`EXPECT_HEROES`, `EXPECT_ABILITIES`) e `tools/radar/catalog-code.ts` pinnano quattro eroi; `RTHeroIvrinTests.cpp` (`Heroes.RosterIsBalanced`) codifica quattro affinità distinte e cadrebbe da solo con un ottavo eroe; le liste del roster stanno in `AGENTS.md`, `README.md`, `docs/CONTEXT_INDEX.md` e in `roadmap-v0.1.md` §1; la descrizione della milestone v0.1 dice «quattro eroi canonici».
- **Epic fra le release**: `docs/roadmap/capability-roadmaps.md` colloca `#326` in v0.2 in due punti. Quando E26 ed E35 entrano in `roadmap-v0.1.md`, vanno scritte **nello stesso commit** la tabella di §3, i checkpoint di §5 e la riga «Totale», o `doc-coherence` A5 va rosso. In `roadmap-v0.1-v1.0.md` la riga 88 («sono 24 epic … verificata per costruzione») diventa falsa nello stesso istante.
- **«Quindici gate» e «15 gate»** compaiono ancora in `roadmap-post-v0.1.md` e nella descrizione della milestone v0.2 ed erano già falsi (`G15` è ritirato da `D-181`): chi scrive i banner nuovi non deve riprendere quel totale.
