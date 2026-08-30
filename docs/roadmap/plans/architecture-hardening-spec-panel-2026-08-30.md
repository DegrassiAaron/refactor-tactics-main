# Architecture Hardening — audit SOLID e confini di sistema

> **Tipo**: referto di sessione spec-panel · **Data**: 2026-08-30 · **HEAD misurato**: `b45c314b`
> **Epic**: [E50 #1816](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1816)
>
> Non è un owner e non aggiunge canone. L'owner architetturale resta
> [`architettura-codice.md`](../../technical/architecture/architettura-codice.md); le decisioni restano del
> Decision Log. Qui c'è **cosa è stato misurato e quando**, perché fra sei mesi la domanda sarà «era già
> così?» e nessun documento normativo risponde a quella.

---

## 1. Snapshot del repository

| Voce | Valore |
|---|---|
| HEAD dell'audit | `b45c314b` (2026-08-30) |
| Branch di lavoro | `refactor/1817-playback-fuori-dal-turnmanager` |
| PR aperte al preflight | 5 — nessuna su `Turn/`, `Combat/`, `Map/` |
| Ultimo `D-nnn` locale | D-284 |

⚠️ **Due percorsi del mandato non esistono**: `docs/architecture/**` e `docs/reviews/**`. Gli owner reali sono
`docs/technical/architecture/` e — per i referti — `docs/roadmap/plans/`. Questo file segue la convenzione
misurata, non quella del mandato: creare `docs/reviews/` avrebbe aperto un terzo albero per un solo file.

---

## 2. La premessa del mandato non regge alla misura

Il mandato presupponeva accoppiamento diffuso, rischio di determinismo e violazioni Open/Closed. **Sette delle
undici aree candidate erano già a posto**, e vale la pena scriverlo con i numeri, perché il prossimo audit
partirà dallo stesso sospetto.

| Area sospettata | Misura | Esito |
|---|---|---|
| Simulazione accoppiata al world | `GetWorld` · `GetGameMode` · `GetSubsystem` · `GetAllActorsOfClass` nelle ~22 library di regole: **0** | **già isolata** |
| RNG non seeded | `FMath::Rand` · `FRandomStream` · `rand()` fuori dai test: **0 occorrenze** | **nessun RNG** |
| Ordering instabile nell'A\* | `GraphNeighbors` restituisce `TArray<TPair<>>`, non una `TMap`; tie-break esplicito `URTHexLibrary::StableLess` | **falso allarme** |
| `TMap` con chiave puntatore | 9 mappe in `RTBlastContext.h`, tutte usate **solo** in `.Find()` | **mai iterate** |
| Wall clock nell'esito | 6 usi: pacing, durata del replay, nome file | **fuori dagli esiti** |
| Branch per contenuto (OCP) | 3 confronti su ID in tutto il core, di cui 1 reale | **data-driven** |
| Leak di privacy | `Replicated` · `NetMulticast` · RPC · `GetLifetimeReplicatedProps`: **0** | **niente da violare** |

Dove il codice aveva già affrontato il problema, lo aveva anche **scritto**: `ReportOrphanRecordedDecisions`
ordina per chiave e dice perché — «un verdetto che cambia riga a ogni esecuzione sarebbe un non-determinismo
introdotto proprio dal codice che verifica il determinismo»; `ARTUnit::GetActiveStatusNames` ordina e cita
l'invariante #4; `RTCombatResolver::ApplyFirstHitDelta` dichiara «nessuna `TMap` in mezzo».

**Zero P0.** Non per prudenza: per assenza di evidenza dopo averla cercata.

---

## 3. SOLID — prima

| | Score | Evidenza |
|---|---|---|
| **S** | 4/10 | `ARTTurnManager`: 1 749 + 6 479 + 2 184 = **10 412 righe**, ~90 metodi, dodici responsabilità |
| **O** | 8/10 | 3 confronti su ID, 6 `switch` su tipo, abilità data-driven |
| **L** | 8/10 | gerarchie piatte, nessun override divergente, nessun virtual vuoto |
| **I** | 5/10 | `RTTurnManager.h` (1 749 righe) è l'unica interfaccia anche per chi legge soltanto |
| **D** | 7/10 | ottimo nelle library; i 51 accessi al world stanno tutti negli orchestratori |
| Determinismo | 8,5/10 | vedi §2 |

Il debito **non è diffuso: è concentrato in una classe**, e ha una conseguenza quantificata —
**74 test su 153 (48 %) spawnano un mondo**, 66 dipendono da `ARTTurnManager`.

---

## 4. Riconciliazione del tracking

| Candidate | Owner esistente | Azione |
|---|---|---|
| Isolate Simulation Dependencies | `architettura-codice.md` §Principi 1 | **NO ACTION** — già fatta |
| Data-driven OCP | `URTCatalogLibrary` | **NO ACTION** — già fatta |
| Determinism Guardrails | **E12 #26** | **REUSE** — nessuna issue nuova |
| Map/Grid Query Boundaries | #1777 (D-250) | **REUSE** |
| Snapshot/TurnLog Value Boundaries | #1800 | **UPDATE** — commento con `FRTResolvedEvent` |
| Intent Privacy Guardrail | #589 · E40 #773 | **DEFER** — nessun leak attuale |
| Header/Include Hygiene | — | **DEFER** — P3, non con P1 aperti |
| TurnManager Orchestration | **nessuno** | **CREATE** → #1818 |
| Simulation→Presentation | #1500 · #1801 · #1525 confinano, nessuno possiede il playback | **CREATE** → #1817 |
| Slim Framework Responsibilities | **nessuno** | **CREATE** → #1820 (DEFER) |
| Read/Write API | **nessuno** | **CREATE** → #1821 |

### Decisione sull'Epic

Nessuna Epic equivalente esisteva — ricerca per «SOLID», «architettura», «refactor», «God Object»,
«accoppiamento», «debito tecnico», «boundary», «GameState»: zero risultati pertinenti. Massimo assegnato
**E49** (#1769) → creata **E50** ([#1816](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1816)),
una sola.

- **Create**: #1817 (P1) · #1818 (P1) · #1820 (P2, DEFER) · #1821 (P2)
- **Aggiornata**: #1800 · **Riusate**: #26, #1777
- **Deferred**: privacy → #589 / E40 #773 · header hygiene

---

## 5. Refactor applicato — #1817, prima fetta

`ARTTurnManager::DurationForPlaybackPhase` → `URTPlaybackLibrary::PhaseDuration`.

Il TurnManager conserva la sola raccolta degli ingressi — il percorso più lungo fra le anim di quella fase,
l'unico dato che la library non possiede. La formula è identica riga per riga: stesso `Max(1, NumAttacks)`,
stesso `Max` invece della somma nel Blast, stessa guardia su `CellsPerSecond <= 0`, stesso beat per le fasi
che non muovono.

Vincoli verificati **prima** di iniziare, non dopo:

- **zero riferimenti Blueprint**: `grep` su `Content/` per tutti e tredici i simboli di playback → 0.
  Nessun `.uasset` toccato, nessun binario da fondere;
- consumatori esterni contenuti: `RTHUD` (7), `RTPlayerController` (3), 5 file di test;
- rete di sicurezza già esistente: `RefactorTactics.Match.Autobattle.DeterminismIsIndependentOfPlayback`.

**Cinque test nuovi, tutti headless** — coprono ciò che prima costava un mondo: il movimento in parallelo, i
due rami del `Max` nel Blast, il pavimento di un colpo quando non ce ne sono, il beat delle fasi ferme, e la
velocità nulla che nel Blast azzera la spinta ma non i colpi.

### Un difetto trovato per strada

`URTPlaybackLibrary::EstimatePlaybackSeconds` **è coperta da quattro asserzioni e non è chiamata da nessun
codice di produzione**. La durata reale la calcola `PhaseDuration`, con un'altra formula. Due verità sulla
stessa cosa, una delle quali verde e morta.

Non è stata rimossa in questo diff: cancellarla avrebbe cancellato quattro test dentro un commit che parla
d'altro. La distinzione è ora scritta accanto a `PhaseDuration`, perché il prossimo lettore non ne scelga una
a caso.

---

## 6. Evidenza di build e test

| Prova | Esito |
|---|---|
| Build `RefactorTacticsEditor Win64 Development` | **`Result: Succeeded`**, 210,88 s |
| `rt-suite.ps1 -Filter RefactorTactics.Playback` | **VALIDA** · 18/18 completati · 0 fallimenti · 04:35 |
| `rt-suite.ps1` (suite intera) | vedi §8 |

---

## 7. Adversarial review del diff

Riletto assumendo che contenesse errori. Cinque ipotesi, tutte risolte con evidenza:

1. **Divisione intera?** `FMath::Max(0, MaxMoveSegments) / CellsPerSecond` — `int32 / float` promuove a
   float. Identico all'originale.
2. **Il clamp `Max(0, …)` non c'era prima.** Il chiamante reale non produce mai un valore negativo, quindi
   non cambia comportamento; è una guardia su una funzione ora pubblica.
3. **`ERTMatchPhase` in una `UFUNCTION(BlueprintPure)`** richiede `UENUM(BlueprintType)` — lo è, e UHT
   avrebbe fatto fallire la build altrimenti.
4. **Regressione di performance**: per `Prep` e `Cleanup` il loop su `MoveAnims` ora viene eseguito e prima
   no. `MoveAnims` ha al più un elemento per unità (≤ 4 in 2v2) e il loop gira una volta per fase in
   `BeginPlayback`. **Dichiarata, quantificata, trascurabile** — non nascosta.
5. **Include in un header pubblico**: `RTPlaybackLibrary.h` include ora `Turn/RTTurnRules.h`. Dei 10
   consumatori, **7 includevano già quella catena**; ne pagano 3, nessuno in un percorso caldo. Non
   eliminabile con una forward declaration, perché UHT vuole il tipo completo.

Nessun ciclo di dipendenze, nessun cambio di lifetime `UObject`, nessun delegate spostato, nessuna modifica a
serializzazione, hash o replay — il playback non entra in alcun hash.

---

## 8. Ciò che questo referto NON prova

- **la suite intera non è ancora registrata.** Al primo tentativo `rt-suite.ps1` è uscita **2**: motore
  occupato da `D:\Repositories\rt-wt-vs`, un altro checkout. Non terminato — è lavoro di qualcun altro.
  L'unica misura registrata è quella del filtro `RefactorTactics.Playback`;
- **nessun passo PIE**: il playback si vede, e vederlo è una verifica manuale;
- i P1 restanti di #1818 non sono toccati. Questa è **una** fetta.

### Il branch è cambiato sotto la sessione

Alle **18:52:57** un'altra sessione ha **ricreato** `refactor/1817-playback-fuori-dal-turnmanager` da un altro
HEAD, e alle 18:53:57 è tornata sul proprio branch — lasciando le quattro modifiche non committate
nell'albero *del suo* branch, dove alle 18:54:16 ha committato. Il suo commit `5215599a` tocca **solo**
`docs/technical/test-manuali-pie.md`: la misura 18/18 vale quindi per il codice, perché nessun sorgente altrui
era nell'albero durante la run.

Nessun lavoro perso, ma solo perché non c'erano ancora commit da perdere. È il caso descritto in
[`CLAUDE.md` §7](../../../CLAUDE.md) (**D-222**) osservato dal vivo, ed è la ragione per cui
`git branch --show-current` va letto **subito prima** del commit e non a inizio lavoro.

---

## 9. SOLID — dopo (questa fetta soltanto)

| | Prima | Dopo | Perché |
|---|---|---|---|
| **S** | 4/10 | 4/10 | 17 righe su 10 412: la classe è ancora un God Object. Onestà, non modestia |
| **D** | 7/10 | 7,5/10 | una formula in più dipende da input espliciti invece che dallo stato dell'Actor |
| Testabilità | — | **+5 test headless** | cinque casi che prima richiedevano un mondo |

Il numero che conta per E50 non è il punteggio: è **quanti test hanno bisogno di un mondo**. Oggi 74 su 153.
Va rimisurato a ogni fetta e riportato in #1818.

---

## Verdetto

**SAFE WITH MINOR FIXES** — dove il *minor fix* è uno solo e dichiarato: **la suite intera va eseguita e
registrata prima del merge**. Il filtro mirato è verde e valido, la build è verde, il diff è stato riletto in
avversariale e non introduce né P0 né P1.
