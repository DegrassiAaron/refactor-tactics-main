# Triage — «FULL CHAT CONSOLIDATION: griglia, geometria, muri, cover, traversal, strutture, acqua»

> `CURRENT` · **Data**: 2026-08-10 · **Modo**: `/sc:spec-panel`
> **Sorgente**: [`2026-08-10-full-grid-geometry-walls-water.md`](../../archive/src/handoff/2026-08-10-full-grid-geometry-walls-water.md)
> — **3159 righe**, 55+ sezioni marcate `LOCKED`, quinto sorgente della giornata.
>
> **Owner di una sola domanda**: quali sezioni cambiano il canone, e quali descrivono codice già spedito.

## 0. Il risultato in quattro righe

**Il documento è il più grande e il meno consumabile in un colpo solo**: 55 sezioni `LOCKED` che coprono
geometria, cover, LOS, traversal, occupazione, verticalità, distruzione strutturale, acqua ed elettricità —
cioè il design di mezzo gioco.

- **Un conflitto reale**, sulla soglia di calpestabilità, **risolto dall'autore a favore del canone**.
- **La sua «ultima decisione prima della pausa» è già implementata e testata** da CP 8.3.
- **Tre blocchi genuinamente nuovi e grossi**: acqua dinamica, strutture/crolli, traversal verticale.
- **Due nomi già presi** con significato diverso.

## 1. Il conflitto — soglia di calpestabilità · **risolto**

La §F e gli scenari `SCN-GRID-04`/`SCN-GRID-05` usano un test **sul centro**:

```text
muro passa per il centro tattico        → Cell = Blocked
muro attraversa l'hex ma NON il centro  → Cell may remain Walkable
```

[D-071](../../decisions/RT_PDR_00_Decision_Log.md) — decisa e mergiata **la mattina dello stesso giorno** —
usa il **cerchio inscritto**: un muro che entra nel nucleo invalida la cella anche se manca il centro esatto.
I due modelli divergono in una banda precisa: muri dentro il cerchio ma fuori dal centro.

> ✅ **Decisione dell'autore (2026-08-10): prevale `D-071`.** La fonte è **superata su questo solo punto**.
> `SCN-GRID-04` e `SCN-GRID-05` non si scrivono come sono: vanno riformulati sul cerchio prima di diventare
> scenari, altrimenti pinnano la regola sbagliata.

Il resto del blocco geometria **non è in conflitto**, e due punti anzi lo rafforzano:

| § | Cosa dice | Rapporto con il canone |
|---|---|---|
| **B**, **C**, **E** | La griglia non è una piastrellatura; `HexEdge == WallSegment` è rifiutato; il runtime non interroga la mesh | **Conferma [D-065](../../decisions/RT_PDR_00_Decision_Log.md)** da una terza fonte indipendente |
| **I.5** | «doorway come posizione transit-only» è **scartata** | **Conferma il corollario di `D-071`**: il varco «ci passo ma non ci sto» non esiste, ed è una scelta, non un limite subìto |
| **B** | «niente footprint continuo come autorità» | **Non** contraddice `D-071`: lì il footprint è un predicato di **cottura** e il runtime legge interi |
| **D** | La dimensione dell'hex è **configurabile**; 1.5 m era indicativo | **Corregge una parentesi di `D-071`**, che la chiamava «già fissata». La sostanza regge: l'apotema è *relativo* al lato, quindi resta zero numeri nuovi qualunque sia il lato |

## 2. La «ultima decisione prima della pausa» è già spedita

La §53 è marcata `LOCKED — RESUME MARKER` e dice:

> *«L'elettricità nell'acqua si propaga attraverso la connettività logica della rete d'acqua, non tramite
> semplice raggio geometrico.»*

