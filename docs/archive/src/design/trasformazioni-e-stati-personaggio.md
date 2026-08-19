# RefactorTactics — Transformation / Alternate Form Exploration

> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> `RESEARCH` · **Sorgente non normativo** · **Data**: 2026-08-08 · **Recepito da**:
> [`../../../gameplay/brief-stati-personaggio-e-trasformazioni.md`](../../../gameplay/brief-stati-personaggio-e-trasformazioni.md)
> · [D-035](../../../decisions/RT_PDR_00_Decision_Log.md) · epic **E34** in
> [`../../../roadmap/roadmap-post-v0.1.md`](../../../roadmap/roadmap-post-v0.1.md)
>
> **Autorità**: nessuna. `RESEARCH` non risolve conflitti ([`../../../README.md`](../../../README.md), tabella delle
> etichette). In caso di contrasto prevalgono il canone, gli ADR e i brief owner.

> **Status originale:** Design exploration / non vincolante  
> **Scopo:** conservare le alternative discusse per la meccanica di trasformazione e fornire una direzione consigliata senza eliminare opzioni future.

## Correzioni applicate il 2026-08-08

Il documento è stato **archiviato con il testo originale intatto** (convenzione 4 di
`docs/src/`, la cartella sorgenti di allora): le correzioni non riscrivono i paragrafi, sono note `⚠️` inserite accanto
all'affermazione che correggono. Elenco completo, così che non vada cercato nel corpo:

| § | Affermazione originale | Correzione |
|---|---|---|
| 2 | Elenca *unità ausiliarie* e *trappole* fra i sistemi che «competono per l'attenzione» oggi | Entrambi sono **fuori dalla v0.1**: le ausiliarie hanno solo vincoli architetturali, il framework di trap è escluso da [D-016](../../../decisions/RT_PDR_00_Decision_Log.md). Il budget di attenzione della v0.1 è più libero di quanto il documento assuma |
| 5.3 | `Decision / Planning → Prep → Dash → Blast → Move` | La sequenza canonica ha sei voci: `Planning → Prep → Dash → Blast → Move → Cleanup`. L'ordine indicato è corretto, manca il `Cleanup` — che è proprio dove un `RevertRule` a durata fissa andrebbe valutato |
| 7 (Wraith) | `Siege Mode` come «primo candidato per prototipare Alternate Form» | Contrasta con l'identità canonica di Wraith: *Predictive Duelist*, «il più mobile del roster», `Slancio` che **recupera muovendosi**. Una forma che rimuove il Dash non è uno stato, è un secondo personaggio. Vedi la nota in §14 |
| 14 | «Raccomandazione specifica per la **v0.1**»: quattro trasformazioni sui quattro eroi | **Respinta.** Lo scope della v0.1 è chiuso (18 epic, 87 CP) e il rischio di scope è già dichiarato *alto*. Il framework è post-v0.1: [D-035](../../../decisions/RT_PDR_00_Decision_Log.md), epic **E34**. L'ordine di prototipazione resta valido come ordine, non come contenuto della vertical slice |
| 15 | Schema `CharacterState` con `AbilityOverrides[]` | Resta **direzione**, non specifica. Vincoli che il documento non cita: **niente GAS** ([D-005](../../../decisions/RT_PDR_00_Decision_Log.md)) — le classi dati sono `URTActionData`/`URTHeroData`/`URTEquipmentData` — e nessun `if (Hero == X)` nel core ([ADR-0006](../../../decisions/adr-0006-ownership-abilita-sinergie.md)) |
| 17 | `Decision Window: 3s` | Coincide con `FastReactionDuration` = **3,0 s**, di cui l'owner è [ADR-0004](../../../decisions/adr-0004-finestre-di-reazione.md) §8. È una **baseline da playtestare**, non un valore deciso: non va reintrodotto con un secondo nome |

**Una cosa che il documento non vede**, e che è la ragione per cui il brief owner esiste: il repository ha già
un meccanismo per «lo stesso comando si comporta diversamente a seconda dell'eroe» — il **profilo** di azione
generica ([`../../../gameplay/brief-azioni-generiche-overwatch.md`](../../../gameplay/brief-azioni-generiche-overwatch.md) §4).
Uno *Stance* è, dal punto di vista dei dati, un **cambio di profilo dichiarato in Planning**. Le categorie
`Stance` e `Configuration` di §4 non chiedono quindi un sistema nuovo: chiedono di rendere commutabile in
partita qualcosa che oggi è fisso per eroe.

---

## 1. Obiettivo

La **Trasformazione** non dovrebbe essere trattata semplicemente come:

> "premi un pulsante e ottieni un buff"

L'idea più interessante è usarla come un sistema capace di modificare temporaneamente o persistentemente **il modo in cui un personaggio gioca**, mantenendo però sotto controllo:

- carico cognitivo;
- leggibilità della partita;
- complessità del Planning;
- difficoltà di bilanciamento;
- costo di implementazione;
- numero di stati che il giocatore deve prevedere.

La trasformazione deve creare **decisioni strategiche** più interessanti delle informazioni aggiuntive che costringe il giocatore a ricordare.

---

# 2. Vincolo fondamentale: peso della meccanica

RefactorTactics possiede già diversi sistemi che competono per l'attenzione del giocatore:

- Planning simultaneo;
- fasi di risoluzione;
- Action Ghost;
- terreno;
- cover;
- Dash;
- Blast;
- Move;
- Overwatch;
- Fast Reaction;
- trigger;
- interazioni ambientali;
- unità ausiliarie;
- trappole;
- coordinamento tra personaggi.

> ⚠️ **Correzione**: *unità ausiliarie* e *trappole* non competono per l'attenzione nella v0.1. Le ausiliarie
> hanno solo vincoli architetturali e il gameplay è fuori release
> ([`../../../gameplay/brief-unita-ausiliarie.md`](../../../gameplay/brief-unita-ausiliarie.md)); il framework di trap
> è escluso da [D-016](../../../decisions/RT_PDR_00_Decision_Log.md), che ammette **una sola** Predictive Action.
> Il budget di attenzione è quindi meno saturo di quanto il paragrafo assuma — il che rafforza la tesi del
> documento, non la indebolisce: c'è spazio, ma va speso una volta sola.

Una trasformazione completa può quasi raddoppiare la quantità di informazioni relativa a un singolo personaggio.

## 2.1 Livelli indicativi di complessità

| Tipo | Peso gameplay | Peso mentale | Peso implementativo |
|---|---:|---:|---:|
| Buff / stato temporaneo | basso | basso | basso |
| Stance A/B | medio-basso | medio-basso | medio |
| Alternate Form | medio | medio-alto | medio-alto |
| Alternate Form con skill mutate | alto | alto | alto |
| Forma che modifica regole/fasi | molto alto | molto alto | molto alto |
| Trasformazioni concatenate | estremo | estremo | estremo |

---

# 3. Complexity Budget

Si consiglia di trattare la complessità di ogni personaggio come un budget finito.

Valori puramente indicativi:

| Sistema | Costo indicativo |
|---|---:|
| Passive semplice | 10 |
| Skill che modifica terreno | 15 |
| Fast Reaction | 15 |
| Overwatch speciale | 15 |
| Trap system | 20 |
| Summon / pet | 20 |
| Stance | 20–30 |
| Alternate Form | 30–40 |
| Alternate Form che modifica tutto il kit | 50+ |

## Regola consigliata

Un personaggio che possiede una trasformazione importante dovrebbe essere **più semplice altrove**.

Evitare, salvo casi eccezionali:

- trasformazione completa;
- summon;
- trappole;
- più Fast Reaction;
- molte risorse a stack;
- terreno personale;
- doppio kit;
- condizioni complesse;

tutti sullo stesso personaggio.

---

# 4. Framework generale consigliato

Non creare un sistema chiamato esclusivamente `Transformation`.

Meglio creare un framework più generico:

# Character State / Configuration System

che possa supportare più famiglie.

## 4.1 STANCE

Cambio leggero e leggibile.

Esempi:

- Guard ↔ Assault
- Precision ↔ Suppression
- Healbeat ↔ Warbeat

Peso consigliato: **2–4/10**

---

## 4.2 FORM

Cambio sostanziale del comportamento del personaggio.

Esempi:

- Mobile ↔ Siege
- Radiant ↔ Corrupted
- Warrior ↔ Living Rampart

Peso consigliato: **4–7/10**

---

## 4.3 OVERDRIVE

Trasformazione potente ma temporanea.

Esempi:

- Berserker
- Primal
- Cyber-Beast
- Blood Queen

Peso consigliato: **4–7/10**

---

## 4.4 ENVIRONMENTAL STATE

Forma o stato causato/interconnesso con il terreno.

Esempi:

- Charged;
- Frostbound;
- Swamp Avatar;
- Winter Avatar.

Peso consigliato: **3–7/10**

---

## 4.5 CONFIGURATION

Riconfigurazione tecnologica o dell'equipaggiamento.

Esempi:

- GRIM.exe Kernel;
- Siege Platform;
- Guard Configuration;
- Weapon Mode.

Peso consigliato: **3–6/10**

---

# 5. Regole di design consigliate

## 5.1 Stato sempre leggibile

Lo stato deve essere immediatamente comprensibile tramite:

- HUD;
- icona;
- colore/VFX;
- silhouette;
- animazione;
- Action Ghost;
- tooltip;
- preview delle skill modificate.

Esempio:

`VEKTOR — MOBILE`

oppure:

`VEKTOR — SIEGE`

Evitare trasformazioni "invisibili" basate su condizioni difficili da ricordare.

