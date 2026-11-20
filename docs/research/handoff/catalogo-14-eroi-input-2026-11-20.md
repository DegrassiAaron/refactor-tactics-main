# Refactor Tactics — Catalogo master personaggi e skill (input ricevuto il 2026-11-20)

> `RESEARCH` · **Materiale NON autorevole.** Due file consegnati dall'autore il 2026-11-20 nella cartella `docs/characters/` del clone
> principale, con la richiesta *«abbiamo inserito in @D:\Repositories\refactor-tactics-main\docs\characters\refactor_tactics_catalogo_master_personaggi_e_skill_14_eroi.md le skill prototipo dei personaggi e in Refactor_Tactics_14_Heroes_Skills.csv (stessa directory) le skill in formato tabellare. dobbiamo integrarle e modificare le skill attuali usando queste. Nella lista manca LinearDischarge di Gadget, da mantenere.»*
> Alla domanda su cosa servisse a lavoro finito l'autore ha risposto *«servono come base questi kit per scegliere e bilanciare le skill. crea una issue per eroe»*: il catalogo è **input di design**, la base da cui ogni eroe sceglie e bilancia il proprio kit, non un kit in vigore.
>
> I due file sono conservati con il contenuto invariato. Il Markdown dell'autore (`refactor_tactics_catalogo_master_personaggi_e_skill_14_eroi.md`) segue questo banner; la tabella (`Refactor_Tactics_14_Heroes_Skills.csv`) è accanto a questo file: [`catalogo-14-eroi-input-2026-11-20.csv`](catalogo-14-eroi-input-2026-11-20.csv).
>
> ⚠️ **Il documento si dichiara «Source of Truth» e non governa.** Le regole in vigore per questo lavoro sono quelle della voce
> [D-633](../../decisions/RT_PDR_00_Decision_Log.md); dove il testo diverge da quella voce vince la voce (per esempio descrive quattro fasi, mentre le fasi
> sono cinque: Prep, Dash, Blast, Move, Cleanup). L'autorità resta quella di [`docs/README.md`](../../README.md)
> §«Gerarchia delle fonti»: Decision Log, ADR, spec owner, cataloghi. Questo file non risolve alcun conflitto.
>
> 🔎 **Il `.md` e il `.csv` portano gli stessi nomi di skill per ogni eroe tranne Riktor**, dove 9 voci su 18 hanno nome diverso.
> Solo nel `.csv`: Static Sweep, Taser Strike, Necrotic Anchor, Magnetic Spikes, Voltage Leap, Riot Charge, Seismic Smash, Arc Discharge,
> Tectonic Upheaval. Solo nel `.md`: Iron Lash, Decay Chain, Grounding Stake, Faraday Coil, Surge Stride, Warden Grapple, Arc Whip,
> Chain Slam, Faraday Cage. Otto dei nove nomi del `.csv` sono quelli del catalogo di Riktor del 2026-11-13
> ([traduzione](../../roadmap/plans/riktor-kit-traduzione-2026-11-13.md)); il nono, Tectonic Upheaval, no. **Per Riktor vale la versione del `.md`**:
> l'autore lo ha deciso nella sessione del 2026-11-20 ([D-633](../../decisions/RT_PDR_00_Decision_Log.md)).
> Le differenze di sola forma non contano: `Death Toll` (`.csv`) contro `Death Toll / Death Sentence` (`.md`) per Kallari, `Seismic Slice (Base)` contro
> `Seismic Slice` per Feng Mao.

---

# Refactor Tactics — Catalogo Master Personaggi e Skill (14 Eroi)

Questo documento funge da **Source of Truth** completa per il roster di **Refactor Tactics**, integrando le statistiche base, il sistema a 9 Elementi Ufficiali, il catalogo completo delle **252 abilità e varianti** ($14 \text{ eroi} \times 6 \text{ slot} \times 3 \text{ opzioni}$), le matrici alchemiche personali e di squadra, le sinergie e i matchup strategici.

---

## 1. Regole Fondamentali per Eroi e Skill

### 1.1 Determinismo, AP e Fasi Temporali
* **Zero RNG:** Ogni abilità produce risultati matematici deterministici al 100% (*Hit Rate = 100%* in Linea di Vista).
* **Economia AP:** Budget fisso di **6 AP** per eroe per turno. Tramite **Overclock** è possibile ottenere **+2 AP** immediati (totale 8 AP) subendo un **Phase Lock** (blocco integrale di una fase a scelta nel turno successivo).
* **4 Fasi Cronologiche (e Grammatica Visiva HUD):**
  1. 🔵 **Prep Phase (Cerchio Blu — Fase 0):** Setup, barriere, trappole, stealth, buff e riscrittura della mappa via *Flux*.
  2. 🟡 **Dash Phase (Chevron Giallo `>>` — Fase 1):** Movimento macro su 6 direttrici, scatti, salti sull'Asse Z ($Z=0, 1, 2$), cariche, *Pull* e *Knockback*.
  3. 🔴 **Blast Phase (Rombo Rosso — Fase 2):** Attacchi base (0 CD), abilità offensive, detonazioni e *Ultimate*.
  4. 🟣 **Reaction / DoT Phase (Esagono Viola — Fase 3):** Risoluzione DoT e difese reattive suddivise in **Guard** (mitigazione/protezione), **Counter** (contrattacco/punizione), **Evasion** (spostamento/schivata) e **Overwatch** (intercettazione in *Dash Phase*).

### 1.2 Formula Deterministica del Danno e Flanking
$$\text{Danno Reale} = (\text{Danno Base} \times \text{Moltiplicatore}) \times \left(\frac{100}{100 + \text{DEF o RES}_{\text{eff}}}\right) \times (1 - \text{DR}_{\text{copertura}})$$

* **Flanking (Sottogriglia a 12 Micro-Spicchi da $30^\circ$):** Colpire un bersaglio nel suo cono posteriore ($90^\circ\text{--}120^\circ$, i 4 spicchi opposti al *Facing*) applica **+50% di danno fisso** e **ignora il 50% della DEF**.
* **Vantaggio di Elevazione (Asse Z):** Attaccare da un livello superiore ($Z_{\text{attaccante}} > Z_{\text{difensore}}$) ignora la Riduzione Danno (DR 35%) delle coperture basse.

### 1.3 I 9 Elementi Ufficiali (Risonanza Ambientale sugli Esagoni)
Gli elementi si accumulano sugli esagoni (fino a 3 cariche: **Tier 1** innesco $\rightarrow$ **Tier 2** stato composto $\rightarrow$ **Tier 3** detonazione definita dalla fase del 3° elemento).
* **6 Elementi Base:** 🔥 **Fuoco**, 💧 **Acqua**, 🪨 **Terra**, 💨 **Aria**, ⚡ **Fulmine**, ❄️ **Freddo**.
* **3 Catalizzatori / Modificatori:** 🛡️ **Scudo**, ⚛️ **Disintegrazione**, 🌌 **Flux** *(riscrittura geometrica e Wireframe)*.
* **Riallineamento Elementi Legacy:** *Vita* $\rightarrow$ 🛡️ Scudo; *Morte* $\rightarrow$ ⚛️ Disintegrazione; *Arcano* $\rightarrow$ 🌌 Flux; *Forza* $\rightarrow$ 🪨 Terra; *Ghiaccio* $\rightarrow$ ❄️ Freddo.

---

## 2. Tabella Sinottica del Roster Completo (14 Eroi)

| # | Eroe | Tier | Archetipo / Ruolo | HP | DEF (Rid.) | RES (Rid.) | Move | Gittata | Affinità (9 Elementi) |
| :-: | :--- | :--- | :--- | :-: | :-: | :-: | :-: | :-: | :--- |
| 1 | **Riktor** | Core | Tank / Disruptor & Combo-Breaker | 1750 | 45 ($\approx 31\%$) | 35 ($\approx 26\%$) | 3 | 1–2 | ⚡ Fulmine, 🪨 Terra, ⚛️ Disint., 🛡️ Scudo |
| 2 | **Gadget** | Core | Maga Tecnologica / Manipolatrice Spaziale | 1000 | 15 ($\approx 13\%$) | 35 ($\approx 26\%$) | 3 | 6 | 🌌 Flux, ⚡ Fulmine, ⚛️ Disint. |
| 3 | **Steel** | Core | Tank / Fortezza Difensiva | 1900 | 60 ($\approx 37\%$) | 30 ($\approx 23\%$) | 2 | 1 | 🛡️ Scudo, 🪨 Terra, ⚡ Fulmine, ⚛️ Disint. |
| 4 | **Muriel** | Core | Supporto Scudi / Riposizionamento & Stasi | 1100 | 25 ($\approx 20\%$) | 40 ($\approx 28\%$) | 3 | 5 | 🛡️ Scudo, ❄️ Freddo, 💨 Aria |
| 5 | **Kallari** | Core | Assassina / Flanker Verticale | 950 | 15 ($\approx 13\%$) | 20 ($\approx 17\%$) | 4 | 1 | ❄️ Freddo, 💨 Aria, 💧 Acqua, ⚛️ Disint. |
| 6 | **Murdock** | Core | Artiglieria Ranged / Cecchino | 1050 | 20 ($\approx 17\%$) | 15 ($\approx 13\%$) | 3 | 8 | ⚛️ Disint., 🔥 Fuoco, ⚡ Fulmine, 💨 Aria |
| 7 | **Feng Mao** | Core | Bruiser / Demolitore di Coperture | 1350 | 35 ($\approx 26\%$) | 25 ($\approx 20\%$) | 3 | 1–2 | 🔥 Fuoco, 🪨 Terra, 🛡️ Scudo, ⚛️ Disint. |
| 8 | **The Fey** | Core | Supporto Naturale / Zoner Ambientale | 1150 | 20 ($\approx 17\%$) | 30 ($\approx 23\%$) | 3 | 5 | 💧 Acqua, 🪨 Terra, ❄️ Freddo, 🛡️ Scudo |
| 9 | **Phase** | Esp. | Supporto Simbiotico / Manipolatrice Legame | 1050 | 20 ($\approx 17\%$) | 35 ($\approx 26\%$) | 3 | 5 | 🛡️ Scudo, 🌌 Flux, ❄️ Freddo |
| 10 | **Aurora** | Esp. | Controller / Dominatrice Glaciale | 1250 | 30 ($\approx 23\%$) | 25 ($\approx 20\%$) | 3 | 1 | ❄️ Freddo, 🌌 Flux, 🛡️ Scudo, 🪨 Terra |
| 11 | **Sevarog** | Esp. | Scaling Tank / Mietitore Oscuro | 1650* | 45 ($\approx 31\%$) | 35 ($\approx 26\%$) | 3 | 1 | ⚛️ Disint., 🪨 Terra, 💨 Aria |
| 12 | **Yin** | Esp. | Duellante Anti-Ranged / Dominatrice Vento | 1150 | 25 ($\approx 20\%$) | 25 ($\approx 20\%$) | 3 | 2 (4) | 💨 Aria, 🌌 Flux, 🛡️ Scudo, 🪨 Terra |
| 13 | **Crunch** | Esp. | Combo-Brawler / Macchina da Pugilato | 1450 | 40 ($\approx 29\%$) | 20 ($\approx 17\%$) | 3 | 1 | 🪨 Terra, 🔥 Fuoco, 💨 Aria, 🌌 Flux |
| 14 | **Drongo** | Esp. | Sabotatore Scavenger / Attrito & Silence | 1100 | 20 ($\approx 17\%$) | 20 ($\approx 17\%$) | 3 | 5–6 | 🔥 Fuoco, 💨 Aria, 🪨 Terra, ⚛️ Disint. |

*\*Sevarog scala permanentemente fino a 2050 HP tramite gli Stack di Anime accumulati con Siphon.*

---

## 3. Roster Core (8 Eroi Iniziali)

---

### ⛓️ 1. RIKTOR — *Tank / Disruptor & Combo-Breaker*
Controllore spaziale puro progettato per punire il micro-posizionamento nemico, trascinare i bersagli prioritari in trappole pre-caricate (Risonanza Ambientale) e spezzare le catene offensive avversarie con scariche elettriche e stordimenti.

* **Statistiche Base:** 1750 HP | DEF Fisica: 45 ($\approx 31\%$ rid.) | RES Energetica: 35 ($\approx 26\%$ rid.) | Movimento: 3 esagoni | Gittata Base: 1–2 esagoni.
* **Affinità Elementali:** ⚡ **Fulmine** (Primario), 🪨 **Terra**, ⚛️ **Disintegrazione** *(riallineamento di 💀 Morte)*, 🛡️ **Scudo**.

#### Catalogo Abilità (6 Slot $\times$ 3 Opzioni)
1. **Attacco Base (🔴 Blast Phase | 2 AP | 0 CD)**
   * **Opzione 1 (Core) — Shock Whip [⚡ Fulmine]:** Frustata elettrica a medio raggio (1–2 esagoni) da 160 Danno Energetico; se il bersaglio occupa uno stato *Tier 2* acquoso (*Fango*, *Vapore*, *Acqua Elettrificata*), incatena il danno a 1 nemico adiacente.
   * **Opzione 2 (Ancoraggio) — Iron Lash [🪨 Terra]:** Colpo di catena pesante da 150 Danno Fisico che deposita *Terra* sull'esagono bersaglio e riduce di 1 esagono la mobilità del nemico nella *Dash Phase* successiva.
   * **Opzione 3 (Lacerazione) — Decay Chain [⚛️ Disintegrazione]:** Frustata perforante da 140 Danno Energetico che applica *Disintegrazione* e ignora il 50% degli scudi temporanei.
2. **Prep Phase (🔵 Prep Phase | 1 AP | CD 1)**
   * **Opzione 1 (Core) — Electrocute [⚡ Fulmine]:** Elettrifica l'esagono occupato da Riktor per 2 turni generando un'aura da 80 Danno Energetico a fine turno sugli adiacenti e applicando *Fulmine* a chiunque lo attacchi in mischia.
   * **Opzione 2 (Sbarramento) — Grounding Stake [🪨 Terra]:** Pianta un picchetto magnetico sul proprio esagono depositando *Terra* e rendendo Riktor immune a sbalzi (*Knockback*) e *Pull* per 1 turno.
   * **Opzione 3 (Schermatura) — Faraday Coil [🛡️ Scudo]:** Attiva un campo protettivo da 150 HP per 1 turno che deposita *Scudo* sull'esagono e prepara *Scudo Polarizzato* con *Fulmine*.
