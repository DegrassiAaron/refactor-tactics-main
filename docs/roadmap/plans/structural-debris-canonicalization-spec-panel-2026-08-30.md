# Detriti strutturali e Rubble — canonizzazione dell'owner, spec panel sul consolidamento

> `CURRENT` · **Stato**: canonizzazione applicata. Nessun runtime toccato, nessuna misura di suite eseguita ·
> **Data**: 2026-08-30
> **HEAD della revisione**: `d446a051` (= `origin/main` al 2026-08-30 dopo `git fetch --prune`).
> ⚠️ `origin/main` si è mosso **durante** l'audit: la prima misura è stata presa su `86c75a29` e riverificata
> su `d446a051` dopo il merge della PR #1846, che tocca due asset e `roadmap-v0.1.md` e **non** tocca
> `Source/`. I fatti misurati reggono entrambi gli HEAD.
> **Oggetto**: il documento Drive *RefactorTactics — Structural Debris & Rubble — Canonical Consolidation*
> letto **come proposta di canonizzazione**, non come fonte. Il gameplay Rubble non è stato implementato.
> **Panel**: Wiegers (lead) · Fowler · Nygard · Cockburn · Adzic · Crispin
> **Modo**: critique · **Focus**: requirements, architecture, testing

---

## 1. Il verdetto in una riga

Il gap che il consolidamento registrava **è reale e ancora aperto**: `#1132` esisteva senza un parent, e la
roadmap post-v0.1 classificava la distruzione come `future` senza owner. La canonizzazione è stata applicata.

Ma **quattro fatti mutabili del consolidamento erano cambiati o falsi**, e tre di questi cambiano il lavoro
che le nuove issue devono fare. Sono la parte di questo referto che vale più della decomposizione.

| | Voci |
|---|---:|
| 🔴 Contraddizione che cambia il lavoro | **3** |
| 🟡 Contraddizione di classificazione | **1** |

---

## 2. La gerarchia di autorità applicata

```
origin/main + codice/test as-built
  → Decision Log / ADR / owner specs
  → roadmap canoniche del repository
  → GitHub Epic / Issue / PR live
  → Google Drive domain roadmaps
  → Structural Debris & Rubble Canonical Consolidation
```

Il consolidamento è un handoff completo e ben scritto. **Non è un owner.** Dove ha divergito dal repository
misurato, ha perso — e il documento Drive è stato aggiornato, non il repository.

---

## 3. Il gap, misurato

Su `d446a051`:

| Termine | Occorrenze in `Source/RefactorTactics/` |
|---|--:|
| `StructuralDebrisYield` | **0** |
| `DebrisBudget` | **0** |
| `DebrisState` | **0** |
| `BlockedDebris` | **0** |
| `StopAtHeavy` | **0** |
| `SpreadProfile` | **0** |
| `CollapseProfile` | **0** |

Le uniche «macerie» nel runtime sono una **metafora in un commento** su archi `Destroyed`
(`Turn/RTTurnManager_Blast.cpp:1452`, `Tests/RTEnvironmentActionTests.cpp:834-838`): descrivono la prova che
resta dopo un abbattimento, non un sistema di detriti.

Su GitHub, `debris` · `rubble` · `detriti` · `macerie` · `crollo` · `collapse` restituivano **una sola** issue
di dominio: `#1132`. Nessuna Epic. **Il gap era ancora aperto.**

---

## 4. 🔴 Le quattro contraddizioni fra consolidamento e repository

### 4.1 `#1733` è mal diagnosticata, e la revisione era già stata fatta

Il consolidamento assegna a `#1733` l'invariante *«una `FRTCellId` non può terminare con due unità vive»* e
istruisce a non duplicarne il fix.

**Misura**: il branch remoto `docs/1733-spec-panel` — revisione chiusa, non mergiata al 2026-08-30 — misura
che l'invariante **è già applicata** in `Turn/RTMovementActionLibrary.cpp:116-143`, pinnata da due test
d'integrazione (`Tests/RTHexMatchIntegrationTests.cpp:125` e `:343`), e che **nessuna delle tre occorrenze**
citate da `#1733` come prova è una sovrapposizione. Il difetto residuo è di **attribuzione nel TurnLog**.

