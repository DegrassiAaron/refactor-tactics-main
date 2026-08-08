# Brief — Consolidamento documentale (revisione `/sc:spec-panel`)

> ## 📦 `DELIVERED PLAN` — REVISIONE CONCLUSA, NON NORMATIVO
>
> Il **verbale** della revisione documentale del 2026-08-07, già eseguita. Non è una specifica e non descrive
> lo stato di oggi: inventario e conteggi valgono per l'`HEAD 50159c6` di allora. Spostato qui il 2026-08-08
> perché a radice di `docs/` si leggeva come un documento operativo.
>
> Owner corrente della struttura: [`README.md`](../../README.md) · stato corrente:
> [`roadmap-checkpoint.md`](../roadmap-checkpoint.md).

> **Stato**: revisione documentale · **Data**: 2026-08-07 · **HEAD analizzato**: `50159c6`
> **Origine**: `/sc:spec-panel` su [`docs/archive/src/handoff/consolidamento-prd-source-of-truth.md`](../../archive/src/handoff/consolidamento-prd-source-of-truth.md)
> **Cosa è**: il verbale della revisione — inventario, critica del sorgente, piano di migrazione.
> **Autorità**: subordinato a [`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md).

## Esito — decisione dell'utente, 2026-08-07

La revisione **sconsigliava** la riorganizzazione in cartelle (§1, §6) e raccomandava il solo indice per owner
del concetto. L'utente ha scelto la **riorganizzazione completa**, con la raccomandazione contraria già nota.
Decisione registrata ed eseguita: **101 file spostati**, **474 link** riscritti in modo programmatico e
verificati (**0 rotti**), **8 difetti** corretti. Dettaglio in
[`CHANGELOG_DOCUMENTATION.md`](../../CHANGELOG_DOCUMENTATION.md).

Le sezioni §1–§7 restano il **verbale com'era**, non riscritto a posteriori: servono a spiegare *perché* la
raccomandazione era diversa, e a rendere reversibile la scelta se il costo si farà sentire.

Per non definire la stessa cosa in due posti, tre sezioni sono state **promosse** a documento con owner proprio:

| Contenuto | Era | Owner |
|---|---|---|
| Matrice dei conflitti | §3 | [`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) |
| Decisioni aperte | §8 | [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) |
| Punto d'ingresso e owner dei concetti | §6 | [`README.md`](../../README.md) |

---

## 1. Verdetto

Il documento sorgente chiede di trasformare il repository nella fonte di verità del progetto. **Lo è già.**
La governance documentale esiste, è a tre livelli (canone → esecuzione → release), ha un Decision Log, cinque
ADR, cataloghi di bilanciamento versionati e 82 checkpoint tracciati su GitHub. Applicare §24 alla lettera —
la struttura target `product/ gameplay/ technical/ balance/ roadmap/ decisions/` — significherebbe **smontare
una governance funzionante per rimontarne una equivalente**, con l'unico effetto certo di rompere ~200 link
interni e la storia git dei file.

Il documento sorgente è stato scritto **senza conoscere lo stato del repository**: prescrive come lavoro da
fare 18 conflitti su 22 che il repo ha già risolto e registrato. Questo non lo rende inutile — al contrario:
i 4 conflitti che **non** ha già risolto sono reali, e sono i più costosi.

Quello che resta davvero da fare:

| | Area | Peso |
|---|---|---|
| **A** | 4 documenti sorgente **mai triagiati** (Auxiliary Units, Azioni generiche/Overwatch universale, Predictive Actions/Trap, 4v4) | grande |
| **B** | 1 decisione di prodotto **aperta e bloccante**: formato principale 3v3 (D-001) *vs* 4v4 | grande |
| **C** | 7 **difetti verificati** nei documenti correnti — canone che cita codice inesistente, link rotti, avvisi obsoleti | piccolo, urgente |
| **D** | Punto d'ingresso mancante: `docs/README.md` non esiste, ed è esattamente ciò che §34 del sorgente chiede | piccolo |

---

## 2. Fase A — Inventario e classificazione

125 file sotto `docs/`. Classificazione secondo §31 del sorgente (`CURRENT · PDR · PROPOSAL · RESEARCH ·
ARCHIVE · UNKNOWN`), con l'aggiunta della colonna **owner del concetto** — perché la regola che conta
(§ Fase C) non è «dove sta il file» ma «chi possiede la regola».

### 2.1 CURRENT — normativi

