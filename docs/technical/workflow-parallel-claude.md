# Sessioni parallele e ID condivisi — meccanismo

> **Owner del meccanismo.** Le *regole* stanno in [`../../AGENTS.md`](../../AGENTS.md) §Git; qui c'è come
> funziona e cosa fare quando si rompe. Decisione: [D-135](../decisions/RT_PDR_00_Decision_Log.md).

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
