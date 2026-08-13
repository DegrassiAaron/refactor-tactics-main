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

**21 sedute** — ✅ **0** · 🟡 **12** · ⏳ **6** · **3** senza stato derivabile (non dichiarano ne' voci ne' artefatti: il codice sotto non esiste ancora).

Stato **derivato**, mai dichiarato: dalle voci `PIE-*` di [`../technical/test-manuali-pie.md`](../technical/test-manuali-pie.md) e da `git ls-files` sugli artefatti. Un artefatto non tracciato impedisce il verde qualunque cosa dicano le voci.

### My Editor Queue

**BLOCKING** 10 · **READY** 2 · **WAITING** 6 · **DONE** 0. **Derivata**, non dichiarata: `unblocked_by` risolto dice se si puo' cominciare, `critical` se blocca la v0.1, lo stato se e' finita. Un checkpoint 🟡 conta come risolto — gli manca la verifica che porti tu; una **seduta** prerequisito no, perche' a meta' non ha ancora prodotto il suo artefatto.

**BLOCKING** — *Blocca la v0.1, e si puo' fare adesso*

- **U1** · Mappa-arena hex — 0/7 voci verdi · sblocca U13, U19
- **U2** · Partita hex, primo giro — 3/4 voci verdi · sblocca U3, M6.1, M6.2
- **U3** · Input e pianificazione — 1/4 voci verdi · sblocca U4, M6.3
- **U4** · Combat e linea di tiro — 0/3 voci verdi · sblocca U5, M6.4, M6.5
- **U5** · Bot e HUD — 0/7 voci verdi · sblocca U6, U19, M6.6, M6.7
- **U6** · Multilivello e partita completa — 0/4 voci verdi · sblocca U16, U19, M6.8
- **U7** · Personaggi Paragon — 1/2 voci verdi · sblocca U8, U19
- **U8** · Animazioni — 0/2 voci verdi · sblocca U9, U19
- **U9** · Leggibilita' e riferimento visivo — 2/4 voci verdi · sblocca M8.1, M8.2, M8.3
- **U15** · HUD, intenti, log e comandi debug — 1/5 voci verdi · sblocca E11

**READY** — *Si puo' fare adesso, fuori percorso critico*

- **U18** · Verifiche senza prerequisiti — 1/15 voci verdi
- **U21** · Luci del graybox e inquadratura della mappa — 0/2 voci verdi

**WAITING** — *Aspetta codice*

- **U11** · I 4 eroi — attende `U10` —
- **U13** · Arena v0.1 — attende `U1` 🟡
- **U14** · Ambiente in partita — attende `U13` ⏳
- **U19** · Durata, ritmo e scala — attende `U6` 🟡, `U1` 🟡, `U5` 🟡, `U7` 🟡, `U8` ⏳
- **U16** · Misura dei KPI — attende `U6` 🟡
- **U20** · Confine fra Guard e Brace — attende `E5.2` ⏳

**DONE** — *Finite*

- —

### Tutte le sedute

