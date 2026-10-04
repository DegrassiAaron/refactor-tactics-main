# Seduta `U60` — il giro di `G16`: cinque superfici nello stesso giro

> **Tipo**: foglio di conduzione · **Scritto**: 2026-10-03 su `main` = `855bd409` (candidate
> `95eddfd37`) · **Owner del criterio**: la cella `G16` di
> [`v0.1-definition-of-done.md`](../../roadmap/v0.1-definition-of-done.md) §3. Questo foglio **non**
> ridefinisce il criterio: dove divergono, vale la cella.
>
> ⚠️ **Mai eseguito.** Questo foglio è stato scritto **prima** del giro, verificando ogni passo contro
> il codice; la §6 dichiara ciò che non è stato possibile verificare da fuori l'Editor.
>
> 🔑 **Il primo giro produrrà un 🔴 sul passo 4, e va eseguito comunque.**
> [#1936](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1936) è aperta con i suoi
> bloccanti. La ragione per non rimandare è nella cella: nessuno ha mai misurato se le **altre quattro**
> superfici reggano *insieme*, e questo gate esiste per quella domanda.

---

## 0. Preflight

```powershell
Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%' OR Name LIKE 'LiveCodingConsole%'" |
  Select-Object ProcessId, Name, CommandLine
```

Deve essere **vuoto**. La `CommandLine` porta il `.uproject` e l'`-abslog`, quindi dice *quale clone* e
*quale sessione* — `Get-Process` da solo non distingue una suite altrui dalla tua.

**L'Editor va compilato dal candidate.** Non serve il checkout: verifica che `main` sia ancora allineato,
e se lo è compila da lì senza perdere i documenti.

```bash
test "$(git rev-parse HEAD:Source)" = "$(git rev-parse 95eddfd37:Source)" \
  && test "$(git rev-parse HEAD:Content)" = "$(git rev-parse 95eddfd37:Content)" \
  && echo "OK: compilare da HEAD = compilare il candidate" \
  || echo "DIVERSI: fai checkout di 95eddfd37"
```

⛔ **Il pacchetto non serve, e non va usato.** Il modulo `RefactorTacticsEditor` è di tipo `Editor`
(`RefactorTactics.uproject:14`): Ability Lab e Hero Lab **non esistono nel pacchetto**. Il giro si fa in
Editor, non sul packaged di `G13`.

⚠️ **L'Editor GUI non viene aperto dal 2026-09-20** (`Saved/Logs/RefactorTactics.log`), e i binari sono
stati ricompilati dopo. Al primo avvio può comparire il modale **«Wait for ZenServer?»**, che il log
**non** registra: lo dice solo il titolo della finestra. Se l'avvio sembra impiccato, è quello — si
risponde e si rilancia, non si aspetta.

---

## 1. Il lancio — **una sola** apertura

```powershell
& "D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" `
  "D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" `
  -NoLiveCoding `
  -abslog="<scratchpad-di-sessione>\g16-u60.log"
```

| pezzo | perché |
|---|---|
| **una sola** apertura | il criterio di `G16` chiede *«nello stesso giro»*, e il testimone è che il **processo sia uno**: un solo `-abslog`, citato in `artifacts:` della seduta |
| `-abslog="..."` **fra virgolette** | senza virgolette si perde e il risultato è `exit 255` e nessun log, indistinguibile da *«non ha funzionato»* |
| `-NoLiveCoding` | Live Coding tiene la DLL e fa fallire la build successiva |

⛔ **Il log prova che il processo è uno; non prova che tu abbia guardato.** L'oracolo dei passi 3–5
resta umano, ed è dichiarato così nella cella.

---

## 2. Passi 1 e 2 — Ability Lab e Hero Lab

Sono **un solo pannello**, e si apre da:

**`Window` → `Tools` → `Ability / Hero Lab`**

| cosa | valore | prova |
|---|---|---|
| `TabId` | `RTLab` | `Source/RefactorTacticsEditor/Private/SRTLabPanel.cpp:13` |
| etichetta | `Ability / Hero Lab` | `RefactorTacticsEditorModule.cpp` — `SetDisplayName(LOCTEXT("LabTitle", ...))` |
| categoria | `Tools` | `.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())` |
| è un **nomad tab** | sì, deliberatamente | `RegisterNomadTabSpawner(SRTLabPanel::TabId, ...)` a `:140`, e il commento a `:115` dice perché non è un'estensione di layout |

**Cosa osservare**: che una ability canonica **giri** nella fixture e che il filtro per eroe mostri il
kit. Il tooltip del pannello dichiara esattamente questo.

⚠️ **Un riquadro vuoto non è un difetto, e i due casi si distinguono.** `SRTLabPanel::TestoTurnLog` ha
**tre** uscite: vuoto quando la run non è ancora partita, il testo *«La run non ha prodotto voci di
TurnLog»* quando la run **è** partita e non ha dato righe, e le righe altrimenti. Solo il **secondo** è
un esito da registrare.

---

## 3. Passo 3 — Presentation

La capability è chiusa da [#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288)
(`CP E21.2 — Animazioni di locomozione e impatto`, `CLOSED`).

**Cosa osservare**: che il risultato del resolver sia **visibile e leggibile a schermo** — non che esista
un evento nel log. È un oracolo percettivo e non si scorpora.

---

## 4. Passo 4 — Turn Log proiettato al giocatore · 🔴 **atteso**

Questo è il passo che darà rosso.
[#1936](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1936) è `OPEN`, e i bloccanti che
il suo ultimo commento elenca — [#2964](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2964),
[#3073](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3073),
[#2764](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2764) — sono aperti tutti e tre.
⚠️ Quelle **non** stanno nel corpo della issue: `gh issue view 1936 --json body` non le nomina.

**Dove guardare**: la zona `Zone_MiddleRight` — la fascia centrale del bordo destro — e il nodo
`WBP_RT_EventLogRight`, confermati dalla seduta del 2026-10-04 (§6).

**Come registrare il rosso senza interrompere il giro.** Con una partita in corso, in console:

```
rt.Debug.ScreenHud
```

Il comando esiste (`Source/RefactorTactics/UI/RTScreenHudConsole.cpp`) e accetta un ritardo opzionale in
secondi — `rt.Debug.ScreenHud 5` differisce il rapporto, utile se il feed si popola dopo il primo turno.

⛔ **NON filtrare il log su `Feed:`.** Il rapporto emette un **blocco** di righe `[RT]`, e quelle che
nominano la **causa** sono righe di continuazione **indentate** che il token `Feed:` non contiene: un
grep su `Feed:` scarterebbe proprio il verdetto. Copia nel referto **tutto il blocco `[RT]`**, dalla
riga di intestazione fino all'ultima indentata.

| cosa leggi | cosa significa | prova |
|---|---|---|
| *«Nessun mondo: questo comando vuole una partita in corso»* | lo hai lanciato fuori partita | `RTScreenHudConsole.cpp:32` |
| *«Screen HUD 4.1: nessuna radice viva in questo mondo»* | l'HUD non è montata — e la riga sotto suggerisce la causa più comune: partita avviata **aprendo una mappa** invece che da `L_Frontend` | `:52-53` |
| **nessuna riga del rapporto** | nessun widget del feed è vivo in quel mondo: il ciclo non trova istanze e il comando non stampa nulla | `:74`, che itera e chiama `DescribeFeedState()` |

**Cosa osservare**: `U60` formula questo passo più largamente di *«il feed mostra righe»* — chiede che
*«l'outcome sia spiegabile con dati canonici e reason code strutturati»*
([`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml), record `U60`). Registra ciò che vedi
contro **quella** formulazione, non contro una più stretta.

