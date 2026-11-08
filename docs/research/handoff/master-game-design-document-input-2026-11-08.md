# Refactor Tactics — Master Game Design Document (input ricevuto il 2026-11-08)

> `RESEARCH` · **Materiale NON autorevole.** PDF consegnato dall'autore il 2026-11-08 (creato alle 09:07 UTC, 17
> pagine) con la richiesta *«leggilo e integralo aggiornando la documentazione»*. L'originale è accanto a questo
> file: [`master-game-design-document-input-2026-11-08.pdf`](master-game-design-document-input-2026-11-08.pdf).
> Questo Markdown è l'estrazione automatica del testo (tabelle ricostruite dove il PDF le portava); le parole sono
> quelle dell'autore, e dove l'estrazione è dubbia fa fede il PDF.
>
> ⚠️ **Il documento si dichiara «Source of Truth» e precede di qualche ora le decisioni che l'autore ha preso in
> sessione lo stesso giorno** ([D-583](../../decisions/RT_PDR_00_Decision_Log.md)…[D-590](../../decisions/RT_PDR_00_Decision_Log.md)).
> Dove diverge da quelle decisioni, **non governa**: l'autore lo ha stabilito con [D-591](../../decisions/RT_PDR_00_Decision_Log.md),
> e i sette punti di divergenza sono elencati nel referto. Dove non confligge, è **input di design** per le istruttorie
> aperte (`MSP-1.n`, `MSP-3.c`, `MSP-4.n`, `MSP-5.d`), per E26, per E52.1 e per la progettazione HUD. L'autorità resta
> quella di [`docs/README.md`](../../README.md) §«Gerarchia delle fonti»: Decision Log, ADR, spec owner, cataloghi.
> Questo file non risolve alcun conflitto.
>
> 🔎 **La misura contro il contratto corrente è nel referto del panel**:
> [`master-gdd-spec-panel-2026-11-08.md`](../../roadmap/plans/master-gdd-spec-panel-2026-11-08.md).
> Il documento precedente dello stesso autore, superato da questo come input di design:
> [`master-technical-spec-csharp-input-2026-11-07.md`](master-technical-spec-csharp-input-2026-11-07.md).

---

MASTER GAME DESIGN DOCUMENT & SOURCE OF TRUTH 

# REFACTOR TACTICS 

Architettura Deterministica, Sistema Alchemico a 9 Elementi, Roster 8 Eroi, Grammatica UI/UX e Motore Swarm AI 

## 1. Core Vision & Pilastri Fondamentali 

Refactor Tactics è un gioco tattico competitivo a turni simultanei in formato 3v3 o 4v4 (6 o 8 eroi attivi contemporaneamente sulla mappa). Fonde la pianificazione simultanea scandita in fasi di Atlas Reactor, le combinazioni alchemiche ambientali di Magicka e il posizionamento geometrico rigoroso di Into the Breach e XCOM. 

- Determinismo Assoluto (0% RNG): Non esistono tiri di dado invisibili, percentuali di precisione (Hit Rate = 100% se il bersaglio è in Linea di Vista) o intervalli di danno casuali. A parità di stato iniziale e sequenza di input, il risultato è matematicamente identico. 

- Finestra di Pianificazione Simultanea: Tutti i giocatori pianificano le proprie azioni contemporaneamente entro un timer reale di 45 secondi, seguito dalla fase di risoluzione cinematica condivisa. 

- Lore Tecnica — L'Energia "Flux": L'ambientazione ruota attorno a un'energia anomala di "Refactoring" (Flux) capace di riscrivere la densità, la massa e la struttura molecolare di oggetti, coperture ed elementi sulla scacchiera. 

## 2. Regole di Gioco, Tempo, Spazio e Movimento 

### 2.1 Le 4 Fasi Cronologiche di Risoluzione 

Le mosse pianificate dai giocatori non si risolvono per iniziativa del singolo personaggio, ma attraverso 4 Fasi Temporali sequenziali: 

1. Prep Phase (Fase 0 — Cerchio Blu): Posizionamento di trappole, innalzamento di scudi e barriere, buff, entrata in stealth, Passo Furtivo e alterazione geometrica dei muri tramite Flux. 

2. Dash Phase (Fase 1 — Chevron Giallo): Movimenti macro tra gli esagoni, scatti, salti sull'asse verticale (Asse Z), cariche e sbalzi o trascinamenti cinetici (Knockback / Pull). 

3. Blast Phase (Fase 2 — Rombo Rosso): Attacchi diretti, proiettili, raggi ad area, esecuzioni e Ultimate offensive. 

4. Reaction / DoT Phase (Fase 3 — Esagono Viola): Risoluzione dei danni continui (DoT) e innesco delle difese reattive (Reaction) in risposta alle azioni nemiche all'interno della fase corrispondente. 

Priorità e Tie-Break Deterministico: Le Reaction hanno sempre priorità sulle abilità attive standard all'interno della fase in cui si innescano. Se due giocatori lanciano un attacco nella stessa identica fase, i colpi avvengono simultaneamente (entrambi infliggono danno). Eventuali stati alterati applicati da un colpo simultaneo (es. Stun) entrano in vigore a partire dal turno successivo senza annullare l'azione già in esecuzione nel turno corrente. 

### 2.2 Geometria Spaziale a Due Livelli e Verticalità (Asse Z) 

- Livello Macro (Griglia Esagonale): Esagoni con lato L = 1,5 m, distanza centro-centro tra esagoni adiacenti di 2,6 m (raggio del corpo a corpo) e superficie di ≈ 5,85 m<sup>2</sup> . Vige la regola dell'Occupazione Esclusiva (massimo 1 solo eroe stazionario per esagono). Il transito tra esagoni avviene lungo le 6 direttrici cardinali (passi angolari di 60°). 

- Livello Micro (Sottogriglia Interna a 12 Spicchi): Ogni esagono è diviso in 12 triangoli rettangoli da 30° ciascuno (t1 … t12), delimitati da 6 raggi Centro-Vertice (C → V, 1,5 m), 6 raggi Centro-Punto Medio (C → M, 1,3 m) e 6 segmenti perimetrici (V → M, 0,75 m). Stare al centro (C) offre massima mobilità d'uscita ma espone a 360°; posizionarsi in uno spicchio (tn) permette di sfruttare i muri radiali/perimetrici e sporgersi di 30° per sparare. 

- Facing & Flanking: Ogni eroe ha un orientamento di sguardo (Facing) allineato a uno spicchio. Colpire un bersaglio nel suo cono posteriore (90°–120°, ovvero i 4 spicchi opposti al Facing) innesca il Flanking: +50% di danno fisso e ignora il 50% dell'Armatura Fisica (DEF). 

- Asse Z (Elevazione a 3 Livelli): Livello 0 (Suolo), Livello 1 (Rialzo/Casse), Livello 2 (Tetto/Strutture). Se Zattaccante > Zdifensore, l'attacco ignora automaticamente la Riduzione Danno (DR) delle coperture basse. Gli sbalzi cinetici giù da un dislivello infliggono danni da caduta deterministici non mitigabili. 

### 2.3 Economia AP, Movimenti Generici e Collisioni 

Ogni eroe dispone di un budget fisso di 6 Action Points (AP) per turno. Un cambio di direzione a U (180°) nello stesso esagono durante la corsa consuma 1 AP extra o riduce il tragitto di 1 casella. Un eroe può attraversare la casella di un alleato in Dash Phase ma non può fermarvisi; i nemici creano invece un blocco fisico (Body Block) che arresta il movimento o costringe a spendere AP extra per aggirarli. 

