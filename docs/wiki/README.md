# `docs/wiki/` — vuota: qui non c'è più niente

> `CURRENT` · **Aggiornato**: 2026-08-17 · **Decisione**: [D-076](../decisions/RT_PDR_00_Decision_Log.md)

**Le pagine della Wiki non sono qui**: la fonte è il clone pubblicato `refactor-tactics-main.wiki`, una
sola copia, che è anche quella che il giocatore legge. Per modificare una pagina si edita il clone; per
aggiornare i blocchi di stato:

```
python scripts/feature_registry.py deploy --wiki-root <clone> --write
```

**E dal 2026-08-17 non ci sono più nemmeno gli asset.** Questo README ha continuato per quattro giorni a
elencare come presenti tre categorie di file che il commit `273c76a6` aveva già portato via:

| Cosa dichiarava | Dov'è finito davvero |
|---|---|
| `RefactorTactics_Wiki_Infographics_v0.1/` — 14 PNG | [`../src/wiki/v0.1/`](../src/wiki/README.md), divise per conformità del roster |
| `RefactorTactics_Facing_Flows_v0.1/` — 7 PNG | [`../src/wiki/facing/`](../src/wiki/facing/README.md) |
| `wiki-manifest-v0.5.json` · `v0.6` · `v0.7` | 🔴 **cancellati**, non spostati: esistono solo nella storia git |

⚠️ **Il file `.zip` che questo README citava non è mai stato un file del repository**: `.gitignore` esclude
`docs/src/*.zip`. Vive nel checkout dell'autore e nient'altro lo vede.

La cartella resta perché [D-076](../decisions/RT_PDR_00_Decision_Log.md) e più documenti la citano per
nome, e un link che marcisce è il difetto che quella decisione esiste per impedire. La sua unica funzione
oggi è dire dove guardare: [`../src/wiki/README.md`](../src/wiki/README.md).
