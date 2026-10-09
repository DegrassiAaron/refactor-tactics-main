# Authoring dei dati di mappa — la forma dei numeri, e dove nessuno ti protegge

> **Tipo**: runbook di authoring · **Creato**: 2026-10-03 · **Owner della riserva**:
> [`D-450`](../../decisions/RT_PDR_00_Decision_Log.md), che chiude
> [`GATE-4`](../../OPEN_DECISIONS.md).
>
> **Perché esiste**: `D-450` prescrive che il troncamento silenzioso del pannello Details sia
> *«accettato e **dichiarato** nel runbook di authoring»*. Quel runbook **non esisteva** — `MoveCost`
> era documentato solo sotto `docs/archive/` — e una riserva dichiarata dove nessuno autora non è
> dichiarata. Questo file è il posto in cui la incontri.

---

## 1. ⛔ Il pannello Details accetta `2.7` in un campo intero, e lo tronca in silenzio

È il difetto che `D-450` ha deciso di **non** presidiare per la v0.1. Leggilo prima di autorare un
costo, perché nessuno te lo dirà dopo.

`MoveCost` (`Source/RefactorTactics/Map/RTHexCellData.h`) e `RoundLimit`
(`Source/RefactorTactics/Turn/RTMatchFormatData.h`) sono `int32`. Nel pannello Details di Unreal il
campo **accetta** che tu digiti `2.7` — `IsCharacterValid` ammette esplicitamente il punto — e il
valore viene **distrutto prima** che qualunque `UPROPERTY` lo veda:
`TDefaultNumericTypeInterface::FromString` fallisce il parse decimale per un tipo integrale, cade
sull'`FBasicMathExpressionEvaluator`, e il risultato passa per un cast C — **troncamento verso lo
zero**.

🔴 **Nessuna traccia del `2.7` sopravvive.** Non c'è un file da `grep`are, non c'è un campo da
ispezionare, non c'è una riga di log. Hai scritto una cosa e il gioco ne gioca un'altra, senza un
errore, senza un avviso.

⛔ **E nessun gate può accorgersene**, per la stessa ragione: `G7` presidia le altre tre superfici del
suo perimetro — il catalogo in codice, le sottoclassi Blueprint, il corpus testuale
`Scenarios/**/*.json` — e su questa **non c'è niente**. `G7` è ✅ perché è soddisfatto su tutte le
superfici in cui la domanda è ponibile; qui non lo è.

---

## 2. ✅ La regola operativa, in una riga

**Digita solo interi.** Se ti serve un costo che non è intero, il problema non è il campo: è il
modello, e si decide — non si arrotonda a mano.

| se ti serve… | fai |
|---|---|
| un terreno che costa «un po' più» | usa il prossimo intero, e se la granularità non basta apri la domanda invece di scrivere `1.5` |
| verificare cosa è stato davvero salvato | riapri l'asset e **rileggi il campo**: quello che vedi è quello che il gioco gioca |
| un valore che il modello non esprime | una issue, non un decimale |

⚠️ **Se sospetti di aver digitato un decimale**, non cercarlo: non c'è. Riapri il campo e confronta col
valore che intendevi — è l'unico controllo disponibile, e sta a te.

---

## 3. 🔑 Perché non è stato presidiato, e cosa costerebbe

`D-450` ha scartato tre direzioni, e le ragioni sono misurate — se un giorno si riapre la domanda,
queste sono le righe da rileggere invece di riderivarle:

| direzione | perché scartata |
|---|---|
| una `IsDataValid` che **rifiuti** | del difetto cattura **zero**: il token è già distrutto prima che l'asset veda il valore. L'hook esiste (`URTHexMapAsset::PostEditChangeChainProperty`, `D-430`) ma guarda il valore **dopo** la coercizione |
| una **detail customization** che tenga il valore autorato | è l'unica via che intercetta davvero il widget, ed è **greenfield due volte**: `grep -rl "IDetailCustomization\|IPropertyTypeCustomization" Source/` → **0 file**. Nessuna customization è mai stata registrata in questo progetto |
| spostare l'authoring sul **testuale** | non elimina la coercizione: la **sposta e ne cambia il verso**. Il JSON **arrotonda** (`FMath::RoundHalfFromZero`), quindi `2.7` darebbe `3` invece di `2` — un difetto diverso, non uno in meno |

⛔ **E il vincolo che nessuna direzione futura può toccare**: `int32` resta la forma che entra in
`ComputeHash`. `FRTHexCellData` è **stato canonico**, e `G7` esiste esattamente per impedire che un
tipo a virgola mobile ci finisca dentro. Una soluzione che mette il valore autorato in `double`
risolve il sintomo **creando** il difetto che il gate previene. Qualunque altra strada passa per un
`D-nnn` che lo autorizzi esplicitamente.

---

## 4. ⚠️ La riserva ha una scadenza

`D-450` non è un giudizio sul difetto: è un giudizio sul suo **costo oggi**, in una v0.1 offline con
un solo autore. Va ripresa se accade una di queste:

- i costi diventano autorabili da **più mani**;
- entra una **seconda superficie** di authoring numerico competitivo;
- il difetto **morde** — e se morde, morde **nell'hash**: un `MoveCost` sbagliato cambia
  `ComputeHash`, quindi cambia il determinismo e il replay, non solo il pathfinding.

🎯 Gli asset esposti oggi sono i data asset di mappa `DA_HexMap_*`. Si contano così, invece di fidarsi
di un numero scritto qui:

```bash
find Content -iname "DA_HexMap_*.uasset" | wc -l
```
