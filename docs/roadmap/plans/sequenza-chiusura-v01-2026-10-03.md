# Sequenza di chiusura della v0.1 — rilettura sul candidate congelato

> **Stato**: `CURRENT` · **Creato**: 2026-10-03 · **Tipo**: **vista di esecuzione**, non owner.
>
> **Base di misura**: `main` = `768c65f4d`, albero `fd85089c153d41243c46d07d97f9532561b55213`,
> `git status --porcelain` → **0** righe. Stato delle issue e delle PR letto lato server con `gh`
> il **2026-10-03**.
>
> ♻️ **Tre basi, e le sezioni dicono quale usano.** §1–§7 stanno su `768c65f4d`. L'**§8** — il
> consuntivo dell'Onda A, eseguita lo stesso giorno — sta su `a89b1f3f9`
> (albero `1cfc8085bd68267509882dfaaeeb923f3d2e1e1c`), e la nota di §2.2 che registra la caduta di
> `doc-tables` pure. L'**§9** — l'Onda B, parziale — sta su `9faced8e7`
> (albero `19c96b7155138f6eb17d522e2f15119b2d3d0b27`).
>
> 🔑 **E le tre basi sono lo stesso codice**, che è ciò che rende confrontabili le misure: tutte
> hanno `Source` e `Content` identici a `95eddfd37`, e ogni commit di mezzo è sotto `docs/`. La
> catena si riverifica con due `git rev-parse` — il blocco in §2.1.
>
> ⚠️ **§2.3 è stata ridatata dall'§9 su `G3` `G4` `G5` `G6` `G8` e `G11`**, e le righe lo dicono.
> L'Onda A invece non toccava `Source/`, quindi non ridatò nulla.
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
[6. Le mie ipotesi cadute](#6-tre-mie-ipotesi-cadute-dichiarate) · [7. Limiti](#7-limiti-dichiarati) ·
[8. Consuntivo dell'Onda A](#8-consuntivo-dellonda-a--eseguita-il-2026-10-03) ·
[9. Consuntivo dell'Onda B](#9-consuntivo-dellonda-b--parziale-il-2026-10-03)

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

> ✅ **Caduto il 2026-10-03, dopo il merge di `#3442`** (§8). Rimisurato su `a89b1f3f9`:
> `node tools/radar/doc-tables.ts --check` → exit **0**, e le righe *«celle invece di»* sono **0**.
> I due comandi di `G14` restano exit **0** sullo stesso albero. ∴ il runbook di `G13` è valido, e
> la seduta `B1` non inciampa nella riga 144.

### 2.3 Dove stanno i gate, e su quale albero

Dalle celle del DoD §3, con la loro data:

| Gate | Stato | Albero su cui la cella lo dichiara |
|---|---|---|
| `G1` `G2` `G10` `G12` | ✅ | candidate **`95eddfd37`** |
| `G7` | ✅ | `3f693c088` (`D-447`), residuo aperto in [#3438](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3438) |
| `G9` | ✅ | `17fbc400`, 2026-09-12 |
| `G11` | 🟡 | **tre KPI su quattro** sul candidate, 2026-10-03 (§9) — era ✅ su `2f765c08f` |
| `G14` | ✅ | rimisurato oggi, §2.2 |
| `G3` `G4` `G5` `G6` `G8` | ✅ | **candidate `95eddfd37`**, ridatati il 2026-10-03 (§9) — erano su `bbf0d780` |
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
| A4 | ~~chiudere~~ **aggiornare** [#2554](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2554) | Il codice **è** su `main` dal 2026-09-06 e il suo unico commento dice il contrario (*«NON compilato, commit di salvataggio»*): `git merge-base --is-ancestor f29dd3749 768c65f4d` → vero. ⛔ **Ma non si chiude** — §8.2: il suo DoD ha sei caselle e cinque chiedono l'occhio in Editor |
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

---

## 8. Consuntivo dell'Onda A — eseguita il 2026-10-03

Misure su `main` = `a89b1f3f9`, albero `1cfc8085bd68267509882dfaaeeb923f3d2e1e1c`,
`git status --porcelain` → 0 righe.

| | Esito | Evidenza |
|---|---|---|
| A1 · [#3443](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3443) | ✅ `MERGED` | merge commit `9bdb1b1a3` |
| A2 · [#3442](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3442) | ✅ `MERGED` | merge commit `3fa814a4b` |
| A3 · [#3444](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3444) | ✅ `MERGED` | merge commit `a89b1f3f9` — dopo `A2`, come prescritto |
| A4 · [#2554](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2554) | ⚠️ **aggiornata, NON chiusa** | §8.2 |
| A5 · [#2477](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2477) | ✅ milestone → `v0.2 · Struttura e finestre` | `gh issue view 2477` lo conferma |
| A6 · [#85](https://github.com/DegrassiAaron/refactor-tactics-main/issues/85) | ✅ candidate dichiarato | passo 1 della sua DoD, e **solo** quello |
| A7 · [#3438](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3438) | ⏳ **non eseguita** | la riga di decisione in `OPEN_DECISIONS.md` resta da aprire |

Effetto laterale misurato: **`doc-tables` è passato a verde** (§2.2), quindi il runbook di `G13` non
ha più la riga rotta che la seduta `B1` avrebbe incontrato.

E una chiusura che non era in tabella: **[#3362](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3362) è
chiusa**. `#3443` la dichiara chiusa nel corpo, ma scrive *«Chiude #3362»* — e GitHub riconosce come
keyword solo le forme inglesi, quindi `closingIssuesReferences` era **vuoto** e la issue non si
sarebbe chiusa da sé. I tre congiunti del suo DoD sono stati misurati prima di chiuderla.

### 8.1 ⚠️ Un test di `#3443` dipende dalla working directory

Trovato eseguendo `A1`, e non invalida il merge:

```sh
python -m pytest -q tools/editor-sessions   # dalla radice: 85 passed
cd tools/editor-sessions && python -m pytest -q   # 1 failed, 84 passed
# FileNotFoundError: 'docs\technical\test-manuali-pie.md'
```

`pie_status.load()` risolve `REGISTRO` come percorso **relativo alla cwd**, mentre il test nuovo
calcola la radice per conto proprio per costruire l'atteso: le due metà dello stesso test usano due
convenzioni, e una sola è indipendente da dove lo lanci.

🔑 **Gli 85 sono 82 + 3**, cioè la baseline che la PR dichiarava più i suoi test nuovi: il conto torna,
e il rosso è di percorso, non di contenuto. ⛔ Ma chi lancerà la suite da dentro quella cartella
leggerà una regressione che non c'è. Il rimedio è una riga — far derivare `REGISTRO` da `__file__`,
come già fa il test — e sta registrato nel commento di chiusura di `#3362`.

### 8.2 ⌫ `A4` era sbagliata: da «mergiata» non segue «chiudibile»

La riga `A4` diceva *«chiudere #2554»*, e la premessa era giusta: il lavoro è su `main` dal
2026-09-06 con [#2591](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2591), un file solo
(`SRTAnimPreviewViewport.cpp`), e il branch remoto non esiste più
(`git ls-remote --heads origin | grep -c 2554` → **0**).

⛔ **Ma la conclusione no.** Il DoD di `#2554` ha sei caselle, **nessuna spuntata**, e cinque non sono
soddisfabili da un merge: i quattro controlli di `Play`, la velocità a `0,1x`, l'inquadratura, i
quattro pack, e *«build pulita e suite verde ricompilando»*. L'unica caduta è la sesta — le tre voci
di giudizio, date in [#2521](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2521),
`CLOSED` dal 2026-09-28.

∴ chiuderla avrebbe registrato come fatto ciò che nessuno ha guardato, ed è la forma che
[`CLAUDE.md`](../../../CLAUDE.md) §14 vieta: `PR MERGED` non è `DONE`. La issue è stata **aggiornata**
— perché il suo unico commento diceva il contrario del vero — e resta aperta per una seduta Editor
sul clone principale, dato che il modulo è `RefactorTacticsEditor`.

---

## 9. Consuntivo dell'Onda B — parziale, il 2026-10-03

Misure su `main` = `9faced8e7` durante la run, albero `19c96b7155138f6eb17d522e2f15119b2d3d0b27`,
il cui `Source` è `d6a1c4419d29` — **identico** a `95eddfd37:Source`.

| | Esito | Nota |
|---|---|---|
| `B1` · `G13` | ⏳ **non eseguibile da qui** | §9.1 |
| `B2` · `G11` | 🟡 **tre KPI su quattro** | il quarto chiede una cattura sul pacchetto |
| `B3` · `G3` `G4` `G6` `G8` | ✅ **ridatati** | una sola passata, nomi verificati sul `Path` |
| `B3` · `G5` | ✅ **ridatato** | `git grep`, con controllo positivo del metodo |
| `B4` · `G16` | ⏳ **non eseguibile da qui** | §9.1 |

La passata: `python tools/suite/esegui.py RefactorTactics` → **2822 trovati, 2822 avviati, 2822
completati, nessun rosso**, 250 s. Letta con i **due** metodi che concordano — l'oracolo dei conteggi
di [#3048](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3048) e il conteggio
indipendente dei `Result={...}`: **2822** `Success`, **0** `Fail`.

🔑 **La validità è stata costruita, non assunta**: modulo ricompilato prima della run
(`Result: Succeeded`, **0** warning), `Source/` intatto nella finestra
(`find Source/ -newermt '2026-10-03 12:55'` → **0**), e l'unico file toccato durante la run —
`docs/OPEN_DECISIONS.md` alle 12:58 — **non è letto da nessun test**: i soli `.md` che un test
Automation apre sono `roadmap-pia.md`, `v0.1-definition-of-done.md` e `test-manuali-pie.md`, tutti
con mtime anteriore a 12:55:04.

### 9.1 ⛔ `G13` e `G16` non si chiudono senza una persona, e non è una questione di permessi

Entrambi hanno **oracoli percettivi** nel criterio, e nessun log li può dare:

- `G13` chiede che l'interfaccia sia **leggibile** e che il giocatore **trovi** l'affordance — più una
  sequenza da *digitare* in una finestra di gioco: selezionare un'unità, posare un waypoint, premere
  l'**RMB**. Il CSV prova che l'input è arrivato, non che fosse ovvio darlo;
- `G16` chiede che cinque superfici siano **usabili nello stesso giro**, e i suoi passi 3–5 sono
  osservazioni: *«il risultato del resolver è visibile e leggibile a schermo»*, *«turno e fase
  correnti visibili»*, `Play`/`Pause`.

∴ il resto dell'Onda B è pronto **per** quella persona: il pacchetto Development è staged e
appartiene al candidate per contenuto, il runbook di `G13` è valido (`doc-tables` exit **0**), e
l'Editor compilato da `main` è bit-per-bit il candidate su `Source/` e `Content/`.

### 9.2 🔴 La cella `G4` nominava un test che non esiste

Trovato ridatando, e non si sarebbe visto altrimenti. Il criterio cita
`Combat.GuardPoolIsPermutationInvariant`; nel codice e nel log c'è **solo**
`Combat.DeflectPoolIsPermutationInvariant`, che è `Result={Success}`:

```sh
grep -rhoE '"RefactorTactics\.Combat\.(Guard|Deflect)PoolIsPermutationInvariant"' Source/
# "RefactorTactics.Combat.DeflectPoolIsPermutationInvariant"   <- solo questa
```

Il rename è il soggetto di [#3203](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3203),
ancora **aperta**: la cella descriveva un futuro come se fosse presente. ⛔ Un criterio che nomina un
test assente **non può fallire per la ragione giusta** — quando #3203 atterra, quella riga torna vera
da sola, e fino ad allora dice il nome sbagliato.

### 9.3 ✅ Il delta del resolver non è una regressione di `691bd63b7` — l'ipotesi è stata esclusa

Era la ragione per cui §3.2 chiedeva la rimisura. Misurato su **tre** ripetizioni, stesso binario,
motore verificato libero prima di ciascuna: **14,631** · **12,772** · **13,915** ms su 10 turni.

Una mediana su dieci turni non è un punto, e con una ripetizione sola avrei pubblicato `14,631` come
se lo fosse.

⛔ **Ma non viene da quel commit**: [#3413](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3413),
aperta il **2026-09-30** su `57b43b589` — prima del candidate e prima di `691bd63b7` — misurava già
**13,209 ms**, dentro la banda di oggi.

⚠️ **L'anomalia è il numero della seduta `U16`**, registrato lo **stesso** 30 settembre: **9,059 ms**
su `6b50eb49f`. Due misure dello stesso giorno a **1,46×** di distanza, e quella bassa è finita nella
§4 come se fosse la banda. Il difetto che `#3413` possiede resta intero: la §4 porta `3,8–4,5 ms/turno`
dal 2026-08-14, e **nessuna** misura successiva ci somiglia.

🎯 **La rimisura è servita a escludere un danno, che è un esito e non un buco nell'acqua.** `691bd63b7`
tocca `ResolveReactionBoundary` e `PumpReactionTriggers`, cioè il percorso che `Perf.*` misura: senza
misurare, il sospetto sarebbe rimasto scritto in §3.2 come una riserva aperta.
