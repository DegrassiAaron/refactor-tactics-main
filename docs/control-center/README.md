# Project Control Center

> `CURRENT` · **Vista web read-only** sopra gli artefatti generati dal Feature Registry.
> Owner del disegno: [`../roadmap/plans/project-control-center-spec.md`](../roadmap/plans/project-control-center-spec.md).
> Tracciata da `RT-FEAT-TOOL-CONTROL-CENTER` in [`../roadmap/feature-registry.yaml`](../roadmap/feature-registry.yaml).

## Come si apre

```bash
python -m http.server 8000        # dalla radice del repository
# poi: http://localhost:8000/docs/control-center/
```

Serve un server perché il browser blocca `fetch` e i moduli ES da `file://`. Aperta comunque da
file, la pagina se ne accorge e chiede i due `.json` con un selettore di file: stesso dato, stessa
vista. Nessuna delle due strade usa la rete — i dati vengono dal working tree.

L'unica chiamata di rete è **facoltativa**: alla GitHub API, per sapere se il grafo è più vecchio
del registry. Senza rete la pagina funziona e lo dichiara («freschezza non verificabile»).

## Cosa mostra

| Vista | Risponde a |
|---|---|
| **Overview** | quanto è avanti il progetto, quali gate mancano di più, quanti warning |
| **Roadmap** | epic e milestone con il loro stato, le feature e i checkpoint di ciascuna |
| **Feature Map** | le 85 feature con dieci filtri e ricerca; una card per feature, gate a barre |
| **Scenario Map** | i 57 scenari, le capability che chiedono, chi li dichiara |
| **Editor Map** | le 19 sedute con prerequisiti, artefatti e stato |
| **My Editor Queue** | `BLOCKING` · `READY` · `WAITING` · `DONE` |
| **Diagnostica** | riferimenti che non risolvono, cicli, i warning del validator per classe |

Cliccando un elemento si apre un pannello con le sue relazioni **nei due versi**: da una feature
alle sue issue e ai suoi scenari, e da uno scenario alle feature che lo dichiarano.

## La regola che la tiene onesta

**La pagina non calcola stato.** Nessun `status`, nessun gate, nessun ✅/🟡/⏳ nasce qui: arrivano
già calcolati da `scripts/feature_registry.py`, che ne è l'unica implementazione. Una seconda
regola in JavaScript divergerebbe in silenzio, e divergerebbe proprio sul dato che questa pagina
esiste per mostrare.

Se una vista ha bisogno di un valore che il generatore non produce, **si estende il generatore**.

Ne discendono tre cose visibili:

- il progresso si legge `N/M gate` e mai in percentuale — `na` esce dal denominatore;
- i link a GitHub si derivano da `meta.project.github` del registry: nessun URL assoluto è scritto
  altrove, e cambiare branch li cambia tutti;
- un riferimento che non risolve è **mostrato in rosso**, non nascosto.

## Il banner di freschezza

Il repository non ha CI: `generate` lo lancia una persona. Un `.yaml` modificato senza rigenerare
produce una pagina plausibile e vecchia — il difetto peggiore, perché non si vede. La pagina
confronta le date degli ultimi commit di `feature-registry.yaml` e `project-graph.json` e, se la
sorgente è più recente, lo dice in testa. Non può impedire il drift; può renderlo visibile.

## File

| File | Cosa |
|---|---|
| `index.html` | la pagina: markup, stile e rendering, senza dipendenze |
| `graph.js` | le funzioni pure — link, indice, relazioni inverse, riferimenti rotti, cicli, filtri |
| `graph.test.mjs` | 18 test: `node --test docs/control-center/graph.test.mjs` |
| `package.json` | dichiara ES module per il runner. Nessuna dipendenza, nessun build step |

I test si dividono in due gruppi, e la differenza conta: quelli su fixture dicono che la **regola**
è giusta e falliscono se la si rompe; quelli sui file veri dicono che il **contratto** regge oggi e
falliscono se il generatore cambia forma senza avvisare. Il secondo gruppo ha già fatto il suo
lavoro una volta: ha trovato che `checkpoint_status()` non leggeva la forma prefissata `E21.1`.

## Cosa non fa

- **Non scrive**: nessuna richiesta che non sia una lettura. La scrittura via PR è discussa nella
  spec §9 (D-D) e resta fuori scope: senza backend questa pagina non potrà mai farla.
- **Non sostituisce le shortlist**: `docs/roadmap/*.shortlist.md` restano le viste in repository,
  leggibili in una code review e in un diff.
- **Non è una source of truth**: nessun dato nasce qui.