| Documento | Ruolo | Owner del concetto |
|---|---|---|
| `piano-canonico-mvp.md` | **Canone**: decisioni, invarianti, regole | invarianti #1–#7, gerarchia fonti, north-star |
| `roadmap-checkpoint.md` | **Esecuzione**: milestone M6–M11, DoD, stato | stato per milestone, KPI, rischi |
| `roadmap-v0.1.md` | **Release**: 16 epic, 82 CP, stato misurato | scope v0.1, mappatura epic↔milestone |
| `v0.1-definition-of-done.md` | Gate di release `G1`–`G14` | criteri di consegnabilità |
| `convenzioni-contenuti-ue.md` | **Normativo**: `Content/`, naming, dipendenze | struttura asset UE |
| `adr-0001` … `adr-0005` | Decisioni architetturali | unità skeletal · hex · modello azioni · finestre di reazione · orientamento |
| `../PDR/RT_PDR_00_Decision_Log.md` | Decision Log D-001…D-010 | decisioni di prodotto |
| `balance/RT_*Catalog_v0.1.md` (4) + `RT_TestMatrix` | **Numeri vigenti** | azioni · eroi · terreni · equipaggiamento |
| `../Data/RefactorTactics_Balance_Matrices_v0.1.xlsx` | Esplorazione numerica riallineata (D8–D11) | parametri non ancora in `.md` |
| `test-manuali-pie.md` | Verifiche interattive per sessioni | PIE |
| `architettura-codice.md` | Mappa delle classi C++ | — |

### 2.2 CURRENT — specifiche di feature attive

`spec-durata-partita-e-scala-mappe.md` (D-010) · `spec-sequenza-turno.md` (APNAP) · `spec-reazioni-componibili-cp55.md` ·
`spec-stati-temporanei-cp82.md` · `spec-propagazione-elettrica-cp83.md` · `spec-fuoco-acqua-cp84.md` ·
`spec-terreni-e8.md` · `spec-motore-azioni-e4.md` · `spec-pacing-turno.md` · `showcase-v0.1.md` ·
`test-automatico-unreal.md` (harness, **primo blocco atterrato** in `50159c6`) · `v0.1-issue-plan.md` ·
`hex-map-roadmap.md` · `roadmap-editor.md` · `spec-team-identity.md` · `spec-pathfinding-pf3-pf4.md` ·
`spec-mappa-multilivello.md` · `h6-*-spec.md` (3) · `../HUD/progettazione-hud.md` · `../Data/use-case-list.md`.

### 2.3 PROPOSAL — brief triagiati, non ancora vincolanti

| Brief | Sorgente in `src/` | Sbocco |
|---|---|---|
| `brief-conoscenza-parziale.md` | `..._Rumore_Claude.md` + sessione | **E13** (`#151`, CP 13.1–13.5) |
| `brief-overwatch-reazioni.md` | `..._Overwatch_FastReaction_Claude.md` | **E14** (`#152`, CP 14.1–14.6) + ADR-0004 |
| `brief-planning-visuale.md` | `..._ActionGhosts_Phases_FastReactions_Claude.md` | **E11** CP 11.5/11.6 (`#172`/`#173`) |
| `brief-delayed-actions.md` | `..._DelayedActions_PhaseWindows_Claude.md` | ⚠️ **nessuna epic aperta** (deliberato) |
| `brief-ghiaccio.md` | `... terreno Ghiaccio ... .md` | E8, chiusa |

### 2.4 ⚠️ UNKNOWN — sorgenti **mai triagiati**

Il buco reale. Quattro documenti in `docs/src/`, tutti del 2026-08-07, nessun brief, nessuna epic, nessuna issue:

| Sorgente | Contenuto portante | Copertura attuale |
|---|---|---|
| `..._Auxiliary_Units_Claude.md` (28 §) | Concetto unico `AuxiliaryUnit` (pet/drone/turret/summon/gadget/decoy), action economy, Scout Drone e Turret come primi prototipi, 8 issue già formulate | **zero** |
| `..._AzioniGeneriche_Overwatch_Universale_v0.1.md` (41 §) | Grammatica comune `Wait · BasicAttack · Interact · Brace · Move · Overwatch`; `Sneak/Move/Sprint` come **profili**; `FRTOverwatchProfile` per eroe; policy `Automatic/Conditional/FastSelect`; costo-opportunità dell'Overwatch | **parziale**: E14 copre la finestra, **non** l'universalità né i profili |
| `..._Predictive_Actions_Traps_Claude.md` (31 §) | Distinzione obbligatoria fra Predictive Action · Trap persistente · Fast Reaction; tassonomia a 14 categorie; 3 prototipi (`Intercept Cell`, `Tripwire Edge`, `Punish Action`) | **parziale**: `brief-delayed-actions.md` copre le Delayed, **non** le trap persistenti né i trigger su arco |
| `..._Consolidamento_NuoveDecisioni_4v4_Claude.md` (35 §) | Scenario 4v4 mirror di stress; arena a tre direttrici; blocco del formato principale; F4.5 in roadmap | **zero** |