| | Seduta | Produce | Sbloccata da | Critico | Voci | Stato |
|---|---|---|---|:--:|:--:|:--:|
| **U18** | Verifiche senza prerequisiti | verdetto su quindici voci che non attendono nulla | — | no | 1/15 | 🟡 |
| **U1** | Mappa-arena hex | `DA_HexMap_Arena` e `L_HexArena`, committati | M6.0 | sì | 0/7 | 🟡 |
| **U2** | Partita hex, primo giro | verdetto su allestimento e movimento | M6.1, M6.2 | sì | 3/4 | 🟡 |
| **U3** | Input e pianificazione | verdetto su selezione, budget e anteprima del percorso | M6.3 | sì | 1/4 | 🟡 |
| **U4** | Combat e linea di tiro | verdetto su forme d'attacco, LOS esagonale e knockback | M6.4, M6.5 | sì | 0/3 | ⏳ |
| **U5** | Bot e HUD | verdetto sul bot su hex e i pesi utility ritarati sulla scala esagonale | M6.6, M6.7 | sì | 0/7 | 🟡 |
| **U6** | Multilivello e partita completa | chiusura di M6 / E2 — sessione D verde | M6.8 | sì | 0/4 | 🟡 |
| **U7** | Personaggi Paragon | i quattro Blueprint-unita' del roster v0.1, committati | — | sì | 1/2 | 🟡 |
| **U8** | Animazioni | gli anim BP dei quattro personaggi e i montaggi Cast/Hit/Death | — | sì | 0/2 | ⏳ |
| **U9** | Leggibilita' e riferimento visivo | il video (o gli screenshot) di riferimento — DoD di milestone di M8 | — | sì | 2/4 | 🟡 |
| **U10** | Data asset delle azioni | il catalogo azioni della v0.1 come dati, non come codice | E1.3, E1.4 | sì | — | — |
| **U11** | I 4 eroi | i data asset di Gadget, Phase, Riktor e Wraith, e lo spawn 2v2 che li usa | E6, U10 | sì | 0/1 | 🟡 |
| **U12** | Loadout | varianti arma, gadget e moduli reazione come dati — 1 + 1 + 1 per eroe | E7, U11 | no | — | — |
| **U13** | Arena v0.1 | l'arena estesa con quanto serve alle verifiche di contenuto | E8, E9, U1 | sì | 0/1 | ⏳ |
| **U14** | Ambiente in partita | verdetto sulle regole ambientali e strutturali | U13 | sì | 0/11 | 🟡 |
| **U15** | HUD, intenti, log e comandi debug | verdetto su leggibilita' e osservabilita' | E11 | sì | 1/5 | 🟡 |
| **U19** | Durata, ritmo e scala | numeri di playtest — non difetti | U6, U1, U5, U7, U8 | sì | 0/4 | ⏳ |
| **U16** | Misura dei KPI | numeri reali nella tabella KPI | U6 | sì | 0/1 | 🟡 |
| **U17** | Release v0.1 | build Windows Development e Shipping, e una partita giocata senza editor | E12 | sì | — | — |
| **U20** | Confine fra Guard e Brace | verdetto di leggibilita' — un dato per `BAL-1`, non un difetto da correggere | E5.2 | no | 0/1 | ⏳ |
| **U21** | Luci del graybox e inquadratura della mappa | verdetto su leggibilita' della scena e inquadratura, piu' il livello illuminato committato | — | no | 0/2 | ⏳ |

### Blocco 1 — Eseguibile oggi

*Nessuna di queste attende codice. ⚠️ **Il banco di prova non si costruisce piu' a mano**: `MapSource = GeneratedTestArena` genera gia' esagono r=4, ostacoli, muro che blocca la vista, fango a costo 3, piattaforma sul layer 1 e una transizione — quindi le sedute della parita' hex non aspettano U1. Costruire una mappa serve a verificare **gli strumenti**, e a preparare il contenuto della v0.1.*

#### U18 · Verifiche senza prerequisiti 🟡

**Sbloccata da**: — · **Percorso critico**: no
**Produce**: verdetto su quindici voci che non attendono nulla
**Verifichi**: `PIE-PREVIEW-AREA` ✅ · `PIE-V01-MATCHEND` ⏳ · `PIE-TEST-CONSOLE` 🟡 · `PIE-HEX-LAYER` ⏳ · `PIE-HEX-TRANS` ⏳ · `PIE-HEX-LAYER-FOCUS` ⏳ · `PIE-HEX-LAYER-CLICK` ⏳ · `PIE-HEX-LAYER-PANEL` ⏳ · `PIE-V01-REACTCOND` ⏳ · `PIE-HEX-VIZ-BLOCCHI` ⏳ · `PIE-HEX-VIZ-COSTO` 🟡 · `PIE-HEX-VIZ-BORDI` ⏳ · `PIE-HEX-VIZ-PORTE` ⏳ · `PIE-HEX-VIZ-UNDO` ⏳ · `PIE-HEX-VIZ-TRANSIZIONI` ⏳
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
**Verifichi**: `PIE-HEX-MODE-E` ⏳ · `PIE-HEX-MODE-F` ⏳ · `PIE-HEX-MODE-G` ⏳ · `PIE-HEX-MODE-H` ⏳ · `PIE-HEX-MODE-L` ⏳ · `PIE-HEX-MODE-N` ⏳ · `PIE-HEX-MODE-O` ⏳
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
**Verifichi**: `PIE-HEXPLAY-1` ✅ · `PIE-HEXPLAY-4` ⏳ · `PIE-HEXPLAY-5` ✅ · `PIE-CAM-START` ✅
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
| Gadget | `Hero.Flux` | `Paragon.Gadget` | `BP_Unit_Gadget` | `…/ParagonGadget/Characters/Heroes/Gadget/Meshes/Gadget` |
| Phase | `Hero.Riva` | `Paragon.Phase` | `BP_Unit_Phase` | `…/ParagonPhase/Characters/Heroes/Phase/Meshes/Phase_GDC` |
| Riktor | `Hero.Bastion` | `Paragon.Riktor` | `BP_Unit_Riktor` | `…/ParagonRiktor/Characters/Heroes/Riktor/Meshes/Riktor` |
| Wraith | `Hero.Vektor` | `Paragon.Wraith` | `BP_Unit_Wraith` | `…/ParagonWraith/Characters/Heroes/Wraith/Meshes/Wraith` |

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
   `UStaticMeshComponent` ed e' il root: nel Blueprint si **aggiunge** uno
   `USkeletalMeshComponent` e si toglie la spunta `Visible` al cilindro, che il C++ usa ancora
   per selezione e fallback.
