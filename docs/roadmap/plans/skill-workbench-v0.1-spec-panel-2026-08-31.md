# Skill Workbench `v0.1 → v1.0` — spec panel contro il repository

> `CURRENT` · **Referto di revisione**, non owner. Sottopone a panel il kit *REFACTORTACTICS — SKILL
> WORKBENCH · Roadmap operativa v0.1 → v1.0 + piano issue per Claude Code* (2026-08-31, 22 sezioni,
> `SW0-00 … SW0-11` + `SW-E1 … SW-E9`).
>
> **Data**: 2026-08-31 · **Modo**: discussion · **Focus**: requirements + architecture + testing
> **Base**: `main` @ `8bfe8f84` = `origin/main` `8bfe8f84` (0 avanti / 0 dietro) · albero pulito salvo
> due referti untracked in `docs/roadmap/plans/`.
>
> **Cosa è**: il verdetto sul mandato. **Non esegue il lavoro che ordina**: nessun documento owner
> riscritto, nessuna riga di `Source/` toccata, nessun `.uasset` aperto, nessun commit.
>
> ✅ **Tracking eseguito dopo conferma esplicita**: tre issue create e agganciate alla gerarchia nativa —
> [#1950](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1950) (parent `TD 0.3`, sotto
> #1105), [#1951](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1951) (il breakdown a
> nove stadi) e [#1953](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1953) (il readout
> numerico), entrambe sotto #1950. **Nessuna delle dodici candidate `SW0-*` è stata aperta come tale**: la
> matrice del §6 le classifica, e le tre issue nascono da ciò che la classificazione ha lasciato in piedi.
>
> **Cosa non è**: un'autorità. Se una riga qui diverge da
> [`../../technical/tooling/spec-tactical-designer.md`](../../technical/tooling/spec-tactical-designer.md),
> dal corpo di [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105) o dal
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md), **ha ragione l'owner**.

---

## 1. Il verdetto in una riga

> **Il kit ha scelto il bersaglio giusto — `TD 0.3` è l'unico gap che il referto di stamattina ha
> confermato scoperto — e ci costruisce sopra un exit gate che il repository non può soddisfare: il
> modifier system su cui poggiano `SW0-03`, `SW0-07` e una riga del gate v0.1 non esiste, e la regola
> «se non esiste, STOP» è del kit stesso.**

> 🔵 **CORRETTO il 2026-08-31, prima di aprire qualunque issue. La seconda metà di quella riga è falsa,
> e il §11 di questo stesso referto aveva dichiarato il rischio che l'ha prodotta.** *«Non esiste un
> modifier system»* era vero del **nome** e falso della **sostanza**: un ordine canonico di risoluzione
> esiste, ha **nove stadi**, un solo punto di orchestrazione
> ([`RTTurnManager.cpp:4660`](../../../Source/RefactorTactics/Turn/RTTurnManager.cpp)), una decisione per
> ciascuno stadio e test guardiani. La ricerca per `Modifier`/`Stack`/`Apply` non poteva vederlo, perché
> il repository non lo chiama così — è esattamente l'assenza-non-dimostrabile che il §11 dichiarava.
>
> ⛔ **`SW0-03` non è quindi uno `STOP`.** La sua risposta è al **§10**, che è il lavoro che questa
> sessione ha svolto. Il verdetto sul kit non cambia di segno — il gate resta da riscrivere, perché il
> kit chiede di *costruire* un sistema che va invece *letto* — ma cambia di natura: da bloccante a
> realizzabile in `v0.1`.

A differenza del kit *TD Skill / Action Lab* consumato stamattina
([referto](td-skill-actionlab-roadmap-spec-panel-2026-08-31.md), `CREATE = 0`), questo non ridescrive
lavoro consegnato. Il §9 C di quel referto misura il gap che questo kit attacca:

```text
C. MISSING CAPABILITIES
   - Skill Workbench (TD 0.3): manca il DATO, non la UI — nessun override di abilita'
     in una variante. E' il vincolo dichiarato dal corpo di #1105.
```

E l'ordine delle sue fasi — `PHASE A` contratto/dato, poi `PHASE C` UI — è **la stessa priorità**
dell'owner (*«prima esiste il dato che la UI compila»*). Il kit non sbaglia dove guarda. Sbaglia **quattro
fatti** sul repository, e ognuno dei quattro, eseguito alla lettera, produce un danno misurabile.

---

## 2. Base di misura

