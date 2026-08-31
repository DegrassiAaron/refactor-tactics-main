# Kit «Combat Shot Presentation v0.1» — spec panel contro il repository

> **Referto di revisione**, non owner. Sottopone a critique il documento d'autore *RefactorTactics — Claude
> Issue / Epic / Roadmap Orchestrator* (prompt operativo + *Checkpoint iniziale — Combat Shot Presentation
> v0.1*) e ne misura ogni claim contro il repository.
>
> **Data**: 2026-08-31 · **Modo**: critique · **Focus**: requirements + architecture
> **Base della revisione**: `origin/main` `8bfe8f84`, albero pulito salvo il referto gemello di oggi
> (`movement-microsteps-facing-pivot-spec-panel-2026-08-31.md`, untracked).
>
> ⛔ **Nessuna issue creata o modificata, nessun owner doc toccato, nessuna suite eseguita, nessuna riga di
> `Source/` cambiata.** Le mutazioni GitHub proposte in §8 attendono conferma.

---

## 1. Il verdetto in una riga

> **Il checkpoint è accurato su tutto ciò che afferma — sette issue su sette esistono con lo stato
> dichiarato — e il gap che intuisce è reale. Ma lo colloca nel posto sbagliato: non manca una cue, manca
> un CAMPO. Il confine simulazione → presentazione che `D-278` dichiara canonico e «non spostabile» non
> trasporta le celle di un attacco, quindi le tre clausole Area/Line/Cone dello scope candidato sono
> irrealizzabili senza violare la clausola di non-duplicazione della issue stessa. E l'owner del contratto
> esiste già — `#1801` — e il checkpoint non lo nomina.**

---

## 2. Ciò che il checkpoint dice, e che è vero

Il documento chiede esplicitamente di rimisurare il proprio checkpoint prima di mutare GitHub. Misurato:

| Claim del checkpoint | Esito | Evidenza |
|---|---|---|
| E11 `#25` possiede HUD, Ghost Timeline, planning visuale | ✅ | `#25` OPEN, `[EPIC v0.1] E11 — HUD, log e debug`, milestone *v0.1 · Leggibilità* |
| `#172` possiede preview per fase, `TargetCells`, `AffectedCells`, `Certainty` | ✅ **alla lettera** | DoD di `#172`: *«`Phase`, `UnitId`, `ActionId`, `PreviewOrigin`, `PreviewDestination`, `Facing`, `PoseId`, `TargetCells`, `AffectedCells`, `Certainty`»* |
| `#172` copre Single/Area/Line/Cone senza issue separate | ✅ | la DoD lega le celle a `HexHitCells`, che è parametrica su `ERTAbilityShape` (`RTHexCombatLibrary.h:290`) |
| `#173` possiede scrubbing e `ReactionPreview` come ramo, non quinta fase | ✅ | `#173` OPEN, `CP 11.6 — Scrubbing delle fasi e ramo condizionale della reaction` |
| E21 `#286` possiede ciò che appare in scena | ✅ | `#286` OPEN, e il body: *«E11 è l'interfaccia … qui si parla di come le unità appaiono in scena»* |
| `#288` possiede Cast/Hit/Death, VFX fuori scope | ✅ | *Fuori scope: «VFX ed effetti di stato: non hanno un evento a cui agganciarsi»* |
| `#911` ha già corretto lo scaglionamento via `AttackShowSeconds` | ✅ | `#911` CLOSED; `URTPlaybackLibrary::AttacksToShow` (`RTPlaybackLibrary.cpp:31`), tre test verdi |
| «non creare un secondo timer di presentazione» | ✅ **e la regola è giusta** | `AttackShowSeconds = 0.50f` (`RTTurnManager.h:681`), consumato in `RTTurnManager.cpp:6245` |

∴ **Il checkpoint non contiene affermazioni false.** È la parte rara: la maggior parte dei kit revisionati in
questa cartella sbaglia il primo nome del proprio elenco. Questo no.

