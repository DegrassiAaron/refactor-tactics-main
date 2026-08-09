# Flux

> **Asset base:** Paragon — Gadget  
> **Hero_Key:** `ASSET_FLUX`  
> **RT Character ID:** `Hero.Flux`  
> **Release:** `v0.1`  
> **Roster status:** Release v0.1  
> **Provenienza visuale:** mesh e animazioni vengono dallo slot Paragon **Gadget** ([D-037](../../decisions/RT_PDR_00_Decision_Log.md) · tabella owner in [`paragon.md`](../paragon.md)). L'asset è la base visuale del prototipo, non l'identità di Flux.

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ENV-SYSTEMIC-COMBOS -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ENV-SYSTEMIC-COMBOS` · Release: `v0.1` · Roadmap: `E8.5`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Visual.Combat.WaterElectric`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ENV-SYSTEMIC-COMBOS -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ENV-ELECTRIC -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ENV-ELECTRIC` · Release: `v0.1` · Roadmap: `E8.3`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Visual.Combat.WaterElectric`  
> Dal 2026-08-09 la scarica ha un **owner nel roster**: `Flux.ConductiveNode` **e'** `Action.Electrify` (D-046, nata come D-039). Prima nessun eroe la possedeva e il motore era verde ma non innescabile in partita.  
> Verificato il `2026-08-09` su `f1f85b1`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ENV-ELECTRIC -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-CHAR-V01-ROSTER -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-CHAR-V01-ROSTER` · Release: `v0.1` · Roadmap: `E6.1, E6.2, E6.3, E6.4, E6.5, E6.6, E6.7`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Combat.BasicAttack`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-CHAR-V01-ROSTER -->

## Panoramica

Tecnico della conduzione: fragile, controlla il campo e converte setup elettrici e bersagli Wet in pressione offensiva.

## Identità tattica

| Campo | Valore |
| --- | --- |
| Ruolo primario | Controller |
| Ruolo secondario | Striker |
| Macro ruolo Signature | Controller |
| Specializzazione | Electro Conduction Technician |
| Profilo range | Medium |
| Tipo danno | Electric |
| Complessità gameplay | TBD |
| Complessità tecnica (1–5) | TBD |
| Risorsa firma | Carica Conduttiva |
| Signature primaria | Conduction |
| Signature secondaria | Water-Electric Combo |
| Framework principali | Resource, Environment, Status, AoE |
| Dipendenze tecniche | Wet, electric propagation, device interactions |
| Player question | Come preparo la conduzione migliore? |

> **Nota bilanciamento:** Canonico v0.1. 90 HP, 5 MP, vista 6; bonus +8 di LinearDischarge contro Wet.

> **Nota su `Water-Electric Combo`:** è un'etichetta storica della meccanica secondaria. Indica l'interazione sistemica acqua/Wet ↔ elettricità, **non** una coppia obbligatoria Flux+Riva.

## Meccanica firma

### Descrizione della meccanica

**Conduction** è il nucleo del gameplay di Flux. Il personaggio non ottiene il massimo valore sparando semplicemente al primo bersaglio disponibile: prepara invece condizioni conduttive e cerca il momento in cui trasformarle in pressione offensiva. La meccanica combina la **Carica Conduttiva** con stati e superfici che rendono l'elettricità più efficace, in particolare `Wet`.

La Carica Conduttiva ha cap 4 e si rigenera di 1 tramite un'interazione elettrica; il valore iniziale non è ancora specificato. Nel kit v0.1 la meccanica ha due facce concrete: `LinearDischarge`, che ottiene +8 danni contro un bersaglio `Wet`, e `ConductiveNode`, che dal **2026-08-09** **è** `Action.Electrify` ([D-046](../../decisions/RT_PDR_00_Decision_Log.md)) — la propagazione sul grafo conduttivo, prima verde nei test e non innescabile in partita perché nessun eroe la possedeva.

Il controgioco è leggibile: evitare o rimuovere `Wet`, spezzare il setup ambientale e mettere pressione su Flux prima che possa convertire il campo preparato in un vantaggio.

### Lettura tattica

