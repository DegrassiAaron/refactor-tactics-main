# Chiusura della v0.1 — quattro decisioni d'autore, e la rotta di milestone che ne deriva

> `CURRENT` · **Stato**: decisioni **prese in sessione, non ancora registrate** · **Data**: 2026-09-06
> **HEAD della ricognizione**: `main` = `d062ccf0`, albero pulito e allineato a `origin/main`.
> **HEAD della misura dei numeri `D-nnn`**: `origin/main` = `9da33c59`. ⚠️ Durante la sessione `origin/main`
> si è mosso (`d062ccf0` → `9da33c59`) e le PR aperte sono passate da **0** a **2**: i numeri sono stati
> rimisurati sul secondo.
> **Oggetto**: convertire il lavoro residuo della v0.1 in una sequenza di milestone verticali dipendenti, e
> chiudere le decisioni di contratto che quella sequenza richiedeva per esistere.
> **Innesco**: sessione di spec panel richiesta dall'autore, seguita da una sessione di decisione sulle voci
> che ne erano emerse come bloccanti.
> 🔑 **Nessun numero qui è ricordato.** Ogni conteggio porta il selettore che l'ha prodotto, e §2 li elenca.
> ⛔ **Questo referto non è un owner.** Le decisioni diventano vere quando le quattro voci di §5 entrano nel
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md) e le celle di §6 sono riscritte dai loro owner.

---

## 1. Il verdetto in quattro righe

