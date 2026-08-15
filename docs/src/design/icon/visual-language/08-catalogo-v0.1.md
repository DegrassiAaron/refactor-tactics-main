# Visual Language — Catalogo v0.1

> **Statuto**: sorgente di design, non canone. Vedi [`01-principi.md`](01-principi.md).
>
> Questo documento è l'**asse catalogo**: le chiavi che un widget risolve a runtime. Le primitive di disegno
> stanno in [`03-forme-e-primitive.md`](03-forme-e-primitive.md) e **non compaiono qui**.

## 1. Il set richiesto non si scrive a mano

`URTIconLibrary::RequiredIconIds()` **genera** la lista delle icone obbligatorie da tre fonti vive del
gioco. Non è una lista in un documento: è una funzione, e cambia da sola quando cambia il gioco.

| Fonte | Che cosa produce | Voci |
|---|---|---|
| `ERTMatchPhase` — le quattro fasi volontarie | `UI.Icon.Phase.*` | **4** |
| `URTCatalogLibrary::GetCoreActionCatalog()` | `UI.Icon.Action.*` | **36** |
| tag figli di `Status.` in `RTGameplayTags.cpp` | `UI.Icon.Status.*` | **11** |
| `URTHeroCatalogLibrary::GetHeroRoster()` | `UI.Icon.Identity.*` | **4** |
| | **totale** | **55** |

### 1.0 La cinquantaduesima

`RequiredIconIds()` non la genera, ma senza di lei **il catalogo non passa la validazione**:

```text
MissingIcon non impostata: una chiave sconosciuta non avrebbe nulla da mostrare
```

È un campo di `URTIconCatalogData`, non una chiave del dizionario, e `ResolveIcon` la restituisce con
`bResolved = false` ogni volta che qualcuno chiede una chiave che non esiste. Si vede **solo quando qualcosa
è rotto**, e proprio per questo non deve somigliare né a `Invalid` (slash) né a `Uncertain` (fade più `?`):
deve dire «manca il contenuto», che è un'altra cosa.

Prodotta come `RT_UI_Icon_MissingIcon`: quattro angoli di un contenitore vuoto.

`URTIconLibrary::FindMissingRequiredIcons()` confronta il catalogo spedito con questa lista. Una chiave
dichiarata con **asset nullo non copre**: è il widget vuoto che CP 20.1 vieta.

La conseguenza pratica per chi produce asset: **la lista da disegnare si interroga, non si copia**. Un
manifest scritto a mano invecchia il giorno in cui qualcuno registra un tag nuovo; questa funzione no.

### 1.1 Stato misurato

Al 2026-08-12, sul branch corrente: **nessun `DA_IconCatalog` esiste, nessuna texture icona esiste in
`Content/RT`**. `FindMissingRequiredIcons(nullptr)` restituisce l'intera lista — «nessun catalogo non è zero
mancanze: è la mancanza totale».

Le 51 icone del set obbligatorio sono quindi **tutte da produrre**.

## 2. Azioni

Le **sette generiche universali** ([D-025](../../../../decisions/RT_PDR_00_Decision_Log.md)):

`Wait` · `Move` · `BasicAttack` · `Guard` · `Brace` · `Interact` · `Overwatch`

Più i profili di movimento e le azioni di controllo che il catalogo spedisce. In tutto 36 `ActionId`
dichiarati, fra cui: `Sprint`, `Dash`, `Charge`, `Leap`, `Reposition`, `PrecisionAttack`, `HeavyAttack`,
`LineAttack`, `CircularAoE`, `SuppressiveLine`, `MarkTarget`, `Counter`, `Deflect`, `Intercept`, `Anchor`,
`Purge`, `Evade`, `Cleanse`, `CreateCover`, `CreateWater`, `Electrify`, `Ignite`, `Heal`, `Shield`, `Push`,
`Pull`, `Root`, `Slow`, `Interrupt`, `ModifyArc`.

### 2.1 Due avvertenze che il manifest sorgente sbaglia

**`Guard` non è legacy.** Il manifest del pack lo marca `LEGACY_CHECK`. È invece una delle sette generiche,
confermata da D-025, ed è **presente in `GetCoreActionCatalog()`**. Ha tre consumatori — il catalogo azioni,
l'interazione con `Status.Root`, la difesa direzionale di
[ADR-0005](../../../../decisions/adr-0005-orientamento.md) §4a. La sua icona è obbligatoria.

**`Overwatch` non è ancora nel catalogo.** È canonica fra le sette generiche, ma `Action.Overwatch` **non
compare** in `GetCoreActionCatalog()`: arriva con E14. Il codice lo dice esplicitamente — «pretenderne
l'icona significherebbe chiedere un disegno per qualcosa che nessuno può pianificare». L'icona si disegna
comunque (serve alla skill bar e alla wiki), ma **non è nel set obbligatorio finché l'azione non atterra**.

### 2.2 Ability degli eroi

Venti ability, quattro eroi. Prendono chiavi regolari sotto `Action`:

```text
UI.Icon.Action.Flux.ArcPulse
UI.Icon.Action.Riva.PressureJet
UI.Icon.Action.Bastion.Ram
UI.Icon.Action.Vektor.PassingBlade
```

Non sono nel set obbligatorio di `RequiredIconIds()`, che legge il catalogo **generico**. La composizione di
ciascuna è in [`04-regole-di-composizione.md`](04-regole-di-composizione.md) §3.

## 3. Status

Gli undici tag registrati sotto `Status.`, che sono la fonte giusta perché sono gli stessi che il gameplay
applica:

`Braced` · `Burning` · `Electrified` · `Exposed` · `Guarded` · `Marked` · `Obscured` · `Reveal` · `Root` ·
`Slow` · `Wet`

