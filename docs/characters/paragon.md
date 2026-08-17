# Personaggi basati sugli asset Paragon

Questa sezione raccoglie **tutti i 38 hero asset slot** presenti nel Character Master Matrix di RefactorTactics.

> **Attenzione:** soltanto **Steel, Aurora, Murdock e Kwang** sono attualmente assegnati alla **v0.2**. Gli altri **34** sono candidati `SIGNATURE_DEFINED` con release `UNASSIGNED`. Le quattro pagine v0.1 di Gadget, Phase, Riktor e Wraith sono personaggi RefactorTactics separati: hanno un asset Paragon come **base visuale** (tabella qui sotto), non un'identità Paragon.

I nomi Paragon identificano la **base asset / slot di roster**. CharacterId, nome finale, lore, kit e bilanciamento RefactorTactics restano originali/TBD quando non già definiti.

## Mapping visuale del roster

[D-037](../decisions/RT_PDR_00_Decision_Log.md) (2026-08-08). **Questa tabella è l'unica fonte del mapping**: gli
altri documenti la referenziano, non la copiano.

| Identità RefactorTactics | `RT Character ID` | Slot asset Paragon | Release |
|---|---|---|---|
| Gadget | `Hero.Gadget` | [Gadget](candidates/gadget.md) | v0.1 |
| Phase | `Hero.Phase` | [Phase](candidates/phase.md) | v0.1 |
| Riktor | `Hero.Riktor` | [Riktor](candidates/riktor.md) | v0.1 |
| Wraith | `Hero.Wraith` | [Wraith](candidates/wraith.md) | v0.1 |
| Steel *(nome di lavoro)* | `TBD` | [Steel](v0.2/steel.md) | v0.2 |
| Aurora *(nome di lavoro)* | `TBD` | [Aurora](v0.2/aurora.md) | v0.2 |
| Murdock *(nome di lavoro)* | `TBD` | [Murdock](v0.2/murdock.md) | v0.2 |
| Kwang *(nome di lavoro)* | `TBD` | [Kwang](v0.2/kwang.md) | v0.2 |

Tre concetti, tre colonne, e restano separati:

- **identità RefactorTactics** — nome, `CharacterId`, kit, lore, fazione;
- **slot asset Paragon** — mesh, scheletro e animazioni usati come base visuale del prototipo;
- **release** — quando l'eroe entra nel roster operativo.

