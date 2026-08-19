# CP 46.2 — Loading e error modal · spec panel

> `CURRENT` · **Stato**: revisione chiusa, **DoD esteso** dalle due decisioni del §8 · **Data**: 2026-08-16
> **HEAD della revisione**: `385ae694` (subito dopo il merge di CP 46.1, `#972`)
> **Oggetto**: il DoD di [`#937`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/937),
> owner [`spec-frontend-navigazione.md`](../../technical/architecture/spec-frontend-navigazione.md) §4.
> **Scopo**: verificare che ciò che il checkpoint chiede di *mostrare* abbia un **produttore**, prima che
> qualcuno lo implementi. È la lezione che questo repository paga più spesso — un dato dichiarato,
> trasportato e mai prodotto.

---

## 1. Il verdetto in una riga

Il DoD è scritto bene e chiede la cosa sbagliata: descrive **un fallimento netto** (`SCENARIO COULD NOT
START`) mentre il percorso d'avvio reale non fallisce quasi mai — **ripiega**, e lo fa **21 volte in
warning contro 8 errori** nel solo `RTGameMode.cpp`. Un modale d'errore costruito su questo DoD sarebbe
corretto, testabile e **non comparirebbe mai** nei casi che oggi fanno perdere tempo all'autore.

E il precedente che lo dimostra è già nel codice, con la motivazione scritta accanto:
`ARTGameMode::GetScenarioBannerText()`.

---

## 2. Il conto

| | Voci | Significato |
|---|---:|---|
| `BLOCKED` | **2** | il DoD chiede di mostrare un dato che **nessuno produce** |
| `CONFLICT` | **1** | contraddice un comportamento reale del codice |
| `PROPOSED` | **2** | gap reale che il DoD non copre, con un precedente interno |
| `CURRENT` | **3** | riporta correttamente una regola già vigente |

---

## 3. `BLOCKED` — i due produttori che non esistono

### 3.1 Le tre fasi di loading non sono uno stato osservabile

Il DoD nomina `Loading map…`, `Initializing scenario…`, `Preparing bots…` come *«messaggi di fase
reale»*. Misurato:

```sh
grep -rn "Loading map\|Initializing scenario\|Preparing bots\|LoadingPhase" Source/
# → 0
```

**Zero occorrenze.** Non esistono né le stringhe né un enum di fase né un evento che le distingua: la
mappa si costruisce dentro `BeginPlay`, e fra l'inizio e la fine non c'è nulla che un widget possa
leggere. Un `WBP_RT_LoadingScreen` scritto oggi mostrerebbe **una stringa costante** scelta a mano.

⚠️ **È lo stesso difetto che il DoD evita con la percentuale, un gradino più su.** La riga *«nessuna
percentuale, perché non esiste un progress model»* è corretta e ben argomentata — ma i **tre messaggi di
fase** sono lo stesso dato inventato in forma discreta: senza produttore, sono tre percentuali con un nome.

**Non è un motivo per rinviare CP 46.2.** È un motivo per dichiarare cosa il checkpoint consegna: o **una**
schermata di attesa senza fasi (onesta, e sufficiente al ciclo di CP 46.4), oppure prima un produttore —
tre `UE_LOG` esistono già nei punti giusti e diventerebbero un evento a costo quasi zero.

### 3.2 «Causa leggibile» non ha un vocabolario, e il repository ne ha già sette

Il DoD chiede che l'errore porti *«un motivo, non un codice muto»*, citando la regola di CP 11.8. Giusto —
ma non dice **da dove** viene il motivo, e il repository ha già **sette** enum di reason:

```
ERTHexTargetReason · ERTActionInvalidReason · ERTHexWaypointReason
ERTDisplacementBlockReason · ERTMatchEndReason · ERTMoveOutcome · ERTNavResult
```

Nessuno dei sette copre l'avvio di una partita, e `ERTNavResult` (CP 46.1) copre la **navigazione**, non
il fallimento del gioco sotto. ∴ CP 46.2 **aprirà un ottavo vocabolario**, ed è una decisione da prendere
esplicitamente: quali sono gli esiti d'avvio ammessi, e chi li produce.

⚠️ Il rischio concreto, e ha un precedente in questo stesso repository: se il motivo nasce come `FString`
composta nel widget, la UI diventa la sorgente della spiegazione — cioè una seconda autorità su *perché*
qualcosa non è partito, che può divergere dal log.

---

## 4. `CONFLICT` — il gioco non fallisce, ripiega

