# GrayKit v0.1 — spec panel sul kit di consolidamento e sul suo mandato

> `CURRENT` · **Stato**: revisione chiusa. I due sorgenti sono **consumati e archiviati**, non applicati ·
> **Data**: 2026-08-28
> **HEAD della revisione**: `483e031a` (`main`)
> **Oggetto**: due file untracked, letti insieme perché sono un kit e il suo mandato di applicazione —
> `RefactorTactics_GrayKit_v0.1_Roadmap_CONSOLIDATED_2026-08-28.md` (360 righe) e
> `CLAUDE_Apply_GrayKit_v0.1_Consolidation_2026-08-28.md` (305 righe), letti contro `Source/`, le quindici
> issue lato server, il Decision Log, `docs/OPEN_DECISIONS.md` e le due spec owner che nominano.
> **Panel**: Wiegers (lead) · Cockburn · Fowler · Nygard · Crispin · Adzic
> **Modo**: critique
> **Archiviati in**: [`../../archive/src/handoff/2026-08-28-graykit-v01-roadmap-consolidated.md`](../../archive/src/handoff/2026-08-28-graykit-v01-roadmap-consolidated.md)
> · [`../../archive/src/handoff/2026-08-28-graykit-v01-apply-mandate.md`](../../archive/src/handoff/2026-08-28-graykit-v01-apply-mandate.md)

---

## 1. Il verdetto in una riga

I due kit sono **disciplinati sull'ownership e scaduti sullo stato**: la regola §0 — *«questa roadmap non
deve diventare un nuovo owner permanente»* — è la cosa migliore che abbiano, e i loro divieti reggono quasi
tutti; ma **la decisione centrale su cui poggia il nodo 4 è stata presa, con esito opposto a quello che i
kit presuppongono**, e due dei sei nodi della roadmap sono issue chiuse.

| | Voci |
|---|---:|
| 🔴 Critico | **4** |
| 🟠 Alto | **4** |
| 🟡 Medio | **5** |

**Raccomandazione operativa**: **non eseguire i due kit come scritti.** `C1` da solo produce l'errore
peggiore possibile per un kit di consolidamento — *omettere* un artefatto che una decisione d'autore rende
necessario, credendo di rispettare una cautela. Le correzioni sono tutte di testo, e tre di esse
**riducono** il lavoro: due nodi su sei sono già chiusi.

⚠️ Nessuna suite eseguita, nessuna build, nessuna scrittura su GitHub. Issue lette lato server con `gh` a
`483e031a`; `Source/` e `docs/` con `git grep` sullo stesso `HEAD`.

---

## 2. 🔴 C1 — `D-225` è decisa, e i kit invertono il suo esito

Entrambi i kit trattano `D-225` come pendente e ne derivano un **divieto**:

| Kit | Cosa dice |
|---|---|
| Roadmap §2 `#1467` | *«Resta una decisione di scope (`D-225` riservata nel thread)»* → *«**Azione:** non modellare `GB_FOW_CellFull` finché la decisione di scope non abilita uno stato `Unseen` realmente nascosto»* |
| Roadmap §1.5 | *«`GB_FOW_CellFull` solo se la decisione di scope abilita il vero nascondimento»* |
| Mandato PASSO 6 | *«Chiudi/scrivi la decisione D-225 prima di produrre `GB_FOW_CellFull`»* |

**Misurato nel Decision Log**, voce `D-225`, **Accettata — decisione d'autore (2026-08-28)**:

> **«La fog of war della v0.1 NASCONDE la geometria che nessuno della squadra osserva, e la scelta non costa
> più del velo perché a entrambi manca lo stesso anello.»** […] ✅ **Rende necessaria l'intera famiglia
> `GB_FOW_*`, `CellFull` incluso, che nell'uscita «velo» non serviva.**

**Cockburn**: l'esito è il **contrario** della cautela che i kit codificano. `CellFull` non è «da non
modellare finché»: è l'artefatto che la decisione rende **obbligatorio**. Un esecutore che segua il PASSO 6
alla lettera omette deliberatamente il proxy che il gioco ora richiede, e lo fa **credendo di essere
prudente** — che è la forma di errore più difficile da scoprire in review, perché l'omissione ha una
giustificazione scritta accanto.

