# `#2826` — il prompt di targeting, e i tre significati di `None`

> `CURRENT` · **Stato**: slice consegnata in corsia `HUD`, **nessun gate Unreal eseguito** · **Data**: 2026-09-10
> **HEAD di partenza**: `9e881ad6` (`main`), allineato a `origin/main` alla stessa sha
> **Branch**: `issue/2826-prompt-di-targeting`
> **Oggetto**: lo **scope 5** di [`#2826`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2826)
> — *«prompt contestuale di targeting»* — letto contro `Source/`, il contratto puntatore (CP 11.8) e
> [`#2757`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2757)
> **Perimetro**: C++ e test Automation. **Nessun `.uasset` toccato, nessuna build, nessun PIE, nessun MCP.**
> **Corsia**: clone `refactor-tactics-refactor` — **non** è il clone principale che ospita il bridge MCP.
> **Scade quando**: `#2826` chiude, oppure un owner produce `ERTPointerTargetKind::Object`.

Questo referto non è un'autorità: registra **misure** e un difetto di specifica. L'owner resta `#2826`.

---

## 1. Il verdetto in una riga

**La premessa principale di `#2826` è caduta fra la sua misura e oggi, e ciò che resta scoperto dello
scope 5 non è implementabile come la issue lo descrive** — perché `ERTPointerTargetKind` da solo non
distingue i cinque prompt che la sua stessa DoD elenca.

---

## 2. Che cosa la issue dava per mancante e invece c'è

`#2826` è misurata su `4fdb01a0`. Su `9e881ad6`:

| Simbolo | Stato della issue | Stato misurato oggi |
|---|---|---|
| `ARTPlayerController::ArmKitAbility(int32)` | non esiste; `SelectAbilityForCurrent` è `private:` e senza `UFUNCTION` | ✅ **esiste**, `UFUNCTION(BlueprintCallable)`, `RTPlayerController.h:743` — delega a `SelectAbilityForCurrent`, con il toggle |
| lo slot «indica il proprio indice» (scope 2) | da fare | ✅ `FRTAbilityCooldownView::AbilityIndex` esiste già: lo slot **riceve** l'indice che il core ha prodotto, non lo deduce dalla posizione nel `ForEach` |
| test della porta | i quattro nomi della DoD non esistono | ⚠️ **i quattro nomi restano inesistenti** (`grep` → 0 ciascuno), ma `RefactorTactics.PlayerInput.TheDockPortArmsAndDisarms` (`RTActionDockPortTests.cpp`) copre arma · disarma · il tasto non fa toggle · sostituzione · fail-closed |

⛔ **Il corpo di `#2826` va aggiornato**: descrive come mancante una porta che è a `main`. Chi legge la
issue oggi ricostruisce lavoro già fatto — è la stessa deriva che `#2697` ha pagato tre volte.

∴ Dello scope della issue restano scoperti i punti **3** (keybind visibile), **4** (stato senza il solo
colore), **5** (prompt contestuale), **6** (motivo di indisponibilità) e **7** (`RMB` collegato alla UI).
Questa sessione prende il **5**, che è l'unico interamente C++ e quindi l'unico eseguibile in questa corsia.

---

## 3. Il difetto di specifica: `None` significa tre cose

Lo scope 5 dice che il prompt è *«alimentato da `GetPointerTargetKind()`, che già esiste»*, ed elenca
**cinque** frasi: *scegli un'unità · scegli una cella · scegli un bordo · scegli l'orientamento · azione su
di sé, confermata*.

Preso alla lettera non è implementabile, e per due ragioni indipendenti, entrambe misurate:

**(a) `ERTPointerTargetKind::None` è la risposta in tre situazioni opposte.**
`ARTPlayerController::GetPointerTargetKind()` (`RTPlayerController.cpp:2492`) restituisce `None` quando non
c'è unità selezionata, quando `SelectedAbilityIndex == INDEX_NONE`, e quando l'azione armata è
`bSelfTarget`. Solo il terzo caso è *«azione su di sé, confermata»*; gli altri due sono *«non c'è dato»* e
*«non c'è niente di armato»*.

