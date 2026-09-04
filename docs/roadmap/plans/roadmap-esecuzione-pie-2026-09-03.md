# Roadmap — eseguire le PIE rimaste

> `CURRENT` · **Creato**: 2026-09-03 · **Owner**: questo file, fino a quando le sedute che elenca sono aperte.
> **Misurato su** `origin/main` **dopo il merge di 8 commit** (vedi la nota della §1). ⚠️ **Questa riga
> diceva `2eb4ace2` e lo ha detto per mezza giornata**: è la base su cui il file è nato, e i suoi numeri
> non valgono più — chi rieseguisse il comando canonico contro quel commit otterrebbe `200 · 98 · 68 · 30`
> e concluderebbe che le tabelle sono sbagliate. 🔑 **Un documento che dichiara la propria base di
> misura deve aggiornarla quando rimisura**, o la dichiarazione diventa la bugia meglio nascosta del file. Il registro degli esiti resta
> [`test-manuali-pie.md`](../../technical/test-manuali-pie.md); *quale voce e quando* resta
> [`editor-sessions.yaml`](../editor-sessions.yaml). Questo file dice **in che ordine** e **cosa manca**.

## 🔑 La risposta, prima di tutto il resto

La domanda era *«quante issue dobbiamo implementare per eseguire le PIE rimaste?»*.

Per la seduta più ricca — **U42, ventuno voci in undici Play** — la risposta misurata è **zero**.

`unblocked_by: []`, e le sue quattro issue (`#231 #233 #1919 #2009`) sono **tutte CLOSED**. I 29 file di
`Scenarios/Visual/` esistono, e girano: `Scenario.EveryShippedScenarioRuns` e
`Scenario.ShippedScenariosAreValid` sono **`Success`** (run VALIDA del 2026-09-03, `168/168`).

⚠️ **Il collo di bottiglia non è l'implementazione: è che nessuno convoca la seduta.** Il titolo di U42 lo
dice da sé — *«Il corpus Visual, diciotto scenari che nessuna seduta convocava»*.

---

## 1. Lo stato, in numeri

| | |
|---|---|
| voci totali · aperte | **203** · **101** |
| aperte **schedulate** in una seduta | **69** |
| aperte **fuori da ogni seduta** | **32** |
| aperte che dichiarano un ostacolo | 21 — ⚠️ **non rimisurato**, vedi la nota |

