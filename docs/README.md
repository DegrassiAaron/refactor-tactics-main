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
| 2 | **Decisioni** | [`decisions/`](decisions/) — 5 ADR + [Decision Log](decisions/RT_PDR_00_Decision_Log.md) | Scelte architetturali e di prodotto, con motivazione |
| 3 | **Esecuzione** | [`roadmap/roadmap-checkpoint.md`](roadmap/roadmap-checkpoint.md) | Milestone M6–M11, DoD misurabili, **stato** |
| 4 | **Release** | [`roadmap/roadmap-v0.1.md`](roadmap/roadmap-v0.1.md) | Scope della v0.1: 17 epic, 85 checkpoint |
| 5 | **Numeri** | [`balance/`](balance/) | Valori vigenti: azioni, eroi, terreni, equipaggiamento |
| 6 | **Specifiche** | [`gameplay/`](gameplay/) · [`technical/`](technical/) | Dettaglio per feature |
| 7 | **Requisiti lungo periodo** | [`roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md`](roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md) | Fasi F0–F6 — **direzione, non scope** |
| 8 | **Visione north-star** | [`src/`](src/) (PDF dei PRD) | Prodotto a lungo termine, **non** obiettivo attuale |
| 9 | **Storico** | [`archive/`](archive/) | Materiale superato, conservato per provenienza |

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
| Ordinamento deterministico degli effetti simultanei (APNAP) | [`gameplay/spec-sequenza-turno.md`](gameplay/spec-sequenza-turno.md) §3 | ✅ |
| Motore azioni: priorità, fallback, collisioni | [`gameplay/spec-motore-azioni-e4.md`](gameplay/spec-motore-azioni-e4.md) | ✅ E4 |
| Reazioni componibili | [`gameplay/spec-reazioni-componibili-cp55.md`](gameplay/spec-reazioni-componibili-cp55.md) | ✅ E5 |
| Finestre di reazione e decision boundary | [`decisions/adr-0004-finestre-di-reazione.md`](decisions/adr-0004-finestre-di-reazione.md) | ✅ deciso, ⏳ E14 |
| Overwatch | [`gameplay/brief-overwatch-reazioni.md`](gameplay/brief-overwatch-reazioni.md) | ⏳ E14 |
| Delayed Actions e boundary nominati | [`gameplay/brief-delayed-actions.md`](gameplay/brief-delayed-actions.md) | ⏳ nessuna epic |
| Griglia esagonale e coordinate | [`decisions/adr-0002-griglia-esagonale.md`](decisions/adr-0002-griglia-esagonale.md) | ✅ |
| Orientamento e direzionalità | [`decisions/adr-0005-orientamento.md`](decisions/adr-0005-orientamento.md) | ⏳ E16 |
| Mappa come grafo tattico, multilivello | [`technical/spec-mappa-multilivello.md`](technical/spec-mappa-multilivello.md) · [`technical/spec-pathfinding-pf3-pf4.md`](technical/spec-pathfinding-pf3-pf4.md) | ✅ |
| Terreni, stati e propagazione | [`gameplay/spec-terreni-e8.md`](gameplay/spec-terreni-e8.md) + spec CP 8.2/8.3/8.4 | ✅ E8 |
| Coperture direzionali | [`gameplay/spec-copertura-cp91.md`](gameplay/spec-copertura-cp91.md) | 🟡 CP 9.1 ✅ · E9 aperta |
| Conoscenza parziale: vista **e** udito | [`gameplay/brief-conoscenza-parziale.md`](gameplay/brief-conoscenza-parziale.md) | ⏳ E13 |
| Durata partita, round, scala mappe | [`gameplay/spec-durata-partita-e-scala-mappe.md`](gameplay/spec-durata-partita-e-scala-mappe.md) (D-010) | ✅ |
| Pacing del turno misurato | [`gameplay/spec-pacing-turno.md`](gameplay/spec-pacing-turno.md) | ✅ |
| Privacy dell'intento | invariante #6 + `URTIntentPrivacyLibrary` | ✅ offline |
| TurnLog, reason code, serializzazione | [`technical/spec-turnlog.md`](technical/spec-turnlog.md) · [`technical/spec-turnlog-serialize.md`](technical/spec-turnlog-serialize.md) | ✅ |
| HUD e leggibilità | [`technical/progettazione-hud.md`](technical/progettazione-hud.md) · [`technical/brief-planning-visuale.md`](technical/brief-planning-visuale.md) | ⏳ E11 |
| Mappa delle classi C++ | [`technical/architettura-codice.md`](technical/architettura-codice.md) | ✅ |
| Struttura di `Content/`, naming asset | [`technical/convenzioni-contenuti-ue.md`](technical/convenzioni-contenuti-ue.md) | ✅ **normativo** |
| Test automatici e scenari | [`technical/test-automatico-unreal.md`](technical/test-automatico-unreal.md) | 🟡 harness in corso |
| Verifiche interattive in editor | [`technical/test-manuali-pie.md`](technical/test-manuali-pie.md) | 🟡 |
| Scenario della showcase | [`product/showcase-v0.1.md`](product/showcase-v0.1.md) | ⏳ E15 |
| Gate di release | [`roadmap/v0.1-definition-of-done.md`](roadmap/v0.1-definition-of-done.md) | ⏳ |
| Unità ausiliarie | [`gameplay/brief-unita-ausiliarie.md`](gameplay/brief-unita-ausiliarie.md) | ✅ vincoli decisi · gameplay **fuori** dalla v0.1 |
| Azioni generiche e Overwatch universale | [`gameplay/brief-azioni-generiche-overwatch.md`](gameplay/brief-azioni-generiche-overwatch.md) | ✅ deciso ([D-012](decisions/RT_PDR_00_Decision_Log.md)) · ⏳ E14 |
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

