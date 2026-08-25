# Reaction Profile e Reaction Clash — specifica di E14.7

> **Stato**: owner documentale della capability *contested reaction*. Le decisioni di forma sono
> [D-047](../decisions/RT_PDR_00_Decision_Log.md), [D-048](../decisions/RT_PDR_00_Decision_Log.md) e
> [D-049](../decisions/RT_PDR_00_Decision_Log.md), prese dall'autore il **2026-08-09**.
> **Dipende da**: [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) — questa spec ne è un'estensione,
> non un secondo sistema di reazioni.
> 🔴 **~~Nessun runtime~~ — falso dal 2026-08-19, e la riga è durata più del suo presupposto.** Diceva: *«E14
> non parte prima di E13, e questa specifica descrive come funzionerà, non come funziona oggi»*. Il **Reaction
> Profile di §2 gira in partita**: `Action.Brace` costruisce l'opportunity dal profilo dell'unità e apre il
> decision boundary quando la cardinalità lo chiede (`RTTurnManager_Blast.cpp`, ramo `Status.Braced`), con
> `Reactions.Brace.ProfileDecidesInPlay` a pinnarlo. Il **Reaction Clash di §3 non ha ancora runtime**: le sue
> funzioni pure esistono (`IsContested`, `SortParticipantsCanonically`, `CompareGrammarIntents`) e nessun
> punto del resolver le chiama. Lo stato verificabile vive in
> `feature-registry.yaml`, non in questa riga: è la seconda volta che una
> frase di stato in testa a un documento invecchia senza che nessuna riga cambi.

## 1. Perché esiste

ADR-0004 ha reso possibile una **scelta live** durante la resolution, e l'ha dimostrata sul caso a un solo
decisore: l'Overwatch risponde `FIRE` o `HOLD` e nessun altro ha voce. Restava fuori il caso in cui **due**
partecipanti hanno una scelta significativa nello stesso boundary — parare contro fintare, tenere la cella
contro spostarla, far scattare la trappola contro cambiare linea.

Questa spec aggiunge quel caso e **una sola** cosa nuova al modello: la nozione di opportunity **contested**.
Tutto il resto — snapshot per segmento, timeout puro, ordine totale, niente nesting — resta quello di ADR-0004.

## 2. `Brace` è l'accesso al Reaction Profile — [D-047](../decisions/RT_PDR_00_Decision_Log.md)

### 2.1 Il significato

> **`Brace` prepara il personaggio a reagire. *Come* reagisce lo dice il suo Reaction Profile.**

La risposta universale, quella che ogni personaggio possiede, è **`Hold Ground`**: tenere la cella. Un
personaggio con un profilo più ricco converte la stessa preparazione in risposte diverse — parare, scartare
di lato, far scattare un dispositivo — senza che il sistema `Brace` sappia nulla di lui.

### 2.2 Cosa **non** cambia, e perché è importante

`Hold Ground` **è** il comportamento che il gioco ha già, non un rimpiazzo:

| Proprietà di oggi | Dove vive | Dopo D-047 |
|---|---|---|
| −10 su **ogni** danno diretto fino al Cleanup | `RTCombatLibrary.h:112`, `RTTurnManager.cpp:2819` | esito di `Hold Ground` |
| blocca la **prima** spinta, senza limite di distanza | `RTTurnManager.cpp:3033` | esito di `Hold Ground` |
| azione **Principale** di `Prep`, costo 10, priorità 30 | `RTCatalogLibrary.cpp:493` | invariato |
| `Status.Braced` e i suoi consumatori | `RTGameplayTags`, `RTTurnManager` | invariati |
| `Visual.Combat.BraceReducesEveryHit`, `Spec.Facing.BraceHoldsFromBehind` | `Scenarios/` | restano verdi |

Nessun numero di bilanciamento cambia. Cambia la **classificazione**: `Action.Brace` smette di essere
«un'azione che si dichiara e basta» e diventa «un'azione che **arma** un profilo di reazione».

### 2.3 Perché questo non contraddice ADR-0004 §2

ADR-0004 elenca `Brace` fra le reazioni con «una sola risposta legale», che nel modello unificato è il caso
degenere `AllowedResponses ≤ 1` — commit immediato, nessun boundary. La precisazione di D-047:

> La cardinalità di `Brace` **non è 1 per natura**: è 1 **per il profilo base**. Un profilo d'eroe che
> dichiara una seconda risposta legale porta la cardinalità a ≥ 2, e la finestra si apre **da sola**, con la
> regola che ADR-0004 ha già.

