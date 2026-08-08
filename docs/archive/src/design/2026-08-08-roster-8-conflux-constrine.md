> ✅ **RECEPITO il 2026-08-08.** Le quattro fazioni sono già in [`../../../wiki/fazioni/index.md`](../../../wiki/fazioni/index.md):
> **Conflux** (Flux, Riva) e **Constrine** (Bastion, Vektor) per la v0.1; **Sentinel Directorate** (Steel,
> Murdock) e **Resonance** (Aurora, Kwang) per la v0.2. Le schede dei quattro personaggi v0.2 esistono come
> `DATA_SPEC` in [`../../../characters/v0.2/`](../../../characters/v0.2/).
> Portarli a runtime è **E21** in [`../../../roadmap/roadmap-post-v0.1.md`](../../../roadmap/roadmap-post-v0.1.md),
> con il vincolo di [ADR-0006](../../../decisions/adr-0006-ownership-abilita-sinergie.md): nessun bonus di
> fazione, nessun kit di coppia.

# RefactorTactics — Roster 8 Personaggi
## v0.1 / v0.2 — Conflux e Constrine

**Data:** 2026-08-08  
**Stato:** Consolidamento roster  
**Scope:** 8 personaggi totali distribuiti su due versioni e due gruppi/fazioni.

---

# 1. Roster canonico per versione

## v0.1

```text
Flux
Riva
Bastion
Vektor
```

Showcase 2v2 corrente:

```text
Conflux
Flux + Riva

vs

Constrine
Bastion + Vektor
```

## v0.2

```text
Steel
Aurora
Murdock
Kwang
```

Questi quattro estendono il roster totale a 8 personaggi.

---

# 2. Identità delle due fazioni

## Conflux

**Principio:** la realtà è una rete di relazioni; il potere nasce dalla connessione, dall'adattamento e dalla trasformazione controllata.

**Identità:** Conflux legge il campo come un sistema dinamico. Non cerca di imporre una forma definitiva al mondo, ma di creare condizioni in cui energia, materia, informazioni e persone possano fluire e combinarsi.

**Valori:**
- interdipendenza;
- adattabilità;
- feedback;
- creatività sistemica;
- trasformazione.

**Stile tattico:**
- combo ambientali;
- propagazione di effetti;
- mobilità reattiva;
- controllo morbido;
- sfruttamento delle connessioni tra unità e terreno.

---

## Constrine

**Principio:** la realtà deve essere resa leggibile e affidabile tramite limiti, struttura e disciplina.

**Identità:** Constrine tende a imporre ordine al campo di battaglia, restringendo opzioni e rendendo prevedibili linee, spazi e comportamenti.

**Valori:**
- struttura;
- controllo;
- disciplina;
- affidabilità;
- previsione.

**Stile tattico:**
- coperture;
- interdizione;
- controllo geometrico;
- prediction;
- reaction;
- pressione sulle rotte e sulle opzioni del nemico.

---

# 3. v0.1 — Conflux

## Flux

**Versione:** v0.1  
**Gruppo:** Conflux  
**Ruolo:** Tecnico elettro / Controller remoto  
**Signature:** Carica / rete conduttiva

### Identità tattica

Flux accumula o distribuisce carica elettrica su bersagli, oggetti e terreno.

### Signature family

```text
Charge
Network
Electricity
Environment
```

### Kit corrente

- Arc Pulse / Basic Attack
- Linear Discharge
- Conductive Node
- Overload
- Reactive Capacitor

### Player Question

> Dove creo la rete che mi permetterà di trasformare una singola scarica in una minaccia più grande?

### Sinergie

- acqua di Riva;
- metallo;
- dispositivi;
- nodi;
- choke point;
- target Wet.

---

## Riva

**Versione:** v0.1  
**Gruppo:** Conflux  
**Ruolo:** Manipolatrice acqua / Supporto / Controller  
**Signature:** Flow / Wet Territory

### Identità tattica

Riva modifica lo stato del terreno creando acqua e sfruttando flusso, attrito e displacement.

### Signature family

```text
Water
Territory
Flow
Displacement
Environment
```

### Kit corrente

- Pressure Jet
- Circular Tide
- Fluid Trail
- Mist Veil
- Flow Reaction

### Player Question

> Quale parte della mappa devo bagnare o spostare per creare la prossima combo?

### Sinergie

- Flux elettrifica le celle Wet;
- crea traiettorie;
- modifica movimento;
- spinge i nemici verso zone preparate.

---

# 4. v0.1 — Constrine

## Bastion

**Versione:** v0.1  
**Gruppo:** Constrine  
**Ruolo:** Architetto / Guardian / Controller  
**Signature:** Strutture direzionali

### Identità tattica

