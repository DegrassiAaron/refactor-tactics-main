# I quattro pannelli del Bot AI

> `CURRENT` · **Rinumerati**: 2026-08-17 · **Autorità**: nessuna — sono una *mini overview*, non la spec.
> L'owner del bot è la roadmap e sono le issue che questi pannelli citano.

Quattro tavole verticali che raccontano come il bot decide. A differenza della
[generazione v0.1](../v0.1/README.md), qui **non c'è alcun nome di personaggio**: parlano di pipeline,
punteggi e conoscenza, quindi il blocco dei nomi non le tocca.

## 🔴 I file erano numerati in un ordine che non era il loro

Ogni immagine dichiara la propria posizione in alto a destra (`1 / 4` … `4 / 4`). Aprendole, tre nomi su
quattro puntavano al pannello sbagliato — e `bot01` era il secondo, non il primo:

| Nome precedente | Pannello reale | Nome attuale |
|---|---|---|
| `bot04.png` | **1 / 4** — Panoramica | `bot-01-panoramica.png` |
| `bot01.png` | **2 / 4** — Decisione & Scoring | `bot-02-decisione-e-scoring.png` |
| `bot03.png` | **3 / 4** — Knowledge, Perception & Privacy | `bot-03-knowledge-percezione-privacy.png` |
| `bot02.png` | **4 / 4** — Mini Roadmap 0.1 | `bot-04-mini-roadmap.png` |

⚠️ **Un nome puramente ordinale non ha modo di sbagliare in modo visibile**: `bot01` non afferma niente che
si possa confrontare con l'immagine, quindi l'errore non aveva superficie. I nomi attuali dicono anche il
contenuto, e da oggi sono falsificabili aprendo il file. È lo stesso difetto che
[`../facing/README.md`](../facing/README.md) documenta su due diagrammi, con una causa diversa.

## Cosa contengono

| Pannello | Sostanza |
|---|---|
| 1 — Panoramica | obiettivo `2v2` locale, quattro principi, e la pipeline in otto stadi da `GAME STATE` a `RESOLUTION` |
| 2 — Decisione & Scoring | i tre stadi vivi (`BuildCandidates → ScorePlans → ChooseBestPlan`) e le sette voci del punteggio, `Risk` sottratto |
| 3 — Knowledge & Privacy | cosa il bot sa, cosa non deve sapere, e i tre livelli `Confermato / Probabile / Sconosciuto` su `FRTTeamKnowledge` |
| 4 — Mini Roadmap | la `Definition of Done 0.1` e la sequenza `B0`–`B6`, ciascun passo con la sua issue |

✅ **Il formato dichiarato è `2v2`**, coerente con il canone — cosa che i poster della v0.1 non fanno
([`../v0.1/README.md`](../v0.1/README.md)).

## ⚠️ Sono una fotografia, e le fotografie invecchiano

I pannelli 2 e 4 citano **dieci issue** per numero e il pannello 2 ne promuove una a *«blocker 0.1
attuale»*. Verificato il 2026-08-17: quella issue è ancora **aperta**, quindi il pannello non mente — ma il
suo titolo si è nel frattempo allargato da *«stallo sulla mappa d'autore»* a un comportamento più generale
e predefinito. Le altre nove non sono state rimisurate.

**Non sono una dashboard.** Lo stato vivo si legge dalle issue e dai file generati della roadmap; questi
pannelli spiegano *come funziona il bot*, e quella parte non scade con lo stato delle issue.
