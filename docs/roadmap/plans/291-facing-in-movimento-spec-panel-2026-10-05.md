# Spec panel — il facing durante il movimento e la chiusura del percorso (#291 · #2167 · #172)

**Data**: 2026-10-05 · **Misurato su**: `main` `b0b827a5e` · **Modalità**: discussion · critique · **Focus**: requirements · architecture · testing
**Panel**: Cockburn (attore/goal) · Wiegers (requisiti) · Fowler (confini) · Adzic (esempi) · Crispin (testabilità) · Nygard (failure mode)

> Referto della sessione chiesta dall'autore il 2026-10-05: *«gestiamo il facing di un personaggio durante il
> movimento. deve cambiare verso e alla fine del movimento girare nel facing scelto dal giocatore. quando si
> pianifica, l'ultimo movimento è il facing. secondo click sul waypoint fa scegliere il verso e chiude il
> movimento.»*
>
> **Non è un owner.** Le regole restano di [D-367](../../decisions/RT_PDR_00_Decision_Log.md) e di
> [D-462](../../decisions/RT_PDR_00_Decision_Log.md), che questa sessione ha prodotto. L'implementazione è delle
> tre issue del §5. ⛔ Nessuna riga di codice è cambiata con questo referto.

---

## 0. Cosa c'era già

