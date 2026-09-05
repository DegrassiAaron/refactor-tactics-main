# Tactical Grid Overlay — roadmap della feature

> `CURRENT` · **Piano di sequenza**, non owner di regole. Ordina il lavoro fra owner che esistono già;
> non ne crea di nuovi e non ridecide nulla.
>
> **Data**: 2026-08-30 · **Rimisurato**: 2026-08-31 su `origin/main` @ `2f45974f` · **Base della prima
> stesura**: `main` @ `d9feb9b0` (7 commit dietro `origin/main` @ `aec66789`)
>
> **Cosa è**: la sequenza reale delle dipendenze del perimetro *«la griglia tattica si vede, la cella sotto il
> cursore si vede, e si può spegnere»*, con owner, stato **misurato** ed exit gate per nodo. ➕ **Dal
> 2026-08-31 copre anche la v0.2** — semantic area overlay, boundary volumetrico, Primary/Secondary — in §8.
>
> **Cosa non è**: una roadmap parallela. La roadmap canonica della v0.1 resta
> [`roadmap-v0.1.md`](../roadmap-v0.1.md) e **non è stata toccata** — vedi §5.
>
> **Origine**: [`tactical-grid-overlay-spec-panel-2026-08-30.md`](tactical-grid-overlay-spec-panel-2026-08-30.md),
> revisione del kit *Tactical Grid Overlay — Issue-Driven Execution*
> ([archiviato](../../archive/src/handoff/2026-08-30-tactical-grid-overlay-issue-driven.md)).

---

## 1. La sequenza

L'ordine **non** è quello del kit, ed è la differenza che vale il documento: il kit metteva il renderer al
nodo 1 e i test al nodo 5. Qui un test viene prima, perché lo chiede l'owner del nodo che lo segue.

