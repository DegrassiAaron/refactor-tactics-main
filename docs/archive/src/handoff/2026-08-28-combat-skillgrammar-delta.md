# CLAUDE HANDOFF — RefactorTactics
## Combat Effect Model + Skill Card Grammar — Consolidation Delta

**Data:** 2026-08-28  
**Stato:** handoff di consolidamento / delta rispetto alla documentazione esistente  
**Scopo:** consolidare le decisioni emerse sul funzionamento di Attack/Damage/Resistance/Vulnerability e sulla nuova organizzazione semantica della Skill Card esagonale, senza alterare accidentalmente il runtime icon catalog o fondere assi di gameplay che devono restare separati nel data model.

---

# 0. Regola operativa

Questo file NON crea un secondo sistema.

Prima di modificare il repository:

1. leggere il Decision Log corrente;
2. leggere il documento owner corrente della Skill Card Grammar;
3. leggere il PRD/Damage Model corrente;
4. leggere `CLAUDE_RT_IconGrammar_Consolidation_Handoff_2026-08-28.md`;
5. verificare le issue/epic già esistenti;
6. modificare gli owner esistenti quando possibile;
7. creare nuove issue/documenti solo se manca realmente un owner.

## Precedenza

In caso di conflitto:

```text
Decision Log / codice canonico
    >
documentazione tecnica owner
    >
PRD consolidati
    >
handoff
    >
docs/research / mockup
```

Non correggere silenziosamente una contraddizione: registrarla e indicare quale fonte prevale.

---

# 1. Scope di questo consolidamento

Il consolidamento copre due domini collegati ma distinti:

```text
A. COMBAT SEMANTICS
   Attack / Hit / Damage / Effect / Resistance / Vulnerability

B. SKILL CARD VISUAL GRAMMAR
   come queste informazioni vengono raggruppate e visualizzate
```

La UI NON deve diventare la sorgente delle regole di gameplay.

Il resolver produce dati canonici.
Il ViewModel produce una vista autorizzata.
La Skill Card li rappresenta.

---

# 2. Combat semantics — regole da consolidare

## 2.1 Skill, Attack, Hit, Damage ed Effect sono concetti distinti

Regola:

```text
Skill
≠ Attack
≠ Hit
≠ Damage
≠ Effect
```

Una Skill può:

- generare un Attack;
- produrre Effect senza Attack;
- produrre più Effect;
- produrre zero Damage;
- modificare mappa/stato senza generare un Hit.

Vietare deduzioni implicite del tipo:

```text
Shape == Single
=> Attack
=> Hit
=> Damage
```

La semantica deve essere dichiarata esplicitamente.

---

## 2.2 Un Attack risolve prima l'interazione, poi gli Effect

Pipeline concettuale:

```text
Skill / Action
    ↓
Validation
    ↓
Targeting / Geometry / Delivery
    ↓
Cover + Facing
    ↓
Active Defense / Reaction
    ↓
Attack Outcome
    ↓
Effect Bundle
    ↓
risoluzione separata degli Effect
```

Possibili Effect:

```text
Damage
Heal
Shield
Status
Control
Displacement
Resource
Environment / Surface
Cover / Structure
Graph / Traversal
Utility
```

---

## 2.3 Hit, ShieldDamage e HealthDamage sono eventi distinti

Un attacco può:

```text
Hit = true
ShieldDamage > 0
HealthDamage = 0
```

Quindi i trigger devono distinguere almeno:

```text
OnHit
OnShieldDamage
OnHealthDamage
OnStatusApplied
OnDisplaced
```

Non usare “ha fatto danno” come sinonimo universale di “ha colpito”.

---

# 3. Damage Model — invarianti da mantenere

Le seguenti regole devono restare allineate al Damage Model canonico.

## 3.1 DamagePacket

Ogni packet contiene un solo `DamageType`.

```text
DamagePacket
- Amount
- DamageSource
- DamageType
- Source
- Target
- flags / semantics
```

