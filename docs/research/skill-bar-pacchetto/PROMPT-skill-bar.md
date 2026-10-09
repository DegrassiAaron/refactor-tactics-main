# Prompt — Skill bar: dal mockup alla Action Dock

> **Destinatario**: Claude Code, nel repo `DegrassiAaron/refactor-tactics-main`.
> **Sorgente visiva**: questo pacchetto. `immagini/01..07` = barra di Aevik in 7 stati di pianificazione; `immagini/08` = gli 8 stati dello slot di `progettazione-hud.md` §7. Valori in `dati/`, ricette in `SPECIFICA-VISIVA.md`.
> **Statuto del mockup**: `PROPOSTA`. È un layout che [D-407](../../decisions/RT_PDR_00_Decision_Log.md) dichiara di **non** prescrivere.
> **Modalità**: sequenziale e a sessione singola ([D-178](../../decisions/RT_PDR_00_Decision_Log.md): niente worktree paralleli, niente multi-terminale).

---

## 0. Regole che valgono per tutto il prompt

1. **Si misura prima di fidarsi.** Ogni fatto del §1 va rimisurato su `HEAD` prima di essere usato. Se una misura diverge dal valore atteso, **fermati e riporta**: non adattare il piano in silenzio.
2. **SEARCH → REUSE → CREATE.** La dock esiste già (`URTActionDockWidget`, `URTActionSlotWidget`, `WBP_RT_ActionDock`, `WBP_RT_ActionSlot`). Si **estende**, non si affianca una seconda dock.
3. **Gli asset binari non si toccano da qui.** `.uasset` e `.umap` si modificano solo in una sessione editor. La sessione si registra in `docs/roadmap/editor-sessions.yaml`, nell'ordine sequenziale che quel file impone. Claude Code prepara il C++ e la voce di sessione; il Blueprint lo cabla Aaron in editor.
4. **Nessuno stato di implementazione si afferma senza grep/file** contro `Source/`.
5. **Una decisione mancante non si deduce.** Le voci marcate 🔴 nel §2 richiedono l'autore. Proponi una voce di Decision Log e attendi; non scegliere tu.


## 0-bis. Il pacchetto entra nel repo come sorgente di design

1. Copia il pacchetto, **senza** `PROMPT.md`, in `docs/research/design/hud/skill-bar-2026-10/`. Prima verifica che la cartella non esista e che non ci sia già una sede equivalente (`ls docs/research/design/hud/`).
2. In testa a `SPECIFICA-VISIVA.md` aggiungi l'intestazione di statuto che il repo usa per le sorgenti di design (*«sorgente di design, non canone»*). Il formato lo prendi da `docs/research/design/icon/visual-language/01-principi.md`.
3. Aggiungi una riga in `docs/CHANGELOG_DOCUMENTATION.md`.
4. ⛔ Non si copia nulla in `Content/`. Le PNG sono riferimento, non texture, e le icone del mockup sono segnaposto: le icone vere arrivano da `DA_IconCatalog` via `IconId`.
5. `sorgente-mockup/*.dc.html` è il formato del canvas Design, non HTML eseguibile: serve solo come riferimento dei valori.

---

## 1. Audit — fatti da rimisurare (sola lettura)

