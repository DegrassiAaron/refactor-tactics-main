# Wraith

> **Asset base:** Paragon — Wraith  
> **Hero_Key:** `ASSET_VEKTOR`  
> **RT Character ID:** `Hero.Wraith`  
> **Release:** `v0.1`  
> **Roster status:** Release v0.1  
> **Provenienza visuale:** mesh e animazioni vengono dallo slot Paragon **Wraith** ([D-037](../../decisions/RT_PDR_00_Decision_Log.md) · tabella owner in [`paragon.md`](../paragon.md)). L'asset è la base visuale del prototipo, non l'identità del personaggio.

## Panoramica

Duellante predittivo: il più mobile del roster, punisce traiettorie e movimento con dash, intercetti e deviazioni.

## Identità tattica

| Campo | Valore |
| --- | --- |
| Ruolo primario | Striker |
| Ruolo secondario | Controller |
| Macro ruolo Signature | Striker |
| Specializzazione | Predictive Duelist |
| Profilo range | Medium |
| Tipo danno | Kinetic |
| Complessità gameplay | TBD |
| Complessità tecnica (1–5) | TBD |
| Risorsa firma | Slancio |
| Signature primaria | Predictive Interception |
| Signature secondaria | Movement Punish |
| Framework principali | Movement, Reaction, Prediction, Control |
| Dipendenze tecniche | Movement triggers, facing, reaction boundaries |
| Player question | Dove passerà il nemico? |

> **Nota bilanciamento:** Canonico v0.1. 100 HP, 6 MP, vista 6; mobilità alta in cambio di minori strumenti difensivi.

## Meccanica firma

### Descrizione della meccanica

**Predictive Interception** premia la lettura delle traiettorie avversarie. Wraith ha il movimento più alto del roster v0.1 e usa questo vantaggio per occupare linee favorevoli, attraversare il campo e punire movimenti che diventano prevedibili.

Lo **Slancio** ha cap 4 e recupera 1 quando viene eseguito movimento; il valore iniziale non è ancora specificato. `InterceptShot` è la manifestazione più diretta della meccanica ed è **giocabile**: si dichiara in pianificazione e si risolve da sola al passaggio dell'avversario, senza chiedere nulla durante la risoluzione. `PassingBlade`, `Deflection` e `Feint` coprono rispettivamente mobilità offensiva, difesa reattiva e previsione.

Il controgioco consiste nel cambiare rotta, usare coperture e LOS per negare le linee preparate, fare bait delle reazioni e impedire a Wraith di convertire mobilità in un duello favorevole.

### Lettura tattica

**Obiettivo del giocatore.** Usare 6 MP e Dash per prendere linee favorevoli, leggere dove passerà l'avversario e trasformare quella previsione in intercetto, attraversamento offensivo o controllo.

**Misplay / Failure State.** Leggere male la traiettoria. La previsione è dichiarata per intero in Planning e non riceve informazione nuova: se il nemico non passa dove Wraith ha scommesso, l'azione **risolve lo stesso** come whiff o fallback dichiarato — è la regola di [D-016](../../decisions/RT_PDR_00_Decision_Log.md), non una nota editoriale. Il costo non è il danno mancato ma il turno: l'azione offensiva è già spesa ([D-012](../../decisions/RT_PDR_00_Decision_Log.md), `Attack` **oppure** `Ability` **oppure** `Overwatch`), e Wraith ha speso mobilità per mettersi sulla linea sbagliata. È l'unico failure state del roster v0.1 che è già una **regola decisa** invece che una descrizione.

**Counterplay / rischio.** Se l'avversario cambia rotta, chiude LOS o forza Wraith a spendere la reazione sul bersaglio sbagliato, il payoff predittivo cala. `InterceptShot` resta rinviata a E14.

### Dati della meccanica

| Campo | Valore |
| --- | --- |
| Mechanic ID | `MECH_VEKTOR_PREDICTIVE_INTERCEPTION` |
| Nome | Predictive Interception |
| Scope | Exclusive |
| Tipo | Primary Signature |
| Secondaria | Movement Punish |
| Framework | Movement, Reaction, Prediction |
| Dipendenze tecniche | Movement triggers, decision boundaries, facing |
| Player question | Dove passerà il nemico? |
| State | Slancio + zone/traiettorie controllate |
| Activation / Trigger | Movimento eseguito e azioni di intercetto |
| Payoff | Punisce traiettorie prevedibili e usa mobilità superiore per il duello |
| Misplay / Failure State | Previsione errata → whiff o fallback dichiarato (D-016); azione offensiva del turno già spesa e posizione presa sulla linea sbagliata |
| Counterplay | Cambiare rotta, chiudere LOS, strutture, bait delle reazioni |
| Telegraphing | Zona armata/trigger osservabili secondo regole di informazione |
| Design Status | IMPLEMENTED_PARTIAL |

