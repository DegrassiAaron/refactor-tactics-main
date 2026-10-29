# Seduta pilotata dall'assistente: `PIE-HUD-FUOCO-AREE` e `PIE-HUD-SKILL-PORTATA` ([#3750](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3750)), 2026-10-29

**Che cosa è.** Quattro tavole di una finestra di gioco (`-game`, non la PIE dell'Editor) pilotata da fuori con input sintetico, per dare all'autore qualcosa da giudicare. **Che cosa non è.** Nessun giudizio: le due voci restano ⏳, e le osservazioni qui sotto sono ciò che si vede, non ciò che è giusto.

## Condizioni

* `main` = `648ebcfd8`, clone principale pulito, build dell'Editor verde (nessuna riga `warning C` / `error C`), `UnrealEditor-RefactorTactics.dll` md5 `0C24897029757B87CF1315482345A5B0`.
* Finestra: `UnrealEditor.exe RefactorTactics.uproject /Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox -game -windowed -ResX=1920 -ResY=1080 -WinX=0 -WinY=-40 -RTScenario=PieSeduta.HoverBench -RTPlanningSeconds=3600 -LiveCoding=false -nosplash -abslog=<scratchpad della sessione>`. Cattura con `PrintWindow` (client 1920x1080); input con `SetCursorPos`, `mouse_event` e `keybd_event`; camera a mano (`N`, rotella).
* Nessun altro processo Unreal all'avvio. Un solo processo, mio, chiuso a fine seduta dopo aver letto la riga di comando.
* Banco `PieSeduta.HoverBench`: uno scenario del Lab (`Saved/RTLab/Scenarios`, non versionato, rimosso a fine seduta), riportato in fondo. Sta su `TestArena`: Aevik accanto al muro che blocca la vista, rivolto a est; Muiren dall'altra parte, che vede le celle dietro il muro; Branth in vista.

## Tavole

| Tavola | Voce | Mostra |
|---|---|---|
| `T1-PIE-HUD-SKILL-PORTATA-tre-stati.png` | `PIE-HUD-SKILL-PORTATA` | prima, cursore su «Impulso ad arco», cursore uscito. In B: 1 libero, 2 bloccato, 3 senza verdetto |
| `T2-PIE-HUD-FUOCO-AREE.png` | `PIE-HUD-FUOCO-AREE` | ventaglio di Aevik, `LMB` su Branth, dopo un waypoint |
| `T3-fuoco-e-hover.png` | entrambe | fuoco su Branth, cursore sullo slot, cursore uscito |
| `T4-N-a-cursore-fermo.png` | `PIE-HUD-SKILL-PORTATA` | `N` con il cursore fermo sullo slot, poi fuori e di nuovo dentro |

## Che cosa si vede

1. **`NativeOnMouseEnter` arriva su un `WBP_RT_ActionSlot` vero.** Passando sullo slot 1 compare la portata (T1 B). Provato sullo slot 1 di Aevik e su quello di Muiren; gli altri slot non sono stati provati uno per uno.
2. **Il ventaglio proprio cede mentre il cursore è sullo slot e torna all'uscita** (T1 A, B, C).
3. ⌫ **Corretto il 2026-10-29: i tre stati NON sono presenti in T1 B.** Il contorno pieno grande sulle celle libere intorno ad Aevik e il tratteggio sulle celle al buio ci sono; le celle dietro il muro fino a Muiren, che questa riga chiamava «bloccate», erano *libere* e solo più lontane dalla camera (vedi la seconda tornata, sotto). *La stesura precedente diceva: «contorno pieno più piccolo sulle celle dietro il muro fino a Muiren»*. La differenza di dimensione fra libero e bloccato è a occhio, non misurata.
4. **Il fuoco**: `LMB` su Branth toglie il ventaglio di Aevik e mostra l'area del nemico con contorno spezzato (T2 B); un waypoint di Aevik la toglie e riporta il ventaglio con la rotta (T2 C). Il log dice `Piano: ... -> 1 waypoint`.
5. **Fuoco e hover insieme**: la portata della skill prende il posto delle aree di Branth, e all'uscita tornano senza un nuovo click (T3).
6. **`N` a cursore fermo**: il dock si ricostruisce con il kit di Muiren, Slate manda un ingresso al nuovo slot e l'hover passa alla portata di Muiren senza che il cursore si muova (T4 B). Non resta nessuna portata di Aevik. **Questo smentisce la scelta 20 del piano**, che diceva che per rivedere la portata il cursore dovesse rientrare: corretta insieme a questa nota.
7. **Il tooltip dello slot** copre gli slot vicini e una fascia bassa del 3D: sul fotogramma 1920x1080 il tooltip di «Impulso ad arco» sale fino a y=855 e quello di «Getto in pressione» fino a y=828, mentre il 3D finisce a y=885.

## Misurato e non misurato

| | Esito |
|---|---|
| Hover sullo slot 1 (Aevik, Muiren) | visto |
| Uscita dallo slot | visto |
| Piano che cede e torna | visto |
| Fuoco, waypoint, fuoco più hover | visto |
| `N` a cursore fermo | visto, con l'esito del punto 6 |
| Giudizio d'autore su leggibilità, resa e disturbo del piano che sparisce | `NOT RUN`: è dell'autore |
| Nemico `Slow` e `Unbalanced` (restringimento, perdita della banda di corsa) | `NOT RUN`: lo scenario non applica stati |
| Nemico fuori vista | `NOT RUN` in PIE: coperto solo da test Automation |
| `RMB` che chiude la scheda | `NOT RUN`: la scheda (#3748) non esiste |
| Skill su se stessi, reazione, senza portata | `NOT RUN` in PIE: coperto solo da test Automation |
| Slot distrutto senza evento di uscita | `NOT RUN`: non si è riusciti a provocarlo in questo banco |
| 1440p e 4K, più sedute della camera | `NOT RUN` |

## Estratti di log

```
[2026.10.29-09.30.39:323][  0]LogRT: Warning: [RT-Test] AUTO-RUN PieSeduta.HoverBench (da: riga di comando -RTScenario=): 0 turni, pausa 1.5s — avanza un passo per frame
[2026.10.29-09.30.39:353][  0]LogRT: [RT] Turno 1 - pianificazione
[2026.10.29-09.31.03:159][796]LogRT: [RT] Selezionata: BP_Unit_Aevik_C_0
[2026.10.29-09.31.03:160][796]LogRT: Display: [RT] N (ciclo della selezione): selezionata 'BP_Unit_Aevik_C_0' (1 di 2 comandabili)
[2026.10.29-09.34.13:632][ 99]LogRT: [RT] Piano: BP_Unit_Aevik_C_0 -> 1 waypoint (costo 1/10)
[2026.10.29-09.34.41:646][194]LogRT: [RT] Selezionata: BP_Unit_Muiren_C_0
[2026.10.29-09.34.41:646][194]LogRT: Display: [RT] N (ciclo della selezione): selezionata 'BP_Unit_Muiren_C_0' (2 di 2 comandabili)
```

## Il banco

```json
{
  "scenarioId": "PieSeduta.HoverBench",
  "tags": [
    "planning",
    "hud",
    "pie"
  ],
  "version": 1,
  "seed": 0,
  "fixture": "TestArena",
  "_nota": "Banco della seduta PIE di #3750 (PIE-HUD-SKILL-PORTATA, PIE-HUD-FUOCO-AREE). Non versionato: sta in Saved/RTLab e si cancella a fine seduta. Aevik accanto al muro che blocca la vista (0,-2..2) e rivolto a est; Muiren dall'altra parte, quindi le celle dietro il muro sono in vista della squadra ma la linea di Aevik e' bloccata (stato 'bloccato'); le celle al buio dietro Aevik sono 'senza verdetto'; Branth e' in vista di Muiren (fuoco).",
  "units": [
    {
      "id": "A1",
      "hero": "Hero.Aevik",
      "team": 0,
      "cell": [
        -1,
        0,
        0
      ],
      "facing": "E"
    },
    {
      "id": "A2",
      "hero": "Hero.Muiren",
      "team": 0,
      "cell": [
        3,
        0,
        0
      ]
    },
    {
      "id": "B1",
      "hero": "Hero.Branth",
      "team": 1,
      "cell": [
        3,
        -2,
        0
      ]
    }
  ],
  "turns": [],
  "expect": [
    {
      "type": "TurnsCompleted",
      "value": 0
    },
    {
      "type": "UnitAtCell",
      "unit": "A1",
      "cell": [
        -1,
        0,
        0
      ]
    },
    {
      "type": "UnitAtCell",
      "unit": "A2",
      "cell": [
        3,
        0,
        0
      ]
    },
    {
      "type": "UnitAtCell",
      "unit": "B1",
      "cell": [
        3,
        -2,
        0
      ]
    },
    {
      "type": "UnitAlive",
      "unit": "A1",
      "value": true
    },
    {
      "type": "UnitAlive",
      "unit": "A2",
      "value": true
    },
    {
      "type": "UnitAlive",
      "unit": "B1",
      "value": true
    }
  ]
}
```

## Seconda tornata: il codice colore in gioco (2026-10-29, [D-561](../../../decisions/RT_PDR_00_Decision_Log.md))

`main` = `584af02f8` (con il codice colore di [PR #3943](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3943)), clone principale pulito, build dell'Editor verde, `UnrealEditor-RefactorTactics.dll` md5 `1CED9FD658BE54705C21298273EFF8F6`. Stesso banco, stessa finestra `-game`, stesso apparato; un solo processo Unreal, mio, chiuso a fine seduta. Tavola: `T5-PIE-HUD-SKILL-PORTATA-codice-colore.png`.

### Una correzione alla prima tornata

**In `T1` l'etichetta «2 = bloccato» era sbagliata.** Le celle dietro il muro, fino a Muiren, erano *libere*: «Impulso ad arco» è un attacco base e ha la politica di tiro `BlindAimDirect` (D-490), per cui in pianificazione la linea non si chiede e ogni cella in vista è libera. Sembravano «più piccole» perché sono più lontane dalla camera, non perché fossero nello stato bloccato (che allora si disegnava a scala 0,7). Lo stato bloccato in `T1` non c'era: la tavola mostrava **due stati su tre**, libero e senza verdetto. La differenza di dimensione che l'autore ha giudicato poco leggibile (`non molto`) era in parte la prospettiva. Lo stato bloccato vero compare con una skill a linea di tiro richiesta, come «Scarica lineare» (slot 2 di Aevik). Ho verificato sul dato (la politica di `MakeBasicAttack`) e a schermo, a occhio: «Impulso ad arco» (Aevik) e «Getto in pressione» (Muiren), le due azioni base, non mostrano mai rosa; «Scarica lineare» (Aevik) e «Marea circolare» (Muiren) sì. ⚠️ Un contatore di pixel che avevo scritto per cercare il rosa dava zero anche su «Scarica lineare», dove il rosa si vede benissimo: non vale come prova, e gli altri slot non li ho guardati uno per uno. La scelta del codice colore ([D-561](../../../decisions/RT_PDR_00_Decision_Log.md)) non dipende da quell'etichetta, ma la tavola su cui è stata presa non mostrava i tre stati: `T5` li mostra.

### Che cosa si vede in `T5`

1. **I tre colori compaiono.** Su «Scarica lineare»: viola sulle celle libere intorno ad Aevik, **rosa sulla colonna dietro il muro** (la linea di Aevik è fermata dal muro, e le celle sono in vista grazie a Muiren), ambra tratteggiato sulle celle al buio.
2. **Il rosa spedito a schermo si legge come un rosso vivo**, non come un rosa. È il rischio dichiarato in D-561 (5): oggi non convivono con l'`Attack` perché la portata armata non ha il bloccato. È il punto da giudicare.
3. **Con l'attacco base il bloccato non compare mai** (`T5` C; visto su «Impulso ad arco» e su «Getto in pressione»). È coerente con D-490 (mira cieca in pianificazione), ma vuol dire che per il giocatore il bloccato si vede con certe skill e non con altre. Gli altri slot non sono stati guardati uno per uno.
4. **La dimensione è la stessa per tutti e tre**: la differenza passa solo dal colore.

### Che cosa NON si è visto

* Il colore sui fondi scuri, a 1440p e a 4K, e con altri banchi (una sola mappa, un solo muro).
* Il ritorno del ventaglio all'uscita e la resa su un nemico a fuoco con i colori nuovi: dalla prima tornata (`T1`-`T4`) la logica non è cambiata, ma i colori sì.
* Un banco in cui il bloccato è il caso comune: qui è una colonna di due celle.
* Il giudizio d'autore: resta suo, e la voce resta ⏳.
