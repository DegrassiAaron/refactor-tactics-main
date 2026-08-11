# RT — Catalogo equipaggiamento v0.1

> **Fonte**: catalogo di bilanciamento v0.1 §6 · PDR-12 — oggi in
> [`prd-personaggi-azioni-e-bilanciamento.md`](../src/prd/prd-personaggi-azioni-e-bilanciamento.md) e
> [`RT_PDR_v0.1_consolidato.md`](../archive/pdr-v0.1/RT_PDR_v0.1_consolidato.md)
> **Decisione abilitante**: [`adr-0003-modello-azioni-v01.md`](../decisions/adr-0003-modello-azioni-v01.md) · **Checkpoint**: CP 1.2 (issue `#28`)
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
| `Gadget.PortableCover` | Copertura portatile | crea una **copertura bassa su un bordo** (modello E9) |
| `Gadget.Sensor` | Sensore | **alza la Team Knowledge** in un'area (modello E13) |

> **Allineamento 2026-08-08 — due gadget parlavano una lingua che non esiste più.**
>
> - `Gadget.PortableCover` crea una copertura **di bordo**, non «di cella»: è
>   `FRTHexCover{Edge, Type: Low, Integrity}` come ogni altra copertura (E9). Ne consegue che è **direzionale**
>   e **distruttibile**, e che la protezione decade fuori dall'arco frontale
>   ([ADR-0005](../decisions/adr-0005-orientamento.md) §4a). Owner del modello:
>   [`RT_TerrainCatalog_v0.1.md`](RT_TerrainCatalog_v0.1.md).
> - `Gadget.Sensor` **non «rivela tutto»**. Nel modello di conoscenza parziale (E13) un sensore **alza il
>   livello** su ciò che copre — tipicamente da `Nascosto`/`ContattoIncerto` a `Rilevato` — e alimenta la
>   **Team Knowledge**, non una visione onnisciente. Raggio e durata restano **non specificati** dalla fonte:
>   si fissano in E7, non qui.
| `Gadget.Anchor` | Ancora | impedisce **una** spinta |

---

## 3. Moduli di reazione

> **I moduli di reazione si dividono in due regimi**, e la divisione **emerge dai dati** — non da un enum di
> policy parallelo ([ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) §2):
>
> | Regime | Condizione | Cosa succede |
> |---|---|---|
> | **Automatico** | `AllowedResponses ≤ 1` | non c'è scelta da fare: risolve deterministicamente, **nessuna finestra**. È il regime di tutte le reazioni di E5 |
> | **Interattivo** | `AllowedResponses > 1` | apre una **Decision Window** di 3,0 s, `Timeout → HOLD`. Epic **E14** |
>
> Un modulo non «è» automatico o interattivo per natura: lo diventa in base a quante risposte legittime i suoi
> dati ammettono. Ecco perché non serve un secondo motore di reazioni — E5 è già il caso semplice di E14.

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

**Non specificato nel PDF** (da fissare in E7): raggio e durata di `Gadget.Sensor` · ~~durata della copertura creata
da `Gadget.PortableCover`~~ · se l'immunità di
`Gadget.Insulator` si consumi anche quando la propagazione non avrebbe fatto danno.

> **Chiuso il 2026-08-09 (E9.5)** — `Gadget.PortableCover` è **costruito**, ed è l'unico gadget della v0.1 a
> esserlo: concede `Action.CreateCover` (integrità 30, durata **2 turni**, la stessa dell'azione core, che era
> «probabile ma non dichiarata»). Slot, loadout e validazione dell'insieme restano `#61`/`#63`.
>
> **Il suo svantaggio non è nel PDF e non è stato inventato.** Il validator ne esige uno dichiarato — un
> equipaggiamento senza svantaggio è una scelta verticale — e allora si dichiara ciò che i cataloghi già
> dicono: **cooldown 3** come ogni gadget (§2) contro il **2** del pannello d'eroe, più l'unico slot gadget
> occupato. Chi non è Bastion può erigere pannelli, ma più di rado e rinunciando a medkit, isolante o sensore.
> Se E7 vorrà uno svantaggio più caratterizzato, questo è il posto dove cambiarlo.
