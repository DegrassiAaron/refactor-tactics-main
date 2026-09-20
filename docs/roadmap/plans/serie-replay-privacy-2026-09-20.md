# Serie replay e privacy — referto della passata del 2026-09-20

> `SNAPSHOT` · **Misurato su**: `main` `5958dfbc` · **Data**: 2026-09-20
> **Cosa è**: il referto di una passata su sette issue — #1805, #2792, #759, #1165, #2325, #2606, #1166 —
> con ciò che è stato misurato, ciò che è stato corretto e ciò che resta a un autore.
> **Cosa non è**: un owner. Ogni voce qui ha un owner altrove, e questo documento lo nomina invece di
> sostituirlo. Quando questo file e la sua issue non concordano, **vince la issue**.
>
> 📐 **Convenzione**: i conteggi che cambiano da solo non sono scritti ([`AGENTS.md`](../../../AGENTS.md) §14).
> Dove un numero compare, o è l'esito del passaggio qui descritto, o è il difetto stesso — e porta il
> comando che lo produce.

## Il risultato in una riga

Nessuna delle sette issue era pronta per essere lavorata **come scritta**: in cinque casi su sette la
misura che le sosteneva era scaduta, e in tre casi la issue affermava qualcosa che il repository
smentisce. Il lavoro sostanziale è stato **rimettere in piedi le evidenze** e misurare un canale di
privacy che nessuno aveva ancora posto.

---

## 🔴 Il risultato che conta: #1805 ha ereditato una domanda e non lo sapeva

L'ultimo commento di #1805 (2026-09-09) diceva: *«quando `BLIND-1` esce (b) o (c), questa issue eredita
una domanda concreta invece di scoprirla a valle»*. `BLIND-1` è uscita **(c)** il giorno dopo con
[`D-371`](../../decisions/RT_PDR_00_Decision_Log.md), e nessuno dei due lati l'ha registrato — `D-371`
non nomina `1805`, `D-276`, `D-316` né `replay`.

La domanda è ora **misurata**, non prevista. I due confini che #1805 ha costruito si dividono il lavoro
per riga e per colonna:

| Confine | Domanda che pone | Cosa la porta |
|---|---|---|
| `FilterEntriesForObserver` ([D-316]) | *«posso vedere questo SOGGETTO?»* | il verdetto, congelato su **una sola** unità |
| `ToPublicTrace` ([D-276]) | *«questa COLONNA è pubblica?»* | la tabella di classificazione |

Nessuno dei due chiede **di chi sia la cella** che una voce nomina, e su una voce che nomina due unità
le due domande divergono. `Replay.Privacy.PublicCellsLeakAThirdPartyPosition` lo misura su due mondi con
la stessa conoscenza autorizzata, su **entrambe** le uscite del prodotto pubblico.

Il produttore esiste in produzione — `ERTFacingOutcome::RearHitBypassedCover` scrive `SrcCell` = cella
dell'attaccante e passa la **vittima** come soggetto. E il repository conosce già la classe: la chiude
per **un** produttore, accettando l'eccezione *«sui rari bypass»* in un docstring, senza voce `D-`.

⚠️ **Adiacenza da tenere d'occhio**: [#649](https://github.com/DegrassiAaron/refactor-tactics-main/issues/649)
guarda la **stessa voce** dal lato opposto — lì *«la traccia non basta a verificare il danno»*, qui *«la
traccia dice troppo sulla posizione»*. Write-set disgiunti, misurato; ma chi tocca uno dei due produttori
tocca il terreno dell'altro.

---

## Cosa è stato corretto, per issue

| Issue | PR | Cosa non reggeva |
|---|---|---|
| **#1805** | #3232 | sei affermazioni del corpo, fra cui *«Nessun test ri-simula dall'archivio»* (falso dal 2026-09-04) e un trailer che contraddiceva le caselle del corpo che lo contiene |
| **#2792** | #3233 | `D-249` non sapeva di essere emendata; la sonda dichiarava un invariante *«che nessuna decisione ha adottato»* cinque ore dopo che `D-371` l'aveva adottato; due inneschi di rosso resi irraggiungibili da `D-373` |
| **#759** | #3234 | la pausa che §7-bis vieta **esiste già** ed è prescritta da `D-350`/`D-355`/#166; il livello DTO era accreditato al test più debole dei due |
| **#1165** | #3235 | una fase della issue **annullata** da un commit di cleanup: quattordici pannelli riaggiunti con gli stessi blob, e il README che lo vieta ancora in piedi |
| **#2325** | #3236 | il corpo divergeva dal proprio documento su due ancore; la nota che certificava la rimisura sbagliava enumerazione, data e conteggio |
| **#1166** | — | il **titolo** diceva il contrario di ciò che il repository dichiara già in due posti |
| **#2606** | — | nulla di falso: **incompleto**. Il corpo non diceva di essere bloccato |