| # | Decisione | Uscita scelta |
|---|---|---|
| `DEC-1` | La riserva della «via a punti» di `G13` **si riscrive**: la via esce dal perimetro verificabile della v0.1 | *(a)* delle tre |
| `DEC-2` | `PIE-V01-PACKAGED` **tiene** l'accoppiamento video + log con la clausola del reason code nello stesso turno | *(a)* delle due |
| `DEC-3` | [#2534](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2534) si chiude per la via **(b)**: il log **nomina** ciò che ferma il tiro | *(b)* delle quattro |
| `DEC-4` | Il rename degli ID d'eroe **si completa dentro la v0.1** | *(b)* delle tre |

Da registrare come [`D-338`](../../decisions/RT_PDR_00_Decision_Log.md)…[`D-341`](../../decisions/RT_PDR_00_Decision_Log.md) — testo pronto in §5.

---

## 2. La baseline, misurata

Il gate di release della v0.1 è `G1`–`G14` in [`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) §3
(`G15` è ritirato da `D-181` e il numero non si riusa). **Cinque su quattordici sono verdi.**

| Misura | Valore | Selettore |
|---|---:|---|
| Test Automation dichiarati | **2008** in 207 file | `grep -rhoE '"RefactorTactics\.[A-Za-z0-9_.]+"' Source/RefactorTactics/Tests/*.cpp \| sort -u \| wc -l` |
| Ultima run *VALIDA* registrata nel DoD | **1368/1368**, 2026-08-29, `bbf0d780` | cella `G2` di §3 — **640 test più giovani della misura** |
| Voci nel registro PIE | **228** — 80 ✅ · 26 🟡 · 118 ⏳ · **4 ❌** | glifo nell'**ultima** colonna di ogni riga `\| **PIE-*`, non il primo della riga |
| Subset `RELEASE-V01` (gate `G9`) | **17** — 15 ✅ · 1 🟡 · **1 ❌** | ``grep -cE '^\| \*\*PIE-[A-Za-z0-9.-]*\*\* `RELEASE-V01`'`` |
| Sedute in editor | **48**, di cui **19** critiche; 4 senza verdetti aperti | [`editor-sessions.yaml`](../editor-sessions.yaml), campo `critical` incrociato con `verifies` |
| Issue aperte con label `v0.1` | **102** | `gh issue list --label v0.1 --state open` |
| … di cui **senza milestone** | **19**, fra cui **2 P0** e **4 epic** | `select(.milestone == null)` sullo stesso elenco |
| Epic v0.1 aperte · P0 aperte | **19** · **11** | `gh issue list --label v0.1 --label epic` |
| Massimo `D-nnn` assegnato | **D-337** | ``grep -oE '^\| \*\*D-[0-9]{3}\*\*'`` su `origin/main` |

⚠️ **Il perimetro di release è la label, non la milestone.** Contare per milestone GitHub perde **19 issue su
102** — e sono le più giovani, quindi proprio quelle che bloccano: fra le orfane ci sono
[#2534](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2534) e
[#2476](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2476), entrambe `P0`.

### 2.1 Tre derive che rendono falsa la lettura ingenua dello stato

**Una issue chiusa non è una prova.** [#959](https://github.com/DegrassiAaron/refactor-tactics-main/issues/959)
— *«CP 47.6 · La partita registrata, in PIE e su packaged»* — è **chiusa** dal 2026-09-04 con sei DoD su sei
spuntate, mentre la voce che ne possiede il verdetto, `PIE-V01-PACKAGED`, è ancora **⏳** e la sua cella dice
*«non aspetta nessuna decisione: aspetta un'esecuzione»*. In più
[`execution-graph.yaml`](../execution-graph.yaml) dichiara `#959 requires #79`, e `#79` è **aperta**: una
dipendenza dura violata da una chiusura.

**Le checklist delle epic sono stantie.** [#286](https://github.com/DegrassiAaron/refactor-tactics-main/issues/286)
elenca `#287` non spuntata — chiusa il 2026-08-25 — e `#1719` non spuntata, chiusa;
[#25](https://github.com/DegrassiAaron/refactor-tactics-main/issues/25) elenca `#78` non spuntata, chiusa, e
dichiara di `#80` che *«nessuno degli otto `rt.Debug.*` nominati esiste»* — nel codice ce ne sono
**quattordici** (`grep -rhoE '"rt\.[A-Za-z0-9_.]+"' Source/ --include=*.cpp | sort -u`).

**`G1` è rosso, e non lo dice nessuna delle due viste di stato.** Vedi §3.

---

## 3. Il reperto che precede tutto: la Shipping non compila

`bKnowledgeDebug` è dichiarato in `Source/RefactorTactics/Map/RTHexMapActor.h:685` **dentro**
`#if !UE_BUILD_SHIPPING`, ed è letto e scritto a profondità di guardia **zero** in
`RTHexMapActor.cpp:1426` e `:1430`.

Misurato **staticamente**, contando le direttive del preprocessore riga per riga — non con una build:

```bash
awk 'NR<=1440{if(/^#if/){d++} if(/^#endif/){d--} if(NR==560||NR==682||NR==1426){print NR": depth="d}}' \
  Source/RefactorTactics/Map/RTHexMapActor.cpp
# 560: depth=1   1426: depth=0
```

Corrisponde a [#2395](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2395), **aperta e senza
milestone**. Conseguenze che non sono opinioni:

- la cella `G1` del DoD dichiara ✅ dal **2026-08-29**, e non lo è;
- `G12` è verde dal 2026-08-16 e descrive un pacchetto che **oggi non si ricostruirebbe**, perché la Shipping
  è una delle due configurazioni che quel gate compila;
- ogni misura presa su questo albero misura un binario che non è quello spedito.

⚠️ **È la terza volta che questa classe di difetto passa inosservata**, e il DoD lo registra: il 2026-08-24
erano test scritti dopo l'`#endif`, il 2026-08-29 era `GetBoolMetaData` sotto `#if WITH_METADATA`. L'oracolo
esistente — `RefactorTactics.Meta.TestGuardClosesAtEndOfFile` — controlla **dove chiude la guardia**, non
**quale simbolo si usa dentro**, e questa forma gli sfugge per costruzione.

---

## 4. Le quattro decisioni

### `DEC-1` — la riserva della «via a punti» di `G13`

**La domanda.** `G13` è 🟡 e la riserva residua dice: *«la via a punti non è esercitata in autobattle (il bot
non cerca l'obiettivo, è E26), e `PIE-V01-BOARD` non è stata eseguita»*. La seconda metà è esecuzione. La
prima è una decisione.

**Il dato che la sposta, e non era stato guardato.** La via a punti è **configurata spenta**:

```cpp
/** Punteggio obiettivo che chiude la partita. 0 = nessuna vittoria per obiettivo in questo formato. */
int32 ScoreToWin = 0;                       // Source/RefactorTactics/Turn/RTMatchFormatData.h:130
```

Lo zero non è un difetto: il codice lo documenta come **scelta legittima di formato**, e
`RTMatchFormatLibrary.cpp:24` lo ripete rifiutando i soli valori negativi. ∴ `G13` chiedeva di dimostrare una
via che il formato spedito disabilita e che nessun agente della v0.1 percorre — il bot che la percorrerebbe è
`E26` ([#326](https://github.com/DegrassiAaron/refactor-tactics-main/issues/326)), **v0.2**.

**Le tre uscite.** *(a)* riscrivere la riserva: la via esce dal perimetro. *(b)* accendere `ScoreToWin > 0` e
farla esercitare da una partita umana dal pacchetto — `G13` resta letterale (*«partita giocabile»*, non
*«non presidiata»*) ma la cattura è **unilaterale**, perché nessuno la contende. *(c)* tirare dentro `E26`.

**Scelta: *(a)*.** Costo accettato: la v0.1 spedisce una condizione di vittoria che **esiste nel codice** —
`ERTMatchOutcome::Objective`, `Turn/RTTurnRules.h:44` — e che nessuno percorre. Va **dichiarata** fra i limiti
noti, non taciuta.

---

### `DEC-2` — l'accoppiamento video + log di `PIE-V01-PACKAGED`

**La domanda.** Due owner chiedono due cose diverse. La colonna «come si verifica» di `G10` dice *«playtest
registrato (log **o** video)»*; la voce `PIE-V01-PACKAGED` del registro dice *«video o sequenza di screenshot
**più** il log della sessione»*, e il DoD di `#959` aggiunge la clausola che l'evidenza mostri un evento
`Combat` a schermo **e** la sua riga di reason code **nello stesso turno**.

**Perché la domanda conta.** È l'unica leva che accorcia il percorso critico: allineare la voce a `G10` toglie
dalla catena la milestone dell'explainability, perché `#79` smetterebbe di essere un prerequisito.

**Scelta: la voce tiene l'accoppiamento.** La ragione scritta nella voce regge — *«il log da solo non mostra
la leggibilità della board; il video da solo non porta i reason code»* — ed è la sola prova che un giocatore
capisca **perché** il turno è finito così.

Conseguenza operativa: l'arco `#79 → #959` di [`execution-graph.yaml`](../execution-graph.yaml) resta valido,
ed è oggi **violato** (§2.1). Chiudere `#79` è la riparazione, non la riscrittura dell'arco.

---

### `DEC-3` — [#2534](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2534), la lastra del blocco-vista

**Lo stato.** `PIE-HEXPLAY-6` è **❌** dal 2026-09-06, giudicata in seduta `U46` sul banco che **isola** la
lastra dalla colonna. È una delle 17 voci `RELEASE-V01`: `G9` ha una voce fallita.

**Le due vie, che l'issue dichiara indipendenti** — *«una basta»*. **(a)** rendere leggibile la lastra:
alzarla (ma si avvicina alla colonna da 55 cm e si perde la distinzione fra *«non si vede»* e *«non si passa»*
— il difetto che `#552` aveva risolto dando **forme** diverse), oppure distinguerla per materiale/colore
(strada di `#956`, ma serve una decisione registrata perché tocca la grammatica visiva). **(b)** far nominare
la causa al log: *«nessuna linea di tiro (muro in `(q,r,L)`)»*.

**Scelta: *(b)*.** Toglie la dipendenza dalla vista invece di migliorarla, non tocca la grammatica di `#956` e
`#552`, e non richiede un secondo `D-nnn` sull'altezza.

⚠️ **Il vincolo fa parte della decisione, non è una raccomandazione a valle**: il log **non può nominare stato
che il giocatore non conosce**. Una cella che l'unità ha osservato è lecita; ciò che sta oltre il velo no. Un
test lo copre, ed è quello che rende la via *(b)* accettabile.

🔁 **Effetto sul grafo, non previsto quando la domanda è stata posta**: il rosso di `G9` **cambia
proprietario**. Si toglie nella milestone dell'explainability, non in quella della presentazione dei corpi.

---

### `DEC-4` — il rename degli ID d'eroe entra nella v0.1

**Lo stato.** [`D-334`](../../decisions/RT_PDR_00_Decision_Log.md) fissa le quattro identità retail
(`Aevik · Muiren · Branth · Ivrin`) e [`D-337`](../../decisions/RT_PDR_00_Decision_Log.md) dichiara il rename
**secco**. L'esecuzione è [#2491](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2491),
etichettata `post-v0.1` `P3`. Nel codice il vocabolario è **a un quarto**: solo `Riktor → Branth` è fatto.

**Scelta: si completa dentro la v0.1.** Non si ferma dichiarando il vocabolario misto come limite noto, non si
reverte `Branth`.

**Il costo, misurato invece che stimato.** La fetta già eseguita è la misura più onesta disponibile:

```bash
for c in $(git log --format=%H --grep='2491' | head -20); do git show --stat --format='' $c; done | tail -1
# 200 files changed, 1011 insertions(+), 993 deletions(-)
```

…ed era la fetta **più piccola**: `Hero.Riktor` aveva **1** file `Source/` e **0** scenari. Le tre residue,
contate col selettore che distingue il namespace:

| Identità | `Source/` | `Scenarios/` | `docs/` |
|---|---:|---:|---:|
| `Hero.Gadget` | 60 | 97 | 78 |
| `Hero.Wraith` | 57 | 74 | 75 |
| `Hero.Phase` | 33 | 44 | 71 |

```bash
for id in Gadget Wraith Phase; do
  printf '%s  Source:%s Scenari:%s docs:%s\n' "$id" \
    "$(grep -rl "Hero\.$id" Source/ | wc -l)" \
    "$(grep -rl "Hero\.$id" Scenarios/ | wc -l)" \
    "$(grep -rl "Hero\.$id" docs/ | wc -l)"
done
```

**Due conseguenze da accettare esplicitamente**, perché nessuna è reversibile a costo zero. **(1)** Il rename è
**secco** (`D-337`): nessun redirect, nessun resolver, nessuna finestra di compatibilità di vocabolario — ogni
riferimento mancato è **rotto**, non degradato. **(2)** Le tracce di replay prodotte col vocabolario precedente
**non si rileggono** come lo stesso replay logico, e [`D-310`](../../decisions/RT_PDR_00_Decision_Log.md) le
rifiuta fail-closed. Non è un difetto della migrazione: è ciò che «secco» significa, ed è già dichiarato in
`D-337`.

✅ **Ciò che rende la decisione eseguibile è un fatto della fetta precedente, non una speranza.** Il commit
`ced0d790` — *«il pack non si deriva dall'HeroId»* — ha staccato il pacchetto d'asset dall'identità.
`Content/RT/Characters/Riktor/Blueprints/BP_Unit_Riktor.uasset` esiste ancora e contiene `Hero.Branth`. **Si
rinomina l'identità, non il pacchetto**: i binari Unreal restano fuori dal write-set, e le icone d'azione —
che portano l'id nel nome file — seguono l'identità. È la sola ragione per cui questa migrazione sta in una
milestone invece che in una release.

**Sequenziamento.** Va in **testa**, subito dopo il ramo verde, e da sola. Ogni milestone eseguita prima le
aggiunge superficie — nuovi record di traccia, gli stessi `BP_Unit_*`, l'evidenza registrata — e un'evidenza
che mostra il vocabolario vecchio **va rifatta**, cioè costa una seduta d'editor. Alla ricognizione le PR
aperte erano **zero**: è la finestra, e non si ripresenta da sola.

---

## 5. Le quattro voci, pronte da registrare

**Numerazione, verificata nei tre posti che la nota su `D-304` prescrive**, su `origin/main` = `9da33c59`:

1. **registro** — massimo assegnato **`D-337`**, contando le **righe di assegnazione** `^| **D-nnn**` e non le
   occorrenze del token;
2. **PR aperte** — **2** (`#2581`, `#2580`), **nessuna** assegna un `D-3nn`;
3. **branch remoti** — **5**, massimo `D-337` su ciascuno.

⚠️ **La citazione `D-337…D-340` nella nota di `D-336` non è una rivendicazione**: è la registrazione che quei
numeri erano *liberi* alla stessa misura. Solo `D-337` è stato poi speso. La distinzione è esattamente quella
che la nota su `D-322` impone — *«una citazione in prosa non è una rivendicazione»* — e senza guardarla si
sarebbe saltato a `D-341`.

🔴 **Va riverificato di nuovo prima del push**, e non è prudenza: `D-336` registra una collisione avvenuta in
una finestra di **ottanta secondi** fra il controllo a tre posti e il merge, con un branch che al momento del
controllo **non esisteva ancora**. La riverifica è l'unico momento in cui quella finestra si chiude.

Le quattro righe seguono, nel formato della tabella `| ID | Decisione | Stato | Impatto |`. Un `|` letterale
dentro una cella va scritto `\|`.

---

### `D-338`

```markdown
| **D-338** | **LA «VIA A PUNTI» ESCE DAL PERIMETRO VERIFICABILE DELLA v0.1: la riserva di `G13` si riscrive, e il gate smette di chiedere che sia esercitata.** Chiude la metà decisionale della riserva del 2026-08-10; la metà residua — l'esecuzione di `PIE-V01-BOARD` — resta lavoro, non domanda. **(1) Il dato che decide.** `URTMatchFormatData::ScoreToWin` vale **0** nella configurazione spedita, e il codice documenta lo zero come scelta legittima di formato: *«0 = nessuna vittoria per obiettivo in questo formato»* (`Turn/RTMatchFormatData.h:130`); `RTMatchFormatLibrary.cpp:24` rifiuta i soli valori **negativi**. ∴ il gate chiedeva di dimostrare una via che il formato spedito **disabilita**. **(2) Chi la percorrerebbe non è della v0.1.** Il bot non cerca l'obiettivo: è `E26` *Tactical Bot v1* ([#326](https://github.com/DegrassiAaron/refactor-tactics-main/issues/326)), **v0.2**. Esercitare la via avrebbe richiesto una delle due cose che questa voce rifiuta — accendere `ScoreToWin` e farla catturare da una partita umana **non contesa**, oppure anticipare `E26`. **(3) Cosa NON si decide.** ⛔ La via **resta nel codice**: `ERTMatchOutcome::Objective` (`Turn/RTTurnRules.h:44`) e `ResolveObjectiveControl` (`:222`) non si rimuovono, e i loro test restano. ⛔ Non si decide se la v0.2 la accenda. ⛔ Non si tocca `RoundLimit`, né [D-184](RT_PDR_00_Decision_Log.md), che dichiara legittimo il pareggio allo scadere. **(4) Il costo, accettato.** La v0.1 spedisce una condizione di vittoria che esiste e che nessuno percorre. ⚠️ **Va dichiarata fra i limiti aperti che il DoD di [#85](https://github.com/DegrassiAaron/refactor-tactics-main/issues/85) pretende** — *«i limiti aperti della v0.1 sono dichiarati»* — e non taciuta: un gate riscritto senza la ragione scritta accanto è indistinguibile da un gate abbassato | **Accettata** — **decisione d'autore** *(2026-09-06)*, presa in sessione di spec panel sulla rotta di chiusura della v0.1. Referto: [`../roadmap/plans/chiusura-v01-decisioni-e-milestone-spec-panel-2026-09-06.md`](../roadmap/plans/chiusura-v01-decisioni-e-milestone-spec-panel-2026-09-06.md) §4. ⚠️ **Il numero è il primo libero, verificato nei tre posti che la nota su `D-304` prescrive**, su `origin/main` = `9da33c59`: registro a `D-337`; **2** PR aperte, nessuna assegna un `D-3nn`; **5** branch remoti, massimo `D-337`. ⛔ **La citazione `D-337…D-340` nella nota di `D-336` NON è una rivendicazione**: registra che quei numeri erano liberi a quella misura, ed è la distinzione che la nota su `D-322` impone | `G13`, [#85](https://github.com/DegrassiAaron/refactor-tactics-main/issues/85), `PIE-V01-PACKAGED`. ⛔ **Nessuna riga di `Source/` toccata**: cambia il perimetro del gate, non il codice. ➡️ **Richiede**: la cella `G13` di [`v0.1-definition-of-done.md`](../roadmap/v0.1-definition-of-done.md) §3 riscritta con la ragione, e la voce fra i limiti noti di `#85` |
```

### `D-339`

```markdown
| **D-339** | **`PIE-V01-PACKAGED` TIENE L'ACCOPPIAMENTO VIDEO + LOG, con la clausola dell'evento a schermo e del reason code nello stesso turno: la voce NON si allinea a `G10`, che accetta «log o video».** Chiude una divergenza fra due owner che nessuno aveva posto come domanda. **(1) La divergenza, misurata.** La colonna «come si verifica» di `G10` dice *«playtest registrato (log **o** video)»*; la voce `PIE-V01-PACKAGED` di [`../technical/test-manuali-pie.md`](../technical/test-manuali-pie.md) dice *«video o sequenza di screenshot **più** il log»*, e il DoD di [#959](https://github.com/DegrassiAaron/refactor-tactics-main/issues/959) aggiunge che l'evidenza deve mostrare almeno un evento `Combat` a schermo **e** la riga di log col reason code corrispondente **allo stesso turno**. **(2) Perché la domanda contava.** Era **l'unica leva** che accorciava il percorso critico della release: allineare la voce a `G10` avrebbe tolto dalla catena la milestone dell'explainability, perché [#79](https://github.com/DegrassiAaron/refactor-tactics-main/issues/79) avrebbe smesso di essere un prerequisito. **(3) Perché la clausola regge.** La ragione è già scritta nella voce e non è stata trovata difettosa: *«il log da solo non mostra la leggibilità della board; il video da solo non porta i reason code»*. È la sola evidenza della release che dimostri che un giocatore capisce **perché** il turno è finito così — cioè l'unica che distingue «il gioco funziona» da «il gioco si spiega». **(4) La conseguenza operativa, che è una riparazione e non una riscrittura.** [`../roadmap/execution-graph.yaml`](../roadmap/execution-graph.yaml) dichiara `#959 requires #79` con un rationale misurato il 2026-08-23; l'arco è **violato oggi**, perché `#959` è **chiusa** e `#79` **aperta**. Questa voce conferma l'arco: si ripara chiudendo `#79`, non riscrivendo la dipendenza. ⚠️ **E registra che una chiusura può scavalcare la propria dipendenza senza che nessun gate se ne accorga**: il DoD di `#959` risulta spuntato mentre `PIE-V01-PACKAGED`, che ne porta il verdetto, è ancora ⏳ | **Accettata** — **decisione d'autore** *(2026-09-06)*, stessa sessione di [D-338](RT_PDR_00_Decision_Log.md). ⚠️ **Numero verificato nella stessa misura a tre posti**: vedi `D-338`. ⚠️ **È la decisione che NON accorcia**: l'alternativa era disponibile e legittima, ed è stata scartata sul merito | `G10`, `G13`, `PIE-V01-PACKAGED`, [#79](https://github.com/DegrassiAaron/refactor-tactics-main/issues/79), [#959](https://github.com/DegrassiAaron/refactor-tactics-main/issues/959). ⛔ **Nessuna riga di `Source/` toccata.** ➡️ **Richiede**: che `#79` sia chiusa **prima** dell'esecuzione di `PIE-V01-PACKAGED`, e che la riapertura o la nota di chiusura di `#959` registri che il suo verdetto vive nella voce PIE |
```

### `D-340`

```markdown
| **D-340** | **[#2534](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2534) SI CHIUDE PER LA VIA *(b)*: il log NOMINA ciò che ferma il tiro — «nessuna linea di tiro (muro in `(q,r,L)`)» — invece di rendere leggibile la lastra.** Chiude la scelta che l'issue lascia aperta nel proprio corpo, dichiarando le due vie *«indipendenti — una basta»*. **(1) Cosa si sceglie, e cosa NON si fa.** ⛔ Non si alza `RTSightSlabHeight` oltre i **10 cm**: avvicinarla alla colonna da **55** perde la distinzione fra *«non si vede»* e *«non si passa»*, che è il difetto che `#552` aveva risolto dando **forme** diverse. ⛔ Non si distingue per materiale o colore: sarebbe la strada di `#956` — *«colore **e** forma, mai solo il colore»* — e richiederebbe una decisione registrata sulla grammatica visiva, cioè un secondo `D-nnn`. **(2) Il vincolo È la decisione, non una raccomandazione a valle.** Il log **non può nominare stato che il giocatore non conosce**: nominare una cella bloccante che l'unità **ha osservato** è lecito, nominare ciò che sta oltre il velo no. Senza il test che lo copre, la via *(b)* è un leak di conoscenza travestito da messaggio d'errore — ed è la ragione per cui il gate di privacy di questa voce non è opzionale. **(3) Perché la comprensibilità dipendeva interamente dalla vista.** Nella riga che il giocatore riceve — `(q=-1,r=0,L=0) -> (q=1,r=0,L=0): nessuna linea di tiro (Action.BasicAttack · Hero.Gadget.ArcPulse, p50)` — non compare il muro, e il **bersaglio è velato** perché è il muro stesso a bloccare la vista: chi guarda vede la propria unità sparare verso il nulla, senza vedere né il nemico né l'ostacolo. ∴ un giocatore conclude che l'attacco è rotto. **(4) L'effetto sul grafo, che è la parte non ovvia.** Il rosso di `G9` **cambia proprietario**: si toglie nel lavoro di explainability, non in quello di presentazione dei corpi. La milestone della presentazione perde un asse e smette di essere larga. **(5) Cosa NON si decide.** ⛔ Non riapre `#956` né `#552`. ⛔ **Non decide `MSE-3`**, il cui innesco resta [#621](https://github.com/DegrassiAaron/refactor-tactics-main/issues/621), la cottura: la lastra è un volume di **presentazione** e non tocca l'occupancy, quindi la scelta fra cerchio inscritto e dodici settori resta aperta e intatta. ⛔ Se un giorno la lastra dovrà essere leggibile, servirà un `D-nnn` proprio | **Accettata** — **decisione d'autore** *(2026-09-06)*, stessa sessione di [D-338](RT_PDR_00_Decision_Log.md). ⚠️ **Numero verificato nella stessa misura a tre posti**: vedi `D-338`. ⚠️ **Il verdetto negativo che la rende necessaria è del 2026-09-06**, seduta `U46` ([#2476](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2476)), sul banco `Visual.Map.SightWallIsWalkable` che **isola** la lastra dalla colonna: la misura precedente diceva il contrario perché guardava una colonna alta cinque volte e mezzo | `G9` — `PIE-HEXPLAY-6` e `PIE-VIS-SIGHTWALL`, due dei quattro ❌ del registro. Combat log, `URTTurnLogLibrary::DescribeEntry`, privacy della conoscenza. ➡️ **Richiede**: il corpo di `#2534` che dichiara la via scelta con la motivazione (è una voce della sua DoD), un test che verifichi che il log non nomini celle oltre il velo, e la **rigiudicazione delle due voci sullo stesso banco** — non su `L_HexArena`, dove le celle di barriera portano entrambi i flag e le due forme si sovrappongono |
```

### `D-341`

```markdown
| **D-341** | **IL RENAME DEGLI ID D'EROE SI COMPLETA DENTRO LA v0.1: le tre fette residue — `Hero.Gadget → Hero.Aevik`, `Hero.Phase → Hero.Muiren`, `Hero.Wraith → Hero.Ivrin` — entrano nel perimetro della release.** ⛔ **Non supersede [D-337](RT_PDR_00_Decision_Log.md)**, che resta il **regime** (rename secco) e non è toccata: questa voce ne decide la **collocazione temporale**, che `D-337` dichiarava esplicitamente di non decidere — *«non esegue il rename: è il regime, non l'applicazione»*. **(1) Cosa cambia.** [#2491](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2491) e [#2297](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2297) escono da `post-v0.1` `P3` ed entrano nella v0.1. Il vocabolario è oggi **a un quarto**: solo `Riktor → Branth` è fatto. **(2) Il costo, misurato sul precedente e non stimato.** La fetta eseguita ha toccato **200 file** (`+1011 / −993`, aggregato dei commit `--grep=2491`) ed era la **più piccola**: `Hero.Riktor` aveva **1** file `Source/` e **0** scenari. Le tre residue, contate con `grep -rl "Hero\.<id>"` — `Source/` · `Scenarios/` · `docs/`: `Gadget` **60 · 97 · 78**, `Wraith` **57 · 74 · 75**, `Phase` **33 · 44 · 71**. **(3) I golden sono SETTE, non otto, e il conteggio ingenuo sbaglia esattamente come `D-337` aveva previsto.** Il selettore `(Gadget\|Phase\|Wraith\|Riktor)` senza il prefisso `Hero.` dà **8** file su 20; con `Hero\.<X>` e `grep -a` — i `.rttl` sono **binari** — dà **7**, tutti in `Tests/Golden/`. L'ottavo è `Golden/Spec.Environment.WaterQuenchesFire/turn-02.rttl`, che porta **7 occorrenze di `Gadget.Sprinkler`**: è **equipaggiamento**, il namespace che [D-120](RT_PDR_00_Decision_Log.md) ha separato. ⚠️ **Questa nota esiste perché la trappola ha colpito di nuovo**, e `D-337` la descriveva già: chi rimisura senza il prefisso la ritroverà. **(4) Le due conseguenze accettate.** *Rename secco*: nessun redirect, nessun resolver, nessuna finestra di compatibilità — ogni riferimento mancato è **rotto**, non degradato. *Le tracce prodotte col vocabolario precedente non si rileggono* come lo stesso replay logico, e [D-310](RT_PDR_00_Decision_Log.md) le rifiuta fail-closed. Entrambe erano già dichiarate da `D-337`: questa voce le **paga**, non le scopre. **(5) Ciò che rende la decisione eseguibile, ed è un fatto della fetta precedente.** Il commit `ced0d790` ha staccato il **pacchetto d'asset** dall'`HeroId`: `BP_Unit_Riktor.uasset` esiste ancora e contiene `Hero.Branth`. **Si rinomina l'identità, non il pacchetto** — i binari Unreal restano fuori dal write-set, mentre le icone d'azione, che portano l'id nel **nome file**, seguono l'identità. È la sola ragione per cui questa migrazione è una milestone e non una release. **(6) La collocazione, che è parte della decisione.** Va **in testa**, subito dopo il ripristino del build, e **da sola**: ogni lavoro eseguito prima le aggiunge superficie, e ogni evidenza registrata col vocabolario vecchio va **rifatta**, cioè costa una seduta d'editor. Alla misura le PR aperte erano **zero**: la finestra ad albero fermo non si ripresenta da sola. **(7) Cosa NON si decide.** ⛔ Non si toccano i **display name** delle abilità. ⛔ Non si rinominano i `.uasset` del pacchetto personaggio. ⛔ Non si riaprono `D-130`, `D-134`, `D-321`, `D-334` né `D-337` | **Accettata** — **decisione d'autore** *(2026-09-06)*, stessa sessione di [D-338](RT_PDR_00_Decision_Log.md). ⚠️ **Numero verificato nella stessa misura a tre posti**: vedi `D-338`. 🔴 **È la decisione che ALLUNGA la chiusura**, e l'analisi che la precedeva raccomandava il contrario — fermarsi e dichiarare il vocabolario misto fra i limiti noti. La raccomandazione è stata respinta sul merito: un pacchetto spedito con un vocabolario a metà è un debito che si paga comunque, e più tardi costa la rigenerazione di un'evidenza in più. ⚠️ **Il perimetro della v0.1 si è ampliato due volte in due giorni** — [D-332](RT_PDR_00_Decision_Log.md) il 2026-09-05 e questa il 2026-09-06 — ed è il **ritmo** il dato da guardare, non i due singoli | Roster, catalogo, scenari, corpus golden, icone, documentazione. ⛔ **Nessun `.uasset` di personaggio toccato** (punto 5). ➡️ **Richiede**: le label e la milestone di `#2491` e `#2297` allineate, la lista nominale dei **7** golden da rigenerare aggiornata in `#2491` — il suo corpo cita il conteggio precedente — e un oracolo che fallisca se un'identità legacy ricompare in `Source/` o `Scenarios/` |
```

---

## 6. Cosa serve oltre alle quattro voci

Una decisione registrata e non applicata ai suoi owner è la deriva che questo repository paga da mesi. Per
ciascuna, ciò che resta da fare **altrove**:

| Decisione | Owner da toccare | Cosa |
|---|---|---|
| `D-338` | [`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) §3 | Cella `G13`: la riserva si riscrive **con la ragione**, non si cancella |
| `D-338` | [#85](https://github.com/DegrassiAaron/refactor-tactics-main/issues/85) | La via a punti entra fra «i limiti aperti della v0.1 dichiarati» |
| `D-339` | [#959](https://github.com/DegrassiAaron/refactor-tactics-main/issues/959) · [#79](https://github.com/DegrassiAaron/refactor-tactics-main/issues/79) | Registrare che il verdetto di `#959` vive in `PIE-V01-PACKAGED`, ancora ⏳; `#79` è prerequisito |
| `D-340` | [#2534](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2534) | Il corpo dichiara la via scelta e la motivazione — è una voce della sua DoD |
| `D-340` | `Source/RefactorTactics/Turn/` · `Tests/` | La riga di rifiuto nomina la cella; un test verifica che non nomini oltre il velo |
| `D-341` | [#2491](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2491) · [#2297](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2297) | Label `post-v0.1` → `v0.1`, milestone, e la lista nominale dei **7** golden |
| tutte | [`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) | Se una delle quattro apre o chiude un conflitto documentale registrato |

⛔ **Nessuna di queste è stata eseguita da questa sessione**, e la ragione è la stessa per tutte: sono
modifiche a owner — DoD, issue, label, milestone — che appartengono a chi registra le decisioni, non a chi le
istruisce.

---

## 7. La rotta di milestone che ne deriva

Dodici milestone obbligatorie e cinque parallele opzionali, verticali e non speculari ai 14 Domain Roadmap.
Il dettaglio per milestone — scope, acceptance criteria binari, sette gate ciascuna, exit condition — vive
nell'artefatto di sessione; qui resta la **sequenza**, che è ciò che il repository deve poter citare.

| M | Goal | Primary Domain | Dipende da | Release |
|---|---|---|---|---|
| **M0** | I tre target compilano e una sola misura VALIDA copre lo stesso SHA | 14 · QA | — | required |
| **M1** | Un solo vocabolario d'identità nel codice, nei dati e nell'evidenza | 13 · Data | M0 | required |
| **M2** | Nessuno scenario dichiara verde ciò che non ha eseguito | 14 · QA | M1 | required |
| **M3** | Chi è spinto oltre un bordo aperto cade, subisce e lascia una catena causale | 1 · Gameplay | M1 | required |
| **M4** | Una persona decide `FIRE`/`HOLD` dentro la risoluzione, e la scelta è input del replay | 1 · Gameplay | M1 | required |
| **M5** | L'HUD di partita è un solo strato UMG e `rt.HUD.CanvasPanels` sparisce | 8 · UI | M1 | required |
| **M6** | Ogni evento a schermo ha il suo reason code nello stesso turno, e il log nomina ciò che ferma il tiro | 8 · UI | M3 M4 M5 | required |
| **M7** | Nessun cilindro e nessuna T-pose in campo | 7 · Animation | M1 | required |
| **M8** | Dal pacchetto: menu → partita sulla mappa d'autore → esito terminale → Result → menu | 1 · Gameplay | M6 M7 | required |
| **M9** | Ogni voce PIE del perimetro v0.1 porta un verdetto reale | 14 · QA | M2 M8 | required |
| **M10** | Ogni riga della tabella KPI ha un valore o una ragione, con la pendenza dichiarata | 14 · QA | M9 | required |
| **M11** | I quattordici gate sono verdi con evidenza, e la v0.1 è dichiarata | 14 · QA | M9 M10 | required |
| M12–M16 | Pianificazione leggibile · conoscenza parziale · icon language · showcase «Il Relè» · interaction graph | — | — | optional |

**Percorso critico**: `M0 → M1 → M2 → M6 → M8 → M9 → M10 → M11`. `M7` non è in quella riga ma vi confluisce
due volte — l'evidenza di `M8` deve mostrare i corpi che si spediscono, e `U19` ne dipende per le righe di
ritmo di `G11`.

⚠️ **`M3`, `M4` e `M5` sono `CURRENT REQUIRED` e fuori dal percorso critico**: sono obbligazioni di
**contratto** — [D-332](../../decisions/RT_PDR_00_Decision_Log.md), l'epic `P0`
[#152](https://github.com/DegrassiAaron/refactor-tactics-main/issues/152),
[D-320](../../decisions/RT_PDR_00_Decision_Log.md) — non di **gate**. È la distinzione che permette di
eseguirle in parallelo senza che nessuna decida quando si spedisce.

---

## 8. Ciò che resta aperto **di proposito**

Non è debito. Due voci erano state elencate come bloccanti in prima analisi, e la misura le ha smentite.

| ID | Perché non si decide adesso | Innesco |
|---|---|---|
| `MOV-3` | 🔴 **Correzione.** [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) la marca `NON LOCKED — PLAYTEST OPTION` e scrive che *«la domanda è di sensazione, non di codice»*. Ha inoltre una dipendenza che il mandato non nomina: lo `Sprint` ha una migrazione di macro-fase aperta ([#199](https://github.com/DegrassiAaron/refactor-tactics-main/issues/199)), e decidere la cadenza prima la **deciderebbe per inerzia** | Un playtest che misuri lo Sprint come «abbastanza lungo ma non abbastanza rapido» |
| `MSE-3` | 🔴 **Correzione: era attribuita male.** Era stata data come blocco alla leggibilità della lastra; il suo innesco è [#621](https://github.com/DegrassiAaron/refactor-tactics-main/issues/621), la cottura — *«il primo codice che deve scegliere»*. La lastra è un volume di presentazione e non tocca l'occupancy, e `D-340` la toglie comunque dal percorso | [#621](https://github.com/DegrassiAaron/refactor-tactics-main/issues/621) · tracciata in [#717](https://github.com/DegrassiAaron/refactor-tactics-main/issues/717) |
| `AE-5` | Costo, portata e rumore dello `Sneak` non sono definiti da **nessuna fonte corrente**, e il catalogo lo dichiara. Non si inventano | La mancanza n. 3 di `E48` resta un limite noto |
| `INT-1` | Asse di bilanciamento: se un solo eroe porta `Interaction.Force`, ogni mappa con una porta rinforzata diventa una mappa su quell'eroe | `E23`, se si apre |
| `GOV-6` | Non blocca — il documento lo dichiara: *«è una questione di sede, non di correttezza»* | La prossima revisione del contratto |

---

## 9. Una correzione a una misura di questo referto

La prima stesura dichiarava **«8 dei 20 golden portano un ID d'eroe»**. È **7**, e l'errore è quello che
[`D-337`](../../decisions/RT_PDR_00_Decision_Log.md) punto **(6)** aveva già registrato come *«una trappola di
misura, registrata perché è costata due volte in due giorni»*.

```bash
git grep -l -a -E 'Gadget|Phase|Wraith|Riktor' origin/main -- '*.rttl' | wc -l          # 8  ← ingenuo
git grep -l -a -E 'Hero\.(Gadget|Phase|Wraith|Riktor)' origin/main -- '*.rttl' | wc -l  # 7  ← corretto
```

L'ottavo è `Golden/Spec.Environment.WaterQuenchesFire/turn-02.rttl`, che porta **sette** occorrenze di
`Gadget.Sprinkler` e **zero** `Hero.<X>`: è **equipaggiamento**, non l'eroe. Due cose valgono più della
correzione. **Una**: il prefisso `Hero.` *è* il discriminante, ed è la ragione per cui `D-130` scelse
`Hero.<Nome>.<Abilità>`. **Due**: i `.rttl` sono **binari**, e senza `-a` un grep li salta **in silenzio** —
un'assenza che sembra misurata.

∴ è la terza volta che lo stesso selettore produce un numero falso, e la difesa non è ricordarselo: è che ogni
cifra di questo referto porti accanto il comando che l'ha prodotta.

---

## 10. Cosa questo referto NON fa

- ⛔ **Non registra le decisioni**: le quattro voci di §5 sono testo pronto, non righe del registro.
- ⛔ **Non tocca nessun owner**: né il DoD, né una issue, né una label, né una milestone. §6 elenca cosa resta.
- ⛔ **Non implementa nessuna milestone.** La sequenza di §7 è architettura, non lavoro iniziato.
- ⛔ **Non esegue il rename** di `D-341`, né rigenera alcun golden.
- ⚠️ **Non ha eseguito nessun gate.** Ogni verdetto qui è `NOT RUN` per costruzione: la sessione ha misurato
  l'albero e GitHub, non ha compilato, non ha lanciato la suite, non ha aperto l'Editor. Il reperto di §3 è
  **statico** — conteggio delle direttive del preprocessore — e va confermato da una build reale prima di
  essere trattato come `FAIL`.
