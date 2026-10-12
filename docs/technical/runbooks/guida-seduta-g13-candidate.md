# Seduta `G13` — partita giocabile senza editor, dal pacchetto del candidate

> **Foglio di conduzione.** Lo esegue una persona davanti allo schermo. Ogni comando qui è stato
> verificato sul filesystem o nel sorgente il **2026-10-03**; ciò che non è stato verificato è dichiarato
> tale invece di essere dato per buono.
>
> **Candidate**: `95eddfd37`, albero `3009fe53b24603b3`. **Pacchetto**: cotto da quel commit il 2026-10-02.
> **Criterio**: la riga `G13` di [`../../roadmap/v0.1-definition-of-done.md`](../../roadmap/v0.1-definition-of-done.md) §3.

⛔ **Questo foglio non sostituisce [`guida-seduta-pia5-packaged.md`](guida-seduta-pia5-packaged.md), e non
lo ripete.** Da quello restano validi il vincolo `FabAsset`, l'igiene di chiusura e la forma del comando.
⚠️ Il suo **§1 enumera sette verifiche** e il criterio riscritto ne ha **otto**, con una cambiata di
natura; il suo **§3 è evidenza di un'altra build** (`33634aee`, 2026-09-12) lanciata con `-RTAutobattle`,
che il criterio nuovo **vieta**. Non riusare quei numeri come baseline.

---

## 0. Prima di lanciare

| | |
|---|---|
| motore libero | `Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'"` → nessuna riga |
| nessun crash pregresso da confondere | annota quante cartelle ci sono ora: `(Get-ChildItem "D:\Repositories\refactor-tactics-main\Saved\StagedBuilds\Windows\RefactorTactics\Saved\Crashes").Count` |
| il runtime di `HEAD` è il candidate | il blocco qui sotto risponde `OK`. Su `DIVERSI` la seduta **non si apre**: è `NOT RUN — candidate superato` finché una decisione non ricongela (referto del 2026-10-09, `D1`) |

```bash
test "$(git rev-parse HEAD:Source)" = "$(git rev-parse 95eddfd37:Source)" \
  && test "$(git rev-parse HEAD:Content)" = "$(git rev-parse 95eddfd37:Content)" \
  && echo "OK: il runtime di HEAD e' il candidate" || echo "DIVERSI: NOT RUN, non aprire la seduta"
```

