# Lo snapshot della posa della sagoma — spec panel (secondo criterio di #1750)

> `CURRENT` · **Referto di revisione**, non owner. Valuta il **secondo** criterio di accettazione di
> [#1750](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1750) — *«`FPoseSnapshot` catturato
> one-shot, PRIMA di `SetVisibility(false)`, dentro `RefreshComponentVisibility`»* — dopo che il **primo**
> (la posa di ripiego) è atterrato in `main` con la PR #2180.
>
> **Data**: 2026-09-05 · **Modo**: critique · **Focus**: requirements + architecture + testing
>
> **Cosa è**: la revisione di un pezzo di **presentazione** che tocca due confini insieme — quello con la
> simulazione (§2 del referto del 2026-08-30) e quello con la **privacy della conoscenza** (D-043, S4).
>
> **Cosa non è**: un'autorità. Se una riga qui diverge dall'owner
> ([`conoscenza-parziale-visibile-spec.md`](../../technical/systems/conoscenza-parziale-visibile-spec.md),
> il [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md)), **ha ragione l'owner**.
>
> **Predecessore**: [`sagoma-ultimo-contatto-posa-spec-panel-2026-08-30.md`](sagoma-ultimo-contatto-posa-spec-panel-2026-08-30.md).
> Questo referto **non lo sostituisce**: ne prende il §4 come mandato e misura ciò che quello aveva lasciato
> come proposta. Dove diverge, lo dichiara nel §5.

---

## 1. Da dove si riparte, misurato e non preso sulla parola

| Sonda | Esito |
|---|---|
| PR #2180 (primo criterio) | **MERGED** il 2026-09-03, `1b6d5c15` antenato di `origin/main` |
| Il ripiego, in `UpdateContactGhost` | `AnimationSingleNode` + `SetAnimation(Idle)` + `SetPosition(0)` + `Stop()` |
| `RefactorTactics.Unit.ContactGhostFallbackPointsAtTheIdleClip` | presente, `RTUnitTests.cpp:622` |
| `FindHeroSkeletal` esclude `ContactGhost` | ✅ invariato — l'esclusione per identità regge |
| `FRTLastKnownContact` | ✅ invariato — nessun campo di posa |

∴ **la T-pose non è più a schermo.** Ciò che manca è il ricordo *vero*: com'era l'unità quando la squadra
l'ha vista l'ultima volta, invece del primo frame del suo idle.

---

## 2. 🔴 CRITICO · Nygard — la premessa del referto precedente è **falsa col default corrente**, e il
> pericolo che ne discende è **peggiore**, non minore

Il referto del 2026-08-30 §3 motivava la cattura *prima* di `SetVisibility(false)` così:

> *«Quando l'unità smette di essere renderizzata UE può smettere di aggiornarne la posa
> (`VisibilityBasedAnimTickOption`). Catturare dopo restituirebbe una posa stantia o la posa di
> riferimento.»*

**Misurato sull'engine 5.8 installato**, `Engine/Source/Runtime/Engine/Private/Components/SkeletalMeshComponent.cpp:446`:

```cpp
VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
```

Il default **non** smette di aggiornare la posa: *«Always Tick and Refresh BoneTransforms whether rendered
or not»*. La posa di un'unità nascosta **continua ad avanzare**.

⚠️ La raccomandazione «cattura prima» **resta giusta**, ma per una ragione diversa da quella scritta: non
perché dopo si prenderebbe una posa ferma, ma perché il valore letto dipenderebbe da un'opzione di
componente che qualunque `BP_Unit_*` può cambiare senza toccare questo codice. Catturare prima è
**indipendente dall'opzione**; catturare dopo è corretto *finché nessuno la cambia*.

🔴 **E il vero pericolo cambia di segno.** Se la posa avanza anche da nascosta, una cattura **ripetuta**
non darebbe «sempre lo stesso valore innocuo»: darebbe la posa **corrente** dell'unità nemica, aggiornata
frame per frame. Non è un ricordo sbiadito, è **una telecamera**. Il terzo leak che il body della issue
teme non è teorico: è il comportamento di default se il one-shot non viene imposto esplicitamente.

**Raccomandazione**: la cattura si arma sulla **transizione** `visibile → nascosto`, non sulla condizione
`bRender == false`.

### 2b. E la condizione non basta, misurato sui chiamanti

`RefreshComponentVisibility` ha **quattro** chiamanti in `RTUnit.cpp`:

| Riga | Chiamante | `bRender` cambia? |
|---|---|---|
| 384 | `SetKnownToObserver` | **sì**, è il suo mestiere |
| 803 | `ApplyTeamColor` | no |
| 872 | `OnSelected` | no |
| 882 | `OnDeselected` | no |

Tre chiamanti su quattro la invocano a visibilità **invariata**. Un `if (!bRender) SnapshotPose(...)`
catturerebbe a ogni selezione e a ogni cambio colore di un'unità già nascosta — cioè la cattura continua di
cui sopra, per una strada che nessuno stava guardando.

∴ serve **stato**: l'ultimo `bRender` applicato, e la cattura solo quando passa da `true` a `false`.

---

## 3. 🔴 CRITICO · Fowler — la classe dedicata si può fare in C++ puro, e il repository lo ha già dimostrato

Il referto precedente (§3, MINORE) raccomandava *«una classe dedicata — che applica una posa e non avanza»*,
e il primo criterio se ne è scostato usando `AnimationSingleNode`, dichiarando: *«per lo SNAPSHOT servirà:
applicare un `FPoseSnapshot` richiede un `AnimInstance` che lo consumi»*.

La lettura ovvia di quella frase è *«serve un AnimBlueprint»*, cioè un `.uasset`. **È falsa qui**, e la
prova è nel repository:

`URTUnitAnimInstance` è *«il grafo di animazione dell'unità, in C++ e senza nessun `.uasset`»* — un
`FAnimInstanceProxy` che monta i propri nodi a mano, sul modello di `FAnimSequencerInstanceProxy`. Il
docstring dichiara anche il numero per cui quella strada fu scelta: **~2,8 MB** di AnimBP duplicati contro
gli **0,7 MB** che pesa tutto `Content/` versionato.

E il nodo che serve esiste già, in un modulo **già dipendenza** (`AnimGraphRuntime`, `Build.cs:43`):

```cpp
// Engine/Source/Runtime/AnimGraphRuntime/Public/AnimNodes/AnimNode_PoseSnapshot.h
enum class ESnapshotSourceMode : uint8 { NamedSnapshot, SnapshotPin };
struct FAnimNode_PoseSnapshot : public FAnimNode_Base { FPoseSnapshot Snapshot; ... };
```

`SnapshotPin` prende la posa da una **variabile**, non dalla cache interna dell'AnimInstance: è
esattamente la forma che serve a un grafo montato in C++.

**Raccomandazione**: `URTContactGhostAnimInstance`, C++ puro, proxy con **un solo** nodo
`FAnimNode_PoseSnapshot`. ⛔ Nessun sequence player nel grafo: la sagoma non può animarsi **per costruzione**,
non per un `Stop()` che qualcuno potrebbe togliere. È una garanzia strutturale, ed è più forte di quella che
il ripiego ha oggi.

⚠️ **`FindHeroSkeletal` continua a escludere `ContactGhost`**: la classe si assegna esplicitamente in
`UpdateContactGhost`, mai da `ApplyUnitAnimClass`. Se ci arrivasse da lì prenderebbe `URTUnitAnimInstance` e
si animerebbe dal vivo — il leak che l'esclusione esiste per impedire.

---

## 4. 🔴 CRITICO · Adzic — lo snapshot eredita il difetto di #1784, sullo stesso eroe

`USkeletalMeshComponent::SnapshotPose` non copia tutte le ossa allo stesso modo
(`SkeletalMeshComponent.cpp`, corpo della funzione):

```cpp
const bool bBoneHasEvaluated = FillComponentSpaceTransformsRequiredBones.IsValidIndex(CurrentRequiredBone)
    && ComponentSpaceIdx == FillComponentSpaceTransformsRequiredBones[CurrentRequiredBone];
...
Snapshot.LocalTransforms[ComponentSpaceIdx] = bBoneHasEvaluated
    ? ChildTransform.GetRelativeTransform(ParentTransform)
    : RefPoseSpaceBaseTMs[ComponentSpaceIdx];      // ← posa di RIFERIMENTO
```

**Un osso fuori dalle *required bones* del LOD corrente entra nello snapshot in posa di riferimento.**

🔴 E le ossa che le LOD dei pack Paragon rimuovono sono **le catene di Riktor**: è la misura di
[#1784](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1784), *«le unità restano a LOD 0,
perché le LOD dei pack rimuovono le ossa delle catene»*.

∴ **su un'unità non a LOD 0, lo snapshot rimetterebbe a schermo esattamente le catene distese** — lo stesso
fotogramma che ha aperto questa issue, arrivato per un'altra strada e questa volta *dentro* la correzione.

**Raccomandazione**: dichiararlo nel codice, accanto alla cattura, e **nominare #1784 come la ragione per
cui oggi non morde**. Non è una dipendenza da aggiungere — il fix di #1784 è già in `main` — è un
accoppiamento che va reso cercabile, perché chi un giorno riabilitasse le LOD sulle unità troverebbe il
difetto senza capire da dove viene.

---

## 5. ⚠️ MAGGIORE · Cockburn — un ricordo per attore, e la seconda incoerenza che nessuno aveva nominato

**D-043**: la conoscenza è di **squadra**. Uno snapshot su `ARTUnit` ha **una** casella. Il referto
precedente lo aveva già dichiarato: *«due squadre che avessero perso di vista la stessa unità in momenti
diversi condividerebbero un ricordo solo»* — latente in v0.1, dove il visore è uno.

Questo referto ne aggiunge **una seconda**, che discende dal §2b e non era stata vista:

> `bRender` è calcolato da `ShouldBeRendered(IsAlive(), bKnownToObserver)`, e `bKnownToObserver` è **la
> conoscenza dell'osservatore locale**. La cattura si arma quando *quell'* osservatore perde di vista
> l'unità. Con due visori, la posa verrebbe catturata sulla transizione del **primo** che perde il contatto,
> e mostrata anche al secondo.

∴ le due incoerenze sono la **stessa**, e hanno la stessa data di scadenza: il secondo osservatore locale.
Vanno dichiarate insieme in **una** riga cercabile, non in due punti che invecchieranno separatamente.

---

## 6. ⚠️ MAGGIORE · Wiegers + Crispin — il criterio del referto precedente resta non eseguibile, e va sostituito

Il §3 del referto del 2026-08-30 proponeva come oracolo headless:

> *«almeno un osso della sagoma differisce dalla posa di riferimento dello skeleton»*

La PR #2180 lo ha **misurato non eseguibile** e lo ha scritto nel test: la skeletal la aggiunge il
Blueprint e non il C++, e le clip non esistono su un checkout senza i pack (`Content/FabAsset/` è
gitignorato). Quel criterio resta **verifica PIE**, ed è giusto che ci resti.

⚠️ Ma **rinunciare all'oracolo automatico non è obbligatorio**, e qui c'è la differenza rispetto al primo
criterio: `FPoseSnapshot` è una struttura **di dati puri** —

```cpp
TArray<FTransform> LocalTransforms; TArray<FName> BoneNames;
FName SkeletalMeshName; FName SnapshotName; bool bIsValid;
```

— costruibile a mano in un test, senza mesh, senza pack, senza Blueprint.

**Criteri proposti, tutti headless**, e ciascuno cade su una mutazione dell'implementazione:

| # | Dato | Quando | Allora | La mutazione che lo fa cadere |
|---|---|---|---|---|
| C1 | l'unità è visibile | passa a nascosta | la cattura si **arma** | invertire il verso della transizione |
| C2 | l'unità è **già** nascosta | `RefreshComponentVisibility` di nuovo (selezione, colore) | la cattura **non** si riarma | condizione su `!bRender` invece che sulla transizione |
| C3 | l'unità è nascosta | torna visibile | la cattura non si arma | armare su ogni cambio invece che su un verso |
| C4 | uno snapshot valido esiste | si mostra la sagoma | sorgente = **snapshot** | preferire sempre il ripiego |
| C5 | nessuno snapshot (contatto da **rumore**) | si mostra la sagoma | sorgente = **ripiego** | far dipendere la sagoma dallo snapshot |
| C6 | uno snapshot `bIsValid == false` | si mostra la sagoma | sorgente = **ripiego**, non uno snapshot vuoto | dimenticare `bIsValid` |

🔑 **C2 è il test della privacy**, ed è quello che questa issue esiste per scrivere: separa «ricordo» da
«telecamera». C5 è il caso di `CP 13.4` che il referto precedente aveva individuato e che nessun test
copriva ancora.

⛔ **Ciò che questi test NON dimostrano**: che la posa a schermo sia quella giusta. Quello resta PIE su
Riktor, ed è già registrato come tale.

---

## 7. 🔧 MINORE · Hohpe — il ricordo sopravvive al soggetto, e va detto quanto

Tre domande che l'implementazione deve rispondere per iscritto, perché nessuna ha una risposta ovvia:

1. **L'unità muore** → `ShouldBeRendered` dà `false` per `!IsAlive()`, quindi la transizione si arma e si
   cattura la posa del morto. Innocuo (la sagoma segue la conoscenza, non la vita) ma **non gratuito**: va
   dichiarato che è voluto.
2. **La mesh cambia** → `FAnimNode_PoseSnapshot` porta il proprio bone mapping fra `SkeletalMeshName`
   sorgente e target, quindi non è un crash. Nessuna azione, ma la ragione va nominata.
3. **L'unità torna in vista e la si riperde** → la casella si sovrascrive. È il comportamento voluto: il
   ricordo è *l'ultimo* contatto, non il primo.

⚠️ **Una posa vista, mostrata su una cella sentita.** Se un'unità viene prima **vista** e poi solo
**sentita** (`CP 13.4`), la sagoma si sposta sulla cella del rumore portandosi la posa dell'ultimo
avvistamento. È coerente — la posa è memoria come la cella — ma è l'unico punto dove i due ricordi hanno età
diverse. Non introduce leak (la posa non rivela nulla di tattico) e non giustifica di buttare lo snapshot.
**Da dichiarare, non da correggere.**

---

## 8. Verdetto

**Il secondo criterio è implementabile per intero in C++, senza nessun `.uasset`, e con un oracolo
automatico più forte di quello che il primo criterio aveva potuto costruire.**

Ordine dei pezzi:

1. `URTContactGhostAnimInstance` + proxy con il solo nodo `PoseSnapshot` — la garanzia strutturale del §3;
2. la cattura sulla **transizione**, con lo stato che la rende one-shot — §2 e §2b;
3. la scelta della sorgente (snapshot ↔ ripiego) come **funzione pura**, perché sia quella che i test
   chiamano — la lezione di mutazione della PR #2180, che un test scritto sui dati invece che sul codice
   resta verde su un fix mutato;
4. le tre dichiarazioni cercabili: LOD/#1784 (§4), un-ricordo-per-attore (§5), i tre casi limite (§7).

⛔ **I vincoli del referto precedente restano tutti**: `FRTLastKnownContact` invariato, `FindHeroSkeletal`
continua a escludere `ContactGhost`, il ripiego **non si toglie** — è il caso del contatto da rumore.

### Scostamenti dichiarati rispetto al referto del 2026-08-30

| # | Quello diceva | Questo misura |
|---|---|---|
| 1 | «UE può smettere di aggiornare la posa» | il default è `AlwaysTickPoseAndRefreshBones`: **non** smette. La raccomandazione regge, la ragione cambia (§2) |
| 2 | «applicare una posa richiede *qualche* `AnimInstance`» — letto come «serve un AnimBP» | si fa in C++ puro: il repository ha già quel pattern (§3) |
| 3 | il criterio «almeno un osso differisce» come oracolo headless | non eseguibile (misurato dalla PR #2180); **sostituito** da sei criteri sui dati puri (§6) |

---

## 9. Cosa non è stato fatto, e perché

- **Nessuna riga di codice al momento della scrittura.** Il panel è documentale (CLAUDE.md §6): questo
  referto precede il diff e non lo autorizza da solo.
- **Nessun `D-nnn`.** Il panel misura, non decide. Il vincolo *«la posa non entra nello stato di gioco»*
  resta materiale da Decision Log, come già osservava il §5 del referto precedente.
- **Suite non eseguita** al momento della scrittura di questo referto: nessuna riga toccata, nessuna misura
  da produrre (**D-222**). La misura arriva col diff.
