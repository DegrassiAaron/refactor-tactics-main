# HANDOFF CHATGPT → CLAUDE CODE — sincronizzazione governance

> `HISTORICAL` · **Materiale NON autorevole** · archiviato il **2026-08-31**
>
> **Origine**: sessione ChatGPT cancellata dall'autore, consegnata a Claude Code come istruzione operativa.
> **Repository**: `DegrassiAaron/refactor-tactics-main`, ramo canonico `main`.
> **Fonte di governance citata**: Google Sheet *RT — Knowledge Index & Consolidation Log*
> (`1GOd_Hi3bZBM0NMXQ7oAKV8XxlzPdywtpgCjWtsZsoeo`), tab `Open Questions`.
>
> ⛔ **Questo file non è una fonte canonica e non sostituisce il
> [Decision Log](../../../decisions/RT_PDR_00_Decision_Log.md).** Conserva il *contesto* di una chat che
> stava per sparire; le decisioni che ne sono uscite vivono nel Decision Log, e dove le due divergono
> prevale il Decision Log.
>
> 🔎 **Esito della sincronizzazione**: referto in
> [`../../../roadmap/plans/sync-governance-dqa-026-030-2026-08-31.md`](../../../roadmap/plans/sync-governance-dqa-026-030-2026-08-31.md),
> decisione [`D-292`](../../../decisions/RT_PDR_00_Decision_Log.md).

---

## 1. Perché questo handoff esiste

La chat ChatGPT aveva letto il Drive, raccolto cinque decisioni d'autore e **tentato di scriverle su
GitHub**. Il tentativo **non ha prodotto nulla**:

- nessun file scritto;
- nessun branch creato;
- nessuna issue aperta;
- GitHub ha risposto **`403 Resource not accessible by integration`** — l'integrazione GitHub di ChatGPT
  era in sola lettura.

🔴 **La conseguenza operativa conta più dell'errore.** Nella parte finale della chat ChatGPT aveva già
riformulato alcune decisioni per prepararne la scrittura, e quelle riformulazioni **non sono canoniche**:
`DQA-026` vi compariva come *collisione di movimento* e `DQA-028` come *schema dei campi di
`DamagePacket`*, entrambe letture più strette del testo Drive. Le righe `RESOLVED — AUTHOR DECISION`
trascritte al §3 sono la formulazione da usare.

## 2. Stato osservato dalla chat, e perché va rimisurato

