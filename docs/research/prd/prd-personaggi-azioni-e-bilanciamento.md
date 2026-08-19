# PRD — Personaggi, azioni, terreni e bilanciamento

> **Non è fonte normativa.** Livello **8** della gerarchia. I **numeri vigenti** della v0.1 vivono nei
> [cataloghi di bilanciamento](../../balance/README.md) — azioni, eroi, equipaggiamento, terreni, matrice di
> test — che sono `CANONICAL` e **superano** questo documento ovunque divergano. Le **decisioni** stanno in
> [`../../product/piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) e negli
> [ADR](../../decisions/).
>
> **Testo estratto dai PDF originari, non riscritto.**

## Da dove viene

| Sorgente (rimosso il 2026-08-12) | Pagine | Cosa contribuisce qui |
|---|---|---|
| `prd-personaggi-e-combattimento-reattivo.pdf` | 10 | Roster su asset Paragon (Steel · Aurora · Murdock · Kwang), abilità, varianti, talenti, reazioni, sistema di **Finestra di Reazione** |
| `idee-ruoli-characters.pdf` | 17 | ≥12 classi, ≥30 azioni/interazioni, ≥20 terreni e oggetti mappa, pattern di design, esempi da giochi esistenti, combo per il vertical slice |
| `catalogo-e-bilanciamento-v0.1.pdf` | 26 | Catalogo completo: assunzioni, fasi, ~35 azioni con fase/priorità/range/cooldown, terreni, coperture, equipaggiamento, eroi, matrice di test |

## Il roster canonico nasce qui

`docs/src/README.md` classificava `idee-ruoli-characters.pdf` come **non recepito** («Recepito da: —»). È
falso, ed è la scoperta più utile di questa consolidazione: **Gadget · Phase · Riktor · Wraith** compaiono in
questo documento con ruolo, meccanica fondamentale, quattro abilità, combo e trade-off — e le quattro abilità
di Gadget sono, nome per nome, quelle del catalogo eroi di oggi:

| `idee-ruoli-characters.pdf` | [`RT_HeroCatalog_v0.1.md`](../../balance/RT_HeroCatalog_v0.1.md) |
|---|---|
| *Scarica Lineare* — attacco in linea, devia se attraversa acqua | `Hero.Gadget.LinearDischarge` — 24 danni, **+8 su bersaglio `Wet`** |
| *Nodo Conduttore* — dispositivo che conduce elettricità fra posizioni | `Hero.Gadget.ConductiveNode` — è `Action.Electrify`, propagazione 3 |
| *Sovraccarico* — AoE sulle celle cariche | `Hero.Gadget.Overload` — 18 danni, `Interrupt` sui dispositivi |
| *Capacitore Reattivo* — counter: assorbe danno e carica chi attacca | `Hero.Gadget.ReactiveCapacitor` — scudo 15 e 10 danni all'attaccante |

Anche il *Pannello Cinetico* di Riktor nasce qui e riappare in PDR-12. Il documento non era «da consumare»:
era **già consumato e non registrato**.

## Cosa resta vero, cosa no

**Recepito nel canone.** L'intera struttura del catalogo di bilanciamento: ID azione stabili, priorità intera
intra-fase, fallback dichiarati, budget **5 MP**, slot Reazione, otto terreni, coperture direzionali
([ADR-0003](../../decisions/adr-0003-modello-azioni-v01.md)). Le regole `Push`/`Pull`, gli stati `Wet` ·
`Burning` · `Electrified` · `Obscured` · `Rooted` · `Marked`, la conduttività dell'acqua.

**Superato.** Il roster **Steel · Aurora · Murdock · Kwang** su asset Paragon: mai adottato — resta come
esplorazione, e l'idea di riusare gli asset Paragon come base tecnica sopravvive solo in
`Content/FabAsset/Paragon/`. La **finestra di reazione di 8 secondi** e la variante a 5-7 s: la baseline è
**3,0 s** con timeout `HOLD` ([ADR-0004](../../decisions/adr-0004-finestre-di-reazione.md) §8). Le **fasi**
del catalogo (`Snapshot → Preparazione → Movimento → Controllo → Attacco → Ambiente → Cleanup`, con il
movimento **prima** dell'attacco): le macro-fasi restano quelle di Atlas Reactor col **Move dopo il Blast** —
le sette divergenze sono elencate una per una in
[`RT_ActionCatalog_v0.1.md` §8](../../balance/RT_ActionCatalog_v0.1.md). **UE 5.6.x**: la versione bloccata è
**5.8.1**. Il **GAS** come motore delle abilità: fuori dalla v0.1.

**Recuperabile, e non ancora recepito da nessuno.**

- 🟢 **Le ≥30 azioni/interazioni e i ≥20 terreni/oggetti mappa** di `idee-ruoli-characters`, con regole di
  targeting (`cell-locked`, `path-intercept`) e priorità di risoluzione. Il catalogo azioni corrente ne ha
  ~35: il resto — correnti, gravità alterata, barili esplosivi, interruttori, trappole — è materiale per E8
  (terreni) e E9 (coperture e strutture).
- **Le combo di squadra pronte da provare** e i dieci stati di terreno con interazioni incrociate. Le sinergie
  del canone sono *esempi che citano*, non fonti di numeri ([D-029](../../decisions/RT_PDR_00_Decision_Log.md)):
  questi restano ipotesi di playtest, non regole.
- **I talenti e le varianti equipaggiabili** di `prd-personaggi-e-combattimento-reattivo`: tre varianti + tre
  talenti per eroe, ciascuno con un trade-off dichiarato. Il catalogo equipaggiamento copre le varianti
  d'arma, i gadget e i moduli di reazione, **non** i talenti.
- **Le tabelle comparative** (§6: abilità definitive, varianti, reazioni a confronto). Sono lo scheletro di
  una revisione di bilanciamento trasversale, che oggi si fa eroe per eroe.

## Perché il testo del catalogo è qui, e non solo nei cataloghi `balance/`

I cinque cataloghi di `docs/balance/` **derivano** da questo PDF, non lo trascrivono: ne cambiano le fasi, ne
ricostruiscono i cooldown mancanti e lo dichiarano nelle *Divergenze rispetto al PDF*. Quella sezione cita
«il PDF» sette volte: senza il testo originale in una forma leggibile, sette righe di motivazione sarebbero
diventate impossibili da verificare. Ora il rimando ha un bersaglio.

> ⚠️ Le tabelle del catalogo originale sono **impaginate su colonne sfalsate**, e l'estrazione lo eredita: nella
> tabella «Azioni fondamentali» i valori di *Fase*, *Priorità*, *Range* e *Cooldown* scivolano di una colonna
> su alcune righe. È lo stesso difetto che
> [`RT_ActionCatalog_v0.1.md` §8](../../balance/RT_ActionCatalog_v0.1.md) riga 6 dichiara di aver risolto
> leggendo le descrizioni a parole. **Per i numeri usa il catalogo, mai questa tabella.**

---

## Personaggi e combattimento reattivo — roster Paragon e finestra di reazione

### Sommario esecutivo
Questo PRD delinea il concept di _RefactorTactics_ , un titolo tattico a turni basato su asset AAA di Paragon rilasciati gratuitamente da Epic Games【6†L4-L8】. La versione iniziale (vertical slice 2v2) prevede quattro personaggi ispirati a Paragon – Steel, Aurora, Murdock e Kwang – riprogettati con ruoli, lore e abilità originali. Il documento specifica il roster v0.1, le meccaniche distintive di ciascun eroe, le abilità definitive, varianti equipaggiabili, talenti e debolezze. Introduce inoltre il sistema di **Finestra di Reazione** , una meccanica di gameplay chiave per reazioni tattiche in tempo rallentato, con regole tecniche (trigger, durata, priorità, risoluzione) e considerazioni UI/UX/server. Vengono forniti esempi di flussi di gioco in match 2v2, casi d’uso e test match consigliati, oltre a una roadmap per il vertical slice. Il PRD include tabelle comparative di abilità, varianti e reazioni, nonché diagrammi visivi (diagramma di flusso e timeline in Mermaid). Tutte le assunzioni (UE5 C++, server-authoritative, 1 slot reazioni per personaggio, finestra default 8s) sono dichiarate esplicitamente.

### 1. Contesto e assunzioni
- **Asset Paragon:** Epic Games ha rilasciato $17M di contenuti Paragon (MOBA) gratuiti per UE4, comprensivi di **39 eroi AAA** e migliaia di asset ambientali【6†L4-L8】. Usiamo alcuni di questi modelli come base tecnica e visiva. Tuttavia, per rispettare i termini licenza, i nomi “Paragon” e degli eroi (es. Murdock, Kwang) saranno solo identificatori interni; nel gioco avranno nomi e lore propri.

- **Motore e architettura:** Unreal Engine 5, C++, server-authoritative. Partita a turni 2v2 (vertical slice). Risorsa personale: Steel (“Integrità scudo”), Aurora (“Cariche termiche”), Murdock (“Munizioni speciali”), Kwang (“Carica della tempesta”).

- **Finestra di Reazione:** Ogni personaggio ha 1 slot di reazione equipaggiabile (3 scelte a disponibilità variabile). Durante la risoluzione, eventi chiave (attacco, movimento, ecc.) possono aprire una finestra di reazione rallentata (default 8s) in cui il giocatore può attivare una reazione pre-equipaggiata【6†L4-L8】. Le regole di interrupt sono server-authoritative e loggate per il replay.

- **Ruoli e obiettivi di design:** Evitare troppi personaggi “damage dealer”. Ogni eroe manipola elementi distinti del campo di battaglia (spazio, terreno, linee di fuoco, ancore elettriche) e ha un gameplay unico. L’attenzione è su interazioni tattiche piuttosto che numeri puri di danno.

### 2. Roster v0.1
La versione iniziale include **4 personaggi** (in futuro potranno aggiungersene altri dal pool Paragon). Ciascun asset è riutilizzato con meccaniche originali. Di seguito l’elenco e i loro ruoli chiave:

|Personaggio|Ruolo primario|Ruolo<br>secondario|Meccanica distintiva|
|---|---|---|---|
|**Steel**|Vanguard|Controller|Scudo direzionale e interposizione|
|**Aurora**|Terrain<br>Controller|Skirmisher|Creazione/trasformazione del ghiaccio|

|Personaggio|Ruolo primario|Ruolo<br>secondario|Meccanica distintiva|
|---|---|---|---|
|**Murdock**|Marksman|Suppressor|Sorveglianza linee di tiro (Overwatch)|
|**Kwang**|Duelist|Electro<br>Controller|Spada-ancora che guida l’uso<br>dell’elettricità|

### 3. Personaggi
Ogni personaggio segue questo schema: ruolo primario/secondario, meccanica chiave, 4 abilità base, 3 varianti equipaggiabili, 3 talenti, debolezza, statistiche di base (provvisorie), 3 reazioni.

#### 3.1 Steel –** **_Bastione cinetico_
- **Ruolo:** Vanguard (difensore frontale) / Controller.

- **Meccanica distintiva:** mantiene uno scudo orientato (fronte protetto, fianchi vulnerabili). Può intercettare attacchi lineari mirati a un alleato (interposizione).

- **Abilità definitive:**

- **Impatto cinetico:** attacco melee frontale con spinta di 1 cella; se il bersaglio collide infligge danno aggiuntivo. Può rompere coperture leggere. (Fallback: colpisce la cella prevista.)

- **Avanzata protetta:** Steel avanza fino a 3 celle con scudo sollevato; subisce danni frontali ridotti durante il movimento e può spingere un nemico incontrato, ma resta vulnerabile lateralmente.

- **Interposizione:** Steel tenta di posizionarsi tra un alleato e un attacco lineare in arrivo. Se ci riesce, intercetta il colpo con lo scudo, proteggendo l’alleato. (Fallimento se Steel è bloccato o arriva tardi.)

- **Muro antisommossa:** pianta lo scudo come copertura fissa direzionale per 2 turni. Il muro blocca movimenti e alcuni proiettili, ma Steel perde parte della difesa personale finché è piantato.

- **Varianti equipaggiabili (3):**

- _Scudo riflettente:_ il primo proiettile frontale viene deviato altrove (meno resistenza complessiva).

- _Scudo d’assedio:_ bonus alle distruzioni di coperture leggeri, ma Steel si muove 1 cella in meno.

- _Scudo espanso:_ amplia lo scudo (copre anche un alleato adiacente), ma riduce la velocità di rotazione.

##### Talenti (3):

_Inamovibile:_ +resistenza alle spinte, –velocità di movimento.

- _Primo della linea:_ bonus difensivo se Steel è alleato più vicino ai nemici.

- _Risposta proporzionata:_ dopo una parata Steel contrattacca automaticamente (danno minore) ma subisce penale difensiva contro attacchi multipli consecutivi.

- **Debolezza:** prevedibile nell’avanzamento, facile da aggirare, poco efficace in AoE ed aereo. Dipende molto dall’orientamento dello scudo.

- **Statistiche iniziali (esempio):** Alta salute, armatura pesante, bassa mobilità laterale.

- **Reazioni equipaggiabili:**

| Reazione | Trigger | Effetto |

|---------------------|--------------------------------|-----------------------------------------------------------------| |

**Interposizione** | Attacco lineare su un alleato | Steel si sposta in percorso libero tra attaccante e bersaglio per assorbire il colpo. | | **Impatto d’arresto** | Nemico attraversa arco adiacente |

Steel interrompe il nemico e lo spinge indietro di 1 cella. Steel perde l’azione difensiva successiva. | | **Scudo d’emergenza** | Steel o alleato adiacente viene attaccato | Steel gira istantaneamente lo scudo verso l’origine dell’attacco, riducendo drasticamente il danno frontale (espone il lato opposto). |

#### 3.2 Aurora –** **_Architetta glaciale_
- **Ruolo:** Terrain Controller / Skirmisher (trapper).

- **Meccanica distintiva:** manipola lo stato termico di acqua e superfici. Può creare/coprirle di ghiaccio o farle tornare acqua. Il ghiaccio influisce su movimento (scivolate, coperture temporanee, barriere) e può interagire con fuoco/elettricità.

##### Abilità definitive:
##### •

- **Frammento glaciale:** attacco a media distanza in linea retta. Danno moderato e applica _Freddo_ . Se il bersaglio è già _Freddo_ , infligge _Congelamento parziale_ . Congela anche piccole pozze d’acqua sul bersaglio o nel percorso.

- **Passo cristallino:** scatto rapido verso una cella (fino a 4). Lascia dietro di sé una scia di ghiaccio per 2 turni: riduce costo movimento per chi la attraversa ma aumenta il rischio di scivolamento (alleati e nemici). Congela automaticamente l’acqua attraversata. (Trade-off: la pista può essere usata anche dai nemici.)

- **Parete glaciale:** crea una parete di ghiaccio (copertura alta) su 1–3 archi adiacenti entro 5 celle. Blocca movimenti e linee di tiro, dura 2 turni o fino a distruzione. Il fuoco la scioglie (genera acqua). Non si può piazzare in unità o su superfici invalide.

- **Zero termico:** AoE circolare (raggio 2) centrato su Aurora. Infligge danno leggero da freddo e: applica _Freddo_ a tutti i nemici, congela acqua e stabili strutture ghiacciate vicine, rallenta temporaneamente fiamme. (Danno diretto basso, effetto utilità alto.)

##### Varianti equipaggiabili (3):
##### •

- _Ghiaccio fragile:_ le strutture di ghiaccio esplodono in schegge alla distruzione. Resistenza ridotta.

- _Ghiaccio compatto:_ pareti e ponti di ghiaccio durano più a lungo, ma Passo cristallino genera meno ghiaccio.

- _Brina conduttiva:_ il ghiaccio conduce l’elettricità (elettrificabile), ma Aurora subisce +25% danni elettrici.

##### Talenti (3):

_Pattinatrice:_ +2 velocità di movimento su terreno ghiacciato, –1 fuori dal ghiaccio.

- _Architetta:_ può generare una struttura ghiaccio addizionale con Parete, ma danno di base ridotto del 20%.

- _Sottozero:_ bonus di controllo (rallentamento, stordimento) sui nemici _Bagnati_ , efficacia ridotta (+50% req. di danno) su nemici _Infiammati_ .

- **Debolezza:** bassa pressione offensiva immediata; le sue strutture possono favorire sia squadre; il fuoco nemico annulla le sue setup rapidamente; richiede pianificazione e sinergia del team.

- **Statistiche iniziali (esempio):** Mobilità elevata, difesa media, poca resistenza diretta.

- **Reazioni equipaggiabili:**

| Reazione | Trigger | Effetto |

|---------------------|----------------------------------------|----------------------------------------------------------------------| | **Parete istantanea** | Attacco lineare attraversa un arco (visibile) | Aurora crea immediatamente una parete di ghiaccio fragile sull’arco bersaglio, tentando di bloccare o deviare il colpo. (La parete può frantumarsi con l’impatto.) | | **Congelamento riflesso** | Aurora o un alleato spostato su cella _bagnata_ | Congela istantaneamente quella cella appena entrata, annullando lo slittamento o modificando (riducendo) lo spostamento. (L’unità può comunque scivolare dopo.) |

| **Estinzione rapida** | Una cella vicina prende fuoco | Congela quella cella, spegne il fuoco e genera vapore istantaneamente (riduce visibilità nei dintorni). |

#### 3.3 Murdock –** **_Tiratore di interdizione_
- **Ruolo:** Marksman / Suppressor.

• **Meccanica distintiva:** domina il campo tramite linee di tiro a lunga distanza. Può preparare colpi ed Overwatch su corridoi, mirando a controllare le strade nemiche. Deve gestire precisione, coperture e munizioni.

##### Abilità definitive:
##### •

• **Colpo calibrato:** attacco a lungo raggio in linea retta. Danno alto a distanza media, cresce finché non passa max range; peggiore su bersagli adiacenti. Coperture leggere dimezzano danno, muri grandi lo bloccano. (Fallback: può scegliere di colpire la cella target originaria o il primo nemico incontrato.)

• **Overwatch:** Murdock attiva il sistema di sorveglianza su un arco o cono (portata ~5). Il primo nemico che entra in quest’area (o attraversa l’arco) subisce un colpo automatico di Murdock, con danno medio. Se nessuno entra, Overwatch finisce senza effetto (selezione sprecata).

- **Munizione perforante:** potenzia il prossimo Colpo calibrato. Il proiettile attraversa 1 copertura leggera e 1 unità (o elementi fragili) dimezzando danno a ogni passaggio. Blocca muri pesanti e Steel difeso. Consuma munizioni extra, cooldown lungo.

- **Fuoco di soppressione:** raffica a cono rivolta avanti (raggio 3). Danno basso su più target, applica _Soppressione_ (riduce mobilità e impedisce alcune reazioni per 1 turno). Distrugge parzialmente coperture leggere o fragili in area. (Usata per interrompere posizioni nemiche.)

##### Varianti equipaggiabili (3):
##### •

- _Ottica predittiva:_ migliora leggermente la mira su bersaglio in movimento, ma riduce il danno massimo del Colpo calibrato del 15%.

- _Munizioni pesanti:_ +50% distruzione coperture con Colpo calibrato, ma costa doppie munizioni (limitato).

- _Sistema di prossimità:_ Overwatch si innesca solo a corto raggio (<3 celle) con danno aumentato, ma guadagno precisione sulle lunghe diminuito.

- **Talenti (3):**

_Posizione dominante:_ +30% danno da quota sopraelevata, –20% in tunnel/stretto.

- _Disciplina di fuoco:_ se Overwatch non si attiva, Murdock recupera parte della spesa (risorsa) al turno successivo.

- _Bersaglio prioritario:_ danno aggiuntivo contro bersagli marcati (da Kwang, Phase, ecc.), ma danno ridotto contro bersagli non marcati.

• **Debolezza:** quasi impotente in corpo a corpo, dipendente da linee di tiro pulite, vulnerabile a fumo e chiusure di angoli. Esige coperture proprie e sinergia per evitare di essere aggirato.

- **Statistiche iniziali (esempio):** Danni a distanza molto alti, risorsa limitata, mobilità bassa, difese leggere.

##### Reazioni equipaggiabili:

| Reazione | Trigger | Effetto |

|---------------------|--------------------------------------------|---------------------------------------------------------------------|

| **Tiro d’opportunità** | Nemico entra o attraversa un arco sorvegliato da Overwatch | Murdock può immediatamente sparare al primo nemico agganciato, consumando la reazione. (Se rinvia, mantiene la reazione per un altro trigger.) | | **Correzione balistica** | Bersaglio originale del Colpo calibrato lascia la cella prevista | Murdock può modificare leggermente l’angolo di tiro per colpire il bersaglio mobile, altrimenti sparare alla cella originaria. Se sceglie di sparare

nuovamente al bersaglio, paga parte del cooldown. | | **Fuoco di arresto** | Nemico inizia un dash in vista | Murdock spara una raffica veloce (danno basso); se colpisce, riduce di 1 cella la lunghezza del dash dell’avversario. |

#### 3.4 Kwang –** **_Conduttore della tempesta_
- **Ruolo:** Duelist / Electro Controller.

- **Meccanica distintiva:** combatte attorno alla **Spada-ancora** . Kwang può piantare (lanciare) la sua spada in una cella: essa diventa un nodo tattico permanente che elettrifica l’acqua circostante e consente abilità di richiamo/teletrasporto. Finché la spada è piantata, Kwang può teletrasportarsi verso di essa o richiamarla. Tutti i suoi attacchi base sono potenziati se azionati vicino alla spada. La posizione dell’ancora è visibile a tutti (aspetto di gioco pubblico).

##### Abilità definitive:

- **Taglio ad arco:** attacco frontale melee con ampiezza ad arco. Danno più alto al centro. Può deviare colpi melee leggeri in arrivo. Senza spada piantata, diventa un colpo energetico dritto con 50% danno.

- **Lancio dell’ancora (Oscillazione):** Kwang scaglia la spada verso una cella entro 5. All’impatto, la spada infligge danno leggero e si pianta nel terreno (creando il “nodo conduttivo”). Eventuali dispositivi o ponti metallici sotto la cella subiscono un breve corto circuito. La spada rimane fino a richiamata o rubata.

- **Passo del fulmine:** teletrasporto rapido verso/dalla spada (massimo 6 celle in linea). Kwang sceglie di scattare “ **verso** la spada” o “ **attraverso** la spada” (ulteriore salto doppio) consumando la sua azione. Colpisce tutti i nemici attraversati (danno medio da colpo spada). (Trade-off: percorso prevedibile e a linea retta.)

- **Tempesta incatenata:** Kwang scatena una scarica elettrica centrata sulla spada (raggio 2). L’elettricità salta automaticamente da cella a cella, preferendo unità vicine e celle bagnate/ metalliche, riducendo danno a ogni salto. Se coinvolge un alleato, scambia parzialmente lo scudo o cura un po’. (Attenzione a non colpire gli alleati, o talenti dedicati evitano questo.)

##### Varianti equipaggiabili (3):
##### •

- _Parafulmine:_ amplia il raggio di Tempesta incatenata (+1), ma aumenta il cooldown di Lancio (la spada non può essere richiamata subito).

- _Lama del duellante:_ potenzia il Taglio ad arco (extra 20% danno vicino alla spada), ma Riduce i salti di Tempesta incatenata (-1 salto).

- _Catena controllata:_ Tempesta incatenata automaticamente evita gli alleati, ma perde 1 salto di danno.

##### Talenti (3):
##### •

- _Occhio del temporale:_ quando Kwang è vicino alla spada (<=2 celle), +30% resistenza ai danni elettrici; quando lontano, +20% vulnerabilità elettrica.

- _Richiamo violento:_ il richiamo della spada infligge danno pari al 10% della salute massima attraversata. Cooldown della spada +1 turno.

- _Giuramento del duello:_ seleziona un singolo bersaglio alla piantata della spada; +50% danno su quel bersaglio, –20% su altri bersagli.

- **Debolezza:** dipende fortemente dal posizionamento della spada; mancare la pianta lascia Kwang con opzioni limitate; le sue abilità elettriche possono danneggiare anche alleati se non pianificate. Contro nemici che controllano aree elevate, la spada può essere facilmente minata.

- **Statistiche iniziali (esempio):** mobilità media, danno melee elevato, resistenza elettrica naturale alta, salute media.

##### • **Reazioni equipaggiabili:**

| Reazione | Trigger | Effetto |

|------------------------|--------------------------------------|-----------------------------------------------------------------| |

**Richiamo fulmineo** | Nemico entra nella linea tra Kwang e spada | Kwang richiama immediatamente la spada, colpendo con essa lungo la traiettoria intermedia (danno medio). Può scegliere di usare o risparmiare la reazione. | | **Passo verso l’ancora** | Kwang viene bersagliato da un attacco | Kwang può immediatamente scattare verso la spada (fino a 4 celle) prima dell’impatto, evitando così l’attacco originale. (Il percorso è prestabilito in linea retta.) | | **Scarica di risposta** | Kwang o la spada subiscono danno elettrico | Rilascia una scarica breve verso tutte le celle _bagnate_ o metalliche vicine (raggio 2) o unità in mischia attorno. Infligge danno basso a tutti i bersagli (inclusi alleati, scudo ridotto). |

### 4. Sistema di Finestra di Reazione
La **Finestra di Reazione** è una meccanica chiave di _RefactorTactics_ che permette risposte tattiche extra durante la risoluzione. Ecco le specifiche tecniche e di gameplay:

- **Trigger di reazione:** Durante la risoluzione di un turno, **eventi di gioco chiave** (es. attacco dichiarato, inizio dash, personaggio spinto, ingresso in zona controllata) possono generare una finestra di reazione. Ogni reazione equipaggiata ha condizioni di trigger specifiche (ad es. “un alleato viene attaccato” per Interposizione, “un nemico entra in Overwatch” per Tiro d’opportunità, ecc.).

- **Finestra di tempo:** Quando un trigger scatta, il gioco entra in slow-motion e si apre una finestra di risposta di default **8 secondi** (configurabile a 5–10s). Il timer è visibile ai giocatori coinvolti. Durante la finestra, la simulazione **logica si congela** sull’evento, mentre la presentazione animata è rallentata. Il giocatore può decidere rapidamente se attivare la reazione o passare. Le azioni dei giocatori non entrano nella simulazione finché la finestra non si chiude.

- **UI/UX:** Durante una finestra, l’interfaccia mostra: countdown (“Reazione disponibile – X.X s”), la descrizione della reazione equipaggiata (nome, effetto, costi) e pulsanti [USA] o [PASSA]. Solo le reazioni attivate (pre-selezionate in equip) compaiono come opzioni. Gli alleati vedono gli alert di altri compagni (chi sta scegliendo, cosa ha selezionato), ma non vedono i piani nemici. Lo slowmotion e la pausa di risoluzione enfatizzano la scelta tattica.

- **Priorità e risoluzione:** Quando più reazioni possibili sono valenti simultaneamente, _non_ vince chi preme prima. Il server raccoglie le scelte di tutti entro il timeout, poi applica le reazioni in ordine predeterminato:

- Immunità / stasi (es. Cristallo di stasi)

- Schivate e spostamenti difensivi (es. Passo del fulmine di Kwang)

- Interposizioni e protezioni attive (es. Scudo d’emergenza di Steel)

- Interruzioni di attacchi (es. Estinzione rapida di Aurora)

- Contrattacchi e reazioni offensive (es. Tiro d’opportunità di Murdock)

- Manipolazioni ambientali (es. Parete istantanea)

- Azione originale rimasta (se non risolta)

Nel caso di pari priorità, si usa iniziativa e ID stabile per l’ordine deterministico. Non si tiene conto dell’ordine di arrivo del comando di rete.

• **Limiti di gioco:** Per evitare chain infinite eccessive, ogni personaggio può equipaggiare **1 reazione** (rinnovo all’inizio del turno), e usarla **1 volta per turno** . Al massimo **2 finestre globali** possono aprirsi simultaneamente in una singola risoluzione. Le reazioni **non attivano** altre reazioni (no catene), e non si possono usare reazioni su reazioni (no interrupt di reazione). Dopo un timeout, la scelta predefinita è [PASSA].

##### • **Flusso server-authoritative (di massima):**

`1. Il Resolver elabora un evento (es. attacco dichiarato).`

```
2. Il sistema Reazioni verifica se il trigger corrisponde a una reazione
equipaggiata e disponibile.
```

```
   - Se NO, la simulazione continua normalmente.
   - Se SÌ, la simulazione logica si congela.
```

`3. Il server apre una *ReactionWindow* e invia l’opzione di reazione ai client coinvolti (privatamente).`

```
4. I giocatori hanno X secondi per scegliere [USA] o [PASSA].
```

```
5. Alla scadenza, il server raccoglie scelte e le valida sullo stato
congelato.
```

```
6. Le reazioni confermate vengono ordinate secondo la priorità
descritta.
```

```
7. Si applicano gli effetti di tutte le reazioni (aggiornando il log di
turno).
```

`8. La simulazione riprende dal punto in sospeso.`

Tutti gli eventi (trigger, scelta, esito) sono loggati in ordine per supportare il replay deterministico (il replay rilegge il _TurnLog_ e applica scelte senza ritardi). La UI può riprodurre le finestre velocemente nel replay.

- **Configurazione iniziale per prototipo:** 1 reazione/personaggio, 1 uso per turno, 8s finestra, massimo 2 finestre contemporanee, nessuna reazione automatica (scelte private simultanee), slow-motion durante la finestra. In seguito, talenti o abilità speciali potrebbero aggiungere meccaniche di _anti-reazione_ o reazioni extra.

### 5. Flussi di gioco e casi d’uso 2v2
Di seguito alcuni scenari tipici e coppie 2v2 consigliate per il testing, che mostrano sinergie e antagonismi tra i personaggi:

#### 5.1 Esempi di flusso giocatore
• **Scenario “Corridoio controllato” (Steel + Murdock vs Aurora + Kwang):** Steel blocca un passaggio chiave con lo scudo (muro antisommossa), costringendo gli avversari ad entrare in un corridoio. Murdock attiva Overwatch sul corridoio. Aurora cerca di aggirare avvelenando l’acqua vicina, e Kwang pianta la spada in posizione strategica dietro una copertura. Se un nemico attraversa il corridoio attivando Overwatch, Steel può usare _Interposizione_ di reazione per intercettare il colpo al posto di un alleato. Questa sequenza illustra controllo dello spazio (Steel), preparazione della linea di tiro (Murdock) e interazioni ambientali (Aurora/Kwang). La combinazione di scudi e overwatch crea una forte zona di negazione. Contro di essa, Aurora+Kwang potrebbero distruggere coperture (Ignition di Iggy o Tempesta di Kwang) o usare fumo/vapore per coprire i movimenti.

• **Scenario “Lago conduttivo” (Aurora + Kwang):** Aurora rallenta l’area con una pozza d’acqua e la congela, piantando Kwang la spada in essa. Ora le superfici sono conduttive. Kwang può allora attivare _Tempesta incatenata_ per propagare elettricità tra più nemici (maggiore con acqua liquida) o far esplodere le coperture gelate con un colpo, creando interazioni “acqua→elettricità” o “ghiaccio→fuoco”. Ad esempio, se Aurora lascia una pozza viva, Kwang la elettrifica; se congela

tutto, costringe i nemici a scivolare o blocca una porta. Di converso, se uno nemico infuoca il ghiaccio di Aurora, esplode in vapore, che Kwang può usare come schermatura visiva. Questo mostra **sinergia ambientale** : acqua/elettricità/ghiaccio/vapore.

• **Scenario “Fortezza temporanea” (Steel + Aurora):** Steel pianta lo scudo come barriera fissa. Aurora crea intorno a esso una parete di ghiaccio o ponte ghiacciato, formando una minicopertura continua. Questo corridoio protetto permette manovre difensive e riposi sicuri. In seguito, Aurora può decidere di sciogliere il ghiaccio (riaprire un passaggio) o lasciare che sia distrutto (costringendo gli avversari a variare percorso). Attenzione però: il team può intrappolarsi se la copertura rimane chiusa dietro di loro.

- **Scenario “Bersaglio ancorato” (Murdock + Kwang):** Kwang pianta la spada in una posizione strategica, marcando una “zona proibita” per l’avversario. Murdock si posiziona in quota puntando su quell’area con Overwatch. Se un nemico cerca di fuggire da un combattimento (ad es. un duello con Steel), Kwang può inseguirlo o spingerlo verso la spada (richiamo, scatto). Intanto Murdock può punire chiunque entri nell’area con fuoco preciso. Questa combo costringe il bersaglio a scegliere: affrontare Kwang da vicino o correre dritto in una linea di tiro fatale.

#### 5.2 Casi d’uso – Match 2v2 consigliati
- **Match A:** _Team 1: Steel + Murdock_ vs _Team 2: Aurora + Kwang_ . Partita di test completa per coprire: controllo spaziale vs controllo mappa, linee di tiro contro mobilità, coperture fisse vs interazioni di terreno (acqua/ghiaccio). La squadra 1 punta su posizione difensiva e overwatch; squadra 2 sfrutta sinergie ambientali e flanking.

- **Match B:** _Team 1: Steel + Aurora_ vs _Team 2: Murdock + Kwang_ . Testa le difese statiche (scudo + muro ghiaccio) contro potenza di fuoco diretta e mobilità. Verifica se Steel protegge troppo bene Aurora, o se Murdock/Kwang riescono a sfondare con distruzione coperture ed elusione.

Altri match possibili (per fasi avanzate): combina i personaggi in modi diversi (es. _Aurora + Murdock_ vs _Steel + Kwang_ ) per esplorare ulteriori sinergie (overwatch + controllo terreno vs scudo + elettricità, ecc.).

### 6. Tabelle comparative
#### 6.1 Confronto abilità definitive
|Personaggio|Abilità 1<br>(Attacco base)|Abilità 2<br>(Movimento)|Abilità 3 (Controllo)|Abilità 4 (AoE/<br>Supporto)|
|---|---|---|---|---|
|**Steel**|_Impatto_<br>_cinetico_(melee<br>push)|_Avanzata protetta_<br>(scudo + spinta)|_Interposizione_(difesa<br>alleato)|_Muro_<br>_antisommossa_<br>(parete fissa)|
|**Aurora**|_Frammento_<br>_glaciale_<br>(lineare)|_Passo cristallino_<br>(dash + scia<br>ghiaccio)|_Parete glaciale_(cover<br>ghiaccio)|_Zero termico_(AoE<br>rallentante)|
|**Murdock**|_Colpo calibrato_<br>(sniper)|_Overwatch_(tira su<br>linea)|_Munizione perforante_<br>(potenziamento)|_Fuoco di_<br>_soppressione_<br>(raffica AoE)|
|**Kwang**|_Taglio ad arco_<br>(melee area)|_Lancio dell’ancora_<br>(pianta la spada)|_Passo del fulmine_<br>(teletrasporto)|_Tempesta_<br>_incatenata_(AoE<br>elettrico)|

_(Ogni cella riassume la funzione principale di quell’abilità.)_

#### 6.2 Confronto varianti equipaggiabili
|Personaggio|Variante|Vantaggio principale|Svantaggio principale|
|---|---|---|---|
|**Steel**|_Scudo riflettente_|devia primo proiettile|minore resistenza<br>complessiva|
||_Scudo d’assedio_|distrugge coperture di più|-1 cella movimento|
||_Scudo espanso_|copre anche alleato adiacente|penalizza rotazione|
|**Aurora**|_Ghiaccio fragile_|esplosione di frammenti alla<br>rottura|strutture più deboli|
||_Ghiaccio compatto_|dura più a lungo|less ghiaccio generato dal<br>dash|
||_Brina conduttiva_|ghiaccio conduce l’elettricità|+ danno elettrico subito|
|**Murdock**|_Ottica predittiva_|aumenta affidabilità vs bersagli<br>mobili|-15% danno max|
||_Munizioni pesanti_|+distruzione coperture|+costo munizioni &<br>cooldown|
||_Sistema di_<br>_prossimità_|Overwatch + in mischia|precisione a lungo ridotta|
|**Kwang**|_Parafulmine_|Tempesta elettrica più ampia|spada con cooldown più<br>alto|
||_Lama duellante_|+danno vicino alla spada|-1 salto elettrico|
||_Catena controllata_|protezione alleati|-1 salto elettrico dannoso|

#### 6.3 Confronto reazioni equipaggiabili
|Personaggio|Reazione|Trigger|Tipo|
|---|---|---|---|
|**Steel**|Interposizione|attacco lineare su alleato vicino|Difensiva|
||Impatto d’arresto|nemico attraversa arco adiacente|Interruzione|
||Scudo d’emergenza|attacco imminente a Steel/alleato|Difensiva|
|**Aurora**|Parete istantanea|colpo lineare in arrivo|Ambiente/Stop|
||Congelamento<br>riflesso|alleato/Kwang su cella_bagnata_|Ambiente|
||Estinzione rapida|cella vicina prende fuoco|Ambiente/Stop|
|**Murdock**|Tiro d’opportunità|nemico entra in Overwatch|Contrattacco|

|Personaggio|Reazione|Trigger|Tipo|
|---|---|---|---|
||Correzione balistica|bersaglio si muove fuori mira<br>prevista|Supporto|
||Fuoco di arresto|nemico inizia un dash|Contrattacco|
|**Kwang**|Richiamo fulmineo|nemico entra tra Kwang e spada|Contrattacco|
||Passo all’ancora|Kwang viene attaccato|Riposizionamento|
||Scarica di risposta|Kwang/spada subiscono danno<br>elettrico|Contrattacco|

_(Ogni reazione è abbinata al suo evento trigger e al tipo principale di effetto.)_

### 7. Roadmap e milestone (Vertical Slice)
Di seguito una pianificazione di massima per il vertica slice (esempi di milestone). Tutte le date sono indicative e dipendono da risorse e priorità.

<!-- Start of picture text -->
Roadmap Vertical Slice (2v2)<br>2026-09-01 2026-10-15 2026-11-01 2026-12-01 2027-01-01 2027-02-01 2027-03-01<br>**Fase Design** – **Prototipo **Implementazione **Abilità **Varianti & **Testing 2v2** – **Vertical Slice<br>PRD finale, concept Iniziale** – Reazioni** – Personaggi** – Talenti** – Playtest scenario Completato** –<br>art, definizione Implementazione Sistema Finestra di Sviluppo abilità Aggiunta varianti (es. Steel+Murdock Rilascio demo<br>meccaniche movimento griglia Reazione integrato, definitivo di Steel, equipaggiabili e vs Aurora+Kwang), interna,<br>e controllo base UI base Aurora, Murdock, talenti, iterazioni di documentazione<br>Kwang bilanciamento bilanciamento finale<br><!-- End of picture text -->

### 8. Conclusioni
Il presente PRD formalizza il design iniziale di _RefactorTactics_ , partendo dal roster v0.1 e dagli asset Paragon【6†L4-L8】. Abbiamo definito quattro eroi unici con abilità e ruoli differenziati, oltre a un innovativo sistema di reazioni in tempo rallentato. Le tabelle e i diagrammi presentati aiutano a confrontare e visualizzare le meccaniche. Le prossime fasi includono lo sviluppo del prototipo di base, iterazioni di bilanciamento sulle abilità e sull’interazione delle reazioni, e i test 2v2 indicati. Le assunzioni chiave (UE5 C++, server author., 1 reazione/personaggio, finestra 8s) sono dichiarate nel documento.

**Fonti principali:** Epic Games – pagina ufficiale Paragon (39 eroi AAA gratuiti)【6†L4-L8】; le idee di design derivano dalle note di progettazione interne discussi in chat.

---

## Idee sui ruoli dei personaggi

### Sintesi esecutiva
Nei migliori giochi tattici a turni, **l’ambiente e il posizionamento** amplificano enormemente la profondità strategica. Terreni variabili (acqua, fuoco, vegetazione, coperture) e oggetti interattivi (porte, barili esplosivi, interruttori) diventano strumenti da sfruttare in combo creative【7†L146-L154】 【37†L67-L75】. Ad esempio, in _Baldur’s Gate 3_ un’ambientazione completamente distruttibile ti spinge a dire “visto quella pozza? Elettrificala!” o “quel nemico sul ciglio? Gravità, tuo problema!”【37†L67L75】. Analogamente, in _Marvel’s Midnight Suns_ il gioco incentiva “attacchi ambientali” come sbattere un demone contro un muro o far cadere un lampadario【37†L160-L167】.

Questa ricerca fornisce:

- **Classi/Personaggi (≥12)** : esempi concreti (ruolo, meccanica principale, 4 abilità, combo, tradeoff).

- **Azioni/Interazioni (≥30)** : funzioni come spinta, elettrificazione, copertura ruotabile, con regole di targeting (cell-locked, path-intercept, ecc.) e priorità di risoluzione.

- **Terreni/Elementi mappa (≥20)** : acqua, fango, ghiaccio, ponti, barili esplosivi, trappole, correnti, gravità alterata… ciascuno con effetti sistemici e consigli di bilanciamento.

- **Pattern di design** : rischi (friendly-fire), costi azione, telegraphing, resource timing e counterplay ricorrenti.

- **Esempi da giochi esistenti** con citazioni (design docs, patch notes, wiki, GDC) per ognuno dei concetti chiave.

- **Proposte per il vertical slice** : 6 combo di squadra pronte da provare, 10 stati di terreno/stato con interazioni, e una sequenza di sotto-fasi suggerita (es. riconfigurazioni → movimento → abilità target → risoluzioni AoE) con diagrammi mermaid.

Tutto il materiale è organizzato in sezioni con tabelle e diagrammi per chiarezza (amiamo le guide stepby-step!).

#### Personaggi e classi (esempi)
|Personaggio /<br>Classe|Ruolo|Meccanica<br>fondamentale|Abilità principali<br>(descr. breve)|Combo tipiche|Trade-off|
|---|---|---|---|---|---|
|**Gadget (Tecnico**<br>**Elettro)**|Control<br>remoto /<br>danno in<br>linea|_Carica:_<br>accumula<br>carica elettrica<br>su oggetti o<br>nemici,<br>abilitando<br>archi elettrici|-_Scarica Lineare:_<br>attacco in linea;<br>devia se attraversa<br>acqua【13†L104-<br>L112】.<br>-_Nodo_<br>_Conduttore:_piazza<br>dispositivo che<br>conduce elettricità<br>tra posizioni.<br>-<br>_Sovraccarico:_AoE<br>su celle cariche;<br>distribuendo<br>danno tra più<br>bersagli.<br>-<br>_Capacitore_<br>_Reattivo:_counter:<br>assorbe danno e<br>carica chi attacca.|_Acqua+Elettricità:_coordina<br>con classe acqua per<br>shock (p.es.<br>Phase)<br>_Nodo + rimbalzi:_<br>posiziona nodo dietro<br>copertura e fai rimbalzare<br>fulmine.<br>_Counter +_<br>_Sovraccarico:_carica un<br>nemico poi<br>esplodi.<br>_Alimenta_<br>_porte:_elettrifica oggetti<br>ambientali.|Fragile, a dis<br>(Danno su pi<br>bersagli rido<br>bilanciare.)|
||||-_Getto_<br>_Pressurizzato:_<br>attacco in linea<br>con spinta|||
|||_Flusso:_genera|direzionale<br>variabile.<br>-<br>_Marea Circolare:_<br>AoE che bagna|_Scia + Fulmine:_Riva bagna<br>il percorso, Gadget<br>elettrifica.<br>_Nebbia +_<br>_Dash assassino:_copertura|Debole se us|
|**Phase**<br>**(Manipolatrice**<br>**Acqua)**|Supporto,<br>controllo<br>terreno|acqua sul<br>campo che<br>scorre verso<br>basso (riduce<br>attrito)|zone e spinge<br>unità verso<br>esterno.<br>-_Scia_<br>_Fluida:_dash che<br>lascia scia d’acqua;<br>attraversa<br>nemici.<br>-_Velo_<br>_di Nebbia:_<br>trasforma acqua<br>in nebbia che<br>riduce visibilità<br>(tiro incerto).|<br>e spinta<br>improvvisa.<br>_Marea +_<br>_AoE alleato:_spingi nemico<br>dentro area<br>alleata.<br>_Getto su_<br>_copertura fragile:_distruggi<br>coperture.|solitaria, dipe<br>alleati per da<br>massimo.<br>Consumabile<br>limitata per t|

|Personaggio /<br>Classe|Ruolo|Meccanica<br>fondamentale|Abilità principali<br>(descr. breve)|Combo tipiche|Trade-off|
|---|---|---|---|---|---|
||||-_Pannello Cinetico:_<br>piazza copertura<br>direzionale<br>protettiva (difende<br>solo da alcune<br>direzioni).<br>-<br>_Riconfigurazione:_|||
|**Riktor**<br>**(Architetto)**|Difesa /<br>copertura|_Strutture_<br>_direzionali:_<br>crea o ruota<br>coperture ad<br>hoc (varia<br>linee di tiro)|ruota o sposta<br>copertura<br>esistente (anche<br>porte, ponti).<br>-<br>_Ariete:_dash che<br>spinge unità; se<br>sbatte contro<br>copertura infligge<br>danno (ma<br>danneggia<br>copertura).<br>-<br>_Interposizione:_<br>counter difensivo<br>lungo un arco<br>diretto (protegge<br>solo un settore,<br>colpendo i<br>proiettili).|_Pannello + Elettricità:_<br>posiziona pannello e fai<br>rimbalzare fulmine di<br>Gadget.<br>_Ariete + Terreno_<br>_allagato:_spingi nemico<br>nell’acqua.<br>_Riconf. +_<br>_Fulmine:_apri linea di tiro<br>di Gadget.</br>_Interposizione_<br>_+ combo Phase:_proteggi<br>alleati mentre Phase<br>prepara combo.|Danni limitat<br>da scudo mo<br>Coperture fra<br>(shatter) rich<br>manutenzion<br>Elevata dipen<br>dalla prevenz|

|Personaggio /<br>Classe|Ruolo|Meccanica<br>fondamentale|Abilità principali<br>(descr. breve)|Combo tipiche|Trade-off|
|---|---|---|---|---|---|
|**Wraith**<br>**(Duellante**<br>**Predittivo)**|Eliminazione /<br>mobilità|_Intercettazione:_<br>colpisce<br>traiettorie<br>previste, non<br>solo posizione<br>corrente|-_Tiro d’Intercetto:_<br>scegli una cella e<br>una finestra<br>temporale; colpisci<br>chi ci passa<br>(azione sprecata<br>se nessuno la<br>attraversa).<br>-<br>_Lama di Passaggio:_<br>attacco lineare<br>corto che colpisce<br>chi attraversa la<br>linea.<br>-<br>_Deviazione:_dash<br>diagonale che<br>cambia direzione<br>di guardia dopo,<br>ignora unità come<br>pareti.<br>-_Finta:_<br>dichiara due<br>destinazioni<br>possibili; alla fine<br>conferma una<br>(coordina piani di<br>squadra, ma è<br>complessa da UI).|_Marea forzata:_Riva spinge<br>bersaglio nella cella di<br>intercetto.<br>_Riconf._<br>_armi:_Bastion chiude un<br>arco per rendere il<br>percorso<br>prevedibile.<br>_Elettricità_<br>_alternativa:_Flux crea vie<br>secondarie<br>(elettrificate).<br>_Presidio:_<br>Wraith sbarra l’unica<br>uscita rimasta.|Ottimo contr<br>bersagli prev<br>debole se ne<br>cambia piano<br>Difficile da b<br>(azioni a vuo|
|**Pyromancer**<br>**(Incantatore**<br>**Fuoco)**|AoE danno /<br>controllo<br>terreno|_Infiammazione:_<br>crea o<br>amplifica<br>incendi<br>ambientali<br>(con<br>persistent<br>burn)|-_Palla di Fuoco:_<br>proiettile che crea<br>campo infuocato<br>(Burning) di danno<br>per tempo.<br>-<br>_Parete di Fiamme:_<br>muro di fuoco<br>continuo<br>distruttivo.<br>-<br>_Campo Infernale:_<br>AoE di largo<br>raggio con<br>bruciatura<br>persistente.<br>-<br>_Detonazione Pyro:_<br>consuma stati<br>Burning vicini per<br>esplosione<br>aggiuntiva.|_Acqua + Fuoco:_spegne<br>incendi; si abbina con Phase<br>(ad es. creare nuvola di<br>vapore<br>steamy).<br>_Combustione_<br>_multipla:_coordina con<br>danni periodici di status<br>(es. +danno se già<br>Burning).<br>_Ignizione_<br>_immediata:_associa a un<br>alleato con vento /<br>scossa.<br>_Incendiare_<br>_barili:_usare barili esplosivi<br>con fiammata.|Tende a dann<br>anche amici<br>fire) se tropp<br>concentrato;<br>necessità di_c_<br>alto. Lento n<br>muoversi, vu<br>da vicino.|

|Personaggio /<br>Classe|Ruolo|Meccanica<br>fondamentale|Abilità principali<br>(descr. breve)|Combo tipiche|Trade-off|
|---|---|---|---|---|---|
|||_Congelamento:_|-_Dardo Gelido:_<br>danno a bersaglio<br>+ rallenta<br>(chilled).<br>-<br>_Tempesta_<br>_Invernale:_AoE che<br>abbassa visibilità e<br>rallenta (glaced|_Acqua ghiacciabile:_Riva<br>può creare pozze che<br>questa<br>congela.<br>_Elettrico:_<br>l’acqua gelata conduce|Debole contr<br>(lo spegne), d|
|**Cryomancer**<br>**(Incantatrice**<br>**Ghiaccio)**|Controllo /<br>area<br>rallentamento|trasforma<br>terreno/<br>bersagli in<br>ghiaccio (slip/<br>slow)|ground).<br>-<br>_Cristallo Riflettente:_<br>pietra di ghiaccio<br>che blocca fuoco e<br>riflette parte di<br>elettricità.<br>-<br>_FrantumaGhiaccio:_<br>colpo che rompe il<br>terreno ghiacciato<br>in scaglie,<br>spingendo<br>fragole.|elettricità in modo diverso<br>(shock + slow).<br>_Inferno:_<br>fare attenzione: usare<br>acqua per spegnere il<br>fuoco altrimenti Crona<br>perde<br>vantaggio.<br>_Inciampo_<br>_alleati/avversari:_posiziona<br>laghi ghiacciati.|da condizion<br>ambientali (a<br>freddo). Mov<br>su ghiaccio p<br>sfavorirlo se<br>gestito.|
|**Pistolero**<br>**(Sharpshooter)**|DPS a<br>distanza /<br>precisione|_Fuoco_<br>_penetrante:_<br>mira lungo<br>traiettorie<br>prevedibili<br>(tracking<br>moderato)|-_Colpo Perfetto:_<br>attacco unico a<br>lunga gittata, alto<br>danno.<br>-<br>_Pioggia di Proiettili:_<br>breve AoE lineare<br>(spara proiettili in<br>linea<br>scaglionati).<br>-<br>_Solo 1 Secondo:_<br>bonus danno al<br>bersaglio se<br>attaccato da altri<br>prima.<br>-<br>_Occhio di Falco:_<br>focalizza un’area:<br>miglior mira/<br>critico (buff alla<br>squadra per un<br>turno).|_Flank:_coordina con<br>Riktor per avere linee di<br>tiro pulite.<br>_A tutta:_<br>aggiunge Shortrange al<br>Tiro d’Intercetto di<br>Wraith.<br>_Barili:_<br>sincronizza con Pyro (barili<br>incendiabili e<br>scoperchiati).<br>_Supporto_<br>_Blitz:_assist con spotter<br>(identifica bersaglio<br>invisibile).|Fragile da co<br>corpo; una s<br>azione forte.<br>Requisto: de<br>corto raggio,<br>**overwatch**n|

|Personaggio /<br>Classe|Ruolo|Meccanica<br>fondamentale|Abilità principali<br>(descr. breve)|Combo tipiche|Trade-off|
|---|---|---|---|---|---|
|**Ingegnere /**<br>**Meccanico**|Trappole /<br>supporto|_Costruzione:_<br>piazza<br>torrette,<br>trappole e<br>interruttori|-_Miniera_<br>_Magnetica:_<br>trappola a terra<br>che attira robot<br>(immobilizza e<br>danneggia).<br>-<br>_Torretta Dronica:_<br>dispositivo che<br>attacca bersagli<br>autonomamente<br>(rotate di<br>fuoco).<br>-<br>_Riparazione_<br>_Rapida:_ripristina<br>copertura / campo<br>sul posto o dà<br>scudo<br>temporaneo.<br>-<br>_Interruttore_<br>_Remoto:_aziona<br>porte, piattaforme<br>o mina (cerca<br>sinergie).|_Trappole + Combos:_spingi<br>nemico in zona mortale<br>con Ariete +<br>Miniera.<br>_Alimenta_<br>_zona:_Flux alimenta le<br>torrette se<br>caricate.<br>_Distruzione +_<br>_Miglioramento:_rimuovi<br>barriera con esplosione e<br>sostituiscila con<br>torretta.<br>_Doppio turno:_<br>in risposta di supporto<br>con Finta.|Dipende dal<br>posizioname<br>statico; costi<br>(moduli limit<br>Delayed (ci m<br>turni a costru|
||||-_Richiamo Bestiale:_<br>evoca minion con<br>ruoli diversi (tank/<br>suicida/|||
||||buffer).<br>-<br>_Marchio del_|_Sacrifico:_coordina con<br>Supporto (usare minion||
|**Evocatore**|Summoning /<br>paura|_Invocazione:_<br>crea creature<br>o effetti di<br>controllo<br>temporanei|_Terrore:_marchia<br>un nemico, lo<br>debolisce se vicino<br>ad alleato.<br>-<br>_Aura Vampirica:_<br>drenaggio che<br>cura te/alleati in<br>base ai danni<br>inflitti.<br>-<br>_Polverizzazione:_<br>colpo finale che<br>consuma alleati|come scudi per curare di<br>più).<br>_Marcatore +_<br>_follow-up:_combinare colpi<br>bassi dopo un marchio di<br>Wraith.<br>_Area share:_<br>rilascia psiceti di danno e<br>cura (PyroInflitto sui<br>minion).<br>_Sovraccarico:_<br>Gadget carica i minion per<br>esplosione ad area.|Le creature p<br>distrarre o m<br>rapidamente<br>Complessità<br>gestione elev<br>Spesso debit<br>risorse di evo|
||||addizionali per<br>potenziarsi.|||

|Personaggio /<br>Classe|Ruolo|Meccanica<br>fondamentale|Abilità principali<br>(descr. breve)|Combo tipiche|Trade-off|
|---|---|---|---|---|---|
|**Assassino /**<br>**Furtivo**|Eliminazione<br>singolo /<br>mobilità|_Occultamento:_<br>invisibilità/<br>trasparenza,<br>colpi critici alle<br>spalle|-_Colpo Silenzioso:_<br>attacco singolo<br>letale (backstab)<br>con drastico<br>critico da<br>dietro.<br>-_Fumo_<br>_Paralizzante:_lancia<br>fumogeno che<br>nasconde alleati/<br>avversari (riduce<br>visibilità).<br>-<br>_Lame Rotanti:_AoE<br>corta attivabile al<br>passaggio|_Nebbia + Shuriken:_la<br>fumata di Phase fornisce<br>copertura; sfrutta lame<br>per danno.<br>_Intercetta +_<br>_Elimina:_Vektor marca<br>bersaglio, Assassin ci<br>balza addosso.<br>_Rete +_<br>_Ariete:_intrappola<br>bersaglio e Riktor lo<br>spinge.<br>_Trappole /_|Potente su s<br>bersagli ma<br>estremamen<br>fragile. Se ril<br>esposto. Dip<br>fattore sorpr|
||||(colpisce chiunque<br>attraversa).<br>-<br>_Trappola a_<br>_Ragnatela:_<br>paralizza bersaglio<br>per breve tempo.|_Cloak:_evocatori con<br>minion come decoy.||
||||-_Carica_<br>_Tempestosa:_corsa<br>verso nemico, lo|||
|||_Carica & Taunt:_|spinge e colpisce<br>ad area.<br>-<br>_Colpo di Scudo:_<br>scudo in avanti|_Spinta in voragine:_Ariete +<br>Terrain: spinge in baratro<br>o fuoco.<br>_Taunt +_<br>_Copertura:_protegge|Velocità lenta|
|**Cavaliere /**<br>**Guerriero**|Corpo a<br>corpo / tank /<br>crowd control|<br>spaziale con<br>push,<br>protezione<br>squadra|che atterra (stun)<br>chi sbatte.<br>-<br>_Ruggito_<br>_Intimorente:_buff<br>difesa alleati<br>(taunt avversari<br>per centro).<br>-<br>_Martello Meteorico:_<br>attacco AoE<br>verticale (colpisce<br>zona ampia).|alleato vulnerabile (con<br>Riktor).><br>_Ruggito e_<br>_follow-up:_Vektor lo<br>anticipa fuori dalla<br>copertura con<br>Instinct.<br>_Buff sinergico:_<br>supporta Maghi area con<br>scudi e spinge nemici.|di rimanere i<br>Scalabile peg<br>endgame se<br>insensibili ai<br>control.|

|Personaggio /<br>Classe|Ruolo|Meccanica<br>fondamentale|Abilità principali<br>(descr. breve)|Combo tipiche|Trade-off|
|---|---|---|---|---|---|
||||-_Cura Radiale:_<br>cura AoE|||
||||moderata intorno|||
||||a sé.<br>-|_Sinergie difensive:_||
||||_Benedizione del_|mantiene vivo il gruppo|Danno dirett|
||||_Santuario:_crea|durante combo di|nullo; dipend|
|||_Aure di_|zona di guarigione|Posizione (es. Phase o|team. Vulner|
|||_Supporto:_|continua (orbe|Riktor sul|esposto, rich|
|**Supporto /**|Guarigione /|fornisce|AoE).<br>-_Aura di_|fronte).<br>_Buff stacking:_|azioni chiare|
|**Curatore**|buff squadra|ripristino o|_Protezione:_|prepara gruppo prima di|fuori dall’azio|
|||potenziamenti<br>difensivi|aumenta<br>temporaneamente|combo con stato<br>Esposto.<br>_Campo_|frontale).<br<br>friendly fire a|
||||ARM/RES agli<br>alleati vicini.<br>-<br>_Silenzio Purificante:_<br>rimuove stati|_alleato:_di solito in fondo<br>alla linea, coordinato con<br>snip.|meno (oppur<br>necessità).|
||||negativi in area (o<br>scudo diurno).|||

**Nota:** Le classi esposte qui sono esempi compositi. Adattate a design astratto, servono a illustrare ruoli e combo possibili. Le abilità possono variare in meccanica e numerazione di turni (cooldown). I trade-off tipicamente bilanciano potenza vs rischi, esposizione vs difesa e costo risorse (AP/mana).

#### Azioni e interazioni chiave
Cataloghiamo 30+ azioni/meccaniche ricorrenti, descrivendone effetto, risoluzione e priorità. Ogni abilità nel gioco deve dichiarare come segue il suo targeting (ad es. _cell-locked_ vs _unit-tracking_ , ecc.) 【13†L104-L112】【44†L69-L73】. Le regole di risoluzione tipiche includono:

- **Push (Spinta/Knockback)** : sposta l’unità bersaglio lungo una direzione. _Resoluzione:_ di solito **cell-locked** (la spinta prosegue lungo la linea originale anche se il bersaglio si muove), o _pathintercept_ se si vuole cogliere unità in movimento【44†L69-L73】. _Priorità:_ Alto, perché cambia posizionamento (opportunità di incassi o ostacoli). Possibile effetto ritardo o _opportunity fire_ da chi viene spinto (XCOM reaction-fire).

- **Pull (Tiro / Attraction)** : avvicina bersaglio al lanzatore. _Cell-locked_ o _unit-tracking_ . Usato per portare nemici su trappole o linee di fuoco【44†L69-L73】.

- **Elettrificazione** : colpisce un’area o oggetto con effetto elettrico, spesso diffondibile su caselle bagnate o cariche. _Path-intercept_ (elettricità si propaga se incontri terreno conduttivo)【13†L104L112】. Ad esempio, Gadget si basa su _Carica_ per far rimbalzare scariche tra nodi.

- **Irrigidimento/Gelificazione** : trasforma acqua in ghiaccio o rallenta unità. _Cell-locked_ , indipendente dal movimento del bersaglio. Può creare percorsi scivolosi (beni per il giocatore, a rischio di scivolare o rimanere intrappolati).

- **Bruciatura** : aggiunge un danno per tempo. Colpisce unità o terreno infiammabile (oggetti, erba secca). _Unit-tracking_ (continua a danneggiare l’unità se resta nella zona). Interazioni: spegnibile dall’acqua【13†L117-L122】.

- **Poison/Acid** : danno nel tempo su aree (gas, polvere tossica). _Path-intercept_ se nube si sposta o diffonde. Tipicamente non visibile a lungo raggio, cambia velocità.

- **Frost (Freezing)** : immobilizza o rallenta drasticamente (target-tracking). Si dissolve con Fuoco.

- **Scarica EMP** : stordisce macchine (mech, robot); _area di effetto_ fissa. _Cell-locked_ con durata (es: EMP XCOM).

- **Soak / Bagnato** : non è un’azione ma uno stato applicato (es. pioggia, marea). Indica **presa d’acqua** : aumenta conduttività, fa scivolare, spegne fuoco.

- **Ignition / Accensione** : applica stato _Burning_ se terreno o unità sono infiammabili. Occorrenza con fiamme o armi incendiarie.

- **Create/Destroy Cover** : piazza o rimuove coperture. _Create:_ often _cell-locked_ sulla cella designata. _Destroy:_ minaccia permanente (AD esempio granata XCOM distrugge copertura【28†L504L512】). **Priorità:** modificano linee di tiro.

- **Rotate Cover (Riconfigurazione)** : cambia orientamento di ostacoli. _Cell-locked_ , istantaneo (spesso come sottofase iniziale【7†L156-L164】).

- **Open/Close Gate** : attiva/disattiva passaggi. _Cell-locked_ . Utile per forzare scelte di percorso (imbuto).

- **Change Elevation** : salire/abbassare livelli (ascensore, scala). _Cell-locked_ ; produce nuove vie/ angoli coperti.

- **Pressure Trap** : scatta quando unità entra in una cella (senza targeting iniziale). _Path-intercept_ implicito.

- **Displacement (Drag/Pull friendlies)** : trascinare alleati verso sicurezza. _Unit-tracking_ , consuma azione.

- **Concealment (Fumo, Bush)** : crea status di **nascosto** per chi vi entra. _Cell-locked area_ di effetto. Abbassa precisione e blocca linee vista.

- **Vision-Altering (Nebbie, Oscurità)** : zona dove linee di tiro vengono confuse ( _scatter_ ). _Cell-locked_ . Es. la Nebbia di Phase rende incerti i colpi.

- **Teleport (Warp)** : bersaglio spostato istantaneamente in un’altra cella. _Destination locked_ (spostamento assoluto), _unit-tracking_ entro l’istante.

- **Delayed Blast** : abilità con trigger ritardato (es. conto alla rovescia). _Cell-locked_ , attivazione a scadenza.

- **Area Buff/Debuff** : zone che potenziano o indeboliscono chi vi entra (es. Aura di Riktor, zone sacre). _Cell-locked_ , persistenti.

- **Launch/Impale (Catapulta)** : tipo di push verticale (proj. su più tile). _Unit-tracking_ verticale, _pathintercept_ orizzontale.

- **Charging** : movimenti continuativi seguiti da attacco finale (es. Warrior). Sequenza _cell-locked_ per movimento, _target locked_ finale.

- **Counterattack / Opportunity Fire** : reazione dopo spinta o entrata in area bersaglio. _Triggered_ , non pianificata (basata sul movimento di nemici).

- **Blast (explosion)** : AoE con caduta lineare di danno (simile a granata). _Cell-locked_ sul punto scelto.

- • **Heal/Restore** : AoE di cura (segue unit-tracking sui bersagli). Può rimuovere stati come _Infezione_ . • **Stealth/Disguise** : cambia temporaneamente la rilevabilità del personaggio. _Self-target_ ; nessuna risoluzione se scoperto (fallisce).

- **Tethering / Link** : connette due unità (es. lampo magico, chain lightning). _Unit-tracking_ tra esseri collegati.

- **Gravity Well** : zona che attrae o respinge unità. _Cell-locked_ campo con effetto continuo.

Per ciascuna azione, in fase di **pianificazione** l’interfaccia deve chiarire se il bersaglio è una cella fissa ( _cell-locked_ ), un’unità mobile ( _unit-tracking_ ), una traiettoria ( _path-intercept_ ), una direzione predefinita ( _direction-locked_ ), o se ricalibra sul nemico più vicino ( _retarget_ ), o se fallisce su invalidità ( _cancel_ ). Questo evita frustrazione e rende chiari i trade-off delle combo.

#### Terreni e oggetti mappa (esempi)
Ogni elemento mappa offre vantaggi/svantaggi sistemici e interagisce con le abilità. Qui 20+ esempi comuni con effetti e suggerimenti di bilanciamento:

|Elemento|Effetto / Uso tattico|Bilanciamento / Nota|
|---|---|---|
|**Acqua (Bagnata)**|Riduce velocità. Conduce elettricità<br>(facilita_electrify_【13†L109-L112】). Fa<br>scivolare unità (salvo ancoraggio).<br>Spegne fuoco/incendi. Mantiene<br>target**umidi**.|Limitare spazi d’acqua troppo<br>grandi. Elevata strategia: può<br>uccidere (affogamento) o<br>rallentare la battaglia.|
|**Fango /**<br>**Quicksand**|Rallenta fortemente, in certi casi<br>blocca (anchorage). È simile all’acqua<br>ma guasta armi a distanza.|Evitare zone vaste; usare per<br>trappole.|
|**Ghiaccio /**<br>**Slippery Floor**|Chi ci cammina scivola di 1-2 caselle<br>verso una direzione predefinita. Usa<br>_path-intercept_se in movimento.|Più utile per il giocatore che per<br>NPC (altrimenti frustrazione).<br>Opzione: permetti ancoraggio o<br>non attraversarlo.|
|**Punto Elevato**<br>**(High Ground)**|Bonus di attacco/difesa (maggior<br>range, danno), linee di tiro estese. Es.<br>portale mira↑15%.|Cruciale per strategia posizionale.<br>Scegliare leve moderate di<br>vantaggio.|
|**Bassa Quota (Low**<br>**Ground /**<br>**Coperture Raso)**|Copertura_parziale_: riduce precisione<br>nemici, ma non conferisce vantaggio<br>adiacente.|Bilanciare fornendo copertura<br>alternativa vicino.|
|**Copertura Solida**<br>**(Rocce, Mura)**|Blocca la linea di tiro. HP se<br>distruttibile (es. muro di pietra). Cover<br>bonus.|Fragile o staccabile come risorsa.<br>Usare HP moderato e abbattere<br>possibili per combo (esplosivi,<br>fuoco)【28†L504-L512】.|
|**Copertura Fragile**<br>**(Barili, Legno)**|Si distruggono con un paio di colpi o<br>fuoco. Poi scompaiono.|Ideali per danno ambientale<br>(barili esplosivi). Dare reazione<br>visiva (crepature).|
|**Fiamme (Burning**<br>**Ground)**|Danno nel tempo alle unità sopra<br>(Burning). Può espandersi o essere<br>estinto.|Normalmente permanente breve<br>(es. 2 turni). Spegnibile con 1<br>abilità acqua.|
|**Vapore / Steam**<br>**Cloud**|Nebbia densa (copertura), può<br>dissippare elettricità in fumo acido se<br>post-combo. Copertura totale visiva.|Arco-Idea: nebbia sottile<br>(volumetrica). Avere solo nel<br>breve termine (creata da mix<br>fuoco+acqua).|
|**Fumo (Smoke)**|Migliora difesa unità dentro (+defisa),<br>blocca linee-sito. Non danneggia da<br>solo.|Durata limitata (turni). Usato per<br>mascherare movimenti.|

|Elemento|Effetto / Uso tattico|Bilanciamento / Nota|
|---|---|---|
|**Gas Tossico**|Nuvola che attraversa muri (es. gas<br>velenoso XCOM【28†L518-L524】).<br>Danno/inganno continuo.|Durata breve, appare sorpresa.<br>Limitare spread per non soffocare<br>tattica.|
|**Cibo / Pozioni**<br>**(Consumable)**|Carota o trucco di gioco: fornisce vite<br>bonus o status, usate fuori<br>combattimento.|Elemento di esplorazione, non<br>sempre in combattimento.|
|**Porta / Cancello**|Blocca passaggio finché chiusa. Può<br>essere attivata (leva, interruttore,<br>distrutta).|Chiudibile solo prima del turno<br>avversario; può chiudersi<br>automaticamente. Servono<br>segnali UI chiari quando si apre/<br>chiude.|
|**Interruttore /**<br>**Leva**|Attiva meccanismi (apre porte,<br>attraversa ponti, disattiva trappole).<br>Azione_cell-locked_su punto fisso.|Può richiedere tempo di<br>attivazione. Feedback visivo (stato<br>su/off).|
|**Ponte sospeso /**<br>**Collassabile**|Collassa se sottoposto a peso o danno:<br>crea**choke point**non permamente.|Vari livelli di vita (es. legno<br>debole). Rischio di intrappolare<br>alleati.|
|**Ascensore /**<br>**Scalinata**|Cambia elevazione su domanda (leva o<br>tempo). Mappa verticale (2 livelli).|Sovente ciclico (ogni X turni) per<br>prevedibilità. Timeout a pazienza.|
|**Barile esplosivo**|Scoppia con danno AoE e fuoco.<br>_Trigger:_colpito da danno da impatto,<br>fuoco, arma (vicino).|Rilevabile (colore lampeggiante).<br>Impatto alto, attento al friendly<br>fire.|
|**Trappola a**<br>**pressione (spike)**|Danno o morte istantanea se unità si<br>avvicina o entra (cell-locked trigger).|Ricarica manuale dopo<br>attivazione. Rischio su alleati,<br>quindi uso raro.|
|**Ventola /**<br>**Soffiante**|Sposta unità (o proiettili) in una<br>direzione. (_Unit-tracking_durante<br>azione).|Finestra di utilizzo limitata (es. 1<br>turn per flusso). Bilanciare<br>portata.|
|**Corrente (flusso)**|Spinge costantemente unità lungo una<br>traiettoria (corrente d’acqua/aria).|Mappa dinamica: modifica<br>posizioni previste. Da usare come<br>ostacolo naturale.|
|**Area Gravità**<br>**Alterata**|Peso/appoggio diverso: spinge verso il<br>basso o zero-G (levitazione).|Flussi visibili (effetti grafici). Effetti<br>su proiettili e danni da caduta.|
|**Rete / Vischio**<br>**(Web)**|Imprigiona bersaglio (immobilità).<br>Lunga durata, rompibile solo con<br>danno dedicato.|Pochi utilizzi per azione (difficile<br>da impostare in combat per GC).|
|**Campo di**<br>**occultamento**|Copertura invisibilità per 1-2 turn se vi<br>si entra (erba alta, lanterna).|Durata breve, visibile chiaramente<br>ai giocatori (luce soffusa).|

**Bilanciamento generale:** Evitare di disseminare troppi pericoli fatali; meglio zone moderate che aggiungono opportunità piuttosto che indiscriminato _killable area_ . Ad esempio, l’acqua profonda in molti giochi infligge danno letale (es. _push nel lago -> danno 999_ 【26†L201-L205】 nel mod FFT). In

genere, ogni effetto ambientale potente dovrebbe avere un _trade-off_ (e.g. colpire alleati, costare azioni/ risorse, visibilità ridotta).

#### Pattern di design ricorrenti e trade-off
Nei giochi tattici emergono pattern chiave:

- **Rischio e Friendly-Fire:** abilità AoE/ambientali spesso possono colpire amici (elemento di rischio). Ad es., XCOM definisce i colpi con distruzione coperture come _“set environment on fire”_ 【28†L504-L512】. Ciò spinge il giocatore a ponderare: vale la pena spingere un nemico nell’acqua se ne faremo lo stesso _electroshock_ degli alleati? (Trade-off: alto danno vs rischio danni interni).

- **Telegraphing / Counterplay:** abilità potenti dovrebbero dare indizi visivi/anticipazioni (es. area di effetto in evidenza). Ad es. Frozen Synapse (sistemi simultanei) richiede che le azioni siano intuibili (lanciano zone di colpo prima). Questo consente contromosse (dash fuori dall’area, muro contro). Inoltre, _coperture direzionali_ (Riktor) introducono telegraphi: difendi solo da un lato. Se il nemico attacca da fuori, passa oltre【19†L181-L184】.

- **Costo temporale / Resource timing:** abilità forti richiedono molte risorse (tempo, punti azione, mana). I giocatori devono bilanciare usarle subito o tenerle. Ad es. Larian, parlando di BG3, evidenzia come surface effects costino risorse preziose: “in DOS era trucco, in BG3 serve un incantesimo dedicato”【35†L88-L96】.

- **Friendly-Fire come tattica:** come suggerito da un’analisi di _Horizon’s Gate_ , a volte si spinge intenzionalmente unità dal percorso di attacchi amici【44†L69-L73】; questo diventa un aspetto tattico legittimo. Il rischio di colpire alleati trasforma il friendly-fire in uno strato in più di strategia.

- **Posizione vs Statistiche:** come sottolinea la sinistra di Sinister Design【7†L146-L154】 e i pad del _Midnight Suns_ 【37†L160-L167】, spesso la posizione (flank, high ground, colonne) vale più di buff numerici. Ad esempio, in _Gears Tactics_ lo stimolo è sempre avanzare e coprirsi in movimento 【37†L120-L126】; nel nuovo gioco _Skell Breaker_ o _Horn_ (basati su Atl. Reactor) il fatto di prevedere dove stare farà la differenza.

- **Obiettivi multipli:** oltre a uccidere, altri obiettivi (salvare civili, proteggere artefatti) creano scelte. Il blog di Sinister citava _Fire Emblem_ ed _XCOM_ come esempi dove raccogliere risorse o mantenere la morale è fondamentale【7†L175-L184】. Ciò incoraggia decisioni sub-ottime (es. curare anziché attaccare).

- **Delay Attacks / Reazione:** abilità di **ritardo** , come contrattacchi o trigger su movimento, arricchiscono il battleflow. _Frozen Synapse_ e _Atlas Reactor_ sono esempi emblematici: la sorpresa di un colpo di intercetto dipende da quanto hai previsto il nemico. Anche XCOM ha _Overwatch_ e reazioni. Queste meccaniche premiano la pianificazione.

Questi pattern mostrano che ogni potere deve bilanciare _potenziale creativo_ (emergent complexity) con _chiarezza di conseguenze_ e _predicibilità_ 【5†L81-L89】【7†L156-L164】. L’incorporazione di costi e telegraphic cues assicura che le combo restino un _opportunità di skill_ e non mera fortuna.

#### Esempi concreti da giochi esistenti
Per ispirarci, vediamo alcuni casi reali:

- **Electrify + Water in Dos:** _Divinity: Original Sin_ consente combo elemento-su-elemento: es. _Water Puddle + Electrical Spell = Electrified Water Puddle_ 【13†L104-L112】. Ogni round gli alleati nel pool prendono danno. Questo definisce bene _water + shock combo_ .

- **Burn + Wind/Gas:** Allo stesso modo, _oil barrel + fire_ produce esplosioni e terreno in fiamme 【13†L98-L105】. Le superfici combinate come _Poison + Fire → Esplosione Tossica_ dimostrano effetto domino ambientale.

- **Cover Distructible:** XCOM 2 mostra che granate frantumano coperture ed incendiano oggetti vicini【28†L504-L512】. Il bilanciamento è chiaro: danno extra ma nessuna rottura totale del gioco (gli alleati hanno armi per spostarsi se perdono cover).

- **Push a hazard:** Nei giochi FFT-like, spingere un nemico nell’acqua o lava causa morte immediata 【26†L201-L205】. Il blog di un modder FFT cita knockback letali: “> into lava for 999 damage”【26†L201-L205】. _Triangle Strategy_ e _Fell Seal_ usano questo concetto (acqua/lava come morte sicura).

- **Multistage Combos (Horizon’s Gate):** Come annotato nel _Radiator blog_ , _Horizon’s Gate_ abbonda di skill push/pull: es. spingi un nemico contro un altro per farli giacere in una linea di tiro comune【44†L69-L73】. O cattura nemici in stun multipli prima di un enorme AoE finale (chain di status). Questo è un ottimo modello di combo emergente ambientale.

- **Attacchi da copertura (Gears Tactics):** Il sistema di _overwatch_ e cover direzionale di Gears fa sì che muoversi dietro ostacoli e abbassarsi sia sempre favorevole【37†L120-L126】. Eliminare cover con granate diventa quindi tattica per _spread damage_ .

- **Sinergie AoE (Into the Breach):** In _Into the Breach_ (turni tradizionali), combinare push e fuoco ad area è fondamentale: spingi i Mech negli attacchi dei colossali, o usa il fuoco sui Vek allineati. Ogni battaglia assomiglia a un puzzle. Menzioniamo però _pure simulazioni tattiche_ , non fonti citate.

Questi esempi mostrano che **la mappa stessa è parte del roster** : usare il terreno per “pensare fuori dagli schemi” è la miglior tattica【7†L156-L164】【37†L67-L75】.

#### Proposte implementative per il vertical slice
**Combo di squadra (6 esempi test):** formuliamo composizioni integrando più meccaniche:

1. **Trappola a imbuto** :

2. Riktor posiziona pannello chiudendo un arco.

3. Phase allaga il percorso alternativo.

4. Wraith prepara un Tiro d’Intercetto all’unica uscita.

5. Gadget elettrifica l’acqua residua. _Esito:_ Il nemico ha opzioni limitate (accettare shock, rompere portale, fermarsi, subire attacco in uscita). Combo graduale, _NON kill garantito_ , ma riduce progressivamente opzioni nemiche.

6. **Martello e incudine** :

7. Riktor usa Ariete caricato.

8. Phase contrattacca con Getto pressurizzato da lato opposto. _Esito:_ Il bersaglio viene costretto contro una copertura o sbilanciato. _Nota bilancio:_ Evitare danno doppio massivo. Dare magari solo stordimento o _Sbilanciato_ per 1-2 turni (come perde controllo, senza morte istantanea).

##### 9. **Circuito vivente** :

10. Phase lancia Scia Fluida (zona d’acqua persistente).

11. Gadget posiziona Nodo Conduttore al centro.

12. Un nemico entra nell’acqua.

13. Gadget attiva Sovraccarico (fulmine esplosivo).

   - _Esito:_ Elettricità ad area tra le acque, colpendo bersaglio e potenzialmente anche Phase se ancora nella pozza. Richiede _tempismo_ non solo posizionamento. Trade-off in questa combo: _coordinazione elevata, rischio friendly-fire_ .

##### **Rete + Spinta** :

##### 14.

15. Rogue (Furtivo) piazza Trappola a Ragnatela su un percorso.

   - Riktor usa Ariete spostando bersaglio.

17. Se il nemico finisce sulla ragnatela, rimane intrappolato e vulnerabile. _Esito:_ Highlight sulla coordinazione tra trappola e movement. Debole contro nemici volanti o già fuoriposto.

##### 18. **Controllo del ponte** :

19. Riktor ruota coperture per creare canale.

   - Supporto sacrifica un turno buffando schieramento.

21. Phase sposta unità nemiche nel canale con Marea Circolare.

22. Gadget incatena scariche elettriche tra due nodi piazzati. _Esito:_ Nemico costretto a passare in zona elettrica, subendo danni continui. Trade-off: lunga preparazione, dipendenza sequenza.

##### 23. **Sventramento d’occhio** :

24. Wraith dichiara destinazioni (Finta) per ampliare rami di piano.

   - Phase e Gadget preparano attacchi in due possibili scorciatoie.

26. Dopo aver visto da dove il nemico tira fuori, la squadra si concentra nella direzione corretta. _Esito:_ Coinvolge planning segreto/cooperativo. Difficile da implementare via UI ma potente come concetto di coordinazione a turni simultanei.

**Stati utili (10):** definiti da significato sistemico, usabili per combo:

- **Bagnato (Wet):** il terreno/bersaglio è innaffiato. Conduce elettricità (dà +danno a shock), spegne fuoco/incendio【13†L117-L122】, aumenta scivolosità (rischio di spinta). I nemici “bagnati” possono anche diventare _sofferenti (soaking)_ per certe magie.

- **Carico (Charged):** in attesa di una scarica elettrica. Es. dispositivi o nemici che ricevono _Carica_ . Fa scattare archi elettici adiacenti; serve a innescare macro-attacchi come _overload_ di Gadget.

- **Sbilanciato (Unbalanced):** subisce penali ai tiri salvezza contro spinte/trazioni e aumento di precisione subita. Non impedisce azioni, ma rende vulnerabile a push/pull. Si ottiene se colpiti durante un movimento (es. interrompere il dash).

- **Esposto (Exposed):** alcune direzioni non protette. Ad es. un nemico con copertura _laterale_ ma nessuna dietro: dietro è direzione _exposed_ . Conferisce bonus d’attacco all’assalitore e penalizza difesa sul lato “scoperto”. Importante specificare direzioni (non uno stato globale).

- **Ancorato (Anchored):** immune alle spinte/spostamenti. Ad es. bloccato da una trappola a terra o ancorato al suolo. Spinta/corpi non fanno nulla.

- **Infiammato (Burning):** subisce danno per tempo. Apporta colore ambientale (Fiamme persistenti). Rimosso dall’acqua (spegnimento). Non stacks (una sola sorgente di Burning per turno).

- **Gelato (Frozen):** rallentamento massimo o immobilizzazione. Viene da _Wet + Freeze_ . Si spezzerà se inflitto danno da fuoco o forte schianto. Elemento di CC forte.

- **Velato (Concealed/Fogged):** riduce visibilità delle unità (nel filo di visione). Ad es. dentro fumo o nebbia. Penalizza la mira dei lontani (pattern Diablo). Non impedisce fisicamente le azioni, ma limita i target selezionabili.

- **Sfortunato / Debilitato (Cursed/Poisoned):** colpo di scenario infetto (veleno). Danno nel tempo moderato. Non permette cure immediate (es. rimuovi con antidoto / rientro in zona pulita).

- **Collettivo di stato (Chain Mark):** bersaglio collegato (tipo scudo collegato) dove danni o effetti si trasferiscono. Ad esempio, nemici afflitti da “Mark” di Wraith o “Link” elettrico. L’abilità successiva che colpisce uno riflette parte sul secondo.

(Abbiamo evitato stun/perma stun o silenzi completi per mantenere flusso tattico, dato che sono spesso frustranti in sotfware di design astratto.)

#### Sottofasi della sequenza di combattimento (timeline)
Le azioni simultanee richiedono ordine ben definito. Proponiamo questa timeline:

<!-- Start of picture text -->
Riconfigurazioni Movimento unità Azioni abilità Risoluzione effetti<br>Riconfigura ambiente (es. rotate cover)<br>Muovi tutte le unità (simultaneo)<br>Piani di attacco preparati (target, dash, trappole)<br>Risolvi danni / stati / interazioni di AoE<br>Riconfigurazioni Movimento unità Azioni abilità Risoluzione effetti<br><!-- End of picture text -->

- **Riconfigurazioni** : fasi preliminari dove abilità come _Riconfigurazione_ di Riktor o chiusura porte di team assegnato avvengono. Non ancora movimenti di unità.

- **Movimento simultaneo** : tutte le unità eseguono dash/direzione finale. Poiché è simultaneo, serve prevedere: per esempio, un push di questo turno sposterà il nemico nella casella risultante.

- **Azioni abilità in Target** : ogni unit dichiara ora le abilità (sparare, lanciare, attivare device). Qui si usano i targeting modes (cell-locked, etc). Nessuna espansione di effetti ancora.

- **Risoluzione ed Effetti** : calcolo simultaneo di danni, knockback, catene di status. Se due abilità colpiscono lo stesso bersaglio, risolvono secondo priorità (ad esempio, effetti di stato di area prima di colpi singoli, o viceversa, da definire in prototipo). Importante definire quale abilità applica il proprio effetto quando il bersaglio si muove (come discusso sopra).

Questo schema garantisce coerenza: le modifiche mappa avvengono prima del posizionamento, poi tutti i movimenti, quindi le azioni in base alla nuova disposizione, infine gli effetti incrociati.

#### Diagrammi Mermaid
**Timeline (sottofasi):** mostrato sopra in `sequenceDiagram` per chiarezza di flow di turni.

##### Esempio Combo multi-actor (Trappola a imbuto):
|chiude arco<br>Phase: allaga percorso<br>Riktor<br>Arco bloccato<br>Canale d'acqua|Wraith: intercetta uscita<br>Uscita svelata|Gadget: elettrifica acqu|a<br>Scossa elettrica|
|---|---|---|---|

**Legenda:** Riktor chiude un passaggio; Phase forma un canale d’acqua alternativo; Wraith piazza un intercetto sull’unica via; Gadget electrocuta il percorso.

##### Mappa semplificata (interazioni ambientali):

<!-- Start of picture text -->
Acqua sul terreno elettrificabile Scossa<br>Barile esplosivo scoppia Area Fiamme<br>Recinzione ruota Aperta Portone aperto Permette passaggio<br>Trappola - Puntoni trigger Danno letale<br>Copertura fragile colpita Detriti rimuove Nuova linea di tiro<br><!-- End of picture text -->

Questo flowchart mostra come acqua, barili, porte e trappole possano connettersi: l’acqua resiste a elettricità (Shock), i barili esplodono creando fuoco, leve ruotano porte, punte infliggono danno, coperture abbattibili riaprono linee di tiro.

#### Conclusioni
Nel progettare un vertical slice tattico basato su ambiente e previsione, è cruciale coniugare libertà di combinazioni con regole chiare e trade-off giusti. Abbiamo visto ispirazione in _BG3_ , _Divinity OS_ , _XCOM_ , _Frozen Synapse_ , _Horizon’s Gate_ , etc., i cui design note sottolineano l’importanza di **spazio** e **manipolazione** del terreno【7†L146-L154】【44†L112-L117】. Le combo di squadre proposte testano identità chiare e pilastri di design: flussi ambientali, archi di visione, coperture direzionali, targeting predittivo. Gli stati ambientali e interazioni elencate garantiscono profondità emergente senza appesantire l’interfaccia.

Continuare esplorando questi concetti porterà a un prototipo verticale ricco: consente di “suonare il campo di battaglia” come uno strumento (cercando quell’effetto **wow** tattico quando incastri il tuo piano vs nemico). Con tabelle, diagrammi e fonti, questa analisi serve come **guida passo-passo** per sviluppatori, esposta con chiarezza (anche se a modo nostro, come si dice fra noi “Gen Z style” `😉` ).

**Fonti:** Ricerche su design tattici e meccaniche (Sinister Design【7†L146-L154】, wiki XCOM【28†L504L512】【28†L525-L529】, guide di _Divinity OS_ 【13†L104-L112】, analisi Horizon’s Gate【44†L69-L73】 【44†L112-L117】, articoli su BG3/Midnight Suns【37†L67-L75】【37†L160-L167】). Questi esempi concreti dimostrano i concetti chiave discussi.

---

## Catalogo e bilanciamento v0.1

### RefactorTactics — Catalogo e bilanciamento v0.1
#### 1. Assunzioni
- **Unreal Engine:** UE 5.6.x, C++.

- **Modalità vertical slice:** 2v2.

- **Mappa:** griglia esagonale con coordinate assiali `X` , `Y` , `Layer` .

- **Salute standard:** 100 HP.

- **Movimento standard:** 5 punti movimento.

- **Durata massima partita:** 12 turni.

- **Planning:** 30 secondi.

- **Resolution:** 6–12 secondi.

- Tutti i valori sono interi.

- I cooldown sono espressi in turni completi.

- Le animazioni non modificano gli esiti.

##### Piano standard di un eroe
Ogni eroe seleziona:

1. Un percorso di movimento.

2. Una Azione Principale.

3. Una Reazione, se disponibile.

- Il facing finale.

5. Un eventuale fallback.

##### Slot disponibili
|Slot|Quantità|Esempi|
|---|---|---|
|Movimento|1|Move|
|Azione principale|1|Attack, Dash, Guard, Heal|
|Reazione|1|Counter, Intercept, Deflect|
|Comunicazione|Illimitata con rate limit|Ping, label|
|Conferma|1|Ready|

### 2. Fasi della resolution
|Fase|Codice|Contenuto|
|---|---|---|
|Snapshot|0|Congelamento stato, intenti, seed e regole|
|Preparazione|10|Scudi, stance, trappole, reazioni preparate|
|Movimento|20|Movimento simultaneo per micro-step|

|Fase|Codice|Contenuto|
|---|---|---|
|Controllo|30|Root, push, interrupt, interposizione|
|Attacco|40|Attacchi, abilità, cure e interazioni|
|Ambiente|50|Fuoco, acqua, elettricità e propagazione|
|Cleanup|60|KO, obiettivi, cooldown e TurnLog|

Dentro la stessa fase, un valore di priorità più basso viene risolto prima.

A parità di fase e priorità si utilizza un ordinamento stabile basato su:

```
ResolutionPhase
Priority
ActionDefinitionId
SourceUnitId
EventSequence
```

Non si utilizza mai l’ordine di una `TMap` .

### 3. Azioni selezionabili
#### 3.1 Azioni fondamentali
|ID|Azione|Slot|Fase|Priorità|Range|Cooldown|
|---|---|---|---|---|---|---|
|`Action.Wait`|Wait|—|20|100|—|0|
|`Action.Move`|Movimento|20|50|5 MP|0||
|`Action.BasicAttack`|Principale|40|50|Arma|0||
|`Action.Guard`|Principale|10|40|Self|0||
|`Action.Activate`|Principale|40|70|1|0||
|`Action.Interact`|Principale|40|80|1|0||

#### Wait
L’eroe non si muove e non utilizza un’Azione Principale.

Può comunque:

- Impostare il facing.

- Preparare una Reazione.

- Mantenere una stance già attiva.

- Contestare un obiettivo.

#### Move
Segue un percorso di celle adiacenti.

##### Regole iniziali
- Budget standard: **5 MP** .

- Cella normale: 1 MP.

- Terreno difficile: 2 MP.

- Salire di livello tramite rampa: 2 MP.

- Una cella non può essere attraversata se occupata da un’unità solida.

- Il percorso non viene ricalcolato globalmente durante la resolution.

##### Fallback
Se il percorso viene bloccato:

```
L’unità si ferma nell’ultima cella valida.
```

Questa deve essere la regola standard del vertical slice.

#### Basic Attack
L’attacco base dipende dall’eroe e dalla variante arma.

Valori di riferimento:

|Tipo|Danno|Range|
|---|---|---|
|Corpo a corpo|28|1|
|Corto raggio|25|3|
|Medio raggio|22|4|
|Lungo raggio|20|6|

#### Guard
Effetto base:

- Riduce di **15** il primo danno diretto ricevuto.

- Fornisce resistenza a una spinta di 1 cella.

- Termina durante il Cleanup.

- Non protegge dagli hazard ambientali già presenti.

#### Activate
Attiva un oggetto adiacente:

- Porta.

- Consolle.

- Ascensore.

- Generatore.

- Sprinkler.

- Ponte.

- Obiettivo.

#### 3.2 Azioni di movimento
|ID|Azione|Slot|Fase|Priorità|Distanza|Cooldown|
|---|---|---|---|---|---|---|
|`Action.Sprint`|Sprint|Movimento<br>+ Principale|20|60|8 MP|0|
|`Action.Dash`|Dash|Principale|20|30|3 celle|1|
|`Action.Charge`|Charge|Principale|20/30|35|3 celle|2|
|`Action.Leap`|Leap|Principale|20|25|3 celle|2|
|`Action.Reposition`|Reposition|Principale|20|40|2 celle|1|

#### Sprint
- Fornisce 8 MP.

- Consuma Movimento e Azione Principale.

- Non permette di preparare una Reazione.

- L’eroe riceve `Status.Exposed` fino al Cleanup.

- `Exposed` aumenta di 5 il primo danno diretto ricevuto.

#### Dash
- Movimento lineare lungo una delle sei direzioni.

- Non consuma il normale percorso Move.

- Può quindi essere usato dopo il Move.

- Non attraversa muri o coperture alte.

- Non può terminare in una cella occupata.

#### Charge
- Movimento lineare massimo di 3 celle. • Infligge 20 danni al primo nemico incontrato.

- Applica Push 1.

- Si ferma dopo l’impatto.

- Viene interrotta da coperture alte, muri e porte chiuse.

#### Leap
- Ignora unità e coperture basse.

- Deve avere una cella finale valida.

- Non può attraversare un soffitto o cambiare arbitrariamente Layer.

- Ignora gli effetti delle celle intermedie.

- Subisce gli effetti della cella di atterraggio.

#### Reposition
- Movimento tattico massimo di 2 celle.

- Non applica `Exposed` .

- Non attraversa unità.

- Pensato per correzioni di posizione, non per coprire grandi distanze.

#### 3.3 Azioni offensive
- | ID | Azione | Fase | Priorità | Danno | Targeting | Cooldown |

- |---|---:|---:|---:|---|---:|

- | `Action.PrecisionAttack` | 40 | 60 | 24 | Bersaglio | 1 |

- | `Action.HeavyAttack` | 40 | 80 | 35 | Bersaglio | 2 |

- | `Action.LineAttack` | 40 | 55 | 22 | Linea | 1 |

- | `Action.CircularAoE` | 40 | 65 | 18 | Cella, raggio 1 | 2 |

- | `Action.SuppressiveLine` | 10/20 | 30 | 16 | Linea/reazione | 2 |

- | `Action.MarkTarget` | 40 | 40 | 0 | Bersaglio | 1 |

#### Precision Attack
- Range dell’arma +1.

- Ignora copertura bassa.

- Non può essere usato dopo Sprint.

- Danno base: 24.

#### Heavy Attack
- Danno: 35.

- Priorità bassa.

- Infligge 20 danni alle coperture distruttibili.

- Se interrotto prima della fase Attacco non produce effetti.

#### Line Attack
- Seleziona una direzione esagonale.

- Colpisce il primo bersaglio valido.

- Range standard: 5 celle.

- Danno: 22.

- Una copertura alta interrompe la linea.

#### Circular AoE
- Centro selezionabile entro 4 celle.

- Raggio: 1.

- Danno: 18.

- Friendly fire attivo.

- La copertura non riduce il danno se il centro dell’esplosione è dalla stessa parte della copertura del bersaglio.

#### Suppressive Line
Prepara una linea di controllo.

Trigger:

```
Un nemico entra in una cella controllata.
```

Effetti:

- 16 danni.

- Interruzione del movimento.

- Una sola attivazione.

- Il nemico resta nella cella appena raggiunta.

#### Mark Target
Applica `Status.Marked` per un turno.

Effetto iniziale:

- Il prossimo attacco alleato contro il bersaglio infligge +6 danni.

- Il marchio viene consumato.

- Non aumenta il danno ambientale.

#### 3.4 Azioni difensive e reazioni
|ID|Azione|Slot|Fase|Priorità|Effetto|Cooldown|
|---|---|---|---|---|---|---|
|`Action.Counter`|Reazione|30/40|20|Contrattacco|2||
|`Action.Intercept`|Reazione|30|10|Protezione alleato|2||
|`Action.Deflect`|Reazione|30|15|Riduzione danno|2||
|`Action.Brace`|Principale|10|30|Anti-spinta|1||
|`Action.Shield`|Principale|10|35|Scudo temporaneo|2||
|`Action.Cleanse`|Principale|30|25|Rimozione stato|2||

#### Counter
Trigger:

```
L’eroe viene colpito da un attacco diretto entro il range consentito.
```

Effetto:

- Esegue un attacco da 16 danni.

- Si attiva dopo l’attacco ricevuto.

- Non si attiva contro danni ambientali.

- Una sola attivazione.

#### Intercept
Trigger:

```
Un alleato entro 2 celle viene bersagliato da un attacco diretto.
```

Effetto:

- L’intercettore diventa il bersaglio.

- L’attacco deve avere una traiettoria compatibile.

- Non intercetta AoE.

- Non intercetta hazard.

- Una sola attivazione.

#### Deflect
- Riduce il danno diretto di 20.

- Se il danno diventa zero, l’attacco è considerato comunque avvenuto.

- Non riflette automaticamente l’attacco.

- Non funziona contro AoE ambientali.

#### Brace
- Impedisce la prima spinta.

- Riduce di 10 tutti i danni diretti fino al Cleanup.

- Blocca il movimento volontario dell’eroe.

#### Shield
- Applica 25 punti Scudo.

- Lo Scudo viene consumato prima della salute.

- Scade durante il Cleanup del turno.

- Non protegge dagli effetti di controllo privi di danno.

#### Cleanse
Rimuove un solo stato tra:

- Burning.

- Electrified.

- Rooted.

- Marked.

- Exposed.

La priorità di rimozione deve essere scelta dal giocatore durante il planning.

#### 3.5 Azioni di controllo
- | ID | Azione | Fase | Priorità | Effetto | Durata | Cooldown |

|---|---:|---:|---|---:|---:|

- | `Action.Push` | 30 | 40 | Spinta 1 | Istantanea | 1 |

- | `Action.Pull` | 30 | 40 | Trazione 1 | Istantanea | 1 |

- | `Action.Root` | 30 | 25 | Blocca movimento | 1 turno | 2 |

- | `Action.Interrupt` | 30 | 20 | Annulla azione compatibile | Istantanea | 2 |

- | `Action.Slow` | 30 | 50 | +1 costo movimento | 1 turno | 1 |

#### Regole Push e Pull
- Una unità non può terminare dentro un’altra unità.

- Una copertura alta blocca lo spostamento.

- Se la destinazione è bloccata, lo spostamento termina.

- Le collisioni non producono danno nel vertical slice iniziale.

- Le cadute vengono aggiunte con le mappe multilivello.

#### Root
- Cancella i micro-step di movimento non ancora risolti.

- Non impedisce attacchi, Guard o Activate.

- Non annulla un Teleport già risolto.

#### Interrupt
Un’azione può essere interrotta solo se dichiara:

```
bCanBeInterrupted = true
```

Non tutte le azioni sono interrompibili.

#### 3.6 Azioni di supporto e ambiente
- | ID | Azione | Fase | Priorità | Range | Effetto | Cooldown |

|---|---:|---:|---:|---|---:|

- | `Action.Heal` | 40 | 70 | 3 | Cura 20 | 1 |

- | `Action.CreateWater` | 40/50 | 60 | 4 | Acqua raggio 1 | 2 |

- | `Action.Ignite` | 40/50 | 60 | 4 | Fuoco su cella | 2 |

- | `Action.Electrify` | 50 | 30 | 4 | Elettrifica | 2 |

- | `Action.CreateCover` | 40 | 75 | 3 | Copertura | 2 |

- | `Action.ModifyArc` | 40 | 75 | 3 | Modifica collegamento | 2 |

#### Heal
- Cura 20 HP.

- Non supera la salute massima.

- Non rimuove stati.

- Può bersagliare se stessi.

#### Create Water
- Crea acqua superficiale.

- Raggio iniziale: 1.

- Durata: 2 turni.

- Applica `Wet` alle unità presenti.

#### Ignite
Crea una cella in fiamme.

- Durata base: 2 turni.

Non incendia automaticamente acqua o metallo.

- Può incendiare vegetazione, olio e gas.

#### Electrify
- Colpisce un bersaglio o una cella conduttiva.

- Propagazione massima: 3 celle.

- Danno iniziale: 20.

- Danno propagato: 12.

- Ogni unità può essere colpita una sola volta dallo stesso evento.

#### Create Cover
- Crea una copertura bassa su un bordo esagonale.

- Integrità: 30.

- Durata: 2 turni.

- Non può sovrapporsi a una copertura esistente.

#### Modify Arc
Può:

- Aprire una porta.

- Chiudere una porta.

- Creare un ponte temporaneo.

- Bloccare temporaneamente un collegamento.

- Rendere un arco conduttivo.

Ogni modifica incrementa la revisione del chunk della mappa.

### 4. Terreni disponibili
#### 4.1 Tabella del vertical slice
|ID|Terreno|Costo<br>MP|Movimento|LOS|Effetto|
|---|---|---|---|---|---|
|`Terrain.Floor`|Pavimento|1|Consentito|Libera|Nessuno|
|`Terrain.Rough`|Accidentato|2|Dash vietato|Libera|Rallentamento|
|`Terrain.ShallowWater`|Acqua bassa|2|Consentito|Libera|Wet|
|`Terrain.Fire`|Fuoco|2|Consentito|Parziale|Burning|
|`Terrain.Conductive`|Superficie<br>metallica|1|Consentito|Libera|Conduce<br>elettricità|
|`Terrain.Smoke`|Fumo|1|Consentito|Ridotta|Obscured|
|`Terrain.Ice`|Ghiaccio|1|Scivoloso|Libera|Sliding|
|`Terrain.HighGround`|Quota<br>elevata|1|Dipende<br>dagli archi|Libera|Bonus visuale|

#### Pavimento
##### Interazioni
- Movimento.

- Sprint.

- Dash.

- Posizionamento gadget.

##### Effetti
Nessuno.

#### Terreno accidentato
##### Interazioni
- Attraversare.

- Livellare.

- Distruggere ostacoli.

##### Effetti
- Costo 2 MP.

- Dash e Charge non possono attraversarlo.

- Non modifica la linea di vista.

#### Acqua superficiale
##### Interazioni
- Elettrificare.

- Congelare.

- Evaporare.

- Spostare tramite abilità.

##### Effetti
- Costo 2 MP.

- Applica `Wet` .

- Spegne `Burning` .

- Conduce elettricità.

##### Propagazione elettrica
L’elettricità attraversa celle d’acqua adiacenti entro il limite definito dall’azione.

Ordine:

```
Distanza dalla sorgente
CellId
UnitId
```

#### Fuoco
##### Interazioni
- Spegnere con acqua.

- Propagare su vegetazione.

- Alimentare con olio o gas. • Attraversare.

##### Effetti
Quando una unità entra:

- 10 danni immediati.

- Applica `Burning` .

`Burning` :

- 8 danni durante il Cleanup. • Durata: 2 turni.

- Viene rimosso da Wet.

#### Superficie conduttiva
##### Interazioni
- Elettrificare.

- Isolare.

- Collegare a dispositivi.

##### Effetti
- Non applica Wet.

- Propaga elettricità.

- Può attivare generatori o porte configurate.

#### Fumo
##### Interazioni
- Ventilare.

- Attraversare.

- Incendiare se associato a gas combustibile.

##### Effetti
- Le unità interne hanno `Obscured` .

- Range massimo di targeting dentro o attraverso il fumo: 2.

- Non blocca il movimento.

#### Ghiaccio
##### Interazioni
- Sciogliere con fuoco.

- Rompere.

- Elettrificare dopo lo scioglimento.

##### Effetti iniziali
- Movimento normale: costo 1.

- Se una unità entra con almeno 2 MP residui, scivola di una cella nella direzione di ingresso.

- Una cella bloccata impedisce lo scivolamento.

- La regola va rimandata se complica troppo il primo test.

### 5. Coperture e strutture
- | ID | Elemento | Movimento | LOS | Integrità | Protezione |

- |---|---|---|---:|---:|

- | `Structure.LowCover` | Copertura bassa | Blocca arco | Parziale | 30 | 10 |

- | `Structure.HighCover` | Copertura alta | Blocca arco | Bloccata | 50 | Totale |

- | `Structure.Door` | Porta | Variabile | Variabile | 40 | Variabile |

- | `Structure.Bridge` | Ponte | Consente arco | Libera | 40 | Nessuna |

- | `Structure.KineticPanel` | Pannello temporaneo | Blocca arco | Parziale | 30 | 10 |

#### Copertura bassa
- È associata a uno specifico bordo della cella.

- Riduce il danno diretto di 10.

- Non protegge da un attacco proveniente da un’altra direzione.

- Non protegge dagli AoE con centro sul lato protetto.

#### Copertura alta
- Blocca movimento.

- Blocca linea di vista.

- Blocca proiettili.

- Può essere distruttibile.

#### Porta
Stati:

```
Open
Closed
```

```
Locked
Destroyed
```

Ogni cambio di stato aggiorna la revisione del grafo.

#### Ponte
- Rappresenta un arco tra due celle.

- Può essere attivo, disattivo o distrutto.

- Nel vertical slice non si muove durante la resolution.

- La sua modifica avviene in modo discreto.

### 6. Equipaggiamenti
#### 6.1 Configurazione del vertical slice
Ogni eroe seleziona:

- `1 Weapon Variant`

- `1 Gadget`

- `1 Reaction Module`

Non sono presenti:

- Livelli.

- Rarità.

- Upgrade numerici.

- Equipaggiamenti casuali.

- Progressione durante la partita.

#### 6.2 Varianti arma
|ID|Variante|Vantaggio|Svantaggio|
|---|---|---|---|
|`Weapon.Precision`|Precisione|+1 range|−4 danni|
|`Weapon.Impact`|Impatto|Push 1|−1 range|
|`Weapon.Overcharge`|Sovraccarico|+6 danni|Cooldown +1|
|`Weapon.Split`|Multiplo|Bersaglio aggiuntivo|−6 danni|
|`Weapon.Suppressive`|Soppressione|Applica Slow|−5 danni|
|`Weapon.Environmental`|Ambientale|Migliora hazard|−5 danni diretto|

#### 6.3 Gadget
|ID|Gadget|Effetto|Cooldown|
|---|---|---|---|
|`Gadget.Medkit`|Medkit|Cura 18|3|
|`Gadget.BreachCharge`|Carica da breccia|35 danni struttura|3|
|`Gadget.Sprinkler`|Sprinkler|Acqua raggio 1|3|
|`Gadget.Insulator`|Isolante|Immunità a una propagazione<br>elettrica|3|
|`Gadget.SmokeEmitter`|Fumo|Fumo raggio 1|3|
|`Gadget.PortableCover`|Copertura<br>portatile|Crea copertura bassa|3|
|`Gadget.Sensor`|Sensore|Rivela area|3|
|`Gadget.Anchor`|Ancora|Impedisce una spinta|3|

#### 6.4 Moduli di reazione
|ID|Reazione|Trigger|Effetto|
|---|---|---|---|
|`Reaction.EmergencyDash`|Dash<br>d’emergenza|Bersagliato|Reposition 1|
|`Reaction.ReactiveShield`|Scudo reattivo|Subisce danno|Scudo 15|
|`Reaction.CounterShot`|Contrattacco|Colpito|14 danni|
|`Reaction.AllyIntercept`|Interposizione|Alleato bersagliato|Cambia bersaglio|
|`Reaction.HazardEscape`|Fuga hazard|Cella diventa<br>pericolosa|Reposition 1|
|`Reaction.Cleanse`|Pulizia<br>automatica|Riceve controllo|Rimuove stato|
|`Reaction.Anchor`|Ancoraggio|Riceve Push/Pull|Annulla<br>spostamento|

Ogni modulo:

- Si attiva massimo una volta per turno.

- Deve essere visibile agli alleati durante il planning.

- Non viene replicato come intento ai nemici.

- Può essere conosciuto dal nemico se fa parte del loadout pubblico pre-partita.

### 7. Configurazione degli eroi
#### Regole comuni
Ogni eroe possiede:

##### Elementi fissi
- Identità.

- Ruolo.

- Attacco base.

- Quattro abilità fondamentali.

- Affinità ambientale.

- Debolezza.

- Statistiche base.

##### Elementi configurabili
- Variante arma.

- Gadget.

- Modulo di reazione.

- Variante di una abilità.

Per il vertical slice una sola abilità per eroe può avere una variante.

### 8. Eroi del vertical slice
#### 8.1 Gadget — Tecnico della conduzione
##### Ruolo
- Attacco.

- Controllo.

- Combo elettrica.

- Disattivazione dispositivi.

##### Statistiche
|Statistica|Valore|
|---|---|
|Salute|90|
|Movimento|5|
|Range visivo|6|
|Resistenza Push|0|
|Affinità|Elettricità|

##### Abilità
|ID|Abilità|Tipo|Effetto|Cooldown|
|---|---|---|---|---|
|`Hero.Gadget.ArcPulse`|Impulso ad<br>arco|Attacco<br>base|22 danni, range 4|0|
|`Hero.Gadget.LinearDischarge`|Scarica lineare|Linea|24 danni, +8 su Wet|2|
|`Hero.Gadget.ConductiveNode`|Nodo<br>conduttore|Cella|Rende conduttiva<br>una cella|2|
|`Hero.Gadget.Overload`|Sovraccarico|AoE|18 danni, Interrupt<br>dispositivi|3|
|`Hero.Gadget.ReactiveCapacitor`|Capacitore<br>reattivo|Reazione|Scudo 15 e 10 danni<br>all’attaccante|3|

##### Variante
###### Scarica concentrata
- +6 danni.

- Non si propaga.

###### Scarica ramificata
- Può colpire un bersaglio aggiuntivo.

- −6 danni per bersaglio.

#### 8.2 Phase — Manipolatrice dell’acqua
##### Ruolo
- Supporto.

- Controllo terreno.

- Setup combo.

- Riposizionamento.

##### Statistiche
|Statistica|Valore|
|---|---|
|Salute|95|
|Movimento|5|
|Range visivo|5|
|Resistenza Push|0|
|Affinità|Acqua|

##### Abilità
|ID|Abilità|Tipo|Effetto|Cooldown|
|---|---|---|---|---|
|`Hero.Phase.PressureJet`|Getto<br>pressurizzato|Linea|16 danni, Wet, Push 1|0|
|`Hero.Phase.CircularTide`|Marea circolare|AoE|Cura alleati 18, Wet ai<br>nemici|2|
|`Hero.Phase.FluidTrail`|Scia fluida|Dash|Dash 3 e crea acqua|2|
|`Hero.Phase.MistVeil`|Velo di nebbia|AoE|Crea fumo raggio 1|3|
|`Hero.Phase.FlowReaction`|Flusso reattivo|Reazione|Reposition 1 dopo un<br>attacco|3|

##### Variante
###### Marea curativa
- Cura 24.

- Non applica Wet ai nemici.

###### Marea d’urto
- Cura 10.

- Applica Push 1 ai nemici.

#### 8.3 Riktor — Architetto del campo
##### Ruolo
- Difesa.

- Controllo spazio.

- Modifica archi.

- Protezione alleati.

##### Statistiche
|Statistica|Valore|
|---|---|
|Salute|120|
|Movimento|4|
|Range visivo|5|
|Resistenza Push|1|
|Affinità|Strutture|

##### Abilità
|ID|Abilità|Tipo|Effetto|Cooldown|
|---|---|---|---|---|
|`Hero.Riktor.ImpactShot`|Colpo cinetico|Attacco<br>base|24 danni, range 3|0|
|`Hero.Riktor.KineticPanel`|Pannello<br>cinetico|Arco|Crea copertura da<br>30 HP|2|
|`Hero.Riktor.Reconfigure`|Riconfigurazione|Arco|Sposta o ruota una<br>copertura|2|
|`Hero.Riktor.Ram`|Ariete|Charge|20 danni e Push 1|2|
|`Hero.Riktor.Interposition`|Interposizione|Reazione|Intercetta attacco<br>su alleato|3|

##### Variante
###### Pannello rinforzato
- 45 integrità.

- Durata 1 turno.

###### Pannello adattivo
- 25 integrità.

- Può essere ruotato gratuitamente una volta.

#### 8.4 Wraith — Duellante predittivo
##### Ruolo
- Assalto.

- Interruzione.

- Punizione del movimento.

- Duello.

##### Statistiche
|Statistica|Valore|
|---|---|
|Salute|100|
|Movimento|6|
|Range visivo|6|
|Resistenza Push|0|
|Affinità|Movimento|

##### Abilità
|ID|Abilità|Tipo|Effetto|Cooldown|
|---|---|---|---|---|
|`Hero.Wraith.PulseShot`|Tiro a impulsi|Attacco<br>base|21 danni, range 4|0|
|`Hero.Wraith.InterceptShot`|Tiro<br>d’intercetto|Reazione|16 danni e stop<br>movimento|2|
|`Hero.Wraith.PassingBlade`|Lama di<br>passaggio|Dash|Dash 3, 20 danni<br>attraversando|2|
|`Hero.Wraith.Deflection`|Deviazione|Reazione|Riduce danno di 20|2|
|`Hero.Wraith.Feint`|Finta|Controllo|Marca cella e ottiene<br>Reposition|2|

##### Variante
###### Intercetto preciso
- 20 danni.

- Controlla una sola cella.

###### Intercetto esteso
- 14 danni.

- Controlla una linea di 3 celle.

### 9. Loadout iniziali consigliati
|Eroe|Variante|Gadget|Reazione|
|---|---|---|---|
|Gadget|Scarica ramificata|Isolante|Scudo reattivo|
|Phase|Marea curativa|Sprinkler|Fuga hazard|
|Riktor|Pannello adattivo|Copertura portatile|Interposizione|
|Wraith|Intercetto esteso|Sensore|Dash d’emergenza|

### 10. Combo da testare
#### Acqua + elettricità
1. Phase crea acqua.

- Una unità entra o si trova nell’acqua.

3. Gadget usa Electrify o Linear Discharge.

4. La propagazione segue le celle conduttive.

5. Ogni bersaglio viene colpito una sola volta.

##### Rischio
Il friendly fire è attivo.

##### UI
- **Confermato:** bersaglio già Wet e collegamento certo.

- **Previsto:** propagazione valida nello snapshot corrente.

- **Incerto:** una cella potrebbe essere occupata o modificata durante il movimento.

#### Copertura + tiro d’intercetto
1. Riktor crea un pannello.

2. Il pannello chiude il percorso più sicuro.

3. Wraith prepara Intercept Shot.

4. Il nemico sceglie se attraversare la linea o perdere posizione.

#### Push + hazard
1. Phase o Riktor applicano Push.

- Il bersaglio entra in acqua, fuoco o area elettrificata.

- L’effetto della cella viene applicato al termine dello spostamento.

4. La propagazione ambientale viene risolta nella fase 50.

### 11. Collisioni simultanee
#### Due unità entrano nella stessa cella
##### Stessa priorità e stessa massa
- Entrambe si fermano nella cella precedente.

##### Una unità usa Charge
- Charge prevale su Move.

- L’altra unità resta nella cella precedente.

- Charge entra nella cella contesa.

##### Cella occupata da unità immobile
- L’unità in movimento si ferma prima della cella.

##### Due Charge opposte
• Entrambe si fermano prima della cella contesa. • Nessun danno aggiuntivo nella v0.1.

Queste regole evitano vantaggi nascosti legati al Player ID.

### 12. Fallback disponibili
|ID|Comportamento|
|---|---|
|`Fallback.Stop`|Si ferma all’ultima posizione valida|
|`Fallback.Wait`|Sostituisce l’azione con Wait|
|`Fallback.AttackCell`|Colpisce la cella pianificata|
|`Fallback.AttackTarget`|Segue il bersaglio se ancora valido|
|`Fallback.BasicAttack`|Usa Basic Attack sul bersaglio valido più vicino|
|`Fallback.Cancel`|Non esegue nulla|

Per il vertical slice:

- Move usa sempre `Fallback.Stop` .

- AoE usa `Fallback.AttackCell` .

- Attacchi diretti usano `Fallback.Cancel` .

- Cure usano `Fallback.Cancel` .

- Reazioni non hanno fallback.

Il targeting automatico del “nemico più vicino” va evitato inizialmente perché può generare risultati poco leggibili.

### 13. File da creare
```
Docs/
└── Design/
    └── Balance/
        ├── RT_ActionCatalog_v0.1.md
        ├── RT_TerrainCatalog_v0.1.md
        ├── RT_EquipmentCatalog_v0.1.md
        ├── RT_HeroCatalog_v0.1.md
        └── RT_TestMatrix_v0.1.md
```

Successivamente:

```
Content/
└── RefactorTactics/
    └── Data/
        ├── Actions/
        ├── Terrains/
        ├── Equipment/
        └── Heroes/
```

Codici ID consigliati:

```
PDA_Action_Move
PDA_Action_BasicAttack
PDA_Terrain_ShallowWater
PDA_Equipment_ReactiveShield
PDA_Hero_Flux
```

### 14. Test manuali minimi
|Test|Risultato atteso|
|---|---|
|Due unità entrano nella stessa cella|Entrambe si fermano|
|Move attraversa terreno accidentato|Consuma 2 MP|
|Dash incontra copertura alta|Si ferma o viene invalidato|
|Push verso cella occupata|Nessuno spostamento illegale|
|Acqua colpita da elettricità|Propagazione deterministica|
|Fuoco colpito da acqua|Fuoco rimosso|
|Basic Attack attraverso copertura bassa|Danno ridotto di 10|
|Intercept protegge alleato|Intercettore diventa bersaglio|
|AoE colpisce alleato|Friendly fire applicato|
|Target si sposta prima dell’attacco|Fallback applicato|
|Porta chiusa durante il turno|Revisione grafo aggiornata|
|Replay dello stesso turno|TurnLog e risultato identici|

### 15. Automation Test previsti
```
RefactorTactics.Actions.Move.PathBlocked
RefactorTactics.Actions.Move.CellConflict
RefactorTactics.Actions.Dash.BlockedArc
RefactorTactics.Actions.Push.InvalidDestination
RefactorTactics.Environment.WaterElectricPropagation
RefactorTactics.Environment.WaterExtinguishesFire
RefactorTactics.Cover.DirectionalDamageReduction
RefactorTactics.Reactions.Intercept
RefactorTactics.Reactions.SingleActivation
RefactorTactics.Simulation.DeterministicReplay
```

Ogni test deve eseguire la stessa simulazione con:

- Stesso snapshot.

- Stesso seed.

- Stesse definizioni.

- Stesso ordine.

- Almeno 100 ripetizioni.

Il checksum finale deve essere identico.

### 16. Debug richiesto
#### Console command futuri
```
rt.Debug.DrawGrid 1
rt.Debug.DrawPaths 1
rt.Debug.DrawCover 1
rt.Debug.DrawIntent 1
rt.Debug.DrawResolution 1
rt.Debug.DumpSnapshot
rt.Debug.DumpTurnLog
rt.Debug.VerifyReplay
```

#### Informazioni visualizzate sulle celle
```
CellId
TerrainId
TraversalCost
OccupantId
HazardTags
```

```
CoverEdges
ChunkRevision
```

#### Informazioni visualizzate sulle azioni
```
ActionId
SourceUnitId
Phase
Priority
Target
Fallback
ValidationResult
EventSequence
```

### 17. Errori da evitare
- Utilizzare float per costi, priorità o danni.

- Applicare danno tramite `AnimNotify` .

- Ricalcolare A* durante ogni micro-step senza una regola esplicita.

- Usare l’ordine di una `TMap` .

- Replicare gli intenti su `GameState` .

- Nascondere gli intenti nemici soltanto tramite UI.

- Creare una classe C++ diversa per ogni variante numerica.

- Rendere un equipaggiamento migliore in ogni parametro.

- Consentire propagazione elettrica senza limite.

- Usare selezione automatica casuale per i fallback.

- Far dipendere le collisioni dal ping o dall’ordine dei pacchetti.

### 18. Definition of Done del catalogo
Il catalogo v0.1 è completato quando:

- Ogni azione possiede un ID stabile.

- Ogni azione dichiara fase, priorità e fallback.

- Ogni terreno dichiara costo e interazioni.

- Ogni variante presenta almeno uno svantaggio.

- Le quattro identità degli eroi sono leggibili.

- La combo acqua/elettricità è deterministica.

- Le collisioni simultanee hanno una regola.

- Esiste un TurnLog verificabile.

- I test passano in Editor e packaged build.

- Nessun intento avversario viene replicato.

### 19. Commit Git
```
docs: add vertical slice gameplay balance catalog v0.1
```

w
