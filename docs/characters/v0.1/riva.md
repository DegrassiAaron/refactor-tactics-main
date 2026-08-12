# Riva

> **Asset base:** Paragon — Phase  
> **Hero_Key:** `ASSET_RIVA`  
> **RT Character ID:** `Hero.Riva`  
> **Release:** `v0.1`  
> **Roster status:** Release v0.1  
> **Provenienza visuale:** mesh e animazioni vengono dallo slot Paragon **Phase** ([D-037](../../decisions/RT_PDR_00_Decision_Log.md) · tabella owner in [`paragon.md`](../paragon.md)). L'asset è la base visuale del prototipo, non l'identità di Riva.

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ENV-SYSTEMIC-COMBOS -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ENV-SYSTEMIC-COMBOS` · Release: `v0.1` · Roadmap: `E8.5`  
> Stato: **INTEGRATED** · Gate: `7/9`  
> Scenario: `Visual.Combat.WaterElectric`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ENV-SYSTEMIC-COMBOS -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ENV-WATER -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ENV-WATER` · Release: `v0.1` · Roadmap: `E8.1, E8.4`  
> Stato: **INTEGRATED** · Gate: `6/9`  
> Scenario: `Visual.Environment.WetExtinguishesFire`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ENV-WATER -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-CHAR-V01-ROSTER -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-CHAR-V01-ROSTER` · Release: `v0.1` · Roadmap: `E6.1, E6.2, E6.3, E6.4, E6.5, E6.6, E6.7`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Combat.BasicAttack`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-CHAR-V01-ROSTER -->

## Panoramica

Support/controller dell'acqua: bagna bersagli, cura, riposiziona e prepara il terreno per interazioni sistemiche e sinergie di squadra.

## Identità tattica

| Campo | Valore |
| --- | --- |
| Ruolo primario | Support |
| Ruolo secondario | Controller |
| Macro ruolo Signature | Support |
| Specializzazione | Water Terrain Manipulator |
| Profilo range | Medium |
| Tipo danno | Water |
| Complessità gameplay | TBD |
| Complessità tecnica (1–5) | TBD |
| Risorsa firma | Riserva Idrica |
| Signature primaria | Water Shaping |
| Signature secondaria | Wet Setup |
| Framework principali | Environment, Support, Movement, Status |
| Dipendenze tecniche | Water, Wet, smoke, reposition |
| Player question | Dove preparo il terreno per la squadra? |

> **Nota bilanciamento:** Canonico v0.1. 95 HP, 5 MP, vista 5; valore massimo quando abilita interazioni e controllo terreno.

## Meccanica firma

### Descrizione della meccanica

**Water Shaping** fa di Riva il principale personaggio di setup ambientale della v0.1. La sua acqua non è solo un tema visivo: serve a applicare `Wet`, spostare unità, sostenere gli alleati e preparare interazioni successive, soprattutto con l'elettricità.

La **Riserva Idrica** ha cap 4 e recupera 1 tramite interazioni con l'acqua; il valore iniziale non è ancora specificato. Il kit alterna effetti immediati (`PressureJet`, `CircularTide`) a trasformazioni del campo (`FluidTrail`, `MistVeil`) e a un riposizionamento reattivo (`FlowReaction`, rinviato a E14).

Il suo payoff cresce quando la squadra sfrutta le celle e gli stati che Riva ha preparato. Il controgioco consiste nel non restare nelle zone predisposte, interrompere la continuità del setup e sfruttare il fatto che l'acqua può diventare un vettore utile anche all'elettricità avversaria.

### Lettura tattica

**Obiettivo del giocatore.** Creare valore di squadra attraverso `Wet`, spinta, cura, acqua e fumo. Riva è più efficace quando il turno successivo o un alleato possono sfruttare ciò che ha preparato.

**Misplay / Failure State.** Preparare il terreno nel posto sbagliato. È il failure state più severo del roster v0.1, perché non si limita a sprecare la Riserva Idrica: `Wet` è uno **stato della cella**, e il bonus elettrico lo legge senza sapere chi l'ha applicato — la stessa proprietà registrata da [D-029](../../decisions/RT_PDR_00_Decision_Log.md) a proposito di `Water-Electric`. Una superficie bagnata piazzata male non è un investimento perso: è un **vettore consegnato all'avversario**, che può usarla contro la squadra di Riva. Gli altri tre eroi v0.1, sbagliando, perdono valore; Riva può regalarlo.

**Counterplay / rischio.** Il setup può essere evitato o sfruttato dall'avversario. Inoltre `FlowReaction` non è ancora attiva nella v0.1 corrente perché dipende dalle decision boundary di E14.

### Dati della meccanica

