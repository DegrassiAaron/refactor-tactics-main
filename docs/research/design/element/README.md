# Ruota elementale — materiale di design

> `HISTORICAL` · **Materiale NON autorevole.** L'autorità sul vocabolario elementale è il
> [Decision Log](../../../decisions/RT_PDR_00_Decision_Log.md), non questa cartella.

Questa cartella contiene **tre** rappresentazioni della ruota degli otto elementi, e **non concordano
sull'ordine**. Senza questa nota chi apre la cartella non ha modo di sapere quale legge.

## Quale segue la decisione

| File | Ordine orario | Stato |
|---|---|---|
| [`element_relation_0.1.png`](element_relation_0.1.png) | `Fuoco → Vuoto → Aria → Armonico → Acqua → Elettricità → Terra → Acido` | ✅ **coincide con D-272** |
| [`element_relation_draft_2026-08-29_a.png`](element_relation_draft_2026-08-29_a.png) | `Fuoco → Terra → Aria → Armonico → Acqua → Vuoto → Elettricità → Acido` | 🔴 **superato** — precede la decisione |
| [`element_relation_draft_2026-08-29_b.png`](element_relation_draft_2026-08-29_b.png) | non ricostruibile con certezza (vedi sotto) | 🔴 **superato** |

**D-272** *(accettata il 2026-08-30)* fissa l'ottagono orario a
`Fire → Void → Air → Harmonic → Water → Electric → Earth → Acid`. I due draft sono del **2026-08-29**:
lo precedono di un giorno, e scambiano `Terra` e `Vuoto` rispetto ad esso.

⚠️ **Nel draft `b` le etichette non si appaiano univocamente ai cerchi**: due posizioni portano la stessa
icona a onda, e `VUOTO` compare come seconda riga sotto `ACQUA` invece che accanto al proprio cerchio.
Per questo la sua colonna qui sopra è vuota: ricostruirne l'ordine sarebbe un'inferenza, non una lettura.

## Cosa la ruota NON dice

D-273 e D-275 sono espliciti, e vanno letti prima di usare queste immagini per qualunque cosa:

- la distanza sull'ottagono definisce classi di relazione **strutturali** — **non** è una tabella di
  moltiplicatori di danno o resistenza;
- l'ottagono **non produce automaticamente** reazione, danno, resistenza o status: solo un `ReactionId`
  stabile con dati di effetto espliciti crea un comportamento;
- `Element` / `Surface` / `Status` / `Reaction` restano quattro cose distinte.

Le caselle di reazione disegnate in `element_relation_0.1.png` sono **esempi illustrativi**, e la loro
autorità è il catalogo delle 28 coppie previsto da D-275 — non l'immagine.