Il DoD immagina un mondo binario: o parte, o compare `SCENARIO COULD NOT START`. Il codice non si comporta
così. Misurato in `RTGameMode.cpp`: **21 `UE_LOG(Warning)` contro 8 `UE_LOG(Error)`**, e i due casi più
importanti dell'avvio sono entrambi **warning con ripiego**:

| Caso | Cosa fa oggi | Cosa vede il giocatore |
|---|---|---|
| Nessuna mappa d'autore | `MapSource=GeneratedTestArena: uso la mappa di PROVA generata` | una partita **normale**, su un'arena che non è un livello di gioco |
| Nessun `MatchFormat` | `MakeFallbackRules()` — *«uso il RIPIEGO … le misure di playtest vanno attribuite al formato giusto»* | una partita **normale**, con regole che nessuno ha dichiarato |

🔴 **Questa riga diceva «sono esattamente le due riserve di `G13`», e l'implementazione l'ha falsificata**
(2026-08-16, test rosso). La riserva di `G13` è: *«la partita gira su `MapSource=GeneratedTestArena` … **e**
la via a punti non è mai stata esercitata, perché la soglia obiettivo è 0»*. La seconda è un **valore** del
formato in vigore, **non** il formato di ripiego — che è per giunta un ramo *raro*, perché
`Format.Skirmish2v2` è spedito da C++ (`9f44570d`) e copre il caso normale.

∴ dei due ripieghi qui sopra, **solo il primo è una riserva di `G13`**. Il secondo resta un difetto reale e
vale la pena mostrarlo, ma non è quello che tiene il gate 🟡. Un modale d'errore non intercetta né l'uno né
l'altro, perché non sono errori — e quella parte regge.

> 🔑 **Il precedente decisivo è già scritto, e risolve lo stesso problema.**
> `ARTGameMode::GetScenarioBannerText()` esiste perché *«il sintomo non punta alla causa … la spiegazione
> c'è, ma è in una riga di Output Log che non si ha motivo di andare a cercare»*. È nato il 2026-08-08 da
> un caso reale (`PIE-SCEN-KEEP`): schermo quasi vuoto, causa nel log, nessuno che la trovi.
>
> La forma della soluzione era **una riga a schermo che dichiara lo stato anomalo**, non un modale. Ed è
> la forma che serve anche qui.

---

## 5. `PROPOSED` — ciò che il DoD non copre e vale più di ciò che copre

### 5.1 Un *banner di ripiego*, accanto al modale

Il modale resta giusto per il fallimento vero (mappa corrotta, scenario inesistente). Ma il caso
**frequente** è l'avvio riuscito **in condizioni degradate**, e per quello serve la forma di
`GetScenarioBannerText`: una riga persistente, leggibile, che dice *cosa* sta girando davvero.

Costo basso e beneficio misurabile: renderebbe visibili in PIE **entrambe** le riserve di `G13` senza
aspettare che qualcuno legga l'Output Log.

### 5.2 `BACK` da un errore d'avvio: verso *dove*?

Il DoD dice *«`BACK` riporta alla schermata precedente con lo stack intatto»*. Con CP 46.1 in `main` la
domanda ha una risposta precisa e va scritta: l'errore d'avvio arriva **dopo** `PushScreen(Play)`, quindi
la schermata precedente è il Main Menu — ma il gioco potrebbe essere già a metà transizione.

⚠️ E c'è un caso che il DoD non nomina: se l'errore compare **mentre la partita è già avviata**, `BACK`
deve fare `ReturnMain()` (che smonta) e **non** `PopScreen()` (che lascerebbe una partita viva sotto il
menu). I due esiti esistono già in `ERTNavResult`: la scelta è di dominio, non di implementazione.

---

## 6. `CURRENT` — tre regole già vigenti, riportate correttamente

- *«Nessuna percentuale»* — coerente con «la UI non ricalcola il risultato», ed è la parte meglio
  argomentata del DoD.
- *«`DETAILS` solo in Development»* — il progetto usa già `#if !UE_BUILD_SHIPPING` in **12 file**, e
  `RTGameMode.cpp` documenta il caso in cui quella guardia ha morso davvero (`-dpcvars` compilato fuori,
  `#926`). Il precedente esiste e va citato, non reinventato.
- *«Un rifiuto senza motivo è un difetto»* — è la regola di CP 11.8, e CP 46.1 l'ha già applicata a
  `ERTNavResult`.

---

## 7. Cosa questa revisione **non** decide

