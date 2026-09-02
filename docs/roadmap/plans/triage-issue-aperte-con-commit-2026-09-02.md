# Le issue aperte che hanno già un commit — triage misurato

> **Referto di misura**, non owner. Nessuna regola citata qui appartiene a questo documento.
>
> **Data**: 2026-09-02 · **Base**: `origin/main` `0f003daf` · **Metodo**: misura per issue, non deduzione
>
> ⛔ **Nessuna riga di `Source/` toccata.** Le due issue chiuse durante il triage lo sono state dopo aver
> misurato la loro Definition of Done contro `main`, voce per voce.

---

## 1. Il verdetto in una riga

> **Cinquantasei issue aperte hanno già un commit che le cita, e per quarantatré di esse il lavoro che
> manca non è codice: è qualcuno che guardi lo schermo. Restano tredici chiudibili misurando.**

E lo stato delle caselle di DoD **non è un segnale** — fallisce in tre modi diversi, tutti incontrati qui.

---

## 2. Perché esiste

Il 2026-09-02 una selezione di *«prossima issue da lavorare»* ha proposto
[#1712](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1712). Era **già implementata**:
`c77e0fd1`, lo stesso giorno, da un'altra sessione che non l'aveva chiusa.

Il difetto della selezione non era la query — la issue *era* aperta e senza assegnatario. Era la domanda
mancante: **«esiste già il codice che la chiude?»**.

Questo referto misura quante altre sono nello stesso stato.

---

## 3. Il metodo, e le sue tre trappole

Per ogni issue aperta si cerca il commit più recente `feat(N)` / `fix(N)` / `test(N)` / `docs(N)` in
`origin/main`. Quello è il **sospetto**. Non è la conclusione.

⚠️ **Un commit che cita la issue non la chiude**, e le ragioni sono tre, tutte misurate:

| Trappola | Esempio |
|---|---|
| `docs(N)` è una **nota**, non un'implementazione | 21 delle 56 |
| Il commit è **lavoro parziale** su un perimetro grande | `refactor(1818)` su un God Object di 10 412 righe |
| La DoD ha voci che **nessun commit può soddisfare** | 43 chiedono di guardare |

∴ La conclusione si prende **misurando la DoD contro il codice**.

### 3.1 🔴 La prima misura era truncata, e non se ne accorgeva

Il primo pass ha interrogato `gh issue list --limit 300`. **Le issue aperte sono 335.**

Il comando non fallisce e non avverte: restituisce 300 righe e tace. Il risultato era **47 issue su 300**,
e mancavano quattro voci di `v0.1` — #16, #80, #85, #159 — cioè proprio la coda che il limite tagliava.

🔑 **Un limite di paginazione produce una risposta plausibile e sbagliata**: 47 e 56 sono entrambi numeri
credibili, e nulla nell'output distingue il primo dal secondo. È lo stesso difetto già registrato per la
`search` di GitHub sopra le 100 righe.

### 3.2 Il criterio che distingue davvero: chi **produce**, non chi **nomina**

Misurato due volte nello stesso pass:

- `Modal` appare in `ResolveBack` e in un confronto — **non sono produttori**; il produttore vero è
  `GetPointerContext` (`RTPlayerController.cpp:1959`)
- `bMapElement` ha **tre** occorrenze dove una nota diceva zero: sono **due consumatori e la dichiarazione
  del campo**. La conclusione della nota reggeva; cambiava solo come misurarla

Contare le occorrenze avrebbe dato due volte il contrario del vero.

---

## 4. La misura

**335** issue aperte · **56** hanno un commit in `main` che le cita.

| Tipo dell'ultimo commit | # |
|---|---|
| `docs` | **21** |
| `fix` | 13 |
| `feat` | 10 |
| `test` | 7 |
| `wip` | 2 |
| `chore` | 2 |
| `refactor` | 1 |

### 4.1 Il vero collo di bottiglia

| Cosa serve per chiuderla | # su 56 |
|---|---|
| 👁️ **una seduta editor / verifica visiva** | **43** |
| 🔧 misurabile a macchina | 13 |

⚠️ **Il rilevatore delle caselle ne vedeva solo 27.** Le altre **15** dichiarano il gate visivo **fuori da
una casella** — in una sezione *«Test / verifica»*, o in prosa.
[#940](https://github.com/DegrassiAaron/refactor-tactics-main/issues/940) scrive
`⏳ PIE-V01-FRONTEND-RESULT — da creare`;
[#220](https://github.com/DegrassiAaron/refactor-tactics-main/issues/220) chiede *«`PIE-ICON-01` verde»*.
Nessuna delle due è una casella, ed entrambe bloccano.

➕ **E il rilevatore in prosa ne mancava ancora uno**: cercava `PIE`, `packaged`, *«a schermo»*, *«si
vede»*. [#1683](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1683) non usa nessuna di
quelle parole — dice *«è ciò che **solo l'editor** può dire»*. Trovato a mano, leggendo le quattordici che
il filtro dichiarava pulite.

∴ **Un contatore di caselle sottostima il gate del 37 %**, e ogni elenco di parole chiave che lo sostituisce
va verificato contro il complemento, non contro sé stesso.

---

## 5. Le quarantatré che aspettano una seduta editor

Raggruppate perché **una sola seduta ne chiude molte**: è il punto di questo referto.
La colonna dice **dove** è dichiarato il gate — `casella` lo rende visibile a un contatore, `prosa` no.

### `v0.1 · Leggibilità` — tredici

| Issue | Gate | |
|---|---|---|
| [#79](https://github.com/DegrassiAaron/refactor-tactics-main/issues/79) | prosa | CP 11.3 — Combat log con reason code |
| [#80](https://github.com/DegrassiAaron/refactor-tactics-main/issues/80) | casella | CP 11.4 — Comandi `rt.Debug.*` |
| [#172](https://github.com/DegrassiAaron/refactor-tactics-main/issues/172) | prosa | CP 11.5 — Ghost Timeline |
| [#220](https://github.com/DegrassiAaron/refactor-tactics-main/issues/220) | prosa | CP 20.3 — I widget consumano il catalogo |
| [#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288) | prosa | CP E21.2 — Animazioni di locomozione |
| [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613) | casella | CP 11.7 — Screen HUD in UMG |
| [#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705) | casella | CP 11.8 — Pointer Interaction Contract |
| [#1095](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1095) | casella | il volume di posa della cella |
| [#1719](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1719) | casella | la skeletal è figlia del ciclo |
| [#1758](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1758) | casella | CP E21.3 — La griglia si vede in partita |
| [#1763](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1763) | casella | il grafo di animazione in C++ |
| [#1784](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1784) | casella | *(parcheggiata)* le LOD dei pack |
| [#1936](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1936) | casella | il log a schermo racconta le celle |

➕ **È il gruppo con il ritorno più alto**: sono tutte cose che si guardano nello stesso viewport, e almeno
due hanno il codice **già spedito**:

- **#1763** — `GetCustomRootNode` e `GetCustomNodes` sono override reali (`RTUnitAnimInstance.h:122,125`).
  Restano due caselle, ed entrambe chiedono di guardare le catene di Riktor in PIE dopo un cambio di LOD.
- **#1719** — `2/4`: gli `.uasset` sono committati, resta `PIE-AS4a` e un giudizio sulla proporzione.

### `v0.1 · Percezione e reazioni` — sette

[#159](https://github.com/DegrassiAaron/refactor-tactics-main/issues/159) *(casella)* ·
[#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) *(prosa)* ·
[#291](https://github.com/DegrassiAaron/refactor-tactics-main/issues/291) *(casella, 5/7)* ·
[#319](https://github.com/DegrassiAaron/refactor-tactics-main/issues/319) *(casella, 1/38)* ·
[#1466](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1466) *(prosa)* ·
[#1738](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1738) *(casella)* ·
[#1933](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1933) *(prosa)*

### `v0.1 · Gate di release` — sette

[#16](https://github.com/DegrassiAaron/refactor-tactics-main/issues/16) *(epic)* ·
[#84](https://github.com/DegrassiAaron/refactor-tactics-main/issues/84) ·
[#85](https://github.com/DegrassiAaron/refactor-tactics-main/issues/85) ·
[#472](https://github.com/DegrassiAaron/refactor-tactics-main/issues/472) ·
[#938](https://github.com/DegrassiAaron/refactor-tactics-main/issues/938) ·
[#940](https://github.com/DegrassiAaron/refactor-tactics-main/issues/940) ·
[#959](https://github.com/DegrassiAaron/refactor-tactics-main/issues/959)

⚠️ **Questo gruppo chiede il pacchetto, non l'editor**: #85, #938, #940 e #959 nominano `packaged`. È una
seduta diversa dalle altre e più costosa — vale la pena raggrupparle fra loro proprio per questo.

### `v0.1 · Prova integrata` — una

[#170](https://github.com/DegrassiAaron/refactor-tactics-main/issues/170) — CP 15.4, golden replay degli 8 turni.

### Senza milestone — quindici

[#712](https://github.com/DegrassiAaron/refactor-tactics-main/issues/712) ·
[#1013](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1013) ·
[#1625](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1625) ·
[#1665](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1665) ·
[#1683](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1683) *(«solo l'editor»)* ·
[#1859](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1859) ·
[#1864](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1864) ·
[#1894](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1894) ·
[#1895](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1895) ·
[#1896](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1896) ·
[#1902](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1902) ·
[#1920](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1920) ·
[#1941](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1941) ·
[#1993](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1993) ·
[#2009](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2009)

⚠️ **Quindici su quarantatré non hanno milestone**: non compaiono in nessuna vista di release, e il
lavoro che portano è invisibile alla pianificazione. È lo stesso difetto di tracking che il referto
[`movement-microsteps-facing-pivot-spec-panel-2026-08-31.md`](movement-microsteps-facing-pivot-spec-panel-2026-08-31.md)
§6.1 ha corretto su quattro issue del movimento.

---

## 6. Le tredici chiudibili misurando

Sono le uniche su cui una sessione senza editor può concludere qualcosa.

[#314](https://github.com/DegrassiAaron/refactor-tactics-main/issues/314) ·
[#726](https://github.com/DegrassiAaron/refactor-tactics-main/issues/726) ·
[#995](https://github.com/DegrassiAaron/refactor-tactics-main/issues/995) ·
[#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165) ·
[#1317](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1317) ·
[#1403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1403) ·
[#1412](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1412) ·
[#1479](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1479) ·
[#1515](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1515) ·
[#1781](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1781) ·
[#1805](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1805) ·
[#1818](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1818) ·
[#2098](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2098)

---

## 7. Le tredici misurate voce per voce

| Issue | Verdetto | Ragione |
|---|---|---|
| [#1712](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1712) | ✅ **chiusa** | 8 criteri su 8; il quarto **superato** — chiedeva due cause di blocco LOS, l'implementazione ne nomina quattro |
| [#1060](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1060) | ✅ **chiusa** | i due tipi di assertion esistono e sono usati; lo showcase è passato da **5 a 8 turni**; `RTScenarioSession.cpp:287` cita la issue per nome |
| [#1515](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1515) | ⏳ **4 criteri su 5** | vedi §7.1 — manca solo una verifica di mutazione, che nessun commit registra |
| [#1317](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1317) | ⏳ **materialmente finita** | `6/10`, ma le 4 non spuntate sono `⛔ via non scelta`: due strade alternative, e l'autore ne ha presa una |
| [#1412](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1412) | 🔒 **bloccata su una dipendenza** | vedi §7.2 — i due siti citati sono chiusi, sette restano e **non sono correggibili** |
| [#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705) | 🔒 **bloccata** | 4 test aspettano produttori a **zero** occorrenze — `AddHitBox`, `AllyIntentGhost`, E14, `bMapElement` |
| [#1805](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1805) | 🔒 **correttamente aperta** | `14/15`: l'ultima casella è privacy **in replica live**, e `DOREPLIFETIME` dà **0 file** — non c'è filo su cui misurare |
| [#1403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1403) | 🔒 **correttamente aperta** | le sue caselle sono **domande** — *«è per scelta o è un difetto?»* — non lavoro |
| [#1781](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1781) | 🔒 **correttamente aperta** | `4/4` spuntate, ma sono il **gate di design**; l'implementazione è `🧊 backlog`, post-v0.1, e dipende da #1773 aperta |
| [#726](https://github.com/DegrassiAaron/refactor-tactics-main/issues/726) | 🟢 **pronta da lavorare, e nessuno l'ha presa** | vedi §7.3 — **zero** runtime, sbloccata dal 2026-08-16, DoD corretta e coerente |
| [#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165) | 🔴 **DoD scaduta** | vedi §7.4 — nomina quattro gate che una decisione ha **rimosso dal repository** |
| [#1683](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1683) | 👁️ **serve l'editor** | il criterio 1 dice *«è ciò che **solo l'editor** può dire»* — non è nelle tredici |
| [#2098](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2098) | 🔵 **lavorata oggi** | `docs(2098)` è del **2026-09-02**; il criterio 1 è una `D-nnn` che non esiste ancora |

### 7.1 #1515 — quattro criteri su cinque, e il quinto costa un `build`

| Criterio | Esito |
|---|---|
| `Validate` rifiuta la squadra fuori intervallo nominando unità e valore | ✅ `RTScenarioLoader.cpp:2014,2019` |
| l'intervallo è **derivato** dal runtime, non una costante `0..1` | ✅ `URTTurnRules::NumTeams`, con un commento a `:2008` che lo dichiara |
| gli scenari versionati continuano a caricare | ✅ `Scenario.EveryShippedScenarioRuns` — e ora sono **111**, non 86 |
| un id duplicato non produce un editor che obbedisce a metà | ✅ `Scenario.DuplicateUnitIdIsRejectedAtBothDoors` |
| **verifica di mutazione** | ⛔ **nessuna traccia** in `docs/` né nei messaggi di commit |

🔑 Il test `TeamConstraintComesFromTheRuntime` non pinna *«il numero è 2»* ma *«è lo stesso numero che
decide chi vince»* — e scarta esplicitamente il candidato ovvio, `FRTKnowledgeVerdict::MaxTeamId`, che
vale **31** perché è la capacità di una bitmask di percezione. Derivare da lì avrebbe ammesso `"team": 7`,
cioè il difetto che la issue descrive.

### 7.2 #1412 — i due siti citati sono chiusi, e i sette rimasti non si possono chiudere

I due punti che il corpo nomina (`RTTurnManager_Blast.cpp:299` e `:461`) **non chiamano più**
`AddLogEvent`: al loro posto c'è un commento che spiega da dove arriva la riga.

Ma il perimetro reale erano **sette** punti, e `RTCombatLogTests.cpp:552` dichiara perché restano:

> *«non si tolgono finché `DescribeTurnLog` non sa nominare l'attore. Esercitarli QUI renderebbe il test
> rosso su un difetto noto e non ancora correggibile, che è un modo per farlo ignorare.»*

∴ **È una dipendenza, non una negligenza.** Il test che c'è protegge dall'**ottavo** duplicato — quello
che #1932 ha creato dando il soggetto alle voci `Move` — non dai sette.

---

### 7.3 🟢 #726 — sbloccata da due settimane, zero righe scritte

È l'unica delle tredici su cui **si può cominciare a scrivere codice adesso**, ed è ferma.

| Misura | Esito |
|---|---|
| `ERTRelativeDirection` in `Source/` | ⛔ **0 file** |
| `ERTRelativeSide`, `RelativeDirectionFrom` | ⛔ **0 file** |
| scenario `Spec.Facing.SixRelativeSides` | ⛔ **non esiste** |
| la decisione che la bloccava, `FAC-15` | ✅ **chiusa il 2026-08-16** da `D-147` |
| la riga contraddittoria del suo DoD | ✅ **corretta** — non nomina più `FrontLeft`/`FrontRight`, asserisce sull'indice di spicchio |

🔑 **Il `fix(726)` che compare in `main` non è un'implementazione: è la scoperta che la issue era
insensata.** `af9d5147` apre `FAC-15` dimostrando che il DoD chiedeva **due proprietà incompatibili** —
equipartizione dei sei lati e simmetria destra/sinistra non possono coesistere su un anello esagonale di
raggio pari. `D-147` ha risolto scegliendo: *«la seconda non era un requisito»*.

∴ Una **quarta** trappola, oltre alle tre di §3: un commit `fix(N)` può essere il referto che dichiara la
issue non costruibile. Qui però la storia è finita bene — la decisione è arrivata **lo stesso giorno**, e
da allora #726 è costruibile e nessuno se n'è accorto.

⚠️ **Una sotto-decisione resta, e `D-147` la assegna esplicitamente a questa issue**: la mappatura
nome↔indice. Gli indici girano `E, NE, NW, W, SW, SE` e `AxialToWorld` manda `NE` a `-Y`, quindi con la
convenzione UE l'indice `f+1` cade a **sinistra** mentre `D-126` chiama `FrontRight` proprio quella
posizione. *«La mappatura si fissa quando la relazione entra in codice, ed è lavoro di #726.»*

➕ `D-147` registra anche una **regola migliore che non adotta** — lo spicchio *centrato*, con `24` celle
asimmetriche su 216 invece di `168` — perché comprerebbe una proprietà dichiarata non richiesta. I numeri
sono già misurati: chi implementa non deve rifarli.

### 7.4 🔴 #1165 — il suo DoD nomina quattro gate che `D-182` ha rimosso

| Il DoD chiede | Stato reale |
|---|---|
| `python scripts/check-docs-links.py --wiki-root <clone>` esce `0` | ⛔ assente — e **Python è vietato** |
| `python scripts/docs_inventory.py` esiste e gira | ⛔ assente, stessa ragione |
| `node --test docs/control-center/graph.test.mjs` | ⛔ la cartella **non esiste** |
| `docs/src/` non esiste più come categoria | ✅ **già rimossa** |

La causa è una decisione, non una svista: `d671df47` — **`chore(D-182): la cartella scripts/ esce dal
repository, e con lei ogni gate automatico»`**. Restano validi *«il divieto di Python, l'assenza di
`scripts/`, l'assenza di CI»*.

➕ **Il successore esiste ed è verde**: `D-188` ha reintrodotto il controllo in Node, e
`node tools/radar/doc-links.ts --check` esce `0` su **5071 link in 337 documenti** — misurato in questo
pass, sul referto che stai leggendo.

∴ **#1165 non aspetta lavoro, aspetta una riscrittura del DoD** contro gli oracoli che esistono oggi. È la
stessa famiglia del difetto di #220 (`IconCatalog.NoDirectTextureInWidgets`, mai esistito) e delle undici
verifiche di `ADR-0008` di cui zero esistono: **un criterio che nomina un oracolo assente non è severo, è
non verificabile — e viene saltato.**

### 7.5 Le altre quattro, in breve

- **#1479** — bloccata su #1403 (che la `Cleanse` sia raggiungibile), a sua volta correttamente aperta. Il
  criterio 1 è una decisione di bilanciamento, non lavoro.
- **#995** — i criteri sono documenti da scrivere: lavoro reale, non ancora fatto.
- **#314** — `1/20`. `feat(314)` ha portato la decisione del `Brace` nel TurnLog con il **formato v10**:
  lavoro sostanziale consegnato, perimetro ancora largo.
- **#1818** — `0/0` su un God Object di **10 412 righe**. Un `refactor(N)` non lo chiude per costruzione;
  serve un perimetro dichiarato prima, non una misura dopo.

---

## 8. 🔴 Le caselle di DoD non sono un segnale, e falliscono in tre modi

| Modo | Esempio | Cosa vede chi legge |
|---|---|---|
| **finita con `0/N`** | #1712, #1060 | due issue intatte da lavorare — è quello che stava per succedere |
| **`N/N` ma aperta apposta** | #1781 `4/4` | una issue finita da chiudere — e invece è un **gate di design** su un lavoro in backlog |
| **caselle per una strada non presa** | #1317 `6/10` | quattro criteri mancanti — e invece sono due alternative, di cui una scartata |

∴ **Nessuna delle tre direzioni si può leggere dal rapporto `x/N`.** In questo repository la frazione non
distingue *«non fatto»* da *«non applicabile»* da *«fatto e non spuntato»*.

➕ **Ciò che è a portata di mano**: spuntare le proprie caselle è l'unico canale che dice a un'altra
sessione che il lavoro è finito — il commit `feat(N)` non basta, perché non distingue *«ho fatto un
pezzo»* da *«ho chiuso»*. Ed è il difetto misurato in §2.

---

## 9. Verifiche eseguite

| Verifica | Esito |
|---|---|
| Elenco issue aperte, `gh --limit 400` | ✅ **335** — il primo pass a `--limit 300` era truncato |
| Commit che le citano, `git log origin/main -3000` | ✅ **56** con almeno un commit |
| Gate visivo, caselle **e** prosa, più una lettura a mano del complemento | ✅ **43 / 56** |
| DoD misurata contro il codice | ✅ **13 issue**, voce per voce |
| `node tools/radar/doc-links.ts --check` | ✅ **exit 0** — 5071 link in 337 documenti risolvono |
| `node tools/radar/issue-refs.ts --check` | ✅ **exit 0** — nessuna issue cita un percorso rimosso |
| `node tools/radar/doc-tables.ts --check` | ✅ **exit 0** — 2135 tabelle allineate |
| `RefactorTactics.Scenario` (per #1060) | ✅ **tutti verdi**, incluso `ShowcaseRelayV01RunsTurnOne` |
| `RefactorTactics.Debug` (per #1712) | ✅ **10 / 0** |
| **Build** · **Suite completa** · **PIE** · **Packaged** | ⛔ **NOT RUN** — nessuna riga di `Source/` toccata da questo pass |

---

## 10. La prossima azione

> 🟢 **Se serve codice da scrivere subito: [#726](https://github.com/DegrassiAaron/refactor-tactics-main/issues/726).**

È l'unica delle cinquantasei che è **sbloccata, specificata, non PIE-gated e a zero righe**. Lo è dal
2026-08-16, e in due settimane nessuno l'ha presa — probabilmente perché il `fix(726)` che compare in
`main` la fa sembrare avviata, mentre è il referto che l'ha sbloccata.

> 👁️ **Se serve chiudere in blocco: una seduta editor sulle tredici di `v0.1 · Leggibilità`.**

Sono nello stesso viewport, e almeno due — #1763 e #1719 — hanno il codice già spedito e aspettano solo di
essere guardate. Chiuderle in una seduta costa meno che aprirne una per ciascuna.

⏭️ **Poi, e separata**: la seduta `packaged` per le quattro di `Gate di release` (#85, #938, #940, #959).
Non si mescola con la prima — vuole un pacchetto, non l'editor.

⏭️ **Due riscritture di DoD, indipendenti da tutto**: #1165 nomina quattro gate che `D-182` ha rimosso, e
#220 nomina un oracolo mai esistito. Nessuna delle due è lavoro di prodotto; entrambe rendono un criterio
verificabile invece che saltabile.

⏭️ **E una riga di tracking**: assegnare una milestone alle **quindici** senza. Senza quella riga il loro
contenuto non esiste per nessuna pianificazione.

⚠️ **Ciò che questo referto NON ha fatto**: misurare la DoD delle restanti 43. Ogni DoD nomina simboli e
comportamenti diversi, quindi non si automatizza — e per 43 di esse finirebbe comunque in *«serve una
seduta»*.
