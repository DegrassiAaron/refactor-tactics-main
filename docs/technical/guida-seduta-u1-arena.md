# Guida operativa — U1 · costruire `L_HexArena`

> `CURRENT` · **Per**: [#451](https://github.com/DegrassiAaron/refactor-tactics-main/issues/451)
> **Owner dei dati della seduta**: [`editor-sessions.yaml`](../roadmap/editor-sessions.yaml), `id: U1` — se i due
> divergono, vince quello. Qui c'è la **procedura**, lì il DoD.
> **Owner degli esiti**: [`test-manuali-pie.md`](test-manuali-pie.md) — le sette voci si scrivono lì.

Una sola apertura dell'editor. Sette passi per costruire, sette voci da verificare, un comando per sapere se
l'arena rispetta i tre criteri prima di committarla.

---

## 0. Prima di aprire l'editor

L'allowlist è già a posto ([#449](https://github.com/DegrassiAaron/refactor-tactics-main/issues/449)): i due
artefatti sono versionabili anche se non esistono ancora. Puoi verificarlo in un secondo:

```bash
git check-ignore -q Content/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.uasset; echo $?   # deve stampare 1
```

**`1` significa «non ignorato», cioè versionabile.** Se stampasse `0` fermati: `git add` non funzionerebbe e
non te lo direbbe. Non usare `-v` per questa verifica — con `-v` il comando esce `0` in entrambi i casi.

> ⚠️ Controlla che **`Scenario To Run` sia vuoto** (`BP_GameMode` → Class Defaults → *RefactorTactics|Test*) e
> che `rt.Test.Scenario` in console sia vuota. Con uno scenario impostato la partita normale non viene
> allestita affatto.

---

## 1. Il livello e l'asset

Nuovo livello in `/Game/RT/Maps/Dev/L_HexArena/`, con dentro un **`ARTHexMapActor`**.
L'asset mappa in `/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena`, assegnato al campo `MapAsset` dell'actor.

Stessa forma di `L_DevSandbox` ([`convenzioni-contenuti-ue.md` §5](convenzioni-contenuti-ue.md)): livello e
cartella `Data/` accanto.

**Perché non estendere `DA_HexMap_Sandbox`**: il sandbox resta il banco per prove distruttive, l'arena è la
mappa **stabile** su cui girano le verifiche. Se la stessa mappa fa entrambe le cose, un esperimento invalida
una verifica e non te ne accorgi.

---

## 2. La forma — esagono di raggio 4

Editor Mode **Hex Map** → tool **Paint** → `BrushRadius = 4` → un click sull'origine.

Esagono pieno di raggio 4 sul layer 0: 61 celle.

**Accendi `bShowOverlay`**: sta nel pannello **del tool** (Paint o Select), sezione `Hex | Overlay` — non
sull'actor. Con l'overlay spento vedi solo mesh grigie e non distingui nulla.

Acceso, ogni cella mostra fino a tre anelli concentrici:

| Anello | Colore | Significato |
|---|---|---|
| esterno | colore della superficie | `Floor`, `Mud`, `Rough`, … |
| medio | **giallo** | blocca la **vista** — ci si passa, non ci si vede attraverso |
| interno | **rosso** | blocca il **movimento** |

Sono marcatori distinti perché sono regole distinte, e una cella può averle entrambe. Finché dipingi solo
`Floor` resteranno comunque tutte dello stesso colore: la varietà arriva coi terreni.

---

## 3. La copertura — *criterio 1*

2–3 celle con `bBlocksMovement` e 2–3 celle con `bBlocksLineOfSight`, entrambe dal pannello del **pennello**.

> `bBlocksLineOfSight` è arrivato nella palette il 2026-08-10
> ([#474](https://github.com/DegrassiAaron/refactor-tactics-main/issues/474)): prima **nessuno strumento
> dell'Editor Mode sapeva scriverlo** e l'unica strada era editare l'array `Cells` a mano nel Data Asset. Il
> pennello lo tratta come `bBlocksMovement` — lo scrive sempre, quindi ridipingere con il flag spento
> **toglie** il muro.

**Il criterio non conta le celle, chiede che la vista sia interrotta**: servono **≥2 celle**
`bBlocksLineOfSight` sul segmento fra i due spawn, e la linea di tiro fra i due deve risultare bloccata.

Due dettagli che fanno fallire il criterio pur avendo messo i muri:

- **gli estremi non bloccano mai** — muri *sugli* spawn non coprono nulla;
- **la copertura non è un ostacolo** — `bBlocksLineOfSight` senza `bBlocksMovement` si attraversa. È quello
  che vuoi per una rotta coperta ma percorribile.

Gli spawn non li scegli tu: li deriva `PickStartCells` dalle celle percorribili in ordine stabile, prendendo
le due estremità. Con un esagono regolare cadono agli angoli opposti sull'asse `q`.

---

## 4. Il terreno costoso — *criterio 2*

Una zona a costo alto (Mud o Water) col tool **Fill**.

**Il criterio chiede due cose insieme**: che il percorso ottimale *attraversi* una cella a costo > 1, e che
costi più del budget di un turno (**5**).

L'errore da evitare è una zona costosa **aggirabile**: se il pathfinding la evita senza rinunciare a nulla,
non è una scelta ed è come se non ci fosse. Deve stare su una strettoia, non in mezzo al campo aperto.

---

## 5. La piattaforma e la transizione

Piattaforma di 3–4 celle sul **layer 1** (`ActiveLayer = 1`), collegata al layer 0 da **una sola**
transizione, creata col tool **Arch**.

Una sola: è ciò che rende la salita una decisione invece di una scorciatoia.

---

## 6. Rileggi a colori

`bShowOverlay` attivo, tool **Select**: ricontrolla costi e blocchi prima di salvare.

---

## 7. Le due rotte — *criterio 3*, il più facile da sbagliare

Due rotte fra gli spawn con trade-off diverso: **una più corta ed esposta, una più lunga e coperta**.

Il criterio, per esteso:

| Condizione | Misura |
|---|---|
| **disgiunte** | non condividono celle oltre agli estremi |
| **costo confrontabile** | rapporto ≤ **1,5** fra la cara e l'economica |
| **trade-off reale** | la rotta più cara è **meno esposta** di almeno **15 punti** percentuali |

Le tre insieme, e la **direzione conta**: se paghi di più e resti esposto uguale, non è una scelta — è una
rotta peggiore, che nessuno prenderebbe.

L'esposizione si misura in **frazione** di celle viste dallo spawn avversario, non in numero assoluto. Il
motivo è un difetto vero trovato dai test: contando le celle, la rotta più lunga ne ha di più e quindi
sembra sempre «diversa» — si misurerebbe la lunghezza invece della copertura.

> **`1,5` e `15%` sono proposti, non misurati.** Se costruendo trovi che descrivono male il gioco, cambiali:
> vivono in `editor-sessions.yaml` (U1 passo 7) e come default in `URTArenaCriteriaLibrary`. U19 li cita, non
> li ridefinisce.

---

## 8. Verifica i tre criteri **prima** di committare

Non serve stimarli a occhio. Con l'arena nel livello, in console (`ò` o `~`):

```
rt.Arena.Check
```

Misura l'asset e stampa i numeri:

```
[RT] Criteri dell'arena su 65 celle (budget 5, rapporto max 1.50, scarto minimo 15%):
[RT] spawn (-4,0,0) <-> (4,0,0)
[RT]   [ok] copertura : 2 celle che bloccano la vista sul segmento (ne servono 2), linea di tiro interrotta
[RT]   [ok] costo     : percorso ottimale costo 11 su budget 5 (oltre), cella piu' cara attraversata 3
[RT]   [no] rotte     : rotte disgiunte costo 10 e 10 (...), esposizione 57% e 56% (scarto minimo 15%: ...)
```

Il comando è **in sola lettura**: non tocca l'asset. Accetta tre argomenti opzionali per provare soglie
diverse senza ricompilare — `rt.Arena.Check <MoveBudget> <MaxCostRatio> <MinExposureGap>`, per esempio
`rt.Arena.Check 5 1.5 0.10` per vedere se l'arena passerebbe con uno scarto minimo del 10%.

Funziona anche sull'arena di ripiego in PIE, se vuoi vedere subito che aspetto ha un verdetto.

Un `[no]` dice **di quanto** hai mancato, non solo che hai mancato: correggere smette di essere tentativo ed
errore.

**Termine di paragone utile** — l'arena generata `MakeTestArena`, quella che le sedute U2…U6 usano, oggi
soddisfa **1 criterio su 3**: passa il costo, fallisce la copertura (1 cella invece di 2) e le rotte (57% e
56% di esposizione, cioè nessun trade-off). La differenza fra quel verdetto e il tuo è esattamente ciò che
questa seduta aggiunge al progetto.

---

## 9. Commit

```bash
git add Content/RT/Maps/Dev/L_HexArena/L_HexArena.umap \
        Content/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.uasset
git ls-files Content/RT/Maps/Dev/L_HexArena/     # l'oracolo: devono comparire entrambi
```

`git ls-files`, non `ls`: il file può esistere sul disco e non essere tracciato, ed è l'unico caso che fa
danno.

---

## 10. Le sette voci

Si verificano in una sola apertura dell'editor. Gli esiti si scrivono in
[`test-manuali-pie.md`](test-manuali-pie.md).

⚠️ **L'arena non è «appena costruita», e §1–§9 non vanno rifatti.** `L_HexArena` e `DA_HexMap_Arena`
esistono in `main` dal **2026-08-11** (`2e2caff6`): chi segue questa guida dall'inizio rifarebbe un
lavoro già committato — e lo riscriverebbe senza lease. Le sezioni da §1 a §9 restano come **storia di
come l'arena è nata**, utili per rifarla da zero su un'altra mappa; per la seduta si apre il livello che
c'è e si parte da qui.

| Voce | Cosa guardi | Tool |
|---|---|---|
| `PIE-HEX-MODE-N` | il secchiello riempie la regione; un Ctrl+Z la ripristina intera | Fill |
| `PIE-HEX-MODE-O` | cambiando `Surface` a `Rough`, `MoveCost` diventa `2` da solo | Paint |
| `PIE-HEX-MODE-F` | le transizioni esistenti appaiono come linee colorate con freccia From→To | Arch |
| `PIE-HEX-MODE-E` | click From → gizmo → drag su To → Commit crea la transizione; Undo la rimuove | Arch |
| `PIE-HEX-MODE-G` | re-click su un'altra cella → **un solo** gizmo; cambio tool → sparisce | Arch |
| `PIE-HEX-MODE-H` | il gizmo si aggancia al centro cella; alzando di `LayerHeight` passa al layer sopra | Arch |
| `PIE-HEX-MODE-L` | con `Operation=Remove`, click su un arco lo rimuove; click nel vuoto non fa nulla | Arch |

**Un ❌ chiude comunque la seduta**: il prodotto è il verdetto, non il successo. Una voce che fallisce diventa
un difetto da aprire, non un motivo per riaprire l'editor.

`PIE-HEX-LAYER` e `PIE-HEX-TRANS` **non sono più qui**: sono passate a U18, perché si verificano su un asset
generato con `GenerateIntoAsset` e non hanno bisogno di quest'arena.

---

## 10b. Le sette voci, in una sola passata

L'ordine sotto è per **tool**, non per importanza: cambiare tool è ciò che costa, e le cinque voci dell'Arch
si verificano quasi tutte con gli stessi gesti.

**⚠️ Prima di cominciare: NON salvare, e non salvare nemmeno alla fine.**

`Content/RT/Maps/Dev/L_HexArena/` è un binario, e la sua **Binary Asset Lease** non è emessa
(**D-139**, `docs/roadmap/parallel-batch.yaml`, track `content_editor`). Salvare quel package è la cosa
da non fare — non «una precauzione in più», ma il gesto che questa seduta ha il vincolo di evitare.

🔴 **Questa riga diceva l'opposto — «prima di cominciare, salva» — e il consiglio era già ridondante
quando è stato scritto**, non solo oggi. Le date lo mostrano:

```
2e2caff6  2026-08-11 07:57  feat(U1): L_HexArena e DA_HexMap_Arena, l'arena della v0.1
8aa1a4e0  2026-08-11 11:13  docs(U1): le sette voci in una sola passata, ordinate per tool
```

§10b è stato scritto **tre ore dopo** che l'arena era già in git. Il punto di ripristino contro l'Undo
di troppo esisteva già, ed era migliore di un salvataggio: **`git checkout`**.

∴ se un `Ctrl+Z` va troppo indietro, **chiudi l'editor senza salvare**: l'asset su disco è intatto, e
non serve altro. Le sette voci non hanno bisogno di scrivere il package — verificano il **mode**, non il
terreno — e la sequenza `-E` (crea transizione) → `-L` (rimuovi arco) si richiude da sola.

### Tool Paint — `PIE-HEX-MODE-O`

Nel pannello del pennello cambia `Surface` a **`Rough`**.

- ✅ `MoveCost` si aggiorna **da solo** a `2` (il valore di catalogo), e `bBlocksMovement` resta `false`
- ❌ se `MoveCost` resta `1` o va cambiato a mano

Non serve dipingere: la voce verifica il pannello, non la cella.

### Tool Fill — `PIE-HEX-MODE-N`

Con il pennello su una superficie diversa da quella della regione, click su una zona di celle contigue dello
stesso terreno.

- ✅ la regione si riempie tutta; **un solo** `Ctrl+Z` la ripristina **intera** (non cella per cella)
- ✅ click su una cella vuota non fa nulla
- ✅ tornando a Select/Paint con l'overlay acceso si vedono i colori nuovi
- ❌ se l'undo ripristina una cella per volta, o se riempie oltre il confine della superficie

### Tool Arch — le cinque restanti, in sequenza

Serve la piattaforma sul layer 1 già fatta, e almeno una transizione esistente.

1. **`-F`** — guarda le transizioni già presenti: ✅ appaiono come **linee colorate** (colore per `Kind`) con
   una **freccia From→To**. ❌ se sono invisibili o senza verso.

2. **`-G`** — click su una cella: compare il gizmo. Poi **click su un'altra cella**: ✅ resta **un solo**
   gizmo, non due. Poi passa al tool **Select**: ✅ il gizmo **sparisce**. ❌ un secondo gizmo, o uno che
   resta in scena dopo il cambio tool.

3. **`-H`** — torna su Arch, click su una cella, poi **trascina** il gizmo: ✅ `To` (nel pannello) si aggancia
   sempre al **centro di una cella**; alzandolo di circa `LayerHeight` il target passa al **layer superiore**.
   ✅ nessun tremolio o rimbalzo durante lo snap. ❌ valori che oscillano, o un target che non sale mai di layer.

4. **`-E`** — dalla stessa posizione premi **`Commit`**: ✅ la transizione compare. Poi `Ctrl+Z`: ✅ viene
   rimossa; `Ctrl+Y`: ✅ torna. Ripeti un paio di volte. Prova anche **`ClearArch`** con un arco pendente:
   ✅ annulla senza scrivere nulla. ❌ un undo che lascia l'arco a metà.

5. **`-L`** — metti `Operation = Remove` e clicca **su un arco disegnato**: ✅ viene rimosso, e `Ctrl+Z` lo
   ripristina. Click **nel vuoto**, lontano da ogni arco: ✅ non succede nulla. Rimetti `Operation = Add`:
   ✅ il flusso del gizmo funziona come prima. ❌ un click nel vuoto che rimuove l'arco più vicino comunque.

### Dopo

`rt.Arena.Check` un'ultima volta: **tre `[ok]`**. Le voci sopra modificano la mappa (il Fill dipinge, gli
archi si aggiungono e tolgono), e questo conferma che l'arena sia tornata al layout verificato.

**⚠️ Dove lanciarlo, perché il posto sbagliato misura un'altra mappa.** In PIE il GameMode esegue
`ApplyMapSource` nel proprio `BeginPlay`, e se la sorgente non è il livello il verificatore misura
l'arena **generata** invece della tua. Due vie corrette, nessuna delle quali tocca un binario:

- dalla **console dell'editor**, col livello aperto e **senza premere Play**: `ApplyMapSource` non gira;
- in **PIE dopo `rt.Map.Source LevelAsset`**: la cvar scavalca la proprietà e lo dichiara nel log.

Il tell che conta non è un numero da confrontare a memoria — è il **warning**, che nomina la causa invece
del sintomo:

```
MapSource=GeneratedTestArena: ... La mappa del livello e' ignorata
```

Se compare, stai misurando la mappa sbagliata: nessun `[ok]` e nessun `[ko]` di quella run vale.

Se un criterio è diventato rosso **e stavi misurando la mappa giusta**, non è un difetto del mode: è la
mappa cambiata sotto le voci. Il rimedio è **chiudere l'editor senza salvare e rilanciare `rt.Arena.Check`**
— l'asset su disco non è mai stato toccato, quindi torna al layout verificato da sé. ⚠️ **Non** «rigenera
con `Generate Arena V01 Into Asset`»: rigenerare riscrive il package, che senza lease è precisamente il
gesto vietato, e butterebbe via un'arena committata per un cambiamento mai finito su disco.

## 11. Chiudere

- le sette voci hanno un esito reale in `test-manuali-pie.md`;
- i due artefatti compaiono in `git ls-files`;
- il verificatore dà `[ok]` sui tre criteri (o hai deciso e annotato di cambiarne le soglie).

Poi `#451` si chiude, e si sbloccano **U13** (che estende quest'arena) e **U19** (che la misura).

---

## Se qualcosa va storto

| Sintomo | Causa | Rimedio |
|---|---|---|
| `git add` non fa nulla, nessun errore | il percorso non è in allowlist | `git check-ignore -q <file>`; deve dare `1` |
| La partita non si allestisce | `Scenario To Run` o `rt.Test.Scenario` non vuoti | svuotali; la console **prevale** sulla property e dura quanto il processo |
| Compare una griglia quadrata | non può più: `ARTGridActor` è stato rimosso al CP 7.2 | — |
| Il gizmo resta dopo il cambio tool | è il difetto che `PIE-HEX-MODE-G` cerca | annota ❌ e continua |
| Build fallisce con `LNK1104` | l'editor è aperto e tiene le DLL | chiudilo e ricompila |
