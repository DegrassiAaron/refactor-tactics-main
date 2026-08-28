Verification done. Here is the specification.

> ⚠️ **Nota di consegna (2026-08-26).** La **Fase 1** di questa specifica — binario di fase ed
> esagono di superficie, entrambi cotti nel master — e' **implementata** in
> `tools/hud-assets/generate_hud_assets.py`, con i gate T1/T3/T5/T6/T7 che fanno uscire il
> generatore con 1. La **Fase 2** — il registro delle conseguenze a runtime — **non** e'
> implementata: richiede C++ ed e' fuori dallo scope di un generatore di asset.
>
> Le misure interne al documento sono state prese durante la stesura, mentre il pack cresceva:
> vanno **rifatte** prima di ogni decisione, non copiate. Il documento lo dichiara gia'.


---

# RT — Alfabeto iconografico: fase e conseguenza

**Stato**: proposta di decisione, non canone.

> 🔴 **SUPERATA IN PARTE dal 2026-08-28 — [D-232](../../../../decisions/RT_PDR_00_Decision_Log.md).** Il
> colore non dice più la **famiglia semantica**: dice la **macro-fase del round**. Cade quindi la riga 254 di
> questo documento, dove adotta la palette per famiglia di `02-color-system.md`.
>
> ✅ **Ciò che NON è superato, ed è la maggior parte**: il **binario di fase** e i suoi gate T1/T3/T5/T6/T7
> restano validi e in produzione. Misurato: il binario non porta la macro-fase del round — porta la **fase di
> risoluzione** di [`RTActionDef.h`](../../../../../Source/RefactorTactics/Ability/RTActionDef.h)
> (`Preparation` · `FastMovement` · `NormalMovement` · `Control` · `Attack` · `Environment`), che è un'altra
> informazione. Le due convivono su due canali diversi: il binario dice *in che ordine risolve dentro il
> turno*, il colore *in quale fase del round si gioca*.
>
> ⚠️ E il colore **non sostituisce** il marker: il vincolo grayscale impone che la fase resti leggibile senza
> tinta, quindi il colore è ridondanza deliberata.
 Il canone dell'iconografia resta **D-031** + `ERTIconCategory` (`Source/RefactorTactics/UI/RTIconCatalogData.h:19-45`). Dove questo documento diverge dal codice o dal Decision Log, vincono codice e Decision Log (`01-principi.md`, statuto).

**Owner**: *come* fase ed effetto sono rappresentati. *Dove e quando* compaiono resta di `progettazione-hud.md` §7/§31/§49. Nessuna regola vive in due posti.

**Misure**: tutte prese sul branch corrente il 2026-08-26, rasterizzando gli SVG generati a 240 px (10 px per unità di griglia, alpha > 24). Generatore `tools/hud-assets/generate_hud_assets.py`, md5 `87561af6cc8965dd5362338dd08e0d99`, working tree **sporco** (`git status`: `M tools/hud-assets/generate_hud_assets.py`).

> ⚠️ **Il pack si è mosso durante la stesura.** Fra la prima e la terza misura `Content/RT/UI/_Generated/Icons/` è passato da **83 a 123 SVG**. I numeri dei tre giudizi in ingresso — «67 glifi», «82 chiavi» — sono entrambi scaduti. Il pack oggi è **123 glifi su 11 categorie**: Action 58, Warning 13, Status 11, Reaction 10, MapInteraction 8, Environment 7, Identity 6, Phase 4, Certainty 3, Information 2, MissingIcon 1. Per **D-178** questo lavoro e quello che sta rigenerando i glifi non possono convivere in una working directory: vanno in fila, e ogni misura qui va **rifatta** al momento dell'implementazione, non copiata.

---

## 1. La regola in una riga

> **Un'icona d'azione dice, nell'ordine in cui si legge: QUANDO risolve (una tacca sul binario alto, prima del verbo) → CHE COSA è (il glifo, invariato) → CHE COSA LASCIA (il registro delle conseguenze, solo dove c'è spazio).**

E la regola che decide l'architettura, che è verificabile e non è un'opinione:

> **Si cuoce nel master solo ciò che una variante d'arma non può cambiare.**

`FRTAbilityVariant` (`Source/RefactorTactics/Ability/RTActionData.h:20-58`) dichiara esattamente due campi di contenuto: `Effects` — con il commento «sostituiscono per intero `URTActionData::Def.Effects`» — e `Parameters`. **Non contiene `ResolutionPhase`, non contiene `bCreatesSurface`, non contiene `Shape`.** Quindi fase e superficie creata sono affermazioni che restano vere sotto qualunque loadout e si possono cuocere nel PNG; gli effetti no, e cuocerli sarebbe una bugia non rilevabile dal giocatore (E4).

Questa riga divide l'alfabeto in due consegne indipendenti, che si spediscono separatamente:

| | Cosa | Dove vive | Costo chiavi | Costo runtime |
|---|---|---|---|---|
| **Fase 1** | fase + superficie creata | **cotto nel master**, `compose(base, …)` a build time | **zero** | **zero** |
| **Fase 2** | conseguenze (effetti) | **layer a runtime**, IconId propri | ~6 chiavi | 1 `UImage` in più, solo ≥32 px |

La Fase 1 si può spedire senza toccare una riga di C++. La Fase 2 no. Sono separabili di proposito: se la Fase 2 non passa la review, la Fase 1 resta valida.

---

## 2. Gli assi e i loro canali

### 2.1 Quadro d'assieme

| Asse | Canale visivo | Cotto o runtime | Valori resi | Taglia minima |
|---|---|---|---|---|
| **Fase** `ERTResolutionPhase` | posizione di una tacca piena su un binario orizzontale alto, **spezzato al centro** | cotto | 6 (≥32 px) → 4 (24 px) → 2 (16 px) | 16 px |
| **Superficie creata** `bCreatesSurface`+`SurfaceCreated` | **cella esagonale** al terminale destro della corsa di suolo | cotto | 3 spediti (Fire, ShallowWater, Smoke) | 32 px |
| **Conseguenza** `Effects[]` | registro a 4 stazioni fisse accese/spente sul gutter destro | runtime | 4 bucket × (applica \| rimuove) | 32 px (segno: 48 px) |
| **Forma** `ERTAbilityShape` | — | **non codificato** | — | — |
| **Slot** `ERTActionSlot` | — | **non codificato** | — | — |
| **Superficie della cella** `FRTHexCellData::Surface` | — | **non codificato** | — | — |

### 2.2 Fase — il binario alto

**Perché il binario alto e non altro.** Misurato oggi sui 123 glifi: la banda `y < 2.6` è invasa da **25 glifi**, di cui solo **8 sono `Action.*`** (BasicAttack, Brace, Electrify, HeavyAttack, Hero.Wraith.Feint, Hero.Wraith.PassingBlade, PrecisionAttack, Shield). La banda bassa **non è disponibile**: `hero_sigil()` (`generate_hud_assets.py:2148`, `HERO_SIGIL_Y = 21.6`) la occupa su **tutte e 20 le ability d'eroe** — è già la marca di provenienza, ed è in produzione. La banda bassa è spesa; la banda alta costa 8 glifi.

**Geometria.** Riusa le costanti già nel generatore (`RAIL_TOP_Y = 1.4`, `RAIL_X0, RAIL_X1 = 3.4, 20.6`).