| Misura | Comando | Esito |
|---|---|---|
| `HEAD` / branch | `git rev-parse HEAD` · `git branch --show-current` | `8bfe8f84` · `main` |
| `origin/main` | `git log -1 origin/main` | `8bfe8f84` — 0 avanti / 0 dietro |
| Engine | `*.uproject` → `EngineAssociation` | **`5.8`** (CLAUDE.md pinna `5.8.1`: il file dichiara la famiglia, non la patch) |
| Modifier system col **nome** `Modifier` | `rg "ApplyModifier\|ModifierStack\|FRTModifier" Source Plugins` | **0 occorrenze** |
| Ordine canonico di risoluzione | `RTTurnManager.cpp:4660` + §10 | 🔵 **esiste — nove stadi**, un solo punto di orchestrazione, una decisione per stadio |
| `MoveCostModifier` | `RTHexSim.h:43` | costo di **una cella**, non un modificatore d'abilità |
| `FRTAbilityVariant` | `Ability/RTActionData.h:19` | **esiste** — `VariantId`, `Effects`, `Parameters` (`TMap<FName,int32>`) |
| `FRTScenarioVariant` | `ScenarioHarness/RTTestScenario.h:486` | esiste, **solo celle**, limitazione deliberata |
| Ability come asset? | `rg "NewObject<URTActionData>" Source` | costruite **in codice**; catalogo = `URTCatalogLibrary::GetCoreActionCatalog()` → `TArray<FRTActionDef>` |
| Doppia casa numerica | `RTControlActionTests.cpp:131,892` | `URTActionData::CooldownTurns` è uno **«specchio legacy»** di `Def.CooldownTurns`, e `ConsumeAbility` legge **il primo** |
| Invariante interi | `RTActionDef.h:281` | `RefactorTactics.Catalog.NoFloatInIntegerFields`, verificato per reflection |
| `Requires` → `Blocked` | `RTTestScenario.h:37-49` | `ERTTestOutcome::{Pass,Fail,Error,Blocked}` — **esiste già** |
| `RulesVersion` / `ContentHash` | `rg "ContentHash\|RulesVersion\|ContentVersion" Source` | **1 occorrenza**, un commento in `RTGoldenCorpusTests.cpp:39` che dice che *sarebbe* il loro mestiere |
| `StateHash` | `Turn/RTMatchStateHash.h` | esiste — checksum di **stato finale**, consumato da Runner/Session/Draft |

Stato issue, letto lato server il 2026-08-31:

| Issue | Stato | Rilievo |
|---:|---|---|
| #1105 | OPEN | epic parent, senza numero `E` (`out_of_release_scope`) |
| #776 | OPEN | `E43` — owner del confronto a lotti |
| #1678 | OPEN | DevSandbox Launcher (parent `P2`, quattro figli su sei chiusi) |
| #1631 | OPEN | il guardiano `ADR-0010` copre otto struct su nove: manca `FRTScenarioVariantUnit` |
| #1626 · #1629 | OPEN | authoring intent · status iniziali |
| `Skill Workbench in:title` | — | **nessuna issue esiste** |

---

## 3. Il panel

### 🔨 WIEGERS — qualità dei requisiti

🔴 **CRITICO — l'exit gate v0.1 contiene una riga insoddisfacibile.** Il **§11 del kit** chiede
*«almeno un modifier supportato può essere testato»*. Misurato: non esiste un modifier system d'abilità.
L'unico punto in cui un valore d'abilità viene stratificato è `EffectiveAttackPower(BasePower,
OccupantDamageBonus)` — **un solo addendo, applicato dal chiamante**, senza ordine, senza sorgente, senza
stack. `MoveCostModifier` appartiene al costo di una cella e non a un'abilità.

Il kit prevede questo caso e prescrive la risposta giusta (`SW0-03`: *«Se NON esiste: STOP. Aprire
decisione tecnica / ADR / issue owner»*), poi **scrive un gate che presuppone il contrario**. Un gate
insoddisfacibile è peggio di un gate assente: la `v0.1` non può mai essere dichiarata Done, e chi ci
lavora sarà spinto a inventare nell'Editor i cinque tipi (`Flat`, `Percent`, `Multiplier`, `Override`,
`Clamp`) che il §7 vieta di inventare.
📝 **Raccomandazione**: `SW0-03` e `SW0-07` escono dalla `v0.1` e diventano **una issue di decisione**
(esiste un ordine canonico dei modificatori? serve?). Il gate perde la riga sul modifier. Ciò che resta —
override numerico, readout, run, confronto — è realizzabile **oggi**.

> 🔵 **CORRETTO — la domanda è stata risolta invece che aperta, e la risposta è «sì, esiste».** Vedi
> **§10**. Il rilievo di WIEGERS regge nella sua parte che conta — *«il kit prescrive di non inventare
> operazioni nell'Editor e poi scrive un gate che ne presuppone un sistema»* — ma la conseguenza si
> ribalta: la riga del gate **è soddisfacibile**, perché i «modifier supportati» sono i nove stadi che
> già esistono. Ciò che va corretto nel kit non è il gate: è il verbo. `SW0-03` dice *«add deterministic
> modifier breakdown»*; il lavoro reale è **registrare** un breakdown che la pipeline già calcola e
> scarta.

⚠️ **MAGGIORE — tre criteri non sono verificabili come scritti.**
`«Playback riusato se disponibile»` (SW0-08) è un condizionale che nessuna automation decide;
`«la classificazione può essere conservativa»` (SW0-09) non è un criterio;
`«L_DevSandbox non dirty»` compare tre volte come Automation ma è, oggi, una verifica **d'occhio in
Editor** — il registro giusto è [`test-manuali-pie.md`](../../technical/test-manuali-pie.md), non una riga
di `Automation`.

