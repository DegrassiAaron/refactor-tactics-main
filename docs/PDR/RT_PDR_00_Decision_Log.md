# REFACTORTACTICS — PDR-00 · Decision Log

> **Sorgente Markdown canonica** del Decision Log (PDR-00 §4), residente in Git per la regola di manutenzione
> PDR-00 §6 #5: *«I PDF sono snapshot di consultazione: le sorgenti testuali devono vivere nel repository Git.»*
> Trascrive fedelmente il Decision Log dallo snapshot PDF `RT_PDR_00_Indice_Governance_v0.1.pdf` (pag. 3) e vi
> aggiunge le decisioni successive. **Owner**: PDR-00. **Regola di prevalenza** (PDR-00): *decisioni esplicite
> del progetto > requisiti consolidati > proposte PDR > ricerca web di supporto.*

## Stati delle decisioni (PDR-00 §3 «Principi di consolidamento»)

Gli stati **non vengono mescolati**: `Consolidato · Assunzione · Proposta · Open question`.
Regola di manutenzione #1: *ogni modifica a un requisito aggiunge o aggiorna una voce in questo log.*

## Decision Log

| ID | Decisione | Stato | Impatto |
|---|---|---|---|
| D-001 | Formato principale 3v3; vertical slice 2v2 | Consolidata | Scope, UI, rete, bilanciamento |
| D-002 | Massimo 12 turni; planning 30 s; resolution 6-12 s | Consolidata | Tempo partita e UX |
| D-003 | Server authoritative; client propone | Consolidata | Rete, validazione, anti-cheat |
| D-004 | C++ per simulazione/rete; Blueprint per contenuti e presentazione | Consolidata | Ownership del codice |
| D-005 | GAS non è l'autorità del simulatore | Consolidata | Confine abilities/resolver |
| D-006 | Mappa come grafo tattico 3D con `FRTCellId` | Consolidata | Pathfinding, targeting, ambienti |
| D-007 | UE 5.8 baseline per questa edizione PDR | Assunzione da bloccare | Build, API, toolchain |
| D-008 | Gameplay Framework legacy replication come primo target; Iris valutato dopo vertical slice | Proposta semplice | Riduce rischio iniziale |
| **D-009** | **Le sorgenti canoniche dei PDR vivono in Git (Markdown); i PDF `v0.1` restano snapshot di consultazione. Prima applicazione: PDR-10 → [`RT_PDR_10_Roadmap_QA_Rischi_v0.2.md`](RT_PDR_10_Roadmap_QA_Rischi_v0.2.md)** | **Consolidata** | **Governance/manutenzione (attua PDR-00 §6 #5)** |

## Note

- **D-001…D-008**: trascritti verbatim dallo snapshot `RT_PDR_00_Indice_Governance_v0.1.pdf` (pag. 3). Se il PDF
  viene aggiornato, questa sorgente Git prevale (regola #5) e il PDF va rigenerato di conseguenza.
- **D-007** resta *Assunzione da bloccare*: nel repo la patch è di fatto bloccata a **UE 5.8.1**
  (vedi `CLAUDE.md` e `piano-canonico-mvp.md`); la formalizzazione a *Consolidata* è una decisione futura.
- Divergenze note MVP↔PDR (segnalate, prevale il canone MVP): **rete/privacy** anticipata dal PDR (F1) vs
  differita nell'MVP; **GAS** previsto a F2 vs **No-GAS nell'MVP**. Dettaglio in
  [`../design/roadmap-checkpoint.md`](../design/roadmap-checkpoint.md) §«Allineamento con i PDR».
