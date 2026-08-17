# Decisioni aperte

> `OPEN` · **Stato**: vivo · **Ultimo aggiornamento**: 2026-08-17
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

## ✅ Chiuse il 2026-08-13 da `D-137` — la versione di formato che non viaggiava

Origine: [`#687`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/687) e
[`roadmap/plans/mappe-generate-o-dipinte-2026-08-12.md`](roadmap/plans/mappe-generate-o-dipinte-2026-08-12.md) §6.

Il [referto del 2026-08-12](archive/roadmap-plans/roadmap-reconciliation-2026-08-12.md) §5 aveva **rinviato**
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

> 🔴 **Una terza affermazione di quel piano è stata verificata e respinta.** La sua §5 registra come debito
> che *«`MakeArenaV01` va nel registry `MakeFixtureArena`: oggi non c'è, quindi nessuno scenario può riferirla
> per nome»*. **C'è.** `Source/RefactorTactics/Turn/RTMatchSetupLibrary.cpp:322-325` risolve
> `FixtureId == "ArenaV01"` su `MakeArenaV01`, ed è entrata con `115f59a3` il **2026-08-12 alle 03:03** —
> cioè **quindici ore prima** che il piano che la denuncia venisse committato (`0485a7b6`, ore 18:52).
>
> Il debito era già chiuso quando è stato scritto. Vale la pena registrarlo perché la tentazione era di
> ricopiarlo qui: un piano è una fonte **interna**, e la regola «rimisura, non trascrivere» non fa sconti
> alle fonti di casa — è dove costa di più, perché nessuno le sospetta.

| ID | Domanda | Esito, e l'istruttoria che ci è arrivata sotto |
|---|---|---|
| ~~`FMT-1`~~ | ~~**Quale delle quattro direzioni di `#687` si prende per far viaggiare `FormatVersion`?**~~ | ✅ **Chiusa il 2026-08-13** — [D-137](decisions/RT_PDR_00_Decision_Log.md), decisa dall'autore su panel `/sc:spec-panel`: **`FCustomVersionRegistry`**. Il lavoro resta di [`#687`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/687). **L'istruttoria che l'ha prodotta**: la domanda era mal posta, e lo si vede misurando. Le quattro direzioni chiedevano tutte *«come faccio viaggiare il numero»*, ma il difetto è di **regime**: il TurnLog è un formato scritto a mano dove la versione è il primo `uint16` del binario e viaggia per costruzione (`ERTTurnLogFormatVersion`, alla v7); `URTHexMapAsset` vive nella serializzazione **UObject** e ne aveva preso il vocabolario — un numero di versione, `MigrateToCurrentFormat`, una storia v1–v8 — senza il meccanismo che lo regge. La causa precisa è `int32 FormatVersion = CurrentFormatVersion` (`RTHexMapAsset.h:74`): un default **mobile**. 🔴 **Una delle quattro non funziona**, ed è stata esclusa **prima** del voto invece che scartata dopo: `SaveGame` è un flag che `FArchive` consulta quando `ArIsSaveGame` è vero, ortogonale al confronto col CDO che produce il delta di package. ⚠️ **E la scorciatoia apparente peggiora le cose**: un default fisso (`= 1`) senza un lato-scrittura che timbri la versione corrente fa ripartire la migrazione **a ogni caricamento** — innocuo finché i passi sono no-op, e il giorno di un passo trasformativo la **riapplica ogni volta**. `URTMatchFormatData` ha già quel default (`RTMatchFormatData.h:81`) e **non è un controesempio**: la sua versione corrente *è* 1, quindi il valore coincide col default e non viaggia neanche lui — stesso difetto, più giovane. ✅ **La scadenza è misurata**: tutti e sette i passi v1→v8 sono dichiarativi — i commenti di `MigrateToCurrentFormat` lo scrivono per ognuno — quindi oggi migrare da 1 o da 6 dà lo stesso risultato e il cambio ha **rischio dati zero**. È il momento più economico che il progetto avrà. ⚠️ **`MapClass` e `HexSize` sono usciti da questa voce**: non sono metadati di versione ma **dati di gioco**, e il loro problema non è *non viaggiano* ma *cambiano significato se il default cambia* — si curano pinnandone i default con un test ([`#830`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/830)), non serializzandoli |
| ~~`FMT-2`~~ | ~~**Quante mappe si committano: una che faccia da rivelatore, o entrambi i `DA_HexMap_*`?**~~ | ✅ **Chiusa il 2026-08-13 per derivazione**, e non da una decisione propria: il suo stesso testo diceva *«non si deduce perché la risposta dipende da `FMT-1`»*, e con `FMT-1` chiusa la condizione è **scattata**. La regola che aveva già scritto: *«se `FormatVersion` viaggia, un rivelatore basta e il resto è codice; se non viaggia, nemmeno quello rivela nulla»*. Con `D-137` viaggia → **uno basta**, ed **esiste già**: `DA_HexMap_Arena`, committato l'11 agosto quando la versione corrente era 6, è l'unico asset mappa con contenuto reale del repository. ⚠️ Dopo `#687` acquista una proprietà che oggi non ha: essendo stato scritto **prima** del meccanismo non avrà voce nel registro delle custom version, cioè sarà `legacy` in modo **non ambiguo** — che è esattamente ciò che un rivelatore deve dimostrare. Committarne un secondo non aggiungerebbe una prova: aggiungerebbe una copia. ⚠️ Restano validi i numeri misurati qui: `.gitignore` ha **18** righe `!Content`, di cui **5** di mappa — tre livelli `.umap` e **due** data asset |
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
| **A. Super Actions** | «Epic da creare post-v0.1» | `RT-FEAT-ACTION-SUPERS` — v0.2, P2, **`IMPLEMENTING`**, **sei** gate `partial` | **owner esiste**, ed è già in lavorazione |
| **B. Modular Effects + Presentation/VFX** | pipeline da proporre | la **catena esiste già quasi tutta**: `FRTActionEffectSpec` a monte, `FRTResolvedEvent` in mezzo (owner `RT-FEAT-CORE-PLAYBACK`, **`INTEGRATED`** in v0.1), `RT-FEAT-CHAR-PRESENTATION` a valle. Manca **solo** il legame dichiarativo *evento → asset VFX/SFX*: di Niagara nel registry non c'è nulla | **gap reale ma stretto** → `FX-1` |
| **C. Seeded Map Generation** | prototipo v0.3 | nessuna feature di generazione. `RT-FEAT-TOOL-MAP-GEOMETRY` e `RT-FEAT-TOOL-MAP-EDITOR` sono **authoring**, non generazione da seed | **gap reale** → `GEN-1` |
| **D. Production Map Generator / Level Designer** | v0.5 da proporre | `RT-FEAT-TOOL-MAP-EDITOR` è **`INTEGRATED`** in v0.1 | **coperto per la parte di authoring**; il resto è il seguito di `GEN-1`, non un'area propria |
| **E. Networking / Dedicated server** | proposta v0.6 | **tre** feature, e **non** sono tutte future: `RT-FEAT-NET-PRIVATE-PLANNING` è **`TESTABLE` in v0.1**, `RT-FEAT-NET-AUTHORITY` è `SPECIFIED`, solo `RT-FEAT-NET-DEDICATED` è `IDEA`/`future` | **owner esiste**, e un terzo è **già in v0.1** |

La lezione è la stessa di `REP-1` e di `MSE-1`: una proposta si registra **dopo** aver cercato chi
possiede già l'area, non prima. Le tre righe «owner esiste» restano qui apposta — il prossimo kit le
riproporrà, e trovarle già risposte costa meno che ridiscuterle.

| ID | Domanda | Perché non si deduce |
|---|---|---|
| `FX-1` | Il legame *evento risolto → asset VFX/SFX* è un **dato dichiarativo** — una tabella che l'artista riempie — o resta codice in Blueprint, un ramo per tipo di evento? | 🔴 **La domanda è stata riscritta il 2026-08-13: la sua prima stesura era falsa.** Diceva che «nessun documento nomina l'anello centrale», e l'anello **esiste**: `Source/RefactorTactics/Turn/RTResolvedEvent.h` dichiara `ERTResolvedEventType { Move, Attack, HazardDamage, Defeated }` e `FRTResolvedEvent`, col commento che scrive già l'invariante — *«L'animazione legge questi eventi: non decide nulla (invariante #1)»*. Owner: **`RT-FEAT-CORE-PLAYBACK`**, `INTEGRATED` in v0.1, con spec `gameplay/spec-anima-risoluzione.md` §4 che lo chiama *timeline di eventi risolti*. La ricerca si era fermata all'area `Characters` senza guardare `Core` — **lo stesso errore** che la tabella qui sopra rimprovera al sorgente, commesso mentre lo si rimproverava. Cade con essa anche «la presentazione della v0.1 è ferma a mesh e anelli». ✅ Quel che **resta** davvero aperto è più stretto e non ha owner: di `Niagara`, `VFX` o `SFX` nel registry non c'è **una riga**, e il piano di `AN.4` si limita a nominare *«Delegate BP (camera/VFX/SFX)»* — cioè la scelta «dato o codice» non è stata fatta, è stata rinviata a un delegate. Innesco: `#288` (locomotion/cast/hit/death), primo consumatore reale |
| `FX-2` | Quando `ERTResolvedEventType` cresce oltre i suoi **quattro** valori, chi si accorge che l'evento nuovo **non ha presentazione**? | ⚠️ **Separata da `FX-1` il 2026-08-13**: erano una voce sola, e una voce con due domande si chiude a metà senza che si veda. Sono legate ma non identiche — `FX-1` sceglie la **forma** del legame, `FX-2` chiede chi **sorveglia** che resti completo. Oggi la risposta è *nessuno*: `ERTResolvedEventType` non ha un test di esaustività, e i quattro valori (`Move`, `Attack`, `HazardDamage`, `Defeated`) sono consumati da delegate Blueprint che il C++ non conta. Un quinto valore compilerebbe, risolverebbe correttamente la logica, e **sparirebbe a schermo** — il fallimento più silenzioso possibile, perché il gioco resta giusto e solo la presentazione mente per omissione. ⚠️ Se `FX-1` sceglie il **dato**, questa si chiude quasi da sé — una tabella dichiarativa può avere un gate di copertura, com'è già per le icone (`FindMissingRequiredIcons`). Se sceglie il **ramo in Blueprint**, resta aperta e serve un test apposta. Innesco: il primo `ERTResolvedEventType` aggiunto dopo i quattro |
| `GEN-1` | La relazione **numero di celle → `RoundLimit`** è una formula canonica o una taratura da misurare? E un generatore da seed deve **validare** connettività, macro-rotte, choke, raggiungibilità degli obiettivi, densità di copertura, alture, transizioni e equità degli spawn — o è il playtest a dirlo? | Il sorgente stesso marca la relazione come *«ipotesi da misurare, non formula canonica»*, ed è la parte che va conservata: è una **taratura**, e questo file ha una sezione apposta per le tarature. Non si deduce perché il generatore non esiste e non c'è nulla da misurare: `RT-FEAT-TOOL-MAP-GEOMETRY` e `RT-FEAT-TOOL-MAP-EDITOR` producono geometria **d'autore**, con un umano che decide. ⚠️ Il vincolo che sopravvive alla proposta è di **formato**, non di algoritmo, e va rispettato dal primo giorno: un generatore deve riusare i dati canonici del filone editor (`#619`–`#623`, `#695`) — **niente secondo formato di mappa**. È lo stesso vincolo che `MSE-1` sta perimetrando dal lato della cottura, ed è il motivo per cui `GEN-1` non può essere decisa senza guardarla. Innesco: v0.3, o la prima mappa non disegnata a mano |

