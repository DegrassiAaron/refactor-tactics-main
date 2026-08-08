# Obiettivi e fine partita

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-MATCH-END-CONDITIONS -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-MATCH-END-CONDITIONS` · Release: `v0.1` · Roadmap: `E10.3`  
> Stato: **RELEASE_READY** · Gate: `7/8`  
> Scenario: `Visual.Combat.Defeat`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-MATCH-END-CONDITIONS -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-OBJECTIVE-SYSTEM -->

> 🚧 **Parzialmente giocabile.** Il codice esiste ma la feature non è completa: i gate qui sotto dicono quanto manca. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-OBJECTIVE-SYSTEM` · Release: `v0.1` · Roadmap: `E10.1, E10.2`  
> Stato: **IMPLEMENTING** · Gate: `2/8`  
> Scenario: `Spec.Objective.PointSurvivesKO`  
> La partita **puo' finire** per obiettivo, ma in mappa **non esiste ancora un oggetto da attivare**: manca il consumatore, non la regola.  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-OBJECTIVE-SYSTEM -->

## Come si vince

Il modello di partita supporta tre vie di conclusione:

1. **Eliminazione** della squadra avversaria.
2. **Obiettivo** completato o punteggio/condizione obiettivo soddisfatta dal ruleset dello scenario.
3. **RoundLimit** raggiunto.

Il gioco non è quindi progettato per ridursi sempre a «uccidi tutti».

## RoundLimit

Il numero massimo di round è un parametro del formato, non una costante universale.

Per il **2v2 vertical slice** la banda di playtest è circa **10–14 round**; la v0.1 usa **12** come valore iniziale del proprio `RoundLimit`.

Per una futura baseline 3v3 sono stati ipotizzati più round, ma il formato competitivo principale non è ancora deciso.

## Durata desiderata

Il principio è mantenere il gioco **compatto nel tempo**, anche se la mappa può avere spazio tattico significativo.

Per il 2v2 la misura reale e il playtest contano più di un numero teorico. Per una futura baseline 3v3 il target di lavoro è circa 25–30 minuti, con 45 minuti come limite superiore da evitare nella maggioranza delle partite.

## Perché esistono gli obiettivi

Gli obiettivi obbligano le squadre a muoversi, contestare spazio e prendere rischi. Senza di essi una strategia puramente passiva potrebbe diventare troppo efficiente, soprattutto con Overwatch e coperture.

## Fonti normative

- `docs/product/piano-canonico-mvp.md`
- `docs/gameplay/spec-durata-partita-e-scala-mappe.md`
- `docs/roadmap/roadmap-v0.1.md`
