# Skill Plus — consolidamento del sorgente e matrice di riconciliazione

> `CURRENT` · **Stato**: sorgente consumato, matrice prodotta · **Data**: 2026-08-17
> **HEAD misurato**: `d849029e` (`origin/main`)
> **Sorgente**: `RefactorTactics_SkillPlus_Claude_Handoff_2026-08-17.md` — 34 sezioni, 10 epic
> proposte (`SP-01`…`SP-10`), ~70 issue candidate. Archiviato in
> [`../../archive/src/`](../../archive/src/RefactorTactics_SkillPlus_Claude_Handoff_2026-08-17.md).
> **Owner delle regole**: [`spec-ownership-abilita-interazioni-sinergie.md`](../../gameplay/spec-ownership-abilita-interazioni-sinergie.md) ·
> [`adr-0006-ownership-abilita-sinergie.md`](../../decisions/adr-0006-ownership-abilita-sinergie.md) ·
> `../feature-registry.yaml`. Questo referto non è owner di niente.
> **Particolarità**: il sorgente chiede al §27 una *«consolidation matrix obbligatoria»* prima di
> qualunque scrittura. È stata prodotta — e ha ribaltato il verdetto.

---

## 1. Il verdetto in una riga

**Il sistema che il sorgente propone di costruire esiste già, è `INTEGRATED`, ed è in `v0.1`.**
`RT-FEAT-ENV-SYSTEMIC-COMBOS` — *«Interazioni sistemiche producer/consumer»* — porta l'ADR, i test e
gli scenari che le sue prime cinque epic descrivono come da progettare. Ciò che manca davvero è
**contenuto**: due elementi (`Debris`, `Wind`) che nel repository **non esistono**, e un nome che ne
sarebbe il quarto.

---

## 2. La misura che ha ribaltato il verdetto

`RT-FEAT-ENV-SYSTEMIC-COMBOS`, letta su `d849029e`:

```yaml
title: Interazioni sistemiche producer/consumer
release: v0.1 · priority: P1 · status: INTEGRATED
roadmap: { epic: E8, checkpoints: ["8.5"] }
owner_specs:
  - docs/gameplay/spec-ownership-abilita-interazioni-sinergie.md
  - docs/decisions/adr-0006-ownership-abilita-sinergie.md
issues: [68]
tests:
  - RefactorTactics.Environment.ChangesAppearInTurnLog
  - RefactorTactics.Actions.EnvironmentalSetMatchesCatalog
  - RefactorTactics.Reactions.NoHeroSpecificBranchInResolver
scenarios: [Visual.Combat.WaterElectric]
```