**Quali test verificano le regole?** **403 test automatici unici in 63 file**, sotto
`Source/RefactorTactics/Tests/`. Misura riproducibile — *non citare a memoria*:

```bash
grep -rhoE '"RefactorTactics\.[A-Za-z0-9_.]+"' Source/RefactorTactics/Tests/*.cpp | tr -d '"' | sort -u | wc -l
```

| Area | Test | Cosa fissa |
|---|---:|---|
| `Actions.*` | 58 | Ordine per priorità, permutazione-invarianza, fallback, mappatura di fase |
| `Reactions.*` | 27 | Attivazione singola, trigger puro, reazioni componibili, privacy |
| `HexSim.*` | 27 | Snapshot, budget, collisioni simultanee, **replay divergence 0** |
| `Heroes.*` | 25 | I 4 eroi corrispondono al catalogo, trade-off delle varianti |
| `TurnLog.*` | 22 | Hash permutazione-invariante, serializzazione versionata, checksum |
| `Combat.*` · `HexCombat.*` | 36 | Danno dopo scudo, forme, LOS, niente fuoco amico |
| `Terrain.*` · `Status.*` · `Environment.*` | 39 | Superfici, stati temporanei, propagazione elettrica, fuoco/acqua |
| `Hex*.*` (mappa, path, vision, bot, match) | ~80 | Coordinate, A\*, LOS, bot, partita completa |
| `Match*.*` | 27 | Allestimento, formato di partita, fine partita a tre vie |
| `Scenario.*` | 3 | Harness degli scenari automatici (in costruzione) |
| `Perf.*` | 2 | Path mediana **0,025 ms** · resolver **0,41 ms/turno** |

---

## Struttura delle cartelle

```
docs/
├── README.md                    ← sei qui: indice e owner dei concetti
├── DOC_CONFLICT_MATRIX.md       conflitti fra documenti e loro stato
├── OPEN_DECISIONS.md            cosa aspetta una decisione umana
├── CHANGELOG_DOCUMENTATION.md   storia della documentazione
├── brief-consolidamento-documentale.md
│
├── product/     visione, canone, vertical slice
├── gameplay/    regole di gioco: turno, azioni, reazioni, percezione, ambiente
├── technical/   architettura, mappa, pathfinding, TurnLog, UI, test, guide
│   └── img/     riferimenti visuali
├── balance/     numeri vigenti (cataloghi .md) + workbook di esplorazione
├── roadmap/     milestone, release v0.1, DoD, requisiti di lungo periodo
│   └── plans/   piani di esecuzione consegnati (storico)
├── decisions/   ADR e Decision Log
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
