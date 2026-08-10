# RT — Catalogo eroi v0.1

> **Fonte**: `docs/src/prd/catalogo-e-bilanciamento-v0.1.pdf` §§7–9 · `docs/archive/pdr-v0.1/RT_PDR_12_Catalog_v0.1.pdf`
> **Decisione abilitante**: [`adr-0003-modello-azioni-v01.md`](../decisions/adr-0003-modello-azioni-v01.md) · **Checkpoint**: CP 1.2 (issue `#28`)
> **Implementazione**: epic **E6** (`#54`–`#59`).

## Stato dell'implementazione (2026-08-06)

**Aggiornato al 2026-08-06 (epic E6 completata)**: i quattro eroi esistono come dati
(`URTHeroCatalogLibrary::MakeFlux/MakeRiva/MakeBastion/MakeVektor`) e `ARTGameMode` allestisce il 2v2 con loro
— formazione di default **Flux + Riva** contro **Bastion + Vektor**. I due archetipi (`ERTArchetype`) non
partecipano più allo spawn di partita; restano come helper nei test d'integrazione.

**Aggiornamento 2026-08-08.** Di quel «non ancora costruito» resta solo una voce: **E8** (terreni dinamici,
superfici attive) ed **E9** (coperture bassa e alta, strutture, distruzione) sono **completate**; le abilità
che dipendevano da loro hanno ora un sistema che le consuma. Manca **E7** (gadget). Ogni caso residuo è
dichiarato nella PR del checkpoint e nei commenti del codice, mai nascosto dietro un effetto segnaposto.

### Overwatch, per ogni eroe

`Overwatch` **non è la skill di nessuno**: è un'azione universale che ogni eroe possiede
([D-012](../decisions/RT_PDR_00_Decision_Log.md) · [D-014](../decisions/RT_PDR_00_Decision_Log.md)), e compete
con l'azione offensiva principale. Ciò che **cambia per eroe** è il **profilo**: cosa fa scattare la reazione e
con quale effetto, secondo kit ed equipaggiamento.

I quattro profili **non esistono ancora** — né nel codice né in una fonte corrente — e questo documento **non
li inventa**. Si definiscono in **E14**, insieme alla finestra di decisione.

### Quota elevata

Nessun eroe riceve danno o vista in più per il fatto di stare in alto
([D-018](../decisions/RT_PDR_00_Decision_Log.md) · [D-024](../decisions/RT_PDR_00_Decision_Log.md)): l'altura
vale per geometria. Un eroe **può** dichiarare un bonus legato alla quota, ma allora è **suo**, scritto nel suo
kit — non una regola implicita del terreno.

### Reazioni — aggiornamento del 2026-08-07 (CP 6.7, `#155`)

Le reazioni **non** sono più in quella lista: il motore di E5 le regge (CP 5.5, `#154`) e tre delle cinque
sono cablate e verificate in partita.

| Reazione | Semantica core riusata | Stato |
|---|---|---|
| `Flux.ReactiveCapacitor` | `Action.Counter` | ✅ scudo 15 **e** 10 danni all'attaccante |
| `Bastion.Interposition` | `Action.Intercept` | ✅ incassa il colpo diretto a un alleato entro 2 celle |
| `Vektor.Deflection` | `Action.Deflect` | ✅ −20 sul colpo che l'ha innescata |
| `Vektor.InterceptShot` | — | ⏳ **E14**: trigger d'ingresso su movimento (ADR-0004), non «sono stato colpito» |
| `Riva.FlowReaction` | — | ⏳ **E14**: produce movimento dentro un boundary di risoluzione |

Le due rinviate lo dichiarano **nei dati** (slot `None`, nessun trigger), non solo nei commenti: con lo slot
`Reaction` il pass del turno le raccoglierebbe e registrerebbe un'attivazione che non produce nulla.
Identità, cooldown ed effetti restano dell'eroe; fase, priorità, slot e trigger vengono dall'azione core.

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
| `Flux.ConductiveNode` | Nodo conduttore | cella | **è `Action.Electrify`**: scarica sul grafo conduttivo, range 4, propagazione 3 ([D-064](../decisions/RT_PDR_00_Decision_Log.md)) | 2 |
| `Flux.Overload` | Sovraccarico | AoE | 18 danni, `Interrupt` sui dispositivi | 3 |
| `Flux.ReactiveCapacitor` | Capacitore reattivo | reazione | scudo 15 e 10 danni all'attaccante | 3 |

