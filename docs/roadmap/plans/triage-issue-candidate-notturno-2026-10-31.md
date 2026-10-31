# Triage delle issue candidate per un lavoro notturno autonomo (2026-10-31)

> **Referto.** Misurato su `origin/main` = `115d40cff` (merge di #3947). Lo ha prodotto una sessione assistita da agenti **in sola lettura**: ogni verdetto è una lettura dell'issue, dei suoi commenti e del codice a quello sha, **non una decisione**. Chi riprende un'issue rimisura la premessa, perché le premesse decadono in ore (`AGENTS.md` §8). Le issue aperte si contano con `gh issue list --state open`: qui non ci sono totali. Le due fette adatte sono consegnate nelle PR #3948 (#3134) e #3949 (#3035).

## Perché esiste

Una notte senza nessuno che risponda a domande di design vuole issue che si possano implementare, verificare in headless e consegnare **senza una decisione d'autore**. Questo referto dice, per le candidate esaminate, quali lo sono e quali no, e **cosa serve per sbloccare le altre**: chi riparte non deve rifare lo stesso giro.

## Criteri

* **Stato su main**: il difetto o la mancanza c'è ancora? (`già fatta`, `in parte`, `non fatta`, `premessa falsa`).
* **Contratto**: il comportamento atteso è fissato da una D-nnn accettata, un ADR o un'owner spec (`deciso`), oppure implementare vorrebbe dire scegliere una semantica competitiva (`serve decisione`, `CLAUDE.md` §13)? `in parte deciso` = una parte è fissata e una no: la cella dice quale.
* **Verificabile in headless**: gli acceptance criteria si dimostrano con Automation o con tool in `tools/`, senza Editor, PIE, asset binari o giudizio umano?
* **Fetta con consumatore**: una tabella o un campo letto solo dal proprio test è il difetto ricorrente del repository, e non basta.
* **Collisione con #3748**: la sessione che lavora su barra, scheda e ispezione tocca `Player/RTPlayerController.*`, `UI/**` e i relativi test.
* Le sigle nelle celle (`STA-4`, `SKB-1`, `DR-12`, `Q4`, `PIE-V01-*`) sono identificatori del progetto (decisioni, domande aperte, voci del registro PIE), non etichette del triage.

## Esito

Adatte, con condizioni scritte per esteso nella tabella: **#3134** (primo punto di D-414: l'azione annullata da un Interrupt paga il cooldown) e **#3035** (il ponte scenario -> archivio replay, opt-in). Sono consegnate in PR a parte, #3948 e #3949, ciascuna con la propria evidenza. Tutte le altre sono `no`, con il motivo.

## Le decisioni che sbloccano più lavoro

Secondo il triage, e da confermare da chi decide:

* **Una fase o due per le ambientali** (#3200). Il catalogo dichiara due fasi (40/50), il codice ne costruisce una. Se la risposta è una sola, la fetta è documentale e piccola.
* **Il prezzo di Guard e Brace** (#3940, con #617). La scelta A/B è rimandata dall'autore alla seduta U20; se sarà A, è un cambio di gameplay con misure da rifare sul catalogo.
* **Il framework degli status (E36).** La decisione sul modello di scadenza (#437) e quella sulla tassonomia (#436, STA-4) sono prerequisiti, non sufficienti. #437 dichiara nel proprio corpo di essere indipendente da CP 36.1 e di poter procedere in parallelo, ma gli serve una D-nnn sul modello di scadenza; #439 aspetta la tassonomia (#436) e la derivazione di #438; #440 vuole in più D-nnn su resistance/immunity e reapply policy, e l'apertura dell'epic E36 dopo i gate v0.1; #441 vuole prima la riscrittura della premessa sul TurnLog reale. Le righe della tabella danno il dettaglio.
* **Rumore -> contatto incerto** (#159): categoria di log acustica, una nuova versione del formato del TurnLog (la prima libera: l'ultima oggi è `WithSightBlocker`; si verifica con `gh pr list` prima di rivendicarla), contenitore del contatto, durata della memoria sonora.
* **Celle di soggetto e forma del log** (#2148, #1392, #2492): la cella del tiratore al colpo predittivo, la cella del verdetto di Move, la forma della traccia. Per #2492 l'ultimo commento indica un percorso meccanico (dare una causa esatta a `ERTLinearStop`), quindi la decisione ipotizzata potrebbe non servire.
* **Cleanse contro Slow e Root** (#1479): la riga della tabella dice cosa manca (un Cleanse attivo raggiungibile, o un oracolo diverso). Non è una decisione che sblocchi lavoro da sola. **Cover window** (#1561, #1562) e **`EvaluationBoundary`** (#1577) sono post-v0.1 e dichiarate non pronte.

## Dopo il merge di #3748, ma ciascuna con le sue condizioni

Il merge di #3748 libera i file HUD e Player; da solo non basta. Per ognuna la tabella dice il blocco che resta:

* #2281: la fetta piccola e headless (`bMapDeclaresObjective` in `FRTMatchHeaderView`) tocca file di #3748; resta la scelta "flag dentro o accanto alla vista", e la via A aspetta un dato che il viewer di replay non ha ancora.
* #2182: il DTO d'ingresso di `RTHudViewModel` è una decisione d'autore ancora aperta; il triage non vede una fetta sicura.
* #3145: dopo #3748 resta da capire cosa manca, e c'è un arco di spec (Targeting -> Pathing) proposto e non deciso.
* #3420: DR-12 e Q4 sono aperte; il primo passo possibile è la funzione pura `ArrivalFacing`, ma senza consumatore non è un deliverable.
* #3764: contratto deciso, ma D-537 la rinvia a v0.2 e restano da chiudere con l'autore le questioni `BlocksTraversal`, simmetria, TurnLog dei piani e facing.
* #1821: da riaprire dopo #2184 e dopo il merge di #3748.

## Residui che vogliono una seduta umana, l'Editor o un'altra issue

Per queste il codice verificabile in headless è già su main, e resta un verdetto a schermo, un'esecuzione in PIE o un asset binario: #1466, #2836, #2916, #3107, #79, #2297 (la fetta sugli `.uasset`). Il triage distingue cinque eccezioni:

* #80: il lato testuale c'è, ma `rt.Debug.DrawPaths`, `DrawCover` e `DrawResolution` stampano e non disegnano (lo dichiara `Debug/RTDebugConsole.cpp`): il lavoro grafico non è fatto.
* #84: serve una decisione sulla sede versionata dell'evidenza e un riallineamento del corpo all'attestazione #85, non una seduta.
* #291 e #3063: il codice è su main, ma il residuo dipende da altre issue aperte (#172 e #1793 per la prima, #3065 per la seconda) e vuole PIE.
* #2501: il difetto è corretto (PR #2557); resta una suite headless valida su main, che sola la chiude.
* #82: servono sedute Editor/PIE umane, ma l'issue è P2 e non blocca la release.

#2566 è un contenitore: le sue fette sono altre issue, per esempio #2576 e #2577 (aperte); #2578 e #2579 risultano chiuse.

## Tabella per issue

| Issue | Titolo (breve) | Stato su main | Contratto | Adatta a una notte autonoma | Perché / cosa serve |
|---|---|---|---|---|---|
| #3035 | Corpus replay-ready: ponte scenario -> archivio | in parte | deciso | adatta, con condizioni | Consegnata nella PR #3949 (il ponte). (1) L'opt-in deve avere almeno un chiamante non-test (es. flag nella console rt.Test.Scenario / RunById), altrimenti è il difetto 'consumatore solo il proprio test'. (2) Usare un FormatId già registrato (es. URTMatchFormatLibrary::Skirmish2v2FormatId, come RTReplayRecordingIntegrationTests.cpp) e dichiararlo nel commento, senza nuove D-nnn; verificare con grep che nessun lettore lo confronti con le unità dello scenario. (3) Non aggiungere MatchId a nulla che entri in hash/digest. |
| #3134 | D-414: un'azione annullata da Interrupt paga il cooldown | non fatta | deciso | adatta, con condizioni | Consegnata nella PR #3948 (primo punto di D-414). 1) Prima di scrivere: git grep -il interrupt su Scenarios/ e sui golden deve restare vuoto (a 115d40cff lo è: nessun scenario o golden contiene Interrupt), poi lanciare la suite golden/replay e dichiarare il commit. 2) Non toccare il Decision Log (sporco nell'altra sessione): annotare D-414 solo in un follow-up. 3) La PR non chiude #3134: usare 'Refs'. 4) Scegliere esplicitamente 'vivo quando annota' (stessa guardia dell'Interrupter) e scriverlo nel commento di codice. |
| #79 | CP 11.3 Combat log con reason code completi | già fatta | deciso | no | Serve una seduta Editor/PIE umana per rimisurare PIE-V01-COLL (c); la chiusura dell'issue è una decisione dell'autore. |
| #80 | CP 11.4 Comandi rt.Debug.* | in parte | in parte deciso | no | Serve una seduta U15 in PIE per la voce PIE-V01-DEBUG; l'overlay richiede verifica a schermo. Attenzione: `rt.Debug.DrawPaths`, `DrawCover` e `DrawResolution` stampano e non disegnano (lo dichiara `Debug/RTDebugConsole.cpp`): il lavoro grafico non è fatto. |
| #82 | CP 12.2 Matrice test manuali v0.1 | in parte | deciso | no | Servono sedute Editor/PIE umane; l'issue è P2 e non blocca la release. |
| #84 | CP 12.4 KPI misurati e registrati (G11) | in parte | serve decisione | no | Serve la decisione dell'autore sulla sede versionata dell'evidenza e un riallineamento del corpo all'attestazione #85. |
| #149 | Bilanciamento bot: scala WThreat vs WDamage | in parte | serve decisione | no | Bloccata da D-102 (competence gate #543, chiusa il 2026-09-20: non è un blocco da aspettare) e dall'E43 batch runner #776 (v0.8). Solo un'eventuale correzione di prosa/ratchet sarebbe fattibile, ma non è un deliverable dell'issue. |
| #159 | CP 13.4 rumore -> contatto incerto, vista per squadra | non fatta | serve decisione | no | Decisioni d'autore su categoria di log, una nuova ERTTurnLogFormatVersion (la prima libera: l'ultima oggi è `WithSightBlocker`; si verifica con `gh pr list` prima di rivendicarla), contenitore del contatto acustico e N di scadenza; poi spezzare in tre PR (emissione, filtro, viste). |
| #160 | CP 13.5 Bot e HUD su conoscenza parziale | in parte | serve decisione | no | Prima deve chiudersi #159 (emissione di rumore, filtro d'uscita, FRTAcousticContact). |
| #219 | CP 20.2 categorie icone v0.1 | in parte | serve decisione | no | Decisione d'autore su quali chiavi Warning entrano nell'insieme richiesto. Asset e catalogo ricostruito nel clone principale con l'Editor. |
| #291 | Facing finale col secondo click sulla destinazione | in parte | deciso | no | Dipende da #172 (overlay) e #1793; PIE obbligatorio. |
| #314 | CP 14.7 Reaction Profile e Reaction Clash | in parte | in parte deciso | no | Prima serve una decisione d'autore: tenere la macchina senza produttore, oppure aspettare un eroe con almeno due profili (BAS-1 / D-127 ha chiuso il roster con un solo decisore). Il lavoro andrebbe poi in sessione diurna con la suite intera. |
| #319 | CP 14.8 Decision Time Bank | non fatta | in parte deciso | no | Bloccata da #166 (misura del pacing, CP 14.6) per la taratura, dal soggetto 'giocatore' sopra ARTUnit e dalla decisione su Space/Quick Confirm. Il formato TurnLog cambia: servono versione e golden. |
| #437 | CP 36.2 dato dello status (categoria, polarità, stacking, durata) | non fatta | in parte deciso | no | Prima serve una decisione (D-nnn) che dica come si rappresenta expiration (boundary + conteggio) e dove vive il dato. Poi l'issue va riscritta sugli status attuali. Il corpo dichiara di essere indipendente da CP 36.1 e di poter procedere in parallelo. Con quella decisione diventa adatta con condizioni, taglia M. |
| #438 | CP 36.3 sei primitive derivate dal dato | non fatta | serve decisione | no | Bloccata da STA-4 (#436), poi #437. Se ne riparla quando entrambe sono in main. |
| #439 | CP 36.4 severity contata e anti-stun-lock come test | non fatta | serve decisione | no | Bloccata da STA-4/#436 e dalla derivazione di #438. Va riletta l'issue includendo Unbalanced, Prone e Stunned, che oggi portano gli status oltre quelli citati dall'issue. |
| #440 | CP 36.5 resistance/immunity/cleanse per categoria | non fatta | serve decisione | no | Bloccata da #436 e #437 (OPEN) e dall'apertura dell'Epic E36 dopo i gate v0.1; servono D-nnn sul modello di resistance/immunity e sulla reapply policy. |
| #441 | CP 36.6 reason code, TurnLog e validator degli status | in parte | in parte deciso | no | Bloccata da #437 e #439 (e a monte da #436). Prima va riscritta la premessa sul TurnLog reale. Un bump di formato tocca un contatore condiviso e i golden. |
| #464 | Eroe con cura/scudo self-target (ramo difensivo bot) | non fatta | serve decisione | no | Decisione d'autore su quale azione/eroe diventa self-target Heal e sul criterio di ripetizione del ramo; E26/E35 post-v0.1. |
| #542 | Seam DecisionProvider unico per l'harness (D-101) | non fatta | in parte deciso | no | Prima: un D-nnn che fissi la forma del provider e la release (v0.2 vs v0.1), con spec-panel dedicato; poi una fetta tipo 'tipo + provider scriptato' con mutazione. |
| #701 | CP 39.3 Short Blink: targeting, validazione, Planning | non fatta | serve decisione | no | Serve la decisione d'autore su (a) vs (b) e sul destino di LinearLeap (D-560), più un D-nnn che la registri; dipendenza dal punto di validazione di #605 e #700. |
| #924 | Scatto e waypoint insieme: il dash cancella il movimento | in parte | serve decisione | no | Decisione d'autore su impedire/spiegare/far funzionare; PIE. |
| #1218 | Traccia di RebuildInstances in un posto solo (OnConstruction) | non fatta | serve decisione | no | Editor interattivo obbligatorio; decisione dell'owner su criterio di soppressione e doppio rebuild. |
| #1239 | D-179 geometria dentro la cella | già fatta | deciso | no | già fatta: conservazione, hash e formato sono su main e il resto del DoD è stato superato da D-285/D-554; serve solo che l'autore chiuda l'issue. |
| #1246 | Volumi di blocco indistinguibili a picco | non fatta | serve decisione | no | Serve sessione Editor/PIE (PIE-HEX-VIZ-BLOCCHI) e la scelta del canale; RTHexMapActor.cpp è file molto toccato (rischio conflitti). |
| #1261 | Elenco chiuso dei tag degli scenari | non fatta | in parte deciso | no | L'autore dichiara dove vive il controllo (test del corpus, loader o entrambi) e se l'elenco è congelato sui tag attuali o da rivedere (riktor/aevik, d-nnn, numerici). |
| #1392 | Colpo di boundary rispetta la copertura e nessuno lo dice (traccia, log, rotta) | in parte | serve decisione | no | Decisione d'autore sulla forma (parametro d'uscita su BoundaryCoverReduction vs nuova funzione) e verifica dei golden replay. |
| #1402 | Back non annulla lo scatto pianificato | non fatta | serve decisione | no | Decisione d'autore sull'ordine Dash/Waypoint/Declaration e un D-nnn; ApplyBack è in lavorazione dall'altro terminale. |
| #1466 | E13.6 conoscenza parziale visibile (porta, leak, sagoma) | già fatta | deciso | no | Esecuzione di PIE-KNOW-POSA da parte dell'autore; poi spuntare le caselle T3-T6 e chiudere l'issue (non da fare di notte). |
| #1479 | Slow/Root non contrastabili dalla Cleanse (durata) | non fatta | serve decisione | no | Decisione d'autore su Root/Slow a 2 turni (D-nnn nuova), e un Cleanse attivo raggiungibile o un oracolo diverso; poi rigenerare i golden. |
| #1561 | CP 22.1 copertura con stato temporaneo e durata nei dati | non fatta | serve decisione | no | Serve una D-nnn d'autore sul modello Cover Window (stati, durata, chi apre/chiude, formato TurnLog/hash) e il via libera sul vincolo gate v0.1 dell'epic #323. |
| #1562 | CP 22.2 LOS rivalutata al Decision Boundary | non fatta | serve decisione | no | Prima #1561 con la sua decisione d'autore; poi suite intera del resolver (AGENTS.md §9). |
| #1569 | CP 27.4 illuminazione cella e SightBudget | non fatta | serve decisione | no | Revoca del vincolo dell'epic #327; decisione d'autore sul modello di luce (sorgenti, penalità, stato competitivo o no). |
| #1577 | CP 33.1 EvaluationBoundary in FRTActionDef | non fatta | in parte deciso | no | Revoca esplicita del vincolo dei gate v0.1 da parte dell'autore, #225 chiusa, insieme chiuso dei boundary specificato. |
| #1821 | Meno file includono RTTurnManager.h per leggere il turno | in parte | in parte deciso | no | Riaprire dopo #2184 e dopo il merge di #3748; prima fissare nel tool il calcolo delle righe consumatore, come l'issue stessa chiede. |
| #1902 | Compagno bot: nessun peso premia la visione (WVision) | in parte | serve decisione | no | Seduta PIE di #1896 con alleato guidato da persona; diagnosi della causa fra le sei candidate; poi scelta d'autore A/B/C/D. |
| #2089 | Gadget.Insulator: passivo, immunità per categoria | non fatta | serve decisione | no | Prerequisito: E36 (#435) con CP 36.1-36.5 consegnati. Poi: Insulator cooldown 3, test 'una propagazione si', 'la successiva no', DefaultLoadoutFor invariato. |
| #2148 | Celle di soggetto: tiratore predittivo e verdetto Move | non fatta | serve decisione | no | Decisione d'autore su (1) cella del tiratore al colpo predittivo e (2) cella del verdetto della voce Move. |
| #2149 | T8 dello showcase: KO di Gadget e seconda rotta di Phase | non fatta | serve decisione | no | Serve prima la scelta d'autore sul T7 (modello §T3 oppure due turni), poi T5, poi T8, come raccomanda l'istruttoria 2149-t7-combo-ambientale. Nota: nel testo si dice Riktor, ora Hero.Branth (D-334). |
| #2182 | Allestimento: ridurre gli spawn di Actor nei test di setup | in parte | serve decisione | no | Riprendere dopo il merge di #3748 e dopo la scelta del DTO d'ingresso; la misura di apertura va rifatta con misure-strutturali.py (non usare i numeri del corpo). |
| #2281 | Punteggio d'obiettivo nella vista senza la condizione | in parte | serve decisione | no | Attendere chiusura/merge di #3748 sui file HUD e decisione d'autore su 'flag dentro o accanto' alla vista (dipende da cosa consuma il viewer di replay). |
| #2291 | CP 35.5 Paragon naming purge e gate | non fatta | in parte deciso | no | Serve prima definire i campi sorvegliati e il rapporto con la allowlist temporanea; la parte abilità richiede una fonte esterna di titoli Paragon. |
| #2297 | Migrazione identità roster v0.1 (fette A-F) | in parte | deciso | no | Editor + Fix Up Redirectors, oracolo PIE umano, e fetta F dipende da #2291 (gate inesistente). |
| #2405 | Anteprima della minaccia di caduta in pianificazione | non fatta | serve decisione | no | Serve prima che il playtest bocci la caduta senza anteprima (D-535) e che #3748 sia mergiata. Poi una decisione sulla forma e un esame privacy della proiezione. |
| #2411 | Playback: il mover corto che arriva prima | non fatta | deciso | no | Serve una seduta U44 con una persona (e U21 per l'illuminazione). Va convocata prima di un'eventuale adozione di #2902. |
| #2492 | Visual.Environment.Acceptance: log che non nomina la causa (conduzione elettrica, fumo) | in parte | serve decisione | no | Decisione d'autore su come il log nomina conduzione e fumo. Il dubbio sulla privacy è un'inferenza dell'agente di triage e non sta nell'issue; l'ultimo commento indica un percorso meccanico (dare una causa esatta a `ERTLinearStop`), quindi la decisione ipotizzata potrebbe non servire. |
| #2501 | Move ricalcola verso la destinazione dopo spinta (Model B) | già fatta | deciso | no | già fatta: il difetto è stato corretto da PR #2557 e le caselle di DoD sono spuntate sul sorgente; resta aperta solo per una misura di suite che non produce una PR. Chiuderla è un'azione GitHub, non lavoro notturno. |
| #2566 | BAL 0.1: la variante misurata (stadio di capability) | in parte | in parte deciso | no | Chiusa dall'owner come capitolo ma riaperta/segnata OPEN: non lavorare finché la milestone non parte. |
| #2576 | TD 0.4: diff baseline<->variante su più scenari | non fatta | in parte deciso | no | Riconsiderare solo quando #2577 definisce il consumatore o l'owner fissa l'insieme minimo di campi del diff. |
| #2657 | Flaky TwoCountersKeepTheirOwnOrigin: autore sbagliato (ipotesi UAF su FRTBlastContext) | in parte | serve decisione | no | Serve la decisione dell'autore sul criterio 4; la diagnosi richiede un rosso strumentato mai osservato in 40 run. |
| #2827 | Targeting diretto di strutture: TargetKindForAction non produce mai Object | non fatta | in parte deciso | no | Attendere che l'altra sessione liberi RTPlayerController/RTPointerInteraction e che INT-9/INT-10 siano decise (per le porte). |
| #2836 | Riga di trasporto 'corsa eseguita' su corsa BLOCCATA / memoria corsa alla deselezione | già fatta | deciso | no | Serve la seduta U44 con un umano e il bridge MCP del clone principale. |
| #2916 | PIE playback controls voce di registro | in parte | deciso | no | Una persona in una seduta U23 con l'Editor libero, con le variabili impostate prima di premere Play. |
| #3063 | Click su cella mai osservata raggiunge il targeting | in parte | deciso | no | Serve PIE (Editor) e la risoluzione di #3065; tocca RTPlayerController.cpp. |
| #3107 | Allestimento ripetibile del rifiuto per copertura | già fatta | deciso | no | Candidata solo alla chiusura/riallineamento da parte dell'owner, non a implementazione. |
| #3130 | D-407/D-409: slot SignatureDefense e famiglia Evasion | in parte | in parte deciso | no | Prima serve decidere come il piano rappresenta un'Evasion (principale che risolve in Dash); poi attendere che l'altra sessione liberi RTPlayerController/HUD. Il blocco SKB-1 è caduto (D-416). |
| #3131 | D-410: famiglia Reflect (Rimando) | non fatta | serve decisione | no | Prima #3130; poi la risposta d'autore a SKB-6. |
| #3133 | D-413: sequenza di Interact rispetto al percorso | non fatta | in parte deciso | no | Serve una decisione tecnica d'autore sul punto di esecuzione post-Move; la riga sullo stordimento ora ha il suo soggetto (D-416, #3142 chiusa). |
| #3145 | Selezione da tastiera, dichiarazione per unità | in parte | in parte deciso | no | Attendere la chiusura di #3748; decisione d'autore sull'arco Targeting->Pathing. |
| #3200 | Catalogo 40/50 ambientali vs codice Environment | non fatta | serve decisione | no | Decisione registrata: le ambientali risolvono in una fase o in due? Poi (a) è meccanica. |
| #3420 | Anteprima della vista dalla cella d'arrivo | non fatta | in parte deciso | no | Attendere la chiusura di #3748; decidere DR-12 e Q4; PIE umana. Dopo, la funzione pura ArrivalFacing è il primo passo candidato. |
| #3740 | Tabella nomi azioni core (rinviata da #3629) | non fatta | serve decisione | no | Un lettore nominato nell'issue, i nomi approvati dall'autore in un commento e la sessione HUD chiusa. |
| #3764 | Carica/scatto/passaggio attraverso i piani per archi | non fatta | deciso | no | Ripresa possibile dopo il freeze v0.1 con #3748 mergiata; chiudere prima le questioni BlocksTraversal, simmetria, TurnLog layer e facing con l'autore. |
| #3940 | Drift D-205/D-207/D-208: Guard e Brace allo stesso prezzo | non fatta | serve decisione | no | Blocco: scelta A/B dell'autore in U20. Se un giorno sarà A, il lavoro sarà un cambio di gameplay con invalidazione delle misure sul catalogo attuale. |

## Limiti di questo referto

* Le righe sono letture di agenti, verificate a campione e non una per una: due revisori indipendenti hanno controllato i fatti di quindici righe e le tre sezioni scritte a mano, e la forma; i loro rilievi sono corretti qui. Un `no` dice "non oggi, non senza chi decide", non "mai".
* Le candidate sono state scelte da una graduatoria euristica sui titoli e sui corpi: non sono tutte le issue aperte. Nella prima graduatoria erano escluse per regola le epic e le issue con label `checkpoint`; sono state riammesse a mano quelle post-v0.1 che sembravano autonome.
* Il triage non ha chiuso né modificato nessuna issue.
* Il referto invecchia con `main`: lo sha è in testa, e ogni cella va rimisurata prima di agire.