**(b) `Facing` non è un `TargetKind`.** «Scegli l'orientamento» vive in `ERTPointerContext::Facing`, prodotto
da `bDeclaringFacing`. Nessun valore di `ERTPointerTargetKind` lo raggiunge.

### La coppia `(Context, Kind)` li separa senza inventare stato

Misurato su `GetPointerContext()` (`RTPlayerController.cpp`, stesso file):

| situazione | `ERTPointerContext` | `ERTPointerTargetKind` | il prompt che serve |
|---|---|---|---|
| nessuna unità selezionata | `IdleSelection` | `None` | *dato non disponibile* |
| unità selezionata, niente armato — neutro `D-128` | `Planning` / `Pathing` | `None` | *widget applicabile, in attesa di una scelta* |
| azione self-target **armata** | `Targeting` | `None` | *dato disponibile, e il suo valore è «nessun bersaglio»* |
| rotazione finale | `Facing` | — | *scegli l'orientamento* |
| pausa, playback, finestra di reazione | `Modal` · `ResolutionPlayback` · `ReactionWindow` | qualsiasi | *fuori fase: il prompt non si mostra* |

🔑 **Sono esattamente le distinzioni che `#2757` chiede** — *dato non disponibile* · *dato disponibile con
valore `None`* · *widget non applicabile nella fase corrente* — su un elemento concreto del §4.1. Questa
slice serve entrambe le issue, e non crea per `#2757` una seconda disciplina: ne applica una che il view
model già pratica campo per campo (`PlanningSecondsRemaining = -1.f`, `RoundLimit = 0`, `bHasBlocker`).

⚠️ **Entrambi gli ingressi sono già `BlueprintPure` sul controller.** Non nasce nessuno stato nuovo, nessuna
copia, nessun campo da tenere aggiornato: il prompt è una **lettura** di due valori derivati.

---

## 4. Che cosa è stato consegnato

| File | Cambiamento |
|---|---|
| `Source/RefactorTactics/UI/RTHudViewModel.h` | `ERTTargetPromptKind`, `FRTTargetPromptView`, dichiarazione di `BuildTargetPrompt`; un `#include` di `Player/RTPointerInteraction.h` |
| `Source/RefactorTactics/UI/RTHudViewModel.cpp` | implementazione, in un `LOCTEXT_NAMESPACE "RTTargetPrompt"` proprio |
| `Source/RefactorTactics/Tests/RTTargetPromptTests.cpp` | **nuovo**, cinque test |

**Comportamento precedente**: nessun prompt esisteva —
`grep -rniE "TargetPrompt|PromptFor|ComposePrompt|TargetingPrompt" Source/` → **0**. Il dock mostrava le
azioni e, armata un'azione, non diceva che cosa il gioco stesse aspettando.

**Comportamento nuovo**: `URTHudViewModel::BuildTargetPrompt(Context, Kind)` restituisce una vista con lo
stato nominato, la frase da mostrare, e `bIsAwaitingTarget` separato dalla visibilità.

### Le tre scelte che vanno riviste, non subite

1. **Il contesto ha la precedenza sulla forma.** `Modal` / `ResolutionPlayback` / `ReactionWindow` escono
   per primi, perché `GetPointerContext()` li mette già davanti a tutto. Guardare prima il `Kind` mostrerebbe
   *«scegli un'unità»* sopra il menu di pausa, con un'azione rimasta armata da prima. È pinnato da un test
   che passa `Unit` di proposito.
2. **`ERTPointerTargetKind::Object` non ha un prompt inventato.** `TargetKindForAction` non lo produce mai
   (misurato sulle quattro combinazioni del catalogo). Cade su `ERTTargetPromptKind::Unsupported`, che è un
   gap **nominato**: una frase inventata prometterebbe un percorso inesistente, un testo vuoto lo farebbe
   sparire come `NotApplicable`.
3. **`bIsAwaitingTarget` è un campo e non `Kind != NotApplicable`.** `Neutral` e `SelfTargetConfirmed` si
   mostrano e **non** aspettano niente. È un campo per la ragione già scritta in
   `FRTUnitOverlayView::bHasBlocker`: un binding di proprietà UMG legge proprietà, non chiama funzioni.

---

## 5. Test aggiunti — tutti `NOT RUN`

Namespace `RefactorTactics.HudViewModel.*`, dove vivono già le pin di neutralità delle view.

