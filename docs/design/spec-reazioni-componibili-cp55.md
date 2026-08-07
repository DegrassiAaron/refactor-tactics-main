# Spec — Reazioni componibili (E5, CP 5.5)

> **Issue**: `#154` · **Epic**: `#19` (E5) · **Sblocca**: `#155` (CP 6.7) · **Data**: 2026-08-07
> **Branch**: `feat/154-reazioni-componibili` · **Baseline misurata**: 342 test in 50 file, 0 fallimenti
> Fonti: [`roadmap-v0.1.md`](roadmap-v0.1.md) §5 E5 · [`RT_ActionCatalog_v0.1.md`](balance/RT_ActionCatalog_v0.1.md) §4 ·
> catalogo eroi v0.1 §1-§4 · [`adr-0003-modello-azioni-v01.md`](adr-0003-modello-azioni-v01.md)

## 1. Obiettivo

E5 era chiusa con 24 test verdi, ma **nessuno la consumava**: le reazioni degli eroi hanno `Effects` vuoto e
commenti «arriva con E5» nel catalogo. Un motore che nessuno consuma non è finito — è un motore non collaudato.

CP 5.5 aggiunge al motore ciò che manca perché le reazioni d'eroe siano *rappresentabili*; **CP 6.7 (`#155`) le
cabla**. Qui non si tocca il catalogo eroi.

## 2. Stato verificato (2026-08-07, sul codice)

| Cosa serve a una reazione d'eroe | Stato prima di CP 5.5 | Verdetto |
|---|---|---|
| Dichiarare **più effetti** on-trigger | `RTTurnManager.cpp:1377` leggeva il **primo** `Damage` e usciva (`break`) | ❌ metà reazione persa |
| Riduzione del danno come **dato** | `if (Reaction->Def.ActionId == "Action.Deflect")` (`RTTurnManager.cpp:1392`) | ❌ ramo per ActionId |
| **Identità** nel TurnLog | `FRTTurnLogEntry` non ha un campo per l'azione | ❌ `Bastion.Interposition` ≡ `Action.Intercept` |
| **Helper** di costruzione | `MakeHeroAction` è in un namespace anonimo, non sa cos'è una reazione | ❌ assente |

Un solo difetto strutturale sotto i quattro: il resolver sapeva **cosa fanno** le reazioni invece di **leggerlo**
dai dati. È lo stesso schema del «dato senza consumatore» visto al CP 8.2, ribaltato: qui il consumatore
esisteva ma leggeva solo ciò che si aspettava.

## 3. Decisioni

### D1 — Il bersaglio dell'effetto è una regola per TIPO, non un campo per azione

`URTReactionLibrary::BuildReactionEvents` assegna il bersaglio così:

| Effetto | Bersaglio | Perché |
|---|---|---|
| `Damage`, `Push`, `Pull`, `Status` | **chi ha innescato** | è il senso di un colpo di ritorno |
| `Heal`, `Shield`, `DamageReduction` | **chi reagisce** | è il senso di una difesa |

*Alternativa scartata*: un campo `Target` su `FRTActionEffectSpec`. Avrebbe aggiunto un dato da compilare in
**ogni** azione del catalogo (~35) per servire un caso che riguarda solo le reazioni, e ogni valore sbagliato
sarebbe stato un difetto silenzioso. La regola per tipo è totale (switch senza `default`: un effetto nuovo non
compila finché qualcuno non decide dove punta) e non ha dati da mantenere.

*Conseguenza dichiarata*: `TriggeredBy == INDEX_NONE` elimina gli effetti offensivi invece di sceglierne un
bersaglio — un contrattacco senza attaccante identificato non si inventa una vittima (fail-closed).

### D2 — `ERTActionEffect::DamageReduction`: la riduzione è un effetto dichiarato

Il -20 di `Action.Deflect` era una costante letta da un ramo sull'ActionId. Diventa un effetto del catalogo,
con `URTCombatLibrary::DeflectDamageReduction` ancora come **fonte del valore** (il numero resta uno solo).

