# RT — Catalogo dei profili di footprint v0.1

> **Decisione abilitante**: [`D-303`](../decisions/RT_PDR_00_Decision_Log.md) (il footprint è un conteggio di
> settori contigui, non un raggio) · **Valori fissati da**: [`D-307`](../decisions/RT_PDR_00_Decision_Log.md)
> **Owner del modello**: [`spec-cover-placement-intra-hex.md`](../technical/systems/spec-cover-placement-intra-hex.md) §13.0
> **Consumatore previsto**: `E23.6` ([#1827](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1827))
>
> ⚠️ **Questo file è l'owner dei tre numeri.** La spec e la voce di decisione ne portano la derivazione, non
> una seconda copia dei valori: se divergono, vince questo file.

🔴 **Nessuna unità dichiara ancora un footprint.** Questo catalogo esiste come **sorgente d'autore**, nella
sede in cui questo repository tiene i numeri (`docs/balance/`), e non ha ancora un consumatore in `Source/`.
Non è una svista: è la stessa forma di [`RT_HeroCatalog_v0.1.md`](RT_HeroCatalog_v0.1.md) prima che `E6`
lo consumasse.

---

## I tre profili

| Profilo | `MinContiguousWedges` | Arco | `bRequiresFreeCore` |
|---|---|---|---|
| `Small` | **2** | 60° | `false` |
| `Medium` | **3** | 90° | `false` |
| `Large` | **4** | 120° | `false` |

Il default di `FRTFootprintProfile` resta **`1`** — l'identità: è il valore di chi non ha ancora dichiarato
un profilo, non una quarta taglia.

---

## Da dove vengono i numeri

⚠️ **Due sono determinati, il terzo è il minimo disponibile.** Questa sezione dice quale è quale, perché un
numero determinato e uno scelto non si difendono allo stesso modo quando qualcuno vorrà cambiarli.

### ① `Small ≥ 2` — determinato

`MinContiguousWedges = 1` è **l'identità**: con `1` basta **un** settore libero perché la cella sia
calpestabile, quindi un profilo a `1` non distingue nulla da nessun profilo.
[`D-289`](../decisions/RT_PDR_00_Decision_Log.md) lo dice del default — *«il default di
`FRTFootprintProfile` è l'**identità** — un settore — e non un numero»* — mentre la formulazione
*«l'identità, che non decide niente»* è dell'istruttoria di `COV-1` in
[`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md), non di `D-289`.
∴ la prima taglia descrivibile è `2`.

### ② `Medium ≤ 3` — determinato, ma è un limite SUPERIORE

`D-289` giustifica il superamento di `FRTOccupancyThresholds::BlockedFrom = 6` così:

> *«rocce sui settori `1,2,3` più un albero su `7,8,9` lasciano **sei** settori liberi in **due gruppi da
> tre**, e uno ci sta benissimo»*

⚠️ **Questa frase stabilisce `≤ 3`, non `= 3`**: è soddisfatta anche da un footprint di `1` o `2`. Dice che
l'unità standard **ci sta** in tre settori, non che ne **abbia bisogno** di tre. Leggerla come uguaglianza
sarebbe metterle in bocca una precisione che non ha.

### Perché `Small = 2` e `Medium = 3` sono comunque determinati

Non da ② da sola, ma da ① e ② **insieme all'ordinamento** `Small < Medium`. Con `Small ≥ 2` e `Medium ≤ 3`
l'unica coppia possibile è `(2, 3)`. ✅ **Qui non c'è scelta da fare**, ed è la parte della derivazione che
regge da sola.

### ③ `Large = 4` — il minimo disponibile, ed è una SCELTA

`Large > Medium = 3` impone `Large ≥ 4`, e `4` è il **primo valore possibile**. Nulla nella geometria lo
obbliga a fermarsi lì: `5`, `6` o più sarebbero altrettanto esprimibili, e direbbero soltanto *«un'unità
grande ha bisogno di più spazio contiguo»*.

⛔ **Scegliere il minimo è la scelta conservativa**, e la ragione è che non esiste un consumatore che possa
smentirla: nessuna unità dichiara un footprint, quindi un `Large` più esigente sarebbe bilanciamento deciso
senza nessuno che lo misuri — precisamente il difetto che `BlockedFrom = 6` è stato.

🔎 **Una verifica utile, che NON è il vincolo che produce il numero.** Un muro **diametrale** occupa
**quattro** settori — i due a cavallo di ciascuno dei due confini opposti che l'asse buca — e lascia **due
regioni da quattro**. Con `Large = 4` un'unità grande sta ancora in una cella tagliata a metà; con `Large =
5` non ci starebbe più. ⚠️ **Non è ciò che determina `4`**: è ciò che `4` fa guadagnare, e va detto come
tale. Se un domani si volesse `Large = 5`, questa riga è la conseguenza da accettare, non un divieto.

---

## Cosa dicono i tre profili sulle configurazioni misurate

| Configurazione | Settori liberi | `Small` (2) | `Medium` (3) | `Large` (4) |
|---|---|---|---|---|
| Cella vuota | 12, una regione | ✅ | ✅ | ✅ |
| Muro diametrale (4 occupati) | 8, **due regioni da 4** | ✅ | ✅ | ✅ *(esatto)* |
| Due ostacoli (`{1,2,3}` e `{7,8,9}`) | 6, **due regioni da 3** | ✅ | ✅ | ❌ |
| Sei in fila (`{0..5}` occupati) | 6, **una regione da 6** | ✅ | ✅ | ✅ |

🔑 **La terza riga è il punto di avere tre taglie**, e va letta con attenzione: è l'esempio canonico di
`D-289`, e il `Large` **non** ci sta. Non contraddice ②, perché `D-289` parla dell'unità **standard**
(*«uno ci sta benissimo»*), non di ogni unità. ⚠️ Il senso stesso di avere taglie è che qualcuna fallisca
dove altre passano: un criterio che pretendesse *«nessun profilo fallisce mai su una cella che `D-289`
dichiara calpestabile»* collasserebbe le tre taglie in una.

🔎 Terza e quarta riga hanno lo **stesso conteggio** di settori liberi — sei — e risposta diversa, perché
ciò che decide è la **contiguità**. È la dimostrazione che
`RefactorTactics.CoverPlacement.FootprintDecidesStandabilityNotTheCount` porta già in `Source/`.
⚠️ Quel test nomina `Small` il profilo a **3** e `Large` quello a **4**: sono variabili locali scritte prima
di questo catalogo, e con `D-307` il suo *«Small»* è il `Medium` di qui.

---

## La misura del diametro, e perché non è una costante

Un muro diametrale occupa **quattro** settori e lascia **due regioni da quattro**, su **ognuno** dei sei
assi tattici. ⛔ **La maschera letterale non si scrive qui**: dipende dall'asse — `Deg0` dà `{0,1,6,7}`,
`Deg30` dà `{1,2,7,8}`, e così via — e la corrispondenza asse → confine vive in
`URTGeometryGrammarLibrary::AxisBoundaryIndex`, che ne è l'unico posto.

🔑 È la stessa disciplina che il test si impone: *«una tabella di costanti qui sarebbe la copia che diverge,
e questo test non misurerebbe più la regola ma la propria costante»*. L'invariante è **quattro settori, due
regioni da quattro**, ed è ciò che
`RefactorTactics.CoverPlacement.DiameterOccupiesFourSectorsOnEveryAxis` pinna su tutti e sei gli assi.

✅ La misura è su `main` da [`D-306`](../decisions/RT_PDR_00_Decision_Log.md), che ha chiuso `MSE-4`
sull'intersezione di lunghezza non nulla: prima di quella voce un muro passante accendeva **dodici** settori
su dodici, e nessuno dei tre profili sarebbe stato in quella cella.

---

## Cosa questo catalogo NON dichiara

- ⛔ **Nessun profilo richiede il centro libero.** `bRequiresFreeCore` resta `false` per tutti e tre, e non è
  una derivazione: è l'assenza di un consumatore. Il campo esiste per *«un'unità grande che deve stare a
  cavallo del centro»* (`FRTFootprintProfile`, `D-289` punto 2), ma **quale** unità lo sia è una scelta di
  contenuto, e nessuna unità esiste ancora. ⚠️ `false` qui significa *«non ancora acceso da nessuno»*, non
  *«deciso di no»*: il primo profilo d'autore che descriva un'unità a cavallo del centro lo accende, e non
  serve toccare `D-307` per farlo.
- ⛔ **Nessuna relazione con l'occupancy.** Un'unità `Large` occupa **uno** slot come tutte: `Large` significa
  *«più difficile da piazzare»*, non *«più posto occupato»* — invariante di `D-289` punto (4).
- ⛔ **Nessun profilo per la transizione fra celle.** Il corridoio spazzato è `MAP-4` in
  [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md): un conteggio di settori non si trasla lungo `A→B`.
- ⛔ **Nessuna assegnazione unità → profilo.** Quale eroe sia `Medium` non si decide qui, e oggi non si decide
  affatto: i quattro eroi di [`RT_HeroCatalog_v0.1.md`](RT_HeroCatalog_v0.1.md) non dichiarano una taglia, e
  nessuna creatura non umanoide è prevista né in [`../roadmap/roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md)
  né in [`../roadmap/roadmap-post-v0.1.md`](../roadmap/roadmap-post-v0.1.md).
  ∴ **oggi userebbero tutti `Medium`**, e `Small`/`Large` sono capacità per contenuto futuro.
