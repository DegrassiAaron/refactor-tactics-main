# CLAUDE HANDOFF — RefactorTactics
## Reaction Outcome Preview · Fast Reaction · Decision Time Bank · E14 Roadmap

> `HISTORICAL` · **Kit d'autore consumato**, non una fonte. · **Consumato**: 2026-08-28 · **Base**:
> `e3911eed` (`main`).
>
> Archiviato da [`docs/archive/`](../../README.md): vale per la **provenienza** e il rationale, mai per la
> regola. Il file stava alla radice del repository come
> `CLAUDE_RefactorTactics_ReactionOutcome_Roadmap_Handoff_2026-08-28_v0.2.md`, untracked, 1436 righe.
>
> **Cosa possiede**: il mandato d'autore del 2026-08-28 sulla Reaction Outcome Preview, le sue trentuno
> sezioni e la roadmap `R0`–`R9`, verbatim.
> **Cosa non possiede**: nessuna autorità, e nessuna esecuzione. Il referto completo — misura per misura — è
> [`../../../roadmap/plans/reaction-outcome-preview-handoff-spec-panel-2026-08-28.md`](../../../roadmap/plans/reaction-outcome-preview-handoff-spec-panel-2026-08-28.md).
>
> ⛔ **Il predecessore che il §Supersede dichiara non esiste.** `CLAUDE_..._2026-08-27.md` non è mai entrato
> nel repository (`git log --all` sul path: vuoto), quindi la catena di supersessione dichiarata in testa
> **non è verificabile** e la v0.1 non è recuperabile. È il secondo kit consecutivo di questa serie ad
> arrivare senza un predecessore tracciato.

## Il verdetto, in breve

✅ **Disciplinato sui divieti.** La §13 — preview side-effect-free — è la sezione meglio costruita: undici
divieti falsificabili **singolarmente**, più la prescrizione di confrontare hash e stato prima e dopo la
query. La §11 formula il test di privacy come oracolo eseguibile (*«hidden choice A vs B ⇒ payload
identico»*), che è la forma già in uso in `Source/` come `UI.NoEnemyIntentExposed`. La correzione di
ownership del Time Bank (§1.3, §12) è **esatta** e allineata a [`#1206`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1206).
Il dedup della §1.6 è stato **rifatto** in questa sessione e regge: nessuna issue equivalente esiste.

🔴 **Scaduto sulle premesse su cui poggia la roadmap.** Quattro rilievi critici:

- **`#886` è CLOSED**, e la §18 la mette come **primo passo di lavoro** (`R1`), la §5 la disegna come nodo
  attivo, la §16 la dichiara prerequisito e la §29 la elenca sotto `TODAY`. L'handoff *possiede* la domanda
  giusta — §22: *«#886 è ancora aperta?»* — quindici sezioni **dopo** averne assunto la risposta.
- L'invariante centrale `Confirmed preview = Committed outcome` (§7) è stata dichiarata **non
  falsificabile** il **2026-08-28**, in questo repository, da
  [`dir-b-core-gameplay-directive-spec-panel-2026-08-28.md`](../../../roadmap/plans/dir-b-core-gameplay-directive-spec-panel-2026-08-28.md) §7.
  La correzione già scritta e non recepita è: *una funzione pura, due chiamanti*.
- **`AppliedDamage` ha zero occorrenze in `Source/`**, e la §8 lo presenta come riuso. Le sue nove
  occorrenze stanno tutte in `docs/`, in **due file del 2026-08-28** che dicono esattamente questo — fra
  cui la premessa `P4` di [`roadmap-main-v0.1.md`](../../../roadmap/roadmap-main-v0.1.md): *«⛔ non esiste»*.
- La roadmap `R0`–`R9` è una **terza numerazione** per un lavoro che ha già i checkpoint (`CP 14.6`–`14.8`)
  e le wave (`B0`–`B6`, scritte lo stesso giorno con le sedi verificate lato server) — mentre la §18 apre
  dichiarando *«questa NON è una nuova roadmap parallela»*.

🟠 **E il seam che presuppone mancante è atterrato**: [`#512`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/512)
è **CLOSED**, `RefactorTactics.ShowcaseRelay.DecisionProviderIsInjectable` è verde e
`Spec.Overwatch.HoldThenFire` si esegue. Il documento non nomina **nessuno** dei tre termini — ed è il
secondo kit consecutivo con lo stesso punto cieco.

---


**Revisione:** v0.2  
**Data audit:** 2026-08-28  
**Repository:** `DegrassiAaron/refactor-tactics-main`  
**Destinatario:** Claude / Claude Code  
**Supersede:** `CLAUDE_RefactorTactics_ReactionOutcome_Roadmap_Handoff_2026-08-27.md`

---

# 0. SCOPO

Questo file consolida lo stato corrente del cluster **E14 — Overwatch e reazioni interattive** e la nuova esigenza **Reaction Outcome Preview**.

Obiettivo UX/gameplay consolidato:

> Durante una **Reaction Decision Window**, mentre sono visibili countdown / Grace / Decision Time Bank, il giocatore deve poter capire **quanto è efficace ogni response legale prima del commit**: target, HIT/MISS quando determinabile, danno effettivo applicato e livello di certezza.

Il preview:

- NON è un secondo resolver;
- NON è client-authoritative;
- NON muta lo stato;
- NON usa dati futuri o privati dell'avversario;
- deve avere parity col commit quando l'informazione è marcata `Confermato`;
- deve essere calcolato sullo **stato valido al Decision Boundary**, non su una preview di Planning ormai stale.

Non creare una nuova Epic. L'owner resta **#152 E14**.

