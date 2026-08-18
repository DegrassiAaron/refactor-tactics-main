# Roadmap — shortlist degli step

> `GENERATA` · il blocco qui sotto lo riscrive `python scripts/feature_registry.py shortlist`.
> **Cosa è**: l'elenco corto degli step, uno per riga, per orientarsi senza aprire 1344 righe.
> **Cosa non è**: una fonte di stato. Lo **stato delle epic** è letto da
> [`roadmap-v0.1.md`](roadmap-v0.1.md) §2.1, i **gate** dal Feature Registry: qui nessun simbolo è
> scritto a mano, quindi non può divergere. La colonna *In una riga* è l'unica scritta da una persona,
> e il generatore la conserva.

Legenda: ✅ chiusa · 🟡 parziale · ⏳ non iniziata · `—` nessuno stato dichiarato nell'owner

---

## 1. Le epic della v0.1

<!-- RT_SHORTLIST_EPICS:BEGIN -->

| Epic | Stato | Feature | Gate | In una riga |
|---|:--:|--:|--:|---|
| **E1** | ✅ | 4 | 18/23 | ID stabili e Primary Data Asset: senza, ogni azione diventa codice hard-coded |
| **E2** | ✅ | 6 | 43/49 | L'intero turno gira su esagoni — non si costruiscono 4 eroi sopra la griglia quadrata |
| **E3** | ✅ | — | — | `FRTGridCoord`, `URTGridLibrary` e `ARTGridActor` non esistono più: doppia manutenzione = ambiguità su dove va scritta una regola |
| **E4** | ✅ | 5 | 32/45 | Priorità intera intra-fase, fallback, collisioni senza bias di Player ID — regge azioni, reazioni, ambiente e obiettivi |
| **E5** | ✅ | 2 | 12/17 | Counter/Deflect/Brace/Shield/Cleanse/Intercept preparate in planning, una attivazione per turno, **nessuna attesa nel resolver** |
| **E6** | ✅ | 1 | 6/8 | Gadget, Phase, Riktor, Wraith da dati; **3 reazioni su 4** cablate, resta `FlowReaction`→E14; `InterceptShot` è Predictive e consegnata (D-016) |
| **E7** | 🟡 | 1 | 1/8 | Scelta orizzontale: ogni variante ha uno svantaggio |
| **E8** | ✅ | 8 | 50/72 | 8 superfici, stati temporanei legati alla cella, propagazione elettrica sul grafo dell'acqua, fuoco/acqua dinamici |
| **E9** | ✅ | 5 | 33/43 | Copertura per **bordo**, porte come bordo (mappa v4), ponti come **arco** (v5), pannello cinetico temporaneo |
| **E10** | 🟡 | 2 | 9/18 | Fine partita a tre vie ✅; ⏳ **nessun oggetto da attivare** in mappa |
| **E11** | 🟡 | 10 | 31/71 | Preview, input e playback esistono; ⏳ **Ghost Timeline** e i comandi `rt.Debug.*` completi |
| **E12** | 🟡 | 7 | 32/49 | Replay deterministico su 100 ripetizioni ✅; ⏳ **packaged build**. Senza checksum e packaged non è v0.1 |
| **E13** | 🟡 | 5 | 18/43 | La vista **decide**: da CP 13.2 un'azione offensiva non parte contro un bersaglio ignoto alla squadra, e un contatto solo `Incerto` si colpisce per cella. il **rumore** si propaga sul grafo (CP 13.3) ma nessuno lo **emette** ancora in partita — il produttore è CP 13.4. ⏳ **bot e HUD**, che la conoscenza non la consumano |
| **E14** | 🟡 | 8 | 21/78 | CP 14.3/14.4/14.5 chiusi: l'opportunity ha un'**identità derivata**, l'Overwatch la produce a ogni micro-step riusando `FRTSuppressiveZone`, e il resolver **apre la finestra dentro il calcolo** — `FIRE` tronca il movimento residuo, la decisione entra nel TurnLog v8. ⏳ **nessuno può ancora rispondere**: manca la UI (CP 14.6), poi Clash e Time Bank |
| **E15** | 🟡 | 2 | 9/16 | La prova integrata: fixture, scenario e golden replay a hash stabile. **Consuma** i sistemi, non li anticipa |
| **E16** | ✅ | 1 | 6/10 | Il facing è **stato di gioco**: deriva da Move e Dash, entra in snapshot/TurnLog/hash, e da dietro annulla copertura e `Guard` |
| **E17** | 🟡 | 1 | 1/7 | **Misura, non produzione**: CP 17.1/17.2 chiusi — il 4v4 gioca e non diverge, resolver **2,319 ms/turno**, e l'`if (Num == 2)` che l'epic cercava **non esiste**. ⏳ CP 17.3 è PIE. **Non** è un gate di release |
| **E18** | ✅ | 1 | 7/9 | **Una sola** azione predittiva rende percepibile il pilastro. Non dipende da E13/E14 |
| **E19** | 🟡 | 1 | 5/8 | Due buchi misurati: la mappa non dichiara la propria **classe**, il formato non dichiara le **unità per squadra** |
| **E20** | 🟡 | 1 | 1/7 | Le icone come **catalogo semantico**, non texture nei widget: va fatto *mentre* E11 costruisce l'HUD |
| **E21** | 🟡 | 2 | 1/12 | Il gioco smette di essere cilindri colorati — mesh, animazioni, anelli team/selezione |
| **E46** | 🟡 | 5 | 5/27 | — |
| **E47** | — | 2 | 3/15 | — |

