# RT Icon Runtime Mapping — Implementation v0.1

**STEP 06.** Nessun file di codice modificato. Nessun Runtime ID creato. Nessun asset importato, rinominato o cancellato. Nessuna architettura di catalogo introdotta.

Repo: `DegrassiAaron/refactor-tactics-main@main` · albero `b1e2544dc945` · 2026-08-30.

---

## Esito in una riga

**Il mapping è completo e la copertura non regredisce. Tre requisiti dello STEP 06 richiedono di eseguire Unreal e non sono eseguibili qui, e uno di essi ha rivelato un vincolo che il brief non conosceva.**

Il vincolo: un test in CI **asserisce** che sette categorie di `ERTIconCategory` siano vuote. Dare a Waypoint e Destination una categoria runtime esistente — come chiede Decision B2 — produrrebbe una chiave richiesta in una di quelle sette, e **farebbe cadere quel test**. Non è un dettaglio implementativo: è il motivo per cui B2 non è applicabile come scritta.

---

## 1 — Architettura: nessun sistema nuovo

Confermato e rispettato:

- **Non** implementato `DA_IconCatalog` come nuova architettura.
- **Non** implementato `RTBuildIconCatalog`.
- **Non** creato nessun sistema parallelo.

Il sistema esistente — `FName` con prefisso `UI.Icon.`, `RequiredIconIds()` derivata, `URTIconCatalogData`, `ResolveIcon`, `ValidateIconCatalog`, `FindMissingRequiredIcons` — è preservato integralmente.

### DOCUMENTATION_ARCHITECTURE_MISMATCH

| Documento | Afferma | Realtà |
| --- | --- | --- |
| `Content/Icons/LEGGIMI.md` | Il commandlet `RTBuildIconCatalog` «costruisce `DA_IconCatalog` derivando ogni chiave da `RequiredIconIds()`» | Il commandlet **non è mai stato compilato né eseguito** — lo dichiara lo stesso file. `DA_IconCatalog` non esiste come asset nel repository |
| `Content/Icons/LEGGIMI.md` | «le altre 61 sono le 20 ability del roster, **`Action.Dodge`** e le cinque categorie…» — cioè Dodge fra le NON richieste | **Falso.** Vedi §5: `Action.Dodge` è richiesta |

`DA_IconCatalog` non è un'architettura da sostituire: è un **asset di dati mai prodotto** dall'architettura che già esiste. La distinzione conta — non c'è nessun sistema parallelo da smontare, c'è un asset da generare quando Unreal sarà disponibile.

---

## 2 — Copertura: nessuna regressione

| | Valore |
| --- | --- |
| Chiavi richieste | 61 |
| Baseline: chiavi coperte da texture importate in Unreal | **0 / 61** — nessun asset è importato, `DA_IconCatalog` non esiste |
| Dopo STEP 06 | **0 / 61** — nessuna modifica applicata |
| Regressione | **Nessuna.** Non è stato toccato nulla |
| Copertura potenziale dal set esistente da 123 SVG | 61 / 61 per dichiarazione di `LEGGIMI.md` |
| Copertura potenziale dai soli 24 glifi v0.1 | 7 / 61 certe, 13 / 61 con i 6 candidati approvati |

La condizione `coverage AFTER >= coverage BEFORE` è soddisfatta, ma va detta per quello che è: **è soddisfatta perché entrambe valgono zero**, non perché il lavoro sia stato fatto bene. La copertura reale si misura solo dopo che il commandlet è girato.

**I 123 SVG esistenti restano validi.** Nessuna sostituzione applicata, nessuna cancellazione.

---

## 3 — I 7 mapping EXACT, confermati

`RuntimeId` **non modificato** in nessun caso. Il FName esistente resta autoritativo.