### ⚔️ FOWLER — confini e nomi

🔴 **CRITICO — `Variant` nel dominio ability è già occupato, ed è la terza omonimia.**
`FRTAbilityVariant` esiste in `Source/RefactorTactics/Ability/RTActionData.h:19`: `VariantId`,
`DisplayName`, `Tradeoff`, `Effects`, `Parameters`. È una variante **di loadout** — *«un compromesso
ORIZZONTALE: cambia COME si usa l'abilità, non QUANTO è forte»* — quindi non è il dato che `SW0-01`
cerca, ma **occupa il nome nello stesso dominio**.

Il kit avverte su `FRTScenarioVariant` (giusto) e su `Candidate` (giusto, e `D-154` lo aveva già
misurato), e **manca il terzo**, che è quello più vicino a ciò che vuole costruire. È esattamente la
classe di difetto che `D-154` documenta: un identificatore condiviso a cui si aggiunge un significato
senza misurare chi lo usa già.
📝 **Raccomandazione**: `SW0-01` non può scegliere un nome prima di dichiarare come convive con
`FRTAbilityVariant`. Le tre opzioni del kit (A/B/C) diventano quattro, e la quarta è la più economica —
vedi §5.

⚠️ **MAGGIORE — `SW-E1 … SW-E9` è una seconda scala di maturità sopra una che esiste.**
`spec-tactical-designer.md` §6 e il corpo di `#1105`, decisi da `D-154`, definiscono `TD 0.3` (Skill
Workbench), `TD 0.4` (variante su più scenari, diff baseline↔variante), `TD 0.7` (confronto a lotti →
**E43**), `TD 0.9` (promozione con gate). `SW-E2`, `SW-E4`, `SW-E7`, `SW-E8` sono **le stesse capability
rinumerate**. Il §7 del kit vieta *«due owner per lo stesso outcome»*; il §13 ne crea uno.

> È la **stessa forma** dell'errore che il kit *TD Skill / Action Lab* ha commesso stamattina con
> `TD 0.1 … TD 1.0`, e che il referto ha dichiarato ⛔ **non recepito**. Cambia il prefisso, non il difetto.

📝 **Raccomandazione**: eliminare `SW-E*`. Gli stadi si citano come `TD 0.3`, `TD 0.4`, `TD 0.7`, `TD 0.9`.

✅ **Il §4 e il §22 sono corretti e non aggiungono nulla.** L'invariante *«il Workbench è una lente, non un
secondo simulatore»* è `ADR-0010` più il §3 dell'owner, riga per riga. È la parte citabile senza
verificarla.

### 🕸️ NYGARD — stato, provenance, modi di fallimento

🔴 **CRITICO — «canonical numeric readout» non ha una sorgente canonica: ce ne sono due, e divergono.**
`URTActionData` porta `Power`, `RangeCells`, `CooldownTurns` **e** un `FRTActionDef Def` che porta gli
stessi nomi. I test lo dicono per esteso:

> *«`ConsumeAbility` legge `URTActionData::CooldownTurns`, **non** `Def.CooldownTurns`, ed è l'unico
> specchio legacy che la fixture NON allinea — `RangeCells`, `Power` e `bSelfTarget` li copia dal `Def`.»*
> — `RTControlActionTests.cpp:131`

`SW0-02` non dice quale leggere. Se legge `Def`, il Workbench mostra un cooldown **che il gioco non
consuma**: è precisamente la divergenza editor/runtime che il §22 dichiara motivo di arresto, e nasce al
primo giorno di lavoro senza che niente diventi rosso.
📝 **Raccomandazione**: `SW0-02` non è «esporre i numeri». È **esporre i numeri che il resolver legge**, e
il suo primo test non è `same ability → same ordered readout`, è: *il readout e il consumatore leggono lo
stesso campo*. Quel test chiude il difetto invece di documentarlo.

⚠️ **MAGGIORE — la provenance di `SW0-04` cita due campi che non esistono.** `RulesVersion` e
`ContentVersion/Hash` hanno **una** occorrenza in `Source/`: un commento che dice che *sarebbe* il loro
mestiere. Esiste `FRTMatchStateHash`/`FRTUnitStateDigest`, ma è un checksum di **stato finale** — risponde
a *«le due partite sono finite uguali?»*, non a *«le due run hanno usato lo stesso contenuto?»*. La riga
*«NON creare un secondo sistema di hash se il repository ne possiede già uno»* è quindi ambigua: il
repository ne possiede uno che risponde a un'altra domanda.
📝 **Raccomandazione**: la provenance `v0.1` porta ciò che esiste — `ScenarioId`, `AbilityId`, `VariantId`,
`StateHash` — e **dichiara assenti** `RulesVersion` e `ContentHash`. Un campo assente e dichiarato è un
gap; un campo assente e presupposto è un difetto silenzioso.

