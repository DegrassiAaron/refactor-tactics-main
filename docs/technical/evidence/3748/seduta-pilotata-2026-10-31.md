# Seduta pilotata dall'assistente: `PIE-HUD-SCHEDA` ([#3748](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3748)), 2026-10-31

**Che cosa è.** Le tavole `V1`-`V6`, da una finestra di gioco (`-game`, non la PIE dell'Editor) pilotata da fuori con input sintetico, per dare all'autore qualcosa da giudicare: la barra delle skill che segue l'unità cliccata in sola lettura (`V1`-`V4`), il box con i dettagli della skill sotto il cursore (`V5`) e `Conferma`/`Invio` vivi con la barra in sola lettura (`V6`). **Che cosa non è.** Non è la PIE e non è un build impacchettato: la voce `PIE-HUD-SCHEDA` resta ⏳ in [`test-manuali-pie.md`](../../test-manuali-pie.md). Nessun giudizio di chi ha pilotato: il giudizio è dell'autore, riportato in fondo.

## Condizioni

* `V1`-`V4`: ramo `issue/3748-scheda-pannello` a `68efec14d` (primo giro: barra in sola lettura, con la scheda di prima del box nuovo). `V5` e `V6`: lo stesso ramo a `8ca0b0d90` (merge di `origin/main`, secondo giro).
* Finestra: `UnrealEditor.exe RefactorTactics.uproject /Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox -game -windowed -ResX=1920 -ResY=1080 -WinX=0 -WinY=-40 -RTScenario=PieSeduta.SchedaBench -RTPlanningSeconds=3600 -LiveCoding=false -nosplash -abslog=<scratchpad di sessione>`. Un solo processo, mio, chiuso a fine seduta dopo aver letto la riga di comando (`-abslog` nominava la mia sessione).
* Catture con `PrintWindow` sul client 1920x1080, input sintetico (click, tasti, rotella); il cursore va parcheggiato lontano dal bordo prima di ogni click, altrimenti la camera scorre da sé.
* Il riquadro rosso nelle tavole è il cursore, disegnato a mano: la cattura non prende il puntatore.
* Banco `PieSeduta.SchedaBench`: uno scenario del Lab (`Saved/RTLab/Scenarios`, non versionato, rimosso a fine seduta), riportato in fondo. Aevik e Muiren sono propri; Branth è nemico in vista; Ivrin è nemico al buio alle spalle di Aevik, per il caso del nemico non osservato.

## Tavole

| Tavola | Mostra |
|---|---|
| `V1-la-barra-segue-il-click.png` | prima e dopo il click su Branth: la barra mostra le sue skill, senza lettere dei tasti. ⌫ *Qui «Conferma» è spenta: l'autore ha deciso il contrario il 2026-10-31, vedi `V6`.* |
| `V2-hover-sugli-slot.png` | il cursore su uno slot della barra di Branth: portata viola sulla mappa e tooltip; per scatti e reazioni nessuna portata (#3507) |
| `V3-il-piano-non-si-scrive.png` | con la barra in sola lettura un tasto, un click sulla mappa e un click su uno slot non armano né pianificano, e il log dice perché. ⌫ *Il log elenca anche `Enter ignorato`, e «Conferma» è spenta: con la decisione del 2026-10-31 `Invio` e «Conferma» funzionano, vedi `V6`.* |
| `V4-come-si-torna-alla-barra-propria.png` | il ritorno alla barra propria (tasto destro, click su una propria unità, `N`); «Conferma» spenta nella tavola di partenza, come in `V1` |
| `V5-box-con-i-dettagli-della-skill.png` | il box: testata compatta con l'invito, poi i dettagli della skill sotto il cursore (tooltip, righe `+` e `-` del pezzo), poi il cursore fuori |
| `V6-conferma-e-invio-con-la-barra-in-sola-lettura.png` | `Conferma` e `Invio` con Branth fissato: dichiarano e ritirano il piano di Aevik |

## Che cosa si vede

1. **`V5`, senza cursore.** La testata ha il nome, PV e scudo, il verso, **una sola riga** «Equipaggiamento: Impatto · Copertura portatile · Pulizia automatica» e la riga d'invito «Passa su una skill della barra per i dettagli». Nessuna riga di kit, nessuna sezione «Ricariche».
2. **`V5`, con il cursore.** Su «Colpo d'impatto» compaiono titolo, descrizione, fase, slot, portata e danno, poi le righe `+` e `-` della variante d'arma. Su «Pannello cinetico» e «Colpo di mortaio» (nessun pezzo) solo le righe del tooltip. Su «Copertura portatile» e «Pulizia automatica» (un gadget e un modulo) le righe `+` e `-` del pezzo che le concede. Uscendo dallo slot torna la testata con l'invito.
3. **`V6`.** Con Branth fissato `Conferma` resta accesa. Il click la porta a «Ritira», un secondo click a «Conferma»; `Invio` fa lo stesso. Il log del gioco, nell'ordine: `BP_Unit_Aevik_C_0: piano DICHIARATO`, `piano ritrattato`, `piano DICHIARATO`, `piano ritrattato`.
4. **Le tavole `V1`-`V4`** sono del primo giro e hanno due parti superate. La scheda a destra elenca le righe `+` e `-` di tutti i pezzi, senza box: la sostituisce `V5`. Il pulsante «Conferma» è spento con la barra sul nemico, e il log di `V3` dice che `Invio` è ignorato: l'autore ha rovesciato quel punto, e `V6` mostra il comportamento nuovo. Per la barra (le sue skill, la portata, il rifiuto di scrivere il piano con un tasto, un click sulla mappa o uno slot, il ritorno) restano valide.

## Misurato e non misurato

| | Esito |
|---|---|
| Barra di Branth in sola lettura, portata e tooltip sugli slot (`V1`, `V2`) | visto |
| Il piano non si scrive; il ritorno alla barra propria (`V3`, `V4`) | visto |
| Box con e senza cursore, con una skill con pezzo e una senza (`V5`) | visto |
| `Conferma` e `Invio` con la barra in sola lettura (`V6`) | visto, con le quattro righe di log sopra |
| Click su uno slot o tasto-skill con la barra in sola lettura (rifiuto) in `V6` | non ripetuto: lo prova `RefactorTactics.PlayerInput.AReadOnlyBarRefusesEveryWayOfWritingThePlan` (in `V3` c'è lo stesso rifiuto, ripreso dal log); `Conferma` accesa la prova `RefactorTactics.ScreenHud.ConfirmStaysEnabledWhileTheBarIsReadOnly` |
| Riga della stima di ricarica dentro i dettagli | `NOT RUN` in finestra (il banco non ha usi visti); la provano `RefactorTactics.UI.InspectCardKeepsTheEstimateInTheVisibleWindow` e `RefactorTactics.UI.InspectDeducedLinesReadAsAnEstimate` |
| Criterio (m): la fissazione cade al turno nuovo | `NOT RUN` in finestra (nel banco il turno non si chiude: `Ready 1/2`); lo prova `RefactorTactics.HexMatch.ThePinFallsWhenANewPlanningTurnStarts` |
| Dettagli oltre `MaxRighe` (la scheda più lunga con due stati e la stima) | `NOT RUN` in finestra; limite dichiarato in [D-562](../../../decisions/RT_PDR_00_Decision_Log.md) |
| PIE, packaged | `NOT RUN` |

Limiti dichiarati, che le tavole non nascondono: la riga di stato in basso e il pannello in basso a sinistra seguono ancora la selezione (dicono «Scegli un'azione» e mostrano Aevik con la barra su Branth); nessuna etichetta «sola lettura»; gli slot di Branth sono tutti «disponibili», cioè «senza stato», non «pronti»; il gate delle fasce non vede le righe che vanno a capo.

## Il giudizio d'autore (2026-10-31)

* `V1`-`V4`: «sì» per ciascuna (nel primo giro).
* `V5`: «Sì, va bene così». `V6`: «Sì, va bene così».
* Dettagli oltre `MaxRighe`: «Lascia com'è, limite dichiarato». Nessun cambio di `MaxRighe` e nessun hover «appiccicoso» in questa fetta.

Il «sì» vale per le tavole di una finestra `-game`: non è un verdetto di PIE né di packaged, e non chiude la voce.

## Banco `PieSeduta.SchedaBench`

```json
{
  "scenarioId": "PieSeduta.SchedaBench",
  "tags": [
    "planning",
    "hud",
    "pie"
  ],
  "version": 1,
  "seed": 0,
  "fixture": "TestArena",
  "_nota": "Banco della seduta PIE di #3748 (PIE-HUD-SCHEDA). Non versionato: sta in Saved/RTLab e si cancella a fine seduta. Come PieSeduta.HoverBench (Aevik accanto al muro, rivolto a est; Muiren e Branth a est) piu' Ivrin nemico al buio alle spalle di Aevik, per il caso del nemico non osservato.",
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
    },
    {
      "id": "B2",
      "hero": "Hero.Ivrin",
      "team": 1,
      "cell": [
        -4,
        0,
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
      "type": "UnitAlive",
      "unit": "A1",
      "value": true
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
      "type": "UnitAlive",
      "unit": "A2",
      "value": true
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
      "unit": "B1",
      "value": true
    },
    {
      "type": "UnitAtCell",
      "unit": "B2",
      "cell": [
        -4,
        0,
        0
      ]
    },
    {
      "type": "UnitAlive",
      "unit": "B2",
      "value": true
    }
  ]
}
```
