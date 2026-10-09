# STEP 06b — Runtime Verification + Product Mapping Freeze

Nessun asset importato. Nessun Runtime ID creato. `ERTIconCategory` non modificata. `RequiredIconIds()` non modificata. Grammatica visuale non toccata.

Repo: `DegrassiAaron/refactor-tactics-main@main` · albero `b1e2544dc945` · 2026-08-30.

---

## Esito

**Il mapping è congelato. La verifica Unreal non è eseguibile in questo ambiente, quindi lo STATUS non può essere PASS.**

Due risultati vanno oltre quello che il brief si aspettava, e li metto per primi perché cambiano il quadro:

**1. Le categorie protette e il set richiesto sono disgiunti.** Zero delle 61 chiavi richieste appartiene a una categoria protetta da `V01CategoriesPopulated`. Le 61 vivono interamente in Phase, Action, Status, Certainty, Identity — cinque categorie non protette. L'invariante non è in tensione col set richiesto: **è al sicuro per costruzione**. Popolare Environment o Warning sarebbe un'aggiunta deliberata, non una conseguenza del mapping.

**2. La classificazione a 24 non esce 7 / 6 / 11.** Esce **7 EXACT · 3 FAMILY_VISUAL_ONLY · 12 VISUAL_ONLY · 2 RUNTIME_NOT_APPLICABLE**. Il brief autorizza la deviazione («non forzare i numeri se il repository mostra diversamente») e la ragione è nella §3.

> **Base probatoria di tutto il documento.** Ogni numero qui deriva dalla **lettura statica del sorgente**, non dall'esecuzione. Non ho un ambiente Unreal: nessuno dei sei test è stato lanciato, e anche la conclusione del punto 1 segue dal codice letto, non da un run. Nessun valore *differisce* dall'atteso; nessuno è *verificato*. È la ragione per cui lo STATUS non è PASS.

---

## 1 — Decisione di prodotto, applicata

La regola: le azioni runtime selezionabili separatamente mantengono identità visuali distinte. Quindi un glifo di famiglia non sostituisce più RuntimeId.

`SEMANTIC_MATCH_NEEDS_APPROVAL` **non è più usata** per i sei casi. Riclassificati:

| GrammarId | Nuova classe | Chiavi runtime coinvolte | Perché |
| --- | --- | --- | --- |
| `Glyph.Attack` | **FAMILY_VISUAL_ONLY** | BasicAttack, PrecisionAttack, HeavyAttack | Tre azioni selezionabili separatamente, con priorità e costi diversi |
| `Glyph.Dash` | **FAMILY_VISUAL_ONLY** | Sprint, Dodge, Reposition | Tre azioni `LinearDash` selezionabili separatamente. `Action.Dash` non esiste |
| `Glyph.Reaction` | **FAMILY_VISUAL_ONLY** | Counter, Deflect, Intercept, Anchor, Purge, Evade | Sei azioni Control con quattro trigger diversi |
| `Glyph.Water` | **VISUAL_ONLY** | nessuna | Vedi §3 |
| `Glyph.Fire` | **VISUAL_ONLY** | nessuna | Vedi §3 |
| `Glyph.Electric` | **VISUAL_ONLY** | nessuna | Vedi §3 |

I tre glifi di famiglia restano **primitive semantiche senza consumer runtime diretto**, come il brief consente. Non sono scarti: sono il vocabolario da cui derivare le varianti quando qualcuno le disegnerà.

---

## 2 — Nessuna evidenza di identità 1:1 per i sei

Il brief ammette `EXACT` per un candidato «solo se evidenza runtime dimostra identità 1:1». Non ce n'è per nessuno dei sei, e vale la pena dire perché in modo verificabile:

- **BasicAttack** non è l'attacco generico: è *uno dei tre*, quello che prende il danno dall'eroe invece di dichiararlo. Precision (24, cd 1, +1 portata) e Heavy (35 + 35 a struttura, cd 2) sono voci autonome del catalogo. Un glifo su tre chiavi non è 1:1.
- **Sprint** non è il movimento rapido generico: applica `Status.Exposed` per un turno, cosa che Dodge e Reposition non fanno. Sono tre comportamenti diversi.
- **Counter** ha trigger `HitByDirectAttack`; le altre cinque azioni Control hanno trigger diversi. Un glifo su sei chiavi non è 1:1.

I quattro `EXACT` sopravvissuti fra le azioni — Move, Wait, Guard, Overwatch — lo sono perché **non hanno varianti nel catalogo**: una chiave, un'azione, un significato.

---

## 3 — Water / Fire / Electric: VISUAL_ONLY, non FAMILY

Qui devio dall'attesa del brief, e la deviazione è sostanziale.