---

## 5.2 Commitment

Una trasformazione interessante dovrebbe spesso richiedere un impegno.

Possibili regole:

- Transform occupa **Prep**;
- Revert occupa **Prep**;
- cooldown prima di poter cambiare nuovamente;
- durata minima;
- costo di risorsa;
- perdita temporanea di una funzione.

L'obiettivo è evitare:

`A → B → A → B`

ogni turno solo perché matematicamente ottimale.

---

## 5.3 Integrazione con le fasi

Ordine principale:

**Decision / Planning → Prep → Dash → Blast → Move**

> ⚠️ **Correzione**: la sequenza canonica ha sei voci — `Planning → Prep → Dash → Blast → Move → **Cleanup**`
> ([`../../../gameplay/spec-sequenza-turno.md`](../../../gameplay/spec-sequenza-turno.md)). L'ordine indicato è
> corretto; manca la fase in cui un `RevertRule` a durata fissa andrebbe valutato, che è esattamente il
> `Cleanup`. È lì che Riktor recupera `Integrità Strutturale`, e sarebbe lì che una forma temporanea scade.

Il normale **Move rimane l'ultima fase volontaria**.

Una trasformazione dovrebbe preferibilmente:

- essere dichiarata nel Planning;
- essere risolta in Prep;
- modificare le preview delle fasi successive;
- aggiornare gli Action Ghost;
- non creare sequenze arbitrarie fuori dal sistema delle fasi.

---

# 6. Tre livelli di trasformazione da usare nel roster

Per ogni personaggio sono mantenute tre possibili direzioni.

### L — Light
Stato o stance.

Peso tipico: **2–3/10**

### M — Medium
Alternate Form che cambia davvero il playstyle.

Peso tipico: **4–6/10**

### S — Signature
Trasformazione identitaria e più invasiva.

Peso tipico: **6–8/10**

Le tre opzioni sono **alternative di design**, non tre trasformazioni da dare contemporaneamente allo stesso personaggio.

---

# 7. Roster v0.1 / v0.2 — idee già discusse

## Gadget

### L — Charged State
Accumula energia.

Possibili effetti:

- Dash migliorato;
- propagazione elettrica;
- sinergia con terreno elettrificato;
- interazioni con acqua.

**Peso:** 3/10

### M — Conductor Mode
Gadget diventa un conduttore.

Possibili effetti:

- attacchi che saltano tra unità;
- conduzione attraverso acqua/oggetti;
- meno danno diretto;
- maggiore controllo.

**Peso:** 5/10

### S — Living Current
Gadget assume temporaneamente una forma energetica.

Possibili effetti:

- movimento speciale;
- attraversamento di determinati ostacoli;
- scia elettrificata;
- interazioni ambientali.

**Peso:** 7/10

**Consiglio:** partire da Charged State. Conservare Living Current come possibile signature futura.

---

## Phase

### L — Flow State

Bonus dopo una sequenza eseguita correttamente.

**Peso:** 2/10

### M — Mist Form

- evasione;
- attraversamento di alcune zone;
- minore potenza offensiva.

**Peso:** 5/10

### S — Tidal Form

Le abilità diventano più orientate a:

- push;
- pull;
- superfici bagnate;
- controllo della posizione.

**Peso:** 7/10

**Consiglio:** Flow State è coerente con un personaggio tecnico senza caricarlo troppo.

---

## Riktor

### L — Fortified

- Move ridotto;
- armor aumentata;
- resistenza al knockback.

**Peso:** 2/10

### M — Bulwark Mode

Riktor diventa:

- cover per gli alleati;
- intercettore;
- elemento di controllo delle linee di tiro.

**Peso:** 5/10

### S — Citadel Form

Riktor diventa quasi parte della mappa:

- quasi immobile;
- genera cover;
- controlla esagoni adiacenti;
- modifica percorsi.

**Peso:** 7/10

**Consiglio:** Bulwark è probabilmente il miglior compromesso.

---

## Wraith

### L — Stabilized Mode

- Move ridotto;
- precisione aumentata;
- Overwatch migliorato.

**Peso:** 3/10

### M — Siege Mode

- Dash assente o fortemente limitato;
- range aumentato;
- Basic Attack modificato;
- Overwatch potenziato;
- possibile mutazione di una skill.

**Peso:** 5/10

### S — Weapons Platform

- fortissimo area denial;
- archi di tiro dedicati;
- Blast pesanti;
- forte commitment;
- Prep necessario per tornare mobile.

**Peso:** 7/10

**Consiglio:** **primo candidato per prototipare Alternate Form.**

