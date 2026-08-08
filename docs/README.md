# Documentazione RefactorTactics

**RefactorTactics** è un gioco tattico PvP a **turni simultanei** ispirato ad *Atlas Reactor*, su Unreal
Engine 5.8.1, sviluppato da un dev singolo. Ogni turno: **pianificazione simultanea e privata** → risoluzione a
macro-fasi **`Prep → Dash → Blast → Move → Cleanup`**, calcolate simultaneamente e applicate in ordine
deterministico, su una griglia **esagonale multilivello**.

> **Questa cartella è la fonte di verità del progetto.** I PDF sono snapshot di consultazione; il Markdown
> versionato prevale sempre. Se due documenti si contraddicono, vince quello più in alto nella gerarchia
> qui sotto — e la contraddizione va registrata, non risolta in silenzio.

---

## Gerarchia delle fonti

| # | Livello | Documento | Cosa decide |
|---|---|---|---|
| 1 | **Canone** | [`product/piano-canonico-mvp.md`](product/piano-canonico-mvp.md) | Invarianti, decisioni operative, regole. **Prevale su tutto** |
| 2 | **Decisioni** | [`decisions/`](decisions/) — 6 ADR + [Decision Log](decisions/RT_PDR_00_Decision_Log.md) | Scelte architetturali e di prodotto, con motivazione |
| 3 | **Esecuzione** | [`roadmap/roadmap-checkpoint.md`](roadmap/roadmap-checkpoint.md) | Milestone M6–M11, DoD misurabili, **stato** |
| 4 | **Release** | [`roadmap/roadmap-v0.1.md`](roadmap/roadmap-v0.1.md) | Scope della v0.1: 18 epic, 87 checkpoint |
| 5 | **Numeri** | [`balance/`](balance/) | Valori vigenti: azioni, eroi, terreni, equipaggiamento |
| 6 | **Specifiche** | [`gameplay/`](gameplay/) · [`technical/`](technical/) | Dettaglio per feature |
| 7 | **Requisiti lungo periodo** | [`roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md`](roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md) | Fasi F0–F6 — **direzione, non scope** |
| 8 | **Visione north-star** | [`src/`](src/) (PDF dei PRD) | Prodotto a lungo termine, **non** obiettivo attuale |
| 9 | **Storico** | [`archive/`](archive/) | Materiale superato, conservato per provenienza |

> **Il canone e gli ADR non possono divergere.** La tabella dice che il livello 1 prevale sul livello 2, ma un
> ADR accettato *corregge* il canone: ADR-0004 ha cambiato la forma della risoluzione, ADR-0005 ha spostato il
> facing fra gli stati di gioco. Senza una regola, «canone > ADR» diventa un paradosso di governance.
> **La regola è: un ADR accettato viene recepito nel canone nello stesso commit che lo accetta.** Non esiste
> uno stato intermedio «canone più emendamenti da assorbire» che il lettore debba ricostruire a mente. Se un
> ADR risulta `Accettato` ma non ancora recepito, quello è un difetto: si registra in
> [`DOC_CONFLICT_MATRIX.md`](DOC_CONFLICT_MATRIX.md) e si chiude, non si lascia implicito.

**Decisioni aperte**: [`OPEN_DECISIONS.md`](OPEN_DECISIONS.md) · **conflitti fra documenti**:
[`DOC_CONFLICT_MATRIX.md`](DOC_CONFLICT_MATRIX.md) · **storia della documentazione**:
[`CHANGELOG_DOCUMENTATION.md`](CHANGELOG_DOCUMENTATION.md).

---

## Owner del concetto

**Un concetto, un documento.** Chi possiede la regola la scrive; gli altri linkano. Se trovi la stessa regola
definita in due posti, è un difetto: apri una issue invece di aggiornarne una sola.