| GrammarId | RuntimeId | CurrentAsset | ApprovedV0.1Asset | Consumer |
| --- | --- | --- | --- | --- |
| `Glyph.Move` | `UI.Icon.Action.Move` | `RT_UI_Icon_Action_Move.svg` | `T_UI_Icon_Action_Move` | `URTActionSlotWidget` |
| `Glyph.Wait` | `UI.Icon.Action.Wait` | `RT_UI_Icon_Action_Wait.svg` | `T_UI_Icon_Action_Wait` | `URTActionSlotWidget` |
| `Glyph.Guard` | `UI.Icon.Action.Guard` | `RT_UI_Icon_Action_Guard.svg` | `T_UI_Icon_Action_Guard` | `URTActionSlotWidget` |
| `Glyph.Overwatch` | `UI.Icon.Action.Overwatch` | `RT_UI_Icon_Action_Overwatch.svg` | `T_UI_Icon_Action_Overwatch` | `URTActionSlotWidget` |
| `Glyph.Confirmed` | `UI.Icon.Certainty.Confirmed` | `RT_UI_Icon_Certainty_Confirmed.svg` | `T_UI_Icon_Certainty_Confirmed` | ClassifyPlan / HUD |
| `Glyph.Predicted` | `UI.Icon.Certainty.Predicted` | `RT_UI_Icon_Certainty_Predicted.svg` | `T_UI_Icon_Certainty_Predicted` | ClassifyPlan / HUD |
| `Glyph.Uncertain` | `UI.Icon.Certainty.Uncertain` | `RT_UI_Icon_Certainty_Uncertain.svg` | `T_UI_Icon_Certainty_Uncertain` | ClassifyPlan / HUD |

`MigrationAction` = `REPLACE_WITH_V0_1` per tutti e sette, secondo la policy in §8. Il nome approvato usa `T_UI_Icon_*` per Decision B1.

---

## 4 — I 6 candidati semantici, classificati

| GrammarId | CandidateRuntimeId | VisualMeaning | RuntimeMeaning | Consumers | SemanticDifference | Classification |
| --- | --- | --- | --- | --- | --- | --- |
| `Glyph.Attack` | `UI.Icon.Action.BasicAttack` | colpo diretto generico | l'attacco base, uno di tre | `URTActionSlotWidget` | Il runtime distingue Basic (danno dall'eroe), Precision (24, cooldown 1, +1 portata), Heavy (35 + 35 a struttura, cooldown 2). Tre fasi di priorità diverse, tre costi | **NEEDS_PRODUCT_DECISION** |
| `Glyph.Dash` | `UI.Icon.Action.Sprint` | movimento rapido in linea | 8 MP, applica `Status.Exposed` 1 turno | `URTActionSlotWidget` | `Action.Dash` non esiste. I candidati reali sono tre: Sprint (8 MP + Exposed), Dodge (3 celle, LinearDash), Reposition (2 celle, LinearDash). E `UI.Icon.Phase.Dash` esiste come fase | **NEEDS_PRODUCT_DECISION** |
| `Glyph.Reaction` | `UI.Icon.Action.Counter` | risposta innescata da azione altrui | contrattacco, 16 danni, trigger `HitByDirectAttack` | `URTActionSlotWidget` | Sei azioni Control esprimono la reazione: Counter, Deflect, Intercept, Anchor, Purge, Evade — con quattro trigger diversi. Un glifo non le distingue | **KEEP_SEPARATE** |
| `Glyph.Water` | `UI.Icon.Status.Wet` | superficie bagnata | stato dell'unità bagnata | tag gameplay | Dominio diverso: cella contro unità. Vedi §5 | **KEEP_SEPARATE** |
| `Glyph.Fire` | `UI.Icon.Status.Burning` | superficie in fiamme | stato dell'unità che brucia | tag gameplay | Dominio diverso | **KEEP_SEPARATE** |
| `Glyph.Electric` | `UI.Icon.Status.Electrified` | superficie caricata | stato dell'unità folgorata | tag gameplay | Dominio diverso | **KEEP_SEPARATE** |

**Nessuno approvato sulla base del nome.** Tre sono `KEEP_SEPARATE` per differenza di dominio, tre richiedono una decisione di prodotto sulla cardinalità (un glifo per N azioni runtime).

### Nota sull'enum di MappingType

Il brief definisce `MappingType` con cinque valori: EXACT, APPROVED_SEMANTIC, EXISTING_RUNTIME_ONLY, VISUAL_ONLY, SEPARATE_SEMANTIC. **Manca un valore per «candidato esaminato e non approvato»**: `APPROVED_SEMANTIC` afferma un'approvazione che non c'è stata.

