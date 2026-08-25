Lavora la issue #1350 del repo DegrassiAaron/refactor-tactics-main (working dir
D:\Repositories\refactor-tactict-dev). Leggi la issue e i suoi due commenti: contengono
la ricostruzione completa e le misure già fatte, non rifarle da zero.

CONTESTO GIÀ VERIFICATO (misurato il 2026-08-25, riverifica solo che regga ancora)

Sei file vivono in due copie, e quelle in docs/roadmap/plans/ sono le PIÙ VECCHIE:
  roadmap-lane-index.md · roadmap_lane_1.md … roadmap_lane_5.md
contro docs/archive/roadmap-plans/, che è l'owner e ha la versione riallineata
(intestazione "HEAD: 59fa6f8a (riallineato al merge)" contro "HEAD: 3c4e48e").
Differenze fino a 89 righe: roadmap_lane_4.md ha 98 righe in plans/ e 155 in archivio.

Come è successo: 6088350b (2026-08-14) SPOSTA i file in archivio con un rename; il
merge di #909 (8880b3cd, 2026-08-15) li RI-AGGIUNGE in plans/, perché il branch
wip/icon-visual-language era anteriore all'archiviazione e git non vede conflitto fra
un rename e un'aggiunta dello stesso path. Le lane 6 e 7 non sono tornate perché quel
branch non le aveva.

Controlli già fatti, da NON ripetere:
- 21 file spostati da 6088350b, 6 doppioni reali, gli altri 14 non sono tornati.
  (README.md risulta omonimo ma è un falso positivo: plans/README.md e
  archive/roadmap-plans/README.md sono due indici distinti e legittimi.)
- Link entranti: nessun documento owner cita le copie attive. Le nominano solo
  quattro-processi-paralleli-triage-2026-08-14.md (che le dichiara inesistenti) e
  walls-doors-interaction-spec-panel-2026-08-13.md, che le tratta come foto storiche.

DA FARE

1. Rimuovere i sei file da docs/roadmap/plans/. L'archivio resta l'owner e non si tocca.
2. Correggere in docs/roadmap/plans/quattro-processi-paralleli-triage-2026-08-14.md la
   riga che afferma «Nessuno di quei path esiste» (§3, intorno a riga 69 — riverifica il
   numero). Era vera il 14 agosto e falsa dal 15: va detto cosa è successo davvero, cioè
   che un merge del giorno dopo li ha resuscitati, non che il triage avesse sbagliato.
3. Verificare che nessun link markdown si rompa con la rimozione — a mano, aprendo i
   target: scripts/check-docs-links.py NON esiste più (D-182, git ls-files scripts → 0).

PERCHÉ NON È COSMETICO, e vale la pena scriverlo nella PR: le copie attive portano i
nomi legacy del roster (Flux, Riva, Bastion, Vektor) dove l'archivio ha quelli correnti
(Gadget, Phase, Riktor, Wraith). D-130 li ha esclusi dal repository e D-182 ha rimosso
il gate che li segnalava, quindi restano finché qualcuno non li toglie a mano.

VINCOLI
- Branch dedicato da main, PR verso main, niente commit diretti su main.
- Prima di iniziare: git fetch --prune origin, git status, gh pr list --state open.
  main si muove (altre sessioni lavorano sugli stessi documenti): riverifica che i sei
  doppioni esistano ancora prima di agire.
- Solo Markdown: nessuna build, nessuna suite. Se il diff dovesse toccare codice, fermati
  e chiedi — significa che lo scope è cambiato.
- Chiudi la issue a mano dopo il merge: le keyword italiane («Chiude #1350») non
  attivano la chiusura automatica di GitHub.

Due note che non ho messo nel prompt perché valgono solo se il lavoro si allarga:

Se la sessione nuova finisce presto e vuoi darle un seguito, i candidati editor-free già istruiti sono #1096 (le sei voci PIE del contratto graybox — gemella di #1242 chiusa stanotte, stesso file e stesso formato) e #1232 parte 1 (puntatori morti a docs/technical/, ancora presenti in .gitignore, Source/RTPlayerController.* e piano-riduzione-hotspot.md; le parti 2–3 sono morte con gli script).

E se dovesse servire il toolchain Unreal per qualsiasi motivo: su questa macchina la build richiede -NoHotReloadFromIDE, perché un processo UnrealEditor zombie (PID 53736, un thread, nessuna finestra) tiene il mutex Live Coding. È già annotato nella memoria di progetto, quindi la sessione nuova lo trova da sola.