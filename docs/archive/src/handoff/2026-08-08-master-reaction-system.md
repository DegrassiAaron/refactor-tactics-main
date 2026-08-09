> ## 🗄️ `HISTORICAL` — SORGENTE RECEPITO
>
> **Archiviato il 2026-08-10.** Era in `todo/consolidazione-chat-openai/`, untracked. E' la **versione
> rivista** del cluster Reaction, ed era gia' assorbita quando il triage e' iniziato.
>
> **Recepito da** la PR `#305`: `D-047`, `D-048`, `D-049` e il checkpoint CP 14.7. Il kit di dettaglio sul
> Reaction Clash e' entrato nella stessa PR.
> Triage: [`consolidamento-chat-openai-triage-2026-08-09.md`](../../../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md).

# RefactorTactics — Cluster Reaction: consolidamento della v0.1

> ⚠️ **`NON CANONICO`** · **Revisione 2 — riconciliata col repository** · **2026-08-09**
>
> **Cos'è**: la mappa del cluster di chat su Reaction, Brace, Overwatch, Fast Reaction, Fast Action e
> Time Bank, riconciliata con ciò che il repository ha **già deciso e già costruito**.
>
> **Cosa NON è**: non è owner di nessuna regola. Dove il canone ha deciso, questo documento **rimanda** e non
> riscrive; dove ha misurato, riporta la misura e non la stima. Se una riga di qui contraddice un owner,
> **prevale l'owner** e la riga è un difetto da correggere qui.
>
> | Regola | Owner reale |
> |---|---|
> | Modello finestre, invariante #3 composto, parametri, privacy | [`adr-0004-finestre-di-reazione.md`](../../../decisions/adr-0004-finestre-di-reazione.md) |
> | Reaction Profile, opportunity *contested*, grammatica del confronto | [`spec-reaction-clash-e14.md`](../../../gameplay/spec-reaction-clash-e14.md) |
> | Reazioni preparate (quelle che girano oggi) | [`spec-reazioni-componibili-cp55.md`](../../../gameplay/spec-reazioni-componibili-cp55.md) |
> | **Decision Time Bank** | `gameplay/spec-decision-time-bank.md` — `CURRENT`, v0.1, CP 14.8 |
> | Overwatch: economia dell'azione, profili | [`brief-overwatch-reazioni.md`](../../../gameplay/brief-overwatch-reazioni.md) · [`brief-azioni-generiche-overwatch.md`](../../../gameplay/brief-azioni-generiche-overwatch.md) |
> | Ordine delle fasi | [`spec-sequenza-turno.md`](../../../gameplay/spec-sequenza-turno.md) |
> | Famiglie di movimento e trigger spaziali | [`spec-tassonomia-movimento.md`](../../../gameplay/spec-tassonomia-movimento.md) |
> | **Stato verificabile** di ogni feature | [`feature-registry.yaml`](../../../roadmap/feature-registry.yaml) |
>
> **Revisione 1 → 2**: vedi il [Changelog](#changelog) in fondo. Nove correzioni, tre delle quali erano
> conflitti col canone che avrebbero cambiato il gioco.
>
> ⚠️ **Disambiguazione degli ID**, doppia:
>
> - il Decision Log contiene **due** serie `D-041 · D-042 · D-043` (vedi
>   [Appendice A](#appendice-a--difetti-trovati-nel-repository)). Qui ogni citazione porta il soggetto fra
>   parentesi: `D-041 (Brace)`, non `D-041`;
> - le sigle **`D16`–`D22`** sono locali a [`brief-overwatch-reazioni.md`](../../../gameplay/brief-overwatch-reazioni.md)
>   e **collidono** con `D-016`–`D-022` del Decision Log, che sono decisioni diverse — il brief stesso lo
>   avverte. Qui le sigle del brief sono sempre scritte senza trattino e qualificate: `D20` del brief.

---

## 0. Legenda

| Marca | Significato |
|---|---|
| ✅ | **Canone**: già deciso altrove. Qui c'è il rimando, non la regola |
| 🟢 | **Costruito**: gira in partita oggi, con test |
| ➕ | **Delta**: contributo genuino di questo cluster di chat, non ancora nel canone |
| ⚠️ | **Correzione** di quanto diceva la revisione 1 di questo documento |
| 🔓 | **Aperto**: registrato in [`OPEN_DECISIONS.md`](../../../OPEN_DECISIONS.md) o gated su un checkpoint |

Il documento ha **zero** delta e **una** proposta di lavoro sull'harness (§17). Tutto il resto è mappatura. È
il risultato atteso: il cluster di chat ha esplorato un'area che era già stata decisa fra il 2026-08-07 e il
2026-08-09.

> ⚠️ **Il Time Bank non è più un delta** — ed è cambiato **due volte** mentre questa revisione veniva scritta.
> Alla prima stesura era l'unico contributo non ancora nel canone e la raccomandazione era «registrarlo come
> proposta gated». Nel giro di poche ore è arrivato
> `gameplay/spec-decision-time-bank.md` — `PROPOSED`, con gate — e
> subito dopo l'autore ha deciso: **v0.1, CP 14.8, senza gate**, spec `CURRENT`. La §14 è stata riallineata
> due volte.
>
> Vale come avvertenza sul genere: un consolidamento dichiara «nuovo» ciò che nessuno ha ancora scritto **nel
> momento in cui guarda**, e quella frase può invecchiare in ore. Prima di usarne una qualsiasi, verifica lo
> stato nell'owner — mai qui.

---

## 1. Il modello, in una pagina — ✅

```
Reaction armata
  └─ trigger valutato                    ← funzione PURA sullo snapshot
       └─ FRTReactionOpportunity { AllowedResponses[] }
            ├─ AllowedResponses ≤ 1  → commit immediato, NESSUN boundary   ← le reazioni di oggi
            ├─ AllowedResponses ≥ 2  → decision boundary + finestra 3,0 s  ← Overwatch
            └─ due partecipanti con ≥ 2 ciascuno → opportunity CONTESTED   ← Reaction Clash
```

Il turno non è più una risoluzione unica: è una **sequenza di segmenti**, ognuno col proprio snapshot.

```
Turno = [ snapshot → risolvi → boundary ] · [ snapshot → risolvi → boundary ] · … · Cleanup
        └───────  «raccogli poi applica»  ──┘   ripetuto, non derogato
```

L'invariante #3 **si compone, non si deroga**: il resolver non attende mai *dentro* un segmento — termina il
segmento e restituisce il controllo. Owner: ADR-0004 §1–§2.

> **Il tipo esiste già come decisione.** `FRTReactionOpportunity` non è «un concetto da introdurre»: è deciso
> da ADR-0004 §2 e ha il suo checkpoint (**CP 14.3**). Manca il codice, non la definizione.

---

## 2. Tassonomia — ✅ `D-019`

`Fast Action` e `Fast Reaction` sono **categorie semantiche distinte sulla stessa infrastruttura**
`DecisionWindow`, con baseline di finestra comune **3,0 s**:

| Categoria | Causa | Esempio |
|---|---|---|
| **Fast Reaction** | evento **esterno** durante la Resolution | Overwatch che vede entrare un bersaglio |
| **Fast Action** | continuazione **esplicita di una propria azione** già in risoluzione | nessuno in v0.1 — vedi sotto |

⚠️ **Correzione**. La revisione 1 diceva due cose che il canone contraddice:

1. **`DASH LEFT / DASH RIGHT` dopo una skill non è un esempio valido.** `D-019` è esplicita: *«la v0.1 non
   inventa una Fast Action concreta finché nessuna ability reale la richiede»*. Inventarla qui creerebbe un
   consumatore che nessun kit ha chiesto. `RT-FEAT-REACTION-FAST-ACTION` è `DESIGNED` proprio per questo: il
   brief la nomina ma non le dedica regole, e il registro dice «da promuovere solo con un owner esplicito».

2. **Counter, Deflect, Shield, Cleanse, Guard, Brace non sono Fast Reaction.** Sono reazioni con **una sola
   risposta legale**: il caso degenere `AllowedResponses ≤ 1`, che committa senza aprire nessuna finestra.
   Metterle nella stessa lista di Overwatch cancella la distinzione che ADR-0004 §2 esiste per fare — ed è la
   distinzione che tiene la suite di E5 verde senza cambiare comportamento atteso.

   `Guard` in particolare è un'**azione generica universale** (`D-025`, sette voci) con tre consumatori già
   in partita — riduzione danno, resistenza alla spinta, decadimento fuori dall'arco frontale
   ([ADR-0005](../../../decisions/adr-0005-orientamento.md) §4a). Non è una finestra.

**Chi apre una finestra, in v0.1**: `Overwatch` sempre; `Brace` **solo** con un profilo d'eroe che dichiari
una seconda risposta (§6); un Clash quando i partecipanti con scelta sono due (§7). Nient'altro.

---

## 3. Reaction Opportunity — ✅ ADR-0004 §2, CP 14.3

Il DTO che il server invia contiene **solo il presente**:

| Contiene | Non contiene |
|---|---|
| `OpportunityId` — **derivato**, mai un GUID | trigger futuri |
| `ReactionInstanceId` | posizioni o percorsi futuri |
| owner autorizzato / **partecipanti** | intenti avversari |
| target ed eventi consentiti | quante opportunity arriveranno dopo |
| `AllowedResponses` — **la cardinalità deriva il tipo di finestra** | il momento in cui l'altro ha lockato (§9) |
| micro-step / boundary corrente, deadline | |

⚠️ **Due campi mancavano nella revisione 1**, ed erano i due che rendono verificabile il resto:

- **`OpportunityId` è derivato deterministicamente** — `Turn.Phase.MicroStep.Owner.ReactionDef.Seq`, **mai un
  GUID runtime** (CP 14.3, test `Reactions.OpportunityIdIsDerivedNotRandom`). Senza questa regola, la
  verifica «`stale OpportunityId` → reject» di §17 non è implementabile: un ID casuale non si ricalcola al
  replay, quindi non c'è niente contro cui confrontare quello ricevuto.
- **partecipanti**, oggi implicitamente 1. È il campo su cui poggia il *contested* di §7, senza introdurre un
  tipo parallelo (`spec-reaction-clash-e14.md` §11).

**Nessun campo `Type`.** Il tipo di finestra si legge dalla cardinalità delle risposte legali. Un enum di
tipo sarebbe una seconda verità accanto alla cardinalità — è il **rischio (b)** dichiarato di E14.

---

## 4. Overwatch v0.1 — ✅ CP 14.4 / 14.5 / 14.6

### 4.1 Cosa si prepara in Planning

| Elemento | Regola | Fonte |
|---|---|---|
| Zona controllata | **riusa `FRTSuppressiveZone`** — nessuna seconda geometria | CP 14.4 |
| Direzione della zona | ⚠️ **nasce dal facing dell'unità**, non da un parametro dichiarato a parte | CP 14.4 · ADR-0005 §4c |
| Range, trigger, risposte legali | **dato per eroe** (profilo), non un ramo nel resolver | `Overwatch.ProfileIsDataNotBranch` |
| Condizione dichiarata | valutata al trigger come funzione pura, **riduce** le risposte legali; se ne resta una, commit immediato | `D-012` |

⚠️ **Correzione**. La revisione 1 elencava *sia* «area/cone direzionale» *sia* «facing/direzione» fra ciò che
il giocatore prepara. Sono **due sorgenti per lo stesso dato**, ed è esattamente ciò che ADR-0005 §4c vieta:
«due sorgenti sarebbero due verità». La direzione del cono è il facing, punto.

### 4.2 Il costo — ⚠️ mancava del tutto

Armare l'Overwatch **consuma lo slot dell'azione offensiva**: `Attack` **oppure** `Ability` **oppure**
`Overwatch`, mai sommati, salvo eccezione dichiarata dall'abilità (`D-012` · `D-025`, chiude `OD-3`).
Test: `Overwatch.CompetesWithOffensiveAction`.

Una revisione che descrive l'Overwatch senza il suo costo descrive un'abilità gratuita.

### 4.3 Il trigger

```
TargetInsideArea  ∧  HasLineOfSight  ∧  TargetDetected  ∧  ReactionStillArmed
```

⚠️ `TargetDetected` **non** è «detection/visibility quando applicabile». Per il profilo Overwatch **visivo** è
il livello **`Rilevato`** dei tre di E13:

| Situazione | Livello | Trigger |
|---|---|---|
| Bersaglio nel cono, a vista | `Rilevato` | ✅ |
| Nel fumo entro 2 celle (cap `Max_Contact_Range`) | `Rilevato` | ✅ |
| Nel fumo oltre 2 celle · solo rumore | `Incerto` | ❌ |
| Fuori vista, ultimo contatto noto | `UltimoContatto` | ❌ |

Un profilo che dichiara un **canale diverso** (acustico) dichiara anche la propria soglia, e può essere
legittimo a `Incerto`: il rumore è un secondo canale percettivo, non una vista degradata. Ciò che resta
vietato a tutti è sparare a una posizione **dedotta** senza contatto (`Resonance Shot`, north-star).

> 🔗 **Dipendenza dichiarata, non nota a margine: E14 non parte prima di E13.** Senza livelli di conoscenza
> `TargetDetected` non ha definizione, e l'Overwatch sparerebbe a unità che la squadra non percepisce.

Il controllo avviene a **ogni micro-step reale** di movimento (CP 14.2 rende il resolver step-able a
comportamento invariato).

### 4.4 La finestra — ✅ ADR-0004 §8

| Parametro | Valore | Nota |
|---|---|---|
| `FastReactionDuration` | **3,0 s** | ⚠️ baseline di sistema per **ogni** Fast Reaction, non solo per l'Overwatch |
| Opzioni | `FIRE` / `HOLD` | |
| `DefaultTimeoutBehavior` | **`HOLD`** | funzione pura dello stato. Mai `FIRE`: un mancato input non spende una risorsa irreversibile |
| `Charges` (Overwatch v0.1) | **1** | |
| `MaxPromptsPerReaction` | **3**, data-driven | limita le opportunity di **una** reaction |
| Cap **aggregato** per turno | **nessuno** | `D20` del brief — scelta esplicita, rischio dichiarato, si misura al playtest |

> ⚠️ Le ultime due righe **non sono in contraddizione** e ADR-0004 §8 lo dice a chiare lettere. Confonderle è
> il motivo per cui la revisione 1 credeva che il Time Bank (§14) fosse già coperto.

`HOLD` perde **solo l'opportunità corrente** e mantiene l'Overwatch armato (`Overwatch.HoldKeepsArmed`).
`FIRE` consuma la charge, committa la reaction e **tronca il movimento residuo** del bersaglio
(`Overwatch.FireTruncatesFutureMovement`). Il giocatore non deve sapere se arriveranno altre opportunity.

### 4.5 Trigger simultanei — ✅ ADR-0004 §4

Più bersagli nello stesso micro-step ⇒ **una sola** opportunity multi-bersaglio (`FIRE A` / `FIRE B` / `HOLD`),
**mai** prompt in sequenza: prompt sequenziali darebbero un vantaggio all'ordine di iterazione, che
l'invariante #4 vieta. Test: `Overwatch.SimultaneousTargetsSingleOpportunity`.

### 4.6 Counterplay — CP 14.6

KO, Stun, Disarm e Forced Movement **invalidano** l'overwatch armato: non è garantito fino a fine turno.
Test: `Overwatch.CancelledByStun`, `Overwatch.CancelledByForcedMovement`.

---

## 5. Reazioni preparate — 🟢 esiste e gira

`RT-FEAT-REACTION-PREPARED` è **INTEGRATED**: una attivazione per turno, reazioni componibili, identità nel
TurnLog, nessun branch per eroe nel resolver (`Reactions.NoHeroSpecificBranchInResolver`).

⚠️ La revisione 1 non la nominava. Era l'omissione più costosa del documento: un elenco di nove feature in cui
non compare l'unica che ships oggi si legge come «niente è costruito».

Nel modello unificato queste reazioni **sono** il caso `AllowedResponses ≤ 1`. La suite di E5 resta verde
senza cambiare comportamento atteso — è il criterio di accettazione di CP 14.3, non un auspicio.

---

## 6. Brace e Reaction Profile — ⚠️ **correzione sostanziale** · `D-041 (Brace)`

> **`Brace` prepara il personaggio a reagire. *Come* reagisce lo dice il suo Reaction Profile.**

```
Brace → Character Reaction Profile → Trigger → Opportunity → Response
```

### 6.1 La cardinalità del profilo base è **1**

⚠️ La revisione 1 dichiarava `opportunity: BRACE / HOLD` come baseline. **È il conflitto più grave del
documento** e va corretto così:

| | Revisione 1 (errata) | Canone `D-041 (Brace)` |
|---|---|---|
| Risposte del profilo **base** | `BRACE` / `HOLD` → **2** | `Hold Ground` → **1** |
| Conseguenza | boundary da 3,0 s a ogni displacement contro un'unità braced | **commit immediato, nessun boundary** |

`Hold Ground` **coincide con il comportamento che il gioco ha già**: −10 su ogni danno diretto fino al
Cleanup, blocco della prima spinta. Nessun numero di bilanciamento si muove, `Status.Braced` e i suoi
consumatori restano, gli scenari verdi restano verdi.

Un profilo d'eroe che dichiara una **seconda** risposta legale porta la cardinalità a ≥ 2 e apre il boundary
**con la regola che ADR-0004 ha già**: nessuna regola nuova, nessun enum, nessuna eccezione.

> **Perché non è pedanteria.** Con `BRACE/HOLD` di default, ogni push su ogni unità braced diventa un prompt.
> Il caso peggiore di ADR-0004 §8 — `3 × 3,0 s = 9 s` per **una** unità armata, contro una soglia d'allarme di
> **20 s** — andrebbe rifatto da capo, e la misura di CP 14.6 nascerebbe già invalidata. La garanzia da
> preservare è testuale: *col profilo base nessun boundary si apre, quindi nulla di verde rallenta*.

### 6.2 Cosa resta baseline, cosa resta aperto

| | |
|---|---|
| ✅ costo Main Action, armamento in `Prep`, 1 charge, durata del round | invariati dal catalogo |
| ✅ trigger base: Forced Movement / displacement | |
| ✅ non riduce automaticamente danno o Stun, non è una difesa universale | |
| 🔓 effetto numerico anti-displacement | `+1 displacement resistance` è **baseline di test**, non regola. `OPEN_DECISIONS.md` |
| 🔓 profili concreti dei 4 eroi | aperti per dichiarazione di `D-041 (Brace)` |

Possibili risposte di profilo, tutte da playtest: tenere la posizione · schivare · deviare · attivare un
dispositivo · counter difensivo · risposta signature.

### 6.3 Il test che deve cadere

`RefactorTactics.Reactions.Brace.IsNotAReaction` (`RTDefensiveReactionTests.cpp:162`) asserisce oggi
`ReactionTrigger == None` e pinna la classificazione **vecchia**. Va **sostituito**, mai cancellato in
silenzio, da:

- `Reactions.Brace.BaseProfileHasSingleResponse`
- `Reactions.Brace.RicherProfileOpensWindow`

Finché la sostituzione non è fatta, il gate `automation` della feature resta `partial` **anche se la suite è
verde**: è verde sulla classificazione sbagliata.

---

## 7. Reaction Clash — ✅ `D-042 (Clash)` · `D-043 (grammatica)` · CP 14.7

### 7.1 Contested è **derivato**, non dichiarato — ⚠️ mancava

Un'opportunity è **contested** quando **due** partecipanti hanno ciascuno **≥ 2 risposte legali** allo stesso
boundary. **Nessun campo `Type = Clash`**: sarebbe una seconda verità accanto alla cardinalità.

La revisione 1 descriveva la procedura in sette passi senza mai dire *cosa rende contestata* un'opportunity —
e un lettore che implementa quella procedura aggiunge naturalmente un enum di tipo, cioè il rischio (b).

### 7.2 La procedura — ✅ invariata dalla revisione 1

boundary → partecipanti autorizzati → piccolo set di risposte → raccolta **in cieco** → chiusura →
matrice deterministica → TurnLog.

### 7.3 La grammatica esiste già come candidata — ⚠️ mancava

`STAND · READ · SHIFT`, con **`READ > STAND > SHIFT > READ`** (`D-043 (grammatica)`).

Stato: **`PROPOSED FOR PLAYTEST`, *non* Consolidata** — l'unica delle tre decisioni del 2026-08-09 a non
essere canonica, e lo è deliberatamente: una relazione ciclica si valuta giocandola.

| Vincolo | Regola |
|---|---|
| La matrice è universale, i **payoff** no | una matrice per eroe farebbe crescere col quadrato del roster la conoscenza richiesta |
| Gli esiti si esprimono **solo** con `FRTActionEffectSpec` | mai callback |
| La direzione è un **payload** della maneuver | non un intento di grammatica |
| `STAND` e non `COMMIT` | «commit» significa già *applicare la risposta scelta*, e `HOLD` è già il timeout dell'Overwatch |
| `GrammarIntent` | l'unico enum nuovo: tre valori, chiuso |
| Niente nesting | un Clash non ne apre un altro (`Clash.NoNestedWindow`) |

⚠️ La revisione 1 elencava la grammatica come «OPEN DESIGN» **vuoto**, perdendo una decisione presa e quattro
file scenario già scritti (§16). Aperto e *inesplorato* non sono la stessa cosa.

### 7.4 Budget

Un boundary contested vale **un solo prompt**, condiviso fra i due partecipanti. Il caso peggiore di
ADR-0004 §8 **non cambia** e la soglia di 20 s non si rimisura da capo.

---

## 8. Determinismo — ✅ ADR-0004 §3–§4 · invariato dalla revisione 1

Mai dipendere da: Tick · frame rate · animazioni · ordine implicito di `TMap`/`TSet` · arrivo dei pacchetti ·
timing client.

Ordine totale e stabile per reaction concorrenti nello stesso micro-step:

```
ReactionPriority → AbilityPriority → UnitInitiative → StableUnitId → ReactionInstanceId
```

Tre proprietà che ne discendono, e che sono le uniche a rendere il replay possibile:

- la decisione del giocatore entra nel **TurnLog come dato** (`OpportunityId → Response`): il replay non
  reinterroga nessuno;
- il **timeout è una funzione pura** dello stato;
- l'esito dipende **solo da *quale*** risposta arriva, **mai da *quando*** arriva dentro la finestra. La
  slow-motion è presentazione: non tocca esiti, seed, ordine, collisioni, path né timing logico.

> Questa sezione è canone dal 2026-08-07. La revisione 1 la riportava correttamente **e nella sua §17 la
> elencava come decisione aperta**. Vedi §18.

---

## 9. Privacy — ✅ ADR-0004 §7 · **§7-bis** · emendamento contested

### 9.1 Il flusso

```
Server:  CanonicalIntentStore → Snapshot → Resolver → Current Boundary → Sanitized Reaction Opportunity
Client:  Opportunity → UI → Response
Server:  Validate → Commit/Hold → TurnLog → Resume
```

**Decide** solo il proprietario della reaction. **Vede** l'intera squadra, in sola lettura. L'avversario non
riceve nulla: né l'esistenza della finestra, né la durata, né l'esito prima che sia applicato.

### 9.2 Privacy **temporale** — ⚠️ mancava, ed è metà del requisito

La §9.1 copre il **payload**. Non basta, ed è la lacuna che `D-021` è stata scritta per chiudere: la
sospensione è **globale**, e una pausa osservabile *è* l'informazione — dice all'avversario che una finestra
si è aperta, in questo istante, su quel micro-step. Una pausa **variabile** è un canale laterale: la durata
del silenzio è correlata alla decisione altrui.

| Livello | Regola |
|---|---|
| **Logico** (server) | la progressione **può** sospendersi al boundary: serve al determinismo. Invariato |
| **DTO avversario** | nessun trigger, opportunity, `AllowedResponses`, identità del responder, timeout, né metadati da cui dedurre la finestra |
| **Presentazione avversaria** | **nessuna pausa variabile correlata alla scelta privata**: buffering, pacing o *fixed resolution beat* |
| **Autorità** | timeout e risposta sono **server-authoritative**: un client lento non allunga la finestra, uno che non risponde ottiene `HOLD` |

Estende l'invariante #6: «zero leak» comprende ora **anche il tempo**. La vecchia formulazione «payload
visibile solo alla propria squadra» non copriva il caso, perché il canale non è il pacchetto — è la sua assenza.

### 9.3 L'eccezione contested

In un Clash entrambi *sono* responder: l'esistenza della finestra è nota a tutti e due **per costruzione**, e
non è più deducibile — è dichiarata. Restano non inviate: le risposte legali dell'altro, la sua scelta prima
del reveal, e **il momento in cui ha lockato**.

Quest'ultimo è il punto in cui il buffering non basta, perché i due sono *dentro la stessa finestra*. Lo
chiude il **reveal a scadenza fissa**: la finestra dura sempre `FastReactionDuration` e non anticipa quando
entrambi lockano subito. Costo dichiarato: ogni finestra contested spende **3,0 s pieni**, e i `3 × 3,0 s`
diventano un **minimo garantito** invece di un massimo raggiunto solo per indecisione — misura di CP 14.7,
non stima.

---

## 10. Movimento e trigger — ✅ `spec-tassonomia-movimento.md`

Quattro famiglie, non cinque: `Move` · `Dash` · `Forced` · `Teleport`. Il **Reaction Movement non è una quinta
meccanica**: è una delle quattro con una **causa** diversa, e ciò che deve restare distinguibile nel TurnLog è
la causa, non il modo di muoversi.

Per Move, Dash e Forced Movement il sistema valuta gli **attraversamenti reali**: chi viene spinto attraverso
due celle di fuoco *le ha attraversate*, anche senza spendere un punto movimento.

### Teleport — ⚠️ la regola c'è, lo scenario no

`Teleport` **non esiste in v0.1**: nessuna azione del catalogo lo produce, e la riga «stato nel codice» della
matrice dice **assente**. La semantica dei trigger è però già scritta come regola preventiva — *trigger
spaziali: solo all'arrivo; non attraversa le celle intermedie* — quindi §10 della revisione 1 duplicava un
owner invece di aggiungere qualcosa.

Conseguenza operativa: lo scenario «Teleport Trigger Semantics» della revisione 1 **non è scrivibile** — non
esiste un'azione che produca la fixture. Rimosso da §16, sostituito dal rimando alla riga di matrice.

---

## 11. Stato runtime — ✅ struttura confermata

Una Reaction Instance contiene: `ReactionInstanceId` · `OwnerUnitId` · `ReactionDefinitionId` ·
`SourceIntentId` · `ArmedAtStep` · `RemainingCharges` · `PromptCount` · `HoldCount` · `ExpirationBoundary` ·
`StablePriority` · `CurrentState`.

Stati: `Inactive` · `Armed` · `OpportunityPending` · `Committed` · `Resolved` · `Expired` · `Cancelled`.

> Nessun tipo parallelo: la maneuver di un Clash è **dato del profilo d'eroe** (`ManeuverId`, `GrammarIntent`,
> costo, direzione opzionale, tre esiti), non una struttura runtime a parte.

---

## 12. Pipeline — ⚠️ **riscritta**: era un secondo ordine di turno

Le macro-fasi sono **invariate** e il Move resta **dopo** il Blast:

```
Planning → Prep → Dash → Blast → Move → Cleanup
```

⚠️ La revisione 1 elencava una pipeline in nove punti con `Movement micro-step` al terzo e
`Attacks / Abilities` al quinto — cioè il movimento **prima** degli attacchi, senza nominare nessuna
macro-fase. Contraddiceva ADR-0003 §1, ADR-0004 §9 **e la sezione precedente dello stesso documento**.

La forma corretta è una **micro-pipeline interna a un segmento**, che vale dentro qualunque macro-fase
comporti movimento (`Dash`, `Move`, o un displacement risolto in `Blast`):

```
per ogni micro-step del segmento:
   1. transition                       5. valutazione dei trigger di reazione
   2. environment                      6. costruzione delle opportunity
   3. occupancy                        7. finestre Fast Reaction   ← qui, e solo qui, il segmento termina
   4. visibility / LOS                 8. applicazione delle reaction committate
```

Il resto — controllo/difese, attacchi, propagazione ambientale, KO/obiettivi, cooldown/cleanup, TurnLog —
**non è pipeline di reazione**: è l'ordine delle macro-fasi, e il suo owner è `spec-sequenza-turno.md`.
Duplicarlo qui è ciò che ha prodotto la contraddizione.

---

## 13. UI — ✅

Una Fast Reaction è minimale: trigger leggibile · target validi · `FIRE`/`HOLD` · countdown · Time Bank
residuo **se e quando** introdotto (§14).

Non mostra: futuro della Resolution · opportunità successive · informazioni private avversarie.

**Nessuna logica di gioco nel widget** (CP 14.6). La presentazione può rallentare; la simulazione resta ferma
su un boundary logico, e la slow-motion non cambia l'esito (`Overwatch.SlowMotionDoesNotChangeOutcome`).

---

## 14. Time Bank — 🔄 **lavoro in volo, non nel repository**

> ⚠️ **Stato osservato il 2026-08-09, e cambiato tre volte in giornata.** La spec
> `gameplay/spec-decision-time-bank.md` — `CURRENT`, v0.1, **CP 14.8**, con audit di provenienza in
> `roadmap/plans/decision-time-bank-conflict-report-2026-08-09.md` — è **esistita nel working tree e poi è
> stata ritirata**: non è su `main`, non è su nessun branch locale, e la riga che la indicizzava in
> `OPEN_DECISIONS.md` è stata rimossa insieme a lei. Il triage del pacchetto
> (`roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md`) la classifica **🔄 in volo, non
> committata**, insieme ad `adr-0007`.
>
> **Conseguenza per chi legge**: nel repository di oggi il Decision Time Bank **non esiste** — né come
> feature, né come CP 14.8 in roadmap, né come voce aperta. Quanto segue descrive il design che era stato
> scritto, e vale come raccordo **se e quando** quel lavoro atterra. Non citarlo come canone.

Qui resta solo il **raccordo col cluster**; il design vive nella sua spec e non si duplica.

### 14.1 Entra senza gate, e il rapporto con `D20` è dichiarato

> Il Time Bank entra in **v0.1** come **CP 14.8**, **senza gate** *(decisione dell'autore, 2026-08-09)*.

La misura di CP 14.5/14.6 **non** è una condizione di ammissione: è un **input di calibrazione**, il primo dato
con cui si tarano `InitialBank` e `Grace`.

Il rapporto con `D20` del brief — *nessun cap aggregato, si misura al playtest* — è quello che la spec §2.1
dichiara apertamente: il bank **è** un cap aggregato, in tempo anziché in prompt, e costruirlo prima della
misura sostituisce quella decisione con una nuova, presa senza il dato che l'avrebbe informata. Il rischio è
**accettato e scritto**, non nascosto — nella stessa forma in cui `D20` dichiarava il suo. I due rientri di
ADR-0004 §Revisione (*cap per turno* · `MaxPromptsPerReaction = 1`) restano disponibili e **compatibili** col
bank: sono parametri, si possono attivare insieme.

**CP 14.8 non precede CP 14.5/14.6**: la prima misura arriva comunque prima della taratura.

Le **regole** sono decise; i **valori** (`InitialBank` derivato da `RoundLimit`, `Grace` 1,0 s,
`ExhaustedGrace` 0,75 s) restano `PROPOSED FOR PLAYTEST` con i criteri di promozione di §3.2, e non vanno
pubblicati come definitivi — Wiki compresa.

> Con la correzione di §6.1 — profilo base di `Brace` a cardinalità 1 — il costo che il bank tara è **più
> basso** di quanto la revisione 1 lasciasse credere. La calibrazione va fatta su questo modello, non su quello.

### 14.2 Cosa questo documento aggiunge alla spec

Due raccordi, entrambi già coerenti con essa:

1. **Il bank non aiuta nel Clash, per costruzione.** Il reveal è a scadenza fissa (§9.3): la finestra contested
   costa `FastReactionDuration` piena anche se entrambi lockano subito. La spec lo dichiara in §1.1 e lo
   registra come rischio `TB-R7`. È il prezzo della privacy temporale, non un difetto da correggere.
2. **Il bank residuo è owner-only.** La revisione 1 metteva «UX quando restano pochi secondi» fra le domande di
   interfaccia. Non è una domanda di UX: per §9.2 il tempo è un canale, e un bank osservabile dall'avversario è
   un leak. La spec la chiude come decisione a sé, **indipendente dalla taratura** — vale comunque.

Resta vero, e la spec lo conferma: per Overwatch **il timeout è `HOLD`** anche a bank esaurito.

> ⚠️ Il valore «circa 30 secondi per giocatore» della revisione 1 non ha fonte ed è superato dai parametri
> della spec §3.2, che dichiarano i propri criteri di promozione. Non usarlo.

### 14.3 Le domande aperte non stanno qui

`TB-1`…`TB-7` vivono in `gameplay/spec-decision-time-bank.md` §17.
[`OPEN_DECISIONS.md`](../../../OPEN_DECISIONS.md) le **indicizza** con la domanda che le governa tutte —
*serve un Decision Time Bank?* — e non le duplica.

---

## 15. Feature Registry — ⚠️ mappatura, non schema nuovo

Lo stato di una feature vive **solo** in `feature-registry.yaml`, `status` è derivato dai gate e il validator
lo verifica. La revisione 1 proponeva nove ID `Feature.Reaction.*` che non esistono lì. Mappatura corretta:

| Feature ID canonico | Titolo | Status | Epic / CP |
|---|---|---|---|
| `RT-FEAT-REACTION-PREPARED` | Reazioni preparate in planning | 🟢 **INTEGRATED** | E5 · 5.1–5.5 |
| `RT-FEAT-CORE-DECISION-BOUNDARY` | Risoluzione segmentata | SPECIFIED | E14 |
| `RT-FEAT-REACTION-OPPORTUNITY` | Modello Opportunity → Commit | SPECIFIED | E14 · 14.3 |
| `RT-FEAT-REACTION-FAST` | Fast Reaction con finestra limitata | SPECIFIED | E14 · 14.5, 14.6 |
| `RT-FEAT-REACTION-OVERWATCH` | Overwatch universale profilabile | SPECIFIED | E14 · 14.4 |
| `RT-FEAT-REACTION-PROFILE` | Reaction Profile armato da Brace | **IMPLEMENTING** | E14 · 14.7 |
| `RT-FEAT-REACTION-CLASH` | Reaction Clash (opportunity contested) | SPECIFIED | E14 · 14.7 |
| `RT-FEAT-REACTION-FAST-ACTION` | Fast Action come continuazione | DESIGNED | E14 · 14.6 |
| *(da registrare)* | **Decision Time Bank** | v0.1, **CP 14.8** — owner `gameplay/spec-decision-time-bank.md` §14 |

Due voci della revisione 1 non avevano corrispondente e non devono averne: `Feature.ReactionSystem` è
un'**area**, non una feature; `Feature.Reaction.DecisionWindow` è `RT-FEAT-CORE-DECISION-BOUNDARY`, che vive
sotto Core perché non serve solo alle reazioni.

---

## 16. Scenari — ⚠️ ID canonici, e cinque esistono già

Gli ScenarioId del repository sono `Modo.Area.Nome`; la categoria è un **tag**, non un enum primario. Lo
schema `REACT-001…011` era un terzo formato e avrebbe prodotto duplicati.

| ScenarioId | Dimostra | Stato su disco |
|---|---|---|
| `Spec.Overwatch.HoldThenFire` | primo entra → `HOLD`, secondo entra → `FIRE` | ✅ esiste · `BLOCKED` su `["DecisionBoundary", "Facing"]` |
| `Spec.Clash.ReadBeatsStand` · `StandBeatsShift` · `ShiftBeatsRead` | la matrice, un lato per file | ✅ esistono · `BLOCKED` su `["DecisionBoundary", "ReactionClash"]` |
| `Spec.Clash.TieAppliesOnce` | il pari non apre un secondo round | ✅ esiste · `BLOCKED` |
| `Spec.Brace.ProfileChangesResponse` | due eroi, stesso `Brace`, risposte diverse, nessun branch | ✅ esiste |
| `Spec.Overwatch.Fire` | nemico entra nel cono, `FIRE`, charge consumata | ➕ da scrivere |
| `Spec.Overwatch.SimultaneousTargets` | due nemici, stesso micro-step, **una** opportunity | ➕ da scrivere |
| `Spec.Overwatch.TimeoutIsHold` | nessuna risposta → `HOLD`, reaction ancora armata | ➕ da scrivere |
| `Spec.Overwatch.CancelledBeforeOpportunity` | KO/Stun prima dell'opportunity → nessun prompt | ➕ da scrivere |
| `Spec.Brace.ForcedMovement` | unità con profilo ricco spinta, risposta non base | ➕ da scrivere |
| `Spec.Clash.HiddenUntilReveal` · `RevealIsFixedDeadline` · `Determinism` · `TimeoutFallback` · `CostConsumedOnLock` · `NoNestedWindow` | privacy, reveal, replay | 🔒 **pianificati, non scrivibili oggi** — vedi §17 |
| ~~Teleport Trigger Semantics~~ | — | ❌ **rimosso**: nessuna azione produce Teleport in v0.1 (§10) |
| ~~Privacy Canary~~ | — | 🔒 spostato in §17: serve un'assertion che oggi non esiste |

Nascere **`BLOCKED`** è la prassi del repository, non un debito: una feature ha una forma eseguibile *prima* di
essere costruita, e il gate `requires` dichiara la capability mancante.

---

## 17. Verifiche — ⚠️ tre classi, non un elenco unico

La revisione 1 elencava undici verifiche come se fossero tutte scrivibili oggi. **Non lo sono**, e la ragione
è misurata: `ERTAssertionKind` ha cinque assertion — `UnitAtCell` · `TurnsCompleted` · `UnitHpEquals` ·
`UnitAlive` · `UnitFacing` — e leggono tutte lo **stato finale**. Nessuna legge il TurnLog, l'ordine degli
eventi o un hash.

### A — test automatici, scrivibili quando la capability atterra

`Reactions.SingleResponseCommitsWithoutWindow` · `Reactions.OpportunityIdIsDerivedNotRandom` ·
`Reactions.DeclaredConditionCollapsesToImmediateCommit` · `Reactions.UnknownConditionIsRejectedByValidator` ·
`Reactions.Brace.BaseProfileHasSingleResponse` · `Reactions.Brace.RicherProfileOpensWindow` ·
`Overwatch.TimeoutIsHold` · `Overwatch.HoldKeepsArmed` · `Overwatch.SimultaneousTargetsSingleOpportunity` ·
`Overwatch.RequiresDetection` · `Overwatch.OrderIsDeterministic` · `Overwatch.CompetesWithOffensiveAction` ·
`Overwatch.ProfileIsDataNotBranch` · `Overwatch.OpportunityLeaksNoFuture` · `Overwatch.CancelledByStun` ·
`Overwatch.CancelledByForcedMovement` · `Clash.ContestedIsDerivedNotDeclared` · `Clash.TieAppliesOnce` ·
`Clash.CostConsumedOnLock` · `Clash.NoNestedWindow`

Più: risposta non autorizzata → reject · `stale OpportunityId` → reject · `FIRE` consuma **una sola** charge ·
`HOLD` non consuma charge se la definition lo dichiara.

### B — 🔒 bloccati sull'harness: **serve prima un'assertion nuova**

| Verifica | Assertion mancante |
|---|---|
| stesso snapshot + stesse risposte → stesso TurnLog | lettura di `LogHash` / `StateHash` |
| permutazione dell'ordine unità → stesso risultato | confronto di hash fra due run |
| nessun canary leak verso il client avversario | ispezione del DTO per squadra |
| `Clash.HiddenUntilReveal` · `RevealIsFixedDeadline` · `Determinism` | ordine degli eventi nel TurnLog |

➕ **Proposta di lavoro**: estendere `ERTAssertionKind` con assertion su TurnLog, ordine eventi e hash. È un
prerequisito dichiarato di CP 14.7, non un'attività opzionale, e va tracciato come tale prima di promettere i
sei scenari `Spec.Clash.*` pianificati.

### C — verifiche umane, non automatizzabili

| Verifica | Perché è umana |
|---|---|
| playback 30 / 60 / 144 FPS → stesso stato logico | richiede un'esecuzione interattiva reale, non headless |
| `PIE-V01-OVERWATCH` — countdown, leggibilità, slow-motion | l'oracolo è un occhio |
| ritmo della resolution osservata **con e senza** finestra aperta (§9.2) | il canale temporale si giudica guardando |

Vanno nel registro `test-manuali-pie.md`, non nella lista dei test automatici.

---

## 18. Decisioni: triage — ⚠️ quattro voci su dieci non erano aperte

| # rev. 1 | Voce | Verdetto |
|---|---|---|
| 1–3 | Time Bank: valore, scope, refill | ⚠️ **il *se* è deciso** (CP 14.8, senza gate); resta aperta la **taratura**, che vive nella spec §17 come `TB-*` |
| 4 | Effetto numerico base di Brace | 🔓 aperta, **già registrata** in `OPEN_DECISIONS.md` |
| 5 | Brace + facing | ⚠️ già registrata come **`FAC-3`**, con risposta di default: ADR-0005 §4a dice che `Deflect`, `Brace` e `Shield` proteggono la **persona**, non un lato. Cambiarlo è un emendamento a §4a, non una decisione libera — e il test `Combat.ShieldWorksFromAnyDirection` esiste per impedire che accada per deriva |
| 6 | Brace + Move normale | ⚠️ già registrata come **`FAC-4`/`FAC-5`** — «la lacuna più urgente»: quale facing legge il trigger *durante* i micro-step di un Move |
| 7 | Grammatica Reaction Clash | 🔓 aperta, ma **con un candidato**: `STAND · READ · SHIFT`, `D-043 (grammatica)`, `PROPOSED FOR PLAYTEST`, 4 scenari su disco (§7.3) |
| 8 | Priorità fra reaction concorrenti | ❌ **DECISA** — ADR-0004 §4. La §8 di questo stesso documento la riportava già: era una contraddizione interna |
| 9 | Cap globale di decision window per turno | ❌ **DECISA** — `D20` del brief: nessun cap, rischio dichiarato, soglia 20 s da misurare |
| 10 | Reaction annidate | ❌ **DECISA** — ADR-0004 §9, «cosa **non** cambia»; `spec-reaction-clash-e14.md` §3.3; test `Clash.NoNestedWindow`. Non è «l'MVP propone» |

Restano aperte **tre** questioni proprie di questo cluster (`TB-1…TB-4`), due già di proprietà del canone
(`FAC-3`, `FAC-4/5`) e una in playtest (grammatica). Non dieci.

---

## 19. Lavoro: non epic nuove, checkpoint esistenti — ⚠️

Le cinque epic proposte dalla revisione 1 esistono già come checkpoint di **E14**, che è in roadmap:

| CP | Contenuto | Stato |
|---|---|---|
| **14.1** | ADR-0004 — composizione dell'invariante #3 | ✅ **chiuso** 2026-08-07 |
| **14.2** | micro-step step-able, comportamento invariato | pianificato |
| **14.3** | modello unificato senza regressioni · `D-012` condizione dichiarata | pianificato |
| **14.4** | Overwatch armato e trigger a micro-step · costo dell'azione offensiva | pianificato |
| **14.5** | finestra, commit e primo consumatore · prima misura di pacing | pianificato |
| **14.6** | counterplay, UI e **misura reale** della resolution | pianificato |
| **14.7** | Reaction Profile e Reaction Clash | pianificato |
| **14.8** | Decision Time Bank | deciso, **senza gate**; 14.5/14.6 lo **tarano**, non lo ammettono |

➕ Il solo lavoro **non** coperto da un checkpoint: le assertion dell'harness di §17-B.

> **E14 è la prima epic da tagliare** se lo scope si accorcia, e `RT-FEAT-REACTION-CLASH` è `P3` dentro di
> essa: esce per prima, e la baseline a un solo decisore resta intera. Vale la pena saperlo prima di
> proporre cinque epic nuove.

---

## 20. Exit criteria — ⚠️ resi verificabili

La revisione 1 chiedeva «Wiki aggiornata» e «Roadmap collega feature → epic → issue»: giudizi, non gate. Il
repository ha gate derivati e un validator, quindi i criteri si esprimono con quelli.

Il cluster è consolidato quando:

1. le otto feature di §15 hanno `owner_specs` che puntano a documenti esistenti, e il validator passa;
2. il gate `spec` di `RT-FEAT-REACTION-OPPORTUNITY`, `-FAST`, `-OVERWATCH`, `-PROFILE`, `-CLASH` è `done`
   (lo è già) e nessuno di questi documenti lo contraddice;
3. gli scenari di §16 marcati ✅ sono su disco con un `requires` che nomina la capability mancante;
4. §17-B è tracciata come prerequisito, non promessa come test;
5. il Decision Time Bank è indicizzato in `OPEN_DECISIONS.md` con il suo gate, e le `TB-*` restano nella sua
   spec senza duplicati;
6. nessuna definizione di *Interrupt* incompatibile sopravvive: lo stack LIFO interattivo e gli interrupt
   annidati **restano scartati** (ADR-0004 §9);
7. ogni sezione di questo documento porta un rimando a un owner, o è marcata ➕.

---

## 21. Cleanup delle chat

⚠️ Prima di eliminare qualcosa, il criterio va reso misurabile: **una chat si archivia quando il suo
contenuto è citato da un owner documentale**, verificabile con una ricerca, non quando sembra recepito.

| Chat | Azione | Condizione |
|---|---|---|
| Focus su Overwatch | archivio | §4 di qui + `brief-overwatch-reazioni.md` |
| Focus su Brace Skill | archivio | §6 di qui + `spec-reaction-clash-e14.md` §2 |
| Time Bank in giochi | archivio | recepita da `gameplay/spec-decision-time-bank.md`, con audit di provenienza |
| Reaction System Overview | sostituita da questo documento | — |
| Skill Move / movement semantics · Action Ghosts / UI · Character reaction profiles · Scenario/QA tooling | collega, non fondere | hanno owner propri |

---

## Appendice A — difetti trovati nel repository

Emersi durante la riconciliazione. **Non sono difetti di questo documento** e non li ho corretti: toccano file
canonici e la loro correzione è una decisione tua.

### A.1 🔴 Collisione di ID nel Decision Log

`RT_PDR_00_Decision_Log.md` contiene, in **un'unica tabella**, due serie con gli stessi identificatori:

| ID | Occorrenza 1 | Occorrenza 2 |
|---|---|---|
| `D-039` | azioni ambientali con owner nel roster | `E21` è *Presentazione*; roster a 8 → `E35` |
| `D-041` | soglia d'udito come statistica per eroe | **`Brace` arma un Reaction Profile** |
| `D-042` | acqua bassa `+2` al rumore | **Reaction Opportunity *contested*** |
| `D-043` | arco frontale e `TeamKnowledge` per squadra | **grammatica `STAND · READ · SHIFT`** |

È il pattern dei contatori condivisi fra sessioni parallele: gli ID vanno assegnati **al merge**, e qui la
rinumerazione non è avvenuta. Ogni link `D-041` è oggi ambiguo, inclusi quelli dentro ADR-0004, il feature
registry e `spec-reaction-clash-e14.md`.

Il log stesso documenta il precedente e la procedura: `D-040` «nata come `D-039` e rinumerata al merge».

### A.2 🟡 `RT-FEAT-REACTION-PROFILE` e `-CLASH` condividono CP 14.7

Non è un errore, ma vale una nota: se E14 si accorcia, `-CLASH` (P3) esce e `-PROFILE` (P2) resta. Il
checkpoint condiviso non lo dice, e la separazione è oggi solo nella priorità delle feature.

---

## Appendice B — gli altri consolidamenti che toccano questo cluster

Verificato il 2026-08-09. **Due sono già stati recepiti**; tre contengono conflitti dello stesso tipo corretto
qui, e non sono stati riconciliati.

### B.1 Già recepiti — non serve altro

| Kit d'autore | Recepito in | Nota |
|---|---|---|
| `RefactorTactics_ReactionSystem_ReactionClash_…_2026-08-09.md` | [`spec-reaction-clash-e14.md`](../../../gameplay/spec-reaction-clash-e14.md) | PR #305, owner di `D-041`–`D-043` |
| `RefactorTactics_Decision_Time_Bank_…_2026-08-09.md` | `gameplay/spec-decision-time-bank.md` | con audit di provenienza dedicato |

### B.2 Da riconciliare — conflitti puntuali, già localizzati

| Documento | Conflitto | Canone |
|---|---|---|
| `RT_Common_Actions_…` §7 | `Planning → Brace armed → Trigger → Opportunity → Response / HOLD`: dà per scontata la finestra su **ogni** Brace | è il **C1** corretto in §6.1 di qui: profilo base = `Hold Ground`, cardinalità 1, nessun boundary |
| `RT_Common_Actions_…` §8 | tratta **`Activate`** come azione universale, con cinque esempi | `D-014` e `D-025`: `Activate` è **assorbita da `Interact`**, e le generiche sono **sette**. Un'azione che non esiste |
| `RT_Scenarios_QA_Bots_…` §11 | ScenarioId `Reaction.Overwatch.HoldThenFire` | il prefisso è il **modo** (`Spec` / `Visual`), non la categoria: `Spec.Overwatch.HoldThenFire`, che esiste già su disco. La nota «non rinominare scenari reali per uniformità estetica» è corretta in generale e **fuori luogo qui**: non è estetica, è lo schema di identificazione |
| `RT_UI_UX_…` §9 | elenca il Time Bank fra i pannelli HUD da progettare | ha un owner e un **gate**: la UI non si progetta prima che il gate passi (§14.1) |

Nessuno dei tre è stato modificato: la riconciliazione di ciascuno è un lavoro della stessa dimensione di
questo, e va fatta un documento per volta.

---

## Changelog

**Revisione 1 → 2** (2026-08-09). Nove correzioni, tre delle quali cambiavano il gioco.

| # | Sezione rev. 1 | Correzione |
|---|---|---|
| 🔴 1 | §5 Brace | `BRACE/HOLD` come baseline → **`Hold Ground`, risposta unica, nessun boundary**. Contraddiceva `D-041 (Brace)` e avrebbe aperto una finestra a ogni displacement |
| 🔴 2 | §14 Feature Registry | nove ID `Feature.Reaction.*` paralleli → mappatura sugli otto `RT-FEAT-*` canonici; aggiunta `RT-FEAT-REACTION-PREPARED`, l'unica INTEGRATED, che mancava |
| 🔴 3 | §12 Pipeline | movimento prima degli attacchi, senza macro-fasi → micro-pipeline **interna al segmento**; l'ordine delle fasi torna al suo owner |
| 🟡 4 | §7 Time Bank | proposta come soluzione → rimando all'owner `gameplay/spec-decision-time-bank.md` (`CURRENT`, v0.1, CP 14.8, **senza gate**: 14.5/14.6 tarano, non ammettono); il valore «30 s» ritirato perché senza fonte |
| 🟡 5 | §4 Overwatch | rimossa la doppia sorgente della direzione (facing **e** cono); aggiunto il costo `D-012`; `TargetDetected` = `Rilevato`; dipendenza E13 dichiarata |
| 🟡 6 | §15 Scenari | `REACT-001…011` → ScenarioId canonici; marcati i **5 già su disco**; rimosso il Teleport, non scrivibile in v0.1 |
| 🟡 7 | §16 Test | elenco unico → tre classi; quattro verifiche spostate in «bloccate sull'harness» con l'assertion mancante nominata |
| 🟡 8 | §17 Decisioni | 10 voci → **4 chiuse con rimando**, 2 già registrate come `FAC-*`, 1 con candidato in playtest, 3 aperte davvero |
| 🟡 9 | §3 · §6 · §10 | aggiunti `OpportunityId` derivato, campo partecipanti, privacy temporale §7-bis, «contested è derivato non dichiarato» |