Il problema non è l'accuratezza. È **la profondità dell'audit che il documento prescrive a sé stesso** — e che
si ferma un livello sopra il punto in cui la decisione si gioca.

---

## 3. 🔴 Il difetto centrale: il confine non porta il dato

Lo scope candidato chiede quattro rese e una garanzia:

> *«resa dell'area effettivamente risolta per Area; resa della linea effettivamente risolta per Line; resa
> del cono effettivamente risolto per Cone; […] usare l'esito e le celle prodotti dal resolver, senza
> ricalcolare gameplay nel VFX»*

`D-278` (**accettata il 2026-08-30**, cioè *ieri*) fissa il canale e lo dichiara immobile:

> *«`FRTResolvedEvent` resta il confine simulazione → presentazione»*

**Misurato — `Source/RefactorTactics/Turn/RTResolvedEvent.h:40-76`:**

| Campo | Tipo | Nota del sorgente |
|---|---|---|
| `Phase` | `ERTMatchPhase` | |
| `Type` | `ERTResolvedEventType` | **quattro valori**: `Move · Attack · HazardDamage · Defeated` |
| `SourceStableUnitId` | `int32` | `0` = nessuno |
| `TargetStableUnitId` | `int32` | *«Solo `Attack` lo valorizza»* |
| `Path` | `TArray<FRTCellId>` | 🔴 *«**Per Move**: la rotta in celle»* |
| `Amount` | `int32` | danno/scudo/durata |

**Non esiste un campo che porti le celle di un attacco.** `Path` è documentato per `Move`. `ERTResolvedEventType`
non distingue Single da Area, Line o Cone.

E il punto di emissione lo conferma — `RTTurnManager.cpp:4867-4873`:

```cpp
// Evento per il playback: colpo Attacker -> Victim (mostrato nel Blast).
FRTResolvedEvent Ev;
Ev.Phase  = ERTMatchPhase::Blast;
Ev.Type   = ERTResolvedEventType::Attack;
Ev.SourceStableUnitId = Attacker ? Attacker->StableUnitId : 0;
Ev.TargetStableUnitId = Victim   ? Victim->StableUnitId   : 0;
Ev.Amount = Hit.Power;
```

L'evento nasce **per vittima**. Un'AoE su tre bersagli emette tre eventi `Attack` indistinguibili da tre colpi
singoli; **un'AoE che investe celle vuote non emette nulla**, perché non c'è `Victim`.

### Perché è un difetto della spec e non un dettaglio implementativo

Chi prende in mano la issue così com'è scritta trova **due sole strade, ed entrambe sono vietate**:

| Strada | Cosa la vieta |
|---|---|
| Ricalcolare `HexHitCells(Shape, From, Target, …)` dentro il VFX | la clausola *«senza ricalcolare gameplay nel VFX»* della issue stessa; l'invariante *«nessuna implementazione parallela delle primitive canoniche dentro renderer/widget/Niagara»* del prompt; `D-124` (*«la presentazione resta consumer del resolver, mai autorità dell'esito»*) |
| Passare dal delegate `OnAttackResolved` | `FRTAttackPlaybackSignature` è `(ARTUnit* Source, ARTUnit* Target, int32 Amount)` — `RTTurnManager.h:156`. Non ha le celle, e non ha un `Target` per una cella vuota |

∴ **La issue proposta, se aperta con questo scope, si chiude in un modo solo: violando la propria DoD.**
È il fallimento peggiore per una spec — non essere impossibile, ma essere *silenziosamente* impossibile,
lasciando che sia chi implementa a scoprirlo e a scegliere quale regola rompere.

**Il gap reale è un livello sotto**: non *«il colpo non si vede»*, ma ***«il confine non porta ciò che serve
per mostrarlo»***. È un gap di **contratto**, in C++, non di cue in editor.

---

## 4. L'owner che il checkpoint non nomina — `#1801`

