# Le mappe si generano o si dipingono?

> `CURRENT` · **Stato**: proposta di decisione, **non applicata**
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

## 3. L'argomento che tiene in vita l'asset

Uno solo, e non è debole: **U13 e la migrazione di formato**.

E9 incrementa la versione del formato mappa, e U13 dichiara che `DA_HexMap_Arena` e `DA_HexMap_Sandbox`
vanno **migrati, non ricostruiti**. Ma `DA_HexMap_Sandbox` pesa **1396 byte** — è di fatto vuoto. Se nessun
asset mappa con contenuto reale esiste nel repository, la migrazione **non ha soggetto**, e la si verificherà
solo in memoria.

Il progetto ha già una regola contro questo, imparata sul campo: *scrivi l'asset col binario vecchio,
rileggilo col nuovo, confronta un digest dei campi vecchi*. Senza un asset committato, quel test non è
scrivibile.

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

## 6. Domanda ancora aperta

Quante mappe committare? Oggi l'allowlist ne ammette nove, ognuna aggiunta a mano. Se le mappe si generano,
committarne **una** — quella che serve alla migrazione — potrebbe bastare, e le altre restano codice.