> Deflection è cablata; InterceptShot è **giocabile** come Predictive Action dal 2026-08-10 (E18).

## Statistiche base

| Campo | Valore |
| --- | --- |
| HP | 100 |
| Armatura | — |
| Resistenza | — |
| Movimento base | 6 |
| Iniziativa | — |
| Precisione (1–10) | — |
| Potenza (1–10) | — |
| Controllo (1–10) | — |
| Supporto (1–10) | — |
| Durabilità (1–10) | — |
| Indice Combat | — |
| Budget Punti | — |
| Delta Budget | — |

**Stato dati:** `CANONICAL_PARTIAL` — Canonico: HP 100, Move 6. Altri attributi di questa matrice non sono definiti nel catalogo v0.1.

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
| Move Hex / MP | 6 |
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

> Move 6 MP e Push Resistance 0 canonici. Altri modificatori di mobilità non sono definiti per eroe.

**Stato dati:** `CANONICAL_PARTIAL` — PassingBlade è un Dash 3 lineare.

## Risorsa firma

| Resource_ID | Nome | Cap | Start | Regen | Regen_Trigger | Spesa | Regola | Audience | Implementation_Status |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| RES_VEKTOR_MOMENTUM | Slancio | 4 | — | 1 | Movimento eseguito | Abilità firma | Cap 4; start non specificato | Team-visible | CANONICAL_PARTIAL |

> **Ownership del kit:** le abilità di questa pagina appartengono esclusivamente a questo personaggio. Le sinergie con altri eroi sono esempi derivati da stati, superfici, geometria e altre regole comuni; non sono abilità condivise. Vedi [Sinergie e combinazioni](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/sinergie-e-combinazioni).

## Profilo di attacco base

| Campo | Valore |
| --- | --- |
| Ability ID | `Hero.Wraith.PulseShot` |
| Famiglia | **Primary Weapon** |
| Danno / portata | 21 · range 4 |
| Payload oltre il danno | nessuno — ed è una risposta, non una lacuna |
| Dipendenza dal base | ★★★★★ — il più alto del roster |

> ⚠️ **«Primary» descrive quanto spesso lo usa, non quanto è forte.** Con 21 danni `PulseShot` non è il più
> forte del roster — `Hero.Gadget.ArcPulse` ne fa 22 — e non è il più lungo, perché Phase arriva a 5: **nessun numero
> lo rende «l'arma primaria»**.
> A renderlo tale è il kit, che crea le occasioni in cui sparare è già la mossa giusta. ADR-0007 ha scelto
> di **non** cambiare i numeri: alzarli contraddirebbe la motivazione scritta nel catalogo («−1 pagato in
> mobilità»), e un payoff condizionale sulla geometria è una feature con dipendenze proprie, non un campo.

### Il test della falsa scelta

| Domanda | Risposta |
| --- | --- |
| Quando è la scelta corretta? | Ogni volta che c'è una linea pulita. È l'azione più frequente di Wraith, e il resto del kit serve a produrre quelle linee |
| Quando è inferiore a un'abilità firma? | Quando conviene prima **costruire** la geometria — `PassingBlade` per attraversare, `Feint` per spostare la lettura dell'avversario — invece di sparare da dove si è |
| Che cosa risparmia? | Tutto: costa zero e non ha ricarica. La domanda per Wraith non è «posso permettermelo» ma «è questo il momento» |
| Che counterplay esiste? | La geometria. Una copertura bassa toglie 10 su 21, e chi gli nega gli angoli lo disinnesca senza toccarlo — è il motivo per cui la sua debolezza dichiarata è `Affinity.Structures` |
| Che cosa impara il giocatore? | Che la bravura è nel posizionamento, non nel pulsante |

### Prove

| Che cosa | Dove |
| --- | --- |
| Il payload è nel dato | `RefactorTactics.Heroes.Hero.Wraith.MatchesCatalog` |
| L'effetto si vede in partita | `Visual.Combat.Defeat` — 21 a turno per quattro turni · `Visual.Map.LowCoverEdge`, dove è **il suo** colpo a passare dal bordo riparato e a scendere da 21 a 11: è la misura della copertura, e regge solo perché il suo danno è più grande della riduzione |

