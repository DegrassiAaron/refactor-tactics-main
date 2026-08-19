# Cinque processi paralleli — mappatura sulle track reali e sequenza di lavoro

> `CURRENT` · **Stato**: proposta operativa, misurata · **Data**: 2026-08-17
> **HEAD misurato**: `9a1bd1d4` (`origin/main`)
> **Owner del write-set**: [`../parallel-batch.yaml`](../parallel-batch.yaml) — questo documento
> **non** lo sostituisce. Se le due fonti divergono, vince il YAML: è lui che i gate leggono.
> **Regola che governa**: [`D-139`](../../decisions/RT_PDR_00_Decision_Log.md) (write-set di batch +
> Binary Asset Lease) · [`../../technical/tooling/workflow-parallel-claude.md`](../../technical/tooling/workflow-parallel-claude.md).
> **Precedente**: [`quattro-processi-paralleli-triage-2026-08-14.md`](quattro-processi-paralleli-triage-2026-08-14.md),
> che di 57 sezioni ne applicò **13** — e le §21–§27 furono respinte perché proponevano *un terzo
> vocabolario di classificazione*. Questo documento non ne propone un quarto.

---

## 1. Il vincolo che decide tutto, e non è il dominio

I cinque processi sono divisi per **materia** (spatial, simulation, client, content, jolly). Il
parallelismo reale è invece limitato dal **write-set**: due sessioni possono lavorare a due domini
diversi e collidere lo stesso, perché scrivono lo stesso file di tracking.

Misurato la mattina del 2026-08-17 su `9a1bd1d4`: le tre PR aperte si contendevano **otto** file, e
nessuna delle tre condivideva una riga di `Source/`.

| File conteso | PR che lo toccava |
|---|---|
| `docs/roadmap/feature-registry.yaml` / `.json` | #1112 *(spatial)*, #1104 *(graybox)* |
| `docs/roadmap/parallel-batch.yaml` | #1113 *(replay_ui)*, #1104 |
| `docs/decisions/RT_PDR_00_Decision_Log.md` | #1104 |
| `docs/OPEN_DECISIONS.md` · `docs/CONTEXT_INDEX.md` | #1104 |
| `docs/archive/src/README.md` · `docs/roadmap/editor-sessions.yaml` | #1104 |

> ⏱️ **Rimisurato poche ore dopo, su `94575ef4`, e la fotografia era già scaduta** — il che vale come
> prova della tesi meglio della tabella stessa. #1104 e #1112 sono **atterrate**; restano **#1120**
> (chiusura della track `graybox_kit`) e **#1113**, e toccano **un solo file entrambe**:
> `docs/roadmap/parallel-batch.yaml`.
>
> ∴ La contesa non è sparita, si è **concentrata**: il file che dichiara chi può scrivere cosa è
> l'ultimo rimasto conteso, ed è conteso da due sessioni che stanno entrambe *rilasciando* un write-set.
> La tabella qui sopra resta con la sua data perché è la misura che ha motivato le scelte di questo
> giro — in particolare il non aver toccato Feature Registry, Decision Log e `OPEN_DECISIONS.md`.

∴ **La regola del batch non è «un dominio per processo»: è «un `writable` per processo, e i file
condivisi si toccano una volta sola, in integrazione».** I cinque processi qui sotto sono una vista
comoda sul lavoro; l'autorità resta `parallel-batch.yaml`.

⚠️ **I cinque processi non sono una funzione delle track.** `parallel-batch.yaml` ne dichiara venti, e
il rapporto è molti-a-uno in entrambi i versi: `rt-unit` ospita **nove** track che ricadono su tre
processi diversi, mentre il processo C ne raccoglie tre già distinte (`replay_ui`, `client_tools`,
`frontend_shell`). Chi cerca la corrispondenza uno-a-uno non la trova, ed è la stessa ragione per cui
il triage del 2026-08-14 respinse `work_tracks`.

---

## 2. La mappatura, misurata

