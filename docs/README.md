# Documentazione RefactorTactics

**RefactorTactics** è un gioco tattico PvP a **turni simultanei** ispirato ad *Atlas Reactor*, su Unreal
Engine 5.8.1, sviluppato da un dev singolo. Ogni turno: **pianificazione simultanea e privata** → risoluzione a
macro-fasi **`Prep → Dash → Blast → Move → Cleanup`**, calcolate simultaneamente e applicate in ordine
deterministico, su una griglia **esagonale multilivello**.

> **Questa cartella è la fonte di verità del progetto**, e dal **2026-08-12** è interamente in Markdown: i
> ventiquattro binari di prosa che c'erano — dieci PDF di PRD, tredici snapshot PDR e un `.docx` — sono
> diventati sei documenti versionati e diffabili ([D-009](decisions/RT_PDR_00_Decision_Log.md)). Se due
> documenti si contraddicono, vince quello più in alto nella gerarchia qui sotto — e la contraddizione va
> registrata, non risolta in silenzio.

---

## Gerarchia delle fonti

| # | Livello | Documento | Cosa decide |
|---|---|---|---|
| 1 | **Canone** | [`product/piano-canonico-mvp.md`](product/piano-canonico-mvp.md) | Invarianti, decisioni operative, regole. **Prevale su tutto** |
| 2 | **Decisioni** | [`decisions/`](decisions/) — 6 ADR + [Decision Log](decisions/RT_PDR_00_Decision_Log.md) | Scelte architetturali e di prodotto, con motivazione |
| 3 | **Esecuzione** | [`roadmap/roadmap-checkpoint.md`](roadmap/roadmap-checkpoint.md) | Milestone M6–M11, DoD misurabili, **stato** |
| 4 | **Release** | [`roadmap/roadmap-v0.1.md`](roadmap/roadmap-v0.1.md) | Scope della v0.1: 21 epic, 100 checkpoint |
| 5 | **Numeri** | [`balance/`](balance/) | Valori vigenti: azioni, eroi, terreni, equipaggiamento |
| 6 | **Specifiche** | [`gameplay/`](gameplay/) · [`technical/`](technical/) | Dettaglio per feature |
| 7 | **Requisiti lungo periodo** | [`roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md`](roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md) | Fasi F0–F6 — **direzione, non scope** |
| 8 | **Visione north-star** | [`research/prd/`](research/prd/) — i PRD, in Markdown dal 2026-08-12 | Prodotto a lungo termine, **non** obiettivo attuale |
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
| Orientamento e direzionalità | [`decisions/adr-0005-orientamento.md`](decisions/adr-0005-orientamento.md) · [ADR-0008](decisions/adr-0008-rotazione-e-policy-di-facing.md) | ⏳ E16 · ADR-0008 (2026-08-10) **supera** §1 e §3 di ADR-0005: la rotazione è una **capacità del personaggio** in step, e il facing nei micro-step è quello dell'**ultimo passo compiuto** |
| **Il bot** — politica, utility, candidate | [`gameplay/spec-bot-hex.md`](gameplay/spec-bot-hex.md) | ✅ spec **attiva** (2026-08-10, [#202](https://github.com/DegrassiAaron/refactor-tactics-main/issues/202)) · ⚠️ dichiara ciò che il bot **non** sa ancora: niente Team Knowledge (⏳ E13), niente facing (⏳ E16), niente reazioni (⏳ E14) · storia in [`technical/h6-5-hex-bot-spec.md`](technical/systems/h6-5-hex-bot-spec.md) (`AS-BUILT`) |
| Migrazione degli Stable ID legacy | [`technical/piano-migrazione-stable-id.md`](technical/piano-migrazione-stable-id.md) | ⏳ **piano, non eseguito** ([#199](https://github.com/DegrassiAaron/refactor-tactics-main/issues/199)) · tassonomia decisa da [D-014](decisions/RT_PDR_00_Decision_Log.md)/[D-015](decisions/RT_PDR_00_Decision_Log.md) |
| Mappa come grafo tattico, multilivello | [`technical/spec-mappa-multilivello.md`](technical/architecture/spec-mappa-multilivello.md) · [`technical/spec-pathfinding-pf3-pf4.md`](technical/architecture/spec-pathfinding-pf3-pf4.md) | ✅ |
| Terreni, stati e propagazione | [`gameplay/spec-terreni-e8.md`](gameplay/spec-terreni-e8.md) + spec CP 8.2/8.3/8.4 | ✅ E8 |
| Conoscenza parziale: vista **e** udito | [`gameplay/brief-conoscenza-parziale.md`](gameplay/brief-conoscenza-parziale.md) | ⏳ E13 |
| Durata partita, round, scala mappe | [`gameplay/spec-durata-partita-e-scala-mappe.md`](gameplay/spec-durata-partita-e-scala-mappe.md) (D-010) | ✅ |
| Pacing del turno misurato | [`gameplay/spec-pacing-turno.md`](gameplay/spec-pacing-turno.md) | ✅ |
| Privacy dell'intento | invariante #6 + `URTIntentPrivacyLibrary` | ✅ offline |
| TurnLog, reason code, serializzazione | [`technical/spec-turnlog.md`](technical/architecture/spec-turnlog.md) · [`technical/spec-turnlog-serialize.md`](technical/architecture/spec-turnlog-serialize.md) | ✅ |
| Replay — cosa è autorevole e chi può calcolare | [`decisions/adr-0009-replay-logico-canonico.md`](decisions/adr-0009-replay-logico-canonico.md) | ⏳ **decisione presa, R1/R3 da implementare** (2026-08-10) · due prodotti: il **Player** riproduce la traccia e non calcola, il **Verifier** ri-simula e non presenta · forma dell'archivio in [D-077](decisions/RT_PDR_00_Decision_Log.md) |
| HUD e leggibilità | [`technical/progettazione-hud.md`](technical/systems/progettazione-hud.md) · [`technical/brief-planning-visuale.md`](technical/systems/brief-planning-visuale.md) | ⏳ E11 |
| Mappa delle classi C++ | [`technical/architettura-codice.md`](technical/architecture/architettura-codice.md) | ✅ |
| Struttura di `Content/`, naming asset | [`technical/convenzioni-contenuti-ue.md`](technical/tooling/convenzioni-contenuti-ue.md) | ✅ **normativo** |
| Quali asset servono, quali esistono, quanti mancano | [`technical/asset-map.md`](technical/tooling/asset-map.md) | registro, misurato sull'allowlist |
| Test automatici e scenari | [`technical/test-automatico-unreal.md`](technical/tooling/test-automatico-unreal.md) | ✅ harness consegnato · ⏳ assertion oltre il movimento |
| Verifiche interattive in editor | [`technical/test-manuali-pie.md`](technical/test-manuali-pie.md) | 🟡 |
| Scenario della showcase | [`product/showcase-v0.1.md`](product/showcase-v0.1.md) | ⏳ E15 |
| Gate di release | [`roadmap/v0.1-definition-of-done.md`](roadmap/v0.1-definition-of-done.md) | ⏳ |
| Unità ausiliarie | [`gameplay/brief-unita-ausiliarie.md`](gameplay/brief-unita-ausiliarie.md) | ✅ vincoli decisi · gameplay **fuori** dalla v0.1 |
| Azioni generiche e Overwatch universale | [`gameplay/brief-azioni-generiche-overwatch.md`](gameplay/brief-azioni-generiche-overwatch.md) | ✅ deciso ([D-012](decisions/RT_PDR_00_Decision_Log.md) · [D-025](decisions/RT_PDR_00_Decision_Log.md): **sette** voci, `Guard` universale) · ⏳ E14 |
| **Ownership di abilità, interazioni e sinergie** | [`gameplay/spec-ownership-abilita-interazioni-sinergie.md`](gameplay/spec-ownership-abilita-interazioni-sinergie.md) · [ADR-0006](decisions/adr-0006-ownership-abilita-sinergie.md) | ✅ **normativa** ([D-029](decisions/RT_PDR_00_Decision_Log.md)) · nessun runtime nuovo |
| Trigger su transizione (trap, tripwire) | [`gameplay/brief-delayed-actions.md`](gameplay/brief-delayed-actions.md) §6-bis | ✅ deciso ([D-013](decisions/RT_PDR_00_Decision_Log.md)) · nessuna epic |
| **Profilo** di azione generica (il modificatore per eroe) | [`gameplay/brief-azioni-generiche-overwatch.md`](gameplay/brief-azioni-generiche-overwatch.md) §4-bis | ✅ [D-033](decisions/RT_PDR_00_Decision_Log.md) · nome unico: **non** `GenericActionModifier` · profili concreti ⏳ |
| **Profilo di `BasicAttack` per i 4 eroi** | [ADR-0007](decisions/adr-0007-attacco-base-per-eroe.md) | ✅ **normativa** · chiude per `BasicAttack` i profili concreti lasciati aperti da [D-033](decisions/RT_PDR_00_Decision_Log.md) · Riktor cambia i numeri, Gadget resta rinviato ([#315](https://github.com/DegrassiAaron/refactor-tactics-main/issues/315)) |
| Schema della Signature e **`Misplay / Failure State`** | [`characters/_Template.md`](characters/_Template.md) | ✅ [D-032](decisions/RT_PDR_00_Decision_Log.md) · compilato sulle 4 schede v0.1 |
| Intento condizionale (`ConditionalIntent`) | [`gameplay/brief-delayed-actions.md`](gameplay/brief-delayed-actions.md) §7 | 📅 [D-034](decisions/RT_PDR_00_Decision_Log.md) · **post-v0.1**, epic E33 · bloccato dai boundary |
| Stati del personaggio e trasformazioni | [`gameplay/brief-stati-personaggio-e-trasformazioni.md`](gameplay/brief-stati-personaggio-e-trasformazioni.md) · candidature in [`characters/matrici-stati-personaggio.md`](characters/matrici-stati-personaggio.md) | 📅 [D-035](decisions/RT_PDR_00_Decision_Log.md) · **post-v0.1**, epic E34 · nessun eroe assegnato |
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
(Gadget · Phase · Riktor · Wraith), ~35 azioni a catalogo, reazioni, 8 terreni attivi, coperture e strutture,
obiettivi dinamici, HUD leggibile, determinismo verificato, build packaged. **Fuori**: rete, 4v4, GAS,
progressione, modding.

**Quali decisioni sono definitive?** Quelle in [`decisions/`](decisions/) con stato `Consolidata` e gli
invarianti del canone. **Quali sono aperte?** [`OPEN_DECISIONS.md`](OPEN_DECISIONS.md).

**Cosa è solo storico?** Tutto in [`archive/`](archive/); i PRD di visione in [`research/prd/`](research/prd/) e il corpus PDR in
[`archive/pdr-v0.1/`](archive/pdr-v0.1/RT_PDR_v0.1_consolidato.md); i piani consegnati in
[`roadmap/plans/`](roadmap/plans/); e i singoli documenti che portano in testa il banner **⚠️ Superato** o
**📦 Piano consegnato**.

**Quali test verificano le regole?** Sotto `Source/RefactorTactics/Tests/`. Il numero **si misura, non si cita
a memoria** — questo file l'ha già sbagliato cinque volte.

> **Il conteggio della suite si misura sul branch corrente**, non si cita da qui: era un
> blocco generato, e il generatore e' stato rimosso il 2026-08-21 (**D-181**). L'ultimo
> valore pubblicato — *875 test in 107 file* — era gia' fermo mentre la suite ne contava 903.

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
├── wiki/        **vuota**: le pagine di gioco vivono nel clone pubblicato (D-076). Resta un puntatore
├── characters/  pagine personaggio: v0.1, v0.2, candidati Paragon — **un kit per pagina**
│   └── radar/   gli otto SVG generati da `tools/radar/`: output, non si editano
├── research/    non normativo: PRD di visione, design, handoff — la ex `src/`, col nome che lo dice
│   ├── prd/        i quattro PRD tematici + il prompt del pivot esagonale
│   ├── design/     icone, showcase, griglie stampabili
│   └── handoff/    prompt e consegne di sessione, non ancora consumati
├── generated/   **output**, non ricerca: ha un generatore committato e non si edita
│   └── icons/      296 master iconografici — ⛔ il generatore che li produceva è uscito con D-182
└── archive/     materiale superato
    ├── src/        i sorgenti già recepiti: design, handoff, audit
    └── pdr-v0.1/   il corpus PDR v0.1, consolidato in un Markdown
```

> ⚠️ Fino al 2026-08-18 questo albero elencava `wiki/game/`, `wiki/meccaniche/` e `wiki/fazioni/` come
> se contenessero le pagine del giocatore. Non le contengono dal **2026-08-10**: [D-076](decisions/RT_PDR_00_Decision_Log.md)
> ha spostato la Wiki in un repository separato, e da allora questa sezione descriveva tre cartelle
> inesistenti. Un albero disegnato a mano non si accorge di un file che sparisce — è lo stesso difetto
> per cui i conteggi di questa pagina sono diventati generati.

### Due deviazioni dichiarate

1. **I nomi dei file restano in italiano kebab-case**, non `UPPER_SNAKE` inglese come nella struttura di
   riferimento. Il repository ha una convenzione consolidata e mescolarla peggiorerebbe la leggibilità;
   la clausola «riutilizzare i file esistenti quando possibile» lo consente.
2. **`research/` non è documentazione**: è la casella di posta dei sorgenti grezzi — PRD di visione,
   design non canonico, handoff non ancora triagiati. Vive accanto ai documenti perché è da lì che
   nascono, ma **non è normativa** e non compare nella gerarchia delle fonti sopra il livello 8.
   > **`src/` non esiste più.** Il 2026-08-19 la fase 3 di
   > [#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165) ha portato via gli ultimi
   > 112 file: `git ls-files docs/src` restituisce **zero**. `src` era ambiguo in un repository che ha
   > anche `Source/`, e conteneva quattro cose diverse sotto un nome che non ne descriveva nessuna —
   > PRD di visione, una pipeline di icone **generate**, kit non consumati e media. Oggi la natura la
   > dice la cartella: [`research/`](research/) per la ricerca, [`generated/`](generated/) per gli output.
   >
   > **La casella continua a svuotarsi**, come dal 2026-08-08: un sorgente recepito si sposta in
   > [`archive/src/`](archive/src/README.md) invece di restare con un banner. `research/` risponde a una
   > domanda sola — *cosa non è ancora stato consumato?* — e la risposta è la posizione del file, non una
   > colonna di un indice.

### Le quattro nature di un file, e perché la posizione le deve dire

Un lettore che arriva deve poter rispondere a *«questa frase decide qualcosa?»* **guardando dove sta il
file**, prima di aprirlo. Le etichette in testa ai documenti (§*Come si classifica un documento*) lo
dicono già; la cartella no, e finché non lo dice l'etichetta va cercata un file alla volta.

| Natura | Dove sta | Chi la scrive | Cosa succede se la si edita |
|---|---|---|---|
| **authored** | `product/` · `gameplay/` · `technical/` · `balance/` · `decisions/` · `characters/` | una persona | è il posto giusto: qui si cambia una regola |
| **generated** | `generated/` · `characters/radar/` | un generatore | **si perde alla rigenerazione**: si corregge la sorgente |
| **research** | `research/` | chiunque, senza gate | non decide niente, e non risolve un conflitto |
| **archive** | `archive/` | nessuno: si conserva | riscriverla falsifica la storia |

**`src/` è diventata `research/`** ([#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165),
chiusa il 2026-08-19): stesso contenuto, nome che dice cos'è invece di dove nasce. Non è una rinomina cosmetica —
`src` è ambiguo in un repository che ha anche `Source/`, e la casella di posta contiene PRD di visione,
handoff, pipeline di icone e kit non consumati, cioè quattro cose diverse sotto un nome che non ne
descrive nessuna.

⚠️ **La posizione governava i gate, non solo la leggibilità** — ⛔ fino a **D-182** (2026-08-21), che li ha rimossi. La regola resta come criterio di collocazione, e nessuno la verifica più. `check-docs-symbols.py` e
`check-docs-tables.py` esentano **per prefisso di path**: `archive/`, `research/`, `roadmap/plans/`. Spostare un documento dentro o fuori da uno di questi ne cambia la copertura **senza
che nessuno lo dica** — misurato, con `EXEMPT_DIRS = ()` il primo passa da 155 a 389 documenti e il
secondo da 165 a 404, ed entrambi diventavano rossi.
⛔ **La prescrizione che seguiva non è più eseguibile**: diceva di aggiornare `EXEMPT_DIRS` nello stesso
commit e di verificare che il numero di documenti controllati non calasse — `EXEMPT_DIRS` viveva dentro
`check-docs-symbols.py` e `check-docs-tables.py`, rimossi con **D-182**. La lezione che la motivava
resta, e vale per il prossimo gate: **un gate il cui scope collassa stampa `OK` lo stesso.**

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

### Dove va un'immagine

`docs/` è fatta di immagini più che di prosa — il 2026-08-17 erano **464 file su 888 e il 93,8% dei byte** —
e fino ad allora nessun gate sapeva dire quante fossero, chi le usasse, quali fossero la stessa immagine due
volte. Il numero di oggi lo dice
un gate che è stato rimosso con **D-182** il 2026-08-21; qui restano le tre regole, che valgono
ancora anche senza qualcuno che le verifichi.

**1. Un'immagine sta accanto al suo owner.** Un riferimento visuale di una spec vive nella cartella di
quella spec (`technical/img/`, `characters/images/`), non in una cartella di immagini globale. Se l'owner
si sposta, l'immagine lo segue nello stesso commit.

**2. Un'immagine generata segue il proprio generatore, e non si edita.** Gli otto SVG di
[`characters/radar/`](characters/radar/) escono da `tools/radar/generate.ts` e hanno un gate
(`--check`, exit 1 se divergono dai cataloghi): correggerli a mano significa perdere la correzione alla
rigenerazione successiva. ⚠️ **I loro nomi sono un contratto**: la Wiki pubblicata li incorpora via
`raw.githubusercontent.com`, quindi rinominarli rompe pagine che nessun gate di questo repository vede.

**3. Un'immagine orfana sta solo in area grezza.** Zero riferimenti significa che nessuno sa perché è lì.
È ammesso in `research/` e in `archive/` — è materiale non ancora consumato, o storia — o in un'area
**generata** con owner dichiarato, dove un output non ha riferimenti entranti per costruzione. In
nessun altro posto. Misurato all'apertura di #1165: le 393 orfane di allora erano **tutte** sotto `src/`,
e fuori di lì erano **zero**. È una proprietà da conservare, non da riscoprire.

Per le immagini *non* governate da un generatore, il nome è `<topic>--<vista>.<ext>` —
`ability-effect-system--uml.png`, non `final2`, non `image1`, e non un refuso reso permanente
(`infografic` accanto a `infographic` è una coppia che esiste davvero, in
`research/design/systems-map/`).

### ~~Gate anti-deriva~~ — ritirati il 2026-08-21

⛔ **I gate non esistono più.** La cartella `scripts/` è stata rimossa con
[D-182](decisions/RT_PDR_00_Decision_Log.md): con lei sono usciti i nove script Python e i due file di
test. Questa sezione ne documentava cinque — simboli, tabelle, link, nomi, inventario — più i due
controlli sui dati di gioco e i due generatori.

⚠️ **Ciò che nessuno verifica più**, e va saputo invece che riscoperto:

| non più controllato | il difetto che tornava |
|---|---|
| **link** | un target inesistente, e un'etichetta che mostra un percorso vecchio mentre il link funziona — erano **36** dopo un solo spostamento |
| **simboli** | un documento che cita una classe o un metodo che non esiste nel codice |
| **tabelle** | una riga vuota che spezza una tabella, e una cella con una pipe non escapata |
| **nomi** | un identificatore legacy che ricompare — il gate che chiudeva **D-130** |
| **inventario** | un'immagine incorporata e assente, un duplicato esatto non dichiarato, un'orfana fuori area grezza |
| **dati di gioco** | equipaggiamento e capability che non combaciano più con `Source/` |
| **provenienza** | le **296 icone** di `generated/icons/` non hanno più il generatore che ne dichiarava la geometria |

### Le discipline che i gate portavano con sé

⚠️ **Non erano nei comandi, erano nei loro dati** — e sarebbero uscite col codice se non fossero
riscritte qui. Valgono ancora, e valgono per il prossimo gate che qualcuno scriva:

- **Un'esenzione dichiarata è una promessa datata, non un permesso.** `DEBITO_NOTO`, `DUPLICATI_NOTI`,
  `ORFANE_NOTE` non esentavano: il gate le verificava **al contrario**, e una voce che non
  corrispondeva più a niente lo faceva fallire. Un'esenzione che non sa invalidarsi nasconde il
  difetto successivo.
- **La somiglianza non è una prova, e una famiglia-template la batte.** I candidati near-duplicate
  restavano candidati: le 38 card di `characters/images/paragon/`, generate dallo stesso layout,
  hanno prodotto **558 falsi positivi su 562**, perché un hash percettivo misura la cornice e non il
  soggetto. Si escludevano per **dichiarazione** (`FAMIGLIE_TEMPLATE`), mai alzando una soglia.
- **Un gate il cui scope collassa stampa `OK` lo stesso.** Il numero di documenti controllati era
  parte del risultato, non un dettaglio del report.
- **Un'etichetta può mentire mentre il link funziona.** ``[`../vecchio/x.md`](../nuovo/x.md)``: erano
  **36** dopo un solo spostamento, e nessun controllo sui soli target le vede.

Queste **restano scritte**; quello che manca è chi le verifica. È una scelta di fase — il progetto è in sviluppo, e il costo di
mantenerli superava quello di non averli. Il punto da riaprire è **D-182**.

Restano **due** `--check`, e nessuno dei due è Python: `node tools/radar/generate.ts --check` (gli SVG contro i cataloghi) e `node tools/radar/wiki-alt.ts --wiki-root <clone> --check` (gli alt sulla Wiki, che il primo **non** copre — lo dichiara il suo stesso docstring), piu' la suite `node --test` di `tools/radar/`, **57 test**.