| Concetto | Owner | Stato |
|---|---|---|
| Invarianti #1–#7 (autorità, determinismo, privacy) | [`product/piano-canonico-mvp.md`](product/piano-canonico-mvp.md) §5 | ✅ |
| Identità e pilastri di prodotto | [`product/piano-canonico-mvp.md`](product/piano-canonico-mvp.md) §2 · [`../README.md`](../README.md) | ✅ |
| Ordine delle macro-fasi | [`decisions/adr-0003-modello-azioni-v01.md`](decisions/adr-0003-modello-azioni-v01.md) | ✅ |
| **Sequenza canonica del round** e tassonomia temporale | [`gameplay/spec-sequenza-turno.md`](gameplay/spec-sequenza-turno.md) | ✅ **normativa** |
| Ordine deterministico delle azioni | [`gameplay/spec-sequenza-turno.md`](gameplay/spec-sequenza-turno.md) §3.1 | ✅ implementato |
| Ordine degli effetti simultanei (APNAP) | [`product/piano-canonico-mvp.md`](product/piano-canonico-mvp.md) §5.1 | ⚠️ **deciso, non implementato** |
| Azioni generiche e profili di `Move` | [`gameplay/brief-azioni-generiche-overwatch.md`](gameplay/brief-azioni-generiche-overwatch.md) · elenco vigente in [`balance/RT_ActionCatalog_v0.1.md`](balance/RT_ActionCatalog_v0.1.md) §1 | ✅ D-014/D-015 · **D-025**: sette generiche, `Guard` universale · ⏳ migrazione ID · ⏳ brief da allineare a D-025 |
| Predictive Action (thin slice v0.1) | [`gameplay/brief-delayed-actions.md`](gameplay/brief-delayed-actions.md) | ✅ D-016 · ⏳ da implementare |
| Coperture direzionali e `Intercept` | [`gameplay/spec-copertura-cp91.md`](gameplay/spec-copertura-cp91.md) | ✅ CP 9.1 · D-017 ⏳ |
| Motore azioni: priorità, fallback, collisioni | [`gameplay/spec-motore-azioni-e4.md`](gameplay/spec-motore-azioni-e4.md) | ✅ E4 |
| Reazioni componibili | [`gameplay/spec-reazioni-componibili-cp55.md`](gameplay/spec-reazioni-componibili-cp55.md) | ✅ E5 |
| Finestre di reazione e decision boundary | [`decisions/adr-0004-finestre-di-reazione.md`](decisions/adr-0004-finestre-di-reazione.md) | ✅ deciso, ⏳ E14 |
| Overwatch | [`gameplay/brief-overwatch-reazioni.md`](gameplay/brief-overwatch-reazioni.md) | ⏳ E14 |
| Delayed Actions e boundary nominati | [`gameplay/brief-delayed-actions.md`](gameplay/brief-delayed-actions.md) | ⏳ nessuna epic |
| Griglia esagonale e coordinate | [`decisions/adr-0002-griglia-esagonale.md`](decisions/adr-0002-griglia-esagonale.md) | ✅ |
| Orientamento e direzionalità | [`decisions/adr-0005-orientamento.md`](decisions/adr-0005-orientamento.md) | ⏳ E16 |
| Mappa come grafo tattico, multilivello | [`technical/spec-mappa-multilivello.md`](technical/spec-mappa-multilivello.md) · [`technical/spec-pathfinding-pf3-pf4.md`](technical/spec-pathfinding-pf3-pf4.md) | ✅ |
| Terreni, stati e propagazione | [`gameplay/spec-terreni-e8.md`](gameplay/spec-terreni-e8.md) + spec CP 8.2/8.3/8.4 | ✅ E8 |
| Conoscenza parziale: vista **e** udito | [`gameplay/brief-conoscenza-parziale.md`](gameplay/brief-conoscenza-parziale.md) | ⏳ E13 |
| Durata partita, round, scala mappe | [`gameplay/spec-durata-partita-e-scala-mappe.md`](gameplay/spec-durata-partita-e-scala-mappe.md) (D-010) | ✅ |
| Pacing del turno misurato | [`gameplay/spec-pacing-turno.md`](gameplay/spec-pacing-turno.md) | ✅ |
| Privacy dell'intento | invariante #6 + `URTIntentPrivacyLibrary` | ✅ offline |
| TurnLog, reason code, serializzazione | [`technical/spec-turnlog.md`](technical/spec-turnlog.md) · [`technical/spec-turnlog-serialize.md`](technical/spec-turnlog-serialize.md) | ✅ |
| HUD e leggibilità | [`technical/progettazione-hud.md`](technical/progettazione-hud.md) · [`technical/brief-planning-visuale.md`](technical/brief-planning-visuale.md) | ⏳ E11 |
| Mappa delle classi C++ | [`technical/architettura-codice.md`](technical/architettura-codice.md) | ✅ |
| Struttura di `Content/`, naming asset | [`technical/convenzioni-contenuti-ue.md`](technical/convenzioni-contenuti-ue.md) | ✅ **normativo** |
| Test automatici e scenari | [`technical/test-automatico-unreal.md`](technical/test-automatico-unreal.md) | ✅ harness consegnato · ⏳ assertion oltre il movimento |
| Verifiche interattive in editor | [`technical/test-manuali-pie.md`](technical/test-manuali-pie.md) | 🟡 |
| Scenario della showcase | [`product/showcase-v0.1.md`](product/showcase-v0.1.md) | ⏳ E15 |
| Gate di release | [`roadmap/v0.1-definition-of-done.md`](roadmap/v0.1-definition-of-done.md) | ⏳ |
| Unità ausiliarie | [`gameplay/brief-unita-ausiliarie.md`](gameplay/brief-unita-ausiliarie.md) | ✅ vincoli decisi · gameplay **fuori** dalla v0.1 |
| Azioni generiche e Overwatch universale | [`gameplay/brief-azioni-generiche-overwatch.md`](gameplay/brief-azioni-generiche-overwatch.md) | ✅ deciso ([D-012](decisions/RT_PDR_00_Decision_Log.md) · [D-025](decisions/RT_PDR_00_Decision_Log.md): **sette** voci, `Guard` universale) · ⏳ E14 |
| **Ownership di abilità, interazioni e sinergie** | [`gameplay/spec-ownership-abilita-interazioni-sinergie.md`](gameplay/spec-ownership-abilita-interazioni-sinergie.md) · [ADR-0006](decisions/adr-0006-ownership-abilita-sinergie.md) | ✅ **normativa** ([D-028](decisions/RT_PDR_00_Decision_Log.md)) · nessun runtime nuovo |
| Trigger su transizione (trap, tripwire) | [`gameplay/brief-delayed-actions.md`](gameplay/brief-delayed-actions.md) §6-bis | ✅ deciso ([D-013](decisions/RT_PDR_00_Decision_Log.md)) · nessuna epic |
| Formato principale di partita | [`decisions/RT_PDR_00_Decision_Log.md`](decisions/RT_PDR_00_Decision_Log.md) `D-001`/`D-011` | ⚠️ **assunzione**: 3v3 baseline, mai misurato |
| Validazione di stress 4v4 | [`roadmap/roadmap-v0.1.md`](roadmap/roadmap-v0.1.md) §E17 | ⏳ E17, dopo E15 |

