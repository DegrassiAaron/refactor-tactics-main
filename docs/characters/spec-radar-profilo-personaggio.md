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

È la vista vicina alle matrici di bilanciamento. Le sue cinque colonne **esistono** nel repository —
`Precisione_1_10 … Durabilità_1_10` nel foglio `03_Stats_Base` di
`RefactorTactics_Balance_Matrices_v0.1.xlsx` — ma **non sono una fonte**: per i quattro eroi della v0.1
portano valori identici, e **§4.2** mostra come il foglio si smentisca da solo. Anche questi cinque assi
si derivano quindi dalla rubrica ([D-112](../decisions/RT_PDR_00_Decision_Log.md)), come i sei del
Profile.

⚠️ Il foglio è anche **indietro**: dà a Wraith `HP 100`, superato da
[D-069](../decisions/RT_PDR_00_Decision_Log.md) (oggi `90`).

> ⚠️ **«Esistono» va detto**, perché cercare quei rating con `grep` nei `.md` non li trova e porta a
> concludere che non esistano affatto — che è falso, e produrrebbe un argomento più debole di quello
> vero: esistono, e il foglio stesso dimostra che sono default.

### 2.2a Che cosa misurano `precision` e `power`

Il Profile ha **un solo** asse `offense`; il Balance lo scompone in due. Senza dire cosa distingue i
due pezzi, la formula non è scrivibile — e per un po' la differenza non è stata scritta da nessuna parte.

**Il vincolo che decide tutto: nel combattimento non c'è RNG.** Nessun tiro per colpire, nessuna
`accuracy`. Quindi «precisione» non può significare *probabilità di andare a segno*: dev'essere
strutturale.

**`power` — quanto danno l'eroe produce per turno**, tenendo conto di quanto spesso può farlo:

```
power_raw = Σ  danno_garantito(azione) × disponibilità(cooldown)      ancora: raw 100 = 10
```

Non è *burst*: un colpo enorme ogni tre turni non vale quanto uno ripetibile.

**`precision` — quanto di quel danno arriva senza chiedere niente**, né setup né posizionamento della
squadra:

```
precision = (incondizionalità + 2 · selettività) / 3
```

- **incondizionalità** — quota di danno **garantito** sul potenziale, pesata per disponibilità;
- **selettività** — quota della **disponibilità** delle azioni rilevanti che non rischia gli alleati.

⚠️ **Due componenti, due metriche, e non è una svista**: un'abilità senza danno non ha danno da pesare,
quindi la selettività si misura sulla disponibilità. È anche più fedele — il rischio di colpire un
alleato dipende da **quanto spesso** usi l'azione, non da quanto fa male.

⚠️ **Il peso `1:2` è misurato, non scelto**: sul roster l'incondizionalità varia da `0.77` a `1.00`
mentre la selettività copre l'intero `0.00–1.00`. Pesare di più la componente che varia meno
comprimerebbe tre eroi su quattro nello stesso intero.

**`precision` guarda solo le azioni offensive** — più le abilità senza danno il cui effetto si applica a
un'**area** e produce sugli alleati lo **stesso** svantaggio che produce sui nemici (`MistVeil`,
`FluidTrail`). Una copertura no: protegge chi vi sta dietro. Altrimenti un eroe risulterebbe «preciso»
per abilità che non colpiscono nessuno.

### 2.2b La commensurabilità è una posizione, non un fatto

Il Balance serve anche a vedere se un eroe **sta nel budget**, e questo impone che i cinque assi si
sommino. Ma `power` è una **quantità** (danno per turno) e `precision` una **qualità** (quanto è
affidabile): sommarle afferma **«l'affidabilità è potenza»**.

È difendibile — un colpo che va sempre a segno vale più di uno da preparare — ed è già ciò che il
workbook faceva sommando i cinque assi in `Indice_Combat`. Ma è una **scelta**, e va scritta qui:
altrimenti fra sei mesi qualcuno somma quei numeri senza sapere di averlo deciso.

