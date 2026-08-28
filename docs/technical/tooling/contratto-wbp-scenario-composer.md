# Contratto — Widget Scenario Composer (TD-EDITOR-01)

> **Stato**: contratto del terminale EDITOR, rimisurato su `feat/editor-docs-1155` HEAD `d0e35814`
> il 2026-08-28. La stesura precedente era del 2026-08-27 su `feat/1114-writer-json-scenario` HEAD
> `5843ea49`, quando i mutatori non esistevano ancora: §3 e §7 sono state riscritte, non ritoccate.
> **Subordinato a**: [`spec-tactical-designer.md`](spec-tactical-designer.md) §3 · [ADR-0010](../../decisions/adr-0010-esposizione-blueprint-scenario-harness.md)

Questo documento dice **cosa il widget chiama e cosa non gli è permesso decidere**. Non è un tracker e non
descrive l'aspetto: la grafica si decide guardando, il contratto no.

---

## 1. L'invariante, applicata a questo widget

`spec-tactical-designer.md` §3: *uno strumento d'authoring non è mai un'autorità di gioco.* Tradotto in
elementi concreti di questa schermata:

| Il widget mostra | Chi lo ha deciso |
|---|---|
| la lista delle unità schierate | `ListUnits()` — è una **fotografia**, modificarla non modifica niente |
| se lo scenario è valido, e perché no | `Validate()` → `URTScenarioLoader::Validate` |
| la frase accanto all'esito | `DescribeResult()` |
| se il salvataggio è andato a buon fine | `SaveToFile()` / `SaveInPlace()` |

E ciò che il widget **non** calcola, in nessun ramo: se una cella è occupata, se un'unità ci sta, se un
facing è legale, quanto costa un movimento, se lo scenario è "buono". Se una di queste risposte servisse e
non esistesse una chiamata che la dà, **la chiamata va aggiunta in C++** — non ricostruita in Blueprint.

> ⚠️ Il vincolo non regge sulla disciplina di chi monta il Blueprint: regge per costruzione. Le nove
> `USTRUCT` di `FRTTestScenario` **non** sono `BlueprintType`, quindi un `FRTTestScenario` non esiste come
> pin e non è componibile da un grafo. È la proprietà che ADR-0010 chiama impossibile, non vietata.

---

## 2. La superficie disponibile — verificata, non ricordata

`URTScenarioAuthoring` (`BlueprintType`, `UObject`) è **l'unica** classe con `UFUNCTION` in tutto
`Source/RefactorTactics/ScenarioHarness/`. Rimisurato il 2026-08-28: `grep -rln "UFUNCTION"
ScenarioHarness/` dà un file, e `grep -c UFUNCTION` su quel file dà **19**.

### Ciclo di vita

| Chiamata | Tipo | Note |
|---|---|---|
| `CreateScenarioDraft(Outer)` | `static` | il widget lo tiene come **variabile**, niente di globale |
| `NewScenario(ScenarioId, MapRadius=3)` | | non valido finché non ha unità e assertion, ed è corretto così |
| `OpenById(ScenarioId, OutError)` | | apre per **ID**, l'indice sa dove vive — il path non si compone a mano |
| `OpenFromFile(FilePath, OutError)` | | per i file che l'indice non copre |
| `Close()` | | le chiamate successive rispondono `NoScenarioOpen` |
| `IsOpen()` | `pure` | |

### Lettura

| Chiamata | Ritorna |
|---|---|
| `GetSummary()` | `FRTScenarioSummary` — 9 campi: `ScenarioId` `Version` `Tags[]` `Fixture` `MapRadius` `UnitCount` `TurnCount` `ExpectationCount` `VariantCount` |
| `ListUnits()` | `TArray<FRTScenarioUnitView>` — `Id` `HeroId` `TeamId` `Cell` `Facing` `bBotControlled` |
| `ListHeroIds()` | `static` — `TArray<FName>`, gli `HeroId` del roster |
| `ListScenarioIds(TagA, TagB)` | `static` — vuoti = elenco completo |
| `ListScenarioTags()` | `static` — per costruire il filtro senza scrivere i tag a mano |