**Conseguenza**: `CP 51.1` non aspetta un fix di `#1733` e non lo duplica. Deve solo dimostrare che
`BlockedDebris` non apre un'eccezione a un'invariante **già applicata**. Il consolidamento chiedeva di
collegarsi a un fix che, per la parte che riguarda i detriti, non è pendente.

### 4.2 `ERTHexArcState::Destroyed` è dichiarato TERMINALE

Il consolidamento §6 afferma che *«Clear/Demolish può riaprire un edge quando lo stato torna legale»*.

**Misura**: `Map/RTHexCellData.h:389` dichiara tre valori e ne dichiara la ragione —

> *«`Inactive` e `Destroyed` sono indistinguibili per il grafo — da nessuno dei due si passa — e differiscono
> per la REVERSIBILITÀ: un ponte disattivato si riattiva, uno distrutto no.»*

**Le due cose non stanno insieme.** Se `Clear Rubble` riapre un arco `Destroyed`, la distinzione da `Inactive`
— l'unica ragione per cui entrambi i valori esistono — sparisce. E il valore è **serializzato**: l'indice è il
dato.

**Conseguenza**: `CP 51.4` deve scegliere fra tre opzioni con costi diversi, e la scelta è **di regola**, non
di implementazione. È registrata nell'issue con la tabella dei costi. Non è stata decisa qui.

### 4.3 Esiste già un modello graduato di «quanto è bloccata una cella»

Il consolidamento propone la scala `Clear → DebrisField → Rubble → HeavyRubble → BlockedDebris` senza
misurare cosa il repository possiede già.

**Misura**: `ERTCellOccupancy{Free, Constrained, Blocked}` (`Map/RTHexOccupancyLibrary.h:19`) è **cotta dalla
geometria** su dodici settori più il centro, con `FRTOccupancyThresholds{ConstrainedFrom, BlockedFrom,
ConstrainedSurcharge}`. Ha un consumatore reale — il sovrapprezzo di attraversamento — e le sue soglie
**entrano nell'hash di stato partita**, ragione per cui vivono nel modulo runtime e non nel tool d'editor.
È arrivata con `#619` e `#621`, sotto `E23`.

**Conseguenza**: il rischio architetturale principale della Epic non è «un secondo Environment manager» — che
il consolidamento già vieta — ma **un secondo modello di blocco**. `CP 51.1` ha come primo lavoro misurare
*come* la scala dei detriti si innesta su `ERTCellOccupancy`, non decidere *che* si innesti.

### 4.4 🟡 La classificazione di release era `future`, non «nessun owner»

**Misura**: `roadmap-post-v0.1.md` classificava `Destruction / debris` come release canonica **`future`**,
azione `DEFER`, owner *«nessuno — `RT-FEAT-MAP-STRUCTURAL` è `IDEA`»*, contro una proposta del Graybox Kit di
**v0.6** — non «nessuna release» in astratto.

**Decisione**: la riga è stata sostituita con l'owner reale (**E51**, **v0.2**), e la tabella dei modi di
divergenza ha guadagnato un **quinto modo** — *«ha una release e un owner, ma il lavoro non è ancora
aperto»* — invece di spostare la distruzione nel primo, che sarebbe stato falso in senso opposto: il
repository **non** la sta costruendo. Il totale resta nove; è il modo a essere cambiato.

La provenance è conservata: la nota sotto la tabella registra che fino al 2026-08-30 quel gruppo contava tre.

---

## 5. Matrice search-before-create

Nessun duplicato creato. `REUSE` dove un owner esisteva.

