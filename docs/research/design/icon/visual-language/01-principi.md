# Visual Language — Principi

> **Statuto**: sorgente di design sotto `docs/research/`, **non canone** *(era `docs/src/` fino al
> 2026-08-19, [#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165))*. Il canone
> dell'iconografia è
> [D-031](../../../../decisions/RT_PDR_00_Decision_Log.md) e l'enum `ERTIconCategory` in
> `Source/RefactorTactics/UI/RTIconCatalogData.h`. Dove questi documenti divergono dal codice o dal Decision
> Log, **prevalgono codice e Decision Log**.
>
> **Ownership**: qui si definisce **come** un concetto viene rappresentato. **Dove e quando** un elemento
> appare è di [`progettazione-hud.md`](../../../../technical/systems/progettazione-hud.md). Nessuna regola vive in
> entrambi i posti: quando serve l'altra metà, si rimanda.

## 1. I due assi

La confusione più costosa di questo dominio è trattare **grammatica** e **catalogo** come la stessa cosa.
Non lo sono, e il repository lo dimostra con un validator.

| | Grammatica compositiva | Catalogo semantico |
|---|---|---|
| Che cos'è | il vocabolario con cui si **disegna** un glifo | l'indice con cui un widget **risolve** un'icona a runtime |
| Unità | primitive: `Target`, `Geometry`, `Effect`, `Modifier` | chiavi: `UI.Icon.<Categoria>.<Nome>` |
| Chi la usa | chi produce l'asset | `URTIconLibrary`, i widget di E11 |
| Dove è definita | [`03-forme-e-primitive.md`](03-forme-e-primitive.md), [`04-regole-di-composizione.md`](04-regole-di-composizione.md) | [`08-catalogo-v0.1.md`](08-catalogo-v0.1.md) |
| Validata da | review visiva (silhouette, grayscale, CVD) | validator: categoria dichiarata == segmento nell'`IconId` |

Nessun widget chiederà mai `UI.Icon.Geometry.Line`. Chiederà `UI.Icon.Action.Dash`, che **è disegnata** con
una `Line` più una grammatica di movimento. Le primitive sono ingredienti, non voci di menu.

La conseguenza pratica: **una primitiva non ha un `IconId`**. Se una primitiva deve essere mostrata da sola —
perché compare in un tooltip, in una legenda della wiki o nel combat log — allora non è più una primitiva:
è una voce di catalogo e prende una delle dodici categorie canoniche.

## 2. Principi LOCKED

Ereditati da `CLAUDE_DESIGN_01` e dal MASTER del pack, riordinati e ridotti a ciò che non è già detto altrove.

1. Le icone sono **primitive semantiche componibili**, non un'illustrazione per skill.
2. Se una distinzione competitiva sparisce in grayscale, il design è da rifare. **Non si corregge col colore.**
3. Il colore è il **secondo** canale. Il primo è silhouette e pattern.
4. `Ally` ed `Enemy` differiscono per **forma**, non solo per tinta.
5. Evitare rosso/verde come coppia primaria `Ally`/`Enemy`.
6. `Electric` e `Reaction` non condividono il fulmine: il fulmine appartiene a `Electric`.
7. `Line` e `Move` sono distinti: `Line` = origine + segmento + punta; `Move` = percorso **con nodi**.
8. `Cover`, `Guard`, `Brace` e `Shield` sono **quattro** concetti diversi, e tutti e quattro esistono nel
   gioco — vedi [`08-catalogo-v0.1.md`](08-catalogo-v0.1.md) §2.
9. `Dash` e `Sprint` leggono come due intenzioni diverse, non come la stessa icona con un moltiplicatore.
10. Buff e debuff non dipendono da verde/rosso: usare `↑`, `↓`, `⊘` o forme equivalenti.
11. Nessuna icona deve richiedere glow per essere riconoscibile.
12. Leggibilità minima: **20–24 px a 1920×1080**.
13. Non creare un Gameplay Tag solo per soddisfare un'icona. Il flusso è l'inverso: il gioco produce un tag,
    l'icona lo rappresenta.
14. Non hardcodare texture o colori nei widget: si usa l'`IconId`, mai un percorso di asset ([D-031](../../../../decisions/RT_PDR_00_Decision_Log.md)).

## 3. Griglia

| Grandezza | Valore |
|---|---|
| Canvas master | `24×24 px` |
| Visual bounds | `20×20 px` |
| Core mass | ~`18×18 px` |
| Safe margin | ~`2 px` |
| Centro logico | `12,12` |
| Guide | `4 / 8 / 12 / 16 / 20` |

**Stroke**: ~2 px a 24 px; 2 px ottici a 20 px; variante semplificata dedicata a 16 px; 2.5–3 px ottici a
32 px. Niente hairline da 1 px per parti che portano significato.

**Complessità**: massimo 3 componenti dominanti a 24 px, 2–3 a 20 px, **2** a 16 px. Niente micro-texture né
ornamentazione interna.

**Linguaggio delle forme**:

- `Ally` / support / movimento — leggermente rounded;
- `Enemy` / attacco / warning — angular, notched;
- cella / obiettivo / geometria — neutral-geometric;
- reaction — grammatica di cerchio e anello spezzato;
- interazione con la mappa — grammatica strutturale, di bordo;
- status — emblema compatto, racchiuso.

## 4. Che cosa questo documento non decide

Non decide **quando** un'icona appare, in quale slot, con quale stato di disponibilità, né se un elemento è
persistente o contestuale: è [`progettazione-hud.md`](../../../../technical/systems/progettazione-hud.md) §7, §31, §49.

Non decide che cosa il giocatore ha il diritto di vedere. La privacy è architetturale
([D-021](../../../../decisions/RT_PDR_00_Decision_Log.md)): un'icona non può rappresentare informazione che il
Team Knowledge non possiede, e nasconderla nel widget non è sicurezza.