🔴 **E «riservata nel thread» è scaduto di otto numeri.** L'ultimo `D-nnn` assegnato in locale è **`D-233`**.
Un kit che rivendica un ID «riservato» due giorni dopo descrive un registro che non esiste più — ed è
esattamente la classe di collisione che `CLAUDE.md` §7 impone di riverificare prima del merge.

**Correzione**: sostituire i tre passaggi con *«`D-225` è **accettata** (2026-08-28) e sceglie il
nascondimento: la famiglia `GB_FOW_*` va modellata **per intero**, `CellFull` incluso»*.

---

## 3. 🔴 C2 — Due dei sei nodi della roadmap sono issue chiuse

La roadmap §3 disegna sei nodi. **Misurati lato server**:

| Nodo | Issue | Stato reale |
|---|---|---|
| `0 · NORMALIZZAZIONE` | `#1174` · `#324` · `#286` | OPEN · OPEN · OPEN ✅ |
| **`1 · BOARD READABILITY`** | `#956` | ⛔ **CLOSED** — *«CP 47.3 · Grammatica visiva della board: colore E forma»* |
| `2 · GRAYKIT DIMENSIONS` | `#1095` | OPEN ✅ |
| `3 · INTERNAL GEOMETRY` | `#1239` | OPEN ✅ |
| **`4 · TEAM KNOWLEDGE`** | `#1467` | ⛔ **CLOSED** — *«E13.7 · Il velo»* |
| `5 · INTEGRATION GATE` | `#286` | OPEN ✅ |

E `#1094`, che la §2 tratta come da chiarire (*«**Azione:** non trattare #1094 come cinque decisioni ancora
aperte»*), è anch'essa **CLOSED**.

**Wiegers**: la §2 del kit si intitola *«Audit delle issue: stato vero da usare»* e produce una lista di sei
«già chiuse — non rifare» che è **corretta** (`#1155` `#619` `#620` `#621` `#832` `#1096`, tutte CLOSED,
verificate). Il difetto non è la mancanza di un audit: è che l'audit **si è fermato alla baseline** e non ha
ricontrollato i nodi del percorso critico. Un kit che misura ciò che dichiara acquisito e assume ciò su cui
manda a lavorare ha il rigore invertito.

⚠️ **Conseguenza operativa sul nodo 1**: `#956` è chiusa come `CP 47.3`, e la §6 azione 9 dice *«#956 →
resta board-readability gate»*. Se resta lavoro di leggibilità, ha un'altra sede — non quella.

---

## 4. 🔴 C3 — Il lavoro vero del nodo 4 non compare in nessuno dei due kit

`D-225`, la stessa voce che chiude lo scope, dichiara **dove sta il costo**:

> ⚠️ **Il lavoro vero non è la scelta ma l'anello mancante** — mappa inversa cella→istanza e aggiornamento
> per-istanza a runtime — e **non è stimato qui**: questa voce decide il *cosa*, non il *quanto*.

E lo misura: non esiste mappa inversa cella→istanza (**zero** riscontri in `RTHexMapActor.h`), e
`RebuildInstances` gira solo all'**allestimento** — i quattro chiamanti runtime in `RTGameMode.cpp` sono
`MakeFixtureArena` / `MakeTestArena` / `MakeDemoArena`, cioè creazione della mappa, non partita in corso.

**Nygard**: i kit descrivono il nodo 4 come *«modellare i proxy»* e *«il renderer deve leggere Team
Knowledge»*. Nessuno dei due nomina la mappa inversa, `RebuildInstances`, o il fatto che l'aggiornamento
per-istanza a runtime **non esiste**. È la differenza fra un lavoro di asset e un lavoro di runtime sul
componente che disegna la board: chi pianifica il primo e trova il secondo scopre a metà nodo che il costo è
di un'altra categoria.

**Correzione**: il nodo 4 dichiara come primo lavoro l'anello mancante, non i proxy. I proxy sono ciò che
l'anello permette di mostrare.

---

## 5. 🔴 C4 — `0.88 H` è un numero nuovo e scelto, e la decisione che lo governa dichiara «zero numeri nuovi»

