# E53 · CP 53.2–53.3 — Il piano della formula nel codice e della passata ×10

> `CURRENT` · **Owner**: E53 ([#4013](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4013)) · **Scritto**: 2026-11-09 · **Decisioni a monte**: [D-587](../../decisions/RT_PDR_00_Decision_Log.md), [D-589](../../decisions/RT_PDR_00_Decision_Log.md), [D-590](../../decisions/RT_PDR_00_Decision_Log.md) (3), [D-595](../../decisions/RT_PDR_00_Decision_Log.md), [D-597](../../decisions/RT_PDR_00_Decision_Log.md), [D-599](../../decisions/RT_PDR_00_Decision_Log.md)
> **Tabella di controllo**: [`e53-tabella-di-controllo-danno-2026-11-09.md`](e53-tabella-di-controllo-danno-2026-11-09.md) · **Inventario**: `python tools/balance/scale_x10_inventory.py` (§3 ne riporta l'uscita del 2026-11-09)
> ⚠️ La finestra di freeze della v0.1 è **terminata** il 2026-10-26 ([D-547](../../decisions/RT_PDR_00_Decision_Log.md)): `Source/` e `Content/` si toccano su branch con PR, come ogni altro lavoro. Le voci del registro che dicono «fuori dal freeze» per E53 vanno lette come «dopo la finestra già chiusa».

## 1. Che cosa è già nel codice (CP 53.2, primo commit)

- `URTCombatLibrary::MitigateDirectDamage(Base, RearBonus, ModPercent, Defense)`: la casa unica della formula, in interi, con un solo arrotondamento per difetto dopo il prodotto. **Nessun chiamante a runtime**: con `Defense = 0` e `ModPercent = 100` è l'identità sui valori del catalogo di oggi, e il test lo pinna. Nessun golden cambia.
- `RTDamageFormulaTests.cpp` (`RefactorTactics.Combat.Formula.*`): gli esempi 1, 2, 5, 6, 7 della tabella e la composizione dell'esempio 3 (copertura e Guard sottratte dopo), più il controllo positivo dell'identità a difesa zero.

## 2. L'ordine dei commit che restano, con il gate di ciascuno

| Passo | Che cosa cambia | Gate che lo chiude | Perché in questo ordine |
|---|---|---|---|
| **53.2b — i campi** | `URTHeroData::Defense` e `ElementResistance` (default per elemento, D-595 (3)), `RearBonus` (D-599), copiati sull'unità; **tutti a zero**. ✅ **Eseguito** in [#4035](https://github.com/DegrassiAaron/refactor-tactics-main/pull/4035) (2026-11-09): `URTHeroData` e `ARTUnit` portano `Defense`, `ResistanceDefault`, `ElementResistance`, `RearBonus`; `URTHeroCatalogLibrary::ElementIds()` tiene i dodici di D-593 e `ValidateHeroes` rifiuta negativi e chiavi fuori elenco. ⚠️ La prima stesura di questa riga diceva che «`FRTUnitCombatState` e `FRTHexCombatUnit` li portano»: misurato, **nessuna delle due porta nemmeno `GuardReduction`** (`grep GuardReduction` su `RTCombatResolver.h` e `RTHexCombatLibrary.h` → 0); il resolver lo legge da `ARTUnit` in `RTTurnManager` (`GuardReductionByTarget`). I campi nuovi seguono la stessa via, e 53.2c decide se e come entrano nelle struct | `Simulation.GoldenCorpusMatches` **byte-identico** e l'hash dello snapshot invariato: se un campo nuovo entra nell'hash, lo si dichiara prima e si rigenera con una PR a sé | i campi a zero non cambiano un numero: è l'unico momento in cui si può verificare che la plumbing sia neutra |
| **53.2c — il chiamante** | ✅ **Eseguito** in [#4036](https://github.com/DegrassiAaron/refactor-tactics-main/pull/4036) (2026-11-09). La sede NON è `URTCombatResolver` ma `URTHexCombatLibrary::CollectHexAttacks` (e `RedirectHitTo`): la copertura si sottrae lì, e D-589 (3) vuole la DEF prima della copertura, quindi `MitigatedHitPower` chiama `MitigateDirectDamage(Base, spalle, 100, DEF)` nel punto in cui il colpo nasce. `FRTHexCombatUnit` porta `Defense` e `RearBonus` (copiati dallo snapshot del Blast e dalla preview); «alle spalle» è «fuori dall'arco frontale» dall'origine d'impatto (`ImpactOriginOf`, la regola della Guardia). Stadi `RearBonus`/`Defense` e op `Mitigate` nel breakdown. **Non entra**: la RES per elemento (il colpo non porta un elemento), `Mod` (53.3b), i valori (53.3c), la voce di TurnLog con il bonus applicato (con i valori) | golden **byte-identici** (tutto è ancora a zero); `Combat.Formula.*` verdi; un test di integrazione con una DEF iniettata sul bersaglio (non dal catalogo) che riproduce l'esempio 1 | la formula entra nel flusso senza cambiare un numero: ogni golden che cambiasse qui sarebbe un difetto di plumbing, non di dati |
| **53.3a — la passata ×10** | ✅ **Eseguito** in [#4038](https://github.com/DegrassiAaron/refactor-tactics-main/pull/4038) (2026-11-09), un commit solo. L'inventario §3 era incompleto e la passata l'ha completato misurando: oltre ai letterali elencati sono scalati le soglie delle fasce di danno (`DamageBandOf` 190/110), i delta delle varianti d'arma, `RTFallDamage`, i default di `URTActionData::Power`, `URTHeroData`/`ARTUnit` (`MaxHealth`, `GuardReduction`), i `"health"` degli scenari JSON, le ancore dei radar (`POWER_ANCHOR` 1000, `DURABILITY_ANCHOR` 1500) e i due pesi del bot che moltiplicano un danno (`WDamage`/`WAllyDamage` 10 → 1, punteggio invariato). **Non scalato, per lettura dell'assistente**: il canale delle strutture (`DamageStructure`, `Integrity`). Tutti i letterali dell'inventario §3: effetti del catalogo, `MaxHealth`, `GuardReduction` (15 → 150), copertura bassa (10 → 100), `BaseShield` (5 → 50), `BurningCleanupDamage`, `PropagatedElectricDamage`, i bonus di cella e di `Wet`; le tabelle di `docs/balance/`; i test che pinnano un valore | un commit solo; `git diff --name-only -- '*.rttl'` elenca **tutti** i golden (ogni danno cambia); i test aggiornati nello stesso commit; `node --test` e la suite intera verdi | atomico, o non è reversibile: un catalogo a metà scala è il peggior stato possibile |
| **53.3b — `Exposed`** | `ExposedFirstHitBonus` (+5) si ritira; `Exposed` diventa `ModPercent = 120` sul primo colpo diretto (D-595 (2)) | `Combat.Formula.ExposedMultipliesTheBase` resta verde; il test che pinnava il +5 si sostituisce, non si spegne; golden con `Exposed` rigenerati e dichiarati per nome | dopo la scala, perché il valore 120 è stato scelto sui numeri ×10 |
| **53.3c — DEF, RES, bonus alle spalle** | i valori di D-595 (3) e D-599 nel catalogo eroi (i quattro a runtime con quelli dello slot che diventeranno) | la tabella di controllo diventa test di integrazione sul catalogo reale; golden rigenerati e dichiarati; `Heroes.RosterIsBalanced` riletto | i numeri entrano per ultimi, quando la forma è dimostrata neutra |
| **53.3d — la quota** | D-597: in `EffectiveCoverReduction`, `Attacker.Cell.Layer > Target.Cell.Layer` annulla la copertura bassa; la ragione («retro» o «quota») entra nel colpo accanto a `CoverBypassedByFacing` | `Combat.HighGroundIgnoresLowCover`; `Reactions.*` sul tracciato della copertura verdi; se il campo nuovo entra nel TurnLog, i golden che lo portano si rigenerano e si dichiarano | separato perché cambia la **semantica** di un tracciato esistente, non un numero |
| **53.5 — le coperture** | i dodici archetipi del GDD come righe del catalogo coperture con valore piatto, Integrity e arco (D-590 (3)) | catalogo terreni e test di copertura | dopo la scala, perché i valori sono ×10 |

Dopo 53.3a la tabella estesa (tutti i colpi, gli otto eroi, le dodici RES) è il posto dove copertura 100 e Guard 150 si **tarano** con la DEF in mano (D-595 (4)): l'esempio «HeavyAttack 350 contro DEF 35 in copertura e Guard = 9» è il primo caso di controllo dichiarato.

## 3. L'inventario, misurato il 2026-11-09 su `main` = `7566359a2`

> ✅ **Eseguito** in [#4038](https://github.com/DegrassiAaron/refactor-tactics-main/pull/4038). Le tabelle qui sotto sono la fotografia **prima** della passata: i valori «Oggi» sono quelli della scala precedente, e `tools/balance/scale_x10_inventory.py` ora stampa la colonna inversa («scala precedente (÷10)»). L'inventario non vedeva: `Action.Charge` (il letterale stava sullo stesso rigo della spinta), le soglie di `DamageBandOf`, i delta d'arma, `RTFallDamage`, i default di dati e unità, gli scenari JSON, le ancore dei radar, i pesi del bot. Sono tutti nella PR, con la ragione di ciascuno.

## Effetti del catalogo con un numero (Damage, Heal, Shield)

| Azione | Effetto | Oggi | ×10 |
|---|---|---|---|
| `Action.Heal` | Heal | 18 | 180 |
| `Action.Counter` | Damage | 14 | 140 |
| `Action.Counter` | Shield | 15 | 150 |
| `Action.Charge` | Damage | 20 | 200 |
| `Action.PrecisionAttack` | Damage | 24 | 240 |
| `Action.HeavyAttack` | Damage | 35 | 350 |
| `Action.LineAttack` | Damage | 22 | 220 |
| `Action.CircularAoE` | Damage | 18 | 180 |
| `Action.Mortar` | Damage | 12 | 120 |
| `Action.SuppressiveLine` | Damage | 16 | 160 |
| `Action.Counter` | Damage | 16 | 160 |
| `Action.Shield` | Shield | 25 | 250 |
| `Action.Electrify` | Damage | 20 | 200 |
| `Action.Heal` | Heal | 20 | 200 |
| `Hero.Aevik.LinearDischarge` | Damage | 24 | 240 |
| `Hero.Aevik.Overload` | Damage | 18 | 180 |
| `Action.Counter` | Shield | 15 | 150 |
| `Action.Counter` | Damage | 10 | 100 |
| `Hero.Aevik.LinearDischarge.Concentrated` | Damage | 30 | 300 |
| `Hero.Aevik.LinearDischarge.Branched` | Damage | 18 | 180 |
| `Hero.Aevik.LinearDischarge.Branched` | Damage | 18 | 180 |
| `Hero.Muiren.PressureJet` | Damage | 16 | 160 |
| `Action.Heal` | Heal | 18 | 180 |
| `Hero.Muiren.CircularTide.Healing` | Heal | 24 | 240 |
| `Hero.Muiren.CircularTide.Impact` | Heal | 10 | 100 |
| `Hero.Branth.ImpactShot` | Damage | 8 | 80 |
| `Hero.Ivrin.PulseShot` | Damage | 21 | 210 |
| `Hero.Ivrin.InterceptShot` | Damage | 16 | 160 |
| `Hero.Ivrin.PassingBlade` | Damage | 20 | 200 |
| `Hero.Ivrin.InterceptShot.Precise` | Damage | 20 | 200 |
| `Hero.Ivrin.InterceptShot.Extended` | Damage | 14 | 140 |

## Statistiche degli eroi

| Eroe | Statistica | Oggi | ×10 |
|---|---|---|---|
| Aevik | `MaxHealth` | 90 | 900 |
| Muiren | `MaxHealth` | 95 | 950 |
| Branth | `MaxHealth` | 120 | 1200 |
| Ivrin | `MaxHealth` | 90 | 900 |

## Costanti intere di `URTCombatLibrary` (da leggere una per una: non tutte sono danni)

| Costante | Oggi |
|---|---|
| `ExposedFirstHitBonus` | 5 |
| `GuardReductionDefault` | 15 |
| `BurningCleanupDamage` | 8 |
| `BaseShield` | 5 |
| `PropagatedElectricDamage` | 12 |
| `GuardResistedPushDistance` | 1 |
| `UnbalancedDurationTurns` | 2 |
| `ProneDurationTurns` | 2 |
| `StunnedDurationTurns` | 2 |
| `StandUpMovePointCost` | 1 |
| `UnbalancedExtraDisplacement` | 1 |
| `MarkedFirstHitBonus` | 6 |
| `DeflectDamageReduction` | 20 |
| `BraceDamageReduction` | 10 |
| `LowCoverDamageReduction` | 10 |
| `AevikWetDischargeBonus` | 8 |

## Tabelle di `docs/balance/` con una colonna numerica (righe)

| File | Righe |
|---|---|
| `README.md` | 0 |
| `RT_ActionCatalog_v0.1.md` | 56 |
| `RT_ActionDescriptions_v0.1.md` | 0 |
| `RT_ElementPairCatalog_v0.1.md` | 0 |
| `RT_EquipmentCatalog_v0.1.md` | 4 |
| `RT_FootprintCatalog_v0.1.md` | 0 |
| `RT_Glossary_v0.1.md` | 0 |
| `RT_HeroCatalog_v0.1.md` | 42 |
| `RT_TerrainCatalog_v0.1.md` | 9 |
| `RT_TestMatrix_v0.1.md` | 12 |

## Golden che la passata rigenera (tutti: ogni danno cambia)

- `Source/RefactorTactics/Tests/Golden/Combat.CounterStrikesBack/turn-01.rttl`
- `Source/RefactorTactics/Tests/Golden/Movement.Basic/turn-01.rttl`
- `Source/RefactorTactics/Tests/Golden/Movement.Collision/turn-01.rttl`
- `Source/RefactorTactics/Tests/Golden/Movement.CostlyCorridor/turn-01.rttl`
- `Source/RefactorTactics/Tests/Golden/RT_Showcase_Relay_v01/turn-01.rttl`
- `Source/RefactorTactics/Tests/Golden/RT_Showcase_Relay_v01/turn-02.rttl`
- `Source/RefactorTactics/Tests/Golden/RT_Showcase_Relay_v01/turn-03.rttl`
- `Source/RefactorTactics/Tests/Golden/RT_Showcase_Relay_v01/turn-04.rttl`
- `Source/RefactorTactics/Tests/Golden/RT_Showcase_Relay_v01/turn-05.rttl`
- `Source/RefactorTactics/Tests/Golden/RT_Showcase_Relay_v01/turn-06.rttl`
- `Source/RefactorTactics/Tests/Golden/RT_Showcase_Relay_v01/turn-07.rttl`
- `Source/RefactorTactics/Tests/Golden/RT_Showcase_Relay_v01/turn-08.rttl`
- `Source/RefactorTactics/Tests/Golden/Spec.Environment.WaterQuenchesFire/turn-01.rttl`
- `Source/RefactorTactics/Tests/Golden/Spec.Environment.WaterQuenchesFire/turn-02.rttl`
- `Source/RefactorTactics/Tests/Golden/Spec.Environment.WaterQuenchesFire/turn-03.rttl`
- `Source/RefactorTactics/Tests/Golden/Spec.Overwatch.HoldThenFire/turn-01.rttl`
- `Source/RefactorTactics/Tests/Golden/Spec.Overwatch.HoldThenFire/turn-02.rttl`
- `Source/RefactorTactics/Tests/Golden/Spec.Predictive.WhiffOnEmptyCell/turn-01.rttl`
- `Source/RefactorTactics/Tests/Golden/Spec.Predictive.WhiffOnEmptyCell/turn-02.rttl`
- `Source/RefactorTactics/Tests/Golden/Visual.Combat.FallbackTargetMoved/turn-01.rttl`

Misurato con `python tools/balance/scale_x10_inventory.py`; i conteggi valgono al commit in cui si legge.


⚠️ Le costanti di `URTCombatLibrary` non si moltiplicano tutte: `GuardResistedPushDistance`, `UnbalancedDurationTurns`, `ProneDurationTurns`, `StunnedDurationTurns` e le altre durate sono **turni e celle**, non danni. L'inventario le elenca perché un `grep` non le distingue; la passata le legge una per una e le lascia.

I **test che pinnano un valore** (per esempio `GuardReductionDefault`, `LowCoverDamageReduction`, i 24/35 degli attacchi nei test di catalogo) non sono nell'inventario: si trovano con `git grep -nE '\b(10|15|24|35|90|95|120)\b' -- Source/RefactorTactics/Tests` letto a mano, e si aggiornano nello stesso commit della passata. Un test che pinnava un numero e resta verde dopo la passata è un test che non misurava quel numero.

## 4. Stati di verifica di questo piano

Compile · Tests: quelli del primo commit di CP 53.2 (vedi la PR). Determinism · Replay: `N/A` per il piano; i gate dei passi sono scritti nella tabella. 53.2b è eseguito ([#4035](https://github.com/DegrassiAaron/refactor-tactics-main/pull/4035), 2026-11-09): build verde, suite `Heroes+Unit+Simulation+Combat` 218/218 senza rossi, `git diff --name-only -- '*.rttl'` vuoto dopo la suite, `Simulation.GoldenCorpusMatches` verde. 53.2c è eseguito ([#4036](https://github.com/DegrassiAaron/refactor-tactics-main/pull/4036), 2026-11-09): build verde, suite `Combat+HexCombat+Damage+Simulation+Heroes+Unit` senza rossi (conteggio nella PR), `git diff --name-only -- '*.rttl'` vuoto dopo la suite, `Simulation.GoldenCorpusMatches` verde. 53.3a è eseguito ([#4038](https://github.com/DegrassiAaron/refactor-tactics-main/pull/4038), 2026-11-09): build verde, suite intera senza rossi, `node --test` dei radar verde, golden rigenerati e dichiarati nella PR. Da 53.3b in poi nessun passo è eseguito.
