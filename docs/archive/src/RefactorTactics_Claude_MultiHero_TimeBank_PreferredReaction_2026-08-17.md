# RefactorTactics — Handoff Claude/Cloud
## Multi-Hero Control, Decision Time Bank e Preferred Reaction Response

**Data consolidamento:** 2026-08-17  
**Repository:** `DegrassiAaron/refactor-tactics-main`  
**Scopo:** integrare nel repository le decisioni emerse dalla discussione, **consolidando ciò che esiste già** e senza creare sistemi paralleli o issue duplicate.

---

## 0. Regola operativa

Prima di modificare documentazione, issue o codice:

1. leggere `main` reale;
2. leggere gli owner documentali correnti;
3. leggere le issue aperte collegate;
4. verificare Feature Registry / roadmap / scenario map / decision log;
5. aggiornare l'owner esistente prima di creare un nuovo documento;
6. aggiornare una issue esistente se il suo scope copre già il lavoro;
7. creare una nuova issue solo se resta un deliverable autonomo non rappresentato;
8. non usare i vecchi PDR/PDF come autorità se la documentazione CURRENT del repository li ha superati.

Le decisioni sotto sono da integrare nel modello già esistente, non da implementare come sottosistema indipendente.

---

# 1. Decisione: Player e Hero non sono 1:1

RefactorTactics non deve assumere architetturalmente:

```text
1 Player = 1 Hero
```

Il modello deve supportare:

```text
Player
└── Control Group
    ├── Hero A
    └── Hero B   // opzionale
```

Il numero di Hero controllati da un Player deve essere definito dal formato/ruleset e non hard-coded nel `PlayerController`, nel `TurnManager` o nel resolver.

## Obiettivo di design

Baseline prevista:

- formato competitivo standard: può restare 1 Hero per Player;
- modalità future possono assegnare 2 Hero allo stesso Player;
- il vertical slice/offline può già beneficiare di un modello che non presuppone 1:1;
- in un futuro molto lontano il sistema non deve impedire modalità di scala molto maggiore, fino a ipotesi tipo **16 Hero vs 16 Hero**.

Questo **non** significa portare il 16v16 nella roadmap vicina.

È solo un guardrail architetturale.

## Regola proposta

Il Ruleset/Match Format deve poter esprimere concettualmente:

```text
MinControlledHeroesPerPlayer
MaxControlledHeroesPerPlayer
MaxHeroesPerTeam
ControlAssignmentPolicy
```

I nomi effettivi vanno scelti verificando il vocabolario esistente: **non introdurre questi campi letteralmente se il repository possiede già un modello equivalente**.

## Invariante del resolver

Il resolver non deve dipendere dal numero di persone che controllano le unità.

Deve continuare a risolvere unità/intenti tramite ID stabili e ordinamento deterministico.

L'associazione Player → Unit/Hero serve a:

- authorization/ownership;
- input;
- planning;
- Ready;
- privacy;
- UI;
- Reaction Decision ownership.

Non deve diventare una regola speciale dentro l'Action Resolver.

---

# 2. Planning con più Hero controllati

Se un Player controlla due Hero, il Planning resta **un'unica finestra temporale del giocatore**.

Il Player deve poter:

- passare rapidamente fra i propri Hero;
- vedere contemporaneamente i propri intenti;
- completare/validare gli intenti di entrambe le unità;
- usare combo fra i propri Hero;
- diventare `Ready` solo quando tutti gli intenti richiesti sono committabili.

Non creare due Planning Phase indipendenti.

## Stato Ready

Preferire il modello:

```text
Unit Intent:
- Draft
- Valid
- Committable
- Committed

Player Ready:
- vero solo se tutte le Controlled Units richieste sono pronte
```

Verificare i nomi e gli stati già esistenti prima di introdurre nuovi enum.

---

# 3. Decision Time Bank: deve considerare il carico di controllo

## Stato attuale verificato

La issue esistente:

- **#319 — CP 14.8 · Decision Time Bank**

copre già:

- bank **per Player**;
- bank condiviso fra tutte le Decision Window;
- `Grace`;
- `ExhaustedGrace`;
- `FastReactionDuration`;
- timeout;
- fallback preselezionato;
- replay/TurnLog;
- privacy;
- bot;
- test.

L'owner è:

```text
docs/gameplay/spec-decision-time-bank.md
```

La specifica corrente deriva `InitialBank` soprattutto da `RoundLimit`.

### Gap nuovo

Con 1–2 Hero per Player cambia il **carico decisionale per persona**.

Quindi `InitialBank`, e potenzialmente `Grace` / `ExhaustedGrace`, devono poter scalare anche in funzione del numero di Hero controllati.

Non creare un secondo Time Bank.

Aggiornare **#319** e il suo owner.

---

# 4. Policy proposta per il multi-Hero Time Bank

I valori numerici devono restare **PROPOSED FOR PLAYTEST**.

Non canonizzarli senza dati.

Una baseline iniziale ragionevole da portare a playtest è:

```text
ExtraHeroes = max(0, ControlledHeroCount - 1)

InitialBank =
    BaseInitialBank * (1 + 0.75 * ExtraHeroes)

Grace =
    BaseGrace + 0.50 s * ExtraHeroes

ExhaustedGrace =
    BaseExhaustedGrace + 0.25 s * ExtraHeroes
```

Con i valori proposti correnti:

```text
1 Hero:
InitialBank = baseline
Grace = 1.00 s
ExhaustedGrace = 0.75 s

2 Hero:
InitialBank ≈ baseline * 1.75
Grace = 1.50 s
ExhaustedGrace = 1.00 s
```

## Variante da playtest

È stata proposta anche una variante:

```text
+0.75 s di Grace per Hero extra
```

cioè:

```text
1 Hero → 1.00 s
2 Hero → 1.75 s
```

Non assumerla come baseline canonica.

Con una finestra massima di 3.0 s, 1.75 s gratuiti riducono molto la pressione effettiva del Time Bank.

Va confrontata tramite playtest con la variante `+0.50 s`.

---

# 5. Guardrail: il numero di Hero NON allunga la singola Fast Reaction

Da mantenere:

```text
FastReactionDuration = 3.0 s
```

Il numero di Hero controllati non deve produrre:

```text
1 Hero → 3.0 s
2 Hero → 3.75 s
```

Motivo:

- allungherebbe direttamente la Resolution;
- farebbe crescere il wall-clock per ogni prompt;
- renderebbe più grave il problema di pacing che il Time Bank serve a controllare.

Il carico aggiuntivo deve essere assorbito dal **budget aggregato** e/o dalla Grace, non dal limite massimo della singola finestra.

---

# 6. Reaction: distinguere Safe Timeout da Preferred Response

## Stato attuale verificato

La #319 e `spec-decision-time-bank.md` hanno già un requisito importante:

- il fallback è **preselezionato**;
- è raggiungibile con **un solo input**;
- mouse/tastiera/controller devono avere percorso equivalente;
- il tempo fino al primo input utile deve stare dentro la Grace.

Quindi il principio “reaction immediatamente confermabile” **esiste già**.

### Gap nuovo

Oggi il concetto è il **fallback sicuro della Decision Definition**.

La nuova richiesta è diversa:

> il Player può scegliere/preparare una risposta preferita che, se ancora legale quando la Reaction si apre, compare già selezionata.

Separare quindi due concetti:

```text
SafeTimeoutResponse
PreferredResponse
```

I nomi finali vanno allineati al vocabolario già presente.

---

# 7. Semantica obbligatoria della Preferred Response

Esempio Overwatch:

```text
SafeTimeoutResponse = HOLD
PreferredResponse   = FIRE
```

Quando si apre la finestra:

```text
REACTION

> FIRE <
  HOLD

[CONFIRM]
```

Se il giocatore aveva previsto correttamente la situazione:

```text
premi subito Confirm
→ FIRE
```

Se cambia idea:

```text
seleziona HOLD
→ Confirm
→ HOLD
```

