# HUD contestuale RefactorTactics — sorgente di design (2026-10-04, tavole del 2026-10-09)

> **Statuto**: tutta la cartella è `PROPOSTA`, cioè **sorgente di design e non canone**. Il repository resta
> la fonte di verità: dove questi file divergono dal codice o dal
> [Decision Log](../../../../decisions/RT_PDR_00_Decision_Log.md), prevalgono questi ultimi.
>
> **Triage e decisioni**:
> [`hud-tavole-a-f-spec-panel-2026-10-09.md`](../../../../roadmap/plans/hud-tavole-a-f-spec-panel-2026-10-09.md).

```
SPECIFICA-ZONE.md          ← regole per zona, persistenza per fase, cosa NON si implementa
dati/zone.json             ← Z1…Z16: widget, vista, campi presenti e mancanti (fotografia del 2026-10-04)
dati/schermate.json        ← lo scenario dimostrativo delle sei tavole
immagini/*.png             ← le sei tavole A–F a 1920×1080, pulite e annotate, più la verifica in scala di grigi
immagini/*.pdf             ← lo stesso, come consegnato su Drive
```

La skill bar (Z5) ha il proprio pacchetto in [`../skill-bar-2026-10/`](../skill-bar-2026-10/), che possiede anche
colori, font e misure ([`dati/tokens.json`](../skill-bar-2026-10/dati/tokens.json)).

🎬 **La scena delle tavole è illustrativa.** La vista isometrica è il livello **strategico**, e non è
implementata; a livello tattico la scena è la 3D attuale.
- Delle zone screen-space valgono **posizione, contenuto e stile**.
- Degli overlay nel mondo — barre PV, anelli, coni, percorsi, marcatori di contatto — valgono **significato e
  forme**, non la resa.

⛔ **Niente di questa cartella va in `Content/`.** Le PNG sono riferimento, non texture. Icone, ritratti e
mappa sono segnaposto: le icone vere arrivano da `DA_IconCatalog` via `IconId`.

ℹ️ Il pacchetto arrivava con un `PROMPT.md`, il work order che lo accompagnava. **Non è versionato**: un work
order esterno è una consegna effimera ([`AGENTS.md`](../../../../../AGENTS.md) §8). Il referto qui sopra è
l'unico posto in cui resta citabile, insieme a ciò che chiedeva e non è stato fatto.