> ⚠️ **Correzione**: contrasta con l'identità canonica di Wraith
> ([`../../../characters/v0.1/wraith.md`](../../../characters/v0.1/wraith.md)): *Predictive Duelist*, «il più mobile
> del roster», con `Slancio` che **recupera muovendosi**. Una forma che toglie il Dash non sospende una
> statistica: spegne la risorsa firma e la player question («dove passerà il nemico?»). Non è uno stato dello
> stesso personaggio, è un secondo personaggio. Se serve un banco di prova `Mobile ↔ Siege`, i candidati
> coerenti sono **Howitzer** o **Murdock**, il cui kit è già costruito sul trade-off mobilità/precisione.

---

## Steel

### L — Guard Stance

- più block/intercept;
- minore capacità offensiva.

### M — Assault Stance

- Dash aggressivo;
- knockback;
- meno protezione.

### S — Juggernaut

- attraversa/sfonda alcune cover;
- displacement;
- modifica fisicamente la posizione avversaria.

**Consiglio:** utilizzare Steel come esempio di **stance**, non vera trasformazione.

---

## Aurora

### L — Frostbound

- maggiore generazione di terreno ghiacciato;
- mobilità ridotta.

### M — Crystal Form

- difesa;
- rifrazione/redirect;
- possibili vulnerabilità a fuoco/shatter.

### S — Winter Avatar

Le abilità modificano fortemente l'ambiente:

- ghiaccio;
- acqua congelata;
- vapore;
- muri;
- superfici scivolose.

**Consiglio:** Frostbound nel breve periodo; Winter Avatar da conservare come evoluzione futura.

---

## Murdock

### L — Targeting Mode

- range/precisione aumentati;
- mobilità ridotta.

### M — Hunter Mode

- range ridotto;
- tracking;
- movimento migliore.

### S — Deadeye Protocol

- selezione anticipata di settore/target;
- forte commitment;
- Overwatch/shot speciale se la previsione è corretta.

**Consiglio:** ottimo esempio di Configuration/Stance.

---

## Kwang

### L — Blade Stance

- combattimento diretto;
- mobilità;
- duello.

### M — Storm Stance

La spada viene usata come centro del controllo territoriale.

### S — Stormbound

Kwang e la spada creano una geometria tattica:

- linee elettriche;
- trigger;
- Dash;
- controllo delle traversate;
- terreno attivo.

**Consiglio:** uno dei migliori candidati Signature del roster.

---

# 8. Roster esteso — alternative per personaggio

---

## Steel

- **L:** Guard Stance
- **M:** Assault Stance
- **S:** Juggernaut

Vedi sezione precedente.

---

## Terra

### L — Stoneguard
Resistenza a displacement.

### M — Earthbound
Si ancora e genera terreno/cover rocciosa.

### S — Living Rampart
Terra diventa una vera struttura tattica temporanea.

**Consiglio:** Signature molto interessante perché trasforma un personaggio in elemento di scenario.

---

## Greystone

### L — Battle Resolve
Bonus sotto soglia HP.

### M — Undying Stance
Mobilità sacrificata in cambio di sopravvivenza.

### S — Revenant Form
Dopo una quasi-morte/morte ritorna temporaneamente con kit offensivo modificato.

**Nota:** evitare che sia semplicemente "seconda vita + buff".

---

## Grux

### L — Bloodrush
Stack offensivi dopo colpi riusciti.

### M — Rampage Stance
Più Dash/CC, meno difesa.

### S — Berserker Form
Diventa progressivamente più forte continuando ad attaccare, ma più prevedibile.

**Categoria consigliata:** Overdrive.

---

## Rampage

### L — Adrenaline
Bonus quando danneggiato.

### M — Beast Mode
Mobility/knockback al posto della precisione.

### S — Monstrous Form
Maggiore controllo dello spazio e interazione fisica con ostacoli.

---

## Sevarog

### L — Soulfed
Potenziamento graduale basato sulle anime.

### M — Soul Armor
Converte anime in protezione.

### S — Ascended Form
Dimensione/presenza/controllo aumentano in base alle anime accumulate.

**Consiglio:** ottimo Overdrive progressivo.

---

## Riktor

### L — Lockdown
Stance da controllo.

### M — Execution Mode
Meno tank, hook/follow-up più aggressivi.

### S — Chain Warden
Crea legami tra:

- se stesso;
- nemici;
- elementi della mappa.

**Consiglio:** Signature molto adatta al gameplay su esagoni.

---

## Crunch

### L — Combo State
Una parte del kit cambia in base alla sequenza.

### M — Power Routing
Configurazione Offensive ↔ Defensive.

### S — Overclock
Ogni skill modifica quella successiva.

**Consiglio:** uno dei migliori candidati per il framework Configuration.

---

## Boris

### L — Hunt State
Bonus contro target marcato.

### M — Predator Mode
Più inseguimento, meno difesa.

### S — Cyber-Beast Overdrive
Movimento speciale e attraversamento di alcuni ostacoli.

---

# 9. Ranged / Tech

## TwinBlast

