# Cataloghi di bilanciamento v0.1

I **numeri vigenti** della release v0.1, in Markdown versionato e diffabile: il bilanciamento si revisiona in PR,
non riaprendo un PDF. Le *decisioni* restano nel canone ([`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md)),
lo *stato di avanzamento* nella [roadmap](../roadmap/roadmap-checkpoint.md).

| File | Contenuto | Implementato da |
|---|---|---|
| [`RT_ActionCatalog_v0.1.md`](RT_ActionCatalog_v0.1.md) | ~35 azioni con ID stabile, macro-fase, priorità, range, costo, cooldown, fallback, interrompibilità | E4 (motore azioni), E5 (reazioni) |
| [`RT_TerrainCatalog_v0.1.md`](RT_TerrainCatalog_v0.1.md) | 8 terreni, stati ambientali, coperture e strutture | E8 (ambiente), E9 (strutture) |
| [`RT_EquipmentCatalog_v0.1.md`](RT_EquipmentCatalog_v0.1.md) | Varianti arma, gadget, moduli di reazione | E7 (equipaggiamento) |
| [`RT_HeroCatalog_v0.1.md`](RT_HeroCatalog_v0.1.md) | Flux, Riva, Bastion, Vektor: statistiche, abilità, varianti, loadout | E6 (roster) |
| [`RT_TestMatrix_v0.1.md`](RT_TestMatrix_v0.1.md) | Collisioni, combo, test manuali e automatici, comandi di debug | E12 (QA e release) |

**Fonte**: `docs/src/RefactorTactics — Catalogo e bilanciamento v0.1.pdf` e `docs/archive/pdr-v0.1/RT_PDR_12_Catalog_v0.1.pdf`,
adottati con [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md).

## Due avvertenze per chi legge

1. **Questi documenti descrivono il bersaglio, non l'esistente.** Ogni file dichiara in testa cosa è già in
   codice e cosa no. Al 2026-08-06 il gioco ha due archetipi (non quattro eroi), nessun effetto attivo sulle
   superfici e nessuna copertura direzionale.
2. **Le divergenze rispetto ai PDF sono elencate in fondo a ogni file**, con il motivo. La più importante:
   le macro-fasi restano quelle di Atlas Reactor (`Prep → Dash → Blast → Move`, **Move dopo il Blast**), mentre
   il catalogo metteva il movimento prima dell'attacco — i codici di fase del catalogo sopravvivono come
   attributo dell'azione ([ADR-0003 §3](../decisions/adr-0003-modello-azioni-v01.md)).

Dove un numero **manca nella fonte**, il file lo dichiara «non specificato» invece di inventarlo.