Mixed damage:

```text
Kinetic 20
+
Fire 10
```

deve essere rappresentato con più packet logici, non con un DamageType ibrido improvvisato.

---

## 3.2 DamageSource, DamageType e Shape sono assi separati

```text
DamageSource
→ decide l'applicabilità di Armor

DamageType
→ decide quale DamageResistance leggere

Shape
→ decide geometria/targeting
```

Minimo corrente:

```text
DamageSource
- Direct
- Environmental

DamageType
- Kinetic
- Fire
- Electric
```

Shape NON decide Armor.

---

## 3.3 Armor

Armor è signed.

```text
Armor > 0
→ mitigazione degli impatti Direct

Armor = 0
→ neutro

Armor < 0
→ vulnerabilità agli impatti Direct
```

Armor si applica solo a:

```text
DamageSource = Direct
```

Environmental Damage ignora Armor.

---

## 3.4 DamageResistance

Usare il termine esplicito:

```text
DamageResistance
```

e non il generico `Resistance`.

È signed e specifica per DamageType.

```text
DamageResistance > 0
→ resistenza

DamageResistance = 0
→ neutro

DamageResistance < 0
→ weakness / vulnerabilità a quel DamageType
```

Esempio:

```text
FireResistance = -4
```

è già una vulnerabilità numerica al Fire.

Non creare automaticamente un secondo status universale `VulnerableToFire` se esprime la stessa cosa.

---

## 3.5 ControlResistance / StatusResistance è un altro sistema

Non unificare:

```text
DamageResistance
```

con:

```text
ControlResistance
StatusResistance
```

Il primo modifica Damage.

Il secondo governa applicazione/degradazione/rifiuto di status e control.

Risultati tipici:

```text
Full
Degraded
Rejected
```

Damage e status/control devono poter produrre risultati indipendenti.

---

## 3.6 Piercing e Shred sono differenti

```text
Piercing
→ ignora soltanto Armor positiva per quel colpo
→ non modifica Armor del target
```

Se Armor è negativa, Piercing NON la riporta a zero.

```text
Shred
→ modifica Armor
→ può portarla sotto zero
```

Questa distinzione deve esistere sia nel resolver sia nel linguaggio visuale.

---

## 3.7 Shield

Shield è una stat/risorsa, non una Active Defense universale.

Ordine:

```text
Damage mitigato
    ↓
TemporaryShield
    ↓
Shield
    ↓
Health
```

Shield assorbe Damage.

Non cancella automaticamente:

```text
Status
Control
Displacement
Environment Effect
```

salvo regola esplicita della skill/difesa.

---

## 3.8 Zero terminale dopo Active Defense

Se una Active Defense neutralizza il Damage:

```text
Damage = 0 terminale
```

Armor o DamageResistance negative NON possono riattivarlo.

Vietare:

```text
0 - (-Resistance) > 0
```

dopo un outcome che ha già dichiarato il packet neutralizzato.

---

# 4. Formula numerica baseline da consolidare

Questa è la baseline emersa nel focus e deve essere verificata contro il Decision Log prima di essere registrata come normativa.

Se non esiste una decisione numerica più recente, consolidare:

```text
ApplicableArmor =
    Direct
        ? Armor
        : 0

ApplicableArmor con Piercing =
    min(Armor, 0)

Defense =
    ApplicableArmor
    + DamageResistance[DamageType]

FinalDamage =
    max(0, BaseDamage - Defense)
```

Interpretazione:

```text
Armor positiva
→ riduce Direct Damage

Armor negativa
→ aumenta Direct Damage

DamageResistance positiva
→ riduce quel DamageType

DamageResistance negativa
→ aumenta quel DamageType
```

## Vincolo

Lo zero terminale prodotto da Active Defense viene valutato PRIMA di questa formula e non viene riaperto.

## Open point da NON inventare

Mixed Damage / più DamagePacket appartenenti allo stesso Attack:

