# Spec panel — #2697 · «Il combat log del giocatore non ha consumatori»

> **Misurato il 2026-09-09** su `1757f430` (branch `docs/2534-verdetti-pie-sightwall`), leggendo `Source/`,
> `docs/decisions/RT_PDR_00_Decision_Log.md` e i corpi GitHub di `#1936`, `#1937`, `#2534`, `#2549`, `#2492`.
> Nessun gate eseguito: questo è un panel di specifica, non una misura di comportamento.
>
> **Oggetto**: [#2697](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697), aperta il
> 2026-09-09, `bug`, milestone assente.

---

## Il panel

| Esperto | Lente | Perché è al tavolo |
|---|---|---|
| **Cockburn** | attore primario, obiettivo | la issue nomina un giocatore ma prescrive una funzione |
| **Wiegers** | testabilità dei criteri | tre delle quattro voci di DoD non sono falsificabili come scritte |
| **Fowler** | confini, seconda autorità | due canali orfani rispondono alla stessa domanda |
| **Newman** | evoluzione del contratto | il canale prescritto è testo; quello deciso è tipizzato |
| **Nygard** | cosa fallisce, e dove si vede | «arriva a schermo» non dice quando sparisce |
| **Crispin** | il verde che non prova nulla | il test richiesto è già scritto e già verde |
| **Adzic** | esempi concreti | la riga d'esempio esiste in **un** ramo su tre |

---

## 1. Cosa regge — verificato

✅ **La misura centrale è esatta.** `GetRecentEventsForTeam` ha zero chiamanti fuori dai test:

```
Source/RefactorTactics/Tests/RTAnimChannelTests.cpp:276
Source/RefactorTactics/Tests/RTCombatLogTests.cpp:790, 934, 991, 1161, 1225
Source/RefactorTactics/Turn/RTTurnManager.{h,cpp}    ← dichiarazione e definizione
```

✅ **La catena tecnica che la issue presuppone esiste davvero.** La riga con il muro entra in
`RecentEvents` e quindi esce da `GetRecentEventsForTeam`:

```
RTTurnManager_Blast.cpp:1531   NoLos.SightBlockerCell = SightBlockerForLog(Los, Knowledge)
RTTurnManager_Blast.cpp:1550   AddLogEvent( DescribeEntry(NoLos) … )
RTTurnManager.cpp:148          RecentEvents.Add(…)
RTTurnManager.cpp:217          ComposeVisibleLogLines(RecentEvents, ObserverTeamId)
```

Disegnare `GetRecentEventsForTeam` **porterebbe** quella riga a schermo. La premessa non è sbagliata.

✅ **Il banco esiste** — `Scenarios/Visual/Map/SightWallIsWalkable.json`, con esiti in `Saved/RTTests/`.

✅ **L'impatto dichiarato è confermato.** `roadmap-pia.md:147` porta `PIE-HEXPLAY-6` ❌ dal 2026-09-06, e
`:154` conferma che appartiene a `RELEASE-V01` e blocca `G9`.

⚠️ **Ma la misura è più forte di come la issue la racconta.** Anche `GetRecentEvents` — la vista non
filtrata — ha **zero** chiamanti di produzione, e così `URTPlayerEventProjector::Project`. Non è che *un*
canale manchi di consumatore: **il combat log non raggiunge lo schermo per nessuna via**.

---

## 2. I findings

### 🔴 F1 — CRITICO · La issue duplica `#1936`, che è aperta e possiede questo scope

**COCKBURN**: *«Prima di chiedere chi è l'attore, chiedo chi ha già firmato per questo obiettivo.»*

[#1936](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1936) è **OPEN**, milestone
*v0.1 — Offline Vertical Slice*, epic [#1937](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1937).
Titolo: **«Il log a schermo racconta le celle, non la partita: i pannelli Canvas legacy escono e il feed del
giocatore entra»**. Il suo confine architetturale, verbatim dal corpo:

```text
Resolver autorevole → TurnLog canonico → predicato di autorizzazione
                    → Player Event Projector → FRTPlayerEvent[] → Player Event Log in UMG
```

#2697 non la cita mai, in nessuna delle sue sezioni. Le due issue chiedono la stessa cosa — che ciò che
spiega la partita raggiunga il giocatore — e ne prescrivono due implementazioni incompatibili.

**Conseguenza pratica**: chi prende #2697 senza aver letto #1936 scrive il pannello che #1936 rimuove.

---

### 🔴 F2 — CRITICO · La DoD di #2697 costruisce esattamente ciò che la DoD di #1936 smonta

**FOWLER**: *«Non è che le due issue si sovrappongano. Si annullano.»*

#1936 §A, verbatim:

> *«La presentazione Canvas del log è buona **diagnostica** e cattiva **narrazione**: espone celle, reason
> code ed eventi intermedi, cioè ciò che serve a chi sviluppa e non a chi gioca.»*

La riga che #2697 vuole a schermo:

```
(q=-1,r=0,L=0) -> (q=1,r=0,L=0): nessuna linea di tiro (muro in (q=0,r=0,L=0))
```

Sono **celle assiali e un reason code**: la definizione letterale di ciò che #1936 classifica come
diagnostica e destina a [#79](https://github.com/DegrassiAaron/refactor-tactics-main/issues/79), che *«resta
deliberatamente dettagliato e non si semplifica»*.

| | #1936 DoD | #2697 DoD |
|---|---|---|
| pannello Canvas screen-space del log | **rimosso** | **creato** |
| forma della riga | `Wraith colpisce Phase · 24 danni.` | `(q=-1,r=0,L=0) -> (q=1,r=0,L=0): …` |
| canale | `FRTPlayerEvent[]` tipizzato | `TArray<FString>` composto |

**ADZIC**: *«Metto i due esempi accanto e la domanda si risponde da sola. #1936 dà "Phase viene fermata
dall'Overwatch di Wraith". #2697 dà una coppia di coordinate assiali. Sono due prodotti, non due versioni
dello stesso.»*

---

### 🔴 F3 — CRITICO · Prescrivere `GetRecentEventsForTeam` introduce una seconda autorità sul feed

**NEWMAN**: *«Ci sono due porte orfane, e la issue ne nomina una sola — quella che l'architettura decisa
aveva messo a valle.»*

Misurato: **entrambe** hanno zero chiamanti di produzione.

| canale | tipo di uscita | chiamanti fuori dai test | decisione che lo copre |
|---|---|---|---|
| `ARTTurnManager::GetRecentEventsForTeam` | `TArray<FString>` | **0** | vista testuale, gemella di `VisibleTrailFor` |
| `URTPlayerEventProjector::Project` | `TArray<FRTPlayerEvent>` | **0** | #1936 §C — *la* porta del feed giocatore |

#2697 scrive, in grassetto: *«Il filtro per squadra è già scritto e va usato: `GetRecentEventsForTeam`»*.
È vero che il filtro è scritto e che `GetRecentEvents` sarebbe un leak — quella parte regge. Ma la
conclusione salta il fatto che **il consumatore canonico è già stato scelto, ed è l'altro**.

⛔ `CLAUDE.md` §16 e §4 (`SEARCH → REUSE → EXTEND`): due produttori dello stesso fatto sopra la stessa
domanda sono il difetto che `D-320` ha già chiuso una volta per l'overlay unità — ed è citato a
`RTHUD.cpp:610` come *«due produttori dello stesso fatto sopra la stessa testa, il difetto che #1500 ha già
misurato cinque volte, con la suite verde»*.

---

### ⚠️ F4 — MAGGIORE · `RTHUD.cpp:893` è citato con il soggetto sbagliato, e #1936 protegge quel commento

**WIEGERS**: *«Il puntatore risolve. Il criterio che ci si appoggia non è falsificabile.»*

Il riferimento è **esatto** — la riga 893 dice verbatim *«punta alla causa — la spiegazione esiste, ma solo
nell'Output Log»*. Ma il commento comincia due righe sopra, a `RTHUD.cpp:891`:

```cpp
// Banda «questa non e' una partita»: quando il GameMode sta eseguendo uno scenario, la partita normale non
// viene allestita e mancano unita' proprie, selezione e barra abilita'. Senza questa riga il sintomo non
// punta alla causa — la spiegazione esiste, ma solo nell'Output Log.
```

Il soggetto è **l'allestimento di uno scenario senza unità proprie**, non il combat log. E la frase non
descrive un difetto aperto: descrive *perché la banda esiste* — il codice che la disegna sta otto righe
più sotto.

🔴 **E #1936 la dichiara esplicitamente da preservare**, riusando la stessa frase:

> *«⚠️ **Preservata anche la banda «questa non è una partita»** … È diagnostica di **allestimento**: senza,
> chi avvia uno scenario senza unità proprie non vede più *perché* lo schermo è vuoto — la spiegazione
> esiste solo nell'Output Log.»*

∴ La voce di DoD *«il commento di `RTHUD.cpp:893` smette di essere vero»* chiede di invalidare una
motivazione che un'altra issue aperta protegge — e che comunque non parla del combat log. Il criterio non
è verificabile: non c'è nessun cambiamento al combat log che possa renderlo falso.

---

### ⚠️ F5 — MAGGIORE · La DoD promette una famiglia; il codice copre un ramo

**ADZIC**: *«"Almeno per la famiglia `Fallback`/`NoLineOfSight`" — quanti esempi concreti reggono questa
frase?»*

Misurato: `SightBlockerCell` ha **un solo sito di scrittura** in tutto `Source/`:

```
RTTurnManager_Blast.cpp:1531   NoLos.SightBlockerCell = SightBlockerForLog(Los, Knowledge)
```

Le altre voci che escono come «nessuna linea di tiro» non lo portano:

| sito | voce | nomina il muro? |
|---|---|---|
| `RTTurnManager_Blast.cpp:1531` | `NoLos` — `Combat`/`NoLineOfSight` | ✅ sì |
| `RTTurnManager_Blast.cpp:606` | `ArcRejected` | ❌ no |
| `RTTurnManager_Blast.cpp:784` | `FallbackEntry` | ❌ no |

E il test `CombatLog.SightBlockerAppearsInTheLine` (`RTCombatLogTests.cpp:1847`) lo dichiara come vincolo di
non-regressione voluto: *«senza muro nominabile la riga non cambia»*.

∴ La famiglia `Fallback` **non nomina alcun muro oggi**. Un criterio scritto su una famiglia si misura su
un ramo solo, e chi lo consuntiva dopo trova due terzi dei casi scoperti senza nessun rosso a segnalarlo.

---

### ⚠️ F6 — MAGGIORE · «Letta a schermo» non è una specifica di presentazione

**NYGARD**: *«Mi dice che compare. Non mi dice quando sparisce, e quello è il criterio che ha un costo.»*

DoD 2 chiede che la riga sia *«letta a schermo … in PIE, senza aprire l'Output Log»*. Non dichiara:

- **budget di righe** — `RecentEvents` è limitato a `MaxLogLines`; quante ne mostra il feed?
- **posizione** — #1936 §F vincola solo *«il centro della mappa resta libero e il feed non copre i controlli
  di piano»*, e rinvia il layout a `progettazione-hud.md` §4.1, che ne è l'owner;
- **durata e fase** — #1936 §F: *«piccolo durante il Planning, più visibile durante la Resolution»*;
- **collasso** — #1936 §E vieta una riga per cella; senza questa regola, il canale testuale ne emette una
  per micro-step.

Senza questi, «letta a schermo» è un verdetto di seduta ripetibile una volta sola: chi rimisura fra un mese
non sa cosa doveva vedere.

---

### ⚠️ F7 — MEDIO · Il test richiesto dalla DoD è già scritto, ed è già verde

**CRISPIN**: *«Il test che chiedete esiste. È il motivo per cui non avete visto il difetto.»*

DoD 3: *«un test copre che il canale usato sia quello **filtrato per squadra**»*. Come formulato, lo
soddisfano già `RTCombatLogTests.cpp:790, 934, 991, 1161, 1225` — che chiamano `GetRecentEventsForTeam` e
asseriscono sull'uscita. La issue stessa lo riconosce: *«il filtro per squadra è coperto e funziona; ciò che
manca è un consumatore»*.

Il test che manca ha un altro soggetto: **dato un consumatore, una riga non autorizzata non compare nel
disegno**. E #1936 ha già incontrato la trappola successiva e l'ha scritta:

> *«Un test «il fatto non autorizzato non compare» passa **anche** in un'implementazione che proietta e poi
> cancella, che è precisamente lo scenario vietato.»*

**GREGORY**: *«La domanda giusta è "chi rompe questo test?". Come scritta, la risposta è nessuno: passa già
prima che il lavoro cominci.»*

---

### ⚠️ F8 — MEDIO · DoD 4 duplica una voce già aperta in #2534

DoD 4 di #2697: *«`PIE-HEXPLAY-6` è rigiudicata su questo fatto nuovo»*.
DoD 2 di #2534: *«`PIE-VIS-SIGHTWALL` e `PIE-HEXPLAY-6` sono **rigiudicate sullo stesso banco**»*.

Due issue aperte portano la stessa rigiudicazione. Chi la esegue soddisfa una voce e ne lascia una identica
scoperta altrove — e `PIE-HEXPLAY-6` è una voce `RELEASE-V01`, quindi la doppia contabilità arriva su `G9`.

---

### ⚠️ F9 — MEDIO · Il rapporto con `D-340` non è dichiarato, e senza la riga sembra una riapertura

`D-340` è **Accettata** (2026-09-06, decisione d'autore) e chiude #2534 per la via *(b)*. Il suo punto (4):

> *«Il rosso di `G9` **cambia proprietario**: si toglie nel lavoro di **explainability**, non in quello di
> presentazione dei corpi.»*

#2697 **conferma** D-340 invece di contraddirla: il lavoro di explainability è precisamente #1936/#1937, e
#2697 sta misurando che quel lavoro non è ancora arrivato. Ma la issue non lo dice, e chi la legge dopo aver
letto D-340 vede una decisione accettata rimessa in discussione tre giorni dopo.

Serve una riga esplicita: **#2697 non riapre D-340 — ne nomina il consumatore che D-340 dava per esistente.**

---

### 💬 F10 — MINORE · Il titolo dice meno della misura

Il titolo nomina `GetRecentEventsForTeam`. La misura completa è che **nessuno dei tre canali** verso il
giocatore ha un consumatore: `GetRecentEvents`, `GetRecentEventsForTeam` e `URTPlayerEventProjector::Project`.
Il titolo attuale suggerisce un difetto di cablaggio; il fatto è che il feed del giocatore non esiste.

---

## 3. Sintesi

**🤝 Consenso del panel** — su tre punti, all'unanimità:

1. **La misura è buona e va conservata.** Zero chiamanti è un fatto, verificato; e il verdetto d'autore del
   2026-09-09 sul banco giusto è l'evidenza che mancava a #1936, che dal 2026-08-31 non aveva un caso
   concreto che ne dimostrasse l'urgenza per la release.
2. **La prescrizione è sbagliata.** Il canale da usare è quello di #1936 §C, non quello testuale.
3. **La issue non è autonoma.** Lasciata così, produce lavoro che #1936 dovrà disfare.

**⚖️ Tensione produttiva** — Wiegers contro Fowler:

- **WIEGERS**: *«#1936 è aperta dal 31 agosto e non si è mossa. Se subordiniamo #2697 a un'epic ferma,
  `G9` resta rosso e la v0.1 non parte. Un criterio ristretto e verificabile che tolga il rosso vale più
  di un'architettura corretta che non arriva.»*
- **FOWLER**: *«E se lo togliamo con il canale sbagliato, #1936 arriva e trova un pannello da rimuovere,
  un test da riscrivere e un'aspettativa d'autore su una forma di riga che la sua §A vieta. Il rosso torna,
  e la seconda volta costa di più.»*
- **MEADOWS** (sul dinamismo): il rosso di `G9` è il sintomo; la struttura è che **il feed del giocatore
  non ha mai avuto un consumatore**, e due issue lo hanno scoperto separatamente a nove giorni di distanza.
  Un terzo ritrovamento è il segnale che manca un punto di leva, non tre issue.

**🧩 Risoluzione**: la tensione si scioglie restringendo #2697 a **una** voce verificabile e agganciandola
a #1936 come sub-issue — la fetta minima che toglie il rosso *dentro* l'architettura decisa, invece che
accanto.

---

## 4. Raccomandazione

**Opzione raccomandata: (B) — sub-issue di #1936, DoD riscritta.**

| | Cosa comporta | Verdetto |
|---|---|---|
| **(A)** chiudere come duplicato, riversare la misura in #1936 | non si perde nulla; ma #1936 è larga e il rosso `G9` resta legato a tutto il suo scope | possibile, non ottimale |
| **(B)** sub-issue di #1936, ristretta a un criterio | toglie `PIE-HEXPLAY-6` con la fetta minima, dentro l'architettura decisa | ✅ **raccomandata** |
| **(C)** lasciarla autonoma come scritta | crea il pannello che #1936 rimuove e la seconda autorità di F3 | ⛔ sconsigliata |

### DoD proposta in sostituzione

```markdown
> Sub-issue di #1936 (epic #1937). ⚠️ Non riapre D-340: ne nomina il consumatore mancante.

- [ ] `URTPlayerEventProjector::Project` ha un consumatore in partita — un widget del feed, non un
      pannello Canvas — e il tipo che attraversa è `FRTPlayerEvent`, non `TArray<FString>`.
- [ ] Il proiettore emette un evento per l'esito `NoLineOfSight` che porta la cella bloccante come
      ARGOMENTO SEMANTICO (`FRTCellId`), non come testo già composto.
- [ ] Sul banco `Visual.Map.SightWallIsWalkable`, in PIE, il giocatore legge a schermo che il tiro è
      fermato da un ostacolo — senza aprire l'Output Log. La forma della frase è quella di #1936 §D/§E
      (narrazione), non `(q=..,r=..,L=..)`.
- [ ] Un test asserisce sul CONSUMATORE, non sul filtro: dato un osservatore che non conosce la cella
      bloccante, l'evento non compare nel feed — e l'autorizzazione precede la proiezione
      (cfr. `UI.PlayerEventLog.AuthorizationMatchesLogLines` e la nota sull'ordine in #1936).
- [ ] La copertura dichiarata è quella misurata: oggi `SightBlockerCell` è scritta SOLO in
      `RTTurnManager_Blast.cpp:1531`. Se la voce vale per `Fallback`, i siti :606 e :784 vanno scritti
      — altrimenti il criterio dice `Combat`/`NoLineOfSight` e basta.
- [ ] `PIE-HEXPLAY-6` è rigiudicata su questo banco. ⚠️ La rigiudicazione è UNA: la DoD 2 di #2534
      chiede la stessa cosa, e va consuntivata in un posto solo.
```

### Da correggere nel corpo, indipendentemente dall'opzione scelta

1. **Togliere il riferimento a `RTHUD.cpp:893`** o riscriverlo dicendo cosa quel commento è davvero — e
   notare che #1936 lo protegge (F4).
2. **Aggiungere la misura completa**: nessuno dei tre canali ha consumatori (F10).
3. **Nominare `#1936`, `#1937` e `URTPlayerEventProjector`** fra i riferimenti in testa (F1, F3).
4. **Dichiarare il rapporto con `D-340`** (F9).
5. **Rimuovere la frase** *«Il filtro per squadra è già scritto e va usato: `GetRecentEventsForTeam`»*: resta
   vera come divieto su `GetRecentEvents`, è sbagliata come prescrizione del canale (F3).

---

## 5. Punteggi

| Dimensione | Punteggio | Motivo |
|---|---|---|
| **Chiarezza** | 8,5 / 10 | prosa precisa, evidenza citata con comandi riproducibili |
| **Completezza** | 4 / 10 | manca l'issue che possiede lo scope, e i due canali sono uno |
| **Testabilità** | 4,5 / 10 | 2 voci di DoD su 4 non falsificabili (F4) o già verdi (F7) |
| **Coerenza** | 3 / 10 | la DoD costruisce ciò che #1936 DoD rimuove (F2) |
| **Qualità della misura** | 9,5 / 10 | ogni fatto verificato regge; il difetto è nella conclusione |

**Complessivo: 5,9 / 10** — un'ottima misura con una prescrizione che va rifatta.

---

## Follow-up candidates

- `URTPlayerEventProjector::Project` con zero chiamanti merita di essere detto **dentro** #1936: è la sua
  §C consegnata a metà, e oggi nessun documento lo registra.
- La forma «canale scritto, testato e senza consumatore» ha ora tre istanze note (#2549, #2492, #2697) più
  due misurate qui. Vale un gate: *una `UFUNCTION` pubblica in `UI/` o `HUD` senza chiamanti fuori dai test
  è un candidato inerte*, nella famiglia di `dati-senza-consumatore`.
