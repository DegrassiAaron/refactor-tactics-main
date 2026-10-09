# RT Icon Runtime Mapping — Audit v0.1

**STEP 05 · read-only.** Nessun file di codice modificato, nessun Runtime ID creato, nessuna voce di catalogo aggiunta, nessun asset importato o rinominato.

Repository: `DegrassiAaron/refactor-tactics-main@main` · albero `b1e2544dc945` · audit del 2026-08-30.

---

## Executive Summary

La premessa dello STEP 04 era sbagliata, e l'audit la corregge. Non ci sono zero Runtime ID: ce ne sono **61**, il sistema che li consuma è completo e testato, e il problema reale è di **scala e di tassonomia**, non di assenza.

Tre fatti che cambiano il piano:

1. **`RequiredIconIds()` non è una lista, è una derivazione.** Le chiavi vengono da dati di gioco reali — le fasi del turno, il catalogo delle azioni, i tag di stato registrati, il roster degli eroi. Aggiungere un'azione al catalogo fa cadere la copertura il giorno stesso, senza che nessuno modifichi la lista delle icone. Questo è per costruzione, ed è dichiarato come tale nel commento della funzione.

2. **La v0.1 visuale e il set richiesto si sovrappongono poco.** 24 glifi contro 61 chiavi: **7 combaciano esattamente**, 6 sono candidati semantici che richiedono approvazione, 11 non hanno consumer nel build attuale. Sul lato opposto, **48 chiavi richieste non hanno nessun glifo v0.1**, nemmeno come candidato.

3. **Esistono già 123 master SVG** in `Content/Icons/Icons/`, generati il 2026-08-26 e mai importati in Unreal. Non sono legacy morto: sono un set parallelo, più ampio della v0.1, prodotto da un generatore diverso.

Il blocco non è più "servono gli ID". È: **la v0.1 copre il 11% delle chiavi che il gioco già pretende.**

---

## Runtime Identifier Model

| Domanda | Risposta reale |
| --- | --- |
| Tipo | **`FName`** — non `FGameplayTag`, non enum, non `PrimaryAssetId` |
| Forma | `UI.Icon.<Categoria>.<Nome>` |
| Prefisso | `URTIconLibrary::IconIdPrefix = TEXT("UI.Icon.")` — obbligatorio, verificato dal validator |
| Costruzione | `URTIconLibrary::MakeIconId(SemanticPath)` — `BlueprintPure` |
| Regola | La chiave si **deriva** dall'identificatore stabile che esiste già, non si inventa accanto a lui |

**Il prefisso non è decorazione.** Senza di esso `Status.Wet` come icona e `Status.Wet` come Gameplay Tag sarebbero la stessa stringa in un log, e D-031 li vuole concetti distinti. È anche il motivo per cui i Gameplay Tag esistono nel progetto (`Status.*` in `Core/RTGameplayTags.cpp`) ma **non sono** l'identificatore delle icone: sono la *sorgente* da cui alcune chiavi si derivano.

Una sola traduzione esiste nel sistema: gli `HeroId` sono `Hero.Gadget`, e `MakeIconId` ne farebbe `UI.Icon.Hero.Gadget` — che il validator rifiuta perché il segmento `Hero` non combacia con la categoria `Identity`. La derivazione tiene il nome e sostituisce il prefisso: `Hero.Gadget` → `Identity.Gadget`.

---

## Required Icons

**61 chiavi**, in ordine deterministico. Composizione ricostruita da `RTIconLibrary.cpp:27–140`:

| Categoria | N | Sorgente | Derivata o scritta |
| --- | --- | --- | --- |
| Phase | 4 | `ERTMatchPhase` — solo `Prep`, `Dash`, `Blast`, `Move` | derivata, filtrata a mano |
| Action | 37 | `URTCatalogLibrary::GetCoreActionCatalog()` | derivata |
| Status | 11 | tag registrati sotto `Status.` | derivata, ordinata lessicalmente |
| Certainty | 3 | `Confirmed`, `Predicted`, `Uncertain` | **scritta a mano, deliberatamente** |
| Identity | 6 | 4 `GetHeroIds()` + `Ally` + `Enemy` | derivata + 2 scritte |