I kit lo pongono in quattro punti come dato consolidato: roadmap §1.5 (*«altezza Full = **`0.88 H`**, pari a
220 cm»*), §4 DoD punto 11, mandato «Decisioni d'autore da preservare» punto 10, PASSO 2.

**Misurato**: `git grep "0.88" -- docs` non trova **una sola occorrenza** in un documento di testo — solo due
SVG archiviati e PNG binari, falsi positivi. `220` non esiste come altezza in `Source/` né nelle spec.
`LayerHeight = 250.f` è confermato (`RTHexMapActor.h`, `RTHexMapAsset.h`).

E `D-225`, che è la decisione di riferimento del FoW, si chiude con: **«Zero numeri nuovi»**.

**Wiegers**: `0.88` non è derivato da niente — non da `LayerHeight`, non da una soglia di leggibilità, non
da una misura di playtest. È un coefficiente **scelto**, presentato sotto la forma retorica di un vincolo
(*«`0.88 H` non cambia `LayerHeight`»*) che è vera e irrilevante: la cautela giusta sul campo sbagliato fa
sembrare misurato un numero che nessuno ha misurato.

È lo stesso difetto che `D-156` ha respinto per il Time Bank — *«reintroduceva il numero magico che `D-056`
aveva tolto»* — e la disciplina di questo repository su questo punto è esplicita e ripetuta.

**Correzione**: o `0.88` si deriva (da cosa? con quale criterio di leggibilità?), o si dichiara
`PROPOSED FOR PLAYTEST` con il confronto che lo promuove, o si toglie e l'altezza resta una scelta d'arte
non normativa. Le tre uscite hanno costi diversi; nessuna è «scriverlo in una spec come fatto».

---

## 6. 🟠 A1 — La granularità che i kit dicono di non inventare è già decisa in `Source/`

Roadmap §1.3: *«exact anchor granularity sul lato **non va inventata** durante il consolidamento: va
definita dall'owner geometrico e testata»*. Mandato punto 7: *«Non inventare la granularità se HEAD non la
decide già»*.

**Misurato** in `Map/RTGeometryGrammar.h`:

```cpp
static constexpr int32 RT_GeometryQuanta    = 12;   // suddivisioni per PUNTO NOTEVOLE della direzione
static constexpr int32 RT_GeometryMaxQuanta = 4 * RT_GeometryQuanta;   // = 48
```

più sei assi tattici (`ERTTacticalAxis::Deg0 … Deg150`) e la nota *«`Along` e `Offset` sono in quanti
(`RT_GeometryQuanta`), misurati rispetto ai punti notevoli»*.

✅ **La clausola del mandato è formalmente corretta** — *«se HEAD non la decide già»* — ed è la disciplina
giusta. Ma la prosa che l'accompagna afferma il contrario («va definita»), e un esecutore che legga la
roadmap §1.3 va a cercare una decisione d'authoring che **esiste da prima del kit**.

**Correzione**: sostituire «va definita» con il valore misurato e la sua sede. La domanda residua non è la
granularità: è se un `SideAnchor` cada su un punto raggiungibile da `Axis × Offset × Along` in quanti.

---

## 7. 🟠 A2 — La struttura citata ha sei campi, non quattro, e quello omesso è `Layer`

Entrambi i kit pongono la «domanda obbligatoria» del PASSO 3 su
`FRTGeometrySegment{Axis, Offset, AlongStart, AlongEnd}`.

**Misurato** (`Map/RTGeometryGrammar.h`): la struttura ha **`Axis` · `Offset` · `AlongStart` · `AlongEnd` ·
`Layer` · `WallType`**.

**Fowler**: `Layer` non è un dettaglio in un gioco dichiarato **hex multilivello**, con
*«Stessa semantica di `FRTCellId::Layer`»* scritto accanto al campo. Una domanda di rappresentabilità posta
su quattro sesti della struttura può concludere «non rappresenta» per un campo che c'è. E `WallType`
(`ERTHexCoverType::High`) è precisamente ciò che serve al kit quando parla di «muri, cover e porte».

