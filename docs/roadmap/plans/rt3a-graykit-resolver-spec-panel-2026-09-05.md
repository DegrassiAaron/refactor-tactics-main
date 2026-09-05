# RT3-A GrayKit Presentation Resolver — revisione della specifica

> `CURRENT` · **Stato**: revisione chiusa · **mandato non eseguito, per decisione registrata in §6** · **Data**: 2026-09-05
> **HEAD della revisione**: misure aperte a `6f6a9997`, chiuse a **`853e92b1`** (`origin/main`) — il delta è dichiarato in §2
> **Aggiornato**: `b8ad0efb`, 2026-09-05 ~14:45Z — **§0**: il bersaglio di §6 è decaduto, la diagnosi regge
> **Panel**: Wiegers (lead) · Fowler · Nygard · Adzic · Crispin · Cockburn · Newman
> **Modo**: `critique` · **Perimetro**: la specifica RT3-A Wave 1. Nessun codice scritto, nessuna suite eseguita, nessun `.uasset` toccato.
> **Scade quando**: `main` avanza sulla serie ANIM, o una delle issue di §5 cambia stato.

---

## 0. Aggiornamento — questo referto è decaduto in un'ora, e sul punto che contava

Fra la stesura e il merge, **quattro issue della serie ANIM si sono chiuse**. Il §6 originale mandava la
lane su #2441: **è chiusa**. Il §5 le contava «cinque issue aperte»: ne restano cinque, ma non le stesse.

| Issue | Chiusa (UTC) | Portava |
|---|---|---|
| **#2447** · *ANIM CORE 3/4 · Binding e **resolver*** | **13:35:52** | 🔴 **il resolver** — chiuso **un'ora prima** che questa revisione fosse consegnata |
| #2445 · ANIM CORE 1/4 · catalogo, `AV_ID` stabile | 13:35:56 | il catalogo |
| #2446 · ANIM CORE 2/4 · lo scanner di Gadget | 13:35:58 | lo scan |
| **#2441** · il contratto `Role × Variant` | **14:34:12** | ⚠️ **83 secondi dopo il merge di questo documento** |

🔴 **#2447 è la scoperta che questa revisione ha mancato**, e il suo scope è RT3-A §6 e §3 `Q3` scritti
prima:

> *«**Resolver come funzione pura**, con esito **osservabile**: `SpecificVariant | GenericFallback |
> MissingPresentation`. Contratto: se `ActiveVariantId` esiste e risolve → variante specifica; altrimenti
> → fallback generico del `PresentationRole`.»* · *«⛔ **nessuna selezione automatica di un'altra
> variante. Mai.**»*

