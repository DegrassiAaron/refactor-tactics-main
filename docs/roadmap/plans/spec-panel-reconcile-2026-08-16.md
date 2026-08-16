# Spec panel — il mandato Reconcile misurato prima di consumarlo

> `SNAPSHOT` · **2026-08-16** · Oggetto: `docs/src/CLAUDE_Reconcile_v0.1_Skill_Ability_Issues_2026-08-16.md`
> **Cosa è**: la revisione del mandato *come specifica*, eseguita prima di applicarlo. Ogni giudizio qui
> sotto è una misura, non un'impressione: la colonna «Misura» dice cosa è stato eseguito per ottenerlo.
> **Perché esiste**: il mandato è una **fotografia datata**, e fra la sua stesura e questo consumo il
> repository si è mosso — in un caso di ore.

## Il verdetto in una riga

Il mandato **regge nella parte normativa** (§1 baseline, §11 naming, §15 confini decisionali) e **è
invecchiato nella parte descrittiva** (§2, §3, §4, §7, §8, §11): descrive uno stato di `main` e delle issue
che in più punti non esiste più. Applicarlo alla lettera rifarebbe misure già fatte e, in un caso, farebbe
**mentire i documenti sul codice**.

---

## Panel — critique mode

### 📋 KARL WIEGERS — qualità dei requisiti

🔴 **CRITICO — il DoD §17 non è falsificabile in blocco.** Diciassette caselle, di cui almeno quattro non
dicono *chi* misura né *in che finestra*:

- *«nessun nuovo hardcode eroe-specifico è stato introdotto»* — introdotto **da chi**, e **da quando**? Senza
  un ref di base la casella è vera per costruzione se la misuri sul tuo diff, e non misurabile su `main`.
- *«nessun nuovo leak di planning avversario è possibile»* — «possibile» è una proprietà universale: nessuna
  esecuzione finita la stabilisce. La forma verificabile è *«i DTO X, Y, Z non contengono i campi A, B»*.
- *«suite automatica pertinente è verde»* — «pertinente» non è definito, e il repository ha già pagato questo:
  un conteggio test copiato invece che misurato.

📝 **RACCOMANDAZIONE**: le caselle di forma universale vanno riscritte come asserzioni su artefatti nominati.
🎯 **PRIORITÀ**: alta — è il DoD che decide quando il lavoro è finito.

✅ **Ciò che invece è scritto bene**: *«Se #738 è ancora aperta e il gate non esiste, non inventare che sia
verde»* (§13). È un anti-pattern dichiarato in modo eseguibile, ed è raro.

### 🎯 GOJKO ADZIC — falsificabilità ed esempi

🔴 **CRITICO — il criterio di verifica del §8 non distingue i due concetti che deve separare.** Il mandato
propone:

```bash
grep -Rni "Reaction.*Intercept\|Intercept.*Reaction" Source docs Scenarios
```

Eseguito, questo pattern restituisce decine di righe su **`Reaction.AllyIntercept`** — l'interposizione, che
*è* una reaction, correttamente, per D-017. Il concetto che il §8 vuole proteggere è **`InterceptShot`**, che è
un'altra cosa. Un criterio che segnala in massa il comportamento corretto viene disattivato al primo uso: è
la stessa lezione che il referto del §12 ha già tratto per #738 — **sei falsi positivi su dodici**.

📝 **RACCOMANDAZIONE**: il grep giusto è sul token `InterceptShot`, non sulla co-occorrenza di due parole.

⚠️ **MAGGIORE — «before → after» è richiesto (§18) ma il formato della prova non è definito.** Il repository
ha già una forma che funziona (la riga citata prima, la misura dopo). Il mandato la chiede senza nominarla.

### 🔨 ALISTAIR COCKBURN — attore e obiettivo

⚠️ **MAGGIORE — il mandato non dichiara chi consuma il risultato.** La §18 chiede otto sezioni di output
(A→H), ma nessuna dice a *chi* servono. La conseguenza è concreta: la «Issue matrix» con `Before/After/PR`
per 23 issue è un artefatto costoso, e se il lettore è l'autore che ha già deciso, gran parte è cerimonia.

✅ **Ciò che invece funziona**: la §16 «Priorità operativa» dichiara un ordine A→L. È l'unica parte del
mandato che rende il lavoro **interrompibile senza perderlo** — e infatti è quella che ha retto meglio.

