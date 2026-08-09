# Consolidamento `todo/consolidazione-chat-openai/` — triage

> `CURRENT` · **Stato**: audit chiuso · **cinque cluster su cinque consumati** — PR `#312`, `#317`, `#321`,
> questa, e `#349` che ha chiuso il residuo Characters (`#336`) il 2026-08-09.
> ⬜ **Resta il master Common Actions**: la decisione che lo bloccava è presa (§8.2), il recepimento no.
> **HEAD dell'audit**: `75eb0f3`; l'esecuzione ha seguito `main` fino a `b7368b8`
> **Sorgente auditata**: i 14 file di `todo/consolidazione-chat-openai/`. **Dal 2026-08-10 sono versionati**:
> dodici archiviati in [`archive/src/README.md`](../../archive/src/README.md) §pacchetto, uno era il duplicato
> di un kit già archiviato, e Common Actions resta fuori finché nessun owner lo recepisce.
> **Scopo**: classificare ogni affermazione dei sei *master* e dei due *kit* contro le source of truth reali,
> **prima** di toccare Decision Log, ADR, Feature Registry, roadmap, Wiki o scenari.
> **Regola applicata**: un handoff AI è l'ultima fonte della gerarchia. Dove contraddice un ADR o una `D-0xx`
> accettata, prevale il canone e la proposta si **registra**, non si applica.

---

## 1. Cosa contiene il pacchetto, e cosa è già stato assorbito

Il pacchetto non è omogeneo: sono **due generazioni** di documenti sovrapposte.

| File | Tipo | Stato rispetto al repository |
|---|---|---|
| `RT_Reaction_System_Master_Consolidation_v0.1.md` | master rivisto | ✅ **assorbito** — PR `#305`, `D-047`/`D-048`/`D-049`, CP 14.7 |
| `RefactorTactics_ReactionSystem_ReactionClash_...md` | kit | ✅ assorbito nella stessa PR |
| `RefactorTactics_Move_Consolidation_Claude.md` | kit | ✅ assorbito — PR `#304` |
| `RefactorTactics_BasicAttack_Consolidation_...md` | kit | ✅ assorbito — [ADR-0007](../../decisions/adr-0007-attacco-base-per-eroe.md) |
| `RefactorTactics_Decision_Time_Bank_Claude_...md` | kit | ✅ assorbito — [`spec-decision-time-bank.md`](../../gameplay/spec-decision-time-bank.md) + conflict report |
| `RT_Common_Actions_Master_Consolidation_v0.1.md` | master | ⬜ **l'unico rimasto** (resta tutto tranne Move e BasicAttack) — vedi §8.2 |
| `RT_Characters_Roster_Master_Consolidation_v0.1.md` | master | ✅ consumato — PR `#321`, poi il residuo in `#349` (`#336`) |
| `RT_Map_Environment_Master_Consolidation_v0.1.md` | master | ✅ consumato — [`spec-interazioni-mappa-cp101.md`](../../gameplay/spec-interazioni-mappa-cp101.md) |
| `RefactorTactics_Interactive_Map_Elements_...md` | kit | ✅ consumato nella stessa sessione (dettaglio del precedente) |
| `RT_UI_UX_Master_Consolidation_v0.1.md` | master | ✅ consumato — PR `#317` |
| `RefactorTactics_HUD_Consolidation_Claude.md` | kit | ✅ consumato con riserva (dettaglio del precedente): 3 dei 7 conflitti nascono qui |
| `RT_Scenarios_QA_Bots_Master_Consolidation_v0.1.md` | master | ✅ consumato — il più assorbito dei tre |
| `RT_Governance_Master_Consolidation_v0.1.md` | master | ✅ consumato — PR `#312` |
| `RT_Chat_Cleanup_Tracker.md` (×2) · `RT_Final_Chat_Cleanup_Plan_v0.1.md` | meta | ⬜ riguardano il progetto ChatGPT, **non** il repository — archiviati per provenienza |

**I due `RT_Chat_Cleanup_Tracker` differiscono**: il secondo (`(1)`) è più recente — aggiunge Map e UI ai
cluster completati e cinque conflitti che il primo non ha. Il primo va ignorato.

### La differenza che conta

I **master** (`RT_*_Master_*`) sono stati scritti contro una fotografia del progetto ferma al **2026-08-08**.
I **kit** (`RefactorTactics_*_Claude_*`) sono più vecchi ancora: HUD e Interactive Map precedono l'intero
lavoro di governance. Fra il 2026-08-08 e oggi sono atterrate `D-039`…`D-047`, il `feature-registry.yaml`
canonico e la decisione sull'identità degli scenari (`#209`).

Conseguenza operativa: **il pacchetto non si applica, si filtra.** Diverse sue "decisioni" sono domande già
chiuse, e almeno una applicherebbe a ritroso una tassonomia superata.

---

## 2. Sintesi della classificazione

| Classificazione | Voci | Significato |
|---|---|---|
| `CURRENT` | 21 | il pacchetto riporta correttamente il canone |
| `PROPOSED` | 14 | idea nuova, nessun conflitto: si registra o si costruisce |
| `CONFLICT` | 7 | contraddice una decisione accettata |
| `STALE` | 6 | usa una formulazione superata da una decisione più recente |
| `DUPLICATE` | 5 | ridefinisce qualcosa che ha già un owner canonico |

Tre osservazioni valgono più delle singole righe:

1. **Sette dei nove "conflitti aperti" dichiarati dal tracker sono già chiusi nel repository.** Non erano
   conflitti: erano voci che nessuno aveva riconciliato con il Decision Log. Vedi §4.
