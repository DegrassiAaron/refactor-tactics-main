# Vita e status temporanei sopra l'unità — brief di brainstorming

> **Data**: 2026-09-04 · **Modo**: `/sc:brainstorm` · **Stato**: scelte prese (§3.3); tre issue aperte, di cui **una mergiata** (§3.2)
> **Branch di misura**: `fix/1793-2167-posa-e-offset` @ `b063a60f` · **Rimisurato** su `origin/main` = `46a23606` prima di aprire le issue (§1.7)
> **Aggiornato** il 2026-09-04 dopo il merge di [#2255](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2255) (`3d4570c6`): il pezzo 3 è fatto, e §3.3.1 non è più una previsione
> **Decisioni toccate**: [D-278](../../decisions/RT_PDR_00_Decision_Log.md) (evento → presentazione), [D-301](../../decisions/RT_PDR_00_Decision_Log.md) (`AttackFootprint`), [D-304](../../decisions/RT_PDR_00_Decision_Log.md) (GrayKit Playground), [D-124](../../decisions/RT_PDR_00_Decision_Log.md) (VFX fuori perimetro)

## 0. La richiesta, e le due parole che significavano due cose

> *«nella versione graykit dovremmo vedere accanto ai cilindri gli effetti temporanei (visibili finché
> effetto attivo, poi rimossi) e una barra di vita sopra il cilindro. Usiamo cilindri, ma dobbiamo capire
> se vengono chiamate le animazioni giuste, gli effetti e gli stati giusti.»*

Misurando, **«GrayKit»** e **«cilindro»** nella frase indicano due mondi che nel repository non si toccano:

| | `L_GrayKitPlayground` | `L_DevSandbox` / `L_HexArena` / `L_Prototype` |
|---|---|---|
| Attori | **un solo** `BP_Graybox_UnitFacingFixture_C_1` + materiali graybox | `ARTUnit` (mesh eroe con **fallback al cilindro**) |
| `ARTUnit` presenti | **zero** | sì |
| GameMode / HUD | **nessuno** dichiarato nella mappa | `BP_GameMode` → `ARTHUD` |
| Cook | escluso per assenza da `MapsToCook` (`DefaultGame.ini:171`, `:197`) | `L_HexArena` cotta |
| Governato da | [D-304](../../decisions/RT_PDR_00_Decision_Log.md): laboratorio **editor-only** | il gioco |

Misura: name table di `L_GrayKitPlayground.umap` (113 KB), che nomina il fixture, tre `MI_Graybox_*` e
`/Script/RefactorTactics.RTGrayboxUnitFacingFixture` — e nessuna unità, nessun game mode, nessun HUD.

🔴 **Conseguenza operativa**: *«nella versione GrayKit vedere la barra vita sopra il cilindro»* oggi **non è
eseguibile lì**. Il cilindro del Playground non ha vita, non ha status, non ha un simulatore che gliene dia.
La risposta al brainstorm — *«prima il banco, poi il gioco»* — resta valida, ma il **banco** va costruito:
non è la mappa che c'è già.

## 1. Cosa esiste davvero — misurato, non supposto

### 1.1 La barra di vita sopra il cilindro **esiste già**

`ARTHUD::DrawHUD` (`Source/RefactorTactics/UI/RTHUD.cpp:424-640`) proietta la testa di ogni unità viva
(`WorldHeadOffset`) e disegna in canvas screen-space:

| Elemento | Riga | Nota |
|---|---|---|
| nome eroe (colore di squadra, prefisso `!` fuoco amico / `*` bersaglio) | `:552-575` | `ARTUnit::DisplayLabel`, nome canonico D-120 |
| **barra HP** verde→rosso | `:599-600` | `Health / MaxHealth` |
| barra **scudo** ciano | `:602-607` | proporzionale a `MaxHealth` |
| barra **energia** oro/giallo | `:609-616` | oro se ultimate pronta |
| vincolo al viewport | `:582-590` | `ClampOverlayAnchor`, chiude `#729` |
| **filtro di conoscenza** | `:540-547` | `bIsKnownToObserver`: un avversario non visto **non ha barra** |

⚠️ **Questo cambia una delle quattro risposte del brainstorm.** La scelta *«WidgetComponent + icone
esistenti»* è stata data quando la barra risultava assente. Mettere la vita in un `WidgetComponent` **senza
togliere** questo blocco creerebbe **due produttori dello stesso fatto sopra la stessa testa** — ed è la
famiglia di difetti che `#1500` («La presentazione non ha un confine unico su `ARTUnit`») ha già misurato:
cinque canali che hanno perso lo stesso fatto in cinque modi diversi, con la suite verde.

Il punto (3) del brainstorm — *«filtrate in partita, aperte nel GrayKit»* — è invece **già rispettato** in
partita, dalla stessa riga `:540`.

### 1.2 Gli status a schermo esistono per **2 tag su 11**, e si escludono a vicenda

```cpp
// RTHUD.cpp:619-625
FString StatusStr;
if      (Unit->HasStatus(TAG_Status_Root)) { StatusStr = TEXT("ROOT"); }
else if (Unit->HasStatus(TAG_Status_Slow)) { StatusStr = TEXT("SLOW"); }
```

Un'unità `Root` + `Burning` + `Wet` mostra **`ROOT`** e nient'altro. Nessuna durata. Nessuna icona.

I tag dichiarati sono undici (`RTGameplayTags.h`): `Root · Slow · Reveal · Exposed · Guarded · Marked ·
Wet · Braced · Burning · Obscured · Electrified`.

🔑 **Ma i mostrabili sono dieci, non undici.** `Status.Electrified` è **inerte per decisione**: il suo
doc-comment (`RTGameplayTags.h:44-45`) dichiara *«non entra in `StatusTurns` e l'unità non lo porta
addosso»* — quindi `HasStatus(Electrified)` non risponde vero **mai**, e un widget che lo cercasse
disegnerebbe un'assenza permanente. Il fatto è già stato lavorato: `#1324`, chiusa.

### 1.3 Le undici icone **esistono e sono versionate** — e un commento in `Source/` dice il contrario

`git ls-files Content/RT/UI/Icons/ | grep Status` → **11 file**, `RT_UI_Icon_Status_{Braced, Burning,
Electrified, Exposed, Guarded, Marked, Obscured, Reveal, Root, Slow, Wet}.uasset`.

⛔ **Difetto documentale trovato**: `RTGameplayTags.h:51-53` afferma
*«in `Content/` versionato non esiste **nessuna** icona di stato — `Wet`, `Burning`, `Obscured` contano
tutte zero»*. È **falso oggi**. È il tipo di frase che il prossimo autore legge per decidere se può usare
un'icona, e che gli fa scrivere un ripiego testuale che non serviva. → correzione a costo zero, indipendente
da tutto il resto di questo brief.

### 1.4 Le cue di animazione sono **chiamate** e **non implementate**

`URTPresentationBindingLibrary::DeclaredBindings()` (D-278, `#1801`) dichiara:

| Evento | Cue dichiarate | Chiamate dal C++ | Implementate in `Content/` |
|---|---|---|---|
| `Move` | `bIsMovingVisually`, `SetVisualLocation` | ✅ `RTTurnManager.cpp:6208` | n/d (le consuma `URTUnitAnimInstance`, C++) |
| `Attack` | `PlayAttackMontage`, `PlayHitMontage` | ✅ `:6257-6258` | 🔴 **zero** |
| `Defeated` | `HideForDefeat`, `PlayDefeatMontage` | ✅ `:6293-6300` | 🔴 **zero** |
| `HazardDamage` | `NoPresentation` (dichiarato) | — | — · ⚠️ **e non ha produttore**: nessuno emette l'evento |
| `AttackFootprint` | `NoPresentation` (temporaneo, attende E21) | — | — |

Misura: `PlayAttackMontage|PlayHitMontage|PlayDefeatMontage|HideForDefeat` su tutto `Content/` → **zero
occorrenze**; la name table di `BP_Unit_Riktor` non ne contiene nessuna.

🔴 **È la risposta diretta alla domanda «vengono chiamate le animazioni giuste?».** Sì, il C++ le chiama.
Sono `BlueprintImplementableEvent`: se nessun Blueprint le implementa **non succede nulla**, la logica resta
invariata (invariante #1) e il gate resta **verde** — perché D-278 misura la **tabella**, non
l'implementazione. Il difetto che quel gate esiste per impedire sopravvive **un passo più a valle**.

### 1.5 Il canale che porterebbe gli status alla presentazione **non esiste**

`ERTResolvedEventType` ha cinque valori — `Move · Attack · HazardDamage · Defeated · AttackFootprint` — e
**nessuno per gli status**. `FRTResolvedEvent` non porta un `FGameplayTag`.

Due fatti che rendono l'aggiunta a basso rischio:

1. **`ResolvedTimeline` non entra in hash né in replay**: è un membro del `TurnManager`, e `RTTurnLog.h:546`
   dichiara già la timeline di playback distinta dallo stato. Un valore **in coda** all'enum non tocca il
   determinismo. (In coda e non in mezzo: è un `uint8` esposto a Blueprint — il vincolo è già scritto in
   `RTResolvedEvent.h:28-29` e in D-301 punto (5).)
2. ~~**I tre punti dove uno status nasce e muore sono già isolati e già loggati**: applicazione, revoca
   cell-bound, scadenza a turni.~~ 🔴 **FALSO, corretto il 2026-09-04 da
   [#2245](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2245) — e la correzione ha
   semplificato il lavoro invece di allargarlo.** Le cause sono **nove**, non tre, e stanno su **due** file:
   quattro nascite (`AppliedByAction`, `AppliedByTerrain`, `AppliedWhileOnCell`, `AppliedInstantly`) e
   **cinque** morti (`Revoked` — ha lasciato la cella; `Expired` — è finito il tempo; `Extinguished` —
   l'acqua; `Cleansed` — un'azione, emesso da `RTTurnManager_Blast.cpp`; `Spent` — `Marked` incassato,
   `#1314`). ✅ **Ma non serve toccarne nove**: tutte passano da `AppendLogEntry`, dove sta *«l'UNICO
   `TurnLog.Add` del file»*, e la voce porta già tag, durata e causa. Emettere lì è esaustivo **per
   costruzione**, e i due canali non possono divergere perché il secondo deriva dal primo.

⚠️ Le due durate non sono la stessa cosa: `StatusTurns` conta turni; `CellBoundStatuses` vale
`PersistentWhileOnCell = -1` e finisce quando l'unità lascia la cella. *«Visibile finché l'effetto è
attivo»* significa **due sorgenti di fine**, non una.

### 1.6 Supporto di disegno

`WidgetComponent` nel progetto: **zero occorrenze**. `UMG`, `Slate`, `SlateCore` sono già dipendenze di
`RefactorTactics.Build.cs:27-29`. Il primo `WidgetComponent` sarebbe il primo del progetto.

### 1.7 Rimisurato su `origin/main` prima di aprire le issue

Il branch di lavoro era indietro di 121 commit, e `origin/main` è avanzato lo stesso giorno
(`46a23606`, PR #2241). Tutte le misure di §1 sono state **ripetute** su `origin/main`. Reggono: il ripiego
`ROOT`/`SLOW` (`RTHUD.cpp:620-622`), la barra HP (`:600`), `SetKnownToObserver` (`:521`), il commento falso
(`RTGameplayTags.h:52`), le 11 icone versionate, le quattro cue montage **ancora senza un solo asset che le
implementi**, zero `WidgetComponent`, e `L_GrayKitPlayground` con il solo fixture.

**Due scostamenti**, entrambi a favore del piano:

1. 🔑 **L'enum ha SEI valori, non cinque**: `ReactionResolved` è entrato il 2026-09-03 con `#2191`, in coda,
   con voce `NoPresentation` temporanea. Le voci `NoPresentation` sono ora **tre su sei**
   (`HazardDamage`, `AttackFootprint`, `ReactionResolved`).
   ✅ **È il precedente di forma esatto del pezzo 2**, arrivato tre giorni prima e già chiuso: stessa
   verifica preliminare (la timeline non è formato versionato, `RTTurnLog.h:628-629`), stesso vincolo «in
   coda», stessa emissione dove il fatto accade, stessa validazione **per mutazione**, stesso contatore
   `…CountForTest`. Il pezzo 2 non inventa una forma: ne ricalca una appena collaudata.
2. **Il commento falso di §1.3 è databile, ed era falso quando è stato scritto**: le icone entrano con
   `7112056f` (**2026-08-28**), la frase che le nega con `2c43dbfc` (**2026-09-03**), e
   `git merge-base --is-ancestor` conferma l'ordine. Sei giorni di scarto.

## 2. Il difetto centrale, in una frase

> Il simulatore **conosce** dieci status con la loro durata e **li scrive nel TurnLog**; la presentazione
> ne mostra **due**, come testo, uno alla volta, senza durata — e undici icone già disegnate e versionate
> non le guarda nessuno.

E il suo gemello, un livello più su:

> Il contratto evento → cue è **dichiarato e sorvegliato**; l'**implementazione** delle cue dichiarate non
> è sorvegliata da niente, e oggi è vuota per quattro cue su sei.

## 3. Cosa il brief propone (e cosa non decide)

### 3.1 Il banco, che non c'è

`L_GrayKitPlayground` non può mostrare vita e status perché non contiene unità. **Scelto uno scenario su
`L_DevSandbox`** (§3.3): unità vere, status accesi via API canoniche, zero `.uasset` toccati, e riusa lo
Scenario Harness invece di allestire a mano.

⚠️ **Conseguenza sul nome**: il banco **non sarà «il GrayKit»**. Il Gray Kit Playground resta il laboratorio
di D-304 per il linguaggio visuale del graybox; la validazione di vita e status vive dove vivono le unità.
Se un giorno il Playground dovrà mostrarli, consumerà una fixture che applica status via API canoniche —
che è ciò che `GrayKitYard` già fa per il dato — e non un simulatore proprio.

### 3.2 I quattro pezzi, in ordine di dipendenza

| # | Pezzo | Issue | Dipende da | Costo | Nota |
|---|---|---|---|---|---|
| 1 | correggere `RTGameplayTags.h:51-53` (le icone esistono) | **#2244** | — | minuti | indipendente, va fatto comunque |
| 2 | `ERTResolvedEventType::StatusApplied` / `StatusEnded` **in coda**, emessi dai tre punti di §1.5 | **#2245** | — | piccolo | non tocca hash né replay; il gate D-278 **diventa rosso** finché non si dichiara la voce → è il comportamento voluto. Ricalca `#2191` |
| 3 | **estrarre il driver del velo** da `DrawHUD`: `FRTKnowledgeView` + `SetKnownToObserver` + contact ghost fuori da una funzione di disegno | **#2246** ✅ | — | medio | ✅ **fatto** — PR [#2255](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2255), merge `3d4570c6`. Era il prerequisito della migrazione. Fetta di `#2184` |
| 4 | funzione **pura** che risponde «quali status mostrare per questa unità, in che ordine, con quale durata residua» | — | 2 | piccolo | dove i test arrivano: il widget riceve una lista già decisa, non interroga `HasStatus` dieci volte |
| 5 | `WBP_RT_UnitOverlay` su `UWidgetComponent`: nome + HP + scudo + energia + **dieci** icone con durata; e **rimozione** del disegno da `RTHUD.cpp:553-649` nello stesso pass | — | 3, 4 | alto, **in Editor** | primo `WidgetComponent` del progetto. Riprodurre o dichiarare superati il clamp `#729` e lo scarto «dietro la camera» |
| 6 | lo **scenario banco** su `L_DevSandbox` che accende i dieci status | — | 5 | piccolo | è ciò che rende la validazione ripetibile invece che a occhio |
| 7 | implementare le quattro cue montage vuote (`PlayAttackMontage`, `PlayHitMontage`, `PlayDefeatMontage`, `HideForDefeat`) | — | — | medio, **in Editor** | è `#288` / `E21`, non lavoro nuovo di questo brief |

I pezzi **1** e **2** restano aperti e scrivibili subito: nessuno dei due tocca un `.uasset` né richiede
una sessione Editor. Il **3** è **mergiato**, quindi i pezzi **4-7** sono ora apribili su un perimetro
misurato invece che previsto — che è la ragione per cui non erano stati aperti insieme agli altri.

### 3.2.1 Cosa il pezzo 3 ha trovato, e che questo brief non sapeva

🔴 **Il perimetro reale era più grande di quello previsto, e in un modo che contava.** §3.3.1 diceva che il
ciclo fa tre mestieri; l'implementazione ha aggiunto il fatto che rende il difetto **silenzioso**:
`bKnownToObserver` nasce `true` e `DrawHUD` comincia con `if (!Canvas) { return; }`, quindi *«il driver non
ha girato»* e *«tutto è noto»* producevano **lo stesso schermo**. `SetKnownToObserver` aveva **un solo
chiamante di produzione** in tutto il repository: quella riga.

Ne è nato un test che il brief non aveva previsto — `RefactorTactics.Veil.DriverRunsOnTick`, che pinna che
il driver **venga eseguito** e non solo che decida bene.

⚠️ **E un comportamento nuovo, dichiarato invece che chiuso**: decisione e disegno non sono più lo stesso
ciclo, quindi un'unità che nascesse fra i due verrebbe disegnata una volta col proprio default. In v0.1 non
accade — `EnsureMatchRoster` congela il roster al bootstrap — ed è scritto nel codice dove qualcuno lo
cercherà.

### 3.3 Le due scelte d'autore — prese il 2026-09-04

| Domanda | Scelta | Alternativa scartata |
|---|---|---|
| Supporto del disegno | **(B) migrare l'intero blocco a `WidgetComponent`** — nome + HP + scudo + energia + status insieme, togliendoli dal canvas nello stesso pass. Direzione di `#613` (Screen HUD in UMG), alleggerisce `#2184` (`DrawHUD` è 706 righe, terza funzione più costosa) | (A) restare in canvas e aggiungere le icone a `RTHUD.cpp:619` |
| Dove sta il banco | **scenario su `L_DevSandbox`** che accende i dieci status su unità vere — zero `.uasset` toccati, riusa lo Scenario Harness | portare `ARTUnit` in `L_GrayKitPlayground` (tocca un `.umap` con l'Editor aperto, e rischia di dare al Playground un simulatore che D-304 gli nega) |

⛔ Resta escluso: status in widget e vita in canvas. Sarebbe un sesto canale per `#1500`.

### 3.3.1 🔴 Il perimetro della migrazione NON è «rimuovi `RTHUD.cpp:424-640`»

Misurato dopo la scelta, ed è la cosa che decide il costo. Il ciclo `for (AActor* Actor : Actors)` di
`DrawHUD` fa **tre mestieri**, e solo il terzo è disegno:

| # | Cosa fa | Righe | Si sposta? |
|---|---|---|---|
| 1 | **applica il velo al modello 3D**: `Unit->SetKnownToObserver(bIsKnownToObserver)` → `RefreshComponentVisibility()` | `:520` | ⛔ **no** — se il ciclo sparisce, **le unità nemiche restano visibili**. È un leak di privacy (CLAUDE.md §4), non un difetto estetico |
| 2 | **pilota la sagoma dell'ultimo contatto**: `UpdateContactGhost` / `HideContactGhost` (CP 13.5) | `:525-540` | ⛔ no — ed è deliberatamente **prima** del filtro, perché spegnerla vale anche per chi il filtro salta |
| 3 | **disegna la sovrapposizione**: nome, HP, scudo, energia, status | `:552-648` | ✅ **sì — è solo questo** |

✅ **La parte facile è già fatta**: `ShouldDrawUnitOverlay` e `ContactGhostTargetForUnit` sono **statiche e
pure**, dichiarate in `RTHUD.h` e testate senza montare un HUD. Il widget le **consuma**; nessuna regola di
conoscenza va riscritta né duplicata.

⚠️ **Ma il driver deve sopravvivere alla migrazione**, e oggi vive dentro una funzione di disegno: la
costruzione di `FRTKnowledgeView` una volta per frame (`:494-540`), il `SetKnownToObserver` per ogni unità e
il contact ghost. Non è presentazione della sovrapposizione — è applicazione del velo. Il pass corretto lo
**estrae**, non lo cancella: e finché resta dentro `DrawHUD`, un `WidgetComponent` che disegna da solo
lascerebbe il velo appeso a un disegno che non fa più niente.

⚠️ **Copertura**: `RTHUD.cpp:665` dichiara che *«`DrawHUD` non ha copertura headless, quindi ciò che si può
sbagliare deve stare dove i test arrivano»*. Un `WidgetComponent` in Blueprint ha copertura headless **zero**:
la stessa disciplina vale, e ogni giudizio (quali status, in che ordine, con quale durata residua) deve
restare in C++ puro e testabile — il widget riceve una lista già decisa.

⚠️ **Due cose che il canvas dava gratis e il world-space no**: `ClampOverlayAnchor` (il vincolo al viewport
che ha chiuso `#729`, con sei casi già sotto test in `RTHudOverlayClampTests.cpp`) e lo scarto «dietro la
camera» (`Screen.Z <= 0`). Un widget world-space non ne eredita nessuna: sono da riprodurre o da dichiarare
non più necessarie — non da dimenticare.

### 3.4 Cosa questo brief **non** decide

- Niente VFX/Niagara: D-124 li tiene fuori dalla v0.1 e `Content/` non ha un solo asset Niagara.
  *«Effetti temporanei»* qui significa **icone di stato con durata**, non particellari.
- Niente framework degli status (categoria, polarità, stacking): è `E36` / `#435` / `#437`, **v0.2**.
- Niente `HazardDamage` con produttore: la voce `NoPresentation` va rivista quando ne acquisterà uno, ed è
  scritto nella tabella stessa.
- Nessun numero di bilanciamento, nessuna regola di simulazione.

## 4. Verifiche eseguite / NOT RUN

**Eseguite**: lettura di `RTHUD.cpp`, `RTPresentationBinding.{h,cpp}`, `RTResolvedEvent.h`,
`RTGameplayTags.h`, `RTUnit.h`, dei tre punti status in `RTTurnManager.cpp`; name table di
`L_GrayKitPlayground.umap`; `git ls-files` sulle icone; grep delle quattro cue su `Content/`; corpi delle
848 issue del repository (nessuna copre la barra vita world-space né le icone di stato sopra l'unità).

**Rimisurato** il 2026-09-04 su `origin/main` = `46a23606` prima di aprire le issue (§1.7): due scostamenti,
entrambi registrati. Verificato che l'unica PR aperta (`#1928`) non tocca questo perimetro.

**NOT RUN al momento della stesura**: nessuna build, nessuna suite, nessun PIE. Nessun file di `Source/` o
`Content/` modificato dal pass di brainstorming. `origin/main` era stato letto via fetch e **non** mergiato:
il working tree aveva una sessione Editor aperta e il branch era indietro di 121 commit.

**Eseguite poi, dal pezzo 3** ([#2246](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2246) / PR [#2255](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2255), merge `3d4570c6`): build Editor **Succeeded**;
`rt-suite.ps1` **VALIDA** — `1918/1918 completati, 0 fallimenti`; `misure-strutturali.py --check` exit 0;
`doc-links.ts --check` verde. ⛔ **PIE resta NOT RUN**: la voce è `PIE-KNOW3`, già `⏳ da eseguire` prima di
quel lavoro.

⚠️ **Due misure erano state scartate prima di quella registrabile**, nessuna per un difetto del codice: una
**NON VALIDA** (una run di un altro worktree comparsa a metà, mentre rigenerava il corpus golden — esito
grezzo `1918/1918, 0 fail`, non registrabile) e una **NON AVVIATA** (lock condiviso). È il regime di più
sessioni che `D-222` dichiara, e il motivo per cui `rt-suite` esiste.

⚠️ **Nessun `D-nnn` è stato assegnato né prenotato** da questo pass: le tre issue sono lavoro
d'implementazione e citano decisioni esistenti (`D-278`, `D-301`, `D-304`, `D-124`). La voce che
registrerebbe la scelta (B) è il punto aperto di §5.

## 5. Prossimo passo — uno

✅ **La voce di Decision Log è scritta**: [D-320](../../decisions/RT_PDR_00_Decision_Log.md) registra la
scelta (B) — la sovrapposizione è un `UWidgetComponent` per unità, il disegno in canvas viene **rimosso
nello stesso pass**, e «metà in widget, metà in canvas» è escluso. I pezzi **4** e **5** hanno ora un canone
da citare invece di un referto.

Il passo successivo è il **pezzo 4**: la funzione pura che risponde *«quali stati mostrare per questa unità,
in che ordine, con quale durata residua»*. Viene prima del widget e non dopo — un `UserWidget` in Blueprint
ha copertura headless **zero**, quindi il giudizio deve esistere dove i test arrivano prima che ci sia
qualcosa da disegnare. Dipende da **#2245**, che porta gli stati nella timeline.

⚠️ **Ciò che D-320 non ha deciso, e resta di questo referto**: il default `bKnownToObserver = true`
(fail-open) e la sorte della **barra di stato in alto**, che è l'altra fetta di `#2184` e resta in canvas.
