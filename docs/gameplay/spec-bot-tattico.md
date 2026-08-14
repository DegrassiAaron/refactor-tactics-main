# Spec — Il bot tattico: piano di squadra, belief e reazioni

> **Owner documentale** di `RT-FEAT-BOT-FAIRNESS`, `RT-FEAT-BOT-TACTICAL`, `RT-FEAT-BOT-BELIEF`,
> `RT-FEAT-BOT-PREDICTIVE`.
> Sorgente: [`../archive/src/handoff/2026-08-11-bot-ai-team-planner-belief-e-tracking.md`](../archive/src/handoff/2026-08-11-bot-ai-team-planner-belief-e-tracking.md),
> filtrato dal [referto del 2026-08-11](../archive/roadmap-plans/bot-ai-consolidamento-2026-08-11.md).

> ## ⚠️ Questo documento non descrive codice che esiste
>
> Il bot **di oggi** è `URTHexBotLibrary`, ed è descritto da [`spec-bot-hex.md`](spec-bot-hex.md), che resta
> l'owner di `RT-FEAT-BOT-BASE`. Quel documento misura anche cosa il bot **non** sa: alla data del
> 2026-08-10, `ARTTurnManager::PlanBots` popolava `Ctx.Enemies` da tutte le unità nemiche vive, senza filtro
> di percezione.
>
> Qui si fissano i **confini** del bot che verrà, perché siano stabili prima che qualcuno scriva la prima
> riga. Nessun gate `runtime` di nessuna feature si muove per effetto di questo file: la specifica non è
> l'implementazione, ed è il §60 del sorgente a dirlo per primo.

---

## 1. La pipeline, e perché non è negoziabile

Il bot è un **produttore di Intent**. Non tocca il mondo.

```text
FRTTeamKnowledge          ← cosa la SQUADRA sa (D-043)
      ↓
Bot Planner
      ↓
Intent / comando normale  ← lo stesso che produce un umano
      ↓
Planning → Validation → Commit
      ↓
Snapshot → Resolver → TurnLog
```

Vietato, e ciascun divieto ha un motivo che non è estetico:

| Divieto | Perché |
|---|---|
| `SetActorLocation` / `ApplyDamage` dal bot | Bypassa il resolver, cioè l'unica autorità. È già la regola dei test (`CLAUDE.md` §4) e vale a maggior ragione per un giocatore artificiale |
| Un secondo simulatore nel planner | Due implementazioni della stessa regola divergono, e la seconda non ha né TurnLog né hash |
| Leggere lo stato autorevole nemico | §2 |
| Ordine di `TMap`/`TSet` come tie-break | Non è un ordine, è un dettaglio di implementazione della hash table |

Un bot e un umano devono giocare **lo stesso gioco**: è la condizione perché un playtest contro il bot dica
qualcosa del gioco.

---

## 2. Fair Knowledge — l'ingresso è la Team Knowledge, non lo stato

Il planner gira server-side e questo **non** è un permesso.

```text
STATO AUTOREVOLE
      ↓
  PERCEZIONE
      ↓
FRTTeamKnowledge
   ↙        ↘
Umano       Bot
```

Ciò che il bot può leggere è esattamente ciò che `FRTTeamKnowledge` trasporta: `VisibleCells`, i `Contacts`
con la loro cella **di contatto** — non la posizione attuale — e il turno in cui sono stati raccolti. Più
ciò che è pubblico per definizione: geometria della mappa, catalogo azioni, catalogo eroi.

Ciò che **non** può leggere, e che il tipo non trasporta: posizione reale di un'unità nascosta, intento
avversario, quale reazione è armata, quali `ReactionOpportunity` scatteranno, il `BotProfile` di un bot
avversario, e se un rumore è un'esca.

> **La difficoltà dà più ragionamento, mai più informazione.** L'invariante è già scritta in
> [`../roadmap/roadmap-post-v0.1.md`](../roadmap/roadmap-post-v0.1.md) § E26 con la ragione per cui è
> pubblicata: *«Il bot non vede più di te»* è una sezione della Wiki, non una nota interna, e un livello
> «Esperto» che bara la trasforma in una bugia pubblicata.