⚠️ **Ne segue la regola che tiene insieme i due assi**: un bonus condizionale non può contribuire a
entrambi. Se `power` contasse il potenziale, il `+8 su Wet` di Gadget **alzerebbe** `power` e
**abbasserebbe** `precision`, e in un indice sommabile i due movimenti **si cancellerebbero** — il
tratto più identitario di Gadget sparirebbe dal costo. Perciò `power` conta solo il danno **garantito**
(§5.2), e il condizionale vive interamente in `precision`.

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

La rubrica che converte stats e kit in un rating è **codice**, non una tabella compilata a mano: vedi
§4.

## 4. I rating non si scrivono: si calcolano

[D-106](../decisions/RT_PDR_00_Decision_Log.md). Il generatore legge
[`RT_HeroCatalog_v0.1.md`](../balance/RT_HeroCatalog_v0.1.md), applica la rubrica e produce i rating
**in memoria**. Non esiste un file di rating, quindi non può nascere una seconda fonte né divergere.

```text
RT_HeroCatalog_v0.1.md      ← autorità (D-023)
        |                     + RT_ActionCatalog_v0.1.md per le abilità che rinviano
        |                       a un'azione core (D-115)
        v  rubrica (codice, rivedibile in PR)
   rating 1..10             ← esistono solo durante la generazione
        |
        v
      SVG
```

### 4.0 Due cataloghi, una sola autorità

[D-115](../decisions/RT_PDR_00_Decision_Log.md). Il catalogo eroi non è autosufficiente: la riga di
`Flux.ConductiveNode` dichiara *«**è `Action.Electrify`**»* e non porta un numero di danno, che vive in
[`RT_ActionCatalog_v0.1.md`](../balance/RT_ActionCatalog_v0.1.md). La rubrica legge quindi **entrambi**,
e il rinvio `` è `Action.X` `` è parte del contratto di lettura, non prosa libera.

Non contraddice D-106, che escludeva i **workbook**: i due cataloghi markdown sono già l'autorità dei
numeri per [D-023](../decisions/RT_PDR_00_Decision_Log.md), diffabili e revisionabili in PR. La regola
resta «un dato ha un solo posto dove vive» — la portata di `Action.Electrify` sta nel catalogo azioni
proprio perché **non** è di Gadget: è dell'azione core che sette abilità potrebbero riusare.

⚠️ **Vale un valore pubblicato, non è teoria**: senza la seconda fonte Gadget esce `power 5` e
`offense 4` invece di `6` e `5`, perché i `20` danni di `ConductiveNode` non vengono letti.

⚠️ **Un solo pattern risolve, e va distinto dall'altro che gli somiglia.** La tabella delle reazioni
del catalogo eroi cita anch'essa azioni core — `Flux.ReactiveCapacitor` → `Action.Counter`,
`Bastion.Interposition` → `Action.Intercept`, `Vektor.Deflection` → `Action.Deflect` — ma tiene i
**propri numeri inline** (`scudo 15 e 10 danni`, `−20`). Quelle riusano la **semantica**, non i valori:
il parser non deve risolverle contro il catalogo azioni. Solo `` è `Action.X` `` delega il dato, e nel
roster v0.1 compare **una volta sola**.

### 4.1 Perché nessun workbook è la fonte

La domanda «quale dei due workbook è autorità sui rating» era **mal posta**, e il repository aveva già
risposto prima che venisse fatta.

[D-023](../decisions/RT_PDR_00_Decision_Log.md) ha dichiarato
`RefactorTactics_Balance_Matrices_v0.1.xlsx` **`RESEARCH`** e spostato l'autorità dei numeri sui
cataloghi `balance/RT_*Catalog_v0.1.md`. [`docs/balance/README.md`](../balance/README.md) vieta anche
la riparazione che sembrava ovvia:

> **Non correggerlo cella per cella.** Un workbook rattoppato diventerebbe una falsa fonte corrente,
> che è peggio di uno dichiaratamente vecchio.