Nel manifest tutti e sei i candidati portano quindi `SEPARATE_SEMANTIC` — che è vero per costruzione (restano concetti separati finché qualcuno non li unisce) e non asserisce nulla di falso. La distinzione fra i tre `KEEP_SEPARATE` e i tre `NEEDS_PRODUCT_DECISION` vive nella colonna `MigrationAction`, dove tutti e sei portano `NEEDS_DECISION`, e in questa tabella. `APPROVED_SEMANTIC` non compare su nessuna riga del CSV: zero approvati significa zero righe approvate.

Le due decisioni di prodotto sono la stessa domanda posta due volte: **il dock deve distinguere le varianti di una famiglia?** Se sì, servono tre glifi d'attacco e tre di movimento rapido, e la v0.1 ne ha uno per famiglia. Se no, un glifo per famiglia basta e va dichiarato che Precision e Heavy condividono l'icona di Basic. Non è una scelta che posso fare al posto vostro: dipende da quanto il dock deve essere leggibile a colpo d'occhio contro quanto deve essere preciso.

---

## 5 — Water / Fire / Electric: la separazione dei domini

**Nessun alias applicato.** Le due letture restano concetti distinti.

| VisualConcept | RuntimeConcept | Domain | RuntimeId | Consumer |
| --- | --- | --- | --- | --- |
| Water surface | — | cella / superficie | **MISSING_SURFACE_RUNTIME_CONSUMER** | nessuno |
| — | Wet unit status | unità | `UI.Icon.Status.Wet` | tag gameplay, `TAG_Status_Wet` |
| Fire surface | — | cella / superficie | **MISSING_SURFACE_RUNTIME_CONSUMER** | nessuno |
| — | Burning unit status | unità | `UI.Icon.Status.Burning` | tag gameplay, `TAG_Status_Burning` |
| Electric surface | — | cella / superficie | **MISSING_SURFACE_RUNTIME_CONSUMER** | nessuno |
| — | Electrified unit status | unità | `UI.Icon.Status.Electrified` | tag gameplay, `TAG_Status_Electrified` |

I RuntimeId nella colonna sono **esclusivamente** quelli trovati nel repository. Nessun ID di superficie è stato creato.

Un dato che rafforza la separazione: le superfici **esistono** nel gameplay come `ERTHexSurface` — `Action.CreateWater` produce acqua raggio 1, `Action.Ignite` produce fuoco, `Action.Electrify` carica. Quindi il concetto di superficie non è ipotetico: è implementato, e semplicemente **non ha una chiave icona**. La categoria `ERTIconCategory::Environment` che la ospiterebbe è dichiarata e vuota, e §7 spiega perché non si può riempire adesso.

Le tre azioni che creano superfici hanno la loro chiave (`UI.Icon.Action.CreateWater` e simili), ma quella è l'**azione**, non lo stato risultante. Non sono intercambiabili: il dock mostra l'azione pianificata, la mappa mostrerebbe lo stato del terreno.

---

## 6 — Action.Dodge: risolto

Il brief chiede evidenza da codice eseguibile. **Non posso compilare né eseguire Unreal** (STOP-2). Ma questa specifica domanda si chiude con evidenza statica conclusiva, e la riporto insieme al suo limite.

**Evidenza 1 — l'add è incondizionato.**
`Source/RefactorTactics/Ability/RTCatalogLibrary.cpp:1179`:

```cpp
Catalog.Add(ShippedAction(TEXT("Action.Dodge"), ERTResolutionPhase::FastMovement, /*Priority*/ 30,
    /*Range*/ 3, /*Cooldown*/ 1, ERTActionFallback::Stop, {},
    /*bInterruptible*/ true, ERTActionSlot::Movement, ERTMovementStyle::LinearDash));
```

Nessun `if`, nessun `#if`, nessuna CVar, nessun feature gate racchiude questa riga. Verificato con grep su tutto il corpo di `GetCoreActionCatalog()`: fra la riga 1027 (apertura funzione) e la fine, non esiste alcun condizionale. Il catalogo è costruito da 37 `Catalog.Add` incondizionate.