**4 + 37 + 11 + 3 + 6 = 61.** L'aritmetica torna con il conteggio dichiarato in `Content/Icons/LEGGIMI.md`.

Due esclusioni sono decisioni documentate, non omissioni:

- **`Planning` e `Cleanup` non sono fasi richieste**, perché non sono fasi in cui il giocatore agisce. E `Reaction` non è una quinta fase: mostrarla fra le fasi la trasformerebbe in una.
- **`Certainty` non deriva da `ERTIntentCertainty`** pur esistendo il tipo. L'enum ha quattro valori e la categoria ne pretende tre: `Unknown = 0` significa "mai calcolato" e non ha una resa — un'icona lo farebbe sembrare uno stato previsto invece del difetto a monte che è. Le chiavi sono i livelli **disegnabili**, non i valori del tipo.

I tag di stato reali: `Root`, `Slow`, `Reveal`, `Exposed`, `Guarded`, `Marked`, `Wet`, `Braced`, `Burning`, `Obscured`, `Electrified`.

**Copertura attuale del catalogo: sconosciuta.** `DA_IconCatalog` non esiste nel repository — il commandlet che lo costruisce (`RTBuildIconCatalog`) non è mai stato compilato né eseguito, per dichiarazione esplicita di `LEGGIMI.md`. Quindi `FindMissingRequiredIcons(nullptr)` restituirebbe oggi tutte e 61.

---

## Icon Catalog Architecture

| | |
| --- | --- |
| **A. Classe** | `URTIconCatalogData : public UPrimaryDataAsset` |
| **B. Key** | `FRTIconDef::IconId` (`FName`) + `Category` (`ERTIconCategory`) |
| **C. Value** | `TSoftObjectPtr<UTexture2D> Asset` — soft: il catalogo si carica intero, le texture no |
| **D. MissingIcon** | Sì, **obbligatoria**. Il validator rifiuta un catalogo che non ce l'ha |
| **E. Validation** | `ValidateIconCatalog()` — rifiuta ID assente, prefisso mancante, duplicato, categoria non combaciante, asset nullo, MissingIcon non impostata |
| **F. Lookup** | `ResolveIcon()` — scansione lineare, restituisce `FRTIconResolution{Asset, bResolved}` |
| **G. Consumer** | `URTActionSlotWidget` via `GetIconId()`; il catalogo si raggiunge da `URTTacticalHUDWidget` risalendo l'albero con `GetIconCatalog()` |
| **H. Blueprint** | Sì, tutta l'API è `BlueprintCallable`; le tre funzioni pure sono `BlueprintPure`, `ResolveIcon` deliberatamente no |
| **I. Data-driven** | Sì, `UPrimaryDataAsset` editabile |
| **J. Dati reali** | **No. L'asset `DA_IconCatalog` non esiste nel repository.** Le entry esistono solo come fixture nei test |

Due dettagli di design che vale la pena non perdere nel passaggio a v0.2:

`ResolveIcon` **non restituisce mai un esito silenziosamente vuoto**: chiave sconosciuta dà il missing-icon con `bResolved = false` più una warning che nomina chiave e consumer. Ed è `BlueprintCallable` e non `BlueprintPure` per una ragione precisa — una funzione pura in un property binding viene valutata a ogni frame, e con un catalogo incompleto la diagnostica diventerebbe spam continuo proprio quando serve leggerla.

La categoria dichiarata e il segmento dentro l'ID **devono** dire la stessa cosa. Senza quel controllo `UI.Icon.Status.Wet` potrebbe dichiararsi `Warning` e nessuno se ne accorgerebbe finché un filtro per categoria non restituisce la lista sbagliata.

---

## 24 Glyph Mapping

Dati completi in `RT_IconRuntimeMapping_Audit_v0.1.csv`. Sintesi:

### EXACT — 7

`Glyph.Move` · `Glyph.Wait` · `Glyph.Guard` · `Glyph.Overwatch` → le quattro azioni core omonime.
`Glyph.Confirmed` · `Glyph.Predicted` · `Glyph.Uncertain` → i tre livelli di certezza.

