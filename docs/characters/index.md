# RefactorTactics — Personaggi

La Wiki distingue il roster operativo dalle basi asset Paragon. Dal 2026-08-08 ogni eroe dichiara **quale**
slot Paragon usa come base visuale ([D-037](../decisions/RT_PDR_00_Decision_Log.md)); la tabella completa sta in
[`paragon.md`](paragon.md#mapping-visuale-del-roster).

## Roster v0.1 — RefactorTactics

I nomi canonici del roster sono fissati da [D-120](../decisions/RT_PDR_00_Decision_Log.md) (2026-08-12) e dal
2026-08-13 **coincidono** con lo slot Paragon che ciascun eroe usa come base visuale.

| Nome canonico | Scheda | Stable ID (legacy) | Base visuale |
|---|---|---|---|
| **Aevik** | [scheda](v0.1/aevik.md) | `Hero.Aevik` | `Paragon.Aevik` |
| **Muiren** | [scheda](v0.1/muiren.md) | `Hero.Muiren` | `Paragon.Muiren` |
| **Riktor** | [scheda](v0.1/riktor.md) | `Hero.Riktor` | `Paragon.Riktor` |
| **Wraith** | [scheda](v0.1/wraith.md) | `Hero.Wraith` | `Paragon.Wraith` |
| **Muriel** ⏳ | [scheda](v0.1/muriel.md) (`DESIGN_SPEC`, prende il posto di Muiren: [D-586](../decisions/RT_PDR_00_Decision_Log.md)) | `Hero.Muriel` dopo #4011 | `Paragon.Muriel` |
| **Kallari** ⏳ | [scheda](v0.1/kallari.md) (`DESIGN_SPEC`, prende il posto di Wraith / Ivrin: [D-586](../decisions/RT_PDR_00_Decision_Log.md)) | `Hero.Kallari` dopo #4011 | `Paragon.Kallari` |

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

> 🔁 **Dal 2026-11-08 ([D-586](../decisions/RT_PDR_00_Decision_Log.md) (1)(4))**: Aurora e Kwang escono; entrano **The Fey** ([scheda](v0.2/the-fey.md)) e **Feng Mao** ([scheda](v0.2/feng-mao.md)), `DESIGN_SPEC` tradotte dal GDD, in v0.2 (E35.2). Steel e Murdock restano, in v0.1 (E35.1), e prendono il nome Paragon con [D-585](../decisions/RT_PDR_00_Decision_Log.md). La tabella sotto è conservata com'era.

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
