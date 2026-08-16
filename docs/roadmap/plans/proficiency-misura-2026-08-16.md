# Proficiency elementale — la misura, prima dell'owner

> `SNAPSHOT` · **2026-08-16**, su `73ec9f9d` · **Fase 1 di [#995](https://github.com/DegrassiAaron/refactor-tactics-main/issues/995)**
> **Cosa è**: la fotografia delle capability elementali dei quattro eroi, estratta dai **dati** del
> catalogo. Serve a decidere se la baseline che l'owner dovrebbe fissare regge.
> **Cosa non è**: un owner. La grammatica e la tabella canonica vivranno in
> `docs/characters/elemental-proficiency.md`, che questa fase non scrive.
>
> ⚠️ **Cita i nomi legacy del codice — `Flux`, `Riva`, `Bastion`, `Vektor` — perché fotografa il codice
> com'è oggi**, e la migrazione di [D-130](../../decisions/RT_PDR_00_Decision_Log.md) è a fette ancora
> aperte (#753…#757). Questa cartella è esente dal gate di naming per questa ragione.

## Perché una fase separata

La baseline di #995 dava **Phase = `Access`**. Applicando il criterio operativo ai dati, [#1006](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1006) ha misurato **tre** capability Water, cioè `Master`: due gradi di distanza. Scrivere l'owner su una baseline non verificata significa scrivere un documento da riscrivere — e la baseline di **Gadget** non era mai stata misurata allo stesso modo.

## Il criterio applicato

Quello che #995 fissa: una capability conta se il kit **Generate · Apply · Propagate · Transform · Consume** un elemento, uno stato o una superficie, e la prova sta **nei dati**:

- ✅ un `FRTActionEffectSpec` che dichiara lo stato o la superficie dell'elemento;
- ✅ `bCreatesSurface` con `SurfaceCreated`;
- ❌ `DamageType`, tag tematico, VFX, nome dell'abilità, slot Paragon.

Fonte: `Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp`, le quattro factory `MakeFlux`, `MakeRiva`, `MakeBastion`, `MakeVektor`.

## La misura

### Gadget (`Hero.Flux`) — `Affinity.Electricity`, debolezza `Affinity.Water`

| Abilità | Effetti dichiarati | Elementale? |
|---|---|:--:|
| `Flux.ArcPulse` | attacco base generico (`MakeBasicAttack(4)`) | ❌ |
| `Flux.LinearDischarge` | **solo `Damage 24`**, forma linea | ⚠️ vedi sotto |
| `Flux.ConductiveNode` | **è** `Action.Electrify`, con `PropagationLimit` dal core | ✅ **Propagate** Electric |
| `Flux.Overload` | `Damage 18`, area r1 | ❌ |
| `Flux.ReactiveCapacitor` | `Shield 15` + `Damage 10` | ❌ |

### Phase (`Hero.Riva`) — `Affinity.Water`, debolezza `Affinity.Electricity`

| Abilità | Effetti dichiarati | Elementale? |
|---|---|:--:|
| `Riva.PressureJet` | `Status.Wet` + `Push 1`, linea | ✅ **Apply** Water |
| `Riva.CircularTide` | `Heal 18` — il `Wet` è uscito con #1006 | ❌ |
| `Riva.FluidTrail` | `Action.Dash` — l'acqua è uscita con #1006 | ❌ |
| `Riva.MistVeil` | `bCreatesSurface` → `Smoke` | ⚠️ vedi «Smoke» |

### Riktor (`Hero.Bastion`) — `Affinity.Structures`, debolezza `Affinity.Movement`

| Abilità | Effetti dichiarati | Elementale? |
|---|---|:--:|
| attacco base | `Damage 8` + `Status.Slow` | ❌ |
| `Bastion.KineticPanel` | crea copertura (struttura, non superficie) | ❌ |
| `Bastion.Reconfigure` · `Bastion.Ram` · `Bastion.Interposition` | nessun effetto elementale | ❌ |

### Wraith (`Hero.Vektor`) — `Affinity.Movement`, debolezza `Affinity.Structures`

| Abilità | Effetti dichiarati | Elementale? |
|---|---|:--:|
| attacco base | `Damage 21` | ❌ |
| `Vektor.InterceptShot` · `PassingBlade` · `Feint` · `Deflection` | solo `Damage` | ❌ |

## 🔴 Il risultato: il criterio non cattura tutto, e la baseline dipende da come lo si scrive

`Flux.LinearDischarge` dichiara **solo `Damage 24`**. Il suo comportamento elettrico — il **+8 contro bersaglio `Status.Wet`** — vive nel **resolver**, non nei dati dell'azione. Il codice lo dichiara e ne spiega la ragione (`Turn/RTTurnManager.cpp:3971`):

> *«`Flux.LinearDischarge` +8 contro bersaglio `Status.Wet` (catalogo eroi §1). Non è nella lista `Effects` perché non è un danno fisso, e vale su OGNI colpo dell'azione finché il bersaglio è bagnato.»*

È un **Consume** di uno stato elementale — uno dei cinque verbi che #995 elenca — ma la prova non è dove il criterio la cerca. Ne seguono due letture, e **la baseline cambia a seconda di quale si sceglie**:

| Criterio | Gadget | Phase | Riktor | Wraith |
|---|---|---|---|---|
| **letterale** (solo `Effects` / `bCreatesSurface`) | `ConductiveNode` → **`Access`** | `PressureJet` → `Access` | `None` | `None` |
| **esteso** (include il resolver) | + `LinearDischarge` → **`Specialist`** | `Access` | `None` | `None` |

**La baseline di #995 regge solo col criterio esteso.** Col criterio letterale — quello scritto oggi nella issue — Gadget scende ad `Access`, e la tabella canonica cade su due eroi su quattro invece che su uno.

⚠️ Non è un dettaglio di forma: è la differenza fra un criterio che si applica leggendo una funzione e uno che richiede di cercare il nome dell'abilità in tutto `Source/`. Il secondo è più vero e **più caro da applicare**, e il costo va accettato consapevolmente.

## Tre questioni che la misura ha sollevato e non risolve

**1. `Smoke` è un elemento?** `Riva.MistVeil` crea davvero una superficie (`bCreatesSurface` → `Smoke`), quindi *genera* qualcosa. Se `Smoke` conta come elemento, Phase ha due capability di elementi **diversi** — e la grammatica di #995 conta per elemento, quindi resterebbe `Access` di Water più `Access` di Smoke. Se non conta, va scritto perché. Oggi la issue non nomina né `Smoke` né `Fire` fra gli elementi.

**2. Le azioni core ospitate contano.** `ConductiveNode` **è** `Action.Electrify` e `FluidTrail` **era** `Action.CreateWater`: l'eroe è il veicolo di un'azione di catalogo. #1006 ha scartato l'opzione di escluderle — sarebbe scesa anche la baseline di Gadget — quindi **contano**, ed è una decisione già presa che l'owner deve scrivere invece di lasciare implicita.

**3. La debolezza non è mai stata considerata.** `Affinity.Water` è la **debolezza** di Gadget e l'affinità di Phase: lo stesso identificatore in due ruoli. La grammatica di proficiency non ha nulla che esprima «vulnerabile a», e l'asse `Affinity`/`Weakness` sì. Un'altra ragione per tenerli distinti — e per scriverlo.

## Raccomandazione per la fase 2

1. **Adottare il criterio esteso** e dichiararlo per intero: la prova di una capability sta negli `Effects`, nei flag di superficie **oppure** in una regola del resolver che nomina l'abilità. Con l'ultima clausola va il comando che la verifica: `grep -rn "<Abilità>" Source/RefactorTactics --include=*.cpp`.
2. **Scrivere la tabella canonica con la colonna «dove sta la prova»**, non solo il grado. È ciò che rende la classificazione rifacibile da un altro invece che ereditata.
3. **Decidere su `Smoke`** prima di pubblicare la tabella, o la prima persona che legge `MistVeil` riaprirà la domanda.

## Cosa questa fase NON ha misurato

- **v0.2** — Aurora, Kwang, Murdock e Steel hanno le pagine e **nessun dato**: il catalogo dichiara quattro factory. Il criterio legge gli effetti, quindi su di loro non è applicabile. Un grado assegnato lì sarebbe design, non misura.
- **L'equipaggiamento** — `Gadget.Sprinkler` porta `Action.CreateWater`, ma per #995 un Generic Equipment è `External Access` e non fa proficiency. Nessuna misura dei loadout è servita.
- **Le pagine `docs/characters/`** — fuori dal write-set di questa track: nove file lì portano blocchi generati la cui sorgente è `integration_only`.