```
guida sinistra   M3.4 1.4 L10.6 1.4     stroke 0.7, stroke-opacity 0.4
guida destra     M13.4 1.4 L20.6 1.4    stroke 0.7, stroke-opacity 0.4
VARCO CENTRALE   x 10.6 .. 13.4         2.8 unità, MAI inchiostro
tacca            M cx-1.0 1.4 L cx+1.0 1.4   stroke 2.3, piena
```

Sei stazioni, in **ordine di turno reale** (`URTCatalogLibrary::MapResolutionPhase`, `RTCatalogLibrary.cpp:169-185`, funzione totale), non in ordine di `ResolutionPhaseCode`:

| Stazione | `ERTResolutionPhase` | macro-fase | `cx` | lato |
|---|---|---|---|---|
| 1 | `Preparation` (10) | Prep | **4.4** | sinistra |
| 2 | `FastMovement` (20) | Dash | **6.8** | sinistra |
| 3 | `Control` (30) | Blast | **9.2** | sinistra, **contro il varco** |
| — | *varco* | — | 10.6–13.4 | — |
| 4 | `Attack` (40) | Blast | **14.8** | destra, **contro il varco** |
| 5 | `NormalMovement` (20) | Move | **17.2** | destra |
| 6 | `Environment` (50) | Cleanup | **19.6** | destra |

