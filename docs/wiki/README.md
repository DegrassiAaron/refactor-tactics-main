# `docs/wiki/` — solo asset, nessuna pagina

> `CURRENT` · **Aggiornato**: 2026-08-10 · **Decisione**: [D-076](../decisions/RT_PDR_00_Decision_Log.md)

**Le pagine della Wiki non sono più qui.** La fonte è il clone pubblicato
`refactor-tactics-main.wiki`: una sola copia, che è anche quella che il giocatore legge. Il
repository ne aveva due e divergevano su 37 pagine su 43, in entrambe le direzioni, perché il deploy
sincronizzava soltanto i blocchi `RT_FEATURE_STATUS` e mai il corpo.

Per modificare una pagina si edita il clone. Per aggiornare i blocchi di stato:

```
python scripts/feature_registry.py deploy --wiki-root <clone> --write
```

## Perché questa cartella esiste ancora

Contiene materiale che **non è duplicato nel clone**, e che quindi la cancellazione avrebbe perso:

| Cosa | Stato |
|---|---|
| `RefactorTactics_Wiki_Infographics_v0.1/` (14 PNG) + `.zip` | Il clone pubblica la **v0.2**, con altri nomi e altri contenuti: verificato per hash, **zero** immagini in comune. Queste sono la v0.1, e non esistono altrove |
| `wiki-manifest-v0.5.json` · `v0.6` · `v0.7` | Tre manifest in parallelo, nessuno dichiarato vincente |

La loro pulizia è **lavoro aperto già registrato** in
[`wiki-consolidamento-2026-08-10.md`](../roadmap/plans/wiki-consolidamento-2026-08-10.md), §D — non
è parte di D-076, che riguarda la doppia sorgente delle **pagine**. Restano qui finché quella
decisione non è presa: cancellarli di iniziativa avrebbe risolto un problema altrui distruggendo
l'unica copia fuori dalla storia di git.

Le immagini delle fazioni sono invece sparite da qui perché erano **identiche** a quelle del clone —
duplicati veri, verificati per hash.
