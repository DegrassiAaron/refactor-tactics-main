# Prompt — Claude Design: RefactorTactics Skill Visual Grammar

Sei un Senior Game UI/UX Designer, Icon Designer e Information Designer.

Progetta per RefactorTactics, tattico competitivo PC-first a turni simultanei, una grammatica visiva modulare per le skill.

## Obiettivo
Il giocatore deve capire in circa 0,5–1 secondo:
- quando la skill agisce;
- cosa fa;
- chi/cosa può bersagliare;
- quale geometria/area influenza;
- come raggiunge il bersaglio;
- quanto vale l'effetto;
- quali modificatori possiede.

Il tooltip serve per dettagli ed eccezioni, non per capire la funzione generale.

## Grammatica obbligatoria
- COLORE = fase/timing
- GEOMETRIA = area/Shape
- ICONA = effetto/proprietà
- NUMERO = quantità
- POSIZIONE = categoria dell'informazione
- GLIFI SECONDARI = modificatori
- BORDI/CONNETTORI = relazioni

Non usare lo stesso canale per due concetti diversi.

## Fasi
- Dodge
- Blast
- Move
- Reaction

Il colore indica la fase. Ogni fase deve avere anche un marker geometrico per accessibilità.
Esplora come base:
- Dodge = triangolo
- Blast = rombo
- Move = quadrato
- Reaction = fulmine/marker dedicato

Scegli una palette leggibile, differenziata e compatibile con daltonismo.

## Target
- Self
- Unit
- Cell
- Direction
- Path

Per Unit:
- Ally
- Enemy
- Any

Non usare il colore di fase per distinguere Ally/Enemy.

## Shape / Area
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

Connected Region deve rappresentare propagazioni lungo celle collegate, per esempio acqua/elettricità, fuoco o gas.

## Delivery
- Direct
- Projectile
- Beam
- Lob
- Ground

Shape e Delivery sono indipendenti.

## Effects
Crea icone SVG coerenti per:
- Damage
- Shield
- Armor
- Heal
- Push
- Pull
- Displace
- Water/Wet
- Electric/Shock
- Fire
- Status
- Cover modification
- Terrain modification
- Graph/traversal modification
- Utility

Shield e Armor devono essere immediatamente distinguibili.

Armor Piercing:
ignora/riduce Armor positiva solo per il colpo e non modifica la statistica.

Armor Shred:
modifica Armor e può portarla sotto zero.

## Numeri
Un numero non compare mai senza simbolo/contesto.

Esempi:
Damage 5
Shield +4
Armor +2
Armor -2
Heal +3
Range 6
Radius 2
Push 3
Pull 2
Duration 2
Targets 3
Cooldown 4
Hits 2

## Modificatori
Crea glifi piccoli e componibili per:
- Pierce
- Stop on First Target
- Hit All
- Bounce
- Ignore Cover
- Destroy Cover
- Friendly Fire possible
- Armor Piercing
- Armor Shred

Non creare una nuova icona completa per ogni combinazione.

## Overwatch
Overwatch NON è un tipo di attacco e NON è una Shape.

È una Reaction:
Reaction → Trigger → Attack generated.

Deve mostrare sia il trigger sia l'attacco generato.

## Layout
Esplora un contenitore esagonale modulare.

Proposta:
- sopra = Phase
- alto/sinistra = Target
- alto/destra = Shape
- sinistra = Delivery
- centro = Primary Effect + Primary Value
- destra = Range / Size
- basso = Modifier
- basso/lati = Secondary Effects / Duration / Cooldown

Puoi migliorarlo, ma una volta scelta la mappa degli slot deve restare stabile.

## Gerarchia visiva
1. quando succede;
2. effetto principale;
3. valore principale;
4. area/target;
5. range;
6. effetti secondari;
7. modificatori avanzati.

## Skill da usare come stress-test

Barrier:
Blast · Ally Unit · Shield +4 · Duration 1

Emergency Brace:
Dodge · Self · Shield +6 · Duration 1

Rail Shot:
Blast · Enemy/Direction · Line · Beam · Damage 5 · Pierce · Range 7

Scatter Shot:
Blast · Direction · Cone · Projectile · Damage 3 · Range 4

Incendiary Grenade:
Blast · Cell · Circle Radius 2 · Lob · Damage 3 · Fire 2 · Range 5

Kinetic Blast:
Blast · Unit · Line · Damage 2 · Push 3 · Range 4

Armor Breaker:
Blast · Enemy Unit · Projectile · Damage 4 · Armor Shred -2 · Range 5

Chain Lightning:
Blast · Enemy Unit · Chain · Beam · Electric Damage 3 · Max Targets 3 · Range 5

Water Burst:
Blast · Cell · Circle Radius 2 · Wet · Push 1 · Range 4 · no direct damage

Overwatch:
Reaction · Trigger enemy movement/entry · generates attack.
Example generated attack: Cone · Projectile · Damage 3 · Range 5

## Stile
- tactical sci-fi
- PC-first
- pulito
- forte silhouette
- geometrie nette
- leggibile a 32–64 px
- niente emoji
- niente dettagli fini indispensabili
- niente fantasy generico
- pensato per SVG/vector
- stroke e silhouette coerenti

## Accessibilità
La maggior parte dell'informazione deve restare comprensibile in scala di grigi.
- fase = colore + marker
- effetto = icona + valore
- AoE = geometria
- numero = sempre accoppiato a un simbolo

## Deliverable
A. Visual Grammar: Phase, Target, Shape, Delivery, Effects, Values, Modifiers.
B. Icon Sheet: tutte le icone individuali su griglia.
C. Skill Examples: le skill sopra isolate e in HUD.
D. Editable Skill Template: pieno + vuoto annotato.
E. Stress Test: 6–8 skill simultaneamente a dimensione HUD reale.
F. Small-size Test: 32, 48, 64 px.
G. 2–3 varianti iniziali della grammatica, con confronto leggibilità/densità.

## Criterio di successo
Un giocatore che conosce la grammatica deve poter leggere quasi subito:
“Blast, linea, beam, 5 danni, perfora, range 7.”
“Dodge, self, +6 Shield.”
“Reaction che si attiva sul movimento nemico.”

Se deve leggere una frase lunga, semplifica il design.
