# Consolidamento HUD / Icon System v0.1→v1.0 (handoff Google Doc) — spec panel

> `CURRENT` · **Stato**: revisione chiusa · **Data**: 2026-08-30
> **HEAD della revisione**: `71261937` · branch `docs/hud-icon-consolidamento-1735` · base `origin/main`
> **Sorgente revisionata**: *RefactorTactics — HUD Icon Design System + Epic Roadmap v0.1→v1.0*,
> Google Doc `1fHAsABR8xCzp80_p497ZHBt0F2dGphr-KKIaqMsoczQ`.
> 🔴 **Il documento NON è stato letto**: `WebFetch` restituisce **HTTP 401 Unauthorized**, e non esiste in
> questo repository una copia scaricata. La revisione è condotta sul **riassunto strutturato nel prompt**
> (FASI 1–10), che è l'unica forma del contenuto effettivamente disponibile. Ogni verdetto qui sotto vale
> per quel riassunto; se il documento originale dice altro, questa revisione non lo sa.
> **Regola applicata**: un handoff AI è l'ultima fonte della gerarchia ([`CLAUDE.md`](../../../CLAUDE.md) §7).
> Dove contraddice un ADR, una `D-nnn`, un gate o un fatto misurabile, prevale il repository.

---

## 1. Il verdetto in una riga

**Il consolidamento richiesto è già stato fatto, e il repository lo ha fatto più preciso.** Il documento
propone di costruire un catalogo semantico, un validator, una grammatica della certezza e un piano di
accessibilità: tutti e quattro **esistono, sono in codice, hanno test verdi e un owner documentale**. Ciò che
il documento chiama «da consolidare» è, misurato, **`D-031` chiusa il 2026-08-08 e `#220` mergiata il
2026-08-28**.

∴ La consegna corretta non è costruire, è **non costruire** — e correggere le due righe del repository che
sono invecchiate mentre il lavoro procedeva.

---

## 2. Ciò che è stato misurato

