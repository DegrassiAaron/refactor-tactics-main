# UI-0 — First Playable HUD

> `CURRENT` · **Piano in esecuzione** · **Data**: 2026-08-12
> `RT-FEAT-UI-SCREEN-HUD` e' **`IMPLEMENTING`** in v0.1: la sequenza descritta qui non e' finita.
> ⚠️ I riferimenti a issue e stato invecchiano — aperto/chiuso vive su GitHub, e in caso di divergenza
> **vince GitHub**.
>
> **Tipo**: piano di sequenza · **Data**: 2026-08-12 · **Stato**: consolidato in sessione
> **Origine**: proposta «UI-0» rivista da un panel di specifica (`/sc:spec-panel`, modalità critique,
> focus requirements + architecture).
>
> **Cosa è**: l'ordine in cui i pezzi dell'interfaccia entrano, e **dove passa il confine** fra ciò che è
> già disegnato e ciò che va costruito.
> **Cosa non è**: l'owner della UI. Il documento normativo resta
> [`progettazione-hud.md`](../../technical/systems/progettazione-hud.md); lo stato delle feature resta
> [`feature-registry.yaml`](../feature-registry.yaml).

---

## 1. La premessa che il piano originale sbagliava

La proposta descriveva #77 come «la prima vera schermata da giocatore», da costruire in UMG partendo da
zero. **Misurato sul branch, non è così.**

`ARTHUD` (`Source/RefactorTactics/UI/RTHUD.{h,cpp}`, 544 righe) è una HUD **Canvas in C++** — l'header lo
dichiara: *«nessun asset UMG»* — e disegna già:

| Voce del DoD di #77 | Stato reale |
|---|---|
| Barre HP, scudo ed energia per unità | ✅ `RTHUD.cpp:124-143` |
| Timer di planning e fase corrente | ✅ `:418-430` |
| Round su `RoundLimit` **letto dal formato** | ✅ `GetMatchRules().RoundLimit` (`:403`) — **già conforme**, nessun `12` scritto a mano |
| Cooldown residui per azione | ✅ `GetAbilityCooldown` (`:510`) |
| Nessuna informazione avversaria | ✅ per costruzione (`ComputePlannedHitMarks` legge solo `PlayerTeamId`) |
| **Slot occupati** movimento/azione/reazione | ❌ **l'unica voce scoperta** |

In più `ARTHUD` disegna intenti alleati, path, waypoint, traiettoria di Dash, AoE, avviso di fuoco amico e
banner di esito.

Contemporaneamente: **`Content/RT/UI` non esiste e non c'è nessun `WBP_*` nel progetto**. Le due cose non
si contraddicono — la HUD è disegnata, ma non con widget.

⚠️ **Conseguenza sul gate di uscita proposto.** «Premo Play e capisco round, fase, salute, azioni, piano e
Ready senza guardare l'Output Log» **è già vero oggi**, tranne la parola *piano*. Un gate che il ramo
soddisfa prima di iniziare non discrimina: va sostituito dai quattro test headless che #77 chiede già e
dalla voce che manca davvero.

---

## 2. La decisione: due layer, non una sostituzione

La owner spec separa già i layer, e il piano originale li fondeva in un unico `WBP_TacticalHUD` full screen.

- **§4.1 Screen HUD / UMG** — turno, fase, timer, objective, team roster, selected unit, HP/shield/risorse,
  action dock, Ghost Timeline, warning, combat log, conferma piano, playback controls.
- **§4.2 Tactical World Overlay** — cell hover, reachable cells, movement path, waypoint, destination,
  traiettoria di Dash, target line, AoE, friendly-fire, facing, coni di Overwatch, hazard, Action Ghost.
  La spec aggiunge, testualmente: questi elementi **«non devono essere realizzati come grandi widget HUD
  statici»**.

Il §4.2 è **esattamente ciò che `ARTHUD` fa oggi**, ed è la forma che la spec prescrive. Quindi:

> **`ARTHUD::DrawHUD` resta e continua a possedere il §4.2. Il nuovo UMG prende solo il §4.1.**

Non è un compromesso di comodo: assorbire il §4.2 dentro un widget riscriverebbe 544 righe coperte da test
(`RefactorTactics.HUD.*`, tre test su `ComputePlannedHitMarks`) per violare la riga della spec che il piano
citava a proprio sostegno.

### 2.1 Chi disegna le barre — la sovrapposizione da evitare

È l'unico punto in cui i due layer si toccano davvero, e va deciso **prima** dell'incremento A, altrimenti
il primo playtest legge doppie barre HP come un difetto.

| Elemento | Layer | Motivo |
|---|---|---|
| Barra HP/scudo/energia **ancorata sopra l'unità in campo** | §4.2 — resta in `ARTHUD` | È proiettata nel mondo e segue l'unità: è overlay, non pannello |
| **Roster di squadra** (elenco laterale, una riga per unità) | §4.1 — UMG | È una vista aggregata, non ancorata a nessuna cella |
| **Selected unit panel** (HP/shield/energia dell'unità selezionata) | §4.1 — UMG | Idem, ed è la vista di dettaglio |

Le due rappresentazioni **coesistono di proposito**: la barra sopra l'unità risponde a «quanto è ferito
*quello lì*», il pannello a «quanto è ferito *chi sto comandando*». Non sono duplicati; diventerebbero
duplicati solo se il roster ripetesse la barra ancorata nello stesso punto dello schermo.

⚠️ **`ARTHUD` continua a girare sotto il widget.** `AHUD::DrawHUD` e i widget UMG si compongono senza
conflitto — l'ordine è Canvas prima, Slate sopra. Ciò che va evitato è che il widget del §4.1 occupi la
**zona centrale**: la mappa resta il centro visivo, e il §4.2 disegna lì.

---

## 3. Sequenza corretta

| # | Issue | Stato reale | Nota rispetto alla proposta |
|---|---|---|---|
| 0 | #218 — `URTIconCatalogData` | ✅ **già chiusa** | Tipo, libreria e cinque test esistono |
| 0 | #219 — categorie della v0.1 | ⏳ aperta | Resta il **dato**: nessun `DA_IconCatalog` è stato creato |
| 1 | **#77 — CP 11.1** | ⏳ aperta | Ridotta al delta reale (§4) — non è più «costruire la HUD» |
| 1b | **#77bis — CP 11.7** *(nuova)* | ⏳ da aprire | La base UMG del §4.1, separata da #77 |
| 2 | #220 — i widget consumano il catalogo | ⏳ aperta | Diventa esigibile solo con CP 11.7: prima non ci sono widget |
| 3 | #79 — CP 11.3 combat log | ⏳ aperta | ✅ codice fatto, resta `PIE-V01-LOG` |
| 4 | #172 — CP 11.5 Ghost Timeline | ⏳ aperta | |
| 5 | #78 — CP 11.2 intenti e certezza | ⏳ aperta | |
| 6 | #173 — CP 11.6 scrubbing | ⏳ aperta | |
| 7 | #291 — rotazione dichiarata | ⏳ aperta | ⚠️ **#601 è CHIUSA**: la reaction è già cablata, resta solo il facing |
| 8 | #166 — CP 14.6 Fast Reaction UI | ⏳ aperta | Corretto rimandarla: #165 non ha ancora il Decision Boundary runtime |

**#289 — CP E21.3 leggibilità tattica** resta parallela e fuori sequenza: non è UMG.

### Perché l'ordine icone → widget regge

Non è una preferenza: **D-031 lo prescrive**. *«Va prima di E11, non dopo: a widget scritti diventa un
refactor.»* La v0.1 popola cinque categorie su dodici, quindi #219 è un lavoro corto e #220 si chiude
mentre i widget nascono.

---

## 4. Il delta di #77, misurato

Cosa manca davvero al checkpoint, dopo aver tolto ciò che è già implementato:

1. **Slot occupati** — movimento / azione principale / reazione per l'unità selezionata, con indicazione di
   cosa è già stato scelto. Oggi la reazione compare solo nella riga di intento sopra l'unità
   (`RTHUD.cpp:316-318`), non come terna leggibile.
2. **I quattro test headless** che il DoD chiede: la feature dichiara `tests: []`. Il precedente da imitare
   esiste — `ComputePlannedHitMarks` è una statica pura testata da `RTHUDMarksTests.cpp`.
3. **Vocabolario**: il codice stampa `"Turno %d/%d"` (`:405`), mentre il DoD prescrive **round** — nel
   progetto il *turno* è la sequenza di fasi dentro il round.

Nient'altro. #77 **non chiede UMG in nessuna delle sue voci**: costruire il §4.1 dentro #77 la renderebbe
un checkpoint senza criterio di chiusura.

---

## 5. Gli incrementi, riassegnati

Gli incrementi A–F della proposta restano validi, ma si dividono fra due checkpoint:

**CP 11.1 (#77) — chiude il contenuto informativo**
- E → **Planning State**: la terna MOVEMENT / MAIN / REACTION, con Undo e Ready
- I quattro test headless
- Il vocabolario `Round`

**CP 11.7 (nuova) — introduce il layer §4.1 in UMG**
- A → **HUD Shell**: `WBP_RT_TacticalHUD`, zone Top / Left / Bottom / Right, **centro libero**
- B → **Match Header**: `WBP_RT_TurnHeader` — round/`RoundLimit`, fase, timer
- C → **Unit Panel**: `WBP_RT_TeamRoster` + `WBP_RT_SelectedUnitPanel`
- D → **Action Bar**: `WBP_RT_ActionDock` + `WBP_RT_ActionSlot`, icone dal catalogo (#220)
- F → **Player View gate**: debug spento di default, `PIE-V01-HUD` eseguita

⚠️ **I nomi seguono la owner spec §45**, che li dichiara già: `WBP_RT_TacticalHUD`, `WBP_RT_TurnHeader`,
`WBP_RT_TeamRoster`, `WBP_RT_SelectedUnitPanel`, `WBP_RT_ActionDock`, `WBP_RT_ActionSlot`. La proposta
scriveva `WBP_TacticalHUD` senza prefisso: su un `.uasset` il rename costa più che scriverlo giusto.

⚠️ **I widget non ricalcolano nulla.** La spec lo ripete in tre punti (§668, §858, §931): la formula, la
visibilità e il reason code arrivano dalla logica. Il §4.1 legge un view model sanitizzato — la stessa
regola per cui `ComputePlannedHitMarks` è statica e senza accesso alla selezione.

---

## 6. Cosa resta fuori, e perché

Confermato dalla proposta originale, senza modifiche: main menu, settings, matchmaking, lobby, CommonUI,
animazioni dei pannelli, tooltip enciclopedici, HUD multiplayer, layout 4v4, UI del rumore, replay browser,
polishing.

**#166 (Fast Reaction FIRE/HOLD)** resta dopo #165: un widget che non ha un Decision Boundary runtime da
consumare è un dato senza produttore.

---

## 7. Gate

- **CP 11.1**: suite verde compresi i quattro test headless nuovi; `PIE-V01-HUD` registrata con esito reale.
- **CP 11.7**: `PIE-V01-HUD` copre anche l'ingombro del §4.1; nessuna regressione in
  `RefactorTactics.HUD.*`; nessun `WBP_*` referenzia una texture direttamente (D-031).

⚠️ Nessuno dei due si dichiara chiuso su «si vede che funziona»: il precedente è annotato nel registry per
`RT-FEAT-UI-TACTICAL-CAMERA` — *«non promuovere per "si vede che funziona"»*.