- DamageResistance è naturalmente per packet;
- non applicare Armor più volte per accidente;
- definire esplicitamente la policy canonica per l'applicazione di Armor a un Hit multi-packet prima dell'implementazione definitiva.

Se il repository ha già una policy più recente, prevale quella.

---

# 5. Vulnerability — grammatica da consolidare

Non creare una singola statistica universale `Vulnerability`.

Usare meccaniche specifiche.

Esempi:

```text
FireResistance < 0
→ vulnerabilità Fire

Armor < 0
→ vulnerabilità agli impatti Direct

Exposed
→ modifica la relazione con Cover

Marked
→ tag/stato consumabile o interrogabile da skill specifiche

Wet
→ stato ambientale/contextual che abilita interazioni

Control weakness
→ dominio ControlResistance / StatusResistance
```

Principio:

> Vulnerability è una famiglia concettuale, non necessariamente un'unica stat.

Evitare pile di moltiplicatori generici difficili da leggere:

```text
Vulnerable +X%
Marked +Y%
Wet +Z%
Exposed +W%
...
```

se lo stesso risultato può essere espresso da una regola sistemica più specifica.

---

# 6. Affinity / Weakness

Affinity/Weakness descrivono identità/predisposizione.

NON devono generare automaticamente DamageResistance.

Esempio:

```text
Affinity.Electric
```

può governare:

- propagazione;
- consumo di stati;
- interazione con celle Conductive;
- payoff di kit;
- eccezioni dichiarate;

senza implicare automaticamente:

```text
ElectricResistance += X
```

Ogni eventuale relazione deve essere dichiarata dal kit/regola.

---

# 7. Elementi — regola di design

Gli elementi NON sono soltanto “colori del danno”.

Possono partecipare a verbi sistemici:

```text
Generate
Apply
Propagate
Transform
Consume
```

Esempi:

```text
Water
→ Create Water
→ Apply Wet
→ Extinguish Fire
→ modificare interazioni

Electric
→ Damage Electric
→ Electrified
→ propagazione su rete Wet / Conductive
→ overload/interazioni strutturali
```

Baseline preferita:

```text
Wet + Electric
→ payoff sistemico / propagation / status
```

non necessariamente:

```text
Wet
→ +X% Electric Damage universale
```

Un DamageTakenModifier percentuale può esistere, ma deve essere esplicito, scoped e raro.

---

# 8. Skill Card Grammar — evoluzione da consolidare

Questa sezione modifica la MAPPA VISIVA della Card Grammar.

IMPORTANTE:

> Raggruppare concetti nello stesso slot UI NON significa fondere i relativi assi nel data model.

Target, Shape e Delivery restano concetti separati nei dati.

Damage, Element e Status restano concetti separati nei dati.

La fusione è solamente un raggruppamento di lettura.

---

# 9. Centro e satelliti

## 9.1 Core

Il centro dell'esagono comunica:

```text
identità / effetto primario della skill
+
valore primario quando utile
```

Esempi:

```text
Damage 30
Shield +20
Heal +15
Wet
Dash
Counter
```

Il centro deve restare la prima cosa leggibile.

---

# 10. Sei gruppi sui vertici dell'esagono

Consolidare come struttura di lavoro della Card Grammar:

```text
1. APPLICATION
2. EFFECT
3. SKILL MODIFIER
4. CONTEXT MODIFIER
5. COUNTER / DEFENSE
6. CONDITION / TRIGGER
```

## 10.1 APPLICATION

Raggruppa visivamente:

```text
Target
Shape / Geometry
Delivery
Range
Hit Rule
```

Domanda:

> Dove, chi e come viene applicata la skill?

Esempi:

```text
Enemy · Line · Beam · Range 7
Cell · Circle R2 · Lob · Range 5
Self · Dash · Path
Direction · Cone · Projectile
```

### Vincolo data model

NON creare un nuovo mega-enum `Application`.