### L — Gun Mode
Precision ↔ Suppression.

### M — Overdrive
Più output offensivo ma accumulo di Heat.

### S — Akimbo Configuration
Le due armi assumono funzioni differenti e combinabili.

**Consiglio:** Configuration > Transformation.

---

## Drongo

### L — Hot Ammo
Munizioni temporanee speciali.

### M — Scrapper Mode
Ranged ↔ traps/area denial.

### S — Mad Bomber
Kit fortemente orientato a ordigni ambientali concatenabili.

---

## Wraith

### L — Recon Mode
Intel e tracking.

### M — Phase Hunter
Stealth/mobility al posto della potenza.

### S — Temporal Shift
Interazione con:

- posizioni previste;
- Action Ghost;
- informazioni del Planning.

**Consiglio:** molto interessante ma potenzialmente pesante.

---

## Lt. Belica

### L — Suppression Mode
Anti-skill.

### M — Enforcer Mode
Control ↔ damage.

### S — Null Field Protocol
Una zona limita o modifica l'uso di determinate abilità.

---

## GRIM.exe

### L — Defense Matrix
Più protezione.

### M — Attack Matrix
Più Blast/range.

### S — Kernel Override
Possibili configurazioni:

- Mobility Kernel;
- Defense Kernel;
- Attack Kernel.

Solo **un kernel attivo alla volta**.

**Consiglio:** probabilmente il miglior candidato dell'intero roster per una vera meccanica di riconfigurazione.

---

## Gadget

### L — Engineering Mode
Gadget più resistenti.

### M — Combat Engineer
Gadget ↔ danno diretto.

### S — Network Mode
I gadget formano una rete tattica interconnessa.

**Consiglio:** molto interessante, ma attenzione a non sovrapporla eccessivamente con il sistema summon/gadget.

---

## Howitzer

### L — Artillery Stance
Range ↑, mobility ↓.

### M — Demolition Mode
Cambio munizioni/pattern Blast.

### S — Siege Platform
Howitzer diventa artiglieria pesante con forte commitment.

**Consiglio:** candidato molto leggibile per Alternate Form.

---

## Zinx

### L — Regeneration State
Sustain.

### M — Chemical Mode
Support ↔ contaminazione.

### S — Bio-Reactor
Converte danno/energia in effetti ambientali.

---

## Muriel

### L — Guardian Mode
Shield migliorati.

### M — Flight Support
Mobility/support.

### S — Seraph Form
Support ad area/globale temporaneo con forte costo.

**Categoria consigliata:** Overdrive.

---

## Dekker

### L — Control Stance
CC migliorato.

### M — Mobility Stance
Reposition/escape.

### S — Arena Architect
Barriere temporanee modificano geometria e percorsi.

**Consiglio:** Signature interessante soprattutto per gli scenari di test della geometria della mappa.

---

# 10. Assassin / Duelist

## Kallari

### L — Cloaked State
Stealth più forte, offense ridotta.

### M — Hunter Mode
Stealth ↔ assassination.

### S — Void Form
Movimento/attraversamento speciale e attacchi basati sulla previsione.

---

## Feng Mao

### L — Offensive Stance
Danno.

### M — Defensive Stance
Shield/reposition.

### S — Momentum Form
Ogni displacement modifica la successiva abilità.

---

## Khaimera

### L — Hunt State
Target lock.

### M — Frenzy
Sustain ↔ aggression.

### S — Primal Form
Fortissimo corpo a corpo, ma minore flessibilità tattica.

**Categoria consigliata:** Overdrive.

---

## Kwang

- **L:** Blade Stance
- **M:** Storm Stance
- **S:** Stormbound

Vedi sezione precedente.

---

## Serath

### L — Radiant State
Forma equilibrata.

### M — Corrupted Form
Più danno, maggiore rischio.

### S — Ascended / Corrupted Duality
Due vere identità tattiche.

**Consiglio:** uno dei candidati più naturali per una vera Alternate Form.

---

## Yin

### L — Flow Stance
Redirect/projectile manipulation.

### M — Wind Stance
Movement/control.

### S — Tempest Form
Trasforma displacement nemico in una risorsa.

---

## Wukong

### L — Agile Stance
Mobility.

### M — Combat Stance
Offense.

### S — Clone Form
Le copie diventano elementi tattici reali, non solo VFX.

**Consiglio:** enorme potenziale per mindgame nel Planning simultaneo, ma costo cognitivo elevato.

---

## Countess

### L — Blood State
Sustain dopo hit.

### M — Shadow Mode
Teleport ↔ offense.

### S — Blood Queen
Le eliminazioni alimentano una forma temporanea.

**Categoria consigliata:** Overdrive.

---

## Shinbi

### L — Performance State
Stack ritmici.

### M — Spirit Pack
Lupi come unità ausiliarie.

### S — Spirit Form
Shinbi entra nel "branco" e cambia movimento/attacchi.