| Campo | Valore |
| --- | --- |
| Mechanic ID | `MECH_RIVA_WATER_SHAPING` |
| Nome | Water Shaping |
| Scope | Exclusive |
| Tipo | Primary Signature |
| Secondaria | Wet Setup |
| Framework | Environment, Support, Movement |
| Dipendenze tecniche | Water, Wet, smoke, reposition |
| Player question | Dove preparo il terreno per la squadra? |
| State | Riserva Idrica + superfici/stati d'acqua |
| Activation / Trigger | Interazioni con acqua e abilità di setup |
| Payoff | Cura, Wet, controllo terreno e riposizionamento |
| Misplay / Failure State | Bagnare l'area sbagliata: `Wet` è stato della cella e non conosce chi l'ha applicato (D-029) → la superficie diventa un vettore sfruttabile **dall'avversario** |
| Counterplay | Disperdere il setup, elettricità nemica, uscire dalle zone preparate |
| Telegraphing | Stato ambientale pubblico/observed; risorsa team-visible |
| Design Status | IMPLEMENTED |

> FlowReaction resta rinviata a E14; il resto del kit è catalogato.

## Statistiche base

| Campo | Valore |
| --- | --- |
| HP | 95 |
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

**Stato dati:** `CANONICAL_PARTIAL` — Canonico: HP 95, Move 5. Altri attributi di questa matrice non sono definiti nel catalogo v0.1.

## Visione e stealth

| Campo | Valore |
| --- | --- |
| Sight Range (hex) | 5 |
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

> Vista 5 canonica. Detection 48 / Identification 45 / Stealth 2 / Tracking 1 sono baseline catalogo, non attive nello slice binario v0.1.

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

**Stato dati:** `CANONICAL_PARTIAL` — FluidTrail è un Dash 3 specifico dell'eroe.

## Risorsa firma

| Resource_ID | Nome | Cap | Start | Regen | Regen_Trigger | Spesa | Regola | Audience | Implementation_Status |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| RES_RIVA_WATER | Riserva Idrica | 4 | — | 1 | Interazione con acqua | Abilità firma | Cap 4; start non specificato | Team-visible | CANONICAL_PARTIAL |

> **Ownership del kit:** le abilità di questa pagina appartengono esclusivamente a questo personaggio. Le sinergie con altri eroi sono esempi derivati da stati, superfici, geometria e altre regole comuni; non sono abilità condivise. Vedi [Sinergie e combinazioni](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/sinergie-e-combinazioni).

## Profilo di attacco base

| Campo | Valore |
| --- | --- |
| Ability ID | `Riva.PressureJet` |
| Famiglia | **Setup** |
| Danno / portata | 16 · range 5 · forma a linea |
| Payload oltre il danno | `Status.Wet` 1 turno · `Push 1` |
| Dipendenza dal base | ★★★☆☆ — è il suo strumento di preparazione, non la sua fonte di danno |

> **È l'unico dei quattro che nasceva già conforme.** Quando ADR-0007 ha fissato le famiglie, `PressureJet`
> non ha richiesto nessuna modifica: era già l'esempio che dimostrava la tesi.

### Il test della falsa scelta

| Domanda | Risposta |
| --- | --- |
| Quando è la scelta corretta? | Quando serve `Wet` su un bersaglio per la scarica elettrica **nello stesso Blast** · quando la spinta di 1 sposta qualcuno da una cella che gli serve |
| Quando è inferiore a un'abilità firma? | Quando serve danno adesso: 16 è il secondo valore più basso del roster, e chi la gioca cercando DPS la sta usando male |
| Che cosa risparmia? | Il cooldown delle sue abilità d'area, e il turno di Flux: la combo non richiede che nessuno dei due spenda una firma |
| Che counterplay esiste? | Il tempo. `Wet` dura **1 turno** e `TickStatuses` lo rimuove nel Cleanup dello stesso turno ([D-036](../../decisions/RT_PDR_00_Decision_Log.md)): la finestra è un solo Blast, e chi conosce l'ordine di priorità sa quando è già passata |
| Che cosa impara il giocatore? | Che il valore di un attacco è quello che **prepara**, non quello che toglie |

### Prove

| Che cosa | Dove |
| --- | --- |
| Il payload è nel dato | `RefactorTactics.Heroes.Riva.MatchesCatalog` |
| L'effetto si vede in partita | `Visual.Combat.WaterElectricCoordinated` — il `Wet` non è osservabile direttamente (il runner non ha `UnitHasStatus`), quindi lo scenario lo prova con l'aritmetica: `100 − 16 − 32 = 52`, dove i 32 valgono solo se il bersaglio è bagnato. Senza `Wet` sarebbe 60, e lo scenario diventa rosso |

