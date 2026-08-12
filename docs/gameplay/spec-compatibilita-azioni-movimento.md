# Spec — Compatibilità fra azioni e movimento: la proposta, in forma decidibile

> `PROPOSED` · **Non è canone.** Esiste per rendere **`AE-2`** decidibile con un modello concreto davanti
> invece che in astratto: senza, la domanda resta *«ti piacerebbe che il movimento contasse?»*, a cui si
> risponde di sì e non si conclude niente.
>
> **Owner della domanda**: [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md) `AE-2` · **issue**:
> [#606](https://github.com/DegrassiAaron/refactor-tactics-main/issues/606) · **feature**:
> `RT-FEAT-ACTION-MOVEMENT-COMPAT` · **epic**: E38 (v0.2).
> **Regola in vigore**: [`spec-economia-del-turno.md`](spec-economia-del-turno.md) — oggi il profilo di
> movimento cambia **distanza, rumore ed esposizione** e nient'altro.
>
> Se `AE-2` viene decisa **no**, questa pagina diventa `HISTORICAL` e resta come referto: costa meno di
> riscoprire fra sei mesi perché era stata scartata.

## 1. Il problema, che non è nuovo

[D-028](../decisions/RT_PDR_00_Decision_Log.md) ha tolto allo `Sprint` il costo dello slot principale e ha
dichiarato l'allarme nello stesso momento:

> *«Senza il costo di slot, il prezzo dello Sprint regge tutto sui dati (`Exposed`, niente reazione) — se non
> basta diventa un `Move` migliore, che è l'upgrade puro vietato da [D-015](../decisions/RT_PDR_00_Decision_Log.md).»*

Oggi le leve per correggerlo sono tutte **numeriche**: alzare il danno che `Exposed` fa incassare, abbassare
gli 8 MP. Nessuna cambia la *natura* della scelta. La compatibilità col movimento è una leva
**strutturale**: lo Sprint smette di essere «un Move più lungo» perché cambia *cosa puoi fare avendolo
scelto*.

## 2. 🔴 Ciò che il kit dava per scontato, e che il modello del turno smentisce

Il kit del 2026-08-12 giustifica la regola con l'immagine ovvia: *non puoi mirare mentre corri*.

**Sul modello del turno di RefactorTactics quell'immagine non descrive niente.** Le macro-fasi risolvono in
ordine — `Prep → Dash → Blast → Move` — e `URTCatalogLibrary::MapResolutionPhase` lo fa valere:

| Piano | Quando parte il colpo | Quando avviene il movimento |
|---|---|---|
| `Move` normale + attacco | **`Blast`** — prima | **`Move`** — dopo |
| `Sprint` + attacco | **`Blast`** — dopo | **`Dash`** — prima ([D-068](../decisions/RT_PDR_00_Decision_Log.md)) |

In **nessuno dei due casi** l'unità spara mentre si muove: o spara da ferma e poi parte, o arriva e poi spara.

Ne segue che la regola, se si adotta, **non si giustifica con la fisica del gesto** ma con l'**impegno del
turno**: chi ha speso il proprio turno a coprire distanza non ha avuto modo di preparare il colpo. È una
differenza che conta in tre posti — il testo che il giocatore legge, il nome del dato, e i casi limite
(un'unità *bloccata* a metà percorso ha comunque «speso il turno»? sì, e la penalità resta).

> ⚠️ Se questa giustificazione non convince, `AE-2` va decisa **no**. Un modello adottato con la motivazione
> sbagliata produce casi limite che nessuno sa risolvere, perché non c'è un principio a cui appellarsi.

## 3. Il modello: una soglia, non una matrice

### 3.1 Perché non la matrice per azione

Il kit propone che **ogni abilità dichiari il proprio comportamento sotto ciascun profilo**. Misurato sul
catalogo attuale:

```text
azioni core che occupano la principale     14
azioni d'eroe non-reazione                 17
                                        ─────
totale                                     31   × 4 profili = 124 celle
```

124 valori da compilare, bilanciare e mantenere — e ognuno è un numero che nessuno ha misurato. È l'ordine di
grandezza che ha fatto dichiarare «costo esplicito» agli **otto** numeri di [ADR-0008](../decisions/adr-0008-rotazione-e-policy-di-facing.md).

### 3.2 La forma che il repository usa già per questo problema

`PushResistance` è una **soglia e non una sottrazione** ([D-038](../decisions/RT_PDR_00_Decision_Log.md)): chi
la possiede regge le spinte fino a quel valore e cede intere a quelle più forti. Un numero per eroe contro
una tabella eroe × fonte di spinta.

Stessa forma, applicata qui:

```text
sull'AZIONE     MinStability   quanta stabilità le serve      (un intero per azione)
sul PROFILO     Stability      quanta stabilità concede       (un intero per profilo)

legale  ⇔  Profilo.Stability >= Azione.MinStability
```

**31 numeri + 4**, contro 124. E la scala è la stessa già usata per il pivot (0–3), quindi non introduce un
vocabolario nuovo.

| Profilo | `Stability` proposta | Lettura |
|---|---:|---|
| fermo (nessun movimento pianificato) | **3** | tutto è possibile |
| `Sneak` | **2** | passo corto e controllato |
| `Move` | **1** | il profilo neutro |
| `Sprint` | **0** | hai speso il turno a coprire distanza |

⚠️ **I quattro valori sono la parte da playtestare, non la parte da decidere.** Ciò che `AE-2` decide è se
esista l'asse; se esiste, questi numeri seguono lo stesso destino di ogni altro valore del catalogo.

### 3.3 Tre stati, non quattro — e il quarto non è stato dimenticato

Il kit propone `NORMAL / IMPAIRED / ENHANCED / BLOCKED`. La soglia ne esprime naturalmente **tre**:

```text
Stability >= MinStability          NORMALE
Stability == MinStability - 1      RIDOTTA        penalità dichiarata sull'azione
Stability <  MinStability - 1      NON PIANIFICABILE   con reason code
```

**`ENHANCED` cade fuori, e non per semplificare.** L'unico esempio che il kit ne dà è *«Momentum Strike:
Sprint → più push/danno»* — ma il momentum dipende da **quante celle hai percorso**, cioè da un fatto del
percorso, che è **`AE-3`** e che [`spec-economia-del-turno.md`](spec-economia-del-turno.md) §4.3 tiene
esplicitamente fuori da E38 per una ragione di determinismo: i fatti del percorso sono noti solo *dopo* la
risoluzione, e un Move può decadere ([D-045](../decisions/RT_PDR_00_Decision_Log.md)).

Una soglia è inoltre **monotona** — più stabilità non può far peggio — mentre `ENHANCED` richiede una
preferenza non monotona. Sono due modelli diversi, e mescolarli costa il doppio.

> Se emergesse un caso di `ENHANCED` **che non dipende dal percorso**, questo modello non lo esprime e va
> esteso. Nessuno è stato prodotto finora: è la condizione di falsificazione di §3.

## 4. Il dato

Nella forma delle convenzioni esistenti — intero, dichiarato, con un default che conserva il comportamento
di oggi:

```text
FRTActionDef                    ← struct ESISTENTE (RTActionDef.h)
  int32 MinStability = 0        // 0 = nessun requisito: l'azione si pianifica con qualunque profilo

<il profilo di movimento>       ← ⚠️ NON esiste come tipo: `grep -rn MovementProfile Source/` dà zero
  int32 Stability               // fermo 3 · Sneak 2 · Move 1 · Sprint 0
```

⚠️ **Il secondo non ha dove atterrare, e va detto invece di sottinteso.** I profili di movimento oggi vivono
**solo nel catalogo markdown** (§2.1: `Move` 5 MP · `Sprint` 8 · `Withdraw` 2): nel codice non esiste nessuna
struct che li rappresenti — `Sprint` è un `ActionId` con `MovementStyle::Budget`, non un profilo con
proprietà. Quindi questa proposta ne richiede **la creazione**, ed è un costo che non appariva nel kit.

È anche l'unico punto in cui #606 tocca strutture invece che valori: se `AE-2` passa, il primo lavoro non è
assegnare soglie, è **dare un tipo ai profili**.

**Il default `0` è il meccanismo, non una cortesia**: un'azione che non dichiara nulla si comporta
esattamente come oggi, quindi il modello non richiede di compilare l'intero catalogo prima di funzionare e
nessun test esistente cambia esito. È lo stesso criterio con cui ADR-0008 §3 ha introdotto le policy di
facing.

**La penalità di `RIDOTTA` va dichiarata sull'azione**, non calcolata: un `-2` alla portata scritto nel dato
è verificabile; una percentuale applicata dal resolver è un ramo che cresce.

## 5. Chi lo legge — e il buco che va detto

| Consumatore | Stato |
|---|---|
| validatore del piano in Planning | 🔴 **non esiste**: `git grep ValidatePlan` non restituisce nulla. È [#605](https://github.com/DegrassiAaron/refactor-tactics-main/issues/605) |
| preview dell'Action Dock | esiste come feature (`RT-FEAT-UI-PLANNING`), non come questo consumo: [#607](https://github.com/DegrassiAaron/refactor-tactics-main/issues/607) |
| bot | 🔴 **deve passare dallo stesso predicato**, o pianifica combinazioni illegali. Ricade su `RT-FEAT-BOT-*` |
| TurnLog | il reason code **estende** `ERTMoveOutcome`/`ERTFallbackOutcome` in coda, non fonda una famiglia nuova |

⚠️ **Senza #605 questa feature non è osservabile.** Un requisito che nessuno valuta è un campo dati senza
consumatore — il difetto ricorrente del repository. L'ordine dei checkpoint di E38 va letto così: #606
definisce il dato, ma è #605 a renderlo una regola.

## 6. Gli scenari, in forma eseguibile

I tre `Spec.ActionEconomy.*` già dichiarati `planned` nel Feature Registry, scritti come oracoli:

```gherkin
Scenario: Spec.ActionEconomy.SprintBlocksPrecision
  Dato    un'unità con `Action.PrecisionAttack` (MinStability 2)
  Quando  pianifica `Sprint` (Stability 0) e quell'attacco
  Allora  il piano è INVALIDO prima del commit
  E       il reason code nomina il profilo, non l'azione
  E       la UI mostra il motivo prima che il giocatore confermi

Scenario: Spec.ActionEconomy.MoveImpairsPrecision
  Dato    la stessa unità e la stessa azione
  Quando  pianifica `Move` (Stability 1) e quell'attacco
  Allora  il piano è VALIDO
  E       la portata effettiva è quella ridotta dichiarata sull'azione
  E       il valore che la preview mostra coincide con quello che il resolver applica

Scenario: Spec.ActionEconomy.SprintEnhancesMomentum
  ⛔ NON si scrive: dipende da AE-3 (fatti del percorso), fuori dallo scope di E38.
     Resta `planned` nel registry come promemoria, e va spostato quando AE-3 è decisa.
```

⚠️ **Il secondo scenario contiene l'assertion che conta**, ed è quella che si dimentica: *preview e resolver
producono lo stesso numero*. Senza, si può avere una UI che promette portata 4 e un resolver che ne applica 6,
ed entrambi «funzionano».

## 7. Come si decide `AE-2` — i criteri, non le opinioni

Perché la decisione sia falsificabile e non una preferenza:

| | Criterio |
|---|---|
| **A favore** | esiste almeno **un'azione** per cui la penalità produce una scelta reale, cioè un turno in cui un giocatore informato sceglierebbe `Move` invece di `Sprint` *per via della penalità*, non per la distanza |
| **Contro** | se ogni azione finisce con `MinStability 0` (nessuno vuole penalizzare niente), l'asse è un campo dati senza consumatore e la risposta è **no** |
| **Costo accettato** | 31 valori nuovi + 4, che nessuno ha misurato, sullo stesso piano degli otto del pivot |
| **Uscita** | se in playtest la penalità non cambia mai la scelta, si azzerano le soglie: il modello resta inerte invece di essere rimosso |

**Vincolo non negoziabile**, invariato dal kit: dev'essere un **dato dell'abilità**, mai un ramo per eroe nel
resolver (invariante #7, [D-029](../decisions/RT_PDR_00_Decision_Log.md)). Criterio d'accettazione nella forma
di quello di E4: *aggiungere «Vektor spara in corsa» non deve toccare `ARTTurnManager`*.

## 8. Cosa questa pagina non fa

- **non decide `AE-2`**: la decisione è dell'autore;
- **non assegna `MinStability` a nessuna azione** — sarebbe inventare 31 numeri prima che il modello esista;
- **non tocca il codice**;
- **non copre i fatti del percorso** (`AE-3`) né il costo del pivot (`FAC-12`);
- **non definisce lo `Sneak`**: il suo budget MP non esiste (`AE-5`), e un profilo senza budget non è
  pianificabile a prescindere dalla stabilità.

## Rapporto con gli altri documenti

| Documento | Cosa possiede |
|---|---|
| [`spec-economia-del-turno.md`](spec-economia-del-turno.md) | la regola in vigore e i quattro budget |
| [`spec-tassonomia-movimento.md`](spec-tassonomia-movimento.md) | il confronto fra le famiglie di movimento |
| [`../balance/RT_ActionCatalog_v0.1.md`](../balance/RT_ActionCatalog_v0.1.md) | i numeri dei profili |
| [`../roadmap/plans/action-economy-consolidamento-2026-08-12.md`](../roadmap/plans/action-economy-consolidamento-2026-08-12.md) | da dove viene la proposta |
| *questa pagina* | **la proposta in forma decidibile**, e i criteri per dirle di no |
