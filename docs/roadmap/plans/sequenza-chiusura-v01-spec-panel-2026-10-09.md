# Sequenza di chiusura della v0.1 — referto spec panel del 2026-10-09

> **Stato**: `CURRENT` · **Creato**: 2026-10-09 · **Tipo**: **vista di esecuzione**, non owner.
>
> **Oggetto della revisione**: [`sequenza-chiusura-v01-2026-10-03.md`](sequenza-chiusura-v01-2026-10-03.md),
> la sequenza di chiusura della v0.1 riletta sul candidate congelato `95eddfd37`. Questo referto la
> **critica e la corregge**; non la sostituisce come documento, e non è un'assegnazione.
>
> **Base di misura**: `main` = `ae805683a` = `origin/main` dopo `git fetch --prune`,
> `git status --porcelain` → **0** righe. Stato di issue e PR letto lato server con `gh` il
> **2026-10-09**. Nessuna misura con il motore: niente build, niente Automation, niente PIE.
>
> **Modalità**: `critique`. **Panel**: Wiegers (requisiti e criteri), Adzic (esempi falsificabili),
> Cockburn (attori e obiettivi), Fowler (struttura e fonte unica), Nygard (modi di guasto e
> operatività), Crispin (evidenza e test).
>
> **Cosa non è**: una seconda Definition of Done, una riscrittura di scope, una decisione d'autore.
> Il criterio di consegna resta [`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) §3;
> `#85` **attesta**. Dove questo referto e un owner divergono, **ha ragione l'owner**. Le decisioni
> che il referto individua sono elencate in §3 come `BLOCKED — DECISION REQUIRED`
> ([`CLAUDE.md`](../../../CLAUDE.md) §13): qui si rendono **prendibili**, non si prendono.
>
> **Convenzione sui numeri** ([`AGENTS.md`](../../../AGENTS.md) §14): nessun conteggio che cambi da
> solo. I numeri scritti sono **esiti del passaggio corrente**, ciascuno col comando che lo produce;
> dove serve un insieme, ci sono i **nomi**.

---

**Indice** · [0. Il verdetto](#0-il-verdetto) · [1. Cosa è cambiato](#1-cosa-è-cambiato-dal-2026-10-03-misurato) ·
[2. Rilievi del panel](#2-rilievi-del-panel) · [3. Decisioni richieste](#3-decisioni-richieste) ·
[4. La sequenza corretta](#4-la-sequenza-corretta) · [5. Cosa la sequenza del 3 ottobre aveva giusto](#5-cosa-la-sequenza-del-3-ottobre-aveva-giusto) ·
[6. Limiti dichiarati](#6-limiti-dichiarati)

---

## 0. Il verdetto

La sequenza del 2026-10-03 era **corretta il giorno in cui è stata scritta** e oggi è **superata nella
premessa**, non nel metodo. Tre fatti la svuotano:

1. **Il runtime non è più quello del candidate.** La sua §2.1 dichiarava *«questa riga invecchierà:
   vale finché il comando 2 risponde `0`»*. Oggi risponde **5222** (§1.1). Ogni gate verde *«sul
   candidate `95eddfd37`»* — `G1` `G2` `G3` `G4` `G5` `G6` `G8` `G10` `G11` `G12` — è verde su un
   albero che `main` ha sorpassato con lavoro di gameplay e presentazione, e la sequenza non lo dice
   da nessuna parte: la riga che doveva invecchiare è invecchiata **in silenzio**.
2. **Il candidate congelato non può chiudere `G16`.** Il passo 5 del giro `U60` riapre nel Replay
   Viewer l'archivio della partita appena giocata; sul pacchetto cotto da `95eddfd37` quella partita
   **non produce nessun archivio** ([#3463](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3463),
   trovato nel giro 2 del 2026-10-04 e corretto **dopo** il congelamento, §1.2). ∴ restare su
   `95eddfd37` non è un'opzione conservativa: è un candidate su cui un gate di release è rosso per
   costruzione.
3. **L'ordine delle onde produce un secondo congelamento.** L'Onda C — il codice del Turn Log
   proiettato, passo 4 di `G16` — stava **dopo** l'Onda B, cioè dopo le misure che dipendono dalla
   build. Chiunque la esegua tocca `Source/` e invalida ciò che l'Onda B ha appena ridatato. Il
   congelamento va **dopo** l'ultimo commit di runtime previsto, non prima (§2.2).

Il referto propone in §4 una sequenza in cui **il codice precede il congelamento**, il congelamento
apre una **finestra dichiarata** in cui `Source/` e `Content/` non si toccano, e le sedute a motore
seguono in un ordine fisso. Cinque decisioni d'autore la condizionano (§3); la prima — su quale SHA
si ricongela — è quella che tutte le altre aspettano.

---

## 1. Cosa è cambiato dal 2026-10-03, misurato

### 1.1 🔴 Il candidate è stato sorpassato dal runtime

I **tre comandi che la sequenza stessa prescrive** in §2.1, eseguiti su `ae805683a`:

```sh
git diff --name-only 95eddfd37..HEAD -- Source/ Content/ | wc -l                       # 169
git diff --name-only 95eddfd37..HEAD -- Source/ Content/ | grep -v '/Tests/' | wc -l   # 104
git diff 95eddfd37..HEAD -- Source/ Content/ ':!*/Tests/*' | grep -E '^[+-]' \
  | grep -vE '^[+-]{3}' | grep -vcE '^[+-][[:space:]]*(//|\*|/\*|$)'                   # 5222
```

Il terzo è quello che la sequenza chiama *«il comando che dà il verdetto»*, e il 2026-10-03
rispondeva `0`. Le PR entrate dal 2026-10-07 — `gh pr list --state merged --search "merged:>=2026-10-07"`
ne elenca **41** oggi — sono lavoro di gameplay e presentazione, non documentazione: fra le altre
il tracer degli attacchi base ([#3552](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3552)),
il beat di cast e il Blast come sequenza per intento ([#3561](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3561)),
il profilo FX per abilità ([#3585](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3585)),
la clip per abilità ([#3571](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3571)),
la cura ad area ([#3599](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3599)),
una sola origine di mira per fase ([#3550](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3550), `D-464`),
la tastiera in sola lettura durante la risoluzione ([#3547](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3547), `D-468`),
e le decisioni `D-470`–`D-476` sullo scatto, la timeline e lo zombie dell'Editor.

🔑 **Non è un difetto del lavoro entrato**: è lavoro della v0.1, convocato da sedute `U67`–`U71`
eseguite e registrate. Il difetto è che la sequenza **non prevedeva una corsia per quel lavoro** e
non aveva un trigger che la dichiarasse superata quando il lavoro è arrivato (§2.1, §2.4).

### 1.2 🔴 `#3463` cade DOPO il candidate, e decide da sola la domanda sul ricongelamento

```sh
git merge-base --is-ancestor ef1aa1895 95eddfd37 && echo "nel candidate" || echo "DOPO"   # DOPO
```

`ef1aa1895` è il merge di [#3464](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3464)
(2026-10-04), che insieme a [#3466](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3466)
chiude `#3463`: *«sul pacchetto una partita completa non produce nessun archivio, e
`BeginReplayRecording` esce in silenzio»*. Il record di `U60` in
[`editor-sessions.yaml`](../editor-sessions.yaml) lo registra come *«il difetto vero, ed è più
grave»*. Il pacchetto staged del 2026-10-02 — quello che la sequenza dichiarava *«da non
ricostruire»* — contiene quel difetto, e il passo 5 di `G16` lo incontra per forza.

### 1.3 🟡 `G16` ha un giro 2, e metà del giro è automatizzata

Il record di `U60` dice: giro 2 del 2026-10-04, **passi 1-2 ✅**, passi 3-4-5 `NOT RUN`; e la metà
meccanica è ora [`tools/seduta/giro-meccanico.ps1`](../../../tools/seduta/giro-meccanico.ps1), con
*«undici esiti su undici PASS»* e il feed che **si popola** sul pacchetto in autobattle
(*«TurnLog=22 voci … righe dopo filtro=5»*). La sequenza (§3.4) descrive ancora il *primo* giro come
futuro. Lo script non chiude `G16` — lo dichiara in testa — ma cambia cosa resta a una persona: i
passi 3-4-5, e il giudizio percettivo.

### 1.4 🟡 L'Onda C non si è mossa; l'Onda B è stata scavalcata

[#3073](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3073),
[#2764](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2764),
[#2964](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2964),
[#1936](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1936) e
[#79](https://github.com/DegrassiAaron/refactor-tactics-main/issues/79) sono tutte `OPEN`, senza
commenti dopo il 2026-10-03. Intanto `B1` — la seduta `G13`, *«la coda più corta, eseguibile senza
ricostruire niente»* — non è stata eseguita: la cella `G13` è ancora 🟡, e l'Editor ha servito le
sedute `U67`, `U68`, `U69`, `U70`, `U71` fra il 7 e il 9 ottobre. Il motore è uno, e l'ordine che la
sequenza fissava per quel motore non è stato quello seguito.

### 1.5 🟡 Le tre PR del gruppo vecchio sono ferme da tre settimane

| PR | Stato oggi | Distanza da `main` |
|---|---|---|
| [#3230](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3230) `D-415`, mira pianificata e tiro alla cieca | `DRAFT`, `CLEAN`, ultimo commit 2026-09-20 | `git rev-list --count <branch>..origin/main` → **875** |
| [#3205](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3205) lo Sprint nella proiezione | `OPEN`, `CLEAN`, 2026-09-18 | **938** |
| [#3203](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3203) il canale della riduzione da reazione | `OPEN`, **`DIRTY`**, 2026-09-18 | **938** |

⌫ *La prima stesura di questo paragrafo diceva che `#3203` «rende vero il nome del test citato dalla cella `G4`» e che
la cella «cita ancora» `Combat.GuardPoolIsPermutationInvariant`. **Entrambe le cose sono false**, e lo dice la cella
stessa letta per intero: il criterio (colonna 3) cita solo `Replay.Verifier.ResimulationIsDeterministic`; il nome
`GuardPool…` compare nella colonna di stato **come nome storico dichiarato** — *«si chiamava … fino al 2026-09-20»*,
con `D-408` — e la cella si era già corretta il 2026-10-03. `#3203` rinomina il canale della riduzione da reazione
(`DeflectDelta`), non quel test. Il rilievo che resta su `#3203` è solo che è `DIRTY` da tre settimane (§7).*

### 1.6 🟡 La milestone `PIA` tace da un mese

`gh issue view <n> --json comments` → `#2617` (PIA-2) **0** commenti, `#2619` (PIA-4) **0**,
`#2621` (PIA-6) **0**, tutte con `updatedAt` 2026-09-06. Nel frattempo il lavoro che **serve PIA-2**
— tracer, beat, FX, clip — è entrato ed è stato giudicato a schermo (`PIE-V01-TRACER` ✅ in `U68`,
`PIE-CAST-BEAT` ✅ in `U69`/`U70`, `PIE-FX-ABILITA` ✅ in `U71`, `PIE-CLIP-ABILITA` ✅ in `U70`
riconvocata). I gate `PIA-2.1`–`PIA-2.10` hanno quindi evidenza che nessuno ha riportato, e la
sequenza non nomina **nessuna** delle issue `#2616`–`#2623`.

---

## 2. Rilievi del panel

Formato: severità · esperto · difetto · raccomandazione · priorità.

### 2.1 🔴 NYGARD — Una premessa che scade senza un trigger scade in silenzio

**Difetto.** La §2.1 della sequenza porta la clausola giusta — *«vale finché il comando risponde `0`»*
— ma nessun passo operativo la **esegue**. Il runbook di `G13`
([`guida-seduta-g13-candidate.md`](../../technical/runbooks/guida-seduta-g13-candidate.md)) verifica il binario
**per contenuto** (l'header a venti colonne), non che il runtime di `HEAD` sia quello del candidate. Quello di `G16`
([`guida-seduta-g16-u60.md`](../../technical/runbooks/guida-seduta-g16-u60.md)) **ha** il blocco `rev-parse` nel
preflight — ⌫ *la prima stesura di questo referto diceva di no, ed era falso (§7)* — ma il suo esito `DIVERSI`
prescriveva *«fai checkout di `95eddfd37`»*, cioè condurre il giro proprio sul candidate su cui il passo 5 è rosso
per costruzione (§1.2). Nessuno dei due fogli ha un esito di **arresto**: una seduta condotta oggi attesterebbe
una build sbagliata in un verso o nell'altro, e il foglio non se ne accorgerebbe.

**Raccomandazione.**
1. Il blocco `rev-parse` della §2.1 entra nel **passo 0** di `G13` e acquisisce in `G16` l'esito di **arresto**:
   su `DIVERSI` la seduta è `NOT RUN — candidate superato`, non si apre l'Editor e non si fa il checkout.
2. Il congelamento apre una **finestra dichiarata** nell'owner (`#85`, passo 1, oppure una nota
   sotto la tabella §3 del DoD): *«fra il freeze e l'attestazione nessun merge tocca `Source/` o
   `Content/`, salvo la correzione di un gate rosso, che ricongela e ridata»*. Oggi questa regola
   non è scritta da nessuna parte, ed è la ragione per cui quarantuno PR sono entrate senza che
   nessuna violasse qualcosa.
3. La sequenza stessa dichiara in testa **il comando che la invalida**, come fa per i numeri: chi la
   apre lo esegue prima di leggerla.

**Priorità**: alta — è il difetto che ha svuotato il documento.

### 2.2 🔴 FOWLER — Il codice del passo 4 sta nell'onda sbagliata

**Difetto.** L'Onda C (`#3073` → `#2764` → `#2964` → `#1936`) è l'unico lavoro di `Source/` che la
sequenza prevede, e sta **dopo** l'Onda B, che ridata `G3`–`G8` e `G11` sul candidate e conduce
`G13`/`G16`. Qualunque esito dell'Onda C invalida l'Onda B: la sequenza prescriveva, per
costruzione, **due** congelamenti. La §3.4 lo accettava implicitamente (*«il primo giro produce
necessariamente 🔴 sul passo 4, va eseguito comunque»*) senza dire che il secondo giro avrebbe avuto
bisogno di un secondo candidate.

**Raccomandazione.** Invertire: **codice → congelamento → misure → sedute**. L'Onda C precede il
freeze; il giro `U60` si fa **una volta**, dopo, con i cinque passi eseguibili. Se l'autore decide
che `#1936` esce dalla v0.1 (§3, `D3`), allora cambia la cella `G16` — non si lascia un gate rosso
atteso.

**Priorità**: alta.

### 2.3 🔴 WIEGERS — Le `P1` fuori dai gate sono un taglio non dichiarato

**Difetto.** La sequenza risponde alla domanda *«quali issue restano»* risalendo **solo** dai gate
non verdi (§1 del documento). È la lettura di `D-377`. Ma la descrizione della milestone — l'owner
della semantica delle label — dice: *«`P1` core della v0.1 · `P2` tagliabile se il tempo stringe ·
`P3` prima da tagliare»* e *«una issue `P2` o `P3` può essere tagliata se il taglio non rende rosso
nessun gate»*. Le `P1` **non** sono nell'insieme tagliabile, e la sequenza non le nomina: è un
taglio di fatto, senza decisione.

Le `P1` **aperte nella milestone `v0.1`** che non sono contenitori (`epic` o `checkpoint`) e che la
sequenza non cita — `gh issue list --milestone "v0.1 — Offline Vertical Slice" --label P1 --state open`:

- [#2297](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2297) migrazione identità roster, piano a fette A–F;
- [#2554](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2554) l'anteprima dell'Anim Browser — codice su `main`, DoD a sei caselle non verificate (lo dice la sequenza stessa in §8.2);
- [#2744](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2744) lo Screen HUD in autobattle;
- [#2748](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2748) AutoBattle Viewer 0.1;
- [#2793](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2793) **bug**: il ventaglio verde ha un buco dove sta un nemico che la squadra non conosce — un leak di conoscenza nell'anteprima di pianificazione;
- [#2826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2826) l'Action Dock arma e non si spiega;
- [#3063](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3063) il click su una cella mai osservata — la sequenza la dichiara *«non una regressione viva»*, resta aperta per i criteri PIE;
- [#1936](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1936) — l'unica che la sequenza nomina, perché è sul percorso di `G16`.

E le `P1` con label `v0.1` ma **senza milestone**, invisibili a chi elenca la release:
[#2951](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2951) (`Shape::Line` verso un'altra piattaforma),
[#2988](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2988) (gli stati dello slot `Planned` e `Disabled by phase`),
[#2989](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2989) (il cablaggio del dock senza oracolo),
[#3135](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3135) (`D-415`, la PR `#3230` in bozza).

⚠️ **`#2793` merita una riga a sé.** `G8` misura che nessun *intento* avversario sia replicato; non
misura che l'anteprima di movimento non disegni la conoscenza dell'autorità. Il difetto è della
stessa famiglia della privacy, su una superficie che nessun gate copre. Non è un argomento per
aggiungere un gate: è un argomento per **decidere** se è nella v0.1.

**Raccomandazione.** Una decisione d'autore in due righe (§3, `D2`): o le `P1` non-gate sono
**differite** con un `D-nnn` e la milestone lo registra (come `D-441` ha fatto per `U5`), o entrano
nella sequenza **prima** del congelamento, perché toccano `Source/`. Lasciarle dove sono è il terzo
stato — quello che la sequenza del 2026-09-27 chiamava *«il silenzio non è uno stato»*.

**Priorità**: alta — condiziona la data del congelamento.

### 2.4 🟡 COCKBURN — Chi esegue l'ordine, e chi ha eseguito un altro ordine

**Difetto.** La sequenza nomina **cosa** fare e **perché**, mai **chi**. L'Onda B dice *«il motore,
una cosa per volta»* e mette `G13` per prima; fra il 7 e il 9 ottobre il motore ha fatto cinque
sedute diverse, tutte legittime, nessuna delle quali era nell'elenco. Non è indisciplina: è che
l'attore primario di quelle sedute — chi fa presentazione e combat feedback, cioè `PIA-2` — **non
aveva una riga** nella sequenza, e ha usato il motore quando era libero.

Lo stesso vale per il riporto: `PIA-2`, `PIA-4` e `PIA-6` hanno zero commenti da un mese (§1.6)
mentre i loro gate maturavano altrove. `PIA-6` esige un *evidence pack* e `#85` un *evidence bundle*:
due attestazioni, due issue, nessuna delle due nomina l'altra.

**Raccomandazione.**
1. La sequenza porta **una corsia per `PIA-2`** con le voci che restano (`PIA-2.6` stati `#2456`,
   `PIA-2.7`/`PIA-2.8` `#2622`, `PIA-2.9` diagnostica OFF `#2457`, `PIA-2.10` hazard `#2505`) e dice
   dove si registra ciò che è già verde.
2. `#2621` diventa **input** di `#85`: l'evidence pack di PIA è una sezione del bundle, non un secondo
   bundle. Una riga in ciascuna delle due issue basta.
3. Ogni onda a motore dichiara **chi** la conduce, o almeno che è *«la prossima apertura
   dell'Editor, chiunque la faccia»*: è ciò che rende un ordine un ordine.

**Priorità**: media.

### 2.5 🟡 CRISPIN — Tre gate percettivi si rifanno, e uno è stato reso impossibile dal candidate

**Difetto.** La sequenza tratta `G10` come chiuso (*«RIGIOCATA sul candidate»*) e `G13`/`G16` come
le due code. Ma `G10` è una **partita giocata da una persona**: cambia il candidate, si rigioca.
`G13` idem. `G16` ha il passo 5 impossibile sul candidate vecchio (§1.2). ∴ dopo il ricongelamento
le sedute a motore sono **tre**, non due, e la sequenza deve metterle in fila con il loro ordine
interno: `G13` (una partita presidiata sul pacchetto) · `G10` (una partita completa non degenere con
una reaction risolta) · `G16` (il giro `U60`, cinque passi).

**E una misura prima di scrivere codice.** [#2964](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2964)
dice *«il feed è montato, il turno produce eventi, a schermo non compare nessuna riga»*. Il giro
meccanico del 2026-10-04 misura sul pacchetto, in autobattle, *«righe dopo filtro=5»*. Le due
osservazioni possono convivere — una in partita presidiata, l'altra in autobattle — ma finché
nessuno lo misura, `#2964` rischia di essere la stessa cosa che la sequenza sospettava di `#3073`:
un difetto falso che costa una seduta.

**Raccomandazione.** `#2964` e `#3073` entrano nell'Onda C come **misure** (un test headless che
cammina il grafo del widget; una partita presidiata col feed osservato), e solo dopo come
correzioni. E la cella `G10` acquisisce la clausola *«si rigioca a ogni candidate»*, come `G12` ha
*«ridatato a ogni release»*.

**Priorità**: media.

### 2.6 🟡 ADZIC — La sequenza non ha un esempio di quando una sua riga smette di valere

**Difetto.** Il documento è ricco di comandi e povero di **scenari**: non dice, in forma
eseguibile, cosa succede alla sequenza quando entra una PR. È per questo che quarantuno PR sono
entrate senza che nessuno rileggesse §2.1.

**Raccomandazione.** Tre scenari in testa alla sequenza, nella forma che il repository già usa per
le voci PIE:

> **Dato** un candidate congelato `X`, **quando** un merge tocca `Source/` o `Content/` fuori da
> `Tests/`, **allora** `G1` `G2` `G3` `G4` `G6` `G8` `G10` `G11` `G12` `G13` `G16` tornano `⏳` per
> l'attestazione, e il candidate va ridichiarato.
>
> **Dato** un candidate congelato `X`, **quando** un merge tocca solo `Tests/` o `docs/`, **allora**
> `G2` e `G14` si ridatano, gli altri no.
>
> **Dato** un gate rosso sul candidate, **quando** la correzione entra, **allora** il candidate è il
> commit della correzione, e il giro ricomincia dal gate rosso — non da capo.

Sono le regole che `#85` scrive in prosa (*«se il candidate SHA cambia, le misure che dipendono
dalla build si rifanno»*); scritte così possono **fallire** su un caso concreto.

**Priorità**: media.

### 2.7 🟡 FOWLER — Quattro viste di esecuzione `CURRENT`, nessuna precedenza dichiarata

**Difetto.** Oggi dichiarano di ordinare il lavoro della v0.1:
[`roadmap-main-v0.1.md`](../roadmap-main-v0.1.md) (tre lane, sei wave, `CURRENT` dal 2026-08-28),
[`roadmap-pia.md`](../roadmap-pia.md) §7 (il grafo `PIA-0` → `PIA-6`),
[`execution-graph.yaml`](../execution-graph.yaml) (quattro catene, *«nessuno script legge questo
file»*), e la sequenza del 2026-10-03. Nessuna delle quattro nomina le altre come subordinate.

Il grafo di esecuzione in particolare: dei suoi nodi `issue:<n>` restano aperti solo
`#79` `#166` `#171` `#289` `#314` `#319` `#542` — comando:
`grep -oE '^  - id: issue:[0-9]+' docs/roadmap/execution-graph.yaml`, incrociato con `gh issue list
--state open` — e il suo header dichiara che resta *«perché la topologia non è ricavabile altrove,
non perché qualcosa la consumi»*. È un archivio che si presenta come strumento.

**Raccomandazione.** Una riga in [`README.md`](README.md) di questa cartella: *la sequenza di
chiusura è la vista di esecuzione della v0.1; `roadmap-pia.md` §7 governa l'ordine interno di PIA;
`roadmap-main-v0.1.md` passa a `SNAPSHOT`; `execution-graph.yaml` si archivia o riceve un lettore*.
Non è lavoro di codice: è una decisione di scaffale (§3, `D5`).

**Priorità**: bassa, ma a costo zero.

### 2.8 🟡 WIEGERS — Tre PR senza esito sono tre decisioni non prese

**Difetto.** `#3203`, `#3205` e `#3230` (§1.5) sono a novecento commit da `main`. La sequenza le
classificava *«non pronte»* e si fermava lì. Ma `#3230` porta `D-415` — *l'attacco base smette di
inseguire e spara alla cieca* — che è una **regola di gioco**: se entra, cambia il resolver, il
golden e i bot ([#3229](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3229) lo
misura: *«i bot mirano ancora all'unità, non alla cella»*). Se non entra, `#3135` è `P1` con label
`v0.1` e senza milestone, e nessuno l'ha differita.

**Raccomandazione.** Prima del congelamento, per ciascuna: **merge, chiusura, o differimento con
`D-nnn`**. `#3203` per prima, perché è l'unica `DIRTY`, e la sequenza del 3 ottobre misurava il conflitto come *«reale e
isolato sul Decision Log»*: si risolve in un file.
`#3230` è la decisione più pesante ed è in §3 come `D3`.

**Priorità**: alta per `#3230`, media per le altre.

### 2.9 ⚠️ Igiene che la sequenza aveva già chiesto e che non è avvenuta

| Rilievo | Chiesto il | Stato oggi |
|---|---|---|
| [#3424](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3424) senza milestone (crash GPU allo shutdown della run packaged di `G2`) | 2026-10-03, §5.6 | ancora senza milestone |
| banner del DoD fermo al 2026-09-12 mentre il file cambia | 2026-10-03, §5.7 | `sed -n 3p docs/roadmap/v0.1-definition-of-done.md` → ancora `2026-09-12` |
| marcatore di `U16` assente benché eseguita | 2026-10-03, §5.8 | non verificato oggi — resta a chi condusse la seduta |
| `A7`: la direzione del residuo di `G7` come riga di `OPEN_DECISIONS.md` | 2026-10-03, §8 | superata: `#3438` è `CLOSED` dal 2026-10-05 (`D-450`) |
| le dodici caselle della §5 del DoD, tutte vuote, *«lavoro da aprire»* (`D-449`) | 2026-10-03 | nessuna issue le riscrive: `gh issue list --search "checklist di contenuto"` restituisce solo voci estranee |
| label `v0.1` senza milestone | — | [#1881](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881) [#1937](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1937) [#2276](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2276) [#2916](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2916) [#2951](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2951) [#2988](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2988) [#2989](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2989) [#3081](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3081) [#3176](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3176) — `gh issue list --label v0.1 --state open --json milestone` filtrato su `null` |

Nessuna di queste blocca un gate. Tutte rendono la prossima rilettura più lunga.

---

## 3. Decisioni richieste

`BLOCKED — DECISION REQUIRED`. Ognuna è binaria o quasi, l'istruttoria è sopra, e **nessuna si
deduce dai documenti**: il repository dice cosa è vero, non cosa deve bastare.

✅ **Tutte e cinque prese il 2026-10-09**, nel brainstorm d'autore che ha seguito questo referto: [`D-483`](../../decisions/RT_PDR_00_Decision_Log.md)–[`D-487`](../../decisions/RT_PDR_00_Decision_Log.md).
Esiti: `D1` (b) · `D2` (c), coi dieci casi decisi uno per uno — sette entrano, `#2297` differita, `#2554` e `#3063` sola
verifica · `D3` **sì, prima del freeze**, con `#3229` nella stessa onda · `D4` (a) · `D5` (a). La colonna «Opzioni» resta
come istruttoria; la colonna «Raccomandazione» è storia.

| ID | Domanda | Opzioni e conseguenze | Raccomandazione del panel |
|---|---|---|---|
| **D1** ✅ [`D-483`](../../decisions/RT_PDR_00_Decision_Log.md) | Su quale SHA si ricongela il candidate? | **(a)** restare su `95eddfd37`: `G16` resta rosso al passo 5 per `#3463` (§1.2), e i verdetti PIE di ottobre sono su un'altra build. **(b)** ricongelare su `main` **dopo** l'Onda C e dopo `D2`/`D3`: si rifanno `G1` `G2` `G12` (una giornata di macchina), `G3`–`G8` `G11` (una run), `G10` `G13` `G16` (tre sedute). | **(b)**. (a) non è un'opzione: un candidate su cui un gate è rosso per costruzione non si attesta |
| **D2** ✅ [`D-484`](../../decisions/RT_PDR_00_Decision_Log.md) | Le `P1` non-gate di §2.3 entrano prima del freeze o sono differite? | **(a)** entrano: la data del freeze slitta di quanto costano, e vanno in Onda C. **(b)** differite con un `D-nnn` e la milestone aggiornata, come `D-441` per `U5`. **(c)** caso per caso, ma **scritto**. | **(c)**, con `#2793` esaminata per prima: è un bug di conoscenza, non una feature |
| **D3** ✅ [`D-485`](../../decisions/RT_PDR_00_Decision_Log.md) | `D-415` (`#3135`, PR `#3230`) è nella v0.1? | **(a)** sì: merge prima del freeze, golden e corpus ricalcolati, `#3229` nella stessa onda. **(b)** no: `#3135` passa a milestone `v0.2`, la PR resta in bozza o si chiude con un rimando. | nessuna: è una regola di gioco, e il panel non la possiede. Ma **va presa prima del freeze** |
| **D4** ✅ [`D-486`](../../decisions/RT_PDR_00_Decision_Log.md) | `#1936` (Turn Log proiettato, passo 4 di `G16`) è nella v0.1? | **(a)** sì: Onda C intera prima del freeze. **(b)** no: la cella `G16` si riscrive a quattro passi con un `D-nnn`, e il giro misura quattro superfici. | **(a)**, perché la milestone lo elenca fra i target (*«Turn Log / player-facing explainability»*) e il gate esiste per misurarlo insieme agli altri |
| **D5** ✅ [`D-487`](../../decisions/RT_PDR_00_Decision_Log.md) | Quale vista di esecuzione governa (§2.7)? | **(a)** la sequenza di chiusura, con le altre tre declassate. **(b)** nessuna: si tengono tutte `CURRENT`. | **(a)**; costa una riga nel `README.md` dei piani e un banner |

---

## 4. La sequenza corretta

Proposta del panel, condizionata da §3. Le frecce sono dipendenze; dentro un'onda l'ordine è quello
scritto. **Il motore è uno** ([`AGENTS.md`](../../../AGENTS.md) §11): le onde 3 e 4 sono seriali
per ragione fisica, le altre no.

```text
Onda 0  decisioni D1–D5, a motore spento            -> sblocca tutto
Onda 1  le tre PR: #3203 -> #3205 -> #3230(D3)       -> nessuna PR orfana al freeze
Onda 2  codice: Onda C del 3 ottobre + le P1 di D2   -> l'ultimo commit di runtime della v0.1
        ---------------------- FREEZE: candidate v2, finestra dichiarata ----------------------
Onda 3  misure senza persona, una giornata:
          G1 (tre build, non incrementali) -> G2 (le due meta', stessa passata)
          -> G12 (cook Dev+Shipping) -> G3 G4 G6 G8 (una run) + G5 (grep) + G7 (grep Scenarios)
          -> G11 (tre Perf.* + CSV in autobattle sul pacchetto) -> G14 (tre comandi)
Onda 4  sedute, una per apertura dell'Editor o del pacchetto:
          G13 (partita presidiata sul pacchetto, runbook) -> G10 (partita non degenere, rigiocata)
          -> G16 (U60 giro 3: i cinque passi, con giro-meccanico.ps1 per la meta' non percettiva)
Onda 5  riporto PIA su G-n, #2621 come sezione del bundle, #85 attesta
```

### Onda 0 — a motore spento, oggi

| | Lavoro | Perché adesso |
|---|---|---|
| 0.1 | `D1`–`D5` di §3, registrate nel Decision Log | tutto il resto le aspetta |
| 0.2 | la **finestra di freeze** scritta in `#85` passo 1 (§2.1) | senza, il freeze v2 scade come il v1 |
| 0.3 | i tre scenari di §2.6 in testa alla sequenza del 3 ottobre; il blocco `rev-parse` come passo 0 di `G13` e l'esito di arresto in `G16` | rende la prossima deriva **visibile** alla prima seduta |
| 0.4 | `#2621` ↔ `#85`: una riga ciascuna che dichiara l'inclusione | toglie la seconda attestazione |
| 0.5 | igiene di §2.9: milestone di `#3424` e delle nove con label `v0.1`, banner del DoD, le righe incrociate fra `#2621` e `#85` | costo zero, evita la terza rilettura |

### Onda 1 — le PR ferme

`#3203` per prima (l'unica in conflitto, isolato sul Decision Log); `#3205` riletta contro `D-425` come la
sequenza chiedeva; `#3230` **si fonde** (`D-485`), e `#3229` la segue nella stessa onda. Esito ammesso per ciascuna: `MERGED`, `CLOSED`, o **differita con
numero di decisione**. Nessuna resta `OPEN` al freeze.

### Onda 2 — il codice, prima del freeze

1. `#3073` **come misura** — il test headless che cammina il grafo di `WBP_RT_EventLog` (la
   sequenza §5.1 dice già il verso dell'asserto);
2. `#2964` **come misura** — una partita presidiata con il feed osservato, contro il
   `righe dopo filtro=5` del giro meccanico;
3. `#2764` — i cinque compositori senza consumatore, col sesto come precedente;
4. `#1936` — il tetto, se `D4` = (a);
5. le `P1` che `D-484` fa entrare — `#2793` per prima, con `BLIND-2` e `OBS-1` da decidere prima del freeze; poi
   `#2951`, `#2988`, `#2989`, `#2826`, `#2744`, `#2748`. `#2297` → `v0.2`; `#2554` e `#3063` nell'onda 4.

🔑 **L'ultimo merge di quest'onda è il candidate v2.** Non si congela prima: ogni commit qui
invalida ciò che l'Onda 3 misura.

### Onda 3 — le misure senza persona

L'ordine è quello di `#85` (*«le misure costose si concentrano alla fine e non si intrecciano»*),
con tre correzioni che la sequenza del 3 ottobre ha misurato e che restano vere:

- `G1` **non incrementale** (il referto del 27 settembre ci è cascato: §2.1 di quel documento);
- `G2` **le due metà nella stessa passata**, Editor e pacchetto;
- `G11` si scrive in **due posti**, cella §3 e righe §4.

Tutto è `NOT RUN` finché non è eseguito. Una cella verde datata 2026-10-02 non vale sul
candidate v2.

### Onda 4 — le sedute

```text
G13   runbook guida-seduta-g13-candidate.md, passo 0 = rev-parse (§2.1)
G10   partita 2v2 completa non degenere: un danno, una reaction risolta, obiettivo mosso
G16   U60 giro 3 — giro-meccanico.ps1 per i passi meccanici, una persona per i percettivi,
      esiti derivati con tools/seduta/esiti.py, archivio confermato per MatchId (sequenza §3.4)
```

Ogni seduta scrive il proprio marcatore nel record della seduta e il proprio esito nella cella
`G-n`: la sequenza del 27 settembre ha misurato cosa costa non farlo (`U1`, `U7`, `U16`).

### Onda 5 — riporto e attestazione

`PIA-2`, `PIA-3`, `PIA-4` registrano **ciò che hanno**: `PASS` con la seduta e il commit, `NOT RUN`
col motivo e l'issue che blocca, `N/A` con l'owner. La regola di PIA lo permette già (§8 di
`roadmap-pia.md`); ciò che manca è che qualcuno la applichi. `#2621` raccoglie, `#85` attesta sul
candidate v2, e dichiara le **known limitations**: le `P1` differite da `D2` ne fanno parte per
nome.

### Corsia parallela — non sul percorso critico, non a motore

`PIA-2.6` (`#2456`), `PIA-2.9` (`#2457`), `PIA-2.10` (`#2505`) e la leaf `#2622` sono lavoro di
presentazione che **tocca `Source/`**: o entra nell'Onda 2, o è `NOT RUN — differito` in Onda 5.
Non c'è una terza via, ed è `D2` a deciderlo. Le dodici caselle della §5 del DoD aspettano
un'issue che le riscriva falsificabili: non blocca `#85`, e va aperta perché `D-449` lo dice.

---

## 5. Cosa la sequenza del 3 ottobre aveva giusto

Il panel conferma, perché un referto che elenca solo i difetti lascia credere che vada riscritto
tutto:

- **il perimetro** — `G1`–`G14` più `G16`, non un intervallo — e la lettura di `D-377` per cui la
  milestone non decide la consegnabilità;
- **il metodo di §2.1**: il controllo sul runtime con un controllo positivo del filtro. Il difetto
  non è il comando, è che nessuno lo esegue (§2.1 di questo referto);
- **le tre trappole di `G13`** (`RecordPacing` da `L_Frontend`, RMB e non `BackSpace`, la cartella
  dei crash preesistente) e il **cancello** della §10.2: restano vere sul candidate v2;
- **`#3073` come misura prima che come correzione**, e il verso dell'asserto;
- **la conferma per `MatchId`** del passo 5 di `G16`;
- **§9.3**: la rimisura di `G11` ha escluso una regressione, e `#3413` resta il proprietario del
  divario fra la banda scritta e il numero misurato;
- **la dichiarazione delle ipotesi cadute** (§6 di quel documento): è il metodo che questo referto
  ha cercato di imitare.

---

## 6. Limiti dichiarati

1. **Nessuna misura con il motore.** Tutti gli stati di gate citati sono celle del DoD con la loro
   data; la deriva del runtime è misurata con `git`, non con una build. `NOT RUN` non è `PASS`.
2. **Il contenuto delle quarantuno PR non è stato riletto.** Il referto ne cita i titoli e i
   numeri di decisione; che tutte appartengano alla v0.1 è assunto dalle sedute che le hanno
   convocate, non verificato PR per PR.
3. **`#2964` non è stata misurata**: il sospetto di §2.5 nasce dal confronto fra il suo corpo e il
   log del giro meccanico, che sono due contesti diversi (presidiata, autobattle).
4. **Le `P1` di §2.3 sono elencate per label, non per merito.** Il panel non ha letto i loro corpi
   uno per uno: `D2` è una decisione proprio perché quel lavoro resta da fare.
5. **Il marcatore di `U16` non è stato riverificato** (§2.9): la riga riporta il rilievo del 3
   ottobre.
6. **Il referto non ha avuto un avvocato del diavolo prima della pubblicazione.** La sequenza del 3 ottobre ne
   aveva uno per fronte; qui due affermazioni sono cadute **dopo**, rileggendo le fonti per eseguire l'Onda 0, e
   stanno in §7.

---

## 7. Due affermazioni della prima stesura, cadute il giorno stesso

Trovate eseguendo l'Onda 0 — cioè aprendo i file che il referto citava, invece di citarli dalla memoria della
lettura precedente. Restano scritte perché il metodo è lo stesso che il referto rimprovera alla sequenza.

| Affermazione | Perché era falsa | Come è stata trovata |
|---|---|---|
| 🔴 *«I runbook di `G13` e `G16` non verificano che il runtime di `HEAD` sia il candidate»* (§2.1) | Il preflight di `G16` **ha** il blocco `rev-parse`. Gli manca l'esito di arresto: su `DIVERSI` prescriveva il checkout del candidate, che oggi è il verso sbagliato | aprendo il runbook per inserirvi il passo 0, e trovandolo già lì |
| 🔴 *«La cella `G4` cita ancora `Combat.GuardPoolIsPermutationInvariant`, e `#3203` lo corregge»* (§1.5, §2.8, §4) | Il `grep -c` → `1` era vero e la conclusione no: la cella lo cita **come nome storico dichiarato**, con `D-408` e la data del rename, e si era già corretta il 2026-10-03. `#3203` rinomina un altro simbolo | leggendo la cella **per intero** invece dei 260 caratteri attorno al match |

🔑 **Il denominatore comune è uno solo**: un `grep` che colpisce non dice *in che ruolo* la stringa compare. Una
cella del DoD può nominare un test proprio per dire che **non si chiama più così**.