La richiesta è per gran parte **già decisa e non implementata**:
- [D-367](../../decisions/RT_PDR_00_Decision_Log.md), accettata il 2026-09-10 con owner [#291](https://github.com/DegrassiAaron/refactor-tactics-main/issues/291), sceglie il facing finale con un secondo click su uno dei sei triangoli dell'esagono finale;
- toglie il tasto `T`;
- ordina il Back: selettore, poi facing, poi waypoint;
- cancella la dichiarazione a ogni modifica del percorso.

Misurato sul codice di `b0b827a5e`:

| Fatto | Dove | Stato |
|---|---|---|
| Facing canonico a sei direzioni | `ARTUnit::Facing` (`Unit/RTUnit.h`), `FRTHexSimUnit::Facing` nello snapshot, nell'hash ([D-261]) | ✅ esiste |
| Facing dichiarato nel piano | `PlannedFacing` + `bDeclaresPlannedFacing` (`RTUnit.h`), applicato dal resolver con `TryApplyDeclaredFacing` entro il budget di pivot | ✅ esiste |
| Facing a metà movimento = ultimo passo compiuto | `URTFacingLibrary::FacingAtMicroStep`; lo leggono il cono dell'Overwatch e i boundary di reazione | ✅ esiste (regola) |
| Facing finale senza dichiarazione | direzione dell'ultimo passo **compiuto** sulla rotta effettiva (`RTTurnManager_Movement.cpp`) | ✅ esiste |
| Budget di pivot per eroe (Move/Dash) | Aevik 2/2, Muiren 2/3, Branth 1/0, Ivrin 3/3; da fermo 3 (`RTHeroCatalogLibrary.cpp`, `RTFacingLibrary.h`) | ✅ esiste |
| Ingresso del facing | solo il tasto `T` → `CycleDeclaredFacing` (`RTPlayerController.cpp`) | ⛔ D-367 non cablata |
| Click ripetuto sull'ultimo waypoint | aggiunge un duplicato a costo zero (`RTHexSimLibrary.cpp`, `BuildCompositeHexPath`) | ⛔ da cambiare |
| Concetto di «movimento chiuso» | non esiste: nessun `MoveConfirmed` o simile | ⛔ nuovo |
| La mesh ruota durante il movimento | **no**: `bFaceMovementDirection` è `false` sui quattro `BP_Unit`, e il playback interpola solo la posizione | ⛔ [#2167](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2167) |
| La mesh ruota a fine movimento | **di scatto**: `FinishPlayback` chiama `SetActorRotation` sul facing finale | ⛔ da animare |
| Test sullo yaw durante o dopo il playback | nessuno; `PIE-FACING` è ⏳ su *«la mesh non si orienta durante il movimento»* | ⛔ |
| Stile di movimento nel controller | `PlannedPath.Num() > 1 ? Budget : None`: un Dash è valutato sul budget Move | 🔴 difetto, già nello scope di #291 (D-367) |

⚠️ Le righe sono del rapporto di ricognizione sul codice; D-367 è stata riletta per intero.

[D-261]: ../../decisions/RT_PDR_00_Decision_Log.md

## 1. Il panel, in sintesi

- **Cockburn** — L'attore decide dove arriva e dove guarda in un gesto solo. *«Chiude il movimento»* aggiunge uno stato che oggi non esiste, e contraddice D-367, per cui un click su un'altra cella *«continua o modifica il percorso»*. → `CONTRACT CONFLICT`, deciso al §2 punto 1.
- **Wiegers** — *«Deve cambiare verso»* diventa verificabile solo così: la mesh coincide con `FacingAtMicroStep` a ogni confine di passo. Altrimenti il cono dell'Overwatch scatta in una direzione diversa da quella che il giocatore vede. Per il giro finale servono un momento (dopo l'arrivo o mentre arriva) e una durata.
- **Fowler** — La simulazione decide, la presentazione consuma. Oggi la presentazione legge `Unit->Facing` solo alla fine. Per girare passo per passo deve chiedere la regola sulla rotta che anima, non derivare lo yaw dalla curva interpolata ([D-243]: gli spicchi sono presentazione).
- **Adzic** — Gli esempi del §3. Quello sul click dopo il facing non si poteva scrivere senza una decisione.
- **Crispin** — Mancano tre controlli:
  - headless: yaw uguale alla regola a ogni confine e a fine playback;
  - headless: il secondo click legale scrive il piano, quello illegale no, e nessuno dei due duplica un waypoint;
  - PIE: il pivot finale si legge come un pivot.
- **Nygard** — Quattro rotture silenziose:
  - lo stile Dash stimato come Move (il selettore mostrerebbe legali triangoli che il resolver rifiuta);
  - Branth in Dash, con budget 0;
  - il facing dichiarato contro il bersaglio di un Blast ([D-020]);
  - il click ripetuto che oggi duplica un waypoint.

[D-243]: ../../decisions/RT_PDR_00_Decision_Log.md
[D-020]: ../../decisions/RT_PDR_00_Decision_Log.md
[D-461]: ../../decisions/RT_PDR_00_Decision_Log.md

## 2. Le decisioni d'autore — [D-462](../../decisions/RT_PDR_00_Decision_Log.md)

1. **Il facing chiude il movimento.** Dopo il secondo click che sceglie il verso, un click su un'altra cella **non fa nulla**. Per riaprire il percorso si fa Back, che toglie prima il verso. Corregge D-367 su questo punto, dove un click su un'altra cella continuava il percorso e cancellava il verso.
2. **In marcia il personaggio si gira a ogni passo**, verso la direzione del passo, con una rotazione breve all'inizio di ciascuno. A ogni confine di cella la mesh coincide con `FacingAtMicroStep`.
3. **A fine marcia arriva guardando l'ultimo passo, poi ruota sul posto** verso il verso scelto, con un pivot animato e non uno scatto. È il pivot finale della regola, che si applica *dopo* e *non retroattivamente*.
4. **Da fermo: un click sulla propria cella**, con l'unità selezionata, apre i sei triangoli; un secondo click sceglie. D-367 dà già ai triangoli la priorità sulla mesh dell'unità.

## 3. Esempi, cioè i criteri d'accettazione

```
Dato   Aevik in (0,0) rivolta a E, budget di pivot Move 2
Quando pianifica (1,0) poi (2,-1) e con un secondo click sull'esagono (2,-1) sceglie il triangolo SE
Allora il piano dichiara SE e il movimento è chiuso
       in playback: guarda E sul primo passo, NE sul secondo, arriva, poi ruota sul posto verso SE
       nel TurnLog: `DeclaredInPlanning`

Dato   lo stesso percorso
Quando il secondo click cade sul triangolo W, che sta oltre il budget di 2
Allora il triangolo è grigio e non risponde: il piano non cambia e il movimento resta aperto

Dato   il movimento chiuso con SE
Quando il giocatore clicca un'altra cella
Allora non succede nulla: nessun waypoint, il verso resta SE                       ← D-462 (1)

Dato   il movimento chiuso con SE
Quando il giocatore preme il tasto destro (o Annulla)
Allora il verso si cancella, torna quello derivato (NE) e il movimento è di nuovo aperto
       un secondo Back toglie l'ultimo waypoint                                   ← D-367, D-461

Dato   Aevik ferma e selezionata, senza waypoint
Quando clicca la propria cella e poi il triangolo W
Allora il piano dichiara W (da fermo il budget è 3: le sei direzioni)            ← D-462 (4)

Dato   un percorso senza facing scelto
Allora in playback guarda ogni passo, e a fine marcia resta sull'ultimo: nessun pivot
```

## 4. Restano aperti, da decidere durante #291

- **Il facing dichiarato contro il bersaglio di un'azione** ([D-020]: un'azione con bersaglio orienta l'unità prima di risolvere). Va scritto quale vince e in quale fase, prima che l'overlay prometta un verso che il Blast cambierà.
- **L'ordine del Back con un'azione armata** ([D-461]): se l'azione è armata *dopo* la chiusura del movimento, l'ultimo gesto è l'azione e non il verso. La regola *«il Back disfa l'ultimo gesto»* basta, ma va resa in `ResolveBack`, che oggi non ha un passo «facing dichiarato».
- **Branth in Dash, con budget 0**: il selettore ha legale il solo verso derivato, e deve leggersi come *«non può girarsi»*, non come un guasto.
- 🔴 **`spec-pointer-interaction.md` non cita D-367.** Il §5.5 non ha il passo «facing dichiarato» nell'ordine del Back: va aggiunto insieme all'implementazione, non prima.

## 5. Le fette, e a chi appartengono

| Fetta | Owner | Contenuto |
|---|---|---|
| **Input** | [#291](https://github.com/DegrassiAaron/refactor-tactics-main/issues/291) | secondo click sui triangoli (anche sulla propria cella da fermo); movimento chiuso e Back che lo riapre; nessun waypoint duplicato; via il tasto `T`; correzione dello stile Dash nei due siti del controller; passo «facing dichiarato» in `ResolveBack` e §5.5 |
| **Presentazione del passo e del pivot** | [#2167](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2167) (con [#1793](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1793)) | la mesh gira a ogni passo leggendo `FacingAtMicroStep` sulla rotta animata; a fine marcia pivot animato al posto di `SetActorRotation` di scatto; `PIE-FACING` rieseguita |
| **Overlay** | [#172](https://github.com/DegrassiAaron/refactor-tactics-main/issues/172) | i sei triangoli con legali, illegali e hover di D-367; chevron sul verso scelto; il segno di «movimento chiuso» |

## 6. I test che le fette devono portare

- `#291`:
  - il secondo click legale scrive `PlannedFacing` e chiude il movimento;
  - quello illegale non scrive niente;
  - nessun waypoint duplicato;
  - un click su un'altra cella a movimento chiuso non cambia il piano;
  - il Back riapre;
  - il click sulla propria cella da fermo apre il selettore;
  - un Dash è valutato sul budget Dash (mutazione: lo stile stimato dal numero di celle).
- `#2167`:
  - lo yaw dell'attore a ogni confine di passo è uguale a `FacingAtMicroStep`, e a fine playback a `Unit->Facing`;
  - il pivot finale parte dopo l'arrivo;
  - un'unità ferma non ruota durante il playback altrui.
- PIE: `PIE-FACING` sullo stesso percorso dell'esempio del §3.
