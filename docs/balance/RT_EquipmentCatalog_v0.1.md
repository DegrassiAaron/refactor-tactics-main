# RT — Catalogo equipaggiamento v0.1

> **Fonte**: `docs/src/RefactorTactics — Catalogo e bilanciamento v0.1.pdf` §6 · `docs/PDR/RT_PDR_12_Catalog_v0.1.pdf`
> **Decisione abilitante**: [`adr-0003-modello-azioni-v01.md`](../adr-0003-modello-azioni-v01.md) · **Checkpoint**: CP 1.2 (issue `#28`)
> **Implementazione**: epic **E7** (`#60`–`#63`). È l'epic **tagliabile per prima** se il tempo stringe
> (roadmap v0.1): il gioco resta completo senza equipaggiamento, non senza azioni.

## Regola di progetto: scelta orizzontale

Ogni variante dichiara **almeno uno svantaggio**. Non esistono livelli, rarità, upgrade numerici,
equipaggiamenti casuali né progressione durante la partita: l'equipaggiamento cambia **come** si gioca, non
**quanto** si è forti. È il pilastro «nessuna potenza permanente pay-to-win» del canone, e il PDF lo ribadisce
nella lista degli errori da evitare («rendere un equipaggiamento migliore in ogni parametro»).

Ogni eroe seleziona: **1 variante arma** + **1 gadget** + **1 modulo di reazione**.

---

## 1. Varianti arma

| WeaponId | Variante | Vantaggio | Svantaggio |
|---|---|---|---|
| `Weapon.Precision` | Precisione | +1 range | −4 danni |
| `Weapon.Impact` | Impatto | applica `Push 1` | −1 range |
| `Weapon.Overcharge` | Sovraccarico | +6 danni | cooldown +1 |
| `Weapon.Split` | Multiplo | bersaglio aggiuntivo | −6 danni |
| `Weapon.Suppressive` | Soppressione | applica `Slow` | −5 danni |
| `Weapon.Environmental` | Ambientale | migliora gli hazard | −5 danni diretti |

Le varianti modificano l'**attacco base** dell'eroe (valori per tipo d'arma nel
[catalogo azioni](RT_ActionCatalog_v0.1.md) §1).

---

## 2. Gadget

Tutti i gadget hanno **cooldown 3**.

| GadgetId | Gadget | Effetto |
|---|---|---|
| `Gadget.Medkit` | Medkit | cura 18 |
| `Gadget.BreachCharge` | Carica da breccia | 35 danni a una struttura |
| `Gadget.Sprinkler` | Sprinkler | acqua raggio 1 |
| `Gadget.Insulator` | Isolante | immunità a **una** propagazione elettrica |
| `Gadget.SmokeEmitter` | Emettitore di fumo | fumo raggio 1 |
| `Gadget.PortableCover` | Copertura portatile | crea una copertura bassa |
| `Gadget.Sensor` | Sensore | rivela un'area |
| `Gadget.Anchor` | Ancora | impedisce **una** spinta |

---

## 3. Moduli di reazione

| ReactionId | Reazione | Trigger | Effetto |
|---|---|---|---|
| `Reaction.EmergencyDash` | Dash d'emergenza | sei bersagliato | `Reposition 1` |
| `Reaction.ReactiveShield` | Scudo reattivo | subisci danno | scudo 15 |
| `Reaction.CounterShot` | Contrattacco | sei colpito | 14 danni |
| `Reaction.AllyIntercept` | Interposizione | un alleato è bersagliato | cambia bersaglio |
| `Reaction.HazardEscape` | Fuga hazard | la cella diventa pericolosa | `Reposition 1` |
| `Reaction.Cleanse` | Pulizia automatica | ricevi un controllo | rimuove lo stato |
| `Reaction.Anchor` | Ancoraggio | ricevi `Push`/`Pull` | annulla lo spostamento |

Regole comuni a **ogni** modulo:

- si attiva **al massimo una volta per turno**;
- deve essere **visibile agli alleati** durante il planning;
- **non** viene replicato come intento ai nemici (invariante #6, privacy dell'intento);
- **può** essere noto al nemico se fa parte del loadout pubblico pre-partita — è informazione di *loadout*, non
  di *intento*.

---

## 4. Loadout iniziali consigliati

| Eroe | Variante | Gadget | Reazione |
|---|---|---|---|
| **Flux** | Scarica ramificata | Isolante | Scudo reattivo |
| **Riva** | Marea curativa | Sprinkler | Fuga hazard |
| **Bastion** | Pannello adattivo | Copertura portatile | Interposizione |
| **Vektor** | Intercetto esteso | Sensore | Dash d'emergenza |

Le *varianti* citate qui sono varianti di **abilità** dell'eroe (una per eroe), non varianti d'arma: dettaglio nel
[catalogo eroi](RT_HeroCatalog_v0.1.md).

---

## 5. Divergenze rispetto al PDF (dichiarate)

| # | PDF | Qui | Motivo |
|---|---|---|---|
| 1 | Tabella gadget con nomi ed effetti sfalsati di una riga nell'estrazione | Ricostruita accoppiando ogni `GadgetId` al proprio effetto per posizione e per senso (es. `Gadget.Insulator` → immunità elettrica) | L'accoppiamento sfalsato produceva voci prive di senso («Isolante → fumo raggio 1») |
| 2 | Tabella reazioni con la stessa sfalsatura | Idem, verificata contro i trigger | Stesso motivo |
| 3 | Cooldown dei gadget ripetuto per riga (tutti 3) | Dichiarato una volta sopra la tabella | Nessuna informazione persa |
| 4 | Il gadget `Gadget.Sensor` non dichiara raggio né durata | Riportato come «rivela un'area», **non specificato** | Non si inventano numeri assenti dalla fonte |

**Non specificato nel PDF** (da fissare in E7): raggio e durata di `Gadget.Sensor` · durata della copertura creata
da `Gadget.PortableCover` (per `Action.CreateCover` è 2 turni: probabile ma non dichiarato) · se l'immunità di
`Gadget.Insulator` si consumi anche quando la propagazione non avrebbe fatto danno.
