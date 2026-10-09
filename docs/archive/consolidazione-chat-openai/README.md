# `consolidazione-chat-openai/` — quello che il triage del 2026-08-09 non ha consumato

> `HISTORICAL` · **Materiale NON autorevole** · **Dichiarato il 2026-09-20**
> **Cosa è**: il residuo del giro di consolidamento delle chat ChatGPT del 2026-08-08/09.
> **Cosa non è**: una fonte. Niente qui risolve un conflitto, e niente qui è stato verificato contro
> `main` dopo l'agosto 2026.

Questa cartella contiene **un file solo**:
[`RT_Common_Actions_Master_Consolidation_v0.1.md`](RT_Common_Actions_Master_Consolidation_v0.1.md).

| | |
|---|---|
| Origine | il master del giro chat del 2026-08-09, `Wait · Move · BasicAttack · Guard · Brace · Activate · Interact · Overwatch`, profili di Move, movimento speciale e facing |
| Triage che lo ha lasciato qui | [`consolidamento-chat-openai-triage-2026-08-09.md`](../../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md), riga *«l'unico rimasto»* — §8.2 |
| Disposizione | ⬜ **non recepito**: nessun owner lo ha consumato, e nessuna issue aperta lo cita |
| Owner della decisione | il perimetro di [#2606](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2606) — è il suo **primo candidato** |

## Perché questa pagina esiste

Prima del 2026-09-20 la cartella non aveva un README, e la conseguenza era strutturale, non estetica: i
sorgenti di [`../src/`](../src/README.md) ereditano `HISTORICAL` **dal contenitore**, questo no. Un lettore
che apriva il file trovava un documento del 2026-08-09 con un banner di rinomina roster e nessuna riga che
dicesse *se quello che afferma valga ancora*. ⚠️ È il difetto che il primo giro di #2606 ha misurato e
nominato — [`chat-knowledge-consolidation-2606-primo-giro-2026-09-06.md`](../../roadmap/plans/chat-knowledge-consolidation-2606-primo-giro-2026-09-06.md)
§6 — senza chiuderlo, perché chiuderlo *sembrava* richiedere di giudicarne il contenuto.

🔑 **Non lo richiede, e la distinzione è il punto**: dichiarare lo **stato del contenitore** è un fatto
misurabile — *nessun owner lo cita, nessun referto lo ha consumato* — mentre dichiarare cosa di quelle 470
righe sopravviva è un giudizio che appartiene a chi lo recepirà. Qui si scrive il primo e si lascia il
secondo.

## ⛔ Quello che questa pagina NON dice

- **non** dice che il contenuto sia superato: dice che **non è stato verificato**. Sono cose diverse, e
  confonderle è il modo in cui un documento utile viene buttato o uno stantio viene creduto;
- **non** apre una issue. Verificare quali affermazioni del master siano ancora vere dopo centinaia di
  commit **è** una passata di consolidamento, non il suo prerequisito: aprire ora una issue
  *«verificare Common Actions»* creerebbe un owner senza misura, che è il difetto registrato da
  [#2533](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2533);
- **non** lo sposta. Il file sta dove il triage lo ha lasciato, e la sua provenienza è leggibile dal
  percorso.

## Se lo consumi

Chi lo recepisce scrive qui la **disposizione** — cosa sopravvive, cosa è falsificato, da quale owner —
nella forma già in uso in [`../src/README.md`](../src/README.md), e cita il referto che lo ha consumato.
Finché quella riga non c'è, la casella resta ⬜.

⚠️ Il master dichiara in proprio un **conflitto documentale aperto** fra due decisioni (§2). Chi lo
consuma non lo risolve leggendo il master: la gerarchia sta in
[`../../DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) §*Chi prevale*, e l'owner è il
[Decision Log](../../decisions/RT_PDR_00_Decision_Log.md).