---

# 1. DELTA RISPETTO ALL'HANDOFF 2026-08-27

Ci sono cambiamenti materiali. Per questo questa revisione v0.2 sostituisce il file precedente.

## 1.1 NUOVA — #1562, CP 22.2

Aperta il **2026-08-28**:

```text
#1562 — CP 22.2 · La linea di tiro si rivaluta al Decision Boundary, non al Planning
state: OPEN
release: post-v0.1 / v0.2
priority: P2
```

Regola nuova rilevante:

```text
Planning preview != autorità del tiro
Decision Boundary = punto in cui LOS viene rivalutata
preview stale non può concedere un tiro
```

#1562 dichiara esplicitamente di essere **tracciata ma non pronta**: non autorizza a saltare i gate v0.1.

Impatto su Reaction Outcome Preview:

- la preview live non deve congelare un risultato nato nel Planning;
- quando si apre la Decision Window, l'outcome query deve usare il **boundary snapshot/stato corrente autorizzato**;
- la privacy deve usare `TeamKnowledge`/equivalente a monte: filtrare soltanto in UMG non basta;
- #1562 è owner futuro della **rivalutazione LOS autorevole**, non della UI E14.

Ownership da non confondere:

```text
E14/#166  -> live Decision Window / UI
nuova child issue E14 -> contratto Reaction Outcome Preview, se ancora mancante
#1392     -> explainability cover/facing/log
E21       -> presentazione preview generale
E22/#1562 -> LOS rivalutata autorevolmente al boundary, post-v0.1
```

## 1.2 NUOVO NELLA CATENA — #1118

```text
#1118 — La risposta di reazione e la sua ragione sono un enum solo...
state: OPEN
priority: P3
```

Problema:

```text
ERTReactionDecisionOutcome
```

confonde oggi:

```text
WHAT  = response applicata
WHY   = motivo/esito della decisione
```

La correzione proposta separa:

```text
CommittedResponse
DecisionOutcome
```

Vincoli di dipendenza:

```text
#1118 NON blocca #886
#1118 PRECEDE #314 Reaction Profile / Reaction Clash
```

Questa dipendenza deve entrare nella roadmap E14. Non implementare Reaction Profile parametrico continuando ad allungare un enum Overwatch-specific.

## 1.3 CORREZIONE TIME BANK — #1206

```text
#1206 — Il soggetto del Time Bank e' attribuito a CP 19.3...
state: OPEN
```

Correzione da mantenere:

```text
CP 19.3 / #1124 -> campo/formato UnitsPerPlayer
CP 14.8 / #319  -> identità/soggetto reale della Decision Window e del Time Bank
```

Quindi NON riportare in vita la vecchia premessa:

```text
"#319 è bloccata perché CP 19.3 deve ancora inventare il soggetto"
```

Il campo di formato è groundwork già atterrato; il **subject runtime** resta responsabilità di #319.

#1206 è anche un esempio importante di documentazione storica contenente strumenti ormai rimossi: Feature Registry, `parallel-batch.yaml` e i vecchi gate Python non sono più autorità eseguibile.

## 1.4 INPUT DELLE REAZIONI — E48 #1408

Stato misurato il 2026-08-26:

```text
#1409 generiche/input -> CLOSED
#1034 reazioni armabili dal giocatore -> CLOSED
#166 finestra live umana -> ancora il gap reale
```

Quindi il giocatore può ormai **armare** le reazioni, ma una vera partita umana non possiede ancora il percorso live completo della Decision Window: E48 registra che `ReactionDecider` è ancora legato dallo scenario/harness e il default runtime umano ricade su `HoldNoDecider` finché #166 non chiude il percorso.

Questo rende #166 ancora più chiaramente il prossimo consumer UI/runtime della Outcome Preview.

## 1.5 #1158 È CHIUSA

```text
#1158 — doppio Overwatch sullo stesso mover / secondo FIRE su target già KO
state: CLOSED
```

Non trattarla come backlog aperto.

Usarla però come precedente di correttezza:

> lo stato può cambiare tra opportunity seriali; una preview non deve essere precomputata una volta per tutte per tutte le future opportunity.

La query deve essere rivalutata per la **opportunity/boundary corrente** e aggiornata su eventi/selection, non su Tick.

## 1.6 NESSUNA ISSUE DEDICATA “REACTION OUTCOME PREVIEW” TROVATA

Audit per termini:

```text
Reaction Outcome Preview
outcome preview
damage preview
hit preview
```

non ha trovato una issue dedicata equivalente.

Il solo risultato vicino è #705 sul Pointer Interaction Contract, che non possiede il calcolo/contratto dell'outcome.

Pertanto resta valida la proposta di creare **una sola child issue E14**, ma solo dopo un ultimo search-first immediatamente prima della write.

## 1.7 GITHUB WRITE — STATO OPERATIVO

Storico della chat precedente:

```text
create issue -> 403 Resource not accessible by integration
add comment  -> 403 Resource not accessible by integration
```

Nell'audit odierno il connector riesce a leggere repository e issue e il repository espone permessi elevati, ma **la mutazione delle issue non è stata ri-testata**.

Regola per Claude:

1. non assumere che il vecchio 403 sia ancora valido;
2. non assumere nemmeno che la write funzioni;
3. fare prima search/dedup;
4. se `gh auth status` e il token hanno write, usare `gh`;
5. fallire in modo trasparente senza creare workarounds documentali paralleli.

---

# 2. GERARCHIA DELLE FONTI

Ordine vincolante:

```text
1. repository HEAD / codice / test / dati correnti
2. issue GitHub correnti
3. ADR / Decision Log / spec owner vive
4. roadmap corrente
5. handoff consolidati recenti
6. chat / handoff storici
```