Funzione pura, esito strutturato, catena `specific → generic`, nessun first-wins. Ed esegue il
DUPLICATION GATE meglio del mandato che lo predicava: nomina `URTPresentationBindingLibrary`
([D-278](../../decisions/RT_PDR_00_Decision_Log.md), #1801) come owner adiacente e scrive perché sono
**due assi diversi** — `ERTResolvedEventType` (6 valori) → cue, contro `Action.*` (~30) → `Role` →
variante.

### Perché non l'avevo trovata — un difetto di metodo, non di fortuna

Le issue della serie sono state cercate con `gh issue list **--state open** --search …`. #2445, #2446 e
#2447 erano **già chiuse** al momento della ricerca, quindi il filtro le ha rese invisibili. ⚠️ **Cercare
l'owner di un design solo fra le issue aperte è cieco esattamente sul caso peggiore**: quello in cui il
lavoro è già stato consegnato. Il predicato corretto è `--state all`.

### Cosa cambia, e cosa no

| | |
|---|---|
| ✅ **La diagnosi regge, rafforzata** | non costruire un secondo resolver: adesso il primo non è più un design in una issue, è codice su `main` con test |
| ❌ **Il bersaglio di §6 decade** | #2441 e #2447 sono chiuse: non c'è più lavoro da assegnarvi |
| 🎯 **Il bersaglio corretto è [#2448](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2448)** | `v0.1` `P1`, OPEN, **sbloccata** dalla chiusura di #2447 da cui dipendeva |

**#2448 nomina il vero collo di bottiglia**, e lo dice meglio di come lo direbbe questo referto:

> *«Il `TurnManager` chiama `PlayAttackMontage()` **senza parametri**. Cioè: oggi la clip la sceglie il
> Blueprint. Un resolver che sceglie una variante può essere corretto, testato e completamente
> **inerte**, perché nessuno gli chiede niente.»*

---

## 1. Il verdetto in una riga

Il mandato RT3-A è **operativamente disciplinato e architetturalmente cieco**: write-set, anti-vacuità e authority
firewall sono di qualità alta, ma il DUPLICATION GATE che la specifica stessa impone al §1 ha **già una risposta**
nel repository, e la risposta è che il contratto `Role × Variant × Promotion` che RT3-A vuole costruire da zero è
già posseduto da una serie di issue in milestone `v0.1 · Leggibilità`, `P1`, create lo stesso giorno del mandato —
e che **il resolver stesso era già stato consegnato** un'ora prima di questa revisione (#2447, **§0**).

| | Voci |
|---|---:|
| 🔴 Critico | **3** |
| 🟠 Alto | **6** |
| 🟡 Medio | **5** |

**Esito**: RT3-A §4–§6 non si esegue. La lane viene riassegnata all'implementazione del contratto di
[#2441](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2441), che si dichiara esplicitamente il
produttore di quel contratto e blocca tre issue sorelle.

---

## 2. Stato misurato — e la base della Wave è già stantia

```bash
git rev-parse origin/main         # 8a530c6e  →  853e92b1   durante la revisione
git worktree list
```

| Fatto | Valore | Nota |
|---|---|---|
| `RT3_BASE_SHA` dichiarato dal mandato | `8a530c6e` | coincideva **esattamente** con `origin/main` all'apertura |
| `origin/main` a chiusura revisione | **`853e92b1`** | avanzato durante la sessione — la base della Wave 1 **non è più la punta** |
| Working tree principale all'apertura | `chore/rt-terminals-multi-istanza` @ `6f6a9997` | 3 commit avanti su BASE |
| Working tree principale a chiusura | `main` @ `853e92b1` | **il branch è cambiato sotto la revisione**, `D-222` |
| Modifica non attribuita | `M Source/RefactorTactics/Tests/RTHexEdgeGuardTests.cpp` | 🔴 è **dentro il write-set dichiarato** della lane A |
| Lane B | `D:/Repositories/rt-wt-rt3b` @ `8a530c6e`, branch `feat/rt3b-graykit-animation-feasibility` | in volo, sul dominio **animazioni** |

⚠️ **La STOP CONDITION del §0 era già attiva all'apertura**, e il mandato non prevede un ramo d'uscita: dice
«STOP. Non stashare, non resettare» e poi §13 pretende comunque `HEAD FINALE`, `TEST`, `PR`. Un esecutore letterale
si ferma prima della prima riga e non ha un formato in cui consegnare.

✅ Questa revisione è stata presa in un **worktree dedicato** (`D:/Repositories/rt-wt-rt3a`, branch
`docs/rt3a-graykit-resolver-spec-panel`, da `origin/main`), senza toccare l'albero condiviso né il file di
un'altra sessione.

---

## 3. `DG-2` — CP 25.2 è vivo, ed è di un'altra epic

`Source/RefactorTactics/UI/RTIconCatalogData.h:130` prenota la catena di fallback per chiave:

> *«Non è una catena di fallback per chiave: quella (`FallbackIconId` + controllo di ciclicità) è **CP 25.2**.»*

Il checkpoint è **[#267](https://github.com/DegrassiAaron/refactor-tactics-main/issues/267) · OPEN**, epic
[#265](https://github.com/DegrassiAaron/refactor-tactics-main/issues/265) `E25`, milestone `v0.2 · Struttura e
finestre`, label `post-v0.1` · `P2`. Il suo scope è testuale e vincolante:

> *«**Estensione del catalogo di #218, non un secondo catalogo.** Campi aggiunti solo quando hanno un consumer
> reale: `FallbackIconId`, `Priority`, `AccessibilityLabel`, …»*

⚠️ **I criteri di accettazione di #267 sono i requisiti di RT3-A §7 e §9, scritti prima:**

| #267 | RT3-A |
|---|---|
| «Il sort è `Priority` → `IconId` stabile. **Mai** l'ordine di `TMap`/`TSet`» | §7 determinismo |
| «Due icone con la stessa `Priority` hanno un tie-break stabile e riproducibile fra run» | §7 ambiguità |
| «Un catalogo con `IconId` duplicato non passa la validazione» | §9 `PresentationCatalog.RejectsDuplicateStableId` |
| «Nessun campo viene aggiunto *perché il modello lo prevede*» | §5 |
| test: `IconCatalog.DuplicateIdIsValidationError` · `FallbackDoesNotCycle` · `PriorityOrderIsDeterministic` | §9, con altri nomi |

**Conclusione `DG-2`**: CP 25.2 è vivo, riguarda **solo il Channel Icon**, non è il parent di RT3-A, e RT3-A non
deve toccarlo.

🔍 **Nota di metodo, perché è costata un finding sbagliato.** `gh issue list --search "CP 25.2 fallback icona"`
restituisce `[]`; `--search "CP 25"` restituisce #267 al secondo posto. La search di GitHub tokenizza, e una query
più specifica può essere *meno* capace di trovare. Il primo passaggio di questa revisione aveva concluso «owner
invisibile a `gh`»: era falso, ed è lo stesso difetto già registrato per la misura delle issue.

---

## 4. `DG-1` — la mappa dei channel decide da sola

| Channel | Owner | Stato misurato |
|---|---|---|
| **Icon** | `URTIconCatalogData` + `URTIconLibrary::ResolveIcon` — [D-031](../../decisions/RT_PDR_00_Decision_Log.md) **Consolidata** | 🔴 **occupato** · due consumer in produzione (`RTScreenHudWidgets.cpp:283`, `RTUnitOverlayWidget.cpp:131`) · esteso da #267 |
| **Animation** | `URTUnitAnimInstance::ClipsPerHero` + [#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288) | 🔴 **occupato** · `v0.1` `P1` · decisione presa su un numero: C++ e non `ABP_*`, 2,8 MB contro gli 0,7 MB di tutto `Content/` |
| **VFX** | — | libero, ma **vietato da RT3-A §11** |
| **SFX** | — | libero, ma **vietato da RT3-A §11** |
| **Projectile** | — | libero, ma **vietato da RT3-A §11** |

**FOWLER**: il resolver multi-channel di Wave 1 **non ha un solo channel su cui possa legittimamente operare**. I
due che hanno consumer sono occupati; i tre liberi sono esclusi dal mandato stesso.

⚠️ **E i due owner hanno politiche di missing opposte, entrambe motivate per iscritto.** `URTIconCatalogData`
pretende una `MissingIcon` e il validator rifiuta un catalogo che non ce l'ha — *«senza di lei `ResolveIcon` non
avrebbe niente da restituire»*. `URTUnitAnimInstance` fa l'opposto: *«un eroe senza voce non è un errore»*, e
l'unità resta in posa di riferimento. Un resolver unico dovrebbe imporne **una sola**, e nessuna delle due è
sbagliata nel proprio dominio.

---

## 5. Il fatto che chiude il caso — `ANIM LAB` esisteva già

Le issue della serie, milestone `v0.1 · Leggibilità`, `P1`, generate da `/implement-feature ANIM LAB` il
**2026-09-05**, cioè lo stesso giorno del mandato RT3-A — **gli stati sono quelli di §0, non quelli
misurati durante la stesura**:

| Issue | Stato | Titolo | Cosa possiede |
|---|---|---|---|
| **#2445** | ✅ chiusa | ANIM CORE 1/4 · catalogo, `AV_ID` stabile | il catalogo |
| **#2446** | ✅ chiusa | ANIM CORE 2/4 · lo scanner di Gadget | lo scan |
| **#2447** | ✅ chiusa | ANIM CORE 3/4 · **Binding e resolver** | 🔴 **il resolver** — vedi §0 |
| **#2441** | ✅ chiusa | *ClipsPerHero si allarga a eroe × ruolo × variante* | **il contratto** `Role` + `Variant` |
| **#2442** | ⏳ aperta | *Centotredici file non sono centotredici animazioni…* | lo scan, e il gate di cook per ruolo |
| **#2443** | ⏳ aperta | *Anim Browser: … si promuovono a mano…* | **`PromotionState`** |
| **#2444** | ⏳ aperta | VALIDATION_EDITOR · golden flow su Gadget | la validazione lato Editor |
| **#2448** | 🎯 aperta | *ANIM LAB · sette ruoli su nove non hanno un consumatore* | **il canale**: senza, il resolver è inerte |
| **#2450** | ⏳ aperta | *I dodici montaggi `AM_<Pack>_{Attack,Hit,Death}` non esistono* | il contenuto, di #288 |

**#2441 si dichiara il produttore del contratto**: *«questa issue **produce il contratto** contro cui le altre
compilano»* · *«**Blocca**: #2442, #2443, #2444»*. E lo porta già scritto:

```cpp
UENUM(BlueprintType)
enum class ERTPresentationRole : uint8 { Idle, Move, Attack, Cast, Dash, Defend, Hit, Death, Fall };

USTRUCT(BlueprintType)
struct FRTAnimVariant
{
    FName                             VariantId;  // AV_0042 — stabile, assegnato dallo SCAN
    FName                             Label;      // "Heavy" | "A" — naming locale, mai giudizio
    TSoftObjectPtr<UAnimSequenceBase> Clip;
};
```

🔴 **Il §4 del mandato è smentito da questa riga.** RT3-A dice di non hardcodare `Attack · Cast · Dash · Defend ·
Move · Death · HitReact` «come se la tassonomia fosse già approvata — la lane B sta misurando proprio questo
problema». La tassonomia **è approvata**: nove valori, in una issue `P1` `v0.1`. E `Label` porta già gli stessi
esempi che RT3-A §4 usa per la Variant — `"A"` e `"Heavy"`.

🔴 **E `Promoted` è già definito, quasi parola per parola.** #2443: *«`Promoted` significa «l'autore l'ha guardata
e la ritiene abbastanza buona da poter essere usata», e non significa shipping, active, cook o scelta
definitiva»*, difeso da un vincolo che RT3-A non ha: *«**Nessun percorso di codice** può scrivere `Promoted`»*,
con **una funzione sola** che scrive lo stato e un test che la difende.

✅ **E `Q2` del mandato ha già risposta.** Decisione d'autore del 2026-09-05 in #2441: *«il binding clip→ruolo
resta **sul CDO di `URTUnitAnimInstance`**, non in un asset nuovo. Zero byte binari aggiunti.»* Il persistence gate
che RT3-A §3 chiede di *segnalare senza inventare* era già chiuso quando il mandato è stato scritto.

---

## 6. La decisione

**RT3-A Wave 1 §4–§6 non si esegue.**

> ⏱️ **Il bersaglio è cambiato dopo la stesura — vale §0.** Questa sezione mandava la lane a *implementare*
> il contratto di **#2441**; #2441 e #2447 si sono chiuse nel frattempo e quel contratto **è già su
> `main`**. Il bersaglio corrente è **#2448**, che apre il canale con cui il resolver esistente smette di
> essere inerte. La decisione sotto — non costruirne un secondo — resta valida e si rafforza.

Le ragioni, in ordine di peso:

1. **Un secondo contratto sarebbe il duplicato più costoso possibile**: due tassonomie di ruolo, due nozioni di
   variante, due nozioni di promozione, su un dominio dove #2441 blocca già tre issue.
2. **RT3-A e RT3-B stavano per collidere sul modello, non sui file.** La lane B è
   `feat/rt3b-graykit-animation-feasibility`: la RT3 PARALLEL POLICY protegge il write-set e non il contratto.
3. **Il resolver `specific → GrayKit → generic` resta desiderabile**, ma è un *secondo passo dentro* il contratto
   di #2441 — un livello di fallback su `PerRole`/`Variants` — non un sottosistema parallelo con cinque channel e
   zero consumer.

⛔ Quel che **non** viene deciso qui: se il fallback GrayKit per Animation debba nascere come issue nuova sotto
#288 o come estensione di #2441. È una scelta di scomposizione, appartiene all'owner di `ANIM LAB`, e questa
revisione non ha titolo per prenderla.

---

## 7. I difetti della specifica che restano validi

Valgono per chiunque riusi il mandato RT3-A come modello, indipendentemente dalla riassegnazione.

### 🟠 A1 — `Ambiguous` ferma la catena o cade a `Generic`?

**ADZIC**: §3 `Q3` dice «nessuna candidate → passa al livello successivo»; §6 dice che due candidate
equally-valid danno `Ambiguous`. Manca l'unica riga che conta: *e allora `Ambiguous` scende a `Generic`, o si
ferma?* Sono due sistemi diversi con lo stesso diagramma, e **nessuno degli otto test del §9 li distingue** —
`UsesGenericWhenRoleMissing` copre il caso *zero* candidate, non il caso *due*.

### 🟠 A2 — `Source` mescola provenienza ed esito

**FOWLER**: `Specific | GrayKit | Generic | Missing | Ambiguous`. I primi tre dicono *da dove viene l'asset*, gli
ultimi due dicono *che non c'è un asset*. Il repository ha già risolto lo stesso problema, meglio:
`FRTIconResolution` separa `Asset` (sempre usabile) da `bResolved`, con la motivazione scritta — *«il chiamante non
deve dedurre il fallimento da un puntatore nullo»*.

### 🟠 A3 — «assente/invalido» non è decidibile senza violare il §7

**NYGARD**: §6 fa cadere lo Specific quando è «assente **o invalido**». Con un `TSoftObjectPtr` non caricato il
resolver sa se il path è *nullo*, non se l'asset *esiste*. Per saperlo servirebbe l'AssetRegistry — che §7 vieta
nominatamente. Contraddizione interna: o «invalido» significa solo `IsNull()`, e va scritto, o la validazione è a
monte e il resolver non la fa.

### 🟠 A4 — `ValidationState` è un campo senza produttore

**WIEGERS**: §5 lo elenca fra i campi minimi e due righe dopo vieta i campi «perché potrebbero servire». §11 non
elenca il validatore fra le cose da implementare. Chi lo calcola, e con quale oracolo, se `A3` dice che il resolver
non può guardare il disco?

### 🟠 A5 — il mutex Unreal è globale: le tre lane non sono parallele sul gate

[`AGENTS.md`](../../../AGENTS.md) §11, testuale: *«Un worktree separato **non elimina il mutex globale
Unreal/Live Coding**.»* Tre terminali che eseguono ciascuno `rt-suite.ps1 -WaitMinutes 40` sono una coda, non un
parallelo. Il mandato presenta il parallelismo come dato e non dichiara mai la contesa, né chi ha la precedenza.

### 🟠 A6 — `DoesNotMutateCatalog` è vacuo per costruzione

**CRISPIN**: con firma `const FCatalog&` il compilatore garantisce l'immutabilità e il test **non può diventare
rosso**. È il «test cosmetico» che §9 vieta, nella stessa lista in cui lo chiede. L'invariante che protegge
davvero è un'altra: *due `Resolve` consecutivi sullo stesso catalogo danno lo stesso risultato* — che copre anche
la cache lazy che qualcuno aggiungerà.

### 🟡 Medi

- **M1** — il determinismo del §7 richiede un ordine totale su `(Key, Variant, StableId)` mai specificato. Il
  pattern esiste già: `RTIconLibrary.cpp` ordina con `LexicalLess` perché *«l'ordine dei tag restituiti dal manager
  non è garantito»*.
- **M2** — `D-222 verdict` è preteso dal §13 e mai definito nel mandato: requisito non tracciabile.
- **M3** — «filtro minimo appropriato» non è specificato, e i nomi proposti (`PresentationResolver.X`) non portano
  il prefisso `RefactorTactics.` con cui `-Filter` seleziona.
- **M4** — §10 concede di scrivere documentazione; `AGENTS.md` §9 invalida una misura il cui working tree cambia
  durante la run. Scrivere in `docs/` mentre la suite gira produce **NON VALIDA con zero fallimenti**. Il mandato
  deve prescrivere l'ordine: documenta → committa → misura.
- **M5** — **COCKBURN**: §11 vieta ogni consumer, quindi il contratto resta validato solo dai propri test. La
  diagnosi esiste già in questo repository, sullo stesso kit —
  [`graykit-asset-roadmap-v10`](graykit-asset-roadmap-v10-spec-panel-2026-08-30.md) §2.4: *«il kit non ha consumer,
  e questo precede ogni altro nodo»*.

### ✅ Cosa la specifica fa bene

Non è un mandato debole, e va detto con la stessa precisione: §7 tratta il determinismo come requisito strutturale
e sceglie `Ambiguous` invece del first-wins, che è la scelta corretta e non quella comoda; §8 elenca nominalmente i
campi che il firewall vieta; §9 impone la **mutazione manuale** per i test discriminanti; §10 nomina i due file di
RT3-C invece di sottintenderli; §11 elenca quindici non-obiettivi; §2 vieta di auto-assegnare un `D-nnn`.

---

## 8. NOT RUN

- ❌ **Nessuna build** — Game né Editor.
- ❌ **Nessuna suite** — `rt-suite.ps1` non è stata avviata. Nessun verdetto `VALIDA` / `NON VALIDA` è disponibile,
  e nessuno è preteso: questa revisione non ha modificato codice.
- ❌ **Nessun PIE, nessun Editor aperto**, nessun `.uasset` letto o scritto.
- ❌ **Nessuna issue creata, chiusa o commentata**, nessun `D-nnn` assegnato, nessuna PR aperta.

Le misure sono `git`, `gh` e `grep` su `6f6a9997` → `853e92b1`, con il delta dichiarato in §2.

---

## 9. Richieste di integrazione

### Per RT3-B — ✅ nessuna collisione: ha consegnato

L'avvertimento originale era: *«verificare #2441 prima di produrre una tassonomia di ruolo — se B propone un
enum, il conflitto arriva al merge invece che alla revisione»*. **Non si è materializzato.** B ha consegnato
un referto e non un enum ([PR #2469](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2469),
mergiata alle **14:32:58Z**, nove secondi dopo questa) —
[`rt3b-graykit-animation-feasibility-2026-09-05.md`](rt3b-graykit-animation-feasibility-2026-09-05.md).
`git grep "enum class ERT.*Role"` su `main` dà **un solo file**: `RTUnitAnimInstance.h`.

🔑 **E le due lane convergono da lati opposti.** B risponde `SI, MA` e il `MA` è *«una decisione già presa
che nessuno ha ancora registrato»* — il rig di fallback, deciso chiudendo #2449, senza `D-nnn` e con due
gate aperti (costo binario, licenza). A risponde *«il resolver esiste già»*. Due mandati indipendenti, la
stessa forma di risposta: **il lavoro della Wave 1 era in gran parte già fatto o già deciso, e nessuno dei
due mandati lo sapeva.**

### Per RT3-C — `docs/technical/test-manuali-pie.md` · `docs/roadmap/editor-sessions.yaml`

Nessuna richiesta. Questa revisione non ha toccato i due file, e non produce voci PIE: il verdetto è documentale e
non ha un oracolo a schermo.

### Per il coordinamento della Wave

⚠️ **`RT3_BASE_SHA` è stantio.** Il mandato congela la Wave su `8a530c6e`, che era `origin/main` all'apertura e
non lo è più (`853e92b1`). Una Wave che vieta l'aggiornamento va ri-basata o va dichiarata la finestra in cui il
divieto vale.

---

## 10. Cosa questo documento NON fa

- ⛔ non esegue il mandato RT3-A, e non ne salva le parti eseguibili: la riassegnazione è totale;
- ⛔ non implementa nulla della serie ANIM — registra dove va la lane, non il lavoro;
- ⛔ non riapre #2441, #2445, #2446 né #2447: sono chiuse `COMPLETED`, e §0 le registra, non le contesta;
- ⛔ non decide se il fallback GrayKit per Animation sia una issue nuova o un'estensione di #2441 (§6);
- ⛔ non tocca `URTIconCatalogData` né #267: il Channel Icon ha owner, ed è `E25`;
- ⛔ non crea una Epic, non assegna un `D-nnn`, non modifica il Decision Log;
- ⛔ non apre né chiude issue: le cinque di `ANIM LAB` sono citate, non modificate.
