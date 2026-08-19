# Guida — costruire i `WBP_RT_*` del frontend

> **A chi serve**: a chi apre l'editor per fare il lavoro che una sessione non può fare (**D-139**: i
> binari sono human-first). Il C++ di CP 46.1 e CP 46.2 è in `main` e testato; qui c'è **cosa costruire e
> in che ordine**.
> **Gemella di** [`guida-screen-hud-umg.md`](guida-screen-hud-umg.md), che CP 11.7 ha scritto per lo
> stesso motivo. La differenza: quella è l'HUD **in-match**, questa è ciò che vive **prima e dopo** la
> partita.
> **La spec dice *cosa*** — [`spec-frontend-navigazione.md`](../architecture/spec-frontend-navigazione.md) — **questa
> guida dice *come***.

---

## 1. Cosa esiste già, e cosa manca davvero

| | Stato |
|---|---|
| Navigation controller (`URTFrontendNavigator`) | ✅ in `main`, 17 test |
| Stack, back stack, modali (`FRTScreenStack`) | ✅ |
| Fasi di caricamento (`ERTLoadPhase`) prodotte da `ARTGameMode` | ✅ |
| Esiti d'avvio (`ERTStartupOutcome`, 9 valori) + rapporto | ✅ 15 test |
| Classi base dei tre widget | ✅ |
| **I `WBP_RT_*`** | ⏳ **questa guida** |
| **Chi chiama `InitializeFrontend`** | ⏳ **e non esiste ancora** |

🔴 **Il secondo ⏳ è il punto che sorprende, e va detto subito.** Misurato:

```sh
grep -rn "InitializeFrontend\|RegisterScreen\|GetStartupReport" Source/
# → solo dichiarazioni e TEST. Nessun chiamante in gioco.
```

Il navigatore esiste e funziona, ma **nessuno lo avvia**. Non è una dimenticanza: CP 46.1 e 46.2 hanno
consegnato il meccanismo, e l'aggancio appartiene a **CP 46.3** (`#938`, il Main Menu) — è lì che il gioco
comincia ad avere una radice. Finché non c'è, i Blueprint che costruisci qui si possono provare **solo a
mano** in PIE, chiamando le funzioni da un livello di prova.

∴ **l'ordine consigliato è quello del §2**, e la ragione è questa: i primi due widget si possono verificare
subito, il terzo no.

---

## 2. L'ordine, e perché

| # | Blueprint | Classe base | Verificabile subito? |
|---|---|---|---|
| 1 | `WBP_RT_FallbackBanner` | `URTFallbackBannerWidgetBase` | ✅ **sì** — il dato lo produce già `ARTGameMode` |
| 2 | `WBP_RT_ErrorModal` | `URTErrorModalWidgetBase` | ✅ sì, forzando un formato invalido |
| 3 | `WBP_RT_LoadingScreen` | `URTLoadingScreenWidgetBase` | 🟡 il dato c'è, ma l'allestimento è **istantaneo**: vedi §6 |
| 4 | `WBP_RT_FrontendRoot` + `WBP_RT_ModalLayer` | `UUserWidget` | ⏳ ha senso con CP 46.3 |

**Comincia dal banner.** È il solo che mostra qualcosa di vero *oggi*, in una partita normale: se apri PIE
adesso, il gioco sta girando sull'arena di prova con un formato che nessuno ha dichiarato — e nessuno lo
vede. Il banner è la prima cosa che rende visibile la riserva di `G13`.

---

## 3. Dove vanno gli asset

Da [`convenzioni-contenuti-ue.md`](../tooling/convenzioni-contenuti-ue.md) §3:

```text
/Game/RT/UI/Framework/          ← i widget del frontend
/Game/RT/UI/HUD/                ← l'HUD in-match (CP 11.7), NON qui
```

⚠️ `UI/Framework/` non esiste ancora e va creata: la convenzione dice *«non creare preventivamente le
directory vuote — una directory nasce quando contiene almeno un asset reale»*. Nasce col primo widget.

Prefisso **`WBP_RT_`**, non `WBP_`. È la decisione di CP 11.7, e la ragione è pratica: su un `.uasset` il
rename costa più che scriverlo giusto.

---

## 4. `WBP_RT_FallbackBanner` — comincia da qui

**Classe padre**: `URTFallbackBannerWidgetBase`.

### Cosa leggere

| Funzione | Tipo | Uso |
|---|---|---|
| `HasAnything()` | `bool` | la **visibilità** del banner: se è falso non disegnare nulla |
| `GetLines()` | `TArray<FText>` | una riga per condizione, **tutte** |