| Affermazione del documento | Esito | Misura |
|---|---|---|
| Serve una pipeline `IconId → Icon Catalog → asset → consumer` | ✅ **esiste da `D-031`** | `URTIconCatalogData` (`UI/RTIconCatalogData.h`) + `URTIconLibrary` (`UI/RTIconLibrary.h`). Decisione **Consolidata**, owner **E20** |
| «NON creare classi duplicate se esiste già un equivalente» | ✅ **premessa sana, e vincolante qui** | l'equivalente esiste per **tutte** le classi proposte |
| Il validator deve rilevare duplicati, ID mancanti, asset nulli, categoria invalida | ✅ **implementato** | `ValidateIconCatalog` rifiuta: `IconId` assente · senza prefisso `UI.Icon.` · duplicato · categoria ≠ segmento nell'ID · asset nullo · missing-icon non impostato |
| Serve una fallback / missing-icon policy | ✅ **implementata** | `FRTIconResolution{Asset, bResolved}`: chiave sconosciuta → missing-icon + `bResolved=false` + warning che nomina chiave **e** consumer |
| Inventario v0.1 = **24** icone (batch AI) | 🟡 **sottoinsieme: le chiavi richieste sono 61** | `URTIconLibrary::RequiredIconIds()` → **61**; `DA_IconCatalog` esiste con **61 voci** e **62 texture** in `Content/RT/UI/Icons/` (62 = 61 + `MissingIcon`). Il generatore produce **121 icone + 9 cornici** |
| «I widget non devono hardcodare texture» | ✅ **regola attiva con gate** | `ScreenHud.WidgetApiExposesNoTexture` itera per reflection le 7 classi C++ dei widget e fallisce su una `UTexture2D`. ⚠️ **non vede i Blueprint** — dichiarato nel DoD di `#220` |
| Griglia canonica 24×24, stroke ~2 px, variante 16 px, 2.5–3 px a 32 px | ✅ **già scritta, alla lettera** | [`01-principi.md`](../../research/design/icon/visual-language/01-principi.md) righe 59–67: canvas master `24×24 px`, stroke ~2 px a 24 px, variante dedicata a 16 px, 2.5–3 px ottici a 32 px |
| Certainty = `Confirmed` · `Predicted` · `Uncertain`, distinguibili senza colore | ✅ **e il repository è più fine** | [`05-certainty-states.md`](../../research/design/icon/visual-language/05-certainty-states.md) §1 separa **tre assi indipendenti** — `Validity`, `Certainty`, `Knowledge` — e vieta di collassarli su un canale. Il documento ne nomina uno |
| Accessibilità: grayscale · Default Accessible · CVD · High Contrast · UI scaling | ✅ **le quattro rese esistono** | [`06-accessibilita.md`](../../research/design/icon/visual-language/06-accessibilita.md) §2, con la regola di review «se non sono distinguibili in grayscale, **non si corregge col colore**» |
| Privacy: «server canonical → sanitizzazione → solo dati autorizzati», mai «replica tutto e nascondi in UI» | ✅ **è esattamente l'architettura in codice** | `URTIntentPrivacyLibrary::FilterForTeam` costruisce il DTO spedito al client. Test: `Reactions.IntentNotVisibleToEnemy`, `UI.NoEnemyIntentExposed`, `Reactions.IntentViewSkipsDeadAndKeepsOrder` |
| Roadmap `v0.5→v1.0` sotto **E40**–**E45** | ✅ **vero, ed è la ladder canonica** | `D-136` la canonizza; `E40`–`E45` esistono su GitHub. Il documento **non** propone una ladder parallela: coincide |
| «NON creare Epic generiche tipo `HUD Icons v1.0`» | ✅ **e c'è già il precedente** | `D-138` e `D-153` hanno **già respinto** due ladder proposte da handoff esterni. Terza applicazione dello stesso principio |
| E11 · E20 · E21 · E25 sono gli owner | ✅ **9 issue su 9 esistono e sono OPEN** | `#217` `#219` `#220` `#637` (v0.1) · `#265` `#266` `#267` `#268` `#269` (post-v0.1) |
| «1024×1024 AI generations sono sorgenti/reference» | 🟡 **vero in generale, ma non descrive questa pipeline** | la produzione reale non parte da immagini AI: `tools/hud-assets/generate_hud_assets.py` rasterizza **SVG deterministici** via `cairosvg` a `16/20/24/32/48 px`, con i gate `T1 T3 T5 T6 T7 T8 T9` |
| Concetti da non perdere: Wait, Height, Critical, Friendly Fire, … | ✅ **nessuno è a rischio** | non sono chiavi richieste perché **non hanno un consumer**; il documento stesso lo prescrive («il consumer reale governa la necessità dell'asset») ed è la regola che `RequiredIconIds()` implementa |

---

## 3. Il panel

### 📚 WIEGERS — qualità del requisito

> ⚠️ **MAGGIORE.** Il documento è scritto in **modo imperativo su un sistema che esiste**: «consolida»,
> «adatta i nomi alle classi realmente presenti», «non creare classi duplicate». Sono istruzioni corrette,
> ma la loro applicazione produce **zero cambiamenti** — e un requisito che non discrimina fra «fatto» e
> «da fare» non è verificabile.
>
> 📝 **Il difetto è di premessa, non di contenuto**: manca uno **stato di partenza misurato**. La FASE 1 lo
> chiede al lettore (*«produci prima un rapporto»*), il che è la mossa giusta — ma significa che il
> documento **non sa** cosa esiste, e quindi non può dire cosa manca. È il motivo per cui l'inventario
> propone 24 chiavi dove il gioco ne pretende 61.
>
> ✅ **Ciò che regge**: la DoD in 17 punti è la parte migliore del documento. È testabile quasi ovunque, e
> il punto 17 («non è stato introdotto un secondo catalogo parallelo») è **esattamente** il rischio che
> questa revisione doveva scongiurare.

### 📊 FOWLER — architettura e confini

> ✅ La pipeline proposta — *gameplay/ViewModel sanitizzato → IconId → catalogo → asset → consumer* — è la
> stessa che `D-031` ha scelto, con la stessa motivazione: la semantica attraversa il confine, non il
> percorso di un asset.
>
> ⚠️ **Un punto in cui il documento è meno preciso del codice.** Dice «widget non calcolano certainty».
> Vero, ma il repository ha scoperto che **`certainty` non è un asse solo**: `Validity` (l'azione si può
> fare?), `Certainty` (quanto è certo?) e `Knowledge` (cosa la squadra sa?) sono ortogonali, e
> `ERTTargetKnowledge::CellOnly` significa che si può mirare alla **cella** e mai all'unità. Un'icona che
> collassa i tre assi **mente al giocatore** su un bersaglio che il gioco rifiuterà. Applicare il documento
> alla lettera qui sarebbe una regressione.

### 🎲 TALEB — cosa può rompersi

> 🔴 **Il rischio non è che manchi qualcosa: è che il documento venga applicato.** Un consolidamento che
> «crea il catalogo» su un repository che ne ha già uno produce il **secondo catalogo** che la DoD §17
> vieta — e lo produrrebbe **eseguendo il documento**, non ignorandolo.
>
> ⚠️ **Il fallimento silenzioso da temere è l'inventario a 24.** Se qualcuno tratta quel batch come il
> catalogo canonico e riallinea `RequiredIconIds()`, **37 chiavi perdono la copertura** e il gate
> `IconCatalog.EveryKeyResolves` diventa verde su un catalogo più povero. Un test che passa perché è stato
> abbassato il requisito è peggio di un test rosso.
>
> ✅ **La difesa esiste già ed è strutturale**: `RequiredIconIds()` **deriva** le chiavi dai dati di gioco
> reali — catalogo azioni, tag `Status.`, roster — quindi non si può abbassare senza cancellare gameplay.

### 🕸️ MEADOWS — struttura del sistema

> 💡 **Questo è il terzo handoff esterno in 17 giorni che propone una ladder di release**, dopo quelli
> respinti da `D-138` (2026-08-14) e `D-153` (2026-08-17). Il pattern è il sistema che parla: fonti esterne
> generano roadmap perché **non vedono `RELEASE_ORDER`**, e ogni volta il repository paga una revisione per
> dire «esiste già».
>
> ✅ **Ma qui c'è una differenza che vale registrare**: questo documento **non propone numeri nuovi** — cita
> `E40`–`E45` corretti e dice esplicitamente di non riusarli. È il primo dei tre a essere **già allineato**.
> Il punto di leva non è un altro respingimento: è che il documento ha imparato la lezione dei due
> precedenti.

### ✏️ DOUMONT — chiarezza

> ⚠️ Il documento è lungo e ripete la stessa istruzione in tre forme («non creare una seconda source of
> truth» · «nessun secondo catalogo» · DoD §17). La ridondanza è **difensiva e appropriata** per un handoff
> AI, ma sposta il peso: 10 fasi di prescrizione per **zero** deliverable mancanti.
>
> 📝 Il messaggio che serviva è di una riga: *«prima misura cosa esiste; probabilmente esiste tutto»*.

---

## 4. Le due righe invecchiate (l'unico difetto reale trovato)

`docs/roadmap/roadmap-v0.1.md` §2.1, riga **E20**, dice:

> `5 test IconCatalog.* … ⏳ i widget non consumano ancora il catalogo`

**Entrambe le metà sono false al 2026-08-30.**

| | Dichiarato | Misurato | Come |
|---|---|---|---|
| Test | **5** | **6** | `grep -c IMPLEMENT_SIMPLE_AUTOMATION_TEST Tests/RTIconCatalogTests.cpp` → 6. Il sesto è `V01CategoriesPopulated` |
| Consumo | «⏳ non ancora» | ✅ **dal 2026-08-28** | PR **#1556** (`feat/220-icon-catalog`) mergiata su `main` il 2026-08-28 18:03. `URTActionSlotWidget::SetAction` riceve il catalogo, `GetResolvedIcon()` chiama `ResolveIcon(ReceivedCatalog, GetIconId(), "ActionSlot")` |

⚠️ **`#220` resta legittimamente OPEN**, e non è una contraddizione: il suo DoD chiede
`ScreenHud.WidgetApiExposesNoTexture` verde **più** la voce `PIE-ICON-01`, che è una verifica **visiva in
editor** e non è stata eseguita. Ciò che è chiuso è il codice; ciò che resta è la conferma a schermo.

∴ La riga si corregge in questa sessione. **Non si tocca lo stato di `#220`.**

---

## 5. Inventario icone — reale, non proposto

Il catalogo canonico è **derivato**, non scritto: `URTIconLibrary::RequiredIconIds()` lo compone dai dati di
gioco. Ecco perché le 61 chiavi non si elencano a mano da nessuna parte, e perché elencarle qui sarebbe
creare la copia che invecchia.

| Categoria | Chiavi | Sorgente della derivazione | Consumer | Asset | Owner |
|---|---|---|---|---|---|
| `Phase` | **4** | `ERTMatchPhase`, solo le **volontarie** (`Prep` `Dash` `Blast` `Move`) | HUD di fase | ✅ | E20 |
| `Action` | **36** | `URTCatalogLibrary::GetCoreActionCatalog()` — si adegua da sola | `URTActionSlotWidget` | ✅ | E20 |
| `Status` | **11** | tag registrati sotto `Status.` in `RTGameplayTags.cpp` | HUD unità | ✅ | E20 |
| `Certainty` | **3** | ⚠️ **lista scritta a mano, e la ragione è documentata**: `ERTIntentCertainty` ha **4** valori, ma `Unknown = 0` significa «mai calcolato» e non ha una resa | rendering intenti | ✅ | E20 |
| `Identity` | **6** | 4 eroi da `GetHeroIds()` + `Ally` + `Enemy`. ⚠️ prefisso **tradotto** `Hero.` → `Identity.` | `ARTHUD` | ✅ | E20 |
| **Totale richiesto** | **61** | + `MissingIcon` (campo, non chiave) = **62 texture** | | ✅ `DA_IconCatalog` | E20 |
| Le altre **7** categorie | **0** — dichiarate, vuote | tassonomia di `D-031`; si popolano quando esiste un sistema che le consuma | — | — | **E25** |

> ✅ **Perché `EveryKeyResolves` non passa per costruzione**: l'insieme richiesto **non** è il contenuto del
> catalogo. Un tag `Status.` nuovo o un'azione nuova fanno **cadere** la copertura finché l'icona non
> esiste. È l'unico meccanismo che tiene vivo questo dato.

**Fuori dal set richiesto**, e legittimamente: **60 icone** generate in più — le ability dei quattro eroi
(skill bar, non copertura) e il censimento delle sette categorie di E25.

---

## 6. Governance — cosa NON si fa

| Proposta | Verdetto | Ragione |
|---|---|---|
| Nuova epic HUD/Icon | ⛔ **no** | `E11` `E20` `E21` `E25` possiedono il lavoro; `E40`–`E45` le release. Precedente: `D-138`, `D-153` |
| Nuovo catalogo o classe | ⛔ **no** | `URTIconCatalogData` esiste; sarebbe il secondo catalogo che la DoD §17 del documento stesso vieta |
| Nuova roadmap UI/Icon | ⛔ **no** | `RELEASE_ORDER` di `D-136` la copre; la ladder del documento vi **coincide** |
| Nuova `D-nnn` | ⛔ **no** | nessuna decisione nuova: la materia è `D-031`. Assegnare un ID per un consolidamento a zero delta è rumore nel Decision Log |
| Riallineare l'inventario a 24 | ⛔ **no** | abbasserebbe la copertura di **37** chiavi rendendo verde un gate più povero |
| Chiudere `#219` / `#220` | ⛔ **no** | il DoD di `#220` include `PIE-ICON-01`, verifica visiva **non eseguita**. Il documento avverte da solo di non chiudere per dichiarazione |
| Correggere la riga E20 di `roadmap-v0.1.md` | ✅ **sì** | due fatti misurati e obsoleti (§4) |

---

## 7. Gap reali ancora aperti

Solo ciò che è **misurato** come mancante, non ciò che il documento immagina:

1. 🔴 **Il gate anti-texture non vede i Blueprint.** `ScreenHud.WidgetApiExposesNoTexture` copre le 7 classi
   C++; una variabile `Texture2D` dentro un `WBP_RT_*` passerebbe. È **dichiarato** nel DoD di `#220`, con
   il rimedio già individuato: `RTMatchWidgetAssetTests.cpp` carica già le
   `UWidgetBlueprintGeneratedClass` dei sette `WBP_RT_*` e ne interroga l'albero. **Owner: `#220`.**
2. ⏳ **`PIE-ICON-01` non eseguita** — le icone si vedono e si distinguono a schermo. Nessun test headless
   può sostituirla. **Owner: `#220`.**
3. ⏳ **`#637`** — 10 decisioni aperte sulle categorie fuori dall'enum (`Effect`, `Stat`, `Gadget`,
   `Target`, `Module`, `Geometry`, `Weapon`, `Decision`, `Timing`, `Result`). Non blocca i checkpoint.
