# Bastion

> ✅ **Stato repository:** personaggio del roster **v0.1 canonico**. La pagina riassume i dati del workbook v0.4, che per i numeri v0.1 è un mirror documentale dei cataloghi versionati della repository. Quando una voce è `PARTIAL` o `DEFERRED_E14`, la pagina lo espone esplicitamente.

> **Asset base:** Bastion  
> **Hero_Key:** `ASSET_BASTION`  
> **RT Character ID:** `Hero.Bastion`  
> **Release:** `v0.1`  
> **Design status:** `IMPLEMENTED`  
> **Roster status:** Release v0.1

## Panoramica

Guardian/architetto del campo: crea e riconfigura coperture, controlla gli spazi e si interpone per gli alleati.

## Identità tattica

| Campo | Valore |
| --- | --- |
| Ruolo primario | Guardian |
| Ruolo secondario | Controller |
| Macro ruolo Signature | Guardian |
| Specializzazione | Field Architect |
| Profilo range | Short |
| Tipo danno | Kinetic |
| Complessità gameplay | TBD |
| Complessità tecnica (1–5) | TBD |
| Risorsa firma | Integrità Strutturale |
| Signature primaria | Field Architecture |
| Signature secondaria | Ally Interposition |
| Framework principali | Structures, Cover, Reaction, Displacement |
| Dipendenze tecniche | Cover, structures, arc manipulation, intercept |
| Player question | Quale spazio chiudo e chi proteggo? |

> **Nota bilanciamento:** Canonico v0.1. 120 HP, 4 MP, vista 5, Push Resistance 1: compra stabilità con mobilità ridotta.

## Meccanica firma

### Descrizione della meccanica

**Field Architecture** permette a Bastion di trattare coperture e strutture come parte del proprio kit. Il suo valore non deriva solo dagli HP: decide quali linee rendere sicure, quali spazi chiudere e quando trasformare la propria resistenza in protezione diretta per un alleato.

L'**Integrità Strutturale** ha cap 4 e recupera 1 nel Cleanup; il valore iniziale non è ancora specificato. `KineticPanel` e `Reconfigure` esprimono il lato di controllo della mappa, mentre `Ram` e `Interposition` gli permettono di convertire presenza fisica e posizione in pressione e protezione.

Il controgioco è geometrico: aggirare le coperture, distruggere o rendere irrilevanti le strutture, separare Bastion dagli alleati che vuole proteggere e costringerlo a scegliere tra tenere una linea e intervenire altrove.

### Lettura tattica

**Obiettivo del giocatore.** Controllare una porzione di mappa con coperture e presenza fisica, quindi proteggere l'alleato giusto con `Interposition` o convertire la posizione in pressione con `Ram`.

**Counterplay / rischio.** Ha 4 MP, il valore più basso del roster v0.1. Flank, separazione dagli alleati e distruzione delle strutture riducono il valore della sua architettura.

### Dati della meccanica

| Campo | Valore |
| --- | --- |
| Mechanic ID | `MECH_BASTION_FIELD_ARCHITECTURE` |
| Nome | Field Architecture |
| Scope | Exclusive |
| Tipo | Primary Signature |
| Secondaria | Ally Interposition |
| Framework | Structures, Cover, Reaction |
| Dipendenze tecniche | Cover, structures, topology, intercept |
| Player question | Quale spazio chiudo e chi proteggo? |
| State | Integrità Strutturale + coperture/strutture controllate |
| Activation / Trigger | Crea/riconfigura strutture e reagisce agli attacchi agli alleati |
| Payoff | Modifica linee e rotte, assorbe pressione al posto degli alleati |
| Counterplay | Flank, mobilità, distruzione strutture, separazione dagli alleati |
| Telegraphing | Strutture pubbliche; intenti di reazione team-only durante planning |
| Design Status | IMPLEMENTED |

> Interposition è cablata; KineticPanel/Reconfigure dipendono dal sistema strutture.

## Statistiche base

| Campo | Valore |
| --- | --- |
| HP | 120 |
| Armatura | — |
| Resistenza | — |
| Movimento base | 4 |
| Iniziativa | — |
| Precisione (1–10) | — |
| Potenza (1–10) | — |
| Controllo (1–10) | — |
| Supporto (1–10) | — |
| Durabilità (1–10) | — |
| Indice Combat | — |
| Budget Punti | — |
| Delta Budget | — |

