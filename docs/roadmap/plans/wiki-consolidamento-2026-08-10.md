# Consolidamento della Wiki — 2026-08-10

> ## 📸 `HISTORICAL` — REFERTO E PIANO, NON UNA VISTA DI STATO
>
> Questo documento registra **cosa è stato trovato** confrontando la sorgente in-repo con il clone
> pubblicato, e il piano deciso di conseguenza. Non si aggiorna con l'avanzamento: quando le fasi
> sono chiuse, resta come spiegazione del perché la Wiki è organizzata così.
>
> **Origine**: `/sc:spec-panel` del 2026-08-10 — «riorganizziamo e consolidiamo la wiki».

## Base verificata

| | |
|---|---|
| HEAD repository | `781e586` (`main`) |
| HEAD clone Wiki | `df44237` (`master`) |
| Sorgenti che mappano a una pagina pubblicata | **44** |
| Pagine allineate | **15 / 44** |
| Righe di **prosa** solo nel clone | **602** |
| Righe di **prosa** solo nel repository | **490** |
| Righe di sola conversione (wiki-link, path immagini) | 178 + 65 link · 27 + 8 immagini |
| Pagine del clone senza sorgente | **40** (di cui 34 schede Paragon) |
| Sorgenti mai pubblicate | **1** (`docs/wiki/game/avversario-bot.md`) |

Misure ottenute confrontando i due tree a blocchi generati rimossi, non citate da altri documenti.

## A. La diagnosi

`docs/wiki/` **non è un duplicato** del clone: è un fork bidirezionale. L'architettura dichiarata —
`DEPLOY.md` nel clone e `deploy_name()` / `apply_wiki_deploy()` in
[`../../../scripts/feature_registry.py`](../../../scripts/feature_registry.py) — prevede
sorgente → deploy, ma il flusso trasporta **solo i blocchi `RT_FEATURE_STATUS`**. Tutta la prosa è
copiata a mano, in entrambe le direzioni, e ha divergito.

### Il caso che dimostra il difetto

`docs/wiki/game/azioni-e-movimento.md` contraddice **se stesso**:

| Riga | Contenuto | Origine |
|---|---|---|
| 62 | «Quattro azioni generiche su **sette** sono complete…» | blocco generato dal registry |
| 92 | `Wait · BasicAttack · Interact · Brace · Move · Overwatch` | prosa a mano — **sei voci, manca `Guard`** |

L'elenco canonico è di **sette** azioni (D-025, incluso `Guard`). La correzione esiste — commit
`5889b64` nel clone, *«quattro azioni generiche su sette, non sei»* — ma è stata applicata **solo a
valle** e mai riportata a monte. La sorgente viola una decisione canonica; la versione corretta vive
soltanto nella pagina pubblicata.

Non è un problema di sincronizzazione: è un **difetto di conformità** che nessun controllo
intercetta. `scripts/check-docs-symbols.py` non copre gli elenchi di azioni in prosa.

### Perché il fork è stato possibile

1. **Il flusso è unidirezionale e parziale.** Esiste `sorgente → nome flat`; non esiste l'inverso.
   Una pagina nata nel clone non ha modo di tornare indietro. `Meccanica-griglia-e-geometria.md`,
   scritta il 2026-08-10, non ha sorgente.
2. **I guasti sono silenziosi.** `deploy --check` stampa `WARN … la pagina non esiste nel clone` ed
   esce con codice 0. Un warning che non fallisce non è un controllo.
