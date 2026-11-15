# Riktor — traduzione del catalogo del 2026-11-13

> 🧪 **Stato:** `DESIGN_SPEC` · **PROPOSTA** per la `D-nnn` d'accettazione prevista da [D-612](../../decisions/RT_PDR_00_Decision_Log.md) (1).
> **Non è in vigore.** Il kit in vigore per lo slot è quello della [scheda di Riktor](../../characters/v0.1/riktor.md) (oggi `Hero.Branth.*` nel codice, fino al rename di [#4011](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4011)). Questa pagina non decide nulla: traduce il catalogo nelle regole del contratto e mette in fila ciò che l'autore deve scegliere.
>
> **Base:** `origin/main` `2d0c1db0c` (worktree `riktor-kit-traduzione`).
> **Input:** il catalogo dell'autore, alla lettera, nell'Appendice A del [referto dello spec panel](riktor-hud-ap-spec-panel-2026-11-13.md); la mappa delle premesse (§2), riuso/estensione/creazione (§3.2), gli esempi che distinguono le letture (§3.3) e le domande di dettaglio (§6) di quel referto sono riusati, non rifatti.
> **Decisioni rispettate:** [D-583](../../decisions/RT_PDR_00_Decision_Log.md) (fasi e posizione), [D-588](../../decisions/RT_PDR_00_Decision_Log.md) e [D-593](../../decisions/RT_PDR_00_Decision_Log.md) (elementi), [D-589](../../decisions/RT_PDR_00_Decision_Log.md) e [D-602](../../decisions/RT_PDR_00_Decision_Log.md) (danno), [D-592](../../decisions/RT_PDR_00_Decision_Log.md) (reazioni in lista), [D-594](../../decisions/RT_PDR_00_Decision_Log.md) e [D-613](../../decisions/RT_PDR_00_Decision_Log.md) (Action Point), [D-612](../../decisions/RT_PDR_00_Decision_Log.md) (il catalogo diventa kit).

## Come leggere le provenienze

Ogni valore porta fra parentesi quadre la sua fonte. Un numero senza fonte non c'è.

| Etichetta | Significato |
|---|---|
| **[CATALOGO]** | testo dell'autore, Appendice A del referto |
| **[DECISO D-nnn]** | testo scelto di una decisione accettata del [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md) |
| **[LETTURA D-nnn (k)]** | lettura dell'assistente registrata dentro una decisione, nella parte «Letture dell'assistente, non decisioni»: **non** è decisa |
| **[GDD]** | input, [`master-game-design-document-input-2026-11-08.md`](../../research/handoff/master-game-design-document-input-2026-11-08.md) righe 220-234; non governa |
| **[PROPOSTA]** | scelta di questa traduzione, col motivo; la decide la `D-nnn` |
| **[DOMANDA → TR-n]** | decisione d'autore; l'elenco consolidato è in [Domande per l'autore](#domande-per-lautore) |
| **[codice]** | `file:riga` letto su `2d0c1db0c`, **non eseguito** |

**Convenzione degli esempi.** Salvo dove l'esempio dice altro: unità sul Layer 0; il bersaglio è colpito **di fronte** (il termine Spalle vale 0), senza copertura né Guard, `Mod` 100; lo scudo base di [D-224](../../decisions/RT_PDR_00_Decision_Log.md) (50, ferma solo il danno `Direct`, `RTCombatLibrary.h:303-312`) è **già eroso**; arrotondamento unico per difetto [DECISO D-595 (1), `RTCombatResolver.h:143`]. Gli esempi sono criteri d'accettazione candidati: valgono solo dopo che le DOMANDE che citano sono chiuse.

## Come leggere la traduzione

| Nel catalogo | Nel contratto, in questa pagina |
|---|---|
| «Prep Phase», «Dash Phase», «Blast Phase», «Reaction Phase» | il Planning e cinque macro-fasi di risoluzione (Prep → Dash → Blast → Move → Cleanup) [DECISO D-583 (1), ADR-0003]; le reazioni sono segmenti con un trigger (ADR-0004), in **lista** per unità [DECISO D-592]. Nel resolver: Preparation 10, FastMovement 20 (Dash), Control 30 e Attack 40 (dentro il Blast), NormalMovement 20 (Move), Environment 50, Cleanup 60 |
| «Blast Phase nemica» | una Blast «nemica» non esiste: le fasi sono Planning → Prep → Dash → Blast → Move → Cleanup [DECISO D-583 (1)]; il Blast è uno per tutte le unità e il suo danno è simultaneo [ADR-0003, emendato da D-503] |
| «singolo spicchio», «micro-spicchio», «condivide il macro-esagono» | celle: un solo occupante per `FRTCellId`, lo spicchio è un righello [DECISO D-583 (4)] |
| «Costante: N AP» | costo per riga; classi Prep 1 · Dash 2 · Blast 2 · Move 2 · reazione 2 [DECISO D-594 (1)], ogni riga dichiara il proprio costo [DECISO D-594 (3)]; ogni Ultimate costa 3 in qualunque fase [DECISO D-613 (4)]; budget variabile per unità e per turno, noto al planning [DECISO D-613 (1)], con 6 come default [LETTURA D-613 (5)] |
| «Nessun Cooldown» / cooldown non dato | il cooldown è un asse proprio [DECISO D-584 (4)]; input solo dal GDD per otto voci; un'azione con cooldown compare al più una volta nel piano, senza cooldown si ripete [LETTURA D-613 (5) della risposta libera (2); riserva `MSP-1.r` aperta] |
| ordine dentro la fase | priorità crescente, il numero basso risolve prima [codice: `RTActionQueueLibrary.cpp:19`, `RTTurnManager.cpp:2819`]; a parità, fra azioni della **stessa unità**, l'ordine di dichiarazione [DECISO D-613 (3)], **oggi non implementato**; fra unità diverse decidono `ActionId`, attaccante, intento [codice: `RTTurnManager.cpp:2819`] |
| «Danno Fisico» | danno `Direct`, mitigato dalla DEF [DECISO D-589 (1)] |
| «Danno Energetico», «Danno Oscuro» | [PROPOSTA] danno elementale dell'elemento della voce, mitigato dalla RES di quell'elemento con la stessa forma [DECISO D-589 (1) per la forma]; se lo sia davvero è [DOMANDA → TR-1]. Oggi `ERTDamageSource` = {`Direct`, `Environmental`} (`RTCombatLibrary.h:177-181`): un percorso elementale **non esiste** |
| «puri», «garantiti», «ignora l'armatura» | D-589 non ha un danno che salti DEF/RES, copertura e Guard [DOMANDA → TR-3]; il GDD chiama «armatura» la DEF |
| ⚡ Fulmine · 🪨 Terra · 💀 Morte · 🛡 Scudo | `Element.Electric` · `Element.Earth` · `Element.Decay` · `Element.Aegis` [DECISO D-588, D-593]. `FRTActionDef` **non ha** un campo elemento |
| «Combo Alchemiche» | candidati per le righe del [catalogo delle coppie](../../balance/RT_ElementPairCatalog_v0.1.md) (owner [D-275](../../decisions/RT_PDR_00_Decision_Log.md)), mai voci della scheda dell'eroe (AGENTS.md §3, invarianti 15 e 16); le combo a tre elementi non hanno una regola [DECISO D-591 (1)] |
| Overclock (non è nel catalogo: risposta libera D-612 (3), dock D-612 (4), definizione D-612 (5); GDD riga 222) | «una skill che ti da un vantaggio per uno svantaggio successivo» [definizione dell'autore, D-612 (5), alla lettera], letta come categoria di skill [LETTURA D-612 (7)]; l'Overclock universale del GDD resta superato [DECISO D-591 (1) (d)]; la skill di Riktor non esiste (`KIT-3`) |
| formula del danno | `max(0, (Base + Spalle) × Mod × 100/(100+DEF) − Copertura − Guard)`; copertura bassa 60, Guard 100, Brace 60; scala ×10 [DECISO D-589, D-602] |

## Il kit in breve

Tutte le voci entrano nel kit [DECISO D-612 (2) «Tutte nel kit»]; quali occupano la dock provvisoria è aperto (`KIT-2`). Celle, AP e priorità sono le proposte delle sezioni per voce.

| Id | Gruppo | Fase (resolver) | AP | Cooldown | Priorità | Portata / forma | Elemento | Danno base |
|---|---|---|---|---|---|---|---|---|
| `Hero.Riktor.ShockWhip` | Attacco base | Blast · Attack | 2 [CATALOGO] | 0 [CATALOGO] | 50 [PROPOSTA] | 1–2 [GDD] · Single + catena | Electric | 160 [CATALOGO] |
| `Hero.Riktor.StaticSweep` | Attacco base | Blast · Attack | 2 [CATALOGO] | 0 [CATALOGO] | 50 [PROPOSTA] | 1 · Cone, 3 celle | Electric | 120 [CATALOGO], per cella o diviso [DOMANDA] |
| `Hero.Riktor.TaserStrike` | Attacco base | Blast · Attack | 2 [CATALOGO] | 0 [CATALOGO] | 50 [PROPOSTA] | 1–2 [PROPOSTA] · Single | Electric | 100 [CATALOGO] |
| `Hero.Riktor.Electrocute` | Prep | Preparation (tick nel Cleanup) | 1 [CATALOGO] | 1 [GDD] | 30 [PROPOSTA] | 0 · Area raggio 1, aura | Electric | 80 per tick [CATALOGO], per nemico o diviso [DOMANDA] |
| `Hero.Riktor.NecroticAnchor` | Prep | Preparation (verifica nel Dash) | 1 [CATALOGO] | 1 [PROPOSTA] | 40 [PROPOSTA] | 1 · una cella | Decay | — |
| `Hero.Riktor.MagneticSpikes` | Prep | Preparation (effetto nel Dash) | 1 [CATALOGO] | 2 [PROPOSTA] | 20 [PROPOSTA] | 1–3 [PROPOSTA] · Area raggio 1 | Earth | 50 [CATALOGO] |
| `Hero.Riktor.ChainHook` | Dash | lettura (a) Control · lettura (b) FastMovement | 2 [CATALOGO] | 2 [GDD] | 40 (a) [PROPOSTA] | 2–4 [CATALOGO] · Single | Earth | 140 [CATALOGO] |
| `Hero.Riktor.VoltageLeap` | Dash | FastMovement, onda nel Blast | 2 [CATALOGO] | 2 [PROPOSTA] | 25 [PROPOSTA] | salto 2 · Area raggio 1 all'atterraggio | Electric | 90 [CATALOGO], per nemico o diviso [DOMANDA] |
| `Hero.Riktor.RiotCharge` | Dash | FastMovement | 2 [CATALOGO] | 2 [PROPOSTA] | 35 [PROPOSTA] | 3 · LinearPass | Aegis | [DOMANDA]; scudo 150 [CATALOGO] |
| `Hero.Riktor.CorrosiveShackles` | Blast primaria | Blast · Attack | 2 [CATALOGO] | 2 [GDD] | 65 [PROPOSTA] | 1–2 [PROPOSTA] · Single | Decay | 200 [CATALOGO] |
| `Hero.Riktor.SeismicSmash` | Blast primaria | Blast · Attack | 2 [CATALOGO] | 2 [PROPOSTA] | 80 [PROPOSTA] | 1 · Single | Earth | 220 [CATALOGO], +50% contro un lato bloccante |
| `Hero.Riktor.ArcDischarge` | Blast primaria | Blast · Attack | 2 [CATALOGO] | 2 [PROPOSTA] | 60 [PROPOSTA] | 1–3 [PROPOSTA] · Single + rimbalzo | Electric | 180 + 100 [CATALOGO] |
| `Hero.Riktor.Skewer` | Ultimate | Blast · Attack | 3 [DECISO D-613 (4)] | 3 [GDD] | 45 [PROPOSTA] | 1–2 · Cone, 8 celle | Electric | 380 [CATALOGO], per cella o diviso [DOMANDA] |
| `Hero.Riktor.IronMaiden` | Ultimate | Preparation [DECISO D-613 (4)] | 3 [DECISO D-613 (4)] | 3 [GDD] | 25 [PROPOSTA] | 1–3 [CATALOGO] · Single | Earth [PROPOSTA] | — |
| `Hero.Riktor.MagneticBastion` | Reazione | Control, punto `BlastIntercept` | 1 [CATALOGO] | 2 [GDD] | 10 [PROPOSTA] | 0–2 dall'alleato [PROPOSTA] | Earth | — |
| `Hero.Riktor.ShockArmor` | Reazione | Control, punti `BlastHits` e `BlastDisplacement` | 1 [CATALOGO] | 1 [GDD] | 20 [PROPOSTA] | 0 · Single sull'attaccante | Electric | 160 [PROPOSTA] |

---

## 1. Attacchi base

Gruppo del catalogo: «1. Attacco Base (Costante: 2 AP | Nessun Cooldown)».

**Scelte comuni.** Fase Blast, segmento `Attack`, come l'attacco base core (`RTCatalogLibrary.cpp:1256`) [PROPOSTA]. 2 AP [CATALOGO; coincide con la classe Blast, DECISO D-594 (1)]. Cooldown 0 [CATALOGO], quindi ripetibili nello stesso piano fino al budget [LETTURA D-613 (5) della risposta (2), non decisa; riserva `MSP-1.r` aperta]. Priorità 50 per tutte e tre, quella dell'attacco base di oggi [PROPOSTA]. Derivate con `MakeHeroBasicAttack` (`RTHeroCatalogLibrary.cpp:166-178`), ereditano `BlindAimDirect` e `bCountsAsAttack` dal core (gate `HeroCatalog.EveryBasicAttackTakesTheCorePolicy`, `RTCatalogLibrary.cpp:1273-1274`). Cornice Blast con glifo saetta [PROPOSTA, lettura di Appendice B].

**Coerenza.** Danno per AP sul singolo: Shock Whip 80, Static Sweep 60 (fino a 180 con tre bersagli nella lettura per cella), Taser Strike 50 più il controllo: il singolo puro sopra, l'area che paga sul singolo, l'utility che paga in danno [PROPOSTA]. Regge solo con Static Sweep a 120 per cella (TR-11). Raffica a budget pieno, **condizionata alla lettura sulla ripetizione** (TR-4): Shock Whip ×3 = 480 di base; Static Sweep ×3 = 360 per cella; Taser ×3 = 300 e un −1 MP che non si cumula [PROPOSTA].

**Nodi strutturali.** (1) Tre attacchi base, mentre il catalogo eroi oggi ne ha uno all'indice 0 e la dock provvisoria un posto [DECISO D-612 (4)]: chi lo occupa e che cosa diventano le altre due è aperto (`KIT-1`, `KIT-2`, TR-6). (2) L'equipaggiamento si ridisegna [DECISO D-612 (6)], e la nuova arma va pensata per tre forme: in `HexCone` portata e profondità sono lo stesso numero, e con portata ≤ 0 il cono è vuoto (`RTHexLibrary.cpp:780`), quindi un −1 di portata come quello di `Weapon.Impact` svuota Static Sweep. (3) Lo Slow di `ImpactShot` esce [DECISO D-612 (6)], e con lui una delle due ragioni di `Reaction.Cleanse`.

### Shock Whip — `Hero.Riktor.ShockWhip`

> «Opzione 1 (Originale) — Shock Whip [⚡ Fulmine]: Infligge 160 Danno Energetico su singolo spicchio. Se il bersaglio si trova su terreni bagnati (Acqua, Fango, Vapore), il danno si incatena ai nemici adiacenti.»

| Campo | Valore |
|---|---|
| Ruolo | attacco base [CATALOGO], voce distinta [DECISO D-612 (2)]; l'unico del gruppo con una riga nel GDD (riga 228). Occupante di default del posto base della dock [PROPOSTA: miglior danno per AP sul singolo, 80 contro 60 e 50; DOMANDA → TR-6] |
| Fase | Blast · `Attack` (40) [GDD riga 228; PROPOSTA: come `Action.BasicAttack`] |
| Costo AP | 2 [CATALOGO; classe Blast, DECISO D-594 (1)] |
| Cooldown | 0 [CATALOGO; GDD «CD 0»]; ripetibile nello stesso piano fino al budget [LETTURA D-613 (5); riserva `MSP-1.r`] |
| Priorità | 50 [PROPOSTA: priorità di `Action.BasicAttack` e di `Hero.Branth.ImpactShot`, `RTCatalogLibrary.cpp:1256`, `RTHeroCatalogLibrary.cpp:840`]. Priorità crescente: il numero basso risolve prima [codice: `RTActionQueueLibrary.cpp:19`, `RTTurnManager.cpp:2819`]. A parità fra azioni della stessa unità: ordine di dichiarazione [DECISO D-613 (3)], oggi non implementato |
| Portata e linea di vista | minima 1, massima 2 [GDD «raggio 1–2 hex»; il catalogo non la dà, DOMANDA → TR-10]. `BlindAimDirect`, policy del core (`RTCatalogLibrary.cpp:1275`) copiata da `MakeHeroBasicAttack` (`RTHeroCatalogLibrary.cpp:176`): si mira anche a una cella non vista, il primo ostacolo ferma il colpo [DECISO D-415, D-490]. `MinRangeCells` esiste (`RTActionDef.h:896`) |
| Forma | `Single` [CATALOGO «singolo spicchio» → cella, DECISO D-583 (4)]; con la condizione vera, un colpo secondario a ciascun nemico nelle sei celle adiacenti al bersaglio, un salto solo [PROPOSTA: lettura letterale; DOMANDA → TR-7] |
| Bersaglio | unità nemica [PROPOSTA]; il secondario colpisce solo nemici [CATALOGO «ai nemici adiacenti»], filtro più stretto del default `bFriendlyFire = true` (`RTActionDef.h:809`) |
| Elemento | `Element.Electric` [CATALOGO; DECISO D-593] |
| Tipo di danno | elementale Electric, RES Electric [PROPOSTA: «Energetico» opposto a «Fisico» nello stesso catalogo; DOMANDA → TR-1] |
| Danno base | 160 al bersaglio [CATALOGO]; 160 a ciascun nemico adiacente [PROPOSTA: nessuna riduzione dichiarata; DOMANDA → TR-7]. Ogni colpo: `max(0, (Base + Spalle) × Mod × 100/(100+DEF) − Copertura − Guard)` [DECISO D-589, D-602]; se il danno è elementale, la stessa forma usa la RES di Electric al posto della DEF [PROPOSTA; DOMANDA → TR-1] |
| Movimento | nessuno [CATALOGO]. Oggi `Weapon.Impact` aggiunge una spinta all'attacco base (`RTCatalogLibrary.cpp:783`); l'equipaggiamento si ridisegna [DECISO D-612 (6)], e se la nuova arma tocchi l'attacco base è [DOMANDA → TR-34] |
| Stati | nessuno [CATALOGO]; la catena non è uno stato, avviene nello stesso colpo |
| Oggetti persistenti | nessuno [CATALOGO] |
| Condizioni | il bersaglio, nello snapshot del colpo, porta `Status.Wet` **oppure** occupa una cella `ERTHexSurface::ShallowWater` (`RTHexCellData.h:20`), che di per sé applica Wet all'occupante (`RTTerrainTests.cpp:74-82`); il secondo ramo copre solo chi è entrato nell'acqua nel Dash prima che lo stato fosse applicato [PROPOSTA; tempistica non verificata]. «Fango» non esiste (candidato OPEN di Water+Earth); «Vapore» non è uno stato (input su Fire+Water, coppia CURRENT `Reaction.WaterQuenchesFire`): entrano solo se una decisione li crea [DOMANDA → TR-9] |
| Rapporto con le coppie | [PROPOSTA] la condizione resta un dato dell'abilità che legge lo stato del bersaglio (D-029, ADR-0006), e Shock Whip si registra come **manifestazione** della riga CURRENT Water+Electric, `Reaction.ElectricPropagatesThroughWater` (owner D-275, [catalogo delle coppie](../../balance/RT_ElementPairCatalog_v0.1.md) riga 63), come già `Hero.Aevik.LinearDischarge` (`RTHeroCatalogLibrary.cpp:498-500`). ⚠️ **Divergenza dal referto**: §2 e §3.2 leggono la catena come riuso della propagazione. Le due letture vanno all'autore [DOMANDA → TR-8] |
| Reazione | — |
| Fallback | `Cancel` [PROPOSTA: fallback di `Action.BasicAttack`, `RTCatalogLibrary.cpp:1257`; un attacco diretto senza bersaglio non esegue nulla, `RTActionDef.h:61`] |
| Icona | cornice Blast (rombo rosso), glifo saetta [PROPOSTA, Appendice B, lettura non confermata]; badge della catena: nessuno fra quelli osservati in `skill.af` [DOMANDA → TR-6]; il costo non si disegna (`progettazione-hud.md` §7.1) |

**Nel motore.**
- *Riuso*: `MakeHeroBasicAttack` con `Attack`, priorità 50, `Cancel`, effetto `Damage` (modello `RTHeroCatalogLibrary.cpp:840-845`); `BlindAimDirect` e `bCountsAsAttack` del core; `Status.Wet`; `ShallowWater`; `MinRangeCells`.
- *Estensione*: la condizione «Wet o acqua» come dato dell'azione, invece di una funzione per eroe (oggi il bonus su Wet di Aevik è codice dedicato, `URTCombatLibrary::AevikWetDischargeBonus`); l'ordine di dichiarazione nel piano [DECISO D-613 (3)], come coordinata in `FRTUnitPlan` e nel TurnLog [LETTURA D-613 (5)].
- *Creazione*: campo elemento su `FRTActionDef`; percorso di danno elementale; il colpo secondario sui nemici adiacenti: oggi `ProduceEvents` legge un solo `TargetUnitId` per istanza (`RTHeroCatalogLibrary.cpp:507-510`). Sede: Combat, raccolta dei colpi del Blast, con eventi ordinati per Stable ID nel TurnLog, nello snapshot del replay e nell'hash; proiezione degli eventi secondari con la stessa regola di vista dei colpi.

**Esempio.** *Given* Riktor in (0,0,0), budget 6; nemico A a distanza 2, `Status.Wet`, HP 1000; nemico B adiacente ad A, HP 900, non bagnato; alleato C adiacente ad A, HP 900; DEF 0 e RES Electric 0 per A, B e C; nessuna cella d'acqua intorno ad A, e si assume che la riga Water+Electric non scatti sul colpo (TR-8). *When* Riktor pianifica Shock Whip su A (2 AP, ne restano 4) e il Blast risolve. *Then* A 1000 → 840; B 900 → 740 (catena a danno pieno, PROPOSTA); C resta a 900 (la catena colpisce solo nemici). Se A non è Wet né in acqua: A → 840, B e C intatti. Se A è a distanza 3: piano rifiutato per portata.

**Domande.** TR-1 (tipo di danno), TR-2 (scudo base), TR-4 (ripetizione), TR-6 (posto nella dock), TR-7 (forma e danno della catena), TR-8 (rapporto con la coppia Water+Electric), TR-9 (Fango e Vapore), TR-10 (portata 1–2 del GDD), TR-34 (spinta della nuova arma).

### Static Sweep — `Hero.Riktor.StaticSweep`

> «Opzione 2 (Cleave) — Static Sweep [⚡ Fulmine]: Fendente ad arco da 120 Danno Energetico distribuito su 3 esagoni frontali contigui. Ottimo per colpire formazioni compatte.»

| Campo | Valore |
|---|---|
| Ruolo | attacco base [CATALOGO], voce distinta [DECISO D-612 (2)]; posto nella dock [DOMANDA → TR-6] |
| Fase | Blast · `Attack` [PROPOSTA: come l'attacco base core] |
| Costo AP | 2 [CATALOGO; DECISO D-594 (1)] |
| Cooldown | 0 [CATALOGO]; ripetibile [LETTURA D-613 (5); riserva `MSP-1.r`] |
| Priorità | 50 [PROPOSTA: come l'attacco base core; un fendente a 1 cella non ha ragione di risolvere dopo i colpi singoli, mentre `Action.CircularAoE` sta a 65]. A parità fra azioni della stessa unità: ordine di dichiarazione [DECISO D-613 (3)] |
| Portata e linea di vista | 1 [PROPOSTA: «esagoni frontali contigui» = celle a contatto]. ⚠️ In `HexCone` `RangeCells` è insieme portata e profondità (`RTHexCombatLibrary.cpp:42-44`, `RTHexLibrary.cpp:777-817`): un −1 d'arma lo azzera, un +1 lo porta a 8 celle [codice]. `BlindAimDirect` copiata da `MakeHeroBasicAttack` qualunque sia la forma; il suo comportamento sul cono non è verificato |
| Forma | `Cone` con `RangeCells` 1: i due settori di 60° a distanza 1 danno esattamente tre celle, la mirata e le due ai suoi lati [CATALOGO «3 esagoni frontali contigui»; codice: `RTHexLibrary.cpp:808-817`]. «Frontali» = verso la cella mirata, non il facing [PROPOSTA] |
| Bersaglio | una cella adiacente (direzione del ventaglio); colpisce ogni unità nelle tre celle [PROPOSTA]. Alleati: il default è `bFriendlyFire = true` [DOMANDA → TR-12] |
| Elemento | `Element.Electric` [CATALOGO; DECISO D-593] |
| Tipo di danno | elementale Electric [PROPOSTA; DOMANDA → TR-1] |
| Danno base | 120 **per cella colpita** [PROPOSTA: con un solo nemico nell'arco fa meno di Shock Whip, coerente con «Ottimo per colpire formazioni compatte»]. Lettura alternativa: 40 per cella (120 diviso tre), che rende la voce dominata da Shock Whip in ogni caso [motivo del rigetto]. [DOMANDA → TR-11] |
| Movimento · Stati · Persistenti · Condizioni | nessuno [CATALOGO] |
| Reazione | — ; un'area non fa scattare `HitByDirectAttack` (trigger dei soli colpi `Single`, `RTActionDef.h:282-286`) [DOMANDA → TR-13] |
| Fallback | `AttackCell` [PROPOSTA: un'area colpisce comunque le celle pianificate (`RTActionDef.h:55`), come `Hero.Aevik.Overload` e `LinearDischarge` (`RTHeroCatalogLibrary.cpp:427`, `:471`)] |
| Icona | cornice Blast, glifo saetta [PROPOSTA]; nessun badge d'area osservato [DOMANDA → TR-6] |

**Nel motore.**
- *Riuso*: `MakeHeroBasicAttack(..., ERTAbilityShape::Cone)` con Range 1 (`RTHeroCatalogLibrary.cpp:166-178`): **nessuna estensione** della funzione; `URTHexLibrary::HexCone`; effetto `Damage`; fallback `AttackCell`; profilo FX `ConeSweep` (`RTPresentationBinding.cpp:474`); regola di vista dell'impronta dei coni (`RTPlaybackLibrary.cpp:137`); riga «Forma: Cono» del tooltip (`RTHudViewModel.cpp:832`).
- *Estensione*: campo elemento (comune); se la nuova arma tocca la portata, separare portata e profondità del cono, o vietare i modificatori di portata sui coni.
- *Creazione*: solo con la lettura «diviso», una regola di ripartizione del danno fra le celle, che oggi non esiste. Con la lettura «per cella», niente.

**Esempio.** *Given* Riktor in (0,0,0), budget 6; tre nemici X, Y, Z nelle tre celle del ventaglio verso la cella adiacente di X (X al centro), HP 900 ciascuno, DEF e RES 0. *When* Riktor pianifica Static Sweep verso la cella di X (2 AP). *Then*, lettura per cella: X, Y, Z 900 → 780. Lettura diviso: 900 → 860 ciascuno. Con un alleato al posto di Z e fuoco amico attivo: anche l'alleato 900 → 780. Se X si sposta prima del Blast: il ventaglio colpisce comunque le tre celle (`AttackCell`).

**Domande.** TR-11 (per cella o diviso), TR-12 (fuoco amico), TR-10 e TR-34 (portata con la nuova arma), TR-13 (reazioni su un colpo d'area), TR-1 (tipo di danno), TR-2 (scudo base), TR-4 (ripetizione), TR-6 (posto nella dock, badge d'area).

### Taser Strike — `Hero.Riktor.TaserStrike`

> «Opzione 3 (Utility) — Taser Strike [⚡ Fulmine]: Colpo mirato da 100 Danno Energetico che riduce la capacità di movimento base del bersaglio di 1 esagono per il turno successivo.»

| Campo | Valore |
|---|---|
| Ruolo | attacco base [CATALOGO], voce distinta [DECISO D-612 (2)]; posto nella dock [DOMANDA → TR-6]. `ImpactShot` e il suo Slow escono dal kit [DECISO D-612 (6)]; Taser ne prende il ruolo di utility [PROPOSTA; LETTURA D-612 (7), che nomina come eredi «gli attacchi base»] |
| Fase | Blast · `Attack` [PROPOSTA] |
| Costo AP | 2 [CATALOGO; DECISO D-594 (1)] |
| Cooldown | 0 [CATALOGO]; ripetibile [LETTURA D-613 (5); riserva `MSP-1.r`] |
| Priorità | 50 [PROPOSTA]; con la lettura «turno successivo» la priorità non cambia l'esito sul Move del turno in corso. A parità fra azioni della stessa unità: ordine di dichiarazione [DECISO D-613 (3)] |
| Portata e linea di vista | 1–2 [PROPOSTA: come Shock Whip, perché il catalogo non dà portata; DOMANDA → TR-10]; `BlindAimDirect` (`RTCatalogLibrary.cpp:1275`) |
| Forma · Bersaglio | `Single` [CATALOGO «colpo mirato»] · unità nemica [PROPOSTA] |
| Elemento | `Element.Electric` [CATALOGO; DECISO D-593] |
| Tipo di danno | elementale Electric [PROPOSTA; DOMANDA → TR-1] |
| Danno base | 100 [CATALOGO] |
| Movimento | nessuno [CATALOGO] |
| Stati | stato **nuovo**, nome di lavoro `Status.Hindered` [PROPOSTA]: −1 MP sul budget di movimento [CATALOGO]. Applicato nel Blast; effetto dal turno successivo: vale per il planning e il Move del turno N+1, scade nel Cleanup di N+1 [CATALOGO «per il turno successivo»; PROPOSTA sul momento, noto quando si pianifica come il budget AP di D-613 (1)]. **Non** è `Status.Slow`: lo Slow è +1 al costo di ogni cella (`RTFocusArea.h:27`, `RTTurnManager.cpp:7691`) e agisce sul Move dello stesso turno. Riapplicato mentre è attivo: rinnova la durata, non si cumula [PROPOSTA; DOMANDA → TR-14] |
| Persistenti · Condizioni · Reazione | nessuno · nessuna · — |
| Fallback | `Cancel` [PROPOSTA] |
| Icona | cornice Blast, glifo saetta [PROPOSTA]; badge della riduzione di movimento: nessuno osservato [DOMANDA → TR-6] |

**Nel motore.**
- *Riuso*: `MakeHeroBasicAttack`, effetti `Damage` e `Status` con durata (modello `ImpactShot`, `RTHeroCatalogLibrary.cpp:843-844`). Precedente del −1 MP piatto: `Status.Prone`, sottratto in `ARTUnit::GetEffectiveMoveRange()` (`RTUnit.cpp:1911-1925`; `StandUpMovePointCost = 1`, `RTCombatLibrary.h:388`), punto unico che alimenta lo snapshot del Move e la validazione del piano.
- *Estensione*: `GetEffectiveMoveRange()` legge anche il nuovo stato (−1, minimo 0).
- *Creazione*: il tag (icona `UI.Icon.Status.*`, perché i figli di `Status` entrano in `RequiredIconIds()`, `RTGameplayTags.h:115-118`; glossario; tooltip) e un **inizio differito**: oggi uno stato applicato nel Blast è letto già dal Move dello stesso turno, quindi serve uno stato attivato nel Cleanup o un campo «turno di attivazione» nello snapshot, con hash e replay. Nessun reason code nuovo: `InsufficientMovementPoints` non ha produttore [DECISO D-190], la riduzione si vede come celle non raggiungibili.

**Esempio.** *Given* nemico E a distanza 2 da Riktor, HP 900, RES 0, 4 MP, che nel turno N pianifica un Move di 4 celle. *When* nel turno N Riktor pianifica Taser Strike su E (2 AP). *Then* nel Blast di N: E 900 → 800; nel Move di N E percorre 4 celle; nel planning di N+1 l'anteprima di E mostra raggiungibili solo le celle a costo ≤ 3; nel Cleanup di N+1 lo stato scade, e in N+2 E torna a 4. Lettura alternativa (tempistica dello Slow): E scende a 3 già in N.

⚠️ L'esempio di §3.3 del referto («arriva a 3 in questo turno» con lo Slow) assume uno Slow da −1 cella; il codice letto dice +1 al costo di ogni cella, cioè con 4 MP su terreno piano 2 celle. Va rimisurato prima di usarlo come criterio.

**Domande.** TR-14 (tempo, cumulo, Slow e Prone, nome e icona dello stato), TR-10 (portata), TR-34 (portata e spinta della nuova arma), TR-1 (tipo di danno), TR-2 (scudo base), TR-4 (ripetizione), TR-6 (posto nella dock, badge della riduzione di movimento).

---

## 2. Prep

Gruppo del catalogo: «2. Prep Phase (Costante: 1 AP)».

**Scelte comuni.** Fase `Preparation` (10) [CATALOGO; DECISO D-583 (1)]; 1 AP, classe Prep [CATALOGO; DECISO D-594 (1)]. Cooldown: solo Electrocute ha un input (1) [GDD riga 226]; per Anchor e Spikes un cooldown ≥ 1 [PROPOSTA], che **dipende** dalla lettura sulla ripetizione [LETTURA D-613 (5); riserva `MSP-1.r`]: senza cooldown due Anchor o due Spikes da 1 AP si ripeterebbero nello stesso piano.

**Priorità** [PROPOSTA]: Magnetic Spikes 20 (cambia il terreno, va prima di ciò che il Prep legge), Iron Maiden 25, l'Ultimate di Prep di §5 (dopo Spikes, perché la gabbia legge il terreno della cella del bersaglio, ramo Fango; a 20 pareggerebbe con Spikes nel piano dello stesso Riktor, 1 + 3 = 4 AP), Electrocute 30 (l'aura si posa su un terreno già definitivo), Necrotic Anchor 40 (si arma soltanto; l'esito si decide nel Dash). Le priorità di Prep di oggi: SuppressiveLine 30 (`RTCatalogLibrary.cpp:1556`), Brace 30 (`:1699`), `Hero.Ivrin.InterceptShot` 30 (`RTHeroCatalogLibrary.cpp:1052`), Shield 35 (`:1710`), `Hero.Muiren.FlowReaction` 36, Guard 40 (`:1289`), Overwatch 45 (`:1383`), `Hero.Branth.Reconfigure` 76, CreateCover 75 (`:1900`). Electrocute 30 **pareggia** con Brace, SuppressiveLine e InterceptShot; Anchor 40 con Guard; fra Spikes, Iron Maiden, Electrocute e Anchor nessun pareggio. Brace e Guard però sono azioni generiche, accodate al kit di ogni unità, Riktor compreso [codice: `RTUnit.cpp:2081`, `RTCatalogLibrary.cpp:2070-2071`]: Electrocute + Brace e Anchor + Guard (1 + 1 AP) pareggiano anche nel piano dello stesso Riktor. Fra unità diverse il pareggio si scioglie per `ActionId`, poi attaccante, poi intento [codice: `RTTurnManager.cpp:2819`; ADR-0003 §3]; l'ordine di dichiarazione [DECISO D-613 (3)] vale solo dentro il piano di una stessa unità. Nei due pareggi di Riktor l'ordine cambia solo il racconto, quindi le priorità restano 30 e 40 [PROPOSTA: Brace e Guard mettono stati solo su Riktor, e né l'aura né la trappola li leggono; se servisse un ordine fisso, 31 e 41 sono oggi libere nel Prep].

**Oggetti persistenti.** Si mettono accanto a `DynamicSurfaces`, `DynamicArcs` e `DynamicCovers` (`RTTurnManager.h:2653-2714`), stato di partita con `TurnsRemaining` e conto alla rovescia nel Cleanup. `CreateCover` nasce nel Prep ed è il precedente per «nasce in Prep, il Cleanup di N la scala già» (`RTTurnManager.h:2697-2701`). ⚠️ L'hash (`RTMatchStateHash.h:117-121`) copre il **valore corrente** di terreni e strutture nella mappa, **non** le liste di scadenza: due finali con durata residua diversa danno lo stesso hash. È un buco già noto, e un'aura che non sta nella mappa sarebbe del tutto invisibile all'hash.

**Visibilità.** Electrocute e Spikes sono stato di mappa o di cella, quindi pubblici; Anchor è un'azione predittiva, privata fino al boundary [PROPOSTA; AGENTS.md §4].

**Asimmetria da decidere in modo uniforme** (TR-18): nella proposta l'Anchor scatta anche su un movimento forzato nel Dash, Spikes solo su scatto o salto volontario.

### Electrocute — `Hero.Riktor.Electrocute`

> «Opzione 1 (Originale) — Electrocute [⚡ Fulmine]: Genera un'aura da 80 Danno Energetico a turno per 2 turni sull'esagono di Riktor; agisce come area denial contro i nemici in mischia e innesca le reazioni a catena.»

| Campo | Valore |
|---|---|
| Ruolo | abilità del gruppo Prep, «Originale» [CATALOGO; DECISO D-612 (2)]; posto nella dock [DOMANDA → TR-6] |
| Fase | cast in `Preparation` [CATALOGO; DECISO D-583 (1)]; danno dell'aura nel Cleanup [PROPOSTA: come `Action.Electrify`, fase Environment, che «colpisce anche chi è appena entrato», `RTCatalogLibrary.cpp:1799-1801`] |
| Costo AP | 1 [CATALOGO; DECISO D-594 (1)] |
| Cooldown | 1 [GDD riga 226]; al più una volta per piano [LETTURA D-613 (5); riserva `MSP-1.r`] |
| Priorità | 30 nel Prep [PROPOSTA: dopo Spikes 20, prima di Shield 35]; pareggia con Brace (anche quello di Riktor: le generiche sono in ogni kit, `RTUnit.cpp:2081`), SuppressiveLine e InterceptShot; fra unità diverse si scioglie per `ActionId`, nel piano di Riktor per ordine di dichiarazione [DECISO D-613 (3)], che qui cambia solo il racconto [PROPOSTA: Brace mette stati solo su Riktor, e l'aura non li legge] |
| Portata e linea di vista | 0, su sé: centro = la cella di Riktor al cast [CATALOGO «sull'esagono di Riktor»]; nessuna linea di vista [PROPOSTA] |
| Forma | `Area`, raggio 1 sulla cella di Riktor (sette celle) [PROPOSTA: il GDD dice «agli adiacenti» (riga 226); sulla sola cella di Riktor l'aura non colpirebbe nessuno finché lui ci sta, perché la cella ha un solo occupante, D-583 (4)]; [DOMANDA → TR-15] |
| Bersaglio | sé come centro; danno solo ai nemici [PROPOSTA: «contro i nemici»; alleati DOMANDA → TR-12] |
| Elemento | `Element.Electric` [CATALOGO; DECISO D-593] |
| Tipo di danno | elementale Electric [PROPOSTA; DOMANDA → TR-1]. ⚠️ Il riuso del tick ambientale produce oggi `ERTDamageSource::Environmental` (`RTCombatLibrary.h:177-181`), che non passa per la DEF né per una RES: va creato il percorso elementale |
| Danno base | 80 per tick [CATALOGO], a ciascun nemico nell'area [PROPOSTA: il GDD dà gli 80 «agli adiacenti» (riga 226), e un'aura di «area denial» colpisce chi ci sta; il catalogo non dice se si dividano; DOMANDA → TR-15], scala ×10 [PROPOSTA: coerente con i 160 di Shock Whip e con i 100 all'ingresso del Fuoco, `RTTerrainLibrary.cpp:50`]; due tick = 160 al massimo per nemico |
| Movimento | nessuno [CATALOGO] |
| Stati | nessuno stato persistente. Prima di ogni tick, la voce **istantanea** `Status.Electrified` che il colpo elettrico dell'ambiente già emette (`RTTurnManager.cpp:3806-3818`, #1324, D-315) [riuso]; nessuno stato resta sull'unità |
| Oggetti persistenti | aura di **cella** [PROPOSTA: «sull'esagono», «area denial»; DOMANDA → TR-15 cella o Riktor]: centro la cella del cast, raggio 1, tick nel Cleanup di N e di N+1 [CATALOGO «per 2 turni»; PROPOSTA sulla fase, come `CreateCover`]. Se Riktor si sposta l'aura resta. Un secondo cast sulla stessa cella rinnova la durata, non somma [PROPOSTA: come `TurnsRemaining = Turns`, `RTTurnManager.cpp:2639`]. Pubblica |
| Condizioni | nessuna sul cast; il tick colpisce i nemici nell'area al Cleanup [PROPOSTA] |
| Reazione | — . Le «reazioni a catena» del catalogo sono la propagazione della coppia Water+Electric (CURRENT): un tick su un'unità in `ShallowWater` innesca `Reaction.ElectricPropagatesThroughWater` con un `PropagationLimit` [PROPOSTA: riuso; valore DOMANDA → TR-15, `Electrify` usa 3] |
| Fallback | `Cancel` [PROPOSTA: come `Action.Electrify`, `RTCatalogLibrary.cpp:1810`]; nel Prep fallisce solo se Riktor non può agire |
| Icona | cornice Prep, glifo Electric [PROPOSTA; icona reale in `skill.af` non verificata] |

**Nel motore.**
- *Riuso*: `ERTResolutionPhase::Preparation`; `Action.Electrify` (danno elettrico e `PropagationLimit`, `RTCatalogLibrary.cpp:1809-1813`); `Reaction.ElectricPropagatesThroughWater`; lo schema dello stato di partita a scadenza (`FRTDynamicSurface`/`Arc`/`Cover`, tick nel Cleanup come `TickDynamicArcs`); `URTCombatLibrary::ApplyDamage`; `ERTAbilityShape::Area`; `Cancel`.
- *Estensione*: il tick dell'ambiente nel Cleanup (`ResolveEnvironment`) scorre anche le aure; una voce di TurnLog per il tick (danno con causa l'aura e il suo owner).
- *Creazione*: campo elemento; percorso di danno elementale; lo stato di partita «aura di cella», per esempio `FRTDynamicAura { Cell, OwnerStableUnitId, Element, DamagePerTick, Radius, TeamFilter, TurnsRemaining }` in `ARTTurnManager`, con snapshot, replay e una **copertura nuova dell'hash** per la lista (cella, owner, elemento, danno, raggio, filtro, `TurnsRemaining`); un effetto in coda a `ERTActionEffect` (per esempio `CreateAura`) [PROPOSTA]. Solo con la lettura «diviso» (TR-15): una regola di ripartizione del danno, che oggi non esiste.

**Esempio.** *Given* Riktor in (0,0,0), HP 1200, budget 6; nemico E in (1,0,0), adiacente, di fronte a Riktor, HP 1000, RES Electric 0; turno N. *When* Riktor pianifica Electrocute (1) + Shock Whip su E (2) + Move a (−1,0,0) (2) = 5 AP; E resta fermo. *Then* nel Prep di N nasce l'aura su (0,0,0), raggio 1, due turni; al Cleanup di N, nella lettura per nemico (PROPOSTA, TR-15), E subisce 80: 1000 − 160 (Whip, elementale) − 80 = 760. In N+1 Riktor resta in (−1,0,0) ed E in (1,0,0): al Cleanup di N+1 E → 680, e l'aura scade. Se E entrasse in (0,0,0), ormai libera, subirebbe gli 80 lo stesso. Lettura «solo la cella di Riktor»: E non subisce nulla.

**Domande.** TR-15 (area, danno per nemico, cella o Riktor, fase e numero dei tick, danni di ritorno in mischia, cumulo, `PropagationLimit`), TR-12 (alleati), TR-1 (tipo di danno), TR-2 (scudo base), TR-4 (ripetizione), TR-5 (cooldown), TR-6 (posto nella dock).

### Necrotic Anchor — `Hero.Riktor.NecroticAnchor`

> «Opzione 2 (Trappola Morte) — Necrotic Anchor [💀 Morte]: Impregna un esagono adiacente di energia oscura. Se un nemico lo attraversa durante la Dash Phase, perde 1 AP dal proprio pool, sfasando la sua pianificazione.»

| Campo | Valore |
|---|---|
| Ruolo | abilità del gruppo Prep, «Trappola Morte» [CATALOGO; DECISO D-612 (2)]; posto nella dock [DOMANDA → TR-6] |
| Fase | dichiarata e pagata nel Prep: cella bloccata in Planning e armata in `Preparation` [CATALOGO; DECISO D-583 (1)]; la previsione si verifica a un boundary della fase Dash [CATALOGO; PROPOSTA: azione predittiva E18, D-016]. Precedente verificato: `Hero.Ivrin.InterceptShot` è una predittiva in Preparation 30 con `LockCell` e `MovementEntry`, armata nel Prep, con cella adiacente obbligatoria (`MinRangeCells` 1, D-610 (2)) [codice: `RTHeroCatalogLibrary.cpp:1052-1062`, `RTTurnManager.cpp:4778-4865`] |
| Costo AP | 1 [CATALOGO; DECISO D-594 (1)] |
| Cooldown | 1 [PROPOSTA: il GDD non ha la voce; 1 è il minimo che, con la lettura di D-613 (5), ne vieta due nello stesso piano; DOMANDA → TR-5] |
| Priorità | 40 nel Prep [PROPOSTA: armarla non cambia lo stato del Prep]; pareggia con Guard (40), anche quello di Riktor (le generiche sono in ogni kit, `RTUnit.cpp:2081`); fra unità diverse si scioglie per `ActionId`, nel piano di Riktor per ordine di dichiarazione [DECISO D-613 (3)], che qui cambia solo il racconto [PROPOSTA: Guard mette uno stato solo su Riktor, e la trappola non lo legge] |
| Portata | 1, cella adiacente [CATALOGO], `MinRangeCells` 1 come InterceptShot [riuso]; linea di vista N/A a portata 1 [PROPOSTA] |
| Forma | `Single`, una cella, `PredictiveTargeting = LockCell` [PROPOSTA: riuso E18] |
| Bersaglio | una cella adiacente, anche occupata [PROPOSTA]; scatta solo per i nemici [CATALOGO]. Cella fuori mappa: non si arma ed è gratuita [riuso #4099, `RTTurnManager.cpp:4805-4810`] |
| Elemento | `Element.Decay` [CATALOGO; DECISO D-593] |
| Tipo di danno · Danno base | N/A · 0 [CATALOGO] |
| Movimento | che fa scattare la trappola: ogni Traversal che nel Dash entra nella cella o la attraversa, Dash e, nella proposta, anche Forced (per esempio Chain Hook) [PROPOSTA; DOMANDA → TR-18]. Un salto non attraversa celle, salvo atterrare sulla cella [PROPOSTA; DOMANDA → TR-16]. Il Move è fuori dal Dash e non la fa scattare [CATALOGO] |
| Stati | nessun tag: un modificatore di budget, −1 sul budget AP del nemico **nel turno dopo** [DECISO D-613 (1): «un piano già validato non cambia mai, quindi l'Anchor colpisce il budget del turno dopo»]. Lo «sfasando la sua pianificazione» del catalogo è superato: il turno in corso risolve intero |
| Oggetti persistenti | trappola sulla cella per il solo turno della dichiarazione [PROPOSTA: semantica di una predittiva E18; una trappola nascosta che vive più turni apre un confine di privacy nuovo, ARC-07; DOMANDA → TR-16]. Privata della squadra di Riktor fino al boundary (AGENTS.md §4); allo scatto evento pubblico nel TurnLog; se non scatta, gli avversari non la vedono mai [PROPOSTA] |
| Condizioni | al boundary del Dash, un nemico ha la cella nella propria sequenza `Entered` [PROPOSTA: stato canonico esistente, `ApplyOnEnter`, `RTTurnManager.h:2892`]; più nemici: −1 a ciascuno [PROPOSTA; DOMANDA → TR-16]. Se `Entered` includa la cella d'arrivo del Dash: non verificato |
| Reazione | — (azione predittiva, che «ha già chiesto tutto in Planning», `RTActionDef.h:494-497`) |
| Fallback | nessuno scatto = nessun effetto, AP speso [PROPOSTA]; `ERTActionFallback` `Cancel` [PROPOSTA]. L'Anchor armata **si perde** se Riktor subisce lo stato di D-319 prima del boundary, e l'AP resta speso [codice: `RTTurnManager.cpp:705-709`]. I bot oggi non pianificano predittive [DECISO D-610 (4); codice `RTTurnManager.cpp:4795-4805`]: DOMANDA → TR-16 |
| Icona | cornice Prep, glifo Decay [PROPOSTA; icona reale non verificata] |

**Nel motore.**
- *Riuso*: `ERTPredictiveTargeting::LockCell` e il campo `PredictiveTargeting` (`RTActionDef.h:477-489`, `:839`); `MinRangeCells` 1; la sequenza `Entered` del Dash (`ApplyOnEnter`); il TurnLog; la regola di #4099.
- *Estensione*: `ERTPredictionBoundary` con un valore nuovo **in coda** (per esempio `DashEntry`), che come `MovementEntry` (`RTActionDef.h:507-514`) guarda chi ha attraversato la cella. Non basta: oggi il boundary si chiama solo alla fine della risoluzione del Move, e l'esito predittivo è un colpo che **tronca** la rotta (`RTTurnManager_Movement.cpp:1586-1593`; `FRTPredictiveShot`, `ApplyPredictionsToMoves`, `RTTurnManager.cpp:7846-7950`). Servono: una chiamata del boundary nella risoluzione del Dash, un tipo di esito predittivo che applica il modificatore di budget senza toccare la rotta, e il `TriggerMatched` nel TurnLog. `ValidatePlan` legge il budget dell'unità per il turno invece della costante (`Turn/Actions/RTPlanValidationLibrary.h:128-132`).
- *Creazione*: il budget AP per unità e per turno in snapshot, hash e replay [DECISO D-613 (1); dipende da E38, non implementato], con la versione dello schema [LETTURA D-613 (5)] e con il modificatore e la sua causa [PROPOSTA: l'anteprima di N+1 mostra il budget con la causa, come nell'esempio]; un effetto in coda a `ERTActionEffect` (per esempio `ModifyNextTurnActionPoints`, Amount −1); la voce di TurnLog «budget del turno dopo −1, causa Anchor di Riktor»; campo elemento.

**Esempio.** *Given* Riktor in (0,0,0); nemico K in (1,1,0), budget 6, HP 1000; turno N. Riktor pianifica Necrotic Anchor sulla cella (0,1,0), adiacente (1 AP). K pianifica un Reposition (Dash, `LinearDash` a portata 2 [codice: `RTCatalogLibrary.cpp:1451-1453`]) lungo la riga r = 1 da (1,1,0) a (−1,1,0), che attraversa (0,1,0) come cella **intermedia**, più Blast e Move: 2 + 2 + 2 = 6, validato. *When* il turno risolve e al boundary del Dash la rotta di K contiene (0,1,0). *Then* il piano di K del turno N risolve intero; il TurnLog registra «Anchor: K −1 AP sul turno N+1»; nel planning di N+1 i pip di K mostrano 5 con la causa, e un piano da 6 è rifiutato con `OverBudget`. Se K avesse attraversato (0,1,0) col Move invece che col Dash: nessun effetto.

**Domande.** TR-16 (durata, visibilità, salto, cumulo e minimo, più nemici, icona di stato, bot), TR-18 (movimento forzato), TR-5 (cooldown), TR-4 (ripetizione), TR-6 (posto nella dock).

### Magnetic Spikes — `Hero.Riktor.MagneticSpikes`

> «Opzione 3 (Magnetismo Terra) — Magnetic Spikes [🪨 Terra]: Magnetizza il terreno (aumentando il costo di movimento di +1 AP). Se un nemico compie uno scatto o un salto nell'area, subisce 50 danni fisici puri e viene ancorato a terra ($Z=0$).»

| Campo | Valore |
|---|---|
| Ruolo | abilità del gruppo Prep, «Magnetismo Terra» [CATALOGO; DECISO D-612 (2)]; posto nella dock [DOMANDA → TR-6] |
| Fase | cast e magnetizzazione in `Preparation` [CATALOGO; DECISO D-583 (1)]; danno e ancoraggio nel Dash, all'ingresso nell'area [CATALOGO «uno scatto o un salto»] |
| Costo AP | 1 [CATALOGO; DECISO D-594 (1)] |
| Cooldown | 2 [PROPOSTA: pari alla durata proposta dell'area, così due aree dello stesso Riktor non si sovrappongono; DOMANDA → TR-5] |
| Priorità | 20 nel Prep [PROPOSTA: cambia il terreno, va prima di ogni lettura del terreno nel Prep] |
| Portata e linea di vista | 1–3 con linea di vista [PROPOSTA: come Iron Maiden «entro 3 esagoni»; il catalogo ha anche 2–4 (Chain Hook); DOMANDA → TR-17] |
| Forma | `Area`, raggio 1 attorno alla cella bersaglio (sette celle) [PROPOSTA; DOMANDA → TR-17] |
| Bersaglio | una cella, anche occupata; danno e ancoraggio solo ai nemici [CATALOGO]; il costo di movimento vale per tutti, è una proprietà della cella [PROPOSTA; DOMANDA → TR-17] |
| Elemento | `Element.Earth` [CATALOGO; DECISO D-593] |
| Tipo di danno | [DOMANDA → TR-3] «fisici puri»: `Direct` con la DEF [PROPOSTA: «fisici» = diretto], oppure un tipo che salta DEF, copertura e Guard; che cosa fa Earth su un colpo diretto [DOMANDA → TR-1]. ⚠️ Il riuso degli `OnEnterEffects` del terreno produce oggi danno `Environmental`, che non passa per `MitigateDirectDamage` (`RTCombatLibrary.h:888`): per un danno `Direct` serve un percorso nuovo |
| Danno base | 50 per ingresso, una volta per unità per turno [CATALOGO 50; PROPOSTA «una volta»: senza, uno scatto che attraversa tre celle pagherebbe tre volte] |
| Movimento | effetto sul movimento altrui: «+1 AP» → **+1 MP per cella** [CONTRACT CONFLICT con D-584 (2): gli AP pagano il profilo, i MP la distanza col costo per cella (D-117) → PROPOSTA +1 MP]. Il costo vale dal turno **dopo** il cast [PROPOSTA: il codice cambia le superfici solo nel Cleanup per non alterare un percorso già calcolato, `RTCatalogLibrary.cpp:1816-1818`; DOMANDA → TR-17]. Lo scatto che entra nell'area non si ferma [PROPOSTA; DOMANDA → TR-17] |
| Stati | stato **nuovo** `Status.Grounded` («ancorato a terra», Z = 0) [PROPOSTA, referto §3.2]: vieta il salto (`Leap`) e il cambio di `Layer`; camminare resta permesso, ed è ciò che lo distingue da `Status.Root`. Inizio: all'ingresso; durata: fino alla fine del turno dopo [PROPOSTA; convenzione del tick come `Status.Prone`, D-319, non verificata]; [DOMANDA → TR-17: Z = 0 fa anche cadere al Layer 0?] |
| Oggetti persistenti | zona di sette celle magnetizzate, due turni [PROPOSTA; DOMANDA → TR-17], conto alla rovescia nel Cleanup come le superfici dinamiche (`RTTurnManager.cpp:2634-2639`); pubblica, perché la superficie corrente sta nella mappa. La superficie corrente entra nell'hash attraverso la mappa, la sua scadenza no (buco noto, `RTMatchStateHash.h:117-121`). Una superficie dinamica sostituisce quella che c'era: l'acqua sotto le punte smetterebbe di condurre [DOMANDA → TR-17] |
| Condizioni | danno e ancoraggio: un nemico entra in una cella dell'area con un'azione `FastMovement` (Charge, Reposition, Leap all'atterraggio) [PROPOSTA; stato canonico: `Entered` e la fase passata a `ApplyOnEnter`, `RTTurnManager.h:2868`, `:2892`]. Il Move e il movimento forzato non lo fanno scattare [CATALOGO «uno scatto o un salto»; PROPOSTA sul forzato, DOMANDA → TR-18] |
| Reazione | — |
| Fallback | `Cancel` se la cella bersaglio non è valida [PROPOSTA]; sulle celle la cui superficie non si può sostituire (Void) l'area vale solo sulle celle valide [PROPOSTA; DOMANDA → TR-17] |
| Icona | cornice Prep, glifo Earth [PROPOSTA] |

**Nel motore.**
- *Riuso*: la routine che trasforma una superficie per N turni (`DynamicSurfaces.FindOrAdd`, originale preservato, voce di TurnLog `SurfaceChanged`, `RTTurnManager.cpp:2632-2654`); `MakeTerrain` (`RTTerrainLibrary.cpp:12-15`); `OnEnterEffects` con Damage e Status (`RTTerrainData.h:103`), già usato dal Fuoco; `ApplyOnEnter` come imbuto unico; `ERTAbilityShape::Area`.
- *Estensione*: `Rough` **non** si riusa così com'è: costa 2 MP ma ha `bBlocksDashCharge = true` (`RTTerrainLibrary.cpp:40`), quindi fermerebbe lo scatto invece di punirlo. `FRTTerrainDef`: un filtro di fase o di famiglia sugli `OnEnterEffects` (oggi valgono in Dash, Move, Cleanup e forzato) e un limite di una volta per unità per turno. Il pathfinding del planning legge il costo nuovo dal turno dopo.
- *Creazione*: ⚠️ una superficie `Magnetized` con `MoveCost` 2 è un costo **assoluto**, non un +1: la routine riusata scrive `MoveCost` dal catalogo della superficie nuova e cambia la mappa **subito**, nel Prep (`RTTurnManager.cpp:2643-2646`), mentre `ShallowWater`, `Rough` e Fire costano già 2 (`RTTerrainLibrary.cpp:40-49`). Una superficie sola non può insieme valere dal turno dopo per il costo e dal Dash di N per il danno. [PROPOSTA] un **modificatore di costo come delta** (+1 MP), separato dalla superficie e attivo da N+1, più un effetto d'ingresso attivo dal Dash di N; altrimenti [DOMANDA → TR-17]: costo assoluto 2 (+0 su acqua e Rough) o +1 sul costo che c'è. Inoltre: `Status.Grounded` in `Core/RTGameplayTags` (letto dal validatore di Leap e dalle transizioni di `Layer`, glossario, icona); il permesso di cambiare una superficie o la sua zona da un'azione di Preparation (oggi lo fanno solo `Ignite` e `CreateWater`, nel Cleanup); campo elemento; percorso di danno `Direct` per l'effetto d'ingresso (oppure TR-3).

**Esempio.** *Given* Riktor in (0,0,0); area su (2,0,0), raggio 1; nemico D in (4,0,0), HP 1000, DEF 0, 4 MP; nemico W in (4,−1,0), 4 MP; turno N. *When* nel Prep di N nascono le sette celle; nel Dash di N, D fa uno scatto verso ovest e attraversa (3,0,0), nell'area. *Then* D subisce 50 (→ 950) una sola volta e riceve `Status.Grounded`: nel planning di N+1 non può pianificare un Leap né cambiare Layer, ma può camminare. In N W cammina con i costi di prima; in N+1 un percorso di W attraverso tre celle dell'area costa 6 MP invece di 3, e con 4 MP l'anteprima non mostra raggiungibile la cella oltre l'area. Nel Cleanup di N+1 la zona scade.

**Domande.** TR-17 (area, portata, durata, momento e forma del costo, scatto, Grounded, acqua, alleati, Void), TR-3 («puri»), TR-1 (Earth su un colpo fisico), TR-18 (movimento forzato), TR-5 (cooldown), TR-4 (ripetizione), TR-6 (posto nella dock).

---

## 3. Dash

Gruppo del catalogo: «3. Dash Phase (Costante: 2 AP)».

**Scelte comuni.** 2 AP, classe Dash [CATALOGO; DECISO D-594 (1)]. Cooldown: solo Chain Hook ha un input (2) [GDD riga 227]; per Voltage Leap e Riot Charge 2 ereditato dal core (`Action.Leap`, `Action.Charge`/`Hero.Branth.Ram`) [PROPOSTA]; al più una volta per piano [LETTURA D-613 (5); riserva `MSP-1.r`]. La cornice Dash vale per tutte e tre, ma gli effetti sugli altri (danno, spinta, trazione, scudo) si applicano nel Blast, lo schema di `Action.Charge` («Gli effetti sono dichiarati qui, ma si applicano nel Blast», `RTCatalogLibrary.cpp:1419`). Ci arrivano solo attraverso `PendingChargeImpacts`, che oggi riempiono soltanto `LinearCharge` (`Stop == Impact`) e `LinearPass` (`PassedThroughUnitIds`) [codice: `RTTurnManager.cpp:5376-5401`]; ogni impatto è poi un intento `Single` a portata 1 col Def della mobilità (`RTTurnManager_Blast.cpp:1681-1718`).

**Priorità dentro FastMovement.** Oggi: Leap 25 · Dodge 30 (`RTCatalogLibrary.cpp:1414`) · PassingBlade 30 · Charge 35 · Reposition 40. Dentro FastMovement la risoluzione è **simultanea**, e la priorità decide **solo** le contese di cella (il numero basso entra, `BlockedByPriority`) [codice: `RTHexSimLibrary.h:315-324`, `RTTurnManager.cpp:5500`]: non mette in sequenza i Dash. Proposte: Voltage Leap 25, Riot Charge 35, Chain Hook 40 in Control nella lettura (a).

**Fallback.** `Stop` per Voltage Leap e Riot Charge, obbligatorio per una mobilità: il validatore decide sulla **fase** (`FastMovement`/`NormalMovement` → serve `Stop`, `RTCatalogLibrary.cpp:316-320`). Spostamenti in celle, mai in spicchi [DECISO D-583 (4)].

**Due Dash nello stesso piano** (per esempio Voltage Leap + Riot Charge, 4 AP) sono legali per budget, ma oggi lo slot Movement di D-191 li esclude a vicenda, e ADR-0011 destina lo slot a cadere [DOMANDA → TR-30].

### Chain Hook / Chain Pull — `Hero.Riktor.ChainHook`

> «Opzione 1 (Originale) — Chain Hook / Chain Pull [🪨 Terra]: Aggancia un bersaglio (gittata 2–4 esagoni) e lo trascina di 2 caselle (o tira Riktor verso un muro), infliggendo 140 Danno Fisico e sradicando i cecchini.»

| Campo | Valore |
|---|---|
| Ruolo | abilità del gruppo Dash, «Originale» [CATALOGO; DECISO D-612 (2)]; posto nella dock [DOMANDA → TR-6]. Il GDD la chiama Chain Pull, slot Base, Dash, 2 AP, CD 2 (riga 227) |
| Fase | cornice Dash e classe di costo [CATALOGO]. ⚠️ Due letture [DOMANDA → TR-19]. **(a)** [PROPOSTA] `ResolutionPhase = Control`, priorità 40, `Cancel`, **come `Action.Pull`** (`RTCatalogLibrary.cpp:1751`): «Dash» resta solo la cornice dell'icona e il costo dichiarato sulla riga; la trazione avviene nel Blast prima di ogni Attack (30 < 40), quindi il cecchino è spostato prima che spari. In fase `FastMovement` con `Cancel` il validatore la **rifiuta** (`RTCatalogLibrary.cpp:316-320`), e un'azione FastMovement senza movimento non produce impatti, quindi la trazione non si applicherebbe mai. **(b)** trazione vera dentro il Dash: richiede `Stop` e uno spostamento forzato di un'altra unità nel passo FastMovement, che non esiste |
| Costo AP | 2 [CATALOGO; classe Dash, DECISO D-594 (1)] |
| Cooldown | 2 [GDD riga 227]; al più una volta per piano [LETTURA D-613 (5); riserva `MSP-1.r`] |
| Priorità | (a) 40 in Control, come `Action.Pull` [PROPOSTA]. (b) la priorità decide solo le contese di cella: per leggere le posizioni dopo il Dash serve un **passo nuovo** dopo `ResolveHexPaths`; la priorità è [DOMANDA → TR-19] |
| Portata e linea di vista | 2–4 [CATALOGO; GDD], `MinRangeCells` 2 (`RTActionDef.h:896`); `ERTLineOfSightPolicy::Required`, il default (`RTActionDef.h:860`) [PROPOSTA: la catena è una linea; il matchup dice di Steel che «le barriere bloccano le traiettorie di trazione»]. Il bersaglio deve stare su una delle sei direttrici (GDD riga 222)? [DOMANDA → TR-19] |
| Forma | `Single` [PROPOSTA: «un bersaglio»] |
| Bersaglio | ramo 1: un'unità nemica [CATALOGO; PROPOSTA: solo nemici, come `Action.Pull`]; ramo 2: una cella muro od ostacolo, verso la quale si sposta Riktor [CATALOGO]. [PROPOSTA] il ramo lo sceglie il **tipo** di bersaglio cliccato (unità → Pull, cella muro → `SelfReposition`): deterministico, leggibile dal piano, senza un campo di modalità. Alleati agganciabili e ostacoli validi per il ramo 2: [DOMANDA → TR-19] |
| Elemento | `Element.Earth` [CATALOGO; DECISO D-593] |
| Tipo di danno | `Direct`, DEF [PROPOSTA: «Danno Fisico», D-589 (1)]; che cosa fa Earth su un colpo diretto [DOMANDA → TR-1] |
| Danno base | 140 [CATALOGO; GDD]; nel ramo muro: [DOMANDA → TR-19] |
| Movimento | ramo 1, famiglia Forced (Pull): il bersaglio va verso la cella di Riktor; `PullToward` = cella dell'attaccante, `PullDist = Event.Amount` (`RTTurnManager.cpp:7082-7084`), quindi Pull(2) si esprime già. «2 caselle» [CATALOGO] o «nell'esagono adiacente a Riktor» [GDD]: [PROPOSTA] «fino all'adiacente, al più 2 celle», che non finisce mai sulla cella di Riktor; [DOMANDA → TR-19]. Una trazione bloccata usa `ERTDisplacementBlockReason` (`RTTurnLog.h:787-810`): `NoDestination` (cella occupata od ostacolo), `ContestedDestination` (due spostati verso la stessa cella), `Guarded`/`Braced`. Ramo 2, famiglia Dash (`SelfReposition`): Riktor verso il muro fino alla cella libera adiacente [PROPOSTA]; la contesa perfetta resta `BlockedContested` senza danno [DECISO D-590 (4)] |
| Stati | nessun tag [PROPOSTA]. «Sradicando i cecchini»: azzerare la `CoverSelection` del bersaglio, o basta lo spostamento [DOMANDA → TR-19]; se uno spostamento forzato la azzeri già oggi: non verificato |
| Oggetti persistenti | nessuno |
| Condizioni | ramo 1: unità a distanza 2–4 con linea di vista; ramo 2: la cella è un muro od ostacolo (topologia canonica) [PROPOSTA] |
| Reazione | — . Fa scattare sul bersaglio `AboutToBeDisplaced` (`Reaction.Anchor` → `CancelDisplacement`) e, per il danno `Single` con `bCountsAsAttack`, `HitByDirectAttack` (Counter, Deflect) [codice: `RTActionDef.h:283-286`, `:296-323`]. **Non** `AboutToReceiveControl`, che riguarda uno stato di controllo; lo spostamento lo fermano anche Guard e Brace |
| Fallback | `Cancel` [PROPOSTA: come `Action.Pull`, `RTCatalogLibrary.cpp:1752`]. Il ramo muro è una mobilità: se in fase di movimento vuole `Stop`, e con una sola fallback per azione i due rami sono in conflitto [DOMANDA → TR-19: due voci, o una fallback per ramo] |
| Icona | cornice Dash (chevron giallo), glifo Terra, badge »» [lettura di Appendice B, da confermare] |

**Nel motore.**
- *Riuso*: `Action.Pull` (`ERTActionEffect::Pull` a distanza parametrica, `bCountsAsAttack`); `MinRangeCells`; `Required`; `Damage`; `SelfReposition`; `ERTDisplacementBlockReason`; i trigger `AboutToBeDisplaced` e `HitByDirectAttack`; clip `Ability_Hook` del pack Paragon (§2 del referto).
- *Estensione*: voce costruita da `Action.Pull` (`MakeHeroActionFromCore`), `RangeCells` 4, `MinRangeCells` 2, effetti { Damage 140, Pull 2 }, cooldown 2, fase Control (lettura a).
- *Creazione*: campo elemento; il ramo per tipo di bersaglio nella risoluzione degli effetti, oppure due voci; solo con la lettura (b): lo spostamento forzato di un'altra unità nel passo FastMovement (`RTHexSimLibrary.cpp`) con un passo dopo `ResolveHexPaths` e un evento nel TurnLog.

**Esempio.** *Given* Riktor in (0,0,0), budget 6, cooldown pronto; cecchino nemico S sulla stessa direttrice, HP 1000, DEF 0; nessun ostacolo e nessuna reazione armata. *When* Riktor pianifica Chain Hook su S (2 AP) e il turno risolve. *Then*, con S in (4,0,0): «2 fisse» → S in (2,0,0); GDD «nell'esagono adiacente a Riktor», senza tetto → S in (1,0,0); PROPOSTA «fino all'adiacente, al più 2» → S in (2,0,0); in tutti i casi HP 1000 → 860. Il caso che separa «2 fisse» dalla PROPOSTA è S in (2,0,0): con «2 fisse» la cella d'arrivo è quella di Riktor, occupata (`NoDestination`); con la PROPOSTA S va in (1,0,0). Con la lettura (a) lo spostamento avviene nel Blast in Control, prima che S risolva il suo Attack.

**Domande.** TR-19 (fase, distanza, ramo muro, cecchini, alleati, direttrici, fallback), TR-1 (Earth su un colpo fisico), TR-18 (la trazione sulle celle di Anchor e Spikes), TR-30 (due Dash nello stesso piano), TR-4 (ripetizione), TR-5 (cooldown), TR-6 (posto nella dock).

### Voltage Leap — `Hero.Riktor.VoltageLeap`

> «Opzione 2 (Sbalzo Fulmine) — Voltage Leap [⚡Fulmine]: Un salto di 2 esagoni. All'atterraggio, sprigiona un'onda d'urto da 90 danni ad area che spinge i nemici adiacenti indietro di 1 micro-spicchio (provocando Body Block se collidono).»

| Campo | Valore |
|---|---|
| Ruolo | abilità del gruppo Dash [CATALOGO; DECISO D-612 (2)]; posto nella dock [DOMANDA → TR-6]. Il GDD non ha la voce |
| Fase | `FastMovement`, come `Action.Leap`; l'onda d'urto si applica nel Blast come gli effetti della carica [PROPOSTA; referto §3.2] |
| Costo AP | 2 [CATALOGO; DECISO D-594 (1)] |
| Cooldown | 2 [PROPOSTA: eredita da `Action.Leap`, `RTCatalogLibrary.cpp:1445`]; al più una volta per piano [LETTURA D-613 (5)] |
| Priorità | 25 [PROPOSTA: eredita `Action.Leap`, `RTCatalogLibrary.cpp:1444`; decide solo le contese della cella d'atterraggio]. L'onda nel Blast, con lo schema attuale, porta la priorità del Def della mobilità, cioè 25; una priorità di controllo 40 come Push è [PROPOSTA] solo se si crea un campo separato |
| Portata e linea di vista | salto di 2 [CATALOGO]; massima 2, minima 1 [PROPOSTA; DOMANDA → TR-20: esattamente 2 o fino a 2]. Linea di vista N/A: `LinearLeap` ignora unità e celle intermedie (`RTActionDef.h:398`); la cella d'atterraggio dev'essere percorribile e libera (`RTCatalogLibrary.cpp:1442-1443`) |
| Forma | movimento `LinearLeap` su una delle sei direzioni, poi `Area` raggio 1 sulla cella d'atterraggio [CATALOGO «ad area», «adiacenti»; PROPOSTA raggio 1] |
| Bersaglio | il salto: una cella; l'onda: i nemici adiacenti all'atterraggio [CATALOGO]; anche gli alleati? [PROPOSTA: solo nemici, coerente con la spinta; DOMANDA → TR-12] |
| Elemento | `Element.Electric` [CATALOGO; DECISO D-593] |
| Tipo di danno | il catalogo non lo dà: [PROPOSTA] elementale Electric, come le altre voci ⚡; oppure `Direct` per un'«onda d'urto» [DOMANDA → TR-20] |
| Danno base | 90 per nemico adiacente [CATALOGO; PROPOSTA «per cella colpita»; DOMANDA → TR-20, lo stesso dubbio di Static Sweep] |
| Movimento | Riktor: famiglia Dash, `LinearLeap`, 2 celle; lo fermano una cella d'atterraggio occupata o impercorribile e la contesa (`BlockedContested`, `BlockedByPriority`, `RTHexSimLibrary.h:323-324`). Nemici adiacenti: famiglia Forced (Push), 1 cella [CATALOGO «1 micro-spicchio» → 1 cella, DECISO D-583 (4)], via dalla cella d'atterraggio; `KnockFrom` = cella dell'attaccante (`RTTurnManager.cpp:7073`), che a quel punto si assume sia l'atterraggio [non verificato]. «Body Block se collidono» = spinta non applicata con `ERTDisplacementBlockReason::NoDestination` («unità subito dietro», `RTTurnLog.h:799-800`); due spinti verso la stessa cella = `ContestedDestination`. Non è la contesa fra movimenti di D-590 (4). ⚠️ D-445 dichiara che «sparisce il body-block» per il movimento: il termine dell'autore va disambiguato [DOMANDA → TR-20]. Un danno d'urto della spinta bloccata: non verificato [DOMANDA → TR-20] |
| Stati · Persistenti | nessuno [CATALOGO] |
| Condizioni | l'onda parte solo se Riktor atterra sulla cella pianificata [PROPOSTA]; si appoggia a `InterruptBeforeEffect`, la policy di `Action.Leap` (`RTCatalogLibrary.cpp:1447`) |
| Reazione | — . La spinta fa scattare `AboutToBeDisplaced` degli spinti; l'onda è `Area`, quindi **non** fa scattare `HitByDirectAttack` |
| Fallback | `Stop` [obbligatorio per una mobilità]; con il salto fermato prima, l'onda non parte [PROPOSTA; DOMANDA → TR-20] |
| Icona | cornice Dash, glifo saetta, badge »» ed eventualmente quota [lettura di Appendice B] |

**Nel motore.**
- *Riuso*: `Action.Leap` (FastMovement 25, `LinearLeap`, `InterruptBeforeEffect`, `Stop`); `ERTAbilityShape::Area` con `AreaRadius` 1; `Damage`; `Push` 1 (pipeline del knockback, `RTTurnManager.cpp:7064-7080`); `ERTDisplacementBlockReason`.
- *Estensione*: nessuna estensione basta.
- *Creazione*: oggi un `LinearLeap` **non produce nessun effetto** nel Blast. Serve un impatto d'area centrato sulla cella d'atterraggio, prodotto nel Dash accanto a `PendingChargeImpacts` e consumato da `AppendChargeImpactIntents` con forma `Area` raggio 1 (`RTTurnManager_Blast.cpp:1700-1718`); campo elemento. Solo con la lettura «diviso» (TR-20): una regola di ripartizione del danno, che oggi non esiste.

**Esempio.** *Given* Riktor in (0,0,0), budget 6; nemico A in (3,0,0), nemico B in (2,1,0), nemico C in (4,0,0) dietro A; HP 1000, DEF 0, RES 0; (2,0,0) libera e percorribile. *When* Riktor pianifica Voltage Leap su (2,0,0) (2 AP) e nessun altro vi punta. *Then* Riktor atterra in (2,0,0); nel Blast, nella lettura per nemico (PROPOSTA, TR-20), A → 910, ma la sua spinta verso (4,0,0) è bloccata da C: A resta in (3,0,0), `NoDestination`, senza danno aggiuntivo (danno d'urto: TR-20); B → 910 e spinto in (2,2,0); C intatto. Variante: un nemico contende (2,0,0) con un movimento della stessa fase: Riktor non atterra e l'onda non parte (PROPOSTA).

**Domande.** TR-20 (distanza, danno per nemico e tipo, Body Block e D-445, danno d'urto, onda col salto fermato), TR-12 (alleati), TR-13 (reazioni su un colpo d'area), TR-30 (due Dash nello stesso piano), TR-2 (scudo base), TR-4 (ripetizione), TR-5 (cooldown), TR-6 (posto nella dock).

### Riot Charge — `Hero.Riktor.RiotCharge`

> «Opzione 3 (Carica Scudo) — Riot Charge [🛡Scudo]: Una carica lineare e inarrestabile di 3 esagoni che spazza via chiunque si trovi sulla traiettoria e conferisce a Riktor un'armatura temporanea da 150 HP alla fine del movimento.»

| Campo | Valore |
|---|---|
| Ruolo | abilità del gruppo Dash [CATALOGO; DECISO D-612 (2)]; posto nella dock [DOMANDA → TR-6]. Erede di `Hero.Branth.Ram` (`RTHeroCatalogLibrary.cpp:893`) [LETTURA D-612 (7)] |
| Fase | `FastMovement` [CATALOGO]; effetti su chi è attraversato nel Blast, come le mobilità che fanno danno [PROPOSTA] |
| Costo AP | 2 [CATALOGO; DECISO D-594 (1)] |
| Cooldown | 2 [PROPOSTA: eredita `Action.Charge`/`Ram`, `RTCatalogLibrary.cpp:1431`, `RTHeroCatalogLibrary.cpp:894`]; al più una volta per piano [LETTURA D-613 (5)] |
| Priorità | 35 [PROPOSTA: eredita Charge/Ram; decide solo le contese di cella con le altre mobilità del Dash] |
| Portata | 3 in linea retta su una delle sei direzioni [CATALOGO; come `Action.Charge`]; minima 1 [PROPOSTA]; linea di vista N/A |
| Forma | `LinearPass`: colpisce ogni unità attraversata (`RTActionDef.h:399-409`); lo scudo su Riktor [CATALOGO] |
| Bersaglio | la cella di destinazione; effetti su «chiunque» sulla traiettoria [CATALOGO], quindi anche gli alleati [PROPOSTA: solo nemici; DOMANDA → TR-21]; scudo: Riktor |
| Elemento | `Element.Aegis` [CATALOGO 🛡; DECISO D-593] |
| Tipo di danno | il catalogo non dichiara un danno [DOMANDA → TR-21]; se c'è, `Direct` [PROPOSTA: una RES Aegis come bersaglio di un colpo non ha una lettura] |
| Danno base | non dato [CATALOGO], non si inventa [DOMANDA → TR-21]; confronto: `Ram`/`Charge` 200 sul primo nemico (`RTCatalogLibrary.cpp:1432`), `PassingBlade` 200 su ogni unità attraversata (`RTHeroCatalogLibrary.cpp:1084`), che non sono proposte per questa voce. Scudo 150 [CATALOGO] |
| Movimento | Riktor: famiglia Dash, `LinearPass`, 3 celle, attraversa le unità e prosegue [PROPOSTA per «spazza via» e «inarrestabile»]. Cella finale occupata: la governa [DECISO D-398]: un attraversamento è un arco solo, copre le celle occupate consecutive più la prima libera, e senza una cella libera non si attraversa (`RTHexSimLibrary.cpp:1712-1724`; MOV-14 chiusa da D-452). Lettura alternativa di «spazza via»: spinta laterale, che il motore non ha (la spinta parte dalla cella dell'attaccante, `RTTurnManager.cpp:7073`) [DOMANDA → TR-21] |
| Stati | nessun tag. L'«armatura temporanea» è l'effetto `Shield` (`AddTemporaryShield`), che oggi scade nel Cleanup (#96) [riuso del concetto]; durata oltre il Cleanup [DOMANDA → TR-21] |
| Persistenti | nessuno |
| Condizioni | lo scudo si applica «alla fine del movimento» [CATALOGO], anche se la corsa è fermata prima, perché `Stop` la conclude sulla cella raggiunta [PROPOSTA; DOMANDA → TR-21] |
| Reazione | — . Se l'effetto di passaggio fa danno e il Def ha `bCountsAsAttack`, scatta `HitByDirectAttack` di chi è attraversato: gli impatti diventano intenti `Single` (`RTTurnManager.cpp:5389-5401`, `RTTurnManager_Blast.cpp:1700-1705`) |
| Fallback | `Stop` [obbligatorio]. «Inarrestabile» non può voler dire «senza Stop». Immunità all'interruzione: `InterruptPolicy = None` esiste («l'`Action.Interrupt` non ha effetto», `RTActionDef.h:114-120`) [PROPOSTA: riuso]; immunità a Root e spostamenti non esiste [DOMANDA → TR-21] |
| Icona | cornice Dash, glifo Scudo (favo ciano), badge »» [lettura di Appendice B] |

**Nel motore.**
- *Riuso*: `ERTMovementStyle::LinearPass` (precedente `Ivrin.PassingBlade`, `RTHeroCatalogLibrary.cpp:1082-1090`); la base `Action.Charge`/`Ram` (FastMovement, portata 3, CD 2, priorità 35, slot Movement ereditato, D-191); `InterruptPolicy::None`; il validatore `Stop`. `ERTActionEffect::Shield`/`AddTemporaryShield` esistono **solo come effetto e come metodo**.
- *Estensione*: voce con `LinearPass`, `RangeCells` 3, cooldown 2, effetti di passaggio (da decidere) più Shield 150.
- *Creazione*: ⛔ **nessun punto del motore applica lo Shield dichiarato da un'azione Dash.** `RTTurnManager.cpp:4979-4980` è il percorso del Prep (`TargetUnitId` = chi le usa); `:6050-6051` sta in `RunReactionPass` (applica lo Shield delle reazioni); nel ciclo degli effetti dei colpi lo Shield cade nel ramo di default («cure e scudi non appartengono a un colpo a segno»); il sito degli effetti su sé delle mobilità applica solo `Status` (`RTTurnManager_Movement.cpp:1831-1835`). Va creato: un case `Shield` nel sito degli effetti su sé delle mobilità (o un destinatario per effetto in `FRTActionEffectSpec`: oggi `bSelfTarget` è per azione, `RTActionDef.h:790`), applicato **alla fine del Dash** perché valga già nel Blast. Il referto (riga 72, ARC-11) aveva dato lo Shield per riuso: è corretto qui. Poi: campo elemento; uno scudo con durata, se l'autore lo vuole oltre il Cleanup (snapshot, hash); una spinta perpendicolare alla corsa, se «spazza via» è una spinta.

**Esempio.** *Given* Riktor in (0,0,0), HP 1200, budget 6; nemico A in (1,0,0), nemico B in (2,0,0), (3,0,0) libera; un nemico C fuori dalla traiettoria pianifica un attacco su Riktor per il Blast. *When* Riktor pianifica Riot Charge verso (3,0,0) (2 AP). *Then* nel Dash Riktor attraversa A e B e finisce in (3,0,0); A e B restano nelle loro celle, il loro danno è TR-21. Lo scudo da 150 è su Riktor prima che il colpo di C risolva **solo se** si crea l'applicazione alla fine del Dash (PROPOSTA che richiede codice nuovo); allora un colpo che dopo la mitigazione vale D toglie min(D, 150) allo scudo e il resto agli HP (ordine fra scudo, copertura e Guard non verificato). Nel Cleanup lo scudo residuo scade. Variante: (3,0,0) occupata da un alleato fermo: per D-398 l'arco si estende alla prima cella libera successiva, se è entro la corsa; altrimenti non si attraversa. Va scritta come criterio derivato da D-398.

**Domande.** TR-21 (effetto sulla traiettoria, alleati, «inarrestabile», durata e condizione dello scudo, esito diverso da D-398), TR-30 (due Dash nello stesso piano), TR-4 (ripetizione), TR-5 (cooldown), TR-6 (posto nella dock).

---

## 4. Blast primaria

Gruppo del catalogo: «4. Blast Phase - Abilità Primaria (Costante: 2 AP)».

**Scelte comuni.** 2 AP [CATALOGO]; coincide con la classe Blast [DECISO D-594 (1)], costo dichiarato per riga [DECISO D-594 (3)]. Fase Blast, segmento `Attack` [PROPOSTA], priorità Arc Discharge 60 < Corrosive Shackles 65 < Seismic Smash 80 [PROPOSTA, riuso di `PrecisionAttack`/`CircularAoE`/`HeavyAttack`, `RTCatalogLibrary.cpp:1455-1509`]. ⚠️ Nel Blast il danno è **simultaneo**: le cure si applicano dopo il danno qualunque sia la priorità, e gli spostamenti accodati ai colpi dopo le cure (`ApplyCombatState` → `ApplyPlannedHeals` → `ApplyDisplacements`, `RTTurnManager.cpp:7434-7446`) [DECISO D-503 (1)-(4)]. La priorità ordina solo ciò che dipende dall'ordine (marchio, racconto); fasce che spezzino la simultaneità sono state scartate [DECISO D-503 (3)]. Cooldown: solo Shackles ha un input (2); 2 per Smash e Arc [PROPOSTA]: con 0 e la lettura sulla ripetizione, tre copie (660 / 540) supererebbero la raffica di Shock Whip (480), ma a vietarle nello stesso piano basterebbe già 1 [LETTURA D-613 (5)]. 2 e non 1 per parità con Shackles (stesso gruppo, stesso costo, GDD 2) e per la cadenza: il cooldown si imposta all'uso e scala nel Cleanup dello stesso turno [codice: `RTUnit.cpp:2324-2326`, `RTTurnManager.cpp:2108`], quindi con 2 la primaria usata in N torna in N+2; con 1 tornerebbe già in N+1, a ogni turno, senza lasciare all'attacco base un turno di ricarica. Danno per AP 90–110 contro gli 80 di Shock Whip: la primaria vale più dell'attacco base, che riempie i turni di ricarica [PROPOSTA; referto §3.4].

### Corrosive Shackles — `Hero.Riktor.CorrosiveShackles`

> «Opzione 1 (Originale) — Corrosive Shackles [💀 Morte]: Infligge 200 Danno Oscuro in un raggio di 2 esagoni e blocca completamente le cure ricevute dal bersaglio per 2 turni.»

| Campo | Valore |
|---|---|
| Ruolo | primaria di Blast, «Originale» [CATALOGO; DECISO D-612 (2)]; posto nella dock [DOMANDA → TR-6] |
| Fase | Blast · `Attack` [PROPOSTA: un colpo che fa danno] |
| Costo AP | 2 [CATALOGO; DECISO D-594 (1), (3)] |
| Cooldown | 2 [GDD riga 229]; al più una volta per piano [LETTURA D-613 (5); riserva `MSP-1.r`] |
| Priorità | 65 [PROPOSTA: come `Action.CircularAoE`, `RTCatalogLibrary.cpp:1506`; conta solo per l'ordine del racconto, D-503 (4)]. Se il blocco valga sulle cure dello **stesso** Blast **non** dipende dalla priorità, ma da dove il resolver applica lo stato del colpo rispetto a `ApplyPlannedHeals` (`RTTurnManager.cpp:7438`), non verificato [DOMANDA → TR-22] |
| Portata e linea di vista | 1–2, `Required` (`RTActionDef.h:860`) [PROPOSTA: «in un raggio di 2» letto come portata, perché il GDD usa «raggio» per la portata anche in Shock Whip (riga 228), e il testo dice «il bersaglio» al singolare]; lettura «area di raggio 2» [DOMANDA → TR-22] |
| Forma | `Single` [PROPOSTA]; con l'area di raggio 2 colpirebbe fino a 19 celle a 200, contro i 180 su raggio 1 di `CircularAoE` allo stesso costo: argomento contro l'area |
| Bersaglio | unità nemica, la sua cella [PROPOSTA; DECISO D-583 (4)] |
| Elemento | `Element.Decay` [CATALOGO; DECISO D-593]. Il GDD (riga 229) dà «Disintegr.» e «Danno Perforante»: vale il catalogo [DECISO D-612 (1)] |
| Tipo di danno | elementale Decay, RES Decay [PROPOSTA: «Oscuro» nomina l'elemento; DOMANDA → TR-1] |
| Danno base | 200 [CATALOGO], scala ×10 [DECISO D-589 (2)], al solo bersaglio |
| Movimento | nessuno [CATALOGO] |
| Stati | stato **nuovo** `Status.HealBlocked` [PROPOSTA, referto §3.2], dopo il danno, durata 2 turni [CATALOGO] via `StatusDuration = 2` (`RTActionEvent.h:190`); come si contino i due turni [DOMANDA → TR-22]. Blocca l'effetto `Heal`; **non** blocca `Shield` [PROPOSTA: il catalogo dice «cure», mentre Iron Maiden elenca «cure e scudi» separati], **in divergenza dal referto** §3.2, che lo leggeva anche in Shield [DOMANDA → TR-22] |
| Persistenti · Condizioni | nessuno · nessuna [CATALOGO] |
| Reazione | — . Come colpo `Single` con `bCountsAsAttack` [PROPOSTA] può innescare `HitByDirectAttack`/`AllyHitByDirectAttack`; se un colpo elementale le inneschi dipende dal tipo di danno [DOMANDA → TR-1] |
| Fallback | `Cancel` [PROPOSTA: come `PrecisionAttack`/`HeavyAttack`, `RTCatalogLibrary.cpp:1468`, `:1479`]; con la lettura ad area, `AttackCell` |
| Icona | cornice Blast; glifo Morte: nessun glifo Morte fra quelli osservati in `skill.af` (Appendice B) [DOMANDA → TR-22]; badge croce verde barrata = blocco cure [Appendice B] |

**Nel motore.**
- *Riuso*: effetto `Damage`; effetto `Status` con `StatusTag` e `StatusDuration` (`RTActionEvent.h:173-197`); `Single`; `RangeCells`/`MinRangeCells` (`RTActionDef.h:584`, `:896`).
- *Estensione*: la lettura di `Status.HealBlocked` nei **due** percorsi della cura: gli eventi `Heal` del Prep (`RTTurnManager.cpp:4983`) e `ApplyPlannedHeals` del Blast (`:2478`), con una voce di TurnLog di cura annullata.
- *Creazione*: `Status.HealBlocked` (tag, glossario, icona); campo elemento; una sorgente di danno elementale (oggi `ERTDamageSource` = {`Direct`, `Environmental`}, `RTCombatLibrary.h:177`) che porti l'elemento e scelga la RES nella mitigazione.

**Esempio.** *Given* Riktor in (0,0,0), budget 6; nemico A a distanza 2 in linea di vista, HP 900, RES Decay 0 e DEF 25 (valori d'esempio), turno T. *When* Riktor pianifica Corrosive Shackles su A (2 AP). *Then* A → 700 (lettura elementale; lo scudo base di D-224 non si applica perché D-224 lo limita al `Direct`; se il tipo risulterà `Direct`: floor(200 × 100/125) = 160 con la DEF 25 di A → 740, con lo scudo base già eroso come da Convenzione). A porta `Status.HealBlocked`. Se la cura di un compagno nello stesso Blast sia annullata: TR-22. Con la lettura d'esempio (PROPOSTA) di durata 2 contata dal turno di applicazione, una cura su A nel turno T+1 non ripristina HP e il TurnLog registra la cura annullata; da T+2 lo stato è scaduto. Variante distintiva: un nemico B con gli stessi valori di A, a 1 da A e a 2 da Riktor, resta a 900 nella lettura singola, scende a 700 nella lettura «area raggio 2».

**Domande.** TR-22 (raggio, che cosa blocca, conteggio dei turni e cure dello stesso Blast, glifo), TR-1, TR-2, TR-4 (ripetizione), TR-5 (cooldown), TR-6 (posto nella dock).

### Seismic Smash — `Hero.Riktor.SeismicSmash`

> «Opzione 2 (Impatto Terra) — Seismic Smash [🪨 Terra]: Un colpo stritolante da 220 Danno Fisico in mischia. Se il bersaglio viene sbalzato contro un muro perimetrale solido, subisce il 50% di danni aggiuntivi per schiacciamento.»

| Campo | Valore |
|---|---|
| Ruolo | primaria di Blast [CATALOGO; DECISO D-612 (2)]; posto nella dock [DOMANDA → TR-6]. Il GDD non ha la voce |
| Fase | Blast · `Attack` [PROPOSTA: come `Action.HeavyAttack`] |
| Costo AP | 2 [CATALOGO; DECISO D-594 (1), (3)] |
| Cooldown | 2 [PROPOSTA: stesso costo e gruppo di Shackles; con 0, tre Smash (660, 6 AP) dominerebbero la raffica di Shock Whip; DOMANDA → TR-5] |
| Priorità | 80 [PROPOSTA: come `Action.HeavyAttack`, `RTCatalogLibrary.cpp:1478`; conta solo per il racconto, D-503 (4)] |
| Portata e linea di vista | 1 fissa, `Required` [CATALOGO «in mischia»; PROPOSTA: portata dell'abilità, non dell'arma come `HeavyAttack` (Range 0)] |
| Forma · Bersaglio | `Single` [CATALOGO] · unità nemica adiacente [PROPOSTA] |
| Elemento | `Element.Earth` [CATALOGO; DECISO D-593] |
| Tipo di danno | `Direct`, DEF [PROPOSTA: «Danno Fisico», D-589 (1)]; che cosa fa Earth su un colpo diretto [DOMANDA → TR-1] |
| Danno base | 220 [CATALOGO]; con l'urto +50% [CATALOGO] come termine di `Mod` = 150 [PROPOSTA: `Mod` è in percentuale come `Exposed` → 120, E53 CP 53.3b; che una condizione di posizione entri in `Mod`, che D-589 (3) definisce prodotto dei moltiplicatori di stato, è DOMANDA → TR-23, REQ-17] |
| Movimento | il catalogo dice «Se il bersaglio viene sbalzato», ma l'abilità come scritta non sbalza [DOMANDA → TR-23]. Lettura (a) [PROPOSTA]: Smash porta una spinta (`Push`) di 1 cella nella direzione Riktor → bersaglio. Lettura (b): nessuna spinta propria, conta solo uno sbalzo di un'altra fonte; richiede un cambio di contratto, perché il danno non vede gli spostamenti dello stesso Blast |
| Stati · Persistenti | nessuno [CATALOGO] |
| Condizioni | bonus d'urto se il lato del bersaglio **opposto a Riktor** è **bloccante** (`bBlocksMovement`, copertura `High`, porta chiusa) [PROPOSTA, vocabolario di [`spec-caduta-e-bordi.md`](../../gameplay/spec-caduta-e-bordi.md) §2; `RTHexCellData.h:42-47`], valutato come **predicato geometrico sullo snapshot del colpo**, non come esito di una spinta: si applica nello stesso istante del danno [DECISO D-503 (1)]. Un lato **aperto** non è un muro: con la lettura (a) il bersaglio cade (§3 di quella spec); se la caduta dia il bonus [DOMANDA → TR-23]. Parapetto (`FRTHexEdgeGuard`), copertura bassa, struttura distruttibile, unità dietro: [DOMANDA → TR-23] |
| Reazione | — . Come colpo diretto con `bCountsAsAttack` [PROPOSTA] innesca `HitByDirectAttack`/`AllyHitByDirectAttack`; con la spinta (a) anche `AboutToBeDisplaced`; se `Reaction.Anchor` annulla la spinta, il bonus vale? [DOMANDA → TR-23] |
| Fallback | `Cancel` [PROPOSTA: come `HeavyAttack`, `RTCatalogLibrary.cpp:1479`] |
| Icona | cornice Blast, glifo Terra; badge »» solo con la lettura (a) [PROPOSTA] |

**Nel motore.**
- *Riuso*: `Action.HeavyAttack` come modello (`RTCatalogLibrary.cpp:1478-1482`); un `Push` dentro un colpo di fase `Attack`, come la spinta che `Weapon.Impact` accoda all'attacco base (`RTBotPlanningLibrary.cpp:828`, `RTCatalogLibrary.cpp:783`) e come `PressureJet` (D-602); `MitigateDirectDamage` (CP 53.2); `Mod` di D-589.
- *Estensione*: campo elemento; un termine di danno condizionato a un predicato geometrico (lato bloccante dietro il bersaglio) invece che a uno stato.
- *Creazione*: un evento TurnLog per lo schiacciamento, con un id e un'etichetta **distinti** da `Fall.Impact`/«Urto» (`RTTurnLogLibrary.h:232-239`), per esempio «schiacciamento contro un lato bloccante» [PROPOSTA; una seconda «Urto» è la seconda verità che D-098 vieta]. Ricerca `PushBlocked|DisplacementBlocked|BlockedByWall` in `Source/RefactorTactics` → solo il nome di scenario `Combat.BlockedByWall` (`RTScenarioRunnerTests.cpp:901`).

**Esempio.** *Given* Riktor in (0,0,0), budget 6; nemico A in (1,0,0), adiacente, HP 1000, DEF 25, colpito di fronte, scudo base di D-224 **intatto** (50); il lato fra (1,0,0) e (2,0,0) ha copertura `High`. *When* Riktor pianifica Seismic Smash su A (2 AP). *Then*, lettura (a): floor(220 × 150/100 × 100/125) = 264, meno scudo base 50 = 214 → A 786 HP, e la spinta è fermata dal lato bloccante. Senza il lato bloccante: floor(220 × 100/125) = 176 − 50 = 126 → 874 HP, A spinto in (2,0,0). Lettura (b), senza un'altra spinta: 874 HP anche col lato bloccante.

**Domande.** TR-23 (chi sbalza, ostacoli validi, `Mod` o danno separato, Anchor, caduta, danno al muro), TR-1, TR-5, TR-4 (ripetizione), TR-6 (posto nella dock).

### Arc Discharge — `Hero.Riktor.ArcDischarge`

> «Opzione 3 (Catena Fulmine) — Arc Discharge [⚡ Fulmine]: Spara un dardo da 180 Danno Energetico. Se il bersaglio condivide il macro-esagono con un alleato, la scossa rimbalza infliggendo 100 danni garantiti al compagno (punendo il posizionamento troppo stretto).»

| Campo | Valore |
|---|---|
| Ruolo | primaria di Blast [CATALOGO; DECISO D-612 (2)]; posto nella dock [DOMANDA → TR-6]. Il GDD non ha la voce |
| Fase | Blast · `Attack` [PROPOSTA] |
| Costo AP | 2 [CATALOGO; DECISO D-594 (1), (3)] |
| Cooldown | 2 [PROPOSTA: con 0 tre Arc (540 + rimbalzi) batterebbero la raffica di Shock Whip; 2 e non 1 come Shackles e Smash, per la cadenza di un turno su due (Scelte comuni); DOMANDA → TR-5] |
| Priorità | 60 [PROPOSTA: come `Action.PrecisionAttack`, `RTCatalogLibrary.cpp:1467`; solo racconto, D-503 (4)] |
| Portata e linea di vista | 1–3, `Required` [PROPOSTA: 3 è la portata di `ImpactShot` in `MakeBranth`, che in partita scende a 2 con `Weapon.Impact` (`RTAbilityLab.cpp:166`, `RTAbilityLabTests.cpp:1429`); l'equipaggiamento si ridisegna, D-612 (6); DOMANDA → TR-10]. Il rimbalzo non chiede linea di vista da Riktor [PROPOSTA: parte dal bersaglio; DOMANDA → TR-24] |
| Forma | `Single` sul primario [CATALOGO «un dardo»] + un bersaglio secondario derivato a distanza 1 dal primario [PROPOSTA] |
| Bersaglio | unità nemica; rimbalzo: un'unità della squadra del bersaglio adiacente al bersaglio [PROPOSTA: «condivide il macro-esagono» è un CONTRACT CONFLICT con un occupante per cella, D-583 (4), referto §2; la lettura istanziabile è l'adiacenza; DOMANDA → TR-24] |
| Elemento | `Element.Electric` [CATALOGO; DECISO D-593] |
| Tipo di danno | primario: elementale Electric [PROPOSTA; DOMANDA → TR-1]. Rimbalzo, «danni garantiti»: [PROPOSTA] «avviene sempre quando la condizione è vera» (il gioco non ha mancati), elementale Electric come il primario; lettura alternativa: danno che salta DEF/RES, copertura e Guard [DOMANDA → TR-3] |
| Danno base | 180 al bersaglio; 100 al compagno [CATALOGO]; spalle di Riktor solo sul primario [PROPOSTA: il rimbalzo non ha una direzione d'attacco] |
| Movimento | nessuno [CATALOGO] |
| Stati | nessuno [CATALOGO]; non applica `Status.Electrified`, istantaneo |
| Persistenti | nessuno |
| Condizioni | rimbalzo se, sullo snapshot con cui si calcola il danno del Blast, esiste un'unità della squadra del bersaglio a distanza 1 dal bersaglio [PROPOSTA]; quali spostamenti dello stesso Blast quello snapshot contenga: non verificato. Con più compagni ne colpisce **uno** [CATALOGO «al compagno», singolare], il `StableUnitId` più basso [PROPOSTA: ordine esplicito, CLAUDE.md §7; DOMANDA → TR-24]; mai Riktor né i suoi alleati |
| Reazione | — . Il primario innesca `HitByDirectAttack`/`AllyHitByDirectAttack`; se il rimbalzo conti come colpo diretto [DOMANDA → TR-24] |
| Fallback | `Cancel` [PROPOSTA]; se il primario non è colpito non c'è rimbalzo |
| Icona | cornice Blast, glifo saetta [Appendice B, lettura] |

**Nel motore.**
- *Riuso*: `Damage`, `Single`, `RangeCells`/`MinRangeCells`; l'occupazione canonica per l'adiacenza; `PropagationLimit` (`RTActionDef.h:701`) come precedente di propagazione limitata.
- *Estensione*: campo elemento; in alternativa alla creazione, `PropagationLimit = 1` con un filtro «unità della squadra del bersaglio adiacente» [PROPOSTA].
- *Creazione*: il bersaglio secondario derivato con il proprio danno (ricerca `Bounce|ChainTarget|Rimbalz` → 0 nel codice di gioco), comune al colpo secondario di Shock Whip; evento TurnLog che nomina primario e secondario; proiezione privacy dell'evento se il compagno è nascosto alla squadra di Riktor (CLAUDE.md §7); un danno «garantito» solo se l'autore lo decide.

**Esempio.** *Given* Riktor in (0,0,0), budget 6; nemico A in (3,0,0) in linea di vista; nemico B in (4,0,0), adiacente ad A; nemico C in (6,0,0); HP 900, RES Electric 0. *When* Riktor pianifica Arc Discharge su A (2 AP). *Then* A → 720, B → 800, C 900. Variante: se B si allontana col Move (dopo il Blast), il rimbalzo lo colpisce comunque. Se nello stesso Blast un alleato di Riktor spinge B via da A con un `Push` (Control, dentro il Blast), il rimbalzo non avviene **solo se** gli spostamenti di Control sono già applicati quando si calcola il danno di Attack [non verificato; gli spostamenti dei colpi si applicano dopo il danno, `RTTurnManager.cpp:7446`]; altrimenti B è colpito.

**Domande.** TR-24 (adiacenza, più compagni, reazioni e privacy del rimbalzo, linea di vista), TR-3, TR-1, TR-10, TR-5, TR-2 (scudo base), TR-4 (ripetizione), TR-6 (posto nella dock).

---

## 5. Ultimate

Gruppo del catalogo: «5. Blast Phase - Ultimate (Costante: 3 AP)».

**Scelte comuni.** 3 AP [DECISO D-613 (4)], cooldown 3 [GDD righe 230-231], al più una volta per piano [LETTURA D-613 (5); riserva `MSP-1.r`]. Con cooldown 3 l'abilità usata nel turno N torna disponibile in N+3: impostata all'uso e scalata nel Cleanup dello stesso turno [codice: `RTUnit.cpp:2324-2330`, `RTTurnManager.cpp:2108`; letto, non eseguito]. Fasi diverse: Iron Maiden nel Prep [DECISO D-613 (4)], Skewer nel Blast [CATALOGO; GDD]; il titolo del gruppo vale solo per Skewer, e la cornice le distingue. Insieme costano 6, l'intero budget di default, e fanno la combinazione più forte del kit (gabbia nel Prep, poi Skewer nel Blast): va misurata con `tools/balance/ap_forms_sim.py`, non decisa qui. «Armatura» = DEF in entrambe [GDD: «ignora l'armatura fisica», «azzerando la DEF»].

### Skewer — `Hero.Riktor.Skewer`

> «Opzione 1 (Originale) — Skewer [⚡ Fulmine]: Eroga 380 Danno Energetico a cono frontale che ignora l'armatura e applica Stun per 1 turno, acquisendo priorità deterministica sui bersagli già elettrificati.»

| Campo | Valore |
|---|---|
| Ruolo | Ultimate, «Originale» [CATALOGO; DECISO D-612 (2)]; posto nella dock [DOMANDA → TR-6] |
| Fase | Blast · `Attack` (40) [CATALOGO; GDD riga 230; PROPOSTA: un colpo di danno con stato accessorio]. L'alternativa Control (30) **non** darebbe allo Stun nessun effetto in più sugli attacchi dello stesso Blast (vedi Priorità) |
| Costo AP | 3 [CATALOGO; DECISO D-613 (4)] |
| Cooldown | 3 [GDD riga 230]; al più una volta per piano [LETTURA D-613 (5)]; usata da stordito si paga lo stesso [DECISO D-563] |
| Priorità | 45 [PROPOSTA], **solo** come ordinamento degli eventi di danno dentro `Attack`. ⚠️ Non nega gli attacchi dello stesso Blast: gli intenti d'attacco del Blast sono raccolti e controllati contro lo stordimento **prima** che qualunque stato del Blast venga applicato (`CollectAttackIntents`, `RTTurnManager.cpp:6219`, con `RefuseMainActionIfStunned`, `RTTurnManager_Blast.cpp:1144`; `ApplyControlStatuses`, `RTTurnManager.cpp:7510`, `RTTurnManager_Blast.cpp:4035`) [codice, letto non eseguito]. «Priorità deterministica» non ha quindi effetto sullo Stun; uno Stun che preceda gli attacchi dello stesso Blast richiede una regola nuova del resolver [DOMANDA → TR-25] |
| Portata e linea di vista | 1–2, `MinRangeCells` 1 [PROPOSTA: la cella mirata orienta il cono e dev'essere diversa da quella di Riktor, perché `HexCone` restituisce un cono vuoto se il bersaglio coincide con l'origine, `RTHexLibrary.cpp:777-782`]; profondità del cono = `RangeCells` 2 [PROPOSTA: il catalogo e il GDD non la danno; DOMANDA → TR-10]; linea di vista per cella colpita come le altre aree [PROPOSTA; filtro di `HexCone` su muri e coperture non verificato] |
| Forma | `Cone` [CATALOGO «cono frontale»]: 120°, `From` escluso, profondità `RangeCells` (`RTHexCombatLibrary.cpp:42-45`); a profondità 2 = **8 celle** (3 a distanza 1, 5 a distanza 2), orientato sulla direzione del primo passo di `HexLine` verso la cella mirata [codice: `RTHexLibrary.cpp:808-828`]. «Frontale» = verso la cella mirata [PROPOSTA] |
| Bersaglio | una cella che orienta il cono; colpisce le unità nemiche nelle celle [PROPOSTA]; alleati esclusi [PROPOSTA; DOMANDA → TR-12] |
| Elemento | `Element.Electric` [CATALOGO; DECISO D-593]. Il GDD (riga 230) dà «Disintegr.»: vale il catalogo [DECISO D-612 (1)] |
| Tipo di danno | elementale Electric, RES Electric, non DEF [PROPOSTA: leggerlo elementale rende «ignora l'armatura» vero per costruzione, perché il GDD chiama «armatura» la DEF]; alternativa: `Direct` con DEF ignorata, un terzo termine non deciso; «ignora» salta anche copertura e Guard? [DOMANDA → TR-3] |
| Danno base | 380 [CATALOGO; GDD] **per cella colpita** [PROPOSTA: il catalogo dice «a cono frontale» senza il «distribuito» di Static Sweep, e il GDD «a cono frontale da 380 Danni»; nessuno dei due dice «per cella»]. Lettura alternativa: 47,5 per cella (380 diviso otto), meno della metà di Taser Strike (100), che costa 2 AP [motivo del rigetto]. [DOMANDA → TR-25]. Sopra la fascia dei colpi 240–350 di D-589: coerente con un'Ultimate, da misurare |
| Movimento | nessuno [CATALOGO] |
| Stati | `Status.Stunned` (esiste, `RTGameplayTags.cpp:16`): nega azione principale, reazione e ciò che è armato [DECISO D-416 (1)] e il movimento alla risoluzione [DECISO D-569]; nato nel Blast, nega il Move dello stesso turno (test `Status.StunnedMovement.AStunBornInTheBlastKeepsBothRefusals`). Durata 2 [PROPOSTA: `TickStatuses` decrementa nel Cleanup (`RTTurnManager.cpp:2098`), quindi una durata 1 nata nel Blast copre solo il resto di N; 2 copre anche N+1, come `Prone`, D-319]; [DOMANDA → TR-25, REQ-13]. Skewer sarebbe il **primo** produttore di Stun (`RTUnbalancedProneTests.cpp:1296`) |
| Persistenti | nessuno |
| Condizioni | «bersagli già elettrificati» non ha soggetto: `Status.Electrified` è istantaneo (REQ-10). [PROPOSTA] nessuna condizione; alternativa: una condizione su `Status.Wet` con un bonus in `Mod`, il cui valore non esiste [DOMANDA → TR-25] |
| Reazione | — |
| Fallback | `Cancel` se Riktor non può agire (stordito: si paga, D-563) [PROPOSTA]; il cono resta orientato sulla cella pianificata anche se vuota, e un'area che non trova nessuno si paga [DECISO D-563] |
| Icona | cornice Blast, glifo saetta [PROPOSTA, Appendice B]; badge possibili: stella (stordimento), rombo barrato (ignora armatura) |

**Nel motore.**
- *Riuso*: `ERTAbilityShape::Cone` e `URTHexCombatLibrary::HexHitCells`; `Damage`; `FRTActionEffectSpec(Status, TAG_Status_Stunned, turni)` sul modello di `Action.Root` (`RTCatalogLibrary.cpp:1759-1761`); negazioni dello Stun già cablate (`RTTurnManager_Blast.cpp:1130`, `RTTurnManager.cpp:3639`, `:4750`); `TAG_Status_Stunned` in `ControlStatusesBySeverity` (`RTReactionLibrary.cpp:181`).
- *Estensione*: `MakeHeroAction` per `Hero.Riktor.Skewer` con costo 3 e cooldown 3; icona `UI.Icon.Status.Stunned` in `DA_IconCatalog` se manca (`RTGameplayTags.h:151`; non verificato).
- *Creazione*: campo elemento; un percorso di mitigazione elementale che legga `ARTUnit::ResistanceAgainst(Element)` (`RTUnit.h:136-142`) al posto della DEF: oggi la formula è `MitigateDirectDamage(Base, RearBonus, ModPercent, Defense)` (`RTCombatLibrary.h:888`), e nessun codice di gioco legge la RES (`grep ResistanceAgainst(` nei `.cpp` fuori dai test → 0 righe). Solo con la lettura «diviso» (TR-25): una regola di ripartizione del danno fra le celle, che oggi non esiste. Solo se l'autore lo vuole (TR-25): uno Stun che preceda la raccolta degli intenti del Blast. Nessuna priorità per bersaglio.

**Esempio.** *Given* Riktor in (0,0,0), budget 6, piano Skewer (3) mirato su (1,0,0) + Move (2) + Electrocute (1) = 6 AP; nemico A in (1,0,0), HP 1000, RES Electric 35, nessun `Exposed`, che ha pianificato un BasicAttack e un Move; nemico B in una cella a distanza 2 dentro il cono, HP 1000, RES Electric 35; alleato C fuori dal cono; nessun'altra azione colpisce A o B nel Blast. *When* il Blast risolve Skewer. *Then*, nella lettura per cella colpita (PROPOSTA, TR-25), A e B, colpiti non alle spalle, subiscono floor(380 × 100/135) = 281 [DECISO D-595 (1)] → 719 HP a fine Blast (il tick di Electrocute su A arriva dopo, nel Cleanup); entrambi `Status.Stunned`. Il BasicAttack di A nel turno N **risolve** (intento raccolto prima dello Stun). Il Move di A in N è negato [DECISO D-569]. Con durata 2, A perde anche azione principale e reazione in N+1; C intatto; la DEF di A e B non entra. Lettura alternativa (durata 1): in N+1 A agisce normalmente.

**Domande.** TR-25 (danno per cella, durata dello Stun, «priorità deterministica», Stun prima degli attacchi), TR-3, TR-10, TR-12, TR-1 (tipo di danno), TR-2 (scudo base), TR-4 (ripetizione), TR-5 (cooldown), TR-6 (posto nella dock), TR-13 (reazioni su un colpo d'area).

### Iron Maiden — `Hero.Riktor.IronMaiden`

> «Opzione 2 (Controllo Morte) — Iron Maiden [🪨 Terra / 💀 Morte]: Ingabbia un singolo bersaglio entro 3 esagoni bloccandone movimento, Linea di Vista, cure e scudi. Se l'esagono di atterraggio è Fango, la gabbia sprofonda azzerando l'armatura del bersaglio.»

| Campo | Valore |
|---|---|
| Ruolo | Ultimate [CATALOGO; DECISO D-612 (2)]; posto nella dock [DOMANDA → TR-6] |
| Fase | Prep (`Preparation`, 10) [DECISO D-613 (4); GDD riga 231]; il titolo del gruppo del catalogo («Blast Phase») è superato su questo punto |
| Costo AP | 3 [DECISO D-613 (4)]; emenda D-594 (3) per le Ultimate fuori dal Blast [LETTURA D-613 (5)] |
| Cooldown | 3 [GDD riga 231]; al più una volta per piano [LETTURA D-613 (5)] |
| Priorità | 25 nel Prep [PROPOSTA: dopo Magnetic Spikes (20), che cambia il terreno prima di ogni lettura del Prep, perché la gabbia legge la cella del bersaglio (ramo Fango); a 20 le due pareggerebbero nel piano dello stesso Riktor (1 + 3 = 4 AP, legale), e le scioglierebbe solo l'ordine di dichiarazione [DECISO D-613 (3)]. Le azioni di Prep lette vanno da 30 (SuppressiveLine, Brace, InterceptShot) a 76 (Reconfigure): a 25 la gabbia le precede tutte, quindi arriva prima che il bersaglio si dia uno scudo o una guardia, che è ciò che «bloccandone… scudi» chiede] |
| Portata e linea di vista | 1–3 [CATALOGO «entro 3 esagoni»; GDD], `MinRangeCells` 1 [PROPOSTA]; linea di vista richiesta [PROPOSTA] |
| Forma · Bersaglio | `Single` [CATALOGO] · un'unità nemica [GDD; CATALOGO], nella sua cella d'inizio turno, perché il Prep risolve prima di Dash e Move |
| Elemento | `Element.Earth` [PROPOSTA: il catalogo dichiara due elementi, ma un'azione porta un solo `Element.<X>` (D-593); il GDD dice Terra, e l'icona di `skill.af` letta è Prep + Terra (HUD-14)]; `Element.Decay` l'alternativa [DOMANDA → TR-26] |
| Tipo di danno · Danno base | N/A · 0 [CATALOGO] |
| Movimento | nessuno [CATALOGO]; «l'esagono di atterraggio» non ha un atterraggio (REQ-16): «la cella del bersaglio quando la gabbia risolve» [PROPOSTA] |
| Stati | composizione [PROPOSTA, motivo il principio di D-416 (1) ⛔ «due stati che negano la stessa cosa sono un stato solo scritto due volte», che D-569 ha già derogato per Stun e Root sul movimento: è un principio, non un vincolo che decide]: `Status.Root` (esiste) per il movimento; `Status.HealBlocked` (nuovo, condiviso con Shackles) letto da `Heal` e da `Shield`; uno stato nuovo per la linea di vista (nome di lavoro `Status.Caged`, con l'icona della gabbia). Alternativa: un solo `Status.Caged` che riusa i percorsi di Root [DOMANDA → TR-26]. Inizio nel Prep: nega Dash e Move di N (un Root nato più tardi nega già il Move: test `Status.StunnedMovement.TheMoveIsDeniedByARootBornInTheBlastToo`). Durata 1 [PROPOSTA: copre Dash, Blast e Move di N e scade nel Cleanup di N; 2 darebbe due turni di controllo per 3 AP; DOMANDA → TR-26]. «Azzerando l'armatura»: DEF a 0 per la durata, modificatore nuovo (nessuno stato tocca la DEF; `Exposed` agisce su `Mod`) |
| Persistenti | nessuno [PROPOSTA: la gabbia è uno stato sull'unità]; se deve bloccare la linea di vista di terzi attraverso la cella diventa un oggetto di cella, che non esiste [DOMANDA → TR-26] |
| Condizioni | «Se l'esagono di atterraggio è Fango»: due letture, la coppia Water+Earth (OPEN, candidato «Fango») o il «Fango Magnetico» Electric+Earth delle Combo Alchemiche dello stesso catalogo (riga OPEN, candidato GDD «Faglia ionizzata»). In entrambi i casi il ramo resta **inerte** finché la coppia è OPEN [PROPOSTA; DOMANDA → TR-26] |
| Reazione | — |
| Fallback | `Cancel` se il bersaglio non è più nella cella mirata o è KO [PROPOSTA: come `Action.Root`, `RTCatalogLibrary.cpp:1760`]; la ricarica si paga [DECISO D-563] |
| Icona | cornice Prep (cerchio blu a quattro tacche), glifo Terra [PROPOSTA, Appendice B: un'icona Prep + Terra III]; badge croce verde barrata e occhio barrato |

**Nel motore.**
- *Riuso*: `ERTResolutionPhase::Preparation`; la **forma** dell'effetto `FRTActionEffectSpec(Status, TAG_Status_Root, turni)` come `Action.Root` (`RTCatalogLibrary.cpp:1759-1762`), con `bCountsAsAttack`; `MinRangeCells`; `Status.Obscured` («targeting limitato a 2 celle», `RTGameplayTags.h:28`) come precedente di uno stato che restringe il targeting. `TAG_Status_Root` è in `ControlStatusesBySeverity` (`RTReactionLibrary.cpp:181`), letta dalla passata `BlastStatus`.
- *Estensione*: ⚠️ `ResolvePrep` oggi applica le azioni non predittive **a chi le usa** (`RTTurnManager.cpp` ~4944: «le azioni di Prep del vertical slice agiscono su chi le usa»; `:1693`): la gabbia finirebbe su Riktor. Serve un percorso di Prep con bersaglio ostile: `TargetUnitId` = bersaglio pianificato, portata e linea di vista ricontrollate in risoluzione, `Cancel`, colpo `bCountsAsAttack`. Poi: `MakeHeroAction` (Preparation, 25, portata 1–3, costo 3, cooldown 3); più effetti di stato nella stessa azione; lettura di `HealBlocked` in `Heal` e `Shield`.
- *Creazione*: `Status.HealBlocked`; lo stato della vista con la sua regola nel servizio LOS/targeting, separato (CLAUDE.md §11; sede non letta); il modificatore «DEF a 0» sulla formula di E53, solo se il ramo Fango si attiva; un **punto di passata delle reazioni nel Prep** (per esempio `PrepStatus`, in coda a `ERTReactionPassPoint`, `RTReactionLibrary.h:42`), se la gabbia deve far scattare `AboutToReceiveControl`: oggi quella passata esiste solo nel Blast (`RTTurnManager_Blast.cpp:4053`), e nessuna reazione anti-controllo vede uno stato del Prep. Ogni valore nuovo di enum in coda (TurnLog serializzato).

**Esempio.** *Given* Riktor in (0,0,0), budget 6, piano Iron Maiden (3) su B + Move (2) + Electrocute (1) = 6 AP; nemico B in (3,0,0), HP 1000, linea di vista libera, piano validato Shield su sé (Prep 35, 1 AP) + BasicAttack su Riktor (Blast 50, 2 AP) + Move (2 AP) = 5 AP. *When* il Prep risolve Iron Maiden a priorità 25. *Then* B riceve Root + HealBlocked + stato di vista; lo Shield di B a 35 non dà nulla (se si adotta HealBlocked su Shield); il Move di B è negato; il BasicAttack di B segue TR-26 (annullato se la gabbia toglie la vista in uscita, eseguito se la toglie solo in entrata); B resta a 1000; gli AP di B restano spesi [DECISO D-613 (1)]; la gabbia scade nel Cleanup di N. Variante Fango: nessun effetto oggi.

**Domande.** TR-26 (elemento, durata, linea di vista, scudi, azioni senza vista, composizione, Fango e DEF a 0, Cleanse, reazioni anti-controllo nel Prep), TR-22 (che cosa blocca `HealBlocked`, glifo Morte), TR-4 (ripetizione), TR-5 (cooldown), TR-6 (posto nella dock).

---

## 6. Reazioni

Gruppo del catalogo: «6. Reaction Phase (Costanti: 1–2 AP)».

**Scelte comuni.** Le due reazioni costano 1 [CATALOGO], contro la classe reazione 2 [DECISO D-594 (1)]; ogni riga dichiara il proprio costo [DECISO D-594 (3)], e lo scostamento si motiva [PROPOSTA: i motivi sono nelle righe Costo AP di Magnetic Bastion e Shock Armor]. Il «1–2 AP» del titolo del gruppo coincide con il «1–2 AP» che il GDD dà a Magnetic Bastion (righe 233-234): una possibile origine, non verificata [DOMANDA → TR-27]. Armarle entrambe costa 2 AP, quanto un Move, ed è legale perché le reazioni sono una lista [DECISO D-592]. Entrambe partono dai colpi diretti `Single` (`RTActionDef.h:282-295`). «Reaction Phase» e «Blast Phase nemica» diventano segmenti di reazione nel Blast del turno [DECISO D-583 (1)]. Fallback `Cancel` inerte (`RTCatalogLibrary.cpp:1580-1581`).

**Ordine.** Lo dà il **punto di valutazione** (`ERTReactionPassPoint`), non la priorità: prima l'interposizione (`BlastIntercept`, che riscrive `Plan.Hits` con `RedirectHitTo`), poi il pass `BlastHits` (Counter e la scarica), per ultimo `BlastDisplacement` (Anchor e l'anti-spinta) [codice: `PassPointFor` in `RTReactionLibrary.cpp`; `RTTurnManager_Blast.cpp` ~2245-2300, `:2405`, `:3236`; `RTCatalogLibrary.cpp:1641-1645`]. La priorità decide solo fra reazioni dello stesso punto; il 5 di Anchor «non precede nessuno».

**Una attivazione per turno.** ⚠️ Oggi un'unità attiva al più **una** reazione per turno (`ReactionActivationsThisTurn`, condiviso fra i pass e controllato anche dall'interposizione, RTTurnManager_Blast.cpp ~2265-2285) [DECISO D-092]. Con quel contatore Bastion e Shock Armor non scattano entrambe nello stesso turno, e Shock Armor in due voci non può annullare la spinta dopo aver scaricato. D-592 fa cadere «una reazione per turno» per l'armamento, ma non nomina il contatore delle attivazioni [DOMANDA → TR-29].

**Cooldown.** Oggi il cooldown di una reazione parte quando **scatta** (`ConsumeAbility` nel ramo Activated, RTTurnManager_Blast.cpp ~2283; `RTUnit.cpp:2324-2326`): se non scatta non parte [codice]. Con gli AP pagati all'armamento [PROPOSTA: lettura di D-592 «un'unità arma quante reazioni il budget consente»] una reazione inutilizzata costa AP ma non cooldown [DOMANDA → TR-27, TR-28: tenerlo allo scatto, come oggi?].

### Magnetic Bastion — `Hero.Riktor.MagneticBastion`

> «Reaction Guard (Guardia / Protezione): Magnetic Bastion [🪨 Terra]: Guardia da 1 AP in cui Riktor intercetta un attacco destinato a un compagno subendolo al suo posto, guadagnando però +1 AP extra per il proprio turno successivo. Con le catene si lancia, spostandosi nell'esagono pianificato, bloccando e ricevendo il colpo.»

| Campo | Valore |
|---|---|
| Ruolo | reazione [CATALOGO]. Candidata a uno dei due posti reazione della dock [DECISO D-612 (4) i posti; PROPOSTA che li occupino le due reazioni del catalogo; `KIT-2` aperto]. Erede di `Hero.Branth.Interposition`, costruita su `Action.Intercept` [LETTURA D-612 (7), non decisione]. Nome visibile: non «Guardia», che collide con `Action.Guard`, azione di Prep (`RTCatalogLibrary.cpp:1289`) il cui nome visibile è già «Guardia» (`:2094`) [PROPOSTA: per esempio «Bastione»; DOMANDA → TR-27] |
| Fase | segmento di reazione nel Blast, risoluzione `Control`, punto `BlastIntercept` [DECISO D-583 (1), ADR-0004; riuso `Action.Intercept`, `RTCatalogLibrary.cpp:1625`] |
| Costo AP | 1 [CATALOGO «Guardia da 1 AP»]; il GDD dà «1–2» [DOMANDA → TR-27]. Scostamento dalla classe 2 [DECISO D-594 (1)], dichiarato sulla riga [DECISO D-594 (3)] e motivato [PROPOSTA: il prezzo vero è il danno che Riktor incassa, e il +1 del turno dopo lo rimborsa solo se scatta; da misurare con `ap_forms_sim.py`] |
| Cooldown | 2 [GDD riga 234]; oggi `Action.Intercept` 2 (`RTCatalogLibrary.cpp:1626`), `Interposition` 3 (`RTHeroCatalogLibrary.cpp:910`). Al più una volta nel piano [LETTURA D-613 (5) della risposta libera (2); riserva `MSP-1.r`]. Parte allo scatto [codice] |
| Priorità | 10 [PROPOSTA: riuso di `Action.Intercept`]; decide solo fra reazioni del punto `BlastIntercept`; a parità fra reazioni della stessa unità, ordine di dichiarazione [DECISO D-613 (3) per due azioni della stessa unità nella stessa fase; per due reazioni della stessa unità armate sullo stesso trigger, LETTURA D-592: «lo stesso ordine totale e deterministico dell'ordine dentro la fase»] |
| Portata | 0–2 celle dall'alleato protetto [PROPOSTA: portata di `Action.Intercept`, «un alleato entro 2 celle», `RTCatalogLibrary.cpp:1619`]; il GDD dice «alleato adiacente» [DOMANDA → TR-27]. La linea di vista dall'attaccante a Riktor è calcolata sulla sola mappa: `HasLineOfSight` non riceve le unità, quindi i corpi non la interrompono [codice] |
| Forma | `Single`: si intercettano solo colpi diretti `Single` (`RTActionDef.h:283`) [riuso]; aree, coni e linee [DOMANDA → TR-27] |
| Bersaglio | un alleato diverso da Riktor, bersaglio di un colpo `Single`; Riktor diventa il nuovo bersaglio [riuso, `RTCatalogLibrary.cpp:1615-1617`] |
| Elemento | `Element.Earth` [CATALOGO; DECISO D-593]. Il GDD (riga 233) dà «Terra / Scudo»: vale il catalogo [DECISO D-612 (1)]. Una reazione senza danno né superficie non innesca coppie: l'elemento serve al glifo [PROPOSTA] |
| Tipo di danno | N/A. Il colpo intercettato si ricalcola su Riktor: sua DEF (45), suo arco frontale per le spalle, copertura della sua cella [riuso: `URTHexCombatLibrary::RedirectHitTo`, D-589 CP 53.2c]. Difetto latente nominato nel codice: il campo `CoverReduction` della traccia resta quello dell'alleato. Domanda: confermi il comportamento di oggi? [DOMANDA → TR-27] |
| Danno base | N/A. Il GDD pone un tetto «fino a 300 danni» (riga 234) che il catalogo non riporta [DOMANDA → TR-27] |
| Movimento | (a) [PROPOSTA, default]: nessuno spostamento, il colpo cambia bersaglio a distanza, come l'Intercept di oggi. (b) [CATALOGO «si lancia, spostandosi nell'esagono pianificato»]: spostamento di famiglia `SelfReposition` (`RTCatalogLibrary.cpp:608-615`) verso una cella derivata, per esempio la cella libera adiacente all'alleato sulla linea dell'attaccante, come `Reaction.HazardEscape` deriva la sua (`:639-641`); lo fermano cella occupata [DECISO D-583 (4)], muro, cambio di Layer. L'«esagono pianificato» dal giocatore per una reazione non esiste: scriverlo nel piano è un campo nuovo, privato (AGENTS.md §4). Con (b): il colpo si valuta e ricalcola dalla cella di partenza o d'arrivo? La (b) chiede una `D-nnn` [DOMANDA → TR-27] |
| Stati | nessuno sul bersaglio. Su Riktor: +1 al budget AP del turno successivo, solo se la reazione è scattata [DECISO D-613 (1): «7 dopo un Bastion scattato»]; modificatore del budget, non un `Status.*` [PROPOSTA]. Tetto del budget non deciso [LETTURA D-613 (5)] |
| Persistenti | nessuno |
| Condizioni | un alleato (diverso da Riktor) entro portata è bersaglio di un colpo `Single` presente in `Plan.Hits`; l'attaccante sta sul Layer di Riktor (D-393, D-509); linea di vista dall'attaccante a Riktor sulla mappa; il colpo non è già stato intercettato (`ExcludedHits`) [riuso: `FindInterceptableHit`]. Solo attaccanti nemici [PROPOSTA: oggi non filtrato; va in estensione]. Il +1 richiede che la redirezione sia avvenuta [DECISO D-613 (1)] |
| Reazione | trigger `AllyHitByDirectAttack` [riuso]. Condizione dichiarata facoltativa: la soglia di D-109, una per reazione [DECISO D-109, D-592]; soggetto l'alleato colpito [PROPOSTA: l'interposizione non ha un bersaglio di risposta, e il codice applica il predicato alle risposte FIRE, `RTReactionOpportunityTypes.cpp:68-99`] |
| Fallback | `Cancel` inerte [riuso]; se non scatta, AP spesi e niente +1 [PROPOSTA] |
| Icona | cornice della reazione, glifo Terra [PROPOSTA, `progettazione-hud.md` §7.1]; quale cornice di `skill.af` corrisponda alle reazioni: non verificato |

**Nel motore.**
- *Riuso*: `Action.Intercept` (Control 10, portata 2, `AllyHitByDirectAttack`, `RTCatalogLibrary.cpp:1625-1629`); `MakeHeroReactionFromCoreAction` come `Interposition` (`RTHeroCatalogLibrary.cpp:909-910`); `FindInterceptableHit`; `RedirectHitTo`; `Reaction.AllyIntercept` come riferimento di modulo (`RTCatalogLibrary.cpp:600-603`); per (b) `SelfReposition`.
- *Estensione*: campo elemento; `ActionPointCost` 1 e cooldown 2 sulla riga (`ActionPointCost` esiste, `RTActionDef.h:623`); filtro sulla squadra dell'attaccante in `FindInterceptableHit`; la lista delle reazioni armate al posto di `PlannedReactionAbility` [DECISO D-592; stato in E38 non verificato].
- *Creazione*: il budget AP per unità e per turno (comune con l'Anchor) [DECISO D-613 (1)]; un effetto in coda a `ERTActionEffect` che concede AP nel turno dopo (per esempio `GrantActionPointsNextTurn` con Amount 1, o un solo effetto con importo con segno condiviso con l'Anchor) e il suo evento nel TurnLog [PROPOSTA]; solo con (b), la regola che deriva la cella d'arrivo; solo col tetto di 300, un campo di tetto sul danno intercettato.

**Esempio.** *Given* Riktor in (2,1,0), HP 1200, DEF 45, budget 6, scudo base già eroso; alleata A in (2,0,0), adiacente a Riktor, HP 900; nemico N a 2 celle da A, sul Layer 0, nell'arco frontale di Riktor, con linea di vista libera verso Riktor e nessuna copertura fra i due, pianifica un attacco base `Single` da 160 `Direct` su A. Riktor pianifica Move 2 + Shock Whip 2 + Magnetic Bastion 1 = 5 AP (lettura (a)). *When* nel Blast N colpisce A. *Then* Bastion scatta, Riktor diventa bersaglio e subisce floor(160 × 100/145) = 110 → HP 1090; A resta a 900; il TurnLog registra la redirezione; nel planning del turno dopo il budget di Riktor è 7. Controllo: se N attacca con un'area, o non attacca, Bastion non scatta, l'AP resta speso, il cooldown non parte e il budget del turno dopo è 6.

**Domande.** TR-27 (costo, portata, tetto 300, spostamento, cumulo del +1, aree, copertura, cooldown allo scatto, nome), TR-29 (più attivazioni), TR-33 (tetto del budget), TR-4 (ripetizione), TR-5 (cooldown), TR-6 (posto nella dock).

### Shock Armor — `Hero.Riktor.ShockArmor`

> «Reaction Counter (Contrattacco / Punizione): Shock Armor [⚡ Fulmine]: Reazione da 1 AP. Se Riktor viene colpito in mischia durante la Blast Phase nemica, l'armatura rilascia una scarica che applica Fulmine sull'esagono dell'attaccante e annulla qualsiasi Knockback subito.»

| Campo | Valore |
|---|---|
| Ruolo | reazione [CATALOGO]. Candidata al secondo posto reazione della dock [DECISO D-612 (4) i posti; PROPOSTA; `KIT-2`] |
| Fase | segmento di reazione nel Blast, risoluzione `Control` [DECISO D-583 (1), ADR-0004; riuso `Action.Counter`, `RTCatalogLibrary.cpp:1591`]. «Blast Phase nemica» → il Blast del turno [DECISO D-583 (1); CONTRACT CONFLICT corretto, referto §2], uno per tutte le unità e col danno simultaneo [ADR-0003, emendato da D-503] |
| Costo AP | 1 [CATALOGO; GDD riga 232]. Scostamento dalla classe 2 [DECISO D-594 (1)], dichiarato sulla riga [DECISO D-594 (3)], motivato [PROPOSTA: scatta solo su un colpo in mischia, trigger più stretto di `HitByDirectAttack`; da misurare] |
| Cooldown | 1 [GDD riga 232]; al più una volta nel piano [LETTURA D-613 (5)]; parte allo scatto [codice] |
| Priorità | 20 per la scarica, come `Action.Counter`, nel punto `BlastHits` [PROPOSTA: riuso]. L'anti-spinta è valutata nel punto `BlastDisplacement`, **dopo** la scarica; il 5 di Anchor non la anticipa. Una voce ha oggi una sola priorità e un solo trigger: vedi Creazione |
| Portata | 0: la scarica raggiunge chi ha colpito, come `Action.Counter` (`RTCatalogLibrary.cpp:1583-1586`) [riuso] |
| Forma | `Single` sulla cella dell'attaccante [PROPOSTA]. **Nessuna propagazione oggi**: la regola Water+Electric esiste solo per `Action.Electrify` nel Cleanup (unico chiamante non di test di `CollectElectricPropagation`, `RTTurnManager.cpp:3783`; [catalogo delle coppie](../../balance/RT_ElementPairCatalog_v0.1.md) riga 63). Una propagazione dalla scarica chiede un `PropagationLimit` sulla reazione e una propagazione nel Blast, con un tempo diverso dalla riga CURRENT [DOMANDA → TR-28] |
| Bersaglio | l'unità nemica che ha colpito Riktor in mischia [CATALOGO; PROPOSTA: l'unità, la cella conta solo per la propagazione]; per l'anti-spinta, Riktor |
| Elemento | `Element.Electric` [CATALOGO; DECISO D-593] |
| Tipo di danno | elementale Electric, RES Electric dell'attaccante [PROPOSTA]; se «applica Fulmine» sia danno, propagazione o stato di cella [DOMANDA → TR-28] |
| Danno base | non dato [CATALOGO]; 160 [PROPOSTA: danno di `Action.Counter`, già sulla scala ×10, `RTCatalogLibrary.cpp:1593`; DOMANDA → TR-28] |
| Movimento | nessuno spostamento proprio; annulla lo spostamento forzato che Riktor sta per subire con `CancelDisplacement`, come `Reaction.Anchor` (`RTCatalogLibrary.cpp:623-626`, `:1648-1653`) [riuso]; solo lo spostamento prodotto dallo stesso colpo in mischia [PROPOSTA: il «Se» governa entrambe le conseguenze, come nel GDD riga 232]; ogni spinta del turno, e le trazioni: [DOMANDA → TR-28] |
| Stati | **nessuno stato sull'attaccante**. L'evento elettrico si vede nel TurnLog come `Electrified` osservabile, alla maniera di D-315; non è un tag che l'unità porta: `ApplyStatus` scarta in silenzio `Turns <= 0` (`RTElectricPropagationTests.cpp:169-175`, `:284`) [riuso della semantica, non di un effetto `Status`] |
| Persistenti | nessuno [PROPOSTA]; una cella elettrificata che dura chiederebbe la stessa aura di Electrocute [DOMANDA → TR-28] |
| Condizioni | Riktor è bersaglio di un colpo diretto andato a segno nel Blast, sparato da un'azione in mischia [PROPOSTA: «in mischia» = portata massima dichiarata dell'azione pari a 1, un dato del catalogo leggibile dall'anteprima; DOMANDA → TR-28] |
| Reazione | trigger **nuovo** in coda a `ERTReactionTrigger` (`RTActionDef.h:278`), nome proposto `HitByMeleeAttack` [PROPOSTA; referto §3.2]: «in mischia» non può stare nella condizione dichiarata, che ammette solo la soglia di salute [DECISO D-109]. Condizione dichiarata facoltativa: la soglia di D-109, una per reazione [DECISO D-109, D-592]; il soggetto «bersaglio», per la scarica, si legge come l'attaccante (la lettura del codice, `RTReactionOpportunityTypes.cpp:68-99`) [PROPOSTA]; Riktor stesso [DOMANDA → TR-28]. Oggi il predicato vive solo nelle opportunity a più risposte, non nel ramo delle reazioni automatiche |
| Fallback | `Cancel` inerte [riuso]; attaccante KO o lontano: la scarica non parte, l'annullamento resta [PROPOSTA] |
| Icona | cornice della reazione, glifo Electric [PROPOSTA] |

**Nel motore.**
- *Riuso*: `Action.Counter` (Control 20, `HitByDirectAttack`, `Damage`, `InterruptBeforeEffect`, slot Reaction, `RTCatalogLibrary.cpp:1591-1596`); `Action.Anchor`/`Reaction.Anchor` con `CancelDisplacement` e `AboutToBeDisplaced`; la voce osservabile `Electrified`.
- *Estensione*: campo elemento; costo 1 e cooldown 1 sulla riga; `Damage` marcato elementale quando esisterà il percorso.
- *Creazione*: (1) `HitByMeleeAttack` in coda a `ERTReactionTrigger`, **con il caso** in `URTReactionLibrary::PassPointFor` (`BlastHits`), uno switch senza default scritto apposta perché un trigger nuovo dimenticato non compili, e in `FindTriggeringAttacker`, che oggi riconosce solo `HitByDirectAttack`; filtro dei colpi `Single` per portata dichiarata = 1. Senza i due casi il trigger compila e non scatta mai. (2) Una reazione con due trigger e due momenti (scarica dopo il colpo come Counter; annullamento prima dello spostamento come Anchor): oggi una reazione ha un solo trigger; le strade sono una lista di coppie (trigger, effetto) su `FRTActionDef`, un trigger composto, o due voci (`ShockArmor` e `ShockArmor.Brace`) [DOMANDA → TR-28], tutte vincolate dal contatore di D-092 (TR-29). (3) Solo se la scarica propaga: una propagazione fuori dal Cleanup.

**Esempio.** *Given* Riktor in (0,0,0), HP 1200, DEF 45, `PushResistance` 0, budget 6, scudo base già eroso; nemico M adiacente in (1,0,0), nell'arco frontale di Riktor, nessuna copertura, Layer 0, HP 1000, RES Electric 0, che pianifica un attacco in mischia `Single`, portata 1, da 200 `Direct`, con Push 1. Riktor pianifica Move 2 + Shock Whip 2 + Magnetic Bastion 1 + Shock Armor 1 = 6 AP. *When* M colpisce Riktor nel Blast. *Then* Riktor subisce floor(200 × 100/145) = 137 → HP 1063; la scarica colpisce M per 160 (PROPOSTA sul valore) → 840; la spinta è annullata nel punto `BlastDisplacement` e Riktor resta in (0,0,0) **solo se** cade il limite di una attivazione per turno di D-092 o se l'anti-spinta non conta come seconda attivazione (TR-29, TR-28); nessuna propagazione (TR-28). Controllo: un colpo `Single` da portata 3 con Push non fa scattare la reazione, e la spinta si applica.

**Domande.** TR-28 (significato e danno di «applica Fulmine», mischia, colpo intercettato, quali spinte, una o due voci, aree, costo, fuoco amico della propagazione, soggetto di D-109), TR-29 (più attivazioni), TR-1 (tipo di danno), TR-2 (scudo base), TR-4 (ripetizione), TR-5 (cooldown), TR-6 (posto nella dock).

---

## Combo (candidati per il catalogo delle coppie)

Una coppia produce il suo effetto per chiunque la crei: **nessuna combo è di Riktor**, e la scheda dell'eroe non le elenca [DECISO D-275 owner; AGENTS.md §3 invarianti 15 e 16; LETTURA D-612 (7)]. In nessuna riga «in Prep», «in Dash», «in Blast» ha un posto, perché una coppia non ha una fase; oggi Water+Electric risolve nel Cleanup (referto §3.5) [DOMANDA → TR-31: serve una regola di fase per le coppie?].

| Combo del catalogo | Riga del [catalogo delle coppie](../../balance/RT_ElementPairCatalog_v0.1.md) | Proposta |
|---|---|---|
| «Fango Magnetico» (⚡ Fulmine + 🪨 Terra in Prep): «Dimezza i Punti Movimento dei nemici nell'esagono.» [CATALOGO] | Earth+Electric, OPEN (riga 71), con già un candidato GDD, «Faglia ionizzata (attira i proiettili)» | **secondo candidato** della riga, accanto a Faglia ionizzata, con l'effetto alla lettera; l'autore sceglie [DOMANDA → TR-31]. Il nome urta il «Fango» candidato di Water+Earth (riga 62, «blocca il Dash») [PROPOSTA: rinominare, per esempio «Terreno magnetico»]. «Dimezza i Punti Movimento» vuole un arrotondamento dichiarato [PROPOSTA: per difetto, minimo 1] e si vede come celle non raggiungibili [DECISO D-190]. Si sovrappone alla penalità di MP di Magnetic Spikes. Il filtro «dei nemici nell'esagono» non esiste per una coppia: vale per chi sta nella cella [PROPOSTA; invariante 16] |
| «Catene Spettrali» (⚡ Fulmine + 💀 Morte in Dash): «Paralisi oscura che disattiva le Reaction nemiche.» [CATALOGO] | Electric+Decay, OPEN, senza input (riga 80) | **primo input** della riga. Chiede uno stato nuovo che sospende la lista di reazioni armate [DECISO D-592], valutato sullo snapshot, e una regola di privacy: l'evento dice che le reazioni sono sospese, non quali fossero [PROPOSTA; AGENTS.md §4]; «nemiche» → «di chi sta nella cella» [PROPOSTA] |
| «Sovraccarico Elettrolitico» (⚡ + 🪨 + 💧, «+450 Danni») [CATALOGO] | nessuna: tre elementi | **non adottata**: nessuna regola accoglie combo a tre elementi [DECISO D-591 (1), punto (a) del GDD superato] |
| «Gabbia di Faraday Letale» (⚡ + 🪨 + 💀, danno puro proporzionale agli HP mancanti, cura Riktor) [CATALOGO] | nessuna: tre elementi | **non adottata** [DECISO D-591 (1)]; viola anche le invarianti 15 e 16 («cura Riktor» dà un proprietario all'effetto) e «danno puro» non esiste in D-589. Aprire una regola a tre elementi: [DOMANDA → TR-31] |

## Affinità e matchup

Note di design per la [scheda di Riktor](../../characters/v0.1/riktor.md), non campi del contratto né righe del catalogo delle coppie [PROPOSTA].

- **Nomi superati**: Phase → Muriel [DECISO D-586 (1)]; Flux → Warp [DECISO D-588, D-593].
- **«Legame Cristallino»** non esiste in nessun documento (`git grep` → 0, referto §2); la meccanica simile del GDD è «Consecrated Ground» di Muriel, che rende immuni a Pull e Knockback [GDD; [scheda di Muriel](../../characters/v0.1/muriel.md)].
- **Contraddizione del GDD**: la scheda di Riktor mette Muriel fra i suoi «Forte contro» (riga 222); quella di Muriel mette Riktor fra i propri (riga 317, citata dal referto); il catalogo segue la seconda [DOMANDA → TR-32].
- **«Kallari (l'aura pre-caricata la stordisce in Dash)»**: nessuna voce del kit stordisce nel Dash (Electrocute fa 80 danni nel Cleanup, Skewer stordisce nel Blast); il matchup non ha oggi una meccanica che lo sostenga [DOMANDA → TR-32].
- **«Murdock (sradicato dalle coperture)»** rimanda a «sradicando i cecchini» di Chain Hook, da definire [DOMANDA → TR-19].
- **«Steel (le barriere bloccano le traiettorie di trazione)»**: coerente con la linea di vista `Required` di Chain Hook e con la traiettoria libera di Intercept [PROPOSTA].
- Non verificato se Murdock, Gadget, The Fey, Kallari e Steel siano i nomi correnti del roster dopo D-586 e il rename di #4011.

## Equipaggiamento di default

«Arma, gadget e modulo di reazione si scelgono di nuovo, coerenti col catalogo elettrico, nella stessa D-nnn del kit» [DECISO D-612 (6)]. Oggi: `Weapon.Impact` (portata −1 e una spinta sull'attacco base, `RTCatalogLibrary.cpp:783`), `Gadget.PortableCover` (copertura bassa), `Reaction.Cleanse`. Qui solo proposte e domande:

- **Arma** [PROPOSTA]: nessun modificatore di portata sull'attacco base, perché in `HexCone` portata e profondità sono lo stesso numero (`RTHexLibrary.cpp:780`): un −1 svuota Static Sweep, un +1 lo porta a 8 celle. Se la nuova arma aggiunga ancora una spinta all'attacco base, e a quale dei tre [DOMANDA → TR-34].
- **Gadget** [DOMANDA → TR-34]: il catalogo non ne nomina; `PortableCover` resta o si sostituisce?
- **Modulo di reazione** [PROPOSTA]: **non** `Reaction.Anchor`, perché l'anti-spinta di Shock Armor ne sarebbe una seconda sede (CLAUDE.md §4). `Reaction.Cleanse` perde una delle sue due ragioni con l'uscita dello Slow di `ImpactShot` [DECISO D-612 (6), testo scelto]; se resti [DOMANDA → TR-34].

## Statistiche

Restano quelle di oggi finché la `D-nnn` non decide: **HP 1200, MP 4, DEF 45, RES 35, spalle 30** (`MakeBranth`, `RTHeroCatalogLibrary.cpp:775-804`; anche vista 5, `PushResistance` 0). Il GDD dà a Riktor 1750 HP, fuori dalla fascia 900–1200 della scala ×10 di [D-589](../../decisions/RT_PDR_00_Decision_Log.md) [DOMANDA → TR-35]. La RES è per elemento [DECISO D-589]: quale RES Electric, Earth, Decay abbia Riktor è input per E53 (#4013).

## La dock provvisoria

La dock provvisoria di Riktor ha **un attacco base, sei abilità, due reazioni e l'Overclock** [DECISO D-612 (4)]; le voci del kit restano tutte nel kit [DECISO D-612 (2)].

- **`KIT-1`** (struttura del kit): la (2) accetta le conseguenze che il suo testo nomina (tetto, struttura, input) ma non la forma della struttura nuova, che la (3) lascia aperta [LETTURA D-612 (7)].
- **`KIT-2`** (chi riempie la dock, e quando): aperto [DECISO D-612 (4)]. Proposte di questa traduzione: Shock Whip nel posto base; le due reazioni del catalogo nei due posti reazione. Una scelta del giocatore prima della partita va misurata contro il testo scelto di D-598 («lo snapshot non porta un loadout») [LETTURA D-612 (7)].
- **`KIT-3`** (la skill di overclock di Riktor non esiste): l'unico input è il GDD riga 222, «In Overclock privilegia il sacrificio della propria Dash Phase futura per garantire la presa immediata». [PROPOSTA di candidato, non numero deciso] una skill scelta nel planning che dà +2 AP nel turno in corso, che fa parte del piano prima della validazione [LETTURA D-613 (5)], e blocca la fase Dash di Riktor nel turno dopo; coerente con un kit il cui Dash è la presa (Chain Hook). Servono il budget per unità [DECISO D-613 (1)] e uno stato «fase bloccata nel turno dopo», che non è fra gli stati di oggi. Non è l'Overclock universale del GDD, che resta superato [DECISO D-591 (1) (d)]; una meccanica di kit è ciò che D-265 ammette [DOMANDA → TR-33].

## Che cosa serve al motore

Cose da **creare**, consolidate; fra parentesi le voci che le chiedono. Le estensioni minori stanno nelle sezioni per voce.

1. **Campo elemento su `FRTActionDef`** (`Ability/RTActionDef.h`, in coda): oggi assente. Tutte le voci.
2. **Percorso di danno elementale**: una sorgente o un tipo di danno che porti l'elemento e scelga la RES (`ARTUnit::ResistanceAgainst`, oggi senza lettori di gioco) al posto della DEF; oggi `ERTDamageSource` = {`Direct`, `Environmental`}. ShockWhip, StaticSweep, TaserStrike, Electrocute, VoltageLeap (se elementale), CorrosiveShackles, ArcDischarge, Skewer, ShockArmor. Insieme: la regola dello scudo base di D-224 sul colpo elementale (TR-2).
3. **Coordinata d'ordine di dichiarazione**: l'ordine entra nel piano, nello snapshot e nell'hash [DECISO D-613 (3)]; come coordinata in `FRTUnitPlan` e nel TurnLog [LETTURA D-613 (5)]. Tutte le voci che possono pareggiare dentro il piano di Riktor.
4. **Budget AP per unità e per turno** in snapshot, hash e replay [DECISO D-613 (1)], con la versione dello schema e la lettura da `ValidatePlan` [LETTURA D-613 (5)]; poi un **effetto di budget del turno dopo** in coda a `ERTActionEffect` con la sua voce di TurnLog. NecroticAnchor, MagneticBastion, la skill di overclock (`KIT-3`).
5. **Bersaglio secondario derivato** con il proprio danno ed eventi ordinati per Stable ID, con la proiezione privacy. ShockWhip (catena), ArcDischarge (rimbalzo).
6. **Ripartizione del danno** (solo con la lettura «diviso»). StaticSweep (TR-11), Electrocute (TR-15), VoltageLeap (TR-20), Skewer (TR-25).
7. **Stati nuovi**: `Status.Hindered` con inizio differito (TaserStrike); `Status.HealBlocked` letto nei due percorsi della cura e, se adottato, in `Shield` (CorrosiveShackles, IronMaiden); `Status.Grounded` (MagneticSpikes); stato della vista / `Status.Caged` con la regola nel servizio LOS (IronMaiden); modificatore «DEF a 0» (IronMaiden, ramo Fango). Ognuno con tag, glossario, icona `UI.Icon.Status.*`.
8. **Aura di cella persistente** (`FRTDynamicAura` o simile) con tick nel Cleanup, snapshot, replay, **copertura nuova dell'hash** e un effetto `CreateAura`. Electrocute; ShockArmor se «applica Fulmine» è una cella che dura.
9. **Modificatore di costo di movimento come delta**, separato dalla superficie e attivo dal turno dopo, più un effetto d'ingresso filtrato per fase e limitato a una volta per unità per turno, e il permesso di cambiare superficie da un'azione di Preparation. MagneticSpikes.
10. **Boundary predittivo nel Dash**: valore in coda a `ERTPredictionBoundary`, chiamata nella risoluzione del Dash, esito predittivo senza danno né troncamento, `TriggerMatched` nel TurnLog. NecroticAnchor.
11. **Ramo per tipo di bersaglio** (unità → Pull, muro → `SelfReposition`) o due voci; con la lettura (b), uno spostamento forzato di un'altra unità nel passo FastMovement dopo `ResolveHexPaths`. ChainHook.
12. **Impatto d'area all'atterraggio di un `LinearLeap`**, prodotto nel Dash e consumato da `AppendChargeImpactIntents` con forma `Area`. VoltageLeap.
13. **Shield applicato da un'azione Dash a chi la usa**, alla fine del Dash (case `Shield` negli effetti su sé delle mobilità, o un destinatario per effetto in `FRTActionEffectSpec`); scudo con durata se l'autore lo vuole. RiotCharge.
14. **Termine di danno condizionato a un predicato geometrico** (lato bloccante dietro il bersaglio) e un **evento TurnLog** di schiacciamento distinto da `Fall.Impact`. SeismicSmash.
15. **Percorso di Prep con bersaglio ostile** in `ResolvePrep`, e un **punto di passata delle reazioni nel Prep** in coda a `ERTReactionPassPoint`. IronMaiden.
16. **Trigger `HitByMeleeAttack`** con i casi in `PassPointFor` e `FindTriggeringAttacker`, e una **reazione a due momenti** (o due voci). ShockArmor.
17. **Lista delle reazioni armate** [DECISO D-592] e la decisione sul contatore di attivazioni di D-092 (TR-29). MagneticBastion, ShockArmor.
18. Solo se l'autore lo decide: uno Stun che preceda la raccolta degli intenti del Blast (Skewer, TR-25); una propagazione elettrica fuori dal Cleanup (ShockArmor, TR-28); un tipo di danno che salta DEF/RES, copertura e Guard (MagneticSpikes, ArcDischarge, Skewer, TR-3); un glifo Morte (CorrosiveShackles, TR-22).

## Domande per l'autore

1. **TR-1 · Tipo di danno.** «Danno Energetico» e «Danno Oscuro» sono danno elementale (RES dell'elemento) o diretto (DEF)? Un colpo diretto che porta un elemento («Danno Fisico» con Terra) paga DEF, RES o entrambe? (REQ-08, REQ-10, ARC-08) — ShockWhip, StaticSweep, TaserStrike, Electrocute, MagneticSpikes, ChainHook, CorrosiveShackles, SeismicSmash, ArcDischarge, Skewer, ShockArmor; il tipo dell'onda di VoltageLeap è in TR-20.
2. **TR-2 · Scudo base.** Lo scudo base di D-224 (50), che oggi ferma solo il `Direct`, ferma anche un colpo elementale? — le voci proposte come elementali: ShockWhip, StaticSweep, TaserStrike, Electrocute, VoltageLeap, CorrosiveShackles, ArcDischarge, Skewer, ShockArmor.
3. **TR-3 · Danno «puro».** «Fisici puri», «danni garantiti», «ignora l'armatura»: un tipo che salta DEF/RES, copertura e Guard, o solo «sempre applicato» / «DEF ignorata»? — MagneticSpikes, ArcDischarge, Skewer.
4. **TR-4 · Ripetizione.** La lettura di D-613 (5) (con cooldown una volta per piano, senza cooldown ripetibile) è quella voluta? Che cosa significa la riserva «ci sono abilita' ripetibili, come muoversi» (`MSP-1.r`)? — tutte: i tre attacchi base (raffica ×3) e ogni voce con cooldown.
5. **TR-5 · Cooldown.** Confermi gli input del GDD (Electrocute 1, ChainHook 2, CorrosiveShackles 2, Skewer 3, IronMaiden 3, ShockArmor 1, MagneticBastion 2) e i valori proposti dove mancano (NecroticAnchor 1, MagneticSpikes 2, VoltageLeap 2, RiotCharge 2, SeismicSmash 2, ArcDischarge 2)? — le voci nominate.
6. **TR-6 · Dock e struttura** (`KIT-1`, `KIT-2`). Chi occupa il posto base, le sei abilità e i due posti reazione, e quando si sceglie? Che ruolo hanno gli altri due attacchi base? Servono badge nuovi in `skill.af` (catena, area, riduzione di movimento)? — tutte.
7. **TR-7 · Catena di Shock Whip.** Un solo salto agli adiacenti del bersaglio o catena fra unità bagnate, con quante celle al massimo? Il secondario fa 160 pieni o un danno ridotto? — ShockWhip.
8. **TR-8 · Shock Whip e Water+Electric.** Shock Whip è una manifestazione della riga CURRENT `Reaction.ElectricPropagatesThroughWater` (proposta, come LinearDischarge) o un riuso della propagazione (lettura del referto §2, §3.2)? Un colpo Electric su un bersaglio Wet o in acqua attiva anche la reazione della coppia, o la catena la sostituisce? — ShockWhip.
9. **TR-9 · Fango e Vapore.** Entrano come stati o superfici canonici, o la condizione resta «Wet o acqua»? — ShockWhip.
10. **TR-10 · Portate non date.** Shock Whip 1–2 (GDD), Taser 1–2, Arc 1–3, profondità di Skewer 2, Static Sweep 1: vanno bene? Se la nuova arma tocchi la portata è TR-34. — ShockWhip, StaticSweep, TaserStrike, ArcDischarge, Skewer.
11. **TR-11 · Static Sweep.** 120 per cella colpita o 120 divisi sulle tre celle? — StaticSweep.
12. **TR-12 · Fuoco amico.** Gli alleati nell'area sono colpiti? — StaticSweep, Electrocute, VoltageLeap, Skewer.
13. **TR-13 · Reazioni su colpi d'area.** Con più bersagli colpiti da un'area, qualcuno può reagire? Oggi `HitByDirectAttack` è dei soli `Single`. — StaticSweep, VoltageLeap, Skewer.
14. **TR-14 · Taser Strike.** La riduzione vale dal turno successivo (lettera) o già sul Move del turno in corso, come lo Slow? Più Taser nello stesso turno sommano o rinnovano? Si cumula con Slow e con il −1 di Prone? Nome e icona dello stato? — TaserStrike.
15. **TR-15 · Electrocute.** Sei adiacenti o sola cella di Riktor? Gli 80 vanno a ciascun nemico dell'area o si dividono (lo stesso dubbio di TR-11)? Legata alla cella o a Riktor? Tick nel Cleanup, due tick (N e N+1)? I «danni di ritorno su colpi melee» del GDD entrano, e allora come reazione propria con un trigger nuovo «colpito in mischia» e un costo da dichiarare (D-592, D-109, D-594)? Due aure sulla stessa cella si rinnovano o si sommano? Quale `PropagationLimit`? — Electrocute.
16. **TR-16 · Necrotic Anchor.** Dura un turno (predittiva) o più turni (oggetto nascosto, confine di privacy nuovo)? Visibile agli avversari? Un salto che atterra sulla cella la fa scattare? Due Anchor sommano (−2), con quale tetto e quale budget minimo? Più nemici perdono 1 ciascuno o si consuma al primo? Il −1 si mostra anche come icona di stato? Il bot deve poterla usare (oggi D-610 (4) lo esclude)? — NecroticAnchor.
17. **TR-17 · Magnetic Spikes.** Area (raggio 1?) e portata (1–3?); durata (2 turni?); il costo vale già nel turno del cast o dal turno dopo; costo assoluto 2 o +1 sul costo che c'è; lo scatto si ferma alla prima cella; «ancorato a terra» vieta solo salto e cambio di Layer o fa anche cadere al Layer 0, e per quanto; l'acqua sotto le punte resta acqua; il costo vale anche per alleati e Riktor; celle non sostituibili (Void). — MagneticSpikes.
18. **TR-18 · Movimento forzato sulle celle preparate.** Una spinta o una trazione nel Dash attraverso le celle di Anchor e di Spikes le fa scattare? Va deciso in modo uniforme. — NecroticAnchor, MagneticSpikes, ChainHook.
19. **TR-19 · Chain Hook.** Lettura (a), Control dentro il Blast con cornice Dash, o (b), trazione vera nel Dash? Trazione di 2 fisse o fino all'adiacente (con tetto 2)? Il ramo «verso un muro»: tipo di bersaglio, seconda voce o modalità; fa danno; quali ostacoli; quale fallback? «Sradicando i cecchini» vuol dire azzerare la `CoverSelection`? Alleati agganciabili? Bersaglio su una delle sei direttrici? — ChainHook.
20. **TR-20 · Voltage Leap.** Salto esattamente 2 o fino a 2? 90 per nemico o divisi (lo stesso dubbio di TR-11)? Tipo di danno dell'onda? «Body Block» = spinta non applicata (`NoDestination`), o un effetto in più, contrario a D-445? Danno d'urto della spinta bloccata? Con il salto fermato, l'onda parte? — VoltageLeap.
21. **TR-21 · Riot Charge.** Che cosa fa a chi è sulla traiettoria (danno, quanto e di che tipo; spinta laterale; niente)? «Chiunque» comprende gli alleati? «Inarrestabile» = solo `LinearPass`, o anche immune a interruzione (`InterruptPolicy::None`), Root e spostamenti? Lo scudo dura oltre il Cleanup? Si ottiene anche con la corsa fermata? Un esito diverso da D-398 per l'ultima cella occupata? — RiotCharge.
22. **TR-22 · Corrosive Shackles.** «Raggio di 2» = portata 2 su un bersaglio o area di raggio 2? Il blocco ferma solo `Heal` o ogni aumento di HP, e anche gli scudi (divergenza dal referto §3.2)? I due turni contano quello di applicazione? Il blocco vale sulle cure dello stesso Blast (dipende da dove il resolver applica lo stato rispetto a `ApplyPlannedHeals`)? Serve un glifo Morte nuovo? — CorrosiveShackles, IronMaiden.
23. **TR-23 · Seismic Smash.** Chi sbalza: Smash stesso (spinta di 1) o un'altra fonte (cambio di contratto)? Quali lati valgono come «muro perimetrale solido» (bloccante, parapetto, copertura bassa, struttura, unità dietro)? Una caduta da un lato aperto dà il bonus? Il +50% entra in `Mod` o è un danno separato? Se Anchor annulla la spinta il bonus vale? Il colpo danneggia il muro (`DamageStructure`)? — SeismicSmash.
24. **TR-24 · Arc Discharge.** Confermi «condivide il macro-esagono» → «compagno del bersaglio adiacente»? Con più compagni, uno solo (il `StableUnitId` più basso) o tutti? Il rimbalzo conta come colpo diretto e fa scattare le reazioni del compagno? Rivela un'unità che la squadra di Riktor non vede? Richiede linea di vista dal bersaglio? — ArcDischarge.
25. **TR-25 · Skewer.** 380 a ciascuna cella colpita o divisi sul cono (lo stesso dubbio di TR-11)? «Stun per 1 turno»: durata 1 (resto del turno N) o 2 (anche N+1)? «Priorità deterministica sui bersagli già elettrificati» si toglie o diventa una condizione su Wet con un bonus, e quale? Vuoi che lo Stun preceda gli attacchi dello stesso Blast (regola nuova del resolver)? — Skewer.
26. **TR-26 · Iron Maiden.** Terra o Morte (un'azione porta un solo elemento)? Durata 1 o 2? «Bloccandone… Linea di Vista»: il bersaglio non mira fuori, gli altri non mirano lui, o entrambe, e anche la vista di terzi attraverso la cella? «Scudi»: niente scudi nuovi o annulla anche quello che ha? Nega un'azione principale che non chiede vista? Una `Status.Caged` o la composizione Root + HealBlocked + stato di vista? Il Fango è Water+Earth o il «Fango Magnetico» Electric+Earth del tuo catalogo, e «azzerando l'armatura» = DEF a 0 per la durata? `Reaction.Cleanse` rimuove la gabbia? Le reazioni anti-controllo devono vederla nel Prep? — IronMaiden.
27. **TR-27 · Magnetic Bastion.** Costo 1 (la voce del catalogo) o 2 (classe), visto che il titolo del gruppo e il GDD dicono «1–2»? Portata 2 o solo l'alleato adiacente? Il tetto «fino a 300 danni» è caduto apposta, e se resta dove va l'eccedenza? Riktor si sposta davvero (lettura b), chi sceglie la cella, e il colpo si ricalcola dalla cella di partenza o d'arrivo? Il +1 si cumula con un overclock o con un secondo Bastion? Solo colpi `Single` o anche aree (la regola generale è TR-13)? Confermi copertura e spalle dell'intercettore (comportamento di oggi)? Cooldown allo scatto, come oggi? Che nome visibile, visto che «Guardia» è già `Action.Guard`? — MagneticBastion.
28. **TR-28 · Shock Armor.** «Applica Fulmine sull'esagono dell'attaccante» = danno immediato, propagazione nell'acqua o cella elettrificata che dura, e con quanto danno? «In mischia» si legge sulla portata dichiarata o sulla distanza effettiva? Un colpo intercettato da Bastion conta come colpo in mischia su Riktor? «Qualsiasi Knockback» = solo quello del colpo o ogni spinta del turno, e anche le trazioni? Una voce con due momenti o due reazioni? Aree e coni in mischia (la regola generale è TR-13)? Costo 1 confermato (il cooldown è in TR-5)? Fuoco amico se la scarica propaga? La soglia di D-109 si legge sull'attaccante o su Riktor? Il cooldown che parte allo scatto: vale la risposta di TR-27. — ShockArmor.
29. **TR-29 · Più attivazioni per turno.** Con le reazioni in lista (D-592), un'unità può **attivarne** più d'una nello stesso turno, o resta il limite di D-092? — MagneticBastion, ShockArmor.
30. **TR-30 · Due Dash in un turno.** Un'unità può fare due Dash nello stesso piano (oggi lo slot Movement di D-191 lo vieta, ADR-0011 lo destina a cadere)? Tocca anche `DashEndPivotMaxSteps`. — ChainHook, VoltageLeap, RiotCharge.
31. **TR-31 · Combo.** «Fango Magnetico» o «Faglia ionizzata» sulla riga Earth+Electric, e con quale nome? Le coppie hanno bisogno di una regola di fase? Vuoi aprire una regola per le combo a tre elementi? — combo del catalogo.
32. **TR-32 · Matchup.** Chi è forte contro chi fra Riktor e Muriel (il GDD si contraddice)? Quale meccanica sostiene «l'aura pre-caricata la stordisce in Dash» (Kallari)? — affinità e matchup.
33. **TR-33 · Overclock.** La candidata (+2 AP nel turno, Dash bloccato nel turno dopo) va bene? Costo AP della skill, cooldown, Ultimate o posto a sé? Quale tetto al budget (non deciso, D-613 (5))? — `KIT-3`, MagneticBastion.
34. **TR-34 · Equipaggiamento.** Arma senza modificatore di portata, e con o senza spinta su un attacco base? Gadget? Modulo di reazione (non Anchor; Cleanse resta?) — D-612 (6), ShockWhip, StaticSweep, TaserStrike.
35. **TR-35 · Statistiche.** HP restano 1200 (fascia di D-589) o si va verso i 1750 del GDD? RES per elemento di Riktor? — statistiche.

## Provenienza e limiti

- **Base**: `origin/main` `2d0c1db0c`, worktree `riktor-kit-traduzione`. Il catalogo è letto dall'Appendice A del [referto](riktor-hud-ap-spec-panel-2026-11-13.md); le decisioni dalle righe `D-583`…`D-613` del [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md); i nodi aperti `MSP-1.r`, `MSP-5.d` stanno in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).
- **Metodo**: una traduzione per gruppo (attacchi base, Prep, Dash, Blast primaria, Ultimate, reazioni e combo), ciascuna con un tetto di chiamate e in sola lettura; per ogni gruppo un verificatore indipendente ha riletto le citazioni sul codice e sul Decision Log e prodotto correzioni; l'assemblaggio di questa pagina ha applicato **ogni** correzione (il valore del verificatore sostituisce quello del traduttore; una correzione posta come domanda è diventata DOMANDA). Le correzioni più pesanti: lo Shield di Riot Charge non ha un punto d'applicazione nel motore (il referto, riga 72, ARC-11, lo dava per riuso); la priorità non ordina gli effetti dentro il Blast (D-503) né i Dash fra loro (`RTHexSimLibrary.h:315-324`); lo Stun di Skewer non nega gli attacchi dello stesso Blast; la ripetizione e la «una volta per piano» sono una **lettura** di D-613 (5), non una decisione; l'hash non copre le scadenze; un percorso elementale e una lettura della RES non esistono.
- **Divergenze dichiarate dal referto**: la catena di Shock Whip (manifestazione della coppia contro riuso della propagazione, TR-8); `HealBlocked` che non legge `Shield` in Corrosive Shackles (TR-22); lo Shield di Riot Charge (da creare, non da riusare); l'esempio dello Slow di §3.3 da rimisurare.
- **Non verificato**: il comportamento di `BlindAimDirect` sul cono; il filtro di `HexCone` su muri e coperture; se `Entered` includa la cella d'arrivo del Dash; la famiglia di `Leap` e se il suo atterraggio passi per `ApplyOnEnter`; `KnockFrom` dopo un salto; se gli spostamenti di Control siano già applicati quando si calcola il danno di Attack; dove il resolver applichi lo stato di un colpo rispetto a `ApplyPlannedHeals`; il danno d'urto di una spinta bloccata; l'ordine fra Shield, copertura e Guard; se il danno `Environmental` passi oggi per una mitigazione; la sede del servizio LOS e dei tag di stato; le icone reali di `skill.af` (Appendice B è una lettura della sessione); se i nomi di Murdock, Gadget, The Fey, Kallari e Steel siano correnti. Le righe di codice citate sono **lette**, non eseguite.
- **Gate**: nessun gate di codice eseguito. Compile, Automation, determinismo, replay, privacy, PIE e packaged: **`NOT RUN`** / `N/A` per un documento di proposta. Unreal non avviato.