---

## 5. Passo 5 — Replay Viewer, e la precondizione che lo rende valido

Si apre dal frontend: le schermate si chiamano `MatchHistory` e `ReplayViewer`
(`Source/RefactorTactics/Frontend/RTFrontendScreenIds.cpp`, insieme a `Result`, `Pause`, `Match`).

🔴 **La precondizione: l'archivio deve essere quello della partita dei passi 3–4**, identificato per
`MatchId`. Non è formale — ci sono centinaia di cartelle di archivio già su disco, e senza questa riga il
passo 5 passerebbe su una qualunque di esse, misurando cinque cose scollegate invece di cinque cose
*insieme*.

⛔ **E la partita non stampa mai il proprio `MatchId`:**

```bash
grep -rnE 'UE_LOG[^;]*MatchId' Source/ --include=*.cpp | grep -v /Tests/   # 0
grep -rn  'MatchId' Source/ --include=*.cpp --include=*.h | grep -v /Tests/ | wc -l   # 74
```

Settantaquattro menzioni non-test, **zero** in un log. Quindi la correlazione si fa in due passi:

1. **prendi la cartella più recente** sotto `Saved/Replays/` subito dopo la partita;
2. **confermala**, perché «più recente» non è un'identità. Il nome della cartella **è** il `MatchId` in
   forma `Digits` (`Source/RefactorTactics/Replay/RTReplayAuditLibrary.cpp:344` combina la radice con
   `Audit.MatchId.ToString(EGuidFormats::Digits)`), e lo stesso valore sta nel campo `MatchId` del
   manifest.