### 3.1 Il manifest sorgente ne sbaglia nove su quindici

Confronto meccanico fra i `UI.Icon.Status.*` del manifest del pack e i tag registrati:

| Esito | Voci |
|---|---|
| Coincidono | `Burning` `Electrified` `Guarded` `Marked` `Obscured` `Wet` — **6** |
| Nel manifest, **non registrati** | `Anchored` `Interrupted` `KO` `LowHealth` `Rooted` `Shielded` `Slowed` `Stunned` `Suppressed` — **9** |
| Registrati, **assenti dal manifest** | `Braced` `Exposed` `Reveal` `Root` `Slow` — **5** |

Due meritano attenzione perché sono quasi-omonimi e passerebbero una lettura distratta: il tag è
**`Status.Root`**, non `Rooted`; è **`Status.Slow`**, non `Slowed`. Un `IconId` con il nome sbagliato non
fallisce la validazione — passa il validator, perché il segmento di categoria è corretto — e semplicemente
**non risolve mai** a runtime.

Gli altri sette (`Shielded`, `Stunned`, `Interrupted`, `Suppressed`, `Anchored`, `LowHealth`, `KO`) non
corrispondono ad alcun tag: disegnarli oggi produce asset senza consumatore. `LowHealth` e `KO` sono
probabilmente stati dell'HUD, non tag di gioco — e in quel caso appartengono a `Identity`, non a `Status`.

## 4. Certainty e Identity — chiusa il 2026-08-12

> **Esito**: la v0.1 popola **quattro** categorie — `Identity`, `Action`, `Phase`, `Status`. Rettifica
> registrata in [D-031](../../../../decisions/RT_PDR_00_Decision_Log.md).
>
> **`Identity` entra.** Ha una fonte generativa come le altre tre: `URTHeroCatalogLibrary::GetHeroRoster()`
> costruisce il roster **in codice** — `MakeFlux()`, `MakeRiva()`, `MakeBastion()`, `MakeVektor()` — quindi
> gli `HeroId` sono enumerabili esattamente come gli `ActionId`. Quattro chiavi, e un consumatore reale: il
> team roster di [`progettazione-hud.md`](../../../../technical/progettazione-hud.md) §6.2.
>
> **`Certainty` esce.** Tre verifiche, tutte negative: `Confirmed` e `Predicted` non compaiono in alcuna
> forma nel codice; l'unico `Uncertain` è `ERTAwareness::Uncertain`, che appartiene all'asse **Knowledge**;
> e i tre stati sono già definiti come **modificatori di stile** da §16 dell'HUD e da
> [`05-certainty-states.md`](05-certainty-states.md) §2. Un'icona che dice «questo è confermato» non esiste:
> si rende l'oggetto in modo solido. A catalogo sarebbe stata una chiave che nessun widget può chiedere.
>
> **Conseguenza**: il set obbligatorio passa da 51 a **55** chiavi. La copertura dichiarata prima di questa
> decisione — 51/51, «100%» — era vera rispetto a una definizione che la decisione corregge.

Il testo che segue è la registrazione originale della discrepanza, tenuta per provenienza.

## 4-bis. Come si presentava

[D-031](../../../../decisions/RT_PDR_00_Decision_Log.md) dice che la v0.1 popola **cinque** categorie:
`Identity`, `Action`, `Phase`, `Status`, `Certainty`. `RequiredIconIds()` ne genera **tre**: `Phase`,
`Action`, `Status`.

`Identity` e `Certainty` non hanno una fonte generativa nel codice — non esiste un enum di fazioni, di ruoli
né di certezza. Quindi:

- non sono nel set obbligatorio, e il loro gate non fallisce se mancano;
- il manifest del pack non ne contiene **nessuna** in forma canonica: `Certainty` è sotto `UI.Style.*` (che
  il validator rifiuta), `Identity` è sparsa su `Target.*`, `Faction.*` e `Role.*` (segmenti inesistenti).

Le chiavi corrette sarebbero:

```text
UI.Icon.Certainty.Confirmed
UI.Icon.Certainty.Predicted
UI.Icon.Certainty.Uncertain

UI.Icon.Identity.Ally
UI.Icon.Identity.Enemy
UI.Icon.Identity.Self
UI.Icon.Identity.Flux        (+ Riva, Bastion, Vektor)
```

**Questa è una decisione aperta, non una proposta da applicare**: o si estende `RequiredIconIds()` con le due
categorie, o si corregge D-031 su quante categorie la v0.1 popola davvero. Le due fonti oggi non dicono la
stessa cosa, e scegliere per conto proprio significherebbe fissare una tassonomia in un documento sorgente.

> **Chiusa lo stesso giorno**, prima che esistesse un `DA_IconCatalog` — che era la scadenza dichiarata.
> L'esito è in §4; il blocco di sopra descrive la domanda com'era posta, e va letto al passato.

## 5. Fuori dalla v0.1

Le altre sette categorie — `Environment`, `MapInteraction`, `Information`, `Reaction`, `Coordination`,
`Warning`, `Objective` — appartengono a **E25** (`#265` epic, `#266`–`#269` checkpoint) in
[`roadmap-post-v0.1.md`](../../../../roadmap/roadmap-post-v0.1.md).

Il catalogo è **lo stesso**, non un secondo: cambia quanto ne è popolato. Un `IconId` di E25 disegnato oggi
non è sprecato, ma non entra nel gate della v0.1.

Attenzione ai due nomi che il manifest sorgente sbaglia e che sembrano innocui: la categoria è
**`MapInteraction`**, non `Map`; è **`Information`**, non `Intel`. Con il nome sbagliato l'ID non passa il
validator.
