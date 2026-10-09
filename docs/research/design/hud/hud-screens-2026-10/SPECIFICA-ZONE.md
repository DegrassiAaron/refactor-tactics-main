# HUD contestuale — specifica per zona (PROPOSTA)

> **Statuto**: sorgente di design sotto `docs/research/`, **non canone**. È una `PROPOSTA` consegnata dall'autore
> il 2026-10-04; le tavole A–F sono state rifatte il 2026-10-09 sulla griglia di
> [D-456](../../../../decisions/RT_PDR_00_Decision_Log.md).
> Il canone della disposizione è [`progettazione-hud.md`](../../../../technical/systems/progettazione-hud.md) §6
> con [D-456](../../../../decisions/RT_PDR_00_Decision_Log.md), [D-458](../../../../decisions/RT_PDR_00_Decision_Log.md)
> e le voci da [D-477](../../../../decisions/RT_PDR_00_Decision_Log.md) a [D-482](../../../../decisions/RT_PDR_00_Decision_Log.md), più
> [D-488](../../../../decisions/RT_PDR_00_Decision_Log.md) per la vista strategica.
> Lo stile è in §32, §33 e §47-bis. Dove questa pagina diverge dal codice o dal Decision Log,
> **prevalgono codice e Decision Log**. Lo scenario delle tavole è dimostrativo
> ([`dati/schermate.json`](dati/schermate.json)).
>
> **Triage**: il confronto zona per zona col repository sta nel referto
> [`hud-tavole-a-f-spec-panel-2026-10-09.md`](../../../../roadmap/plans/hud-tavole-a-f-spec-panel-2026-10-09.md).
> Il referto riporta ciò che il mockup chiede, con quale classe, chi possiede ogni zona e che cosa **non** si fa.
> [`dati/zone.json`](dati/zone.json) è la fotografia del 2026-10-04, e le sue posizioni e classi sono corrette lì.
>
> ⚠️ **Questa pagina è più vecchia di D-456, e le posizioni le contraddicono in tre punti.** «Unità selezionata in
> basso a sinistra», «Ghost Timeline sopra la dock» e «Conferma in basso a destra» non valgono più. Vale la griglia
> di [`guida-screen-hud-umg.md`](../../../../technical/runbooks/guida-screen-hud-umg.md) §3, ed è quella che le
> tavole nuove disegnano. Anche «il pulsante invoca `LockIn`» (Z7) è superato:
> [D-458](../../../../decisions/RT_PDR_00_Decision_Log.md) dice che `Conferma` è `Invio` e `Annulla` è il Back.
>
> ⚠️ **E nel corpo sono superate altre due affermazioni, più una della tavola C.**
> - «Editing/Ready | Locked» del Roster (§1 e Z2) è `FUTURE` da
>   [D-478](../../../../decisions/RT_PDR_00_Decision_Log.md), e il bordo tratteggiato passa al chip `REAZ.`.
> - «Su Drive c'è il PDF» (§2): nessun PDF entra, perché entrambi precedono le correzioni.
> - Fuori dal corpo, nella tavola C: il costo del conflitto su G7 vale solo per arrivi nello stesso microstep
>   (referto §6 D7).
>
> 🎬 **Le tavole mostrano la vista strategica** ([D-488](../../../../decisions/RT_PDR_00_Decision_Log.md)).
> Con `Tab` la parte centrale passerà dalla 3D tattica all'isometrica semplificata, e l'HUD resta identico.
> ⏳ Non cablato: oggi `Tab` cicla la selezione.
> - **Le zone screen-space** valgono in entrambe le viste.
> - **La scena isometrica** è il bersaglio della vista strategica, che oggi esiste come stato e non come vista.
> - **Nella vista tattica 3D**, degli overlay nel mondo (Z13, Z15, Z16) valgono significato e forme.
>
> ⚠️ Della nota a piè di pagina delle tavole annotate e del sorgente, «Scena illustrativa … non la resa», sono
> superate «illustrativa» e «non la resa». Che l'isometrica sia il livello strategico resta giusto.
>
> **Griglia**: 1920×1080, margine di sicurezza 24 px, **centro libero** (§3.1, §47.2). Colori, font e misure:
> [`../skill-bar-2026-10/dati/tokens.json`](../skill-bar-2026-10/dati/tokens.json).
>
> **Regola sovraordinata**: il colore non porta mai il significato da solo (§47-bis.1). Ogni stato ha anche una
> forma, un pattern o un testo.

## 1. Idea: tre strati di persistenza (§31)

| Strato | Zone | Quando compare |
|---|---|---|
| **Sempre visibile** | Header (Z1), Roster (Z2), Obiettivo (Z3), Unità selezionata (Z4), Dock (Z5) | in ogni schermata |
| **Contestuale** | Ghost Timeline (Z6), Conferma (Z7), Avvisi (Z8), Intenti alleati (Z9), Registro (Z10), Conoscenza parziale (Z15) | solo in Pianificazione o solo in Risoluzione |
| **Temporaneo** | Reazione rapida (Z14), Danno fluttuante (Z13), Evento corrente (Z11) | per pochi secondi, poi spariscono |