3. **Nessuno esegue il check.** 13 pagine pubblicate portano blocchi di stato derivati da uno stato
   vecchio; il repository è allineato (`wiki --check` → «pagine gia' allineate»), il clone no.
4. **Il processo non ha una casa.** È descritto solo dentro
   [`../feature-registry.md`](../feature-registry.md), che è un documento **generato**.

### Difetto latente da correggere

`deploy_name("docs/wiki/game/sinergie-e-combinazioni.md")` restituisce `sinergie-e-combinazioni.md`,
ma il clone traccia `Sinergie-e-Combinazioni.md`. Su Windows `os.path.isfile` è case-insensitive e la
cosa passa inosservata; su un filesystem case-sensitive la pagina risulterebbe **assente** e verrebbe
saltata in silenzio, insieme ai suoi blocchi di stato.

## B. Il criterio deciso

Nessuna delle due copie «vince». Il contenuto si separa per **natura**, e ogni classe prende una
sola casa:

| Classe di contenuto | Casa | Perché |
|---|---|---|
| Blocchi di stato, tabelle Fazione, ID, gate, scenari | **generato** dal registry | ha già un oracolo verificabile |
| Prosa di regole con riferimenti ad ADR, catalogo, decisioni | `docs/wiki/` + `docs/characters/` | path relativi e simboli sono verificabili solo in-repo |
| Navigazione, `_Sidebar`, `_Footer`, wiki-link, immagini pubblicate | **solo clone** | è formato di presentazione, non contenuto |
| 34 schede Paragon | **solo clone**, dichiarate fuori ciclo | provenienza visuale: nessun ADR o catalogo le governa |

Le schede Paragon restano nel clone **per decisione**, non per dimenticanza: vanno dichiarate
esplicitamente fuori dal ciclo sorgente→deploy, altrimenti continuano a risultare orfane a ogni
verifica futura.

## C. Matrice per pagina

Colonne: righe di prosa presenti **solo** da un lato, a blocchi generati rimossi. Le colonne `link` e
`img` sono conversione di formato e **non** vanno merghiate: sono la differenza legittima fra
sorgente e deploy.

| Pagina pubblicata | prosa solo clone | prosa solo repo | Azione |
|---|---:|---:|---|
| `Fazione-Sentinel-Directorate.md` | 90 | 13 | merge manuale |
| `Fazione-Constrine.md` | 89 | 13 | merge manuale |
| `Fazione-Conflux.md` | 88 | 13 | merge manuale |
| `Fazione-Resonance.md` | 88 | 13 | merge manuale |
| `sinergie-e-combinazioni.md` | 59 | 25 | merge manuale |
| `Personaggio-flux.md` | 22 | 46 | merge manuale |
| `Personaggio-riva.md` | 20 | 49 | merge manuale |
| `Personaggi-Paragon.md` | 5 | 55 | merge manuale |
| `reazioni-overwatch-e-previsioni.md` | 4 | 55 | merge manuale |
| `Meccanica-facing-e-direzionalita.md` | 13 | 35 | merge manuale |
| `Fazioni.md` | 35 | 6 | merge manuale |
| `Personaggio-vektor.md` | 9 | 29 | merge manuale |
| `Stato-delle-feature.md` | 12 | 25 | merge manuale |
| `azioni-e-movimento.md` | 8 | 29 | merge manuale — **contiene la violazione D-025** |
| `Personaggio-bastion.md` | 9 | 25 | merge manuale |
| `Home.md` | 11 | 21 | merge manuale |
| `Personaggi.md` | 1 | 12 | merge manuale |
| `Personaggio-aurora.md` | 9 | 1 | merge manuale — blocco Fazione → generabile |
| `Personaggio-kwang.md` | 9 | 1 | merge manuale — blocco Fazione → generabile |
| `Personaggio-murdock.md` | 9 | 1 | merge manuale — blocco Fazione → generabile |
| `Personaggio-steel.md` | 9 | 1 | merge manuale — blocco Fazione → generabile |
| `Meccanica-acqua-e-elettricita.md` | 1 | 6 | merge manuale |
| `Meccaniche.md` | 0 | 11 | deploy repo → clone |
| `Meccanica-topologia-dinamica.md` | 0 | 4 | deploy repo → clone |
| `mappa-terreni-e-ambiente.md` | 0 | 1 | deploy repo → clone |
| `che-cose-refactortactics.md` | 2 | 0 | backport clone → repo |
| `struttura-del-round.md` | 0 | 0 | solo immagine |
| `visibilita-rumore-e-informazione.md` | 0 | 0 | solo immagine |
| `avversario-bot.md` | — | — | **mai pubblicata** |

**22** pagine richiedono un merge editoriale vero; le altre 7 sono meccaniche.

Le quattro pagine Fazione e i quattro eroi v0.2 condividono lo stesso schema: la prosa del clone è la
**tabella Fazione** (`FactionId`, `Faction Mark`, scenario dimostrativo). È dato puro e va **generata**
dal `factions-manifest`, non merghiata a mano — altrimenti tornerà a divergere.

Le quattro schede v0.1 hanno lo schema opposto: nel repo esistono *Profilo di attacco base*,
*Misplay / Failure State* e il banner di ownership che il clone non ha.

## D. Le fasi

### Fase 0 — fermare la deriva ✅ *(eseguita su questo branch)*

Nessuna riscrittura di contenuto.

- `deploy --check` esce con codice non-zero quando disallineato.
- Il `WARN` per sorgente-senza-pagina diventa un **errore**, per la stessa ragione dello scenario
  orfano: il gate `ui_wiki` afferma una copertura che il lettore non può vedere.
- Il confronto coi nomi del clone diventa **case-sensitive** anche su Windows, e
  `game/sinergie-e-combinazioni.md → Sinergie-e-Combinazioni.md` entra in `GAME_PAGE_EXCEPTIONS`:
  vince il nome già pubblicato, perché rinominarlo cambierebbe l'URL della pagina.
- Questo documento committato come inventario.

### Fase 1 — conformità, non sincronizzazione ✅ *(chiusa, ma non da questo branch)*

> **La violazione di D-025 è stata corretta da [#399](https://github.com/DegrassiAaron/refactor-tactics-main/pull/399)
> mentre questo branch era aperto**, e meglio di come l'aveva corretta questo branch: stessi numeri, più
> fatti — `Brace` costa il movimento volontario e regge anche da dietro, mentre `Guard` copre solo il
> davanti — e una nota `BAL-1` più affilata, che dice *perché* oggi la differenza non si vede (nessuna
> spinta del gioco supera una cella). Al merge ha vinto la versione di `main`; di questo branch resta
> solo il conteggio esplicito «di **sette** voci» nell'introduzione.
>
> Vale la pena registrarlo: il difetto era reale e due lavori indipendenti l'hanno trovato lo stesso
> giorno. Non è una duplicazione da evitare, è la conferma che era visibile.

Restava da fare, ed è ciò che questo branch ha effettivamente prodotto: la **rassegna** delle altre
pagine divergenti, per capire se il caso fosse isolato o sistemico.

**Esito della rassegna sulle altre 21 pagine divergenti**: `azioni-e-movimento` era l'**unico** caso di
correzione viva solo a valle. La restante prosa esclusiva del clone è di due tipi, e nessuno dei due è
un backport:

- **note a figure che esistono solo nel clone** (`che-cose-refactortactics`, `reazioni-overwatch`): la
  didascalia che avverte «l'etichetta *fog of war* va letta come genere» annota un'infografica che la
  sorgente non ha, e resta legittimamente dov'è;
- **testo che il repository ha già superato**: `Meccanica-acqua-e-elettricita` dice ancora che nessun
  eroe possiede `Action.Electrify`, mentre la sorgente registra dal 2026-08-09 che
  `Flux.ConductiveNode` **è** `Action.Electrify` (D-046, D-064); `Meccanica-facing-e-direzionalita`
  descrive il modello a «tre direzioni» uguale per tutti, mentre la sorgente ha già la rotazione
  per-eroe in *step* e cita D-060. Sono casi di **deploy**, non di backport.

### Fase 2 — merge per classe di contenuto

- **Backport** clone → sorgente: prosa nata nel clone (fazioni, `Home`, `sinergie`,
  `planning-e-coordinazione`, `turnlog-e-determinismo`, `Meccanica-griglia-e-geometria`).
- **Promozione a generato**: tabelle Fazione nelle schede eroe, dal `factions-manifest`.
- **Solo clone**, dichiarati: `_Sidebar`, `_Footer`, `Wiki-Guide`, `DEPLOY`, immagini pubblicate,
  34 schede Paragon.
- Decidere su `avversario-bot.md`: pubblicarla o ritirarla.

### Fase 3 — chiudere il ciclo

- `deploy --write` diventa l'**unico** modo di scrivere nel clone, su ramo e mai su `master` diretto:
  il clone è un repository pubblico che altre sessioni possono avere in lavorazione.
- Pulizia di `docs/wiki/`: `RefactorTactics_Wiki_Infographics_v0.1.zip` è **tracciato in git**, le sue
  14 PNG non sono quelle usate dal clone (che usa `images/wiki/**` con altri nomi), e convivono tre
  manifest in parallelo (`v0.5`, `v0.6`, `v0.7`).
- Documento owner del processo Wiki in `docs/technical/`, che oggi non esiste.

## E. Controlli che mancano

Oggi non è possibile rispondere a «la Wiki pubblicata è corretta?» se non leggendola.

1. `deploy --check` fallisce se disallineato *(oggi stampa e passa)*.
2. Ogni pagina del clone ha una sorgente, e viceversa *(oggi 40 orfani in un verso, 1 nell'altro)*.
3. `scripts/check-docs-links.py` gira anche sul clone, dove i link sono `[[wiki]]` e non markdown
   relativo *(oggi non copre affatto il clone)*.

## Nota di gerarchia

`docs/wiki/README.md` e [`../../../AGENTS.md`](../../../AGENTS.md) §12 dichiarano che le pagine Wiki
**non sono normative**: prevalgono piano canonico, ADR, roadmap e cataloghi. Ne segue che la sorgente
in-repo non è più autorevole del clone — **entrambe sono derivate**. La domanda «quale delle due è la
verità» è mal posta: la verità è a monte di tutte e due, ed è per questo che il criterio di
consolidamento separa per natura del contenuto invece di scegliere un vincitore.
