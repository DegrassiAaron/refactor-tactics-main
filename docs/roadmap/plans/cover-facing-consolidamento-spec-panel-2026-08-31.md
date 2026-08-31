# Cover & Facing — referto di consolidamento

> `CURRENT` · **Tipo**: referto di audit · **Data**: 2026-08-31 · **Misurato su**: `origin/main` `0b179ff6`
> **Owner**: nessuno — questo documento **non è** una sede normativa. Ogni regola qui citata appartiene
> all'owner indicato nella colonna *Owner*; se una riga di questo referto e il suo owner divergono,
> **vince l'owner**.
>
> **Mandato**: consolidare Cover e Facing verificando prima lo stato reale di codice, test, documentazione,
> roadmap e backlog GitHub.
>
> **Esito in una riga**: il perimetro **non era greenfield e non era disallineato**. Le due epic sono chiuse
> (`E16` [#175](https://github.com/DegrassiAaron/refactor-tactics-main/issues/175),
> `E9` [#23](https://github.com/DegrassiAaron/refactor-tactics-main/issues/23)), il lavoro residuo era **già
> tracciato issue per issue**, e il consolidamento si è ridotto a **due correzioni documentali** e a nessuna
> nuova issue.

## 1. Perché esiste

Un mandato di consolidamento presuppone una deriva. Qui la deriva **non c'era**, e il referto serve a
registrare *come è stato misurato* — perché la prossima richiesta di consolidamento su Cover/Facing possa
partire da questa misura invece di rifarla.

Vale la stessa disciplina di [`facing-visualdocs-triage-2026-08-13.md`](facing-visualdocs-triage-2026-08-13.md):
il referto di un audit che non trova niente è un risultato, non uno spreco.

## 2. Misura

| Voce | Valore |
|---|---|
| Ref misurato | `origin/main` `0b179ff6` |
| Unreal Engine | `5.8` (`RefactorTactics.uproject`, `EngineAssociation`) — `5.8.1` in [`AGENTS.md`](../../../AGENTS.md) §1 |
| Metodo | `git grep`/`git show` contro il ref remoto, **senza** working tree locale |

⚠️ **Perché contro il ref e non contro il checkout**: al momento dell'audit il clone primario era 30 commit
indietro rispetto a `origin/main` e portava lavoro staged di un'altra sessione. Misurare lì avrebbe prodotto
un audit vero di uno stato che nessuno sta per mergiare.

## 3. Le due primitive sono separate, e lo sono per costruzione

Il vincolo che il mandato chiedeva di **verificare** — «la Cover ambientale non dipende dal Facing
dell'unità» — non è una proprietà da testare: è una proprietà della **firma**.

```
URTHexCoverLibrary::CoverBetween(Map, From, To)                        // geometria: non prende un facing
URTHexCombatLibrary::IsInFrontalArc(DefenderCell, Facing, OriginCell)  // orientamento: non prende una mappa
URTHexCombatLibrary::EffectiveCoverReduction(Map, Attacker, Target, Shape)  // il solo punto che li COMPONE
```

Un muretto non può ruotare con chi ci sta dietro perché la funzione che lo legge **non ha un parametro
`Facing` da cui dipendere**. Il commento di `FRTHexCover` lo dichiara già: «la direzionalità è del BORDO, non
dell'unità: girarsi non sposta un muretto».

∴ **non si è aggiunto un test di invarianza**: sarebbe stato vacuo — asserirebbe che una funzione non dipende
da un argomento che non riceve. Ciò che invece **può** regredire è la composizione, ed è già coperta:
`Cover.AddCover.RearHitBypassesRuntimeCover` chiama la stessa scena con due facing e ne asserisce **due esiti
diversi**.

### Il confine 6 / 12 regge

[D-243](../../decisions/RT_PDR_00_Decision_Log.md) è esplicita: i **dodici spicchi sono presentazione**, e
ciò che entra nella regola è il **facing a sei** che ne deriva (`EdgeIndex = SectorIndex / 2`). Nessuna
pressione a portare le unità a dodici direzioni, e nessuna sede viva che lo suggerisca.

## 4. Stato per concetto

| Concetto | Owner | Code | Tests | State |
|---|---|---|---|---|
| Facing core (6 vie) | [ADR-0005](../../decisions/adr-0005-orientamento.md) · [ADR-0008](../../decisions/adr-0008-rotazione-e-policy-di-facing.md) | `ERTHexDirection` (6 valori), `FRTHexSimUnit::Facing` | 13 `Facing.*` | **IMPLEMENTED** |
| Timeline intra-round | [D-020](../../decisions/RT_PDR_00_Decision_Log.md) | `URTFacingLibrary` | 11 scenari `Spec.Facing.*` | **IMPLEMENTED** |
| Move → Facing | ADR-0005 §1, ADR-0008 §2 | `FacingFromPath`, `LegalFacings` | `Spec.Facing.DerivesFromMove`, `…TurningPathUsesLastCompletedStep` | **IMPLEMENTED** |
| Pivot (budget per eroe) | ADR-0008 §1 | — | — | **MISSING** → [#1605](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1605) |
| Displacement → Facing | ADR-0008 §3 | `FacingAfterDisplacement`, `ERTDisplacementCause` | `RTFacingTests` | **IMPLEMENTED** |
| Relazione a 6 lati relativi | [D-126](../../decisions/RT_PDR_00_Decision_Log.md) | **zero** occorrenze in `Source/` | — | **MISSING** → [#726](https://github.com/DegrassiAaron/refactor-tactics-main/issues/726) |
| Environmental Cover | [`spec-copertura-cp91.md`](../../gameplay/spec-copertura-cp91.md) · [`spec-copertura-alta-cp92.md`](../../gameplay/spec-copertura-alta-cp92.md) | `URTHexCoverLibrary` | 21 `Cover.*` | **IMPLEMENTED** |
| Cover ⊕ Facing (combat) | CP 16.2 | `EffectiveCoverReduction` | `Combat.BackAttackIgnoresCover`, `Combat.FlankAttackKeepsCover` | **IMPLEMENTED** |
| Guard direzionale | [D-206](../../decisions/RT_PDR_00_Decision_Log.md) → [D-292](../../decisions/RT_PDR_00_Decision_Log.md) | `RTTurnManager` (pool frontale) | `Combat.BackAttackIgnoresGuard` | **IMPLEMENTED**, corpus in corso → [#1919](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1919) |
| Brace | `FAC-3` **aperta** | non direzionale per decisione | `Combat.ShieldWorksFromAnyDirection` | **CURRENT by decision** |
| Overwatch ← Facing | [D-020](../../decisions/RT_PDR_00_Decision_Log.md), E14 | `ReadFacingForConsumer` senza produttori in gioco | 2 test, **0** chiamanti | **PARTIAL** → [#1933](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1933) |
| LOS / Trajectory | `URTHexVisionLibrary`, `URTHexPathLibrary` | servizi **separati**, consumano `BlocksTraversal` | — | **IMPLEMENTED** |
| Snapshot | ADR-0005 §5 | `FRTHexSimUnit::Facing` | `RTFacingTests` | **IMPLEMENTED** |
| Hash **della traccia** | [D-067](../../decisions/RT_PDR_00_Decision_Log.md) | `URTTurnLogLibrary::HashTurnLog` | `RTTurnLogCauseTests` | **IMPLEMENTED** |
| Hash **di stato** | [D-261](../../decisions/RT_PDR_00_Decision_Log.md) | `FRTUnitStateDigest` — **7 campi, nessuno è `Facing`** | — | **MISSING** → [#1800](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1800) |
| TurnLog | [`spec-turnlog.md`](../../technical/architecture/spec-turnlog.md) | `ERTFacingOutcome` (11 valori) | `RTTurnLogCauseTests` | **IMPLEMENTED** |
| Debug | — | `rt.Debug.DrawCover`, `RTDebugReportLibrary` (`facing=`, `cover[…]`) | `RTDebugConsoleTests` | **IMPLEMENTED** |
| Privacy | [`AGENTS.md`](../../../AGENTS.md) §4 | il facing è stato **pubblico**: nessun intento privato coinvolto | `RTTeamKnowledgeTests` | **IMPLEMENTED** |

> 🔁 **Corretto il 2026-08-31 (secondo passaggio), riga `Guard direzionale`**: l'owner citato era `D-289`, che è la posa e la copertura intra-hex. Il pool frontale è [`D-292`](../../decisions/RT_PDR_00_Decision_Log.md) — *«la `Guard` … diventa un POOL di 15 danni assorbibili, che solo i colpi dell'arco FRONTALE consumano»* —, consegnata da [#1909](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1909) (chiusa). ⚠️ La misura della riga non cambia: cambia il numero che la possiede.

## 5. Conflitti trovati — due, entrambi documentali

### 5.1 «Facing … e hash»: una parola per due hash

| | |
|---|---|
| **Sedi** | [`roadmap-v0.1.md`](../roadmap-v0.1.md), riga *Facing come stato di gioco* e riga *E16* |
| **Conflitto** | dichiaravano il facing «in snapshot, TurnLog e **hash**» mentre `FRTUnitStateDigest` lo **omette** |
| **Perché non era una bugia** | il progetto ha **due** hash: `HashTurnLog` (traccia) e `RTMatchStateHash` (stato). Nel primo il facing c'è — [ADR-0005 §5](../../decisions/adr-0005-orientamento.md) dice «hash del replay» ed è **corretta**. Nel secondo no |
| **Costo dell'ambiguità** | la parola non qualificata ha reso il difetto **invisibile alla lettura** fino a [D-261](../../decisions/RT_PDR_00_Decision_Log.md) (2026-08-30) |
| **Azione** | disambiguato in `roadmap-v0.1.md` (**hash della traccia**) con puntatore a D-261/[#1800](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1800). ADR-0005 **non toccata**: era già vera |

### 5.2 `FAC-11` cita una sede uscita dal repository

| | |
|---|---|
| **Sede** | [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), riga `FAC-11` |
| **Conflitto** | lo sweep di [D-147](../../decisions/RT_PDR_00_Decision_Log.md) elencava «tutte e sette le sedi vive» fra cui `feature-registry.yaml` e la sua vista `.json` — **rimossi** da [D-181](../../decisions/RT_PDR_00_Decision_Log.md) il 2026-08-21 |
| **Azione** | la voce è barrata e annotata: le sedi vive sono **sei**. Lo sweep del 2026-08-16 resta vero *alla sua data* |

⚠️ **Fuori perimetro, registrato e non corretto**: altre quattro sedi vive citano `feature-registry.yaml`
come destinazione ancora risolvibile — `gameplay/brief-super-e-cooldown.md`,
`gameplay/spec-decision-time-bank.md`, `gameplay/spec-interazioni-mappa-cp101.md`,
`gameplay/spec-tassonomia-movimento.md`. Non sono Cover/Facing e non si toccano da qui: è lavoro di D-181,
non di questo mandato.

## 6. Cosa **non** si è fatto, e perché

| Non fatto | Ragione misurata |
|---|---|
| Nessuna **Epic** nuova | `E16` e `E9` sono chiuse e il residuo vive già su `E12`/`E11`/`E14`. [D-126](../../decisions/RT_PDR_00_Decision_Log.md) lo vieta esplicitamente: «nessuna seconda epic Facing accanto a E16 già chiusa» |
| Nessuna **issue** nuova | ognuno dei gap misurati aveva già la sua: #1800, #1933, #726, #1605, #1919 |
| Nessun **Feature Registry** | uscito dal repository con [D-181](../../decisions/RT_PDR_00_Decision_Log.md). Ricrearlo sarebbe una seconda tassonomia contro una decisione `Consolidata` |
| Nessun test di **invarianza** Cover↔Facing | vacuo per firma — vedi §3 |
| Nessuna **decisione di design** chiusa | `FAC-3`, `FAC-5`…`FAC-9`, `FAC-12`…`FAC-14` ([#339](https://github.com/DegrassiAaron/refactor-tactics-main/issues/339)) e `COV-1`…`COV-8` ([#1833](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1833)) restano **OPEN**: sono scelte d'autore, non lacune di consolidamento |
| Nessun tocco alla **roadmap** oltre la disambiguazione | il runtime **non** è più avanti della roadmap: le due si corrispondono |

## 7. Prossimo passo

[**#1800** — *il Facing entra in `FRTUnitStateDigest` e nel checksum di stato*](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1800).

È l'unico gap `P1` su `v0.1 · Gate di release`, ed è la **dipendenza a monte** degli altri: finché due stati
che differiscono solo per orientamento producono lo stesso digest, ogni golden test costruito sopra —
compresi quelli che #726 e #1933 produrranno — confronta un checksum che **non discrimina** la cosa che
stanno verificando.
