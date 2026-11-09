# Seduta pilotata dall'assistente: `PIE-HUD-FUMETTO` ([#4025](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4025)), 2026-11-09

**Che cosa è.** Le tavole di una finestra di gioco (`-game`, non la PIE dell'Editor) pilotata da fuori con input sintetico, per i criteri del fumetto della skill: §7, riga PIE, della [spec](../../../superpowers/specs/2026-11-08-fumetto-skill-design.md), più i due anti-obiettivi di layout di §3.4 (L1, L2). Le ha raccolte un passaggio **indipendente**, che non ha scritto codice di #4025 (CLAUDE.md §6 e §10: chi ripara non firma). **Che cosa non è.** Nessun giudizio d'autore: la resa del fumetto, e se sia «sobrio», restano suoi. L'esito di ogni criterio vive nella cella di `PIE-HUD-FUMETTO` in [`test-manuali-pie.md`](../../test-manuali-pie.md), che ne è l'unico owner (R-6); qui c'è ciò che si vede, e con che cosa.

## Condizioni

* **Codice**: clone principale in `HEAD` staccato a `ab05e9b55` (`feat/4025-fumetto-skill`, overlay 220×200) per tutti i criteri; il criterio L1 è rimisurato a `5167ae133` (overlay 220×228), dopo la correzione che la prima misura ha chiesto.
* **Suite**: `RefactorTactics` intera a `ab05e9b55` sul clone principale, build unity piena senza esclusioni adattive: `Found 3702 automation tests`, 3702 `Result={Success}`, `TEST COMPLETE. EXIT CODE: 0` (esito di quel passaggio; il log sta fuori dal repository).
* **Finestra**: `UnrealEditor.exe RefactorTactics.uproject /Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox -game -windowed -ResX=1920 -ResY=1080 -WinX=0 -WinY=0 -RTScenario=<banco> -RTPlanningSeconds=3600 -LiveCoding=false -nosplash -abslog=<scratchpad della sessione> -dpcvars=rt.Debug.PlaybackControls=1,rt.Debug.PlaybackStartPaused=1,t.MaxFPS=60`. I comandi di playback accesi servono a (h) (`K` mette in pausa) e a far partire ogni playback fermo, così che la raffica di catture cominci da un istante noto.
* **Apparato**: catture `PrintWindow` per PID a raffica (circa una ogni 0,15 s), client 1920×1080; input sintetico; camera a mano (rotella, `W`/`S`). Gli orari delle tavole sono quelli del log (UTC), che è l'oracolo del tempo: ogni istante è contato dalla riga `Attiva:` del motore.
* ⚠️ **La riga del log `modulo compilato il Nov  9 2026 alle 10:48:33` non identifica il binario**: è identica in tutti i giri, anche in quello a `5167ae133`, perché viene da `__DATE__`/`__TIME__` di `RTFrontendNavigator.cpp`, che nessun commit di #4025 tocca. Che a `5167ae133` girasse il binario nuovo lo dice l'effetto: il salto di 18 px, ripetuto in due giri a 220×200, a 220×228 non c'è.

## Le tavole

Le tavole sono **JPEG** (qualità 92, croma piena): gli originali PNG pesavano circa 25 MB, troppo per il repository. Ogni misura di pixel citata qui e sulle tavole (righe della barra, «identico al pixel») è fatta sui fotogrammi PNG originali, che stanno fuori dal repository con i log; la compressione non tocca gli esiti. Ogni tavola porta in calce la domanda, che cosa mostra, che cosa **non** mostra e l'esito proposto.

| Punto | Domanda | Esito | Tavola |
|---|---|---|---|
| (a) | una skill propria mostra il fumetto per circa 3 s sopra il nome? | **PASS**: pieno fino a +2,65 s, in dissolvenza a +2,96, sparito a +3,11 (turno a x1, quindi tempo di parete = tempo del playback) | `tav_a_skill_propria.jpg` |
| (b) | una skill nemica **visibile** lo mostra? | **PASS**, col contorno nel colore della squadra nemica | `tav_b_skill_nemica_visibile.jpg` |
| (c) | un nemico **non osservato** che usa una skill non mostra nulla? | **PASS**: B2 (Muiren avversaria, fuori vista) si cura, il resolver lo registra, a chi guarda non arriva né la riga `Attiva:` né un fumetto | `tav_c_nemico_non_osservato.jpg` |
| (d) | due unità attivano nello stesso Blast e i due fumetti sono vivi insieme? | **PASS** | `tav_d_due_fumetti_stesso_blast.jpg` |
| (e) | in vista strategica il fumetto sparisce **e** la riga `Attiva:` sta nel log a destra? | **FAIL sulla seconda metà**: il fumetto sparisce; il Registro a destra mostra solo `BLAST #1 Colpita — 13` e `BLAST #2 Colpita — 27`, nessuna voce dice quale skill è partita. Seguito: [#4033](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4033) | `tav_e_vista_strategica.jpg` |
| (e′) | tornando in tattica entro 3 s il fumetto ricompare? | **PASS**, anche quello nato **durante** la vista strategica (`SOVRACCARICO` di A1, età 0,6 s) | `tav_e_vista_strategica.jpg` |
| (f) | a fine playback nessun fumetto resta in pianificazione? | **PASS** | `tav_f_fine_playback.jpg` |
| (g) | un attacco base non grida? | **PASS** (`Hero.Branth.ImpactShot`); fuori criterio, non grida nemmeno la generica `Action.Brace` | `tav_g_attacco_base.jpg` |
| (h), pausa del playback | messo in pausa il playback (`K`), il fumetto resta fermo e riprende? | **PASS**: due catture a 1,97 s di distanza in pausa hanno l'interno del fumetto identico al pixel; alla ripresa riparte e svanisce a ~3,05 s di playback | `tav_h1_pausa_playback_K.jpg` |
| (h), `ESC` | con `ESC` il fumetto resta fermo e riprende col playback? | **FAIL rispetto alla domanda, non un difetto del fumetto**: `ESC` apre il menu e non sospende niente (`RTFrontendNavigator.h:290-310`: nessun `SetPause`, nessuna dilatazione, nessun flag nel `TurnManager`), quindi il playback continua e il fumetto lo segue. La premessa di A8 («il playback si ferma (ESC, `Next Action`)») era imprecisa; che il fumetto debba seguire il playback anche col menu aperto è un `Ruling` di chi scrive (spec, sotto A8). ⚠️ I tempi della didascalia sono imprecisi: vedi sotto | `tav_h2_ESC.jpg` |
| (i) | se una skill nemica apre una finestra di reazione, il suo fumetto resta visibile per tutta la finestra? | **PASS**: `CARICA D'ARIETE` resta visibile e fermo (interno identico al pixel) mentre il pannello conta da 2,9 a 0,1 s; dopo la chiusura il fumetto non c'è | `tav_i_finestra_reazione.jpg` |
| L1 a 220×200 | con fumetto, cifra del danno e una riga di stati insieme, nome, barra e stati restano interi? | **niente tagliato, ma il blocco si sposta**: nome, barra e stati scendono di 18 px verso l'unità per circa 0,9 s (barra da y 626-632 a 644-652), in due giri. Corretto in `5167ae133` (220×228) | `tav_L1_L2_layout.jpg` |
| L1 a 220×228 | lo stesso, senza il salto? | **PASS**, rimisurato a `5167ae133`: barra a y 639-645 prima e dopo, 638-646 con la cifra (stesso centro, un pixel di spessore in più per lato: il lampo della barra, che c'era anche a 200) | `tav_L1_228.jpg` |
| L2 | il fumetto copre il nome o la barra della stessa unità? | **PASS**: sta sopra il nome con un margine e non copre né nome né barra; misura circa 185×30 px a 1920×1080, una riga in maiuscolo | `tav_L1_L2_layout.jpg` |

## I due FAIL, e dove vanno

**(e), la riga `Attiva:` nel log a destra.** La stringa esiste, ma va solo nel log del motore e in `RecentEvents`: `ARTTurnManager::ShowActivation` la scrive con `AddLogEvent`, e l'accesso per squadra, `GetRecentEventsForTeam`, non ha nessun chiamante nei sorgenti non di test (`git grep -n GetRecentEventsForTeam -- Source` fuori dai `Tests/`: la dichiarazione, la definizione e tre commenti). Il Registro a destra si compone dal `TurnLog` tipizzato, e il canale testuale «non entra in questa catena, e non deve» (`RTHudViewModel.h:1360-1361`). La premessa di R8 della spec — *«Il log a destra porta già la riga `Attiva:`»* — era falsa, ed era la sola parte di D-582 (3) che la fetta dava per fatta senza lavoro. Il seguito è [#4033](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4033).

**(h) con `ESC`.** Non è un difetto del fumetto: il fumetto segue l'orologio del playback (R4) in entrambi i casi, e il playback con `ESC` non si ferma, per scelta (`ESC` apre il menu di pausa, che «Non sospende niente»). La pausa vera del playback è `K` (`ARTTurnManager::PausePlayback`, con `rt.Debug.PlaybackControls 1`), e con quella (h) è verde. È la premessa di A8 a nominare `ESC`; la spec lo annota accanto ad A8. Che il fumetto segua il playback anche col menu aperto è però una lettura di chi scrive, non dell'autore: la spec la marca `Ruling`, revocabile, col suo costo (con il menu di `ESC` aperto il fumetto continua e si dissolve dietro al menu; se l'autore vuole che `ESC` fermi tutto, è lavoro del frontend, non del fumetto).

⚠️ **La didascalia di `tav_h2_ESC.jpg` è imprecisa sui tempi**, e la tavola non si rigenera: dice «pieno fino a +2,42 s, in dissolvenza a +2,59 s, sparito a +2,74 s». Sui PNG originali, ritagliati attorno ad A1, il fumetto è pieno in f019 (+2,59 s), ancora visibile e appena sbiadito in f020 (+2,74 s, ed è anche nel riquadro f020 della tavola), quasi trasparente in f021 (+2,89 s) e assente in f022 (+3,05 s). La conclusione non cambia, e anzi è più netta: col menu aperto il fumetto vive circa 3 s, come senza pausa. In f019 compare la cifra «-18» e il blocco scende di circa 18 px (l'overlay era ancora a 220×200): è una spiegazione possibile del rilevatore a finestra fissa della didascalia, non verificata.

## Che cosa le tavole NON mostrano

* **Il giudizio d'autore.** La forma è un rettangolo arrotondato con un contorno sottile nel colore di squadra (pastello) e testo scuro, **non** il «burst» a punte con contorno scuro spesso di §5 della spec (un `Ruling`). Se si legge come un grido manga, e se è «sobrio» (A3), lo dice l'autore sulle tavole (`G17`, domanda `fumetti`, [#3997](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3997)). Il carattere non è stato verificato.
* **Il pop di comparsa** (scala da 1,15 a 1,0 in circa 0,1 s): la raffica cattura un fotogramma ogni ~0,15 s e non lo risolve. Le durate di (a) sono misurate a ±0,15 s.
* **Una seconda attivazione sulla stessa unità** che sostituisce la prima (R5): nessuna tavola la mostra.
* **`Next Action` e lo Skip**, che R4 e R7 nominano: non provati.
* **Lo slow-motion** è visto solo in (i): il turno della finestra scorre a x0,35 (`Risoluzione: 3 fasi, ~8.6s (x0.35, …)`), e 3 s di playback durano circa 8,6 s di parete. Una finestra di Overwatch e una risposta del giocatore alla finestra (lasciata scadere: `HOLD`) no.
* **Il taglio esatto a fine finestra**: l'ultima cattura della raffica è 0,11 s prima della chiusura; la tavola mostra lo stato dopo.
* **La posizione a schermo della cella di B2 in (c)**: non verificata con un click; si sa che sta nella zona velata.
* **Gli attacchi base degli altri eroi** non sono in tavola (quello di Aevik, `Hero.Aevik.ArcPulse`, è passato senza fumetto nelle catture del terzo turno del primo giro).
* **PIE dell'Editor, pacchetto, 1440p e 4K**: `NOT RUN`.

**Fuori dai criteri**, due fatti. In (i): il pannello della finestra di reazione («Irrigidimento — Muiren») copre parte dell'overlay di un'altra unità (Branth). In (h) con `K` (`tav_h1_pausa_playback_K.jpg`, giro 1, turno 3): durante la pausa la cifra «+18» della cura, che corre a orologio di parete, svanisce, mentre il fumetto «MAREA CIRCOLARE» resta fermo e opaco. Il token va a parete per disegno e il fumetto no (`progettazione-hud.md`, il fumetto della skill): con la pausa del playback i due si separano.

## I banchi

Tre scenari del Lab, scritti in `Saved/RTLab/Scenarios` del clone principale e copiati qui come evidenza (`Lab.T5.Fumetto4025.json`, `Lab.T5.Fumetto4025b.json`, `Lab.T5.Fumetto4025c.json`). Il loro `_nota` che li dice «NON versionati» è di prima della copia; il `_nota_evidenza` dice come rieseguirli (copiarli in `Saved/RTLab/Scenarios/` del clone principale). La geometria è la stessa nei tre: la squadra di chi guarda ha `visionRange` 0 (solo la consapevolezza ravvicinata, 2 celle), quindi B1 e B3 si vedono e B2, a 4-5 celle, no; A1 porta `Status.Wet`, cioè una riga di stati per L1.

* **`Lab.T5.Fumetto4025`** (primo giro): (a), (b), (d), (e), (e′), (f), (g), L2. ⚠️ Per (c) **era una prova vacua**: B2 usava `Overload` su una cella vuota e quell'intento non produce eventi (`Blast: 3 attacchi`). Il referto del banco è `ERROR` al quarto turno, per un errore del banco: `Hero.Branth.Ram` è una mobilità e si dichiara con `dash`/`dashTo`, non con `ability`. I primi tre turni sono giocati.
* **`Lab.T5.Fumetto4025b`** (secondo giro, e la rimisura di L1 a `5167ae133`): (c) rifatta con B2 che si cura da sola fuori vista, (h) con `K`, (i), L1. Una prima stesura è stata rifiutata al caricamento (`intent di 'B2': l'unita' bersaglia se stessa`) e corretta bersagliando la cella accanto. Il referto è `ERROR` (`il turno 2 non ha finito di risolvere entro 900 passi`): è il tetto di passi dell'harness con il playback a x0,35; il `TurnManager` ha proseguito e la finestra di reazione si è aperta.
* **`Lab.T5.Fumetto4025c`** (terzo giro): un turno solo, (h) con `ESC`. Referto `PASS`.

## Estratti di log

```
[2026.11.09-12.14.15:233][319]LogRT: [RT] Attiva: BP_Unit_Branth_C_0 -> Hero.Branth.ImpactShot
[2026.11.09-12.14.16:734][471]LogRT: [RT] Attiva: BP_Unit_Aevik_C_0 -> Hero.Aevik.LinearDischarge
[2026.11.09-12.14.18:229][625]LogRT: [RT] Attiva: BP_Unit_Aevik_C_1 -> Hero.Aevik.Overload
[2026.11.09-12.17.50:058][533]LogRT: [RT] Vista STRATEGICA (arm=2400, enter=2400, exit=1900)
[2026.11.09-12.17.51:123][670]LogRT: [RT] Vista tattica (arm=1100, enter=2400, exit=1900)
[2026.11.09-12.30.04:751][241]LogRT: [RT] (q=3,r=-3,L=0) -> (q=3,r=-3,L=0): 18 punti vita curati (Hero.Muiren.CircularTide, p60)
[2026.11.09-12.30.35:095][794]LogRT: [RT] Attiva: BP_Unit_Muiren_C_0 -> Action.Brace
[2026.11.09-12.30.37:387][ 67]LogRT: [RT] Attiva: BP_Unit_Branth_C_0 -> Hero.Branth.Ram
[2026.11.09-12.30.43:656][807]LogRT: [RT] Finestra di reazione presentata: T2|P3|M0|U1|action.brace|S0 (playback trattenuto=1)
[2026.11.09-12.30.46:656][142]LogRT: [RT] Risoluzione: 3 fasi, ~8.6s (x0.35, slack x1.00)
[2026.11.09-12.39.16:003][602]LogRT: Warning: [RT-Test] FINITO Lab.T5.Fumetto4025c -> PASS (1/1 assertion, 1 turni)
```

Nel secondo giro la cura di B2 c'è nel resolver (la riga `18 punti vita curati` qui sopra) e non c'è nel feed di chi guarda: `grep -c "Attiva: BP_Unit_Muiren_C_1"` sul log di quel giro risponde `0`, ed è lo zero che (c) chiede.