> 🔄 **Rimisurato il 2026-09-03 su `31859e13`, e i quattro numeri erano già invecchiati prima di
> questa passata.** Dicevano `200 · 98 · 68 · 30`; la base senza le modifiche di oggi misura
> `201 · 99 · 66 · 33`. Questo file è stato scritto **stamattina** su `2eb4ace2`: fra le due misure
> `main` è avanzato, ed è il difetto che il registro delle PIE documenta da undici giri — *il numero si
> ricalcola, non si aggiorna a mente*, e vale anche per un documento nato lo stesso giorno.
>
> ➕ **Il contributo di questa passata è esattamente uno**: entra `PIE-V01-SHIELD`, schedulata in `U18`
> ([#1403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1403)). Totale e aperte salgono
> di uno, le **schedulate** di uno, e le **orfane non si muovono**: 33 prima, 33 dopo. ⏱️ *Rimisurato il 2026-09-04: le orfane sono **32** — `PIE-V01-PACKAGED` è entrata in `U23`, che aveva `verifies: []` pur avendo un piano e cinque issue.*
>
> ⚠️ **Le «21 con ostacolo dichiarato» non sono state rimisurate** e restano come le trovo: contarle
> chiede di leggere il testo di ogni riga, non un marcatore, e nessuno strumento qui lo fa. Il numero
> è quello di stamattina e va trattato come tale.
>
> ✅ **Rimisurato una seconda volta dopo aver mergiato `origin/main` (8 commit, merge PULITO), e i
> numeri NON si muovono**: `202 · 100 · 67 · 33` prima e dopo. 🔑 **E si sa perché, che è diverso dal
> constatarlo**: dei tre file toccati, il registro ha ricevuto **testo e nessuna riga di voce**, e la
> seduta nuova `U44` ha `verifies: []`, quindi non sposta né le schedulate né le orfane. ⚠️ Il merge non
> ha chiesto attenzione — nessun conflitto, nessun marcatore — ed è il caso in cui l'unica cosa che
> segnalerebbe uno scarto è **rieseguire il comando**: una conferma vale la misura quanto una correzione.
>
> 🔑 **Metodo**: stesso criterio del comando canonico del registro — le **righe di tabella**, non un
> `grep` sugli ID. ⚠️ La prima stesura di questa misura usava `[A-Z0-9-]` per l'identificativo e
> perdeva **dieci** voci, quelle con una minuscola (`PIE-AS4a`, `PIE-BU2c`, `PIE-HEXPLAY-3b`): dava
> `192` dove il comando canonico dice `202`, e sarebbe passata inosservata se i due numeri non fossero
> stati confrontati.

⚠️ *«Senza ostacolo dichiarato»* **non** significa eseguibile: significa che la voce non dice di essere
bloccata. **Trentatré** di esse non sono in nessuna sequenza, ed è la condizione che
`spec-tactical-designer.md` §9 descrive come *«tende a non essere mai eseguita»*.

## 2. Le sedute, in ordine di resa

| seduta | voci aperte | `unblocked_by` | issue | verdetto |
|---|---|---|---|---|
| **U42** | **21** | `[]` | #231 #233 #1919 #2009 **CLOSED** · #2187 la convoca | 🟢 **eseguibile ora, e sono 11 Play non 21** |
| **U43** | **7** | `[]` | #151 (EPIC aperta) | 🟡 da verificare voce per voce |
| U25 | 6 | `[U21]` | #1095 aperta | 🔴 bloccata da una seduta |
| U5 | 5 | `[M6.6, M6.7]` | — | 🔴 bloccata da due checkpoint |
| **U18** | **5** | `[]` | #450 #567 #583 #551–554 | 🟢 **non attende nulla**, come U42 |
| U19 · U22 | 4 ciascuna | — | varie | 🟡 |
| **U39** | **1** | `[U21]` | #1920 aperta | 🔴 bloccata da una seduta, **come U25** — ma ne condivide l'allestimento: vedi in coda alla §4 |
| altre 12 sedute | 1–3 ciascuna | — | varie | — |

⚠️ **`U42` e `U43` sono cambiate, e non di poco**: `U43` dichiarava **dieci** voci aperte e ne misura
**sette**. 🔑 **`U18` esce dal gruppo delle quattro** perché questa passata le ha aggiunto
`PIE-V01-SHIELD`, e con `unblocked_by: []` è la **seconda** seduta del file che non attende nulla — un
fatto che la riga *«U18 · U19 · U22, varie»* nascondeva mettendola fra due sedute che invece attendono.

🔑 **Due sedute coprono 28 delle 69 voci schedulate.** Le altre diciotto si dividono il resto, molte con una
voce sola: l'ordine non è una preferenza, è dove una sessione produce venti verdetti invece di uno.

## 3. Come si avvia uno scenario, e la trappola che lo rende inutile

Il meccanismo esiste ed è in `PIE-TEST-CONSOLE`:

- `rt.Test.List` — elenca gli scenari registrati. ⚠️ **Il numero si rimisura, non si cita**: questa riga ha
  detto «4», poi «8», poi «9», e al 2026-08-17 ne contava **78**.
- `rt.Test.Run <ScenarioId>` — esegue lo scenario **nella partita in corso**.
- `rt.Test.DumpResult` — ristampa l'ultimo `result.json`.

🔴 **La trappola, dichiarata nel registro e non teorica:**

> *«eseguire uno scenario **sostituisce la mappa** e aggiunge unità alla partita in corso — è previsto (il
> runner riusa mappa e turn manager), ma dopo conviene riavviare con `R`»*
>
> *«due esecuzioni consecutive dello stesso scenario **non sono confrontabili** senza `R` in mezzo»*

∴ con venti verifiche in fila, **dimenticare `R` una volta contamina tutti i verdetti successivi**. La
procedura è: `rt.Test.Run <id>` → guarda → **`R`** → prossimo. Senza eccezioni, anche quando sembra che non
serva.

⚠️ E l'esito dei comandi si legge nell'**Output Log**: è il medium legittimo, deciso dall'autore il
2026-08-16. L'overlay della console in PIE scorre via.

## 4. L'ordine consigliato

### Passo 1 — U42, ventuno voci in **undici Play**, nessun prerequisito

Le voci: `PIE-VIS-ICE`, `-WETFIRE`, `-KO`, `-CHARGE`, `-ROUGH`, `-COMBO`, `-COORD`, `-FALLBACK`, `-SMOKE`,
`-PHASES`, `-LEVEL`, `-COVER`, `-DOOR`, `-HIGH`, `-HIGHCOVER`, `-GUARD`, `-BRACE`, `-AREAGUARD`, più
`PIE-ACC-GUARDBRACE` e `PIE-ACC-ENVIRONMENT`.

✅ Tutte e venti **citano il proprio scenario** nella riga di registro: nessuna richiede di indovinare
cosa allestire — verificato anche sulla ventesima, che dichiara `Visual.Environment.Acceptance` e il
percorso del suo file.

➕ **La ventesima è `PIE-ACC-ENVIRONMENT`, ed è arrivata dopo che questo file era stato scritto**: la
voce ombrello del composito d'ambiente, entrata in `main` lo stesso 2026-09-03. Il testo qui sopra diceva
*diciannove* ed elencava diciannove nomi: era corretto stamattina.

✅ **La guida è stata allineata il 2026-09-03, e il ⛔ che stava qui è superato.**
[`guida-seduta-u42-corpus-visual.md`](../../technical/runbooks/guida-seduta-u42-corpus-visual.md) dice ora
**ventuno** voci, nomina `PIE-ACC-ENVIRONMENT` e `PIE-ACC-MAP`, e porta in testa alla §3 la sezione che
cambia la resa della seduta: **tre Play coprono tredici voci**.

🔑 **La seduta non è ventuno aperture: è UNDICI.** I tre compositi di acceptance —
`Visual.Environment.Acceptance` (6 voci), `Visual.Map.Acceptance` (4) e
`Visual.Combat.GuardVsBraceUnderSmallHits` (3) — ne coprono tredici; le altre otto (`-KO`, `-CHARGE`,
`-COORD`, `-FALLBACK`, `-PHASES`, `-LEVEL`, `-HIGHCOVER`, `-AREAGUARD`) hanno un Play ciascuna.

⚠️ **Un composito toglie l'allestimento, non il giudizio**, e chiede *più* attenzione per Play non meno:
la colonna «falsificata se» della guida va letta **prima** di premere Play. Se al terzo turno non ricordi
cosa stavi cercando, hai risparmiato un riavvio e perso un verdetto.

⚠️ **Prima di aprire l'editor va scritto, per ciascuna, cosa la falsifica.** Sono verifiche visive: senza un
criterio scritto prima, venti verifiche producono venti *«sembra ok»*, che non è un verdetto.

### Passo 2 — U43, sette voci

`unblocked_by: []`, ma la sua issue #151 è un'**epic aperta**. Il titolo dichiara che *«i due compositi
allestiscono già»* le voci, quindi il lavoro potrebbe essere solo di esecuzione — ⛔ **da verificare voce per
voce prima di convocarla**, non da assumere.

### Passo 3 — le 33 orfane

Non sono lavoro da implementare: sono lavoro da **schedulare**. Ognuna va assegnata a una seduta esistente o
a una nuova, e la decisione è di `editor-sessions.yaml`, che ne è l'owner.

⛔ **Non prima di U42**: assegnare trentatré voci richiede un'analisi, mentre U42 è pronta adesso.

### In coda a `U21` — `U39`, **una** voce, e chiude una issue

⚠️ **Non è un passo a sé, e la prima stesura di questa sezione lo chiamava «Passo 0» mettendolo
davanti a `U42`.** Era sbagliato in due modi: `U39` dichiara `unblocked_by: [U21]` — lo **stesso**
ostacolo per cui `U25` è marcata 🔴 due righe sopra — e `U42` invece non attende nulla. Un ordine che
mette una seduta bloccata prima di una libera non è un ordine di esecuzione.

Detto questo, quando `U21` si apre `U39` costa quasi nulla e rende molto:

`PIE-HEX-COORD-COSTO` è la sua unica voce aperta, e la gemella `PIE-HEX-COORD-LEGGIBILITA` è già ✅
(2026-09-01, confermata dall'autore su `L_DevSandbox`). **Metà del criterio è già misurata** — il
conteggio dei segmenti — e resta il giudizio che nessuna sonda headless può dare: navigare l'**arena
piena** (raggio 50, 7 651 celle) e dire se il viewport regge, anche trascinando il pennello.

🔑 **Chiuderla chiude l'intera seduta e con essa**
[#1920](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1920), la cui implementazione è già
tutta in `main` — runtime, guscio editor e voci di registro. Resta aperta **solo** per questo verdetto a
schermo.

🔑 **E non chiede una propria apertura**: `shares_setup_with: [U21, U22, U25, U26]`, e di quel gruppo
è la meno esigente — nessuno strumento attivo. Si esegue **nella stessa apertura**.

⚠️ **Perché fin qui non compariva in questo file**: la tabella della §2 raggruppava le sedute da
una-tre voci in *«altre 16»*, e una seduta a **una** voce sparisce nel gruppo che l'ordinamento per resa
mette per ultimo. Ordinare per numero di verdetti nasconde chi ne produce pochi **a costo quasi nullo** —
il che resta vero, ed è il motivo per cui ha una riga propria nella tabella, non un posto in cima alla fila.

## 5. Cosa questa roadmap NON dice

- ⛔ **Non dice che i venti verdetti saranno verdi.** Dice che sono *osservabili*. Uno scenario che gira
  headless prova che il gioco non si rompe, non che a schermo si veda ciò che deve vedersi — è precisamente
  la ragione per cui esiste una verifica PIE.
- ⛔ **Non copre le 21 voci con ostacolo dichiarato** — ⚠️ numero **non rimisurato**, vedi la §1: quelle
  hanno cause proprie, scritte nelle loro righe,
  e vanno lette una per una.
- ⛔ **Non tocca le sedute con `execution_lane: asset`** (U1, U8, U24, U28, U29, U30): producono `.uasset`, e
  sono un altro mestiere — Content Browser e Binary Asset Lease, non osservazione.
