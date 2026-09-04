# Personaggi basati sugli asset Paragon

Questa sezione raccoglie **tutti i 38 hero asset slot** presenti nel Character Master Matrix di RefactorTactics.

> **Attenzione:** soltanto **quattro** dei 38 slot sono assegnati alla **v0.2** — Steel, Aurora, Murdock e Kwang, la cui identità RefactorTactics è **Ward**, **Rime**, **Vigil** e **Tethra**. Gli altri **34** sono candidati `SIGNATURE_DEFINED` con release `UNASSIGNED`.
>
> 🔴 **Corretta il 2026-09-04 da [D-321](../decisions/RT_PDR_00_Decision_Log.md): questa riga affermava il contrario di ciò che è vero.** Diceva che *«le quattro pagine v0.1 di Gadget, Phase, Riktor e Wraith sono personaggi RefactorTactics separati: hanno un asset Paragon come base visuale, non un'identità Paragon»*. È **falso**, e la misura che lo falsifica è nella tabella qui sotto: quei quattro nomi **sono** i nomi degli slot, non nomi ispirati ad essi — `Gadget`, `Phase`, `Riktor` e `Wraith` compaiono in [`candidates/`](candidates/), il pool dei 38. La coincidenza è letterale, quindi sono identità Paragon.
>
> Le loro identità retail sono **Nexis**, **Slake**, **Kern** e **Scryer** ([D-322](../decisions/RT_PDR_00_Decision_Log.md)). ⏳ Il runtime porta ancora gli ID legacy: la migrazione è **post-v0.1** e ha come owner [#2297](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2297).

I nomi Paragon identificano la **base asset / slot di roster**. CharacterId, nome finale, lore, kit e bilanciamento RefactorTactics sono originali. Dal 2026-09-04 **nessuno slot del roster ha più un nome retail `TBD`**: gli otto sono decisi da [D-322](../decisions/RT_PDR_00_Decision_Log.md) (v0.1) e dalla decisione d'autore registrata in [#322](https://github.com/DegrassiAaron/refactor-tactics-main/issues/322) (v0.2).

## Mapping visuale del roster

[D-037](../decisions/RT_PDR_00_Decision_Log.md) (2026-08-08). **Questa tabella è l'unica fonte del mapping**: gli
altri documenti la referenziano, non la copiano.

| Identità RefactorTactics | `RT Character ID` | Slot asset Paragon | Release |
|---|---|---|---|
| **Nexis** ⏳ | `Hero.Gadget` → `Hero.Nexis` | [Gadget](candidates/gadget.md) | v0.1 |
| **Slake** ⏳ | `Hero.Phase` → `Hero.Slake` | [Phase](candidates/phase.md) | v0.1 |
| **Kern** ⏳ | `Hero.Riktor` → `Hero.Kern` | [Riktor](candidates/riktor.md) | v0.1 |
| **Scryer** ⏳ | `Hero.Wraith` → `Hero.Scryer` | [Wraith](candidates/wraith.md) | v0.1 |
| **Ward** | `Hero.Ward` | [Steel](v0.2/steel.md) | v0.2 |
| **Rime** | `Hero.Rime` | [Aurora](v0.2/aurora.md) | v0.2 |
| **Vigil** | `Hero.Vigil` | [Murdock](v0.2/murdock.md) | v0.2 |
| **Tethra** | `Hero.Tethra` | [Kwang](v0.2/kwang.md) | v0.2 |

⏳ = **identità decisa, ID non ancora migrato.** La freccia è il lavoro di [#2297](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2297), differito post-v0.1 da [D-321](../decisions/RT_PDR_00_Decision_Log.md). Finché quella issue è aperta, `Hero.Gadget` è ciò che il codice contiene e `Nexis` è ciò che il personaggio si chiama: **due verità simultanee e dichiarate**, non una divergenza.

Tre concetti, tre colonne, e restano separati:

- **identità RefactorTactics** — nome, `CharacterId`, kit, lore, fazione;
- **slot asset Paragon** — mesh, scheletro e animazioni usati come base visuale del prototipo;
- **release** — quando l'eroe entra nel roster operativo.

~~Per i quattro slot v0.2 il nome coincide con quello di lavoro. **Coincidere non è essere deciso**: il nome retail
resta soggetto alla governance del roster.~~

✅ **Chiuso il 2026-09-04.** La governance ha deciso: **Ward · Vigil · Rime · Tethra** per la v0.2, **Nexis · Slake ·
Kern · Scryer** per la v0.1 ([D-322](../decisions/RT_PDR_00_Decision_Log.md)). La riga «Identità originale» di
[`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md) non ha più uno slot di roster da coprire. La frase barrata resta
perché descriveva correttamente lo stato fino a quel giorno, ed è la ragione per cui i nomi v0.2 sono stati a lungo
scritti come *nomi di lavoro*.

> **Si scrive `Paragon.Gadget`, mai `Gadget` nudo.** In questo repository `Gadget` è già una categoria di
> equipaggiamento (`ERTEquipmentSlot::Gadget`, voci `Gadget.Medkit`, `Gadget.Sensor` dei cataloghi): un mapping
> scritto senza qualificatore produrrebbe due significati per la stessa parola nello stesso documento.

<!-- rename-exempt: la riga dichiara la rinomina: sostituirla la renderebbe muta -->
~~Il mapping **non rinomina niente nel gameplay**: `Hero.Flux` resta `Hero.Flux`, e uno slot Paragon usato come
base visuale non diventa un personaggio RefactorTactics.~~

🔁 **Doppiamente superata, e va letta come storia.** `Hero.Flux` non esiste più dal 2026-08-13:
[D-130](../decisions/RT_PDR_00_Decision_Log.md) ha rimosso `Flux`, `Riva`, `Bastion` e `Vektor` da tutto il
repository, e quei quattro **non tornano** — la transizione in corso è `Gadget → Nexis`, non un ritorno.
La conclusione è caduta a sua volta: [D-321](../decisions/RT_PDR_00_Decision_Log.md) stabilisce che un nome
**identico** allo slot *è* identità Paragon, e quindi il mapping oggi **rinomina eccome** — in modo differito,
tracciato e con un owner ([#2297](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2297)).

✅ **Ciò che resta vero è la separazione**, che [D-321](../decisions/RT_PDR_00_Decision_Log.md) ripristina intera:
uno slot asset non è un'identità, e le tre colonne della tabella restano tre concetti distinti.

## Mapping FX del kit — abilità RT → asset Paragon

[D-297](../decisions/RT_PDR_00_Decision_Log.md) (2026-08-31) estende [D-037](../decisions/RT_PDR_00_Decision_Log.md)
dall'eroe alla **singola azione**: uno slot FX Paragon può essere la sorgente visiva di un'abilità
RefactorTactics, e **non ne vincola il comportamento**. Vale qui la stessa separazione della tabella sopra —
questa colonna è presentazione, non gameplay.

> ⚠️ **Questa tabella non è un criterio d'accettazione.** D-297 toglie il campo `Fidelity` — «alta / molto
> alta» non ha scala e non è falsificabile. Una riga vuota qui non è un difetto da riempire.

### Cosa il pacchetto permette di scrivere oggi

Misurato su `Content/FabAsset/Paragon/` (2026-08-31): **un eroe su quattro** ha gli FX segmentati per abilità.

| Eroe | `FX/` del pacchetto | Tabella costruibile |
|---|---|---|
| Gadget | `Abilities/` → `ElectroGate · Primary · RollingBot · StickyBomb · Ultimate · VisionBot` | sì |
| Phase | `Materials · Meshes · Particles · Textures` | **no** — nessuna partizione per abilità |
| Riktor | `Cut_Outs · Materials · Meshes · Particles · Textures · VectorFields` | **no** |
| Wraith | `Materials · Meshes · Particles · Textures · VectorFields` | **no** |

🔴 **Per tre eroi su quattro la colonna nasce vuota, e va lasciata vuota.** Riempirla con i nomi di skill
Paragon presi da un kit di consolidamento reintrodurrebbe proprio gli identificatori che D-297 ha misurato
come inaffidabili: il kit che quella decisione consuma si dichiarava *«verificato nelle fonti»* e attribuiva a
Gadget un `Plasma Blast` che appartiene a **un altro eroe del pack**.

### Gadget

| AbilityId | Abilità | FX Paragon | Base |
|---|---|---|---|
| `Hero.Gadget.ArcPulse` | Impulso ad arco *(attacco base)* | `Primary` | **misurata** — corrispondenza di **ruolo**: `ArcPulse` è l'indice 0 del kit RT, e `Primary` è l'attacco base lato Paragon — lo nomina il soundcue `Gadget_Effort_Ability_Primary_Fire`, che è audio e non FX, ma fissa il **vocabolario** del pack |
| `Hero.Gadget.LinearDischarge` | Scarica lineare | — | aperta |
| `Hero.Gadget.ConductiveNode` | Nodo conduttore | — | aperta |
| `Hero.Gadget.Overload` | Sovraccarico | — | aperta |
| `Hero.Gadget.ReactiveCapacitor` | Capacitore reattivo | — | aperta |

Restano **non assegnate** tre cartelle: `RollingBot`, `StickyBomb`, `VisionBot`.

⚠️ **Quelle tre orfane non sono un buco da tappare: sono la prova materiale di D-297.** Il Gadget Paragon
*dispiega* — droni, mine, bot da ricognizione; il Gadget RefactorTactics **conduce** — `Affinity.Electricity`,
il grafo conduttivo di `Action.Electrify`, la sinergia con `Status.Wet`. Sono due kit diversi che condividono
una mesh, ed è esattamente ciò che significa *«lo slot non è l'identità»*. Assegnare `StickyBomb` a
un'abilità elettrica per riempire una cella sarebbe far derivare il comportamento dalla presentazione, cioè
il legame che D-297 esclude.

Le quattro righe `aperta` si chiudono **una alla volta**, quando qualcuno guarda l'effetto e decide che serve
a quell'abilità. Nessuna scadenza: una cella vuota qui non blocca niente.

### Prerequisito dichiarato

[#1663](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1663) è **OPEN** — *«nel pacchetto gli
eroi non hanno animazioni: 756 asset Paragon cotti, zero clip»*. Finché quella issue non chiude, questa
tabella è un **mapping di intenzione** e non una consegna di presentazione: nomina l'effetto che si vorrebbe
usare, non un effetto che la partita sa già mostrare.

## Immagini

Ogni pagina contiene una card grafica locale in `images/paragon/`. È un placeholder informativo generato dai dati. Può essere sostituito con uno screenshot in-engine dell'asset corrispondente senza modificare il Markdown.

## Indice completo

| Asset base | Macro ruolo RT | Signature | Complessità tecnica | Stato roster |
| --- | --- | --- | ---: | --- |
| [Steel](v0.2/steel.md) | Guardian / Vanguard | Guard Meter | 3/5 | **v0.2** — `Hero.Ward` |
| [Aurora](v0.2/aurora.md) | Controller | Frozen Domain | 4/5 | **v0.2** — `Hero.Rime` |
| [Murdock](v0.2/murdock.md) | Marksman | Focus + Fire Sector | 3/5 | **v0.2** — `Hero.Vigil` |
| [Kwang](v0.2/kwang.md) | Fighter / Controller | Electric Anchor | 4/5 | **v0.2** — `Hero.Tethra` |
| [Terra](candidates/terra.md) | Guardian | Fortress | 3/5 | Candidate |
| [Greystone](candidates/greystone.md) | Vanguard | Resolve | 3/5 | Candidate |
| [Grux](candidates/grux.md) | Bruiser | Battle Momentum | 3/5 | Candidate |
| [Rampage](candidates/rampage.md) | Bruiser / Tank | Fury | 3/5 | Candidate |
| [Sevarog](candidates/sevarog.md) | Tank / Controller | Essence Harvest | 4/5 | Candidate |
| [Riktor](candidates/riktor.md) | Controller | Tether | 4/5 | **Roster v0.1** — `Hero.Riktor` → `Hero.Kern` ⏳ |
| [Crunch](candidates/crunch.md) | Combo Fighter | Combo State Machine | 5/5 | Candidate |
| [Boris](candidates/boris.md) | Bruiser | Overdrive / Heat | 3/5 | Candidate |
| [TwinBlast](candidates/twinblast.md) | Skirmisher | Alternating Rhythm | 4/5 | Candidate |
| [Drongo](candidates/drongo.md) | Striker / Controller | Payload Mix | 4/5 | Candidate |
| [Wraith](candidates/wraith.md) | Recon / Predicter | Insight | 5/5 | **Roster v0.1** — `Hero.Wraith` → `Hero.Scryer` ⏳ |
| [Lt. Belica](candidates/lt-belica.md) | Counter / Control | Suppression Charge | 4/5 | Candidate |
| [GRIM.exe](candidates/grim-exe.md) | Ranged / Utility | Core Modes | 4/5 | Candidate |
| [Gadget](candidates/gadget.md) | Engineer | Device Network | 5/5 | **Roster v0.1** — `Hero.Gadget` → `Hero.Nexis` ⏳ |
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
| [Phase](candidates/phase.md) | Support / Mobility | Phase Link | 5/5 | **Roster v0.1** — `Hero.Phase` → `Hero.Slake` ⏳ |
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