Se non dà input:

```text
TIMEOUT
→ HOLD
```

## Invariante critico

**PreferredResponse non è la risposta di timeout.**

Quindi:

```text
PreferredResponse = FIRE
```

NON implica:

```text
timeout → FIRE
```

Timeout deve continuare a usare:

```text
SafeTimeoutResponse
```

Per Overwatch:

```text
timeout → HOLD
```

e deve restare valido il comportamento esistente:

- nessuna charge spesa al timeout;
- nessun `FIRE` automatico;
- reaction ancora armata quando le regole correnti lo prevedono.

---

# 8. Preferred Response e consumo di risorse

La preselezione non deve produrre effetti da sola.

Regola:

```text
preselected != committed
```

Quindi:

- visualizzare `FIRE` come preferito non consuma charge;
- solo un commit valido può consumare la risorsa;
- un timeout non trasforma la preferred response in commit;
- se la PreferredResponse non è più legale quando la finestra si apre, non va auto-committata né forzata.

Fallback suggerito:

```text
if PreferredResponse is legal:
    preselect PreferredResponse
else:
    preselect SafeTimeoutResponse
```

Il resolver/server continua a rivalidare la risposta al commit.

---

# 9. Quick Confirm

La UX deve supportare un input universale tipo:

```text
ConfirmCurrentReactionSelection
```

Durante una Reaction:

```text
pressione singola
→ commit della risposta attualmente selezionata
```

È stata proposta la **barra spaziatrice** come binding PC naturale.

Prima di fissarla:

1. verificare Enhanced Input e mapping correnti;
2. verificare conflitti con Space;
3. riusare un `IA_Confirm`/azione equivalente se esiste;
4. evitare input hard-coded nel widget.

Non introdurre direttamente `SpaceBar` nel codice della logica di Reaction.

La Reaction deve conoscere un **semantic action**, non un tasto fisico.

---

# 10. Reaction Profile / Clash

La issue:

- **#314 — CP 14.7 · Reaction Profile e Reaction Clash**

copre già il modello dove una Reaction può avere più risposte legali.

Non creare un altro sistema per Preferred Response.

La Preferred Response deve essere una policy/metadata di scelta sopra le `AllowedResponses`.

Esempio:

```text
Profile.Sidestep

AllowedResponses:
- HOLD
- SIDESTEP_LEFT
- SIDESTEP_RIGHT

PreferredResponse:
- SIDESTEP_LEFT
```

Se `SIDESTEP_LEFT` resta legale al Decision Boundary:

```text
> SIDESTEP LEFT <
  SIDESTEP RIGHT
  HOLD

[CONFIRM]
```

Se è diventata illegale:

```text
> HOLD <
  SIDESTEP RIGHT
```

o altra scelta sicura definita dalla Decision Definition.

Non cambiare la cardinalità delle `AllowedResponses` solo per effetto della preferenza.

---

# 11. Reaction multiple per Player multi-Hero

Con due Hero controllati dallo stesso Player possono aprirsi più opportunità nello stesso Decision Boundary.

Non serializzare automaticamente una finestra da 3 s per ogni Hero.

La direzione da preservare è:

```text
Decision Boundary
└── Player Decision Batch
    ├── Hero A opportunity
    └── Hero B opportunity
```

Una sola finestra di interazione può presentare più decisioni indipendenti del Player, se il modello di Reaction consente che coesistano nello stesso boundary.

Questo punto è **design futuro da verificare contro ADR-0004 / #314**.

Non implementarlo automaticamente dentro #319 se richiede modificare la semantica del Decision Boundary.

Se non è già coperto, creare una decisione/issue separata solo dopo aver dimostrato il gap.

---

# 12. Aggiornamenti richiesti alle issue

## #319 — UPDATE

Integrare almeno:

### Controlled Hero Load

```text
- InitialBank può scalare col numero di Hero controllati dal Player.
- Grace ed ExhaustedGrace possono scalare secondo policy data-driven.
- FastReactionDuration resta invariata.
- I coefficienti restano PROPOSED FOR PLAYTEST.
- Il conteggio usa le Hero realmente assegnate/controllate per quel match, non la dimensione teorica del roster.
```

