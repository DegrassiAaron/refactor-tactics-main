# Brief — Azioni generiche e Overwatch universale

> ✅ **Non è più una proposta: [D-014](../decisions/RT_PDR_00_Decision_Log.md) e
> [D-015](../decisions/RT_PDR_00_Decision_Log.md) (2026-08-08) hanno reso canonica la tassonomia di questo
> brief.** Vale:
>
> ```text
> Generiche:  Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch
> Profili:    MoveProfile = Sneak | Normal | Sprint
> Speciali:   Dash / Charge / Leap / Blink / Reposition / displacement forzato   (pre-Blast)
>
> Activate  -> assorbita da Interact
> Guard     -> fondamentale universale (D-025 emenda D-014 su questo solo punto)
> Sprint    != Dash
> Attack | Ability | Overwatch   (mai sommati, salvo eccezione dichiarata)
> ```
>
> 🔁 **Allineato a [D-025](../decisions/RT_PDR_00_Decision_Log.md) il 2026-08-08 — solo su `Guard`.** D-014
> l'aveva declassata a stance specifica, ma `Guard` aveva già **tre** consumatori: il catalogo azioni (−15 e
> resistenza a 1 cella di spinta), l'interazione con `Status.Root`, e la difesa direzionale di
> [ADR-0005](../decisions/adr-0005-orientamento.md) §4a. L'elenco canonico è di **sette** voci.
> L'economia `Attack | Ability | Overwatch` **non cambia**.
>
> ⚠️ **La tassonomia è chiusa, la migrazione no.** `Action.Activate` e `Action.Sprint`
> **esistono e sono consumati** — misurato: `Action.Sprint` in 9 file di codice e 6 di test. Cancellarli o
> rinominarli qui romperebbe test e replay: la migrazione è tracciata come issue, con Stable ID/replay safety
> e validator fra i requisiti. `Action.Guard` **non fa più parte di questa migrazione**: dopo D-025 è di nuovo
> uno Stable ID canonico, non un residuo.
>
> Restano **tunable**, non bloccanti: costi MP, rumore, exposure, eventuale costo extra di slot dello Sprint,
> differenze di profilo per eroe. Vivono nei cataloghi, e **non vanno inventati** in un consolidamento
> documentale.

> **Stato**: brief di requisiti · **Data**: 2026-08-07 · **Origine**: `/sc:brainstorm` su
> [`../src/RefactorTactics_AzioniGeneriche_Overwatch_Universale_v0.1.md`](../src/RefactorTactics_AzioniGeneriche_Overwatch_Universale_v0.1.md) (41 §)
> **Decisione abilitante**: [`D-012`](../decisions/RT_PDR_00_Decision_Log.md) — l'Overwatch **compete** con
> l'azione offensiva; le tre policy entrano nel **DoD di CP 14.3**.
> **Autorità**: [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) prevale su finestre, timeout e trigger.
> Questo brief copre ciò che ADR-0004 **non** copre: l'universalità, i profili e il costo.

## 1. Il confine con E14

E14 costruisce **la finestra**: `opportunity → commit`, 3 s, `Timeout → HOLD`, trigger simultanei aggregati.
Tutto già deciso e non si riapre ([`brief-overwatch-reazioni.md`](brief-overwatch-reazioni.md), D16–D22).

Quello che nessun documento del repo copriva:

| Tema | Stato prima | Ora |
|---|---|---|
| L'Overwatch è di **tutti** o di Vektor? | implicito: skill di Vektor | **di tutti**, come azione di Planning |
| Che cosa **costa** andare in Overwatch? | mai scritto | **D-012**: compete con l'azione offensiva |
| Tutte le opportunity aprono una finestra? | implicito: sì | **no**: tre policy, solo `FastSelect` ferma la resolution |
| `Sneak`/`Move`/`Sprint`: tre azioni o tre profili? | assenti | **profili** della stessa azione Move |

## 2. La grammatica comune

Sette azioni disponibili a tutti come **concetto**; l'implementazione dipende da eroe, equipaggiamento, profilo e
stato (sorgente §3, con `Guard` reintegrata da [D-025](../decisions/RT_PDR_00_Decision_Log.md)):

```text
Wait · Move · Basic Attack · Guard · Brace · Interact · Overwatch
```