> **Ownership del bonus `Wet`** ([D-029](../decisions/RT_PDR_00_Decision_Log.md) ·
> [ADR-0006](../decisions/adr-0006-ownership-abilita-sinergie.md)). Il `+8` di `Flux.LinearDischarge` è una
> condizione **dell'abilità di Flux** su uno **stato del sistema**: dipende da `Status.Wet` sul bersaglio, non
> dall'eroe che ha applicato `Wet`. Riva è oggi la sorgente più comune, ma **non** è un requisito: qualsiasi
> sorgente di `Wet` autorizzata dalle regole (`Gadget.Sprinkler`, acqua bassa del terreno, una futura abilità)
> abilita lo stesso payoff. Le etichette storiche **`Water-Electric Combo`** (Signature secondaria di Flux) e
> «combo elettrica» qui sopra nominano quell'**interazione sistemica**, non una coppia Flux + Riva: restano
> invariate perché sono dati canonici e un rename richiede migrazione, non una PR documentale.

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
| `Bastion.ImpactShot` | Colpo cinetico | attacco base | 8 danni, range 3, applica `Slow` | 0 |
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
| Debolezza | strutture (`Affinity.Structures`) — decisa in CP 6.5, non nel PDF: simmetrica a Bastion |

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

### 5.1 Percezione e risorsa firma — consolidato il 2026-08-07

La colonna **Vista** smette di essere inerte: con lo slice di conoscenza parziale
([`brief-conoscenza-parziale.md`](../gameplay/brief-conoscenza-parziale.md)) decide **cosa la squadra può bersagliare**.
Nessuna azione del roster supera la vista di chi la usa (max `range` 5 su vista minima 5), quindi il vincolo
non riduce la gittata di nessuno: la vista lunga vale **anticipo d'informazione**, non danno.

| Eroe | Vista | Ruolo | Risorsa firma | Ricarica su | Cap |
|---|---:|---|---|---|---:|
| Flux | 6 | Controller | Carica Conduttiva | interazione elettrica | 4 |
| Riva | 5 | Support | Riserva Idrica | interazione con acqua | 4 |
| Bastion | 5 | Guardian | Integrità Strutturale | Cleanup | 4 |
| Vektor | 6 | Striker | Slancio | movimento eseguito | 4 |

I restanti parametri di percezione (`Detection`, `Identification`, `Stealth`, `Tracking`, firme) sono
inizializzati **al valore più basso disponibile, uguale per tutti** (Detection 48, Identification 45,
Stealth 2, Tracking 1): si parte piatti e si differenzia col playtest.

> ⚠️ **Corretto il 2026-08-08.** Questo paragrafo diceva: «*nessuno di essi entra nella v0.1 — lo slice è
> binario (in vista / fuori vista)*». **Non è più vero**, e i numeri qui sopra venivano dal workbook, che è
> `RESEARCH` ([D-023](../decisions/RT_PDR_00_Decision_Log.md)).
>
> La percezione della v0.1 **non è binaria**: è **conoscenza parziale** a tre livelli — `Nascosto`,
> `ContattoIncerto`, `Rilevato` — più `UltimoContatto`, unita per squadra (**Team Knowledge**, epic **E13**).
> Il **rumore** è un secondo canale, propagato con interi sul grafo. Il **facing** aggiunge un cono frontale
> più la consapevolezza ravvicinata a 360° entro 2 celle
> ([ADR-0005](../decisions/adr-0005-orientamento.md) §4b, epic **E16**).
>
> Quali di questi parametri diventino statistiche per eroe, e con quali valori, si decide in **E13**: qui non
> si scrive un numero che nessun sistema legge.

Nessun eroe domina in ogni parametro: Bastion compra HP e resistenza con **movimento** e vista; Vektor compra
mobilità con l'assenza di difese; Flux ha il danno combo più alto ma la salute più bassa.

