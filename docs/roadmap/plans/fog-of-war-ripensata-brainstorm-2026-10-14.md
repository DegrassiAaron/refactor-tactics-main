# La fog of war ripensata — brainstorming del 2026-10-14

> `SNAPSHOT` · fotografia del 2026-10-14: vale finché è l'ultima misura del suo oggetto.
>
> - **Statuto**: brief di un brainstorming con l'autore (`/sc:brainstorm`). Le scelte di §3 sono sue, ognuna col
>   testo dell'opzione **come l'ha vista**. Il taglio del lavoro di §5 e le domande di §6 sono di chi scrive.
> - **Misurato su** `origin/main` = `1a40b3de1`.
> - **Decisioni registrate**: nessuna, ancora. §7 dice quali servono.
> - **Owner di dominio**: la percezione e la sua presentazione, epic #151;
>   [`conoscenza-parziale-visibile-spec.md`](../../technical/systems/conoscenza-parziale-visibile-spec.md) e
>   [`brief-conoscenza-parziale.md`](../../gameplay/brief-conoscenza-parziale.md).
>
> ⛔ Nessuna riga di `Source/` e di `Content/` cambia con questo brief.
>
> Convenzione: niente totali che cambiano da soli. Dove un numero serve, sta accanto al comando che lo produce.

---

## 1. Da dove viene

