# Editor map — le sedute davanti a Unreal

> `GENERATA` · il blocco §2 lo riscrive `python scripts/feature_registry.py shortlist`, leggendo le
> sedute da [`editor-sessions.yaml`](editor-sessions.yaml), lo stato delle voci da
> [`../technical/test-manuali-pie.md`](../technical/test-manuali-pie.md) e gli artefatti da `git ls-files`.
> **Cosa è**: la sequenza del lavoro che solo una persona davanti all'editor può fare.
> **Cosa non è**: il registro degli esiti — l'esito atteso e lo stato delle voci `PIE-*` vivono nel
> registro. Qui si citano gli **ID**, mai il contenuto.

⚠️ **Non si modifica a mano.** Il testo delle sedute — procedura, avvertenze, debiti noti — si scrive in
[`editor-sessions.yaml`](editor-sessions.yaml), che ne è la sorgente. Una modifica fatta qui viene persa alla
prossima rigenerazione, e `--check` la segnala prima.

---

## 1. Cos'è una seduta, e perché questa vista esiste due volte

Questa vista **è già morta una volta**: [`roadmap-editor.md`](roadmap-editor.md) è stata ritirata il
2026-08-08 perché era la terza vista di stato mantenuta a mano e aveva perso la gara col codice. Il ritiro
poneva la condizione per farla tornare — *«servirà se un giorno questa vista verrà **generata** invece che
scritta»* — ed è la condizione che questo file soddisfa: nessun simbolo qui dentro è scritto da una persona.

**Anatomia di una seduta**

- **Sbloccata da** — i checkpoint di codice che devono avere il **codice fatto**. Un 🟡 basta e anzi è il caso
  normale: gli manca proprio la verifica che porti tu. Non aspettare che diventi ✅ — non può, senza di te.
- **Preparazione condivisa con** — le sedute che usano lo stesso allestimento: falle nella stessa apertura.
- **Produce** — due forme legittime: un **asset committato**, oppure un **verdetto** che chiude un checkpoint.
- **Percorso critico** — se `no`, la seduta non blocca la v0.1.
- **Verifichi** — gli ID delle voci nel registro, col loro stato misurato lì.
- **Finita quando** — condizione osservabile, non impressione.

**Come si deriva lo stato**

| | Quando |
|---|---|
| ⏳ | nessuna voce verde, nessun artefatto tracciato |
| 🟡 | aperta, parte fatta — oppure una verifica è andata storta |
| ✅ | **tutte** le voci verdi **e** tutti gli artefatti tracciati da `git` |
| — | non dichiara né voci né artefatti: il codice sotto non esiste ancora |

Un artefatto non tracciato impedisce il verde qualunque cosa dicano le voci. E `git add` su un `.uasset` non
basta: `.gitignore` esclude `Content/**/*.uasset` e riammette i singoli file con un `!` esplicito — un asset
fuori dall'allowlist non viene tracciato, **in silenzio**.

**Quando una verifica va storta**: la voce resta ⏳ nel registro con una **nota datata** — convenzione che
quel documento già usa. Nessun simbolo nuovo. La seduta scende a 🟡 da sola.

---

## 2. Le sedute

<!-- RT_SHORTLIST_EDITOR:BEGIN -->

**27 sedute** — ✅ **4** · 🟡 **12** · ⏳ **5** · **6** senza stato derivabile (non dichiarano ne' voci ne' artefatti: il codice sotto non esiste ancora).

Stato **derivato**, mai dichiarato: dalle voci `PIE-*` di [`../technical/test-manuali-pie.md`](../technical/test-manuali-pie.md) e da `git ls-files` sugli artefatti. Un artefatto non tracciato impedisce il verde qualunque cosa dicano le voci.

### My Editor Queue

**BLOCKING** 10 · **READY** 2 · **WAITING** 5 · **DONE** 4. **Derivata**, non dichiarata: `unblocked_by` risolto dice se si puo' cominciare, `critical` se blocca la v0.1, lo stato se e' finita. Un checkpoint 🟡 conta come risolto — gli manca la verifica che porti tu; una **seduta** prerequisito no, perche' a meta' non ha ancora prodotto il suo artefatto.

**BLOCKING** — *Blocca la v0.1, e si puo' fare adesso*

- **U1** · Mappa-arena hex — 6/7 voci verdi · sblocca U13, U19
- **U2** · Partita hex, primo giro — 7/8 voci verdi · sblocca U3, M6.1, M6.2
- **U3** · Input e pianificazione — 1/4 voci verdi · sblocca U4, M6.3
- **U4** · Combat e linea di tiro — 0/3 voci verdi · sblocca U5, M6.4, M6.5
- **U5** · Bot e HUD — 0/7 voci verdi · sblocca U6, U19, M6.6, M6.7
- **U6** · Multilivello e partita completa — 0/4 voci verdi · sblocca U16, U19, M6.8
- **U7** · Personaggi Paragon — 1/2 voci verdi · sblocca U8, U19
- **U8** · Animazioni — 0/2 voci verdi · sblocca U9, U19
- **U9** · Leggibilita' e riferimento visivo — 2/4 voci verdi · sblocca M8.1, M8.2, M8.3
- **U15** · HUD, intenti, log e comandi debug — 1/5 voci verdi · sblocca E11

**READY** — *Si puo' fare adesso, fuori percorso critico*

- **U18** · Verifiche senza prerequisiti — 4/15 voci verdi
- **U20** · Confine fra Guard e Brace — 0/1 voci verdi

**WAITING** — *Aspetta codice*

- **U11** · I 4 eroi — attende `U10` —
- **U13** · Arena v0.1 — attende `U1` 🟡
- **U14** · Ambiente in partita — attende `U13` ⏳
- **U19** · Durata, ritmo e scala — attende `U6` 🟡, `U1` 🟡, `U5` 🟡, `U7` 🟡, `U8` ⏳
- **U16** · Misura dei KPI — attende `U6` 🟡

**DONE** — *Finite*

- **U21** · Luci del graybox e inquadratura della mappa ✅
- **U22** · Il gesto dell'autore — ghost, snap e Undo del tool Geometry ✅
- **U24** · I `WBP_RT_*` del frontend — banner, modale d'errore, loading e root ✅
- **U27** · Il pulsante BACK del modale d'errore, collegato al navigatore ✅

### Tutte le sedute

| | Seduta | Lane | Produce | Sbloccata da | Critico | Voci | Stato |
|---|---|:--:|---|---|:--:|:--:|:--:|
| **U18** | Verifiche senza prerequisiti | `PIE` | verdetto su quindici voci che non attendono nulla | — | no | 4/15 | 🟡 |
| **U1** | Mappa-arena hex | `ASSET` | `DA_HexMap_Arena` e `L_HexArena`, committati | M6.0 | sì | 6/7 | 🟡 |
| **U2** | Partita hex, primo giro | `PIE` | verdetto su allestimento e movimento | M6.1, M6.2 | sì | 7/8 | 🟡 |
| **U3** | Input e pianificazione | `PIE` | verdetto su selezione, budget e anteprima del percorso | M6.3 | sì | 1/4 | 🟡 |
| **U4** | Combat e linea di tiro | `PIE` | verdetto su forme d'attacco, LOS esagonale e knockback | M6.4, M6.5 | sì | 0/3 | ⏳ |
| **U5** | Bot e HUD | `PIE` | verdetto sul bot su hex e i pesi utility ritarati sulla scala esagonale | M6.6, M6.7 | sì | 0/7 | 🟡 |
| **U6** | Multilivello e partita completa | `PIE` | chiusura di M6 / E2 — sessione D verde | M6.8 | sì | 0/4 | 🟡 |
| **U23** | Autobattle registrata — la partita che si guarda, in PIE e su packaged | `PIE` | il video/log della partita non presidiata — l'evidenza che G10 chiede e la riserva che G13 dichiara | E47.1, E47.3 | sì | — | — |
| **U7** | Personaggi Paragon | `ASSET` | i quattro Blueprint-unita' del roster v0.1, committati | — | sì | 1/2 | 🟡 |
| **U8** | Animazioni | `ASSET` | gli anim BP dei quattro personaggi e i montaggi Cast/Hit/Death | — | sì | 0/2 | ⏳ |
| **U9** | Leggibilita' e riferimento visivo | `PIE` | il video (o gli screenshot) di riferimento — DoD di milestone di M8 | — | sì | 2/4 | 🟡 |
| **U10** | Data asset delle azioni | `PIE` | il catalogo azioni della v0.1 come dati, non come codice | E1.3, E1.4 | sì | — | — |
| **U11** | I 4 eroi | `PIE` | i data asset di Gadget, Phase, Riktor e Wraith, e lo spawn 2v2 che li usa | E6, U10 | sì | 0/1 | 🟡 |
| **U12** | Loadout | `PIE` | varianti arma, gadget e moduli reazione come dati — 1 + 1 + 1 per eroe | E7, U11 | no | — | — |
| **U13** | Arena v0.1 | `PIE` | l'arena estesa con quanto serve alle verifiche di contenuto | E8, E9, U1 | sì | 0/1 | ⏳ |
| **U14** | Ambiente in partita | `PIE` | verdetto sulle regole ambientali e strutturali | U13 | sì | 0/11 | 🟡 |
| **U15** | HUD, intenti, log e comandi debug | `PIE` | verdetto su leggibilita' e osservabilita' | E11 | sì | 1/5 | 🟡 |
| **U19** | Durata, ritmo e scala | `PIE` | numeri di playtest — non difetti | U6, U1, U5, U7, U8 | sì | 0/4 | ⏳ |
| **U16** | Misura dei KPI | `PIE` | numeri reali nella tabella KPI | U6 | sì | 0/1 | 🟡 |
| **U17** | Release v0.1 | `PIE` | build Windows Development e Shipping, e una partita giocata senza editor | E12 | sì | — | — |
| **U20** | Confine fra Guard e Brace | `PIE` | verdetto di leggibilita' — un dato per `BAL-1`, non un difetto da correggere | E5.2 | no | 0/1 | ⏳ |
| **U21** | Luci del graybox e inquadratura della mappa | `PIE` | verdetto su leggibilita' della scena e inquadratura, piu' il livello illuminato committato | — | no | 2/2 | ✅ |
| **U22** | Il gesto dell'autore — ghost, snap e Undo del tool Geometry | `PIE` | verdetto su leggibilita' del ghost, percepibilita' dello snap e granularita' dell'Undo | U21 | no | 4/4 | ✅ |
| **U24** | I `WBP_RT_*` del frontend — banner, modale d'errore, loading e root | `PIE` | i primi cinque widget del frontend, sotto `/Game/RT/UI/Framework/` | — | no | — | ✅ |
| **U25** | Il volume di posa della cella, e la scena che dice se il graybox si legge | `PIE` | verdetto di leggibilita' del kit graybox e il volume di posa come guida d'editor | U21 | no | — | — |
| **U26** | La griglia di lavoro e la sonda di movimento nell'editor | `PIE` | verdetto su leggibilita' della griglia di lavoro e della sonda di movimento | U21 | no | — | — |
| **U27** | Il pulsante BACK del modale d'errore, collegato al navigatore | `PIE` | il `BACK` di `WBP_RT_ErrorModal` che chiama `BackFromError` invece di essere disegnato e inerte | — | no | — | ✅ |

**Lane**: `PIE` **24** · `ASSET` **3**. `ASSET` significa che l'uscita e' un asset da costruire e committare, `PIE` che e' un verdetto da dare guardando il gioco. Non e' l'evidenza: U7 e' `ASSET` **e** verifica due voci `PIE-*`. Serve a rispondere a una domanda sola — *cosa mi serve per farla, il gioco che gira o gli asset che non ho ancora?*

### Blocco 1 — Eseguibile oggi

*Nessuna di queste attende codice. ⚠️ **Il banco di prova non si costruisce piu' a mano**: `MapSource = GeneratedTestArena` genera gia' esagono r=4, ostacoli, muro che blocca la vista, fango a costo 3, piattaforma sul layer 1 e una transizione — quindi le sedute della parita' hex non aspettano U1. Costruire una mappa serve a verificare **gli strumenti**, e a preparare il contenuto della v0.1.*

#### U18 · Verifiche senza prerequisiti 🟡

**Sbloccata da**: — · **Percorso critico**: no
**Produce**: verdetto su quindici voci che non attendono nulla
**Verifichi**: `PIE-PREVIEW-AREA` ✅ · `PIE-V01-MATCHEND` ✅ · `PIE-TEST-CONSOLE` 🟡 · `PIE-HEX-LAYER` ✅ · `PIE-HEX-TRANS` ✅ · `PIE-HEX-LAYER-FOCUS` ⏳ · `PIE-HEX-LAYER-CLICK` ⏳ · `PIE-HEX-LAYER-PANEL` ⏳ · `PIE-V01-REACTCOND` ⏳ · `PIE-HEX-VIZ-BLOCCHI` ⏳ · `PIE-HEX-VIZ-COSTO` 🟡 · `PIE-HEX-VIZ-BORDI` ⏳ · `PIE-HEX-VIZ-PORTE` ⏳ · `PIE-HEX-VIZ-UNDO` ⏳ · `PIE-HEX-VIZ-TRANSIZIONI` ⏳
**Finita quando**: le quindici voci hanno esito reale nel registro

