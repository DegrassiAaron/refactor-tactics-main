# Sessioni parallele e ID condivisi — meccanismo

> **Owner del meccanismo.** Le *regole* stanno in [`../../AGENTS.md`](../../AGENTS.md) §Git; qui c'è come
> funziona e cosa fare quando si rompe. Decisioni:
> [D-135](../decisions/RT_PDR_00_Decision_Log.md) (worktree e ID condivisi) e
> [D-139](../decisions/RT_PDR_00_Decision_Log.md) (write-set di batch e lease binaria).
>
> Le §1–§10 rispondono a *«due sessioni possono scegliere lo stesso numero?»*. Le §11–§15 a
> *«due sessioni possono scrivere lo stesso file?»*, che è un problema diverso e non lo risolve
> nessun allocatore.

## 1. Perché lo stesso worktree non è sicuro

Due sessioni Claude nella stessa working directory non sono due sessioni: sono due processi che si
scrivono addosso. Possono editare lo stesso file nello stesso momento, vedere le modifiche non committate
dell'altra, cambiare branch sotto i piedi dell'altra, e produrre un `git status` che non appartiene a
nessuna delle due task. Un allocatore di ID non risolve niente di tutto questo — risolve **solo** la
collisione di numerazione.

Per il resto vale l'isolamento: **una sessione esecutiva = un worktree + un branch**.

## 2. Un worktree per sessione

```powershell
git fetch origin
git worktree add ..\rt-wt-621 -b feat/621-geometry-bake origin/main
cd ..\rt-wt-621
claude
```

Secondo terminale, stesso clone:

```powershell
git worktree add ..\rt-wt-712 -b feat/712-editor-tool origin/main
cd ..\rt-wt-712
claude
```

Il repository Git è **uno solo**: i worktree condividono oggetti, ref e — cosa che qui conta — la
directory di stato dell'allocatore. Non serve un secondo clone, e un secondo clone anzi *toglie* la
garanzia (§8).

La working directory principale resta il punto di integrazione: si tiene libera, non ci si lavora in
parallelo.

## 3. Prenotare un ID

```powershell
python scripts/rt_shared_id.py reserve D --reason "#621 geometry bake provenance"
```

Stampa una riga — `D-136` — e quella è l'unica da usare. `--count 3` ne dà tre contigui.

Non si sceglie a mano, e non si «verifica l'ultimo e si fa +1»: è la race, non la sua mitigazione.

## 4. Dove vive lo stato

```
$(git rev-parse --git-common-dir)/rt-shared-ids/
    state.json        # last_issued per namespace + reservation, diagnostiche
    allocator.lock    # la sezione critica
```

**Non versionato**, e nel *common dir* — l'unica directory condivisa da tutti i worktree di un clone. Un
file versionato tipo `docs/decisions/next-id.txt` avrebbe l'effetto opposto: una copia per worktree, che
è precisamente non-condivisione, più un merge conflict a ogni PR.

Il lock è reale (`msvcrt.locking` su Windows, `fcntl.flock` su POSIX) e copre **tutta** la sequenza
*leggi → ispeziona → scegli → scrivi*. Leggere fuori dal lock e scrivere dentro ricrea la race che si
voleva togliere.

### Cosa conta come «già preso»

L'allocatore considera occupato un ID che compaia in **una qualsiasi** di queste:

| fonte | copre |
|---|---|
| working tree di **tutti** i worktree del clone | il lavoro scritto e non ancora committato |
| `refs/heads/*` | i branch locali |
| `refs/remotes/origin/*` | i branch pushati, dopo un `git fetch` |

La definizione è arrivata in tre passaggi, e ognuno è costato una collisione mancata per un soffio —
`origin/main` da solo emetteva un numero già preso da due branch aperti; esteso ai ref, ne emetteva uno
che un worktree accanto teneva fra le modifiche non committate. Vale la pena leggerlo come un principio:
**una rivendicazione non ha bisogno di essere mergiata per essere reale.**

## 5. Perché i gap sono ammessi

Il contatore è **monotono**: non scende, e un ID prenotato per un branch poi abbandonato resta un buco.

```
D-134 prenotato e mai usato
D-135 usato
```

*I gap costano zero; il riuso costa ambiguità.* Un numero riemesso rende due decisioni indistinguibili in
ogni citazione già scritta — e le citazioni sono centinaia. Un buco, al massimo, fa chiedere «e il 134?».

## 6. I due gate, che rispondono a due domande diverse