Il checkpoint chiede: *«verificare se esiste già un owner per la resa del colpo DURANTE la Resolution»*.
La risposta è **parzialmente sì**, e sta dove il checkpoint non ha cercato.

**[`#1801`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1801) — OPEN, P1** —
*«presentazione: validator di esaustività per il binding ResolvedEvent → presentazione»*.

Possiede, per proprio dichiarato scope: **il contratto e il suo gate**. Discende da `D-278` / `AUTHOR-PRES-001`
e chiude `FX-1` + `FX-2`. Il suo *Out of scope* dice: *«nessun asset e nessuna cue concreta vengono decisi qui»*.

Quindi la ripartizione misurata è a **tre livelli**, non due:

| Livello | Owner | Stato |
|---|---|---|
| Il **contratto** evento → presentazione, e il gate di esaustività | `#1801` (D-278) | ✅ ha owner, OPEN |
| Il **payload**: l'evento `Attack` porta le celle risolte e la forma | ⛔ **nessuno** | il gap reale |
| La **cue**: tracer, impatto, resa dell'area | ⛔ nessuno | ma non costruibile prima del payload |

### E c'è una sequenza obbligata che il checkpoint inverte

`#1801` impone: *«Un `ERTResolvedEventType` nuovo senza copertura fa **FALLIRE** validatore o Automation Test»*.

Se il payload si risolvesse aggiungendo valori all'enum (per distinguere l'area dal colpo singolo), quel
lavoro **innesca il gate di `#1801`** — che oggi non esiste ancora. Il checkpoint colloca la issue E21 al
**terzo** posto del proprio ordine e non nomina `#1801` affatto: eseguito così, l'ordine produce un enum
allargato mentre il guardiano che dovrebbe sorvegliarlo è ancora da scrivere.

⚠️ `#1801` è **senza milestone**. Se il payload del colpo è lavoro v0.1, il suo prerequisito non può restare
fuori dalla release.

---

## 5. Il ceiling di `D-124`, e perché lo scope candidato lo sfiora

`D-124` fissa il perimetro di E21 e mette **fuori**:

> *«sistema VFX completo per tutti gli status, **Niagara dedicato a ogni abilità**, foot IK raffinato,
> locomotion set bespoke, cinematic death, e ogni presentation framework che nessun gate v0.1 misura»*

e avverte, nella parte che la voce stessa chiama *operativa*:

> ⚠️ *«E21 è l'unica epic della v0.1 il cui DoD non è chiudibile in automation, quindi senza un confine
> scritto "leggibile" si allarga a "bello" a costo zero apparente»*

Lo scope candidato chiede **quattro rese distinte** — Single, Area, Line, Cone — più tracer, beam o
proiettile, più impatto. Misurato:

- **`Content/` non contiene un solo asset Niagara**: la ricerca per `*niagara*`, `NS_*`, `NE_*` restituisce
  **zero**. Il tracer sarebbe il primo effetto di quella famiglia nel progetto.
- [`#1663`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1663) è **OPEN**: *«756 asset
  Paragon cotti, **zero** animazioni»*.
- `D-297` (**oggi**, panel `/sc:spec-panel`) misura che la tabella *abilità → FX Paragon* è costruibile per
  **un eroe su quattro**: solo `ParagonGadget/FX/Abilities/` segmenta per abilità; Phase, Riktor e Wraith
  hanno `FX/` organizzato per tipo di asset, *«senza alcuna partizione per abilità»*.

∴ Una issue che chieda quattro forme di resa attinge a una base asset che **non esiste per tre eroi su
quattro**, e lo fa dentro l'unica epic che `D-124` protegge con un ceiling scritto proprio contro questo.

---

## 6. Il vicinato in E21 che il checkpoint non ha guardato

Il checkpoint elenca `#286`, `#288` e si ferma. La catena E21 ha un terzo checkpoint e due adiacenze:

| Issue | Perché conta qui |
|---|---|
| **`#289`** — CP E21.3 · Leggibilità tattica | 🔴 **non citata dal checkpoint.** È il vicino più prossimo: possiede anelli, colori di superficie in partita, camera, e il confine fra celle (`#1758`). **Non** possiede il colpo — e delimita da sé: *«La leggibilità dell'anteprima di tiro è `PIE-PREVIEW-AREA` ed è E11»* |
| **`#1392`** — il colpo di boundary rispetta la copertura | La sua **parte 3** atterra esplicitamente in **E21 `#286`**, *«vicino a `PIE-PREVIEW-AREA`»*. Se nasce una issue di resa del colpo in E21 senza legarsi a questa, si creano due owner per «cosa si capisce guardando un colpo» |
| **`#1936`** — il log a schermo racconta le celle, non la partita | E11, feed del giocatore: l'altra metà della leggibilità del colpo. Un tracer senza il feed, o viceversa, risolve mezza domanda |

`PIE-PREVIEW-AREA` è ✅ **dal 2026-08-09**, con una lezione registrata che vale come vincolo per chiunque
implementi la cue — `docs/technical/test-manuali-pie.md:1000`:

> *«**(a)** L'arancione era disegnato con `DepthPriority=SDPG_World` a 2,5 unità dal piano della cella: il
> cilindro lo copriva … **(b)** Si vedeva ma non rispondeva alla domanda: l'anteprima parla di **celle**, chi
> guarda chiede "questo cilindro lo prendo?"»*

⚠️ E il numero è **cambiato da allora**: `#289` avverte che `RTCellTopZ` è salito da `2,5` a `7,5` uu il
2026-08-28, *«quindi un numero riscritto a mano oggi sbaglia di più di allora»*.

---

## 7. Critique del prompt orchestratore (non solo del checkpoint)

Il documento è anche una **specifica di processo**. Sotto critique, tre difetti di forma e uno di merito.

### 7.1 ✅ Ciò che regge

La gerarchia delle fonti, la matrice REUSE/UPDATE/CREATE/DEFER/REJECT, il divieto di Epic per simmetria e
l'elenco degli invarianti non negoziabili sono coerenti con `AGENTS.md`, `CLAUDE.md` §3-§4 e il Decision Log.
Il prompt **si è auto-applicato correttamente**: ha prodotto un checkpoint senza affermazioni false.

### 7.2 ⚠️ *(Wiegers — testabilità)* «rimisuralo» non è un criterio

Il documento dice *«questo checkpoint è un punto di partenza, non una verità eterna: rimisuralo»*, ma non dice
**contro cosa** né **con quale profondità**. È esattamente la differenza fra l'audit che il checkpoint ha
svolto — issue e titoli, tutti corretti — e quello che avrebbe scoperto §3: il confine e i suoi campi.

📝 **Raccomandazione**: la clausola d'audit dovrebbe imporre di **scendere fino al tipo di dato che attraversa
il confine** quando la issue proposta è di presentazione. *«Verifica che il canale canonico trasporti il dato
che la DoD richiede»* è falsificabile; *«rimisura»* no.

### 7.3 ⚠️ *(Nygard — modi di fallimento)* Manca la clausola di fallimento del dato

L'elenco dei campi obbligatori del body — quindici voci, da *«privacy impact»* a *«debug/log evidence»* — non
contiene **«da quale canale arriva il dato, e quel canale lo porta già?»**. È il campo la cui assenza ha
prodotto §3.

### 7.4 🔴 *(Cockburn — attore primario)* Il documento chiede a Claude di mutare GitHub senza un gate umano

> *«Non fermarti a un piano teorico. Dopo l'audit, se hai accesso alle integrazioni necessarie, **esegui
> realmente** le create/update»*

