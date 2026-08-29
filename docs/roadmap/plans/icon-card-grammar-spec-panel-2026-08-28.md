# Skill Card Grammar radiale (handoff Icon Grammar Consolidation) — spec panel

> `CURRENT` · **Stato**: revisione chiusa · **Data**: 2026-08-28
> **HEAD della revisione**: `c1a7cd9d` · branch `feat/220-slot-consuma-catalogo` · `origin/main` `0f3f8882`
> ⚠️ **Il branch di lavoro è 18 commit DIETRO `origin/main`** (misurato: `git rev-list --count HEAD..origin/main`
> → 18, e `HEAD` è antenato di `origin/main`). Le misure qui sotto sono state prese contro `origin/main`
> aggiornato dopo `git fetch --prune`, non contro il branch stantio.
> **Sorgente revisionata**: `CLAUDE_RT_IconGrammar_Consolidation_Handoff_2026-08-28.md` (727 righe), arrivato
> untracked a radice; archiviato a fine sessione in
> [`../../archive/src/handoff/2026-08-28-icon-grammar-consolidation.md`](../../archive/src/handoff/2026-08-28-icon-grammar-consolidation.md).
> **Regola applicata**: un handoff AI è l'ultima fonte della gerarchia ([`CLAUDE.md`](../../../CLAUDE.md) §7).
> Dove contraddice un ADR, una `D-nnn`, un gate o un fatto misurabile, prevale il repository. È anche ciò che
> l'handoff stesso prescrive al §18: *«se trovi una contraddizione: misurala; cita i due owner in conflitto;
> registra l'open point o proponi una decisione; non risolverla silenziosamente»*.

---

## 1. Il verdetto in una riga

L'handoff è **corretto su ciò che sa e cieco su ciò che è successo nelle 48 ore precedenti**: la separazione
grammatica/catalogo è giusta e va conservata, ma tre delle sue prescrizioni sono già state consumate,
implementate o smentite dal repository — e una di esse, applicata alla lettera, **rovescerebbe una specifica
già in codice con i suoi gate attivi**, senza che il rovesciamento sia stato né misurato né discusso.

Il difetto non è di scrittura. È che l'handoff **elenca nove file di `visual-language/` quando ne esistono
quindici**, e i due che non nomina — `09-alfabeto-fase-e-conseguenza.md` e `10-catalogo-sette-categorie.md`,
entrambi del 2026-08-26 — sono esattamente quelli che decidono la materia di cui parla.

**Ciò che resta genuinamente nuovo e merita una decisione** è la **grammatica dei satelliti**: sei forme
tipizzate, i loro caps e il loro ancoraggio geometrico. Quella parte non esiste altrove nel repository, ed è
la sola per cui vale una `D-nnn`.

---

## 2. Ciò che è stato misurato

| Affermazione dell'handoff | Esito | Misura |
|---|---|---|
| §3.3 «esagono **flat-top**» | 🔴 **falso, e lo smentisce il suo stesso asset** | il gioco è **pointy-top** in 17 punti di `Source/RefactorTactics/Map/` (`RTCellId.h:7,22`, `RTHexLibrary.cpp:379` «primo vertice a −30 gradi») e il test `RefactorTactics.Hex.CellCornersFormPointyTopHexagon` lo verifica. Lo SVG master consegnato è **pointy-top**: misurato h/w = **1.1547** (= 2/√3), vertici a 30°/90°/150°/210°/270°/330° |
| §2.2 `Dodge` non è una macro-fase, `Dash` è la fase | ✅ **vero, e già deciso** | è **D-230**. Era in volo sulla PR **#1532** durante la revisione; **mergiata su `main` prima della fine della sessione**. Non registrato due volte |
| §4 «il conflitto sul colore è fra Card Grammar e `02-color-system.md`» | 🔴 **incompleto: i contendenti sono tre** | `09-alfabeto-fase-e-conseguenza.md` riga 254 **adotta** la palette per famiglia di `02` e assegna la fase a un **binario geometrico**, non al colore. La sua Fase 1 è **implementata** in `tools/hud-assets/generate_hud_assets.py` con i gate T1/T3/T5/T6/T7 |
| §1 elenco fonti di `visual-language/` | 🔴 **9 su 15** | mancano `09-alfabeto-fase-e-conseguenza.md`, `10-catalogo-sette-categorie.md`, `CLAUDE_CLI_RT_VisualLanguage_Roadmap.md`, `RT_VisualLanguage_Epics_Issues.md`/`.yaml`, `visual-language-skill.pdf` |
| §1 elenco owner in `docs/technical/` | 🟡 **incompleto** | non nomina `docs/technical/runbooks/guida-catalogo-icone.md`, owner **vivo** della procedura di catalogo (61 chiavi, `FindMissingRequiredIcons` → 0) |
| §2.2 «macro-fasi vere: **sei**» | 🟡 **vero del round, falso del catalogo** | `ERTMatchPhase` ha **7** valori (con `MatchEnded`); `RequiredIconIds()` ne icona **4** e la variabile si chiama `VoluntaryPhases` (`RTIconLibrary.cpp:35-36`) |
| §3.3 reticolo a **30°**, 12 raggi | ✅ **vero, e già esiste in codice** | `FRTOccupancyMask` (`RTHexOccupancyLibrary.h:76-99`) è **dodici settori da 30 gradi**. ⚠️ ancoraggio **diverso**: il codice parte da **−30°**, l'handoff dalla linea orizzontale (**0°**) |
| §7.1 «rimosso lo script di prenotazione ID» | ✅ **vero** | ma `tools/decision-log/` **esiste** (vista HTML + cache GitHub): è un derivato, non un prenotatore |
| §7.1 «assegna il successivo ID libero» | 🟡 **il numero non è quello che si dedurrebbe** | all'inizio: massimo su `origin/main` = **D-229**, **D-230** rivendicato dalla PR aperta #1532 → primo libero **D-231**. ⚠️ `CLAUDE.md` §7 dichiarava «ultimo assegnato: **D-222**», **stantio di sette**: corretto in questa sessione sostituendo il numero col comando per misurarlo. Assegnate **D-231** e **D-232**, entrambe riverificate libere dopo il merge di #1532 |
| §13 asset da cercare | 🟡 **nessuno dei tre nomi esiste** | esistono `RefactorTactics_ShieldHex_Satellites_Master.svg` (4137 B), `icon-mockup.v0.1.png`, `icon-mockup.v0.2.png` — tutti **untracked** |
| §14 «non devono nascere categorie runtime» | ✅ **premessa sana** | `ERTIconCategory` ha **12** valori (D-031) e `10-catalogo-sette-categorie.md` conferma: le sette non popolate restano a **E25** |
| Le nove issue citate esistono | ✅ **9 su 9, tutte OPEN** | #217 #219 #220 #637 (v0.1, P2) · #265 #266 #267 #268 #269 (post-v0.1, P2) |

