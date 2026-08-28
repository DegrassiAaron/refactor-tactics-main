# RefactorTactics — Skill Visual Grammar Handoff v0.1

## Obiettivo
Creare un linguaggio visuale modulare per le skill che permetta di capire in circa 0,5–1 secondo:
- quando la skill agisce;
- cosa fa;
- chi/cosa bersaglia;
- quale area/geometria influenza;
- come raggiunge il bersaglio;
- quanto vale l'effetto;
- quali modificatori possiede.

## Decisioni consolidate

### Canali visuali
- Colore = fase/timing.
- Geometria = Shape/Area.
- Icona = effetto/proprietà.
- Numero = quantità.
- Posizione = categoria dell'informazione.
- Glifi secondari = modificatori.
- Bordi/connettori = relazioni.

### Fasi
- Dodge
- Blast
- Move
- Reaction

Ogni fase deve avere colore + marker geometrico secondario per accessibilità.
Marker proposti, non ancora definitivi:
- Dodge = triangolo
- Blast = rombo
- Move = quadrato
- Reaction = fulmine/marker dedicato

I colori esatti non sono ancora fissati.

### Target
- Self
- Unit
- Cell
- Direction
- Path

Relazione Unit:
- Ally
- Enemy
- Any

### Shape / Area
- Single
- Line
- Ray
- Cone
- Circle
- Ring
- Arc
- Cross
- Rectangle
- Wall
- Chain
- Connected Region

Connected Region serve per propagazioni su celle collegate: acqua/elettricità, fuoco, gas, contaminazione, ecc.

### Delivery
- Direct
- Projectile
- Beam
- Lob
- Ground

Shape e Delivery sono assi separati.

### Effect
- Damage
- Shield
- Armor
- Heal
- Push
- Pull
- Displace
- Water / Wet
- Electric / Shock
- Fire
- Status
- Cover modification
- Terrain modification
- Graph / traversal modification
- Utility

Shield e Armor devono essere visivamente distinti.

Armor Piercing e Armor Shred sono diversi:
- Piercing: ignora/riduce Armor positiva per quel colpo; non modifica Armor.
- Shred: modifica Armor e può portarla sotto zero.

### Numeri
Un numero non appare mai senza contesto.

Esempi:
- Damage 5
- Shield +4
- Armor +2
- Armor -2
- Heal +3
- Range 6
- Radius 2
- Push 3
- Pull 2
- Duration 2
- Targets 3
- Cooldown 4
- Hits 2

### Layout proposto
- sopra = Phase
- alto/sinistra = Target
- alto/destra = Shape
- sinistra = Delivery
- centro = Primary Effect + Primary Value
- destra = Range / Size
- basso = Modifiers
- basso/lati = Secondary Effects / Duration / Cooldown

La disposizione è ancora da validare, ma una volta scelta deve restare stabile.

### Modificatori
- Pierce
- Stop on First Target
- Hit All
- Bounce
- Ignore Cover
- Destroy Cover
- Friendly Fire possible
- Armor Piercing
- Armor Shred

### Overwatch
Overwatch NON è un tipo di attacco e NON è una Shape.

È una Reaction:
Reaction → Trigger → genera un Attack.

L'attacco generato possiede poi Target, Shape, Delivery, Effect, Range e modificatori.

### Accessibilità
Il sistema deve restare interpretabile anche in scala di grigi:
- fase = colore + marker;
- effetto = icona + valore;
- AoE = geometria;
- numero = sempre associato a simbolo.

## Esempi usati

### Barrier
Blast · Ally Unit · Shield +4 · Duration 1

### Emergency Brace
Dodge · Self · Shield +6 · Duration 1

### Rail Shot
Blast · Enemy/Direction · Line · Beam · Damage 5 · Pierce Units · Range 7

### Scatter Shot
Blast · Direction · Cone · Projectile · Damage 3 · Range 4

### Incendiary Grenade
Blast · Cell · Circle Radius 2 · Lob · Damage 3 · Fire 2 · Range 5

### Kinetic Blast
Blast · Unit · Line · Damage 2 · Push 3 · Range 4

### Armor Breaker
Blast · Enemy Unit · Projectile · Damage 4 · Armor Shred -2 · Range 5

### Chain Lightning
Blast · Enemy Unit · Chain · Beam · Electric Damage 3 · Max Targets 3 · Range 5

### Water Burst
Blast · Cell · Circle Radius 2 · Wet · Push 1 · Range 4 · No direct damage

### Overwatch
Reaction · Trigger enemy movement/entry · generates attack.
Example generated attack: Cone · Projectile · Damage 3 · Range 5

## Criterio di successo
Dopo aver imparato la grammatica, il giocatore dovrebbe interpretare in 0,5–1 secondo frasi come:
“Blast, linea, beam, 5 danni, perfora, range 7.”
“Dodge, self, +6 Shield.”
“Reaction che si attiva sul movimento nemico.”
