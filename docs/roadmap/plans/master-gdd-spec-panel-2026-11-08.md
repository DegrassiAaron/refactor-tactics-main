# Spec panel — il «Master Game Design Document» del 2026-11-08 contro il contratto e contro le decisioni dello stesso giorno

> `SNAPSHOT` · **Scritto**: 2026-11-08 · **Input**: [`../../research/handoff/master-game-design-document-input-2026-11-08.md`](../../research/handoff/master-game-design-document-input-2026-11-08.md)
> (PDF di 17 pagine, creato alle 09:07 UTC, conservato accanto al Markdown) · **Richiesta dell'autore**: *«leggilo e integralo aggiornando la
> documentazione. Successivamente dovremo sistemare le issue attualmente aperte e crearne di nuove»* · **Esito**: [D-591](../../decisions/RT_PDR_00_Decision_Log.md)
> **Metodo**: lo stesso del referto precedente ([`master-spec-csharp-spec-panel-2026-11-07.md`](master-spec-csharp-spec-panel-2026-11-07.md)): ogni premessa del
> documento è misurata contro il contratto (Decision Log → ADR → spec owner → cataloghi → codice) e classificata; nessun conflitto è stato
> riconciliato per plausibilità ([`AGENTS.md`](../../../AGENTS.md) §2). La novità di questo referto è che l'autore ha **due** fonti proprie dello
> stesso giorno: il PDF e le decisioni `D-583`…`D-590` prese in sessione poche ore dopo. La precedenza fra le due l'ha decisa l'autore (§3).

## 1. Verdetto in breve

Il GDD è la seconda stesura della «Master Technical Specification» del 2026-11-07: stesso impianto (quattro fasi, 6 AP, dodici spicchi,
formula moltiplicativa, nomi Paragon), con tre differenze di sostanza (nove elementi in due tier con matrice di fusione al posto dei dodici,
Overclock con Phase Lock al posto del settimo AP, Flanking che ignora metà DEF) e molto contenuto nuovo (kit completi degli otto eroi,
dodici archetipi di muro con reazione al Flux, modalità «Core Refactor», progressione a sidegrade, grammatica HUD, architettura dei bot).

**Dove diverge dalle decisioni dello stesso giorno, non governa**: l'autore ha scelto *«Le decisioni di oggi governano; il GDD entra dove non
confligge»*. **Dove non confligge, è input di design**: sostituisce la specifica del 2026-11-07 come input per i kit (`MSP-5.d`), le statistiche
(`MSP-4.n`) e i costi AP (`MSP-1.n`); la matrice di fusione alimenta le 66 coppie (`MSP-3.c`); la grammatica HUD entra nei riferimenti di E52.1
e nella progettazione HUD; l'architettura dei bot è input dichiarato per E26. La modalità «Core Refactor» resta ricerca con una voce aperta.

## 2. Mappa delle premesse

Legenda: **CONFERMA** = coincide col contratto · **CONCORDE CON OGGI** = coincide con una decisione di `D-583`…`D-590` · **SUPERATO (D-591)** =
diverge da una decisione dello stesso giorno, e l'autore ha fatto prevalere la decisione · **INPUT** = entra come materiale di design dove
una istruttoria è aperta · **RESEARCH** = si conserva, nessuna sede lo riceve oggi · **ASSENTE** = nulla nel repository lo descrive.