```powershell
python scripts/rt_shared_id.py check        # dentro questo albero
git fetch --prune origin
python scripts/rt_shared_id.py audit-refs   # fra i rami
```

| | domanda | trova |
|---|---|---|
| `check` | *questo Decision Log è coerente con sé stesso?* | lo stesso ID dichiarato due volte, ID malformati (`D-44`, `D_045`), un ID riservato da un altro branch |
| `audit-refs` | *due rami stanno per portare la stessa etichetta su decisioni diverse?* | stesso ID con **tesi** diverse fra i ref, e duplicati interni a un singolo ref |

`audit-refs` confronta la **tesi** — il primo segmento in grassetto della decisione — non il testo
intero. Una decisione ereditata da `main` e poi ampliata su un branch è la stessa decisione: confrontare
il corpo la segnalerebbe a ogni revisione, e un gate che suona sempre viene spento.

E salta i ref **già antenati di `origin/main`**, perché una cicatrice non è una ferita: un branch
mergiato porta la storia com'era *prima* del merge, comprese le collisioni che il merge ha già risolto
rinumerando. Su questo repository erano **12 ref su 22** — più della metà del rumore, tutto su rami che
nessuno toccherà più. Non produce falsi negativi: la storia di un ref mergiato è contenuta in `main`,
quindi una collisione ancora viva lì la vede `check`.

⚠️ L'unico ref che il filtro **non** può saltare è `origin/main` stesso, che è antenato di sé stesso.
Escluderlo toglie il termine di paragone e rende verde una collisione viva — è successo, ed è il motivo
per cui esiste `test_un_ref_vivo_con_la_stessa_collisione_suona`.

⚠️ Il `fetch` **non** è dentro lo script: è un'operazione di rete, e qui le operazioni di rete si fanno
esplicitamente. Senza fetch, `audit-refs` giudica su ref vecchi.

## 7. Quando `audit-refs` è rosso

Non risolve niente da solo. Dice quale ID e quali rami:

```
COLLISION D-132
  7b3e0a474dd0  **I profili di reazione sono entità di catalogo…
    docs/e14-profili-brace-e-taratura-adr
  de8aa3450fff  **Il redirect degli Stable ID ritirati si cancella…
    origin/fix/d132-elimina-redirect-legacy
```

Chi non è ancora mergiato **rinumera prima del merge**: `reserve` per un ID nuovo, poi si correggono i
rimandi per coppia `(file, riga)` — mai con una sostituzione globale, che tocca anche le citazioni
storiche che devono restare com'erano. Se una delle due è già su `main`, rinumera la **seconda** e
registra lo spostamento nelle Note del Decision Log: la numerazione del progetto è un dato pubblico, e un
numero che cambia senza traccia rompe ogni riferimento esterno.

## 8. Il limite: più cloni, più PC

L'atomicità si ferma al **clone**. Due cloni sullo stesso PC, o due PC, non condividono il common dir:
nessun lock li serializza, e possono emettere lo stesso ID nello stesso istante.

Per quel caso `audit-refs` è una difesa **a valle** — diagnostica prima del merge, non previene. Non è
un difetto nascosto: è il perimetro dichiarato della soluzione. Se le collisioni residue dovessero
arrivare solo da lì, le evoluzioni possibili sono una reservation remota (GitHub come autorità, con il
costo di rete e autenticazione) o l'abbandono dei contatori globali — nessuna delle due giustificata
finché il problema misurato è locale.

Resta fuori anche un file aperto in un editor e **non salvato**: nessuna difesa, se non salvare.

## 9. Recovery

**`state.json` assente** — normale al primo uso su un clone: l'allocatore fa il bootstrap dal massimo
canonico e prosegue. Non serve fare niente.

**`state.json` illeggibile** — non va riparato a intuito. Si rinomina come backup e si rilancia: il
bootstrap lo ricostruisce dalle dichiarazioni correnti, e il contatore resta monotono perché non riempie
mai i buchi. Se il backup è leggibile, se ne prende il `last_issued` come pavimento aggiuntivo. In ogni
caso si sceglie un valore **maggiore**: mai tentare di recuperare i gap.

**Reservation orfana** — un ID prenotato e mai comparso nel Decision Log. `status` la mostra come
`orfana`, e resta così finché la decisione non atterra. **Non torna disponibile.** «Orfana» qui significa
anche solo «dichiarata su un branch che questo worktree non ha»: è diagnostica, non un verdetto.

**Cedere un ID a un'altra sessione**

