# Sequenza di chiusura della v0.1 — referto spec panel

> **Stato**: `CURRENT` · **Creato**: 2026-09-27 · **Tipo**: **vista di esecuzione**, non owner.
>
> **Base di misura**: `origin/main` = `988c6bc689bc2a8deb3461a56a945331aa0f1675` dopo `git fetch --prune`,
> albero pulito. Stato delle issue letto lato server con `gh` il **2026-09-27**.
>
> **Cosa è**: l'ordine in cui il lavoro residuo della v0.1 può essere affrontato, derivato **dai gate** e
> non dall'elenco delle issue aperte.
>
> **Cosa non è**: un'assegnazione, una riscrittura di scope, una seconda Definition of Done. Il criterio
> di consegna resta [`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) §3. Dove questo
> documento e un owner divergono, **ha ragione l'owner**.
>
> ⚠️ **Gate rieseguiti per questo referto**: `G1` (i tre target, **rossa**, §2.1) e `G9` (verde, §2.2).
> Tutti gli altri stati verdi citati sono **celle del DoD con la loro data**, non misure di oggi: senza
> esecutore automatico ([`D-182`](../../decisions/RT_PDR_00_Decision_Log.md)) un gate non è verde, è verde
> *a una data*.
>
> 🔴 **Questa è la seconda stesura, e la prima aveva difetti che una review avversariale ha trovato.**
> Quattro sono correzioni di merito, non di forma, e sono dichiarate in §5.7 invece di sparire nella
> revisione — fra esse **un comando pubblicato il cui output non riproduceva** e **un'affermazione
> negativa senza controllo positivo**.
>
> **Convenzione sui numeri** (`AGENTS.md` §14): nessun conteggio che cambi da solo. Dove serve una
> cardinalità, accanto c'è il comando che la produce; dove serve un insieme, ci sono i **nomi**.

---

**Indice** · [1. Il perimetro](#1-il-perimetro-non-è-la-milestone) · [2. Stato dei gate](#2-stato-dei-gate) ·
[3. Il grafo](#3-il-grafo-delle-dipendenze) · [4. La sequenza](#4-la-sequenza) ·
[5. Rilievi](#5-rilievi-del-panel) · [6. Le issue da aprire](#6-le-issue-da-aprire) ·
[7. Fuori sequenza](#7-cosa-non-è-in-sequenza) · [8. Limiti dichiarati](#8-limiti-dichiarati)

---

## 1. Il perimetro non è la milestone

Il criterio di consegna è la tabella §3 di [`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md):
`G1`–`G14` **più `G16`**. Non è un intervallo — `G15` è ⌫ dal 2026-08-21
([`D-181`](../../decisions/RT_PDR_00_Decision_Log.md)) e il numero non si riusa.