**Nota:** attenzione a non sommare troppo sistema summon + trasformazione.

---

# 11. Mage / Controller / Support

## Aurora

- **L:** Frostbound
- **M:** Crystal Form
- **S:** Winter Avatar

Vedi sezione precedente.

---

## Gideon

### L — Void Charge
Accumulo energia.

### M — Gravity Stance
Damage ↔ displacement.

### S — Event Horizon
Gideon diventa il centro di una zona gravitazionale mobile.

**Consiglio:** ottima Signature per modificare geometria e previsione.

---

## The Fey

### L — Bloom State
Crescita vegetale.

### M — Thorn Form
Support/control ↔ offense.

### S — Wild Growth
La vegetazione conquista progressivamente esagoni.

**Categoria:** Environmental State.

---

## Morigesh

### L — Marked State
Curse migliorata.

### M — Effigy Form
Collegamento vita/effetti con il bersaglio.

### S — Swamp Avatar
Si dissolve nel terreno contaminato e può riemergere altrove.

**Consiglio:** una delle migliori Environmental Form.

---

## Phase

### L — Linked State
Legame potenziato.

### M — Rescue Mode
Pull/support.

### S — Symbiosis
Condivide temporaneamente proprietà con un alleato.

---

## Narbash

### L — Rhythm State
Bonus mantenendo una sequenza.

### M — Warbeat ↔ Healbeat
Due stance chiarissime.

### S — Battle Concert
Il ritmo collettivo modifica progressivamente l'area.

**Consiglio:** Medium molto leggibile; Signature da tenere come alternativa futura.

---

## Iggy & Scorch

### L — Heat State
Accumulo temperatura.

### M — Iggy Mode ↔ Scorch Mode
Control ↔ aggression.

### S — Inferno Engine
La coppia entra in una configurazione ambientale fortemente basata sul fuoco.

**Consiglio:** candidato Signature molto tematico.

---

## Sparrow

### L — Focus State
Precisione crescente.

### M — Volley Stance
Single target ↔ area.

### S — Storm of Arrows
Un settore della mappa diventa zona di pressione.

---

## Revenant

### L — Loaded State
Gestione speciale munizioni.

### M — Duelist Mode
Bersaglio marcato.

### S — Nether Form
Isola temporaneamente sé e il bersaglio in un confronto tattico dedicato.

**Nota:** Signature affascinante ma molto invasiva sulle regole della partita.

---

# 12. Categorie consigliate nel roster

## Vere Alternate Form

Candidati principali:

- Serath — Radiant / Corrupted;
- GRIM.exe — Kernel Override;
- Crunch — Power Routing;
- Iggy & Scorch — Control / Aggression;
- Wukong — Clone Form;
- Terra — Living Rampart;
- Howitzer — Mobile / Siege;
- Wraith — Mobile / Siege.

---

## Stance

Candidati:

- Steel — Guard / Assault;
- Murdock — Mobile / Targeting;
- Feng Mao — Offensive / Defensive;
- TwinBlast — Precision / Suppression;
- Narbash — Healbeat / Warbeat;
- Sparrow — Focus / Volley;
- Phase — Flow State.

---

## Overdrive

Candidati:

- Grux — Berserker;
- Khaimera — Primal;
- Boris — Cyber-Beast;
- Countess — Blood Queen;
- Sevarog — Ascended;
- Rampage — Monstrous;
- Muriel — Seraph.

---

## Environmental State / Map Transformation

Candidati:

- Aurora — Winter Avatar;
- The Fey — Wild Growth;
- Morigesh — Swamp Avatar;
- Gideon — Event Horizon;
- Gadget — Network;
- Riktor — Chain Warden;
- Terra — Living Rampart;
- Gadget — Charged / Living Current;
- Kwang — Stormbound.

---

# 13. Ranking iniziale dei prototipi

## Tier S — prototipare / studiare con priorità

### 1. GRIM.exe — Kernel Override
Perché:

- leggibile;
- modulare;
- facile da rappresentare;
- sfrutta perfettamente il framework Configuration;
- può validare più stati senza doppio kit completo.

### 2. Kwang — Stormbound
Perché:

- coinvolge personaggio, spada e terreno;
- crea geometrie;
- molto identitario;
- perfetto per testare interazioni con la mappa.

### 3. Serath — Radiant / Corrupted
Perché:

- due forme semanticamente chiarissime;
- forte identità;
- ottimo caso per vera Alternate Form.

### 4. Crunch — Power Routing
Perché:

- trasformazione e combo sono naturalmente compatibili;
- configurazione leggibile.

### 5. Aurora — Winter Avatar
Perché:

- porta la trasformazione sulla mappa;
- valorizza il terrain system.

### 6. Terra — Living Rampart
Perché:

- trasforma un character in elemento tattico dello scenario.

---