L'interfaccia **cambia con la fase**, non con l'interazione:

| Zona | Pianificazione (A B C F) | Risoluzione (D E) |
|---|---|---|
| Dock | piena, editabile | **collassata** in riepilogo del piano (3 slot) |
| Ghost Timeline / Conferma / Avvisi | visibili | nascosti |
| Registro | assente o chiuso | **promosso**, con WHY? |
| Header | timer di pianificazione | macrofase corrente |
| Roster | Editing/Ready | Locked |

Regola: in Risoluzione si nasconde tutto ciò che permette di *modificare*; si promuove tutto ciò che *spiega* (§13.2).

## 2. Schermate e che cosa verificano

| File | Che cosa dimostra | Zone nuove rispetto alla precedente |
|---|---|---|
| `A-pianificazione-pulita` | Stato base | Z1–Z7, Z9 chiuso, Z16 |
| `B-selezione-e-avviso` | Un'azione con rischio | Z8 |
| `C-intenti-di-squadra` | Coordinamento tra alleati | Z9 espanso |
| `D-risoluzione` | Il piano che si attua | Z10, Z11, Z12, Z13 |
| `E-reazione-rapida` | Interruzione con timeout | Z14 |
| `F-conoscenza-parziale` | Informazione incompleta | Z15 |

Ogni PNG esiste in due versioni: pulita e `-annotato` (etichette CURRENT / PARZIALE / PROPOSTA / FUTURE per zona). Su Drive c'è il PDF con le versioni annotate.

## 3. Zone

La tabella completa, con widget, view, campi presenti e mancanti, è in `dati/zone.json`. Qui solo le regole che il codice non dice.

### Z1 Header
- Esempio di §6.1: `TURNO 04 · PIANIFICAZIONE · 00:21`.
- Le quattro celle PREP·DASH·BLAST·MOVE: in Risoluzione la cella attiva ha **quadrato pieno + bordo + nome**; le altre cerchio piccolo e testo spento.
- `PlanningSecondsRemaining == 0` significa «scaduta adesso», non «assente» (convenzione della view).

### Z2 Roster
- Compatto: **mai** tre schede grandi (§6.2).
- Con sessione non presidiata, due liste distinte (#2744). In sessione presidiata la seconda è vuota **per costruzione**.
- Editing = bordo tratteggiato ambra; Ready = bordo pieno ciano; Locked = grigio. Il testo c'è sempre.

### Z6 Ghost Timeline
- Quattro celle fisse, mai una coda: non deve poter sembrare «Attack → Move → Attack → Dash».
- Cella vuota = `—` in grigio. Cella con avviso = bordo tratteggiato ambra.

### Z7 Conferma
- Il pulsante invoca `LockIn`. Nel flusso reale c'è un countdown annullabile fra Ready e commit: il mockup mostra **solo lo stato di riposo**.
- Non mostrare `TEAM READY 1/2` finché non è reale (§6.8).

### Z8 Avvisi
- Tre livelli: Info, Warning, Critical (§15). Il mockup usa solo Warning.
- Un avviso dice: che cosa, perché, cosa costa. Triangolo + testo; l'ambra da sola non basta.

### Z10 Registro e WHY?
- Il registro è guidato dal TurnLog autorevole. Una riga = fase (striscia colore + testo) + attore + azione.
- WHY? mostra reason code già prodotti; **non ricalcola**. Le voci del mockup (Base, Bagnato + Elettricità, Copertura, Finale) sono l'esempio di §14.1.

### Z13 Danno fluttuante
- Vicino alla barra PV, mai al centro dell'unità.
- Numero = `Amount` della view (nominale). Nessuna deduzione dal delta della barra.

### Z14 Reazione rapida (§11.1)
- Compatto, countdown subito visibile, **due opzioni**, non copre la scena, timeout = scelta sicura, nessuna anticipazione di futuri trigger, nessun `Opportunity 1/3`.
- Le etichette vengono da `Options`; la scelta sicura da `SafeResponse`.
- Il mockup fissa FIRE / HOLD perché è il caso Overwatch.

### Z15 Conoscenza parziale
- Quattro simboli, ognuno con forma e testo: `?` incerto (cerchio tratteggiato + area), `×` ultimo contatto (sbiadito + turno), `◎` rumore (onde), unità rilevata (rappresentazione normale).
- Il pannello contatti conta per categoria. **Mai** coordinate esatte per un contatto incerto.

## 4. Cosa le immagini mostrano ma NON si implementa ora

| Elemento | Perché |
|---|---|
| Intenti alleati (Z9) e chip Editing/Ready in coppia | FUTURE: servono più comandanti (DEC-HUD-2) |
| Controlli di riproduzione (Z12) | comandi da spettatore (DEC-HUD-1) |
| WHY? (Z10) | nessun reason code a schermo misurato |
| Superficie di conoscenza parziale (Z15) | decisione su `Last Contact` e sul rumore |
| Nomi e dati: «Nodo centrale», G7, turno 04, 2–1 | dimostrativi |
| Icone, ritratti, mappa | segnaposto |
