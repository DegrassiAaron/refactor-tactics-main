# `PIE-V01-PREDICTIVECLICK` e `PIE-V01-PREDICTIVEDASH`: seduta pilotata, 2026-11-13

Questa seduta misura a schermo le due voci della predittiva cliccata:
- `PIE-V01-PREDICTIVECLICK`, il click su una cella e su un'unità ([#4088](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4088)), con la resa decisa da [`D-610`](../../../decisions/RT_PDR_00_Decision_Log.md) (3) ([#4093](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4093));
- `PIE-V01-PREDICTIVEDASH`, il colpo con lo scatto nello stesso turno ([#4091](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4091), [`D-610`](../../../decisions/RT_PDR_00_Decision_Log.md) (1)).

Il metodo è quello della [seduta del 2026-11-11](../4052/seduta-pilotata-2026-11-11.md): una finestra `-game` pilotata da fuori, con input sintetico. Misura la meccanica dei criteri, non l'occhio dell'autore. Il giudizio a schermo l'autore l'ha dato sulle tavole, ed è riportato qui sotto con le sue parole.

⚠️ **Chi misura ha scritto il codice di #4091, #4093 e #4099.** Non è un verdetto indipendente. La misura avviene dopo il merge, su un commit dichiarato.

## Condizioni

- **Commit:** `main` = `2792eee41` (PR [#4100](https://github.com/DegrassiAaron/refactor-tactics-main/pull/4100)). Il clone principale è stato portato a quel commit in fast-forward, sul suo branch `docs/guida-vfx-skill`, che non aveva commit propri. L'Editor è stato compilato nel clone principale; la DLL `UnrealEditor-RefactorTactics.dll` (2026-11-13 08:28) è più recente di ogni file di `Source/`.
- **Motore:** l'Editor interattivo che l'autore aveva aperto nel clone principale è stato chiuso da lui prima della build: teneva la DLL del 2026-11-12. Durante i giri non sono girate build, suite né altri processi Unreal.
- **Finestra:** `UnrealEditor.exe … L_DevSandbox -game -windowed -ResX=1920 -ResY=1080 -RTPlanningSeconds=3600`. Catture `PrintWindow` a 1920×1080, mai massimizzata (`zoomed=False` in ogni cattura).
- **Banco:** `Visual.Input.InterceptShotClick`. Nei giri `c2`, `d1` e `d2` è stata usata una **copia di laboratorio**, `Lab.PieV01.InterceptShotClickBot` in `Saved/RTLab/Scenarios/`, identica salvo `"bot": true` su B1 e B2: vedi «Il banco non risolveva il turno». Con [#4103](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4103) il banco versionato è come la copia.
- **Camera:** `N` (selezione), `F` (fuoco su Ivrin), `E`, `Tab`, `Tab`, `E`, tre tacche di rotella. La sequenza ha dato la stessa inquadratura in ogni giro: la differenza media della scena fra due giri, alla stessa tappa, sta fra 1,3 e 2,9 (`c1`/`c2`, `c2`/`d1`, `d1`/`d2`), e viene dalle anteprime, che cambiano da un giro all'altro.
- **Giri:**
  - `g1`: interrotto. Durante il primo giro l'autore ha usato la finestra: ha cliccato due slot del dock e la cella `(-2,2)`, poi ha confermato. L'assistente ha smesso di mandare input e ha chiesto; l'autore ha risposto «Sì, ho finito: riparti tu».
  - `c1`: il banco versionato. Passi (1)-(3); il lock-in non risolve.
  - `c2`: la copia di laboratorio. Passi (1)-(4).
  - `d1` e `d2`: la copia di laboratorio. `PREDICTIVEDASH`, giro (1) e giro (2).

## Esiti

### `PIE-V01-PREDICTIVECLICK`

| Passo | Esito | Tavola |
|---|---|---|
| (1) il dock chiede una cella | **PASS**. Armato `Intercetto` col tasto `2`, il prompt in basso a sinistra dice «Scegli una cella», e le sei celle accanto a Ivrin hanno il contorno viola | `tav_1_click_predittivo.jpg` |
| (2) il click sulla cella vuota `(-2,3)` | **PASS**, due giri su due. Log `Piano: BP_Unit_Ivrin_C_0 usa Intercetto sulla cella (-2,3,0)`, nessuna riga `waypoint`. A schermo, contorno rosso sulla cella e linea da Ivrin | `tav_1_click_predittivo.jpg` |
| (3) il click sulla **mesh** di B2 | ❌ **FAIL** → [#4101](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4101). Il click sulle gambe di Muiren prende il pavimento della cella dietro di lei sullo schermo, cioè quella di Ivrin: `Cella (-3,3,L0) non bersagliabile: troppo vicino (minimo 1)`. Tre punti nel giro `c1` — (1275,372), (1292,368), (1262,362) — e uno nel giro `c2`; mai `(-2,2)`. L'hover, che usa lo stesso raggio, lo conferma: sulla colonna x=1265 il contorno giallo resta sull'esagono di Muiren fino a y=390 e passa a quello di Ivrin da y=370 in su | `tav_2_click_sul_corpo.jpg` |
| (3) il click sul **pavimento** dell'esagono di B2 | Registrato: blocca `(-2,2)`, due giri su due (`usa Intercetto sulla cella (-2,2,0)`). Il pannello a destra non passa all'ispezione | `tav_1_click_predittivo.jpg` |
| (3) come l'anteprima segna B2 | **Giudicato dall'autore sulla tavola 1**: «Sì, è quella», alla domanda «sopra Muiren non c'è una marca di bersaglio, e la cella (-2,2) è contornata di rosso. È la resa che chiede D-610, "l'anteprima di una predittiva segna la cella e non chi ci sta sopra"?» | `tav_1_click_predittivo.jpg` |
| (4) dopo il lock-in, la voce `Predictive` | **PASS** (giro `c2`). Il TurnLog, nella sua riga leggibile: `(q=-3,r=3,L=0) -> (q=-2,r=2,L=0): previsione a vuoto, nessuno e' entrato (Hero.Ivrin.InterceptShot)`. A vuoto perché Muiren nella cella c'era già | `tav_1_click_predittivo.jpg` |

### `PIE-V01-PREDICTIVEDASH`

| Punto | Esito | Tavola |
|---|---|---|
| Il dock lascia pianificare le due azioni insieme | Registrato: sì. Il pannello sinistro mostra «PRINCIPALE Intercetto» e «MOVIMENTO Lama di passaggio» | `tav_3_scatto.jpg` |
| (1) scatto su `(-3,4)`, accanto alla cella bloccata | **PASS**. `Scatto: BP_Unit_Ivrin_C_0 -> (q=-3,r=4,L0)`, poi `(q=-3,r=4,L=0) -> (q=-2,r=3,L=0): previsione a vuoto, nessuno e' entrato`: il colpo parte dalla cella d'arrivo. Nessun rifiuto | `tav_3_scatto.jpg` |
| (2) scatto su `(-4,3)`, a due celle | **PASS**. Nessuna voce predittiva; `Ivrin: (q=-4,r=3,L=0) -> (q=-2,r=3,L=0): azione annullata (fuori portata) (Hero.Ivrin.InterceptShot, p30)`, fra le righe del Move. La cella di partenza è `(-4,3)` e il motivo è fuori portata. Il dump strutturato (`rt.Debug.DumpTurnLog`) non è stato eseguito: vedi «La console» | `tav_3_scatto.jpg` |
| Che cosa mostra l'anteprima dopo lo scatto | Registrato: è la stessa nei due giri. La linea del colpo parte dalla cella di adesso, e sopra Ivrin l'etichetta dice «Intercetto ?» in tutti e due | `tav_3_scatto.jpg` |
| Il colpo che non parte, a schermo | **Giudicato dall'autore sulla tavola 3**: «Tutti e due», alla domanda «nel giro (2) il colpo non parte, ma né l'anteprima prima, né il Registro dopo lo dicono a schermo […] Va bene così o serve un segno?». Serve un segno nell'anteprima e nel Registro → [#4102](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4102) | `tav_3_scatto.jpg` |

## Il banco non risolveva il turno

Nel banco versionato Branth e Muiren non avevano `"bot": true`. La squadra 1 contava come un partecipante umano, e dopo il lock-in il log si fermava:

```
[2026.11.13-07.44.53:718][761]LogRT: [RT] Lock-in del giocatore -> risoluzione anticipata (turno 1)
[2026.11.13-07.44.53:718][761]LogRT: [RT] Ready del partecipante (squadra 0, gruppo 0) -> 1/2 (turno 1)
```

Nessun turno 2 in 120 s, con la pianificazione a 3600 s. Lo stesso nel giro dell'autore. In PIE dall'Editor non è misurato, ma la pianificazione va allungata anche lì per avere il tempo di cliccare (`rt.Match.PlanningSeconds`), e il conteggio dei partecipanti è lo stesso. Con la copia `bot: true` il lock-in dà `1/1` e il turno si risolve. [#4103](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4103) mette `bot: true` nel banco versionato.

Da bot, Branth entra in `(-2,3)` con `Hero.Branth.Ram` nella fase Dash, prima del Move. Per la predittiva non è un ingresso: per questo il giro (1) è a vuoto. Che il colpo vada a segno su chi entra non è un criterio di queste voci, e lo copre headless `AbilityLab.ThePredictiveCatchesWhoEntersItsCell`.

## Che cosa le tavole NON mostrano

- **La PIE dell'Editor**, il pacchetto, 1440p e 4K: è una finestra `-game`.
- **Il dump strutturato del TurnLog** (fase, categoria, `SrcCell` come campi): le righe citate sono le righe leggibili del log del motore.
- **Dove stia il cilindro** che fa da bersaglio al raggio del click: [#4101](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4101) chiede di misurarlo.
- **Un colpo che va a segno** su chi entra nella cella durante il Move.

## Fuori dai criteri

- **La console.** Il tasto della tilde (`VK_OEM_3`) non apre la console della finestra `-game` su questa tastiera italiana. L'`Invio` incollato dopo è arrivato al gioco e ha dichiarato il piano: `BP_Unit_Ivrin_C_0: piano DICHIARATO`. Il piano è stato ritirato con un secondo `Invio` (`piano ritrattato`). Per questo `rt.Debug.DumpTurnLog` e `show collision` non sono stati eseguiti.
- **Il colpo a vuoto non ha una riga nel Registro a schermo**, come il colpo che non parte. Dell'Intercetto il Registro mostra solo l'attivazione al Prep, «Ivrin — Intercetto». [#4102](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4102) chiede di decidere anche questo.
- **Il click sul torso di Branth**, tre punti nel giro `c1`, ha dato sempre la sua cella `(-1,3)`, fuori portata. Quei punti cadono anche sopra il suo esagono, quindi la prova non distingue la mesh dal pavimento.
- **Ogni processo è stato chiuso** dopo averne letto la riga di comando (`-abslog` nella cartella del suo giro), col suo crash reporter. A fine seduta non resta nessun processo Unreal. La copia di laboratorio è stata cancellata da `Saved/RTLab/Scenarios/`.

## Estratti di log

```
# c1 — banco versionato: il click sulle gambe di Muiren prende la cella di Ivrin
[2026.11.13-07.36.08:746][438]LogRT: [RT] Piano: BP_Unit_Ivrin_C_0 usa Intercetto sulla cella (-2,3,0)
[2026.11.13-07.37.50:628][658]LogRT: [RT] Cella (-3,3,L0) non bersagliabile: troppo vicino (minimo 1)
[2026.11.13-07.38.42:513][591]LogRT: [RT] Cella (-3,3,L0) non bersagliabile: troppo vicino (minimo 1)
[2026.11.13-07.41.39:023][ 31]LogRT: [RT] Cella (-3,3,L0) non bersagliabile: troppo vicino (minimo 1)
[2026.11.13-07.44.28:510][264]LogRT: [RT] Piano: BP_Unit_Ivrin_C_0 usa Intercetto sulla cella (-2,2,0)

# c2 — copia di laboratorio: passi (2)-(4)
LogRT: [RT] Piano: BP_Unit_Ivrin_C_0 usa Intercetto sulla cella (-2,3,0)
LogRT: [RT] Cella (-3,3,L0) non bersagliabile: troppo vicino (minimo 1)
LogRT: [RT] Piano: BP_Unit_Ivrin_C_0 usa Intercetto sulla cella (-2,2,0)
LogRT: [RT] Ready del partecipante (squadra 0, gruppo 0) -> 1/1 (turno 1)
LogRT: [RT] (q=-3,r=3,L=0) -> (q=-2,r=2,L=0): previsione a vuoto, nessuno e' entrato (Hero.Ivrin.InterceptShot)

# d1 — scatto accanto
LogRT: [RT] Piano: BP_Unit_Ivrin_C_0 SCATTO -> (-3,4,L0)
LogRT: [RT] Piano: BP_Unit_Ivrin_C_0 usa Intercetto sulla cella (-2,3,0)
LogRT: [RT] Scatto: BP_Unit_Ivrin_C_0 -> (q=-3,r=4,L0)
LogRT: [RT] (q=-3,r=4,L=0) -> (q=-2,r=3,L=0): previsione a vuoto, nessuno e' entrato (Hero.Ivrin.InterceptShot)

# d2 — scatto a due celle
LogRT: [RT] Piano: BP_Unit_Ivrin_C_0 SCATTO -> (-4,3,L0)
LogRT: [RT] Piano: BP_Unit_Ivrin_C_0 usa Intercetto sulla cella (-2,3,0)
LogRT: [RT] Scatto: BP_Unit_Ivrin_C_0 -> (q=-4,r=3,L0)
LogRT: [RT] Ivrin: resta (q=-4,r=3,L=0) (Action.Move, p50)
LogRT: [RT] Ivrin: (q=-4,r=3,L=0) -> (q=-2,r=3,L=0): azione annullata (fuori portata) (Hero.Ivrin.InterceptShot, p30)
```