## Tier A — molto promettenti

- Wukong — Clone Form;
- Gideon — Event Horizon;
- Morigesh — Swamp Avatar;
- Howitzer — Siege Platform;
- Wraith — Siege Mode;
- Riktor — Chain Warden;
- Iggy & Scorch — Inferno Engine.

---

# 14. Raccomandazione specifica per la v0.1

> ⚠️ **Respinta il 2026-08-08 — [D-035](../../../decisions/RT_PDR_00_Decision_Log.md).** Nessuna trasformazione
> entra nella v0.1. Lo scope della release è chiuso (18 epic, 87 checkpoint) e il rischio di scope è già
> registrato come **alto** in [`../../../roadmap/roadmap-v0.1.md`](../../../roadmap/roadmap-v0.1.md) §8; per di più
> due dei quattro stati proposti qui dipendono da sistemi non ancora costruiti — `Charged` dal canale acustico
> e ambientale di **E13**, `Bulwark` dal sistema strutture che regge già solo in parte `KineticPanel`.
>
> **Cosa sopravvive**: l'ordine di prototipazione di §22, come ordine. Il framework vive in
> [`../../../gameplay/brief-stati-personaggio-e-trasformazioni.md`](../../../gameplay/brief-stati-personaggio-e-trasformazioni.md)
> ed è pianificato come epic **E34**, post-v0.1.

Roster:

- Gadget;
- Phase;
- Riktor;
- Wraith.

Non introdurre quattro trasformazioni complete.

## Proposta

### Wraith
**Alternate Form principale**

`Mobile ↔ Siege`

Usarlo come test reale del sistema.

---

### Gadget
**Environmental State leggero**

`Normal → Charged`

Serve a testare trasformazioni causate/interconnesse con il terreno.

---

### Riktor
**Stance / Configuration**

`Normal ↔ Bulwark`

Serve a testare il passaggio da character a pseudo-cover senza doppio kit.

---

### Phase
**State leggero**

`Normal → Flow`

Serve a validare una trasformazione quasi invisibile al ruleset ma evidente nel gameplay.

---

## Perché questa distribuzione

I quattro personaggi possono testare quattro livelli diversi dello stesso framework:

| Character | Tipo |
|---|---|
| Wraith | Alternate Form |
| Gadget | Environmental State |
| Riktor | Stance / Configuration |
| Phase | Lightweight State |

Questo permette di validare l'architettura senza caricare eccessivamente la vertical slice.

---

# 15. Requisiti tecnici suggeriti

> ⚠️ **Direzione, non specifica** — lo dice già il documento in chiusura di sezione, ma mancano tre vincoli del
> repository: **niente GAS** ([D-005](../../../decisions/RT_PDR_00_Decision_Log.md)), quindi le classi dati sono
> `URTActionData`/`URTHeroData`/`URTEquipmentData`; **nessun branch per eroe** nel core
> ([ADR-0006](../../../decisions/adr-0006-ownership-abilita-sinergie.md)); e `AbilityOverrides[]` deve restare
> compatibile con l'ownership del kit — uno stato può sostituire le abilità **del proprio** personaggio, mai
> introdurre l'abilità di un altro.

Il sistema dovrebbe supportare almeno:

```text
CharacterState
    StateId
    DisplayName
    Category
        Stance
        Form
        Overdrive
        Environmental
        Configuration

    Activation
        PlanningAction
        PrepAction
        Trigger
        FastReaction
        Environment
        ResourceThreshold

    Duration
        Persistent
        TurnLimited
        UntilCondition
        UntilRevert

    RevertRule

    StatModifiers[]

    AbilityOverrides[]

    PassiveOverrides[]

    MovementOverrides[]

    VisualState

    GhostPreviewState

    Cooldown

    ResourceCost

    Tags[]
```

Non è una specifica definitiva: serve come direzione architetturale.

---

# 16. Action Ghost e Planning

La trasformazione deve essere visibile già durante Planning.

Esempio:

```text
Wraith
Current: Mobile

PLAN
1. Transform → Siege
2. Blast → Heavy Cannon
3. Move → unavailable

Ghost:
- mostra Wraith nella configurazione Siege;
- aggiorna range;
- aggiorna linea di tiro;
- mostra le azioni non più disponibili;
- mostra la posizione finale prevista.
```

Questo è fondamentale per evitare che la trasformazione aumenti troppo il carico cognitivo.

---

# 17. Fast Reaction e trasformazioni

Possibile estensione futura:

```text
IF incomingDamage >= threshold
    OFFER Reactive Armor
    Decision Window: 3s
```

> ⚠️ **Nota di nomenclatura**: quel `3s` è `FastReactionDuration` = **3,0 s**, il cui owner è
> [ADR-0004](../../../decisions/adr-0004-finestre-di-reazione.md) §8, ed è una **baseline da playtestare**, non un
> valore deciso. Non va reintrodotto con un secondo nome: è già successo una volta con `FastDecisionDuration`
> ([`../../../gameplay/brief-delayed-actions.md`](../../../gameplay/brief-delayed-actions.md) §6.4).