⚠️ Nota di merito: il campo `Offset` porta già in commento *«Zero = il segmento passa per il centro della
cella»* — cioè **il `Center` di `Center → SideAnchor` è già rappresentato e documentato**. Il kit lo dice
(§1.3 di `#1239`: *«`Offset == 0` identifica il passaggio per il centro»*) e ha ragione.

---

## 8. 🟠 A3 — `GEO-4` esiste già, completa, e porta un collegamento che i kit non hanno

Mandato PASSO 5: *«se `GEO-4` esiste, aggiorna il trigger/owner; se manca, aggiungilo come decisione
aperta»*. Roadmap §2: *«registrare/tenere `GEO-4` come decisione separata»*.

**Misurato** in `docs/OPEN_DECISIONS.md`: `GEO-4` esiste, con la domanda, **due uscite argomentate** (a: LoS
resta cella-a-cella · b: la LoS consulta i segmenti), il perché non si deduce, e un innesco. ✅ La cautela
dei kit — «non dedurre `blocks LOS` da `blocks movement`» — coincide con la decisione.

🔴 **Ma la voce porta un collegamento che nessuno dei due kit nomina**:

> ⚠️ La stessa domanda vale per il **proiettile**, che oggi ha il suo owner in `#1392`, e le due **non vanno
> separate**: risposte diverse renderebbero visibile un bersaglio che non si può colpire.
> **Innesco**: il primo consumatore di LoS che legga geometria intra-cella — plausibilmente `E13`/`#1467`, o
> **`#1392` se arriva prima**.

Due conseguenze operative. La prima: uno dei due inneschi nominati, `#1467`, è **chiuso** (vedi `C2`) —
quindi chi eredita l'innesco va deciso. La seconda: `#1392` è in lavorazione **prima**, per decisione
d'autore del 2026-08-28 registrata in
[`reaction-outcome-preview-handoff-spec-panel-2026-08-28.md`](reaction-outcome-preview-handoff-spec-panel-2026-08-28.md)
§4. `GEO-4` va quindi riletta **con quella sequenza in mano**, non archiviata come «rinviata».

---

## 9. 🟠 A4 — Il nodo 3 costruisce authoring per un caso che non esiste ancora

`Center → SideAnchor` è il nodo 3 della roadmap e il PASSO 3-4 del mandato.

**Misurato**, e lo dice `OPEN_DECISIONS` citando `D-179`:

> 🔎 **Perché non è urgente.** Nessun contenuto versionato usa ancora il campo — `D-179` ha misurato
> `InteriorWall` in **0** dei 17 `.uasset` — quindi oggi non esiste una mappa in cui la domanda cambi una
> partita. Diventa urgente **insieme** al primo muro interno di produzione, non prima.

**Cockburn**: zero muri interni in diciassette asset significa che il nodo 3 estende un formato per un
autore che non l'ha ancora usato. Non è sbagliato in sé — un formato si prepara — ma **contraddice la
priorità** che i kit gli danno mettendolo in parallelo al nodo 2 sul percorso critico, e va detto: è
*dichiarato · trasportato · mai letto*.

**Correzione**: dichiarare il nodo 3 come lavoro di formato **non bloccante** per il gate v0.1, con il primo
muro interno di produzione come suo vero innesco — la stessa formulazione che `OPEN_DECISIONS` usa per
`GEO-4`.

---

## 10. 🟡 I cinque rilievi medi