---

## Le risposte brevi

**Qual è il gameplay loop?** Pianificazione simultanea e privata (30 s in 2v2) → lock-in → risoluzione a fasi
osservabile → cleanup. La partita finisce per **eliminazione**, **obiettivo** oppure `RoundLimit`, che è un
parametro del formato e non la costante «12».

**Come funziona il turno?** `Planning → Prep → Dash → Blast → Move → Cleanup`. Il **Move normale è sempre
l'ultima fase volontaria**: sta *dopo* il Blast. Dash, Charge e Leap sono spostamenti speciali della loro fase,
non un secondo Move. Le azioni dichiarano una fase e una **priorità intera** intra-fase.

**Come funziona il resolver?** «Raccogli poi applica»: snapshot all'inizio di ogni **segmento**, nessuna attesa
dentro il segmento, l'ordine dell'array non cambia l'esito. Dopo [ADR-0004](decisions/adr-0004-finestre-di-reazione.md)
il turno è una **sequenza** di segmenti, ciascuno completo: le finestre di reazione compongono l'invariante #3,
non lo derogano.

**Quali dati sono privati?** Le intenzioni di pianificazione. Mai replica globale con occultamento grafico:
stato server + `FilterForTeam → FRTIntentView` + autorizzazione lato server. Oggi è banale perché il gioco è
offline; diventa il gate `intent leak = 0` in **M10**.

**Qual è la mappa?** Un **grafo tattico**: celle esagonali (`FRTCellId{q, r, Layer}`, 6 vicini) più archi
espliciti fra layer. Le celle sono dati compatti in un `URTHexMapAsset` con hash stabile — **nessun Actor per
cella**. Porte, ponti e ascensori modificano una *transizione del grafo*, non solo una mesh.

**Come funzionano visibility e rumore?** Non ancora: la vista è una statistica a catalogo che non decide nulla.
Il modello deciso separa **LOS · rilevamento · conoscenza di squadra**, con tre livelli (`Nascosto`,
`ContattoIncerto`, `Rilevato`) più `UltimoContatto`. Il rumore è il **secondo canale** dello stesso modello,
propagato con flood fill intero sul grafo — mai `SphereOverlap`. Epic **E13**.