**Come si dimostra**, ed è la parte che conta: un test *canary* che mette la stessa `FRTTeamKnowledge`
davanti a due stati autorevoli diversi — il nemico nascosto in A oppure in B — e pretende **lo stesso
Intent**. Un bot che legge lo stato lo fallisce; nessuna review lo prende. È il gate di
`RT-FEAT-BOT-FAIRNESS`, e la sua premessa oggi è falsa per costruzione: il filtro di percezione lo installa
`E13.5` ([#160](https://github.com/DegrassiAaron/refactor-tactics-main/issues/160)).

---

## 3. Determinismo — senza seed, con budget di conteggio

**Stesso `FRTTeamKnowledge` + stesso profilo + stesso contenuto ⇒ stesso Intent.** Il seed non compare, e non
è una dimenticanza: nel progetto **non esiste un RNG** — il `Seed` degli scenari è dichiarato e mai consumato
— e il determinismo viene da coordinate intere e ordinamenti totali (`D-077`). Un requisito che ammettesse un
seed permetterebbe un `FMath::Rand()` addomesticato invece di vietarlo. Vedi `D-096`.

I budget di ricerca sono **conteggi**, mai millisecondi:

```text
MaxGoals · MaxSkeletons · MaxCandidateCells · MaxCandidates · TopK · MaxTeamPlans · MaxEnemyScenarios
```

Un budget in millisecondi renderebbe l'esito dipendente dalla macchina, cioè dal frame rate — lo stesso
motivo per cui il sequencing competitivo non passa da `Tick` né da `DeltaTime`.

Il tie-break finale è l'ordine totale che il bot di oggi già usa: mossa minima da `Origin`, poi `StableLess`
su `(Layer, X, Y)`. Permutare le candidate non cambia l'esito, ed è testato
(`RefactorTactics.HexBot.ChooseBestPlanOrderIndependent`).

---

## 4. Candidate — da un goal, non da un prodotto cartesiano

Enumerare `percorsi × celle × abilità × bersagli × facing × reazioni` esplode e non serve.

```text
Tactical Goal → Action Skeleton → celle/bersagli interessanti → validazione legale → Candidate
```

I goal sono pochi e non appartengono a un eroe: `SecureObjective · KillEnemy · ProtectAlly · CreateCombo ·
BreakEnemyFormation · GainPosition · DenyArea · Recover · GatherInformation · Disengage`. È il profilo a
pesarli, non un `if (Hero == …)` — [ADR-0006](../decisions/adr-0006-ownership-abilita-sinergie.md).

La validità **non si ricalcola**: le celle vengono da `URTHexSimLibrary::ReachableCells`, che ha già applicato
budget, blocchi, occupanti e archi verticali. È la proprietà per cui il bot di oggi non può proporre una
mossa illegale, e va conservata.

**Diversità prima di Top-N.** Prendere le N candidate migliori in assoluto cancella le opzioni di categoria
diversa: se l'offesa domina, il piano di controllo non arriva mai allo scoring di squadra e nessuno se ne
accorge, perché il risultato *sembra* ragionevole. Si tiene un Top-K **per bucket**: `Objective · Offense ·
Cover · Control · Escape · Setup · Information · CurrentCell`.

---

## 5. Utility — interi, e con il conto in chiaro

Una candidata illegale è scartata **prima** dello scoring, non penalizzata: una penalità è un numero che
qualcuno può battere.

Categorie: `Objective · Offense · Survival · Position · Control · TeamSynergy · Information · Risk ·
FriendlyFire · ResourceCost · Uncertainty`.

Punteggi **interi**, e ogni candidata produce un breakdown, non un totale:

```text
Objective       +120
Damage          +290
KillPotential   +210
TeamSynergy     +260
Exposure        -120
Hazard           -40
Uncertainty      -50
--------------------
TOTAL            670
```

Un `Score = 670` senza righe è indebuggabile: quando il bot sbaglia non si sa **quale** termine ha vinto, e
si finisce a ritoccare i pesi a caso. Il bot di oggi lo dimostra al contrario — `#149` misura come nessuna
costante di premio al posizionamento riesca a stare fra «battere due minacce» e «non battere un attacco
vero», ed è una diagnosi possibile solo perché i termini sono separabili.

I valori sono tuning, non regola: vivono nel profilo.

---

## 6. Team Planner — Top-K per unità, combinazione al centro

```text
Unità A → Top-K        Unità B → Top-K        Unità C → Top-K
                    ↘        ↓        ↙
                    RTBotTeamPlanner
                sinergie · conflitti · risorse
                          ↓
                   miglior Team Plan
```

Ogni unità mantiene le proprie preferenze; il planner sceglie la **combinazione**. Con 3 unità e 6 candidate
ciascuna sono 216 combinazioni: la forza bruta deterministica basta, e la beam search si apre solo quando un
numero misurato dice che non basta più — con budget fisso. Vedi `D-097`.

**Ruoli dinamici, non assegnati.** Il ruolo di un eroe non è il suo ruolo del turno: `Initiator · Setup ·
Payoff · Finisher · Protector · Anchor · Controller · Denier · Flanker · Scout · ObjectiveRunner ·
Disengage` **emergono** dal piano scelto. Assegnarli prima della valutazione è la scorciatoia che rende il
planner un distributore di compiti: sembra funzionare e non trova mai il piano in cui due unità si scambiano
il mestiere.

**Le capability sono semantiche**, non nomi d'eroe: un'unità «può fare `Setup` su superficie d'acqua» perché
la sua azione lo dichiara. È lo stesso vincolo di `D-029` — niente `if (HeroA && HeroB)` — applicato al bot,
dove costerebbe meno e romperebbe di più.

**Conflitti**, e la distinzione decide se un piano muore o paga:

| Hard — si scarta il Team Plan | Soft — si penalizza |
|---|---|
| destinazione incompatibile · risorsa condivisa insufficiente · interazione mutualmente esclusiva · intenti illegali insieme | percorsi che si incrociano · affollamento · stessa dipendenza da copertura · rischio di fuoco amico · ridondanza · esposizione condivisa |

Il fuoco amico ha già un modello nel bot di oggi — penalità **proporzionale al danno**, non veto — e va
riusato invece di reinventato ([`spec-bot-hex.md`](spec-bot-hex.md) §3b).

**Risorsa contesa: utilità marginale, non `UnitId`.** `MarginalUtility = ScoreConRisorsa − ScoreSenzaRisorsa`;
vince chi alza di più il piano di squadra, e `StableUnitId` è solo il tie-break finale. Assegnare per indice
significa che l'esito dipende da chi è stato costruito prima.

---

## 7. Compatibilità temporale — la combo che il resolver non permette

Una combo intra-turno vale **solo se l'ordine delle fasi lascia che il setup influenzi il payoff**. Le
macro-fasi sono `Planning → Prep → Dash → Blast → Move → Cleanup`, e un setup che risolve *dopo* il suo
payoff produce una sinergia che non accade.

Il planner **non riscrive** quell'ordine. Lo **chiede** — concettualmente
`CanSetupAffectPayoff(Producer, Consumer, Ruleset)` — e la stessa risposta deve alimentare la preview UI, il
validator e la spiegazione. Due copie dell'ordine delle fasi divergono al primo cambio, e il bot sarebbe la
copia che nessuno rilegge. Vedi `D-098`.

Il gate è uno scenario che pianifica una sinergia **temporalmente impossibile** e pretende che non riceva
bonus.

> **Aperto**: umano + bot nella stessa squadra. Il draft umano è un vincolo di preview, il commit è un
> vincolo fisso, e il bot ottimizza *intorno* — mai sopra. La forma della risposta è vincolata da `D-097`,
> ma il caso non esiste in v0.1, che è **2v2 offline contro il bot**.

---

## 8. Belief — e la riga che non si attraversa

**Una belief non diventa conoscenza perché è lo scenario più plausibile.** È la regola, e `D-099` la fissa.

Il repository ha già il vocabolario della certezza, e non se ne aggiunge un altro:

| Esiste | Dice |
|---|---|
| `ERTAwareness` — `Hidden · Uncertain · Detected` | quanto la squadra sa di una cella |
| `ERTTargetKnowledge` — `Allowed · CellOnly · Rejected` | cosa il targeting può farne |
| `FRTLastKnownContact.TurnNumber` | quanto è vecchio il ricordo |

La confidenza del bot è un **ordinamento dentro `Uncertain`**: serve a scegliere quale cella plausibile
guardare per prima, non a produrre un quarto giudizio su *sappiamo dov'è?*. Non è una percentuale: una
percentuale non calibrata è falsa precisione, e verrebbe letta come statistica.

Le celle plausibili si generano dal grafo reale a partire da `LastKnownCell`, applicando muri, porte note,
layer, costo del terreno e capacità di movimento nota — poi si **raggruppano** per destinazione tattica.
Tenere tutte le celle non è più preciso: è più costoso e altrettanto incerto.

L'incertezza **cresce** con i turni trascorsi, la mobilità del bersaglio, il diramarsi della mappa e i cambi
di `GraphRevision` — che invalidano la raggiungibilità su cui la belief era costruita, ed è il caso in cui una
belief resta plausibile per una mappa che non c'è più.

**Threat projection** = minacce visibili + posizioni plausibili + inviluppi d'attacco possibili + hazard noti.
Un nemico non localizzato diventa una `UnaccountedThreat`: un peso, **non** una posizione inventata.

**Information gain** vale senza sapere cosa si troverà — area incerta ridotta, rotte d'obiettivo osservate,
choke coperti. Valutare un reveal leggendo ciò che nasconde è la forma più silenziosa di imbroglio: il
punteggio sarebbe giusto e il metodo falso.

---

## 9. Reazioni — `FIRE`/`HOLD` senza futuro

Il bot riceve la stessa `FRTReactionOpportunity` sanificata del giocatore autorizzato, e vale il modello
`Opportunity → Commit` di [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md): timeout = `HOLD`.

Valuta valore strategico del bersaglio, danno/uccisione attesa, minaccia all'obiettivo, protezione di un
alleato, costo della risorsa, minacce non contabilizzate, costo opportunità. `ReactionPatience` sposta la
**soglia**, mai la conoscenza.

**Il bait è deducibile, la preveggenza no**, e la differenza si testa. Il bot può tenere il fuoco perché un
tank vale poco *e* uno striker nemico non è localizzato: ragiona su un'incertezza che ha. Non può tenerlo
perché «lo striker passerà dopo»: quello è un trigger futuro, e non è nel dato.

Bersagli simultanei nello stesso micro-step: utility, poi tie-break stabile.

---

## 10. Predictive — scenari pochi e coerenti

Niente minimax esaustivo. Pochi scenari nemici coerenti — `Objective Push · Aggressive Focus · Defensive
Hold · Flank · Disengage` — costruiti su ruoli e capability **noti** e posizioni plausibili, mai su intenti
nascosti. Non si combinano ingenuamente tutte le ipotesi di tutte le unità.

`RobustScore = punteggio pesato sugli scenari − penalità sul caso peggiore`. Personalità e difficoltà
cambiano la **tolleranza** al caso peggiore, non le informazioni. Serve a preferire piani meno fragili senza
conoscere il futuro.

La simulazione counterfactual — copia dello stato logico, resolver riusabile, valutazione — è `E28` e non
apre prima che il resolver sia estraibile: `D-078` ha già separato *chi riproduce* da *chi verifica*, ed è
lo stesso confine.

---

## 11. Trace e debug

Il `BotDecisionTrace` è **server-side e dev-only**, separato dal TurnLog: il TurnLog registra cosa è successo,
non il ragionamento privato di chi l'ha deciso. Contenuto utile: turno, revisione di conoscenza, profilo e
versione, goal considerati, conteggio candidate, top candidate con breakdown, sinergie e conflitti, scenari
valutati, robust score, motivo di `FIRE`/`HOLD`.

**Non si replica all'avversario durante il Planning.** È la stessa regola di `D-043` per le reazioni, e vale
qui perché un trace contiene esattamente ciò che il §2 vieta di conoscere.

> **Aperto**: visibilità del trace in replay e spectator. L'innesco è l'archivio replay (`R1`, `D-077`) più un
> bot che produca trace; nessuno dei due esiste.

Comandi e overlay di debug si nomineranno secondo la convenzione `rt.*` già in uso, quando ci sarà qualcosa
da stampare.

---

## 12. Cosa misurare — senza soglie, perché nessuno ha profilato

Conteggio e tempo di generazione candidate · rapporto di potatura · dimensione del Top-K · combinazioni di
squadra valutate · tempo del team planner · celle di belief · scenari · cache hit · ripianificazioni · cambi
di piano.

Nessun numero è fissato qui, ed è deliberato: il §62 del sorgente lo dice e ha ragione. **Il comportamento
decisionale resta deterministico anche se la misura varia** — è il senso del §3.

---

## 13. Fuori scope

Learning Agents, RL e reti neurali come dipendenza del bot competitivo · Mass AI · minimax profondo e Monte
Carlo in v0.1 · Strategic Director multi-turno · opponent model che legge il profilo avversario invece di
dedurlo da eventi pubblici · bonus di HP o danno come livello di difficoltà · qualunque ramo per nome d'eroe.

---

## 14. Riferimenti

- Il bot di oggi: [`spec-bot-hex.md`](spec-bot-hex.md) · codice `Source/RefactorTactics/Bot/RTHexBotLibrary.{h,cpp}`
- Conoscenza di squadra: `Source/RefactorTactics/Perception/RTTeamKnowledge.h` · [`brief-conoscenza-parziale.md`](brief-conoscenza-parziale.md)
- Reazioni: [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) · [`brief-overwatch-reazioni.md`](brief-overwatch-reazioni.md)
- Ownership e sinergie: [ADR-0006](../decisions/adr-0006-ownership-abilita-sinergie.md)
- Decisioni: `D-095`–`D-099` in [`../decisions/RT_PDR_00_Decision_Log.md`](../decisions/RT_PDR_00_Decision_Log.md)
- Referto di consolidamento: [`../archive/roadmap-plans/bot-ai-consolidamento-2026-08-11.md`](../archive/roadmap-plans/bot-ai-consolidamento-2026-08-11.md)
- Wiki, lato giocatore: [`avversario-bot` (Wiki)](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/avversario-bot)