| # | Dove | Rilievo | Evidenza | Correzione |
|---|---|---|---|---|
| **M1** | Roadmap §5 | *«rimuovere riferimenti vivi al Feature Registry se ancora presenti come authority»* — **già rimosso** da entrambe le spec owner | `grep -ci "feature registry"` → **0** e **0** | La clausola condizionale è giusta e va tenuta; il lavoro non esiste. Toglierlo dalla lista delle azioni |
| **M2** | Roadmap §5 | ✅ **Qui il kit indica lavoro reale**: l'occupancy 12+core **non** è nella spec di placement, benché `#619` sia CLOSED | `grep -c "12 settori\|12 sector"` su `spec-graybox-placement-contract.md` → **0** | Tenere. È il contributo più concreto dei due kit |
| **M3** | Roadmap §2 · Mandato PASSO 8 | ✅ **`#324` regge**: titolo `[EPIC v0.2]`, label `v0.1` | `gh issue view 324` | Tenere. Rilievo amministrativo corretto e a costo zero |
| **M4** | i due file | **Ridondanti fra loro**: roadmap §6 (10 azioni) e mandato PASSO 8 (8 azioni) sono la stessa lista con numerazioni diverse; §7 e PASSO 10 danno **due** commit plan diversi per lo stesso lavoro | lettura | Un solo elenco di azioni. Due liste divergenti per lo stesso lavoro sono la seconda source of truth che la §0 vieta — **dentro il kit stesso** |
| **M5** | Roadmap §2 `#1174` | *«`PIE-HEX-VIZ-BORDI` è stato rifatto ed è verde»*: la voce PIE **esiste**, ma il suo stato corrente **non è verificabile da riga di comando** — il documento ne porta sia un ❌ (2026-08-18) sia un ✅ in cronologia | `test-manuali-pie.md:553` | Trattare la chiusura di `#1174` come **verifica manuale in PIE**, non come fatto misurato. Il kit la presenta come acquisita |

---

## 11. Cosa regge, misurato

| Proprietà | Come regge |
|---|---|
| **§0 regola di ownership** | *«Questa roadmap non deve diventare un nuovo owner permanente»* più l'elenco degli owner canonici e l'auto-prescrizione di archiviarsi. È la sezione migliore dei due file, ed è il motivo per cui questo referto li archivia invece di conservarli |
| **Baseline «già chiuse — non rifare»** | ✅ **6 su 6 verificate**: `#1155` `#619` `#620` `#621` `#832` `#1096` sono tutte CLOSED lato server |
| **I quattro owner citati esistono** | `roadmap-v0.1.md` (2036 righe) · `spec-graybox-placement-contract.md` (977) · `spec-hex-geometry-authoring.md` (500) · `OPEN_DECISIONS.md` (1406). Nessun percorso inventato |
| **Le otto decisioni citate esistono** | `D-146` `D-163` `D-171` `D-172` `D-173` `D-179` `D-183` `D-189` — tutte presenti nel Decision Log |
| **`LayerHeight = 250` invariato** | Confermato in `Source/`, e `D-225` lo **vieta esplicitamente** di toccare. La cautela del kit coincide con la decisione |
| **12 settori ≠ 12 direzioni · occupancy ≠ visibility** | `D-225` lo ribadisce fra ciò che **non** autorizza: *«promuovere i dodici settori di `#619` a granularità di visibility — restano occupancy»*. I kit e la decisione dicono la stessa cosa |
| **Riusare `#1239`, niente issue radiale** | `#1239` è **OPEN** ed è davvero l'owner: `InteriorWalls`, `Offset == 0` per il centro, `D-179`, hash. Il divieto di duplicare è corretto |
| **Nessuna nuova Epic / nessuna seconda issue FoW** | Coerente con la disciplina del repository; e `C2` rende il divieto ancora più giusto di quanto il kit sapesse |
| **Le clausole condizionali** | *«se HEAD non la decide già»*, *«se ancora presenti»*, *«se la prova su main è ancora vera»*, *«HEAD vince e devi riportare la divergenza»*. È la forma giusta, e in tre casi su tre la condizione era **falsa** — il che dice che le clausole hanno funzionato e la prosa attorno no |

---

## 12. Cosa fare, in ordine

