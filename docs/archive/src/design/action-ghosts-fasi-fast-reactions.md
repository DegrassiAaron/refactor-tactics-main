# RefactorTactics — Action Ghosts, ordine delle fasi e Fast Reactions
> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

## Specifica di design/implementazione da usare come contesto per Claude

### Contesto
RefactorTactics è un tattico competitivo PC-first in Unreal Engine 5 basato su turni simultanei.

Questa nota consolida le decisioni emerse sulla rappresentazione grafica del Planning, sull'ordine delle fasi e sull'integrazione delle Fast Reaction.

---

# 1. Vincolo fondamentale: ordine delle fasi

RefactorTactics deve mantenere come riferimento strutturale le principali fasi ispirate ad Atlas Reactor:

```text
DECISION / PLANNING
        ↓
PREP
        ↓
DASH
        ↓
BLAST
        ↓
MOVE
```

Regola fondamentale:

> Il normale MOVE è sempre l'ultima fase/azione volontaria del turno.

Non progettare sequenze arbitrarie del tipo:

```text
Move → Attack → Move → Jump
```

Il giocatore non costruisce una timeline totalmente libera.

Le azioni vengono invece assegnate alla loro fase appropriata.

Esempio:

```text
PREP:  Hardlight Barricade
DASH:  -
BLAST: Riktor Bolt
MOVE:  Move verso H8
```

Uno spostamento speciale può avvenire prima di Blast solo se appartiene esplicitamente alla fase DASH o a un'altra meccanica speciale.

Esempi:

- Dash;
- Blink;
- Charge;
- Leap classificato come Dash;
- displacement causato da un effetto;
- movimento reattivo.

Questi NON sono la normale Move Phase.

---

# 2. Planning visuale tramite Action Ghosts

Durante il Planning, quando il giocatore prepara le azioni di un personaggio, la mappa deve mostrare delle copie visive semitrasparenti del personaggio.

Nome di lavoro:

```text
Action Ghosts
```

oppure, per l'intero sistema:

```text
Ghost Timeline
```

Un Action Ghost rappresenta il personaggio nella posizione e nella posa significativa relativa a una determinata fase/azione.

Non mostra solo la destinazione.

Deve aiutare il giocatore a capire:

- dove si troverà il personaggio;
- in quale direzione sarà orientato;
- da quale posizione partirà un attacco;
- dove terminerà un Dash;
- quale copertura avrà;
- quale linea di tiro avrà;
- quale AoE produrrà;
- se un alleato attraverserà una traiettoria;
- se il personaggio sarà esposto;
- come apparirà la sequenza complessiva del turno.

---

# 3. Non usare una sequenza libera: visualizzare le fasi

La Ghost Timeline deve essere organizzata per fasi.

Esempio UI:

```text
┌────────┬────────┬────────┬────────┐
│ PREP   │ DASH   │ BLAST  │ MOVE   │
├────────┼────────┼────────┼────────┤
│ Shield │   -    │ Shot   │ H8     │
└────────┴────────┴────────┴────────┘
```

Sulla mappa:

```text
Current Unit
    |
    | PREP ghost
    |
    | DASH ghost (se presente)
    |
    | BLAST ghost / attack pose
    |
    +-----------------> MOVE ghost finale
```

Il Move ghost rappresenta sempre lo stato finale della normale azione di movimento del turno.

---

# 4. Pose chiave, non simulazione completa

Durante il Planning NON è necessario riprodurre l'animazione completa delle azioni.

Preferire:

- pose statiche;
- brevi loop;
- anticipation pose;
- landing pose;
- aiming pose;
- shield pose;
- cast pose;
- dash endpoint pose.

Obiettivo:

> massima leggibilità con minimo rumore visivo.

Le animazioni complete appartengono alla Resolution.

La preview del Planning è una rappresentazione visuale derivata dagli intenti, non una simulazione autorevole.

---

# 5. Scrubbing della Ghost Timeline

Il giocatore deve poter selezionare una fase o un'azione della timeline.