`FAMILY_VISUAL_ONLY` descrive un glifo che copre **più chiavi runtime della stessa famiglia**. Water, Fire ed Electric non sono in quella condizione: coprono **zero** chiavi runtime.

`Status.Wet` esiste, ma non è una variante di `Glyph.Water` — è un **concetto di dominio diverso**. La superficie bagnata è una proprietà della cella; lo stato bagnato è una proprietà dell'unità. Le due informazioni compaiono in punti diversi della schermata, cambiano per cause diverse, e in una partita reale possono essere vere indipendentemente: un'unità asciutta su una cella allagata, un'unità bagnata su terreno asciutto.

Chiamarli famiglia implicherebbe che `Status.Wet` sia una specializzazione di `Glyph.Water`, cioè esattamente l'aliasing che lo STEP 06 ha rifiutato. La classe onesta è **VISUAL_ONLY** con annotazione `MISSING_SURFACE_RUNTIME_CONSUMER`.

Le superfici **esistono** nel gameplay come `ERTHexSurface` — `CreateWater` produce acqua raggio 1, `Ignite` fuoco, `Electrify` carica. Il concetto non è ipotetico: è implementato e non ha una chiave icona, perché la categoria che la ospiterebbe (`Environment`) è protetta.

**Conteggio risultante:** 7 EXACT + 3 FAMILY_VISUAL_ONLY + 12 VISUAL_ONLY + 2 RUNTIME_NOT_APPLICABLE = 24.

I due `RUNTIME_NOT_APPLICABLE` sono Waypoint e Destination: nessuna delle dodici `ERTIconCategory` descrive un nodo di percorso, e `Marker` non è stata introdotta.

---

## 4 — Categorie protette: nessuna popolata

`V01CategoriesPopulated` è trattato come autoritativo per la milestone.

| Categoria protetta | Chiavi richieste dentro | Nuove chiavi create |
| --- | --- | --- |
| Environment | 0 | 0 |
| MapInteraction | 0 | 0 |
| Information | 0 | 0 |
| Reaction | 0 | 0 |
| Coordination | 0 | 0 |
| Warning | 0 | 0 |
| Objective | 0 | 0 |

**Zero su tutta la riga.** La mappatura documentale `Intel` → `Information` resta registrata come equivalenza semantica e **non** implica la creazione di un RuntimeId.

Il fatto che tutte e sette contengano zero chiavi richieste è il risultato strutturale della §Esito: l'invariante e il set richiesto non si toccano.

---

## 5 — Verifica Unreal: NON ESEGUITA

Il brief chiede di eseguire sei test nell'Editor o in una build reale. **Non è possibile in questo ambiente**: non ho Unreal Editor né toolchain di compilazione.

| # | Test richiesto | Esiste in repo | Esecuzione |
| --- | --- | --- | --- |
| 1 | `RequiredIconIds` | `RequiredIdsFollowGameData` | **NOT_RUN** |
| 2 | IconLibrary coverage | `EveryKeyResolves` | **NOT_RUN** |
| 3 | duplicate Runtime ID | `DuplicateIdIsValidationError` | **NOT_RUN** |
| 4 | `V01CategoriesPopulated` | presente | **NOT_RUN** |
| 5 | deterministic required set | `RequiredIdsFollowGameData` | **NOT_RUN** |
| 6 | `Action.Dodge` presence | implicito in #1 | **NOT_RUN** |

Tutti e sei **esistono già** nel repository. Manca l'esecuzione, non la copertura di test.

### I tre valori da verificare

| Valore | Atteso | Stato qui |
| --- | --- | --- |
| `RequiredIconIds` count | 61 | **61 per derivazione statica** — non per esecuzione |
| `Action.Dodge` | present | **present per evidenza statica** — non per esecuzione |
| Coverage baseline | invariata / completa | **invariata a 0/61** — nessun asset importato, `DA_IconCatalog` inesistente |

Nessuno dei tre *differisce* dall'atteso, quindi la condizione di STOP del brief («se uno di questi valori differisce: STOP») non è scattata. Ma nessuno dei tre è **verificato per esecuzione**, che è ciò che il brief chiede.

---

## 6 — Action.Dodge

Il brief condiziona la marcatura all'esecuzione: «se l'esecuzione conferma Dodge». L'esecuzione non c'è, quindi mantengo la marcatura **provvisoria** con l'evidenza che ho:

```
Action.Dodge = RUNTIME_REQUIRED   [evidenza statica, non di esecuzione]
LEGGIMI.md   = STALE_DOCUMENTATION [conseguenza della precedente]
```

Evidenza, invariata dallo STEP 06 e ancora conclusiva sul piano statico:

1. `RTCatalogLibrary.cpp:1179` — `Catalog.Add(ShippedAction(TEXT("Action.Dodge"), …))` senza alcun condizionale. Verificato su tutto il corpo di `GetCoreActionCatalog()`: nessun `if`, `#if` o CVar fra l'apertura e la chiusura.
2. Aritmetica: con Dodge il totale è 61, che è quanto `LEGGIMI.md` dichiara. Senza, 60. Il documento afferma 61 *ed* esclude Dodge — le due cose non stanno insieme.

**Il runtime non è stato modificato per conformarlo al README**, come prescritto. È il README che è disallineato.

Nota di metodo: la marcatura resta provvisoria non perché dubiti dell'evidenza, ma perché il brief distingue esplicitamente fra le due fonti. Il primo run dei test la promuove o la smentisce in pochi secondi.

---

## 7 — Matrice di copertura · 61 righe

File: `docs/UI/IconLanguage/RT_IconCoverage_61_v0.1.csv`

| Colonna | Contenuto |
| --- | --- |
| `Required` | **TRUE** su tutte e 61 |
| `Resolved` | **UNVERIFIED** su tutte e 61 |
| `ProtectedCategory` | **FALSE** su tutte e 61 |
| `V01ExactReplacementCandidate` | popolata su **7**, `NONE` sulle altre 54 |

**Il target 61/61 resolved non è raggiunto, e non è raggiungibile qui.** `Resolved` misura se una chiave trova la sua texture nel catalogo; `DA_IconCatalog` non esiste e nessun asset è importato in Unreal. Scrivere `TRUE` sarebbe inventare un dato — `UNVERIFIED` è l'unico valore onesto, e il primo `RTBuildIconCatalog -DryRun` lo sostituisce con la misura vera.

---

## 8 — Matrice visuale · 24 righe

File: `docs/UI/IconLanguage/RT_IconVisual_24_v0.1.csv`

```
EXACT                    7
FAMILY_VISUAL_ONLY       3
VISUAL_ONLY             12
RUNTIME_NOT_APPLICABLE   2
                        --
                        24
```

`SEMANTIC_MATCH_NEEDS_APPROVAL` non compare più. `ReplacementCandidate` è popolata solo sui sette EXACT.

---

## STEP 06b STATUS

```
RUNTIME VERIFICATION:              NOT_EXECUTED  (nessun Unreal Editor disponibile)

RequiredIconIds count:             61        [statico, non eseguito]
Action.Dodge:                      present   [statico, non eseguito]
Coverage:                          0 / 61    [invariata; target 61/61 NON raggiunto]
CI category invariants:            NOT_RUN
Nuovi RuntimeId:                   0   ✓
Categorie protette popolate:       0   ✓
Runtime asset cancellati:          0   ✓
Chiavi richieste in cat. protette: 0   ✓ (risultato strutturale)

24-glyph freeze:                   COMPLETO
  EXACT                            7
  FAMILY_VISUAL_ONLY               3
  VISUAL_ONLY                     12
  RUNTIME_NOT_APPLICABLE           2

STATUS:                            BLOCKED_ON_VERIFICATION
```

**Non è PASS.** Quattro dei sette criteri di uscita sono soddisfatti — nessun nuovo RuntimeId, nessuna categoria protetta popolata, nessun asset cancellato, e il freeze del mapping è completo. Tre no, e tutti tre per la stessa causa: `RequiredIconIds` non verificata per esecuzione, copertura a 0/61 invece di 61/61, invarianti CI non eseguite.

Nessuna condizione di STOP del brief è scattata: nessun valore *differisce* dall'atteso. La distinzione è fra "smentito" e "non verificato", e siamo nel secondo caso.

---

## Prossimo passo

Poiché lo STATUS non è PASS, **non propongo STEP 07 — Selective Visual Adoption Plan**. La sua premessa è che i mapping EXACT siano verificati, e la verifica è esattamente ciò che manca.

Il freeze però è completo e non richiede altro lavoro di design. Quello che serve sono tre comandi in un ambiente Unreal:

1. `RefactorTactics.IconCatalog.*` — chiude i criteri 1, 4, 5, 6 e promuove la marcatura di Dodge da statica a eseguita.
2. `RequiredIconIds()` nell'Editor — conferma le 61 e in particolare i tag `Status.*`, che arrivano da `UGameplayTagsManager` e potrebbero includerne altri da `.ini`.
3. `RTBuildIconCatalog -DryRun` con il set da 123 e prefisso `T_UI_Icon_*` — sostituisce i 61 `UNVERIFIED` con la copertura reale.

Con quei tre output, STEP 06b si chiude in PASS senza altre decisioni e STEP 07 diventa proponibile sui sette EXACT.
