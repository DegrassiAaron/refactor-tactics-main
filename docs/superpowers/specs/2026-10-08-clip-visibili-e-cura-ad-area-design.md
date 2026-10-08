# Clip visibili e cura ad area — una clip additiva non entra nel montaggio, una cura d'eroe passa dalle cure

> **Statuto**: bozza del 2026-10-08, scritta dopo la seduta `U70` (`PIE-CLIP-ABILITA`) sulle decisioni d'autore D1–D4 (§0,
> prese lo stesso giorno con AskUserQuestion). Chiude due difetti aperti da quella seduta — [#3590](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3590)
> (le clip d'azione di Aevik e le `Hit` di tutti i pack sono additive: suonano e non si vedono) e
> [#3593](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3593) (`Hero.Muiren.CircularTide` non cura nessuno) —
> e prepara la **riconvocazione di `U70`** sui tre banchi clip di [#3592](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3592).
> È il seguito del terzo sotto-progetto della richiesta d'autore *«associare animazioni e FX alle skill e vederle in azione»*:
> la clip per abilità ([#3563](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3563), spec
> [`2026-10-07-clip-per-abilita-design.md`](2026-10-07-clip-per-abilita-design.md)), di cui corregge la tabella §2.6 (`⌫`
> in §2.6 qui sotto). ➕ rev. **Rivista dal panel il 2026-10-08** (`.superpowers/sp5-spec-panel.md`, verdetto *APPROVATA CON MODIFICHE*, finding
> F1–F11): le modifiche sono incorporate e marcate `➕ rev.`; il `Ruling R2` è ribaltato (`⌫`). **Approvata dall'autore** il
> 2026-10-08 (AskUserQuestion).
>
> ⌫ **Perimetro ridotto lo stesso giorno, dai fatti**: mentre la spec era in review, la sessione parallela ha chiuso
> [#3590](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3590) con la PR
> [#3595](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3595) (`main` = `1c5f7150a`): il predicato
> `RTClipIsAdditive` (risponde come il motore, F1 compreso), la guardia sui soli **gesti** (`Cast`, `Attack`) con ripiego al
> ruolo, la tabella di Gadget con le `_Slow_V1` e i cast tolti (D2), il test `Unit.DefaultGestureClipsAreNotAdditive` sulla
> tabella, e la hit-react **tenuta**: la review di quella PR ha trovato che l'autore l'ha vista sui quattro eroi
> (`PIE-AS4b`, seduta `U8`, 2026-09-28), quindi D3 è sciolta dall'evidenza e non da una misura nuova. Le parti di questa spec
> che #3595 copre — §1.1, §2.2, §2.3, §2.4, le righe di clip di §5.1, le scene 1, 2 e 4 di §5.2 — restano scritte come
> registro e sono marcate `⌫ #3595`; **non si implementano qui**. Resta in perimetro: §2.1 (la cura ad area, #3593), la
> riga FX di `CircularTide` (F4), R8 (il bot), §2.5, e la riconvocazione di `U70` che giudica Aevik su #3595 e Muiren su
> questa spec nella stessa seduta. Branch: `issue/3593-cura-ad-area`, worktree `rt-wt-sp5-clip`.
>
> **Stato misurato**: 2026-10-08, `origin/main` = `834f4ff18`. Ogni `file:riga` è stato letto su quel commit, in sola
> lettura, sul clone principale; chi lo rilegge più tardi lo **rimisura**. Nessun totale volatile: dove serve una misura
> c'è il comando, dove serve un elenco ci sono i nomi. Le scelte di chi scrive sono marcate `Ruling`, ognuna col suo
> costo se è sbagliata; sono revocabili dall'autore.

## 0. Le decisioni d'autore (2026-10-08) — non si riaprono

- **D1 — la cura deriva da `Action.Heal` e il percorso delle cure impara la forma `Area`.** `CircularTide` si
  costruisce con `MakeHeroActionFromCore(…, TEXT("Action.Heal"), …, Area, 1)` come `TideGuard` da `Action.Shield`;
  `CollectHealActions` cura ogni alleato vivo nel raggio. È la regola già scritta per `Gadget.Medkit` (#1443). Scartate:
  «il percorso delle cure accetta ogni azione Heal-only» (due modi di dire *sono una cura*) e «cura dal percorso
  d'attacco» (una cura fra i colpi, con scudo e facing pensati per il danno).
- **D2 — per Gadget gli attacchi prendono le varianti `_Slow_V1`, i cast tornano alla `Cast` del pack.** `ArcPulse` →
  `LMB_Fire_A_Slow_V1`, `LinearDischarge` → `LMB_Fire_B_Slow_V1`, `Overload` → `LMB_Fire_C_Slow_V1` (full-body, 0,9 s);
  le righe di cast `Ability_Q_Target` e `Throw_Ready` **escono** dalla tabella (nessuna variante full-body nel pack:
  ripiego al ruolo, R13 della spec sorella). Scartate le `_Fast_V1` (0,6 s) e la sola guardia a runtime senza tabella.
- **D3 — il ruolo `Hit` si misura prima di deciderlo.** ⌫ Sciolta da #3595 con l'evidenza di `PIE-AS4b` (`U8`): la hit-react additiva si vede e resta. Le hit-react dei quattro pack sono additive e le sole full-body
  sono `KnockBack` (~2 s) e `Stun_*`. La guardia a runtime di questa spec **non** tocca `Hit`: nella riconvocazione di `U70`
  una scena con un colpo su una mesh dice se l'additiva sommata all'idle si legge. La decisione (esentare, togliere il
  ruolo in v0.1, o `KnockBack`) è un follow-up (§7).
- **D4 — flusso completo**: spec, panel, piano, esecuzione con subagenti, in un solo sotto-progetto per i due difetti;
  la riconvocazione di `U70` chiude.

## 1. Il problema, misurato

### 1.1 Le clip additive (#3590)

Seduta `U70` su `main` = `fa4b90001`, camera stretta su Aevik in `Visual.Combat.WaterElectricCoordinated`: all'`Attiva: …
LinearDischarge` e ai due `Colpo:` la posa **non cambia**, mentre tracer e numeri arrivano. Il log verbose non porta
nessuna riga `Clip d'azione non caricata`: la clip **si carica e viene suonata**. Sonda Python nell'Editor
(`unreal.load_asset(path).get_editor_property("additive_anim_type")` su ogni path della tabella):

| Eroe (pack) | clip d'azione in `MakeActionClips` (`Unit/RTUnitAnimInstance.cpp:192-227`) | tipo |
|---|---|---|
| Aevik (Gadget) | `LMB_Fire_A`, `Ability_Q_Target` (0,17 s), `LMB_Fire_B`, `Throw_Ready`, `LMB_Fire_C` | **tutte `AAT_LocalSpaceBase`** |
| Muiren (Phase) | `Primary_Attack_A_Medium`, `R_Ability_Intro`, `Ability_E`, `Ability_R_Alt` | full-body |
| Branth (Riktor) | `PrimaryAttack_A_Slow`, `PrimaryAttack_B_Slow`, `Ability_Lockdown`, `Ability_Hook_Pull`, `Ability_Hook_Start`, `Ability_Hook_Cast`, `Ability_ShockingPunch` | full-body |
| Ivrin (Wraith) | `Fire_A_Fast_V1`, `Ability_Q_Fire_Fwd`, `Ability_E_Targeting_Start`, `Ability_R_InMotion`, `Ability_E`, `Ability_RMB_Start` | full-body |
| ruolo `Hit` (`MakeClips`) | Gadget `Hitreact_Fwd`, Phase `HitReact_Fwd`, Riktor `HitReact_Front`, Wraith `HitReact_Front` | **additive** |
| ruoli `Idle`, `Move`, `Cast`, `Death` (`MakeClips`, ➕ rev. F10) | Gadget `Idle`/`Run_Fwd`/`Cast`/`Death_Fwd`, Phase `Idle`/`Jog_Fwd`/`Cast`/`Death`, Riktor `Idle`/`Jog_Fwd`/`Cast`/`Death_Fwd`, Wraith `Idle_NonCombat`/`Jog_Fwd`/`Cast`/`Death_Forward` | full-body (sonda `PROBEROLE` della stessa seduta) |

Perché non si vede: `ARTUnit::PlayPresentationRole` (`Unit/RTUnit.cpp:744-812`) suona la sequenza con
`PlaySlotAnimationAsDynamicMontage` (`:790`), che la avvolge com'è (`Engine/Private/Animation/AnimMontage.cpp`,
`CreateSlotAnimationAsDynamicMontage`); lo slot separa i montaggi additivi dai pieni e somma i primi come **delta** sulla
posa sorgente (`AnimInstanceProxy.cpp`, `SlotEvaluatePose`, «Split this in an additive and non additive list»). Le
additive di Gadget sono autorate contro una posa di mira, non contro `Idle`: sommate all'`Idle` del grafo non cambiano la
posa in modo leggibile. Nel pack Gadget le varianti full-body esistono per gli attacchi (`LMB_Fire_A_Slow_V1`,
`LMB_Fire_B_Fast_V1`/`_Slow_V1`, `LMB_Fire_C_Fast_V1`/`_Slow_V1`) e non per i cast (`Ability_Q*`, `Throw_Ready*` sono
tutte additive).

Perché nessun test lo vede: `Unit.DefaultActionClipsResolveForEveryKitAbility` (`Tests/RTAnimChannelTests.cpp`) verifica
che ogni path **risolva**; i gate di cook (`Tests/RTPackagingConfigTests.cpp`) che sia **raggiungibile**. Nessuno chiede
se la sequenza sia additiva, e il runtime non lo chiede: `UAnimSequence::IsValidAdditive()` (`AnimSequence.cpp:2638-2663`: vero quando `AdditiveAnimType != AAT_None` **e**
`RefPoseType` descrive una base valida — `ABPT_RefPose`, o `ABPT_AnimScaled`/`ABPT_AnimFrame` con `RefPoseSeq`, o
`ABPT_LocalAnimFrame` con un `RefFrameIndex`; nel `default` risponde falso) esiste e non viene letto. ➕ rev. F1: è lo
stesso predicato con cui lo slot separa gli additivi (`FAnimTrack::IsAdditive`, `AnimCompositeBase.cpp:333-345`), ed è
per questo che la guardia di §2.2 lo chiama e non ricostruisce la regola da `AdditiveAnimType`. La spec sorella §2.6 lo dichiarava: *«scelta dai
nomi dei pack (nessuno le ha viste)»*.

### 1.2 La cura che non arriva (#3593)

`Visual.Ability.ClipMuiren` (#3592), headless: Aevik alleata a 60 HP dentro il raggio 1 di `CircularTide` resta a 60, con
`targetCell` sulla sua cella e con `targetCell` sulla cella vuota accanto. Il log porta `Attiva: … CircularTide` e nessun
`+18 salute`.

Perché: le cure passano da `CollectHealActions` (`Turn/RTTurnManager_Blast.cpp:494-608`), che raccoglie **solo** le azioni
per cui `IsCoreAction(Heal->Def, ActionHeal)` (`:222-225`: `ActionId == Core || DerivedFromActionId == Core`) e cura **un**
bersaglio (`PlannedAttackTarget`, o chi la pianifica; `:524`). `CircularTide` è costruita da zero con `MakeHeroAction`
(`Ability/RTHeroCatalogLibrary.cpp:518-522`: fase `Attack`, priorità 60, portata 4, cooldown 2, `Area` raggio 1, un solo
effetto `Heal 18`) e non deriva da niente: finisce in `CollectAttackIntents`, dove un'area non colpisce gli alleati
(`bFriendlyFire`) e l'effetto `Heal` non ha un consumatore. È il meccanismo di #1443 (`Gadget.Medkit` «curava zero»),
chiuso lì guardando `DerivedFromActionId`; qui il campo è vuoto per dichiarazione
(`Tests/RTHeroCatalogTests.cpp:628-672`: *«otto abilità non ereditano da nessuna azione core, e restano vuote:
`LinearDischarge`, `Overload`, `CircularTide`, …»*). `Heroes.Phase.TideHealsWithoutWetting` (`Tests/RTHeroMuirenTests.cpp`)
verifica la **dichiarazione** — *«Qui si verifica la dichiarazione, non l'applicazione»* — e resta verde. Le varianti
`Hero.Muiren.CircularTide.Healing` (cura 24) e `.Impact` (cura 10 + spinta 1) ereditano il difetto.

## 2. Il disegno

### 2.1 La cura ad area (D1)

**Catalogo.** `CircularTide` diventa
`MakeHeroActionFromCore(TEXT("Hero.Muiren.CircularTide"), TEXT("Action.Heal"), /*Cooldown*/ 2, ERTAbilityShape::Area, /*AreaRadius*/ 1)`
(`RTHeroCatalogLibrary.cpp:1051-1066` copia fase, priorità, portata, fallback ed effetti del core, e scrive
`DerivedFromActionId`, `bSelfTarget`, `LineOfSightPolicy`). `Action.Heal` (`Ability/RTCatalogLibrary.cpp:1834-1836`) porta
fase `Attack`, priorità 70, portata 3, cooldown 1, `Heal 20`: i numeri **dell'eroe** restano quelli di oggi e si
riscrivono dopo la derivazione, come `FluidTrail` fa con lo slot — ➕ rev. F5, **elenco chiuso**: `Def.RangeCells = 4` con lo specchio legacy `RangeCells`, `Def.Priority = 60`,
`Def.Effects = { Heal 18 }`. **Niente altro**: `Power` resta 0 — viene dal solo `Damage`
(`RTHeroCatalogLibrary.cpp:119-122`) e il bot lo legge come danno (`Bot/RTBotPlanningLibrary.cpp:856-858`); `Fallback`
resta quello del core (`Cancel`: nessuno lo legge sul percorso delle cure) ed entra nell'asserto di
`TideDerivesFromHeal` così com'è. `MakeHeroActionFromCore` può restituire `nullptr`: l'azione si aggiunge all'**indice 1**
— `Muiren->Actions[1]` è indirizzato per indice dal catalogo (`:677-678`) e dai test (`Tests/RTHeroMuirenTests.cpp:137`,
`:173`) — dentro un `if` con `check`, come il catalogo fa per le altre derivate.
`Ruling R1`: la derivazione dice *da quale percorso passa*, non *con quali numeri* — costo se è sbagliata: tre numeri in
più da tenere allineati a mano, pinnati da `TideHealsWithoutWetting`, che resta verde così com'è. Le due varianti non
cambiano. `Tests/RTHeroCatalogTests.cpp:628-672` aggiunge la riga `{ Hero.Muiren.CircularTide, Action.Heal }` e il
commento passa da «otto» a «sette» proprie: `LinearDischarge`, `Overload`, `Reconfigure`, `FlowReaction`, `InterceptShot`,
`PassingBlade`, `Feint`; ➕ rev. F5: i conteggi scritti a `:617` («undici») e `:665` («Undici derivate + otto proprie»)
diventano «dodici» e «dodici derivate + sette proprie».

➕ rev. F11 — **gli altri consumatori di `DerivedFromActionId`**, che la derivazione tocca senza che nessuno glielo chieda:
il ripiego d'icona (`UI/RTIconLibrary.cpp:131-138`) cade su `Action.Heal` se `CircularTide` non ha un glifo proprio; il
generatore HUD (`tools/hud-assets/generate_hud_assets.py:2096-2105`) toglie dai glifi richiesti ogni abilità costruita
con una fabbrica `FromCore`, e `tools/hud-assets/action_axes.py:269-287` legge fase ed effetti **del core** quando
l'eroe li riscrive dopo la chiamata (qui: `Heal 20` invece di 18). Il piano rilancia il generatore e il gate delle icone
(`docs/technical/runbooks/guida-catalogo-icone.md`) e confronta `Chiavi richieste` prima e dopo; se `action_axes` sbaglia
l'effetto, la riscrittura va dichiarata nel sorgente in una forma che il suo parser legge, o il suo output va corretto
nella stessa PR.

**Percorso delle cure.** `CollectHealActions` impara la forma. Oggi: un bersaglio (`PlannedAttackTarget` o sé), controllo
di portata, `Ctx.AddHeal` (`Turn/RTBlastContext.h:237`, cinque array paralleli), poi `ApplyPlannedHeals`
(`Turn/RTTurnManager.cpp:2232-2320`): `Min(MaxHealth, Health + Amount)`, voce `Combat/Healed` con `Amount` = quanto
curato davvero, evento `+N salute`. Con `Shape == Area`:

1. il **centro** è `PlannedAttackCell` se `bAttackTargetsCell`, altrimenti la cella del bersaglio pianificato, altrimenti
   la cella di chi cura (`Unit/RTUnit.h:306-392`: i due canali sono mutuamente esclusivi, `#2884`); ➕ rev. F8: centro e
   bersaglio si leggono **prima** di `ClearPlannedAttack()` (`Turn/RTTurnManager_Blast.cpp:530`), che oggi gira prima del
   controllo di portata e azzererebbe `bAttackTargetsCell`;
2. la portata si misura **sul centro** (`HexDistance(Unit->Cell, Centro) > RangeCells` → `MakeSupportFallback(OutOfRange)`,
   come oggi);
3. i **destinatari** sono le unità della squadra di chi cura — chi cura compresa — con `HexDistance(Centro, U->Cell) <=
   AreaRadius`, nell'ordine di `Ctx.Units` (➕ rev. F3: è già l'ordine totale canonico, cella per prima,
   `Turn/RTTurnManager_Blast.cpp:281` e `Turn/RTActionQueueLibrary.cpp:154`; un secondo ordinamento sarebbe una seconda
   regola accanto alla sede unica di `#2922`). Un alleato **morto** nell'area entra come oggi un bersaglio morto di `Single`:
   `ApplyPlannedHeals` scrive `Fallback/TargetDead` (`Turn/RTTurnManager.cpp:2245-2290`) e non lo cura; non si filtra prima;
4. per ciascuna, `Ctx.AddHeal(Unit, U, Amount, Unit->Cell, Def)`: una voce `Healed` per alleato, con il suo `Amount`
   reale. ⌫ `Ruling R2` (➕ rev. F2): un'area **senza nessuno** della squadra dentro è un **esito**, non una mira
   impossibile — gli alleati possono essere usciti nel Dash ([D-200]): l'azione **parte**, il cooldown si paga
   (`MarkAbilitySpent`, `:563`), l'attivazione si emette (punto 5), e il TurnLog riceve **una** voce
   `MakeSupportFallback(…, NoEffect)` con `TgtCell` = centro scritto a mano (`MakeSupportFallback` con bersaglio nullo
   metterebbe `SrcCell`, `:241-255`). Non `TargetGone`, che in questo percorso significa già «l'Actor è stato distrutto fra
   raccolta e applicazione» (`Turn/RTTurnManager.cpp:2275-2278`) ed entra nell'hash come `Amount`; costo se è sbagliato:
   una cura a vuoto che costa un cooldown, che è ciò che un giocatore si aspetta da un'azione partita;
5. `EmitAbilityActivated` **una** volta, con `ERTAbilityShape::Area` e il centro (oggi `:557-558`: `Single` e la cella del
   bersaglio) e ➕ rev. F8 `TargetStableUnitId` = quello del bersaglio pianificato se c'è, altrimenti `0` (il valore che
   l'`AbilityActivated` di un'area mirata a cella porta già); nessuna `AttackFootprint` (è un'impronta d'attacco: §6).
   ➕ rev. F4 — **`CONTRACT CONFLICT` con la spec del profilo FX**, deciso qui: la riga d'override approvata
   `Hero.Muiren.CircularTide` = `Pulse / None / Marker / AreaPulse` (`Turn/RTPresentationBinding.cpp:498`, gemella in
   `Tests/RTAbilityFxProfileTests.cpp:177`) assegna a questa abilità il `Marker` e l'onda d'area, che arrivano **solo** da
   un colpo (`RTPresentationBinding.cpp:36-39`): sul percorso delle cure sono dato morto. La riga diventa
   `Pulse / None / None / None` nella stessa PR, con la gemella nel test e una riga `⌫` nella spec del profilo FX §2.2; al
   cast si vede il `Pulse` (due esagoni che si stringono), che è ciò che §5.2 scena 3 giudica. `Ruling R7`: niente onda
   per le cure in questa spec — un'impronta di supporto è il follow-up §7; costo: una cura ad area senza segno sulle
   celle curate, letta dai numeri verdi;
6. l'**amount** viene dalla variante attiva se ne dichiara uno (`Ability->FindVariant(Unit->ActiveVariantId)` →
   `Effects`, primo `Heal`), altrimenti da `Def.Effects` — oggi `CollectHealActions` legge solo `Def` (`:571-575`) e
   `Healing` (24) non arriverebbe mai; `Ruling R3`: la `Push 1` di `Impact` **non** passa dalle cure (§6, follow-up §7),
   costo: la variante cura 10 e non spinge, finché qualcuno non decide dove vive una spinta curativa.

`Single` resta com'è: `Medkit` e `Action.Heal` non cambiano una riga di esito (`Equipment.MedkitHealsInMatch`,
`Tests/RTEquipmentTests.cpp:576`, è il controllo di regressione).

**Determinismo.** Le voci `Healed` entrano nel TurnLog, `Health` entra nello `StateHash`: l'esito di una partita con
`CircularTide` **cambia per disegno** (prima non curava). Nessuno scenario del corpus la usa fuori da
`Visual.Ability.ClipMuiren` (`grep -rl CircularTide Scenarios/` → solo quello), quindi nessun golden cambia; l'`expect`
di Aevik torna `78` (`60 + 18`) e la `_nota_curare` perde il rimando a #3593.

### 2.2 La guardia a runtime (#3590) — ⌫ #3595

⌫ **Implementata da #3595** (`Unit/RTUnit.cpp`, `Unit/RTUnitAnimInstance.{h,cpp}`: `RTClipIsAdditive`, guardia sui gesti). Testo tenuto come registro del disegno proposto.

In `PlayPresentationRole`, dopo il caricamento (`Unit/RTUnit.cpp:751`) e **prima** del montaggio (`:790`):

```cpp
// Una clip ADDITIVA non entra nel montaggio dinamico: lo slot la sommerebbe come delta all'idle e a schermo non
// si vedrebbe niente (#3590, misurato nella seduta U70). Vale per i beat che questa tabella possiede, Cast e Attack;
// Hit e Death restano fuori finché la seduta non li ha misurati (D3).
if (Sequenza != nullptr && RefusesAdditive(Ruolo) && !URTUnitAnimInstance::IsPlayableAsMontage(Sequenza))
{
    UE_LOG(LogRT, Warning, TEXT("[RT] Clip additiva rifiutata (%s, %s): ripiego sulla clip di ruolo"), ...);
    Sequenza = <clip di ruolo, se diversa e a sua volta playable; altrimenti nullptr>;
    bRipiegoAdditivo = true;
}
```

- `URTUnitAnimInstance::IsPlayableAsMontage(const UAnimSequenceBase*)` è una funzione **pura**, statica, in
  `Unit/RTUnitAnimInstance.{h,cpp}`: falsa per `nullptr` e per `IsValidAdditive()` vero, vera altrimenti. `Ruling R4`: vive
  sull'AnimInstance e non su `ARTUnit`, perché `Unit.BlueprintSurfaceIsCensused` non deve cambiare e perché è l'AnimInstance
  a possedere il grafo che decide come una clip viene valutata; costo: un include in più nei test.
- `RefusesAdditive(Ruolo)` è `Ruolo == Cast || Ruolo == Attack` (D3): una funzione pura accanto all'altra, così il
  giorno in cui `Hit` entra cambia una riga.
- Il ripiego riusa la clip di **ruolo** dell'eroe (`Defaults->ActiveClipFor(HeroId, Ruolo)`, come il ripiego al
  caricamento di `:757-776`): se è diversa dal path rifiutato e a sua volta playable, si carica e si suona; altrimenti
  `Sequenza = nullptr` e il Blueprint riceve `nullptr` come oggi (`:794-811`). Nessun nuovo `Play*Montage`, nessuna
  `UPROPERTY`.
- Un **seam** di misura accanto a `LastClipLoadFellBackToRole` (➕ rev. F9: i vicini sono membri **incondizionati**
  dell'header, `Unit/RTUnit.h:1371-1407`; solo le scritture nel `.cpp` stanno sotto `WITH_DEV_AUTOMATION_TESTS`, `:753`,
  `:779` — stessa forma): `LastClipRefusedAsAdditive.Add(Ruolo, bRipiegoAdditivo)` e il lettore
  `LastClipRefusedAsAdditiveForTest(Ruolo)`; più un **ingresso** di prova `ForcedClipForTest`, un
  `TStrongObjectPtr<UAnimSequenceBase>` (**non** `TObjectPtr`: non riflesso, non terrebbe vivo un `NewObject` fino alla
  chiamata), letto in `PlayPresentationRole` sotto lo stesso `#if` al posto di `LoadSynchronous` quando è impostato. Il
  confronto «diversa dal path rifiutato» si fa con il path **effettivamente caricato**: dopo il ripiego al caricamento
  (`:757-776`) è `PathRuolo`, non `Path`.
  `Ruling R5`: senza l'ingresso un test headless non può far passare un'additiva dal percorso vero — i pack sono
  gitignorati e in un worktree nessun path risolve — e un test che chiama solo la funzione pura sarebbe verde con la
  guardia tolta; costo: un membro di prova in più su `ARTUnit`, non censito perché non è `UPROPERTY`.
- Il log è `Warning`, una riga per chiamata: in PIE si legge nel feed di `LogRT`; nessun `ensure` (un asset sbagliato non
  è un invariante rotto).

### 2.3 La tabella di Gadget (D2) — ⌫ #3595

⌫ **Fatta da #3595**, che ha anche annotato la spec sorella §2.6 e il runbook delle animazioni.

`MakeActionClips(TEXT("Gadget"), …)` (`Unit/RTUnitAnimInstance.cpp:193-198`) diventa:

| `ActionId` | beat | prima | **dopo** |
|---|---|---|---|
| `Hero.Aevik.ArcPulse` | Attack | `LMB_Fire_A` (additiva) | `LMB_Fire_A_Slow_V1` |
| `Hero.Aevik.LinearDischarge` | Cast | `Ability_Q_Target` (additiva) | **riga tolta** → `Cast` del pack (R13) |
| `Hero.Aevik.LinearDischarge` | Attack | `LMB_Fire_B` (additiva) | `LMB_Fire_B_Slow_V1` |
| `Hero.Aevik.Overload` | Cast | `Throw_Ready` (additiva) | **riga tolta** → `Cast` del pack |
| `Hero.Aevik.Overload` | Attack | `LMB_Fire_C` (additiva) | `LMB_Fire_C_Slow_V1` |

La **seconda copia dichiarata** della tabella (`Tests/RTAnimChannelTests.cpp:168-172`, `ClipAtteseDefault`) cambia con
lei: `nullptr` sui due cast. I tre path nuovi sono nell'elenco del disco misurato in `U70` (`PROBEPACK Gadget
LMB_Fire_A_Slow_V1 full len=0.90`, `…_B_Slow_V1 full len=0.90`, `…_C_Slow_V1 full len=0.90`): entrano nel set richiesto
del cook (D-262) al posto dei cinque che escono; `Packaging.RequiredActionClipsAreCooked` resta rosso per scelta di #3562
e il suo elenco di scoperte cambia nomi, non stato. La spec sorella §2.6 riceve il blocco `⌫` con queste cinque righe e
la riga *«scelte dai nomi dei pack (nessuno le ha viste)»* riceve il `➕ rev.` che dice **come** si guarda prima di
scegliere (§2.5).

### 2.4 Il test sulla tabella — ⌫ #3595

⌫ **Coperto da `Unit.DefaultGestureClipsAreNotAdditive` di #3595** (sui gesti, non su `Hit`, che resta per evidenza).

`Unit.DeclaredClipsAreNotAdditive` (`EditorContext`, `Tests/RTAnimChannelTests.cpp`): per ogni eroe del CDO di
`URTUnitAnimInstance`, per ogni voce di `MakeClips` (`Idle`, `Move`, `Cast`, `Death` — **non** `Hit`, D3) e di
`MakeActionClips`, `LoadSynchronous()` sul path; se **risolve**, `TestTrue(IsPlayableAsMontage)`; se non risolve, ➕ rev. F10: `N/A` (una riga
`AddInfo`) **solo** se manca la cartella del pack (`Content/FabAsset/Paragon/Paragon<Pack>/`); se il pack c'è e il path
non risolve è un **errore** (un nome sbagliato in tabella); se le voci misurate sono zero, `AddWarning`. `Ruling R6`: il test vale dove i pack ci sono (clone
principale) e tace dove non ci sono (worktree, CI assente per scelta), invece di essere rosso per un gitignore; costo:
un verde da worktree non dice niente su questa domanda, e il piano lo esegue **sul clone principale** una volta, con il
log allegato. Un contatore di `N/A` in `AddInfo` dice quante voci ha davvero misurato: un run con tutte `N/A` non è un
gate.

### 2.5 Documenti

- Spec sorella `2026-10-07-clip-per-abilita-design.md` §2.6: `⌫` sulle cinque righe di Gadget (§2.3) e `➕ rev.` sulla
  regola di scelta: *«prima di dichiarare una clip, sonda l'asset: `additive_anim_type` deve essere `AAT_None`»*, con
  il comando Python della seduta.
- `docs/technical/test-manuali-pie.md`, voce `PIE-CLIP-ABILITA`: nota in coda alla cella (**non** un nuovo stato): dopo
  questa spec Aevik ha clip full-body sugli attacchi e la `Cast` sui cast; da rigiudicare nella riconvocazione di `U70`.
  Lo stato resta 🟡 finché la seduta non lo cambia.
- `docs/roadmap/editor-sessions.yaml`, `U70`: la riconvocazione dichiarata, con le scene (§5.2).
- Issue #3590 e #3593: chiuse dalla PR con il DoD nel commento; #3593 cita l'`expect` di `ClipMuiren` tornato a 78.
- Spec del profilo FX `2026-10-08-profilo-fx-per-abilita-design.md` §2.2: `⌫` sulla riga di `CircularTide` (F4).
- ➕ rev. F11: generatore HUD e gate delle icone rieseguiti (§2.1), con `Chiavi richieste` prima e dopo nel report.
- Memoria di progetto già scritta (`clip-additiva-suona-e-non-si-vede`): la regola «sonda prima di dichiarare».

## 3. Fuori scope

- Il ruolo `Hit` (D3): la guardia non lo tocca, la tabella non lo cambia; §5.2 lo **misura**, §7 lo decide.
- La `Push 1` della variante `CircularTide.Impact` (R3).
- Un'`AttackFootprint` per le aree di cura (nessuna impronta: l'onda d'area del profilo FX è per i colpi).
- #3562 (riferimenti duri alle clip nei `BP_Unit_*`): il gate di cook resta rosso per scelta.
- Qualunque asset, Blueprint o Niagara.

## 4. Errori e degrado

- Clip additiva su `Cast`/`Attack`: `Warning` nel log, ripiego al ruolo, poi `nullptr` → posa di riferimento, partita
  identica (invariante #1 di `PlayPresentationRole`).
- Clip di ruolo a sua volta additiva (`Hit` oggi; `Cast` mai, sondato): `nullptr`, stesso degrado.
- Area di cura senza alleati: una voce `Fallback/TargetGone`, nessuna cura, nessun `ensure`.
- Centro fuori portata: `Fallback/OutOfRange`, come oggi per `Single`.
- Alleato a salute piena nell'area: voce `Healed` con `Amount = 0` (già così per `Single`).

## 5. Verifica

### 5.1 Automation, headless

➕ rev. F7: **suite completa** `RefactorTactics` prima (sull'albero di partenza) e dopo, con il diff dei nomi e degli esiti:
l'unico rosso ammesso è `Packaging.RequiredActionClipsAreCooked` (ereditato, #3562). Famiglie che leggono i dati toccati e
che il piano nomina una per una: `Heroes.*`, `Equipment.*` (`MedkitHealsInMatch` è la regressione di `Single`), `Actions.*`
(`Actions.Heal.*`, `Actions.HeroKitsMatchTheirCatalogDef`), `Fx.*` (la gemella della riga d'override, F4), `AimOrigin.*`
(`CellTargetAfterADashAimsFromTheDash`, `Tests/RTAimOriginTests.cpp:487-535`), `Icon*`/`Hud*`, `Bot.*`, `Unit.*`,
`Playback.*`, `Anim.*`, `Packaging.*`, `Scenario.EveryShippedScenarioRuns` (con `ClipMuiren` a 78), `Determinism.*`.

`Ruling R8` (➕ rev. F7, il bot): il passo 2 dei candidati d'attacco (`Bot/RTBotPlanningLibrary.cpp:850-858`) esclude già
`bSelfTarget` e la mobilità; oggi `CircularTide` vi entra con `Power` 0 e la derivazione non lo cambia. Per non lasciare
al caso che il bot «attacchi» con una cura, il passo 2 esclude anche le azioni con `DerivedFromActionId == Action.Heal`,
pinnato da `Bot.DerivedHealIsNotAnAttackCandidate`; l'uso della cura ad area da parte del bot è il follow-up §7. Costo se è
sbagliato: un bot Muiren che oggi, per caso di punteggio, curava gli alleati accanto a un nemico smette di farlo.

⌫ #3595: le righe `Anim.AdditiveClipIsNotPlayableAsMontage`, `Playback.AdditiveClipFallsBackToRole`, `Playback.HitRoleKeepsAdditiveClips`,
`Unit.DeclaredClipsAreNotAdditive` e `Unit.DefaultActionClipsResolveForEveryKitAbility` (mutazioni 12–19) sono coperte
da `Anim.Channel.AdditiveClipIsDetected`, `…AdditiveActionClipFallsBackToTheRole`, `…AdditiveGestureRoleClipIsNotPlayed`,
`…AdditiveFallbackOnAnAdditiveRolePlaysNothing`, `…AdditiveHitClipStillPlays` e `Unit.DefaultGestureClipsAreNotAdditive`,
già in `main`; qui restano come registro.

Test nuovi di **questa** spec, ciascuno visto rosso prima del codice e validato per mutazione:

| Test | Asserto | Mutazione che lo fa cadere |
|---|---|---|
| `Heroes.Phase.TideDerivesFromHeal` | `DerivedFromActionId == Action.Heal`, `RangeCells == 4`, `Priority == 60`, `Effects == {Heal 18}`, `Area`, raggio 1 | (1) `MakeHeroAction` al posto di `MakeHeroActionFromCore`; (2) `RangeCells` non riscritto (→ 3) |
| `Heroes.TideHealsAlliesInArea` (fixture come `MedkitHealsInMatch`) | due alleati nel raggio a −40 HP salgono di 18; un alleato fuori raggio e un nemico dentro non cambiano; chi cura, dentro il raggio e a −10, sale di 10 (tetto); **tre** voci `Healed` nell'ordine di `Ctx.Units`; un alleato **morto** nel raggio produce una voce `Fallback/TargetDead` e resta morto; `AbilityActivated` una sola volta con `Shape == Area`; il cooldown è pagato | (3) il filtro «stessa squadra» tolto → il nemico guarisce; (4) `<= AreaRadius` → `<` → l'alleato sul bordo non è curato; (5) il morto filtrato prima di `AddHeal` → la voce `TargetDead` sparisce; (6) `EmitAbilityActivated` con `Single`; (7) `MarkAbilitySpent` saltato → il cooldown non è pagato |
| `Heroes.TideHealsVariantAmount` | con `ActiveVariantId = …Healing` l'alleato sale di 24 | (8) l'amount letto solo da `Def` |
| `Heroes.TideOnEmptyAreaStillStarts` | nessuno della squadra nel raggio → una voce `Fallback/NoEffect` con `TgtCell` = centro, nessuna `Healed`, cooldown pagato, `AbilityActivated` emesso | (9) il ramo vuoto tolto → nessuna voce; (10) `TgtCell` lasciato a `SrcCell` |
| `Bot.DerivedHealIsNotAnAttackCandidate` (R8) | un bot Muiren con un nemico in portata e `CircularTide` pronta non la pianifica come attacco | (11) l'esclusione tolta dal passo 2 |
| `Anim.AdditiveClipIsNotPlayableAsMontage` | `NewObject<UAnimSequence>` con `AdditiveAnimType = AAT_LocalSpaceBase` **e** `RefPoseType = ABPT_RefPose` (➕ rev. F1: il costruttore lascia `ABPT_None` e `IsValidAdditive` risponderebbe falso) → `false`; con `AAT_LocalSpaceBase` e `ABPT_None` → `true` (pinna il predicato del motore, non una regola nostra); con `AAT_None` → `true`; `nullptr` → `false` | (12) il predicato restituisce sempre `true`; (13) il predicato riscritto come `AdditiveAnimType != AAT_None` → cade il caso `ABPT_None` |
| `Playback.AdditiveClipFallsBackToRole` | `ForcedClipForTest` = sequenza additiva valida; `PlayPresentationRole(Cast, LinearDischarge)` **e** `PlayPresentationRole(Attack, LinearDischarge)` (➕ rev. F6: il difetto misurato era sul colpo) → seam vero su entrambi; `CastCuesPlayed` sale di uno (la cue parte lo stesso) | (14) la guardia tolta → il seam resta falso; (15) `RefusesAdditive = (Ruolo == Cast)` → cade il ramo `Attack`; (16) `return` dopo il rifiuto → `CastCuesPlayed` non sale |
| `Playback.HitRoleKeepsAdditiveClips` | stessa sequenza, `PlayPresentationRole(Hit, …)` → seam **falso** (D3) | (17) `RefusesAdditive` sempre vero |
| `Unit.DeclaredClipsAreNotAdditive` (§2.4) | ogni path che risolve è playable; `N/A` solo senza la cartella del pack; errore se il pack c'è e il path non risolve; `AddWarning` a zero misure | (18) una riga di Gadget riportata a `LMB_Fire_B` — eseguita **sul clone principale** |
| `Unit.DefaultActionClipsResolveForEveryKitAbility` (esistente) | la copia dichiarata coincide con la tabella | (19) una riga della copia lasciata a `Ability_Q_Target` |
| `Fx.DeclaredOverridesMatchTheProposal` (esistente, F4) | la riga di `CircularTide` è `Pulse/None/None/None` | (20) la gemella lasciata a `Marker/AreaPulse` |

### 5.2 Seduta PIE — riconvocazione di `U70`

Sul clone principale, Editor **ricompilato** con il codice di questa spec (controllo: nel log di build `Compile
Module.RefactorTactics.N.cpp` per i moduli che contengono `RTUnit.cpp`, `RTUnitAnimInstance.cpp`, `RTTurnManager_Blast.cpp`,
`RTHeroCatalogLibrary.cpp`; «Target is up to date» = binario vecchio), stessa ricetta di `U70`: scenari da console,
`rt.Debug.PlaybackStartPaused 1`, ripresa con `K`, camera stretta su un personaggio, catture a raffica.

1. **Aevik, colpo** (giudica #3595) — `Visual.Combat.WaterElectricCoordinated`, camera su Aevik: al `Colpo:` di
   `LinearDischarge` una posa diversa dall'idle (`LMB_Fire_B_Slow_V1`); all'`Attiva:` la `Cast` del pack. Atteso **sì**.
2. **Aevik, area** (giudica #3595) — `Visual.Ability.FxProfile`: `LMB_Fire_C_Slow_V1` al colpo di `Overload`.
3. **Muiren, cura** (giudica questa spec) — `Visual.Ability.ClipMuiren`: la barra di Aevik sale (`+18 salute` nel feed), il
   `Pulse` sulla cella di Muiren al cast (F4) e la posa di `R_Ability_Intro`.
4. ⌫ **Ruolo `Hit`**: già misurato (`PIE-AS4b`, `U8`); nessuna scena.
5. Le scene (0)–(3) di `PIE-CLIP-ABILITA` già verdi in `U70` si rigiudicano solo se cambia qualcosa che le tocca: la
   guardia non tocca Muiren, Branth e Ivrin (clip full-body), quindi **no**, salvo regressione vista per caso.

Esito atteso: `PIE-CLIP-ABILITA` da 🟡 a ✅ se 1–3 sono sì (le abilità senza scena hanno ora i banchi di #3592);
`U70` chiusa; `doc-coherence` A4, rosso su `main` da #3595 («`PIE-CLIP-ABILITA` è 🟡 ma #3590 è CLOSED»), si spegne
con il rigiudizio.

## 6. Limiti dichiarati

- La cura ad area non lascia un'impronta sulla mappa: l'`AbilityActivated` con `Area` dà l'anello d'attivazione sulla
  sorgente (profilo FX), i numeri verdi dicono chi è stato curato; l'onda d'area è dei colpi.
- La `Push 1` di `CircularTide.Impact` non si applica (R3).
- Il bot non usa la cura ad area (R8): la pianifica nessuno finché il follow-up non le dà un punteggio.
- `Hit` resta additiva fino a decisione (D3).
- La guardia legge `IsValidAdditive()` dell'asset: una clip full-body **autorata male** passa; una clip additiva
  **pensata** per sommarsi all'idle (nessuna oggi fra Cast/Attack) verrebbe rifiutata. Il giorno che ne serve una, la
  guardia prende un'eccezione per `ActionId`, non un flag globale.
- `Unit.DeclaredClipsAreNotAdditive` vale solo dove i pack sono sul disco (R6).

## 7. Follow-up candidates

- **La spinta curativa** della variante `Impact`: dove vive un'azione con `Heal` agli alleati e `Push` ai nemici nella
  stessa area (R3).
- **L'impronta delle cure**: un'`AttackFootprint`-gemella per le aree di supporto, se il profilo FX vuole un'onda verde (R7).
- **Il bot e la cura ad area** (R8): un punteggio per curare gli alleati raggruppati, nel passo dei candidati di supporto.
- **Il test sulla tabella in un gate con i pack**: oggi `R6` lo lascia a una corsa manuale sul clone principale.