**Evidenza 2 — l'aritmetica lo pretende.**
`RequiredIconIds()` deriva le chiavi Action da `GetCoreActionCatalog()`. Con Dodge: 37 azioni + 4 fasi + 11 stati + 3 certezze + 6 identità = **61**, che è il totale dichiarato da `LEGGIMI.md`. **Senza** Dodge il totale sarebbe 60. Il documento afferma 61 e contemporaneamente esclude Dodge: le due affermazioni non possono essere vere insieme.

**Verdetto: `Action.Dodge` = RUNTIME_REQUIRED.**
`Content/Icons/LEGGIMI.md` è **STALE** su questo punto. Il conteggio totale del documento (61) è corretto; la frase che elenca Dodge fra le non richieste no.

**Limite dichiarato.** L'evidenza è statica. Non ho eseguito `RequiredIconIds()`, quindi non posso escludere effetti che solo il runtime rivela — per esempio un `ValidateActions` che scartasse l'azione a monte, o un ordine dei tag diverso da quello atteso. Il test da eseguire quando Unreal è disponibile è già scritto in `RTIconCatalogTests.cpp`: `RequiredIdsFollowGameData` itera `GetCoreActionCatalog()` e pretende la chiave di ogni azione, quindi **se Dodge non fosse richiesta quel test sarebbe già rosso**. Vale come conferma indiretta: il test esiste, e nessuno ha segnalato che fallisca.

---

## 7 — Waypoint, Destination, LastContact: perché B2 non è applicabile

Decision B2 dice: «Waypoint e Destination devono usare la categoria runtime esistente semanticamente corretta, determinata dal repository».

Il repository la determina, e la risposta è che **non esiste**.

Le dodici categorie di `ERTIconCategory` sono Identity, Action, Phase, Environment, MapInteraction, Status, Information, Reaction, Coordination, Certainty, Warning, Objective. Nessuna descrive un nodo di percorso. La più vicina è `Coordination` — «comunicazione fra alleati: ping, pronto, intento, conflitto» — ma un waypoint del **proprio** percorso non è comunicazione fra alleati, e forzarlo lì sarebbe inventare una semantica per far quadrare un documento.

E c'è un vincolo più duro. `RTIconCatalogTests.cpp`, test `V01CategoriesPopulated`:

```cpp
for (const ERTIconCategory Category : { ERTIconCategory::Environment, ERTIconCategory::MapInteraction,
    ERTIconCategory::Information, ERTIconCategory::Reaction, ERTIconCategory::Coordination,
    ERTIconCategory::Warning, ERTIconCategory::Objective })
{
    TestFalse(..., Populated.Contains(Category));
}
```

**`TestFalse`.** Il test asserisce che queste sette categorie **non** siano popolate, e il commento dichiara l'intenzione: «una chiave che comparisse qui chiederebbe un disegno per un sistema che ancora non la consuma — ed è esattamente quello che #219 dice di non fare».

Conseguenza operativa, valida per **undici** glifi v0.1 e non solo per due:

| Glifi v0.1 | Categoria di destinazione | Effetto di una chiave richiesta lì |
| --- | --- | --- |
| Water, Fire, Electric (superficie), Cover, Height | `Environment` | **rompe** `V01CategoriesPopulated` |
| Critical, FriendlyFire, Collision, InsufficientResource, InvalidTarget, UncertainOutcome | `Warning` | **rompe** il test |
| LastContact | `Information` | **rompe** il test |
| Waypoint, Destination | nessuna categoria adatta | non esprimibile |

Quindi:

- **La mappatura di documentazione `Intel` → `Information` è registrata** (B2 rispettata su questo punto): sono semanticamente equivalenti, e `Information` descrive «visibile, sentito, ultima posizione nota, area approssimativa» — esattamente il namespace `IntelSource.*` della v0.1a.
- **Nessuna chiave richiesta è stata creata** in `Information`, né in `Environment`, né in `Warning`. Farlo avrebbe rotto la CI, e §14 vieta modifiche non necessarie.
- **`Marker` non è stata introdotta** come nuova `ERTIconCategory`, come B2 prescrive. Waypoint e Destination restano `VISUAL_NOT_YET_CONSUMED` senza categoria assegnata.