Questi sette sono l'unico terreno solido. Notevole: la categoria Certainty combacia **perfettamente**, tre su tre, e non per caso — è l'unica categoria del runtime le cui chiavi sono state scritte a mano scegliendo cosa è disegnabile, cioè con lo stesso criterio con cui è stata costruita la v0.1.

### SEMANTIC_MATCH_NEEDS_APPROVAL — 6

| Glifo v0.1 | Candidato runtime | Perché serve approvazione |
| --- | --- | --- |
| `Glyph.Attack` | `UI.Icon.Action.BasicAttack` | Il runtime ha **tre** attacchi: Basic, Precision, Heavy. Un glifo generico ne copre uno o tutti tre? |
| `Glyph.Dash` | `UI.Icon.Action.Sprint` | `Action.Dash` **non è** fra le azioni core spedite, e `UI.Icon.Phase.Dash` esiste come *fase del turno*. Collisione di nome su due assi |
| `Glyph.Water` | `UI.Icon.Status.Wet` | `Status.Wet` è lo stato di un'**unità**, `Glyph.Water` quello di una **superficie**. Non sono la stessa informazione |
| `Glyph.Fire` | `UI.Icon.Status.Burning` | Stessa distinzione unità / superficie |
| `Glyph.Electric` | `UI.Icon.Status.Electrified` | Stessa distinzione. Esiste anche l'asset `RT_UI_Icon_Environment_Electric`, che nessuna chiave consuma |
| `Glyph.Reaction` | `UI.Icon.Action.Counter` | Il runtime esprime la reazione come **sei** azioni di fase Control: Counter, Deflect, Intercept, Anchor, Purge, Evade. Un glifo non copre sei azioni |

La distinzione unità/superficie è il punto più delicato dei sei. La v0.1 ha progettato Water, Fire ed Electric come **stati materiali del terreno**, con pattern screen-space dedicati; il runtime li conosce come **stati di un'unità**. Sono due letture diverse della stessa parola, e vanno separate prima dell'implementazione: probabilmente servono entrambe le chiavi.

### MISSING_RUNTIME_ID — 3

`Glyph.Height` · `Glyph.Waypoint` · `Glyph.Destination`

Waypoint e Destination sono il caso più netto: **`ERTIconCategory` non ha una categoria `Marker`**. Le dodici categorie dichiarate sono Identity, Action, Phase, Environment, MapInteraction, Status, Information, Reaction, Coordination, Certainty, Warning, Objective. Il namespace `Marker.*` della v0.1 non esiste nella tassonomia del codice.

### NO_RUNTIME_CONSUMER — 1

`Glyph.Cover`. `Action.CreateCover` esiste ed è richiesta, ma è **l'azione di creare copertura**, non lo stato del terreno che offre copertura. Significati diversi. L'asset `RT_UI_Icon_Environment_Cover` esiste già senza chiave che lo consumi.

### NOT_REQUIRED_BY_CURRENT_BUILD — 7

I sei `Warning.*` più `Glyph.LastContact`.

`ERTIconCategory::Warning` è **dichiarata e non popolata**, per scelta: "una categoria senza chiavi è legale — è la tassonomia; una CHIAVE senza asset non lo è". La v0.1 ha progettato il linguaggio dei warning prima che esistesse il sistema che lo consuma. Non è un errore su nessuno dei due lati, ma va registrato: quei sei glifi non hanno oggi nessuno che li chieda.

`Glyph.LastContact` ha un parente nella tassonomia — `ERTIconCategory::Information`, "cosa la squadra SA: visibile, sentito, ultima posizione nota, area approssimativa". È esattamente il namespace `IntelSource.*` della v0.1a, con un nome diverso. **La v0.1 dovrebbe adottare `Information`, non introdurre `Intel`.**

---

## Semantic Gaps

**A — runtime esistente + match esatto:** 7 (le 4 azioni omonime, i 3 livelli di certezza).

**B — runtime esistente, nome diverso:** 6 candidati semantici sopra, più una divergenza di tassonomia su due namespace interi: `Marker.*` della v0.1 non esiste nel codice, e `Intel.*` della v0.1 si chiama `Information` nel codice.