⚠️ **Il guardiano di `ADR-0010` ha un buco noto proprio dove `SW0-01` atterrerebbe.** Se la scelta è
estendere `FRTScenarioVariant`, va letta **#1631**: `MustStayInternal` copre otto struct su nove e
`FRTScenarioVariantUnit` resta verde. Aggiungere struct alla famiglia delle varianti **senza** chiudere
#1631 allarga il buco.

### 🧪 CRISPIN — testabilità e riuso

⚠️ **MAGGIORE — `SW0-08` chiede di costruire un esito che esiste.** *«UnsupportedScenarioIsBlockedOrFails
Explicitly»*: il formato ha già `Requires` per turno → `ERTTestOutcome::Blocked`, distinto da `Fail`
(difetto del gioco) e da `Error` (difetto del test), e documentato come *«non è un difetto di nessuno: è
il progetto che non c'è ancora»*. Non va costruito, va **usato** — ed è anche il modo corretto di far
convivere uno scenario col modifier assente.

✅ **`Errore 5` ha ragione e ha già un guardiano.** *«Usare float perché comodi nell'Editor: NO»* è
l'invariante #4 del catalogo, verificata per reflection da
`RefactorTactics.Catalog.NoFloatInIntegerFields`. Il kit non deve introdurre la regola: deve citarla.

### 📚 ADZIC — esempi eseguibili

⚠️ Il kit dichiara i suoi esempi concettuali (*«Damage 40 → 45»*, *«×1.20 Character Buff»*, *«×1.50 Wet
Target»*) e fa bene, ma **non si àncora mai a un'abilità reale**. Il repository ne ha una con percorso
completo fino al TurnLog, misurata stamattina: `Hero.Gadget.ArcPulse`, **22 danni / range 4**,
34 occorrenze in 21 scenari, con baseline `Scenarios/Combat/BasicAttack.json`.
📝 **Raccomandazione**: `SW0-06` e `SW0-08` si scrivono su quell'abilità e su quello scenario. Un esempio
falsificabile costa una riga e rende l'acceptance verificabile prima di scrivere codice.

### 🧭 COCKBURN — attore e obiettivo

✅ Il §1 è la parte migliore del kit: l'attore è dichiarato (technical/gameplay designer), l'obiettivo è un
**ciclo** e non una somma di funzioni, e le cinque domande che apre (*«cosa succede se Damage passa
40 → 45?»*) sono il criterio di successo esterno allo strumento. È la stessa forma del §6.1 dell'owner.

⚠️ Lo scenario principale però **non nomina il fallimento**: cosa vede il designer quando l'abilità che ha
scelto non ha parametri sovrascrivibili, o quando lo scenario non la esercita? `Blocked` esiste per
questo, e il kit non lo collega.

---

## 4. Il guardrail che protegge un rischio quasi inesistente

Il kit ripete in cinque punti *«non modificare production asset»* (`Errore 3`, la DoD §17.6, `SW0-01`,
`SW0-10`, il gate `v0.1`). Misurato: **le abilità non sono Data Asset**. Nascono da
`NewObject<URTActionData>` in `RTCatalogLibrary.cpp`, `RTHeroCatalogLibrary.cpp` e nei test, e il catalogo
è `URTCatalogLibrary::GetCoreActionCatalog()`, che restituisce un `TArray<FRTActionDef>` scritto in C++.

Non c'è un `.uasset` da sporcare. Il dato di produzione **è codice**, e una Variant non può raggiungerlo
per errore — può raggiungerlo solo con una PR.

⚠️ **E il rischio reale non è nominato**: le istanze `URTActionData` sono condivise e transienti dentro una
run. Una Variant che scrivesse sull'istanza invece che su un contesto laterale muterebbe l'abilità **per
tutte le unità della stessa run**, e il test `VariantDoesNotMutateCanonicalAbility` — scritto contro
l'asset — resterebbe verde.
📝 **Raccomandazione**: il test da scrivere è *due run consecutive nella stessa sessione, la seconda senza
Variant, danno il risultato baseline*. Quello morde; quello sull'asset no.

---

## 5. La quarta opzione per `SW0-01`, che il kit non elenca

Il kit propone tre strade (estendere `FRTScenarioVariant` · creare un Workbench Run Context · usare un
seam esistente) e avverte di non scegliere la prima automaticamente. L'avvertenza è giusta e **la risposta
è già scritta nell'owner**, §5, nella tabella *«ciò che il formato non esprime»*:

| Manca | Serve a | Innesco |
|---|---|---|
| override di abilità in una variante | *baseline vs variante* | **lo Skill Workbench** |

Non è un'apertura generica: è un'estensione **pre-registrata**, con il suo innesco nominato, nello stesso
documento che vieta di duplicare il formato (`D-154` punto 4: *«il formato scenario non si duplica: si
estende»*). La strada `A` non è la scorciatoia da temere — è il piano dichiarato.

Restano due vincoli, entrambi misurati:

