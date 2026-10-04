# Skill bar RefactorTactics — sorgente di design (2026-10-04)

> **Statuto**: tutta la cartella è `PROPOSTA`, cioè **sorgente di design e non canone**. Il repository resta
> la fonte di verità: dove questi file divergono dal codice o dal
> [Decision Log](../../../../decisions/RT_PDR_00_Decision_Log.md), prevalgono quelli.
>
> **Triage e decisioni**:
> [`skill-bar-mockup-spec-panel-2026-10-04.md`](../../../../roadmap/plans/skill-bar-mockup-spec-panel-2026-10-04.md).

```
SPECIFICA-VISIVA.md       ← anatomia dello slot, ricette degli stati, composizione della dock
dati/tokens.json          ← colori, font e misure in px di design a 1920×1080
dati/comandi.json         ← i comandi di Aevik: tasto, fase, slot, regola di movimento (fotografia del 2026-10-03)
immagini/01..08.png       ← render del mockup, uno per stato significativo (2x)
sorgente-mockup/*.dc.html ← sorgente del canvas Design: solo riferimento dei valori, non HTML eseguibile né codice UE
```

⛔ **Niente di questa cartella va in `Content/`.** Le PNG sono riferimento e non texture, e le icone del mockup
sono segnaposto: le icone vere arrivano da `DA_IconCatalog` via `IconId`.

ℹ️ Il pacchetto arrivava con un `PROMPT.md`, cioè il work order che lo accompagnava. **Non è versionato**: un
work order esterno è una consegna effimera ([`AGENTS.md`](../../../../../AGENTS.md) §8), e il referto qui sopra è
l'unico posto in cui resta citabile, insieme a ciò che chiedeva e non è stato fatto.