### Tre commenti del codice che affermavano il falso

Corretti con la nota che li data, non rimossi — la storia di come si è deciso vale più della sua
riscrittura:

- `RTReplayProducerTests.cpp` — *«Nessuno era quel chiamante»*, mentre
  `Replay.Verifier.ArchiveReplaysThroughTheResolver` esiste dal 2026-09-04 (#2196). 🔑 **Il corpo di
  #1805 citava quella frase alla lettera come lavoro mancante**: è il difetto che mandava qualcuno a
  riaprire lavoro finito;
- `RTServerOnlyGuardTests.cpp` — *«zero `UPROPERTY(Replicated)` in tutto `Source/`»*, mentre la fixture
  che il file stesso include ne porta due: sono il suo oracolo;
- `RTPlaybackControlsTests.cpp` — *«`DOREPLIFETIME` → zero»*, oggi non più vero alla lettera. La forma
  che regge è *«di produzione»*, col comando accanto.

---

## ⛔ Ciò che resta a un autore

Due voci nuove in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), e quattro che la passata ha confermato
aperte.

| Voce | Domanda | Perché non si deduce |
|---|---|---|
| **`REPCELL-1`** *(nuova)* | una voce che passa il filtro può nominare, in un campo `Public`, la cella di un'unità diversa dal proprio soggetto di verdetto? | tre uscite, tre costi diversi. ⛔ Marcare i campi `AuditOnly` **non** è fra le uscite: svuoterebbe il prodotto pubblico |
| **`BEAT-1`** *(nuova)* | quale meccanismo tiene il ritmo osservato indipendente dal tempo di risposta altrui, e con quale tolleranza? | §7-bis elenca tre alternative e non ne sceglie nessuna; la tolleranza non è un numero da nessuna parte; e il canale da neutralizzare è **già prescritto** |
| `BLIND-2` · `BLIND-3` | dove vive il filtro · un'entità mai percepita è selezionabile | catena di blocco misurata: `OBS-1 → BLIND-2 → #2792 → #2793` |
| `CHAT-1` | quali chat compongono il perimetro di #2606 | nessuna fonte accessibile porta la lista: portano le **tracce**, e dedurne il perimetro dà copertura `100 %` per costruzione |
| `ROSTER-1` | rigenerare o rimuovere le immagini col roster legacy nei pixel | il precedente d'autore su #853 esiste (*rimuovere*) ma *«non si estende da solo»* |
| #1165 | se ritogliere i pannelli di `systems-map` | l'identità dei byte è un fatto; cosa `research/` conserva è una scelta di contenuto, già presa e già ribaltata una volta |

---

## Ciò che è stato deliberatamente **non** fatto

- ⛔ **nessuna delle sette issue è stata chiusa**, e nessuna PR porta `Closes`. Una prima stesura ne
  portava due: le avrebbe chiuse in silenzio al merge, ed è stato corretto prima della pubblicazione;
- ⛔ **il canary `HexBotPlay.HiddenEnemyFairness` non è stato toccato**: è di #2793, che lo porta già in
  DoD, e `D-371` glielo assegna per nome. 🔴 Ma il principio ritirato ha una **terza sede** che nessuna
  DoD nomina — `Scenarios/Spec/Bot/HiddenEnemyFairness.json`, che `capability-map.md` elenca fra gli
  scenari-gate. Segnalata nel commento della sonda;
- ⛔ **`PlanningView` come tipo e l'owner documentale del Blind Actions Contract** non sono stati scritti:
  #2792 dichiara *«qui si decide, non si implementa»*, e un contratto scritto mentre `BLIND-2` e
  `BLIND-3` sono aperte sarebbe obsoleto alla prima risposta. ℹ️ Un substrato reflectable esiste già
  (`FRTPlanPreview`) e va riusato invece che duplicato — ma attraversa `Combat/`;
- ⛔ **l'antideriva di #2325**: nessun gate confronta lo stato GitHub delle issue con ciò che un documento
  ne dichiara, e `issue-refs.ts` va nel verso opposto. È uno strumento nuovo, con una politica da
  decidere per `NOT_PLANNED` e per l'assenza di rete;
