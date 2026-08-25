# ADR-0004 — Finestre di reazione: composizione dell'invariante #3 e modello unificato

> `CANONICAL` · **Stato**: Accettato — da implementare (E14) · **Data**: 2026-08-07 · **Decisore**: utente (dev singolo)
>
> ⚠️ **Emendamento 2026-08-08 — [D-021](RT_PDR_00_Decision_Log.md)**: la §5 (sospensione **globale**) e la §7
> (l'avversario «non riceve nulla») erano in tensione fra loro. Una pausa globale *osservabile* è essa stessa
> l'informazione: dice all'avversario che una finestra si è aperta. Vedi **§7-bis**, che separa la sospensione
> **logica** — che resta globale, perché serve al determinismo — dalla **presentazione**, che non deve avere
> una pausa correlata alla scelta altrui. La §6 è precisata: `Rilevato` è il requisito del **profilo Overwatch
> visivo**, non di *ogni* reazione.
>
> ⚠️ **Emendamento 2026-08-09 — [D-047](RT_PDR_00_Decision_Log.md) e [D-048](RT_PDR_00_Decision_Log.md)**: la §2
> è **precisata** (la cardinalità di `Brace` è 1 per il *profilo base*, non per natura) e la §7 è **emendata**
> per la finestra **contested**, in cui entrambi i partecipanti sanno che la finestra esiste. Nessuna delle due
> tocca il modello: la finestra continua a derivare dalla cardinalità delle risposte legali.
> Owner dell'estensione: [`spec-reaction-clash-e14.md`](../gameplay/spec-reaction-clash-e14.md).
> **Contesto sorgente**: `docs/archive/src/design/overwatch-e-fast-reaction.md` (19 sezioni)
> **Brief**: [`brief-overwatch-reazioni.md`](../gameplay/brief-overwatch-reazioni.md) (decisioni D16–D22)
> **Supera**: [ADR-0003](adr-0003-modello-azioni-v01.md) §4 riga «Stack di reazioni LIFO interattivo → scartato»
> **limitatamente alla finestra singola non annidata**; lo stack LIFO resta scartato.

## Contesto

Il canone vieta le attese nel resolver (invariante **#3**, «raccogli poi applica»: snapshot a inizio fase,
nessun `Delay`/timeline/montage, l'ordine dell'array non cambia l'esito). Su questa base:

- `spec-sequenza-turno.md` §2 classifica le **finestre di reazione live** come conflitto **C1**, «north-star,
  gated», e propone **due** riconciliazioni: (a) politiche *pre-committed*, (b) «ogni finestra apre un **nuovo
  round di sotto-risoluzione deterministico**»;
- ADR-0003 §4 scarta lo **stack di reazioni LIFO interattivo** per lo stesso motivo;
- l'epic **E5** è stata chiusa con la via (a): 24 test verdi, reazioni dichiarate in planning e valutate come
  funzioni pure sullo snapshot di fase.

Il documento sorgente sull'Overwatch, redatto dopo, porta a maturità la via **(b)** e aggiunge l'argomento di
design che la via (a) non può soddisfare: **bait, bluff e commitment** (§18). Con reazioni dichiarative il
mindgame collassa — se dichiaro «spara al primo che entra», il tank brucia sempre l'Overwatch e il portatore
di danno passa. Il valore della meccanica sta nel *non sapere se arriverà un bersaglio migliore*.

La decisione **D7** del brief sulla conoscenza parziale era passata in giornata da «nessuna finestra» a
«finestra come presentazione»; entrambe le formulazioni sono ora superate da questo ADR.

## Decisione

### 1. L'invariante #3 si **compone**, non si deroga

Il turno cessa di essere una singola risoluzione e diventa una **sequenza di sotto-risoluzioni**:

```
Turno = [ snapshot → risolvi → boundary ] · [ snapshot → risolvi → boundary ] · … · Cleanup
        └───────  «raccogli poi applica»  ──┘   ripetuto, non violato
```

Un **decision boundary** è un punto in cui la simulazione autorevole si arresta, raccoglie decisioni dai
giocatori e riparte con un nuovo snapshot. Dentro ciascun segmento valgono tutte le regole di oggi: nessun
`Delay`, nessuna timeline, nessun montage, e l'ordine dell'array non cambia l'esito.

**Riformulazione dell'invariante #3** (da recepire in `piano-canonico-mvp.md §5`):

> Resolver «raccogli poi applica»: snapshot a inizio **segmento di risoluzione**, nessun `Delay`/timeline/
> montage dentro il segmento, l'ordine dell'array non deve cambiare l'esito. Un segmento è delimitato
> dall'inizio di una macro-fase **oppure** da un decision boundary.

Il resolver non attende **mai** dentro un segmento: termina il segmento e restituisce il controllo.

### 2. Modello unificato: `opportunity → commit`

Tutte le reazioni — difensive esistenti e Overwatch — usano un solo modello:

```
Reaction armata
  └─ trigger valutato            ← funzione PURA sullo snapshot, come oggi
       └─ FRTReactionOpportunity { AllowedResponses[] }
            ├─ AllowedResponses ≤ 1 → commit immediato, NESSUN boundary   ← caso degenere: E5 di oggi
            └─ AllowedResponses ≥ 2 → decision boundary + finestra         ← Overwatch
```

`Counter`, `Deflect`, `Brace`, `Shield`, `Cleanse` hanno **una sola risposta legale**: scattano o non scattano.
Restano deterministiche e senza finestre. **I 24 test di E5 restano verdi senza cambiare comportamento atteso.**

> **Precisazione 2026-08-09 — [D-047](RT_PDR_00_Decision_Log.md)**: la cardinalità di `Brace` è **1 per il
> profilo base**, non 1 per natura. `Brace` *arma* un Reaction Profile la cui risposta universale è
> `Hold Ground` — che è esattamente il comportamento di oggi (−10 a ogni danno diretto, blocco della prima
> spinta), quindi questa riga resta vera per ogni personaggio che non dichiari altro. Un profilo d'eroe che
> dichiara una **seconda** risposta legale porta la cardinalità a ≥ 2 e apre il boundary **con la regola qui
> sopra**, senza aggiungerne una nuova. `Counter`, `Deflect`, `Shield` e `Cleanse` non hanno profili
> alternativi e restano casi degeneri. Owner: [`spec-reaction-clash-e14.md`](../gameplay/spec-reaction-clash-e14.md) §2.

`Reactions.NoResolverWait` conserva il suo significato, precisato: **il trigger resta puro** — è il *commit*
che può richiedere input, ed è un passo distinto e successivo.

### 3. Determinismo

- La decisione del giocatore entra nel **TurnLog come dato** (`OpportunityId → Response`), quindi il replay la
  riproduce senza reinterrogare nessuno.
- Il **timeout è una funzione pura dello stato**: `Timeout → HOLD`. Mai `FIRE`, perché consuma una risorsa
  irreversibile e un mancato input non deve spenderla.
- L'esito dipende **solo** da *quale* risposta arriva, **mai** da *quando* arriva dentro la finestra.
- La slow-motion durante la finestra è **presentazione**: non influenza esiti, seed, ordine, collisioni, path
  né timing logico.

### 4. Trigger simultanei

Se più unità soddisfano il trigger nello **stesso micro-step**, si genera **una sola** opportunity con più
bersagli (`FIRE A` / `FIRE B` / `HOLD`), **mai** prompt in sequenza: prompt sequenziali darebbero un vantaggio
all'ordine di iterazione, che l'invariante **#4** vieta.

Quando più reazioni distinte scattano nello stesso micro-step, l'ordine è totale e stabile:
`ReactionPriority → AbilityPriority → UnitInitiative → StableUnitId → ReactionInstanceId`.

### 5. La sospensione è **globale** *(risolve la domanda aperta §8.2 del brief)*

Durante una finestra la simulazione si ferma **per tutte le unità**, non solo per quella che decide.

**Derivata, non preferita**: se il resto della resolution proseguisse, l'avanzamento delle altre unità
dipenderebbe dal tempo di risposta umano — il *quando* tornerebbe a influenzare l'esito, contro §3 di questo
ADR e l'invariante #4. La sospensione globale è anche la più leggibile: chi guarda vede il mondo fermarsi su
un momento di tensione, non alcune unità muoversi e altre no.

### 6. Il trigger richiede il livello **`Rilevato`** *(risolve §8.4)*

La condizione del trigger è `TargetInsideArea ∧ HasLineOfSight ∧ TargetDetected ∧ ReactionStillArmed`
(§14 sorgente). Con i tre livelli di conoscenza di **E13**, `TargetDetected` significa **`Rilevato`**.

Conseguenze operative, tutte derivate senza regole nuove:

| Situazione | Livello | Trigger |
|---|---|---|
| Bersaglio nel cono, a vista | `Rilevato` | ✅ scatta |
| Bersaglio nel fumo entro 2 celle (cap `Max_Contact_Range`) | `Rilevato` | ✅ scatta |
| Bersaglio nel fumo oltre 2 celle | `Incerto` | ❌ non scatta |
| Solo rumore, nessun contatto visivo | `Incerto` | ❌ non scatta |
| Fuori vista, ultimo contatto noto | `UltimoContatto` | ❌ non scatta |

Sparare su un contatto **incerto** è una meccanica diversa (`Resonance Shot`, §17 sorgente): resta north-star.
L'Overwatch base non spara a una posizione dedotta.

**Dipendenza dichiarata**: E14 non parte prima di **E13**. Senza livelli di conoscenza, `TargetDetected` non
ha una definizione e l'Overwatch sparerebbe a unità che la squadra non percepisce.

> **Precisazione 2026-08-08.** `Rilevato` è il requisito di **questo** trigger — l'Overwatch a profilo visivo —
> e non va promosso a requisito universale di **ogni** reazione. Una reazione **acustica** può essere legittima
> con una Team Knowledge derivata dal **rumore**, cioè a livello `Incerto`: il rumore è un secondo canale
> percettivo, non una vista degradata. La tabella qui sopra descrive il profilo visivo; un profilo che dichiara
> un canale diverso dichiara anche la propria soglia. Ciò che resta vietato a tutti è sparare a una posizione
> **dedotta** senza alcun contatto (`Resonance Shot`, north-star).

> **Precisazione 2026-08-17 — [D-169](RT_PDR_00_Decision_Log.md): cosa rende falso `ReactionStillArmed`.**
> Il quarto termine della condizione era l'unico senza un elenco, e CP 14.6 ne chiedeva uno che nominava
> quattro eventi — due dei quali non esistono nel gioco.
>
> | Evento | `ReactionStillArmed` | Perché |
> |---|---|---|
> | **KO del proprietario** | ❌ falso | già vero nel codice dal CP 14.5: il watcher caduto viene saltato, «in silenzio, come per la predittiva» |
> | **Charge già spesa** | ❌ falso | `Charges = 1` (§8): `bCharged` **è** questo termine |
> | **Cap dei prompt raggiunto** | ❌ falso | §8, `MaxPromptsPerReaction`: una reaction che ha esaurito le proprie domande non entra fra quelle valutate |
> | **Movimento forzato del proprietario** | ✅ **resta vero** | il watcher **rilocalizza**: si ricostruisce a ogni micro-step dalla cella corrente, col facing **dichiarato** all'armamento. Guarda dal punto nuovo, nella stessa direzione |
>
> ⚠️ **La riga del movimento forzato è la sola che decide qualcosa di nuovo**, e conferma il comportamento
> esistente invece di cambiarlo. Il motivo è scritto accanto al codice che lo produce: *«un watcher costruito
> una volta nel Prep avrebbe la LOS di tre celle fa»*. Farlo decadere avrebbe dovuto argomentare contro
> [ADR-0005](adr-0005-orientamento.md) §4c — il facing si dichiara e non cambia dopo l'impegno — e nessuno
> lo ha fatto.
>
> ⚠️ **La rilocalizzazione non è sempre innocua, e va detto qui invece che scoperto al playtest.** La zona
> si traccia con `LineCells`, che si ferma quando la cella successiva è fuori mappa o blocca la vista. Un
> watcher spinto **contro un muro o sul bordo** conserva `ReactionStillArmed` ma ottiene `Zone.Cells` **vuota**,
> e `BuildOverwatchTriggers` lo salta (`if (!Watcher.bArmed || Watcher.Zone.Cells.Num() == 0) continue;`): la
> reaction resta armata e non controlla niente, per quel micro-step. Non è una deroga alla riga sopra — è la
> stessa regola vista da una posizione senza linea — ma è il caso in cui «guarda dal punto nuovo» non produce
> nulla da guardare. Trovato in code review, **non coperto da test**: sta a CP 14.6 decidere se una zona vuota
> per spinta meriti una voce nel TurnLog, perché oggi è indistinguibile da un turno senza bersagli.
>
> ⛔ **`Stun` e `Disarm` non compaiono in questa tabella perché non esistono**: il catalogo degli stati è
> `Braced · Burning · Electrified · Exposed · Guarded · Marked · Obscured · Reveal · Root · Slow · Wet`.
> Rientreranno insieme allo stato che li porta, e questa tabella guadagnerà due righe — non prima.

### 7. Visibilità della finestra *(risolve §8.1)*

- **Decide** solo il proprietario della reaction.
- **Vede** l'intera squadra, in sola lettura, coerentemente con la privacy di squadra dell'invariante #6
  (gli intenti alleati sono già condivisi).
- L'avversario **non riceve nulla**: né l'esistenza della finestra, né la sua durata, né l'esito prima che
  sia applicato.

> **Emendamento 2026-08-09 — [D-048](RT_PDR_00_Decision_Log.md), finestra *contested***. La riga qui sopra
> presuppone **un solo** decisore. Quando due partecipanti hanno ciascuno ≥ 2 risposte legali allo stesso
> boundary, l'opportunity è **contested** ed entrambi *sono* responder: l'esistenza della finestra è nota a
> tutti e due **per costruzione**, e non è più deducibile — è dichiarata. Restano non inviate: le risposte
> legali dell'altro, la sua scelta prima del reveal e **il momento in cui ha lockato**. Quest'ultimo è il
> punto in cui §7-bis non basta: i due sono *dentro la stessa finestra*, quindi il buffering della
> presentazione non chiude il canale. Lo chiude il **reveal a scadenza fissa** — la finestra dura sempre
> `FastReactionDuration` e il reveal non anticipa quando entrambi lockano subito. Chi non partecipa continua
> a non ricevere nulla. Costo dichiarato: ogni finestra contested spende **3,0 s pieni** di resolution;
> **budget invariato**, perché un boundary contested vale **un solo** prompt condiviso (§8). Owner:
> [`spec-reaction-clash-e14.md`](../gameplay/spec-reaction-clash-e14.md) §7.

È l'unica delle quattro domande che non discende dagli invarianti: in tre secondi la coordinazione vocale non
è realistica, quindi la visione dell'alleato serve alla **leggibilità**, non alla decisione. Se al playtest
risultasse rumore inutile, si degrada a «vede solo il proprietario» senza toccare il modello.

### 7-bis. Privacy **temporale**: la finestra non deve essere deducibile *(emendamento 2026-08-08, [D-021](RT_PDR_00_Decision_Log.md))*

La §7 copriva il **payload**: l'avversario non riceve i dati della finestra. Non basta. La §5 sospende la
simulazione **per tutte le unità**, e se quella sospensione è osservabile l'avversario impara comunque tutto
ciò che conta: *qualcuno ha appena ricevuto una scelta, in questo istante, su quel micro-step*. Una pausa
variabile è un canale laterale — la durata del silenzio è correlata alla decisione altrui.

Questo è un **requisito di privacy**, non una rifinitura di presentazione. I personaggi devono continuare a
essere percepiti come agenti che agiscono in contemporanea.

| Livello | Regola |
|---|---|
| **Logico** (server) | La progressione **può** sospendersi al Decision Boundary: serve al determinismo (§3, §5). Resta invariato |
| **DTO verso l'avversario** | Non contiene trigger, opportunity, `AllowedResponses`, identità del responder, timeout, né metadati da cui dedurre la finestra |
| **Presentazione avversaria** | **Nessuna pausa variabile correlata alla scelta privata.** Buffering, pacing, *fixed resolution beat* o meccanismo equivalente: il ritmo osservato non deve dipendere dal tempo di risposta di un altro giocatore |
| **Autorità** | Timeout e risposta sono **server-authoritative**: un client lento non allunga la finestra, e un client che non risponde ottiene `HOLD` |

Entra nell'invariante #6 come sua estensione: *zero leak di intenti e informazioni private* comprende ora
**anche il tempo**. La vecchia formulazione «payload visibile solo alla propria squadra» non copriva questo
caso perché il canale non è il pacchetto, è la sua assenza.

**Verifica**: test di packet privacy (nessun campo della finestra nel DTO avversario) **e** un piano di
verifica del canale temporale in **M10/E14** — la resolution osservata da un avversario deve avere la stessa
forma con e senza finestra aperta. Se in v0.1 (offline, contro bot) il requisito non è pienamente
verificabile, si documenta l'architettura e si apre la issue per il multiplayer: **non si degrada il
requisito** a «lo sistemeremo con la UI».

> ✅ **La issue esiste dal 2026-08-13: [#759](https://github.com/DegrassiAaron/refactor-tactics-main/issues/759)**,
> `post-v0.1`, milestone di roadmap **M10**. Fino a quel giorno questa prescrizione non aveva un destinatario
> — nessuno dei tre checkpoint di E14 aveva una voce di DoD per il canale temporale, e la riga qui sopra si
> leggeva come se il lavoro fosse tracciato.
>
> **Cosa è già coperto**: il livello **DTO** lo è da CP 14.3 — `Overwatch.OpportunityLeaksNoFuture`
> (`RTReactionOpportunityTests.cpp:159`) verifica l'**elenco chiuso dei campi per riflessione**, quindi chi
> aggiunge un campo deve passare di lì e dichiarare perché non è informazione futura. Scoperta resta la
> **presentazione avversaria**: nessuna pausa variabile correlata alla scelta privata. In v0.1 — offline,
> contro bot — non c'è un avversario umano che osservi la resolution, quindi il requisito non è
> falsificabile: è la ragione per cui #759 nasce `post-v0.1` invece di entrare in un DoD che non potrebbe
> verificarla.

### 8. Parametri iniziali *(risolve §8.3)*

| Parametro | Valore | Origine |
|---|---|---|
| `FastReactionDuration` | **3.0 s** — **baseline di sistema per ogni Fast Reaction**, non solo per l'Overwatch | §3 sorgente · confermato da [`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md) §8 |
| `MaxPromptsPerReaction` | **3** | §5 sorgente; data-driven |
| `DefaultTimeoutBehavior` | **Hold** | §3 di questo ADR |
| `Charges` (Overwatch v0.1) | **1** | §5 sorgente |
| Cap aggregato di finestre per turno | **nessuno** | decisione **D20**: si misura al playtest |

`MaxPromptsPerReaction` limita le opportunity di **una** reaction; D20 riguarda il budget **aggregato**, che
resta volutamente non limitato. I due non sono in contraddizione.

> **Precisazione 2026-08-09 — [D-048](RT_PDR_00_Decision_Log.md)**: un boundary **contested** vale **un solo**
> prompt, condiviso fra i due partecipanti. Il caso peggiore di questa tabella non cambia, e la soglia
> d'allarme di **20 s** non va rimisurata da capo. Contarne due l'avrebbe raddoppiato proprio mentre il reveal
> a scadenza fissa rende ogni finestra **incomprimibile**: con quella regola i `3 × 3,0 s = 9 s` diventano un
> **minimo garantito** invece di un massimo raggiunto solo per indecisione. È una misura di **CP 14.7**, non
> una stima.
>
> 🔎 **Precisata il 2026-08-13 ([D-128](RT_PDR_00_Decision_Log.md)): le due frasi qui sopra parlano di
> statistiche diverse.** «Non va rimisurata da capo» vale per il **massimo**, che con un solo prompt per
> boundary resta quello di questa tabella; «da misurare, non da stimare» vale per il **minimo garantito**, che
> il reveal fisso alza. Poiché la soglia si legge come `p50`/`p90` (§Revisione), dopo CP 14.7 la taratura si
> **ripete** — non si riapre la decisione, si riesegue la misura sul regime nuovo. La distinzione non era
> esplicita e le due letture sembravano contraddirsi.

### 8-bis. Il colpo deciso al boundary è un **tiro normale** *(D-189, 2026-08-25)*

Copertura e facing si applicano come per il Blast: `EffectiveCoverReduction`, lo stesso della risoluzione
ordinaria. Vale per l'Overwatch `FIRE` e per il boundary predittivo.

**Perché.** Questo brief dichiara che chi arma *«si trattiene il colpo e spara con la propria arma quando
qualcuno passa»*. Se l'arma è la stessa, lo sono anche le regole del tiro — l'onere della prova sta su chi
vuole un'eccezione, e nessuno l'aveva scritta. ⚠️ **Fino al 2026-08-25 non era così**, e non per scelta: due
percorsi chiamavano `ApplyDamage` diretto, e un bersaglio dietro un muro prendeva danno pieno da un Overwatch
e ridotto da un attacco base della stessa arma. Nessun test difendeva l'asimmetria
([#888](https://github.com/DegrassiAaron/refactor-tactics-main/issues/888)).

**E l'Overwatch sarebbe forte due volte.** Chi arma paga già il costo-opportunità di §8: niente attacco quel
turno, e tutto perso se nessuno passa. Un colpo che in più **buca i muri** trasformerebbe la scommessa in un
upgrade. Il counterplay del difensore resta la **rotta**: non passare di lì. Chi ci passa comunque può pagare
meno se si è coperto.

**Quale cella.** La domanda *«quella lasciata o quella raggiunta?»* presuppone un movimento continuo che il
resolver non ha: si lavora a **micro-step**, e il bersaglio è in una cella determinata quando il colpo parte.
I due percorsi la conoscono in modo diverso, e la passano esplicitamente:

| Percorso | Cella del bersaglio | Perché |
|---|---|---|
| Predittivo | `Armed.LockedCell` | è la cella **su cui si è scommesso**; al momento del danno il troncamento del movimento non è ancora avvenuto |
| Overwatch `FIRE` | la cella corrente | al suo micro-step è già quella giusta |

⚠️ **Copertura e facing arrivano insieme**, perché `EffectiveCoverReduction` li calcola in una funzione sola:
chi viene preso fuori dall'arco frontale perde il beneficio del muro, come in ogni altro tiro. Le eccezioni
vivono nelle **abilità dei singoli eroi** ([D-014](RT_PDR_00_Decision_Log.md)/[D-028](RT_PDR_00_Decision_Log.md)),
mai nella regola generale.

🔴 **Resta invisibile al giocatore**: nessun elemento di presentazione dice che quel colpo rispetta il muro.
È tracciato in [#1392](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1392) — è lavoro di presentazione, e questa regola non lo aspetta.

### 9. Cosa **non** cambia

Restano scartati o north-star: **stack di reazioni LIFO interattivo** · **interrupt annidati** (una finestra
non può aprirne un'altra) · reveal progressivo · 5 categorie di velocità · timeline di esecuzione 45–60 s ·
probabilità di qualunque tipo. Le macro-fasi `Prep → Dash → Blast → Move → Cleanup` sono **invariate**, Move
dopo Blast (ADR-0003 §1).

## Alternative considerate

| Alternativa | Esito |
|---|---|
| Confermare D7: nessuna finestra, tutto dichiarativo | **Scartata dall'utente**: rinuncia definitiva a bait, bluff e commitment |
| Finestra come sola *presentazione*, condizioni dichiarate in planning | **Scartata**: il primo bersaglio che entra brucia sempre la reaction — il bait diventa banale |
| Filtro di bersaglio dichiarato (soglia di minaccia, «ignora il primo») | **Scartata**: neutralizza il bait ma sposta tutto il mindgame in planning, rendendolo statico |
| Finestre solo per Overwatch, difensive invariate | **Scartata dall'utente**: due modelli di reazione da mantenere |
| **Modello unificato con caso degenere** ✅ | **Scelta**: un solo modello, E5 conservata come `AllowedResponses ≤ 1` |

## Conseguenze

**Positive**: un solo modello di reazione, estendibile a Guard, Counter, Dodge, Intercept, Ambush, Trap,
Opportunity Attack senza toccare la pipeline · bait e bluff diventano gameplay reale · l'aggancio esiste già
(`Action.SuppressiveLine` in fase Prep e `Hero.Wraith.InterceptShot`, entrambi a catalogo e testati) · E5 resta
chiusa e verde.

**Negative / costi**:
- la **forma del turno** cambia: da una risoluzione a una sequenza di segmenti. Tocca `ARTTurnManager`, il
  playback e il TurnLog (che deve registrare i boundary e le risposte);
- **netcode a N round-trip per turno** invece di uno (rilevante da **M10**, non prima);
- **durata della resolution non limitata**: `MaxPromptsPerReaction 3` × 3 s = **9 s per una sola unità
  armata**, contro i 12 s di `Resolution_sec` del workbook. Rischio accettato con D20, da misurare;
- in turni **simultanei**, chi non ha reazioni armate attende senza agire;
- il TurnLog cresce di una dimensione (decisioni), e la sua serializzazione va **versionata**.

**Invarianti**: **#3 riformulato** (§1) — non indebolito: nessuna attesa entra in un segmento. #1, #2, #5, #7
invariati. **#4 preservato**: l'input è un dato del log, il timeout è una funzione pura, i trigger simultanei
non dipendono dall'ordine di iterazione. **#6 preservato e rafforzato**: la opportunity inviata al client
contiene **solo il presente** — mai trigger futuri, percorsi futuri, opportunity future o intenti avversari.

## Verifica

| Test | Cosa dimostra |
|---|---|
| `Reactions.SingleResponseCommitsWithoutWindow` | `AllowedResponses ≤ 1` non apre boundary: E5 invariata |
| suite E5 (24 test) **invariata** | l'unificazione non cambia il comportamento delle difensive |
| `Reactions.NoResolverWait` **invariato** | il trigger resta una funzione pura senza `UWorld` |
| `Overwatch.TimeoutIsHold` | il default allo scadere è puro e non consuma la charge |
| `Overwatch.DecisionIsReplayable` | stesso snapshot + stesse risposte registrate ⇒ stesso TurnLog e stesso checksum |
| `Overwatch.SimultaneousTargetsSingleOpportunity` | trigger nello stesso micro-step ⇒ una opportunity, non prompt in sequenza |
| `Overwatch.OrderIsDeterministic` | permutare l'input non cambia l'ordine delle opportunity |
| `Overwatch.RequiresDetection` | contatto `Incerto` (fumo oltre 2, o solo rumore) **non** arma il trigger |
| `Overwatch.OpportunityLeaksNoFuture` | la opportunity non contiene trigger, percorsi o posizioni future |
| `Overwatch.HoldKeepsArmed` | `HOLD` perde l'opportunità, non la reaction |
| ~~`Overwatch.CancelledByStun` · `…ByForcedMovement`~~ → `Reactions.ArmedZoneFollowsCurrentCell` | 🔴 **Riga corretta il 2026-08-17 ([D-169](RT_PDR_00_Decision_Log.md)), ed era falsa due volte.** *(a)* Diceva «l'overwatch armato non è garantito fino a fine turno» per il **movimento forzato**, cioè l'opposto di ciò che §6 stabilisce: il watcher spinto **rilocalizza**. *(b)* Citava due test che **non sono mai esistiti** — `grep -rn "CancelledByStun\|ByForcedMovement" Source/` dà zero — e nessun gate lo vede, perché `check-docs-symbols.py` non risolve i nomi delle automation test. Il pin vero verifica che la zona sia funzione di *(cella, facing)*; `Stun` e `Disarm` escono con lo stato che non esiste |

Checkpoint in [`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) epic **E14** (CP 14.1–14.5); questo ADR **è** il CP 14.1.

## Revisione → **punto di taratura** *(riformulato il 2026-08-13, [D-128](RT_PDR_00_Decision_Log.md))*

Alla chiusura di **CP 14.6**, che misura la durata reale della resolution con 1, 2 e 3 unità armate.

> ⚠️ **Questa sezione diceva «Rivedere alla chiusura di CP 14.5», ed erano sbagliate due cose.** Il
> checkpoint: la misura con 1/2/3 unità armate è di **CP 14.6** — `roadmap-v0.1.md` §5 e
> [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md) lo dicono da tempo, e CP 14.5 misura a decisore immediato,
> cioè con Decision Time nullo per costruzione. E il **verbo**: quello che 14.6 produce non è l'apertura di
> una revisione architetturale, sono i numeri con cui si **tarano** i parametri.
>
> I due rientri che questa sezione teneva pronti sono stati **consumati prima della misura**, e registrarlo
> vale più che prometterli:
>
> | Rientro | Stato |
> |---|---|
> | *cap aggregato per turno condiviso* | **già scelto** da [D-050](RT_PDR_00_Decision_Log.md) — il Decision Time Bank *è* un cap aggregato, in tempo anziché in prompt. `DOC_CONFLICT_MATRIX` riga 60 marca `D20` `SUPERSEDED` |
> | `MaxPromptsPerReaction = 1` | **disponibile**, ma il [triage del 2026-08-10](../roadmap/plans/overwatch-runtime-lifecycle-triage-2026-08-10.md) ha misurato che in 2v2 il valore `3` è **già irraggiungibile** da una singola Overwatch: «diventa molto meno probabile che serva» |
>
> Restano entrambi validi e **compatibili** col bank — sono parametri, si attivano insieme. Ciò che cade è
> l'idea che 14.6 debba *scegliere* fra loro: quella scelta è già stata fatta altrove.
> [`spec-decision-time-bank.md`](../gameplay/spec-decision-time-bank.md) §2.2 lo scrive già in forma negativa
> — «si tarano i valori, **non si riapre ADR-0004**» — ed era l'unico documento a dirlo.

**Soglia di allarme**: `ReactionDecisionSeconds` **stabilmente** sopra i **20 secondi**, letta come **`p50` e
`p90`** — non come massimo osservato. La parola «stabilmente» era già qui e indicava il caso **tipico**; ora
lo dice anche la statistica, perché un massimo su poche esecuzioni è rumore e una soglia che non può scattare
non sorveglia niente.

> **Perché il tipico e non il peggiore** *(precisa [D-048](RT_PDR_00_Decision_Log.md), non la cambia)*. D-048
> contiene entrambe le letture a mezza riga di distanza: «il caso peggiore di §8 e la soglia d'allarme di 20 s
> **non si rimisurano da capo**», e subito dopo «il reveal fisso rende i 9 s un **minimo garantito** … **da
> misurare**, non da stimare». Non litigano, si dividono il lavoro: un boundary *contested* vale **un solo**
> prompt, quindi il **massimo** non cambia; ma il reveal a scadenza fissa alza il **pavimento**, ed è il `p50`
> a vederlo. Conseguenza operativa: dopo CP 14.7 la taratura si **ripete**, non si riapre.
>
> **Campione**: ≥ **10 partite**, lo stesso di `InitialBankMs` in
> [`spec-decision-time-bank.md`](../gameplay/spec-decision-time-bank.md) §3.2 — così le due misure leggono lo
> stesso campione e sono confrontabili. Se il costo è proibitivo con una UI appena consegnata, l'alternativa
> è N resolution con risposte scriptate a tempi realistici, **dichiarando N e la provenienza**: è una scelta
> di strumentazione, non di modello.

> **La soglia dei 20 s è coerente con le bande di formato** ([`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md)
> §9): playback tipico **8–15 s** in 2v2 e **12–20 s** in 3v3 Standard. Attenzione a **cosa** si confronta: le
> bande misurano il **playback** (Presentation Time), la soglia misura la resolution **comprese le finestre**
> (Presentation + Decision Time). Sommarle senza distinguerle è l'errore che §11 di quella spec esiste per
> evitare — e il motivo per cui `ReactionDecisionSeconds` è una metrica separata da `ResolutionPlaybackSeconds`.

**Seconda revisione** a **M10** (rete e privacy): il modello a N round-trip va verificato contro latenza,
riconnessione e timeout di rete, dove il «timeout → HOLD» diventa anche la risposta al giocatore disconnesso.
