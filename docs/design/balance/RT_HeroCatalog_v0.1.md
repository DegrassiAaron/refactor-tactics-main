# RT — Catalogo eroi v0.1

> **Fonte**: `docs/src/RefactorTactics — Catalogo e bilanciamento v0.1.pdf` §§7–9 · `docs/PDR/RT_PDR_12_Catalog_v0.1.pdf`
> **Decisione abilitante**: [`adr-0003-modello-azioni-v01.md`](../adr-0003-modello-azioni-v01.md) · **Checkpoint**: CP 1.2 (issue `#28`)
> **Implementazione**: epic **E6** (`#54`–`#59`).

## Stato dell'implementazione (2026-08-06)

Il gioco ha oggi **due archetipi** in codice (`ERTArchetype::Ranger` / `Guardian`, statistiche e abilità in
`ARTUnit::ConfigureAsArchetype`), non quattro eroi. Questo documento è il riferimento verso cui E6 deve
convergere. La sostituzione dei due archetipi con i quattro eroi è la parte del roster che il canone teneva
volutamente piccola «finché il loop non è chiuso»: ora il loop esagonale è chiuso (M6).

## Struttura di un eroe

**Fisso**: identità · ruolo · attacco base · **quattro abilità fondamentali** · affinità ambientale · debolezza ·
statistiche base.
**Configurabile**: variante arma · gadget · modulo di reazione · **variante di una** abilità (una sola per eroe
nel vertical slice).

---

## 1. Flux — tecnico della conduzione

**Ruolo**: attacco · controllo · combo elettrica · disattivazione dispositivi.

| Statistica | Valore |
|---|---:|
| Salute | 90 |
| Movimento | 5 MP |
| Range visivo | 6 |
| Resistenza Push | 0 |
| Affinità | elettricità |
| Debolezza | acqua (`Affinity.Water`) — decisa in CP 6.2, non nel PDF: stesso identificatore dell'affinità di Riva |

| AbilityId | Abilità | Tipo | Effetto | CD |
|---|---|---|---|---:|
| `Flux.ArcPulse` | Impulso ad arco | attacco base | 22 danni, range 4 | 0 |
| `Flux.LinearDischarge` | Scarica lineare | linea | 24 danni, **+8 su bersaglio `Wet`** | 2 |
| `Flux.ConductiveNode` | Nodo conduttore | cella | rende conduttiva una cella per 2 turni | 2 |
| `Flux.Overload` | Sovraccarico | AoE | 18 danni, `Interrupt` sui dispositivi | 3 |
| `Flux.ReactiveCapacitor` | Capacitore reattivo | reazione | scudo 15 e 10 danni all'attaccante | 3 |

**Variante di `LinearDischarge`**
- *Scarica concentrata*: **+6 danni**, ma **non si propaga**.
- *Scarica ramificata*: **bersaglio aggiuntivo**, ma **−6 danni per bersaglio**.

---

## 2. Riva — manipolatrice dell'acqua

**Ruolo**: supporto · controllo del terreno · setup di combo · riposizionamento.

| Statistica | Valore |
|---|---:|
| Salute | 95 |
| Movimento | 5 MP |
| Range visivo | 5 |
| Resistenza Push | 0 |
| Affinità | acqua |
| Debolezza | elettricità (`Affinity.Electricity`) — decisa in CP 6.3, non nel PDF: simmetrica a Flux |

| AbilityId | Abilità | Tipo | Effetto | CD |
|---|---|---|---|---:|
| `Riva.PressureJet` | Getto pressurizzato | linea | 16 danni, applica `Wet`, `Push 1` | 0 |
| `Riva.CircularTide` | Marea circolare | AoE | cura 18 agli alleati, `Wet` ai nemici | 2 |
| `Riva.FluidTrail` | Scia fluida | dash | `Dash 3` e crea acqua lungo il percorso | 2 |
| `Riva.MistVeil` | Velo di nebbia | AoE | crea fumo raggio 1 | 3 |
| `Riva.FlowReaction` | Flusso reattivo | reazione | `Reposition 1` dopo un attacco | 3 |

**Variante di `CircularTide`**
- *Marea curativa*: cura **24**, ma **non applica `Wet`** ai nemici (niente setup elettrico).
- *Marea d'urto*: cura **10**, ma applica **`Push 1`** ai nemici.

---

## 3. Bastion — architetto del campo

**Ruolo**: difesa · controllo dello spazio · modifica degli archi · protezione degli alleati.

| Statistica | Valore |
|---|---:|
| Salute | 120 |
| Movimento | 4 MP |
| Range visivo | 5 |
| Resistenza Push | 1 |
| Affinità | strutture |
| Debolezza | movimento (`Affinity.Movement`) — decisa in CP 6.4, non nel PDF: simmetrica a Vektor |

| AbilityId | Abilità | Tipo | Effetto | CD |
|---|---|---|---|---:|
| `Bastion.ImpactShot` | Colpo cinetico | attacco base | 24 danni, range 3 | 0 |
| `Bastion.KineticPanel` | Pannello cinetico | arco | crea una copertura da 30 HP | 2 |
| `Bastion.Reconfigure` | Riconfigurazione | arco | sposta o ruota una copertura | 2 |
| `Bastion.Ram` | Ariete | charge | 20 danni e `Push 1` | 2 |
| `Bastion.Interposition` | Interposizione | reazione | intercetta un attacco diretto a un alleato | 3 |