**Come funzionano reaction e Overwatch?** Le reazioni sono dichiarate in pianificazione, con **1 attivazione
per turno**, e oggi sono deterministiche e senza finestre. Il modello deciso è unico — `opportunity → commit` —
di cui quello attuale è il caso `AllowedResponses ≤ 1`. L'Overwatch apre una finestra di **3,0 s** con
`FIRE`/`HOLD`, `Timeout → HOLD`. Epic **E14**, dipende da E13.

**Qual è lo scope della v0.1?** Vertical slice **2v2 offline contro bot** su hex multilivello: 4 eroi
(Flux · Riva · Bastion · Vektor), ~35 azioni a catalogo, reazioni, 8 terreni attivi, coperture e strutture,
obiettivi dinamici, HUD leggibile, determinismo verificato, build packaged. **Fuori**: rete, 4v4, GAS,
progressione, modding.

**Quali decisioni sono definitive?** Quelle in [`decisions/`](decisions/) con stato `Consolidata` e gli
invarianti del canone. **Quali sono aperte?** [`OPEN_DECISIONS.md`](OPEN_DECISIONS.md).

**Cosa è solo storico?** Tutto in [`archive/`](archive/); i PDF in [`src/`](src/) e
[`archive/pdr-v0.1/`](archive/pdr-v0.1/); i piani consegnati in [`roadmap/plans/`](roadmap/plans/); e i singoli
documenti che portano in testa il banner **⚠️ Superato** o **📦 Piano consegnato**.

**Quali test verificano le regole?** Sotto `Source/RefactorTactics/Tests/`. Il numero **si misura, non si cita
a memoria** — questo file l'ha già sbagliato quattro volte:

```bash
grep -rhoE '"RefactorTactics\.[A-Za-z0-9_.]+"' Source/RefactorTactics/Tests/*.cpp | tr -d '"' | sort -u | wc -l
```

Ultima misura: **456 test unici in 68 file**, il 2026-08-08 dopo il merge di CP 9.3 (porte) e CP 9.4
(ponti). La ripartizione sotto è derivata dalla stessa misura e somma esattamente a 456.

