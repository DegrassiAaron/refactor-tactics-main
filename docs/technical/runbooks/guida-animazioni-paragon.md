# Guida — Workflow prototipo Paragon: animare un personaggio (AS.3 / AS.4)

> `CURRENT` come **metodo**, `HISTORICAL` come **casting** · **Ultimo aggiornamento**: 2026-08-13
>
> **Il procedimento vale; i nomi dei personaggi no.** Questa guida usa **Gideon** e **Sparrow** con gli
> archetipi *Guardian* e *Ranger*: erano il prototipo del 2026-08-03, **non** il roster canonico, che è
> **Gadget · Phase · Riktor · Wraith**.
>
> **Il casting non è più aperto** (2026-08-08, [D-037](../../decisions/RT_PDR_00_Decision_Log.md)) e dal
> **2026-08-11 gli asset portano il nome del pack**, non dell'eroe
> ([`convenzioni-contenuti-ue.md` §5b](../tooling/convenzioni-contenuti-ue.md)). Questa è la tabella da cui tradurre
> ogni esempio qui sotto — **verificata sul disco l'11-08**, perché tre nomi su otto non sono quelli che ci
> si aspetta:
>
> | Eroe | Pack | Blueprint | Skeletal Mesh | Skeleton |
> |---|---|---|---|---|
> | Gadget | `Paragon.Gadget` | `BP_Unit_Gadget` | `Gadget` | `Gadget_Skeleton` |
> | Phase | `Paragon.Phase` | `BP_Unit_Phase` | ⚠️ `Phase_GDC` | `phase_Skeleton` |
> | Riktor | `Paragon.Riktor` | `BP_Unit_Riktor` | `Riktor` | `Riktor_Skeleton` |
> | Wraith | `Paragon.Wraith` | `BP_Unit_Wraith` | `Wraith` | `Wraith_Skeleton` |
>
> 🔴 **La colonna «AnimBP» è stata tolta il 2026-08-30, e non è un dettaglio di formato**: gli `ABP_<Pack>`
> **non esistono e non vanno creati**. Il grafo di locomozione vive in C++ (`URTUnitAnimInstance`) dal
> 2026-08-25, e la classe la assegna `ARTUnit::ApplyUnitAnimClass()` allo spawn —
> [D-248](../../decisions/RT_PDR_00_Decision_Log.md), [#1720](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1720).
> La cartella `Animation/` resta nel percorso qui sotto perché è dove andranno i **montaggi** `AM_*`, che
> sono lavoro reale e ancora aperto (§AS.4b).
>
> Cartelle: `/Game/RT/Characters/<Pack>/{Blueprints,Animation}/`. Pack in
> `/Game/FabAsset/Paragon/Paragon<Pack>/Characters/Heroes/<Pack>/…`.
>
> ⚠️ **La mesh di Phase non si chiama `Phase`**: è `Phase_GDC` (22,6 MB — gli altri file in quella cartella
> pesano 0,1 MB e sono extents, shadow e skeleton). È l'errore che costa mezz'ora a cercare un asset che
> non esiste.
>
> 🔴 **Lo skeleton si LEGGE dalla mesh, non si cerca nella cartella.** La prima stesura di questa tabella
> dava `gadget_bot_Skeleton` e `belly_Riktor_Skeleton`, presi perché erano il primo file `*Skeleton*` di
> quella cartella — e appartengono invece a **mesh diverse e più piccole** (`gadget_bot`, `belly_Riktor`).
> Corretta il 2026-08-11 leggendo il riferimento dentro ciascun `.uasset`. In editor: apri la Skeletal Mesh
> e leggi il campo **Skeleton** nei Details; non dedurlo dal nome del file.
>
> Gli esempi restano scritti su **Gideon/Sparrow** perché sono il *procedimento* registrato nel prototipo del
> 2026-08-03: leggi «Gideon» come «il personaggio che stai importando» e traduci con la tabella. Riscriverli
> uno per uno introdurrebbe più errori di quanti ne toglierebbe, e la guida resterebbe comunque un esempio.

> Guida operativa per l'editor UE 5.8.1. Riferita a [`spec-asset-pipeline.md`](../architecture/spec-asset-pipeline.md) (AS.3/AS.4).
> Presuppone la Fase 2 fatta: un `BP_Unit_<Pack> : ARTUnit` con Skeletal Mesh assegnata, cilindro **nascosto**
> (`ARTUnit::Mesh` è uno `UStaticMeshComponent` e resta il root: si aggiunge una skeletal accanto, non la si
> sostituisce), `VisualZOffset=0` e i due materiali degli anelli. I passi completi sono in
> [`../../roadmap/editor-sessions.yaml`](../../roadmap/editor-sessions.yaml), seduta **U7**.
> Tutto quello che segue è **Blueprint** (nessuna ricompilazione C++).

## AS.3 — Animazioni: con Paragon niente retargeting

I personaggi Paragon includono **le loro animazioni native sul loro scheletro** (`Gideon_Skeleton`). Non serve
l'IK Retargeter finché non vuoi condividere animazioni tra scheletri diversi o usare Mixamo (quello resta AS.3
"avanzato", rimandato). Clip utili di **Gideon** (in `/Game/FabAsset/Paragon/ParagonGideon/Characters/Heroes/Gideon/Animations/`):

