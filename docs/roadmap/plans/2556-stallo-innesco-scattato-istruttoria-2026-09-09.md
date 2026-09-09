# Istruttoria — l'innesco di `BOT-STALL-1` è scattato (#2556), 2026-09-09

> `ISTRUTTORIA` · **Oggetto**: [#2556](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2556), due test dei bot rossi su `main`
> **Decisione da prendere**: quale definizione di stallo usi l'oracolo dell'**arena generata**, ora che le
> due candidate producono verdetti diversi.
> **Governata da**: [`D-244`](../../decisions/RT_PDR_00_Decision_Log.md), che ha chiuso `BOT-STALL-1` con
> l'uscita (d) — *restare divergenti* — **e ne ha dichiarato l'innesco di riapertura**.
> **Misure**: su `main` = `9fa7c416`, motore libero, working tree pulito verificato prima e dopo.
> Compile `PASS`, suite `RefactorTactics` 2225 test, 3 fail tutti con owner.
> **Esito dell'istruttoria**: il rosso **non misura un difetto del bot**. Ogni definizione che guarda se il
> bot *fa* qualcosa è **identica a prima di `Model A`**; si è mossa solo quella che guarda se cambia cella.

---

## 1. L'innesco, e da dove è arrivato

`D-244` (2026-08-29) ha chiuso `BOT-STALL-1` scegliendo l'uscita **(d)** — ciascun oracolo tiene la
definizione che lo rende falsificabile sulla propria board — e si chiude con una clausola esplicita:

> *«**Innesco**: il primo terzo oracolo di stato assorbente, oppure [#149](https://github.com/DegrassiAaron/refactor-tactics-main/issues/149), che ritarando i pesi del bot consuma il margine — oggi **4 su soglia 4**, margine zero.»*

**Il margine è stato consumato.** Non da #149, e non da un ritocco ai pesi: da
[`Model A`](../../decisions/RT_PDR_00_Decision_Log.md) (`D-045`, adottato con
[#2501](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2501) il 2026-09-06), che nessuno
aveva elencato fra i possibili consumatori.

L'innesco era scritto per un evento di **bilanciamento**; è arrivato da una decisione di **resolver**.

## 2. La misura, a sei definizioni e non a una

`Bot.StallDefinitionsOnTheGeneratedTestArena` stampa sei letture. #2556 e #2501 ne citavano una.

**Arena generata — 12 turni giocati, 4 unità osservate, limite in uso: 4**

| Definizione | Oggi (`9fa7c416`) | Prima di `Model A` | Δ |
|---|---|---|---|
| **(b) immobilità** | **11** | **4** (= soglia) | **+7** |
| **(a) immobilità *sterile*** | **2** | **2** | **0** |
| (c) salute netta | 2 | 2 | 0 |
| (c) pool netto (salute+scudo) | 2 | 2 | 0 |
| (c) salute o eliminazione | 2 | 2 | 0 |
| (c) salute per turno | 2 | 2 | 0 |
| (c) eliminazione | 4 | 4 | 0 |

*(la colonna «prima» è quella registrata in [`OPEN_DECISIONS`](../../OPEN_DECISIONS.md) `BOT-STALL-1`,
misurata il 2026-08-29 da [#1551](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1551))*

🔑 **Una sola riga si è mossa, ed è quella che non guarda l'effetto.** `Model A` non ha peggiorato
l'efficacia del bot: ha aumentato i turni in cui resta fermo **combattendo**. L'oracolo gemello lo dice
già con parole sue — *«fermo 11 turni, di cui 3 inerti e **8 armati**»*: gli 8 armati sono la `(b)` che
sale mentre la `(a)` non si muove.

## 3. Il repository lo aveva previsto, e per iscritto

`RTMatchAutobattleTests.cpp:2161-2172`, scritto il 2026-08-28 accanto al `AddWarning` del margine zero:

> 🔵 **E consegna a `BOT-STALL-1` (#1551) l'evidenza che gli mancava**: le due definizioni di «stallo» non
> differiscono solo in principio — **su questa board differiscono per l'intero margine**. Con la regola
> della mappa d'autore, che esenta chi ha colpito, questa sequenza varrebbe **ZERO**; con quella di qui vale
> 4 su 4. Il «margine zero» è un **artefatto della divergenza**, non una proprietà del bot.
>
> ∴ **il rosso che il warning promette sarà un rosso DA LEGGERE**, non un difetto.

E la sequenza che consumava tutto il margine era `Hero.Wraith`, **quattro turni su quattro armati, zero
inerti** — cioè `hold-and-shoot`, *«la condotta che `ScorePlan` dichiara corretta»*.

⚠️ **Il rosso previsto è arrivato.** Questa istruttoria non scopre nulla di nuovo sul bot: constata che
l'evento per cui il warning esisteva si è verificato, e che va **letto**, non riparato.

## 4. Chi usa quale definizione, oggi

| Oracolo | Board | Definizione | Dove |
|---|---|---|---|
| `AuthoredMapEngagement` (#1088) | mappa d'autore | **(a)** sterile — esenta chi ha colpito | il predicato *decide l'oracolo* |
| `Match.Autobattle.Engages…` | **arena generata** | **(b)** immobilità — nessuna esenzione | il predicato *decide solo il referto* |

È la divergenza che `D-244` ha adottato con l'uscita (d), e la board dell'arena generata è *«la
configurazione che la partita non presidiata carica»*.

## 5. Le uscite

### (A) L'oracolo dell'arena generata adotta la `(a)`, come il gemello

Il predicato «inerte» smette di decidere solo il referto e decide anche l'oracolo, su **entrambe** le board.

- ✅ **Misurato**: `2` su soglia `4` → verde, con **margine 2**. I due rossi di #2556 si chiudono **senza
  toccare nessuna soglia** (`D-184` intatta) e senza una riga di gameplay;
- ✅ chiude anche [#2629](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2629), che lamenta
  il rumore ereditato da ogni branch;
- ⛔ **Costo, ed è quello che `D-244` temeva**: l'oracolo perde la capacità di vedere un `hold-and-shoot`
  prolungato. Un bot che restasse fermo a sparare per 30 turni sarebbe verde. `D-184` dichiara però
  legittimo il pareggio allo scadere, e distingue *«un pareggio in cui il campo si è consumato»* da *«un
  pareggio in cui nessuno cade»*: sotto la `(a)`, il secondo resta visibile — è l'immobilità **sterile** —
  mentre il primo diventa invisibile per costruzione;
- ⚠️ **e la divergenza (d) finisce**: «stallo» tornerebbe ad avere un significato unico nel repository, che
  `D-244` elencava fra i costi dell'uscita adottata.

### (B) Restare sulla `(b)` e leggere il rosso

`D-244` e il commento del 2026-08-28 dicono entrambi che questo rosso è **da leggere**. Restare significa
accettarlo come stato permanente di `main`.

- ✅ zero modifiche, e l'oracolo conserva tutto il proprio potere discriminante;
- ⛔ **Costo misurato oggi**: `main` porta un rosso permanente che ogni branch eredita — è precisamente il
  reclamo di #2629 — e il criterio *«un terzo rosso è una regressione»* diventa illeggibile: oggi i rossi
  sono già tre, e il terzo ([#2551](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2551),
  `DA_IconCatalog` non ricostruito) non c'entra con i bot.

### (C) Intervenire sul bot o passare a `Model C`

Le due uscite che #2556 elencava. **L'istruttoria le sconsiglia entrambe**, e la ragione è misurata:

- **il bot non è meno efficace**: la `(a)` è `2` prima e `2` dopo. Si interverrebbe su un comportamento che
  la misura dichiara invariato;
- l'ipotesi più naturale — *«fargli scegliere destinazioni più vicine»* — **non tocca la causa**:
  `ApplyForcedDisplacement` (`Turn/RTTurnManager.cpp:2503-2510`) azzera la destinazione **senza guardare la
  distanza**, quindi una meta a 1 cella decade come una a 5. E il bot ripianifica già ogni turno: non sta
  perdendo un piano lungo, perde il piano di quel turno;
- **`Model C` non è applicabile al bot**: `Turn/RTTurnManager.cpp:837-841` azzera `PlannedPath` e
  `PlannedWaypoints` e pianifica **solo** una destinazione, quindi al momento della spinta non esiste
  nessuna *sequenza di direzioni* da rieseguire. `Model C` è la risposta al **giocatore** che posa
  waypoint;
- ⚠️ e c'è un modo sbagliato di fare la (C) che #2556 già segnala: far ripianificare il bot dopo lo
  spostamento è **`Model B` con un altro nome**, concesso alla macchina e negato al giocatore.

## 6. Ciò che questa istruttoria **non** decide

- ⛔ **La soglia**, che è `D-184` e non materia di un test — nessuna uscita la tocca;
- ⛔ **`Model A` stesso**: il criterio di uscita di `D-045` (*«un Move annullato più di una volta ogni due
  round»*) è scattato sul **bot**, che pianifica destinazioni ed è in mischia ogni turno. **Quanto spesso
  un giocatore perda il Move non è mai stato misurato**, e finché non lo è, riaprire `D-045` sarebbe
  deciderlo sui dati di qualcun altro. È il candidato naturale per la prossima misura;
- ⛔ **La `(c)`**: `D-244` l'ha misurata e non compra verdetti diversi dalla `(a)`, al costo di una soglia
  nuova. I numeri di oggi lo confermano — tutte le letture `(c)` basate sulla salute valgono `2`, come la
  `(a)`. L'unica che diverge è `eliminazione`, che vale `4` e coincide con la `(b)` per costruzione.

## 7. Riferimenti

- [#2556](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2556) · [#2501](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2501) · [#2629](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2629) · [#1551](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1551) · [#1088](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1088)
- [`D-244`](../../decisions/RT_PDR_00_Decision_Log.md) · [`D-184`](../../decisions/RT_PDR_00_Decision_Log.md) · [`D-045`](../../decisions/RT_PDR_00_Decision_Log.md)
- [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) → `BOT-STALL-1`
- `Source/RefactorTactics/Tests/RTStallDefinitionMeasureTests.cpp` (la sonda a sei definizioni)
- `Source/RefactorTactics/Tests/RTMatchAutobattleTests.cpp:1856-1870`, `:2140-2175` (l'oracolo `(b)` e il warning che ha previsto questo rosso)
- `Source/RefactorTactics/Tests/RTAuthoredMapEngagementTests.cpp` (l'oracolo `(a)`)