⚠️ **`Outcome` nell'indice è un intero, non un nome**, e la legenda è l'ordine di `ERTMatchOutcome`
(`Source/RefactorTactics/Turn/RTTurnRules.h`): `0` = `InProgress` — **la partita non è finita** — `1` =
`Team0Wins`, `2` = `Team1Wins`, `3` = `Draw`. Un `Outcome : 0` è un secondo testimone indipendente che
la partita non ha raggiunto un esito terminale, e in quel caso il passo 5 è ❌, non ✅ su un archivio
preesistente.

---

## 6. ⛔ Cosa questo foglio NON ha potuto verificare

Dichiarato invece di riempito a intuizione — chi conduce lo completi dall'Editor.

✅ **I nomi delle zone: CONFERMATI dalla seduta del 2026-10-04, e rimessi.**

Questa sezione li dichiarava non verificabili, e la cautela era giusta nel metodo — da fuori l'Editor
`grep -rhoE "Zone_[A-Za-z]+" Source/` risponde col solo `Zone_C`, perché i nomi vivono **dentro**
`WBP_RT_TacticalHUD.uasset`, che `7d1145cd7` ha toccato come file binario. Ma il rapporto di
`rt.Debug.ScreenHud` li porta, e ora sono misurati:

| zona | inquilino |
|---|---|
| `Zone_TopLeft` | `WBP_RT_TeamRosterLeft` (+ due `WBP_RT_UnitCard_C`) |
| `Zone_TopCenter` | `WBP_RT_TurnHeader` |
| `Zone_TopRight` | — |
| `Zone_MiddleLeft` | `WBP_RT_SelectedUnitPanelLeft` (+ `WBP_RT_UnitCard`) |
| **`Zone_MiddleRight`** | **`WBP_RT_EventLogRight`** (`WBP_RT_EventLog_C`) — **è il feed del passo 4** |
| `Zone_BottomLeft` | — |
| `Zone_BottomCenter` | `WBP_RT_ActionDockBottom`, `WBP_RT_FastDecisionCenter` |
| `Zone_BottomRight` | — |

Albero a **17** widget innestati sotto `WBP_RT_TacticalHUD_C_0`.

⛔ **E `[MANCA] ActionSlot: 0` non è un difetto**, benché il rapporto lo scriva fra i cinque `[ok]`. Il
comando lo documenta da sé: fotografa l'albero nell'istante in cui la radice si costruisce, e gli
`WBP_RT_ActionSlot` li crea il dock *«quando arrivano le azioni»*. Il testimone che li prenderebbe sono
i warning `Icona non risolta` firmati `'ActionSlot'` — se rispondono `0`, nessuna azione è arrivata al
dock, e il censimento dice il vero per quel momento.

---

## 7. Dove va il verdetto

1. **Il record di seduta `U60`** in [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml): gli
   esiti dei cinque passi, e il log di `-abslog` citato in `artifacts:`.
   ⚠️ `issues: [2601]` **è già dentro e non va toccato**: senza, l'asserzione `A2` di
   `doc-coherence.ts` rende rosso `G14`.
2. **La cella `G16`** del DoD, con il verdetto.
3. **Il marcatore della seduta**, nella forma che il comando canonico riconosce —
   `(?:✅|🟡)\s*\*\*(Eseguita|CHIUSA|Chiusa|Parzialmente)`. Senza, il registro conta la seduta **aperta**
   anche se l'hai fatta: è già accaduto a `U16` e a `PIE-V01-FRONTEND-MAIN`.

**Il verdetto è una funzione dei cinque passi**: ✅ solo se tutti e cinque sono ✅; **un** passo ❌ rende
`G16` 🔴 — **mai** ⏳. ⏳ significa *giro non eseguito*, e smette di coprire *eseguito male*.

⚠️ **Se scrivi un comando con una pipe dentro una cella di tabella, scrivila `\|`** — e non fidarti
dell'occhio: `node tools/radar/doc-tables.ts --check` è il controllo che esiste per questo, e conta
correttamente `\|`. Una cella in più sposta ogni colonna a valle senza che nessun errore lo dica.

---

## 8. Se un passo va male

**Un rosso noto non interrompe il giro**: si registra e si prosegue, perché nessun passo successivo deve
diventare `NOT RUN` per contagio.

⛔ **Ma un difetto trovato qui NON si corregge qui.** `G16` è un gate di **verifica**, non di
implementazione: ogni difetto si apre sull'owner della capability, con la sua evidenza, e si rimisura su
un commit successivo. Chi scrive una correzione non ne firma da solo il verdetto.

⛔ E `G16` **non blocca** su Map Editor
([#1861](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1861)), Certification
([#814](https://github.com/DegrassiAaron/refactor-tactics-main/issues/814)) né PC Gym
([#1859](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1859)): sono fuori dal suo
perimetro per dichiarazione.