```powershell
python scripts/rt_shared_id.py release D-134
```

Quando l'ID che hai riservato lo sta già usando qualcun altro — capita se quella sessione non passa
dall'allocatore — la reservation a tuo nome diventa una diagnostica **falsa**: `check` segnalerebbe come
collisione (`reserved-by-other-branch`) un uso perfettamente legittimo. `release` la toglie.

⚠️ **Cedere non è liberare.** `last_issued` non scende: il numero resta bruciato per chiunque altro, e
chi lo sta già usando se lo tiene. È successo il giorno dell'introduzione, con `D-134`.

## 10. Cleanup dei worktree

```powershell
git worktree list                  # cosa esiste davvero
git worktree remove ..\rt-wt-621   # rifiuta se ci sono modifiche non committate
git worktree prune                 # ripulisce le registrazioni morte
```

Il rifiuto di `remove` è una protezione, non un ostacolo: quei file esistono **solo** lì. Si guardano e
si decide, non si forza.

Rimuovere un worktree **non** libera gli ID che ha prenotato, e non deve: la decisione può essere
atterrata da un'altra parte, e un numero riemesso è ambiguo per sempre.

---

## 11. Quattro processi, non quattro roadmap

Il progetto avanza su **quattro processi paralleli reali**: tre sessioni Claude su worktree e branch
distinti, e l'autore umano davanti a Unreal Editor.

| Processo | Missione | Chi |
|---|---|---|
| **Spatial / World** | lo spazio logico come dato e query: celle, layer, graph, path, LOS, occupancy, bake | Claude |
| **Simulation / Rules / AI** | ciò che il gioco **decide**: planning, snapshot, resolver, reazioni, status, bot, TurnLog | Claude |
| **Client / Replay / Tooling** | come il risultato viene **visto, riprodotto e diagnosticato**: HUD, preview, replay, editor tooling | Claude |
| **Content / Editor** | il lavoro davanti a Unreal: import, Blueprint, Data Asset, materiali, mappe, PIE, QA visiva | umano |

Due invarianti li tengono insieme:

- **Simulation produce il TurnLog canonico; Replay lo consuma.** Non nasce un secondo simulatore.
- **Client non decide esiti competitivi.** Presenta ciò che il resolver ha già deciso.

⚠️ **Questi quattro nomi non sono un dato, e non devono diventarlo.** Non esiste un
`parallel-tracks.yaml`, non esistono quattro shortlist generate, non esiste un campo `work_tracks` nel
Feature Registry — e la ragione è misurata in
[`../roadmap/plans/quattro-processi-paralleli-triage-2026-08-14.md`](../roadmap/plans/quattro-processi-paralleli-triage-2026-08-14.md) §4:
[`execution-graph.yaml`](../roadmap/execution-graph.yaml) ha già **due** tassonomie validate sugli stessi
oggetti — `execution_lanes` (`code`/`pie`/`asset`: *chi esegue*) e `domain_groups` (8 gruppi sulle `area`
del registry: *di cosa parla*) — e il processo Content/Editor **è** `pie` + `asset`. I tre processi Claude,
per contro, non sono una funzione di `domain_group`: `characters_content` si spezza su tre di essi e
`tooling_data_qa` su due. Un terzo vocabolario andrebbe scritto per nodo, e oggi **nessuno lo legge**.

Un processo può essere **IDLE**, e non è un difetto. *Il parallelismo minimizza il wall-clock, non
massimizza il numero di branch.* Non si inventano task per saturare quattro sessioni.

## 12. Il write-set: non basta il dominio

Due processi con missioni diverse possono comunque dover toccare lo stesso file — un test condiviso, un
header che entrambi estendono, un documento owner. Il dominio non è una garanzia di disgiunzione: il
write-set sì.

Prima di avviare un lotto di sessioni parallele si dichiara chi scrive cosa, in
[`../roadmap/parallel-batch.yaml`](../roadmap/parallel-batch.yaml).

```
WritableSet(T1) ∩ WritableSet(T2) = ∅
BinaryLeaseSet(T1) ∩ BinaryLeaseSet(T2) = ∅
```

**La regola, in una riga: file non assegnato = STOP.** Il path deve appartenere al `writable` della tua
track; altrimenti ci si ferma e si registra una richiesta di riallocazione. Non si fa «solo questa piccola
fix». Vale per C++, docs, scripts, Config, `.uasset`, `.umap`, test e output generati.

Tre categorie non si assegnano a nessuno:

