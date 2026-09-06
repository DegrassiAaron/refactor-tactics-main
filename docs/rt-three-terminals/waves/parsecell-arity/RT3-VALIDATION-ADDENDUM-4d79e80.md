# Addendum VALIDATION — misura indipendente su `4d79e805`

Non e' un handoff. `RT3_CONTRACT.md` §10 vieta di riscrivere un handoff emesso, e
[`RT3-VALIDATION-cdcf1da.md`](RT3-VALIDATION-cdcf1da.md) resta il verdetto della wave:
`RISULTATO: DONE`. Questo file non lo modifica e non lo contraddice.

⚠️ Il nome porta lo SHA **misurato**, non quello del commit che contiene il file. E' la
   forma che `parsecell-arity/1-F3` chiede di decidere: la regola di naming di §10 e'
   auto-referenziale per chi emette, e qui il vincolo si scioglie perche' l'oggetto
   della misura e' dichiarato invece che dedotto dal nome.

## Perche' esiste

Il gate della wave e' stato eseguito su `cdcf1dad`. La head della PR
[#2496](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2496) e'
`4d79e805`.

`CLAUDE.md` §6: *«Prima del merge: verifica che il gate sia stato eseguito sul commit
che stai realmente mergiando.»*

Questo addendum chiude quella lacuna.

## Misura

```text
command:      ./scripts/rt-suite.ps1 -Filter RefactorTactics.Scenario
              -LogName rt-2482-rerun.log -WaitMinutes 30 -PollSeconds 30
HEAD:         4d79e805
albero:       ae48caf4
binario:      RefactorTactics       2026-09-06 01:20:51 / 14393856
              RefactorTacticsEditor 2026-09-06 00:27:33 /  1959424
found N:      174     performed N: 174
passed N:     174     failed N:    0
exit code:    0       **** TEST COMPLETE. EXIT CODE: 0 ****
durata:       00:39
verdetto:     VALIDA   (dichiarato da rt-suite: HEAD, albero, binario e processi
                        invariati per tutta la run)

EVIDENCE_REF: Saved/Logs/rt-2482-rerun.log

I due test della wave, nominatamente:
  RefactorTactics.Scenario.EveryCellFieldRejectsWrongArity        Success
  RefactorTactics.Scenario.LoaderRejectsCellArityOtherThanThree   Success
```

## Il deliverable e' identico

```text
git diff --stat cdcf1dad..4d79e805 -- Source/   ->  vuoto
```

Fra lo SHA del gate e la head della PR cambia **solo** `RT3-VALIDATION-cdcf1da.md`, 512
righe di documentazione. Il codice sotto test e' byte per byte quello misurato dal gate.

## Limiti dichiarati

⚠️ **Il binario non e' stato prodotto da questa misura.** `Build.bat` ha risposto
   `Target is up to date`, 0 azioni, exit 0. UBT ha **verificato** la corrispondenza col
   sorgente — non e' un confronto di timestamp — ma `WAVE_VALIDATION.md` §B chiede di
   ricompilare, e una compilazione a zero azioni non e' una compilazione. Sostanza
   soddisfatta, lettera no.

⚠️ Le tre mutazioni del criterio 4 **non** sono di questa sessione. Restano quelle di
   `RT3-VALIDATION-cdcf1da.md`, con le loro impronte `sha256`. Un tentativo indipendente
   di rieseguirle e' uscito `NON VALIDA` — la mutazione e' sparita dal working tree
   durante la run — ed e' la seconda occorrenza non attribuita dello stesso fenomeno che
   quel referto registra come `F17`.

## Evidenza per `F13`

Il lock di macchina mancante non e' piu' una previsione.

```text
attesa    25 minuti, poll ogni 20s (75 controlli consecutivi)
esito     nessuna finestra libera, nemmeno di 20 secondi
titolari  rt-wt-2443 · rt-wt-2455 · rt-wt-2486 · rt-wt-2518 ·
          refactor-tactics-main · refactor-tactict-dev
```

Una procedura che richiede quattro minuti esclusivi sul motore non e' eseguibile con
questo carico, e nessuna disciplina di prompt lo risolve: serve un lock a livello di
macchina, non di workspace root.

## Cosa questo addendum NON fa

⛔ Non cambia il verdetto della wave: resta `DONE`.
⛔ Non chiude nessuno degli otto `P1`, che sono contro owner diversi da questa wave.
⛔ Non chiude `F1` — `targetCell:986` e `dashTo:1073` conservano il difetto, e la
   condizione di sblocco dichiarata resta la issue figlia.