### 🏗️ MARTIN FOWLER — confini e architettura

✅ **La §1 è la parte migliore del documento.** Dieci invarianti architetturali, tutte in forma negativa
verificabile («niente seconda pipeline per Reaction, Predictive, Equipment o Elemental»), più due diagrammi
di pipeline che dicono *dove* passa l'autorità. Non è invecchiata di una riga.

✅ **La §4 evita la trappola più comune** — *«non creare `BraceResolver`, `OverwatchResolver`,
`FastReactionResolver`, `ClashResolver` come autorità indipendenti»* — e nomina esattamente dove le differenze
devono vivere: trigger, profilo, allowed responses, effect specs, policy, dati.

⚠️ **MINORE**: la §12 chiede una scheda di 13 campi per issue senza dire quando è **completa**. Su 23 issue è
un ordine di grandezza di lavoro che il mandato non stima. Il referto precedente lo ha dichiarato come
perimetro escluso — la scelta giusta, ma è una lacuna della specifica che ha costretto il lettore a coprirla.

### 🧪 LISA CRISPIN — validazione

⚠️ **MAGGIORE — il §13 chiede i gate senza chiedere la baseline.** Quattro `--check` da eseguire, e nessuna
riga che dica di misurarli **prima** di toccare i file. Un gate rosso preesistente attribuito al proprio
lavoro costa un'indagine inutile; un gate rosso proprio scambiato per preesistente costa un merge sbagliato.

📝 **RACCOMANDAZIONE**: misurare i quattro gate su `origin/main` prima del primo edit e riportare i due
numeri, non uno.

---

## L'invecchiamento, misurato voce per voce

Fra la stesura del mandato e questo consumo, **cinque** delle sue premesse descrittive sono cadute. Tre nella
stessa giornata, dopo le 14:00.

| § | Il mandato asserisce | Misura | Esito |
|---|---|---|---|
| §2 | **#1006** è da riconciliare, verifica la matrice A/B/C/D | `gh issue view 1006` → `CLOSED`; opzione **C** decisa e implementata (PR #1008) | 🔴 invecchiato |
| §2 | **#995** va aggiornata se contiene affermazioni non misurate | track `proficiency` `ACTIVE` su #995, referto già prodotto (`plans/proficiency-misura-2026-08-16.md`) | ⛔ **conteso** — D-139 |
| §3 | **#63** è `open`, `v0.1/P2` | `CLOSED` il 2026-08-16 alle 16:31Z | 🔴 invecchiato |
| §4 | **#583** dichiara ancora il blocco da #165 | corretta alle 12:20Z; i `#165` residui sono **cronaca della correzione** | 🔴 invecchiato |
| §4 | **#152** dichiara «l'ultima epic della v0.1» | corretta alle 12:24Z, con nota di rettifica e attribuzione | 🔴 invecchiato |
| §7 · §15 | **#403** è una decisione d'autore **aperta** | corretta alle 13:29Z; titolo attuale: *«la decisione c'e' (D-121), resta il gate umano U20/PIE-BAL1»* | 🔴 invecchiato |
| §8 · §11 | `Vektor.InterceptShot` — token legacy da non migrare qui | il codice ha già **`Hero.Wraith.InterceptShot`** | 🔴 invecchiato |

⚠️ **Il §15 è il caso più delicato**, perché non è solo invecchiato: è un **elenco di cose che Claude non
deve decidere**, e include #403. Una decisione già presa (D-121) lasciata in quell'elenco produce l'errore
opposto a quello che l'elenco previene — fermarsi a chiedere all'autore ciò che l'autore ha già risposto.

---

## Il finding del §8 ha la direzione invertita

Il mandato chiede di *«verificare che nessun documento/issue recente abbia **ri-trasformato** `InterceptShot`
in Reaction»*, cioè presume una **regressione documentale** su un codice fermo.

La misura dice il contrario.

**Il codice ha migrato, i documenti no.** In `Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp`:

```cpp
URTActionData* InterceptShot = MakeHeroAction(TEXT("Hero.Wraith.InterceptShot"), ERTResolutionPhase::Preparation,
    /*Priority*/ 30, /*Range*/ 1, /*Cooldown*/ 2, ERTActionFallback::Cancel,
    { FRTActionEffectSpec(ERTActionEffect::Damage, 16) }, ERTAbilityShape::Single, /*AreaRadius*/ 0,
    ERTActionSlot::Main, /*bInterruptible*/ false);
InterceptShot->Def.PredictiveTargeting = ERTPredictiveTargeting::LockCell;
InterceptShot->Def.PredictionBoundary  = ERTPredictionBoundary::MovementEntry;
```