Bastion crea, ruota o usa strutture e coperture per rendere il campo più leggibile e restringere le opzioni avversarie.

### Signature family

```text
Cover
Structure
Protection
Geometry
Reaction
```

### Kit corrente

- Impact Shot
- Kinetic Panel
- Reconfigure
- Ram
- Interposition / Intercept

### Player Question

> Quale linea devo chiudere o proteggere per costringere il nemico a giocare dove voglio io?

### Sinergie

- canalizza i movimenti verso Vektor;
- crea linee favorevoli;
- protegge durante setup alleati;
- modifica cover e archi.

---

## Vektor

**Versione:** v0.1  
**Gruppo:** Constrine  
**Ruolo:** Duellante predittivo / Interceptor  
**Signature:** Prediction / Interception

### Identità tattica

Vektor non vuole soltanto colpire dove si trova il nemico: vuole colpire dove sarà costretto o incentivato a passare.

### Signature family

```text
Prediction
Interception
Delayed Action
Path Control
Mobility
```

### Kit corrente

- Pulse Shot
- Intercept Shot
- Passing Blade
- Deflection
- Feint

### Player Question

> Quale scelta farà il nemico, e come posso trasformarla in un errore?

### Sinergie

- Bastion restringe le rotte;
- Riva può forzare displacement;
- Flux può rendere alcune rotte troppo costose;
- Vektor presidia l'uscita più probabile.

---

# 5. v0.2 — Steel, Aurora, Murdock, Kwang

La release v0.2 contiene:

```text
Steel
Aurora
Murdock
Kwang
```

Le meccaniche Signature già definite sono:

| Personaggio | Signature |
|---|---|
| Steel | Guard Meter / Effective Protection |
| Aurora | Frozen Domain |
| Murdock | Focus + Fire Sector |
| Kwang | Electric Anchor |

---

# 6. Steel

**Versione:** v0.2  
**Gruppo:** da confermare  
**Ruolo:** Guardian / Vanguard  
**Signature:** Guard Meter basato su `RES_SHIELD`

### Identità tattica

Steel viene premiato quando una protezione modifica davvero un esito.

### Signature family

```text
Resource
Protection
Reaction
Interposition
```

### Player Question

> Chi devo proteggere e quando vale davvero la pena intervenire?

---

# 7. Aurora

**Versione:** v0.2  
**Gruppo:** da confermare  
**Ruolo:** Terrain Controller  
**Signature:** Frozen Domain

### Identità tattica

Aurora trasforma il terreno in una risorsa tattica persistente.

### Signature family

```text
Territory
Ice
Environment
Movement
```

### Player Question

> Quale parte della mappa voglio trasformare adesso, e chi potrà sfruttarla dopo?

---

# 8. Murdock

**Versione:** v0.2  
**Gruppo:** da confermare  
**Ruolo:** Marksman  
**Signature:** Focus + Fire Sector

### Identità tattica

Murdock trae valore dalla disciplina posizionale, dal facing e dal controllo di un settore.

### Signature family

```text
Focus
Facing
LOS
Overwatch
Reaction
```

### Player Question

> Quale settore devo dominare e quanto posso permettermi di restare fermo?

---

# 9. Kwang

**Versione:** v0.2  
**Gruppo:** da confermare  
**Ruolo:** Fighter / Controller  
**Signature:** Electric Anchor

### Identità tattica

Kwang usa un oggetto persistente per creare geometrie e connessioni elettriche.

### Signature family

```text
Persistent Object
Link
Electricity
Geometry
```

### Player Question

> Dove piazzo il mio punto di potere per trasformare la geometria del combattimento?

---

# 10. Master Matrix

| # | Personaggio | Versione | Gruppo | Ruolo | Signature |
|---:|---|---|---|---|---|
| 1 | Flux | v0.1 | Conflux | Electro Controller | Charge / Conductive Network |
| 2 | Riva | v0.1 | Conflux | Water Controller | Flow / Wet Territory |
| 3 | Bastion | v0.1 | Constrine | Guardian / Architect | Directional Structures |
| 4 | Vektor | v0.1 | Constrine | Predictive Duelist | Prediction / Interception |
| 5 | Steel | v0.2 | TBD | Guardian / Vanguard | Guard Meter |
| 6 | Aurora | v0.2 | TBD | Terrain Controller | Frozen Domain |
| 7 | Murdock | v0.2 | TBD | Marksman | Focus / Fire Sector |
| 8 | Kwang | v0.2 | TBD | Fighter / Controller | Electric Anchor |

---

# 11. Mapping v0.2 — proposta, NON ancora canone

La seguente divisione è coerente con la filosofia delle due fazioni, ma NON risulta ancora confermata da una decisione esplicita precedente.

## Possibile Conflux

```text
Aurora
Kwang
```

