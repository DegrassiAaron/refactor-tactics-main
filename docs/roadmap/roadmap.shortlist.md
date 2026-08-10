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
| **E2** | ✅ | 6 | 40/46 | L'intero turno gira su esagoni — non si costruiscono 4 eroi sopra la griglia quadrata |
| **E3** | ✅ | — | — | `FRTGridCoord`, `URTGridLibrary` e `ARTGridActor` non esistono più: doppia manutenzione = ambiguità su dove va scritta una regola |
| **E4** | ✅ | 5 | 28/40 | Priorità intera intra-fase, fallback, collisioni senza bias di Player ID — regge azioni, reazioni, ambiente e obiettivi |
| **E5** | ✅ | 2 | 11/16 | Counter/Deflect/Brace/Shield/Cleanse/Intercept preparate in planning, una attivazione per turno, **nessuna attesa nel resolver** |
| **E6** | ✅ | 1 | 6/8 | Flux, Riva, Bastion, Vektor da dati; **3 reazioni su 5** cablate, `InterceptShot`→E18 e `FlowReaction`→E14 |
| **E7** | ⏳ | 1 | 1/8 | Scelta orizzontale: ogni variante ha uno svantaggio |
| **E8** | ✅ | 8 | 48/64 | 8 superfici, stati temporanei legati alla cella, propagazione elettrica sul grafo dell'acqua, fuoco/acqua dinamici |
| **E9** | ✅ | 5 | 30/40 | Copertura per **bordo**, porte come bordo (mappa v4), ponti come **arco** (v5), pannello cinetico temporaneo |
| **E10** | 🟡 | 2 | 9/16 | Fine partita a tre vie ✅; ⏳ **nessun oggetto da attivare** in mappa |
| **E11** | 🟡 | 8 | 28/56 | Preview, input e playback esistono; ⏳ **Ghost Timeline** e i comandi `rt.Debug.*` completi |
| **E12** | 🟡 | 7 | 30/47 | Replay deterministico su 100 ripetizioni ✅; ⏳ **packaged build**. Senza checksum e packaged non è v0.1 |
| **E13** | ⏳ | 4 | 8/36 | Oggi la vista è una statistica a catalogo che **non decide nulla**; il rumore è il secondo canale |
| **E14** | ⏳ | 8 | 9/70 | `Opportunity → Commit`, Fast Reaction 3,0 s, `Timeout → HOLD`, Clash, Time Bank. ADR-0004 accettato, **nessun codice** |
| **E15** | 🟡 | 2 | 9/16 | La prova integrata: fixture, scenario e golden replay a hash stabile. **Consuma** i sistemi, non li anticipa |
| **E16** | ✅ | 1 | 5/9 | Il facing è **stato di gioco**: deriva da Move e Dash, entra in snapshot/TurnLog/hash, e da dietro annulla copertura e `Guard` |
| **E17** | ⏳ | 1 | 1/7 | **Misura, non produzione**: dove si rompe il sistema con otto unità. Dopo E15; **non** è un gate di release |
| **E18** | ✅ | 1 | 6/8 | **Una sola** azione predittiva rende percepibile il pilastro. Non dipende da E13/E14 |
| **E19** | 🟡 | 1 | 5/8 | Due buchi misurati: la mappa non dichiara la propria **classe**, il formato non dichiara le **unità per squadra** |
| **E20** | 🟡 | 1 | 1/7 | Le icone come **catalogo semantico**, non texture nei widget: va fatto *mentre* E11 costruisce l'HUD |
| **E21** | 🟡 | 1 | 1/7 | Il gioco smette di essere cilindri colorati — mesh, animazioni, anelli team/selezione |

**21 epic** · stato da [`roadmap-v0.1.md`](roadmap-v0.1.md) §2.1 · gate dal Feature Registry.

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

**Totale: 21 epic, 95 checkpoint.** Il collo di bottiglia dichiarato non è il codice di gioco: è la
**verifica interattiva** — il playtest E2.8/M6.8 non è mai stato eseguito.

---

## 3. Gli step oltre la v0.1

Non generate: queste epic non hanno feature nel registry, e nessuna si apre prima che **i 15 gate della
v0.1 siano verdi** ([`roadmap-post-v0.1.md`](roadmap-post-v0.1.md)).

| Step | Rel. | Issue | In una riga |
|---|:--:|---|---|
| **E35** Roster 8 — Sentinel Directorate e Resonance | v0.2 | `#322` | Steel, Aurora, Murdock, Kwang: da 4 a 8 eroi |
| **E22** Cover Window: OPEN → FIRE → SEAL | v0.2 | `#323` | La copertura diventa una finestra temporale, non uno stato fisso |
| **E23** Muri, porte e interaction graph | v0.2 | `#324` | Il grafo di interazione generalizza il bordo commutabile di E9 |
| **E24** Formato Standard 3v3 | v0.2 | `#325` | La baseline competitiva dichiarata (4v4 resta stress test) |
| **E25** Icon Language completo | v0.2 | — | Estensione di E20 all'intero HUD |
| **E26** Tactical Bot v1 | v0.2 | `#326` | Il bot usa conoscenza parziale e reazioni |
| **E27** Percezione completa: vista, udito, memoria | v0.3 | `#327` | Oltre E13: identificazione, firma, sensori |
| **E28** Expert Bot v2 | v0.3 | `#328` | Bot che pianifica contro l'incertezza |
| **E29** Predictive avanzato | v0.3 | `#329` | Oltre la thin slice di E18: il framework completo |
| **E33** Conditional Intent | v0.3 | `#330` | Un intento con una biforcazione dichiarata in planning |
| **E30** Classe di mappa Operations | v0.4 | `#331` | La terza classe di mappa, fuori scope in v0.1 |
| **E31** Obiettivi multipli e logistica | v0.4 | `#332` | Più obiettivi contemporanei con dipendenze |
| **E32** Formato 4v4 competitivo | v0.4 | `#333` | Se E17 dice che regge |
| **E34** Stati del personaggio e trasformazioni | v0.4 | `#244` | Stance e trasformazioni — le 10 voci `PIE-STATE-*` sono la sua controparte umana |

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