e l'header è esplicito: *«`InterceptShot` **non e' piu' una reazione**: dal 2026-08-10 (E18 CP 18.2, D-016)
e' una **Predictive Action**»*. Lo slot è tornato `Main`; i due campi che la rendono predittiva stanno **nei
dati**, non in un ramo del resolver.

Sono i documenti a non aver recepito la riclassificazione del 2026-08-10:

| File | Riga | Cosa dice oggi |
|---|---|---|
| `docs/decisions/RT_PDR_00_Decision_Log.md` | 442 | *«**oggi** l'azione è a catalogo con `ERTActionSlot::None` e **nessun trigger** … resta una migrazione di classificazione **da tracciare, non da fare**»* — l'«oggi» è il **2026-08-08**, e non è scritto |
| `docs/product/showcase-v0.1.md` | 116 | *«**due** reazioni d'eroe non sono cablate: `Riva.FlowReaction` e `Vektor.InterceptShot`»* |
| `docs/characters/spec-radar-profilo-personaggio.md` | 277 | *«Le **reazioni** rinviate a E14 contano. `Vektor.InterceptShot` e `Riva.FlowReaction`…»* |
| `docs/roadmap/roadmap-v0.1.md` | 102 · 174 | *«**tre reazioni su cinque** cablate; `InterceptShot`/`FlowReaction` rinviate»* |
| `docs/roadmap/roadmap.shortlist.md` | 25 | idem — ⚠️ **vista generata**: si rigenera, non si edita |

🔴 **Conseguenza operativa**: applicare il §8 alla lettera («se trovi prosa CURRENT che li confonde, correggi
gli owner») avrebbe fatto cercare un colpevole inesistente. Il lavoro vero è l'opposto: **propagare ai
documenti una migrazione che il codice ha già fatto sei giorni fa**.

⚠️ Il conteggio *«tre reazioni su cinque»* va rimisurato, non aggiustato di uno: se `InterceptShot` esce
dall'insieme delle reazioni, cambia il **denominatore**, e resta da verificare se `FlowReaction` è l'unica
non cablata o se il quadro è cambiato anche lì.

---

## Cosa il mandato NON copre, e serve

Il consumo richiesto va oltre il mandato. Queste quattro voci non hanno una sezione che le possieda:

- **Wiki** — il mandato non la nomina mai. È un **repository separato** (D-076): il clone è la fonte, e non
  si aggiorna con una PR di questo repo.
- **Verifiche PIE** — il §13 non le elenca fra il tracking da riconciliare. Se qualcosa resta da fare dentro
  l'editor, la voce va in `docs/technical/test-manuali-pie.md`, non in un acceptance criterion ordinario.
- **Epic relation** — il mandato parla di issue e di dipendenze, mai di relazioni epic→checkpoint come
  oggetto da creare o correggere.
- **Scenario map** — nominata nel §13 (`docs/technical/scenario-map.md`) ma senza dire cosa vi si
  riconcilia, e la verifica di uno scenario vive in `expect`, non nel conteggio delle asserzioni.

---

## Perimetro dichiarato di questo consumo

**Consumato**: §0 (misura), §8 (con direzione corretta), §11 limitatamente a ciò che il §8 tocca, §13
(tracking + gate), §16 per la parte residua, §12 solo per le issue non già riconciliate da altri.

**Non consumato, e perché**:

- **§2 su #995** — track `proficiency` `ACTIVE`: **STOP** per D-139, non una lacuna.
- **§2 su #1006** — issue chiusa, decisione presa: riaprirla sarebbe un regresso.
- **§5 (#314/#319), §6 (#888), §9 (#690/#686/#159), §10 (UI consumers)** — sono lavoro di gameplay e
  decisioni d'autore, non riconciliazione documentale. Il §10 lo dice esplicitamente: *«Aggiorna queste issue
  **solo** se il contratto skill/reaction riconciliato cambia una loro premessa»* — e questo giro non lo cambia.
- **La scheda §12 completa per 23 issue** — perimetro già escluso dal referto precedente, per costo; nulla
  di nuovo lo rende più economico oggi.