| Test | Che cosa fallirebbe senza |
|---|---|
| `TargetPromptSeparatesTheThreeMeaningsOfNone` | riscrivere il prompt come funzione del solo `Kind` |
| `TargetPromptNamesEveryProducibleTargetKind` | un prompt morto per una forma che il catalogo non produce più; include l'anti-vacuità su `TargetKindForAction` |
| `TargetPromptIsNotApplicableWhileTheWorldIsReadOnly` | invertire la precedenza contesto/forma |
| `TargetPromptDistinguishesVisibleFromAwaiting` | comprimere *visibile* e *in attesa* in un booleano solo |
| `TargetPromptDeclaresTheUnproducibleTargetKind` | far sparire `Object` in un ramo muto |

⚠️ **Nessuno è stato eseguito**: questa corsia non compila e non lancia Automation. Sono scritti, non misurati.

---

## 6. Gate

| Gate | Esito | Motivo |
|---|---|---|
| Compile (Game) | `NOT RUN` | vincolo di corsia: non è la directory Unreal autorevole |
| Compile (Editor) | `NOT RUN` | idem |
| Automation | `NOT RUN` | idem — i cinque test sopra sono **scritti**, non eseguiti |
| Determinismo | `N/A` | nessuna modifica a resolver, snapshot, TurnLog o formati serializzati |
| Replay | `N/A` | idem |
| Privacy | `N/A` sul dato, ⚠️ **da confermare in PIE** sulla resa | `BuildTargetPrompt` non legge intenti, unità, manager né catalogo: solo due enum già derivati. Non introduce nessun canale nuovo |
| PIE | `NOT RUN` | richiede Editor |
| Packaged | `NOT RUN` | non pertinente alla slice |

**Verifiche statiche realmente eseguite** in questa corsia:

- `node tools/radar/catalog-code.ts` → verde. ⛔ **Non copre questa modifica**: confronta i campi degli eroi
  fra cinque fonti. Riportato perché eseguito, non perché pertinente.
- `TestNotEqual` su `enum class`: precedente identico verificato in
  `RTActionMirrorFieldsTests.cpp:381`, che lo usa proprio su `ERTPointerTargetKind`.
- Nessun ciclo di include: `RTPointerInteraction.h` → `Map/RTCellId.h`, `Ability/RTActionData.h` → `RTActionDef.h`. Non risale a `UI/`.
- Nessuna collisione di nomi: i cinque nomi di test e i tre simboli nuovi compaiono solo nei file di questa slice.
- `RefactorTactics.Build.cs` non elenca sorgenti: il nuovo `.cpp` entra nel modulo senza modifiche al build.
- Graffe e `LOCTEXT_NAMESPACE` bilanciati nei tre file.

⛔ **Nessuno di questi è una prova di compilabilità.** Sono controlli testuali: il primo `Build.bat` in MAIN
è l'unico oracolo.

---

## 7. Che cosa deve fare la corsia MAIN

1. `Build.bat RefactorTacticsEditor Win64 Development` con `-WaitMutex`, a Editor chiuso.
2. Automation, filtro `RefactorTactics.HudViewModel` — i cinque test nuovi più le pin esistenti del view model.
3. ⚠️ **Verifica di mutazione sui cinque**, come `#2826` richiede per i propri: togliere il ramo che ciascuno
   presidia e controllare che diventi rosso. Un test che passa anche senza il ramo non misura il ramo.
   Il candidato più a rischio è `TargetPromptSeparatesTheThreeMeaningsOfNone`: si falsifica riscrivendo
   `BuildTargetPrompt` in modo che ignori `Context`.

### Istruzioni Blueprint — `WBP_RT_ActionDock`, da eseguire nel clone principale

⛔ **Authoring asset appartiene al clone che ospita il bridge MCP** (`AGENTS.md` §11): un worktree non ha i
file gitignorati, e salvare un asset i cui riferimenti duri leggono `None` **li azzera** senza errore.

Sul grafo del dock, dove oggi si aggiornano gli slot:

