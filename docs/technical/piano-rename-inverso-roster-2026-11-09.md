# Piano del rename inverso del roster — gli ID tornano ai nomi degli slot Paragon (#4011)

> `CURRENT` · **Data**: 2026-11-09 · **Owner del piano**: questo documento · **Issue**: [#4011](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4011)
> **Consegue da**: [D-585](../decisions/RT_PDR_00_Decision_Log.md) (gli otto nomi Paragon come nomi di sviluppo, rename secco inverso), [D-586](../decisions/RT_PDR_00_Decision_Log.md) (mappatura per ruolo, kit di Muiren e Ivrin ritirati, E35.1 in v0.1), [D-593](../decisions/RT_PDR_00_Decision_Log.md) (identificatori degli elementi che i kit nuovi usano)
> **Modellato su**: [`piano-migrazione-roster.md`](piano-migrazione-roster.md) (D-130, eseguito) e sul corpo di [#2297](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2297) (fette B–F, eseguite in parte da #2491): le trappole misurate lì valgono qui alla lettera, nel verso opposto
> **Release**: post-v0.1, fuori dal freeze ([D-542](../decisions/RT_PDR_00_Decision_Log.md)) · ⛔ **Nessun commit di questo piano è stato eseguito**: è la mappa, misurata il 2026-11-09 su `main` = `5eac6115e`

## 1. Che cosa cambia, per eroe

| Oggi (ID e kit) | Domani | Natura del passo |
|---|---|---|
| `Hero.Aevik`, kit a runtime | `Hero.Gadget`, **stesso kit** | rename secco di ID, token abilità (`Hero.Aevik.ArcPulse` → `Hero.Gadget.ArcPulse`), path, cartelle |
| `Hero.Branth`, kit a runtime | `Hero.Riktor`, **stesso kit** al rename; ➕ poi il kit nuovo di [D-612](../decisions/RT_PDR_00_Decision_Log.md) (2026-11-13) | rename secco, come sopra; il kit si sostituisce **dopo** il rename, così gli id nuovi nascono già `Hero.Riktor.*` |
| `Hero.Muiren`, kit a runtime | `Hero.Muriel`, **kit nuovo** ([scheda](../characters/v0.1/muriel.md)) | il kit di Muiren si **ritira**: gli `Hero.Muiren.*` si rimuovono, non si rinominano; il kit nuovo entra da `MSP-5.d` |
| `Hero.Ivrin`, kit a runtime | `Hero.Kallari`, **kit nuovo** ([scheda](../characters/v0.1/kallari.md)) | come sopra: gli `Hero.Ivrin.*` si rimuovono |
| `Hero.Ward`, `Hero.Vigil` (E35.1, nessun runtime) | `Hero.Steel`, `Hero.Murdock` | solo documenti e schede: in `Source/` e `Scenarios/` non compaiono |
| `Hero.Rime`, `Hero.Tethra` (E35.2, nessun runtime) | escono; entrano `Hero.TheFey`, `Hero.FengMao` in v0.2 | solo documenti: le schede di Aurora e Kwang restano con il banner |

⚠️ **Il verso di D-130 e di #2491 era «un nome nuovo per lo stesso kit»**; qui due eroi su quattro cambiano kit. È la differenza che decide l'ordine dei commit (§4): il rename di Aevik e Branth è meccanico, quello di Muiren e Ivrin è una **rimozione** seguita da un'**aggiunta**, e gli scenari che usavano le loro abilità come strumento vanno riscritti prima.

## 2. Il perimetro, misurato

Righe che portano il token (`git grep -IhoE 'Hero\.<X>\b'`, testo), su `main` = `5eac6115e`:

| Eroe | `Source/` | `Scenarios/` | `docs/` (fuori `archive/` e `research/`) | `Content/` (testo) |
|---|---|---|---|---|
| Aevik | 446 | 214 | 724 | 0 |
| Muiren | 225 | 126 | 362 | 0 |
| Branth | 345 | 272 | 324 | 5 |
| Ivrin | 276 | 189 | 249 | 5 |

I numeri sono una fotografia: si rimisurano con lo stesso comando prima di ogni fetta.

- **Abilità di Muiren e Ivrin** (token distinti in `Source/` e `Scenarios/`): `Hero.Muiren.PressureJet`, `TideGuard`, `CircularTide`, `MistVeil`, `FluidTrail`, `FlowReaction`; `Hero.Ivrin.PulseShot`, `Deflection`, `PassingBlade`, `InterceptShot`, `PhaseGuard`, `Feint`, `PiercingShot`. I token `Hero.Muiren.SecondoScudo`, `Hero.Ivrin.NonEsiste`, `Hero.Ivrin.IvrinGuard`, `Hero.Ivrin.Base` sono fixture di test, non abilità: si rinominano con il test che le usa.
- **File che nominano un'abilità di Muiren o Ivrin**: 48 test `.cpp`, 44 scenari, 3 golden (`RT_Showcase_Relay_v01/turn-02.rttl`, `…/turn-05.rttl`, `Spec.Predictive.WhiffOnEmptyCell/turn-02.rttl`), 19 file di `Source/` fuori dai test (catalogo, HUD, bot). I conteggi sono di oggi; la lista nominale degli scenari è nel banner di [`muiren.md`](../characters/v0.1/muiren.md) e [`wraith.md`](../characters/v0.1/wraith.md).
- **Golden con un ID d'eroe**: Aevik in `Combat.CounterStrikesBack/turn-01`, `RT_Showcase_Relay_v01/turn-06`, `turn-07`; Branth negli stessi tre più `Visual.Combat.FallbackTargetMoved/turn-01`; Ivrin nei tre sopra; Muiren in nessuno. `Tests/Fixtures/Legacy/turnlog-v10-movement-collision.rttl` non porta ID d'eroe e **non** si rigenera.
- **Path C++ degli asset** (`grep -rn "Game/RT/Characters" Source/`): `RTGameMode.cpp:593–596` (`FClassFinder` nel costruttore, i quattro `BP_Unit_*`), `Tests/Meta/RTG7BlueprintSurfaceTests.cpp:15–18`, `Tests/Map/Authoring/RTHeroSpawnTests.cpp:577–580`, `Tests/Unit/RTUnitClickTargetTests.cpp:255` (Muiren), `Tests/UI/HUD/RTHudScenarioTests.cpp:694` (path costruito dal nome). `RTUnit.h:2379` e `RTUnitLayerOutline.cpp:15` puntano a `Shared/` e **non** si toccano. `RTHeroLab.h:86` è un commento.
- **Content**: `Content/RT/Characters/{Aevik,Branth,Ivrin,Muiren}/Blueprints/BP_Unit_<X>.uasset` (binari: Editor + Fix Up Redirectors); `Content/RT/UI/Icons/RT_UI_Icon_Identity_<X>.uasset` per i quattro e `RT_UI_Icon_Action_Hero_<X>_<Abilità>.uasset` (Aevik 2, Branth 1, Ivrin 3, Muiren 2); `Content/Icons/Icons/*.svg` (Branth 6, Ivrin 6: SVG sorgente, `git mv`).
- **Documenti con il nome nel file**: `docs/characters/v0.1/aevik.md`, `muiren.md`, `docs/characters/radar/{aevik,muiren,branth,ivrin}-{balance,profile}.svg`, due evidenze PIE (restano: sono storia).
- **Test di identità e ratchet**: `RTLegacyIdentityRatchetTests.cpp` (tetti per `Hero.Gadget` 1/0, `Hero.Wraith` 0/0, `Hero.Riktor` 4, `Hero.Phase`), `Heroes.HeroIdsMatchRoster`, `Heroes.HeroUnitClassesCoversTheRoster`, `Heroes.CanonicalNamesReachTheLabel`, `Heroes.AbilityIdsAreNamespacedUnderTheirHero`, `Unit.ShortHeroNameFromStableId`, `Unit.CanonicalHeroIdHasNoLegacyName` (lista a otto voci), `Meta.LegacyHeroIdentitiesDoNotSpread`.

## 3. Le fette, al contrario di #2297

- **B — ID canonici**: `Hero.Aevik → Hero.Gadget`, `Hero.Branth → Hero.Riktor`, con i token `Hero.<X>.<Abilità>` a seguire; nessun redirect ([D-134](../decisions/RT_PDR_00_Decision_Log.md)). ⚠️ `Hero.Gadget` convive con `ERTEquipmentSlot::Gadget` e `Gadget.<Oggetto>`: innocuo nei token grazie al namespace (D-130 (3)), ma la grep che misura il rename deve usare `Hero\.Gadget\b`, mai `Gadget` nudo.
- **B′ — i kit che si ritirano**: `Hero.Muiren.*` e `Hero.Ivrin.*` si **rimuovono** da `RTHeroCatalogLibrary.cpp` e da ogni consumatore; i 44 scenari che li usano come strumento si riscrivono con un'azione universale equivalente (`Guard` per `TideGuard`/`PhaseGuard`, `BasicAttack` per `PulseShot`/`PressureJet`, `Deflect` per `Deflection`, `Leap` o `Charge` per `PassingBlade`), uno per uno, **prima** di rimuovere l'abilità; poi `Hero.Muiren → Hero.Muriel`, `Hero.Ivrin → Hero.Kallari` con il kit nuovo di `MSP-5.d`.
- **C — runtime, catalogo, test**: `RTGameMode.cpp` e i tre test con i path **nello stesso commit** dei `BP_Unit_*`; il ratchet si **ribalta** (i tetti misurano `Hero.Aevik`, `Hero.Muiren`, `Hero.Branth`, `Hero.Ivrin`, con lo stesso meccanismo); `CanonicalHeroIdHasNoLegacyName` inverte la lista a otto; `HeroIdsMatchRoster` legge il roster nuovo; `ShortHeroNameFromStableId` si sostituisce, mai si spegne.
- **D — scenari, replay, golden**: i golden della §2 si rigenerano **dopo** che i test di identità sono verdi; `git diff --name-only -- '*.rttl'` deve elencare tutti e soli i previsti; la fixture legacy resta byte-identica.
- **E — asset e documenti**: `BP_Unit_*` via Editor con Fix Up Redirectors, cartelle `Content/RT/Characters/<Nome>/`, icone `.uasset` via Editor, SVG con `git mv`, PNG rigenerati dal generatore (`tools/hud-assets/generate_hud_assets.py`), `docs/generated/icons/` rigenerata; schede `aevik.md → gadget.md`, `muiren.md` conservata con il banner e `muriel.md` già presente, `wraith.md` conservata e `kallari.md` già presente; `docs/archive/` e `docs/research/` **bit-identici**. Gate di ownership MCP prima di ogni scrittura (`CommandLine` dell'Editor contro il checkout inteso).
- **F — il gate**: la allowlist temporanea di #2291 cade; il gate che resta difende che `Hero.Aevik`, `Hero.Muiren`, `Hero.Branth`, `Hero.Ivrin` non rientrino come identità.

## 4. L'ordine dei commit

1. **Scenari riscritti** (B′ prima parte): i 44 scenari che usano un'abilità di Muiren o Ivrin come strumento passano a un'azione universale; suite verde; golden che cambiano rigenerati e dichiarati per nome. Nessun ID cambia ancora.
2. **Kit nuovi a catalogo** (`MSP-5.d`, parte di codice): `Hero.Muriel.*` e `Hero.Kallari.*` entrano con i valori della loro `D-nnn`; i test di `#4014` si estendono (`AbilityIdsAreNamespacedUnderTheirHero`, `HeroIdsMatchRoster`). ⚠️ Dipende dai numeri di `D-594` (costi) e `D-595` (DEF/RES): va **dopo** che E38 ed E53 li hanno portati nel catalogo, o con valori dichiarati input e un test che lo dice.
3. **Rename degli ID** (B e C in un commit solo per eroe, o per coppia): ID, token, path C++, `BP_Unit_*` via Editor, ratchet ribaltato, golden rigenerati.
4. **Asset e documenti** (E) e **gate** (F).
5. Oracolo umano in PIE: nessun nome RT a schermo, giudicato da una persona e dichiarato tale.

## 5. Che cosa questo piano non decide

I valori dei kit nuovi (`MSP-5.d` → `D-nnn`), i costi AP ([D-594](../decisions/RT_PDR_00_Decision_Log.md), nel catalogo con E38), DEF e RES ([D-595](../decisions/RT_PDR_00_Decision_Log.md), con E53), l'import degli asset Paragon dei quattro entranti (non sono nel checkout), la rianimazione (`GDD-4`), i nomi di pubblicazione ([#4019](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4019)).
