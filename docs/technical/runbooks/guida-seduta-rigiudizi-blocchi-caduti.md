# Guida di seduta — i rigiudizi: voci il cui blocco è già caduto

> `CURRENT` · **Creata**: 2026-09-25 · **Owner degli esiti**:
> [`test-manuali-pie.md`](../test-manuali-pie.md) — questa guida dice **come** osservare, non **cosa è
> risultato**. Preparata su `main = a1017da78`.
> **Voci**: `PIE-VIS-CHARGE`, `PIE-VIS-PHASES` (seduta `U42`), `PIE-VIS-PUSH`
> (era `not_schedulable`), `PIE-V01-SCREENHUD` criterio (3) (seduta `U54`), `PIE-V01-REACTCOND`
> (seduta `U18`).

## Perché queste cinque, e perché insieme

Non sono voci nuove: sono voci **già giudicate o già dichiarate ineseguibili**, il cui impedimento è
stato rimosso e che nessuno ha ripreso. Il corpo delle issue che le convocano dichiara ancora blocchi
che non esistono più.

| voce | il blocco era | chiuso il |
|---|---|---|
| `PIE-VIS-CHARGE`, `PIE-VIS-PHASES` | [#3261](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3261) — la spinta non passava dal playback | 2026-09-24 |
| `PIE-VIS-PUSH` | l'`oracle` della sua voce in `not_schedulable`, che era #3261 | 2026-09-24 |
| `PIE-V01-SCREENHUD` (3) | [#3178](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3178) — il dock coi rettangoli bianchi | 2026-09-18 |
| `PIE-V01-REACTCOND` | [#1034](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1034), ritirato da #1439 | 2026-08-26 |

⚠️ **Una voce rigiudicata costa minuti; una voce dimenticata costa la credibilità del registro.** Un ❌
che descrive un difetto corretto è peggio di una voce ⏳: chi legge conclude che il gioco sia rotto dove
non lo è, e chi lavora nell'area lo prende come vincolo.

---

## 1. 🔑 Il gesto che rende questa seduta possibile, e che a `U42` mancava

La seduta `U42` ha giudicato `PIE-VIS-CHARGE` e `PIE-VIS-PHASES` guardando finestre da **~0,69 secondi**
— un segmento a `PlaybackCellsPerSecond = 1.44f` — e la cella di `PIE-VIS-CHARGE` registra **dieci corse**
per un solo verdetto. Non serve più:

```
rt.Debug.PlaybackControls 1
rt.Debug.PlaybackStartPaused 1
```

La seconda è documentata alla lettera nel sorgente: *«ogni playback comincia FERMO, così il primo confine
osservabile è il primo»* (`RTGameMode.cpp`). Poi, in partita:

| tasto | effetto |
|---|---|
| **`K`** | pausa / riprendi il playback |
| **`L`** | avanza di **un passo** |

⛔ **Il tasto `V` NON rallenta.** Cicla `x1 → x2 → x4` e non scende sotto `x1`: la scala è
`{ 1.f, 2.f, 4.f }`. Se una sessione precedente l'ha toccato, l'etichetta si legge sulla HUD, e i
~0,69 s diventano ~0,17. Verificalo invece di fidartene.

⚠️ `rt.Debug.PlaybackStartPaused` **richiede** `rt.Debug.PlaybackControls 1`: senza il comando per
riprendere, partire fermi bloccherebbe la partita. Il default di entrambe è `0`, fail-closed di #1879.

---

## 2. Preflight — prima di aprire l'Editor

1. **Il motore è uno per macchina.** Leggi la `CommandLine`, non contare i processi:
   ```powershell
   Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, CommandLine
   ```
   Il `.uproject` dice **quale clone** e `-abslog` **quale sessione**. Un `UnrealEditor-Cmd` di un altro
   clone non ti riguarda; un Editor interattivo ovunque blocca `Build.bat` dappertutto.
2. **Il binario deve contenere il fix.** ⛔ È la trappola che costa l'intera seduta: `00162cb64` nell'albero
   non significa nella DLL che l'Editor ha caricato. Un Editor aperto da prima, o un Live Coding non
   applicato, produce un rosso **indistinguibile** dal difetto vero. Compila con l'Editor chiuso e
   annota lo SHA del binario nel referto.
3. **`git rev-parse HEAD` subito prima del Play**, e scrivi *quello* nel referto: durante la preparazione
   di questa guida `HEAD` si è mosso quattro volte.

---

## 3. Allestimento comune — Play 1, 2 e 3

Apri `/Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox`.

⛔ **Non le mappe che i criteri nominano.** Le fixture generano la propria arena dal file dello scenario
(`mapRadius`), e `L_DevSandbox` serve solo da livello d'avvio.

Console dell'Editor, **prima** del primo Play:

```
rt.Debug.PlaybackControls 1
rt.Debug.PlaybackStartPaused 1
```

Poi, per ogni banco: **Stop** → `rt.Test.Scenario <Id>` → **Play**. Con PIE fermo si cambia banco senza
riaprire l'Editor.

### ⛔ Prima di guardare qualunque cosa, leggi il log

```
[RT-Test] AUTO-RUN <Id> (da: <fonte>): <N> turni, pausa <X>s — avanza un passo per frame
```

Due modi di sbagliare, con **sintomi opposti**:

- **id con refuso** → `LogRT: Error: [RT-Test] scenario '<id>' non caricabile` e **non parte niente**:
  fail-closed, la scena resta inerte;
- **CVar mai impostata, o svuotata** → parte una **partita normale, in silenzio**, e chi guarda crede di
  vedere il banco. È successo il 2026-09-03 con un `Enviorment` per `Environment`.

⚠️ La CVar dura quanto il **processo dell'Editor**, non quanto il Play: se la seduta precedente ne aveva
impostata un'altra, è ancora lì. Rileggi la riga `AUTO-RUN` **a ogni Play**, non solo al primo.

A fine seduta: `rt.Test.Scenario ""` per tornare alla partita normale.

---

## 4. Play 1 — `PIE-VIS-CHARGE`

```
rt.Test.Scenario Visual.Movement.Charge
```

**Il banco**: un turno. `(3,0)` è il **caricante** (`Hero.Branth.Ram`, `dashTo (1,0,0)`); `(1,0)` è il
**bersaglio**, che non parte.

⛔ **Identifica le unità per CELLA, mai per nome.** Il criterio del registro dice «addosso a **Gadget**» e
il runbook `scenari-validazione-visiva.md` dice «**Riktor** usa Ram»: nessuno dei due nome esiste più nel
banco — `Hero.Gadget` → `Hero.Aevik` è un rename eseguito (#2491).

**Dove posare l'occhio**: sul cilindro fermo a `(1,0)`, dal primo passo del playback.

> **Domanda** — il bersaglio lascia la sua cella **dopo** che il caricante gli è arrivato accanto?
>
> - 🟢 il caricante si ferma a `(2,0)` e in quell'istante il bersaglio è **ancora** su `(1,0)`: i due
>   cilindri **si toccano, senza nessuna cella in mezzo**. Solo dopo il bersaglio scivola a `(0,0)`.
> - 🔴 il bersaglio è già su `(0,0)` al primo passo, oppure si muove insieme o prima del caricante, e i
>   due non appaiono **mai** adiacenti.

⛔ **Questa domanda non è il criterio integrale**, ed è importante scriverlo nel referto. Il criterio
chiede «accelerazione, impatto e arresto addosso, più la spinta di 1, più il confronto diretto con
`Movement.LongWalk`». Questa è la **condizione osservata a U42**, cioè la causa del ❌. Un verde fa cadere
quella causa; **non promuove da solo la voce a ✅**.

### Fuori dal criterio, e perché non si guarda qui

- **L'accelerazione non è giudicabile su questo banco.** L'harness spawna `ARTUnit::StaticClass()`, non i
  `BP_Unit_*`: niente skeletal, niente `AnimInstance`, niente curva — solo la traslazione di un cilindro
  a velocità costante.
- **Il caricante era già animato prima del fix**: il suo Dash porta i `CellVerdicts` da sempre. Vedere il
  cilindro che parte scivolare **non è evidenza di niente**. Il discriminante è il movimento del
  **bersaglio**, e il suo istante.
- **Il `PASS 3/3` in log non risponde**: `expect` pinna HP, vita e turni, ed era verde anche con la spinta
  invisibile.

---

## 5. Play 2 — `PIE-VIS-PHASES`

```
rt.Test.Scenario Visual.Core.PhaseOrder
```

**Il banco**: un turno, tre fasi. `Dash` (Branth carica da `(1,0)` su Aevik a `(-1,0)`), `Blast` (Ivrin
spara su Aevik, e nello stesso Blast si risolve la spinta), `Move` (Muiren cammina).

⚠️ **Branth percorre UNA sola cella**, da `(1,0)` a `(0,0)`: si ferma **accanto** ad Aevik, non dentro.
È tutto il Dash, e dura ~0,69 s. Decidi **prima** di premere Play se ti basta a occhio, o se vuoi `L`.

**Dove posare l'occhio**: su Aevik, il cilindro a `(-1,0)`, prima di premere Play.

> **Domanda** — i tre momenti si **leggono** separati: prima solo Branth che carica, poi il colpo con
> Aevik che arretra, poi solo Muiren che cammina?
>
> - 🟢 sì, si leggono come **tre momenti**
> - 🔴 no — e il rosso ha **due cause che portano altrove**, da distinguere nel referto:
>   - **(a)** Aevik è già arretrato al primo passo → **`NOT RUN`, non ❌**: il binario non porta #3261.
>     Scrivi lo SHA del binario e rifai la build.
>   - **(b)** la sequenza è corretta — Aevik parte fermo e arretra dopo — ma il Dash resta comunque
>     irriconoscibile → ❌ per una causa **nuova**: una cella in ~0,69 s in inquadratura d'insieme. Non è
>     #3261, ed è un difetto da aprire con issue propria.

---

## 6. Play 3 — `PIE-VIS-PUSH`

```
rt.Test.Scenario Visual.Combat.PushResistance
```

**Il banco**: un turno, quattro unità. Squadra 0 in difesa: `(0,0)` dichiara `Action.Guard`, `(0,-2)` no.
Squadra 1 all'attacco da est, entrambi con `Hero.Muiren.PressureJet` (spinta 1).

**Dove posare l'occhio**: in fase `Blast`, sui due difensori insieme.

> **Domanda** — il cilindro in `(0,0)`, quello in guardia, resta sulla propria cella mentre quello in
> `(0,-2)` arretra di una?
>
> - 🟢 `(0,0)` non lascia la cella e **non viene scostato in nessun istante**; `(0,-2)` arretra e si ferma
>   in `(-1,-2)`. Si legge una spinta **assorbita** accanto a una spinta **subìta**.
> - 🔴 **(a)** il difensore in guardia viene scostato — anche se poi rientra — e si legge una spinta
>   riuscita dove è stata assorbita; **(b)** `(0,-2)` non arretra affatto: la spinta continua a non
>   arrivare a schermo.

⚠️ **Il tabellone finale era verde anche prima del fix.** `expect` controlla celle, HP e turni, e il
corpus era `PASS` con la spinta invisibile. E la fase `Blast` **si apriva comunque**, perché ci sono dei
colpi: vederla aprirsi non prova che la spinta sia animata.

⛔ **L'`oracle` scritto nella voce `not_schedulable` è cieco al proprio fix**: letto alla lettera direbbe
che il blocco è ancora in piedi. Vale la issue chiusa, non il grep.

---

## 7. Play 4 — `PIE-V01-SCREENHUD`, criterio (3)

⚠️ **Allestimento diverso**: questa voce non si allestisce con `rt.Test.Scenario`. Vuole una partita vera
con l'HUD §4.1 montato, e il layer lo monta `EnterMatch` **dal frontend** — non l'apertura di una mappa.

```
rt.Test.Scenario ""          ← azzera lo scenario, o la partita normale non viene allestita
rt.Match.Autobattle 0
```
Mappa `L_Frontend` → **Play** → premi **a mano** il bottone di avvio dal menu.

⛔ **Autobattle acceso = seduta persa in silenzio.** La nota di `U54` lo dichiara verbatim: senza, nessuna
unità viene selezionata, il dock non costruisce nessuno slot, e i criteri non sono misurabili affatto.

**Dove posare l'occhio**: il riquadro dell'icona **dentro** ciascuno slot del dock — non il nome scritto
accanto, che è una riga testuale composta di proposito.

> **Domanda** — con gli slot costruiti, **ogni** slot mostra un glifo disegnato al posto del rettangolo
> bianco?
>
> - 🟢 ogni slot porta un disegno riconoscibile a 24 px e distinguibile da quello accanto, **e** sul log
>   di questa seduta `grep -c "Icona non risolta"` → `0` e `grep -c "Icona non caricabile"` → `0`.
> - 🔴 anche **un solo** slot col rettangolo bianco o il riquadro vuoto. ⚠️ Non è la riconferma del
>   difetto: #3178 è **chiusa**, quindi sarebbe una **regressione**, ed è una cosa nuova.

🔑 Un verde chiude la **sola metà icone** del criterio (3). Nel referto si scrive «(3) icone ✅, stati
disponibile/selezionato/cooldown **non osservati**», non «(3) ✅».

---

## 8. Play 5 — `PIE-V01-REACTCOND` *(si guarda il log, non lo schermo)*

🔑 **La cella del registro si contraddice, e va letta per intero.** Il primo marcatore per posizione è
`⏳ non eseguibile — 2026-08-16`, ma più avanti **nella stessa cella** `🔵 Il bloccante è caduto il
2026-08-26` lo ritratta: *«la voce passa da non eseguibile a **da eseguire**»*. Il ⏳ è la testa stantia,
non una fonte concorrente.

⛔ **Due trappole che fabbricano un rosso falso:**

1. **L'autobattle ingoia ogni pressione di tasto** (`IsPlanningInputInert()`). Il banco che una
   pianificazione precedente assegna a questa voce prescrive `rt.Match.Autobattle=1`: va spento.
2. **Non filtrare l'Output Log su `LogRT`.** Le quattro risposte del comando escono dall'`FOutputDevice`
   della console (`Ar.Logf`), **non** dalla categoria `LogRT` — che invece porta la riga di armamento.
   Col filtro attivo si vede metà della prova e si conclude il falso.

> **Domanda** — le quattro emissioni di `rt.Reaction.Condition` (senza reazione armata; a 50 con reazione
> armata; a 150; senza argomenti) producono **ciascuna la propria** riga dichiarata?
>
> - 🟢 tutte e quattro, ognuna la sua, nell'ordine
> - 🔴 anche una sola manca, oppure risponde con la riga di un'altra — e si scrive **quale** delle quattro
>   e **con quale riga** al posto di quella attesa

⚠️ L'ordine conta: «senza reazione armata rifiuta» si osserva **una volta sola**, prima di armare, perché
`PlannedReactionAbility` non si azzera con un disarmo.

---

## 9. Come si scrive il referto

Per ciascuna voce, e **prima** di toccare il registro:

```
voce · esito (PASS / FAIL / NOT RUN) · ciò che hai visto, con le tue parole
commit dichiarato · SHA del binario · quale Play
ciò che NON hai guardato, e perché
```

⛔ **Non allargare il criterio mentre lo esegui.** Ha un owner —
[`test-manuali-pie.md`](../test-manuali-pie.md) — e l'evidenza in più si registra come *«fuori dal
criterio»*, non come parte di esso. Un verdetto che risponde a una domanda diversa da quella posta è
indistinguibile da un verdetto giusto, e nessuno lo rileggerà.

⚠️ E un `NOT RUN` dichiarato batte sempre un verde ottenuto in finestra sporca: se il motore è occupato,
se il binario non è quello, se non sei sicuro di che cosa hai visto — scrivilo. Costa un giro; un verdetto
falso costa la fiducia in tutti gli altri.