## Abilità

### Pressure Jet

#### Descrizione

Pressure Jet è l'attacco base tematico di Riva: una linea a range 5 che infligge 16 danni, applica `Wet` per 1 turno e `Push 1`.

| Campo | Valore |
| --- | --- |
| Ability ID | `Riva.PressureJet` |
| Categoria | Attacco base lineare |
| Priorità | 50 |
| Costo risorsa | — |
| Cooldown (turni) | 0 |
| Range (hex) | 5 |
| AoE Radius | 0 |
| Danno base | 16 |
| Control Strength | 1 |
| Durata (turni) | 1 |
| Loss/Contact Policy | Fallback.AttackCell |
| Interazione terreno | Applica Wet 1 e Push 1 |
| Gameplay Tags | `Ability.Offense.Line.Water` |
| Implementation Status | IMPLEMENTED |
| Data Status | CANONICAL |

> Attacco base tematico: 16 danni, range 5.

#### Uso tattico e limiti

Fa tre cose con una sola azione leggibile: danno leggero, spostamento e setup. Il suo valore principale è spesso la posizione finale o il `Wet` lasciato al team, non il danno grezzo.

### Circular Tide

#### Descrizione

Circular Tide è l'AoE di supporto di Riva. A range 4 e raggio 1, cura 18 agli alleati e applica `Wet` ai nemici per 1 turno.

| Campo | Valore |
| --- | --- |
| Ability ID | `Riva.CircularTide` |
| Categoria | AoE support |
| Priorità | 60 |
| Costo risorsa | — |
| Cooldown (turni) | 2 |
| Range (hex) | 4 |
| AoE Radius | 1 |
| Danno base | 0 |
| Control Strength | 0 |
| Durata (turni) | 1 |
| Loss/Contact Policy | Fallback.AttackCell |
| Interazione terreno | Cura 18 agli alleati e applica Wet ai nemici |
| Gameplay Tags | `Ability.Support.AOE.Water` |
| Implementation Status | PARTIAL |
| Data Status | CANONICAL |

> Resolver ally/enemy differenziato nella stessa AoE è limite dichiarato.

#### Uso tattico e limiti

La specifica è `PARTIAL` perché il resolver corrente non differenzia ancora pienamente effetti alleati/nemici nella stessa AoE. Le varianti scelgono fra più cura (`Healing`) o più controllo (`Impact`).

### Fluid Trail

#### Descrizione

Fluid Trail è un Dash lineare di 3 celle che dovrebbe lasciare acqua lungo il percorso. Il movimento è la parte già rappresentata; la creazione dinamica dell'acqua lungo il path resta parziale.

| Campo | Valore |
| --- | --- |
| Ability ID | `Riva.FluidTrail` |
| Categoria | Dash |
| Priorità | 30 |
| Costo risorsa | — |
| Cooldown (turni) | 2 |
| Range (hex) | 3 |
| AoE Radius | 0 |
| Danno base | 0 |
| Control Strength | 0 |
| Durata (turni) | 0 |
| Loss/Contact Policy | Fallback.Stop |
| Interazione terreno | Dash 3 e crea acqua lungo il percorso |
| Gameplay Tags | `Ability.Mobility.Dash.Water` |
| Implementation Status | PARTIAL |
| Data Status | CANONICAL |

> Movimento lineare dichiarato; creazione acqua lungo path dipende dal terreno dinamico.

#### Uso tattico e limiti

È insieme mobilità e setup: Riva cambia posizione mentre prepara celle utili a Wet, conduzione e controllo ambientale. Il fallback è `Stop` se il Dash non può essere completato.

### Mist Veil

#### Descrizione

Mist Veil crea fumo in un'area di raggio 1 attorno al bersaglio. È pensata per modificare la leggibilità e le linee di visione del campo, non per infliggere danno.

| Campo | Valore |
| --- | --- |
| Ability ID | `Riva.MistVeil` |
| Categoria | Environment/AoE |
| Priorità | 60 |
| Costo risorsa | — |
| Cooldown (turni) | 3 |
| Range (hex) | 4 |
| AoE Radius | 1 |
| Danno base | 0 |
| Control Strength | 0 |
| Durata (turni) | 2 |
| Loss/Contact Policy | Fallback.Cancel |
| Interazione terreno | Crea `Terrain.Smoke` raggio 1, per 2 turni; il cap di targeting a 2 celle vale appena la cella cambia |
| Gameplay Tags | `Ability.Vision.Smoke` |
| Implementation Status | IMPLEMENTED |
| Data Status | CANONICAL |