Motivazione:
- Aurora trasforma e propaga condizioni ambientali;
- Kwang crea connessioni e reti elettriche tramite Anchor.

## Possibile Constrine

```text
Steel
Murdock
```

Motivazione:
- Steel impone protezione, posizione e risposta strutturata;
- Murdock controlla settori, facing e linee di tiro.

Questa sezione deve essere promossa a canone solo dopo decisione esplicita.

---

# 12. Evoluzione delle Signature tra v0.1 e v0.2

La v0.1 valida quattro famiglie:

```text
Flux
→ Network / Charge

Riva
→ Territory / Flow

Bastion
→ Structure / Protection

Vektor
→ Prediction / Interception
```

La v0.2 espande le stesse fondamenta senza creare quattro sistemi completamente separati:

```text
Steel
→ Protection + Reaction + Resource

Aurora
→ Territory + Environment

Murdock
→ Prediction-adjacent + Reaction + Facing

Kwang
→ Network + Persistent Object + Electricity
```

Questo è utile tecnicamente perché la v0.2 può riusare framework già validati nella v0.1.

---

# 13. Framework condivisi

| Framework | v0.1 | v0.2 |
|---|---|---|
| Personal Resource | Flux | Steel |
| Territory / Environment | Riva | Aurora |
| Structure / Persistent Entity | Bastion | Kwang |
| Prediction / Reaction | Vektor | Murdock |
| Electricity | Flux | Kwang |
| Protection | Bastion | Steel |
| Facing / Geometry | Bastion, Vektor | Murdock, Kwang |
| Movement manipulation | Riva, Bastion | Aurora, Kwang |

---

# 14. Scenario suite — v0.1

## Character scenarios

```text
Character.Flux.*
Character.Riva.*
Character.Bastion.*
Character.Vektor.*
```

## Team scenarios

```text
Team.Conflux.FluxRiva.*
Team.Constrine.BastionVektor.*
```

## Showcase

```text
RT_Showcase_Relay_v01
```

Composizione:

```text
Flux + Riva
vs
Bastion + Vektor
```

---

# 15. Scenario suite — v0.2

## Character scenarios

```text
Character.Steel.*
Character.Aurora.*
Character.Murdock.*
Character.Kwang.*
```

## Interaction scenarios

Candidate:

```text
Steel + Murdock
Aurora + Kwang
Steel + Aurora
Murdock + Kwang
```

La nomenclatura di fazione va fissata solo dopo il mapping definitivo Conflux/Constrine.

---

# 16. Test minimo per ogni personaggio

Ogni Signature deve avere almeno:

```text
1. Happy Path
2. Failure / Counterplay
3. Boundary Case
4. Interaction Test
5. Determinism Repeat
```

Esempio:

```text
Character.Flux.Signature.HappyPath
Character.Flux.Signature.Counterplay
Character.Flux.Signature.Boundary
Character.Flux.Signature.Interaction
Character.Flux.Determinism.Repeat
```

---

# 17. Regola di release

La versione del personaggio e la fazione sono campi diversi.

```text
CharacterId
ReleaseVersion
FactionId
SignatureId
```

Non codificare:

```text
v0.1 == Conflux
v0.2 == Constrine
```

perché entrambe le release possono contenere entrambe le fazioni.

---

# 18. Data model suggerito

Concettualmente:

```text
CharacterDefinition
├── CharacterId
├── DefinitionVersion
├── ReleaseVersion
├── FactionId
├── RoleTags
├── SignatureDefinition
├── AbilityIds[]
├── ResourceDefinitions[]
├── ReactionProfile
├── EnvironmentAffinity
└── AssetSlot
```

Tutti gli ID devono essere stabili e indipendenti dal display name.

---

# 19. Stato canonico attuale

## Confermato

```text
ROSTER TOTALE = 8
```

### v0.1

```text
Conflux:
- Flux
- Riva

Constrine:
- Bastion
- Vektor
```

### v0.2

```text
- Steel
- Aurora
- Murdock
- Kwang
```

### Signature v0.2

```text
Steel   -> Guard Meter
Aurora  -> Frozen Domain
Murdock -> Focus + Fire Sector
Kwang   -> Electric Anchor
```

## Da confermare

```text
Faction di Steel
Faction di Aurora
Faction di Murdock
Faction di Kwang
```

---

# 20. Prossimo passo

Dopo il mapping definitivo dei quattro personaggi v0.2:

1. eliminare `TBD` dalla Master Matrix;
2. creare `FactionId` definitivi;
3. costruire le 8 schede complete;
4. completare gli scenari automatici di v0.1;
5. progettare la suite v0.2;
6. verificare che ogni nuova Signature riusi framework generali invece di introdurre logica hard-coded per personaggio.
