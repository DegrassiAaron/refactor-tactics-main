# Architecture Hardening — rimisura di E50 e l'asse che mancava

> **Tipo**: referto di sessione spec-panel · **Data**: 2026-09-03 · **HEAD misurato**: `bbb20f16`
> **Epic**: [E50 #1816](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1816) ·
> **Issue**: [#1818](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1818)
>
> Non è un owner e non aggiunge canone. L'owner architetturale resta
> [`architettura-codice.md`](../../technical/architecture/architettura-codice.md); le decisioni restano del
> Decision Log. Continua [l'audit del 2026-08-30](architecture-hardening-spec-panel-2026-08-30.md), che
> chiude chiedendo che la misura sia **rifatta a ogni fetta**: questo è quel rifacimento, quattro giorni e
> ottanta commit dopo.

---

## 1. Snapshot

| Voce | Valore |
|---|---|
| HEAD misurato | `bbb20f16` (2026-09-03) |
| Confronto con | `b45c314b` (audit del 2026-08-30) |
| Commit di distanza | 80 su `main` |
| Branch di lavoro | `audit/1818-complessita-per-funzione`, worktree `D:/Repositories/rt-wt-e50` |
| Commit che hanno toccato `RTTurnManager.*` nel periodo | **53** |

⚠️ **Il checkout principale era stantio.** `refactor-tactics-main` era fermo su `fix/2118-verifica-byte`,
**80 commit** dietro `origin/main`, con un commit locale non ancora mergiato di un'altra sessione. La prima
tornata di misure di questa sessione è stata presa lì ed è stata **buttata**: sul checkout vecchio
`ResolveCombatPasses` misura 789 righe, su `main` ne misura 888. Il lavoro è ripreso in un worktree nuovo,
senza toccare il branch altrui.

---

## 2. Le metriche di E50 non sono riproducibili

L'audit del 2026-08-30 chiude così: *«Il numero che conta per E50 non è il punteggio: è quanti test hanno
bisogno di un mondo. Oggi 74 su 153. Va rimisurato a ogni fetta e riportato in #1818.»*

Il numero c'è. Il **comando** che lo produce no. Rimisurando sullo stesso commit `b45c314b`:

| Grandezza dichiarata | Valore dichiarato | Rimisura su `b45c314b` | Esito |
|---|---|---|---|
| file di test (denominatore) | 153 | **153** | ✅ riproducibile |
| test che «spawnano un mondo» | 74 | `SpawnActor` → **67** · `UWorld` → **80** | ❌ nessuna variante dà 74 |
| test che dipendono da `ARTTurnManager` | 66 | `ARTTurnManager` → **62** · `RTTurnManager` → **69** · `TurnManager` → **70** | ❌ nessuna variante dà 66 |

Otto pattern provati su due perimetri (`Source/RefactorTactics/Tests/*.cpp` e tutti i `Tests/`). Nessuno
riproduce 74 né 66.

**Questo non prova che i numeri siano sbagliati** — chi ha misurato può aver usato un criterio che non ho
indovinato. Prova qualcosa di più scomodo: che **chi legge non può rifare la misura**, e quindi la prossima
misura non sarà confrontabile con questa. Il delta fra due grep leggermente diversi è rumore che assomiglia
a un progresso.

Non è un rilievo esterno: **#1818 lo aveva già trovato da sé** nel commento del 2026-08-31 — *«l'audit
dichiara 19 `GetAllActorsOfClass`, le chiamate vere sono 11, le altre 8 sono occorrenze del nome in
commento»*. Lo stesso difetto, sulle metriche rimaste.

**Rimedio**: [`tools/architettura/misure-strutturali.py`](../../../tools/architettura/misure-strutturali.py)
fissa i pattern in un solo posto e sa confrontare due commit. Da qui in avanti «prima» e «dopo» misurano la
stessa cosa, e cambiare un pattern è un atto visibile nel diff.

---

## 3. Rimisura: `b45c314b` → `bbb20f16`

Prodotta da `python tools/architettura/misure-strutturali.py --base b45c314b --markdown`.

| Metrica | 2026-08-30 | 2026-09-03 | Δ |
|---|---|---|---|
| `ARTTurnManager` righe (3 file) | 10 412 | 11 754 | **+1 342** |
| `ARTTurnManager` righe di **codice** | 5 393 | 5 788 | **+395** |
| metodi `ARTTurnManager::` | 92 | 102 | **+10** |
| file di test | 153 | 187 | +34 |
| test unici | 1 421 | 1 775 | +354 |
| file di test che spawnano un Actor | 67 | 74 | +7 |
| **quota test con mondo** | 43,8 % | **39,6 %** | **−4,2** ✅ |
| file di test che dipendono dal TurnManager | 62 | 76 | +14 |
| quota test col TurnManager | 40,5 % | 40,6 % | +0,1 |
| funzioni ≥ 100 righe | 72 | **91** | **+19** |
| righe dentro quelle funzioni | 15 653 | 19 632 | **+3 979** |
| quota del codice di produzione | 32,1 % | 32,2 % | +0,1 |

### ✅ Il criterio dichiarato di E50 è rispettato

*«Quota di test che richiedono un mondo misurata prima e dopo, e in calo.»* È in calo: **43,8 % → 39,6 %**.
Non perché siano stati tolti mondi — i file che spawnano un Actor sono passati da 67 a 74 — ma perché **dei
34 file di test nuovi, 27 sono headless**. L'Epic sta vincendo sul proprio asse, e va scritto prima di tutto
il resto.

### 🔴 Ciò che nessun criterio dell'Epic sorveglia

`ARTTurnManager` è cresciuto di **+395 righe di codice** e **+10 metodi** mentre l'Epic che deve ridurlo era
aperta. Le tre fette chiuse nel periodo hanno un saldo proprio quasi nullo o positivo — #1817 ne sposta 17,
#1863 ne toglie 7 dal `.cpp` e ne aggiunge 10 all'header, #1907 ne aggiunge 92, come la stessa issue
registra con scrupolo. La crescita non viene dalle fette: viene dai **53 commit** di gameplay ordinario che
nel frattempo sono atterrati sulla stessa classe, perché è lì che vive il sequenziamento.

Non è un problema di disciplina: è che **una specifica che misura solo le fette non può accorgersi del
fondo che sale**. Manca il gate di non-regressione, non lo sforzo.

---

## 4. Le righe del file non sono le righe del codice

Il `+1 342` della prima riga della tabella è vero e fuorviante. Scomposto:

| Righe del diff `b45c314b..bbb20f16` sui tre file | Valore |
|---|---|
| aggiunte, totali | 1 616 |
| — di cui **commento** | **949 (58 %)** |
| — di cui vuote | 103 (6 %) |
| — di cui **codice** | **564 (34 %)** |
| rimosse, di cui codice | 274, di cui 169 |
| **saldo netto di codice** | **+395** |

**Il 58 % della crescita è documentazione.** In questo repository il commento accanto al codice cita le
`D-nnn` e porta la ragione della regola: è un bene. Ma la metrica «righe del file» sale allo stesso modo se
aggiungi una regola o se spieghi quella che c'è già — e una fetta che togliesse 100 righe di codice
aggiungendo 150 righe di spiegazione **peggiorerebbe il numero mentre migliora il codice**.

La metrica di ampiezza di E50 penalizza chi documenta. Va letta sulla riga «righe di codice», che lo
strumento riporta accanto a quella grezza.

---

## 5. L'asse che l'audit non aveva: la lunghezza delle funzioni

L'audit del 2026-08-30 ha misurato l'**isolamento** — zero `GetWorld`/`GetGameMode`/`GetSubsystem` nelle ~22
library di regole — e l'**ampiezza** per classe. Non ha misurato la **complessità interna**, e i due assi non
si implicano: una funzione può essere perfettamente isolata dal world e lunga 249 righe.
`URTHexBotLibrary::ScorePlan` lo è.

Su `bbb20f16`, **91 funzioni su 1 643 (5,5 %) superano le 100 righe e contengono 19 632 righe: il 32,2 % di
tutto il codice `.cpp` di produzione.**

| Contenitore | Fn ≥ 100 | Righe | Lettura |
|---|---:|---:|---|
| **Actor (`A*`)** | **31** | **8 710** | il principio #1 dice che la matematica non dovrebbe stare qui |
| Function Library (pura) | 28 | 4 709 | isolate dall'audit, **non** semplici |
| UObject/Asset (`U*`) | 12 | 2 107 | |
| funzione libera / ns anonimo | 11 | 2 032 | |
| struct/classe pura (`F*`) | 7 | 1 626 | |

Le dodici più lunghe, con la densità di rami accanto:

| Righe | Rami | Funzione | File:linea |
|---:|---:|---|---|
| 958 | 92 | `ARTTurnManager::PlanBots` | `Turn/RTTurnManager.cpp:646` |
| 888 | 64 | `ARTTurnManager::ResolveCombatPasses` | `Turn/RTTurnManager.cpp:4765` |
| 705 | 68 | `ARTHUD::DrawHUD` | `UI/RTHUD.cpp:408` |
| 589 | **0** | `URTCatalogLibrary::GetCoreActionCatalog` | `Ability/RTCatalogLibrary.cpp:1055` |
| 443 | 51 | `ParseScenarioTurns` | `ScenarioHarness/RTScenarioLoader.cpp:666` |
| 423 | 31 | `ARTTurnManager::ResolveDash` | `Turn/RTTurnManager.cpp:4101` |
| 420 | 55 | `URTHexMapAsset::ValidateMap` | `Map/RTHexMapAsset.cpp:512` |
| 410 | 31 | `ARTTurnManager::ApplyDisplacements` | `Turn/RTTurnManager_Blast.cpp:1667` |
| 408 | 39 | `ARTHexMapActor::RebuildInstances` | `Map/RTHexMapActor.cpp:1211` |
| 408 | 31 | `ARTTurnManager::LockInAndResolve` | `Turn/RTTurnManager.cpp:1662` |
| 402 | 35 | `FRTScenarioSession::Finish` | `ScenarioHarness/RTScenarioSession.cpp:1759` |
| 380 | 38 | `ARTTurnManager::ApplyInterrupts` | `Turn/RTTurnManager_Blast.cpp:769` |

### ⚠️ La lunghezza da sola produce falsi positivi

`URTCatalogLibrary::GetCoreActionCatalog` è il quarto imputato di qualunque classifica per righe. Ha **zero
rami in 589 righe**: è una tabella di dati con la ragione di ogni voce scritta accanto. Spezzarla non
toglierebbe complessità, sposterebbe dati — e promuoverla a data asset è una decisione di pipeline, non un
refactoring di struttura.

È l'unica voce a densità nulla della classifica, e questo è il motivo per cui la colonna `rami` esiste: **una
worklist costruita sulle sole righe conterrebbe lavoro che peggiora il codice.**

### La quota resta ferma, ed è il dato che conta

Le funzioni lunghe passano da 72 a 91 e le righe che contengono da 15 653 a 19 632, ma la **quota** sul
codice di produzione è identica: 32,1 % → 32,2 %. Il modulo non sta degenerando — sta crescendo
**riproducendo la propria struttura**. Il codice nuovo ha la stessa distribuzione di complessità del vecchio.

È una notizia peggiore di un peggioramento, perché un peggioramento si nota. Una quota costante significa che
**senza un gate il pattern si autoreplica**, e che le fette non scalfiscono il tasso con cui si riforma.

---

## 6. Cosa questo referto raccomanda

**Per E50 #1816** — aggiungere un criterio di non-regressione accanto a quelli esistenti. Nella forma:
*«ogni PR che tocca `RTTurnManager.*` riporta le righe di codice prima/dopo; una crescita non è un veto, ma
non può essere silenziosa.»* Oggi si può chiudere una fetta con successo mentre il fondo sale di 395 righe, e
nessun gate se ne accorge.

**Per #1818** — tre cose:
1. sostituire «righe» con «righe di codice» nelle misure prima/dopo, per non penalizzare la documentazione;
2. usare lo strumento per la rimisura, così i numeri di fette diverse sono confrontabili;
3. registrare che al 2026-09-03 la classe misura **11 754 righe / 5 788 di codice / 102 metodi**, contro le
   10 412 / 5 393 / 92 dell'audit.

**Per #1821** — **è sbloccata**. La tabella dell'Epic la dà ancora *«bloccata da #1817»*, e #1817 è chiusa.
La dipendenza dichiarata è caduta.

**Sull'ordine delle prossime fette** — l'incrocio fra debito e rete di test dice dove *non* cominciare:

| Area | Fn ≥ 100 | File di test che la nominano | Lettura |
|---|---:|---:|---|
| `Turn/` | 29 | 76 | debito massimo, **rete massima** → è qui che si lavora |
| `Map/` | 13 | 65 | rete densa |
| `ScenarioHarness/` | 18 | 23 (11 `Session` · 13 `Loader` · 17 `Runner`) | **rete sottile**: ispessire prima |
| `Player/` | 5 | 18 (per 2 415 righe e 73 metodi) | **rete sottile per la massa** |
| `Match/` | 2 su 4 funzioni totali | **2** | **rete quasi assente** |

`FRTMatchBootstrapper::Bootstrap` è 269 righe ed è nominato da **due** file di test. Il documento di
architettura lo descrive come ciò che *«rende l'allestimento verificabile senza sporcare lo stato globale»*:
è progettato per essere verificabile, e quasi nessuno lo verifica. Toccarlo oggi significa muoversi senza
rete.

---

## 7. Ciò che questo referto NON prova

- **nessun build e nessuna run di `rt-suite`**: qui non è stata cambiata una riga di codice di produzione, e
  misurare non è verificare. Chi userà queste misure per una fetta dovrà comunque pagare build e suite;
- **non prova che 74 e 66 siano sbagliati**: prova che non sono riproducibili da chi legge il referto;
- **`rami` non è complessità ciclomatica**: conta parole chiave, non cammini. Serve a separare le tabelle di
  dati dalle funzioni con logica, non a ordinare le seconde fra loro;
- **manca il churn**. Una funzione lunga che nessuno tocca non costa niente a nessuno; una di 120 righe
  attraversata da ogni feature costa a ogni feature. Il selettore vero è `righe × frequenza di modifica`, e
  la frequenza sta in `git log`, non in questo strumento. È il primo pezzo che manca a una worklist.

---

## Verdetto

**E50 sta funzionando sul proprio asse** — la quota di test che richiedono un mondo è scesa di 4,2 punti in
quattro giorni, ed è il criterio che l'Epic si è data.

Restano tre rilievi, nessuno dei quali mette in discussione la direzione:

1. le metriche non hanno il comando che le genera, quindi le rimisure non sono confrontabili — **risolto** da
   questo referto con uno strumento versionato;
2. la metrica di ampiezza conta il commento come debito, e il 58 % della crescita recente è commento;
3. non esiste un gate di non-regressione: il fondo può salire mentre ogni singola fetta riesce, e in questi
   quattro giorni è salito di 395 righe di codice e 10 metodi.

Il quarto punto non è un rilievo ma un'aggiunta: **c'è un asse di debito che nessuno stava misurando**, e su
quello la quota è ferma al 32 %. Non peggiora. Non migliora neanche.