E `scripts/build-state-matrices-xlsx.py` lo aveva scritto ancora prima: *«il progetto ha tre `.xlsx` e
**nessuno script li legge**: sono dump di consultazione»* — non diffabili, non revisionabili in PR,
invisibili ai gate.

Il conflitto fra i due workbook quindi non si risolve: **si dissolve**. Nessuno dei due era una fonte.

### 4.2 Il difetto che questa scelta rende impossibile

Vale la pena conservare cosa sarebbe successo riusando il workbook, perché è il difetto che la
regola previene.

Il foglio `03_Stats_Base` assegna a Gadget, Phase, Riktor e Wraith **la stessa identica riga**
(`6/4/4/2/2`, `Indice_Combat 45.6`, `Budget 60`). Che siano default lo dimostra il modo in cui tratta
tutti gli altri: le restanti 38 righe si distribuiscono su **12 combinazioni assegnate per ruolo** —
otto Controller condividono `7/6/10/6/5`, sei Bruiser `7/8/6/3/7`. Il foglio discrimina per ruolo, e i
quattro della v0.1 hanno **quattro ruoli diversi** con valori identici: sotto la sua stessa regola
dovrebbero differire.

⚠️ Un controllo a campione **non** lo rivela: si incontrano righe popolate e plausibili, e sono
placeholder esattamente le quattro che servono alla v0.1. Con i rating calcolati il caso non si
presenta più — non c'è nessuna cella da riusare per sbaglio.

### 4.3 Il costo, dichiarato

Se i rating non sono scritti, senza rubrica **non esistono affatto**: non c'è il ripiego «intanto li
mettiamo a mano». La rubrica deve spiegare anche i casi che un umano avrebbe risolto a intuito, e un
eroe il cui kit sfugge alla formula non ha una scappatoia — si corregge la formula, che vale per tutti.

In cambio: cambiare `Salute` o `Movimento` in un catalogo **cambia i radar da solo**.

## 5. Che cosa alimenta ogni asse

[D-107](../decisions/RT_PDR_00_Decision_Log.md) conferma i sei assi e li fa **modellare** invece di
ridurre il radar a ciò che era già derivabile. Sotto §4 «modellare» significa scrivere una formula sui
dati dei cataloghi.

Gli input che il catalogo eroi dichiara **per eroe** sono: `Salute`, `Movimento`, `Range visivo`,
`Resistenza Push`, `Affinità`/`Debolezza`, la risorsa firma con ricarica e cap, e le abilità con
danno, range, effetto e cooldown.

| Asse | Si deriva da | Stato |
|---|---|:--:|
| `offense` | `power × (0.5 + precision/20)` — il Profile **aggrega** ciò che il Balance scompone (§5.2) | ✅ |
| `durability` | `Salute` + `Resistenza Push` | ✅ |
| `mobility` | `Movimento` + abilità di riposizionamento (dash, blink, self-reposition) | ✅ |
| `control` | effetti dichiarati: push, slow, zoning, `Interrupt`, modifica del terreno | ✅ |
| `support` | effetti dichiarati: scudo, heal, peel, buff ad alleati | ✅ |
| `information` | **solo** `Range visivo` | ⚠️ |

### 5.1 `information` nasce con un solo ingrediente

Va detto adesso, perché è la parte che promette più di quanto oggi mantenga.

L'unico input per eroe è la **Vista**: Gadget 7 ([D-073](../decisions/RT_PDR_00_Decision_Log.md)),
Wraith 6, Phase e Riktor 5. Stealth, detection e tracking **esistono solo** in `03_Hero_Vision` del
workbook character, che §4 esclude dalle fonti — quindi non entrano.

Ne segue che `information` è una funzione quasi monovariata, e sul roster v0.1 produrrà **tre valori
distinti su quattro eroi**.

⚠️ **La strada per arricchirlo non è ripescare il workbook.** Nel gioco il rumore è una proprietà
delle **azioni**, non degli eroi ([D-042](../decisions/RT_PDR_00_Decision_Log.md)): il contributo per
eroe si deriva dal **kit** — quali azioni possiede e quanto rumore fanno. Finché quel passo non
esiste, `information` va letto come «quanto lontano vede», e la Wiki non deve promettere di più.