|Azione /<br>Movimento|Costo<br>AP|Fase|Distanza|Rumore /<br>Footprint|Regola Tattica|
|---|---|---|---|---|---|
|Passo Furtivo<br>(Micro-Shift)|1 AP|Prep|1 hex (2,6 m) o<br>rotazione 180°|Silenzioso (0<br>m)|Non innescaOverwatch<br>nemici.|
|Corsa Tattica<br>(Tactical Run)|2 AP|Dash|2–3 hex (5,2–7,8<br>m)|Medio (Raggio<br>2 hex)|Movimento base; innesca<br>Overwatch.|
|Scatto Veloce (Fast<br>Sprint)|3 AP|Dash|4–5 hex (10,4–<br>13,0 m)|Alto (Raggio 4<br>hex)|Traccia acustica visibile<br>nellaFog of War.|
|Disimpegno<br>(Disengage Roll)|2 AP|Dash|1 hex (2,6 m)|Basso|Esce dal corpo a corpo<br>senza subire penalità.|
|Interazione Mappa|1 AP|Prep/<br>Blast|Adiacente / In cella|—|Cattura iRefactor Nodes.|

### 2.4 Meccanica di Overclock (+2 AP con Debito) e Progressione Orizzontale 

Overclock (Debito Tattico / Phase Lock): Durante i 45 secondi di pianificazione, il giocatore può attivare liberamente l'Overclock ottenendo istantaneamente +2 AP nel turno corrente (portando il budget a 8 AP). Come pagamento obbligatorio, deve dichiarare subito quale Fase Temporale (Prep, Dash, Blast o Reaction) verrà bloccata integralmente (Phase Lock) nel suo turno successivo. Nel turno seguente, la fase sacrificata è sigillata sull'HUD con strisce di pericolo rosse e nere, visibili a tutti gli avversari. 

Progressione Orizzontale (Sidegrades — Zero Power Creep Numerico): Salendo di maestria con un eroe non aumentano mai HP o Danni base (preservando la purezza matematica), ma si sbloccano moduli logici pre-partita: 

- Sintonizzazione Elementale (Liv. 5): Modifica l'elemento di una specifica abilità (es. Seeker Bot di Gadget da Fulmine a Fuoco). 

- Spostamento Temporale / Phase Shifting (Liv. 10): Sposta una skill in una Fase diversa bilanciandone AP e gittata (es. Chain Pull di Riktor spostata da Dash a Prep Phase a 3 AP, raggio 3). 