Non sostituiscono l'identità dei kit: sono il linguaggio che il resolver riusa. `Interact` in particolare è già
il punto di contatto previsto con porte, ponti, console e obiettivi — cioè con **E9** ed **E10**, dove
`Action.Activate`/`Action.Interact` esistono già come identità a catalogo senza nulla da attivare.

## 3. `Move` come famiglia, non come tre abilità

```text
Sneak   distanza ridotta   · rumore molto basso  · esposizione bassa
Move    distanza standard  · rumore standard     · riferimento di bilanciamento
Sprint  distanza alta      · rumore alto         · esposizione alta alle reazioni
```

Valori indicativi del sorgente (§10), **da playtestare**: distanza 2/3/5, rumore 0-1/2/5. Il personaggio li
modifica dal proprio profilo.

> **Aggancio già esistente**: `FRTActionDef::MovementStyle` c'è ed è usato (`LinearDash`, scatti). I profili di
> movimento sono un'estensione di quel campo, **non** un secondo sistema — e il rumore che li distingue è
> esattamente il canale acustico di **E13** ([`brief-conoscenza-parziale.md`](brief-conoscenza-parziale.md) §12).
> Senza E13, `Sneak` e `Sprint` differiscono solo per distanza: metà del loro senso.

## 4. Overwatch universale — profilo per eroe

> **Overwatch è universale come postura, framework e comando di Planning. Trigger, area, risposte legali ed
> effetto dipendono dal profilo.**

È la regola che evita il problema opposto: una regola comune che rende tutti gli eroi uguali.

| Archetipo | Trigger tipico | Risposte legali |
|---|---|---|
| Marksman | movimento nemico, arco stretto, lungo raggio | `FIRE` / `HOLD` |
| Tank | nemico si avvicina a un alleato, arco largo, corto raggio | `INTERCEPT` / `HOLD` |
| Controller | nemico attraversa un bordo sorvegliato | `PUSH LEFT` / `PUSH RIGHT` / `HOLD` |
| Assassin | nemico entra in prossimità | `AMBUSH` / `HOLD` |
| Engineer | nemico interagisce con un dispositivo | `HACK` / `HOLD` |

Campi concettuali del profilo (§13, da adattare alle convenzioni reali): area e forma · range · ampiezza
dell'arco · trigger ammessi · bersagli ammessi · requisiti di detection e LOS · charge · `MaxPrompts` ·
risposte legali · policy · **fasi sorvegliate** · priorità · timeout.

> **`AllowedResponses` è già il ponte**: ADR-0004 lo usa per distinguere il commit immediato (`≤ 1`) dalla
> finestra (`≥ 2`). Le risposte legali del profilo **sono** quel campo — non serve un secondo modello.

## 5. Le tre policy — regimi, **non** un enum