Se un documento storico contraddice HEAD o una decisione successiva:

```text
STALE / SUPERSEDED
```

Non “correggere” il codice per farlo tornare coerente con una fonte vecchia.

## Tracking corrente

Dopo D-178 / D-181 / D-182:

- sviluppo sequenziale: niente vecchio parallel-batch operativo;
- Feature Registry rimosso;
- script/gate Python documentali rimossi;
- lo stato feature vive principalmente in:

```text
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-checkpoint.md
```

Verificare sempre i path reali a HEAD.

---

# 3. BASELINE TECNICA

Progetto: **RefactorTactics**, tattico competitivo PC-first UE5.

Versione attesa dal contesto corrente: **UE 5.8.1**; verificare comunque `EngineAssociation`, `.uproject` e branch corrente prima di scrivere codice.

Principi invarianti:

```text
client propone; server valida/applica
simulation authority != presentation
snapshot / ordering / IDs deterministici
TurnLog e replay canonici
planning avversario mai replicato ai client nemici
intent alleati team-only
same snapshot + rules/version + seed -> same result
combat preview non usa percentuali se il sistema è deterministico
```

---

# 4. OWNER E14 CORRENTE

Epic:

```text
#152 — [EPIC v0.1] E14 — Overwatch e reazioni interattive
```

Checkpoint noti:

```text
14.1  #161  CLOSED
14.2  #162  CLOSED
14.3  #163  CLOSED
14.4  #164  CLOSED
14.5  #165  CLOSED
14.6  #166  OPEN — live Decision Window, FIRE/HOLD UI, countdown, pacing
14.7  #314  OPEN — Reaction Profile + Reaction Clash
14.8  #319  OPEN — Decision Time Bank
```

NON creare:

```text
nuova Epic Reaction Outcome
nuovo Reaction System
nuovo Time Bank System
nuova fase Reaction
CP 14.9 automaticamente
```

Reaction è una **Decision Boundary / conditional branch**, non una quinta macro-fase.

---

# 5. DIPENDENCY GRAPH CONSOLIDATO

```text
                         #152 E14
                            |
           +----------------+----------------+
           |                                 |
        #886                              #165 CLOSED
 Replay consumes                           runtime baseline
 reaction trace                               |
           |                                   |
           +-------------> #166 <--------------+
                            |
              live human Decision Window
              FIRE/HOLD + pacing + UI
                            |
                +-----------+-----------+
                |                       |
              #319                    #1118
          Decision Time Bank      response != reason
                |                       |
                |                       v
                |                     #314
                |             Reaction Profile / Clash
                |                       |
                +----------- retune ----+

Cross-cutting:
#172/#173 -> ReactionPreview + Confirmed/Predicted/Uncertain precedent
#1392     -> effective cover/facing damage + explainability
#705      -> ReactionWindow input precedence / keyboard-controller parity
#1408     -> reaction arming is reachable; live window still #166
#1206     -> subject ownership correction for #319
#1562     -> post-v0.1 LOS re-evaluation at boundary
#1158 CLOSED -> sequential-opportunity correctness precedent
```

## Sequence rules

### #886 -> #166

#886 deve atterrare **prima o insieme a #166**: quando esiste un decisore umano, il replay/verifier deve consumare le decisioni registrate dal TurnLog invece di interpellare un decisore live diverso.

### #1118 -> #314

#1118 non blocca #886. Serve prima di Reaction Profile/Clash perché risposte come `SIDESTEP:<Cell>` non devono diventare nuovi valori di un enum Overwatch-specific.

### #166 -> #319

#319 assume la Decision Window live; il Time Bank è una risorsa temporale del decisore, non il produttore della finestra.

### #314 e #319

Sono estensioni indipendentemente tagliabili dopo #166. Se #314 entra prima della chiusura/tuning di #319, ripetere la misura del pacing Time Bank sul caso Clash.

### #1124 / #1206 -> #319

`UnitsPerPlayer` è groundwork di formato. Il soggetto runtime `ControlledHeroes`/decision owner resta da materializzare dentro la catena di #319. Non creare un nuovo owner.

### #1562

Post-v0.1 e non ready. Non blocca il vertical slice v0.1, ma stabilisce una direzione architetturale che il nuovo preview non deve contraddire: **boundary current state wins over Planning stale preview**.

---

# 6. #166 — IL GAP LIVE REALE

#166 possiede già:

- UI `FIRE` / `HOLD`;
- countdown `FastReactionDuration = 3.0 s`;
- target;
- DTO sanitizzato;
- timeout safe `HOLD` / `HoldTimeout`;
- slow-motion solo presentazione;
- privacy della finestra;
- `ReactionDecisionSeconds` separato da `ResolutionPlaybackSeconds`;
- pacing p50/p90/playtest;
- futuro tuning Time Bank.

E48 #1408 conferma una cosa importante:

```text
reaction can be armed by the human
BUT
live human Decision Window is still missing
```

Quindi non iniziare dalla UMG isolata: prima chiudere il seam runtime input/decisione e la query dell'outcome.

---

# 7. REACTION OUTCOME PREVIEW — CONTRATTO CANONICO PROPOSTO

Per ogni **response offensiva legale** della opportunity corrente, il decisore può vedere:

```text
Target
HitState        = Hit | Miss | Uncertain
AppliedDamage   = valore effettivo se determinabile
Certainty       = Confirmed | Predicted | Uncertain
ReasonCodes     = minimo necessario
Breakdown       = player-facing e privacy-safe
```

Esempio:

```text
TARGET: Wraith
FIRE

HIT · 14 DAMAGE
Cover: -2
CONFERMATO

Time Bank: 18.4 s
Window: 2.1 s
```

Caso contested:

```text
SHIFT
Outcome: INCERTO
Danno finale non determinabile prima del reveal
```

## Invariante principale

```text
Confirmed preview(Response R, Boundary B)
=
Committed outcome(Response R, Boundary B)
```

Se la UI mostra `CONFERMATO · HIT · 14`, il commit sullo stesso boundary deve applicare `HIT · 14`.

Se esistono input autorizzati non ancora risolti, il risultato NON va etichettato Confirmed.

---

# 8. APPLIED DAMAGE, NON DANNI NOMINALI

Mostrare:

```text
AppliedDamage
```

Non mostrare come outcome definitivo:

```text
CatalogDamage
BaseDamage
Armed.Damage grezzo
```

Il preview deve riusare il percorso puro/autorevole delle mitigazioni già determinabili, incluse dove applicabili:

- cover;
- facing;
- shield;
- armor;
- status/modificatori;
- immunità/invalidazioni correnti.

Non duplicare formule dentro UMG o ViewModel.

#1392 resta owner della spiegazione/traccia del “perché”; Reaction Outcome Preview consuma il valore autorevole e un breakdown minimo autorizzato.

---

# 9. DECISION BOUNDARY E STALE PREVIEW

Dopo #1562 il contratto deve essere esplicito:

```text
Planning preview = advisory/prediction
Decision Boundary outcome query = current authoritative basis
```

Non usare il percorso/LOS/cover del Planning come grant di legalità.

Quando la Decision Window si apre:

1. ottenere la opportunity sanitizzata corrente;
2. interrogare lo stato/snapshot del boundary;
3. rivalutare ciò che il ruleset corrente richiede;
4. costruire la preview soltanto da dati autorizzati;
5. il commit ripete/consuma la stessa semantica pura.

Per v0.1 non anticipare implementazioni E22 non pronte, ma evitare una API che renda impossibile il comportamento futuro.

---

# 10. CERTAINTY

Riutilizzare il vocabolario già introdotto da #172/#173:

```text
CONFERMATO
PREVISTO
INCERTO
```

Interpretazione per la Reaction Window:

### CONFERMATO

Tutti gli input necessari sono noti/autorizzati e frozen per quella response sul boundary corrente.

### PREVISTO

Usare solo se il progetto possiede davvero un'anteprima predittiva fuori dal boundary o un dato non frozen ma legittimamente stimabile. Non inventare probabilità.

### INCERTO

Il payoff dipende da un input non ancora rivelato/autorizzato, ad esempio hidden response in Reaction Clash.

Non mostrare percentuali di hit chance se il sistema non è probabilistico.

---

# 11. REACTION CLASH — NO SIDE CHANNEL

#314 introduce scelta nascosta con fixed-deadline reveal.

Prima del reveal:

```text
own response = known
opponent response = hidden
```

Se il risultato finale dipende dalla hidden choice:

```text
Certainty = Uncertain
```

Test privacy obbligatorio:

```text
same public/authorized state
opponent hidden choice A vs B
=> pre-reveal preview payload identical to unauthorized client
```

Il preview non deve permettere di inferire:

- quale response ha scelto l'avversario;
- quando l'ha lockata;
- il suo Time Bank;
- future opportunity;
- intent/path/position future private.

---

# 12. TIME BANK — RAPPORTO CON OUTCOME PREVIEW

Regola assoluta:

```text
Time Bank
Grace
ExhaustedGrace
countdown
```

NON modificano:

```text
HitState
AppliedDamage
AllowedResponses
combat legality
resolver result
```

Modificano solo la disponibilità temporale del giocatore e il fallback di timeout secondo la Decision Definition.

## PreferredResponse

#319:

```text
PreferredResponse != committed response
PreferredResponse != TimeoutResponse
```

Quindi può:

- preselezionare una response;
- evidenziarne immediatamente la Outcome Preview;
- essere confermata con Quick Confirm.

Non può:

- commit automatico;
- cambiare AllowedResponses;
- spendere charge solo perché preselezionata;
- sostituire il timeout safe `HOLD` per Overwatch.

## Subject del bank

Correzione #1206:

```text
UnitsPerPlayer -> formato/capacità
ControlledHeroes / decision owner -> runtime subject di #319
```

Non rinviare di nuovo il subject a CP19.3.

---

# 13. PREVIEW SIDE-EFFECT-FREE

Una query di preview non deve:

```text
mutare HP
mutare Shield
applicare Status
spendere charge
spendere cooldown
fermare movimento
cambiare occupancy
scrivere un combat outcome nel TurnLog
cambiare seed
aprire nuove Reaction Window
cambiare il Time Bank al di fuori del normale wall-clock owner
```

Il risultato è una query/read model.

La suite deve confrontare hash/stato prima e dopo la query.

---

# 14. MULTI-TARGET E OPPORTUNITY SERIALI

Per Overwatch multi-target:

```text
FIRE:A -> preview A
FIRE:B -> preview B
HOLD   -> no offensive damage preview
```

Ogni response ha preview indipendente.

Il precedente #1158, ormai chiuso, dimostra perché non conviene precomputare il futuro: un outcome precedente può rendere un target successivo KO/non valido.

Regola:

```text
preview = current opportunity + current boundary state
```

Aggiornare quando cambiano gli input logici della window/selection, non per ogni Tick.

---

# 15. DTO — FORMA SUGGERITA, NON AUTORITÀ

Prima cercare tipi equivalenti a HEAD.

