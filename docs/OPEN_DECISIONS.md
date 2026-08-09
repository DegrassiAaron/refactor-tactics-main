# Decisioni aperte

> `OPEN` · **Stato**: vivo · **Ultimo aggiornamento**: 2026-08-08
> **Cosa è**: l'elenco di ciò che **aspetta una persona**. Nessuna di queste voci può essere chiusa
> deducendola dai documenti: o mancano i dati, o due fonti si contraddicono senza gerarchia.
> **Cosa non è**: il registro delle decisioni prese — quello è il
> [Decision Log](decisions/RT_PDR_00_Decision_Log.md), che resta l'**owner**. Quando una voce qui si chiude,
> diventa una `D-0xx` lì e sparisce da qui.

---

## ✅ Chiuse il 2026-08-07 — sessione `/sc:brainstorm`

Le cinque voci `OD-1`…`OD-5` aperte dalla revisione documentale sono state decise. Restano qui **solo come
indice**, per chi cerca la sigla vecchia: il contenuto vive nel Decision Log.

| Era | Decisione presa | Dove vive ora |
|---|---|---|
| `OD-1` | Formato principale **non deciso**: D-001 declassata da *Consolidata* ad *Assunzione da bloccare*. Il 3v3 resta baseline di lavoro, il 4v4 solo scenario di stress | [`D-011`](decisions/RT_PDR_00_Decision_Log.md) |
| `OD-2` | Unità ausiliarie: **concetto unico**, solo vincoli architetturali, gameplay fuori dalla v0.1 | [`gameplay/brief-unita-ausiliarie.md`](gameplay/brief-unita-ausiliarie.md) |
| `OD-3` | L'Overwatch **compete** con l'azione offensiva; le tre policy entrano nel DoD di **CP 14.3** | [`D-012`](decisions/RT_PDR_00_Decision_Log.md) · [`gameplay/brief-azioni-generiche-overwatch.md`](gameplay/brief-azioni-generiche-overwatch.md) |
| `OD-4` | Il trigger su transizione è **possesso della trap**, non della mappa: `FRTHexEdge` resta per i soli salti di layer | [`D-013`](decisions/RT_PDR_00_Decision_Log.md) |
| `OD-5` | Scenario **4v4 di stress** in roadmap dopo E15, come validazione e non come produzione | epic **E17**, [`roadmap/roadmap-v0.1.md`](roadmap/roadmap-v0.1.md) |

**Nota di metodo, che vale più delle singole risposte.** Due delle cinque domande erano **mal poste**, e lo si è
scoperto guardando il codice invece dei documenti:

- `OD-4` chiedeva «gli archi portano trigger?». Gli archi degli adiacenti **non esistono**: `GraphNeighbors`
  li calcola da `URTHexLibrary::Neighbors`, e solo le transizioni fra layer sono dati. La domanda vera —
  *dove vive la coppia `(From→To)`* — ha una risposta che non tocca la mappa.
- `OD-1` era etichettata **bloccante**. La dimensione della squadra è un `TArray<FName>` sul GameMode, non un
  campo di `FRTMatchRules`: in v0.1 non si costruisce né un 3v3 né un 4v4. Bloccava la *documentazione*, non
  il codice.

Etichettare l'urgenza dai documenti, senza verificarla sul codice, aveva prodotto l'ordine di priorità
sbagliato: l'unica decisione che bloccava lavoro costruibile era `OD-3`.

---

## Aperte — facing e azioni base, dal consolidamento del 2026-08-08

Origine: l'handoff [`archive/src/handoff/2026-08-08-azioni-base-e-facing.md`](archive/src/handoff/2026-08-08-azioni-base-e-facing.md), che chiudeva con
quindici «decisioni ancora aperte». **Cinque erano già decise**, e una sesta era una domanda diversa da come
era posta. Non compaiono fra le voci aperte, perché una domanda già chiusa che resta scritta come aperta
invita a ridecidere ciò che è stato deciso:

| Domanda dell'handoff | Perché non è aperta |
|---|---|
| §38.4 — quanto può girarsi un'unità **ferma** | [ADR-0005](decisions/adr-0005-orientamento.md) §1: stile `None` → **sei** direzioni, rotazione libera, **non consuma slot** |
| §38.7 — quali Basic Attack usano `FaceTarget` | [D-020](decisions/RT_PDR_00_Decision_Log.md): **tutte** le azioni con bersaglio o direzione orientano prima di risolvere. Non è una proprietà per azione |
| §38.10 — quali forced movement preservano il facing | [ADR-0005](decisions/adr-0005-orientamento.md) §3: verso l'origine dell'**ultimo** spostamento subito nell'ordine canonico; **ambientale senza sorgente** → invariato; un Move volontario successivo vince |
| §38.14 — un'azione che ruota torna al facing precedente? | No: la timeline di D-020 va in avanti e non ha punto di ripristino. `FacingFinalAfterMove` **persiste** nel round dopo |
| §38.15 — «Dash azione base» player-facing vs tassonomia tecnica | [D-015](decisions/RT_PDR_00_Decision_Log.md) + [D-028](decisions/RT_PDR_00_Decision_Log.md) + [`balance/RT_ActionCatalog_v0.1.md`](balance/RT_ActionCatalog_v0.1.md) §2.2: `Dash` è **mobilità speciale in fase Dash**, slot movimento. Che la Wiki lo presenti come azione base non è un conflitto: la Wiki è **dichiaratamente non normativa** (invariante #12) |
| §38.5/§38.6 — valori di rotazione per personaggio | Non sono valori mancanti: sono un **modello diverso**. Vedi `FAC-1` |

### Il modello — tre proposte che cambiano ADR-0005

Righe **50–52** di [`DOC_CONFLICT_MATRIX.md`](DOC_CONFLICT_MATRIX.md). Oggi prevale l'ADR; queste voci
esistono perché la proposta è coerente e nessun documento può accettarla al posto dell'autore.

| ID | Domanda | Cosa cambierebbe |
|---|---|---|
| `FAC-1` | La rotazione è **derivata dal movimento** (canone) o è una **capacità del personaggio** misurata in step? | L'handoff propone `MoveEndPivotMaxSteps`/`DashEndPivotMaxSteps` per eroe. Il pregio dichiarato di ADR-0005 è **zero numeri nuovi**: questo ne aggiunge due per eroe, cioè un asse di bilanciamento. In cambio dà a Bastion e Vektor un'identità di movimento che oggi non hanno. **Non decidibile dai documenti**: serve l'autore |
| `FAC-2` | Le regole universali D-020 / ADR-0005 §3 vanno sostituite da **policy dichiarative** per azione e per effetto? | Un enum per azione è più espressivo e va compilato per **ogni** azione del catalogo. Il costo si paga se esiste un caso reale che la regola universale sbaglia: **nessuno è stato prodotto** dall'handoff. Finché non esiste, la regola unica costa meno ed è già testabile |
| `FAC-3` | `Brace` deve diventare **direzionale**? | ADR-0005 §4a dice il contrario in modo esplicito: `Deflect`, `Brace`, `Shield` proteggono la **persona**, non un lato. Cambiarlo è una modifica di §4a, e il test `Combat.ShieldWorksFromAnyDirection` (CP 16.2) esiste per impedire che accada per deriva |

### Le lacune — cose che il canone non dice affatto

Queste **non** contraddicono nessuna decisione: sono buchi. Vanno decise prima che E16 le incontri in codice.

| ID | Domanda | Perché serve una risposta |
|---|---|---|
| `FAC-4` | Qual è il facing dell'unità **durante** i micro-step di un Move? | È la lacuna più urgente, ed è una conseguenza diretta di D-020. Overwatch, reazioni e cover direzionale si valutano a un boundary che cade **dentro** il movimento: se il facing intermedio non è definito, non è definito nemmeno cosa legge il trigger. ADR-0005 copre l'inizio e la fine del Move, non il mezzo. Blocca il DoD «snapshot e TurnLog dicono *quale* facing ha usato ciascun consumatore» |
| `FAC-5` | Una **reazione** può ruotare l'unità che reagisce? | D-020 nomina `FacingUsedByOverwatch` come valore **letto**, mai scritto. Se una Return Fire ruota verso l'attaccante, il facing cambia a metà round e i consumatori successivi lo ereditano: è una regola nuova, non una precisazione |
| `FAC-6` | `Interact` **richiede** un facing verso l'oggetto, oppure lo **impone**? | `Interact` è universale ([D-025](decisions/RT_PDR_00_Decision_Log.md)) e assorbe `Activate`: la risposta tocca porte, console e valvole di E9, non un solo dispositivo |
| `FAC-7` | Quali **status** limitano la rotazione, separatamente dal movimento? | Oggi nessuno: `Status.Root` interagisce con `Guard`, non col facing. Distinguere «non può muoversi» da «non può girarsi» è una scelta di design, non un dettaglio implementativo |
| `FAC-8` | Il **terreno** può limitare la rotazione (ghiaccio, condotti, scale)? | Fuori v0.1 — ma il ghiaccio esiste già in E8 e in `Visual/Environment/IceSlide`, quindi la domanda va registrata prima che qualcuno la risolva localmente in una spec di terreno |
| `FAC-9` | Il pathfinding deve diventare **orientation-aware** — stato `(CellId, Facing)` invece di `CellId`? | Fuori v0.1, e va tenuto fuori finché non c'è una misura: moltiplica per sei lo spazio degli stati di A\*, contro i budget già dichiarati in [`technical/spec-pathfinding.md`](technical/spec-pathfinding.md). Il ripiego dichiarato dall'handoff — path geometrico, facing derivato, pivot validato alla fine — **è** già il modello di ADR-0005 |
| `FAC-10` | Come si chiama la rotazione in posto? | Il canone dice «rotazione dichiarata», l'handoff dice `Pivot`, la Wiki non usa nessuno dei due. Non è pedanteria: `Reposition` è **già** una mobilità speciale a catalogo, quindi il vocabolario della rotazione va scelto una volta prima che tre documenti ne usino tre |

> **Nota di metodo.** Filtrare le quindici domande contro il canone ne ha chiuse cinque senza discussione, e
> ne ha riqualificata una sesta: «quali valori di rotazione per ogni eroe» non è un valore mancante, è la
> domanda `FAC-1` travestita da tabella da compilare. Vale la stessa lezione di `OD-1`/`OD-4`: un elenco di
> domande aperte redatto senza verificare il repository misura ciò che l'estensore non sapeva, non ciò che il
> progetto non ha deciso.
>
> Le **dieci** voci qui sopra sono il residuo dopo la verifica — sette dall'elenco §38, tre (`FAC-1`…`FAC-3`)
> promosse da proposte che l'handoff dava per acquisite. `FAC-4` è l'unica che blocca lavoro costruibile oggi:
> le altre nove possono aspettare, ma non oltre l'apertura di E16, perché a quel punto il codice risponderà
> per conto suo.

## Aperte — livello prodotto

Owner: [`product/piano-canonico-mvp.md`](product/piano-canonico-mvp.md) §9. Non bloccano l'MVP tecnico.

| Tema | Stato |
|---|---|
| Budget e modello commerciale | non specificati in nessun documento |
| Composizione del team | assunzioni discordanti; il progetto assume **dev singolo** |
| Direzione artistica | inesistente; si usano placeholder e asset Paragon |
| Hardware target | mai definito → i budget KPI restano **da misurare**, non garanzie |
| Identità originale (nomi, lore) | necessaria per una pubblicazione |
| ~~Mapping visuale Paragon → roster~~ | **chiusa il 2026-08-08** da [`D-037`](decisions/RT_PDR_00_Decision_Log.md): Flux → `Paragon.Gadget`, Riva → `Paragon.Phase`, Bastion → `Paragon.Riktor`, Vektor → `Paragon.Wraith`. Tabella owner in [`characters/paragon.md`](characters/paragon.md). Resta aperto solo il **nome retail** dei quattro slot v0.2, che è la riga «Identità originale» qui sopra |

## Aperte — bloccano un'epic gia' specificata

Owner: [`gameplay/brief-conoscenza-parziale.md`](gameplay/brief-conoscenza-parziale.md). A differenza delle
voci di prodotto qui sopra, queste **fermano lavoro pianificato**: senza risposta, il primo checkpoint di E13
non ha un criterio. Aperte dalla spec panel del 2026-08-09 ([`#294`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/294)).

| ID | Tema | Perche' non e' deducibile | Blocca |
|---|---|---|---|
| `PER-1` | **Soglia d'udito**: statistica per eroe o costante di regola? | Il dato **non esiste**: `URTHeroData` ha `MaxHealth`, `MovePoints`, `VisionRange`, `PushResistance` e nient'altro, e il catalogo `balance/` non lo aggiunge. Se la risposta e' «per eroe» servono quattro numeri **decisi**, non dedotti dal workbook, che e' `RESEARCH` ([D-023](decisions/RT_PDR_00_Decision_Log.md)) | CP 13.3, CP 13.4 |
| `PER-2` | **`Noise_Mod` di acqua bassa e vegetazione** | Due fonti si contraddicono senza gerarchia: acqua bassa **+3** (workbook) contro **+2** (documento sorgente), vegetazione **+1** contro **−2**. La seconda e' una divergenza **di segno** — «la vegetazione ti nasconde» e «ti tradisce» sono due giochi diversi sullo stesso terreno | CP 13.3 |
| `PER-3` | **Il contatto e' simmetrico?** Se A vede B, B vede A? | Era un caso di bordo quando l'asimmetria veniva dai soli range diversi (6 contro 5). Con **CP 16.1 chiuso** la vista e' a **cono di 120°**: A puo' avere B nel proprio arco mentre B guarda altrove, e l'asimmetria diventa la norma. La risposta decide se `TeamKnowledge` e' una **relazione** o un **insieme**, cioe' la struttura dati | CP 13.1 |

Minori, stessa sessione: un alleato nel **tunnel** resta noto (§10.2 del brief)? E `range azione ≤ vista` diventa
un invariante verificato del catalogo o un controllo caso per caso (§10.4)?

## Assunzioni da bloccare

Voci del Decision Log che **aspettano una misura**, non una discussione. Si consolidano da sole quando il dato
arriva; finché non arriva, nessun documento deve trattarle come vincolanti.

| ID | Assunzione | Cosa la consolida |
|---|---|---|
| [`D-001`](decisions/RT_PDR_00_Decision_Log.md) | Formato principale **3v3** | La prima misura reale su una partita ≥3v3. Oggi non esiste: la v0.1 misura la banda **2v2** |

`D-007` (baseline motore) **non è più qui**: chiusa il 2026-08-08 come
[`D-022`](decisions/RT_PDR_00_Decision_Log.md) — **UE 5.8.1**, upgrade solo fra milestone e con migrazione
esplicita. Era il caso opposto a `D-001`: non mancava la misura, mancava solo la formalizzazione di un vincolo
che il repository applicava già.

## Da playtestare — non decisioni, tarature

Numeri già in vigore che nessuno ha ancora misurato sul campo. Vivono nei documenti che li possiedono; sono
elencati qui perché **sembrano decisi e non lo sono**.

| Parametro | Valore in vigore | Owner |
|---|---|---|
| `FastReactionDuration` = **3,0 s** | baseline | [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) §8 |
| `MaxPromptsPerReaction` = **3**, nessun cap aggregato | scelta esplicita, rischio dichiarato | [`gameplay/brief-overwatch-reazioni.md`](gameplay/brief-overwatch-reazioni.md) §5 |
| Effetto esatto di `Brace`, numeri `Sneak/Move/Sprint` | non definiti | [`gameplay/brief-azioni-generiche-overwatch.md`](gameplay/brief-azioni-generiche-overwatch.md) |
| `RoundLimit` 2v2 **10–14**, 3v3 **16–20** | bande, non costanti | [D-010](decisions/RT_PDR_00_Decision_Log.md) · [`gameplay/spec-durata-partita-e-scala-mappe.md`](gameplay/spec-durata-partita-e-scala-mappe.md) |
| Durata della resolution con 1/2/3 unità armate | **mai misurata**; soglia d'allarme 20 s | CP 14.5 |
| Grammatica `STAND · READ · SHIFT` del Reaction Clash | **`PROPOSED FOR PLAYTEST`**, non canonica ([D-043](decisions/RT_PDR_00_Decision_Log.md)) | [`gameplay/spec-reaction-clash-e14.md`](gameplay/spec-reaction-clash-e14.md) §4 |
| Payoff `Win/Tie/Lose` delle maneuver dei 4 eroi | **inesistenti**: i nomi nella spec sono fixture, nessun dato d'eroe li contiene | idem §5 |
| Costo di un Clash: `Charges` proprio o quello della reaction | non deciso | idem §14 (`CLASH-3`) |

> **Il Reaction Clash incontra due domande già aperte, e non le risolve.** `FAC-3` (*`Brace` diventa
> direzionale?*) e `FAC-5` (*una reazione può ruotare chi reagisce?*) tornano rilevanti perché maneuver come
> `Pivot Step` e `Sidestep` presuppongono la risposta «sì». Restano `PROPOSED` finché quelle due non sono
> decise — vedi la §6 della spec, che lo dichiara invece di risolverle di lato.