Per i quattro slot v0.2 il nome coincide con quello di lavoro. **Coincidere non è essere deciso**: il nome retail
resta soggetto alla governance del roster ([`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md), riga «Identità
originale»).

> **Si scrive `Paragon.Gadget`, mai `Gadget` nudo.** In questo repository `Gadget` è già una categoria di
> equipaggiamento (`ERTEquipmentSlot::Gadget`, voci `Gadget.Medkit`, `Gadget.Sensor` dei cataloghi): un mapping
> scritto senza qualificatore produrrebbe due significati per la stessa parola nello stesso documento.

<!-- rename-exempt: la riga dichiara la rinomina: sostituirla la renderebbe muta -->
Il mapping **non rinomina niente nel gameplay**: `Hero.Flux` resta `Hero.Flux`, e uno slot Paragon usato come
base visuale non diventa un personaggio RefactorTactics.

## Immagini

Ogni pagina contiene una card grafica locale in `images/paragon/`. È un placeholder informativo generato dai dati. Può essere sostituito con uno screenshot in-engine dell'asset corrispondente senza modificare il Markdown.

## Indice completo

| Asset base | Macro ruolo RT | Signature | Complessità tecnica | Stato roster |
| --- | --- | --- | ---: | --- |
| [Steel](v0.2/steel.md) | Guardian / Vanguard | Guard Meter | 3/5 | v0.2 |
| [Aurora](v0.2/aurora.md) | Controller | Frozen Domain | 4/5 | v0.2 |
| [Murdock](v0.2/murdock.md) | Marksman | Focus + Fire Sector | 3/5 | v0.2 |
| [Kwang](v0.2/kwang.md) | Fighter / Controller | Electric Anchor | 4/5 | v0.2 |
| [Terra](candidates/terra.md) | Guardian | Fortress | 3/5 | Candidate |
| [Greystone](candidates/greystone.md) | Vanguard | Resolve | 3/5 | Candidate |
| [Grux](candidates/grux.md) | Bruiser | Battle Momentum | 3/5 | Candidate |
| [Rampage](candidates/rampage.md) | Bruiser / Tank | Fury | 3/5 | Candidate |
| [Sevarog](candidates/sevarog.md) | Tank / Controller | Essence Harvest | 4/5 | Candidate |
| [Riktor](candidates/riktor.md) | Controller | Tether | 4/5 | **Roster v0.1** — `Hero.Riktor` |
| [Crunch](candidates/crunch.md) | Combo Fighter | Combo State Machine | 5/5 | Candidate |
| [Boris](candidates/boris.md) | Bruiser | Overdrive / Heat | 3/5 | Candidate |
| [TwinBlast](candidates/twinblast.md) | Skirmisher | Alternating Rhythm | 4/5 | Candidate |
| [Drongo](candidates/drongo.md) | Striker / Controller | Payload Mix | 4/5 | Candidate |
| [Wraith](candidates/wraith.md) | Recon / Predicter | Insight | 5/5 | **Roster v0.1** — `Hero.Wraith` |
| [Lt. Belica](candidates/lt-belica.md) | Counter / Control | Suppression Charge | 4/5 | Candidate |
| [GRIM.exe](candidates/grim-exe.md) | Ranged / Utility | Core Modes | 4/5 | Candidate |
| [Gadget](candidates/gadget.md) | Engineer | Device Network | 5/5 | **Roster v0.1** — `Hero.Gadget` |
| [Howitzer](candidates/howitzer.md) | Artillery | Artillery Heat | 4/5 | Candidate |
| [Zinx](candidates/zinx.md) | Sustain / Controller | Energy Debt | 5/5 | Candidate |
| [Muriel](candidates/muriel.md) | Support | Guardian Link | 4/5 | Candidate |
| [Dekker](candidates/dekker.md) | Controller | Control Geometry | 4/5 | Candidate |
| [Kallari](candidates/kallari.md) | Infiltrator | Hunt / Trace | 5/5 | Candidate |
| [Feng Mao](candidates/feng-mao.md) | Skirmisher | Flow | 3/5 | Candidate |
| [Khaimera](candidates/khaimera.md) | Pursuer | Predator / Pursuit | 4/5 | Candidate |
| [Serath](candidates/serath.md) | Duelist | Dual State | 4/5 | Candidate |
| [Yin](candidates/yin.md) | Reaction Duelist | Deflection | 5/5 | Candidate |
| [Wukong](candidates/wukong.md) | Trickster / Mobility | Echo Paths | 5/5 | Candidate |
| [Countess](candidates/countess.md) | Assassin | Blood Contract | 4/5 | Candidate |
| [Shinbi](candidates/shinbi.md) | Duelist / Setup | Resonance | 4/5 | Candidate |
| [Gideon](candidates/gideon.md) | Controller / Mobility | Rift Network | 5/5 | Candidate |
| [The Fey](candidates/the-fey.md) | Terrain Controller | Growth | 5/5 | Candidate |
| [Morigesh](candidates/morigesh.md) | Hexer / Hunter | Hex | 4/5 | Candidate |
| [Phase](candidates/phase.md) | Support / Mobility | Phase Link | 5/5 | **Roster v0.1** — `Hero.Phase` |
| [Narbash](candidates/narbash.md) | Support / Rhythm | Beat | 4/5 | Candidate |
| [Iggy & Scorch](candidates/iggy-and-scorch.md) | Terrain Controller / Duo | Combustion Field | 5/5 | Candidate |
| [Sparrow](candidates/sparrow.md) | Marksman | Precision Chain | 3/5 | Candidate |
| [Revenant](candidates/revenant.md) | Execution Marksman | Chamber | 5/5 | Candidate |

## Nota sul conteggio degli asset Epic

Il Character Master Matrix traccia **38 hero nominati**. Le liste ufficiali Epic del 2018 riportano 19 hero nel primo gruppo e 19 nel gruppo finale. La pagina generale degli asset parla anche di “39 characters”; nel primo elenco era presente separatamente il pack **Minions and Jungle Creeps**, che qui non viene trattato come hero con una pagina personaggio.

## Fonti

- `RefactorTactics_Character_Master_Matrix.md`
- `docs/src/data/characters-wiki-data-v0.4.xlsx`
- Epic Games, Paragon asset release (March/September 2018)

## Governance del kit

Le future abilità restano proprietà del singolo personaggio. Una Signature può suggerire affinità con stati o sistemi, ma non crea automaticamente un kit di coppia o di fazione.