| # | Azione | Dove | Blocca l'esecuzione? |
|---|---|---|---|
| 1 | Registrare che `D-225` è **accettata** e sceglie il nascondimento: `GB_FOW_*` va modellata per intero, `CellFull` incluso (`C1`) | Roadmap §1.5 · §2 `#1467` · Mandato PASSO 6 | **sì** |
| 2 | Rimisurare i sei nodi: `#956` e `#1467` sono CLOSED, `#1094` pure (`C2`) | Roadmap §2 · §3 · §6 | **sì** |
| 3 | Dichiarare l'anello mancante — mappa inversa cella→istanza, aggiornamento per-istanza a runtime — come primo lavoro del nodo 4 (`C3`) | Roadmap §3 nodo 4 · Mandato PASSO 6 | **sì** |
| 4 | Derivare `0.88`, marcarlo `PROPOSED FOR PLAYTEST`, o toglierlo (`C4`) | Roadmap §1.5 · §4.11 · Mandato PASSO 2/3 | **sì** |
| 5 | Sostituire «granularità da definire» con `RT_GeometryQuanta = 12` e la sua sede (`A1`) | Roadmap §1.3 · Mandato PASSO 3 | no |
| 6 | Correggere la firma di `FRTGeometrySegment`: sei campi, `Layer` incluso (`A2`) | Mandato PASSO 3 | no |
| 7 | Rileggere `GEO-4` con il collegamento a `#1392` e decidere chi eredita l'innesco di `#1467` (`A3`) | Mandato PASSO 5 | no |
| 8 | Dichiarare il nodo 3 non bloccante, con il primo muro interno come innesco (`A4`) | Roadmap §3 nodo 3 | no |
| 9 | I cinque medi: togliere il Feature Registry dalle azioni, tenere l'occupancy nelle spec, tenere `#324`, unificare i due elenchi di azioni, declassare `#1174` a verifica PIE (`M1`–`M5`) | entrambi | no |

⚠️ **Nessuna azione tocca il codice.** Come per i due referti fratelli di oggi, i difetti non sono di
implementazione: sono premesse di stato in un documento che si rilegge a ogni sessione. Il costo di
correggerle ora è nove edit di testo; il costo di eseguirle è un proxy `CellFull` non costruito perché una
riga diceva di non costruirlo.

---

## 13. Nota di regime

**Le ancore sono simboli, ID e nomi di costante**, non numeri di riga: `D-222` è il regime e `HEAD` si è
mosso due volte durante la sola sessione che ha prodotto questo referto e il suo fratello
(`e3911eed` → `483e031a`).

**Nessun `D-nnn` riservato.** La revisione applica `D-225`, `D-179`, `D-163` e `GEO-4` a due documenti che
li ignorano o li invertono. ⚠️ E registra il difetto che ha trovato sul registro stesso: i kit rivendicano
`D-225` come «riservata nel thread» mentre l'ultimo assegnato è **`D-233`** — la verifica che `CLAUDE.md` §7
impone sui ref remoti prima del merge, qui non è stata fatta a monte.

**Le tre azioni sopravvissute alla misura sono state applicate** — vedi §14. Le altre quindici della
roadmap §6 e del PASSO 8 **no**.

⛔ **I sorgenti non sono stati modificati**: archiviati verbatim sotto un preambolo di verdetto, come
prescrive la loro stessa §0 e il PASSO 10 (*«archivia o elimina il precedente file standalone; non lasciare
due source of truth»*). È l'unico passo dei due mandati che questa sessione ha eseguito alla lettera.

---

## 14. Le tre azioni applicate, e cosa ha cambiato la misura

> Applicate il **2026-08-28** su richiesta d'autore, base `483e031a`. Ogni scrittura è stata **rimisurata
> prima** e **verificata lato server dopo**. Le altre quindici azioni dei due kit non sono state applicate.

| # | Azione | Esito |
|---|---|---|
| 1 | `#324` titolo → `[EPIC v0.1] E23 · Muri, porte e interaction graph` | ✅ fatto, verificato lato server, più un commento che registra `D-160` |
| 2 | `#1239` esteso con `Center → SideAnchor` | ✅ commento con il delta **misurato**; issue lasciata OPEN |
| 3 | `#1174` chiusa `completed` | ✅ chiusa `COMPLETED`, con commento che separa le due metà |

### 🔴 La rimisurazione ha cambiato la motivazione dell'azione 1

Il kit motivava il rename così: *«il titolo dice ancora `[EPIC v0.2]`, mentre label **e corpo** dichiarano
l'anticipazione a v0.1»*. **Il corpo dichiara l'opposto**, e in tre punti: owner
`docs/roadmap/roadmap-post-v0.1.md`, *«⚠️ Non si apre prima dei 15 gate della v0.1»*, *«**Questa epic non si
apre.**»*.

Anche `M3` di questo referto aveva verificato **titolo e label** e non il corpo: la conclusione reggeva per
una ragione che non era quella scritta. È la forma di [[criterio-applicato-a-meta]] — l'esito giusto trovato
con mezzo criterio.

