# RefactorTactics — Personaggi

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-CHAR-PRESENTATION -->

> 🚧 **Parzialmente giocabile.** Il codice esiste ma la feature non è completa: i gate qui sotto dicono quanto manca. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-CHAR-PRESENTATION` · Release: `v0.1` · Roadmap: `E21.1, E21.2, E21.3 · M8`  
> Stato: **IMPLEMENTING** · Gate: `1/7`  
> Scenario: `—`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-CHAR-PRESENTATION -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-CHAR-V01-ROSTER -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-CHAR-V01-ROSTER` · Release: `v0.1` · Roadmap: `E6.1, E6.2, E6.3, E6.4, E6.5, E6.6, E6.7`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Combat.BasicAttack`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-CHAR-V01-ROSTER -->

La Wiki distingue il roster operativo dalle basi asset Paragon. Dal 2026-08-08 ogni eroe dichiara **quale**
slot Paragon usa come base visuale ([D-037](../decisions/RT_PDR_00_Decision_Log.md)); la tabella completa sta in
[`paragon.md`](paragon.md#mapping-visuale-del-roster).

## Roster v0.1 — RefactorTactics

I nomi canonici del roster sono fissati da [D-120](../decisions/RT_PDR_00_Decision_Log.md) (2026-08-12) e dal
2026-08-13 **coincidono** con lo slot Paragon che ciascun eroe usa come base visuale.

| Nome canonico | Scheda | Stable ID (legacy) | Base visuale |
|---|---|---|---|
| **Gadget** | [scheda](v0.1/flux.md) | `Hero.Flux` | `Paragon.Gadget` |
| **Phase** | [scheda](v0.1/riva.md) | `Hero.Riva` | `Paragon.Phase` |
| **Riktor** | [scheda](v0.1/bastion.md) | `Hero.Bastion` | `Paragon.Riktor` |
| **Wraith** | [scheda](v0.1/vektor.md) | `Hero.Vektor` | `Paragon.Wraith` |

> **I token della colonna «Stable ID» sono legacy implementation identifiers, non nomi del personaggio.**
> Sopravvivono dove esistono già — codice, asset, scenari, replay — e **non si rinominano**: una migrazione
> di ID richiede compatibilità, versioning e replay-safety propri, e ha un blocker misurato ancora aperto
> ([#716](https://github.com/DegrassiAaron/refactor-tactics-main/issues/716)). Anche i file delle schede
> conservano per ora il nome storico: cambiarlo è un `move` che rompe i link, non un rename di prosa.
>
> ✅ **Dal 2026-08-13 è anche quello che si legge a schermo.** Il nome canonico è dichiarato dal catalogo
> (`URTHeroData::DisplayName`), trasportato sull'unità da `ConfigureFromHeroData` e letto dalla HUD via
> `ARTUnit::DisplayLabel`, che ricade sull'ID stabile solo quando nessun eroe ha dichiarato un nome.
> Pinnato da `RefactorTactics.Heroes.CanonicalNamesReachTheLabel` sul percorso reale; il giudizio a schermo
> resta la voce `PIE-NAME` di [`../technical/test-manuali-pie.md`](../technical/test-manuali-pie.md).

## Roster v0.2 — asset base Paragon

Nome di lavoro e slot asset coincidono; il `RT Character ID` definitivo resta **TBD**.

- [Steel](v0.2/steel.md)
- [Aurora](v0.2/aurora.md)
- [Murdock](v0.2/murdock.md)
- [Kwang](v0.2/kwang.md)

## Tutti gli asset Paragon

➡️ **[Indice completo dei 38 personaggi/asset hero Paragon](paragon.md)**

Gli altri 34 slot sono conservati in `candidates/`: hanno Signature e dipendenze tecniche definite, ma non una release, un kit o statistiche competitive approvate.

## Regola di lettura

- `IMPLEMENTED` / v0.1: valori runtime/canonici dove presenti.
- `DATA_SPEC` / v0.2: design numerico pianificato, non runtime v0.1.
- `SIGNATURE_DEFINED` / Candidate: solo identità meccanica e framework; kit e numeri restano TBD.

## Regola sui kit

Le abilità sono organizzate per **personaggio**, non per combinazione. Vedi [Sinergie e combinazioni](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/sinergie-e-combinazioni) e [Fazioni](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/Fazioni).
