# Pianificazione: mappa pulita, aree a bordo, tre fasce di movimento, effetto sotto il cursore, tooltip sul nemico, ghost del piano — disegno

> **Data**: 2026-11-27. **Stato**: approvato dall'autore il 2026-11-27 ("ok, fai plan, reviw plan, implementa plan,
> review,merge e cleanup"). **Misurato su** `main` = `3ea74a4b7`.
> **Nato da** un `/sc:brainstorm` con l'autore. Le scelte sue sono in Appendice A, col testo che ha visto; ciò che
> è marcato "lettura dell'assistente" è una proposta ribaltabile.
> **Mockup che l'autore ha giudicato**: [`01-pianificazione-default.html`](../../research/design/hud/pianificazione-2026-11/01-pianificazione-default.html)
> e [`02-stili-aree.html`](../../research/design/hud/pianificazione-2026-11/02-stili-aree.html), poi
> [`03-ghost-del-piano.html`](../../research/design/hud/pianificazione-2026-11/03-ghost-del-piano.html). Si aprono in un
> browser; colori, numeri, icone e nomi delle skill sono d'esempio.
> **Decisioni**: nessuna `D-nnn` è assegnata qui. I numeri si misurano quando si scrive la riga, e valgono al merge.
>
> Convenzione: niente totali che cambiano da soli. Dove serve una misura c'è il comando o la funzione che la produce.

## 1. Perché

L'autore, alla lettera, il 2026-11-27:

1. "lavoriamo sulla UX. In particolare della grafica durante l'uso delle skill e delle informazioni che l'ambiente
   e l'interfaccia forniscono all'utente durante la pianificazione"
