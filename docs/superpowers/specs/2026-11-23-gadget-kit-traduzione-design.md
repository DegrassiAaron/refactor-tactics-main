# Kit di Gadget: traduzione del catalogo del 2026-11-20 — design

> `RECORD` · **Da approvare** (sessione `/sc:brainstorm` del 2026-11-23, issue
> [#4170](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4170)).
> Questo file conserva il **perché** e la forma del lavoro. Le regole stanno nel
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md) e vincono su questo testo.

## 1. Intento

Il catalogo del 2026-11-20 è la base da cui ogni eroe sceglie e bilancia il proprio kit
([D-633](../../decisions/RT_PDR_00_Decision_Log.md)): *«servono come base questi kit per scegliere e bilanciare le
skill. crea una issue per eroe»*. Gadget è il primo eroe su cui si lavora: *«iniziamo creando/modificando le skill di
Gadget»*.

Il lavoro porta le voci `GAD_*` del catalogo
([`.md`](../../research/handoff/catalogo-14-eroi-input-2026-11-20.md) §3 «Gadget»,
[`.csv`](../../research/handoff/catalogo-14-eroi-input-2026-11-20.csv)) nelle regole del contratto, mette in fila ciò
che l'autore deve scegliere, e arriva alla `D-nnn` d'accettazione che la #4170 chiede.

## 2. Che cosa ha scelto l'autore in questa sessione

Domande a scelta; il testo delle opzioni scelte va riportato alla lettera nella `D-nnn`, controllato sulla trascrizione.

- **Che cosa produce il primo giro:** «Traduzione e scheda (Recommended)». Il testo dell'opzione diceva che ogni voce
  si traduce nel contratto con la provenienza di ogni numero, poi vengono la scheda `DESIGN_SPEC`, le domande e la
  `D-nnn` d'accettazione, e che non si scrive codice.
- **Le skill di oggi:** «Escono tutte e quattro (Recommended)». Escono `ArcPulse`, `ConductiveNode`, `Overload` e
  `ReactiveCapacitor`; il kit sono le voci del catalogo, `LinearDischarge` e l'overclock. Il testo dell'opzione diceva
  anche le due conseguenze: `Action.Electrify` resta un'azione del core senza un eroe che la usi, e dell'acqua resta solo
  il bonus di `LinearDischarge` contro Wet.
- **Come si traduce:** l'approccio A del §4.

## 3. Obiettivi, non obiettivi, anti-obiettivi

**Obiettivi**
- Un documento di traduzione, `docs/roadmap/plans/gadget-kit-traduzione-2026-11-23.md`, con:
  - la legenda delle provenienze e la tabella «come leggere la traduzione» della
    [traduzione di Riktor](../../roadmap/plans/riktor-kit-traduzione-2026-11-13.md), aggiornate a D-633 (nove
    elementi, Overwatch nel kit, copertura bassa fissa 60);
  - il kit in breve, una riga per voce;
  - una scheda per ciascuna voce del catalogo, per `LinearDischarge` e per l'overclock;
  - le statistiche base tradotte (HP, DEF, RES, movimento, gittata, vista);
  - ciò che esce, per nome;
  - i sistemi che mancano, ciascuno con il dominio che lo possiede;
  - le misure possibili oggi (danno per AP con la formula di D-589, colpi per abbattere gli eroi Core del catalogo);
  - le combo come candidate del catalogo delle coppie;
  - le domande per l'autore, numerate.
- In [`gadget.md`](../../characters/v0.1/gadget.md), una sezione `DESIGN_SPEC` con il kit proposto, separata dal kit in
  vigore, che resta descritto com'è fino alla `D-nnn`.
- Le domande portate all'autore a giri; le risposte diventano la `D-nnn` d'accettazione.
- La #4170 aggiornata e chiusa al merge.

**Non obiettivi**
- Codice, asset, scenari, i cataloghi di `docs/balance/`: nessun numero entra in vigore prima della `D-nnn`, e neanche
  con lei a runtime finché il codice non arriva.
- La dock e la struttura del kit (`KIT-1`, `KIT-2` di [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md)).
- Gli altri eroi e l'aggiornamento del catalogo delle coppie, che riceve solo candidate.

**Anti-obiettivi**
- Copiare il catalogo come regola.
- Presentare come deciso ciò che è una proposta o una lettura.
- Presentare una misura come bilanciamento fatto: il confronto con gli altri eroi vale quando i loro kit sono tradotti.

## 4. Approcci

| Approccio | Cosa comporta | Esito |
|---|---|---|
| **A. Documento di traduzione più scheda** | Una scheda per voce con la fonte di ogni valore; in `gadget.md` il kit in breve | ✅ **scelto dall'autore** — ogni numero è verificabile, e la `D-nnn` cita una fonte sola |
| B. Solo la scheda | Tutta la traduzione in `gadget.md` | ❌ scartato — la pagina descrive il kit in vigore, e i numeri proposti si leggerebbero come in vigore |
| C. Prima il kit in breve e le domande | Le schede per voce dopo le risposte | ❌ scartato — le domande nascono scrivendo il dettaglio, e servirebbe un secondo giro |

## 5. Vincoli

- **Le decisioni di D-633**: cinque fasi; nove elementi; overclock come tipo di skill che hanno tutti; spalle di oggi;
  copertura bassa fissa 60; nessuna progressione; Overwatch come reazione del kit; Tier 3 fuori; tutte le opzioni di ogni
  slot nel kit; nomi Paragon. Le letture di D-633 (11) restano letture, e la scheda lo dice dove le usa.
- **Le altre regole del contratto**: D-583 (fasi, una cella per occupante), D-589 e D-602 (danno, scala ×10), D-594 e
  D-613 (AP, Ultimate a 3), D-621 (catena di `LinearDischarge`).
- **Le regole del catalogo che D-633 non ha misurato** diventano domande (#4170, punto 4): l'elevazione contro la
  copertura bassa, i livelli Z, la «RES Energetica» unica contro la RES per elemento, l'Overclock «+2 AP» con
  *Phase Lock*.
- **Domande già aperte per Riktor** che valgono anche per Gadget (per esempio se il «Danno Energetico» sia elementale,
  o un danno che ignora l'armatura): una domanda sola, che cita quella di Riktor, perché la risposta vale per entrambi.
- **Ogni voce si traduce anche se il suo sistema manca.** Il sistema mancante si nomina, non si inventa.
- Il codice citato è letto su un commit dichiarato e **non eseguito**.
- Nessun totale che cambia da solo; ogni citazione dell'autore è controllata sulla trascrizione della sessione.
- Il numero della `D-nnn` vale al merge e va rimisurato prima.
- Gate dei documenti: `node tools/radar/{doc-links,doc-tables,doc-coherence,issue-refs,decision-ids,scenario-notes}.ts
  --check` e `node tools/radar/generate.ts --check`.

## 6. Scelte dell'assistente, ribaltabili

- **Un ramo solo** (`docs/4170-kit-gadget`): traduzione, revisione, domande all'autore, `D-nnn`, una PR. Se l'autore
  rinvia alcune risposte, la `D-nnn` accetta in parte, come [D-612](../../decisions/RT_PDR_00_Decision_Log.md).
- **Numeri di questo giro**: quelli del catalogo, con le misure come [PROPOSTA].
- **Id delle voci**: `Hero.Gadget.<nome del catalogo in PascalCase>` (`Hero.Gadget.SparkBolt`, …).
- **Le varianti `Concentrated` e `Branched` di `LinearDischarge` e l'equipaggiamento di default di Gadget**
  (`Gadget.Insulator`, `Reaction.ReactiveShield`): domande, perché nessuna risposta le ha toccate.
- **Overclock**: una [PROPOSTA] costruita sulla definizione dell'autore in D-633 (1), con l'Overclock generico del
  catalogo come alternativa nella domanda.

## 7. Lasciato a chi esegue

La traduzione voce per voce: fase del resolver, priorità, portata e forma in celle (le aree «3x3» e gli spicchi del
catalogo), bersaglio, elemento, tipo di danno, stati, oggetti persistenti, sistemi che mancano, ognuno con la sua fonte.