**23 epic** · stato da [`roadmap-v0.1.md`](roadmap-v0.1.md) §2.1 · gate dal Feature Registry.

> ⚠️ **Senza stato dichiarato nell'owner**: **E47**. §2.1 non le copre — non è una svista di questa vista, è una riga che manca là.

<!-- RT_SHORTLIST_EPICS:END -->

---

## 2. L'ordine di lavoro

La *sequenza consigliata* di [`roadmap-v0.1.md`](roadmap-v0.1.md) §3 — quella che rispetta le dipendenze.
**Nessuna data**: il progetto è a dev singolo e non ha velocity misurata. Nessuno stato qui: sta nella
tabella sopra, che è generata.

| # | Fase | Epic |
|--:|---|---|
| 1 | Fondamenta | E1 cataloghi e modello dati · E2 parità hex del substrato |
| 2 | Un solo substrato | E3 dismissione del quadrato |
| 3 | Ossatura | E4 motore delle azioni a priorità |
| 4 | Contenuto | E6 roster 4 eroi · E5 reazioni · E8 terreni e ambiente |
| 5 | Prima prova integrata | E15 · CP 15.1–15.2 (scenario e fixture Lite) |
| 6 | Mondo reattivo | E9 coperture e strutture · E7 equipaggiamento · E10 obiettivi dinamici |
| 7 | Leggibilità | E11 HUD, log e debug · E20 icone · E21 presentazione |
| 8 | Percezione | E16 orientamento *(prerequisito)* · E13 vista e udito |
| 9 | Interazione | E14 overwatch e finestre di reazione · E18 predictive thin slice |
| 10 | Prova finale | E15 · CP 15.3–15.5 (golden replay degli 8 turni) · E17 stress 4v4 · E19 formato |
| 11 | Release | E12 determinismo, QA, packaging |

