# Onda 4 della sequenza di chiusura — le sedute, 2026-10-26

> **Che cosa è**: l'evidenza dei gate dell'Onda 4 ([`sequenza-chiusura-v01-spec-panel-2026-10-09.md`](../../../roadmap/plans/sequenza-chiusura-v01-spec-panel-2026-10-09.md) §4) sul candidate v2
> ([`D-541`](../../../decisions/RT_PDR_00_Decision_Log.md)). L'Onda 3 sta in [`onda3-misure-2026-10-26.md`](onda3-misure-2026-10-26.md). Non è un owner: dove diverge dalla
> [`v0.1-definition-of-done.md`](../../../roadmap/v0.1-definition-of-done.md), vale la DoD.
>
> **Convenzione sui numeri** ([`AGENTS.md`](../../../../AGENTS.md) §14): i numeri sono **esiti di questa tornata**, ciascuno col comando che lo produce.

## 0. Su che cosa si è misurato

| | |
|---|---|
| **Candidate v2** | `7450ead647cfce43ae419218302bdf8bd183cc73` (`D-541`) |
| **`HEAD`** | `db3c10f4dc051fb4c6814670ff8db7f167c6c05f`; `git status --porcelain` → 0 righe prima e dopo ogni run; `git diff --quiet 7450ead64 HEAD -- Source Content ':(exclude)Source/RefactorTactics/Tests' ':(exclude)Source/RefactorTacticsEditor/Private/Tests'` → `OK` ([`D-542`](../../../decisions/RT_PDR_00_Decision_Log.md)) |
| **Binario** | DLL dell'Editor `UnrealEditor-RefactorTactics.dll` `sha256` `43A0842A6FBD13D9…`, quella della suite di `G2` (Onda 3); invariata fra l'inizio e la fine di ciascuna run |
| **Macchina** | Unreal e Live Coding spenti prima di ogni lancio (`Get-CimInstance Win32_Process`). Nessuna misura di tempo in questa sezione: l'esito di una partita non dipende dal carico di fondo |

## 1. `G10` — partita 2v2 autobattle

`UnrealEditor-Cmd.exe <uproject> /Game/RT/Maps/Dev/L_HexArena/L_HexArena -game -RTAutobattle -useFixedTimeStep -fps=60 -unattended -nopause -nosplash -nullrhi -NoLiveCoding -abslog="…"`, due volte di seguito. In entrambe il log dichiara la fine dopo 31 s e 41 s; il processo non esce da solo (`Fine partita al round …` e poi il game loop), quindi l'ho terminato io dopo aver verificato sulla `CommandLine` che portasse il mio `-abslog`. Nessun processo Unreal residuo.

**Esito** (riga testuale): *«Partita finita: Vince il team 0 (blu) - per eliminazione (round 9/12, obiettivo 0-2, formato Format.Skirmish2v2)»* e *«Fine partita al round 9: ERTMatchOutcome::Team0Wins»*. Apertura: *«Board 2v2 esagonale avviata su 64 celle con 4 eroi»*.

**Non degenere**, clausola per clausola:

- *danni inflitti*: 26 righe con ` danni`; due eliminazioni (*«Eliminata: BP_Unit_Ivrin_C_0 (team 1)»*, *«Eliminata: BP_Unit_Branth_C_0 (team 1)»*);
- *reaction risolta*: `Hero.Aevik.ReactiveCapacitor` infligge 5 danni (p20) due volte (`(q=-1,r=1,L=0) -> (q=-3,r=0,L=0)` e `(q=-1,r=-2,L=0) -> (q=-1,r=-3,L=0)`, la seconda con *eliminata*);
- *progresso obiettivo mosso*: `obiettivo 0-2`.

**Determinismo**: il flusso `LogRT` senza timestamp è identico fra le due run (407 righe, stesso `sha256` `db535f47768c3d79…` del testo). **Differenza dal 2026-10-02** (`Team1Wins`, `RoundLimit`, round 12): attesa, perché il codice è cambiato (`git rev-list --count 95eddfd37..7450ead64 -- Source` → 639 commit).

