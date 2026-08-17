> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> `HISTORICAL` · **Sorgente recepito — materiale NON autorevole.**
> Archiviato il **2026-08-13**. Recepito da [D-120…D-124](../../../decisions/RT_PDR_00_Decision_Log.md),
> [`../../../OPEN_DECISIONS.md`](../../../OPEN_DECISIONS.md) (`BAL-1`, `BAS-2`),
> [`../../../roadmap/roadmap-v0.1.md`](../../../roadmap/roadmap-v0.1.md) e
> [`../../../characters/index.md`](../../../characters/index.md).
> Il testo sotto **non è stato riscritto**: le correzioni sono note `⚠️` accanto alle affermazioni.
>
> ⚠️ **La base dichiarata era già vecchia al momento del consumo.** Il pacchetto verifica
> `origin/main = 79b9d891`; il 2026-08-13 `origin/main` era `05bbe3dc`. Gli ID `D-120`…`D-124` sono stati
> **rimisurati liberi** sul nuovo HEAD prima di assegnarli, non ereditati da questa riga.
>
> 🔴 **Due lacune trovate applicandolo, entrambe di merito e non di forma:**
>
> 1. **Il nome canonico non raggiunge la presentazione.** Il pacchetto assume che vietare il rename degli
>    Stable ID basti a lasciare la UI coerente. Non basta: l'etichetta a schermo è **derivata** dallo Stable
>    ID da `ARTUnit::ShortHeroName` (`RTUnit.cpp:110`, disegnata da `RTHUD.cpp:159`) e il comportamento è
>    pinnato da `RefactorTactics.Unit.ShortHeroNameFromStableId`. Il §8 chiede che «#286/#287 non presentino
>    più i nomi storici come roster player-facing» senza accorgersi che **il prodotto li presenta comunque,
>    perché li calcola**. → [#715](https://github.com/DegrassiAaron/refactor-tactics-main/issues/715).
>    🔄 **Questa nota diceva «non ha un produttore», e la correzione è del 2026-08-13.** Il produttore c'è:
>    `URTHeroData::DisplayName` (`RTHeroData.h:37`) è popolato per tutti e quattro da
>    `RTHeroCatalogLibrary.cpp`. A mancare è il **trasporto** — `ConfigureFromHeroData` copia sette campi
>    sull'unità e salta il nome — quindi il lavoro è più piccolo di come la nota lo descriveva.
> 2. **D-120 supera D-037 senza rispondere alla sua prova.** D-037 vietava `Gadget` nudo per una ragione
>    misurata — è già una categoria di equipaggiamento (`ERTEquipmentSlot::Gadget`, **8** oggetti `Gadget.*`) —
>    e quella ragione non è venuta meno. Con la convenzione `<Eroe>.<Abilità>` la migrazione porterebbe
>    `Hero.Gadget.ArcPulse` in `Gadget.ArcPulse`, accanto a `Gadget.Medkit`; `Phase` collide con `ERTMatchPhase`
>    (**367** occorrenze in 64 file di `Source/`). → [#716](https://github.com/DegrassiAaron/refactor-tactics-main/issues/716).
>
> ⚠️ **Il §6 «canary naming» è stato applicato come script, non come CI**
> (`scripts/check-docs-naming.py`): `.github/workflows/` è vuota **per scelta**
> ([D-108](../../../decisions/RT_PDR_00_Decision_Log.md)). E la copertura è dichiarata parziale: dopo la
> bonifica dei tre file sotto gate resta un arretrato di **832 occorrenze in 72 file** (erano 836 in 73 prima
> della bonifica), troppo per una sola PR onesta. Il numero si **rimisura** eseguendo lo script.

# RefactorTactics — v0.1 Focus Decisions Consolidation
**Data:** 2026-08-12  
**Base verificata:** `origin/main` = `79b9d891a4ccccee0bbd020fa55abbd071f9e797`  
**Engine baseline:** Unreal Engine 5.8.1  
**Scopo:** consolidare le decisioni v0.1 su naming roster, Guard/Brace, Overwatch, Rumore ed E21 senza trascinare scope v0.2.

> IMPORTANTE: prima di assegnare gli ID `D-120`…`D-124`, rifare una ricerca su `origin/main` e sulle PR aperte. Al momento della preparazione di questo handoff gli ID `D-120`…`D-124` non risultano usati né in `main` né nelle PR aperte.

---

## 0. Naming canonico da usare da ora

I nomi correnti/player-facing del roster v0.1 sono:

| Personaggio v0.1 | Nome da usare in documentazione/UI |
|---|---|
| **Gadget** | `Gadget` |
| **Phase** | `Phase` |
| **Riktor** | `Riktor` |
| **Wraith** | `Wraith` |

I vecchi identificatori presenti nel repository (`Gadget`, `Phase`, `Riktor`, `Wraith`, inclusi Stable ID o simboli C++ già serializzati) sono **legacy implementation identifiers** finché non viene eseguita una migrazione esplicita. Non usarli come nomi correnti del personaggio.

Quando una specifica deve citare un simbolo che oggi esiste ancora in codice, usare questa forma:

- `Wraith` — legacy Stable ID/simbolo: `Wraith...`
- `Phase` — legacy Stable ID/simbolo: `Phase...`
- `Gadget` — legacy Stable ID/simbolo: `Gadget...`
- `Riktor` — legacy Stable ID/simbolo: `Riktor...`

Non rinominare Stable ID, replay key, catalog key o simboli C++ in questa PR solo per uniformare la prosa: una migrazione di ID deve avere compatibilità/replay/versioning propri.

---

# 1. Decision Log — nuove decisioni

## D-120 — Naming canonico del roster v0.1

**Decisione**

Il roster v0.1 usa come nomi canonici/player-facing **Gadget, Phase, Riktor e Wraith**.

Le denominazioni `Gadget`, `Phase`, `Riktor`, `Wraith` non sono più nomi correnti del roster. Possono sopravvivere temporaneamente solo come **legacy implementation identifiers** quando esistono già in codice, asset, scenario o replay.

**Impatto**

- supera la parte nominale di `D-037`; resta valida la parte che collega ciascun personaggio al proprio asset Paragon;
- documentazione, Wiki, issue e UI nuove usano solo Gadget/Phase/Riktor/Wraith;
- nessun rename automatico dei Stable ID già serializzati;
- ogni futura migrazione degli ID deve essere esplicita, versionata e replay-safe.

**Stato:** Consolidata — decisione d'autore, 2026-08-12.

---

## D-121 — BAL-1: Guard e Brace mantengono il modello corrente

**Decisione**

Per la v0.1 non si separano `Guard` e `Brace` in «solo danno» contro «solo displacement» e non si introducono nuove magnitudini.

Identità tattica:

- `Guard` = difesa **front-loaded**, migliore contro il primo impatto/burst;
- `Brace` = difesa **sostenuta**, migliore contro colpi ripetuti e più robusta contro displacement forte quando il contenuto/equipaggiamento lo produce.

Il caso `Weapon.Impact + PressureJet` resta un caso valido che rende osservabile anche la differenza sul displacement.

**Regola di uscita**

La scelta di design è chiusa, ma `#403` resta aperta fino alla seduta `U20 / PIE-BAL1`.

Se `PIE-BAL1` mostra che le due azioni non sono distinguibili a occhio:
1. prima si migliora feedback/UI/presentazione;
2. solo se il feedback non basta si riapre il tuning delle magnitudini.

**Nessun rebalance numerico in questa decisione.**

**Stato:** Accettata — decisione d'autore, 2026-08-12; verifica visiva pendente.

---

## D-122 — BAS-2 / Overwatch v0.1: quattro profili, condizione opzionale, bot senza auto-condizione

**Decisione**

I quattro profili Overwatch entrano nel contenuto v0.1 e riusano la stessa infrastruttura di CP 14.4:

| Personaggio | Profilo |
|---|---|
| **Gadget** | `Conductive` |
| **Phase** | `Pressure` |
| **Riktor** | `Frontline` |
| **Wraith** | `Predictive` |

I profili cambiano **dati/geometria/parametri**, non creano quattro resolver separati.

Il ciclo resta:

`arm → micro-step → target rilevato → opportunity → FIRE/HOLD`

La condizione dichiarata v0.1 resta quella già definita da D-109:

`TargetHealthAtOrBelowPercent(N)`

ed è **opzionale**.

### Policy bot v0.1

Il bot **non dichiara automaticamente** una soglia/condizione Overwatch in v0.1.

Motivo: una soglia fissa sarebbe bilanciamento travestito da default; derivarla dall'utility planner è comportamento bot più avanzato e non serve per provare la Decision Window.

Quando riceve una opportunity sanitizzata, il bot decide **immediatamente** tramite DecisionProvider: nessuna attesa wall-clock artificiale.

### KPI

Separare sempre:

- **resolver/pacing tecnico:** DecisionProvider immediato;
- **pacing umano:** vera finestra da 3,0 s in PIE.

Il campione con bot senza auto-condizione va etichettato come tale.

**Conseguenze**

- `BAS-2` è chiusa;
- `#657` può essere chiusa dopo il merge documentale;
- `#165` continua a implementare la finestra viva; se il codice conserva un identificatore storico, la documentazione parla di **Wraith** e annota il legacy ID solo dove tecnicamente necessario.

**Stato:** Consolidata — decisione d'autore, 2026-08-12.

---

## D-123 — Rumore: `NoiseIntensity` appartiene a ogni azione che può produrre rumore

**Decisione**

Ogni azione/abilità che può produrre `FRTNoiseEvent` deve dichiarare esplicitamente il proprio `NoiseIntensity` nel dato canonico, scala intera `0..10`.

Vale per:

- le sette azioni generiche;
- le signature ability che producono rumore.

«Avere un'intensità propria» significa **possedere il campo**, non avere per forza un numero unico. Più azioni possono condividere lo stesso valore.

Questo evita di trasformare il volume in un identificatore implicito della sorgente.

**Vincoli**

- i valori generici già decisi da D-041 vengono portati in colonna;
- il tipo/identità dell'evento non determina la precisione del contatto acustico: resta valida D-113;
- `Sneak` non viene deciso qui: resta owner `AE-5`;
- nessun producer runtime deve hardcodare un valore che ha già owner nel catalogo.

**Conseguenze**

`#690` resta aperta fino all'aggiornamento reale dei cataloghi/validator; la parte di design «le signature hanno il campo?» è però chiusa: **sì**.

**Stato:** Consolidata — decisione d'autore, 2026-08-12.

---

## D-124 — E21 v0.1: soglia di presentazione necessaria, non polish completo

**Decisione**

La v0.1 considera E21 completa quando il gioco è tatticamente leggibile e non più rappresentato da placeholder, senza richiedere un presentation pass cinematografico.

### Dentro E21 v0.1

1. skeletal mesh correttamente posate sugli hex per **Gadget, Phase, Riktor, Wraith**;
2. locomozione essenziale `Idle ↔ Run`;
3. montaggi/eventi essenziali `Cast / Hit / Death`;
4. team ring e selection ring leggibili;
5. superfici/terreni riconoscibili durante la partita senza console debug;
6. camera tarata sulla scala esagonale;
7. Sessione C / verifiche PIE obbligatorie;
8. misura FPS rappresentativa dopo l'integrazione delle mesh.

### Fuori E21 v0.1

- sistema VFX completo per tutti gli status;
- Niagara dedicato a ogni abilità;
- foot IK raffinato;
- locomotion set bespoke per ogni direzione/personaggio;
- cinematic death/polish;
- presentation framework che non serve ai gate v0.1.

La presentazione non decide mai l'esito logico.

**Stato:** Consolidata — scope di release, 2026-08-12.

---

# 2. `docs/OPEN_DECISIONS.md`

## BAL-1

Non lasciarla formulata come scelta ancora aperta. La domanda è stata scelta da D-121; resta il **gate umano**.

Forma consigliata:

```md
| ~~`BAL-1`~~ | ~~`Guard` e `Brace` devono separarsi in danno contro spinta?~~ | ✅ **Decisione chiusa da D-121 (2026-08-12): status quo.** `Guard` resta front-loaded sul primo impatto; `Brace` resta sostenuta sui colpi ripetuti e robusta al displacement forte quando esiste. Nessun rebalance numerico. ⚠️ `#403` resta aperta solo per `U20 / PIE-BAL1`: il gate verifica che la differenza sia leggibile a schermo; se non lo è, prima si interviene su feedback/UI, non sui numeri. |
```

Nel testo storico che cita l'attacco di Phase, usare **Phase** come nome corrente. Se serve citare un ID esistente in codice, annotarlo come legacy, non come nome del personaggio.

## BAS-2

Sostituire la riga con:

```md
| ~~`BAS-2`~~ | ~~I quattro profili `Overwatch` entrano come contenuto di CP 14.4?~~ | ✅ **Sì — D-122 (2026-08-12).** Gadget `Conductive` · Phase `Pressure` · Riktor `Frontline` · Wraith `Predictive`. Sono profili data-driven della stessa macchina CP 14.4, non quattro rami di resolver. La condizione HP di D-109 è opzionale; il bot v0.1 non ne dichiara una automaticamente e risponde subito all'opportunity sanitizzata. |
```

## BAS-1 / BAS-3 / BAS-4 — naming

Non cambiare simboli tecnici che esistono davvero in codice. Correggere però la prosa player-facing:

- Gadget `Grounding`
- Phase `Flow`
- Riktor `Anchor`
- Wraith `Deflection`

Per riferimenti esistenti come `Hero.Phase.FlowReaction` o `Hero.Wraith.Deflection`, scrivere ad esempio:

`Phase.FlowReaction` *(legacy code id: `Hero.Phase.FlowReaction`)*

solo se viene pianificata una migrazione; altrimenti conservare il token esatto in backtick e chiamare **Phase/Wraith** il personaggio in prosa.

---

# 3. Issue GitHub

## #403 — BAL-1

**Stato:** resta OPEN fino a `PIE-BAL1`.

Aggiungere commento:

```md
## Decisione d'autore — 2026-08-12

Scelgo **status quo**, senza rebalance dei numeri.

- `Guard` = protezione front-loaded, migliore sul primo impatto/burst.
- `Brace` = protezione sostenuta, migliore sui colpi ripetuti e più robusta contro displacement forte quando il contenuto/equipaggiamento lo produce.

Il caso `Weapon.Impact + PressureJet` resta una prova utile della differenza sul displacement.

La decisione di design è presa, ma la issue **non si chiude** finché `U20 / PIE-BAL1` non è eseguita.
Se la distinzione non è leggibile a occhio, il primo intervento è feedback/UI/presentazione; tuning numerico solo se quello non basta.

Naming corrente v0.1: **Gadget, Phase, Riktor, Wraith**.
```

## #657 — Overwatch condition / bot

Aggiornare la conclusione e poi **chiudere completed** dopo che D-122 è in main.

Commento:

```md
## Decisione d'autore — 2026-08-12

`BAS-2 = SÌ`.

Profili v0.1:
- Gadget → `Conductive`
- Phase → `Pressure`
- Riktor → `Frontline`
- Wraith → `Predictive`

La condizione di D-109 (`TargetHealthAtOrBelowPercent(N)`) è opzionale.

Per la v0.1 il **bot non dichiara automaticamente una condizione**. Non introduciamo né una soglia fissa travestita da default né un'estensione dell'utility planner solo per abbassare artificialmente il numero di finestre.

Quando riceve una opportunity sanitizzata, il bot decide immediatamente tramite DecisionProvider.

I KPI distinguono:
1. resolver/pacing tecnico con risposta immediata;
2. pacing umano con finestra reale da 3,0 s.

Decisione registrata da D-122.
```

## #690 — Rumore per azione

**Stato:** resta OPEN, perché deve ancora modificare i cataloghi.

Commento:

```md
## Decisione sulla parte aperta del DoD — 2026-08-12

Sì: anche le **signature ability** che possono produrre rumore dichiarano `NoiseIntensity` nel dato canonico.

Il campo è obbligatorio per ogni producer acustico, scala `0..10`. «Proprio» non significa «unico»: più azioni possono avere lo stesso valore.

Non usiamo il volume per identificare implicitamente la sorgente. D-113 continua a governare la precisione del contatto; `Sneak` resta ad `AE-5`.

Questa decisione non chiude la issue: restano l'aggiornamento delle colonne e il validator. Decisione registrata da D-123.
```

## #286 — E21

Aggiungere commento di scope:

```md
## Scope v0.1 fissato — D-124

E21 finisce alla leggibilità necessaria per giocare e misurare la v0.1:

- mesh in scena per Gadget, Phase, Riktor, Wraith;
- Idle↔Run + Cast/Hit/Death;
- team/selection ring;
- superfici leggibili senza debug;
- camera;
- Sessione C e FPS rappresentativo.

Non si allarga a status VFX completi, Niagara per ogni skill, foot IK, cinematic polish o presentation framework aggiuntivi.

La presentazione resta consumer del resolver, mai autorità dell'esito.
```

## #287 — E21.1

Correggere il roster player-facing nel body:

```md
Gadget · Phase · Riktor · Wraith
```

Se `HeroUnitClasses`, Stable ID o asset path usano ancora identificatori storici, conservarli come riferimenti tecnici annotati `legacy`; non chiamare il personaggio con quel nome nella prosa.

---

# 4. `docs/roadmap/roadmap-v0.1.md`

Non cambiare il numero di epic/checkpoint per queste decisioni.

## Lane A — Reactions

Aggiungere una nota sotto la lane:

```md
✅ D-122 chiude BAS-2: i profili Overwatch v0.1 sono
Gadget/Conductive · Phase/Pressure · Riktor/Frontline · Wraith/Predictive.
La condizione HP di D-109 è opzionale; il bot v0.1 non la dichiara automaticamente.
#165 resta il gate della Decision Window viva.
```

Dove la roadmap nomina il personaggio per CP 14.5, usare **Wraith**. Se è necessario preservare il token tecnico corrente:

```md
Wraith.InterceptShot (legacy Stable ID corrente: `Hero.Wraith.InterceptShot`)
```

Non rinominare il token serializzato dentro questa PR.

## Lane B — Perception

Aggiungere:

```md
✅ D-123 chiude la domanda di #690 sulle signature: ogni azione/abilità che emette rumore possiede `NoiseIntensity` nel catalogo; il valore può essere condiviso. #690 resta aperta per data/validator.
```

## E21

Aggiungere alla descrizione/gate:

```md
D-124 fissa il ceiling v0.1: skeletal + locomozione essenziale + Cast/Hit/Death + leggibilità tattica + camera + Sessione C. Full status/VFX polish è post-v0.1.
Roster player-facing: Gadget · Phase · Riktor · Wraith.
```

---

# 5. Naming: correzione minima necessaria nello stesso consolidamento

Il repository corrente contiene ancora una contraddizione documentale esplicita: `docs/characters/index.md` elenca i vecchi nomi come roster e D-037 li tratta come identità RT separate dagli slot Paragon.

D-120 deve diventare la nuova autorità nominale.

## Aggiornamento minimo consigliato

`docs/characters/index.md`:

```md
## Roster v0.1

- **Gadget**
- **Phase**
- **Riktor**
- **Wraith**
```

Le pagine esistenti con filename storico possono restare temporaneamente come redirect/compatibilità finché non si decide la migrazione dei path. Non cancellare file né cambiare Stable ID in massa nella stessa PR.

Aggiungere una nota:

```md
> I token storici ancora presenti in codice, scenario e replay sono legacy implementation identifiers.
> Il nome player-facing/canonico del personaggio è quello elencato qui.
```

## `D-037`

Non cancellarla: emendarla/superarla **solo sulla parte nominale**.

La sua informazione utile — quale asset Paragon alimenta la presentazione — resta vera. D-120 cambia la conclusione «lo slot non è l'identità» per il roster v0.1: da ora il nome canonico coincide con Gadget/Phase/Riktor/Wraith, mentre gli identificatori precedenti restano compatibilità tecnica fino a migrazione.

---

# 6. Test e gate dopo il consolidamento

| Area | Gate |
|---|---|
| Guard/Brace | `PIE-BAL1` obbligatorio; nessuna chiusura #403 prima del risultato |
| Overwatch | test CP14.5 + pacing tecnico con DecisionProvider immediato + sessione umana 3,0 s |
| Rumore | validator: ogni producer acustico ha `NoiseIntensity 0..10`; nessun hardcode duplicato |
| E21 | #287 → #288/#289 + Sessione C; FPS solo dopo skeletal/presentazione rappresentativa |
| Naming | grep/document audit: nuova prosa non introduce Gadget/Phase/Riktor/Wraith come nomi player-facing |

### Canary naming suggerito

Aggiungere un check documentale semplice (script o CI) sulle directory normative, con allowlist per i legacy token nei blocchi tecnici:

```text
docs CURRENT: vietato usare i vecchi nomi come testo player-facing
legacy code ids: consentiti solo in backtick/annotati come legacy
archive/HISTORICAL: escluso dal gate
```

Non fare un search/replace globale: romperebbe simboli C++, Stable ID, link e riferimenti storici.

---

# 7. Sequenza Git consigliata

1. `git fetch origin`
2. verificare `origin/main` e PR aperte;
3. ricontrollare che `D-120`…`D-124` siano liberi;
4. branch:
   `docs/v01-focus-decisions-2026-08-12`
5. commit 1:
   `docs(decisions): fix v0.1 roster naming and close focus decisions`
6. commit 2:
   `docs(roadmap): apply v0.1 focus decisions to open questions and lanes`
7. aggiornare issue #403/#657/#690/#286/#287;
8. `#657` chiusa only-after-docs;
9. `#403`, `#690`, `#286`, `#287` restano aperte fino ai rispettivi gate;
10. eseguire validator/link checker/registry generation secondo i comandi già documentati nel repo;
11. rileggere `origin/main` prima del merge per evitare collisioni di ID.

---

# 8. Definition of Done di questo consolidamento

- [ ] D-120 registra **Gadget, Phase, Riktor, Wraith** come nomi correnti v0.1.
- [ ] D-121 sceglie lo status quo Guard/Brace senza cambiare numeri.
- [ ] `BAL-1` non è più una scelta aperta; `#403` resta aperta solo per `PIE-BAL1`.
- [ ] D-122 chiude `BAS-2`.
- [ ] Gadget/Conductive, Phase/Pressure, Riktor/Frontline, Wraith/Predictive sono i profili v0.1.
- [ ] `#657` è chiusa dopo il merge della decisione.
- [ ] D-123 impone `NoiseIntensity` a ogni producer, incluse signature rumorose.
- [ ] `#690` resta aperta per applicare dati/validator, senza decidere `Sneak`.
- [ ] D-124 blocca il ceiling E21 senza espansione di polish.
- [ ] #286/#287 non presentano più i nomi storici come roster player-facing.
- [ ] Roadmap lane A/B ed E21 riportano le nuove decisioni.
- [ ] Nessun Stable ID/replay key viene rinominato implicitamente.
- [ ] Feature Registry e link checker restano verdi.
- [ ] `PIE-BAL1` viene eseguita prima di chiudere #403.

---

## Nota operativa

Questo pacchetto **non implementa gameplay nuovo**. Chiude le scelte d'autore e impedisce che codice, issue e roadmap continuino a divergere.

Il prossimo lavoro runtime resta:
- Lane Reactions: `#165 → #166`;
- Lane Perception: `#690 + #686 → #159 → #160`;
- E21: `#287 → #288/#289`.

Con i nomi correnti: **Gadget, Phase, Riktor, Wraith**.