**Stato dati:** `CANONICAL_PARTIAL` — Canonico: HP 120, Move 4. Altri attributi di questa matrice non sono definiti nel catalogo v0.1.

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
| Move Hex / MP | 4 |
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
| Knockback Resist | 1 |
| Collision Priority | — |

> Move 4 MP e Push Resistance 1 canonici. Altri modificatori di mobilità non sono definiti per eroe.

**Stato dati:** `CANONICAL_PARTIAL` — Ram riusa la semantica di Charge 3.

## Risorsa firma

| Resource_ID | Nome | Cap | Start | Regen | Regen_Trigger | Spesa | Regola | Audience | Implementation_Status |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| RES_BASTION_STRUCTURE | Integrità Strutturale | 4 | — | 1 | Cleanup | Abilità firma | Cap 4; start non specificato | Team-visible | CANONICAL_PARTIAL |

> **Ownership del kit:** le abilità di questa pagina appartengono esclusivamente a questo personaggio. Le sinergie con altri eroi sono esempi derivati da stati, superfici, geometria e altre regole comuni; non sono abilità condivise. Vedi [Sinergie e combinazioni](../../wiki/game/sinergie-e-combinazioni.md).

## Abilità

### Impact Shot

#### Descrizione

Impact Shot è l'attacco base di Bastion: 24 danni a range 3. È semplice e corto, coerente con un Guardian che vuole stare vicino alle linee e agli alleati che protegge.

| Campo | Valore |
| --- | --- |
| Ability ID | `Bastion.ImpactShot` |
| Categoria | Attacco base |
| Priorità | 50 |
| Costo risorsa | — |
| Cooldown (turni) | 0 |
| Range (hex) | 3 |
| AoE Radius | 0 |
| Danno base | 24 |
| Control Strength | 0 |
| Durata (turni) | 0 |
| Loss/Contact Policy | Fallback.Cancel |
| Interazione terreno | Attacco cinetico corto |
| Gameplay Tags | `Ability.Offense.Kinetic` |
| Implementation Status | IMPLEMENTED |
| Data Status | CANONICAL |

> 24 danni, range 3.


#### Uso tattico e limiti

Serve come pressione diretta quando non è necessario spendere il turno per costruire o riconfigurare il campo.

### Kinetic Panel

#### Descrizione

Kinetic Panel è un'azione di Prep che crea una copertura con integrità base 30. È la manifestazione principale di Field Architecture: Bastion modifica il campo prima che gli attacchi del Blast vengano risolti.

| Campo | Valore |
| --- | --- |
| Ability ID | `Bastion.KineticPanel` |
| Categoria | Prep/Cover |
| Priorità | 30 |
| Costo risorsa | — |
| Cooldown (turni) | 2 |
| Range (hex) | 1 |
| AoE Radius | 0 |
| Danno base | 0 |
| Control Strength | 0 |
| Durata (turni) | 0 |
| Loss/Contact Policy | Fallback.Cancel |
| Interazione terreno | Crea copertura da 30 HP |
| Gameplay Tags | `Ability.Defense.Cover` |
| Implementation Status | PARTIAL |
| Data Status | CANONICAL |

> Dipende dal sistema strutture; varianti definiscono integrità/durata.


#### Uso tattico e limiti

La creazione completa della struttura è `PARTIAL`. Le varianti scambiano durata, integrità e flessibilità: `Reinforced` sale a 45 ma dura 1 turno; `Adaptive` scende a 25 e concede una rotazione gratuita.

### Reconfigure

#### Descrizione

Reconfigure sposta o ruota una copertura esistente entro range 1. Non crea nuovo valore dal nulla: converte una struttura già presente nella geometria più utile per il turno corrente.

| Campo | Valore |
| --- | --- |
| Ability ID | `Bastion.Reconfigure` |
| Categoria | Prep/Map |
| Priorità | 31 |
| Costo risorsa | — |
| Cooldown (turni) | 2 |
| Range (hex) | 1 |
| AoE Radius | 0 |
| Danno base | 0 |
| Control Strength | 0 |
| Durata (turni) | 0 |
| Loss/Contact Policy | Fallback.Cancel |
| Interazione terreno | Sposta o ruota una copertura esistente |
| Gameplay Tags | `Ability.Control.Map` |
| Implementation Status | PARTIAL |
| Data Status | CANONICAL |

