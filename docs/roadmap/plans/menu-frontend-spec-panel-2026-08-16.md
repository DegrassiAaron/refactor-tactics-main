# Frontend, menu e tracking v0.1→v1.0 — spec panel

> `CURRENT` · **Stato**: revisione chiusa, consolidamento applicato · **Data**: 2026-08-16
> **HEAD della revisione**: `4ab36b48` · branch `docs/menu-frontend-consolidamento`
> **Sorgente revisionata**: `Claude_RefactorTactics_Menu_Features_Issues_Tracking_v0.1_to_v1.0.md`
> (1004 righe, untracked in root), archiviata a fine sessione in
> [`../../archive/src/handoff/2026-08-16-menu-frontend-tracking.md`](../../archive/src/handoff/2026-08-16-menu-frontend-tracking.md)
> **Scopo**: classificare ogni affermazione del documento contro il repository **prima** di applicarla a
> roadmap, registry, epic o issue — che è l'ordine che il documento stesso prescrive al §0 e al §30.
> **Regola applicata**: un handoff AI è l'ultima fonte della gerarchia. Dove contraddice un ADR, una
> `D-nnn`, un gate o un fatto misurabile sul branch, prevale il repository e la proposta si **registra**.

---

## 1. Il verdetto in una riga

Il documento **non duplica niente** — ed è la prima volta che un handoff di questa serie non lo fa: il
frontend è genuinamente assente dal repository, misurato in cinque punti indipendenti. Ma sbaglia lo
**scope**: mette in v0.1 `P0` quattro sezioni che il repository ha già classificato *fuori release con
motivazione dichiarata*, e che il documento stesso qualifica DEV/TEST.