`Snapshot` e `Cleanup` **non hanno stazione**. Sono zero azioni su 57 (misurato: `action_axes()` restituisce `FastMovement 5, NormalMovement 2, Attack 9, Preparation 6, Control 12, Environment 3` per le 37 generiche; `hero_ability_axes()` restituisce `Attack 7, Preparation 4, Control 4, FastMovement 3, Environment 2` per le 20 d'eroe). Se una si popola, il generatore **solleva** — non inventa una posizione.

**La decisione che distingue questa specifica dalla proposta da cui nasce, e perché.** La proposta originale metteva sei stazioni a passo uniforme. Il primo giudizio l'ha dichiarata fatale per due ragioni indipendenti, ed entrambe le accetto:

1. *Periodicità.* Sei tacche a passo costante sotto ~3 px non leggono come struttura ma come **texture punteggiata**, cioè un falso positivo su `Certainty.Uncertain` (`05-certainty-states.md` §2, canale non riassegnabile). Il fallimento peggiore non è «illeggibile»: è «dice la cosa di un altro asse».
2. *La coppia sbagliata muore per prima.* Con un passo uniforme, `Control`(30) e `Attack`(40) — la coppia che decide il turno, «riesco a deflettere prima che colpisca?» — erano le due stazioni più vicine e le prime a fondersi.

**Correzione**: il binario non è periodico. Ha **una sola discontinuità**, il varco centrale, e le due stazioni che lo toccano sono esattamente `Control` e `Attack`. Conseguenze misurabili:

- il passo non è costante (2.4 dentro un lato, **5.6 attraverso il varco**), quindi la figura non è periodica a nessuna scala e non può fingere un tratteggio;
- la coppia decisiva non è più la prima a morire ma **l'ultima**: a 16 px, dove sopravvive solo «da che lato sta la tacca», `Control` è a sinistra del varco e `Attack` a destra. La distinzione che conta di più è quella che regge di più.

**Costo di questa scelta, dichiarato**: `Preparation` e `FastMovement` sono entrambe a sinistra e `NormalMovement` e `Environment` entrambe a destra. Alle taglie basse si fondono a coppie. Sono fusioni fra vicini in ordine di turno, mai riordini.

**Le stazioni non si spostano mai fra una taglia e l'altra.** Non esiste un secondo master per la 16 px, non esiste un riallineamento dichiarato. Ciò che degrada è la **risoluzione del lettore**, non la geometria. Un revisore che confronta l'export a 16 e quello a 32 deve trovare la tacca nello stesso punto.

### 2.3 Superficie creata — la cella esagonale

Tre azioni su 57 impostano `bCreatesSurface` (verificato per grep): `Action.Ignite` (Fire, `SurfaceRadius = 0`), `Action.CreateWater` (ShallowWater, raggio 1), `Hero.Phase.MistVeil` (Smoke, raggio 1). Tutte e tre hanno **`Effects` vuoto**.

Senza questo canale quelle tre azioni escono con il registro spento e leggono «non fa niente», che è falso: il loro esito *è* la superficie. È l'unica ragione per cui il canale esiste, e copre il 5% del catalogo. Lo si spende comunque, perché il 5% che copre è il 5% che altrimenti mente.

Il contenitore è la **cella esagonale** che `04-regole-di-composizione.md` §4 promette alle superfici e che nessuno ha mai speso: `polygon(hexagon(cx, cy, 2.2))`, stroke 1.3, al terminale **destro** della corsa di suolo del glifo — riquadro `x 17.4..21.6, y 18.8..23.2`. Dentro, la riduzione minima dell'elemento, ripresa dalle primitive già in uso:

| `SurfaceCreated` | Segno dentro l'esagono | Primitiva di origine |
|---|---|---|
| `Fire` | una lingua sola, arco asimmetrico che sale e si piega | `g_ignite` |
| `ShallowWater` | un'onda ridotta | `waves()` |
| `Smoke` | due barre orizzontali sfalsate, lunghezza decrescente | `g_phase_mist_veil` |
| `Ice`, `Conductive`, `Rough`, `Void`, `HighGround` | **disegnati e non spediti** | — |

Ice e gli altri restano fuori: nessuna azione li produce, e una chiave o un segno che il gioco non genera non risolve mai (A15).

Il **raggio non si codifica**. `SurfaceRadius` è un numero, e `progettazione-hud.md` §41.2 vieta di cuocere un'area in un asset fisso. `Ignite` (raggio 0) e `CreateWater` (raggio 1) portano lo stesso esagono.

### 2.4 Conseguenza — il registro (Fase 2, runtime)

Non cotto, per la ragione della §1. Vive su un canvas di composizione **24 → 28**: il master 24×24 non si sposta e non si scala, si aggiunge un gutter di 4 unità **solo a destra**. Il bbox massimo dell'inchiostro misurato oggi è `x = 23.2` (`Action.Hero.Phase.PressureJet`), quindi il gutter è pulito per costruzione; i 28 glifi con inchiostro oltre `x = 22.0` restano dentro il proprio box e non entrano nel gutter.

Colonna a `x = 25.8`, quattro stazioni fisse a `y = 5.6 · 10.4 · 15.2 · 20.0`, passo 4.8. **Il pavimento della colonna si disegna sempre** — `M25.8 3.4 L25.8 22.2`, stroke 0.7, opacity 0.4 — così una colonna spenta legge «zero conseguenze dichiarate» e non «il layer manca». Questa distinzione non è cosmetica: **22 azioni su 57** (14 generiche + 8 d'eroe) dichiarano `Effects` vuoto, cioè il caso più comune del catalogo.

I quattro bucket, con le frequenze misurate su tutte e 57 le azioni:

| Stazione | Bucket | `ERTActionEffect` raccolti | Occorrenze |
|---|---|---|---|
| y 5.6 | **HARM** | `Damage`, `DamageStructure` | 19 |
| y 10.4 | **BODY** | `Heal`, `Shield`, `DamageReduction` | 6 |
| y 15.2 | **DISPLACE** | `Push`, `Pull`, `SelfReposition`, `CancelDisplacement` | 7 |
| y 20.0 | **STATE** | `Status`, `CancelStatus`, `SetDoorState` | 11 |

Bucket e non valori: un `ERTActionEffect` nuovo atterra su un bucket esistente **a costo zero di asset**. Il canale cresce con la semantica dell'enum, non con l'enum.

Segno, che è la grammatica salvata dalla terza proposta: **pieno = applica, contorno aperto = riduce o rimuove**. `Shield` è un semidisco chiuso e pieno; `DamageReduction` è la stessa curva, aperta e di contorno. Pieno contro contorno è il canale che il censimento misura vergine — in tutto il generatore l'unico `fill` è `dot()`, nessun `path` o `polygon` chiuso è riempito — ed è la sostituzione lecita di ↑/↓/⊘ senza toccare verde e rosso (LOCKED 10).

**Il registro non sceglie.** `FRTActionDef::Effects` è una lista ordinata senza campo «effetto dominante»; l'unico riduttore in codice è `URTCatalogLibrary::FirstDamage`, che risponde a *quanto fa male*, non a *qual è il principale*. Un alfabeto che pretendesse un glifo per azione dovrebbe inventare quella regola nella UI, e sarebbe una seconda verità senza owner nel gameplay. Quattro stazioni indipendenti non hanno niente da inventare: accendono quello che c'è.

**Quante ne accendono davvero**, misurato: su 57 azioni, **52 hanno al massimo un bucket**; 4 ne hanno due (`Action.Charge`, `Hero.Riktor.ImpactShot`, `Hero.Riktor.Ram`, `Hero.Gadget.ReactiveCapacitor`); **una sola** ne ha tre (`Hero.Phase.PressureJet`). Il registro non è mai affollato.

---

## 3. Come si compone — otto azioni reali

Ordine di lettura, che estende `04-regole-di-composizione.md` §1 (`TARGET + GEOMETRY → EFFECT + MODIFIER`) verso l'esterno: **BINARIO (quando) → GLIFO (che cosa) → ESAGONO (che materia scrive) → REGISTRO (che cosa lascia)**.

---

**1. `Action.Guard`** — `Preparation`, slot Main, `Effects = [Status(Status.Guarded)]`.
Binario: tacca a **x 3.4→5.4**, stazione 1, la più esterna a sinistra. Guida spezzata al centro.
Glifo: `g_guard` invariato — l'arco frontale `M4.8 14.2 A 8.4 8.4 0 0 1 19.2 14.2`, il punto corpo, la base. È il glifo con più inchiostro basso della tavola e non incontra mai il binario, che è in alto.
Esagono: nessuno. Registro: un pip **pieno** a STATE (y 20.0).
Lettura: «si arma per primo, prima di tutto; lascia uno stato; non fa male a nessuno».

---

**2. `Action.Deflect`** — `Control`, slot **Reaction**, `Effects = [DamageReduction]`.
Binario: tacca a **x 8.2→10.2**, stazione 3, **appoggiata al bordo sinistro del varco**.
Glifo: `g_deflect` invariato — superficie inclinata `M6.6 18.6 L18.6 6.6` stroke 2.4 con le due frecce di rimbalzo.
Registro: un solo pip a BODY (y 10.4), **contorno aperto** = riduce.
Lettura: «risolve nello scambio, **prima** del colpo; trattiene invece di colpire».
È il caso che giustifica il varco: `Deflect` a x 9.2 e `HeavyAttack` a x 14.8 sono due tacche separate dal solo buco della guida. Il confronto che decide il turno è il confronto più facile della tavola, non il più difficile.

---

**3. `Action.HeavyAttack`** — `Attack`, slot Main, `Effects = [Damage, DamageStructure]`.
Binario: tacca a **x 13.8→15.8**, stazione 4, appoggiata al bordo **destro** del varco.
Glifo: `g_heavy_attack`, con una correzione obbligata: oggi l'inchiostro sale a `y = 2.30` ed entra nella banda del binario. Va abbassato di 0.6 unità.
Registro: **un solo pip pieno** a HARM. E qui l'alfabeto dichiara un limite invece di fingere: `Damage` e `DamageStructure` cadono entrambi in HARM, quindi due effetti accendono un pip solo. Il registro dice «fa male», non «fa male anche alle strutture».
Accanto a `Action.PrecisionAttack` (stessa stazione, stesso pip): a separarle resta il verbo, ed è corretto — hanno davvero la stessa collocazione nel turno e la stessa classe di conseguenza.

---

**4. `Action.Charge`** — `FastMovement`, slot Movement, `Effects = [Damage, Push]`, `Movement = LinearCharge`.
Binario: tacca a **x 5.8→7.8**, stazione 2.
Glifo: `g_charge` invariato — dot d'origine, due chevron, barra d'impatto. Inchiostro `y 4.0..19.0`: non tocca nulla, **costo zero**.
Registro: **due pip pieni**, HARM e DISPLACE.
Il `SelfReposition` che il brief attribuisce a Charge **non compare**: nel catalogo è il profilo `Movement = LinearCharge`, non un `Effect` (`action_axes()['Action.Charge']` restituisce `effects: [Damage, Push]`). Il registro disegna il dato, non l'aspettativa.

---

**5. `Action.Move`** — `NormalMovement`, slot Movement, `Effects = []`.
Binario: tacca a **x 16.2→18.2**, stazione 5, **a destra del varco**.
Glifo: `g_move` invariato, con i suoi nodi intermedi.
Registro: **tutte e quattro le stazioni spente, pavimento presente**.
È il caso che il binario guadagna e che nessun'altra superficie dell'HUD mostra: `Action.Move` ha `ResolutionPhaseCode` 20, lo stesso di `Dash` e `Charge`, ma risolve nella macro-fase `Move`, **dopo** il Blast. Il codice dice «insieme al Dash», il turno dice «dopo il colpo». Il binario segue il turno, perché è il turno che il giocatore vive.
Contro `Action.Dodge` (`FastMovement`, `Effects` vuoto): stesso registro spento, tacche su **lati opposti** del varco. Due azioni di movimento che il codice numera uguale e che il turno separa: è la distinzione che l'alfabeto aggiunge davvero.

---

**6. `Action.Ignite`** — `Environment`, slot Main, **`Effects = []`**, `bCreatesSurface = true`, `SurfaceCreated = Fire`, `SurfaceRadius = 0`.
Binario: tacca a **x 18.6→20.6**, stazione 6, la più esterna a destra.
Glifo: `g_ignite` — la corsa di suolo va **accorciata** a `x ≤ 16.8` per fare posto all'esagono (misurato: oggi occupa 652 celle nel riquadro BR).
Esagono: `hexagon(19.5, 21.0, 2.2)` con dentro la lingua di fuoco ridotta.
Registro: **spento**, pavimento presente.
Lettura completa: «a fine turno, dopo il Move, questa cella diventa fuoco, e a nessuna unità succede niente». Il registro vuoto **è** l'informazione.
⚠️ **Correzione al brief**: `Action.Ignite` non ha né `Damage` né `Status`. `RTCatalogLibrary.cpp:1470-1475` le assegna zero `Effects`; il danno del fuoco viene dalla superficie, non dall'azione. È il caso che dimostra perché la composizione deve leggere i campi e non una tabella scritta a mano.

---

**7. `Action.BasicAttack`** — `Attack`, slot Main, **`Effects = []`**.
Binario: tacca a x 13.8→15.8, stazione 4. Il glifo va corretto: inchiostro a `y = 1.70`, dentro la banda.
Registro: **spento**.
E qui l'alfabeto mette a schermo una verità scomoda e la mette **giusta**: `Action.BasicAttack` non dichiara effetti perché il suo danno vive nella variante d'arma dell'istanza. Accanto a `Action.PrecisionAttack`, che accende HARM, `BasicAttack` sembra «non fare danno». **Non lo sembra a runtime**, perché il registro si compone dall'istanza `URTActionData` e la variante attiva riempie `Effects`; lo sembra **nella contact sheet**, che compone dal catalogo statico. La contact sheet lo mostrerà, e va scritto nella scheda di consegna che è così di proposito.

---

**8. `Hero.Phase.PressureJet`** — `Attack`, `Effects = [Damage, Status(Status.Wet), Push]`, l'unica azione del gioco con tre effetti.
Binario: tacca a x 13.8→15.8, stazione 4.
Glifo: il verbo, più `hero_sigil("Phase")` in basso a sinistra — la marca di materia già in produzione (`generate_hud_assets.py:2000`), che dice a quale eroe l'ability appartiene. **Binario in alto e sigillo in basso non si toccano mai**: sono i due canali che il generatore ha già misurato liberi, uno per estremità.
Registro: **tre pip pieni**, HARM, DISPLACE, STATE. BODY spento.
È il caso limite del registro ed è l'unico in 57. Un alfabeto che pretendesse un glifo per azione avrebbe dovuto scegliere quale dei tre disegnare; il registro non sceglie.

---

## 4. Cosa non codifichiamo, e perché

Il canale a cui si rinuncia vale quanto quelli usati: ogni asse non codificato è spazio che resta al verbo e una collisione che non si apre.

**4.1 `ERTAbilityShape` — la forma.** Tre ragioni indipendenti, ognuna sufficiente.
- *È già disegnata, meglio.* `ARTHUD::ComputePlannedHitMarks` (`UI/RTHUD.cpp:69-79`) legge `Shape`, `RangeCells`, `AreaRadius` e chiama `URTHexCombatLibrary::HexHitCells`. La forma è resa **come celle reali sulla mappa**, a piena fedeltà. Codificarla in 6 unità di glifo è una miniatura peggiore dell'originale.
- *Per gran parte del catalogo è già il verbo.* `LineAttack` = Line, `CircularAoE` = Area, `Overwatch`/`ModifyArc` = Cone, `PrecisionAttack` = Single. Il glifo la dice già.
- *È l'unico asse che non sta sul `Def`.* Vive su `URTActionData::Shape` (`RTActionData.h:118`), non in `FRTActionDef`. Costa il puntatore all'istanza e non si recupera da un `ActionId`.

**4.2 `ERTActionSlot` — lo slot.** Non si codifica, ed è il canale che l'alfabeto **restituisce** invece di consumare. Il dato **arriva già** al widget (`FRTAbilityCooldownView::Slot`, `RTHudViewModel.h:158`, popolato a `RTHudViewModel.cpp:151`) e la superficie che lo mostra esiste già: le due corsie della Action Dock (`progettazione-hud.md` §6.7). Metterlo anche nel glifo sarebbe una seconda verità senza owner.
Unica eccezione, già in vigore e non toccata: le azioni con `Slot = Reaction` conservano la grammatica di cerchio e anello spezzato di `01-principi.md` §3 — è una scelta di **famiglia di forma**, non una codifica di slot.
⚠️ C'è una discrepanza aperta e non è mia da risolvere: il catalogo passa `ERTActionSlot::Movement` per `Action.Sprint` (`RTCatalogLibrary.cpp:1029`) mentre `RTHudSlotLinesTests.cpp:68` e `RTPlanValidationTests.cpp:87` affermano `MovementAndMain`. Va risolta nel gameplay, non compensata nella UI — e non codificare lo slot significa che l'alfabeto non la eredita.

**4.3 La superficie della **cella** (`FRTHexCellData::Surface`).** Non si codifica, per due motivi.
- *Duplicherebbe un canale esistente.* `URTHexLibrary::SurfaceRingCount` (D-183) è **già** il secondo canale delle superfici, in world space, ad anelli concentrici. Inventarne uno nel glifo crea una seconda grammatica accanto a una che funziona.
- *È l'unica query davvero nuova.* `URTHexMapAsset::FindCell` non è `UFUNCTION` (`Map/RTHexMapAsset.h:329`) e in tutto il modulo Map non esiste un `BlueprintPure` che risponda «che superficie ha questa cella». Rifiutando l'asse, l'intera Fase 1 non tocca il gameplay.
La conseguenza è che teniamo **separati** i tre dati che `03-forme-e-primitive.md` §7 dichiara collidenti: cosa l'azione *scrive* (esagono), cosa l'unità *subisce* (`Status.*`, glifo proprio con la sua famiglia), cosa la cella *è* (anelli sul mondo). Un glifo «fuoco» generico li collasserebbe tutti e tre.

**4.4 Gli stati sugli intenti.** Il binario **non si disegna sugli intenti nemici in v0.1**. `FRTIntentView` porta l'azione come `FText ActionName` (`RTIntentPrivacyLibrary.h:169`, popolato da `Planned->DisplayName`) e **non ha `ActionId`**: nessun asse è ricostruibile su quel percorso. Ma il punto non è tecnico. In un tattico a fasi simultanee la `ResolutionPhase` di un'azione avversaria è l'informazione più azionabile che esista, ed è **una decisione di privacy sotto D-021**, non un refactoring. Il precedente e il costo sono visibili: `DashStyle` è l'unico campo strutturato sopravvissuto al filtro, e ogni campo nuovo passa da `FilterForTeam` e dal test `RefactorTactics.UI.NoEnemyIntentExposed`.

**4.5 Quale effetto, quando sono due dello stesso bucket.** `Action.Brace` ha `[Status(Braced), Status(Root)]` e accende **un** pip. Il registro dice «lascia uno stato», mai *quale*. È un limite dichiarato, non un difetto da aggirare: *quale* stato lo dice il glifo `Status.*` sull'unità, che è una vista diversa.

---

## 5. Convivenza con i canali già occupati

| Canale occupato | Da chi | Come l'alfabeto lo evita |
|---|---|---|
| **tratto solido / tratteggiato / puntinato** | asse `Certainty` (`05` §2) | Binario, esagono e registro sono **tratto continuo e massa piena**, mai un pattern di trattini. E il binario è **aperiodico** per costruzione (§2.2), quindi non può degradare in una texture punteggiata che imiti `Uncertain`. |
| **slash / cross-hatch / ⊘** | asse `Validity` = `Invalid` | Nessun segno dell'alfabeto attraversa il glifo. Il registro sta nel gutter, il binario in una banda riservata. |
| **desaturazione + frame dedicato** | `Disabled` | L'alfabeto non usa opacità come portatore: le uniche opacità sono `0.4` sulle **guide**, che sono riferimento e non valore. `Disabled` continua a operare sull'intero gruppo. |
| **esagono ALLUNGATO (punte ai lati)** | chip di macro-fase | L'esagono di superficie è **regolare, r 2.2, piatto**, dentro il glifo, e non racchiude il glifo. La regola dura è comunque un'altra: vedi sotto. |
| **esagono A PUNTA IN ALTO** | badge `Identity.*` | Idem. |
| **punto unità in basso al centro** | famiglia `Status.*` | Il registro sta a `x = 25.8`, fuori dal box 24. Il binario a `y = 1.4`. Nessuno dei due tocca l'impronta a `STATUS_UNIT_Y = 19.6`. |
| **banda bassa sinistra** | `hero_sigil()`, marca di materia, **in produzione** | Il binario è in alto. Misurato: tutte e 20 le ability d'eroe hanno inchiostro oltre `y = 21.4`, che è il sigillo. La banda bassa è **spesa** e questo alfabeto non la reclama. |
| **palette Okabe-Ito per famiglia** | `02-color-system.md` §2 | Vedi la regola di tinta qui sotto. |

**Regola di esclusione dura.** Un glifo riceve il binario **solo** se ha una `ResolutionPhase`. Non la ricevono mai: `Phase.*` (4), `Identity.*` (6), `Certainty.*` (3), `Reaction.*` (10), `Warning.*` (13), `MapInteraction.*` (8), `Environment.*` (7), `Information.*` (2), `Status.*` (11), `MissingIcon`. Sono **65 glifi esenti su 123**; il binario riguarda i 58 `Action.*`.
La funzione di composizione **rifiuta con un errore** una richiesta di binario su una chiave esente — non la salta in silenzio. Due contenitori o due grammatiche sullo stesso asset sono un difetto, e un `if` che salta lo nasconde. In questo repository una convenzione senza gate è già uscita di scena una volta (**D-182**, `scripts/check-docs-naming.py`).

**Regola di tinta.** Binario, esagono e registro **non si tingono mai** con la tinta semantica di famiglia: restano `Frame_Mid #4A5568` per la struttura spenta e `White #FFFFFF` per la tacca accesa. La tinta di famiglia (C4) continua ad applicarsi al solo glifo interno.
Motivo, e non è stilistico: è una differenza di **luminanza**, non di tinta, cioè quello che `progettazione-hud.md` §47-bis.2 chiede («contrasto, non saturazione») e che sopravvive a prot/deut/tritanopia per costruzione. E chiude preventivamente il precedente di C9: la premessa «non competono mai sulla stessa superficie» è già stata falsificata una volta da §6.7, e un alfabeto che tinge due superfici a poche unità di distanza dentro lo stesso glifo la rifarebbe.
**Unica eccezione**: l'esagono di superficie porta il colore proprio dell'elemento (Water `#0072B2`, Fire `#D55E00`, indipendente dalla squadra, C5), perché un fuoco alleato è del colore di uno nemico. La forma dentro l'esagono resta il primo canale e regge da sola.

**Layer e gerarchia.** Il binario e l'esagono sono **dentro** l'asset dell'IconId: vivono nel layer `icon` di `progettazione-hud.md` §7.1, non aggiungono un layer. Il registro è un layer separato **fra `icon` e `state overlay`**. Per §47.4 tutto l'alfabeto è **livello 3** (modifier). Coesiste con gli otto stati di §7: `Cooldown` e `Unavailable` restano testo e numero, `Invalid` resta la mask, `Selected` resta l'ambra e la cornice interna di `f_slot`, `Warning` resta forma+icona+pattern+colore (F33).

**Il notch di `f_slot()`, che è un attrito reale e non risolto qui.** La cornice dello slot incide un pip di stato in alto-centro: `M23 3 L32 3 L36 8 L28 8 Z` su 64×64 (`generate_hud_assets.py:504`), cioè il 36–56% della larghezza. Il varco centrale del binario sta al 44–56% della larghezza dell'icona, quindi **cade sotto il notch** — il che è fortunato, perché è la parte del binario che è deliberatamente vuota. Ma la stazione `Control` (x 8.2–10.2 = 34–43%) ci finisce sotto in parte. Va verificato in PIE con l'inset reale dell'icona nello slot. Non lo decido qui: è §7/§49, appartiene all'owner dell'HUD (B2). Opzioni in §9.

---

## 6. Degradazione

`PNG_SIZES = [16, 20, 24, 32, 48]`, `MIN_READABLE = {"Phase": 24, "Certainty": 20}`, `DEFAULT_MIN_READABLE = 16` (verificato, `generate_hud_assets.py:1910-1920`). Una unità di griglia vale `taglia/24` px.

| Export | 1 unità | Binario | Esagono superficie | Registro |
|---|---|---|---|---|
| **48 px** | 2.00 px | 6 stazioni | sì | 4 stazioni, **pieno/contorno** |
| **32 px** | 1.33 px | 6 stazioni | sì | 4 stazioni, **solo pieno** |
| **24 px** | 1.00 px | **4 classi** | no | no |
| **20 px** | 0.83 px | **2 classi** | no | no |
| **16 px** | 0.67 px | **2 classi** | no | no |
| **< 16 px** | — | niente | no | no |

**Che cosa sono le classi, valore per valore.**

- **≥32 px — sei valori.** Le sei stazioni si risolvono contro la guida, i suoi due estremi e il varco. Passo intra-lato 2.4 unità = 3.2 px a 32 px.
- **24 px — quattro classi.** `{Preparation, FastMovement}` · `{Control}` · `{Attack}` · `{NormalMovement, Environment}`. Le due centrali si risolvono perché sono **appoggiate al bordo del varco**, che è l'unica discontinuità della figura e quindi il riferimento più forte disponibile; le quattro esterne si fondono a coppie perché il loro riferimento è solo l'estremo della guida, a 2.4 unità = 2.4 px.
  ⚠️ **Questa è una scommessa, non una misura.** L'affermazione «a 24 px la tacca appoggiata al varco si distingue da quella che non lo è» non è stata verificata su una persona. Va misurata sulla calibration sheet e in **E21**. Se cade, il gradino a 24 px scende a due classi come il 20, e la specifica resta valida con una degradazione più ripida.
- **20 e 16 px — due classi.** Sopravvive solo il **lato**: tacca a sinistra del varco = «risolve **prima** del colpo» (`Preparation`, `FastMovement`, `Control`); tacca a destra = «risolve **dal colpo in poi**» (`Attack`, `NormalMovement`, `Environment`). La tacca è massa piena 2.0×2.3 unità = 1.3×1.5 px a 16 px, aperiodica, contro una guida che a quella taglia non si vede più. Non è un righello: è un interruttore, e la tacca è comunque nella sua posizione vera.

**Perché questa scala e non un'altra: si sceglie che cosa perdere in base al danno tattico, non ai pixel.**
La soglia che sopravvive fino in fondo è **`Control` | `Attack`**, cioè il confine di pre-emption. In un gioco a fasi simultanee «la mia cosa atterra prima o dopo la loro» è la domanda che decide il turno, e sotto densità (A12, A13) il giocatore ha ~1 secondo. Le fusioni che accetto sono, in ordine:
1. `NormalMovement` con `Environment` — entrambe risolvono dopo lo scambio, e `Environment` è 5 azioni su 57;
2. `Preparation` con `FastMovement` — collassa una distinzione che il **verbo** porta già da solo e che i documenti hanno già bloccato per quella coppia (LOCKED 7 e 9: `Move` con i nodi contro `Dash` con i chevron);
3. il secondo e terzo effetto — a 24 px il registro non esiste affatto, quindi non c'è troncatura: c'è assenza. **Nessuna regola di UI inventa un «effetto principale».**

**Che cosa si perde e va detto.** Sotto i 32 px un'azione non dice più che cosa lascia. Sotto i 24 px `Preparation` e `FastMovement` sono la stessa cosa. Sotto i 16 px l'alfabeto non c'è. E il binario, a 16 px, è una macchia di 1.3×1.5 px su un fondo che può essere acqua o fuoco: sopravvive perché è **massa piena** e non stroke (A11: acqua e fuoco mangiano gli stroke sottili), ma la **guida** — 0.7 unità = 0.5 px — sparisce ben prima. È voluto: la guida è la scala, la tacca è il valore. Se sopravvive solo la tacca, sopravvive l'informazione.

**Nessuna variante semplificata dedicata.** `01-principi.md` §3 la prevede; questo alfabeto non la usa, perché le stazioni non si spostano fra le taglie. È una scelta contro la proposta d'origine, che dichiarava una rottura di allineamento fra la 16 e la 24: una rottura del genere costringe un revisore a sapere quale gradino sta guardando, e nessun gate la può controllare.

---

## 7. Test di accettazione

Controlli **meccanici**, con `exit 1`, tutti dentro `tools/hud-assets/generate_hud_assets.py` accanto a `check_coverage()`. Non in `scripts/build-icon-assets.py`, che `06-accessibilita.md` §6 nomina e che **non esiste nel repository** (verificato: la cartella `scripts/` non risponde). Il documento owner va corretto.

**T1 — Banda del binario riservata.** Rasterizza ogni glifo `Action.*` e fallisce se un pixel `alpha > 24` cade in `y < 2.6`. Oggi fallirebbe su **8 glifi** (elencati in §8).

**T2 — Banda del binario, categorie esenti.** Per i glifi non-`Action.*`, fallisce se nella banda esiste una **corsa orizzontale continua ≥ 1.6 unità** — cioè qualcosa che possa leggersi come una tacca. Un tratto verticale o un arco nella banda è ammesso. Oggi va **misurato**: 17 glifi non-Action hanno inchiostro sopra `y = 2.6` (4 `Identity`, 8 `Reaction`, 2 `MapInteraction`, 2 `Status`, 1 `Warning`), e non so ancora quanti di quelli siano corse orizzontali.

**T3 — Riquadro dell'esagono di superficie.** Per i soli glifi con `bCreatesSurface`, fallisce se c'è inchiostro del verbo in `x 17.4..21.6 ∧ y 18.8..23.2`. Oggi fallisce su `Action.Ignite` (652 celle) e `Action.CreateWater` (138); `Hero.Phase.MistVeil` è già libero.

**T4 — Esclusione dura.** Fallisce se un binario o un esagono viene richiesto per una chiave di categoria esente. Il generatore conosce già la categoria di ogni chiave (la usa a `generate_hud_assets.py:2002` per `MIN_READABLE`).

**T5 — Fasi non popolate.** Fallisce se una qualunque azione del catalogo dichiara `Snapshot` o `Cleanup`. Non c'è stazione per loro, e il silenzio sarebbe peggio dell'errore.

**T6 — Aperiodicità.** Sulla riga del binario, fallisce se la distanza massima fra centri di stazione adiacenti è meno di **1.8×** la minima. È il controllo che impedisce a un futuro riassetto di riportare il binario a passo uniforme e quindi a un falso positivo su `Certainty.Uncertain`. Oggi il rapporto è `5.6 / 2.4 = 2.33`.

**T7 — Fase derivata, mai scritta a mano.** La stazione di ogni glifo si legge da `action_axes()` e `hero_ability_axes()` (`tools/hud-assets/action_axes.py`), che parsano il C++. Fallisce se un glifo `Action.*` non compare in nessuna delle due tabelle. Verificato: la copertura è **57/57** — `action_axes()` restituisce 37 azioni con `phase`, `hero_ability_axes()` restituisce 20 con `phase` e `shape`. *(La quarta proposta affermava che il parser non legge il catalogo eroi: è falso sul branch corrente, `action_axes.py:201` `HERO_CPP`, `:240` `hero_ability_axes`.)*

**T8 — Collisione, e chi vince.** `collision_check.py` gira **insieme** a T1–T7. Quando un vincolo di banda e una coppia di `DOC_PAIRS` litigano, **vince la coppia**: un glifo si sposta dalla banda solo se la sua distanza L2 non peggiora sotto le soglie già in uso (`0.14` separabile, `0.20` sicuro, `collision_check.py:127-128`). Altrimenti il glifo esce dall'alfabeto e la deroga si scrive. Motivo: le distanze in `DOC_PAIRS` sono misure di leggibilità **in grayscale già al limite**, e scambiare una collisione documentata con una ignota è una regressione anche quando il conto degli asset migliora.

**T9 — Due coppie da aggiungere a `DOC_PAIRS`, indipendenti da questo alfabeto.** `Action_Move` ↔ `Action_Dash` e `Action_BasicAttack` ↔ `Action_Overwatch` sono nelle quattordici coppie critiche di `03` §7 e oggi nessun gate le sorveglia con le nuove stazioni. Vanno misurate prima e dopo.

**T10 — Copertura, che oggi copre meno di quanto si crede.** `check_coverage()` (`generate_hud_assets.py:1652`) restituisce mancanti **ed extra**, ma la sua docstring dice esplicitamente «Gli extra non sono un errore» e `main()` fa `return 1 if missing else 0` (`:2087`). Quindi una chiave di layer che nessuno pretende **passa silenziosa** e a runtime `ResolveIcon` la risolve nel `MissingIcon` con `bResolved = false`. Per la Fase 2 questo è il rischio reale, ed è l'opposto di quello che le proposte temevano: le ~6 chiavi del registro vanno **emesse da `URTIconLibrary::RequiredIconIds()` iterando i bucket in C++**, non elencate a mano, altrimenti esistono nel pack e non esistono nel gioco (A15).

**Controlli non automatizzabili, che restano alla calibration sheet e a E21** (A16, A17, F11): la scommessa dei quattro valori a 24 px (§6); le quattro rese obbligatorie a geometria e layout invariati — Default Accessible, Grayscale, CVD, High Contrast (A4); i fondi di tortura, acqua e fuoco per primi (A10, A11); la prova di densità a schermata piena (A12) e la soglia di ~1 secondo (A13). **Nulla qui è stato misurato su un giocatore reale, e questo documento non è una dichiarazione di conformità WCAG né un audit.**

### Il test grayscale, e dove questa specifica non lo passa

Nessun canale dell'alfabeto è cromatico: posizione, massa piena, presenza/assenza, pieno contro contorno, silhouette dentro l'esagono. Portare la tavola in scala di grigi non toglie un pixel, perché non c'era niente di colorato da togliere.

**Le tre cose che il grigio non recupera, scritte qui invece che nascoste:**

1. **L'esagono di superficie perde l'elemento come *categoria*.** Fire, ShallowWater e Smoke si distinguono per forma (lingua, onda, barre sfalsate), e la forma regge. Ma la coppia **Smoke ↔ ShallowWater** è la più debole dell'alfabeto: due o tre tratti orizzontali morbidi contro un'onda, dentro un esagono da 4.4 unità. A 32 px valgono 5.9 px. È il primo test che rischio di fallire, e cade comunque sotto i 32 px.
2. **Il segno «applica / rimuove» del registro non esiste sotto i 48 px.** Un anello r 0.9 stroke 0.9 a 32 px è un disco da 2.4 px. Quindi `Action.Guard` (applica uno stato) e `Action.Purge` (ne rimuove uno) accendono la stessa stazione con lo stesso segno a 32 px. La distinzione la fanno il verbo e la famiglia di forma della reazione, non il registro. **Va scritto nella scheda di consegna** (G8) come feature ≥48 px, non spedito come se funzionasse ovunque.
3. **A 20 e 16 px l'asse fase ha due valori, non sei.** Non è una perdita di colore: è una perdita di risoluzione, e la dichiaro come tale in §6 invece di sostenere che il binario «funziona a tutte le taglie».

---

## 8. Costo

### 8.1 Glifi da toccare — **10 su 123**

Nessuno da riconcepire. Tutti delta di coordinate in `generate_hud_assets.py`.

**Banda del binario (`y < 2.6`), solo `Action.*` — 8 glifi**, con l'inchiostro più alto misurato:

| Glifo | `y` minima oggi | Intervento |
|---|---|---|
| `Action.Brace` | **0.90** | il chevrone di postura parte a `M9.8 1.6`. È l'unico glifo del pack che tocca la riga `y=0` ed è già il maggior trasgressore dei visual bounds. Va abbassato di ~2.0 unità o accorciato. **L'unico che richiede una decisione di disegno.** |
| `Action.Hero.Wraith.PassingBlade` | 1.30 | traslazione |
| `Action.BasicAttack` | 1.70 | traslazione |
| `Action.PrecisionAttack` | 2.10 | traslazione |
| `Action.HeavyAttack` | 2.30 | traslazione |
| `Action.Electrify` | 2.50 | traslazione |
| `Action.Hero.Wraith.Feint` | 2.50 | traslazione |
| `Action.Shield` | 2.50 | traslazione |

Al limite: `Action.Hero.Wraith.Deflection` a 2.60. Da rimisurare, non da assumere.

**Riquadro dell'esagono — 2 glifi**: `Action.Ignite` e `Action.CreateWater` accorciano la corsa di suolo a `x ≤ 16.8`.

**Totale 10.** Per confronto onesto con le proposte in ingresso: la seconda ne dichiarava 28 (perché riservava anche il gutter destro **dentro** la griglia 24), la quarta 43 su 82 (perché deformava la silhouette). Il conto scende a 10 per due scelte precise: il registro sta **fuori** dal box 24 invece che dentro, e la banda bassa non si reclama perché `hero_sigil()` l'ha già presa.

**Asset nuovi**: nessuno per la Fase 1 — binario ed esagono sono geometria dentro i master esistenti, generata da `compose()`. Per la Fase 2: **6 stampi** (4 stazioni piene, più le due varianti a contorno che si usano davvero), tutti minuscoli.

**La trappola che rende il conto 10 e non 0.** `01-principi.md` §3 promette già visual bounds 20×20 e safe margin ~2 px — cioè esattamente la banda che reclamo. Ma il margine è libero **di fatto, non di diritto**: nessuna regola lo controlla. Non aggiungo un vincolo nuovo, **chiedo che quello esistente diventi eseguibile** (T1). Senza il gate nello stesso commit dei 10 delta, la banda è una convenzione, e in questo repository una convenzione è già uscita di scena.

### 8.2 Dati a runtime

**Fase 1 — nessun dato nuovo, nessuna riga di C++.** Fase e superficie sono cotte nel master. `URTActionSlotWidget::GetIconId()` (`RTScreenHudWidgets.cpp:170-180`) resta `MakeIconId(Action.ActionId)`; `FRTIconDef` resta una chiave → una `TSoftObjectPtr<UTexture2D>`; `RequiredIconIds()` non si tocca; il muro `protected` di `URTScreenHudWidgetBase::GetSelectedUnit()` resta intatto; nessuna decisione di privacy. **D-031 è rispettato per costruzione, perché non c'è nessun asset in più da referenziare.**
Il meccanismo è già in produzione: `compose(base, hero_sigil(...))` è usato alle righe 1945 e 2000 per cuocere la marca d'eroe dentro l'asset dell'IconId. `rail()` esiste già (riga 2117) e non è ancora chiamato da nessuno. `action_axes` è già importata (riga 37) e la fase è già scritta nel manifest come campo `"Axes"` (riga 2012).
Lavoro sul generatore: sostituire il passo uniforme di `rail()` con la tabella di sei stazioni + varco, e aggiungere `surface_cell()`.
**Un'estensione di parsing è obbligatoria**: `action_axes.py` **non legge** `bCreatesSurface`/`SurfaceCreated`/`SurfaceRadius`, perché sono assegnati *dopo* la chiamata `ShippedAction(...)` che il parser riconosce (`action_axes.py:27-33`, `SHIPPED_DEFAULTS`). Senza, l'esagono è scritto a mano e deriva in silenzio. È lavoro di parsing su un dato che esiste già.

**Fase 2 — cinque campi in due struct, nessuna query nuova sul gameplay.**
`FillSlotFromAbility` (`RTHudViewModel.cpp:68-78`) e `BuildAbilityCooldowns` (`:139-162`) tengono già in mano un `const URTActionData*` completo e ne copiano due campi su quindici. Serve aggiungere a `FRTPlannedSlotView` / `FRTAbilityCooldownView`:

| Campo | Da dove | Query nuova? |
|---|---|---|
| `TArray<FRTActionEffectSpec> Effects` | `Action->Def.Effects` — `FRTActionEffectSpec` è già `USTRUCT(BlueprintType)` (`RTActionEvent.h:149-166`) | no |
| `bool bPhaseKnown` (sentinella) | vedi sotto | no |

`ERTActionSlot` **è già esposto** (`RTHudViewModel.h:158`). Nessun altro campo serve: forma, slot e superficie di cella non si codificano (§4).

**Tre regole di correttezza, non negoziabili.**

1. **Si legge dall'istanza `URTActionData`, mai dal catalogo statico.** `Variants[].Effects` sostituisce per intero `Def.Effects` quando `ARTUnit::ActiveVariantId` è impostato. Un registro composto dal solo `ActionId` mente su ogni azione con variante — ed è anche il motivo per cui `Action.BasicAttack`, che nel catalogo non dichiara effetti, sul campo ne ha.
2. **La via `ActionId → URTCatalogLibrary::FindCoreAction` è vietata.** Non è `UFUNCTION` e non copre le ability d'eroe: per un `ActionId` d'eroe restituisce un `FRTActionDef` vuoto il cui `ResolutionPhase` **ha come default `ERTResolutionPhase::Attack`** (`RTActionDef.h:306`) — un valore **legittimo**, non un «non calcolato».
3. **Sentinella esplicito.** Quando la fase non è nota, **non si disegna la tacca**. Un binario assente dice «non lo so»; un binario sulla stazione sbagliata dice una bugia che il giocatore non ha modo di rilevare. Il modello è `ERTIntentCertainty::Unknown = 0` (`RTIntentPrivacyLibrary.h:23-38`), che esiste per chiudere esattamente questa classe di difetto. *(Nella Fase 1 il problema non si presenta a runtime — la fase è cotta e derivata dal parser al build — ma il sentinella serve al generatore: una chiave `Action.*` assente da entrambe le tabelle degli assi è T7, non un default.)*

### 8.3 Fuori scope

- Il binario **sugli intenti nemici** (§4.4) — decisione di privacy sotto D-021, non lavoro d'arte.
- La superficie della cella sotto i piedi (§4.3) — è di `SurfaceRingCount`, D-183.
- Forma e slot (§4.1, §4.2).
- Il tema **High Contrast**: è un tema separato, non un CVD più saturo (A6), e per un binario da 2.3 unità l'unica leva è lo **spessore**, non la saturazione. Va progettato, non dedotto.
- La rasterizzazione di un composito per azione: sono 57 assiemi statici e F23/F24 vietano di congelare ciò che dipende dallo stato. La Fase 1 è cotta perché **non** dipende dallo stato; la Fase 2 non si cuoce mai.
- Il conflitto con il notch di `f_slot()` (§5): è §7/§49, appartiene all'HUD.

---

## 9. Le domande che restano a una persona

**D1 — La scommessa dei quattro valori a 24 px.** La specifica afferma che una tacca appoggiata al bordo del varco si distingue, a 24 px, da una che non lo è. Non è misurato.
*Opzioni:* (a) misurarlo sulla calibration sheet prima di implementare, e se cade dichiarare 2 classi a 24 px; (b) implementare con la scommessa scritta nella scheda di consegna e rimandare a E21; (c) allargare il varco da 2.8 a 4.0 unità, che rafforza l'ancora e stringe le stazioni esterne a passo 1.8 — migliora il gradino 24 e peggiora il gradino 32.

**D2 — Il notch di `f_slot()` sopra la stazione `Control`.** Il notch copre il 36–56% della larghezza dello slot; `Control` cade al 34–43% della larghezza dell'icona.
*Opzioni:* (a) misurare in PIE l'inset reale e non toccare niente se non morde; (b) spostare il notch a destra, dentro la zona che il varco lascia vuota; (c) spostare l'intero binario a `y = 2.2` e accettare che i glifi da correggere salgano da 8 a ~12. **Una delle tre, decisa** — non lasciata a caso.

**D3 — La Fase 2 vale il suo prezzo?** Il registro costa ~6 chiavi, un'estensione di `RequiredIconIds()`, un `UImage` in più per slot, e non esiste sotto i 32 px. `progettazione-hud.md` §49 chiede a ogni elemento permanente «il giocatore ha bisogno di vederlo proprio adesso?», e per un modificatore di livello 3 una risposta legittima è no.
*Opzioni:* (a) spedirla nella sola Action Dock e nel tooltip; (b) spedirla solo on-hover; (c) non spedirla in v0.1 e tenere la sola Fase 1, che è autosufficiente. La specifica è costruita perché (c) sia possibile senza rifare niente.

**D4 — Il gutter destro del canvas 28 costa layout.** La Fase 2 richiede che ogni superficie che la mostra conceda il 17% di larghezza in più. `04-regole-di-composizione.md` §3 chiede arte 32–36 px dentro uno slot 56–60 px, e il gutter ci sta; altre superfici no.
*Opzioni:* (a) accettare il gutter e limitare la Fase 2 alle superfici che lo concedono; (b) portare il registro **dentro** il box 24, il che significa reclamare la banda destra e rialzare i glifi da toccare da 10 a ~38 (misurato: 36 glifi hanno inchiostro oltre `x = 21.6`); (c) non spedire la Fase 2.

**D5 — Le due fasi vuote.** `Snapshot` e `Cleanup` non hanno stazione e T5 fa fallire il build se una si popola. È il comportamento voluto, ma va **atteso** da qualcuno.
*Opzioni:* (a) tenere T5 e accettare che popolare quelle fasi sia un ticket d'arte; (b) riservare due stazioni ora, agli estremi assoluti del binario (`x 3.4` e `x 20.6`), spostando le sei attuali verso l'interno — costa risoluzione a tutte per un valore che oggi nessuno usa.

**D6 — La correzione ai documenti owner.** Tre divergenze verificate, che questo lavoro tocca e non risolve da solo:
- `06-accessibilita.md` §6 attribuisce il gate a `scripts/build-icon-assets.py --check`, **che non esiste**. Il gate reale è `check_coverage()` dentro il generatore, più i test C++ (`RTIconCatalogTests.cpp`) e il commandlet.
- `03-forme-e-primitive.md` §1 afferma che `Certainty` non è una categoria di catalogo; `05-certainty-states.md` §4 e il codice (`ERTIconCategory::Certainty`, `RTIconCatalogData.h:41`) dicono l'opposto. Per lo statuto, **vince il codice** e `03 §1` è da correggere.
- Nessun documento owner registra che la **banda bassa è spesa** da `hero_sigil()`. Chi progetterà il prossimo canale la troverà occupata senza sapere perché.
*Opzioni:* (a) correggerli nello stesso commit; (b) aprire issue separate; (c) scriverlo solo qui, che è la scelta che ha già prodotto una volta il caso `docs/src/`.

**D7 — Chi possiede la specifica.** Questo documento sta in `docs/research/`, che **non è canone**. La regola di composizione che descrive è però eseguibile e verrà applicata da un gate.
*Opzioni:* (a) restare in `research/` e accettare che un gate applichi una regola non canonica; (b) promuovere le sole regole eseguibili (§5 esclusioni, §7 test) in `docs/technical/systems/`, lasciando qui la grammatica; (c) aprire un ADR e assegnare un `D-nnn` — da leggere dall'ultimo assegnato nel Decision Log e **riverificare prima del merge**, perché una PR aperta che rivendica lo stesso ID è una collisione.

---

## Limiti di questa specifica

- Verificata per **lettura statica e misura raster** sul branch corrente. Non ho compilato, non ho aperto l'Editor, non ho eseguito il generatore. «Non è `UFUNCTION`» è dedotto dall'assenza del marker, non da un fallimento di link.
- La working directory è **sporca** e il pack è cresciuto da 83 a 123 glifi **durante** la stesura. Tutte le misure di §8.1 vanno rifatte al momento dell'implementazione. Per **D-178**, questo lavoro non può convivere con quello che sta rigenerando i glifi.
- Le tre affermazioni che sono scommesse e non misure sono etichettate come tali: i quattro valori a 24 px (§6, D1), la separabilità Smoke ↔ ShallowWater a 32 px (§7), e l'ipotesi che il varco centrale sia un'ancora percettiva più forte dell'estremo di una guida (§2.2). Le altre affermazioni citano un file e una riga, oppure un numero che ho misurato.
- Non è una dichiarazione di conformità WCAG e non contiene alcun dato raccolto su una persona. La verifica di leggibilità appartiene al playtest **E21**.