⛔ **Il pacchetto staged si verifica per contenuto, ma il contenuto prova solo che il binario è quello: non che il
candidate sia ancora quello da attestare.** Il 2026-10-09 il blocco risponde `DIVERSI` — il fix di
[#3463](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3463) e il lavoro delle sedute `U67`–`U71` sono
entrati dopo `95eddfd37` — e una seduta condotta lo stesso attesterebbe una build che `main` ha sorpassato. Quando il
candidate viene ricongelato, lo SHA in questo blocco si aggiorna **insieme** a quello di `#85`.

🔴 **Al 2026-10-03 quelle cartelle sono tre, e sono tutte di misure mie del 2026-10-02** — due da `-nullrhi`
e una da `-RenderOffScreen`. Servono come **controllo positivo** del passo 6a: contengono un crash vero, su
questo stesso pacchetto.

---

## 1. Il lancio

```powershell
& "D:\Repositories\refactor-tactics-main\Saved\StagedBuilds\Windows\RefactorTactics\Binaries\Win64\RefactorTactics.exe" `
  -windowed -ResX=1600 -ResY=900 `
  -RTPlanningSeconds=600 `
  -abslog=D:\Repositories\refactor-tactics-main\Saved\Logs\g13-seduta.log
```

🔑 **L'eseguibile è quello NIDIFICATO.** I due `.exe` nella radice dello stage sono stub
`BootstrapPackagedGame`: funzionano, ma lanciare direttamente il binario toglie un livello di indirezione
e rende inequivocabile *quale* configurazione stai misurando.

⛔ **Development, non Shipping.** Lo Shipping sta nella **stessa cartella** (`RefactorTactics-Win64-Shipping.exe`)
ed è compilato **senza logging** (`USE_LOGGING_IN_SHIPPING 0`): i cinque esiti che G13 legge dal log lì
**non esistono**, e il grep risponderebbe `0` con l'aspetto di un verde. Verificato nei binari —
`Fine partita al round` è **presente** nel Development e **assente** nello Shipping.

⛔ **Niente `-nullrhi` e niente `-RenderOffScreen`.** Entrambi hanno fatto crashare *questo* pacchetto il
2026-10-02: il primo con l'assert `RenderData should not initialize resources in headless cooked runs`, il
secondo con `GPU Crash dump Triggered`. La seduta è **presidiata**: serve una finestra vera.

⛔ **Non passare una mappa.** Il pacchetto parte **già** da `L_Frontend` — `GameDefaultMap` letto dal `.ini`
**cotto** dentro il `.pak` — e passarla aggirerebbe il frontend che il criterio richiede.

⚠️ **`-RTPlanningSeconds=600` non è un lusso**: il tetto di pianificazione è **30 secondi** e il turno si
chiude da solo. Aprire la console, digitare, leggere la risposta, poi selezionare, ordinare e disfare non
sta in trenta secondi.

### Primo controllo, prima di giocare

```bash
grep -m1 "commandline=" "D:/Repositories/refactor-tactics-main/Saved/Logs/g13-seduta.log"
```

🔑 **Il log dichiara la riga di comando che il processo ha davvero ricevuto.** Se un flag si è perso, lo
scopri adesso e non dopo un'ora di gioco.

---

## 2. Armare il CSV — ⛔ **dopo** `PLAY`, non prima

1. dal frontend, **PLAY** fino a essere in partita;
2. apri la console con **`~`** (Tilde — default del motore, il progetto non lo ridefinisce);
3. digita: `rt.Debug.RecordPacing 1`
4. **leggi la risposta.** Deve dire che la registrazione è attiva.

🔴 **La trappola che costa l'intera seduta.** `rt.Debug.RecordPacing` è un **comando di console**, non una
cvar: pretende un `ARTTurnManager` vivo nel mondo. Da `L_Frontend` non ce n'è, e il comando risponde
**`[RT] Nessun TurnManager nel livello.`** senza armare niente. ⛔ Per la stessa ragione **non** si può
armare da riga di comando: `-ExecCmds=` gira all'inizializzazione, quando il mondo è ancora il frontend, e
`-dpcvars=rt.Debug.RecordPacing=1` non esiste come variabile e **non fa nulla**.

⚠️ **Il comando senza argomento NON arma**: `rt.Debug.RecordPacing` da solo è un'*interrogazione*, stampa lo
stato e non cambia il flag. Un argomento illeggibile viene **rifiutato** con
`[RT] Argomento non interpretabile: nulla e' stato cambiato.` — leggila, non darla per buona.

✅ **La finestra è più larga di quanto sembri**: il flag si legge alla **chiusura** del turno
(`ConcludeTurn` → `Pacing.Close`), e i contatori crescono **comunque**, flag o no. Quindi armare in
qualunque istante **prima che il turno 1 si concluda** salva anche la riga del turno 1 per intero.

---

## 3. Cosa fare in partita, per il congiunto che conta

Il CSV chiede, **su almeno un turno**: `SelectionCount >= 1` **e** `OrderCount >= 1` **e** `UndoCount >= 1`.
I primi due vengono da sé giocando. Il terzo no, e ha una sequenza obbligata:

1. **seleziona** un'unità propria (click sinistro) → `SelectionCount`
2. **posa un waypoint di movimento** (click sinistro su una cella) → `OrderCount`
3. ⛔ **nessuna abilità armata**
4. **tasto DESTRO** → `UndoCount`

🔴 **Premi l'RMB, non `BackSpace`.** `UndoAction` è mappata su **entrambi**: `BackSpace` soddisfa la
*lettera* del congiunto e **manca esattamente il difetto** per cui la seduta packaged esiste — che nella
finestra cotta il tasto destro non raggiunga il controller per fuoco o cattura del cursore.

⚠️ **E l'Undo conta solo se il livello di Back è `Waypoint`.** Con un'abilità armata il primo destro
**disarma il targeting** e registra `Click`, non `Undo`. Con il countdown di Ready attivo il destro è
**Unready** e ritorna prima. Da qui il passo 3.

Poi: **gioca la partita fino a un esito**, e osserva. Le due domande percettive del passo 7 si rispondono
mentre giochi, non dopo.

---

## 4. Chiudere, e mettere al sicuro l'evidenza

Il CSV nasce in un percorso **gitignorato**, con un nome che dipende dall'ora:

```
Saved\StagedBuilds\Windows\RefactorTactics\Saved\RT\pacing_<AAAAMMGG-hhmmss>.csv
```

✅ Le righe si appendono a **ogni fine turno** con apri-scrivi-chiudi: chiudere il gioco con la X **non
perde** ciò che è già scritto. ⚠️ Ma se non lo copi subito, l'evidenza del gate vive **solo su questo
disco**. Copiala in `docs/technical/evidence/g13/` insieme al log.

⚠️ `Saved/RT/` **oggi non esiste**: il primo file prodotto è quindi inequivocabilmente il tuo.

---

## 5. I sei esiti meccanici

> 🔑 **Non serve greppare a mano: c'è uno script, e dichiara quale rete ha colpito.**
>
> ```bash
> python tools/seduta/esiti.py Saved/Logs/g13-seduta.log \
>   --csv "Saved/StagedBuilds/Windows/RefactorTactics/Saved/RT/pacing_<timestamp>.csv" \
>   --crashes "Saved/StagedBuilds/Windows/RefactorTactics/Saved/Crashes"
> ```
>
> Fa tre cose che i comandi qui sotto, usati a mano, non fanno: tiene **tre** reti sui crash e dice
> **quale** ha colpito; valida ogni ricerca negativa con un **controllo positivo** sullo stesso file,
> così che uno zero non si confonda con un pattern rotto; e **rifiuta di emettere verdetti** se il
> cancello della §5-bis è chiuso. `NOT RUN` resta distinto da `PASS`.
> I comandi che seguono restano la forma leggibile dello stesso controllo, e servono quando vuoi
> guardare una riga invece di un verdetto.

---

## 5-bis. ⛔ Il cancello: leggilo PRIMA dei sei

I sei esiti attendono tutti **zero**. Una seduta che non parte, o che muore al frontend, li soddisfa
**tutti e sei** — e un gate di soli zeri non distingue *«pulito»* da *«mai avvenuto»*.

Il criterio di `G13` chiede anche *«risoluzione completa»* e *«risultato raggiungibile»*, che sono
**positivi**; la misura del 2026-09-13 li ha letti come `Travel Failure: 0`, che è un negativo e non
li prova. I testimoni che li provano esistono:

| # | esito | comando | atteso |
|---|---|---|---|
| 0a | ⬆️ **la partita è stata allestita** | `grep -ac "Board 2v2 esagonale avviata" "$LOG"` | **≥ 1** |
| 0b | ⬆️ **la partita è FINITA** | `grep -ac "Fine partita al round" "$LOG"` | **≥ 1** — `Frontend/RTFrontendNavigator.cpp:373` |
| 0c | ⬆️ **l'esito è stato deciso** | `grep -ac "Partita finita:" "$LOG"` | **≥ 1** — `Turn/RTTurnManager.cpp:3596` |
| 0d | la schermata di Result si è aperta | `grep -ac "Partita finita senza frontend" "$LOG"` | `0` — `Frontend/RTMatchFrontendBridge.cpp:64` |

⬆️ significa **atteso diverso da zero**. `0b` e `0c` sono testimoni **distinti** e non si sostituiscono:
il primo è del navigatore e dice che il *frontend* ha chiuso la partita, il secondo è del TurnManager e
dice che la *simulazione* ha deciso un esito. Se compare solo `0c`, la risoluzione è arrivata e la
schermata no — ed è esattamente ciò che `0d` nomina.

⛔ **Se `0a` risponde `0`, i sei esiti sotto NON si leggono**: la seduta è `NOT RUN`, non verde.

---

Sostituisci `$LOG` con `D:/Repositories/refactor-tactics-main/Saved/Logs/g13-seduta.log`.

| # | esito | comando | atteso |
|---|---|---|---|
| 6a | nessun crash | `grep -ac "Fatal error\|Assertion failed\|=== Critical error" "$LOG"` | `0` |
| 6a-bis | **rete larga** | `grep -ac "Error:" "$LOG"` | `0` |
| 6a-ter | **ortogonale al log** | nessuna cartella nuova sotto `...\RefactorTactics\Saved\Crashes\` rispetto al conteggio del passo 0 | nessuna |
| 6b | nessun asset editor-only | `grep -ac "Failed to find object" "$LOG"` e `grep -ac "SkipPackage" "$LOG"` | `0` e `0` |
| 6c | nessun `Travel Failure` | `grep -ac "Travel Failure" "$LOG"` | `0` |
| 6d | il CSV ha il turno buono | vedi il blocco **6d** qui sotto | almeno un turno con tutte e tre |

🔴 **Perché 6a ha tre righe e non una.** Il pattern storico è **cieco a una classe di crash già vista su
questo pacchetto**: sul log della run che è morta di GPU crash il 2026-10-02,
`grep -ac "Fatal error\|Assertion failed\|=== Critical error"` risponde **`0`**. Un controllo cieco è
indistinguibile da un verde. La rete larga lo prende (`Error:` → 13 su quello stesso log), e il conteggio
delle cartelle di crash non dipende da **nessun** pattern.

⛔ **Il controllo «nessun overlay di debug acceso di default» NON si fa col grep del runbook vecchio.** Il
pattern `DrawCells|DrawIntent|ShowDebug|DebugDraw` **non può colpire**: nessuna riga di produzione contiene
quei nomi, e il testo digitato in console non raggiunge il file di log. Il controllo onesto è
**percettivo** e appartiene al passo 7: *a schermo non c'è nessun overlay di debug che non hai acceso tu*.

### 6d — il CSV

```bash
CSV=$(ls -t "D:/Repositories/refactor-tactics-main/Saved/StagedBuilds/Windows/RefactorTactics/Saved/RT/"pacing_*.csv | head -1)
head -1 "$CSV"                 # l'intestazione: SelectionCount, OrderCount, UndoCount sono le colonne 6, 7, 8
awk -F, 'NR>1 && $6>=1 && $7>=1 && $8>=1 {print "turno " $1 ": " $6 "/" $7 "/" $8; t++} END {exit !t}' "$CSV"
```

⚠️ **I CSV già nel repository hanno 13 colonne, non 20**: sono di un formato precedente e **non valgono
come riferimento**. Fidati dell'intestazione che `head -1` stampa.

---

## 6. Le due domande che restano all'occhio

Non si scorporano, e il criterio lo dichiara. Rispondi **sì** o **no**, con una frase:

1. **L'interfaccia è leggibile?** — i testi si leggono sul fondo, le righe non si accavallano, nulla di
   essenziale è coperto o fuori dallo schermo.
2. **Il giocatore trova l'affordance?** — sapevi *dove* cliccare senza averlo letto qui.

🔑 **Il CSV prova che l'input è arrivato, non che fosse ovvio darlo.** È la ragione per cui questa seconda
domanda non ha un comando.

➕ E con esse: **nessun overlay di debug acceso che non hai acceso tu** (vedi sopra).

---

## 7. Dove va il verdetto

| destinazione | quando |
|---|---|
| **colonna 4 della riga `G13`** di [`../../roadmap/v0.1-definition-of-done.md`](../../roadmap/v0.1-definition-of-done.md) §3 | **sempre** — è l'unica obbligatoria |
| [`../test-manuali-pie.md`](../test-manuali-pie.md) | **solo se** la seduta giudica voci PIE. Ne tocca almeno quattro ancora ⏳ — `PIE-V01-FRONTEND-PLAY`, `-RESULT`, `-PAUSE`, `-REPLAY` — nessuna nel subset `RELEASE-V01`, quindi `G9` non si muove |
| `docs/technical/evidence/g13/` | il log e il CSV |

⛔ **Non scrivere un record nuovo in `editor-sessions.yaml`.** Il criterio di `G13` non lo chiede (lo chiede
`G16`), e **`U17` è già questa seduta** — `critical: true`, `done_when: BUILD SUCCESSFUL su entrambe le
configurazioni e una partita conclusa dalla packaged`. Un record nuovo lascerebbe due contenitori per lo
stesso lavoro. ⚠️ E una seduta `critical: true` senza `issues:` fa cadere l'asserzione `A2` di
`doc-coherence`, cioè rende rosso `G14`.

⚠️ **Una copia derivata è già stantia e non la aggiorna nessuno**:
`docs/technical/architecture/capability-map.md` §«Stato reale dei gate» porta ancora `G13 🟡 2026-09-03`,
`G12 🔴 STANTIO`, `G2 🟡`. Si dichiara da sé una trascrizione datata, quindi invecchia per progetto — ma
sappi che esiste, se cerchi lo stato dei gate e trovi due risposte.

---

## 8. Se un esito è rosso

⛔ **Non chiudere il giro.** Registra il rosso, nomina il passo, e apri la issue **sull'owner** — mai dentro
il gate. Un passo non eseguito si dichiara **col motivo**, mai omesso.