Non serve nessuna regola nuova, nessun enum di tipo, nessuna eccezione. `Counter`, `Deflect`, `Shield` e
`Cleanse` restano dove sono: la loro cardinalità è 1 perché non hanno profili alternativi dichiarati.

### 2.4 L'asserzione che deve cadere

La classificazione vecchia non è pinnata da un test dedicato: è **un'asserzione dentro un altro test** —
`TestTrue(TEXT("Brace non e' una reazione: nessun trigger"), … ReactionTrigger == ERTReactionTrigger::None)`
a `RTDefensiveReactionTests.cpp:171`, dentro `RefactorTactics.Reactions.DefensivesMatchCatalog`.

> ⚠️ **Corretto il 2026-08-13.** Fino a oggi questo paragrafo, il [Decision Log](../decisions/RT_PDR_00_Decision_Log.md)
> (D-047), `roadmap-v0.1.md` §5, `feature-registry.yaml` e la issue `#314` nominavano un test
> `Reactions.Brace.IsNotAReaction` a `RTDefensiveReactionTests.cpp:162`. Quel nome **non è mai esistito**:
> `grep -rn "IsNotAReaction" Source/` non trova nulla e `git log -S` nemmeno. La differenza non è di forma —
> «sostituire il test» eseguito alla lettera cancellerebbe anche il pinning di costo, priorità, cooldown,
> slot e macro-fase delle cinque azioni difensive, che vive **nello stesso test**.

D-047 supera quell'asserzione. Va **rimossa dal ciclo di conformità e sostituita**, mai cancellata in
silenzio, dalla coppia:

- `Reactions.Brace.BaseProfileHasSingleResponse` — col solo profilo base la cardinalità resta 1 e **nessun
  boundary si apre**: è la garanzia che nulla di verde rallenta;
- `Reactions.Brace.RicherProfileOpensWindow` — un profilo con due risposte legali apre il boundary senza che
  il resolver conosca l'eroe.

Il resto di `DefensivesMatchCatalog` — costi, priorità, cooldown, slot e macro-fasi delle difensive — **resta
invariato**: non è ciò che D-047 tocca, e un test che perde metà delle sue asserzioni per un cambio di
classificazione è un test che smette di sorvegliare in silenzio.

### 2.5 I profili del roster v0.1 — [D-132](../decisions/RT_PDR_00_Decision_Log.md), contenuto deciso

Un profilo è un'entità di **catalogo**, non un'abilità d'eroe: vive nel namespace `Profile.<Nome>` come
`Action.Brace` e `Reaction.Anchor`, e l'eroe lo **referenzia da un campo dato**. È la forma letterale di
«profilo come dato, non ramo nel resolver», e ha tre conseguenze che un prefisso d'eroe non avrebbe dato:
nessun profilo può collidere con un'abilità, nessun token nuovo nasce con un nome che
[D-120](../decisions/RT_PDR_00_Decision_Log.md) ha declassato a legacy, e un profilo si riassegna fra eroi
senza rename quando il roster cresce.

| Eroe | Profilo | Risposte oltre `Hold Ground` | Cardinalità |
|---|---|---|---|
| **Gadget** | `Profile.Grounding` | `GROUND` | 2 → apre la finestra |
| **Phase** | `Profile.Sidestep` | `SIDESTEP` — hex adiacente legale | 2 → apre la finestra |
| **Wraith** | `Profile.Glance` | `GLANCE LEFT` · `GLANCE RIGHT` | 3 → apre la finestra |
| **Riktor** | — solo profilo base | — | **1 → nessuna finestra** |

**Perché Riktor non ne prende uno.** La proposta gli assegnava `ANCHOR`, «il riferimento anti-displacement del
roster». Ma `Hold Ground` — cioè `Status.Braced` — lo fa **già e con la stessa ampiezza**: il ramo `Braced` del
resolver non controlla `KnockDist`, a differenza di `Guarded`, che regge solo fino a
`GuardResistedPushDistance`. Una seconda risposta che coincide con la prima lascia la cardinalità a **1**,
quindi per §2.3 non apre nulla — e sarebbe stata la terza scrittura della stessa regola, dopo `Reaction.Anchor`
(«la prima spinta o trazione del turno non ti sposta, a qualunque distanza») e `Gadget.Anchor`. Non è un taglio
di contenuto: è che quel contenuto **esisteva già**, due volte.

Il roster conserva così un eroe **senza finestra sul `Brace`**, ed è la baseline con cui confrontare gli altri
tre quando CP 14.6 misura il pacing.