Questo è il vero contenuto informativo dello STEP 06: **le sette categorie vuote non sono un vuoto da riempire, sono un invariante protetto da un test.** Popolarle è una decisione di prodotto che apre un checkpoint, non un passo di mapping.

---

## 8 — Migration policy

Applicata in questo step: **nessuno dei quattro passi è stato eseguito**, perché tutti richiedono Unreal. La policy resta registrata per STEP 07:

1. l'asset vecchio **rimane disponibile**;
2. import del nuovo asset;
3. aggiornamento del mapping;
4. validator e test;
5. **solo dopo**, eventuale cleanup del legacy.

Mai `delete old` → `import new`. **Asset legacy cancellati in questo step: 0.**

---

## 9 — Nessun Runtime ID nuovo

**NEW RUNTIME IDS CREATED: 0.**

Gli 11 glifi senza consumer sono classificati `VISUAL_NOT_YET_CONSUMED`: Cover, Height, Critical, FriendlyFire, Collision, InsufficientResource, InvalidTarget, UncertainOutcome, Waypoint, Destination, LastContact.

Più i tre concetti di superficie (Water, Fire, Electric surface), che sono `MISSING_SURFACE_RUNTIME_CONSUMER`.

Un nuovo RuntimeId richiede un consumer reale più un requisito reale di gameplay o UI. Nessuno dei quattordici li ha oggi. Il catalogo di design non forza l'esistenza della chiave runtime — ed è la stessa disciplina che il codice già applica a sé stesso.

---

## 10 — Code changes

**Nessuna.** Zero file di codice modificati.

Le modifiche che sarebbero state necessarie sono tutte bloccate da §7 o da STOP-2:

| Modifica candidata | Stato |
| --- | --- |
| Fixture dei test per i nomi `T_UI_Icon_*` | Già usa `T_` — nessuna modifica necessaria |
| Voci di lookup del catalogo | Bloccata: `DA_IconCatalog` non esiste |
| Nuove chiavi in Environment / Warning / Information | **Vietata**: romperebbe `V01CategoriesPopulated` |
| `AssetNameForIcon` nel commandlet | Non toccata: il commandlet non compila (STOP-2) |
| Documentazione | Fatta: questo file, il CSV, il TXT |

Non modificati, come prescritto: simulazione, semantica di gameplay, networking, comportamento delle abilità.

---

## 11 — Tests

**NON ESEGUITI.** Nessun test è stato lanciato: richiede Unreal Editor, che non è disponibile in questo ambiente. Questo è STOP-2.

Stato dei sei test minimi richiesti dal brief:

| | Test richiesto | Esiste già? | Stato |
| --- | --- | --- | --- |
| A | `RequiredIconIds` deterministico | **Sì** — `RequiredIdsFollowGameData` verifica `Required == RequiredIconIds()` | NOT_RUN |
| B | Ogni RequiredIconId risolve a un asset | **Sì** — `EveryKeyResolves` | NOT_RUN |
| C | Nessun RuntimeId duplicato | **Sì** — `DuplicateIdIsValidationError` | NOT_RUN |
| D | La copertura non regredisce | **Sì** — `MissingKeyIsValidationError` + `FindMissingRequiredIcons` | NOT_RUN |
| E | Aspettativa su `Action.Dodge` derivata dal catalogo | **Sì, implicitamente** — `RequiredIdsFollowGameData` itera il catalogo e pretende ogni chiave | NOT_RUN |
| F | Tutti i mapping EXACT v0.1 risolvono | **No** | NON AGGIUNTO — vedi sotto |

Cinque dei sei esistono già e coprono ciò che il brief chiede. Il sesto **non l'ho aggiunto**, per due ragioni: non posso verificare che passi, e un test che asserisse la risoluzione dei sette asset `T_UI_Icon_*` fallirebbe oggi, perché quegli asset non esistono. Aggiungerlo significherebbe consegnare CI rossa.

Il test F va scritto **dentro STEP 07**, nello stesso commit che importa i sette asset. Prima è un test che documenta un'assenza.