## Abilità

### Pulse Shot

#### Descrizione

Pulse Shot è l'attacco base di Wraith: 21 danni a range 4. Offre pressione a medio raggio senza compromettere il posizionamento per le sue azioni predittive.

| Campo | Valore |
| --- | --- |
| Ability ID | `Hero.Wraith.PulseShot` |
| Categoria | Attacco base |
| Priorità | 50 |
| Costo risorsa | — |
| Cooldown (turni) | 0 |
| Range (hex) | 4 |
| AoE Radius | 0 |
| Danno base | 21 |
| Control Strength | 0 |
| Durata (turni) | 0 |
| Loss/Contact Policy | Fallback.Cancel |
| Interazione terreno | Attacco base medio raggio |
| Gameplay Tags | `Ability.Offense.Kinetic` |
| Implementation Status | IMPLEMENTED |
| Data Status | CANONICAL |

> 21 danni, range 4.

#### Uso tattico e limiti

È l'opzione neutra del kit quando non si vuole investire in una linea d'intercetto o in una Dash.

### Intercept Shot

#### Descrizione

Intercept Shot prepara una punizione su movimento: quando un nemico entra nella cella o zona controllata, infligge 16 danni e interrompe il movimento.

| Campo | Valore |
| --- | --- |
| Ability ID | `Hero.Wraith.InterceptShot` |
| Categoria | **Predictive Action** (non è una reazione) |
| Priorità | 30 |
| Costo risorsa | — |
| Cooldown (turni) | 2 |
| Range (hex) | 1 |
| AoE Radius | 0 |
| Danno base | 16 |
| Control Strength | 1 |
| Durata (turni) | 0 |
| Loss/Contact Policy | `PredictionBoundary.MovementEntry` — la cella è bloccata alla dichiarazione (`PredictiveTargeting.LockCell`) e **non si rivaluta** |
| Interazione terreno | 16 danni e stop movimento quando un nemico entra nella cella controllata |
| Gameplay Tags | ⚠️ era `Ability.Reaction.Overwatch`, che riflette la classificazione superata. In v0.1 **non c'è GAS**, quindi questo campo è documentale e non ha un consumatore: non è stato sostituito con un tag inventato |
| Implementation Status | **IMPLEMENTED** (2026-08-10, E18 CP 18.2) |
| Data Status | CANONICAL |

> Slot **`Main`** nel dato corrente: è un'azione dichiarata in pianificazione come le altre, non una reazione
> tenuta pronta. Lo slot era `None` finché il rinvio a E14 era dichiarato nei dati; E18 l'ha sciolto.

#### Uso tattico e limiti

La meccanica **non** dipende dalle finestre di E14: si risolve a un **boundary deterministico** sull'ingresso in movimento, senza input durante la risoluzione. Il limite vero è un altro, ed è di gioco, non di implementazione — la cella si blocca alla dichiarazione e non si rivaluta, quindi una previsione sbagliata costa il cooldown a vuoto. Le varianti scambiano danno contro ampiezza della zona controllata.

### Passing Blade

#### Descrizione

Passing Blade è un Dash lineare di 3 celle che infligge 20 danni alle unità attraversate. Usa `LinearDash`: attraversa la linea invece di fermarsi al primo bersaglio come una charge.

| Campo | Valore |
| --- | --- |
| Ability ID | `Hero.Wraith.PassingBlade` |
| Categoria | Dash/Line |
| Priorità | 30 |
| Costo risorsa | — |
| Cooldown (turni) | 2 |
| Range (hex) | 3 |
| AoE Radius | 0 |
| Danno base | 20 |
| Control Strength | 0 |
| Durata (turni) | 0 |
| Loss/Contact Policy | Fallback.Stop |
| Interazione terreno | Dash 3 lineare, 20 danni alle unità attraversate |
| Gameplay Tags | `Ability.Mobility.Dash.Kinetic` |
| Implementation Status | IMPLEMENTED |
| Data Status | CANONICAL |

> MovementStyle LinearDash.

#### Uso tattico e limiti

È la conversione più diretta della mobilità di Wraith in pressione offensiva e permette di cambiare lato dello scontro mentre si produce danno.

### Deflection

#### Descrizione

Deflection è una reazione su attacco diretto che riduce di 20 il danno del colpo che l'ha innescata, con cooldown 2.

