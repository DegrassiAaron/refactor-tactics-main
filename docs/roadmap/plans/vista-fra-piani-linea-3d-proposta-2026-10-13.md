# La vista fra piani segue una linea vera — proposta del 2026-10-13

> `SNAPSHOT` · fotografia del 2026-10-13: vale finché è l'ultima misura del suo oggetto.
>
> - **Statuto**: proposta scritta, chiesta dall'autore prima di implementare. La direzione è sua. Le quattro scelte
>   di §5 le ha prese lo stesso giorno, dopo averla letta: sono segnate in §5 e registrate in
>   [D-511](../../decisions/RT_PDR_00_Decision_Log.md). Le raccomandazioni erano di chi ha scritto la proposta.
> - **Viene da**: la tavola `S1` della seduta `U88` su [#3741](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3741) e [D-510](../../decisions/RT_PDR_00_Decision_Log.md).
> - **Issue**: [#3743](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3743).
> - **Numeri**: quelli di §4.3 vengono da un prototipo fuori dal motore, e sono ordini di grandezza. Il metodo è in §9.
>   La misura fatta dal gioco, dopo l'implementazione, è in coda a §4.3.

---

## 1. Da dove viene

[D-510](../../decisions/RT_PDR_00_Decision_Log.md) ha chiuso un caso: due unità sulla stessa colonna, una sopra l'altra, non si vedono. Davanti alla
tavola `S1`, dove Aevik sta a terra sotto una piattaforma e due nemici stanno sulla piattaforma, l'autore ha scritto:

> «mi sembra che aevik non potrebbe vedere nessuno dei due nemici sopra, o c-e- una linea di visuale che non ho visto?»

Una linea di visuale non c'è: fra Aevik e il nemico accanto c'è il pavimento della piattaforma. Il gioco li fa vedere
perché calcola la vista **in pianta** e i pavimenti non li considera. Alla domanda su quale regola volesse, fra «Al
coperto non si vede sopra», «Linea vera in 3D» e «Com'è ora», l'autore ha scelto la seconda. *Il testo dell'opzione,
come l'ha vista*:

> «Linea vera in 3D. Il gioco traccia una linea dagli occhi di chi guarda al corpo del bersaglio, e la ferma su ogni
> pavimento che attraversa. È la più fedele, ma è un lavoro più grande e con scelte da fare (altezza degli occhi, casi
> sul bordo): prima ti porto una proposta scritta, non la implemento alla cieca.»

Questa è quella proposta.

## 2. Com'è oggi

Misurato su `main` = `4e6d198cd`.

- **Che cosa vede un'unità** lo decide `URTPerceptionLibrary::VisibleCells`. I candidati sono le celle di ogni piano.
  Distanza e arco frontale si valutano sulla proiezione esagonale. Il contatto (`ContactHolds`) chiede portata, fumo e
  linea di vista.
- **La linea di vista** (`URTHexVisionLibrary::DescribeLineOfSight`) è tracciata sul piano di chi guarda: contano i
  soli ostacoli di quel piano. È la regola D3 di [`h6-4-hex-vision-spec.md`](../../technical/systems/h6-4-hex-vision-spec.md). Un pavimento non blocca, e un muro che sta su un
  altro piano non conta.
- **L'unica cecità verticale** è quella di D-510: stessa colonna, piano diverso, nessun contatto.
- **Le misure del mondo**: i centri di due celle vicine distano 259,8 unità (`HexSize` 150); un piano è alto 250
  (`LayerHeight`); un'unità è alta 180 (`UnitHalfHeight` 90). Un piano di dislivello e una cella di distanza sono
  quasi uguali: chi sta una cella più in là e un piano più su si guarda a circa 45 gradi.

## 3. Il modello proposto

### 3.1 La regola, in quattro frasi

1. Ogni cella che esiste è un **pavimento**, alla quota del suo piano. Dove una cella non c'è, c'è un buco.
2. La linea parte dalla **testa** di chi guarda e arriva alla testa di chi è guardato: il centro della cella, alzato
   di un'altezza fissa sopra il suo pavimento.
3. La linea è **ferma** se attraversa la quota di un piano in un punto dove quel piano ha una cella: ha bucato un
   pavimento.
4. Lungo la linea contano gli **ostacoli del piano in cui la linea sta passando** in quel tratto, non quelli del
   piano di chi guarda.

Fra due celle dello **stesso piano** la linea non cambia quota, quindi non attraversa nessun pavimento e incontra i
soli ostacoli di quel piano: è la regola di oggi, identica.

### 3.2 Di lato

```
   quota
   piano 1 ═══╤═══════════╤═══     pavimento della piattaforma
              │     B     │        B sta sulla piattaforma
              │    ╱      │
        ······│···╳·······│····    la linea arriva alla quota del piano 1 QUI:
              │  ╱        │        se in questo punto c'è una cella del piano 1, è ferma
   piano 0 ───┴─A─────────┴───     A sta a terra
```

Il punto in cui la linea raggiunge la quota del piano di sopra dipende solo dall'altezza della testa. Con la testa a
sei decimi di piano, quel punto sta a **quattro decimi** della distanza, contando da chi sta sotto.

### 3.3 Dove sta nel codice

Nel **contatto** della percezione, come D-510, e solo per le coppie di piani diversi. `HasLineOfSight` non cambia: è
la geometria su cui i bot scelgono le celle, e #3741 ha misurato che toccarla fa cadere due sonde anti-stallo
(`Bot.StalemateProbeGeneratedArenaFilterNeverStepsBack`, `Bot.StalemateProbeGeneratedArenaClosesNoCycleOfAnyPeriod`).
Colpire un altro piano resta un rifiuto dichiarato ([D-393](../../decisions/RT_PDR_00_Decision_Log.md), [D-509](../../decisions/RT_PDR_00_Decision_Log.md)): fra piani la linea non autorizza nessun colpo.

### 3.4 Determinismo

Solo interi. Con la testa a una frazione fissa del piano, il punto in cui la linea raggiunge una quota è una frazione
con numeri piccoli, che non dipende dalle misure in unità di mondo: fra un piano e quello sopra, a tre quinti di
testa, è il punto a due quinti della distanza. La cella che contiene quel punto si trova con lo stesso
arrotondamento intero di `URTHexLibrary::HexLine`, compreso il suo spareggio fisso sui confini. La regola è una
funzione pura di mappa e celle: nessun Actor, nessun mondo, nessuna virgola mobile.

## 4. Che cosa cambia a schermo

### 4.1 I casi della tavola `S1`

Con la testa a sei decimi di piano, sulla piattaforma di `TestArena`:

| Chi guarda | Chi è guardato | Oggi | Con la linea vera |
|---|---|---|---|
| Aevik, a terra sotto la piattaforma | Branth, sopra la sua testa | non lo vede (D-510) | non lo vede |
| Aevik, a terra sotto la piattaforma | Ivrin, sulla piattaforma una cella più in là | lo vede | **non lo vede**: la linea buca il pavimento sopra Aevik |
| un'unità a terra accanto alla piattaforma, allo scoperto | chi sta sulla cella di bordo | lo vede | lo vede |
| la stessa unità a terra, allo scoperto | chi sta una cella più indietro dal bordo | lo vede | **non lo vede**: il bordo lo copre |
| un'unità a terra dall'altro lato, a una cella dal bordo opposto | Ivrin, che da quel lato è sul bordo | lo vede | lo vede |

### 4.2 La regola pratica che ne esce

Lungo una riga dritta di celle, con la testa a sei decimi di piano e un piano di dislivello:

| Chi è guardato sta… | …e si vede da sotto solo da almeno |
|---|---|
| sulla cella di bordo della piattaforma | 1 cella |
| una cella più indietro | 3 celle |
| due celle più indietro | 5 celle |

Vale lo stesso al contrario. Chi sta sul bordo vede la cella a terra lì sotto; chi sta una cella più indietro ha una
**zona cieca** sotto il bordo, e vede a terra solo da tre celle in là. E chi sta a terra sotto una piattaforma non
vede niente di ciò che ha sopra.

### 4.3 Quanto cambia, sulle mappe che esistono

Coppie di celle su piani diversi entro sette celle, contate nei due versi. Senza l'arco frontale.

| Mappa | Coppie | Si vedono oggi | Testa a 0,5 | Testa a 0,6 | Testa a 0,72 |
|---|---|---|---|---|---|
| `TestArena`: un piano di raggio 4, una piattaforma di quattro celle | 488 | 375 | 218 | 298 | 364 |
| `LayerStack`: tre piani annidati | 3438 | 3372 | 736 | 941 | 1424 |

Su `LayerStack` oggi si vede quasi tutto; con la linea vera resta fra un quinto e due quinti. È la mappa dove ogni
piano alto ha un piano sotto, quindi è il caso in cui i pavimenti coprono di più.

➕ **Misurato dal gioco il 2026-10-13, con la regola implementata e la testa a 0,6.** Lo stampa il test
`RefactorTactics.Vision.AcrossLayers.SameAnswerBothWays`, sulle stesse coppie:

| Mappa | Coppie | Libere per la linea di tiro, che non cambia | Libere per la linea fra piani |
|---|---|---|---|
| `TestArena` | 488 | 386 | 294 |
| `LayerStack` | 3438 | 3438 | 944 |
| `LayerYard`: le impronte di `LayerStack`, con blocchi, coperture e porte | 3438 | 3005 | 672 |

Nessuna coppia a senso unico. Il prototipo aveva detto 298 e 941: la differenza sono i casi sul confine fra due
celle, come §9 avvertiva. Un secondo oracolo, scritto dalla regola in soli interi e non dal codice, dà 294 e 944.

## 5. Le quattro scelte, e come l'autore le ha prese

> ➕ **Prese il 2026-10-13**: in tutte e quattro l'opzione (a). Sotto ogni scelta c'è il testo dell'opzione come
> l'autore l'ha vista quando ha risposto; il resto della sezione è la proposta com'era.

### S1 — Da dove a dove va la linea

- **(a) Da testa a testa, alla stessa altezza.** La linea da A a B e quella da B ad A sono la stessa: se io ti vedo,
  tu mi vedi. *Raccomandata.*
- **(b) Dagli occhi di chi guarda al corpo di chi è guardato.** È la descrizione dell'opzione che l'autore ha scelto.
  Le due linee sono diverse, e nei casi di bordo uno vede l'altro senza esserne visto. Nel prototipo: 34 coppie a
  senso unico su `LayerStack`.

Colpire un altro piano è vietato, quindi a senso unico vorrebbe dire solo «io so dove sei e tu no». Resta comunque una
cosa difficile da leggere a schermo.

➕ **Scelta dell'autore: (a)**, «Da testa a testa». *«Stessa altezza per chi guarda e per chi è guardato. La linea da
A a B e quella da B ad A sono la stessa: se io ti vedo, tu mi vedi.»*

### S2 — Quanto è alta la testa

| Altezza | In unità | Che cosa vuol dire |
|---|---|---|
| 0,5 del piano | 125 | la più severa. Il punto critico cade spesso sul confine fra due celle: decide lo spareggio |
| **0,6 del piano** | 150 | poco sotto la cima di un'unità, alta 180. *Raccomandata* |
| 0,72 del piano | 180 | la cima dell'unità. La più permissiva: su `LayerStack` si vede una volta e mezza in più che a 0,6 |

Non è una leva di bilanciamento: è una costante della regola, fissata da un test.

➕ **Scelta dell'autore: 0,6 del piano.** *«150 unità, poco sotto la cima di un'unità. Da sotto: chi sta sul bordo
della piattaforma si vede da 1 cella, chi sta una cella più indietro da 3, due più indietro da 5.»*

### S3 — Quali muri fermano la linea fra piani

- **(a) I muri del piano in cui la linea sta passando.** Un muro a terra ferma il tratto basso della linea, un muro
  sulla piattaforma il tratto alto. È ciò che la regola 4 di §3.1 dice. *Raccomandata.*
- **(b) I soli muri del piano di chi guarda**, come oggi. Più semplice, ma a senso unico per costruzione: su
  `TestArena` 105 coppie in cui uno vede l'altro e non viceversa, perché il muro di `q=0` sta a terra e vale solo per
  chi guarda da terra.

➕ **Scelta dell'autore: (a)**, «Del piano attraversato». *«Un muro a terra ferma il tratto basso della linea, un
muro sulla piattaforma il tratto alto. Vale uguale nei due versi.»*

### S4 — Vale anche per le celle, o solo per le unità

- **(a) Anche per le celle.** Un'unità è rilevata quando la sua cella è vista: è una verità sola, ed è come funziona
  oggi. ⚠️ La conseguenza si vede: chi sta sotto un mezzanino non ne vede le celle, e il velo non le disegna finché
  qualcuno della squadra non le vede da fuori. Una volta viste restano ricordate. *Raccomandata.*
- **(b) Solo per le unità.** La mappa dei piani sopra e sotto si continua a vedere in pianta, come oggi; la linea vera
  decide solo chi è rilevato. Costa una seconda verità: una cella «vista» su cui un nemico non si vede.

➕ **Scelta dell'autore: (a)**, «Anche le celle». *«Una verità sola: un'unità è rilevata quando la sua cella è vista.
Conseguenza visibile: chi sta sotto un mezzanino non ne vede le celle, e il velo non le disegna finché un compagno
non le vede da fuori.»*

## 6. Che cosa non cambia

- La vista **dentro un piano**: portata, arco frontale, consapevolezza ravvicinata, fumo, muri.
- `HasLineOfSight`, e con lei la geometria dei bot, le reazioni, i criteri d'arena, l'ispettore e la console.
- [D-393](../../decisions/RT_PDR_00_Decision_Log.md) e [D-509](../../decisions/RT_PDR_00_Decision_Log.md): colpire un altro piano resta un rifiuto dichiarato.
- [D-018](../../decisions/RT_PDR_00_Decision_Log.md): la quota non dà bonus numerici alla vista. Qui non se ne aggiungono: si toglie ciò che un pavimento copre.
- D-510 resta vera, come caso particolare: nella stessa colonna la linea è verticale e buca il pavimento su cui sta
  chi è sopra.

## 7. Che cosa tocca

- **Stato canonico.** La conoscenza di squadra decide che cosa i bot pianificano. Su una mappa a più piani l'esito di
  una partita non presidiata può cambiare. Si misura sulla suite intera; il sorgente non ha una versione delle regole
  da alzare.
- **Il velo.** Con S4 (a), le celle dei piani che un pavimento copre non risultano osservate.
- **I bot.** Pianificano sulla conoscenza di squadra, che cambia. La loro stima di esposizione usa `HasLineOfSight`,
  che non cambia: fra piani continuano a credersi in vista quando non lo sono. È una stima prudente.
- **Le scene.** `Visual.Map.StackedColumn` va ridisegnata: oggi chi guarda sta sotto la piattaforma, e con la linea
  vera non vede nessuno dei due nemici. Serve chi guarda da fuori, un nemico sul bordo e uno più indietro.
  ➕ Fatto in due scene: `Visual.Map.StackedColumn` resta la vista da sotto, con un nemico a terra che fa da
  controllo; `Visual.Map.PlatformEdge`, nuova, è la vista da fuori.
- **Le voci PIE.** `PIE-VIS-COLONNA` diventa la voce di questa regola. Le scene a più piani già giudicate hanno i
  nemici a terra, sullo stesso piano di almeno un'unità di chi guarda: non dovrebbero cambiare, e si verifica.
- **Privacy.** La regola toglie informazione a una squadra, non ne aggiunge.

## 8. Come si verifica

- Test headless sulla funzione pura: i casi di §4.1 e di §4.2 uno per uno, nei due versi; lo stesso piano identico a
  oggi; la stessa colonna; un buco nel pavimento da cui la linea passa.
- Test sulla percezione: gli stessi casi attraverso `VisibleCells`, con la domanda di squadra.
- Con S1 (a): un test che per ogni coppia di celle di una mappa a più piani chiede la stessa risposta nei due versi.
- Verifica di mutazione sui test nuovi.
- Suite intera: le sonde dei bot devono restare verdi, e ogni test che cambia esito ha la ragione scritta.
- PIE: le scene ridisegnate, giudicate dall'autore sulle tavole.

## 9. Come sono fatti i numeri di §4.3

Un prototipo in Python, locale alla sessione che ha scritto la proposta e non nel repository. Rifà la geometria in
virgola mobile con le misure di §2: per ogni coppia di celle su piani diversi entro sette celle traccia la linea da
testa a testa, trova la cella in pianta nel punto in cui la linea raggiunge la quota di ogni piano attraversato, e
la dichiara ferma se quella cella esiste. Per i muri usa la variante (a) di S3.

⚠️ Non è il codice del gioco. Il gioco usa interi e uno spareggio fisso sui confini; il prototipo no, e i casi che
cadono esattamente sul confine fra due celle possono differire. I numeri dicono quanto cambia, non fissano un'attesa:
quella la fisseranno i test.
