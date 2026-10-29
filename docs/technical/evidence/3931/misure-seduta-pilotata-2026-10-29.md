# #3931 - sedute pilotate di `PIE-HEX-PAINT-ERASE`, `PIE-GEO-BORDO` e del ghost del diametro lato → lato (2026-10-29)

Evidenza **dell'assistente**, non un giudizio dell'autore: rimisura la meccanica, non l'occhio. Riguarda [#3931](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3931).

## Che cosa è stato fatto, e come

- **Editor interattivo sul clone principale** (`main` `e9f6e63bf`), livello `L_DevSandbox`, asset `DA_HexMap_Scratch_Basin`, mode **Hex Map**, camera a piombo a 800 uu.
- **Gesti con input sintetico**: mouse iniettato (movimenti assoluti sul desktop virtuale), tastiera per `Ctrl+Z` e `Canc`, console digitata carattere per carattere. Una guardia si ferma se rileva input dell'utente dopo l'ultimo input dell'assistente.
- **Letture dal bridge MCP** (`GetCurrentMap`, `ValidateTacticalMap`, `ObjectTools.get_properties`) prima e dopo ogni passo; il log dell'Editor (`-abslog`) per `[HexMode]`, `TRANSACTION UNDO` e `rt.Debug.DumpCellPlacement`.
- **Allestimento**: muro interno con il tool Geometry e arco con il tool Arch (*Commit*); i nomi `MURO-A` e `ARCO-A`, la porta `PORTA-A` sul lato E di `1,0` e il binding `ARCO-A` → `PORTA-A` scritti **in memoria** con `set_properties`.
- **Nessun salvataggio**: Editor chiuso con «Don't Save»; `git status` pulito.

## Le tre tavole

- [`T1-PIE-GEO-BORDO.png`](T1-PIE-GEO-BORDO.png): muro lungo un lato, ghost a tasto premuto, posa e verso inverso.
- [`T2-ghost-diametro-lato-lato.png`](T2-ghost-diametro-lato-lato.png): ghost e posa del diametro lato → lato.
- [`T3-PIE-HEX-PAINT-ERASE.png`](T3-PIE-HEX-PAINT-ERASE.png): prima, dopo *Erase* e dopo `Ctrl+Z`, con il pannello «Mappa Aperta».

## Che cosa NON è coperto

- Il giudizio dell'occhio su tutte e quattro le voci; il movimento del trascinamento (le catture sono fermo-immagine).
- Gli altri cinque lati per `PIE-GEO-BORDO` (solo il lato E), e il `Ctrl+click` sulle sette celle (non ha accumulato: non si sa se per il pilotaggio o per il tool).
- Un binding con la porta come sorgente o che perde solo il bersaglio; il pennello trascinato; `Ctrl+Y`.

## Log e misure

L'ora del log è UTC; l'ora locale è un'ora dopo. Le righe sono estratte da `editor-seduta.log` senza modifiche.

