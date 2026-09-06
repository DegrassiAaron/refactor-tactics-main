# Consolidamento della conoscenza delle chat — primo giro di #2606

> `CURRENT` · **Stato**: primo giro chiuso, perimetro **non** chiuso · **Data**: 2026-09-06
> **Base di misura**: `origin/main` @ `c5412238` (2026-09-06 16:24), letta in un worktree pulito
> (`D:/rt-wt-2606`) — i due checkout condivisi ospitavano lavoro altrui e nessuna misura è stata presa lì.
> **Drive**: *RT — Knowledge Index & Consolidation Log* (`1GOd_Hi3bZBM0NMXQ7oAKV8XxlzPdywtpgCjWtsZsoeo`),
> letto il 2026-09-06.
> **Owner**: [#2606](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2606) · **Panel**:
> Wiegers (lead) · Fowler · Hohpe · Nygard · Cockburn · Crispin · Adzic · **Modo**: critique
> **Cosa non è**: non è una roadmap, non ristruttura `docs/` ([#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165)),
> non tocca la navigazione delle capability ([#2325](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2325)),
> non dice nulla sullo stato della v0.1 ([#14](https://github.com/DegrassiAaron/refactor-tactics-main/issues/14)).

---

## In una riga

Il registro che #2606 prescriveva **esiste già in due sedi**, la sua tassonomia **ne duplicava una terza**, e
il difetto misurabile non è quello che l'issue immaginava: **16 sorgenti archiviati su 124 non hanno una riga
d'indice che ne dichiari la disposizione** — l'indice stesso ne dichiara **tre**.

---

## 1. Il registro esisteva già, due volte

Il corpo di #2606 prescrive un registro di decommissioning con owner, issue collegata, materiale superseded,
verifica e stato finale. **Esiste già**, applicato ai documenti invece che alle chat:

| Sede | Oggetto | Semantica già in uso |
|---|---|---|
| [`docs/archive/src/README.md`](../../archive/src/README.md) — `HISTORICAL`, *materiale NON autorevole* | i sorgenti archiviati | riga per riga: cosa il documento sosteneva, referto che l'ha consumato, disposizione (*«Applicato in parte»*, *«Recepito come conferma, non come delta»*), issue collegate, **cosa è stato respinto e perché** |
| Google Sheet *RT — Knowledge Index & Consolidation Log*, tab sorgenti | documenti Drive, cartelle, PDF | `Source · Type · Domain · Disposition · Authority class · Repo owner / decision · Retained · Superseded / corrected` |

Scriverne un terzo avrebbe violato il guardrail che #2606 dichiara in proprio — *«non creare una seconda
Source of Truth»*, [`AGENTS.md`](../../../AGENTS.md) §8. **Non è stato scritto.** #2606 estende questi due.

## 2. Due tassonomie, due assi — e vanno dichiarati tali

#2606 introduce `FACT` · `DECISION` · `EXISTING_ISSUE` · `NEW_GAP` · `OPEN_QUESTION` · `HISTORICAL` ·
`SUPERSEDED`. Il Drive usa già `WORKING` · `REPO-CANONICAL` · `PARTLY REPO-CANONICAL` ·
`PROPOSED — NON-NORMATIVE` · `NON-AUTHORITATIVE MIRROR` · `HISTORICAL` · `SUPERSEDED`.

**Due termini su sette coincidono**, e la sovrapposizione parziale è peggio di nessuna: invita a tradurre
l'una nell'altra. Non sono la stessa scala:

- la tassonomia di #2606 classifica **l'elemento estratto** → *dove deve atterrare?*
- la tassonomia del Drive classifica **la fonte** → *quanta autorità ha, e chi la possiede nel repository?*

Un elemento `DECISION` esce legittimamente da una fonte `WORKING`. La riconciliazione è questa
dichiarazione, non un rename: rinominare avrebbe rotto 324 righe di foglio per un problema che non c'era.

## 3. Il difetto misurabile: 16 sorgenti senza riga d'indice

**Selettore dichiarato**: un sorgente è indicizzato se `README.md` contiene una **riga di tabella** che lo
linka — `^\|.*\]\([^)]*<basename>\)`. Menzione dentro una nota di rimisura **non** conta: quattro dei
sedici sono nominati proprio dalla nota che li dichiara mancanti.

```bash
find docs/archive/src -name '*.md' ! -name README.md            # 124
find docs/archive/src/handoff -name '*.md' ! -name README.md    #  76
```

| Misura | Valore su `c5412238` | Cosa dichiara l'indice |
|---|---:|---:|
| sorgenti archiviati | **124** | 106 |
| di cui in `handoff/` | **76** | 60 |
| **senza riga d'indice** | **16** | **3** |

I sedici:

`CLAUDE_Apply_Elemental_Proficiency_Consolidation_2026-08-16.md` ·
`CLAUDE_Reconcile_v0.1_Skill_Ability_Issues_2026-08-16.md` ·
`2026-08-29-roadmap-issues-v01-v10-prompt.md` ·
`2026-08-29-tactical-designer-devsandbox-launcher.md` ·
`2026-08-29-td-trial-scenario-sandbox.md` ·
`2026-08-30-animation-issue-orchestrator.md` ·
`2026-08-31-chatgpt-governance-handoff.md` ·
`2026-08-31-player-event-log-issue-epic-docs.md` ·
`2026-08-31-roadmap-1.0-v01-execution-prompt.md` ·
`2026-09-03-claudia-handoff-v01-refresh.md` ·
`2026-09-05-hud-v01-rt-three-terminals.md` ·
`RefactorTactics_DeepResearch_DamageModel_E49_2026-08-27.md` ·
`RefactorTactics_v0.1_Characters_Elemental_Consolidation.md` ·
`RT_ClaudeDesign_Prompt_SkillGrammar_v0.1.md` ·
`RT_SkillVisualGrammar_Handoff_v0.1.md` ·
`RT_SkillVisualGrammar_OpenQuestions_v0.1.md`

🔮 **La previsione verificabile dell'indice non ha retto, e il modo in cui non ha retto è il punto.**
`README.md` prometteva *«quando questo lavoro atterra, il comando canonico risponde `106` con `60`, e lo
scarto indice-archivio resta `tre` finché le tre righe mancanti non le scrive chi ha consumato quei kit»*.
Oggi risponde `124` con `76`, e lo scarto è `16`. **La previsione non è stata falsificata da un errore: è
stata superata dall'arrivo di altri diciotto sorgenti**, ciascuno col proprio scarto. Il numero `tre` non è
sbagliato per il giorno in cui è stato scritto — è **stantio**, e un lettore che oggi legge *«sono 106
archiviati e 103 con una riga»* prende per misura una fotografia di due giorni fa.

⛔ **Queste sedici righe non le scrive questo referto**, e il motivo è dell'indice, non mio:

> *«una riga d'indice dichiara cosa sopravvive e cosa è falsificato di un kit, e quel giudizio appartiene a
> chi l'ha revisionato»*

Nessuno dei sedici è stato consumato qui. Scriverne la disposizione significherebbe giudicare cosa
sopravvive di 1955 righe di handoff — nel solo caso dei due `2026-08-29-*` — senza averle revisionate: è
esattamente la conversione di una proposta in fatto che #2606 vieta. **Il difetto si nomina e si misura; a
chiuderlo è chi ha consumato ciascun kit.**

## 4. Il falso positivo, registrato perché è il difetto tipico di questo lavoro

La prima misura di questo giro diceva *«43 handoff su 76 non dichiarano il proprio stato»*. Era **falsa**, e
si è sgonfiata tre volte mentre il selettore migliorava:

| Selettore | Risultato |
|---|---:|
| stato cercato nelle prime 3 righe del file | 43 su 76 |
| stato cercato in tutto il file | 31 su 76 |
| stato cercato **anche nel contenitore** (`archive/src/README.md` è `HISTORICAL` per l'intero lotto) | **0 rilevanti** |
| domanda diversa — *chi ha una riga di disposizione?* (§3) | **16 su 124** |

**Lo stato di un documento può essere dichiarato dal suo contenitore.** Un audit che cerca l'etichetta solo
dentro il file misura il proprio selettore, non il repository — ed è il modo in cui questo giro avrebbe
prodotto un debito inventato di 43 voci. Le prime tre righe della tabella non sono tre errori: sono lo
stesso errore che si accorcia man mano che si guarda dove il repository dichiara davvero le cose.

## 5. Il Drive: governato, e misurato contro uno stato di 44 ore fa

Il foglio dichiara in proprio il ruolo giusto — *«Curated external knowledge index. It is not a Feature
Registry and is not a new normative authority»* (`D-181` / `D-210` / `D-246`) — e nomina gli owner del
repository invece di sostituirli. Non serve correggerlo.

⚠️ **Ma dichiara `Observed HEAD = 29a3f517`** (2026-09-04 20:00). `origin/main` è a `c5412238`:
**488 commit**, ~44 ore. Le **53 righe** `pending repo sync` del foglio sono contate contro quello stato, e
`D-295` ha già misurato lo scarto una volta — *«il Drive dichiarava 31 pendenze e ne aveva 4»*.
**53 non è un debito: è un limite superiore mai rimisurato.** Rimisurarlo è lavoro del perimetro, non di
questo giro.

## 6. Un residuo senza owner, e non è stata aperta una issue

[`docs/archive/consolidazione-chat-openai/RT_Common_Actions_Master_Consolidation_v0.1.md`](../../archive/consolidazione-chat-openai/RT_Common_Actions_Master_Consolidation_v0.1.md)
è l'unico file della sua cartella, **senza README che ne dichiari lo stato** — quindi, a differenza dei
sorgenti di `archive/src/`, non ne eredita nessuno — e nessuna issue aperta lo cita. Il triage del
2026-08-09 lo lasciava fuori *«finché nessun owner lo recepisce»*, e nessuno l'ha recepito in 28 giorni.

**Non è stata aperta una issue**, e non per prudenza formale: verificare quali affermazioni del master siano
ancora vere dopo 488+ commit **è** una passata di consolidamento, non il suo prerequisito. Aprire ora una
issue che dice *«verificare Common Actions»* creerebbe un owner senza misura — il difetto che
[#2533](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2533) ha già registrato. È il primo
candidato del perimetro.

## 7. Il blocco, dichiarato e non aggirato

**Il perimetro delle chat non è enumerabile da nessuna fonte accessibile.** Repository, GitHub e Drive
contengono le *tracce* delle chat — handoff, kit, referti — ma nessuno dei tre porta la lista delle
conversazioni vive: nella colonna `Source` del foglio non c'è una riga che sia una conversazione.

Senza quella lista, `unresolved durable knowledge = 0` non ha denominatore, e qualunque dichiarazione di
copertura sarebbe inventata. **Owner della domanda: l'autore.** È la prima `OPEN_QUESTION` del perimetro,
ed è registrata come tale invece di essere risolta a intuito.

---

## Cosa questo giro NON ha fatto, e perché

| Non fatto | Perché |
|---|---|
| le 16 righe d'indice | il giudizio appartiene a chi ha consumato ciascun kit (§3) |
| una issue per `RT_Common_Actions_Master` | sarebbe un owner senza misura (§6) |
| rimisurare le 53 righe `pending repo sync` | è lavoro del perimetro, e richiede il perimetro (§5) |
| toccare `Source/` | nessun cambiamento di runtime: non applicabile, non `NOT RUN` |
| build UE, test automation, scenari | idem — nessun Editor era in esecuzione, e la verifica di questa issue è documentale |
| chiudere #2606 | il perimetro non è chiuso, e il blocco di §7 ha un owner che non è questo workflow |

**Nessuna suite eseguita**: il giro non tocca `Source/`. Non è un gate saltato — è un gate che non si applica.
