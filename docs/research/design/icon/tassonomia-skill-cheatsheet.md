# La cheat sheet della tassonomia skill — cosa contiene, e chi possiede ogni riga

> `RESEARCH` · **Dichiarato il 2026-09-20** · **Autorità: nessuna.**
> **Cosa è**: l'indice dei due binari che stanno qui accanto —
> [`RefactorTactics_Skill_Taxonomy_CheatSheet_LARGE_v0.1.pdf`](RefactorTactics_Skill_Taxonomy_CheatSheet_LARGE_v0.1.pdf)
> e il `.docx` gemello. Cinque pagine di liste: fase, targeting, effetti, modificatori, difese, trigger,
> costi, disponibilità, grammatica visiva e una checklist di dieci domande.
> **Cosa non è**: ⛔ **non è una copia della tassonomia.** Quasi ogni riga di quelle liste ha già un owner
> nel repository, e riscriverle qui creerebbe la seconda fonte che
> [`AGENTS.md`](../../../../AGENTS.md) §8 vieta. Questa pagina dice **dove** guardare, non cosa vale.

## Perché esiste questa pagina invece della conversione

I due binari sono entrati il **2026-08-29** con `fe4af8bf`, un commit di consegna cumulativa che non li
nominava singolarmente, e sono rimasti nella **radice di `docs/`** fino al 2026-09-20 — dove
[`../../../README.md`](../../../README.md) dichiara che *«dal 2026-08-12 questa cartella è interamente in
Markdown»* ([D-009](../../../decisions/RT_PDR_00_Decision_Log.md)).

`D-009` ha convertito ventiquattro binari di prosa in Markdown perché **il loro contenuto non viveva
altrove**. Qui non è così: il contenuto vive altrove quasi tutto, e la conversione produrrebbe un secondo
posto in cui la stessa regola può divergere. 🔑 **Il difetto che `D-009` chiude è che un binario non è
`grep`-abile** — e a quello risponde la tabella qui sotto: cercando `HIT RULE` o `ControlResistance` ora si
arriva a un file di testo che dice chi li possiede.

## Chi possiede ogni sezione

Misurato il 2026-09-20 su `origin/main` `25ec8bad`.