| Ruolo nel gioco | Clip Paragon | Note |
|---|---|---|
| Idle | `Idle` | in piedi, loop |
| Corsa (Move) | `Jog_Fwd` | corsa in avanti, loop |
| Attacco (Blast) | `Cast` | Gideon è un caster → "lancio" |
| Colpito (Hit) | `HitReact_Front` | reazione al danno |
| Morte (Defeated) | `Death_Fwd` | caduta in avanti |

> Per **Sparrow** (Ranger), quando è scaricato, cerca gli equivalenti in
> `/Game/FabAsset/Paragon/ParagonSparrow/.../Animations/` (`Idle`, `Jog_Fwd`, un attacco tipo `Primary`/`Fire`, `HitReact_Front`, `Death_Fwd`).

## AS.3b — Le clip dei quattro pack del roster

🔴 **Nessuno dei cinque nomi di Gideon vale per tutti e quattro i pack, e sei caselle su venti hanno un nome
diverso da quello che ci si aspetta.** La tabella di Gideon è l'esempio di *quali ruoli* servono, non di come si
chiamano i file. È lo stesso errore già costato una correzione un piano più in basso — gli skeleton presi dal
nome del file invece che dalla mesh — e si evita nello stesso modo: **leggendo la cartella, non deducendo il
nome**.

I due numeri, misurati e non stimati: **14 caselle su 20** portano lo stesso nome che porta in Gideon, e la
distribuzione conta più del totale — `Cast` regge **4** volte su 4, `Idle` e `Jog_Fwd` **3**, `HitReact_Front` e
`Death_Fwd` solo **2**. Cioè: più della metà dei nomi si trasferisce, ma **nessun ruolo tranne l'attacco** si
trasferisce sempre, ed è per questo che dedurre il nome non funziona mai per un pack intero.

<!-- I due numeri si rimisurano leggendo la cartella, non ricopiando questa riga:
     python - <<'PY'   (dalla radice del repo)
     import os
     B = "Content/FabAsset/Paragon"
     G = {"Idle":"Idle","Corsa":"Jog_Fwd","Attacco":"Cast","Hit":"HitReact_Front","Morte":"Death_Fwd"}
     R = {"Gadget": {"Idle":"Idle","Corsa":"Run_Fwd","Attacco":"Cast","Hit":"Hitreact_Fwd","Morte":"Death_Fwd"},
          "Phase":  {"Idle":"Idle","Corsa":"Jog_Fwd","Attacco":"Cast","Hit":"HitReact_Fwd","Morte":"Death"},
          "Riktor": {"Idle":"Idle","Corsa":"Jog_Fwd","Attacco":"Cast","Hit":"HitReact_Front","Morte":"Death_Fwd"},
          "Wraith": {"Idle":"Idle_NonCombat","Corsa":"Jog_Fwd","Attacco":"Cast","Hit":"HitReact_Front","Morte":"Death_Forward"}}
     print(sum(1 for r,a in G.items() for p in R if R[p][r] == a), "caselle su 20")
     PY -->

