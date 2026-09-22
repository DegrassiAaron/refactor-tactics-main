# Tassonomia icone — il manifest di design contro `ERTIconCategory`

> **Tipo**: documento owner di riconciliazione · **Creato**: 2026-09-22 · **Owner della domanda**: *quali
> categorie del materiale di design esistono nel runtime, quali no, e che cosa si fa di quelle che non ci
> sono* · **Issue**: [#637](https://github.com/DegrassiAaron/refactor-tactics-main/issues/637) ·
> **Epic**: [#217](https://github.com/DegrassiAaron/refactor-tactics-main/issues/217) (E20)
>
> ⛔ **Questo documento non decide come si compone una card** — quella è
> [`../systems/spec-icon-card-grammar.md`](../systems/spec-icon-card-grammar.md)
> ([D-231](../../decisions/RT_PDR_00_Decision_Log.md)) — **né quali icone servono alla v0.1**, che è
> [`brief-icone-v01.md`](brief-icone-v01.md). Possiede una cosa sola: la **corrispondenza fra i segmenti
> del manifest e i valori di `ERTIconCategory`**, e lo statuto della sorgente da cui il manifest viene.

**Convenzione sui numeri.** Ogni conteggio di questa pagina porta accanto il comando che lo produce ed è
datato. Un numero senza comando non va copiato da qui: si rimisura.

---

## 1. Perché esiste

Una chiave il cui segmento non è un valore di `ERTIconCategory` non è caricabile, e i punti che la fermano
sono **due** — confonderli fa cercare il simbolo sbagliato:

| Dove | Che cosa fa |
|---|---|
| `URTIconLibrary::IsDeclaredIconCategory` (`RTIconLibrary.cpp:29`) | itera `ERTIconCategory` e confronta il **capo** del percorso semantico. È il controllo che scarta un segmento sconosciuto nel percorso che **costruisce** l'id (`:71`, `:136`) |
| `URTIconLibrary::ValidateIconCatalog` | confronta il **segmento** dentro l'`IconId` con la `Category` **dichiarata nella voce**, e ne pretende la corrispondenza |

⚠️ **Il secondo non enumera l'enum, e non ne ha bisogno**: `FRTIconDef::Category` è un campo
**tipizzato**, quindi una categoria fuori dall'enum non è nemmeno **esprimibile** in una voce di catalogo.
Non è un avviso, è un rifiuto — e per due ragioni indipendenti. Il materiale di design usa segmenti che il runtime non ha, quindi una parte
del manifest **non è innestabile così com'è**.

D-031 dà il criterio, e non è estetico: *il catalogo risolve ciò che il gameplay produce come chiave*. La
sua forma eseguibile è `URTIconLibrary::RequiredIconIds()`, che deriva le chiavi da **cinque** sorgenti di
dati di gioco e da nessun elenco scritto a mano:

| Famiglia | Da dove il codice la deriva |
|---|---|
| `Phase` | i quattro valori volontari di `ERTMatchPhase` |
| `Action` | `URTCatalogLibrary::GetCoreActionCatalog()` più le abilità dei kit d'eroe |
| `Status` | i figli registrati del tag `Status.` |
| `Certainty` | i tre livelli di CP 11.2 |
| `Identity` | `URTHeroCatalogLibrary::GetHeroIds()` più la relazione di squadra |

`RefactorTactics.IconCatalog.V01CategoriesPopulated` pinna esattamente queste cinque come popolate e le
altre come **vuote di proposito**, e il suo commento dice perché: *«una chiave che comparisse qui
chiederebbe un disegno per un sistema che ancora non la consuma»*.

∴ la domanda «questa categoria entra nell'enum?» ha una forma operativa: **esiste una macchina che ne
deriverebbe le chiavi dai dati di gioco?** Non «il manifest la nomina».

---

## 2. Statuto della sorgente di design — risolto, e due premesse di #637 sono scadute

🔴 **I sorgenti di design sono versionati dal 2026-08-19.** #637 è nata dichiarando che
`docs/src/design/icon/` era *untracked* e che non c'era *«una fonte stabile da riconciliare»*. Quel blocco
non esiste più: [#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165) ha spostato
quell'area in `docs/research/design/icon/` con il commit `ce05ed76` — *«quattro aree di `docs/src` prendono
il nome che dice cosa sono — solo move»*.

```
git log -1 --format='%h %ad' --date=short --diff-filter=A -- docs/research/design/icon/CLAUDE_DESIGN_02_Icon_Manifest_v0.1.md
git ls-files docs/research/design/icon/ | wc -l
```

⚠️ **Versionati non significa vincolanti, e la differenza è la risposta alla domanda «con quale
statuto».** `AGENTS.md` §2 elenca `docs/research/` fra ciò che **non è source of truth per default**. Quindi:

- il materiale **si può citare** e non si muove più sotto i piedi di chi lo legge: è in `main`, ha una
  storia, e una modifica produce un diff invece di un errore silenzioso;
- ma **non è specifica**: un batch di produzione che lo citasse come tale citerebbe una fonte non
  normativa. Ciò che vincola è il documento owner che lo **recepisce** — `brief-icone-v01.md` per le chiavi
  e il colore della v0.1, questa pagina per la tassonomia.

Questo è lo statuto, e non richiede una decisione nuova: lo fissa già `AGENTS.md` §2.

⚠️ **Seconda premessa scaduta**: due caselle della DoD di #637 rimandano a `docs/src/README.md` per la
regola *«un sorgente recepito non resta più qui: si sposta in `../archive/src/`»*. Quel file è stato
rimosso da #1165 insieme all'area che lo conteneva, quindi quelle caselle **non sono eseguibili come
scritte** — non si spuntano su un file che non esiste, e vanno riformulate sulla regola di `AGENTS.md` §2
prima di poter essere dichiarate verdi.

---

## 3. La misura — rifatta il 2026-09-22, non copiata

Sui due file del manifest insieme (`CLAUDE_DESIGN_02_Icon_Manifest_v0.1.md` e
`RefactorTactics_UI_Icon_Manifest_v0.1.csv`), contando i segmenti dentro gli `UI.Icon.<Segmento>.<Nome>`:

| | 2026-08-12 | **2026-09-22** |
|---|---:|---:|
| chiavi uniche | 195 | **195** |
| segmenti distinti | 25 | **25** |
| chiavi con segmento **dentro** l'enum | 84 | **84** |
| chiavi con segmento **fuori** | 111 | **111** |
| segmenti fuori dall'enum | 17 | **17** |

✅ **La misura di agosto non è invecchiata**, ed è un esito che va scritto: il manifest non si è mosso da
quando #637 l'ha misurato — si è mosso il suo *percorso* (§2), che è un'altra cosa.

⚠️ **Il segmento non è la colonna «Categoria» del manifest.** Quella colonna porta accent e famiglia di
design (`Movement`, `Defense/Reaction`, `Utility`…) e non ha nulla a che vedere con l'enum. `ValidateIconCatalog`
guarda il segmento dell'`IconId`. Confonderli fa contare le categorie sbagliate.

```
grep -ohE 'UI\.Icon\.[A-Za-z]+\.[A-Za-z0-9_.]+' \
  docs/research/design/icon/CLAUDE_DESIGN_02_Icon_Manifest_v0.1.md \
  docs/research/design/icon/RefactorTactics_UI_Icon_Manifest_v0.1.csv \
  | sort -u | cut -d. -f3 | sort | uniq -c | sort -rn
```

**Quattro valori dell'enum non compaiono mai nel manifest**: `Certainty`, `Identity`, `Information`,
`MapInteraction`. La copertura va nella direzione opposta a quella che sembra, ed `Identity` è il caso che
#637 racconta — il manifest colloca gli alleati sotto `Target.*` mentre il codice li vuole sotto
`Identity.*`.

---

## 4. Dei diciassette, quanti sono davvero una decisione

| Sorgente | Quante chiavi | Esito | Chi lo stabilisce |
|---|---:|---|---|
| `Intel.*` | 9 | → **`Information`** | la descrizione dell'enum: *«cosa la squadra SA»* |
| `Map.*` | 12 | → **`MapInteraction`** | *«porte, leve, ponti, ascensori»*, alla lettera |
| `Surface.*` | 8 | → **`Environment`** | *«superfici e terreni»* |
| `Role.*` | 4 | → **`Identity`** | *«chi è: personaggio, **ruolo**…»* |
| `Faction.*` | 2 | → **`Identity`** | idem — *«…**fazione**…»* |
| `Boundary.*` | 2 | → **`Phase`** | confine di fase |
| `UI.*` | 6 | **esce dal catalogo** | è chrome, non semantica di gioco: D-031 risolve ciò che *il gameplay produce come chiave*, e un pulsante Undo non lo è |
| `Effect.*` | 14 | **nessun valore d'enum richiesto** | [D-231](../../decisions/RT_PDR_00_Decision_Log.md): primitiva della **grammatica compositiva** |
| `Geometry.*` | 7 | **nessun valore d'enum richiesto** | idem (`Shape`/`Geometry`) |
| `Target.*` | 8 | **metà e metà** | `Ally`/`Enemy` → `Identity`; `Cell`/`Object`/`Direction`/`Structure` restano primitive di composizione ([D-231](../../decisions/RT_PDR_00_Decision_Log.md)) |

Restano **sette** segmenti su cui nessuna fonte normativa si è pronunciata: `Stat` · `Gadget` · `Module` ·
`Weapon` · `Decision` · `Timing` · `Result`.

⚠️ **`spec-icon-card-grammar.md` §1 dichiara `ERTIconCategory` e `RequiredIconIds()` esplicitamente FUORI
dal proprio scope.** `D-231` chiude la scorciatoia — *«non serve un valore d'enum per comporre una card»* —
ma **non arbitra** questi sette. Chi cercasse lì la risposta troverebbe un non-scope, non un silenzio.

---

## 5. I sette, misurati contro il criterio

Per ognuno: il gameplay produce quel nome come **id**? Misurato il 2026-09-22 su `Source/`, esclusi i test.

```
git grep -ohE '"<Segmento>\.[A-Za-z]+"' -- Source/ ':!Source/RefactorTactics/Tests/' | sort -u
```

### Hanno un'entità reale dietro — la decisione costa, perché i numeri dell'enum sono serializzati

| Segmento | Chiavi | Cosa esiste nel codice |
|---|---:|---|
| **`Module`** | 7 | ✅ **tutti e sette**, sotto un altro nome: `Reaction.AllyIntercept`, `.Anchor`, `.Cleanse`, `.CounterShot`, `.EmergencyDash`, `.HazardEscape`, `.ReactiveShield` — gli `EquipmentId` che `URTCatalogLibrary::MakeReactionModules()` costruisce |
| **`Gadget`** | 8 | ✅ con lo **stesso** nome: `Gadget.BreachCharge`, `.Insulator`, `.Medkit`, `.Mine`, `.PortableCover`, `.Sensor`, `.SmokeEmitter`, `.Sprinkler` |
| **`Weapon`** | 6 | ✅ con lo **stesso** nome e in **corrispondenza esatta**: `Weapon.Environmental`, `.Impact`, `.Overcharge`, `.Precision`, `.Split`, `.Suppressive` |

🔴 **E le due liste `Gadget` divergono, in entrambi i versi.** Il codice ha `Gadget.Mine`, che il manifest
non nomina; il manifest ha `Gadget.Anchor`, che il codice non ha. ⚠️ **`Gadget.Anchor` è anche una trappola
di nome**: `Reaction.Anchor` e `Gadget.Anchor` sarebbero due cose diverse con lo stesso nome, e
`10-catalogo-sette-categorie.md` lo segnala già. Chi decide `Gadget` decide anche questo.

⚠️ **Su `Module` esiste una proposta in conflitto con l'istruttoria di #637, e va risolta esplicitamente**:
`10-catalogo-sette-categorie.md` — materiale di ricerca, quindi **non vincolante** (§2) — assegna i moduli a
**chiavi proprie in `Reaction`**, perché risolverli dal `GrantedActionId` farebbe collassare
`ReactiveShield` e `CounterShot`, che condividono `Action.Counter` con effetti opposti. L'istruttoria di
#637 obietta che la categoria `Reaction` dell'enum è il **ciclo di vita** di una reazione — *«armata,
opportunità, consumata, invalidata»* — e che metterci gli oggetti confonde la cosa con la sua macchina a
stati. **Le due osservazioni sono entrambe vere**: la prima dice perché i moduli servono chiavi proprie, la
seconda perché `Reaction` non è il posto. Nessuna delle due dice dove vanno.

### Non esistono come chiave — e due, mappate, appiattirebbero informazione vera

| Segmento | Chiavi | Cosa ha trovato la misura |
|---|---:|---|
| **`Stat`** | 11 | ❌ **zero id**. Sono **letture numeriche**: `Cooldown` e `Range` sono campi di `FRTActionDef`, `Health` e `Shield` di `ARTUnit`. ⚠️ `Vision` e `Noise` non esistono nemmeno come campo in quei due header. Un numero non ha una chiave — e D-231 colloca costo/cooldown/cariche fra i **satelliti della card** (cerchio, alto-sinistra, max 1) |
| **`Decision`** | 2 | ❌ `FastAction` e `FastReaction` non esistono. L'enum che porta quel nome è `ERTReactionDecisionOutcome`, e ha **sei** valori — `Chosen`, `CollapsedByCondition`, `Immediate`, `NoDecider`, `Rejected`, `Timeout`. 🔴 Mapparci sopra due chiavi **perderebbe** quattro esiti |
| **`Timing`** | 3 | ❌ parziale: `Predictive` esiste davvero (`ERTPredictiveOutcome`, `ERTPredictiveTargeting`, `FRTPredictiveShot`); `Delayed` e `Trap` danno **zero** riscontri non-test |
| **`Result`** | 2 | ❌ `Success`/`Failure` generici non esistono. Gli `ERT*Result` del codice sono **specifici per dominio** (`ERTNavResult::BlockedByModal`, `ERTMovementAdvanceResult::Suspended`, …): un binario sopra di loro sarebbe una terza verità sopra due già distinte |

⚠️ **Zero riscontri non è di per sé una risposta**, ed è il difetto che #1403 racconta: un test che cercava
`MakeGenericActions` nel file sbagliato leggeva lo zero come *«non è generica»*. Qui la domanda è stata
**ribaltata** su ciascuno dei cinque segmenti a zero — *esiste sotto un altro nome?* — ed è così che
`Module` è risultato **pieno** (`Reaction.*`) mentre gli altri quattro sono risultati vuoti davvero.

---

## 6. Cosa NON si fa

⛔ **Non si forza una categoria in una esistente per far passare il validator.** Il validator confronta il
segmento con la categoria dichiarata e **non giudica se la classificazione ha senso**: una forzatura passa
in verde e mente in modo permanente, perché i numeri dell'enum sono **serializzati negli asset**.

⛔ **Non si aggiunge un valore all'enum se non in coda**, e non lo si aggiunge affatto senza una voce di
Decision Log: `RTIconCatalogData.h` lo scrive — *«aggiungere valori solo IN CODA: i numeri già serializzati
negli asset non cambiano»*.

⛔ **Non si aggiunge oggi una chiave in una delle sette categorie che #219 prescrive vuote**: fa fallire
`RefactorTactics.IconCatalog.V01CategoriesPopulated`, che verifica **entrambe** le direzioni. È voluto.

⛔ **Non si modifica `ERTIconCategory` per derivare una risposta.** Se una modifica all'enum sembra il modo
di chiudere una riga di §5, la decisione è stata **dedotta invece che presa**.

---

## 7. Puntatori

- [`brief-icone-v01.md`](brief-icone-v01.md) — quali chiavi servono alla v0.1 e di che colore sono
- [`../systems/spec-icon-card-grammar.md`](../systems/spec-icon-card-grammar.md) — come si compone una card
  ([D-231](../../decisions/RT_PDR_00_Decision_Log.md)); dichiara `ERTIconCategory` fuori scope
- `Source/RefactorTactics/UI/RTIconCatalogData.h` — l'enum e la regola di serializzazione
- `Source/RefactorTactics/UI/RTIconLibrary.cpp` — `RequiredIconIds()` e `ValidateIconCatalog()`
- `Source/RefactorTactics/Tests/RTIconCatalogTests.cpp` — `V01CategoriesPopulated` e gli altri gate
- `docs/research/design/icon/` — il materiale di design: **versionato, non vincolante** (`AGENTS.md` §2)
- [#266](https://github.com/DegrassiAaron/refactor-tactics-main/issues/266) — CP 25.1, che porta a spec la
  tassonomia **dopo** che #637 l'ha decisa. Le due non si duplicano: #637 **decide**, #266 **specifica**