> ⚠️ **Verificato in CP 6.5, e non è del tutto vero sulle statistiche**: sulle sole quattro statistiche base
> **Vektor domina Flux e Riva** — è migliore o pari ovunque, e strettamente migliore in salute *e* movimento
> (100/6/6/0 contro 90/5/6/0 e 95/5/5/0). L'affermazione qui sopra regge solo considerando il pacchetto
> completo (statistiche **+ abilità**): Flux compensa col bonus combo più alto del roster (+8 su `Wet`), Riva
> con la cura ad area. Sono i numeri del PDF, mantenuti invariati; il ribilanciamento è **E11**, tracciato
> nella issue dedicata.

**Debolezza dichiarata**: il PDF elenca «debolezza» fra gli elementi fissi di ogni eroe ma **non la esplicita**
per nessuno dei quattro. Va fissata in E6 e scritta qui: senza, l'identità resta metà. **Flux**: fissata in
CP 6.2, acqua (`Affinity.Water`) — vedi §1. **Riva**: fissata in CP 6.3, elettricità (`Affinity.Electricity`),
simmetrica a Flux — vedi §2. **Bastion**: fissata in CP 6.4, movimento (`Affinity.Movement`), simmetrica a
Vektor — vedi §3. **Vektor**: fissata in CP 6.5, strutture (`Affinity.Structures`) — vedi §4.

Il roster chiude in **due coppie simmetriche**: Flux↔Riva sull'acqua/elettricità, Bastion↔Vektor sullo
spazio/movimento. La debolezza di ogni eroe è l'affinità di un altro — verificato da
`RefactorTactics.Heroes.RosterIsBalanced`.

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
| 2 | «Debolezza» dichiarata fra gli elementi fissi | ~~Assente~~ → **fissata in E6** per tutti e quattro (CP 6.2–6.5), in due coppie simmetriche | Non è stata inventata: decisa esplicitamente eroe per eroe |
| 3 | 4 eroi | ~~In codice esistono 2 archetipi~~ → **risolto in E6**: i quattro eroi sono in codice e in partita | Lo stato aggiornato è dichiarato in testa |
| 4 | Cooldown di `Riva.PressureJet` non leggibile nella colonna | Assunto **0** (è l'attacco base per la sua colonna «Tipo: linea» a costo 0) | Coerente con gli altri attacchi base, tutti a CD 0 — assunzione **marcata** |
| 5 | `Bastion.ImpactShot`: 24 danni | **8 danni + `Slow` 1 turno**, range 3 invariato ([ADR-0007](../decisions/adr-0007-attacco-base-per-eroe.md), 2026-08-09) | A 24 era l'attacco base **più forte del roster**, mentre il ruolo dichiarato di Bastion è Utility/Emergency: la contraddizione stava nei numeri, non nel ruolo. 8 è la metà esatta di `Riva.PressureJet` (16), che sta un gradino sopra. Lo `Slow` è l'unica delle utility candidate insieme esprimibile e coerente — `ERTStructureOp` non danneggia coperture, e uno `Status` si applica al bersaglio, quindi «genera Guard su di sé» non è rappresentabile |

**Non specificato nel PDF** (da fissare in E6): debolezza di ciascun eroe (**tutte fissate**: Flux CP 6.2, Riva
CP 6.3, Bastion CP 6.4, Vektor CP 6.5) ·
range di `Flux.Overload` (fissato in CP 6.2: **3**, coerente con `ConductiveNode`) e `Riva.CircularTide`
(fissato in CP 6.3: **4**, come `Flux.Overload`) · durata di `Status.Wet` (fissata in CP 6.3: **1 turno**, come
`Guard`/`Exposed`/`Marked` — finestra di combo stretta) · durata di `Vektor.Feint` (fissata in CP 6.5: **1 turno**, come `Wet`/`Marked`) · se le reazioni
degli eroi occupino lo stesso slot dei moduli di reazione dell'equipaggiamento (probabile, ma il PDF elenca
entrambi senza dirlo).