| # | Fatto atteso | Come si misura |
|---|---|---|
| F1 | Le generiche hanno tasti stabili: `Guard G · Brace B · Overwatch C · Interact X · Wait Z` | `grep -n -A8 "ARTPlayerController::GenericHotkeys()" Source/RefactorTactics/Player/RTPlayerController.cpp` |
| F2 | Il kit usa i tasti `1`…`9`, `0` | `grep -n -A12 "ARTPlayerController::AbilityHotkeys()" …/RTPlayerController.cpp` |
| F3 | `FRTAbilityCooldownView` porta: `ActionId · IconId · FallbackIconId · DisplayName · AbilityIndex · HotkeyLabel · bPlanned · Slot · TurnsRemaining · TotalTurns · bUsableNow`. **Non** porta fase né corsia | `awk '/^struct FRTAbilityCooldownView/,/^};/' Source/RefactorTactics/UI/RTHudViewModel.h` |
| F4 | `ERTActionSlot` non contiene `SignatureDefense` (D-407: *«nome deciso, non campo aggiunto»*) | `grep -n -A20 "enum class ERTActionSlot" Source/RefactorTactics/Ability/RTActionDef.h` |
| F5 | `FRTActionDef::ReservesMovementProfileId` esiste | `grep -n ReservesMovementProfileId Source/RefactorTactics/Ability/RTActionDef.h` |
| F6 | `FRTActionDef::MinStability` **non** esiste (spec-barra-comandi, #606) | `grep -rn MinStability Source/` |
| F7 | Fasi: `Guard`, `Brace` e `Overwatch` → Prep; `Interact` e `BasicAttack` → Blast; `Wait` → `NormalMovement`; `Action.Electrify` (Conductive Node) → `Environment`, cioè Cleanup | `grep -n 'TEXT("Action.<Id>"), ERT' Source/RefactorTactics/Ability/RTCatalogLibrary.cpp` + `MapResolutionPhase` |
| F8 | Kit di Aevik: `ArcPulse` (indice 0) · `LinearDischarge` · `ConductiveNode` · `Overload` · `ReactiveCapacitor` (reazione) | `grep -oE "Hero\.Aevik\.[A-Za-z]+" Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp \| sort -u` |
| F9 | Palette di fase (D-233): Prep `#56B4E9` · Dash `#009E73` · Blast `#D55E00` · Move `#0072B2`. Chrome (§32): `#080F14 #151A23 #212733 #203542 #4A5568`; accenti `#00E0FF #7C5CFF #FFD456 #FF4D4D` | `docs/technical/systems/progettazione-hud.md` §32, `docs/research/design/icon/visual-language/02-color-system.md` |
| F10 | Test ScreenHud della dock esistenti (non vanno resi rossi): `ActionDockShowsTheNeutralState`, `DockArmsOnlyTheSelectedAction`, `DockSurvivesAHoleInTheKit`, `SlotsAreReadNotDeduced`, `ActionSlot*` (10), `MovementSlotSaysNothingWhenUnauthorized` | `grep -rhoE "RefactorTactics\.ScreenHud\.[A-Za-z]+" Source/RefactorTactics/Tests \| sort -u` |
| F11 | Stato di #1410 (selettore di profilo) e #653 (profilo come entità) | `gh issue view 1410 --json state,title` · `gh issue view 653 --json state,title` |
| F12 | Ultima sessione editor registrata | `grep -n "  - id: U" docs/roadmap/editor-sessions.yaml \| tail -3` |

Se l'accesso a GitHub va in rate-limit, usa il clone locale per tutto tranne F11. Per F11 riporta `NON MISURATO`: non dedurre.

---

## 2. Che cosa il mockup chiede, classificato

Usa il formato `CURRENT / PARTIAL / DESIGNED / FUTURE / PROPOSTA`, e dichiara i conflitti a parte.

| Elemento del mockup | Classe attesa | Note |
|---|---|---|
| Gruppi visivi: **Comuni** (Guardia, Irrigidimento, Guardia reattiva, Interagisci, Attesa) · **Base** (attacco base, difesa caratteristica) · **Kit** (4 skill) · **Profilo di movimento** | PROPOSTA | Le corsie di §6.7 sono «aiuto alla lettura». D-397: l'ordine di `GetActions()` è **identità**, e il widget non deduce mai un indice dalla posizione. Il raggruppamento si fa **leggendo** un campo, non riordinando la lista |
| Tasto mostrato su ogni slot | CURRENT | `HotkeyLabel` (F1/F2). Nessun lavoro |
| Striscia colore + etichetta di fase (`PREP`/`BLAST`/`CLEANUP`/`REAZ.`) | DESIGNED | Manca il campo nella view (F3). Il colore è il secondo canale: l'etichetta testuale resta obbligatoria |
| Stati Available / Hover / Selected / Planned / Cooldown / Unavailable / Invalid / Warning | PARTIAL | `bPlanned`, `TurnsRemaining`, `bUsableNow` esistono; Invalid e Warning vanno verificati |
| Indicatore **slot occupati** (Principale · Movimento · Reazione) | DESIGNED | È l'unica voce scoperta del piano UI-0 (`docs/roadmap/plans/ui-0-first-playable-hud-2026-08-12.md`) |
| Selettore di profilo `Withdraw ×0,25 · Sneak ×0,5 · Move ×1 · Sprint ×2` con blocco per Stability/riserva | FUTURE | Bloccato da #1410 e #653 (D-397). **Non** implementarlo qui |
| Slot **Difesa caratteristica** vuoto e tratteggiato | FUTURE | F4. Il roster non soddisfa lo slot (spec-barra-comandi §1.2) |
| **Irrigidimento** (`Action.Brace`, tasto B) nelle Comuni | 🔴 **CONFLITTO** | Vedi §3 |

---

## 3. 🔴 Decisioni che richiedono l'autore — non procedere su queste senza risposta

**DEC-SB-1 — `Brace` nella barra.** *(decisa: resta da registrare)*
- L'autore ha **deciso di inserirlo** il 2026-10-03, ed è nel mockup (tasto B, gruppo Comuni). La decisione d'autore c'è; manca la sua voce nel Decision Log.
- D-407 elenca **undici** comandi senza `Brace`; D-025 e `GenericHotkeys()` lo includono.
- Scrivi una voce `D-4xx`, numerata dopo l'ultima in `HEAD`, come *decisione d'autore in sessione (2026-10-03)*: *«`Action.Brace` resta un comando della barra: i comandi diventano dodici»*.
- Nella stessa voce, verifica e correggi il testo di D-407 e di `docs/gameplay/spec-barra-comandi.md` §1.
- Registra anche la compatibilità col movimento: `Brace` applica `Root`, quindi blocca ogni profilo. È coerente con §1.1 della spec, ma la tabella di §3 non ha la riga.

**DEC-SB-2 — dove vive la classificazione di gruppo.**
- Scelta 1: un campo dichiarato sull'azione (dato).
- Scelta 2: derivato da `AbilityIndex == 0` e dall'appartenenza alle generiche (regola).
- Presenta entrambe con il costo misurato, nel formato `FATTO / INTENTO / PROBLEMI·AMBIGUITÀ / RISCHI`.

**DEC-SB-3 — colore di Cleanup.**
- D-233 ha quattro fasi; `Action.Electrify` risolve in Cleanup.
- Il mockup usa un tratteggio neutro.
- Serve una decisione, oppure la conferma che Cleanup resta senza colore e con la sola etichetta.

---

## 4. Lavoro eseguibile adesso (dopo l'audit, senza le 🔴)

Ordine **sequenziale**. Un passo non parte finché il precedente non è verde.

**P1 — Issue.** `gh issue list --search "ActionDock OR skill bar OR barra comandi" --state all`.
- Riusa un'issue che copra il lavoro.
- Altrimenti creane **una** col template `.github/ISSUE_TEMPLATE/task.md`, sotto l'epic UI pertinente, con link a #1410, #653, #606 e D-407.
- Il titolo nomina l'esito, non il widget: *«La dock dice di che fase è ogni azione e quali slot del turno sono occupati»*.

**P2 — Fase nella view (C++).**
- Aggiungi a `FRTAbilityCooldownView` la macro-fase (`ERTMatchPhase`), letta da `MapResolutionPhase(Def.ResolutionPhase)`.
- Per le azioni con `Slot == Reaction`, un flag dedicato (`bIsReaction`), perché la reazione non è una fase.
- Test nuovi:
  - `ScreenHud.ActionSlotCarriesItsPhase`: almeno un caso per Prep, Blast, Cleanup e Reaction, sul kit reale di Aevik.
  - `ScreenHud.ActionSlotPhaseIsReadNotDeduced`: la fase viene dal catalogo, non dall'indice.
    - Controprova: cambiando la fase nel dato, il campo segue.

**P3 — Slot occupati (C++).**
- Una view `FRTPlanSlotsView { Main, Movement, Reaction }` per l'unità selezionata.
- Ogni voce porta `ActionId` e `DisplayName` o vuoto, letti dal piano.
- Rispetta `InspectedEnemyNeverCarriesItsPlannedSlots`: per un nemico ispezionato la view è vuota.
- Test nuovi:
  - `ScreenHud.PlanSlotsNameWhatIsPlanned`.
  - `ScreenHud.PlanSlotsSayNothingForAnInspectedEnemy`.

**P4 — Sessione editor (YAML, non binari).**
- Aggiungi a `editor-sessions.yaml` la prossima `U<n>` dopo F12.
- Contenuto (riferimenti: `SPECIFICA-VISIVA.md` §2–4, `dati/tokens.json`, `immagini/`):
  - cablaggio in `WBP_RT_ActionSlot` di striscia di fase ed etichetta;
  - gli stati di §7 secondo `immagini/08-stati-slot.png`, con forma/pattern come secondo canale (Planned = angolo pieno; Unavailable = tratteggio diagonale; Invalid = ✕; Warning = bordo tratteggiato + triangolo);
  - `WBP_RT_ActionDock` con i separatori dei gruppi;
  - un `WBP_RT_PlanSlots` per gli slot occupati.
- Le misure di `tokens.json` sono in px di design a 1920×1080: in UMG si usano come valori di riferimento, applicando la DPI scale del progetto.
- I colori fuori da §32 (sezione `testo_mockup_non_in_style_guide`) **non** si coniano come nuovi token senza conferma. Proponili in sessione.
- Criterio di accettazione: il test della scala di grigi di §47-bis.1 (nessuno stato si confonde senza colore).
- Precondizione: commit di `main` che contiene P2 e P3 (D-403).

**Fuori scopo esplicito**: il selettore di profilo e la difesa caratteristica (§2, FUTURE), e qualunque riordino di `GetActions()`.

---

## 5. Verifica

- Suite completa prima e dopo. Riporta `N/N, 0 fallimenti` e i test nuovi per nome.
- I test di F10 restano verdi. Se uno diventa rosso, è un difetto di P2/P3, non un aggiornamento del test.
- Nessun bump del corpus golden atteso: la modifica è di sola view. Se il digest del TurnLog cambia, **fermati**: qualcosa ha toccato lo stato di gioco.

---

## 6. Report finale (formato fisso)

0. Percorso del pacchetto nel repo e commit.
1. Audit F1–F12: valore atteso, valore misurato, ✅/❌.
2. Classificazione del §2 confermata o corretta.
3. Le tre 🔴 con la voce di Decision Log proposta (testo pronto, **non** committato).
4. Issue riusata o creata (link), commit e test.
5. Voce `U<n>` aggiunta e cosa resta ad Aaron in editor.
6. Prompt di handoff per la sessione successiva.