---

## 3. Il panel

### 📚 WIEGERS — qualità del requisito

> ❌ **CRITICO.** «Esagono flat-top» è un requisito **verificabile e falso**. Non è ambiguo né
> sotto-specificato: è misurabile in un comando, e la misura dice il contrario sia contro il codice sia
> contro l'asset allegato allo stesso handoff. Un requisito che il proprio allegato smentisce non è un
> requisito debole — è un errore di trascrizione che si propaga a chiunque disegni.
>
> **Raccomandazione**: la decisione dice **pointy-top** e cita il test che lo prova. Costo: una parola. Se
> restasse «flat-top», ogni card autorata sarebbe ruotata di 30° rispetto alla griglia su cui viene letta.

### 🔨 ADZIC — falsificabilità

> ⚠️ **MAGGIORE.** I tre esempi del §6 (Rail Shot, Chain Lightning, Overwatch) sono la parte migliore
> dell'handoff: rendono la grammatica falsificabile. Ma **nessuno dei tre è un'abilità del roster**, che è
> Gadget/Phase/Riktor/Wraith (D-120).
>
> **Raccomandazione**: conservare i tre come *didattici* — funzionano — e aggiungerne **uno reale**,
> `Hero.Wraith.InterceptShot`, che è già la thin slice Predictive dichiarata in `CLAUDE.md` §3. Un esempio
> che attraversa il roster vero è l'unico che può diventare un test percettivo in #269.

### 🏗️ FOWLER — confini

> ✅ **La tesi centrale è corretta e va difesa.** «Grammatica compositiva ≠ catalogo semantico runtime» è la
> stessa separazione che il codice già impone: `IconId` risolve un asset, non descrive come è disegnato. Il
> §9 sull'issue #637 — *«`Target`, `Shape`, `Delivery`, `HitRule`, `Effect` possono esistere nella grammatica
> senza richiedere una `ERTIconCategory`»* — è il contributo più utile del documento, perché chiude in
> anticipo la scorciatoia che avrebbe gonfiato l'enum.
>
> ⚠️ **Ma il reticolo apre un confine che l'handoff non vede.** Esistono già dodici settori da 30° in
> `FRTOccupancyMask`, ancorati a −30° «*perché senza, due implementazioni entrambe corrette producono
> maschere diverse dalla stessa geometria*». Un secondo reticolo a 12 raggi ancorato a 0° dà due spazi di
> indice che sembrano uno: stesso insieme di direzioni, numerazione sfasata di uno.
>
> **Raccomandazione**: riusare l'ancoraggio a **−30°**, oppure dichiarare a lettere che gli slot della card
> **non si numerano** — si nominano per posizione. Non lasciare la scelta implicita.

### 🎲 NYGARD — cosa si rompe