**È già il canone, ed è già codice.** [`spec-propagazione-elettrica-cp83.md`](../../gameplay/spec-propagazione-elettrica-cp83.md)
§D1 si intitola *«Il percorso è il grafo dell'acqua, non un raggio»*, la propagazione è una **visita in
ampiezza** su `bConductsElectricity` in `URTTerrainLibrary::CollectElectricPropagation`, e il caso limite che
la fonte teme è **pinnato da un test**: `StopsAtNonConductive` verifica che un'unità sull'asciutto **non sia
un ponte**, nemmeno se adiacente.

> 🎲 **NYGARD**: «Il segnalibro dice *riprendi da qui*. Il punto in cui riprendere è a valle: quella decisione
> ha già superato una code review e un test. Ciò che resta aperto sono le sette domande che la §53 elenca
> *dopo* — attenuazione, durata, interazione con le valvole — e nessuna di quelle è decisa.»

## 3. Due nomi già presi

| La fonte usa | Nel repository significa | Esito |
|---|---|---|
| **`Breach`** come *slot strutturale* con integrità, stati, slot adiacenti e riparazione (§40) | **`Gadget.BreachCharge`**: una carica da sfondamento dell'equipaggiamento, **35 danni a struttura** (E7) | **Collisione semantica.** Un `BreachSlot` accanto a una `BreachCharge` è la frase «la carica apre uno slot» che si legge in due modi |
| **`ForcedEscape`** (§51) | — libero | ok |

Da verificare al momento della spec: il modello a slot ha bisogno di un nome che non collida con l'oggetto
che li apre.

## 4. Triage per blocchi

| § | Blocco | Esito |
|---|---|---|
| A–E, G, H, J | Griglia, geometria, direttrici, bake, transition | `ALIGNED` — è `D-065`, confermato |
| F | Soglia di calpestabilità | **`CONFLICT` risolto** → §1 |
| I | Undici decisioni «da non reintrodurre» | `ALIGNED` — nessuna delle undici è nel canone. La **10** (ZoC automatica) non ha mai avuto un'implementazione: `grep` la trova **solo** in un PDF archiviato e in due poster |
| K, M, N | Wiki, feature map, epic da creare | `PROC` → §5, §6 |
| L | `SCN-GRID-01…08` | **`CONFLICT` parziale**: gli ID non sono la convenzione (`Spec.<Area>.<Nome>`), e 04/05 pinnano la soglia sbagliata |
| 7 | Cover quantizzata su **6 settori** | `ALIGNED` — `FRTHexCover{Edge, Type, Integrity}` è già per bordo, e `BlocksTraversal` è condiviso fra grafo e vista |
| 8–10 | Cover e quota, high ground, full wall | `ALIGNED` — «nessun bonus numerico alla vista» è già `D-018`/`D-024` |
| 11–13 | Unità come ostacoli, occupazione, collisioni simultanee | `ALIGNED` in gran parte: le catene di collisione e lo swap sono già in CP 4.8 (`BlockedByPriority`, `BlockedByImpact`) |
| 14 | Nessuna ZoC universale | `ALIGNED` per assenza — non c'è niente da togliere |
| 15–16 | Overwatch segreto, reveal, intel | `ALIGNED` — ADR-0004 §7 e §7-bis ([D-021](../../decisions/RT_PDR_00_Decision_Log.md)) coprono privacy e sospensione |
| 17 | **Vault / low wall** | **`NEW`** — nessuna traccia in `Source/` |
| 18–21 | Hazard su cella e transition, ordine, trigger acquisiti | `NEW` **parziale** — gli hazard di cella esistono (E8), quelli **su transition** no |
| 22–27 | Move end semantics, grafo causale, provenance, credit | **`NEW`** e vicino a lavoro in corso: il TurnLog v6 e i reason code sono atterrati oggi (`D-062`, `D-063`, `D-067`). Va riconciliato con chi ci sta lavorando, **non** consolidato al buio |
| 28–39 | Layer, cadute, drop, salita, push sui bordi, displacement a catena | **`NEW`** in gran parte; il knockback e `D-045` esistono, la verticalità no |
| 40–48 | Breach slot, dipendenze strutturali, crolli, macerie, connettività | **`NEW`** — il blocco più grande e più autonomo |
| 49–52 | **Acqua: profondità, flooding, corrente** | **`NEW`** — `WaterDepth` e `Flooding` hanno **zero** occorrenze nel repo; `ShallowWater` esiste solo come **una delle otto superfici** di CP 8.1, non come asse |
| 53 | Elettricità sulla rete d'acqua | ✅ **già implementata** → §2 |
| 54–55 | Regole generali, UI/debug | `ALIGNED` — determinismo, no fisica autorevole, `GraphRevision`, privacy sono invarianti già scritte |