**C — glifo visuale senza semantica runtime:** 11 (3 MISSING_RUNTIME_ID + 1 NO_RUNTIME_CONSUMER + 7 NOT_REQUIRED). Sono `VISUAL_V0.1_NOT_YET_CONSUMED`.

**D — RUNTIME_REQUIRED_NOT_IN_VISUAL_V0.1: 48 chiavi.**

Questa è la voce che dimensiona il lavoro vero:

- **30 azioni** senza glifo v0.1: Interact, Dodge, Charge, Leap, Reposition, PrecisionAttack, HeavyAttack, LineAttack, CircularAoE, SuppressiveLine, MarkTarget, Deflect, Intercept, Anchor, Purge, Evade, Brace, Shield, Cleanse, Push, Pull, Root, Slow, Interrupt, Electrify, Ignite, CreateWater, ModifyArc, CreateCover, Heal

  `Sprint`, `Counter` e `BasicAttack` **non** sono in questa lista: hanno un candidato v0.1 e stanno fra i 6 SEMANTIC_MATCH_NEEDS_APPROVAL. Una chiave non può essere in entrambi i secchi.
- **4 fasi**: `Phase.Prep`, `Phase.Dash`, `Phase.Blast`, `Phase.Move`
- **8 stati** senza candidato: Root, Slow, Reveal, Exposed, Guarded, Marked, Braced, Obscured
- **6 identità**: i quattro eroi (Gadget, Phase, Riktor, Wraith) più Ally ed Enemy

**30 + 4 + 8 + 6 = 48.** E 7 esatte + 6 candidate + 48 = 61: le tre partizioni tornano.

**Non li aggiungo al visual set.** Il brief lo vieta e la ragione è buona: 48 icone nuove non sono un'estensione della v0.1, sono una v0.2 intera.

**E — asset senza consumer attuale:** vedi Legacy Audit.

---

## Legacy Audit

Due generazioni parallele di asset, entrambe presenti, entrambe con prefisso `RT_UI_`:

| Percorso | Contenuto | Generatore | Stato |
| --- | --- | --- | --- |
| `Content/Icons/Icons/` | **123 master SVG** + 604 PNG (16/20/24/32/48) | `tools/hud-assets/generate_hud_assets.py`, 2026-08-26 | set corrente, mai importato in Unreal |
| `docs/generated/icons/svg/` | **56 SVG** | `build-icon-assets.py`, **uscito dal repository** | generazione precedente, orfana |
| `Content/Icons/Frames/` | 9 cornici SVG | stesso generatore | chrome, non icone |

Il set da 123 dichiara la propria composizione: 122 icone più `MissingIcon`, di cui 61 sono le chiavi richieste e 61 sono le 20 ability del roster, `Action.Dodge` e le categorie che la v0.1 mostra ma il catalogo non dichiara.

**Confidenza del mapping verso la v0.1:**

- **EXACT — 7**: gli asset omonimi delle 7 chiavi che combaciano
- **LIKELY — 6**: gli asset dei candidati semantici (`Status_Wet`, `Status_Burning`, `Status_Electrified`, `Action_BasicAttack`, `Action_Sprint`, `Action_Counter`)
- **WEAK — 2**: `RT_UI_Icon_Environment_Cover` e `RT_UI_Icon_Environment_Electric` — nomi che combaciano con la v0.1 ma nessuna chiave richiesta li consuma
- **NONE — il resto**

Nessun mapping automatico è stato applicato ai casi LIKELY e WEAK.

**Le 56 SVG di `docs/generated/icons/svg/` sono orfane in senso forte**: il generatore che le ha prodotte non è più nel repository, quindi la loro geometria non è riproducibile. `AGENTS.md` lo dichiara: la geometria "era dichiarata dentro `build-icon-assets.py` ed è uscita con lui".

### Il conflitto di naming

| Fonte | Prescrive | Autorità |
| --- | --- | --- |
| File reali in `Content/Icons/Icons/` | `RT_UI_Icon_*` | stato di fatto |
| `docs/technical/tooling/brief-icone-v01.md` | `T_UI_Icon_*` | nominato dall'issue #219 |
| Fixture di `RTIconCatalogTests.cpp` | `T_UI_Icon_*` | test in CI |
| `progettazione-hud.md` §43 | `RT_UI_Icon_Overwatch` | documento owner |