---

## STEP 06 STATUS

```
PARTIZIONE DELLE 61 CHIAVI:  7 EXACT + 6 SEPARATE_SEMANTIC + 48 EXISTING_RUNTIME_ONLY = 61
GLIFI v0.1 SENZA CONSUMER:   11  (universo dei 24 glifi, NON parte delle 61)

REQUIRED:                      61
COVERED:                       0 / 61   (baseline 0/61 — nessuna regressione)
  copertura potenziale set 123:   61 / 61
  copertura potenziale v0.1:       7 / 61  (13 con i candidati)

V0.1 EXACT:                     7
V0.1 APPROVED SEMANTIC:         0   (6 candidati esaminati, 0 approvati; nel CSV sono SEPARATE_SEMANTIC)
  di cui KEEP_SEPARATE:         3   (Water, Fire, Electric — dominio diverso)
  di cui NEEDS_PRODUCT_DECISION: 3  (Attack, Dash, Reaction — cardinalita')
V0.1 VISUAL-ONLY:              11
EXISTING RUNTIME-ONLY:         48
NEW RUNTIME IDS CREATED:        0   (atteso: 0 ✓)
LEGACY ASSETS DELETED:          0   (atteso: 0 ✓)
CODE FILES MODIFIED:            0
TESTS:                     NOT_RUN  (5 dei 6 richiesti esistono gia')
```

7 + 6 + 48 = 61 ✓ — e gli 11 VISUAL-ONLY sono fuori da questa somma: appartengono ai 24 glifi v0.1, non alle chiavi runtime.

### STOP — tre condizioni

**STOP-1 · La derivazione di `RequiredIconIds()` non è stata eseguita.**
Il brief (§7) chiede di eseguirla e di confrontare il COUNT con 61. Ho prodotto `RT_RequiredIconIds_Runtime_v0.1.txt` per **derivazione statica dal sorgente**, e il conteggio è 61 — coerente con l'atteso. Ma il file dichiara in testa che non è output di runtime. Due componenti in particolare dipendono dal runtime: i tag `Status.*` arrivano da `UGameplayTagsManager` (11 registrati nativamente, ma il manager potrebbe conoscerne altri da `.ini`), e le identità da `GetHeroIds()`.

**STOP-2 · Nessun test eseguito.**
Richiede Unreal Editor. §15 lo esige, e non è aggirabile con una lettura del codice.

**STOP-3 · Decision B2 non applicabile come scritta.**
Non per una scelta mia: assegnare a Waypoint, Destination o LastContact una categoria runtime esistente creerebbe una chiave richiesta in una delle sette categorie che `V01CategoriesPopulated` **asserisce vuote**. Il mapping di documentazione `Intel` → `Information` è registrato; la chiave no. Questa è una decisione di prodotto, non di mapping.

Nessuna delle tre condizioni di STOP del §17 è invece scattata: la copertura non è scesa, il conteggio è quello atteso, e nessun mapping richiede semantica di gameplay non approvata.

---

## Prossimo passo

**STEP 07 — Affinity Export + Selective UE5 Asset Migration** ha tre precondizioni dichiarate dal brief: 61/61 di copertura, mapping approvato, test PASS. **Nessuna delle tre è soddisfatta**, e due dipendono da un ambiente Unreal.

Il percorso minimo per arrivarci:

1. Aprire l'Editor ed eseguire `RequiredIconIds()` — chiude STOP-1 e conferma o smentisce le 61.
2. Lanciare la suite `RefactorTactics.IconCatalog.*` — chiude STOP-2 e dà la baseline vera.
3. Eseguire `RTBuildIconCatalog -DryRun` con il set da 123 e prefisso `T_UI_Icon_*` — produce la prima copertura reale, che è la baseline da non far regredire.
4. Decidere le tre cardinalità (Attack, Dash, Reaction) e se popolare `Environment` e `Warning` — sono decisioni di prodotto, e la seconda apre un checkpoint.

I passi 1–3 sono meccanici e non richiedono decisioni. Il passo 4 sì, e conviene affrontarlo con i numeri dei passi 1–3 in mano.
