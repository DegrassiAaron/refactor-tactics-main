# Piano di migrazione del roster — `Flux`, `Riva`, `Bastion`, `Vektor` escono dal repository

> `CURRENT` · **Data**: 2026-08-13 · **Owner del piano**: questo documento
> **Consegue da**: [D-130](../decisions/RT_PDR_00_Decision_Log.md) — la decisione è presa, l'esecuzione no
> **Chiude**: [#716](https://github.com/DegrassiAaron/refactor-tactics-main/issues/716) · **Milestone**: post-v0.1
>
> Modellato su [`piano-migrazione-stable-id.md`](piano-migrazione-stable-id.md), che ha già eseguito la stessa
> forma di lavoro per gli `Action.*`. Le sue due lezioni valgono qui alla lettera: *«gli Stable ID si
> deprecano e si redirigono, non si rinominano»* — vera per gli `ActionId`, **falsa per gli `HeroId`**, e la
> §2 spiega perché — e *«il rischio non era "il corpus esiste", era "il corpus contiene l'ID che stai
> migrando": è una domanda a cui si risponde con un `grep`»*.

## 1. Stato misurato — 2026-08-13, su `525a72cd`

File versionati, esclusi `.uasset`/`.umap`. Conteggio per **occorrenze**, non per righe.

| Area | Occorrenze | File | Natura del lavoro |
|---|---:|---:|---|
| `docs/` vivi | 1600 | 106 | prosa: sostituzione, con rilettura |
| `Source/RefactorTactics/Tests/` | 1186 | 55 | identificatori, stringhe, nomi di test |
| `docs/archive/` | 1128 | 40 | prosa storica — **inclusa per D-130**, vedi §6 |
| `Scenarios/*.json` | 531 | 73 | dati versionati, diff leggibile |
| `Source/` (codice) | 367 | 30 | simboli C++, `FName`, commenti |
| altro | 182 | 19 | `.gitignore`, script, config |
| **totale** | **4994** | **~323** | |

```sh
# riproducibile
for n in Flux Riva Bastion Vektor; do printf "%-8s %s\n" "$n" "$(grep -roF "$n" Source/ docs/ Scenarios/ | wc -l)"; done
```

### File da **rinominare**, non solo da editare — 10

| File | Diventa |
|---|---|
| `Source/RefactorTactics/Tests/RTHeroFluxTests.cpp` | `RTHeroGadgetTests.cpp` |
| `Source/RefactorTactics/Tests/RTHeroRivaTests.cpp` | `RTHeroPhaseTests.cpp` |
| `Source/RefactorTactics/Tests/RTHeroBastionTests.cpp` | `RTHeroRiktorTests.cpp` |
| `Source/RefactorTactics/Tests/RTHeroVektorTests.cpp` | `RTHeroWraithTests.cpp` |
| `Scenarios/Combat/BastionImpactShotSlows.json` | `RiktorImpactShotSlows.json` |
| `Scenarios/Spec/Combat/BastionIsPushedLikeAnyone.json` | `RiktorIsPushedLikeAnyone.json` |
| `docs/wiki/…/10_Flux_scheda_Wiki.png` … `13_Vektor_…` | `10_Gadget_…` … `13_Wraith_…` (4 file) |

⚠️ Un rename di file scenario cambia anche **l'ID dello scenario** se l'harness lo deriva dal nome: da
verificare in `RTScenarioIndex.cpp` prima della fetta 4, non dopo.

### I 5 ID di test che sono **gate**

```
RefactorTactics.Heroes.Bastion.MatchesCatalog
RefactorTactics.Heroes.Bastion.KineticPanelVariantApplied
RefactorTactics.Heroes.Bastion.ReconfigureDoesNotDuplicate
RefactorTactics.Heroes.Bastion.ReconfigureRefusesInsteadOfGuessing
RefactorTactics.Heroes.Vektor.InterceptShotIsPredictive
```

Sono citati in `docs/roadmap/feature-registry.yaml`. **Un ID di test che cambia non fa fallire niente: fa
sparire un test dalla run**, mentre il registry continua a citare un nome che non esiste. Il gate non diventa
rosso — smette di esistere. Vanno cambiati **nello stesso commit** del registry.

## 2. Perché gli `HeroId` si rinominano e i token abilità no

È il discrimine che rende questo piano più corto di quanto #716 lasciasse credere, e non è un'opinione:

```
FRTTurnLogEntry:  Phase · Category · Outcome · SrcCell · TgtCell · Amount
                  ActionId (FName) · BaseActionId (FName)
                  UnitId (int32) · TurnNumber · GraphRevision · Priority
```

**Non c'è un `HeroId`.** L'unità entra nel formato su disco come intero.

| Token | Serializzato | Trattamento |
|---|---|---|
| `Hero.Flux` → `Hero.Gadget` | no | **rinomina**: nessun redirect, nessuna doppia verità, nessuna finestra di transizione |
| `Flux.ArcPulse` → `Hero.Gadget.ArcPulse` | **sì**, come `ActionId` | **redirect** in lettura via `ResolveLegacyActionId`; in scrittura il validator lo rifiuta |

La regola che tiene insieme le fette è quella di #199, invariata: **il redirect vale in lettura, mai in
scrittura**. Non esiste un momento in cui due ID sono entrambi autorevoli.

### Perché `Hero.<Nome>.<Abilità>` e non `<Nome>.<Abilità>`

`Gadget.ArcPulse` finirebbe accanto a `Gadget.Medkit`, e `Phase.FlowReaction` accanto a un sostantivo che il
turno usa 503 volte (`ERTMatchPhase` 367 + `ERTResolutionPhase` 136). Il prefisso `Hero.` toglie l'ambiguità
per costruzione, e **non costa nulla**: nessun punto del codice legge la struttura interna di un token
abilità. `ValidateActions` verifica `IsNone`, i duplicati e il redirect — mai la forma. I due soli split di
`Source/` (`ARTUnit::ShortHeroName`, `URTIconLibrary`) usano `FindLastChar` e operano su `HeroId`, quindi
`Hero.Gadget.ArcPulse` → `ArcPulse` funziona già oggi.

## 3. Il piano, a fette

Ogni fetta chiude con la suite verde. Nessuna fetta lascia il repository in uno stato con due nomi correnti
per la stessa cosa.

| # | Fetta | Tocca la serializzazione? | Dipende da | Gate |
|---:|---|---|---|---|
| 1 | **Rete di sicurezza**: `ResolveLegacyActionId` esteso ai 20 token; `ValidateActions` rifiuta un token legacy **dichiarato**, nominando l'erede | in lettura | — | `Catalog.ValidatorRejectsRetiredHeroAbilityId` (nasce **rosso**: i 20 token legacy sono ancora a catalogo) |
| 2 | **Prova di rilettura**: una traccia `.rttl` scritta col vocabolario vecchio si rilegge, l'ID resta scritto com'era, il catalogo risponde con l'erede, l'hash è riproducibile | sì | 1 | `TurnLog.RetiredHeroAbilityIdIsStillReadableFromDisk` |
| 3 | **`HeroId` + simboli C++**: `Hero.Flux` → `Hero.Gadget` e i quattro `MakeFlux`/`MakeRiva`/`MakeBastion`/`MakeVektor`; 4 file di test rinominati | no | 2 | `Unit.CanonicalHeroIdHasNoLegacyName` sostituisce `ShortHeroNameFromStableId` |
| 4 | **Token abilità a catalogo** → `Hero.<Nome>.<Abilità>`; scenari JSON e i 2 file scenario rinominati | no (il catalogo non è una traccia) | 3 | i 5 ID di test aggiornati **con** il registry, `feature_registry.py generate` **e** `shortlist` |
| 5 | **Documentazione viva** — 1600 occorrenze, 106 file | — | 4 | `check-docs-naming.py --check` **senza esenzioni** per i file vivi |
| 6 | **Archivio e citazioni datate** — 1128 occorrenze, 40 file; rimozione delle esenzioni «registri datati» dal gate | — | 5 | `check-docs-naming.py --check` verde con **zero** esenzioni |
| 7 | **Wiki** (repo separato): 4 PNG rinominati, pagine rigenerate da un checkout col registry aggiornato | — | 4 | `deploy --wiki-root` da albero allineato |

### Ordine, e perché non è negoziabile

Le fette 1-2 **non cambiano un solo nome**: costruiscono la rete che rende i nomi sostituibili senza rompere i
replay. Farle dopo significherebbe avere una finestra in cui una traccia scritta ieri non si rilegge — che è
esattamente il rischio che D-130 accetta di non correre.

La fetta 6 **non può precedere la 5**: finché il gate ha esenzioni non può dimostrare che i file vivi sono
puliti, e un gate che non può fallire non prova nulla.

## 4. Cosa **non** si tocca, e non è un'eccezione al perimetro

1. **I corpi di issue e PR già chiuse su GitHub.** Non sono file di questo repository. Restano a dire il nome vecchio, ed
   è la ragione per cui la §6 registra il costo invece di negarlo.
2. **I byte dentro le tracce `.rttl` già scritte.** È precisamente ciò che il redirect esiste per leggere:
   riscriverli sarebbe falsificare un replay, che è l'unica cosa che questo progetto tratta come prova.

Il corpus golden **non li contiene**: `Tests/Golden/Movement.Basic/turn-01.rttl` e `Movement.Collision/…`
sono 142 byte ciascuno e portano solo `RTTL` e `Action.Move`. Quindi i due test golden restano verdi **per
costruzione**, e ogni PR di questo piano lo dimostra col `grep` invece di dichiararlo:

```sh
python -c "import pathlib,re
for f in pathlib.Path('Source/RefactorTactics/Tests/Golden').rglob('*.rttl'):
    print(f, [s.decode() for s in re.findall(rb'[ -~]{4,}', f.read_bytes())])"
```

## 5. Rischio

**Basso sul codice, medio sulla prosa.** Non c'è un solo `.uasset` o `.umap` con un nome legacy fuori da
`Content/FabAsset` — il rischio più caro (rename di asset in Editor, non versionabile a mano) **non esiste**.
I due rischi reali sono:

1. **I 5 ID di test che sono gate** (§1). Mitigazione: cambiare test e registry nello stesso commit, e
   rigenerare `project-graph.json` con `generate` **e** `shortlist`.
2. **La sostituzione cieca in prosa.** `Flux` compare dentro parole e dentro citazioni; un `sed` globale su
   4994 occorrenze produrrebbe frasi che nessuno ha riletto. Mitigazione: fetta per fetta, con `git diff`
   letto, mai un solo commit da 323 file.

## 6. Il costo accettato, scritto per intero

D-130 estende il perimetro all'archivio. La conseguenza:

> Un handoff datato 2026-08-08 dirà «Gadget» mentre la issue GitHub che quel documento cita dice ancora
> il nome vecchio. **La provenienza smette di essere seguibile** — e questa è la ragione per cui
> [D-120](../decisions/RT_PDR_00_Decision_Log.md) vietava il search/replace globale.

La scelta è stata posta come domanda e decisa con la motivazione opposta: i nomi legacy non descrivono un
passato utile, descrivono un errore, e un archivio che li conserva lo ripropone a ogni lettura. Chi in futuro
troverà un rimando che non si segue deve poter leggere **qui** perché, invece di dedurre che qualcuno è stato
sbadato.

Un mitigante esiste e costa poco: la fetta 6 può lasciare in testa a ogni file di `docs/archive/` toccato una
riga sola — *«nomi del roster sostituiti il 2026-08-13 per D-130; le issue citate usano i nomi precedenti»* —
che non ripristina la provenienza ma dice a chi legge cosa cercare. Da decidere in quella fetta, non prima.

## 7. Definition of Done del piano

- [ ] `grep -roE "Flux|Riva|Bastion|Vektor" Source/ docs/ Scenarios/ | wc -l` → **0**
- [ ] `check-docs-naming.py --check` verde **con zero esenzioni** dichiarate nello script
- [ ] Suite verde, con il conto dei test **misurato sul branch**, non copiato da qui
- [ ] I 5 ID di test rinominati esistono con il nome nuovo e il registry li cita
- [ ] Una traccia `.rttl` scritta prima della migrazione si rilegge e produce lo stesso hash
- [ ] I due test golden verdi, con il `grep` di §4 nel corpo della PR