| Sezione della cheat sheet | Owner |
|---|---|
| **1. Phase / Timing** | [`adr-0003-modello-azioni-v01.md`](../../../decisions/adr-0003-modello-azioni-v01.md) · [`spec-sequenza-turno.md`](../../../gameplay/spec-sequenza-turno.md) |
| **2. Application / Targeting** — target, relation, shape, delivery, hit rule, range | [`spec-icon-card-grammar.md`](../../../technical/systems/spec-icon-card-grammar.md) per delivery e hit rule · [`RT_ActionCatalog_v0.1.md`](../../../balance/RT_ActionCatalog_v0.1.md) per i valori |
| **3. Effect / Outcome** — effetto primario, damage type, damage source | [`RT_ActionCatalog_v0.1.md`](../../../balance/RT_ActionCatalog_v0.1.md) |
| Environment / Element · System verbs | [`RT_TerrainCatalog_v0.1.md`](../../../balance/RT_TerrainCatalog_v0.1.md) |
| Status v0.1 | [`brief-stati-personaggio-e-trasformazioni.md`](../../../gameplay/brief-stati-personaggio-e-trasformazioni.md) |
| Control / Displacement | [`spec-tassonomia-movimento.md`](../../../gameplay/spec-tassonomia-movimento.md) |
| **4. Modifier** — skill modifier, context modifier | [`spec-icon-card-grammar.md`](../../../technical/systems/spec-icon-card-grammar.md) · `Ignore Cover` anche in [`spec-cover-placement-intra-hex.md`](../../../technical/systems/spec-cover-placement-intra-hex.md) |
| **5. Counter / Defense** | [`spec-icon-card-grammar.md`](../../../technical/systems/spec-icon-card-grammar.md), via **`D-237`**/**`D-238`** |
| **6. Condition / Trigger** | [`adr-0004-finestre-di-reazione.md`](../../../decisions/adr-0004-finestre-di-reazione.md) · `ERTReactionTrigger` in `Source/` |
| **7. Cost / Cooldown / Charges** | [`spec-economia-del-turno.md`](../../../gameplay/spec-economia-del-turno.md) · [`brief-super-e-cooldown.md`](../../../gameplay/brief-super-e-cooldown.md) |
| **8. Availability / Information** — certainty, knowledge | [`brief-conoscenza-parziale.md`](../../../gameplay/brief-conoscenza-parziale.md), col vocabolario `Nascosto` / `ContattoIncerto` / `Rilevato` |
| **9. Grammatica visiva delle icone** | [`spec-icon-card-grammar.md`](../../../technical/systems/spec-icon-card-grammar.md) · [`visual-language/09-alfabeto-fase-e-conseguenza.md`](visual-language/09-alfabeto-fase-e-conseguenza.md) |
| **10–12.** Checklist, formula, scope v0.1 | ⚠️ **nessuno, e non serve**: sono una procedura di design, non una regola |

## ⚠️ Quattro etichette che nessun owner vivo ha adottato

Misurate una per una il 2026-09-20 con `git grep -lI` su **tutto** l'albero versionato, escludendo i due
binari:

| Etichetta | Dove compare |
|---|---|
| `Until Triggered`, nella forma con lo spazio che usa la cheat sheet | **nessun file** |
| `UntilTriggered` | 2 file, entrambi sorgenti già archiviati in [`archive/src/`](../../../archive/src/README.md) |
| `Start Turn` · `End Turn` come qualificatori di timing | solo in `archive/src/handoff/`, più una menzione di `End Turn` dentro `D-095` — che parla di **AI planner**, non di timing |
| `StartTurn` · `EndTurn` | in [`../../../product/showcase-v0.1.md`](../../../product/showcase-v0.1.md):408 e nel suo sorgente, ma come **campo di schedule di scenario** (`ObjectivePhase[]` con `StartTurn`, `ActiveCells`, `Duration`): è un altro asse, non questo vocabolario |
| `OnShieldDamage` | 1 file, [`archive/src/handoff/2026-08-28-combat-skillgrammar-delta.md`](../../../archive/src/handoff/2026-08-28-combat-skillgrammar-delta.md) — un sorgente archiviato |

🔑 **Nessuna delle quattro è adottata da un owner vivo come qualificatore di timing.** Le occorrenze
sono materiale archiviato, oppure la **stessa stringa su un asse diverso** — e la seconda è la più
insidiosa: un `grep` che trova `StartTurn` nella showcase e conclude *«il vocabolario esiste»* sta leggendo
un campo di scenario.

⚠️ **Zero non vuol dire «da adottare».** Vuol dire che nessun owner le ha scelte, e che chi le trovasse
qui starebbe leggendo una **proposta**. È la ragione per cui questi file stanno in `research/` — *«input
non ancora consumato»*, [`../../../README.md`](../../../README.md) §*Le quattro nature di un file* — e non
in `archive/`: non sono stati recepiti.

⚠️ **E non sono stati recepiti nemmeno dal referto che sembra il loro.**
[`combat-skillgrammar-delta-spec-panel-2026-08-28.md`](../../../roadmap/plans/combat-skillgrammar-delta-spec-panel-2026-08-28.md)
ha consumato un kit **diverso** — `CLAUDE_RT_Combat_SkillGrammar_Consolidation_Delta_2026-08-28.md`, oggi
in `archive/src/handoff/` — e ne sono uscite `D-237` e `D-238`. I due binari sono arrivati **il giorno
dopo** e non sono mai stati triagiati per conto loro. Il vocabolario condiviso viene dalla stessa
lavorazione di design, non da un consumo di questi file.

## Se li consumi

La disposizione si scrive in [`../../../archive/src/README.md`](../../../archive/src/README.md) e i file si
spostano lì, come ogni sorgente recepito. ⛔ **Prima**, però, va deciso cosa fare delle quattro etichette
sopra: adottarle è una decisione di vocabolario, scartarle pure. Consumare il kit senza pronunciarsi le
lascerebbe in un archivio da cui rientrano per copia-incolla.