⚠️ **La prima stesura di questa sezione diceva altro, e le due frasi erano false**: «i cinque nomi non esistono
in nessuno dei quattro pack» (`Cast` c'è in tutti e quattro) e «solo otto portano il nome che ci si aspetta»
(sono quattordici). La tabella qui sotto era invece corretta in tutte e venti le caselle — è la **frase di
sintesi** ad aver divergato dai dati che riassumeva, e si rilegge contro la tabella, non a memoria.

Misurata sul disco il **2026-08-13**, cartella
`/Game/FabAsset/Paragon/Paragon<Pack>/Characters/Heroes/<Pack>/Animations/`:

| Ruolo | `Paragon.Gadget` | `Paragon.Phase` | `Paragon.Riktor` | `Paragon.Wraith` |
|---|---|---|---|---|
| **Idle** | `Idle` | `Idle` | `Idle` | 🔴 `Idle_NonCombat` |
| **Corsa** (Move) | 🔴 `Run_Fwd` | `Jog_Fwd` | `Jog_Fwd` | `Jog_Fwd` |
| **Attacco** (Blast) | `Cast` | `Cast` | `Cast` | `Cast` |
| **Colpito** (Hit) | 🔴 `Hitreact_Fwd` | 🔴 `HitReact_Fwd` | `HitReact_Front` | `HitReact_Front` |
| **Morte** (Defeated) | `Death_Fwd` | 🔴 `Death` | `Death_Fwd` | 🔴 `Death_Forward` |

Le quattro caselle che fanno perdere più tempo, e perché:

- **Gadget non ha `Jog_Fwd`.** Ha `Jog_Fwd_Start`, `_Stop`, `_CircleLeft`, `_Pivot180` — tutte transizioni, non
  loop. La corsa in ciclo è la famiglia **`Run_*`** (`Run_Fwd`, `Run_Bwd`, `Run_Lft`, `Run_Rt`, `Run_Zero`), che
  è anche quella che alimenta il suo blendspace `Run_Direction_1D`. Cercare `Jog_Fwd` qui dà zero risultati e
  sembra un pack incompleto: non lo è, è nominato diversamente.
- **Wraith non ha un `Idle` nudo.** Ha `Idle_Ability_Q`, `Idle_Noise_A/B`, `Idle_NonCombat` e — in una
  **sottocartella** — `Locomotion_Combat/Idle_Combat`.
- **`Hitreact` di Gadget ha la `r` minuscola** (`Hitreact_Fwd`), Phase la maiuscola (`HitReact_Fwd`). Il
  Content Browser cerca senza distinguere maiuscole, ma un path scritto a mano no.
- **Morte**: tre nomi diversi su quattro pack (`Death_Fwd`, `Death`, `Death_Forward`). Gadget ne ha anche uno
  intitolato al personaggio, `Gadget_Death_A`.

⚠️ **Idle e corsa si scelgono in coppia, non a due voci indipendenti.** Wraith e Phase hanno una locomozione
*combat* completa e separata (`Locomotion_Combat/` per Wraith, `Jog_Fwd_Combat` per Phase): mescolare un idle
in posa di combattimento con una corsa fuori combattimento — o viceversa — fa cambiare postura al personaggio a
ogni transizione. Per Wraith le due coppie coerenti sono `Idle_NonCombat` + `Jog_Fwd` (entrambe in root) oppure
`Locomotion_Combat/Idle_Combat` + `Locomotion_Combat/Jog_Fwd_Combat`.

⚠️ **Quello che questa tabella non dice**: se la clip sia marcata **loop**. Il flag sta dentro l'asset e si
legge solo aprendolo. `Idle` e la corsa devono esserlo, altrimenti lo stato gioca una volta e si ferma — un
personaggio immobile con l'AnimBP che *sembra* funzionare. Si controlla al primo PIE, prima di ripetere la
procedura sugli altri tre.

---

## AS.4a — Locomozione Idle ↔ Run (obiettivo: i quattro eroi corrono nel Move)

🔴 **Il grafo di locomozione NON si costruisce in editor: vive in C++, e questa sezione è stata riscritta il 2026-08-30 ([#1720](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1720)) per dirlo.**
Fino a quel giorno mandava a creare quattro `ABP_<Eroe>`, ed era già la **seconda** stesura: la prima
(2026-08-03) diceva `ABP_Gideon` e `BP_Unit_Guardian`, e la riscrittura del **2026-08-25** ne corresse i
**nomi** senza sapere che poche ore dopo, dentro lo stesso [#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288),
sarebbe cambiata la **via**. Un documento che si dichiara appena riverificato è più credibile di uno silente,
e per questo sbagliava più a fondo — è la deriva che
[D-248](../../decisions/RT_PDR_00_Decision_Log.md) esiste per chiudere.

⚠️ **Non è una correzione di nomi: è lavoro che non va fatto.** Creare oggi i quattro `ABP_*` costerebbe
~2,8 MB di binario versionato contro gli 0,7 MB che pesa **tutto** `Content/` — e i `.uasset` non si
comprimono per delta, quindi il prezzo si ripaga a ogni salvataggio.

### Cosa fa il codice, e che quindi non devi fare tu

Il grafo è `URTUnitAnimInstance` (`Source/RefactorTactics/Unit/RTUnitAnimInstance.h` +
`Source/RefactorTactics/Unit/RTUnitAnimInstance.cpp`), sotto test con `FRTUnitAnimClipsTest`:

| Il passo che stava qui | Chi lo fa oggi |
|---|---|
| «crea l'Animation Blueprint, uno per eroe» | **nessuno**: non esistono `.uasset` di animazione e non vanno creati (`find Content -iname "ABP_*"` → zero) |
| «+ Variable `bIsMoving`, Event Blueprint Update Animation, Cast To `RTUnit`» | il proxy legge `ARTUnit::bIsMovingVisually` da sé, senza bind e senza cast in Blueprint |
| «Add New State Machine `Locomotion`, stati Idle e Run, transizioni» | `FRTUnitAnimProxy::Initialize` monta `Idle` e `Run` in un `FAnimNode_TwoWayBlend` e ne pilota l'`Alpha` |
| «Anim Class = `ABP_<Eroe>`» | `ARTUnit::ApplyUnitAnimClass()` allo spawn; il default è `URTUnitAnimInstance`, assegnato nel costruttore |
| «le clip si prendono pack per pack da [AS.3b](#as3b--le-clip-dei-quattro-pack-del-roster)» | `ClipsPerHero`, `TSoftObjectPtr` composti a runtime — restano **dati diffabili**, non un binario |

⚠️ **Il `Try Get Pawn Owner` resta lo scoglio, ma non è più tuo.** `ARTUnit` deriva da **`AActor`** e non da
`APawn`, quindi quel nodo restituisce `null` e la macchina resterebbe in `Idle` senza un errore, senza un
warning e senza un log; il C++ usa `GetOwningActor` proprio per questo. La nota resta qui perché è la prima
cosa che si sbaglia il giorno in cui qualcuno prova comunque un AnimBP a mano — cosa che
`ApplyUnitAnimClass` **permette** ancora: una `Anim Class` già scelta in Blueprint vince, e il default C++
non la scavalca.

### Quello che resta davvero da fare in editor

1. **La Skeletal Mesh sul `BP_Unit_<Eroe>`** — è l'unico lavoro aperto di questa sezione, ed è
   [#1719](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1719). Su **Gadget** è agganciata;
   su **Phase**, **Riktor** e **Wraith** no: le clip ci sono, la mesh sta ancora sotto il cilindro e le tre
   unità **si vedono deformate**. Lo skeleton si legge **dalla mesh**, non dal nome del file
   ([AS.3](#as3--animazioni-con-paragon-niente-retargeting)).
2. **Il flag `loop` delle clip**, che nessun documento può dichiarare al posto tuo: sta dentro l'asset e si
   legge aprendolo. `Idle` e la corsa devono esserlo. I due nodi C++ sono già in loop
   (`SetLoopAnimation(true)`), ma una clip non-loop resta non-loop.
3. **`MeshYawOffset`**, se il personaggio corre di traverso: le skeletal si modellano lungo **+Y**, il
   forward di un attore è **+X**. `ACharacter` compensa i 90° nel proprio costruttore, `AActor` **no** — il
   default è `-90`, ma è un parametro perché dipende dal pack. Si corregge guardando `FacingArrow`, che
   punta sempre lungo il forward dell'attore.
4. ⚠️ **Un Blueprint per volta.** Due `.uasset` non si fondono: se una seconda sessione ha lo stesso file
   aperto, uno dei due lavori si perde senza conflitto e senza avviso. I quattro eroi sono quattro file
   distinti, quindi il lavoro è parallelizzabile — ma non sullo stesso.

**Verifica (PIE)**: al lock-in del turno, durante la fase **Move** l'unità passa a `Jog_Fwd` (`Run_Fwd` per
Gadget) mentre scorre lungo il percorso, e torna a `Idle` a fine risoluzione. Voce `PIE-AS4a`.

⚠️ **Che il flag si accenda e si spenga nei momenti giusti è già coperto headless** da
`RTPlaybackPhaseTests` — «nel Dash l'unità che scatta è in corsa», «nel Blast non è più in corsa», «a fine
risoluzione nessuna corsa residua». Quei test misurano il **contratto** che il grafo consuma, non
l'animazione: se al PIE la mesh non corre mentre quei test sono verdi, il difetto **non è nel C++ del
playback**. Da quando il grafo è in `URTUnitAnimInstance` i sospetti sono due, in quest'ordine: la
**Skeletal Mesh non agganciata** ([#1719](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1719))
e una **clip non in loop**.

---

## AS.4b — Colpi e morte (montaggi via eventi C++)

Il C++ è pronto (commit `feat(unit): eventi montaggio colpi/morte`): `RTUnit` espone 3 eventi —
`PlayAttackMontage`, `PlayHitMontage`, `PlayDefeatMontage` — che il `TurnManager` chiama **da solo**
sull'attaccante e sul bersaglio (colpi risolti) e sull'unità che muore. **Niente bind/branch/cast**: implementi
solo i 3 eventi nel BP (uniforme per ogni personaggio; se un evento non è implementato, nessun effetto — invariante #1).

1. **Montaggi** (tasto destro sull'anim ▸ **Create ▸ Create AnimMontage**):
   - *Esempio del prototipo 2026-08-03, **fuori roster**, tenuto per il procedimento e non per i nomi*:
     Gideon `Cast`→`AM_Gideon_Attack`, `HitReact_Front`→`AM_Gideon_Hit`, `Death_Fwd`→`AM_Gideon_Death`;
     Sparrow attacco (tiro), `HitReact`, `Death` → `AM_Sparrow_*`. Il roster della v0.1 è la riga qui sotto.
   - **Per il roster della v0.1**, clip di partenza dalle righe *Attacco*, *Colpito* e *Morte* di
     [AS.3b](#as3b--le-clip-dei-quattro-pack-del-roster). Il nome del montaggio segue il **pack** come tutto il
     resto (`AM_Gadget_Attack`, `AM_Gadget_Hit`, `AM_Gadget_Death`, e così per Phase, Riktor, Wraith), e le
     **sei** caselle che non si chiamano come ci si aspetta stanno lì — *questa riga diceva «sette»
     fino al 2026-08-25, e la tabella di [AS.3b](#as3b--le-clip-dei-quattro-pack-del-roster) ne conta
     **sei**, coerente con il suo «14 su 20»; qui ne sono elencate tre* — `Hitreact_Fwd` con la `r` minuscola per
     Gadget, `Death` nudo per Phase, `Death_Forward` per Wraith.
2. ✅ **Lo slot NON va inserito: c'è già.** Questo punto diceva *«Slot in `ABP_Gideon`/`ABP_Sparrow`
   ▸ AnimGraph: inserisci un nodo Slot 'DefaultSlot'»*, ed è caduto il 2026-08-25 insieme agli AnimBP:
   `FRTUnitAnimProxy::Initialize` monta già `Slot.SlotName = FAnimSlotGroup::DefaultSlotName` come radice
   del grafo, a valle del blend Idle/Run. È esattamente lo slot che `Play Anim Montage` usa quando non
   gliene si passa un altro, quindi i montaggi entrano **in override** su idle e corsa senza altro lavoro.
3. **`BP_Unit_<Eroe>`** (Gadget · Phase · Riktor · Wraith — ⚠️ *diceva "Guardian/Ranger": quei due Blueprint non esistono*) ▸ Event Graph: aggiungi gli eventi (tasto destro ▸ cerca il nome) e collega
   ciascuno a un **Play Anim Montage** (target: lo Skeletal Mesh Component):
   - **Event Play Attack Montage** → `AM_…_Attack`
   - **Event Play Hit Montage** → `AM_…_Hit`
   - **Event Play Defeat Montage** → `AM_…_Death`
   - **Compile ▸ Save**.
4. **PIE**: colpo → attacco + reazione del bersaglio; morte → caduta, sincronizzati col playback. Voce `PIE-AS4b`.

> Sincronia col resolver: gli eventi arrivano già al momento giusto (Blast per i colpi, fine fase per la morte).
> L'animazione **riproduce**, non decide (invariante #1).

---

## Facing (fatto in C++)

`bFaceMovementDirection` (default off) è pronto su `RTUnit`. Su **`BP_Unit_<Eroe>`** ▸ Class Defaults
spunta **`Face Movement Direction`**: l'unità si orienterà verso la direzione di corsa, così `Jog_Fwd`
(`Run_Fwd` per Gadget), che va in avanti, è credibile in ogni direzione. Solo presentazione: non tocca la
logica.

⚠️ *Questa riga diceva `BP_Unit_Guardian`: quel Blueprint non esiste ed è fuori roster
([D-120](../../decisions/RT_PDR_00_Decision_Log.md)). Corretta il 2026-08-30,
[#1720](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1720).*

🔴 **Se il personaggio corre di traverso il colpevole NON è questo flag, ma `MeshYawOffset`**: le due
cose si somigliano a schermo e si separano con `FacingArrow`, che segue l'**attore**. Se la freccia punta
dove ti aspetti ma la mesh guarda altrove, il difetto è nell'offset — vedi il punto 3 di
[AS.4a](#as4a--locomozione-idle--run-obiettivo-i-quattro-eroi-corrono-nel-move).

---

## Ripetere per un eroe in più — ⚠️ sezione storica: non è più un duplicato

🔴 **Sezione storica dal 2026-08-25: il *metodo* del duplicato regge, i suoi cinque passi no.** Sparrow è
fuori dal roster ([D-120](../../decisions/RT_PDR_00_Decision_Log.md)); `BP_Unit_Guardian` e `BP_Unit_Ranger` **non
esistono** — in `Content/RT/Characters/` ci sono i quattro `BP_Unit_<Eroe>`; il passo 4 rincorre un problema che
[AS.4a](#as4a--locomozione-idle--run-obiettivo-i-quattro-eroi-corrono-nel-move) ha eliminato (con `bIsMovingVisually` nel
BP_Unit non c'è nessun cast all'AnimBP da correggere); e il passo 5 assegna una `Ranger Unit Class` che il
GameMode non ha — oggi è `HeroUnitClasses`, e dal 2026-08-25 la popola il costruttore C++
([#287](https://github.com/DegrassiAaron/refactor-tactics-main/issues/287)). **Per il quinto eroe segui AS.4a**, che
è già scritta per ripetersi.

🔴 **Aggiornamento 2026-08-30 ([#1720](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1720)):
adesso non regge più nemmeno il metodo, e i sei passi sono stati tolti perché si leggevano come istruzioni.**
Duplicare un BP unità per cambiargli l'AnimBP era la via di quando il grafo stava in un `.uasset`. Da quando
sta in `URTUnitAnimInstance` **non c'è nessun AnimBP da duplicare**: un eroe nuovo non aggiunge un binario,
aggiunge **una riga di dati** — la sua coppia `Idle`/corsa in `ClipsPerHero` — e il grafo, la classe e lo
slot dei montaggi valgono già per lui.

*Cosa dicevano, per la provenienza*: duplicare `BP_Unit_Guardian` in `BP_Unit_Ranger`, creare `ABP_Sparrow`
sullo skeleton `Sparrow_Skeleton`, assegnarlo come `Anim Class`, correggere i due `Cast To ABP_Gideon`
ereditati nell'Event Graph e impostare una `Ranger Unit Class` sul GameMode. **Quattro di questi cinque
passi non hanno più un oggetto su cui operare**: quei due Blueprint non esistono, gli `ABP_*` non vanno
creati, il cast in Blueprint non c'è più (il proxy C++ legge `bIsMovingVisually` da sé) e la
`Ranger Unit Class` è oggi `HeroUnitClasses`, popolata dal costruttore C++
([#287](https://github.com/DegrassiAaron/refactor-tactics-main/issues/287)). Il testo integrale resta nella
storia di questo file.

**Per un eroe in più, oggi**: aggiungi la sua riga a `ClipsPerHero` in
`Source/RefactorTactics/Unit/RTUnitAnimInstance.cpp` — i nomi delle clip si **misurano sul disco**, non si
deducono ([AS.3b](#as3b--le-clip-dei-quattro-pack-del-roster) conta sei caselle su venti che divergono). Poi
aggancia la Skeletal Mesh al suo `BP_Unit_<Eroe>`, che è l'unico passo rimasto in editor
([AS.4a](#as4a--locomozione-idle--run-obiettivo-i-quattro-eroi-corrono-nel-move)).

## Nota — fix ordine-spawn (applicato)
`ARTGameMode::BeginPlay` ora spawna il `TurnManager` **prima** delle unità: in `BP_Unit` puoi agganciarti ai
delegate direttamente in BeginPlay **senza Delay**. Verificato sicuro (il TurnManager non usa le unità al proprio
BeginPlay, fa solo `StartPlanningTimer`) e ricompilato (70 test verdi).

---

## AS.5 — Identità di team: crea `M_TeamRing` (anello a terra)

Il C++ è pronto (`TeamRing` + `TeamColorFor`, 71 test verdi): sotto ogni unità c'è un anello che si colora per
squadra **se** trova il materiale `M_TeamRing`; senza, resta nascosto (fallback → cilindro colorato come prima).
Serve creare **un solo materiale** in editor.

1. **Content Browser ▸ `Materials`** ▸ tasto destro ▸ **Material** ▸ nome **`M_TeamRing`**.
2. Aprilo ▸ **Details ▸ Material ▸ Shading Model = `Unlit`** (colore pieno e brillante dall'alto).
3. Nel grafo: tasto destro ▸ **Parameter ▸ VectorParameter** ▸ nome **`Color`** (esattamente `Color`: è il parametro che il C++ imposta).
4. Collega **`Color` → Emissive Color**.
5. *(Opzionale, anello vero col buco centrale)* **Blend Mode = `Masked`** + una maschera radiale sull'**Opacity Mask**.
6. **Compile ▸ Save**.
7. Assegnalo: su ogni **`BP_Unit`** ▸ **Class Defaults ▸ `Team Ring Material` = `M_TeamRing`**.

**Verifica (PIE-AS5)**: sotto ogni unità un anello **blu** (team 0) / **rosso** (team 1), visibile dall'alto.
Se il disco è troppo grande/piccolo è il `RelativeScale3D` del `TeamRing` (ora `1.6` ≈ raggio 80 cm): chiedi la taratura.
