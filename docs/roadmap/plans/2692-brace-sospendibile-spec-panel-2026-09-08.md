# Spec panel — #2692 «Il Brace non può sospendere», 2026-09-08

> `REFERTO` · **Oggetto**: [#2692](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2692)
> **Modalità**: `critique` · **Focus**: requirements · architecture · testing
> **Misure**: su `docs/2692-voce-pie-brace`, `HEAD` = **`6516a1a2`** (working tree pulito salvo
> `docs/technical/test-manuali-pie.md`).
> **Esito**: **1 criterio di DoD con un numero non riproducibile**, **1 requisito mancante che rende il
> contratto soddisfacibile senza soddisfare [`D-355`](../../decisions/RT_PDR_00_Decision_Log.md)**, 2 costi
> strutturali non dichiarati, 1 decisione ereditata e non chiusa, 2 correzioni al piano di test.
> **Contesto**: la issue scorpora il **secondo** dei due siti che `D-355` nomina; il primo è arrivato con
> [#2679](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2679).

**Le citazioni della issue sono accurate, una per una.** Verificate a `6516a1a2`:

| Affermazione della issue | Misura |
|---|---|
| `Profile.Sidestep` ha `SIDESTEP` con `SelfReposition` | ✅ `Ability/RTCatalogLibrary.cpp:53-54` |
| Phase usa quel profilo | ✅ `Ability/RTHeroCatalogLibrary.cpp:445` |
| `BraceExecutableResponses` filtra per effetti → cardinalità 2 | ✅ `Ability/RTCatalogLibrary.cpp:98-111` |
| Il `Brace` chiama `AskReactionDecision` **diretta**, fuori dal pump | ✅ `Turn/RTTurnManager_Blast.cpp:2304` |
| Esito `NoDecider` → `SafeResponse` | ✅ `Turn/RTTurnManager.cpp:6537-6541` |
| `ApplyDisplacements` = **670** righe (1928→2597), decisione al **56 %** | ✅ misurato |
| `FRTBlastContext` dichiara di **non** sopravvivere alla fase | ✅ `Turn/RTBlastContext.h:50-53` |
| `FRTMovementResolutionState` era già sospendibile, con input copiati | ✅ `Turn/RTHexSim.h:195-201` |
| `Move` è l'ultima fase, ed è la scorciatoia che #2679 ha usato | ✅ `Turn/RTTurnManager.cpp:1854-1860` |

✅ **E il primo criterio di DoD ha già assorbito la lezione di #2679.** Là il panel dovette invertire
*«una finestra sospende ogni unità»* perché era vero per costruzione; qui la issue scrive già
*«falsificato da: **il Blast avanza mentre la finestra è aperta**»*, che è la forma per inversione. Si
registra perché è la correzione più cara del panel precedente, ed è arrivata da sola.

Ciò che segue riguarda **due cose che il DoD non chiede** e senza le quali la issue può chiudersi verde
lasciando il difetto in campo.

---

## 1. C1 🔴 — il bound di #1818 nel DoD non è riproducibile

Il criterio dice: *«Il bound di #1818 resta rispettato — misura di partenza **10.115** su 11.000
(`6516a1a2`)»*.

La formula del bound è fissata dal piano di #2679 (`2679-riprendibilita-resolvemovement-piano-2026-09-08.md`,
Task 5): **`RTTurnManager.cpp` + `RTTurnManager.h`**, soglia **11.000**. Misurata sul commit che la issue
stessa dichiara:

```
git show 6516a1a2:Source/RefactorTactics/Turn/RTTurnManager.cpp | wc -l   →  7 707
git show 6516a1a2:Source/RefactorTactics/Turn/RTTurnManager.h   | wc -l   →  2 549
                                                                    TOT   → 10 256
```

**10.256, non 10.115.** Lo scarto è **141 righe**, e nessuna combinazione plausibile dei file lo spiega:
`.cpp` da solo = 7.707 · `.cpp`+`Blast` = 10.503 · `.cpp`+`.h`+`Movement` = 11.420 · `.h`+`Blast` = 5.345.
Il numero non è ottenibile da nessuna formula, quindi non è stato misurato: è stato riportato.

**KARL WIEGERS**: *«Un criterio di accettazione con una baseline sbagliata non è un criterio debole — è un
criterio che mente in una direzione sola. Chi chiuderà questa issue misurerà, poniamo, 10.640, calcolerà
+525 contro una partenza che non esiste, e non saprà se ha aggiunto 525 righe o 384. Il margine reale è
**744**, non 885: la issue si è regalata 141 righe di spazio che non ha.»*

📝 **Correzione**: sostituire con *«misura di partenza **10.256** su 11.000 (`6516a1a2`), formula
`RTTurnManager.cpp` + `RTTurnManager.h` come da piano #2679 Task 5 — margine **744**»*. La formula va scritta
nel criterio, non lasciata a un piano di un'altra issue: è la sola cosa che rende la rimisura ripetibile.

---

## 2. C2 🔴 — il playback parziale non esiste per il Blast, e senza di esso `D-355` non è soddisfatta

Questo è il rilievo più grave, perché è un **requisito assente**, non un requisito scritto male.

`D-355` non decide «la resolution si sospende». Decide che **la finestra si apre *durante* il playback**, e
la sua motivazione è testuale: senza playback il giocatore deciderebbe *«su un movimento che il suo schermo
non ha mostrato»*. Per il movimento quella metà è stata costruita — `BeginPartialPlayback()`
(`Turn/RTTurnManager_Movement.cpp:753`), il cui commento la chiama *«la metà che mancava a D-355»*.

Applicata al Blast, quella funzione **non fa niente**:

```cpp
void ARTTurnManager::BeginPartialPlayback()
{
    const FRTMovementResolutionContext* Ctx = PendingMovement.Get();
    if (!Ctx || !bEnablePlayback || bIsResolving) { return; }   // ← esce qui, sempre
    ...
    EmitMoveEvents(Units, Ctx->State.Results);                  // ← e comunque emette solo MOVIMENTO
}
```

Con una sospensione nel Blast, `PendingMovement` è nullo: si esce alla prima riga. E se anche non lo fosse,
l'unica cosa che quella funzione emette sono eventi di movimento — non la spinta, non il colpo che l'ha
causata.

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

Una sospensione del Blast che non estenda questo predicato produce **due difetti silenziosi**, entrambi
peggiori di quello che la issue chiude:

| Chiamante | Cosa fa se il predicato mente | Conseguenza |
|---|---|---|
| `LockInAndResolve:1863` | non vede la sospensione, prosegue | `ConcludeResolution()` su un turno **a metà**: TurnLog ordinato e chiuso, Cleanup eseguito, verdetto di fine partita emesso su uno stato incompleto |
| `FinishPlayback:7502` | non alza `bPlaybackHeldByWindow` | il playback si chiude e `bIsResolving` va a falso mentre una finestra è aperta |

**MARTIN FOWLER**: *«Il nome della funzione dice "resolution", il corpo dice "movement". Finché il sito era
uno la differenza non si vedeva; questa issue è il momento in cui diventa un bug. E non è una rinomina: è la
domanda "**che cosa** è sospeso", che oggi il manager non sa porsi.»*

📝 **Criterio da aggiungere**:

> - [ ] `IsResolutionSuspended()` è vero anche quando la sospensione è nel Blast — falsificato da:
>   `ConcludeResolution` viene raggiunta con una finestra del `Brace` aperta

---

## 4. C4 ⚠️ — `FRTBlastContext` non è trasportabile: sei array e nove mappe di `ARTUnit*` grezzi

La issue confronta correttamente le due strutture sul **ciclo di vita**. Manca il confronto sui **tipi**, ed
è la differenza che costa di più.

| | `FRTMovementResolutionContext` | `FRTBlastContext` |
|---|---|---|
| Unità | `TArray<TWeakObjectPtr<ARTUnit>>` | `TArray<ARTUnit*>` × **6** |
| Indicizzazione per unità | per indice | `TMap<ARTUnit*, …>` × **9** |
| Sopravvive a un frame | ✅ per costruzione | ⛔ mai fatto |

`FRTMovementResolutionContext` usa i weak pointer e dichiara perché (`Turn/RTMovementResolutionContext.h:78`).
`FRTBlastContext` usa `ARTUnit*` **come chiave di `TMap`** — `KnockFrom`, `KnockDist`, `KnockCount`,
`PullToward`, `PullDist`, `PullCount`, `PushCause`, `PullCause`, `IndexOf`.

**MICHAEL NYGARD**: *«Un `TWeakObjectPtr` che muore diventa `nullptr` e lo si vede al primo `Get()`. Un raw
pointer usato **come chiave** che muore resta un indirizzo: la `TMap` continua a rispondere, l'hash è
ancora valido, e la lookup restituisce il valore di un'unità che non c'è più. È il difetto che non fallisce
il test — fallisce la partita, tre turni dopo, in un modo che nessuno riconduce qui.»*

⚠️ Il rischio non è teorico neanche nella finestra dei tempi: la finestra del `Brace` dura
`FastReactionDuration` di orologio, e il Blast è la fase in cui le unità **muoiono**. `DestroyDefeatedUnits`
gira in Cleanup, quindi dopo — ma il commento di `RTTurnManager_Blast.cpp:2270-2280` documenta già che
`Ctx.Units` e `MakeCurrentSnapshot` divergono **appena qualcuno è caduto**, e quella divergenza è stata
trovata da una code review, non dalla suite.

📝 **Non è un criterio di DoD, è un vincolo di progetto** da scrivere nella issue: *la parte di
`FRTBlastContext` che sopravvive alla sospensione porta identità stabili (`TWeakObjectPtr` o
`StableUnitId`, [D-063]), mai `ARTUnit*` grezzi — le mappe indicizzate per puntatore si traducono o restano
fuori dal contesto trasportato.*

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

Il caso è costruito su Phase perché è l'unico con cardinalità 2. Ma la cardinalità non viene dal profilo:
viene dal filtro `Effects.Num() > 0` di `BraceExecutableResponses`, e il commento sopra il catalogo dichiara
perché gli altri due profili sono vuoti:

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
(`Ability/RTHeroCatalogLibrary.cpp:723-728`), che è `Action.Charge` — 20 danni + `Push 1` — e il cui impatto
entra nel Blast via `AppendChargeImpactIntents` (`Turn/RTTurnManager.cpp:5146`). Le altre due sorgenti di
`Push` del roster (`Hero.Phase.PressureJet`, la variante `CircularTide.Impact`) appartengono a **Phase
stessa**, quindi servirebbe un mirror match.

📝 **Precondizione da aggiungere alla voce**: *«il nemico è **Branth**, che usa `Ram` (`Action.Charge`,
`Push 1`) su Phase: è l'unica spinta ostile del roster canonico che non richieda Phase su entrambi i lati»*.

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
| 🔴 | **C1** baseline #1818 sbagliata di 141 righe | correggere il numero e scrivere la formula | 1 riga |
| 🔴 | **C2** nessun criterio sul playback parziale del Blast | criterio nuovo + stima da rifare | alto |
| ⚠️ | **C3** `IsResolutionSuspended` cieco al Blast | criterio nuovo | medio |
| ⚠️ | **C4** `FRTBlastContext` porta 15 raw `ARTUnit*` | vincolo di progetto nella issue | medio |
| ⚠️ | **C5** forma della continuazione non decisa | dichiararla, o `BLOCKED — DECISION REQUIRED` | decisione |
| ⚠️ | **C6** «non riesegue» falsificato solo sul danno | riformulare il criterio | 1 riga |
| 🟢 | **C7** esempio ancorato all'eroe invece che alla cardinalità | nota nel piano di test | 1 riga |
| 🟢 | **C8** la voce PIE non nomina chi spinge | precondizione nella voce | 1 riga |

**Punto cieco del panel.** Nessuno di questi rilievi tocca il **countdown**. `D-355` colloca la finestra
dentro il playback e `D-350` ne rallenta l'orologio; nel Blast la sospensione avviene in una fase che oggi
**non ha un playback interlacciato**, e il panel non ha misurato se `FastReactionDuration` scorra
correttamente in quel contesto. È la prima cosa da guardare dopo C2.

**Domanda aperta per l'autore.** C5 è una decisione, non una raccomandazione: (A) rientro nel `do…while`
con un secondo punto d'ingresso in `LockInAndResolve`, o (B) indice di fase nel contesto sospeso con un
`ResumePhaseLoop` proprio. Il panel non la prende — ma segnala che `D-355` l'ha già dichiarata **non decisa**,
e che una issue che la lascia aperta la farà decidere all'implementazione.

---

## Verification

* Compile: `NOT RUN` — nessuna modifica al codice
* Tests: `NOT RUN`
* Determinism · Replay · Privacy · PIE · Packaged: `N/A` — referto di specifica
* Misure del referto: **eseguite** su `6516a1a2` (conteggi righe, citazioni file:riga, catalogo azioni)