## 5. Cosa entra

Tre feature nuove, tutte **v0.2 o oltre** e tutte `IDEA`/`DESIGNED`: il documento è una conversazione, non
una spec, e nessuno dei tre blocchi ha ancora un owner documentale.

| Feature | Copre | Perché è un blocco a sé |
|---|---|---|
| `RT-FEAT-MAP-WATER-DYNAMICS` | §49–§52 | `WaterDepth` è un **asse ortogonale** alla superficie, non una nona superficie: una cella può essere `ShallowWater` e cambiare profondità durante il match. Flooding e corrente producono `ForcedMovement`, quindi toccano il resolver |
| `RT-FEAT-MAP-STRUCTURAL` | §40–§48 | Integrità, dipendenze, crolli a catena e macerie. È l'unico blocco che si regge da solo senza toccare gli altri |
| `RT-FEAT-MAP-VERTICALITY` | §17, §28–§39 | Vault, drop, salita, cadute e displacement a catena. Dipende dai Layer, che esistono |

## 6. Epic

La §N chiede un *EPIC — Hex Grid & Architectural Geometry Foundation*. **Non serve crearlo**: è **E23**
(Muri, porte e interaction graph, v0.2), che dopo `D-065` e `D-071` ha già `23.6` e `23.7` per standability e
transition. La numerazione delle epic è unica e si assegna **al merge** ([D-039](../../decisions/RT_PDR_00_Decision_Log.md)).

I tre blocchi nuovi **non** stanno in E23: sono grossi quanto lui. Vanno come **epic sorelle**, e la
raccomandazione è di non aprirle finché E23 non ha almeno la cottura, perché tutte e tre ci poggiano sopra.

## 7. Cosa resta aperto

| ID | Domanda |
|---|---|
| `GEO-1` | `WaterDepth` è un campo della cella accanto a `Surface`, o una **superficie composta**? La fonte dice asse separato; il repo ha otto superfici piatte, e la scelta decide se CP 8.1 si estende o si riscrive |
| `GEO-2` | Il modello a **slot** della §40 come si chiama, dato che `BreachCharge` è già l'oggetto che li apre? |
| `GEO-3` | Le §22–§27 (move end semantics, grafo causale, credit) si riconciliano con il TurnLog v6 appena atterrato — chi ha il codice in mano, non questo triage |

## 8. Nota di metodo — quinto sorgente, stesso rapporto

È il quinto handoff del 2026-08-10, e il rapporto regge: **le parti che descrivono il presente sono corrette
e già fatte; quelle che descrivono il futuro sono nuove e non specificate.**

Qui però c'è una variante che vale la pena registrare, perché è la più costosa: **la sezione marcata come
punto di ripresa era la più vecchia del documento**. Un `RESUME MARKER` dice *«riprendi da qui»* e invita a
costruire; sotto c'era codice spedito e un test verde. Il segnalibro non sapeva di essere in ritardo.

> ✅ **WIEGERS**: «Un marcatore di stato scritto dentro il documento è un'affermazione sul mondo come
> qualunque altra, e va verificata con la stessa disciplina. Questo diceva *ultima decisione* ed era la
> prima già chiusa.»