**Obiettivo del giocatore.** Preparare `Wet` o condizioni conduttive e scegliere quando convertire quel setup in danno/pressione. Quando il setup non è disponibile, `ArcPulse` mantiene una pressione stabile.

**Misplay / Failure State.** Preparare la conduzione dove il nemico non arriverà, o scaricare prima che il bersaglio sia `Wet`. `LinearDischarge` **parte comunque**: perde gli +8, non l'azione. Il costo vero è la Carica Conduttiva, che ha cap 4 e si rigenera di 1 per interazione elettrica — un setup speso male non si recupera nel turno, e a Flux resta `ArcPulse`, cioè pressione stabile senza il picco che giustifica il personaggio. È il failure state più *silenzioso* del roster: il turno sembra normale, e la differenza si vede solo nel confronto con ciò che la stessa carica avrebbe reso al momento giusto.

**Counterplay / rischio.** È il personaggio più fragile del roster v0.1. Se il nemico evita `Wet`, rompe il setup o lo costringe a spendere azioni difensive, la sua meccanica firma rende meno.

### Dati della meccanica

| Campo | Valore |
| --- | --- |
| Mechanic ID | `MECH_FLUX_CONDUCTION` |
| Nome | Conduction |
| Scope | Exclusive |
| Tipo | Primary Signature |
| Secondaria | Water-Electric Combo |
| Framework | Resource, Environment, Status |
| Dipendenze tecniche | Wet, electric interactions, propagation |
| Player question | Come preparo la conduzione migliore? |
| State | Carica Conduttiva + stato Wet / celle conduttive |
| Activation / Trigger | Interazioni elettriche e abilità di setup |
| Payoff | Trasforma setup ambientali in pressione, incluso +8 su Wet con LinearDischarge |
| Misplay / Failure State | Conduzione preparata nella zona sbagliata o scarica anticipata: `LinearDischarge` parte senza i +8 e la Carica Conduttiva è spesa; resta `ArcPulse` senza picco |
| Counterplay | Uscire da Wet, interrompere setup, pressione sul fragile Flux |
| Telegraphing | Stato pubblico/ambientale; risorsa team-visible |
| Design Status | IMPLEMENTED |

> Etichetta editoriale della meccanica; i numeri canonici vivono nei cataloghi v0.1.

## Statistiche base

| Campo | Valore |
| --- | --- |
| HP | 90 |
| Armatura | — |
| Resistenza | — |
| Movimento base | 5 |
| Iniziativa | — |
| Precisione (1–10) | — |
| Potenza (1–10) | — |
| Controllo (1–10) | — |
| Supporto (1–10) | — |
| Durabilità (1–10) | — |
| Indice Combat | — |
| Budget Punti | — |
| Delta Budget | — |

**Stato dati:** `CANONICAL_PARTIAL` — Canonico: HP 90, Move 5. Altri attributi di questa matrice non sono definiti nel catalogo v0.1.

## Visione e stealth

| Campo | Valore |
| --- | --- |
| Sight Range (hex) | 6 |
| Detection (0–100) | 48 |
| Identification (0–100) | 45 |
| Tracking (turni) | 1 |
| Vision Height | — |
| Stealth (1–10) | 2 |
| Reveal Recovery Step | — |
| Spotting Support (1–10) | — |
| Firma Movimento | — |
| Firma Attacco | — |
| Sensor Resist (1–10) | — |
| Contatto Default | — |

> Vista 6 canonica. Detection 48 / Identification 45 / Stealth 2 / Tracking 1 sono baseline catalogo, non attive nello slice binario v0.1.

**Stato dati:** `CANONICAL_BASELINE` — Percezione avanzata da differenziare via playtest.

## Mobilità

| Campo | Valore |
| --- | --- |
| Move Hex / MP | 5 |
| Sprint Bonus | — |
| Dash Range | — |
| Verticalità (1–10) | — |
| Costo Acqua | — |
| Costo Ghiaccio | — |
| Costo Fango | — |
| Porta AP | — |
| Ponte Mod | — |
| Tunnel Mod | — |
| Ascensore AP | — |
| Knockback Resist | 0 |
| Collision Priority | — |

> Move 5 MP e Push Resistance 0 canonici. Altri modificatori di mobilità non sono definiti per eroe.