1. `FRTScenarioVariant` è **deliberatamente** solo-celle, e il suo commento spiega il costo di allargarla:
   *«il giorno in cui servisse variare altro, lo si aggiunge sapendo che si sta allargando ciò che il
   canary può attribuire»*. Allargarla è autorizzato **a condizione di dichiarare quella perdita**.
2. #1631 va chiusa prima o insieme, o il guardiano `ADR-0010` copre nove struct su dieci invece di otto
   su nove.

---

## 6. Matrice delle issue candidate

Applicato il criterio del kit stesso (§7): *overlap 70–100% → REUSE oppure UPDATE.*

| Candidate | Owner esistente | Classificazione | Ragione |
|---|---|---|---|
| **SW0-00** audit | `D-154` · owner §6 · questo referto | **PARTIAL_OVERLAP** | metà è misurata qui; ciò che resta è la sola decisione sul modifier |
| **SW0-01** contract Variant | `FRTAbilityVariant` · `FRTScenarioVariant` · owner §5 · #1631 | **UPDATE** | l'estensione è pre-registrata; il contratto va scritto **contro** i due omonimi, non accanto |
| **SW0-02** numeric readout | `URTActionData` + `FRTActionDef` + `Def.Effects[]` | **CREATE** → **#1953** | vero gap, riscritta: la materia sono **tre** case, il ternario di autorità e un guardiano inesistente (§8.1) |
| **SW0-03** modifier breakdown | la pipeline a nove stadi, `RTTurnManager.cpp:4660` (**§10**) | **UPDATE** 🔵 | ~~STOP~~ — l'ordine esiste: la issue cambia verbo, da *«aggiungere»* a *«registrare»* |
| **SW0-04** Variant → Harness | `URTScenarioAuthoring` · `RTScenarioRunner` · `RTMatchStateHash` | **PARTIAL_OVERLAP** | il percorso esiste; nuovo è solo l'ingresso della Variant. Provenance da ridefinire |
| **SW0-05** entry point | **#1678** (+ #1682 chiusa, #1683 aperta) | **UPDATE / LINK** | il launcher da `L_DevSandbox` è già la sua materia |
| **SW0-06** editor numerico | — | **CREATE** | vero gap, dipende da SW0-02 corretta |
| **SW0-07** modifier stack | idem | **UPDATE**, sola lettura 🔵 | ~~fuori dalla v0.1~~ — l'**editor** dei modifier esce; il **display** del breakdown resta (§10.3) |
| **SW0-08** single skill run | `#1117` (Run/Reset/Result/TurnLog) · `Requires`→`Blocked` | **PARTIAL_OVERLAP** | il run canonico è consegnato; nuovo è il binding della Variant |
| **SW0-09** A/B | `RTMatchStateHash` · TurnLog | **CREATE** — ma è **`TD 0.4`** | l'owner la colloca allo stadio successivo, non in `TD 0.3` |
| **SW0-10** profili | — | **CREATE**, `P1`, tagliabile | nessun owner; non blocca il gate |
| **SW0-11** gates | — | **UPDATE** | il gate va riscritto senza la riga sul modifier |

**Sintesi** *(aggiornata dopo il §10)*: `CREATE` reali = **3** (`SW0-02` riscritta, `SW0-06`, `SW0-10`).
`UPDATE/LINK` = **6** (`SW0-01`, `SW0-03`, `SW0-05`, `SW0-07`, `SW0-11`, `SW0-00`). `PARTIAL_OVERLAP` = 3.
Fuori scope `v0.1` = **1** (`SW0-09` → `TD 0.4`) più l'**editor** dei modifier, non il loro display.

> 🔵 La riga precedente diceva *«+ 1 decisione (`SW0-03`)»* e *«fuori scope = 2»*. Risolto al §10: la
> decisione non serviva, serviva una misura.

---

## 7. La parent issue

Il kit propone una parent sotto `#1105`. **Accettabile**, ed è la forma che `#1678` usa già. Due vincoli:

1. il titolo cita lo stadio esistente — *«`TD 0.3` — Skill Workbench: la variante d'abilità che non tocca
   il dato di produzione»* — e **non** apre `SW-E##`;
2. l'outcome è la riga `TD 0.3` dell'owner, non una riscrittura: *«configurare una skill variante senza
   toccare il dato di produzione, e provarla sulla mappa con le regole runtime»*.

Tracking: parent `#1105` · checkpoint `M9.4` · `out_of_release_scope` · lotti a `#776` (**dopo** il
competence gate di `D-102`, come `D-154` dichiara esplicitamente).

⚠️ **`RT-FEAT-TOOL-SKILL-WORKBENCH` non è più citabile**: `D-181` ha eliminato `feature-registry.yaml` il
2026-08-21. Un corpo di issue che lo citasse punterebbe a un registro che non esiste.

---

## 8. Prima issue implementabile senza blocchi

> **`SW0-02`, riscritta: «il readout espone il campo che il resolver legge, e un test lo dimostra».**

