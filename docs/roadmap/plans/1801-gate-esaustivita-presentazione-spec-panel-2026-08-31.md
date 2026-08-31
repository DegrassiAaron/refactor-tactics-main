# `#1801` — il gate di esaustività ResolvedEvent → presentazione, sotto critique

> **Referto di revisione**, non owner. Sottopone a critique la specifica di
> [`#1801`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1801) — *«validator di esaustività
> per il binding ResolvedEvent → presentazione»* — e ne misura ogni criterio contro il repository.
>
> **Data**: 2026-08-31 · **Modo**: critique · **Focus**: testing + requirements
> **Base**: `origin/main` `9727316f` *(la misura è iniziata su `6094f309`: il repo è avanzato durante il pass)*
>
> ⛔ **Nessun owner doc toccato, nessuna suite eseguita, nessuna riga di `Source/` cambiata.**
> ✅ **Eseguito dopo conferma**: i sette aggiornamenti di §7 su `#1801`, in [un commento](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1801#issuecomment-5479136442),
> e la **milestone assegnata**. Due scelte lasciate aperte dal pass sono state prese dall'autore — §7.1.

---

## 1. Il verdetto in una riga

> **La issue ha ragione sul problema e sbaglia il criterio che dovrebbe dimostrarlo risolto: il suo test
> centrale — *«si aggiunge un valore fittizio all'enum in un test dedicato»* — non è scrivibile, perché un
> `UENUM` è compile-time. Il repository risolve lo stesso problema in dieci punti con un pattern diverso, e il
> precedente che la issue stessa cita porta la lezione che le manca: là un'assertion di copertura era già
> risultata VERA PER COSTRUZIONE, e l'ha scoperta una code review.**

---

## 2. Ciò che la issue ha ragione di dire

| Claim | Esito | Evidenza |
|---|---|---|
| `ERTResolvedEventType` può crescere senza che nulla se ne accorga | ✅ | quattro valori — `Move · Attack · HazardDamage · Defeated` (`RTResolvedEvent.h:10-16`) — e nessun test li enumera |
| I valori sono consumati da delegate che il C++ non conta | ✅ | il dispatch è una catena di `if/else if` su **tre** dei quattro tipi (`RTTurnManager.cpp:6117-6141`), senza `else` finale: `HazardDamage` non ha alcun ramo |
| Un quinto valore *«compilerebbe, risolverebbe correttamente la logica, e sparirebbe a schermo»* | ✅ **misurato** | il dispatch cade nel nulla per default: nessun `else`, nessun `checkNoEntry`, nessun log |
| `FRTResolvedEvent` è il confine e non si sposta | ✅ | `D-278`; e `RTTurnLog.h:546` lo dichiara già distinto dal TurnLog canonico |
| Il precedente utile è il gate delle icone | ✅ **ed è più ricco di quanto la issue dica** | vedi §4 |

➕ 🔴 **E c'è di più di quanto la issue dichiari — misurato**: `HazardDamage` compare **una sola volta in tutto
`Source/`**, ed è la riga che lo dichiara (`RTResolvedEvent.h:14`). Nessuno lo **emette**, nessuno lo
**consuma**, nessuno lo dichiara `NoPresentation`.

∴ Il difetto che `#1801` esiste per prevenire **è già avvenuto**, e non è un'ipotesi sul futuro: un valore
dell'enum è entrato ed è rimasto muto. È il primo caso di prova del gate, disponibile senza scrivere una riga
di gameplay — e trasforma un criterio della DoD (*«tutti i valori esistenti hanno una voce o un
`NoPresentation` dichiarato»*) da adempimento formale a **domanda vera**: `HazardDamage` è un
`NoPresentation` legittimo, o un evento che qualcuno ha dimenticato di emettere?

---

## 3. 🔴 Il difetto centrale: il criterio d'accettazione descrive un test che non si può scrivere

La DoD chiede:

> *«Il test è scritto in modo da **non poter passare per omissione**: si aggiunge un valore fittizio all'enum
> in un test dedicato e si verifica che il gate diventi rosso.»*

**Un `UENUM` è compile-time.** Non esiste un modo, dentro un Automation Test, di aggiungere un valore a
`ERTResolvedEventType` e osservare il gate diventare rosso: servirebbe ricompilare con l'enum modificato, cioè
un esperimento manuale, non un test che gira in `rt-suite`.

### E il repository ha già la risposta, dieci volte

Il pattern esiste, con la sua convenzione e il suo tranello documentato:

```cpp
// RTScenarioLoader.cpp:66-67
// `NumEnums() - 1`: l'ultimo e' il `_MAX` sintetico che UHT aggiunge, e non e' un valore scrivibile.
for (int32 I = 0; I < Enum->NumEnums() - 1; ++I)
```

Usato in `RTGoldenCorpusTests` (`ERTLogCategory`), `RTHexTests` (`ERTHexSurface`), `RTReactionTests`
(`ERTReactionTrigger`), `RTIconCatalogTests` (`ERTIconCategory`), `RTScenarioDraft`, `RTScenarioLoader`,
`RTHexBotIntegrationTests`, `RTScenarioAuthoringTests`, `RTScenarioRunResetTests`.

🔑 **Un gate costruito così copre i valori futuri per costruzione**: non ha bisogno di simulare la crescita
dell'enum, perché itera l'enum vero. Il giorno in cui `AttackFootprint` entra
([`D-301`](../../decisions/RT_PDR_00_Decision_Log.md), in PR [#1948](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1948)),
il gate lo vede senza che nessuno abbia toccato il test.

📝 **Riformulazione proposta del criterio** — falsificabile, e scrivibile oggi:

- [ ] Il gate itera `StaticEnum<ERTResolvedEventType>()->NumEnums() - 1`, **non** una lista di valori scritta
      a mano: una lista è essa stessa una copertura da mantenere, e il difetto si sposta invece di chiudersi.
- [ ] Un test costruisce un binding **volutamente incompleto** (una voce tolta) e verifica che il gate riporti
      **esattamente quella** mancanza — non «almeno una».
- [ ] Un test verifica che `NumEnums() - 1` sia il conteggio atteso, così l'aggiunta di un valore **rompe una
      riga leggibile** invece di scivolare dentro un ciclo.

---

## 4. 🔴 La lezione del precedente che la issue cita — e non riporta

La issue rimanda al gate delle icone. Quel gate porta un commento che è **esattamente il rischio di questa
issue**, e la issue non lo eredita — `RTIconCatalogTests.cpp:200-207`:

> ⚠️ *«Qui c'era un'assertion **che non poteva fallire**, e la sua storia vale il commento: diceva "un catalogo
> che copre l'insieme richiesto non ha mancanze", costruendo il catalogo con `MakeCoveringIconCatalog()` — che
> itera `RequiredIconIds()` — e confrontandolo con `FindMissingRequiredIcons()`, che itera `RequiredIconIds()`.
> **Vero per costruzione**, qualunque cosa facesse la derivazione. È lo stesso difetto che questo checkpoint
> denuncia, lasciato dentro il test che lo denuncia; l'ha trovato la code review.»*

**Per `#1801` il rischio è identico e più insidioso**, perché la simmetria è più naturale: se la tabella di
mapping viene *derivata* dall'enum e il gate *itera* l'enum, «ogni valore ha una voce» è vero per costruzione
e il test è verde per sempre — mentre il difetto che deve sorvegliare (una voce **mancante**, o presente ma
**vuota**) resta invisibile.

La forma che cade davvero è già scritta, tre righe sotto:

> *«La forma sotto invece cade in due modi: se `Certainty` sparisce dall'insieme richiesto le mancanze
> diventano 0, se qualcuno ne aggiunge una quarta diventano 4.»*

⚠️ E il progetto ha un secondo modo di essere verde a vuoto, che questa issue deve evitare per iscritto: un
`RunTest` che **esce prima delle assertion** riporta comunque `Success`. Un gate che restituisce presto — per
un `StaticEnum` nullo, per un Data Asset non caricato — passa silenzioso. Il precedente lo tratta
esplicitamente: *«Nessun catalogo non è "zero mancanze": è la mancanza totale»* (`RTIconLibrary.cpp:146-149`).

📝 **Da aggiungere alla DoD**: il gate, ricevendo un binding **assente**, deve riportare **tutte** le voci
mancanti — mai zero.

---

## 5. Ambiguità che decidono l'architettura, e che la spec lascia aperte

### 5.1 ⚠️ *(Fowler)* «validatore **o** Automation Test» — la disgiunzione non è innocua

Sono due cicli di vita diversi, e il precedente delle icone ha **entrambi**, con ruoli distinti:

| Strumento | Dove vive | Cosa morde |
|---|---|---|
| `URTIconLibrary::ValidateIconCatalog` | libreria pura, `RTIconLibrary.h:87` | chiamabile ovunque, anche da un commandlet |
| `RTBuildIconCatalogCommandlet` | Editor | *«se non scendono a zero, il catalogo non è pronto e il commandlet lo dice»* |
| `RefactorTactics.IconCatalog.*` | Automation | gira in `rt-suite` |

Un validatore editor-side **non gira in `rt-suite`**; un Automation Test **non blocca chi salva un Data Asset
rotto**. Scegliere «o» significa lasciare scoperta una delle due porte.

📝 **Raccomandazione**: dichiarare la **funzione pura** come owner del giudizio (una `FindMissing…` che
restituisce le mancanze), e **sopra di essa** l'Automation Test. Il validatore editor diventa un consumatore in
più, non un'alternativa.

### 5.2 ⚠️ *(Wiegers)* Dove vive il mapping non è specificato

`D-278` dice *«dato dichiarativo»*; la issue eredita la formula e non sceglie il **supporto**: Data Asset,
tabella C++, `DataTable`. È una scelta architetturale che, non dichiarata, la prende chi implementa.

Il precedente indica una strada già percorsa: **Primary Data Asset** (`URTIconCatalogData`) + libreria pura +
commandlet di build. ⚠️ Ma con una differenza che va detta: le icone sono asset, e il loro catalogo **deve**
essere un asset; una cue di presentazione **potrebbe** invece essere una tabella C++ finché nessun artista la
tocca. La issue deve dichiarare quale delle due, e perché.

### 5.3 ⚠️ *(Adzic)* `NoPresentation` è definito per negazione

> *«`NoPresentation` è una dichiarazione **positiva** e distinguibile dall'assenza, non il default implicito di
> un `TMap` che non trova la chiave.»*

La proprietà è giusta ma non è ancora un esempio. Il precedente ha la forma concreta:
*«Una chiave dichiarata con asset nullo NON copre: è esattamente il widget vuoto che CP 20.1 vieta»*
(`RTIconLibrary.cpp:153`).

📝 **Da rendere esplicito**: una voce che nomina il tipo **ma lascia le cue vuote** conta come **mancanza**,
non come `NoPresentation`. Sono due stati diversi, e senza questa riga collassano nel medesimo.

### 5.4 ✅ Un criterio è già realizzabile, e la issue non lo sa

> *«La presentazione **non** influenza l'esito logico: il TurnLog è identico con e senza presentazione.»*

**`bEnablePlayback` esiste** e i test lo pilotano già a varianti on/off
(`RTMatchAutobattleTests.cpp:1116-1159`, *«accende o spegne il playback per intero»*). Il criterio è scrivibile
oggi: stessa partita, due valori del flag, TurnLog e `StateHash` confrontati.

---

## 6. ⚠️ Una dipendenza che raffredda senza motivo — `#789` è **v0.6**

La issue dichiara:

> *«Interagisce con `CP 41.4` (#789), che dà a `FRTResolvedEvent` un secondo consumatore via GAS (`D-260`).»*

**Misurato**: `#789` è OPEN in milestone **v0.6 · Ability Runtime** — quattro release dopo. E `D-260` fissa che
GAS *«è un layer di supporto obbligatorio **post-v0.1**»*.

∴ Trattare `#789` come vincolo di progettazione per un gate che serve alla **v0.1** congela decisioni su un
consumatore che non esiste e non esisterà per quattro milestone. 📝 **Declassare a «correlata»**, con la nota
che il contratto dovrà reggere un secondo consumatore — che è diverso dal progettarlo ora per lui.

🔴 **E il contrasto rende visibile il difetto di pianificazione vero**: `#1801` non ha milestone, il suo
consumatore `#1945` è in **v0.1 · Leggibilità**, e `D-301` l'ha reso **bloccante**. Una issue senza data che
blocca una issue con data è una consegna che nessuno sorveglia.

---

## 7. Matrice e mutazioni proposte

✅ **Eseguite dopo conferma esplicita.**

| Oggetto | Verdetto | Motivo |
|---|---|---|
| `#1801` come owner del contratto e del gate | **REUSE** | corretto e senza alternative: `D-278` lo assegna, nessun'altra issue lo rivendica |
| Aprire una issue separata per il gate | **REJECT** | è precisamente lo scope di `#1801` |
| Il criterio *«valore fittizio nell'enum»* | **UPDATE** — `U1` | non scrivibile; sostituirlo con l'iterazione di `StaticEnum` + binding incompleto |
| La lezione «vero per costruzione» | **UPDATE** — `U2` | è nel precedente che la issue cita, e non è ereditata |
| «validatore **o** Automation Test» | **UPDATE** — `U3` | scegliere: funzione pura come owner, test sopra, validatore come consumatore |
| Supporto del mapping non dichiarato | **UPDATE** — `U4` | Data Asset o tabella C++: è una scelta architetturale, va nella issue |
| `#789` come dipendenza | **UPDATE** — `U5` | è v0.6; declassare a correlata |
| Milestone assente | **UPDATE** — `U6` | 🔴 bloccante per `#1945` che è v0.1 |
| `HazardDamage` senza presentazione **oggi** | **UPDATE** — `U7` | il difetto è già presente: è il primo caso di prova, e la issue non lo nomina |

Tutte e sette sono **additive**: nessuna riscrive lo scope, che è corretto. Applicate in
[un commento](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1801#issuecomment-5479136442) più la milestone.

### 7.1 Le due scelte che il pass non poteva fare, e che l'autore ha preso

| Scelta | Esito | Conseguenza misurata |
|---|---|---|
| **Milestone** (`U6`) | **v0.1 · Leggibilità** | la stessa del consumatore `#1945`: `D-301` rende questa issue bloccante, e un prerequisito senza data non lo sorveglia nessuno |
| **Supporto del mapping** (`U4`) | **tabella C++**, non Data Asset | `D-124` tiene *«Niagara dedicato a ogni abilità»* fuori dalla v0.1 e `Content/` non ha un solo asset Niagara: in v0.1 le cue le tocca chi scrive codice, e un `.uasset` aggiungerebbe versionamento e lease per un dato che nessun artista modifica |

🔑 **E le due scelte interagiscono, il che non era ovvio prima di farle.** Senza Data Asset **non c'è nulla da
validare al salvataggio**: il validatore editor/commandlet del precedente (§5.1) **non ha oggetto**, e resta la
coppia *funzione pura + Automation Test*. È la ragione per cui `U3` mette l'owner del giudizio nella funzione
pura e non nel test: se un giorno le cue diventano asset, il validatore torna utile **senza che il gate cambi**.

⚠️ **`«dato dichiarativo»` di `D-278` resta rispettato**: dichiarativo si oppone a **branching**, non a
*compilato*. Una tabella letta da una funzione pura è dato; un `switch` che esegue presentazione è codice.

---

## 8. Verifiche

### Eseguite
- `gh issue view` su `#1801`, `#789`; `git fetch` e conferma base `9727316f`
- Lettura di `RTResolvedEvent.h`, del dispatch `RTTurnManager.cpp:6109-6141`, di `RTIconLibrary.{h,cpp}`,
  `RTIconCatalogTests.cpp`, `RTScenarioLoader.cpp`
- `grep` su `StaticEnum<`, `NumEnums()`, `bEnablePlayback`, `FindMissingRequiredIcons`,
  `ValidateIconCatalog` — dieci occorrenze del pattern di esaustività, censite in §3
- Decision Log: `D-278`, `D-260`, `D-301`

### ⛔ NOT RUN
- **`./scripts/rt-suite.ps1`** — non eseguita; nessuna riga di `Source/` toccata.
- **PIE / packaged** — non pertinenti: questa issue non ha oracolo visivo.
- **La consegna di `#1801`** — questo pass **non** implementa il gate: rivede la sua specifica. Nessuna
  `FindMissing…`, nessun test, nessuna tabella scritti.

---

## 9. Rischi e aperti

- ✅ **Milestone risolta**: `v0.1 · Leggibilità`, allineata al consumatore.
- ⚠️ **`D-301` non è ancora su `main`**: vive nella PR [#1948](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1948). Finché non è mergiata, chi legge `#1801` da `main`
  non trova la voce che lo rende bloccante.
- ✅ **Supporto del mapping scelto**: tabella C++ — §7.1. ⚠️ **Resta da rivedere se le cue diventano asset**:
  la scelta è corretta finché nessun artista le tocca, e quel giorno cambia il supporto, non il gate.
- ⚠️ **`U4` è una scelta di supporto che `D-278` non aveva fatto.** Se va registrata come decisione, serve un
  `D-nnn` **verificato al momento** — non è stato prenotato qui.
- ⚠️ **`HazardDamage` non è solo senza presentazione: è senza produttore.** Va deciso se è un `NoPresentation`
  legittimo, un evento mai implementato, o un valore da rimuovere. Il gate lo renderà rosso comunque — meglio
  saperlo prima di scriverlo, perché le tre risposte portano a tre lavori diversi.

---

## 10. Prossimo passo

**Decidere cosa è `HazardDamage`** — `NoPresentation` legittimo, evento mai implementato, o valore da
rimuovere. È l'unica domanda rimasta che il gate non può rispondere da sé, e diventerà rossa su di lui il
giorno in cui viene scritto.
