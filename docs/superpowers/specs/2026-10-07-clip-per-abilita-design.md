# La clip per abilità — una chiave `ActionId` sopra il ruolo di presentazione

> **Statuto**: design **accettato in sessione** il 2026-10-07 (decisioni d'autore D1–D4, §0), **rivisto dal
> panel** lo stesso giorno in due giri (le modifiche sono incorporate e marcate `➕ rev.` e `➕ rev2.`) e
> **implementato** in [#3563](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3563) sul branch
> `issue/3563-clip-per-abilita` (blocco «Implementato» qui sotto), PR in apertura. La voce `PIE-CLIP-ABILITA` resta da
> eseguire. È il **terzo di quattro sotto-progetti** della richiesta d'autore
> *«associare animazioni e FX alle skill e vederle in azione»*: il primo è il banco Ability Lab → PIE
> ([#3532](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3532)), il secondo il momento
> `AbilityActivated` ([#3549](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3549), spec
> [`2026-10-07-momento-ability-activated-design.md`](2026-10-07-momento-ability-activated-design.md)), entrambi in `main`.
>
> **Stato misurato**: 2026-10-07, `origin/main` = `12bd4def4`. Ogni `file:riga` è stato letto su quel commit
> (ricognizione in sola lettura); chi lo rilegge più tardi lo **rimisura**. Nessun totale volatile: dove serve
> una misura c'è il comando. Le decisioni prese in sessione dal controller sono marcate `Ruling`, con il loro costo.
>
> ✅ **Implementato il 2026-10-08** sul branch `issue/3563-clip-per-abilita`, da `12bd4def4`, col piano
> [`2026-10-07-clip-per-abilita.md`](../plans/2026-10-07-clip-per-abilita.md): un commit per task, più i giri di
> review. Spec e piano `255696ef4`; il modello (`PerAction`, `ActiveClipFor` a tre livelli) `03754e78b`; il beat
> conosce l'azione `0b25a1d56`; la mappa di default `f4731c68d`; il catalogo JSON (`actionId`, `formatVersion` 2)
> `20360b730` e `96732a147`; commandlet e browser `fe5dfd8c4` e `193984837`; il gate di cook `637b0c02a` e
> `9ed05ef60`; registro PIE, seduta `U70`, runbook e questo statuto nel commit `docs(3563)` che porta questo
> blocco. Le correzioni emerse in implementazione sono marcate `➕ impl.` (§2.1, §2.2, §2.3) e
> `➕ misurato 2026-10-08.` (§2.5, §5.1, §6).
>
> **Gate**, ciascuno sul commit del proprio task, con il log nei report della sessione (non versionati): compile
> `PASS`; `RefactorTactics.Unit`, `RefactorTactics.Anim` e `RefactorTactics.Playback` `PASS` su `f4731c68d`
> (nessun `Result={Fail}`); `RefactorTactics.Anim` `PASS` su `96732a147` e sul contenuto di `193984837`;
> `RefactorTactics.Packaging` su `9ed05ef60`: `RequiredSetIncludesActionClips` `PASS`,
> `RequiredAnimationClipsAreCooked` `FAIL` dichiarato (qui sotto). Ogni test nuovo è stato visto rosso prima del
> codice, e ciascuna delle mutazioni (1)–(9) di §5.1 è caduta sul proprio asserto. `RefactorTactics.Unit` `PASS`
> anche sull'albero del commit `docs(3563)`. `NOT RUN`: la suite intera `RefactorTactics` sull'ultimo commit
> (`Playback` non è stato rilanciato dopo `f4731c68d`); la PIE
> (`PIE-CLIP-ABILITA`, seduta `U70`); il pacchetto. `N/A`: determinismo, replay e privacy — nessun dato nuovo in
> snapshot, TurnLog o `StateHash`, nessun tipo di evento nuovo.
>
> 🔴 **Il gate di cook era VERDE, e questa spec lo dava per rosso** (`Ruling` R12, 2026-10-08). §2.5 e §6
> davano `Packaging.RequiredAnimationClipsAreCooked` per già rosso sulle clip di ruolo, sulla misura del runbook
> del 2026-09-05. Rimisurato con `Automation RunTests RefactorTactics.Packaging`: su `193984837`, prima che il set
> richiesto includesse le clip d'azione, il gate è **verde** (`Result={Success}`, nessun package scoperto); su
> `9ed05ef60` è **rosso**, e ogni package scoperto ha la provenienza `Hero / Action / Ruolo` di una clip d'azione
> del default (§2.6), nessuno quella di un ruolo. ⚠️ **Il merge di #3563 rende quindi rosso il gate su `main`**,
> finché [#3562](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3562) non fa il gesto in Editor sui
> `BP_Unit_*`. ⛔ Non lo decide questa spec né chi l'ha implementata: la PR lo espone in testa, e l'autore sceglie
> al merge fra tre uscite — merge col rosso dichiarato; PR in attesa di #3562; un gate separato per le clip
> d'azione, rosso con owner #3562, mentre quello storico resta verde.
>
> **Follow-up candidates** emersi in implementazione: in §7, marcati `➕ impl.`.

---

## 0. Le decisioni d'autore (2026-10-07)

| # | Domanda | Decisione |
|---|---|---|
| D1 | Scope | **Asse dati + runtime**: chiave `ActionId` nel binding del catalogo JSON, nel commandlet e nel CDO (`PerAction` accanto a `PerRole`); `PlayPresentationRole(Role, ActionId)` con ripiego sul ruolo; gate estesi. Il **pannello di legame** nell'Anim Browser (eroe, ruolo, azione) resta un sotto-progetto a sé: oggi il pannello non ha nessuna UI di bind, nemmeno per ruolo. |
| D2 | Chiave e ripiego | **`(ActionId, Ruolo)` → `(BaseActionId, Ruolo)` → `Ruolo`**: il profilo, poi la generica condivisa fra eroi, poi la clip di ruolo di oggi. |
| D3 | Ruoli | **Solo `Cast` e `Attack`** propagano l'azione (`ShowActivation` → `Cast`, `LaunchPlaybackAttack` → `Attack`); `Hit` e `Death` restano per ruolo. |
| D4 | Default | **Mappa abilità→clip in C++** (`MakeClips`), proposta a tavolino dai nomi dei pack e **giudicata dall'autore** (poi a schermo col banco); il catalogo JSON può sovrascriverla. |

---

## 1. Il problema, misurato

**Una clip per (eroe, ruolo), mai per abilità.** `URTUnitAnimInstance::ClipsPerHero` è `TMap<FName, FRTHeroPresentationClips>`
(`Unit/RTUnitAnimInstance.h:184-185`); dentro, `PerRole: TMap<ERTPresentationRole, FRTAnimRoleClips>` (`:139-150`), ogni
ruolo con `Variants` e un solo `ActiveClipVariant` (`:72-83`). `ActiveClipFor(HeroId, Role)` (`:197`, `.cpp:168-183`) è
l'unico punto di risoluzione. Dal sotto-progetto 2 ogni abilità ha un **momento** (`AbilityActivated` → ruolo `Cast`) e
l'evento porta `ActionId`/`BaseActionId` (`Turn/RTResolvedEvent.h:387, 405`), ma i due consumatori lo scartano:
`ShowActivation` chiama `PlayPresentationRole(ERTPresentationRole::Cast)` (`Turn/RTTurnManager.cpp:8103`) e
`LaunchPlaybackAttack` chiama `PlayPresentationRole(ERTPresentationRole::Attack)` (`:8289`). Il default C++ lega la clip
`Cast` dei pack sia al ruolo `Attack` sia al ruolo `Cast` (`.cpp:57, 59`): ogni abilità di un eroe suona la stessa sequenza.

**Il dato d'autore esiste, l'asse no.** `Data/Anim/AnimCatalog.json` (`formatVersion: 1`) è letto e scritto da
`Unit/RTAnimCatalogLibrary.cpp` (parse `:394-426`, scrittura `:489-497`); `FRTAnimBinding { HeroId, Role, bActive }`
(`Unit/RTAnimCatalogTypes.h:102-123`); il commandlet `RTBuildAnimBindings` traduce i binding nel CDO di
`/Game/RT/Anim/ABP_RTUnitAuthored` (`RefactorTacticsEditor/Private/Content/RTBuildAnimBindingsCommandlet.cpp:28-63,
157-163`); `ValidateCatalog` difende «una sola attiva» con chiave `(HeroId, Role)` (`RTAnimCatalogLibrary.cpp:608-629`). Il
modello del browser ha `BindToRole`/`MakeActive`/`Unbind` con predicati su `(HeroId, Role)`
(`RTAnimBrowserModel.cpp:175-176, 198, 226`); il pannello non li chiama (`SRTAnimBrowserPanel.cpp`: solo
Promote/Candidate/Reject, `:200-222`). Il catalogo reale ha cinque voci del pack Gadget, tutte `Unreviewed`, **nessun binding**.

**Il percorso runtime è il default C++.** `ABP_RTUnitAuthored` non è versionato (`Content/RT/Anim` assente da
`git ls-files Content`, che versiona invece `Content/RT/Core`, `Maps`, `Art`, `Editor` e i `BP_Unit_*`). La
risoluzione legge il CDO di `UnitAnimClass` (`Unit/RTUnit.cpp:723-740`), che resta `URTUnitAnimInstance::StaticClass()`
(`:146`) finché i `BP_Unit_*` non lo impostano sulla classe autorata: senza quel gesto in Editor sui binari il catalogo
non suona. ➕ rev. Il gesto non ha più un owner: #2444 è **chiusa** dal 2026-09-05 con il DoD «gate di cook verde» non
spuntato; lo eredita [#3562](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3562) (aperta il 2026-10-07),
che possiede sia `UnitAnimClass` sia i riferimenti duri.

**UHT vieta i contenitori annidati** (`RTUnitAnimInstance.h:134-137`): una mappa per azione passa da una `USTRUCT` intermedia.

**I pack hanno le clip.** Misurato sul clone principale, per nome (`Content/FabAsset/Paragon/<Pack>/Characters/Heroes/<Pack>/Animations`,
cartella ignorata da git, `.gitignore:108`): Gadget ha `Ability_Q*`, `LMB_Fire_A/B/C*`, `Throw_Ready*`; Phase `Ability_E*`,
`Ability_Q*`, `Ability_R_Alt*`, `Primary_Attack_A/B/C/D_*`, `RMB_Throw`; Riktor `Ability_Hook_*`, `Ability_Lockdown`,
`Ability_ShockingPunch*`, `Ability_Ultimate*`, `PrimaryAttack_A/B/C_Slow*`; Wraith `Ability_E*`, `Ability_Q_Fire*`,
`Ability_R*`, `Ability_RMB_*`, `Fire_A_*`, `Fire_Snipe_A_Slow`. Il conteggio esatto si rimisura con
`Get-ChildItem <pack>/Animations -Filter *.uasset`.

---

## 2. Il disegno

### 2.1 Il modello dati (CDO)

```cpp
/** Le clip di UN'azione per ruolo di presentazione: la stessa forma di FRTHeroPresentationClips::PerRole. */
USTRUCT()
struct FRTActionPresentationClips
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, Category = "RT|Anim")
	TMap<ERTPresentationRole, FRTAnimRoleClips> PerRole;
};

// in FRTHeroPresentationClips, accanto a PerRole:
/** Clip per (ActionId, ruolo). Chiave: `ActionId` del catalogo (profilo `Hero.X.Y` o generica `Action.Z`). */
UPROPERTY(EditDefaultsOnly, Category = "RT|Anim")
TMap<FName, FRTActionPresentationClips> PerAction;
```

La struttura intermedia riusa `FRTAnimRoleClips`: varianti, `AddVariant`, `MakeActive` atomico, `FindActive`, senza
toccarli. Una clip di ruolo e una clip d'azione sono due pool distinti: «una sola attiva» vale **per pool**.

➕ impl. **Gli specifier sono quelli del file, non quelli del blocco qui sopra**: `USTRUCT(BlueprintType)` e
`UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")`, come `PerRole`
(`Unit/RTUnitAnimInstance.h:142-149` per la struct, `:176-177` per `PerAction`). La prima stesura scriveva
`USTRUCT()`, `EditDefaultsOnly` e `"RT|Anim"`: la forma, non gli specifier, era il contenuto di questa sezione.

**Alternativa scartata**: una `TMap<FName, FName> ActiveClipVariantPerAction` dentro `FRTAnimRoleClips` che scelga una
variante del pool di ruolo. Mescola varianti di ruolo e d'azione nello stesso pool e rende «una sola attiva» ambiguo.

### 2.2 La risoluzione (D2, D3)

```cpp
// URTUnitAnimInstance
TSoftObjectPtr<UAnimSequenceBase> ActiveClipFor(const FName& HeroId, ERTPresentationRole Role) const;           // oggi
TSoftObjectPtr<UAnimSequenceBase> ActiveClipFor(const FName& HeroId, ERTPresentationRole Role,
                                                const FName& ActionId, const FName& BaseActionId) const;      // nuovo
```

➕ impl. L'overload a quattro argomenti **non ha default** (`Unit/RTUnitAnimInstance.h:238-239`): chi chiama
`ActiveClipFor` passa sempre la terna, `NAME_None` compreso. I default stanno solo sulle due funzioni di `ARTUnit`
qui sotto, dove servono a far compilare invariati i chiamanti di oggi.

Ordine: `PerAction[ActionId].PerRole[Role].FindActive()` → `PerAction[BaseActionId].PerRole[Role].FindActive()` →
`PerRole[Role].FindActive()`. Ogni livello «non popolato» (azione assente, ruolo assente, nessuna variante attiva) passa
al successivo; il risultato nullo finale degrada come oggi (`IsNull`/`LoadSynchronous` a `RTUnit.cpp:746-747`, notifica
al BP con `nullptr` a `:766-779`). `ActionId`/`BaseActionId` vuoti saltano il proprio livello. `Ruling`: la seconda
chiave si legge dall'**evento** (`Ev.BaseActionId`), mai derivata a valle (`RTResolvedEvent.h:393-397` lo vieta); con
`BaseActionId == NAME_None` si passa al ruolo. ➕ rev. Fra le abilità degli eroi solo gli attacchi base hanno un
`BaseActionId` (`MakeHeroBasicAttack`, `Ability/RTHeroCatalogLibrary.cpp:168, 353, 496, 740, 928`): oggi il livello
generico è raggiungibile da quelli soltanto; vale per ogni abilità che un domani lo dichiarerà.

```cpp
// ARTUnit (non UFUNCTION: il default è lecito e i chiamanti esistenti compilano invariati)
TSoftObjectPtr<UAnimSequenceBase> ResolvedClipPathFor(ERTPresentationRole Ruolo,
    FName ActionId = NAME_None, FName BaseActionId = NAME_None) const;
void PlayPresentationRole(ERTPresentationRole Ruolo, FName ActionId = NAME_None, FName BaseActionId = NAME_None);
```

`PlayPresentationRole` risolve con la terna, carica e suona come oggi, e notifica il Blueprint con lo stesso `switch`:
`Play*Montage(UAnimSequenceBase*)` **non cambiano firma** (`Ruling`: nessun `BP_Unit_*` li implementa, ma il contratto
C++/BP e il censimento `BlueprintSurfaceIsCensused` restano intatti; l'`ActionId` arriverà al BP se e quando servirà).
Il seam `CastCuesPlayedForTest` conta ancora le chiamate; un seam gemello, ➕ rev. **per ruolo**,
`LastResolvedClipPathForTest(ERTPresentationRole)` (sotto `WITH_DEV_AUTOMATION_TESTS`), espone **quale** path ha
risolto l'ultima chiamata di quel ruolo: la stessa unità suona `Cast` e poi `Attack` nello stesso turno, e uno slot
unico mostrerebbe solo l'ultimo.

I due consumatori passano l'azione (D3): `ShowActivation` → `PlayPresentationRole(Cast, Ev.ActionId, Ev.BaseActionId)`;
`LaunchPlaybackAttack` → `PlayPresentationRole(Attack, Atk.ActionId, Atk.BaseActionId)`. `Hit` (`:8307`) e `Death`
(`:8999`, `:9162`) restano a un argomento.

### 2.3 Il catalogo JSON e il commandlet (D1)

- `FRTAnimBinding` guadagna `FName ActionId` (vuoto = binding di ruolo, come oggi). Parse e scrittura in
  `RTAnimCatalogLibrary.cpp` (`actionId` opzionale). `Ruling`: **`formatVersion` 2**. Il codice avverte che una build
  vecchia ignora i campi sconosciuti e legge un catalogo dimezzato senza accorgersene (`RTAnimCatalogTypes.h:210-217`):
  un `actionId` ignorato trasformerebbe un binding per azione in un binding di ruolo, cioè in una clip **sbagliata e
  attiva**. Il bump fa rifiutare il file alle build vecchie (`CurrentFormatVersion`, `:218`); il commento che
  giustificava il non-bump per i binding assenti (`RTAnimCatalogLibrary.cpp:394-395`) si aggiorna.
- ➕ rev. **Il writer scrive sempre `CurrentFormatVersion`** (oggi scrive `Catalog.FormatVersion`,
  `RTAnimCatalogLibrary.cpp:463`): un catalogo v1 a cui si aggiunge un `actionId` si risalva come v2, e il round-trip
  regge. Il reader rifiuta un `actionId` in un file dichiarato v1 (è il caso del file scritto da una build nuova e letto
  da una vecchia, o editato a mano). `Data/Anim/AnimCatalog.json` passa a `formatVersion: 2` nel commit.
- `ValidateCatalog`: la chiave di unicità diventa `(HeroId, Role, ActionId)`; `Ruling`: un `actionId` non vuoto che
  **non** è un'azione conosciuta è un **errore**, non un avviso — un refuso sarebbe un binding che non suona mai, e
  nessun test lo vedrebbe. ➕ rev. L'insieme delle azioni valide è **esplicito**: ogni `ActionId` del catalogo core,
  delle generiche (`MakeGenericActions`, `RTCatalogLibrary.h:613`), delle ambientali e di ogni abilità di ogni eroe
  (`URTHeroCatalogLibrary`, reazioni comprese), ➕ rev2. **e di ogni pezzo di equipaggiamento che concede un'azione**
  (`MakeEquipmentAction`, `RTCatalogLibrary.h:441`, riscrive `ActionId` con l'id del pezzo, es. `Gadget.Sprinkler`:
  quell'azione si pianifica e si attiva come le altre, quindi il suo `AbilityActivated` porta l'id del pezzo — `Ruling`
  R10: entra nell'insieme; costo se sbagliato: un binding per un gadget accettato ma mai suonato, visibile al banco),
  calcolato una volta per validazione (il costo di costruire il roster a ogni `ValidateCatalog` è accettato: la
  validazione gira nel commandlet e nei test, non nel gioco). ➕ rev. Un `actionId` su un ruolo che non propaga
  l'azione (`Idle`, `Move`, `Hit`, `Death`, `Dash`, `Defend`, `Fall`: D3) è lo stesso errore: non suonerebbe mai.
  ➕ rev2. Lo pinna il test `Anim.Catalog.RejectsActionIdOnNonPropagatingRole` (mutazione (9): il controllo del ruolo
  tolto → cade).
- `BuildClipsPerHero` nel commandlet: `FindOrAdd` su `PerAction[ActionId].PerRole[Role]` quando `ActionId` non è vuoto,
  altrimenti su `PerRole[Role]` come oggi; `AddVariant` + `MakeActive` invariati. ➕ rev. **Il commandlet oggi
  SOSTITUISCE l'intera mappa** (`Cdo->ClipsPerHero = PerEroe` in `Main`, `:163` su `12bd4def4`; ➕ impl. oggi
  `RTBuildAnimBindingsCommandlet.cpp:202`, la riga `Cdo->ClipsPerHero = URTBuildAnimBindingsCommandlet::MergeClipsPerHero(`): con la classe
  autorata cablata, un eroe senza binding perderebbe tutte le clip. `Ruling`: il commandlet **fonde per pool** — parte
  dal default C++ e, per ogni `(eroe, ruolo)` o `(eroe, azione, ruolo)` presente nel catalogo, sostituisce quel pool;
  eroi e pool senza binding tengono il default. È ciò che D4 chiama «il catalogo sovrascrive il default». Costo se
  sbagliato: un pool d'autore che non voleva il default lo eredita; si toglie con un binding vuoto esplicito (follow-up).
  ➕ rev2. **Come**: `BuildClipsPerHero` resta pura e con la semantica di oggi (il test `MapToCdo` asserisce che Ivrin
  **non** abbia il ruolo `Move` sulla sua uscita: deve restare vero). La fusione è una **seconda funzione statica
  pura**, `MergeClipsPerHero(const TMap<FName, FRTHeroPresentationClips>& Base, const TMap<…>& PerEroe)`, che ⌫ ~~`Run`~~
  ➕ impl. **`Main`** (`URTBuildAnimBindingsCommandlet::Main`: è lì l'assegnazione del CDO, e `Run` non esiste)
  chiama con `Base = URTUnitAnimInstance::StaticClass()->GetDefaultObject<URTUnitAnimInstance>()->ClipsPerHero` — il
  CDO della **classe base**, non quello della classe generata, che si porterebbe dietro la mappa della run precedente.
  Test proprio `Anim.Bindings.MergeKeepsDefaultPools`: un eroe senza binding tiene tutti i pool del default; un eroe con
  un binding `(Cast)` tiene `Move` del default e prende `Cast` dal catalogo; un pool d'azione del catalogo si aggiunge a
  `PerAction` senza toccare `PerRole`. Mutazione (8): `Merge` sostituita dall'assegnazione → cade. I commenti del
  commandlet `:126` («il catalogo possiede per intero `ClipsPerHero`») e `:155-156` si riscrivono: il catalogo possiede
  i **pool che nomina**.
- Il modello del browser: i predicati `B.HeroId == HeroId && B.Role == Role` sono ➕ rev. **quattro**: le tre lambda
  (`RTAnimBrowserModel.cpp:175, 199, 228`) e il ciclo atomico di `MakeActive` (`:205-214`). Tutti guadagnano
  `&& B.ActionId == ActionId`, con `ActionId` parametro opzionale (`NAME_None` = ruolo): senza il quarto, attivare un
  binding di ruolo spegnerebbe quelli d'azione dello stesso `(eroe, ruolo)`, e «una sola attiva per pool» si romperebbe.
  Così `Anim.Browser.BindingRules` resta vero e il pannello futuro ha l'API pronta. **Nessuna UI nuova** (D1).

### 2.4 Il default C++ (D4)

Il costruttore di `URTUnitAnimInstance` popola `PerAction` per le abilità dei kit del roster con la **mappa giudicata
dall'autore** (§2.6), una variante `AV_Roster` attiva per (abilità, beat), con i path dei pack nella forma di `:19`.
➕ rev. `MakeClips(Pack, Idle, Move, Attack, Hit, Death)` (`RTUnitAnimInstance.cpp:51-52`) non porta una tabella per
eroe: nasce un helper `MakeActionClips(Pack, { {ActionId, Role, Clip}, … })` chiamato nel costruttore accanto a
`MakeClips` (`:156-165`), una riga per voce. La clip di ruolo resta com'è (`Cast` = `Attack` = clip `Cast` del pack): è
il ripiego. Il catalogo JSON, quando arriverà ai Blueprint (#3562), **fonde per pool** sopra questo default (§2.3).
➕ rev. I commenti da aggiornare: il commento-trappola di `RTUnitAnimInstance.cpp:43-47` (il ruolo `Cast` suona, e
sopra di esso ci sono le clip per abilità), `Tests/RTUnitTests.cpp:611-612` («una clip diversa sul ruolo Cast è un
giudizio umano» — vero per il ruolo; per azione il giudizio è in §2.6), il commento di `ValidateCatalog`
(`RTAnimCatalogLibrary.cpp:599`, «(eroe, ruolo)» → terna), quelli del commandlet (`:96`, `:126`, `:155-156`) e
➕ rev2. il messaggio del gate di cook (`RTPackagingConfigTests.cpp:728`), che cita ancora `#2444`: l'owner è `#3562`.

### 2.5 I gate

- `Packaging.RequiredAnimationClipsAreCooked` (`Tests/RTPackagingConfigTests.cpp:541-739`): il set richiesto itera **anche**
  `PerAction` (ogni (eroe, azione, ruolo) con variante attiva), con la provenienza `Hero.X / Action / Ruolo`; il conteggio
  anti-sottrazione conta le **terne** di entrambi i pool in un ciclo indipendente. ➕ rev. Il calcolo del set richiesto
  esce in un helper (`RequiredAnimationPackages(Cdo, OutProvenienza)`) con un **test proprio, verde**
  (`Packaging.RequiredSetIncludesActionClips`): la mutazione «`PerAction` saltato» cade lì, non dentro un gate già rosso
  dove un altro fallimento si distingue solo dal messaggio. `Ruling`: ⌫ ~~il gate di cook è rosso per le clip di ruolo
  senza riferimento duro — misura **storica** del runbook (`docs/technical/runbooks/guida-animazioni-paragon.md:283-285`),
  da rimisurare nel piano; le clip per azione del default allargano il set e il rosso cresce.~~ ➕ misurato 2026-10-08.
  Il gate di cook era **verde** su `193984837` per le clip di ruolo (`Result={Success}`, nessun package scoperto: i
  `BP_Unit_*` versionati le referenziano già); il rosso **nasce** con le clip d'azione di questo sotto-progetto (su
  `9ed05ef60` ogni package scoperto ha la provenienza `Hero / Action / Ruolo`). La misura è il run
  `RefactorTactics.Packaging` prima e dopo il gate esteso, e si ripete col comando del runbook. Resta rosso finché i
  `BP_Unit_*` non referenziano le clip: owner [#3562](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3562)
  (eredita #2444, chiusa). Dichiarato in §6; il messaggio del gate elenca i package per nome.
- `Unit.CastRoleResolvesAClipForEveryHero` (`Tests/RTAnimChannelTests.cpp:313-330`): + «azione senza voce → ripiego sul ruolo».
- `Unit.DiscreteRoleClipsMatchThePacks` (`Tests/RTUnitTests.cpp:553-620`): invariato (pinna `Cast == Attack` sul **ruolo**).
- `Anim.Bindings.MapToCdo`, `Anim.Browser.BindingRules`, `Anim.Catalog.RejectsTwoActivePerRole` (`RefactorTacticsEditor/Private/Tests/RTAnimBrowserModelTests.cpp:167, 228, 277`): estesi alla chiave nuova. ➕ rev. Il round-trip dei binding vive in `RTAnimBrowserModelTests.cpp:43-48`, non in `Tests/RTAnimCatalogTests.cpp` (che non ha binding): il test con `actionId` è **nuovo**, accanto a quello.
- `Playback.ActivationPlaysTheCastCue` (`Tests/RTPlaybackActivationTests.cpp:124-148`): asserisce anche **quale** path.
- Non cambiano: `Unit.BlueprintSurfaceIsCensused` (nessuna UFUNCTION nuova né rinominata), D-278 (nessun tipo di evento nuovo).

### 2.6 La mappa abilità→clip (giudizio dell'autore)

Approvata dall'autore il 2026-10-07 («approvo così»), **scelta dai nomi dei pack** (nessuno le ha viste): il giudizio a
schermo (§5.2) può cambiare ogni riga con un commit di una riga. «ruolo» = resta la clip di ruolo (`Cast` del pack).
Le abilità di fase Environment (`ConductiveNode`, `MistVeil`) non hanno un beat in v0.1 (spec del momento §2.2: fuori
dagli intenti di Prep/Dash/Blast) e non ricevono clip; le reazioni non si attivano. L'impatto di una carica porta
l'`ActionId` dello scatto (`Impact.Def = Dash->Def`), quindi `Ram` e `PassingBlade` hanno un beat `Attack`. Path nella
forma `/Game/FabAsset/Paragon/Paragon<Pack>/Characters/Heroes/<Pack>/Animations/<Clip>.<Clip>` (`RTUnitAnimInstance.cpp:19`).

| Eroe (pack) | `ActionId` | natura | beat `Cast` | beat `Attack` |
|---|---|---|---|---|
| Aevik (Gadget) | `Hero.Aevik.ArcPulse` | attacco base (`Action.BasicAttack`) | ruolo | `LMB_Fire_A` |
| | `Hero.Aevik.LinearDischarge` | linea, Blast | `Ability_Q_Target` | `LMB_Fire_B` |
| | `Hero.Aevik.Overload` | area r1, Blast | `Throw_Ready` | `LMB_Fire_C` |
| | `Hero.Aevik.ConductiveNode` | Environment | nessun beat in v0.1 | — |
| | `Hero.Aevik.ReactiveCapacitor` | reazione | non suona | — |
| Muiren (Phase) | `Hero.Muiren.PressureJet` | attacco base | ruolo | `Primary_Attack_A_Medium` |
| | `Hero.Muiren.CircularTide` | cura ad area, Blast | `R_Ability_Intro` | — |
| | `Hero.Muiren.FluidTrail` | scatto (Dash) | `Ability_E` | — |
| | `Hero.Muiren.TideGuard` | scudo su sé, Prep | `Ability_R_Alt` | — |
| | `Hero.Muiren.FlowReaction` | inerte (slot `None`) | ruolo | — |
| | `Hero.Muiren.MistVeil` | Environment | nessun beat in v0.1 | — |
| Branth (Riktor) | `Hero.Branth.ImpactShot` | attacco base | ruolo | `PrimaryAttack_A_Slow` |
| | `Hero.Branth.KineticPanel` | copertura, Prep | `Ability_Lockdown` | — |
| | `Hero.Branth.Reconfigure` | sposta copertura, Prep | `Ability_Hook_Pull` | — |
| | `Hero.Branth.Ram` | carica (Dash) + impatto | `Ability_Hook_Start` | `Ability_ShockingPunch` |
| | `Hero.Branth.MortarShot` | area r1, Blast | `Ability_Hook_Cast` | `PrimaryAttack_B_Slow` |
| | `Hero.Branth.Interposition` | reazione | non suona | — |
| Ivrin (Wraith) | `Hero.Ivrin.PulseShot` | attacco base | ruolo | `Fire_A_Fast_V1` |
| | `Hero.Ivrin.InterceptShot` | predittiva, Prep | `Ability_E_Targeting_Start` | — (➕ rev. il colpo predittivo scrive solo una voce di TurnLog, nessun evento `Attack`: `RTTurnManager.cpp:7133-7136`; `Fire_Snipe_A_Slow` resta fuori dal set finché non avrà un consumatore) |
| | `Hero.Ivrin.PassingBlade` | scatto che attraversa (Dash) + colpo | `Ability_R_InMotion` | `Ability_Q_Fire_Fwd` |
| | `Hero.Ivrin.Feint` | controllo, Blast, nessun effetto | `Ability_E` | — |
| | `Hero.Ivrin.PhaseGuard` | scudo su sé, Prep | `Ability_RMB_Start` | — |
| | `Hero.Ivrin.Deflection` | reazione | non suona | — |

Da vedere per prime a schermo: `R_Ability_Intro` e `Ability_R_InMotion` (potrebbero essere lunghe o preludere a un loop).
Clip del pack che entrano nel set richiesto del cook (D-262), oltre a quelle di ruolo: Gadget `LMB_Fire_A/B/C`,
`Ability_Q_Target`, `Throw_Ready`; Phase `Primary_Attack_A_Medium`, `R_Ability_Intro`, `Ability_E`, `Ability_R_Alt`;
Riktor `PrimaryAttack_A_Slow`, `PrimaryAttack_B_Slow`, `Ability_Lockdown`, `Ability_Hook_Pull`, `Ability_Hook_Start`,
`Ability_Hook_Cast`, `Ability_ShockingPunch`; Wraith `Fire_A_Fast_V1`, `Ability_Q_Fire_Fwd`, `Ability_E_Targeting_Start`,
`Ability_R_InMotion`, `Ability_E`, `Ability_RMB_Start`. ➕ rev. **I nomi non si deducono** (`RTUnitAnimInstance.h:178-181`):
il piano allega l'elenco dei path misurati sul disco del clone principale con il comando che lo produce —
`Get-ChildItem "D:\Repositories\refactor-tactics-main\Content\FabAsset\Paragon\Paragon<Pack>\Characters\Heroes\<Pack>\Animations" -Filter *.uasset | Select-Object -ExpandProperty BaseName`
— e ogni path della mappa deve comparire in quell'elenco prima di entrare in `MakeActionClips` (un refuso e un
riferimento mancante sarebbero altrimenti indistinguibili nel gate di cook, `RTPackagingConfigTests.cpp:724-730`).

---

## 3. Fuori scope

- Il pannello di legame nell'Anim Browser (D1), il cablaggio di `UnitAnimClass`/`ABP_RTUnitAuthored` e i riferimenti
  duri nei `BP_Unit_*` (#3562), la promozione delle clip nel catalogo (è «giudizio umano», `RTAnimCatalogTypes.h:29-40`).
- L'`ActionId` come parametro dei `Play*Montage` Blueprint.
- Clip per `Hit`/`Death`; FX (sotto-progetto 4); un `CastShowSeconds` per abilità.
- Il momento delle reazioni (`ReactionResolved` non suona un ruolo).

---

## 4. Errori e degrado

| Caso | Comportamento |
|---|---|
| Azione senza clip (nessuna voce in `PerAction`, né per il profilo né per la generica) | Ripiego sul ruolo: suona com'oggi. |
| Clip d'azione legata ma nessuna variante attiva | Come «non popolata»: ripiego sul ruolo. |
| `BaseActionId` vuoto sull'evento | Si salta il secondo livello; nessuna derivazione a valle. |
| `actionId` nel JSON che non è un'azione del catalogo | `ValidateCatalog` rifiuta il catalogo con il nome dell'azione; il commandlet non genera. |
| Catalogo `formatVersion` 2 letto da una build vecchia | Rifiutato per versione (non letto a metà). |
| Catalogo `formatVersion` 1 che contiene un `actionId` (editato a mano, o scritto da una build nuova e riletto da una vecchia) | Il reader lo rifiuta: un `actionId` esiste solo da v2. Il writer scrive sempre `CurrentFormatVersion`, quindi un v1 che guadagna un `actionId` si risalva come v2. |
| Due binding attivi per `(eroe, ruolo, azione)` | Rifiutati da `ValidateCatalog`. |
| `actionId` su un ruolo che non propaga (`Move`, `Hit`, …) | Rifiutato da `ValidateCatalog` col nome dell'azione e del ruolo. |
| ➕ rev2. Catalogo **senza** nessun `actionId` risalvato da una build nuova | Diventa v2 comunque (il writer scrive sempre `CurrentFormatVersion`): le build vecchie lo rifiutano per versione. È voluto: una sola versione in circolazione, nessun file «v1 ma scritto da v2». |
| Catalogo v2 che il commandlet fonde sopra il default | Un eroe o un pool senza binding tiene il default C++; solo i pool nominati dal catalogo cambiano (§2.3). |
| Path del default che non esiste nel pack | `LoadSynchronous` nullo: nessuna clip, notifica BP con `nullptr`, la partita gioca; il gate di cook lo nomina. |

---

## 5. Verifica

### 5.1 Automation, headless

| Test | Asserisce |
|---|---|
| `Unit.ActionClipWinsOverRoleClip` | Con `PerAction[Hero.X.Y][Cast]` attiva, `ActiveClipFor(Hero, Cast, Hero.X.Y, Action.Z)` è il path d'azione; senza, è il path di ruolo (controllo positivo e ripiego nello stesso test). Mutazione: ordine dei livelli invertito → cade. |
| `Unit.GenericActionClipIsSharedAcrossHeroes` | `PerAction[Action.BasicAttack][Attack]` attiva su due eroi: `ActiveClipFor(H1, Attack, Hero.H1.Q, Action.BasicAttack)` e `(H2, …)` risolvono la generica quando il profilo non ha voce. |
| `Unit.BaseActionIdIsNeverDerived` | ➕ rev. **Con la generica popolata** (`PerAction[Action.BasicAttack][Attack]` attiva) e nessuna voce per il profilo: passando `BaseActionId = Action.BasicAttack` risolve la generica (controllo positivo); passando `NAME_None` risolve il **ruolo**, non la generica (non la indovina). |
| `Unit.DefaultActionClipsResolveForEveryKitAbility` | ➕ rev. Per ogni abilità del catalogo eroi (lista attesa = **funzione** di `URTHeroCatalogLibrary`, non letterale) legge `PerAction` **direttamente** sul CDO e confronta con la mappa di §2.6 (una seconda copia dichiarata nel test): le voci attese ci sono, con path **diverso** da quello di ruolo; le abilità senza voce (Environment, reazioni, `FlowReaction`, i `Cast` degli attacchi base) sono elencate come tali. Mutazione: una riga tolta da `MakeActionClips` → cade. Ogni ritocco a schermo tocca **due** righe (mappa e test): dichiarato. |
| `Anim.Catalog.ActionIdRoundTrips` · `RejectsUnknownActionId` · `RejectsTwoActivePerActionRole` · `FormatVersion2IsRequired` · ➕ rev2. `RejectsActionIdOnNonPropagatingRole` · `AcceptsEquipmentActionId` | Il JSON con `actionId` fa round-trip; un `actionId` ignoto è errore; due attive sulla stessa terna sono errore; `formatVersion: 1` con `actionId` è rifiutato; un `actionId` valido su `Move` è errore (mutazione (9)); `Gadget.Sprinkler` su `Cast` è accettato (R10, controllo positivo del primo). |
| `Anim.Bindings.MapToCdoPerAction` | `BuildClipsPerHero` mette un binding con `actionId` in `PerAction` e uno senza in `PerRole`; `MapToCdo` (esistente) resta vero: Ivrin senza `Move`. |
| ➕ rev2. `Anim.Bindings.MergeKeepsDefaultPools` | `MergeClipsPerHero(Base, PerEroe)`: eroe assente dal catalogo → tutti i pool del default; eroe con solo `Cast` → `Move` del default e `Cast` del catalogo; pool d'azione aggiunto senza toccare `PerRole`. Mutazione (8): `Merge` = assegnazione → cade. |
| `Anim.Browser.BindingRulesPerAction` | I predicati del modello distinguono `(Hero, Role)` da `(Hero, Role, ActionId)`. |
| `Playback.ActivationPlaysTheActionClip` | ➕ rev. La fixture di `RTPlaybackActivationTests.cpp` (Scudo `TideGuard`, Tiratore `ImpactShot`): dopo il playback `LastResolvedClipPathForTest(Cast)` dello **Scudo** è il path **d'azione** di `TideGuard` e `LastResolvedClipPathForTest(Attack)` del **Tiratore** è il path **d'azione** di `ImpactShot` (il `Cast` del Tiratore è «ruolo», non discrimina). ➕ rev2. `Ruling` R11: il test **inietta** path sintetici in `PerAction` sul CDO e li ripristina alla fine, invece di leggere i path del default di §2.6 — così prova la catena evento → `ActionId` → risoluzione senza dipendere dal Task 3, e la mappa non vive in una terza copia (§6 ne dichiara due). Che i path del default siano quelli giusti lo dice `Unit.DefaultActionClipsResolveForEveryKitAbility`; che suonino a schermo lo dice la seduta PIE (§5.2). Costo se sbagliato: nessun test headless lega «quale clip del pack» a «quale evento»; lo lega la PIE. Mutazione (3): `ShowActivation` torna a un argomento → cade sullo Scudo; mutazione (6): `LaunchPlaybackAttack` torna a un argomento → cade sul Tiratore. ➕ rev2. Nessun test di oggi su questa fixture asserisce un beat `Attack` (contano solo le cue `Cast`): il piano **mostra** che `LaunchPlaybackAttack` (`RTTurnManager.cpp:8256`) parte nella fixture — asserto positivo sul path `Attack` del Tiratore **prima** della mutazione — altrimenti la (6) è vacua e la fixture va estesa finché non parte. |
| `Packaging.RequiredSetIncludesActionClips` (nuovo, verde) + `RequiredAnimationClipsAreCooked` (esteso, ⌫ ~~ancora rosso~~ rosso **da qui**) | ➕ rev. Il nuovo test chiama l'helper `RequiredAnimationPackages` sul CDO e asserisce che le clip d'azione attive siano nel set con la provenienza `Hero / Action / Ruolo`, e che le terne coperte eguaglino quelle attese (mutazione (4): `PerAction` saltato → cade QUI). Il gate di cook usa lo stesso helper e ⌫ ~~resta ROSSO per i riferimenti duri mancanti (#3562): il suo rosso è dichiarato, non misura questo sotto-progetto~~ ➕ misurato 2026-10-08. **diventa** rosso con questo sotto-progetto: era verde su `193984837`, e i package che scopre su `9ed05ef60` sono le sole clip d'azione senza riferimento duro (#3562). Il rosso è dichiarato nello statuto (R12). |
| `Unit.CastRoleResolvesAClipForEveryHero` (esteso) · `DiscreteRoleClipsMatchThePacks` (invariato) · `BlueprintSurfaceIsCensused` (invariato) | Il ripiego e i contratti di oggi. |

🔴 Controlli di mutazione dichiarati: (1) ordine dei livelli invertito; (2) una riga del default tolta; (3) `ShowActivation`
a un argomento (cade sullo Scudo); (4) `PerAction` saltato nel set richiesto (cade nel test verde dell'helper);
(5) `ValidateCatalog` senza il controllo dell'azione ignota; (6) `LaunchPlaybackAttack` a un argomento (cade sul Tiratore);
(7) il quarto predicato di `MakeActive` senza `ActionId` → attivare un binding di ruolo spegne quello d'azione → cade
`Anim.Browser.BindingRulesPerAction`; ➕ rev2. (8) `MergeClipsPerHero` ridotta all'assegnazione → cade
`Anim.Bindings.MergeKeepsDefaultPools`; (9) `ValidateCatalog` senza il controllo del ruolo che non propaga → cade
`Anim.Catalog.RejectsActionIdOnNonPropagatingRole`.

### 5.2 Seduta PIE

Voce **`PIE-CLIP-ABILITA`**, da eseguire: dal banco Ability Lab (#3532), per ciascuna abilità con una clip di default **diversa**
dalla clip di ruolo, il beat di cast mostra **quella** clip e non la `Cast` del pack (binario: la posa è un'altra).
➕ impl. Lo stesso sul colpo, per le abilità con una clip di default sul beat `Attack` (gli attacchi base,
`Hero.Aevik.LinearDischarge`, `Hero.Aevik.Overload`, `Hero.Branth.MortarShot`); per `Hero.Branth.Ram` e
`Hero.Ivrin.PassingBlade` il colpo è l'impatto della carica, e mostra anch'esso la clip d'abilità.
Criterio per un'abilità senza voce: la `Cast` di prima. Il verdetto a schermo e il giudizio estetico sono dell'autore;
l'anteprima dell'Anim Browser (#2554) serve a vedere le clip prima di promuoverle.

---

## 6. Limiti dichiarati

- Il catalogo JSON **non arriva al gioco** finché i `BP_Unit_*` non impostano `UnitAnimClass` sulla classe autorata
  ([#3562](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3562), che eredita la chiusa #2444): in v0.1
  suona il default C++ di §2.4.
- ⌫ ~~Il gate di cook è rosso (misura storica del runbook, da rimisurare) e cresce con le clip d'azione, finché i
  riferimenti duri non ci sono (#3562).~~ ➕ misurato 2026-10-08. Il gate di cook era verde per le clip di ruolo (su
  `193984837`) ed è rosso per le sole clip d'azione di §2.6 (da `637b0c02a`), finché i `BP_Unit_*` non le
  referenziano (#3562). Il merge porta il rosso su `main`: statuto, R12.
- Il livello generico `(BaseActionId, Ruolo)` è raggiungibile oggi solo dagli attacchi base degli eroi del roster: le altre abilità
  degli eroi non dichiarano un `BaseActionId`.
- La mappa di default vive in due righe per voce — `MakeActionClips` e la lista attesa del test — e ogni ritocco a
  schermo le tocca entrambe.
- Un binding di ruolo attivo e uno d'azione attivo convivono per lo stesso `(eroe, ruolo)`: «una sola attiva» vale per pool.
- Le clip del default sono scelte **dai nomi** dei pack: il giudizio a schermo può cambiarle (è un `Set` di una riga).
- `Hit` e `Death` non conoscono l'azione.
- Il pannello del browser non lega: si lega modificando il JSON, validato dal commandlet.
- ➕ rev2. Ogni salvataggio del catalogo da una build nuova produce un file `formatVersion` 2, anche senza `actionId`:
  le build vecchie lo rifiutano. Una sola versione in circolazione, per scelta.
- ➕ rev2. La fusione del commandlet parte dal CDO della classe base `URTUnitAnimInstance`: una run precedente del
  commandlet non lascia traccia nel risultato. Un pool d'autore si toglie solo con una voce nel catalogo (follow-up:
  binding vuoto esplicito).

---

## 7. Follow-up candidates

- Il pannello di legame nell'Anim Browser (eroe, ruolo, azione) sopra il modello già pronto.
- `ActionId` come parametro dei `Play*Montage` Blueprint, se un BP vorrà distinguerli.
- Clip per `Hit` per abilità (reazione al colpo di una specifica azione).
- Uno `RTAnimScan` versionabile dei pack per non scegliere a tavolino.
- ➕ impl. Il binding vuoto esplicito, per togliere un pool del default dal catalogo (§2.3, §6).
- ➕ impl. `label`, `notes`, `hero` e `role` del catalogo si leggono con `TryGetStringField`, che converte un numero
  in testo: lo stesso rischio che per `actionId` è chiuso da un controllo di tipo (`Anim.Catalog.RejectsNonStringActionId`).
- ➕ impl. Un `actionId` non stringa in un file v1 dà l'errore di tipo invece di «esiste solo da 2»: il file è
  rifiutato comunque, col messaggio meno preciso.
- ➕ impl. Un test di appartenenza per generiche, armi e reazioni nell'insieme delle azioni conosciute
  (`RTAzioniConosciute`, `Unit/RTAnimCatalogLibrary.cpp`): oggi un catalogo che non vi entrasse ne resterebbe fuori in
  silenzio.
- ➕ impl. `LastResolvedClipPaths` in `Unit/RTUnit.h` è dichiarata anche fuori da `WITH_DEV_AUTOMATION_TESTS`, come
  `CastCuesPlayed`.
- ➕ impl. L'asserto sul `Cast` della carica in `Playback.ChargeImpactPlaysTheDashAttackClip`
  (`Tests/RTPlaybackActivationTests.cpp`) non ha una prova di mutazione.
- ➕ impl. La traccia `L0` di `LaunchPlaybackAttack` è scritta fuori da `if (AtkSrc)`: prova che la funzione è
  partita, non che abbia trovato la sorgente.
- ➕ impl. Due righe con la stessa (abilità, beat) in `MakeActionClips` si sovrascriverebbero in silenzio.
- ➕ impl. Un run `RefactorTactics.Anim` si è fermato una volta senza `TEST COMPLETE`, dopo l'avvio di
  `RosterMigrationKeepsPaths`, per una causa ignota; i run successivi sono completi. Da rilanciare alla chiusura; se
  si ripete, è un'issue.
