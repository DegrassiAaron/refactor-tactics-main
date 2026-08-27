# Decisioni aperte

> `OPEN` · **Stato**: vivo · **Ultimo aggiornamento**: 2026-08-27
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

## Aperta — come il bot sceglie fra due reazioni, dal 2026-08-27

Origine: [D-220](decisions/RT_PDR_00_Decision_Log.md). Fino a [D-218](decisions/RT_PDR_00_Decision_Log.md)
ogni eroe ne portava **una** e la domanda non esisteva. Oggi ne porta due **un eroe solo**: Riktor. Gadget e
Wraith hanno il loadout **vuoto** (i gadget prescritti non sono spediti), Phase ha il modulo ma **nessuna
reazione di kit**.

| ID | Domanda | Esito, e l'istruttoria che ci è arrivata sotto |
|---|---|---|
| `BOT-REACT-1` | **Il bot deve preferire la reazione di kit o il modulo di loadout?** | ⏳ **Aperta.** [D-220] ha **dichiarato la regola che c'era già** — prima il kit, il modulo come riserva quando il kit è in ricarica — invece di inventarne una: prima la produceva l'**ordine degli indici**, per accidente. La preferenza ha una ragione (*la reazione di kit è ciò che l'eroe **è**, il modulo è ciò che la composizione gli **aggiunge***), ma non è una valutazione tattica. 🔴 **E invertirla non è una pulizia**: i moduli hanno `CooldownTurns = 0`, quindi un bot che preferisse il loadout non armerebbe **mai più** la reazione d'eroe — un «mai» scambiato col «mai» opposto. 🔴 **E il costo e' misurato, non teorico**: `Interposition` va in ricarica **solo quando scatta** — non in pianificazione — quindi e' utilizzabile quasi ogni turno, e il modulo viene armato solo nei ≤3 turni dopo un'interposizione. Il bot Riktor riceve la `Reaction.Cleanse` che [D-218] gli ha dato in una **minoranza** dei turni. ⚠️ **La domanda vera per E15**: quando conviene una purificazione preventiva e quando un'interposizione? Finché non c'è un'euristica che lo valuti, la regola dichiarata è la risposta meno arbitraria disponibile |

---

## Aperte — governance documentale, trasferite da `#1396` il 2026-08-27

`#1396` è stata **chiusa** perché il suo unico creditore — `#1389` — è chiuso, e la matrice ha **zero** righe
in stato `CONFLICT`: nessuna riga aspettava quella risposta. Il caso concreto che la bloccava l'ha risolto
[D-210](decisions/RT_PDR_00_Decision_Log.md). Le tre domande di fondo restano vere e senza consumatore, e
questo è il posto di ciò che aspetta una persona senza fingere di essere lavoro pendente.

| ID | Domanda | Esito, e l'istruttoria che ci è arrivata sotto |
|---|---|---|
| `GOV-1` | **Le quattro formulazioni di prevalenza sono una gerarchia espressa male, o quattro con oggetti diversi?** | ⏳ **Aperta.** Una ordina **documenti** (`README.md` §Gerarchia), una **tipi di affermazione** (header del Decision Log), una **fonti di natura diversa** (prosa di `DOC_CONFLICT_MATRIX.md`, ora normativa per [D-210]), una un **taglio del canone** (`piano-canonico-mvp.md` §1). ⚠️ **Se ordinano oggetti diversi non sono in conflitto**, e la premessa di `#1396` — «quattro formulazioni di prevalenza» — è la cosa da correggere, non la gerarchia |
| `GOV-2` | **Il canone contiene una gerarchia che batte quella che lo classifica: paradosso o delega?** | ⏳ **Aperta.** `piano-canonico-mvp.md` §1 sta al **livello 1** della tabella del README, che lo annota «prevale su tutto». Delle due: o è un paradosso di governance, o è una **delega** — il README dice «per i documenti chiedi al canone». La seconda lettura non richiede nessuna correzione, solo una riga che la dichiari |
| `GOV-3` | **Dove stanno i cataloghi di `balance/` nella prosa della matrice, che non li nomina?** | ⏳ **Aperta, e già mezza risposta.** [D-210] li ha collocati nella tabella del README — sotto le specifiche, «numeri non regole» — ma la scala normativa in cima a `DOC_CONFLICT_MATRIX.md` continua a non nominarli. Coerente, incompleta |

⚠️ **Nessuna delle tre blocca niente oggi.** Vanno riaperte come issue il giorno in cui una riga di matrice
finisce in `CONFLICT` per causa loro — non prima.

---

## Aperte — `Action.Cleanse`, dal consolidamento documentale del 2026-08-27

Origine: [D-211](decisions/RT_PDR_00_Decision_Log.md), che ha allineato tre documenti sul limite reale di
`Action.Cleanse` e ne ha lasciato scoperta la parte che **non è documentale**. La riga 78 di
[`DOC_CONFLICT_MATRIX.md`](DOC_CONFLICT_MATRIX.md) rimanda qui, come prescrive il suo preambolo: o
`SUPERSEDED` con la fonte che prevale, o una voce in questo file.

| ID | Domanda | Esito, e l'istruttoria che ci è arrivata sotto |
|---|---|---|
| `CLEANSE-1` | **`Action.Cleanse` spedisce in v0.1? E se sì, chi riempie `PlannedCleansePriority`?** | ⏳ **Aperta.** Le due metà non si decidono separate: una `Cleanse` raggiungibile con lista vuota non fa niente, e una lista piena su un'azione irraggiungibile nemmeno. 🔴 **Misure che restringono il lavoro che l'azione fa**: la cleanse **reattiva** (`Reaction.Cleanse` → `Action.Purge`) è costruita e testata, e annulla `Root` e `Slow` **in arrivo** — sono le due sole voci di `ControlStatusesBySeverity()`. `Burning` lo spegne **l'acqua** (`ARTUnit::ApplyStatus`: *«rimosso da `Wet`»*), e `Gadget.Sprinkler` è il default di Phase. `Marked` ed `Exposed` si consumano da soli. 🔴 **Misura del 2026-08-27, alla seconda stesura** ([#1479](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1479)) — ⚠️ **la prima era sbagliata e diceva che nessuno stato inflitto da un nemico fosse purificabile.** Falso: la Cleanse è il **primo** pass del Blast e precede tutti e tre i punti in cui gli stati vengono letti (`bMarkedBeforeBlast` `:3903`, `bWetBeforeBlast` `:4026`, `Exposed` `:4044`), quindi previene già **`Marked`** — che è inflitto da un avversario — oltre a `Wet`, `Exposed` e `Burning`. Gli unici due che non contrasta **mai** sono **`Root`** e **`Slow`**: applicati in coda al Blast, consumati dal Move, scaduti nello stesso `Cleanup`. E la leva non è la posizione della Cleanse — spostarla è uno scambio a somma quasi nulla, provato e chiuso senza merge in #1481 — ma la **durata** di quei due stati. ∴ l'insieme utile per il produttore è più largo di `{ Status.Burning }`: ci stanno anche `Wet` ed `Exposed`, che sono contrastabili oggi. ⚠️ **Resta da nominare il caso che solo l'attiva risolve**: se non c'è, la risposta è che esce dalla v0.1 — e allora `Action.Shield`, nella stessa condizione, esce con lei. ⚠️ **La forma costa**: `PlannedCleansePriority` è l'unico dei **dodici** parametri di piano con zero produttori, ed è anche il più caro da esporre — gli altri undici sono una cella, un bersaglio, una direzione: cose che si cliccano. Un ordinamento di N tag no. Le alternative sono un **default d'eroe** in `URTHeroData` (dato, nessuna UI — ma supera *«la priorità è scelta dal giocatore durante il planning»*, e va superata esplicitamente) o **una scelta sola** invece di una lista |
| ~~`CLEANSE-2`~~ | ~~**`Reaction.Cleanse` entra in un loadout di default?**~~ | ✅ **SÌ, A RIKTOR — chiusa il 2026-08-27 da [D-218](decisions/RT_PDR_00_Decision_Log.md)**, [PR #1485](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1485). Il «a chi» è stato **misurato, non scelto**: il modulo prescritto a Riktor era `Reaction.AllyIntercept`, costruito su `Action.Intercept` — lo **stesso core** della sua reazione di kit `Hero.Riktor.Interposition`. Lo slot spendeva su una capacità già posseduta, quindi sostituirlo non gli toglie un mestiere. ⚠️ **Non a costo zero come diceva la prima stesura**: `Interposition` ha cooldown 3 e il modulo 0, quindi si perde un'interposizione *ogni turno* in cambio di *ogni tre* — piccolo, ma reale. ⚠️ **Gadget ha lo stesso duplicato e resta**: il suo loadout è vuoto perché `Gadget.Insulator` non è spedito, e l'eccezione è dichiarata nel test `Equipment.DefaultReactionModuleIsNotADuplicate` |

---

## ✅ Chiuse il 2026-08-24 da `D-187` — i prerequisiti della seduta `U19` e il suo `done_when`

Origine: lo spec panel su [`#84`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/84) (PR [`#1313`](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1313)) e la sua riverifica del
2026-08-24. Le due domande erano già state **trovate** e scritte nel corpo di `#84`. Un corpo di issue non è
una sede: non ha stato, non si chiude, e la prossima sessione che pianifica `U19` le ritrova solo se apre
quella issue. ⚠️ È la stessa forma del difetto che `#1313` denunciava in chiusura — *«una correzione resta
dove nasce»* — applicata a una **domanda** invece che a una correzione.

Nessuna delle due è deducibile dai documenti: due fonti si contraddicono senza gerarchia, ed è il criterio
d'ingresso di questo file.

| ID | Domanda | Esito, e l'istruttoria che ci è arrivata sotto |
|---|---|---|
| ~~`KPI-1`~~ | ~~I prerequisiti della seduta **`U19`** sono **cinque** o **tre**?~~ | ✅ **CINQUE — chiusa il 2026-08-24 da [`D-187`](decisions/RT_PDR_00_Decision_Log.md).** `unblocked_by: [U6, U1, U5, U7, U8]` è la fonte. 🔴 **E la contraddizione era meno grave di come questa voce la scriveva**: il campo `steps` dice *«è il motivo per cui `U5` e `U8` sono **fra** i suoi prerequisiti»* — **«fra»** dichiara un sottoinsieme, quindi la prosa **commenta** `unblocked_by` invece di contraddirlo. Gli altri due sono giustificati dal contenuto: `U1` produce l'arena che il **passo 1** misura (*«il criterio è definito in U1 passo 7»*), `U6` è la partita intera che la seduta gioca. ⚠️ A essere imprecisa era la **nota §4** del DoD, ora allineata: nomina i tre più stringenti e **dichiara** di essere un sottoinsieme |
| ~~`KPI-2`~~ | ~~**`E14`** entra in `U19.unblocked_by`, o il `done_when` della seduta si restringe?~~ | ✅ **IL `done_when` SCENDE A TRE — chiusa il 2026-08-24 da [`D-187`](decisions/RT_PDR_00_Decision_Log.md).** `PIE-V01-MATCHLEN`, `PIE-V01-READY`, `PIE-V01-MAPSCALE`. `PIE-V01-OVERWATCH` esce, perché il **passo 4** della stessa seduta la rinvia a `E14` — che non è fra i prerequisiti e resta aperta: un `done_when` che pretende una voce rinviata a un'epic assente **non si chiude mai**. ⛔ **Scartata l'alternativa** di aggiungere `E14` ai prerequisiti: bloccherebbe una seduta di **misura** dietro un'epic di funzionalità, e `U19` sta sul percorso di [`#84`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/84), che è gate di release. ⚠️ **La voce non sparisce, cambia owner**: resta nel registro PIE con `E14` come sbloccante, e va rimessa in un `done_when` il giorno in cui `E14` chiude |

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
| ~~`FMT-2`~~ | ~~**Quante mappe si committano: una che faccia da rivelatore, o entrambi i `DA_HexMap_*`?**~~ | ✅ **Chiusa il 2026-08-13 per derivazione**, e non da una decisione propria: il suo stesso testo diceva *«non si deduce perché la risposta dipende da `FMT-1`»*, e con `FMT-1` chiusa la condizione è **scattata**. La regola che aveva già scritto: *«se `FormatVersion` viaggia, un rivelatore basta e il resto è codice; se non viaggia, nemmeno quello rivela nulla»*. Con `D-137` viaggia → **uno basta**, ed **esiste già**: `DA_HexMap_Arena`, committato l'11 agosto quando la versione corrente era 6, è l'unico asset mappa con contenuto reale del repository. ⚠️ Dopo `#687` acquista una proprietà che oggi non ha: essendo stato scritto **prima** del meccanismo non avrà voce nel registro delle custom version, cioè sarà `legacy` in modo **non ambiguo** — che è esattamente ciò che un rivelatore deve dimostrare. Committarne un secondo non aggiungerebbe una prova: aggiungerebbe una copia. ⚠️ **I numeri misurati qui sono scaduti, e la riga li rimisura invece di ripeterli.** Diceva *«`.gitignore` ha **18** righe `!Content`, di cui **5** di mappa — tre livelli `.umap` e **due** data asset»*, e al 2026-08-18 sono **26** righe `!Content`, di cui **6** di mappa: tre `.umap` e **tre** data asset (`DA_HexMap_Sandbox`, `DA_Format_Scratch` che #623 ha riammesso, `DA_HexMap_Arena`). ⚠️ *La conclusione della voce **non** dipende da questi numeri — il rivelatore resta `DA_HexMap_Arena` e uno basta — ma la riga li dichiarava «validi», quindi andavano rimisurati da chi tocca il file: trovato in code review su [`#1188`](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1188), che ne aggiungeva una ventiseiesima senza rileggere la frase. Il comando è `grep -c "^!Content" .gitignore`, e va rieseguito invece che copiato* |
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

## ✅ Chiuse il 2026-08-24 da `D-185` — come il bot sceglie fra vedere e avvicinarsi

Origine: [`#1296`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1296), PR
[`#1297`](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1297) e la code review che l'ha
seguita. La misura sotto viene da `L_HexArena` con `DA_HexMap_Arena`, la mappa che la partita carica.

Il bot non ha alcun termine che dica **«da qui posso ingaggiare»**. Il punteggio misura danno, minaccia,
distanza e quota; la linea di tiro entra solo come condizione della minaccia *subita*. Finora la lacuna è
stata coperta due volte, e in due modi che si escludono:

| | come | cosa produce |
|---|---|---|
| [`#1287`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1287) (2026-08-23, mattina) | **filtro sul dominio**: se non puoi colpire **e non vedi già nessuno**, restano solo le celle da cui si vede | **oscillazione di periodo due** — il filtro è acceso quando sei cieco e spento appena vedi, quindi la cella cieca torna candidata nello stesso istante. Misurato: otto alternanze in dodici turni, e la partita che non si decide in quaranta |
| [`#1296`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1296) (2026-08-23, sera) | **niente filtro**, e l'avvicinamento misurato in **passi sul grafo** invece che in linea d'aria | **parcheggio cieco** — una cella senza linea di tiro può essere l'ottimo globale. Misurato su Gadget al turno 5: `standoff 3`, distanza 4, LoS assente, `resta (-1,1,L0) score = -10` contro `-20` della migliore fra le **24** celle che vedono. Sette turni fermi senza sparare |

🔴 **La giustificazione con cui il filtro è stato tolto è vera solo a metà, e la metà falsa è il difetto.**
Diceva: *«una cella dietro un muro adesso è lontana, e il punteggio la scarta da solo»*. Vale per i blocchi
al **passo**; non vale per i blocchi alla **vista**, dove la metrica non mente affatto — Gadget è davvero a
quattro passi — e semplicemente non vede. `ArenaV01` ha entrambi: la barriera centrale ferma passo e vista,
lo schermo meridionale `(2,1,0)`/`(3,1,0)` ferma **solo la vista**, e `ERTHexSurface::Smoke` ha la stessa
forma con `MoveCost` 1.

⚠️ **I due difetti non sono lo stesso difetto con due nomi.** Sulla mappa d'autore Gadget è inerte circa
sette turni su dodici in **entrambe** le versioni: con il filtro lo esprime muovendosi fra tre celle, senza
filtro stando fermo. `Match.Autobattle.NobodyParksOnTheAuthoredMap` vede solo la seconda forma,
`Match.Autobattle.NobodyOscillatesOnTheAuthoredMap` solo la prima — e **nessuna delle due configurazioni
passa entrambi**.

**Una terza forma esiste, ed è misurata sulla carta ma non implementata**: un **termine di punteggio sulla
destinazione** — penalità `WBlind` per una cella senza linea di tiro verso un contatto noto, applicata
quando il piano non contiene un attacco. Non oscilla perché guarda **dove vai**, non **da dove parti**:

    Gadget T5          cieca -10 - W    vede -20      con W > 10 si sposta e spara
    Riktor su (1,-1)   cieca -20 - W    vede -36      con W > 16 sale sulla piattaforma
    Riktor sulla piattaforma            vede -36      resta: nessun ciclo

E conserva ciò che `#1287` aveva comprato — attraversare una zona cieca per **avvicinarsi** resta possibile
quando accorcia di più di `W / WApproach` passi, cioè è una scelta tarabile invece di un divieto.

⛔ **Ma è un peso nuovo nel bot**, e `#1287` aveva scartato per iscritto l'approccio *«aggiungere un secondo
punteggio»* a favore della restrizione del dominio. Rovesciare quel giudizio con l'evidenza è legittimo;
farlo di lato dentro una PR di correzione no — il bilanciamento del bot ha la sua sede in
[`#149`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/149) e [D-102](decisions/RT_PDR_00_Decision_Log.md).

✅ **Implementata e misurata il 2026-08-24 — otto forme, e nessuna gratis**
([#1300](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1300#issuecomment-5393465886)).
🔴 La riga qui sopra dice *«misurata sulla carta ma non implementata»*, ed è **falsa da quella data**: la
forma «termine» è stata scritta e rimisurata otto volte sullo stesso banco a modalità — `WBlind` compresa —
e ciò che la carta non poteva vedere è **dove** cade.

| forma | esito sui sette oracoli |
|---|---|
| `WBlind`, penalità sulla cella cieca | mappa d'autore ✅, ma **tre** rossi: le due di copertura e l'arena generata |
| lo stesso **col segno invertito** (bonus a chi vede) | salva `PlanUnitSeeksCover`, restano rossi `ScoreThreatRespectsCover` e l'arena generata |
| **intento derivato** in `ChooseBestPlan` (nessuna candidata attacca) | copertura intatta, ma parcheggio **6** su limite 4 |
| penalità solo dove la cecità **non è copertura** | nessun effetto misurabile su queste due mappe |
| simmetrico di §3c sulla **propria gittata** | copertura rotta e scenario `ArenaV01` che non si decide più in 40 turni |
| **differenziale** (bonus solo a chi cambia cella) | i sette oracoli passano con `W` fra **11 e 16** — ma la suite intera **scambia** il rosso: cade `HexBot.ScoreKiterVsMelee` |
| bonus solo se si **guadagna** vista | è la condizione di `#1287` in forma di peso, e si comporta come `#1287`: non compra la deviazione, e oscilla |
| **memoria per unità**: bonus che **decade** con i turni senza ingaggiare | ✅ l'unica che fa passare **i due oracoli di parcheggio insieme** — sei su sette a `W=15 D=5` e a `W=20 D=10`. Sulla suite intera due rossi, entrambi su **valori assoluti** e nessun ordinamento (`0 → +15`, `−10 → +5`, `−20 → −5`) |

Quattro reperti che cambiano il **costo** della decisione, non la decisione:

1. **`HexBot.ScoreThreatRespectsCover` pinna due zeri** — la cella coperta *e* la cella esposta fuori dalla
   gittata nemica valgono entrambe `0` — quindi un termine posizionale sulla linea di tiro ne sposta uno
   **qualunque sia il segno**. Rispondere «termine» costa la riscrittura di quel test, e quel costo la riga
   `BOT-1` non lo elenca.
2. **Un termine guardato da `AttackRange > 0` sarebbe morto in partita.** `Ctx.AttackRange` non viene mai
   assegnato sul contesto con cui `PlanBots` chiama `ChooseBestPlan`/`ScorePlan` — le due sole occorrenze
   sono su `LocalCtx` — e per le candidate di **movimento** vale `0` per costruzione, perché il Move viene
   dopo il Blast. Sarebbe acceso solo nei test unitari: suite verde, zero mosse cambiate.
3. **Per il termine posizionale la finestra del peso è vuota**, misurata intero per intero: l'arena
   generata cade **da `W = 7`** — dove `WElevation × MaxLayer + W < WApproach` lo prevede — e la mappa
   d'autore si sblocca **da `W = 11`**. Fra 7 e 10 sono rossi **entrambi**.
4. **La linea di tiro è simmetrica solo dentro un layer.** Su `DA_HexMap_Arena`: **0** asimmetrie su 2016
   coppie dello stesso piano, **91** fra piani diversi, e dalle tre celle della piattaforma L1 si vedono
   **64 celle su 64** contro le 28–36 degli spawn a terra — `HexLine` costruisce la linea sul layer del
   tiratore. È lì che ogni forma posizionale manda il bot, e da lì non spara.

🔴 **E il quinto reperto è quello che sposta la domanda verso `E26`.** Il termine posizionale **senza
memoria** non fa passare i due oracoli di parcheggio a **nessun** peso; il differenziale li fa passare
pagando il **movimento** invece dell'ingaggio, e infatti inverte un ordinamento di `ScoreKiterVsMelee`. La
forma con **memoria per unità** — un bonus che decade con i turni senza ingaggiare, cioè il campo di stato
che [`E26`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/326) porterebbe — li fa passare
entrambi, e i due rossi che lascia sulla suite intera sono **solo valori assoluti**: nessun ordinamento
misurato si inverte. ⚠️ Ma **non gioca meglio**: sulla mappa d'autore i colpi calano da 17 a 13 e lo
scenario `ArenaV01` passa dal turno 19 al 21. Passa gli oracoli, non li batte.

⚠️ **La decisione resta aperta**: la misura dice quanto costa ciascuna via, non quale prendere. In
particolare `BOT-2` cresce di un parametro se la risposta è «memoria»: **peso e decadimento**, e i quattro
punti misurati dicono che non è il loro rapporto a decidere.

✅ **Chiuse da [`D-185`](decisions/RT_PDR_00_Decision_Log.md)**: il termine entra nel punteggio **con
memoria** — `Score += max(0, WEngage − WEngageDecay × IdleTurns)` sulle celle che vedono, nei piani senza
attacco — con `IdleTurns` = turni consecutivi senza *pianificare* un colpo. `WEngage = 15`, `WEngageDecay = 5`.
Owner: [`gameplay/spec-bot-hex.md`](gameplay/spec-bot-hex.md) **§3e**, che prima non nominava la linea di tiro.
Le due righe qui sotto restano com'erano scritte, con la risposta accanto: la domanda che sono state vale più
del fatto che ora abbiano un numero.

| ID | Domanda | Perché non si deduce — e come si è chiusa |
|----|---------|----------------------|
| `BOT-1` | La capacità di **ingaggiare da una cella** entra nel punteggio come **termine** (`WBlind`), oppure resta una **restrizione del dominio** delle candidate? | Sono due modelli diversi, non due implementazioni della stessa cosa. **Termine**: componibile con gli altri, tarabile, e permette lo scambio *«vado cieco perché accorcio di tre passi»* — al prezzo di un peso in più da bilanciare, e `#1287` lo aveva scartato. **Dominio**: nessun peso nuovo e nessun bilanciamento da rifare, ma è una scelta binaria che non sa esprimere quello scambio — ed è la forma che ha prodotto l'oscillazione. ⚠️ Nessuna delle due si ricava dai documenti: la spec owner [`gameplay/spec-bot-hex.md`](gameplay/spec-bot-hex.md) §3d elenca i termini del punteggio e **non nomina la linea di tiro** fra di essi, quindi non dice se appartenga a quella lista o al filtro a monte. ⚠️ **Dalla misura del 2026-08-24**: entrambe le opzioni sono state implementate, e nessuna passa i sette oracoli — ma è emerso un **terzo asse che la domanda non contiene**, *posizionale* contro *differenziale*, cioè se il termine paga lo **stare** in una cella o l'**entrarci**. Solo il differenziale passa i sette oracoli, e lo fa pagando il **movimento** invece dell'ingaggio. ✅ **Risposta: TERMINE, ma su un terzo asse che la domanda non conteneva** — non *posizionale* contro *dominio*, bensì **con memoria** contro *senza*. Il termine posizionale non passa i due oracoli di parcheggio a nessun peso; il differenziale li passa ma **inverte un ordinamento pinnato**; la memoria li passa entrambi e non inverte nulla. `D-185` |
| `BOT-2` | Se la risposta a `BOT-1` è «termine»: quanto vale `WBlind`, e **chi lo pinna**? | I due casi misurati danno un limite inferiore — `> 16` per coprire entrambi — e nient'altro. Il limite superiore è una scelta di gioco: più alto è, meno il bot accetta di attraversare una zona cieca per chiudere la distanza, che è precisamente il comportamento che `#1287` è andato a comprare. ⚠️ **E c'è un secondo ordinamento già mosso e non pinnato**: da `#1296` `MinDist` non è più limitato dal raggio della mappa, quindi `WApproach × MinDist` può superare `WThreat` — prima non poteva, e nessun test lo verifica. Un peso nuovo entra in una scala che ha appena smesso di avere un tetto noto. ⚠️ **Il limite superiore non è più solo una scelta di gioco**: misurato il 2026-08-24, per il termine **posizionale** non esiste alcun valore che passi entrambi gli oracoli di parcheggio — arena generata rossa da `W = 7`, mappa d'autore verde solo da `W = 11` — mentre per la forma **differenziale** la finestra è `11 ≤ W ≤ 16`, con numeri identici in tutto l'intervallo. Il limite inferiore *«> 16»* di questa riga viene dai due casi sulla carta; sull'oracolo la soglia misurata è `W = 11`. ✅ **Risposta: `WEngage = 15` e `WEngageDecay = 5`, e a pinnarli è `HexBot.EngageBonusFadesWithIdleTurns`** — che misura l'**esito** di `ChooseBestPlan` fresco contro inerte, non il punteggio di un piano isolato. ⚠️ **Il peso è diventato due**, e non è il loro rapporto a decidere: `15/5` ✅ e `20/10` ✅ contro `20/5` 🔴 e `30/10` 🔴. La taratura fine resta bilanciamento, #149 / `D-102`. `D-185` |

**Cosa bloccava**: la PR `#1297` lasciava `Match.Autobattle.NobodyParksOnTheAuthoredMap` **rosso** (7 turni
fermi su limite 4) e non era mergiabile finché `BOT-1` non fosse decisa. ✅ Sbloccata da `D-185`. `#959` (CP 47.6) resta eseguibile solo
sul free-run in pareggio: lo scenario `AutoBattle.ArenaV01` arriva alla vittoria **con** il fix di `#1296`,
e senza torna a non decidersi.

Tracciata su GitHub: [`#1300`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1300)
(`question`) — aperta **nello stesso commit** di questa voce, perché una decisione aperta che vive solo in
un documento non entra in nessuna coda di lavoro.

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
[`technical/spec-graybox-placement-contract.md`](technical/systems/spec-graybox-placement-contract.md).

Il kit decide la **grammatica** e non tutti i numeri, e lo dichiara esso stesso: *«la sessione ha deciso la
grammatica, non necessariamente tutti i numeri finali»*. **Due** voci restano aperte, e condividono la
stessa ragione — **si validano guardando**, e la scena in cui guardarle non esiste ancora: la frazione del
Safe Placement inset (`GBX-1`) e l'ingombro dell'unità rispetto alla cella (`GBX-5`). Entrambe hanno lo
stesso innesco, la seduta **U25**. Le altre tre — la lacuna di grammatica su `Locked` (`GBX-2`), le soglie
di lettura dell'integrità (`GBX-3`) e il percorso di `Content/` (`GBX-4`) — sono ✅ **CHIUSE il 2026-08-18**
da [`D-171`](decisions/RT_PDR_00_Decision_Log.md), [`D-172`](decisions/RT_PDR_00_Decision_Log.md) e
[`D-173`](decisions/RT_PDR_00_Decision_Log.md), e stanno nella sezione dedicata più sotto. La sesta —
**due scale che divergevano di 1,5×** — è ✅ **CHIUSA il 2026-08-17** da
[`D-163`](decisions/RT_PDR_00_Decision_Log.md) e sta anch'essa in una sezione propria. **La tabella qui
accanto ne ha due, e questo è il conto** — le quattro chiuse non vi compaiono, per la ragione scritta in
testa a ciascuna sezione di chiusura.

> 🔴 *Ennesima ricaduta — le note qui sotto ne registrano altre tre, e questa non è «la terza»: contarle è
> proprio l'esercizio che continua a fallire.* Il 2026-08-17, chiudendo `GBX-6`, questa riga è rimasta a
> **«Sei»** mentre ne elencava cinque e la conclusione della sezione diceva già «cinque»: la stessa
> direzione dei casi precedenti — conclusione aggiornata, apertura no — non l'immagine speculare, come una
> stesura di questa nota sosteneva. Nessun gate legge un numero in prosa,
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
| `GBX-5` | Quanto deve essere grande l'**unità** rispetto alla cella — e i `1,20 m` di oggi sono uno **stato** o un **target**? | 🆕 Aperta il 2026-08-17 da [D-158](decisions/RT_PDR_00_Decision_Log.md). Tre documenti dello stesso bundle danno tre valori: il kit dice **`0.23 C`** (≈60 cm con la scala d'arte), l'handoff dice **`70–80 cm`**, l'infografica dice **`1,10–1,20 m`**. La misura scioglie *quale sia vero oggi* e non *quale sia giusto*: `BaseMeshScale = (1.2, 1.2, 1.8)` su un cilindro engine da 50 uu di raggio dà **120 uu**, cioè il valore dell'infografica — che quindi **fotografa il presente e lo etichetta «consigliato»**. ⚠️ **Il kit chiedeva l'opposto**, in lettere: *«i cilindri-unità devono essere visivamente più piccoli di quanto sono oggi»*, per lasciare spazio leggibile a cover, path, facing e superfici. ⚠️ **Il rapporto si misura nella scala in cui le mappe girano, non in quella d'arte**: nessuna mappa sovrascrive `HexSize` — misurato in [`D-163`](decisions/RT_PDR_00_Decision_Log.md), che possiede l'istruttoria e l'oracolo con cui è stata fatta — quindi la cella era larga `173 uu` e l'unità ne occupava il **69%** — contro il **23%** che il kit propone, un fattore di **circa tre**. 🔴 **E `D-163`, deciso lo stesso giorno, cambia il denominatore**: a `HexSize = 150` la cella è larga `260 uu`, l'unità ne occupa il **46%** e il fattore scende a **due**. 🔵 **E la scelta di lasciare `LayerHeight` a `250` lavora nella stessa direzione**: le altezze non si muovono — su `H` sono invarianti, ed è [`D-168`](decisions/RT_PDR_00_Decision_Log.md) a fissare che è `H` l'asse su cui si misurano — mentre la cella si allarga, quindi **ogni silhouette guadagna spazio libero intorno a sé**. È lo stesso effetto che questa voce chiede per l'unità, ottenuto sul denominatore invece che sul numeratore: chi prende `GBX-5` decide quanto **ancora** serve toccando `BaseMeshScale`, non se serve tutto. Il numeratore — `120 uu` — non si muove, perché non dipende da `HexSize`. ⚠️ **Chi prende `GBX-5` a U25 usi `46%` e il fattore `2`**, non i valori pre-decisione: sono la stessa misura in due mondi, e quello che conta è quello in cui la si andrà a guardare. ✅ **[`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155) è atterrata il 2026-08-25, e questa riga è stata riscritta al solo `46%`** come prescriveva. Il gioco gira a `HexSize = 150`: la cella è larga `260 uu`, l'unità ne occupa il **46%**, e il fattore rispetto al `23%` del kit è **due**. *(Fino a quel giorno la riga portava anche il `69%` misurato a `HexSize = 100` — erano due misure vere in due momenti, e ora ne resta una sola.)* *(La prima stesura scriveva «46%» e «la metà». Il numeratore — `120 uu` — **non dipende da `HexSize` affatto**: l'errore era prendere per denominatore la **scala d'arte** rispondendo a una domanda sulle **mappe reali**. Trovato in code review; e la diagnosi «due scale nella stessa frazione», scritta al primo tentativo di correzione, mandava a cercare sul numeratore un fattore che non c'è.)* **Perché non si deduce**: è una scelta di leggibilità che si valida **guardando**, ed è precisamente ciò che la seduta **U25** esiste per fare — la stessa natura di `GBX-1`. ⛔ **E non si chiude cambiando `BaseMeshScale` e basta**: quel valore ha consumatori in `RTUnit.{h,cpp}`, quindi la modifica appartiene a `RT-FEAT-CHAR-PRESENTATION` e non a chi modella. 🔴 **Ma nessun test lo protegge, ed è il contrario di quanto questa riga diceva prima**: la prima stesura citava `Unit.RingClearsCellDisc` come guardia, e quel test **non legge `BaseMeshScale`** — usa `90.f` letterale contro una faccia del disco scritta a mano. Il legame fu **reciso di proposito** da `#593`, che rese la clearance dell'anello una costante invece di un prodotto. `Unit.RootIsNeutral` verifica solo che la scala del segnaposto non sia unitaria: portare `BaseMeshScale` a `(0.6, 0.6, 1.8)` lascia **tutta la suite verde**. Chi prende `GBX-5` non ha una rete, e saperlo è il vero rischio. Innesco: **U25**, insieme a `GBX-1` |

**Le due che restano non bloccano il consolidamento**: il contratto dice *che forma* devono avere gli
asset, e non cambiano quella forma.

⚠️ **Una sesta voce c'era, `GBX-6`, ed era l'unica che bloccava**: decideva in quale scala si modella. È ✅ **chiusa** — vince la scala d'arte — e sta nella sezione dedicata più sotto. 🔴 **Ma chiusa la decisione, il blocco resta**: la condizione bloccante non è mai stata «la domanda è aperta», era «un asset autorato oggi risulta 1,5× fuori misura sulla mappa in cui atterra», e quella era vera finché [`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155) non atterrava. ✅ **È atterrata il 2026-08-25**: `HexSize` è `150` in `RTHexMapAsset.h` e `RTHexMapActor.h`, quindi un asset modellato alla scala d'arte è ora in misura sulla mappa in cui si posa, e **questo blocco è sciolto**.

> ⚠️ *La prima stesura diceva «nessuna delle sei», ereditando la frase da quando le voci erano quattro e
> nessuna toccava la forma. Aggiungere una riga alla tabella senza rileggere la conclusione è il difetto
> che le due note qui sotto registrano — terza volta nella stessa sezione.*

✅ **Le due che andavano chiuse prima di produrre lo sono**, ed erano il cancello vero di questa sezione:
`GBX-2` perché modellare la porta senza sapere come si distingue `Locked` violerebbe `D-146` **all'atto** —
ora è una traversa in rilievo sul pannello, [`D-171`](decisions/RT_PDR_00_Decision_Log.md) — e `GBX-4`
perché la riga d'allowlist viene **prima** dell'asset, o `git add` tace e il lavoro resta locale: il
percorso è `/Game/RT/World/Graybox/` e la riga in `.gitignore` è stata scritta nello stesso commit della
decisione, [`D-173`](decisions/RT_PDR_00_Decision_Log.md).

⏱️ **Ma «si può modellare» non è «si può committare»**, e il blocco che resta non è in questa tabella: è
quello registrato qui sopra. ✅ **Sciolto il 2026-08-25**: fino all'atterraggio di [`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155)
un volume autorato era corretto per il canone e **1,5× fuori misura** per la mappa in cui si
posa — quindi si modella alla scala nuova e si **rimanda il commit**, non il lavoro.

> ⚠️ *Questo paragrafo diceva che «tre delle quattro riguardano un numero o un percorso che si fissa quando
> il primo asset viene prodotto», ed era il gemello non corretto della frase in testa alla sezione: falso
> per `GBX-4` — che la sua stessa riga smentisce in grassetto — e per `GBX-3`, che non ha più un innesco da
> aspettare. Correggere l'apertura di una sezione e non la sua chiusura è lo stesso difetto due volte nello
> stesso testo.*

Tracciate su GitHub: [`#1094`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1094) — (`question`), aperta **nello stesso commit** di questa voce e oggi ridotta alle **due** rimaste, dopo che `GBX-6` è stata chiusa da [`D-163`](decisions/RT_PDR_00_Decision_Log.md) e `GBX-2`/`GBX-3`/`GBX-4` da [`D-171`](decisions/RT_PDR_00_Decision_Log.md)/[`D-172`](decisions/RT_PDR_00_Decision_Log.md)/[`D-173`](decisions/RT_PDR_00_Decision_Log.md) — `GBX-5` e `GBX-6` vi erano state aggiunte il 2026-08-17, per la ragione che `RNG-1`/`RNG-2` hanno già
scritto: una decisione aperta che vive solo in un documento non entra in nessuna coda di lavoro.

---

## ✅ Chiusa il 2026-08-17 da `D-163` — la scala d'arte governa anche il mondo

Nata come `GBX-6` nella sezione qui sopra, **aperta e chiusa lo stesso giorno**: l'ha resa visibile il
consolidamento del bundle `GrayToolkit` e l'ha decisa l'autore poche ore dopo. Sta qui e non fra le aperte
perché una riga barrata in una tabella di domande si conta lo stesso quando qualcuno le enumera.

| ID | Domanda | Esito, e l'istruttoria che ci è arrivata sotto |
|---|---|---|
| ~~`GBX-6`~~ | ~~La **scala d'arte** (lato 1,5 m) e la **scala di ogni mappa esistente** (lato 1,0 m) divergono di 1,5×. Quale delle due governa?~~ | ✅ **Vince la scala d'arte**: lato `1,5 m`, `HexSize = 150` ([`D-163`](decisions/RT_PDR_00_Decision_Log.md)). ✅ **E l'atterraggio c'è stato**: [`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155) è mergiata il 2026-08-25, `HexSize = 150` in `RTHexMapAsset.h` e `RTHexMapActor.h`. Fino a quel giorno la decisione era presa e il mondo girava ancora a `1,00 m` — la distinzione fra decidere e atterrare resta il punto di questa voce, e ora entrambe sono fatte. *Aperta e chiusa nello stesso giorno; la misura sotto resta perché è ciò che ha reso decidibile la domanda.* 🆕 Aperta il 2026-08-17 da [D-158](decisions/RT_PDR_00_Decision_Log.md), ed è la domanda che il bundle `GrayToolkit` ha reso visibile senza porla. `convenzioni-contenuti-ue.md` §11-bis fissa il **lato a 1,5 m** come scala d'arte dal 2026-08-09; misurato, **nessuna mappa la usa** — misurato in [`D-163`](decisions/RT_PDR_00_Decision_Log.md) — `HexSize` e' un `UPROPERTY` e il valore vive dentro i `.uasset`/`.umap`, che un `grep` su `Scenarios/` e `Config/` non apre, e il default resta `100.f` in `RTHexMapAsset.h` e `RTHexMapActor.h`. **Conseguenza concreta**: una copertura bassa modellata a `0.28 C` con `C = 2,60 m` era alta 73 cm, e su una mappa reale (cella 1,73 m) copriva il **42%** invece del 28% budgetato. ⏱️ *Esempio dell'epoca: [`D-168`](decisions/RT_PDR_00_Decision_Log.md) ha poi spostato le altezze da `C` a `H`, quindi oggi la guida bassa è `0.28 H` = 70 cm. La divergenza che questa riga descriveva resta reale — cambia solo il denominatore con cui la si misura.* ⚠️ **Non si chiude scegliendo il numero più bello**: alzare `HexSize` a `150` cambia il mondo sotto ogni mappa e ogni test che misura in unità Unreal; lasciare `100` significa che §11-bis descrive una scala che nessuno usa. **Nessuna delle due è gratis**, ed è per questo che è una decisione e non una correzione. ⛔ E non era del contratto d'ingombro: l'owner della scala è `convenzioni-contenuti-ue.md` — che il 2026-08-17 **non era assegnato a nessuna track**, la stessa condizione di `GBX-4`, e che `D-163` ha portato in `integration_only` proprio per poterci scrivere la risposta |


---

## ✅ Chiuse il 2026-08-18 da `D-171`, `D-172`, `D-173` — le tre del contratto graybox che non aspettavano di guardare

Nate come `GBX-2`, `GBX-3` e `GBX-4` nella sezione qui sopra, chiuse su [`#1094`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1094).
**Stanno qui e non fra le aperte** per la stessa ragione di `GBX-6`: una riga barrata in una tabella di
domande si conta lo stesso quando qualcuno le enumera.

⚠️ **Ciò che le separava dalle due rimaste non è l'importanza, è l'oracolo.** `GBX-1` e `GBX-5` si validano
**guardando** e la scena non esiste ancora — è la seduta **U25** — mentre queste tre avevano già sul
repository tutto il materiale per essere decise: un enum a quattro stati, due costanti di catalogo e una
sezione normativa con quattro famiglie e nessuna che le contenesse. *Restavano aperte perché nessuno le
aveva prese, non perché mancasse qualcosa.*

| ID | Domanda | Esito, e l'istruttoria che ci è arrivata sotto |
|---|---|---|
| ~~`GBX-2`~~ | ~~Quale **canale non cromatico** distingue una porta `Closed` da una `Locked`?~~ | ✅ **Una traversa in rilievo modellata sul pannello: il marcatore è geometria** ([`D-171`](decisions/RT_PDR_00_Decision_Log.md)). `Locked` è una mesh **diversa** da `Closed`, non la stessa ricolorata — quindi nessuno stato di visibilità da guidare e nessun secondo componente da tenere allineato al pivot. Scartate le altre due opzioni di design: un catenaccio come mesh separata costava un asset più una regola di visibilità per stato; un'icona d'overlay spostava il canale nella **UI**, legando il kit a **E20** che ha un owner diverso. ⚠️ **Costo accettato**: l'elemento **#8** del catalogo di §8 passa da una mesh a **due**, e il conto degli elementi resta diciannove — §8 classifica elementi, §8.1 elenca path. *(La prima stesura scriveva «il catalogo guadagna una voce» e non la applicava: trovato in code review su [`#1188`](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1188).)* *L'istruttoria che l'ha resa decidibile*: era il caso che rompeva `D-146`, ed è un contributo dell'audit e non del kit — il kit conosceva **tre** stati di porta, `ERTHexDoorState` ne ha **quattro**. `Closed` e `Locked` **negano entrambi il passaggio** e hanno la stessa geometria: la sola differenza è che il secondo non si apre. Col solo colore a separarli, «mai solo il colore» sarebbe stata violata **dal primo asset prodotto**. ✅ **Conseguenza a valle**: `PIE-GBX-DOOR`, **proposta** in [`#1096`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1096), era scritta per **escludere** `Locked` finché questa voce fosse aperta — tre stati su quattro — e ora può includerlo. ⚠️ *Sblocca il **contenuto** della voce, non la sua esistenza: nel registro manuale quella voce non c'è — `grep -c "PIE-GBX" docs/technical/test-manuali-pie.md` dà **0** (dalla radice del repository, come l'oracolo di `FMT-2` più in alto in questo file: il path relativo a `docs/` che questa riga portava fino al 2026-08-19 esce **`2`** con *«No such file or directory»*, cioè un fallimento invece del risultato dichiarato), ed è la ragione per cui `#1096` è aperta. La prima stesura scriveva «ora li copre tutti», che presuppone una voce che nessuno ha ancora potuto scrivere. Trovato in code review.* |
| ~~`GBX-3`~~ | ~~A quali valori di `Integrity` corrispondono **«danneggiato»** e **«critico»**?~~ | ✅ **A nessun valore: a una FRAZIONE del catalogo** ([`D-172`](decisions/RT_PDR_00_Decision_Log.md)). In aritmetica intera, senza float e senza arrotondamenti da concordare — e i predicati **non sono mutuamente esclusivi**, quindi l'ordine è parte della regola, e il **dominio è una entry di `Covers`**: si legge dall'alto e il primo che regge vince. `1. critico ⟸ Integrity * 3 <= DefaultIntegrity(Type)` · `2. danneggiato ⟸ Integrity < DefaultIntegrity(Type)` · `3. intatto ⟸ altrimenti`. 🔵 **EMENDATA da [`D-186`](decisions/RT_PDR_00_Decision_Log.md) il 2026-08-24**: la seconda lettura si chiama **`ridotto`**. Le soglie non cambiano — cambia ciò che le tre dichiarano di significare: **forza relativa al catalogo del tipo**, non una storia di colpi. *«Danneggiato»* affermava una causa, ed era falsa per un pannello `Adaptive` appena eretto, che nasce a `25` contro un catalogo `30` perché la fragilità è il prezzo della rotazione gratuita. 🔵 **EMENDATA da `D-175` il 2026-08-19**: i predicati erano quattro e il primo, `distrutto ⟸ l'entry non è più in Covers`, non aveva dominio — su ogni bordo nudo reggeva, e iterando sulle sole entry esistenti era irraggiungibile; in più l'assenza dell'entry la producono anche la **scadenza** (`CoverExpired`) e lo **spostamento** di `Reconfigure` (`CoverMoved`), entrambi distinti da `CoverDestroyed`. «Distrutto» è ora la transizione del TurnLog, non una lettura del dato di mappa. I tre percorsi, con la funzione che rimuove l'entry e l'esito che ognuno logga, stanno in §7.2 dell'owner — non qui, e non in `D-175`. 🔴 **La prima stesura era difettosa in tre punti, trovati in code review su [#1188](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1188)**: non dichiarava l'ordine (e senza ordine `critico` è irraggiungibile, perché una `High` a `10` soddisfa anche `danneggiato`); ancorava `intatto` a un'**uguaglianza**, lasciando senza stato i valori sopra il catalogo che `Hero.Riktor.KineticPanel` produce oggi (`Integrity` **45** rinforzato su un default di `30`); e dava `distrutto ⟺ bDestroyed` come osservabile, mentre quel `bool` vive su una struct di **ritorno** ed è seguito da `RemoveAt` — a fase conclusa l'entry non c'è più, quindi `bDestroyed` è l'**evento** e non è osservabile dal dato di mappa. ⚠️ *Questo punto concludeva anche «quindi distrutto è l'**assenza**», e `D-175` l'ha corretto: l'assenza è prodotta **anche** dalla scadenza (`CoverExpired`), quindi non identifica la distruzione.* **Perché ⅓, e cosa la misura dimostra davvero**: `Action.HeavyAttack` fa `20` di `DamageStructure`, quindi le sequenze reali sono `High 50 → 30 → 10 → 0` e `Low 30 → 10 → 0`; con ⅓ «critico» cade **sull'ultimo passo prima di zero su entrambi i tipi**, cioè significa *un altro colpo e cade*. La misura **esclude ¼**, dove una `Low` a `10` resta «danneggiata» e cadrebbe senza mostrare lo stato più forte. ⚠️ **Ma NON seleziona ⅓ contro ½**: su quelle sequenze `* 2` classifica identicamente, e fra le due si prende la più stretta perché «critico» resti raro anche con colpi più leggeri di `20`. È design, non misura — e la prima stesura la vendeva come discriminante. ✅ **La frazione è ciò che la rende stabile al balance**: se `DefaultIntegrity` o `DamageStructure` cambiano, le letture seguono senza riscrivere la decisione. *L'istruttoria*: la voce era stata riscritta **due volte** il 2026-08-17 e la seconda perché la prima correzione aveva sbagliato i numeri — l'innesco era `CP 9.2`, chiuso dal 2026-08-07, e la citazione a sostegno era un'assertion su `FRTHexEdge`, cioè un **ponte**. I fatti veri: `DefaultIntegrity` dà **`50` per `High` e `30` per `Low`** — due partenze, ed è la ragione per cui «critico» non poteva essere un numero assoluto |
| ~~`GBX-4`~~ | ~~Sotto quale percorso di `Content/` vive il kit graybox degli **oggetti**?~~ | ✅ **`/Game/RT/World/Graybox/`, con `Cover/ · Doors/ · Surfaces/ · Volumes/`** ([`D-173`](decisions/RT_PDR_00_Decision_Log.md)). **Non `World/Grid/Graybox/`**: §5 descrive già `Grid/Generation/` come *«generatori graybox»*, e due significati di *graybox* a un livello di distanza si confondono al primo che cerca — inoltre porte e coperture stanno sui **bordi**, dove la direzionalità è del bordo e non della cella (`E9.1`), non sulla griglia. **Non un top-level `/Game/RT/Graybox/`**: il primo livello di `RT/` è organizzato per **dominio**, e «graybox» è un modo di fare gli asset — il giorno in cui il kit diventasse arte finale quel nome mentirebbe sull'intero albero, mentre sotto `World/` la promozione è un rename locale. ⚠️ **La riga d'allowlist è stata scritta nello stesso commit**, prima che un solo asset esista: [`technical/asset-map.md`](technical/tooling/asset-map.md) §6 lo prescrive e senza di essa `git add` **tace e non segnala nulla** — è lo stato di `ABP_Gadget`. Oracolo: `git check-ignore -q <file>` → exit **`1`**. *L'istruttoria*: [`technical/convenzioni-contenuti-ue.md`](technical/tooling/convenzioni-contenuti-ue.md) §5 è **normativa** e copre griglia, mappe, personaggi, UI e dati; un kit di primitive riusabili non è nessuna delle famiglie, e **§5b** dà il criterio che decide — *«se è usato da più mappe, va in una cartella condivisa»*. ⚠️ *Fino al 2026-08-17 quel file **non era assegnato a nessuna track** e questa riga dava lo STOP di `D-139` come la ragione per cui la voce era una domanda invece di un edit: era vero a metà — toglieva il permesso di scrivere la risposta, non la difficoltà di trovarla. `D-163` lo ha portato in `integration_only`, e quando qualcuno l'ha presa la domanda si è rivelata decidibile in un pomeriggio* |

✅ **Il motivo per cui nessuna delle tre rendeva committabile un asset è caduto il 2026-08-25**:
[`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155) è atterrata e il mondo gira a `HexSize = 150`, la stessa scala d'arte di
[`D-163`](decisions/RT_PDR_00_Decision_Log.md) con cui si modella. Le due scale coincidono.

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
| `AE-8` | Con quale **intensità di rumore** suonano le sei azioni generiche che D-041 non nomina — `Move`, `BasicAttack`, `Guard`, `Brace`, `Interact`, `Overwatch`? | [D-041](decisions/RT_PDR_00_Decision_Log.md) fissa la scala `0-10` e ne dichiara **tre** valori (`Wait 0 · Sprint 5 · Dash 6`, più «esplosione 10» che non è un'azione): le altre sei **non esistono in nessuna fonte corrente**, e il catalogo azioni le porta come `—` invece di inventarle ([#690](https://github.com/DegrassiAaron/refactor-tactics-main/issues/690), 2026-08-25). ⚠️ **Non è urgente e non blocca**: `FRTNoiseEvent::Intensity` non è assegnata da nessuna parte fuori dai test, quindi oggi nessun consumatore leggerebbe il dato. Diventa **esigibile insieme a [#159](https://github.com/DegrassiAaron/refactor-tactics-main/issues/159)** (CP 13.4), la cui casella `0` — *«la risoluzione EMETTE rumore»* — è tuttora aperta: se quel produttore arriva prima, hardcoderà le intensità e il numero avrà **due sedi**, che è il difetto che [D-023](decisions/RT_PDR_00_Decision_Log.md) e [D-115](decisions/RT_PDR_00_Decision_Log.md) hanno eliminato altrove. Gemella di `AE-5`, che pone la stessa domanda per il profilo `Sneak` |

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
| `GEO-3` | Il modello causale delle §22–§27 **non entra** nel v6: presuppone un `EventId` per voce, che non esiste. Tre rinvii con l'innesco dichiarato — cause contribuenti, dedup, provenance — e **una risposta piena**: un `EventId` sarebbe **identità**, quindi resterebbe fuori dall'hash come `UnitId` e `TurnNumber` | [`D-080`](decisions/RT_PDR_00_Decision_Log.md) · owner [`spec-turnlog.md`](technical/architecture/spec-turnlog.md) §12-bis |

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
| `FAC-9` | Il pathfinding deve diventare **orientation-aware** — stato `(CellId, Facing)` invece di `CellId`? | Fuori v0.1, e va tenuto fuori finché non c'è una misura: moltiplica per sei lo spazio degli stati di A\*, contro i budget già dichiarati in [`technical/spec-pathfinding.md`](technical/architecture/spec-pathfinding.md). Il ripiego dichiarato dall'handoff — path geometrico, facing derivato, pivot validato alla fine — **è** già il modello di ADR-0005. ⚠️ **Dal 2026-08-10 la pressione aumenta**: con `FAC-1` accettata la cella d'arrivo vale diversamente a seconda del lato da cui la si raggiunge, quindi la **preview** deve mostrare il facing ottenibile. Resta fuori v0.1, ma non più «senza motivo» |

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
| `INT-8` | Un'azione che opera su una **struttura** conta come un attacco su chi le sta accanto? | Aperta il 2026-08-27 dalla **misura** di [#1491](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1491), non da un documento: `Action.Interact` puntata su un'unità produce un colpo da **0** con forma `Single`, e il motore lo tratta come un attacco vero. I consumatori sono **due, non uno** — `FindTriggeringAttacker` per `HitByDirectAttack` chiede *«un colpo che bersaglia me»* più *«forma `Single`»* e **non guarda mai il danno** (`RTReactionLibrary.cpp:59-66`), quindi la reazione del bersaglio scatta e contrattacca; e lo stesso colpo paga `EnergyOnHit = 15` dal ramo `else` di `RTTurnManager_Blast.cpp:1919`, **incondizionatamente**, cioè anche quando il bersaglio non ha armato nulla. 🔴 **Non è decidibile abbassando la soglia sul danno**: un colpo da 0 è legittimo — `Action.MarkTarget` applica `Marked` esattamente così, e il commento a `RTHexCombatLibrary.cpp:356` lo dichiara (*«il colpo resta avvenuto, trigger e marchi contano lo stesso»*). Le tre strade **non hanno la stessa copertura**, ed è il punto della domanda: **(a)** *un'azione con `StructureOp` non produce colpi su unità* chiude reazione **e** energia usando un dato che esiste già; **(b)** *un campo nuovo `bCountsAsAttack`* ottiene lo stesso, ma aggiunge un dato da ricordarsi per ogni azione futura; **(c)** *un filtro dentro il trigger* ferma la reazione e **lascia in piedi** il generatore d'energia, che è la metà più sfruttabile. ⚠️ **Chi risponde noti che il puntatore ha già risposto**: `URTPointerLibrary::TargetKindForAction` classifica `StructureOp != None` come **bordo** e non come unità (`RTPointerInteraction.cpp:16`) — oggi resolver e interfaccia dicono due cose diverse sulla stessa azione, e l'anomalia da spiegare è quella, non il colpo. ⚠️ **Innesco e portata**: il giocatore non ci arriva, glielo impedisce il puntatore; ci arriva l'harness, e **forse il bot**, il cui pool d'attacco non filtra né su `Power` né su `StructureOp` (`RTTurnManager.cpp:1016-1021`) — se una candidata da zero danni vinca mai il punteggio **non è misurato**, ed è la misura che rende urgente questa riga invece che teorica. ⚠️ `Scenarios/Spec/Reaction/InteractCountsAsADirectHit.json` pinna il comportamento **attuale**: quando la risposta arriva, o si ribalta l'attesa e si aggiunge il tag `expected-fail`, o il file esce |

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
| ~~`BAS-5`~~ | ~~Dopo l'`Overwatch`: **Move con budget ridotto**, o **Watch Stage + Reposition** pianificato?~~ | ✅ **Chiusa come domanda il 2026-08-10**: prevale il modello **Watch → EndWatchStage → Reposition**. Non per la data — è la stessa — ma perché il sorgente gemello si dichiara superato su questo punto (§34 e §48 elencano «vecchio post-Overwatch Normal/Sneak Move» fra ciò da correggere), e perché è l'unico dei due modelli che dice **dove si trova** il personaggio quando l'Overwatch finisce. Restano aperti il suo **costo** (`OW-1`) e il suo **nome** (`OW-2`): vedi [`roadmap/plans/overwatch-runtime-lifecycle-triage-2026-08-10.md`](roadmap/plans/overwatch-runtime-lifecycle-triage-2026-08-10.md) |

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
| `BAL-1` | `Guard` e `Brace` devono separarsi in **danno contro spinta**? | 🔄 **RIAPERTA il 2026-08-27 da [D-205](decisions/RT_PDR_00_Decision_Log.md)**, e non per un ripensamento: `D-121` aveva fissato la divisione del lavoro fra le due difese **come se fossero due azioni dello stesso tipo**, tre giorni dopo che [D-047](decisions/RT_PDR_00_Decision_Log.md) aveva cambiato *che cosa e'* il `Brace` — un'azione che **arma un profilo di reazione**. Quando quel runtime e' atterrato, lo «status quo» che D-121 conferma ha smesso di descrivere qualcosa che esiste. D-205 scambia i due mestieri: la mitigazione **sostenuta** passa alla `Guard`, che la paga piantandosi (`Status.Slow`); il `Brace` smette di radicare. ⚠️ **La domanda non torna com'era**: non e' piu' *«separarli in danno contro spinta?»* ma *«ora che i mestieri sono scambiati, con quali numeri e con quale direzione?»* — vedi `BAL-2`, `BAL-3`, `BAL-4`. 🔴 **Riformulata di nuovo il 2026-08-27 da [D-208](decisions/RT_PDR_00_Decision_Log.md), e questa volta alla radice**: chiuse `BAL-2`, `BAL-4` e `BAL-5`, le due difese hanno **lo stesso identico prezzo** — principale, `Status.Slow`, niente slot reazione. Nessun costo le separa piu': la `Guard` e' **sostenuta e frontale** (120°), il `Brace` e' **per-evento** e **omnidirezionale** con un profilo d'eroe sopra. La domanda diventa **«la differenza di GENERE e' leggibile senza un costo che la annunci?»**, ed e' una domanda per la seduta, non per un documento. ⏳ **`#403` / `U20 · PIE-BAL1` non cade, si ripunta**: verificava se la differenza fra le due difese e' leggibile a schermo, e la differenza da giudicare e' cambiata. **Storico:** ~~✅ **Chiusa il 2026-08-12**~~ da [D-121](decisions/RT_PDR_00_Decision_Log.md), decisa dall'autore: **status quo**, nessuna separazione e nessuna magnitudine nuova. `Guard` resta difesa **front-loaded** sul primo impatto; `Brace` resta **sostenuta** sui colpi ripetuti e più robusta contro il displacement forte quando il contenuto lo produce. **Nessun rebalance numerico.** ⏳ **Resta il gate umano, non la scelta**: [#403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/403) è aperta solo per `U20 / PIE-BAL1`, che verifica se la differenza è **leggibile a schermo**. E l'ordine è vincolato: se non lo è si interviene prima su feedback/UI, poi — solo se non basta — sui numeri. ⚠️ Da tenere separata da `ECO-1` (*quanto costano*): se costo ed effetto cambiassero insieme il playtest non saprebbe quale dei due sta misurando. **Storico della misura che ha portato alla decisione:** [D-066](decisions/RT_PDR_00_Decision_Log.md) ha misurato il modello in vigore: entrambi fanno entrambe le cose, e differiscono per *forma* (primo colpo forte vs ogni colpo; spinta di 1 cella vs spinta qualsiasi). È bilanciamento: si chiude con una partita, non con un documento. ✅ **La Fase 0 è decisa** ([D-074](decisions/RT_PDR_00_Decision_Log.md), 2026-08-10, issue [#400](https://github.com/DegrassiAaron/refactor-tactics-main/issues/400)): si accetta che in v0.1 ogni spinta valga 1 e si **riscrive** la clausola «senza limite di distanza» invece di introdurre una spinta `≥ 2`. Conseguenza sulle opzioni ancora in campo: restano lo **status quo** e l'**ibrido** (separare le magnitudini); l'opzione *«`Guard` solo danno, `Brace` solo spostamento»* era **preclusa**, perché senza spinta forte lasciava `Brace` senza mestiere. 🔄 **Non più, dal 2026-08-11**: `Weapon.Impact` su `Riva.PressureJet` produce una spinta di **2** ([D-085](decisions/RT_PDR_00_Decision_Log.md)) ed è il default di Riva ([D-089](decisions/RT_PDR_00_Decision_Log.md)), quindi la spinta forte che `D-074` aveva scartato è arrivata dall'**equipaggiamento** invece che dal catalogo azioni. Misurato da `Equipment.PushTwoSeparatesGuardFromBrace`: contro una spinta di 2 **`Guard` cede e `Brace` regge**. Le opzioni tornano **tre**, con una domanda nuova — quel mestiere dipende da un equipaggiamento equipaggiato, non da una regola del turno. ✅ Gli scenari che servono a decidere esistono e sono verdi ([#401](https://github.com/DegrassiAaron/refactor-tactics-main/issues/401)): `Spec.Brace.GuardAndBraceOnMixedHit` e `Spec.Brace.BraceWinsOnSecondHit` pinnano il trade-off reale — *primo colpo pesante* (`Guard` 1 danno) contro *colpi ripetuti* (`Brace` 12 contro 17 su due colpi). ⏳ **Resta l'unica parte che richiede l'autore**: la seduta editor **U20** (voce `PIE-BAL1`) e la scelta fra le due opzioni superstiti. Roadmap e numeri: [`bal-1-guard-brace-roadmap-2026-08-10.md`](roadmap/plans/bal-1-guard-brace-roadmap-2026-08-10.md). Issue [#403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/403) (decisione) | <!-- rename-exempt: misura datata: riscriverla la renderebbe falsa -->
| ~~`BAL-2`~~ | ~~La `Guard` **piantata** copre a 360°, o resta frontale?~~ | ✅ **Chiusa il 2026-08-27** da [D-206](decisions/RT_PDR_00_Decision_Log.md), decisa dall'autore: **resta frontale** — 120°, 3 direzioni su 6 — e il controllo direzionale diventa **per-colpo**, che e' il contenuto operativo della voce: il gate di oggi guarda *«il PRIMO dell'array»*, criterio che una difesa sostenuta rende privo di significato. La ragione della scelta e' che la geometria e' **una sola**: `IsInFrontalArc` e `EffectiveCoverReduction` implementano la stessa regola CP 16.2, e una Guardia a 360° ne creerebbe una seconda. ➕ **Eccezioni per eroe ammesse nel kit** ([D-014](decisions/RT_PDR_00_Decision_Log.md)/[D-028](decisions/RT_PDR_00_Decision_Log.md)), con la forma gia' in uso di `URTHeroData::ReactionProfileId` — ma devono passare il test di [D-132](decisions/RT_PDR_00_Decision_Log.md): *«non era contenuto, era un nome»*. Nessun kit ne dichiara una oggi, e il campo si scrive quando un kit lo chiede. ⛔ Respinto l'arco variabile per stato: sarebbe stato decidere un numero credendo di decidere una struttura. **Storico della domanda:** Oggi due scenari pinnano la coppia opposta: `Spec.Facing.BackAttackIgnoresGuard` — *«la guardia copre il davanti»*, da fuori dall'arco frontale il −15 **non vale** — e `Spec.Facing.BraceHoldsFromBehind` — *«il `Brace` protegge la persona, non un lato»*. Con [D-205](decisions/RT_PDR_00_Decision_Log.md) la mitigazione sostenuta passa alla `Guard`, che e' **frontale**: *«sono piantato a incassare»* diventerebbe *«piantato, ma solo da davanti»*, e chi colpisce alle spalle non trova niente. ⚠️ **Le tre uscite non costano uguale**: renderla omnidirezionale fa **rosso `BackAttackIgnoresGuard`**, che e' **CP 16.2**; lasciarla frontale non muove nessuno scenario e fa della schiena il prezzo della durata; lasciare l'omnidirezionalita' al `Brace` la conserva a `Hold Ground` anche dopo l'accorciamento di [D-204](decisions/RT_PDR_00_Decision_Log.md). Da decidere **prima** di toccare il catalogo: e' l'unica delle tre che cambia uno scenario verde |
| `BAL-3` | Con quali **numeri** mitiga la `Guard` piantata, e quanto dura `Hold Ground`? | [D-205](decisions/RT_PDR_00_Decision_Log.md) decide **chi** fa il mestiere, non con quali valori: `GuardFirstHitReduction = 15` descrive una difesa front-loaded che non e' piu' quella, e il −10 *«su ogni danno fino al Cleanup»* diventa *«sul colpo a cui risponde»* con [D-204](decisions/RT_PDR_00_Decision_Log.md). I due numeri **non sono indipendenti**: sono il prezzo l'uno dell'altro, e presi separatamente rifanno l'errore che `BAL-1` ha gia' pagato. ⛔ [`balance/README.md`](balance/README.md) e [D-023](decisions/RT_PDR_00_Decision_Log.md) vietano di correggerli cella per cella: si chiude con una partita, non con un documento — ed e' la stessa riserva con cui D-121 era stata presa. Gemella di `AE-5` |
| ~~`BAL-4`~~ | ~~La `Guard` piantata nega anche la **reazione** (`bAllowsReaction = false`)?~~ | ✅ **Chiusa il 2026-08-27** da [D-207](decisions/RT_PDR_00_Decision_Log.md), decisa dall'autore: **sì**. Assorbire oppure rispondere, mai entrambi — il prezzo di una difesa **sostenuta** è l'iniziativa. ⚠️ **L'argomento contrario resta registrato nella voce**: la finzione punta nel verso opposto (*piantato e pronto* è la definizione di pronto), e la decisione toglie dal gioco `Guard + Counter`, che nessuno ha ancora giocato. Da rivedere se `U20`/`PIE-BAL1` misura un turno difensivo passivo. 🔴 **Due costi di implementazione dichiarati**: il valore del flag lo assegna oggi un `if` sull'ActionId dentro `ShippedAction`, e un secondo utente lo rende un **parametro** invece di allungare il predicato; e `ResolvePrep` **non legge** il flag, quindi senza quel ramo sarebbe dichiarato e mai applicato. **Storico della domanda:** Il meccanismo **esiste**, e' un dato di `FRTActionDef`, ed e' applicato in due punti del resolver (`ResolveDash`, il Blast) che scrivono in `ReactionBlockedThisTurn`. Oggi lo usa **una sola azione in tutto il catalogo core**: `Action.Sprint`. E il test che lo fissa scrive accanto il criterio che decide questa domanda — *«se tutte la negassero, il dato non distinguerebbe nulla»* (`Actions.Sprint.NoReaction`). ➕ **L'argomento a favore e' di identita', non di economia**: da [D-047](decisions/RT_PDR_00_Decision_Log.md) e' il `Brace` **l'azione che arma una reazione**, quindi una `Guard` che ne armasse una seconda farebbe il mestiere di entrambe. ⚠️ **Un costo da verificare prima**: `ResolvePrep` **non ha** il ramo che leggono Dash e Blast — `Guard` risolve in `Preparation`, quindi il flag sarebbe **dichiarato e non applicato** finche' quel ramo non esiste. Questa domanda e' la forma superstite della tesi *«la Reaction non e' gratuita»* del sorgente del 2026-08-26: ridotta da regola generale a **un booleano su un'azione**. Da tenere separata da `ECO-1` |
| ~~`BAL-5`~~ | ~~Ora che `ResolvePrep` legge `bAllowsReaction`, le altre tre azioni di `Prep` — `Brace`, `Shield`, `Overwatch` — lo lasciano a `true`?~~ | ✅ **Chiusa il 2026-08-27** da [D-208](decisions/RT_PDR_00_Decision_Log.md), decisa dall'autore, e la risposta è **diversa per le tre**: il `Brace` **no** (`false`), l'`Overwatch` e lo `Shield` **sì**. Il `Brace` perché un'unità con `Action.Brace` + `Action.Counter` risponde **due volte allo stesso colpo** — misurato: le cinque letture/scritture di `ReactionActivationsThisTurn` riguardano tutte lo slot, e il ramo del profilo non è fra loro. L'`Overwatch` **non per default** ma per una regola scritta nel catalogo §1 e per la dipendenza funzionale di [D-109](decisions/RT_PDR_00_Decision_Log.md), la cui condizione si dichiara **solo** attraverso lo slot reazione. ➕ La stessa voce porta il `Brace` da `Status.Root` a `Status.Slow`, **specificando** [D-205](decisions/RT_PDR_00_Decision_Log.md) che diceva solo *«smette di radicare»*. 🔴 **Conseguenza aperta**: `Guard` e `Brace` hanno ora lo stesso prezzo, e la scelta poggia solo sulla **forma** della mitigazione — vedi `BAL-1`. **Storico della domanda:** Nasce da [D-207](decisions/RT_PDR_00_Decision_Log.md), e **non è un rinvio**: il ramo che quella voce fa scrivere è **generico**, quindi il giorno in cui esiste il `true` delle altre tre smette di essere un'**assenza di meccanismo** e diventa una **scelta**. ➕ Il caso più concreto è l'**`Overwatch`**: [D-070](decisions/RT_PDR_00_Decision_Log.md) gli fa già pagare l'azione principale e riserva il movimento a `Withdraw`, e il sorgente del 2026-08-26 sosteneva che non debba *«stackare gratuitamente con una seconda reazione forte»* — tesi che finora non era verificabile perché il meccanismo non arrivava in `Prep`. ⚠️ **Il `Brace` è il caso opposto e va tenuto distinto**: da [D-047](decisions/RT_PDR_00_Decision_Log.md) arma un **profilo**, canale parallelo allo slot reazione, quindi negargli lo slot significherebbe togliergli una cosa che non usa. ⛔ **Il metro di questa famiglia è scritto** in `Actions.Sprint.NoReaction` — *«se tutte la negassero, il dato non distinguerebbe nulla»*: con la `Guard` gli utenti sono **due**, e ogni aggiunta va misurata contro quel criterio invece che decisa una per volta |
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
Il repository ha **undici** status implementati e **nessun framework** che li governi. Queste due vanno
decise **prima** di scrivere la spec owner, perché ognuna cambia la forma del dato, non un valore.

> ⚠️ *Diceva «**tre**», ed era vero quando la sezione è nata (`0d4ce475`): le voci erano `STA-1`, `STA-2` e
> `STA-3`. Poi [`D-072`](decisions/RT_PDR_00_Decision_Log.md) ne ha chiuse **due** e ne ha aperta una nuova
> — `STA-4`, che la riga qui sotto dichiara «aperta **da** `D-072`» — e il numero in prosa non ha seguito.
> Corretto il 2026-08-25. ⚠️ **«undici» non è stato toccato, ma va letto per quello che è**:
> `Core/RTGameplayTags.h` dichiara esattamente undici `TAG_Status_*` — contati — e *dichiarati* non è
> sinonimo di *implementati*. Almeno uno, `Status.Electrified`, non ha nessun sistema che lo applichi
> ([`#1324`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1324)). Chi volesse il numero
> degli status con un produttore deve misurarlo a parte: non è questo.*

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
| `WV-2` | Le **soglie delle fasce** di danno e i **delta per fascia** | [D-087](decisions/RT_PDR_00_Decision_Log.md) ha deciso il **principio**; i numeri sono `PROPOSED FOR PLAYTEST`. Baseline proposta: `Low 1–10` · `Medium 11–18` · `High 19+`, con `Precisione −2/−3/−4`, `Sovraccarico +3/+5/+6`, `Soppressione −2/−4/−5`. Si chiudono con una partita, non con un documento — e vanno misurati **dopo** che le fasce esistono nel codice. ✅ **Dal 2026-08-25 esistono** ([#509](https://github.com/DegrassiAaron/refactor-tactics-main/issues/509)): `ERTAttackDamageBand`, `DamageDeltaByBand` e la lettura in `ApplyWeaponVariant` sono in `Source/`, con le soglie della baseline. ⚠️ I **delta** restano da tarare — i tre valori di ogni variante sono oggi uguali fra loro, cioe' la struttura senza la scelta. 🔴 E per `Sovraccarico` c'e' una divergenza da sciogliere: D-087 propone `+3/+5/+6`, [D-090](decisions/RT_PDR_00_Decision_Log.md) decide `+8/+14/+18` — nessuna delle due e' entrata nel codice |
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
| ~~**Come si chiama la categoria di log del Decision Time Bank: `Decision` o `ReactionDecision`?**~~ | ✅ **Chiusa il 2026-08-17 da [D-166](decisions/RT_PDR_00_Decision_Log.md)**, dopo meno di un giorno di apertura — mentre lo **scarto fra spec e codice** che l'aveva prodotta durava da **tre** (`75039d93`, 2026-08-14). Esito: **`Decision` nasce distinta**, con `ERTDecisionOutcome` (`BankConsumed`, `BankAfter`, `BankExhausted`) e `Amount` in **millisecondi**; `ReactionDecision` resta con `Amount` in **danni**. Prevale [`technical/spec-turnlog.md`](technical/architecture/spec-turnlog.md) §4.2, che lo prescriveva dal 2026-08-09 (`#361`), ed è il caso particolare di [D-162](decisions/RT_PDR_00_Decision_Log.md) — *una categoria, un enum*. **L'istruttoria vive nell'owner** e non si duplica qui: [`gameplay/spec-decision-time-bank.md`](gameplay/spec-decision-time-bank.md) §10.1. ⚠️ Resta da **scrivere** l'enum, la voce in coda a `ERTLogCategory` e il `case` in `OutcomeEnumForCategory`: sono tre siti, non uno, ed è lavoro di CP 14.8 |

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