`FRTCellId` è `USTRUCT(BlueprintType)` ed `ERTHexDirection` è `UENUM(BlueprintType)`: entrambi sono pin
legittimi, quindi cella e facing viaggiano come dati e **non** come stringhe da riparsare.

### Esiti

`ERTScenarioAuthoringResult` — cinque valori, e la distinzione conta perché il widget li mostra diversi:

```text
Success         fatto
NotFound        l'ID non esiste nell'indice, o il file che dichiara non si legge
Invalid         non passa Validate — accusa lo SCENARIO
WriteFailed     validato ma non scritto — accusa il DISCO
NoScenarioOpen  si è chiesto qualcosa a un draft senza scenario aperto
```

> 🔑 `Invalid` e `WriteFailed` non si fondono in un "errore di salvataggio". Uno si corregge cambiando lo
> scenario, l'altro chiudendo un file aperto altrove: mostrarli uguali manda l'utente a cercare nel posto
> sbagliato. `DescribeResult()` fornisce già la frase — **non riscriverla nel widget**.

---

## 3. I mutatori — consegnati con #1115, e con quattro differenze che cambiano il widget

L'API non è più read-only. I mutatori sono atterrati con
[#1115](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1115), mergiata in `main` con
[PR #1513](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1513) — che è l'`HEAD` su cui questo
documento è misurato.

| Elemento della UI | Chiamata | Stato |
|---|---|:--:|
| browser degli scenari, filtro per tag | `ListScenarioIds` · `ListScenarioTags` | ✅ |
| intestazione | `GetSummary` | ✅ |
| lista unità: Hero · Team · Facing · Cell | `ListUnits` | ✅ |
| esito e ragione | `Validate` · `DescribeResult` | ✅ |
| `[ SAVE ]` | `SaveToFile` · `SaveInPlace` | ✅ |
| tendina degli eroi | `ListHeroIds` | ✅ *(non era stata chiesta)* |
| `[ + UNIT ]` | `AddUnit` | ✅ |
| `[ REMOVE ]` | `RemoveUnit` | ✅ |
| editare Cell | `MoveUnit` | ✅ |
| editare Facing | `SetUnitFacing` | ✅ |
| **editare Team** | — | ❌ **resta scoperto** |

### La superficie reale

```cpp
ERTScenarioAuthoringResult AddUnit(const FString& UnitId, FName HeroId, int32 TeamId,
        FRTCellId Cell, ERTHexDirection Facing, FString& OutError);
ERTScenarioAuthoringResult MoveUnit(const FString& UnitId, FRTCellId Cell, FString& OutError);
ERTScenarioAuthoringResult RemoveUnit(const FString& UnitId, FString& OutError);
ERTScenarioAuthoringResult SetUnitFacing(const FString& UnitId, ERTHexDirection Facing, FString& OutError);
static TArray<FName> ListHeroIds();
```

🔴 **Questa sezione chiedeva una superficie diversa, e va riscritta sulle firme vere invece che corretta a
mente.** Diceva *«l'API consegnata è read-only + IO»* e proponeva cinque chiamate; ne sono arrivate quattro
più una non prevista, e **tre differenze su cinque cambiano il widget**, non solo il nome da digitare.

**1. `AddUnit` prende `UnitId` in ingresso, non lo restituisce.** La richiesta era
`AddUnit(HeroId, …, FString& OutUnitId, …)` — il C++ conia l'id, il widget lo riceve. È l'opposto: **conia
il widget**. Non è una sfumatura di firma, è una responsabilità che la richiesta toglieva a questa
schermata e che la firma vera le restituisce. L'header dichiara *«id già preso»* fra le ragioni di
`Invalid`, quindi la collisione è un errore che il widget **può produrre da sé**. Serve una strategia di
conio dichiarata, e va scritta qui: è l'unico documento che la vede.

**2. `MoveUnit` al posto di `SetUnitCell`.** Stessa forma, nome diverso. Non cambia niente se non che il
contratto smette di chiamarla con un nome che non esiste.

**3. `SetUnitTeam` non esiste**, ed è l'unica delle tre righe di editing che resta ❌. Oggi cambiare
squadra a un'unità schierata significa `RemoveUnit` + `AddUnit`, cioè riconiare l'id — e con esso perdere
l'identità che `FRTScenarioUnitView::Id` garantisce stabile. ⚠️ **Va deciso in C++ se sia una lacuna o una
scelta**: se il team è parte dell'identità di un'unità dentro uno scenario, l'assenza è coerente e va
dichiarata; se non lo è, manca un mutatore. Il widget non può decidere quale delle due, e non deve
aggirarla con la coppia remove/add senza che qualcuno l'abbia detto.

**4. `ListHeroIds()` in più.** Non era chiesta e serve: popola la tendina degli eroi senza scrivere i nomi
a mano. L'header spiega anche perché non è `GetHeroRoster()` — quello istanzia quattro `URTHeroData` **con
tutte le loro abilità** a ogni chiamata, e una tendina che si riapre le pagherebbe tutte per leggere
quattro nomi.

### La domanda che andava decisa in C++ ha una risposta

Era: *se `AddUnit` su una cella già occupata debba fallire subito, o passare e far cadere `Validate`.*

**Fallisce subito.** Il doc-comment di `AddUnit` elenca le ragioni di `Invalid`, e la cella occupata è fra
quelle: *«id già preso, eroe fuori catalogo, cella fuori arena, cella occupata, cella che blocca il
movimento»*. La conseguenza sul widget è quella che questa sezione prevedeva per il primo dei due casi:
**il ghost di placement può dire *invalido* prima del click**, e non serve un secondo posto in cui mostrare
l'errore dopo.

⛔ **Questo non autorizza il widget a calcolare la validità.** L'anticipo si ottiene chiamando, non
deducendo: la regola resta in C++ e §1 non si allenta di un millimetro. Ciò che cambia è *quando* si può
chiamare, non *chi* decide.

### Nota di pianificazione, non una richiesta

`URTScenarioRunner` e `FRTScenarioSession` esistono e **non** hanno `UFUNCTION` — rimisurato il 2026-08-28,
il `grep` dà ancora un solo file. **TD-EDITOR-03** (`[ RUN ]` / `[ RESET ]` / TurnLog) troverà quindi lo
stesso muro che #1115 ha appena abbattuto per l'authoring. Non è lavoro di adesso: è la ragione per cui
vale la pena che l'esecuzione passi dalla stessa porta, invece di aprirne una seconda.

---

## 4. Stati del widget

```text
NESSUN DRAFT      solo [ NEW ] e il browser sono attivi
APERTO, VALIDO    [ SAVE ] attivo, esito verde
APERTO, INVALID   [ SAVE ] attivo — l'utente ha il diritto di provarci: il disco non viene toccato;
                  esito rosso, con la frase di DescribeResult
SALVATO           esito Success; l'intestazione si ricarica da GetSummary
ERRORE DI DISCO   WriteFailed, e il testo NON dice "scenario non valido"
```

> Uno scenario nuovo **nasce invalido** — non ha unità né assertion — e questo non è un errore da
> nascondere: è lo stato di partenza. Mostrare un rosso allarmante al primo `NEW` insegnerebbe a ignorarlo.

Il widget non salva da solo. ADR-0010: *esplicito, nessuna scrittura implicita a ogni modifica*.

---

## 5. Viewport — cosa si riusa

L'audit TD-EDITOR-00 ha misurato che la presentazione graybox **esiste già** ed è procedurale:

- unità → `ARTUnit`, cilindro con root neutro (`#593`), anello di team;
- celle → `ARTHexMapActor::Cells` (ISM), più `Relief`, `Blockers`, `EdgeFeatures`, `SurfaceGlyphs`.

**Nessuna mesh nuova serve per TD-EDITOR-01.** Il ghost di placement è una variante di presentazione
dell'unità esistente, non un asset da modellare.

---

## 6. Dove vive l'asset — due candidati, come §13 prescrive

Le convenzioni §13 chiedono di **fermarsi e descrivere le due collocazioni** quando l'owner non è ovvio.
Qui non lo è, e ci sono due dubbi distinti.

**Dubbio A — la cartella.** §5 vuole `/Game/RT/UI/<funzione>/`, «divisa per funzione, non per tipo tecnico».

| Candidato | A favore | Contro |
|---|---|---|
| `UI/Scenario/` | la funzione è *lo scenario*; segue `Framework/`, che nomina la funzione | uno strumento accanto alle schermate di gioco |
| `UI/Tools/` | separa l'authoring dal gioco: si vede subito cosa non entra in una build | «Tools» è un tipo, non una funzione — è ciò che §5 chiede di non fare |

**Raccomando `UI/Scenario/`**: rispetta §5 alla lettera, e la separazione gioco/strumento è già portata dal
nome dell'asset. Entrambi sono committabili — `.gitignore:78` `!Content/RT/UI/**/*.uasset` è un glob e
copre tutta la sottocartella. Verificato il 2026-08-28: `git check-ignore -q` esce **1** su entrambi.

**Dubbio B — il prefisso.** §6 dà il formato `<Tipo>_<Feature>_<Nome>` con l'esempio `WBP_Planning_ActionBar`
— **senza** `RT`. Ma tutti e otto i widget esistenti sono `WBP_RT_*` (`WBP_RT_MainMenu`, `WBP_RT_ErrorModal`, …).
La convenzione scritta e la pratica divergono, e non è un dettaglio: gli otto path dei personaggi sono
elencati **per esteso** nell'allowlist, e §5b avverte che un rename li rende muti senza che nulla lo segnali.

Qui il rischio non morde — la riga è un glob, non un elenco — quindi il costo di scegliere è basso e il
costo di sbagliare pure. **Raccomando `WBP_RT_ScenarioComposer`**, per coerenza con gli otto asset reali:
una convenzione che nessun asset segue è la convenzione da correggere, non da applicare all'unico nuovo.

Percorso proposto: `/Game/RT/UI/Scenario/WBP_RT_ScenarioComposer`

> ⚠️ **Restano due dubbi aperti, non due decisioni prese.** §13 chiede di descriverli e fermarsi, ed è ciò
> che questa sezione fa: le raccomandazioni sono raccomandazioni. Diventano decisioni quando qualcuno le
> accetta, e il posto dove si registrano è `OPEN_DECISIONS.md`, non questo file.

---

## 7. Verifica

Il DoD di TD-EDITOR-01 chiede PIE: *piazza almeno due unità, salva, riapri, verifica Cell / Facing / ID*.

✅ **I mutatori esistono da #1115**: `AddUnit`, `MoveUnit`, `SetUnitFacing`, `RemoveUnit`. ⛔ **Ma il
percorso non è ancora eseguibile**, per l'altra metà della precondizione: manca il widget. *Questa riga
diceva «il percorso è eseguibile» — vero dei mutatori, falso del percorso — e la si leggeva come via
libera a schedularlo. Corretta il 2026-08-28.*
🔴 **Questa sezione diceva «non è eseguibile finché i mutatori non esistono»**: vero fino al 2026-08-27,
falso dal merge di [PR #1513](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1513).

✅ **Il percorso ha una voce dal 2026-08-28**: `PIE-SCEN-COMPOSER` in
[`test-manuali-pie.md`](../test-manuali-pie.md). *Questa riga diceva «non ha ancora una voce», ed era
vera fino a quel giorno.* Nasce ⏳ e non eseguibile — l'asset non esiste — e **nessuna seduta la
rivendica**: quello resta da fare, in [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml), perché
una verifica PIE che non appartiene a una seduta tende a non essere mai eseguita
(`spec-tactical-designer.md` §9).

⚠️ **Un passo in più rispetto al DoD, e nasce dalla differenza 1.** Siccome è il widget a coniare `UnitId`,
la verifica deve includere **due unità aggiunte di seguito** e il controllo che gli id siano distinti e
sopravvivano al round-trip. Il DoD dice già *«verifica … ID»*, ma lo diceva quando l'ID era un dato che
**arrivava**: ora è un dato che la schermata **produce**, e un conio che collide si manifesta come
`Invalid` con *«id già preso»* — cioè un messaggio che accusa lo scenario per un difetto del widget.

Il round-trip di per sé è già coperto in Automation da `RTScenarioWriterTests` e `RTScenarioAuthoringTests`:
la voce PIE non ripete quello, verifica che **la schermata** faccia arrivare i dati giusti al writer.