### 5.2 Le regole comuni a ogni asse

Valgono per **entrambi** i radar. Stanno qui e non nelle formule dei singoli assi perché una risposta
diversa per asse produrrebbe due viste che raccontano lo stesso eroe in modo diverso.

**Il kit letto è quello base** — le quattro abilità fondamentali. Niente varianti, gadget o moduli di
reazione: il radar descrive il **personaggio**, non una build, ed è la base comune che rende due eroi
confrontabili (§1). Il loadout consigliato aprirebbe il catalogo equipaggiamento come terza fonte e
richiederebbe di assumere **quanti bersagli** colpisce *Scarica ramificata* — che è uno scenario, non un
dato. Le varianti restano documentate come **direzione**, senza numeri.

**Le reazioni rinviate a E14 contano.** `Vektor.InterceptShot` e `Riva.FlowReaction` non producono nulla
in partita, ma il radar descrive l'eroe come il **catalogo lo dichiara**. Escluderle legherebbe i rating
al calendario di implementazione: quando E14 atterra i numeri cambierebbero e il gate di §8 diventerebbe
rosso **senza** che nessuno abbia toccato un dato competitivo.

**Il cooldown pesa `disponibilità(CD) = 10 / (1 + CD/2)`** — `CD 0 → 10 · 1 → 6.67 · 2 → 5 · 3 → 4`.
Risolve `CD 0` senza casi speciali, con **una sola costante** da giustificare invece di una tabella di
quattro. Scartata la frequenza pura `10/(1+CD)`, che fa collidere Riktor e Phase.

> ⚠️ **La curva non è taratura: cambia il carattere di un eroe.** Gadget ha quattro abilità su cinque a
> `CD 2–3`, quindi una curva severa le svaluta tutte — e siccome le sue abilità **non selettive** sono
> proprio quelle a cooldown alto, la sua selettività *sale*. Con una tabella piatta usciva
> `power 7 / precision 6`; con questa curva esce `6 / 7`.

**Un bonus condizionale vale zero come danno.** Il `+8 su `Wet`` di `Flux.LinearDischarge` non entra in
nessun rating come danno: entra come **condizionalità**. La regola vale su ogni asse e su **entrambe** le
sedi in cui il catalogo dichiara una condizione — la cella `Effetto` per gli stati, la tabella delle
reazioni per le previsioni (`Vektor.InterceptShot`). Due meccanismi separati potrebbero divergere.

**Le ancore sono assolute, non relative al roster.** `rate(raw, ancora)` porta `raw = 0` a `1` e
`raw = ancora` a `10`, e satura. Una normalizzazione min-max garantirebbe lo spread per costruzione, ma
farebbe cambiare i rating degli eroi esistenti all'arrivo di un eroe nuovo (**E35**) — e il `--check` di
§8 diventerebbe rosso senza che nessun dato competitivo sia cambiato. Lo spread resta un requisito da
**verificare**, non da imporre.

**L'ancora di `power` è `raw 100`**, cioè «uccide un eroe medio in un turno»: la salute del roster è
`90/95/120/90`, media `98.75`, e `power_raw` è già danno per turno. Deriva da un dato del gioco, non da
un numero scelto perché i conti tornassero.

> **Implementazione**: `tools/radar/rubric.ts` (scala, ancore, peso del cooldown, `TBD`) e
> `tools/radar/power.ts`. L'aritmetica è **intera esatta**: `10/(1+CD/2)` è `20/(2+CD)`, i denominatori
> possibili sono `{2,3,4,5}` e il loro mcm è `60`, quindi ogni peso è un intero. Serve al `--check` di
> §8, che confronta artefatti byte a byte: un arrotondamento in virgola mobile lo renderebbe rosso su
> una macchina e verde su un'altra.

## 6. Valori mancanti: TBD non è zero

Un asse senza rating è `TBD`, e **`TBD` non si disegna**.