Chiusa la linea di vista fra piani (#3743, [D-511](../../decisions/RT_PDR_00_Decision_Log.md)), l'autore ha chiesto di ripensare la fog of
war prima di ogni altra cosa. Le sue due ragioni, scelte fra quattro:

- **come appare**: la resa graybox non gli basta;
- **il gioco**: che cosa fa il giocatore con la nebbia.

Il riferimento che ha scelto è **Invisible Inc.**: le stanze già viste restano come pianta, i nemici si vedono solo
in vista.

## 2. Com'è oggi

| Che cosa | Com'è | Dove |
|---|---|---|
| Celle | Tre stati. Osservata ora: piena. Esplorata ma non osservata: colori al 35 %. Mai vista: non disegnata | [D-225](../../decisions/RT_PDR_00_Decision_Log.md), [D-227](../../decisions/RT_PDR_00_Decision_Log.md); `ARTHexMapActor::ApplyKnowledgeVeil`, `RTVeilExploredFactor` |
| Memoria del terreno | La squadra ricorda **quali** celle ha visto (`FRTTeamKnowledge::ExploredCells`), non **com'erano**. Il velo attenua la mappa **attuale**: un muro crollato fuori vista si vede crollato anche lì | `Perception/RTTeamKnowledge.h` |
| Nemici | Un nemico che la squadra non conosce sparisce. Uno perso lascia una sagoma; in strategica un segnalino con la X | Fase A della spec owner |
| Ricordo del nemico | Scade **dopo un turno** | brief §5 («persistenza 1 turno»); `ContactLifetimeTurns`; il test `Vision.LastContactExpiresAfterOneTurn` |
| Bordi | Un pannello si nasconde solo se la squadra non ha mai visto nessuna delle due celle | [D-502](../../decisions/RT_PDR_00_Decision_Log.md) |
| Confine della vista | Il modello esiste: i lati esposti dell'unione vista (`FRTExposedEdge`). In partita **non si disegna**: lo usa solo l'anteprima scenari dell'Editor | `Perception/RTVisibilityBorder.*`; `git grep -l VisibilityBorder -- Source ':!*/Tests/*'` |
| Arco del nemico | Non si disegna da nessuna parte | `git grep -n 'VisionCone\|VisionArc' -- Source` → nessun riscontro |
| Rumore | La regola di propagazione c'è, e il livello `Uncertain` la aspetta; **nessun sistema di gioco la chiama** | Fase C della spec owner; `git grep -l 'URTAcousticPropagationLibrary::' -- Source ':!*/Tests/*'` → solo la libreria |
| Rivelare | `Status.Reveal` è dichiarato e mai implementato | brief §8 |
| Fra piani | Da [D-511](../../decisions/RT_PDR_00_Decision_Log.md) molto del piano di sopra e di sotto resta non visto: una piattaforma mai vista da fuori non si disegna | — |
| Scenari | Dentro uno scenario il velo delle **celle** non si vede; quello sulle unità sì | osservato il 2026-10-13, seduta `U87` |

## 3. Le scelte dell'autore

Quattro tornate di domande, il 2026-10-14. Le citazioni sono il testo dell'opzione scelta, come l'ha vista.

### 3.1 La nebbia sul terreno

| Domanda | Scelta | Il testo dell'opzione | Le altre |
|---|---|---|---|
| Il terreno **mai visto** | **Non disegnato** | «Com'è oggi: lì non c'è niente. Una piattaforma mai vista non esiste a schermo, anche se ci passi sotto.» | «Forma sì, tipo no», «Tutto noto» |
| Il terreno **esplorato** ma non osservato | **Com'era** | «Il ricordo congelato: mostra la cella com'era l'ultima volta che l'hai vista. Se intanto un muro è crollato, non lo sai.» | «Com'è ora, attenuato», «Non lo so ancora» |
| La **resa** del ricordo | **Pianta** | «Come Invisible Inc.: la cella ricordata diventa un disegno a linee monocromo, quasi olografico. Si capisce la forma, non che è il presente.» | «Attenuato», «Desaturato» |
| Il **confine** fra ciò che vedi adesso e il resto | **Linea sul bordo** | «Un contorno netto lungo il bordo della zona vista (il gioco calcola già questi lati esposti). Si legge a colpo d'occhio dove finisce la vista.» | «Basta la resa», «Sfumato» |

### 3.2 I nemici

| Domanda | Scelta | Il testo dell'opzione | Le altre |
|---|---|---|---|
| Vedi che cosa vede un nemico in vista? | **Su richiesta** | «Si vede passando il mouse sul nemico, o con un tasto. Lo schermo resta pulito.» | «Sì, sempre», «No» |
| Il ricordo di un nemico perso | **Anche regola** | «La sagoma resta finché qualcuno guarda quella cella e non lo trova. Fino ad allora la cella ricordata si può bersagliare come cella incerta, e i bot ne tengono conto.» | «Solo a schermo», «Sbiadisce» |

### 3.3 Gli strumenti del giocatore

Fra quattro ne ha scelti tre: **rivelare**, **rumori come indizi**, **ricordo più lungo**. Ha lasciato fuori
«Sbirciare» («Un'azione per guardare oltre un angolo o una porta senza esporsi del tutto»).

| Domanda | Scelta | Il testo delle opzioni scelte |
|---|---|---|
| **Rivelare**: in che forma | **tutte e quattro** | «Abilità d'eroe: fa parte del kit di qualche eroe: uno è lo scout della squadra.» · «Oggetto: un consumabile che chiunque può usare, in quantità limitata.» · «Elemento di mappa: una torre o una console da raggiungere e attivare: rivela una zona, e conquistarla è un obiettivo.» · «Azione comune: chiunque può spendere il turno per guardarsi intorno meglio: più portata o 360 gradi per un turno.» |
| **Rumori**: che cosa fa rumore | **tutto** | «Movimento: muoversi fa rumore; muoversi tanto (correre) ne fa di più che muoversi poco.» · «Attacchi: colpi ed esplosioni si sentono lontano, anche fuori vista.» · «Ambiente: porte che si aprono, coperture che crollano, oggetti attivati.» · «Abilità dichiarate: ogni abilità dice se è rumorosa o silenziosa: è una scelta di bilanciamento, eroe per eroe.» |
| **Rumori**: che cosa dice a schermo | **Una zona** | «Un gruppo di celle dove può essere chi ha fatto rumore: più lontano è, più larga la zona.» |

Il testo dell'opzione «Rivelare» nella tornata degli strumenti era: «Abilità o oggetti che illuminano un'area per un
turno: scanner, drone, razzo.»

### 3.4 Ordine e versione

- **Per primo: la resa.** «Pianta per il ricordo, linea netta sul confine, vista del nemico su richiesta. Cambia come
  si legge la nebbia, non le regole.»
- **Versione: tutto nella v0.1.** «La nebbia ripensata fa parte della fetta verticale offline.»

## 4. Che cosa cambia rispetto a oggi

**Resta com'è**: il terreno mai visto non si disegna ([D-225](../../decisions/RT_PDR_00_Decision_Log.md), confermato).

**Cambia la resa**:
- il ricordo diventa **pianta**, non più colore attenuato;
- il **confine** della vista diventa una linea in partita: il modello esiste, manca il disegno;
- l'**arco** di un nemico in vista compare su richiesta: oggi non esiste.

**Cambiano le regole**, e sono le scelte più pesanti:
- **il terreno si ricorda com'era.** La squadra deve conservare una fotografia di ogni cella esplorata: superficie,
  coperture, porte, muri interni, volumi. La pianta disegna quella fotografia, non la mappa attuale. Chiude una fuga
  d'informazione che c'è oggi: il velo mostra lo stato attuale anche dove nessuno guarda;
- **il ricordo di un nemico resta finché non è smentito.** Supera la «persistenza 1 turno» del brief di gioco, e il
  test che la sorveglia andrà riscritto, non tolto. La cella ricordata si bersaglia come cella incerta; il meccanismo
  per bersagliare una cella e non un'unità c'è già (`Vision.UncertainTargetsCellNotUnit`);
- **il rumore nasce**: da movimento, attacchi, ambiente e abilità dichiarate, e si mostra come una zona;
- **rivelare nasce**, in quattro forme.

**Cambia il perimetro della v0.1.** Il canone classificava la fog of war fuori dalla v0.1; [D-225](../../decisions/RT_PDR_00_Decision_Log.md) ne ha portato
dentro il velo. «Tutto nella v0.1» porta dentro anche rumori, rivelare e la memoria del terreno: è un allargamento
di scope, e va registrato come tale (§7).

## 5. Il lavoro, nell'ordine scelto

Ogni pezzo è una issue sua, sotto un'epic. I criteri qui sono un abbozzo: li fissa la issue, col triage.

### F1 — La resa *(per prima)*

| Pezzo | Che cosa | Si verifica |
|---|---|---|
| **F1a — La pianta** | Le celle esplorate e non osservate si disegnano come pianta: linee monocrome, nessun colore di superficie. In un primo tempo sulla mappa attuale, finché F2a non porta la fotografia | test sul presenter del velo; PIE: i tre stati si distinguono, e il ricordo non si confonde col presente |
| **F1b — Il confine** | In partita, una linea lungo i lati esposti della zona vista dalla squadra. Riusa `FRTExposedEdge`, non lo riscrive. ➕ *2026-10-14, [#3761](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3761), [D-519](../../decisions/RT_PDR_00_Decision_Log.md)*: ciano della vista, anche in strategica, mai lungo il vuoto | test sul modello (c'è) e sul disegno; PIE: si legge a colpo d'occhio dove finisce la vista |
| **F1c — L'arco del nemico** | Passando il mouse su un nemico **in vista**, o con un tasto, si disegna il suo arco frontale e la sua consapevolezza ravvicinata. Le stesse primitive della percezione, non una seconda geometria. ➕ *2026-10-14, [#3762](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3762), [D-521](../../decisions/RT_PDR_00_Decision_Log.md)*: mouse e `H`, le celle viste davvero sul solo terreno noto | test: l'arco è quello di `VisibleCells`; privacy: solo per nemici rilevati; PIE |
| **F1d — Il velo negli scenari** | Dentro uno scenario si vede anche il velo delle celle | PIE sugli scenari di percezione |

### F2 — La memoria

| Pezzo | Che cosa | Si verifica |
|---|---|---|
| **F2a — Il terreno com'era** | `FRTTeamKnowledge` conserva la fotografia delle celle esplorate; la pianta la disegna. Formato versionato: entra nello snapshot e nelle repliche | test di regola: un muro crollato fuori vista resta in piedi nel ricordo, e si aggiorna quando lo si rivede; determinismo; replay |
| **F2b — Il ricordo fino a smentita** | Il contatto resta finché la squadra non osserva quella cella senza trovarci il nemico, o lo vede altrove. Bersagliabile come cella incerta; i bot lo usano | test di regola, di squadra e dei bot; il test di oggi sulla scadenza riscritto, non tolto |

### F3 — I rumori

Chi produce rumore (movimento, attacchi, ambiente, abilità dichiarate), quanto lontano si sente, e la zona a schermo.
La regola di propagazione e la sua verifica di scala stanno già in Fase C della spec owner; manca il produttore. I bot
sentono come il giocatore: è la stessa conoscenza di squadra.

### F4 — Rivelare

Le quattro forme: abilità d'eroe, oggetto, elemento di mappa, azione comune. Il gancio è `Status.Reveal`. Ognuna tocca
un dominio diverso (kit, oggetti, mappe, azioni comuni), quindi una issue per forma.

## 6. Domande ancora aperte

Non sono state poste in questa sessione. Si chiudono pezzo per pezzo, col pezzo.

- **F1a**: che colore e che spessore ha la pianta; se anche i bordi e i muri ricordati diventano linee; come si legge
  in vista strategica.
- **F1b**: se la linea di confine si vede anche in strategica; che colore ha rispetto al contorno delle unità.
- **F1c**: mouse, tasto, o tutti e due; se l'arco mostra anche la portata piena o solo il cono.
- **F2a**: che cosa entra nella fotografia di una cella, e che cosa no (gli effetti di terreno a tempo, per esempio).
- **F2b**: una sagoma che resta a lungo diventa rumore a schermo? Serve un limite, o la smentita basta?
- **F3**: quanto lontano si sente ogni fonte; se il passo lento e la corsa diventano due modi di muoversi.
- **F4**: quanto dura e quanto è largo ogni rivelare; che cosa costa l'azione comune («più portata o 360 gradi»).
- **Fra piani**: con la regola di [D-511](../../decisions/RT_PDR_00_Decision_Log.md), una piattaforma vista una volta da fuori diventa pianta quando ci passi sotto. Va bene
  così, o il piano di sopra si disegna in un altro modo?

## 7. Le decisioni da registrare

Nessuna è ancora nel Decision Log. Si registrano col primo merge che le implementa, col testo di §3. Il numero si
prende su misura al momento.

1. Il terreno esplorato si ricorda **com'era** (affina [D-227](../../decisions/RT_PDR_00_Decision_Log.md)).
2. Il ricordo si disegna come **pianta**, e il confine della vista come **linea** (affina [D-225](../../decisions/RT_PDR_00_Decision_Log.md) e [D-227](../../decisions/RT_PDR_00_Decision_Log.md)).
3. L'arco di un nemico in vista si vede **su richiesta**.
4. Il ricordo di un nemico resta **finché non è smentito**, ed è una regola (supera la «persistenza 1 turno» del brief).
5. Il rumore: le quattro fonti, e la **zona** a schermo.
6. Rivelare: le quattro forme.
7. Lo scope: la fog of war ripensata entra **tutta nella v0.1**.

## 8. Vincoli che non cambiano

- **Autorità.** La conoscenza di squadra si calcola sull'autorità. Il client riceve la parte della sua squadra, e la
  presentazione la consuma: non la ricalcola.
- **Privacy.** Niente di ciò che la squadra non conosce arriva al suo schermo. La fotografia del terreno chiude una
  fuga che c'è oggi; il ricordo lungo è informazione che la squadra ha davvero avuto.
- **Determinismo.** La fotografia e il ricordo entrano nello stato canonico: ordine esplicito, formato versionato,
  repliche.
- **Bot.** Usano la stessa conoscenza del giocatore: ricordo lungo e rumori valgono anche per loro.
- [D-018](../../decisions/RT_PDR_00_Decision_Log.md): la quota non dà bonus numerici alla vista.

## 9. Rischi

- **La memoria del terreno è stato canonico, non presentazione.** Cresce la conoscenza di squadra, e con lei snapshot
  e repliche. Va misurata sulla mappa più grande della v0.1 prima di fissarne il formato.
- **La pianta è un materiale nuovo**: authoring sul clone principale, che va prenotato fra le sessioni.
- **Il ricordo lungo cambia i bot.** Una sagoma vecchia attira mosse: le sonde anti-stallo e i corpus dei bot vanno
  rimisurati sulla suite intera.
- **Lo scope della v0.1 cresce di molto.** Rumori e rivelare in quattro forme toccano kit, oggetti, mappe e
  bilanciamento.