Esempio:

```text
[PREP] [DASH] [BLAST] [MOVE]
                 ^
              selected
```

Quando viene selezionata BLAST:

- il relativo Action Ghost diventa evidente;
- gli altri ghost vengono attenuati;
- vengono enfatizzati:
  - origine dell'attacco;
  - target;
  - linea;
  - AoE;
  - facing;
  - cover rilevante;
  - eventuali warning.

Questo consente di "scrubbare" il turno prima di premere Ready.

---

# 6. Confermato / Previsto / Incerto

Gli Action Ghost devono rispettare la grammatica UI generale di RefactorTactics.

## Confermato

Informazione deterministica sulla base dello stato disponibile.

Visuale proposta:

```text
linea piena
ghost più leggibile
nessun simbolo ?
```

Esempio:

```text
Il personaggio può legalmente raggiungere questa cella.
```

---

## Previsto

Risultato che incorpora gli intenti della propria squadra.

Visuale proposta:

```text
linea tratteggiata
icona team
ghost leggermente attenuato
```

Esempio:

```text
Un alleato dovrebbe liberare quella posizione prima del tuo passaggio.
```

---

## Incerto

Risultato dipendente da:

- azioni nemiche non conosciute;
- collisioni future;
- Reaction;
- target che potrebbe muoversi;
- Fog of War;
- condizioni che verranno rivalidate durante Resolution.

Visuale proposta:

```text
ghost dissolto
gradient
?
```

Non trasformare una previsione client-side in una promessa di risultato.

---

# 7. Intenti alleati

Gli Action Ghost degli alleati possono essere mostrati perché fanno parte della coordinazione team-only.

Un giocatore può quindi vedere:

- path;
- destinazione;
- Action Ghost;
- target;
- AoE;
- direzione;
- breve label;
- Ready.

Questo permette di leggere una strategia di squadra direttamente sulla mappa.

Esempio:

```text
Aegis:
PREP  Barricade
BLAST Riktor Bolt
MOVE  H8

Drift:
PREP  Flood
BLAST Pressure Wave
MOVE  G7
```

I ghost dei due personaggi devono permettere di comprendere visivamente la combo.

---

# 8. Privacy: nessun Action Ghost nemico derivato dal Planning

Requisito critico.

Un client avversario NON deve ricevere gli intenti nemici per poi semplicemente nasconderli graficamente.

Quindi:

```text
Enemy Canonical Intent
        X
        X  NO replication
        X
Enemy Client
```

Gli Action Ghost nemici non devono esistere sul client se richiedono informazioni private del Planning.

Il sistema di ghost deve consumare esclusivamente:

- intenti del giocatore locale;
- intenti sanitizzati della propria squadra;
- stato pubblico;
- informazioni osservate consentite.

Mai:

- path nemici nascosti;
- target nemici pianificati;
- AbilityId privati;
- destinazioni future;
- Reaction future.

---

# 9. Fast Reaction: sì nel sistema di Planning, ma NON come azione lineare

Le Fast Reaction hanno senso nella stessa interfaccia di preparazione del turno, ma non devono diventare una quinta fase lineare.

NON:

```text
PREP → DASH → BLAST → MOVE → REACTION
```

e NON:

```text
PREP → REACTION → DASH → BLAST → MOVE
```

La Reaction è invece una condizione armata.

Rappresentazione:

```text
PREP → DASH → BLAST → MOVE
              |
              +---- ⚡ REACTION ARMED
```

oppure:

```text
┌────────┬────────┬────────┬────────┐
│ PREP   │ DASH   │ BLAST  │ MOVE   │
└────────┴────────┴────────┴────────┘

⚡ Reaction Armed: Overwatch
```

---

# 10. Il Planning configura la Reaction

Durante il Planning il giocatore prepara la Reaction.

Per esempio, Overwatch:

```text
Reaction: Overwatch
Direction: North-East
Shape: Cone
Range: 5
Trigger: EnemyEnterArea
Charge: 1
```

La UI può mostrare:

- cono;
- origine;
- direzione;
- range;
- ghost del personaggio in posa di guardia;
- icona Reaction;
- condizioni note.

Ma NON deve mostrare trigger futuri che dipendono dal piano avversario.

---

# 11. La decisione della Fast Reaction avviene durante la Resolution

La Fast Reaction non deve essere decisa completamente durante il Planning.

Esempio:

```text
Planning:
Overwatch armed
```

Poi durante Resolution:

```text
Enemy enters valid area
        ↓
Reaction Opportunity
        ↓
FAST REACTION — 3 seconds
        ↓
[FIRE] [HOLD]
```

Se il giocatore sceglie:

```text
FIRE
```

la Reaction viene consumata secondo le regole.

Se sceglie:

```text
HOLD
```

perde soltanto quella specifica opportunità e può attendere un eventuale trigger successivo, se la Reaction lo permette.

Importante:

> Il giocatore non deve sapere in anticipo se arriverà un altro trigger.

---

# 12. Perché non programmare FIRE/HOLD nel Planning

Evitare sistemi come:

```text
If Tank enters → HOLD
If Carry enters → FIRE
```

come comportamento standard.

Questo trasformerebbe la Fast Reaction in una macro automatizzata e ridurrebbe:

- tensione;
- commitment;
- bluff;
- bait;
- lettura dell'avversario;
- pressione temporale.

La Reaction deve restare una vera decisione durante la Resolution.

---

# 13. Fast Reaction e Ghost Timeline

La Reaction può comunque essere rappresentata graficamente durante il Planning come ramo condizionale.

Esempio:

```text
                 ┌── ⚡ OVERWATCH ?
                 │
PREP → DASH → BLAST → MOVE
```

Il simbolo `?` comunica che:

- la Reaction è armata;
- l'effetto potrebbe non verificarsi;
- il trigger dipende dalla Resolution;
- il Planning non ne conosce necessariamente l'esito.

Possibile presentazione sulla mappa:

```text
       Ghost in Overwatch pose
               |
           cone / arc
             ??????
               |
        conditional branch
```

---

# 14. Fast Reaction non modifica l'ordine globale delle fasi

Regola importante:

> Una Fast Reaction crea un decision boundary dentro la fase corrente, ma non riscrive la sequenza globale del turno.

Esempio:

```text
BLAST
  |
  | enemy event
  v
FAST REACTION
  |
  | decision resolved
  v
continue BLAST
  |
  v
MOVE
```

Se la Reaction produce uno spostamento:

- Blink;
- dodge;
- knockback;
- intercept;
- reactive dash;

questo è uno spostamento reattivo speciale.

NON è la normale Move Phase.

La normale Move Phase resta l'ultima fase volontaria.

---

# 15. Slow-motion e decision boundary

Durante una Fast Reaction:

```text
Authoritative simulation:
PAUSED at a deterministic decision boundary

Presentation:
may continue in slow-motion

Player:
3 seconds to decide
```

La slow-motion è esclusivamente presentazione.

Non deve modificare:

- ordine logico;
- seed;
- collisioni;
- path;
- risultato;
- durata delle fasi del simulatore.

---

# 16. Action Ghost e Reaction Ghost sono presentation-only

Architetturalmente:

```text
Intent / Team Intent
        |
        v
Planning View Model
        |
        v
Ghost Timeline Renderer
        |
        +--> Character Ghost
        +--> Path Ghost
        +--> AoE Ghost
        +--> Reaction Arc / Cone
        +--> Warning
```

Il renderer non deve decidere regole competitive.

Il server / resolver resta autorità.

---

# 17. Possibile modello dati UI

Non è una specifica definitiva C++, ma una struttura concettuale utile.

```cpp
enum class ERTPlanningPhase
{
    Prep,
    Dash,
    Blast,
    Move
};
```

Una entry di preview può contenere concettualmente:

```text
Phase
UnitId
ActionId
PreviewOrigin
PreviewDestination
Facing
PoseId
TargetCells
AffectedCells
Certainty
```