**L'autorità vera è `D-160`** (*Accettata, 2026-08-17*): *«E23 — muri, porte e interaction graph — è
ANTICIPATA dalla v0.2 alla v0.1»*, con la clausola che scioglie il vincolo: *«E23 esce da quella milestone,
quindi il vincolo dei 15 gate non le si applica più»*. `roadmap-post-v0.1.md` l'ha recepita in tre punti; il
titolo era **l'ultimo dei cinque owner** che `D-160` elencava come contraddittori a non essere allineato.
Nessuno dei due kit nomina `D-160`.

✅ **Corretto il 2026-08-29** (era registrato come aperto nel commento a `#324`): due righe del suo corpo — *«non si apre prima dei 15
gate»* e *«questa epic non si apre»* — erano **false da `D-160`**, cioè da undici giorni, su GitHub, dove il
corpo è la copia più letta. Sono state riscritte su richiesta d'autore: ciascuna porta ora la propria riga
originale citata, la data in cui è diventata falsa e la clausola di `D-160` che la supera. L'argomento del
secondo paragrafo — *authoring anticipato* contro *logica di transizione* — **non è superato** ed è marcato
come tale: una correzione di stato non deve trascinare via un ragionamento che regge. Diff verificato:
**2 righe rimosse, 18 aggiunte**, nessun'altra riga del corpo toccata.

### ✅ L'azione 2 ha prodotto una misura che i kit chiedevano e non avevano

La «domanda obbligatoria» del PASSO 3 — *«rappresenta davvero tutto lo scope `Center → SideAnchor`?»* — ha
una risposta esatta in `Map/RTGeometryGrammar.h`, dove `RT_GeometryQuanta = 12` è **relativo al punto
notevole della direzione**: *«`Q` quanti significano esattamente il punto notevole, che sia un vertice
(raggio pieno) o un punto medio di lato (apotema)»*.

Con `Offset == 0` e sei assi **non orientati**, il delta è quantificato: **12 anchor raggiungibili** — 6
vertici e 6 punti medi. Un anchor sul lato che non sia uno dei dodici non è un problema di *granularità* ma
di **insieme delle giaciture**, che è una domanda diversa e più cara. Il commento porta il DoD del solo delta.

➕ E il dato che rende il nodo non bloccante è **cresciuto a favore**: `D-179` misurava `InteriorWall` in
**0** dei **17** `.uasset`; rimisurato oggi, gli `.uasset` versionati sono **99** e quelli che lo citano
sono ancora **0**.

### ⚠️ `M5` era una cautela di troppo

Questo referto declassava la chiusura di `#1174` a *«verifica manuale in PIE, non fatto misurato»*, avendo
letto nel documento sia un ❌ (2026-08-18) sia un ✅ in cronologia. **La riga della voce porta il proprio
esito**: `PIE-HEX-VIZ-BORDI` è **✅ 2026-08-20**, seduta U18, `#1013` — *«la metà che era impossibile è
diventata osservabile»*. Non andava rieseguita: andava letta nella tabella invece che nella prosa attorno.

Le due metà di `#1174` si sono chiuse in modi diversi, e il commento le separa: il fallback è **il prisma
esagonale generato, non più il cilindro** (codice); e la parte documentale si è risolta **per dissoluzione**
— `asset-map.md` non elenca una mesh di cella, e ora è corretto così, perché `GetCellPrismMesh()` la genera
e non esiste un asset da tracciare.

### ✅ `C3` verificato una seconda volta, e regge

Prima di chiudere `#1174` ho trovato cinque array `Last*VeilState` in `RTHexMapActor.h` — uno per ciascuno
dei cinque ISM che `D-225` nomina — e ho riverificato se l'anello mancante fosse già stato costruito:

- mappa inversa cella→istanza: **zero** riscontri;
- `InstanceCells` è *«mapping instance index → FRTCellId»*, cioè il **verso opposto** a quello che serve;
- `ApplyVeil` / `UpdateVeil` / `RefreshVeil`: **zero**.

Lo stato per-istanza del velo esiste; il meccanismo che lo consuma a runtime no. È una conferma in più di
`C3` e di `D-225`, non una smentita.
