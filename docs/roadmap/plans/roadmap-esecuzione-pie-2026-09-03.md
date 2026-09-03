# Roadmap — eseguire le PIE rimaste

> `CURRENT` · **Creato**: 2026-09-03 · **Owner**: questo file, fino a quando le sedute che elenca sono aperte.
> **Misurato su** `origin/main = 2eb4ace2`. Il registro degli esiti resta
> [`test-manuali-pie.md`](../../technical/test-manuali-pie.md); *quale voce e quando* resta
> [`editor-sessions.yaml`](../editor-sessions.yaml). Questo file dice **in che ordine** e **cosa manca**.

## 🔑 La risposta, prima di tutto il resto

La domanda era *«quante issue dobbiamo implementare per eseguire le PIE rimaste?»*.

Per la seduta più ricca — **U42, diciannove voci** — la risposta misurata è **zero**.

`unblocked_by: []`, e le sue quattro issue (`#231 #233 #1919 #2009`) sono **tutte CLOSED**. I 29 file di
`Scenarios/Visual/` esistono, e girano: `Scenario.EveryShippedScenarioRuns` e
`Scenario.ShippedScenariosAreValid` sono **`Success`** (run VALIDA del 2026-09-03, `168/168`).

⚠️ **Il collo di bottiglia non è l'implementazione: è che nessuno convoca la seduta.** Il titolo di U42 lo
dice da sé — *«Il corpus Visual, diciotto scenari che nessuna seduta convocava»*.

---

## 1. Lo stato, in numeri

| | |
|---|---|
| voci totali · aperte | **200** · **98** |
| aperte **schedulate** in una seduta | **68** |
| aperte **fuori da ogni seduta** | **30** |
| aperte che dichiarano un ostacolo | **21** |

⚠️ *«Senza ostacolo dichiarato»* **non** significa eseguibile: significa che la voce non dice di essere
bloccata. Trenta di esse non sono in nessuna sequenza, ed è la condizione che
`spec-tactical-designer.md` §9 descrive come *«tende a non essere mai eseguita»*.

## 2. Le sedute, in ordine di resa

| seduta | voci aperte | `unblocked_by` | issue | verdetto |
|---|---|---|---|---|
| **U42** | **19** | `[]` | #231 #233 #1919 #2009 — tutte **CLOSED** | 🟢 **eseguibile ora, zero issue** |
| **U43** | **10** | `[]` | #151 (EPIC aperta) | 🟡 da verificare voce per voce |
| U25 | 6 | `[U21]` | #1095 aperta | 🔴 bloccata da una seduta |
| U5 | 5 | `[M6.6, M6.7]` | — | 🔴 bloccata da due checkpoint |
| U18 · U19 · U22 | 4 ciascuna | — | varie | 🟡 |
| altre 16 sedute | 1–3 ciascuna | — | varie | — |

🔑 **Due sedute coprono 29 delle 68 voci schedulate.** Le altre ventuno si dividono il resto, molte con una
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

∴ con diciannove scenari in fila, **dimenticare `R` una volta contamina tutti i verdetti successivi**. La
procedura è: `rt.Test.Run <id>` → guarda → **`R`** → prossimo. Senza eccezioni, anche quando sembra che non
serva.

⚠️ E l'esito dei comandi si legge nell'**Output Log**: è il medium legittimo, deciso dall'autore il
2026-08-16. L'overlay della console in PIE scorre via.

## 4. L'ordine consigliato

### Passo 1 — U42, diciannove voci, nessun prerequisito

Le voci: `PIE-VIS-ICE`, `-WETFIRE`, `-KO`, `-CHARGE`, `-ROUGH`, `-COMBO`, `-COORD`, `-FALLBACK`, `-SMOKE`,
`-PHASES`, `-LEVEL`, `-COVER`, `-DOOR`, `-HIGH`, `-HIGHCOVER`, `-GUARD`, `-BRACE`, `-AREAGUARD`, più
`PIE-ACC-GUARDBRACE`.

✅ Tutte e diciannove **citano il proprio scenario** nella riga di registro: nessuna richiede di indovinare
cosa allestire.

⚠️ **Prima di aprire l'editor va scritto, per ciascuna, cosa la falsifica.** Sono verifiche visive: senza un
criterio scritto prima, diciannove scenari producono diciannove *«sembra ok»*, che non è un verdetto.

### Passo 2 — U43, dieci voci

`unblocked_by: []`, ma la sua issue #151 è un'**epic aperta**. Il titolo dichiara che *«i due compositi
allestiscono già»* le voci, quindi il lavoro potrebbe essere solo di esecuzione — ⛔ **da verificare voce per
voce prima di convocarla**, non da assumere.

### Passo 3 — le 30 orfane

Non sono lavoro da implementare: sono lavoro da **schedulare**. Ognuna va assegnata a una seduta esistente o
a una nuova, e la decisione è di `editor-sessions.yaml`, che ne è l'owner.

⛔ **Non prima di U42**: assegnare trenta voci richiede un'analisi, mentre U42 è pronta adesso.

## 5. Cosa questa roadmap NON dice

- ⛔ **Non dice che i diciannove verdetti saranno verdi.** Dice che sono *osservabili*. Uno scenario che gira
  headless prova che il gioco non si rompe, non che a schermo si veda ciò che deve vedersi — è precisamente
  la ragione per cui esiste una verifica PIE.
- ⛔ **Non copre le 21 voci con ostacolo dichiarato**: quelle hanno cause proprie, scritte nelle loro righe,
  e vanno lette una per una.
- ⛔ **Non tocca le sedute con `execution_lane: asset`** (U1, U8, U24, U28, U29, U30): producono `.uasset`, e
  sono un altro mestiere — Content Browser e Binary Asset Lease, non osservazione.