**Variante di `KineticPanel`**
- *Pannello rinforzato*: integrità **45**, ma durata **1 turno**.
- *Pannello adattivo*: integrità **25**, ma **ruotabile gratuitamente una volta**.

---

## 4. Vektor — duellante predittivo

**Ruolo**: assalto · interruzione · punizione del movimento · duello.

| Statistica | Valore |
|---|---:|
| Salute | 100 |
| Movimento | 6 MP |
| Range visivo | 6 |
| Resistenza Push | 0 |
| Affinità | movimento |

| AbilityId | Abilità | Tipo | Effetto | CD |
|---|---|---|---|---:|
| `Vektor.PulseShot` | Tiro a impulsi | attacco base | 21 danni, range 4 | 0 |
| `Vektor.InterceptShot` | Tiro d'intercetto | reazione | 16 danni e **stop del movimento** | 2 |
| `Vektor.PassingBlade` | Lama di passaggio | dash | `Dash 3`, 20 danni attraversando | 2 |
| `Vektor.Deflection` | Deviazione | reazione | riduce il danno di 20 | 2 |
| `Vektor.Feint` | Finta | controllo | marca una cella e ottiene `Reposition` | 2 |

**Variante di `InterceptShot`**
- *Intercetto preciso*: **20 danni**, ma controlla **una sola cella**.
- *Intercetto esteso*: **14 danni**, ma controlla **una linea di 3 celle**.

---

## 5. Confronto rapido

| Eroe | HP | MP | Vista | Push res. | Affinità | Identità in una riga |
|---|---:|---:|---:|---:|---|---|
| Flux | 90 | 5 | 6 | 0 | elettricità | fragile, trasforma l'acqua altrui in danno |
| Riva | 95 | 5 | 5 | 0 | acqua | prepara il terreno agli altri e cura |
| Bastion | 120 | 4 | 5 | 1 | strutture | cambia la forma della mappa, lento |
| Vektor | 100 | 6 | 6 | 0 | movimento | punisce chi si muove, il più mobile |

Nessun eroe domina in ogni parametro: Bastion compra HP e resistenza con **movimento** e vista; Vektor compra
mobilità con l'assenza di difese; Flux ha il danno combo più alto ma la salute più bassa.

**Debolezza dichiarata**: il PDF elenca «debolezza» fra gli elementi fissi di ogni eroe ma **non la esplicita**
per nessuno dei quattro. Va fissata in E6 e scritta qui: senza, l'identità resta metà. **Flux**: fissata in
CP 6.2, acqua (`Affinity.Water`) — vedi §1. **Riva**: fissata in CP 6.3, elettricità (`Affinity.Electricity`),
simmetrica a Flux — vedi §2. **Bastion**: fissata in CP 6.4, movimento (`Affinity.Movement`), simmetrica a
Vektor — vedi §3. Vektor resta aperta fino a CP 6.5.

---

## 6. Loadout iniziali consigliati

| Eroe | Variante d'abilità | Gadget | Modulo di reazione |
|---|---|---|---|
| Flux | Scarica ramificata | `Gadget.Insulator` | `Reaction.ReactiveShield` |
| Riva | Marea curativa | `Gadget.Sprinkler` | `Reaction.HazardEscape` |
| Bastion | Pannello adattivo | `Gadget.PortableCover` | `Reaction.AllyIntercept` |
| Vektor | Intercetto esteso | `Gadget.Sensor` | `Reaction.EmergencyDash` |

---

## 7. Divergenze rispetto al PDF (dichiarate)

| # | PDF | Qui | Motivo |
|---|---|---|---|
| 1 | Tabelle delle abilità con nomi, effetti e cooldown sfalsati nell'estrazione | Ricostruite accoppiando `AbilityId` → effetto per posizione e per coerenza semantica (es. `Riva.MistVeil` → fumo, non «cura alleati») | L'accoppiamento letterale produceva abilità incoerenti col nome e col ruolo |
| 2 | «Debolezza» dichiarata fra gli elementi fissi | **Assente** per tutti e quattro | Non si inventa: va decisa in E6 |
| 3 | 4 eroi | In codice esistono 2 archetipi (Ranger/Guardian) | Lo stato è dichiarato in testa: il documento è il bersaglio di E6, non la descrizione dell'esistente |
| 4 | Cooldown di `Riva.PressureJet` non leggibile nella colonna | Assunto **0** (è l'attacco base per la sua colonna «Tipo: linea» a costo 0) | Coerente con gli altri attacchi base, tutti a CD 0 — assunzione **marcata** |

**Non specificato nel PDF** (da fissare in E6): debolezza di ciascun eroe (Flux e Riva: fissate, CP 6.2/6.3) ·
range di `Flux.Overload` (fissato in CP 6.2: **3**, coerente con `ConductiveNode`) e `Riva.CircularTide`
(fissato in CP 6.3: **4**, come `Flux.Overload`) · durata di `Status.Wet` (fissata in CP 6.3: **1 turno**, come
`Guard`/`Exposed`/`Marked` — finestra di combo stretta) · durata di `Vektor.Feint` (aperto) · se le reazioni
degli eroi occupino lo stesso slot dei moduli di reazione dell'equipaggiamento (probabile, ma il PDF elenca
entrambi senza dirlo).
