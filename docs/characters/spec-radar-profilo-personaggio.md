# RT — Radar di profilo del personaggio

> **Owner** del modello di rappresentazione radar: che cosa sono i due radar, quali assi hanno, che
> scala usano e — soprattutto — **che cosa non sono**.
> **Decisione abilitante**: [D-105](../decisions/RT_PDR_00_Decision_Log.md).
> **Feature**: `RT-FEAT-CHAR-RADAR-MODEL` · `RT-FEAT-CHAR-RADAR-RATINGS-V01` · `RT-FEAT-WIKI-CHART-GENERATOR`.
> **Fonte**: [`RefactorTactics_Character_Radar_Wiki_Generator_Claude.md`](../archive/src/RefactorTactics_Character_Radar_Wiki_Generator_Claude.md)
> (handoff archiviato, filtrato — il repository era più avanti del sorgente su §4).

## 1. L'invariante

Un radar è una **vista derivata**. Non è un ingresso della simulazione, non è un catalogo, e nessun
valore che compare su un raggio viene letto dal resolver.

```text
dati competitivi (catalogo, data asset, C++)   ← autorità
        |
        v
rating normalizzati 1..10                      ← vista, questo documento
        |
        v
SVG generato                                   ← artefatto, mai sorgente
```

La freccia non torna indietro. **Non si modifica un valore di gameplay perché un radar risulti più
bello**, e non si aggiunge un asse perché un sistema nuovo sembra meritarlo (§5).

Il motivo per cui la regola va scritta invece che sottintesa: HP 120, Move 5, Range 7 e Detection 40
sono numeri **non comparabili**, e un radar che li mette sugli stessi raggi produce una forma che
significa soltanto «quale unità di misura ha il numero più grande». I rating esistono per rendere
comparabile ciò che nel gameplay non lo è, e questo li rende **utili alla lettura e inservibili al
calcolo**.

## 2. I due radar

Sono due viste distinte, con pubblico e assi diversi. Non si mescolano e non si confrontano fra loro.

### 2.1 Profile Radar — pubblico, Wiki, design

Sei assi, in **quest'ordine**:

| # | Asse | Chiave | Copre |
|---|---|---|---|
| 1 | Offesa | `offense` | Pressione, danno, capacità di finire un bersaglio, affidabilità con e senza setup |
| 2 | Durabilità | `durability` | Resistenza, mitigazione, scudi, capacità di sostenere focus |
| 3 | Mobilità | `mobility` | Move, dash, reposition, libertà di percorso, engage/escape, cambio quota |
| 4 | Controllo | `control` | Push, pull, slow, zoning, denial, modifica cover e archi, blocco di rotte |
| 5 | Supporto | `support` | Protezione alleati, sustain, buff, peel, setup per combo |
| 6 | Informazione | `information` | Reveal, detection, stealth, rumore, tracking, negazione informativa |

### 2.2 Balance Radar — interno, tuning

Cinque assi, in **quest'ordine**: Precisione (`precision`), Potenza (`power`), Controllo (`control`),
Supporto (`support`), Durabilità (`durability`).

È la vista vicina alle matrici di bilanciamento, e le sue cinque colonne **esistono già** nel
repository (§4).

### 2.3 L'ordine fa parte della specifica

Cambiare l'ordine dei raggi cambia la forma del poligono a parità di valori. Due radar prodotti con
ordini diversi non sono confrontabili anche se i numeri coincidono — quindi l'ordine è normativo,
non estetico, e vale per single e compare.

## 3. La scala 1..10

| Valore | Lettura |
|---:|---|
| 1–2 | Molto debole |
| 3–4 | Sotto media |
| 5–6 | Medio |
| 7–8 | Forte |
| 9–10 | Eccellente, tratto identitario |

Interi. Un `10` deve corrispondere a qualcosa che il personaggio **è**, non a un arrotondamento
generoso; un roster in cui tutti hanno 9 e 10 non comunica niente, e un personaggio piatto su tutti i
raggi non ha identità leggibile.

⚠️ **La rubrica di conversione kit/stats → rating non esiste ancora.** Finché non esiste, i rating si
assegnano con una decisione tracciata, non «a vibe» e non a intuito: vedi
[`OPEN_DECISIONS.md`](../OPEN_DECISIONS.md).

## 4. Le due fonti che si contraddicono

Questo è lo stato verificato il **2026-08-11**, ed è il motivo per cui i rating v0.1 non sono
canonici oggi.

Il Balance Radar non è una dimensione nuova: le sue cinque colonne esistono in **due** workbook, con
contenuti incompatibili sugli stessi quattro eroi.

**A** — `docs/balance/RefactorTactics_Balance_Matrices_v0.1.xlsx`, foglio `03_Stats_Base`:

| Hero | Precisione | Potenza | Controllo | Supporto | Durabilità | Indice_Combat | Budget |
|---|---:|---:|---:|---:|---:|---:|---:|
| Flux | 6 | 4 | 4 | 2 | 2 | 45.6 | 60 |
| Riva | 6 | 4 | 4 | 2 | 2 | 45.6 | 60 |
| Bastion | 6 | 4 | 4 | 2 | 2 | 45.6 | 60 |
| Vektor | 6 | 4 | 4 | 2 | 2 | 45.6 | 60 |

**B** — `docs/characters/data/RefactorTactics_Characters_Wiki_Data_v0.4.xlsx`, foglio `02_Hero_Stats`:
le stesse cinque colonne sono **vuote**, con `Data_Status: CANONICAL_PARTIAL` e la nota
*«Canonico: HP 90, Move 5. Altri attributi di questa matrice non sono definiti nel catalogo v0.1»*.