- **Se aggiungere un produttore di fasi di loading.** È lavoro che il DoD non prevede e che nessun gate
  richiede: va deciso, non dedotto da questo referto.
- **Se il banner di ripiego (§5.1) sia di CP 46.2 o un checkpoint proprio.** Ha un precedente interno e
  costo basso, ma allarga il DoD.
- **L'ottavo enum di reason.** Va istruito con l'elenco chiuso degli esiti d'avvio, come `ERTNavResult`.

---

## 8. Le tre domande, e come sono state chiuse

Decise dall'autore in sessione il 2026-08-16. **Le prime due allargano CP 46.2**, ed è la scelta più cara
delle alternative offerte: va detto, perché il checkpoint smette di essere «due schermate».

### 8.1 Le fasi di loading → **si costruisce il produttore**

> *«Il loading consegna tre fasi o una schermata di attesa senza fasi?»* → **tre fasi, con l'evento che le
> produce.**

`ERTLoadPhase { Map, Scenario, Bots }` più la sua emissione nei tre punti di `BeginPlay` dove oggi ci sono
già tre `UE_LOG`. Il widget **legge un evento**, non compone una stringa.

⚠️ **Conseguenza dichiarata**: CP 46.2 tocca `RTGameMode`, cioè **codice d'avvio e non solo UI**. Non è
neutro rispetto a D-139 — `RTGameMode.cpp/.h` non è nel `writable` della track `frontend_shell` e non è di
nessun'altra: va **assegnato prima di scrivere**, non durante.

✅ Ciò che questa scelta compra: la fase diventa un **dato** invece di un'etichetta, quindi il DoD è
verificabile (`l'evento è emesso tre volte, nell'ordine dichiarato`) e non solo osservabile a occhio.

### 8.2 Il ripiego silenzioso → **banner dentro CP 46.2**

> *«Il ripiego entra come banner o resta un difetto registrato?»* → **banner, in questo checkpoint.**

Due forme per due casi che il DoD confondeva in uno:

| Forma | Caso | Esempio |
|---|---|---|
| **Modale** | fallimento vero: non si parte | scenario inesistente, mappa corrotta |
| **Banner** | avvio riuscito, condizioni degradate | arena di PROVA · formato di RIPIEGO |

Il banner riusa la forma di `GetScenarioBannerText`, che esiste già per questo problema — e ne eredita la
motivazione: *«il sintomo non punta alla causa»*.

✅ Effetto misurabile: **la prima riserva di `G13`** — l'arena di test — diventa visibile in PIE senza
aprire l'Output Log, insieme a ogni altra condizione degradata. Non la chiude — resta una mancanza di dati
— ma smette di essere invisibile, che è il motivo per cui il 2026-08-10 nessuno se n'era accorto guardando
lo schermo.

*(Questa riga diceva «le due riserve». Corretta il 2026-08-16 dall'implementazione: la seconda riserva è
la soglia obiettivo a 0, non il formato di ripiego — vedi §4.)*

### 8.3 `BACK` da un errore a partita già avviata → **`ReturnMain`**

Non è stata portata all'autore perché la regola esiste già e decide: CP 46.6 chiede che il ritorno al menu
**non lasci stato vivo**, e `PopScreen` lascerebbe una partita viva sotto il menu. Quindi:

- errore **prima** dell'avvio (durante il loading) → `PopScreen`, si torna al Main Menu, nulla è stato
  costruito;
- errore **a partita avviata** → `ReturnMain`, che smonta.

Entrambi gli esiti esistono già in `ERTNavResult`: nessun vocabolario nuovo.

---

## 9. Cosa cambia nel DoD di `#937`

| | Prima | Dopo |
|---|---|---|
| Loading | tre messaggi, nessun produttore | `ERTLoadPhase` + emissione, e il widget legge |
| Errore | un modale per ogni caso | **modale** (fallimento) + **banner** (ripiego) |
| `BACK` | «alla schermata precedente» | `PopScreen` prima dell'avvio · `ReturnMain` a partita viva |
| Write-set | `Frontend/` | ➕ `RTGameMode` — **da assegnare** |

⚠️ **Il checkpoint è cresciuto, e nessun gate della v0.1 lo richiedeva.** È la stessa natura di E46, già
dichiarata in [D-144](../../decisions/RT_PDR_00_Decision_Log.md): scelta di prodotto, non esecuzione di un
gate. Chi lo riprende deve saperlo prima di stimarlo.
