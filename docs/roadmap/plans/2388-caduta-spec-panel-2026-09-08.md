# Spec panel — capability *verticalità tattica* (#2388), 2026-09-08

> `REFERTO` · **Oggetto**: [#2388](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2388) e
> [`../../gameplay/spec-caduta-e-bordi.md`](../../gameplay/spec-caduta-e-bordi.md)
> **Modalità**: `critique` · **Focus**: requirements · architecture · testing
> **Misure**: tutte su `origin/main` = **`13ddc500`**, salvo dove indicato.
> **Esito**: 2 decisioni ([`D-349`](../../decisions/RT_PDR_00_Decision_Log.md) ·
> [`D-350`](../../decisions/RT_PDR_00_Decision_Log.md)), 1 domanda registrata (`VERT-1`), 3 correzioni di
> testo, 2 lavori nuovi nominati.

---

## 1. Corpus letto

| Artefatto | Che cosa è |
|---|---|
| #2388 | l'epic — owner cross-release della capability |
| `docs/gameplay/spec-caduta-e-bordi.md` (249 righe) | owner semantico delle regole |
| #2402 · #2403 · #2404 · #2406 | il lavoro figlio della lane CODE letto per intero |
| `Source/RefactorTactics/Tests/RTFallOverLedgeTests.cpp` | 14 test, letti per nome e due per corpo |
| `Turn/RTTurnLog.h` · `Map/RTHexCellData.h` · `Map/RTHexLedgeLibrary.h` | il codice che la spec dichiara |

---

## 2. Valutazione

| Dimensione | Score | Nota |
|---|---|---|
| Chiarezza | 8,5/10 | la colonna *«come si sa»* del §2 è materiale di riferimento |
| Completezza | 7,0/10 | due buchi: la *forma* della traccia, e il giocatore |
| Testabilità | 7,5/10 | 14 test reali; una clausola di fuga, un caso concorrente scoperto |
| Consistenza | 8,0/10 | due stale minori |

Giudizio del panel, non una misura strumentale.

---

## 3. Findings

### C1 · La §9 dice **che** la traccia distingue, non **come** — e il codice ha un valore solo

**Wiegers + Fowler.** §4 dichiara tre esiti e §9 chiede che la traccia li distingua. In codice
`ERTMoveOutcome::Fell` è **uno** (`Turn/RTTurnLog.h:670`, indice **15**). Restavano tre design incompatibili
— valori d'enum, estensione di `ERTDisplacementBlockReason`, campo dedicato — e nessuno li aveva scelti: chi
implementava #2403 avrebbe deciso da solo su un formato che ha un owner.

**Esito**: [`D-349`](../../decisions/RT_PDR_00_Decision_Log.md). Valori in coda a `ERTMoveOutcome`, sul
precedente `Displaced`/`DisplacementResisted` e `Slid`/`SlideBlocked` che è **dentro lo stesso enum**. Nasce
un **quarto** valore che la §9 non chiedeva, perché il §4 ⚠️ dichiara la colonna senza fondo *«una caduta
senza atterraggio»* — una differenza dichiarata e non leggibile è peggio di una differenza non dichiarata.
Scritto in `spec` §9.1.

### C2 · Il caso saturo aveva una clausola di fuga, e la concorrenza non è coperta

**Nygard + Crispin.** §4.3 identificava il failure mode giusto — *«nello stesso Blast un'altra unità può
aver preso `LastStableCell`»* — e poi lo declassava a *«modello consigliato»* con l'eccezione *«a meno che
l'architettura del resolver lo imponga»*. Un requisito con eccezione discrezionale è soddisfatto da
qualunque implementazione, inclusa quella che sovrappone.

Misura: dei 14 test di `RTFallOverLedgeTests.cpp`, `Fall.NeverOverlaps` (riga 641) mette in scena **una**
caduta e un occupante fermo — tre unità, una spinta. Nessuno ha **due** unità che cadono nello stesso Blast;
`simultan`/`contemporane` danno **zero** occorrenze nel file.

**Esito**: [`D-350`](../../decisions/RT_PDR_00_Decision_Log.md). Invariante osservabile (`spec` §4.3.1), il
modello resta **raccomandato** e non normativo — prescrivere la sequenza interna sarebbe un secondo resolver
in prosa, vietato dal guardrail 2 della spec stessa. Lavoro che ne nasce:
`Fall.TwoFallersSameLandingIsDeterministic` su #2402, mutazione al gate #2406.

### M1 · La DoD aggregava sei issue e nascondeva un blocco decisionale

**Wiegers.** `- [ ] #2401 ✅ · #2402…#2407 da chiudere` è una casella che copre sei issue e non dice quale
manchi. #2402 è aperta **di proposito** — la casella D010 aspetta **#2501** (*«Il Move ricalcola verso la
destinazione dopo una spinta: è il Model B che D-045 esclude»*, `OPEN`), e l'epic non nominava #2501 da
nessuna parte.

