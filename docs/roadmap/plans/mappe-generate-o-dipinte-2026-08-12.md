# Le mappe si generano o si dipingono?

> `CURRENT` · **Stato**: proposta di decisione, **non applicata**
> **Aggiornato il 2026-08-12**: la §3 argomentava su una premessa **scaduta** — la migrazione era data per
> futura ed era già passata. Vedi §3bis, che ne rovescia una parte.
> **Nata da**: la seduta U1 ([#451](https://github.com/DegrassiAaron/refactor-tactics-main/issues/451)) e dalla domanda «non puoi costruire tu delle mappe scenario?»
> **Tocca**: U1, U13, il senso degli artefatti `.uasset` di mappa

## 1. La tensione, che è già scritta nel repository

Due frasi del progetto si contraddicono, e finora nessuno le aveva messe una accanto all'altra.

`MakeTestArena`:

> «Esiste in codice e non come `.uasset` per una ragione precisa: `Content/**` è gitignorato, quindi una mappa
> dipinta a mano **non sopravvive a un clone** e non è riproducibile fra macchine. Questa sì.»

U1, passo 1:

> «Livello nuovo in `/Game/RT/Maps/Dev/L_HexArena/` … i due asset sono tracciati da git.»

La prima dice che le mappe stanno in codice perché quelle dipinte non sono riproducibili. La seconda chiede di
dipingerne una e committarla. Sono due filosofie, e la seduta U1 le ha fatte collidere.

## 2. Cosa è successo davvero, costruendo

Non sono argomenti teorici: sono i costi osservati in una sola seduta.

| Evidenza | Costo |
|---|---|
| l'allowlist va estesa a mano, e senza `git add` **tace** | [#449](https://github.com/DegrassiAaron/refactor-tactics-main/issues/449) — la seduta non si sarebbe chiusa senza capire perché |
| il lavoro dipinto **si è perso due volte** | un `Ctrl+Z` che ha annullato la generazione, e una rigenerazione che ha cancellato piattaforma e transizione |
| costruire a mano il layout dei criteri era **lavoro cieco** | tre errori consecutivi, nessuno visibile guardando la mappa ([#491](https://github.com/DegrassiAaron/refactor-tactics-main/issues/491)) |
| l'asset ha contenuto per un'ora una mappa **diversa** da quella creduta | `GeneratedTestArena` scambiata per la propria |
| i `.uasset` sono **binari** | conflitto non risolvibile a mano, l'ultimo che committa vince |

Contro un solo vantaggio della mappa dipinta: **esercita gli strumenti dell'editor**. Ma quel vantaggio è già
coperto altrove — le sette voci `PIE-HEX-MODE-*` verificano il *mode*, non chi ha creato la geometria.

## 3. L'argomento che teneva in vita l'asset

Uno solo, e sembrava non debole: **U13 e la migrazione di formato**.

E9 incrementa la versione del formato mappa, e U13 dichiara che `DA_HexMap_Arena` e `DA_HexMap_Sandbox`
vanno **migrati, non ricostruiti**. Ma `DA_HexMap_Sandbox` pesa **1396 byte** — è di fatto vuoto. Se nessun
asset mappa con contenuto reale esiste nel repository, la migrazione **non ha soggetto**, e la si verificherà
solo in memoria.

Il progetto ha già una regola contro questo, imparata sul campo: *scrivi l'asset col binario vecchio,
rileggilo col nuovo*. Senza un asset committato, quel test non è scrivibile.

## 3bis. Cosa è successo davvero — e perché quell'argomento va rovesciato

*(aggiunto il 2026-08-12)*

L'argomento sopra era **al futuro**: «E9 farà una migrazione, tenete l'asset». Nel frattempo la migrazione
è passata — **v6 → v7 con [#619](https://github.com/DegrassiAaron/refactor-tactics-main/issues/619)**, il
giorno dopo che questo documento è stato scritto — e nessuno l'ha verificata contro un asset serializzato.

Scrivendo quel test ([#688](https://github.com/DegrassiAaron/refactor-tactics-main/pull/688)) è emerso che
**la migrazione è inerte** ([#687](https://github.com/DegrassiAaron/refactor-tactics-main/issues/687)):
`FormatVersion` non finisce nei byte serializzati, perché la delta serialization di UE salta le property
uguali al default del CDO — e il default *è* `CurrentFormatVersion`. Un asset si ricarica sempre «già
aggiornato», e `MigrateToCurrentFormat` non fa nulla.

**Quindi l'argomento va letto al contrario di come era scritto.**

| Come suonava | Come sta |
|---|---|
| l'asset serve alla migrazione futura | la migrazione è già passata e **non è partita** |
| senza asset la migrazione non ha soggetto | con l'asset ha un soggetto, e ha rivelato che il meccanismo è rotto |
| è un argomento a favore del committato | resta **a favore**, ma per una ragione diversa e più forte |

La ragione più forte: l'asset committato è **l'unico modo in cui quel difetto poteva emergere**. Nessun test
in memoria l'avrebbe mai trovato — tutti impostano `FormatVersion` a mano, cosa che nella realtà non accade
mai. Un asset serializzato reale nel repository ha fatto da rivelatore.

Il che sposta la conclusione: l'asset non serve a *superare* una migrazione futura, serve a **tenere onesto
il meccanismo**. Ed è un argomento che vale a prescindere da chi genera la geometria.

## 4. Proposta: generato **e** committato, con un test che li tiene insieme

L'asset non sparisce e non si dipinge: **si genera e si committa**.

- **La fonte è il codice** (`MakeArenaV01`): riproducibile, verificabile, versionabile in testo, senza conflitti binari irrisolvibili
- **L'artefatto è committato**: esiste un `.uasset` serializzato reale, che E9 potrà migrare e U13 estendere
- **Un test lega i due**: l'asset committato deve corrispondere a ciò che il generatore produce

Il terzo punto è quello che rende la proposta diversa dall'avere due fonti di verità. Senza, l'asset e il
codice divergerebbero al primo che dipinge sopra — e nessuno se ne accorgerebbe.

L'oracolo esiste già: `URTHexMapAsset::ComputeHash()` è deterministico e indipendente dall'ordine di
inserimento. Un test che confronta l'hash dell'asset caricato con quello di `MakeArenaV01` cade **esattamente**
quando qualcuno modifica la mappa senza passare dal codice.

### Cosa cambia per chi lavora

| Prima | Dopo |
|---|---|
| dipingi, e se sbagli ricominci | modifichi il codice, e il layout è verificato da un test |
| il lavoro sta solo nel `.uasset` | il lavoro sta nel codice, l'asset è un artefatto |
| un `Ctrl+Z` può cancellare due ore | rigeneri con un pulsante |
| la mappa d'autore non è riproducibile | chiunque cloni ottiene la stessa mappa |

### Cosa si perde

**La possibilità di comporre a occhio.** Un layout organico, disegnato guardando, è più facile da fare col
pennello che da scrivere in coordinate. Per l'arena della v0.1 — 61 celle con vincoli misurabili — il costo è
basso. Per una mappa futura pensata per l'estetica, no.

Non è una decisione irreversibile: un asset generato si può sempre aprire e dipingere. Diventa d'autore nel
momento in cui il test che lo lega al codice viene ritirato **di proposito**, invece che rotto per sbaglio.

## 5. Conseguenze da accettare esplicitamente

- **U1 cambia natura**: i passi 2–7 non si eseguono più a mano. Restano le sette voci `PIE-HEX-MODE-*`, che sono ciò che verifica davvero il mode
- **U13 modifica il codice** invece dell'asset, e rigenera. La migrazione di formato resta verificabile perché l'asset committato esiste
- **`MakeArenaV01` va nel registry** `MakeFixtureArena`: oggi non c'è, quindi nessuno scenario può riferirla per nome — è un buco già presente

## 6. Domande ancora aperte

**Quante mappe committare?** Oggi l'allowlist ne ammette nove, ognuna aggiunta a mano. Se le mappe si
generano, committarne **una** — quella che fa da rivelatore per la serializzazione — potrebbe bastare, e le
altre restano codice.

**La proposta della §4 dipende da [#687](https://github.com/DegrassiAaron/refactor-tactics-main/issues/687)?**
No, ma ne è illuminata. Il test che lega asset e generatore via `ComputeHash` funziona comunque — `ComputeHash`
non passa dalla serializzazione delta. Ma finché `FormatVersion` non viaggia, quel test direbbe «l'asset
corrisponde al codice» **senza poter dire con quale versione di formato è stato scritto**. Le due cose vanno
decise insieme, o si costruisce un secondo controllo sopra un meccanismo inerte.