**Raccomandazione: `T_UI_Icon_*` per le icone**, `RT_UI_*` per le cornici che sono chrome. Tre ragioni: il brief è più recente e più specifico, l'issue #219 lo nomina come autorità, e la fixture dei test già lo usa — cambiare direzione significherebbe cambiare anche il test.

Il costo del rename è basso e noto: una riga in `asset_name()` del generatore e una in `AssetNameForIcon` del commandlet. Ma **va deciso prima dell'import**: il commandlet trova la texture di una chiave derivandone il nome, quindi un prefisso diverso significa reimportare tutto.

---

## Missing Assets

Rispetto alle 61 chiavi richieste, gli asset esistono (il set da 123 le copre per dichiarazione) ma **nessuno è importato in Unreal**: non esistono `UTexture2D`, non esiste `DA_IconCatalog`, e il commandlet che li produrrebbe non è mai stato compilato.

Rispetto ai 24 glifi v0.1 di STEP 03: **nessun asset esportato**. La libreria Affinity di STEP 04 è specificata ma non costruita.

---

## Missing Runtime IDs

**3 su 24**, non 24 su 24 come stimava STEP 04: `Glyph.Height`, `Glyph.Waypoint`, `Glyph.Destination`.

A questi si aggiungono 7 glifi che hanno una categoria nella tassonomia ma nessuna chiave richiesta — che è una condizione diversa e va tenuta separata: l'ID non manca, manca il consumer.

---

## Consumers

| Consumer | Come chiede l'icona | Evidenza |
| --- | --- | --- |
| `URTActionSlotWidget` | `GetIconId()` → `MakeIconId(Action.ActionId)`, poi `ResolveIcon(..., TEXT("ActionSlot"))` | `RTScreenHudWidgets.cpp:266,279` |
| `URTScreenHudWidgetBase` | `GetIconCatalog()` risale l'albero fino a `URTTacticalHUDWidget` | `RTScreenHudWidgets.cpp:115–131` |
| `ARTHUD` | colora gli intenti per squadra leggendo `View.bIsAlly` (consumer di `Identity.Ally` / `Identity.Enemy`) | `RTHUD.cpp:285` |
| `URTHudViewModel` | espone `MakeIconId` come `BlueprintPure` | `RTHudViewModel.h:190` |
| Test in CI | 8 test in `RTIconCatalogTests.cpp` + 2 in `RTScreenHudWidgetTests.cpp` | — |

Il catalogo si raggiunge **risalendo l'albero dei widget**, non per riferimento diretto. Fuori dall'HUD non si raggiunge affatto, e questo è verificato da un test. Il campo sul widget figlio si chiama `ReceivedCatalog` e non `IconCatalog` per una ragione esplicita: con `IconCatalog` il getter Blueprint automatico diventava `Get IconCatalog`, indistinguibile a occhio da quello della radice.

---

## Risks

**R1 — La v0.1 potrebbe essere la tassonomia sbagliata.** Il codice ha dodici categorie dichiarate; la v0.1 usa Action, Environment, Reaction, Warning, Certainty, Marker, Intel. `Marker` non esiste nel codice, `Intel` si chiama `Information`. Procedere senza allineare i nomi crea una terza convenzione dopo `RT_UI_` e `T_UI_`.

**R2 — Le tre categorie ambientali sono ambigue.** Water, Fire ed Electric significano una cosa per la v0.1 (superficie) e un'altra per il runtime (stato di unità). Un'unica icona per entrambe è una decisione, non un dettaglio.

**R3 — Copertura 11%.** Il gioco pretende 61 chiavi; la v0.1 ne copre 7 con certezza. Il set da 123 SVG esistente le copre tutte. Importare la v0.1 al posto suo **peggiorerebbe** la copertura.

**R4 — Il commandlet non è mai stato eseguito.** L'intera pipeline di import è codice non compilato. Il primo tentativo reale scoprirà problemi che nessun documento può anticipare.