- **`integration_only`** — i file che più branch potrebbero voler toccare (`AGENTS.md`, il Decision Log,
  `feature-registry.yaml`, `scripts/feature_registry.py`, il `.uproject`, `Config/`). Si aggiornano **una
  volta**, in integrazione. Non è ownership permanente: è una proprietà del batch.
- **`generated_only`** — le viste generate. **Non si assegnano mai direttamente: seguono la propria
  sorgente.** Chi possiede la sorgente nel proprio `writable` è autorizzato — e obbligato — a rigenerare
  esattamente le viste che quella sorgente alimenta, e nient'altro. Se due track possiedono sorgenti della
  **stessa** vista, quella vista è contesa e una delle due esce dal batch: è un test da fare in fase di
  **selezione**, non al merge.

  ⚠️ *«Vietato a tutti» è la formulazione sbagliata, e la prima stesura di questa riga la usava.* Produce
  una coppia di regole **insoddisfacibile**: chi tocca una sorgente non può né rigenerare (path non
  assegnato) né lasciar stare (`--check` rosso). Il caso è reale — `wt-cap` possiede `RTScenarioSession.cpp`
  e ha già dovuto rigenerare `scenariomap.shortlist.md`, correttamente.

  ⚠️ *Un branch che **non** ha toccato sorgenti e rigenera lo stesso porta dentro modifiche altrui*, e
  sembrano sue. E in ogni caso **si rigenera un'ultima volta sull'albero unito**: due rami che rigenerano
  ciascuno sulla propria base producono due versioni dello stesso file da una sorgente che nessuno dei due
  vede intera, e il conflitto che ne esce non si risolve scegliendo un lato.

  ➕ Il repository ha **due** toolchain: `feature_registry.py` in Python e `tools/radar/` in Node, che
  produce otto SVG versionati con un `--check` proprio ([D-108](../decisions/RT_PDR_00_Decision_Log.md)).
  Chi tocca `docs/balance/` rende rosso il secondo gate senza che il primo dica niente.
- **`preexisting`** — i branch già vivi al momento del calcolo. Non fanno parte del batch: sono il vincolo
  che lo determina.

### Come si sceglie un batch

1. ordina il backlog per release, priorità e dipendenze;
2. prendi il task più utile e stima il suo `WritableSet`;
3. cerca il prossimo task con set **disgiunto** da tutti i precedenti *e dai branch già aperti*;
4. ripeti finché i set restano disgiunti;
5. assegna le lease binarie necessarie;
6. il task umano entra solo se è sbloccato;
7. se nessun task residuo è sicuro, quella track è `IDLE`.

⚠️ **Il passo 3 include i branch aperti, e questa è la parte che si dimentica.** Il write-set di un branch
vivo si misura, non si intuisce:

```powershell
git worktree list
gh pr list --state open
git diff --name-only origin/main...<branch>
```

Il 2026-08-14, il primo batch calcolato dopo `D-139` ha perso il proprio validator proprio così: il file da
estendere era fra i 16 di una PR aperta.

## 13. Binary Asset Lease

I binari Unreal sono **human-first, non human-only**: il processo Content/Editor è l'holder predefinito, e
Claude può creare, modificare, rinominare, risalvare o migrare un binario **solo** con una lease esclusiva
dichiarata nel batch.

```yaml
binary_leases:
  - key: "BINARY-GH623-DEVSANDBOX-LIGHTING"
    holder: content_editor
    issue: 623
    base_sha: "<sha su cui la lease è valida>"
    operation: modify          # create | modify | rename | resave | migrate
    paths:
      - Content/RT/Maps/Dev/L_DevSandbox/L_DevSandbox.umap
      - Content/RT/Maps/Dev/L_DevSandbox/Data/DA_HexMap_Sandbox.uasset
    verification:
      - load_in_editor
      - save_without_errors
      - run_validator
```

- `key` **semantica e legata a GitHub**, mai `LEASE-001`: un identificatore progressivo è un altro contatore
  condiviso, e questo documento esiste perché quelli collidono.
- **Un solo holder** per path. Nessun secondo holder, né umano né Claude.
- La lease vale sul `base_sha`: se `main` tocca lo stesso asset, la lease è **stale** e va riemessa.
- `create` prenota una **destinazione**: due track non possono creare lo stesso path.

⚠️ **Una lease su una mappa si emette sulla cartella, non sul file.** Una mappa non è un file: è una
cartella, e quanti package contenga **non si deduce dal nome**. Delle tre mappe del progetto, due portano
anche il proprio `Data/DA_HexMap_*.uasset` — che la mappa referenzia — e `L_Prototype` no. Si misura, una
per una:

```powershell
git ls-files "Content/RT/Maps/Dev/L_DevSandbox/"
```

Se la mappa usa World Partition, External Actors, sublevel o Data Layer, anche i package che Unreal genera
entrano nella lease.

### Non esiste il binary merge

Due versioni di un `.uasset` non si fondono. Se entrambe le modifiche servono:

```text
scegli una base → apri Unreal → riapplica l'altra modifica a mano → save → validator/test
```

La lease non previene un conflitto: previene un **lavoro perso**.

### Anche con la lease, solo attraverso Unreal

Consentiti: Unreal Editor, Content Browser, editor scripting approvato, commandlet di resave supportati,
tool del progetto. Vietati: hex editor, patch binarie, rename da filesystem, spostare package senza il
workflow dei redirector. Rename e move passano da Content Browser + **Fix Up Redirectors** + audit dei
riferimenti — è la regola di [`../../AGENTS.md`](../../AGENTS.md) §Unreal, che la lease non sostituisce.

⚠️ **La ignore policy non si tocca per comodità.** Molti asset Fab vivono nel vault, restano fuori da git e
vengono migrati per dipendenza: prima di modificare allowlist o `.gitignore` si legge
[`convenzioni-contenuti-ue.md`](convenzioni-contenuti-ue.md) e si verifica il vault. `Content/` non diventa
un albero tutto tracciato per far entrare una lease.

## 14. Gli altri contatori condivisi

`rt_shared_id.py` copre `D-nnn`. Gli altri namespace monotoni sono stati auditati il 2026-08-14, e il
risultato è che **non serve estenderlo**:

| Namespace | Serializzato | Owner | Policy |
|---|:---:|---|---|
| `D-nnn` | no | Decision Log | ✅ `reserve` |
| `E-nn` · `XXX-n` | no | roadmap · `OPEN_DECISIONS.md` | manuale, verifica sul remote prima del merge |
| `CP x.y` | no | roadmap owner | manuale — ⚠️ i due spazi (`Enn` e `Mnn`) **collidono già** per costruzione |
| `ADR-nnnn` | no | `docs/decisions/` | manuale, `0001`…`0009` |
| `ERTTurnLogFormatVersion` | **sì** | `RTTurnLog.h` | v7 — audit forte |
| `URTHexMapAsset::CurrentFormatVersion` | **sì** | `RTHexMapAsset.h` | v8, in migrazione ([D-137](../decisions/RT_PDR_00_Decision_Log.md)) |
| `ERTReplayManifestVersion` | **sì** | `RTReplayManifest.h` | 1 |

I tre serializzati hanno **una sola sorgente di scrittura** ciascuno: la difesa che serve non è un lock, è
il controllo che i loro stessi commenti prescrivono — *prima di prendere il numero, cercalo su **tutti** i
branch remoti, non solo su `main`*. Una versione di formato duplicata non si rinumera: corrompe tracce già
scritte, perché il loader sceglie l'interpretazione dal numero.

Il dettaglio della misura è nel triage
[§7.3](../roadmap/plans/quattro-processi-paralleli-triage-2026-08-14.md).

## 15. Chiudere un batch

Non serve un `Gxx` nuovo: l'integration gate è **operativo**, e l'identità del batch non è progressiva —
`BATCH-<base_sha>-gh41-gh583`.

1. freeze delle sessioni attive;
2. `git fetch --prune origin`;
3. `python scripts/rt_shared_id.py check` e `audit-refs`;
4. verifica che i write-set siano rimasti disgiunti e che i `base_sha` reggano;
5. merge dei branch testuali/C++;
6. integra **una lease binaria alla volta**, con load/save/validator dentro Unreal fra una e l'altra;
7. aggiorna i documenti `integration_only` **una volta**;
8. rigenera le viste (`generate` **e** `shortlist`, in quest'ordine e alla fine);
9. gate di documentazione e suite richiesta dal DoD;
10. registra gli esiti PIE/manuali quando applicabili;
11. aggiorna GitHub;
12. calcola il batch successivo.

⚠️ **Il passo 8 va fatto sull'albero unito, non su un branch.** Un generato prodotto prima del merge è
corretto sulla propria base e falso dopo — è la stessa lezione che il totale di
[`../archive/src/README.md`](../archive/src/README.md) ha imparato nove volte.
