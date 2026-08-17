# RefactorTactics — v0.1 Character Elemental Consolidation

> 🗄️ **ARCHIVIATO il 2026-08-16 — consumato.** Questo è un **sorgente**, non un owner: si legge per la
> provenienza, mai per la regola. Insieme al suo gemello operativo
> [`CLAUDE_Apply_Elemental_Proficiency_Consolidation_2026-08-16.md`](CLAUDE_Apply_Elemental_Proficiency_Consolidation_2026-08-16.md),
> con cui è stato letto e filtrato nello stesso passaggio.
>
> **Cosa è entrato** — la grammatica `Master / Specialist / Access / None`, il criterio che una capability
> conta solo se *genera, applica, propaga, trasforma o consuma* davvero un elemento, la distinzione fra
> `Innate` / `Signature-Profile Equipment` / `Generic Equipment`, e la baseline Gadget `Specialist` · Phase
> `Access` · Riktor e Wraith non elementari. Tutto questo vive ora in
> [**#995**](https://github.com/DegrassiAaron/refactor-tactics-main/issues/995), con il DoD filtrato. L'owner
> `docs/characters/elemental-proficiency.md` **non esiste ancora**: la sua creazione è la prima casella di
> quella issue, e il write-set di `docs/characters/**` va assegnato prima di toccarlo (D-139).
>
> 🔴 **Cosa NON è entrato, e perché — il documento non sa che l'asse esiste già, ed è a runtime.** `URTHeroData`
> porta `Affinity` **e** `Weakness` come `FName`, popolati per tutti e quattro gli eroi in
> `RTHeroCatalogLibrary.cpp` e copiati sull'unità da `ARTUnit::ConfigureFromHeroData`, dentro l'elenco che il
> test `RefactorTactics.Unit.HeroDataCrossesTheBoundary` sorveglia: Gadget `Affinity.Electricity`/debole
> `Affinity.Water`, Phase l'inverso, Riktor `Affinity.Structures`/debole `Affinity.Movement`, Wraith
> l'inverso. Decisi in CP 6.2–6.5. Ne seguono due cose che questo documento non poteva vedere: **Riktor e
> Wraith non sono «senza affinità»** — ce l'hanno, non elementale — e **le debolezze sono simmetriche e
> cablate**, quindi la debolezza di Gadget *è* l'affinità di Phase. Decisione d'autore presa al consumo:
> **due assi distinti**, e nessuno dei due implica l'altro. Il rapporto va scritto nell'owner.
>
> 🔴 **Il vincolo di naming è superato.** Il documento elenca fra le cose «da non rompere» che *«gli ID
> gameplay rimangono `Hero.Flux`, `Hero.Riva`, `Hero.Bastion`, `Hero.Vektor`»*: è la clausola di
> [D-120](../../decisions/RT_PDR_00_Decision_Log.md), che
> [D-130](../../decisions/RT_PDR_00_Decision_Log.md) supera il 2026-08-13 — gli `Hero.<Nome>` si rinominano —
> e che [D-134](../../decisions/RT_PDR_00_Decision_Log.md) completa togliendo il redirect dei token abilità.
> Il target è `Hero.Gadget.LinearDischarge`. Un sorgente è l'ultima fonte della gerarchia: valgono le
> decisioni.
>
> 🔴 **Due prescrizioni non hanno oggetto.** Chiede di *«rimuovere ogni classificazione baseline `Electric
> Access`»* da Riktor e *«`Cold Access`»* da Wraith. Misurato su tutto `docs/`: **zero occorrenze** di
> entrambe. Non c'è nulla da rimuovere — resta valida la parte positiva, cioè dichiarare che i due sono non
> elementari, che oggi nessuna pagina dice.
>
> ✅ **Cosa è stato verificato vero**: i quattro path `docs/characters/v0.1/*.md` esistono; nessun owner della
> proficiency esiste; `paragon.md` è owner del solo mapping visuale; il Decision Log è `integration_only`
> nel batch corrente; la Wiki reale è il clone e non `docs/wiki/` (D-076).


**Data:** 2026-08-16  
**Scope:** Gadget, Phase, Riktor, Wraith  
**Tipo:** design/documentation reconciliation package  
**Stato:** APPROVED DESIGN TARGET, da applicare al repository e al clone Wiki rispettando D-139/D-076

## 1. Fonti correnti verificate

Repository: `DegrassiAaron/refactor-tactics-main`, `main`.

File principali:

- `docs/characters/v0.1/gadget.md` — pagina corrente di **Gadget** (`Hero.Flux`)
- `docs/characters/v0.1/phase.md` — pagina corrente di **Phase** (`Hero.Riva`)
- `docs/characters/v0.1/riktor.md` — pagina corrente di **Riktor** (`Hero.Bastion`)
- `docs/characters/v0.1/wraith.md` — pagina corrente di **Wraith** (`Hero.Vektor`)
- `docs/characters/paragon.md` — owner del mapping identita RT ↔ slot asset Paragon
- `docs/wiki/README.md` — D-076: `docs/wiki/` contiene solo asset; le pagine vivono nel clone `refactor-tactics-main.wiki`
- `docs/roadmap/parallel-batch.yaml` — D-139: un path deve appartenere al `writable` di una track prima di essere modificato

### Vincoli da non rompere

- I nomi visibili v0.1 sono **Gadget, Phase, Riktor, Wraith**.
- Gli ID gameplay rimangono `Hero.Flux`, `Hero.Riva`, `Hero.Bastion`, `Hero.Vektor`.
- Lo slot Paragon resta una base asset/presentation e non decide automaticamente il kit RT.
- Non aggiornare PDF snapshot come fonte primaria.
- Non dichiarare `IMPLEMENTED` un contratto che il runtime non supporta.

## 2. Grammatica di proficiency elementale

### Gradi

| Grado | Contratto |
|---|---|
| **Master** | almeno 3 capability elementali significative dello stesso elemento e almeno 2 funzioni differenti |
| **Specialist** | 2 capability elementali significative dello stesso elemento |
| **Access** | 1 capability elementale significativa dello stesso elemento |
| **None** | nessuna capability elementale nativa |

Una capability conta solo se il profilo/kit **Generate, Apply, Propagate, Transform o Consume** realmente l'elemento, uno stato o una superficie del sistema.

Non bastano da soli:

- `DamageType`;
- Gameplay Tag tematico;
- VFX;
- nome dell'abilità;
- estetica dello slot Paragon.

### Origine dell'accesso

| Origine | Conta nella proficiency? |
|---|---:|
| **Innate** | si |
| **Signature/Profile Equipment** | si |
| **Generic Equipment** | no: e `External Access` |

Il grado puo dipendere dal profilo/loadout. La copertura del roster, invece, va misurata per personaggi distinti.

## 3. Baseline v0.1 approvata

| Personaggio | Elemento baseline | Grado | Sintesi |
|---|---|---|---|
| **Gadget** | Electric | **Specialist** | Conduction specialist: due capability sistemiche di riferimento |
| **Phase** | Water | **Access** | un solo contributo Water tramite Signature/Profile Gadget |
| **Riktor** | — | **Non elementare** | Field Architecture, cover, structures, interposition, kinetic/displacement |
| **Wraith** | — | **Non elementare** | prediction, reaction, movement, interception |

## 4. Gadget — Electric Specialist

### Identita da conservare

La pagina corrente e gia coerente sul nucleo: **Conduction**, fragile controller/striker che sfrutta `Wet` e il grafo conduttivo.

### Capability che contano per il grado

1. `Flux.LinearDischarge`
   - funzione: **Apply/Exploit Electric**;
   - payoff su bersaglio `Wet`;
   - contribuisce alla proficiency.

2. `Flux.ConductiveNode`
   - coincide con `Action.Electrify` secondo D-046/D-064;
   - funzione: **Propagate Electric** sul grafo conduttivo;
   - legge acqua/superfici conductive, non crea conduttivita;
   - contribuisce alla proficiency.

### Capability che NON devono aumentare automaticamente il grado

- `Flux.ArcPulse`: puo restare arma/danno a tema Electric; il solo tag/damage type non crea una terza capability sistemica.
- `Flux.Overload`: puo restare AoE/EMP/device interruption; non conta come terzo produttore salvo futura decisione esplicita.
- `Flux.ReactiveCapacitor`: counter/scudo a tema elettrico; non conta automaticamente.

### Risultato canonico

**Gadget = Electric Specialist, non Master.**

La documentazione deve esplicitare la differenza fra **tema/damage type** e **capability elementale che conta per la proficiency**.

## 5. Phase — Water Access via Signature Gadget

### Divergenza attuale

La pagina corrente descrive Phase come:

- `Water Terrain Manipulator`;
- Signature `Water Shaping`;
- secondaria `Wet Setup`;
- risorsa `Riserva Idrica`;
- piu abilita/trasformazioni acquatiche (`PressureJet`, `CircularTide`, `FluidTrail`, `MistVeil`, ecc.).

Se tutte queste fossero capability Water sistemiche, Phase risulterebbe **Specialist/Master**, in conflitto con la decisione approvata **Water Access**.

### Target approvato

Phase non deve diventare visivamente o meccanicamente una water mage. L'accesso all'acqua arriva da un **Signature/Profile Gadget** personale, leggibile sul personaggio/profilo.

Capability Water di riferimento:

1. `Riva.PressureJet`
   - sorgente: Signature Water Gadget;
   - funzione: **Apply Water/Wet**;
   - mantiene `Wet + Push` come grammatica utile;
   - e l'unica capability che deve contare per il grado Water del profilo baseline.

### Identita principale proposta

Il centro del kit deve rimanere/ritornare su:

- supporto;
- link;
- reposition;
- mobility;
- reaction;
- utility.

Le altre skill possono usare VFX fluidi o tecnologie di flusso senza diventare automaticamente produttori Water nel sistema.

### Regola di migrazione documentale

Non cancellare o riscrivere l'as-built come se il codice fosse gia allineato.

La pagina deve separare chiaramente:

1. **AS-BUILT v0.1** — comportamento reale di catalogo/codice oggi;
2. **APPROVED TARGET PROFILE** — Water Access via Signature Gadget, con `PressureJet` unica capability Water che conta;
3. **IMPLEMENTATION GAP** — eventuali altre skill oggi sistemicamente Water da riallineare in issue di codice/kit separate.

### Risultato canonico

**Phase = Water Access.**

Il Signature Gadget conta come accesso nativo del profilo; un gadget generico equipaggiabile da chiunque non conterebbe.

## 6. Riktor — non elementare

### Identita da conservare

La pagina corrente e gia coerente:

- Guardian / Controller;
- `Field Architecture`;
- `Ally Interposition`;
- structures/cover/topology;
- displacement;
- danno `Kinetic`;
- `KineticPanel`, `Reconfigure`, `Ram`, `Interposition`.

### Decisione

Rimuovere ogni classificazione baseline `Electric Access` introdotta per associazione con il vecchio personaggio Paragon.

Il mapping visuale non giustifica un elemento.

### Risultato canonico

**Riktor = non elementare nella baseline v0.1.**

Un futuro profilo potrebbe ottenere Electric tramite una variante specifica, ma non fa parte dell'identita base e non va anticipato nella matrice canonica.

## 7. Wraith — non elementare

### Identita da conservare

La pagina corrente e gia coerente:

- Predictive Duelist;
- `Predictive Interception`;
- `Movement Punish`;
- movement/reaction/prediction/facing;
- `InterceptShot`, `PassingBlade`, `Deflection`, `Feint`;
- danno `Kinetic`.

### Decisione

Rimuovere ogni classificazione baseline `Cold Access` introdotta solo per completare la matrice degli elementi.

### Risultato canonico

**Wraith = non elementare nella baseline v0.1.**

Un futuro profilo criogenico e possibile, ma non e parte del contratto corrente.

## 8. Owner document consigliato

Prima di creare un nuovo file, cercare un owner equivalente. Se non esiste, creare:

`docs/characters/elemental-proficiency.md`

Contenuto owner:

- definizione Master / Specialist / Access / None;
- cosa conta come capability;
- Innate vs Signature/Profile Equipment vs Generic Equipment;
- distinzione proficiency di profilo vs roster coverage;
- tabella canonica dei personaggi, iniziando dalla v0.1;
- rimandi alle pagine personaggio senza duplicarne kit e numeri.

`docs/characters/paragon.md` deve mantenere il proprio ruolo di owner del mapping visuale e al massimo linkare l'owner elementale: non duplicare la matrice li.

## 9. Modifiche richieste alle quattro pagine docs

### `docs/characters/v0.1/gadget.md`

Aggiungere una sezione `## Proficiency elementale` che dichiari:

- Elemento: Electric;
- Grado: Specialist;
- capability contate: `LinearDischarge`, `ConductiveNode`;
- tag/danno/VFX non fanno salire automaticamente il grado;
- non introdurre `Charged` o altri stati inesistenti.

### `docs/characters/v0.1/phase.md`

Aggiungere una sezione `## Proficiency elementale` che dichiari:

- Elemento: Water;
- Grado target: Access;
- sorgente: Signature/Profile Gadget;
- capability target contata: `PressureJet`;
- blocco esplicito `AS-BUILT vs APPROVED TARGET` finche il kit corrente contiene altre capability Water.

Rivedere `Water Terrain Manipulator`, `Water Shaping`, `Wet Setup` e la descrizione della risorsa per evitare che sembrino il contratto finale se il target e Access.

### `docs/characters/v0.1/riktor.md`

Aggiungere una sezione breve:

- Elemento: None;
- stato: non elementare;
- motivo: Field Architecture e Kinetic/Structures sono il centro del kit;
- nessun Electric baseline.

### `docs/characters/v0.1/wraith.md`

Aggiungere una sezione breve:

- Elemento: None;
- stato: non elementare;
- motivo: Prediction/Reaction/Movement sono il centro del kit;
- nessun Cold baseline.

## 10. Wiki

D-076 e vincolante: le pagine Wiki non vivono in `docs/wiki/`.

Fonte da modificare: clone `refactor-tactics-main.wiki`.

Procedura:

1. aprire/clonare il Wiki repo reale;
2. trovare i nomi/path esatti delle pagine dei quattro personaggi; non indovinarli;
3. applicare lo stesso contratto delle pagine `docs/characters/v0.1/*`;
4. trovare una pagina owner gia esistente per affinita/elementi; se assente, creare una pagina centrale di proficiency;
5. evitare copie divergenti della matrice: una pagina owner, le pagine personaggio linkano;
6. dopo il corpo pagina, aggiornare i blocchi status con:

```bash
python scripts/feature_registry.py deploy --wiki-root <clone> --write
```

Il deploy del Feature Registry aggiorna i blocchi `RT_FEATURE_STATUS`, non sostituisce il lavoro sul corpo della Wiki.

## 11. Decision Log e governance

`docs/decisions/RT_PDR_00_Decision_Log.md` e `integration_only` nel batch corrente.

Quindi:

- non modificarlo da una track personaggi non autorizzata;
- preparare la decisione per il passaggio di integrazione;
- la decisione dovrebbe fissare almeno la grammatica proficiency e la baseline v0.1.

D-139 richiede che prima della prima modifica i path delle pagine personaggio siano assegnati a un `writable` della track che esegue il lavoro.

## 12. DoD

- [ ] write-set assegnato prima di toccare i file
- [ ] owner doc della proficiency individuato o creato
- [ ] Gadget = Electric Specialist
- [ ] Phase = Water Access via Signature/Profile Gadget
- [ ] Riktor = non elementare
- [ ] Wraith = non elementare
- [ ] AS-BUILT di Phase non falsificato
- [ ] eventuale migrazione runtime di Phase tracciata separatamente
- [ ] ID tecnici invariati
- [ ] mapping Paragon non usato come sorgente di elementi
- [ ] Wiki clone aggiornato, non `docs/wiki/`
- [ ] nessuna matrice duplicata in piu owner
- [ ] link/validator documentali verdi
- [ ] eventuale Decision Log applicata solo in integrazione

## 13. Commit suggeriti

Repository principale:

`docs(characters): consolidate v0.1 elemental proficiency contracts`

Wiki clone:

`docs(wiki): align v0.1 characters with elemental proficiency`

Eventuale issue successiva per il runtime di Phase:

`design/ability: align Phase baseline profile to single Water capability`