Renderizzare un asse mancante come `0` produce un poligono con un rientro profondo, cioè la stessa
forma che avrebbe una debolezza deliberata. Il radar direbbe «questo personaggio è pessimo in
Informazione» quando il dato significa «non lo sappiamo ancora»: una forma che mente è peggio di
nessuna forma.

Regola: **se un asse del radar richiesto è `TBD`, la generazione di quel radar fallisce** con un
errore che nomina personaggio e asse, e non produce artefatti parziali. Non esiste un radar «quasi
completo».

⚠️ Con i rating calcolati (§4) `TBD` cambia significato: non è più «nessuno ha compilato la cella» ma
**«la rubrica non sa produrre questo asse»**. Resta un fallimento di generazione, e resta la ragione:
un asse che la formula non copre disegnato come `0` accuserebbe l'eroe di una debolezza che nessuno ha
misurato.

✅ **Nell'implementazione `TBD` è un `Symbol`, non un numero** (`tools/radar/rubric.ts`): non può
diventare `0` per errore di tipo o per una somma distratta. La regola qui sopra smette di dipendere
dalla disciplina di chi scrive la formula.

## 7. Ordine di lavoro

1. **Rubrica** — è il prerequisito di tutto: senza formula i rating non esistono affatto (§4.3).
2. **Assi Profile** nell'ordine di §5: prima i cinque derivabili, poi `information` col suo limite.
3. **Generatore** sul Profile, dimostrato end-to-end con il gate di §8.
4. **Integrazione Wiki** e pubblicazione dei primi radar.
5. **Balance Radar** — `power` è definito e implementato; resta `precision`.
6. Rumore per eroe derivato dal kit, che è ciò che rende `information` un asse vero (§5.1).

> **Stato al 2026-08-12.** Fatti: il **parser** che riempie `HeroInput` dai due cataloghi
> (`tools/radar/parse-catalog.ts`), l'**infrastruttura** della rubrica — scala, ancore, peso del
> cooldown, `TBD` — e l'asse **`power`**. Restano `precision` e i sei assi del Profile, poi il
> generatore.
>
> ⚠️ Il punto 5 diceva che la differenza fra `precision` e `power` «non è scritta da nessuna parte»:
> non è più vero, `power` è *danno sostenuto* e `precision` *incondizionalità più selettività*.

Il primo punto non è più «risolvere il conflitto dei workbook»: [D-106](../decisions/RT_PDR_00_Decision_Log.md)
lo ha dissolto.

> ✅ **Il Balance non è più il passo 2** ([D-112](../decisions/RT_PDR_00_Decision_Log.md), 2026-08-12).
> [D-105](../decisions/RT_PDR_00_Decision_Log.md) lo metteva per primo con una motivazione esplicita —
> *«è l'unico dei due dimostrabile end-to-end senza modellare tre assi nuovi»* — che valeva **solo**
> finché le sue cinque colonne erano una fonte. D-106 le ha escluse e §4.2 mostra che per la v0.1 sono
> default: da allora il Balance richiede **due** assi modellati (`precision`, `power`) che il Profile non
> ha, mentre il Profile ne richiede tre già coperti dalla rubrica. L'ordine si inverte perché si è
> invertito il costo, non perché sia cambiato quale radar è pubblico.

## 8. Toolchain, determinismo e gate

[D-108](../decisions/RT_PDR_00_Decision_Log.md): il generatore è **Node/TypeScript**, e gli SVG
prodotti si **committano** con un `--check` che li confronta con la rigenerazione. Il gate è parte
dell'MVP, non lavoro futuro.

### 8.1 Byte-identical, e perché qui non è ovvio

A parità di input, versione e configurazione l'SVG deve essere **byte-identical**. Oltre alle insidie
consuete — timestamp, UUID, ordine di iterazione non stabile, metadata variabili — questo generatore
ne ha una propria: **le coordinate dei vertici vengono da `sin`/`cos`**, quindi da float, la cui resa
decimale può cambiare con piattaforma, versione di libreria matematica e locale.