**Nove gate su dieci sono `done`.** E l'epic che la possiede — **E8** (#22) — è **`CLOSED`**, come i
suoi checkpoint: `CP 8.3` (#66), `CP 8.4` (#67), `CP 8.5` (#68).

Confronto diretto col sorgente:

| Il sorgente propone | Nel repository |
|---|---|
| §5.2 *«non hard-codare coppie di personaggi»* → producer/consumer | **`D-029` + `ADR-0006`**, e un test che lo pinna: `RefactorTactics.Reactions.NoHeroSpecificBranchInResolver` |
| §7.2 `Electric + Water → Conductive Propagation` | scenari **`Visual.Combat.WaterElectric`** e **`WaterElectricCoordinated`**, entrambi in `Scenarios/` |
| §7.2 `Fire + Water → Steam` | feature **`RT-FEAT-ENV-STEAM`** |
| `SP-040`/`SP-041` (v0.3–v0.4) | `CP 8.3` e `CP 8.4`, **chiusi in v0.1** |
| §7.6 Certainty `Confermato/Previsto/Incerto` | feature **`RT-FEAT-UI-CERTAINTY`** |
| §7.5 friendly-fire warning | feature **`RT-FEAT-UI-WARNINGS`** |
| §7.2 icon grammar | feature **`RT-FEAT-UI-ICON-LANGUAGE`** + epic `E20` |

∴ **`SP-01` è consegnato e `SP-05` è consegnato per metà.** Seguire la roadmap del sorgente avrebbe
riaperto lavoro chiuso e spostato in `v0.3–v0.4` due checkpoint che sono in `main` da tempo — la stessa
forma d'errore che l'archivio ha già registrato per la *Mini Roadmap Autobattle*, dove le sezioni
respinte chiedevano **meno** di ciò che era consegnato.

---

## 3. Il gap reale, misurato

Tre grep su `Source/RefactorTactics/`, e due dei tre risultati sono nulli:

| Concetto | Occorrenze | Nota |
|---|--:|---|
| `Debris` | **0** | nessun header, nessuna feature, nessuna issue, nessuno scenario |
| `Rubble` | **0** | idem |
| `Scatter` | **0** | idem |
| `Wind` | **0** | vedi §3.1 |

### 3.1 🔴 «Wind» sembra esistere in sei header e non esiste

Il primo `git grep -il "Wind"` restituiva **6 header**, e sarebbe stato ragionevole concludere che il
vento esiste. Disambiguato con `git grep -ioh -E "\w*Wind\w*"`, le occorrenze sono **tutte**:

```
CutoffWindowMs · MaxWindow · ReactionWindow · UncoveredReactionWindow
WindowsOpened · Window · Rewind · FRTShowcaseUncoveredWindowTest
```

Cioè `Window` (la finestra di reazione) e `Rewind` (il replay). **Zero vento.**

E lo stesso inganno si ripete in italiano: `git grep -il "vento"` sui documenti gameplay dà otto file,
ma sono `e`**`vento`** e `inter`**`vento`**. Con `\bvento\b` l'occorrenza isolata è **una sola**.

∴ **il caso di riferimento del sorgente — `Wind + Debris → Flying Debris` — ha entrambi i termini
assenti.** Non è un difetto del sorgente: è contenuto nuovo, e va tracciato come tale invece che come
integrazione di un sistema esistente.

---

## 4. 🔴 CONFLICT — il nome sarebbe il quarto

Il sorgente §1 chiede: *«Se trovi un conflitto, NON scegliere silenziosamente»*. Questo è il conflitto.

`spec-ownership-abilita-interazioni-sinergie.md` **§9 — Naming editoriale** decide già il vocabolario:

> `Combo` resta ammesso come termine descrittivo […] Per esempi cross-character preferire **Sinergia**,
> **Interazione sistemica**, **Setup → Payoff**, **Scenario dimostrativo**.

«Skill Plus» sarebbe un **quarto** termine per un asse già nominato tre volte, e nominerebbe
esattamente ciò che `RT-FEAT-ENV-SYSTEMIC-COMBOS` chiama *«interazioni sistemiche producer/consumer»*.

Il precedente esiste ed è nell'archivio: il triage dei *quattro processi paralleli* respinse le §21–§27
perché *«proponevano un terzo vocabolario di classificazione accanto a `execution_lanes` e
`domain_groups`, che sono già validati»*.

**Non deciso qui.** La decisione appartiene a chi possiede la spec — sorgente A, sorgente B, differenza
e proposta sono nella issue.

---

## 5. Consolidation matrix (§27 del sorgente)

Vocabolario richiesto dal sorgente stesso.

| Seed | Owner esistente | Issue/Epic | Azione | Ragione |
|---|---|---|---|---|
| `SP-01` Skill Plus Core | `RT-FEAT-ENV-SYSTEMIC-COMBOS` | `E8` #22, `CP 8.5` #68 | **REUSE** | `INTEGRATED`, 9 gate su 10 `done`, epic chiusa |
| `SP-001` grammatica producer/consumer | `spec-ownership-abilita-interazioni-sinergie.md` · `ADR-0006` | #68 | **REUSE** | è `D-029`, già normativo |
| `SP-003` valutazione nel resolver | test `NoHeroSpecificBranchInResolver` | #68 | **REUSE** | il divieto di branch per eroe è **pinnato da un test** |
| `SP-005` eventi TurnLog | test `Environment.ChangesAppearInTurnLog` | #68 | **REUSE** | esiste |
| `SP-02` Debris Scatter reference | *nessuno* | *nessuna* | **CREATE** | **0 occorrenze**: è il gap reale |
| `SP-010`…`SP-016` (7 issue debris) | *nessuno* | *nessuna* | **DEFER** | dipendono da `SP-02`: aprirle ora sarebbe work-in-progress senza soggetto |
| `SP-03` UI affordance | `RT-FEAT-UI-CERTAINTY` · `-WARNINGS` · `-ICON-LANGUAGE` · `-ACTION-GHOSTS` | `E20` #217 | **LINK** | quattro owner esistenti; l'affordance *specifica del Plus* si valuta quando il Plus ha un contenuto da mostrare |
| `SP-04` Team Combo Discovery | `RT-FEAT-PERCEPTION-*` + privacy | #780, #784 | **DEFER** | dipende dalla rete (`E40`, `v0.5`) |
| `SP-040` Water + Electricity | scenari `Visual.Combat.WaterElectric*` | `CP 8.3` #66 | **REUSE** | **chiuso**, non v0.3 |
| `SP-041` Water + Fire → Steam | `RT-FEAT-ENV-STEAM` | `CP 8.4` #67 | **REUSE** | **chiuso**, non v0.3 |
| `SP-042`…`SP-047` grammar expansion | `RT-FEAT-ENV-*` | — | **DEFER** | il sorgente stesso dice *«creare solo esempi che servono davvero alle milestone»* |
| `SP-06` Authoring / Tactical Designer | `spec-tactical-designer.md` | epic #1105 | **LINK** | owner nato il 2026-08-17 con #1108 |
| `SP-07`…`SP-10` (Codex, integrazioni, balance, hardening) | epic `E43`–`E45` | #776, #777, #778 | **DEFER** | release `v0.8`–`v1.0`, epic già aperte |
| Nome «Skill Plus» | `spec-…-sinergie.md` §9 | — | **CONFLICT** | quarto termine per un vocabolario già deciso |
| 5 feature `RT-FEAT-*-SKILLPLUS-*` (§14) | — | — | **NOT NEEDED** | quattro delle cinque hanno un omonimo semantico; crearle sarebbe *Feature ID explosion* |

**Conteggio**: `REUSE 6 · CREATE 1 · LINK 2 · DEFER 5 · CONFLICT 1 · NOT NEEDED 1`.

**Una issue creata su ~70 candidate**, più una domanda di naming. Il rapporto è il risultato, non una
scorciatoia: 15 delle 16 righe hanno già un proprietario.

---

## 6. Cosa entra

1. **[#1132](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1132)** — i detriti come
   producer/consumer nella grammatica **esistente**, non come sistema nuovo. È il solo `CREATE`.
2. **[#1133](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1133)** — la domanda di
   naming (`SPN-1`), `question`, con sorgente A / sorgente B / differenza / proposta come il §1 chiede.

**Nessun `D-nnn` nuovo · nessuna feature nuova · nessuna epic nuova.** Le tre decisioni che il sorgente
propone come ADR candidate (§17) sono già prese: `D-029`/`ADR-0006` per la grammatica data-driven,
`ADR-0003` per l'autorità del resolver, e la privacy delle preview è il perimetro di `E40`.

---

## 7. Cosa NON è entrato, e perché

| Sezione | Perché |
|---|---|
| §12 (10 epic `SP-*`) | 9 su 10 hanno un owner esistente o una release che le differisce. Il sorgente stesso dice *«Non imporre questa numerazione se esiste uno schema Epic/Checkpoint corrente»* — e esiste: `E1`–`E47` |
| §14 (5 feature nuove) | `RT-FEAT-ABILITY-SKILLPLUS-CORE` → `ENV-SYSTEMIC-COMBOS`; `-UI-SKILLPLUS-AFFORDANCE` → `UI-CERTAINTY`/`-WARNINGS`; `-TOOLS-SKILLPLUS-AUTHORING` → `TOOL-SCENARIO-COMPOSER`. Solo `ENV-DEBRIS-SCATTER` non ha omonimo, e attende che #1131 dimostri il contenuto |
| §20 (6 voci `PIE-SKILLPLUS-*`) | il registro PIE è di un'altra track, e le voci **si propongono in handoff** — non si scrivono da fuori. Vale `D-139` |
| §18 (6 scenari) | cinque presuppongono debris o rete. Il sesto (privacy canary) è già il perimetro di #784 |
| §22 (12 metriche di telemetria) | `RT-FEAT-TOOL-BALANCE-GROUND` è l'owner, e `E43` (#776) la milestone |
| §26 (`release: v1.0`) | lo schema del registry ammette `v0.1…v0.4 \| future`. Il sorgente lo dichiara da sé al §2.2 e chiede di non scrivere `v1.0` alla cieca: rispettato |

---

## 8. Limiti

- **Nessuna build, nessun test Unreal**: il write-set è `docs/` e GitHub.
- **`RT-FEAT-ENV-SYSTEMIC-COMBOS` non è stata riverificata sul codice.** Il suo `status: INTEGRATED` è
  letto dal registry, e il registry dichiara `last_verified` — non l'ho rimisurato eseguendo i tre test
  che nomina. Chi apre #1131 lo faccia prima di appoggiarcisi.
- **Il Feature Registry non è stato modificato**, benché fosse libero da PR aperte: nessuna feature
  cambia stato per effetto di questo consolidamento, e `RT-FEAT-ENV-DEBRIS-SCATTER` nasce quando #1131
  ha un contenuto, non prima.
- Il gate `check-docs-symbols` **non copre questa cartella** (`EXEMPT_DIRS` include
  `docs/roadmap/plans/`): i simboli citati qui sono verificati a mano.
- ⚠️ **Nessuna delle due issue ha un parent epic**, e non è una dimenticanza: `E8` (#22) è `CLOSED`, e
  agganciare lavoro nuovo a un'epic chiusa la riaprirebbe di fatto. #1132 lo dichiara nel proprio
  `Tracking` come decisione di triage; #1133 è una domanda editoriale e non ne ha bisogno.