- Modificatori di Debito Overclock: Credito a Lungo Termine (+2 AP ora, nessun Phase Lock, ma rigenerazione scende da 6 a 4 AP per i 2 turni successivi), Sovraccarico Biologico (+2 AP ora, nessun Phase Lock, ma sacrifica istantaneamente il 35% degli HP massimi come danno puro non letale) o Cortocircuito (+2 AP ora, nessun Phase Lock, ma disabilita permanentemente un'abilità difensiva per il resto del match). 

### 2.5 Formula del Danno, Combo-Breaking e Modalità "Core Refactor" 

Danno Reale = (Danno Base × Modificatori) × [ 100 / (100 + DEF o RES) ] × (1 − DRCopertura) 

Dove DEF riduce i danni fisici/cinetici e RES riduce i danni energetici/elementali. Non esistono annullamenti binari astratti; una combo nemica si spezza tramite 3 vettori di Combo-Breaking: Disruption Temporale (Stun o drenaggio AP nella fase antecedente), Displacement Geometrico (spostare il bersaglio o l'attaccante fuori dal cono di tiro di 30° prima della Blast Phase) o Sovraccarico Reattivo (intercettare con Reaction Guard o risolidificare un muro alterato dal Flux). 

Modalità Principale "Core Refactor": La mappa presenta 3 Refactor Nodes. Catturarne uno costa 1 AP (in Prep o Blast). Ogni 2 turni i nodi rilasciano un impulso Flux che potenzia le combo negli esagoni adiacenti. Vince la prima squadra che raggiunge 100 Punti Energia (generati dai nodi e dai K.O. nemici). 

## 3. Muri, Coperture e Interazione Flux 

I segmenti di muro corrono lungo i raggi interni (C → V, C → M) o lungo il perimetro (V → M) dei 12 spicchi. La tabella seguente codifica i 12 Archetipi di Muro e la loro reazione deterministica quando colpiti dall'elemento Flux: 

|#|Archetipo Muro /<br>Copertura|HP<br>Struttura|DR|Linea di Vista (LoS) /<br>Regole Base|Reazione al Refactoring<br>(Elemento Flux)|
|---|---|---|---|---|---|
|1|Concrete Wall<br>(Cemento)|300 HP|75%|Blocca LoS e movimento<br>fisico.|Diventa ologrammaWireframeper<br>1 turno (LoS e colpi passano).|
|2|Low Cover<br>(Copertura Bassa)|120 HP|35%|LoS libera; scavalcabile<br>(+1 AP in Dash).|Liquefazione per 2 turni (perde il<br>bonus difesa per i nemici).|
|3|Heavy Steel Plate<br>(Acciaio)|Infinito|60%|Segmento radiale; blocca<br>LoS e movimento.|Inversione Magnetica (respinge<br>proiettili ed elettrifica lo spicchio).|
|4|Core Column<br>(Colonna)|Infinito|50%|Centro C; genera cono<br>d'ombra di 60°.|Onda di Risonanza radiale a 360°<br>(respinge chi è a ridosso).|
|5|Force Shield<br>(Barriera Steel)|200 HP|50%|LoS libera per alleati,<br>bloccante per nemici.|Polarizzazione Riflettente (riflette il<br>1° proiettile nemico).|
|6|Seismic Rock Wall<br>(Roccia)|150 HP|40%|Occupa 2 raggi radiali<br>consecutivi; blocca LoS.|Polverizzazione Magmatica<br>(diventa pozza di lava con DoT<br>Fuoco 2T).|
|7|Regenerative<br>Hedge(Siepe)|80 HP|20%|Rigenera +15 HP/turno;<br>blocca LoS.|Spore Parassite (applicaRootper<br>1 turno ai nemici adiacenti).|
|8|Holo Wall(Muro<br>Olografico)|0 HP|0%|LoS ingannevole;<br>passaggio consentito.|Solidificazione Fotonica (diventa<br>muro solido da 100 HP per 2 turni).|
|9|Speed Gate<br>(Portale Gadget)|Infinito|0%|Passaggio libero; +2<br>esagoni di movimento.|Vortice Cinetico (inverte la spinta<br>respingendo chi tenta di varcarlo).|
|10|Arcane Field<br>Barrier|150 HP|25%|Anello a 12 spicchi;<br>silenzia leReaction.|Sovraccarico Energetico (esplode<br>infliggendo danno AoE interno).|
|11|Steam Vent(Sfiato<br>Vapore)|Infinito|15%|Punto medio perimetrale;<br>LoS Filtro nebbia.|Getto Corrosivo ad alta pressione<br>(applicaBlindsu cono di 90°).|
|12|Flux Cluster<br>(Cristallo Flux)|50 HP|50%|Occupa 3 spicchi (90°);<br>blocca LoS e passaggio.|Risonanza a Catena (detona le<br>combo incomplete negli esagoni<br>vicini).|

## 4. Sistema Alchemico a 9 Elementi e Risonanza Ambientale 

### 4.1 I 9 Elementi Ufficiali e le Regole di Durata 

Ancoraggio sull'Esagono (Risonanza Ambientale): Gli elementi non si accumulano sui personaggi, ma direttamente sulle celle esagonali. Trascinare o spingere un nemico dentro una cella carica innesca gli effetti su di lui. Le cariche elementali e gli Stati Composti Tier 2 sull'esagono durano 2 turni; gli stati alterati minori (Slow, DoT) durano 2 turni; gli stati maggiori (Stun, Root, Silence, Blind) durano 1 turno. 

- 6 Elementi Base (Tier 1): Fuoco, Acqua, Terra, Aria, Fulmine, Freddo. 

- 3 Catalizzatori / Modificatori Speciali: Scudo (Barriere, mitigazione e supporto), Disintegrazione (Danno perforante puro e anti-struttura), Flux (Riscrittura geometrica e manipolazione muri). 

### 4.2 Matrice di Fusione (Tier 1 + Tier 1 = Stato Composto Tier 2) 

L'ordine di applicazione dei primi due elementi non conta. La sovrapposizione di 2 elementi base muta fisicamente il materiale dell'esagono in uno Stato Composto (Tier 2) persistente: 

|+|FUOCO|ACQUA|TERRA|ARIA|FULMINE|FREDDO|
|---|---|---|---|---|---|---|
|FUOCO|Fuoco<br>Infernale<br>(DoT Puro<br>Incurabile)|Vapore<br>(Acceca,<br>annulla LoS)|Magma<br>(Costo AP<br>move ×2)|Tempesta<br>Fuoco<br>(Espande<br>danno<br>vicini)|Plasma<br>(-50% RES<br>energetica)|Shock Termico<br>(0% DEF fisica)|
|ACQUA|Vapore<br>(Acceca,<br>annulla LoS)|Allagamento<br>(Gittata<br>Ranged -50%)|Fango<br>(Blocca Dash<br>Phase)|Nebbia<br>(Schivata<br>da lontano)|Acqua Elettrif.<br>(Stun se<br>attraversata)|Ghiaccio<br>(Root assoluto)|
|TERRA|Magma<br>(Costo AP<br>move ×2)|Fango<br>(Blocca Dash<br>Phase)|Sisma<br>(Sbalza via<br>dall'hex)|Tempesta<br>Sabbia<br>(Blind + No<br>Reaction)|Faglia Ionizzata<br>(Attira i proiettili)|Permafrost<br>(Muro<br>indistruttibile)|
|ARIA|Tempesta<br>Fuoco<br>(Espande<br>danno vicini)|Nebbia<br>(Schivata da<br>lontano)|Tempesta<br>Sabbia<br>(Blind + No<br>Reaction)|Vuoto<br>Pneumat.<br>(Risucchia<br>al centro)|Tempesta Ionica<br>(Silenzia abilità)|Bufera<br>(Slow + DoT<br>cumul.)|
|FULMINE|Plasma<br>(-50% RES<br>energetica)|Acqua<br>Elettrif.<br>(Stun se<br>attraversata)|Faglia<br>Ionizzata<br>(Attira i<br>proiettili)|Tempesta<br>Ionica<br>(Silenzia<br>abilità)|Sovraccarico<br>(Azzera AP a chi<br>sosta)|Superconduttore<br>(Raggio catene +)|
|FREDDO|Shock<br>Termico<br>(0% DEF<br>fisica)|Ghiaccio<br>(Root<br>assoluto)|Permafrost<br>(Muro<br>indistruttibile)|Bufera<br>(Slow +<br>DoT<br>cumul.)|Superconduttore<br>(Raggio catene +)|Zero Assoluto<br>(Paralisi 2T +<br>Invuln.)|

#### Fusioni Tier 2 con i 3 Catalizzatori (Scudo, Disintegrazione, Flux): 

- Scudo + Terra = Bastione Sismico: Aumenta la riduzione danni della cella al 75%. 

- Scudo + Fulmine = Specchio Elettrico: Riflette danni elettrici sugli attaccanti. 

- Scudo + Freddo = Bolla Ibernata: Assorbe il 100% del prossimo attacco in arrivo. 

- Scudo + Acqua = Bolla Rigenerante: Gli alleati all'interno recuperano HP e 1 AP. 

- Fulmine + Flux = Muro Elettrificato: I muri dell'esagono conducono elettricità danneggiando chi è in copertura. 

- Fuoco + Flux = Terreno Fuso: Liquefà la superficie raddoppiando il costo AP per muoversi. 

- Fulmine + Disintegrazione = Griglia Molecolare: Silenzia le Reaction nemiche e consuma +1 AP per esagono. 

- Acqua + Disintegrazione = Nebbia Corrosiva: Blocca le cure ricevute e degrada le coperture leggere. 

### 4.3 Catalogo Completo delle Detonazioni (Tier 2 + 3° Elemento = Finisher Tier 3) 

L'inserimento del 3° elemento fa esplodere e consumare lo Stato Tier 2, resettando l'esagono. L'effetto deterministico del Finisher cambia radicalmente in base alla Fase Temporale della 3ª abilità: 

|Stato Tier 2|3°<br>Elemento|Fase<br>Chiusura|Nome Finisher<br>(Tier 3)|Effetto Deterministico & Impatto<br>Tattico|
|---|---|---|---|---|
|Ghiaccio<br>(Acqua+Freddo)|Terra|Prep|Shrapnel Trench|Il ghiaccio si sfalda creandoTerreno<br>Accidentatoche ferisce e arresta chi vi<br>cammina.|
|Ghiaccio<br>(Acqua+Freddo)|Terra|Dash|Vector Avalanche|Sbalza il blocco di ghiaccio col nemico<br>dentro di 2 esagoni e lo frantuma a fine<br>corsa.|
|Ghiaccio<br>(Acqua+Freddo)|Terra|Blast|Lethal Shatter|Implosione interna da500 Danni Puri<br>(ignora il 100% della DEF fisica).|
|Ghiaccio<br>(Acqua+Freddo)|Terra|Reaction|Ablative Armor|Su alleato ghiacciato: sfalda il ghiaccio<br>parando il 100% di un colpo letale in<br>arrivo.|
|Ghiaccio<br>(Acqua+Freddo)|Fuoco|Prep/<br>Dash/<br>Blast|Thermal Fog /<br>Steam Vent /<br>Geyser|In Prep crea nebbia curativa (HoT); in<br>Dash respinge i vicini di 1 hex; in Blast<br>sbalza a Z+2 con danni da caduta.|
|Ghiaccio<br>(Acqua+Freddo)|Flux|Prep/<br>Blast|Ice Mirror /<br>Molecular Freeze|In Prep congela il muro adiacente<br>facendogli riflettere i tiri Ranged; in<br>Blast riduce di 1 la rigenerazione AP del<br>bersaglio.|
|Magma<br>(Fuoco+Terra)|Aria|Prep|Sulfur Cloud|Genera nube tossica 3×3 che applica<br>Silence (-1 AP)a chi inizia il turno<br>dentro.|
|Magma<br>(Fuoco+Terra)|Aria|Dash|Propulsive Geyser|Sbalza l'occupante a Livello Z+1<br>facendolo atterrare in una casella<br>adiacente (scavalca muri).|
|Magma<br>(Fuoco+Terra)|Aria|Blast|Thermobaric<br>Bomb|Esplosione AoE devastante che<br>polverizza ogni muro e copertura nelle<br>vicinanze.|
|Magma<br>(Fuoco+Terra)|Acqua|Prep/<br>Dash/<br>Blast|Obsidian Floor /<br>Steam Repulsion|In Prep sigilla l'esagono; in Dash<br>respinge tutti entro 3 esagoni; in Blast<br>distrugge l'armatura del bersaglio.|
|Magma<br>(Fuoco+Terra)|Flux|Prep|Volcanic Extrusion|Solidifica il magma in un Muro di<br>Ossidiana alto indistruttibile (Copertura<br>Totale permanente).|
|Fango<br>(Acqua+Terra)|Fulmine|Prep/<br>Dash/<br>Blast|Pantano Ionizzato /<br>Scossa / Arco<br>Voltaico|In Prep blocca Dash in area 7 hex; in<br>Dash infligge Root + 150 danni/hex<br>residuo; in Blast scarica+450 danni<br>Fulminea catena.|

|Stato Tier 2|3°<br>Elemento|Fase<br>Chiusura|Nome Finisher (Tier<br>3)|Effetto Deterministico & Impatto<br>Tattico|
|---|---|---|---|---|
|Fango<br>(Acqua+Terra)|Disintegr.|Blast|Sinkhole (Voragine)|Il terreno collassa facendo<br>precipitare gli occupanti a Livello<br>Z−1 con pesanti Danni da Caduta.|
|Vapore<br>(Fuoco+Acqua)|Freddo /<br>Aria /<br>Fulmine|Prep/<br>Dash/<br>Blast|Hail Prison /<br>Vacuum Dispersal /<br>Boiling Arc|Col Freddo (Prep) intrappola e<br>blocca gli Scudi; con Aria (Dash)<br>spazza via e silenzia (-1 AP); col<br>Fulmine (Blast) infligge danni puri<br>AoE.|
|Tempesta Fuoco<br>(Fuoco+Aria)|Terra / Flux<br>/ Acqua|Dash/<br>Prep/<br>Blast|Pyroclastic Surge /<br>Containment /<br>Scalding|Con Terra (Dash) spinge la<br>tempesta per 3 hex; col Flux (Prep)<br>crea pilastro di fuoco inaccessibile;<br>con Acqua (Blast) fonde il 50% di<br>DEF.|
|Plasma<br>(Fuoco+Fulmine)|Disint. /<br>Scudo /<br>Freddo|Blast/<br>Reaction/<br>Dash|Antimatter / Plasma<br>Mirror / Contraction|Con Disint. (Blast) danno critico +<br>cancella l'hex; con Scudo<br>(Reaction) riflette il 100% del danno<br>a cono; col Freddo (Dash) attira i<br>vicini al centro.|
|Shock Termico<br>(Fuoco+Freddo)|Terra / Flux|Blast/<br>Prep|Brittle Shatter /<br>Entropy Loop|Con Terra (Blast) infligge danni<br>fisici tripli (3×) su bersagli a 0%<br>DEF; col Flux (Prep) aumenta di +1<br>AP il costo delle skill nemiche.|
|Nebbia<br>(Acqua+Aria)|Fulmine /<br>Fuoco|Prep/<br>Blast|Static Cloud / Flash<br>Boil|Col Fulmine (Prep) rivela e<br>stordisce per 1T le unità in Stealth;<br>col Fuoco (Blast) infligge danno<br>critico istantaneo a chi è nascosto<br>dentro.|
|Acqua Elettrif.<br>(Acqua+Fulm.)|Terra /<br>Freddo|Dash/<br>Prep|Magnetic Mud /<br>Cryo-Battery|Con Terra (Dash) trascina al centro<br>i bersagli corazzati; col Freddo<br>(Prep) immagazzina la carica<br>donando +1 AP agli alleati sopra.|
|Tempesta Sabbia<br>(Terra+Aria)|Fuoco /<br>Acqua|Blast/<br>Dash|Glass Shards /<br>Mudslide|Col Fuoco (Blast) fonde la sabbia in<br>vetro causando Bleed DoT<br>pesante; con Acqua (Dash) travolge<br>e trascina i bersagli di 2 esagoni.|
|Faglia Ionizzata<br>(Terra+Fulm.)|Scudo /<br>Disintegr.|Reaction/<br>Blast|Electromagnetic<br>Aegis / Railgun<br>Crater|Con Scudo (Reaction) assorbe tutti<br>i tiri Ranged per 1T (+1 AP/colpo);<br>con Disint. (Blast) sfonda l'Asse Z<br>creando un cratere.|
|Permafrost<br>(Terra+Freddo)|Fuoco /<br>Flux|Blast/<br>Prep|Steam Explosion /<br>Glacial Labyrinth|Col Fuoco (Blast) fa esplodere il<br>muro accecando e respingendo i<br>vicini; col Flux (Prep) genera 3 muri<br>permafrost negli esagoni adiacenti.|

|Stato Tier 2|3°<br>Elemento|Fase<br>Chiusura|Nome Finisher<br>(Tier 3)|Effetto Deterministico & Impatto<br>Tattico|
|---|---|---|---|---|
|Tempesta Ionica<br>(Aria+Fulm.)|Disintegr. /<br>Acqua|Blast/<br>Prep|Plasma<br>Detonation /<br>Conductivity<br>Trap|Con Disint. (Blast) esplosione al<br>plasma che acceca chi guarda; con<br>Acqua (Prep) espande l'effetto di<br>Silencea un'area 3×3.|
|Bufera<br>(Aria+Freddo)|Fuoco /<br>Terra|Dash/<br>Blast|Thermal Updraft /<br>Avalanche|Col Fuoco (Dash) corrente<br>ascensionale che solleva le unità a<br>Z+2; con Terra (Blast) danno da<br>schiacciamento scalato sugli stack di<br>Freddo.|
|Superconduttore<br>(Fulm.+Freddo)|Terra / Flux|Blast/<br>Dash|Magnetic<br>Implosion /<br>Super-rail|Con Terra (Blast) attira tutti entro 2<br>esagoni al centro schiacciandoli; col<br>Flux (Dash) gli alleati scattano sugli<br>esagoni pagando 0 AP.|

## 5. Roster degli 8 Eroi Core: Statistiche, Skill Kit, Sinergie e Matchup 

### 5.1 Struttura del Loadout Fisso e Archetipi di Reaction 

Ogni eroe scende in campo con un Loadout Fisso incentrato sulle proprie Skill di Base (1 per Fase: Prep, Dash, Blast), integrate dalle opzioni Ultimate e Reaction (il numero definitivo di slot attivi simultaneamente in partita verrà fissato in fase di playtest/UI). Le Reaction si dividono in 4 archetipi funzionali: Overwatch (intercettazione movimento in Dash Phase), Guard (mitigazione o protezione alleato in Blast Phase), Counter (contrattacco istantaneo) ed Evasion (riposizionamento di 1 esagono o spicchio prima del calcolo del danno). 

|Eroe|Ruolo<br>Tattico|HP<br>Max|DEF Fisica<br>(% Rid.)|RES<br>Energ. (%<br>Rid.)|Move<br>Base|Gittata<br>Base|Affinità Elementali (9<br>Elementi)|
|---|---|---|---|---|---|---|---|
|1. Riktor|Tank /<br>Disruptor|1750|45 (~31%)|35 (~26%)|3 hex|1–2 hex|Fulmine / Terra /<br>Disintegrazione /<br>Scudo|
|2. Steel|Tank /<br>Fortezza|1900|60 (~37%)|30 (~23%)|2 hex|1 hex|Scudo / Terra / Fulmine<br>/ Disintegrazione|
|3. Kallari|Assassin /<br>Flanker|950|15 (~13%)|20 (~17%)|4 hex|1 hex|Freddo / Aria / Acqua /<br>Disintegrazione|
|4. Feng<br>Mao|Bruiser /<br>Melee|1350|35 (~26%)|25 (~20%)|3 hex|1–2 hex|Fuoco / Terra / Scudo /<br>Disintegrazione|
|5.<br>Murdock|Artiglieria<br>Ranged|1050|20 (~17%)|15 (~13%)|3 hex|8 hex|Fuoco / Fulmine / Aria /<br>Disintegrazione|
|6. Gadget|Mage /<br>Spazio Flux|1000|15 (~13%)|35 (~26%)|3 hex|6 hex|Flux / Fulmine /<br>Disintegrazione|
|7. Muriel|Support /<br>Scudi|1100|25 (~20%)|40 (~28%)|3 hex|5 hex|Scudo / Freddo / Aria|

|Eroe|Ruolo|HP|DEF Fisica|RES Energ.|Move|Gittata|Affinità Elementali|
|---|---|---|---|---|---|---|---|
||Tattico|Max|(% Rid.)|(% Rid.)|Base|Base|(9 Elementi)|
|8. The|Support /|1150|20 (~17%)|30 (~23%)|3 hex|5 hex|Acqua / Terra /|
|Fey|Zoner||||||Freddo / Scudo|

### 5.2 Schede Analitiche degli 8 Eroi 

#### 1. RIKTOR — Tank / Disruptor e Combo-Breaker (1750 HP | 45 DEF | 35 RES) 

Controllore spaziale che punisce gli errori di posizionamento trascinando i nemici lungo le 6 direttrici dentro esagoni già saturati di elementi. In Overclock privilegia il sacrificio della propria Dash Phase futura per garantire la presa immediata. Sinergie: Murdock, Gadget e The Fey (trascina i bersagli nel Fango o sulla linea dei laser). Forte contro: Kallari, Murdock, Muriel. Debole contro: Steel (che blocca le linee di trazione del gancio). 

|Abilità|Tipo / Slot|Fase|Elemento|AP /<br>CD|Effetto Deterministico & Valori|
|---|---|---|---|---|---|
|Electrocute|Base|Prep|Fulmine|1 AP<br>/ CD<br>1|Elettrifica l'esagono per 2T (80 Danno<br>Energetico/turno agli adiacenti) + danni di<br>ritorno su colpi melee.|
|Chain Pull|Base|Dash|Terra|2 AP<br>/ CD<br>2|Aggancia un bersaglio entro 2–4 esagoni<br>trascinandolo nell'esagono adiacente a Riktor<br>(140 Danno Fisico).|
|Shock Whip|Base (0-<br>CD)|Blast|Fulmine|2 AP<br>/ CD<br>0|Frustata da 160 Danno Energetico (raggio 1–<br>2 hex); incatena il danno ai nemici vicini se il<br>bersaglio è su Acqua/Fango/Vapore.|
|Corrosive<br>Shackles|Variante<br>Blast|Blast|Disintegr.|2 AP<br>/ CD<br>2|Infligge 200 Danno Perforante (raggio 2 hex)<br>e blocca tutte le cure ricevute dal bersaglio<br>per 2 turni.|
|Skewer|Ultimate 1|Blast|Disintegr.|3 AP<br>/ CD<br>3|Esplosione di catene a cono frontale da 380<br>Danni che ignora l'armatura fisica e applica<br>Stunper 1 turno.|
|Iron Maiden|Ultimate 2|Prep|Terra|3 AP<br>/ CD<br>3|Ingabbia un nemico entro 3 hex bloccando<br>movimento, LoS, cure e scudi; sulFango<br>sprofonda azzerando la DEF.|
|Shock Armor|Reaction<br>(Counter)|Reaction|Fulmine|1 AP<br>/ CD<br>1|Se colpito in mischia, applicaFulmine<br>sull'esagono dell'attaccante e annulla<br>qualsiasi sbalzo (Knockback).|
|Magnetic|Reaction|Reaction|Terra /|1–2|Intercetta un colpo destinato a un alleato|
|Bastion|(Guard)||Scudo|AP /<br>CD 2|adiacente (fino a 300 danni) e fornisce +1 AP<br>a Riktor nel turno seguente.|

#### 2. STEEL — Tank / Fortezza Difensiva (1900 HP | 60 DEF | 30 RES) 

Bastione inamovibile che ridisegna le coperture sul campo e controlla gli sbalzi cinetici. In Overclock usa spesso Bull Rush + Force Shield nello stesso turno per salvare un carry alleato, sacrificando la propria Blast Phase successiva. Sinergie: Murdock (lo protegge con Force Shield e prepara Terra per il Magma). Forte contro: Riktor, Feng Mao, Kallari. Debole contro: Gadget (che smaterializza le sue barriere col Flux) e i finisher Lethal Shatter (che ignorano i suoi 60 punti di DEF). 

|Abilità|Tipo / Slot|Fase|Elemento|AP /<br>CD|Effetto Deterministico & Valori|
|---|---|---|---|---|---|
|Force Shield|Base|Prep|Scudo|1 AP /<br>CD 1|Erige una barriera olografica da 200 HP (50%<br>DR) sul lato dell'esagono che blocca il 100%<br>dei proiettili fisici.|
|Bull Rush|Base|Dash|Terra|2 AP /<br>CD 2|Carica lineare di 3 esagoni (180 Danno<br>Fisico) che sbalza indietro i nemici colpiti di 1<br>esagono.|
|Shield Bash|Base (0-<br>CD)|Blast|Scudo|2 AP /<br>CD 0|Colpo ravvicinato da 150 Danno Fisico che<br>riduce del 50% il danno in uscita del<br>bersaglio nel turno successivo.|
|Ground<br>Shock|Variante<br>Blast|Blast|Fulmine|2 AP /<br>CD 2|Onda d'urto a cono frontale da 220 Danno<br>Fisico/Energetico.|
|Shield Slam|Ultimate 1|Blast|Terra|3 AP /<br>CD 3|Salta e schianta al suolo in un'area 3×3 (320<br>Danno Fisico), applicandoTerra, sbalzo Z+1<br>eStunper 1 turno.|
|Kinetic<br>Earthquake|Ultimate 2|Blast|Disintegr.|3 AP /<br>CD 3|Colpisce il suolo in un cono frontale<br>polverizzando tutte le coperture basse e i<br>detriti.|
|Ablative<br>Plating|Reaction<br>(Guard)|Reaction|Terra|1–2<br>AP /<br>CD 2|Riduce del 50% il danno fisico subito (o erige<br>scudo da 250 HP su alleato) e rende<br>l'esagonoTerreno Sconnesso.|
|Kinetic<br>Rebound|Reaction<br>(Counter)|Reaction|Terra|1 AP /<br>CD 1|Subendo danno fisico, rilascia un'onda d'urto<br>a 360° che respinge tutti i nemici adiacenti di<br>1 esagono.|

#### 3. KALLARI — Assassin / Furtività e Verticalità (950 HP | 15 DEF | 20 RES) 

Flanker letale che sfrutta l'Asse Z, il Passo Furtivo e l'invisibilità termica per colpire sistematicamente nei 4 spicchi posteriori (Flanking +50% danno). Sinergie: Feng Mao e Steel (Kallari congela un esagono da sola con Shadow Walk [Freddo] + Mist Slash [Acqua] = Ghiaccio, preparando lo Shatter). Forte contro: Murdock e Gadget. Debole contro: Riktor (le aure elettriche e gli Stun ad area puniscono i suoi 950 HP). 

|Abilità|Tipo / Slot|Fase|Elemento|AP /<br>CD|Effetto Deterministico & Valori|
|---|---|---|---|---|---|
|Shadow<br>Walk|Base|Prep|Freddo|1–2<br>AP /<br>CD 2|Abbassa la traccia termica entrando in Stealth<br>per 2T; ignoraOverwatch, non attiva trappole e<br>dà +30% al 1° colpo.|
|Double<br>Jump|Base|Dash|Aria|1 AP /<br>CD 1|Salta di 2 esagoni scavalcando muri e ostacoli<br>Z-Axis, atterrando alle spalle del bersaglio per<br>ilFlanking.|
|Mist Slash|Base (0-<br>CD)|Blast|Acqua|2 AP /<br>CD 0|Fendente melee da 210 Danno Fisico che<br>applicaAcquasull'esagono e riduce di 3<br>esagoni la LoS nemica.|
|Death Toll|Ultimate 1|Blast|Disintegr.|3 AP /<br>CD 2|Esecuzione verticale da 450 Danno<br>Perforante; triplica il danno (o ignora il 100%<br>DEF) su bersagli a 0 DEF o in Flanking.|
|Phantom<br>Strike|Ultimate 2|Dash|Freddo|3 AP /<br>CD 3|Scatto rettilineo di 5 esagoni che trapassa tutti<br>i modelli sulla linea, applicandoFreddoe danni<br>critici.|
|Acrobatic<br>Evasion|Reaction<br>(Evasion)|Reaction|Aria|2 AP /<br>CD 2|Se bersagliata da un AoE in Blast Phase, salta<br>in un esagono vuoto adiacente prima del<br>calcolo del danno, schivandolo.|
|Vengeful<br>Shade|Reaction<br>(Counter)|Reaction|Disintegr.|1 AP /<br>CD 1|Se un alleato nella sua LoS subisce danno,<br>scaglia istantaneamente un pugnale perforante<br>contro l'attaccante.|

#### 4. FENG MAO — Bruiser / Mischia Tattica (1350 HP | 35 DEF | 25 RES) 

Duellante aggressivo capace di convertire la difesa in economia d'azione (+1 AP su rottura scudo) e spazzare le coperture nemiche con fendenti termici e sismici. Sinergie: The Fey e Muriel (detona il Ghiaccio con Seismic Slice o si auto-innesca il Magma combinando Reaping Dash + Seismic Slice). Forte contro: Murdock e assassini melee. Debole contro: The Fey (il Fango blocca il suo Reaping Dash). 

|Abilità|Tipo /<br>Slot|Fase|Elemento|AP /<br>CD|Effetto Deterministico & Valori|
|---|---|---|---|---|---|
|Conversion<br>Shield|Base|Prep|Scudo|1 AP /<br>CD 1|Scudo personale da 220 HP per 2 turni; se viene<br>infranto nel turno corrente, rimborsa+1 APa Feng<br>Mao.|
|Reaping<br>Dash|Base|Dash|Fuoco|1–2<br>AP /<br>CD 2|Scatto propulsivo di 3 esagoni che applicaFuoco<br>all'atterraggio e lascia una scia infuocata (90<br>Danno/turno per 2T).|
|Seismic Slice|Base (0-<br>CD)|Blast|Terra|2 AP /<br>CD 0|Fendente pesante da 260 Danno Fisico che<br>colpisce tutti i bersagli in un cono frontale stretto di<br>60° (o linea di 3 hex).|
|Earth Shatter|Ultimate<br>1|Blast|Disintegr.|3 AP /<br>CD 3|Schianto rettilineo (5 hex) da 480 Danno<br>Energetico perforante che polverizza le coperture<br>leggere sul percorso.|

|Abilità|Tipo / Slot|Fase|Elemento|AP /<br>CD|Effetto Deterministico & Valori|
|---|---|---|---|---|---|
|Dragon's<br>Breath|Ultimate 2|Blast|Fuoco|3 AP /<br>CD 3|Spazzata frontale di 180° (profonda 2<br>esagoni) che applicaFuocoe distrugge<br>l'armatura fisica dei bersagli.|
|Parry &<br>Riposte|Reaction<br>(Counter)|Reaction|Scudo|1–2<br>AP /<br>CD 2|Devia un attacco melee frontale, risponde con<br>200 danni e applicaDazed(disabilita le<br>Reaction) all'attaccante.|
|Smoldering<br>Step|Reaction<br>(Evasion)|Reaction|Fuoco|1 AP /<br>CD 1|Se mirato da un attacco Ranged, scatta di 1<br>esagono lateralmente schivando il colpo e<br>lasciandoFuocosulla cella d'origine.|

#### 5. MURDOCK — Artiglieria Ranged / Cecchino (1050 HP | 20 DEF | 15 RES) 

Zoner a lunghissima gittata (8 esagoni) che controlla i corridoi di tiro, crea campi di Magma/Vapore a distanza e giustizia i bersagli attraverso i muri. Sinergie: Gadget (Structure Shift apre linee di tiro impreviste attraverso i muri solidi) e Riktor. Forte contro: Steel e The Fey. Debole contro: Kallari (che aggira il suo Overwatch col Passo Furtivo e lo colpisce alle spalle dove Riot Shield non protegge). 

|Abilità|Tipo / Slot|Fase|Elemento|AP /<br>CD|Effetto Deterministico & Valori|
|---|---|---|---|---|---|
|Static Trap|Base|Prep|Fulmine|1 AP /<br>CD 1|Piazza una mina su un esagono entro<br>raggio 2 (120 Danni Energetici +Root/Stun<br>istantaneo di 1 turno a chi vi entra).|
|Tactical Retreat|Base|Dash|Aria|1–2<br>AP /<br>CD 1|Spara al suolo per sbalzarsi indietro di 2<br>esagoni mantenendo ilFacingfisso in<br>avanti e respingendo i nemici adiacenti.|
|Incendiary<br>Shell|Base (0-<br>CD)|Blast|Fuoco|2 AP /<br>CD 0|Proiettile esplosivo AoE da 220–240 Danni<br>Fuoco (esagono bersaglio + 3 spicchi<br>contigui; raggio 6–8 hex).|
|Devastating<br>Blast|Ultimate 1|Blast|Disintegr.|3 AP /<br>CD 3|Raggio laser globale da 520 Danno<br>Energetico che trapassa tutti i muri sulla<br>mappa (traiettoria visibile in Prep Phase).|
|Orbital Laser|Ultimate 2|Prep|Fuoco|3 AP /<br>CD 3|Marca un'area 2×2; alla fine dellaBlast<br>Phase del turno successivoinfligge danni<br>termici apocalittici incurabili.|
|Riot Shield|Reaction<br>(Guard)|Reaction|Scudo|1 AP /<br>CD 1|Schiera uno scudo fisico sui 3 spicchi<br>frontali (90°) che assorbe 1 proiettile o<br>attacco melee frontale.|
|Smokescreen<br>Retreat|Reaction<br>(Evasion)|Reaction|Aria|1 AP /<br>CD 1|Se attaccato in mischia, salta indietro di 1<br>esagono e rilasciaAria(acceca chi vi entra)<br>sulla cella d'origine.|

|Abilità|Tipo / Slot|Fase|Elemento|AP /<br>CD|Effetto Deterministico & Valori|
|---|---|---|---|---|---|
|Sniper|Reaction|Reaction|Disintegr.|2 AP|Sorveglia un cono lungo 6 esagoni; spara|
|Overwatch|(Overwatch)|||/ CD<br>2|un colpo da 250 danni al primo nemico che<br>vi si muove inDash Phase.|

#### 6. GADGET — Mage / Manipolatrice Spaziale del Flux (1000 HP | 15 DEF | 35 RES) 

La regina del Refactoring ambientale: altera la topografia della mappa rendendo i muri trasparenti, drena gli AP nemici e controlla i Refactor Nodes con cupole elettriche e bombe a tempo. Sinergie: Murdock e Steel (con Flux su Magma innalza muri permanenti di Ossidiana; con Structure Shift apre linee di tiro per Murdock). Forte contro: Steel e formazioni chiuse. Debole contro: Kallari e Feng Mao. 

|Abilità|Tipo / Slot|Fase|Elemento|AP /<br>CD|Effetto Deterministico & Valori|
|---|---|---|---|---|---|
|Structure<br>Shift|Base|Prep|Flux|1 AP<br>/ CD<br>1|Rende un muro olografico (Wireframe) per 1<br>turno consentendo a LoS e proiettili di passare,<br>o attiva i 12 muri Flux.|
|Seeker Bot|Base|Dash|Fulmine|2 AP<br>/ CD<br>2|Rilascia un drone che avanza di 3 esagoni e<br>fulmina il primo nemico sulla linea (150 Danno +<br>drena 1 AP nel turno successivo).|
|Spark Bolt|Base (0-<br>CD)|Blast|Fulmine|2 AP<br>/ CD<br>0|Dardo elettrico a lungo raggio (6 hex) da 170<br>Danno Energetico; innesca statiPlasmao<br>Acqua Elettrificata.|
|Sticky<br>Bomb|Ultimate 1|Blast|Disintegr.|3 AP<br>/ CD<br>2|Piazza una bomba a scoppio ritardato su un<br>esagono (raggio 5 hex) che esplode a fine<br>fase/turno per 420 Danni perforanti AoE.|
|Tesla<br>Dome|Ultimate 2|Prep|Fulmine|3 AP<br>/ CD<br>3|Cupola statica (raggio 3 esagoni); chiunque<br>scatti o spari attraverso il bordo subisce gravi<br>danni elettrici e perde 1 AP.|
|Flux<br>Deflector|Reaction<br>(Evasion)|Reaction|Flux|1 AP<br>/ CD<br>1|Quando è bersaglio di un proiettile Ranged,<br>distorce lo spazio deviando la traiettoria di 60°<br>su uno spicchio adiacente.|
|Feedback<br>Loop|Reaction<br>(Counter)|Reaction|Fulmine|1 AP<br>/ CD<br>1|Se colpita in mischia, sbalza l'attaccante indietro<br>di 1 esagono e applicaFulminesulla sua cella.|

#### 7. MURIEL — Support / Scudi, Stasi e Manipolazione AP (1100 HP | 25 DEF | 40 RES) 

Supporto predittivo che legge in anticipo la Blast Phase avversaria, salvando i compagni con scudi globali, sollevamenti sull'Asse Z e campi di accelerazione AP privi di debito. Sinergie: The Fey (Frost Binding + Nettle Shot congela istantaneamente i nemici nel Ghiaccio) e Kallari (Alacrity Field le fornisce +2 AP senza Phase Lock). Forte contro: Riktor e Steel (Consecrated Ground rende immuni a Pull e Knockback). Debole contro: Detonazioni sul Vapore e blocchi di cura/scudo. 

|Abilità|Tipo / Slot|Fase|Elemento|AP /<br>CD|Effetto Deterministico & Valori|
|---|---|---|---|---|---|
|Consecrated<br>Ground|Base|Prep|Scudo|2 AP /<br>CD 2|Crea una cupola 3×3 (250 HP scudo) i cui<br>occupanti sono immuni a sbalzi (Knockback)<br>e trascinamenti forzati (Pull).|
|Reversal of<br>Fortune|Base / Ult|Dash|Scudo|2–3<br>AP /<br>CD 2|Vola globalmente verso un alleato ovunque<br>sulla mappa, atterrando con un'onda d'urto<br>protettiva e scudi pesanti (450 HP).|
|Frost Binding|Base (0-<br>CD)|Blast|Freddo|2 AP /<br>CD 0|Sfera glaciale (raggio 5 hex, 160 Danno) che<br>applicaFreddoe aumenta di +1 AP il costo<br>dei movimenti del bersaglio.|
|Alacrity Field|Ultimate 2|Prep|Aria|3 AP /<br>CD 3|Infonde un'area 3×3; gli alleati che iniziano il<br>turno successivo al suo interno ricevono+2<br>AP gratuiti senza debito Overclock.|
|Aegis|Reaction<br>(Guard)|Reaction|Scudo|1–2<br>AP /<br>CD 2|Guardia attiva che trasferisce i propri HP o<br>erige uno scudo istantaneo da 220 HP per<br>schermare un alleato inBlast Phase.|
|Harmonic<br>Resonance|Reaction<br>(Guard)|Reaction|Scudo|1 AP /<br>CD 1|Se un alleato entro 3 esagoni perde più del<br>30% degli HP, gli trasferisce<br>istantaneamente uno scudo rigenerante<br>d'emergenza.|
|Angelic<br>Ascent|Reaction<br>(Evasion)|Reaction|Aria|1 AP /<br>CD 1|Se bersagliata da un AoE a terra, solleva sé<br>stessa e 1 alleato adiacente a Livello Z+2<br>(Airborne), evitando i danni terrestri.|

#### 8. THE FEY — Support / Zoner Ambientale e Controllo AP (1150 HP | 20 DEF | 30 RES) 

Alchimista del terreno che inonda gli esagoni di Acqua, Fango e Ghiaccio per strangolare l'economia AP e bloccare la Dash Phase avversaria. Sinergie: Riktor e Gadget (crea Fango da sola con Untamed Growth + Bramble Patch, preparando l'Arco Voltaico col Fulmine). Forte contro: Feng Mao e Murdock. Debole contro: Kallari (che scavalca il fango e i rovi sull'Asse Z con Double Jump). 

|Abilità|Tipo /<br>Slot|Fase|Elemento|AP /<br>CD|Effetto Deterministico & Valori|
|---|---|---|---|---|---|
|Untamed<br>Growth|Base|Prep|Acqua|2 AP /<br>CD 2|Pianta un bocciolo esplosivo che genera un'area 3×3<br>diAcquaed esplode a fine turno (180 Danno).|
|Bramble<br>Patch|Base|Dash|Terra|2 AP /<br>CD 2|Crea un muro di rovi lungo 4 esagoni; attraversarlo<br>fisicamente costa +2 AP extra ai nemici (con Acqua<br>formaFango).|
|Nettle Shot|Base (0-<br>CD)|Blast|Acqua|1 AP /<br>CD 0|Dardo perforante (150 Danno, raggio 5 hex); se<br>colpisce un nemico nelFango(Acqua+Terra),<br>rimborsa+1 APa The Fey.|
|Flytrap|Ultimate<br>1|Blast|Freddo|3 AP /<br>CD 3|Pianta carnivora gigante che trascina tutti i nemici<br>entro 3 esagoni al centro dell'area (280 Danno) e<br>applicaRooteFreddo.|

|Abilità|Tipo / Slot|Fase|Elemento|AP /<br>CD|Effetto Deterministico & Valori|
|---|---|---|---|---|---|
|Nature's<br>Vengeance|Ultimate 2|Prep|Scudo|3 AP<br>/ CD<br>3|Rianima un alleato caduto al 25% degli HP<br>sull'esagono bersaglio in un bozzolo<br>protettivo, ma l'alleato subiscePhase Lock.|
|Thorned<br>Carapace|Reaction<br>(Counter)|Reaction|Acqua|1 AP<br>/ CD<br>1|Reagisce agli attacchi Ranged accecando<br>(Blind/ no LoS) il tiratore per il resto del turno,<br>annullando i suoi colpi successivi.|
|Bramble<br>Shield|Reaction<br>(Guard)|Reaction|Terra|1 AP<br>/ CD<br>1|Si radica sul posto diventando immune a ogni<br>spostamento forzato e ottenendo il 50% di<br>Riduzione Danno inBlast Phase.|

## 6. Direzione Artistica, UI/UX e Grammatica Visiva 

Rendering Anime Cel-Shading & Cut-In Diagonali: I modelli 3D di Paragon sono renderizzati tramite un Post-Process Material in Cel-Shading morbido con contorni neri marcati (outlines). Quando viene chiuso un Finisher Tier 3, attivato un Overclock o innescata una Reaction, l'azione va in freeze-frame per 0,5 secondi mostrando un Cut-In diagonale in stile manga accompagnato da onomatopee 2D/3D dinamiche sugli esagoni: "FLANKED!", "INTERCEPT!", "REFACTORED! / 再構築!", "OVERCLOCK READY! / 限界突破!". 

Materiali Procedurali sugli Esagoni: Gli Stati Composti Tier 2 modificano fisicamente la texture 3D della cella (fango denso, lastra di ghiaccio, magma incandescente), eliminando la necessità di leggere etichette testuali durante il turno. 

HUD dei 45 Secondi (Timeline a 8 Tracce): La parte superiore dello schermo ospita il MultiTrack Resolver diviso nelle 4 colonne temporali (Prep, Dash, Blast, Reaction/DoT) con 8 righe sincronizzate per visualizzare l'incastro delle fasi ed eventuali Phase Lock sigillati da strisce di pericolo. Sulla mappa compaiono i Color-Coded Echoes (ombre e nastri vettoriali delle mosse pianificate dai propri alleati in tempo reale) e, passando il cursore sui nemici visibili, la Bussola Under-Foot a 12 Spicchi mostra il loro Facing, i 4 spicchi posteriori di Flanking e il Threat Cone massimo. 

|Categoria<br>Grammatica|Concetto /<br>Elemento|Rappresentazione Visiva sull'HUD|Significato Comunicativo|
|---|---|---|---|
|Contenitore<br>Fase (Quando)|Prep / Dash /<br>Blast /<br>Reaction|Cerchio Blu / Chevron Giallo (>>) / Rombo<br>Rosso / Esagono Viola|La forma esterna dell'icona<br>comunica istantaneamente in<br>quale fase temporale si<br>risolverà l'abilità.|
|Glifo Interno<br>(Cosa)|6 Elementi<br>Base (Tier 1)|Fuoco (Fiamma Rosso/Arancio), Acqua (3<br>Gocce Blu), Terra (Montagne Ocra), Aria<br>(Vento Azzurro), Fulmine (Saetta Giallo<br>Neon), Freddo (Cristallo Ciano Ghiaccio)|Colori primari ad alto contrasto<br>per la lettura periferica<br>immediata degli elementi sugli<br>esagoni.|
|Glifo Interno<br>(Catalizz.)|Scudo /<br>Disintegr. /<br>Flux|Scudo (Nido d'ape Ciano), Disintegrazione<br>(Sfera infranta Viola Scuro), Flux (Tesseratto<br>Glitch Magenta)|Identifica i 3 catalizzatori di<br>barriera, perforazione<br>armatura o riscrittura<br>geometrica della mappa.|

|Categoria<br>Grammatica|Concetto /<br>Elemento|Rappresentazione Visiva<br>sull'HUD|Significato Comunicativo|
|---|---|---|---|
|Sintassi<br>Olografica|Syntax Ring<br>(Anello a 3<br>Segmenti)|Anello olografico diviso in 3 settori<br>sotto l'esagono bersaglio + icona<br>di Fase al centro in Lock-On|Mostra gli elementi presenti sulla cella;<br>puntando il 3° elemento fa<br>lampeggiare al centro la forma della<br>Fase del Finisher.|
|Regola<br>Modificatore|Wireframe<br>Rule (Flux)|L'intera interfaccia dell'esagono o<br>del muro passa in griglia<br>Wireframe Magenta|Avvisa immediatamente che la combo<br>riscriverà la geometria della mappa<br>(muri/coperture) invece di infliggere<br>solo danni HP.|

## 7. Architettura dell'Intelligenza Artificiale (Bot Singoli & Swarm AI di Squadra) 

Poiché Refactor Tactics è un gioco a turni simultanei (Simultaneous Move Game — SMG) privo di RNG, lo stato della scacchiera muta mentre tutti gli 8 eroi agiscono insieme. L'architettura AI combina 3 livelli decisionali: 

1. Utility-Based AI (Valutazione Geometrica e Alchemica): Ogni bot valuta il campo tramite curve di utilità dedicate che pesano: HP residui del bersaglio, esposizione dei 4 spicchi posteriori (Flanking), differenza di quota sull'Asse Z (il peso di uno sbalzo cinetico sale esponenzialmente se il nemico è a Livello 2 sul tetto, garantendo danni da caduta non mitigabili) e cariche elementali già depositate sugli esagoni (Risonanza Ambientale). 

2. Monte Carlo Tree Search Adattato (Decoupled UCT / Regret Matching): Durante i 45 secondi, il bot simula centinaia di incroci tra le proprie sequenze di 6 AP (o 8 AP in Overclock) e le mosse statisticamente più probabili degli avversari. L'MCTS calcola il rischio di Combo-Breaking temporale: valuta se depositare un elemento in Prep Phase rischi di far scappare il nemico in Dash Phase, o se convenga chiudere una detonazione garantita su un bersaglio che ha la Dash Phase bloccata da un Phase Lock. 

3. Logica di Overclock per Archetipo Bot: Il bot attiva l'Overclock (+2 AP) solo quando la simulazione MCTS conferma che il guadagno nel turno corrente (K.O. confermato o salvataggio di un alleato) supera il costo del Phase Lock futuro. I bot Disruptor (Riktor) sacrificano la propria Dash Phase futura pur di eseguire Chain Pull + Electrocute + Skewer; i bot Fortezza (Steel) sacrificano la Blast Phase futura per eseguire Bull Rush + Force Shield e proteggere un carry; i bot Assassin (Kallari) prediligono il modulo Sovraccarico Biologico (-35% HP) per non perdere mai la mobilità nel turno seguente. 

### Protocollo "Blackboard" (Lavagna Condivisa per lo Swarm AI di Squadra) 

Per evitare che i 4 bot di una squadra giochino in modo scollegato o sprechino AP sullo stesso bersaglio, il team condivide una Blackboard in memoria aggiornata in tempo reale durante la finestra di pianificazione: 

|Ordine<br>Logico|Agente<br>Bot|Hex<br>Bersaglio|Fase<br>Prenotata|Elemento /<br>Azione|Blackboard Intent & Reazione a<br>Catena del Team|
|---|---|---|---|---|---|
|Step 1<br>(Setup<br>LoS)|1.<br>Gadget<br>Bot|Muro Hex<br>E-04|Prep<br>Phase (0)|Flux (Structure<br>Shift)|PubblicaIntent:<br>Open_LoS_Wall_E04<br>smaterializzando il muro per 1 turno.|
|Step 2<br>(Innesco<br>T1)|2. The<br>Fey Bot|Hex E-04|Prep<br>Phase (0)|Acqua<br>(Untamed<br>Growth)|PubblicaIntent:<br>Apply_Water_E04pre-caricando la<br>cella in fase di preparazione.|
|Step 3<br>(Fusione<br>T2)|3. Riktor<br>Bot|Nemico→<br>E-04|Dash<br>Phase (1)|Terra (Chain<br>Pull)|Legge la Blackboard; tira il nemico in<br>E-04 creandoFango (Acqua+Terra)<br>che blocca la Dash nemica.|
|Step 4|4.|Hex E-04|Blast|Disintegr.|Legge il bersaglio bloccato nel Fango|
|(Detonaz.<br>T3)|Murdock<br>Bot||Phase (2)|(Devastating<br>Blast)|e la LoS aperta da Gadget; spara in<br>Blast Phase chiudendo il Finisher T3.|
