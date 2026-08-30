# Animazioni Paragon — orchestrazione delle issue · spec panel

> `CURRENT` · **Referto di revisione**, non owner. Consuma il kit
> *«CLAUDE — RefactorTactics · Animation Issue Orchestrator»*, arrivato come file nella radice del
> checkout di lavoro il 2026-08-30.
>
> **Data**: 2026-08-30 · **Base**: `origin/main` @ `8693b635` · **Modo**: critique · **Focus**: requirements + architecture
>
> **Cosa è**: il verdetto su un **mandato di scrittura su GitHub** — il kit chiede di misurare l'ownership
> delle issue sulle animazioni Paragon, aggiornare quelle esistenti, crearne di nuove e valutare una nuova
> Epic. `/sc:spec-panel` è task **documentale** ([`CLAUDE.md`](../../../CLAUDE.md) §6), quindi il referto è
> stato prodotto **prima** e senza toccare GitHub; le quattro scritture di §9 sono state poi **eseguite su
> autorizzazione esplicita dell'utente**, e §9 le registra con i numeri reali. **Nessun `D-nnn` è stato
> assegnato** — quello resta lavoro di [#1720](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1720).
>
> **Cosa non è**: un'autorità. Se una riga qui diverge dall'owner
> ([`roadmap-v0.1.md`](../roadmap-v0.1.md) § E21, il [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md),
> [`test-manuali-pie.md`](../../technical/test-manuali-pie.md)), **ha ragione l'owner**.
>
> **Archiviato in**: [`../../archive/src/handoff/2026-08-30-animation-issue-orchestrator.md`](../../archive/src/handoff/2026-08-30-animation-issue-orchestrator.md)

---

## 1. Il verdetto in una riga

> **Il kit chiede quattro `ABP_*` — ed è la via che `#288` ha misurato e scartato il 2026-08-25, mettendo
> il grafo di locomozione in C++. Il kit non sbaglia perché è disinformato: sbaglia perché tre documenti
> owner del repository dicono ancora la via vecchia, e chi lo esegue trova conferma ovunque tranne che nel
> codice.**

L'impianto di processo del kit è **giusto** — la regola di ownership §6, il divieto di Epic per simmetria §7,
il «non ottimizzare per il numero di issue» §19 sono esattamente le regole che questo repository applica già,
e che hanno prodotto `E21` con tre CP e non con dodici. Il suo §2 identifica correttamente i tre candidati
ownership, e tutti e tre sono ancora aperti e ancora corretti.

È la **mappa del lavoro** a essere scaduta di cinque giorni, e in una direzione che conta: il kit descrive
come da fare un lavoro che è **stato deciso diversamente**, e chiede asset (`ABP_Gadget`, `ABP_Phase`,
`ABP_Riktor`, `ABP_Wraith`) la cui creazione è oggi un **regresso** rispetto a `main`.

Il contributo che vale il consumo è in §7: **tre documenti owner contraddicono il codice**, la decisione che
li ha superati **non ha un `D-nnn`**, e nessuna issue possiede quella correzione.

---

## 2. Base di misura

Misurato su albero e lato server, non ricordato.

```text
Repo       : DegrassiAaron/refactor-tactics-main
Base       : origin/main @ 8693b635  (git fetch --prune eseguito)
Checkout   : D:/Repositories/refactor-tactics-technical-designer/refactor-tactics-main
             branch fix/1515-teamid-e-id-duplicati @ 9c8e85ae — NON e' main, e non ci si scrive
Albero     : una modifica non attribuibile a questa sessione — Source/.../RTScenarioLoader.cpp (M).
             Non toccata. Su questo checkout scrivono piu' sessioni (D-222)
Milestone  : «v0.1 · Leggibilita'» — 19 aperte / 11 chiuse
Epic #286  : OPEN · milestone «v0.1 · Leggibilita'» · CP #287 CLOSED, #288 e #289 OPEN
Suite      : NON eseguita. Nessuna riga di codice toccata, quindi niente da misurare (CLAUDE.md §7, D-222)
```

Il kit dichiara la clausola giusta — *«NON fidarti di numeri, path o stati copiati in questo file se GitHub o
il repository dicono qualcosa di diverso»* — e questo referto la applica alla lettera. È anche la clausola che
lo smonta.

---

## 3. Il panel

| Voce | Perché al tavolo |
|---|---|
| **Karl Wiegers** | i criteri del kit sono falsificabili? e quelli di `#288`, che il kit vorrebbe estendere? |
| **Martin Fowler** | dove vive il grafo di animazione: `.uasset` o codice, e chi possiede la scelta |
| **Michael Nygard** | il degrado progettato per il clone che diventa lo stato normale del pacchetto (`#1663`) |
| **Alistair Cockburn** | chi è il consumatore di ciascuna issue proposta, e se esiste |
| **Gojko Adzic** | gli esempi del kit contro le verifiche che il registro PIE esegue già |

---

## 4. La matrice di ownership (§4 del kit)

Costruita prima di proporre qualunque scrittura, come il kit richiede.

| Issue | Tipo | Stato | Scope reale | Overlap col kit | Azione |
|---|---|---|---|---|---|
| [#286](https://github.com/DegrassiAaron/refactor-tactics-main/issues/286) | Epic E21 | OPEN | come le **unità** appaiono in scena: mesh, animazioni, materiali, anelli | totale | **KEEP** — è l'owner semantico corretto, il body non va riscritto |
| [#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288) | CP E21.2 | OPEN | locomozione Idle↔Run + montaggi Cast/Hit/Death | §3 «Locomotion», §3 «Azioni discrete», §9-B, §9-C | **UPDATE** — il body cita `ABP_*`, che il CP stesso ha scartato |
| [#289](https://github.com/DegrassiAaron/refactor-tactics-main/issues/289) | CP E21.3 | OPEN | anelli, colori, camera | nessuno | **KEEP** — fuori tema, non toccare |
| [#287](https://github.com/DegrassiAaron/refactor-tactics-main/issues/287) | CP E21.1 | **CLOSED** 2026-08-25 | mesh sui centri esagonali | §3 «Asset / integrazione» | — chiuso, non riaprire |
| [#1663](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1663) | Bug | OPEN | le clip non entrano nel cook: 756 asset, **zero** animazioni | §3 «Packaging», §9-E | **KEEP** — possiede già il cook, con misure che il kit non ha |
| [#1665](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1665) | Bug | OPEN | la board è nera nel pacchetto | nessuno — stessa classe, altro asset | **KEEP** |
| [#1525](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1525) | Bug | OPEN | il playback anima anche le unità non viste | nessuno — è *conoscenza*, non presentazione | **KEEP** |

**Nessuna issue duplicata trovata.** La ricerca ha coperto i tredici concetti di §4 del kit più i loro
equivalenti italiani — il repository scrive le issue in italiano, e `animation`/`locomotion`/`AnimBP` in
`in:title` restituiscono **zero** righe: cercare solo in inglese avrebbe fatto concludere che il tema non
esiste, ed è la trappola che questa nota esiste per disarmare.

---

## 5. Lo stato reale, misurato

Il kit chiede (§5) un audit Unreal MCP read-only. **Non è stato necessario aprire l'Editor**: le tre domande
che l'audit avrebbe risposto hanno già una risposta versionata e più precisa.

| Domanda del kit | Risposta misurata | Fonte |
|---|---|---|
| esistono i `BP_Unit_*`? | **sì, tutti e quattro** | `Content/RT/Characters/{Gadget,Phase,Riktor,Wraith}/Blueprints/BP_Unit_*.uasset` |
| esistono gli `ABP_*`? | **no, e non devono esistere** | `find Content -iname "ABP_*"` → zero |
| chi assegna l'`AnimClass`? | **il C++**, allo spawn | `ARTUnit::ApplyUnitAnimClass()`, `RTUnit.h:1078` |
| il grafo dove vive? | in `URTUnitAnimInstance`, **senza nessun `.uasset`** | `Source/RefactorTactics/Unit/RTUnitAnimInstance.{h,cpp}` |
| le clip come si risolvono? | `TSoftObjectPtr` composti a runtime, default C++ per i quattro eroi | `RTUnitAnimInstance.cpp:33-36` |
| chi suona i montaggi? | il `TurnManager`, **da solo** | `RTTurnManager.cpp:6149, 6172, 6185, 6241` |

Il grafo montato a mano è questo, ed è già completo di slot per i montaggi:

```
Idle  ─┐
       ├─ TwoWayBlend ── Slot('DefaultSlot') ── Output
Run   ─┘      (alpha)         (Cast/Hit/Death)
```

**Classificazione delle capability** nel vocabolario che il kit chiede (§5):

| Capability | Classe | Perché |
|---|---|---|
| Idle ↔ Run, quattro eroi | **READY** (codice) · **MANUAL ART REVIEW** (tre eroi su quattro) | `PIE-AS4a` ✅ 2026-08-25 **solo su Gadget** |
| facing sulle sei direzioni | **READY** | E16 chiusa 2026-08-09; `MeshYawOffset` corregge i 90° di `AActor` |
| montaggi Cast / Hit / Death | **ASSET MISSING** | gli eventi C++ e lo slot esistono; i dodici `AM_*` no |
| clip nel pacchetto | **CODE REQUIRED** + decisione d'autore | `#1663`, e il nodo è dichiarato lì |
| Dodge, Stagger, pose Prepared | **fuori canone v0.1** | vedi §8 |

---

## 6. Dove il kit sbaglia: la via scartata

Il kit chiede, in §5 «Asset candidati», `ABP_Gadget` · `ABP_Phase` · `ABP_Riktor` · `ABP_Wraith`, e in §9-B
un checkpoint per produrli.

Quella via è stata **misurata e scartata** dal commit `1d0007a9` del 2026-08-25 — *«feat(288): il grafo di
locomozione vive in C++, e il repository non cresce di un byte»* — su un numero che il docstring di
`RTUnitAnimInstance.h` riporta in chiaro:

> gli AnimBlueprint dei pack pesano **650–735 KB l'uno, ~2,8 MB per quattro**, contro gli **0,7 MB** che pesa
> oggi tutto `Content/` versionato. I `.uasset` non si comprimono per delta, quindi il prezzo si ripaga a
> ogni salvataggio successivo.

⚠️ Creare oggi i quattro `ABP_*` non sarebbe lavoro nuovo: sarebbe **quadruplicare il contenuto binario
versionato** per sostituire un grafo che è già sotto test automatico (`FRTUnitAnimClipsTest`,
`RTUnitTests.cpp:438`) con quattro binari che non si diffano e non si fondono.

E il kit **non poteva saperlo dal repository**, perché il repository non lo dice dove si va a cercare: lo dice
in un docstring C++ e in una cella di tabella. È la ragione per cui §7 di questo referto è il contributo che
vale il consumo.

---

## 7. Tre documenti owner descrivono la via scartata

Questa è la lacuna reale, e non è nel kit: è nel repository.

| Documento | Riga | Cosa dice ancora | Cosa è vero |
|---|---|---|---|
| [`roadmap-v0.1.md`](../roadmap-v0.1.md) | `1519` | «**`ABP_*`** con Idle↔Run nella fase Move» | il grafo è `URTUnitAnimInstance`, in C++ |
| [`guida-animazioni-paragon.md`](../../technical/runbooks/guida-animazioni-paragon.md) | §AS.4a punti **1–4** | «Crea l'Animation Blueprint — uno per eroe… Nome: `ABP_<Eroe>`», poi variabile `bIsMoving`, state machine `Locomotion`, `Anim Class = ABP_<Eroe>` | niente di tutto ciò va fatto: `ApplyUnitAnimClass` assegna la classe C++ allo spawn |
| ↑ stessa guida | §AS.4b punto **2** | «Slot in `ABP_Gideon`/`ABP_Sparrow` ▸ AnimGraph: inserisci un nodo **Slot 'DefaultSlot'**» | lo slot **è già nel grafo C++** dal 2026-08-25 (`Slot.SlotName = FAnimSlotGroup::DefaultSlotName`) |
| [`roadmap-editor.md`](../roadmap-editor.md) | `107-108`, `258-288` | seduta **U7** produce `BP_Unit_Guardian` / `BP_Unit_Ranger`; **U8** produce `ABP_Gideon`, `ABP_Sparrow` | quei quattro asset non esistono e sono **fuori roster** (D-120: Gadget · Phase · Riktor · Wraith); i `BP_Unit_*` reali sono già committati |
| ↑ stessa guida | § «Facing (fatto in C++)» | «Su `BP_Unit_Guardian` ▸ Class Defaults…» | stesso roster fuori scadenza |

🔴 **Il caso più insidioso è §AS.4a**, perché porta in testa un avviso rosso datato **2026-08-25** che
dichiara la sezione *riscritta* e corregge tre errori — `ABP_Gideon`, `BP_Unit_Guardian`, il wiring dei
delegate. Quella riscrittura ha corretto i **nomi** e non la **via**: poche ore dopo, lo stesso `#288` ha
spostato il grafo in C++. Un documento che si dichiara appena riverificato è più credibile di uno silente,
e per questo sbaglia più a fondo.

Il registro PIE, invece, è **corretto e aggiornato** — `test-manuali-pie.md:545` scrive *«la via che `#288`
ha scartato»*. Il repository sa la cosa giusta in un punto solo, e nei tre punti dove un esecutore va a
leggere dice l'altra.

> 🔴 **Aggiornamento del 2026-08-30, a esecuzione avvenuta: i documenti non erano quattro, erano NOVE.**
> [#1720](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1720) è chiusa e la decisione ha il suo
> ID — **[D-248](../../decisions/RT_PDR_00_Decision_Log.md)**. Oltre ai quattro di questa tabella sono risultati
> scaduti anche `editor-sessions.yaml` (U8: `produces`, `artifacts`, `done_when` e la procedura),
> `asset-map.md` (quattro righe di backlog `⏳ assente` assegnate a **U8**, più la riga E35),
> `spec-asset-pipeline.md` (§7.5, riga AS.2, «Aperto — AS.2»), `convenzioni-contenuti-ue.md` §5b,
> `adr-0001-skeletal-unit.md` punto 1 e `roadmap-checkpoint.md` § M8.2.
>
> ⚠️ **Dove questa analisi ha guardato male, e vale come metodo**: ha letto i documenti che un esecutore
> *apre* — roadmap e guida — e non quelli da cui *prende il lavoro*. I due più actionable erano proprio i
> mancanti: un campo `artifacts:` con quattro path e una tabella di backlog con una seduta assegnata. La
> ricerca era `grep` sui `.md`, e `editor-sessions.yaml` non è un `.md`.

**E la decisione non ha un `D-nnn`.** Il Decision Log non contiene nessuna voce su `AnimBlueprint`,
`AnimInstance` o grafo di animazione: la scelta vive solo nel docstring di un header e in una cella di
tabella. È esattamente la classe di deriva che il Decision Log esiste per fermare, e ha già prodotto
**quattro** documenti scaduti e **un kit** che li ha creduti.

---

## 8. Lo scope del kit che il canone v0.1 non ha

Il kit elenca (§3) un perimetro largo. Metà non ha owner nel canone, e crearne uno sarebbe lavoro inventato:

| Voce del kit | Stato reale |
|---|---|
| Idle / Walk-Jog / Run, forward, ritorno a Idle | **nel canone** — `#288`, due clip per eroe (`Idle` + corsa) |
| facing sulle sei direzioni hex | **fatto** — E16, chiusa 2026-08-09 |
| Backward, Strafe, start/stop, **Blend Space** | **fuori canone**: il grafo ha due stati e un blend, il playback non produce backward né strafe |
| Basic Attack, Cast, Hit Reaction, Death / KO | **nel canone** — `#288`, eventi C++ già chiamati dal TurnManager |
| **Dodge / Evade** | il canone ha il **Dash** (`docs/gameplay/spec-dash.md`), e `bIsMovingVisually` lo copre già come locomozione (`RTTurnManager.cpp:6104`). Non serve una clip dedicata né una issue |
| **Stagger / Knockback** | lo spostamento forzato esiste nel TurnLog (`TurnLog.DisplacementHasCauseAndSource`), **nessuna presentazione dedicata** è richiesta dalla v0.1 |
| **pose Prepared (Guard / Brace / Overwatch)** | ⛔ l'Overwatch in partita si chiude in `HoldNoDecider` — *nessuno può ancora armarlo* (E14.6, `#166`). Una posa per uno stato che nessuno produce non è verificabile |
| Animation Blueprint originali Paragon / RefactorTactics | **da non fare** — §6 |
| assegnazione `AnimClass` | **fatta in C++** — `ApplyUnitAnimClass` |
| Data Asset / profili di presentation | il kit lo propone in §16 (`RT.Action.Dodge → profile RT → clip`). Oggi la mappa è `TMap<FName, FRTLocomotionClips>` con default C++ ed è già indipendente dai nomi Paragon lato gameplay: **il principio è già rispettato**, il livello di indirezione in più non ha un consumatore |

**Wiegers**: cinque di queste voci non hanno un criterio falsificabile perché non hanno uno stato di gioco che
le produca. Scriverle in una DoD le renderebbe **permanentemente aperte**.

---

## 9. Scritture GitHub — proposte e poi eseguite

Quattro scritture, nessuna creazione di Epic, nessuna chiusura. **Eseguite il 2026-08-30** su
autorizzazione esplicita, dopo la consegna del referto.

| Tipo | Issue | Azione | Motivazione | Esito |
|---|---|---|---|---|
| **UPDATE** | [#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288) | correggere il body: `ABP_*` → `URTUnitAnimInstance`; separare lo stato reale di `PIE-AS4a` (✅ **su Gadget**, ⏳ sugli altri tre) da `PIE-AS4b` (⏳, restano i dodici `AM_*`) | il body dice ancora la via che il CP stesso ha scartato, ed è il primo documento che un esecutore apre | ✅ fatto, **additivo**: le due affermazioni false restano leggibili accanto alla correzione |
| **CREATE** | [#1719](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1719) | *«Tre eroi su quattro hanno la skeletal sotto il cilindro: le clip ci sono, la mesh no»* — figlia di #286, `Refs #288` | è l'**unico gap con lavoro reale e senza owner**: `PIE-AS4a` è verde su Gadget e i tre restanti *«si vedono deformati»* (`test-manuali-pie.md:545`). Vive nella seduta **U7**, che **non ha issue GitHub** | ✅ creata · `v0.1`, `P1` · milestone «v0.1 · Leggibilità» |
| **CREATE** | [#1720](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1720) | *«Quattro documenti mandano a costruire gli `ABP_*` che #288 ha scartato, e la decisione non ha un `D-nnn`»* — figlia di #286 | §7. Cinque correzioni puntuali in quattro file più una voce nel Decision Log; gate falsificabile: `grep -rn "ABP_" docs/` non restituisce più una **istruzione** | ✅ creata · `v0.1`, `documentation`, `P1` |
| **UPDATE** | [#286](https://github.com/DegrassiAaron/refactor-tactics-main/issues/286) | aggiungere #1719 e #1720 come lavoro collegato, con il perché non sono checkpoint (**D-153**) | l'Epic è l'owner: due figlie invisibili nel body sono due figlie che nessuno trova | ✅ fatto, **inserzione additiva** prima di «Gate di chiusura» — il gate **non** si allarga |
| **LINK** | [#1663](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1663) → #288 | `Refs`, non `Depends on` | il cook non blocca la locomozione in PIE: sono due gate diversi (PIE vs packaged), e il kit stesso §13 vieta le dipendenze per affinità tematica | ✅ [commento](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1663#issuecomment-5465356498), con l'avviso che i dodici montaggi porterebbero il perimetro da 8 a 20 clip |
| **CLOSE DUPLICATE** | — | nessuna | non sono stati trovati duplicati | — |

⛔ **Non creare** — valutati e respinti, con la ragione:

- **«Paragon Animation Audit»** (kit §9-A): l'inventario esiste, misurato e versionato in §AS.3b della guida
  — venti caselle, sei divergenti. Una issue per rifarlo è lavoro già fatto.
- **«AnimBP locomotion»** (kit §9-B): produrrebbe i quattro `ABP_*`. Vedi §6.
- **«Playback Blast» separato** (kit §9-C): i montaggi sono **dentro** la DoD di `#288`, che li nomina.
  Separarli spezzerebbe un checkpoint la cui unica verifica residua è `PIE-AS4b`.
- **«Dodge / movement presentation»** (kit §9-D): il Dash è già coperto da `bIsMovingVisually`.
- **«Cook subset Paragon»** (kit §9-E): è `#1663`, che ha misure che il kit non ha — 756 asset, zero clip,
  otto `SkipPackage`, 4550 file su disco. Il kit stesso dice di non duplicarlo, ed è la sua riga migliore.
- **«Verification»** (kit §9-F): le verifiche stanno nelle voci PIE, che hanno già un registro e una sessione.

---

## 10. Epic decision

**`NEW EPIC REQUIRED: NO`**

Tutte e sette le condizioni di §8 del kit falliscono, e la prima basta: **#286 possiede semanticamente
l'intero gruppo di lavoro**. Il suo body lo dichiara — *«come le unità appaiono in scena: mesh, animazioni,
materiali, anelli»* — e distingue già il proprio perimetro da E11 (widget) e dal contratto graybox (ciò che
sta sulla mappa).

Il kit prevede anche il caso contrario, in §7, con sei righe che descrivono esattamente questa situazione:
*«NON creare una nuova Epic se #286 / E21 possiede già Presentation e animazioni… se il problema è soltanto
packaging/cook… se il lavoro è una singola integrazione AnimBP»*. Applicata alla lettera, la sua stessa
regola vieta l'Epic che la sua §5 lascia intravedere.

E il precedente esiste: **D-153 vieta le epic e i CP creati per simmetria**, ed è già stato pagato una volta
su questa stessa epic — il kit graybox si è innestato su owner esistenti invece di diventare `E21.4`.

---

## 11. Ordine di esecuzione

I punti 1–4 sono **fatti** (§9). Restano i punti 5 e 6, che sono lavoro.

1. ~~UPDATE #288~~ ✅ — era il documento che un esecutore apre per primo, e lo mandava a sbattere.
2. ~~CREATE la issue dei documenti scaduti~~ ✅ **#1720**.
3. ~~CREATE la issue delle tre skeletal~~ ✅ **#1719**.
4. ~~LINK #1663 ↔ #288 come `Refs`~~ ✅.
5. **#1720 per prima** — finché `roadmap-v0.1.md`, la guida e `roadmap-editor.md` dicono `ABP_*`, ogni kit
   successivo ripeterà questo errore. Include l'assegnazione del `D-nnn` alla scelta del grafo in C++.
6. **#1719** — sblocca `PIE-AS4a` su Phase, Riktor e Wraith, e la riconferma di `#289` sulla skeletal.
7. Poi, e solo poi, il lavoro in editor che resta in #288: i dodici montaggi
   `AM_<Pack>_{Attack,Hit,Death}` per `PIE-AS4b`.

**Blocker residui**: il nodo d'autore di `#1663` — quali clip entrano nel cook e con quale meccanismo, dato
che `Content/FabAsset/` è gitignorato e pesa ~44 GB. Non è risolvibile dai dati, ed è dichiarato lì.

**Prossimo task concreto con Unreal MCP**: nessuno in scrittura. L'audit read-only che il kit chiede in §5 è
già soddisfatto da §5 di questo referto, su fonti versionate; il primo lavoro che richiede davvero l'Editor è
il punto 5 — la creazione dei montaggi — e non va iniziato prima dei punti 1-3.

---

## 12. Cosa non è stato fatto, e perché

- **Le mutazioni GitHub non sono state fatte in automatico.** Il kit §17 autorizza a procedere
  autonomamente, ma `/sc:spec-panel` è documentale per [`CLAUDE.md`](../../../CLAUDE.md) §6, e la stessa §7
  dice che *«un handoff/audit non è autorità e non autorizza da solo a implementare tutto ciò che
  contiene»*. Il referto è stato consegnato **prima**, le quattro scritture sono state eseguite **dopo**,
  su una parola esplicita. La distinzione conta: l'ordine giusto qui non era «misura poi scrivi», era
  «misura, **fai vedere la misura**, poi scrivi» — perché la misura contraddiceva il mandato.
- **Nessun `D-nnn` assegnato.** L'ID per la decisione «il grafo vive in C++» va preso al momento della
  scrittura e riverificato sui ref remoti (CLAUDE.md §7): un numero fermo in un documento scade in ore.
- **Nessun commit.** Il checkout è su `fix/1515-teamid-e-id-duplicati`, che non è `main`, e porta una
  modifica C++ di un'altra sessione. I due file di questo lavoro sono nuovi e non tracciati.
- **Suite non eseguita.** Nessuna riga di codice toccata: non c'è misura da produrre (D-222).
