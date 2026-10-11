# I piani rispetto a quello attivo: dal brainstorming del 2026-10-11 a tre issue della v0.1

> `SNAPSHOT` · fotografia del 2026-10-11: vale finché è l'ultima misura del suo oggetto.
>
> - **Data**: 2026-10-11.
> - **Misurato su** `origin/main` = `83224c205`.
> - **Oggetto**: come si vedono i piani di una mappa multilivello rispetto al piano attivo del giocatore, e la separazione dei piani in vista strategica.
> - **Issue**: [#3678](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3678) (CAM-06b, scenario e separazione) · [#3679](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3679) (CAM-06c, presentazione dei piani) · [#3680](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3680) (HUD-PIANO, indicatore nel dock). Scorporate da [#1775](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1775) (CAM-06).
> - **Decisione registrata**: [D-501](../../decisions/RT_PDR_00_Decision_Log.md).
>
> Convenzione: niente totali che cambiano da soli. Dove un numero serve, sta accanto al comando che lo produce.

---

## 1. Il verdetto in una riga

Il piano attivo del giocatore **esiste ma non si vede**: decide su quale cella risolvono hover e click (D-255), e a schermo non cambia niente. L'autore ha scelto di **mostrare tutti i piani, trattati rispetto a quello attivo**, invece degli indicatori «c'è qualcosa più su» che #1775 prevedeva.

## 2. Che cosa esiste

Misurato su `83224c205`, cioè **prima** di #3678: due righe sono cambiate con essa, ed è annotato accanto (§7).

| Cosa | Stato | Dove |
|---|---|---|
| Piano attivo del giocatore | ✅ esiste. `PageUp`/`PageDown` lo cambiano, nient'altro lo cambia | `ARTPlayerController::ActiveLayer`, `StepActiveLayer`, `SetActiveLayer` |
| Segno a schermo del piano attivo | ❌ nessuno. Al cambio parte solo `UE_LOG` (`Piano attivo: N`) | `RTPlayerController.cpp`, `SetActiveLayer` |
| Modi di vista dei piani | 🟡 esistono, ma **solo per l'authoring**: `AllLayers`, `ActiveOnly` e `Focus` (gli altri piani a contorno, dall'editor mode) | `ARTHexMapActor::LayerView`, `ARTHexMapActor::ActiveLayer` |
| Bordo delle celle | ✅ esiste: è la griglia in partita (#1758), con il colore scuro del «segno inciso» (D-183) | `ARTHexMapActor::CellBorders` |
| Colore della cella | ✅ per istanza: `M_HexCell` legge tre `PerInstanceCustomData`, che porta anche il velo | `ARTHexMapActor::CellMaterial`, `RebuildInstances` |
| Separazione dei piani in strategica | ✅ `StrategicLayerSeparation = 3`, scelta nella seduta `U78`; in console `rt.Map.StrategicLayerSeparation`. 🔄 Con #3678 vale **2** (seduta `U81`, D-501 (7)) | `ARTHexMapActor` (#3632) |
| Mappa a tre piani | ❌ nessuna fixture ne ha più di due: `TestArena` ha una piattaforma di quattro celle sul piano 1. 🔄 Con #3678 c'è `LayerStack`, e lo scenario `Visual.Map.LayerStack` | `RTMatchSetupLibrary.cpp`, `GFixtures` |

⚠️ **Due campi con lo stesso nome.** `ARTHexMapActor::ActiveLayer` è stato di authoring, e lo scrive l'editor mode; il piano attivo del giocatore è `ARTPlayerController::ActiveLayer`. La presentazione legge il secondo. Due scrittori sullo stesso campo sono il difetto che D-255 e #1775 descrivono.

## 3. Le decisioni d'autore di questa sessione

Prese con domande a scelta, nell'ordine in cui sono state poste.

| Domanda | Scelta | Scartate |
|---|---|---|
| Cosa non funziona | non si capisce il piano attivo; i piani sopra coprono; i piani sotto distraggono | — |
| Piani **sopra** l'attivo | **traslucidi** | solo contorno, come `Focus`; nascosti; invariati |
| Piani **sotto** l'attivo | **desaturati e scuriti**, di più con la distanza. L'autore ha chiesto di decidere pensando ai **terreni con texture** che arriveranno, non agli esagoni grigi di oggi | solo scuriti; velati di nebbia; invariati |
| Segno del piano attivo | **indicatore nell'HUD** e **bordo delle celle**, con i **tasti** per cambiare piano accanto all'indicatore | il solo trattamento degli altri piani |
| Unità su un altro piano | **sempre piene** | col trattamento del loro piano: un nemico su un altro piano si leggerebbe meno |
| Viste | **tattica e strategica** | solo strategica; solo tattica |
| Come si giudica | **uno scenario nuovo a tre piani** | estendere `Visual.Map.MultiLevel`, che altre voci PIE usano |
| Posizione dell'indicatore | **nel dock**, cioè la fascia bassa | fascia destra sopra il registro; header; bordo della scena |
| Mappa a un piano | **indicatore nascosto** | sempre visibile, con i tasti disattivati |
| Ordine | **prima la prova ×2 e lo scenario**; l'indicatore dopo #3661, che decide le fasce | prima i piani; prima #3661 |
| Milestone | **tutte e tre in v0.1** | solo la prova ×2 in v0.1 |

E, prima delle domande: in strategica il piano 1 sembra **troppo alto** a ×3, quindi **si prova ×2**.

## 4. Vincoli che il lavoro eredita

- **Solo presentazione.** Nessuna `FRTCellId` cambia, e lo stato canonico nemmeno. Il picking resta quello di D-255, cioè un click risolve sul piano attivo anche attraverso un piano traslucido.
- **Il velo viene prima** (D-242, e il vincolo scritto in #1775). Una cella mai vista resta velata anche su un piano traslucido. «Unità sempre piene» vale per le unità che la squadra vede.
- **Texture in arrivo.** L'attenuazione dei piani sotto agisce sull'uscita del materiale, non sostituisce il colore. Un trattamento tarato sul grigio di oggi andrebbe rifatto.
- **Le fasce dell'HUD si misurano sugli inquilini** (#3661). L'indicatore nel dock è un inquilino nuovo della fascia bassa: o ci sta, o la fascia cresce e il dato di #3661 si aggiorna.

## 5. Le tre issue

| Issue | Che cosa | Dipende da |
|---|---|---|
| [#3678](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3678) CAM-06b | la fixture `LayerStack` e lo scenario `Visual.Map.LayerStack`; la seduta che confronta ×2 e ×3; il nuovo default, se confermato | — |
| [#3679](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3679) CAM-06c | i piani sopra traslucidi, quelli sotto attenuati, il bordo dell'attivo marcato, le unità piene; leve in console per le intensità | #3678 |
| [#3680](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3680) HUD-PIANO | «Piano N di M» nel dock, con ▲/▼ che passano da `StepActiveLayer`; nascosto con un piano solo | #3661, #3678 |

## 6. Punti aperti, da chiudere nella PR che li incontra

- **La tecnica della traslucenza** (#3679): custom data per istanza con un'opacità a dithering, oppure un materiale traslucido per piano. Contano l'ordinamento, il velo e le ombre.
- **Le intensità** (#3679): opacità sopra, desaturazione e scurimento sotto, enfasi del bordo. Si scelgono in PIE, come la separazione.
- **L'anteprima del percorso e le sagome** (`PlanGhosts`) su un altro piano (#3679): la proposta è di trattarle come le unità, cioè piene.
- **La numerazione dell'indicatore** (#3680): la proposta è l'ordinale fra i piani che la mappa ha davvero, da 1.

## 7. Esito della prova a ×2

Nella seduta `U81` l'autore ha confrontato 3 e 2 in isometrica su `Visual.Map.LayerStack`, a 0° e a 60°: a 3 la cima esce dal quadro, a 2 i tre piani ci stanno. **Ha scelto 2** ([D-501](../../decisions/RT_PDR_00_Decision_Log.md) (7)). A 60° il cilindro di Aevik sparisce sotto il mezzanino e resta la sua etichetta: è il caso che #3679 deve rendere leggibile.

## 8. Fuori da questo lavoro

- La piattaforma che a 120° e a 180° non si vede (#3674): è un difetto di render, non di presentazione dei piani.
- La scena fra le fasce dell'HUD (#3661).
