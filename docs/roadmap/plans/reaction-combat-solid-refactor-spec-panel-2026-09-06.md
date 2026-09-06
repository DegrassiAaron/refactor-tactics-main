# `reaction-combat-solid-refactor` — le quattro wave misurate contro il codice

> `CURRENT` · **Stato**: revisione conclusa, nessuna wave ancora eseguita · **Data**: 2026-09-06
> **HEAD della misura**: `origin/main` = `9da33c59`, albero pulito, nessuna modifica preesistente.
> **Oggetto**: verificare premessa per premessa il work order DEV-LEAD del programma
> `reaction-combat-solid-refactor` — quattro wave sequenziali su `FRTReactionPassResult`,
> `DeflectDelta`, `GuardFirstHitReduction` e `ApplyFirstHitDelta`.
> 🔑 **Nessun numero qui è ricordato.** Ogni conteggio porta in §2 il selettore che l'ha prodotto.
> ⛔ **La WAVE 4 non è in questo documento**: la sua premessa era falsa e la wave è stata riscritta come
> work order proprio — [`waves/first-hit-provenance/WORK-ORDER.md`](../../rt-three-terminals/waves/first-hit-provenance/WORK-ORDER.md).
> Qui restano le wave 1-3 e le osservazioni di processo.

---

## 1. Il verdetto in una riga

> **Tre wave su quattro sono eseguibili; la quarta partiva da una premessa che il codice smentisce.** E le
> tre eseguibili hanno tutte lo stesso difetto di forma: propongono *ex novo* una soluzione che il
> repository ha già istituito venti righe più in là.

| wave | oggetto | verdetto | correzione minima |
|---|---|---|---|
| 1 | array paralleli del Counter | ✅ eseguibile | citare il precedente in-file; classificare la seconda coppia parallela |
| 2 | `DeflectDelta` → reduction pool | ✅ eseguibile, la più pulita | ancorare il nome al simbolo che già esiste |
| 3 | `GuardFirstHitReduction` → pool | ⚠️ eseguibile con correzione | recepire la clausola scritta nel codice; write-set reale ≈13 file |
| 4 | provenienza del primo colpo | 🔴 riscritta | vedi il work order dedicato |

---

## 2. Come è stata misurata

Ogni conteggio di questo documento viene da uno di questi selettori.

| cosa | selettore |
|---|---|
| occorrenze di un simbolo | `grep -rn --include=*.h --include=*.cpp -c <sym> Source/` |
| issue | `gh issue list --state all --limit 3000 --json number,title,state,body`, poi ricerca **in locale** sui corpi |
| produttori di uno stadio | `grep -rn "ERTDamageStage::" Source/ --include=*.cpp --include=*.h`, escluso `/Tests/` |
| esposizione Blueprint | presenza di `UFUNCTION`/`UPROPERTY` sulla dichiarazione |

⚠️ **La ricerca issue non usa `gh issue list --search`**: quel filtro legge un indice asincrono e su issue
recenti risponde vuoto. I corpi sono stati scaricati e cercati in locale — 981 issue, 376 `OPEN` e 605
`CLOSED`.

⛔ **Un selettore è stato scartato perché non misura**: la ricerca dei simboli nella name table dei
`.uasset` (`grep -rl --binary-files=text ... Content/`). Calibrata con cinque simboli certamente usati —
`ResolveAttacks`, `EffectiveAttackPower`, `ApplyAbsorptionPool`, `ApplyDamageDelta`, `RTCombatResolver` —
ha risposto **zero su tutti e cinque**: i package sono compressi. Ogni affermazione sull'uso Blueprint di un
simbolo in questo documento è quindi `NOT MEASURED`, non «nessuno lo usa».

---

## 3. WAVE 1 — il precedente sta venti righe sopra

**Premessa del work order**: verificata e corretta. `FRTReactionPassResult` rappresenta un Counter con sei
array paralleli — `CounterAttacks`, `CounterAttackSrc`, `CounterActionId`, `CounterBaseActionId`,
`CounterPriority`, `CounterAttackActors` (`Source/RefactorTactics/Turn/RTReactionPassResult.h:48-58`).

I guardrail reggono, misurati:

- `FRTReactionPassResult` **non è** una `USTRUCT` (`:42`), quindi «niente `USTRUCT`» è realizzabile;
- `TArray<ARTUnit*>` grezzo è **già** lo status quo (`:58`): il refactor non introduce un rischio GC nuovo.

Write-set reale: **3 file** — l'header, `RTTurnManager.cpp` (15 riferimenti), `RTReactionTests.cpp` (1). È
la wave più piccola e più netta delle quattro.

### 3.1 ⚠️ Il precedente che il work order non cita