---

## Aperte — due contraddizioni fra campi, dal consolidamento roadmap→v1.0 del 2026-08-13

Origine: [`archive/src/RefactorTactics_Claude_Consolidamento_Roadmap_v1_0_2026-08-13.md`](archive/src/RefactorTactics_Claude_Consolidamento_Roadmap_v1_0_2026-08-13.md)
§12, e [D-136](decisions/RT_PDR_00_Decision_Log.md).

Il sorgente chiedeva di riconciliare «feature v0.2 senza owner/epic chiara» e i «conflitti fra
CHARACTER-STATE / CHAR-TRANSFORMATION / E34 e release v0.2/v0.4». Misurando, **la maggior parte non era
un conflitto**: era un campo che il registry non sapeva scrivere, e `D-136` l'ha reso scrivibile — 19
feature hanno preso l'epic che il loro owner o le loro `notes` **già dichiaravano**, senza che nessuno
scegliesse niente.

Restano **due** casi, e sono di natura diversa da quello: non manca un campo, due campi si contraddicono.
Nessuno dei due si chiude leggendo — si chiudono decidendo quale dei due mente.

🔴 **Non erano invisibili per caso.** Entrambi vivevano dove nessun gate guardava: `REL-1` era
*renderizzato* nella tabella §2.2 della v0.1 come se fosse una feature della v0.1, e `REL-2` stava in una
nota della shortlist che si concludeva con *«finché l'audit non dice se sono due scope o un duplicato,
**nessuna delle due si sposta**»*. Questo è quell'audit, e per `REL-2` la risposta onesta è che **non si
deduce**: il criterio per distinguere «due scope» da «un duplicato» è una scelta di prodotto.

| ID | Domanda | Perché non si deduce |
|---|---|---|
| `REL-1` | `RT-FEAT-REPLAY-ARCHIVE` dichiara `release: v0.2` e `epic: E12`, che è un'epic della **v0.1**. Quale dei due si corregge: la feature scende a **v0.1** (e allora entra nel perimetro dei gate di release, oggi a `4/6`), oppure resta **v0.2** e le si assegna un'epic v0.2 — che però **non esiste**? | Le prove tirano da entrambe le parti e nessuna è decisiva. **Per la v0.1**: la feature è `INTEGRATED` con 26 test verdi misurati il 2026-08-10, l'epic `E12` (*Determinismo, QA e release*) è v0.1, e [D-083](decisions/RT_PDR_00_Decision_Log.md) rinvia alla v0.2 **soltanto** `ContentManifestHash` e `RulesVersion` — che sono `RT-FEAT-DATA-HASH`, una feature **diversa**, già `RELEASE_READY` in v0.1. Cioè il `v0.2` potrebbe essere D-083 applicato all'oggetto sbagliato. **Contro**: far scendere a v0.1 una feature a `4/6` gate **allarga il perimetro di una release che sta chiudendo**, e non è una correzione di bookkeeping — è una decisione di scope, e la prende chi possiede la release. ⚠️ Nessuna epic v0.2 la rivendica: `roadmap-post-v0.1.md` non la nomina in nessuna delle sue sezioni, quindi la terza via «resta v0.2 con la sua epic» oggi **non ha un posto dove atterrare**. Innesco: **scaduto** — dal 2026-08-13 la contraddizione è visibile nella tabella dedicata di `roadmap-v0.1.md` §2.2 invece di mimetizzarsi fra le feature della v0.1 |
| `REL-2` | `RT-FEAT-CHARACTER-STATE` (`v0.4`, `E34`, `SPECIFIED`, 5 scenari `planned`) e `RT-FEAT-CHAR-TRANSFORMATION` (`v0.2`, `DESIGNED`, nessuno scenario) sono **due scope distinti** o **un duplicato con due ID**? E se è un duplicato, quale ID sopravvive? | I due puntano allo **stesso brief** (`../gameplay/brief-stati-personaggio-e-trasformazioni.md`) e alla stessa decisione [D-035](decisions/RT_PDR_00_Decision_Log.md), che presenta il sistema in **cinque famiglie** — `Stance · Form · Overdrive · Environmental · Configuration`. Una lettura plausibile è che `CHAR-TRANSFORMATION` sia `Form`/`Overdrive` (v0.2, il pezzo leggero) e `CHARACTER-STATE` l'infrastruttura completa (v0.4): il documento owner dice che *«metà dell'epic potrebbe non servire»* perché uno `Stance` è un profilo commutabile, il che **suggerisce** una separabilità ma non la disegna. ⚠️ Non si deduce perché **un Feature ID è identità persistente**: fonderli è una migrazione, e la shortlist avvertiva già che *«cambiare release a un ID stabile per far quadrare un'epic è la migrazione che poi nessuno sa più motivare»*. Finché la domanda è aperta, `CHAR-TRANSFORMATION` resta **senza epic** — assegnarle `E34` le darebbe un'epic di **v0.4** mentre lei è `v0.2`, cioè creerebbe un secondo `REL-1` invece di risolvere il primo. ✅ Una cosa si è chiusa da sé: la sua nota diceva che il brief *«esiste nel branch […] non in `main`»* e che lo stato andava rialzato **al merge** — il merge c'è (`c2560c37`), quindi la condizione era **scattata** e lo stato è passato a `DESIGNED`. Innesco: la prima issue che apra `E34` |

---

## Aperta — la varietà pseudo-casuale, dal consolidamento Mini Autobattle del 2026-08-16

Origine: [`archive/src/RefactorTactics_Mini_Roadmap_v01_Autobattle_Claude_Consolidation_2026-08-16.md`](archive/src/README.md)
§6, [D-145](decisions/RT_PDR_00_Decision_Log.md) §5 e il referto
[`roadmap/plans/mini-roadmap-autobattle-spec-panel-2026-08-16.md`](roadmap/plans/mini-roadmap-autobattle-spec-panel-2026-08-16.md) §9.

Il sorgente chiede un `MatchSeed` con stream RNG derivati da identificatori stabili, perché *«il
comportamento può avere varietà pseudo-casuale ma deve essere deterministico»*. La misura dice che il
presupposto non c'è: **il runtime non ha alcun RNG**. `FRTTestScenario::Seed` esiste ed è documentato nel
codice come *«seed dichiarato ma **non consumato**»*, e `FRTTestResult::Seed` lo registra nel report
*«anche se oggi nessun RNG lo consuma»*.

⚠️ **Non è una lacuna da colmare: è una proprietà da spendere o conservare.** Oggi il determinismo è
**strutturale** — non c'è casualità da controllare, quindi `SameSeedSameResult` è vero per costruzione.
Introdurre un seed sostituisce una garanzia gratuita con una da mantenere.

🔴 **E il guardiano di quella proprietà esiste già, il che rende la decisione più cara di quanto sembri.**
`RefactorTactics.Simulation.SeedIsDeclaredAndUnconsumed` (2026-08-15) verifica l'invariante nel verso che
morde — *due seed **diversi** devono dare lo stesso risultato, perché oggi nessuno legge quel campo* — e il
suo commento spiega perché la formulazione ovvia sarebbe vacua: *«su un progetto senza RNG, "stesso seed →
stesso output" confronta una funzione deterministica con sé stessa: passa sempre, anche a resolver
rotto»*. Il test dichiara anche cosa fare quando diventa rosso: **non aggiustarlo** — è il segnale che un
RNG è entrato — ma **sostituirlo** con due test nuovi, e *«la sostituzione è una decisione, e va scritta
accanto all'invariante #4 del piano canonico»*.

Conseguenza pratica, che è il vero costo della domanda: introdurre il seed **non aggiunge** un test,
**ne rimuove uno verde** e ne apre due. E la formula di derivazione è già scritta — PDR-05 §5,
`Hash(TurnSeed, ActionId, RollKind)`, *«così che aggiungere un VFX casuale non sposti hit e crit»*.
Quello che manca non è il come: è il **se**.

| ID | Domanda | Perché non si deduce |
|---|---|---|
| `RNG-1` | Il gioco deve avere **varietà fra partite a parità di stato iniziale**? Cioè: due esecuzioni con lo stesso setup devono poter divergere per scelta di design, oppure l'identità di risultato è essa stessa una proprietà desiderata? | È la domanda che decide se il seed serve. **Per la varietà**: quattro eroi deterministici su un layout fisso producono sempre la stessa partita, e una demo che si ripete identica dice poco su un bot che dovrà reggere avversari diversi. **Contro**: la varietà si può ottenere dal **layout e dalla disposizione iniziale** senza toccare il resolver — che è esattamente ciò che il sorgente stesso chiede di dimostrare al suo `a4` (*«due match con layout differenti»*). Nessuna delle due si ricava dai documenti: il canone dichiara il determinismo come invariante, non come *assenza di varietà*. Innesco: la prima issue che chieda `DifferentSeedVariation` — che **non fallirebbe per assenza di premessa: contraddirebbe un test verde**, `Simulation.SeedIsDeclaredAndUnconsumed`. Aprirlo significa dismettere quel test, e la procedura è già scritta nel suo commento |
| `RNG-2` | Se la risposta a `RNG-1` è sì: `BotPolicyVersion` è un **campo nuovo** o si deriva da `RT-FEAT-DATA-HASH` (*Hash di regole e contenuti*, `RELEASE_READY`)? | Il sorgente li elenca come due cose (`RulesVersion`, `BotPolicyVersion`) accanto a `MatchSeed`. Nel repository `RulesVersion` **non esiste come simbolo** — è coperto dall'hash di regole e contenuti, che è una feature già chiusa — e [D-083](decisions/RT_PDR_00_Decision_Log.md) rinvia `ContentManifestHash`/`RulesVersion` alla v0.2. Aggiungere un terzo campo di versione senza decidere se i primi due si fondono è il modo in cui nasce il quarto. ⚠️ Dipendente da `RNG-1`: senza RNG la versione della policy del bot non ha niente da qualificare. ➕ **La derivazione del seed non è invece aperta**: PDR-05 §5 la fissa in `Hash(TurnSeed, ActionId, RollKind)`, e `SeedIsDeclaredAndUnconsumed` la cita come contratto che diventa esigibile il giorno in cui il primo RNG entra. Qui si decide **se**, non **come** |

**Nessuna delle due blocca l'epic E47**: `E47.5` esclude `DifferentSeedVariation` dal proprio corpus e lo
dichiara, invece di scriverne uno che **contraddirebbe** `Simulation.SeedIsDeclaredAndUnconsumed`.
🔴 Questa riga diceva *«un test che fallirebbe per una premessa mai presa»* — cioè la formulazione che il
paragrafo quattro righe sopra ha appena corretto, sopravvissuta alla propria smentita dentro la stessa
sezione. Trovata in code review.

Tracciata su GitHub: [`#960`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/960)
(`question`) — aperta **nello stesso commit** di questa voce, perché una decisione aperta che vive solo in
un documento non entra in nessuna coda di lavoro.

---

## Aperte — ingombro e leggibilità degli oggetti graybox, dal consolidamento del 2026-08-17

Origine: [`archive/src/README.md`](archive/src/README.md) (kit `Graybox_Kit_Cover_CellVolume`),
[D-152](decisions/RT_PDR_00_Decision_Log.md) e l'owner
[`technical/spec-graybox-placement-contract.md`](technical/spec-graybox-placement-contract.md).