Perciò le coordinate si **arrotondano esplicitamente** a un numero fisso di decimali prima della
serializzazione, e la formattazione numerica è indipendente dal locale.

Un test che genera due volte nello stesso processo **non dimostra** questa proprietà: passerebbe anche
senza arrotondamento. Il golden test va ancorato a un hash committato.

⚠️ **Il determinismo viene prima del gate, non dopo**: senza arrotondamento il `--check` è rosso su una
macchina e verde su un'altra, e un gate che dipende dalla macchina insegna a ignorarlo.

### 8.2 L'effetto combinato: il gate guarda i cataloghi

Le due scelte di D-108 producono insieme qualcosa che nessuna delle due ha da sola. Poiché i rating
vengono dai cataloghi (§4), il gate degli SVG diventa **un test di regressione sui dati competitivi**:

> Cambiare `Salute` in `RT_HeroCatalog_v0.1.md` fa diventare rosso il gate finché gli SVG non sono
> rigenerati **nello stesso commit**.

Un rebalance porta con sé i suoi grafici e non può più dimenticarli. È il beneficio del committare, ed
è anche il suo prezzo: chi tocca un catalogo deve poter eseguire il generatore, quindi la toolchain
Node diventa un prerequisito del **bilanciamento**, non solo della documentazione.

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

**Leggibili su entrambi i temi.** «Valori leggibili come testo» non è soddisfatto da un testo che
*esiste* nel documento: fino al 2026-08-12 le etichette erano `#333` e i valori `#111` su fondo
trasparente, e su tema scuro sparivano. L'SVG porta ora un blocco
`@media (prefers-color-scheme: dark)`, che vale anche quando l'immagine è caricata come `<img>`
perché la query legge la preferenza del **sistema**. Limite noto e accettato: segue l'OS, non
l'interruttore di tema del sito che ospita l'immagine.

**Il `<title>` dice quale vista è.** È il nome accessibile a cui punta `aria-labelledby`, e le due
viste sono altrimenti indistinguibili — due poligoni 400×400 sugli stessi colori. Fino al 2026-08-12
tutti e quattro i Balance Radar si annunciavano come `Profile Radar`, perché la vista non era un
parametro. Ora lo è, ed è obbligatorio (`RadarView` in `tools/radar/svg.ts`).

**Il grafico dichiara la propria dimensione.** Un `viewBox` senza `width`/`height` dà proporzione
ma non misura: dentro un `<img>` la taglia la decide il contenitore. Misurato sulla Wiki il
2026-08-12, prima della correzione: **896×896** su una scheda eroe — l'intera colonna — e **47×47**
dentro la tabella di `Profilo tattico`, dove le etichette da 12px scendono a circa 1,4px. Due celle
della stessa tabella divergevano (143 contro 117) perché il layout si dimensiona sul contenuto, cioè
sulla **lunghezza del testo alternativo**: la descrizione finiva per decidere la taglia del grafico.
La radice porta ora `width="400" height="400"`, e il contenitore può solo ridurre.

**Il testo alternativo ripete i valori, e un gate lo verifica.** Quando il radar è pubblicato come
immagine, il `<title>` interno non raggiunge il lettore: conta l'`alt` della pagina che lo incorpora.
Fino al 2026-08-12 sulla Wiki erano etichette senza numeri («Profile Radar di `Flux`»): chi vede il
grafico leggeva sei valori, chi usa uno screen reader ne riceveva zero. L'`alt` è ora
`<title>: <asse> <valore>, …` e si ricava **dall'SVG**, mai da una tabella parallela.

Ne segue la catena, che è la ragione del gate: **cataloghi → SVG → alt**. Il primo anello lo tiene
`generate.ts --check`, il secondo `wiki-alt.ts --check`. Senza il secondo, un rebalance aggiornava il
grafico e lasciava indietro la descrizione, in silenzio — la prosa della Wiki è scritta a mano
([D-076](../decisions/RT_PDR_00_Decision_Log.md)) e nessun deploy la tocca.
