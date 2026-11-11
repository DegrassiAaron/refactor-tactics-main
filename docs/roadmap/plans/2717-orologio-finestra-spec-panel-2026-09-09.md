# Spec panel — #2717 «La finestra di reazione non scade da sola», 2026-09-09

> `REFERTO` · **Oggetto**: [#2717](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2717)
> **Modalità**: `critique` · **Focus**: requirements · architecture · testing
> **Misure**: su `1695bfed`, motore libero.
> **Precedenti**: [`2692-brace-sospendibile-spec-panel-2026-09-08.md`](2692-brace-sospendibile-spec-panel-2026-09-08.md) ·
> [`2692-forma-della-continuazione-istruttoria-2026-09-09.md`](2692-forma-della-continuazione-istruttoria-2026-09-09.md)
> **Esito**: **5 rilievi**, e **quattro colpiscono la stessa premessa** — che il sito dell'orologio fosse
> un dettaglio di implementazione. Non lo è: è il rilievo.
> ⚠️ **La issue è stata scritta dalla stessa sessione che ora la revisiona.** Su #2692 il panel verificò
> ogni citazione della issue e **nessuna delle proprie premesse**, e la code review dovette correggerne
> quattro. Questo referto parte da lì: le affermazioni sotto esame sono le **proprie**.

---

## Le misure della issue reggono, una per una

| Affermazione | Misura su `1695bfed` |
|---|---|
| `OpenWindowElapsed` azzerato in 4 punti, incrementato in 0 | ✅ `RTTurnManager_Blast.cpp:2363`, `_Movement.cpp:617`, `:675`, `:831` — tutti `= 0.f` |
| `ExpireReactionWindow()` senza chiamanti di produzione | ✅ la sola occorrenza è la definizione a `_Movement.cpp:574` |
| `GetFastReactionDuration()` usata solo dal pacing | ✅ `RTPacingConsole.cpp:41` |
| `OnReactionWindowOpened` senza binding di produzione | ✅ zero fuori da `Tests/` |
| Il `Tick` del manager gira durante la resolution | ✅ `RTTurnManager.cpp:128-135` |

**Il difetto è reale**: senza orologio, la prima finestra aperta in partita ferma il turno per sempre.
Ciò che segue non lo mette in dubbio — mette in dubbio **dove la issue dà per scontato che l'orologio
vada**.

---

## 1. C1 🔴 — il sito naturale è l'unico in cui l'orologio non può girare

La issue chiude la tabella «cosa esiste già» con: *«Il `Tick` del `TurnManager` — ✅ esiste e gira durante
la resolution (`RTTurnManager.cpp:130-133`)»*. Vero, e insufficiente:

```cpp
void ARTTurnManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bIsResolving) { TickPlayback(DeltaSeconds); }
}
```

Il tick arriva. Ma tutto ciò che il manager fa col tempo vive in `TickPlayback`, e **la prima cosa che
`TickPlayback` fa quando una finestra è aperta è uscire**:

```cpp
if (bPlaybackHeldByWindow) { return; }
```
`RTTurnManager.cpp:7359`

🔴 **L'orologio della finestra messo lì non girerebbe mai**, e per costruzione: quel `return` esiste
esattamente per il caso in cui l'orologio serve. Il commento accanto lo dichiara — *«il playback parziale
ha mostrato tutto ciò che era risolto e aspetta la risposta»*.

**MICHAEL NYGARD**: *«È il difetto peggiore di un piano: il posto ovvio è quello sbagliato, e sembra
giusto finché non lo si prova. Chi implementa questa issue seguendo la tabella "cosa esiste già" scrive
tre righe dentro `TickPlayback`, vede il test rosso e non capisce perché — la funzione si chiama tick e
il tick arriva.»*

📝 **La issue deve dire dove**: sopra il `return` di `bPlaybackHeldByWindow`, oppure direttamente in
`Tick` prima di `TickPlayback`. Non è un dettaglio: è la sola parte non ovvia del lavoro.

---

## 2. C2 🔴 — il tick può non essere mai abilitato, e allora la finestra è eterna comunque

`SetActorTickEnabled(true)` sta in **`BeginPlayback`** (`RTTurnManager.cpp:7206`); `SetActorTickEnabled(false)`
in `FinishPlayback` (`:7592`).

E `BeginPartialPlayback` chiama `BeginPlayback()` **solo se la timeline non è vuota**:

```cpp
if (ResolvedTimeline.Num() > 0) { BeginPlayback(); }
```

⛔ **Quindi esiste un percorso in cui una finestra si apre e il tick non è acceso**: `bEnablePlayback`
falso, o una timeline vuota. In quel caso nessun orologio, per quanto ben collocato, riceverebbe un
`DeltaSeconds` — e la finestra resterebbe aperta esattamente come oggi.

⚠️ **Non è teorico anche se oggi è raro**: la fetta 4 di #2692 ha appena reso il playback parziale
possibile per il Blast, e la condizione sul contenuto è **nuova**. Un Blast che si sospende senza aver
emesso nulla — nessun colpo, solo una spinta ambientale — apre una finestra e non accende il tick.

📝 **Criterio da aggiungere**: *l'orologio avanza anche quando il playback non è partito* — falsificato
da: una finestra aperta con `bEnablePlayback` falso che non scade mai.

**MARTIN FOWLER**: *«L'orologio della simulazione non può dipendere dall'avvio della presentazione. È lo
stesso confine che `D-350` e `D-355` hanno già dovuto tracciare due volte, e qui si ripresenta dal lato
del tick.»*

---

## 3. C3 🔴 — il `Dt` di quel sito è scalato, e la scala è proprio la slow-motion della finestra

Sotto il `return` di C1, `TickPlayback` calcola:

```cpp
const float Dt = DeltaSeconds * URTPlaybackLibrary::EffectivePlaybackSpeed(ViewerPlaybackSpeed);
```
`RTTurnManager.cpp:7364`

E [`D-355`](../../decisions/RT_PDR_00_Decision_Log.md) dichiara che `D-350` realizza la slow-motion della
finestra **scrivendo `ViewerPlaybackSpeed < 1`**.

🔴 **Un orologio che leggesse quel `Dt` si allungherebbe insieme al rallentamento**: la finestra durerebbe
`FastReactionDuration / velocità`, cioè più di 3 secondi reali proprio quando la slow-motion è attiva —
che è durante la finestra.

⛔ E contraddirebbe [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) riga 59, che
la issue **cita** nel proprio `Must not`: *«un client lento non allunga la finestra»*. Il `Must not` c'è;
ciò che manca è **da quale orologio** il tempo debba venire, ed è l'unica cosa che lo renderebbe
falsificabile.

📝 **Riformulare il `Must not`**: da *«un client lento non allunga la finestra»* a *«il tempo della
finestra è **non scalato**: non passa da `EffectivePlaybackSpeed`, e la slow-motion di CP 14.6 non la
allunga»* — falsificato da: la finestra dura più di `FastReactionDuration` con `ViewerPlaybackSpeed < 1`.

---

## 4. C4 ⚠️ — un contratto della issue contraddice ADR-0004 §7-bis

La issue scrive:

> **Given** la stessa finestra · **When** il giocatore risponde prima della scadenza · **Then** l'orologio
> si ferma e non produce alcuna scadenza in ritardo

Per la finestra **contested** è **falso**, e l'ADR lo dichiara:

> *«Lo chiude il **reveal a scadenza fissa** — la finestra dura sempre `FastReactionDuration` e il reveal
> non anticipa quando entrambi lockano subito.»*
> `adr-0004-finestre-di-reazione.md:191-193`

La ragione è un canale laterale: se rispondere chiudesse la finestra in anticipo, **il tempo stesso
direbbe all'avversario che l'altro ha già scelto**. L'ADR spende `3,0 s` pieni di resolution per non
aprirlo.

✅ **E le finestre contested non sono un'ipotesi futura**: `FRTContestedBoundary`, `IsContested`,
`SortParticipantsCanonically` e `ResolveContestedBoundary` esistono in
`Turn/RTReactionOpportunityTypes.cpp`.

**KARL WIEGERS**: *«Il contratto non è sbagliato: è incompleto in un modo che l'implementazione
erediterebbe. Un orologio scritto su "risposta → stop" rende la scadenza fissa un caso speciale da
aggiungere dopo, e i casi speciali aggiunti dopo a un orologio sono il modo in cui nascono i canali
laterali di timing.»*

📝 **Il contratto va spezzato in due**, con la distinzione dichiarata: *risposta → chiude* per la finestra
a responder singolo, *scadenza fissa* per la contested. Oppure la issue dichiara le contested **fuori
scope** e scrive perché — ma non può restare muta, perché il codice le contiene già.

---

## 5. C5 ⚠️ — la pausa fermerebbe l'orologio, contro `D-351`

Sopra il `return` di C1 ce n'è un altro:

```cpp
if (bPlaybackPaused && PlaybackStepTargetElapsed < 0.f) { return; }
```
`RTTurnManager.cpp:7350`

E [`D-351`](../../decisions/RT_PDR_00_Decision_Log.md) è esplicita nel titolo: **«ESC NON FERMA IL
COUNTDOWN DELLA FINESTRA»**, con la ragione citata da `Frontend/RTFrontendNavigator.h:293` — *«ciò che in
rete non potrà esistere è fermare il tempo di tutti»*.

⛔ Un orologio collocato dopo quel `return` **si fermerebbe con la pausa del giocatore**, cioè
realizzerebbe l'opposto di una decisione accettata quattro giorni fa.

⚠️ La issue mette `TimeoutReason = Abandoned` (`D-351`) **fuori scope**, ed è ragionevole — quello è il
marcatore. Ma la **prima metà** di `D-351`, che il countdown non si fermi, è un vincolo su *questa*
issue e non è nominata.

📝 **Criterio da aggiungere**: *la pausa del giocatore non ferma l'orologio della finestra* ([D-351]) —
falsificato da: `bPlaybackPaused` vero e `OpenWindowElapsed` che smette di crescere.

---

## 🧩 Sintesi

**Il difetto della issue è reale e ben misurato.** Le cinque misure reggono una per una, e la conclusione
— la prima finestra aperta in partita ferma il turno per sempre — è corretta.

**Ma quattro rilievi su cinque colpiscono la stessa premessa**: che il sito dell'orologio fosse un
dettaglio di implementazione da lasciare a chi scrive il codice. La tabella «cosa esiste già» chiude con
*«il Tick esiste e gira»* come se il lavoro fosse innestarci tre righe. Il lavoro è **decidere da quale
tempo leggere**, e ogni risposta sbagliata è già vietata da una decisione accettata:

| Se l'orologio vive… | Cosa rompe |
|---|---|
| dopo `if (bPlaybackHeldByWindow) return;` | non gira mai — C1 |
| dopo `if (bPlaybackPaused) return;` | `D-351` — C5 |
| usando il `Dt` scalato | `D-350` + #166 riga 59 — C3 |
| dentro `TickPlayback` comunque | dipende da `BeginPlayback` — C2 |

**Resta un solo posto**: `ARTTurnManager::Tick`, prima di `TickPlayback`, con `DeltaSeconds` **non**
scalato, e con il tick abilitato indipendentemente dal playback.

📝 **Questa è la forma che la issue deve dichiarare**, non lasciare dedurre — insieme al fatto che
`SetActorTickEnabled` va sganciato da `BeginPlayback`, che è l'unica modifica strutturale del lavoro.

**Punto cieco di questo panel.** Non ho misurato cosa succede all'orologio con la **time dilation**
globale di Unreal (`UGameplayStatics::SetGlobalTimeDilation`) né in **dedicated server**, dove il tick del
manager potrebbe avere una cadenza diversa. `D-351` dice che in rete non si può fermare il tempo di
tutti: questa issue è il punto in cui quella frase diventa codice, e non l'ho verificata.

---

## Verification

| Gate | Esito |
|---|---|
| Compile · Tests · Determinism · Replay · Privacy · PIE · Packaged | **`N/A`** — referto di specifica, nessuna modifica al codice |
| `doc-tables --check` · `doc-links --check` | eseguiti sul commit che porta questo referto |

* Misure: letture di `RTTurnManager.cpp` (`Tick`, `TickPlayback`, `BeginPlayback`, `FinishPlayback`),
  `RTTurnManager_Movement.cpp`, `RTTurnManager_Blast.cpp`, `RTReactionOpportunityTypes.cpp`,
  `adr-0004-finestre-di-reazione.md`, `D-350`, `D-351`, `D-355` — tutte su `1695bfed`.