Questi assi restano indipendenti nei dati e nel resolver.

`Application` è un gruppo di presentazione.

---

## 10.2 EFFECT

Raggruppa visivamente:

```text
Output
+
Element / Nature
```

Domanda:

> Cosa produce la skill e di che natura è?

Esempi:

```text
Damage 30 · Kinetic
Damage 20 · Fire
Damage 24 · Electric
Wet · Water
Shield +20
Heal +15
Push 2
Armor Shred -4
Destroy Cover
```

Una skill può contenere:

```text
Primary Effect
+
Secondary Effect
```

ma la Card compatta deve mantenere una gerarchia netta.

### Vincolo data model

NON fondere `Effect`, `DamageType`, `Status`, `Environment` in un singolo enum solo perché condividono lo stesso settore visuale.

---

## 10.3 ♦ SKILL MODIFIER

Forma:

```text
DIAMOND / ROMBO
```

Significato:

> cosa modifica direttamente la definizione/esecuzione della skill stessa.

Esempi:

```text
Piercing
Chain 3
Bounce
Ignore Cover
Stop on Blocker
Hits All
Armor Piercing
special moving-target policy
```

Regola sintetica:

```text
♦ = nasce dalla skill
```

---

## 10.4 ■ CONTEXT MODIFIER

Forma:

```text
SQUARE / QUADRATO
```

Significato:

> cosa sta migliorando, peggiorando o modificando questa esecuzione della skill dall'esterno.

Fonti possibili:

```text
unità
status
talent
cella
high ground
surface
alleato
obiettivo
regola di scenario
```

Esempi:

```text
■ Damage +5
■ Range +1
■ Chain +2
■ Cost -1
■ Ignore Cover
```

Regola sintetica:

```text
■ = arriva dal contesto
```

La UI deve poter indicare la provenienza quando serve alla spiegabilità.

Esempio:

```text
Wet
  ↓
■ Chain +2
```

---

## 10.5 COUNTER / DEFENSE

Gruppo stabile dedicato a:

> cosa può ridurre, impedire o degradare il risultato della skill.

Esempi:

```text
Armor
KineticResistance
FireResistance
ElectricResistance
Cover
Dodge / Active Defense
Shield
ControlResistance
StatusResistance
geometry / blocker
```

Non mostrare ogni possibile passaggio della pipeline.

Mostrare il counter più rilevante per la comprensione rapida della skill.

Esempi:

```text
Direct Kinetic Attack
→ Armor + KineticResistance

Environmental Fire
→ FireResistance
→ no Armor

Root
→ ControlResistance
```

Questo settore è informativo/UI.
NON crea una seconda pipeline di risoluzione.

---

## 10.6 ▲ CONDITION / TRIGGER

Forma:

```text
TRIANGLE / TRIANGOLO
```

Significato:

> requisito, trigger o clausola condizionale.

Esempi:

```text
Target Wet
Target Marked
If Flanked
HP < 50%
On Hit
On Shield Break
On Entry
Start of Resolution
Requires LoS
```

Regola sintetica:

```text
▲ = SE / QUANDO
```

Può essere assente.

Non inventare una condizione per riempire il sesto vertice.

---

# 11. Elementi esterni ai sei gruppi

## Phase / Timing

Resta sopra la card.

```text
PHASE / TIMING
```

Il colore primario della Card Grammar comunica la macro-fase/timing reale.

Non usare il colore primario contemporaneamente come unico indicatore di:

```text
Damage
Element
Ally/Enemy
Defense
```

## Cost / Cooldown / Charges

Resta esterno al ring principale.

Forma:

```text
CIRCLE / CERCHIO
```

Comunica:

```text
Cost
Cooldown
Charges
usage limit
```

Non occupa uno dei sei gruppi semantici principali.

---

# 12. Canali visuali consolidati