- nodo `Get Pointer Context` (target: `PlayerController`, `BlueprintPure`);
- nodo `Get Pointer Target Kind` (stesso target, `BlueprintPure`);
- nodo `Build Target Prompt` (`RefactorTactics|HUD`), i due pin sopra in ingresso;
- il `TextBlock` del prompt legge `.Text`;
- la sua `Visibility` segue `Kind != NotApplicable` — **non** `Text.IsEmpty()`, che confonderebbe «fuori
  fase» con un dato mancante;
- l'evidenziazione del cursore segue `bIsAwaitingTarget`, **non** la visibilità del prompt.

⚠️ **Non ricablare il grafo rileggendolo con `read_graph_dsl` e riscrivendolo**: `write_graph_dsl` rifiuta i
pin dinamici, che spariscono in silenzio (`AGENTS.md` §9). Aggiungere i nodi e collegarli con `connect_pins`.

### La voce PIE — preparata qui, **non** scritta

`#2826` chiede di scrivere la voce PIE insieme al lavoro. **Non è stata aggiunta a
`docs/technical/test-manuali-pie.md`**, e la scelta è dichiarata:

- quel file porta un totale che la sua stessa disciplina impone di **rimisurare col comando dopo il merge** —
  cosa che questa corsia non può fare, e un totale scritto prima del merge nasce falso;
- la voce della DoD copre il dock **scopribile**, cioè l'intera `#2826`, non lo scope 5 da solo. Scriverla ora
  registrerebbe come da verificare una feature che gli scope 3, 4, 6 e 7 non hanno ancora.

Testo pronto da inserire quando `#2826` si chiude, nella seduta **U43** (che condivide l'allestimento con
`PIE-V01-POINTER`, `#2618`) — ⛔ **non aprire una seduta nuova**:

> **`PIE-V01-DOCKPROMPT`** | Il dock dice che cosa sta aspettando | partita in PIE, unità propria selezionata
> | Armata un'azione, il prompt nomina la forma di bersaglio che l'azione dichiara. Un'azione su di sé dice
> *confermata* e **non** resta in attesa. Senza niente di armato il prompt invita a scegliere un'azione, e
> **non** è la stessa frase di quando non c'è nessuna unità selezionata. A menu di pausa aperto e durante il
> playback il prompt **non compare**, anche con un'azione ancora armata. | ⏳

---

## 8. Rischi, dipendenze, conflitti

| | Nota |
|---|---|
| **Conflitto testuale** | `RTHudViewModel.h/.cpp` sono file caldi. Il diff è **additivo** (un include, un blocco prima della classe, un metodo in coda, un blocco in fondo al `.cpp`): un merge conflict qui è meccanico, non semantico |
| **Perché non un file nuovo** | il posto canonico delle `FRT*View` è questo, e `#2757` lo dichiara come la `Source` ✅ del §4.1. Un file a parte avrebbe creato un secondo posto dove cercare le view |
| **Dipendenza nuova** | `UI/RTHudViewModel.h` → `Player/RTPointerInteraction.h`. Prima nessun file di `UI/` la aveva. Stesso modulo, nessun ciclo, ma è una direzione UI → Player da tenere d'occhio |
| **Contratto altrui** | ⛔ **nessun contratto di targeting, overlay o playback è stato modificato.** `URTPointerLibrary`, `TargetKindForAction`, `ResolveBack`, `GetPointerContext` sono **letti** e non toccati |
| **`ERTPointerContext` cambia** | se un owner aggiunge un contesto (E14 produrrà `ReactionWindow`), cade nel ramo `Neutral`. È il default meno dannoso, ma va rivisto insieme al suo produttore |
| **`#2826` da correggere** | il corpo della issue descrive `ArmKitAbility` come mancante. Va rimisurato prima che qualcuno lo riscriva |

## 9. Follow-up candidates

- **`#2826`**: aggiornare il corpo (§2 di questo referto) e spuntare la casella *«il prompt contestuale nomina
  la forma di bersaglio»* **solo dopo** build, Automation e mutazione in MAIN.
- **`#2757`**: `ERTTargetPromptKind` è un caso concreto della tassonomia che la issue chiede. Non la chiude —
  copre un elemento, non le cinque zone.
- `ERTPointerTargetKind::Object` resta **senza produttore**: ora ha un nome nel prompt e un test che lo
  dichiara. Chi lo produrrà sostituisce `Unsupported` con la frase vera.
