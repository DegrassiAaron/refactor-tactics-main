# Seduta U25 + U35 — il kit graybox e la griglia, in una sola apertura

> `CURRENT` · **Stato**: protocollo, non ancora eseguito · **Scritto**: 2026-08-30
> **Copre**: le sei `PIE-GBX-*` · `GBX-1` e `GBX-5` ([#1095](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1095)) · `PIE-GRID-CONFINE` ([#1758](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1758))
> **Owner delle voci**: [`../test-manuali-pie.md`](../test-manuali-pie.md) — questo file dice **come**, quello dice **cosa** e **con che esito**.

**Perché una sola apertura.** `editor-sessions.yaml` lo raccomanda già da sé, in un commento su `U21`:
*«Quattro sedute condividono ora `L_DevSandbox` illuminato, ed è un argomento per aprirle nella stessa
apertura invece che in quattro»*. Con `U35` sono **cinque**, e due di esse — `U25` e `U35` — chiedono la
**stessa fixture**, `CoverYard`.

🔴 **E non è comodità.** `PIE-GBX-FIT` lo dichiara: `GBX-1` si decide provando valori di
`SafeInsetFraction`, e **due letture su scene diverse non sono confrontabili** — la seconda misurerebbe la
scena invece dell'inset. Lo stesso vale per il confine di cella: giudicare una griglia su una board e
ritararla su un'altra non è una taratura, è due opinioni.

---

## 0 · Preflight — cinque righe, e la seduta non parte senza

| | Condizione | Come si verifica |
|---|---|---|
| 1 | **`U21` eseguita**: `L_DevSandbox` è illuminata | [#623](https://github.com/DegrassiAaron/refactor-tactics-main/issues/623) è `CLOSED`. ⚠️ Una scena di leggibilità valutata al buio direbbe più sulle luci che sull'oggetto |
| 2 | Gli asset del kit esistono | `git ls-files 'Content/RT/World/Graybox/*'` → **7** |
| 3 | Il binario è di **questo** albero | Ricompila prima di aprire, se un'altra sessione ha toccato il motore. È la trappola del DLL stantio |
| 4 | La griglia di #1758 è nel binario | O `feat/1758-confine-celle-in-partita` è checkout-ato, o [#1841](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1841) è mergiata. **Senza, il blocco B non ha oggetto** |
| 5 | Nessun'altra sessione sul motore | `Get-Process -Name UnrealEditor-Cmd` vuoto. Un editor che apri tu **blocca le build delle altre sessioni** |
| 6 | La fixture `GrayKitYard` è nel binario | Compare nella tendina `FixtureId`, o `rt.Map.Fixture` la nomina nell'help. Arriva con [#1857](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1857) |

---

## 1 · Allestimento — si fa una volta, serve a entrambi i blocchi

### 1.1 La board — una riga, e c'è tutto

Sull'`ARTHexMapActor` di `L_DevSandbox`, nel pannello Details:

```text
FixtureId = GrayKitYard      →  premi GenerateFixtureIntoAsset
```

Oppure, per guardarla in partita invece che nell'editor, su `ARTGameMode`:

```text
Scenario To Run = Visual.Map.GrayKitYard   →  Play
```

Il risultato è un **esagono pieno di raggio 3, 37 celle**, e porta **tutti e quattro** i soggetti della
seduta:

| Cosa | Dove | Serve a |
|---|---|---|
| Copertura **alta** | bordo `(0,0,0)`–`(1,0,0)` | `PIE-GBX-COVER` |
| Copertura **bassa** | bordo `(0,1,0)`–`(1,1,0)` | idem — una riga di distanza, **è il confronto** |
| **Quattro** stati di porta | riga `r = -1`: `Destroyed · Open · Closed · Locked` | `PIE-GBX-DOOR` |
| Acqua e ghiaccio | `(-1,2)`+`(0,2)` e `(1,2)`+`(2,1)` | `PIE-GBX-SURFACE` |
| Coppie della **stessa** superficie | acqua\|acqua e ghiaccio\|ghiaccio | `PIE-GRID-CONFINE` |

> 🔑 **`GrayKitYard` è `CoverYard` con tre aggiunte, e la base identica è un requisito.** `PIE-GBX-FIT`
> dichiara che `GBX-1` si decide provando valori di inset, e **due letture su scene diverse non sono
> confrontabili** — la seconda misurerebbe la scena invece dell'inset. Le coperture stanno sulle **stesse
> celle** di `CoverYard`, quindi questa lettura si confronta con `PIE-HEX-VIZ-BORDI`.
>
> 🔑 **Due celle per superficie e non una**, e la seconda è il punto: `PIE-GBX-SURFACE` vuole due superfici
> **diverse** adiacenti, `PIE-GRID-CONFINE` vuole l'**opposto** — due celle della *stessa* superficie, dove
> il colore non dice dove finisce una. Con una cella per tipo il giudizio più difficile della griglia
> resterebbe non guardabile proprio nella scena costruita per guardarlo.
>
> ⛔ **`GenerateFixtureIntoAsset` SOSTITUISCE, non fonde.** Su un asset d'autore cancellerebbe la mappa e
> la rimpiazzerebbe. Verifica di essere sull'asset di sandbox prima di premerlo.
>
> ⏱️ **Fino al 2026-08-30 questo paragrafo diceva `FixtureId = CoverYard` e mandava al §1.3 per porte e
> superfici**, perché nessuna fixture le portava insieme: `CoverYard` non ha porte — lo dichiara il proprio
> corpo — e `RelayBasin` ne ha una sola, chiusa. Le si scriveva a mano nel Details.

### 1.2 Le tre distanze — numeri veri, non impressioni

Sono i valori di `ARTCameraPawn`, così la seduta si ripete invece di essere ricordata:

| Distanza | `ArmLength` | Da dove viene |
|---|---:|---|
| **ravvicinata** | `100` | `MinArmLength` — il fondo corsa dello zoom |
| **di gioco** | `450` | `MatchStartArmLength` — l'inquadratura **con cui la partita parte** |
| **tattica** | `4000` | `MaxArmLength` — il cielo dello zoom |

`ZoomStep` vale `150`: da `450` a `4000` sono ~24 tacche.

### 1.3 Le aggiunte a mano — solo ciò che la fixture non può dare

⚠️ **La fixture porta il DATO, non le mesh.** `SM_Graybox_*` sono asset da posare in scena e nessuna
fixture li posa: `PIE-GBX-COVER`, `-DOOR` e `-SURFACE` guardano **il kit**, quindi resta questa metà. Il
blocco B (la griglia) è invece interamente dato-derivato, e non richiede nulla di questo paragrafo.

**Le mesh del kit**, sopra il dato che la fixture ha già messo:

| Sul dato | Posa |
|---|---|
| le due coperture | `SM_Graybox_Cover_High` · `SM_Graybox_Cover_Low` |
| le quattro porte | `SM_Graybox_Door_Panel`, e su `Locked` **anche** `SM_Graybox_Door_Locked` |
| acqua e ghiaccio | `SM_Graybox_Surface_Water` · `SM_Graybox_Surface_Ice` |

**Volume** (per `PIE-GBX-VOLUME` e `-FIT`): posa `BP_Graybox_CellPlacementVolume` su almeno una cella.

> ⏱️ **Qui stava la trappola peggiore di questa seduta, e dal 2026-08-30 non esiste più.** Non esiste un
> tool porte: si scriveva `Cells[i].Doors` a mano nel Details, e poi **andava forzato il ridisegno**
> toccando una property dell'actor — `ActiveLayer` `0 → 1 → 0` — perché l'asset non notifica l'actor. Senza
> quel gesto si guardava la geometria **vecchia**, e la voce sembrava fallita mentre era solo non
> allestita. Nate dalla fixture, quel modo di sbagliare non c'è.
>
> ⚠️ **Resta vero per qualunque porta aggiunta a mano** oltre alle quattro: se ne tocchi una nel Details,
> il ridisegno va ancora forzato.

### 1.4 Come si guarda

- **Senza HUD, senza selezionare nulla, con `rt.Debug.DrawCells` a `0`.**
- Ogni voce si rilegge su uno **screenshot in scala di grigi** — `HighResShot 2` cattura, la desaturazione
  si fa fuori da Unreal, che è il modo verificabile. ⚠️ **Se lì la distinzione sparisce, il secondo canale
  non esiste e `D-146` è violata**: è un artefatto, non un giudizio.
- Si **modella e si guarda alla stessa scala**, `C = 2,60 m`: le due scale hanno smesso di divergere il
  2026-08-25 con [#1155](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155).

---

## 2 · Blocco A — il kit graybox (`U25`)

Ordine obbligato: `PIE-GBX-ZOOM` è l'ultima **per costruzione**, perché rilegge le altre cinque.

### A1 · `PIE-GBX-VOLUME` — la metà che conta è quella negativa

In viewport si vede il prisma esagonale col footprint sicuro e le guide verticali
`1.00 · 0.85 · 0.55 · 0.28 · 0.00 H`. Premuto **Play**: **non compare**.

Poi muovi un'unità **attraverso** la cella che lo contiene: il percorso non cambia.

> ⛔ Che si veda in editor è facile. Che **sparisca** in PIE è la proprietà: comparire in una build packaged
> *«sarebbe un difetto, non una funzionalità di debug»*. E il volume **non produce clearance** — se qualcuno
> ne legge l'inset come dato di simulazione, l'errore è quello e non la geometria.

### A2 · `PIE-GBX-FIT` — 🔑 qui si decidono `GBX-1` e `GBX-5`

Il `CellBound` sta **dentro** l'inset e non invade la cella vicina. L'`EdgeBound` è agganciato al **bordo
condiviso** e **sporge da entrambe le celle** — ⚠️ **che è corretto, non un difetto**: chi legge lo
sconfinamento come errore corregge la cosa sbagliata.

Vista **dall'alto e isometrica**, a scala di board canonica, con unità, cella, copertura e porta nella
**stessa scena** — è ciò che `D-283` prescrive.

| Domanda | Cosa registrare |
|---|---|
| **`GBX-1`** | quale **frazione di `C`** è il Safe Placement inset. Il kit proponeva `~90%` come baseline da validare |
| **`GBX-5`** | il rapporto **unità/cella**. Oggi è **46%** (cella `260 uu`, unità `120 uu`); il kit chiedeva `23%`, fattore **2** |

> ⛔ **`D-283` vieta di derivarli da clearance, gameplay geometry o altre costanti tecniche**: un numero
> preso in prestito *sembra* derivato e non lo è. Si guardano e si scrivono.
>
> 🔴 **`GBX-5` non ha rete**: nessun test legge `BaseMeshScale`. Portarlo a `(0.6, 0.6, 1.8)` lascia **tutta
> la suite verde**. E la modifica appartiene a `RT-FEAT-CHAR-PRESENTATION`, non a chi modella.
>
> ⚠️ La larghezza di un pannello di bordo è `0.92` del **lato**, non di `C`: confonderli sbaglia di `1,73×`.

### A3 · `PIE-GBX-COVER` — si legge in pianta **prima** che in alzato

Bassa e alta distinguibili **senza colore**, a tutte e tre le distanze.

| Canale | Bassa | Alta | Fattore |
|---|---:|---:|---:|
| **spessore** (del lato) — quello che regge in pianta | `0.10` | `0.20` | **2** |
| altezza (di `H`) | `0.28` | `0.85` | ~3 |

> 🔴 **Il precedente dice che questo può fallire, e come**: `PIE-HEX-VIZ-BLOCCHI` è ❌ dal 2026-08-20 perché
> la differenza fra due volumi stava nell'**altezza**, che la vista a picco proietta a zero
> ([#1246](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1246)). Guardare solo le altezze
> qui ripete lo stesso difetto.

### A4 · `PIE-GBX-DOOR` — quattro stati, `Locked` compreso

`Open` col pannello **fuori** dal passaggio · `Closed` dentro · `Destroyed` **rimosso** (stato terminale,
non richiudibile) · `Locked` dentro **più la traversa in rilievo**, che è una **mesh diversa** e non la
stessa ricolorata.

⚠️ La traversa va vista in scala di grigi a **tutte e tre** le distanze: se sparisce alla **tattica**, il
secondo canale non c'è e `D-146` è violata dal primo asset che il kit produce.

### A5 · `PIE-GBX-SURFACE` — acqua e ghiaccio a zoom **tattico**

Alla distanza **più lontana**, dove la cella occupa meno pixel: si dice quale è acqua e quale ghiaccio
**senza cliccare**. Poi si rilegge in scala di grigi.

⛔ Un ✅ qui **non chiude `D-146`**: acqua/ghiaccio è **una** delle coppie, e `Floor~Fire` resta aperta.

### A6 · `PIE-GBX-ZOOM` — la rilettura, su **quattro** coppie e non sei

Si ripete tutto a ravvicinata · di gioco · tattica, e nessuna categoria diventa illeggibile passando da una
all'altra.

| Coppia | Osservabile in v0.1 |
|---|---|
| unità | ✅ |
| copertura bassa vs alta | ✅ |
| porta aperta vs chiusa vs bloccata | ✅ |
| acqua vs ghiaccio | ✅ |
| muro vs muro sfondato | ⛔ **DEFER** — dipende da `RT-FEAT-MAP-STRUCTURAL`, `IDEA` su release `future` |
| intatto vs distrutto (macerie) | ⛔ **DEFER**, stessa ragione |

> ⛔ **L'esito deve scrivere che due sono fuori.** Una voce che tacesse farebbe leggere «contratto
> verificato» dove è verificato per **quattro coppie su sei**.
>
> 🔴 **Se la lettura non regge, si cambia la grammatica prima di aggiungere altri asset.** È l'unica
> prescrizione del kit sorgente che il contratto adotta senza emendarla, e vale più di qualunque asset già
> modellato.

---

## 3 · Blocco B — la griglia (`U35`)

`rt.Debug.DrawCells` resta a **`0`**: la griglia è ON di default, e accenderla con un comando sarebbe la
soluzione che #1758 esclude per nome.

Quattro giudizi, tutti su `PIE-GRID-CONFINE`:

| | Cosa guardare |
|---|---|
| **B1** | Dall'alto, su **due celle adiacenti dello stesso colore**: si dice dove finisce una. Poi a distanza **tattica** — è lì che un bordo sottile muore |
| **B2** | Il **glifo di superficie** resta leggibile. Il bordo ne copre il **36%** dell'anello esterno: quello che deve sopravvivere è il **conteggio** degli anelli (`D-183`), non la loro larghezza |
| **B3** | Il **ricordo velato** resta distinguibile dall'osservato (`D-227`, RGB × `0,35`) — serve una partita con fog of war, non la sandbox statica |
| **B4** | La griglia **non copre** l'anteprima di pianificazione, che le sta sopra (`RTLiftPreview = +2,5` contro `RTLiftCellBorder = +0,4`) |

> 🔑 **Se uno dei quattro cade, il numero da ritarare è `RTCellBorderThickness`** in
> [`RTMapVisuals.h`](../../../Source/RefactorTactics/Map/RTMapVisuals.h). Vale `0.02` per **scelta motivata,
> non per misura**: nessun test può dire se un bordo si legge, ed è questa seduta l'oracolo.
>
> ⛔ **Non riguardare ciò che l'automation copre già.** Che il toggle non muti snapshot, TurnLog o graph
> revision lo prova `RefactorTactics.HexMapActor.GridToggleChangesNothingButVisibility`. Un test manuale che
> sostituisse un controllo automatizzabile è il difetto che il registro PIE esiste per evitare.

---

## 4 · Registrare gli esiti — dove, e in che ordine

1. **Le sette voci PIE** → [`../test-manuali-pie.md`](../test-manuali-pie.md), colonna `Stato`, con la data
   e ciò che si è visto. ⚠️ Poi **rieseguire il comando canonico di conteggio** e aggiornare l'intestazione:
   *il numero si ricalcola, non si aggiorna a mente*, e va rieseguito **dopo** ogni modifica — due stesure
   precedenti hanno rotto l'oracolo senza che il totale cambiasse.
2. **`GBX-1` e `GBX-5`** → [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) § graybox, **chiuse** col
   valore osservato, più una nuova `D-nnn` nel
   [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md).
   ⛔ **Il `D-nnn` si riverifica sui ref REMOTI prima del merge**, non su `gh pr list`: fra il commit che
   prende un ID e l'apertura della sua PR c'è una finestra in cui l'ID è **preso e invisibile**. Il progetto
   ha già pagato diciassette collisioni.
3. **Le sedute** → `U25` e `U35` in [`../../roadmap/editor-sessions.yaml`](../../roadmap/editor-sessions.yaml).
   ⚠️ Chi esegue **un solo** blocco lo dichiara, invece di chiudere entrambe le sedute.
4. **Le issue** → #1095 e #1758 si chiudono **solo** dopo che i rispettivi esiti sono a verbale negli owner
   canonici. Poi si guarda #289 (CP E21.3) e infine #286 (E21) come parent di release, **senza introdurre
   nuovi checkpoint** — `D-153` li vieta quando esiste già un owner.

---

## 5 · Cosa questa seduta **non** decide

- **Non** archi e transizioni ([#1768](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1768)),
  provenance degli asset esterni (#1767), griglia di lavoro dell'editor (#622), hover (#1614), LOS (#1712).
- **Non** la clearance di CP 23.6: *quanto grande posso modellare un asset* non è *dove un'unità ci sta in
  piedi*, e leggere l'inset come dato di simulazione è l'errore che il contratto esiste per impedire.
- **Non** `D-146` per intero: quattro coppie su sei, e `Floor~Fire` resta aperta.