- ⛔ **le righe d'indice dei sorgenti archiviati di #2606**: il referto del secondo giro dichiara la
  regola che le trattiene — *«quel giudizio appartiene a chi l'ha revisionato»*. Sono altrettanti giudizi
  di merito, non una coda eseguibile;
- ⛔ **il totale `30` di #1166 non è stato corretto in `28`**: è giusto, e cambiarlo avrebbe introdotto un
  errore che oggi non c'è.

---

## Due punti ciechi dei gate, misurati e senza owner

1. **`issue-refs.ts` non vede i percorsi spostati.** Costruisce l'insieme dei percorsi morti da
   `git log --diff-filter=D`, che **esclude i rename**: su `docs/src/wiki/v0.1/roster-legacy/`, `-D`
   conta `0` e `-R` conta `48`. Conseguenza misurata: #1166 cita un percorso inesistente e il gate resta
   verde.
2. **Due delle tre rotte di `RTServerOnlyGuard` girano senza oracolo positivo.** `OwnMember` e
   `RpcParameter` non compaiono in nessun test; solo `ReplicatedProperty` ha il proprio leak piantato.
   È letteralmente l'argomento con cui la fixture giustifica la propria esistenza, applicato a due terzi
   della guardia. ⚠️ Fuori dal perimetro dichiarato di #1805 (`D005`: *«⛔ Niente sotto `Turn/`»*), e
   l'owner #589 è chiuso: serve una issue propria.

---

## Gate

| Gate | Esito |
|---|---|
| Compile | **PASS** — `Build.bat RefactorTacticsEditor Win64 Development` → `Result: Succeeded`, exit 0, due volte |
| Automation — `Replay`+`Privacy`+`TurnLog` | **PASS** — `Success=163 Fail=0` |
| Automation — `BlindActions`+`Knowledge`+`Veil`+`BlindFire` | **PASS** — `Success=88 Fail=0` |
| Gate docs | **PASS** — `doc-links.ts --check` e `doc-tables.ts --check`, exit 0 |
| Determinism · Replay · Privacy | **N/A** sul comportamento: nessuna semantica cambia. La privacy è **misurata** e il canale resta aperto per scelta |
| PIE · Packaged | **NOT RUN** — fuori mandato dichiarato |

**Validità delle misure.** `HEAD` e hash del contenuto modificato identici prima e dopo ogni run
(`git diff HEAD` hashato, non `git status --porcelain`).
⚠️ **Una finestra dichiarata sporca**: la run `Replay`+`Privacy`+`TurnLog` è andata da `15:28:50` a
`15:29:29`, e `refactor-tactics-designer` è entrato alle `15:29:07` su un clone distinto — `Binaries/` è
per clone e l'Engine è una *installed build*. È una misura di **esito**, e per
[`AGENTS.md`](../../../AGENTS.md) §9 uno zero di rossi in finestra contesa è un risultato *più* forte,
non più debole. Si dichiara invece di tacerlo.

⚠️ `issue-refs.ts --check` esce `1`, su #1941 e #1993 — fuori da questa serie, e il gate **esenta
esplicitamente** #1165.

---

## Metodo, e perché è dichiarato

Ogni misura di questa passata è stata prodotta due volte: una volta per misurare, una volta per provare a
**falsificarla**. Il contraddittorio ha intercettato errori che sarebbero stati pubblicati, e due meritano
di essere nominati perché sono la ragione per cui vale la pena:

- una correzione a `systems-map/README.md` attribuiva la misura `ImageChops` a *«uno strumento che stava
  in `scripts/`»*. **Nessuno script con quel codice è mai stato committato**: il confronto fu ad hoc, e
  ciò che è uscito con `D-182` era il *gate*, non lo strumento della misura. Una correzione che nasce per
  togliere un riferimento morto non può inventarne la provenienza;
- una lettura di #1166 concludeva *«24 + 4 = 28, quindi la issue sbaglia»*. Il perimetro è `24 + 4 + 2`,
  l'autore l'ha enumerato, e i due file del terzo gruppo esistono. Correggere avrebbe **introdotto**
  l'errore.

🔑 Entrambi hanno la stessa forma: una correzione più sicura di sé del proprio fondamento. È il difetto
che questa serie di issue documenta da mesi, commesso mentre lo si correggeva.