```text
POSIZIONE
→ categoria / gruppo dell'informazione

FORMA DEL SATELLITE
→ ruolo grammaticale dell'informazione

GLIFO
→ significato concreto

NUMERO
→ quantità

COLORE PRIMARIO
→ phase / timing

CONNETTORE
→ relazione, causa, dipendenza, trigger
```

Esempio:

```text
▲ Wet
      ↓
■ Chain +2
```

si legge:

```text
SE Wet
ALLORA il contesto modifica la skill con Chain +2
```

---

# 13. Anti icon-soup

La Card compatta non deve tentare di visualizzare l'intero data model.

Baseline di authoring:

```text
Core
→ 1 informazione dominante

Ogni gruppo
→ 0..2 glifi

Ring
→ target indicativo max 8 glifi leggibili
```

Se la skill supera la capacità della Card compatta:

- mostrare le informazioni più rilevanti;
- usare hover/tooltip/pannello esteso per i dettagli;
- non ridurre i glifi fino a renderli illeggibili.

Il cap finale deve essere validato con stress-test percettivi.

---

# 14. Esempi obbligatori da documentare

## 14.1 Rail Shot

```text
Blast
Enemy / Direction
Line
Beam
Damage 30 Kinetic
Range 7
Piercing
```

Mappa visuale:

```text
CORE
Damage 30

APPLICATION
Enemy · Line · Beam · Range 7

EFFECT
Kinetic Damage

♦ SKILL MODIFIER
Piercing

■ CONTEXT
eventuale buff corrente

COUNTER
Armor · KineticResistance · Cover/Defense secondo regole

▲ CONDITION
solo se presente
```

---

## 14.2 Chain Shock

```text
Blast
Enemy
Chain / Beam
Damage 24 Electric
Electrified
Chain 2
Wet/Conductive payoff
```

Mappa visuale:

```text
CORE
Electric Damage 24

APPLICATION
Enemy · Chain · Beam

EFFECT
Electric · Electrified

♦ SKILL MODIFIER
Chain 2

■ CONTEXT
Wet → Chain +2

COUNTER
ElectricResistance

▲ CONDITION
Wet / Conductive solo se la specifica skill richiede realmente la condizione
```

Nota:
Wet non deve essere automaticamente requisito universale di ogni skill Electric.

---

## 14.3 Water Burst

```text
Cell
Circle R2
Wet
Push 1
No Direct Damage
```

Serve come regression test per:

```text
Effect ≠ Damage
Skill ≠ Attack obbligatorio
Element/Nature può esistere senza Damage
```

---

## 14.4 Armor Breaker

```text
Enemy
Projectile
Damage Kinetic
Armor Shred -2
```

Deve rendere visivamente evidente:

```text
Shred
≠
Piercing
```

---

## 14.5 Environmental Fire

Serve a spiegare COUNTER:

```text
Environmental Fire
→ FireResistance
→ Shield
→ Health
→ NO Armor
```

La Card non deve suggerire Armor come counter se la sorgente non la usa.

---

# 15. Migrazione dalla Card Grammar precedente

Il precedente handoff usa:

```text
small hex
→ proprietà intrinseche
→ Target / Element / Shape / Delivery
```

La nuova decisione introduce gruppi semantici più compatti.

Non cancellare il significato precedente alla cieca.

Aggiornamento richiesto:

```text
PRIMA
Target / Shape / Delivery / Element
come satelliti intrinseci separati

DOPO
APPLICATION
→ raggruppa Target / Shape / Delivery / Range / HitRule

EFFECT
→ raggruppa Output + Element/Nature
```

Questo è un cambiamento di COMPOSIZIONE VISIVA.

Non implica modifica automatica a:

```text
ERTIconCategory
Gameplay Tags
DamageType
Targeting enum
Shape enum
Delivery enum
Effect definitions
```

---

# 16. Conflitti/open point da registrare

## Open point A — formula damage

Verificare se il repository possiede già una formula numerica congelata.

Se no, registrare la formula additive/flat proposta in questo handoff come decisione esplicita.

