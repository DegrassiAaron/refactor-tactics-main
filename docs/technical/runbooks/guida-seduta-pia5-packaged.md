# Guida di seduta — `PIA-5.3`, il percorso giocabile sul pacchetto (`G13`)

> `CURRENT` · **Creata**: 2026-09-13 · **Issue**: [#2620](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2620) (`PIA-5`) ·
> **Gate**: `G13` in [`v0.1-definition-of-done.md`](../../roadmap/v0.1-definition-of-done.md) §3 ·
> **Owner degli esiti**: [`test-manuali-pie.md`](../test-manuali-pie.md) — questa guida dice **come**
> osservare, non **cosa è risultato**.
> Misurata su `main = 17fbc400`, sul pacchetto Development costruito il 2026-09-12 da `33634aee`.

> 📐 **Convenzione dei numeri.** Ogni conteggio porta accanto il comando che lo produce e la data.
> Chi rilegge **ricalcola**: senza un gate che li rimisuri, invecchiano da soli.

---

## 1. La seduta è molto più corta di quanto `PIA-5.3` faccia sembrare

Il gate elenca un percorso e sette verifiche:

```text
Launch → Main Menu → Play → Match → Planning → Ready → Resolution → Result → Restart/Quit
nessun crash · nessun asset editor-only · debug OFF di default · input funzionante ·
UI leggibile · resolution completa · risultato di partita raggiungibile
```

Letto così sembra una seduta lunga. **Non lo è**: la maggior parte si legge da un log, e una partita
non presidiata sul pacchetto lo produce senza che nessuno tocchi il mouse.

| Verifica | Chi la può dare | Stato al 2026-09-13 |
|---|---|---|
| nessun crash | il log | ✅ misurata, §3 |
| nessun asset editor-only | il log, più `PIE-PKG-EDITOR-NAMESPACE` ✅ 2026-09-03 | ✅ misurata, §3 |
| debug OFF di default | il log | ✅ misurata, §3 |
| resolution completa | il log | ✅ misurata, §3 |
| risultato raggiungibile | il log | ✅ misurata, §3 |
| **input funzionante** | **un occhio e una mano** | ⏳ §4 |
| **UI leggibile** | **un occhio** | ⏳ §4 |

⛔ **`Launch → Main Menu → Play` non va rifatto**: lo copre `PIE-V01-FRONTEND-MAIN`, **eseguita il
2026-09-04** su pacchetto Development avviato senza argomenti. ⚠️ Il suo **marcatore** dice ancora `⏳`
mentre la cella dichiara *«ESEGUITA»* — vedi §5, ed è un difetto di registrazione, non di esecuzione.

---

## 2. Il comando, misurato

⚠️ **`-dpcvars=` e non `-ExecCmds=`**, e non è una preferenza di stile: `-ExecCmds` gira **dopo**
l'inizializzazione, quando il GameMode ha già allestito la partita — la variabile viene impostata e non
serve a niente, senza un errore che lo dica. Misurato sul pacchettizzato il 2026-08-10, e dichiarato in
`RTTestConsole.cpp`.

```powershell
& "<clone-principale>\Saved\StagedBuilds\Windows\RefactorTactics.exe" `
    L_HexArena -RTAutobattle -dpcvars=rt.Map.Source=LevelAsset `
    -windowed -ResX=1280 -ResY=720 -abslog="<scratchpad>\pkg-smoke.log"
```

⛔ **Il pacchetto va costruito dal clone PRINCIPALE**, e questo vincolo ha già prodotto un falso verde il
2026-08-30. `Content/FabAsset/` — i pack Paragon — è **gitignorato**: un clone o un worktree che non li ha
cuoce senza di essi, `SkipPackage` risponde `0` *perché la domanda non è stata posta*, e le unità arrivano a
schermo senza mesh né animazioni. Si verifica prima di cuocere, non dopo:

```bash
find <clone>/Content/FabAsset -name "*.uasset" | wc -l
# 37482 nel clone principale · cartella ASSENTE in refactor-tactics-refactor — 2026-09-13
```

⚠️ **`-nullrhi` è VIETATO su dati cotti**: la `RenderData` pretende di poter renderizzare *oppure* che i
dati non siano cotti, e in un packaged headless non vale nessuna delle due. La finestra si apre davvero.

---

## 3. Cosa il log ha già risposto — 2026-09-13, pacchetto da `33634aee`

Partita non presidiata, **un solo lancio, nessun input**:

```text
[RT] Board 2v2 esagonale avviata su 64 celle con 4 eroi
[RT] Formato di partita in vigore: 'Format.Skirmish2v2' (RoundLimit 12, soglia obiettivo 5, 2 unita' per squadra)
[RT] Partita finita: Vince il team 1 (rosso) - allo scadere dei round (round 12/12, obiettivo 0-4, …)
[RT] Fine partita al round 12: ERTMatchOutcome::Team1Wins
```

I conteggi, col comando che li produce — tutti sul log di quella singola esecuzione:

```bash
grep -ac "Fatal error\|Assertion failed\|=== Critical error" pkg-smoke.log   # 0
grep -ac "Error:"                                            pkg-smoke.log   # 0
grep -ac "Failed to find object"                             pkg-smoke.log   # 0
grep -ac "Travel Failure"                                    pkg-smoke.log   # 0
grep -aic "DrawCells\|DrawIntent\|ShowDebug\|DebugDraw"      pkg-smoke.log   # 0
grep -ac "Objective.Control"                                 pkg-smoke.log   # 12
```

🔑 **Le uniche `Failed to load` sono `aqProf.dll`, `VtuneApi.dll`, `VtuneApi32e.dll` e
`WinPixGpuCapturer.dll`** — profiler assenti dalla macchina, non asset del gioco. ⛔ Chi conta
`Failed to load` invece di `Failed to find object` legge quattro rossi che non esistono.

ℹ️ L'unica occorrenza di `rt.Debug` nel log è un **suggerimento** dentro una riga `Display`
(*«per l'albero a regime usa `rt.Debug.ScreenHud`»*), non una variabile accesa. `debug OFF di default`
si legge dai due zeri qui sopra, non dall'assenza della stringa.

### 🔴 E la riserva storica di `G13` cade per intero

La riserva del 2026-08-10 dichiarava due mancanze. La prima — *«la partita gira sull'arena di test, non su
un livello di gioco»* — era già caduta con [#1654](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1654).
**La seconda cade qui**: diceva *«la via a punti non è esercitata in autobattle, il bot non cerca
l'obiettivo, ed è E26»*, e *«la soglia obiettivo è 0»*.

Misurato: la soglia è **5**, `Objective.Control` compare **12** volte, e la partita si decide **a punti**
— `obiettivo 0-4` — non per eliminazione né per pareggio allo scadere senza progresso.

⚠️ **Due warning del pianificatore restano aperti** e non bloccano il gate, ma vanno visti da chi possiede
il bot: `[RT] Prenotazione rotta u0: la cella (q=-4,r=1,L=0) risulta gia' di u1 — invariante rotta a monte`,
due occorrenze in dodici round, da `RTHexBotLibrary.cpp`. È il **pianificatore**, non il resolver, e nessun
errore ne è seguito.

---

## 4. Cosa resta, e si fa in una sola apertura

Due criteri, entrambi percettivi. Si giudicano **nello stesso lancio**, togliendo `-RTAutobattle` così che
la partita chieda input:

```powershell
& "<clone-principale>\Saved\StagedBuilds\Windows\RefactorTactics.exe" `
    -windowed -ResX=1280 -ResY=720 -abslog="<scratchpad>\pkg-seduta.log"
```

**(1) Input funzionante.** Dal menu: `PLAY` con un clic reale; in partita, selezionare un'unità, dare un
ordine di movimento, premere `Ready`. Il criterio è che il gesto **arrivi**, non che sia bello.
🔴 *Falsificazione*: un clic che non produce la riga di log corrispondente, o un `Ready` che non chiude il
turno, è un `FAIL` — non «riprovare più forte».

**(2) UI leggibile.** Si rilegge **in scala di grigi**: nessuno stato che conti — selezione, hover, squadra,
turno — può essere distinguibile dal **solo** colore.
🔴 *Falsificazione*: se in scala di grigi due stati diversi diventano indistinguibili, è un `FAIL` anche se
a colori si leggevano benissimo. È la stessa regola che `PIE-V01-BOARD` applica alla board.

⛔ **`PIE-V01-PACKAGED` non si chiude con questa guida**, e va detto prima di aprire il pacchetto: quella
voce chiede *«video o sequenza di screenshot **più** il log»*, e il materiale deve mostrare almeno un evento
`Combat` a schermo **e** la riga di log col reason code dello stesso turno. Il log di §3 è **metà** di ciò
che chiede. Chi esegue la seduta registri anche il visivo, altrimenti la voce resta ⏳ con una ragione
diversa da quella di prima.

---

## 5. ⚠️ Un difetto di registrazione, da risolvere prima di leggere `G13`

`PIE-V01-FRONTEND-MAIN` porta nella cella *Stato* un `⏳` iniziale e, più avanti nella **stessa cella**,
`✅ ESEGUITA il 2026-09-04` con i tre punti verificati — la schermata `PLAY · SETTING · QUIT`, il `PLAY`
premuto con un clic reale che porta in partita, il focus da tastiera visto.

Il comando canonico legge **il primo marcatore** della cella:

```bash
awk -F'|' '/^\| \*\*PIE-V01-FRONTEND-MAIN\*\*/ {s=$(NF-1);
  if (match(s, /✅|🟡|❌|⏳/)) print substr(s, RSTART, RLENGTH) }' docs/technical/test-manuali-pie.md
# ⏳     ← 2026-09-13, main = 17fbc400
```

∴ la voce è **contata aperta** mentre dichiara di essere stata eseguita, e `G13` legge come da fare un pezzo
che è stato fatto. ⛔ **Il marcatore non si gira da qui**: il verdetto appartiene a chi ha condotto la seduta
del 2026-09-04, e la cella non dichiara esplicitamente tutti i criteri della voce — `SETTINGS` che annuncia
*coming soon* e la label di versione leggibile senza console. Chi possiede quel verdetto chiuda la voce o
dichiari quale criterio manca.

---

## 6. Igiene

Il pacchetto non esce da solo: arriva alla schermata di risultato e resta lì. Si chiude **per
`CommandLine`**, mai per nome — l'eseguibile staged è un *launcher* che avvia il binario vero sotto
`Binaries/Win64/`, quindi terminare il solo padre lascia vivo il figlio (misurato il 2026-09-13):

```powershell
Get-CimInstance Win32_Process -Filter "Name LIKE 'RefactorTactics%'" |
  Where-Object { $_.CommandLine -like "*<il proprio -abslog>*" } |
  ForEach-Object { Stop-Process -Id $_.ProcessId -Force }
```

⛔ Un `Stop-Process` per nome chiuderebbe anche il pacchetto di un'altra sessione.
