# RT3-B — Fallback animato GrayKit: referto di fattibilità del 2026-09-05

> **Mandato**: Terminale B della RT3 GrayKit Wave 1 · **Base misurata**: `8a530c6e` (`origin/main` alle 13:14)
> **Owner**: [#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288) (CP E21.2) · **Epic**: [#286](https://github.com/DegrassiAaron/refactor-tactics-main/issues/286)
> **Domanda**: *si può usare una presentazione animata GrayKit generica come fallback sui personaggi reali
> senza una seconda pipeline, senza perdere leggibilità e senza che l'animazione diventi autorità?*

Questo referto non è un'autorità: registra **misure**. Gli owner restano #288, #2444, il Decision Log e
`docs/technical/runbooks/guida-animazioni-paragon.md`.

---

## 1. Risposta

**`SI, MA` — e il `MA` non è tecnico: è una decisione già presa che nessuno ha ancora registrato.**

I tre gate che il mandato temeva sono **già chiusi dall'architettura corrente** (§5, §6, §7): il root motion
non ha un consumatore, nessun gameplay è pilotato da callback di animazione, e il timing è governato dal
resolver attraverso una tabella evento→cue esplicita. Un fallback generico non può diventare autorità
*perché non esiste il canale con cui lo diventerebbe*.

Ma la domanda «quale rig» **non è aperta**: è stata decisa il 2026-09-05 e la decisione è di un'altra
forma da quella che il mandato ipotizza — non un rig, non un mannequin, non un retarget, ma
**`UAnimSequence` RT-owned esportate e autosufficienti**. La decisione è registrata in un commento
(chiusura di [#2449](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2449)) e porta con sé
**due gate che nessuno ha chiuso** — costo binario e licenza (§4).

∴ Non c'è un `DECISION GATE — Canonical Fallback Presentation Rig` da aprire. C'è una **decisione presa
senza `D-nnn`, con due riserve scritte, e zero asset prodotti**.

---

## 2. Che cosa è stato misurato, e con quale comando

| # | Misura | Comando | Esito |
|---|---|---|---|
| M1 | `ABP_*` versionati | `find Content -iname 'ABP_*'` | **0** — la regola §3 del mandato regge |
| M2 | `AM_*` versionati | `find Content -iname 'AM_*'` | **0** |
| M3 | asset `GK_*` | `git ls-tree -r origin/main \| grep GK_` | **0** |
| M4 | pack Paragon in *questo* checkout | `ls Content/FabAsset` | **assenti** — `.gitignore:105`, ~48 GB |
| M5 | skeleton del roster | name table dei quattro `*_Skeleton.uasset` | **4 distinti** |
| M6 | catena ossea core condivisa | insiemi dei nomi, 23 ossa canoniche UE | **23/23 in tutti e quattro** |
| M7 | somiglianza complessiva degli skeleton | Jaccard sugli insiemi di nomi | **0,207** (127 comuni su 614) |
| M8 | `IK_*` / `RTG_*` nei pack | `find Content/FabAsset/Paragon -iname 'IK_*' -o -iname 'RTG_*'` | **0** |
| M9 | asset di retarget esistenti | `find … -iname '*Retarget*'` | **19**, tutti `URig` legacy UE4 |
| M10 | rig di retarget sul roster | presenza nella name table dei 4 skeleton | **Phase e Riktor sì (file), Gadget e Wraith no; nessuno referenziato** |
| M11 | le 20 clip della tabella AS.3b | esistenza sul disco | **20/20 presenti** |
| M12 | notify nelle 20 clip del roster | `grep -F 'AnimNotify_'` | **0/20** (6 file su 714 in tutto il corpus) |
| M13 | consumo del root motion | `grep -rn 'RootMotion\|ERootMotionMode' Source/` | **0 occorrenze** |
| M14 | classe base dell'unità | `RTUnit.h:27` | `ARTUnit : public AActor` — **niente `UCharacterMovementComponent`** |
| M15 | gameplay pilotato da animazione | `grep -rn 'OnMontageEnded\|AnimNotify\|OnNotifyBegin' Source/` | **0 occorrenze** |
| M16 | ruoli di presentazione già definiti | `ERTPresentationRole`, `origin/feat/2441` | **9** |
| M17 | path sperimentale del mandato | `git check-ignore -q Content/RT/Editor/GrayKit/Experiments/RT3B/GK_Test.uasset` | exit **1** → **versionabile** |
| M18 | path di un fallback per ruolo | `git check-ignore -q Content/RT/Characters/Shared/Animation/GK_Move.uasset` | exit **0** → **ignorato** |
| M19 | processi Unreal | `Get-Process UnrealEditor*` | **`UnrealEditor-Cmd` PID 51556, avviato 16:19:47** — non di questa sessione |
| M20 | ponte MCP | connessione TCP a `127.0.0.1:8765` | **rifiuto persistente** |
| M21 | tool MCP Unreal/Epic in sessione | ricerca tool su `unreal epic editor asset blueprint PIE` | **nessuno** (la ricerca ha restituito altri tool: assenza misurata, non vuota) |

⚠️ **Due misure di questo referto sono state rifatte perché la prima stesura era falsa.** Sono elencate in
§9 insieme alla causa: servono a chi rileggerà i numeri, e una delle due indebolisce una conclusione.

---

## 3. Headless inventory

Misurato leggendo la name table dei `.uasset` (senza aprire l'Editor) e i pack in
`D:/Repositories/refactor-tactics-main/Content/FabAsset/Paragon` — **sola lettura**: i pack sono
gitignorati e identici in ogni checkout, non appartengono a nessun branch.

| Personaggio | Skeleton | Mesh usata dal `BP_Unit_*` | AnimClass | Idle | Move | Attack/Cast | Hit | Death | Root motion | Note |
|---|---|---|---|---|---|---|---|---|---|---|
| **Gadget** | `Gadget_Skeleton` | `Meshes/Gadget` | `URTUnitAnimInstance` (C++) | `Idle` | `Run_Fwd` | `Cast` | `Hitreact_Fwd` | `Death_Fwd` | non misurabile headless | 🔴 non ha `Jog_Fwd`; `r` minuscola in `Hitreact` |
| **Phase** | `phase_Skeleton` | `Meshes/Phase_GDC` | idem | `Idle` | `Jog_Fwd` | `Cast` | `HitReact_Fwd` | `Death` | idem | mesh `_GDC`, non `Phase`; ha `Paragon_Proto_Retarget` |
| **Riktor** | `Riktor_Skeleton` | `Meshes/Riktor` | idem | `Idle` | `Jog_Fwd` | `Cast` | `HitReact_Front` | `Death_Fwd` | idem | ha `Paragon_Proto_Retarget`; catene di ossa lunghe (#1763) |
| **Wraith** | `Wraith_Skeleton` | `Meshes/Wraith` | idem | `Idle_NonCombat` | `Jog_Fwd` | `Cast` | `HitReact_Front` | `Death_Forward` | idem | 🔴 `Calf_L`/`Calf_R` maiuscoli; locomozione *combat* separata |

✅ **La tabella AS.3b del runbook regge**: tutte e venti le caselle esistono sul disco oggi, con i nomi che
il documento dichiara. I riferimenti reali dei quattro `BP_Unit_*` coincidono con le righe *Idle* e *Corsa*.

⚠️ **Nessuna delle venti clip porta notify** (M12) — quindi nessuna cue di presentazione esiste ancora, e
nessun rischio di autorità arriva da lì.

---

## 4. Il rig canonico: la decisione esiste, i suoi due gate no

### Che cosa esiste davvero

| Cercato | Trovato |
|---|---|
| un rig GrayKit | **no** — gli unici GrayKit versionati sono `WBP_RT_GrayKitPlayground` e `L_GrayKitPlayground`, di RT3-C |
| un mannequin fallback | **no** |
| uno skeleton presentation-only canonico | **no** |
| una strategia retarget adottata | **no** — il runbook §AS.3 si intitola *«con Paragon niente retargeting»* e rimanda l'IK Retargeter |
| infrastruttura di retarget | **parziale e legacy** — 19 `URig` UE4 (`Paragon_Proto_Retarget`, `Orion_Proto_Retarget`), zero IK Rig UE5 |

🔴 **L'infrastruttura legacy non copre il roster e non è agganciata.** `Paragon_Proto_Retarget` esiste
nella cartella di **Phase** e **Riktor**; **Gadget** e **Wraith** non ne hanno nessuno. E nessuno dei
quattro skeleton lo referenzia (M10): sono file presenti, non un sistema in uso. `URig` è inoltre il
meccanismo di retarget **deprecato** in UE5.8, sostituito da IK Rig + IK Retargeter, di cui il progetto
non ha un solo asset.

### Perché il retarget generico costerebbe, e quanto

I quattro skeleton **condividono la nomenclatura Epic sul corpo**: tutte e 23 le ossa della catena core
(`root`, `pelvis`, `spine_01..03`, `neck_01`, `head`, `clavicle/upperarm/lowerarm/hand` ×2,
`thigh/calf/foot/ball` ×2) esistono in tutti e quattro (M6). Su questo, un IK Rig si scrive.

⚠️ **Ma la somiglianza si ferma lì**: sull'insieme completo dei nomi la sovrapposizione è **0,207** (M7).
Le differenze non sono cosmetiche — sono ossa che esistono solo su un personaggio: `arm_wire_*` e
`backpack_*` su Gadget, `FX_Candles_*` e le catene di spalla su Riktor, `robo_arm_*`, i `*_middle` di
Wraith. Un retarget generico le lascia ferme: leggibile su una posa di corsa, **non giudicabile a priori**
su un `Cast` o una `Death`, che è dove quelle appendici si muovono.

E due trappole di scrittura, misurate: **Wraith porta `Calf_L`/`Calf_R` maiuscoli** dove gli altri tre
hanno `calf_l`/`calf_r`, e **Phase porta `Spine_02`/`Spine_03`/`Head`** maiuscoli. Il Content Browser
cerca senza distinguere maiuscole; un mapping scritto a mano no. È la stessa classe di difetto che il
runbook §AS.3b ha già pagato una volta sui nomi delle clip.

### La decisione d'autore del 2026-09-05

Registrata nel commento di chiusura di **#2449**, che si è ritirata come duplicato di **#2444**:

> l'autore ha scelto **`UAnimSequence` esportate, autosufficienti**: ~2,68 MB, **+70 %** di `Content/`.

🔴 **Quella decisione porta due riserve scritte, e nessuna delle due è stata chiusa.**

1. **Costo.** 2,68 MB è **0,96×** l'ordine di grandezza che **D-248** ha rifiutato per i quattro `ABP_*`
   (2,8 MB). La decisione è a un soffio dal precedente che la contraddice. ⚠️ E il denominatore che
   circola è stantio: `Content/` versionato **non pesa 0,7 MB ma 3,815 MB** — il numero `0,7 MB` è ancora
   nel docstring di `RTUnitAnimInstance.h` e in D-248.
2. **Licenza.** Esportare pose dai pack Paragon in namespace RT-owned non è coperto da **#2291**, che
   mette `Content/FabAsset/Paragon/**` in allowlist — cioè copre un *soft pointer* verso un asset vendor,
   non un contenuto *esportato*. Il registro di provenienza è lavoro di **#1767** (branch
   `docs/1767-registro-provenienza-asset`, aggiornato oggi).

⛔ **Stato di fatto: zero asset creati, nessuna riga di `.gitignore` toccata, nessun `D-nnn` assegnato.**

---

## 5. Root motion — il gate è chiuso, ma non dove sembrava

**Verdetto: il fallback non può muovere l'unità, e la ragione è architetturale, non di configurazione.**

| Domanda del mandato | Esito |
|---|---|
| Root Motion presente nelle clip? | ⚠️ **NON MISURATO** — vedi §9, la mia misura non discrimina il valore |
| Root Motion consumato? | ❌ **No** — `grep -rn 'RootMotion\|ERootMotionMode' Source/` → **0 occorrenze** |
| Root Motion potrebbe muovere l'Actor? | ❌ **No** — `ARTUnit : public AActor` (`RTUnit.h:27`), nessun `UCharacterMovementComponent` nel runtime |

🔑 **È il secondo punto che chiude il gate, e vale anche se il primo fosse `sì`.** Il delta di root motion
viene applicato all'attore da `UCharacterMovementComponent`, che qui non esiste: `ARTUnit` deriva da
`AActor`. Nessuna riga di `Source/` nomina il root motion. Una clip che ne portasse resterebbe
**in place**, che è esattamente la direzione raccomandata dal mandato §9 — ottenuta però dall'assenza di
un consumatore, non da una scelta dichiarata.

⚠️ **Questa assenza non è protetta da nulla.** È un fatto dell'architettura odierna, non un invariante
scritto né un test. Il giorno in cui qualcuno derivasse `ARTUnit` da `ACharacter` — o aggiungesse un
movement component per un'altra ragione — il gate si aprirebbe **in silenzio**. `#2449` aveva già messo
in acceptance *«Root Motion non sposta l'unità oltre la cella che il resolver ha deciso»*, da verificare
in PIE: quel criterio è ora di **#2444** ed è l'unico presidio previsto.

---

## 6. Notify — nessun gameplay pilotato da animazione

`grep -rn 'OnMontageEnded\|OnNotifyBegin\|OnPlayMontageNotify\|AnimNotify\|OnMontageBlendingOut' Source/`
→ **zero occorrenze** in tutto il progetto (runtime, editor, test).

✅ Non esiste oggi gameplay pilotato da callback di animazione: nessun `ApplyDamage`, `ResolveHit`,
`MoveUnit` o `AdvanceMicroStep` raggiungibile da un notify. Il gate §10 del mandato **non ha nulla da
correggere**, e un fallback GrayKit non introdurrebbe il problema perché nessuna delle venti clip del
roster porta notify (M12) — nel corpus intero sono 6 file su 714.

⚠️ Se un giorno una clip di fallback ne portasse, l'uso ammesso resta quello dichiarato dal mandato
(footstep, spark, sound, camera cue) e la difesa è la stessa che il progetto usa altrove: la simulazione
decide, la presentazione mostra (invariante #1).

---

## 7. Timing e autorità — il canale esiste già ed è esplicito

`Source/RefactorTactics/Turn/RTPresentationBinding.cpp` è la tabella dichiarata evento→cue. È il punto in
cui un fallback GrayKit si aggancerebbe, ed è già scritto con la disciplina che serve:

| Evento risolto | Cue dichiarate |
|---|---|
| `Move` | `bIsMovingVisually`, `SetVisualLocation` |
| `Attack` | `PlayAttackMontage` (attaccante), `PlayHitMontage` (bersaglio) |
| `Defeated` | `HideForDefeat`, `PlayDefeatMontage` |
| `HazardDamage` | *NoPresentation* — per scelta, ma ⚠️ **senza produttore** |
| `AttackFootprint` · `ReactionResolved` · `StatusChanged` | *NoPresentation* — assenza **temporanea e attesa** |

Il `TurnManager` chiama le cue da sé (`RTTurnManager.cpp:7628-7629, 7655-7656, 7671, 7729`). La durata
dell'animazione non entra da nessuna parte nella risoluzione.

🔴 **Il documento contiene già l'avvertenza che vale anche per GrayKit**, e va rispettata alla lettera:

> *«Dichiarare cue inventate sarebbe peggio che dichiarare l'assenza. Le altre voci di questa tabella
> nominano funzioni che il C++ chiama davvero; scrivere qui il nome di un effetto che nessuno ha ancora
> scritto renderebbe la tabella una lista di intenzioni, e il gate smetterebbe di misurare qualcosa.»*

∴ Un fallback GrayKit **non va dichiarato in questa tabella finché non esiste**. Le tre voci temporanee
sono il segnaposto corretto.

---

## 8. Tassonomia — esiste già, e diverge dal mandato in due punti

Il mandato §5 chiede di **proporre** una tassonomia come output per RT3-A. ⚠️ **RT3-A l'ha già
implementata**: `ERTPresentationRole` (`origin/feat/2441-clips-per-hero-ruolo-variante`) porta **nove
ruoli**, e #2445/#2446/#2447 sono **chiuse**.

| Famiglia del mandato | Ruolo del mandato | `ERTPresentationRole` reale |
|---|---|---|
| LOCOMOTION | Idle | ✅ `Idle` |
| LOCOMOTION | Move | ✅ `Move` |
| ACTION | Attack | ✅ `Attack` |
| ACTION | Cast | ✅ `Cast` |
| ACTION | Dash | ✅ `Dash` |
| ACTION | Defend | ✅ `Defend` |
| ACTION | **Interact** | 🔴 **assente** |
| REACTION | HitReact | ✅ `Hit` |
| REACTION | Death | ✅ `Death` |
| REACTION | Knockdown/GetUp *(condizionale)* | ⚠️ esiste `Fall`, che è **uno** stato, non la coppia |

**Le due divergenze, e che cosa significano.**

1. **`Interact` non ha un ruolo di presentazione**, ma è una delle sette azioni universali dichiarate in
   `AGENTS.md` §1 (`Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch`). Non sono le sole
   scoperte: **`Wait` e `Overwatch` non hanno un ruolo**, e **`Guard` e `Brace` — due azioni distinte —
   condividono `Defend`**. Il conto esatto sulle sette azioni universali: **due** hanno un ruolo proprio
   (`Move`, `BasicAttack`), **due** ne condividono uno (`Guard` e `Brace` su `Defend`), **tre** non ne
   hanno nessuno (`Wait`, `Interact`, `Overwatch`).

   ⚠️ *La prima stesura di questa riga diceva «ne coprono **tre** con un ruolo proprio», e il numero era
   sbagliato: `Guard` e `Brace` hanno un ruolo, ma non **proprio**, e contarli fra i coperti confondeva
   le due colonne. Corretto il 2026-09-05 ricontando contro `AGENTS.md` §1 e l'enum su `main`; il
   conteggio giusto è anche nel commento di [#2448](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2448).*

   🔴 **E la condivisione non è neutra**: il corpus separa esplicitamente le due difese
   (`Scenarios/Visual/Combat/GuardVsBraceUnderSmallHits.json`, `GuardReducesFirstHit`,
   `BraceReducesEveryHit`). Due azioni che il resolver tratta diversamente e che giocano la stessa posa
   non sono distinguibili a schermo — è un problema di leggibilità, cioè di E21.
2. **`Fall` è uno stato solo.** Una coppia `Knockdown`/`GetUp` sarebbe due clip e una transizione; `Fall`
   non lo è. Non va promosso a coppia senza un consumer reale — e oggi non ce n'è.

⚠️ **Questo referto non modifica il resolver, non aggiunge `FallbackRole` ad `Action`, e non promuove
questa tabella a enum.** #2448 misura già che *«sette ruoli su nove non hanno un consumatore»*: aggiungere
ruoli prima dei consumatori peggiorerebbe quel rapporto.

### Classificazione — e perché non uso il vocabolario del mandato

Il mandato §13 propone `Candidate / Previewable / Compatible / Incompatible / NeedsFix`. ⛔ **Non l'ho
usato**: `ERTAnimClipStatus` esiste già con quattro valori — `Unreviewed`, `Candidate`, `Promoted`,
`Rejected` — e `Promoted` porta in codice il commento *«⛔ Solo per mano umana»*. Introdurre un secondo
vocabolario creerebbe la seconda source of truth che `AGENTS.md` §8 vieta.

**Tutte e venti le clip del roster restano `Unreviewed`.** Nessuna promozione, nessuna euristica sui nomi.

---

## 9. Le due misure che ho dovuto rifare

Registrate perché una indebolisce una conclusione, e chi rilegge i numeri deve saperlo.

1. **Il lettore di name table era cieco sulle `UAnimSequence`.** La prima stesura dichiarava *«0 clip su 20
   con root motion, 0 con notify»*. Un controllo positivo su tutto il corpus (`grep -F` sui byte) ha
   trovato il marcatore in **657 file su 714**: il parser derivava sull'header (`LegacyFileVersion = -9`)
   e leggeva una name table parziale. Le misure sulle clip sono state rifatte sui byte.
2. 🔴 **E il marcatore non prova ciò che sembrava provare.** L'ipotesi era: *«`bEnableRootMotion` compare
   solo se il valore è diverso dal default, quindi presenza ⇒ root motion attivo»*. Elencando le **57**
   clip che **non** lo portano si vede che sono tutte `AimOffset`, `BlendSpace`, `Montage`, `Lean`,
   `TurnInPlace` — cioè **classi di asset che quella property non hanno**. Il marcatore discrimina la
   *classe*, non il *valore*.
   ∴ **«Root motion presente nelle clip» resta `NOT RUN`**, misurabile solo in Editor. La conclusione di
   §5 non ne dipende: il gate è chiuso dall'assenza di un consumatore (M13, M14), non dal valore del flag.

Una terza svista, senza conseguenze sul verdetto: un `grep -c … || echo 0` produceva `"0 0"` quando non
trovava nulla, e il confronto con `0` falliva — la tabella diceva `SI` ovunque. Rifatta con lo stato
d'uscita.

---

## 10. Asset sperimentali — dove sarebbero ammessi, e dove no

| Path | `git check-ignore -q` | Esito |
|---|---|---|
| `Content/RT/Editor/GrayKit/Experiments/RT3B/GK_Test.uasset` | exit **1** | ✅ **versionabile** — `.gitignore:257` (`!Content/RT/Editor/**/*.uasset`) |
| `Content/RT/Characters/Shared/Animation/GK_Move.uasset` | exit **0** | ❌ **ignorato** — nessuna allowlist |
| `Content/RT/Characters/Gadget/Animation/GK_Move.uasset` | exit **0** | ❌ **ignorato** |

🔑 **Il path sperimentale che il mandato ipotizzava è ammesso dalla policy corrente.** Ma è sotto
`Content/RT/Editor/`, cioè **editor-only**: va bene per un esperimento, non per un fallback che deve
esistere nel gioco cotto.

⚠️ **Un fallback per ruolo e condiviso — la forma che la decisione d'autore ha scelto — oggi non ha un
path versionabile.** Servirebbe una riga di allowlist, che #2449 prescriveva di scrivere **prima**
dell'asset (l'ordine di `asset-map.md` §6, e il precedente di `ABP_Gadget`: la riga c'era, mancava il
`git add`). ⛔ Questo referto **non tocca `.gitignore`**: il mandato lo vieta, e la riga va scritta da chi
possiede la decisione, cioè **#2444**.

**Nessun asset è stato creato da questo lavoro.** `PROMOTED = NONE`.

---

## 11. Che cosa resta a una persona

| Domanda | Perché non è headless |
|---|---|
| la posa retargettata è **leggibile**? | giudizio a schermo — classe C in `scenario-map.md` |
| deformazioni sulle appendici non condivise (§4) | idem, e sono il 79 % dei nomi |
| orientamento / `MeshYawOffset` per pack | si corregge guardando `FacingArrow` |
| flag `loop` delle clip | sta dentro l'asset, si legge aprendolo |
| root motion **attivo** su una clip | §9 — il marcatore non lo dice |
| costo authoring reale di una slice | richiede l'Editor |

Gli scenari per il giudizio **esistono già** e non ne servono di nuovi: `Visual.Combat.Defeat`
(`Scenarios/Visual/Combat/Defeat.json`) e `Visual.Movement.Charge`; `Movement.LongWalk` esiste per il
movimento. ⚠️ **Ma nessuno di essi istanzia un `AnimInstance` con asset**: `RTScenarioSession.cpp` spawna
`ARTUnit::StaticClass()` e non i `BP_Unit_*`. Un'asserzione headless in quest'area sarebbe verde per
costruzione — la classe di difetto di #1763.

---

## 12. Rischi

1. 🔴 **La decisione sul fallback è a 0,96× dal precedente che l'ha rifiutata** (D-248), e il denominatore
   che circola è sbagliato di 5,4×. Chi la eredita senza rileggere i due numeri sta decidendo su una
   soglia stantia.
2. 🔴 **La domanda di licenza sull'export in namespace RT non ha una risposta scritta.** #2291 copre il
   naming, non l'export.
3. ⚠️ **L'assenza di consumatore del root motion non è protetta da nulla** (§5): è un fatto, non un
   invariante.
4. ⚠️ **`Source/RefactorTactics/Unit/` è conteso.** Il mio write-set nominale lo includeva, ma
   `origin/feat/2441` sta riscrivendo `RTUnitAnimInstance.{h,cpp}` (**+303 / −26** righe). **Non l'ho
   toccato.**
5. ⚠️ I numeri di riga citati nelle issue di questa materia sono già stantii: `feat/2341` sposta i call
   site dei montaggi di ~+90 righe.

---

*Referto additivo: nessun documento owner è stato modificato, nessun asset creato, nessuna issue aperta,
nessun `D-nnn` assegnato.*