Nello stesso file, `:19-27`, vive già `FRTDisplacementCause` — una struct coesa **nata dallo stesso
difetto**, e il commento lo racconta (`:13-17`): due `TMap` parallele condivise fra spinta e trazione, la
seconda `Add` sovrascriveva la prima, *«una voce finiva per dichiarare l'attaccante sbagliato. Trovato in
code review»*.

E `:33-36` **documenta già** che gli array del Counter sono paralleli e che disallinearli attribuirebbe un
contrattacco all'unità sbagliata — *«la stessa classe di difetto già costata una correzione a
`FRTDisplacementCause` qui sopra»*.

🔑 Il work order propone `FRTCounterAttackResult` come forma nuova. La forma **esiste già in casa**, con un
nome che dichiara la relazione (`…Cause`) invece dell'esito. Il nome finale è una scelta della wave, ma va
fatta citando il precedente, non ignorandolo: è `SEARCH → REUSE` applicato allo stesso file.

### 3.2 La seconda coppia parallela, che nessuna wave raccoglie

`:89-90`:

```cpp
TArray<int32> HazardFlees;
TArray<int32> HazardFleeDistance;   // "parallelo con quante celle"
```

Un solo record, due array, identico modo di rottura, e il commento lo dichiara. Il guardrail del work order
— *«nessun array parallelo residuo **per lo stesso record**»* — la esclude per costruzione.

O si allarga lo scope, o la si dichiara `DEFERRED` con la ragione. Lasciarla implicita significa chiudere la
wave dicendo «gli array paralleli non ci sono più» in un file dove ci sono ancora.

---

## 4. WAVE 2 — il nome bersaglio è già metà istituito

**Premessa del work order**: verificata. `DeflectDelta` (`RTReactionPassResult.h:45`) è semanticamente
obsoleto dopo `D-309`, che ha reso la riduzione un absorption pool.

**È la wave più pulita del programma, e il work order lo sottovaluta.** `DeflectDelta` è **puramente
interno**: 6 occorrenze di codice (`RTReactionPassResult.h`, `RTTurnManager.cpp` ×4,
`RTTurnManager_Blast.cpp`) più una menzione in `RTCombatLibrary.h` e una nel Decision Log. Nessuna chiave di
dato, nessun `UFUNCTION`, nessuno scenario.

### 4.1 Il nome non è una proposta: è un allineamento

Il work order presenta `ReactionReductionPoolByTarget` come nome candidato da giustificare. Ma
`RTCombatLibrary.h:204` dichiara già:

```cpp
static const FName ReactionReductionPoolSource;
```

introdotto da [#2213](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2213), accanto a
`GuardPoolSource`. Il vocabolario `ReactionReductionPool*` **esiste**, con owner e ragione scritti: il pool
si costruisce per qualunque reazione dichiari `ERTActionEffect::DamageReduction`, e *«qui non si guarda mai
l'`ActionId`»* — è ciò che permette a `Hero.Wraith.Deflection` di riusare la semantica di `Action.Deflect`.

🔑 Citare quel simbolo trasforma la scelta del nome da questione di gusto in coerenza verificabile. È anche
l'argomento che chiude in anticipo la clausola *«accetta un nome diverso solo se l'audit dimostra…»*.

---

## 5. WAVE 3 — il codice ha già deciso, e pone una condizione

**Premessa del work order**: verificata. `GuardFirstHitReduction` descrive la semantica pre-`D-292`, e il
valore lo consuma `ApplyAbsorptionPool`, non `ApplyFirstHitDelta` (`RTCombatLibrary.h:98`).

### 5.1 ⚠️ La clausola che il work order non recepisce

`RTCombatLibrary.h:94-96`:

> ⚠️ **Il NOME resta `GuardFirstHitReduction`, e non è una svista.** Rinominarlo tocca i chiamanti ed è un
> refactor, non una correzione di prosa: finché il nome vive, questo commento è l'unico posto che dice cosa
> il valore fa davvero. **Chi lo rinomina porti via anche questo paragrafo.**

Non è un divieto: è un rinvio consapevole **con una condizione di consegna**. Va negli acceptance criteria
della wave, altrimenti il rename cancella l'unico punto che spiega la semantica — e il difetto che la wave
voleva chiudere si sposta invece di sparire.

### 5.2 La misura che il work order chiede, eseguita

| canale | esito | evidenza |
|---|---|---|
| Blueprint API | **no** | è `static constexpr int32` (`:105`), nessun `UFUNCTION`/`UPROPERTY` |
| serialization / save / replay / hash | **no** | nessuna occorrenza fuori da `Source/`, `Scenarios/`, `docs/` |
| dati esterni | **no come chiave** | le 2 occorrenze in `Scenarios/` sono in campi nota (`_nota_numeri`, `_nota_permutazione`), non parsate |
| decisione aperta correlata | **sì** | `BAL-3` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) cita il simbolo per nome |