| # | Nodo | Owner | Stato misurato | Dipende da | Exit gate |
|---|---|---|---|---|---|
| **0** | Audit del tracking e del codice | questo referto | ✅ **fatto** — 2026-08-30 | — | Nessun owner duplicato aperto: verificato su `hover`, `grid`, `overlay`, `bordo`, `griglia`, `pointer` |
| **1** | `PlayerInput.HoverNeverCommits` esiste e prova la regola | [#1766](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1766), sotto [#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705) | ⛔ **non scritto** — zero occorrenze in `Source/`; i `PlayerInput.*` reali sono **16**. ⏳ **owner aperto il 2026-08-30** | — | I due test sono verdi e **falliscono** se li si muta; `rt-suite.ps1 -Filter RefactorTactics.PlayerInput` passa da **16** a **18** |
| **2** | L'hover si vede | [#1614](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1614) | ⏳ aperta, P1 — premessa corretta il 2026-08-30 | **1** | Le 12 caselle di #1614 + `PIE-V01-POINTER` eseguita |
| **3** | La griglia si vede in partita: confine fra celle + toggle | [#1758](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1758) | 🟡 **codice a terra** — `85b9935f` su `main` (PR #1841, #1857): `CellBorders` è la settima famiglia ISM, ON di default. ⏳ **Resta il giudizio a schermo** | — (indipendente da 1–2) | `PIE-GRID-CONFINE`, seduta **U35** — quattro giudizi che nessun test può dare |
| **4** | `FRTCellId` si legge a schermo in Development | casella di [#1614](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1614) | ⛔ **nessun consumatore** — `grep HoveredCell` in `Debug/` e `UI/` dà zero | **2** | Hovered `(X,Y,Layer)` e centro world visibili in PIE Development |
| **5** | Selezione coerente col contratto | [#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705) | 🟡 **parziale** — nella DoD dell'owner documentale: **7** caselle spuntate, **10** aperte | **1** | DoD §7 di [`spec-pointer-interaction.md`](../../technical/systems/spec-pointer-interaction.md); ⛔ cinque delle dieci aperte sono bloccate da [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613) e [#74](https://github.com/DegrassiAaron/refactor-tactics-main/issues/74) e **non** sono di questa feature |

🔴 **Il nodo 1 viene prima del nodo 2, e non è pignoleria: lo dichiara #1614 stessa.** Metà della sua DoD —
*«cambiare hover non modifica selezione, waypoint, azione armata o intent»* — è la regola che
`HoverNeverCommits` dovrebbe provare, e un overlay più grande rende **più visibile** un comportamento che
nessuno sta misurando. Il test viene prima, o almeno insieme.

🟢 **Il nodo 3 non dipende da 1–2 e può correre in parallelo**: disegna la board a riposo, non la cella sotto
il cursore. È l'unico ramo di questa sequenza che si può iniziare oggi senza precondizioni.

---

### Il nodo 1 copre DUE test, non otto — misurato il 2026-08-30

Il perimetro di §7 della spec ne elenca otto. Verificati uno per uno su `9434b950`, solo **due** si scrivono
su comportamento che già regge; gli altri sei non sono lavoro rimandato per stanchezza, e ognuno porta chi lo
blocca:

| Test | Stato | Blocco |
|---|---|---|
| `HoverNeverCommits` · `RightClickCancelsPreviewOnly` | ✅ **in #1766** | — `SetHoveredCell` (`RTHexMapActor.cpp:806`) e `ApplyBack()` (`RTPlayerController.cpp:1466`) |
| `PlaybackRejectsPlanningInput` | 🟡 **feature travestita da test** | `ResolutionPlayback` è *prodotto* e **nessuno lo consuma**: unica occorrenza `RTPlayerController.cpp:1415` |
| `HiddenEnemyCannotBecomeHoverTarget` | 🟡 **feature travestita da test** | `PlayerTick` non filtra per percezione. È §6.1 — la **privacy** — e merita il proprio owner |
| `HUDConsumesPointerBeforeWorld` | ⛔ | [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613) — `AddHitBox` **0** occorrenze |
| `AllyGhostIsReadOnly` | ⛔ | CP 11.5/11.6 — `AllyIntentGhost` **0** occorrenze |
| `ReactionWindowOwnsInputPriority` | ⛔ | E14 — il contesto non è mai prodotto |
| `LogicalMapObjectResolvedFromStableId` | ⛔ | [#74](https://github.com/DegrassiAaron/refactor-tactics-main/issues/74) — `bMapElement` **zero produttori** |

⚠️ **I due 🟡 sono il candidato naturale per il nodo successivo**, e non sono equivalenti:
`HiddenEnemyCannotBecomeHoverTarget` è la privacy del contratto, quindi vale più di un test.

---

## 2. Cosa è già consegnato — e per questo non è un nodo

Il kit lo trattava come lavoro da fare. Misurato, non lo è.

| Il kit chiede | Stato | Evidenza |
|---|---|---|
| «nessun Actor per cella» | ✅ **invariante d'origine** | doc-comment di `ARTHexMapActor`: *«genera un'ISTANZA per cella (ISM), NON un Actor per cella»* |
| multilayer: due celle stesso `X/Y`, `Layer` diverso restano distinte | ✅ **consegnato** | `ERTLayerViewMode{AllLayers, ActiveOnly, Focus}` + `ActiveLayer` + `GhostLayerRange` (#567) |
| «layer nascosti non pickati accidentalmente» | ✅ **per costruzione** | i piani di contesto **non producono istanze**, quindi non hanno collisione: il raycast non li può colpire |
| il pick non deve confondere componenti | ✅ **chiuso** | `IsPickOnSelectableCell` guarda il **componente**, non l'actor — #588, PR #598 |
| conversione `World → FRTCellId` | ✅ **chiuso** | #33 (CP 2.3) |
| un toggle esiste | ✅ **per lo sviluppatore** | `rt.Debug.DrawCells`, uno dei **26** comandi `rt.*` — il toggle *per il giocatore* è dentro il nodo 3 |

---

## 3. Perimetro — cosa NON entra

- ❌ **La griglia di lavoro dell'editor** è [#622](https://github.com/DegrassiAaron/refactor-tactics-main/issues/622),
  sotto #1105, stadio TD 0.1: riguarda **dove le celle non esistono**. Altro mestiere, altro owner.
- ❌ **L'overlay della LOS** è [#1712](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1712).
- ❌ **Il settore sotto il cursore** è [#1615](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1615).
- ❌ **Lo Screen HUD** è [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613); il
  contenitore di un eventuale toggle a schermo è suo, non di questa sequenza.
- ❌ **Nessuna Epic nuova.** Il perimetro ha già i suoi owner: **E21 #286** per ciò che sta in scena, **E11 #25**
  per hover, puntatore e comandi `rt.Debug.*`. Il Caso A del kit era quello vero.

---

## 4. Le tre precondizioni che mordono

Sono scritte qui perché ognuna ha già prodotto un difetto reale in questo repository.

1. 🔴 **Nessuna quota in centimetri assoluti.** Tutto ciò che si posa sulla faccia di una cella sta **sopra
   `RTCellTopZ`** (`7,5` uu) e riusa le costanti di `RTHexMapActor.cpp:124-128`.
   [`RTMapVisuals.h`](../../../Source/RefactorTactics/Map/RTMapVisuals.h) dichiara che *«chi disegna qualcosa
   SOTTO `RTCellTopZ` lo disegna dentro un volume opaco»* e registra **due** episodi; un terzo è nella voce
   `PIE-DEBUG-CELLS` di [`test-manuali-pie.md`](../../technical/test-manuali-pie.md) — contorno a `z = 2.0`
   contro faccia a `2.5`, *«sepolto nella mesh»*. ⚠️ **E `RTCellTopZ` è salito da `2,5` a `7,5` il
   2026-08-28**, quindi un numero riscritto a mano oggi sbaglia di più di allora.
2. 🔴 **Il fill translucido copre due canali che portano informazione.** Il glifo ad anelli si legge come
   contrasto d'area (`9,7%`–`32,9%` della faccia, `D-183`) e il velo distingue il *ricordo* dall'*osservato*
   moltiplicando l'RGB per `0,35` (`D-225`, `D-227`). L'opacità va verificata **contro il ricordo velato**,
   non solo contro il terreno pieno.
3. 🟠 **`G` è occupato** da `Action.Guard` (`RTPlayerController.cpp:201`): un toggle di presentazione non
   sovrascrive un binding di gameplay.

---

## 5. Rapporto con la roadmap canonica — e perché non è stata toccata

[`roadmap-v0.1.md`](../roadmap-v0.1.md) porta **CP 11.8** alla riga `887`, con la colonna evidenza già
riconciliata il 2026-08-29: elenca i **6** test `PlayerInput.*` che esistono e i **10** dichiarati e non
scritti. Aggiungere lì una feature «Tactical Grid Overlay» accanto a CP 11.8 creerebbe una **seconda
descrizione** dello stesso perimetro, che è il difetto contro cui quella riconciliazione è stata fatta.

Questo documento **punta** a quella riga invece di duplicarla. Se la sequenza qui sopra si chiude, ciò che
cambia in `roadmap-v0.1.md` è la colonna evidenza di CP 11.8 e la riga di **E21** — non una voce nuova.

⚠️ **E il nodo 1 è il pezzo che quella colonna già dichiara mancante**: chiudendolo, `HoverNeverCommits` esce
dall'elenco dei dieci «dichiarati e non scritti». Nessun altro nodo di questa sequenza tocca quel conteggio.

---

## 6. Gate della feature — e quello che NON è un gate per nodo

| Gate | Quando | Nota |
|---|---|---|
| `./scripts/rt-suite.ps1 -Filter RefactorTactics.PlayerInput` | nodi 1, 2, 4 | Filtro mirato, molto più veloce della suite intera |
| `./scripts/rt-suite.ps1` | prima di chiudere ogni nodo | Il referto vale solo se la run è dichiarata **VALIDA** (`D-222`) |
| Build Editor `Development` | prima di ogni PIE | L'Editor deve essere **chiuso**, o il link fallisce con `LNK1104` |
| PIE su `L_DevSandbox` | nodi 2, 3, 4 | Voce registrata in [`test-manuali-pie.md`](../../technical/test-manuali-pie.md) **e collocata in una seduta** |
| Packaged Development | ⛔ **non per nodo** | Vedi sotto |

⛔ **«Packaged verificato» non è un gate di questi nodi, ed è misurato perché.** La suite è **Editor-only per
costruzione**: [`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) `G2` registra **1373**
dichiarazioni `EditorContext` contro **11** `ClientContext` e `0` `ApplicationContextMask`. Un test nuovo di
questa feature nascerebbe `EditorContext` e **non sarebbe raggiungibile** da un binario impacchettato. Il
packaged qui è `G2`/`G13`, gate di release, con la sua storia già scritta — `-nullrhi` vietato sui dati cotti,
e un crash GPU allo shutdown che impedisce di leggere il verdetto dall'exit code.

---

## 7. Prossimo passo

**Nodo 1** — scrivere `RefactorTactics.PlayerInput.HoverNeverCommits`. È la sola precondizione di due nodi su
cinque, vive sul percorso headless che `HandleClickOnCell`/`HandleClickOnUnit` già espongono, e non aspetta
nessun altro owner.

🔁 **Aggiornato il 2026-08-31.** Il nodo 3 non è più «può iniziare oggi»: il suo codice è su `main` da
`85b9935f`. Ciò che resta è **`PIE-GRID-CONFINE` nella seduta U35** — quattro giudizi a schermo — e non è
lavoro di programmazione. Il nodo 1 resta il prossimo passo di codice della v0.1.

---

## 8. La v0.2 — semantic area overlay

> Aggiunta il **2026-08-31** dalla revisione di *RT — Tactical Grid Overlay v0.2 — Implementation Spec*.
> La v0.2 **supera** la v0.1 su semantic overlay, palette, boundary volumetrico, Primary/Secondary,
> priorità dinamica, certainty, source, multilayer, persistenza e transition timing. Non la sostituisce sui
> nodi 1–5: quelli restano v0.1 e restano dove sono.

### 8.1 L'ownership — e perché non nasce un'Epic

**Nessuna Epic nuova**, come già §3. Il dominio è *presentazione della mappa*, e l'owner vivo si chiama
letteralmente così: **[E49 #1769](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1769) ·
Tactical Camera & Map Presentation**. Possiede già le due precondizioni che il perimetro v0.2 dichiara
obbligatorie — il **multilayer** ([#1775](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1775))
e il **picking layer-aware** ([#1776](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1776),
`D-255`) — e ha già il precedente dichiarato di assorbire ownership senza creare Epic.

⚠️ **E la grid base NON si sposta lì.** #1758, #1614 e #705 restano sotto E21/#289 e CP 11.8: sono `v0.1`,
sono in corso, e riassegnarle sposterebbe lavoro vivo per simmetria.

### 8.2 La sequenza v0.2

| # | Nodo | Owner | Dipende da | Exit gate |
|---|---|---|---|---|
| **6** | Modello dell'area semantica + palette centralizzata | [#1941](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1941) **OVL-01** | — | Gate di distinguibilità sulle 7 tinte **e** contro le 9 superfici; `D-nnn` che chiude le tre collisioni |
| **7** | Perimetro di regione + ribbon volumetrica | [#1942](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1942) **OVL-02** | **6** | Concava · buco · disconnessa headless; nessun edge interno; `PIE-*` in seduta |
| **8** | Priorità dinamica + Primary/Secondary | [#1943](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1943) **OVL-03** | **6** | 1 Primary / max 2 Secondary contati da un test; `D-nnn` sul tratteggio con #172/#173 |
| **9** | I sette significati, privacy e costo | [#1944](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1944) **OVL-04** | **7**, **8** | 60 FPS misurato; zero Actor per cella/edge; nessun overlay da intenti avversari |

🟢 **I nodi 7 e 8 sono paralleli**: entrambi dipendono solo dal 6, e non l'uno dall'altro. È l'unico
parallelismo reale di questa metà.

⛔ **Le Slice «Base Tactical Hex Grid» e «Pointer/Cell Interaction» del kit v0.2 non compaiono qui, ed è
misurato perché**: la prima è il nodo 3 e il suo codice è su `main`; la seconda è il nodo 2 (#1614) e il
nodo 5 (#705), già in questa sequenza. Riproporle come nodi nuovi sarebbe una seconda descrizione dello
stesso lavoro.

### 8.3 I tre conflitti che l'audit ha trovato — e che nessun nodo può ignorare

Non sono rischi ipotetici: sono cose già scritte nel repository che la spec v0.2 contraddice.

1. 🔴 **Tre colori sono già occupati.** `DrawPlanningPreview` (`RTHexMapActor.cpp:1023-1110`) assegna
   **arancione** `FColor(255,150,30)` al fuoco amico, **ciano** `FColor(40,220,220)` alla traccia del
   percorso e **giallo** all'hover. La v0.2 li vuole per Hazard, Vision/LOS e Objective. Il fuoco amico ha
   una voce PIE **verde** con un giudizio umano registrato — *«sì, si capisce»* — e riassegnarne il colore
   invalida quella verifica. Owner della decisione: **#1941**.
2. 🔴 **Il tratteggio ha già un proprietario.**
   [`brief-planning-visuale.md`](../../technical/systems/brief-planning-visuale.md) §2 A3 lo assegna alla
   **certezza** (*previsto = tratteggiata*); la v0.2 lo assegna ai **Secondary** e vieta di usarlo per
   `Uncertain`. I due canali sono diversi — traiettorie contro edge di esagono — ma la grammatica che il
   giocatore impara è una sola. Owner della decisione: **#1943**, insieme a #172/#173, entrambe aperte.
3. ⚠️ **«Depth Test ON» contraddice un incidente già pagato.** `DrawPlanningPreview` usa `SDPG_Foreground`
   *deliberatamente*: l'arancione del fuoco amico *«finiva SEMPRE sotto un cilindro, e l'avviso che deve
   arrivare prima del lock-in non arrivava mai»* (PIE, 2026-08-08). Una ribbon con depth test ON dietro
   un'unità è invisibile esattamente dove serve. Owner della decisione: **#1942**, e l'oracolo è una voce
   PIE, non un test.

### 8.4 Rapporto con la roadmap canonica — invariato

[`roadmap-v0.1.md`](../roadmap-v0.1.md) **non è stata toccata**, e per la stessa ragione di §5: la famiglia
OVL è `post-v0.1`, come E49. Quando i nodi 6–9 si chiuderanno, ciò che cambia lì è la riga di **E49** — non
una voce nuova, e nessuna seconda roadmap.