Le quattro righe di **A** sono identiche fra loro in ogni colonna, `Indice_Combat` e `Budget_Punti`
compresi: sono valori di default mai differenziati, non profili. Nello stesso foglio i 38
personaggi *candidate* hanno invece valori distinti (Aurora `7/6/10/6/5`, Countess `8/8/5/2/2`) —
quindi un controllo a campione sul foglio **non** rivela il problema: a essere placeholder sono
esattamente le quattro righe che servono alla v0.1.

**Conseguenza operativa.** «Riusa i rating che esistono già» è la regola giusta e qui produce il
risultato sbagliato: quattro radar sovrapponibili, dichiarati canonici, che violano §3. La regola va
letta come **riusa se esistono e discriminano**, con un criterio meccanico verificabile:

> Una colonna in cui tutte le righe del roster target hanno lo stesso valore non è una fonte di
> rating: è un default. Va rifiutata dal validator, non copiata.

Il workbook **B** ha già preso una posizione — quei rating non sono canonici. Quale delle due fonti
sopravvive è una **decisione aperta**, non un dettaglio di implementazione: finché entrambe vivono,
un generatore che punta a quella sbagliata produce output canonico all'aspetto e falso nel merito.

## 5. Copertura degli assi, oggi

| Asse | Profile | Balance | Fonte |
|---|:--:|:--:|---|
| `control` · `support` · `durability` | ✅ | ✅ | colonne esistenti (contese, §4) |
| `precision` · `power` | — | ✅ | colonne esistenti (contese, §4) |
| `offense` · `mobility` · `information` | ❌ | — | **nessuna fonte** |

Il Profile Radar ha quindi **tre assi su sei senza alcun dato**, mentre il Balance ne ha cinque su
cinque già modellati. È una differenza di costo che l'ordine di lavoro deve rispettare: il Balance
Radar arriva prima perché è l'unico dei due che può essere dimostrato end-to-end senza inventare una
dimensione (§7).

Questo non cambia **quale** radar è pubblico: il Profile resta la vista rivolta a chi legge la Wiki.

## 6. Valori mancanti: TBD non è zero

Un asse senza rating è `TBD`, e **`TBD` non si disegna**.

Renderizzare un asse mancante come `0` produce un poligono con un rientro profondo, cioè la stessa
forma che avrebbe una debolezza deliberata. Il radar direbbe «questo personaggio è pessimo in
Informazione» quando il dato significa «non lo sappiamo ancora»: una forma che mente è peggio di
nessuna forma.

Regola: **se un asse del radar richiesto è `TBD`, la generazione di quel radar fallisce** con un
errore che nomina personaggio e asse, e non produce artefatti parziali. Non esiste un radar «quasi
completo».

⚠️ **Conseguenza da tenere presente**: con lo stato di §5, oggi nessun Profile Radar dei quattro eroi
v0.1 è generabile. È voluto — è il modo in cui la regola impedisce di pubblicare quattro poligoni
inventati — e si scioglie chiudendo i rating, non allentando la regola.

## 7. Ordine di lavoro

1. **Risolvere il conflitto di §4** — quale workbook è autorità, e ritiro dell'altro lato.
2. Rubrica di conversione, almeno per gli assi Balance.
3. Rating canonici v0.1 sui cinque assi Balance.
4. Generatore, dimostrato prima sul Balance Radar.
5. Assi `offense` / `mobility` / `information`, che richiedono modellazione nuova.
6. Profile Radar e integrazione Wiki.

## 8. Determinismo del generatore

A parità di input, versione e configurazione l'SVG deve essere **byte-identical**. Oltre alle insidie
consuete — timestamp, UUID, ordine di iterazione non stabile, metadata variabili — questo generatore
ne ha una propria: **le coordinate dei vertici vengono da `sin`/`cos`**, quindi da float, la cui
resa decimale può cambiare con piattaforma, versione di libreria matematica e locale.

Perciò le coordinate si **arrotondano esplicitamente** a un numero fisso di decimali prima della
serializzazione, e la formattazione numerica è indipendente dal locale.

Un test che genera due volte nello stesso processo e confronta i risultati **non dimostra questa
proprietà**: passerebbe anche senza arrotondamento. Il golden test va ancorato a un hash committato.

## 9. Che cosa il radar non è

- **Non è Affinity.** Il radar dice *qual è il profilo tattico*; l'Affinity dice *verso quale stile
  tende un oggetto o una variante*. Le Affinity potranno in futuro modificare il profilo di una
  build, ma non diventano assi.
- **Non è una Synergy.** Una sinergia è una relazione che il sistema riconosce formalmente ([D-029](../decisions/RT_PDR_00_Decision_Log.md)).
- **Non è un asse per sistema.** Overwatch e reazioni ricadono su Controllo, Informazione, Supporto o
  Offesa secondo il kit; rumore e percezione su Informazione; stealth su Informazione e Mobilità;
  cover manipulation su Controllo, Durabilità o Supporto secondo l'effetto. Restano **sei** raggi:
  un radar con dodici assi non si legge.

## 10. Accessibilità

Scala identica in ogni confronto, nessun auto-scaling per personaggio, valori leggibili anche come
testo, e distinzione fra poligoni che **non dipende solo dal colore** (tratto, pattern, legenda).
L'SVG dichiara `role="img"` e una descrizione testuale.