### Preferred Response

```text
- Il Player può dichiarare una PreferredResponse per una Reaction/preparazione.
- Se è ancora legale all'apertura della finestra, viene preselezionata.
- Un singolo Confirm la committa.
- Se è illegale/stale, la UI torna a una selezione sicura.
- Il timeout usa sempre SafeTimeoutResponse.
- La PreferredResponse non consuma risorse finché non viene committata.
- Nessuna PreferredResponse può trasformare l'assenza di input in FIRE/azione irreversibile.
```

### Quick Confirm

```text
- percorso da prompt aperto a commit della risposta preselezionata = un input;
- input semanticamente configurabile;
- mouse, tastiera e controller equivalenti;
- misurato dentro la Grace.
```

## #314 — LINK / eventuale piccolo UPDATE

Non spostare lì la logica del Time Bank.

Aggiungere solo il collegamento concettuale se necessario:

```text
PreferredResponse non cambia AllowedResponses;
è una preferenza del decisore sopra il Reaction Profile.
```

---

# 13. Test da aggiungere/consolidare

Nomi indicativi: prima verificare la naming convention reale e non duplicare test esistenti.

## Time Bank

```text
TimeBank.TwoHeroesReceiveConfiguredLoadAllowance
TimeBank.HeroCountNeverExtendsMaxWindow
TimeBank.MultiHeroGraceUsesConfiguredPolicy
TimeBank.MultiHeroAllowanceIsRecordedForReplay
```

## Preferred Response

```text
Reaction.PreferredResponseIsPreselectedWhenLegal
Reaction.IllegalPreferredResponseFallsBackSafely
Reaction.TimeoutIgnoresPreferredResponse
Reaction.QuickConfirmCommitsPreferredResponse
Reaction.PreselectionDoesNotConsumeResource
Reaction.PreferredResponseDoesNotChangeAllowedResponses
```

## UI / Functional

```text
TimeBank.PreferredResponseReachableWithinGrace
Reaction.QuickConfirmWorksOnKeyboard
Reaction.QuickConfirmHasControllerEquivalent
```

## Multi-Hero ownership

Se manca copertura equivalente:

```text
Planning.PlayerMayControlMultipleHeroes
Planning.PlayerCannotCommitIntentForUnownedHero
Planning.ReadyRequiresAllControlledHeroIntents
```

I nomi sono suggerimenti, non prescrizioni.

---

# 14. Privacy

Non cambiare l'invariante di privacy.

Per il multiplayer futuro:

- il numero di Hero controllati da un Player può essere pubblico se il formato lo rende pubblico;
- il **bank residuo** resta owner-only secondo la #319;
- PreferredResponse è informazione privata del decisore;
- non inviarla agli avversari;
- non inserirla in GameState/PlayerState globalmente replicati;
- l'avversario non deve poter dedurre PreferredResponse da payload, timing o anticipazione della UI.

Il Quick Confirm non deve creare un canale che riveli la risposta prima del momento in cui la Reaction Definition permette il reveal.

Per Reaction Clash resta vincolante il reveal a scadenza fissa.

---

# 15. Determinismo e replay

Il wall-clock continua a non entrare nel resolver.

Registrare ciò che serve a riprodurre la decisione, non il tasto fisico premuto.

Il replay deve poter ricostruire:

```text
OpportunityId
Response committata
eventuale decision timing/bank canonico già previsto
```

Non serve che il replay conservi:

```text
"SpaceBar"
```

La PreferredResponse è rilevante nel replay solo se serve a verificare una proprietà di UI/decision policy.

L'esito competitivo dipende dalla **Response committata**, non dalla preselezione visuale.

---

# 16. Data-driven, senza nuovi numeri magici

I coefficienti per Hero extra devono stare nella stessa policy dei tempi di parete già usata dal Decision Time Bank.

Non hard-codare:

```cpp
if (HeroCount == 2)
{
    Grace += 0.5f;
}
```

Preferire concettualmente una policy:

```text
DecisionTimingPolicy
- BaseGrace
- ExtraControlledHeroGrace
- BaseExhaustedGrace
- ExtraControlledHeroExhaustedGrace
- ExtraControlledHeroBankFactor
```

I nomi reali devono essere scelti dopo aver letto il codice/owner corrente.

I parametri restano wall-clock/pacing e non devono diventare accidentalmente input del resolver deterministico se la specifica corrente li tiene fuori.

---

# 17. Scope

## Dentro il consolidamento

- Player può controllare 1–2 Hero come concetto architetturale.
- Ruleset-driven control count.
- Time Bank sensibile al carico di Hero controllati.
- Grace/ExhaustedGrace scalabili.
- FastReactionDuration invariata.
- PreferredResponse distinta da SafeTimeoutResponse.
- Quick Confirm a un solo input.
- aggiornamento #319.
- link/consistenza con #314.
- test e documentazione.

## Fuori scope

- implementare ora 16v16;
- networking per 16 giocatori;
- UI definitiva Grand Battle;
- aumentare `FastReactionDuration` per chi controlla più Hero;
- auto-FIRE su timeout;
- secondo sistema di Reaction;
- secondo Time Bank;
- hard-code di Space nella logica;
- nuove macro-fasi del resolver;
- modifiche al Reaction Clash non richieste dal gap reale.

---

# 18. Tracking Impact Pass richiesto

Claude deve verificare e aggiornare solo gli owner realmente impattati:

- `docs/gameplay/spec-decision-time-bank.md`
- ADR-0004 se e solo se la distinzione Preferred/SafeTimeout cambia un contratto normativo già definito
- spec Reaction Clash / Reaction Profile solo per link o chiarimento
- Decision Log se questa è una nuova decisione d'autore
- Feature Registry
- roadmap/checkpoint
- scenario map
- editor/manual test map se nasce verifica UI
- Wiki corrispondente
- issue #319
- issue #314 solo se serve
- eventuale documentazione del Match Format / Player ownership per il multi-Hero

Non modificare viste generate a mano: aggiornare le sorgenti e rigenerare col meccanismo del repository.

---

# 19. Criteri di accettazione del consolidamento

Il lavoro documentale è corretto quando:

1. nessun documento CURRENT assume più implicitamente `Player == Hero` dove il vincolo non è necessario;
2. #319 rappresenta esplicitamente il carico multi-Hero;
3. `FastReactionDuration` resta una sola e non cresce con HeroCount;
4. PreferredResponse e SafeTimeoutResponse sono semanticamente distinti;
5. il timeout non può produrre FIRE solo perché FIRE era preferito;
6. una PreferredResponse legale è confermabile con un input;
7. una PreferredResponse illegale degrada in modo sicuro;
8. nessuna preselezione consuma risorse;
9. nessun dato privato nuovo viene replicato agli avversari;
10. test/scenari previsti sono registrati nei sistemi di tracking esistenti;
11. non nasce un secondo sistema di Reaction o Time Bank;
12. non nasce una nuova issue se #319/#314 coprono già il deliverable;
13. eventuali nuovi coefficienti restano `PROPOSED FOR PLAYTEST`;
14. la documentazione generata è riallineata alle sorgenti;
15. i gate documentali del repository restano verdi.

---

# 20. Output richiesto a Claude

Produrre un breve referto finale con:

```text
1. Stato di main misurato
2. Documenti owner trovati
3. Issue esistenti consolidate
4. Eventuali gap reali rimasti
5. Decisioni aggiornate
6. File modificati
7. Issue aggiornate/aperte
8. Feature/scenario/editor map aggiornate
9. Test/scenari aggiunti o pianificati
10. Gate eseguiti e risultati
11. Contraddizioni trovate e risolte
12. Cose deliberatamente NON fatte
```

Se emerge che una parte è già stata implementata o decisa dopo questo handoff, **prevale `main`**: integrare solo il delta reale e documentarlo.
