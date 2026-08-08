# RefactorTactics — Personaggi

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-CHAR-PRESENTATION -->

> 🚧 **Parzialmente giocabile.** Il codice esiste ma la feature non è completa: i gate qui sotto dicono quanto manca. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-CHAR-PRESENTATION` · Release: `v0.1` · Roadmap: `E21.1, E21.2, E21.3`  
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

- [Flux](v0.1/flux.md) — base visuale: `Paragon.Gadget`
- [Riva](v0.1/riva.md) — base visuale: `Paragon.Phase`
- [Bastion](v0.1/bastion.md) — base visuale: `Paragon.Riktor`
- [Vektor](v0.1/vektor.md) — base visuale: `Paragon.Wraith`

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

Le abilità sono organizzate per **personaggio**, non per combinazione. Vedi [Sinergie e combinazioni](../wiki/game/sinergie-e-combinazioni.md) e [Fazioni](../wiki/fazioni/index.md).