*Perché un effetto e non un campo di `FRTActionDef`*: «cosa fa un'azione e quanto» vive già negli `Effects`;
un secondo posto dove cercarlo renderebbe la lettura del catalogo ambigua. Il commento del catalogo che diceva
«ridurre il danno non è un effetto applicato a un bersaglio» è stato **corretto**, non aggirato: resta vero che
non è uno *scudo*, ed è per questo che ha un valore d'enum proprio invece di riusare `Shield`.

*Differenza da `Action.Guard`*: quello è una stance di Prep che passa da uno **stato** (`Status.Guarded`), non
una reazione con trigger. Il suo -15 resta dov'era.

### D3 — Lo scudo di una reazione protegge dal colpo che l'ha innescata

`Flux.ReactiveCapacitor` (CP 6.7) dichiarerà `Shield 15` **+** 10 danni all'attaccante. Lo scudo si applica
**prima** che i colpi siano risolti, aggiornando anche lo snapshot `States` da cui il resolver legge.

*Perché*: uno scudo temporaneo **scade nel Cleanup dello stesso turno**. Applicato dopo la risoluzione non
proteggerebbe da nulla, mai: sarebbe il quarto caso di «dato che nessuno legge». La simmetria con `Deflect`
(-20 sul colpo che l'ha innescata) è la stessa regola vista da un'altra angolazione.

### D4 — `FRTTurnLogEntry::ActionId` e formato serializzato v3

Il TurnLog guadagna l'identità dell'azione. Formato **versione 3**: `uint16` di lunghezza + byte UTF-8 in coda
alla voce — primo campo a lunghezza variabile del formato. Il loader accetta **anche la versione 2**, lasciando
l'ActionId vuoto: quei byte non contenevano un'identità, e inventarne una sarebbe peggio che non averla.

*Ordinamento e hash*: `EntryLess` confronta l'ActionId **lessicograficamente** (`FName::Compare`), mai
`FastLess` — quello ordina per indice nella name table, che dipende dall'ordine di creazione dei nomi nel
processo: due esecuzioni della stessa partita darebbero due hash diversi (invariante #4). L'hash mescola i byte
del nome, quindi due reazioni identiche in tutto tranne l'abilità non collidono più.

*Perché adesso e non a CP 11.3*: il corpus golden di TurnLog (CP 12.6, `#178`) fisserà i byte del formato.
Cambiarlo dopo significherebbe rigenerare il corpus; farlo ora costa una versione e due test.

*Limite dichiarato*: lo popolano solo le voci di categoria `Reaction`. Le voci `Combat` lo lasciano vuoto —
completare i reason code è **CP 11.3** (`#79`), e riempirlo a metà direbbe meno del nulla.

### D5 — `MakeHeroReactionFromCoreAction` è fail-closed sulla semantica core

Dal core arrivano fase, priorità, slot, trigger, fallback e interrompibilità; dall'eroe identità, cooldown ed
effetti. Se l'azione core non esiste o **non è una reazione**, l'helper restituisce `nullptr`: costruire una
"reazione" sopra un'azione principale produrrebbe un'abilità che il pass delle reazioni non guarda mai — inerte
in partita e silenziosa nel TurnLog.

`Effects` vuoto significa «gli stessi del core», non «nessun effetto»: per toglierli davvero il posto è
l'azione core.

## 4. Fuori scope dichiarato

- **Il cablaggio delle quattro reazioni d'eroe**: è CP 6.7 (`#155`). Qui il catalogo eroi non è toccato, e i
  test che oggi fissano `Effects.Num() == 0` restano verdi finché quel checkpoint non li sostituisce.
- **`Vektor.InterceptShot` e `Riva.FlowReaction`**: trigger d'ingresso su movimento e movimento reattivo →
  **E14** (ADR-0004), non E5.
- **`Heal`/`Push`/`Pull`/`Status` da reazione**: `BuildReactionEvents` li traduce (la regola è totale), ma il
  pass del `TurnManager` non li consuma: nessuna reazione del catalogo v0.1 li dichiara, e applicarli
  richiederebbe ciò che il pass non ha (una direzione per la spinta, il consumo degli stati insieme agli altri
  colpi). Il punto dove aggiungerli è dichiarato nel codice.
- **Cooldown residuo verificato a runtime**: l'harness di test non chiama `World->BeginPlay()`, quindi
  `AbilityCooldowns` resta vuoto e ogni cooldown legge 0 in tutta la suite — difetto noto, issue **`#135`**,
  non toccato qui. Il test verifica il cooldown sui **due** campi del dato (`Def.CooldownTurns` e il campo
  legacy che `ConsumeAbility` legge davvero): un helper che ne popolasse solo uno renderebbe la reazione
  riutilizzabile ogni turno senza che nulla lo segnali.

## 5. Test

| Test | Cosa fisserebbe se cadesse |
|---|---|
| `Reactions.MultiEffectReactionAppliesAll` | il resolver torna a leggere un solo effetto |
| `Reactions.HeroReactionKeepsIdentityInLog` | il TurnLog perde l'identità dell'abilità |
| `Reactions.NoHeroSpecificBranchInResolver` | riappare un ramo per ActionId (tre identità, stesso esito) |
| `TurnLog.ActionIdRoundTrip` | l'identità non sopravvive alla serializzazione o non entra nell'hash |
| `TurnLog.LegacyVersionWithoutActionIdIsReadable` | le tracce v2 diventano illeggibili |

### 5.1 Verifiche di mutazione eseguite

| Mutazione | Test caduti | Atteso |
|---|---|---|
| `BuildReactionEvents` si ferma al primo effetto | `MultiEffectReactionAppliesAll` | ✅ solo quello |
| La riduzione del danno viene ignorata | `NoHeroSpecificBranchInResolver`, `Deflect.ReducesDirectDamage`, `Deflect.ZeroDamageStillHits` | ✅ il nuovo + i due di E5 |
| L'ActionId non viene serializzato | `TurnLog.ActionIdRoundTrip` | ✅ solo quello |
| L'ActionId non entra nella voce di log | `HeroReactionKeepsIdentityInLog` | ✅ solo quello |

**Suite**: 347 test unici in 51 file (da 342 in 50), 0 fallimenti, build `RefactorTactics` e
`RefactorTacticsEditor` verdi.

> ⚠️ **Correzione di conteggio**: `roadmap-checkpoint.md` e `roadmap-v0.1.md` dichiaravano **338 test in 49
> file**. Il valore misurato su `main` prima di questo checkpoint è **342 in 50**: i merge di `#179`/`#180` sono
> arrivati dopo l'ultima misura. Rimisurato col comando canonico, non citato a memoria.

## 6. File coinvolti

| File | Modifica |
|---|---|
| `Turn/RTActionEvent.h` | `ERTActionEffect::DamageReduction` (in coda all'enum) |
| `Turn/RTActionEffectLibrary.cpp` | traduzione del nuovo effetto |
| `Turn/RTReactionLibrary.{h,cpp}` | `BuildReactionEvents` + regola di bersaglio per tipo |
| `Turn/RTTurnManager.cpp` | pass delle reazioni senza rami su ActionId; scudo prima della risoluzione; identità nelle due voci di log |
| `Turn/RTTurnLog.h`, `Turn/RTTurnLogLibrary.cpp` | campo `ActionId`, ordine, hash, formato v3, descrizione |
| `Ability/RTCatalogLibrary.cpp` | `Action.Deflect` dichiara la propria riduzione |
| `Ability/RTHeroCatalogLibrary.{h,cpp}` | `MakeHeroReactionFromCoreAction` |
| `Tests/RTComposableReactionTests.cpp` (nuovo), `Tests/RTTurnLogSerializationTests.cpp` | i cinque test |

## 7. Rischi

- **Formato del TurnLog**: la versione 3 non è ancora stata scritta su disco da una partita reale. Il round-trip
  e la lettura delle tracce v2 sono coperti da test; il corpus golden (CP 12.6) partirà da questo formato.
- **`DamageReduction` fuori dalle reazioni**: un'azione di Prep che la dichiarasse verrebbe ignorata da
  `ResolvePrep` (come ogni effetto fuori posto). Il validator del catalogo non lo rifiuta: quando esisterà una
  seconda azione con riduzione, la forma giusta è una regola del validator, non un secondo `if`.
