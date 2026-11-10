# Seduta pilotata dall'assistente: `PIE-HUD-REGISTRO-ATTIVA` ([#4033](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4033)), 2026-11-10

**Che cosa è.** Le tavole di una finestra di gioco (`-game`, non la PIE dell'Editor) pilotata da fuori con input sintetico, per i criteri (a)-(f) di `PIE-HUD-REGISTRO-ATTIVA` ([registro PIE](../../test-manuali-pie.md)). La voce chiede che il Registro a destra scriva la riga «unità — azione» quando il playback mostra un'attivazione: [D-600](../../../decisions/RT_PDR_00_Decision_Log.md) (1), [spec](../../../superpowers/specs/2026-11-10-registro-attivazione-design.md) §3, R1-R10. Le ha raccolte un passaggio che non ha scritto codice di #4033 (CLAUDE.md §6 e §10: chi ripara non firma), in due giri sullo stesso banco e sullo stesso binario. **Che cosa non è.** Non è un giudizio d'autore. L'ordine delle righe e il loro budget restano una lettura dell'assistente (R6 della spec), e se il Registro si legga lo dice l'autore. L'esito di ogni criterio vive nella cella della voce, che resta in attesa di quel giudizio.

## Condizioni

* **Codice**: clone principale su `main` a `f1256ca3b`.
  * Il binario `UnrealEditor-RefactorTactics.dll` (13:36 locali) è più recente di ogni file di `Source/`: `find Source -newer <dll>` non risponde nulla.
  * Fra `d8b13c3cb` (il merge di #4033) e `f1256ca3b` cambia solo il Decision Log.
* **Suite**: `NOT RUN` in questo passaggio.
* **Finestra**: la stessa riga di comando nei due giri, `UnrealEditor.exe RefactorTactics.uproject /Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox -game -windowed -ResX=1920 -ResY=1080 -WinX=0 -WinY=0 -RTScenario=Lab.T5.Registro4033 -RTPlanningSeconds=3600 -LiveCoding=false -nosplash -abslog=<scratchpad della sessione>`, più un argomento diverso per accendere i comandi di playback.
  * **Primo giro** (19:22-19:24 UTC): `-ExecCmds="rt.Debug.PlaybackControls 1"`. ⚠️ **Non ha acceso i comandi di playback.**
    * `ApplyPlaybackControlCVars` legge la CVar una volta sola, all'avvio dello scenario (`RTGameMode.cpp`, ramo `ERTScenarioStart::Started`).
    * La CVar è arrivata alle 19:23:50.207, dopo `AUTO-RUN` (19:23:47.783).
    * La riga `comandi di playback ACCESI` manca dal log: `grep -c` risponde `0`.
    * Per questo il primo giro non ha misurato la pausa.
  * **Secondo giro** (21:21-21:25 UTC): `-DPCVars=rt.Debug.PlaybackControls=1`.
    * La CVar nasce prima che il modulo di gioco la registri: `LogConfig: CVar [[rt.Debug.PlaybackControls:1]] deferred - dummy variable created`.
    * All'avvio dello scenario il log dice `comandi di playback ACCESI`.
    * È la forma che usava già la [seduta di #4025](../4025/seduta-pilotata-2026-11-09.md).
* **Apparato**:
  * catture `PrintWindow` per PID, una ogni ~0,15 s durante la risoluzione, client 1920×1080;
  * input sintetico, con la camera dello scenario e nessun movimento a mano;
  * gli orari sono quelli del log (UTC), che è l'oracolo del tempo: ogni istante è contato dalla riga `Attiva:` del motore;
  * fotogrammi e log stanno fuori dal repository.
* **Chi guarda**: la squadra 0, con `visionRange` 0, cioè solo la consapevolezza ravvicinata di 2 celle. B1 e B3 si vedono; B2, a 4-5 celle, no.

## Le tavole

Le tavole sono JPEG (qualità 92) composte dai fotogrammi. In calce ognuna porta la domanda, che cosa mostra, che cosa non mostra e l'esito proposto. I tempi «+N s» sono contati dalla riga `Attiva:` dell'azione.

| Punto | Domanda | Esito | Tavola |
|---|---|---|---|
| (a), istante e posto | la riga «unità — azione» compare quando il playback mostra l'azione, sopra la riga d'esito della stessa unità, che resta com'è? | **PASS** (primo giro, vista tattica). Le righe d'esito ci sono dall'inizio del playback. «Aevik — Scarica lineare» entra fra +0,21 e +0,36 s, sopra «Colpita — 190»; «Aevik — Sovraccarico» fra +0,12 e +0,31 s, sopra «Colpita — 130». Le righe d'esito non cambiano | `tav_a_riga_sopra_esito.jpg` |
| (a), pausa | con il playback in pausa (`K`) non arrivano righe? | **PASS** (secondo giro). `K` cade 0,16 s dopo l'inizio del playback di T2. Per 8,3 s, con tre `L` in mezzo, l'avanzamento resta a `[4%]` e il Registro a quattro righe. Alla ripresa le due attivazioni che mancavano entrano con le loro righe di log: Irrigidimento fra +0,11 e +0,26 s, Impulso ad arco fra +0,08 e +0,23 s | `tav_a_pausa_K.jpg` |
| (a), passo | al passo (`L`) le righe arrivano con le azioni? | **NOT RUN, e la premessa è imprecisa.** `L` è il micro-passo delle rotte, e in una fase senza rotte resta fermo per disegno: vedi sotto | `tav_a_pausa_K.jpg` |
| (b) | in vista strategica (`Tab`) la stessa riga compare nello stesso istante, e nessun fumetto? | **PASS** (secondo giro). Tutto il playback di T1 passa in vista strategica. «Aevik — Scarica lineare» entra fra +0,03 e +0,18 s, «Aevik — Sovraccarico» fra −0,09 e +0,10 s. Sopra le unità ci sono le cifre del danno e nessun fumetto | `tav_b_vista_strategica.jpg` |
| (c) | un nemico **non osservato** che usa una skill non produce riga, né fumetto? | **PASS**, nei due giri. In T1 B2 (Muiren avversaria, fuori vista) si cura: `180 punti vita curati (Hero.Muiren.CircularTide, p60)`. `grep -c "Attiva: BP_Unit_Muiren_C_1"` risponde `0` su entrambi i log, e nel Registro di T1 non c'è nessuna riga di Muiren | il Registro di T1 in `tav_b_vista_strategica.jpg` |
| (d) | l'attacco base, un'azione generica e un gadget hanno la riga ma non il fumetto? | **PASS per l'attacco base e l'azione generica.** «Aevik — Impulso ad arco» e «Muiren — Irrigidimento» sono nel Registro, e sopra le unità non c'è fumetto. **Per il gadget** «Branth — Copertura portatile» ha la riga, ma l'assenza del fumetto non è osservabile: Branth (A3) sta al bordo basso dell'inquadratura, senza nome né barra disegnati, in entrambi i giri | `tav_d_senza_fumetto.jpg` |
| (e) | saltando il playback a metà (`Spazio`), a fine playback ci sono le righe di tutte le attivazioni che il playback avrebbe mostrato, e nessuna doppia? | **PASS** (primo giro, ripetuto nel secondo). Il salto cade dopo due attivazioni mostrate, e dopo il salto il Registro ne ha quattro, nessuna doppia. Il feed finale (`rt.Debug.ScreenHud`) è uguale nei due giri: otto righe, di cui quattro attivazioni | `tav_e_f_salto_e_planning.jpg` |
| (f) | nel Planning successivo le righe del turno restano, sotto «ROUND nn · PRECEDENTE»? | **PASS** (primo giro). Le stesse otto righe restano dieci secondi dopo, in pianificazione, sotto «ROUND 04 · PRECEDENTE» | `tav_e_f_salto_e_planning.jpg` |

## Il passo `L`, e la premessa della voce

L'esito atteso di (a) dice «al passo (`L`) arrivano con le azioni». Per il codice non può valere in una fase senza rotte:

* `L` chiama `ARTPlayerController::OnStepPlaybackMicroStep`, che chiama `ARTTurnManager::StepMicroStep`. Questo avanza fino al prossimo confine di **micro-passo delle rotte** della fase in corso.
* Se nella fase non ci sono rotte (`MicroStepsInCurrentPlaybackPhase() <= 0`), il playback resta fermo: «Una fase senza segmenti o senza durata non ha barriere da attraversare: si resta fermi invece di inventare un avanzamento» (`RTTurnManager.cpp`).
* La Prep di T2 non ha rotte. Le tre `L` del secondo giro non hanno fatto avanzare niente: nessuna riga `Attiva:` fra la pausa (21:24:39.022) e la ripresa (47.535), e l'avanzamento è rimasto a `[4%]`.
* Il comando che ferma il playback alla prossima azione, `ARTTurnManager::RequestPlaybackStopAt`, non ha un tasto. Fuori dai test, `git grep -n RequestPlaybackStopAt -- Source ':!Source/**/Tests/**'` trova solo la dichiarazione, la definizione e due commenti.

La voce va quindi corretta, ed è corretta con una nota nella sua cella. Se il passo debba raggiungere le azioni è una domanda per chi possiede i comandi di playback, non per il Registro.

## Per l'autore

Due letture che la meccanica non decide:

* **Il ritardo della riga rispetto al fumetto.**
  * In vista tattica la riga entra fra +0,21 e +0,36 s e fra +0,12 e +0,31 s dalla riga di log nel primo giro. Nel secondo giro, dopo la ripresa, entra fra +0,11 e +0,26 s e fra +0,08 e +0,23 s.
  * In vista strategica entra entro +0,18 s e +0,10 s.
  * Il fumetto, nel primo giro, compare entro 0,06 s dalla riga di log, quindi la riga del Registro arriva uno o due fotogrammi di cattura dopo il fumetto.
  * La causa non è indagata. Resta da giudicare se a questa grana vale ancora «nell'istante in cui la mostra».
* **Dove va la riga del gadget.**
  * «Branth — Copertura portatile» sta in fondo al Registro, sotto la riga BLAST di T2 (`tav_d_senza_fumetto.jpg`, f043).
  * È ciò che R6 della spec fa con una riga d'attivazione la cui unità non ha una riga d'esito nel turno. La riga «L'ambiente cambia» (la copertura eretta), per R6, non è la riga d'esito di Branth.
  * Ne esce una riga PREP sotto una riga BLAST. R6 è una lettura dell'assistente, ribaltabile, e questa è la domanda di giudizio della voce sull'ordine.

## Che cosa le tavole NON mostrano

* **Il giudizio d'autore.** Le tavole non dicono se il Registro si legge, né se l'ordine e il budget di R6 vanno bene.
* **La PIE dell'Editor, il pacchetto, 1440p e 4K**: `NOT RUN`.
* **Il passo `L` con righe**: vedi sopra.
* **L'assenza del fumetto per il gadget**: vedi (d).
* **Il lampeggio delle righe sotto un inserimento** (il «➕ Ancora (a)» della voce, `OnLineChanged`): i fotogrammi non sono stati letti per questo, quindi non è verificato.
* **Il budget pieno**: il feed ha `budget=12` e il turno più lungo ne ha mostrate otto. Non si vede nessuna attivazione che esce.
* **Due attivazioni della stessa unità nello stesso turno**: nessun turno del banco le ha.

**Fuori dai criteri**, tre fatti.

* **T3 non è uguale nei due giri.** La finestra di reazione di T3 è finita diversamente:
  * nel primo giro «la reazione risponde «SIDESTEP»»;
  * nel secondo «la reazione non risponde in tempo, resta armata».
  * Con lei cambiano i danni di T4: «Colpita — 146» e «64» nel primo giro, «130» e «30» nel secondo.
  * La finestra prende un input del giocatore, e i tasti dello script la raggiungono in istanti diversi. Non è indagato.
* **In vista strategica le etichette delle unità** (nome, barra, punti vita) si sovrappongono e coprono la mappa: f017 di `tav_b_vista_strategica.jpg`. Non è un fatto del Registro.
* **Perché (b) e la pausa stanno nel secondo giro.** Lo script del primo giro non ha mandato il `Tab` di T1: contava i turni solo dopo l'attesa dell'avvio, e la riga del turno 1 era già passata. I tasti di T2 sono poi scivolati in T3.

## Il banco

`Lab.T5.Registro4033.json` è stato scritto in `Saved/RTLab/Scenarios` del clone principale ed è copiato qui come evidenza. Il suo `_nota_evidenza` dice come rieseguirlo. La geometria è quella dei banchi `Lab.T5.Fumetto4025*`, più A3.

* **A3** è un Branth alleato senza loadout dichiarato. Ha quindi il loadout di default dell'eroe, che porta `Gadget.PortableCover`.
* **T1**: le skill di A1 e B1, l'attacco base di B3, e la cura di B2 fuori vista. Copre (a), (b) e (c).
* **T2**: il gadget di A3, `Action.Brace` di A2 e l'attacco base di A1. Copre la pausa e (d).
* **T3**: una carica di `Hero.Branth.Ram` che apre una finestra di reazione su A2 in `Action.Brace`. È fuori dai criteri.
* **T4**, l'ultimo turno: quattro attivazioni visibili e il salto. Copre (e) e (f).

L'aspettativa è `TurnsCompleted 4`, e il referto è `PASS` in entrambi i giri.

## Estratti di log

```
# primo giro
[2026.11.10-19.23.47:783][  0]LogRT: Warning: [RT-Test] AUTO-RUN Lab.T5.Registro4033 (da: riga di comando -RTScenario=): 4 turni, pausa 1.5s — avanza un passo per frame
[2026.11.10-19.23.50:207][  0]rt.Debug.PlaybackControls = "1"
[2026.11.10-19.23.52:805][224]LogRT: [RT] Attiva: BP_Unit_Aevik_C_0 -> Hero.Aevik.LinearDischarge
[2026.11.10-19.23.54:310][350]LogRT: [RT] Attiva: BP_Unit_Aevik_C_1 -> Hero.Aevik.Overload
[2026.11.10-19.23.56:303][517]LogRT: [RT] (q=3,r=-3,L=0) -> (q=3,r=-3,L=0): 180 punti vita curati (Hero.Muiren.CircularTide, p60)
[2026.11.10-19.24.07:081][403]LogRT: [RT] Attiva: BP_Unit_Branth_C_1 -> Hero.Branth.ImpactShot
[2026.11.10-19.24.08:575][529]LogRT: [RT] Attiva: BP_Unit_Aevik_C_1 -> Hero.Aevik.LinearDischarge
[2026.11.10-19.24.09:066][572]LogRT: [RT] Risoluzione: salto
[2026.11.10-19.24.09:070][572]LogRT: Warning: [RT-Test] FINITO Lab.T5.Registro4033 -> PASS (1/1 assertion, 4 turni)

# secondo giro
[2026.11.10-21.21.54:068][  0]LogDeviceProfileManager: Setting CommandLine Device Profile CVar: [[rt.Debug.PlaybackControls:1]]
[2026.11.10-21.21.54:068][  0]LogConfig: CVar [[rt.Debug.PlaybackControls:1]] deferred - dummy variable created
[2026.11.10-21.24.26:766][  0]LogRT: Warning: [RT-Test] AUTO-RUN Lab.T5.Registro4033 (da: riga di comando -RTScenario=): 4 turni, pausa 1.5s — avanza un passo per frame
[2026.11.10-21.24.26:839][  0]LogTemp: Display: [RT] rt.Debug.PlaybackControls: comandi di playback ACCESI (Pause/Resume/Step).
[2026.11.10-21.24.31:424][  5]LogRT: [RT] Vista STRATEGICA (arm=2400, enter=2400, exit=1900)
[2026.11.10-21.24.33:840][179]LogRT: [RT] Attiva: BP_Unit_Aevik_C_0 -> Hero.Aevik.LinearDischarge
[2026.11.10-21.24.35:337][303]LogRT: [RT] Attiva: BP_Unit_Aevik_C_1 -> Hero.Aevik.Overload
[2026.11.10-21.24.37:347][439]LogRT: [RT] (q=3,r=-3,L=0) -> (q=3,r=-3,L=0): 180 punti vita curati (Hero.Muiren.CircularTide, p60)
[2026.11.10-21.24.38:024][476]LogRT: [RT] Vista tattica (arm=800, enter=2400, exit=1900)
[2026.11.10-21.24.38:862][529]LogRT: [RT] Attiva: BP_Unit_Branth_C_0 -> Gadget.PortableCover
[2026.11.10-21.24.47:860][147]LogRT: [RT] Attiva: BP_Unit_Muiren_C_0 -> Action.Brace
[2026.11.10-21.24.48:672][200]LogRT: [RT] Attiva: BP_Unit_Aevik_C_0 -> Hero.Aevik.ArcPulse
[2026.11.10-21.24.58:987][886]LogRT: Warning: [RT-Test] FINITO Lab.T5.Registro4033 -> PASS (1/1 assertion, 4 turni)
[RT] Feed: TurnLog=15 voci, osservatori={0}, righe dopo filtro=8 (di cui attivazioni=4), budget=12
```

I tasti del secondo giro, dal registro delle azioni dello script (UTC): `K` alle 21:24:39.022; `L` alle 41.579, 44.570 e 45.546; `K` di ripresa alle 47.535. Fra la pausa e la ripresa il log del motore non ha nessuna riga `Attiva:`.