> ❌ **CRITICO, ed è il vero rischio operativo di questo consolidamento.** Il §4 chiede di riservare il
> colore alla fase. Ma `09-alfabeto-fase-e-conseguenza.md` ha **già** risolto lo stesso problema in senso
> opposto — fase = binario geometrico, colore = famiglia — e la sua Fase 1 **è in produzione**: il
> generatore la cuoce nel master e **cinque gate** (T1/T3/T5/T6/T7) fanno uscire il processo con 1 se manca.
> D-230 stesso vi si appoggia: *«`Action.Dodge` prende la sua stazione sul binario di fase; il gate T7 la
> pretendeva già»*.
>
> Registrare «colore = fase» oggi rovescerebbe una specifica implementata, **e i gate resterebbero verdi**,
> perché controllano il binario, non la tinta. Il fallimento sarebbe silenzioso.
>
> **Raccomandazione**: il canale colore è un **open point**, non una decisione da recepire. Va scritto come
> tale, coi tre owner nominati.

### 🧭 COCKBURN — chi è l'attore

> ⚠️ La Skill Card serve **due** attori con bisogni diversi, e l'handoff non li separa: chi **autora**
> (vuole reticolo, caps, anchor) e chi **legge in partita** (vuole la gerarchia CORE > PHASE > proprietà).
> Il §3.3 lo sfiora — *«il reticolo è una regola di authoring, non una texture da mostrare»* — ma poi tratta
> caps e gerarchia come se fossero lo stesso vincolo.
>
> **Raccomandazione**: nel documento owner, due sezioni intestate ai due attori. I caps sono un vincolo
> d'**authoring**; la gerarchia di lettura è un vincolo di **resa**. Confonderli produce revisioni che
> «correggono» una card leggibile perché viola un cap che serviva a un altro scopo.

### 🧪 CRISPIN — come si valida

> ⚠️ Il §12 apre otto open point e il §15 chiude quindici caselle di DoD, ma **nessuna delle due liste è un
> comando**. «Forme satellite + caps registrati» si spunta leggendo; non c'è modo di sapere domani se una
> card in produzione viola un cap.
>
> ✅ Il §11 di #269 è invece ottimo: grayscale, 24 px, card densa al cap, `Electric` vs `Reaction`. Quelli
> sono test veri, e il repository ha già dove metterli — il generatore ha gate numerati che escono con 1.
>
> **Raccomandazione**: **non** costruire un validator adesso (non ha consumatori: nessuna card esiste). Ma
> scrivere il DoD di #269 come **soglia misurabile** invece che come casella, così che il primo che autora
> una card trovi il criterio già scritto.

---

## 4. Sintesi — cosa consolidare, cosa no

| Materia dell'handoff | Destino | Perché |
|---|---|---|
| Grammatica satelliti: 6 forme, significati, caps | ✅ **decisione nuova** | non esiste altrove; è il solo contenuto originale |
| Reticolo a 30°, anchor sui vertici | ✅ **decisione**, con l'ancoraggio del codice | evita il secondo spazio di indici |
| Esagono **pointy-top** | ✅ **decisione**, corretta rispetto all'handoff | codice e asset concordano; l'handoff no |
| Grammatica ≠ catalogo runtime | ✅ **decisione** + nota su #637 | chiude la scorciatoia sull'enum |
| `Dodge`/`Dash`, Reaction non è fase | ⛔ **non registrare** | è **D-230**, già in volo su #1532 |
| Colore = fase | ✅ **deciso dall'autore in sessione**: **D-232** | ⚠️ misurando, non rovescia `09` come si temeva: il suo binario porta la fase di **risoluzione**, il colore la macro-fase del **round**. Di `09` cade la sola riga 254. Cade invece `02-color-system.md` §2, e **119** icone perdono il token di famiglia |
| Sei marker di fase | 🟡 **ridotto a quattro** | `RequiredIconIds()` icona le sole `VoluntaryPhases` |
| `Object` nel Target set | 🟡 **resta open point** | l'handoff stesso lo chiede, ed è corretto |
| Nuova epic | ⛔ **no** | E20/E25 esistono e hanno i checkpoint giusti |
| Modifiche a `ERTIconCategory` | ⛔ **no** | D-031 regge; le sette vuote sono di E25 |

---

## 5. Il difetto strutturale, e come non ripagarlo

Questo è il **terzo** kit di consolidamento archiviato il 2026-08-28. Tutti e tre condividono lo stesso
difetto: **sono fotografie datate che non dichiarano di esserlo e non portano il comando per riaggiornarsi**.

Qui il costo è stato concreto. L'elenco fonti del §1 è stato scritto quando `visual-language/` aveva nove
file. Ne ha quindici, e i due che decidono la materia sono fuori dall'elenco. Chi avesse eseguito l'handoff
alla lettera — leggere le fonti elencate, poi consolidare — **non avrebbe mai aperto `09`**, e avrebbe
rovesciato una spec implementata credendo di registrare una decisione nuova.

Il rimedio non è scrivere elenchi più lunghi: è **non fidarsi di un elenco scritto a mano**. Prima di
trattare una lista di fonti come completa, `ls` la cartella. È la stessa regola che `CLAUDE.md` §4 dichiara
per i gate — *«un elenco scritto a mano non si accorge di un file nuovo»* — e vale identica per le fonti.