> Dipende dal targeting delle strutture.


#### Uso tattico e limiti

È `PARTIAL` perché dipende dal targeting delle strutture. Il suo payoff è cambiare linee e angoli senza dover ricreare una copertura.

### Ram

#### Descrizione

Ram riusa la semantica di `Action.Charge`: movimento lineare fino a 3 celle, 20 danni al primo impatto e `Push 1`, con cooldown 2.

| Campo | Valore |
| --- | --- |
| Ability ID | `Bastion.Ram` |
| Categoria | Charge |
| Priorità | 35 |
| Costo risorsa | — |
| Cooldown (turni) | 2 |
| Range (hex) | 3 |
| AoE Radius | 0 |
| Danno base | 20 |
| Control Strength | 1 |
| Durata (turni) | 0 |
| Loss/Contact Policy | Fallback.Stop |
| Interazione terreno | Charge 3, 20 danni, Push 1 |
| Gameplay Tags | `Ability.Mobility.Charge.Kinetic` |
| Implementation Status | IMPLEMENTED |
| Data Status | CANONICAL |

> Riusa Action.Charge e LinearCharge.


#### Uso tattico e limiti

È l'opzione con cui Bastion trasforma massa e presenza in spostamento offensivo. Essendo una charge, può essere fermata dalla topologia e usa `Fallback.Stop`.

### Interposition

#### Descrizione

Interposition è la reazione firma di protezione: quando un alleato entro 2 celle è bersagliato da un attacco diretto, Bastion diventa il bersaglio del colpo.

| Campo | Valore |
| --- | --- |
| Ability ID | `Bastion.Interposition` |
| Categoria | Reazione/Intercept |
| Priorità | 10 |
| Costo risorsa | — |
| Cooldown (turni) | 3 |
| Range (hex) | 2 |
| AoE Radius | 0 |
| Danno base | 0 |
| Control Strength | 0 |
| Durata (turni) | 0 |
| Loss/Contact Policy | Reaction.Trigger.AllyTargeted |
| Interazione terreno | Bastion diventa il bersaglio di un attacco diretto a un alleato entro 2 |
| Gameplay Tags | `Ability.Reaction.Intercept` |
| Implementation Status | IMPLEMENTED |
| Data Status | CANONICAL |

> Riusa Action.Intercept; cooldown eroe 3.


#### Uso tattico e limiti

È già implementata riusando `Action.Intercept`. Non aggiunge danno o scudo: il suo valore è cambiare chi riceve l'attacco, sfruttando la maggiore resistenza di Bastion.

## Fast Reactions / Reaction

### Descrizione delle reazioni

- **`Bastion.Interposition`** — Quando un alleato entro 2 celle è bersagliato da un attacco diretto, Bastion intercetta automaticamente nella v0.1 corrente e diventa il bersaglio. È già descritta anche fra le abilità.

| Reaction_ID | Trigger | Tipo | Finestra_sec_SOURCE | Costo | Priorità | Scelta_A | Scelta_B | Default_Timeout | Tradeoff | Implementation_Status |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Bastion.Interposition | Alleato entro 2 celle bersagliato da attacco diretto | Intercept | — | — | 10 | Intercept automatico (v0.1 attuale) | — | — | Bastion diventa bersaglio | IMPLEMENTED |

> `Bastion.Interposition` — Reazione deterministica attuale; nessuna finestra live.

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
| Bastion.KineticPanel.Reinforced | Pannello rinforzato | Integrità 45 | Durata 1 turno | Bastion.KineticPanel.Adaptive | Guardian | CANONICAL |
| Bastion.KineticPanel.Adaptive | Pannello adattivo | 1 rotazione gratuita | Integrità 25 | Bastion.KineticPanel.Reinforced | Controller | CANONICAL |

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

- **Dataset:** `RefactorTactics_Characters_Wiki_Data_v0.4.xlsx`.
- **Release dati:** `v0.1`.
- I campi `—` / `TBD` sono intenzionali: indicano che la fonte non definisce quel valore.
- La Wiki è una vista documentale; il runtime non deve usare Markdown come fonte competitiva.
- Per la v0.1, i valori competitivi canonici restano i cataloghi versionati sotto `docs/balance/` e l'implementazione C++ verificata dai test.