L'appartenenza alla milestone `v0.1` **non** rende una issue release-blocking: la milestone è un
*tracking container* ([`D-377`](../../decisions/RT_PDR_00_Decision_Log.md)). `roadmap-v0.1.md` dice
**cosa** appartiene alla release, il DoD **cosa deve essere vero**,
[#85](https://github.com/DegrassiAaron/refactor-tactics-main/issues/85) **attesta** che uno SHA la
soddisfa.

⚠️ **E l'esecutore di un gate non si cerca nel corpo delle issue.** Il legame `PIA-x → Gnn` è dichiarato
nel preambolo della tabella §3 e in [`roadmap-pia.md`](../roadmap-pia.md) §3, **non** nei corpi: una
ricerca full-text su `"G14"` non lo trova, ed è lo strumento sbagliato per la domanda. §5.7 dice come me
ne sono accorto.

---

## 2. Stato dei gate

| Gate | Stato | Esecutore | Residuo reale |
|---|---|---|---|
| `G1` build dei tre target | 🔴 **ROSSA** | — (nuova issue, §6.3) | **4 `C4996`** in `RTMatchWidgetAssetTests.cpp`, su Editor **e** Game Development. Misurata oggi, §2.1 |
| `G2` suite automation | 🟡 | nessuno dichiarato | le due metà non sono mai state misurate **sullo stesso candidate** |
| `G7` niente float in costi | 🟡 | ⛔ **nessuno** | la *revisione dei data asset* non è mai stata fatta |
| `G10` esito terminale dichiarato | ⏳ | [#38](https://github.com/DegrassiAaron/refactor-tactics-main/issues/38) · riporto di `PIA-4` [#2619](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2619) | una partita 2v2 **non degenere** |
| `G11` KPI registrati | ⏳ | [#84](https://github.com/DegrassiAaron/refactor-tactics-main/issues/84) | vedi §2.3: **non** è tutto in coda alle sedute |
| `G13` giocabile senza editor | 🟡 | [#2620](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2620) (`PIA-5`) | `input funzionante` e `UI leggibile`: **percettivi**, nessun log li dà |
| `G14` documentazione allineata | ⏳ | `PIA-2` [#2617](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2617) vi **riporta** · `PIA-0` #2615 ha già riportato (CLOSED) | ⛔ il gate **non ha un criterio falsificabile** (§5.2) — difetto diverso dall'assenza di esecutore |
| `G16` Labs & Playback | ⏳ | [#2601](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2601) | «Presentation» ([#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288)) e «Turn Log proiettato» ([#79](https://github.com/DegrassiAaron/refactor-tactics-main/issues/79) · [#1936](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1936)) |

⚠️ `PIA-6` [#2621](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2621) **non** è esecutore
di `G14`: è il *final acceptance report*, certifica i gate e quindi non può eseguire ciò che attesta.

### 2.1 `G1` — rimisurata oggi, ed è rossa

Tre build su `988c6bc6`, albero pulito, dal clone principale, macchina non contesa (zero processi
`UnrealEditor*`/`LiveCodingConsole*` misurati prima del lancio):

| Target | `Result` | `C4996` |
|---|---|---|
| `RefactorTacticsEditor Win64 Development` | `Succeeded` | **4** |
| `RefactorTactics Win64 Development` | `Succeeded` | **4** |
| `RefactorTactics Win64 Shipping` | `Succeeded` | 0 — i file di test non esistono in Shipping |

Le quattro sono accesso diretto a `UOverlaySlot::HorizontalAlignment`/`VerticalAlignment`
(`Source/RefactorTactics/Tests/RTMatchWidgetAssetTests.cpp`, righe 1475, 1476, 1478, 1488).

🔑 **Sono nuove rispetto alla misura che la cella dichiara.** `git merge-base --is-ancestor 4bb5c43d0 b0dc801a`
→ **falso**: il codice è entrato con `4bb5c43d0` (2026-09-12 **13:22**), **undici ore dopo** `b0dc801a`
(2026-09-12 **02:15**), il commit su cui `G1` fu rimisurata dichiarando «zero warning» su tutti e tre. Il
gate chiede *«senza warning nuovi»*: è rossa, e lo è da quindici giorni.

⛔ **E il primo tentativo di questa misura è stato un falso verde, per incrementalità.** La prima build
dell'Editor ha dato **0** warning perché il TU non era stato ricompilato: il file vive nel blob unity
`Module.RefactorTactics.19.cpp`, e solo cancellandone l'`.obj` in `Intermediate/` (gitignorato,
rigenerabile) le quattro sono comparse anche lì. **Una build incrementale non è una misura di `G1`**, e
questo referto lo dichiara perché ci è cascato.

### 2.2 `G9` — rimisurato, verde

Il conteggio del subset:

```bash
grep -c '^| \*\*PIE-[A-Za-z0-9.-]*\*\* `RELEASE-V01`' docs/technical/test-manuali-pie.md   # -> 17
```

⚠️ **La lettura dell'esito è a DUE passi, e comprimerla in uno dà un altro verdetto.** La cella `G9`
prescrive: si scorre la riga **dalla fine** fino al primo **campo** che porta un'icona fra `✅ 🟡 ❌ ⏳`,
e di quel campo si legge la **prima** icona. Chi cerca invece la prima icona incontrata scorrendo da
destra ottiene `7 ✅ · 10 ⏳`, perché molte celle *chiudono* con una nota `⏳` di prosa. Col metodo
corretto: **17 `✅` · 0 🟡 · 0 ❌** su `988c6bc6`.

Fuori dal subset restano non verdi, e il gate non li conta: `PIE-HEXPLAY-7` 🟡, `PIE-HEX-MODE-H` ❌,
`PIE-AS2` 🟡, `PIE-AS4b` ⏳, `PIE-AS4c` ⏳, `PIE-PREVIEW-PERSIST` ⏳, `PIE-PACING-1` ⏳,
`PIE-V01-REPLAY` 🟡, `PIE-VIS-SIGHTWALL` ❌.

⚠️ `PIE-VIS-SIGHTWALL` **non aspetta una seduta**: dal 2026-09-18 il suo owner è di **design**,
[#3176](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3176) (`D-426`), che ha dichiarato
insufficiente la rigiudicazione proposta da [#2476](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2476).

### 2.3 `G11` non è una coda unica, ed è la correzione che cambia la sequenza

La «Chiusura» di `#84` **decide** un criterio che la prima stesura di questo referto dava per aperto:

> *«Merge quando **ogni riga della §4 ha uno stato motivato** e le voci del perimetro di questo checkpoint
> sono ✅ con numero, data e metodo. ⛔ **Non «zero ⏳»**, che è il criterio precedente e non è
> soddisfacibile: […] sette ⏳ appartengono a **E13**, **E14** e **M10**.»*

∴ **`E13` e `E14` NON sono sul percorso critico della release.** Le loro righe restano ⏳ con ragione e
sbloccante dichiarati, ed è conforme.

Restano allora due produttori distinti, e solo uno è in coda alle sedute:

| Cosa serve a `G11` | Chi lo produce | Quando |
|---|---|---|
| le **quattro voci di performance** sulla build della v0.1 | seduta **`U16`** *Misura dei KPI* (`critical: true`, `issues: [84]`, `unblocked_by: [U6]`) | dopo `U6` |
| le righe di **ritmo** (durata, round, scala) | seduta **`U19`** | dopo `U6, U1, U5, U7, U8` |
| le quattro caselle **redazionali** di `#84` — stato motivato per ogni riga, serie e pendenza, riserve di metodo come voci, sede versionata dell'evidenza | nessuna seduta | **subito, a motore spento** |

---

## 3. Il grafo delle dipendenze

```text
#288 (E21.2 animazioni) ──→ U8 ─┐
#2477 (sei residui) ───────→ U5 ─┤
#449/#450/#451 CLOSED ─────→ U1 ─┼─→ U19 (ritmo) ────┐
#1719 CLOSED, PIE-AS2 🟡 ──→ U7 ─┤                    ├─→ #84 (G11) ─┐
U2 → U3 → U4 → U5 ─────────→ U6 ─┘                    │              │
                                 └─→ U16 (4 KPI) ─────┘              │
                                                                     ├─→ #85
#166 (CP 14.6) ──→ reazioni in partita ──→ #38 (G10) ────────────────┤
#288 + #79/#1936 ────────────────────────→ #2601 (G16) ──────────────┤
#2620 (PIA-5) ───────────────────────────→ G13 ──────────────────────┤
nuova issue C4996 ───────────────────────→ G1 ───────────────────────┤
nuova issue G7 · #2617 (PIA-2) → G14 ────────────────────────────────┘
```

`U6` è il **collo di bottiglia strutturale**: sblocca sia `U16` sia `U19`. Il collo di bottiglia di
*lavoro* resta **#288**, che alimenta `U8` e la voce «Presentation» di `G16`.

### I prerequisiti, misurati

| Seduta | Marcatore | Convocata da | Residuo |
|---|---|---|---|
| `U1` Mappa-arena hex | assente | #449, #450, #451 — tutte **CLOSED** | `PIE-HEX-MODE-H` ❌, con [#931](https://github.com/DegrassiAaron/refactor-tactics-main/issues/931) **CLOSED**: correzione atterrata, verdetto mai rigiudicato |
| `U2` Partita hex, primo giro | assente | #38, casella 3 | — |
| `U3` Input e pianificazione | assente | #38, casella 3 | `PIE-PREVIEW-PERSIST` ⏳ |
| `U5` Bot e HUD | assente | #38, casella 3 · [#2477](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2477) | i sei residui di #2477 |
| `U6` Multilivello e partita completa | assente | **#38, casella 3** | il `done_when` sono le quattordici voci del perimetro E2: **tutte `✅` tranne `PIE-HEXPLAY-7`** |
| `U7` Personaggi Paragon | assente | #1719 **CLOSED** | ⚠️ **non è «—»**: il `done_when` chiede esito su tutti e quattro gli eroi e `PIE-AS2` è 🟡, con la seconda metà rinviata a `U8` |
| `U8` Animazioni | assente | [#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288) · [#2521](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2521) | `PIE-AS4b` e `PIE-AS4c` non hanno mai avuto un esito |

Criterio del marcatore: `(?:✅|🟡)\s*\*\*(Eseguita|CHIUSA|Chiusa|Parzialmente)`, cercato in **tutti** i
campi della seduta, non nel solo `notes`.

⚠️ **`PIE-PACING-1` è una precondizione di `U19`, non una sua misura.** Il record della seduta prescrive
di eseguirla **prima** della partita cronometrata, su un turno da buttare: un suo rosso **invalida** le
tre misure che `G11` consuma. È ⏳.

---

## 4. La sequenza

### Onda 1 — quattro fronti indipendenti, a motore spento o quasi

| # | Lavoro | Perché adesso |
|---|---|---|
| 1A | **nuova issue `C4996`** (§6.3) | `G1` è rossa. Quattro letture in un file di test: correzione meccanica, e finché non atterra ogni build della release porta warning nuovi |
| 1B | [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) CP 14.6 `P0` | l'unico `P0` vivo di `E14`. Sblocca la clausola di non-degenerazione di `G10`. I tre blocchi del suo vecchio riquadro sono caduti |
| 1C | [#79](https://github.com/DegrassiAaron/refactor-tactics-main/issues/79) + [#1936](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1936) | insieme sono **una** delle cinque capability di `G16` — «Turn Log proiettato al giocatore» — e sono l'unico residuo di `#2601` oltre a Presentation |
| 1D | le **quattro caselle redazionali** di [#84](https://github.com/DegrassiAaron/refactor-tactics-main/issues/84) (§2.3) | non dipendono da nessuna seduta né dal candidate. Toglierle dalla coda accorcia `G11` |
| 1E | **nuova issue `G7`** (§6.1) e **criterio di `G14`** (§6.2) | indipendenti da tutto il resto |

`1A` va per prima fra queste: tocca lo stesso `Source/` che tutte le build successive misurano.

### Onda 2 — [#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288), e poi le sedute

`#288` è codice più asset: apre la strada sia a `U8` sia a `G16`.

Poi le sedute, **una per volta — il motore è uno** — nell'ordine che i loro stessi campi dichiarano:

```text
U2 → U3 → U4(✔) → U5 → U6 → { U16 , U19 }
```

- `U5` chiude anche `PIE-HEXPLAY-7`, il cui residuo è un giudizio sul **comportamento** del bot (preferisce
  il riparo, il kiter tiene la distanza, la mischia chiude) e non una mancanza di codice;
- `U6` chiude quando quelle quattordici voci sono verdi insieme;
- `U1`, `U7` e `U8` sono in parallelo logico ma seriali sul motore. Per `U1` e `U7` il lavoro è
  **rigiudicare** (`PIE-HEX-MODE-H`, `PIE-AS2`) e scrivere il marcatore, non rifare la seduta;
- ⚠️ `PIE-PACING-1` **prima** di `U19`, su una partita da buttare.

🔑 La rigiudicazione ha già il suo runbook:
[`guida-seduta-rigiudizi-blocchi-caduti.md`](../../technical/runbooks/guida-seduta-rigiudizi-blocchi-caduti.md).
`PIE-HEX-MODE-H` (#931 chiusa) ne è un caso nuovo, da aggiungere all'elenco.

### Onda 3 — i gate che dipendono dalla build

Qui cade la regola di `#85`: *«la tabella si compila sul candidate, non si copia da `main`»*.

∴ **si congela lo SHA prima di misurare, non dopo.** Poi, sul candidate:

- `G1`, `G2` (entrambe le metà, **stessa passata**), `G12` — ricostruzione e riesecuzione;
- `G3`, `G4`, `G6`, `G8` — ⚠️ verdi grazie alla stessa run Editor del 2026-08-29 su `bbf0d780`: **vanno
  ridatati anche loro**, perché la clausola del candidate non fa eccezioni;
- `G10` → #38 · `G11` → #84 · `G13` → #2620 · `G16` → #2601 · `G14` → #2617 · `G7` → §6.1.

### Onda 4 — [#85](https://github.com/DegrassiAaron/refactor-tactics-main/issues/85)

Compilare la tabella di attestazione sullo SHA congelato, allegare l'evidence bundle, dichiarare le known
limitations, dichiarare la release.

---

## 5. Rilievi del panel

### 5.1 🔴 `G1` è rossa e nessuno la sta guardando

Misura in §2.1. Il fatto strutturale non è il warning: è che **fra due rimisure può passare qualunque
commit**, e qui ne sono passati quindici giorni. È la stessa forma che la cella `G1` denuncia di sé
quattro volte.

### 5.2 🔴 `G14` non ha un criterio che possa fallire

La colonna *Come si verifica* dice per intero: **«canone, roadmap, cataloghi, README, PIE senza
contraddizioni»**. Non nomina i documenti, non definisce «contraddizione», non dà un comando. Un gate
così non ha un momento in cui diventa verde.

⚠️ **Il difetto non è l'assenza di esecutore** — `PIA-2` #2617 vi riporta. È che l'esecutore non ha un
criterio da eseguire.

⛔ E non è teorico: questo referto ha trovato **`#38` che dichiara bloccanti due voci PIE oggi verdi**
(§5.3) e **`PIE-HEX-MODE-H` ❌ con la sua issue chiusa** (§3) — due contraddizioni dentro il perimetro
che `G14` dichiara di coprire.

### 5.3 🟡 Il corpo di `#38` nomina blocchi che non esistono più

Il blocco «DoD viva» del 2026-09-10 dichiara `PIE-HEXPLAY-6` ❌ e `PIE-HEXPLAY-8` 🟡. Misurati su
`988c6bc6`: **entrambi `✅`** — #2697, #2911 e #2549 sono CLOSED e
[`D-402`](../../decisions/RT_PDR_00_Decision_Log.md) ha ridotto il criterio di `-8`. Il blocco vero è
`PIE-HEXPLAY-7`, che il corpo non nomina.

### 5.4 🟡 `U1` e `U7` sono più fatte di come il registro le dà, e meno di come sembra

Le issue sono chiuse, il marcatore manca: chi conta i prerequisiti di `U19` col criterio canonico ne
legge cinque aperti su cinque. ⚠️ **Ma scrivere un marcatore di esecuzione piena sarebbe il difetto
opposto**: `U1` ha `PIE-HEX-MODE-H` ❌ e `U7` ha `PIE-AS2` 🟡. Vanno **rigiudicate**, non timbrate.

### 5.5 🟡 `G2` chiede due misure e non chiede che siano la stessa

Le due metà sono verdi su candidate diversi (`bbf0d780`, `33634aee`). ⚠️ La conseguenza è già impedita
dalla DoD di `#85`, che vincola ogni gate al candidate: il difetto è **la ridondanza mancante**, non un
buco aperto. La clausola *«sullo stesso candidate»* va comunque scritta nella colonna di `G2`, perché un
gate che dipende da una regola scritta altrove si legge verde da solo.

### 5.6 🟡 Il perimetro di ridatazione è più largo di com'è scritto

Otto gate portano un timbro datato; `G3`, `G4`, `G6` e `G8` vengono tutti dalla **stessa** run del
2026-08-29. Un piano che ne ridata tre li lascia arrivare a `#85` col timbro vecchio. §4 onda 3 li include.

### 5.7 ⌫ Quattro difetti della **prima stesura di questo referto**, corretti

Dichiarati invece di spariti, perché tre sono difetti di metodo e il metodo è riutilizzabile.

| Difetto | Come è stato trovato |
|---|---|
| 🔴 **Un comando pubblicato il cui output non riproduce.** Il referto scriveva che `gh search issues … '"G14"'` desse *«#26, #85 e #2621»*. Rieseguito: `"G7"` → `26 85` (niente #2621); `"G14"` → **30** righe al limite di default, **51** con `--limit 100`, in gran parte estranee | rieseguendo il comando invece di rileggerlo |
| 🔴 **Un'affermazione negativa senza controllo positivo.** *«Nessuna issue aperta nomina `U6`»*, fondata su `gh search 'U6 seduta'` → 0. Lo stesso comando dà **0 anche per `U5`**, che un convocatore ce l'ha (#2477, citato due righe sotto). Lo zero era del metodo. **`U6` è convocata da `#38`**, casella 3 | eseguendo il comando su un caso che *deve* dare non-zero |
| 🔴 **Una domanda dichiarata aperta che era già decisa.** Il referto dava `#84` come portatrice di due letture incompatibili di `G11`. La sezione «Chiusura» della stessa issue le arbitra esplicitamente (§2.3): la conseguenza è che **`E13` e `E14` escono dal percorso critico** | leggendo il corpo **intero**, non i due frammenti che si contraddicevano |
| 🟡 **Un metodo che non produce il proprio numero.** La regola di lettura di `G9`, come era scritta, dà `7 ✅ · 10 ⏳` invece di `17 ✅`: comprimeva in un passo una regola a due (§2.2) | rieseguendo il criterio scritto, non quello usato |

🔑 **Il denominatore comune**: tre su quattro sono *misure che non verificano sé stesse*. Un comando
pubblicato accanto al proprio output va rieseguito prima di pubblicarlo; un'affermazione negativa vuole
un controllo positivo; un criterio scritto vuole di essere eseguito **come scritto**.

---

## 6. Le issue da aprire

### 6.1 `G7` — l'unico gate davvero senza esecutore

⚠️ **La misura che lo dimostra non è la full-text search** (§5.7). È la lettura delle due fonti che
*possiedono* il legame gate→esecutore: il preambolo della tabella §3 del DoD e
[`roadmap-pia.md`](../roadmap-pia.md) §3. Nessuna delle due assegna `G7` a qualcuno, e la cella di `G7`
dichiara da due misure che la revisione dei `.uasset` non è stata fatta.

**Il gap, misurato**: `RefactorTactics.Catalog.NoFloatInIntegerFields` itera le `FProperty` riflesse di
**`FRTActionDef`** e pretende zero `FFloatProperty`/`FDoubleProperty`. Non tocca nessun'altra classe di
dati, e non tocca gli asset.

⛔ **Ma il controllo positivo ovvio non è costruibile, e va detto prima di scrivere la DoD**: `MoveCost`
(`RTHexCellData.h:380`) e `RoundLimit` (`RTMatchFormatData.h:35,118`) sono `int32`. Una `UPROPERTY int32`
**non può ospitare** un valore frazionario: «cerco un float scritto da un autore in un campo di costo» è
una ricerca che non può trovare niente per costruzione, e una revisione che non trova niente non si
distingue da una che non guarda.

∴ la domanda giusta non è *«qualcuno ha scritto 1.5?»* ma **«esiste un campo in virgola mobile
raggiungibile da un asset che alimenti un costo, una priorità, un danno o una soglia?»** — che si
risponde per **reflection sulla classe**, come già fa il validator, e il cui controllo positivo **è**
costruibile: si aggiunge un `float` a una struct di prova e il test deve fallire.

**Perimetro degli asset**, da rileggere col comando e non da questa riga:

```bash
git ls-files 'Content/**/*.uasset' | grep -E '/DA_'
```

⚠️ **`DA_Format_Scratch` non è un match format: è un `URTHexMapAsset`**, e il nome inganna. Misurato con
controllo positivo sul metodo — `grep -ac` sul binario dà `RTHexMapAsset` → 1, `RTMatchFormatData` → 0,
`RefactorTactics` → 2 (il metodo discrimina), `ZZNonEsiste` → 0. ∴ nessun asset versionato porta
`RoundLimit`.

### 6.2 `G14` — il criterio, non la revisione

L'issue non è «rileggere la documentazione»: è dare a `G14` asserzioni **falsificabili**, ciascuna col
proprio comando e col proprio controllo positivo.

⛔ **Due vincoli che la prima stesura sbagliava:**

- **un gate automatico esiste già**, e va riusato invece di riproposto:
  `node tools/radar/doc-links.ts --check` verifica che i percorsi citati dai documenti risolvano.
  [`D-188`](../../decisions/RT_PDR_00_Decision_Log.md) supera **la coda** di `D-182` — niente GitHub
  Actions, il gate si lancia a mano — quindi *«non si costruiscono gate automatici»* è falso come
  clausola;
- un'asserzione *«ogni `Gnn` nomina un esecutore vivo»* eseguita alla lettera **fallisce su `~~G15~~`**,
  che è un ritiro dichiarato. Un criterio che produce un falso positivo strutturale al primo giro è
  quello che viene disattivato al secondo: la riga barrata va esclusa esplicitamente.

### 6.3 `C4996` — il rosso di `G1`

Correzione: `Slot->HorizontalAlignment` → `Slot->GetHorizontalAlignment()` (idem verticale), quattro
occorrenze, tutte letture. ⚠️ Per la regola di indipendenza della misura, **correzione e verdetto sono
due momenti**: si corregge, si dichiara il commit, si rimisurano i tre target su quello — e la
rimisurazione dell'Editor **non deve essere incrementale** (§2.1).

### 6.4 Forma delle tre issue

Milestone `v0.1 — Offline Vertical Slice`; etichette `v0.1` + `P0` (`G1`, `G7`) e `v0.1` + `P1` (`G14`).
⚠️ Senza milestone ed etichette non compaiono nelle query che i loro consumatori usano.

**Non** sono numerate `CP 12.n`: `E12` porta già `CP 12.1`–`CP 12.6` e la tabella §3 di `roadmap-v0.1.md`
ne dichiara il conteggio, che dovrebbe tornare con le righe di §5. Seguono il precedente di `#2601`, che
esegue un gate senza essere un `CP N.M`.

⚠️ I link al presente documento nei corpi delle issue vanno scritti **dopo il merge** di questo file, o
puntano a un path che su `main` non esiste.

---

## 7. Cosa non è in sequenza

Fuori dal gate **per dichiarazione**:

- Map Editor [#1861](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1861), Certification
  [#814](https://github.com/DegrassiAaron/refactor-tactics-main/issues/814), PC Gym
  [#1859](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1859) — esclusi dal perimetro di `G16`;
- [#82](https://github.com/DegrassiAaron/refactor-tactics-main/issues/82) — uscita dalle dipendenze di
  `#85` il 2026-09-10; resta aperta come QA/hardening, `P2`;
- `E17` [#221](https://github.com/DegrassiAaron/refactor-tactics-main/issues/221) — è una **misura**, non
  un gate;
- **`E13` e `E14` come epic intere** — le loro righe KPI restano ⏳ motivate, ed è conforme alla Chiusura
  di `#84` (§2.3). Resta dentro il solo `#166`, e per `G10`, non per `G11`;
- `G16` non è nel perimetro di riporto di PIA, che mappa su `G1`–`G14`.

---

## 8. Limiti dichiarati

1. **Rieseguiti solo `G1` e `G9`.** Gli altri verdi sono celle datate, e l'onda 3 li ridata tutti.
2. **Lo stato delle sedute è prosa in uno YAML senza validator.** Il criterio è dichiarato, le righe sono
   stampate — ma nulla protegge quel file da una modifica che lo renda illeggibile.
3. **Ordine, non durata.** Le sedute PIE sono seriali per una ragione fisica: il motore è uno.
4. **Non ho verificato le sette caselle di `#84` una per una contro la §4 del DoD.** La ripartizione di
   §2.3 fra «redazionali» e «da seduta» viene dal corpo della issue; chi esegue l'onda 1D la rilegga.
5. **Il residuo di `#166` non è stato misurato**: è assunto dal suo corpo e dall'essere l'unico `P0` vivo
   di `E14`. Se fosse più grande di così, l'onda 1 si allunga.
