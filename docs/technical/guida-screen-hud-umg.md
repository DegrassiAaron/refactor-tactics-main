# Guida — costruire i `WBP_RT_*` dello Screen HUD

> `CURRENT` · **Ricetta d'Editor** per [CP 11.7](../roadmap/roadmap-v0.1.md) ·
> issue [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613) ·
> owner del *cosa mostrare*: [`progettazione-hud.md`](systems/progettazione-hud.md) §4.1.

Questo file dice **come costruire i Blueprint**. Non dice cosa l'HUD mostra — quello è `progettazione-hud.md`
— e non dichiara regole di gioco.

Esiste perché i `.uasset` **non sono versionati** in questo repository (`Content/**/*.uasset` in
`.gitignore`): il codice può preparare tutto tranne l'ultimo passo, e senza una ricetta scritta quell'ultimo
passo si rifà a memoria ogni volta.

---

## 1. Cosa esiste già, e cosa devi fare tu

| Pezzo | Dove | Stato |
|---|---|---|
| Classi base C++ dei widget | `Source/RefactorTactics/UI/RTScreenHudWidgets.h` | ✅ |
| Viste sanitizzate (round, roster, slot, cooldown) | `URTHudViewModel` | ✅ |
| Catalogo icone (chiave → asset) | `URTIconCatalogData` + `URTIconLibrary` | 🟡 codice sì, **il `.uasset` no** ([#220](https://github.com/DegrassiAaron/refactor-tactics-main/issues/220)) |
| I sei `WBP_RT_*` | `Content/RT/UI/` | ⛔ **questo lavoro** |

`Content/RT/UI/` **non esiste**: va creata.

---

## 2. I sei Blueprint, e la classe da cui derivano

Crea ogni widget con **Widget Blueprint → scegli la classe padre**, non con il padre di default
`UserWidget`. Se ne hai già creato uno sbagliato: `Class Settings → Parent Class`.

| Blueprint | Parent Class | Cosa legge |
|---|---|---|
| `WBP_RT_TacticalHUD` | `RTTacticalHUDWidget` | contenitore a schermo intero; tiene `IconCatalog` |
| `WBP_RT_TurnHeader` | `RTTurnHeaderWidget` | `GetRoundCounterText`, `GetHeader` |
| `WBP_RT_TeamRoster` | `RTTeamRosterWidget` | `GetRoster` |
| `WBP_RT_SelectedUnitPanel` | `RTSelectedUnitPanelWidget` | `HasSelection`, `GetCard`, `GetSlots` |
| `WBP_RT_ActionDock` | `RTActionDockWidget` | `GetActions`, `GetArmedActionIndex` |
| `WBP_RT_ActionSlot` | `RTActionSlotWidget` | riceve `SetAction`; implementa `OnActionChanged` |

> ⚠️ **I nomi non sono suggerimenti.** `progettazione-hud.md` §45 li dichiara, e su un `.uasset` il rename
> costa più che scriverlo giusto la prima volta (redirector, riferimenti, Fix Up).

---

## 3. Il layout di `WBP_RT_TacticalHUD`

Quattro zone ancorate ai bordi, **centro libero**:

```text
┌─────────────────────────────────────────┐
│              Top: TurnHeader            │
├──────────┬───────────────────┬──────────┤
│  Left:   │                   │  Right:  │
│  Team    │   ← CENTRO        │  (spazio │
│  Roster  │     LIBERO →      │  futuro) │
│          │                   │          │
├──────────┴───────────────────┴──────────┤
│  Bottom: SelectedUnitPanel + ActionDock │
└─────────────────────────────────────────┘
```

🔴 **Il centro libero è un requisito, non un gusto.** Il layer §4.2 (`ARTHUD::DrawHUD`) continua a disegnare
path, waypoint, AoE, fuoco amico e le barre ancorate **sopra la mappa**: un pannello al centro glieli
coprirebbe. Si verifica a occhio in `PIE-V01-HUD`, ed è il primo difetto che un playtest segnalerebbe.

Usa un `Canvas Panel` con anchor ai bordi, non una `Vertical Box` a schermo pieno: quest'ultima non lascia un
centro davvero libero.

---

## 4. Le tre regole che il codice non può importi

Le prime due il C++ le rende difficili da violare; la terza è solo tua.

### 4.1 🔴 Nessun widget referenzia una texture

Le icone viaggiano come **chiave** (`UI.Icon.Action.Move`) e si risolvono dal catalogo — [D-031](../decisions/RT_PDR_00_Decision_Log.md).

- `URTActionSlotWidget::GetIconId()` restituisce un `FName`. **Non aggiungere una variabile `Texture2D` al
  Blueprint** per «comodità»: è esattamente la scorciatoia che il catalogo esiste per impedire, e il giorno in
  cui `Status.Wet` cambia disegno diventa un refactor di ogni widget invece di una riga di dato.
- `RefactorTactics.ScreenHud.WidgetApiExposesNoTexture` pinna la superficie **C++** via reflection e nomina la
  proprietà colpevole. ⚠️ **Non vede i Blueprint**: quella metà è tua.

Finché il catalogo `.uasset` di #220 non esiste, `ResolveIcon` restituisce il missing-icon con
`bResolved = false` e una warning che nomina la chiave. **A schermo si vede che manca** — è il comportamento
voluto, non un difetto da nascondere con una texture cablata.

### 4.2 I widget non ricalcolano

Nessuna classe base espone `ARTTurnManager` o `ARTUnit` a un Blueprint: restituiscono solo le viste di
`URTHudViewModel`, già sanitizzate. Se ti serve un dato che non c'è, **la risposta non è `Get All Actors Of
Class`**: è aggiungere un campo alla vista, dove è testabile.

Il caso concreto che si sbaglia per primo: il roster non ha un parametro «mostra anche gli avversari», e non
va aggirato leggendo le unità dal mondo. La privacy degli intenti è verificata in `FilterForTeam`, e un
secondo filtro nel widget sarebbe una seconda verità da tenere allineata.

### 4.3 Usa i binding di proprietà, non variabili copiate

Tutte le funzioni delle basi sono `BlueprintPure`: si collegano **direttamente** a un binding di proprietà
(`Text`, `Visibility`, `Percent`). Non chiamarle in `Event Tick` per salvarne il risultato in una variabile —
quella copia è la seconda verità che si scollega al primo turno in cui qualcuno dimentica di aggiornarla.

`URTActionSlotWidget` è l'eccezione **voluta**: riceve i dati da `SetAction` e ridisegna in
`OnActionChanged`. Un dock con sei slot che leggono ciascuno il proprio stato farebbe sei letture per frame e
potrebbe mostrarne una disallineata dalle altre.

---

## 5. I tre stati del contatore di round

`WBP_RT_TurnHeader` deve mostrare **tre** cose diverse, e due si confondono:

| Stato | Testo | Perché |
|---|---|---|
| nessun contesto | `—` | un widget costruito prima del `TurnManager`; `Round 0` sembrerebbe un dato |
| formato con limite | `Round 3/12` | il limite viene dal **formato**, mai da una costante |
| formato senza limite | `Round 3` | `RoundLimit == 0` = «nessun limite», **non** «su zero» |

`GetRoundCounterText()` li decide già tutti e tre: **usa quella**, non comporre il testo nel Blueprint. Un
binding ingenuo stampa `Round 3/0`, che si legge come una partita già scaduta.

---

## 6. Agganciare l'HUD

`WBP_RT_TacticalHUD` va creato e aggiunto al viewport dal `PlayerController` o dal `GameMode`. Oggi **non c'è
codice che lo faccia**: `ARTHUD` è un `AHUD` in Canvas e non conosce UMG.

Quando aggiungi l'aggancio, `bShowDebug` resta `false` — è `EditDefaultsOnly` sulla classe proprio per non
restare acceso in una sola schermata dimenticata.

---

## 7. Cosa NON migrare

⚠️ `ARTHUD::DrawHUD` **resta dov'è**. La spec dice testualmente che il layer §4.2 «non deve essere
realizzato come grandi widget HUD statici», e sono 594 righe coperte da `RefactorTactics.HUD.*`.

| Elemento | Layer |
|---|---|
| Barra ancorata **sopra l'unità in campo** | §4.2 — Canvas, resta in `ARTHUD` |
| **Roster** di squadra | §4.1 — UMG |
| **Selected unit panel** | §4.1 — UMG |

Le barre coesistono di proposito: quella ancorata risponde a «quanto è ferito *quello lì*», il pannello a
«quanto è ferito *chi sto comandando*».

---

## 8. Verifica

Quando i sei Blueprint esistono e l'HUD è agganciato, esegui **`PIE-V01-HUD`**
([`test-manuali-pie.md`](test-manuali-pie.md)) e registra l'esito. È la parte che richiede un occhio:
leggibilità delle barre, ingombro, coerenza visiva durante il playback, e il **centro libero** che nessun
test automatico può guardare.

I gate di `RT-FEAT-UI-SCREEN-HUD` in [`feature-registry.yaml`](../roadmap/feature-registry.yaml) restano
`partial` finché quel passo non è fatto: **lo stato vive lì**, non in questo file.