Perché questa e non `SW0-05`: il launcher è già coperto da `#1678` e la sua catena avanza da sola
(`#1683`). Perché non `SW0-01`: il contratto della Variant non si può scrivere prima di sapere **su quale
campo** l'override atterra — ed è la domanda che la doppia sorgente lascia aperta.

È implementabile oggi, non dipende dal modifier, non tocca l'Editor, e chiude con un test il difetto che
oggi vive in un commento di test.

### 8.1 Aperta come #1953 — ed è peggio di come il §3 la descriveva

Misurando per scriverla sono emersi **tre fatti** che la formulazione «due sorgenti che divergono» non
copriva:

1. **Le case sono tre, non due.** Il danno **non ha un campo nel `Def`**: vive in `Def.Effects[]` come
   `FRTActionEffectSpec{Damage, Amount}`, e `URTActionData::Power` ne è una **proiezione del primo**
   (`Power = 0; for (Spec : Effects) if (Damage) { Power = Spec.Amount; break; }`, identico in tre punti).
   Un'azione con due effetti `Damage` ne proietta uno solo. Un Workbench che offra di «cambiare il danno»
   sta editando `Effects[i].Amount`, e va detto invece che scoperto.
2. **La regola di autorità esiste ed è un ternario ricopiato a mano**:
   `Def.ActionId.IsNone() ? specchio : Def`, in `RTPlayerController.cpp:1376`, `RTTurnManager.cpp:1124` e
   `:3725`. **Il bot non lo applica** (`RTTurnManager.cpp:1271`, `:1329` leggono
   `Ability->RangeCells, Ability->Power` diretti) e **nemmeno `ConsumeAbility`**. Lo stesso parametro ha
   autorità diversa a seconda di chi lo legge.
3. 🔴 **Il guardiano che copre gli eroi è citato e non esiste.**
   `RTActionMirrorFieldsTests.cpp:35` giustifica la propria esclusione delle azioni d'eroe con
   *«hanno già un guardiano: `RefactorTactics.Catalog.HeroKitsMatchTheirCatalogDef`»*. Misurato:
   **una sola occorrenza in tutto `Source/`, quel commento**. `RTCatalogTests.cpp` ha dodici test e nessuno
   è quello; nessun test con `Hero` nel nome confronta `Def` e specchio.

   Ciò che copre davvero il file, per le venti abilità del roster, è **un verso solo**:
   `UnitKitCarriesNoUndeclaredDamage` verifica che il catalogo **muto** produca uno specchio muto, e fa
   `continue` quando il catalogo **dichiara** — cioè non verifica mai che il `22` dichiarato arrivi allo
   specchio. E `UnitPaysTheDeclaredCooldown` cerca **apposta** fra le generiche, con il commento
   *«il loro cooldown lo copia `MakeHeroAction`, che lo fa correttamente»*: assunto, non asserito.

> ⚠️ **È la stessa forma di difetto che questo referto ha già commesso al §1** — dare per esistente ciò che
> un testo afferma — e la stessa che il repository ha registrato altrove: un nome di test citato in prosa
> non è un test. La differenza è che qui il commento **spegne la ricerca**: chi lo legge smette di cercare
> il guardiano, perché gli è stato detto che c'è.

`D-090` dà la misura del prezzo: scrivere solo su `->Def` produsse *«una `Weapon.Impact` che fa pianificare
al bot un attacco che il resolver poi rifiuta — il **pulsante finto**»*. `EquipWeaponVariant` è il lato
**scrittura** di quel ponte ed esiste; #1953 è il lato **lettura** dello stesso ponte.

---

## 9. Cosa del kit sopravvive

✅ **Il §4 e il §22** — l'invariante lente/simulatore e la lista dei divieti: coincidono con `ADR-0010`, il
§3 dell'owner e il *«cosa NON autorizza»* di `D-154`. Zero contenuto nuovo, zero errori.

✅ **La scelta del bersaglio.** `TD 0.3` è l'unico gap che il referto di stamattina ha confermato scoperto,
e le fasi `A → C` rispettano *«prima esiste il dato che la UI compila»*.

✅ **Il §16 e il §17** — una issue = una capability = una verifica, e la DoD in dieci punti. È la
disciplina che ha permesso a questo consumo di classificare invece di aprire dodici issue.

✅ **`Errore 5`, `Errore 7`, `Errore 8`** — interi anziché float, lotti a `#776`, nessun secondo catalogo:
tutti e tre veri, tutti e tre già presidiati.

⛔ **Non recepito**: la scala `SW-E1 … SW-E9`; la riga sul modifier nell'exit gate `v0.1`; `SW0-03` e
`SW0-07` dentro la `v0.1`; la provenance con `RulesVersion`/`ContentHash`; `SW0-09` come stadio `0.1`;
`SW0-01` senza `FRTAbilityVariant` nel contratto.

---

## 10. `SW0-03` risolta: l'ordine canonico esiste, e ha nove stadi

> Misurato su `8bfe8f84`. La domanda del kit era: *«prima controllare se il runtime possiede già un ordine
> canonico. Se esiste: riutilizzarlo. Se NON esiste: STOP»*. **Esiste.**

