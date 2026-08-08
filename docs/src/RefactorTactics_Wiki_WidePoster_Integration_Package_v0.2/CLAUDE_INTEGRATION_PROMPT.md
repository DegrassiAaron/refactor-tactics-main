# TASK — INTEGRARE IL SET WIDE-POSTER NELLA WIKI REFACTORTACTICS

Stai lavorando nella repository `refactor-tactics-main`.

Questo pacchetto sostituisce il precedente pacchetto “OldStyle”. Usa ESCLUSIVAMENTE gli asset presenti qui.

## STILE APPROVATO

Gli asset appartengono allo stile visuale approvato dal progetto:
- poster wide / landscape;
- dark sci-fi;
- pseudo-personaggi illustrati;
- mappe isometriche esagonali;
- mini-scene di gameplay;
- pannelli/callout compatti;
- più informazione visuale, meno look da dashboard tecnica.

NON sostituirli con:
- il vecchio set verticale “old-style”;
- il set schematico Python/v0.8;
- placeholder o screenshot generici.

## PRIMA DI MODIFICARE
1. Leggi `AGENTS.md` / `CLAUDE.md`.
2. Leggi `docs/wiki/README.md` e `docs/wiki/index.md`.
3. Leggi le pagine target presenti in `wiki-image-manifest.json`.
4. Leggi Decision Log, ADR e cataloghi pertinenti.
5. Verifica `.gitattributes` / Git LFS per PNG.

## REGOLA DI AUTORITÀ
Le immagini NON sono normative. Il testo Wiki deve restare aderente alle fonti correnti.
Se un testo dentro una figura è storico o diverge dal canone:
- non cambiare il gameplay per far combaciare la figura;
- mantieni corretta la pagina testuale;
- segnala il mismatch nel report finale;
- se il mismatch è grave e rende l’immagine ingannevole, non inserirla e riportala come BLOCKED.

## STRUTTURA EDITORIALE
- Pagina hub principale: UNA immagine `core`.
- Pagina di dettaglio/meccanica: usa immagini `detail` solo se aggiungono informazione distinta.
- Non usare la stessa immagine in più pagine.
- Non trasformare la Home in una galleria infinita.

## MAPPING
Usa `wiki-image-manifest.json` come fonte machine-readable per:
- `asset`;
- `targetPage`;
- `placement`;
- note/editorial constraints.

Sono presenti 10 immagini core e un set più ampio di immagini di dettaglio, comprese quattro card personaggio.

## PAGINE NUOVE CONSENTITE
Creale soltanto se non esistono già equivalenti:
- `docs/wiki/game/planning-e-coordinazione.md`;
- `docs/wiki/technical/turnlog-e-determinismo.md`.

Prima cerca equivalenti nella struttura attuale. Non duplicare pagine esistenti.

## NAVIGAZIONE
Aggiorna solo quanto serve:
- `docs/wiki/index.md`;
- `docs/wiki/README.md`;
- `docs/characters/index.md`;
- eventuale indice `docs/wiki/meccaniche/index.md`;
per rendere raggiungibili le nuove pagine.

## PRIVACY PLANNING
Qualunque testo sulla coordinazione deve ribadire il modello reale:
- CanonicalIntentStore completo solo server;
- relay sanitizzato team-only;
- nessun planning avversario replicato e poi “nascosto dalla UI”.

## ACTION ECONOMY / DASH
Nella pagina azioni preserva il canone corrente della repository.
L’immagine core #3 è stata scelta perché rende visibili Dash e le azioni fondamentali; confrontala comunque con Decision Log e Action Catalog prima dell’inserimento.

## REAZIONI
Verifica che il testo corrente mantenga Fast Reaction baseline 3,0 s e timeout conservativo HOLD, se ancora canonici.
Non introdurre vecchi riferimenti a 5 s.

## ROSTER
Per la v0.1 usare soltanto Flux, Riva, Bastion, Vektor.
Le 4 card individuali vanno nelle rispettive schede personaggio, non ripetute altrove.

## VALIDAZIONE
Dopo l’integrazione:
1. controlla tutti i path relativi immagini;
2. controlla rendering Markdown;
3. cerca immagini duplicate nella stessa area;
4. cerca vecchi roster nelle pagine modificate;
5. cerca `5s` / Reaction Window obsoleta nelle pagine modificate;
6. cerca vecchi Stable ID/azioni legacy nelle pagine modificate;
7. verifica Git LFS;
8. esegui validator docs già presenti.

## OUTPUT FINALE
Restituisci:
### Files changed
### Images integrated
Per ogni immagine: pagina + sezione + path.
### Images blocked
Con motivo e mismatch.
### New pages
### Redundancy audit
### Canon mismatches found
### Validation
### Suggested commit
`docs(wiki): integrate wide gameplay infographic set`