| Cosa | Valore osservato da ChatGPT | Autorità |
|---|---|---|
| `main` HEAD | `e30361e9302e21009b73641be217b4277617165d` (merge PR [#1905](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1905)) | istantanea storica |
| massimo Decision ID | `D-285` | istantanea storica |
| `D-286`…`D-289` | «non presenti» | istantanea storica |

⚠️ **Nessuno di questi tre valori va riusato per assegnare un ID.** L'istruzione originale lo dice da sé —
*«Gli ID sono assegnati in base allo stato corrente, non a questa chat»*. Il freshness check è
`git fetch origin && git checkout main && git pull --ff-only`, poi si rimisura il massimo `D-nnn`.

🔑 **La riserva si è verificata nel giro di poche ore.** Al momento della sincronizzazione `main` era
`7c48ce63` — **dieci commit oltre** l'HEAD osservato — e il massimo era **`D-291`**, non `D-285`. Nel
frattempo `D-286`, `D-287`, `D-289` e `D-290` erano diventati **buchi mai usati**: la PR
[#1899](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1899) ha rinominato la propria
`D-288` in `D-291` *«perche' main ne ha mergiata un'altra con quel numero»* (commit `598717fd`). Due
sessioni avevano scelto lo stesso ID nello stesso pomeriggio, ed è esattamente il difetto che il
freshness check previene.

## 3. Le cinque decisioni d'autore, come stanno sul Drive

Rilette **live** dal tab `Open Questions` il 2026-08-31 (foglio modificato alle `09:55Z`). Le righe
coincidono verbatim con quelle trascritte nell'istruzione: **nessuna divergenza fra handoff e Drive**.

| DQA | Repo ID (Drive) | Owner dichiarato (Drive) | Stato |
|---|---|---|---|
| `DQA-026` | `NEW — Resolution Ordering` | `AUTHOR-RESOLUTION-001 pending repo sync` | `RESOLVED — AUTHOR DECISION` |
| `DQA-027` | `NEW — Same-Boundary Reactions` | `AUTHOR-REACTION-BOUNDARY-001 pending repo sync` | `RESOLVED — AUTHOR DECISION` |
| `DQA-028` | `NEW — Damage Contract` | `AUTHOR-DAMAGE-001 pending repo sync` | `RESOLVED — AUTHOR DECISION` |
| `DQA-029` | `NEW — Same-Layer KO` | `AUTHOR-KO-001 pending repo sync` | `RESOLVED — AUTHOR DECISION` |
| `DQA-030` | `NEW — KO Occupancy` | Snapshot / Resolver + Unit State + Movement occupancy owner | `RESOLVED — AUTHOR DECISION` |

### `DQA-026` — Resolution Ordering

> A — Freeze Turn → MacroPhase → MicroStep → ResolutionLayer → Same-Layer Contributions → Atomic Commit
> → Derived-State Checkpoint. Canonical layer order: PreInterrupt, Structural, Movement,
> PostEntryEnvironment, TriggerDetection, Reaction, ActionEffects, PostEffectEnvironment,
> EnvironmentPropagation, Finalize. Same-layer contributions read one State N; Priority resolves only
> genuine same-layer conflicts; derived-state refresh is a checkpoint, not a gameplay layer.

### `DQA-027` — Same-Boundary Reactions

> A — Evaluate the complete same-boundary Contribution Set before selecting reactions. Resolve all
> non-conflicting valid reactions. Consuming inputs may be consumed once; explicitly shareable inputs may
> support multiple matches. Conflicts use ReactionPriority, then stable IDs. Reaction outputs do not
> recursively re-enter the same boundary; explicit immediate propagation is queued to a subsequent
> canonical boundary/layer. No RNG or hard-coded skill pairs.

### `DQA-028` — Damage Contract

> A — Mitigate independently per DamagePacket. Direct: ApplicableArmor = Armor; Environmental: 0.
> Piercing preserves negative Armor and ignores only positive Armor. Defense = ApplicableArmor + signed
> DamageResistance\[DamageType\]. FinalDamage = max(0, BaseDamage - Defense), then TemporaryShield →
> Shield → Health. Multi-packet attacks are mitigated per packet; presentation hits need not equal packet
> count. No universal MinArmor clamp in v0.1.

### `DQA-029` — Same-Layer KO

> A — KO is materialized at Atomic Commit. Units eligible at State N keep already-committed same-layer
> contributions even if they are KO'd by simultaneous damage during that commit. KO blocks subsequent
> layers, microsteps and newly generated opportunities. KO is not an implicit same-layer interrupt; only
> an explicit earlier-layer cancel/interrupt can remove a committed contribution.

### `DQA-030` — KO Occupancy

La riga che l'istruzione segnalava come *«potrebbe essere stata aggiunta dopo l'inizio della
conversazione»*. Esiste, ed è `RESOLVED — AUTHOR DECISION`.

> A — KO removes blocking occupancy at the Atomic Commit boundary. The defeated unit keeps
> already-committed contributions from the current same-layer State N, but after the commit it no longer
> blocks its FRTCellId for subsequent layers or micro-steps. The body may remain visible, but presentation
> is non-authoritative. Any future corpse-blocking mechanic requires an explicit gameplay object/rule.

## 4. Cosa l'handoff non sapeva

🔴 **Il tab `Open Questions` non finisce a `DQA-030`: arriva a `DQA-044`.** L'istruzione si ferma a 030
perché la chat si era fermata lì, non perché il Drive lì finisse. Le righe `DQA-031`…`DQA-044` hanno un
`Repo ID` che punta a issue esistenti
([#1833](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1833),
[#1826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1826),
[#1733](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1733),
[#1879](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1879),
[#1880](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1880)) e **non appartengono a questo
handoff**: hanno già un owner nel repository.

🔴 **Le quattro domande `DQA-026`…`DQA-029` erano già state consumate** — da un *altro* kit, il *Session
Handoff 2026-08-31 §11*, istruito su `origin/main` @ `0c0ee87c` e chiuso da
[`D-291`](../../../decisions/RT_PDR_00_Decision_Log.md), mergiata alle **12:01** del 2026-08-31, cioè
mentre questa sincronizzazione era in corso. `D-291` **non promuove nessuna delle quattro a canone**, e su
due punti dice l'opposto della riga Drive. Il conflitto è istruito nel referto e nelle voci
`AUTHOR-RESOLUTION-001` / `AUTHOR-DAMAGE-001` di [`OPEN_DECISIONS.md`](../../../OPEN_DECISIONS.md):
**non è stato sciolto qui**, perché scioglierlo sarebbe stato inventare una sintesi fra una decisione
d'autore e una misura sul codice.

## 5. L'istruzione originale, per intero

Chiedeva in ordine: leggere `main`, leggere il Decision Log, conservare l'handoff, rileggere il Drive live,
sincronizzare le decisioni ancora `pending repo sync` **senza duplicare** quelle già sincronizzate,
verificare i gate di test e documentazione, e produrre un report. Poneva inoltre tredici invarianti da
preservare — determinismo a parità di snapshot/regole/versione/seed; invarianza dell'hash alla permutazione
dei contributi same-layer; causalità dal `ResolutionLayer` e non dall'ordine Actor/container; commit della
topologia prima del refresh derivato; reaction matching sull'intero Contribution Set; combo cross-team
deterministiche; nessuna ricorsione incontrollata nello stesso boundary; mitigazione conforme a `DQA-028`;
KO che non cancella contributi same-layer già validati; KO che libera la cella dopo il proprio Atomic
Commit; ordine di presentazione ininfluente sull'esito; TurnLog che spiega causalità e risultati; zero leak
degli intenti avversari — e vietava esplicitamente di *«usare ordine di Actor, array, container o Tick come
autorità gameplay»*, vincolo che il repository applica già con il sort stabile di
`ARTTurnManager::CollectLivingUnits` (`RTTurnManager.cpp:4994`) e i sei test di permutazione.

La regola finale dell'istruzione è quella che ha governato questa sessione:

> Se trovi una differenza fra questa istruzione, il Drive live e il repository corrente:
> **misura e segnala la differenza; non indovinare.**
