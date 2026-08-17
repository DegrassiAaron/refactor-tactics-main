# Brief — Overwatch e modello unificato delle reazioni

> **Stato**: brief di requisiti · **Data**: 2026-08-07 · **Origine**: `/sc:brainstorm` su
> `docs/archive/src/design/overwatch-e-fast-reaction.md` (19 sezioni)
> **Esito**: le finestre di reazione **rientrano in scope**, con la via di riconciliazione già proposta da
> [`spec-sequenza-turno.md`](spec-sequenza-turno.md) §3; tutte le reazioni passano a **un solo modello**
> a *opportunity → commit*, di cui quello attuale diventa il **caso degenere**.
> **Autorità**: richiede **ADR-0004** prima dell'implementazione (vedi §3).

> ⚠️ **`D16`…`D22` in §3 sono ID LOCALI di questo brief**, non del
> [Decision Log](../decisions/RT_PDR_00_Decision_Log.md). Le sigle **collidono**: il `D19` locale è
> «`Timeout → HOLD`», il `D-019` globale è la distinzione fra Fast Action e Fast Reaction. Il trattino
> distingue: `D-0xx` globale e vincolante, `Dxx` locale al brief.

## 0. Decisioni arrivate dopo questo brief — 2026-08-07/08

Il brief resta valido su finestra, timeout, trigger e privacy. Quattro decisioni successive lo **estendono**;
sono qui in testa perché cambiano cosa va costruito, non solo come si racconta.

| | Cosa aggiunge |
|---|---|
| [**D-012**](../decisions/RT_PDR_00_Decision_Log.md) | L'Overwatch **compete** con l'azione offensiva: `Attack` \| `Ability` \| `Overwatch`, mai sommati. Senza costo-opportunità è la scelta di chi è indeciso |
| [**D-014**](../decisions/RT_PDR_00_Decision_Log.md) | L'Overwatch è una **azione generica universale**, disponibile a tutti; l'effetto viene dal **profilo dell'eroe** (area, arco, trigger ammessi, risposte legali), non da un ramo nel resolver |
| [**D-019**](../decisions/RT_PDR_00_Decision_Log.md) | `Fast Reaction` (evento **esterno**) e `Fast Action` (continuazione di una **propria** azione) sono categorie **distinte** sulla stessa `DecisionWindow`. Questo brief descrive la prima |
| [**ADR-0005**](../decisions/adr-0005-orientamento.md) | Il cono sorvegliato **nasce dal `Facing`**, non da una `Direction` dichiarata a parte. Due sorgenti sarebbero due verità |

I tre regimi *Automatic · Conditional · FastSelect* **non sono un enum**: emergono da `AllowedResponses` più
una condizione dichiarata in Planning — dettaglio in
[`brief-azioni-generiche-overwatch.md`](brief-azioni-generiche-overwatch.md) §5. È la mitigazione del rischio
di §5 di questo brief, ed è per questo che sta nel DoD di **CP 14.3** e non a valle.

## 1. Il conflitto era già registrato — e già riconciliato

Non è un tema nuovo. `spec-sequenza-turno.md` lo classifica come conflitto **C1** e ne propone due vie:

| Riga | Contenuto |
|---|---|
| `:32` | «Finestre di reazione live (popup 3s, input a metà esecuzione) → ❌ conflitto con l'invariante #3 → **north-star, gated**» |
| `:56` | via **(a)** politiche pre-committed · via **(b)** «ogni finestra apre un **nuovo round di sotto-risoluzione deterministico**» |
| `:137` | «Timeout finestra: default = **funzione pura dello stato**» |

La decisione **D7** aveva scelto la via (a) senza che il documento Overwatch fosse noto. Con esso disponibile,
la via **(b)** diventa preferibile — e non è una deroga all'invariante #3, ma la sua **composizione**:

```
Turno = [ snapshot → risolvi → boundary ] · [ snapshot → risolvi → boundary ] · … · Cleanup
        └──────── «raccogli poi applica» ────┘   ripetuto, non violato
```

Ogni segmento fra due *decision boundary* è un «raccogli poi applica» completo. Il documento sorgente dice la
stessa cosa in §12: «la simulazione autorevole si ferma su un decision boundary; il gameplay logico **NON
avanza**; la slow-motion è soltanto presentazione». Il determinismo regge perché la scelta del giocatore entra
nel TurnLog **come dato**, e il timeout (`→ HOLD`) è una funzione pura costante.

## 2. Il modello unificato, e come **non** rompe E5

E5 è chiusa con **24 test verdi**. L'unificazione li conserva se il modello dichiara le risposte legali:

```
Reaction armata
   └─ trigger valutato (funzione PURA, come oggi)
        └─ FRTReactionOpportunity { AllowedResponses[] }
             ├─ AllowedResponses ≤ 1  →  commit immediato, NESSUNA finestra   ← caso degenere = E5 di oggi
             └─ AllowedResponses ≥ 2  →  decision boundary + finestra          ← Overwatch
```

`Counter`, `Deflect`, `Brace`, `Shield`, `Cleanse` hanno **una sola risposta legale**: scattano o non scattano.
Restano deterministiche, senza finestre, e i loro test continuano a valere. `Overwatch` ne ha almeno due
(`FIRE` / `HOLD`) e apre la finestra.

**`Reactions.NoResolverWait` continua a passare**: quel test verifica che `EvaluateReactionTrigger` sia una
funzione pura chiamabile senza `UWorld`. Nel modello unificato **il trigger resta puro** — è il *commit* che
può richiedere input, ed è un passo distinto.

## 3. Decisioni

| # | Decisione | Motivo |
|---|---|---|
| **D16** | Le finestre di reazione **rientrano in scope**: `Reactions.FastWindow` con `FIRE`/`HOLD`, 3 s, `Timeout → HOLD` | Il bait/bluff (§18) non è recuperabile con condizioni dichiarate: se dichiaro «spara al primo che entra», il tank brucia sempre l'overwatch. Il valore sta nel *non sapere se arriverà un bersaglio migliore* |
| **D17** | **Un solo modello** per tutte le reazioni: `opportunity → commit`. Il comportamento attuale è il caso `AllowedResponses ≤ 1` | Due modelli coesistenti sarebbero due cose da mantenere e due posti dove scrivere una regola |
| **D18** | L'invariante #3 si **compone**, non si deroga: il turno è una sequenza di sotto-risoluzioni, ciascuna «raccogli poi applica» | Riconciliazione (b) di `spec-sequenza-turno.md` §3. Richiede comunque **ADR-0004**, perché cambia la forma del turno |
| **D19** | `Timeout → HOLD`, mai `FIRE` | `FIRE` consuma una risorsa irreversibile: un timeout non deve spenderla (§3 sorgente). È anche la «funzione pura dello stato» richiesta dalla spec |
| **D20** | **Nessun cap** sul numero di finestre per turno nel vertical slice: si misura al playtest | Scelta esplicita, con il rischio dichiarato in §5 |
| **D21** | Trigger simultanei nello stesso micro-step → **una sola** opportunity con più bersagli, mai prompt sequenziali | §9 sorgente: prompt in sequenza darebbero vantaggio all'ordine di iterazione — esattamente ciò che l'invariante #4 vieta |
| **D22** | Il trigger richiede `TargetDetected`, non solo LOS | §14 sorgente. **Overwatch è il primo consumatore reale di E13**: senza detection sparerebbe a unità che la squadra non percepisce |

## 4. Perimetro MVP

**Dentro** (§5 sorgente, invariato): scelta in planning · cono direzionale · range fisso · richiede LOS **e**
detection · trigger `EnemyEnterArea` valutato **a ogni micro-step** · 1 charge · più opportunity fino a
consumo o scadenza · finestra 3 s · `FIRE` consuma, `HOLD` mantiene armata · nessun interrupt annidato ·
cancellabile da KO/Stun · ordine deterministico (`ReactionPriority → AbilityPriority → UnitInitiative →
StableUnitId → ReactionInstanceId`).

**Fuori**: Suppressive Overwatch · Patient Hunter · Ambush · reaction stack LIFO interattivo · interrupt
annidati · reazioni ambientali · Hack/Counterspell · probabilità di qualunque tipo.

**Aggancio esistente**: `Action.SuppressiveLine` (fase Prep, priorità 30, 16 danni, CD 2) e
`Hero.Wraith.InterceptShot` (16 danni + stop del movimento) sono **già a catalogo e testati**. Il commento in
`RTHeroWraithTests.cpp:103` li dichiara esplicitamente «l'aggancio per E5»: l'Overwatch **si innesta lì**,
non si reinventa.

## 5. Il rischio che il documento sorgente non affronta

**Il budget di tempo aggregato.** `MaxPromptsPerReaction = 3` × 3 secondi = **9 secondi per una sola unità
armata**, contro i `Resolution_sec = 12` del workbook di bilanciamento. Con due o tre unità in overwatch la
resolution triplica, in modo **non prevedibile dal giocatore che guarda**.

Per **D20** si è scelto di non mettere un cap e di misurare. La misura va fatta esplicitamente:

> **Da registrare al playtest**: durata reale della resolution con 1, 2 e 3 unità armate; quante opportunity
> si generano per turno; quante finiscono in timeout. Se la resolution supera stabilmente i 20 secondi, il cap
> torna sul tavolo — le opzioni già valutate sono *cap per turno condiviso fra le unità* e
> *`MaxPromptsPerReaction = 1`*.

Altri rischi: **netcode a N round-trip per turno** (rilevante da M10, non prima) · un giocatore in finestra
mentre l'altro guarda, in turni **simultanei** — chi non ha reazioni armate attende senza agire.

## 6. Checkpoint proposti — epic **E14 · Overwatch e reazioni interattive** (P2)

Dipende da **E13** (detection, D22) e da **E5** (chiusa, di cui riusa i trigger). Fonte: documento §5–§17.

> ⚠️ **La tabella dei checkpoint viveva qui ed è stata rimossa il 2026-08-07.** Era duplicata in
> [`../roadmap/roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) §E14, e le due copie erano **divergenti**: qui
> c'erano 5 checkpoint, lì 6 — la rinumerazione del 2026-08-07, che ha inserito **CP 14.2** (micro-step
> step-able) e fatto scorrere tutti i successivi, non era mai tornata su questo file. Le issue GitHub
> `#161`–`#166` seguono la roadmap.
>
> **Owner dei checkpoint: la roadmap.** Questo brief possiede le *decisioni* (§3, D16–D22), non il piano.

Epic **E14 · Overwatch e reazioni interattive** (P2), 6 checkpoint: →
[`../roadmap/roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) §E14.

Dipende da **E13** (detection, D22) e da **E5** (chiusa, di cui riusa i trigger). Fonte: documento §5–§17.
Due innesti decisi dopo questo brief, entrambi in roadmap:
[D-012](../decisions/RT_PDR_00_Decision_Log.md) porta in **CP 14.3** la condizione dichiarata e in **CP 14.4**
il costo-opportunità e i profili per eroe — dettaglio in
[`brief-azioni-generiche-overwatch.md`](brief-azioni-generiche-overwatch.md).

## 7. Privacy — il requisito più delicato

§7 del sorgente è **critico** e coincide con l'invariante #6. La opportunity inviata al client deve contenere
**solo** il presente: bersagli legittimamente noti, stato pubblico corrente, risposte legali, durata.

**Mai**: «ci saranno altri due trigger» · percorsi o destinazioni future · opportunity future · intenti privati
avversari. Il protocollo deve comportarsi come se il giocatore osservasse la resolution in tempo reale — che è
esattamente ciò che rende possibile il bluff di §18: se il client sapesse che stanno arrivando altri due
bersagli, il mindgame sparirebbe.

Il test `Overwatch.OpportunityLeaksNoFuture` (CP 14.4) è il guardiano di questa proprietà, ed è **l'estensione
naturale** di `Reactions.IntentNotVisibleToEnemy` già esistente.

## 8. Domande aperte — ✅ tutte chiuse il 2026-08-07

Risolte in [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md). Tre discendono dagli invarianti, una è una scelta.

| # | Domanda | Risposta | Natura |
|---|---|---|---|
| 1 | Chi vede la finestra in 2v2? | **Decide** il proprietario, **vede** la squadra in sola lettura, l'avversario non riceve nulla | **Scelta** — in 3 s la coordinazione non è realistica: la visione dell'alleato serve alla leggibilità. Degradabile a «solo il proprietario» senza toccare il modello (ADR §7) |
| 2 | La resolution si ferma per tutti o solo per chi decide? | **Per tutti** | **Derivata**: se il resto proseguisse, l'avanzamento dipenderebbe dal tempo di risposta umano — il *quando* tornerebbe a influenzare l'esito (invariante #4). ADR §5 |
| 3 | `MaxPromptsPerReaction` 3 o 1? | **3**, data-driven; nessun cap **aggregato** per turno | **Derivata** da D20 + §5 sorgente: D20 riguarda il budget aggregato, `MaxPrompts` la singola reaction — non sono in contraddizione. ADR §8 |
| 4 | Overwatch e fumo: il trigger scatta? | Solo con livello **`Rilevato`**. Nel fumo entro 2 celle sì, oltre no; su rumore o ultimo contatto **mai** | **Derivata** da §14 sorgente + i tre livelli di E13. Sparare su un contatto incerto è `Resonance Shot`, north-star. ADR §6 |

Conseguenza operativa registrata: **E14 non parte prima di E13** — senza livelli di conoscenza,
`TargetDetected` non ha una definizione.