### 10.1 La pipeline

Un valore d'abilità non viene trasformato in un posto solo, ma la sequenza è **unica, ordinata e
motivata**. Il punto di orchestrazione è uno: `ARTTurnManager`, riga **4660**.

```cpp
// L'assorbimento della Guardia viene per ULTIMO, dopo i modificatori del danno ([D-292]): il pool copre
// cio' che resta, non il danno nominale. […] se assorbisse per primo, `Status.Exposed` ne mangerebbe una
// parte prima che il difensore la usi.
TArray<FRTAttack> Attacks = URTCombatResolver::ApplyAbsorptionPool(
    URTCombatResolver::ApplyDamageDelta(
        URTCombatResolver::ApplyFirstHitDelta(URTHexCombatLibrary::ToAttacks(Plan), FirstHitDelta),
        EveryHitDelta),
    GuardPool, bFrontalHit);
```

| # | Stadio | Operazione | Owner | Perché lì |
|---:|---|---|---|---|
| 1 | Valore di catalogo | — | `URTActionData::Power` · `FRTActionDef` | invariante #4: **solo interi**, `NoFloatInIntegerFields` |
| 2 | Bonus di cella dell'attaccante | `+` | `EffectiveAttackPower` · `RTTurnManager_Blast.cpp:675,750` | il terreno di chi tira |
| 3 | Bonus condizionale di catalogo | `+` | `RTTurnManager.cpp:4535` (`Wet` × `Hero.Gadget.LinearDischarge`) | **limite dichiarato CP 8.2**: è l'unico, e il commento dice che il secondo andrà a catalogo, non in un secondo `if` |
| 4 | Copertura, per-colpo e direzionale | `− … clamp 0` | `CollectHexAttacks` · `RTHexCombatLibrary.cpp:444` | dipende da **dove sta chi subisce**, non dall'intento: due bersagli della stessa azione sono riparati diversamente (`D-206`) |
| 5 | Delta di **primo colpo** per bersaglio | `± … clamp 0` | `ApplyFirstHitDelta` (`Status.Exposed` `+5`, `Deflect` `−20`) | vale una volta sola; il totale non dipende da quale colpo se lo prenda |
| 6 | Delta su **ogni colpo** | `± … clamp 0` | `ApplyDamageDelta` (`Action.Brace` `−10`) | senza il gate «una volta sola»: `Brace` su `ApplyFirstHitDelta` proteggerebbe da un colpo e lascerebbe passare gli altri |
| 7 | **Pool** di assorbimento, eleggibilità frontale | pool, `Min` | `ApplyAbsorptionPool` (`Status.Guarded`, 15) | `D-292` + `D-206`: il pool è **commutativo per costruzione**, il delta negativo no |
| 8 | Somma per bersaglio sullo stato iniziale | `Σ` | `ResolveAttacks` | invariante #3: l'ordine degli attacchi non decide nulla |
| 9 | Scudo temporaneo → scudo base → HP | assorbimento a due strati | `ApplyDamage` · `RTCombatLibrary.cpp:6-33` | `D-224`: la base ferma **solo** il danno diretto; il temporaneo assorbe per primo perché sta per scadere |

Le operazioni reali sono dunque **cinque**: somma, sottrazione con `clamp` a `0`, pool con avanzo,
somma per bersaglio, assorbimento a due strati. I cinque tipi che il kit teme di veder inventare
nell'Editor — `Flat`, `Percent`, `Multiplier`, `Override`, `Clamp` — **non descrivono questo sistema**:
non esiste un solo moltiplicatore in tutta la catena, e la `v0.1` è a interi per invariante.

### 10.2 Cosa manca davvero

Non l'ordine: **il registro**. Ogni stadio calcola e scarta. Il `TurnLog` porta `Amount` — documentato come
*«danno effettivo»*, cioè lo stadio 9 — più `ActionId` e `BaseActionId`. Fra lo stadio 1 e lo stadio 9 non
resta traccia di **quale** stadio ha tolto quanto.

⚠️ **Una sola eccezione, ed è la prova che la forma funziona**: `FRTHexAttackHit::CoverBypassedByFacing`
esiste già e registra *«quanto la direzione ha annullato»* dello stadio 4 — un valore intermedio,
conservato perché qualcuno doveva spiegarlo. È il precedente su cui il breakdown si modella.

### 10.3 Conseguenze per il Workbench

🔵 **La Variant tocca solo lo stadio 1.** Gli stadi 2-9 sono funzione dello **stato di gioco** — chi tira,
da dove, con quale status, contro quale copertura — non della definizione d'abilità. Questo scioglie
l'ambiguità di `SW0-01` senza un sistema nuovo: la superficie sovrascrivibile è il valore di catalogo, e il
breakdown è il racconto di come gli altri otto stadi lo hanno trasformato **in quella run**.

∴ tre correzioni al kit:

