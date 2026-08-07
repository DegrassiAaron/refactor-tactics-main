# RefactorTactics — Roadmap di prodotto

> **Fase corrente**: post-tutorial · **Ultimo aggiornamento**: 2026-08-05
> Vista di **esecuzione** del progetto: milestone, checkpoint, Definition of Done (DoD) **misurabile** e metodo
> di verifica. Il *cosa* e il *perché* stanno in [`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md); i requisiti
> tecnici di lungo periodo in [`../PDR/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md`](RT_PDR_10_Roadmap_QA_Rischi_v0.2.md).
>
> Regola: un checkpoint è "fatto" solo quando il DoD è verificato col metodo indicato — non "sembra funzionare".

## Chiusura della fase tutorial (2026-08-05)

I due corsi (`02-Tutorial`, `03-TutorialToMVP`) e il framing «percorso per imparare UE 5.8 partendo da C#»
sono **chiusi**. Hanno prodotto l'MVP quadrato M0–M5 e gli incrementi post-MVP; da qui in poi RefactorTactics
è un **progetto di prodotto** e la roadmap risponde a un obiettivo di gioco, non a un programma didattico.

Conseguenze operative:

- I corsi passano da «materiale di riferimento» a **storico** nella gerarchia delle fonti (piano canonico §1).
- Le milestone **M0–M5 sono archiviate** come storico (§ *Archivio*): non si riaprono, non si estendono.
  Il codice quadrato che ne deriva ha una data di dismissione pianificata (**M7**).
- La numerazione delle milestone **prosegue da M6**: la storia del repo resta leggibile.
- Nessun tutoring C++/UE per default nelle risposte (resta disponibile su richiesta esplicita).

## Come tracciamo il lavoro

- **1 milestone = più feature branch**, PR verso il branch padre a checkpoint completo (non a milestone intera:
  i branch lunghi hanno già causato merge onerosi).
- **1 checkpoint = ≥1 commit** con messaggio significativo; si committa quando il DoD del checkpoint è verde.
- Test automatici prima di chiudere un checkpoint che tocca le regole (mai saltarli).
- Le verifiche che richiedono l'editor sono voci in [`test-manuali-pie.md`](../technical/test-manuali-pie.md), non gate impliciti.

## Stato attuale

Legenda: ✅ fatto e verificato · 🟡 fatto in parte (vedi nota) · ⏳ da fare

| Milestone | Stato | Sintesi |
|---|---|---|
| **M0–M5** MVP quadrato (tutorial) | ✅ **archiviata** | Vertical slice 2v2 offline giocabile, packaging Windows, suite verde — vedi § *Archivio* |
| **H0–H6.5** Fondamenta esagonali | ✅ | Coordinate/asset/A\*/multilivello/editor mode + simulazione hex pura (snapshot, budget, collisioni, TurnLog, LOS, bot) — dettaglio in [`hex-map-roadmap.md`](hex-map-roadmap.md) |
| **M6** Parità hex | 🟡 | **Codice completo** (CP 6.1–6.7 mergiati): la partita gira su esagoni — wiring, movimento, input, combat, scatto/spinta, bot, HUD. Resta il **playtest** CP 6.8 (sessione PIE) |
| **M7** Dismissione del quadrato | 🟡 | **Un solo substrato + release interna**: CP 7.1, 7.2 e 7.4 fatti (rimozione, packaging Development e Shipping avviati). Resta **CP 7.3**: 2 KPI su 4 misurati, FPS e preview richiedono rendering/editor |
| **M8** Presentazione e identità | 🟡 | **Le regole dei 4 eroi ci sono e le reazioni funzionano in partita** (E6: 25 test; CP 5.5 + CP 6.7 del 2026-08-07 hanno reso il motore componibile e cablato `Interposition`, `Deflection`, `ReactiveCapacitor`; `InterceptShot` e `FlowReaction` sono rinviate a **E14** e lo dichiarano nei dati). Resta la **presentazione**: personaggi animati, anelli team/selezione, **Ghost Timeline del planning** (`#172`/`#173`), leggibilità tattica — banco di prova: la showcase E15 |
| **M9** Ambienti tattici + editor maturo | 🟡 | **Terreni e stati** (E8: costi, Rough, Fire, ShallowWater, Smoke, Ice — 17 test; **CP 8.2 stati temporanei chiuso il 2026-08-07**, 11 test: durata legata alla cella, danno di `Burning` nel Cleanup prima dei KO, `Wet` che spegne le fiamme, `Marked` cablato). **CP 8.3 chiuso il 2026-08-07** (7 test: propagazione elettrica sul grafo dell'acqua, limite 3 passi, unicità per evento, ordine totale). **Epic E8 chiusa il 2026-08-07** (CP 8.1-8.5): terreni, stati, propagazione elettrica, terreno dinamico con fuoco/acqua, azioni ambientali e di supporto. **E9 aperta con CP 9.1 e CP 9.2 chiusi il 2026-08-07** (7 test, [`spec-copertura-cp91.md`](../gameplay/spec-copertura-cp91.md)): la copertura bassa esiste come dato **per bordo di cella**, il formato mappa è alla **v3** con migrazione verificata sulla serializzazione reale, e toglie 10 al danno diretto solo dal lato riparato. **CP 9.2** (10 test, [`spec-copertura-alta-cp92.md`](../gameplay/spec-copertura-alta-cp92.md)): la copertura **alta** nega vista, passo e proiettili sul bordo — nei due versi — e si abbatte, riaprendo LOS e grafo dalla fase successiva con la revisione incrementata. Restano **porte e ponti** (CP 9.3-9.4), le coperture **temporanee** con `CreateCover` (CP 9.5, rinviata da CP 8.5) e il residuo H5 dell'editor |
| **M10** Rete e privacy | ⏳ | Listen server, validazione server, planning team-only, canary intent leak. **Nuovo vincolo**: [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) introduce N round-trip per turno (finestre di reazione) |
| **M11** Production readiness | ⏳ | Budget in CI, validator commandlet, packaged soak, replay audit |

**Suite automatica**: `Source/RefactorTactics/Tests/` — **403** test unici in **63** file (misurati 2026-08-07
dopo il merge della riorganizzazione documentale; le due viste arrivavano al merge con 390 e 394, entrambi
corretti alla propria base e nessuno dei due valido dopo l'unione).
Storia del numero: **172** alla chiusura di CP 6.0 → **230** con M6+E1 → **171** dopo la rimozione del quadrato
(i **59** test previsti dall'inventario) → **324** con E4/E5/E6 e i terreni → **325** col merge di `#144` →
**338** con gli stati temporanei di CP 8.2 (`#65`) → **342** con i merge di `#179`/`#180` (misurato, mai
dichiarato: le due viste citavano ancora 338) → **347** con le reazioni componibili di CP 5.5 (`#154`) → **352** col cablaggio delle reazioni d'eroe di CP 6.7 (`#155`) → **359** con la propagazione elettrica di CP 8.3 (`#66`) → **362** col terreno dinamico di CP 8.4 (`#67`) → **366** con le azioni ambientali di CP 8.5 (`#68`, epic E8 chiusa) → **390** col primo blocco dell'harness degli scenari (`50159c6`).

Comando di misura, riproducibile:

```bash
grep -rhoE '"RefactorTactics\.[A-Za-z0-9_.]+"' Source/RefactorTactics/Tests/*.cpp | tr -d '"' | sort -u | wc -l
```

> **Correzione 2026-08-05**: questo documento dichiarava «169 test» mentre il CP 6.0, poche righe sotto,
> riportava già 172/172. Le due cifre convivevano: il conteggio reale era **172**.
>
> ⚠️ **Correzione 2026-08-07**: il documento dichiarava «179/179» — il conteggio reale è **324**, uno scarto di
> **145 test**. La causa è strutturale, non una svista: le epic **E4, E5, E6** sono state costruite e testate
> *senza* che questa vista di esecuzione venisse aggiornata, perché il lavoro era tracciato solo in
> [`roadmap-v0.1.md`](roadmap-v0.1.md). Con due viste sullo stesso lavoro, aggiornarne una sola è il modo in cui
> la deriva si crea. **D'ora in poi il conteggio si misura col comando qui sopra, non si cita a memoria.**

**Stato del gioco, in una riga** (2026-08-06): **un solo substrato, esagonale**. Il codice quadrato non esiste
più (`Grid/`, `Terrain/`, bot e resolver quadrati rimossi al CP 7.2; punto di ritorno: tag `pre-hex-only`).
Manca la prova sul campo: la sessione di playtest **CP 6.8**, che si esegue in editor.

---

## La release v0.1 (2026-08-05)

Le milestone qui sotto restano la vista di **esecuzione**. Sopra di esse esiste ora una vista di **release**:
[`roadmap-v0.1.md`](roadmap-v0.1.md) — **17 epic, 85 checkpoint** *(era 12/59; **E13** conoscenza parziale,
**E14** overwatch, **E15** showcase ed **E16** orientamento aggiunte il 2026-08-07, insieme ai checkpoint del
planning visuale in **E11** → [`brief-planning-visuale.md`](../technical/brief-planning-visuale.md))*, issue `#14`–`#85` e
`#151`–`#177` — che aggrega M6–M9 e
aggiunge il contenuto del catalogo v0.1 (4 eroi, ~35 azioni, reazioni, ambiente attivo, strutture, obiettivi
dinamici, comandi debug). La decisione abilitante è
[`adr-0003-modello-azioni-v01.md`](../decisions/adr-0003-modello-azioni-v01.md): **le macro-fasi restano quelle di Atlas**
(`Prep → Dash → Blast → Move`), mentre dal catalogo si adottano modello azioni, priorità intera intra-fase,
budget **5 MP**, reazioni, terreni e obiettivi.

| Epic v0.1 | Milestone | Relazione |
|---|---|---|
| **E1** Cataloghi e modello dati | — | nuova (il canone non prevedeva cataloghi versionati né validator) |
| **E2** Parità hex | **M6** | **identica**: CP 2.1–2.8 ≡ CP 6.1–6.8, stesso lavoro |
| **E3** Dismissione quadrato | **M7** | identica, meno il packaging (in E12) |
| **E4** Motore azioni · **E5** Reazioni | — | nuove, introdotte dall'ADR-0003 |
| **E6** Roster 4 eroi · **E11** HUD/debug | parte di **M8** | M8 copriva la presentazione; E6/E11 aggiungono regole e osservabilità |
| **E7** Equipaggiamento · **E10** Obiettivi | — | nuove |
| **E8** Terreni/ambiente · **E9** Strutture | **M9** | M9 con i valori del catalogo, anticipata nella v0.1 |
| **E12** QA e release | **M7** CP 7.3/7.4 + parte di **M11** | anticipa KPI e packaging; CI e soak restano a M11 |
| **E13** Conoscenza parziale (vista e udito) | parte di **M8**/**M10** | **nuova** (2026-08-07): la vista decide il targeting, il rumore è il secondo canale — [`brief-conoscenza-parziale.md`](../gameplay/brief-conoscenza-parziale.md) |
| **E14** Overwatch e reazioni interattive | — | **nuova** (2026-08-07): [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md); dipende da E13 |
| **E15** Showcase «Il Relè» e golden replay | parte di **M8** | **nuova** (2026-08-07): la prova integrata — fixture, scenario scriptabile, replay a hash stabile — [`showcase-v0.1.md`](../product/showcase-v0.1.md) |
| **E16** Orientamento e direzionalità | parte di **M8**/**M9** | **nuova** (2026-08-07): [ADR-0005](../decisions/adr-0005-orientamento.md); il facing deriva dal movimento e decide difesa, percezione e reazioni. **Prerequisito di E13** |
| **E17** Validazione di stress 4v4 | oltre **M9** | **nuova** (2026-08-07): [D-011](../decisions/RT_PDR_00_Decision_Log.md). **Misura, non produzione**: dove si rompe il sistema con otto unità. Dopo E15; **non** decide il formato principale |
| — | **M10** Rete e privacy | **fuori** dalla v0.1 |

### Stato misurato delle epic — 2026-08-07

Non si duplica qui: la tabella completa epic → evidenza è in [`roadmap-v0.1.md`](roadmap-v0.1.md) **§2.1**.
La conseguenza sulle milestone di *questa* vista è:

| Epic | Stato misurato | Effetto sulla milestone |
|---|---|---|
| E1, E4 | ✅ chiuse (9 + 52 test) | — |
| E5, E6 | ✅ **richiuse il 2026-08-07** (27 + 25 test verdi): **CP 5.5** rende il motore componibile, **CP 6.7** cabla tre reazioni d'eroe su cinque e dichiara nei dati il rinvio delle altre due a E14 | **M8** resta 🟡 solo per la **presentazione** (personaggi animati, anelli, Ghost Timeline): le regole degli eroi non hanno più abilità inerti |
| E8 | 🟡 parziale (17 test; mancano `Environment.*`) | **M9 passa a 🟡** |
| E10 | 🟡 parziale (18 test `Match.*`/`MatchFormat.*`): **CP 10.3 chiuso il 2026-08-07** — fine partita a tre vie e `RoundLimit` da `URTMatchFormatData`. Restano **CP 10.1** (`#74`) e **CP 10.2** (`#75`): senza di loro la via «obiettivo» ha il giudice ma non la fonte | l'epic **non** è chiusa |
| E7, E9, E11, E12 | ⏳ assenti (nessun test nelle rispettive aree) | M9 e M11 restano aperte |
| E13, E14, E15, E16 | ⏳ nuove (issue `#151`–`#177`) | E13/E14 preparano **M10**; E15 è il banco di leggibilità di **M8**; E16 dà peso tattico all'orientamento e **precede E13** |

> **Riapertura di E5 ed E6 (2026-08-07)**: `Bastion.Interposition`, `Vektor.Deflection`,
> `Flux.ReactiveCapacitor` e `Riva.FlowReaction` sono identità a catalogo con `Effects` vuoto, e
> `RTHeroCatalogLibrary.cpp` porta ancora commenti «arriva con E5» — mentre E5 è chiusa. Un motore che nessuno
> consuma non è collaudato: **CP 5.5** (`#154`) lo rende componibile e **CP 6.7** (`#155`) cabla gli eroi —
> ✅ **entrambi chiusi il 2026-08-07**, vedi [`spec-reazioni-componibili-cp55.md`](../gameplay/spec-reazioni-componibili-cp55.md).
> Restano fuori `Vektor.InterceptShot` e `Riva.FlowReaction`, rinviate a **E14** e dichiarate tali nei dati.
> `Vektor.InterceptShot` e `Riva.FlowReaction` appartengono invece a **E14** (trigger su movimento e movimento
> reattivo).

**Come leggere questa correzione**: M6 e M7 erano tracciate qui e sono accurate; M8 e M9 erano dichiarate ⏳
perché il lavoro di E4–E6 è stato eseguito con la vista di release, senza tornare su questa. Le due viste vanno
aggiornate **insieme** o la deriva si ripete.

Conseguenza pratica: **chi lavora su M6 sta lavorando su E2**. Le issue `#31`–`#38` sono i checkpoint 6.1–6.8;
si chiudono una volta, aggiornando entrambe le viste.

⚠️ **Allineamento 2026-08-07**: `#31` e `#32` (CP 6.1/6.2) erano **aperte a lavoro concluso** — mentre `#40`,
la rimozione del quadrato, era chiusa: non si rimuove un substrato se la partita non è già passata all'altro.
Chiuse con l'evidenza allegata; il residuo `PIE-HEXPLAY-1/4/5` resta dove è sempre stato, in `#38`.

⚠️ Il budget di movimento «4 celle / Dash 3» della tabella §6 del canone è **superato** dall'ADR-0003
(5 MP con costi interi per cella). L'allineamento del canone è il checkpoint **CP 1.1** (issue `#27`).

---

## M6 — Parità hex

**Obiettivo**: la partita 2v2 contro bot gira **interamente** su griglia esagonale, con parità funzionale
rispetto all'MVP quadrato. Nessuna feature nuova: è una **sostituzione del substrato**, non un'espansione.

**Perché prima di tutto il resto**: oggi esistono due mondi paralleli (il gioco quadrato e la simulazione hex
pura). Ogni giorno in più di convivenza costa doppia manutenzione e rende ambiguo dove va scritta una regola.

Lo strato puro esiste già ed è testato (`URTHexSimLibrary`, `URTHexPathLibrary`, `URTHexVisionLibrary`,
`URTHexBotLibrary`, `URTHexLibrary`): M6 **non riscrive le regole**, collega Actor/controller/HUD a quelle regole.

| CP | Obiettivo | Definition of Done | Verifica |
|---|---|---|---|
| **6.0** ✅ | Riordino dei contenuti | 11 asset sotto `/Game/RT` **feature-first**; percorsi hard-coded (`DefaultEngine.ini`, default `TSoftObjectPtr` in C++) e `.gitignore` aggiornati; nessun redirector residuo. Fatto **2026-08-05** via API Editor headless, suite **172/172** | Registro e insidie in [`convenzioni-contenuti-ue.md`](../technical/convenzioni-contenuti-ue.md) §A. ⏳ resta il PIE degli anelli (`PIE-AS5`/`PIE-SEL`) |
| **6.1** 🟡 | Allestimento della partita su mappa hex | `ARTGameMode` allestisce la partita da un `ARTHexMapActor` + `URTHexMapAsset`; `ARTUnit` ha la posizione autorevole in `FRTCellId` (**sostituzione**, non campo parallelo: due coordinate = due verità); l'occupazione è ricostruibile dallo stato unità | Codice **fatto** 2026-08-05: `FRTGridCoord` rimosso (35 file, tag `pre-cellid-swap`), `URTMatchSetupLibrary` pura, posizionamento via `URTHexLibrary`. Build Editor ✅, suite **180/0**. ⏳ resta `PIE-HEXPLAY-1`. Piano: [`cp6-1-hex-match-setup-plan.md`](plans/cp6-1-hex-match-setup-plan.md) |
| **6.2** 🟡 | Movimento end-to-end | `ARTTurnManager` costruisce `FRTHexSnapshot`, risolve con `ResolveHexPaths`, applica gli esiti e produce il TurnLog con `BuildMoveLog`; playback lungo i centri esagonali senza deriva | Codice **fatto** 2026-08-05: `GetHexContext` unica fonte di scala; 3 test d'integrazione in `UWorld` (cella raggiunta senza deriva, destinazione fuori mappa rifiutata, contesa con 2 `BlockedContested` nel TurnLog). Suite **183/0**. ⏳ restano `PIE-HEXPLAY-4/5`. **Perdita dichiarata**: cross-damage del terreno non portato (dipendeva da `URTTerrainData` quadrato) → epic **E8** |
| **6.3** 🟡 | Input, selezione, preview | Raycast → cella assiale del layer corretto; pianificazione a waypoint con rifiuto di celle oltre budget/bloccate/occupate; anteprima del percorso | Codice **fatto** 2026-08-05: `WorldToCellId`, `BuildCompositeHexPath` (budget cumulativo, rifiuto intero), `GetHexContext`/`FindInWorld` (unica fonte di scala, 4 duplicazioni rimosse), `MakeCurrentSnapshot` (il client valida sullo stato dell'autorità), anteprima a debug-line. **Riparata una regressione**: il controller cercava `ARTGridActor`, non più spawnato → movimento non pianificabile e LOS **fail-open**. Suite **192/0**. ⏳ restano `PIE-HEXPLAY-2/3`. Piano: [`cp6-3-hex-input-plan.md`](plans/cp6-3-hex-input-plan.md) |
| **6.4** 🟡 | Combat su hex | Attacchi, forme (Single/Area/Line/Cone via `HexLine`/`HexCone`), LOS via `URTHexVisionLibrary`, energia/ultimate, status Root/Slow/Reveal risolti su `FRTCellId`; combat resolver «raccogli poi applica» ordine-indipendente | Codice **fatto** 2026-08-06: libreria pura `URTHexCombatLibrary` (`HexHitCells` + `CollectHexAttacks` fail-closed senza mappa, piano in ordine canonico) e `ResolveCombat` che ne dipende; ordine delle unità reso stabile per cella. 13 nuovi test (4 forme, portata, LOS, no-fuoco-amico, permutazione, 3 d'integrazione in `UWorld`). Suite **205/0**. ⏳ restano `PIE-HEXPLAY-6/6b`. **Perdite dichiarate**: bonus altura e incendio del terreno **quadrato** non portati (→ epic **E8**); la **spinta** resta cardinale fino al CP 6.5 |
| **6.5** 🟡 | Dash e knockback su hex | Fase Dash attiva con budget esagonale; `KnockbackDestination` in versione esagonale (direzione fra celle adiacenti, spinte opposte che si annullano, contesa che resta ferma) | Codice **fatto** 2026-08-06: `ResolveDash` usa lo stesso strato puro del movimento (`MakeCurrentSnapshot` + `FindPathForUnit` col budget dello scatto + `ResolveHexPaths`), quindi anche le unità ferme bloccano; `URTHexCombatLibrary::HexKnockbackDestination` spinge lungo la **direzione esagonale** del colpo (fail-closed senza mappa, si ferma su bordo/ostacolo/unità, preserva il layer). Suite **213/0**. ⏳ restano `PIE-HEXPLAY-4b/6c` |
| **6.6** 🟡 | Bot su hex | `ARTTurnManager` pianifica i bot via `URTHexBotLibrary`; nessuna mossa illegale proponibile (le candidate nascono da `ReachableCells`); pesi utility restano `UPROPERTY` tunabili in PIE | Codice **fatto** 2026-08-06: `PlanBots` costruisce un pool unico di candidate (riposizionamento · attacco da fermo · scatto+attacco · scatto di riposizionamento) e lascia scegliere a `ChooseBestPlan`; l'attacco è valutato **solo** dalla cella in cui il bot sarà nel Blast. Guardie conservate (supporto, panico del kiter via nuova `BestKiteCell` pura). I 5 test d'integrazione del bot **quadrato** sono stati **portati** su hex, non cancellati. Suite **215/0**. ⏳ resta `PIE-HEXPLAY-7` |
| **6.7** 🟡 | HUD e osservabilità | Barre HP/scudo/energia, timer, fase, combat log e anteprima piani sui centri esagonali; i reason code del TurnLog compaiono nel log con coordinate assiali `(q,r,L)` | Codice **fatto** 2026-08-06: `ARTHUD` proietta ogni cella con `URTHexLibrary::AxialToWorld` dal contesto della mappa (traccia post-lock, anteprime, waypoint, scatto) e le rotte vengono dallo stesso A\* dell'autorità; nuova `URTTurnLogLibrary::DescribeEntry` (pura) porta i reason code nel combat log — quel che il giocatore legge e quel che il replay registra sono la **stessa** cosa. Sistemato anche `Home`: la camera ricentra sulla mappa esagonale (`URTHexMapAsset::GetCenterCell`), non più sulla griglia quadrata inesistente. Suite **217/0**. ⏳ resta `PIE-HEXPLAY-9` |
| **6.8** 🟡 | Playtest della partita hex | Mappa di prova costruita con l'editor mode (esagono r=4, ostacoli, celle che bloccano la vista, superficie costosa, piattaforma su layer 1 con una transizione); partita completa fino alla vittoria Parte **headless fatta** 2026-08-06: `RefactorTactics.HexMatch.PlaysToCompletion` gioca un 2v2 bot-vs-bot completo (invarianti per turno: nessuna sovrapposizione, nessuna cella fuori mappa) e la partita **si decide al turno 25** — dato che ha aperto la issue di bilanciamento `#96` (il catalogo prevedeva 12 turni; dopo il fix la partita chiude al **round 10**, dentro la banda 10–14 del formato 2v2 — [`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md) §6). Resta la **sessione D**: `PIE-HEXPLAY-1..9` in editor |

**DoD di milestone**: le 9 voci `PIE-HEXPLAY` verdi · suite automatica verde con i nuovi test hex · una partita
2v2 completa dall'avvio alla vittoria su mappa esagonale multilivello · nessun percorso di gioco che passi
ancora da `FRTGridCoord` (il tipo può esistere, ma non nel flusso della partita) · packaging Development che
si avvia e gioca senza editor.

**Rischi**: (a) la sostituzione della coordinata su `ARTUnit` tocca 34 file — va fatta a fette compilabili, non
in un commit unico; (b) il knockback esagonale non ha un equivalente 1:1 del quadrato (6 direzioni invece di 8):
è l'unico punto dove serve una **decisione di design**, non una traduzione.

---

## M7 — Dismissione del quadrato

**Obiettivo**: un solo substrato in repo. Il quadrato smette di esistere nel gameplay, con un punto di ritorno
esplicito prima della rimozione.

| CP | Obiettivo | Definition of Done | Verifica |
|---|---|---|---|
| **7.1** 🟡 | Punto di ritorno + inventario | Tag git annotato (es. `pre-hex-only`) sull'ultimo commit con entrambi i substrati; classificazione dei test non-hex in **neutri**, **da portare**, **da rimuovere** | Fatto **2026-08-06**: tag annotato `pre-hex-only` creato **in locale** (il push richiede richiesta esplicita, come dice la issue `#39`); classificazione dei **123** test non-hex nella tabella qui sotto |
| **7.2** ✅ | Rimozione del gameplay quadrato | Via `Grid/RTGridActor`, `Grid/RTGridLibrary`, resolver/bot/terreno quadrati e i test relativi; ciò che è neutro resta e continua a girare | Fatto **2026-08-06**: rimosse le cartelle `Grid/` e `Terrain/`, `Bot/RTBotLibrary`, `Turn/RTMovementResolver`, `URTCombatLibrary::KnockbackDestination` e 59 test. `DirectionYaw` **portata** in `URTPlaybackLibrary` (lavora su `FVector`: è presentazione, non griglia). Suite **171/171** — esattamente il numero dichiarato al CP 7.1. Build Editor e Game verdi; il gioco avviato headless gira identico |
| **7.3** 🟡 | Misurazione dei budget | KPI del PDR misurati **una volta su hex** e registrati: FPS client, path mediana, tempo resolver per turno. Un numero misurato, anche fuori target, vale più di un ⏳ | Fatto **2026-08-06** per i due misurabili headless: **path 0,025 ms** e **resolver 0,41 ms/turno** (test `RefactorTactics.Perf.*`, due ordini di grandezza sotto i target). FPS client e preview restano ⏳: richiedono rendering ed editor |
| **7.4** ✅ | Release interna hex | Packaging Windows Development **e** Shipping dal codice solo-hex; partita completa senza editor | Fatto **2026-08-06**: `RunUAT BuildCookRun` **BUILD SUCCESSFUL** per Development e Shipping; l'eseguibile impacchettato **avviato e verificato** (arena, 2v2, turni che si susseguono). Il primo tentativo ha rivelato che i materiali referenziati via `TSoftObjectPtr` **non venivano cookati** (unità grigie): risolto con `+DirectoriesToAlwaysCook=(Path="/Game/RT")` in `DefaultGame.ini` |

### Inventario dei test prima della rimozione (CP 7.1, 2026-08-06)

Suite **230 test**: **107 esagonali** + **123 non-hex**. I 123 si dividono così:

| Categoria | Test | File | Destino |
|---|---:|---|---|
| **Neutri** — non conoscono la topologia | **63** | `RTCombatLibraryTests` (14/15), `RTCombatResolverTests`, `RTTurnLogSerializationTests`, `RTTurnLogLibraryTests`, `RTTurnRulesTests`, `RTPlaybackLibraryTests`, `RTPlaybackPhaseTests`, `RTUnitTests`, `RTUnitPlacementTests`, `RTMatchSetup*`, `RTCatalogTests` | **restano** |
| **Da portare** — logica valida, casa sbagliata | **1** | `RTGridFacingTests` (`DirectionYaw`: lavora su `FVector`, ma vive in `URTGridLibrary` ed è usata da `ARTUnit`) | sposta la funzione fuori da `Grid/`, poi il test resta |
| **Da rimuovere** — vivono del substrato quadrato | **59** | `RTGridTests` (19), `RTBotUtilityTests` (13), `RTBotLibraryTests` (12), `RTMovementResolverTests` (8), `RTGridElevationTests` (3), `RTTurnLogTests` (2), `RTTerrainTests` (1), `RTCombatLibraryTests::Knockback…` (1) | via con `Grid/`, `Bot/RTBotLibrary`, `Turn/RTMovementResolver`, `Terrain/` |

**Conteggio dichiarato in anticipo**: dopo il CP 7.2 la suite deve avere **171 test** (230 − 59). Un numero
diverso significa che la rimozione ha portato via qualcosa che non doveva, o ne ha lasciato indietro.

**Punto di ritorno**: tag annotato **`pre-hex-only`** sul commit `cad2693` (l'ultimo con entrambi i substrati).
Esiste **in locale**: il push va chiesto, come prescrive la issue `#39`.

**DoD di milestone**: un solo substrato · suite verde · budget misurati e registrati · build packaged giocabile ·
`hex-map-roadmap.md` e `piano-canonico-mvp.md` allineati (nessun riferimento al quadrato come sistema vivo).

---

## M8 — Presentazione e identità

**Obiettivo**: il gioco smette di essere cilindri colorati. Il C++ è già in `main` (spawn `TSubclassOf` con
fallback, facing, eventi di montaggio, anello di team); manca il lavoro **in editor**.

| CP | Obiettivo | Definition of Done | Verifica |
|---|---|---|---|
| **8.1** | Personaggi su hex | `BP_Unit_*` (Paragon) posati sui centri esagonali, a terra, senza compenetrazione; fallback al cilindro se l'asset manca | `PIE-AS2`, `PIE-FACING` |
| **8.2** | Animazioni | `ABP_*` con locomozione Idle↔Run in fase Move; montaggi Cast/Hit/Death nel Blast; la morte visiva resta differita (la presentazione non decide, invariante #1) | `PIE-AS4a`, `PIE-AS4b` |
| **8.3** | Leggibilità tattica | `M_TeamRing` + `M_SelectionRing` assegnati; colori delle superfici leggibili in partita (non solo nell'overlay dell'editor); camera tarata (pitch/distanza) sulla scala esagonale | `PIE-AS5`, `PIE-SEL` + giudizio a schermo |

**DoD di milestone**: sessione C di `test-manuali-pie.md` completamente verde · nessun cilindro nel gioco a
meno di asset mancanti · una partita registrata (video/screenshot) come riferimento di stato.

---

## M9 — Ambienti tattici e maturità dell'editor

**Obiettivo**: la mappa diventa un sistema di gioco (pilastro di prodotto), non solo un piano da percorrere.
Assorbe **H8** e il residuo di **H5**.

| CP | Obiettivo | Definition of Done |
|---|---|---|
| **9.1** | Residuo editor mappa (H5) | Verifiche PIE aperte dell'editor mode chiuse (`E/F/G/H/L/N`); copia-incolla di regioni e palette Slate **solo se** l'uso reale le richiede (YAGNI: la mappa di prova di M6 è il banco di prova) |
| **9.2** | Superfici attive | Acqua/fuoco/elettricità con effetto sul turno (costo, hazard di fine turno, propagazione deterministica); ogni modifica ambientale compare nel TurnLog |
| **9.3** | Cover dinamica e passaggi | Coperture distruttibili/mobili e porte-ponti che cambiano la topologia: le modifiche **invalidano** cache e path (revisione dell'asset), mai path fantasma |

**DoD di milestone**: un incremento ambientale cambia in modo osservabile l'esito di un turno · nessuna cache
stantia (test di invalidazione) · le regole ambientali sono coperte da test puri.

> **Scala delle mappe (2026-08-07, D-010)**: le mappe di M9 **non** sono vincolate alla compattezza di Atlas
> Reactor. Il principio è *«compatto nel tempo, non necessariamente piccolo nello spazio»* e la metrica è
> **temporale**: quanti Move servono per raggiungere una zona rilevante. Classi iniziali — **Skirmish**
> (~3–4 Move di attraversamento, primo contatto ~1 round) per demo e vertical slice · **Standard**
> (~5–7 Move, contatto in 1–2 round, 2–3 macro-rotte, choke point **e** alternative) per il formato 3v3 ·
> **Operations** fuori scope. Fog of War, rumore, stealth, flank e Overwatch **degradano tutti insieme** su
> mappe troppo piccole: è il motivo per cui la scala è una decisione di sistema e non di gusto.
> Dettaglio e vincoli di level design: [`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md)
> §3–§4, §14.

---

## M10 — Rete e privacy

**Obiettivo**: dal 2v2 offline al 2v2 in rete, senza cedere sull'invariante #6 (privacy dell'intento).
Corrisponde a **H7** e alla fase **F1** del PDR.

| CP | Obiettivo | Definition of Done |
|---|---|---|
| **10.1** | Listen server + autorità | Ogni decisione di gameplay è calcolata sul server; il client produce solo preview; hash dell'asset mappa validato all'ingresso |
| **10.2** | Piani team-only | I piani viaggiano in DTO filtrati per squadra: nessuna replica globale con occultamento grafico |
| **10.3** | Canary anti-leak | Test automatico che fallisce se un client riceve **qualunque** byte del piano avversario prima del reveal |

**DoD di milestone**: **intent leak = 0** dimostrato dal canary · il server rifiuta ogni percorso illegale
proposto dal client (test) · una partita in rete completata senza desync (replay divergence 0).

> ⚠️ **Rischio accettato**: il PDR ordina le fasi *per rischio* e metterebbe la rete subito dopo le fondamenta.
> Posticiparla a M10 significa che M8 e M9 aggiungono superficie da rendere autoritativa. **Mitigazione**: ogni
> PR di M8/M9 mantiene la logica decisionale in `ARTTurnManager`/librerie pure (invariante #5) — le
> presentazioni non accedono a stato che il server non abbia già deciso.

---

## M11 — Production readiness

**Obiettivo**: quello che serve per considerare il gioco distribuibile a persone esterne. Corrisponde a **H9**
e alle fasi **F5–F6** del PDR.

Contenuto: chunking e performance su mappe grandi · validator della mappa come **commandlet in CI** · soak test
su build packaged · replay audit (registrazione e riesecuzione di partite reali) · checklist di rilascio ·
accessibilità di base. **DoD**: i budget KPI rispettati (o le deviazioni registrate) su mappa grande, validator
che blocca la CI su mappa non valida, soak test senza crash.

---

## KPI / Performance budget

| Budget | Target | Stato |
|---|---|---|
| Client FPS | 60 | ⏳ non misurato → **CP 7.3** (richiede il rendering: non misurabile in `-nullrhi`) |
| Path (mediana) | < 2 ms | ✅ **0,025 ms** (2026-08-06, headless, A* su mappa r=12 = 469 celle, mediana di 200 percorsi da bordo a bordo) |
| Preview completa | < 50 ms | ⏳ non misurato → **CP 7.3** (richiede l'editor: la preview è disegnata dalla HUD) |
| Resolver | < 100 ms/turno | ✅ **0,41 ms/turno** (2026-08-06, headless, 2v2 su mappa r=6, playback escluso: è presentazione) |
| Intent updates | 8–12 Hz | ⏳ con M10 |
| **Replay divergence** | **0** | ✅ determinismo by-design; TurnLog permutazione-invariante, hash di replay, serializzazione versionata con checksum, verificato **anche su hex** (`RefactorTactics.HexSim.ReplayDivergenceZero`) |
| **Intent leak** | **0** | ⏳ canary con M10 — privacy già invariante #6, oggi banale perché offline |
| **Round per partita** (2v2) | 10–14 (cap 14–16) | 🟡 **10** misurato bot-vs-bot (2026-08-06, `HexMatch.PlaysToCompletion`) — dentro banda, ma è un bot contro un bot |
| **Durata match P50 / P90** | 25–30 min / < 40–45 min *(3v3 Standard)* | ⏳ **il 3v3 non esiste**: in v0.1 si misura la banda 2v2 e si registra come tale |
| **Playback per round** | 8–15 s (2v2) | ⏳ con la sonda di pacing (`MsPlayback`) |

> I tre budget nuovi vengono da **D-010** / [`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md).
> Sono **target di game design**, non budget di macchina: si registrano anche fuori target, e con la riserva sul
> campione (un solo giocatore, che è l'autore del gioco).

## Rischi aperti

| Rischio | P/I | Mitigazione | Stato |
|---|---|---|---|
| La sostituzione della coordinata rompe il gioco a metà | H/H | M6 a fette compilabili, ogni CP con suite verde; tag di ritorno prima di M7 | attivo |
| Doppia manutenzione quadrato/hex | H/M | M7 con data: la dismissione è una milestone, non un'intenzione | pianificato |
| Rete introdotta tardi su superficie ampia | M/H | Autorità isolata come gate di PR (invariante #5) | accettato, monitorato |
| Budget mai misurati → target mitici | M/M | CP 7.3 forza una misura reale | pianificato |
| Verifiche PIE che si accumulano | M/M | Raggruppate in sessioni A–D; ogni milestone chiude le proprie voci | attivo |
| Scope roster/ambienti | H/M | 2 archetipi (Ranger/Guardian) finché il loop non è chiuso | attivo |
| Upgrade UE dentro una milestone | M/H | UE 5.8.1 bloccata (canone), upgrade solo fra milestone | ✅ |

## Definition of Done trasversale (per ogni PR)

1. Compila (Game + Editor). 2. Suite automatica verde, con test nuovi per la logica nuova. 3. Le decisioni di
gameplay restano in C++ autoritativo (invarianti #1/#5). 4. Determinismo preservato: nessun float in
coordinate/hash, nessuna dipendenza dall'ordine dei container. 5. Log/TurnLog spiegano l'esito. 6. Verifiche
interattive registrate in `test-manuali-pie.md` (non dichiarate verdi senza esecuzione). 7. Documentazione
aggiornata, commit focalizzato, nessun file generato o segreto. **Se qualcosa non è verificabile, si dichiara.**

---

## Archivio — MVP quadrato M0–M5 (fase tutorial, chiusa)

Storico compresso. **Non si riapre**: le voci residue ⏳ sono superate dal pivot esagonale o assorbite dalle
milestone sopra. Il dettaglio dei checkpoint originali resta nella storia git di questo file (fino a `8974e46`).

| Milestone | Esito |
|---|---|
| **M0** Fondamenta | Progetto UE 5.8.1 nella radice del repo, compila Game + Editor, Git LFS |
| **M1** Sandbox | Camera tattica, Enhanced Input **in C++** (niente asset `IA_*`/`IMC_*`), griglia logica + ISM, selezione via `IRTSelectable` |
| **M2** Turn loop | `ERTMatchPhase`, timer 30 s + lock-in, pianificazione, risoluzione movimento ordine-indipendente |
| **M3** Combat loop | Abilità data-driven (`URTAbilityData`, no GAS), danno/scudo/morte, energia/ultimate, status Root/Slow/Reveal, forme Single/Area/Line/Cone, LOS/copertura |
| **M4** Vertical slice | Bot utility scoring (focus-fire, kiting, panic, support, dash+attacco), HUD + combat log, vittoria e riavvio |
| **M5** Release interna | Suite verde, packaging Windows Development **e** Shipping, DoD MVP rivista voce per voce |

**Incrementi post-MVP consegnati** (tutti su `main`): path finding PF.1–PF.3 (BFS → Dijkstra pesato) e **PF.4**
grafo multilivello con mappa "ponte sopraelevato" · terreno v1 (5 tipi, hazard, fuoco dinamico) · movimento v2
(microstep sincroni, path a waypoint editabile) · animazione della risoluzione AN.1–AN.6 (round osservabile,
morte visiva differita, skip) · Dash come fase attiva · knockback deterministico · **TurnLog + reason codes**
con hash di replay, serializzazione versionata e I/O su file con checksum · personaggi Paragon (C++).

**Decisioni che divergevano dai DoD originali** e restano valide: input in C++, selezione via interfaccia C++,
colore team via materiale parametrico, attacco base su `ARTUnit` prima delle abilità data-driven.

**Cosa resta utile del quadrato**: il comportamento di riferimento per la traduzione su hex (M6) e i test
neutri (combat math, serializzazione, regole di fase). Il resto ha data di scadenza in **M7**.

---

## Rapporto con gli altri documenti

| Documento | Ruolo |
|---|---|
| [`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) | **Canone**: decisioni vincolanti, invarianti, regole numeriche |
| [`roadmap-v0.1.md`](roadmap-v0.1.md) | **Release v0.1**: 14 epic, 69 checkpoint, mappatura con queste milestone + **§2.1 stato misurato** |
| [`v0.1-definition-of-done.md`](v0.1-definition-of-done.md) | Gate di release `G1`–`G14`, KPI, checklist di contenuto |
| [`balance/`](../balance) | **Numeri vigenti v0.1**: cataloghi azioni, terreni, equipaggiamento, eroi, matrice di test |
| [`spec-motore-azioni-e4.md`](../gameplay/spec-motore-azioni-e4.md) | **Proposta di design** del motore azioni (epic E4): modello, fette, rischi, domande aperte |
| [`spec-stati-temporanei-cp82.md`](../gameplay/spec-stati-temporanei-cp82.md) | **Stati temporanei** (CP 8.2): durata legata alla cella, ordine del Cleanup, decisioni e difetti di cablaggio trovati |
| [`spec-fuoco-acqua-cp84.md`](../gameplay/spec-fuoco-acqua-cp84.md) | **Terreno dinamico** (CP 8.4): dove vive la superficie corrente, copia di lavoro della mappa, ordine del Cleanup, cosa brucia |
| [`spec-propagazione-elettrica-cp83.md`](../gameplay/spec-propagazione-elettrica-cp83.md) | **Propagazione elettrica** (CP 8.3): grafo dell'acqua, ordine nel Cleanup, `Action.Electrify`, limiti dichiarati |
| [`spec-reazioni-componibili-cp55.md`](../gameplay/spec-reazioni-componibili-cp55.md) | **Reazioni componibili** (CP 5.5): più effetti per reazione, riduzione danno come dato, identità nel TurnLog (formato v3), helper per le reazioni d'eroe |
| [`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md) | **Durata della partita, budget del round e scala delle mappe**: target per formato, `RoundLimit` parametrico, classi di mappa (Skirmish/Standard/Operations), telemetria. Supera D-002 del Decision Log |
| [`spec-pacing-turno.md`](../gameplay/spec-pacing-turno.md) | **Tempo di un turno** misurato sul giocatore: sonda, percentili, regola di taratura di `PlanningSeconds` |
| [`brief-delayed-actions.md`](../gameplay/brief-delayed-actions.md) | **Brief di scope** delle Delayed Actions e dei boundary di fase: cosa aggiungono oltre ad ADR-0004, dati, test minimi. Nessuna epic aperta |
| [`v0.1-issue-plan.md`](v0.1-issue-plan.md) | Titoli e body delle 72 issue (`#14`–`#85`) e ordine di apertura dei branch |
| [`adr-0003-modello-azioni-v01.md`](../decisions/adr-0003-modello-azioni-v01.md) | Modello azioni del catalogo v0.1 sulle macro-fasi di Atlas |
| [`convenzioni-contenuti-ue.md`](../technical/convenzioni-contenuti-ue.md) | **Normativo**: struttura di `Content/`, naming, dipendenze fra cartelle, spostamenti |
| *questo file* | **Esecuzione**: milestone, checkpoint, DoD, stato |
| [`roadmap-editor.md`](roadmap-editor.md) | **Operativo in editor**: sedute di authoring e verifica (U1–U17), ordine e dipendenze verso i checkpoint |
| [`hex-map-roadmap.md`](hex-map-roadmap.md) | **Dettaglio tecnico** della linea esagonale H0–H6.5 (consegnate) e del residuo editor H5 |
| [`../PDR/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md`](RT_PDR_10_Roadmap_QA_Rischi_v0.2.md) | **Requisiti** di lungo periodo (fasi F0–F6, QA, rischi) — direzione, non scope |
| [`test-manuali-pie.md`](../technical/test-manuali-pie.md) | Verifiche interattive in editor, per sessioni |
| ADR ([`adr-0002-griglia-esagonale.md`](../decisions/adr-0002-griglia-esagonale.md)) | Decisioni architetturali con motivazione e revisione |

**Mappatura con le fasi PDR**: M6+M7 ≈ **F0** consolidata su hex · M8 ≈ parte di **F4** · M9 ≈ **F3**+**F2**
(ambienti, kit) · M10 ≈ **F1** · M11 ≈ **F5–F6**. Le divergenze consolidate restano due, e prevale il canone:
**rete differita** (il PDR la anticipa) e **no-GAS** (il PDR prevede il mirror GAS in F2).