2. "i contorni si vedono soli tenendo premuto un particolare tasto. di default si vede l'area della skill attiva
   (fin dove colpisce e, se il target e' corretto, l'effetto (buff, debuff, danni, modifiche all'esagono, etc) o se
   il traget non e' valido un icona sull'UI. se nessuna skill attiva si vede il movimento (corsa blu, movimento
   verde, silenzioso giallo) . se non si a una skill attiva e si passa sul nemico un tooltip mostra alcune
   informazioni, decidiamo cosa potremmo mostrare."
3. "normalmente gli esagoni non sono evidenziati e i contorni non si vedono, con il tasto/contesto giusto si
   visualizzano i bordi. (dobbiamo migliorare l'estetica, vedi come hanno risolto altri giochi)."

4. Dopo aver letto la prima stesura di questo disegno: "aggiungiamo la visualizzazione ghost. il piano si deve
   presentare come il personaggio, semitrasparente, con delle icona sulla testa che fan capire cosa sta pianificando.
   i compagni di squadra devono vedere il piano e capire cosa stanno pianificando i compagni. con le giuste skill si
   hanno effetti potenziati in combinata"

Serve la riga che l'autore ha scelto il 2026-11-04 per la v0.1 giocabile: "Si usano da sole: scegliere, mirare,
vedere l'anteprima e capire l'esito (dock, targeting, tooltip) senza spiegazioni."
([`v01-giocabile-brainstorm-2026-11-04.md`](../../roadmap/plans/v01-giocabile-brainstorm-2026-11-04.md) §3.2).

**Com'è oggi, misurato nel codice** (dettaglio in Appendice C):

- Ogni area sulla mappa è un contorno esagonale per cella, a segmenti piatti (`ARTHexMapActor::DisegnaLineaAnteprima`).
- La griglia è sempre accesa: la famiglia `CellBorders` di `ARTHexMapActor`, nata con
  [#1758](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1758) perché "chi non vede il confine non
  puo' contare il movimento".
- Il ventaglio di movimento ha due colori: verde ciò che il residuo di AP paga, rosso cupo il tratto successivo
  ([D-627](../../decisions/RT_PDR_00_Decision_Log.md), [D-628](../../decisions/RT_PDR_00_Decision_Log.md) (3)). Move e
  Sprint non si distinguono sulla mappa.
- L'area colpita da una skill compare solo dopo il click
  ([#3512](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3512), aperta).
- Nessuna anteprima dell'esito: `FRTBlastPreview` porta origine, celle colpite e celle alleate. L'unico numero è il
  danno di catalogo nel tooltip dello slot.
- Passando su un'unità compare lo "sguardo" (`URTGlanceLibrary::ComposeUnit`): nome, "Guarda verso X." e gli stati.
- Il ghost del piano è un prisma piatto sulla cella, uno per voce del piano, impilati (`PlanGhosts`,
  `FRTPhasePreviewEntry::StackIndex`). Il piano di un'unità della squadra è una riga di testo sopra la testa
  ("[PIANO] Azione -> (q,r)"), con quadrati per destinazione e waypoint. La spec chiede altro da tempo: "copia 3D
  semitrasparente del personaggio" (`progettazione-hud.md` §9).

## 2. Obiettivi, non obiettivi, anti-obiettivi

**Obiettivi**

1. **Mappa pulita di norma.** Nessun esagono evidenziato e nessun contorno finché un contesto non lo chiede.
2. **Aree a bordo luminoso.** Un'area è una linea luminosa lungo il suo perimetro, con un bagliore verso l'interno e
   niente riempimento. Il terreno sotto resta del suo colore.
3. **Bordo del singolo esagono solo dove serve**: sotto il cursore, lungo il percorso, sulle celle colpite. Tenendo
   premuto un tasto compaiono i bordi di tutti gli esagoni.
4. **Tre fasce di movimento** con un'unità selezionata e nessuna skill attiva: gialla silenzioso, verde movimento,
   blu corsa.
5. **Skill attiva**: la portata; sotto il cursore le celle colpite; sul bersaglio valido l'effetto previsto, calcolato
   col piano della propria squadra; sul bersaglio non valido un'icona.
6. **Tooltip sul nemico** senza skill attiva, con le righe scelte dall'autore.
7. **Il ghost del piano**: il personaggio semitrasparente dove agirà, con sopra la testa le icone di ciò che ha
   pianificato. Tutta la squadra vede i piani della squadra, e una combo fra due azioni si riconosce.

**Non obiettivi**

- Ciò che si vede quando la skill **si risolve**: effetti, suono, camera. L'autore lo ha messo in coda ("poi dobbiamo
  occuparci della grafica"): è il prossimo brainstorm, e l'owner di oggi è l'ondata "Skill viste e sentite".
  *Lettura dell'assistente: "la grafica" è da definire in quel brainstorm.*
- La Ghost Timeline come widget dell'HUD e lo scrubbing delle fasi
  ([#172](https://github.com/DegrassiAaron/refactor-tactics-main/issues/172)), il selettore del verso.
- Le pose d'autore per azione. Il ghost della prima versione ha una posa ferma.
- Un overlay di minaccia sulla mappa
  ([#536](https://github.com/DegrassiAaron/refactor-tactics-main/issues/536), v0.3). Qui la minaccia è una riga di testo.
- La pelle del tooltip. Resta il riquadro provvisorio di oggi: l'aspetto appartiene a "la grafica".
- Qualunque regola. Costi in AP, bande, tetti, `Sneak` dichiarato, validazione del piano non cambiano.
- I segni del playback in risoluzione (anelli, traiettorie, croci): restano come sono.

**Anti-obiettivi** (lo renderebbero un fallimento anche funzionando)

- **Una fuga di informazione.** Un nemico o un terreno che la squadra non conosce non deve comparire attraverso
  l'effetto, le celle colpite, il tooltip o un tempo di calcolo diverso.
- **Una seconda verità.** A nemici fermi, l'effetto mostrato deve essere quello che il resolver applica.
- **Un what-if che tocca la partita.** Nessuna scrittura su stato, snapshot, TurnLog, `StateHash`, replay.
- **Uno scatto del frame** a ogni movimento del cursore.
- **L'albero di Natale.** Più segni insieme di quanti il giocatore ne legga
  ([`progettazione-hud.md`](../../technical/systems/progettazione-hud.md) §15.1).

## 3. Vincoli

- **La simulazione decide, la presentazione mostra** ([`AGENTS.md`](../../../AGENTS.md) §3). Ogni giudizio sta in una
  funzione pura con il suo test; il widget e l'attore posano il risultato. Un `UUserWidget` non ha copertura headless
  ([D-320](../../decisions/RT_PDR_00_Decision_Log.md)).
- **Privacy** ([`CLAUDE.md`](../../../CLAUDE.md) §7). Un nemico non osservato risponde come una cella vuota, carattere per
  carattere (`progettazione-hud.md` §18.0). La minaccia si calcola dalle capacità note, mai dall'intento
  ([D-620](../../decisions/RT_PDR_00_Decision_Log.md) (8a)). ⚠️ `RTPlanningSnapshot::For` chiede lo stato
  all'autorità: offline è lo stato **vero**, non la vista della squadra. Il filtro oggi lo applicano le singole guardie.
- **`Sneak` si dichiara, e ciò che dichiara è un tetto** ([D-425](../../decisions/RT_PDR_00_Decision_Log.md),
  [D-457](../../decisions/RT_PDR_00_Decision_Log.md), [D-609](../../decisions/RT_PDR_00_Decision_Log.md)). `Move` e
  `Sprint` si leggono dal costo del percorso in Movement Point ([D-629](../../decisions/RT_PDR_00_Decision_Log.md)).
- **I moltiplicatori dei profili** sono `Withdraw` ×0,25 · `Sneak` ×0,5 · `Move` ×1 · `Sprint` ×2
  ([D-412](../../decisions/RT_PDR_00_Decision_Log.md)). Oltre il tetto dello Sprint nasce un tratto concatenato a 2 AP
  ([D-625](../../decisions/RT_PDR_00_Decision_Log.md) (1)).
- **La palette degli overlay la determina il gate** ([D-364](../../decisions/RT_PDR_00_Decision_Log.md),
  [D-368](../../decisions/RT_PDR_00_Decision_Log.md)): `RefactorTactics.AreaOverlay.PaletteIsDistinguishable`.
- **L'hover annuncia ciò che il click esegue** ([D-128](../../decisions/RT_PDR_00_Decision_Log.md)).
- **Un asset Unreal è binario**: il materiale nuovo si autora nel clone principale, in Editor, e il motore è uno per
  macchina (`AGENTS.md` §7, §11).

**Decisioni e issue che questo disegno tocca**

| Che cosa | Oggi | Con questo disegno |
|---|---|---|
| Griglia sempre accesa (#1758) | `CellBorders` visibile in partita | spenta di norma; accesa col tasto. **Emenda** |
| Terreno ricordato, la "pianta" (D-520) | anello ciano e fondo scuro, nell'anello della griglia | **invariato**: resta sempre, anche a griglia spenta |
| Codice di piano sul bordo della cella (D-508) | nero sul piano attivo, bianco sopra, viola sotto, sempre | si vede solo col tasto. **Emenda** |
| Colori del ventaglio (D-627, D-628 (3)) | verde il pagabile, rosso cupo il tratto successivo | tre fasce per profilo; il rosso cupo resta per ciò che gli AP non pagano. **Emenda la resa** |
| Portata a tre stati (D-561) | viola, rosa, ambra tratteggiata, per cella | stessi significati e colori, come bordi d'area |
| Area sotto il cursore (#3512) | senza produttore | costruita qui, con le sue regole |
| Sguardo all'hover (#3747, D-513) | nome, verso, stati | righe nuove per un nemico osservato |
| Ghost del piano (#172, `PIE-HUD-AP-STACKS`) | un prisma piatto per voce, impilati | il personaggio semitrasparente, uno per tappa, con un'icona per voce. **Emenda la resa** |
| Piano di un'unità della squadra sul Canvas | testo "[PIANO] …", quadrati | ghost e icone |
| Intenti di un compagno che non si comanda (D-478) | `FUTURE` | **invariato**: questa fetta copre le unità comandate |
| Motivo `OverBudget` all'hover (D-625 (2), D-631) | testo "Non si muove: …" | invariato |

## 4. Approcci

### 4.1 Come si disegna l'area: tre stili, mostrati all'autore

Mockup [`02-stili-aree.html`](../../research/design/hud/pianificazione-2026-11/02-stili-aree.html).

| Stile | Dà | Costa |
|---|---|---|
| **A · Solo il bordo** | la mappa più pulita; fuoco, acqua e coperture restano del loro colore | dentro l'area le celle non si contano a occhio: serve il mouse o il tasto |
| B · Bordo e punti | le celle si contano senza tasto | con la corsa i punti sono tanti e fanno rumore |
| C · Bordo e velo | le fasce si leggono al primo sguardo | il velo tinge il terreno |

**Scelta dell'autore: A.**

### 4.2 Con che cosa si disegna il bordo

| Approccio | Dà | Costa |
|---|---|---|
| A. Restare sui segmenti del line batcher | nessun asset nuovo | niente bagliore verso l'interno; l'aspetto resta quello di oggi |
| **B. Un piano esagonale per cella, a istanze, con un materiale che accende i lati indicati** | un solo pezzo per bordo d'area, bordo di cella, celle colpite e griglia; segue i dislivelli perché ogni istanza sta sulla sua cella; la mappa disegna già così celle, griglia e velo | un materiale nuovo da autorare in Editor |
| C. Contorno in post-process | il bordo esce da solo | spessore in pixel di schermo; si somma al post-process che contorna le unità e a quello dell'ambiente (E52) |

**Scelta dell'autore: B.**

### 4.3 Che cosa legge il numero dell'effetto

| Lettura | Dà | Costa |
|---|---|---|
| 1. Il campo com'è adesso | semplice, sempre spiegabile | non vede le combo preparate in quel turno |
| **2. Il campo col piano della propria squadra** | rende visibili le combo | più lavoro; il numero cambia quando si ritocca il piano di un'altra unità |

In entrambe i nemici sono fermi: il loro piano non si conosce. **Scelta dell'autore: la 2.**

### 4.4 Come si calcola l'effetto

| Approccio | Dà | Costa |
|---|---|---|
| **A. Far girare il resolver vero su una copia**, in un mondo isolato, come fa lo Scenario Harness | giusto per costruzione: nessuna seconda versione delle regole | costo a giro non misurato; va ricalcolato solo quando cambia qualcosa |
| B. Un calcolatore dedicato: funzioni pure del danno più gli effetti dichiarati, applicati nell'ordine del resolver | veloce | riscrive l'ordine degli effetti: può divergere su reazioni, spinte, terreno |
| C. Estrarre il resolver da `ARTTurnManager` ([#1818](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1818)) | la risposta giusta a lungo termine | troppo grande per questo lavoro |

**Scelta dell'autore: A, con la misura del costo come primo passo.** Se un giro costa troppo si ripiega su B, con un
test di parità contro il resolver vero. "Com'è adesso" è lo stesso meccanismo col piano vuoto.

### 4.5 Dove stanno il ghost e le icone

Mockup [`03-ghost-del-piano.html`](../../research/design/hud/pianificazione-2026-11/03-ghost-del-piano.html).

| Modo | Dà | Costa |
|---|---|---|
| **A · Un ghost per tappa**, con le icone di ciò che l'unità fa lì | si vede da dove parte ogni colpo | più sagome sulla mappa |
| B · Un ghost solo, a fine turno, con tutte le icone | una sagoma per unità | il colpo sembra partire dalla cella finale |
| C · Icone sull'unità vera, ghost solo dove finirà | sai cosa farà guardando il personaggio | le icone si sommano a nome, barra e stati |

**Scelta dell'autore: A.**

## 5. Il disegno

Sei fette, in quest'ordine. Ognuna si chiude con una tavola giudicata dall'autore.

### 5.1 Fetta 1 — Il disegno delle aree e i bordi a contesto

**Che cosa si vede**

- Senza unità selezionata: niente.
- Un'area è il suo perimetro: una linea luminosa che segue i lati degli esagoni, con un bagliore che sfuma verso
  l'interno dell'area. Nessun riempimento.
- Due aree che condividono un lato: vince la più interna (la più specifica).
- Il bordo del **singolo esagono**, sottile, compare in tre contesti:
  - sotto il cursore (prende il posto del contorno giallo di oggi, in un colore neutro);
  - lungo il percorso che si sta componendo o che è pianificato, accanto a una linea bianca continua;
  - sulle celle colpite da una skill.
- Tenendo premuto il tasto compaiono i bordi di **tutti** gli esagoni della mappa: quelli dentro un'area nel colore
  dell'area, gli altri in un colore neutro. Rilasciando spariscono.

**Regole**

- **Un solo pezzo.** Una famiglia di istanze, un piano esagonale per cella, con dati per istanza: colore, quali dei
  sei lati sono accesi, stile del lato (luminoso, sottile, tratteggiato), eventuale velo. Il materiale disegna i lati.
- **Chi decide i lati è una funzione pura**: dato un insieme di celle, per ogni cella i lati il cui vicino non è
  nell'insieme. Il vicino è quello dello stesso `Layer`; una cella senza vicino su quel lato ha il lato acceso.
- **Tutte le aree passano al pezzo nuovo senza cambiare significato né colore**: movimento, tratto non pagato,
  portata a tre stati (D-561), celle colpite e fuoco amico, aree di movimento del nemico a fuoco, arco di vista del
  nemico. Quando un significato passa al pezzo nuovo, il suo disegno a segmenti si toglie: mai due disegni per lo
  stesso significato.
- **Le linee restano linee**: percorso, mira, tiro interrotto, confine di ciò che la squadra vede.
- **La griglia** (`CellBorders`) resta la famiglia che è. Di norma si spegne **sulle celle in vista**; col tasto si
  accende su tutte.
- **Il terreno ricordato resta sempre com'è oggi**, anello ciano e fondo scuro (D-520): è il segno di "lo ricordo,
  non lo vedo". *Scelta dell'autore, Appendice A.6.* ⚠️ La pianta vive nell'anello della griglia, e
  `ARTHexMapActor::SetCellBordersVisible(false)` spegnerebbe anche lei: la visibilità va decisa per cella (in vista o
  ricordata), non per l'intera famiglia.
- **Il codice di piano** (D-508: bordo nero sul piano attivo, bianco sopra, viola sotto) **si vede solo col tasto**.
  Di norma nessuna cella in vista ha il bordo, su nessun piano; i piani si distinguono dalla tinta dei volumi e
  dall'indicatore "Piano N di M". *Scelta dell'autore, Appendice A.6.* Il contorno delle unità e dei ghost per piano
  non cambia.
- **Il tasto.** *Lettura dell'assistente*: `Maiusc`, tenuto premuto. `Alt` è escluso: `Alt+Invio` passa a schermo
  intero (`bAltEnterTogglesFullscreen`) e `Invio` è Conferma.
- Tutto questo vale in Pianificazione. In risoluzione le aree di pianificazione sono spente come oggi.

**Come si chiude**

- La funzione dei lati ha test headless: area convessa, area con un buco, due aree annidate, bordo di mappa, due piani.
- `RefactorTactics.AreaOverlay.PaletteIsDistinguishable` verde.
- Nessun significato è disegnato due volte (un test conta i produttori per significato).
- Una voce `PIE-*` nuova in [`test-manuali-pie.md`](../../technical/test-manuali-pie.md), con una domanda: "senza
  tasto, capisci dove finisce ogni area? col tasto, conti le celle?"

### 5.2 Fetta 2 — Le tre fasce di movimento

**Che cosa si vede**, con un'unità comandata selezionata e nessuna azione a bersaglio armata:

| Fascia | Colore (famiglia) | Celle |
|---|---|---|
| silenzioso | giallo | costo del percorso entro il tetto di `Sneak` (×0,5 del movimento base) |
| movimento | verde | oltre, fino a ×1 |
| corsa | blu | oltre, fino a ×2 |

- Le fasce sono annidate, ciascuna col suo bordo.
- Il costo è quello che il click pagherebbe: i waypoint già posati più il passo fino alla cella, la stessa somma che
  oggi divide il ventaglio (`ReachableCellsAfterPlan` in `RTPlanningPreview.cpp`).
- **Sotto il cursore**, accanto alla cella, un'etichetta dice **ciò che il click farebbe** (D-128) e quanto costa.
  Per la corsa dice anche che cosa comporta, letto dai dati del profilo (oggi: Esposto per due turni, nessuna
  reazione). Dentro la fascia gialla, senza `Sneak` dichiarato, dice "movimento" e aggiunge che lì si arriva anche in
  silenzio dichiarandolo; con `Sneak` dichiarato dice "silenzioso". I pip degli AP continuano a mostrare il costo
  come oggi.

**Regole**

- **La fascia gialla informa, non dichiara.** Cliccare dentro il giallo non rende il movimento silenzioso: `Sneak`
  resta dichiarato col tasto o col badge (D-425, D-457). *Scelta dell'autore, Appendice A.6.*
  La ragione è misurata in D-425: quasi tutti i movimenti del corpus erano di una o due celle, e un `Sneak` derivato
  dalla distanza avrebbe reso silenzioso quasi ogni passo corto.
- **Con `Sneak` dichiarato** si vede la sola area gialla, che è il suo tetto.
- **Con `Withdraw` imposto** (Overwatch armato) si vede la sola area del suo tetto. *Lettura dell'assistente*: in
  verde, perché è un movimento ordinario più corto.
- **Oltre la corsa**: ogni tratto concatenato che il residuo paga ha un bordo blu **tratteggiato**. *Lettura
  dell'assistente.*
- **Ciò che gli AP non pagano** tiene il rosso cupo di D-627, come bordo: è il tratto successivo a quelli pagati,
  come vuole D-628 (3). Se il residuo non paga nemmeno il primo tratto, tutto il ventaglio è rosso cupo. Il motivo
  `OverBudget` all'hover resta com'è (D-625 (2), D-631).
- **Con una skill a bersaglio armata** le fasce spariscono e compare la portata, come oggi fra ventaglio e portata.
- **La palette.** Servono due significati nuovi nella palette degli overlay, per il silenzioso e per la corsa. Le
  famiglie di colore sono dell'autore; i valori esatti li fissa il gate. Oggi il giallo `#FFD60A` è il cursore e
  l'azzurro `#32ADE6` è il confine della vista: il cursore passa a un colore neutro (5.1), e il blu della corsa deve
  distare dall'azzurro quanto il gate chiede.

**Come si chiude**

- Test headless: una cella per fascia su una mappa con costi diversi (una cella cara sposta la fascia, come in
  `Spec.Map.ConstrainedCellCostsMore`); `Sneak` dichiarato; residuo che non paga.
- Per ogni cella del ventaglio, il profilo che l'etichetta nomina è quello che il resolver applica. La fascia gialla
  si verifica come costo entro il tetto di `Sneak`, non come profilo.
- Voce `PIE-*`: "le tre fasce si leggono come tre cose diverse? sai cosa ti costa la corsa prima di cliccare?"

### 5.3 Fetta 3 — Celle colpite sotto il cursore, e icona del bersaglio non valido

È la [#3512](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3512): si costruisce quella, con le sue
regole e la sua Definition of Done. Qui stanno solo le aggiunte.

- **Bersaglio che il click accetterebbe**: le celle colpite hanno il bordo di cella nel colore dell'attacco; un
  alleato dentro l'area prende il colore del fuoco amico, come oggi dopo il click.
- **Bersaglio rifiutato**: nessuna cella colpita. Compare un'icona di divieto in due posti:
  - accanto al cursore, con una parola per il motivo (fuori portata, tiro interrotto, altro piano, troppo vicino);
  - sulla skill nella barra, che è lo stato `Non valida` di oggi.
- **Il predicato è uno, ed esiste già**: l'icona accanto al cursore compare se e solo se lo slot è `Non valida` per
  ciò che sta sotto il cursore ([D-459](../../decisions/RT_PDR_00_Decision_Log.md)). *Lettura dell'assistente, dopo
  il controllo del costruttore.* Ne segue che con un'azione a bersaglio unità e il cursore su una cella vuota non
  compare niente, come oggi: come annunciare quel rifiuto resta di
  [#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705).
- **Dopo la dichiarazione del bersaglio** vale il comportamento di oggi, senza cambi.
- **Fuori portata**: una linea tratteggiata dall'origine di mira alla cella. *Lettura dell'assistente.*
- I motivi sono quelli che il click già produce (`ERTHexTargetReason`, `RefusalForObserver`): nessun motivo nuovo, e
  nessun motivo che un nemico non osservato renderebbe diverso da una cella vuota.
- Il testo in basso dopo un click rifiutato resta com'è.

**Come si chiude**: la DoD di #3512, più un test che l'icona accanto al cursore compare se e solo se lo slot è
`Non valida`, più la sua voce PIE.

### 5.4 Fetta 4 — L'effetto sul bersaglio

**Che cosa si vede**, con un'azione armata e il cursore su un bersaglio che il click accetterebbe:

- Accanto a ogni unità dentro le celle colpite, un cartellino con il bordo **tratteggiato** (è una previsione,
  `progettazione-hud.md` §16):
  - il danno, o la cura, o lo scudo dato;
  - gli stati applicati o tolti, con la durata;
  - lo spostamento forzato, con una sagoma tratteggiata sulla cella d'arrivo;
  - "KO" se la previsione porta a zero;
  - una riga per ogni bonus che viene dal piano (per esempio "Bagnato +80"), dal breakdown del danno;
  - in fondo, piccolo: "previsto: se resta lì".
- Sulla barra della vita dell'unità, il tratto che perderebbe entro la fine del turno è rigato.
- Sulle celle: ciò che cambierebbe (superficie, copertura creata o spostata, porta), come segno tratteggiato.
- Un alleato colpito ha il cartellino nel colore del fuoco amico.
- Dopo il click le celle colpite restano; il cartellino torna quando il cursore passa di nuovo sul bersaglio o su una
  cella colpita. *Lettura dell'assistente*: tenerli tutti accesi riempirebbe la mappa.

**Regole**

- **Il numero del danno è quello che il playback mostrerà** sopra l'unità: `FRTResolvedEvent::Amount`, la stessa
  convenzione del token di danno (`progettazione-hud.md` §4.2.1). La barra rigata mostra invece la vita che resterebbe
  davvero, scudo compreso. *Lettura dell'assistente*: così anteprima e risoluzione raccontano lo stesso colpo con lo
  stesso numero.
- **Una sola fonte.** Da questa fetta le celle colpite e i cartellini vengono dallo **stesso** giro di what-if: le
  celle sono quelle dell'evento di impronta dell'azione (`AttackFootprint`), non una geometria calcolata a parte.
  Così, se il piano sposta qualcuno prima del colpo, celle e cartellini dicono la stessa cosa. Nella fetta 3 le celle
  vengono da `MakeBlastPreview`; quando il piano non sposta nessuno le due fonti devono coincidere, e lo misura il
  test di parità. La regola "uguale prima e dopo il click" di #3512 resta vera, perché hover e click leggono la stessa
  fonte. *Lettura dell'assistente, dopo il controllo del costruttore: segue dalla scelta "col tuo piano".*
- **A chi va il cartellino.** A chi il what-if colpisce. Si ancora all'unità dove sta adesso; se il piano la sposta
  prima del colpo, una sagoma tratteggiata dice dove sarà. Un'unità che sta adesso in una cella disegnata e che il
  what-if non colpisce non ha cartellino.
- **Il cartellino è di un'azione, la barra è del turno.** Le righe del cartellino sono gli eventi dell'azione a
  fuoco: quella armata sotto il cursore, oppure, dopo il click, la principale a fuoco dell'unità selezionata (la
  stessa di cui oggi si disegnano area e mira). La barra rigata mostra invece la vita a fine turno con tutto il piano
  più l'azione candidata, e "KO" compare se a fine turno l'unità è caduta. Quindi il servizio restituisce sia gli
  eventi attribuiti per azione sia le differenze di stato finali. *Lettura dell'assistente, dopo il controllo del
  costruttore.*
- **Il piano della squadra** sono le azioni pianificate di tutte le unità che il giocatore comanda, più l'azione
  sotto il cursore. In una partita con compagni umani saranno anche i loro intenti autorizzati: non fa parte di
  questo lavoro.
- **I nemici sono fermi.** Niente azioni, niente reazioni armate, niente Guardia: il loro piano è privato. Per questo
  il cartellino dice "previsto".
- **Il servizio di what-if** riceve la vista della squadra, il piano e l'azione candidata; restituisce, per unità e
  per cella, ciò che cambierebbe. Lo calcola facendo girare **il flusso vero del turno** in un mondo isolato, per la
  via dello Scenario Harness (`URTScenarioRunner`), e leggendo eventi risolti e differenze di stato
  (`FRTResolvedEvent`, `FRTTestResult::StateDiff`).
- **La copia nasce dalla vista della squadra.** Contiene solo le unità nemiche che la squadra osserva adesso e il
  terreno come la squadra lo conosce, come già vale per il costo del percorso (D-628, lettura (b)).
- **Si ricalcola quando cambia un ingresso**: piano, azione armata, cella sotto il cursore. Mai a ogni frame. Il
  risultato per (piano, azione, cella) si conserva finché il piano non cambia.
- **Il mondo del what-if non scrive niente della partita**: stato, snapshot, TurnLog, `StateHash`, archivio del
  replay, log del giocatore.

**Il primo passo è una misura.** Quanto costa un giro di what-if di un turno, sui kit del roster, sulla macchina di
riferimento (`FRTTestResult::WallClockSeconds`, più la creazione del mondo).

- *Lettura dell'assistente*: il tetto è 10 ms per ricalcolo.
- Sopra il tetto, in ordine: tenere vivo un solo mondo di what-if per tutta la Pianificazione invece di crearne uno a
  giro; poi il ripiego sull'approccio B di §4.4, con un test di parità contro il resolver vero.
- La misura e la scelta che ne segue si scrivono nel referto della fetta, col commit.

**Come si chiude**

- **Parità**: su un corpus di scenari a nemici fermi, l'effetto previsto coincide con gli eventi che il turno vero
  produce. Il corpus copre almeno un caso per tipo di effetto di `ERTActionEffect` usato dai kit e una combo fra due
  unità della squadra (per esempio `Visual.Combat.WaterElectricCoordinated`).
- **Privacy, mondi gemelli**: stessa scena con un nemico osservato, con lo stesso nemico non osservato, con la cella
  vuota. Le ultime due danno uscite identiche; un nemico non osservato sulla linea non compare nell'effetto. La
  mutazione "what-if dallo stato vero" fa diventare rosso il test.
- **Isolamento**: `StateHash`, TurnLog e archivio del replay identici prima e dopo una raffica di what-if.
- **Costo**: la misura è scritta e sta sotto il tetto, oppure il referto dice quale ripiego è stato preso.
- Voce `PIE-*`: "il numero che vedi prima del click è quello che vedi dopo, a nemico fermo? la combo si vede?"

### 5.5 Fetta 5 — Il tooltip sul nemico

**Che cosa si vede**, senza azione armata, fermando il cursore su un nemico che la squadra osserva:

| Riga | Esempio | Fonte |
|---|---|---|
| nome | "Riktor" | come oggi |
| stati, con cosa fanno e quanti turni restano | "Rallentato, 1 turno: movimento ridotto" | stati visibili e glossario, come oggi, più la durata |
| fin dove può colpire | "Colpisce fino a 5 celle (Colpo a impatto)" | la portata massima fra le azioni del suo kit di catalogo |
| quanto può muoversi | "Si muove di 4 celle, di corsa 8" | il movimento base, che già alimenta le aree del nemico a fuoco |
| skill che gli hai visto usare | "Ariete al turno 2: forse di nuovo pronta" | la stima che oggi sta nella scheda al click |

**Regole**

- È lo sguardo di oggi (`URTGlanceLibrary::ComposeUnit`) con righe nuove per un nemico osservato. Ritardo,
  ancoraggio e resa non cambiano.
- **Per un nemico la riga "Guarda verso X." esce**: l'autore non l'ha scelta. Il verso resta leggibile dall'unità.
  Per un'unità alleata lo sguardo non cambia. *Lettura dell'assistente.*
- **La stima della ricarica non è mai un conto dei turni che restano** (D-562, punto (11)): usa le parole della scheda.
- **Mai**: il piano, l'azione armata, le ricariche vere, se una reazione è armata.
- Kit ed equipaggiamento di un nemico osservato sono pubblici (decisione d'autore del 2026-10-20,
  `progettazione-hud.md` §18.1): la portata viene da lì.
- Un nemico non osservato risponde come una cella vuota, come oggi.
- **Se le righe non entrano**: nome, colpisce, si muove e visto usare restano; si accorciano gli stati, con la riga
  "E altri N stati." di oggi. Il tetto delle righe (`MaxUnitLines`) va alzato di conseguenza. *Lettura dell'assistente.*
- **Con un'azione armata** lo sguardo su un nemico non compare: il suo posto lo prende il cartellino dell'effetto.
- Le righe che l'autore ha lasciato fuori: PV e scudo in numeri, difese, verso, copertura, reazioni del kit, ultimo
  turno, quota.

**Come si chiude**

- Test headless su `ComposeUnit`: le righe, l'ordine, il taglio.
- Mondi gemelli: nemico non osservato e cella vuota danno la stessa uscita.
- `PIE-HUD-SGUARDO` aggiornata con le righe nuove.

### 5.6 Fetta 6 — Il ghost del piano

**Che cosa si vede**, in Pianificazione, per ogni unità della squadra che ha un piano:

- **Una tappa** è una cella in cui l'unità si troverà dopo una voce del piano e che non è quella dove sta adesso:
  l'arrivo di uno scatto, la fine del movimento.
- **Su ogni tappa c'è un ghost**: il personaggio stesso, semitrasparente, girato nel verso previsto per quel momento.
- **Sopra la testa del ghost, le icone** delle voci che la vedono lì: lo scatto che ce la porta e i colpi che partono
  da quella cella; sul ghost di fine turno, il movimento. Un'azione d'ambiente, che risolve dopo il movimento, sta sul
  ghost di fine turno anche se la sua mira parte da dove è stata pianificata
  ([D-470](../../decisions/RT_PDR_00_Decision_Log.md)): è il solo caso in cui l'icona non sta sulla cella da cui parte
  il colpo.
- **Ciò che l'unità fa da dove sta adesso** (un'azione di preparazione, un colpo senza scatto) ha le icone sopra il
  personaggio vero. Un'unità che non si muove non ha ghost.
- **Le icone** sono quelle che la skill ha nella barra, col colore della fase, nell'ordine in cui il turno le risolve.
  Un'azione messa più volte nel piano ha un'icona per volta.
- **L'unità selezionata** ha ghost e icone pieni; le altre della squadra più tenui.
- **Un'icona dice anche se la voce accadrà.** Una voce col bersaglio rifiutato porta il segno di divieto che lo slot
  `Non valida` ha nella barra. Una voce che il turno non eseguirà (uno scatto che non si applica, un movimento negato)
  ha l'icona attraversata da un taglio, come il fumetto barrato in risoluzione, e non genera un ghost. *Lettura
  dell'assistente, dopo il controllo del costruttore.*
- **Una reazione armata** non è una fase: ha la sua icona sopra il personaggio vero, staccata dalle altre e nel colore
  delle reazioni. *Lettura dell'assistente*, sulla regola di #172 ("la reaction non è una quinta fase").
- **La combo.** Quando due azioni del piano della squadra si potenziano, le due icone hanno un anello dorato e un filo
  che le lega; accanto a quella potenziata c'è il guadagno (per esempio "+80"). *Lettura dell'assistente, mostrata
  all'autore nel mockup e non corretta.*
- Percorso e scatto restano linee, come oggi.
- **Celle colpite e mira** si disegnano per ogni unità della squadra, quelle della sua principale a fuoco: piene per
  l'unità selezionata, tenui per le altre, come nel mockup. *Lettura dell'assistente, dopo il controllo del
  costruttore*: oggi si disegnano per la sola unità selezionata.

**Regole**

- **Il ghost non calcola niente** (`progettazione-hud.md` §9.1). Tappe, ordine, verso e certezza vengono dal produttore
  che esiste, `FRTPhasePreviewEntry` in `RTPlanPreview`: una tappa raggruppa le voci che hanno la stessa
  `PreviewDestination`, cioè la cella dove l'unità si trova quando la voce è risolta, e l'ordine delle icone è quello
  che `StackIndex` porta già. Le voci la cui `PreviewDestination` è la cella dove l'unità sta adesso hanno le icone
  sul personaggio vero. D-470 non cambia.
- **Che cos'è una combo.** L'azione A è in combo con l'azione B di un'altra unità della squadra se, togliendo B dal
  piano, l'effetto previsto di A **cresce**: più danno a un nemico, più cura o scudo a un alleato, uno stato o un KO
  in più su un nemico. Il guadagno scritto accanto è la differenza di danno o di cura; se cambia solo uno stato o un
  KO, è il suo nome. Un'azione che ne **indebolisce** un'altra non è una combo e non ha l'anello: l'effetto più basso
  si legge comunque nel cartellino della fetta 4. *Lettura dell'assistente, dopo il controllo del costruttore: l'autore
  ha parlato di "effetti potenziati".* Si ricava dal servizio di what-if della fetta 4:
  nessuna tabella di coppie e nessun ramo per eroe (`AGENTS.md` §3, invariante 16). Chi costruisce può ottenerlo con
  meno giri, purché un test lo confronti con questa definizione.
- **Si ricalcola quando cambia un piano della squadra**, non a ogni frame, e i ghost si riusano da un pool: nessun
  attore creato per ogni anteprima (budget di #172).
- **Chi vede cosa, in questa fetta.** Ghost e icone si disegnano per **tutte le unità che il giocatore comanda**, non
  solo per la selezionata. In v0.1 sono tutta la squadra
  ([D-478](../../decisions/RT_PDR_00_Decision_Log.md)). Il produttore, che oggi gira per la sola unità selezionata
  leggendone il piano, gira per ciascuna unità comandata.
- **Il compagno che non si comanda resta `FUTURE`, come dice D-478.** Il disegno è lo stesso e lo aspetta, ma la sua
  porta non si apre qui: la vista degli intenti (`FRTIntentView`) oggi porta nomi di testo e un solo bersaglio, e
  per tappe e icone andrebbe estesa. È un cambio del confine di privacy, con i suoi test, e in v0.1 non ha un caso da
  guardare a schermo. *Lettura dell'assistente, dopo il controllo del costruttore, su D-478.*
- **Il piano nemico non si vede.** Il nemico rivelato da `Status.Reveal` resta com'è oggi, con la sua etichetta: non
  entra in questa fetta. Nel sorgente nessuna azione concede oggi quello stato, quindi a schermo non si verificherebbe.
- **Un ghost non è mai un bersaglio**: dove copre una cella o un'unità vince ciò che sta sotto
  (`PlayerInput.GhostIsNeverAGameplayTarget`).
- **La certezza non si gradua sul ghost.** Un ghost nasce solo da uno spostamento, che nel produttore è sempre
  "incerto": un elemento a un solo livello non si gradua (`progettazione-hud.md` §16). La trasparenza dice soltanto
  selezionata o non selezionata.
- **Che cosa sostituisce.** I prismi (`PlanGhosts`, `PlanGhostBorders`) e, per le unità della squadra, l'etichetta
  "[PIANO] …" con i quadrati di destinazione. Quando il ghost nuovo li copre, i vecchi disegni si tolgono.
- **La posa** della prima versione è una sola, ferma, per ogni eroe. *Lettura dell'assistente*: le pose per azione
  sono lavoro d'autore e restano fuori.
- **Dipendenza**: il segno della combo vuole la fetta 4. Ghost e icone no.

**Come si chiude**

- Test headless sul raggruppamento in tappe e sull'ordine delle icone: scatto più colpo più movimento; colpo da fermo;
  due copie della stessa azione; unità senza piano.
- L'anti-fuga resta quello di `RefactorTactics.Preview.TimelineCarriesOnlyThePlanningUnit`, esteso al consumatore
  nuovo: nessuna tappa e nessuna icona per un nemico non rivelato.
- La combo: un test con e senza l'azione del compagno, sul caso acqua più elettricità
  (`Visual.Combat.WaterElectricCoordinated`); un caso senza combo e un caso in cui un'azione ne indebolisce un'altra,
  che non accendono il segno.
- Una voce col bersaglio rifiutato e una che non accadrà hanno ciascuna il suo segno, e la seconda nessun ghost.
- `PIE-HUD-AP-STACKS` e `PIE-PREVIEW-GHOST` riscritte sulla resa nuova, con una domanda: "guardando la mappa, sai
  cosa farà ogni unità e da dove? riconosci la combo?"

## 6. Che cosa resta a chi costruisce

- Com'è fatto il materiale dei lati, quanti dati per istanza servono, quante istanze, e come si aggiornano senza
  ricostruire tutto a ogni cambio.
- La forma esatta del bagliore e degli stili "sottile" e "tratteggiato", da portare all'autore su una tavola.
- I valori esatti dei colori nuovi, entro il gate della palette.
- Il tasto definitivo, se `Maiusc` collide con qualcosa.
- Dove vive il servizio di what-if, come compone lo scenario dalla vista della squadra, e la strategia di cache.
- I nomi dei test, delle voci PIE e dei significati nuovi della palette.
- Come si rende il personaggio semitrasparente (materiale, componenti in pool, costo), e dove vive la riga di icone:
  sopra un'unità vera sta nella sovrapposizione che c'è già, la cui altezza è stata misurata per fumetto, token e
  stati (`progettazione-hud.md` §4.2.1, "Layout"); sopra un ghost serve un appoggio nuovo.
- La divisione in issue e PR dentro ogni fetta. Gli owner esistenti si riusano: #3512, #1941, #3747, #1758, #172.

## 7. Decisioni da registrare

Ognuna vuole la sua riga nel Decision Log, col testo dell'Appendice A.

1. La mappa è pulita di norma; i bordi compaiono col tasto o col contesto. Emenda la griglia sempre accesa di #1758.
2. Le aree si disegnano col solo bordo (stile A).
3. Il movimento ha tre fasce: silenzioso gialla, movimento verde, corsa blu. Emenda la resa di D-627 e D-628 (3).
4. Con una skill attiva: portata, celle colpite sotto il cursore, effetto sul bersaglio valido, icona sul non valido.
5. L'effetto si calcola col piano della propria squadra, a nemici fermi.
6. Il tooltip sul nemico porta: stati, fin dove colpisce, quanto si muove, skill viste usare.
7. Si costruisce con un piano esagonale a istanze per il disegno e col resolver vero su una copia per l'effetto,
   nell'ordine delle fette di §5.
8. Il terreno ricordato resta sempre disegnato com'è oggi (D-520 invariata).
9. Il codice di piano sul bordo della cella si vede solo col tasto. Emenda D-508.
10. Cliccare nella fascia gialla senza `Sneak` dichiarato è un movimento normale: il giallo informa. Conferma D-425.
11. Il piano si presenta come il personaggio semitrasparente, un ghost per tappa, con sopra la testa le icone di ciò
    che fa lì; la squadra vede i piani della squadra. Emenda la resa a prismi impilati.

**Letture dell'assistente, ribaltabili** (ciascuna è marcata nel testo): la risoluzione è fuori; il rosso cupo resta per ciò che gli AP non pagano; l'icona sta in due posti; il tasto è `Maiusc`; `Withdraw`
in verde; i tratti oltre la corsa tratteggiati; la linea tratteggiata fuori portata; il cartellino torna all'hover dopo
il click; il numero è quello del playback; l'icona segue lo stato `Non valida` dello slot; celle colpite e cartellini
hanno una sola fonte; il cartellino è di un'azione e la barra è del turno; l'etichetta dice ciò che il click fa; il segno della combo, solo sui guadagni, e la sua definizione; la reazione
armata come icona a parte; il nemico rivelato resta com'è; la posa ferma del ghost; i segni di rifiutata e di non
accadrà sulle icone; celle colpite e mira per ogni unità della squadra; il compagno che non si comanda resta `FUTURE`; il tetto di 10 ms; la riga del verso esce solo per i nemici; l'ordine del
taglio delle righe.

## Appendice A — Le scelte dell'autore, col testo che ha visto

Sessione del 2026-11-27. Le citazioni sono alla lettera.

**A.1 La descrizione** (risposta alla domanda "in quale momento lo schermo non ti ha dato quello che cercavi?"): il
punto 2 di §1.

**A.2 La correzione dopo il primo mockup** (`01-pianificazione-default.html`): "- normalmente gli esagoni non sono
evidenziati e i contorni non si vedono, con il tasto/contesto giusto si visualizzano i bordi. (dobbiamo migliorare
l'estetica, vedi come hanno risolto altri giochi). - tooltip i consigliati"

Le righe consigliate, come comparivano nel mockup con la stella: "Stati attivi, con cosa fanno e quanti turni
restano", "Fin dove può colpire", "Quanto può muoversi", "Skill che gli hai visto usare, con stima della ricarica".
Sulla pagina l'autore ha acceso e spento più volte "Difese: DEF e RES" e l'ha lasciata spenta.

**A.3 Lo stile** (`02-stili-aree.html`). La domanda: "Quale di questi tre modi di disegnare le aree si avvicina a
quello che hai in mente?" Le opzioni: "A · Solo il bordo. Come XCOM 2: una linea luminosa attorno all'area, niente
dentro."; "B · Bordo e punti. Come BattleTech: un punto al centro di ogni cella raggiungibile."; "C · Bordo e velo.
Come la prima tavola che ti ho mostrato, più leggero." Sotto, uguale per tutti: "Sotto il cursore compare il bordo di
quell'esagono."; "Lungo il percorso compaiono i bordi delle celle che attraversi, con una linea bianca."; "Le celle
colpite dalla skill hanno il loro bordo: lì conta sapere quali sono."; "Col tasto premuto compaiono i bordi di tutti
gli esagoni." **Risposta: "a"**, e il click su "Scelgo A · solo il bordo".

**A.4 Che cosa legge l'effetto.** Le opzioni: "1. Com'è adesso. Riktor è asciutto, quindi la scarica di Gadget mostra
«−210». È semplice e sempre spiegabile, ma non vede le combo che stai preparando in quel turno."; "2. Col tuo piano. Se
nello stesso turno Muiren bagna Riktor prima che Gadget scarichi, la scarica mostra «−290, Bagnato +80». Rende visibili
le combo, che sono il cuore del gioco. Costa di più da costruire, e il numero cambia ogni volta che ritocchi il piano
dell'altra unità." E: "In entrambi i casi i nemici sono trattati come fermi, perché il loro piano non lo conosci."
**Risposta: "la 2"**.

Nello stesso messaggio l'assistente aveva scritto quattro scelte sue, chiedendo di correggerle: "Lo Sneak resta
dichiarato col tasto M."; "Resta fuori ciò che si vede quando la skill si risolve (effetti, suono, camera)."; "Ciò che
gli AP non pagano tiene il rosso cupo di oggi (D-627), come bordo."; "L'icona del bersaglio non valido sta in entrambi
i posti". L'autore non le ha corrette: restano letture dell'assistente, tranne la prima, che ha poi scelto in A.6.

**A.5 Come si costruisce.** L'assistente ha mostrato gli approcci di §4.2 e §4.4, consigliando B per il disegno e A
per l'effetto, e l'ordine delle cinque fette, chiudendo con: "Vado con B per il disegno e A per l'effetto, in
quest'ordine?" **Risposta: "si, poi dobbiamo occuparci della grafica"**.

**A.6 Dopo il controllo del costruttore.** Tre domande a scelta; "(Recommended)" è l'etichetta dell'assistente,
riportata com'era.

1. "La griglia che spegniamo di norma porta anche l'anello ciano del terreno ricordato (quello visto prima e non più
   in vista: la «pianta»). Con la griglia spenta, che ne è di quell'anello?" Opzioni: "Resta sempre (Recommended)":
   "Il terreno ricordato si disegna com'è oggi, con anello e fondo scuro: è il segno di «lo ricordo, non lo vedo». Si
   spegne solo la griglia delle celle in vista."; "Solo col tasto": "Di norma resta il solo fondo scuro; l'anello
   compare tenendo premuto il tasto, insieme alla griglia." **Scelta: "Resta sempre (Recommended)"**.
2. "Sulle mappe a più piani il colore del bordo dice a che piano sta una cella: nero sul piano attivo, bianco sopra,
   viola sotto. Con la griglia spenta di norma, quel codice quando si vede?" Opzioni: "Fuori dal piano attivo
   (Recommended)": "Le celle del piano attivo sono pulite; quelle sopra e sotto tengono il bordo bianco o viola, così i
   piani si distinguono senza tasto."; "Solo col tasto": "Di norma nessun bordo su nessun piano; il codice compare col
   tasto. I piani si distinguono dalla tinta dei volumi e dall'indicatore «Piano N di M»."; "Sempre": "Il codice resta
   acceso su tutte le celle delle mappe a più piani: lì la mappa non è pulita." **Scelta: "Solo col tasto"**, contro
   la raccomandata.
3. "La fascia gialla mostra fin dove arrivi in silenzio. Se clicchi una cella dentro il giallo senza aver dichiarato
   lo Sneak col tasto M, come ti muovi?" Opzioni: "Movimento normale (Recommended)": "Lo Sneak resta una
   dichiarazione, come oggi: il giallo informa. L'etichetta sotto il cursore dice «movimento» e ricorda che lì arrivi
   anche in silenzio con M."; "In silenzio da solo": "Cliccare nel giallo dichiara lo Sneak per quel movimento. Riapre
   una decisione presa: quasi ogni passo corto diventerebbe silenzioso, perché i movimenti di una o due celle sono la
   maggioranza." **Scelta: "Movimento normale (Recommended)"**.

Prima delle domande l'assistente aveva scritto tre scelte sue, nate dallo stesso controllo: l'icona segue lo stato
"Non valida" dello slot; celle colpite e cartellini hanno una sola fonte; il cartellino è di un'azione e la barra è
del turno. Restano letture dell'assistente.

**A.7 Il ghost del piano.** Dopo la prima consegna di questo disegno l'autore ha scritto il punto 4 di §1.
L'assistente ha mostrato `03-ghost-del-piano.html` con la domanda "Il ghost del piano: dove stanno il personaggio
semitrasparente e le icone?" e tre modi: "A · Un ghost per tappa. Un ghost dove l'unità agisce, con le icone di ciò che
fa lì."; "B · Un ghost solo, a fine turno. Una sagoma dove l'unità finirà, con tutte le icone in ordine."; "C · Icone
sull'unità vera. Le icone sopra il personaggio; il ghost dice solo dove finirà." Sotto, uguale per tutti: "Il ghost è
il personaggio, semitrasparente, girato nel verso previsto. Oggi è un prisma piatto."; "Le icone sono quelle delle
skill nella barra, con il colore della fase, nell'ordine in cui il turno le risolve."; "Chi non si muove non ha ghost:
le icone stanno sopra il personaggio."; "La combo: le due icone che si potenziano hanno un anello dorato e un filo che
le lega; accanto a quella potenziata c'è il guadagno."; "La squadra vede tutti i piani della squadra: l'unità
selezionata è piena, le altre più tenui."; "Mai il piano nemico. Un nemico non ha ghost né icone." Nel terminale
l'assistente ha consigliato A e ha chiesto: "Quale dei tre vuoi? Dimmi anche se il segno della combo va bene così."
**Risposta: "a"**. Sul segno della combo l'autore non ha detto niente: resta una lettura dell'assistente.

Un secondo controllo del costruttore, sulla sola fetta 6, ha lasciato cinque domande. L'assistente le ha sciolte tutte
nel testo come letture proprie, senza una domanda nuova all'autore: la tappa raggruppa per `PreviewDestination`
(D-470 non cambia); il compagno che non si comanda resta `FUTURE` (D-478) e il nemico rivelato resta com'è; la combo
è solo un guadagno; la certezza non si gradua sul ghost, e le icone portano i segni di rifiutata e di non accadrà;
celle colpite e mira si disegnano per ogni unità della squadra, come nel mockup.

## Appendice B — Cosa fanno gli altri giochi

`BENCHMARK`: letto da guide e descrizioni, non verificato giocando. Le tavole sono la resa dell'assistente di questi
principi, non copie.

| Gioco | Che cosa fa | Fonte |
|---|---|---|
| XCOM 2 | bordo blu per il movimento, giallo per lo scatto; scudi sulle celle con copertura | [gamepressure](https://www.gamepressure.com/xcom2/movement/za848f), [Fextralife](https://xcom2.wiki.fextralife.com/Combat) |
| Atlas Reactor | tenendo premuto Alt le celle colpite da una skill si accendono di giallo; il percorso è una linea bianca | [tips](https://atlasreactor.blogspot.com/p/tips.html), [Getting Started](https://atlasreactor.blogspot.com/p/getting-started-in-ar.html) |
| Gears Tactics | nessuna griglia visibile; linea piena se si può agire sul bersaglio, tratteggiata se lo si vede ma è troppo lontano | [game guide](https://www.gearsofwar.com/en-us/game-guide/abilities/), [PC Gamer](https://www.pcgamer.com/uk/gears-tactics-is-a-shockingly-good-strategy-game) |
| BattleTech | nessuna griglia; punti attorno all'unità mentre si sceglie dove muovere | [PC Gamer](https://pcgamer.com/battletech-review), [Nexus](https://www.nexusmods.com/battletech/mods/698) |
| Civilization VI | la griglia esagonale è un'opzione che si accende e si spegne | [Console FAQ](https://support.civilization.com/hc/en-us/articles/37661409606547-Civilization-VI-Console-FAQ) |
| Into the Breach | gli autori dichiarano di rinunciare a ogni idea bella che tolga chiarezza | [Game Developer](https://gamedeveloper.com/design/-i-into-the-breach-i-dev-on-ui-design-sacrifice-cool-ideas-for-the-sake-of-clarity-every-time-) |
| (un toolkit Unreal) | i bordi d'area su terreno a più livelli possono risultare strani | [forum Unreal](https://forums.unrealengine.com/t/support-advanced-turn-based-tile-toolkit/21930/2215) |

## Appendice C — Lo stato misurato che questo disegno tocca

Su `3ea74a4b7`, letto nel codice da due ricognizioni in sola lettura. I nomi sono di funzione, perché le righe
invecchiano.

| Che cosa | Dove | Com'è reso |
|---|---|---|
| Cella sotto il cursore | `ARTHexMapActor`, significato `Hover` | contorno giallo, a segmenti |
| Ventaglio e tratto non pagato | `RTPlanningPreview.cpp`, `SetPreviewReachableCells` | contorni verdi e rosso cupo |
| Portata dell'azione armata | `SetPreviewRangeCells`, `URTCombatLibrary::TargetableRangeCells` | contorni viola |
| Portata a tre stati all'hover su uno slot | `FRTPlanningOverlayArbiter`, `SetPreviewSkillRange` | viola, rosa, ambra tratteggiata |
| Area colpita e fuoco amico, dopo il click | `URTHexCombatLibrary::MakeBlastPreview`, `SetPreviewHitCells` | contorni rossi e arancio |
| Aree di movimento del nemico a fuoco | `SetPreviewFocusAreas` | contorni spezzati |
| Griglia | `ARTHexMapActor::CellBorders` | famiglia di istanze, sempre visibile |
| Celle, rilievo, velo | `Cells`, `Relief`, `KnowledgeVolumes`; materiale `M_HexCell` | istanze con dati per istanza |
| Sguardo all'hover | `URTGlanceLibrary::ComposeUnit`, `ARTHUD::DrawHUD` | riquadro sul Canvas |
| Ghost del piano | `RTPlanPreview` (`FRTPhasePreviewEntry`), `ARTHexMapActor::PlanGhosts` | prismi piatti a istanze, impilati |
| Piano di un'unità della squadra | `ARTHUD::DrawHUD`, dalla vista degli intenti | testo e quadrati sul Canvas |
| Scheda al click, con la stima della ricarica | `URTInspectLibrary::Compose` | widget UMG |
| Danno e suo breakdown | `URTHexCombatLibrary::CollectHexAttacks`, `URTCombatResolver::ResolveAttacksWithBreakdown` | funzioni pure |
| Turno eseguito fuori dalla partita | `URTScenarioRunner::Run`, `FRTScenarioSession` | mondo isolato, con `ARTTurnManager` |
| Snapshot di pianificazione | `RTPlanningSnapshot::For` | stato chiesto all'autorità |

**Stati di verifica.** Compile · Tests · Determinism · Replay · Privacy · PIE · Packaged: `N/A`. Questo documento non
cambia codice, asset o scenari.