| Concetto | Owner esistente | Runtime as-built | Azione |
|---|---|---|---|
| Minimal Debris producer/consumer | `#1132` (`E8` `#22` chiusa) | — | **REUSE** + nota di parent |
| Tassonomia degli stati | — | `ERTHexSurface` (9 valori, nessuno è detrito) | CREATE → `CP 51.1` |
| Occupied-cell · `BlockedDebris` | — (`#1733` adiacente) | invariante già applicata | CREATE → `CP 51.1` |
| Forced displacement | **`#541`** (chiusa) | `ARTTurnManager::ApplyForcedDisplacement`, `Turn/RTTurnManager.h:1218` | **REUSE, non duplicare** |
| Destinazione dello spostamento | `#541` | `URTHexCombatLibrary::HexKnockbackDestination`, `Combat/RTHexCombatLibrary.h:400` | REUSE |
| `StructuralDebrisYield` · `DebrisBudget` | — | 0 | CREATE → `CP 51.3` |
| `CollapseProfile` · `SpreadProfile` · accumulo · overflow | — | 0 | CREATE → `CP 51.3`, tassonomie **OPEN** |
| Cell debris | `E8` · `E23` | `ERTHexSurface`, `ERTCellOccupancy` | CREATE → `CP 51.4`, **sul modello esistente** |
| Edge debris | `E23` `#324` | `FRTHexEdge` + `ERTHexArcState` | CREATE → `CP 51.4` + §4.2 |
| `GraphRevision` | Map/Turn | `CurrentGraphRevision()` `RTTurnManager.h:1634`; campo TurnLog v6 `RTTurnLog.h:738` | REUSE |
| Clear Rubble | — | 0 | CREATE → `CP 51.5` |
| TurnLog provenance | Turn (`#307`, chiusa) | `ERTLogCategory::Environment`, `RTTurnLog.h:18` | REUSE |
| `StateHash` · determinismo | `E12` `#26` | `RTMatchStateHash` | CREATE → `CP 51.6` (riusa) |
| `MaterialProfile` · `Dust` · `Wind` · Flying Debris | — | 0 | **DEFER** |
| Rumore e privacy | `E13` `#151`, `#159` | `URTIntentPrivacyLibrary::FilterForTeam` | **DEFER** |
| Valutazione bot · telemetria · hardening rete | `E26` `#326` e altri | — | **DEFER** |

---

## 6. Cosa è diventato canonico