**R5 — Doppia generazione orfana.** 56 SVG in `docs/generated/` senza generatore. Se qualcuno le importa credendole correnti, il catalogo mescola due generazioni.

**R6 — `Action.Dodge`.** `LEGGIMI.md` la elenca fra le chiavi **non** richieste, ma `RTCatalogLibrary.cpp:1179` la registra come azione spedita, quindi `RequiredIconIds()` dovrebbe includerla. Una delle due fonti è disallineata: da verificare eseguendo la funzione, che è l'unico modo di risolverlo.

---

## Recommended Changes

Proposte, **non applicate**.

1. **Decidere il prefisso prima di qualunque import.** `T_UI_Icon_*` per le icone. Costo: due righe. Momento: adesso, non dopo.

2. **Allineare la tassonomia v0.1 a `ERTIconCategory`.** `Marker.*` → una categoria esistente o una nuova voce in coda all'enum; `Intel.*` → `Information`. La v0.1 è ancora un documento: cambiare un nome adesso costa una riga, dopo l'import costa un rename di asset.

3. **Sciogliere l'ambiguità Water / Fire / Electric** con due chiavi distinte per concetto: `Status.Wet` per l'unità, `Environment.Water` per la superficie. La seconda richiede di popolare la categoria `Environment`, che è dichiarata e vuota.

4. **Decidere se `Glyph.Attack` è uno o tre.** Il runtime ha già tre attacchi con fasi e priorità diverse. Un glifo unico è legittimo se il dock non deve distinguerli, e va detto esplicitamente.

5. **Non sostituire il set da 123 con i 24 della v0.1.** Trattare la v0.1 come la *grammatica* con cui il set esistente verrà rivisto, non come il set stesso. Sono compatibili: STEP 03 definisce le regole, i 123 asset sono la superficie da portare a norma.

6. **Verificare `Action.Dodge`** eseguendo `RequiredIconIds()` — è l'unica fonte che può dirimere R6.

7. **Compilare ed eseguire `RTBuildIconCatalog -DryRun`** prima di ogni altra cosa nel dominio Unreal. Finché quel comando non è mai girato, ogni stima sull'import è teorica.

---

## STEP 05 AUDIT STATUS

```
RUNTIME IDENTIFIER TYPE:            FName con prefisso "UI.Icon." (non FGameplayTag)
REQUIRED RUNTIME ICONS:             61
24 VISUAL GLYPHS EXACTLY MAPPED:    7 / 24
SEMANTIC MATCH NEEDS APPROVAL:      6
MISSING RUNTIME IDS:                3
NOT REQUIRED BY CURRENT BUILD:      7
NO RUNTIME CONSUMER:                1
MISSING CATALOG ENTRIES:            61 / 61  (DA_IconCatalog non esiste)
LEGACY ASSETS FOUND:                188  (123 Content/Icons + 56 docs/generated + 9 Frames)
ORPHAN LEGACY ASSETS:               56   (generatore non piu' nel repository)
RUNTIME_REQUIRED_NOT_IN_VISUAL_V01: 48
VISUAL_V01_NOT_YET_CONSUMED:        11
BLOCKERS:                           4
```

**I quattro blocker.**

**B1 — Prefisso non deciso.** Due documenti owner in contrasto. Blocca l'import, non l'export.

**B2 — Tassonomia divergente.** `Marker` e `Intel` della v0.1 non esistono in `ERTIconCategory`. Blocca la scrittura delle chiavi.

**B3 — `DA_IconCatalog` inesistente e commandlet non compilato.** Blocca ogni verifica reale della copertura.

**B4 — Scala.** 48 chiavi richieste senza glifo v0.1. Non blocca l'export dei 24, blocca l'idea che la v0.1 possa sostituire il set esistente.

Nessuno dei quattro si risolve con una decisione di design visuale: sono tutti decisioni di ownership sui dati.

---

**FERMO QUI.** Nessuna modifica implementata, nessun Runtime ID creato, nessuna voce di catalogo scritta, nessun asset importato.

Passo successivo proponibile: **STEP 06 — Runtime Mapping Implementation**, con precondizione che B1 e B2 siano decisi da chi possiede i documenti in conflitto.