| § GDD | Premessa | Classe | Fonte che governa, e misura |
|---|---|---|---|
| 1 | tattico competitivo a turni simultanei, 0% RNG, hit garantito in LoS | **CONFERMA** | `AGENTS.md` §1, CLAUDE.md §7 (determinismo); la precisione è già 100% nel resolver |
| 1 | formato 3v3 o 4v4 | parziale | 3v3 è lo Standard (`D-256`); 4v4 è stress (`spec-durata-partita-e-scala-mappe.md` riga 18); la v0.1 è 2v2 contro bot |
| 1 | timer di 45 s | **SUPERATO (D-590 (2))** | timer per formato, sul misurato (`D-010`): 30 s nel 2v2, 40–45 s nel 3v3 |
| 1 | lore: l'energia «Flux» che riscrive la materia | **SUPERATO (D-588 (2)) / RESEARCH** | l'elemento si chiama `Warp`; `Flux` resta testo rimosso da `D-130`. La lore è `PROPOSED` (`D-246`) e il nome dell'energia vi si rilegge |
| 2.1 | quattro fasi con Reaction/DoT come quarta; Reaction prioritarie nella fase; colpi simultanei entrambi validi; stati da colpo simultaneo dal turno dopo | **SUPERATO (D-583 (1))** per le fasi; **CONFERMA** per i colpi simultanei | cinque macro-fasi con il Move dopo il Blast (ADR-0003), reazioni come segmenti (ADR-0004); il Blast simultaneo già infligge a entrambi; lo stun nato nel Blast di T nega il Move di T (`D-569`) e non il Dash: il GDD dice «dal turno successivo», il contratto è più fine |
| 2.2 | esagono L = 1,5 m, 2,6 m centro-centro, sei direttrici, occupazione esclusiva | **CONFERMA** nella sostanza | griglia esagonale (ADR-0002), un solo slot di occupancy (`D-302`/`D-303`/`D-308`); le misure in metri sono presentazione |
| 2.2 | dodici spicchi come posizione (centro contro spicchio) | **SUPERATO (D-583 (4))** | dodici settori come righello, un solo slot |
| 2.2 | Facing allineato a uno spicchio; Flanking sui 4 spicchi posteriori, +50% e ignora 50% DEF | **SUPERATO (D-587)** | sei lati (`D-126`); bonus per personaggio dell'attaccante sui tre lati posteriori, sommato alla Base; il retro toglie copertura e Guard (ADR-0005 §4a) |
| 2.2 | asse Z a tre livelli; Z attaccante > Z difensore ignora la DR delle coperture basse; danni da caduta deterministici | **CONFERMA** per i livelli e la caduta, **ASSENTE** per la regola di quota | `FRTCellId::Layer` separa i piani (celle su layer diversi non adiacenti senza archi espliciti); `RTFallOverLedgeTests.cpp` e `RTTurnManager.cpp` portano la caduta oltre il bordo; nessuna regola lega la quota alla copertura: `GDD-2` |
| 2.3 | 6 AP; inversione a U 1 AP; transito fra alleati in Dash; Body Block nemico | **INPUT (MSP-1.n)** per i costi; **CONFERMA** per il transito | pool AP puro di `D-584` senza numeri; D-302 (contesa deterministica) e il transito fra alleati sono già nel contratto; l'inversione a U è un costo di pivot, che `D-060`/ADR-0008 tengono su un asse proprio |
| 2.3 | tabella dei movimenti generici (Passo Furtivo 1 AP Prep, Corsa 2 AP, Scatto 3 AP, Disimpegno 2 AP, Interazione 1 AP) | **INPUT (MSP-1.n)** | letta sulle cinque fasi: il camminare è nel Move e costa per profilo (`D-584` (2)); «Passo Furtivo» nel Prep è una mobilità nuova |
| 2.4 | Overclock +2 AP con Phase Lock, tre modificatori di debito | **SUPERATO (D-584 (5), D-265)** | nessuna risorsa universale; il settimo e ottavo AP non entrano |
| 2.4 | progressione orizzontale: sidegrade a livello 5 e 10 | **RESEARCH** | nessuna progressione nel contratto; `piano-canonico-mvp.md` §9 tiene la progressione fra le decisioni aperte |
| 2.5 | `Danno = (Base × Mod) × 100/(100+DEF o RES) × (1 − DR copertura)` | **SUPERATO (D-589, D-590 (3))** | copertura e Guard **piatte**, sottratte dopo la formula; il bonus alle spalle sulla Base; DEF = Armor, RES **per elemento** |
| 2.5 | tre vettori di Combo-Breaking | **CONFERMA** nel principio | ADR-0004 e `D-109` (reazioni), spinte e trazioni, Interrupt: nessun annullamento binario esiste già |
| 2.5 | modalità «Core Refactor»: tre nodi, 1 AP per catturare, impulso ogni due turni, 100 punti | **RESEARCH** | scelta dell'autore (2): si decide con il suo owner (`GDD-1`); oggi vittoria = eliminazione + obiettivi + limite di round (ADR-0003), il bot cerca obiettivi non contesi (`HexBot.SeeksUncontestedObjective`) |
| 3 | dodici archetipi di muro con HP, DR %, LoS e reazione al Flux | **INPUT (DR-10 → catalogo coperture; MSP-3.c per le reazioni)** | `D-590` (3): piatta per colpo, i dodici tipi entrano come contenuto con Integrity e arco; la DR % del GDD si traduce in un valore piatto sulla scala ×10; le reazioni al Flux sono input per le coppie di `Warp` |
| 4.1 | nove elementi: sei Tier 1 e tre catalizzatori; cariche sulle celle, durate 2/2/1 turni | **SUPERATO (D-588 (1)(3))** per il vocabolario; **INPUT (MSP-3.c)** per le durate | dodici elementi piatti; Morte, Vita e Arcano del GDD precedente non ci sono più nel GDD, ma `D-588` li ha accettati: la divergenza è registrata, non riconciliata |
| 4.2 | matrice di fusione 6×6 → stati Tier 2; fusioni con i catalizzatori | **INPUT (MSP-3.c)** | `D-275` esige una voce per coppia con stato: gli stati Tier 2 sono candidati `DESIGNED` per le coppie corrispondenti, non regole |
| 4.3 | Finisher Tier 3 (Tier 2 + terzo elemento, effetto per fase) | **SUPERATO (D-583 (8))** | la combo a tre cariche non è stata scelta e resta fuori (invariante 16); il catalogo delle detonazioni resta ricerca |
| 5.1 | loadout fisso: una skill base per fase, Ultimate e Reaction, slot attivi da fissare in playtest; quattro archetipi di Reaction | **INPUT (MSP-5.d)**; **CONFERMA** parziale | `Counter`, `Intercept`, `Deflect`, `Evade` esistono; `Overwatch` e `Guard` sono principali del Prep, non reazioni (spec-economia §2) |
| 5.1–5.2 | otto eroi: Riktor, Steel, Kallari, Feng Mao, Murdock, Gadget, Muriel, The Fey | **CONCORDE CON OGGI (D-585, D-586)** | sono gli otto nomi decisi; la mappatura per ruolo di `D-586` coincide con i ruoli del GDD |
| 5.1 | HP 950–1900, DEF 15–60, RES 15–40, Move 2–4, gittata 1–8 | **INPUT (MSP-4.n)** | scala ×10 di `D-589` (2): gli HP del GDD vanno oltre il 900–1200 ipotizzato, ed è un dato per E49, non un conflitto |
| 5.2 | kit per eroe con AP, cooldown, elemento, valori; sinergie e matchup | **INPUT (MSP-5.d, MSP-1.n)** | scelta dell'autore (3): il GDD sostituisce la specifica del 2026-11-07 come input; i valori si accettano con una `D-nnn` dopo la traduzione (cinque fasi, pool AP, scala ×10) |
| 6 | cel-shading con outline, cut-in manga con freeze-frame, onomatopee | **CONCORDE CON OGGI (D-590 (1))** | E52.1 (#4005) recepisce lo stile; cut-in e onomatopee sono presentazione (CLAUDE.md §11) |
| 6 | materiali procedurali per stato della cella | **INPUT (E52)** | l'ambiente è E52; gli stati Tier 2 non esistono, il principio «la cella mostra il suo stato» sì |
| 6 | HUD: timeline a 8 tracce su quattro colonne, Echoes alleati, bussola a 12 spicchi | **INPUT (progettazione HUD)** | la Ghost Timeline esiste (`progettazione-hud.md`); le colonne sono cinque, non quattro; la bussola a dodici settori è il righello di `D-303` |
| 6 | grammatica: forme per fase, glifi per elemento, Syntax Ring, Wireframe Rule | **INPUT (progettazione HUD, E52.1)** | le icone di fase esistono (`RT_UI_Icon_Phase_*`, quattro: manca il Move); i glifi sono dei dodici elementi di `D-588`, non dei nove |
| 7 | bot: utility geometrica e alchemica, MCTS adattato, Overclock per archetipo, Blackboard di squadra | **INPUT (E26)**; **SUPERATO** per l'Overclock | E26 è al tetto C (`D-578`) con il modello utility; MCTS e Blackboard sono candidati da misurare, non impegni; la coordinazione di squadra esiste (#326) |

## 3. La precedenza fra le due fonti dell'autore, e le altre tre scelte

Quattro domande a scelta, testo delle opzioni alla lettera in `D-591`.

1. **Dove il GDD diverge dalle decisioni di oggi, quale fonte governa?** → *«Le decisioni di oggi governano; il GDD entra dove non
   confligge»*. I sette punti superati: elementi (D-588, D-583 (8)), copertura (D-589, D-590 (3)), Flanking (D-587), Overclock (D-584, D-265),
   fasi e spicchi (D-583 (1)(4)), RES per elemento (D-589 (1)), il nome «Flux» (D-588 (2), D-130).
2. **«Core Refactor»** → *«Resta RESEARCH: una modalità nuova si decide con il suo owner»* (`GDD-1`).
3. **Kit e numeri del GDD** → *«Sì, il GDD sostituisce la specifica di ieri come input di design»* per `MSP-5.d`, `MSP-4.n`, `MSP-1.n`.
4. **AI e HUD** → *«Sì, come input di design per E26 e per la progettazione HUD»*.

## 4. Che cosa entra, e dove

| Contenuto del GDD | Sede che lo riceve | Come |
|---|---|---|
| kit degli otto eroi (§5.2), statistiche (§5.1), costi dei movimenti (§2.3) | `MSP-5.d`, `MSP-4.n`, `MSP-1.n` in `OPEN_DECISIONS.md`; [`e38-action-point-numeri-simulazione-2026-11-08.md`](e38-action-point-numeri-simulazione-2026-11-08.md) | input dichiarato; nessun valore in vigore prima della sua `D-nnn` |
| matrice di fusione (§4.2) e durate (§4.1) | `MSP-3.c` | candidati `DESIGNED` per le coppie corrispondenti dei dodici di `D-588`, con i nomi tradotti (Flux → Warp) |
| dodici archetipi di muro (§3) | catalogo coperture (CP 9.1, `D-590` (3)) | righe con valore piatto, Integrity e arco; le reazioni al Flux → `MSP-3.c` |
| grammatica HUD e cut-in (§6) | [`../../technical/systems/progettazione-hud.md`](../../technical/systems/progettazione-hud.md) (nota in testa), E52.1 (#4005) | riferimenti visivi, non regole; le forme di fase sono cinque |
| architettura dei bot (§7) | E26 (#326), nota nel referto e commento sull'epic | input; il tetto C di `D-578` resta |
| «Core Refactor» (§2.5), sidegrade (§2.4), regola di quota (§2.2) | `GDD-1`, `GDD-3`, `GDD-2` in `OPEN_DECISIONS.md` | aperte, con il loro owner |

## 5. Il panel

- **Wiegers**: il GDD si dichiara «Source of Truth» ma non porta criteri di accettazione né misure; la gerarchia del repository li porta.
  La scelta (1) dell'autore è la risposta corretta alla domanda «chi governa», e il referto la registra perché la domanda non torni.
- **Adzic**: i kit sono il contenuto più utile del documento, perché sono esempi concreti con numeri; vanno tradotti, non copiati:
  quattro fasi → cinque, AP → pool di `D-584`, DEF/RES → scala ×10 e RES per elemento. Un kit tradotto è un esempio del contratto; uno
  copiato è un drift.
- **Fowler**: nove elementi in due tier con matrice di fusione è un *modello*; dodici piatti è un *vocabolario*. L'autore ha scelto il
  vocabolario e lasciato il modello alla sede delle coppie (`MSP-3.c`): è coerente, purché quella sede nasca.
- **Nygard**: tre fonti dell'autore in due giorni (specifica, decisioni, GDD) sono il rischio operativo di questo mese: ogni nuova
  stesura va misurata contro le decisioni registrate **prima** di essere letta come verità, e questo referto è il precedente.
- **Crispin**: nulla di oggi cambia un test. Il primo test che il GDD rende scrivibile è nei kit: «ogni abilità del catalogo dichiara fase,
  costo ed elemento fra i dodici» (`Catalog.EveryActionDeclaresACost` della proposta E38).
- **Cockburn**: la nota IP resta (D-585 (2)); il GDD usa i nomi Paragon e il nome «Flux» ovunque, e nessuna delle due cose è un
  problema finché è sviluppo. Il giorno della pubblicazione lo è, e la voce §9 del canone lo ricorda.

## 6. Difetti interni del GDD, indipendenti dal contratto

1. §2.3 dà 6 AP, §5.2 dà a ogni eroe tre skill base (1+2+2 = 5 AP) più Ultimate (3) e Reaction (1–2): un turno «pieno» del GDD costa 9–10
   AP e il testo non dice quale parte cada; l'Overclock (+2) non basta.
2. §4.1 dichiara **nove** elementi; il titolo del documento pure; §4.2 ne fonde sei e tratta i tre catalizzatori a parte, ma §4.3 usa
   «Disintegrazione» e «Scudo» come terzo elemento di Finisher: i catalizzatori sono elementi o no?
3. §2.2 dà al Flanking i «4 spicchi opposti» (120°) e lo chiama «cono posteriore 90°–120°»: 4 spicchi da 30° sono 120°, non 90°.
4. §5.1 dice che Overwatch, Guard, Counter ed Evasion sono archetipi di **Reaction**; §5.2 dà a Steel `Ablative Plating (Guard)` con «1–2 AP»:
   un costo variabile senza regola che lo decida.
5. §3 dà HP «Infinito» a quattro archetipi e DR 0% a due: un muro indistruttibile con DR 0% (Speed Gate) non è una copertura, è un
   terreno; la tabella mescola due categorie.
6. §6 descrive la timeline con «4 colonne temporali» e §2.1 chiama la quarta «Reaction / DoT»: i DoT del GDD risolvono dopo il Blast, ma
   nella tabella dei Finisher (§4.3) chiusure in Prep producono DoT «a chi inizia il turno dentro»: due momenti per lo stesso effetto.

## 7. Provenienza, limiti e stati di verifica

- Testo letto per intero (17 pagine, estrazione `pymupdf4llm`, tabelle ricostruite; dove dubbio fa fede il PDF).
- Misure sul repository: `FRTCellId::Layer` (`Source/RefactorTactics/Map/RTCellId.h`), `RTFallOverLedgeTests.cpp`, `HexBot.SeeksUncontestedObjective`,
  `RT_UI_Icon_Phase_*` in `Content/RT/UI/Icons/`, `spec-durata-partita-e-scala-mappe.md` righe 17–19, `D-010`, `D-256`, `D-578`.
- ⚠️ **Numero di epic conteso**: il PRD del danno si intitola «E49 proposta», ma l'epic **E49** esistente è «Tactical Camera & Map
  Presentation» (#1769). `D-583` (6) e `D-589` chiamano «E49» l'owner del danno **leggendo il PRD**: l'owner è nato come **E53** (#4013) il 2026-11-08, e le due voci vanno lette con questa correzione (registrata in `D-591`).
- Issue aperte il 2026-11-08 dalle decisioni di oggi: #4012 (CP 38.6, numeri AP), #4013 (E53), #4014 (`MSP-5.d`), #4015–#4017 (`GDD-1`…`GDD-3`),
  #4018 (icone di fase), #4019 (licenza Paragon), #4020 (vocabolario di `D-588`); commentate #609, #617, #995, #1557, #1803, #1833, #326, #4005, #4011.
- Compile · Tests · Determinism · Replay · Privacy · PIE · Packaged: `N/A`. Nessun codice, asset o scenario cambia.
- `NOT RUN`: la licenza degli asset Paragon (prerequisito della pubblicazione, `D-585` (2)).
