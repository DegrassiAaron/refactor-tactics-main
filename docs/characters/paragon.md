# Personaggi basati sugli asset Paragon

Questa sezione raccoglie **tutti i 38 hero asset slot** presenti nel Character Master Matrix di RefactorTactics.

> 🔁 **Dal 2026-11-08 ([D-585](../decisions/RT_PDR_00_Decision_Log.md)) gli eroi prendono il nome dello slot Paragon che li veste, come nomi di sviluppo**: Riktor, Steel, Gadget, Murdock e quattro slot nuovi, Kallari, Feng Mao, Muriel, The Fey,
> al posto di Phase, Wraith, Aurora e Kwang. L'invariante «lo slot non è l'identità» cade; la nota IP del canone resta e i nomi di pubblicazione si decidono prima di pubblicare. ✅ **Il rename è avvenuto per due eroi**
> ([#4011](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4011), 2026-11-13): `Hero.Aevik` è ora `Hero.Gadget` e `Hero.Branth` è ora `Hero.Riktor`, nel codice e nelle tabelle di questa pagina. ⏳ Nel codice restano `Hero.Muiren` e `Hero.Ivrin`; `Hero.Ward`, `Hero.Rime`, `Hero.Vigil` e `Hero.Tethra` stanno solo nella tabella qui sotto e non sono nel codice. Tutti descrivono lo stato in vigore finché le loro fette di #4011 non consegnano, e la
> mappatura dei quattro slot uscenti sui quattro entranti è decisa da [D-586](../decisions/RT_PDR_00_Decision_Log.md): **per ruolo**, Phase → Muriel, Wraith → Kallari, Aurora → The Fey, Kwang → Feng Mao,
> con le fazioni ereditate dallo slot sostituito e i kit dalla specifica come input di design (`MSP-5.d`). E35 porta Steel e Murdock nella v0.1; The Fey e Feng Mao sono v0.2.

> **Attenzione:** soltanto **quattro** dei 38 slot sono assegnati alla **v0.2** — Steel, Aurora, Murdock e Kwang, la cui identità RefactorTactics è **Ward**, **Rime**, **Vigil** e **Tethra**. ⌫ *Dal 2026-11-04 i quattro sono di E35, anticipata alla v0.1 ([`D-576`](../decisions/RT_PDR_00_Decision_Log.md) (4)): «v0.2» qui e nella tabella sotto è il nome che aveva la loro release, e la cartella v0.2 conserva il nome.* Gli altri **34** sono candidati `SIGNATURE_DEFINED` con release `UNASSIGNED`.
>
> 🔴 **Corretta il 2026-09-04 da [D-321](../decisions/RT_PDR_00_Decision_Log.md): questa riga affermava il contrario di ciò che è vero.** Diceva che *«le quattro pagine v0.1 di Gadget, Phase, Riktor e Wraith sono personaggi RefactorTactics separati: hanno un asset Paragon come base visuale, non un'identità Paragon»*. È **falso**, e la misura che lo falsifica è nella tabella qui sotto: quei quattro nomi **sono** i nomi degli slot, non nomi ispirati ad essi — `Gadget`, `Phase`, `Riktor` e `Wraith` compaiono in [`candidates/`](candidates/), il pool dei 38. La coincidenza è letterale, quindi sono identità Paragon.
>
> Le loro identità retail sono **Aevik**, **Muiren**, **Branth** e **Ivrin** ([D-334](../decisions/RT_PDR_00_Decision_Log.md), che supersede [D-322](../decisions/RT_PDR_00_Decision_Log.md) sui soli quattro nomi v0.1). ⏳ Il runtime porta ancora gli ID legacy: la migrazione è **post-v0.1** e ha come owner [#2297](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2297).
>
> ⌫ *Dal 2026-11-08 [D-585](../decisions/RT_PDR_00_Decision_Log.md) supera D-334: gli eroi prendono il nome dello slot Paragon che li veste, come nomi di sviluppo, e la migrazione va al contrario, in [#4011](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4011), che supera #2297. Per Aevik e Branth è avvenuta il 2026-11-13: oggi sono `Hero.Gadget` e `Hero.Riktor`. Muiren e Ivrin restano fino alla loro fetta, con cui al loro posto entrano Muriel e Kallari, con kit nuovi ([D-586](../decisions/RT_PDR_00_Decision_Log.md) (1)(2)).*

I nomi Paragon identificano la **base asset / slot di roster**. CharacterId, nome finale, lore, kit e bilanciamento RefactorTactics sono originali. Dal 2026-09-04 **nessuno slot del roster ha più un nome retail `TBD`**: gli otto sono decisi da [D-334](../decisions/RT_PDR_00_Decision_Log.md) (v0.1, che supersede [D-322](../decisions/RT_PDR_00_Decision_Log.md)) e dalla decisione d'autore registrata in [#322](https://github.com/DegrassiAaron/refactor-tactics-main/issues/322) (v0.2). ⌫ *Dal 2026-11-08 [D-585](../decisions/RT_PDR_00_Decision_Log.md) supera questo paragrafo nei nomi: gli eroi prendono il nome dello slot Paragon che li veste, come nomi di sviluppo, e il rename di [#4011](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4011) porta quel nome nel `CharacterId` — `Hero.Gadget` e `Hero.Riktor` dal 2026-11-13. Il nome finale si decide prima di pubblicare (D-585 (2)).*

## Mapping visuale del roster

[D-037](../decisions/RT_PDR_00_Decision_Log.md) (2026-08-08). **Questa tabella è l'unica fonte del mapping**: gli
altri documenti la referenziano, non la copiano.

| Identità RefactorTactics | `RT Character ID` | Slot asset Paragon | Release |
|---|---|---|---|
| **Gadget** | `Hero.Gadget` | [Gadget](candidates/gadget.md) | v0.1 |
| **Muiren** ⏳ | `Hero.Muiren` → `Hero.Muiren` | [Phase](candidates/phase.md) | v0.1 |
| **Riktor** | `Hero.Riktor` | [Riktor](candidates/riktor.md) | v0.1 |
| **Ivrin** ⏳ | `Hero.Wraith` → `Hero.Ivrin` | [Wraith](candidates/wraith.md) | v0.1 |
| **Ward** | `Hero.Ward` | [Steel](v0.2/steel.md) | v0.1 (E35, ex v0.2) |
| **Rime** | `Hero.Rime` | [Aurora](v0.2/aurora.md) | v0.1 (E35, ex v0.2) |
| **Vigil** | `Hero.Vigil` | [Murdock](v0.2/murdock.md) | v0.1 (E35, ex v0.2) |
| **Tethra** | `Hero.Tethra` | [Kwang](v0.2/kwang.md) | v0.1 (E35, ex v0.2) |

⏳ = **identità decisa, ID non ancora migrato**, nel verso di [D-334](../decisions/RT_PDR_00_Decision_Log.md): la freccia era il lavoro di [#2297](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2297). [D-585](../decisions/RT_PDR_00_Decision_Log.md) ha rovesciato quel verso il 2026-11-08, e #2297 è chiusa come superata da [#4011](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4011), che porta gli ID agli otto nomi Paragon della specifica. Per Gadget e Riktor il rename è avvenuto il 2026-11-13: le loro righe non portano più né ⏳ né freccia. Su Muiren e Ivrin il segno è stantio — nel codice gli ID sono già `Hero.Muiren` e `Hero.Ivrin` — e lo toglie la loro fetta di #4011, con cui al loro posto entrano Muriel e Kallari, con kit nuovi ([D-586](../decisions/RT_PDR_00_Decision_Log.md) (1)(2)). Ward, Rime, Vigil e Tethra non hanno il segno, ma aspettano anch'essi #4011.

Tre concetti, tre colonne, e restano separati:

- **identità RefactorTactics** — nome, `CharacterId`, kit, lore, fazione;
- **slot asset Paragon** — mesh, scheletro e animazioni usati come base visuale del prototipo;
- **release** — quando l'eroe entra nel roster operativo.

~~Per i quattro slot v0.2 il nome coincide con quello di lavoro. **Coincidere non è essere deciso**: il nome retail
resta soggetto alla governance del roster.~~

✅ **Chiuso il 2026-09-04.** La governance ha deciso: **Ward · Vigil · Rime · Tethra** per la v0.2, **Aevik · Muiren ·
Branth · Ivrin** per la v0.1 ([D-322](../decisions/RT_PDR_00_Decision_Log.md), i cui quattro nomi v0.1 sono stati ridecisi il 2026-09-05 da [D-334](../decisions/RT_PDR_00_Decision_Log.md)). La riga «Identità originale» di
[`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md) non ha più uno slot di roster da coprire. La frase barrata resta
perché descriveva correttamente lo stato fino a quel giorno, ed è la ragione per cui i nomi v0.2 sono stati a lungo
scritti come *nomi di lavoro*. ⌫ *Dal 2026-11-08 la riga «Identità originale» ha di nuovo gli slot del roster da
coprire: con [D-585](../decisions/RT_PDR_00_Decision_Log.md) (2) i nomi del roster sono nomi di sviluppo, e quelli di
pubblicazione si decidono lì prima di pubblicare (D-585 (4)). Se con i nomi Paragon o con nomi originali, lo decide la
lettura della licenza degli asset in [#4019](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4019).*

> **Si scrive `Paragon.Gadget` quando si parla del pack e `Hero.Gadget` per l'ID dell'eroe, mai `Gadget` nudo.** In questo repository
> `Gadget` è già una categoria di equipaggiamento (`ERTEquipmentSlot::Gadget`, voci `Gadget.Medkit`, `Gadget.Sensor`
> dei cataloghi), e con [D-585](../decisions/RT_PDR_00_Decision_Log.md) è anche il nome dell'eroe, il cui ID è
> `Hero.Gadget` dal 2026-11-13 ([#4011](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4011)): un
> mapping scritto senza qualificatore produrrebbe tre significati per la stessa parola nello stesso documento. È la
> collisione di lettura che [D-120](../decisions/RT_PDR_00_Decision_Log.md) aveva misurato e che D-585 (4) ritrova:
> il namespace `Hero.<Nome>.<Abilità>` di [D-130](../decisions/RT_PDR_00_Decision_Log.md) (3) la rende innocua nei
> token, non nella prosa.

<!-- rename-exempt: la riga dichiara la rinomina: sostituirla la renderebbe muta -->
~~Il mapping **non rinomina niente nel gameplay**: `Hero.Flux` resta `Hero.Flux`, e uno slot Paragon usato come
base visuale non diventa un personaggio RefactorTactics.~~

🔁 **Doppiamente superata, e va letta come storia.** `Hero.Flux` non esiste più dal 2026-08-13:
[D-130](../decisions/RT_PDR_00_Decision_Log.md) ha rimosso `Flux`, `Riva`, `Bastion` e `Vektor` da tutto il
repository, e quei quattro **non tornano** — la transizione in corso è `Gadget → Aevik`, non un ritorno.
La conclusione è caduta a sua volta: [D-321](../decisions/RT_PDR_00_Decision_Log.md) stabilisce che un nome
**identico** allo slot *è* identità Paragon, e quindi il mapping oggi **rinomina eccome** — in modo differito,
tracciato e con un owner ([#2297](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2297)).

✅ **Ciò che resta vero è la separazione**, che [D-321](../decisions/RT_PDR_00_Decision_Log.md) ripristina intera:
uno slot asset non è un'identità, e le tre colonne della tabella restano tre concetti distinti.

⌫ *Dal 2026-11-08 [D-585](../decisions/RT_PDR_00_Decision_Log.md) supera anche i due paragrafi qui sopra: la transizione
è al contrario, `Aevik → Gadget`, eseguita il 2026-11-13 da [#4011](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4011)
che supera #2297; e cade l'invariante «lo slot non è l'identità», perché Gadget e Riktor portano il nome dello slot che
li veste. Resta la separazione fra kit e slot: il kit di Gadget è quello RefactorTactics, non quello del pack (sotto,
«Mapping FX del kit»).*

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
| [Steel](v0.2/steel.md) | Guardian / Vanguard | Guard Meter | 3/5 | **v0.1** (E35, ex v0.2) — `Hero.Ward` |
| [Aurora](v0.2/aurora.md) | Controller | Frozen Domain | 4/5 | **v0.1** (E35, ex v0.2) — `Hero.Rime` |
| [Murdock](v0.2/murdock.md) | Marksman | Focus + Fire Sector | 3/5 | **v0.1** (E35, ex v0.2) — `Hero.Vigil` |
| [Kwang](v0.2/kwang.md) | Fighter / Controller | Electric Anchor | 4/5 | **v0.1** (E35, ex v0.2) — `Hero.Tethra` |
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
| [Wraith](candidates/wraith.md) | Recon / Predicter | Insight | 5/5 | **Roster v0.1** — `Hero.Wraith` → `Hero.Ivrin` ⏳ |
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
| [Phase](candidates/phase.md) | Support / Mobility | Phase Link | 5/5 | **Roster v0.1** — `Hero.Muiren` → `Hero.Muiren` ⏳ |
| [Narbash](candidates/narbash.md) | Support / Rhythm | Beat | 4/5 | Candidate |
| [Iggy & Scorch](candidates/iggy-and-scorch.md) | Terrain Controller / Duo | Combustion Field | 5/5 | Candidate |
| [Sparrow](candidates/sparrow.md) | Marksman | Precision Chain | 3/5 | Candidate |
| [Revenant](candidates/revenant.md) | Execution Marksman | Chamber | 5/5 | Candidate |

## Nota sul conteggio degli asset Epic

Il Character Master Matrix traccia **38 hero nominati**. Le liste ufficiali Epic del 2018 riportano 19 hero nel primo gruppo e 19 nel gruppo finale. La pagina generale degli asset parla anche di “39 characters”; nel primo elenco era presente separatamente il pack **Minions and Jungle Creeps**, che qui non viene trattato come hero con una pagina personaggio.

## Fonti

- `RefactorTactics_Character_Master_Matrix.md`
- [`RefactorTactics_Characters_Wiki_Data_v0.4.xlsx`](data/RefactorTactics_Characters_Wiki_Data_v0.4.xlsx)
- Epic Games, Paragon asset release (March/September 2018)

## Governance del kit

Le future abilità restano proprietà del singolo personaggio. Una Signature può suggerire affinità con stati o sistemi, ma non crea automaticamente un kit di coppia o di fazione.
