# RefactorTactics — Stato consolidato del focus “griglia, muri, acqua/elettricità e Planning HUD”

**Data audit:** 2026-08-12  
**Base verificata:** repository `DegrassiAaron/refactor-tactics-main` + spec CP8.3 fornita nella conversazione.

## Riassunto

La chat è partita dalla relazione fra **griglia esagonale e geometria architettonica** e si è estesa fino a
cover/LOS, traversal, collisioni simultanee, verticalità, strutture, acqua e infine UI di planning.

Il principio più stabile è che l'hex è **topologia tattica**, non geometria costruttiva: muri e aperture non
sono obbligati sui bordi degli esagoni. Il runtime non deve interrogare la mesh per decidere; usa un bake
deterministico su celle/archi/profili.

Una correzione importante rispetto alle prime decisioni della chat è già stata fatta dal repository:
**D-071** supersede il test “muro sul centro esatto”: la standability usa il footprint/cerchio inscritto.

Sul lato acqua/elettricità la baseline CP8.3 è già concreta: BFS sulle celle conduttive, limite in passi,
conductivity della cella e non di `Wet`, danno iniziale/propagato distinti, evento istantaneo. CP9.4 ha già
aggiunto **archi conduttivi fra Layer**. Quindi il futuro sistema va descritto come evoluzione del grafo
esistente, non come un secondo motore.

Le decisioni nuove di questa chat estendono questo modello: superfici/oggetti/strutture conduttive possono
fare da ponte, un connector può avere più rami, una struttura lunga deve consumare più passi, nessun salto
automatico nell'aria. Un item conduttivo può ampliare la zona di Chain Lightning. Un eventuale hazard
elettrico persistente è distinto dalla scarica standard, rivaluta la topologia al proprio activation boundary
e può avere trigger `Pulse/OnEnter`.

Seconda correzione importante: **D-081** ha deciso che la futura profondità dell'acqua è una **superficie
composta**, non un campo `WaterDepth` ortogonale. Le parole “Shallow/Deep/Impassable” restano utili come design,
ma l'implementazione non deve introdurre l'asse parallelo senza superseding decision.

Sul Planning HUD è stata fissata una grammatica contestuale: la mappa enfatizza lo **step selezionato**,
mantiene ghosted quelli già pianificati e nasconde/minimizza i futuri. Semantic baseline:
Move green, Attack red, Utility blue, Control amber, Overwatch/Reaction purple, Post turquoise, Electric blue.
Il colore non basta: pattern/outline/icon sono obbligatori per leggibilità/CVD.

Per Chain Lightning la preview deve spiegare primary reach, chain reach e l'estensione causata da acqua,
metallo, item o connector. La UI non deve disegnare un raggio finto: deve usare la stessa query del resolver
sullo snapshot e sulla conoscenza autorizzata. Hidden enemy Overwatch/intent non può modificare overlay o
threat map e quindi non può leakare.

## Stato vs futuro

**Già presente nel repository**
- hex multilayer come unico substrato;
- CP8.3 electric BFS;
- dynamic surfaces CP8.4;
- conductive arcs / multilayer conductivity CP9.4;
- HUD/overlay e feature Planning già esistenti;
- Feature Registry e mappe generate;
- Scenario Map e Editor Map con owner definiti.

**Design consolidato ma da specificare/implementare**
- generalized conductive connector multi-port;
- long-conductor segmentation;
- persistent electric hazard;
- future Deep/Impassable water composite surfaces/current;
- visual grammar completa della Planning Preview e parity test;
- UI explanation dei conductive bridge/branches.

## Governance

Non aggiornare “tutte le copie” a mano:
- feature source = `feature-registry.yaml`;
- editor source = `editor-sessions.yaml`;
- shortlists/JSON/wiki derivate = generate;
- Wiki = spiegazione;
- cataloghi Markdown = numeri;
- Balance workbook v0.1 = RESEARCH;
- Character Wiki workbook = character authoring, non owner delle regole ambientali/UI.