**E' la sola seduta che non attende nulla**: nessun checkpoint da chiudere, nessuna seduta
prima, nessun artefatto da committare. *Senza prerequisiti* non vuol dire *senza allestimento*:
le prime tre voci si fanno premendo Play sull'arena di ripiego, le ultime due in editor, su un
asset **generato e usa-e-getta** — nessuna mappa da costruire a mano, e niente che finisca in
git.

> ⚠️ **Prima delle tre voci in Play, controlla che `Scenario To Run` sia vuoto**
> (`BP_GameMode` → Class Defaults → *RefactorTactics|Test*) e che `rt.Test.Scenario` in console
> sia vuota — quest'ultima **prevale** sulla property e dura quanto il processo dell'editor. Con
> uno scenario impostato la partita normale non viene allestita affatto e tutte e tre le voci
> diventano non eseguibili. E' successo il 2026-08-08, uscendo da `PIE-SCEN-KEEP`.

**`PIE-V01-REACTCOND`** (aggiunta il 2026-08-12, [D-109]) si fa in Play e chiede un ordine
preciso: prima **arma una reazione** sull'unita' selezionata, poi `rt.Reaction.Condition 50`.
Al contrario il comando rifiuta, ed e' il comportamento giusto — una condizione senza reazione
resterebbe orfana. Si verifica il **canale**, non l'effetto: le risposte legali si riducono
davvero, ma la finestra che quel collasso evita non esiste finche' non atterra CP 14.5, e
quella parte e' coperta headless da `Reactions.DeclaredConditionCollapsesToImmediateCommit`.

L'ordine non e' arbitrario:

1. `PIE-PREVIEW-AREA` e `PIE-V01-MATCHEND` su una partita normale.
2. `PIE-TEST-CONSOLE` per **ultima**, e non per gusto di ordine: `rt.Test.Run` **sostituisce
   la mappa** e aggiunge unita' alla partita in corso. Eseguirla prima lascerebbe le altre due
   su uno stato che non e' quello che dicono di verificare. Dopo, si riavvia con `R`.
3. `PIE-HEX-LAYER` e `PIE-HEX-TRANS` in coda, sull'**editor** e non in Play: servono un
   `ARTHexMapActor` con celle su >=2 layer, che si ottiene con `GenerateIntoAsset` —
   `ActiveLayer=0`, poi `ActiveLayer=1`. Nessuna mappa da costruire a mano.
4. Le tre `PIE-HEX-LAYER-*` **sullo stesso asset del punto 3**, senza rigenerare nulla: e' per
   questo che stanno qui e non in una seduta propria. In piu' serve il mode **Hex Map** attivo,
   perche' i contorni dei piani di contesto li disegnano i tool, non l'actor: con `LayerView=Focus`
   ma senza mode attivo e senza `bShowOverlay` acceso nel pannello del tool, `Focus` e'
   indistinguibile da `ActiveOnly` — e la voce sembrerebbe fallita mentre e' solo non allestita.

   > ⚠️ **`PIE-HEX-LAYER-CLICK` va fatta con la camera OBLIQUA**, non dall'alto. Il difetto che
   > verifica e' che il raggio del click agganci il disco di un piano superiore e venga poi
   > proiettato su quello attivo: lo scarto e' orizzontale e proporzionale a `LayerHeight`,
   > quindi guardando a picco vale zero e la voce passerebbe comunque, rotta o no.
5. Le sei `PIE-HEX-VIZ-*` **in coda, e con un allestimento proprio**: sono l'unico gruppo di
   questa seduta che non riusa l'asset del punto 3. Servono coperture e porte, quindi si parte
   da `MakeCoverYardArena`; `-BLOCCHI` e `-COSTO` vogliono in piu' una cella costosa che blocca
   anche la vista, e `-TRANSIZIONI` una piattaforma **senza** archi.

   > ⚠️ **`PIE-HEX-VIZ-BLOCCHI` e `-COSTO` si guardano DALL'ALTO** — e' la vista di lavoro, ed
   > e' quella in cui il difetto del 2026-08-12 si vedeva: la lastra della vista era piu' alta
   > E piu' larga del rilievo del costo, quindi ogni cella costosa che bloccava la vista taceva
   > sul proprio costo. Di taglio le due forme si distinguono comunque e la voce passerebbe.

   > ⚠️ **`PIE-HEX-VIZ-TRANSIZIONI` va fatta con un tool DIVERSO da Arch** (Paint o Fill). Il
   > difetto che verifica e' che le transizioni si vedessero **solo** dentro il tool Arch: con
   > Arch attivo la voce passerebbe sempre, prima e dopo la correzione.