Il precedente immediato dice il contrario, ed è di **oggi**: il referto gemello
`movement-microsteps-facing-pivot-spec-panel-2026-08-31.md` registra *«Tracking eseguito **dopo conferma**»*.
La convenzione viva di questa cartella è **referto → conferma → mutazione**. Questo referto la rispetta: §8
propone, non esegue.

📝 **Raccomandazione**: sostituire *«esegui realmente»* con *«presenta la matrice ed esegui dopo conferma
esplicita»*. Una spec che autorizza mutazioni non revisionate su un tracker condiviso da sessioni parallele
(`CLAUDE.md` §7) è un rischio di collisione, non un'ottimizzazione.

---

## 8. Matrice, e mutazioni proposte

⛔ **Nessuna eseguita.** Attendono conferma.

| Proposta del checkpoint | Verdetto | Motivo |
|---|---|---|
| Nuova Epic «Combat Preview / Projectile / AoE Presentation» | **REJECT** | confermato: `#286` e `#25` coprono il dominio; `D-153` vieta l'epic per residuo |
| Issue duplicate per preview linea/AoE | **REJECT** | `#172` le possiede alla lettera (`TargetCells`, `AffectedCells`) |
| Secondo timer di presentazione | **REJECT** | `AttacksToShow` + `AttackShowSeconds`, tre test verdi |
| **Una** issue residuale E21 «il colpo si vede durante Blast», con lo scope candidato | ⛔ **DEFER** | lo scope Area/Line/Cone non è realizzabile: §3. Aprirla oggi significa consegnare una DoD che si può chiudere solo violandola |
| Owner del contratto evento → presentazione | **REUSE** — `#1801` | esiste, OPEN, P1, discende da `D-278` |

### Mutazione **M1** — `CREATE`, una sola issue: il payload

**Titolo proposto**: *«L'evento `Attack` non dice dove ha colpito: `FRTResolvedEvent` porta le celle risolte
e la forma, o la presentazione dell'area è impossibile senza ricalcolarla»*

- **Epic**: `#25` (E11) o vicino a `#1801` — **non** E21: è C++ di confine, non lavoro in editor
- **Why misurato**: `RTResolvedEvent.h:65-70` — `Path` è *«per Move»*; `RTTurnManager.cpp:4867` emette per
  vittima, e un'AoE su celle vuote non emette nulla
- **Dipendenza dichiarata**: `D-278` (il confine non si sposta) · **interagisce con** `#1801` (se la forma
  richiede nuovi `ERTResolvedEventType`, il gate deve esistere prima) · `#789` (secondo consumatore GAS)
- **Out of scope**: la cue, gli asset, Niagara, ogni resa a schermo
- **Determinism impact**: il TurnLog e lo `StateHash` restano invariati — è un campo di playback, non di stato
- **Test**: un evento `Attack` di forma `Area` porta le stesse celle che `HexHitCells` produrrebbe, **senza
  che il consumatore la chiami**

### Mutazione **M2** — `UPDATE` `#1801`

Aggiungere: *(a)* il legame verso M1 come **primo consumatore reale** del contratto; *(b)* la nota che il
payload dell'evento `Attack` è oggi insufficiente per la presentazione dell'area; *(c)* valutare la
**milestone v0.1**, oggi assente, se M1 è lavoro di release.

### Mutazione **M3** — `UPDATE` `#286` (E21)

Aggiungere sotto *«Lavoro collegato — non un checkpoint»* la nota che la **resa del colpo** (tracer, impatto,
area) è **riconosciuta come gap e deliberatamente differita**, con il motivo — il confine non porta il dato —
e il rinvio a M1. Serve perché senza di essa il prossimo audit ricomincia da capo e riscopre lo stesso gap.

### Mutazione **M4** — `UPDATE` `#289`

Back-reference verso `#1392` parte 3, oggi a senso unico: `#1392` dichiara di atterrare in E21 «vicino a
`PIE-PREVIEW-AREA`», e nessuna issue E21 lo sa.

---

## 9. Ordine v0.1 — proposto contro quello del checkpoint