**Totale: 23 epic, 112 checkpoint** *(rimisurato il 2026-08-16 sull'albero unito con **E46** ed **E47**;
⚠️ questa riga è **fuori** dai marker `RT_SHORTLIST_EPICS` — è prosa a mano dentro un file generato,
quindi nessun `--check` la confronta con la sorgente, ed è andata fuori sincrono **due volte in un
giorno**: [#962](https://github.com/DegrassiAaron/refactor-tactics-main/issues/962))*. Il collo di bottiglia dichiarato non è il codice di gioco: è la
**verifica interattiva** — il playtest E2.8/M6.8 non è mai stato eseguito.

---

## 3. Gli step oltre la v0.1

> `GENERATA` dal 2026-08-13 · il blocco qui sotto lo riscrive `python scripts/feature_registry.py shortlist`.
> Prima era ricopiata a mano e aveva perso **E36**, **E38** ed **E39** — tre epic che la tabella «Le
> release» di [`roadmap-post-v0.1.md`](roadmap-post-v0.1.md) elenca in v0.2 — e mostrava **E25** senza
> issue benché `#265` esista. Release, titolo, priorità e issue vengono da lì; il conteggio Feature dal
> Feature Registry. Nessuna si apre prima che **i 15 gate della v0.1 siano verdi**.

<!-- RT_SHORTLIST_RELEASES:BEGIN -->

| Step | Rel. | Issue | Feature | In una riga |
|---|:--:|---|--:|---|
| **E22** Cover Window: OPEN → FIRE → SEAL · P1 | v0.2 | `#323` | — | La copertura diventa una finestra temporale, non uno stato fisso |
| **E23** Muri, porte e interaction graph · P1 | v0.2 | `#324` | 5 | Il grafo di interazione generalizza il bordo commutabile di E9 |
| **E24** Formato Standard 3v3 · P1 | v0.2 | `#325` | — | La baseline competitiva dichiarata (4v4 resta stress test) |
| **E25** Icon Language completo · P2 | v0.2 | `#265` | — | Estensione di E20 all'intero HUD |
| **E26** Tactical Bot v1 · P1 | v0.2 | `#326` | 1 | Il bot usa conoscenza parziale e reazioni |
| **E35** Roster 8: Sentinel Directorate e Resonance · P0 | v0.2 | `#322` | 2 | Steel, Aurora, Murdock, Kwang: da 4 a 8 eroi |
| **E36** Framework degli status: capability, primitive e severity · P2 | v0.2 | `#435` | 1 | — |
| **E38** Economia del turno, accoppiamento col movimento e validazione del piano · P2 | v0.2 | `#609` | 3 | — |
| **E39** Spatial Transfer — teleport, blink e movimento istantaneo · P3 | v0.2 | `#704` | 1 | — |
| **E27** Percezione completa: vista, udito, memoria · P1 | v0.3 | `#327` | 1 | Oltre E13: identificazione, firma, sensori |
| **E28** Expert Bot v2 · P2 | v0.3 | `#328` | 1 | Bot che pianifica contro l'incertezza |
| **E29** Predictive avanzato · P2 | v0.3 | `#329` | — | Oltre la thin slice di E18: il framework completo |
| **E33** Conditional Intent · P2 | v0.3 | `#330` | 1 | Un intento con una biforcazione dichiarata in planning |
| **E30** Classe di mappa Operations · P2 | v0.4 | `#331` | — | La terza classe di mappa, fuori scope in v0.1 |
| **E31** Obiettivi multipli e logistica · P3 | v0.4 | `#332` | — | Più obiettivi contemporanei con dipendenze |
| **E32** Formato 4v4 competitivo · P3 | v0.4 | `#333` | — | Se E17 dice che regge |
| **E34** Stati del personaggio e trasformazioni · P3 | v0.4 | `#244` | 1 | Stance e trasformazioni — le 10 voci `PIE-STATE-*` sono la sua controparte umana. ⚠️ **Due Feature ID si sovrappongono e nessuno è stato scelto**: `RT-FEAT-CHARACTER-STATE` (`future`, `SPECIFIED`, 5 scenari planned) e `RT-FEAT-CHAR-TRANSFORMATION` (`v0.2`, `IDEA`). Finché l'audit non dice se sono due scope o un duplicato, **nessuna delle due si sposta**: cambiare release a un ID stabile per far quadrare un'epic è la migrazione che poi nessuno sa più motivare |
| **E37** Radar di personaggio e generatore Wiki · P3 | v0.4 | `#555` | 3 | — |
| **E40** Il turno simultaneo in rete · P0 | v0.5 | `#773` | 1 | — |
| **E41** GAS come runtime delle abilità, mai come autorità · P1 | v0.6 | `#774` | 1 | — |
| **E42** Dedicated server e loop online reale · P0 | v0.7 | `#775` | 1 | — |
| **E43** Misura a lotti, e il bot che sa cosa sta misurando · P1 | v0.8 | `#776` | 1 | — |
| **E44** Feature freeze, e ciò che regge · P0 | v0.9 | `#777` | — | — |
| **E45** Un gate di produzione, non una release di feature · P0 | v1.0 | `#778` | — | — |

**24 epic** su 9 release (v0.2 · v0.3 · v0.4 · v0.5 · v0.6 · v0.7 · v0.8 · v0.9 · v1.0) · release, titolo e issue da [`roadmap-post-v0.1.md`](roadmap-post-v0.1.md) · Feature dal Feature Registry.

> ⚠️ **Epic non ancora aperte**: 7 feature post-v0.1 (v0.2 **5** · v0.3 **2**) hanno `roadmap.epic` nullo; **20** la dichiarano. Un campo nullo qui non e' piu' un limite dello schema — dal 2026-08-13 le epic post-v0.1 sono scrivibili — quindi significa **l'epic non e' ancora aperta**, e la motivazione va nelle `notes` della feature. Le contraddizioni fra release della feature e release della sua epic sono diagnosticate dalla §2.2 di [`roadmap-v0.1.md`](roadmap-v0.1.md), che ha una tabella apposta.

<!-- RT_SHORTLIST_RELEASES:END -->

---

## 4. Dove guardare

| Domanda | Documento |
|---|---|
| *A che punto è questa epic?* | [`roadmap-v0.1.md`](roadmap-v0.1.md) §2.1 — misurata sui nomi dei test |
| *A che punto è questa milestone?* | [`roadmap-checkpoint.md`](roadmap-checkpoint.md) · [`milestonemap.shortlist.md`](milestonemap.shortlist.md) |
| *Questa feature esiste?* | [`feature-registry.yaml`](feature-registry.yaml) · [`featuremap.shortlist.md`](featuremap.shortlist.md) |
| *Chi verifica questa regola?* | [`../technical/scenario-map.md`](../technical/scenario-map.md) · [`scenariomap.shortlist.md`](scenariomap.shortlist.md) |
| *Quando la v0.1 è consegnabile?* | [`v0.1-definition-of-done.md`](v0.1-definition-of-done.md) — gate `G1`–`G15` |
| *Perché è stato deciso così?* | [`../decisions/RT_PDR_00_Decision_Log.md`](../decisions/RT_PDR_00_Decision_Log.md) + ADR |