Tre di questi non sono nemmeno versionati: vedi difetto **D6**.

### 2.5 PDR — requisiti di lungo periodo

`../PDR/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md` (sorgente Markdown, D-009) + 11 snapshot PDF `v0.1`.
**Direzione, non scope.** Due divergenze consolidate e dichiarate: rete anticipata dal PDR (F1) vs differita a
M10; GAS previsto in F2 vs no-GAS nel canone.

### 2.6 RESEARCH / ARCHIVE

`src/*.pdf` (8 PDF di visione north-star) · `sequenza-turno.md` (trascrizione esplorativa) ·
`archive/` (2 file + README) · i `plan-*` e `h5c*` degli editor (storia di esecuzione consegnata).

### 2.7 Superato ma **non marcato** — vedi difetto D7

`spec-pathfinding.md` · `spec-bot-utility.md` · `spec-knockback.md` · `spec-hover-cella.md` ·
`spec-anima-risoluzione.md` · `spec-turnlog.md` · `plan-turnlog.md` — descrivono il substrato **quadrato**,
rimosso al CP 7.2. Solo `spec-terreni.md` porta un marker di superamento.

---

## 3. Fase B — matrice dei conflitti

Prodotta qui, **promossa** a documento proprio: [`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md).
Copre tutti e 22 i controlli obbligatori di §25 del sorgente, in 26 righe.

**Esito**: 20 già risolti (`CONFIRMED`/`SUPERSEDED`) · 1 `DUPLICATE` chiuso · **3 `OPEN`** (unità ausiliarie,
Overwatch universale, trap persistenti) · **1 `CONFLICT`** bloccante (formato principale).

## 4. Difetti verificati nei documenti correnti

Non ipotesi: ognuno è stato misurato sul branch al commit `50159c6`.

| # | Difetto | Evidenza | Gravità |
|---|---|---|---|
| **D1** | `piano-canonico-mvp.md` §5 «Classi principali» elenca **4 classi su 10 che non esistono più**: `URTGridLibrary`, `URTTurnResolver`, `ARTGameState`, `URTAbilityData` | `grep -rl "class .*URTGridLibrary" Source/` → vuoto. Le prime tre sono state rimosse al CP 7.2; `URTAbilityData` è oggi `URTActionData`/`URTHeroData` | **alta** — è il documento che prevale su tutto, e descrive un codice che non c'è |
| **D2** | `docs/README.md` **non esiste**. §34 del sorgente lo indica come il punto d'ingresso da cui un nuovo sviluppatore deve capire il progetto | `ls docs/README.md` → *No such file* | **alta** — l'unico requisito di §24 che manca davvero |
| **D3** | `CLAUDE.md` linka `docs/SuperClaude_RefactorTactics_CheatSheet.md`; il file è in `docs/src/`. Stesso link rotto in `archive/README.md` | `ls docs/SuperClaude_*.md` → *No such file*; `ls docs/src/SuperClaude_*.md` → ok | media | <!-- nomi storici: il file è oggi `docs/src/superclaude-cheatsheet.md` -->
| **D4** | `brief-delayed-actions.md` §6.1 afferma «`RTReactionLibrary`: nel repo **non c'è** un file con quel nome». **C'è**: `Turn/RTReactionLibrary.{h,cpp}`, epic E5, 27 test | `roadmap-v0.1.md` riga 75 lo dichiara ✅ nella stessa giornata | media — un brief che smentisce sé stesso induce a ricostruire ciò che esiste |
| **D5** | `brief-conoscenza-parziale.md` §11 avverte «il workbook **non è ancora versionato** in git (`docs/*.xlsx` untracked)». È versionato (allora in `docs/Data/`, oggi in `docs/balance/`) | `git ls-files docs/` → `docs/balance/RefactorTactics_Balance_Matrices_v0.1.xlsx` | bassa |
| **D6** | Tre sorgenti di `docs/src/` sono **untracked**, incluse le due che *chiedono* il consolidamento | `git status --porcelain docs/` → `?? ..._NuoveDecisioni_4v4_...md`, `?? ..._Consolidamento_PRD_SourceOfTruth_...md`, `?? ..._MatchTiming_MapScale_...md` | media — un input non versionato non è una fonte |
| **D7** | Sette spec descrivono il substrato **quadrato** senza marker di superamento: `spec-pathfinding` (dichiara «MVP 2D/Manhattan» come corrente), `spec-bot-utility`, `spec-knockback`, `spec-hover-cella`, `spec-anima-risoluzione`, `spec-turnlog`, `plan-turnlog` | intestazioni prive di `Superato`/`Storico`; solo `spec-terreni.md` ce l'ha | media — sono la trappola in cui il sorgente stesso è caduto (§11: «qualunque documentazione 4-way deve essere marcata `SUPERSEDED`») |

> Il difetto D1 è la conferma pratica della tesi del sorgente: **la deriva non nasce dai PDF vecchi, nasce dai
> documenti correnti che nessuno rilegge dopo un refactor.** Il PDF del 2026-08-01 è onestamente storico; il
> canone del 2026-08-05 che elenca classi rimosse il 2026-08-06 è pericoloso perché è dichiarato vincolante.

---

## 5. Le quattro aree aperte

### 5.1 Auxiliary Units — `OPEN`, nessuna copertura

Il sorgente chiede un **concetto unico data-driven** invece di sette sistemi (pet, drone, torretta, summon,
gadget, construct, decoy). Baseline: max 1 attiva per proprietario · 0 abilità proprie · 0–1 comando per turno ·
nessun Ready indipendente · `StableUnitId` · snapshot e TurnLog sì. Regola di action economy: **un'ausiliaria
non concede un secondo turno completo**.

**Verdetto del panel**: la parte architetturale (`StableUnitId`, stessi sistemi di cella/occupancy/visibility)
è a costo quasi nullo *se decisa ora* e a costo alto se scoperta dopo — è la classica assunzione «ogni unità è
uno dei 4 eroi» che si insinua nei tipi. La parte di gameplay (Scout Drone, Turret) è **fuori dalla v0.1**: il
canone lo dice già («la v0.1 deve usare questi sistemi solo se necessari al roster definitivo», sorgente §19) e
il roster definitivo non ne ha bisogno.

**Azione proposta**: un brief `brief-unita-ausiliarie.md` che (a) registri la decisione «concetto unico, non
sette», (b) elenchi le assunzioni da **non** prendere nei tipi durante E13/E14/E16, (c) dichiari esplicitamente
il gameplay fuori scope v0.1. Nessuna epic.

### 5.2 Azioni generiche e Overwatch universale — `OPEN`, copertura parziale

E14 costruisce la **finestra** di reazione. Il sorgente chiede due cose in più che E14 non copre:

1. **Grammatica comune**: `Wait · BasicAttack · Interact · Brace · Move · Overwatch` disponibili a tutti, con
   `Sneak/Move/Sprint` come **profili della stessa azione Move** e non tre abilità.
2. **Overwatch universale con profilo per eroe**: `FRTOverwatchProfile` (area, arco, trigger ammessi, risposte
   legali, charge, `MaxPrompts`, policy `Automatic/Conditional/FastSelect`, fasi sorvegliate).

Il punto che rende questa area **urgente e non rinviabile**: la policy `Automatic`/`Conditional` è la
**mitigazione principale** del rischio già registrato in `brief-overwatch-reazioni.md` §5 — «la resolution
triplica in modo non prevedibile». Se l'Overwatch diventa universale *dopo* che la finestra è stata costruita
assumendo un solo consumatore (`Vektor.InterceptShot`), la mitigazione arriva a valle del problema.

Il sorgente aggiunge anche una regola di bilanciamento non registrata da nessuna parte: **l'Overwatch compete
con l'azione offensiva del turno** (`Attack` OR `Ability` OR `Overwatch`, mai `Attack + Overwatch`). Va decisa
prima di CP 14.4, non dopo.

**Azione proposta**: brief `brief-azioni-generiche-overwatch.md`, e **`ResolutionPolicy` anticipata dentro
CP 14.3** invece di essere una epic nuova.

### 5.3 Predictive Actions e Trap — `OPEN`, copertura parziale

`brief-delayed-actions.md` copre le Delayed Actions (decise in Planning, risolvono a un boundary). Il sorgente
chiede la distinzione **a tre**, ed è una distinzione semantica che il repo non ha ancora scritto:

| Pattern | Decisione | Vive | Nel repo |
|---|---|---|---|
| **Predictive Action** | tutta in Planning | dentro il turno | `brief-delayed-actions.md` |
| **Trap / effetto persistente** | tutta in Planning | **oltre** il turno, con charge, rilevabilità, disarmo, distruzione | **assente** |
| **Fast Reaction** | al boundary | finestra interattiva | ADR-0004 / E14 |

Il pezzo mancante non è «le mine». È il **trigger su arco** (`CrossEdge(H6,H7)` distinto dall'ingresso in H7 da
un altro lato) e il fatto che una trap possa **modificare il grafo tattico**. Entrambi toccano E9 (porte, ponti,
`ModifyArc`, revisione del grafo) — che è aperta e non ancora costruita. La finestra per decidere se gli archi
sono entità di prima classe **si chiude con E9**.

**Azione proposta**: nessuna epic. Una nota vincolante dentro la spec di E9 («gli archi devono poter portare
trigger, non solo costo e transizione»), e il resto in un brief.

### 5.4 ⛔ Formato principale — `CONFLICT` bloccante

| Fonte | Dice |
|---|---|
| **D-001**, Decision Log, *Consolidata* | «Formato principale **3v3**; vertical slice 2v2» |
| **D-010** / `spec-durata-partita-e-scala-mappe.md` | tutte le bande temporali sono tarate su **3v3 Standard** (25–30 min, `RoundLimit` 16–20, planning 40–45 s) |
| `..._NuoveDecisioni_4v4_...md` §5 | «la discussione recente introduce esplicitamente un **4v4 con tutti e quattro i personaggi della vertical slice per squadra**» |
| `..._Consolidamento_PRD_...md` §22 | «Formato principale target: **3v3**. Scenario 4v4: supportabile, per stress/design test, **non** aumenta lo scope v0.1» |

I due documenti sorgente della **stessa giornata si contraddicono**. Uno dei due chiede di bloccare il formato
con una issue di design; l'altro lo dà per deciso a 3v3.

**Questo è il conflitto che il sorgente stesso vieta di risolvere in silenzio** (§33: «non trasformare una
proposta in una decisione canonica solo perché sembra buona»). Resta `OPEN DECISION` **OD-1**.

Ciò che si può dire senza decidere: il 4v4 **come scenario di stress** non è in conflitto con nulla — è un
banco di prova per resolver, leggibilità con 8 unità e frequenza dei prompt di reazione, e va bene in roadmap
dopo E15. Il 4v4 **come formato principale** invalida le bande di D-010 e va deciso da una persona.

---

## 6. Fase C — modello canonico: struttura riconciliata

La struttura target di §24 e quella esistente **descrivono lo stesso sistema**. La mappatura:

| §24 propone | Nel repo è già | Verdetto |
|---|---|---|
| `docs/README.md` | — | 🆕 **da creare** (D2) |
| `DOC_CONFLICT_MATRIX.md` | — | §3 di questo documento; da promuovere solo se serve manutenerla nel tempo |
| `OPEN_DECISIONS.md` | `PDR/RT_PDR_00_Decision_Log.md` (stati `Consolidato/Assunzione/Proposta/Open question`) | **esiste già**, con più struttura di quella proposta |
| `CHANGELOG_DOCUMENTATION.md` | i blocchi «⚠️ Revisione *data*» dentro ogni documento | equivalente, e **migliore**: il changelog sta accanto alla regola che cambia |
| `product/GAME_VISION · MATCH_STRUCTURE · VERTICAL_SLICE` | `README.md` · `piano-canonico-mvp.md` · `roadmap-v0.1.md` §1 | esiste |
| `gameplay/TURN_STRUCTURE · ACTION_SYSTEM · REACTIONS · OVERWATCH · DELAYED_ACTIONS · MOVEMENT · VISIBILITY · NOISE · ENVIRONMENT · CHARACTERS` | `spec-sequenza-turno` · `spec-motore-azioni-e4` · `spec-reazioni-componibili-cp55` · `brief-overwatch-reazioni` · `brief-delayed-actions` · `spec-pathfinding-pf3-pf4` · `brief-conoscenza-parziale` (vista **e** udito) · `spec-terreni-e8` · `balance/RT_HeroCatalog` | esiste, con nomi diversi |
| `gameplay/AUXILIARY_UNITS` | — | 🆕 §5.1 |
| `technical/ARCHITECTURE · SIMULATION · NETWORKING · MAP_GRAPH · PATHFINDING · DATA · UI_UX · AUTOMATED_TESTING` | `architettura-codice` · `piano-canonico` §5 · invariante #6 · `spec-mappa-multilivello` · `spec-pathfinding-pf3-pf4` · `convenzioni-contenuti-ue` · `HUD/progettazione-hud` · `test-automatico-unreal` | esiste |
| `technical/ABILITIES_GAS` | — | non serve: **no-GAS** è decisione canonica |
| `balance/BALANCE_MODEL · PARAMETERS` | `balance/` (4 cataloghi + matrice test) + workbook | esiste, **più granulare** |
| `roadmap/MILESTONES · V0_1_SCOPE` | `roadmap-checkpoint.md` · `roadmap-v0.1.md` | esiste |
| `decisions/` | `adr-000{1..5}` in `design/` | esiste |
| `archive/pdr-v0.1/` | `archive/` + `PDR/` (snapshot dichiarati) | esiste |

**Raccomandazione**: **non** riorganizzare le cartelle. Il valore della struttura di §24 è la *ownership del
concetto* — «un documento è owner, gli altri linkano» — e quella si ottiene con un indice, non con uno
spostamento di file. Il costo di uno spostamento è ~200 link interni e la perdita del `git log --follow` sui
file più letti del progetto.

Si adotta invece **la regola**, non la cartella: `docs/README.md` diventa la tabella *concetto → documento
owner*, e questo rende immediatamente visibile ogni duplicazione futura.

---

## 7. Revisione del panel sul documento sorgente

`--mode critique` · focus `requirements, architecture`.

**KARL WIEGERS — qualità dei requisiti**
> ❌ **CRITICO**: §24 prescrive una struttura di file senza un criterio di accettazione. «Portare gradualmente
> la repository verso una struttura simile» non è verificabile: *simile* quanto? Un requisito di riorganizzazione
> deve dichiarare cosa si rompe se non lo si applica. Qui non si rompe niente — il che è la prova che il
> requisito reale era un altro, ed è §34 («un nuovo sviluppatore apre `docs/README.md` e capisce»). Quello **è**
> testabile.
> 📝 Sostituire §24 con §34 come requisito primario. La struttura di cartelle diventa un mezzo opzionale.

**ALISTAIR COCKBURN — attore e obiettivo**
> ⚠️ Il documento non ha un attore primario dichiarato. Scritto per «Claude Code», ma il beneficiario di §34 è
> *un nuovo sviluppatore*, e quello di §29 (issue granulari) è *il maintainer*. Sono due obiettivi con costi
> molto diversi: il primo si soddisfa con un file, il secondo con settimane. Averli nello stesso documento è ciò
> che fa sembrare il lavoro enorme quando l'80% è già fatto.

**MARTIN FOWLER — ownership e confini**
> ✅ §Fase C è la parte migliore del documento: «un documento è owner del concetto, gli altri linkano». È la
> regola giusta, ed è l'unica che il repository **non** applica sistematicamente.
> ⚠️ Ma §24 la contraddice: una struttura per *categoria* (`gameplay/`, `technical/`) taglia trasversalmente
> l'ownership. Il rumore è gameplay, percezione, rete e UI insieme — in quale cartella va? Nel repo attuale sta
> in un solo brief, con l'owner giusto. La cartella lo separerebbe.

**MICHAEL NYGARD — modi di fallimento**
> ❌ **CRITICO**: il documento non prevede il modo di fallimento che si è **già verificato** — un documento
> normativo che sopravvive alla propria smentita (difetto D1: il canone cita 4 classi rimosse). §Fase G
> elenca «broken Markdown links» ma non «riferimenti a simboli di codice inesistenti», che è il caso che fa
> davvero danno perché il documento resta leggibile e sbagliato.
> 📝 Aggiungere un controllo automatizzabile: i nomi di classe citati nei documenti normativi devono esistere
> in `Source/`. È un `grep` in CI, ed è l'unico gate di questa lista che si può automatizzare davvero.

**GOJKO ADZIC — testabilità**
> ⚠️ §32 chiede 14 output; §30 dà 10 criteri di Done. Nessuno dei due dice **quando il consolidamento è finito**.
> §32 di `..._NuoveDecisioni_4v4_...md` ci arriva più vicino con 16 caselle — ed è quella la lista da usare,
> perché 12 delle 16 si possono verificare con un `grep`.
> 💡 Esempio concreto di criterio verificabile, nello stile del repo:
> `Given` un documento normativo di `docs/` `When` cita un identificatore `URT*`/`ART*`
> `Then` quell'identificatore esiste in `Source/`.

**LISA CRISPIN — strategia di verifica**
> ✅ §20 (Automated Scenario Test Harness) è il requisito con il ritorno più alto del documento, ed è
> **già in costruzione** (`50159c6`). Il vincolo che lo rende utile è dichiarato bene: «il test deve usare il
> percorso gameplay reale, non `SetActorLocation`/`ApplyDamage`».
> ⚠️ §22.2 elenca 18 scenari minimi di cui **10 riguardano sistemi che non esistono**. Elencare test per
> feature assenti è il modo più veloce di rendere una matrice di test non affidabile: dopo tre voci ⏳ nessuno
> la legge più. Il repo lo evita già distinguendo «test previsti dal catalogo» da «test esistenti, misurati».

**JANET GREGORY — chi ha partecipato**
> Il documento è un prompt a senso unico: prescrive «segnala i conflitti prima di risolverli» (§Fase B) ma
> non prevede un momento in cui una **persona** decide. OD-1 (§5.4) è esattamente quel caso, e senza una
> risposta umana il consolidamento non può chiudersi.

---

## 8. Decisioni aperte

Prodotte qui, **promosse** a documento proprio: [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).

`OD-1` formato principale 3v3 *vs* 4v4 (**bloccante**) · `OD-2` unità ausiliarie · `OD-3` costo-opportunità e
policy dell'Overwatch · `OD-4` trigger sugli archi del grafo · `OD-5` scenario 4v4 di stress.

Nessuna era stata chiusa dalla revisione: sono i punti in cui il sorgente stesso vieta di scegliere per
plausibilità (§33).

> ✅ **Tutte e cinque chiuse il 2026-08-07** dalla sessione `/sc:brainstorm` successiva — `D-011`, `D-012`,
> `D-013`, due brief e l'epic **E17**. Due erano **mal poste**, e lo ha detto il codice: vedi la nota di metodo
> in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md). In particolare `OD-1` **non era bloccante** e `OD-4` non aveva
> la scadenza legata a E9 che questo documento le attribuisce in §5.3 e §8.

## 9. Piano di migrazione — **eseguito**

Ordinati per rapporto valore/rischio quando furono proposti. **Tutti e otto eseguiti** fra il 2026-08-07 e la
sessione `/sc:brainstorm` dello stesso giorno; la colonna *Esito* registra dove il piano ha dovuto cambiare.

| # | Checkpoint | DoD misurabile | Esito |
|---|---|---|---|
| **C.1** | **Riallineare il canone al codice** | `piano-canonico-mvp.md` §5 non cita più `URTGridLibrary`, `URTTurnResolver`, `ARTGameState`, `URTAbilityData`; la tabella elenca le classi **verificate** (`ARTGameMode`, `ARTTurnManager`, `ARTUnit`, `ARTPlayerController`, `ARTCameraPawn`, `URTCombatLibrary`, `URTActionData`, `URTHeroData`, `URTHexMapAsset`, `URTMatchFormatData`). Riferimenti a `URTAbilityData` corretti anche in `CLAUDE.md` e `README.md` | ✅ **fatto**, ed **esteso**: anche l'invariante #2 citava `FRTGridCoord`, rimosso al CP 6.1 |
| **C.2** | **`docs/README.md` — indice per owner del concetto** | Tabella *concetto → documento owner → stato*, che copre le 13 domande di §34 del sorgente. Nessun concetto con due owner. Link verificati | ✅ **fatto** |
| **C.3** | **Igiene dei riferimenti** | Link `SuperClaude_...` corretto (D3); §6.1 di `brief-delayed-actions.md` e §11 di `brief-conoscenza-parziale.md` corretti (D4, D5); i 3 sorgenti untracked versionati (D6); i documenti del substrato quadrato marcati (D7) | ✅ **fatto**, con una correzione: dei 7, solo **4** erano davvero superati — uno è un piano consegnato, tre restano normativi con esempi datati. Più **D8**: `progettazione-hud.md` referenziava un PNG inesistente |
| **C.4** | **Gate automatico anti-deriva** | Uno script (`scripts/check-docs-symbols.*` o test Automation) che fallisce se un documento normativo cita un identificatore `URT*`/`ART*` assente da `Source/`. È la proposta di Nygard, ed è l'unico controllo di §Fase G realmente automatizzabile | ✅ **fatto**, e **due volte riscritto** dopo test di mutazione: contava le occorrenze invece delle dichiarazioni, e segnalava troppo. Vedi [`CHANGELOG_DOCUMENTATION.md`](../../CHANGELOG_DOCUMENTATION.md) |
| **C.5** | **Brief delle tre aree aperte** | `brief-unita-ausiliarie.md`, `brief-azioni-generiche-overwatch.md`, `brief-predictive-e-trap.md`: ognuno con perimetro, fuori scope dichiarato, decisioni | 🟡 **due su tre**: unità ausiliarie e azioni generiche. Il terzo non serve più — `D-013` ha chiuso il tema in [`gameplay/brief-delayed-actions.md`](../../gameplay/brief-delayed-actions.md) §6-bis |
| **C.6** | **Vincoli anticipati sulle epic aperte** | `ResolutionPolicy` (`Automatic`/`Conditional`/`FastSelect`) entra nel DoD di **CP 14.3**; la nota «gli archi devono poter portare trigger» entra nella spec di **E9**; l'assunzione «ogni unità è uno dei 4 eroi» è vietata nei tipi introdotti da E13/E14/E16 | ✅ **fatto**, con una correzione: `ResolutionPolicy` **non** diventa un enum — la roadmap aveva già deciso il contrario, i tre regimi emergono da `AllowedResponses` + condizione dichiarata |
| **C.7** | **Chiusura di OD-1/OD-2** | Il Decision Log guadagna `D-011` (formato principale) con stato esplicito; la risposta è stata **«nessuna delle due»**: D-001 declassata ad *Assunzione da bloccare* | ✅ **fatto** — `D-011` |
| **C.8** | **Scenario 4v4 di stress in roadmap** | epic dopo E15 con quell'exit gate | ✅ **fatto** — **E17**, 3 checkpoint, P3 |

**Quello che il piano dichiarava di NON fare** — e che poi è stato fatto, per decisione dell'utente: la
riorganizzazione delle cartelle (§6 la sconsigliava) e i tre file di governance (`OPEN_DECISIONS.md`,
`DOC_CONFLICT_MATRIX.md`, `CHANGELOG_DOCUMENTATION.md`), che §6 riteneva già coperti da equivalenti.
Regge invece la terza rinuncia: **nessuna epic aperta** per Delayed, Trap o Auxiliary. L'unica epic nuova è
**E17**, che è una *misura*, non produzione.

---

## 10. Issue proposte

Prima di aprirle: le 62 issue esistenti sono state confrontate: nessuna copre queste aree.

| Titolo proposto | Tipo | Corrisponde a |
|---|---|---|
| `[DOCS] Il canone cita quattro classi rimosse: riallineare piano-canonico §5 al codice` | difetto | C.1 |
| `[DOCS] docs/README.md — indice per owner del concetto` | docs | C.2 |
| `[DOCS] Igiene dei riferimenti: link rotti, avvisi obsoleti, spec quadrate non marcate` | difetto | C.3 |
| `[TEST] Gate anti-deriva: i documenti normativi non citano simboli inesistenti` | test | C.4 |
| `[DESIGN] Unità ausiliarie: concetto unico e vincoli da rispettare nei tipi` | design | C.5 |
| `[DESIGN] Azioni generiche e Overwatch universale: profili e ResolutionPolicy` | design | C.5/C.6 |
| `[DESIGN] Predictive Actions e Trap: distinguere i tre pattern, trigger su arco` | design | C.5/C.6 |
| `[DESIGN] Bloccare il formato principale: 3v3 o 4v4, e modello di controllo` | decisione | C.7 / **OD-1** |
| `[DESIGN] Scenario 4v4 mirror come validazione di stress` | design | C.8 / **OD-2** |

---

## 11. Rapporto con gli altri documenti

| Documento | Relazione |
|---|---|
| [`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) | **Prevale**. C.1 lo corregge, non lo sostituisce |
| [`roadmap-v0.1.md`](../../roadmap/roadmap-v0.1.md) | I checkpoint di §9 vi entrano solo se approvati; §8 (rischio scope `H/H`) è il motivo per cui §9 apre pochissimo |
| [`../PDR/RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md) | **Owner** delle `OPEN DECISIONS`: OD-1 diventa `D-011` lì, non qui |
| [`brief-delayed-actions.md`](../../gameplay/brief-delayed-actions.md) · [`brief-overwatch-reazioni.md`](../../gameplay/brief-overwatch-reazioni.md) · [`brief-conoscenza-parziale.md`](../../gameplay/brief-conoscenza-parziale.md) | Coprono già Delayed, Overwatch e percezione: questo brief **non li ripete**, ne registra i difetti D4/D5 |
| `../archive/src/handoff/consolidamento-prd-source-of-truth.md` | Sorgente di questa revisione. Resta in `src/` come input, **non** diventa normativo. Il link diventa attivo con **C.3** (oggi il file è untracked) |