4. **`VisualZOffset = 0`.** Il default e' `UnitHalfHeight` (90), giusto per il cilindro che ha il
   pivot al CENTRO; i personaggi UE ce l'hanno ai PIEDI. Lasciarlo fa fluttuare l'unita' a 90 cm.
5. `TeamRingMaterial` e `SelectionRingMaterial` → `M_TeamRing` / `M_SelectionRing` in
   `/Game/RT/Characters/Shared/Materials/`. Il colore lo mette il codice sul MID; assenti,
   l'anello resta nascosto senza rompere nulla.
6. **Scala del componente skeletal: World/Absolute, `1,1,1`.** ⚠️ Il cilindro e' il ROOT e porta
   `BaseMeshScale = (1.2, 1.2, 1.8)`: un componente attaccato a lui eredita quel fattore e il
   personaggio esce **stirato in altezza di 1,5x** (1.8 / 1.2). Peggio, la selezione rimoltiplica
   il root per `1.15` (`RTUnit.cpp:221`), quindi la mesh **si ingrandisce quando la selezioni**.
   Nel Details del componente, alla riga *Scale*, si commuta l'icona su **World/Absolute**.
   Difetto strutturale registrato in **issue #593**: finche' resta, il passo va rifatto a ogni
   nuovo `BP_Unit`.
7. Registra ciascuno in **`HeroUnitClasses`** del `RTGameMode` — `TMap` con chiave l'`HeroId`
   (`Hero.Flux` → `BP_Unit_Gadget`, …). ⚠️ **E' il passo che sbaglia in silenzio**:
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
   | contiene | `Hero.Flux` | `Hero.Flux` → `BP_Unit_Gadget` |

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

> **I quattro pack sono tutti sul disco** (verificato 2026-08-11): Gadget 1232 file, Phase 1155, Riktor 1261, Wraith 1322. `ParagonGadget` mancava ed e' stato portato con la procedura `convenzioni-contenuti-ue.md` **B.2a** (magazzino + rename headless + copia). I pack non sono nel repo (`/Content/FabAsset/` e' ignorato): chi clona se li scarica. ⚠️ **Naming deciso dall'autore il 2026-08-11**: cartelle e asset di questa seduta portano il nome del **pack Paragon**, non dell'eroe, «per non creare problemi» — in editor si vede `Gadget` e si cerca `Gadget`. Ribalta `convenzioni-contenuti-ue.md` §A, che raccomandava l'opposto perche' il nome del pack lega l'asset a una mesh sostituibile: il rischio resta, ed e' accettato consapevolmente. Se un eroe cambiasse base visuale, il Blueprint andrebbe rinominato. ⚠️ **Eccezione dichiarata**: i data asset eroe (`DA_Hero_Flux`, …) restano intitolati all'**eroe** pur stando nella cartella del pack. Sono dati di gioco, non presentazione: non dipendono dalla mesh, e `HeroId` in C++ resta `Hero.Flux`. ⚠️ Se una mesh appare **senza materiali** non e' un errore di questa seduta: sono i soft reference di A.6 — 9 asset su 1229 in Gadget, 6 su 1698 in Gideon che e' in uso da giorni. ⚠️ **E se un personaggio appare in T-pose, schiacciato o con catene lunghissime, NON e' un difetto**: senza anim BP la skeletal resta nella **posa di riferimento** dello scheletro, dove le ossa di catene e tentacoli stanno distese in fila. Riscontrato su `Riktor` il 2026-08-12. Il controllo che lo isola in dieci secondi: **apri la Skeletal Mesh nel Content Browser** — se appare cosi' anche li', fuori dal gioco e fuori dal Blueprint, e' la bind pose e la sistema **U8**. Se invece li' e' normale e in partita no, allora guarda il Blueprint.

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
senza difesa — e una Phase avversaria che usa `Riva.PressureJet` su ciascuna nello stesso turno.
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