2. **Il pacchetto propone cinque convenzioni di ID diverse**, nessuna delle quali è quella del progetto.
   Applicarle creerebbe esattamente il registry parallelo che il Governance Master §39 vieta.
3. **L'audit ha trovato un difetto reale del repository**, indipendente dal pacchetto: `D-039` è implementata
   nel codice e **contraddetta da tre documenti**. Vedi §5.

---

## 3. Conflitti — le sette voci che contraddicono il canone

| # | Tema | Cosa dice il pacchetto | Cosa dice HEAD | Fonte che prevale | Azione |
|---|---|---|---|---|---|
| 1 | **Tassonomia delle azioni generiche** | `ACTION-TAXONOMY-01`: «6 vs 8, baseline **8**», con `Guard` **e** `Activate` universali | **sette** voci: `Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch`; `Activate` **assorbita** da `Interact` | [`D-025`](../../decisions/RT_PDR_00_Decision_Log.md) *(emenda `D-014`)* · [`DOC_CONFLICT_MATRIX` #27](../../DOC_CONFLICT_MATRIX.md) | **non applicare.** Il conflitto è già registrato e **chiuso**: il pacchetto lo riapre citando una `D-AUDIT-01` che non esiste nel Decision Log |
| 2 | **Identità degli scenari** | Master Scenarios §5: una `PrimaryCategory` obbligatoria + `PurposeTags` secondari | **un asse solo**: `scenarioId` puntato + tag liberi, ID **staccato** dal percorso, indice che rifiuta gli ID ambigui | [`scenario-index-e-tag.md`](../../technical/scenario-index-e-tag.md), deciso 2026-08-08 (`#209`) | **non applicare.** La motivazione è scritta: separare tipologia e lente costringe a decidere per ogni parola in quale casella vive, e «la risposta onesta è spesso *entrambe*» |
| 3 | **Vocabolario di status** | Governance §5 propone 13 status (`IMPLEMENTED`, `IMPLEMENTED_PARTIAL`, `DATA_SPEC`, `FUTURE`, `HISTORICAL`…) | 10 status, **derivati dai gate** con regola deterministica e verificati dal validator | [`feature-registry.yaml`](../feature-registry.yaml) intestazione | **non applicare**: romperebbe `feature_registry.py validate`. I **gate** invece coincidono alla lettera (9 su 9) — quella parte è `CURRENT` |
| 4 | **`TEAM READY 2/3`** | Kit HUD §3.5 lo prescrive come componente persistente | «non simulare un falso stato `TEAM READY 1/2` finché non è realmente supportato» | [`progettazione-hud.md:341`](../../technical/progettazione-hud.md) · UI Master §15 `UI-READY-01` | **non applicare.** Il conflitto è **interno al pacchetto**: il master corregge il kit. Prevale il master, che coincide col repository |
| 5 | **Fog of War** | Kit HUD §11/§30/§31 struttura tre milestone attorno alla «Fog of War» | **non è FoW**: conoscenza parziale a tre livelli, geometria statica **nota** | [`DOC_CONFLICT_MATRIX` #13](../../DOC_CONFLICT_MATRIX.md) · [`brief-conoscenza-parziale.md`](../../gameplay/brief-conoscenza-parziale.md) D1 | **rinominare in sede di recepimento.** Anche qui il Map Master §29 corregge il kit |
| 6 | **Eleggibilità per nome d'eroe** | Kit HUD §15: `Eligible: Flux / Bastion` nel pannello interazione | le abilità hanno ownership singola, le sinergie sono **derivate**; nessuna dipendenza da `HeroId` | [`ADR-0006`](../../decisions/adr-0006-ownership-abilita-sinergie.md) · [`D-029`](../../decisions/RT_PDR_00_Decision_Log.md) | **non applicare.** Il modello corretto — capability, non nome — è nel Map Master §15-§18 e nel kit Interactive Map §3 |
| 7 | **Schema della pagina personaggio** | Characters Master §20 propone 11 sezioni Wiki | il template ha 16 sezioni e un campo **obbligatorio** che il master non nomina: `Misplay / Failure State` | [`D-032`](../../decisions/RT_PDR_00_Decision_Log.md) · [`characters/_Template.md`](../../characters/_Template.md) | **non applicare** lo schema. Le 4 pagine v0.1 lo compilano già: sostituirlo perderebbe un criterio anti-clone |

---

## 4. I «conflitti aperti» del tracker — sette su nove erano già chiusi

Il `RT_Chat_Cleanup_Tracker(1).md` elenca nove conflitti da non perdere. Confrontati con HEAD:

| ID del tracker | Stato reale | Dove è chiuso |
|---|---|---|
| `ACTION-TAXONOMY-01` | ✅ **chiuso** — e il tracker propone la baseline **sbagliata** | [`D-025`](../../decisions/RT_PDR_00_Decision_Log.md) |
| `ROSTER-01` | ✅ chiuso | [`D-037`](../../decisions/RT_PDR_00_Decision_Log.md) · [`DOC_CONFLICT_MATRIX` #12](../../DOC_CONFLICT_MATRIX.md) |
| `BALANCE-WORKBOOK-01` | ✅ chiuso — il workbook **è** `RESEARCH` | [`D-023`](../../decisions/RT_PDR_00_Decision_Log.md) |
| `HIGHGROUND-01` | ✅ chiuso — ed è una **feature con scenario**, non una lacuna | `RT-FEAT-MAP-HIGH-GROUND` (`INTEGRATED`) · `Visual.Map.HighGroundNoBonus` |
| `TIMEBANK-01` | ✅ chiuso il 2026-08-09 | `spec-decision-time-bank.md` (CP 14.8) — *sessione parallela, non ancora committato* |
| `UI-REACTION-01` | ✅ chiuso — la reaction **non** è una quinta fase | CP 11.5, test `Preview.ReactionIsNotAPhaseEntry` |
| `UI-GHOST-PRIVACY-01` | ✅ chiuso | invariante **#6** · CP 11.2 |
| `FACING-01` | 🟡 **realmente aperto**, e il tracker lo classifica bene | `FAC-1` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md): le matrici `MoveEndPivotMaxSteps` per eroe restano proposal |
| `NOISE-SCOPE-01` · `MAP-SCOPE-01` | 🟡 aperti come **scope**, non come contraddizione | `RT-FEAT-PERCEPTION-NOISE` è `SPECIFIED` (E13); ogni feature ambientale ha già uno status per riga |

**Perché conta.** Un handoff che ripropone come aperte sette decisioni chiuse invita a ridecidere ciò che è
stato deciso — è lo stesso difetto già registrato in `OPEN_DECISIONS.md` per il consolidamento facing, dove
cinque domande su quindici erano già risposte. Il pacchetto va letto sapendolo.

---

## 5. Difetto del repository trovato durante l'audit — `D-046` non è propagata

Indipendente dal pacchetto, ma scoperto perché il Map Master §11 ripete l'affermazione vecchia.

[`D-046`](../../decisions/RT_PDR_00_Decision_Log.md) (2026-08-09) stabilisce che **`Flux.ConductiveNode` *è*
`Action.Electrify`**, ed è **cablata nel codice**:

```
Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp:172
// D-046: cablata su `Action.Electrify`.
```

Tre documenti dicono ancora l'opposto, in quattro punti:

| File | Riga | Testo |
|---|---|---|
| [`feature-registry.yaml`](../feature-registry.yaml) | 1638 | `wiki_note`: «nessun eroe della v0.1 ha `Action.Electrify` come abilita' normale» |
| [`wiki/meccaniche/acqua-e-elettricita.md`](../../wiki/meccaniche/acqua-e-elettricita.md) | 11 · 71 | stessa affermazione, propagata dal `wiki_note` e ripetuta in prosa |
| [`characters/v0.1/flux.md`](../../characters/v0.1/flux.md) | 26 | stessa affermazione, nella pagina dell'eroe che **ora la possiede** |

La riga 1638 è la **sorgente**: le altre sono propagazione. Il difetto è di classe nota — *dato che nessuno
legge* rovesciato in *decisione che nessuno propaga* — e va corretto alla sorgente, non copia per copia.

⚠️ **Non correggere in questa sessione senza coordinamento**: `feature-registry.yaml` è fra i file modificati
dalla sessione parallela.

---

## 6. Cinque convenzioni di ID, nessuna delle quali è la nostra

| Sorgente | ID proposti | Esiste nel repository? |
|---|---|---|
| Kit HUD §28 | **38** feature `HUD.Core`, `HUD.TimeBank`, … | ❌ nessuna. Il registry usa `RT-FEAT-UI-*` |
| UI Master §30 | 15 `RT-FEAT-UI-*` | **6 sì, 9 no**; e ne **omette due che esistono** (`RT-FEAT-UI-ICON-LANGUAGE`, `RT-FEAT-UI-SCENARIO-BROWSER`) |
| Scenarios Master §30 | 11 `RT-FEAT-BOT-*` | **2 sì** (`RT-FEAT-BOT-BASE`, `RT-FEAT-BOT-TACTICAL`), 9 no |
| Kit Interactive Map §25 | 15 `MAP-INTERACTION-*` | ❌ nessuna. Esistono `RT-FEAT-MAP-*` (8 delle quali il Map Master cita **correttamente**) |
| Scenari | `ACTION-001`, `MOVE-001`, `FACE-001`, `MAP-001`, `ENV-001`, `INTERACT-001`, `NOISE-001`, `HUD-001`, `SCN-HUD-001`, `MAP-INTERACT-001` | ❌ la convenzione è puntata: `Visual.Map.HighGroundNoBonus`, `Spec.Perception.HeardNotSeen` |

Il registry canonico ha **83 feature**. Il pacchetto ne proporrebbe una settantina di nuove, quasi tutte
ri-etichettature di capability già inventariate a granularità diversa.

**Regola per il recepimento**: nessun `feature_id` nuovo senza aver prima cercato quello esistente; nessuno
`ScenarioId` che non segua `scenario-index-e-tag.md`; nessun numero di issue inventato.

Nota positiva, e non scontata: i **nove gate** del Governance Master §4 (`spec · data · runtime · log_debug ·
automation · scenario · ui_wiki · packaged · network_privacy`) coincidono **alla lettera** con quelli del
registry. Quella sezione si può citare, non riscrivere.

---

## 7. Cosa il pacchetto aggiunge davvero — le proposte che valgono

Filtrato il rumore, resta materiale che il repository **non** ha:

| # | Proposta | Dove vive oggi | Perché vale |
|---|---|---|---|
| 1 | **Modello capability/verb per gli elementi di mappa** — `Element → State · Capabilities · Verbs · Requirements · Effects` | nulla di equivalente; E10 CP 10.1 fa `Activate`/`Interact` su oggetto adiacente, senza grammatica | È la forma corretta della regola già decisa da `ADR-0006`: sostituisce il branch per eroe con un requisito dichiarato. Owner naturale: E9/E10 |
| 2 | **State machine dichiarata per elemento** (`Generator: Off → Online → Overloaded → Destroyed`) | porte e ponti hanno spec proprie ([`spec-porte-cp93`](../../gameplay/spec-porte-cp93.md), [`spec-ponti-cp94`](../../gameplay/spec-ponti-cp94.md)), ma nessun modello comune | Generalizza due casi già costruiti invece di inventarne un terzo |
| 3 | **Regola delle tre soluzioni** (un'affordance importante offre ≥3 approcci sensati) | assente | Guideline di level design con una motivazione verificabile: evita il personaggio obbligatorio. Da registrare come criterio, non come regola di runtime |
| 4 | **Separazione griglia tattica / architettura fisica** con `AffectedTransitions[]` esplicite | [`spec-mappa-multilivello.md`](../../technical/spec-mappa-multilivello.md) copre il grafo, non il binding delle strutture | Impedisce che il resolver legga la geometria. ⚠️ ma vedi §8.1 sulla scala in metri |
| 5 | ~~`Wet(unit) ≠ Conductive(cell)` come invariante nominata~~ | ✅ **ha già un nome**: è `D2` di [`spec-propagazione-elettrica-cp83.md`](../../gameplay/spec-propagazione-elettrica-cp83.md), «la conduzione è della CELLA, mai dello stato dell'unità», con il test `Environment.Propagation.StopsAtNonConductive` | **Scartata dopo verifica.** Il pacchetto propone `ENV-001` come sigla nuova: sarebbe un secondo identificatore per una regola che ne ha già uno — lo stesso difetto che `D-033` ha respinto per `GenericActionModifier` |
| 6 | **Warning centralizzato con severità** `Info · Warning · Block` e sorgente dichiarata | CP 11.6 richiede che i warning vengano dai reason code, non ne classifica la severità | Completa un DoD già scritto |
| 7 | **Outcome Explanation** — inspector dell'evento distinto dal Combat Log | CP 11.3 ha il log con reason code; non ha il «perché è fallita» | Estensione naturale, stesso dato |
| 8 | **Acoustic mask e memoria sonora con decadimento** | `brief-conoscenza-parziale.md` copre `UltimoContatto`; il masking no | Coerente con `D-044`/`D-045`, che hanno appena fissato la scala 0–10 |
| 9 | **Trigger geografici vs semantici** per le reaction (`EnemyEnterCell` vs `EnemyUsesSprint`) | `spec-tassonomia-movimento.md` ha i profili, non i trigger semantici | Materiale per E14, non per la v0.1 |

---

## 8. Le tre domande che richiedevano l'autore — chiuse il 2026-08-09

Nessuna era decidibile dai documenti. Tutte e tre sono state decise dall'autore in sessione.

### 8.1 · La scala della mappa si misura in metri?

Il Map Master §2 introduce **`lato esagono ≈ 1,5 m`** come baseline di authoring.
[`spec-durata-partita-e-scala-mappe.md`](../../gameplay/spec-durata-partita-e-scala-mappe.md) dichiara
l'opposto in modo esplicito: «**Metrica primaria — non i metri, non il numero assoluto di celle**», e
`D-030` ribadisce che il sorgente «non fissa un numero di celle».

Non è una contraddizione piena — una scala di authoring per le mesh non è una metrica di design — ma è un
**numero nuovo** in un documento che aveva deciso di non averne. O si dichiara come scala d'arte con quel
nome, o non entra.

> ✅ **Deciso il 2026-08-09 dall'autore: entra come *scala d'arte*, con quel nome.** Vive nella pipeline dei
> contenuti, non nei documenti di design, e non diventa una metrica di progetto: la metrica primaria resta
> **temporale**. Chi dimensiona una mesh ha un riferimento; chi dimensiona una mappa continua a contare i
> Move. Owner: [`convenzioni-contenuti-ue.md`](../../technical/convenzioni-contenuti-ue.md).

### 8.2 · `Activate` sopravvive come Stable ID: fino a quando?

`D-025` ha chiuso la **semantica** (sette azioni, `Activate` assorbita), non la **migrazione**.
Oggi convivono:

- `Action.Activate` a catalogo, barrato ma vivo, «ancora consumato dal codice»;
- CP 10.1 intitolato «`Activate` / `Interact`», con test `Objectives.ActivateAdjacentOnly`;
- [`spec-motore-azioni-e4.md`](../../gameplay/spec-motore-azioni-e4.md) che parla ancora di «sei azioni fondamentali».

Il pacchetto **spinge nella direzione sbagliata**: propone `Activate` come ottava azione universale. Prima di
recepire il cluster Common Actions serve sapere se la migrazione degli Stable ID entra nella v0.1 o resta
dichiarata. È la stessa domanda aperta di `DOC_CONFLICT_MATRIX` #27.

> ✅ **Deciso il 2026-08-09 dall'autore: la migrazione resta *dichiarata*, i documenti si allineano.**
> `Action.Activate` sopravvive come Stable ID consumato dal codice — cancellarlo è lavoro di codice, non di
> documentazione, e non ha un checkpoint. Si correggono invece i testi che lo presentano come **azione
> universale**, perché è lì che nasce il conflitto: un lettore che apre CP 10.1 o
> [`spec-motore-azioni-e4.md`](../../gameplay/spec-motore-azioni-e4.md) legge una tassonomia superata da
> `D-025`. La riga #27 di `DOC_CONFLICT_MATRIX` resta aperta **sulla sola migrazione**, che è ciò che
> davvero manca.

### 8.3 · Quale cluster si consolida per primo?

I quattro rimasti hanno costi e rischi diversi:

| Cluster | Costo | Rischio di collisione | Valore immediato |
|---|---|---|---|
| **Map & Environment** + Interactive Map | alto | basso | alto — sblocca la grammatica di E9/E10 |
| **UI / UX** + HUD | alto | **medio** — tocca E11/E20, e il kit ha 3 dei 7 conflitti | medio |
| **Characters & Roster** | medio | basso | basso — quasi tutto già canonico (`D-037`, `D-032`) |
| **Scenarios / QA / Bots** | medio | **alto** — tocca `feature-registry.yaml` e `BP_GameMode.uasset`, entrambi modificati dalla sessione parallela | basso |
| **Governance** | basso | basso | medio — ma va ridotto a ciò che il repository non ha già |

> ✅ **Deciso il 2026-08-09 dall'autore: si parte da Map & Environment** (master + kit Interactive Map).
> È il cluster che porta l'unica proposta strutturale del pacchetto — la grammatica capability/verb — e
> l'unico che non tocca nessuno dei file in volo nella sessione parallela.

---

## 9. Vincoli operativi di questa sessione

1. **Non assegnare `D-0xx` né numeri di ADR.** Una sessione parallela ha in volo `D-044`…`D-048` e
   `ADR-0007`, non committati. Il progetto ha già avuto **cinque** collisioni di contatore: gli ID si
   assegnano **al merge**, chi arriva secondo rinumera.
2. **Non toccare** `feature-registry.yaml`, `feature-registry.json`, `RT_PDR_00_Decision_Log.md`,
   `OPEN_DECISIONS.md`, `BP_GameMode.uasset`, `RTHeroCatalogLibrary.cpp` finché la sessione parallela non
   ha mergiato: sono tutti nel suo diff.
3. **Non cancellare** i file di `todo/`: contengono l'unica provenance di alcune proposte finché non sono
   registrate. Il piano di cleanup delle chat riguarda il progetto ChatGPT, non il repository.
4. Il lavoro procede su worktree; PR verso `main`.

---

## 10. Stato del lavoro

### ✅ Fatto in questa sessione — cluster **Map & Environment**

| File | Che cosa |
|---|---|
| [`gameplay/spec-interazioni-mappa-cp101.md`](../../gameplay/spec-interazioni-mappa-cp101.md) | **nuovo** — owner della grammatica delle interazioni: verbi dell'elemento, capability dell'unità, tre dimensioni dell'accesso, macchina a stati, reason code che non perdono, regola delle tre soluzioni. Nessun numero, nessun tipo nuovo, quattro domande registrate (`INT-1`…`INT-4`) |
| [`roadmap/roadmap-v0.1.md`](../roadmap-v0.1.md) | CP 10.1 riscritta: titolo allineato a `D-025`, conseguenze topologiche dal Cleanup al **Blast**, link all'owner. Nota di tracciabilità sotto i rischi di E10 |
| [`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) | righe **53–56** + nota del sesto passaggio |
| [`gameplay/spec-motore-azioni-e4.md`](../../gameplay/spec-motore-azioni-e4.md) | banner storico completato con `D-025`: diceva ancora «`Guard` non è più universale», superato |
| [`technical/convenzioni-contenuti-ue.md`](../../technical/convenzioni-contenuti-ue.md) | §11-bis — la scala d'arte `≈ 1,5 m`, con il vincolo di §8.1: non è una metrica di design |
| [`technical/spec-mappa-multilivello.md`](../../technical/spec-mappa-multilivello.md) | §7 rimanda al nuovo owner |

**Non toccati di proposito**: `feature-registry.yaml`/`.json`, `RT_PDR_00_Decision_Log.md`,
`OPEN_DECISIONS.md`, `BP_GameMode.uasset`, `RTHeroCatalogLibrary.cpp` — tutti nel diff della sessione
parallela (§9.2).

### ✅ Fatto in questa sessione — cluster **UI / UX**

L'esito è diverso dal cluster precedente, e vale la pena dirlo prima dell'elenco: **il documento HUD del
repository è più avanzato del pacchetto.** [`technical/progettazione-hud.md`](../../technical/progettazione-hud.md) ha **50 sezioni** e
copre già, spesso con più precisione della fonte, tutto ciò che l'UI/UX Master e il kit HUD propongono come
lavoro da fare:

| Proposta del pacchetto | Dove esisteva già |
|---|---|
| Layer HUD, niente `WBP_TacticalHUD` monolitico | §4, §45 |
| Ghost Timeline `PREP · DASH · BLAST · MOVE`, reaction come ramo | §8, §11 — e CP 11.5 con il test `Preview.ReactionIsNotAPhaseEntry` |
| Action Ghost 3D presentation-only | §9, §9.1 |
| Certainty grammar Confermato/Previsto/Incerto | §16, §16.1 |
| Warning centralizzato con severità | §15, §15.1 |
| **Outcome Explanation** | §14.1 — si chiama «WHY?» |
| Overlay system con filtri | §24 |
| Conoscenza parziale a livelli | §25 — e **non** la chiama «Fog of War» |
| Sound overlay | §26 |
| Notification feed | §27 |
| Fast Reaction, target simultanei, timeout HOLD | §11.1, §11.2 |
| Facing UI per stile di movimento | §10, §10.1 |
| Combat Log da reason code | §14 |

Restavano **quattro** buchi veri, e sono le uniche aggiunte:

| File | Che cosa |
|---|---|
| [`technical/progettazione-hud.md`](../../technical/progettazione-hud.md) §11-bis | **Decision Time Bank**: il documento non lo conosceva, ed è entrato in v0.1 come CP 14.8 il giorno prima. Rimanda a [`gameplay/spec-decision-time-bank.md`](../../gameplay/spec-decision-time-bank.md) §11 per i requisiti e aggiunge le tre trappole di presentazione |
| idem §18-bis | **Interaction Inspector**: la superficie UI di CP 10.1, che senza questa PR non esisteva da nessuna parte. Tre corsie (disponibile · richiede capability · non disponibile) e reason code che non perdono conoscenza |
| idem §47-bis | **Accessibilità**: il vincolo «non solo colore» era in tre punti diversi e non copriva il caso più stretto — la finestra da 3,0 s. Raccolto, con la prova in scala di grigi come verifica meccanica |
| idem §50 | checklist DoD: due voci nuove che rendono verificabili le prime due |

**Non applicato** dal pacchetto, oltre ai conflitti di §3: le **38** feature `HUD.*` del kit e le 15
`RT-FEAT-UI-*` del master (9 inesistenti, e ne omette 2 che esistono); gli scenari `SCN-HUD-001`/`HUD-001`;
le issue `HUD-PLN-01`. Registrato come riga **57** della matrice.

### ✅ Fatto in questa sessione — cluster **Governance**

Come per l'UI, il pacchetto descrive in gran parte cose che il repository ha già, e spesso meglio:

| Proposta del Governance Master | Dove esisteva già |
|---|---|
| §1 ordine di prevalenza | [`README.md`](../../README.md) §gerarchia — e la riga 35 della matrice ha già chiuso il paradosso «canone + emendamenti» |
| §2 classificazione dei documenti (8 stati) | **7 tag**, con la motivazione: `CANONICAL · CURRENT · AS-BUILT · DELIVERED PLAN · HISTORICAL · RESEARCH · OPEN`. Il repository ha `CANONICAL` e `DELIVERED PLAN`, che il master non ha, e spiega **perché un `AS-BUILT` superato non è un difetto da correggere** |
| §4 gate del Feature Registry | ✅ **coincidono alla lettera**, 9 su 9 |
| §5 vocabolario di status | ✗ conflitto già registrato — riga 55 |
| §7 «Feature Registry = inventory, Roadmap = delivery order» | è l'intestazione stessa di [`feature-registry.yaml`](../feature-registry.yaml): «lo stato vive QUI e in nessun altro posto» |
| §13 Definition of Done globale | i gate, più [`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) |
| §15 viste generate | `feature_registry.py generate · wiki · workbook`, con `--check` come gate |
| §19 workbook `RESEARCH` | [`D-023`](../../decisions/RT_PDR_00_Decision_Log.md) |
| §20–§23 cleanup, CORE, Control Center | riguardano il **progetto ChatGPT**, non il repository |

Il valore era tutto in **§16, l'audit automatico documentale**: dodici controlli proposti, confrontati con
quelli che le tre macchine fanno davvero.

| Controllo §16 | Stato |
|---|---|
| `FeatureId` duplicati · roadmap ref stale · Wiki ref stale · `DONE` con gate mancanti | ✅ già nel validator |
| link rotti | ✅ `check-docs-links.py` |
| `ScenarioId` duplicati | ✅ `ScenarioIndex.DuplicateIdIsRejected` |
| **scenario senza feature** | ❌ **mancava** → aggiunto |
| `issue senza feature` · `CURRENT e SUPERSEDED simultanei` · `numeric value duplicato` · `roster/version mismatch` | ⬜ non implementati, e non tutti valgono il costo — vedi §12 |

| File | Che cosa |
|---|---|
| [`scripts/feature_registry.py`](../../../scripts/feature_registry.py) | **controllo nuovo**: nessuno `ScenarioId` senza una feature che lo rivendichi. Errore, non avviso, per simmetria con «`planned` ma presente» |
| [`roadmap/feature-registry.yaml`](../feature-registry.yaml) | i **6 scenari orfani** su 54 attaccati alla feature che dimostrano; `wiki_note` di `RT-FEAT-ENV-ELECTRIC` corretta (§5) |
| [`roadmap/feature-registry.md`](../feature-registry.md) | il controllo nuovo documentato, con il perché è un errore |
| `feature-registry.json` · `wiki/feature-status.md` · `wiki/meccaniche/acqua-e-elettricita.md` · `characters/v0.1/flux.md` | **rigenerati** dalla sorgente |
| gli stessi due file, in prosa | le due frasi **scritte a mano** che ripetevano l'affermazione superata, che nessun generatore poteva raggiungere |

**Verifica di mutazione**: staccato `Visual.Map.HighCoverBlocks` dalla sua feature, `validate` esce `1`
nominando esattamente quello scenario; ripristinato, `0`.

### 🟡 Difetto trovato durante il cluster — quattro `D-0xx` assegnati due volte

Non viene dal pacchetto. Quattro ID del Decision Log nominavano due decisioni diverse:

| ID | Prima occorrenza | Seconda occorrenza | Chi lo cita |
|---|---|---|---|
| `D-039` | azioni ambientali con owner nel roster | `E21` → `E35` | **9 siti**, tutti la prima (codice, test, harness, scenario JSON) |
| `D-041` | soglia d'udito per eroe | `Brace` prepara una reazione | 5 siti, tutti la **seconda** (spec Reaction Clash, `Scenarios/Spec/Brace/`, registry) |
| `D-042` | acqua bassa `+2` al rumore | Reaction Opportunity *contested* | 12 siti, tutti la **seconda** |
| `D-043` | arco frontale e `TeamKnowledge` | grammatica `STAND · READ · SHIFT` | **entrambe**: 2 siti la prima (`RTPerceptionTests.cpp`), 5 la seconda |

È la **sesta** collisione di contatore del progetto — il Decision Log ne conta cinque e ha già la regola:
*chi arriva secondo rinumera, non contende*.

**Chiusa il 2026-08-09 dalla PR [`#320`](https://github.com/DegrassiAaron/refactor-tactics-main/pull/320)**,
di una sessione parallela, mentre questa la stava chiudendo con lo schema **opposto**. Ha mergiato per prima,
quindi vale la sua e questa cede — è la regola applicata a sé stessa:

| Vecchio | Nuovo su `main` | Decisione che si è spostata |
|---|---|---|
| `D-039` | **`D-046`** | azioni ambientali con owner nel roster |
| `D-041` | **`D-047`** | `Brace` prepara una reazione |
| `D-042` | **`D-048`** | Reaction Opportunity *contested* |
| `D-043` | **`D-049`** | grammatica `STAND · READ · SHIFT` |

A tenere l'ID è stato il lato **meno** citato — `E21`→`E35` tiene `D-039`, la soglia d'udito tiene `D-041` —
cioè l'opposto del criterio «cede il lato che nessuna macchina verifica» enunciato per `D-040`. Il contenuto
delle otto decisioni non cambia; **prossimo ID libero: `D-058`**, dopo le `D-050`…`D-057` del Time Bank.

#### ✅ La conseguenza: la propagazione, completata qui

Spostare il lato **citato** significa che tutte le sue citazioni vanno riscritte. La `#320` ha coperto
`docs/` — 42 riferimenti in 12 file — ma non `Source/` né `Scenarios/`, dove ne restavano **26**:

| ID vecchio → nuovo | Riferimenti | Dove |
|---|---|---|
| `D-039` → **`D-046`** | 15 | commenti in 8 file di codice (`RTHeroCatalogLibrary`, `RTTurnManager`, `RTScenarioSession`, 4 test, `RTActionDef.h`) + 3 scenari-specifica |
| `D-041` → **`D-047`** | 2 | `Scenarios/Spec/Brace/ProfileChangesResponse.json` |
| `D-042`/`D-043` → **`D-048`/`D-049`** | 9 | i quattro `Scenarios/Spec/Clash/` |

⚠️ **Non si risolveva con un search/replace**: i tre `D-043` di `RTPerceptionLibrary.h` e
`RTPerceptionTests.cpp` sono rimasti **invariati**, perché quell'ID la decisione sulla percezione l'ha
**conservato**. Sostituirli avrebbe rotto l'altra metà.

Il diff su `Source/` è **interamente commenti**: nessun comportamento cambia.

> È lo stesso difetto di §5 — una decisione presa e non propagata — nato lo stesso giorno, dallo stesso
> contatore condiviso. La nota del Decision Log lo dice da due giorni: «fino a quando epic e decisioni si
> numerano a mano, la collisione è una questione di quando, non di se». Questa volta ne sono nate due nello
> stesso pomeriggio, e la seconda l'ha prodotta il tentativo di chiudere la prima.
>
> La regola operativa che ne esce: **una rinumerazione non è finita quando il log è coerente.** Un commento
> di codice che rimanda a `D-039` per una decisione che ora è `D-046` è leggibile, plausibile e falso.

#### E le righe della matrice hanno fatto lo stesso

Anche `DOC_CONFLICT_MATRIX.md` aveva **`53`, `54`, `55` due volte**: le mie della PR `#312` e quelle del Time
Bank della `#320`. Stesso contatore condiviso, stesso giorno, stessa forma. Rinumerate le seconde a
**`60`–`62`**, per lo stesso criterio: la `#312` era atterrata prima e nessuno cita quelle righe per numero.

### ✅ Fatto in questa sessione — cluster **Scenarios / QA / Bots**

L'ultimo del pacchetto, e il più assorbito dei tre. Il sorgente del bot —
[`archive/src/handoff/2026-08-08-bot-ai-roadmap-e-test-pie.md`](../../archive/src/handoff/2026-08-08-bot-ai-roadmap-e-test-pie.md) —
porta in testa **«✅ RECEPITO il 2026-08-08»**: il master ne è il riassunto, arrivato dopo.

| Proposta del master | Dove esisteva già |
|---|---|
| §0 tutti i producer passano dallo stesso path; niente `SetActorLocation`/`ApplyDamage` | [`test-automatico-unreal.md`](../../technical/test-automatico-unreal.md) · [`test-e-diagnosi.md`](../../technical/test-e-diagnosi.md) · `AGENTS.md` — **tre** punti |
| §6 execution mode Visual · Fast · Headless | `test-automatico-unreal.md`, con l'**equivalenza fra i tre come test** |
| §5 `PrimaryCategory` | ✗ respinta — riga 54 |
| §7 Scenario Registry, Stable ScenarioId | [`scenario-index-e-tag.md`](../../technical/scenario-index-e-tag.md) (`#209`) |
| §9 relazione Feature ↔ Scenario obbligatoria | ✅ **resa eseguibile** dal cluster Governance: è il controllo dello scenario orfano |
| §11 `result.json` strutturato · §14 determinismo | harness reale, `Simulation.DeterministicReplay` (100 iterazioni) |
| §16 as-built CVar + GameMode, niente `ARTTestDirector` | riga **23-bis** di questa matrice, dal 2026-08-08 |
| §17–§19 bot come producer di Intent, niente stato nascosto | [`avversario-bot.md`](../../wiki/game/avversario-bot.md) «Il bot non vede più di te» · `PIE-AI-02` · banner di [`h6-5-hex-bot-spec.md`](../../technical/h6-5-hex-bot-spec.md) |
| §22 profilo del bot senza `if Hero ==` | `h6-5-hex-bot-spec.md` |
| §25–§26 determinismo e decision trace | `avversario-bot.md` §«Se il bot fa una mossa che non capisci» |
| §27 roadmap bot v0.1 → v1 → v2 | **E26** e **E28** di [`roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) |
| §33 `spec-bot-utility.md` square-grid da archiviare | già in `archive/gameplay/` |

**Una** riga non aveva un documento corrente che la possedesse:

| File | Che cosa |
|---|---|
| [`roadmap/roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) — E26 | **invariante di difficoltà**: più difficile = più *ragionamento*, mai più *informazione*. Con il motivo per cui va scritta prima e non dopo, e il vincolo sull'errore intenzionale (deterministico, stream di seed dedicato) |
| [`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) | riga **63** — 14 feature `RT-FEAT-BOT-*`/`TEST-*` proposte contro le 2 reali |

**Perché quella riga e non altre.** È la scorciatoia più economica che esista: rendere un bot «difficile»
togliendogli la Team Knowledge costa cinque righe e funziona benissimo — e invalida ogni playtest fatto
contro di lui. Ed è già una **promessa pubblicata**: «Il bot non vede più di te» è una sezione della Wiki,
non una nota interna.

**Non applicato**: le 14 feature di §30; la `PrimaryCategory` di §5; gli `AI.*`/`SCN-*` di §28; la roadmap
`v0.1…v0.6` del Developer Toolkit (§1), che è una **numerazione parallela** a M6–M11 ed E1–E21 — vietata
dalla riga 24.

### ⬜ Resta da fare

1. ✅ ~~Difetto `D-046` di §5~~ — **chiuso il 2026-08-09**: corretto alla sorgente (`wiki_note`), rigenerate
   le due pagine derivate, riscritte a mano le due frasi in prosa che il generatore non raggiungeva.
2. ✅ ~~La collisione dei quattro `D-0xx`~~ — **chiusa il 2026-08-09** da una sessione parallela (`#320`).
   Resta la **propagazione**: vedi il punto 7.
3. ✅ ~~`INT-1`…`INT-4`~~ — **chiuso da `#344`**: `INT-1`, `INT-2` e `INT-4` sono in
   [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md); **`INT-3` non esiste**, perché era già `FAC-6` e il suo
   owner è [ADR-0005](../../decisions/adr-0005-orientamento.md) — la spec di CP 10.1 ne è il primo
   consumatore, non il proprietario. Il rimando è nei due sensi. Issue `#335`.
4. ✅ ~~**Characters & Roster**~~ — **atterrato con `#346`** (`d5b36cf`), lavorato da una sessione parallela:
   `ADR-0007`, la sezione *Profilo di attacco base* nel template applicata ai quattro eroi,
   `Bastion.ImpactShot` 24 → 8 + `Status.Slow`, quattro scenari rimisurati e due nuovi.
   **Verificato in modo indipendente** su worktree separato prima del merge: build pulita e **544/544
   `Success`**, zero `Fail`; merge di prova contro `main` senza conflitti e senza regressioni su `#321`.
   > Storia che vale la pena conservare: quel lavoro era finito in `stash@{0}` con l'etichetta
   > **«scartato»** durante un fast-forward, e da lì sembrava perduto. Il recupero tentato è stato
   > **abbandonato** quando si è scoperto che la sessione era ripartita altrove con una versione migliore —
   > più recente di 22 commit e con i due scenari che al recupero mancavano.
   > ⚠️ Quei due scenari erano stati scritti **una prima volta**, sono rimasti non tracciati e sono stati
   > cancellati: non erano in nessun commit, stash o oggetto pendente. La seconda scrittura è quella che è
   > atterrata. **Il lavoro non committato non è lavoro salvato**, nemmeno per un'ora.
   >
   > Resta aperto il **resto** del master `RT_Characters_Roster_Master_*` — fazioni, Signature Mechanics,
   > Super/Cooldown v0.2, data model — che `ADR-0007` non tocca. Issue `#336`.
5. ✅ ~~`RT-FEAT-MAP-INTERACTIVE-EDGES` e le feature di E10~~ — collegate a
   [`spec-interazioni-mappa-cp101.md`](../../gameplay/spec-interazioni-mappa-cp101.md) fra i loro
   `owner_specs`. Issue `#340`.
6. I quattro controlli §16 non implementati — `issue senza feature`, `CURRENT`/`SUPERSEDED` simultanei,
   valore numerico duplicato fra normative, `roster/version mismatch` — vanno valutati uno per uno. Il terzo
   è il più interessante e il più costoso: sarebbe il gate che impedisce il difetto che il progetto ha già
   pagato cinque volte col conteggio dei test.
7. ✅ ~~La propagazione della rinumerazione~~ — **chiusa**. `Source/` e `Scenarios/` in `#321` (26
   riferimenti), il **registry** qui: cinque righe di `feature-registry.yaml` puntavano ancora ai numeri
   pre-`#320` — `D-041`→`D-047`, `D-042`→`D-048` ×3, `D-043`→`D-049` — e da lì il generatore li propagava.
   > Era il posto che entrambe le passate avevano saltato, ed è **il peggiore in cui saltarlo**: la yaml è
   > la sorgente verificata dalla macchina, e tutto il resto ne è generato. Se n'è accorto il commit
   > `ddba108`, la cui rigenerazione aveva riscritto `D-048` → `D-042` in
   > [`reazioni-overwatch-e-previsioni.md`](../../wiki/game/reazioni-overwatch-e-previsioni.md),
   > sostituendo un riferimento **corretto** con uno stantio — senza colpa di chi l'ha fatto: il generatore
   > stava facendo esattamente il suo lavoro su una sorgente sbagliata.
