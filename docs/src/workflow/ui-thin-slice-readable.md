# Mock leggibile — Thin Slice v0.1

Questo file mostra **come leggere** il thin slice senza grandi diagrammi ASCII.

## Reactions

| Nodo | Lane | Hard prerequisite | Soft order | Stato atteso dalla sorgente |
|---|---|---|---|---|
| #165 | CODE | #164 chiusa | — | OPEN, candidato READY |
| #166 | CODE | #165 | — | BLOCKED |
| #314 | CODE | #165 | dopo #166 | BLOCKED finché #165 è open |
| #319 | CODE | — | dopo #314 | queued/soft order, non “blocked” per il solo `follows` |
| #512 | CODE | #165 | — | BLOCKED |

### Cosa deve succedere cliccando #165

Mostrare:
- upstream hard;
- downstream hard (#166, #314, #512);
- capability `DecisionBoundary`;
- Feature collegate;
- scenari che richiedono `DecisionBoundary`.

Non mostrare tutta la roadmap.

---

## Golden / Showcase

| Nodo | Lane | Hard prerequisite | Soft / related |
|---|---|---|---|
| #625 | CODE | — | meglio prima del golden |
| #512 | CODE | #165 | — |
| #75 | CODE | #74 | — |
| #170 | CODE | #512, #66, #75 | #625 prima; #649/#687 related |
| #171 | PIE | #170 | — |

### Cosa deve succedere cliccando #170

Mostrare il junction:
- DecisionProvider;
- objective;
- environment;
- consistency work soft/related.

Poi mostrare `#171` come uscita PIE.

---

## Character Presentation

| Nodo | Lane | Tipo | Output |
|---|---|---|---|
| U7 | ASSET | sessione umana | Blueprint unità |
| U8 | ASSET | sessione umana | Anim BP/montage |
| U9 | PIE | sessione umana | giudizio + screenshot/video |
| #593 | CODE | bug non bloccante | neutral root / scaling |
| #715 | CODE | data boundary | DisplayName + HearingThreshold |
| #287 | — | checkpoint | E21.1 |
| #288 | — | checkpoint | E21.2 |
| #289 | — | checkpoint | E21.3 |

### Perché i checkpoint non devono per forza avere una lane

`#287/#288/#289` sono checkpoint di prodotto/roadmap.
Il **lavoro concreto umano** è già modellato da U7/U8/U9.

Quindi è legittimo che:
- checkpoint = nodo di coordinamento;
- sessione = nodo esecutivo ASSET/PIE.

Questo evita di contare due volte lo stesso lavoro.

---

## Test visuale del modello

Il thin slice è promosso se nel Control Center:

1. `follows` non rende rosso un nodo.
2. #593 non blocca U7.
3. #165 fa vedere tre consumer.
4. #170 appare come convergenza.
5. U7/U8 sono ASSET e U9 è PIE.
6. un click su U7 mostra PIE-AS2/PIE-FACING come evidence.
7. un click su #287 mostra U7 come implementazione.
8. nessuna logica di stato vive nel JavaScript.