| Area | Test | Cosa fissa |
|---|---:|---|
| `Hex*` (mappa, path, vision, bot, blast, move, match) | 88 | Coordinate, A\*, LOS, bot, partita completa |
| `Actions.*` | 58 | Ordine per priorità, permutazione-invarianza, fallback, mappatura di fase |
| `Terrain.*` · `Status.*` · `Environment.*` | 39 | Superfici, stati temporanei, propagazione elettrica, fuoco/acqua |
| `Combat.*` · `HexCombat.*` | 36 | Danno dopo scudo, forme, LOS, niente fuoco amico |
| `Reactions.*` | 27 | Attivazione singola, trigger puro, reazioni componibili, privacy |
| `HexSim.*` | 27 | Snapshot, budget, collisioni simultanee, **replay divergence 0** |
| `Match*` (allestimento, formato, fine partita) | 27 | Le tre vie di fine partita e il `RoundLimit` da formato |
| `Heroes.*` | 25 | I 4 eroi corrispondono al catalogo, trade-off delle varianti |
| `TurnLog.*` | 22 | Hash permutazione-invariante, serializzazione versionata, checksum |
| `Scenario.*` | 22 | Harness: PASS/FAIL/ERROR/**BLOCKED**, fixture per nome, niente bypass |
| `Structures.*` | 18 | Porte come bordo (CP 9.3), ponti come arco (CP 9.4) |
| `Playback.*` · `Preview.*` · `PlayerInput.*` · `ShowcaseRelay.*` | 19 | Presentazione e input: non decidono, riproducono |
| `Unit.*` · `Turn.*` · `Simulation.*` | 17 | Stato unità, **ciclo di vita dei piani**, determinismo del replay |
| `Cover.*` | 13 | Copertura bassa e alta, bordi, danno a struttura e distruzione |
| `Catalog.*` | 9 | Invarianti del catalogo: solo interi, slot dichiarati, ID stabili |
| `Pacing.*` | 7 | Pacing del turno misurato |
| `Perf.*` | 2 | Path mediana **0,025 ms** · resolver **0,41 ms/turno** |

---

## Struttura delle cartelle

```
docs/
├── README.md                    ← sei qui: indice e owner dei concetti
├── DOC_CONFLICT_MATRIX.md       conflitti fra documenti e loro stato
├── OPEN_DECISIONS.md            cosa aspetta una decisione umana
├── CHANGELOG_DOCUMENTATION.md   storia della documentazione
│
├── product/     visione, canone, vertical slice
├── gameplay/    regole di gioco: turno, azioni, reazioni, percezione, ambiente
├── technical/   architettura, mappa, pathfinding, TurnLog, UI, test, guide
│   └── img/     riferimenti visuali
├── balance/     numeri vigenti (cataloghi .md) + workbook di esplorazione
├── roadmap/     milestone, release v0.1, DoD, requisiti di lungo periodo
│   └── plans/   piani di esecuzione consegnati (storico)
├── decisions/   ADR e Decision Log
├── wiki/        guida al gioco per il giocatore (meccaniche, fazioni, sinergie) — divulgativa
│   ├── game/        loop, azioni, ambiente, sinergie e combinazioni
│   ├── meccaniche/  manuale per regola
│   └── fazioni/     identità, filosofia, roster, scenari dimostrativi
├── characters/  pagine personaggio: v0.1, v0.2, candidati Paragon — **un kit per pagina**
├── src/         sorgenti non normativi: PDF di visione e brief grezzi
└── archive/     materiale superato
    └── pdr-v0.1/  snapshot PDF dei PDR
```

### Due deviazioni dichiarate

1. **I nomi dei file restano in italiano kebab-case**, non `UPPER_SNAKE` inglese come nella struttura di
   riferimento. Il repository ha una convenzione consolidata e mescolarla peggiorerebbe la leggibilità;
   la clausola «riutilizzare i file esistenti quando possibile» lo consente.
2. **`src/` non è documentazione**: è la casella di posta dei sorgenti grezzi (PDF di visione, brief non ancora
   triagiati). Vive accanto ai documenti perché è da lì che nascono, ma **non è normativa** e non compare nella
   gerarchia delle fonti sopra il livello 8.

---

## Convenzioni

- **Documentazione in italiano**; codice e identificatori in inglese.
- Ogni documento dichiara in testa **stato**, **data** e **autorità** (a chi è subordinato).
- Quando una regola cambia, il documento che la possiede guadagna un blocco `⚠️ Revisione <data>` — il
  changelog sta **accanto alla regola**, non in un file separato.
- Un documento superato **non si cancella**: guadagna un banner in testa e resta come provenienza.
- Nessuna affermazione su codice, test o metriche senza **misura riproducibile** nel documento stesso.

### Come si classifica un documento

Ogni documento porta in testa **una** di queste etichette. Serve a rispondere alla sola domanda che conta
prima di leggerlo: *questa frase vale ancora?* — senza dover interpretare date, commit e correzioni.

| Tag | Significato | Si aggiorna? |
|---|---|---|
| `CANONICAL` | Decide: è la fonte che prevale in caso di conflitto | Sì — è il posto dove si cambia una regola |
| `CURRENT` | Descrive com'è il progetto **oggi**, subordinato al canone | Sì |
| `AS-BUILT` | Congela ciò che fu **consegnato** a un checkpoint | No — si emenda con un rimando, non si riscrive |
| `DELIVERED PLAN` | Piano di esecuzione già eseguito | No |
| `HISTORICAL` | Superato, conservato per provenienza | No |
| `RESEARCH` | Esplorazione, non dato vigente | Non è una fonte: non risolve conflitti |
| `OPEN` | Aspetta una decisione umana | Vive in [`OPEN_DECISIONS.md`](OPEN_DECISIONS.md) |

Un documento `AS-BUILT` o `HISTORICAL` che descrive un mondo scomparso **non è un difetto da correggere**:
riscriverlo falsificherebbe la storia. La correzione va nel documento `CURRENT` che possiede la regola; allo
storico basta un rimando in testa. Il difetto vero è l'opposto — uno storico *senza* etichetta, che si legge
come se fosse la specifica di oggi.

### Gate anti-deriva

```bash
python scripts/check-docs-symbols.py
```

Fallisce se un **inventario di classi** in un documento normativo cita un simbolo `URT*`/`ART*`/`FRT*`/`ERT*`
che non è **dichiarato** in `Source/`. Nasce dal difetto D1: il canone elencava quattro classi su dieci rimosse
dal codice, e restava leggibile e falso.

Il gate è deliberatamente **stretto**: controlla solo le righe di tabella la cui prima cella è un simbolo — la
forma in cui un documento afferma «questo esiste oggi». Non controlla la prosa, dove lo stesso simbolo può
comparire come storia («è stato rimosso») o come proposta («il DoD introduce…»): distinguerli lessicalmente
non è affidabile, e un gate che sbaglia viene disattivato al terzo falso positivo. Sono esentati i documenti
storici, i brief propositivi e le cartelle `archive/`, `src/`, `roadmap/plans/`.