**Stato dati:** `CANONICAL_PARTIAL` — Sprint/Dash usano le regole comuni del catalogo azioni.

## Risorsa firma

| Resource_ID | Nome | Cap | Start | Regen | Regen_Trigger | Spesa | Regola | Audience | Implementation_Status |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| RES_FLUX_CONDUCTIVE | Carica Conduttiva | 4 | — | 1 | Interazione elettrica | Abilità firma | Cap 4; start non specificato | Team-visible | CANONICAL_PARTIAL |

> **Ownership del kit:** le abilità di questa pagina appartengono esclusivamente a questo personaggio. Le sinergie con altri eroi sono esempi derivati da stati, superfici, geometria e altre regole comuni; non sono abilità condivise. Vedi [Sinergie e combinazioni](../../wiki/game/sinergie-e-combinazioni.md).

## Profilo di attacco base

| Campo | Valore |
| --- | --- |
| Ability ID | `Flux.ArcPulse` |
| Famiglia | **Engine** — *payload rinviato, vedi sotto* |
| Danno / portata | 22 · range 4 |
| Payload oltre il danno | **nessuno in v0.1** |
| Dipendenza dal base | ★★★☆☆ — colpo affidabile mentre le sue abilità sono in ricarica |

> ⚠️ **La famiglia descrive il kit, non ancora l'attacco base.** Il motore elettrico di Flux **esiste** — è
> `ConductiveNode`, cablata su `Action.Electrify` da [D-039](../../decisions/RT_PDR_00_Decision_Log.md) — ma
> non passa da `ArcPulse`, che in v0.1 fa solo danno. ADR-0007 ha rinviato il payload di carica sull'attacco
> base per una ragione precisa: darebbe alla generazione elettrica un **secondo produttore**, cioè il
> contrario di quello che D-039 ha appena messo in ordine. Nessuno stato `Charged` esiste nel codice.
>
> Finché vale questo, **non dichiarare Flux «Engine Attack» come se fosse già così**: sarebbe uno stato che
> il codice non sostiene.

> `ArcPulse` è anche l'**unico** attacco base del roster che prende i numeri dalla tabella a fasce condivisa
> (`MakeBasicAttack(4)` → 22 a range 4). Gli altri tre sono dell'eroe.

### Il test della falsa scelta

| Domanda | Risposta |
| --- | --- |
| Quando è la scelta corretta? | Quando `LinearDischarge` e `Overload` sono in ricarica e serve pressione affidabile a medio raggio. A 22 danni non è affatto debole: è il secondo del roster |
| Quando è inferiore a un'abilità firma? | Quando il bersaglio è **bagnato**: lì `LinearDischarge` vale 24 + 8 = 32 contro 22, e sprecare la finestra di `Wet` su un attacco base è l'errore tipico della coppia con Riva |
| Che cosa risparmia? | Il cooldown di `LinearDischarge` per il turno in cui l'acqua ci sarà davvero |
| Che counterplay esiste? | Quello ordinario — coperture, angoli, distanza. Non ha un counterplay proprio, perché non ha ancora un payload proprio |
| Che cosa impara il giocatore? | Che con Flux la domanda non è «quanto tolgo adesso» ma «l'acqua è già arrivata». L'attacco base è ciò che si fa **aspettando** che lo sia |

### Prove

| Che cosa | Dove |
| --- | --- |
| Il payload è nel dato | `RefactorTactics.Heroes.Flux.MatchesCatalog` · `RefactorTactics.Heroes.BasicAttackByRangeBand` — è l'unico legato alla fascia condivisa |
| L'effetto si vede in partita | `Combat.BasicAttack` — 120 − 22 = 98 su Bastion |
| Il payload di carica | ⏳ **non esiste** — dipende da `RT-FEAT-ENV-ELECTRIC`, non da questa pagina |

## Abilità

### Arc Pulse

#### Descrizione

Arc Pulse è l'attacco base affidabile di Flux. Infligge 22 danni a range 4 e non richiede setup ambientale: serve come opzione stabile quando non conviene investire una risorsa o preparare un payoff sistemico.