La Reaction deve essere separata:

```text
ReactionPreview
ReactionId
TriggerType
Origin
Direction
Area
Certainty
Armed
```

Non inserirla forzatamente nella stessa lista lineare delle fasi.

---

# 18. Certainty non significa esito simulato

Importante distinzione.

Un Action Ghost rappresenta:

```text
"Questo è ciò che stai ordinando al personaggio."
```

Non necessariamente:

```text
"Questo è ciò che accadrà sicuramente."
```

Il resolver può produrre:

- blocked;
- interrupted;
- target moved;
- displacement;
- Reaction;
- altered LOS;
- cover destroyed;
- KO;
- path invalidation.

Il playback finale viene guidato dal TurnLog autorevole.

---

# 19. UX desiderata

Il Planning dovrebbe permettere al giocatore di guardare la mappa e leggere visivamente:

```text
Cosa farà il mio personaggio?
Da dove attaccherà?
Dove finirà?
Cosa faranno i miei alleati?
Ci stiamo ostacolando?
Quale zona sto controllando?
Quale Reaction ho preparato?
Quali parti del piano sono certe e quali no?
```

L'obiettivo non è fare una timeline cinematografica.

L'obiettivo è rendere un turno simultaneo complesso comprensibile in pochi secondi.

---

# 20. Regole da considerare consolidate

Considerare queste decisioni come vincoli di design correnti di RefactorTactics:

1. Ordine principale:
   `Planning → Prep → Dash → Blast → Move`.

2. Il normale Move è sempre l'ultima fase/azione volontaria.

3. Dash, Blink e displacement non vanno confusi con Move.

4. Il Planning usa Action Ghost per mostrare pose/posizioni significative.

5. La Ghost Timeline segue le fasi, non una sequenza arbitraria di comandi.

6. Il giocatore può selezionare/scrubbare le singole fasi.

7. Gli Action Ghost rispettano:
   - Confermato;
   - Previsto;
   - Incerto.

8. Gli intenti alleati possono generare Action Ghost team-only.

9. Gli intenti nemici privati non vengono mai inviati al client per produrre ghost nascosti.

10. Le Reaction vengono armate/configurate durante il Planning.

11. Le Fast Reaction vengono decise durante la Resolution quando appare una Reaction Opportunity valida.

12. Reaction e Fast Reaction sono rami condizionali, non una fase lineare aggiuntiva.

13. Le Fast Reaction creano decision boundary dentro la fase corrente senza cambiare l'ordine globale.

14. Gli spostamenti prodotti da una Reaction sono displacement/reaction movement, non la normale Move Phase.

15. Ghost, animazioni e slow-motion sono presentazione e non autorità della simulazione.

---

# 21. Indicazione per implementazione UE5

Quando verrà implementato il sistema:

- mantenere la logica dei ghost separata dal resolver;
- usare un renderer dedicato per path/AoE/ghost;
- evitare Actor persistenti inutili per ogni preview;
- usare pooling per mesh/decal/indicatori;
- aggiornare preview a frequenza limitata, non obbligatoriamente ogni Tick;
- usare pose/AnimSequence/AnimMontage solo come rappresentazione;
- non derivare mai l'esito logico dal tempo dell'animazione;
- tutti i dati team-only devono arrivare al view model già sanitizzati;
- la Reaction preview deve contenere solo dati che il giocatore è autorizzato a conoscere.

---

# 22. Prossimo task consigliato

Progettare un proof of concept della **Ghost Timeline** su una singola unità con quattro slot:

```text
PREP | DASH | BLAST | MOVE
```

Supportare inizialmente:

- ghost posizione corrente;
- ghost endpoint Dash;
- attack pose + linea/AoE;
- ghost posizione Move finale;
- selezione della fase dalla UI;
- tre stili Certainty;
- un Reaction Arc condizionale per Overwatch.

Solo dopo validare:

- ghost alleati;
- collision warning;
- Fast Reaction reale;
- Fog of War;
- multilivello;
- animation polish.