| | Processo | Track in `parallel-batch.yaml` | Worktree | Stato oggi |
|---|---|---|---|---|
| **A** | Spatial / World | `spatial` | `D:/rt-spatial` | 🟢 `ACTIVE` su **#833**, PR **#1112** aperta |
| **B** | Simulation / Rules | `simulation` | `D:/rt-simulation` | 🟢 `ACTIVE` su **#886** |
| **C** | Client / Replay / Tools | `replay_ui` · `client_tools` · `frontend_shell` | `D:/rt-client` · `D:/rt-menu` | 🟢 `replay_ui` `ACTIVE` (#472, PR **#1113**), `frontend_shell` `ACTIVE` (#937, seduta U24), `client_tools` `IDLE` (#78) |
| **D** | Content / Editor *(autore)* | `content_editor` · `graybox_kit` · sedute `U*` | `D:/rt-content` · `D:/rt-unit` | 🟡 `content_editor` **`IDLE`** — #451 è `CLOSED`; `graybox_kit` era `ACTIVE` e **sta chiudendo** con #1120 |
| **E** | Jolly | *nessuna* — da dichiarare all'apertura | `D:/rt-xxx` | 🔴 **non dichiarato**: un processo senza `writable` è, per `D-139`, un processo che deve fermarsi al primo file |

---

## 3. Assegnazione delle issue aperte

79 issue portano l'etichetta `v0.1` ed è il perimetro che conta. Le altre 153 sono `post-v0.1` e non
entrano in una sequenza di release.

### A — Spatial / World
**Owner naturale**: hex, grafo, pathfinding, LOS, map state, environment.

| Issue | Titolo breve | Nota |
|---|---|---|
| **#833** | Grafo di interazione, catena locale per `Interact` | in corso, PR #1112 |
| #74 | `CP 10.1` — Activate e Interact sugli oggetti | consuma #833 |
| #983 | `RTCellTopZ` è una costante in namespace anonimo | debito, tocca solo `Map/` |
| #214 | `E19` · Classe di mappa e composizione | epic |
| #695 | Quattro stati di porta, due forme | ⚠️ **confine con C**: la *viz* è editor tooling |

### B — Simulation / Rules
**Owner naturale**: turni, resolver, abilities, reactions, objectives, TurnLog.

| Issue | Titolo breve | Nota |
|---|---|---|
| **#886** | Le decisioni di reazione tornano dal TurnLog | in corso; DoD riscritto oggi |
| #166 | `CP 14.6` — Counterplay, UI della finestra, pacing | ⚠️ la **UI** è di C, la finestra è di B |
| #314 · #319 | `CP 14.7` Reaction Clash · `CP 14.8` Time Bank | P3, tagliabili |
| #542 | Seam dei `DecisionProvider` (`D-101`) | design già scritto |
| #1077 | Uno stato temporaneo nasce, dura e finisce: il TurnLog tace | |
| #1060 | Il T6 dello showcase non ha vocabolario verificabile | |
| #583 | La condizione di `D-109` gira, nessuno scenario la esercita | |
| #888 | Il danno a un decision boundary ignora la copertura | `question` |
| #690 · #686 | Rumore per azione · soglia d'udito nei cataloghi | alimentano #159/#160 |
| #159 · #160 | `CP 13.4` · `CP 13.5` — conoscenza parziale | |
| #75 | `CP 10.2` — Obiettivo contestabile | |
| #61 · #509 | `CP 7.2` Gadget · `CP 7.6` delta di danno | |
| #1088 | I bot non attaccano mai: 12 round, zero `Combat` | 🔴 regressione |
| **#1118** | La risposta di reazione e la sua ragione sono un enum solo | 🆕 precede #314, **non blocca** #886 |
| **#1119** | `RCI-1`..`RCI-4` — Canonical Intent | 🆕 `question`, `post-v0.1`: risposte producibili oggi |

### C — Client / Replay / Tools
**Owner naturale**: UI C++, presentation, replay, editor tooling C++.

| Issue | Titolo breve | Nota |
|---|---|---|
| **#472** | Replay R6 — l'interfaccia | in corso, spec panel fatto |
| **#937**–#941 | `CP 46.2`–`46.6` — loading, menu, play, result, pause | seduta U24, `frontend_shell` |
| #77 · #78 · #79 · #80 | `CP 11.1`–`11.4` — HUD, intenti alleati, combat log, `rt.Debug.*` | #78 è `P0` e la track è `IDLE` |
| #613 · #705 | `CP 11.7` Screen HUD UMG · `CP 11.8` Pointer Interaction | |
| #172 · #173 | `CP 11.5` Ghost Timeline · `CP 11.6` scrubbing | |
| #219 · #220 · #637 | `CP 20.2` · `CP 20.3` · tassonomia icone | `E20` |
| #291 | La rotazione dichiarata: resta l'input | |
| #871 · #921 · #931 | Fill non aggiorna `MoveCost` · overlay per-tool · gizmo Arch | **editor tooling C++**, non contenuto |
| #956 | `CP 47.3` — grammatica visiva della board | |
| **#1114**→**#1117** | Scenario Composer: writer canonico, placement, turni, `Run`/`Reset` | 🆕 catena stretta, in quest'ordine. #1114 è **runtime**, non Editor |

### D — Content / Editor *(l'autore davanti a Unreal)*
**Owner naturale**: asset, Blueprint, mappe, materiali, widget, import, configurazione Editor.

| Issue | Titolo breve | Nota |
|---|---|---|
| #1095 · #1096 | Seduta **U25** · voci PIE del contratto graybox | #1096 è bloccata da `D-139`, non da contenuto |
| #287 · #288 · #289 | `CP E21.1`–`21.3` — personaggi, animazioni, leggibilità | `E21` |
| #38 · #82 | `CP 2.8` playtest hex (sessione D) · `CP 12.2` matrice test manuali | gate umani |
| #1069 | `BP_GameMode` con `MatchFormat` non versionato, entrato fuori dalla lease | 🔴 violazione `D-139` già avvenuta |
| #996 | Un arco pendente sparisce modificando la Transform | residuo U1 |
| #868 | La guida di U1 precede `D-139` | residuo U1 |

⚠️ **D è l'unico processo che tocca binari.** Ogni `.uasset`/`.umap` richiede una **Binary Asset Lease**
esclusiva dichiarata nel batch, un holder per path. #1069 esiste perché la regola è già stata violata
una volta.

### E — Jolly
**Owner naturale**: nessuno per costruzione — ed è il problema.

| Issue | Titolo breve | Nota |
|---|---|---|
| #1109 | Il gate di naming valida 375 `.md` e zero `.yaml` | gate |
| #1106 | Il validator non legge i checkpoint di milestone | gate |
| #1111 | La nota del Relè giustifica il T5 con una portata che non esiste | docs |
| #995 | Consolidare Elemental Proficiency v0.1 | docs |
| #738 | Una riga dichiara bloccante una issue già chiusa | gate |
| #960 · #403 | `RNG-1`/`RNG-2` · `BAL-1` | `question`, gate umani |

🔴 **E è il processo a rischio più alto, e per una ragione strutturale**: quasi tutto il suo lavoro
naturale — gate, riconciliazioni, documenti — atterra su file `integration_only`, cioè su ciò che gli
altri quattro processi si contendono. Un jolly senza `writable` dichiarato **collide con tutti**.

**Rimedio, e non è opzionale**: prima di aprire E, dichiarare la sua track in `parallel-batch.yaml` con
un `writable` esplicito e ristretto. Se il lavoro è un `integration_only`, allora E **non è un quinto
processo parallelo**: è il **turno di integrazione**, e gira *dopo* gli altri, non insieme.

---

## 4. Sequenza consigliata

Le dipendenze reali sono poche; la maggior parte del lavoro è genuinamente parallelo.

```text
ORA (nessuna dipendenza fra loro)
  A  #833 ──► #74
  B  #886 ──► #166 ──► #314 · #319
  C  #472        · #937→#941 (U24) · #77/#78
  D  U25 (#1095) ──► #1096

POI (aprono quando il predecessore chiude)
  B  #690 + #686 ──► #159 ──► #160
  C  #613 ──► #705 ──► #172 ──► #173
  A  #695 ──► (viz porte, con C)

INTEGRAZIONE (turno singolo, non parallelo)
  E  gate: #1109 · #1106 · #738
     riconciliazione: feature-registry · parallel-batch · Decision Log · OPEN_DECISIONS
```

**Punto di sincronizzazione obbligato**: `feature-registry.yaml` e le sue cinque viste generate. Tutti e
cinque i processi lo alimentano; nessuno dei cinque lo possiede. Si rigenera **una volta**,
sull'albero unito, al passo 8 della chiusura del batch — mai sulla propria base.

---

## 5. Le tre condizioni che oggi non reggono

1. 🔴 **`content_editor` è `ACTIVE` nella prosa e `IDLE` nel dato**, su `#451` che è `CLOSED`. Tiene
   prenotato un `writable` di 26 path che nessuno sta usando. Rilasciarlo è una decisione di chi
   riapre la seduta — ma finché resta, il processo **D** non ha una track viva propria.
2. 🔴 **Il processo E non esiste in `parallel-batch.yaml`.** Va dichiarato prima di scrivere la sua
   prima riga, o `D-139` lo ferma al primo file.
3. 🟡 **`parallel-batch.yaml` è l'ultimo file conteso, e lo contendono due rilasci.** La mattina del
   2026-08-17 erano contesi anche il Decision Log e `OPEN_DECISIONS.md` (#1104): è la ragione per cui
   il triage del Canonical Intent ha portato le sue quattro domande in una **issue** invece che lì.
   #1104 è atterrata; restano **#1120** e **#1113**, entrambe sul solo `parallel-batch.yaml`. Quando
   atterrano, `RCI-1`…`RCI-4` vanno riconciliate in `OPEN_DECISIONS.md` — è un debito **dichiarato**,
   con un owner (#1119) e non una lacuna.

---

## 6. Limiti

- Le assegnazioni della §3 sono una **proposta di lettura**, non una modifica a `parallel-batch.yaml`:
  nessun `writable` è stato riscritto da questo documento.
- Il perimetro è `v0.1` (79 issue aperte su 232). Le release successive hanno epic già aperte
  (`E36`–`E45`) e non sono state affettate qui — farlo produrrebbe la seconda copia della roadmap che
  il §35 del sorgente replay proponeva e che è stata respinta.
- Nessuna build, nessun test Unreal: il write-set di questo lavoro è `docs/` e GitHub.