1. **`SW0-03` cambia verbo.** Da *«add deterministic modifier breakdown»* a *«registrare il breakdown che
   la pipeline già calcola»*. Nessuna operazione nuova, nessun ordine nuovo: gli `Step[]` che il kit
   descrive (`SourceId`, `Operation`, `Operand`, `Before`, `After`) sono **esattamente** i nove stadi, e
   `SourceId` esiste già come decisione (`D-292`, `D-206`, `D-224`, CP 5.2, CP 8.2).
2. **`SW0-07` si dimezza, e la metà che resta è quella utile.** Il *display* del breakdown entra in `v0.1`.
   L'*editor* dei modifier esce — non per scope, ma perché **non ha oggetto**: non si può «aggiungere un
   modifier sperimentale» a stadi che dipendono dallo stato, e i due stadi configurabili (3 e 4) sono
   proprietà del catalogo e del terreno, non della Variant.
3. **La riga del gate `v0.1` diventa soddisfacibile** e va riscritta perché dica ciò che verifica:
   ~~«almeno un modifier supportato può essere testato»~~ → *«il breakdown mostra i nove stadi, e il valore
   finale coincide con `FRTTurnLogEntry::Amount` della stessa run»*. Questo è un criterio che
   un'automation decide.

⚠️ **Un difetto adiacente, dichiarato e già tracciato**: lo stadio 5 porta `Deflect` (`−20`) come delta di
primo colpo, e il commento a `RTTurnManager.cpp:4632` dice che conserva il difetto che `D-292` ha tolto
alla Guardia — *«su un colpo più piccolo l'avanzo si perde»*. È in **#1909**. Un breakdown che mostrasse
quello stadio lo renderebbe **visibile a un designer** prima che a un lettore di codice: è un argomento a
favore di `SW0-03`, non contro.

⚠️ **Ciò che questo §10 NON stabilisce**: che l'ordine debba restare così. Dice che **esiste**, che è
motivato stadio per stadio da decisioni accettate, e che la `v0.1` del Workbench non ha bisogno di
cambiarlo. Se un domani servisse un sistema dato-guidato, questa pipeline è la specifica di partenza — non
un ostacolo. Candidato a voce di Decision Log; **nessun `D-nnn` rivendicato qui**, perché prenotarlo in un
referto non lo protegge.

---

## 11. Stato di questo lavoro

⚠️ **Il checkout è condiviso, e `HEAD` si è mosso durante il lavoro**: `8bfe8f84` → `3e7b3110`, per mano
di un'altra sessione, con due commit `docs(spec-panel)`. Nessun `UnrealEditor.exe` vivo. Nessun branch
creato, nessun commit da questa sessione.

✅ **Le misure reggono, e non è un'assunzione**: `git diff --name-only 8bfe8f84..HEAD` sui file misurati —
`RTTurnManager.cpp`, `Combat/`, `RTActionData.h`, `RTActionDef.h`, `RTTestScenario.h`, `RTTurnLog.h` — è
**vuoto**, e la riga 4660 è verbatim quella citata al §10. I due commit toccano solo `docs/`.

**`NOT RUN`**: build, suite (`./scripts/rt-suite.ps1`), sessioni PIE/Editor. Questo referto è documentale e
non ha eseguito nulla. Le classificazioni del §6 sono misurate su codice e su stato issue lato server, non
su una run.

⚠️ **Una misura di questo referto era inferenziale, è stata dichiarata, ed è caduta.** La prima stesura
scriveva: *«"non esiste un modifier system" è un'assenza, provata da grep su `Source/` e `Plugins/`. Un
sistema che esistesse con un vocabolario diverso da `Modifier`/`Stack`/`Apply` non comparirebbe.»*

**È esattamente ciò che è successo.** Il sistema esiste e si chiama `ApplyFirstHitDelta` ·
`ApplyDamageDelta` · `ApplyAbsorptionPool` · `EffectiveAttackPower` · `EffectiveCoverReduction` ·
`ApplyDamage`. Nessuno dei sei nomi contiene una delle tre parole cercate.

> 🔵 **La riserva ha fatto il suo mestiere: era scritta prima di sapere che serviva.** Un'assenza
> dichiarata come inferenza si corregge; un'assenza dichiarata come misura sarebbe diventata la premessa
> di una issue che ordinava di costruire ciò che c'è già — cioè il secondo modifier system che il §22 del
> kit vieta, aperto **in nome** del §22.

### Prossimo passo

⛔ **Fatto due volte.** `SW0-03` è risolta al §10 (l'ordine esiste, nove stadi, la Variant tocca solo il
primo) e `SW0-02` è aperta come **#1953** con la misura del §8.1.

Il passo successivo è **implementare #1953**, che non dipende da #1951 e non tocca l'Editor. Il suo primo
lavoro non è la query: è il test `HeroKitsMatchTheirCatalogDef` — scrivere il guardiano che un commento
dichiara esistente da prima di questo referto, e vedere se le venti abilità del roster passano. **Se una
non passa, il readout va progettato sapendolo**; se passano tutte, la query nasce sopra una base misurata
invece che assunta.
