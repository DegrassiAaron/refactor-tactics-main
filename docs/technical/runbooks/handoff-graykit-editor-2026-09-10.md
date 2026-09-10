# EDITOR HANDOFF — Graykit procedurale, dal 2026-09-10

> `CURRENT` · **Stato**: consegnato, **nessuna voce eseguita** · **Data**: 2026-09-10
> **Branch**: `feat/graykit-procedural-pose` · **Commit del codice**: `27f43c4a`
> **Referto che lo produce**: [`../../roadmap/plans/graykit-procedural-spec-panel-2026-09-10.md`](../../roadmap/plans/graykit-procedural-spec-panel-2026-09-10.md)
> **Owner della presentazione**: [#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288) (`CP E21.2`) · **Epic**: [#286](https://github.com/DegrassiAaron/refactor-tactics-main/issues/286)

---

## Prima di cominciare

⚠️ **Il motore è uno per macchina, e non c'è più un lease che lo serializzi** (`AGENTS.md` §11). Prima di
aprire l'Editor:

```powershell
Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, CommandLine
```

La `CommandLine` porta il `.uproject` — quindi **quale clone** — e `-abslog` — quindi **quale sessione**. Un
conteggio di processi non lo dice. Al 2026-09-10 su questa macchina lavoravano almeno altri due cloni
(`-dev`, `-designer`).

⛔ **L'authoring asset appartiene al clone principale**, quello che ospita il bridge MCP. Un worktree non ha i
file gitignorati: salvare un asset i cui riferimenti duri leggono `None` **li azzera**, senza errore.

🔑 **Precondizione comune a tutte le voci**: il codice di `27f43c4a` è in `Source/` ma **non ha un
consumatore**. `E-01` è quella che lo crea; le altre dipendono da lei.

---

## E-01 — I due bracci e il ponte fra posa e componenti

| Campo | Valore |
|---|---|
| **Related issue** | #2880 · epic #2879 · dipende da #288 |
| **Map/scenario** | `L_GrayKitPlayground` (esiste, `Content/RT/Maps/Dev/`) |
| **Asset interessato** | `BP_Unit` (o la classe base equivalente) — **nessun asset nuovo** |
| **Setup richiesto** | branch `feat/graykit-procedural-pose` compilato; `27f43c4a` presente |
| **Operazione editor** | Aggiungere a `ARTUnit` due `UStaticMeshComponent` figli di `SceneRoot`, nominati `LeftArm` e `RightArm`, posizionati sugli anchor che `URTGraykitLibrary::AnchorOffset` già calcola (`LeftHand` → `(0, -45, 31.5)`, `RightHand` → `(0, +45, 31.5)` per il cilindro da 90/45). ⚠️ **Se i componenti si aggiungono in C++ invece che in Blueprint, questa voce non è Editor** — è DEV, e va fatta lì: si decida prima quale delle due. |
| **MCP operation** | Nessuna se si sceglie il C++. Se Blueprint: `AssetTools` sul clone principale |
| **PIE richiesto** | **Sì** — un'unità che si muove, per vedere il braccio oscillare |
| **Controllo visuale** | I bracci restano attaccati al corpo durante `Move`; nessuna compenetrazione col cilindro; l'oscillazione è leggibile a distanza di camera tattica |
| **Log da verificare** | `LogRefactorTactics` per warning di componenti mancanti |
| **Risultato atteso** | La posa valutata si vede addosso all'unità |
| **Blocca DEV** | **Sì** — finché non c'è un consumatore, `27f43c4a` è codice senza uso |

---

## E-02 — Dimensione del cilindro rispetto all'esagono

| Campo | Valore |
|---|---|
| **Related issue** | #1095 (Seduta U25, il volume di posa della cella) · #1871 |
| **Map/scenario** | `L_GrayKitPlayground`, Station 01 |
| **Asset interessato** | nessuno da modificare — è una **misura** |
| **Setup richiesto** | una cella graybox posata e un'unità sopra |
| **Operazione editor** | Confrontare il diametro del cilindro (`2 × 45 = 90 uu`) con l'inraggio dell'esagono. Se divergono, il numero da correggere è `ARTUnit::UnitHalfHeight` / il raggio, **e il test `RefactorTactics.Graykit.Anchor` diventa rosso**: è la sorveglianza voluta, non un guasto |
| **MCP operation** | `SlateInspectorToolset` per leggere le dimensioni senza aprire il Details a mano |
| **PIE richiesto** | No — basta il viewport |
| **Controllo visuale** | Il cilindro sta dentro la cella con margine visibile; due unità adiacenti non si toccano |
| **Log da verificare** | — |
| **Risultato atteso** | Un numero, scritto nell'issue. Non un aggiustamento a occhio |
| **Blocca DEV** | No |

---

## E-03 — Move contro Run: la validazione che i test non possono dare

| Campo | Valore |
|---|---|
| **Related issue** | #288 · #2881 · epic #2879 |
| **Map/scenario** | `L_GrayKitPlayground` o una scena 1v1 con percorso lungo |
| **Asset interessato** | nessuno |
| **Setup richiesto** | `E-01` completata |
| **Operazione editor** | Far percorrere lo stesso tragitto a due unità, una con `ERTGraykitLocomotionStyle::Normal` e una con `Run`. **Guardarle senza HUD.** |
| **MCP operation** | `AutomationTestToolset` no — questa è una misura di occhio |
| **PIE richiesto** | **Sì** |
| **Controllo visuale** | 🔴 **Il criterio del mandato**: sono distinguibili *senza HUD*? Se la risposta è no, i numeri di `DescriptorForStyle` vanno cambiati — sono un punto di partenza, non un accordo di design, e il referto lo dichiara |
| **Log da verificare** | — |
| **Risultato atteso** | Sì/no motivato, e se no i valori proposti |
| **Blocca DEV** | No — ma è **l'unico gate che può chiudere `GK-03`** sul piano estetico |

---

## E-04 — Material instance del graykit

| Campo | Valore |
|---|---|
| **Related issue** | #1714 (le sei mesh uscivano con lo slot vuoto — precedente diretto) |
| **Map/scenario** | `L_GrayKitPlayground` |
| **Asset interessato** | `MI_Graykit_Body`, `MI_Graykit_Arm` (nuovi) sotto `Content/RT/World/Graybox/Materials/`, da `M_Graybox_Master` |
| **Setup richiesto** | clone principale, bridge MCP acceso |
| **Operazione editor** | Creare le due Material Instance e assegnarle. ⚠️ `.gitignore:192` porta `!Content/RT/World/Graybox/**/*.uasset`: sono committabili, quindi vanno committate |
| **MCP operation** | `AssetTools` (`write_file`) — **solo dal clone principale** |
| **PIE richiesto** | No |
| **Controllo visuale** | Corpo e bracci si distinguono dal terreno graybox e fra loro; il colore squadra resta leggibile |
| **Log da verificare** | Warning di slot materiale vuoto |
| **Risultato atteso** | Due `.uasset` committati, nessuno slot vuoto |
| **Blocca DEV** | No |

---

## E-05 — Sei-otto unità simultanee

| Campo | Valore |
|---|---|
| **Related issue** | #221 (`E17` · Validazione di stress 4v4) |
| **Map/scenario** | scenario 4v4 |
| **Setup richiesto** | `E-01` e `E-04` completate |
| **Operazione editor** | Risolvere un turno con tutte le unità in movimento |
| **MCP operation** | — |
| **PIE richiesto** | **Sì** |
| **Controllo visuale** | La scena resta leggibile; nessuna posa fa perdere quale unità sta dove; la **base** continua a dire la verità tattica anche quando il corpo si inclina |
| **Log da verificare** | — |
| **Risultato atteso** | Verdetto di leggibilità |
| **Blocca DEV** | No |
| **Nota** | ⛔ **Questa non è una misura di performance.** Se serve anche quella, il motore dev'essere **libero** — è l'unico caso in cui la contesa fra cloni morde davvero (`AGENTS.md` §9) |

---

## Ciò che NON è in questo handoff, e perché

- ⛔ **Le modalità `Final`/`Graykit`/`Hybrid`/`Debug`**: sono `GKPROC-1` in
  [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md). Contraddicono la decisione d'autore del 2026-09-05
  (chiusura #2449). **Non allestire una scena per provarle**: renderebbe la decisione per effetto collaterale.
- ⛔ **Le geometrie tattiche** (`Line`, `Cone`, `Disc`, `Ring`, `Beam`): hanno già un owner,
  `FRTOverlayArea`. Se servono a schermo, il lavoro appartiene a chi possiede l'overlay.
- ⛔ **`Trail` e `Ghost`**: non sono pose, sono emissioni con stato. Nessun owner assegnato.