**Quante finestre.** Il `Brace` segue `MaxPromptsPerReaction` (**3**) come ogni altra reazione: nessun cap
dedicato, nessuna eccezione nel modello unificato. Il tetto teorico di una resolution sale, ed è dichiarato —
è la configurazione in cui la misura di CP 14.6 può davvero fallire.

⚠️ **Nessun numero è deciso qui**: resistenza del `Brace`, Charge del `Grounding` e ampiezza della deviazione
restano aperti come li elencava il triage del 2026-08-10. Sono bilanciamento, e si restringono al playtest.

### 2.5-bis Cosa il resolver sa **eseguire** oggi — `D-176` *(2026-08-19, decisione d'autore)*

> ✅ **La voce canonica è nel [Decision Log](../decisions/RT_PDR_00_Decision_Log.md) dal 2026-08-24**
> ([#1243](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1243)): `D-176` **è** quel link, e
> non più un rinvio a vuoto. Questo paragrafo torna a essere ciò che deve essere — il **dettaglio
> operativo** della decisione, non la decisione stessa. Se le due divergono, vince il Decision Log.
> *(Per un mese l'ID è stato riservato e citato senza esistere: la voce doveva scriverla l'integrazione,
> perché il Decision Log era `integration_only` nel write-set di batch, e quel vincolo è caduto solo il
> 2026-08-20 con [D-178](../decisions/RT_PDR_00_Decision_Log.md). ⚠️ Il buco non si sarebbe chiuso da
> solo: il regime post-D-178 dice di **leggere l'ultimo assegnato**, e chi guardava vedeva `D-179`.)*

Una risposta del profilo si esprime con le primitive di §5, e **una risposta senza effetti dichiarati non
viene offerta**. Le due domande sono diverse e vanno tenute separate:

| Domanda | Chi risponde | Oggi |
|---|---|---|
| Cosa il profilo **dichiara** | `URTCatalogLibrary::BraceAllowedResponses` | 2 · 2 · 3, la cardinalità di [D-132] |
| Cosa il resolver sa **eseguire** | `URTCatalogLibrary::BraceExecutableResponses` | **1 · 2 · 1** |

La distanza fra le due colonne non è un difetto da correggere: è il registro delle due voci che questa spec
dichiara aperte due paragrafi più su. `Profile.Grounding` e `Profile.Glance` sono contenuto **deciso** e
restano nel catalogo; le loro risposte non hanno effetti perché «Charge del `Grounding`» e «ampiezza della
deviazione» sono bilanciamento non deciso. Offrirle comunque aprirebbe una finestra su una scelta che il
resolver non sa applicare — un prompt che ferma la resolution per non fare niente, e che nessun test
distinguerebbe da uno che funziona. Chi chiude una delle due voci aggiunge gli effetti al catalogo, e
`Reactions.Brace.DeclaredIsNotYetExecutable` diventa rosso per ricordarglielo.

**`SIDESTEP` si esprime con `SelfReposition`, ampiezza 1.** Non è una primitiva nuova: è quella che
[D-093](../decisions/RT_PDR_00_Decision_Log.md) ha creato perché nessun effetto sapeva spostare *chi
reagisce*, ed è la stessa che `Reaction.EmergencyDash` e `Reaction.HazardEscape` già portano. Il `1` non è un
numero nuovo per la stessa ragione. La scelta è vincolata da `BAS-4`, che dichiara questo profilo «nuovo, si
chiama `Profile.Sidestep` e risponde al **Forced Movement**», nella stessa forma di `Hero.Phase.FlowReaction`
(`Reposition 1`).

**`SIDESTEP` esce dalla LINEA di spinta, non la percorre.**

> 🔴 **Corretto il 2026-08-19, ed era un difetto di design e non di codice.** Questo paragrafo diceva:
> *«`SelfReposition` allontana di una cella dalla linea di chi ha innescato … la scelta vera esiste contro le
> spinte di 2»*, e l'alternativa perpendicolare era dichiarata **scartata**. Una code review ha misurato che
> quella conclusione è falsa in entrambe le metà: il ramo `Status.Braced` blocca la spinta a **qualunque**
> distanza — non guarda `KnockDist` — quindi contro la spinta di 2 `Hold Ground` tiene la cella mentre
> `SIDESTEP` la cede. Con danno identico (§5: lo scarto non riduce nulla), `SIDESTEP` era **strettamente
> dominato**: non esisteva un caso in cui convenisse. Un decision boundary in cui un'opzione è sempre
> peggiore non è una scelta, ed è peggio di nessun boundary — costa un prompt e non compra niente.

La destinazione è una cella **fuori dalla linea**: i vicini del bersaglio, meno i due che stanno sulla linea —
quello verso cui la spinta spinge e quello da cui arriva. Restano quattro candidate, e le due sotto-decisioni
seguono il precedente già in vigore per `Reaction.HazardEscape`
(`URTTerrainLibrary::FindEscapeCell`) invece di aprirne uno secondo:

| Domanda | Risposta | Perché |
|---|---|---|
| Quale delle quattro? | **la cella che si ha davanti**, se è fra le candidate | la scelta è prevedibile guardando il campo, senza conoscere l'ordine interno delle direzioni ([D-104]) |
| E se il facing punta sulla linea? | **ordine canonico** `E, NE, NW, W, SW, SE` | arbitrario per il giocatore ma *dichiarato* e stabile: due situazioni identiche danno lo stesso scarto |
| E se nessuna è praticabile? | **`Hold Ground`** | ⚠️ è l'opposto di `Reaction.EmergencyDash`, che in quel caso **si spreca**: là una reazione si consuma, qui `Hold Ground` non è una risorsa — chi sceglie di scartare non deve finire meno protetto di chi non ha scelto affatto |

Praticabile = raggiungibile sul **grafo** (`GraphNeighbors`: archi e dislivelli contano, non i sei vicini
geometrici) e non occupata. La funzione è `URTReactionLibrary::FindSidestepCell`, pura e fail-closed, pinnata
da `Reactions.Brace.SidestepLeavesThePushLine` — che asserisce la **proprietà** («non sta sulla linea») e non
una cella fissa, per non diventare rosso a ogni riordino delle direzioni.

✅ **Conseguenza misurata**: `Spec.Brace.ProfileChangesResponse` ora **discrimina**. Con lo scarto lungo la
linea le due celle coincidevano e togliere il ramo `Braced` lasciava lo scenario verde; ora la cella dello
scarto `(1,2)` e quella della spinta `(1,3)` sono diverse, e la stessa mutazione fa cadere **due** assertion.

**Se non c'è dove scartare si ripiega su `Hold Ground`**, e qui la regola è l'opposto di
`Reaction.EmergencyDash` («se non c'è dove andare si spreca»): là una reazione si consuma, qui `Hold Ground`
non è una risorsa. Chi sceglie di scartare non deve finire meno protetto di chi non ha scelto affatto.

### 2.5-ter La **scelta sicura** non è la stringa `HOLD`

`Timeout → HOLD` di [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) §3 è una regola sul
*comportamento*, non sul token: il `Brace` chiama la propria scelta sicura `Hold Ground`, e per lui `HOLD` non
è nemmeno una risposta legale. `URTReactionOpportunityLibrary::SafeResponse` la deriva dall'opportunity —
`HOLD` quando è offerta, altrimenti la **prima** risposta — e l'ordine dei due rami è la regola: nell'Overwatch
`HOLD` sta in **coda**, dopo i `FIRE:`, quindi «prendi la prima» farebbe **sparare** una finestra scaduta.

⚠️ La responsabilità si sposta su chi *costruisce* le risposte: un produttore futuro che mettesse in testa una
risposta costosa la renderebbe l'esito di ogni scadenza. È il vincolo che sostituisce la vecchia costante, ed
è pinnato in entrambi i versi da `Reactions.SafeResponsePrefersHoldWhenOffered`.

### 2.6 La Preferred Response non tocca il profilo — [D-157](../decisions/RT_PDR_00_Decision_Log.md)

Dal 2026-08-17 un decisore può dichiarare in planning **quale** delle risposte legali preferisce, e vedersela
preselezionata all'apertura della finestra. La regola sta altrove — è §4.4 di
[`spec-decision-time-bank.md`](spec-decision-time-bank.md), perché nasce dal requisito «il default è
raggiungibile entro la grace» — e qui vale solo per ciò che **non** cambia:

> La preferenza **ordina la presentazione**; il profilo **decide la legalità**. Le `AllowedResponses` di
> §2.5 sono identiche con e senza preferenza dichiarata, e la cardinalità che apre la finestra (§2.3) non si
> muove.

`Profile.Glance` con `GLANCE LEFT` preferito resta a **3** risposte, non a una: chi cambia idea le vede tutte
e tre. Se la preferenza non è più legale al Decision Boundary, decade alla scelta sicura della Decision
Definition — non trascina con sé la cardinalità.

⚠️ È la distinzione da tenere ferma rispetto alla **condizione dichiarata** di
[D-109](../decisions/RT_PDR_00_Decision_Log.md), che vive nello stesso posto (`FRTArmedOverwatch`) ed è nata
per fare l'opposto: quella **riduce** le `AllowedResponses` al trigger, e può portarle a 1 chiudendo la
finestra. Due dichiarazioni di planning sullo stesso oggetto con effetti opposti: se si confondono, un profilo
a tre risposte ne mostra una sola e nessun test se ne accorge.

Nel Clash nulla cambia: il reveal resta a **scadenza fissa** (§7.1) e la preferenza è privata del decisore
fino al commit, come lo è il lock.

## 3. Reaction Clash — [D-048](../decisions/RT_PDR_00_Decision_Log.md)

### 3.1 Definizione

Una Reaction Opportunity è **contested** quando **due** partecipanti hanno, allo stesso boundary, almeno due
risposte legali ciascuno. Entrambi scelgono **in cieco**, il server blocca le scelte, le rivela insieme e
risolve.

```text
segmento di risoluzione
   └─ trigger valutato (funzione pura, come oggi)
        └─ FRTReactionOpportunity
             ├─ 1 partecipante,  AllowedResponses ≤ 1 → commit immediato          (E5 di oggi)
             ├─ 1 partecipante,  AllowedResponses ≥ 2 → finestra single-responder (E14.5, Overwatch)
             └─ 2 partecipanti,  AllowedResponses ≥ 2 ciascuno → CONTESTED        (E14.7)
```

**Contested è derivato, non dichiarato.** Non esiste un campo `Type = Clash`: sarebbe una seconda verità
accanto alla cardinalità, e le due divergerebbero. Il criterio è *quanti partecipanti hanno una scelta vera*.

### 3.2 Il Clash non sostituisce le altre modalità

Un attacco base non diventa un Clash. La regola di ammissibilità è **negativa**, e va verificata al playtest
con le metriche di §9:

> Se una delle due risposte è di fatto obbligata, l'opportunity **non** è contested: si degrada a
> single-responder, a condizione dichiarata in planning (D-012) o a commit immediato.

### 3.3 Niente nesting

Un Clash non apre un secondo boundary interattivo. Vale il §9 di ADR-0004 senza modifiche.

## 4. Grammatica `STAND · READ · SHIFT` — [D-049](../decisions/RT_PDR_00_Decision_Log.md), `PROPOSED FOR PLAYTEST`

### 4.1 Le tre intenzioni

| Intento | Significato | Esempi di maneuver |
|---|---|---|
| **`STAND`** | tengo il piano, la posizione, la linea, lo spazio | `Hold Ground`, `Shield Block`, `Stand & Fire` |
| **`READ`** | anticipo la scelta avversaria | `Parry`, `Counter`, `Intercept`, `Trigger Trap`, `Redirect` |
| **`SHIFT`** | cambio geometria, angolo, timing, vettore | `Dodge`, `Sidestep`, `Pivot Step`, `Break LOS` |

Relazione ciclica: **`READ` > `STAND` > `SHIFT` > `READ`**.

| A \ B | `STAND` | `READ` | `SHIFT` |
|---|---|---|---|
| **`STAND`** | pari | vantaggio B | vantaggio A |
| **`READ`** | vantaggio A | pari | vantaggio B |
| **`SHIFT`** | vantaggio B | vantaggio A | pari |

### 4.2 Perché `STAND` e non `COMMIT`

Nel modello di ADR-0004 «commit» significa già **applicare la risposta scelta** (`opportunity → commit`).
Usare la stessa parola per un'intenzione di grammatica renderebbe scrivibile la frase «il commit del COMMIT».
Il vocabolario si sceglie una volta, prima che tre documenti ne usino tre — è la lezione di `FAC-10` in
[`OPEN_DECISIONS.md`](../OPEN_DECISIONS.md). `HOLD` era l'altro candidato ed è escluso: è già il timeout
dell'Overwatch.

### 4.3 La matrice è universale, i payoff no

> I personaggi cambiano maneuver, costo, trigger e payoff. **Non** cambiano la relazione
> `READ > STAND > SHIFT > READ`.

La grammatica deve restare imparabile con un roster grande: se ogni eroe altera la matrice, la conoscenza
richiesta cresce col quadrato del roster. La differenziazione passa **solo** dai payoff (§5).

Se il playtest dimostra che la matrice unica è troppo rigida, la conseguenza è una **nuova decisione di
sistema**, non un'eccezione per personaggio.

## 5. Win / Tie / Lose non è successo/fallimento

Il confronto produce solo `Vantaggio A · Pari · Vantaggio B`. È la maneuver a dichiarare i tre esiti:

```text
Hold Ground / STAND              Predictive Counter / READ
  Win  : nessun displacement,      Win  : impatto evitato, contrattacco,
         mitigazione piena,               attaccante esposto
         facing preservato          Tie  : mitigazione parziale, nessun contrattacco
  Tie  : nessun displacement        Lose : l'azione originale risolve,
  Lose : nessuna mitigazione               possibile auto-esposizione
```

Questo è ciò che rende diversi i profili a parità di matrice: un tank tollera il `Lose`, un duellante ha
`Win` enorme e `Lose` pesante.

**Vincolo di determinismo.** Gli esiti si esprimono **solo** con le primitive del catalogo effetti già
esistente (`FRTActionEffectSpec` / `ERTActionEffect`): danno, stato, spinta, trazione, risorsa, facing. Mai
una callback. Un `WinOutcome` che richiede una primitiva inesistente è un errore di validazione del ruleset,
non un caso da risolvere nel resolver.

## 6. La direzione è un payload, non grammatica

`Left / Center / Right` **non** sono intenti. La direzione è un parametro della maneuver quando serve:

```text
Maneuver = Dodge        Grammar = SHIFT   Direction = Left
Maneuver = Hold Ground  Grammar = STAND   Facing    = NorthEast
```

Così la stessa grammatica descrive anche ciò che non è spaziale — hacking, dispositivi, trappole.

> **Dipendenze aperte.** Una maneuver che ruota chi reagisce tocca `FAC-5` di
> [`OPEN_DECISIONS.md`](../OPEN_DECISIONS.md) (*una reazione può ruotare l'unità?* — D-020 nomina
> `FacingUsedByOverwatch` come valore **letto, mai scritto**), e una maneuver direzionale di `Brace` tocca
> `FAC-3` ([ADR-0005](../decisions/adr-0005-orientamento.md) §4a dice che `Brace` protegge la persona, non un
> lato, ed è pinnato da `Combat.ShieldWorksFromAnyDirection`). **Nessuna delle due è risolta qui**: `Pivot
> Step` e `Sidestep` restano `PROPOSED` finché non lo sono.

## 7. Reveal a scadenza fissa e privacy

### 7.1 La regola

> La finestra contested dura **sempre** `FastReactionDuration` (3,0 s). Il reveal avviene **alla scadenza**,
> mai all'arrivo del secondo lock.

Un reveal anticipato direbbe a ciascuno *quando* l'altro ha deciso, e la latenza di decisione è una lettura
dell'avversario che il gioco non intende offrire. È l'applicazione diretta di
[ADR-0004 §7-bis](../decisions/adr-0004-finestre-di-reazione.md) (D-021, privacy temporale) al caso in cui i
due sono **dentro la stessa finestra**, dove il buffering della presentazione non basta.

**Costo dichiarato**: ogni Clash costa 3,0 s pieni di resolution, anche quando entrambi lockano subito. È il
prezzo della privacy temporale, ed entra nel budget di §8.

### 7.2 Cosa cambia rispetto ad ADR-0004 §7

§7 dice che l'avversario «non riceve nulla: né l'esistenza della finestra, né la sua durata». In un Clash
entrambi **sono** partecipanti, quindi l'esistenza della finestra è nota a tutti e due per costruzione.
L'emendamento è puntuale:

| | Single-responder | Contested |
|---|---|---|
| Esistenza della finestra | ignota all'avversario | **nota a entrambi i partecipanti** |
| Identità del responder | ignota | nota (sono loro due) |
| Risposte legali dell'altro | — | **mai inviate** |
| Scelta dell'altro | — | **mai inviata prima del reveal** |
| Momento del lock dell'altro | — | **non osservabile** (§7.1) |
| Chi non partecipa | non riceve nulla | non riceve nulla |

### 7.3 Ordine canonico dei lock

Il TurnLog registra due eventi di lock. Il loro ordine **non** è l'ordine di arrivo — sarebbe una dipendenza
dal tempo reale, contro l'invariante #4 — ma l'ordine totale già in vigore
([ADR-0004 §4](../decisions/adr-0004-finestre-di-reazione.md)):
`ReactionPriority → AbilityPriority → UnitInitiative → StableUnitId → ReactionInstanceId`.

## 8. Budget dei prompt — un boundary contested vale **1**

> Un boundary contested consuma **un solo** prompt dal budget, condiviso fra i due partecipanti.

Il caso peggiore di ADR-0004 §8 resta quindi quello dichiarato — `MaxPromptsPerReaction 3 × 3,0 s = 9 s` per
unità armata — e la soglia d'allarme di **20 s** non va rimisurata da capo. Contare 2 prompt avrebbe
raddoppiato il caso peggiore proprio mentre §7.1 rende ogni finestra incomprimibile.

Resta vero che con reveal fisso i 9 s diventano un **minimo garantito** e non un massimo raggiungibile solo
in caso di indecisione: è la misura da fare a CP 14.7, non una stima da scrivere qui.

## 9. Timeout e costi

**Timeout**: ogni partecipante ha un fallback deterministico dichiarato dalla propria maneuver. Mai casuale,
mai «la risposta migliore».

| Ruolo | Fallback |
|---|---|
| Difensore in `Brace` | `Hold Ground` / `STAND` |
| Attaccante | continua l'azione originale |

**Costo**: una maneuver con costo consuma la risorsa **al lock valido**, anche se perde il Clash, salvo
policy diversa dichiarata dalla maneuver. Senza questo, «provo comunque, tanto se perdo non costa nulla»
svuota la scelta. Non si introduce una risorsa nuova: si usano `Charges`, cooldown e stato del dispositivo
che il progetto ha già.

## 10. TurnLog

Eventi da registrare, con i nomi allineati a quelli esistenti quando un equivalente c'è:

```text
ReactionOpportunityCreated      OpportunityId, partecipanti, cardinalità
ReactionChoiceLocked            × 2, in ordine canonico (§7.3)
ReactionClashRevealed           alla scadenza fissa
ReactionClashCompared           intento A, intento B, risultato
ReactionOutcomeResolved         ramo Win/Tie/Lose e primitive applicate
ReactionCostConsumed
```

Il log canonico non dipende da timestamp di presentazione. In rete, nessun evento di scelta viene pubblicato
prima del reveal.

## 11. Modello dati — mappato sui tipi reali

Non si introduce un tipo parallelo. `FRTReactionOpportunity` (ADR-0004 §2) si estende:

| Concetto | Dove va | Nota |
|---|---|---|
| partecipanti | campo di `FRTReactionOpportunity` | oggi implicitamente 1 |
| risposte legali per partecipante | `AllowedResponses`, per partecipante | la cardinalità **deriva** il tipo di finestra |
| `OpportunityId` | derivato, mai GUID | regola già fissata dal CP 14.3 |
| maneuver | dato del profilo d'eroe | `ManeuverId`, `GrammarIntent`, costo, direzione opzionale, tre esiti |
| esiti | `FRTActionEffectSpec[]` | vincolo di §5 |

`GrammarIntent` è l'unico enum nuovo: tre valori, chiuso.

## 12. Scenari

Categoria a **tag** (`clash`, `reactions`), non a categoria primaria: l'enum di categorie primarie è stato
**scartato** in [`scenario-index-e-tag.md`](../technical/tooling/scenario-index-e-tag.md) §8. Il prefisso è il **modo**
(`Spec` / `Visual`), com'è per ogni ScenarioId del repository.

| ScenarioId | Dimostra |
|---|---|
| `Spec.Clash.ReadBeatsStand` · `Spec.Clash.StandBeatsShift` · `Spec.Clash.ShiftBeatsRead` | la matrice, un lato per file |
| `Spec.Clash.TieAppliesOnce` | il pari non apre un secondo round |
| `Spec.Clash.HiddenUntilReveal` | nessuna scelta visibile prima del reveal; ordine canonico stabile |
| `Spec.Clash.RevealIsFixedDeadline` | il reveal non anticipa quando entrambi lockano subito |
| `Spec.Clash.TimeoutFallback` | fallback deterministico per entrambi i ruoli |
| `Spec.Clash.CostConsumedOnLock` | la risorsa se ne va anche perdendo |
| `Spec.Clash.NoNestedWindow` | un Clash non ne apre un altro |
| `Spec.Clash.Determinism` | stesso snapshot + stesse scelte ⇒ stesso `StateHash`, `LogHash`, ordine eventi |
| `Spec.Brace.ProfileChangesResponse` | due eroi, stesso `Brace`, risposte diverse, nessun branch per eroe |

Nascono **`BLOCKED`** e va bene: è la prassi del repository — una feature ha una forma eseguibile *prima* di
essere costruita, come `Spec.Overwatch.HoldThenFire`. Il gate `requires` dichiara la capability mancante
(`DecisionBoundary`, `ReactionClash`).

> ✅ **`Spec.Brace.ProfileChangesResponse` è VERDE dal 2026-08-19** (E14.7 fetta 4): `ReactionProfile` è fra le
> capability disponibili, e il file **esegue ciò che descrive** — un turno solo, con una `decisions` che
> risponde `SIDESTEP` a una finestra che si apre davvero.
> 🔴 **Questa riga diceva «il suo T2 non è più `intents: []`», e quel T2 non esiste**: lo scenario è a **un
> turno**, perché `Action.Brace` dura 1 turno e ha cooldown 1 — al T2 lo stato è scaduto e l'azione non è
> ridichiarabile. La frase descriveva il piano invece del file, ed era ripetuta identica in
> `RTScenarioSession.cpp`: due sedi, una sola misura sbagliata.
> ⚠️ **I quattro `Spec.Clash.*` restano `BLOCKED`** su `ReactionClash`, che è tuttora indisponibile — le sue
> funzioni pure esistono e **nessun punto del resolver le chiama**. 🔴 Diceva «gli **otto**»: sono **quattro**
> su disco (`ReadBeatsStand`, `StandBeatsShift`, `ShiftBeatsRead`, `TieAppliesOnce`) — contati con `ls`, non
> letti dalla tabella di §12, che ne elenca dieci fra scritti e ancora da scrivere.

### 12.1 Il vocabolario di `decisions` — versione **3** del formato scenario

Uno scenario scripta la risposta a un boundary con `decisions[].respond`. Fino alla v2 il vocabolario era
chiuso a `FIRE` / `HOLD`; dalla **v3** accetta anche le risposte di un Reaction Profile:

```jsonc
"decisions": [
  { "unit": "V1", "respond": "FIRE", "target": "R1" },  // target OBBLIGATORIO
  { "unit": "V1", "respond": "HOLD" },                  // e VIETATO con tutto il resto
  { "unit": "R1", "respond": "SIDESTEP" }               // v3: risposta di profilo
]
```

Tre proprietà, ciascuna con la ragione per cui non è un dettaglio:

- **il vocabolario si chiede al catalogo** (`URTCatalogLibrary::AllReactionProfileResponses`), non si
  riscrive nel loader: una seconda lista divergerebbe al primo profilo aggiunto, e a divergere sarebbe il
  *gate* — cioè il pezzo il cui mestiere è accorgersene;
- **il gate di versione fa dire al messaggio la cosa giusta**: senza, una build a `SupportedVersion = 2`
  accetterebbe un file `version: 2` con `respond: "SIDESTEP"` e lo rifiuterebbe con «risposta sconosciuta»,
  accusando il file mentre il difetto è la build;
- **una risposta legale nel catalogo può non esserlo in quella finestra** — `SIDESTEP` chiesto a Riktor, che
  non ha profilo. La session lo verifica con `IsResponseAllowed` e lo dichiara come nota del turno; senza,
  il resolver produrrebbe un `HoldRejected` che nel referto somiglia a un `HOLD` voluto.

## 13. Metriche di playtest

Si riusano i nomi in vigore — `ReactionDecisionSeconds` e `ResolutionPlaybackSeconds` restano **separate**,
perché sommarle è l'errore che la nota di revisione di ADR-0004 esiste per evitare. Si aggiungono solo:

`ClashesPerMatch` · `ClashesPerTurn` · `TimeoutRate` · `ChoiceDistributionStandReadShift` ·
`WinTieLoseDistribution` · `RepeatChoiceRate` per giocatore.

Domande a cui il playtest deve rispondere: il giocatore capisce **perché** ha vinto? La scelta sembra una
lettura o un tiro di dadi? La grammatica è intuitiva dopo due partite? I personaggi sembrano diversi pur
usando la stessa matrice? Il `Tie` è interessante senza un secondo prompt?

## 14. Cosa resta aperto

| | Domanda |
|---|---|
| `CLASH-1` | `STAND/READ/SHIFT` diventa canonica o resta grammatica di playtest? Serve una partita, non un documento |
| `CLASH-2` | Payoff `Win/Tie/Lose` esatti per le maneuver dei quattro eroi — i nomi di §4 sono fixture, non kit approvati |
| `CLASH-3` | Il Clash ha bisogno di un `Charges` proprio o riusa quello della reaction? |
| `CLASH-4` | Durata della presentazione del reveal, che è separata dai 3,0 s logici |
| `CLASH-5` | Policy di disconnessione dentro una finestra contested (M10) |

Vedi anche `FAC-3` e `FAC-5` in [`OPEN_DECISIONS.md`](../OPEN_DECISIONS.md): il Clash **le incontra**, non le
risolve.