### Layout minimo

```text
[ Border (colore d'avviso, opacità bassa) ]
  └── VerticalBox
        └── (una TextBlock per elemento di GetLines())
```

Un `ListView` o un `VerticalBox` popolato in `NativeConstruct`: entrambi vanno bene. **Non** un singolo
`TextBlock` con le righe concatenate — perché il numero di righe conta (§5).

### Come agganciarlo

`SetFromReport(GameMode->GetStartupReport())` una volta, dopo l'allestimento. In attesa di CP 46.3 puoi
chiamarlo da un `BP_GameMode` derivato o da un livello di prova.

### ✅ Come verificare che funzioni

Apri PIE su un livello **senza `MapAsset` popolato**. Devi vedere **due righe**:

```text
⚠ Il livello non porta una mappa esagonale: arena di ripiego (arena di ripiego r=N)
⚠ Nessun TurnManager: il formato non è stato applicato (Format.Skirmish2v2)
```

Se ne vedi **una sola**, il `VerticalBox` sta mostrando solo il primo elemento — ed è esattamente il
difetto che le note sono una lista per evitare.

---

## 5. Le tre regole che il codice non può importi

Valgono per la superficie C++; un Blueprint può sempre aggirarle, e nessun gate lo impedisce perché i
`.uasset` non sono versionati. Sono qui perché siano una scelta consapevole.

### 5.1 🔴 Non scrivere il testo di un errore nel Blueprint

Il testo viene da `DescribeOutcome()`. Se lo riscrivi in un `TextBlock` con `Set Text (Text)`, la UI
diventa la **sorgente della spiegazione** — libera di divergere dal log a parità di causa.

È il difetto per cui esiste l'ottavo vocabolario di reason del progetto: se il motivo fosse una stringa
libera, non ci sarebbe stato bisogno di istruirlo.

### 5.2 Non decidere nel Blueprint se un esito è fatale

`IsFatal()` è nella libreria ed è **l'unico posto** in cui la divisione modale/banner è scritta. Un
`Switch on ERTStartupOutcome` in Blueprint che ridecidesse quali valori sono fatali sarebbe una seconda
copia della stessa regola — e le due divergerebbero al primo esito nuovo.

Il modale si arma con `ShowFor(Note)`, che **rifiuta da sé** le note degradate.

### 5.3 Non navigare dal widget

Nessun widget chiama `CreateWidget` / `AddToViewport` / `RemoveFromParent`. L'unico owner del flow è
`URTFrontendNavigator`, ed è un criterio **verificabile**:

```sh
grep -rn "AddToViewport\|RemoveFromParent\|CreateWidget" Source/
# → 3 sole chiamate, tutte in RTFrontendNavigator.cpp
```

Un Blueprint non compare in quel grep — quindi la regola qui è **disciplina**, non un gate. Se un widget
deve cambiare schermata, chiama il **navigatore**: mai `CreateWidget`/`AddToViewport` da sé.

> 🔴 **Aggiornato il 2026-08-18 — e per il modale d'errore la riga precedente era una trappola.**
> Diceva *«chiama `PushScreen`/`PopScreen` sul navigatore»*, e per il `BACK` di `WBP_RT_ErrorModal`
> **quella è la cosa sbagliata**. `PopScreen` è giusto solo durante il loading; a partita viva serve
> `ReturnMain`, o resta una partita viva **sotto** il menu. Un Blueprint che scegliesse fra i due sarebbe
> una seconda autorità sulla navigazione, e la scelta finirebbe dentro un `.uasset` dove nessun test la
> vede.
>
> Chiama **`BackFromError(GetPhaseWhenArmed())`** e la scelta resta nel navigatore. Vedi §5.4.

### 5.4 Il `BACK` del modale d'errore: una chiamata sola

**Nodo da collegare** — `WBP_RT_ErrorModal`, evento `OnClicked` del pulsante `BACK`:

```
[Button BACK · OnClicked]
        │
        ├─ Get Game Instance Subsystem (RT Frontend Navigator)
        │
        └─ BackFromError
              PhaseWhenArmed ← GetPhaseWhenArmed()      ← su SELF, non una costante
```

Tre modi di sbagliarlo, tutti plausibili e nessuno segnalato da un errore di compilazione:

| Sbagliato | Perché |
|---|---|
| `PopScreen()` diretto | funziona durante il loading e **lascia una partita viva sotto il menu** quando l'errore arriva a partita avviata |
| `ReturnMain()` diretto | smonta anche quando non c'era niente da smontare — e nasconde che i casi fossero due |
| `BackFromError(Ready)` con la fase **costante** | riporta la decisione dentro la UI per un'altra strada: il nodo sembra giusto e la regola non è più nel navigatore |

⚠️ **Il modale si chiude da sé**: `BackFromError` fa `CloseModal` prima del pop. Non aggiungere un
`CloseModal` nel Blueprint — un doppio `CloseModal` risponde `NoModalOpen`, che è distinto da `Ok` proprio
perché un doppio click non mangi una schermata.

⚠️ **Non lo vedrai funzionare in PIE**, e non è un difetto: nessuno chiama ancora `InitializeFrontend`
(è di CP 46.3, `#938`). La verifica di questa seduta è **nel grafo**, non a schermo.

---

## 6. `WBP_RT_LoadingScreen` — e perché non lo vedrai

**Classe padre**: `URTLoadingScreenWidgetBase`.

| Funzione | Uso |
|---|---|
| `IsLoading()` | visibilità: falso su `Idle` **e** su `Ready` |
| `GetPhaseText()` | la riga: vuota nelle stesse due fasi |
| `GetPhase()` | se vuoi differenziare la grafica per fase |

⚠️ **L'allestimento oggi è istantaneo**: `SetupHexMatch` attraversa `Map → Scenario → Bots → Ready` dentro
un solo `BeginPlay`, senza cedere un frame. Un widget che leggesse la fase a ogni tick vedrebbe **solo
`Ready`**.

Non è un difetto da correggere adesso: il caricamento asincrono arriverà con `CP 46.4` (`Play` che avvia
davvero una mappa). Fino ad allora la schermata si prova **forzando la fase a mano** — `SetPhase(Map)` da
un pulsante di debug — e serve a verificare il layout, non il flusso.

∴ costruiscilo, ma non aspettarti di vederlo in una partita normale. È il motivo per cui sta **terzo**
nell'ordine.

---

## 7. Cosa NON costruire adesso

- **`WBP_RT_MainMenu`, `WBP_RT_ResultScreen`, `WBP_RT_PauseMenu`** — sono di `#938`, `#940`, `#941`, e
  hanno DoD propri. Costruirli ora significherebbe farlo senza la spec del loro checkpoint.
- **Un secondo root dell'HUD in-match.** `WBP_RT_TacticalHUD` esiste già ed è quello: `Frontend != In-Match
  HUD` vale in entrambi i versi.
- **Uno stile condiviso**, se non serve a queste tre schermate. `UI/Styles/` nasce quando c'è qualcosa da
  condividere, non prima.

---

## 8. Verifica, e cosa registrare

Quando i primi due widget disegnano:

1. **In PIE**, sul livello senza mappa d'autore: il banner mostra **due** righe (§4).
2. **Con un `MatchFormat` invalido** assegnato al GameMode: compare il **modale**, e il banner **no** —
   sono due forme per due casi, e il modale filtra da sé.
3. **In build Development**: il pulsante `DETAILS` c'è. In **Shipping** non c'è, e non perché il Blueprint
   lo nasconda: `GetDetail()` restituisce una stringa vuota perché il campo **non è compilato**.

⏳ **Poi proponi le voci PIE.** `PIE-V01-FRONTEND-NAV` e `-ERROR` non esistono — misurato:
`grep -c "PIE-V01-FRONTEND" docs/technical/test-manuali-pie.md` → **0**. Quel registro appartiene alla
track `playtest`, e la procedura è **proporle in handoff**, non scriverle.

⚠️ **Quando i widget esistono, il gate `runtime` di `RT-FEAT-UI-FRONTEND-SHELL` può passare da `partial`.**
Non farlo su «si vede che funziona»: è il precedente annotato nel registry per
`RT-FEAT-UI-TACTICAL-CAMERA`, e vale qui allo stesso modo.

---

## 9. In caso di dubbio

- **Cosa** deve fare una schermata → [`spec-frontend-navigazione.md`](../architecture/spec-frontend-navigazione.md)
- **Dove** va un asset → [`convenzioni-contenuti-ue.md`](../tooling/convenzioni-contenuti-ue.md)
- **Come si comporta** l'HUD in-match → [`guida-screen-hud-umg.md`](guida-screen-hud-umg.md)
- **Perché** E46 esiste → [D-144](../../decisions/RT_PDR_00_Decision_Log.md)
