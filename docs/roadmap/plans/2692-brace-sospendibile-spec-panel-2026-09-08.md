# Spec panel — #2692 «Il Brace non può sospendere», 2026-09-08

> `REFERTO` · **Oggetto**: [#2692](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2692)
> **Modalità**: `critique` · **Focus**: requirements · architecture · testing
> **Misure**: su **`6516a1a2`** per il codice; la voce PIE letta a **`02c71f58`**, che è il commit in cui
> `PIE-V01-RXBRACE` esiste (era working tree quando il panel l'ha letta, poi mergiata con #2693).
> **Precedente**: [`2679-finestra-interlacciata-spec-panel-2026-09-08.md`](2679-finestra-interlacciata-spec-panel-2026-09-08.md)
> · **Piano che fissa il bound**: [`2679-riprendibilita-resolvemovement-piano-2026-09-08.md`](2679-riprendibilita-resolvemovement-piano-2026-09-08.md)
> **Esito**: **9 rilievi** — 1 attribuzione di commit stale su un criterio di DoD, 1 requisito mancante che
> rende il contratto soddisfacibile senza soddisfare
> [`D-355`](../../decisions/RT_PDR_00_Decision_Log.md), 1 costo strutturale non dichiarato, 1 decisione
> ereditata e non chiusa, 1 criterio falsificato sulla grandezza sbagliata, 1 cautela di progetto, 3
> correzioni al piano di test.
> 🔴 **Referto CORRETTO dopo code review** ([PR #2694](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2694)):
> C1 diceva che il numero della issue non era misurato — è **falso**, ed è l'errore più grave di questo
> documento; C3 elencava due chiamanti su tre; C4 era contraddetto dal codice ed è stato declassato; **C9 è
> nuovo** e nessuno del panel l'aveva visto. Le correzioni sono **in linea**, non in coda: un referto che
> lascia in piedi la diagnosi sbagliata e la smentisce sotto è peggio di un referto sbagliato.
> 🔴 **E una seconda correzione, dall'autore**: C7 negava che la cardinalità venisse dal profilo. `Brace` è
> una **famiglia** di reazioni, non un'azione con un effetto proprio — le risposte le determinano le skill
> del personaggio, e il filtro sugli effetti le *riduce*.
> **Contesto**: la issue scorpora il **secondo** dei due siti che `D-355` nomina; il primo è arrivato con
> [#2679](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2679).

**Le citazioni della issue sono accurate, una per una.** Verificate a `6516a1a2`:

| Affermazione della issue | Misura |
|---|---|
| `Profile.Sidestep` ha `SIDESTEP` con `SelfReposition` | ✅ `Ability/RTCatalogLibrary.cpp:53-54` |
| Phase usa quel profilo | ✅ `Ability/RTHeroCatalogLibrary.cpp:445` |
| `BraceExecutableResponses` filtra per effetti → cardinalità 2 | ✅ `Ability/RTCatalogLibrary.cpp:98-111` |
| Il `Brace` chiama `AskReactionDecision` **diretta**, fuori dal pump | ✅ `Turn/RTTurnManager_Blast.cpp:2304` |
| Esito `NoDecider` → `SafeResponse` | ✅ `Turn/RTTurnManager.cpp:6534-6535`, con la ragione a `:6531-6533` |
| `ApplyDisplacements` = **670** righe (1928→2597), decisione al **56 %** | ✅ misurato |
| `FRTBlastContext` dichiara di **non** sopravvivere alla fase | ✅ `Turn/RTBlastContext.h:50-53` |
| `FRTMovementResolutionState` era già sospendibile, con input copiati | ✅ `Turn/RTHexSim.h:195-201` |
| `Move` è l'ultima fase, ed è la scorciatoia che #2679 ha usato | ✅ `Turn/RTTurnManager.cpp:1854-1860` |

✅ **E il primo criterio di DoD ha già assorbito la lezione di #2679.** Là il panel dovette invertire
*«una finestra sospende ogni unità»* perché era vero per costruzione; qui la issue scrive già
*«falsificato da: **il Blast avanza mentre la finestra è aperta**»*, che è la forma per inversione. Si
registra perché è la correzione più cara del panel precedente, ed è arrivata da sola.

Ciò che segue riguarda **ciò che il DoD non chiede** e senza cui la issue può chiudersi verde lasciando il
difetto in campo — più una cosa che **nessuno del panel aveva guardato**: le due condizioni che aprono il
ramo del `Brace` (C9).

---

## 1. C1 🔴 — il bound di #1818 è attribuito al commit sbagliato

Il criterio dice: *«Il bound di #1818 resta rispettato — misura di partenza **10.115** su 11.000
(`6516a1a2`)»*.

La formula è fissata dal
[piano di #2679](2679-riprendibilita-resolvemovement-piano-2026-09-08.md), Task 5:
**`RTTurnManager.cpp` + `RTTurnManager.h`**, soglia **11.000**. Misurata sul commit che la issue dichiara:

```
git show 6516a1a2:…/RTTurnManager.cpp | wc -l  →  7 707
git show 6516a1a2:…/RTTurnManager.h   | wc -l  →  2 549
                                          TOT  → 10 256
```

**10.256, non 10.115.** Ma il numero della issue **non è inventato**: è la stessa formula, su un commit
precedente dello stesso branch.

```
git show a7c2c600:…/RTTurnManager.cpp | wc -l  →  7 609
git show a7c2c600:…/RTTurnManager.h   | wc -l  →  2 506
                                          TOT  → 10 115   ← esatto
```

`a7c2c600` (*«refactor(2679): gli helper condivisi escono dal namespace anonimo»*) è antenato di
`6516a1a2`. Fra i due sono atterrati gli ultimi commit del branch, che hanno aggiunto le 141 righe.

🔴 **La prima stesura di questo referto concludeva che il numero «non era stato misurato, era stato
riportato», ed era falsa.** Aveva enumerato quattro *insiemi di file* a **un** commit — `.cpp` da solo,
`.cpp`+`Blast`, `.cpp`+`.h`+`Movement`, `.h`+`Blast` — presentando l'enumerazione come esaustiva senza mai
variare **il commit**. La correzione arriva da una code review, non dal panel.

**KARL WIEGERS**: *«Il difetto resta, ed è più insidioso di quello che il panel credeva di aver trovato: un
numero misurato bene e attribuito male non si smaschera rileggendo, perché è internamente coerente. Chi
chiude la issue rimisurerà su `6516a1a2`, otterrà 10.256 e calcolerà +141 di crescita che nessuno ha
scritto. Un numero inventato lo si scopre; questo no.»*

⚠️ **E la lacuna di processo è un'altra**: non «qualcuno ha riportato invece di misurare», ma **la misura
non è stata rifatta dopo gli ultimi commit del branch**. È la stessa cautela che la nota di `D-355`
prescrive per la numerazione — *«misurare su `origin`, non sul checkout»* — applicata a una grandezza che
cambia a ogni commit invece che a ogni merge.

📝 **Correzione**: *«misura di partenza **10.256** su 11.000 a `6516a1a2`, formula `RTTurnManager.cpp` +
`RTTurnManager.h` (piano #2679 Task 5) — margine **744**»*. La formula **e lo sha** stanno dentro il
criterio: senza entrambi la rimisura non è ripetibile, ed è esattamente il modo in cui questo scarto è
nato.

---

## 2. C2 🔴 — il playback parziale non esiste per il Blast, e senza di esso `D-355` non è soddisfatta

Questo è il rilievo più grave, perché è un **requisito assente**, non un requisito scritto male.

`D-355` non decide «la resolution si sospende». Decide che **la finestra si apre *durante* il playback**, e
la sua motivazione è testuale: senza playback il giocatore deciderebbe *«su un movimento che il suo schermo
non ha mostrato»*. Per il movimento quella metà è stata costruita — `BeginPartialPlayback()`
(`Turn/RTTurnManager_Movement.cpp:758`), il cui commento la chiama *«la metà che mancava a D-355»*.

Applicata al Blast, quella funzione **non fa niente**:

```cpp
void ARTTurnManager::BeginPartialPlayback()
{
    const FRTMovementResolutionContext* Ctx = PendingMovement.Get();
    if (!Ctx || !bEnablePlayback || bIsResolving) { return; }   // ← nullo se la sospensione è nel Blast
    ...
    EmitMoveEvents(Units, Ctx->State.Results);                  // ← e comunque emette solo MOVIMENTO
    if (ResolvedTimeline.Num() > 0) { BeginPlayback(); }        // ← il gancio che il criterio misura
}
```

⚠️ **La guardia non è inerte in generale** — sul percorso del movimento passa, ed è tutta la consegna di
#2679 fetta 3 (`RTTurnManager.cpp:1863-1871`). È nulla **per il Blast**, perché `PendingMovement` non è il
contesto che si è sospeso. E se anche lo fosse, l'unica cosa che questa funzione emette sono eventi di
movimento — non la spinta, non il colpo che l'ha causata.

🔑 L'ultima riga è il motivo per cui il criterio proposto sotto si misura su `ResolvedTimeline`: è lì che
`BeginPlayback` guarda, e una timeline vuota è esattamente lo schermo nero.

**MICHAEL NYGARD**: *«Il difetto che la issue descrive è "la finestra non si apre". Il difetto che
l'implementazione minima produrrebbe è peggiore: la finestra **si apre su uno schermo nero**. Il giocatore
riceve `Hold Ground` / `SIDESTEP` senza aver visto chi lo spinge né da dove. Oggi almeno subisce un default;
domani gli si chiede di scegliere alla cieca, con un countdown che scorre.»*

**GOJKO ADZIC**: *«E nessun criterio di questa DoD lo coglie. Rileggendo i cinque: "sospende e riprende",
"il ciclo delle fasi riprende", "sospende ogni unità", "nessuna finestra in ri-simulazione", "nessuna
regressione". Tutti e cinque restano verdi con lo schermo nero. È il caso da manuale di un contratto
soddisfacibile senza soddisfare la decisione che lo governa.»*

📝 **Criterio da aggiungere alla DoD**:

> - [ ] Quando il `Brace` sospende, il playback mostra **la spinta e il colpo che l'ha causata** prima che la
>   finestra si apra — falsificato da: `ResolvedTimeline` non contiene alcun evento del Blast corrente
>   nell'istante in cui `OnReactionWindowOpened` viene eseguito

⚠️ **E va detto che questo è lavoro vero, non una riga.** `EmitMoveEvents` ha una controparte per il Blast
(`ResolvedTimeline` viene popolata dentro `ResolveCombatPasses`), ma il punto in cui il playback *parte*
oggi è uno solo e assume il contesto di movimento. Chi stima questa issue senza questo criterio la stima
per difetto — che è esattamente ciò che la sezione «Perché costa più del movimento» esiste per evitare.

---

## 3. C3 ⚠️ — `IsResolutionSuspended()` è un predicato *del movimento*, e tre chiamanti ne dipendono

```cpp
bool ARTTurnManager::IsResolutionSuspended() const
{
    const FRTMovementResolutionContext* Ctx = PendingMovement.Get();
    return Ctx != nullptr && Ctx->bActive;
}
```
`Turn/RTTurnManager_Movement.cpp:737`

Una sospensione del Blast che non estenda questo predicato produce **tre difetti silenziosi**, tutti
peggiori di quello che la issue chiude:

| Chiamante | Cosa fa se il predicato mente | Conseguenza |
|---|---|---|
| `LockInAndResolve:1863` | non vede la sospensione, prosegue | `ConcludeResolution()` su un turno **a metà**: TurnLog ordinato e chiuso, Cleanup eseguito, verdetto di fine partita emesso su uno stato incompleto |
| `FinishPlayback:7502` | non alza `bPlaybackHeldByWindow` | il playback si chiude e `bIsResolving` va a falso mentre una finestra è aperta |
| `ResumeSuspendedResolution:642` | `if (!IsResolutionSuspended()) { return; }` | 🔴 **il turno non riprende affatto**: chi chiude la finestra chiama una funzione che esce subito — finestra chiusa, `bPlaybackHeldByWindow` ancora vero, `bIsResolving` mai spento. Un **blocco permanente**, non una conclusione anticipata |

🔴 **Il terzo era stato omesso dalla prima stesura, ed è il peggiore.** La correzione arriva da una code
review. Conta due volte: perché il difetto è di gravità diversa dagli altri due, e perché il criterio che
questo rilievo proponeva — *«`ConcludeResolution` viene raggiunta»* — **è cieco proprio a quel caso**, dato
che in quello scenario `ConcludeResolution` non viene raggiunta mai.

**MARTIN FOWLER**: *«Il nome della funzione dice "resolution", il corpo dice "movement". Finché il sito era
uno la differenza non si vedeva; questa issue è il momento in cui diventa un bug. E non è una rinomina: è la
domanda "**che cosa** è sospeso", che oggi il manager non sa porsi.»*

📝 **Criterio da aggiungere**, riformulato per coprire tutti e tre:

> - [ ] `IsResolutionSuspended()` è vero quando la sospensione è nel **Blast**, e la ripresa arriva a
>   `ConcludeResolution` — falsificato da: `ConcludeResolution` raggiunta **con la finestra ancora aperta**,
>   oppure **mai raggiunta** dopo che la finestra si è chiusa

---

## 4. C4 🟢 — `FRTBlastContext` porta sei array e nove mappe di `ARTUnit*` grezzi

> 🔴 **Declassato da ⚠️ dopo la code review.** La prima stesura sosteneva che una chiave potesse *danglare
> durante la finestra*, ed è **falso**: `RTTurnManager_Blast.cpp:2787` dichiara che *«nessuna unità viene
> DISTRUTTA dentro il Blast — `DestroyDefeatedUnits` gira in `ConcludeTurn`, dopo»*, e una sospensione
> tiene il turno **aperto**, quindi `ConcludeTurn` non può girare mentre la finestra è su schermo. Ciò che
> resta è una cautela di progetto, non un difetto — e la sezione lo dice adesso invece di lasciarlo credere.

La issue confronta correttamente le due strutture sul **ciclo di vita**. Manca il confronto sui **tipi**.

| | `FRTMovementResolutionContext` | `FRTBlastContext` |
|---|---|---|
| Unità | `TArray<TWeakObjectPtr<ARTUnit>>` | `TArray<ARTUnit*>` × **6** |
| Indicizzazione per unità | per indice | `TMap<ARTUnit*, …>` × **9** |
| Sopravvive a un frame | ✅ per costruzione | ⛔ mai fatto |

`FRTMovementResolutionContext` usa i weak pointer e dichiara perché (`Turn/RTMovementResolutionContext.h:78`).
`FRTBlastContext` usa `ARTUnit*` **come chiave di `TMap`** — `KnockFrom`, `KnockDist`, `KnockCount`,
`PullToward`, `PullDist`, `PullCount`, `PushCause`, `PullCause`, `IndexOf`.

**MICHAEL NYGARD**: *«Un `TWeakObjectPtr` che muore diventa `nullptr` e lo si vede al primo `Get()`. Un raw
pointer usato **come chiave** che muore resta un indirizzo: la `TMap` continua a rispondere e la lookup
restituisce il valore di un'unità che non c'è più. Oggi non può succedere, e va detto. Ma il codice stesso
tiene la guardia per un motivo dichiarato — `RTTurnManager_Blast.cpp:2787`: *«resta perché il contesto
tiene puntatori grezzi … se un giorno un pass distruggesse un attore, saltare è meglio che
dereferenziare»* — e quel "se un giorno" è un'ipotesi che regge finché il contesto **muore a fine fase**.
Questa issue è precisamente ciò che gliela toglie.»*

⚠️ **Quindi non è un difetto presente, è un invariante che smette di essere gratuito.** Oggi «nessuno
distrugge dentro il Blast» è vero perché il Blast dura una chiamata; da qui in poi dura quanto un umano ci
mette a scegliere. Il costo di scriverlo giusto ora è una traduzione di indici; il costo di scoprirlo dopo è
la classe di difetto che il commento di 2787 descrive.

📝 **Non è un criterio di DoD, è un vincolo di progetto** da scrivere nella issue: *la parte di
`FRTBlastContext` che sopravvive alla sospensione porta identità stabili (`TWeakObjectPtr` o
`StableUnitId`, [D-063]), mai `ARTUnit*` grezzi — le mappe indicizzate per puntatore si traducono o restano
fuori dal contesto trasportato.* Con la ragione onesta accanto: **non** «altrimenti danglano», ma «altrimenti
la guardia di 2787 diventa l'unica difesa di un invariante che questa issue rende non più ovvio».

---

## 5. C5 ⚠️ — la forma della continuazione è ereditata da `D-355` come **non decisa**, e la issue non la chiude

`D-355` dichiara testualmente, in coda: *«⛔ **Non deciso qui**: … **quale forma prenda la continuazione**»*.
#2679 non ha dovuto deciderlo — `Move` è l'ultima fase, e gli è bastato un `if (IsResolutionSuspended())`
prima della coda. Qui la scelta va fatta, e le due forme non sono equivalenti.

**(A) Rientro nel `do…while` esistente.** `Phase` è già un **membro**, e `NextPhase` lo avanza. Riprendere
significherebbe: completare `ApplyDisplacements`, completare `ResolveCombatPasses`, poi rientrare nel ciclo
con `Phase == Blast` e lasciare che avanzi a `Move`.
⚠️ Ma `LockInAndResolve` ha una guardia in testa — `if (Phase != ERTMatchPhase::Planning || bIsResolving) return;`
— scritta per respingere un secondo lock-in, non per un rientro. Riusare quella funzione richiede un
secondo punto d'ingresso, non una chiamata.

**(B) Indice di fase esplicito nel contesto sospeso**, e un `ResumePhaseLoop` che riparte da lì.

**SAM NEWMAN**: *«C'è un terzo fatto che decide fra le due, e nessuna delle due sezioni della issue lo
nomina: `ResumeSuspendedResolution()` oggi è **cablata sul movimento**. Chiama `AdvanceMovementResolution()`
in un `while`, poi `FinishMovementResolution()`, poi `ConcludeResolution()`. Non c'è un punto in cui possa
chiedersi che cosa stia riprendendo. Qualunque forma si scelga, quella funzione diventa polimorfa rispetto
al tipo di sospensione — ed è la parte che la tabella "Perché costa più del movimento" non conta.»*

📝 **La issue deve dichiarare quale forma prende**, con la sua ragione, oppure aprire un
`BLOCKED — DECISION REQUIRED` come fece #2679. Non è un dettaglio di implementazione: (A) e (B) hanno
superfici di regressione diverse su `LockInAndResolve`, che è la funzione più esposta del manager.

---

## 6. C6 ⚠️ — «non ri-eseguire il Blast» si falsifica su uno **stato**, non solo sul danno

Il criterio dice: *«i danni applicati una volta sola»*. È un buon esempio e non basta.

Fra l'inizio di `ResolveCombatPasses` e il punto di sospensione girano, nell'ordine:
`GatherBlastUnits` · `RefreshTeamKnowledgeForBlast` · `ResolveCleanseActions` · `CollectHealActions` ·
`CollectAttackIntents` · `AppendChargeImpactIntents` · `ApplyInterrupts` · `ResolveInterceptions` ·
`RunBlastReactions` · `LogBlockedIntents` · `ApplyEnvironmentChanges` (`Turn/RTTurnManager.cpp:5141-5181`).

**LISA CRISPIN**: *«Un'implementazione che riprende dal punto giusto ma ricostruisce il contesto
ri-eseguendo la sequenza produrrebbe: `Wet` applicato due volte, `Prone` applicato due volte, la cura
incassata due volte, e `KnockCount` raddoppiato — con i danni **corretti**, perché il piano dei colpi era
già congelato. Il criterio scritto oggi passerebbe. Va falsificato su uno stato che si accumula, non su
un numero che si ricalcola.»*

📝 **Riformulato**: *«la ripresa non riesegue nulla di ciò che ha già applicato — falsificato da: un
`Status` applicato due volte, un `KnockCount` maggiore del numero di spinte, o una voce di TurnLog
duplicata»*.

---

## 7. C7 🟢 — l'esempio è ancorato all'eroe, e l'eroe è un dato di bilanciamento aperto

> 🔴 **Frase corretta dopo la revisione dell'autore.** Diceva *«la cardinalità **non** viene dal profilo:
> viene dal filtro `Effects.Num() > 0`»*, e nega il modello: `Brace` è una **famiglia** di reazioni, non
> un'azione con un effetto proprio, e **cosa** un'unità può fare nella finestra lo decidono le skill di
> reazione che il personaggio porta. Il filtro non produce la cardinalità: la **riduce**.

Il caso è costruito su Phase perché è l'unico con cardinalità 2. La cardinalità **viene dal profilo** — cioè
dalle skill di reazione del personaggio — e `BraceExecutableResponses` la riduce a quella *eseguibile*
scartando le risposte che il catalogo dichiara ma il resolver non sa ancora applicare (`Effects.Num() > 0`).
`Hold Ground` resta in ogni profilo, universale: è **lei** la risposta che tiene la cella, e il suo esito è
il ramo `Status.Braced` del resolver.

| Personaggio | Profilo | Oltre a `Hold Ground` | Eseguibile |
|---|---|---|---|
| Phase | `Profile.Sidestep` | `SIDESTEP` — `SelfReposition 1` | ✅ → cardinalità **2** |
| Gadget | `Profile.Grounding` | `GROUND` | ⛔ nessun effetto → cardinalità **1** |
| Wraith | `Profile.Glance` | `GLANCE LEFT` / `GLANCE RIGHT` | ⛔ nessun effetto → cardinalità **1** |
| Branth | — | — | cardinalità **1**, per costruzione |

Le due assenze non sono lacune: il commento sopra il catalogo dichiara perché quelle risposte non contano
ancora:

> *«`spec-reaction-clash-e14.md` §2.5 e [D-132] dichiarano **aperti** "Charge del `Grounding`" e "ampiezza
> della deviazione": sono bilanciamento … Finché restano aperti, `BraceExecutableResponses` non offre quelle
> due risposte.»*

**GOJKO ADZIC**: *«Il giorno in cui D-132 chiude, `Grounding` e `Glance` acquistano effetti e la finestra si
apre per Gadget e Wraith — tre eroi su quattro. Un test scritto su "Phase" resta verde e smette di essere
il test di questa regola. Si scrive sull'**invariante** — `RequiresDecisionBoundary() == true` — e Phase è
il dato che oggi lo realizza.»*

📝 Nel piano di test: il caso AUTOMATION arma il profilo per cardinalità, e nomina Phase come la
configurazione **oggi** disponibile.

---

## 8. C8 🟢 — la voce `PIE-V01-RXBRACE` non dice chi spinge

La voce aggiunta a `docs/technical/test-manuali-pie.md` è accurata su tutto il resto — il vincolo su Phase,
la ragione per cui gli altri profili non sono eseguibili, la distinzione da `PIE-V01-RXPLAYBACK`. Chiede
però *«un nemico che la **spinga** durante il Blast»* senza nominarlo, e chi esegue la seduta deve
ricostruirlo dal catalogo.

Misurato: nel roster canonico l'unica spinta **ostile** disponibile è **`Hero.Branth.Ram`**
(`Ability/RTHeroCatalogLibrary.cpp:720-729` — i numeri nel commento a `:720`, l'identificatore a `:728`),
che è `Action.Charge` — 20 danni + `Push 1` — e il cui impatto entra nel Blast via
`AppendChargeImpactIntents` (`Turn/RTTurnManager.cpp:5146`). Le altre due sorgenti di `Push` del roster
(`Hero.Phase.PressureJet`, la variante `CircularTide.Impact`) appartengono a **Phase stessa**, quindi
servirebbero due Phase.

⚠️ **E la formazione di default lo consente, ma il trigger lo produce un bot.** `RTGameMode.h:86-89` dà
`Team0Heroes = {Gadget, Phase}` al giocatore e `Team1Heroes = {Branth, Wraith}` al bot: Branth è dal lato
giusto, ma **`Ram` lo pianifica il bot**, e deve capitare su Phase nel turno in cui Phase è in `Brace`. La
voce sorella `PIE-V01-RXPLAYBACK` dichiara il pericolo speculare — *«serve un decisore umano: col bot il
ramo interattivo non si prende»* — e questa ha bisogno del contrario: che il bot **produca** l'evento. Senza
dirlo, chi esegue ritenta un numero indefinito di turni e la voce resta ⏳ per un motivo di **setup**, non di
comportamento.

📝 **Precondizioni da aggiungere alla voce**: *«il nemico è **Branth**, che usa `Ram` (`Action.Charge`,
`Push 1`) su Phase: è l'unica spinta ostile del roster canonico che non richieda due Phase. È pianificata
dal **bot**, quindi il caso va atteso o forzato dallo Scenario Harness»*.

---

## 9. C9 🔴 — le due precondizioni che aprono davvero il ramo non sono scritte da nessuna parte

> 🔴 **Rilievo NUOVO, trovato dalla code review e non dal panel.** Nessuna delle otto sezioni sopra guardava
> il gate del ramo `Brace`: tutte davano per buono che «Phase spinta» bastasse.

Il ramo che contiene la chiamata a `AskReactionDecision` è protetto da una condizione che né la issue né la
voce PIE nominano:

```cpp
if (T->HasStatus(TAG_Status_Braced) && !T->HasStatus(TAG_Status_Unbalanced))
```
`Turn/RTTurnManager_Blast.cpp:2248`

`Status.Braced` lo concede **solo** `Action.Brace` (`Ability/RTCatalogLibrary.cpp:1465-1467`, fase
`Preparation`, 1 turno). Quindi **Phase deve aver pianificato `Action.Brace` in quel turno**: senza, il ramo
non si apre, nessuna finestra è dovuta, e non c'è alcun difetto da osservare.

⛔ **E c'è una seconda condizione, che sopprime il caso silenziosamente.** Il ramo `Guarded`
(`Turn/RTTurnManager_Blast.cpp:2208-2209`) precede quello del `Brace` e fa `continue`:

```cpp
if (T->HasStatus(TAG_Status_Guarded) && !T->HasStatus(TAG_Status_Unbalanced)
    && KnockDist[T] <= URTCombatLibrary::GuardResistedPushDistance)
```

`GuardResistedPushDistance = 1` (`Combat/RTCombatLibrary.h:142`) e `Ram` spinge di **1**: se Phase ha
`Action.Guard` invece di — o insieme a — `Action.Brace`, l'unità resiste alla spinta e l'esecuzione **esce
prima** di arrivare al `Brace`.

**LISA CRISPIN**: *«Un operatore che segue la voce alla lettera — 2v2 live, Phase del giocatore, Branth che
carica — non vede nessuna finestra, e registra un ⛔ contro #2692 per una ragione che con #2692 non c'entra.
È il modo peggiore in cui un test manuale può fallire: produce una misura, e la misura è sbagliata nella
direzione che conferma l'aspettativa.»*

📝 **Va scritto in due posti**: nella voce PIE come precondizione (`Action.Brace` su Phase, **niente**
`Action.Guard`), e nella issue come parte dell'armamento del caso AUTOMATION.

---

## 🧩 Sintesi

**Convergenza del panel.** La issue è **ben istruita sul difetto** e onesta sul costo — la tabella
«Perché costa più del movimento» è il pezzo migliore, e la riga sul ciclo delle fasi identifica correttamente
il costo nascosto che #2679 aveva evitato. Il problema non è ciò che dice: è ciò che la sua DoD **non
chiede**, e che un'implementazione può quindi non consegnare restando verde.

**Tensione produttiva.** Wiegers e Adzic non concordano sul rimedio a C2. Wiegers vuole un criterio in più
nella DoD; Adzic osserva che un criterio sul playback parziale è, di fatto, il **riconoscimento che questa
issue eredita la seconda metà di `D-355` e non solo la prima** — e che se il playback del Blast è lavoro
comparabile alla riprendibilità, la issue è due issue. Il panel non la scorpora: la sospensione senza
playback è inutilizzabile, quindi la coesione regge. Ma la stima va rifatta con C2 dentro.

**Priorità.**

| | Rilievo | Azione | Costo |
|---|---|---|---|
| 🔴 | **C2** nessun criterio sul playback parziale del Blast | criterio nuovo + stima da rifare | alto |
| 🔴 | **C9** `Status.Braced` e la soppressione di `Guard` non sono scritte | precondizioni nella voce PIE e nella issue | 2 righe |
| 🔴 | **C1** baseline #1818 attribuita al commit sbagliato | correggere numero **e sha**, scrivere la formula | 1 riga |
| ⚠️ | **C3** `IsResolutionSuspended` cieco al Blast, ripresa inclusa | criterio nuovo, che copre anche il blocco permanente | medio |
| ⚠️ | **C5** forma della continuazione non decisa | dichiararla, o `BLOCKED — DECISION REQUIRED` | decisione |
| ⚠️ | **C6** «non riesegue» falsificato solo sul danno | riformulare il criterio | 1 riga |
| 🟢 | **C4** `FRTBlastContext` porta 15 raw `ARTUnit*` | vincolo di progetto nella issue | medio |
| 🟢 | **C7** esempio ancorato all'eroe invece che alla cardinalità del profilo | nota nel piano di test | 1 riga |
| 🟢 | **C8** la voce PIE non nomina chi spinge, né che lo fa un bot | precondizione nella voce | 1 riga |

**Punti ciechi del panel.** Due, e il primo è stato chiuso da qualcun altro.

**(1) Il gate del ramo** — C9. Otto sezioni scritte sul `Brace` senza che nessuna leggesse la condizione che
lo apre. La code review l'ha trovata, e con essa il caso in cui `Action.Guard` sopprime tutto. Registrato
qui perché è la lezione più utile del documento: **il panel ha verificato ogni citazione della issue e non
ha verificato le proprie premesse**.

**(2) Il countdown**, ancora aperto. `D-355` colloca la finestra dentro il playback e `D-350` ne rallenta
l'orologio; nel Blast la sospensione avviene in una fase che oggi **non ha un playback interlacciato**, e
nessuno ha misurato se `FastReactionDuration` scorra correttamente in quel contesto. È la prima cosa da
guardare dopo C2.

**Domanda aperta per l'autore.** C5 è una decisione, non una raccomandazione: (A) rientro nel `do…while`
con un secondo punto d'ingresso in `LockInAndResolve`, o (B) indice di fase nel contesto sospeso con un
`ResumePhaseLoop` proprio. Il panel non la prende — ma segnala che `D-355` l'ha già dichiarata **non decisa**,
e che una issue che la lascia aperta la farà decidere all'implementazione.

---

## Verification

I due gate che questo cambiamento può realmente far fallire, eseguiti sul commit che lo porta:

| Gate | Esito | Misura |
|---|---|---|
| `node tools/radar/doc-tables.ts --check` | **PASS** | 2.460 tabelle in 391 documenti, tutte le righe alla larghezza delle sorelle |
| `node tools/radar/doc-links.ts --check` | **PASS** | 5.797 link in 391 documenti, tutti i percorsi risolvono |

* Compile · Tests · Determinism · Replay · Privacy · PIE · Packaged: **`N/A`** — nessuna modifica al codice,
  nessun asset toccato. `N/A` e non `NOT RUN`: non sono gate saltati, sono gate che questo cambiamento non
  può esercitare.
* Misure del referto: **eseguite** su `6516a1a2` per il codice e `02c71f58` per la voce PIE — conteggi
  righe, citazioni `file:riga`, catalogo azioni, formazione di default.
* ⚠️ **Le misure di C1, C3, C4, C8 e C9 sono state rifatte** dopo la code review della
  [PR #2694](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2694): quattro erano sbagliate o
  incomplete, una mancava del tutto.
* ⚠️ **C7 è stato corretto dopo la revisione dell'autore**: la frase che lo motivava negava il ruolo del
  profilo di reazione, cioè il modello stesso del `Brace`.