## Open point B — mixed damage e Armor

Definire policy unica per un singolo Hit con più DamagePacket.

Non applicare Armor N volte per accidente.

## Open point C — Counter visual density

Definire se il settore Counter mostra:

```text
un solo counter primario
```

oppure:

```text
max 2 counter
```

Preferenza iniziale: max 2.

## Open point D — Condition come sesto vertice

La condizione può essere assente.

Il vertice resta semanticamente riservato ma vuoto, salvo decisione futura esplicita.

Non usare dinamicamente lo stesso vertice per categorie diverse nella v0.1, perché la posizione deve restare imparabile.

---

# 17. Documentazione owner da aggiornare

Prima fare audit della IA corrente.

Aggiornare preferibilmente:

```text
docs/decisions/RT_PDR_00_Decision_Log.md
```

e il documento owner già esistente della Skill Card Grammar.

Il precedente handoff proponeva, solo se manca un owner:

```text
docs/technical/systems/icon-skill-card-grammar.md
```

Non creare il path senza verificare naming e struttura attuali.

Aggiornare anche il Damage Model owner/PRD solo per le decisioni realmente nuove:

- formula numerica se approvata e non già presente;
- clarification Attack/Hit/Damage/Effect se manca;
- policy multi-packet/Armor quando decisa.

---

# 18. Issue / Epic

NON creare automaticamente una nuova epic.

Per la Card Grammar partire dalle ownership già identificate nel precedente handoff:

```text
E20 · HUD Icon Language
E25 · Icon Language completo
```

Per il Damage Model usare l'epic/issue owner già esistente del Damage Model.

Aggiornare le issue con acceptance criteria concreti, non con copie della documentazione.

## Acceptance criteria UI

- APPLICATION raggruppa Target/Shape/Delivery senza fondere il data model;
- EFFECT raggruppa Output + Element/Nature senza fondere i tipi runtime;
- ♦ e ■ sono semanticamente distinguibili;
- ▲ significa sempre condizione/trigger;
- COUNTER non ricalcola il gameplay;
- Phase e Cost restano fuori dai sei gruppi;
- colore primario = timing/fase;
- test grayscale;
- leggibilità 20–24 px;
- Rail Shot, Chain Shock, Water Burst e Armor Breaker superano lo stress-test.

## Acceptance criteria Combat

- Skill/Attack/Hit/Damage/Effect distinti;
- DamageSource / DamageType / Shape distinti;
- Direct usa Armor;
- Environmental non usa Armor;
- DamageResistance signed;
- Piercing ≠ Shred;
- Shield non cancella automaticamente status/control;
- zero terminale dopo Active Defense;
- TurnLog distingue Hit / ShieldDamage / HealthDamage;
- preview/UI non implementano una seconda formula.

---

# 19. Output richiesto a Claude/PKI

Dopo l'audit, produrre:

1. elenco dei file letti;
2. conflitti trovati;
3. D-ID nuovo o D-ID esistente aggiornato;
4. patch del Decision Log;
5. patch del documento owner Skill Card Grammar;
6. patch del Damage Model owner solo dove necessario;
7. issue/epic aggiornate senza duplicati;
8. elenco degli open point rimasti;
9. test/checklist di verifica;
10. commit Git suggerito.

Commit suggerito:

```text
docs(ui,combat): consolidate skill card groups and damage semantics
```

---

# 20. Regola finale

La Skill Card deve essere una proiezione leggibile del modello, non un secondo modello.

```text
GAMEPLAY DATA
    ↓
authoritative resolver
    ↓
sanitized ViewModel
    ↓
VISUAL GRAMMAR
```

La nuova compressione visuale:

```text
APPLICATION
EFFECT
♦ SKILL MODIFIER
■ CONTEXT MODIFIER
COUNTER
▲ CONDITION
```

serve a ridurre il carico cognitivo.

NON autorizza la fusione dei concetti corrispondenti nel runtime.