La trasformazione può quindi essere anche una Decision Boundary.

Da usare con moderazione.

Categorie adatte:

- Reactive Armor;
- Panic Form;
- Emergency Shield;
- Counter Configuration.

Non dovrebbe essere la modalità principale della maggior parte delle trasformazioni.

---

# 18. Trasformazione e terreno

Possibile pipeline:

```text
Environment
    ↓
Character State
    ↓
Ability Mutation
    ↓
Environment Mutation
```

Esempio:

```text
Gadget enters Electrified Tile
    ↓
Charged State
    ↓
Electric Skill gains Conduction
    ↓
Hits Wet Tile
    ↓
Electricity propagates
```

Questa famiglia può diventare particolarmente importante per l'identità di RefactorTactics.

---

# 19. Anti-pattern da evitare

## Trasformazione = bonus statistico

Esempio da evitare:

`Transform → +20% damage`

Troppo poco interessante rispetto al costo di sistema.

---

## Doppio personaggio completo

Evitare, salvo casi rarissimi:

```text
4 skill normal
+
4 skill transformed
+
2 passive
+
2 reactions
```

Carico cognitivo troppo elevato.

---

## Toggle gratuito ogni turno

Se non esiste commitment, il sistema diventa micro-ottimizzazione.

---

## Stato poco visibile

Mai obbligare il giocatore a ricordare da solo in quale configurazione si trova un'unità.

---

## Trasformazioni per tutti

Il framework può essere comune.

La meccanica percepita non deve esserlo.

Non ogni personaggio deve sembrare un "trasformista".

---

# 20. Direzione consigliata

Implementare un'infrastruttura comune:

# Character State / Configuration System

e poi presentarla in modi diversi:

- Steel **cambia stance**;
- GRIM.exe **riconfigura il kernel**;
- Serath **si trasforma**;
- Aurora entra in **Winter Avatar**;
- Khaimera entra in **Frenzy**;
- Howitzer entra in **Siege**;
- Gadget diventa **Charged**;
- Kwang diventa **Stormbound**.

Il giocatore percepisce sistemi differenti.

Il codice sfrutta una base comune.

---

# 21. Decisione NON presa

Questo documento **non seleziona una sola trasformazione definitiva per ogni personaggio**.

Le alternative Light / Medium / Signature devono essere conservate.

Motivi:

- future versioni possono richiedere maggiore complessità;
- alcune idee possono essere riutilizzate come skill;
- una Signature scartata come Transform può diventare Ultimate;
- una Medium può diventare Stance;
- una Light può diventare Passive;
- alcune alternative possono essere usate in scenari o modalità speciali.

---

# 22. Strategia di prototipazione consigliata

Ordine:

1. **Wraith — Siege Mode**
   - validare cambio di forma;
   - override di movimento;
   - override di una skill;
   - Action Ghost;
   - revert.

2. **Gadget — Charged State**
   - validare trigger ambientale;
   - modifica di skill;
   - interaction tags.

3. **Riktor — Bulwark**
   - validare collisione/cover;
   - character come elemento tattico.

4. **Phase — Flow State**
   - validare uno stato leggero e temporaneo.

5. **GRIM.exe — Kernel Override**
   - validare 3 configurazioni.

6. **Kwang — Stormbound**
   - validare relazione personaggio ↔ oggetto ↔ terreno.

7. **Aurora — Winter Avatar**
   - stress test del sistema terreno.

---

# 23. Metriche da osservare nei test

Durante gli scenari automatici/manuali raccogliere almeno:

- tempo aggiuntivo di Planning;
- numero medio di cambi di stato;
- frequenza di Transform/Revert;
- percentuale di turni in cui una forma domina tutte le alternative;
- errori di previsione del giocatore;
- azioni annullate perché incompatibili con lo stato;
- utilizzo reale delle skill mutate;
- differenza di win rate tra forme;
- numero di tooltip consultati;
- numero di stati contemporaneamente presenti nella partita.

## Segnale positivo

Il giocatore dice:

> "Devo scegliere quale modalità mi conviene usare."

## Segnale negativo

Il giocatore dice:

> "Non ricordo cosa cambia in questa modalità."

---

# 24. Principio conclusivo

> **Una buona trasformazione deve aumentare le decisioni strategiche più di quanto aumenti le informazioni da ricordare.**

Le idee Signature vanno conservate perché possono diventare elementi fortemente identitari del roster, ma non devono essere implementate tutte contemporaneamente.

La priorità è costruire **un framework flessibile**, provarlo con casi a complessità crescente e decidere personaggio per personaggio quanto del sistema esporre realmente al giocatore.