**Riserva che resta**: nessuna riga del flusso porta `L=1` (`grep -aoE '[( ,]L=?[0-9]+[),]'` → 208 `L=0` e 31 `L0`, nessun'altra classe): la mappa è multilivello, la partita no. Le due righe `Caduta:` sono cadute di dislivello nella classe `L0`. **La riserva sulle reaction cade**: una è stata risolta con effetto.

Log: [`../g10/partita-2v2-autobattle-candidate-v2-2026-10-26.log`](../g10/partita-2v2-autobattle-candidate-v2-2026-10-26.log) (il solo flusso `LogRT`, senza prefissi di timestamp, come i due precedenti).

## 2. `G13` — la seduta sul pacchetto

**Pacchetto**: quello cotto da `HEAD` `ad06c1cfc` (Onda 3 §4), `Saved/StagedBuilds/Windows/RefactorTactics/Binaries/Win64/RefactorTactics.exe`, Development, 380.946.944 byte. **Lancio**, come prescrive il foglio: `RefactorTactics.exe -windowed -ResX=1600 -ResY=900 -RTPlanningSeconds=600 -abslog=…\Saved\Logs\g13-seduta.log`, **senza** `-RTAutobattle` e senza mappa. Prima del lancio: nessun processo del progetto, `git diff --quiet 7450ead64 HEAD -- Source Content …` → `OK`, cartelle in `Saved/Crashes/` del pacchetto: **1** (quella della metà packaged di `G2`, 07:17:07). Il log dichiara la riga di comando ricevuta (`commandline="" -windowed -ResX=1600 -ResY=900 -RTPlanningSeconds=600 -abslog=…`) e il frontend (`L_Frontend` presente).

**Partita giocata dall'autore**, dal frontend: 9 turni, `Team1Wins` per obiettivo, *«Partita finita: Vince il team 1 (rosso) - per obiettivo (round 9/12, obiettivo 0-5, formato Format.Skirmish2v2)»* e *«Fine partita al round 9: ERTMatchOutcome::Team1Wins»*. Una eliminazione (*«Eliminata: BP_Unit_Aevik_C_… (team 0)»*). Il processo è uscito normalmente: il log termina con `LogExit: Exiting.`, senza traccia di GPU crash.

**Gli esiti meccanici**, da `python -X utf8 tools/seduta/esiti.py Saved/Logs/g13-seduta.log --csv <pacing_20261026-081447.csv> --crashes <Saved/Crashes del pacchetto>` (output integrale in [`../g13/esiti-seduta-g13-2026-10-26.txt`](../g13/esiti-seduta-g13-2026-10-26.txt)):

| esito | risultato |
|---|---|
| controllo del file (la ricerca sa rispondere) | PASS, `LogInit` ×92 |
| cancello: la partita è avvenuta | PASS: allestimento 1, fine partita del frontend 1, esito deciso 1; `Partita finita senza frontend` → 0 |
| nessun crash (rete storica / rete larga / GPU) | PASS 0 / PASS 0 / PASS 0 |
| nessun asset mancante, nessun `Travel Failure` | PASS 0 / PASS 0 |
| CSV: colonne per nome | `SelectionCount` col. 6, `OrderCount` col. 7, `UndoCount` col. 8 |
| CSV: input funzionante | PASS: 9 turni letti, 1 turno soddisfa i tre congiunti (turno 1: 2 · 10 · 2) |
| cartelle di crash | **FAIL dello script, non della seduta** — vedi sotto |

⚠️ **Il `FAIL` sulle cartelle di crash**: lo script conta le cartelle in assoluto (*«fotografa questa cartella PRIMA della seduta: le preesistenti non sono tue»*). La sola presente, `UECC-Windows-1621894E4065C4EA1D33AF8BCAD149A0_0000`, è del 07:17:07, cioè della metà packaged di `G2` ([#3424](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3424)); il conteggio prima della seduta era 1 e dopo è 1, con lo stesso nome. **Nessuna cartella nuova**: la riga 6a-ter del foglio (*«nessuna cartella nuova rispetto al conteggio del passo 0»*) è soddisfatta.

**Giudizio a occhio, dell'autore**: interfaccia leggibile **sì**; affordance trovata senza leggere il foglio **sì**; nessun overlay di debug acceso che non avesse acceso lui **sì (assente)**. Il terzo giudizio è stato confermato esplicitamente: la prima risposta («sì») era ambigua rispetto alla polarità della domanda.

**Non giudicate**: le voci PIE `PIE-V01-FRONTEND-PLAY`, `-RESULT`, `-PAUSE`, `-REPLAY` (il foglio le nomina come giudicabili da questa seduta, nessuna è nel subset `RELEASE-V01`): non sono state giudicate in questa tornata.

**Un fatto emerso dal log, non un esito del gate**: `grep -a -c "Icona non risolta: 'None' richiesta da 'ActionSlot'" Saved/Logs/g13-seduta.log` → **29424**, otto per frame, in una sessione di circa cinque minuti (il 98,5% delle righe con la categoria `LogRT: `: `29424` su `29868`). È un `Warning` e non un `Error`, quindi fuori dai controlli del gate. Compare identico nelle catture di `Client FPS` (32008 righe su 4000 frame). Candidato a una issue sull'owner dell'`ActionSlot`; non aperta qui.

File: [`../g13/pacing_20261026-081447.csv`](../g13/pacing_20261026-081447.csv), [`../g13/esiti-seduta-g13-2026-10-26.txt`](../g13/esiti-seduta-g13-2026-10-26.txt), [`../g13/estratto-log-seduta-g13-2026-10-26.txt`](../g13/estratto-log-seduta-g13-2026-10-26.txt). Il log intero (4 MB) resta in `Saved/Logs/g13-seduta.log`, gitignorato.

## 3. `G16`

**NOT RUN.** È la seduta con una persona nell'Editor (`U60`, una sola apertura): questa sezione si compila quando è eseguita.

## 4. `G9`

**NOT RUN.** Il subset `RELEASE-V01` delle verifiche manuali (PIE) richiede una persona.

## 5. `Client FPS` — rimisura e correzione della lettura dell'Onda 3

L'Onda 3 ([§5](onda3-misure-2026-10-26.md)) registrava `Client FPS` **fuori target e non attribuito**: `p99` 18,8–20,6 ms nella serie più pulita, fino a 111 ms nella più sporca. Le catture sono state **ripetute due volte in più sullo stesso pacchetto** e la lettura va corretta.

**Comando** (uguale alle serie precedenti): `RefactorTactics.exe /Game/RT/Maps/Dev/L_HexArena/L_HexArena -RTAutobattle -dpcvars=rt.Map.Source=LevelAsset -csvCaptureFrames=4000 -ExitAfterCsvProfiling -windowed -ResX=1280 -ResY=720 -unattended -nopause -nosplash -NoSound -abslog=…`. Le sei catture di questa passata sono **alternate**: una con `-LogCmds="LogRT Error"` (gli avvisi `Icona non risolta` soppressi: 0 righe nel log) e una normale, `FrameTime` cercata per nome, nearest-rank sui 3700 frame dopo i primi 300:

| cattura | p50 | p95 | p99 | max | avvisi nel log |
|---|---|---|---|---|---|
| 1, senza avvisi | 6,466 | 8,409 | **9,610** | 18,5 | 0 |
| 1, normale | 6,591 | 8,270 | **9,629** | 18,8 | 32008 |
| 2, senza avvisi | 6,663 | 8,479 | **9,591** | 18,7 | 0 |
| 2, normale | 6,021 | 7,971 | **9,267** | 18,2 | 32008 |
| 3, senza avvisi | 5,726 | 7,426 | **8,686** | 26,7 | 0 |
| 3, normale | 5,892 | 7,429 | **8,511** | 29,7 | 32008 |

**Lettura.** (1) Le tre catture normali danno `p99` 8,511–9,629 ms, cioè **104–117 fps** (target 60), **in linea con `9,326–9,683` ms del 2026-10-03**. (2) **Gli avvisi non pesano**: con e senza, `p99` 9,610/9,629, 9,591/9,267, 8,686/8,511. (3) **Le tre passate di oggi sullo stesso pacchetto non concordano**: 24,6–111,5 ms (serie `a`), 18,8–20,6 ms (serie `b`), 8,5–9,6 ms (questa). Il binario non è cambiato fra le passate (il pacchetto è lo stesso, cotto una volta), quindi la differenza viene da cosa girava intorno o dallo stato del pacchetto appena cotto. **Non attribuito**; ipotesi non verificata: cache di shader/PSO fredde dopo la cottura. (4) 🔴 **Il `Path` di controllo dell'Onda 3 non ha visto la differenza** (0,074–0,115 ms in tutte le passate): su questa macchina non è un controllo valido per il tempo di frame, e la frase dell'Onda 3 che lo usava per escludere la macchina va letta così. (5) Il valore da citare è quello di questa passata, **con le altre due accanto**: ripeterla in un altro momento è la prova che manca.
