# Sequenza di chiusura della v0.1 — rilettura sul candidate congelato

> **Stato**: `CURRENT` · **Creato**: 2026-10-03 · **Tipo**: **vista di esecuzione**, non owner.
>
> **Base di misura**: `main` = `768c65f4d`, albero `fd85089c153d41243c46d07d97f9532561b55213`,
> `git status --porcelain` → **0** righe. Stato delle issue e delle PR letto lato server con `gh`
> il **2026-10-03**.
>
> **Cosa è**: l'ordine in cui il lavoro residuo della v0.1 può essere affrontato **sul candidate
> congelato `95eddfd37`**, derivato dai gate e non dall'elenco delle issue aperte. Aggiorna
> [`sequenza-chiusura-v01-spec-panel-2026-09-27.md`](sequenza-chiusura-v01-spec-panel-2026-09-27.md),
> di cui conserva il grafo e sostituisce lo stato.
>
> **Cosa non è**: un'assegnazione, una riscrittura di scope, una seconda Definition of Done. Il
> criterio di consegna resta [`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) §3; `#85`
> **attesta** che uno SHA la soddisfa. Dove questo documento e un owner divergono, **ha ragione
> l'owner**.
>
> ⚠️ **Gate rimisurati per questo documento**: `G14` (i due comandi del criterio, §2.2). Tutti gli
> altri stati citati sono **celle del DoD con la loro data**, non misure di oggi.
>
> 🔑 **Le affermazioni di questo documento sono state passate a un avvocato del diavolo**, uno per
> fronte. Tre mie ipotesi non hanno superato la verifica e sono dichiarate in **§6** invece di
> sparire nella revisione.
>
> **Convenzione sui numeri** ([`AGENTS.md`](../../../AGENTS.md) §14): nessun conteggio che cambi da
> solo. Dove serve una cardinalità, accanto c'è il comando che la produce; dove serve un insieme,
> ci sono i **nomi**.

---

**Indice** · [1. Il perimetro](#1-il-perimetro-e-invariato) · [2. Stato dei gate](#2-stato-dei-gate-sul-candidate) ·
[3. Le tre code](#3-le-tre-code) · [4. La sequenza](#4-la-sequenza) · [5. Rilievi](#5-rilievi) ·
[6. Le mie ipotesi cadute](#6-tre-mie-ipotesi-cadute-dichiarate) · [7. Limiti](#7-limiti-dichiarati)

---

## 1. Il perimetro è invariato

`G1`–`G14` **più `G16`**. Non è un intervallo: `G15` è ⌫ dal 2026-08-21
([`D-181`](../../decisions/RT_PDR_00_Decision_Log.md)) e il numero non si riusa.

L'appartenenza alla milestone `v0.1` **non** rende una issue release-blocking: la milestone è un
*tracking container* ([`D-377`](../../decisions/RT_PDR_00_Decision_Log.md)). ∴ la domanda *«quali
issue restano»* non si risponde elencando la milestone, ma risalendo dai due gate non verdi.

---

## 2. Stato dei gate sul candidate

### 2.1 Il candidate, e perché `HEAD` serve per compilarlo

Candidate congelato: **`95eddfd37`** (2026-10-02 20:23 locali). I commit entrati dopo toccano solo
`docs/`:

```sh
# ancorato a 768c65f4d, non a HEAD: il confronto che segue vale su QUESTO albero
git rev-parse 768c65f4d:Source    # d6a1c4419d29f68f6ac79b8ca8f38c686efdc728
git rev-parse 95eddfd37:Source    # d6a1c4419d29f68f6ac79b8ca8f38c686efdc728
git rev-parse 768c65f4d:Content   # 75f060efe82e756d60350a965aaedfb02a511f5e
git rev-parse 95eddfd37:Content   # 75f060efe82e756d60350a965aaedfb02a511f5e
git diff --name-only 95eddfd37..768c65f4d -- Source/ Content/   # 0 file
```

🔑 **Il 2026-10-03 compilare da `main` era compilare il candidate**, per contenuto e non per fiducia:
chi conduce una seduta non deve fare checkout del candidate per non perdere i documenti.

⛔ **E questa uguaglianza va RIMISURATA all'atto d'uso, non ereditata da questa riga.** Vale finché
nessun commit toccherà `Source/` o `Content/`; il primo che lo fa la rompe in silenzio, e un `git
rev-parse` di due righe la riverifica:

```sh
test "$(git rev-parse HEAD:Source)" = "$(git rev-parse 95eddfd37:Source)" \
  && test "$(git rev-parse HEAD:Content)" = "$(git rev-parse 95eddfd37:Content)" \
  && echo "compilare da HEAD = compilare il candidate" || echo "DIVERSI: fai checkout di 95eddfd37"
```

### 2.2 `G14` — rimisurato oggi, verde

I **due** comandi che la cella del DoD assegna a questo gate, lanciati su `768c65f4d` con albero
pulito:

| Comando | Esito |
|---|---|
| `node tools/radar/doc-links.ts --check` | exit **0** |
| `node tools/radar/doc-coherence.ts --check` | exit **0** |

⚠️ **E un rosso che NON è di `G14`**: `node tools/radar/doc-tables.ts --check` → exit **1**, su una
riga sola — `docs/technical/runbooks/guida-seduta-g13-candidate.md:144`, *«3 celle invece di 4»*.
`doc-tables` non appartiene al criterio di `G14`, ma quella riga vive nel **runbook di `G13`**, cioè
nel foglio che la prossima seduta esegue. La corregge [#3442](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3442).

### 2.3 Dove stanno i gate, e su quale albero

Dalle celle del DoD §3, con la loro data:

| Gate | Stato | Albero su cui la cella lo dichiara |
|---|---|---|
| `G1` `G2` `G10` `G12` | ✅ | candidate **`95eddfd37`** |
| `G7` | ✅ | `3f693c088` (`D-447`), residuo aperto in [#3438](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3438) |
| `G9` | ✅ | `17fbc400`, 2026-09-12 |
| `G11` | ✅ | **`2f765c08f`** — non il candidate (§3.2) |
| `G14` | ✅ | rimisurato oggi, §2.2 |
| `G3` `G4` `G5` `G6` `G8` | ✅ | **`bbf0d780`**, 2026-08-29 (§3.3) |
| `G13` | 🟡 | `17fbc400` / `33634aee` — non il candidate |
| `G16` | ⏳ | giro non eseguito |

---

## 3. Le tre code

### 3.1 `G13` — la coda più corta, ed è eseguibile senza ricostruire niente

Il residuo è **una seduta di gioco sul pacchetto Development**, da `L_Frontend`, senza
`-RTAutobattle`. Il foglio di conduzione è
[`guida-seduta-g13-candidate.md`](../../technical/runbooks/guida-seduta-g13-candidate.md).

Il pacchetto **non va ricostruito**, e il binario appartiene al candidate **per contenuto**: contiene
l'header a venti colonne di `URTPacingLibrary::CsvHeader()` in forma contigua, inclusa la colonna
`BoundaryCpuUsTotal` che `691bd63b7` ha aggiunto. Vive in
`Saved/StagedBuilds/Windows/RefactorTactics/Binaries/Win64/RefactorTactics.exe` — **nidificato**, ed
è il Development: quello alla radice di `StagedBuilds/Windows/` è il launcher.

⚠️ **Tre trappole, e ciascuna è costata a qualcuno:**

1. **`rt.Debug.RecordPacing 1` da `L_Frontend` non arma niente.** Senza `TurnManager` nel livello il
   comando risponde e non registra: va digitato **dopo** essere in partita.
2. **Va premuto l'RMB, non `BackSpace`.** `UndoAction` è mappata su entrambi
   (`Source/RefactorTactics/Player/RTPlayerController.cpp:610-611`), e `BackSpace` soddisfa la
   lettera del congiunto `UndoCount >= 1` mancando **esattamente** il difetto per cui esiste.
3. **`Saved/StagedBuilds/Windows/RefactorTactics/Saved/Crashes/` non è vuota**: contiene gli
   artefatti delle run packaged di `G2` del 2026-10-02 (due `Assert` da `-nullrhi`, un `GPUCrash`).
   Una cartella preesistente non è evidenza di questa seduta.

⛔ **Il difetto RMB non è accertato, è il bersaglio di falsificazione.** La cella `G13` lo scrive in
controfattuale e il runbook al congiuntivo; `grep -rn "non raggiunge il controller" docs/` colpisce
quei due soli. Trattarlo come misurato cambierebbe la seduta da **misura** a **conferma**.

### 3.2 `G11` — la rimisura è dovuta nel merito, non per formalità

`G11` è verde su `2f765c08f`. Fra quell'albero e il candidate, su `Source/`, c'è **un solo** commit:

```sh
git log --oneline 2f765c08f..95eddfd37 -- Source/
# 691bd63b7 feat(2516): il boundary si cronometra per DURATA, e nessun ramo del resolver legge un tempo
```

🔑 **E tocca il sistema che `G11` misura.** Il cronometraggio entra in `ResolveReactionBoundary` —
chiamata da `Source/RefactorTactics/Turn/RTTurnManager_Movement.cpp:421`, dentro la risoluzione del
movimento — e in `PumpReactionTriggers`. I tre KPI che sono test Automation stanno in
`Source/RefactorTactics/Tests/RTHexPerfTests.cpp`: `RefactorTactics.Perf.PathfindingMedian`,
`RefactorTactics.Perf.PlanningPreviewMedian`, `RefactorTactics.Perf.TurnResolverMedian`. Il quarto è
una cattura CSV sul pacchetto Development, che `G12` dichiara staged da `95eddfd37`.

⚠️ **L'esito va scritto in DUE posti**, o la deriva si ricrea: la cella `G11` del §3 **e** le righe
di budget della §4. Nessuna riga della §4 nomina oggi il candidate.

⛔ **Non è il riferimento di `G16` a imporlo** — vedi §6.1, dove la mia ipotesi contraria è caduta.

### 3.3 `G3` `G4` `G5` `G6` `G8` — verdi su un albero che il candidate ha sorpassato

Le loro celle portano `bbf0d780`, 2026-08-29. La distanza da lì al candidate:

```sh
git rev-list --count bbf0d780..95eddfd37 -- Source/        # 1455
git diff --name-only bbf0d780..95eddfd37 -- Source/ | wc -l # 697
```

La clausola di [#85](https://github.com/DegrassiAaron/refactor-tactics-main/issues/85) — *«la tabella
si compila sul candidate, non si copia da `main`»* — non fa eccezioni, e l'Onda 3 del referto
precedente lo prescriveva già.

🔑 **Costano una sola run.** `G3`, `G4`, `G6` e `G8` sono test Automation: una passata di
`python tools/suite/esegui.py RefactorTactics` sul candidate li ridata tutti. `G5` è un `git grep`
istantaneo (`git grep -l FRTGridCoord -- Source/`).

⛔ **L'evidenza non esiste già**: `find Saved -iname "*.log" -newermt "2026-10-02 20:23"` risponde coi
soli log di `UnrealPak`, e gli `-abslog` di sessione del candidate sono `g1`, `g2`, `g10`, `g12` —
nessuna run di automation dopo il congelamento.

### 3.4 `G16` — il giro è lanciabile oggi, e il suo primo esito sarà 🔴

Pronto, misurato:

- il contenitore esiste — `U60` in [`editor-sessions.yaml`](../editor-sessions.yaml) con
  `issues: [2601]`, che è ciò che disinnesca `A2` di `doc-coherence.ts` (`seduteOrfane`), e §2.2
  misura `A1`–`A5` tutte PASS;
- quattro delle cinque capability sono chiuse: Ability Lab
  [#2599](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2599), Hero Lab
  [#2600](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2600), Presentation
  [#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288), Replay Viewer
  [#472](https://github.com/DegrassiAaron/refactor-tactics-main/issues/472) — `gh issue view`,
  `state: CLOSED`. Resta scoperta **Turn Log proiettato al giocatore**
  ([#1936](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1936)).

∴ per la funzione di verdetto che la cella scrive — ✅ solo se tutti e cinque i passi sono ✅, un
passo ❌ rende `G16` 🔴 e **mai** ⏳ — il primo giro produce necessariamente 🔴 sul passo 4. **Va
eseguito comunque**: nessuno ha mai misurato se le altre quattro superfici reggano *insieme*, ed è la
domanda per cui questo gate esiste.

🔴 **Un rischio sul passo 5 indipendente dal passo 4.** L'archivio da riaprire è quello prodotto dalla
partita dei passi 3–4, identificato per `MatchId` — ma la partita non lo stampa mai:

```sh
grep -rnE 'UE_LOG[^;]*MatchId' Source/ --include=*.cpp | grep -v /Tests/   # 0
grep -rn  'MatchId' Source/ --include=*.cpp --include=*.h | grep -v /Tests/ | wc -l   # 74
```

La correlazione si fa sulla cartella più recente sotto `Saved/Replays/` e si **conferma** sul nome
della cartella (`MatchId.ToString(EGuidFormats::Digits)`) o sul campo `"MatchId"` di
`match.rtmanifest`. Senza questa conferma il passo 5 passerebbe su un archivio qualunque.

---

## 4. La sequenza

### Onda A — a motore spento

| | Lavoro | Perché adesso |
|---|---|---|
| A1 | merge [#3443](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3443) | Chiude [#3362](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3362). Due file sotto `tools/editor-sessions/`, nessuna sovrapposizione con le altre PR aperte, DoD verificabile **senza** Unreal. Baseline riconfermata: `python -m pytest -q` in quella cartella → `82 passed, 2 subtests passed` |
| A2 | merge [#3442](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3442) | Rende verde `doc-tables`, e la riga che oggi lo tiene rosso è nel **runbook di `G13`** (§2.2) |
| A3 | merge [#3444](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3444) | **Dopo** `A2`: il suo stesso corpo dichiara che `doc-tables` resta rosso sul suo branch per quella riga. Apre `MOV-15` come domanda, quindi porta una decisione da firmare |
| A4 | chiudere [#2554](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2554) | **È già mergiata** e il suo unico commento dice il contrario (*«NON compilato, commit di salvataggio»*): `git merge-base --is-ancestor f29dd3749 768c65f4d` → vero |
| A5 | milestone di [#2477](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2477) | `U5` è **differita a `E26`** dal 2026-09-28 e [`D-441`](../../decisions/RT_PDR_00_Decision_Log.md) la registra; la issue è ancora `v0.1` · `P1` |
| A6 | candidate in [#85](https://github.com/DegrassiAaron/refactor-tactics-main/issues/85) | Il suo passo 1 è *«congelare il candidate SHA e dichiararlo»*. Il body dice ancora *«da congelare»*, e lo stato non vive in un commento: `gh issue view 85 --comments` ne restituisce **uno**, del 2026-09-12 su `G12` |
| A7 | la direzione di [#3438](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3438) | Il residuo di `G7` ha quattro direzioni possibili e nessuna scelta: va aperta come riga in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) perché un `D-nnn` la possa chiudere. ⛔ Lo slot `GATE-1` è chiuso e il numero non si riusa |

⚠️ **`A1`–`A3` non sono tutto il gruppo recente.**
[#3445](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3445) e
[#3441](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3441) hanno
`closingIssuesReferences: []` — non chiudono issue — e il corpo di `#3441` annuncia una metà che la
PR non porta più: `bdfeebdd4` l'ha messa su `main` direttamente.

⛔ **Le PR del gruppo vecchio non sono pronte, e `mergeable` non lo dice.** Il discriminante è
`git rev-list --count`: [#3206](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3206) è
la più pulita (file non conteso, corpus golden immutato) e blocca solo sulla rimisura;
[#3205](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3205) va riletta contro
[`D-425`](../../decisions/RT_PDR_00_Decision_Log.md), che non cita affatto;
[#3203](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3203) ha un conflitto reale e
isolato sul Decision Log; [#3230](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3230)
è in bozza per decisione dell'autore.

### Onda B — il motore, una cosa per volta

Unreal è **uno** ([`AGENTS.md`](../../../AGENTS.md) §11). L'ordine non è una preferenza:

```text
B1  G13   seduta sul pacchetto staged        -> chiude il 🟡
B2  G11   i quattro KPI sul candidate        -> toglie la deriva (§3.2)
B3  G3 G4 G6 G8   una run Automation         -> ridata su 95eddfd37 (§3.3); G5 e un git grep
B4  G16   il giro U60, accettando il 🔴       -> misura se le cinque superfici reggono insieme
```

`B4` **non aspetta** la chiusura di `#1936`: il suo esito 🔴 sul passo 4 è previsto dal criterio, e
rimandarlo lascerebbe non misurate le altre quattro superfici.

### Onda C — il codice del passo 4

La catena del Turn Log, nell'ordine che la misura suggerisce:

1. [#3073](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3073) **come misura, non
   come correzione**: potrebbe essere un difetto falso (§5.1);
2. [#2764](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2764) — **cinque**
   compositori senza consumatore, col sesto come precedente già eseguito (§5.2);
3. [#2964](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2964) — richiede una partita
   in corso, non è una misura statica;
4. [#1936](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1936) — il tetto.

### Onda D — [#85](https://github.com/DegrassiAaron/refactor-tactics-main/issues/85)

Compilare la tabella di attestazione sullo SHA congelato, allegare l'evidence bundle, dichiarare le
known limitations, dichiarare la release.

---

## 5. Rilievi

### 5.1 🔴 [#3073](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3073) potrebbe essere un difetto falso, e il suo DoD lo chiede

Il suo DoD chiede di distinguere *«la correzione di #2831 non ha mai toccato questo Blueprint»* da
*«la guardia è stata reintrodotta»*. Misurato:

```sh
git log -1 --oneline -- Content/RT/UI/Match/WBP_RT_EventLog.uasset
# d47f5caa8 fix(2831): il feed aggiorna i testi anche quando il numero di righe non cambia
```

∴ la prima ipotesi è **falsa** — la correzione di `#2831` ha toccato proprio quell'asset, ed è
l'ultimo commit che lo tocca. Scrivere prima il test headless che cammina il grafo (sul modello di
`Source/RefactorTacticsEditor/Private/Tests/RTActionSlotClickWiringTests.cpp`) evita di spendere una
seduta d'Editor su un difetto inesistente.

⚠️ **E l'asserto va scritto nel verso giusto.** Nel grafo non esiste un nodo di sequenziamento — niente
`Sequence`, `MultiGate`, `DoOnce` — e senza di esso un'istruzione che giri dopo un `Branch` può
essere cablata **solo** dai pin d'uscita del `Branch`. L'asserto corretto è *«il `SetText` è
raggiungibile da **ogni** pin exec in uscita dell'unico `K2Node_IfThenElse`»*, e chi lo scrive deve
visitare più successori. L'asserto opposto andrebbe rosso sul grafo **corretto**.

### 5.2 🟡 [#2764](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2764) è più piccola del suo titolo, e il suo corpo lo ha già ritrattato

Il titolo dice *«i sei compositori di `ARTHUD` hanno zero consumatori»*. Oggi sono **cinque** senza
consumatore — `ComposeMatchStatusLine`, `ComposeSlotLineStyle`, `ComposePreviewZoneLine`,
`ComposePlaybackSpeedLabel`, `ComposeSlotLines` — mentre `ComposeAbilityLine` ha un chiamante di
produzione reale a `Source/RefactorTactics/UI/RTScreenHudWidgets.cpp:613`, esposto come
`GetActionLine()` e pinnato per uguaglianza da
`RefactorTactics.ScreenHud.ActionSlotLineIsTheSameComposerAsTheHud`.

🔑 **Il sesto non è un orfano: è il precedente eseguito** che gli altri cinque devono seguire. La
forma della correzione — porta in C++ come inoltro puro — è già pagata una volta.

### 5.3 🟡 I bloccanti di `#1936` sono quattro, e nessuno sta nel corpo

```sh
gh issue view 1936 --json body --jq .body | grep -cE '#(2964|3073|2764)'   # 0
```

Stanno nell'ultimo commento, del 2026-09-15, sotto *«Che cosa blocca la chiusura»* — e sono
**quattro** voci, non tre: dopo `#2964`, `#3073` e `#2764` c'è *«oracolo umano»*. *«Tre bloccanti»* è
il conteggio delle issue, non dei bloccanti.

### 5.4 🟡 Nessuna `P0` aperta è lavoro

Le `P0` aperte della milestone portano tutte `epic` o `checkpoint` — `#14` `#16` `#26` `#38` `#84`
`#85` `#152` `#166` `#2462` `#2601` — e l'elenco non è troncato (`gh issue list --limit 500` dà gli
stessi nomi). Un contenitore si chiude quando si chiudono i figli: non è una issue da «risolvere».

### 5.5 🟡 [#3063](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3063) non è una regressione viva

Il corpo cita `RTPlayerController.cpp:1331` e `:1334` come la guardia che uccide il click. Su
`768c65f4d` quel `return` non esiste: il trace è non-bloccante a `:1613-1617`, e il commento sopra
nomina la issue. Resta aperta per i soli criteri PIE.

### 5.6 ⚠️ [#3424](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3424) è aperta e senza milestone

È il crash GPU allo shutdown dell'automation packaged, che la cella `G2` dichiara difetto aperto e
*«da nessuna parte lavorato»*. Non blocca il gate — il verdetto si legge dai conteggi, e il crash cade
dopo l'ultimo `Result={Success}` — ma senza milestone non è in nessuna coda.

### 5.7 ⚠️ Il banner del DoD è fermo, e il file no

`docs/roadmap/v0.1-definition-of-done.md:3` dice *«Ultimo aggiornamento: 2026-09-12»*; l'ultimo
commit che tocca quel file è `4b1566ccb`, **2026-10-03**. Le celle portano ciascuna la propria data —
che è ciò che conta — ma il banner invita a fidarsi di una data sbagliata.

### 5.8 ⚠️ Il marcatore di `U16` manca benché la seduta sia stata eseguita

Il campo `steps` di `U16` dichiara *«ESEGUITA il 2026-09-30 sul candidate `6b50eb49f`»*, e `G11` è
passata ✅ nella stessa tornata. Ma il criterio canonico del marcatore —
`(?:✅|🟡)\s*\*\*(Eseguita|CHIUSA|Chiusa|Parzialmente)` — non trova nulla nel suo record. Chi legge il
registro col comando canonico la conta **aperta**. È la stessa forma che la cella `G13` denuncia per
`PIE-V01-FRONTEND-MAIN`, e il marcatore appartiene a chi condusse la seduta.

---

## 6. Tre mie ipotesi cadute, dichiarate

### 6.1 ⌫ *«Con due SHA diversi il riferimento di `G16` non risolve»* — falso

Il criterio di `G16` dice *«lo stesso candidate che `G2`, `G11` e `G12` dichiarano nelle proprie
celle»*, non *«il più recente che ciascuna dichiara»*. Intersecando i token SHA delle tre righe:

| Cella | SHA citati |
|---|---|
| `G2` | `0a0d73bfa` `2f765c08f` `5a1a5a5a1` `6b50eb49f` `6e2bf7153` `95eddfd37` `d9950063d` |
| `G11` | `2f765c08f` `6b50eb49f` |
| `G12` | `2f765c08f` `95eddfd37` `d5a004b36` |

**Intersezione = `{2f765c08f}`**, uno solo: il riferimento risolve, e risolve in modo univoco. `G2` e
`G12` contengono anche il candidate vecchio come *«misura precedente»*, marcata col glifo cronometro.

🔑 **Il nucleo regge per un'altra ragione, ed è quella di §3.2**: la rimisura di `G11` è dovuta perché
l'unico commit di delta tocca il sistema che `G11` misura — non perché un riferimento sia rotto.

### 6.2 ⌫ *«La metà packaged di `G2` non ha evidenza: i log finiscono in cartelle di crash»* — falso

Avevo contato `ClientContext` nei log packaged e trovato **0**, concludendo che i tredici `Success`
non fossero lì. `ClientContext` è un **flag C++** (`EAutomationTestFlags::ClientContext`), non un
token di log: cercarlo nel log è la domanda sbagliata. La cella `G2` documenta già il crash, lo
colloca **dopo** l'ultimo `Result={Success}` per numero di riga, e si era già corretta da sé il
2026-10-03 su *«0 crash»*.

### 6.3 ⌫ *«La rimozione di `U5` dai prerequisiti di `U19` non è registrata nel Decision Log»* — falso

Il commento in `editor-sessions.yaml` dice *«va registrato nel Decision Log … qui non si rivendica un
numero `D-nnn`»*, e l'ho letto come una prescrizione pendente.
[`D-441`](../../decisions/RT_PDR_00_Decision_Log.md) la registra — accettata il 2026-09-27,
registrata il 2026-09-28. È il commento a essere stantio, non la decisione a mancare.

---

## 7. Limiti dichiarati

1. **Rimisurato solo `G14`.** Gli altri stati sono celle datate. L'Onda B li ridata.
2. **Nessuna misura con il motore.** Niente build, niente Automation, niente PIE: `G13`, `G11`,
   `G3`–`G8` e `G16` restano `NOT RUN` su questo albero, e `NOT RUN` non equivale a `PASS`.
3. **Il binario staged è stato verificato per contenuto, non ricompilato.** La prova è l'header a
   venti colonne e `BoundaryCpuUsTotal`; chi conduce `B1` rifaccia il passo 0 del runbook.
4. **Le sette caselle di `#84` non sono state verificate una per una** contro la §4 del DoD — stesso
   limite che il referto del 27-09 dichiarava, e non è stato colmato.
5. **Il residuo di `#166` è assunto dal suo ultimo commento**, non misurato: *«ciò che manca non è che
   il bot possa fermare il giocatore, è la finestra»*. Se fosse più grande, l'Onda B si allunga.
6. **Le PR del gruppo vecchio non sono state ricompilate.** Il giudizio su `#3203`, `#3205`, `#3206`
   e `#3230` viene dal diff e dal grafo dei commit, non da una build.