> Nasce dalla sessione G del registro (2026-08-08), che non aveva una seduta corrispondente: le tre voci erano nella checklist ma in nessuna seduta, e una voce che non sta in una seduta non viene eseguita mai. **Dal 2026-08-10 ne ospita cinque**: `PIE-HEX-LAYER` e `PIE-HEX-TRANS` erano in U1, ma le loro precondizioni nel registro citano un asset *generato* (`GenerateIntoAsset`, due celle sovrapposte) e non l'arena — stavano su una seduta del percorso critico senza dipenderne. Issue 450. Nello stesso passaggio il titolo e' cambiato da «senza preparazione» a «senza prerequisiti»: le due voci nuove un allestimento ce l'hanno — generano celle su due layer in editor — e il vecchio titolo sarebbe diventato falso per due voci su cinque. Cio' che accomuna le cinque non e' l'assenza di allestimento ma l'assenza di **attese**: nessuna dipende da codice mancante o da una seduta prima. **Dal 2026-08-12 sono otto**: `PIE-HEX-LAYER-FOCUS`, `-CLICK` e `-PANEL` arrivano col merge di #565 (issue 567) e riusano l'asset generato del punto 3 senza aggiungere allestimento. Sono finite qui per la stessa ragione per cui questa seduta esiste: erano nel registro e in nessuna seduta — il conteggio della EditorMap le aveva gia' contate fra le voci orfane (`PIE-HEX-*` da 9 a 12) prima che qualcuno le collocasse. **Dal 2026-08-12 (sera) sono quindici**: le sei `PIE-HEX-VIZ-*` della serie viz editor — `-BLOCCHI`, `-COSTO`, `-BORDI`, `-PORTE`, `-UNDO` (nate coi merge di #551/#552/#553) e `-TRANSIZIONI`, che **non esisteva**: #554 e' stata chiusa e mergiata (PR #707) lasciando la propria acceptance **visiva** senza una voce che la registrasse, cioe' lo stesso difetto che #553 aveva evitato aprendone cinque. Sono le uniche voci `PIE-HEX-*` **aperte** rimaste orfane: le altre otto (`PIE-HEX`, `MODE-A`, `-B`, `-C`, `-D`, `-I`, `-J`, `-K`, `-M`) sono orfane perche' gia' ✅, e non vanno collocate. A differenza delle otto precedenti queste **portano un allestimento proprio** (`MakeCoverYardArena`), quindi il titolo «senza prerequisiti» regge solo nel senso stretto in cui e' stato ridefinito il 2026-08-10: assenza di **attese**, non di allestimento. Nessuna dipende da codice mancante — le quattro issue della serie sono chiuse.

#### U1 · Mappa-arena hex 🟡

**Sbloccata da**: M6.0 · **Preparazione condivisa con**: U13 · **Percorso critico**: sì
**Produce**: `DA_HexMap_Arena` e `L_HexArena`, committati
**Artefatti**: `Content/RT/Maps/Dev/L_HexArena/L_HexArena.umap` ✅ · `Content/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.uasset` ✅
**Verifichi**: `PIE-HEX-MODE-E` ✅ · `PIE-HEX-MODE-F` ✅ · `PIE-HEX-MODE-G` ✅ · `PIE-HEX-MODE-H` 🟡 · `PIE-HEX-MODE-L` ✅ · `PIE-HEX-MODE-N` ✅ · `PIE-HEX-MODE-O` ✅
**Finita quando**: i due asset sono tracciati da `git ls-files`, le sette voci hanno un esito reale, e l'arena soddisfa i tre criteri dei passi 3, 4 e 7
**Sblocca**: U13, U19

**Costruire una mappa e' il modo di esercitare gli strumenti**: le sette voci qui sopra verificano
il mode, non il terreno. Il banco di prova della parita' hex arriva invece da
`MapSource = GeneratedTestArena` e **non va costruito a mano** — U2…U6 non aspettano questa seduta.
L'asset serve al contenuto della v0.1: e' quello che U13 estende e U19 misura.

Nessuna guida copre ancora questa procedura, quindi i passi stanno qui.

1. Livello nuovo in `/Game/RT/Maps/Dev/L_HexArena/`, con un `ARTHexMapActor`; l'asset mappa in
   `.../L_HexArena/Data/DA_HexMap_Arena` (stessa forma di `L_DevSandbox`,
   `convenzioni-contenuti-ue.md` §5).
   *Perche' non estendere `DA_HexMap_Sandbox`*: il sandbox resta il banco per prove distruttive,
   l'arena e' la mappa **stabile** su cui girano le verifiche — se la stessa mappa fa entrambe
   le cose, un esperimento invalida una verifica e non te ne accorgi.
2. Editor Mode **Hex Map** → tool **Paint**, `BrushRadius=4`, un click sull'origine: esagono
   pieno di raggio 4 sul layer 0.
3. 2–3 celle con `bBlocksMovement` e 2–3 celle con `bBlocksLineOfSight` **allineate** fra le due
   meta' del campo — servono a `PIE-HEXPLAY-6`, che senza copertura non dimostra niente.
   **Criterio**: esistono >=2 celle `bBlocksLineOfSight` tali che il segmento fra i due spawn ne
   attraversi almeno una. «Allineate» e' una condizione sul segmento, non un giudizio a occhio.
4. Una zona a costo alto (Mud o Water) col tool **Fill**: e' quella che fa mordere il budget in
   `PIE-HEXPLAY-3`.
   **Criterio**: la zona rende il percorso piu' breve **non percorribile** nel budget Move di un
   turno — e' cio' che `PIE-HEXPLAY-3` chiama «far mordere il budget». Una zona costosa che si
   aggira senza rinunciare a nulla non e' una scelta.
5. Piattaforma di 3–4 celle sul layer 1 (`ActiveLayer=1`), collegata al layer 0 da **una sola**
   transizione, creata col tool **Arch**.
6. `bShowOverlay` attivo per rileggere il risultato a colori prima di salvare.
7. **Due rotte distinte** fra gli spawn, con trade-off diverso — una piu' corta ed esposta, una
   piu' lunga e coperta. Senza scelta di percorso il tempo di contatto e' una costante della
   mappa, non una decisione del giocatore, e `PIE-V01-MAPSCALE` (U19) non e' verificabile.
   **Criterio** — e' qui che si scrive, non in U19, perche' e' questa seduta a produrre l'arena:
   due percorsi minimi fra gli spawn che **non condividono celle** oltre agli estremi, di costo
   entro un fattore **1,5** l'uno dall'altro, e con un numero **diverso** di celle esposte alla
   LOS avversaria. Senza le tre condizioni insieme «trade-off» non e' misurabile: due rotte
   gemelle, o una lunga il doppio, non mettono davanti a una decisione.

> ⚠️ **`git add` su un `.uasset` non basta.** `.gitignore` esclude `Content/**/*.uasset` e
> `Content/**/*.umap`, e riammette i singoli file con un `!` esplicito. I due artefatti di questa
> seduta sono gia' **in allowlist** dal 2026-08-10 (issue 449), aggiunti *prima* che gli asset
> esistano: senza, `git add` non fa nulla, senza errore, e la seduta non si chiude mai senza che
> si capisca perche'. L'oracolo e' `git check-ignore -q <file>` → **exit 1**; con `-v` il comando
> esce `0` anche sulle regole di negazione e non distingue i due casi.

> **Debito noto**: `FRTHexCellData` oggi non ha il campo cover; E9 / CP 9.1 incrementa la versione del formato. Quest'arena andra' **migrata** in U13, non ricostruita. Costruirla dopo E9 eviterebbe la migrazione ma lascerebbe M6 senza banco di prova: costo accettato consapevolmente. **Alza la posta**: `DA_HexMap_Sandbox.uasset` pesa 1396 byte, cioe' e' di fatto vuoto — questa sara' la prima mappa con contenuto reale del repository, e in U13 il primo soggetto vero della migrazione di formato.

### Blocco 2 — Parita' hex (M6 / E2)

*Ogni seduta segue il checkpoint di codice che la abilita. L'ordine e' quello del codice, non una scelta.*

#### U2 · Partita hex, primo giro 🟡

**Sbloccata da**: M6.1, M6.2 · **Preparazione condivisa con**: U3, U4, U5, U6 · **Percorso critico**: sì
**Produce**: verdetto su allestimento e movimento
**Verifichi**: `PIE-HEXPLAY-1` ✅ · `PIE-HEXPLAY-4` ⏳ · `PIE-HEXPLAY-5` ✅ · `PIE-CAM-START` ✅ · `PIE-CAM-ORBIT` ✅ · `PIE-CAM-ZOOM-ANCHOR` ✅ · `PIE-CAM-BOUNDS` ✅ · `PIE-CAM-FOCUS` ✅
**Finita quando**: le voci hanno esito reale nel registro
**Sblocca**: U3, M6.1, M6.2

**Non aspetta U1**: il terreno lo fornisce `MapSource = GeneratedTestArena`, che genera gia'
esagono r=4, ostacoli, muro che blocca la vista, fango a costo 3, piattaforma sul layer 1 e una
transizione. Questa e' la preparazione condivisa da U2…U6: una apertura, cinque sedute.

1. `RTGameMode` come GameMode Override sul livello (`debug-vs-unreal.md` §2).
2. Play. Le unita' sono **cilindri**: i `BP_Unit_*` non esistono in `Content/` e il fallback e'
   previsto — non e' un difetto.
3. Due unita' verso la stessa cella, lock-in con **Spazio** → contesa.
4. Ripeti con lo **scambio diretto A↔B**: e' l'unico caso che i test headless non coprono.

> La verifica «se compare una griglia quadrata c'e' un `ARTGridActor` posato a mano» **non esiste piu'**: `ARTGridActor` e' stato rimosso dal codice al CP 7.2, quindi non puo' comparire in un livello. La nota della vista ritirata era gia' superata quando fu archiviata.

#### U3 · Input e pianificazione 🟡

**Sbloccata da**: M6.3 · **Preparazione condivisa con**: U2, U4, U5, U6 · **Percorso critico**: sì
**Produce**: verdetto su selezione, budget e anteprima del percorso
**Verifichi**: `PIE-HEXPLAY-2` 🟡 · `PIE-HEXPLAY-3` ✅ · `PIE-HEXPLAY-3b` 🟡 · `PIE-PREVIEW-PERSIST` ⏳
**Finita quando**: le voci hanno esito reale nel registro
**Sblocca**: U4, M6.3

Selezioni un'unita', muovi il mouse sulla griglia, provi una cella **valida**, una **oltre il
budget**, una **bloccata** e una **occupata**. Su mappa multilivello controlla che la cella
selezionata sia quella del layer giusto: le celle sovrapposte non devono confondersi.

#### U4 · Combat e linea di tiro ⏳

**Sbloccata da**: M6.4, M6.5 · **Preparazione condivisa con**: U2, U3, U5, U6 · **Percorso critico**: sì
**Produce**: verdetto su forme d'attacco, LOS esagonale e knockback
**Verifichi**: `PIE-HEXPLAY-6` ⏳ · `PIE-HEXPLAY-6b` ⏳ · `PIE-HEXPLAY-6c` ⏳
**Finita quando**: le voci hanno esito reale nel registro
**Sblocca**: U5, M6.4, M6.5

Attacco attraverso una cella che blocca la vista, poi da una cella di lato; ostacolo su un
**altro layer** (regola di elevazione). Il knockback a 6 direzioni e' l'unico punto di M6 con
una decisione di design dietro: **guardalo**, non solo verificarlo.

Per la spinta usa **Phase** (`PressureJet`) o **Riktor** (`Ram`). Fino al 2026-08-10
`PIE-HEXPLAY-6c` chiedeva `Guardian.Sweep` (knockback 2), un archetipo legacy che nessun eroe
della v0.1 aveva — la voce **non era eseguibile** come scritta (#410); quell'azione e' poi
sparita del tutto con gli archetipi (#426), ed era l'ultimo `Push 2` del progetto. Ora la voce
e' sul roster, ma con `Push 1`: si osservano la direzione esagonale, il blocco contro
ostacolo/unita'/bordo e l'annullamento fra spinte opposte. L'arresto anticipato contro un
ostacolo a **due** celle non e' piu' osservabile da nessuna parte in partita, e resta coperto
solo headless (`HexCombat.Knockback*`, che la spinta se la costruisce da solo).

#### U5 · Bot e HUD 🟡

**Sbloccata da**: M6.6, M6.7 · **Preparazione condivisa con**: U2, U3, U4, U6 · **Percorso critico**: sì
**Produce**: verdetto sul bot su hex e i pesi utility ritarati sulla scala esagonale
**Verifichi**: `PIE-HEXPLAY-7` 🟡 · `PIE-HEXPLAY-9` ⏳ · `PIE-AI-01` ⏳ · `PIE-AI-02` ⏳ · `PIE-AI-03` ⏳ · `PIE-AI-04` ⏳ · `PIE-AI-05` ⏳
**Finita quando**: le voci hanno esito reale e i pesi eventualmente modificati sono committati
**Sblocca**: U6, U19, M6.6, M6.7

Partita con almeno un'unita' `bIsBotControlled`; il log utility deve mostrare coordinate
**assiali** `(q,r,L)`. Poi taratura: `TurnManager` nel World Outliner → Details ▸ *Bot*; i pesi
hanno effetto **dal turno successivo senza ricompilare** (voce `PIE-BU2b`). I default vengono
dal quadrato: su hex vanno riguardati, non dati per buoni.

#### U6 · Multilivello e partita completa 🟡

**Sbloccata da**: M6.8 · **Preparazione condivisa con**: U2, U3, U4, U5 · **Percorso critico**: sì
**Produce**: chiusura di M6 / E2 — sessione D verde
**Verifichi**: `PIE-HEXPLAY-8` 🟡 · `PIE-HEXPLAY-10` 🟡 · `PIE-HEXPLAY-4b` ⏳ · `PIE-FACING-1` ⏳
**Finita quando**: le nove voci `PIE-HEXPLAY` sono verdi, rilette tutte insieme
**Sblocca**: U16, U19, M6.8

Percorso che usa la transizione fra layer 0 e 1 — l'unita' deve **cambiare quota**, non
teletrasportarsi. Poi una partita intera, dall'avvio alla vittoria, rileggendo
`PIE-HEXPLAY-1..9` insieme.

⚠️ **«Rimuovi l'arco e verifica che il path fallisca» NON si fa in questa apertura** (corretto il
2026-08-10): su `MapSource=GeneratedTestArena` la transizione e' creata da codice
(`MakeTestArena`, `(1,0,0) -> (2,0,1)`) e non c'e' un asset da editare. Quella meta' vive su un
asset mappa vero, cioe' **U1**/**U13**, ed e' comunque gia' coperta headless da
`Structures.Bridge.RemovalBreaksPath` e `…NoTeleportOnRemoval`.

**Prima esecuzione parziale il 2026-08-10** (esiti nel registro): la partita e' arrivata a
conclusione ma **per scadenza dei round**, `Pareggio (round 5/5)` con una squadra in vantaggio
2 contro 1 — quindi la chiusura **per eliminazione** che `PIE-HEXPLAY-10` descrive non e' stata
vista. Il playtest ha fatto il suo mestiere: ha falsificato `RoundLimit 5`, che contraddiceva i
**10-14** del 2v2 fissati da D-010, e il formato spedito e' passato a **12** nello stesso
giorno. ⚠️ La partita completa va quindi **rigiocata col limite nuovo**: il ritmo misurato su
5 round («un lampo») non descrive piu' il gioco.

#### U23 · Autobattle registrata — la partita che si guarda, in PIE e su packaged —

**Sbloccata da**: E47.1, E47.3 · **Percorso critico**: sì
**Produce**: il video/log della partita non presidiata — l'evidenza che G10 chiede e la riserva che G13 dichiara
**Finita quando**: esiste una registrazione di una partita 2v2 conclusa senza un solo input, eseguita due volte — PIE e pacchetto Development

Attivare la modalita' non presidiata (**E47.1**), avviare su **mappa esagonale multilivello** — non
sull'arena generata di test: e' precisamente la riserva che G13 dichiara — e **guardare**.
Registrare video o sequenza di screenshot **piu'** il log della sessione, poi ripetere sul pacchetto
Development.

⚠️ **Perche' due esecuzioni e non una.** Sono due domande diverse — «le regole girano» e «girano
fuori dall'editor» — e il repository ha gia' pagato la differenza: al primo tentativo di packaging
di M7.4 i materiali referenziati via `TSoftObjectPtr` non venivano cookati, e le unita' erano grigie.
Un video in PIE non lo avrebbe mai mostrato.

⚠️ **Perche' servono ENTRAMBI video e log.** Il log non mostra la leggibilita' della board (CP 47.3);
il video non porta i reason code. La DoD chiede evidenza allegata, e nessuno dei due da solo la
soddisfa per intero.

⚠️ **`unblocks: []` non e' una dimenticanza.** Questa seduta **non sblocca U6**: le nove voci
`PIE-HEXPLAY` restano da eseguire una per una, ciascuna con la propria domanda. E47 cambia il
**costo** di eseguirle — da «gioca una partita intera» a «guardala» — non la loro natura, ed e'
scritto qui perche' la coda non la mostri come una scorciatoia verso M6.8.

⚠️ **`verifies: []` non e' una lacuna: e' l'unica lettura onesta oggi.** Le due voci che questa
seduta *sembrerebbe* chiudere sono gia' rivendicate da **U6** — `PIE-HEXPLAY-10` e
`PIE-V01-MATCHEND` — e una voce con due sedute che se la contendono non dice piu' chi la chiude.
U23 le rende **piu' economiche**, non le esegue al posto di U6.
Vale la pena registrare anche l'esclusione che si era tentata per prima: `PIE-V01-MATCHLEN` non
entrerebbe comunque, perche' la sua precondizione dice «partita 2v2 completa **giocata da un umano**
contro i bot, cronometro alla mano» — una partita che si gioca da sola non risponde a quella
domanda. `PIE-HEXPLAY-10` invece la dichiara nella propria precondizione («almeno un'unita' per
squadra col bot»), ed e' proprio per questo che appartiene a U6 e non qui.

⚠️ **Le voci nuove — `PIE-V01-BOARD`, `PIE-V01-PACKAGED` — non sono state aperte, e restano da
scrivere.** *(Fino al 2026-08-20 `docs/technical/test-manuali-pie.md` stava nel `writable` della
track `playtest` e non lo si poteva toccare da qui; con `D-178` il write-set di batch e' stato
rimosso e non c'e' piu' un owner da attendere.)*
🔴 L'attribuzione sbagliata e' stata ereditata dal corpo di E46, che porta lo stesso errore, ed e'
stata trovata in code review. Cambia la conseguenza pratica: **`playtest` e' IDLE**, quindi il
blocco non e' un'attesa su una sessione viva ma una **riallocazione da dichiarare** — che e' una
cosa che si puo' fare oggi, non domani. Le due voci si aprono col prossimo batch, insieme alle sei
`PIE-V01-FRONTEND-*` che E46 ha lasciato nella stessa condizione.

⚠️ ID assegnato prima del merge: `U23`, con `U22` come ultimo su `main` e su tutti i branch remoti
misurati il 2026-08-16 (`git show origin/<branch>:docs/roadmap/editor-sessions.yaml`). Chi arriva
secondo rinumera, non contende.

### Blocco 3 — Presentazione (M8)

*Sbloccato da subito: il C++ e' gia' in `main`. ⚠️ **Dal 2026-08-10 e' percorso critico**, e prima diceva l'opposto — «nessuna di queste sedute blocca la v0.1». Era un disallineamento col canone, non una scelta: **E21 «Presentazione e leggibilita'»** e' P1 e sta **dentro** lo scope di release (`v0.1 = E1-E21`), e queste tre sedute sono il suo lavoro in editor. Confermato dall'autore: per la v0.1 i modelli e le animazioni dei personaggi ci devono essere. Il `done_when` di U9 — **nessun cilindro in campo** — e' il modo in cui lo si verifica.*

#### U7 · Personaggi Paragon 🟡

**Sbloccata da**: — · **Preparazione condivisa con**: U8, U9 · **Percorso critico**: sì
**Produce**: i quattro Blueprint-unita' del roster v0.1, committati
**Artefatti**: `Content/RT/Characters/Gadget/Blueprints/BP_Unit_Gadget.uasset` ✅ · `Content/RT/Characters/Phase/Blueprints/BP_Unit_Phase.uasset` ✅ · `Content/RT/Characters/Riktor/Blueprints/BP_Unit_Riktor.uasset` ✅ · `Content/RT/Characters/Wraith/Blueprints/BP_Unit_Wraith.uasset` ✅
**Verifichi**: `PIE-AS2` 🟡 · `PIE-FACING` ✅
**Finita quando**: i quattro Blueprint sono tracciati da git e le due voci hanno esito reale sui BP nuovi
**Sblocca**: U8, U19

**Un Blueprint per eroe, ma intitolato al PACK.** La base visuale la fissa **D-037** (tabella
owner in `docs/characters/paragon.md`); i nomi degli asset e delle cartelle seguono il pack
Paragon, non l'eroe (deciso 2026-08-11 — vedi `notes`):

| Eroe | `HeroId` | Pack | Blueprint | Skeletal Mesh da assegnare |
|---|---|---|---|---|
| Gadget | `Hero.Gadget` | `Paragon.Gadget` | `BP_Unit_Gadget` | `…/ParagonGadget/Characters/Heroes/Gadget/Meshes/Gadget` |
| Phase | `Hero.Phase` | `Paragon.Phase` | `BP_Unit_Phase` | `…/ParagonPhase/Characters/Heroes/Phase/Meshes/Phase_GDC` |
| Riktor | `Hero.Riktor` | `Paragon.Riktor` | `BP_Unit_Riktor` | `…/ParagonRiktor/Characters/Heroes/Riktor/Meshes/Riktor` |
| Wraith | `Hero.Wraith` | `Paragon.Wraith` | `BP_Unit_Wraith` | `…/ParagonWraith/Characters/Heroes/Wraith/Meshes/Wraith` |

⚠️ **La mesh di Phase NON si chiama `Phase`**: e' `Phase_GDC` (22,6 MB). Gli altri file in quella
cartella pesano 0,1 MB — sono extents, shadow e skeleton. Verificato sul disco il 2026-08-11.
⚠️ Si scrive **`Paragon.Gadget`, mai `Gadget` nudo** quando si parla del pack: `Gadget` da solo e'
gia' una categoria di equipaggiamento (`ERTEquipmentSlot::Gadget`).

1. I pack stanno in `Content/FabAsset/Paragon/` — path `/Game/FabAsset/Paragon/<Pack>/…`, mai
   `/Game/<Pack>/…` (`convenzioni-contenuti-ue.md` appendice B).
2. Un Blueprint in `/Game/RT/Characters/<Pack>/Blueprints/BP_Unit_<Pack>`, classe base
   **`ARTUnit`**. I pack di terze parti restano **fuori** da `/Game/RT`: qui ci va il Blueprint,
   non il pack.
3. ⚠️ **Il cilindro non si sostituisce, si nasconde.** `ARTUnit::Mesh` e' uno
   `UStaticMeshComponent`: nel Blueprint si **aggiunge** uno `USkeletalMeshComponent` e si toglie
   la spunta `Visible` al cilindro, che il C++ usa ancora per selezione e fallback.

   🔴 **Attaccalo a `SceneRoot`, non a `Mesh`.** Dal **2026-08-16** (`#593`) il root di `ARTUnit`
   e' un `USceneComponent` neutro chiamato `SceneRoot`, e `Mesh` e' un suo figlio che porta
   `BaseMeshScale = (1.2, 1.2, 1.8)`. Chi attacca la skeletal al cilindro ne eredita la scala e
   ricostruisce a mano la deformazione che quella modifica ha tolto. Nel pannello Components la
   skeletal va **trascinata sotto `SceneRoot`**; se il nodo padre dice `Mesh`, non e' ancora a
   posto.
4. **`VisualZOffset = 0`.** Il default e' `UnitHalfHeight` (90), giusto per il cilindro che ha il
   pivot al CENTRO; i personaggi UE ce l'hanno ai PIEDI. Lasciarlo fa fluttuare l'unita' a 90 cm.
5. `TeamRingMaterial` e `SelectionRingMaterial` → `M_TeamRing` / `M_SelectionRing` in
   `/Game/RT/Characters/Shared/Materials/`. Il colore lo mette il codice sul MID; assenti,
   l'anello resta nascosto senza rompere nulla.
6. **Scala del componente skeletal: `1,1,1`, relativa.** Con il passo 3 fatto — skeletal sotto
   `SceneRoot` — non c'e' piu' niente da compensare: il root e' unitario e la selezione scala
   `Mesh`, che le sta accanto (`RTUnit.cpp:266`). ⛔ **Non commutare l'icona su World/Absolute**:
   era il workaround dell'era in cui il cilindro era il root, e su una gerarchia sana congela la
   dimensione contro la scala dell'attore lasciandone scorrere la posizione.

   🔴 **Sui quattro `BP_Unit_*` GIA' ESISTENTI il passo 3 non si e' applicato da solo**, e questa
   e' la prima cosa da fare in questa seduta. I loro nodi SCS dichiarano il genitore **per nome**:
   finche' esiste un componente nativo chiamato `Mesh` — ed esiste — la skeletal resta figlia di
   lui. L'ordine e' obbligato, e invertirlo deforma i personaggi invece di raddrizzarli:

   > **(a) MISURA prima.** Apri i quattro BP e guarda **come e' compensata oggi** la deformazione:
   > icona *World/Absolute* sulla riga Scale, oppure una `RelativeScale3D` tarata a mano, oppure
   > **niente** — nel qual caso i personaggi sono stirati adesso.
   > ⚠️ La misura statica restringe il campo ma non chiude: cercando ogni stringa in ASCII **e**
   > UTF-16LE nei quattro `.uasset`, `bAbsoluteScale` e' **assente in tutti e quattro** (l'icona
   > non e' mai stata commutata) mentre `RelativeScale3D` e' **presente in tutti e quattro**. Il
   > candidato piu' probabile e' quindi una scala relativa tarata a mano — ma il nome della
   > proprieta' e' comune e non dice su quale componente stia ne' con che valore. Lo dice
   > l'editor: e' questo il passo (a).
   > **(b) RIPARENTA** la skeletal sotto `SceneRoot`.
   > **(c) Solo allora TOGLI** l'eventuale compensazione, che a quel punto e' davvero ridondante.

   ⚠️ **Questa seduta non ha una voce PIE che la falsifichi**, e va aggiunta. *(Fino al 2026-08-20
   non si poteva scriverla da qui: `docs/technical/test-manuali-pie.md` era nel `writable` della
   track `playtest`; `D-178` ha rimosso quel vincolo.)* L'invariante da scrivere e' una sola: *a
   schermo, un
   personaggio selezionato non cambia dimensione e non e' piu' alto della sua silhouette a riposo*.
7. Registra ciascuno in **`HeroUnitClasses`** del `RTGameMode` — `TMap` con chiave l'`HeroId`
   (`Hero.Gadget` → `BP_Unit_Gadget`, …). ⚠️ **E' il passo che sbaglia in silenzio**:
   `RTGameMode.cpp` fa `HeroUnitClasses.Find(Hero->HeroId)` e senza corrispondenza spawna
   `ARTUnit::StaticClass()`, cioe' il cilindro. Un Blueprint perfetto ma non registrato non
   viene mai istanziato.

   🔴 **Non confonderla con `Team0Heroes`/`Team1Heroes`**, che le stanno **accanto nella stessa
   categoria** `RefactorTactics|Units` e sono la trappola vera di questa seduta — ci si e' caduti
   **due volte** il 2026-08-11. Rispondono a domande diverse:

   | | `Team0Heroes` / `Team1Heroes` | `HeroUnitClasses` |
   |---|---|---|
   | risponde a | **chi** scende in campo | **con che aspetto** |
   | tipo | lista: un campo per riga | mappa: **due campi** per riga |
   | contiene | `Hero.Gadget` | `Hero.Gadget` → `BP_Unit_Gadget` |

   Se la riga che stai compilando ha **un solo campo**, sei nella proprieta' sbagliata. Mettere i
   nomi dei Blueprint nelle formazioni produce `BP_Unit_Gadget non e' nel catalogo eroi` e
   **`0 eroi`** in campo — il log nomina l'id introvabile, ed e' il primo posto dove guardare.
   Per non sbagliare: filtra il pannello Details scrivendo `Hero Unit` nella barra di ricerca,
   cosi' resta visibile solo la mappa.
8. **Le statistiche non si toccano**: `MaxHealth`, `AttackPower`, `MoveRange` arrivano da
   `URTHeroData`. Scriverle nel Blueprint significa scrivere numeri che il catalogo sovrascrive.

Procedura per animazioni e montaggi: `guida-animazioni-paragon.md` §AS.3 e §AS.4.

> ⚠️ **Riscritta il 2026-08-10, era obsoleta su tre punti insieme**: chiedeva `BP_Unit_Guardian`
> (Gideon) e `BP_Unit_Ranger` (Sparrow) da assegnare a `GuardianUnitClass`/`RangerUnitClass`.
> Gli **archetipi** Guardian/Ranger sono stati rimossi, quei due campi **non esistono piu'** nel
> codice (oggi c'e' `HeroUnitClasses`, per `HeroId`), e Gideon/Sparrow in `paragon.md` sono
> *Candidate*: nessuno dei due e' la base visuale di un eroe della v0.1. Seguendola si sarebbero
> costruiti due Blueprint che il gioco non istanzia.

> **I quattro pack sono tutti sul disco** (verificato 2026-08-11): Gadget 1232 file, Phase 1155, Riktor 1261, Wraith 1322. `ParagonGadget` mancava ed e' stato portato con la procedura `convenzioni-contenuti-ue.md` **B.2a** (magazzino + rename headless + copia). I pack non sono nel repo (`/Content/FabAsset/` e' ignorato): chi clona se li scarica. ⚠️ **Naming deciso dall'autore il 2026-08-11**: cartelle e asset di questa seduta portano il nome del **pack Paragon**, non dell'eroe, «per non creare problemi» — in editor si vede `Gadget` e si cerca `Gadget`. Ribalta `convenzioni-contenuti-ue.md` §A, che raccomandava l'opposto perche' il nome del pack lega l'asset a una mesh sostituibile: il rischio resta, ed e' accettato consapevolmente. Se un eroe cambiasse base visuale, il Blueprint andrebbe rinominato. ⚠️ **Eccezione dichiarata**: i data asset eroe (`DA_Hero_Flux`, …) restano intitolati all'**eroe** pur stando nella cartella del pack. Sono dati di gioco, non presentazione: non dipendono dalla mesh, e `HeroId` in C++ resta `Hero.Gadget`. ⚠️ Se una mesh appare **senza materiali** non e' un errore di questa seduta: sono i soft reference di A.6 — 9 asset su 1229 in Gadget, 6 su 1698 in Gideon che e' in uso da giorni. ⚠️ **E se un personaggio appare in T-pose, schiacciato o con catene lunghissime, NON e' un difetto**: senza anim BP la skeletal resta nella **posa di riferimento** dello scheletro, dove le ossa di catene e tentacoli stanno distese in fila. Riscontrato su `Riktor` il 2026-08-12. Il controllo che lo isola in dieci secondi: **apri la Skeletal Mesh nel Content Browser** — se appare cosi' anche li', fuori dal gioco e fuori dal Blueprint, e' la bind pose e la sistema **U8**. Se invece li' e' normale e in partita no, allora guarda il Blueprint.

#### U8 · Animazioni ⏳

**Sbloccata da**: — · **Preparazione condivisa con**: U7, U9 · **Percorso critico**: sì
**Produce**: gli anim BP dei quattro personaggi e i montaggi Cast/Hit/Death
**Artefatti**: `Content/RT/Characters/Gadget/Animation/ABP_Gadget.uasset` ⏳ · `Content/RT/Characters/Phase/Animation/ABP_Phase.uasset` ⏳ · `Content/RT/Characters/Riktor/Animation/ABP_Riktor.uasset` ⏳ · `Content/RT/Characters/Wraith/Animation/ABP_Wraith.uasset` ⏳
**Verifichi**: `PIE-AS4a` ⏳ · `PIE-AS4b` ⏳
**Finita quando**: i quattro anim BP sono tracciati da git e le due voci hanno esito reale
**Sblocca**: U9, U19

Procedura completa in `guida-animazioni-paragon.md` §AS.4a (locomozione Idle↔Run pilotata dai
delegate, **non** da `GetVelocity`) e §AS.4b (montaggi via eventi C++), poi si ripete per gli
altri tre eroi.

**Il nome segue il PACK** (deciso 2026-08-11, come U7): `ABP_Gadget`, `ABP_Phase`,
`ABP_Riktor`, `ABP_Wraith`. La collocazione e' `/Game/RT/Characters/<Pack>/Animation/` — non
`Blueprints/`, che ospita i `BP_Unit_*`.

Ogni anim BP va costruito sullo scheletro **che la sua mesh referenzia** — letto dall'asset, non
dedotto dal nome del file (corretto il 2026-08-11, vedi sotto):

| Pack | Mesh assegnata in U7 | Skeleton da usare |
|---|---|---|
| `Paragon.Gadget` | `Gadget` | `Gadget_Skeleton` |
| `Paragon.Phase` | `Phase_GDC` | `phase_Skeleton` |
| `Paragon.Riktor` | `Riktor` | `Riktor_Skeleton` |
| `Paragon.Wraith` | `Wraith` | `Wraith_Skeleton` |

⚠️ **Lo skeleton si LEGGE dalla mesh, non si cerca nella cartella.** Una prima stesura di questa
tabella dava `gadget_bot_Skeleton` e `belly_Riktor_Skeleton`, presi perche' erano il primo file
`*Skeleton*` di quella cartella: appartengono pero' a **mesh diverse e piu' piccole**
(`gadget_bot`, `belly_Riktor`), non a quelle che U7 assegna. Assegnarli avrebbe prodotto un anim
BP legato a uno scheletro incompatibile — un errore silenzioso, come quello del punto 6 di U7.
Il controllo, in editor: aprire la Skeletal Mesh e leggere il campo **Skeleton** nei Details.

> ⚠️ **Riscritta due volte.** Il 2026-08-10 chiedeva `ABP_Gideon` e `ABP_Sparrow` in
> `Blueprints/`: due anim BP intitolati a pack che non sono la base visuale di nessun eroe della
> v0.1, nella cartella sbagliata. Il 2026-08-11 i nomi sono passati dall'eroe al pack per scelta
> dell'autore — vedi le `notes` di U7 per il costo accettato.

#### U9 · Leggibilita' e riferimento visivo 🟡

**Sbloccata da**: — · **Preparazione condivisa con**: U7, U8 · **Percorso critico**: sì
**Produce**: il video (o gli screenshot) di riferimento — DoD di milestone di M8
**Verifichi**: `PIE-AS5` ✅ · `PIE-SEL` ✅ · `PIE-ICON-01` ⏳ · `PIE-FMT-01` ⏳
**Finita quando**: nessun cilindro in campo (salvo asset mancanti) e il riferimento visivo e' nel repo
**Sblocca**: M8.1, M8.2, M8.3

Assegna `M_TeamRing` e `M_SelectionRing` — esistono gia' in
`/Game/RT/Characters/Shared/Materials/` — sui Blueprint nuovi; il colore lo imposta il codice
sul MID, un solo materiale basta per entrambi gli anelli. Poi tara la camera sulla scala
esagonale (`Camera Pitch`, `Default Arm Length` sul `RTCameraPawn`, effetto immediato anche a
PIE avviato) e giudica a schermo se i colori delle superfici sono leggibili **in partita**,
non solo nell'overlay dell'editor.

> `PIE-AS5` e `PIE-SEL` sono gia' verdi sui cilindri: qui si **riverificano** sugli skeletal.

### Blocco 4 — Il contenuto diventa dati

*Grana media: il codice sotto e' specificato ma non scritto. Da rivedere all'apertura di E1 ed E6.*

#### U10 · Data asset delle azioni —

**Sbloccata da**: E1.3, E1.4 · **Percorso critico**: sì
**Produce**: il catalogo azioni della v0.1 come dati, non come codice
**Finita quando**: il validator di CP 1.4 rifiuta un asset volutamente invalido e accetta i tuoi
**Sblocca**: U11

Oggi in `Content/` non esiste **nessun** data asset di abilita': le abilita' sono di fatto
hard-coded (`roadmap-v0.1.md` §9 punto 3). Questa seduta chiude quel buco.
Il validator deve **rifiutare** ID duplicato, fallback mancante e variante senza svantaggio.

> **Conflitto da risolvere in CP 1.3, non qui.** `roadmap-v0.1.md` CP 1.3 chiede asset `PDA_*`; `convenzioni-contenuti-ue.md` §6 — documento **normativo** — assegna `DA_` ai Data Asset. Finche' non e' deciso, `artifacts` resta vuoto: un path scritto qui sarebbe una terza opinione.

#### U11 · I 4 eroi 🟡

**Sbloccata da**: E6, U10 · **Percorso critico**: sì
**Produce**: i data asset di Gadget, Phase, Riktor e Wraith, e lo spawn 2v2 che li usa
**Verifichi**: `PIE-V01-ROSTER` 🟡
**Finita quando**: i quattro asset sono tracciati da git e la voce ha esito reale
**Sblocca**: U12

Un asset eroe per personaggio con statistiche distinte (90/95/120/100 HP, 5/5/4/6 MP), poi una
partita per vedere che il bot gestisca MP diversi senza proporre mosse illegali.
Asset mancante = fallback al cilindro, previsto.

> `artifacts` resta vuoto finche' CP 1.3 non fissa prefisso e collocazione (vedi U10). Dipende da **U10** e non solo da E6: `URTHeroData::Actions` e' un `TArray<TObjectPtr<URTActionData>>`, cioe' un puntatore diretto agli asset azione. Senza il catalogo di U10 il campo non e' popolabile, e i quattro eroi non si possono committare.

#### U12 · Loadout —

**Sbloccata da**: E7, U11 · **Percorso critico**: no
**Produce**: varianti arma, gadget e moduli reazione come dati — 1 + 1 + 1 per eroe

> *Provvisoria*: E7 e' l'epic che la roadmap v0.1 dichiara tagliabile per prima se il tempo stringe. Dipende da **U11** per il contenuto, non per un puntatore: `URTEquipmentData` referenzia `GrantedActionId` (un `FName`) e non l'eroe, ma il loadout e' «1 + 1 + 1 **per eroe**» e senza i quattro asset eroe non c'e' niente su cui verificarlo.

### Blocco 5 — La mappa diventa un sistema

*Grana grossa: dipende da E8 ed E9.*

#### U13 · Arena v0.1 ⏳

**Sbloccata da**: E8, E9, U1 · **Preparazione condivisa con**: U1 · **Percorso critico**: sì
**Produce**: l'arena estesa con quanto serve alle verifiche di contenuto
**Verifichi**: `PIE-V01-COVEREDIT` ⏳
**Sblocca**: U14

Estende l'artefatto di U1 con: una cella `Terrain.Rough`, una zona d'acqua adiacente a una
superficie conduttiva, una **porta** su un passaggio obbligato, una **copertura bassa** su un
bordo esposto.

> Qui avviene la **migrazione di formato** annunciata in U1: `FRTHexCellData` guadagna il campo cover e la versione dell'asset sale. `DA_HexMap_Arena` e `DA_HexMap_Sandbox` vanno entrambi **migrati, non ricostruiti**.

#### U14 · Ambiente in partita 🟡

**Sbloccata da**: U13 · **Percorso critico**: sì
**Produce**: verdetto sulle regole ambientali e strutturali
**Verifichi**: `PIE-V01-COLL` 🟡 · `PIE-V01-ROUGH` 🟡 · `PIE-V01-DASHCOVER` 🟡 · `PIE-V01-PUSH` 🟡 · `PIE-V01-ELEC` ⏳ · `PIE-V01-FIREWATER` ⏳ · `PIE-V01-LOWCOVER` 🟡 · `PIE-V01-INTERCEPT` 🟡 · `PIE-V01-FF` 🟡 · `PIE-V01-FALLBACK` 🟡 · `PIE-V01-DOOR` 🟡
**Finita quando**: le undici voci hanno esito reale nel registro

> Le voci si aprono gradualmente man mano che E4, E5, E8 ed E9 chiudono: non tutte insieme.

### Blocco 6 — Chiusura della v0.1

#### U15 · HUD, intenti, log e comandi debug 🟡

**Sbloccata da**: E11 · **Percorso critico**: sì
**Produce**: verdetto su leggibilita' e osservabilita'
**Verifichi**: `PIE-V01-HUD` ⏳ · `PIE-V01-INTENT` 🟡 · `PIE-V01-LOG` 🟡 · `PIE-V01-DEBUG` 🟡 · `PIE-DEBUG-CELLS` ✅
**Finita quando**: le voci hanno esito reale nel registro
**Sblocca**: E11

Il punto che merita attenzione non e' l'HUD: e' che `rt.Debug.DrawIntent` **non deve** rivelare
gli intenti avversari (invariante #6, oggi banale perche' offline — ma e' ora che si crea
l'abitudine sbagliata).

#### U19 · Durata, ritmo e scala ⏳

**Sbloccata da**: U6, U1, U5, U7, U8 · **Percorso critico**: sì
**Produce**: numeri di playtest — non difetti
**Verifichi**: `PIE-V01-MATCHLEN` ⏳ · `PIE-V01-READY` ⏳ · `PIE-V01-OVERWATCH` ⏳ · `PIE-V01-MAPSCALE` ⏳
**Finita quando**: le quattro voci hanno un numero registrato, anche fuori target

**Non e' una seduta di caccia ai difetti: e' una seduta di misura.** Si gioca **una partita
intera fino alla fine**, senza fermarsi a indagare, e si annotano i numeri. Serve un cronometro
e `bRecordPacing` attivo sul `TurnManager` (il CSV finisce in `Saved/RT/`, fuori dal versionamento).

⚠️ **Non anticiparla** (deciso con l'autore il 2026-08-10). Aspetta **due** condizioni, ed e' il
motivo per cui `U5` e `U8` sono fra i suoi prerequisiti:

1. **Il bot gioca bene abbastanza** — cioe' il bot della v0.1 con i **pesi utility ritarati sulla
   scala esagonale**, che e' esattamente il prodotto di `U5`. Non serve E26 *Tactical Bot v1*, che
   e' post-v0.1: il gate e' la taratura, non la nuova architettura.
2. **In campo ci sono personaggi, non cilindri** — i modelli di `U7` e le animazioni di `U8`.

La ragione e' che questa seduta misura **ritmo e leggibilita' percepiti**, e nessuno dei due si
misura su cilindri mossi da un bot non tarato: si otterrebbero numeri veri di una partita che non
e' quella che si spedisce. Una misura presa troppo presto e' peggio di una misura assente, perche'
finisce nella tabella KPI e sembra un dato.

1. **Prima della partita**: conta i Move per attraversare la mappa da spawn a spawn e verifica
   che esistano **almeno due rotte** con trade-off diverso (`PIE-V01-MAPSCALE`). Il criterio e'
   definito in **U1 passo 7** — non condividono celle oltre agli estremi, costo entro un fattore
   1,5, numero diverso di celle esposte — e qui si **misura**, non si ridefinisce: U1 produce
   l'arena, questa seduta la cronometra.
2. **Durante**: a che secondo dichiari Ready nei round tipici, e se il countdown esiste e si
   annulla (`PIE-V01-READY`).
3. **A fine partita**: round giocati, durata a cronometro, round del primo contatto, via di fine.
4. **Solo dopo E14**: ripeti con un'unita' in Overwatch e giudica la finestra da 3 s.

> I numeri vanno nella tabella KPI di `v0.1-definition-of-done.md` §4 **con la riserva sul campione** (un solo giocatore, che e' l'autore) e **con l'etichetta 2v2**: i target di 25–30 min sono del 3v3 Standard, che in v0.1 non esiste. Un esagono r=4 e' **Skirmish** per costruzione. Nasce dalla sessione F del registro, che nessuna seduta U copriva.

#### U16 · Misura dei KPI 🟡

**Sbloccata da**: U6 · **Percorso critico**: sì
**Produce**: numeri reali nella tabella KPI
**Verifichi**: `PIE-V01-REPLAY` 🟡
**Finita quando**: i quattro KPI hanno un valore misurato, anche fuori target
**Sblocca**: E3.3, M7.3, E12.4

Nessuna guida copre questa procedura, quindi i passi stanno qui.

1. Partita in PIE sull'arena, `stat unit` e `stat game` dalla console → FPS client (target 60).
2. Unreal Insights per path, preview e resolver (target: path mediana < 2 ms, preview < 50 ms,
   resolver < 100 ms/turno).
3. `rt.Debug.DumpTurnLog` + `rt.Debug.VerifyReplay` per replay divergence = 0.
4. I numeri vanno **registrati anche se fuori target**: un valore misurato vale piu' di un ⏳.

> Distinta da U19: qui si misurano KPI **tecnici** (frame, millisecondi, determinismo), la' il *game feel* (durata, ritmo, scala). Mescolarle farebbe interrompere la partita a meta'.

#### U17 · Release v0.1 —

**Sbloccata da**: E12 · **Percorso critico**: sì
**Produce**: build Windows Development e Shipping, e una partita giocata senza editor
**Finita quando**: BUILD SUCCESSFUL su entrambe le configurazioni e una partita conclusa dalla packaged

> L'invocazione esatta di `RunUAT BuildCookRun` si fissa alla prima esecuzione riuscita e si scrive qui: non va inventata a tavolino.

#### U20 · Confine fra Guard e Brace ⏳

**Sbloccata da**: E5.2 · **Percorso critico**: no
**Produce**: verdetto di leggibilita' — un dato per `BAL-1`, non un difetto da correggere
**Verifichi**: `PIE-BAL1` ⏳
**Finita quando**: la voce ha un esito reale nel registro, in un verso o nell'altro

**Non e' una caccia ai difetti: e' una domanda di leggibilita'.** Tre unita' con
`PushResistance = 0` (Gadget, Phase, Wraith) — una in `Action.Guard`, una in `Action.Brace`, una
senza difesa — e una Phase avversaria che usa `Hero.Phase.PressureJet` su ciascuna nello stesso turno.
E' l'unica azione del gioco che porta danno **e** spinta nello stesso colpo.

1. Guarda il turno **senza leggere i numeri**. «Questo si e' piantato» dev'essere distinguibile
   da «questo ha parato».
2. Solo dopo, controlla il log: 1 danno per chi e' in `Guard`, 6 per chi e' in `Brace`,
   **nessuno dei due arretra**.
3. Ripeti con **due** colpi sullo stesso bersaglio: qui l'ordine si inverte (17 contro 12), ed
   e' il solo caso in cui `Brace` conviene.

> ⚠️ **Non mettere due attaccanti sullo stesso bersaglio nello stesso passaggio**: il resolver
> esclude i bersagli spinti da 2+ attaccanti (forze contraddittorie). Misureresti quella
> regola, non questa.

> Nasce dal piano `BAL-1` (`plans/bal-1-guard-brace-roadmap-2026-08-10.md` §6). **Non entra nel subset `RELEASE-V01`**: `BAL-1` non blocca la consegna, e un gate che si allarga senza motivo e' il difetto che G9 ha gia' avuto due volte. La seduta ha una voce sola di proposito — e' una domanda che si risponde una volta, guardandola. ⚠️ ID assegnato al merge: preso `U20` con `U19` come ultimo su `main`. Chi arriva secondo rinumera, non contende.

#### U21 · Luci del graybox e inquadratura della mappa ✅

**Sbloccata da**: — · **Preparazione condivisa con**: U22, U25, U26 · **Percorso critico**: no
**Produce**: verdetto su leggibilita' della scena e inquadratura, piu' il livello illuminato committato
**Verifichi**: `PIE-MAPED-LIGHT` ✅ · `PIE-MAPED-FRAME` ✅
**Finita quando**: le due voci hanno un esito reale e il livello illuminato e' committato
**Sblocca**: U22, U25, U26

> Nasce dal referto `plans/map-sketch-editor-spec-panel-2026-08-12.md` (`P6`). E' una seduta e non una issue di codice per una ragione strutturale: `L_DevSandbox.umap` e' un `.umap`, e questo repository non modifica `.umap` da riga di comando. ⚠️ `artifacts` e' VUOTO di proposito, benche' la seduta committi un livello. L'oracolo degli artefatti e' `git ls-files`, che sa dire se un path esiste e non se e' stato MODIFICATO: `L_DevSandbox.umap` e' gia' tracciato da mesi, quindi dichiararlo qui farebbe derivare 🟡 («parte fatta») su una seduta non ancora aperta. Lo stato deriva dalle due voci PIE, che sono la cosa che davvero non esiste ancora. Il livello committato resta nella DoD della issue. ⚠️ Il sorgente chiedeva anche di ricostruire la navigazione della camera (MMB pan, RMB orbit, wheel zoom, WASD, F focus). **Il viewport di Unreal le fornisce gia' tutte**, e un `UEdMode` non possiede la camera del viewport: `RTCameraPawn` e' la camera di GIOCO, un oggetto diverso. Resta solo l'inquadratura della mappa, che e' `PIE-MAPED-FRAME`. ⚠️ ID assegnato al merge: preso `U21` con `U20` come ultimo su `main`. Chi arriva secondo rinumera, non contende. --- **COSA FAI** — i passi stanno qui perche' nessuna guida copre ancora l'illuminazione: la checklist di `convenzioni-contenuti-ue.md` §12 riguarda dove va un asset, non come si accende una scena. Stessa ragione per cui `U1` e `U16` portano i propri. **0. Misura il prerequisito invece di assumerlo.** Entrambe le voci PIE chiedono celle su **≥2 layer**, e `PIE-MAPED-FRAME` chiede anche celle **lontane dall'origine**. Se `L_DevSandbox` non le ha, dipingile prima: tool **Paint** con `BrushRadius` alto per il piano 0, `ActiveLayer=1` per il secondo, tool **Arch** per una transizione. `rt.Arena.Check` dice se la mappa ha celle; la vista `Focus` dice se i piani sono due. Cosi' facendo cambia anche `Data/DA_HexMap_Sandbox.uasset`, ed e' il motivo per cui la lease copre **entrambi** i package. **1. Le luci, e perche' sono piu' di una.** Il criterio di `PIE-MAPED-LIGHT` vieta esattamente *«una meta' leggibile e una no»*, che e' la firma di una **sola** luce direzionale: la faccia opposta al sole non riceve niente. Serve quindi anche una sorgente ambientale che riempia l'ombra, e qualcosa che quella sorgente possa raccogliere. Il sorgente proponeva `Directional Light` + `Sky Light` + `Sky Atmosphere`: la combinazione e' ragionevole, ma il criterio e' l'**orbita**, non la lista degli attori — chi la ottiene con meno pezzi ha comunque ragione. **2. L'esposizione si fissa NEL LIVELLO, non nella viewport.** Il prerequisito dice *«nessuna modifica all'esposizione della viewport»*, e non e' un dettaglio procedurale: se la scena si legge solo dopo aver alzato l'esposizione a mano, allora non si legge — e le altre tre sedute che ereditano questo allestimento vedrebbero altro. Un `Post Process Volume` **unbound** nel livello e' lo scope giusto. ⛔ **NON toccare `Config/DefaultEngine.ini`** per l'auto-exposure di progetto: cambiare un default di progetto per un banco di prove distruttive e' il caso che `#926` ha gia' respinto — «paga un binario e cambia il comportamento predefinito del gioco che si distribuisce». *(La riga citava anche `Config/` come `integration_only` nel batch: quel vincolo e' decaduto con `D-178`, la ragione di merito no.)* **3. Il verdetto e' un'orbita, non uno screenshot.** Gira attorno alla mappa e guarda le facce che il sole non prende: superficie, marcatori blocca-movimento/blocca-vista e contorni dei piani di contesto devono restare distinguibili **da ogni angolo**. **4. `PIE-MAPED-FRAME` nella stessa apertura.** Con il mode Hex Map attivo, premi **`Home`**: la vista deve portare dentro **tutte** le celle, comprese quelle sui layer diversi da `ActiveLayer` e quelle lontane dall'origine. ⚠️ Il comando arriva con `#623` parte B: serve un editor compilato **dopo** quel merge, o il tasto non fa niente e il verdetto sarebbe sul binario sbagliato. **5. Salva e committa.** L'oracolo e' `git status` sui due package, non «l'ho salvato»: questo repository ha gia' avuto asset presenti su disco e non versionati. ⚠️ **Una seduta per volta su questo livello.** Due `.umap` non si fondono, e quattro sedute usano lo stesso allestimento: chi lo apre lo tiene finche' non ha salvato e committato. *(Fino al 2026-08-20 la regola passava per una Binary Asset Lease dichiarata nel batch; `D-178` ha rimosso quel meccanismo, il fatto fisico che lo motivava no.)*

#### U22 · Il gesto dell'autore — ghost, snap e Undo del tool Geometry ✅

**Sbloccata da**: U21 · **Preparazione condivisa con**: U21, U25, U26 · **Percorso critico**: no
**Produce**: verdetto su leggibilita' del ghost, percepibilita' dello snap e granularita' dell'Undo
**Verifichi**: `PIE-GEO-GHOST` ✅ · `PIE-GEO-SNAP` ✅ · `PIE-GEO-UNDO` ✅ · `PIE-GEO-RESIDUI` ✅
**Finita quando**: le quattro voci hanno un esito reale, e il .umap resta pulito dopo la seduta

> Nasce da #712, il gesto dell'autore. La seduta esiste perche' QUATTRO voci del suo DoD non sono osservabili headless: ghost, snap, Undo e residui vivono nell'occhio di chi disegna, non in una asserzione. La parte verificabile e' gia' nel runtime — `SnapToGrammar` con i suoi due test, `ValidateSegment` con i cinque di #620, `BakeCell` con i sette di #621 — e il tool d'editor NON contiene una sola regola: misurato, tre chiamate al runtime e zero logica duplicata. ⚠️ `unblocked_by: [U21]` non e' una dipendenza tecnica ma pratica: `L_DevSandbox` va illuminato prima, o il ghost si valuta su una scena in cui non si vede niente — e il verdetto direbbe piu' sulle luci che sul tool. ⚠️ `PIE-GEO-RESIDUI` chiede anche un `git status` pulito sul `.umap`: la geometria non si salva nel livello, ed e' l'unico modo di accorgersene: nessun test headless apre un `.umap`. ⚠️ ID assegnato prima del merge: `U22`, con `U21` come ultimo su `main` e su tutti i branch remoti. Chi arriva secondo rinumera, non contende.

#### U24 · I `WBP_RT_*` del frontend — banner, modale d'errore, loading e root ✅

**Sbloccata da**: — · **Percorso critico**: no
**Produce**: i primi cinque widget del frontend, sotto `/Game/RT/UI/Framework/`
**Artefatti**: `Content/RT/UI/Framework/WBP_RT_FallbackBanner.uasset` ✅ · `Content/RT/UI/Framework/WBP_RT_ErrorModal.uasset` ✅ · `Content/RT/UI/Framework/WBP_RT_LoadingScreen.uasset` ✅ · `Content/RT/UI/Framework/WBP_RT_FrontendRoot.uasset` ✅ · `Content/RT/UI/Framework/WBP_RT_ModalLayer.uasset` ✅
**Finita quando**: i cinque `.uasset` esistono e si aprono senza errori; i TRE con una classe base (`WBP_RT_FallbackBanner`, `WBP_RT_ErrorModal`, `WBP_RT_LoadingScreen`) ereditano da quella dichiarata e leggono il dato invece di comporlo; i DUE strutturali (`WBP_RT_FrontendRoot`, `WBP_RT_ModalLayer`) non creano ne' rimuovono widget
**Sblocca**: E46.3, E46.5, E46.6

> ✅ **CHIUSA il 2026-08-18 — cinque `.uasset` su cinque, PR #1178 (`1ff48baf`).** Le otto `verification` delle due Binary Asset Lease sono consuntivate e le lease rilasciate: due misurate headless (la matrice incrociata delle classi base, e zero `AddToViewport`/`RemoveFromParent`/ `CreateWidget` con controprova **2/1/3** sul navigatore), sei eseguite dall'autore in editor — `load_in_editor` e `save_without_errors` non sono surrogabili da un test, perche' caricare la generated class non e' aprire il Blueprint. ⚠️ **`verifies: []` resta vero e resta il punto scomodo**: `PIE-V01-FRONTEND-NAV` e `-ERROR` non esistono ancora, e quel registro e' di `playtest` — le voci si **propongono in handoff**. Questa seduta ha prodotto asset e non ha chiuso nessuna voce del registro, esattamente come dichiarava. ⏳ **Cio' che resta non e' d'editor**: nessuno chiama `InitializeFrontend`, che e' di CP 46.3 (`#938`). I cinque Blueprint esistono, sono provati, e non li avvia nessuno. 🔴 **La seduta e' chiusa, `#937` NO — e le due cose sono state confuse per un merge.** Una voce del DoD non e' implementata: il `BACK` del modale deve fare `PopScreen` prima dell'avvio e `ReturnMain` a partita viva, e misurando i package si trova la stringa «Back» in `WBP_RT_ErrorModal` con **zero** `ReturnMain`/`PopScreen`/`RequestBack`. Il pulsante e' disegnato e non chiama niente. ∴ **`done_when` di questa seduta resta soddisfatto**: chiedeva cinque `.uasset` che si aprono, con il criterio giusto per ciascuno, e quelli ci sono. Una seduta d'editor consegna asset; il collegamento al navigatore e' codice, e non era mai stato suo. ───── ➕ **Seduta aperta il 2026-08-16, a lavoro gia' cominciato**: i widget si stavano costruendo e il tracking non li nominava — ne' qui, ne' come `binary_leases`. `grep -n "WBP_RT_" editor-sessions.yaml` dava **zero**, e nessuna lease copriva `Content/RT/UI/`. *(Il meccanismo delle lease e' stato rimosso con `D-178`; il rischio che intercettava resta: due binari non si fondono, quindi un `.uasset` si tocca da un lavoro solo per volta.)* **Cosa costruire e in che ordine** sta in [`../technical/runbooks/guida-frontend-umg.md`](../technical/runbooks/guida-frontend-umg.md), che e' l'owner del *come*; la spec del *cosa* e' `spec-frontend-navigazione.md`. Qui c'e' solo l'esistenza della seduta e il suo write-set. ⚠️ **`verifies: []` non e' una dimenticanza, ed e' il punto piu' scomodo di questa seduta**: `PIE-V01-FRONTEND-NAV` e `-ERROR` **non esistono** — misurato, `grep -c "PIE-V01-FRONTEND" docs/technical/test-manuali-pie.md` da' **0** — e quel registro appartiene alla track `playtest`. Le voci si **propongono** in handoff, non si scrivono da fuori: finche' non ci sono, questa seduta produce asset e non chiude nessuna voce del registro. ⚠️ **Il primo e il secondo widget si verificano subito, il terzo no.** Il dato del banner lo produce gia' `ARTGameMode` e quello del modale si forza con un formato invalido; il `LoadingScreen` ha il suo dato ma un allestimento **istantaneo**, quindi a schermo non si vede — la guida lo dice al suo §6, e va saputo prima di concludere che il widget sia rotto. ⚠️ **Nessuno chiama ancora `InitializeFrontend`**: il navigatore esiste, testato, e non lo avvia nessuno — l'aggancio e' di **CP 46.3** (`#938`). Fino ad allora questi Blueprint si provano solo a mano, chiamando le funzioni da un livello di prova. ⛔ **Fuori da questa seduta**: `WBP_RT_MainMenu`, `WBP_RT_ResultScreen` e `WBP_RT_PauseMenu` sono di `#938`, `#940` e `#941` · `WBP_RT_TacticalHUD` e' l'HUD in-match e ha gia' il suo root (CP 11.7) · le due schermate del replay (`WBP_RT_MatchHistory`, `WBP_RT_ReplayViewer`) sono di `#472`, e aspettano una lease propria. 🔴 **Il `done_when` chiedeva ai cinque widget una cosa che solo tre possono fare, e riscriverlo e' del 2026-08-18.** Diceva *«ereditano dalle classi base giuste»*: le classi base dichiarate in `RTFrontendWidgets.h` sono **tre** — `URTLoadingScreenWidgetBase`, `URTErrorModalWidgetBase`, `URTFallbackBannerWidgetBase` — e per `WBP_RT_FrontendRoot` e `WBP_RT_ModalLayer` non esiste una «classe base giusta»: erediterebbero da `UUserWidget` nudo, e il criterio diventava **vacuo** esattamente sui due widget che nessun test copre. `RTFrontendWidgetAssetTests.cpp` ne conosce **tre** su cinque, misurato. ✅ **Decisione dell'autore: i due restano nei cinque** — sono widget, non impalcatura da togliere dal piano. Cambia il criterio, non lo scopo: per loro vale `no_widget_creates_widgets`, che e' l'invariante 1 di CP 46.1 e l'unica cosa che un contenitore strutturale puo' violare. Un `.uasset` e' il solo posto dove `AddToViewport` puo' rientrare senza che un test se ne accorga. ⚠️ ID assegnato prima del merge: `U24`, con `U23` come massimo misurato su `main` **e su tutti i tredici branch remoti** — non solo sul proprio. Chi arriva secondo rinumera, non contende.

#### U25 · Il volume di posa della cella, e la scena che dice se il graybox si legge —

**Sbloccata da**: U21 · **Preparazione condivisa con**: U21, U22, U26 · **Percorso critico**: no
**Produce**: verdetto di leggibilita' del kit graybox e il volume di posa come guida d'editor
**Finita quando**: la scena di validazione esiste e le sue voci PIE hanno un esito reale, in un verso o nell'altro

**Non e' una caccia al bello: e' una domanda sola, ripetuta a tre distanze.** *Guardando la scena
senza HUD e senza selezionare niente, so dire che cosa ho davanti?* Se la risposta e' no, si cambia
la grammatica **prima** di aggiungere altri asset — l'unica prescrizione del kit sorgente che il
contratto adotta senza emendarla.

⚠️ **Questa ricetta non ripete numeri e formule, e la ragione e' misurata.** Due stesure precedenti
li contenevano ed erano **sbagliate entrambe**, ciascuna in modo diverso, perche' chi le scrive non
puo' eseguire i passi: la quota del volume ignorava l'altezza per-cella, e la conversione dell'inset
ne invertiva la semantica. I valori vivono negli owner linkati qui sotto; questa lista dice **cosa
fare, in che ordine, e dove si sbaglia**.

**Prima di aprire l'editor — tre condizioni, tutte e tre bloccanti.**

· ⛔ **Una seduta per volta** su `Content/RT/Maps/Dev/L_DevSandbox/`. **Quattro** sedute dichiarano
lo stesso allestimento — `U21`, `U22`, `U25`, `U26` — e due aperte insieme sullo stesso `.umap`
producono due versioni che non si fondono. *(Era una Binary Asset Lease dichiarata nel batch fino
al 2026-08-20; `D-178` ha rimosso il meccanismo, non il vincolo.)*
E' l'unica delle tre la cui omissione **distrugge lavoro**.
· ✅ **`GBX-2` chiusa il 2026-08-18** da `D-171` (#1188): `Locked` si distingue da `Closed` con una
**traversa in rilievo** modellata sul pannello, quindi sono due mesh e non una ricolorata. Questa
precondizione **e' soddisfatta**; era la sola delle tre che bloccava la modellazione della porta.
· 🔴 **`U21` fatta**: una scena di leggibilita' valutata prima delle luci direbbe piu' sulle luci che
sul kit.

`GBX-1` — l'inset — **non** e' bloccante: si sceglie **qui**, guardando, ed e' l'unico modo di
validarlo. Il valore scelto si scrive in
[#1094](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1094), la issue che lo
possiede; scriverlo solo nel `.umap` significa perderlo.
⚠️ **`GBX-4` NON blocca questa seduta**, e va detto perche' una stesura precedente lo affermava:
`L_DevSandbox.umap` e il suo `DA_HexMap_Sandbox` sono **gia' nell'allowlist**, quindi il livello si
salva e si committa oggi. `GBX-4` morde solo se il volume diventa un **asset separato** sotto un
percorso non deciso.

**La quota, prima di tutto il resto.** E' il passo che si sbaglia, e viene per primo apposta.

· Il disco che rappresenta la cella e' **opaco**, e tutto cio' che sta sotto la sua faccia superiore
diventa invisibile. Un volume invisibile e un volume mai costruito si somigliano molto.
· ⚠️ **La quota della faccia NON e' una costante sola**: `RebuildInstances` somma l'**altezza della
cella** prima dello spessore del disco, quindi su una cella elevata la faccia sta piu' in alto di
quanto una formula a termine singolo suggerisca. Chi disegna a quota fissa affonda il volume in ogni
cella rialzata.
· ⚠️ **La costante dello spessore non e' raggiungibile**: `RTCellTopZ` e' `constexpr` in un namespace
anonimo, e condividerla e'
[#983](https://github.com/DegrassiAaron/refactor-tactics-main/issues/983), **aperta**. Qualunque
valore si usi qui e' una copia a mano che nessun compilatore verifica — e le altre due copie che
esistono in `Source/` non stanno meglio: il test che dovrebbe pinnarle confronta **due copie a mano**
fra loro. Vale la pena leggere `#983` prima di scegliere un numero.

**I passi.**

1. **Il volume**: prisma esagonale sul centro cella, vertici da `URTHexLibrary::HexCorners`, centro
   da `AxialToWorld` — mai angoli incisi a mano, per la ragione che
   [`../technical/systems/spec-hex-geometry-authoring.md`](../technical/systems/spec-hex-geometry-authoring.md) §4
   scrive gia' per la geometria.
2. **Il footprint sicuro** e' l'**outer footprint meno un inset**, non l'outer scalato:
   [`../technical/systems/spec-graybox-placement-contract.md`](../technical/systems/spec-graybox-placement-contract.md)
   §5 lo definisce e `GBX-1` ne fissa il valore. ⚠️ L'inset e' in frazioni di **`C`** mentre
   `HexCorners` vuole il **raggio**, e i due differiscono di `√3`: la conversione sta in §6.
3. **Le guide verticali** sono quelle di §6 — cinque, con le loro frazioni. Si leggono di la', non si
   ricopiano: sono gia' cambiate una volta.
4. **La scena, in COPPIE.** La domanda e' sempre *questo o quello?*
   · unita' · copertura **bassa vs alta** · muro **vs muro sfondato** · porta nei suoi **quattro**
   stati (`Open` · `Closed` · `Locked` · `Destroyed`) · acqua **vs** ghiaccio · **intatto vs
   distrutto**.
   🔴 **Le coppie che il kit sorgente non chiedeva sono quelle che contano**: senza `Locked` la
   seduta non verifica la traversa che `D-171` ha scelto — la domanda `GBX-2` e' chiusa, il suo
   **esito** no, e si guarda se quella traversa si legge; senza `Destroyed` non guarda lo stato
   **terminale** della porta; senza
   intatto/distrutto non verifica il punto (4) di `D-152` — *distrutto cambia geometria, non colore*.
   Una scena senza di esse soddisfa il `done_when` e **non risponde alla domanda**.
5. **Gli `EdgeBound` si posano con `EdgeMidpointWorld` e `EdgeRotation`**, mai a occhio: il bordo `E`
   di una cella **e'** il bordo `W` del vicino, e a mano si ottengono due barriere dove ce n'e' una.
6. **Tre distanze di camera** — ravvicinata, di gioco, tattica — e gli screenshot **anche in scala di
   grigi**: non e' accessibilita', e' il modo in cui `D-146` intende la ridondanza. Se due categorie
   si distinguono solo in colore, in grigio spariscono, e con esse la prova.
7. **Il volume non deve arrivare al giocatore.** Il criterio dell'owner (§5) e' la build **packaged**,
   non PIE: un attore nascosto in gioco puo' essere cotto lo stesso. Se una packaged non e'
   disponibile nella seduta, si dichiara che il controllo **non e' stato fatto** invece di
   sostituirlo con la prova piu' debole.

> ⚠️ **Il `.umap` DEVE risultare modificato a fine seduta**, ed e' il contrario di `U22`: li'
> `PIE-GEO-RESIDUI` chiede `git status` pulito perche' il tool di geometria non deve persistere le
> anteprime, qui il livello salvato **e' il prodotto**. Il controllo giusto e' l'inverso: che
> risultino modificati **solo** i package del livello, e nessun altro asset toccato per errore.

> 🔴 **Nessuno di questi controlli ha un posto dove registrare l'esito.** `verifies:` e' vuoto e il
> `done_when` chiede che «le sue voci PIE abbiano un esito reale»: le voci sono redatte in
> [#1096](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1096) e il registro
> appartiene a un'altra track. Finche' quella issue non atterra, la seduta produce screenshot e un
> giudizio che **nessun registro conserva**.

> ⏱️ **Derivata dal codice e dagli owner, non provata in editor.** I simboli sono verificati in
> `Source/`; la procedura no. Due stesure sono state riscritte in code review — la prima costruiva il
> volume dentro il disco, la seconda sbagliava la quota sulle celle elevate e invertiva l'inset. Chi
> la esegue per primo la corregge dove sbaglia ancora, ed e' il motivo per cui i passi **nominano
> owner e simboli invece di ripetere numeri**.

> ➕ **Seduta aperta il 2026-08-17** dal consolidamento del kit `Graybox_Kit_Cover_CellVolume` ([D-152](../decisions/RT_PDR_00_Decision_Log.md)). Owner del modello: [`../technical/systems/spec-graybox-placement-contract.md`](../technical/systems/spec-graybox-placement-contract.md); qui c'e' solo l'esistenza della seduta. **Due cose, e stanno insieme perche' la seconda misura la prima**: il **Cell Placement Volume** — prisma esagonale `EditorOnly` con footprint sicuro e guide verticali — e una **scena di validazione** che mette in campo unita', copertura bassa e alta, muro, porta nei suoi stati, acqua e ghiaccio, e li guarda a tre distanze di camera. ⚠️ **`verifies: []` non e' una dimenticanza**, ed e' lo stesso caso di `U24`: le voci PIE di questo dominio **non esistono** — misurato, `grep -c "PIE-GBX" ../technical/test-manuali-pie.md` da' **0**. *(Fino al 2026-08-20 quel registro era assegnato a un'altra track e le voci si potevano solo proporre; `D-178` ha rimosso il vincolo, quindi ora si scrivono.)* Finche' non ci sono, questa seduta produce una scena e non chiude nessuna voce del registro. ⚠️ **`artifacts: []` per la ragione di `U21`**: l'oracolo degli artefatti e' `git ls-files`, che sa dire se un path **esiste** e non se e' stato modificato. ✅ **Il secondo motivo e' CADUTO il 2026-08-18, e va ritirato invece che lasciato in piedi.** Questa riga diceva *«il percorso non e' deciso — `GBX-4`»*, e `D-173` (#1188) lo ha deciso: `/Game/RT/World/Graybox/`, con la riga d'allowlist scritta nello stesso commit — misurata con `git check-ignore -q`, che da' exit 1. Cio' che `asset-map.md` §6 chiedeva **prima** dell'asset adesso c'e'. ⏱️ **Ma la seduta non dichiara artefatti nemmeno ora, e la ragione e' un'altra**: finche' #1155 non atterra il mondo gira a `HexSize = 100` mentre si modella alla scala d'arte di `D-163`, quindi un volume finito e' `1,5x` fuori misura sulla mappa in cui si posa. Si modella alla scala nuova e si rimanda il **commit** — non il lavoro. ⛔ **Fuori da questa seduta**: i diciannove elementi del catalogo. Sette sono `DEFER` — tre per dipendenza da feature `IDEA`, due fuori scope v0.1 dichiarato, due proxy senza produttore — e dei dodici restanti la maggior parte e' `UPDATE` di presentazione su asset che esistono. Questa seduta porta **il volume e la scena**, cioe' gli strumenti con cui gli altri si giudicano — non gli altri. ⚠️ **`unblocked_by: [U21]` non e' una dipendenza tecnica ma la stessa di `U22`**: una scena di leggibilita' valutata prima che `L_DevSandbox` sia illuminato direbbe piu' sulle luci che sul kit. 🔴 **La conseguenza dei due campi vuoti INSIEME, che i due paragrafi qui sopra giustificano separatamente senza mai dirla.** Senza `verifies` e senza `artifacts` lo stato non e' derivabile: `project-graph.json` porta questa seduta con `state: "—"` e `queue_group: null`, e `editormap.shortlist.md` la conta solo fra le «senza stato derivabile» — **non compare ne' in READY ne' in WAITING**. `U24` sfugge al caso solo perche' ha artefatti. ∴ **questa seduta non entra in NESSUNA delle tre code** — `BLOCKING`, `READY`, `WAITING` — e per l'avanzamento vive attraverso `#1095`. ⚠️ *Due correzioni sulla stessa frase, in due tornate di review. Diceva «invisibile a ogni vista»: falso, `U25` compare nella tabella delle sedute e ha una sezione propria in `editormap.shortlist.md` — cio' che manca e' la CODA, non la visibilita'. Poi diceva «nessuna delle DUE code», e le code sono **tre**: `BLOCKING` e' quella da cui si pesca per prima, e scriverne due lasciava credere che fosse stata guardata.* Non e' riparabile qui: dichiarare voci PIE che non esistono sarebbe peggio — e' il difetto che `asset-map.md` §6 documenta. Si chiude quando uno dei due campi diventa vero. ⏱️ **Aggiornato il 2026-08-18**: questa riga diceva *«dichiarare artefatti prima che `GBX-4` scelga il percorso»* e chiudeva con *«e' il primo effetto utile della chiusura di `GBX-4`»* — cioe' **prevedeva** questo aggiornamento. `GBX-4` e' chiusa (`D-173`, #1188) e l'allowlist esiste, quindi il vincolo sugli artefatti non e' piu' il percorso: e' la scala, che aspetta #1155. *(La seconda condizione era sulle voci PIE e dipendeva da `D-139`: con `D-178` e' decaduta, e quelle voci ora si scrivono.)* ⚠️ ID assegnato prima del merge: `U25`, con `U24` come massimo misurato su `main` **e su tutti i diciassette branch locali e gli undici remoti**. Chi arriva secondo rinumera, non contende.

#### U26 · La griglia di lavoro e la sonda di movimento nell'editor —

**Sbloccata da**: U21 · **Preparazione condivisa con**: U21, U22, U25 · **Percorso critico**: no
**Produce**: verdetto su leggibilita' della griglia di lavoro e della sonda di movimento
**Finita quando**: le voci `PIE-*` che `#622` e `#711` creeranno hanno un esito reale

> ➕ **Seduta aperta il 2026-08-17 dal consolidamento Tactical Designer** ([D-154], referto `plans/tactical-designer-consolidamento-2026-08-17.md`). Nasce da un buco misurato: `#622` e `#711` chiedono ENTRAMBE, nel proprio DoD, una voce `PIE-*` «collocata in una **seduta** di `editor-sessions.yaml` — una voce che non sta in una seduta non viene eseguita mai», e nessuna seduta le riceveva. `#623` aveva `U21` e `#712` aveva `U22`; queste due erano le sole due issue d'editor aperte senza un posto dove atterrare. ⚠️ **`verifies: []` non e' una dimenticanza, ed e' la stessa scelta di `U24`.** Le voci non esistono ancora — misurato: `grep -cE "PIE-(MAPED-GRID|HEX-MOVEMENT-PROBE)" docs/technical/test-manuali-pie.md` da' **0** — e **non vanno create adesso**: una voce che chiede di verificare una griglia di lavoro che nessun codice disegna direbbe qualcosa di falso sul repository, e il registro PIE e' una lista di cose *verificabili*, non di cose desiderate. Le crea la PR che implementa, che e' anche l'unica che sa che aspetto avranno. `#711` ha gia' scelto il proprio nome: `PIE-HEX-MOVEMENT-PROBE`. ⚠️ **Le due issue condividono l'allestimento, non lo scopo.** La griglia di lavoro (`#622`) si guarda **dove le celle non esistono**; la sonda (`#711`) si guarda dove esistono, su una mappa con superfici costose e una transizione. Stessa apertura, stesso `L_DevSandbox` illuminato da `U21`, due verdetti distinti: chi ne esegue una sola lo dichiara, invece di chiudere la seduta. ⚠️ `unblocked_by: [U21]` per la stessa ragione pratica di `U22`: su una scena non illuminata il verdetto direbbe piu' sulle luci che sulla griglia. ⚠️ ID assegnato prima del merge: `U26`, con `U25` come massimo misurato su `main` **e su tutti i branch remoti** — `U25` vive su `origin/docs/graybox-kit-consolidamento`, che al 2026-08-17 e' la PR **#1099** aperta. Preso `U26` e non `U25` proprio per questo. Chi arriva secondo rinumera, non contende.

#### U27 · Il pulsante BACK del modale d'errore, collegato al navigatore ✅

**Sbloccata da**: — · **Percorso critico**: no
**Produce**: il `BACK` di `WBP_RT_ErrorModal` che chiama `BackFromError` invece di essere disegnato e inerte
**Artefatti**: `Content/RT/UI/Framework/WBP_RT_ErrorModal.uasset` ✅
**Finita quando**: il pulsante chiama `BackFromError(GetPhaseWhenArmed())` sul navigatore — non `PopScreen` ne' `ReturnMain` diretti — e il package si salva senza errori

> ✅ **CHIUSA il 2026-08-19 — il pulsante chiama `BackFromError`, misurato sui byte del package.** `BackFromError` **2**, `GetPhaseWhenArmed` **2**, `RTFrontendNavigator` **1**; e soprattutto `PopScreen`, `ReturnMain`, `PushScreen`, `CloseModal` tutti a **zero** — il widget non naviga da se', che era l'unica cosa che questa seduta poteva sbagliare. Suite `Frontend` **30/30** dopo il salvataggio: il resave non ha rotto i tre binding di visibilita'. Lease rilasciata, quattro verification su quattro. ⚠️ **Cio' che la misura NON prova**, e vale dirlo: un grep sui byte trova i nomi, non la topologia del grafo. Che il pin `PhaseWhenArmed` prenda `GetPhaseWhenArmed()` invece di una costante regge perche' quel nome **compare** — una costante non lo nominerebbe — ma un grafo che lo chiamasse ignorandone il valore passerebbe lo stesso. La chiude il primo test che apra il grafo eventi di un `WBP_RT_*`, che oggi non esiste. ───── ➕ **Aperta il 2026-08-18 per l'ULTIMA voce del DoD di `#937`.** Il C++ e' consegnato (PR #1183): `URTFrontendNavigator::BackFromError` sceglie fra `ReturnMain` a partita viva e `CloseModal`+`PopScreen` durante il loading, provato da **3** test e da una verifica di mutazione che ne fa cadere esattamente due. Quel che manca e' un collegamento dentro il Blueprint: il pulsante `BACK` esiste nel package — misurato, la stringa «Back» c'e' — e **non chiama niente**. ⛔ **Il pulsante deve chiamare `BackFromError`, non `PopScreen` ne' `ReturnMain`.** Quei due sono sul navigatore e un Blueprint puo' chiamarli: sarebbe il widget che naviga da se', cioe' l'invariante 1 di CP 46.1 violata **dentro un `.uasset`**, dove nessun test di navigazione se ne accorge. La regola di dove si torna vive in un posto solo, e questa seduta non e' quel posto. ⚠️ **La fase si passa, non si sceglie**: l'argomento e' `GetPhaseWhenArmed()`, che il widget espone gia'. Un Blueprint che passasse una costante — `Ready` fisso, per dire — riporterebbe la decisione dentro la UI per un'altra strada. ⚠️ **Non e' esercitabile a schermo dopo questa seduta, e va saputo prima.** Nessuno chiama `InitializeFrontend`: e' di CP 46.3 (`#938`). Il collegamento si verifica **nel grafo del Blueprint**, non giocando — ed e' la ragione per cui `verifies: []`, come per `U24`. ⚠️ **Lease `BINARY-GH937-ERRORMODAL-BACK`, su UN package.** `WBP_RT_FrontendRoot` e `WBP_RT_ModalLayer` sono toccati da `origin/feat/937-widget-layout`, un branch vivo che non dichiara nessuna lease: questa seduta non li apre e non li prenota. ⚠️ ID assegnato prima del merge: `U27`, con `U26` come massimo misurato su `main` **e su tutti i diciotto branch remoti**, non solo sul proprio. Chi arriva secondo rinumera, non contende.

> **64 voci del registro non stanno in nessuna seduta** — `PIE-BU-*` 4 · `PIE-CP-*` 1 · `PIE-FMTVER-*` 1 · `PIE-HEX-*` 12 · `PIE-HEXPLAY-*` 1 · `PIE-MP-*` 1 · `PIE-MUT-*` 2 · `PIE-NAME-*` 1 · `PIE-P-*` 1 · `PIE-REPLAY-*` 1 · `PIE-SCEN-*` 2 · `PIE-STATE-*` 10 · `PIE-TEST-*` 2 · `PIE-V-*` 4 · `PIE-VIS-*` 21. Non e' per forza un difetto (le `PIE-VIS-*` hanno il proprio scenario, le `PIE-STATE-*` verificano un sistema che non esiste), ma una voce che non sta in una seduta non viene eseguita mai: e' la ragione per cui questo conteggio e' qui.

<!-- RT_SHORTLIST_EDITOR:END -->

---

## 3. Rapporto con le altre viste

| Vista | Risponde a | Owner |
|---|---|---|
| **EditorMap** *(questo file)* | *cosa faccio all'editor adesso* | [`editor-sessions.yaml`](editor-sessions.yaml) |
| Registro PIE | *cosa devo verificare, e com'è andata* | [`../technical/test-manuali-pie.md`](../technical/test-manuali-pie.md) |
| [`milestonemap.shortlist.md`](milestonemap.shortlist.md) | *a che punto è il lavoro* | `roadmap-checkpoint.md` |
| [`scenariomap.shortlist.md`](scenariomap.shortlist.md) | *chi esegue cosa* — macchina o persona | `../technical/scenario-map.md` |
| [`featuremap.shortlist.md`](featuremap.shortlist.md) | *questa cosa esiste* | `feature-registry.yaml` |
| [`roadmap.shortlist.md`](roadmap.shortlist.md) | *quando si lavora a questo* | `roadmap-v0.1.md` §2.1 |

**Procedure**: [`../technical/tooling/convenzioni-contenuti-ue.md`](../technical/tooling/convenzioni-contenuti-ue.md) (dove va
un asset, come si chiama) · [`../technical/runbooks/guida-animazioni-paragon.md`](../technical/runbooks/guida-animazioni-paragon.md)
(personaggi, AnimBP, montaggi) · [`../technical/runbooks/debug-vs-unreal.md`](../technical/runbooks/debug-vs-unreal.md)
(compilare, avviare, debuggare). I passi espliciti compaiono nelle sedute **solo** dove nessuna guida copre
ancora la cosa.

## 4. Manutenzione

Una seduta nuova nasce quando un checkpoint di codice richiede l'editor e nessuna seduta lo copre. Si aggiunge
un record a [`editor-sessions.yaml`](editor-sessions.yaml) e si rigenera.

La regola che impedisce la divergenza è una sola: **qui non si ripete mai l'esito atteso di una voce `PIE-*`**.
Si cita l'ID e basta. Se ti trovi a scrivere una colonna «esito atteso», stai scrivendo nel file sbagliato.
