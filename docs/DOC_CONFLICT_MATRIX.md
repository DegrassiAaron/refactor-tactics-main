# Matrice dei conflitti documentali

> **Stato**: vivo · **Ultimo aggiornamento**: 2026-08-07 · **Owner**: questo file
> **Scopo**: registrare dove due documenti dicono cose diverse, e cosa vale oggi.
> **Regola**: un conflitto non si risolve in silenzio. O si registra `SUPERSEDED` con la fonte che prevale,
> o diventa una voce di [`OPEN_DECISIONS.md`](OPEN_DECISIONS.md). Mai una scelta implicita.

## Stati

| Stato | Significato |
|---|---|
| `CONFIRMED` | La specifica corrente è l'unica; nessuna azione |
| `SUPERSEDED` | Una specifica precedente è stata sostituita, e la sostituzione è registrata |
| `CONFLICT` | Due fonti si contraddicono e **nessuna prevale**: serve una decisione |
| `OPEN` | Tema deciso in un sorgente, **mai recepito** in un documento normativo |
| `DUPLICATE` | La stessa regola definita in più posti → consolidare su un solo owner |

---

## Matrice

| # | Area | Specifica precedente | Specifica corrente | Fonte che prevale | Stato | Azione |
|---|---|---|---|---|---|---|
| 1 | Griglia | quadrata 10×10, 4-way, Manhattan | esagonale assiale `FRTCellId{q,r,Layer}`, 6 vicini + archi fra layer | [ADR-0002](decisions/adr-0002-griglia-esagonale.md) | `SUPERSEDED` | ✅ documenti superati marcati il 2026-08-07 |
| 2 | Ordine fasi | `Movement + Action`; `Preparation→Movement→Actions` | `Planning → Prep → Dash → Blast → Move → Cleanup` | [ADR-0003](decisions/adr-0003-modello-azioni-v01.md) | `CONFIRMED` | — |
| 3 | Move ultima fase volontaria | implicito | esplicito: il Move resta **dopo** il Blast | [ADR-0003](decisions/adr-0003-modello-azioni-v01.md) §3 | `CONFIRMED` | — |
| 4 | Finestra di reazione | interrupt 5 s · 7–8 s · `Reaction Charge` | **3,0 s**, `Timeout → HOLD` | [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) §8 · D-010 | `SUPERSEDED` | ✅ |
| 5 | Nome del parametro | `FastDecisionDuration` | `FastReactionDuration` | [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) | `DUPLICATE` | ✅ un solo nome nel codice |
| 6 | Modello delle reazioni | deterministiche, senza finestre | unico `opportunity → commit`; l'attuale è `AllowedResponses ≤ 1` | [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) (D17) | `CONFIRMED` | — |
| 7 | Overwatch | skill del singolo eroe | caso concreto del modello generale di reazione | [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) · E14 | `CONFIRMED` | — |
| 8 | **Overwatch universale** | — | azione di Planning **per tutti**, effetto dal profilo di eroe/equipaggiamento; **compete** con l'azione offensiva | [D-012](decisions/RT_PDR_00_Decision_Log.md) | `CONFIRMED` | ✅ chiuso 2026-08-07 · owner [`gameplay/brief-azioni-generiche-overwatch.md`](gameplay/brief-azioni-generiche-overwatch.md) |
| 9 | Action Ghosts | — | Ghost Timeline per fase, presentation-only | [`technical/brief-planning-visuale.md`](technical/brief-planning-visuale.md) | `CONFIRMED` | CP 11.5/11.6 |
| 10 | Delayed Actions | — | dichiarate in Planning, risolvono a un boundary nominato | [`gameplay/brief-delayed-actions.md`](gameplay/brief-delayed-actions.md) | `CONFIRMED` | nessuna epic, deliberato |
| 11 | **Trigger su transizione** | «gli archi portano trigger?» — domanda mal posta: gli adiacenti **non sono dati** | la **trap possiede** la coppia `(From→To)`; `FRTHexEdge` resta per i soli salti di layer | [D-013](decisions/RT_PDR_00_Decision_Log.md) | `CONFIRMED` | ✅ chiuso 2026-08-07 · mappa invariata, nessun vincolo su E9 |
| 12 | Roster | Aegis/Nyx/Drift/Vex · Mara/Ivo/Nyx/Sol · Steel/Aurora/Murdock/Kwang | **Flux · Riva · Bastion · Vektor** | [`balance/RT_HeroCatalog_v0.1.md`](balance/RT_HeroCatalog_v0.1.md) + codice | `SUPERSEDED` | ✅ i nomi vecchi restano solo in righe che li dichiarano storici |
| 13 | Fog of War | north-star P1 | **non** è FoW: conoscenza parziale a 3 livelli, mappa statica nota | [`gameplay/brief-conoscenza-parziale.md`](gameplay/brief-conoscenza-parziale.md) D1 | `CONFIRMED` | E13 |
| 14 | Team Knowledge | — | unione per squadra + `UltimoContatto` (1 turno) | [`gameplay/brief-conoscenza-parziale.md`](gameplay/brief-conoscenza-parziale.md) D3/D5 | `CONFIRMED` | CP 13.1/13.2 |
| 15 | Rumore | debuff | secondo canale percettivo, flood fill intero sul grafo | [`gameplay/brief-conoscenza-parziale.md`](gameplay/brief-conoscenza-parziale.md) §12 | `CONFIRMED` | CP 13.3/13.4 |
| 16 | **Unità ausiliarie** | — | concetto unico `AuxiliaryUnit`; in v0.1 entrano **solo i vincoli architetturali**, nessun gameplay | [`gameplay/brief-unita-ausiliarie.md`](gameplay/brief-unita-ausiliarie.md) | `CONFIRMED` | ✅ chiuso 2026-08-07 |
| 17 | Durata match | 20–30 min come invariante | 3v3 Standard **25–30 min**, tetto ~45; parametro di formato | D-010 | `SUPERSEDED` | ✅ D-002 barrata, non cancellata |
| 18 | Turn cap | «12 turni» costante | `RoundLimit` da `URTMatchFormatData`: 10–14 in 2v2, 16–20 in 3v3 | D-010 · CP 10.3 | `SUPERSEDED` | ✅ implementato e testato |
| 19 | Planning duration | 30 s universale | 30 s nel 2v2 corrente, **40–45 s** baseline 3v3 | D-010 §7 | `SUPERSEDED` | ✅ |
| 20 | **Formato principale** | D-001: **3v3** principale, *Consolidata* | **non deciso**: D-001 declassata ad *Assunzione da bloccare*; 3v3 resta baseline, 4v4 solo stress (E17) | [D-011](decisions/RT_PDR_00_Decision_Log.md) | `CONFIRMED` | ✅ chiuso 2026-08-07 — si consolida con la **prima misura** su una partita ≥3v3 |
| 21 | GAS | previsto in F2 dal PDR | **no-GAS**: `URTActionData : UPrimaryDataAsset` | canone | `CONFIRMED` | divergenza dichiarata |
| 22 | Multilivello | 2D piatto | `Layer` in `FRTCellId`, A\* multilivello | [ADR-0002](decisions/adr-0002-griglia-esagonale.md) · PF.4 | `CONFIRMED` | — |
| 23 | Testing automatico | test unitari + Automation | **RT Scenario Test Harness**: scenari JSON → percorso di gioco reale → `result.json` | [`technical/test-automatico-unreal.md`](technical/test-automatico-unreal.md) | `CONFIRMED` | 🟡 primo blocco atterrato |
| 24 | Numerazione roadmap | F0–F6 del PDR | M6–M11 (esecuzione) + E1–E17 (release), **mappate** sulle F | [`roadmap/roadmap-checkpoint.md`](roadmap/roadmap-checkpoint.md) | `CONFIRMED` | non rinumerare |
| 25 | Determinismo | — | snapshot + RulesVersion + seed ⇒ stesso `StateHash`/`LogHash`; le Fast Decision entrano nel TurnLog **come dato** | invariante #4 · [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) | `CONFIRMED` | — |
| 26 | Privacy dell'intento | — | `FilterForTeam → FRTIntentView`; canary a M10 | invariante #6 | `CONFIRMED` | — |

**Riepilogo al 2026-08-07**: **24 risolti** (`CONFIRMED`/`SUPERSEDED`) · 1 `DUPLICATE` chiuso · **0 `OPEN`** · **0 `CONFLICT`**.

> Le quattro voci aperte dalla revisione documentale sono state chiuse dalla sessione `/sc:brainstorm` del
> 2026-08-07 (`D-011`, `D-012`, `D-013` + due brief). Due delle domande erano **mal poste**, e lo si è scoperto
> guardando il codice: vedi la nota di metodo in [`OPEN_DECISIONS.md`](OPEN_DECISIONS.md).

---

## Come si aggiorna

1. Quando una decisione ne supera un'altra, aggiungi o modifica una riga qui **e** una voce nel
   [Decision Log](decisions/RT_PDR_00_Decision_Log.md). Barra la vecchia, non cancellarla.
2. Se non è chiaro quale fonte prevalga, lo stato è `CONFLICT` e la riga rimanda a
   [`OPEN_DECISIONS.md`](OPEN_DECISIONS.md). Non scegliere per plausibilità.
3. Un tema deciso in `src/` ma mai recepito in un documento normativo è `OPEN`, non `CONFIRMED`: un
   sorgente non è una specifica finché qualcuno non ne è owner.