| | |
|---|---|
| **Epic** | `E51` · [#1848](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1848) — *Detriti strutturali, crolli e Rubble* |
| **Release** | v0.2 · milestone `v0.2 · Struttura e finestre` |
| **Label** | `epic` · `P3` · `post-v0.1` |
| **Dominio primario** | Environment Systems & Gameplay Effects |
| **Cross-domain** | Tactical Map · QA/Replay · Abilities · Perception/TeamKnowledge · Bot |
| **Owner documentale** | [`../roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) § E51 |

**Numerazione**: `E51` è il primo libero. `E50` (`#1816`) era il massimo; `E51` non compariva su `origin/main`,
né in alcun ref remoto, né in issue o PR — verificato prima di assegnarlo, come `AGENTS.md` §12 richiede.

**Decomposizione** — sei checkpoint, uno riusato:

| CP | Issue | Owner |
|---|---|---|
| 51.1 | [#1849](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1849) | stati discreti, cella occupata, `BlockedDebris`, destinazione deterministica |
| **51.2** | [#1132](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1132) (**esistente**) | il primo slice producer/consumer — **non gonfiata** |
| 51.3 | [#1850](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1850) | yield, budget, profili, accumulo, overflow |
| 51.4 | [#1851](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1851) | celle e archi, traversal, topologia, `GraphRevision` |
| 51.5 | [#1852](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1852) | `Clear Rubble` esplicito |
| 51.6 | [#1853](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1853) | determinismo, `StateHash`, TurnLog, scenari |

> ⚠️ **I titoli seguono la convenzione `CP NN.M ·` del repository**, non la forma `[DESIGN/CORE]` proposta dal
> consolidamento. Nessuna issue di questo repository usa il prefisso di dominio fra parentesi quadre;
> introdurlo avrebbe creato un terzo schema di nomi accanto a `CP` e `[EPIC vX.Y]`. Il dominio è dichiarato
> **nel corpo** di ogni issue.

---

## 7. `StopAtHeavy + overflow` — working policy, non canone

È **la sola aggiunta nuova** del consolidamento alla precedente decisione occupied-cell, che era `OPEN`.

```
cella occupata che dovrebbe diventare BlockedDebris
            ↓
   CollapseImpact, se applicabile
            ↓
   ricerca deterministica di una destinazione legale
            ↓
   ┌─────── esiste? ───────┐
  SÌ                       NO
   ↓                        ↓
riusa #541             StopAtHeavy + overflow
```

Significa tre cose insieme: la cella **resta** `HeavyRubble`; il Debris eccedente **non sparisce**;
l'eccedenza **rientra** nella distribuzione deterministica. Nessun teleport.

⚠️ **Non è stata canonizzata qui.** È registrata in `#1849` come *working policy da canonizzare*, con
provenance esplicita, perché è `CP 51.1` a doverla accogliere o respingere **dopo** l'audit dei servizi
runtime reali. Trasformarla in canone in questa sessione sarebbe stato esattamente l'errore che la gerarchia
di autorità esiste per impedire.

---

## 8. Cosa resta OPEN

Non canonizzato, e **da non improvvisare nel codice**:

nomi esatti degli stati · soglie · unità dello yield · tassonomia `SpreadProfile` · tassonomia
`CollapseProfile` · tassonomia `MaterialProfile` · ordinamento e capacità esatti dell'overflow oltre la
baseline · ranking esatto della destinazione dopo i vincoli di legalità · casi d'uso di `Crush` ·
worker base vs Auxiliary per il `Clear` · `Clear` multi-worker · `Demolish`/`HeavyClear` · detriti verticali ·
crollo a catena · promozione di `Dust` · modificatori di rumore · `FortifiedRubble` come stato o modificatore ·
prima skill che consuma detriti · prima reazione materiale · scatter da vento/forza ·
**se l'ostruzione d'arco sia uno stato o una proprietà** (§4.2).

---

## 9. Corpus scenari preservato nel backlog

`Oggetto piccolo` · `Stesso oggetto / effetto diverso` · `Accumulo` · `Clear Rubble` (con interruzione che
preserva il progresso) · `Cella occupata` · `Edge blocking` · `Consume debris` · `Determinismo`.

⚠️ **`Cella occupata` conta due casi, non uno**: una destinazione legale → displacement con la primitiva
esistente; **zero** destinazioni legali → `StopAtHeavy + overflow`. Il secondo è quello che la baseline
esiste per definire, ed è il primo a essere dimenticato.

Restano futuri: reazione materiale · rumore e privacy.

---

## 10. Cosa questa sessione NON ha fatto

- **Nessun runtime.** `Source/` e `Content/` invariati.
- **Nessuna suite eseguita.** Nessun claim di verde su test Unreal: **NOT RUN**.
- **Nessun Feature Registry.** `D-181` lo ha rimosso; gli `RT-FEAT-*` citati restano provenance storica.
- **Nessun `D-nnn` nuovo.** `StopAtHeavy + overflow` vive nell'issue, non nel Decision Log, finché `CP 51.1`
  non lo canonizza.
- **Nessuna modifica** a `roadmap-v0.1.md`, `roadmap-checkpoint.md`, `v0.1-definition-of-done.md`.
- **Nessuna issue** per `MaterialProfile`, `Dust`, `Wind`, Chaos gameplay, valutazione bot, telemetria o VFX.
- **`E23` non è stata resa parent.** Resta related/cross-domain, come il consolidamento richiede.
- **`E8` non è stata riaperta.**

---

## 11. Incoerenze preesistenti non toccate

Dichiarate perché non vengano scoperte dopo, e **non** corrette qui: sarebbero state espansione di scope.

- La tabella *«Quattro cluster il kit li mette dopo»* di `roadmap-post-v0.1.md` elenca lo **stato delle
  feature** dei quattro cluster che il repository possiede. `Destruction` non vi è stata aggiunta: non ha
  feature né runtime, e inventarne uno stato sarebbe stato un numero falso. Il conteggio «quattro» di quella
  tabella resta corretto per ciò che misura.
- `E48`, `E49` ed `E50` non compaiono nella tabella di `roadmap-v0.1-v1.0.md` §2 perché **non hanno
  milestone**: non sono assegnate a una release. Non è un difetto di quella vista.