3. **Dash Phase (🟡 Dash Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Chain Pull [🪨 Terra]:** Lancia la catena lungo una direttrice macro fino a 4 esagoni infliggendo 140 Danno Fisico e trascinando forzatamente il bersaglio di 2–4 esagoni fino alla cella adiacente a Riktor *(Spostamento Temporale Liv. 10: variante "Agguato Pesante" in Prep Phase, 3 AP, raggio 3)*.
   * **Opzione 2 (Avanzata Voltaica) — Surge Stride [⚡ Fulmine]:** Carica pesante di 2 esagoni che infligge 110 Danno Energetico agli adiacenti all'arrivo e deposita *Fulmine* sull'esagono di destinazione.
   * **Opzione 3 (Scambio Forzato) — Warden Grapple [🛡️ Scudo]:** Aggancia un bersaglio entro 2 esagoni scambiando parzialmente lo spazio di ingaggio (tira di 1 esagono e ruota il *Facing* nemico di $180^\circ$) ottenendo 120 HP di scudo.
4. **Abilità Speciale / Controllo (🔴 Blast Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Corrosive Shackles [⚛️ Disintegrazione / 💀 Morte]:** Catene ad alta frequenza (gittata 2 esagoni) da 200 Danno Perforante che applicano *Disintegrazione* e bloccano completamente qualsiasi cura in entrata sul bersaglio per 2 turni.
   * **Opzione 2 (Sovraccarico) — Arc Whip [⚡ Fulmine]:** Scarica ad arco su 3 micro-spicchi frontali ($90^\circ$) da 180 Danno Energetico che applica *Fulmine* e silenzia le *Reaction* difensive per 1 turno.
   * **Opzione 3 (Frantuma-Suolo) — Chain Slam [🪨 Terra]:** Sbatte le catene a terra in linea di 2 esagoni (170 Danno Fisico) trasformando le celle in *Terreno Sconnesso* e depositando *Terra*.
5. **Ultimate (3 AP | CD 3)**
   * **Ultimate 1 (Core — 🔴 Blast Phase | CD 3) — Skewer [⚛️ Disintegrazione / ⚡ Fulmine]:** Esplosione di catene dal petto in un cono frontale da 380 Danni Perforanti che ignora completamente la DEF fisica e applica **Stun** per 1 turno (priorità deterministica sui bersagli elettrificati).
   * **Ultimate 2 (Core — 🔵 Prep Phase | CD 3) — Iron Maiden [🪨 Terra]:** Intrappola un bersaglio entro 3 esagoni in una gabbia metallica (250 HP) che blocca Linea di Vista e movimento per 1 turno; se combinata con *Fango*, frantuma l'armatura del prigioniero.
   * **Ultimate 3 (Variante — 🔴 Blast Phase | CD 3) — Faraday Cage [⚡ Fulmine]:** Elettrifica un'area 3x3 attorno a Riktor (280 Danno Energetico) bloccando gli scatti in uscita e drenando -1 AP a tutti i nemici al suo interno.
6. **Reaction Phase (🟣 Reaction Phase | 1–2 AP)**
   * **Opzione 1 (Core — Counter | 1 AP) — Shock Armor [⚡ Fulmine]:** Quando colpito in mischia, rilascia una scarica che applica *Fulmine* sull'esagono dell'attaccante (90 Danno) e annulla eventuali effetti di *Knockback* su Riktor.
   * **Opzione 2 (Core — Guard | 1 AP) — Magnetic Bastion [🪨 Terra]:** Intercetta un attacco diretto a un alleato adiacente assorbendolo al suo posto e rigenerando **+1 AP** nel turno corrente.
   * **Opzione 3 (Core — Guard | 2 AP | CD 2) — Chain Intercept [🛡️ Scudo]:** Schiera una rete di catene che assorbe fino a 300 Danni diretti a un alleato (efficace anche contro snipe attraverso muri *Flux*).

#### Matrice Combo, Sinergie e Matchup
| Nome Combo | Elementi | Fase di Chiusura | Effetto Deterministico |
| :--- | :--- | :--- | :--- |
| **Faglia Ionizzata** | 🪨 Terra + ⚡ Fulmine | 🟡 Dash Phase | Trincea elettrificata che attira i proiettili e blocca le abilità di movimento *Aria*. |
| **Scudo Polarizzato** | 🛡️ Scudo + ⚡ Fulmine | 🔵 Prep Phase | Riflette danni elettrici sugli attaccanti a distanza. |
| **Cratere Railgun** | 🪨 Terra + ⚡ Fulmine + ⚛️ Disint. | 🔴 Blast Phase | Colpo verticale che sfonda l'esagono creando una fossa ($Z-1$) con danno da caduta. |
| **Arco Voltaico Catastrofico (Squadra)** | ⚡ Fulmine + 💧 Acqua + 🪨 Terra | 🔴 Blast Phase | Scarica da +450 Danni Fulmine che rimbalza sui bersagli metallici entro 2 esagoni. |
| **Cinetica d'Arresto (Squadra)** | ⚡ Fulmine + 💧 Acqua + 🪨 Terra | 🟡 Dash Phase | Radica istantaneamente il bersaglio infliggendo 150 danni per ogni esagono di movimento residuo. |

* **Sinergie Ottimali:** **The Fey** (crea *Fango* autonomamente lasciando a Riktor la chiusura *Fulmine* per *Arco Voltaico*), **Gadget** (*Chain Pull* trascina i nemici dentro *Tesla Dome* o *Sticky Bomb*) e **Murdock** (allinea i bersagli sulle *Static Trap* o sul raggio di *Devastating Blast*).
* **Matchup:** Domina contro **Kallari** (*Electrocute* e *Skewer* la puniscono senza bisogno di LoS), **Murdock** (*Chain Intercept* ferma gli snipe e *Chain Pull* lo strappa dalle coperture) e **Feng Mao** (trascinandolo fuori asse prima di *Earth Shatter*). Soffre **Muriel & Phase** (*Consecrated Ground* e *Legame Cristallino* annullano *Chain Pull*), **Steel** (*Bodyguard* intercetta il gancio) e **Drongo** (*Gag Grenade* in Prep lo silenzia prima che possa ganciare in Dash).

---

### ⚙️ 2. GADGET — *Maga Tecnologica / Manipolatrice Spaziale*
Perno del controllo territoriale e della riscrittura geometrica: sfrutta l'elemento **Flux** e la sottogriglia a 12 spicchi per trasformare muri solidi in ologrammi *Wireframe*, drenare l'economia AP nemica e catalizzare esplosioni ad area.

* **Statistiche Base:** 1000 HP | DEF Fisica: 15 ($\approx 13\%$ rid.) | RES Energetica: 35 ($\approx 26\%$ rid.) | Movimento: 3 esagoni | Gittata Base: 6 esagoni.
* **Affinità Elementali:** 🌌 **Flux** (Primario), ⚡ **Fulmine**, ⚛️ **Disintegrazione** (varianti su 💨 Aria, 🛡️ Scudo, ❄️ Freddo, 🔥 Fuoco).

#### Catalogo Abilità (6 Slot $\times$ 3 Opzioni)
1. **Attacco Base (🔴 Blast Phase | 2 AP | 0 CD)**
   * **Opzione 1 (Core) — Spark Bolt [⚡ Fulmine]:** Dardo energetico a lungo raggio (6 esagoni) da 170 Danno Energetico; agisce da catalizzatore a distanza su esagoni già caricati.
   * **Opzione 2 (Controllo Facing) — Phase Dart [🌌 Flux]:** Proiettile di distorsione da 130 Danno Energetico che ruota forzatamente il *Facing* del bersaglio di $60^\circ$ (2 spicchi), esponendolo al *Flanking*.
   * **Opzione 3 (Rimbalzo) — Static Arc [⚡ Fulmine]:** Dardo da 120 Danno Energetico che si propaga automaticamente su 1 esagono adiacente se entrambi contengono già una carica elementale.
2. **Prep Phase (🔵 Prep Phase | 1 AP | CD 1)**
   * **Opzione 1 (Core) — Structure Shift [🌌 Flux]:** Smaterializza un segmento di muro solido per 1 turno trasformandolo in **Wireframe** (trasparente a LoS e proiettili, blocco fisico attivo) o innescando la reazione *Flux* del muro.
   * **Opzione 2 (Mobilità) — Speed Gate [💨 Aria]:** Proietta un varco di accelerazione su un lato dell'esagono; gli alleati che lo attraversano in *Dash Phase* ottengono +2 esagoni di movimento per 2 turni.
   * **Opzione 3 (Copertura) — Holo-Barricade [🛡️ Scudo]:** Materializza una parete olografica leggera (150 HP) su un lato vuoto bloccando la Linea di Vista nemica per 1 turno.
3. **Dash Phase (🟡 Dash Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Seeker Bot [⚡ Fulmine]:** Rilascia un drone che avanza di 3 esagoni lungo le macro-direttrici infliggendo 150 Danno Energetico, applicando *Slow* (+1 AP al movimento) e drenando -1 AP al primo nemico incontrato *(Sintonizzazione Liv. 5: variante Fuoco)*.
   * **Opzione 2 (Disimpegno) — Flux Blink [🌌 Flux]:** Teletrasporto istantaneo di 2 esagoni che ignora i muri solidi (senza attivare *Overwatch*) lasciando carica *Flux* sull'esagono di partenza.
   * **Opzione 3 (Zoning) — Cryo-Drone [❄️ Freddo]:** Arretra di 1 esagono rilasciando un micro-drone stazionario sulla cella d'origine che applica *Freddo* (+1 AP al movimento) entro 1 esagono.
4. **Abilità Speciale / Controllo (🔴 Blast Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core Riallineata) — EMP Disruption [⚡ Fulmine]:** Impulso ad area (esagono + 3 spicchi contigui) da 210 Danno Energetico che silenzia tutte le *Reaction* nemiche per 1 turno *(ex Arcane Flux)*.
   * **Opzione 2 (Attrazione) — Gravity Collapse [🌌 Flux]:** Implosione spaziale da 150 Danno Energetico che attira tutti i nemici entro 1 esagono verso il centro della cella bersaglio.
   * **Opzione 3 (Innesco Termico) — Thermite Pulse [🔥 Fuoco]:** Scarica termica da 160 Danno Fuoco che surriscalda l'esagono per 2 turni preparando *Vapore* o *Magma*.
5. **Ultimate (3 AP | CD 2–3)**
   * **Ultimate 1 (Core — 🔴 Blast Phase | CD 2) — Sticky Bomb [⚛️ Disintegrazione]:** Bomba adesiva ritardata su area 3x3 che ignora coperture parziali e detona a fine fase/turno infliggendo 420 Danni perforanti AoE.
   * **Ultimate 2 (Core — 🔵 Prep Phase | CD 3) — Tesla Dome [⚡ Fulmine]:** Cupola statica (raggio 3 esagoni); attraversarne il bordo in *Dash Phase* o spararvi attraverso infligge gravi danni elettrici e drena -1 AP.
   * **Ultimate 3 (Variante — 🔵 Prep Phase | CD 3) — Reality Refactor [🌌 Flux]:** Riscrive un'area 3x3 per 1 turno: tutti i muri solidi interni diventano *Wireframe* e ogni esagono acquisisce una carica *Flux*.
6. **Reaction Phase (🟣 Reaction Phase | 1–2 AP)**
   * **Opzione 1 (Core — Evasion | 1 AP) — Flux Deflector [🌌 Flux]:** Quando bersagliata da un proiettile Ranged, ne devia la traiettoria di $60^\circ$ (uno spicchio adiacente), potendo colpire un altro modello sulla nuova linea.
   * **Opzione 2 (Core — Counter | 1 AP) — Feedback Loop [⚡ Fulmine]:** Se colpita in mischia, rilascia una scarica magnetica che respinge l'attaccante di 1 esagono e applica *Fulmine* sulla sua cella.
   * **Opzione 3 (Core — Overwatch | 2 AP | CD 2) — Tesla Trap [⚡ Fulmine]:** Elettrifica 3 spicchi adiacenti ($90^\circ$); chi li attraversa in *Dash Phase* subisce 200 danni, mini-stun e interruzione immediata del movimento.

#### Matrice Combo, Sinergie e Matchup
| Nome Combo | Elementi | Fase di Chiusura | Effetto Deterministico |
| :--- | :--- | :--- | :--- |
| **Ionizzazione Spaziale** | 🌌 Flux + ⚡ Fulmine | 🔵 Prep Phase | Rende le coperture adiacenti conduttive, folgorando chi vi staziona dietro. |
| **Gabbia Ologramma** | 🌌 Flux + ⚡ Fulmine | 🔵 Prep Phase | Converte una copertura fisica in un muro elettrificato a senso unico. |
| **Collasso Strutturale** | 🌌 Flux + ⚡ Fulmine + ⚛️ Disint. | 🔴 Blast Phase | Polverizza tutti i muri nell'esagono infliggendo danni perforanti massicci. |
| **Polarizzazione Ionica** | 🌌 Flux + ⚡ Fulmine + 💧 Acqua | 🔴 Blast Phase | Trasforma il bersaglio in un magnete che attira i proiettili dei propri compagni per 1 turno. |
| **Monolite Riflettente** | 🌌 Flux + 🪨 Terra | 🔵 Prep Phase | Crea un muro radiale solido che isola il bersaglio generando *Body Block*. |

* **Sinergie Ottimali:** **Murdock** (*Structure Shift* apre linee di tiro impossibili per *Rifle Shot* e *Devastating Blast*), **Riktor** (*Chain Pull* trascina i nemici dentro *Tesla Dome* o *Sticky Bomb*) e **Crunch** (fornisce *Flux* per *Overdrive Meccanico*).
* **Matchup:** Domina contro **Steel** (annullandone le barriere fisiche con *Flux*) e **The Fey** (*Flux* su *Regenerative Hedge* attiva *Spore di Parassita*). Soffre **Kallari** (aggira le trappole sull'Asse Z colpendola nel cono posteriore) e **Drongo** (*Gag Grenade* spegne le sue skill).

---

### 🛡️ 3. STEEL — *Tank / Fortezza Difensiva*
Bastione inamovibile che presidia i coni di minaccia avversari, ridisegna le coperture geometriche e controlla l'inerzia cinetica. Con mobilità base 2, usa spesso l'**Overclock** per combinare *Bull Rush* e *Force Shield* nello stesso turno.

* **Statistiche Base:** 1900 HP | DEF Fisica: 60 ($\approx 37\%$ rid.) | RES Energetica: 30 ($\approx 23\%$ rid.) | Movimento: 2 esagoni | Gittata Base: 1 esagono.
* **Affinità Elementali:** 🛡️ **Scudo** (Primario), 🪨 **Terra**, ⚡ **Fulmine**, ⚛️ **Disintegrazione**, 💨 **Aria**.

#### Catalogo Abilità (6 Slot $\times$ 3 Opzioni)
1. **Attacco Base (🔴 Blast Phase | 2 AP | 0 CD)**
   * **Opzione 1 (Core) — Heavy Punch / Shield Bash [🛡️ Scudo]:** Colpo ravvicinato da 150 Danno Fisico che deposita *Scudo*, spinge indietro l'avversario e riduce del 50% il danno in uscita del bersaglio nel turno successivo.
   * **Opzione 2 (Spazzata Angolare) — Tower Cleave [🪨 Terra]:** Fendente pesante con lo scudo su 3 micro-spicchi frontali ($90^\circ$) da 120 Danno Fisico che applica *Terra*.
   * **Opzione 3 (Blocco Rotazione) — Magnetic Slam [⚡ Fulmine]:** Impatto da 130 Danno Fisico/Energetico che blocca il *Facing* del bersaglio per 1 turno impedendogli di ruotare.
2. **Prep Phase (🔵 Prep Phase | 1 AP | CD 1)**
   * **Opzione 1 (Core) — Force Shield [🛡️ Scudo]:** Erige una barriera olografica a "L" da 200 HP (50% DR / parata frontale per 2 turni), trasparente alla LoS e ai proiettili alleati ma bloccante per i nemici.
   * **Opzione 2 (Ancoraggio) — Bastion Anchor [🪨 Terra]:** Rinforza l'esagono occupato per 2 turni rendendo Steel immune al *Knockback* e applicando *Terra*.
   * **Opzione 3 (Deterrenza) — Arc Perimeter [⚡ Fulmine]:** Elettrifica i 3 spicchi frontali del proprio esagono; i nemici che terminano la *Dash Phase* a contatto subiscono 70 Danni Energetici e ricevono *Fulmine*.
3. **Dash Phase (🟡 Dash Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Bull Rush [🪨 Terra]:** Carica lineare di 3 esagoni da 180 Danno Fisico che sbalza i nemici indietro di 1 esagono (micidiale sui dislivelli dell'Asse Z per danni da caduta).
   * **Opzione 2 (Avanzata Protetta) — Phalanx Advance [🛡️ Scudo]:** Avanza di 2 esagoni con scudo alzato riducendo del 50% i danni frontali in corsa e generando 150 HP di scudo all'arrivo.
   * **Opzione 3 (Onda d'Urto) — Magneto Surge [⚡ Fulmine]:** Scatto di 2 esagoni che attira di 1 spicchio i nemici adiacenti alla traiettoria applicando *Fulmine* all'arrivo.
4. **Abilità Speciale / Controllo (🔴 Blast Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Ground Shock [⚡ Fulmine]:** Onda sismica elettrificata in cono frontale di $60^\circ$ da 220 Danno Fisico/Energetico.
   * **Opzione 2 (Faglia) — Tectonic Fracture [🪨 Terra]:** Colpisce in linea retta su 2 esagoni (170 Danno Fisico) trasformandoli in *Terreno Sconnesso* (costo movimento raddoppiato per 2 turni).
   * **Opzione 3 (Impulso Cinetico) — Repulsor Bash [🛡️ Scudo]:** Urto ad arco di $120^\circ$ (160 Danno Fisico) che respinge i nemici frontali di 1 esagono sul posto.
5. **Ultimate (🔴 Blast Phase | 3 AP | CD 3)**
   * **Ultimate 1 (Core) — Shield Slam [💨 Aria / 🪨 Terra]:** Salta e si schianta su area 3x3 ($360^\circ$) infliggendo 320 Danno Fisico, applicando **Stun** per 1 turno e **Knockup** ($Z+1$).
   * **Ultimate 2 (Core) — Kinetic Earthquake [⚛️ Disintegrazione]:** Onda in cono frontale che polverizza tutte le coperture leggere, i detriti e i bonus difensivi ambientali sul percorso.
   * **Ultimate 3 (Variante Difensiva) — Citadel Dome [🛡️ Scudo]:** Schiera simultaneamente 3 barriere direzionali da 250 HP attorno a un'area 3x3 isolando un nodo per 2 turni.
6. **Reaction Phase (🟣 Reaction Phase | 1–2 AP)**
   * **Opzione 1 (Core — Guard | 2 AP | CD 2) — Bodyguard [🪨 Terra]:** Intercetta un colpo diretto a un alleato vulnerabile ed erige una barriera da 250 HP che blocca i passaggi stretti.
   * **Opzione 2 (Core — Guard | 1 AP | CD 2) — Ablative Plating [🪨 Terra]:** Riduce del 50% il danno fisico subito; i frammenti staccati trasformano l'esagono in *Terreno Sconnesso*.
   * **Opzione 3 (Core — Counter | 1 AP) — Kinetic Rebound [🪨 Terra]:** Subendo danno fisico rilascia un'onda d'urto a $360^\circ$ che respinge di 1 esagono tutti i nemici adiacenti.

#### Matrice Combo, Sinergie e Matchup
| Nome Combo | Elementi | Fase di Chiusura | Effetto Deterministico |
| :--- | :--- | :--- | :--- |
| **Bastione Sismico** | 🛡️ Scudo + 🪨 Terra | 🔵 Prep Phase | Eleva la DR delle barriere al 75% conferendo +25% DEF base nell'esagono. |
| **Scudo Polarizzato** | 🛡️ Scudo + ⚡ Fulmine | 🔵 Prep Phase | Ritorno di fiamma elettrico sugli attaccanti a distanza. |
| **Faglia Ionizzata** | 🪨 Terra + ⚡ Fulmine | 🟡 Dash Phase | Trincea elettrificata che disabilita salti e abilità *Aria* nell'esagono. |
| **Cataclisma Difensivo** | 🛡️ Scudo + 🪨 Terra + ⚡ Fulmine | 🔴 Blast Phase | Fa esplodere lo scudo infliggendo sbalzo e stato *Dazed* (disabilita *Reaction*). |
| **Rifrazione Cinetica** | 🛡️ Scudo + ⚡ Fulmine + 🪨 Terra | 🟡 Dash Phase | Doppio *Knockback* ai nemici e invulnerabilità temporanea per Steel. |

* **Sinergie Ottimali:** **Murdock** (*Force Shield* gli lascia LoS libera mentre *Bull Rush* spinge i nemici sulle *Static Trap*), **Muriel** (accumulo scudi) e **Sevarog** (fornisce muri solidi per *Colossal Blow*).
* **Matchup:** Domina contro **Kallari** (*Faglia Ionizzata* blocca *Double Jump*, posizionamento spalle al muro nega *Flanking*), **Riktor** (*Bodyguard* intercetta il gancio) e **Yin**. Soffre **Gadget** (*Structure Shift* bypassa *Force Shield*), **Feng Mao** (*Earth Shatter* ignora DEF 60) e **Crunch** (*Attrito Termico* fonde l'armatura).

---

### 👼 4. MURIEL — *Supporto Scudi / Riposizionamento & Stasi*
Supporto predittivo capace di leggere in anticipo la *Blast Phase* nemica, salvare compagni ovunque sulla mappa, sollevarli sull'Asse Z e generare AP extra senza *Phase Lock*.

* **Statistiche Base:** 1100 HP | DEF Fisica: 25 ($\approx 20\%$ rid.) | RES Energetica: 40 ($\approx 28\%$ rid.) | Movimento: 3 esagoni | Gittata Base: 5 esagoni.
* **Affinità Elementali:** 🛡️ **Scudo** (Primario, assorbe 💚 Vita), ❄️ **Freddo**, 💨 **Aria**.

#### Catalogo Abilità (6 Slot $\times$ 3 Opzioni)
1. **Attacco Base (🔴 Blast Phase | 2 AP | 0 CD)**
   * **Opzione 1 (Core) — Frost Binding [❄️ Freddo]:** Sfera glaciale da 150–160 Danno Energetico che applica *Freddo* (+1 AP al costo di movimento successivo).
   * **Opzione 2 (Supporto Attivo) — Orb Pulse [🛡️ Scudo]:** Infligge 140 Danno Energetico a un nemico oppure conferisce 140 HP di scudo temporaneo a un alleato entro 5 esagoni.
   * **Opzione 3 (Spinta Angolare) — Serenity Gale [💨 Aria]:** Impulso d'aria da 110 Danno Energetico che spinge/ruota il bersaglio di 1 spicchio ($30^\circ$) spezzando la sua LoS.
2. **Prep Phase (🔵 Prep Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Consecrated Ground [🛡️ Scudo]:** Cupola protettiva 3x3 (250 HP scudo temporaneo per 2 turni) con immunità a *Knockback* e *Pull*.
   * **Opzione 2 (Innesco Glaciale) — Rime Sanctuary [❄️ Freddo]:** Infonde un'area 3x3 di *Freddo* conferendo +15 RES Energetica agli alleati interni.
   * **Opzione 3 (Corridoio Eolico) — Zephyr Veil [💨 Aria]:** Corrente ascensionale su 2 esagoni che riduce di -1 AP il costo di attraversamento in *Dash Phase* per gli alleati (2 turni).
3. **Dash Phase (🟡 Dash Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Reversal of Fortune [🛡️ Scudo]:** Volo globale istantaneo verso un alleato ovunque sulla mappa (ignora Asse Z), atterrando con scudi pesanti (fino a 450 HP) e sbalzo sui nemici adiacenti.
   * **Opzione 2 (Planata Aerea) — Seraphic Glide [💨 Aria]:** Spostamento verticale di 4 esagoni che scavalca ostacoli fino a $Z+2$, depositando *Aria* all'arrivo.
   * **Opzione 3 (Scia Criogenica) — Glacial Wake [❄️ Freddo]:** Scatto lineare di 3 esagoni che deposita *Freddo* lungo la scia e genera 150 HP di scudo personale.
4. **Abilità Speciale / Controllo (🔴 Blast / 🔵 Prep | 1–2 AP | CD 1–2)**
   * **Opzione 1 (Core — 🔴 Blast | 1 AP | CD 1) — Serenity [💨 Aria]:** Dardo rapido da 110 Danno che applica *Aria* e riduce di 2 esagoni la gittata di movimento nemica per 2 turni.
   * **Opzione 2 (Drenaggio AP — 🔴 Blast | 2 AP | CD 2) — Cryo-Shackle [❄️ Freddo]:** Scarica da 160 Danno Energetico che sottrae -1 AP al bersaglio nel turno successivo.
   * **Opzione 3 (Deviazione — 🔵 Prep | 1 AP | CD 1) — Soul Link Barrier [🛡️ Scudo]:** Collega Muriel a un alleato entro 4 esagoni per 1 turno: il 50% dei danni all'alleato viene deviato sugli scudi di Muriel.
5. **Ultimate (2–3 AP | CD 2–3)**
   * **Ultimate 1 (Core — 🔵 Prep | 3 AP | CD 3) — Alacrity Field [💨 Aria]:** Area 3x3; gli alleati che iniziano il turno successivo al suo interno ottengono **+2 AP gratuiti senza Phase Lock**.
   * **Ultimate 2 (Core — 🟣 Reaction | 2 AP | CD 2) — Aegis [🛡️ Scudo]:** Guardia suprema che trasferisce HP/Scudi per assorbire fino a 400 danni su un alleato durante la *Blast Phase*, impedendo colpi letali.
   * **Ultimate 3 (Variante — 🔴 Blast | 3 AP | CD 3) — Absolute Zero Stasis [❄️ Freddo]:** Congela un'area 3x3 in stasi cristallina per 1 turno (0 danni subiti, 0 azioni eseguibili).
6. **Reaction Phase (🟣 Reaction Phase | 1 AP)**
   * **Opzione 1 (Core — Guard | 1 AP) — Harmonic Resonance [🛡️ Scudo]:** Se un alleato entro 3 esagoni perde più del 30% degli HP in un colpo, gli conferisce istantaneamente uno scudo rigenerante d'emergenza.
   * **Opzione 2 (Core — Evasion | 1 AP) — Angelic Ascent [💨 Aria]:** Se bersagliata da un AoE a terra, solleva sé stessa e 1 alleato adiacente a Livello $Z+2$ (*Airborne*) evitando integralmente i danni terrestri.
   * **Opzione 3 (Variante — Guard | 1 AP) — Rime Counter [❄️ Freddo]:** Quando un alleato protetto da scudo viene colpito in mischia, applica istantaneamente *Freddo* (+1 AP al movimento) all'attaccante.

#### Matrice Combo, Sinergie e Matchup
| Nome Combo | Elementi | Fase di Chiusura | Effetto Deterministico |
| :--- | :--- | :--- | :--- |
| **Cristallizzazione Divina** | 🛡️ Scudo + ❄️ Freddo | 🔵 Prep Phase | Armatura cristallina che assorbe il 100% del danno del prossimo singolo attacco. |
| **Scudo Conduttivo** | 🛡️ Scudo + ⚡ Fulmine | 🔵 Prep Phase | Riflette parte dei danni energetici subiti sugli attaccanti. |
| **Bolla Olografica** | 🛡️ Scudo + 🌌 Flux | 🔵 Prep Phase | Cupola protettiva trasparente alla Linea di Vista alleata. |
| **Vapore Ghiacciato** | ❄️ Freddo + 🔥 Fuoco | 🔴 Blast Phase | Spegne le fiamme e stordisce il bersaglio per 1 turno. |
| **Ibernazione Cinetica** | ❄️ Freddo + 🪨 Terra | 🟡 Dash Phase | Congela il bersaglio trascinato o sbalzato impedendone le azioni. |

* **Sinergie Ottimali:** **The Fey** (*Freddo* + *Acqua* = *Ghiaccio* istantaneo), **Kallari & Feng Mao** (*Alacrity Field* regala 8 AP senza debito) e **Steel**.
* **Matchup:** Domina contro **Riktor & Steel** (*Consecrated Ground* e *Angelic Ascent* neutralizzano *Pull*, *Bull Rush* e *Shield Slam*) e **Murdock** (*Cristallizzazione Divina* azzera *Devastating Blast*). Soffre **Gadget** (*EMP Disruption* spegne le *Reaction*), **Drongo** (*Silence*) e **Feng Mao** (*Disintegrazione* perfora gli scudi).

---

### 🗡️ 5. KALLARI — *Assassina / Flanker Verticale*
Specialista del micro-posizionamento e delle esecuzioni chirurgiche: combina invisibilità termica e verticalità sull'Asse Z per superare coperture e colpire costantemente nel cono posteriore di *Flanking*.

* **Statistiche Base:** 950 HP | DEF Fisica: 15 ($\approx 13\%$ rid.) | RES Energetica: 20 ($\approx 17\%$ rid.) | Movimento: 4 esagoni | Gittata Base: 1 esagono.
* **Affinità Elementali:** ❄️ **Freddo**, 💨 **Aria**, 💧 **Acqua**, ⚛️ **Disintegrazione** *(riallineamento di 💀 Morte)*.

#### Catalogo Abilità (6 Slot $\times$ 3 Opzioni)
1. **Attacco Base (🔴 Blast Phase | 2 AP | 0 CD)**
   * **Opzione 1 (Core — 9 Elementi) — Mist Slash [💧 Acqua]:** Fendente ravvicinato da 210 Danno Fisico che lascia condensa depositando *Acqua* sull'esagono e riduce la LoS nemica per 2 turni.
   * **Opzione 2 (Core Spec) — Shadow Dagger [⚛️ Disintegrazione / 💀 Morte]:** Stoccata da 190 Danno Fisico (285 in *Flanking* posteriore), ideale come innesco rapido.
   * **Opzione 3 (Controllo Angolare) — Frost Feint [❄️ Freddo]:** Colpo da 150 Danno Fisico che applica *Freddo* e ruota Kallari di 1 spicchio ($30^\circ$) attorno al bersaglio per restarne nell'angolo cieco.
2. **Prep Phase (🔵 Prep Phase | 1–2 AP | CD 2)**
   * **Opzione 1 (Core) — Shadow Walk [❄️ Freddo]:** Entra in *Stealth* termico per 2 turni; ignora gli *Overwatch* nemici, non fa scattare trappole, deposita *Freddo* e conferisce +30% di danno al colpo che rompe l'invisibilità.
   * **Opzione 2 (Cortina Visiva) — Condensation Veil [💧 Acqua]:** Micro-nebbia sull'esagono che applica *Acqua* e interrompe la LoS nemica fino a fine *Dash Phase*.
   * **Opzione 3 (Preparazione Verticale) — Thermal Updraft [💨 Aria]:** Infonde *Aria* e riduce di -1 AP il costo del prossimo spostamento sull'Asse Z.
3. **Dash Phase (🟡 Dash Phase | 1 AP | CD 1)**
   * **Opzione 1 (Core) — Double Jump [💨 Aria]:** Salto acrobatico di 2–3 esagoni a raggio acustico zero che scavalca muri radiali e dislivelli sull'Asse Z, atterrando direttamente nei 4 spicchi posteriori nemici.
   * **Opzione 2 (Disimpegno Fluido) — Slipstream Slide [💧 Acqua]:** Scivolata di 2 esagoni (2 AP | CD 2) che ignora il *Body Block* sui bordi e deposita *Acqua* in partenza.
   * **Opzione 3 (Infiltrazione Glaciale) — Rime Step [❄️ Freddo]:** Scatto silenzioso di 2 esagoni (1 AP | CD 2) che deposita *Freddo* sull'esagono d'arrivo.
4. **Abilità Speciale / Esecuzione (🔴 Blast Phase | 2–3 AP | CD 2)**
   * **Opzione 1 (Core) — Crippling Strike [⚛️ Disintegrazione]:** Attacco perforante da tergo che applica Sanguinamento (DoT 2 turni) e ignora le difese fisiche in *Flanking*.
   * **Opzione 2 (Affondo Criogenico) — Icepick Lunge [❄️ Freddo]:** Stoccata lineare (gittata 2 esagoni, 2 AP) da 180 Danno Fisico che applica *Freddo* per chiudere *Ghiaccio*.
   * **Opzione 3 (Taglio Aerodinamico) — Vacuum Cut [💨 Aria]:** Fendente da 170 Danno Fisico (2 AP) che applica *Aria* e ruota forzatamente il *Facing* nemico di $60^\circ$.
5. **Ultimate (3 AP | CD 2–3)**
   * **Ultimate 1 (Core — 🔴 Blast | CD 2–3) — Death Toll / Death Sentence [⚛️ Disintegrazione]:** Esecuzione verticale da 450 Danno Perforante; infligge danni tripli (o 100% bypass DEF) su bersagli con armatura a 0 o colpiti alle spalle in *Flanking*.
   * **Ultimate 2 (Core — 🟡 Dash | CD 3) — Phantom Strike [❄️ Freddo]:** Scatto rettilineo perforante di 5 esagoni attraverso tutti i modelli fisici sulla traiettoria, applicando *Freddo* e danni critici.
   * **Ultimate 3 (Variante Aerea — 🔴 Blast | CD 3) — Zero-Point Drop [💨 Aria]:** Balzo a $Z+2$ seguito da fendente circolare da 340 Danno Fisico che deposita *Aria* e dissolve nubi difensive.
6. **Reaction Phase (🟣 Reaction Phase | 1–2 AP)**
   * **Opzione 1 (Core — Evasion | 2 AP | CD 2) — Acrobatic Evasion / Shadow Dodge [💨 Aria]:** Se bersagliata da un attacco *Blast* o AoE, salta in un esagono adiacente (o trasla di $30^\circ$) evitando il 100% del danno.
   * **Opzione 2 (Core — Counter | 1 AP) — Vengeful Shade [⚛️ Disintegrazione]:** Se un alleato in LoS subisce danno, lancia istantaneamente un pugnale perforante contro l'attaccante (danno puro).
   * **Opzione 3 (Evasion Criogenica | 1 AP | CD 2) — Frost Decoy [❄️ Freddo]:** In risposta a un attacco melee, arretra di 1 esagono lasciando *Freddo* sulla cella dell'attaccante.

#### Matrice Combo, Sinergie e Matchup
| Nome Combo | Elementi | Fase di Chiusura | Effetto Deterministico |
| :--- | :--- | :--- | :--- |
| **Ghiaccio $\rightarrow$ Frantumazione Letale** | 💧 Acqua + ❄️ Freddo (+ 🪨 Terra) | 🔴 Blast Phase | Kallari congela in autonomia (*Root*); un alleato con *Terra* detona 500 Danni Puri (100% bypass DEF). |
| **Vento Gelido** | 💨 Aria + ❄️ Freddo | 🟡 Dash Phase | +1 AP al costo di movimento nell'esagono. |
| **Lama Sottovuoto** | 💨 Aria + ⚛️ Disintegrazione | 🔴 Blast Phase | Ignora scudi leggeri e raddoppia la distanza di qualsiasi sbalzo subito dal bersaglio. |
| **Dispersione Sottovuoto** | 🔥 Fuoco + 💧 Acqua + 💨 Aria | 🟡 Dash Phase | Spazza via il *Vapore* applicando *Silenzio* (-1 AP) agli occupanti. |
| **Detonazione al Plasma** | 💨 Aria + ⚡ Fulmine + ⚛️ Disint. | 🔴 Blast Phase | Acceca tutti i modelli rivolti verso l'esplosione (Gittata azzerata per 1 turno). |

* **Sinergie Ottimali:** **Feng Mao & Steel** (detonano con *Terra* il *Ghiaccio* di Kallari), **Muriel** (*Alacrity Field* le dona 8 AP senza debito) e **Sevarog** (*Lama Sottovuoto* raddoppia lo sbalzo di *Colossal Blow*).
* **Matchup:** Domina contro **Murdock & Gadget** (aggira *Overwatch* e trappole tramite *Stealth* e Asse Z). Soffre **Riktor** (aura *Electrocute* e *Skewer* senza LoS), **Steel** (*Faglia Ionizzata* anti-*Aria* e spalle al muro) e **Muriel** (*Angelic Ascent* salva da *Death Toll*).

---

### 🔫 6. MURDOCK — *Artiglieria Ranged / Cecchino*
Dominatore delle lunghe direttrici macroscopiche (8 esagoni): presidia i corridoi con *Overwatch* e trappole, prepara reazioni termobariche da lontano e colpisce bersagli anche attraverso i muri solidi.

* **Statistiche Base:** 1050 HP | DEF Fisica: 20 ($\approx 17\%$ rid.) | RES Energetica: 15 ($\approx 13\%$ rid.) | Movimento: 3 esagoni | Gittata Base: 8 esagoni.
* **Affinità Elementali:** ⚛️ **Disintegrazione** (Primario), 🔥 **Fuoco**, ⚡ **Fulmine**, 💨 **Aria**, 🛡️ **Scudo**.

#### Catalogo Abilità (6 Slot $\times$ 3 Opzioni)
1. **Attacco Base (🔴 Blast Phase | 2 AP | 0 CD)**
   * **Opzione 1 (Core) — Rifle Shot [⚛️ Disintegrazione]:** Colpo sniper da 220 Danno Perforante lungo i coni da $30^\circ$ (gittata 8 esagoni).
   * **Opzione 2 (Core Variante) — Plasma Shot [⚡ Fulmine]:** Sparo energetico a lunga gittata che deposita carica *Fulmine* sull'esagono bersaglio.
   * **Opzione 3 (Ravvicinato) — Concussive Shot [💨 Aria]:** Rosa a corto raggio (3 esagoni, 3 spicchi frontali) da 160 Danno Fisico che applica *Aria* e respinge di 1 spicchio.
2. **Prep Phase (🔵 Prep Phase | 1 AP | CD 1–2)**
   * **Opzione 1 (Core) — Static Trap [⚡ Fulmine]:** Mina invisibile su esagono adiacente (2 turni) che infligge 120 Danni e applica *Root / Stun* per 1 turno.
   * **Opzione 2 (Marcatura Termica) — Flare Beacon [🔥 Fuoco]:** Segnalatore fino a 6 esagoni che deposita *Fuoco* e rivela modelli in *Stealth* per 1 turno.
   * **Opzione 3 (Calibrazione) — Rail Overcharge [⚛️ Disintegrazione]:** Estende di +2 esagoni la gittata della prossima abilità *Blast* e deposita *Disintegrazione* sul proprio esagono.
3. **Dash Phase (🟡 Dash Phase | 1–2 AP | CD 1)**
   * **Opzione 1 (Core) — Tactical Retreat [💨 Aria]:** Spara a terra per sbalzarsi indietro di 2 esagoni mantenendo il *Facing* puntato in avanti (kiting anti-flanking).
   * **Opzione 2 (Angolazione) — Thruster Strafe [⚡ Fulmine]:** Scatto laterale di 2 esagoni a mira bloccata che lascia carica *Fulmine* in partenza.
   * **Opzione 3 (Disimpegno Termico) — Scorched Disengage [🔥 Fuoco]:** Arretra di 1 esagono incendiando la cella abbandonata (*Fuoco* per 2 turni).
4. **Abilità Speciale / Controllo (🔴 Blast Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Incendiary Shell / Buckshot [🔥 Fuoco]:** Proiettile esplosivo AoE da 220–240 Danni Fuoco (esagono + 3 spicchi) che lascia fuoco per 2 turni.
   * **Opzione 2 (Perforazione) — Sabot Round [⚛️ Disintegrazione]:** Proiettile da 200 Danno Perforante che attraversa 1 copertura leggera o barriera parziale.
   * **Opzione 3 (Soppressione) — Arc Suppressor [⚡ Fulmine]:** Scarica da 170 Danno Energetico che applica *Fulmine* e riduce di 1 esagono il movimento nemico successivo.
5. **Ultimate (3 AP | CD 3)**
   * **Ultimate 1 (Core — 🔴 Blast | CD 3) — Devastating Blast [⚛️ Disintegrazione]:** Raggio laser globale passante da 520 Danno Energetico che ignora e trapassa qualsiasi muro (traiettoria telegraphed in *Prep Phase*).
   * **Ultimate 2 (Core — 🔵 Prep | CD 3) — Orbital Laser [🔥 Fuoco]:** Marca un'area 2x2; alla fine della *Blast Phase* del turno successivo colpisce con danni termici apocalittici incurabili.
   * **Ultimate 3 (Variante — 🔴 Blast | CD 3) — Ion Rail Barrage [⚡ Fulmine]:** Fascio ionico lungo 6 esagoni da 340 Danno Energetico che deposita *Fulmine* e drena -1 AP ai bersagli colpiti.
6. **Reaction Phase (🟣 Reaction Phase | 1–2 AP)**
   * **Opzione 1 (Core — Overwatch | 2 AP | CD 2) — Sniper Overwatch [⚛️ Disintegrazione]:** Colpisce per 250 danni il primo nemico che corre o sprinta nel cono visivo in *Dash Phase*.
   * **Opzione 2 (Core — Guard | 1 AP) — Riot Shield [🛡️ Scudo]:** Scudo fisico sui 3 spicchi frontali ($90^\circ$) che assorbe integralmente 1 proiettile o attacco melee frontale.
   * **Opzione 3 (Core — Evasion | 1 AP) — Smokescreen Retreat [💨 Aria]:** Se attaccato in mischia, balza indietro di 1 esagono rilasciando *Aria* (accecamento) sulla cella d'origine.

#### Matrice Combo, Sinergie e Matchup
| Nome Combo | Elementi | Fase di Chiusura | Effetto Deterministico |
| :--- | :--- | :--- | :--- |
| **Colpo Termico Perforante** | ⚛️ Disint. + 🔥 Fuoco | 🔴 Blast Phase | Ignora il 50% dell'armatura bersaglio e incendia la copertura. |
| **Forza di Taglio** | ⚛️ Disint. + 💨 Aria | 🔴 Blast Phase | Annulla il 100% della Riduzione Danno conferita dagli scudi. |
| **Vuoto Termobarico** | 💨 Aria + 🔥 Fuoco | 🔴 Blast Phase | Risucchia i nemici verso il centro dell'esagono prima di detonare. |
| **Detonazione Termobarica** | 🔥 Fuoco + 💨 Aria + ⚛️ Disint. | 🔴 Blast Phase | Esplosione pura che polverizza i muri e ignora l'armatura. |
| **Arresto Orbitale** | ⚛️ Disint. + ⚡ Fulmine + 💨 Aria | 🟡 Dash Phase | Infligge uno *Stun* di 2 turni con paralisi totale. |

* **Sinergie Ottimali:** **Gadget** (*Structure Shift* apre varchi *Wireframe* per sparare), **Steel** (*Force Shield* lo protegge frontalmente) e **Riktor** (*Chain Pull* trascina i nemici nell'*Orbital Laser* o sul laser globale).
* **Matchup:** Domina contro **The Fey, Muriel e Drongo** (li sovrasta in gittata). Soffre **Kallari** (aggira *Overwatch* e *Riot Shield* colpendo alle spalle), **Yin** (*Backlash* riflette i suoi proiettili) e **Riktor** (*Chain Intercept*).

---

### ⚔️ 7. FENG MAO — *Bruiser / Guerriero Tattico*
Duellante autosufficiente da prima linea che trasforma la difesa in economia d'azione (+1 AP alla rottura dello scudo) e sfonda le coperture con scatti infuocati e fendenti sismici.

* **Statistiche Base:** 1350 HP | DEF Fisica: 35 ($\approx 26\%$ rid.) | RES Energetica: 25 ($\approx 20\%$ rid.) | Movimento: 3 esagoni | Gittata Base: 1–2 esagoni.
* **Affinità Elementali:** 🔥 **Fuoco** (Primario), 🪨 **Terra**, 🛡️ **Scudo**, ⚛️ **Disintegrazione**.

#### Catalogo Abilità (6 Slot $\times$ 3 Opzioni)
1. **Attacco Base (🔴 Blast Phase | 2 AP | 0 CD)**
   * **Opzione 1 (Core Spec) — Blade Sweep [🔥 Fuoco]:** Spazzata di lama infuocata da 180 Danno su 2 micro-spicchi frontali contigui che deposita *Fuoco*.
   * **Opzione 2 (Core GDD) — Seismic Slice [🪨 Terra]:** Fendente pesante da 260 Danno Fisico in cono frontale stretto di $60^\circ$ per auto-innescare *Magma* dopo *Reaping Dash*.
   * **Opzione 3 (Variante Difensiva) — Sweep Slash [🛡️ Scudo]:** Fendente rotante a medio raggio che deposita carica *Scudo* sull'esagono.
2. **Prep Phase (🔵 Prep Phase | 1 AP | CD 1)**
   * **Opzione 1 (Core) — Conversion Shield [🛡️ Scudo]:** Scudo personale da 220 HP per 2 turni; se infranto nel turno corrente, rimborsa **+1 AP** a Feng Mao.
   * **Opzione 2 (Sintonizzazione Liv. 3) — Flux Conversion [🌌 Flux]:** Trasforma lo scudo in intangibilità ai proiettili fisici, riducendo però la RES Energetica.
   * **Opzione 3 (Assetto Sismico) — Tectonic Stance [🪨 Terra]:** Applica *Terra* sulla cella occupata e riduce di 1 esagono gli sbalzi subiti nel turno.
3. **Dash Phase (🟡 Dash Phase | 1–2 AP | CD 2)**
   * **Opzione 1 (Core) — Reaping Dash [🔥 Fuoco]:** Scatto propulsivo di 3 esagoni che oltrepassa coperture senza costi AP extra, applica *Fuoco* all'atterraggio e lascia scia da 90 Danno/turno per 2 turni.
   * **Opzione 2 (Avanzata Corazzata) — Vanguard Charge [🛡️ Scudo]:** Scatto lineare di 2 esagoni con 120 HP di scudo frontale e deposito *Scudo* all'arrivo.
   * **Opzione 3 (Incursione Sismica) — Tremor Lunge [🪨 Terra]:** Balzo radente di 2 esagoni che deposita *Terra* all'atterraggio per innescare *Shatter* o *Magma*.
4. **Abilità Speciale / Controllo (🔴 Blast Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Seismic Slice [🪨 Terra]:** Fendente radente lungo un asse macro-esagonale (260 Danno Fisico) che demolisce barriere leggere e detona *Ghiaccio* o *Fango* *(Phase Shift Liv. 10: spostabile in Reaction Phase)*.
   * **Opzione 2 (Perforazione) — Piercing Halberd [⚛️ Disintegrazione]:** Affondo a gittata 2 esagoni da 200 Danno Perforante (ignora 50% DEF) che deposita *Disintegrazione*.
   * **Opzione 3 (Sbarramento Termico) — Cinder Arc [🔥 Fuoco]:** Spazzata ad arco di $120^\circ$ da 190 Danno Fuoco con braci persistenti sui 3 spicchi frontali per 1 turno.
5. **Ultimate (🔴 Blast Phase | 3 AP | CD 3)**
   * **Ultimate 1 (Core) — Earth Shatter [⚛️ Disintegrazione]:** Schianto rettilineo (3–5 esagoni) da 480 Danno Energetico perforante che ignora l'armatura e polverizza tutte le coperture leggere sul percorso.
   * **Ultimate 2 (Core) — Dragon's Breath [🔥 Fuoco]:** Spazzata frontale di $180^\circ$ (profonda 2 esagoni) che applica *Fuoco* e distrugge la DEF fisica dei bersagli colpiti.
   * **Ultimate 3 (Variante Sismica) — Tectonic Sundering [🪨 Terra]:** Colpo d'alabarda su area 3x3 da 340 Danno Fisico che deposita *Terra* e crea *Terreno Sconnesso*.
6. **Reaction Phase (🟣 Reaction Phase | 1–2 AP)**
   * **Opzione 1 (Core — Counter | 1–2 AP | CD 2) — Parry & Riposte [🛡️ Scudo]:** Devia un attacco melee frontale (0 danno), risponde per 200 danni e applica **Dazed** (no *Reaction*).
   * **Opzione 2 (Core — Evasion | 1 AP) — Smoldering Step [🔥 Fuoco]:** Se puntato da un attacco Ranged, scatta lateralmente di 1 esagono annullando la mira e lasciando *Fuoco*.
   * **Opzione 3 (Variante — Counter | 2 AP | CD 2) — Counter Slash [⚛️ Disintegrazione]:** Riduce del 60% il danno melee subito e contrattacca con 200 danni perforanti.

#### Matrice Combo, Sinergie e Matchup
| Nome Combo | Elementi | Fase di Chiusura | Effetto Deterministico |
| :--- | :--- | :--- | :--- |
| **Magma / Lama di Magma** | 🔥 Fuoco + 🪨 Terra | 🔵 Prep / 🟡 Dash | Fonde il suolo raddoppiando i costi AP di movimento + DoT. |
| **Fornace Cinetica** | 🛡️ Scudo + 🔥 Fuoco | 🔵 Prep Phase | Surriscalda *Conversion Shield* infliggendo ustioni di ritorno in mischia. |
| **Taglio Fotonico** | 🔥 Fuoco + ⚛️ Disint. | 🔴 Blast Phase | I successivi 2 attacchi ignorano muri e scudi energetici. |
| **Schianto Magmatico** | 🔥 Fuoco + 🛡️ Scudo + ⚛️ Disint. | 🔴 Blast Phase | Apre un crepaccio con danni critici abbassando l'esagono a $Z-1$. |
| **Frantumazione Letale (Squadra)** | 💧 Acqua + ❄️ Freddo + 🪨 Terra | 🔴 Blast Phase | *Seismic Slice* detona il *Ghiaccio* per 500 Danni Puri (100% bypass DEF). |

* **Sinergie Ottimali:** **The Fey & Muriel** (Feng Mao detona il loro *Ghiaccio*; Muriel fornisce *Scudo* a distanza), **Murdock** (asse *Fuoco + Disintegrazione*) e **Crunch** (asse *Fuoco + Terra*).
* **Matchup:** Domina contro **Steel, Muriel & Murdock** (perfora armature/scudi e raggiunge le alture). Soffre **The Fey** (*Fango* blocca *Reaping Dash*), **Aurora** (*Hoarfrost* ferma gli assalti melee) e **Riktor** (*Chain Pull* lo sposta fuori asse).

---

### 🌿 8. THE FEY — *Supporto Naturale / Zoner Ambientale*
Alchimista del terreno specializzata nell'inondare la griglia di *Acqua*, *Fango* e *Ghiaccio* per alterare il pathfinding, strangolare l'economia AP avversaria e paralizzare la *Dash Phase* nemica.

* **Statistiche Base:** 1150 HP | DEF Fisica: 20 ($\approx 17\%$ rid.) | RES Energetica: 30 ($\approx 23\%$ rid.) | Movimento: 3 esagoni | Gittata Base: 5 esagoni.
* **Affinità Elementali:** 💧 **Acqua** (Primario), 🪨 **Terra**, ❄️ **Freddo**, 🛡️ **Scudo** *(riallineamento di 💚 Vita; legacy 💀 Morte riallineata su 💧 Acqua / ❄️ Freddo)*.

#### Catalogo Abilità (6 Slot $\times$ 3 Opzioni)
1. **Attacco Base (🔴 Blast Phase | 1–2 AP | 0 CD)**
   * **Opzione 1 (Core) — Nettle Shot [💧 Acqua]:** Dardo di spine da 140–150 Danno Energetico che applica *Acqua*; se colpisce un nemico nel *Fango* (Acqua + Terra), rimborsa **+1 AP** a The Fey.
   * **Opzione 2 (Spingimento Botanico) — Thorn Lash [🪨 Terra]:** Frustata di radici (4 esagoni) da 130 Danno Fisico che deposita *Terra* e spinge il nemico di $30^\circ$.
   * **Opzione 3 (Rallentamento) — Dew Spike [❄️ Freddo]:** Scheggia di brina da 120 Danno Energetico che applica *Freddo* (+1 AP al movimento).
2. **Prep Phase (🔵 Prep Phase | 1–2 AP | CD 1–2)**
   * **Opzione 1 (Core) — Untamed Growth [💧 Acqua]:** Bocciolo esplosivo che inonda un'area 3x3 di *Acqua* ed esplode a fine turno per 180 Danno.
   * **Opzione 2 (Copertura) — Regenerative Hedge [🛡️ Scudo / 💚 Vita]:** Siepe da 80 HP (20% DR) sul perimetro che blocca la LoS nemica e conferisce 60 HP/turno di scudo locale (o cura in variante *Vita*).
   * **Opzione 3 (Ancoraggio) — Root Bed [🪨 Terra]:** Area 3x3 di radici che applicano *Terra* e impediscono ai nemici interni di attivare l'Overclock nel turno corrente.
3. **Dash Phase (🟡 Dash Phase | 1–2 AP | CD 1–2)**
   * **Opzione 1 (Core) — Bramble Patch [🪨 Terra]:** Muro/striscia di rovi lungo 4 esagoni; attraversarlo costa +2 AP extra ai nemici e genera **Fango** in autonomia se sovrapposto a *Untamed Growth*.
   * **Opzione 2 (Leggera) — Wet Brambles [💧 Acqua]:** Scia di rovi bagnati a basso costo (1 AP | CD 1) da 70 Danno/turno e +1 AP al costo di movimento.
   * **Opzione 3 (Disimpegno) — Mist Hop [💧 Acqua]:** Arretra di 2 esagoni lasciando una chiazza d'*Acqua* sulla cella di partenza.
4. **Abilità Speciale / Controllo (🔴 Blast Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core Riallineata) — Spore Cloud [💧 Acqua / 💀 Morte]:** Nube di spore da 180 Danno AoE con 50 Danno/turno per 2 turni.
   * **Opzione 2 (Sprofondamento) — Mire Surge [🪨 Terra]:** Radici e fango in cono frontale di $60^\circ$ (160 Danno) che depositano *Terra* e interrompono eventuali *Overwatch*.
   * **Opzione 3 (Gelo Botanico) — Frost Pollen [❄️ Freddo]:** Polline criogenico (gittata 5 esagoni, 150 Danno Energetico) che applica *Freddo* congelando all'istante le celle bagnate.
5. **Ultimate (3 AP | CD 3)**
   * **Ultimate 1 (Core — 🔴 Blast | CD 3) — Flytrap [❄️ Freddo]:** Pianta carnivora che trascina al centro i nemici entro 2–3 esagoni infliggendo 280 Danno Energetico e applicando *Root / Freeze* + *Freddo*.
   * **Ultimate 2 (Core — 🔵 Prep | CD 3) — Nature's Vengeance [🛡️ Scudo / 💚 Vita]:** Rianima un alleato caduto al 25% HP avvolto in un bozzolo protettivo, imponendogli però un *Phase Lock* nel turno successivo.
   * **Ultimate 3 (Variante — 🔵 Prep | CD 3) — Overgrowth Labyrinth [🪨 Terra]:** Circonda un'area 3x3 con pareti di rovi (150 HP ciascuna) che bloccano movimento e LoS per 2 turni.
6. **Reaction Phase (🟣 Reaction Phase | 1–2 AP)**
   * **Opzione 1 (Core — Counter | 1 AP) — Thorned Carapace [💧 Acqua]:** Reagisce agli attacchi Ranged applicando **Blind** (annulla LoS e *Facing*) al tiratore per il resto del turno.
   * **Opzione 2 (Core — Guard | 1–2 AP) — Bramble Shield [🪨 Terra]:** Si radica al suolo ottenendo immunità a *Knockback* / *Pull* e 50% DR contro attacchi ed esplosioni *Blast*.
   * **Opzione 3 (Core — Overwatch | 2 AP | CD 2) — Vines Overwatch [💧 Acqua]:** Intercetta il primo nemico che entra nel cono visivo in *Dash Phase* infliggendo 130 Danni, applicando *Acqua* e radicandolo (*Root*).

#### Matrice Combo, Sinergie e Matchup
| Nome Combo | Elementi | Fase di Chiusura | Effetto Deterministico |
| :--- | :--- | :--- | :--- |
| **Fango Vischioso** | 💧 Acqua + 🪨 Terra | 🟡 Dash Phase | Azzera i PM residui e attiva il rimborso +1 AP di *Nettle Shot*. |
| **Fioritura del Ghiaccio** | 💧 Acqua + ❄️ Freddo | 🔵 Prep / 🟡 Dash | Interrompe il movimento (*Dash*) o applica *Root* per 1 turno (*Prep*). |
| **Elettrolisi Ambientale** | 💧 Acqua + ⚡ Fulmine | 🔵 Prep / 🟡 Dash | Drena AP (*Prep*) o stordisce chi scatta nell'area (*Dash*). |
| **Vapore Tossico** | 💧 Acqua + 🔥 Fuoco | 🔴 Blast Phase | Nube ustionante che oscura totalmente la LoS. |
| **Sovraccarico Elettrolitico (Squadra)** | ⚡ Fulmine + 💧 Acqua + 🪨 Terra | 🔴 Blast Phase | *Arco Voltaico* da +450 Danni Fulmine a catena. |

* **Sinergie Ottimali:** **Riktor & Gadget** (detonano il suo *Fango* con *Fulmine*; *Flytrap* blocca bersagli per *Sticky Bomb*), **Murdock** (*Flytrap* raggruppa per *Incendiary Shell*) e **Kallari**.
* **Matchup:** Domina contro **Feng Mao & Sevarog** (*Fango* e rovi paralizzano gli scatti a terra), **Riktor & Steel** (*Bramble Shield* nega *Pull* e cariche). Soffre **Kallari** (scavalca rovi sull'Asse Z), **Gadget** (*Flux* trasforma la siepe in trappola contro gli alleati) e **Murdock**.

---

## 4. Roster di Espansione (6 Eroi Avanzati)

---

### 🔗 9. PHASE — *Supporto Simbiotico / Manipolatrice del Legame*
Unico supporto basato sulla simbiosi permanente tramite legame telecinetico (*Link*): estrae istantaneamente il compagno dai coni di tiro, lo fionda in ingaggio e ne potenzia l'economia AP.

* **Statistiche Base:** 1050 HP | DEF Fisica: 20 ($\approx 17\%$ rid.) | RES Energetica: 35 ($\approx 26\%$ rid.) | Movimento: 3 esagoni | Gittata Base: 5 esagoni.
* **Affinità Elementali:** 🛡️ **Scudo** *(ex 💚 Vita)*, 🌌 **Flux** *(ex Arcano)*, ❄️ **Freddo** (+ supporto 💨 Aria).

#### Catalogo Abilità (6 Slot $\times$ 3 Opzioni)
1. **Attacco Base (🔴 Blast Phase | 2 AP | 0 CD)**
   * **Opzione 1 (Core) — Energy Lance [❄️ Freddo]:** Raggio continuo (5 esagoni) da 150 Danno Energetico che applica *Freddo/Slow*; se il bersaglio è già rallentato, lo radica sul posto (*Root*).
   * **Opzione 2 (Catalizzatore) — Psionic Bolt [🌌 Flux]:** Dardo da 140 Danno Energetico che deposita *Flux* e spinge il bersaglio di $30^\circ$ fuori asse.
   * **Opzione 3 (Sostegno) — Tether Pulse [🛡️ Scudo]:** Infligge 120 Danno Energetico e trasferisce 90 HP di scudo temporaneo all'alleato collegato.
2. **Prep Phase (🔵 Prep Phase | 1–2 AP | CD 1)**
   * **Opzione 1 (Core) — Telekinetic Link [🛡️ Scudo / 💚 Vita]:** Collega un alleato entro 5 esagoni condividendo scudi/rigenerazione (140 HP scudo o 80 HP/turno HoT); riattivarlo (1 AP) estrae l'alleato fino a 3 esagoni verso Phase o lo proietta in avanti.
   * **Opzione 2 (Sospensione) — Grav-Lift Conduit [🌌 Flux]:** Solleva l'alleato collegato a Livello $Z+1$ per 1 turno (immune a trappole terrestri, vantaggio d'elevazione).
   * **Opzione 3 (Ancoraggio) — Cryo-Tether [❄️ Freddo]:** Deposita *Freddo* sull'esagono dell'alleato collegato per auto-attivare *Legame Cristallino*.
3. **Dash Phase (🟡 Dash Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Hyperflux Step [🛡️ Scudo / 💨 Aria]:** Scatto sincronizzato di 2 esagoni per Phase e simultaneamente di 2 esagoni per l'alleato collegato.
   * **Opzione 2 (Fionda) — Telekinetic Sling [🌌 Flux]:** Lancia l'alleato collegato di 3 esagoni lungo una direttrice ignorando il *Body Block* e depositando *Flux* all'atterraggio.
   * **Opzione 3 (Disimpegno) — Frost Slide [❄️ Freddo]:** Scivolata di 3 esagoni lasciando scia di *Freddo* sulla cella d'origine.
4. **Abilità Speciale / Controllo (🔴 Blast Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Psychic Flare [🌌 Flux]:** Onda psionica da 170 Danno Energetico emessa attorno sia a Phase sia all'alleato collegato (raggio 1 esagono ciascuno), applicando **Blind** per 1 turno.
   * **Opzione 2 (Sbarramento) — Cryo Lance Sweep [❄️ Freddo]:** Fascio congelante su 3 spicchi frontali (4 esagoni, 160 Danno) che applica *Freddo* e respinge di 1 esagono i nemici entro 2 celle.
   * **Opzione 3 (Sovraccarico) — Kinetic Feedback [🛡️ Scudo]:** Impulso da 130 Danno attorno all'alleato collegato che converte il 100% del danno in scudo condiviso.
5. **Ultimate (3 AP | CD 3)**
   * **Ultimate 1 (Core — 🔵 Prep | CD 3) — Hyperflux Overdrive [🛡️ Scudo / 💨 Aria]:** Sia Phase sia l'alleato collegato guadagnano **+2 AP** nel turno successivo senza *Phase Lock*, -1 turno ai cooldown e +1 movimento.
   * **Ultimate 2 (Core — 🔴 Blast | CD 3) — Void Singularity [🌌 Flux]:** Detona l'intera linea del *Link* per 310 Danno Energetico lungo il vettore e adiacenze, applicando *Blind* e *Root*.
   * **Ultimate 3 (Variante — 🟡 Dash | CD 3) — Glacial Extraction [❄️ Freddo]:** Estrazione a 7 esagoni (ignora muri e Asse Z): teletrasporta l'alleato accanto a Phase con 300 HP di scudo cristallino congelando il punto di partenza.
6. **Reaction Phase (🟣 Reaction Phase | 1–2 AP)**
   * **Opzione 1 (Core — Guard | 2 AP | CD 2) — Emergency Pull [🛡️ Scudo]:** Se l'alleato collegato è bersagliato da un colpo *Blast* o AoE, lo estrae di 2 esagoni verso Phase prima del danno applicando 150 HP di scudo.
   * **Opzione 2 (Core — Counter | 1 AP) — Blinding Flash [🌌 Flux]:** Se Phase o l'alleato subiscono un colpo melee, acceca (*Blind*) l'attaccante e deposita *Flux*.
   * **Opzione 3 (Variante — Evasion | 1 AP | CD 2) — Neutral Wave [❄️ Freddo]:** Se bersagliata da un proiettile Ranged, arretra di 1 esagono lasciando *Freddo* e deviando il tiro successivo.

#### Matrice Combo, Sinergie e Matchup
| Nome Combo | Elementi (9 Elementi) | Fase di Chiusura | Effetto Deterministico |
| :--- | :--- | :--- | :--- |
| **Legame Cristallino** | 🛡️ Scudo + ❄️ Freddo | 🔵 Prep Phase | Alleato collegato immune a *Knockback*, *Pull* e *Stun* per 1 turno. |
| **Eco Psionico** | 🛡️ Scudo + 🌌 Flux | 🔴 Blast Phase | I danni dell'alleato collegato generano rigenerazione condivisa sul *Link*. |
| **Onda Neutrale** | 🌌 Flux + ❄️ Freddo | 🟡 Dash Phase | Respinge proiettili aerei e purifica le cariche elementali ostili. |
| **Sinfonia del Vuoto** | 🛡️ Scudo + ❄️ Freddo + 🌌 Flux | 🔴 Blast Phase | Detonazione sul *Link*: applica *Blind* + *Root* e ripristina **+2 AP** all'alleato. |
| **Sospensione Flux** | 🌌 Flux + ❄️ Freddo + 🛡️ Scudo | 🔵 Prep Phase | Solleva l'alleato a $Z+1$ ignorando trappole e ostacoli al suolo. |

* **Sinergie Ottimali:** **Crunch, Feng Mao & Murdock** (+2 AP per catene offensive ed estrazione d'emergenza).
* **Matchup:** Domina contro **Riktor & Sevarog** (*Legame Cristallino* annulla *Chain Pull* e *Colossal Blow*). Soffre **Kallari & Drongo** (isolarla o silenziarla spegne simultaneamente entrambi i membri del *Link*).

---

### ❄️ 10. AURORA — *Controller / Dominatrice Glaciale*
Specialista dell'inganno geometrico e del controllo dei grafi di navigazione: brucia gli *Overwatch* nemici tramite simulacri di ghiaccio e scolpisce muri e ponti glaciali sull'Asse Z.

* **Statistiche Base:** 1250 HP | DEF Fisica: 30 ($\approx 23\%$ rid.) | RES Energetica: 25 ($\approx 20\%$ rid.) | Movimento: 3 esagoni | Gittata Base: 1 esagono.
* **Affinità Elementali:** ❄️ **Freddo** *(assorbe Ghiaccio)*, 🌌 **Flux** *(ex Arcano)*, 🛡️ **Scudo**, 🪨 **Terra**.

#### Catalogo Abilità (6 Slot $\times$ 3 Opzioni)
1. **Attacco Base (🔴 Blast Phase | 2 AP | 0 CD)**
   * **Opzione 1 (Core) — Rime Blade [❄️ Freddo]:** Fendente glaciale su 2 spicchi frontali da 160 Danno Fisico/Energetico che deposita *Freddo*.
   * **Opzione 2 (Sfondamento) — Shatter Strike [🪨 Terra]:** Affondo da 150 Danno Fisico (+60 bonus su bersagli congelati/radicati) che deposita *Terra*.
   * **Opzione 3 (Parata) — Frost Guard Cut [🛡️ Scudo]:** Colpo da 130 Danno Fisico che deposita *Scudo* e concede 80 HP di scudo personale.
2. **Prep Phase (🔵 Prep Phase | 1 AP | CD 1–2)**
   * **Opzione 1 (Core) — Frozen Simulacrum [❄️ Freddo]:** Arretra di 1 esagono lasciando un'esca glaciale (180 HP) che blocca LoS, assorbe colpi letali e brucia gli *Overwatch*.
   * **Opzione 2 (Rifrazione) — Mirror Decoy [🌌 Flux]:** Simulacro da 120 HP caricato di *Flux* che riflette il 50% del danno subito sull'attaccante.
   * **Opzione 3 (Guscio) — Permafrost Shell [🛡️ Scudo]:** Barriera cristallina da 200 HP per 1 turno che deposita *Scudo*.
3. **Dash Phase (🟡 Dash Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Glacial Charge [❄️ Freddo]:** Scivolamento di 3 esagoni capace di guadare dislivelli sull'Asse Z ($Z=0 \rightarrow Z+1$) erigendo dietro di sé un muro di ghiaccio (160 HP/segmento, 2 turni).
   * **Opzione 2 (Scorrimento) — Ice Spike Slide [🪨 Terra]:** Scatto di 2 esagoni che lascia spuntoni (*Terreno Sconnesso*, 70 Danni Fisici) depositando *Terra*.
   * **Opzione 3 (Spettrale) — Glacier Phase [🌌 Flux]:** Balzo di 2 esagoni attraverso coperture leggere che deposita *Flux* all'arrivo.
4. **Abilità Speciale / Controllo (🔴 Blast Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Hoarfrost [❄️ Freddo]:** Anello radiale (tutti gli esagoni adiacenti) da 180 Danno Energetico che applica *Root* bloccando i fiancheggiatori melee.
   * **Opzione 2 (Stasi) — Crystalline Prison [🛡️ Scudo]:** Bolla inerte entro 2 esagoni (120 Danno): impedisce al bersaglio di agire fino al turno successivo e deposita *Scudo*.
   * **Opzione 3 (Spaccatura) — Permafrost Rift [🪨 Terra]:** Linea di 2 esagoni (170 Danno Fisico) che frantuma muri di ghiaccio pre-esistenti infliggendo +100 Danni ad area agli adiacenti.
5. **Ultimate (3 AP | CD 3)**
   * **Ultimate 1 (Core — 🔴 Blast | CD 3) — Cryoseism [🌌 Flux]:** Congelamento di massa su area 3x3 (280 Danno + *Root*) con detonazione a catena secondaria da +140 Danni per ogni modello adiacente congelato.
   * **Ultimate 2 (Variante — 🔵 Prep | CD 3) — Glacial Bastion [❄️ Freddo]:** Innalza 3 pareti di ghiaccio da 250 HP attorno a un esagono saturando l'area 3x3 di *Freddo* per 2 turni.
   * **Ultimate 3 (Variante — 🟡 Dash | CD 3) — Glacial Banshee [🛡️ Scudo]:** Scatto perforante di 4 esagoni (260 Danno Fisico) che azzera la DEF dei nemici attraversati fino a fine *Blast Phase*.
6. **Reaction Phase (🟣 Reaction Phase | 1–2 AP)**
   * **Opzione 1 (Core — Evasion | 1 AP | CD 2) — Ice Clone Decoy [❄️ Freddo]:** Si sposta di 1 esagono lasciando un mini-simulacro che assorbe integralmente il colpo *Blast* o *Overwatch*.
   * **Opzione 2 (Core — Counter | 2 AP | CD 2) — Mirror Shatter [🌌 Flux]:** Riflette il 50% del danno subito da sé o dal simulacro sull'attaccante applicando *Flux*.
   * **Opzione 3 (Variante — Guard | 1 AP) — Cryo-Stasis Cocoon [🛡️ Scudo]:** Di fronte a un colpo letale si chiude in stasi azzerando il danno (0 azioni residue nel turno).

#### Matrice Combo, Sinergie e Matchup
| Nome Combo | Elementi (9 Elementi) | Fase di Chiusura | Effetto Deterministico |
| :--- | :--- | :--- | :--- |
| **Specchio Ingannevole** | ❄️ Freddo + 🌌 Flux | 🔵 Prep Phase | Il simulacro riflette il 50% dei danni sull'attaccante. |
| **Pista Fratturata** | ❄️ Freddo + 🪨 Terra | 🟡 Dash Phase | Il muro di ghiaccio si scompone in terreno spinato dannoso. |
| **Stasi Assoluta** | ❄️ Freddo + 🛡️ Scudo | 🔴 Blast Phase | Bolla inerte che blocca sia azioni sia danni. |
| **Era Glaciale** | ❄️ Freddo + 🌌 Flux + 🪨 Terra | 🔴 Blast Phase | Esplosione a catena che ignora le resistenze sui bersagli congelati. |
| **Banshee di Ghiaccio** | ❄️ Freddo + 🛡️ Scudo + 🌌 Flux | 🟡 Dash Phase | Scatto perforante che frantuma l'armatura al contatto. |

* **Sinergie Ottimali:** **The Fey, Muriel & Steel** (*Terra* e *Freddo* per chiudere *Era Glaciale*) e **Gadget** (*Hoarfrost* blocca i nemici nella *Sticky Bomb*).
* **Matchup:** Domina contro **formazioni compatte e flanker melee** (*Feng Mao*). Soffre **Murdock & Feng Mao** con combinazioni *Fuoco + Disintegrazione* che sciolgono rapidamente le strutture di ghiaccio.

---

### 💀 11. SEVAROG — *Scaling Tank / Mietitore Oscuro*
Tank da medio-lungo termine che accumula **Stack di Anime** deterministici con *Siphon* per incrementare gli HP massimi (+40 HP Max per colpo su eroe, +80 su eliminazione, cap +400 HP) e scaraventa i bersagli contro muri o giù dall'Asse Z.

* **Statistiche Base:** 1650 HP *(fino a 2050 HP con Stack)* | DEF Fisica: 45 ($\approx 31\%$ rid.) | RES Energetica: 35 ($\approx 26\%$ rid.) | Movimento: 3 esagoni | Gittata Base: 1 esagono.
* **Affinità Elementali:** ⚛️ **Disintegrazione** *(ex 💀 Morte)*, 🪨 **Terra** *(assorbe Forza)*, 💨 **Aria**.

#### Catalogo Abilità (6 Slot $\times$ 3 Opzioni)
1. **Attacco Base (🔴 Blast Phase | 2 AP | 0 CD)**
   * **Opzione 1 (Core) — Siphon [⚛️ Disintegrazione]:** Colpo di falce (1–2 esagoni) da 160 Danno Energetico (con danno puro su fragili) che assegna Stack di Anime (+40 HP Max / +80 su kill).
   * **Opzione 2 (Spazzata) — Reaper's Cleave [💨 Aria]:** Falciata su 3 spicchi frontali ($90^\circ$) da 130 Danno Fisico che deposita *Aria* e ruota il *Facing* nemico di $30^\circ$.
   * **Opzione 3 (Frantumazione) — Grave Hammer [🪨 Terra]:** Impatto da 150 Danno Fisico che deposita *Terra* e riduce di 1 esagono il movimento nemico successivo.
2. **Prep Phase (🔵 Prep Phase | 1 AP | CD 1–2)**
   * **Opzione 1 (Core) — Consuming Shroud [⚛️ Disintegrazione]:** Miasma sull'esagono per 2 turni che deposita *Disintegrazione* e prepara *Miasma Terrestre* / *Vuoto Consumante*.
   * **Opzione 2 (Ancoraggio) — Dread Ground [🪨 Terra]:** Radici su un esagono entro 2 celle (*Terra* 2 turni) che vietano l'Overclock ai nemici al suo interno.
   * **Opzione 3 (Velo) — Spectral Mantle [💨 Aria]:** Applica *Aria* concedendo 140 HP di barriera spettrale e +1 movimento nella *Dash Phase* corrente.
3. **Dash Phase (🟡 Dash Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Phantom Rush [💨 Aria]:** Scatto spettrale intangibile di 3 esagoni che attraversa unità solide (ignora *Body Block*), infligge 110 Danno agli attraversati e deposita *Aria*.
   * **Opzione 2 (Inseguimento) — Soul Tether Dash [⚛️ Disintegrazione]:** Avanzata di 2 esagoni che applica *Disintegrazione* al primo nemico superato drenandogli -1 AP se la cella è carica di *Aria*.
   * **Opzione 3 (Avanzata) — Juggernaut March [🪨 Terra]:** Carica di 2 esagoni immune al *Terreno Sconnesso* che applica *Terra* e spinge di $30^\circ$ i nemici laterali.
4. **Abilità Speciale / Controllo (🔴 Blast Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Subjugate [🪨 Terra]:** Radici oscure a distanza (4 esagoni) da 170 Danno che applicano *Terra* e **Root** (setup per *Colossal Blow*).
   * **Opzione 2 (Mietitura) — Soul Harvest [⚛️ Disintegrazione]:** Cono frontale di $60^\circ$ da 190 Danno Perforante anti-cura (2 turni); raddoppia gli Stack di Anime sui bersagli radicati.
   * **Opzione 3 (Risacca) — Wraith Pull [💨 Aria]:** Vortice a 3 esagoni (150 Danno) che attira il bersaglio di 1 esagono verso Sevarog applicando *Aria*.
5. **Ultimate (3 AP | CD 3)**
   * **Ultimate 1 (Core — 🔴 Blast | CD 3) — Colossal Blow [🪨 Terra]:** Spazzata da 280 Danno Fisico che scaglia i nemici in linea retta fino a **3 esagoni** ("Home run"); impattare contro un muro o cadere sull'Asse Z infligge +200 Danni Puri (+2 Stack se letale).
   * **Ultimate 2 (Variante — 🔵 Prep | CD 3) — Consuming Void [⚛️ Disintegrazione]:** Cupola 3x3 (1 turno) che assorbe i proiettili balistici nemici convertendo ogni colpo intercettato in +1 Stack di Anime.
   * **Ultimate 3 (Variante — 🔴 Blast | CD 3) — Reaper's Toll [💨 Aria]:** Scatto di 2 esagoni + falciata a $360^\circ$ da 310 Danno Energetico che applica *Aria* e drena -1 AP ai colpiti.
6. **Reaction Phase (🟣 Reaction Phase | 1–2 AP)**
   * **Opzione 1 (Core — Guard | 2 AP | CD 2) — Soul Aegis [🪨 Terra]:** Mitiga del 50% il danno *Blast*, garantisce immunità agli sbalzi e deposita *Terra*.
   * **Opzione 2 (Core — Counter | 1 AP) — Spectral Retaliation [⚛️ Disintegrazione]:** Se colpito in mischia, risponde con 120 Danno Perforante anti-cura e +1 Stack di Anime.
   * **Opzione 3 (Variante — Evasion | 1 AP | CD 2) — Ethereal Shift [💨 Aria]:** Scivola intangibile di 1 esagono attraverso unità adiacenti evitando il colpo e lasciando *Aria*.

#### Matrice Combo, Sinergie e Matchup
| Nome Combo | Elementi (9 Elementi) | Fase di Chiusura | Effetto Deterministico |
| :--- | :--- | :--- | :--- |
| **Miasma Terrestre** | ⚛️ Disint. + 🪨 Terra | 🔵 Prep Phase | Esagono necrotico che impedisce qualsiasi cura. |
| **Catena Spettrale** | ⚛️ Disint. + 💨 Aria | 🟡 Dash Phase | Applica *Root* e ruba -1 AP nel turno successivo. |
| **Sussulto d'Ombra** | 💨 Aria + 🪨 Terra | 🔴 Blast Phase | Sbalza disorientando e impedendo il coordinamento difensivo. |
| **Giudizio del Mietitore** | ⚛️ Disint. + 🪨 Terra + 💨 Aria | 🔴 Blast Phase | Se il bersaglio sbalzato urta un muro, subisce danno puro letale all'anima. |
| **Vuoto Consumante** | ⚛️ Disint. + 💨 Aria + 🪨 Terra | 🔵 Prep Phase | Assorbe proiettili balistici convertendoli in stack permanenti. |

* **Sinergie Ottimali:** **Steel** (erige muri contro cui lanciare nemici), **Kallari** (*Lama Sottovuoto* raddoppia lo sbalzo di *Colossal Blow*) e **Riktor**.
* **Matchup:** Domina contro **Tank stazionari e Healer**. Soffre **Phase** (*Legame Cristallino* annulla *Colossal Blow*), **The Fey** (blocchi a terra) e **Steel** (*Faglia Ionizzata* anti-*Aria*).

---

### 🌪️ 12. YIN — *Duellante Anti-Ranged / Dominatrice del Vento*
Calcolatrice balistica specializzata nel punire i tiratori: estende la portata della frusta fino a 4 esagoni e rispedisce i proiettili avversari al mittente con *Backlash*.

* **Statistiche Base:** 1150 HP | DEF Fisica: 25 ($\approx 20\%$ rid.) | RES Energetica: 25 ($\approx 20\%$ rid.) | Movimento: 3 esagoni | Gittata Base: 2 esagoni *(4 con Windburn)*.
* **Affinità Elementali:** 💨 **Aria** (Primario), 🌌 **Flux** *(ex Arcano)*, 🛡️ **Scudo**, 🪨 **Terra**.

#### Catalogo Abilità (6 Slot $\times$ 3 Opzioni)
1. **Attacco Base (🔴 Blast Phase | 2 AP | 0 CD)**
   * **Opzione 1 (Core) — Lash Out [💨 Aria]:** Frustata a media portata (2 esagoni) da 160 Danno Fisico con *Cleave* su 2 spicchi frontali, applicando *Aria*.
   * **Opzione 2 (Perforazione) — Photonic Lash [🌌 Flux]:** Colpo rettilineo da 140 Danno Energetico che ignora coperture leggere e deposita *Flux*.
   * **Opzione 3 (Sbarramento) — Gale Whip [🛡️ Scudo]:** Schiocco da 130 Danno Fisico che deposita *Scudo* concedendo 90 HP di scudo anti-Ranged frontale.
2. **Prep Phase (🔵 Prep Phase | 1 AP | CD 1–2)**
   * **Opzione 1 (Core) — Windburn [🌌 Flux]:** Estende per 2 turni la gittata della frusta di +2 esagoni (fino a 4), aggiunge +30 danno AoE e attiva -25% danno Ranged frontale.
   * **Opzione 2 (Barriera) — Zephyr Wall [🛡️ Scudo]:** Parete di vento frontale (160 HP, 1 turno) che deposita *Scudo*, rallenta i nemici e ferma proiettili fisici.
   * **Opzione 3 (Ancoraggio) — Grounding Knot [🪨 Terra]:** Deposita *Terra* sull'esagono di Yin con immunità agli sbalzi per 1 turno.
3. **Dash Phase (🟡 Dash Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Rift Surge [💨 Aria]:** Aggancia bersaglio o pilastro entro 4 esagoni chiudendo istantaneamente la distanza (130 Danno Fisico), depositando *Aria* e riallineando il *Facing*.
   * **Opzione 2 (Vortice) — Cyclone Vault [🪨 Terra]:** Balzo di 2 esagoni che genera un micro-uragano (raggio 1) attirando i nemici verso il centro e applicando *Terra*.
   * **Opzione 3 (Rifrattivo) — Slipstream Step [🌌 Flux]:** Scatto laterale di 2 esagoni che ignora *Body Block* lasciando *Flux* in partenza.
4. **Abilità Speciale / Controllo (🔴 Blast Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Sky Rend [💨 Aria]:** Frustata ad arco di $120^\circ$ (190 Danno Fisico) che deposita *Aria* e solleva a $Z+1$ i bersagli affetti da *Terra* o *Flux*.
   * **Opzione 2 (Arcana) — Arcane Crack [🌌 Flux]:** Schiocco a 3 esagoni (180 Danno Energetico) che attraversa ostacoli leggeri e silenzia le *Reaction* fino a fine turno.
   * **Opzione 3 (Tellurica) — Dust Devil Sweep [🪨 Terra]:** Linea di 2 esagoni (170 Danno Fisico) che applica *Terra* e ruota il *Facing* nemico di $60^\circ$.
5. **Ultimate (2–3 AP | CD 3)**
   * **Ultimate 1 (Core — 🔵 Prep | 3 AP | CD 3) — Quelling Gale [💨 Aria]:** Cupola ciclonica (raggio 2 esagoni, 1 turno) che sospende a mezz'aria i proiettili balistici nemici; attivando *Backlash* li riflette tutti simultaneamente con +30% danno.
   * **Ultimate 2 (Core — 🔴 Blast | 3 AP | CD 3) — Heavensplitter [🌌 Flux]:** Cono frontale (3 esagoni) da 320 Danno che scaglia i bersagli a $Z+2$, stordendoli (*Stun* 1 turno) alla caduta.
   * **Ultimate 3 (Variante — 🟣 Reaction | 2 AP | CD 3) — Inverted Void [🛡️ Scudo]:** Riflette anche proiettili o raggi *Ultimate* aggiungendo +120 Danni Energetici e applicando *Silence* per 1 turno.
6. **Reaction Phase (🟣 Reaction Phase | 1–2 AP)**
   * **Opzione 1 (Core — Counter | 1–2 AP | CD 1) — Backlash [💨 Aria]:** Quando un proiettile Ranged entra nel cono frontale ($180^\circ$), ne inverte la traiettoria rispedendolo al mittente al 100% del danno (+25% con *Windburn*).
   * **Opzione 2 (Variante — Guard | 1 AP) — Wind Barrier [🛡️ Scudo]:** Raffica sui 3 spicchi frontali ($90^\circ$) che riduce del 60% il danno frontale e deposita *Scudo*.
   * **Opzione 3 (Variante — Evasion | 1 AP | CD 2) — Gale Pivot [🌌 Flux]:** In risposta a un colpo melee diretto, ruota di 1 esagono fuori dal cono frontale dell'attaccante lasciando *Flux*.

#### Matrice Combo, Sinergie e Matchup
| Nome Combo | Elementi (9 Elementi) | Fase di Chiusura | Effetto Deterministico |
| :--- | :--- | :--- | :--- |
| **Frusta Fotonica** | 💨 Aria + 🌌 Flux | 🔴 Blast Phase | L'attacco base ignora ostacoli ambientali leggeri. |
| **Ciclone Devastante** | 💨 Aria + 🪨 Terra | 🟡 Dash Phase | Micro-uragano che attira i nemici verso il centro dell'esagono. |
| **Barriera Eolica** | 💨 Aria + 🛡️ Scudo | 🔵 Prep Phase | Muro di vento che rallenta e arresta proiettili fisici. |
| **Vuoto Invertito** | 💨 Aria + 🌌 Flux + 🛡️ Scudo | 🟣 Reaction Phase | Riflette il proiettile con danni energetici bonus e *Silence* sul mittente. |
| **Squarcio nel Cielo** | 💨 Aria + 🪨 Terra + 🌌 Flux | 🔴 Blast Phase | Sbalza i nemici sull'Asse Z stordendoli all'impatto della caduta. |

* **Sinergie Ottimali:** **Muriel & Steel** (forniscono *Scudo* e *Terra* per *Vuoto Invertito* e *Squarcio nel Cielo*) e **Gadget** (*Flux*).
* **Matchup:** Counter naturale di **Murdock, Drongo & Gadget** (riflette proiettili e *Rad Rounds*). Soffre **Steel & Riktor** (attacchi diretti non riflettibili e *Faglia Ionizzata* anti-*Aria*).

---

### 🥊 13. CRUNCH — *Combo-Brawler / Macchina da Pugilato*
Combattente da mischia nativamente allineato ai 9 Elementi Ufficiali, costruito sulla passiva **Empowered**: ogni terza abilità eseguita nella sequenza ($0 \rightarrow 1 \rightarrow 2 \rightarrow \text{Empowered}$) innesca la propria versione potenziata.

* **Statistiche Base:** 1450 HP | DEF Fisica: 40 ($\approx 29\%$ rid.) | RES Energetica: 20 ($\approx 17\%$ rid.) | Movimento: 3 esagoni | Gittata Base: 1 esagono.
* **Affinità Elementali:** 🪨 **Terra**, 🔥 **Fuoco**, 💨 **Aria**, 🌌 **Flux**.

#### Catalogo Abilità (6 Slot $\times$ 3 Opzioni)
1. **Attacco Base (🔴 Blast Phase | 1–2 AP | 0 CD)**
   * **Opzione 1 (Core — 2 AP) — Left Crunch [🪨 Terra]:** Pugno sinistro con *Cleave* su 3 spicchi frontali ($90^\circ$) da 160 Danno Fisico che deposita *Terra*. *(Empowered: +100% danno Cleave + Stun 1 turno sul bersaglio centrale).*
   * **Opzione 2 (Jab Rapido — 1 AP) — Piston Jab [🔥 Fuoco]:** Diretto a 1 AP da 90 Danno Fisico/Termico su singolo spicchio che deposita *Fuoco* e accelera il ciclo *Empowered*.
   * **Opzione 3 (Angolare — 2 AP) — Pneumatic Hook [💨 Aria]:** Gancio da 140 Danno Fisico che applica *Aria* e ruota il *Facing* nemico di $60^\circ$.
2. **Prep Phase (🔵 Prep Phase | 1 AP | CD 1–2)**
   * **Opzione 1 (Core) — Piston Overdrive [🌌 Flux]:** Deposita *Flux* sull'esagono riducendo di -1 AP il costo della prossima abilità *Blast* potenziata (prepara *Overdrive Meccanico*).
   * **Opzione 2 (Termica) — Thermal Vent [🔥 Fuoco]:** Applica *Fuoco* sul proprio esagono (2 turni) caricando il pugno successivo con +50 Danno Termico contro barriere/scudi.
   * **Opzione 3 (Assetto) — Hydraulic Brace [🪨 Terra]:** Applica *Terra* con immunità a *Knockback* (1 turno) e +1 carica immediata al contatore *Empowered*.
3. **Dash Phase (🟡 Dash Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Forward Crunch [💨 Aria]:** Carica lineare di 3 esagoni (150 Danno Fisico) che trascina il primo nemico lungo la traiettoria depositando *Aria*. *(Empowered: carica di 4 esagoni + Stun se impatta contro muro o modello).*
   * **Opzione 2 (Fusa) — Afterburner Dash [🔥 Fuoco]:** Scatto di 2 esagoni con scia infuocata da 80 Danno/turno (2 turni) e deposito *Fuoco*.
   * **Opzione 3 (Tellurica) — Bulldozer Rush [🪨 Terra]:** Avanzata di 2 esagoni che demolisce coperture leggere sul percorso depositando *Terra*.
4. **Abilità Speciale / Controllo (🔴 Blast Phase | 2 AP | CD 1–2)**
   * **Opzione 1 (Core) — Right Crunch [🔥 Fuoco]:** Montante termico da 190 Danno Fisico/Fuoco che applica *Fuoco* e sbalza il bersaglio indietro di 1 esagono. *(Empowered: montante sismico che lancia a $Z+1$ con Stun 1 turno).*
   * **Opzione 2 (Frantuma-Armatura) — Meltdown Uppercut [🪨 Terra]:** Colpo da 170 Danno Fisico che deposita *Terra* e applica -15 DEF Fisica per 2 turni.
   * **Opzione 3 (Compressione) — Concussive Cross [💨 Aria]:** Cono di $60^\circ$ da 160 Danno Fisico che applica *Aria* e **Dazed** (no *Reaction*).
5. **Ultimate (3 AP | CD 2–3)**
   * **Ultimate 1 (Core — 🔴 Blast | CD 2) — Re-Crunch [🌌 Flux]:** Duplica istantaneamente l'ultima abilità usata (inclusa la variante *Empowered*), riscrive i cooldown interni e deposita *Flux*.
   * **Ultimate 2 (Variante — 🔴 Blast | CD 3) — Piston Detonation [🔥 Fuoco]:** Esplosione AoE pura a $360^\circ$ (raggio 1) da 360 Danni Perforanti che polverizza coperture leggere.
   * **Ultimate 3 (Variante — 🔵 Prep | CD 3) — Full Overclock Core [🪨 Terra]:** Porta subito *Empowered* a 2/3, deposita *Terra* e conferisce +40% danno contro bersagli corazzati/scudati nei prossimi 2 colpi *Blast*.
6. **Reaction Phase (🟣 Reaction Phase | 1–2 AP)**
   * **Opzione 1 (Core — Counter | 2 AP | CD 2) — Cross Counter [🪨 Terra]:** Mitiga del 40% un colpo melee in arrivo e risponde con 170 Danno Fisico depositando *Terra* (+1 carica *Empowered*).
   * **Opzione 2 (Variante — Guard | 1 AP) — Thermal Guard [🔥 Fuoco]:** Riduce del 50% il danno sui 3 spicchi frontali infliggendo 70 Danno Fuoco di ritorno in mischia.
   * **Opzione 3 (Variante — Evasion | 1 AP | CD 2) — Thruster Slip [💨 Aria]:** Scivola lateralmente di 1 esagono evitando proiettili o AoE frontali e lasciando *Aria*.

#### Matrice Combo, Sinergie e Matchup
| Nome Combo | Elementi (9 Elementi) | Fase di Chiusura | Effetto Deterministico |
| :--- | :--- | :--- | :--- |
| **Montante Sismico** | 🪨 Terra + 💨 Aria | 🔴 Blast Phase | Sbalza il bersaglio a $Z+1$ esponendolo a danni da caduta. |
| **Attrito Termico** | 🪨 Terra + 🔥 Fuoco | 🔴 Blast Phase | Fonde la difesa bersaglio applicando **-10 DEF a colpo**. |
| **Propulsione Fusa** | 💨 Aria + 🔥 Fuoco | 🟡 Dash Phase | Getto di fiamme persistente lungo la scia dello scatto. |
| **Esplosione a Pistoni** | 🪨 Terra + 💨 Aria + 🔥 Fuoco | 🔴 Blast Phase | Converte gli stack fisici in un'esplosione AoE pura. |
| **Overdrive Meccanico** | 🔥 Fuoco + 🌌 Flux + 🪨 Terra | 🔵 Prep Phase | Azzera il costo AP del prossimo attacco combo surriscaldando l'esagono. |

* **Sinergie Ottimali:** **Phase** (rigenera AP per catene multiple senza *Phase Lock*), **Gadget** (fornisce *Flux* per *Overdrive Meccanico*) e **Feng Mao / Drongo** (asse *Fuoco + Terra*).
* **Matchup:** Domina contro **Steel & Sevarog** (fonde alte armature). Soffre **Muriel & Gadget** (il drenaggio AP di *Frost Binding* / *Seeker Bot* / *Tesla Dome* spezza la catena da 3 colpi).

---

### ☢️ 14. DRONGO — *Sabotatore Scavenger / Attrito & Silence*
Sabotatore asimmetrico progettato per spegnere le abilità e le *Reaction* avversarie tramite nubi silenzianti e logorare le difese con munizioni radioattive a scoppio ritardato.

* **Statistiche Base:** 1100 HP | DEF Fisica: 20 ($\approx 17\%$ rid.) | RES Energetica: 20 ($\approx 17\%$ rid.) | Movimento: 3 esagoni | Gittata Base: 5–6 esagoni.
* **Affinità Elementali:** 🔥 **Fuoco**, 💨 **Aria**, 🪨 **Terra**, ⚛️ **Disintegrazione** *(ex 💀 Morte)*.

#### Catalogo Abilità (6 Slot $\times$ 3 Opzioni)
1. **Attacco Base (🔴 Blast Phase | 2 AP | 0 CD)**
   * **Opzione 1 (Core) — Rad Rounds [🔥 Fuoco]:** Proiettili all'uranio (5 esagoni) da 150 Danno Energetico/Termico con penetrazione passiva; ogni colpo applica 1 stack radioattivo (40 DoT/turno; detona per +90 danni a 2 stack).
   * **Opzione 2 (Corta) — Scrap Buckshot [🪨 Terra]:** Rosa a 3 esagoni (2 spicchi frontali) da 140 Danno Fisico che deposita *Terra*.
   * **Opzione 3 (Corrosiva) — Decay Slug [⚛️ Disintegrazione]:** Proiettile da 130 Danno Energetico che applica *Disintegrazione* (+60 danni bonus su scudi temporanei).
2. **Prep Phase (🔵 Prep Phase | 1 AP | CD 1–2)**
   * **Opzione 1 (Core) — Gag Grenade [⚛️ Disintegrazione]:** Granata a gas (gittata 4 esagoni, cella + 3 spicchi, 1 turno) che genera una zona di **Silence** negando abilità attive e *Reaction* nemiche.
   * **Opzione 2 (Innesco) — Uranium Primer [🔥 Fuoco]:** Deposita *Fuoco* sulla propria cella; il prossimo colpo *Blast* applica subito 2 stack di *Rad Rounds* ignorando coperture leggere.
   * **Opzione 3 (Mina) — Junkyard Mine [🪨 Terra]:** Carica su esagono adiacente (2 turni) che deposita *Terra* e infligge 110 Danno Fisico in *Dash Phase*.
3. **Dash Phase (🟡 Dash Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Old Rusty [💨 Aria]:** Scatto laterale di 1 esagono + boomerang lungo 4 esagoni (100 Danno andata + 100 ritorno) che applica *Aria* e *Slow* (+1 AP movimento).
   * **Opzione 2 (Propulsivo) — Dirty Thruster [🔥 Fuoco]:** Balzo indietro di 2 esagoni lasciando una carica incendiaria istantanea (130 Danno Fuoco) in partenza.
   * **Opzione 3 (Schegge) — Caltrop Roll [🪨 Terra]:** Rotolata di 2 esagoni che trasforma la cella d'origine in *Terreno Sconnesso* depositando *Terra*.
4. **Abilità Speciale / Controllo (🔴 Blast Phase | 2 AP | CD 2)**
   * **Opzione 1 (Core) — Isotope Detonator [🔥 Fuoco]:** Impulso a 6 esagoni (160 Danno Fuoco) che fa esplodere subito tutti gli stack di *Rad Rounds* raddoppiandone il danno da scoppio (anti-cura con *Disintegrazione*).
   * **Opzione 2 (Tossica) — Blight Canister [⚛️ Disintegrazione]:** Gas su area 2x2 (150 Danno Perforante) che applica *Disintegrazione* e spegne *Guard* e *Counter*.
   * **Opzione 3 (Curva) — Boomerang Arc [💨 Aria]:** Lancio curvo a 5 esagoni (170 Danno Fisico) che aggira una copertura leggera depositando *Aria* e ruotando il *Facing* di $30^\circ$.
5. **Ultimate (3 AP | CD 3)**
   * **Ultimate 1 (Core — 🔴 Blast | CD 3) — Shrapnel Cannon [🪨 Terra]:** Sventagliata ravvicinata ($90^\circ$, profondità 3 esagoni) da 340 Danno Fisico che applica **-25 DEF** (2 turni), deposita *Terra* e sbalza Drongo indietro di 2 esagoni.
   * **Ultimate 2 (Variante — 🔴 Blast | CD 3) — Fallout Barrage [⚛️ Disintegrazione]:** Scarica su area 3x3 (300 Danno Perforante) che demolisce muri leggeri/medi lasciando gas radioattivo (90 DoT/turno + blocco cure per 2 turni).
   * **Ultimate 3 (Variante — 🔵 Prep | CD 3) — Reactor Breach [🔥 Fuoco]:** Nucleo su area 3x3 che satura di *Fuoco* ed esplode a fine *Blast Phase* per 350 Danni Termici incurabili.
6. **Reaction Phase (🟣 Reaction Phase | 1–2 AP)**
   * **Opzione 1 (Core — Evasion | 1–2 AP | CD 2) — Recoil Disengage [🪨 Terra]:** Se attaccato in mischia, spara a salve sbalzandosi indietro di 1 esagono (-40% danno subito) e depositando *Terra*.
   * **Opzione 2 (Core — Overwatch | 2 AP | CD 2) — Silence Trap Overwatch [⚛️ Disintegrazione]:** Intercetta il primo nemico in *Dash Phase* con una micro-*Gag Grenade* (120 Danni + *Silence* nella *Blast Phase* successiva).
   * **Opzione 3 (Variante — Counter | 1 AP) — Snap Rusty [💨 Aria]:** Risponde a un colpo a media distanza con un boomerang rapido (110 Danno Fisico + *Aria* + *Slow*).

#### Matrice Combo, Sinergie e Matchup
| Nome Combo | Elementi (9 Elementi) | Fase di Chiusura | Effetto Deterministico |
| :--- | :--- | :--- | :--- |
| **Vento Tossico** | 💨 Aria + ⚛️ Disint. | 🟡 Dash Phase | Il boomerang diffonde nubi silenzianti lungo l'intera traiettoria. |
| **Fusione Nucleare** | 🔥 Fuoco + ⚛️ Disint. | 🔴 Blast Phase | Esplosione incurabile con blocco totale delle cure. |
| **Schegge Infuocate** | 🪨 Terra + 🔥 Fuoco | 🔴 Blast Phase | Incendia le coperture adiacenti trasformandole in falò nocivi. |
| **Pioggia di Scorie** | 💨 Aria + ⚛️ Disint. + 🪨 Terra | 🔴 Blast Phase | Distrugge i muri fisici infettando l'area con gas radioattivo. |
| **Detonazione Sporca** | 🔥 Fuoco + ⚛️ Disint. + 🪨 Terra | 🟡 Dash Phase | Salto propulsivo che depone una mina nucleare a innesco istantaneo. |

* **Sinergie Ottimali:** **Feng Mao & Crunch** (asse *Fuoco + Terra* per *Schegge Infuocate* e *Attrito Termico*) e **Riktor** (trattiene i nemici dentro *Gag Grenade*).
* **Matchup:** Domina contro **Riktor, Gadget, Muriel, Phase & The Fey** (*Silence* preventivo in *Prep Phase* spegne ganci e *Reaction*). Soffre **Murdock** (gittata superiore) e **Yin** (riflette *Rad Rounds* e boomerang).

---

## 5. Riferimento Rapido: Matrice Alchemica Universale (Tier 2) e Progressione Orizzontale

### 5.1 Stati Composti Tier 2 (6 Elementi Base)
| + | 🔥 Fuoco | 💧 Acqua | 🪨 Terra | 💨 Aria | ⚡ Fulmine | ❄️ Freddo |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **🔥 Fuoco** | Hellfire (DoT incurabile) | **Vapore** (Blind, No LoS) | **Magma** (Costo AP $\times 2$) | **Tempesta Fuoco** (AoE+) | **Plasma** (-50% RES) | **Shock Termico** (0% DEF) |
| **💧 Acqua** | **Vapore** (Blind, No LoS) | Alluvione (Gittata $/ 2$) | **Fango** (Root / No Dash) | **Nebbia** (Schivata pass.) | **Acqua Elettr.** (Stun) | **Ghiaccio** (Root assoluto) |
| **🪨 Terra** | **Magma** (Costo AP $\times 2$) | **Fango** (Root / No Dash) | Sisma (Spostamento) | **Temp. Sabbia** (No React) | **Faglia Ioniz.** (Attira colpi) | **Permafrost** (Muro indistr.) |
| **💨 Aria** | **Tempesta Fuoco** (AoE+) | **Nebbia** (Schivata pass.) | **Temp. Sabbia** (No React) | Vuoto (Attira al centro) | **Tempesta Ionica** (Silence) | **Tormenta** (Slow + DoT) |
| **⚡ Fulmine** | **Plasma** (-50% RES) | **Acqua Elettr.** (Stun) | **Faglia Ioniz.** (Attira colpi) | **Tempesta Ionica** (Silence) | Sovraccarico (0 AP) | **Superconduttore** (Catena+) |
| **❄️ Freddo** | **Shock Termico** (0% DEF) | **Ghiaccio** (Root assoluto) | **Permafrost** (Muro indistr.) | **Tormenta** (Slow + DoT) | **Superconduttore** (Catena+) | Zero Assoluto (Stasi 2T) |

### 5.2 Progressione Orizzontale (Sidegrades senza Power Creep)
Per preservare l'equilibrio matematico deterministico, la maestria dell'eroe sblocca **varianti logiche** anziché aumenti statistici:
1. **Sintonizzazione Elementale (Liv. 3–5):** Sostituisce l'elemento di una skill (es. *Seeker Bot* di Gadget da ⚡ Fulmine a 🔥 Fuoco; *Conversion Shield* di Feng Mao da 🛡️ Scudo a 🌌 Flux).
2. **Moduli Overclock Personali (Liv. 6):** Alternative al *Phase Lock* standard per pagare i +2 AP:
   * *Credito a Lungo Termine:* Nessuna fase bloccata, ma rigenerazione base ridotta da 6 a 4 AP per 2 turni.
   * *Sovraccarico Biologico:* Nessuna fase bloccata, costo istantaneo del 35% degli HP massimi (danno puro non letale).
3. **Spostamento Temporale / Phase Shift (Liv. 10):** Sposta un'abilità in un'altra fase ribilanciandone AP/gittata (es. *Chain Pull* di Riktor in *Prep Phase* a 3 AP / raggio 3; *Seismic Slice* di Feng Mao in *Reaction Phase*).