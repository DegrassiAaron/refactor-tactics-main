# Spec panel — capability *verticalità tattica* (#2388), 2026-09-08

> `REFERTO` · **Oggetto**: [#2388](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2388) e
> [`../../gameplay/spec-caduta-e-bordi.md`](../../gameplay/spec-caduta-e-bordi.md)
> **Modalità**: `critique` · **Focus**: requirements · architecture · testing
> **Misure**: tutte su `origin/main` = **`13ddc500`**, salvo dove indicato.
> **Esito**: 2 decisioni ([`D-352`](../../decisions/RT_PDR_00_Decision_Log.md) ·
> [`D-353`](../../decisions/RT_PDR_00_Decision_Log.md)), 1 domanda registrata (`VERT-1`), 3 correzioni di
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

**Esito**: [`D-352`](../../decisions/RT_PDR_00_Decision_Log.md). Valori in coda a `ERTMoveOutcome`, sul
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

**Esito**: [`D-353`](../../decisions/RT_PDR_00_Decision_Log.md). Invariante osservabile (`spec` §4.3.1), il
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
- **§9 sul formato** — la nota sul versionamento è viva, ma il **numero** era stale e il panel non se n'era
  accorto: vedi §9.

---

## 9. Ciò che la code review ha trovato, 2026-09-08

🔴 **Il panel ha certificato un numero che aveva letto in un output troncato.** La §9 della spec diceva
*«il formato è oggi `ERTTurnLogFormatVersion::WithMicroStep` = 12»*, e la §4 di questo referto lo ha
promosso a *«è ancora il massimo su `13ddc500`»*. È **falso**: il massimo è `WithSightBlocker` = **13**,
entrato il **2026-09-06** con `0698594b` (#2534). La stale aveva **due giorni** quando il panel l'ha
certificata.

**Come è successo, per non ripeterlo**: la misura dell'enum era stata fatta con un `sed -n '1,60p'` su un
grep — l'output si è fermato a `WithMicroStep = 12` e la riga successiva del file era `Square = 0`, che
sembrava la fine dell'enum. Non lo era: `WithSightBlocker` sta **42 righe più sotto**, dopo un commento
lungo.

📝 **La regola che ne segue**: un numero che si dichiara *«il massimo»* si misura con `tail`, mai con
`head`. Corretto in tre sedi — `spec` §9 e §9.1, `D-352` — prima del merge. **La conclusione di `D-352` non
cambia**: estendere in coda resta non-migrazione, e un campo dedicato costerebbe **14** invece di 13.

---

## 5. Che cosa è cambiato, file per file

| File | Modifica |
|---|---|
| `docs/gameplay/spec-caduta-e-bordi.md` | §4.3.1 nuova (invariante) · §5 riformulata · §9.1 nuova (forma) · §11 nota di revisione |
| `docs/decisions/RT_PDR_00_Decision_Log.md` | `D-352` · `D-353` + le due note di assegnazione |
| `docs/OPEN_DECISIONS.md` | sezione nuova, `VERT-1` |
| #2402 · #2403 · #2406 | note additive, nessun corpo riscritto |
| #2388 | note additive **e** correzione del corpo — tabelle e DoD, dal secondo giro della §7 |

⚠️ **Nessuna modifica a `Source/`.** I due lavori che il panel produce —
`Fall.TwoFallersSameLandingIsDeterministic` e i tre valori d'enum — appartengono a #2402 e #2403, e vanno
scritti là con la loro evidenza. Questo referto li nomina, non li anticipa.

---

## 6. Nota sull'assegnazione dei numeri

`D-348` **non era il primo libero**: è rivendicato dal branch
`docs/166-d348-fast-reaction-duration-configurabile`, aperto e non mergiato, quindi invisibile a un
conteggio fatto sul solo `main`. È la variante che `D-344` aveva già incontrato con una PR; qui a tenere il
numero è il branch di un'altra sessione, che sta lavorando #166 in parallelo. Misura a tre posti ripetuta
per `D-352` e `D-353`: Decision Log massimo `D-347`, **zero** su tutti i branch remoti, **zero**
assegnazioni su GitHub (i match full-text sono cifre dentro altri testi, verificati uno per uno nei corpi).

---

## 7. Secondo giro — i follow-up del panel, passati al panel

I tre follow-up della §3 sono stati rimessi in `critique` lo stesso giorno. **Uno è stato respinto.**

### 7.1 `m2` — l'overload di `HexKnockbackDestination` · **RESPINTO**

Il follow-up diceva: *«un overload che restituisce la ragione dell'arresto — oggi ogni consumatore
ricostruisce la direzione con `DirectionBetween`»*. Due affermazioni, e la seconda è falsa.

**MARTIN FOWLER**: *«La misura non regge il verbo. `DirectionBetween` è chiamata in 12 file, ma il sito che
riguarda la caduta è **uno**: `RTTurnManager_Blast.cpp:90`, dentro `DirezioneSpostamentoForzato` — una
funzione di namespace anonimo che porta il proprio perché scritto sopra: 'Non è nel valore di ritorno di
`HexKnockbackDestination` […] ed è il motivo per cui questa funzione esiste invece di allargare quella
firma, che ha molti chiamanti'. La duplicazione che il follow-up temeva è già stata prevenuta
dall'incapsulamento, dallo stesso autore, nello stesso commit.»*

**KARL WIEGERS**: *«E il requisito non ha uno stakeholder. Chi ha bisogno della **ragione** dell'arresto —
bordo, muro, unità? Un consumatore: la caduta, che la ottiene a valle con `IsEdgeOpen`. Un overload per il
secondo consumatore è un requisito senza richiedente.»*

🔴 **La funzione non è nemmeno una riscrittura banale**, ed è il dettaglio che il follow-up non conosceva:
deve replicare la semantica di `HexKnockbackDestination` — l'**ultimo** passo della linea, non il primo,
perché su celle non allineate `HexLine` zigzaga e `DirectionTowards` risponderebbe un'altra cosa — e
invertirla per la trazione. Un overload che restituisse la ragione **non** eviterebbe questo calcolo.

∴ **Respinto per YAGNI**, che qui non è una preferenza di stile: `CLAUDE.md` §4 vieta di *«introdurre
placeholder per roadmap lontane»* e i *«refactor opportunistici»*.
➡️ **La via di rientro, se un secondo consumatore nasce**: non un overload, ma promuovere
`DirezioneSpostamentoForzato` da funzione anonima a membro di libreria. E nemmeno quello prima del secondo
consumatore.

### 7.2 · 7.3 — colonna «Fase» e DoD spacchettata · **ACCOLTI, e allargati**

Entrambi confermati dalla misura, **entrambi più larghi di come erano formulati**:

- **la colonna** — le sette milestone di fase non sono *«confluite»* in senso lasco: **non esistono**, nemmeno
  chiuse. Il repository ha **11** milestone, tutte per release (`gh api …/milestones?state=all`). Lo stesso
  valore stale vive in **#921**, **#1095** e **#1317**, in una riga `| Milestone |` di §Tracking: nominati
  nella nota, non toccati — sono di altri owner;
- **la DoD** — spacchettarla non bastava. L'inventario dei figli era **incompleto**:
  [#2430](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2430) dichiara *«Capability #2388»*
  nella propria prima riga e non compariva in nessuna delle due tabelle (**zero** occorrenze di `2430` nel
  corpo). L'epic dichiarava sette figli e ne aveva otto. Il difetto che il follow-up nominava — una casella
  che aggrega — ne nascondeva un secondo: una tabella che non elenca.

**LISA CRISPIN**: *«È il caso in cui la correzione trova più di quanto cercava. Se la DoD fosse rimasta
aggregata, l'assenza di #2430 non sarebbe emersa: `#2402…#2407` non nomina ciò che manca fuori dal proprio
intervallo.»*

---

## 8. Nota di metodo

Un panel che rivede i propri follow-up ne boccia uno su tre. Vale la pena registrarlo: i follow-up del primo
giro erano stati scritti **senza** aprire `RTTurnManager_Blast.cpp`, e il rilievo `m2` sarebbe passato per
buono. La misura che l'ha respinto — *«quanti consumatori, e la ricostruzione è già incapsulata?»* — è
esattamente quella che il primo giro non aveva fatto.

---

## 10. La collisione di numerazione, e cosa insegna

Le due decisioni di questo panel sono nate `D-349` e `D-350`. Si chiamano **`D-352`** e **`D-353`**.

**La misura era corretta.** Al momento dell'assegnazione — `origin/main` = `13ddc500` — il massimo del
registro era `D-347`, i numeri erano liberi su **tutti** i branch remoti e su GitHub, e `D-348` era già stato
escluso perché rivendicato dal branch `docs/166-d348-fast-reaction-duration-configurabile` (la variante che
`D-344` aveva già incontrato).

**Poi la PR #2677 ha mergiato `D-349`, `D-350` e `D-351`** mentre questa PR era aperta — dalla sessione che
lavora #166 in parallelo, con la stessa misura a tre posti, fatta correttamente anche lei.

🔑 **È un caso nuovo.** `D-344` descriveva un numero **già rivendicato** da una PR aperta: la misura poteva
trovarlo e non l'aveva cercato. Qui i numeri non erano rivendicati da nessuna parte quando entrambe le
sessioni li hanno presi. Nessuna misura sullo stato poteva prevederlo, perché il fatto che le rende
incompatibili — il merge — non era ancora avvenuto.

📝 **La regola che ne segue**: *la misura a tre posti riduce le collisioni, non le elimina. A decidere è il
**merge**: chi arriva secondo rinumera, e lo dichiara.* Rimisurato prima di riscrivere: `D-352` e `D-353`
liberi su `origin/main` = `0a119308`, su tutti i branch remoti e su GitHub.

⚠️ **I quattro commenti già postati su #2388, #2402, #2403 e #2406 citavano `D-349`/`D-350`**, che ora
puntano a decisioni di un'altra sessione — l'overwatch che non spara, la velocità di playback. Corretti con
una nota additiva su ciascuno: un riferimento sbagliato in una issue è peggio di un riferimento assente,
perché **risolve**.