4. ⏳ **Le 7 categorie vuote** restano a **E25**, per costruzione: una categoria si popola quando ha un
   consumer, non quando esiste un'icona.

---

## 8. Divergenze da segnalare (repository vince)

| # | Documento | Repository | Esito |
|---|---|---|---|
| 1 | inventario v0.1 = 24 icone | **61** chiavi derivate | il batch AI è un **sottoinsieme**; utile come produzione, non come catalogo |
| 2 | `certainty` come asse singolo | **tre** assi ortogonali (`Validity` · `Certainty` · `Knowledge`) | collassarli produrrebbe icone che mentono su `CellOnly` |
| 3 | sorgenti AI a 1024×1024 | SVG deterministici → `cairosvg` a 16/20/24/32/48 px, con 7 gate | la pipeline reale è riproducibile; quella AI non lo sarebbe |
| 4 | «consolida il design system» | `D-031` **Consolidata** dal 2026-08-08 | zero delta |

---

## 9. Prossimo passo — uno solo

**Chiudere la metà Blueprint del gate anti-texture** dentro `#220`: l'asserzione che nessuna proprietà
dichiarata da un `WBP_RT_*` sia una `UTexture2D`, in `RTMatchWidgetAssetTests.cpp`, dove le
`UWidgetBlueprintGeneratedClass` sono già caricate.

È il solo punto in cui il documento indica un rischio **reale e ancora scoperto** («nessun widget
gameplay-facing deve hardcodare texture semantiche»), il rimedio è già individuato dal repository, e costa
un'asserzione in un file che esiste.
