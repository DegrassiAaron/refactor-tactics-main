# HUD contestuale RefactorTactics — sorgente di design (2026-10-04, tavole del 2026-10-09)

> **Statuto**: tutta la cartella è `PROPOSTA`, cioè **sorgente di design e non canone**. Il repository resta
> la fonte di verità: dove questi file divergono dal codice o dal
> [Decision Log](../../../../decisions/RT_PDR_00_Decision_Log.md), prevalgono questi ultimi.
>
> **Triage e decisioni**:
> [`hud-tavole-a-f-spec-panel-2026-10-09.md`](../../../../roadmap/plans/hud-tavole-a-f-spec-panel-2026-10-09.md).

```
SPECIFICA-ZONE.md            ← regole per zona, persistenza per fase, cosa NON si implementa
dati/zone.json               ← Z1…Z16: widget, vista, campi presenti e mancanti (fotografia del 2026-10-04)
dati/schermate.json          ← lo scenario dimostrativo delle sei tavole
immagini/*.png               ← le sei tavole A–F a 1920×1080, pulite e annotate, più G in scala di grigi
                               (export del 2026-10-09; ricompresse senza perdita in RGB, pixel verificati uguali)
sorgente-mockup/*.dc.html    ← sorgente del canvas Design: riferimento dei valori, non HTML eseguibile né codice UE
```

La skill bar (Z5) ha il proprio pacchetto in [`../skill-bar-2026-10/`](../skill-bar-2026-10/), che possiede anche
colori, font e misure ([`dati/tokens.json`](../skill-bar-2026-10/dati/tokens.json)).

🎬 **Le tavole mostrano la vista strategica** ([D-488](../../../../decisions/RT_PDR_00_Decision_Log.md)).
- **Come si apre.** Con `Tab` la parte centrale passa dalla 3D tattica a questa isometrica semplificata.
- **Cosa resta uguale.** L'HUD intorno è identico nelle due viste.
- **Cosa vale, e per chi.**
  - Delle zone screen-space valgono **posizione, contenuto e stile**, in entrambe le viste.
  - La scena isometrica è il bersaglio della vista strategica. Non è implementata: oggi esiste lo stato, non la vista.
  - Nella vista tattica 3D, degli overlay nel mondo valgono significato e forme.

⚠️ **La nota a piè di pagina delle tavole annotate dice «Scena illustrativa … non la resa», ed è superata.** È
stata scritta prima che l'autore precisasse la vista strategica, e si è scelto di non rifare le tavole: vale
questo paragrafo.

⛔ **Niente di questa cartella va in `Content/`.** Le PNG sono riferimento, non texture. Icone, ritratti e
mappa sono segnaposto: le icone vere arrivano da `DA_IconCatalog` via `IconId`.

ℹ️ Il pacchetto arrivava con un `PROMPT.md`, il work order che lo accompagnava. **Non è versionato**: un work
order esterno è una consegna effimera ([`AGENTS.md`](../../../../../AGENTS.md) §8). Il referto qui sopra è
l'unico posto in cui resta citabile, insieme a ciò che chiedeva e non è stato fatto.