| Campo | Valore |
| --- | --- |
| Ability ID | `Flux.ArcPulse` |
| Categoria | Attacco base |
| Priorità | 50 |
| Costo risorsa | — |
| Cooldown (turni) | 0 |
| Range (hex) | 4 |
| AoE Radius | 0 |
| Danno base | 22 |
| Control Strength | 0 |
| Durata (turni) | 0 |
| Loss/Contact Policy | Fallback.Cancel |
| Interazione terreno | Attacco base medio raggio |
| Gameplay Tags | `Ability.Offense.Electric` |
| Implementation Status | IMPLEMENTED |
| Data Status | CANONICAL |

> MakeBasicAttack(4): 22 danni, range 4.

#### Uso tattico e limiti

È la scelta a basso impegno del kit: mantiene pressione a medio raggio senza consumare cooldown. Non riceve il bonus `Wet` di `LinearDischarge`.

### Linear Discharge

#### Descrizione

Linear Discharge è l'attacco lineare firma di Flux. Infligge 24 danni a range 5 e, se il bersaglio è `Wet`, aggiunge +8 danni. Il fallback è `AttackCell`, quindi la linea dichiarata resta rilevante anche se la situazione cambia durante la resolution.

| Campo | Valore |
| --- | --- |
| Ability ID | `Flux.LinearDischarge` |
| Categoria | Attacco lineare |
| Priorità | 55 |
| Costo risorsa | — |
| Cooldown (turni) | 2 |
| Range (hex) | 5 |
| AoE Radius | 0 |
| Danno base | 24 |
| Control Strength | 0 |
| Durata (turni) | 0 |
| Loss/Contact Policy | Fallback.AttackCell |
| Interazione terreno | +8 danni contro bersaglio Wet |
| Gameplay Tags | `Ability.Offense.Line.Electric` |
| Implementation Status | IMPLEMENTED |
| Data Status | CANONICAL |

> Bonus Wet gestito separatamente dal danno base.

#### Uso tattico e limiti

Premia il setup di Riva o di altre fonti di `Wet`. Le varianti spostano l'abilità verso burst singolo (`Concentrated`) o pressione multi-target (`Branched`).

### Conductive Node

#### Descrizione

Conductive Node è un'azione di setup in Prep che rende conduttiva una cella per 2 turni. Serve a estendere il concetto di Conduction dalla sola condizione `Wet` alla topologia ambientale.

| Campo | Valore |
| --- | --- |
| Ability ID | `Flux.ConductiveNode` |
| Categoria | Setup/Prep |
| Priorità | 35 |
| Costo risorsa | — |
| Cooldown (turni) | 2 |
| Range (hex) | 0 |
| AoE Radius | 0 |
| Danno base | 0 |
| Control Strength | 0 |
| Durata (turni) | 2 |
| Loss/Contact Policy | Fallback.Cancel |
| Interazione terreno | Rende conduttiva una cella per 2 turni; range 0 è placeholder tecnico corrente |
| Gameplay Tags | `Ability.Environment.Setup` |
| Implementation Status | PARTIAL |
| Data Status | CANONICAL |

> Effetto di conduttività non completamente rappresentato nel modello dell'azione.

#### Uso tattico e limiti

Il comportamento completo della cella conduttiva è ancora `PARTIAL`; il range 0 nel dato corrente è un placeholder tecnico e non va interpretato come scelta di bilanciamento finale.

### Overload

#### Descrizione

Overload è l'AoE di Flux: 18 danni, range 3, raggio 1 e cooldown 3. La specifica prevede anche un'interruzione dei dispositivi, ma questa parte dipende da sistemi non ancora completati.

| Campo | Valore |
| --- | --- |
| Ability ID | `Flux.Overload` |
| Categoria | AoE |
| Priorità | 65 |
| Costo risorsa | — |
| Cooldown (turni) | 3 |
| Range (hex) | 3 |
| AoE Radius | 1 |
| Danno base | 18 |
| Control Strength | 0 |
| Durata (turni) | 0 |
| Loss/Contact Policy | Fallback.AttackCell |
| Interazione terreno | Interrupt dispositivi previsto; danno AoE implementabile |
| Gameplay Tags | `Ability.Offense.AOE.Electric` |
| Implementation Status | PARTIAL |
| Data Status | CANONICAL |