#### U21 · Luci del graybox e inquadratura della mappa ⏳

**Sbloccata da**: — · **Percorso critico**: no
**Produce**: verdetto su leggibilita' della scena e inquadratura, piu' il livello illuminato committato
**Verifichi**: `PIE-MAPED-LIGHT` ⏳ · `PIE-MAPED-FRAME` ⏳
**Finita quando**: le due voci hanno un esito reale e il livello illuminato e' committato

> Nasce dal referto `plans/map-sketch-editor-spec-panel-2026-08-12.md` (`P6`). E' una seduta e non una issue di codice per una ragione strutturale: `L_DevSandbox.umap` e' un `.umap`, e questo repository non modifica `.umap` da riga di comando. ⚠️ `artifacts` e' VUOTO di proposito, benche' la seduta committi un livello. L'oracolo degli artefatti e' `git ls-files`, che sa dire se un path esiste e non se e' stato MODIFICATO: `L_DevSandbox.umap` e' gia' tracciato da mesi, quindi dichiararlo qui farebbe derivare 🟡 («parte fatta») su una seduta non ancora aperta. Lo stato deriva dalle due voci PIE, che sono la cosa che davvero non esiste ancora. Il livello committato resta nella DoD della issue. ⚠️ Il sorgente chiedeva anche di ricostruire la navigazione della camera (MMB pan, RMB orbit, wheel zoom, WASD, F focus). **Il viewport di Unreal le fornisce gia' tutte**, e un `UEdMode` non possiede la camera del viewport: `RTCameraPawn` e' la camera di GIOCO, un oggetto diverso. Resta solo l'inquadratura della mappa, che e' `PIE-MAPED-FRAME`. ⚠️ ID assegnato al merge: preso `U21` con `U20` come ultimo su `main`. Chi arriva secondo rinumera, non contende.

> **58 voci del registro non stanno in nessuna seduta** — `PIE-BU-*` 4 · `PIE-CP-*` 1 · `PIE-HEX-*` 9 · `PIE-MP-*` 1 · `PIE-MUT-*` 2 · `PIE-NAME-*` 1 · `PIE-P-*` 1 · `PIE-REPLAY-*` 1 · `PIE-SCEN-*` 2 · `PIE-STATE-*` 10 · `PIE-TEST-*` 2 · `PIE-V-*` 3 · `PIE-VIS-*` 21. Non e' per forza un difetto (le `PIE-VIS-*` hanno il proprio scenario, le `PIE-STATE-*` verificano un sistema che non esiste), ma una voce che non sta in una seduta non viene eseguita mai: e' la ragione per cui questo conteggio e' qui.

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

**Procedure**: [`../technical/convenzioni-contenuti-ue.md`](../technical/convenzioni-contenuti-ue.md) (dove va
un asset, come si chiama) · [`../technical/guida-animazioni-paragon.md`](../technical/guida-animazioni-paragon.md)
(personaggi, AnimBP, montaggi) · [`../technical/debug-vs-unreal.md`](../technical/debug-vs-unreal.md)
(compilare, avviare, debuggare). I passi espliciti compaiono nelle sedute **solo** dove nessuna guida copre
ancora la cosa.

## 4. Manutenzione

Una seduta nuova nasce quando un checkpoint di codice richiede l'editor e nessuna seduta lo copre. Si aggiunge
un record a [`editor-sessions.yaml`](editor-sessions.yaml) e si rigenera.

La regola che impedisce la divergenza è una sola: **qui non si ripete mai l'esito atteso di una voce `PIE-*`**.
Si cita l'ID e basta. Se ti trovi a scrivere una colonna «esito atteso», stai scrivendo nel file sbagliato.
