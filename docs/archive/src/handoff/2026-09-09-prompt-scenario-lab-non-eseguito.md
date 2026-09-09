> `ARCHIVE` · **Prompt conservato, NON eseguito.** Ricevuto il 2026-09-09, sottoposto a spec panel e
> respinto. Sta qui per la stessa ragione per cui ci sta
> [`scenario-harness-task-originale.md`](scenario-harness-task-originale.md): *«un prompt non è una
> specifica: descrive ciò che si voleva provare a costruire, non ciò che è stato costruito»*.
>
> ## Perché non è stato eseguito
>
> Chiedeva di creare dieci milestone `Scenario Lab v0.1…v1.0`, un'epic ombrello, dieci release epic e le
> relative issue figlie. Il panel (Wiegers, Adzic, Cockburn, Fowler, Nygard, Crispin, Hightower) ha
> trovato tre difetti che lo rendevano ineseguibile **come scritto**:
>
> 1. **La baseline non è ferma.** Definisce la parità contro «la baseline v0.1» e chiede di congelarla.
>    Misurato: la milestone `v0.1 — Offline Vertical Slice` era ancora aperta. Non esisteva una v0.1 da
>    congelare.
> 2. **Il controllo duplicati proposto non trova il duplicato.** Cercava `Scenario Lab`, `map editor`,
>    `parity matrix`. Nessuno di quei termini trova [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105),
>    *«Tactical Designer — un solo loop fra mappa, skill e scenario»*, che copriva `v0.1`–`v0.4` e `v0.6`
>    del prompt.
> 3. **Sarebbe stato il terzo nome** per la stessa superficie, dopo *Tactical Designer* e *Scenario
>    Composer*, contro `spec-tactical-designer.md` §2.
>
> ⛔ E [`roadmap-balance.md`](../../../roadmap/roadmap-balance.md) §11 elenca come **primo** rischio
> aperto il *«backlog parallelo: quarta ricomparsa della stessa proposta»*. Questo prompt sarebbe stato la
> quinta.
>
> ## Cosa ne è uscito invece
>
> La discovery che ha sostituito l'esecuzione ha prodotto **quattro issue mirate** invece di ~81
> artefatti, e due sono già chiuse:
> [#2786](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2786) (il piazzamento delle unità,
> ✅) · [#2789](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2789) (il Composer si
> ritira, ✅) · [#2788](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2788) ·
> [#2802](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2802).
>
> 🔑 **Cosa del prompt regge, e vale la pena rileggere**: la piramide di test
> `pure → headless → engine → Editor → packaged` e la regola *«non richiedere un test engine quando un
> test pure dimostra lo stesso comportamento»*. Quella parte è buona, ed è indipendente dal backlog che
> il resto proponeva.
>
> Referti: [`match-lab-discovery-spec-panel-2026-09-09.md`](../../../roadmap/plans/match-lab-discovery-spec-panel-2026-09-09.md)
> · [`pie-scen-composer-ordine-del-giorno-2026-09-09.md`](../../../roadmap/plans/pie-scen-composer-ordine-del-giorno-2026-09-09.md)

---

# Prompt per Claude Code — creare roadmap e issue di Scenario Lab fino alla v1.0

Incolla integralmente il testo seguente in Claude Code aperto sul repository `DegrassiAaron/refactor-tactics-main`.

---

## RUOLO

Agisci come Technical Product Owner, Lead Game Designer e Senior Unreal/Tools Engineer del progetto **Refactor Tactics**.

Devi organizzare e creare in GitHub il backlog completo di **Scenario Lab**, uno strumento 2D con griglia esagonale per:

- creare e modificare mappe e stati iniziali;
- posizionare personaggi, oggetti, terreno, hazard e obiettivi;
- pianificare le azioni di uno o più turni simultanei;
- eseguire il resolver reale del gioco;
- salvare il risultato canonico di un turno come stato iniziale del successivo;
- riprodurre il turno tramite event log e replay;
- permettere scenari incompleti o errati come bozze;
- rilevare, spiegare, localizzare e correggere errori senza perdere il lavoro;
- raggiungere nella v1.0 la parità verificata con tutte le abilità, gli stati e i comportamenti effettivamente supportati dal runtime di Refactor Tactics v0.1.

## VINCOLO OPERATIVO ASSOLUTO

In questa attività devi creare **soltanto artefatti GitHub di pianificazione**:

1. milestone;
2. epic rappresentati come GitHub issue;
3. issue figlie;
4. collegamenti/checklist tra le issue appena create.

Non devi:

- modificare codice o documentazione del repository;
- creare file nel repository;
- creare branch, commit o pull request;
- implementare feature;
- eseguire refactor, formattazioni o correzioni;
- creare o modificare GitHub Actions;
- creare label nuove;
- cambiare issue esistenti, salvo aggiungere un riferimento non distruttivo quando indispensabile per evitare un duplicato;
- creare Project, board o campi personalizzati.

Puoi ispezionare repository, documentazione, issue, milestone, label e codice in sola lettura. Puoi aggiornare le epic create durante questa esecuzione per inserire i link alle relative issue figlie.

Se non hai autenticazione o permessi GitHub sufficienti, fermati prima di creare risultati parziali e restituisci i comandi o il requisito mancante.

## FATTI DI DESIGN DA PRESERVARE

Scenario Lab non è una seconda implementazione delle regole.

- Il designer definisce stato iniziale, comandi, decisioni registrate e risultati attesi.
- Il resolver reale determina eventi e risultato.
- L'event log canonico alimenta replay, spiegazioni e test.
- Lo stato finale confermato di un turno può diventare lo stato iniziale del turno seguente.
- Se viene modificato un turno precedente, i discendenti devono essere invalidati o ricalcolati.
- Una bozza può contenere errori; gli errori di authoring non devono causare crash o perdita del lavoro.
- Le regole non devono essere duplicate nella UI.
- Parità significa copertura provata rispetto al runtime v0.1, non semplice presenza di controlli grafici.
- Build complete, avvii dell'Editor/engine e test packaged sono risorse costose: vanno concentrati in gate espliciti, mentre la maggior parte dei test deve girare nel livello più leggero che dimostra davvero il requisito.

## DEFINIZIONE DI PARITÀ CON REFACTOR TACTICS v0.1

Prima di creare le issue, ricava dal repository la lista effettiva delle capacità della v0.1. Non dare per implementato ciò che compare solo in tavole, roadmap o documenti aspirazionali.

Per ogni capacità assegna una delle seguenti evidenze:

- `Runtime proven`: implementata e raggiungibile nel runtime;
- `Test proven`: coperta da test automatici esistenti;
- `Documented only`: descritta ma non verificata;
- `Planned`: presente soltanto in issue o roadmap;
- `Unknown`: evidenza insufficiente.

La v1.0 di Scenario Lab deve garantire parità soltanto con le capacità `Runtime proven` della baseline di gioco v0.1, includendo i relativi stati e casi d'errore. Le capacità `Documented only`, `Planned` o `Unknown` non devono essere trasformate silenziosamente in requisiti v1.0: vanno segnalate nella issue di matrice di parità.

Verifica almeno queste famiglie, senza presumere che esistano tutte:

- mappa esagonale, livelli, altezza, terreno, copertura e occupazione;
- personaggi, team, orientamento, HP, armor, risorse, cooldown e stati;
- movimento e pathfinding;
- linea di vista e linea di tiro;
- targeting di entità, cella, direzione e area d'effetto;
- attacchi, abilità, Overwatch e reazioni;
- Decision Window e preset;
- fallback `cancel`, `retarget`, `use fallback`, `hold` e `safe stop`;
- conflitti simultanei, priority resolution e tie-break;
- trigger, Reaction Stack, State Queue, danno, morte e aggiornamenti obbligatori;
- oggetti, hazard, ambiente e obiettivi di missione;
- TurnLog, event log canonico, replay e reason code;
- gestione degli input invalidi e dei fallimenti del resolver.

## CONTROLLO DUPLICATI PRIMA DELLA CREAZIONE

1. Identifica il repository remoto e conferma che sia quello corretto.
2. Leggi `AGENTS.md`, `CLAUDE.md` e le convenzioni applicabili, se presenti.
3. Elenca milestone aperte e chiuse.
4. Cerca issue aperte e chiuse contenenti almeno: `Scenario Lab`, `scenario editor`, `map editor`, `replay`, `causal log`, `turn chain`, `scenario authoring`, `parity matrix` e termini equivalenti usati nel repository.
5. Ispeziona label esistenti. Riutilizzale solo se il loro significato è chiaro; non crearne di nuove.
6. Costruisci una tabella interna `planned item -> existing issue or create`.
7. Non creare duplicati. Se una issue esistente copre realmente lo stesso outcome, riusala e riportala nel riepilogo finale.

Una somiglianza nel titolo non è sufficiente: confronta scope e criteri di accettazione.

## GERARCHIA DA CREARE

Crea:

- una epic ombrello: `[EPIC] Scenario Lab — roadmap v0.1 → v1.0`;
- dieci milestone senza data di scadenza: `Scenario Lab v0.1`, `v0.2`, `v0.3`, `v0.4`, `v0.5`, `v0.6`, `v0.7`, `v0.8`, `v0.9`, `v1.0`;
- una release epic per ciascuna milestone, con titolo `[EPIC][Scenario Lab vX.Y] <outcome>`;
- le issue figlie elencate sotto.

Se il repository supporta GitHub sub-issues, usa la relazione nativa. Altrimenti inserisci nelle epic una checklist con link alle issue figlie. Collega le dieci release epic anche alla epic ombrello.

Ogni epic e issue deve appartenere alla milestone corretta. Non assegnare date, persone o stime temporali non presenti nel repository.

## STRUTTURA OBBLIGATORIA DEI BODY

Ogni release epic deve contenere:

- Outcome della release;
- perché serve;
- incluso;
- escluso;
- dipendenze dalla release precedente;
- checklist delle issue figlie;
- Definition of Done della milestone;
- rischi principali.

Ogni issue figlia deve contenere:

1. `## Obiettivo`
2. `## Contesto`
3. `## Scope`
4. `## Fuori scope`
5. `## Criteri di accettazione` con checkbox verificabili
6. `## Errori ed edge case`
7. `## Dipendenze`
8. `## Evidenza richiesta`

I criteri di accettazione devono descrivere comportamento osservabile. Evita formule vaghe come “funziona correttamente”, “supportare completamente” o “migliorare la UX”.

Ogni issue che richiede test deve inoltre dichiarare:

- il livello minimo necessario tra `pure/domain`, `headless resolver`, `engine integration`, `Editor UI` e `packaged`;
- se richiede realmente un nuovo avvio dell'engine;
- come può essere eseguita in batch con altre verifiche della stessa milestone;
- quale evidenza leggera può essere prodotta prima del gate engine.

Usa le label esistenti più adatte. Se non esistono label equivalenti, omettile e segnala la mancanza nel riepilogo: non crearle.

## STRATEGIA BUILD E TEST: MINIMIZZARE IL TEMPO OCCUPATO DALL'ENGINE

Organizza le issue seguendo questa piramide, dal livello preferito al più costoso:

1. **Pure/domain:** schema, serializzazione, validazione, hash, diff, invalidazione, migrazioni e manipolazione della catena di turni senza engine.
2. **Headless resolver:** esecuzione deterministica, golden scenario, assertion ed error case senza aprire l'Editor, se il repository lo consente.
3. **Engine integration:** adapter, registri/asset reali e integrazioni che non possono essere provate correttamente fuori dall'engine.
4. **Editor UI:** interazioni della griglia, authoring, navigazione degli errori e replay visuale.
5. **Packaged:** soltanto smoke test ed evidence di release nei gate previsti.

Regole operative per le issue:

- non richiedere un test engine quando un test pure o headless dimostra lo stesso comportamento;
- separare la logica testabile dall'adapter engine, senza duplicare le regole del resolver;
- raggruppare i test engine per suite e milestone, evitando un avvio completo per singolo caso;
- preferire build incrementali e target minimi già supportati dal repository;
- non introdurre una nuova infrastruttura complessa soltanto per risparmiare pochi secondi;
- misurare prima di ottimizzare build e bootstrap;
- rendere visibili durata, numero di avvii engine e cause dei test lenti nei gate di v0.9/v1.0;
- eseguire build completa e test packaged soltanto quando danno evidenza non ottenibile a livelli inferiori.

Crea una piccola **test/build matrix** nella epic ombrello: per ogni famiglia di verifica indica livello preferito, necessità dell'engine e gate di milestone. Non creare workflow o script durante questa attività.

---

# ROADMAP DA CREARE

## Scenario Lab v0.1 — Contratto, baseline e vertical slice

**Outcome:** una singola situazione minima viene caricata nella griglia 2D, pianificata, inviata al resolver reale e restituisce un risultato ispezionabile, senza duplicare le regole.

Issue figlie:

1. **`[Scenario Lab v0.1] Inventariare la baseline runtime e costruire la matrice di parità`**  
   Identificare capacità realmente eseguibili, relativi stati, errori, test ed entry point. La matrice deve distinguere `Runtime proven`, `Test proven`, `Documented only`, `Planned` e `Unknown`.

2. **`[Scenario Lab v0.1] Definire i contratti versionati di scenario, turno, decisione, evento e risultato`**  
   Specificare identità stabili, versione regole/contenuti, seed, stato iniziale, comandi, decisioni, event log, snapshot finale, assertion e hash. Includere esempi validi e invalidi.

3. **`[Scenario Lab v0.1] Definire il confine tra Scenario Lab e resolver canonico`**  
   Scenario Lab compone input e presenta output; non ricalcola movimento, LOS, danno, priorità o reazioni. Aggiungere test/guardrail architetturali come criterio di accettazione.

4. **`[Scenario Lab v0.1] Creare la shell 2D e visualizzare una griglia esagonale minima`**  
   Vista dall'alto, coordinate leggibili, pan, zoom, selezione e ispezione della cella. Nessun editor di mappa completo in questa release.

5. **`[Scenario Lab v0.1] Caricare uno scenario fixture e visualizzarne unità e stato iniziale`**  
   Caricare una fixture versionata, mostrare almeno due unità, team, posizione, facing e stato minimo proveniente dai dati reali.

6. **`[Scenario Lab v0.1] Eseguire un turno minimo tramite il resolver reale`**  
   Vertical slice con almeno un comando runtime-proven, cattura dell'event log e snapshot finale. Stesso input e stessa versione devono produrre stesso output/hash.

7. **`[Scenario Lab v0.1] Mostrare errori di caricamento ed esecuzione senza crash o perdita della bozza`**  
   Distinguere errore di formato, riferimento mancante, input non valido e fallimento interno; conservare il contenuto modificabile.

8. **`[Scenario Lab v0.1] Aggiungere smoke test end-to-end della vertical slice`**  
   Fixture -> UI/input adapter -> resolver -> event log -> stato finale, con evidenza automatica ripetibile.

9. **`[Scenario Lab v0.1] Definire la test/build matrix e i gate di utilizzo dell'engine`**  
   Classificare test pure, headless, engine integration, Editor UI e packaged; identificare le suite aggregabili, il target minimo disponibile e la baseline misurata di build/bootstrap senza creare nuova infrastruttura in questa issue.

## Scenario Lab v0.2 — Authoring di mappa e stato iniziale

**Outcome:** il designer può creare, salvare e modificare una situazione iniziale coerente con la baseline v0.1.

Issue figlie:

1. **`[Scenario Lab v0.2] Creare, ridimensionare, salvare e ricaricare una mappa esagonale`**  
   Gestire coordinate e identificatori stabili; una riduzione che elimina contenuti deve richiedere conferma e produrre diagnostica.

2. **`[Scenario Lab v0.2] Modificare celle usando il registro runtime di terreno e proprietà`**  
   Mostrare solo tipi realmente disponibili nella baseline; valori sconosciuti devono restare visibili come errori modificabili.

3. **`[Scenario Lab v0.2] Gestire livelli, altezza e collegamenti verticali supportati dalla v0.1`**  
   Creare questa issue solo con scope concreto ricavato dalla matrice; se la verticalità non è runtime-proven, trasformarla in issue di gap e non prometterne l'implementazione.

4. **`[Scenario Lab v0.2] Posizionare, spostare, duplicare e rimuovere entità`**  
   Unità, oggetti, hazard e obiettivi devono provenire dai registri reali. Gestire collisioni e riferimenti orfani come diagnostica.

5. **`[Scenario Lab v0.2] Modificare team, facing e stato completo di un'unità`**  
   Esporre soltanto campi runtime-proven: ad esempio HP, armor, risorse, cooldown e stati quando disponibili.

6. **`[Scenario Lab v0.2] Implementare validazione incrementale dello stato iniziale`**  
   Separare errori bloccanti, errori eseguibili per test negativo e warning. Ogni diagnostica deve indicare entità/cella/campo sorgente.

7. **`[Scenario Lab v0.2] Salvare bozze incomplete senza normalizzazioni distruttive`**  
   Salvataggio e riapertura devono preservare valori invalidi, campi sconosciuti compatibili e diagnostica.

## Scenario Lab v0.3 — Pianificazione completa del turno

**Outcome:** il designer può comporre un piano simultaneo con gli stessi comandi e tipi di bersaglio disponibili nel runtime v0.1.

Issue figlie:

1. **`[Scenario Lab v0.3] Popolare il catalogo comandi dalle capacità runtime`**  
   Evitare una lista UI mantenuta manualmente in parallelo alle regole. Azioni non disponibili devono spiegare la causa.

2. **`[Scenario Lab v0.3] Pianificare movimento, percorso e orientamento finale`**  
   Visualizzare percorso richiesto, costo e stato di validità; consentire di conservare un percorso invalido come bozza.

3. **`[Scenario Lab v0.3] Pianificare attacchi e abilità con targeting runtime-proven`**  
   Coprire soltanto i tipi verificati: entità, cella, direzione, linea, cono o area. Evidenziare portata, LOS e celle coinvolte senza prevedere falsamente un esito certo.

4. **`[Scenario Lab v0.3] Pianificare Overwatch, reazioni, preset e fallback supportati`**  
   Registrare configurazione e scelte come input, senza risolverne localmente l'effetto.

5. **`[Scenario Lab v0.3] Rappresentare il piano simultaneo di tutte le unità`**  
   Filtri per team/unità, leggibilità dei conflitti e distinzione tra ordine di authoring e ordine canonico di risoluzione.

6. **`[Scenario Lab v0.3] Implementare editor delle Decision Window registrabili`**  
   Supportare scelte, preset, auto-risoluzione e timeout solo quando presenti nella baseline. Le decisioni mancanti devono produrre diagnostica riproducibile.

7. **`[Scenario Lab v0.3] Distinguere previsione Confirmed, Predicted e Uncertain`**  
   Non mostrare come certo ciò che dipende da reazioni, occupazione, perdita di LOS, movimento del target o mutamento dello stato.

8. **`[Scenario Lab v0.3] Validare e correggere comandi invalidi dalla griglia e dalla timeline`**  
   Cliccando la diagnostica si deve selezionare il comando e il campo responsabile; la correzione non deve ricreare il turno da zero.

## Scenario Lab v0.4 — Esecuzione canonica, errori e assertion

**Outcome:** qualsiasi piano salvabile può essere inviato in modo controllato al resolver; successi e fallimenti producono evidenza strutturata.

Issue figlie:

1. **`[Scenario Lab v0.4] Implementare l'adapter di esecuzione verso il resolver canonico`**  
   Definire confine, serializzazione, isolamento e timeout. Vietare calcoli sostitutivi nella UI.

2. **`[Scenario Lab v0.4] Catturare pacchetto di riproducibilità dell'esecuzione`**  
   Stato iniziale, comandi, decisioni, seed, versioni, ordinamento stabile e hash devono essere esportabili insieme.

3. **`[Scenario Lab v0.4] Gestire errori di validazione, dominio, resolver e infrastruttura`**  
   Ogni classe deve avere messaggio, reason code, sorgente, gravità e azione consigliata. Stack trace tecnici non sostituiscono il messaggio utente.

4. **`[Scenario Lab v0.4] Conservare e ispezionare event log e snapshot canonico finale`**  
   Nessun callback grafico o `AnimNotify` può diventare autorità sul risultato.

5. **`[Scenario Lab v0.4] Definire assertion su stato, eventi e assenza di eventi`**  
   Posizione, HP, stati, morte, obiettivi, reason code, evento presente/assente e primo punto di divergenza.

6. **`[Scenario Lab v0.4] Supportare esplicitamente scenari negativi attesi`**  
   Il designer può dichiarare che un input deve fallire con uno specifico reason code; un fallimento atteso soddisfatto non è un crash né un falso successo.

7. **`[Scenario Lab v0.4] Confrontare risultato atteso e risultato ottenuto`**  
   Diff strutturato e navigabile, senza affidarsi soltanto a snapshot test testuali giganteschi.

## Scenario Lab v0.5 — Catena multi-turno deterministica

**Outcome:** il risultato confermato di un turno alimenta il successivo, formando scenari lineari riproducibili.

Issue figlie:

1. **`[Scenario Lab v0.5] Confermare lo stato finale come stato iniziale del turno successivo`**  
   Operazione esplicita e atomica; nessuna modifica implicita dell'input originale.

2. **`[Scenario Lab v0.5] Persistenza versionata della catena di turni`**  
   Ogni turno referenzia l'hash del risultato genitore e conserva input, decisioni, log, risultato e assertion.

3. **`[Scenario Lab v0.5] Navigare, rinominare, duplicare e rimuovere turni`**  
   Le operazioni distruttive richiedono conferma e mostrano quali discendenti diventano invalidi.

4. **`[Scenario Lab v0.5] Invalidare i turni discendenti dopo la modifica di un antenato`**  
   Stati almeno `valid`, `outdated`, `recompute available` e `conflict`, con causa e turno d'origine.

5. **`[Scenario Lab v0.5] Rieseguire l'intera catena e fermarsi alla prima divergenza`**  
   Mostrare input hash, evento o stato divergente e dipendenze successive coinvolte.

6. **`[Scenario Lab v0.5] Gestire un comando successivo non più applicabile`**  
   Non correggere automaticamente in silenzio; offrire diagnostica e modifica, rimozione o uso del fallback previsto.

7. **`[Scenario Lab v0.5] Salvare e ricaricare uno scenario lineare di almeno tre turni`**  
   La riesecuzione completa deve produrre gli stessi output nella stessa versione.

## Scenario Lab v0.6 — Replay e causalità leggibile

**Outcome:** il designer può capire cosa è successo, in quale ordine e per quale motivo usando gli eventi canonici.

Issue figlie:

1. **`[Scenario Lab v0.6] Implementare controlli replay Play, Pause, Step, Slow e Fast`**  
   I controlli cambiano soltanto la presentazione; il risultato canonico rimane invariato.

2. **`[Scenario Lab v0.6] Proiettare gli eventi canonici sulla griglia 2D`**  
   Movimento, targeting, impatto, trigger, reazioni e cambi di stato usano una presentazione basilare e interrompibile.

3. **`[Scenario Lab v0.6] Implementare livelli Summary, Detail e Forensic`**  
   Summary spiega cosa; Detail spiega perché; Forensic espone ordine, trigger, mitigazioni, reason code e dati completi disponibili.

4. **`[Scenario Lab v0.6] Visualizzare il diff di stato per evento e per turno`**  
   Evidenziare entità e campi cambiati senza ricostruire il risultato dai soli effetti grafici.

5. **`[Scenario Lab v0.6] Navigare dal log causale alla cella, entità e comando sorgente`**  
   Riferimenti mancanti o rimossi devono produrre una diagnostica gestita.

6. **`[Scenario Lab v0.6] Verificare che animazioni e replay consumino eventi senza generarne`**  
   Test architetturali e di regressione contro una seconda autorità visiva.

7. **`[Scenario Lab v0.6] Esportare un evidence package riproducibile`**  
   Scenario, versioni, hash, risultati assertion e primo errore/divergenza, senza dati transienti non necessari.

## Scenario Lab v0.7 — Editing sicuro, recupero e varianti

**Outcome:** scenari complessi possono essere modificati, rotti intenzionalmente, recuperati e confrontati senza perdita di dati.

Issue figlie:

1. **`[Scenario Lab v0.7] Implementare undo/redo per mappe, stato e piani`**  
   Confini delle transazioni espliciti; esecuzione e conferma del risultato non devono essere confuse con una singola modifica grafica.

2. **`[Scenario Lab v0.7] Implementare pannello diagnostico unificato e navigabile`**  
   Filtri per gravità, turno, entità e categoria; selezione della sorgente; diagnostica aggiornata dopo la correzione.

3. **`[Scenario Lab v0.7] Preservare campi sconosciuti e contenuti non caricabili`**  
   Evitare salvataggi distruttivi quando manca un asset, un tipo o una versione compatibile.

4. **`[Scenario Lab v0.7] Creare una variante da qualsiasi turno confermato`**  
   Ramo alternativo con parent hash e nome, senza duplicare inutilmente asset immutati.

5. **`[Scenario Lab v0.7] Confrontare due varianti di piano e risultato`**  
   Differenze di comandi, eventi, stato finale e assertion devono essere distinguibili.

6. **`[Scenario Lab v0.7] Gestire conflitti dopo modifica o ricalcolo dell'antenato`**  
   Nessun merge automatico di comandi semanticamente incompatibili; offrire scelte esplicite.

7. **`[Scenario Lab v0.7] Implementare autosave e recupero dopo arresto anomalo`**  
   Il recupero deve dichiarare versione e ultima operazione confermata, senza sovrascrivere automaticamente il file principale.

## Scenario Lab v0.8 — Parità funzionale con il runtime v0.1

**Outcome:** tutte le capacità `Runtime proven` della baseline v0.1 possono essere configurate, eseguite, riprodotte e verificate in Scenario Lab.

Issue figlie:

1. **`[Scenario Lab v0.8] Automatizzare la matrice di copertura Scenario Lab ↔ runtime v0.1`**  
   Ogni capacità runtime-proven deve collegare schema, authoring, validazione, esecuzione, replay, error case e test.

2. **`[Scenario Lab v0.8] Chiudere la parità di comandi, abilità e modalità di targeting`**  
   Issue guidata dalla matrice: elencare elementi concreti mancanti; non usare “tutte le abilità” senza inventario verificabile.

3. **`[Scenario Lab v0.8] Chiudere la parità di stati delle unità e risorse persistenti`**  
   HP, armor, status, cooldown, risorse, morte e ogni altro stato runtime-proven devono sopravvivere a save/load e passaggio di turno.

4. **`[Scenario Lab v0.8] Chiudere la parità di mappa, ambiente, oggetti, hazard e obiettivi`**  
   Scope derivato esclusivamente dalla matrice aggiornata.

5. **`[Scenario Lab v0.8] Chiudere la parità di simultaneità, trigger, reazioni e fallback`**  
   Includere tie-break, Decision Window, Reaction Stack e State Queue soltanto nei comportamenti effettivamente runtime-proven.

6. **`[Scenario Lab v0.8] Coprire errori e casi limite per ogni capacità runtime-proven`**  
   Ogni riga della matrice deve avere almeno un caso nominale e, dove sensato, uno invalido o conflittuale.

7. **`[Scenario Lab v0.8] Creare il golden scenario pack di parità`**  
   Scenari piccoli e focalizzati più almeno uno scenario integrato multi-turno; tutti rieseguibili in modalità headless.

8. **`[Scenario Lab v0.8] Introdurre il gate di parità verificata`**  
   La milestone non è completabile finché restano righe runtime-proven senza evidenza o divergenze non accettate esplicitamente.

## Scenario Lab v0.9 — Hardening, compatibilità e workflow quotidiano

**Outcome:** il tool è affidabile su scenari reali, aggiornamenti di schema e flussi di lavoro ripetuti.

Issue figlie:

1. **`[Scenario Lab v0.9] Versionare e migrare i file di scenario senza perdita silenziosa`**  
   Migrazione esplicita, backup, report delle modifiche e possibilità di annullare l'apertura in scrittura.

2. **`[Scenario Lab v0.9] Eseguire scenari e suite in modalità headless`**  
   Exit code, report machine-readable, primo punto di divergenza e integrazione compatibile con la CI esistente senza modificarla in questa attività.

3. **`[Scenario Lab v0.9] Stabilire budget prestazionali e testarli su mappe/scenari rappresentativi`**  
   Misurare caricamento, validazione incrementale, replay, memoria e riesecuzione completa; fissare soglie basate su evidenza del progetto.

4. **`[Scenario Lab v0.9] Rafforzare leggibilità e accessibilità della griglia, timeline e diagnostica`**  
   Non affidarsi soltanto al colore; navigazione da tastiera dove applicabile; zoom e densità informativa verificati.

5. **`[Scenario Lab v0.9] Gestire riferimenti ad asset rinominati, rimossi o incompatibili`**  
   Diagnostica, placeholder non distruttivo e strumenti di rimappatura esplicita.

6. **`[Scenario Lab v0.9] Validare input non fidati e limiti di complessità`**  
   File malformati, dimensioni eccessive, cicli nei riferimenti, profondità di eventi e dati sconosciuti non devono bloccare o corrompere il tool.

7. **`[Scenario Lab v0.9] Creare workflow di triage issue da evidence package`**  
   Da un pacchetto deve essere possibile riprodurre il problema e identificare versione, scenario, turno e divergenza senza passaggi manuali ambigui.

8. **`[Scenario Lab v0.9] Preparare build distributiva interna e smoke test fuori dall'Editor`**  
   Se coerente con l'architettura corrente, verificare launcher -> Scenario Lab -> esecuzione -> risultato in ambiente packaged. Se non applicabile, documentare il motivo nell'issue invece di inventare una build separata.

9. **`[Scenario Lab v0.9] Ridurre e concentrare build e sessioni di test dipendenti dall'engine`**  
   Misurare numero di avvii, tempo di bootstrap/build/test e duplicazioni; aggregare le verifiche compatibili in suite di milestone, mantenendo test piccoli e diagnostica del primo fallimento. L'ottimizzazione non deve ridurre copertura né introdurre una cache non verificabile.

## Scenario Lab v1.0 — Parità certificata e release stabile

**Outcome:** Scenario Lab copre e prova l'intera baseline runtime di Refactor Tactics v0.1, inclusi authoring, errori, modifica, multi-turno, replay e verifica automatica.

Issue figlie:

1. **`[Scenario Lab v1.0] Congelare la baseline di parità con Refactor Tactics v0.1`**  
   Registrare commit/tag/versioni della baseline, chiudere ogni `Unknown` e rendere esplicite eventuali esclusioni approvate.

2. **`[Scenario Lab v1.0] Validare lo scenario canonico showcase di 8–12 turni`**  
   Deve dimostrare movimento, LOS/targeting, almeno una reazione runtime-proven, un conflitto o fallback, modifica di stato, morte/KO se disponibile, obiettivo e replay causale.

3. **`[Scenario Lab v1.0] Completare il test end-to-end authoring → errore → correzione → esecuzione → turno successivo`**  
   Il test deve includere una bozza invalida conservata, navigazione all'errore, correzione, conferma del risultato e riesecuzione deterministica.

4. **`[Scenario Lab v1.0] Verificare determinismo e primo punto di divergenza sull'intera suite`**  
   Ripetizioni, reload e modalità headless devono concordare; una mutazione controllata deve essere individuata nel turno/evento corretto.

5. **`[Scenario Lab v1.0] Eseguire audit finale contro la matrice di parità`**  
   Zero righe runtime-proven prive di authoring, validazione, esecuzione, replay, gestione errori ed evidenza automatica, salvo eccezioni approvate e motivate.

6. **`[Scenario Lab v1.0] Completare documentazione utente, tecnica e troubleshooting`**  
   Quick start, formato scenario, workflow multi-turno, diagnostica, compatibilità, evidence package e confine di autorità con il resolver.

7. **`[Scenario Lab v1.0] Eseguire release candidate e sessione di accettazione designer`**  
   Checklist osservabile con creazione, errore intenzionale, modifica, replay, branching, save/load e verifica suite.

8. **`[Scenario Lab v1.0] Pubblicare la release stabile e registrare i gap post-v1.0`**  
   Release notes, versione formato, baseline compatibile, limiti noti e backlog separato per funzionalità successive alla parità v0.1.

---

## DIPENDENZE E REGOLE DI SEQUENZIAMENTO

- Ogni milestone dipende dalla precedente, salvo attività di discovery chiaramente parallele.
- `v0.1` è un gate: nessuna UI può diventare una seconda autorità.
- `v0.4` è un gate: niente multi-turno basato su risultati non canonici.
- `v0.5` è un gate: replay multi-turno e branching devono usare parent hash e invalidazione.
- `v0.8` è il gate di parità funzionale.
- `v0.9` rende la parità affidabile nel workflow reale.
- `v1.0` certifica la baseline e non deve introdurre nuovi sistemi di gameplay.

Le issue che dipendono da una decisione o da un inventario devono dichiararlo. Non colmare informazioni mancanti inventando dettagli tecnici.

## DEFINITION OF DONE GLOBALE v1.0

La epic ombrello può essere chiusa soltanto quando:

- esiste una baseline v0.1 identificata e verificabile;
- ogni capacità `Runtime proven` compare nella matrice di parità;
- ogni capacità richiesta può essere configurata o caricata in Scenario Lab;
- input validi e invalidi sono gestibili senza perdita di lavoro;
- il resolver canonico è l'unica autorità sul risultato;
- lo stato finale può alimentare il turno successivo;
- modifiche ai turni precedenti invalidano correttamente i discendenti;
- replay e causal log derivano dagli eventi canonici;
- save/load conserva stato, errori, versioni e riferimenti compatibili;
- la suite headless rileva la prima divergenza;
- lo scenario showcase di 8–12 turni è riproducibile;
- la maggioranza dei test di schema, validazione, catena e diff gira senza engine;
- gli avvii engine e le build complete sono concentrati in gate misurati e motivati;
- non restano gap di parità non esplicitamente accettati.

## MODALITÀ DI CREAZIONE

Procedi in questo ordine:

1. completa l'audit read-only e il controllo duplicati;
2. mostra un preflight sintetico con repository, numero di milestone/issue da creare e duplicati da riusare;
3. crea o riusa le dieci milestone;
4. crea o riusa la epic ombrello;
5. per ogni release crea prima la release epic, poi le issue figlie;
6. collega le issue figlie alla release epic;
7. collega le release epic alla epic ombrello;
8. verifica che ogni nuova issue abbia milestone, body completo e collegamento alla epic;
9. non iniziare alcuna implementazione.

Se devi ridurre il numero di chiamate, non comprimere più outcome diversi in una singola issue gigantesca. Puoi creare in batch soltanto quando il risultato resta verificabile e i body non vengono persi.

## OUTPUT FINALE RICHIESTO

Al termine restituisci:

1. URL della epic ombrello;
2. tabella milestone -> release epic -> issue create -> issue riusate;
3. elenco dei duplicati evitati con motivazione;
4. label mancanti ma non create;
5. eventuali capacità v0.1 rimaste `Unknown`;
6. conferma esplicita che non sono stati modificati file, codice, branch, commit, PR, workflow o Project.

Non proporre né iniziare il codice dopo la creazione delle issue. Fermati al riepilogo degli artefatti GitHub.