> **Aggiornata il 2026-08-09 (issue `#353`).** Dichiarava «crea fumo raggio 1» e **non lo faceva**: `Smoke` era
> l'unica delle otto superfici che nessuna azione sapeva creare. Tre valori di questa tabella sono cambiati come
> conseguenza, e nessuno è stato inventato:
>
> - **Categoria e fase**: `Prep` → `Environment`. `ResolveEnvironment` — l'unico posto che crea superfici —
>   processa solo le azioni che risolvono nel Cleanup; in Prep l'abilità non ci arrivava mai. Stesso movimento
>   che [D-046](../../decisions/RT_PDR_00_Decision_Log.md) ha fatto su `FluidTrail`.
> - **Range 0 → 4** e **priorità 35 → 60**: sono i valori di entrambe le azioni ambientali del core
>   (`Action.Ignite`, `Action.CreateWater`). Il `range 0` era dichiarato «placeholder tecnico» e non è mai stato
>   un numero di bilanciamento; la priorità ordina **dentro** la fase, quindi il 35 scelto per il Prep non aveva
>   più significato dove l'azione è andata a vivere.
> - **Durata 2 turni**: quella di ogni superficie dinamica, poi la cella torna alla superficie che aveva.
>
> **Conseguenza di gioco da conoscere**: il fumo si alza nel Cleanup, quindi copre il turno **seguente**. Si
> prepara un attraversamento, non si rompe una linea nell'istante.

#### Uso tattico e limiti

Il valore tattico è proteggere un attraversamento o preparare un cambio di posizione. **Non** rompere una linea di tiro nell'istante: il fumo si alza nel Cleanup, quindi si gioca un turno prima di quando serve.

Un limite resta, ed è dichiarato invece che nascosto: chi **si trova già** nella cella quando il fumo si alza non riceve `Status.Obscured`, che è applicato dagli `OnEnterEffects` a chi *entra*. Non cambia l'effetto tattico — il cap di targeting a 2 celle è letto dalla **superficie della cella** da combat, bot e percezione, quindi vale da subito per tutti — ma è la stessa asimmetria che `Action.CreateWater` risolve esplicitamente per `Wet`.

### Flow Reaction

#### Descrizione

Flow Reaction prevede un `Reposition 1` dopo che Riva subisce un attacco. È una reazione di movimento e quindi richiede una decision boundary durante la resolution.

| Campo | Valore |
| --- | --- |
| Ability ID | `Riva.FlowReaction` |
| Categoria | Reazione/Reposition |
| Priorità | 36 |
| Costo risorsa | — |
| Cooldown (turni) | 3 |
| Range (hex) | 0 |
| AoE Radius | 0 |
| Danno base | 0 |
| Control Strength | 0 |
| Durata (turni) | 0 |
| Loss/Contact Policy | DecisionBoundary.E14 |
| Interazione terreno | Reposition 1 dopo un attacco subito |
| Gameplay Tags | `Ability.Reaction.Movement` |
| Implementation Status | DEFERRED_E14 |
| Data Status | CANONICAL |

> Slot None nel dato corrente per evitare false attivazioni.

#### Uso tattico e limiti

Per evitare TurnLog falsi è esplicitamente `DEFERRED_E14`: nel dato corrente non occupa ancora uno slot Reaction attivo.

## Fast Reactions / Reaction

### Descrizione delle reazioni

- **`Riva.FlowReaction`** — Dopo un attacco subito, la specifica prevede `Reposition 1`. Richiede una decision boundary e resta `DEFERRED_E14`; la baseline di 3 s appartiene al modello futuro, non al comportamento runtime corrente.

| Reaction_ID | Trigger | Tipo | Finestra_sec_SOURCE | Costo | Priorità | Scelta_A | Scelta_B | Default_Timeout | Tradeoff | Implementation_Status |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Riva.FlowReaction | Dopo un attacco subito | Movement reaction | 3 | — | 36 | Reposition 1 | Hold | Hold | Movimento dentro decision boundary | DEFERRED_E14 |

> ⚠️ **Review required:** una o più finestre temporali sono valori sorgente/storici. Il modello corrente di Fast Reaction usa una baseline di 3,0 s; questi valori vanno riallineati prima dell'implementazione.
> `Riva.FlowReaction` — Rinviata a E14; durata 3 s è baseline del modello Fast Reaction, non implementazione corrente.

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
| Riva.CircularTide.Healing | Marea curativa | Cura 24 invece di 18 | Non applica Wet ai nemici | Riva.CircularTide.Impact | Support | CANONICAL |
| Riva.CircularTide.Impact | Marea d'urto | Applica Push 1 ai nemici | Cura solo 10 | Riva.CircularTide.Healing | Control | CANONICAL |

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
