# La barra dei comandi prende tutta la fascia bassa (2026-10-04) — spec panel sulla posa, e che cosa ne è entrato

> `CURRENT` · **Stato**: decisione registrata ([D-456](../../decisions/RT_PDR_00_Decision_Log.md)); il campo
> `Group` consegnato in [#3468](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3468). La zona
> unica, i gate e l'asset restano in [#3469](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3469).
> **Data**: 2026-10-04, pomeriggio · **Misurato su** `origin/main` = `06b76a716`, nel clone `refactor-tactics-dev`.
> Il clone principale era sul branch della PR #3466, e un Editor era aperto lì da un'altra sessione.
> **Riprende**: [`skill-bar-mockup-spec-panel-2026-10-04.md`](skill-bar-mockup-spec-panel-2026-10-04.md), il
> triage del mattino. Quel referto ha chiuso il mockup **slot per slot**; questo tratta la **posa** della barra.
> **Panel**: Wiegers (lead) · Fowler · Crispin · Adzic · Nygard · **Modo**: critique · **Focus**: requirements,
> architecture.

---

## 1. Il requisito, e che cosa nominava davvero

L'autore, in sessione: *«la barra sostituisce l'area viola in basso al centro, la zona blu in basso a sinistra
e quella viola in basso a destra»*.

Le tre zone colorate sono i bordi da cantiere di `URTHudZoneWidget::BlockoutColor`, che deriva la tinta
dall'indice di `ERTHudZone` con `MakeFromHSV8` (tinta su 0–255, quindi `× 360 / 256`):

| Zona | Indice → tinta | A schermo | Contenuto a `06b76a716` |
|---|---|---|---|
| `BottomLeft` | 5 → 225° | blu | vuota, senza inquilino |
| `BottomCenter` | 6 → 270° | viola | `WBP_RT_ActionDockBottom` |
| `BottomRight` | 7 → 315° | magenta | vuota, riservata da `progettazione-hud.md` §6.8 a Confirm · Undo |

## 2. Il panel

### 2.1 WIEGERS — il requisito diceva *dove*, non *perché*

❌ **CRITICAL.** Misurato sui token del mockup (`dati/tokens.json`, px di design a 1920×1080: slot 78, gap 8,
separatore 22, padding orizzontale 24 + 24, profilo 80):

| Contenuto | Larghezza | Contro |
|---|---|---|
| Mockup intero | 1380 px (≈1440 sul render) | `BottomCenter` utile: 1152 − 8 = **1144 px** ❌ |
| Senza il selettore di profilo | **1014 px** | `BottomCenter` ✅ |
| Fascia intera | — | 1920 − 8 = **1912 px** |

La parte che non entra nella cella centrale è **il selettore di profilo**, cioè proprio quella che
[D-425](../../decisions/RT_PDR_00_Decision_Log.md) esclude. ∴ con il contenuto costruibile oggi la dock entra
in `BottomCenter`, e prendere la fascia è una decisione su ciò che sta alle **estremità**. Domanda posta
all'autore, §3.

### 2.2 FOWLER — la griglia è un contratto nel codice

⚠️ **MAJOR.** La griglia a otto zone sta in quattro punti, e tutti e quattro rifiuterebbero una dock su tre celle:

- l'enum `ERTHudZone`;
- le mappe `Colonne` e `Righe` di `RTGrigliaZone::CellaAttesa`, legate a `ERTHudZone::Count` da `static_assert`;
- `ScreenHud.TheEightZonesAreDeclaredExactlyOnce`;
- `ScreenHud.ZoneRectanglesMatchTheThreeByThreeGrid`.

| Strada | Esito |
|---|---|
| **A** — le tre celle diventano un valore, `Bottom` | ✅ scelta. È lo stesso ragionamento con cui `ERTHudZone` non ha un `Center`: un valore che non si deve usare è un invito a riempirlo |
| **B** — tenere `BottomLeft`/`BottomRight` esentati dai gate | ❌ valori morti |
| **C** — allargare gli anchor di `BottomCenter` | ❌ il gate della griglia diventa rosso, e il nome dice il falso |

### 2.3 CRISPIN — i gate cambiano nome, e l'altezza non regge l'intestazione

⚠️ **MAJOR.**
- «Eight» in `TheEightZonesAreDeclaredExactlyOnce` diventerebbe falso: il test va rinominato, insieme ai riferimenti nei documenti.
- Serve una controprova: un `Zone_BottomLeft` rimasto nell'albero deve rendere il gate rosso.
- Sull'altezza: la fascia ha **208 px** utili (216 − 8). Il pannello del mockup ne occupa circa 165 sul render, e con la riga d'intestazione (`PIANIFICAZIONE` · unità · chip) circa **224**. L'intestazione scenderebbe nel centro, e fase e unità sono già mostrate altrove.

### 2.4 NYGARD — le risorse condivise

⚠️ **MAJOR.**
- Il clone principale era sul branch di **#3466** (U60), con un **Editor aperto** da un'altra sessione (log `u62-seduta-2.log`). Quella «U62» non è registrata in `editor-sessions.yaml` né su alcun ref remoto: il numero della prossima seduta va rimisurato al merge.
- Il C++ è stato scritto nel clone `refactor-tactics-dev`. L'asset si tocca solo via bridge, nel clone principale libero.
- I gate della griglia restano rossi fra il commit C++ e quello dell'asset, quindi i due stanno nella **stessa PR** (#3469). Il precedente è `7d1145cd7`, che ha portato le otto zone «rosso → verde» in un branch solo.

### 2.5 ADZIC — l'inquilino che la tabella non nominava

⚠️ **MINOR.** `guida-screen-hud-umg.md` §3 elenca gli inquilini delle zone, ma **il Context Inspector non c'è**. È l'overlay di debug in Slate, posato in basso a destra (`HAlign_Right`, `VAlign_Bottom`, `MaxWidth = 460`).
- La sua convivenza con la fascia bassa è **dichiarata e giudicata accettabile** dal 2026-09-24 (`RTContextInspector.h`, #3319).
- Il suo gate di colonna, `Debug.ContextInspectorColumnsDoNotOverlap`, lo confronta con l'overlay del verdetto PIE, **non** con la dock: la fascia unica non lo rende rosso.
- Con la barra a tutta fascia l'overlay copre l'estremità destra della barra invece di §6.8. Dichiarato in D-456 punto 9.

## 3. Le decisioni

Prese dall'autore in sessione il 2026-10-04, dopo il referto delle misure del §2.

| Domanda | Risposta |
|---|---|
| Che cosa va all'estremità destra della barra? | **La lettura del movimento**: un indicatore di sola lettura del profilo, da `FRTUnitSlotsView::MovementProfileId`, col badge del tasto `M`. Confirm · Undo salgono in `TopRight` |
| Dove va la Ghost Timeline (#172), che §6.6 metteva in basso al centro? | **`TopCenter`**, accanto alla fase |

Le conseguenze che la scelta comporta, senza una seconda domanda:

- la riga d'intestazione del mockup non si costruisce;
- i chip restano nel `SelectedUnitPanel`, come oggi;
- l'Objective di §6.3 perde la cella `TopRight`, che era vuota. Dove vada il resto della sua lista **non** è deciso, ed è un seguito di #613.

Registrato in [D-456](../../decisions/RT_PDR_00_Decision_Log.md). Nello stesso giro entrano anche
[D-454](../../decisions/RT_PDR_00_Decision_Log.md) (`Brace`, dodici comandi) e
[D-455](../../decisions/RT_PDR_00_Decision_Log.md) (il gruppo si deriva), col testo pronto del referto del
mattino.

⏱️ **D-455 è stata precisata alla registrazione, e la precisazione è dichiarata nella voce.** Il testo deciso
diceva *«Base se `BaseActionId == Action.BasicAttack`»*. Ma per [D-033](../../decisions/RT_PDR_00_Decision_Log.md)
la generica stessa porta `BaseActionId` **vuoto**, e un attacco base nudo sarebbe caduto nel Kit. La regola
registrata guarda `ActionId` **o** `BaseActionId`.

## 4. Che cosa è stato fatto

| Passo | Esito |
|---|---|
| Issue | [#3468](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3468) il gruppo · [#3469](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3469) la zona unica · [#3470](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3470) l'indicatore del movimento · [#3471](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3471) Confirm · Undo · commento su [#172](https://github.com/DegrassiAaron/refactor-tactics-main/issues/172) per la timeline |
| Registro | D-454, D-455, D-456; D-407 annotata, non riscritta |
| `spec-barra-comandi.md` | dodici comandi, la riga di `Brace` in §1 e §3; un puntatore `file:riga` scaduto sostituito col simbolo |
| `progettazione-hud.md` | §6.3, §6.4, §6.6, §6.7 e §6.8 allineate a D-456; la nota di §6.7 su #1410/#653 rimisurata contro D-425 |
| C++ (#3468) | `ERTActionGroup`, `FRTAbilityCooldownView::Group`, `URTHudViewModel::GroupFor`; test `HudViewModel.ActionSlotCarriesItsGroup` e `HudViewModel.ActionSlotGroupIsReadNotDeduced` |
| `RTContextInspector.h` | il commento che elenca i proprietari delle zone di §6 segue D-456 |

## 5. Verifica

Vedi la PR di #3468 per gli esiti: build, test nuovi, mutazione, suite prima e dopo.

## 6. Seguiti

- **#3469**: enum, gate e asset nello stesso branch, con l'asset toccato via bridge nel clone principale; `guida-screen-hud-umg.md` §3 nello stesso commit dell'asset.
- **Seduta Editor per la resa della barra**: separatori dei gruppi da `Action.Group`, disposizione nella fascia, posto per l'indicatore. Il numero `U<n>` si misura quando la si registra (§2.4).
- **#3470**, **#3471**, **#172**: ciascuna col proprio scope.
- **Il resto dell'Objective (§6.3)**: senza sede, seguito di #613.
- Ereditati dal mattino e ancora aperti: il produttore di Invalid/Warning sullo slot, e il piano UI-0 che dichiara gli slot occupati come *«unica voce scoperta»*.