Il rename è quindi interno — ma **non piccolo**. Write-set reale ≈ **13 file**: 1 header, 7 file di test,
`RTTurnManager.cpp`, 2 JSON di scenario, ≥3 documenti. Il work order lo presenta come «rename puramente
interno da verificare».

⚠️ `BAL-3` chiede *«con quali numeri mitiga la `Guard` piantata»* e usa il simbolo per nominare il problema.
Il rename non tocca il valore, quindi non pregiudica la decisione — ma se il simbolo cambia nome e la voce
non viene aggiornata, `BAL-3` resta con un riferimento morto.

---

## 6. Osservazioni di processo

### 6.1 Le issue da consultare sono in parte le sbagliate

**Nessuna issue `OPEN` copre alcuna delle quattro wave**, e `FRTReactionPassResult` non compare in nessuna
issue — aperta o chiusa. La discovery del work order è quindi legittima.

Ma la lista di lettura obbligatoria manca gli antenati diretti e ne include uno marginale:

| issue | stato | rilevanza |
|---|---|---|
| [#2207](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2207) | CLOSED | **mancante** — è la deriva-commenti che le wave 2 e 3 continuano |
| [#2118](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2118) | CLOSED | **mancante** — *«quali costanti di combattimento si possono cambiare senza che niente diventi rosso»*: è l'audit della wave 3 |
| [#1951](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1951) | CLOSED | **mancante** — nascita di `SourceId`, antenato della wave 4 |
| [#1909](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1909) | CLOSED | **mancante** — `D-292`, la Guardia |
| [#2190](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2190) | CLOSED | ⛔ **citata a torto** — corpus del replay viewer, una sola menzione di `ApplyFirstHitDelta` |

### 6.2 Il bootstrap §0 non è soddisfacibile da una shell non interattiva

Il work order impone `rtws -Action verify` senza clausola condizionale, e dichiara `BLOCKED` se una
precondizione non coincide. Misurato: `rtstatus`, `rtws`, `rtlease`, `rtmcp`, `rtsuite` **non risolvono** né
in Bash né in PowerShell non interattiva. Gli script esistono — `scripts/rt-workspace.ps1`,
`scripts/rt-lease.ps1`, `scripts/rt-suite.ps1` — quindi gli alias vengono da un profilo che quella shell non
carica.

Una precondizione fail-closed va scritta sull'artefatto stabile, cioè il path dello script, non sull'alias.

### 6.3 Deriva già misurabile che nessuna wave raccoglie

Due commenti falsi al `HEAD` misurato, entrambi in punti che le wave attraversano:

- `RTCombatLibrary.h:210` — *«`Marked` passa dalla stessa `ApplyFirstHitDelta` di `Exposed` e **`Guard`**»*:
  falso su entrambi. La Guardia è un pool da `D-292`; `Marked` non ci passa affatto (dettaglio nel work
  order della wave 4);
- `RTTurnManager.cpp:~5321` — *«`FRTAttack` conserva solo il bersaglio»*: superato da `D-212`, che ha
  aggiunto `AttackerIndex` (`RTCombatResolver.h:177`). È la giustificazione per cui i bonus condizionati
  alla coppia stanno fuori dal resolver, e non regge più.

🔑 Entrambi i commenti portano già un ⚠️ con un `D-nnn` di una correzione precedente. Il marcatore prova
manutenzione, non verità: la revisione che l'ha lasciato lì aveva sistemato una metà sola.

### 6.4 Ciò che il work order fa bene

Va detto, perché è la parte che non va cambiata.

- **§10 è fedele** a [`WAVE_DEV_LEAD.md`](../../rt-three-terminals/prompts/WAVE_DEV_LEAD.md) `:181-195`:
  busta e non payload, nessuna matrice di verdetti, semantica corretta di `STATUS: READY`.
- Il naming `RT3-DEVLEAD-<sha7>.md` e la cartella `contrib/` **corrispondono** al precedente reale
  (`waves/parsecell-arity/`).
- La sequenzialità delle wave è necessaria e non è burocrazia: 1 e 2 toccano lo **stesso** header e lo
  stesso `RTTurnManager.cpp`, 3 e 4 lo stesso `RTCombatLibrary.h`. Eseguirle in parallelo produce conflitti
  garantiti.

---

## 7. Cosa non è stato misurato

- **Se un Blueprint chiami `ApplyFirstHitDelta`** — è `UFUNCTION(BlueprintPure)`, e il selettore sui
  `.uasset` è stato scartato in calibrazione (§2). Dominio EDITOR.
- **Compile, Automation, scenari, determinismo, replay, PIE, packaged**: `NOT RUN — dominio VALIDATION`.
  Nessuna wave è stata eseguita: questo documento è una revisione di specifica, non un referto di gate.
- **Il gate dei link del repository**: non eseguito. I link relativi di questo file sono stati verificati a
  mano, uno per uno.