Possibile forma minima:

```text
FRTReactionResponsePreview

OpportunityId
ResponseId / CommittedResponse candidate
TargetStableId
HitState
AppliedDamage?
ShieldDelta?
HealthDelta?
Certainty
ReasonCodes[]
Breakdown[]
```

Non aggiungere campi solo per comodità UI se producono leakage.

Il DTO non deve contenere:

- enemy future path;
- enemy future position;
- enemy private intent;
- future opportunity count;
- hidden Clash response;
- opponent Time Bank;
- dati non autorizzati da TeamKnowledge.

---

# 16. NUOVA CHILD ISSUE E14 — SOLO SE IL GAP È ANCORA REALE

Titolo consigliato:

```text
E14 · Reaction Outcome Preview — hit/miss e danno durante la Decision Window
```

Non assegnare automaticamente `CP 14.9`.

Body consigliato:

```markdown
**Epic**: #152 (E14)
**UI consumer**: #166
**Replay prerequisite**: #886
**Time context**: #319
**Contested extension**: #314
**Response model prerequisite for #314**: #1118
**UI/certainty precedents**: #172/#173/#705
**Explainability**: #1392
**Future boundary LOS rule**: #1562 (post-v0.1, tracked/not-ready)

## Obiettivo

Durante una Decision Window live, mostrare al decisore l'efficacia di ogni response legale prima del commit: HIT/MISS quando determinabile, AppliedDamage effettivo e Certainty.

La UI non calcola gameplay. La preview usa una query side-effect-free basata sullo stesso ruleset del commit e sul boundary state corrente.

## DoD

- [ ] Ogni response offensiva legale espone Target, HitState, AppliedDamage quando determinabile, Certainty e reason/breakdown minimo autorizzato.
- [ ] AppliedDamage include le mitigazioni correnti applicabili; non è il danno nominale da catalogo.
- [ ] Multi-target: ogni FIRE target ha preview indipendente.
- [ ] Una preview `Confirmed` coincide col commit sullo stesso boundary.
- [ ] Preview side-effect-free: nessuna mutazione gameplay, charge o combat TurnLog.
- [ ] Planning/stale preview non concede legalità al Decision Boundary.
- [ ] Time Bank/Grace/countdown non cambiano combat outcome o AllowedResponses.
- [ ] PreferredResponse evidenzia/preseleziona ma non committa.
- [ ] Reaction Clash non rivela hidden choice/payoff prima del reveal.
- [ ] Nessun future/private enemy state entra nel payload.
- [ ] Aggiornamento selection/event-driven, non full recompute su Tick.
- [ ] Mouse, keyboard e controller permettono di confrontare/confermare le response equivalenti.
- [ ] Preview completa entro il target progetto (<50 ms nel vertical slice).

## Test minimi

- `RefactorTactics.ReactionPreview.ConfirmedMatchesCommittedOutcome`
- `RefactorTactics.ReactionPreview.CoverAndFacingUseAppliedDamage`
- `RefactorTactics.ReactionPreview.MultiTargetResponsesAreIndependent`
- `RefactorTactics.ReactionPreview.PreviewDoesNotMutateState`
- `RefactorTactics.ReactionPreview.TimeBankDoesNotAffectOutcome`
- `RefactorTactics.ReactionPreview.PreferredResponseOnlyHighlights`
- `RefactorTactics.ReactionPreview.ClashDoesNotRevealHiddenChoice`
- `RefactorTactics.ReactionPreview.NoFutureIntentLeak`
- `RefactorTactics.ReactionPreview.StalePlanningPreviewDoesNotGrantOutcome`
```

I nomi sono proposti: deduplicare contro i test reali prima di crearli.

---

# 17. PATCH DI TRACKING ALLE ISSUE ESISTENTI

## #152

Aggiungere una nota/cross-link, non un nuovo checkpoint automaticamente:

```text
Reaction Outcome Preview è un consumer trasversale E14:
#166 live UI
#886 replay seam
#319 Time Bank / PreferredResponse
#314 contested privacy/certainty
#1118 response model prima di #314
#1392 effective-damage explainability
#1562 future LOS authority at boundary
```

## #166

Estendere la Decision Window con:

```text
Target
HitState
AppliedDamage
Certainty
minimal reason/breakdown
```

Vincoli:

```text
no combat formula in widget
live human ReactionDecider path
countdown does not change outcome
```

## #319

Consolidare:

```text
Bank/Grace/countdown != combat authority
PreferredResponse highlights preview, does not commit
Decision subject belongs here, not CP19.3
```

Linkare #1206 come correzione di ownership documentale.

## #314

Aggiungere il contratto:

```text
pre-reveal preview cannot reveal hidden opponent response/payoff
final outcome dependent on hidden choice = Uncertain
```

Dipendenza strutturale:

```text
#1118 before #314
```

## #172 / #173

Solo cross-link:

```text
ReactionPreview remains separate from phases
reuse Confirmed / Predicted / Uncertain vocabulary
```

## #1392

Cross-link senza duplicazione:

```text
Reaction Outcome Preview consumes effective AppliedDamage
#1392 owns why/trace/cover-facing readability
```

## #705

Usare il contratto di precedenza:

```text
Modal / ReactionWindow > HUD > world tactical hit
```

e mantenere equivalenza mouse/tastiera/controller.

## #1562

NON spostarla in v0.1 e NON trasformarla in prerequisite immediata.

Aggiungere, se utile, soltanto il cross-link concettuale:

```text
live Reaction Outcome Preview must not rely on stale Planning LOS;
post-v0.1 authoritative LOS re-evaluation is owned by #1562.
```

---