| # | Checkpoint | Proposto | Perché cambia |
|---|---|---|---|
| 1 | `#172` preview | **`#172` preview** | invariato: è Planning, non tocca il confine |
| 2 | `#288` montage | **`#288` montage** | invariato: consuma i tre `BlueprintImplementableEvent` già chiamati dal `TurnManager` |
| 3 | *nuova issue E21 — tracer* | 🔄 **M1 (payload) → `#1801` (gate)** | la cue non è costruibile prima; e un enum allargato senza il gate di `#1801` è il fallimento che `FX-2` descrive |
| 4 | `#173` warning/reason | **`#173`** | invariato |
| 5 | `#613` Screen HUD | **`#613`** + `#1936` | il feed del colpo è l'altra metà della stessa domanda |

La cue — tracer, impatto, resa dell'area — resta **post-payload**, e va scritta con il ceiling di `D-124`
citato nel body, non lasciato implicito.

---

## 10. Verifiche

### Eseguite
- `git status` / `branch` / `HEAD` / `origin/main` — base `8bfe8f84`
- `gh issue view` su `#25 #172 #173 #286 #288 #289 #613 #911 #1392 #1801` — dieci issue lette, corpo incluso
- `gh issue list --search` su tracer/proiettile/impatto/VFX/AoE/area/cono/linea e su `D-278` / `ResolvedEvent`
- Lettura di `RTResolvedEvent.h`, `RTTurnManager.{h,cpp}` (punti di emissione), `RTPlaybackLibrary.{h,cpp}`,
  `RTHexCombatLibrary.h`, `RTUnit.h`
- `grep` su `AttackShowSeconds`, `HexHitCells`/`AffectedCells`/`TargetCells`, Niagara/VFX in `Content/`
- Decision Log: `D-124`, `D-269`, `D-270`, `D-278`, `D-297` · `OPEN_DECISIONS.md`: `FX-1`, `FX-2`

### ⛔ NOT RUN
- **`./scripts/rt-suite.ps1`** — non eseguita. Questo referto non tocca `Source/`, ma **scrivere in `docs/`
  invaliderebbe comunque una run in corso**: il digest copre l'albero, non i soli sorgenti.
- **PIE / Editor** — nessuna seduta. Ogni claim di leggibilità qui è **citato** da voci già registrate
  (`PIE-PREVIEW-AREA`, `PIE-AS4a/b`), mai osservato in questa sessione.
- **Packaged build** — non pertinente a un referto documentale.
- **Mutazioni GitHub** — §8 **non eseguito**.

---

## 11. Rischi e aperti

- 🔴 **`#1801` è senza milestone.** Se M1 è v0.1, il suo gate non può restare fuori dalla release.
- ⚠️ **Il branch è cambiato sotto la sessione**: l'ambiente si è aperto su `feat/1864-gesto-select-erase` e la
  misura è avvenuta su `main` `8bfe8f84`. Il repository è lavorato in parallelo (`CLAUDE.md` §7): **rifetchare
  prima di eseguire §8**.
- ⚠️ **Nessun `D-nnn` è stato prenotato** da questo referto. Se la scelta *«il payload dell'evento `Attack`
  porta le celle»* va registrata come decisione, l'ID va **cercato e verificato al momento**, non assunto:
  l'ultimo osservato è `D-298`, e prenotarlo non lo protegge da una sessione parallela.
- ⚠️ La forma del payload — campo `TArray<FRTCellId>` riusato, campo nuovo, o nuovi `ERTResolvedEventType` —
  **non è decisa qui**. Tocca `#789` (GAS) come secondo consumatore: è una scelta di contratto, e va fatta
  con i due consumatori sul tavolo.

---

## 12. Prossimo passo

**Confermare o correggere le quattro mutazioni di §8**, a partire da **M1** — è l'unica che sblocca le altre,
ed è la sola `CREATE` che questo audit giustifica.
