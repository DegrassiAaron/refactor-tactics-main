# Condurre una seduta PIE — una apertura, N voci, un tasto per verdetto

> **Statuto**: il conduttore vive sul branch `issue/3208-conduttore-seduta-pie` ([#3208](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3208)),
> in attesa di merge. ⚠️ La sua prima seduta reale non è ancora stata condotta: leggibilità dell'overlay
> e ergonomia dei tasti non hanno un gate headless, e restano da giudicare a schermo.
> Copre le voci allestite da uno scenario del corpus. Le altre restano la seduta a mano di sempre, e
> questa guida dice quali sono e perché.
>
> Owner del verdetto resta [`test-manuali-pie.md`](../test-manuali-pie.md): il conduttore produce un file,
> non aggiorna il registro. La propagazione è il sotto-progetto successivo.

---

## 1. Cosa fa, in una riga

Incatena gli scenari che allestiscono le voci PIE, e dopo ognuno si ferma col mondo fermo e chiede
**sì / no / non giudicabile**. Non devi spostare unità, scegliere abilità, né ridigitare l'id fra una voce
e l'altra.

## 2. Aprire una seduta

```
1. Apri  /Game/RT/Maps/Dev/L_DevSandbox   (il livello ospite di OGNI scenario del corpus)
2. Play
3. Console (`)   →   rt.Pie.Session Visual.Perception.*
```

⚠️ **Premi Play prima di aprire la seduta.** Le porte con cui il conduttore avvia gli scenari le installa
`ARTGameMode::BeginPlay`: senza partita, il comando risponde *«porte non installate»* e non parte.

### Il dry-run viene prima, e non è una formalità

```
rt.Pie.Session                    → stampa l'uso, non compone
rt.Pie.Session <selettore>        → stampa la coda E la avvia
```

Il comando stampa sempre la coda **prima** di avviarla: quali voci, in che ordine, da quale allestimento,
e — la parte che conta — quali sono rimaste fuori e perché. Scoprire a metà seduta che tre voci non erano
coperte costa la stessa apertura che il conduttore esiste per risparmiare.

### Due forme di selettore

| Forma | Esempio | Cosa seleziona |
|---|---|---|
| prefisso di scenario | `Visual.Perception.*` | gli **scenari**; ogni voce del loro `verifies` diventa un passo |
| elenco di voci | `PIE-VIS-SIGHTWALL,PIE-V01-LOG` | le **voci**; il conduttore risale allo scenario che le dichiara |

Il guadagno sta nella prima forma: `Visual.Perception.Acceptance` dichiara sette voci, e le giudichi tutte
in **una sola apertura dell'Editor**.

⚠️ Sette voci sono sette playback, ed è voluto: aprire l'Editor costa minuti, rigiocare uno scenario
costa secondi, e guardare sette cose in un passaggio solo non è un giudizio affidabile. Ogni passo
rigioca la scena con la sua domanda davanti.

⛔ Se una voce è dichiarata da **due** scenari, la coda non parte: il conduttore stampa `AMBIGUA` e chiede
di nominare lo scenario. Sceglierne uno in silenzio farebbe giudicare la voce in un allestimento diverso
da quello che hai in mente.

## 3. Giudicare

A fine scenario il mondo resta fermo e compare il prompt con l'**ID della voce** e, se lo scenario ce
l'ha, il suo `_nota_cosa_guardare`.

| Tasto | | Console |
|---|---|---|
| `1` | sì | `rt.Pie.Verdict pass [motivo]` |
| `2` | no | `rt.Pie.Verdict fail [motivo]` |
| `3` | non giudicabile | `rt.Pie.Verdict na <motivo>` |
| `Esc` | interrompi la seduta | `rt.Pie.Session.Abort` |

⛔ **Il prompt non riporta l'esito atteso**, e non è una dimenticanza: l'esito atteso ha un owner solo, ed
è il registro. Tienilo aperto accanto — l'ID a schermo è l'ancora per ritrovarci la voce.

**Interrompere non perde niente**: i verdetti già dati si scrivono, il resto diventa `NOT_RUN`.

## 4. Cosa esce

`Saved/RTPieSessions/<sessionId>/session.json`, un file per seduta:

```json
{ "sessionId": "20260920-143012", "commit": "15c50b96", "buildVersion": "...",
  "steps": [ { "pieItem": "PIE-VIS-SIGHTWALL", "criterion": null,
               "scenarioId": "Visual.Map.SightWallIsWalkable",
               "machineOutcome": "PASS 12/12", "verdict": "FAIL",
               "reason": null, "at": "2026-09-20T14:30:41Z" } ] }
```

### I cinque verdetti, e i tre che non dai tu

| `verdict` | Chi lo scrive | Quando |
|---|---|---|
| `PASS` · `FAIL` | tu | hai guardato e deciso |
| `NOT_JUDGEABLE` | tu, **oppure** il conduttore | tu se non è giudicabile; lui se lo scenario non si è caricato |
| `BLOCKED` | il conduttore | la sessione è partita ma la scena non ha mai giocato |
| `NOT_RUN` | il conduttore | la seduta è finita prima di arrivarci |

I tre automatici portano sempre un `reason`. Se manca, il file scrive `(motivo non registrato)`: è un
difetto da guardare, non rumore.

### Due campi, e nessuno scende dall'altro

`machineOutcome` è l'esito delle `expect` dello scenario; `verdict` è il tuo giudizio. **Un `expect` rosso
con «a schermo si capisce» è informazione, non contraddizione** — è esattamente la coppia prodotta dalla
seduta `U54`, dove il feed passava a verde mentre il dock falliva. Per questo uno scenario con `expect`
fallite ti chiede comunque il verdetto invece di chiudersi da solo.

### Il commit, e quando non c'è

Il file dichiara lo SHA letto da `.git/HEAD`. Se non riesce a leggerlo scrive `"commit": null` **e lo
annuncia nel log**: *«commit non dichiarato — questa seduta non è attribuibile a un albero»*.

⚠️ Dichiara il `HEAD`, **non** che l'albero sia pulito. Se hai modifiche non committate, il file porta lo
SHA e nessuna promessa sul resto.

## 5. Cosa il conduttore NON copre

Dichiarato qui perché tu lo sappia prima di aprire, non dopo:

1. **Le voci che vogliono un'altra mappa.** `PIE-V01-SCREENHUD` va giudicata con la partita avviata da
   `L_Frontend`: il conduttore sa avviare scenari, non sa navigare il frontend.
2. **Le voci che vogliono le mani** — un click sul dock, un `TAB`, un hover. Nel repository non esiste
   nessuna injection di input (`grep -rln "AutomationDriver|ProcessMouse|InjectInput|SimulateInput" Source/`
   → nessun file), quindi l'harness non può produrle. Dove nessuno può produrre l'osservazione, il
   precedente è [`D-402`](../../decisions/RT_PDR_00_Decision_Log.md): si riduce il criterio, non si
   costruisce un produttore.
3. **Le voci senza scenario.** Se nessuno scenario le dichiara in `verifies`, compaiono fra le `ESCLUSA`
   del dry-run.

## 6. Dichiarare una voce su uno scenario

Nel JSON dello scenario, in testata:

```json
"verifies": ["PIE-KNOW1", "PIE-KNOW2"]
```

⛔ **Solo gli ID, mai la domanda né l'esito atteso.** È la regola che `editor-sessions.yaml` esiste per far
rispettare, e vale identica qui.

Il gate `RefactorTactics.PieSession.CatalogDeclaresOnlyRealItems` diventa rosso se un id non ha una riga
nel registro. Per lanciarlo:

```powershell
& "D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "<clone>\RefactorTactics.uproject" `
  "-ExecCmds=Automation RunTests RefactorTactics.PieSession;Quit" `
  -unattended -nopause -nosplash -nullrhi -NoLiveCoding -abslog=<tuo scratchpad>\pie.log
```

⚠️ Passa sempre `-abslog` nel tuo scratchpad: è ciò che rende il processo attribuibile alla tua sessione
quando qualcun altro conta i motori vivi. E se l'editor esce `255` senza scrivere quel file, il log c'è
comunque in `Saved/Logs/RefactorTactics.log` — un file di `-abslog` mancante significa «cerca l'altro log»,
non «non è partito».

## 7. Il motore è uno

Prima di aprire una seduta:

```powershell
Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, CommandLine
```

La `CommandLine` porta il `.uproject` — quindi quale clone — e `-abslog` — quindi quale sessione. Una
seduta PIE tiene l'Editor aperto a lungo: se qualcuno sta misurando, aspetta o dichiaralo.

⚠️ **Una run headless su questo clone accende il bridge MCP sulla 8765 per tutta la macchina**
(`Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini` ha `bAutoStartServer=True`, che vince sul
versionato). Le chiamate MCP di altre sessioni atterrano nel tuo processo e perdono il ponte quando esce.
