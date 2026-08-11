# ADR-0009 — Il replay ha due prodotti: chi riproduce non calcola, chi verifica non presenta

> `CANONICAL` · **Stato**: Accettato — da implementare (R1/R3) · **Data**: 2026-08-10 · **Decisore**: utente (dev singolo)
>
> **Chiude**: issue [#412](https://github.com/DegrassiAaron/refactor-tactics-main/issues/412) · il punto 2 del §10 del
> [conflict report replay](../roadmap/plans/replay-system-conflict-report-2026-08-10.md)
> **Poggia su**: [D-062](RT_PDR_00_Decision_Log.md) (hash ordinato, e dove va) · [D-063](RT_PDR_00_Decision_Log.md) e
> [D-067](RT_PDR_00_Decision_Log.md) (cosa porta una voce) · [D-077](RT_PDR_00_Decision_Log.md) (manifest per partita + traccia per turno)
> **Non tocca**: il formato del TurnLog (`v7`), la sua forma canonica (`D-SR-1`) né alcun hash esistente

## Contesto

Il replay deterministico della v0.1 **esiste ed è verificato** — `CP 12.1`, `RT-FEAT-CORE-TURNLOG` è
`RELEASE_READY`, e il KPI «replay divergence = 0» è misurato. Ciò che non era dichiarato è **quale artefatto
sia autorevole** quando si riproduce una partita, e finché non lo è un recorder non sa cosa registrare e un
player non sa cosa gli è vietato ricalcolare.

La domanda sembrava una scelta fra due opzioni. Non lo è: il repository contiene già **entrambi** i
comportamenti, e li chiama con lo stesso nome.

- `RefactorTactics.Simulation.DeterministicReplay` (`RTSimulationDeterminismTests.cpp`) **non riproduce una
  traccia**: rilancia lo scenario 100 volte attraverso il resolver e confronta gli `StateHash`. Si chiamava
  «replay» ed è **ri-simulazione**. *(Dal 2026-08-11 si chiama `Replay.Verifier.ResimulationIsDeterministic`:
  [D-095](RT_PDR_00_Decision_Log.md), [#538](https://github.com/DegrassiAaron/refactor-tactics-main/issues/538).
  Il nome vecchio resta qui perché è la ragione per cui questo ADR è stato scritto.)*
- `URTReplaySeekLibrary` (`#415`) fa l'opposto: si posiziona dentro una traccia già scritta e non ha modo di
  chiamare il resolver.

Due cose diverse con lo stesso nome sono due cose che prima o poi qualcuno scambia. Questo ADR le separa.

## Decisione

### 0. Il replay è **logico**, e va detto perché nessuno l'aveva mai scritto

Il replay di RefactorTactics **non è un video** e **non è il network replay di Unreal**: è la
ricostruzione di una partita da `snapshot + intenti + decisioni + TurnLog + hash`. Il progetto lo pratica
da sempre — l'intero `CP 12.1` poggia su questo — ma nessun documento lo affermava, ed è la premessa senza
la quale il resto di questo ADR non si capisce: se il replay fosse un filmato, la domanda «chi può
calcolare» non esisterebbe.

### 1. Tre attori, e cosa ottiene ciascuno

| Attore | Goal | Cosa ottiene in v0.1 |
|---|---|---|
| **Giocatore** | rivedere la partita | riproduzione fedele della traccia; nessun ricalcolo, quindi nessuna possibilità che il replay «giochi diversamente» |
| **QA / diagnosi** | capire perché un turno è andato così | verdetto di equivalenza con **turno, fase e `ActionId`** della prima divergenza |
| **Verifica d'integrità** | sapere se una traccia è quella che dice di essere | ⚠️ **rilevamento di corruzione e di incompatibilità — non di manomissione**: vedi §5 |

### 2. Due prodotti, due autorità, perimetri disgiunti

**`ReplayPlayer` — autorevole la TRACCIA archiviata.** Riproduce ciò che è già stato risolto. Non calcola
collisioni, danni, reazioni, KO, legalità dei percorsi né targeting. Non produce esiti: li **legge**.

**`ReplayVerifier` — autorevole il RESOLVER.** Ri-simula e confronta con la traccia archiviata. Produce un
**verdetto**, mai una presentazione.

Le due garanzie sono diverse e vanno dichiarate diverse: il Player garantisce *«vedi quello che è successo»*,
il Verifier garantisce *«quello che è successo è riproducibile dalle regole»*. Sceglierne una sola avrebbe
mutilato l'altra.

**Classificazione di ciò che esiste già** — questo ADR non progetta da zero, nomina:

| Codice esistente | Lato |
|---|---|
| `URTReplaySeekLibrary` (`#415`) | **Player** |
| `DeserializeTurnLog`, `LoadTurnLogFromFile` | **Player** |
| `CompareSerializedTraces`, `DescribeFirstDivergence` | **Verifier** |
| `Replay.Verifier.ResimulationIsDeterministic`, `HashMatchState` | **Verifier** |

> ✅ **Corretto il 2026-08-11** ([#538](https://github.com/DegrassiAaron/refactor-tactics-main/issues/538),
> [D-095](RT_PDR_00_Decision_Log.md)). Il nome `Simulation.DeterministicReplay` insegnava il contrario di
> questo ADR — chi lo leggeva imparava che «replay» significa ri-simulare — ed è ora
> `Replay.Verifier.ResimulationIsDeterministic`. Il prezzo è dichiarato invece che nascosto: quel nome era
> uno dei dieci **vincolanti** del catalogo (p.24 §15), e il gate **G3** della v0.1 ora dice «nove con quei
> nomi, uno rinominato di proposito».

### 3. Il confine è reso impossibile dalla struttura, e il test è la rete

Il confine Player/Verifier è un **invariante**, non una raccomandazione. Ma un invariante scritto e basta è
il rischio `REPLAY-04` del risk register: *il player ri-risolve il gameplay, e nessun test attuale se ne
accorgerebbe*.

**Requisito**: il codice del Player vive dove il resolver **non è raggiungibile**. Non è un ideale: `#415`
l'ha già fatto — `URTReplaySeekLibrary` non include il resolver, quindi lì «il player non chiama il resolver»
non è disciplina, è **assenza della possibilità di violarla**. Un test d'architettura si aggira aggiungendo
un `#include`; una dipendenza che non esiste no.

**Rete secondaria**: un test che rende il confine osservabile. La forma più forte è **negativa** — il Player
riproduce una traccia in un contesto dove il resolver non è disponibile, e se funziona il confine è
dimostrato invece che affermato.

> ✅ **Scritti il 2026-08-11** in `RTReplayPlayerTests.cpp`, insieme al Player (`#470`, PR #496). Erano la
> condizione che questo ADR poneva, e con essi **`REPLAY-04` si chiude**: il rischio esisteva perché nessun
> test si sarebbe accorto di un player che ri-risolve, e adesso `Replay.Player.RunsWithoutResolver` lo
> dimostra nella forma negativa — riproduce dove il resolver non c'è.

### 4. Divergenza: il Verifier fallisce esplicito, il Player non verifica mai

Il Verifier gira **offline** — test, corpus golden, diagnosi — e una divergenza è un **fallimento
dichiarato**, con turno, fase e `ActionId`: `DescribeFirstDivergence` lo sa già fare, e
`CompareSerializedTraces` distingue già `FormatMismatch` e `TopologyMismatch` da una vera differenza di
contenuto.

Il **Player a runtime non verifica affatto**. È una conseguenza del §3, non una scelta a parte: verificare
richiederebbe il resolver accanto al Player, cioè il confine attraversato per costruire una difesa. E sarebbe
una difesa peggiore del problema — una riproduzione interrotta a metà lascia il giocatore davanti a uno stato
che nessuno ha finito di mostrargli.

Quello che il Player fa a runtime è **rifiutare in apertura** ciò che non sa leggere: versione di formato
sconosciuta, topologia incompatibile, checksum non valido. È la convenzione fail-closed che
`DeserializeTurnLog` applica già al formato — rifiutare invece di interpretare byte arbitrari.

### 5. Cosa la verifica d'integrità ottiene davvero, e cosa no

L'attore «integrità» è in scope, quindi va detto con precisione cosa gli si può promettere oggi.

Il formato porta dalla `v2` un **checksum FNV-1a a 32 bit** del payload. FNV non è una funzione crittografica:
chi modifica una traccia deliberatamente **ricalcola il checksum** e il file torna valido.

| Minaccia | Rilevata in v0.1? |
|---|---|
| Corruzione accidentale (disco, trasferimento, troncamento) | **sì** — checksum |
| Traccia di un altro formato o di un'altra topologia | **sì** — `CompareSerializedTraces` |
| Traccia riletta con regole diverse | **no, e la scelta è stata presa**: [D-083](RT_PDR_00_Decision_Log.md) rinvia `ContentManifestHash`/`RulesVersion` alla v0.2, perché in v0.1 il corpus golden vive nello stesso repository delle regole — il rilevamento c'è, manca l'**attribuzione** ([#413](https://github.com/DegrassiAaron/refactor-tactics-main/issues/413)) |
| **Manomissione deliberata** | **no** — servirebbe una firma, che non esiste |

Non è una lacuna da tappare adesso: la v0.1 è **2v2 offline contro un bot**, non c'è un avversario da cui
difendersi né un server che riceva tracce. La tamper-evidence è lavoro futuro, e l'archivio deciso da D-077 è
già il posto dove atterrerebbe — il **manifest** ha un header, ed è lì che una firma vivrebbe. Dichiararlo
adesso evita che qualcuno legga «verifica d'integrità» e creda di avere una garanzia che non ha.

## Conseguenze

- **R1** (Replay Archive e Recorder) e **R3** (Replay Player) diventano specificabili.
- **R3 nasce con un vincolo strutturale**, non solo con una checklist: dove mettere il codice è parte della
  decisione.
- Il `HashTurnLogOrdered` di D-062 ha una casa e un consumatore: sta nel manifest (D-077) ed è il Verifier a
  leggerlo.
- `Simulation.DeterministicReplay` va rinominato: è un test del Verifier e il suo nome dice Player.
  ✅ **Fatto il 2026-08-11** → `Replay.Verifier.ResimulationIsDeterministic` ([#538](https://github.com/DegrassiAaron/refactor-tactics-main/issues/538), [D-095](RT_PDR_00_Decision_Log.md)).
- `#415` è retroattivamente conforme: era Player-side prima che questo ADR esistesse, e lo era per la ragione
  giusta.

## Cosa questo ADR non decide

- **Nomi dei tipi C++, formato del manifest, comandi console**: implementazione.
- **`ContentManifestHash` / `RulesVersion`**: era [#413](https://github.com/DegrassiAaron/refactor-tactics-main/issues/413),
  **chiusa il 2026-08-10** da [D-083](RT_PDR_00_Decision_Log.md). Questo ADR dice *che* un archivio
  incompatibile va rifiutato in apertura; **quali campi** rendano «incompatibile» una condizione misurabile
  lo dice ora D-083 — *entra ciò che il resolver legge* — e ne rinvia la **costruzione** alla v0.2, con
  l'innesco dichiarato: quando un archivio esce dalla macchina che l'ha prodotto.
- **Firma e tamper-evidence**: fuori dalla v0.1, con il posto già previsto.
- **Se il Verifier giri anche in CI su ogni build**: è una scelta di processo, non di architettura.

## Verifica

I test seguenti erano **richiesti da questo ADR e non esistevano** alla sua stesura. ✅ **Esistono tutti dal
2026-08-11** (`RTReplayPlayerTests.cpp`, `#470`): la tabella resta perché dice *perché* servono, non solo che
servono.

| Test | Cosa rende osservabile |
|---|---|
| `Replay.Player.RunsWithoutResolver` | il Player riproduce una traccia in un contesto privo di resolver — la forma **negativa**, che dimostra il confine invece di affermarlo |
| `Replay.Player.RejectsIncompatibleArchive` | fail-closed in apertura su versione, topologia e checksum |
| `Replay.Verifier.ReportsFirstDivergence` | il verdetto nomina turno, fase e `ActionId` |

Già esistenti e riclassificati da questo ADR: `Replay.Verifier.ResimulationIsDeterministic` (Verifier —
rinominato da `Simulation.DeterministicReplay` il 2026-08-11), `Replay.Seek.*` (Player, `#415`).