Il kit decide la **grammatica** e non tutti i numeri, e lo dichiara esso stesso: *«la sessione ha deciso la
grammatica, non necessariamente tutti i numeri finali»*. **Cinque** voci restano aperte, e ciascuna per una
ragione propria: un valore che si valida **guardando** (`GBX-1`), una lacuna di grammatica (`GBX-2`), una
scelta di presentazione su una scala che **esiste già** (`GBX-3`), un percorso che vive in un file non
assegnato (`GBX-4`) e un ingombro che ha tre valori in tre documenti (`GBX-5`). La sesta — **due scale che
divergevano di 1,5×** — è ✅ **CHIUSA il 2026-08-17** da [`D-163`](decisions/RT_PDR_00_Decision_Log.md):
resta in tabella perché la sua riga porta la misura, non perché sia da decidere.

> 🔴 *Terza recidiva, e le prime due sono documentate qui sotto.* Il 2026-08-17, chiudendo `GBX-6`, questa
> riga è rimasta a **«Sei»** mentre ne elencava cinque e la conclusione della sezione diceva già «cinque» —
> l'immagine speculare esatta del caso registrato nella nota seguente. Nessun gate legge un numero in prosa,
> quindi l'unica difesa è **rileggere apertura e chiusura insieme**, ogni volta che la tabella cambia.
>
> ⚠️ *Questa riga ha detto «quattro» finché le voci non sono state sei, e la nota in fondo alla sezione —
> aggiunta apposta per il caso opposto — dice che «correggere l'apertura di una sezione e non la sua
> chiusura è lo stesso difetto due volte». Il 2026-08-17 è successo **nel verso inverso**: aggiornata la
> chiusura a «cinque», lasciata l'apertura a «quattro». Trovato in code review.*

> 🔴 **Questa riga diceva che *«tre delle quattro sono aperte per la stessa ragione — sarebbero numeri
> inventati sopra un produttore che non è ancora atterrato»*, e la generalizzazione era falsa su due delle
> tre.** `GBX-2` non è un numero, e il produttore di `GBX-3` esiste dal 2026-08-07. Una ragione comune
> scritta per tre voci nasconde le due che non la condividono, e con esse l'innesco sbagliato che ne
> discendeva. Trovato in code review.

⚠️ **`GBX-1` non è la stessa domanda di `STA-*`**, e la distinzione è l'unica cosa che impedisce il secondo
owner del clearance: *quanto grande posso modellare un asset* è un contratto d'authoring `EditorOnly`,
*dove un'unità ci sta in piedi* è un dato cotto di CP 23.6. Chiuderle insieme le fonderebbe.

