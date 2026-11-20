# Kit degli eroi dal catalogo del 2026-11-20 — design

> `RECORD` · **Approvato dall'autore il 2026-11-20** (procedura e modello di issue, sessione `/sc:brainstorm`).
> Questo file conserva il **perché** e la forma del lavoro. Le regole stanno nella voce del
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md) che nasce con lo stesso ramo, e vincono su questo testo.

## 1. Intento

L'autore ha consegnato un catalogo prototipo dei kit degli eroi in due file: una descrizione per eroe in Markdown e la
stessa materia in tabella CSV, sei slot per eroe e tre opzioni per slot. La richiesta, alla lettera:
*«dobbiamo integrarle e modificare le skill attuali usando queste. Nella lista manca LinearDischarge di Gadget, da
mantenere.»* Lo scopo, alla domanda su cosa servisse a lavoro finito: *«servono come base questi kit per scegliere e
bilanciare le skill. crea una issue per eroe»*.

Il catalogo è quindi un **input di design**: la base da cui ogni eroe sceglie e bilancia il proprio kit, non un kit già
in vigore. I due file sono conservati come `RESEARCH` in
[`catalogo-14-eroi-input-2026-11-20.md`](../../research/handoff/catalogo-14-eroi-input-2026-11-20.md) e
[`catalogo-14-eroi-input-2026-11-20.csv`](../../research/handoff/catalogo-14-eroi-input-2026-11-20.csv); le regole che li
governano sono nella voce [D-633](../../decisions/RT_PDR_00_Decision_Log.md).

## 2. Obiettivi, non obiettivi, anti-obiettivi

**Obiettivi**
- Le scelte di regola prese nella sessione, registrate in una voce del Decision Log con il testo che l'autore ha visto.
- I due file d'input conservati come `RESEARCH` in `docs/research/handoff/`, senza modifiche al contenuto.
- Una issue per eroe del roster del catalogo, che parte da quella voce e porta il kit fino a una `D-nnn` d'accettazione
  dell'autore, come [D-612](../../decisions/RT_PDR_00_Decision_Log.md) (1) prevede per Riktor.
- [#4014](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4014) chiusa come superata.

**Non obiettivi**
- Codice, asset, scenari, golden, radar: nulla cambia a runtime.
- Nessun numero di bilanciamento entra in vigore: i numeri del catalogo restano input finché la `D-nnn`
  d'accettazione dell'eroe non li accetta.
- La struttura del kit e la scelta di cosa va nella dock (`KIT-1`, `KIT-2` di
  [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md)) restano aperte.

**Anti-obiettivi**
- Copiare il catalogo come regola: un kit copiato senza traduzione è `IMPLEMENTATION DRIFT`.
- Riscrivere le regole nel corpo di ogni issue: diventerebbero copie che divergono. Le issue rimandano alla voce.

## 3. Approcci

| Approccio | Cosa comporta | Esito |
|---|---|---|
| **Prima la voce, poi le issue** | Una PR con la `D-nnn` delle scelte, i due file d'input e questo design; poi una issue per eroe che cita la voce | ✅ **scelto dall'autore** — le regole stanno in un posto solo |
| Solo le issue, subito | Le issue ora, ognuna con le regole scritte nel corpo; la voce dopo | ❌ scartato — più veloce, ma le regole vivono in tante copie finché la voce non esiste |

## 4. Che cosa è deciso

Le risposte dell'autore, raccolte in quattro giri. Il testo delle domande e delle opzioni sta nella voce del Decision Log.

- **Fasi**: restano cinque (Prep, Dash, Blast, Move, Cleanup).
- **Elementi**: i nove del catalogo, «per il momento».
- **Overclock**: un tipo di skill che hanno tutti gli eroi: più AP o altri vantaggi per un round, con un costo nel round
  successivo.
- **Nomi**: quelli Paragon usati nel catalogo.
- **Bonus alle spalle**: resta quello di oggi ([D-587](../../decisions/RT_PDR_00_Decision_Log.md),
  [D-599](../../decisions/RT_PDR_00_Decision_Log.md)).
- **Progressione**: nessuna ([D-598](../../decisions/RT_PDR_00_Decision_Log.md) resta).
- **Rianimazione**: si riapre [D-596](../../decisions/RT_PDR_00_Decision_Log.md).
- **Riktor**: vale la versione del Markdown, non quella del CSV.
- **Overwatch**: reazione dell'eroe, come nel catalogo.
- **Copertura bassa**: resta la sottrazione fissa di 60.
- **Tier 3**: resta fuori, per ora.
- **Le tre opzioni di ogni slot**: tutte nel kit, come per Riktor.
- **Procedura**: prima la voce, poi le issue; #4014 si chiude come superata.

## 5. Scelte dell'assistente, ribaltabili

Proposte mostrate all'autore come correggibili e non contestate. Non sono decisioni.

- Il roster è quello del catalogo: gli otto eroi «Core» già pianificati più Phase, Aurora, Sevarog, Yin, Crunch e Drongo.
- Con i nove elementi gli identificatori restano quelli di [D-593](../../decisions/RT_PDR_00_Decision_Log.md): «Flux»
  è `Element.Warp`, «Scudo» è `Element.Aegis`, «Freddo» è `Element.Frost`; escono `Element.Decay`, `Element.Life` e
  `Element.Arcane`.
- Bersagli e rotazioni a spicchi da 30° si traducono in celle e in passi da 60°, come nella
  [traduzione di Riktor](../../roadmap/plans/riktor-kit-traduzione-2026-11-13.md).
- Le reazioni Guard, Counter ed Evasion del catalogo si traducono sulle semantiche che esistono già (`Intercept`,
  `Deflect`, `Counter`, `Evade`).
- Le tabelle delle combo nelle schede diventano candidate per il catalogo delle coppie, mai regole dell'eroe.
- I numeri del catalogo (HP, movimento, gittata, danni, AP, cooldown) sono la base del bilanciamento e sostituiscono
  quelli di oggi solo con la `D-nnn` d'accettazione.
- `Hero.Gadget.LinearDischarge` entra come quarta opzione dell'Abilità Speciale di Gadget, con i numeri e la catena di
  oggi ([D-621](../../decisions/RT_PDR_00_Decision_Log.md)).
- Milestone: gli eroi «Core» collegati a E35 ([#322](https://github.com/DegrassiAaron/refactor-tactics-main/issues/322))
  con la milestone del loro gruppo (v0.1 per Gadget, Riktor, Muriel, Kallari, Steel e Murdock; v0.2 per Feng Mao e
  The Fey); gli altri senza milestone, con l'etichetta `post-v0.1`.

## 6. Lasciato a chi esegue ogni issue

Per il proprio eroe: tradurre le voci del catalogo nelle regole del contratto, progettare la skill di overclock, mettere
in fila le domande per l'autore, e arrivare alla `D-nnn` d'accettazione che sceglie e bilancia. Il modello è la
[traduzione di Riktor](../../roadmap/plans/riktor-kit-traduzione-2026-11-13.md): ogni valore con la sua fonte, e ciò che
si scarta detto per nome.