# 18. ROADMAP OPERATIVA CONSOLIDATA

Questa NON è una nuova roadmap parallela. È l'ordine proposto da riportare negli owner reali.

## R0 — Reconciliation

- audit HEAD;
- leggere roadmap vive;
- deduplicare issue;
- verificare stato #152/#166/#314/#319/#886/#1118/#1206/#1392/#705/#1562;
- creare la sola Outcome Preview child issue se ancora mancante;
- correggere riferimenti stale a Feature Registry / parallel-batch / vecchi gate.

Exit:

```text
one owner per responsibility
zero duplicate issue
current dependencies recorded
```

## R1 — Replay seam prima della finestra umana (#886)

Prima o insieme a #166:

- il Verifier consuma reaction decision dal TurnLog;
- lookup per `OpportunityId`, non per ordine;
- collapsed window ricalcolata, non riletta;
- orphan/illegal/missing disagreement => verification error/fail-closed secondo decisione owner;
- live decider non viene interrogato quando la trace possiede la risposta.

Exit:

```text
human decision can exist without breaking deterministic resimulation
```

## R2 — Pure Reaction Outcome Query

Prima della UMG:

```text
Response candidate + current Decision Boundary state
-> side-effect-free ReactionOutcomePreview
```

- cercare funzioni pure esistenti;
- estrarre soltanto se necessario;
- stessa semantica di hit/mitigation del commit;
- test parity/mutation.

Exit:

```text
ConfirmedMatchesCommittedOutcome green
PreviewDoesNotMutateState green
```

## R3 — Sanitized Preview DTO / Privacy

- DTO owner-only / authorized;
- TeamKnowledge a monte;
- no future/private data;
- payload invariant su hidden opponent choice pre-reveal;
- stable IDs, non Actor pointer come contratto di rete.

Exit:

```text
privacy canaries green
```

## R4 — CP 14.6 Live Human Decision Window

- collegare il decisore umano al runtime;
- FIRE/HOLD e target response;
- countdown 3s;
- Outcome Preview per response;
- input priority ReactionWindow;
- mouse/tastiera/controller;
- slowmo presentation only;
- timeout safe HOLD.

Scenario minimo:

```text
Watcher A
Mover B dietro cover
Mover C esposto
FIRE:B / FIRE:C / HOLD
switch response -> preview updates
commit -> Confirmed value equals applied result
```

Exit:

```text
PIE human can arm + receive + answer live reaction
```

## R5 — Explainability / #1392

- effective damage già autorevole;
- mostrare il reason breakdown minimo quando disponibile;
- cover/facing leggibili;
- no formula duplicata nel client.

Exit:

```text
player can explain why nominal damage != applied damage
```

## R6A — Decision Time Bank / #319

Dopo #166:

- per-player bank;
- owner-only;
- Grace/ExhaustedGrace;
- subject runtime (`ControlledHeroes`/decision owner) materializzato qui;
- PreferredResponse;
- Quick Confirm;
- timeout safe;
- replay usa il dato registrato, non wall-clock come regola di resolver.

Exit:

```text
same response + same boundary -> same combat outcome regardless of bank balance
```

## R6B — Generalize reaction decision model / #1118

Prima di #314:

```text
CommittedResponse != DecisionOutcome
```

- risposta parametrica rappresentabile;
- adapter trace v8;
- verifier non hard-coded per roster reactions;
- versioning fail-closed.

Exit:

```text
new reaction response does not require new reason-enum value
```

## R7 — Reaction Profile / Reaction Clash / #314

Solo dopo #1118 se #314 resta nel release cut:

- Reaction Profiles;
- contested derived from cardinality;
- hidden choices;
- fixed-deadline reveal;
- no nested Clash;
- Outcome Preview pre-reveal privacy-safe;
- final dependent payoff = Uncertain.

Se #319 è già implementata, ripetere tuning/pacing Time Bank sul Clash.

Exit:

```text
no hidden-choice/timing leak
```

## R8 — Pacing / packaged v0.1

Misurare:

```text
ReactionDecisionSeconds p50/p90
preview latency
response switch latency
timeouts
windows/turn
Time Bank spent, se #319 inclusa
60 FPS client
preview < 50 ms
```

Verificare packaged, non soltanto PIE.

## R9 — Post-v0.1 boundary LOS / #1562 + E21/E22

Solo quando i gate consentono il post-v0.1:

- LOS re-evaluated at Decision Boundary;
- stale Planning preview cannot grant shot;
- cover opened/closed symmetric tests;
- rejection reason code TurnLog;
- TeamKnowledge privacy;
- general presentation owner E21.

Questa fase non deve essere anticipata soltanto perché #1562 esiste.

---

# 19. TEST MATRIX MINIMA

## Correctness

```text
ReactionPreview.ConfirmedMatchesCommittedOutcome
ReactionPreview.CoverAndFacingUseAppliedDamage
ReactionPreview.MultiTargetResponsesAreIndependent
ReactionPreview.StalePlanningPreviewDoesNotGrantOutcome
```

## Side effects

```text
ReactionPreview.PreviewDoesNotMutateState
```

Asserire almeno:

```text
HP
Shield
statuses
charge
cooldown
occupancy
TurnLog combat entries
seed/relevant deterministic state
```

## Time Bank

```text
ReactionPreview.TimeBankDoesNotAffectOutcome
ReactionPreview.PreferredResponseOnlyHighlights
```

## Privacy

```text
ReactionPreview.ClashDoesNotRevealHiddenChoice
ReactionPreview.NoFutureIntentLeak
```

## Replay

#886 owns:

```text
Replay.Verifier.ReactionDecisionsComeFromTheTrace
Replay.Verifier.RecordedResponseBeatsLiveDecider
Replay.Verifier.OrphanRecordedResponseIsReported
Replay.Verifier.CollapsedWindowIgnoresTheTrace
```

## Future LOS (#1562)

```text
RefactorTactics.Cover.StalePreviewDoesNotGrantTheShot
RefactorTactics.Cover.WindowOpenedBeforeBoundaryGrantsTheShot
RefactorTactics.Cover.RejectionCarriesReasonCode
```

Non duplicare i test se esistono già con altri nomi.

---

# 20. AUDIT COMMANDS PER CLAUDE CODE

```bash
git fetch --prune origin
git status
git branch --show-current
git rev-parse HEAD

gh auth status
gh repo view DegrassiAaron/refactor-tactics-main

gh issue view 152
gh issue view 166
gh issue view 314
gh issue view 319
gh issue view 886
gh issue view 1118
gh issue view 1206
gh issue view 1392
gh issue view 705
gh issue view 1408
gh issue view 1562

gh issue list --state all --limit 500 --search '"Reaction Outcome Preview"'
gh issue list --state all --limit 500 --search '"outcome preview"'
gh issue list --state all --limit 500 --search '"damage preview"'
gh issue list --state all --limit 500 --search '"hit preview"'

git grep -n "ReactionPreview" -- Source Tests docs || true
git grep -n "ReactionDecider" -- Source Tests docs || true
git grep -n "AppliedDamage" -- Source Tests docs || true
git grep -n "EffectiveCoverReduction" -- Source Tests docs || true
git grep -n "ERTReactionDecisionOutcome" -- Source Tests docs || true
git grep -n "FRTReactionDecision" -- Source Tests docs || true
git grep -n "PreferredResponse" -- Source Tests docs || true
git grep -n "DecisionTimeBank" -- Source Tests docs || true
git grep -n "ControlledHeroes" -- Source Tests docs || true
git grep -n "TeamKnowledge" -- Source Tests docs || true
```

Produrre prima di scrivere:

```text
CURRENT
ALREADY IMPLEMENTED
OPEN
CLOSED
STALE DOC
MISSING
DUPLICATE RISK
OPEN DECISION
```

---

# 21. SEARCH-FIRST GITHUB WRITE PROCEDURE

Prima di creare la child issue:

```bash
gh issue list --state all --limit 500 --search '"Reaction Outcome Preview"'
gh issue list --state all --limit 500 --search 'reaction damage decision window'
```

Se esiste equivalente:

```text
UPDATE/LINK existing issue
DO NOT CREATE duplicate
```

Se non esiste:

```text
CREATE one child issue under #152
NO new Epic
NO automatic CP14.9
```

Poi commentare/cross-linkare gli owner reali invece di riscrivere corpi storici giganteschi, salvo necessità esplicita.

---

# 22. OPEN DECISIONS DA NON INVENTARE

Misurare/decidere con gli owner:

- quale funzione oggi produce il danno effettivo di un boundary shot?
- Shield/Armor sono già nello stesso percorso di commit?
- UI deve mostrare `HealthDamage`, `ShieldDamage` separati o solo `AppliedDamage` aggregato?
- esiste già un tipo condiviso per Certainty?
- quale DTO/view model arriva oggi alla Fast Decision UI?
- #886 è ancora aperta al momento del lavoro?
- l'errore trace/resimulation di #886 è verification error o nuovo log outcome?
- #314 rimane nel cut v0.1 o scende?
- #319 entra prima o dopo #314?
- quale semantica precisa ha una response che diventa invalida fra apertura e commit?
- in quale release entra davvero #1562? La issue dice esplicitamente post-v0.1/tracked-not-ready: rispettarlo.

---

# 23. ERRORI DA NON REINTRODURRE

```text
NO new Reaction Epic
NO CP14.9 inventato per comodità
NO Reaction as fifth phase
NO client combat formula
NO Damage formula in UMG
NO stale Planning preview as authority
NO future enemy path/position/intent in preview
NO opponent bank in preview
NO hidden Clash response in preview
NO percentages for deterministic hit result
NO color as only information channel
NO full simulation on Tick
NO timeout FIRE
NO PreferredResponse auto-commit
NO Time Bank changing combat outcome
NO #1124 re-owned as Time Bank subject
NO #1158 treated as open backlog
NO #1118 made blocker of #886
NO #314 implemented before #1118 without revisiting response representation
NO Feature Registry/parallel-batch resurrected
NO #1562 pulled into v0.1 just because it was opened early
```

---

# 24. DEBUG / OBSERVABILITY

Per ogni preview richiesta in dev/debug build poter ispezionare almeno:

```text
OpportunityId
ResponseId
TargetStableId
Boundary/Turn/MicroStep identity
HitState
AppliedDamage
Certainty
ReasonCodes
computation time
```

Non loggare dati privati su un canale replicato/visibile a client non autorizzati.

Il TurnLog continua a registrare **outcome committati**, non richieste speculative di preview come se fossero fatti di partita.

---

# 25. PERFORMANCE

Target progetto:

```text
client 60 FPS
Reaction/preview completa < 50 ms
intent update 8–12 Hz (planning; non usare come cadence della reaction preview)
server resolution MVP < 100 ms/match target di progetto
```

Outcome Preview:

- event/selection-driven;
- cache solo se key include tutto ciò che rende la preview valida;
- invalidare su boundary/state revision;
- non nascondere stale data dietro una cache aggressiva.

---

# 26. SETUP EDITOR / VERIFICA VISIVA

Prima scenario visuale v0.1:

```text
L_DevSandbox o scenario map owner corrente
2v2
Watcher A con Overwatch armato
B dietro cover
C esposto
Decision Window umana
```

Verificare:

1. reaction armabile da input umano;
2. movement trigger apre la window;
3. FIRE B / FIRE C / HOLD selezionabili;
4. preview cambia passando B/C;
5. danno mostrato è effettivo;
6. Confirmed == commit;
7. timeout => HOLD;
8. niente click-through al mondo;
9. keyboard/controller equivalenti;
10. client non autorizzato non riceve la opportunity/preview privata.

Poi packaged validation.

---

# 27. COMMIT GIT SUGGERITI

Adattare ai file reali:

```text
docs(reactions): reconcile E14 outcome preview dependencies
test(replay): consume recorded reaction decisions in verifier
test(reactions): pin preview commit parity
feat(reactions): expose side-effect-free response outcome query
test(privacy): sanitize reaction outcome preview payload
feat(ui): show reaction hit damage and certainty
feat(timebank): bind preferred response to live outcome preview
refactor(reactions): separate committed response from decision outcome
feat(reactions): add privacy-safe reaction clash preview
```

Commit piccoli, sequenziali, verificabili.

---

# 28. DEFINITION OF DONE DELLA REACTION OUTCOME PREVIEW

```text
[ ] owner/issue/roadmap consolidati, zero duplicati
[ ] pure/current-boundary outcome query
[ ] Confirmed preview == committed outcome
[ ] effective damage, non nominal damage
[ ] cover/facing coerenti col resolver corrente
[ ] stale Planning preview non usata come authority
[ ] preview side-effect-free
[ ] sanitized DTO / TeamKnowledge-safe
[ ] no enemy/future/hidden leak
[ ] Confirmed/Predicted/Uncertain corretti
[ ] multi-target response preview
[ ] human live Decision Window path
[ ] Time Bank independent from combat result
[ ] PreferredResponse no auto-commit
[ ] Clash no pre-reveal leak
[ ] replay seam non diverge con human decisions
[ ] Automation Tests
[ ] mutation/privacy tests
[ ] PIE visual verification
[ ] preview < 50 ms
[ ] packaged validation
```

---

# 29. ROADMAP SNAPSHOT PER UNA SESSIONE DI CLAUDE

Ordine di lavoro consigliato:

```text
TODAY / FIRST PASS
R0 audit + dedup + tracking
R1 #886 replay seam
R2 outcome pure query + parity tests
R3 sanitized DTO/privacy
R4 #166 live human window + UI

NEXT / IF IN CUT
R5 #1392 explainability integration
R6A #319 Time Bank + subject + PreferredResponse
R6B #1118 response/outcome split
R7 #314 Reaction Profile/Clash
R8 pacing + packaged v0.1

POST-v0.1
R9 #1562 boundary LOS + E21/E22 preview/readability
```

Nota: #1118 può essere lavorata appena serve la generalizzazione, ma deve essere chiusa prima di #314; non serve bloccare #166 o #886 per lei.

---

# 30. PROMPT DI AVVIO PER CLAUDE / CLAUDE CODE

> Tratta questo handoff v0.2 come contesto operativo e non come autorità superiore a HEAD. Auditare `DegrassiAaron/refactor-tactics-main`, la roadmap viva e le issue #152, #166, #314, #319, #886, #1118, #1206, #1392, #705, #1408 e #1562. Verificare prima se esiste già una issue equivalente a “Reaction Outcome Preview”; non creare duplicati e non creare una nuova Epic o CP14.9 automaticamente. Consolidare il contratto: durante una live Reaction Decision Window il decisore vede Target, HIT/MISS quando determinabile, AppliedDamage effettivo e Certainty; una preview Confirmed deve coincidere col commit sullo stesso Decision Boundary; nessuna formula combat in UMG, nessuna mutazione, nessun dato futuro/privato; Time Bank/Grace/countdown non cambiano combat outcome; PreferredResponse evidenzia ma non committa; Reaction Clash usa Uncertain quando il payoff dipende dalla hidden response. Integrare il nuovo vincolo di #1562 senza anticiparne il lavoro post-v0.1: Planning preview non è autorità e la LOS autorevole futura viene rivalutata al Decision Boundary. Mantenere #886 prima/insieme a #166, #1118 prima di #314, e correggere la vecchia attribuzione del subject Time Bank: #1124 porta il campo UnitsPerPlayer, #319 possiede il subject runtime. Prima di scrivere mostra CURRENT / IMPLEMENTED / OPEN / CLOSED / STALE / MISSING / DUPLICATE RISK; poi proponi patch issue/roadmap e il primo piano C++ test-first partendo dal pure outcome query, non dalla UMG.

---

# 31. CHANGELOG

## v0.2 — 2026-08-28

- aggiunta #1562 e regola LOS/current Decision Boundary;
- aggiunta dipendenza #1118 -> #314;
- consolidata correzione #1206: UnitsPerPlayer field vs #319 Decision Window subject;
- registrato E48 #1408: reaction arming raggiungibile, live human window ancora #166;
- #1158 corretto a CLOSED e mantenuto come precedente di serial-state correctness;
- rafforzata distinzione E14 live UI / E21 presentation / E22 LOS authority;
- aggiornata roadmap R0–R9;
- mantenuta una sola proposta di child issue Outcome Preview;
- aggiornato lo stato del precedente 403 GitHub come storico da ri-testare, non come fatto corrente garantito.

## v0.1 — 2026-08-27

- primo handoff consolidato Reaction Outcome Preview / E14 / Time Bank / Clash.

---

**Fine handoff v0.2.**