| ID | Domanda | Perché non si deduce |
|---|---|---|
| `GBX-1` | Quale frazione di `C` è il **Safe Placement inset** — il margine che un asset `CellBound` lascia rispetto al bordo della cella? | Il kit propone **~90%** e lo dichiara *«baseline di design da validare visivamente, non un numero competitivo sacro»*. Non si deduce perché l'unico modo di validarlo è **guardarlo**: un inset che sembra generoso a camera tattica può far sembrare le celle vuote a camera ravvicinata. ⚠️ **E non si prende in prestito da CP 23.6**: quel numero risponde a un'altra domanda (§1.1 dell'owner), e usarlo qui creerebbe una dipendenza della presentazione dal dato cotto — cioè la simulazione che decide come si modella. Innesco: la seduta che produce il Cell Placement Volume |
| `GBX-2` | Quale **canale non cromatico** distingue una porta `Closed` da una `Locked`? | È il caso che rompe `D-146` se lasciato aperto, ed è un contributo dell'audit — il kit conosce **tre** stati di porta, `ERTHexDoorState` ne ha **quattro**. `Closed` e `Locked` **negano entrambi il passaggio** e hanno la stessa geometria: la sola differenza è che il secondo non si apre. Se l'unico canale a distinguerli fosse il colore, la regola «mai solo il colore» sarebbe violata dal primo asset prodotto. Non si deduce perché le opzioni sono di design e si escludono a vicenda — un marcatore geometrico sul pannello, una barra applicata, un'icona d'overlay — e ciascuna costa un pezzo di grammatica diverso |
| `GBX-3` | A quali valori di `Integrity` corrispondono **«danneggiato»** e **«critico»**? | 🔴 **Riscritta due volte il 2026-08-17, e la seconda volta perché la prima correzione aveva sbagliato i numeri.** *(1)* La stesura originale diceva che *«niente le scala ancora»* con innesco su **CP 9.2** — che è **chiuso dal 2026-08-07** (`#70`): una domanda ancorata a un evento **già scattato** non si sveglia più. *(2)* La correzione citava una scala «`30 → 20 → 0`» e un test a sostegno: entrambi sbagliati. Il `20` veniva da un'assertion su **`FRTHexEdge`** — un **ponte** — e non esiste una scala unica. **I fatti misurati**: `FRTHexCover::DefaultIntegrity` dà **`50` per `High` e `30` per `Low`** (due soglie di partenza), i residui che i test pinnano sono **`0 · 18 · 22 · 25 · 30`**, e `bDestroyed` è un esito **enumerato**, non una soglia. **Perché resta aperta**: «critico» non può essere un numero assoluto — con due partenze diverse dev'essere una **frazione**, e sceglierla è presentazione, non balance. Nessun innesco da aspettare. Owner dei fatti: [`technical/spec-graybox-placement-contract.md`](technical/spec-graybox-placement-contract.md) §7.2 |
| `GBX-4` | Sotto quale percorso di `Content/` vive il kit graybox degli **oggetti**? | [`technical/convenzioni-contenuti-ue.md`](technical/convenzioni-contenuti-ue.md) §5 è **normativo** e non ha una riga per questa famiglia: copre la griglia (`/Game/RT/World/Grid/`, dove `Generation/` sono i *generatori*), le mappe, i personaggi, la UI. Un kit di primitive riusabili non è nessuno dei quattro. ⚠️ **Non si sceglie di fatto committando il primo asset**: [`technical/asset-map.md`](technical/asset-map.md) §6 dice che la riga d'allowlist viene **prima**, e senza di essa `git add` tace e l'asset resta locale — è lo stato di `ABP_Gadget` oggi. ✅ **L'ostacolo procedurale è caduto il 2026-08-17**: il file che deve rispondere è entrato in `integration_only` con [D-163](decisions/RT_PDR_00_Decision_Log.md). ⚠️ *Fino a quel giorno **non era assegnato a nessuna track** — né `writable`, né `integration_only`, né `generated_only` — cioè lo **STOP** di `D-139`, e questa riga lo dava come la ragione per cui la voce era una domanda invece di un edit. Era vero a metà: toglieva il permesso di scrivere la risposta, non la difficoltà di trovarla.* **Resta aperta** perché la domanda è di design e nessuno l'ha decisa: un kit di primitive riusabili non è nessuna delle quattro famiglie che §5 copre |
| `GBX-5` | Quanto deve essere grande l'**unità** rispetto alla cella — e i `1,20 m` di oggi sono uno **stato** o un **target**? | 🆕 Aperta il 2026-08-17 da [D-158](decisions/RT_PDR_00_Decision_Log.md). Tre documenti dello stesso bundle danno tre valori: il kit dice **`0.23 C`** (≈60 cm con la scala d'arte), l'handoff dice **`70–80 cm`**, l'infografica dice **`1,10–1,20 m`**. La misura scioglie *quale sia vero oggi* e non *quale sia giusto*: `BaseMeshScale = (1.2, 1.2, 1.8)` su un cilindro engine da 50 uu di raggio dà **120 uu**, cioè il valore dell'infografica — che quindi **fotografa il presente e lo etichetta «consigliato»**. ⚠️ **Il kit chiedeva l'opposto**, in lettere: *«i cilindri-unità devono essere visivamente più piccoli di quanto sono oggi»*, per lasciare spazio leggibile a cover, path, facing e superfici. ⚠️ **Il rapporto si misura nella scala in cui le mappe girano, non in quella d'arte**: nessuna mappa sovrascrive `HexSize` — verificato sui **tre binari che contengono una mappa configurata**, ciascuno con l'oracolo giusto — `Cells` per l'asset dati, `MapAsset` per i livelli, dove vivrebbe un override d'istanza, perché quel valore vive dentro i `.uasset`/`.umap` che un `grep` ordinario non apre — quindi la cella era larga `173 uu` e l'unità ne occupava il **69%** — contro il **23%** che il kit propone, un fattore di **circa tre**. 🔴 **E `D-163`, deciso lo stesso giorno, cambia il denominatore**: a `HexSize = 150` la cella è larga `260 uu`, l'unità ne occupa il **46%** e il fattore scende a **due**. 🔵 **E la scelta di lasciare `LayerHeight` a `250` lavora nella stessa direzione**: le altezze non si muovono — su `H` sono invarianti, ed è [`D-168`](decisions/RT_PDR_00_Decision_Log.md) a fissare che è `H` l'asse su cui si misurano — mentre la cella si allarga, quindi **ogni silhouette guadagna spazio libero intorno a sé**. È lo stesso effetto che questa voce chiede per l'unità, ottenuto sul denominatore invece che sul numeratore: chi prende `GBX-5` decide quanto **ancora** serve toccando `BaseMeshScale`, non se serve tutto. Il numeratore — `120 uu` — non si muove, perché non dipende da `HexSize`. ⚠️ **Chi prende `GBX-5` a U25 usi `46%` e il fattore `2`**, non i valori pre-decisione: sono la stessa misura in due mondi, e quello che conta è quello in cui la si andrà a guardare. ⏱️ Finché [`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155) non atterra, il gioco mostra ancora il `69%`: sono due misure vere in due momenti, non una che sostituisce l'altra. **Quando quella issue chiude, questa riga va riscritta al solo `46%`** — e non è l'unica: `grep -rn 1155 docs/` le trova tutte. *(La prima stesura scriveva «46%» e «la metà». Il numeratore — `120 uu` — **non dipende da `HexSize` affatto**: l'errore era prendere per denominatore la **scala d'arte** rispondendo a una domanda sulle **mappe reali**. Trovato in code review; e la diagnosi «due scale nella stessa frazione», scritta al primo tentativo di correzione, mandava a cercare sul numeratore un fattore che non c'è.)* **Perché non si deduce**: è una scelta di leggibilità che si valida **guardando**, ed è precisamente ciò che la seduta **U25** esiste per fare — la stessa natura di `GBX-1`. ⛔ **E non si chiude cambiando `BaseMeshScale` e basta**: quel valore ha consumatori in `RTUnit.{h,cpp}`, quindi la modifica appartiene a `RT-FEAT-CHAR-PRESENTATION` e non a chi modella. 🔴 **Ma nessun test lo protegge, ed è il contrario di quanto questa riga diceva prima**: la prima stesura citava `Unit.RingClearsCellDisc` come guardia, e quel test **non legge `BaseMeshScale`** — usa `90.f` letterale contro una faccia del disco scritta a mano. Il legame fu **reciso di proposito** da `#593`, che rese la clearance dell'anello una costante invece di un prodotto. `Unit.RootIsNeutral` verifica solo che la scala del segnaposto non sia unitaria: portare `BaseMeshScale` a `(0.6, 0.6, 1.8)` lascia **tutta la suite verde**. Chi prende `GBX-5` non ha una rete, e saperlo è il vero rischio. Innesco: **U25**, insieme a `GBX-1` |
| ✅ ~~`GBX-6`~~ *(riga chiusa: la terza colonna porta l'**istruttoria**, non un «perché non si deduce»)* | ~~La **scala d'arte** (lato 1,5 m) e la **scala di ogni mappa esistente** (lato 1,0 m) divergono di 1,5×. Quale delle due governa?~~ **CHIUSA il 2026-08-17: vince la scala d'arte** — lato `1,5 m`, `HexSize = 150` ([`D-163`](decisions/RT_PDR_00_Decision_Log.md)). Il cambio di codice è [`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155). | *Aperta e chiusa nello stesso giorno; la misura sotto resta perché è ciò che ha reso decidibile la domanda.* 🆕 Aperta il 2026-08-17 da [D-158](decisions/RT_PDR_00_Decision_Log.md), ed è la domanda che il bundle `GrayToolkit` ha reso visibile senza porla. `convenzioni-contenuti-ue.md` §11-bis fissa il **lato a 1,5 m** come scala d'arte dal 2026-08-09; misurato, **nessuna mappa la usa** — verificato sui **tre binari che contengono una mappa configurata**, ciascuno con l'oracolo giusto — `Cells` per l'asset dati, `MapAsset` per i livelli, dove vivrebbe un override d'istanza — `HexSize` e' un `UPROPERTY` e il valore vive dentro i `.uasset`/`.umap`, che un `grep` su `Scenarios/` e `Config/` non apre, e il default resta `100.f` in `RTHexMapAsset.h` e `RTHexMapActor.h`. **Conseguenza concreta**: una copertura bassa modellata a `0.28 C` con `C = 2,60 m` era alta 73 cm, e su una mappa reale (cella 1,73 m) copriva il **42%** invece del 28% budgetato. ⏱️ *Esempio dell'epoca: [`D-168`](decisions/RT_PDR_00_Decision_Log.md) ha poi spostato le altezze da `C` a `H`, quindi oggi la guida bassa è `0.28 H` = 70 cm. La divergenza che questa riga descriveva resta reale — cambia solo il denominatore con cui la si misura.* ⚠️ **Non si chiude scegliendo il numero più bello**: alzare `HexSize` a `150` cambia il mondo sotto ogni mappa e ogni test che misura in unità Unreal; lasciare `100` significa che §11-bis descrive una scala che nessuno usa. **Nessuna delle due è gratis**, ed è per questo che è una decisione e non una correzione. ⛔ E non era del contratto d'ingombro: l'owner della scala è `convenzioni-contenuti-ue.md` — che il 2026-08-17 **non era assegnato a nessuna track**, la stessa condizione di `GBX-4`, e che `D-163` ha portato in `integration_only` proprio per poterci scrivere la risposta |

**Le cinque che restano non bloccano il consolidamento**: il contratto dice *che forma* devono avere gli
asset, e non cambiano quella forma.

🔴 **`GBX-6` era l'eccezione, ed è chiusa come decisione ma NON come blocco.** La condizione bloccante non
è mai stata «la domanda è aperta»: era **«un asset autorato oggi risulta 1,5× fuori misura sulla mappa in
cui atterra»**, e quella è vera finché [`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155) non atterra. Vince la scala d'arte (lato `1,5 m`,
`HexSize = 150`), ma il mondo gira ancora a `1,00 m`.

> ⏱️ **La chiusura della decisione non è l'atterraggio del cambio.** `D-163` dice quale scala governa; il
> codice dice ancora `100.f`, e lo dirà finché [`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155) non chiude. Nell'intervallo un asset
> modellato secondo il contratto è **corretto per il canone e 1,5× grande per la mappa di oggi** — che è
> lo stesso rischio di prima, ma ora con una scadenza invece che con una domanda aperta.

> ⚠️ *La prima stesura diceva «nessuna delle sei», ereditando la frase da quando le voci erano quattro e
> nessuna toccava la forma. Aggiungere una riga alla tabella senza rileggere la conclusione è il difetto
> che le due note qui sotto registrano — terza volta nella stessa sezione.* Ma **due vanno chiuse prima di produrre**, non dopo:
`GBX-2` perché è una lacuna di grammatica — modellare la porta senza sapere come si distingue `Locked`
violerebbe `D-146` all'atto — e `GBX-4` perché la riga d'allowlist viene **prima** dell'asset, o `git add`
tace e il lavoro resta locale.

> ⚠️ *Questo paragrafo diceva che «tre delle quattro riguardano un numero o un percorso che si fissa quando
> il primo asset viene prodotto», ed era il gemello non corretto della frase in testa alla sezione: falso
> per `GBX-4` — che la sua stessa riga smentisce in grassetto — e per `GBX-3`, che non ha più un innesco da
> aspettare. Correggere l'apertura di una sezione e non la sua chiusura è lo stesso difetto due volte nello
> stesso testo.*

Tracciate su GitHub: [`#1094`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1094) — che porta tutte e **sei**: `GBX-5` e `GBX-6` sono state aggiunte il 2026-08-17
(`question`) — aperta **nello stesso commit** di questa voce, per la ragione che `RNG-1`/`RNG-2` hanno già
scritto: una decisione aperta che vive solo in un documento non entra in nessuna coda di lavoro.

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

Origine: [referto del kit action economy](archive/roadmap-plans/action-economy-consolidamento-2026-08-12.md).
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
| `AE-4` | Qual è il **contratto della risorsa firma**: nome player-facing, costo per azione, e quale dei tre valori vive? | Metà è **già decisa** e il kit non lo sapeva: la risorsa è **per personaggio** (Flux `Carica Conduttiva` · Riva `Riserva Idrica` · Bastion `Integrità Strutturale` · Vektor `Slancio`), cap **4**, ricarica **1** sul trigger d'affinità. ⚠️ Ciò che non è deciso è più concreto: `ARTUnit` dichiara `MaxEnergy = 100`, `EnergyPerTurn = 25`, `EnergyOnHit = 15` — parametri dell'MVP quadrato mai rivisti — e il **costo** in risorsa non è un campo del catalogo azioni (`EnergyCost` sta su `URTActionData`, il data asset legacy, non su `FRTActionDef`). Quindi un'azione del catalogo v0.1 **non sa dichiarare quanto costa** | <!-- rename-exempt: la riga dichiara la rinomina: sostituirla la renderebbe muta -->
| `AE-5` | Con quali numeri esiste il profilo **`Sneak`**? | Costo, portata e rumore non sono definiti **da nessuna fonte corrente**: il catalogo lo dichiara apertamente e non si inventano. È l'unico dei quattro profili senza budget, quindi oggi non è pianificabile. Già elencato fra le assunzioni da bloccare in fondo a questo file; qui prende un ID perché `AE-2` lo userebbe come colonna |
| `AE-6` | `Wait` restituisce risorsa firma? | Il kit chiede di **non** concederlo di default, e il canone concorda: `Wait` non dà armatura, precisione, furtività né reazione gratis (`Actions.Wait.AllowsFacingAndReaction` verifica che conservi ciò che aveva, non che guadagni). Ma la ricarica «sul trigger d'affinità» non dice cosa succede a chi non innesca nulla per tre turni, e con cap 4 la differenza è grande |
| `AE-7` | Le **eccezioni per eroe** alla compatibilità e al vincolo dell'`Overwatch` sono contenuto di kit o regole? | Il pattern è già fissato da [D-014](decisions/RT_PDR_00_Decision_Log.md)/[D-028](decisions/RT_PDR_00_Decision_Log.md) — un'eccezione si dichiara **nel kit dell'eroe**, mai nella regola generale — ma nessun kit ne dichiara una, quindi il meccanismo che le renderebbe esprimibili non esiste. Da decidere **dopo** `AE-2`: prima non si sa a cosa si fa eccezione. ➕ **Il caso concreto è arrivato il 2026-08-12**, dal brainstorming su [#604](https://github.com/DegrassiAaron/refactor-tactics-main/issues/604): l'autore vuole poter giocare **`Dash` + attacco + `Move` normale**. Come **regola generale** contraddice [D-028](decisions/RT_PDR_00_Decision_Log.md) — lo scatto occupa lo slot movimento, e quella impossibilità *è* la scelta fra *schivo e sparo* e *sparo e muovo* — ma come **eccezione di kit** è più forte della regola: un turno che tutti possono fare non caratterizza nessuno. Resta da decidere **di chi è**, e il candidato ovvio è quello che complica: `Vektor` ha già `PassingBlade` in `FastMovement` — una mobilità che **attraversa e colpisce** per 20 danni, cioè già una forma di *muoviti e colpisci* nella fase `Dash`. Concedergli anche `Dash` + attacco + `Move` renderebbe `PassingBlade` dominata dalla combinazione generica, che è il difetto che [D-028](decisions/RT_PDR_00_Decision_Log.md) ha appena corretto fra `Dash+attacco` e `Charge`. L'eccezione va quindi **assegnata guardando cosa rende ridondante**, non solo cosa caratterizza | <!-- rename-exempt: misura datata: riscriverla la renderebbe falsa -->

> **Tre voci del kit non sono qui perché non sono domande.** *«Il divieto di `Dash` con l'`Overwatch`»* è
> una **conseguenza** già decisa ([D-070](decisions/RT_PDR_00_Decision_Log.md): lo slot movimento è riservato
> a `Withdraw`) e non una regola da scrivere · *«le azioni possono orientare senza pagare due volte»* è
> ADR-0008 §3, decisa e da implementare · *«aggiornare il workbook di bilanciamento»* è vietato per iscritto
> da [`balance/README.md`](balance/README.md) e da [D-023](decisions/RT_PDR_00_Decision_Log.md).

---

## ✅ Chiuse il 2026-08-12 — traversal contro transfer

> Aperte dal consolidamento Teleport del mattino, chiuse la sera dall'autore in sessione sul secondo
> handoff ([`roadmap/plans/spatial-transfer-epic-2026-08-12.md`](archive/roadmap-plans/spatial-transfer-epic-2026-08-12.md)).
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
> destinazione, Gadget a 90 HP, e l'assertion che dopo il trasferimento sia **ancora 90**. Il turno 2 dichiara
> `requires: ["Teleport"]`, capability che non esiste, quindi resta `BLOCKED` e i 90 HP passano perché Gadget
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
| ~~`MSE-1`~~ | ✅ **CHIUSA da [`D-131`](decisions/RT_PDR_00_Decision_Log.md) il 2026-08-13.** `FRTHexCover` acquista `bGenerated`: il rebake rimuove e riscrive **solo** le coperture generate, e non tocca mai quelle dipinte a mano. Il nodo non era la sovrascrittura ma la **cancellazione** — senza provenienza un rebake non sa quali coperture erano sue, quindi togliere un segmento non poteva togliere la sua copertura. ⚠️ `bGenerated` **non entra in `ComputeHash`**: e' metadato d'authoring, e due mappe che si giocano identiche non devono divergere. ✅ La meta' **runtime** della domanda non esisteva: `RTGameMode.cpp:264` duplica la mappa d'autore a inizio partita (CP 8.4), quindi `Action.CreateCover` scrive sulla copia. Misurato, non assunto | — |

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

## Aperta — il footprint che sfiora un vertice, da `D-125`

Ciò che sopravvive di `MSE-2` dopo che [`D-125`](decisions/RT_PDR_00_Decision_Log.md) ne ha corretto
l'ingresso. Molto più stretta della domanda originale, e senza urgenza: richiede un **footprint**, non un
muro.

Un vertice dell'esagono è il punto in comune fra **quattro** triangoli di settore. Un footprint il cui bordo
ci passa esattamente sopra accende quindi due settori che non invade per area — la regola collineare di
`SegmentsIntersect` (`RTHexOccupancyLibrary.cpp:38`) tratta il contatto puntuale come occupazione.

| ID | Domanda | Perché non si deduce |
|---|---|---|
| `MSE-4` | Un settore toccato in un **solo punto** dal bordo di un footprint va contato come occupato, o serve un'intersezione di lunghezza non nulla? | Non si deduce dal codice perché la regola c'è ed è **deliberata**: il commento la dichiara scelta conservativa, e va bene per il contatto lungo un **segmento** — un footprint appoggiato al confine fra due settori li invade entrambi, ed è giusto. Il caso puntuale è un'altra cosa e nessuno l'ha considerato separatamente. ⚠️ Non si deduce dai numeri perché **oggi non esiste un produttore**: `ComputeMask` non ha chiamanti di produzione, quindi nessun footprint reale è mai stato misurato. Due uscite: **contare solo l'intersezione di lunghezza non nulla** (più preciso, e negli esagoni non apre varchi perché non esiste adiacenza per solo vertice) · **lasciare la regola conservativa** e accettare che un footprint tangente a un vertice pesi due settori in più. Innesco: [`#621`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/621), quando scriverà il primo produttore vero ⚠️ **Non più innescata da `#621`** ([`D-129`](decisions/RT_PDR_00_Decision_Log.md), 2026-08-13): il bake cuoce solo i **bordi**, quindi nessun footprint di produzione esiste e la domanda non ha ancora chi la ponga. Innesco spostato al **primo footprint di produzione**. |

> 🔎 **Perché non è urgente come sembrava.** Un footprint che *sfiora* un vertice senza entrare nella cella
> è un caso di bordo raro, e le sue due uscite differiscono di due settori su dodici — mai abbastanza da
> cambiare `Free` in `Blocked` da solo. La versione precedente di questa domanda sembrava urgente perché era
> misurata sui **muri**, che arrivano al vertice *sempre*: vedi il blocco qui sotto.

---

## ✅ Sciolta il 2026-08-13 da `D-125` — misurava un ingresso che la pipeline non produce

> ### L'occupancy misura il volume, non i muri
>
> **[`D-125`](decisions/RT_PDR_00_Decision_Log.md)**, decisione dell'autore: una cella è **libera**,
> **ingombrata** o **piena**, e ciò che la riempie sono **entità con volume** — unità, materiali, elementi
> interattivi e statici. Un **muro è un concetto di bordo**: occupa uno spessore trascurabile, sta *fra* due
> celle, e cuoce in `FRTHexCover` come `#621` già stabiliva. **Non entra nel conteggio dei settori.**
>
> 🔴 **Quindi questa domanda era mal posta, e la sua misura misurava la cosa sbagliata.** Il numero che
> l'aveva resa urgente — *«due muri consecutivi rendono la cella `Blocked` con quattro lati aperti»* — è
> stato ottenuto passando **muri perimetrali** a `ComputeMask`. Nella pipeline reale quei muri non arrivano
> mai lì: diventano coperture sul bordo.
>
> ⚠️ **Il segnale c'era, ed era nel codice.** Le quattro fixture originali stanno tutte a raggio `0.3`–`0.6`,
> cioè **dentro** la cella; quella perimetrale a raggio `1.0` è stata aggiunta il 2026-08-13 *per indagare il
> caso collineare*, e poi usata come se descrivesse l'ingresso tipico. Una fixture scritta per esplorare un
> confine non è una fixture che descrive l'uso.
>
> ✅ **Resta vero, e non era in nessun documento**: negli esagoni non esiste adiacenza per solo vertice — le
> sei direzioni condividono ciascuna un **lato intero** (`RTCellId.h:11-19`) — quindi il *varco diagonale*,
> l'argomento classico per cui il contatto puntuale deve contare, qui non è rappresentabile.
>
> ➡️ Ciò che sopravvive è **`MSE-4`** più sotto, molto più stretta: un *footprint* il cui bordo passa per un
> vertice. Le soglie `4` e `6` restano invariate, e non c'è nulla da ritarare.

---

### Il testo originale della domanda, conservato perché la sua misura è ciò che ha portato a `D-125`

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

## ✅ Chiusa il 2026-08-13 da `D-125` — due modelli di calpestabilità

> **Non erano due modelli in competizione: misurano la stessa cosa a due granularità.**
> [`D-125`](decisions/RT_PDR_00_Decision_Log.md) stabilisce che l'occupancy misura il **volume** che ingombra
> una cella, ed è esattamente ciò che il cerchio inscritto di `D-071` chiede: *ci sta un'unità?*
>
> | | Domanda | Risposta |
> |---|---|---|
> | `D-071` — cerchio inscritto | ci sta un'unità? | **binaria** |
> | #619 — dodici settori | e quanto ci sta stretta? | **ternaria**: libera · ingombrata · piena |
>
> Il secondo **raffina** il primo, non lo contraddice. La domanda «quale dei due scrive `bBlocksMovement`»
> aveva una premessa falsa: nessuno dei due lo scrive per i **muri**, che sono bordi e diventano
> `FRTHexCover`; entrambi lo fanno per il **volume**, e sono d'accordo perché misurano la stessa cosa.
>
> 🔧 **Una precisazione a `D-071` resta necessaria** ed è registrata nella sua riga del
> [Decision Log](decisions/RT_PDR_00_Decision_Log.md): *«non tocca»* si legge *«non vi entra»*. Un muro
> appoggiato al lato dell'esagono è **esattamente tangente** al cerchio inscritto — misurato, `86.602540`
> contro un'apotema di `86.602540`, differenza **zero** — e alla lettera avrebbe reso non calpestabile ogni
> cella addossata a una parete. `D-071` **non è superseded**: le mancava una parola.

---

### Il testo originale della domanda

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
> 🔑 **Rivisto il 2026-08-13: `MSE-2` e `MSE-3` non vanno «decise insieme» — sono la STESSA domanda.**
> [Referto](roadmap/plans/mse-contatto-misura-nulla-spec-panel-2026-08-13.md), che rivede `#717`.
>
> Entrambi i modelli trattano un **contatto di misura nulla** come invasione: in `B` è il **vertice**
> dell'esagono, in comune fra quattro triangoli di settore; in `A` è la **tangenza** del cerchio inscritto.
> Una sola risposta le chiude entrambe.
>
> 🔴 **E il caso di `A` è più grave di come era stato scritto.** `D-071` dice «calpestabile se il cerchio
> inscritto **non tocca** blocking geometry», con raggio = **apotema**. Misurato: un muro appoggiato al lato
> dell'esagono dista dal centro `86.602540` con `HexSize = 100`, e l'apotema è `86.602540` — **differenza
> zero**, cioè tangenza esatta. Alla lettera, *ogni cella adiacente a una parete di stanza sarebbe
> inagibile*. La prima stesura di `MSE-3` diceva che la tangenza è un «caso limite per `A`» e lasciava
> intendere che `A` fosse il modello più permissivo: **è il contrario**, e il verso era invertito.
>
> Nessuno pensa che `D-071` intendesse questo — la sua riga distingue già il muro che «taglia gli **angoli**»
> da quello che «entra nel **nucleo**» — ma la regola **scritta** non distingue *toccare* da *entrare*, che è
> esattamente la distinzione richiesta dai settori.
>
> **Le uscite si riducono a due**: il contatto di misura nulla non è invasione in nessuno dei due modelli
> (`2N` settori per `N` muri, soglie invariate, e `D-071` acquista la parola che le manca) · oppure si
> accetta e lo si dichiara in entrambi (soglie da ritarare, `D-071` da riscrivere).
>
> ✅ **Costo misurato: nessuno dei 19 test cade.** L'unica fixture con contatto puntuale è il muro
> perimetrale, e il solo test che la usa asserisce monotonia e sottoinsieme — vere per costruzione e
> indipendenti dai valori. ⚠️ Il che significa «la suite non copre la scelta», non «la scelta è innocua»:
> le quattro fixture originali evitano i multipli di 30 **di proposito**.

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
| `GEO-2` | Lo slot strutturale si chiama **`Bulkhead`**, verificato libero con `git grep`: zero occorrenze. Scartati `StructuralSlot` (`Slot` è l'economia del turno), `Panel` (collide con `Bastion.KineticPanel`, che è *copertura*) e `Section` (termine Unreal per le mesh) | [`D-082`](decisions/RT_PDR_00_Decision_Log.md) · owner [triage](roadmap/plans/triage-grid-geometry-water-2026-08-10.md) §3 | <!-- rename-exempt: misura datata: riscriverla la renderebbe falsa -->
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

⚠️ **La tabella ha oggi cinque righe, non quattro.** `FAC-15` è del 2026-08-16 e **non viene da quel
pacchetto**: sta qui perché è della stessa famiglia `FAC-*`, e la sua provenienza è lo spec panel su
[#726](https://github.com/DegrassiAaron/refactor-tactics-main/issues/726). Chi ne traccia l'origine al kit
del 2026-08-10 sbaglia documento.

> ⤵️ **Superato dal blocco qui sotto** — il conteggio di questa riga è fermo al 2026-08-13.
> **Stato al 2026-08-13**: `FAC-11` è **chiusa** da [D-126](decisions/RT_PDR_00_Decision_Log.md). **Ne restano
> tre aperte** — `FAC-12`, `FAC-13`, `FAC-14` — e nessuna delle tre è decisa dalla chiusura della prima:
> `FAC-12` aspetta la revisione dei numeri di ADR-0008, `FAC-13` aspetta che E8/E9 diano una direzione agli
> effetti d'area, `FAC-14` aspetta un caso che la richieda. Referto del secondo passaggio in
> [`roadmap/plans/facing-visualdocs-triage-2026-08-13.md`](roadmap/plans/facing-visualdocs-triage-2026-08-13.md).

> **Aggiornamento 2026-08-16 — `FAC-15` aperta e chiusa in giornata; ne restano tre.** Si era aperta
> da uno spec panel su [#726](https://github.com/DegrassiAaron/refactor-tactics-main/issues/726), e nasce da ciò che la chiusura di
> `FAC-11` **non** ha deciso: `D-126` ha fissato i sei **nomi**, non l'invariante **geometrico** della
> relazione. Le due proprietà che il lavoro runtime dà per acquisite — equipartizione dei sei lati e simmetria
> destra/sinistra — sono **incompatibili**, misurate. ✅ **Ma la seconda non era un requisito**, e
> [D-147](decisions/RT_PDR_00_Decision_Log.md) l'ha registrato: `FAC-15` si chiude e **#726 si sblocca**.
> Restano `FAC-12`, `FAC-13` e `FAC-14`, nessuna delle quali ferma lavoro costruibile.

| ID | Domanda | Cosa cambierebbe |
|---|---|---|
| ~~`FAC-11`~~ | ~~I **sei lati** devono diventare la primitiva, con gli archi **derivati** per abilità?~~ | **✅ Decisa il 2026-08-13 da [D-126](decisions/RT_PDR_00_Decision_Log.md)**: **sì**, la primitiva semantica sono le sei direzioni relative, e l'insieme di lati appartiene al **consumatore** che lo dichiara. ⚠️ **Ma la parte difficile della domanda ha ricevuto una risposta diversa da quella che sembrava implicarla**: `HexCone` **non** viene sostituito nei consumatori d'area, e nessun esito di `Guard`, copertura, vista o Overwatch cambia. Il dubbio registrato qui — *«per un attaccante lontano un cono e un insieme di tre lati non coincidono»* — è stato **misurato** invece che stimato: replicando `HexCone`/`HexLine` con le costanti reali su raggio `1..10` ci sono **45** celle di divergenza, **tutte** nel verso «tre-lati **dentro** / cono **fuori**», **zero** nel verso opposto, la prima a distanza **2**. Il cono è cioè **strettamente contenuto** nell'insieme dei tre lati, e sostituirlo sarebbe un **buff difensivo netto** — un cambio di bilanciamento travestito da rinomina. `FAC-3` resta aperta e non è toccata. Il lavoro runtime che ne nasce è [#726](https://github.com/DegrassiAaron/refactor-tactics-main/issues/726). ⚠️ **Il numero diceva «50» fino al 2026-08-16.** `50` è la misura della regola a **linea**, che lo spec panel del 2026-08-13 ha scartato lasciando in piedi le cifre; a settore le divergenze sono **45** e a distanza `2` ce n'è **una sola**, `(1, -2)`. ✅ **Lo sweep è stato eseguito con [D-147](decisions/RT_PDR_00_Decision_Log.md)**: tutte e sette le sedi vive sono allineate — `D-126`, [ADR-0005](decisions/adr-0005-orientamento.md), [ADR-0008](decisions/adr-0008-rotazione-e-policy-di-facing.md), [`DOC_CONFLICT_MATRIX.md`](DOC_CONFLICT_MATRIX.md) riga 67, [`roadmap-v0.1.md`](roadmap/roadmap-v0.1.md) epic E16, `feature-registry.yaml` con la sua vista `.json`, e il [referto del triage](roadmap/plans/facing-visualdocs-triage-2026-08-13.md). `docs/archive/` porta ancora `50` **per costruzione**: è storico. La **conclusione** non cambia |
| `FAC-12` | Il pivot **si paga** in punti movimento, o resta solo un **tetto**? | ADR-0008 §1 misura la rotazione in step e la tratta come un tetto (`MoveEndPivotMaxSteps`): ruotare fin dove è consentito è **gratis**. La fonte §10 propone un prezzo — `Move 2 celle + Pivot 60° = 3 MP` — che mette *quanto mi muovo* contro *quanto ruoto*, e farebbe pagare 3 MP anche a chi ruota da fermo (oggi libero e universale). Sono due assi di scelta diversi, non due formulazioni. Da guardare alla **prima revisione dei numeri di ADR-0008**, cioè alla chiusura di CP 16.2. 🔄 **Riproposta il 2026-08-12** dal kit dell'action economy (§15), che la presenta come «*latest explicit direction*» senza sapere di ADR-0008. Non cambia la risposta e **non cambia la data della revisione**: cambia il peso della domanda, perché due sorgenti indipendenti hanno chiesto la stessa cosa a due giorni di distanza. Il secondo aggiunge un argomento che il primo non aveva — *«viaggio più lontano o arrivo orientato bene?»* è la scelta che il **tetto** non produce, perché un tetto non si spende |
| `FAC-13` | Da dove «arriva» un colpo che **non ha una sorgente puntuale**? | Oggi la direzione d'impatto è implicitamente la cella dell'attaccante (`IsInFrontalArc(…, OriginCell)`), e non c'è risposta per proiettile con traiettoria, esplosione con centro d'area o terreno che brucia — `grep` di `ImpactDirection`/`FromTrajectory` in `Source/` dà **zero**. La fonte propone una policy esplicita (`FromSource`, `FromTrajectory`, `FromImpactCenter`, `NonDirectional`). **Non è un difetto attivo**: il danno ambientale non passa da `Plan.Hits` e un'area azzera già la copertura per costruzione. Diventa un caso da correggere quando **E8/E9** daranno una direzione agli effetti d'area |
| `FAC-14` | La **rotazione forzata** è un effetto di controllo a catalogo? | `ERTActionEffect` ha `Damage · Heal · Shield · Push · Pull · Status · DamageReduction · DamageStructure`: **nessuna rotazione**. Girare un avversario è geometricamente equivalente a spostarlo — apre un lato — e `Push`/`Pull` esistono già. ⚠️ Si tiene con `FAC-3` e `FAC-11`: se una difesa diventasse direzionale, la rotazione forzata sarebbe **l'unico modo di aggirarla senza spostare nessuno**, quindi decidere l'una senza l'altra lascia il modello sbilanciato in un verso o nell'altro |
| ~~`FAC-15`~~ | ~~La relazione a **sei direzioni relative** dev'essere **equipartita** o **speculare**?~~ | **✅ Chiusa il 2026-08-16 da [D-147](decisions/RT_PDR_00_Decision_Log.md), il giorno stesso in cui era stata aperta**: la simmetria destra/sinistra **non è un requisito**, quindi il dilemma non esisteva. ⚠️ **L'impossibilità che la voce dimostrava resta vera** — equivarianza per rotazione, simmetria speculare e partizione in sei classi non coesistono su un anello di raggio **pari**, e ai raggi dispari invece sì: l'ostruzione è di parità. Ma la voce *presupponeva* che la simmetria fosse voluta senza mai stabilirlo, e `IsInFrontalArc` ce l'ha perché è un **predicato booleano**, non perché qualcuno l'abbia scelta. ∴ l'equipartizione resta e la regola a settore semiaperto di [#726](https://github.com/DegrassiAaron/refactor-tactics-main/issues/726) **si sblocca**. 🔴 **Lo skew resta e va dichiarato**: `Front` è il raggio dritto davanti più **uno solo** dei due spicchi adiacenti, e **168 celle su 216** non ricevono la direzione speculare della propria immagine speculare — un `Shield = {Front}` proteggerebbe un fianco e non l'altro. Chi dichiara il primo insieme di lati lo sa **prima** di sceglierlo. 📐 **L'istruttoria misurata, tenuta qui perché non sopravviva solo in una nota di batch**: la regola è equipartita — **36** celle per lato a raggio `1..8`, esattamente `r` per anello, verificato `r = 1..8`; e la via d'uscita ovvia non esiste, perché **0** sottoinsiemi propri su **62** producono una regione speculare (vale per **entrambe** le regole, quindi non discrimina fra loro). 🔴 **Una regola migliore è misurata e non adottata**: lo spicchio **centrato** è altrettanto equipartito (`r` per anello) e confina l'asimmetria a **24** celle su 216 — i soli punti medi, `6` per anello pari, `0` ai dispari, cioè il **minimo possibile**. Non serve a una proprietà non richiesta; servirà se peserà la **fedeltà dei nomi** (un `TurnLog` che dice `Front` per una cella di lato è explainability di **E16**). Le due regole classificano diversamente **96 celle su 216**. ⚠️ Il verso dello skew **non è deciso**: `ERTRelativeDirection` non esiste in `Source/`, e gli indici e i nomi di `D-126` si contraddicono sulla mano. Si fissa quando la relazione entra in codice, in #726 |

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
| `INT-1` | Quali **capability di interazione** porta ciascun eroe della v0.1? | È un **asse di bilanciamento**, non un dettaglio di contenuto: se un solo eroe porta `Interaction.Force` e la mappa mette una porta rinforzata sull'unica rotta buona, quell'eroe non è una scelta — è una tassa. Le assegnazioni discusse (Gadget → `Electric`/`Tech`, Phase → `Fluid`, Riktor → `Engineering`/`Force`, Wraith → `Precision`/`Sensor`) sono **coerenti** con le identità canoniche, e proprio per questo è facile scambiarle per decise: non compaiono né nel [catalogo eroi](balance/RT_HeroCatalog_v0.1.md) né nel Decision Log |
| `INT-2` | Un **verbo** può risolvere in una fase diversa dal Blast? | Il pacchetto propone `OpenDoor → Prep`. Una porta aperta in Prep è attraversabile **col Dash**; una aperta nel Blast solo col Move. Sono due economie del turno diverse — la seconda fa costare un turno intero al piano di sfondamento, la prima no. Va decisa con un dato in mano, non per analogia |
| `INT-4` | Il **costo** di un `Interact` dipende dal verbo? | Se sì, il verbo entra nell'action economy e smette di essere solo un payload dell'elemento (§6 della spec). Nessun numero va inventato prima della risposta |
| `INT-5` | Se **più sorgenti** comandano lo stesso bersaglio, come si compongono: `AND`, `OR`, priorità, sequenza? | Aperta il 2026-08-14 con [D-138](decisions/RT_PDR_00_Decision_Log.md). `1→1` e `1→N` sono cardinalità: si rappresentano e si risolvono con un ordine deterministico, e `#833` le porta. `N→1` **non è una cardinalità in più**: è una **semantica**, e ognuna delle quattro risposte è un gioco diverso — con `AND` due giocatori devono coordinarsi nello stesso turno, con `OR` il secondo interruttore è ridondanza, con la priorità nasce un conflitto fra squadre avversarie sullo stesso bersaglio. Il dato può reggerle tutte; sceglierne una guardando il codice significa sceglierla per comodità di implementazione. ⚠️ Innesco: la prima mappa che metta due sorgenti su una porta. Fino ad allora `#833` **rappresenta** più sorgenti senza comporle |
| `INT-7` | Un `Interact` può **richiudere** ciò che ha aperto, o serve un'azione distinta? | Aperta il 2026-08-17 con [D-151](decisions/RT_PDR_00_Decision_Log.md), che spedisce il solo `Open`. **Non è un rinvio per prudenza**: `ERTHexDoorState` ha **quattro** valori e due (`Locked`, `Destroyed`) non hanno opposto, quindi «commuta» richiede una tabella completa che nessun caso di design chiede ancora; e `SetDoorState` opera su un **gruppo** di bordi il cui stato corrente può non essere unico — commutare *cosa*, se due facce divergono? ⚠️ E la risposta ha un vincolo tecnico che va letto prima di sceglierla: con la commutazione **ogni pressione produce sempre un cambio**, quindi ogni `Interact` incrementa la revisione e scrive nel TurnLog, mentre `Open` è idempotente (`CanTransition` restituisce `false` quando `Current == Wanted`). ⚠️ **Chi risponde decida anche DOVE**: se richiudere non è universale quanto aprire, appartiene a un profilo d'eroe e non al catalogo core — è il confine di [D-033](decisions/RT_PDR_00_Decision_Log.md) che [D-148](decisions/RT_PDR_00_Decision_Log.md) ha usato in senso opposto. ⚠️ Innesco: il primo scenario che abbia bisogno di **richiudere un varco**. Fino ad allora la lacuna è dichiarata — il giocatore non può chiudere una porta — e non è un difetto da playtest |
| `INT-6` | La relazione `sorgente → bersaglio` è **pubblica**, o è conoscenza di squadra? | Aperta il 2026-08-14 con [D-138](decisions/RT_PDR_00_Decision_Log.md). Non è la stessa domanda di `INT-1`: lì si chiede *chi può agire*, qui *chi può sapere che agendo là succede qua*. Le due risposte producono giochi diversi — se la relazione è pubblica, il controllo remoto è un puzzle di posizionamento; se è conoscenza, diventa ricognizione, e `Controller: ???` è uno stato dell'interfaccia. Owner della conoscenza: `E27` (v0.3) e [`gameplay/brief-conoscenza-parziale.md`](gameplay/brief-conoscenza-parziale.md); la spec CP 10.1 §11 dichiara già che il controllo remoto *«richiede la privacy dei collegamenti»*, il che rende la domanda **prerequisito** del dominio, non un suo dettaglio. ⚠️ Va risposta **prima** che `#834` scelga dove filtrare: un solo punto di lettura si progetta, dieci si scoprono |

> **`INT-3` non esiste come voce separata.** La domanda «`Interact` **richiede** un facing verso l'elemento,
> oppure lo **impone**?» era già registrata come **`FAC-6`** dal consolidamento del 2026-08-08, e resta lì:
> è una domanda sull'**orientamento**, e il suo owner è [ADR-0005](decisions/adr-0005-orientamento.md), non la
> grammatica delle interazioni. La spec di CP 10.1 ne è però il **primo consumatore concreto** — è lì che
> «girarsi verso la porta» smette di essere un caso ipotetico.
>
> Registrarla due volte sarebbe costato più della duplicazione: due ID per la stessa domanda si chiudono in
> momenti diversi, e il secondo resta aperto a mentire.

---

## ✅ Chiuse — profili d'eroe di `Brace` e `Overwatch`, dal consolidamento del 2026-08-10

Origine: [`roadmap/plans/baseaction-signatures-spec-panel-2026-08-10.md`](roadmap/plans/baseaction-signatures-spec-panel-2026-08-10.md),
triage del sorgente omonimo. Il **meccanismo** era deciso da entrambi i lati — [D-047](decisions/RT_PDR_00_Decision_Log.md)
per `Brace`, [D-012](decisions/RT_PDR_00_Decision_Log.md)/[D-014](decisions/RT_PDR_00_Decision_Log.md) e
**CP 14.4** per `Overwatch`, con il profilo dichiarato «dato per eroe, non ramo nel resolver». Mancava il
**contenuto**.

> ✅ **La sezione è chiusa dal 2026-08-13.** `BAS-2` da [D-122](decisions/RT_PDR_00_Decision_Log.md) (2026-08-12),
> `BAS-5` dal triage del 2026-08-10, e `BAS-1`/`BAS-3`/`BAS-4` da [D-132](decisions/RT_PDR_00_Decision_Log.md).
> Resta fuori dal perimetro di queste voci ciò che non era mai loro: i **numeri** dei profili — resistenza del
> `Brace`, Charge del `Grounding`, ampiezza della deviazione — che il triage elencava già come aperti e che
> sono bilanciamento, non identità. Si restringono al playtest, non in un documento.
>
> ⚠️ **Le voci restano scritte, non si cancellano**: `BAS-1` chiedeva **quattro** profili e la risposta ne dà
> **tre**, e la differenza — perché Riktor non ne prende uno — è la parte che serve a chi legge fra un anno.

| ID | Domanda | Perché serve una risposta |
|---|---|---|
| ~~`BAS-1`~~ | ~~I quattro profili `Brace` — Gadget `Grounding`, Phase `Flow`, Riktor `Anchor`, Wraith `Deflection` — entrano nel canone come contenuto di **CP 14.7**?~~ | ✅ **Sì, ma sono TRE — chiusa il 2026-08-13** da [D-132](decisions/RT_PDR_00_Decision_Log.md), decisa dall'autore: Gadget `Profile.Grounding` · Phase `Profile.Sidestep` · Wraith `Profile.Glance`, dati sopra il meccanismo di D-047, nella stessa forma con cui D-122 ha chiuso `BAS-2`. 🔴 **Riktor resta al solo profilo base, e la ragione è misurata**: `ANCHOR` prometteva «annulla lo spostamento», che `Hold Ground` (`Status.Braced`) **fa già con la stessa ampiezza** — il ramo `Braced` di `ARTTurnManager` non controlla `KnockDist`, a differenza di `Guarded`. Cardinalità 1, nessun boundary: non era contenuto, era un nome, e sarebbe stata la terza scrittura della stessa regola dopo `Reaction.Anchor` e `Gadget.Anchor`. Il roster conserva così **un eroe senza finestra sul `Brace`**, baseline della misura di CP 14.6. ⚠️ Il `Brace` segue `MaxPromptsPerReaction` (3): nessuna eccezione, e il tetto di una resolution sale — è ciò che rende la soglia dei 20 s capace di scattare. I **numeri** dei tre profili restano aperti (resistenza, Charge del `Grounding`, ampiezza della deviazione): sono bilanciamento |
| ~~`BAS-2`~~ | ~~I quattro profili `Overwatch` — `Conductive`, `Pressure`, `Frontline`, `Predictive` — entrano come contenuto di **CP 14.4**?~~ | ✅ **Sì — chiusa il 2026-08-12** da [D-122](decisions/RT_PDR_00_Decision_Log.md), decisa dall'autore: Gadget `Conductive` · Phase `Pressure` · Riktor `Frontline` · Wraith `Predictive`. Sono **profili data-driven della stessa macchina** di CP 14.4 — cambiano geometria e parametri, non creano quattro rami di resolver — e il ciclo resta `arm → micro-step → target rilevato → opportunity → FIRE/HOLD`. La condizione HP di [D-109](decisions/RT_PDR_00_Decision_Log.md) è **opzionale**. ⚠️ **Il bot v0.1 non ne dichiara una automaticamente**: una soglia fissa sarebbe bilanciamento travestito da default, e derivarla dall'utility planner non serve a provare la Decision Window. Risponde **subito** all'opportunity sanitizzata via `DecisionProvider`. 🔴 I due pacing non si sommano in un KPI: quello tecnico è immediato, quello umano è la finestra reale da 3,0 s, e un campione col bot va etichettato come tale. ➡️ Ora `Overwatch.ProfileIsDataNotBranch` ha il secondo profilo che gli mancava. Issue [#657](https://github.com/DegrassiAaron/refactor-tactics-main/issues/657) |
| ~~`BAS-3`~~ | ~~`Vektor.Deflection` (**Wraith**): un nome, **due semantiche**. Si rinomina il profilo `Brace`, si rinomina la reazione, o si unificano?~~ | ✅ **Chiusa il 2026-08-13** da [D-132](decisions/RT_PDR_00_Decision_Log.md): si rinomina il **profilo**, in `Profile.Glance`. `Vektor.Deflection` (`RTHeroCatalogLibrary.cpp:604`, CP 6.7, **−20 sul colpo diretto**) resta un'entità sua: cede il nome **chi arriva dopo**, come per la rinumerazione dei `D-04x`. 🔄 **Motivazione aggiornata**: non è più «il token è serializzato e non si rinomina» — [D-130](decisions/RT_PDR_00_Decision_Log.md) supera D-120 proprio lì e lo redirige a `Hero.Wraith.Deflection` ([#716](https://github.com/DegrassiAaron/refactor-tactics-main/issues/716)). Resta vero, e basta: **due semantiche opposte non condividono un nome**. ⚠️ Scartato `Profile.Deflect`: due semantiche **opposte** sullo stesso eroe che differiscono di una sillaba sono peggio di due nomi lontani — in prosa nessuno le distingue e il gate naming non le vede. Il namespace di catalogo rende comunque la collisione impossibile a livello di token | <!-- rename-exempt: la riga dichiara la rinomina: sostituirla la renderebbe muta -->
| ~~`BAS-4`~~ | ~~`Riva.Flow` (**Phase**): il profilo `Brace` è la **promozione** di `Riva.FlowReaction`, o una terza cosa?~~ | ✅ **Chiusa il 2026-08-13** da [D-132](decisions/RT_PDR_00_Decision_Log.md): **due entità distinte**. `Riva.FlowReaction` (`RTHeroCatalogLibrary.cpp:377`, `Reposition 1` **dopo un attacco subito**, slot `None`) resta com'è e prosegue in E14; il profilo `Brace` di Phase è nuovo, si chiama `Profile.Sidestep` e risponde al **Forced Movement**. Conserva il caso che li separa — «Phase colpita ma non spostata» — che una fusione avrebbe cancellato in silenzio, e che oggi nessun documento dice quale dei due produca. 🔎 **La domanda era più stretta del problema**: «Flow» per Phase erano già **tre** entità — `Riva.FlowReaction`, `State.Riva.Flow` (stance post-v0.1, E34) e il profilo proposto — mentre questa voce chiedeva solo se fondere le prime due. Per questo il profilo non si chiama `Flow`: la stessa regola applicata a Wraith in `BAS-3`. ⚠️ Le righe citate da questa voce e da `BAS-3` erano **scadute** (`:351` e `:557`) e sono state rimisurate qui | <!-- rename-exempt: misura datata: riscriverla la renderebbe falsa -->
| ~~`BAS-5`~~ | ~~Dopo l'`Overwatch`: **Move con budget ridotto**, o **Watch Stage + Reposition** pianificato?~~ **Chiusa come domanda il 2026-08-10**: prevale il modello **Watch → EndWatchStage → Reposition**. Non per la data — è la stessa — ma perché il sorgente gemello si dichiara superato su questo punto (§34 e §48 elencano «vecchio post-Overwatch Normal/Sneak Move» fra ciò da correggere), e perché è l'unico dei due modelli che dice **dove si trova** il personaggio quando l'Overwatch finisce. Restano aperti il suo **costo** (`OW-1`) e il suo **nome** (`OW-2`): vedi [`roadmap/plans/overwatch-runtime-lifecycle-triage-2026-08-10.md`](roadmap/plans/overwatch-runtime-lifecycle-triage-2026-08-10.md) |

> **Le affinità di interazione per eroe non aprono una voce nuova.** Il §17 del sorgente (Gadget → generatori e
> pannelli, Phase → valvole e pompe, Riktor → cover, porte e barricate, Wraith → standard) **ricalca** le
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
| ~~`BAL-1`~~ | ~~`Guard` e `Brace` devono separarsi in **danno contro spinta**?~~ | ✅ **Chiusa il 2026-08-12** da [D-121](decisions/RT_PDR_00_Decision_Log.md), decisa dall'autore: **status quo**, nessuna separazione e nessuna magnitudine nuova. `Guard` resta difesa **front-loaded** sul primo impatto; `Brace` resta **sostenuta** sui colpi ripetuti e più robusta contro il displacement forte quando il contenuto lo produce. **Nessun rebalance numerico.** ⏳ **Resta il gate umano, non la scelta**: [#403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/403) è aperta solo per `U20 / PIE-BAL1`, che verifica se la differenza è **leggibile a schermo**. E l'ordine è vincolato: se non lo è si interviene prima su feedback/UI, poi — solo se non basta — sui numeri. ⚠️ Da tenere separata da `ECO-1` (*quanto costano*): se costo ed effetto cambiassero insieme il playtest non saprebbe quale dei due sta misurando. **Storico della misura che ha portato alla decisione:** [D-066](decisions/RT_PDR_00_Decision_Log.md) ha misurato il modello in vigore: entrambi fanno entrambe le cose, e differiscono per *forma* (primo colpo forte vs ogni colpo; spinta di 1 cella vs spinta qualsiasi). È bilanciamento: si chiude con una partita, non con un documento. ✅ **La Fase 0 è decisa** ([D-074](decisions/RT_PDR_00_Decision_Log.md), 2026-08-10, issue [#400](https://github.com/DegrassiAaron/refactor-tactics-main/issues/400)): si accetta che in v0.1 ogni spinta valga 1 e si **riscrive** la clausola «senza limite di distanza» invece di introdurre una spinta `≥ 2`. Conseguenza sulle opzioni ancora in campo: restano lo **status quo** e l'**ibrido** (separare le magnitudini); l'opzione *«`Guard` solo danno, `Brace` solo spostamento»* era **preclusa**, perché senza spinta forte lasciava `Brace` senza mestiere. 🔄 **Non più, dal 2026-08-11**: `Weapon.Impact` su `Riva.PressureJet` produce una spinta di **2** ([D-085](decisions/RT_PDR_00_Decision_Log.md)) ed è il default di Riva ([D-089](decisions/RT_PDR_00_Decision_Log.md)), quindi la spinta forte che `D-074` aveva scartato è arrivata dall'**equipaggiamento** invece che dal catalogo azioni. Misurato da `Equipment.PushTwoSeparatesGuardFromBrace`: contro una spinta di 2 **`Guard` cede e `Brace` regge**. Le opzioni tornano **tre**, con una domanda nuova — quel mestiere dipende da un equipaggiamento equipaggiato, non da una regola del turno. ✅ Gli scenari che servono a decidere esistono e sono verdi ([#401](https://github.com/DegrassiAaron/refactor-tactics-main/issues/401)): `Spec.Brace.GuardAndBraceOnMixedHit` e `Spec.Brace.BraceWinsOnSecondHit` pinnano il trade-off reale — *primo colpo pesante* (`Guard` 1 danno) contro *colpi ripetuti* (`Brace` 12 contro 17 su due colpi). ⏳ **Resta l'unica parte che richiede l'autore**: la seduta editor **U20** (voce `PIE-BAL1`) e la scelta fra le due opzioni superstiti. Roadmap e numeri: [`bal-1-guard-brace-roadmap-2026-08-10.md`](roadmap/plans/bal-1-guard-brace-roadmap-2026-08-10.md). Issue [#403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/403) (decisione) | <!-- rename-exempt: misura datata: riscriverla la renderebbe falsa -->
| `ECO-1` | `Guard` e `Brace` competono con il **Main Commitment**, o hanno un'altra economia? | [D-012](decisions/RT_PDR_00_Decision_Log.md) copre `Attack \| Ability \| Overwatch` e **non** dice nulla di `Guard` e `Brace`, che a catalogo occupano l'azione principale ma non compaiono in quella regola. La domanda si porta dietro la matrice Sprint/Sneak proposta dal sorgente (`Brace` e `Overwatch` senza Sprint), che **non è canonica** e non va resa tale senza playtest. 🔄 **Generalizzata il 2026-08-12 da `AE-1`**: il kit dell'action economy pone la stessa domanda per *tutte* le azioni, non per due. Resta qui perché è la sua istanza concreta — se `AE-1` conferma gli slot, `ECO-1` va comunque risposta; se li sostituisce, `ECO-1` si dissolve. ➕ **Ha una proposta e quattro opzioni misurate**: [#617](https://github.com/DegrassiAaron/refactor-tactics-main/issues/617). ⚠️ **E la domanda è mal posta in un punto**: tratta `Guard` e `Brace` come un caso solo, mentre il catalogo dice che **`Brace` paga due prezzi e `Guard` uno** — `Action.Brace` applica a sé `Braced` **e `Root`**, e `Root` porta `EffectiveMoveRange` a zero già nel `Move` dello stesso turno. Da qui l'opzione più economica: `Brace` esce dalla principale perché il radicamento *è già* il suo prezzo, `Guard` resta. La via che sembrava ovvia — spostarli sullo slot `Reaction` — **è più cara di così**: quello slot è l'ingresso alla macchina dei trigger, e nessuna delle due azioni ne ha uno. Da tenere separata da `BAL-1`/[#403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/403), che chiede *cosa fanno*: se costo ed effetto cambiano insieme, il playtest non sa quale delle due sta misurando |

---

## Il ciclo Watch/Withdraw — tre voci chiuse il 2026-08-10, una aperta

Origine: [`roadmap/plans/overwatch-runtime-lifecycle-triage-2026-08-10.md`](roadmap/plans/overwatch-runtime-lifecycle-triage-2026-08-10.md).
Il **modello** era già deciso da `BAS-5` sopra — `Watch → EndWatchStage → Withdraw` — e gran parte del sorgente
era già canone-compatibile: la cadence *once-per-target* ha persino uno scenario che la esprime da prima
(`Spec.Overwatch.HoldThenFire`, dove Wraith fa `HOLD` su Gadget e `FIRE` su Phase). Restavano aperti **quanto
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
| ~~`WV-3`~~ | ~~Il **default** di variante per ciascun eroe~~ | ✅ **Chiusa il 2026-08-11** — [D-089](decisions/RT_PDR_00_Decision_Log.md): `Flux → Precisione`, `Riva → Impatto`, `Vektor → Soppressione`, `Bastion → Impatto`. Criterio: il default **rinforza l'identità**. Nessun default usa `Sovraccarico`, il cui costo è ancora `WV-1` | <!-- rename-exempt: misura datata: riscriverla la renderebbe falsa -->
| ~~`WV-4`~~ | ~~Che cosa modifica **`Weapon.Environmental`**~~ | ✅ **Chiusa il 2026-08-11** — [D-100](decisions/RT_PDR_00_Decision_Log.md): resta fuori dalla v0.1, e non perché manchi **un parametro** ma perché manca un **produttore** — *nessun attacco base crea ambiente*. Su Flux e Vektor non avrebbe niente da migliorare. Si riapre da sé il giorno in cui un attacco base dichiarerà `bCreatesSurface` | <!-- rename-exempt: misura datata: riscriverla la renderebbe falsa -->
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
| ~~Mapping visuale Paragon → roster~~ | **chiusa il 2026-08-08** da [`D-037`](decisions/RT_PDR_00_Decision_Log.md): Flux → `Paragon.Gadget`, Riva → `Paragon.Phase`, Bastion → `Paragon.Riktor`, Vektor → `Paragon.Wraith`. Tabella owner in [`characters/paragon.md`](characters/paragon.md). Resta aperto solo il **nome retail** dei quattro slot v0.2, che è la riga «Identità originale» qui sopra | <!-- rename-exempt: misura datata: riscriverla la renderebbe falsa -->

## ✅ Chiuse il 2026-08-09 — sessione `/sc:brainstorm` su E13

Le tre voci `PER-1`…`PER-3`, aperte dalla spec panel dello stesso giorno, sono state decise dall'autore.
Restano qui **solo come indice**: il contenuto vive nel [Decision Log](decisions/RT_PDR_00_Decision_Log.md).

| Era | Decisione presa | Dove vive ora |
|---|---|---|
| `PER-1` | La soglia d'udito e' una statistica **per eroe** e **compensa** la vista: Gadget 5 · Phase 3 · Riktor 3 · Wraith 5 | [`D-041`](decisions/RT_PDR_00_Decision_Log.md) |
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
| **Con quali valori si tara il Decision Time Bank?** | ⏳ **aperta il 2026-08-09**. *Non* è più aperto **se** costruirlo: entra in **v0.1** come **CP 14.8**, senza gate — owner [`gameplay/spec-decision-time-bank.md`](gameplay/spec-decision-time-bank.md), audit di provenienza [`roadmap/plans/decision-time-bank-conflict-report-2026-08-09.md`](roadmap/plans/decision-time-bank-conflict-report-2026-08-09.md). Il bank è un cap aggregato per un costo che [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) §8 aveva deciso di **misurare prima di contenere** (`D20`, nessun cap): quel rischio è ora **assunto in senso opposto e dichiarato** (spec §2.1), e i due rientri di ADR-0004 §Revisione — *cap aggregato condiviso* e `MaxPromptsPerReaction = 1` — restano validi e compatibili. Resta aperta la **taratura**: `InitialBank` è derivato (`RoundLimit × (MaxWindow − Grace)` → 24 s in 2v2), `Grace` 1,0 s ed `ExhaustedGrace` 0,75 s sono `PROPOSED FOR PLAYTEST` con i criteri di uscita di §3.2. Prima misura utile **CP 14.6**, che CP 14.8 non precede. Metrica che decide: `ReactionDecisionSeconds`, separata da `ResolutionPlaybackSeconds`. 🔎 **Precisato il 2026-08-13 da [D-133](decisions/RT_PDR_00_Decision_Log.md)**: CP 14.6 è il **punto di taratura** di ADR-0004 — la §Revisione prometeva un atto, ma i suoi due rientri erano già stati consumati, uno proprio da `D-050` che costruisce questo bank. La soglia si legge `p50`/`p90`, non come massimo, su un campione di **≥ 10 partite**: lo stesso di `InitialBankMs` qui sotto, perché due misure che si informano a vicenda devono leggere lo stesso campione. Dopo **CP 14.7** la taratura si **ripete** — il reveal a scadenza fissa alza il pavimento, e sono i valori di `Grace` a risentirne. Restano aperte anche `TB-5` e `TB-7` (policy di rete, M10): vivono nella spec §17, **non si duplicano qui**. ➕ **Dal 2026-08-17 la taratura ha una dipendenza in più, e va detta prima che qualcuno pianifichi un playtest solo**: [D-156](decisions/RT_PDR_00_Decision_Log.md) aggiunge `LoadFactor` e i due coefficienti di grace per Hero extra, aperti come `TB-9` nella spec §17 — e `TB-9` è **bloccata da `TB-8`**, perché sono moltiplicatori di un `GraceMs` che è a sua volta `PROPOSED`. Misurarli insieme significherebbe leggere due incognite in un campione solo. 🔴 **E questa riga ha detto il contrario per mezza giornata, corretto in code review**: diceva *«in v0.1 il caso non si produce … `TB-9` diventa misurabile quando un formato dichiara un conteggio diverso da uno»*. **Falso, e nel modo che conta**: `Format.Skirmish2v2` è offline contro bot, quindi c'è **un** umano che comanda **due** unità — la v0.1 *è* il caso multi-Hero, e `LoadFactor(2)` è vivo nell'unico formato che spedisce. La conseguenza non è teorica: con `RoundLimit = 12` il bank derivato passa da **24 s** a **31,5 s** e il costo del timeout da 2,0 s a 1,5 s. Nessun numero è stato promosso — restano tutti `PROPOSED` — ma il valore che CP 14.6 andrà a misurare **non è quello scritto in §3.2**, ed è la prima cosa da rileggere prima di raccogliere il campione. Owner del conteggio: [D-155](decisions/RT_PDR_00_Decision_Log.md) e **CP 19.3** ([`#1124`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1124)), che porta il campo da cui il fattore si legge |
| ~~**Come si chiama la categoria di log del Decision Time Bank: `Decision` o `ReactionDecision`?**~~ | ✅ **Chiusa il 2026-08-17 da [D-166](decisions/RT_PDR_00_Decision_Log.md)**, dopo meno di un giorno di apertura — mentre lo **scarto fra spec e codice** che l'aveva prodotta durava da **tre** (`75039d93`, 2026-08-14). Esito: **`Decision` nasce distinta**, con `ERTDecisionOutcome` (`BankConsumed`, `BankAfter`, `BankExhausted`) e `Amount` in **millisecondi**; `ReactionDecision` resta con `Amount` in **danni**. Prevale [`technical/spec-turnlog.md`](technical/spec-turnlog.md) §4.2, che lo prescriveva dal 2026-08-09 (`#361`), ed è il caso particolare di [D-162](decisions/RT_PDR_00_Decision_Log.md) — *una categoria, un enum*. **L'istruttoria vive nell'owner** e non si duplica qui: [`gameplay/spec-decision-time-bank.md`](gameplay/spec-decision-time-bank.md) §10.1. ⚠️ Resta da **scrivere** l'enum, la voce in coda a `ERTLogCategory` e il `case` in `OutcomeEnumForCategory`: sono tre siti, non uno, ed è lavoro di CP 14.8 |

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