> 🔴 **Questa riga ne conteneva un secondo argomento, ed è caduto mentre il referto veniva scritto.**
> Diceva che le §5–§8 poggiano su un catalogo che nel pacchetto non esiste, falsificate da
> [`#926`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/926). La PR
> [`#935`](https://github.com/DegrassiAaron/refactor-tactics-main/pull/935) ha chiuso quella causa il
> 2026-08-16: gli scenari **entrano** nel pacchetto. Dettaglio e conseguenze in §4.6 — la conclusione non
> cambia, ma oggi la regge **un** argomento invece di due.

Ciò che sopravvive è però il pezzo che vale di più, e il documento lo trova senza sapere di averlo trovato:
**il gate G13 della v0.1 è 🟡 da sei giorni per la ragione esatta che un frontend risolverebbe.**

---

## 2. Il conto

| | Voci | Significato |
|---|---:|---|
| `PROPOSED` | **6** | gap reale, nessun conflitto: si costruisce o si registra |
| `CURRENT` | **7** | riporta correttamente una regola del repository, quasi sempre senza sapere che è già regola |
| `CONFLICT` | **5** | contraddice una decisione dichiarata, un naming deciso o un gate |
| `DUPLICATE` | **3** | chiede di costruire qualcosa che ha già codice, owner e test |
| ~~`BLOCKED`~~ | **0** | ce n'era **1**, ed è caduta mentre il referto veniva scritto (§4.6) |

---

## 3. L'assenza è reale — misurata in cinque punti

Il documento vale la revisione perché la sua premessa regge. Verificato su `4ab36b48`:

| Misura | Comando | Esito |
|---|---|---|
| Widget di frontend nel codice | `git ls-files Source/RefactorTactics/UI/` | **9 file**, tutti in-match (`RTHUD`, `RTHudViewModel`, `RTScreenHudWidgets`, `RTIconLibrary`, `RTIconCatalogData`) |
| Asset UMG | `Glob Content/**/*{WBP,Widget,Menu,UI,HUD}*` | **zero** |
| Feature di frontend nel registry | **110** feature, **11** con `area: UI` | tutte in-match tranne una, e quella non è un widget (§4.1) |
| Epic | E1–E45 | **nessuna** frontend; le adiacenti sono E11 (HUD in-match), E21 (presentazione) |
| Release oltre la v0.1 | `roadmap-post-v0.1.md` | zero occorrenze di *menu*, *frontend*, *settings*, *training*, *browser* |

∴ non esiste un secondo sistema da non duplicare. Il vincolo §32 *«non inventare naming se esiste già»*
è soddisfatto per costruzione — con **una** eccezione, che è il §4.3 qui sotto.

> 🔴 **Due righe di questa tabella portavano numeri che nessun comando produce**, corretti in code review.
> «6 file» veniva da un `Glob` **case-sensitive**: `HUD` ≠ `Hud`, quindi mancavano `RTHudViewModel.*`,
> `RTIconLibrary.*` e `RTIconCatalogData.h` — sono **9**, con `git ls-files Source/RefactorTactics/UI/`.
> «90 feature» non discende da nulla: sono **110** (`grep -c "  - feature_id: RT-FEAT"`). L'`11` di
> `area: UI` era giusto. **La conclusione — tutte in-match — regge in entrambi i casi**, ed è il punto:
> una tabella di misure che sostiene una tesi vera con due numeri sbagliati è comunque una tabella
> sbagliata, perché è *quella* che il lettore riusa.

---

## 4. Le voci che il repository falsifica o vincola

### 4.1 `CONFLICT` — «Scenario Browser» è un nome già occupato, da una cosa diversa

Il documento §5 chiede un **widget** che elenchi gli scenari. `RT-FEAT-UI-SCENARIO-BROWSER` esiste già,
è `INTEGRATED`, e non è un widget: è l'indice C++ nato da
[`#209`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/209) — ScenarioId staccato dal
percorso, tag su un solo asse, `ResolvePath` che rifiuta un ID ambiguo, catena di redirect. Owner spec
[`scenario-index-e-tag.md`](../../technical/tooling/scenario-index-e-tag.md), test `ScenarioIndex.*`.

È il caso inverso di quello solito: di norma si scopre che «X non esiste» è vero del nome e falso della
semantica. Qui il **nome esiste e la cosa no**. Chi legge il registry cercando lo Scenario Browser trova
una riga `INTEGRATED` e conclude che è fatto.

### 4.2 `CONFLICT` — tre sezioni sono fuori release *per decisione già presa*

`RT-FEAT-UI-SCENARIO-BROWSER` porta un campo `out_of_release_scope` scritto il 2026-08-08:

> *«Tooling di test nato dall'issue #209: serve a chi sviluppa, non è contenuto della release. Già
> costruito e coperto da test; nessun gate della v0.1 dipende da lui.»*

Lo stesso vale per `RT-FEAT-TOOL-CONTROL-CENTER` (`release: future`, *«collocarlo nella roadmap di
release lo farebbe competere con la consegna»*).

Il documento mette **Scenario Browser, Scenario Detail, Scenario Runner UI e Bot Visual Simulation** in
v0.1 `P0` (§26). Il documento stesso le qualifica come DEV/TEST (§5: *«`SCENARIOS` è una sezione
DEV/TEST»*; §8: *«entry di sviluppo»*) — e poi le mette nel percorso critico di una release che il §1
definisce come vertical slice giocabile. Le due cose non stanno insieme, e il repository ha già deciso
da che parte sta.

### 4.3 `CONFLICT` — il naming UMG collide con quello deciso a CP 11.7

Il documento §15 propone `WBP_FrontendRoot` e `WBP_GameHUDRoot`. Il repository ha già deciso i nomi
dell'HUD in-match — `WBP_RT_TacticalHUD`, `WBP_RT_TurnHeader`, `WBP_RT_TeamRoster`,
`WBP_RT_SelectedUnitPanel`, `WBP_RT_ActionDock`, `WBP_RT_ActionSlot` — nella owner spec §45, ripresi da
CP 11.7 e dal panel [`ui-0-first-playable-hud-2026-08-12.md`](ui-0-first-playable-hud-2026-08-12.md),
che annota testualmente: *«La proposta scriveva `WBP_TacticalHUD` senza prefisso: su un `.uasset` il
rename costa più che scriverlo giusto.»*

Due conseguenze: il prefisso è **`WBP_RT_`**, non `WBP_`; e `WBP_GameHUDRoot` sarebbe un **secondo root**
dell'HUD in-match accanto a `WBP_RT_TacticalHUD`. Il principio del documento — *«Frontend != In-Match
HUD»* — è giusto e va tenuto; il suo lato in-match esiste già e non va ribattezzato.

### 4.4 `CONFLICT` — la scala delle release è compatibile, la sua assegnazione no

Verifica opposta all'attesa: `RELEASE_ORDER` in `scripts/feature_registry.py` ammette **già**
`v0.1…v1.0`, e `roadmap-post-v0.1.md` le descrive tutte in una tabella con tema ed epic — il gate
`check_release_order()` lo verifica nei due versi. La scala a dieci gradini del §18 è dunque
**scrivibile**, il che è più di quanto ci si potesse aspettare.

Ma l'assegnazione diverge in un punto misurabile: il documento mette Online Play, Lobby e Spectator in
**v0.7** (con la riserva *«Se networking è canonico qui»*). Il repository ha `v0.5 — Online Foundation`
(**E40**, `Standard 3v3 online, lobby privata`) e in v0.7 ha `Competitive Alpha` (**E42**, dedicated
server). La riserva del documento è quindi risolta: **no, il networking è v0.5**, e la UI di lobby lo
segue.

### 4.5 `CONFLICT` — §22 apre un terzo vocabolario di tracking

Gli 11 campi che il §22 chiede a ogni issue (`owner code/UI`, `Feature ID`, `release`, `dependencies`,
`scenario/test`, `manual PIE`, `packaged requirement`, `debug/log`, `privacy impact`, `performance
impact`, `accessibility impact`) descrivono in prosa ciò che il repository ha già come **dato**: i 10
gate del feature registry (`spec · data · runtime · log_debug · automation · scenario · ui_wiki ·
packaged · network_privacy · replay_representable`) più il DoD trasversale di
[`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) §1.

`parallel-batch.yaml` respinge esattamente questa mossa, e l'ha già respinta una volta:

> *«la classificazione delle feature — `execution-graph.yaml` ha già `execution_lanes` e `domain_groups`,
> entrambi validati. Un terzo vocabolario sugli stessi oggetti è ciò che il triage del 2026-08-14 ha
> respinto.»*

I due campi che il registry **non** ha sono `accessibility impact` e `performance impact` per la UI.
Quelli sono la parte nuova, e sono l'unica che vale la pena portare.

### 4.6 ~~`BLOCKED`~~ → **caduta**: §5–§8 non sono più bloccate dal packaging

> 🔴 **Questa sezione è stata scritta e falsificata nello spazio di poche ore, ed è conservata invece che
> riscritta.** È la lezione più utile del referto: un argomento misurato su un fatto che qualcun altro sta
> chiudendo *in quel momento* non è meno falso di uno dedotto male.

**Come era scritta.** Il documento chiede due cose che sembravano escludersi: §2 *«Main Menu avviabile in
packaged build»* e §5 *«legge il catalogo/registry reale»*.
[`#926`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/926) misurava su un pacchetto vero
che **`Scenarios/` non è staged** — dunque uno Scenario Browser packaged non avrebbe catalogo da leggere.

**Cosa è successo.** La PR [`#935`](https://github.com/DegrassiAaron/refactor-tactics-main/pull/935) ha
chiuso la **causa 1** il 2026-08-16, mentre questo referto era in scrittura:
`+DirectoriesToAlwaysStageAsUFS=(Path="…")` porta i 76 JSON dentro il pak — `10 654 115 → 10 790 839`
byte, misurati, perché contarli sullo staged dà `0` anche quando ha funzionato (finiscono *dentro* il pak).
Il catalogo nel pacchetto **c'è**.

**Cosa resta di #926: niente.** ⏱️ Questa riga diceva *«la sola causa 2 — in Shipping `-dpcvars` è
compilato fuori»*, ed è invecchiata mentre la si scriveva: **#945** l'ha chiusa dando allo scenario una
porta d'ingresso che funziona anche in Shipping (`Development PASS` / `Shipping PASS`, stesso
`stateHash 572184bb`), e la issue è **CLOSED/completed** dal `2026-08-15T23:46:24Z` — quattro minuti
**prima** che la PR di questo consolidamento venisse aperta.

> 🔴 **Tre stesure, tre stati diversi della stessa issue, e la terza l'ha trovata la code review.** È lo
> stesso difetto che il §4.6 documenta, al secondo giro: un argomento misurato su un fatto che qualcun
> altro sta chiudendo *in quel momento*. La lezione operativa non è misurare meglio — è che una premessa
> presa da una issue **aperta** va riletta con `gh issue view` prima del merge, non prima della scrittura.

∴ **il secondo argomento del §7 cade, e resta il solo §4.2** — che è sufficiente da solo, ed è la ragione
per cui la conclusione non cambia. Ma chi rilegge deve sapere che oggi la sostiene **un** argomento, non due.

---

## 5. Le tre voci che il repository ha già, e il documento riscopre

| § | Afferma | Stato reale |
|---|---|---|
| §7 | `same Scenario + same Seed → same StateHash + same LogHash` | `DUPLICATE`. È il **gate G4** della v0.1, coperto da `Replay.Verifier.ResimulationIsDeterministic` (100 ripetizioni, checksum identico). Non va costruito: va **citato** |
| §10 | Replay minimo: ultimo run, play/pause, next event, focus | `DUPLICATE`. `RT-FEAT-REPLAY-ARCHIVE` esiste, e la UI è già una issue aperta — [`#472`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/472) *«Replay R6: l'interfaccia — guardare una partita registrata, e sapere dove si è»* |
| §8 | *«bot producono intenti tramite API reali; niente shortcut dirette nel resolver»* | `DUPLICATE` come regola: è [D-101](../../decisions/RT_PDR_00_Decision_Log.md) — *«chi decide restituisce decisioni, mai esiti»* — con la sua issue [`#542`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/542) |

---

## 6. Ciò che il documento aggiunge davvero

### 6.1 Il pezzo che vale l'intero handoff — e l'argomento con cui era stato sostenuto, che è falso

> 🔴 **Questa sezione è l'errore più grave del referto, ed è conservata invece che riscritta.** Sosteneva:
> *«Il gate **G13** è 🟡 perché la partita packaged gira sull'arena di test; ciò che manca fra una build
> eseguibile e una giocabile è esattamente il minimo del documento. Questo non è un ampliamento di scope:
> è il completamento di un gate `P0` già dichiarato.»* La code review l'ha falsificata leggendo la
> riserva **intera** invece della frase citata.

**La riserva di `G13` nomina due mancanze, e le qualifica insieme** (`v0.1-definition-of-done.md`):

> *«Restano **due** mancanze, ed **entrambe sono dati, non codice**: la partita gira su
> `MapSource=GeneratedTestArena` — l'arena di test, non un livello di gioco — **e la via a punti non è mai
> stata esercitata, perché la soglia obiettivo è 0**.»*

Il rimedio della prima **esiste già e appartiene a qualcun altro**: `PIE-V01-ARENA`, seduta **U1**, che
`test-manuali-pie.md` dichiara *«l'**ultimo** ostacolo a CP 12.5»*. Nessuna delle due si chiude con un menu.

∴ **costruire E46 non rende verde `G13`.** Citarne mezza riserva per concludere il contrario è il difetto
che questo repository registra da tredici voci del Decision Log: una premessa vera a metà che sostiene una
conclusione comoda. Ed è più insidioso di un numero sbagliato — un numero lo rimisuri, una citazione
troncata *sembra* una prova.

**L'argomento che regge, ed è un'altra cosa**: una build che avvia **direttamente in partita** e non offre
alcun modo di iniziarla, riavviarla o uscirne non è un *vertical slice consegnabile* — è un eseguibile che
carica una mappa. È una decisione di **prodotto**, non l'esecuzione di un gate, e va pesata come tale:
aggiunge la 22ª epic a una release in chiusura e **nessun gate della v0.1 la richiede**.

L'autore ha scelto di tenerla in v0.1 sapendo questo ([D-144](../../decisions/RT_PDR_00_Decision_Log.md)).

### 6.2 Navigation Controller (§16) — buono, con un vincolo

L'idea è corretta e il repository non ha nulla di equivalente: un solo owner del flow (`Push/Pop Screen`,
`Show/Close Modal`, `Return Main`) invece di `CreateWidget`/`RemoveFromParent` sparsi.

⚠️ Va coordinato con **CP 11.8** (Pointer Interaction Contract), che ha già `Modal` fra i sette contesti
del `PlayerController` e una precedenza dichiarata `Modal/Reaction UI > HUD > world tactical hit`. Il
navigation controller del frontend **non** può possedere quel `Modal`: sono due strati.

🔴 **Questa riga diceva «già scritto e testato con dieci test `PlayerInput.*`», ed era falsa** — trovata in
code review. Il contratto è **scritto**; la precedenza **non è testata e non è implementata**:
`grep -rn "HUDConsumesPointerBeforeWorld\|ReactionWindowOwnsInputPriority" Source/` → **zero**, e la nota di
CP 11.8 in `roadmap-v0.1.md` lo dichiara fra i propri delta aperti (*«oggi ogni click passa al mondo»*). I
dieci test che esistono sono quelli di `RTPointerInteractionTests.cpp` e coprono altro. Probabile
trasposizione da *«nove delle sue **dieci regole**»*, che è una frase diversa nello stesso documento.
L'argomento del confine non ne soffre — regge sulla spec, non sui test — ma il numero era inventato.

### 6.3 Le sette regole già canoniche che il documento enuncia correttamente

`CURRENT`, elencate perché il loro valore è confermativo e non va scambiato per lavoro: la UI non
ricalcola il risultato (§9); il replay è presentazione e non decide (§10); la velocità influenza solo la
presentazione (§8); Training riusa lo Scenario System invece di un framework parallelo (§4); mai mostrare
hidden enemy planning (§3); non modificare i file generated a mano (§24); niente matchmaking, ranked,
progression in v0.1 (§19).

---

## 7. Lo scope che questa revisione propone

**In v0.1** — una sola epic, `P1`, per la ragione del §6.1 e per nessun'altra:

```text
E46 · Frontend shell e ciclo di partita
  Main Menu · navigazione e back stack · loading · error modal
  Play → partita esistente → Result → Main Menu · Quit
  Pause (Resume / Return to Main Menu)
```

**Fuori dalla v0.1**, con la motivazione dichiarata invece che dedotta:

| Sezione | Dove va | Perché |
|---|---|---|
| Scenario Browser / Detail / Runner UI | v0.2+, **non più dietro `#926`** | §4.2: tooling dichiarato `out_of_release_scope`. ⚠️ Il secondo motivo (nessun catalogo in packaged) **è caduto** — vedi §4.6 |
| Bot Visual Simulation UI | v0.2+ | idem; è `entry di sviluppo` per ammissione del documento |
| Replay UI | resta su `#472` | esiste già come issue |
| Settings completo, Training Lite | v0.2 | nessun gate della v0.1 dipende da loro |
| Briefing | v0.2 | il §3 lo ammette preconfigurato: in v0.1 non c'è niente da scegliere |

⚠️ **Questa è una decisione di scope, non una misura.** Le misure sono nei §3–§6; la riga che le
trasforma in un piano è una scelta, e va registrata come tale nel Decision Log — non applicata perché
sembra ovvia.

---

## 8. Cosa questa revisione **non** decide

- ~~**Se aprire E46 adesso.**~~ **Deciso il 2026-08-16: sì, in v0.1.** La v0.1 aveva 21 epic e 100
  checkpoint (ora **22** e **106**) e i gate G1–G15 sono quasi tutti ⏳: aggiungere un'epic `P1` a una
  release in chiusura è una scelta dell'autore, non una conseguenza dell'audit — ed è stata presa come
  tale, **dopo** che la code review aveva falsificato l'argomento del gate (§6.1). L'alternativa era
  registrarla post-v0.1; `G13` resta 🟡 in entrambi i casi, perché le sue riserve non dipendono da E46.
- **CommonUI.** Il vincolo §32 dice *«non rendere CommonUI obbligatorio se non già deciso»*. Non è deciso:
  quattro sorgenti archiviati dicono *«CommonUI solo dopo proof of concept»*, e nessuno di essi è
  normativo. Resta fuori, e resta una domanda aperta.
- **Il formato delle issue.** §22 va risolto togliendo i nove campi che il registry già ha e tenendo i
  due che non ha (§4.5) — ma quali due campi entrino nel registry è una modifica di schema, e lo schema
  ha un owner documentale (`feature-registry.md`).