⚠️ Il documento sorgente (§19, §32) propone `enum ERTOverwatchResolutionPolicy { Automatic, Conditional,
FastSelect }`. **Non si costruisce così**, e la ragione è già registrata in
[`../roadmap/roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) §E14, rischio (b): un enum di policy affiancato ad
`AllowedResponses` sarebbe una **seconda verità** sullo stesso comportamento.

I tre regimi **emergono dai dati** che ADR-0004 ha già:

| Regime | Come si ottiene | Ferma la resolution? |
|---|---|---|
| `Automatic` | `AllowedResponses ≤ 1` — il caso degenere di ADR-0004 §2 | **no** |
| `Conditional` | `AllowedResponses ≥ 2` **+ condizione dichiarata in planning**: valutata al trigger, riduce le risposte legali; se ne resta una, commit immediato | **no** |
| `FastSelect` | `AllowedResponses ≥ 2`, nessuna condizione dichiarata → boundary + finestra 3 s | **sì** |

L'unica cosa **nuova** rispetto ad ADR-0004 è quindi la **condizione dichiarata**: un predicato scelto in
planning, valutato dal resolver come funzione pura sullo stato al boundary. Le condizioni ammesse devono essere
poche, leggibili e validate dal ruleset (sorgente §19.2) — altrimenti la finestra di reazione rientra dalla
porta di servizio come mini-linguaggio di scripting.

Questa è la **mitigazione** del rischio già registrato in
[`brief-overwatch-reazioni.md`](brief-overwatch-reazioni.md) §5: *«`MaxPromptsPerReaction = 3` × 3 s = 9 secondi
per una sola unità armata; con due o tre unità la resolution triplica in modo non prevedibile»*.

Con l'Overwatch universale il rischio **non è più teorico**: se otto unità possono armarsi, la resolution è
governabile solo se la maggior parte delle opportunity si risolve senza fermarla.

Ed è la ragione per cui la policy sta nel **DoD di CP 14.3** e non in CP 14.6: costruire la finestra assumendo
un solo consumatore (`Vektor.InterceptShot`) e poi renderla universale significa scoprire il problema **dopo**
aver costruito la soluzione sbagliata. La misura di CP 14.5 misura allora qualcosa di governabile, non un
sistema da rifare.

## 6. Il costo — `D-012`

```text
Attack   OPPURE   Ability   OPPURE   Overwatch
```

mai `Attack + Overwatch`, salvo eccezione dichiarata da un'abilità.

Senza costo-opportunità l'Overwatch diventa la scelta di chi è indeciso (sorgente §35.1: *«non so cosa fare →
Overwatch»*), e il gioco premia l'attesa invece della lettura. Con il costo, la frase che il giocatore si dice
diventa quella giusta:

> «Rinuncio a colpire ora perché penso che passerai di lì.»

Se nessun trigger avviene, l'investimento è perso. È il rischio che rende la scommessa una scommessa.

**Baseline**: `Charges = 1` · `FastReactionDuration = 3,0 s` · `Timeout → HOLD` · `HOLD` **non** consuma la
charge · `MaxPrompts` data-driven · trigger simultanei aggregati in **una** opportunity · nessun interrupt
annidato.

## 7. Rischi

| Rischio | P/I | Mitigazione |
|---|---|---|
| **Omogeneizzazione del roster**: tutti sembrano avere la stessa quinta abilità | M/**H** | Regola comune, effetto specifico: area, trigger, risposte, fasi e requisiti di detection differenziano (§4). Il gate è che due eroi non abbiano lo stesso profilo |
| La resolution si frammenta comunque | M/**H** | §5. Se la misura di CP 14.5 supera stabilmente i 20 s, i rientri sono già scritti: cap aggregato per turno, oppure `MaxPromptsPerReaction = 1` |
| L'Overwatch universale allarga E14 oltre il budget | **H**/M | Nessuna epic nuova: l'universalità è **dati** (profili), la policy è un campo. Se il tempo stringe, la via di degrado è un solo profilo (`Marksman`) e le altre risposte rinviate |
| `Sneak`/`Sprint` senza rumore sono solo «più corto» e «più lungo» | M/M | Dipendenza esplicita da **E13**: senza il canale acustico i profili di movimento non entrano |

## 8. Domande aperte — da playtestare, non da decidere a tavolino

Effetto esatto di `Brace` · numeri definitivi di `Sneak`/`Move`/`Sprint` · valore di `MaxPrompts` · quali
condizioni sono ammesse in `Conditional` · quante Overwatch contemporanee per squadra · quali profili per i
quattro eroi della v0.1 · comportamento a fine turno dopo soli `HOLD` · quali reazioni possono interrompere
Dash, Attack o Interact.

## 9. Rapporto con gli altri documenti

| Documento | Relazione |
|---|---|
| [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) | **Prevale**: finestra, timeout, privacy, trigger, `AllowedResponses` |
| [`brief-overwatch-reazioni.md`](brief-overwatch-reazioni.md) | Owner di E14 e dei suoi checkpoint; questo brief ne estende il **DoD di CP 14.3** con le policy |
| [`brief-conoscenza-parziale.md`](brief-conoscenza-parziale.md) | E13: `TargetDetected` (D22) e il rumore che dà senso ai profili di movimento |
| [`brief-unita-ausiliarie.md`](brief-unita-ausiliarie.md) | Una torretta è un consumatore di `Automatic`, non un sistema parallelo |
| [`../balance/RT_ActionCatalog_v0.1.md`](../balance/RT_ActionCatalog_v0.1.md) | Owner dei numeri: i valori di §3 vi atterrano quando il tema entra |
