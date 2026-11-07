# Refactor Tactics — Master Technical Specification Document (input ricevuto il 2026-11-07)

> `RESEARCH` · **Materiale NON autorevole.** Testo consegnato dall'autore il 2026-11-07 con la richiesta
> *«aggiorniamo con queste info»*, tramite `/sc:spec-panel`. È conservato qui **com'è stato ricevuto**, con la
> sola riformattazione in Markdown (titoli, tabelle, blocchi di codice); le parole sono quelle dell'autore.
>
> ⚠️ **Il documento dichiara di sé di essere «riferimento assoluto» per un prototipo C# / .NET.** Questo
> repository è Unreal Engine 5.8.1 in C++ ([`AGENTS.md`](../../../AGENTS.md) §1), e la fase C# è chiusa dal
> 2026-08-05 ([`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) §2). L'autorità resta quella di
> [`docs/README.md`](../../README.md) §«Gerarchia delle fonti»: Decision Log, ADR, spec owner, cataloghi.
> Questo file non risolve alcun conflitto.
>
> 🔎 **La misura contro il contratto corrente è nel referto del panel**:
> [`master-spec-csharp-spec-panel-2026-11-07.md`](../../roadmap/plans/master-spec-csharp-spec-panel-2026-11-07.md).
> Ogni affermazione qui sotto è classificata là (allineata, parziale, in conflitto, assente o fuori
> piattaforma), con la fonte che governa.

---

Questo documento costituisce la specifica tecnica, concettuale e sistemica unificata di Refactor Tactics,
progettata come riferimento assoluto (System Prompt / Context File) per Claude durante l'aggiornamento, il
refactoring e l'espansione del codice sorgente del prototipo in C# / .NET.

## 1. Filosofia di Design & Architettura Deterministica

- **100% Deterministico (Zero RNG)**: nessun dado invisibile, nessuna percentuale di precisione (Hit Rate %) o
  variazione casuale nel danno. A parità di stato iniziale e di sequenza di input pianificata, l'esecuzione
  produce sempre e inequivocabilmente lo stesso identico output.
- **Turni Simultanei (Finestra di 45 Secondi)**: match calibrati per 3v3 o 4v4 in cui tutti gli eroi (6 o 8
  contemporaneamente) pianificano le loro mosse entro un timer sincronizzato di 45 secondi.
- **Risoluzione Cronologica in 4 Fasi**: le azioni pianificate vengono eseguite su scala temporale rigida:
  - **Prep Phase (Fase 0)**: buff, posizionamento trappole/muri, stealth, riscrittura della mappa via Flux.
  - **Dash Phase (Fase 1)**: macro-movimento tra esagoni, scatti tattici, salti acrobatici, sbalzi cinetici.
  - **Blast Phase (Fase 2)**: attacchi diretti primari, skill offensive pesanti, Finisher e Ultimate.
  - **DoT / Reaction Phase (Fase 3)**: danni nel tempo, risoluzione delle Reaction (Overwatch, Guard, Brace).
- **Economia Azioni (AP)**: ogni eroe dispone di un budget fisso di 6 Action Points (AP) per turno, spendibili
  nelle 4 fasi. Un eventuale 7° AP è sbloccabile tramite l'accumulo dell'Overclock Meter.

## 2. Geometria Spaziale a Due Livelli: Macro-Esagoni e Micro-Spicchi

Il terreno di gioco non impiega blocchi monolitici "tutto o niente", bensì un'architettura geometrica scalata
su due livelli distinti.

```text
(diagramma come ricevuto: la copia ha perso parte dei vertici e dei punti medi)
        V1
      / │ \
     /  │  \
   M6   │   M1
   /    │    \
  /  t12│t1   \
V6──────C──────V2
  \  t7 │t6   /
   \    │    /
   M5   │   M2
     \  │  /
      \ │ /
        V4
```

### A. Livello Macro (Griglia Esagonale Pura)

- **Dimensioni Casella**: esagoni con lato L = 1,5 m. Distanza centro-centro tra caselle adiacenti = 2,6 m.
  Superficie ≈ 5,85 m².
- **Occupazione Esclusiva**: 1 solo personaggio per esagono. Non è consentita alcuna sovrapposizione
  stazionaria.
- **Macro-Movimento su 6 Direttrici**: il transito da un esagono all'altro avviene esclusivamente lungo i 6 assi
  cardinali esagonali (passi angolari di 60°).

### B. Livello Micro (Sottogriglia Interna a 12 Spicchi)

**Struttura Vettoriale**: ogni macro-esagono è suddiviso internamente in 12 triangoli d'angolo di 30° ciascuno
(t1 … t12), generati da:

- 6 raggi radiali interni Centro-Vertice (C → V, lunghezza 1,5 m);
- 6 raggi radiali interni Centro-Punto Medio (C → M, lunghezza 1,3 m);
- 6 sub-segmenti perimetrici esterni Vertice-Punto Medio (V → M, lunghezza 0,75 m).

**Funzioni Tattiche del Micro-Livello**:

- **Micro-Posizionamento**: determina lo spicchio specifico in cui l'eroe staziona (es. incollato a un muro o in
  campo aperto).
- **Facing (Orientamento di Sguardo)**: direzione vettoriale dell'eroe (arco primario di 30°–60°).
- **Flanking (Colpo alle Spalle)**: qualsiasi attacco che colpisce l'eroe dal cono posteriore (90°–120° rispetto
  al Facing) infligge il +50% di danno fisso e ignora il 50% dell'Armatura Fisica (DEF) del bersaglio.
- **Linea di Vista (LoS)**: tracciamento millimetrico dei coni di tiro attraverso fessure di copertura fino a
  soli 30°.

## 3. Movimento, Collisioni e Body Block

Il movimento gestisce inerzia, rumorosità e scontro fisico.

### A. Modalità di Movimento

| Tipo di movimento | Costo AP | Fase temporale | Distanza massima | Firma acustica / Fog of War | Note tattiche |
|---|---|---|---|---|---|
| Micro-Shift / Furtivo | 1 AP | Prep Phase | 1 esagono o riorientamento | Silenzioso (0 m) | Ruota il Facing fino a 180° o scivola di 1 casella. Non attiva Overwatch. |
| Corsa Tattica | 2 AP | Dash Phase | 2–3 esagoni | Raggio 2 esagoni | Movimento standard. Innesca le Reaction Overwatch nel raggio. |
| Sprint Veloce | 3 AP | Dash Phase | 4–5 esagoni | Raggio 4 esagoni | Consuma metà AP. Rivela la posizione sonora nella Fog of War nemica. |
| Disimpegno / Roll | 2 AP | Dash Phase | 1 esagono | Basso | Schivata che spezza l'ingaggio corpo a corpo senza subire attacchi d'opportunità. |

### B. Risoluzione dei Conflitti di Movimento (Body Block)

Poiché vige la regola di 1 eroe per esagono:

- **Transito tra Alleati**: un eroe può attraversare un esagono occupato da un compagno durante la Dash Phase,
  ma non può fermarvisi.
- **Contesa Simultanea dello Stesso Esagono**:
  - *Arrivo per Priorità di Fase*: se un'unità spende AP in Prep Phase per anticipare il tragitto, si impossessa
    della casella. L'unità nemica rimbalza nello spicchio dell'esagono precedente subendo lo stato Body Blocked.
  - *Contesa Perfetta (stessa fase, stessa distanza)*: entrambi gli eroi collidono al centro casella: rimbalzano
    entrambi all'indietro nella posizione di partenza e subiscono 50 danni cinetici fissi da impatto.

## 4. Catalogo dei 12 Muri, Coperture e Riscrittura Flux

I muri corrono lungo i segmenti radiali o perimetrici dei 12 spicchi.

```text
[Attaccante] ──(Raggio LoS 30°)──> [Segmento Muro] ──> Calcolo DR / Blocco Colpo
                                        │
                              [Reazione con Elemento Flux]
                                        ▼
                              Riconfigurazione Fisica
```

**Parametri base**

- **HP**: resistenza strutturale (50 HP … ∞).
- **LoS**: Bloccante (opaco), Filtro (trasparente con penalità mira), Ingannevole.
- **DR (Damage Reduction)**: riduzione percentuale fissa del danno assorbito (15%–75%).
- **Passaggio**: Bloccato, Scavalcabile (+1 AP), Volo/Acrobatico.

**I 12 tipi di coperture**

| # | Copertura | Geometria | HP | LoS | DR | Passaggio | Reazione al Flux |
|---|---|---|---|---|---|---|---|
| 1 | Parete in Cemento Armato (Concrete Wall) | segmento perimetrico o radiale | 300 | Bloccante | 75% | Bloccato | *Riconfigurazione Trasparente*: diventa traslucido per 1 turno (mantiene blocco fisico e DR ma apre la LoS in entrambe le direzioni) |
| 2 | Barricata Leggera / Macerie (Low Cover) | sub-segmento V → M alto 1 m | 120 | Libera | 35% | Scavalcabile (+1 AP) | *Liquefazione Temporanea*: si appiattisce al suolo per 2 turni azzerando la copertura |
| 3 | Lastra d'Acciaio Corazzato (Heavy Steel Plate) | radiale C → M | ∞ | Bloccante | 60% | Bloccato | *Inversione Magnetica*: riflette ballisticamente i colpi ed elettrifica lo spicchio adiacente |
| 4 | Pilastro Centrale di Sostegno (Core Column) | centro C (diametro 30 cm) | ∞ | Bloccante (cono d'ombra di 60°) | 50% | — | *Onda di Risonanza*: impulso radiale a 360° che spinge via chiunque si trovi adiacente |
| 5 | Barriera Force Shield (Steel) | perimetrico + radiale a "L" | 200 | Libera per alleati / Bloccante per nemici | 50% | — | *Polarizzazione Riflettente*: rimanda il primo proiettile nemico al mittente |
| 6 | Muro Sismico di Roccia (Feng Mao / Steel) | 2 radiali contigui | 150 | Bloccante | 40% | — | *Polverizzazione Magmatica*: si scioglie in lava (copertura rimossa, spicchio con DoT Fuoco per 2 turni) |
| 7 | Siepe Rigenerativa (The Fey) | perimetro intero casella | 80 | Bloccante | 20% | — · cura alleati +15 HP/turno | *Spore di Parassita*: converte le cure in radici che bloccano i nemici adiacenti per 1 turno |
| 8 | Parete a Ologramma Tattico (Holo-Wall) | radiale C → M | 0 | Ingannevole | 0% | Permesso | *Solidificazione Fotonica*: diventa un muro di vetro solido da 100 HP per 2 turni |
| 9 | Portale d'Accelerazione (Gadget) | radiale | ∞ | Libera | 0% | +2 esagoni mobilità | *Vortice Cinetico*: inverte il flusso, respinge indietro chi prova a varcarlo in Dash Phase |
| 10 | Barriera a Campo Arcano (Gadget) | anello a 12 spicchi | 150 | Libera | 25% | — · silenzia le Reaction | *Sovraccarico Energetico*: esplode infliggendo danno Arcano ad area negli spicchi interni |
| 11 | Condotto di Vapore (Steam Vent) | grata su punto medio perimetrale | ∞ | Filtro nebbia | 15% | — | *Getto Corrosivo*: spara vapore ad alta pressione a 90° che acceca i bersagli per 1 turno |
| 12 | Cristallo di Energia Grezza (Flux Cluster) | monolito su 3 spicchi (90°) | 50 reattivi | Bloccante | 50% | — | *Risonanza a Catena*: detona e innesca all'istante qualsiasi combo incompleta sulle caselle adiacenti |

## 5. Meccanica Elementale: Combo a 3 Cariche & Il Ruolo del Flux

Il sistema magico ed energetico non segue una tabella rigida prefissata sasso-carta-forbice, ma un accumulo ad
alta densità:

- **Pool Elementale**: Flux, Fuoco, Acqua, Terra, Fulmine, Freddo, Morte, Vita, Arcano, Scudo, Aria,
  Disintegrazione.
- **Stack a 3 Elementi**: qualsiasi bersaglio o cella che riceve 3 elementi scatena un Combo Finisher. Non conta
  l'ordine con cui vengono applicati i primi due elementi.
- **Finisher scatenato dalla fase temporale del 3° elemento**:
  - chiusura in Prep Phase: effetto di manipolazione ambientale/buff difensivo (150–250 Danno Bonus);
  - chiusura in Dash Phase: sbalzo cinetico, riorientamento forzato o displacement (250–350 Danno Bonus);
  - chiusura in Blast Phase: Critico Finisher Puro (400–600 Danno Bonus ad alto impatto).
- **Il Ruolo Chiave del Flux**: il Flux funge da elemento di riscrittura molecolare e manipolazione dello
  spazio: è l'unico elemento in grado di alterare lo stato fisico dei muri e delle geometrie interne dell'arena.

## 6. Sistema di Combo-Breaking Tattico (Senza Coppie Fisse)

Per evitare l'appiattimento di un sistema ad annullamento binario (es. Acqua spegne Fuoco), la rottura di una
combo avversaria si basa su 3 vettori fisici e geometrici:

1. **Disruption Temporale**: anticipare la chiusura della combo applicando stordimenti o sottrazioni di AP nella
   fase antecedente a quella del finisher avversario.
2. **Displacement Geometrico**: spostare il bersaglio o l'attaccante lungo i 6 assi esagonali per mandare fuori
   bersaglio il cono a 30° della LoS.
3. **Sovraccarico Reattivo (Reaction Guard)**: intercettare fisicamente il vettore d'impatto o solidificare
   coperture tramite abilità a reazione.

**Case Study: Riktor come Combo-Breaker Triplo**

- *Contro Combo di Congelamento/Cecchinaggio*: utilizza Chain Hook (Dash Phase, 2 AP) per agganciare il
  compagno preso di mira o il cecchino nemico, sbalzandolo di 2 esagoni ed estraendolo dal cono di tiro prima
  della Blast Phase.
- *Contro Riconfigurazione Flux + Snipe*: utilizza Chain Intercept (Reaction, 2 AP). Nel momento in cui il colpo
  nemico passa attraverso il muro smaterializzato dal Flux, la catena polarizza il segmento forzandone la
  solidificazione istantanea e bloccando il tiro.
- *Contro Dash Aggressivo + Finisher Ravvicinato*: attiva Electrocute (Prep Phase, 1 AP) saturando l'esagono di
  cariche. All'ingresso del nemico in Dash, chiude con Skewer (Blast Phase, 3 AP) che acquisisce priorità
  deterministica sui bersagli elettrificati e stordisce l'avversario prima del suo fendente.

## 7. Roster degli 8 Eroi: Statistiche & Bilanciamento Numerico

Formula del danno deterministico:

```text
Danno Reale = (Danno Base × Modificatori) × ( 100 / (100 + DEF o RES) )
```

### 7.1 Riktor (Tank / Disruptor)

HP 1750 · DEF 45 (~31% red.) · RES 35 (~26% red.) · Move 3 esagoni/turno

| Slot | Costo | Abilità | Elemento | Effetto |
|---|---|---|---|---|
| Attacco base | 2 AP | Shock Whip | Fulmine | 160 Danno Energetico |
| Prep | 1 AP · CD 1 | Electrocute | Fulmine | aura da 80 Danno Energetico/turno per 2 turni |
| Dash | 2 AP · CD 2 | Chain Hook | Terra | trascina il bersaglio di 2 caselle (o se stesso verso un muro). 140 Danno |
| Blast | 2 AP · CD 2 | Corrosive Shackles | Morte | 200 Danno Oscuro + blocca le cure ricevute per 2 turni |
| Blast (Ultimate) | 3 AP · CD 3 | Skewer | Fulmine | 380 Danno Energetico a cono + Stun 1 turno |
| Reaction (Guard) | 2 AP · CD 2 | Chain Intercept | Scudo | assorbe fino a 300 danni a un compagno |

### 7.2 Steel (Tank / Fortezza Difensiva)

HP 1900 · DEF 60 (~37% red.) · RES 30 (~23% red.) · Move 2 esagoni/turno

| Slot | Costo | Abilità | Elemento | Effetto |
|---|---|---|---|---|
| Attacco base | 2 AP | Heavy Punch | Scudo | 150 Danno Fisico |
| Prep | 1 AP · CD 1 | Force Shield | Scudo | muro su sub-segmento da 200 HP (DR 50%) |
| Dash | 2 AP · CD 2 | Bull Rush | Terra | carica di 3 esagoni (180 Danno + sbalzo di 1 casella) |
| Blast | 2 AP · CD 2 | Ground Shock | Fulmine | 220 Danno Fisico/Energetico a cono frontale |
| Blast (Ultimate) | 3 AP · CD 3 | Shield Slam | Aria | 320 Danno Fisico a 360° + Stun 1 turno |
| Reaction (Guard) | 2 AP · CD 2 | Bodyguard | Terra | intercetta il tiro ed erige scudo da 250 HP |

### 7.3 Kallari (Assassino Flanker)

HP 950 · DEF 15 (~13% red.) · RES 20 (~17% red.) · Move 4 esagoni/turno

| Slot | Costo | Abilità | Elemento | Effetto |
|---|---|---|---|---|
| Attacco base | 2 AP | Shadow Dagger | Morte | 190 Danno Fisico (285 da Flanking posteriore) |
| Prep | 2 AP · CD 2 | Shadow Walk | Morte | Stealth per 2 turni (+30% danno al colpo che rompe lo stealth) |
| Dash | 1 AP · CD 1 | Double Jump | Aria | salto di 2 esagoni oltrepassando ostacoli e muri fino a 2 m |
| Blast | 2 AP · CD 2 | Mist Slash | Acqua | 210 Danno Fisico + accecamento LoS nemica per 2 turni |
| Blast | 3 AP · CD 2 | Crippling Strike | Disintegrazione | 450 Danno Perforante (ignora 100% DEF alle spalle) |
| Reaction (Brace) | 2 AP · CD 2 | Shadow Dodge | Aria | schiva l'attacco traslando di 1 spicchio |

### 7.4 Feng Mao (Bruiser / Mischia)

HP 1350 · DEF 35 (~26% red.) · RES 25 (~20% red.) · Move 3 esagoni/turno

| Slot | Costo | Abilità | Elemento | Effetto |
|---|---|---|---|---|
| Attacco base | 2 AP | Blade Sweep | Fuoco | 180 Danno su 2 spicchi frontali |
| Prep | 1 AP · CD 1 | Conversion Shield | Scudo | scudo protettivo da 220 HP |
| Dash | 2 AP · CD 2 | Reaping Dash | Fuoco | scatto di 3 esagoni + scia infuocata (90 Danno/turno per 2 turni) |
| Blast | 2 AP · CD 2 | Seismic Slice | Terra | 260 Danno Fisico in linea retta su asse macro |
| Blast (Ultimate) | 3 AP · CD 3 | Earth Shatter | Disintegrazione | 480 Danno Energetico perforante che ignora coperture |
| Reaction (Brace) | 2 AP · CD 2 | Counter Slash | Disintegrazione | riduce il danno subito del 60% e risponde con 200 danni |

### 7.5 Murdock (Tiratore Ranged / Cecchino)

HP 1050 · DEF 20 (~17% red.) · RES 15 (~13% red.) · Move 3 esagoni/turno · Gittata 8 esagoni

| Slot | Costo | Abilità | Elemento | Effetto |
|---|---|---|---|---|
| Attacco base | 2 AP | Rifle Shot | Disintegrazione | 220 Danno Perforante |
| Prep | 1 AP · CD 2 | Static Trap | Fulmine | trappola a terra (120 Danno + Stun 1 turno al calpestamento) |
| Dash | 2 AP · CD 1 | Tactical Retreat | Aria | rinculo balistico di 2 esagoni mantenendo il Facing fisso sul nemico |
| Blast | 2 AP · CD 2 | Incendiary Shell | Fuoco | 240 Danno Fuoco AoE (esagono bersaglio + 3 spicchi contigui) |
| Blast (Ultimate) | 3 AP · CD 3 | Devastating Blast | Disintegrazione | 520 Danno Energetico globale a trave passante (ignora i muri) |
| Reaction (Overwatch) | 2 AP · CD 2 | Sniper Overwatch | Disintegrazione | spara colpo da 250 danni al primo nemico in movimento nel cono |

### 7.6 Gadget (Maga Tecnologica / Controllo Spaziale)

HP 1000 · DEF 15 (~13% red.) · RES 35 (~26% red.) · Move 3 esagoni/turno · Gittata 6 esagoni

| Slot | Costo | Abilità | Elemento | Effetto |
|---|---|---|---|---|
| Attacco base | 2 AP | Spark Bolt | Fulmine | 170 Danno Energetico |
| Prep | 1 AP · CD 1 | Structure Shift | Flux | riscrive o smaterializza un segmento di muro per 1 turno (oppure Speed Gate [Aria]) |
| Dash | 2 AP · CD 2 | Seeker Bot | Fulmine | droide a inseguimento (150 Danno + −1 AP al bersaglio nel turno successivo) |
| Blast | 2 AP · CD 2 | Arcane Flux | Arcano | 210 Danno Arcano AoE + silenziamento Reaction nemiche per 1 turno |
| Blast (Ultimate) | 3 AP · CD 2 | Sticky Bomb | Disintegrazione | bomba a tempo (420 Danno Energetico AoE dopo 1 turno) |
| Reaction (Overwatch) | 2 AP · CD 2 | Tesla Trap | Fulmine | elettrifica lo spicchio bersaglio con 200 Danno a chi vi entra |

### 7.7 Muriel (Supporto Scudi / Riposizionamento)

HP 1100 · DEF 25 (~20% red.) · RES 40 (~28% red.) · Move 3 esagoni/turno · Gittata 5 esagoni

| Slot | Costo | Abilità | Elemento | Effetto |
|---|---|---|---|---|
| Attacco base | 2 AP | Orb Pulse | Vita | 140 Danno a nemico OPPURE 140 Scudo/Cura ad alleato |
| Prep | 2 AP · CD 2 | Consecrated Ground | Scudo | bolla protettiva da 250 HP per 2 turni su un'area di 3 esagoni |
| Dash (Ultimate) | 3 AP · CD 3 | Reversal of Fortune | Vita | volo globale istantaneo su un compagno (450 HP scudo + 200 cura) |
| Blast | 1 AP · CD 1 | Serenity | Arcano | 110 Danno Arcano + rallentamento di 2 esagoni per 2 turni |
| Blast | 2 AP · CD 2 | Frost Binding | Freddo | 160 Danno Freddo + sottrazione di 1 AP nemico |
| Reaction (Guard) | 2 AP · CD 2 | Aegis Shield | Scudo | scudo reattivo da 220 HP su compagno bersagliato |

### 7.8 The Fey (Supporto Naturale / Crowd Control)

HP 1150 · DEF 20 (~17% red.) · RES 30 (~23% red.) · Move 3 esagoni/turno · Gittata 5 esagoni

| Slot | Costo | Abilità | Elemento | Effetto |
|---|---|---|---|---|
| Attacco base | 2 AP | Nettle Shot | Acqua | 150 Danno Energetico (applica stato Bagnato) |
| Prep | 2 AP · CD 2 | Untamed Growth | Vita | siepe da 80 HP che cura di 60 HP/turno e blocca la LoS |
| Dash | 1 AP · CD 1 | Bramble Patch | Acqua | rovi bagnati a terra (70 Danno/turno + +1 AP costo di movimento) |
| Blast | 2 AP · CD 2 | Spore Spores | Morte | 180 Danno Tossico + DoT da 50/turno per 2 turni |
| Blast (Ultimate) | 3 AP · CD 3 | Flytrap | Freddo | attira nemici entro 2 esagoni, 280 Danno ed infligge Freeze (0 movimento) |
| Reaction (Overwatch) | 2 AP · CD 2 | Vines Overwatch | Acqua | radica sul posto il primo nemico che entra nel raggio (130 Danno) |

## 8. UX, HUD e Direzione Artistica Anime / Cel-Shaded

### A. Estetica visiva

- **Rendering Cel-Shaded 3D**: stile manga/anime ad altissima chiarezza visiva e silhouette marcate.
- **Onomatopee Dinamiche Spaziali**: nei punti di rottura (critici, distruzione muri, annullamento combo),
  scritte onomatopeiche 3D si ancorano sulla griglia (es. "CRASH!", "REFACTORED!", "ELEMENT NULLIFIED!").
- **Manga Split-Screen**: se due eroi agiscono simultaneamente nella stessa fase, la visuale si divide
  diagonalmente per mostrare entrambi gli impatti in contemporanea.

### B. Struttura dell'interfaccia tattica (finestra 45 s)

- **Bussola under-foot a 12 Spicchi**: proietta costantemente sotto l'eroe la suddivisione angolare da 30°,
  evidenziando coni coperti (rossi), aperti (blu) e l'arco di Facing per prevenire il Flanking.
- **Timeline a 8 Tracce (Multi-Track Timeline)**: traccia orizzontalmente le azioni simultanee degli 8 eroi
  suddivise per colore fase: Blu = Prep Phase · Giallo = Dash Phase · Rosso = Blast Phase · Viola = Reaction
  Phase.
- **Ghost Units & Nastri Vettoriali**: ombre semitrasparenti mostrano dove l'unità atterrerà alla fine del Dash
  prima di confermare.
- **Combo Snap & Tactical Ping**: un clic destro su uno spicchio invia marker strategici agli alleati; il tasto
  Combo Snap allinea automaticamente l'elemento complementare nella fase corretta senza dover riprogrammare la
  skill.

## 9. Struttura Dati di Riferimento in C# (RefactorTactics.Core.cs)

```csharp
namespace RefactorTactics.Core
{
    public enum TemporalPhase
    {
        Prep = 0,     // Buff, piazzamento muri/trappole, stealth, riscrittura Flux
        Dash = 1,     // Macro-movimento su 6 assi, scatti, sbalzi
        Blast = 2,    // Attacchi diretti, Ultimate, Finisher
        Reaction = 3  // Overwatch, Guard, Brace (intercettazione turno avversario)
    }

    public enum ElementType
    {
        None,
        Flux,         // Elemento cardine per riscrittura e manipolazione ambientale
        Fire, Water, Earth, Lightning, Ice, Death, Life, Arcane, Shield, Air, Disintegration
    }

    public enum SkillTargetType
    {
        Self,
        AdjacentHex,   // Distanza 1 esagono (~2.6m corpo a corpo)
        RangedHex,     // N esagoni lungo le 6 direttrici macro
        SectorTarget,  // Spicchio micro-angolare (t1-t12) nell'esagono target
        LineAoe,       // Trave rettilinea lungo le direttrici
        ConeAoe        // Cono frontale basato sul Facing (arco 30°-60°-120°)
    }

    public class SkillData
    {
        public string Id { get; set; }
        public string Name { get; set; }
        public TemporalPhase Phase { get; set; }
        public int ApCost { get; set; }          // 1 a 3 AP
        public int CooldownTurns { get; set; }   // 0 a 3 turni (0 per attacco base)
        public int RangeMin { get; set; }        // Distanza minima in macro-esagoni
        public int RangeMax { get; set; }        // Distanza massima
        public ElementType PrimaryElement { get; set; }
        public SkillTargetType TargetType { get; set; }

        // Metriche Deterministiche (Zero RNG)
        public float BaseDamage { get; set; }
        public bool IgnoresArmor { get; set; }   // True per attacchi penetranti / disintegrazione
        public bool RequiresFacing { get; set; } // Verifica lo spicchio d'impatto per bonus Flanking

        // Interazione Ambientale & Muri
        public bool ModifiesWalls { get; set; }         // True se interagisce con i segmenti muro o elemento Flux
        public string StatusEffectApplied { get; set; } // Es. "Stun", "Blind", "Root"
    }

    public class CharacterState
    {
        public string HeroId { get; set; }
        public int CurrentHexId { get; set; }     // Coordinate macro-esagono
        public int SubSectorIndex { get; set; }   // Spicchio micro corrente (t1-t12)
        public int FacingSector { get; set; }     // Orientamento dello sguardo (t1-t12)
        public float CurrentHp { get; set; }
        public float MaxHp { get; set; }
        public float ArmorDEF { get; set; }
        public float ResistanceRES { get; set; }
        public int AvailableAp { get; set; } = 6;
        public bool IsOverclocked { get; set; }   // Sblocca il 7° AP
    }

    public class WallSegment
    {
        public int SegmentId { get; set; }
        public int HexId { get; set; }
        public bool IsRadial { get; set; }        // True se C->V o C->M; False se perimetrico V->M
        public int SectorIndex { get; set; }      // Spicchio t1-t12 corrispondente
        public float MaxHp { get; set; }
        public float CurrentHp { get; set; }
        public bool BlocksLoS { get; set; }       // False se alterato via Flux o se è filtro
        public float DamageReductionDR { get; set; } // Da 0.15f a 0.75f
        public bool IsRefactored { get; set; }    // True se sotto effetto attivo di Flux
        public int RefactorDurationTurns { get; set; }
    }
}
```
