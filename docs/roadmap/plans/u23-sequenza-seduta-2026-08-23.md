# U23 · Sequenza della seduta: la partita registrata, PIE e packaged

**Issue**: [#959](https://github.com/DegrassiAaron/refactor-tactics-main/issues/959) (CP 47.6) · **Seduta**: `editor-sessions.yaml` U23 · **Data**: 2026-08-23

**Decisione d'autore**: si registrano **entrambe** le partite e **si dichiara la differenza** — il free-run
sull'asset d'autore mostra la configurazione che il giocatore ottiene, lo scenario porta la vittoria che
`G10` e `G13` chiedono. Quattro registrazioni in tutto: due partite × due ambienti.

> ⚠️ Questo documento è la sequenza, non l'esito. L'esito va in `PIE-V01-PACKAGED` e nel commento di #959,
> **con l'evidenza allegata** — è la regola del registro: una voce diventa ✅ solo con evidenza allegata.

---

## Passo 0 — prima di aprire l'editor

Il fix di [#1296](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1296) deve essere in
`main`, altrimenti l'editor gira senza e il free-run torna a oscillare: PR **#1297**. Poi:

```
git checkout main && git pull
```

E ricompila l'editor, perché la DLL sul disco è quella del branch:

```
D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat RefactorTacticsEditor Win64 Development ^
  -Project="D:\Repositories\refactor-tactics-dev\RefactorTactics.uproject" -WaitMutex
```

⚠️ **Se la build dice «Unable to build while Live Coding is active»**, c'è un editor aperto — anche su un
altro clone, perché il guard vede il *motore*, non il progetto. O lo chiudi, oppure aggiungi
`-NoHotReloadFromIDE`, che salta il controllo senza toccarlo (`HotReload.cs`, `bAllowHotReloadFromIDE`).

---

## Parte 1 — PIE

Apri `Content/RT/Maps/Dev/L_HexArena/L_HexArena`. Tutto ciò che segue si fa **dalla console** (tasto
backtick): nessun `.uasset` va modificato né salvato.

> ⛔ **Se l'editor chiede di salvare `L_HexArena` o `DA_HexMap_Arena`, la risposta è NO.** È la regola già
> scritta in `m6-8-sequenza-sedute-u2-u6`: quel package è il prodotto di un'altra seduta.

### 1A — free-run sull'asset d'autore

```
rt.Test.Scenario                 (senza argomenti: azzera, vedi la trappola qui sotto)
rt.Map.Source LevelAsset
rt.Match.Autobattle 1
```

Poi **Play**. Attesa misurata headless: `Board 2v2 esagonale avviata su 64 celle con 4 eroi`, dodici turni,
**pareggio allo scadere 12/12**, ~25 voci `Combat`.

Da registrare: video o sequenza di screenshot **più** il log. Il log di sessione della PIE è in
`Saved/Logs/RefactorTactics.log`.

### 1B — scenario `AutoBattle.ArenaV01`

```
rt.Match.Autobattle -1           (torna alla proprietà: lo scenario allestisce da sé)
rt.Test.Scenario AutoBattle.ArenaV01
```

Poi **Play**. Il GameMode allestisce lo scenario **al posto** della partita normale, avanza un passo per
frame e centra la camera da solo (`[RT-Test] Camera centrata sulla mappa dello scenario`). Attesa misurata:
**vince il team 0 (blu) per eliminazione al turno 19**, 33 voci `Combat`.

> 🔴 **Trappola: `rt.Test.Scenario` dura quanto il processo dell'editor.** Digitata una volta resta attiva a
> ogni Play successivo, e la partita normale non parte più. Per tornare al free-run: `rt.Test.Scenario` senza
> argomenti. Lo stesso vale per `rt.Match.Autobattle` (`-1` per tornare alla proprietà) e `rt.Map.Source`
> (stringa vuota).

### Il criterio del punto 3, e come lo si soddisfa

Il DoD chiede **almeno un evento `Combat` a schermo e la riga di log col reason code dello stesso turno**.
In pratica: fermati su un turno in cui una unità colpisce, cattura lo schermo, e affianca la riga

```
(q=-1,r=1,L=0) -> (q=-2,r=1,L=0): 20 danni, eliminata (Hero.Gadget.LinearDischarge, p55)
```

Le coordinate nella riga sono quelle delle due celle: è ciò che rende schermo e log **la stessa cosa** e non
due cose che si somigliano.

---

## Parte 2 — pacchetto Development

**Non esiste un pacchetto**: `Saved/StagedBuilds` e `Saved/Packaged` sono assenti, e non c'è nessun
`RefactorTactics.exe` nel clone. Va prodotto.

```
D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat BuildCookRun ^
  -project="D:\Repositories\refactor-tactics-dev\RefactorTactics.uproject" ^
  -noP4 -platform=Win64 -clientconfig=Development ^
  -cook -build -stage -pak -archive ^
  -archivedirectory="D:\Repositories\refactor-tactics-dev\Saved\Packaged"
```

✅ `Scenarios/` **è già in staging** — `Config/DefaultGame.ini` porta
`+DirectoriesToAlwaysStageAsUFS=(Path="../Scenarios")`, corretto con
[#926](https://github.com/DegrassiAaron/refactor-tactics-main/issues/926) — quindi `AutoBattle.ArenaV01`
si carica anche fuori dall'editor. **Verificalo comunque nel log**: è la metà di U23 che il repository ha
già pagato una volta (M7.4, i materiali `TSoftObjectPtr` non cookati, unità grigie).

### 2A — free-run

```
RefactorTactics.exe -dpcvars=rt.Map.Source=LevelAsset -RTAutobattle -abslog=free-run-packaged.log
```

> 🔴 **`-dpcvars=`, non `-ExecCmds=`.** Misurato sul pacchettizzato il 2026-08-10: `-ExecCmds` gira **dopo**
> l'inizializzazione, quando il GameMode ha già allestito la partita — la variabile si imposta e non serve a
> niente, senza un errore che lo dica.

### 2B — scenario

```
RefactorTactics.exe -RTScenario=AutoBattle.ArenaV01 -abslog=scenario-packaged.log
```

---

## Punto 5 del DoD — il confronto, e cosa si pretende

Sono **due domande diverse** («le regole girano» e «girano fuori dall'editor»), quindi il confronto si
dichiara per ciascuna partita:

- **Scenario**: ci si aspetta lo **stesso esito** — è una configurazione dichiarata, e il determinismo del
  runtime è strutturale (nessun RNG: `RNG-1`/`RNG-2` in `OPEN_DECISIONS`). Il confronto è sul **TurnLog
  canonico turno per turno**, mai su `StateHash`, che è permutazione-invariante e non esprime divergenze
  d'ordine (E47.2). In pratica: le righe `LogRT` dei due log **derivano dal TurnLog** dalla fetta A di
  [#79](https://github.com/DegrassiAaron/refactor-tactics-main/issues/79) — un produttore solo — quindi
  confrontare quelle righe *è* confrontare il TurnLog.
- **Free-run**: stesso ragionamento, e l'esito atteso è il pareggio 12/12.

Se qualcosa diverge, la divergenza **è il risultato della seduta** e va registrata: è precisamente ciò che
la doppia esecuzione esiste per trovare.

---

## Cosa questa seduta NON chiude

Le voci `PIE-HEXPLAY-*` non ancora verdi restano da eseguire una per una, ciascuna con la propria domanda:
E47 ne cambia il **costo** — da «gioca una partita intera» a «guardala» — non la natura. Restano lavoro di
**M6.8** (U6).