**Esito**: nota additiva su #2388.

### M2 · Manca l'attore, e la leggibilità è P2 dietro una meccanica P1

**Cockburn.** §1 pone il problema al sistema, mai al giocatore. La preview della minaccia è #2405, **P2** e
a valle di #2402/#2403 che sono **P1**. §8 esclude l'input durante la risoluzione — deciso — ma nessuna riga
dice cosa il giocatore vede **prima**.

**Esito**: non decidibile dal panel, è di prodotto. Registrata come `VERT-1` in
[`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), con innesco *«prima che #2403 chiuda»*.

### m1 · Una misura della spec si era auto-invalidata

**Adzic.** §5 diceva *«`FallEffects`, `ImpactEffects` e `FallDamage` hanno zero occorrenze in `Source/`»*.
Misurato: `FallDamage` **0** ✅, ma gli altri due **1 ciascuno** — `Tests/RTFallOverLedgeTests.cpp:12`, il
commento che ripete quella frase. È il rilievo che #166 fa per `FastReactionDuration`. **Esito**: riscritta
con la formula del Decision Log — *le sole occorrenze sono le note che lo dichiarano*.

### m2 · Debito dichiarato e mai registrato

**Fowler.** #2402 D003 vieta di cambiare la firma di `HexKnockbackDestination` e impone di ricostruire la
direzione a valle con `DirectionBetween`. Vincolo locale corretto; ma ogni consumatore futuro ripeterà la
ricostruzione, e l'epic non lo elencava fra i follow-up. Un **overload** che restituisce la ragione non
sarebbe un cambio di firma. **Esito**: follow-up candidate su #2388.

### m3 · La colonna «Milestone» nominava fasi che non sono più milestone

**Wiegers.** «Mondo giocabile», «Leggibilità», «Gate di release», «Prova integrata» sono le fasi confluite
nella milestone canonica unica col consolidamento del 2026-09-06 (descrizione della milestone #6: *«le sette
milestone di fase v0.1 … confluiscono qui. Le fasi restano rappresentate da Epic, checkpoint e label»*).
Corretta come **fase**, stale come **milestone**. **Esito**: nota additiva su #2388.

---

## 4. Ciò che il panel ha misurato e non ha toccato

- **§2, la colonna «come si sa»** — `FRTHexEdgeGuard` vive su `Map/RTHexCellData.h:266`, con
  `TArray<FRTHexEdgeGuard> Guards` a :416, accanto a `Covers` e `Doors`: esattamente dove §2 argomenta che
  debba stare, e **non** su `FRTHexEdge`. L'argomento della spec ha retto all'implementazione.
- **§4.2 punti 4–6** — pinnato da `Fall.AlternativeFollowsCanonicalRingFromFacing`; il Facing è dichiarato
  tie-break e non fonte d'ordine.
- **L'asimmetria spinta/trazione** — `ForcedMovement.PullOverOpenLedgeStartsFall` copre un caso che nessuna
  regola dichiarava, e che §3 conteneva già dicendo *«spostamento forzato»* invece di *«spinta»*.
- **#2406 come gate di mutazione** — cinque mutazioni nominate, il verso della misura dichiarato,
  l'avvertenza sul mutex globale del motore. Più rigoroso della media del repository.
- **§9 sul formato** — `WithMicroStep` = **12** è ancora il massimo su `13ddc500`: la nota è viva.

---

## 5. Che cosa è cambiato, file per file

| File | Modifica |
|---|---|
| `docs/gameplay/spec-caduta-e-bordi.md` | §4.3.1 nuova (invariante) · §5 riformulata · §9.1 nuova (forma) · §11 nota di revisione |
| `docs/decisions/RT_PDR_00_Decision_Log.md` | `D-349` · `D-350` + le due note di assegnazione |
| `docs/OPEN_DECISIONS.md` | sezione nuova, `VERT-1` |
| #2388 · #2402 · #2403 · #2406 | note additive, nessun corpo riscritto |

⚠️ **Nessuna modifica a `Source/`.** I due lavori che il panel produce —
`Fall.TwoFallersSameLandingIsDeterministic` e i tre valori d'enum — appartengono a #2402 e #2403, e vanno
scritti là con la loro evidenza. Questo referto li nomina, non li anticipa.

---

## 6. Nota sull'assegnazione dei numeri

`D-348` **non era il primo libero**: è rivendicato dal branch
`docs/166-d348-fast-reaction-duration-configurabile`, aperto e non mergiato, quindi invisibile a un
conteggio fatto sul solo `main`. È la variante che `D-344` aveva già incontrato con una PR; qui a tenere il
numero è il branch di un'altra sessione, che sta lavorando #166 in parallelo. Misura a tre posti ripetuta
per `D-349` e `D-350`: Decision Log massimo `D-347`, **zero** su tutti i branch remoti, **zero**
assegnazioni su GitHub (i match full-text sono cifre dentro altri testi, verificati uno per uno nei corpi).