```text
## HexMode / Erase / Arco / Selezione
[2026.10.28-23.17.41:237][544]LogTemp: [HexMode] Selezione (q=0,r=0,L=0) -> cella (q=0,r=0,L=0) (1 elementi).
[2026.10.28-23.18.46:077][ 89]LogTemp: [HexMode] Selezione (q=0,r=0,L=0) -> copertura su (q=0,r=0,L=0) bordo 0 (1 elementi).
[2026.10.28-23.18.49:905][362]LogTemp: [HexMode] Selezione (q=0,r=0,L=0) -> cella (q=0,r=0,L=0) (1 elementi).
[2026.10.28-23.18.53:786][645]LogTemp: [HexMode] Selezione (q=0,r=0,L=0) -> copertura su (q=0,r=0,L=0) bordo 0 (1 elementi).
[2026.10.28-23.19.01:448][120]LogTemp: [HexMode] Selezione (q=0,r=0,L=0) -> cella (q=0,r=0,L=0) (1 elementi).
[2026.10.28-23.19.05:711][418]LogTemp: [HexMode] Selezione (q=1,r=0,L=0) -> cella (q=1,r=0,L=0) (1 elementi).
[2026.10.28-23.20.13:773][243]LogTemp: [HexMode] Selezione (q=0,r=0,L=0) -> cella (q=0,r=0,L=0) (1 elementi).
[2026.10.29-06.34.14:306][879]LogTemp: [HexMode] Arco: nessun pendente da chiudere (chiamata a vuoto).
[2026.10.29-06.34.14:349][879]LogTemp: [HexMode] Arco: From (q=0,r=0,L=0), gizmo spawnato.
[2026.10.29-06.35.19:477][218]LogTemp: [HexMode] Arco: chiusura del pendente — Commit: la transizione e' stata scritta (gizmo vivo).
[2026.10.29-06.36.36:815][303]LogTemp: [HexMode] Arco: nessun pendente da chiudere (chiamata a vuoto).
[2026.10.29-06.38.26:075][207]LogTemp: [HexMode] Erase: 7 celle cancellate, 4 elementi portati via con esse (muri interni, archi, binding).
[2026.10.29-06.39.25:599][375]LogTemp: [HexMode] Selezione (q=0,r=0,L=0) -> muro interno 'MURO-A' (1 elementi).
[2026.10.29-06.41.56:440][872]LogTemp: [HexMode] Selezione (q=0,r=0,L=0) -> cella (q=0,r=0,L=0) (1 elementi).
[2026.10.29-06.42.01:085][243]LogTemp: [HexMode] Selezione (q=0,r=0,L=0) -> muro interno 'MURO-A' (1 elementi).
[2026.10.29-06.42.19:519][431]LogTemp: [HexMode] Selezione (q=0,r=0,L=0) -> cella (q=0,r=0,L=0) (1 elementi).
[2026.10.29-06.43.07:970][ 61]LogTemp: [HexMode] Erase: 1 celle cancellate, 4 elementi portati via con esse (muri interni, archi, binding).
[2026.10.29-06.44.06:947][227]LogTemp: [HexMode] Erase: 7 celle cancellate, 4 elementi portati via con esse (muri interni, archi, binding).
## TRANSACTION UNDO (ogni Ctrl+Z eseguito dall'Editor)
[2026.10.28-23.16.06:613][132]Cmd: TRANSACTION UNDO
[2026.10.28-23.21.54:265][774]Cmd: TRANSACTION UNDO
[2026.10.28-23.21.59:748][ 91]Cmd: TRANSACTION UNDO
[2026.10.28-23.22.17:665][541]Cmd: TRANSACTION UNDO
[2026.10.29-06.29.15:275][528]Cmd: TRANSACTION UNDO
[2026.10.29-06.29.31:313][ 14]Cmd: TRANSACTION UNDO
[2026.10.29-06.30.50:616][567]Cmd: TRANSACTION UNDO
[2026.10.29-06.38.52:987][369]Cmd: TRANSACTION UNDO
[2026.10.29-06.42.35:509][437]Cmd: TRANSACTION UNDO
[2026.10.29-06.43.28:403][612]Cmd: TRANSACTION UNDO
[2026.10.29-06.45.04:577][983]Cmd: TRANSACTION UNDO
## Console
[2026.10.29-06.28.37:828][396]Cmd: rt.Debug.DumpCellPlacement 0 0
[2026.10.29-06.28.37:843][396][RT] 0 muro/i interno/i in questa cella.
[2026.10.29-06.28.37:843][396][RT] Posa in (q=0,r=0,L=0): 0 settori su 12 occupati, centro libero
[2026.10.29-06.28.37:843][396][RT]   settori 0..11  ............   (0 = primo vertice, -30 gradi, verso crescente)
[2026.10.29-06.28.37:843][396][RT]   1 regione/i libera/e, in ordine di FirstWedge crescente:
[2026.10.29-06.28.37:843][396][RT]     FirstWedge= 0  Size=12  settori {0,1,2,3,4,5,6,7,8,9,10,11}
[2026.10.29-06.28.37:843][396][RT]   1 copertura/e di bordo:
[2026.10.29-06.28.37:843][396][RT]     bordo E   tipo High  generata
[2026.10.29-06.29.58:873][309]Cmd: rt.Debug.DumpCert.Debug.DumpCellPlacement 0 0llPlacement 0 0
[2026.10.29-06.30.13:700][646]Cmd: rt.Debug.DumpCellPlacement 0 0
[2026.10.29-06.30.13:701][646][RT] 0 muro/i interno/i in questa cella.
[2026.10.29-06.30.13:701][646][RT] Posa in (q=0,r=0,L=0): 0 settori su 12 occupati, centro libero
[2026.10.29-06.30.13:701][646][RT]   settori 0..11  ............   (0 = primo vertice, -30 gradi, verso crescente)
[2026.10.29-06.30.13:701][646][RT]   1 regione/i libera/e, in ordine di FirstWedge crescente:
[2026.10.29-06.30.13:701][646][RT]     FirstWedge= 0  Size=12  settori {0,1,2,3,4,5,6,7,8,9,10,11}
[2026.10.29-06.30.13:701][646][RT]   1 copertura/e di bordo:
[2026.10.29-06.30.13:701][646][RT]     bordo E   tipo High  generata
[2026.10.29-06.31.36:706][825]Cmd: rt.Debug.DumpCellPlacement 0 0
[2026.10.29-06.31.36:706][825][RT] 1 muro/i interno/i in questa cella.
[2026.10.29-06.31.36:706][825][RT] Posa in (q=0,r=0,L=0): 4 settori su 12 occupati, centro libero
[2026.10.29-06.31.36:706][825][RT]   settori 0..11  ##....##....   (0 = primo vertice, -30 gradi, verso crescente)
[2026.10.29-06.31.36:706][825][RT]   2 regione/i libera/e, in ordine di FirstWedge crescente:
[2026.10.29-06.31.36:706][825][RT]     FirstWedge= 2  Size= 4  settori {2,3,4,5}
[2026.10.29-06.31.36:706][825][RT]     FirstWedge= 8  Size= 4  settori {8,9,10,11}
[2026.10.29-06.31.36:706][825][RT]   coperture di bordo: nessuna.
## Misure di stato (bridge: GetCurrentMap/ValidateTacticalMap/get_properties)
PRIMA | celle: 331 | transizioni: 2 | muri interni: 1 | binding: 1 | porte nella regione: 1 | celle della regione presenti: 7 | Revision: 7996 | valida: True | segnalazioni: []
DOPO | celle: 324 | transizioni: 0 | muri interni: 0 | binding: 0 | porte nella regione: 0 | celle della regione presenti: 0 | Revision: 8003 | valida: True | segnalazioni: []
DOPO 1 CTRL+Z | celle: 331 | transizioni: 2 | muri interni: 1 | binding: 1 | porte nella regione: 1 | celle della regione presenti: 7 | Revision: 7996 | valida: True | segnalazioni: []
PRIMA CANC | celle: 331 | transizioni: 2 | muri interni: 1 | binding: 1 | porte nella regione: 1 | celle della regione presenti: 7 | Revision: 7996 | valida: True | segnalazioni: []
DOPO CANC (cella 0,0) | celle: 330 | transizioni: 0 | muri interni: 0 | binding: 0 | porte nella regione: 1 | celle della regione presenti: 6 | Revision: 7997 | valida: True | segnalazioni: []
DOPO 1 CTRL+Z DOPO CANC | celle: 331 | transizioni: 2 | muri interni: 1 | binding: 1 | porte nella regione: 1 | celle della regione presenti: 7 | Revision: 7996 | valida: True | segnalazioni: []
PRIMA PENNELLO r0 | celle: 331 | transizioni: 2 | muri interni: 1 | binding: 1 | porte nella regione: 1 | celle della regione presenti: 7 | Revision: 7996 | valida: True | segnalazioni: []
DOPO PENNELLO r0 (cella 0,0) | celle: 330 | transizioni: 0 | muri interni: 0 | binding: 0 | porte nella regione: 1 | celle della regione presenti: 6 | Revision: 7997 | valida: True | segnalazioni: []
DOPO 1 CTRL+Z DOPO PENNELLO r0 | celle: 331 | transizioni: 2 | muri interni: 1 | binding: 1 | porte nella regione: 1 | celle della regione presenti: 7 | Revision: 7996 | valida: True | segnalazioni: []
DOPO PENNELLO r1 (bis) | celle: 324 | transizioni: 0 | muri interni: 0 | binding: 0 | porte nella regione: 0 | celle della regione presenti: 0 | Revision: 8003 | valida: True | segnalazioni: []
DOPO 1 CTRL+Z (bis) | celle: 331 | transizioni: 2 | muri interni: 1 | binding: 1 | porte nella regione: 1 | celle della regione presenti: 7 | Revision: 7996 | valida: True | segnalazioni: []
```