> Danno 18/raggio1; interrupt dispositivi dipende da sistemi futuri.

#### Uso tattico e limiti

È il payoff ad area del kit quando più bersagli o elementi del campo sono raccolti nello stesso spazio. Oggi il danno AoE è la parte più definita; l'interazione con dispositivi resta `PARTIAL`.

### Reactive Capacitor

#### Descrizione

Reactive Capacitor è la reazione difensiva/offensiva di Flux. Quando subisce un attacco diretto, applica scudo 15 a Flux e 10 danni all'attaccante, con cooldown 3.

| Campo | Valore |
| --- | --- |
| Ability ID | `Flux.ReactiveCapacitor` |
| Categoria | Reazione/Counter |
| Priorità | 20 |
| Costo risorsa | — |
| Cooldown (turni) | 3 |
| Range (hex) | 0 |
| AoE Radius | 0 |
| Danno base | 10 |
| Control Strength | 0 |
| Durata (turni) | 0 |
| Loss/Contact Policy | Reaction.Trigger.DirectHit |
| Interazione terreno | Scudo 15 a sé + 10 danni all'attaccante |
| Gameplay Tags | `Ability.Reaction.Counter` |
| Implementation Status | IMPLEMENTED |
| Data Status | CANONICAL |

> Riusa Action.Counter con effetti componibili.

#### Uso tattico e limiti

Nella v0.1 attuale è una reazione deterministica che riusa `Action.Counter`; non apre ancora una finestra live di scelta. Il modello `opportunity → commit` arriva con E14.

## Fast Reactions / Reaction

### Descrizione delle reazioni

- **`Flux.ReactiveCapacitor`** — Si attiva quando Flux subisce un attacco diretto. Nella v0.1 corrente il commit è automatico: applica scudo 15 e 10 danni all'attaccante. È già descritta anche fra le abilità.

| Reaction_ID | Trigger | Tipo | Finestra_sec_SOURCE | Costo | Priorità | Scelta_A | Scelta_B | Default_Timeout | Tradeoff | Implementation_Status |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Flux.ReactiveCapacitor | Subisce un attacco diretto | Counter | — | — | 20 | Commit automatico (v0.1 attuale) | — | — | Scudo 15 + 10 danni all'attaccante | IMPLEMENTED |

> `Flux.ReactiveCapacitor` — Reazione deterministica attuale: nessuna finestra live; il modello opportunity→commit arriva con E14.

## Equipaggiamento

Per la v0.1 il workbook assegna agli eroi il **catalogo generico canonico**: varianti arma, gadget e moduli di reazione sono condivisi. La tabella sotto mostra tutti i valori disponibili nella matrice.