| Campo | Valore |
| --- | --- |
| Ability ID | `Hero.Wraith.Deflection` |
| Categoria | Reazione/Deflect |
| Priorità | 15 |
| Costo risorsa | — |
| Cooldown (turni) | 2 |
| Range (hex) | 0 |
| AoE Radius | 0 |
| Danno base | 0 |
| Control Strength | 20 |
| Durata (turni) | 0 |
| Loss/Contact Policy | Reaction.Trigger.DirectHit |
| Interazione terreno | Riduce di 20 il colpo diretto che l'ha innescata |
| Gameplay Tags | `Ability.Reaction.Deflect` |
| Implementation Status | IMPLEMENTED |
| Data Status | CANONICAL |

> Riusa Action.Deflect.

#### Uso tattico e limiti

È già implementata riusando `Action.Deflect`. A differenza di uno scudo persistente, modifica quel singolo colpo e non crea una riserva di HP temporanei.

### Feint

#### Descrizione

Feint è un'azione di controllo predittivo: marca una cella per 1 turno e concede un `Reposition`. La sua identità è legata alla previsione di dove il duello si sposterà, non al danno diretto.

| Campo | Valore |
| --- | --- |
| Ability ID | `Hero.Wraith.Feint` |
| Categoria | Control |
| Priorità | 40 |
| Costo risorsa | — |
| Cooldown (turni) | 2 |
| Range (hex) | 3 |
| AoE Radius | 0 |
| Danno base | 0 |
| Control Strength | 1 |
| Durata (turni) | 1 |
| Loss/Contact Policy | Fallback.Cancel |
| Interazione terreno | Marca una cella e concede Reposition; durata marcatura 1 turno |
| Gameplay Tags | `Ability.Control.Prediction` |
| Implementation Status | PARTIAL |
| Data Status | CANONICAL |

> Target cell + reposition non sono ancora completamente rappresentati.

#### Uso tattico e limiti

È ancora `PARTIAL` perché target-cell e reposition non sono completamente rappresentati nello stesso effetto runtime.

## Fast Reactions / Reaction

### Descrizione delle reazioni

- **`Hero.Wraith.Deflection`** — Quando Wraith subisce un attacco diretto, riduce automaticamente di 20 il danno del colpo nella v0.1 corrente. È già descritta anche fra le abilità.

> ➖ **`Hero.Wraith.InterceptShot` non è più in questa sezione, ed è la correzione principale di questa pagina.**
> Era descritta come reazione con finestra `FIRE/HOLD` da 3 s. Non lo è: dal **2026-08-10** (E18 CP 18.2) è
> una **Predictive Action** — si dichiara in pianificazione, si risolve al passaggio dell'avversario e
> **non chiede alcun input durante la risoluzione**. Una finestra e una scelta `FIRE/HOLD` descrivevano un
> meccanismo che l'abilità non ha mai avuto in partita.
> La sua scheda completa è sopra, fra le **abilità**. Vedi [#1063](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1063).

| Reaction_ID | Trigger | Tipo | Finestra_sec_SOURCE | Costo | Priorità | Scelta_A | Scelta_B | Default_Timeout | Tradeoff | Implementation_Status |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `Hero.Wraith.Deflection` | Subisce un attacco diretto | Deflect | — | — | 15 | Deflect automatico (v0.1 attuale) | — | — | -20 danno sul colpo | IMPLEMENTED |

> ⚠️ **Review required:** una o più finestre temporali sono valori sorgente/storici. Il modello corrente di Fast Reaction usa una baseline di 3,0 s; questi valori vanno riallineati prima dell'implementazione.
> `Hero.Wraith.Deflection` — Reazione deterministica attuale; nessuna finestra live.
> *(La riga di `Hero.Wraith.InterceptShot` diceva «rinviata a E14; richiede trigger su movimento e decision
> boundary». Il rinvio è caduto per la ragione **opposta** a quella che l'aveva prodotto: non le serve una
> finestra interattiva, le serve un boundary deterministico — ed è esattamente ciò che E18 le ha dato.)*

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
| `Hero.Wraith.InterceptShot.Precise` | Intercetto preciso | 20 danni | Controlla una sola cella | `Hero.Wraith.InterceptShot.Extended` | Duelist | CANONICAL |
| `Hero.Wraith.InterceptShot.Extended` | Intercetto esteso | Controlla linea di 3 celle | 14 danni | `Hero.Wraith.InterceptShot.Precise` | Control | CANONICAL |

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
| Reactions | DEFINED (2) |
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