| Equipment_ID | Slot | Nome | Budget | Vantaggio | Svantaggio | Sinergia | Principio | Implementation_Status |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Weapon.Precision | Weapon | Precisione | — | +1 range | −4 danni | Attacco base | Scelta orizzontale | CANONICAL |
| Weapon.Impact | Weapon | Impatto | — | Applica Push 1 | −1 range | Attacco base | Scelta orizzontale | CANONICAL |
| Weapon.Overcharge | Weapon | Sovraccarico | — | +6 danni | Cooldown +1 | Attacco base | Scelta orizzontale | CANONICAL |
| Weapon.Split | Weapon | Multiplo | — | Bersaglio aggiuntivo | −6 danni | Attacco base | Scelta orizzontale | CANONICAL |
| Weapon.Suppressive | Weapon | Soppressione | — | Applica Slow | −5 danni | Attacco base | Scelta orizzontale | CANONICAL |
| Weapon.Environmental | Weapon | Ambientale | — | Migliora hazard | −5 danni diretti | Attacco base | Scelta orizzontale | CANONICAL |
| Gadget.Medkit | Gadget | Medkit | — | Cura 18 | — | Support | Cooldown 3 | CANONICAL |
| Gadget.BreachCharge | Gadget | Carica da breccia | — | 35 danni a struttura | — | Map | Cooldown 3 | CANONICAL |
| Gadget.Sprinkler | Gadget | Sprinkler | — | Acqua raggio 1 | — | Water combo | Cooldown 3 | CANONICAL |
| Gadget.Insulator | Gadget | Isolante | — | Immunità a una propagazione elettrica | — | Electric defense | Cooldown 3 | CANONICAL |
| Gadget.SmokeEmitter | Gadget | Emettitore di fumo | — | Fumo raggio 1 | — | Vision | Cooldown 3 | CANONICAL |
| Gadget.PortableCover | Gadget | Copertura portatile | — | Crea copertura bassa | — | Structures | Cooldown 3 | CANONICAL |
| Gadget.Sensor | Gadget | Sensore | — | Rivela un'area | — | Vision | Cooldown 3; raggio/durata non specificati | CANONICAL_PARTIAL |
| Gadget.Anchor | Gadget | Ancora | — | Impedisce una spinta | — | Displacement | Cooldown 3 | CANONICAL |
| Reaction.EmergencyDash | Reaction | Dash d'emergenza | — | Reposition 1 | — | Reaction | Trigger: sei bersagliato | CANONICAL |
| Reaction.ReactiveShield | Reaction | Scudo reattivo | — | Scudo 15 | — | Reaction | Trigger: subisci danno | CANONICAL |
| Reaction.CounterShot | Reaction | Contrattacco | — | 14 danni | — | Reaction | Trigger: sei colpito | CANONICAL |
| Reaction.AllyIntercept | Reaction | Interposizione | — | Cambia bersaglio | — | Reaction | Trigger: alleato bersagliato | CANONICAL |
| Reaction.HazardEscape | Reaction | Fuga hazard | — | Reposition 1 | — | Reaction | Trigger: cella diventa pericolosa | CANONICAL |
| Reaction.Cleanse | Reaction | Pulizia automatica | — | Rimuove lo stato | — | Reaction | Trigger: ricevi controllo | CANONICAL |
| Reaction.Anchor | Reaction | Ancoraggio | — | Annulla Push/Pull | — | Reaction | Trigger: ricevi Push/Pull | CANONICAL |

## Varianti

| Variant_ID | Nome | Vantaggio | Svantaggio | Incompatibile_Con | Specializzazione | Implementation_Status |
| --- | --- | --- | --- | --- | --- | --- |
| Flux.LinearDischarge.Concentrated | Scarica concentrata | +6 danni (30 totali) | Non si propaga a un secondo bersaglio | Flux.LinearDischarge.Branched | Burst | CANONICAL |
| Flux.LinearDischarge.Branched | Scarica ramificata | Un bersaglio aggiuntivo | −6 danni per bersaglio (18 ciascuno) | Flux.LinearDischarge.Concentrated | Multi-target | CANONICAL |

## Talenti

_NOT DEFINED nelle tabelle correnti; nessun talento è stato inventato._

## Stato produzione

| Campo | Valore |
| --- | --- |
| Page Status | DATA_READY |
| Stats | PARTIAL_CANONICAL |
| Vision | BASELINE |
| Mobility | PARTIAL_CANONICAL |
| Signature | DEFINED |
| Abilities | DEFINED (5) |
| Reactions | DEFINED (1) |
| Resource | DEFINED |
| Equipment | CANONICAL GENERIC |
| Variants | DEFINED (2) |
| Talents | NOT DEFINED |
| Release | v0.1 |

## Stato della pagina

Questa pagina è **operativa per la v0.1**: numeri e semantiche competitive derivano dai cataloghi/versioni correnti della repository. Le descrizioni narrative aggiunte qui sono una vista editoriale delle stesse regole e **non introducono nuovi effetti**. Le voci `PARTIAL` / `DEFERRED_E14` restano esplicitamente non complete.

## Governance

- **Dataset:** `docs/src/data/characters-wiki-data-v0.4.xlsx`.
- **Release dati:** `v0.1`.
- I campi `—` / `TBD` sono intenzionali: indicano che la fonte non definisce quel valore.
- La Wiki è una vista documentale; il runtime non deve usare Markdown come fonte competitiva.
- Per la v0.1, i valori competitivi canonici restano i cataloghi versionati sotto `docs/balance/` e l'implementazione C++ verificata dai test.
