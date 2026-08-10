# Piano di migrazione degli Stable ID legacy — `Action.Guard`, `Action.Activate`, `Action.Sprint`

> `CURRENT` · **Data**: 2026-08-10 · **Owner del piano**: questo documento
> **Consegue da**: [D-014 e D-015](../decisions/RT_PDR_00_Decision_Log.md) — la **tassonomia** è decisa, la **migrazione** no
> **Issue**: [#199](https://github.com/DegrassiAaron/refactor-tactics-main/issues/199) · **Milestone**: v0.1 · Gate di release
>
> ⚠️ **Questo documento pianificava una migrazione. Dal 2026-08-10 ne esegue una parte.** Le fette **1** e **3**
> sono atterrate, la **4** è stata **cancellata da [D-025]** e la **5** è stata **decisa** da [D-068]. Chi lo
> legge cercando lo stato del codice deve fidarsi della §3, che ora dichiara cosa è fatto, e della §1, che
> resta la misura di partenza.

## 1. Stato misurato — 2026-08-10

| Stable ID | File di codice | File di test | Fase di risoluzione | Note |
|---|---:|---:|---|---|
| `Action.Sprint` | **9** | **6** | `FastMovement` → `ERTMatchPhase::Dash` | 8 MP di budget, slot `Movement`, stile `Budget`, effetto `Status.Exposed` 1 turno, `bAllowsReaction = false` |
| `Action.Guard` | **6** | **5** | `Preparation`, priorità 40 | in crescita: la issue ne misurava 4/4 il 2026-08-08 |
| `Action.Activate` | **2** | **1** | `Attack`, priorità 70 | il meno diffuso dei tre |

*Misura: `grep -rl` sui file, non sulle righe — è il conteggio che usa la issue.*

Rispetto alla misura del 2026-08-08, `Action.Sprint` è **invariato** (9/6) e `Action.Activate` ha guadagnato un
file di codice. `Action.Guard` è passato da 4/4 a 6/5: **cresce mentre lo si vuole ritirare**, che è
esattamente la ragione per cui questo piano non può restare implicito.

## 2. La domanda che si poteva chiudere subito: «`Sprint` = `Dash`?»

Il DoD della issue chiede: *«`Sprint` non risolve più in `ResolveDash`, **oppure** è documentato perché lo fa
ancora»*. Ecco la seconda via, misurata.

**«`Sprint` = `Dash`» è falso come *stile*, vero come *fase*.**

- **Come fase**: `Action.Sprint` è `ERTResolutionPhase::FastMovement`, che mappa a `ERTMatchPhase::Dash`. Sta
  lì perché è **mobilità rapida**, e la mobilità rapida precede il Blast. Questo è corretto e non va cambiato.
- **Come stile**: `Action.Sprint` dichiara `ERTMovementStyle::Budget`, **non** un `Linear*`. Il commento
  dell'orchestratore lo dice già: «l'instradamento per STILE è quello di `ResolveDash`: solo le mobilità
  **lineari** passano da `ResolveLinearMove`. Una mobilità a budget (`Action.Sprint`) risolve col
  **pathfinding**, lo stesso grafo da cui le candidate sono nate».

Quindi `Sprint` **entra** nell'orchestratore della fase Dash ma **non** percorre il codice del Dash lineare:
non si ferma davanti agli ostacoli come una carica, spende punti movimento su terreno difficile, e arriva meno
lontano dove il terreno costa di più.

**Conseguenza per la tassonomia desiderata.** L'obiettivo di D-014/D-015 — `Sprint` come profilo della famiglia
`Move` (`MoveProfile.Sprint`) e non come `Dash` — è **più vicino di quanto la issue lasciasse intendere**: il
comportamento è già quello di un Move a budget maggiorato. Ciò che resta legacy è **il nome e la collocazione
di fase**, non la meccanica.

## 3. Il piano, a fette

Ogni fetta chiude con la suite verde. Nessuna fetta rinomina un ID: gli Stable ID entrano nel **TurnLog
serializzato**, quindi si deprecano e si redirigono, non si rinominano.

| # | Fetta | Stato | Tocca la serializzazione? | Dipendenze |
|---:|---|:--|---|---|
| 1 | **Tabella di redirect** `legacy → canonico`, in una sede unica | ✅ **2026-08-10** — `URTCatalogLibrary::ResolveLegacyActionId`, chiamata da `FindCoreAction`, che è **l'unico ingresso del catalogo per ID** | no | — |
| 2 | **Validator**: un ID legacy usato in un contesto nuovo produce un errore diagnostico che *nomina* l'ID e il sostituto | ✅ **2026-08-10** — `ValidateActions` rifiuta un'azione **dichiarata** con un ID ritirato e nomina l'erede. Il redirect vale in **lettura**; in scrittura ricreerebbe la doppia verità appena tolta. Test: `Catalog.ValidatorRejectsRetiredStableId` | no | 1 |
| 3 | `Action.Activate` → assorbita da `Interact` | ✅ **2026-08-10** — fuori dal catalogo spedito, ID reindirizzato in lettura. Test: `Actions.RetiredStableIdRedirectsToHeir` | sì, in lettura | 1 |
| 4 | ~~`Action.Guard` → capacità/stance specifica~~ | ❌ **CANCELLATA** — la fetta era già superata quando il piano è stato scritto: [D-025] **emenda D-014** e riporta `Guard` fra le **sette generiche universali**, per i suoi tre consumatori (catalogo, `Status.Root`, difesa direzionale di ADR-0005 §4a). Non c'è niente da migrare | — | — |
| 5 | `Action.Sprint` → `MoveProfile.Sprint` | ✅ **DECISA 2026-08-10** — [D-068]: **resta** `Action.Sprint` in fase `FastMovement`, con la motivazione scritta. La rinomina non avviene. Test: `Actions.SprintIsAMoveProfileResolvedPreBlast`, scritto per **cadere** se la fase viene migrata | no | — |
| 6 | **Test di migrazione**: un TurnLog scritto col vocabolario vecchio si rilegge col nuovo e produce lo stesso stato | ✅ **2026-08-10** — `TurnLog.RetiredActionIdIsStillReadableFromDisk`: la traccia si rilegge, l'ID **resta scritto com'era** (il loader non riscrive), il catalogo risponde con l'erede, e l'hash è riproducibile | sì | 3 |

> **Quanto è cambiato il piano.** Delle tre migrazioni previste ne resta **una**, ed è la più piccola. Non
> perché il lavoro sia stato tagliato: due delle tre erano **decisioni già prese altrove** che il piano non
> aveva incrociato — `Guard` da [D-025], che è più recente di D-014 e lo emenda; `Sprint` da una domanda che
> il DoD stesso ammetteva di poter chiudere scrivendo. Il rischio «medio-alto» della §6 era in larga parte il
> costo di non aver letto la decisione più recente.

**La regola che tiene insieme le fette 3–5**: il redirect si applica in **lettura**, mai in scrittura. Un
replay registrato prima della migrazione continua a valere; uno registrato dopo usa il vocabolario nuovo. Non
esiste un momento in cui due ID sono entrambi autorevoli.

**Verifica a due binari** (obbligatoria per la fetta 6): scrivere il log col binario **vecchio**, rileggerlo
col **nuovo**, confrontare un digest dei soli campi preesistenti. Un test in memoria non tocca la
serializzazione e non dimostra nulla di ciò che questa migrazione rischia.

## 4. La finestra — **chiusa il 2026-08-09, e non è costata nulla**

> ⚠️ **Questa sezione era sbagliata dal giorno dopo.** Diceva: *«Il corpus golden non esiste ancora»*, e
> raccomandava di concordare l'ordine fra `#178` e `#199` perché *«la via che costa di più è la terza, cioè non
> decidere»*.

**`#178` è atterrata** (CP 12.6): `Source/RefactorTactics/Tests/Golden/` esiste, con `Movement.Basic` e
`Movement.Collision`. Quindi è successa proprio la terza via — il corpus è nato mentre la migrazione era ferma
— e **non si è pagata due volte**, per una ragione che il piano non poteva prevedere e che vale registrare:

**il corpus congelato non contiene nessuno dei tre ID legacy.** Sono due scenari di movimento puro. La fetta 3
è atterrata senza toccare un solo byte del corpus, e il DoD «i golden restano validi, oppure la PR dichiara
perché cambiano» si chiude sulla **prima** via, con la prova: suite **596/596** dopo la migrazione, i due test
golden compresi.

**Cosa se ne impara**, dato che la prossima volta la fortuna può mancare: il rischio non era «il corpus
esiste», era «il corpus contiene l'ID che stai migrando». È una domanda a cui si risponde con un `grep`, non
con un coordinamento fra due issue.

## 5. Definition of Done della issue — stato al 2026-08-10

| Voce | Stato | Dove |
|---|---|---|
| Nessuna doppia verità runtime: un solo ID autorevole per azione | ✅ **fatto** per `Activate` (fuori dal catalogo, redirect in lettura) · per `Guard` **non si applica** (D-025) · per `Sprint` **non si applica** (D-068) | fette 3–5 |
| `Sprint` non risolve più in `ResolveDash`, **oppure** è documentato perché lo fa | ✅ **fatto**, e ora è una **decisione** — [D-068], non solo una nota | §2 + `Actions.SprintIsAMoveProfileResolvedPreBlast` |
| Validator aggiornato | ✅ **fatto** | fetta 2 — `Catalog.ValidatorRejectsRetiredStableId` |
| Test di migrazione o redirect per la serializzazione persistente | ✅ **fatto** | fetta 6 — vedi §7 sul perché il test è equivalente a usare due binari |
| Golden TurnLog: o restano validi, o la PR dichiara perché cambiano | ✅ **restano validi**, e c'è la prova — vedi §4 | §4 |
| Nessun documento insegna «`Sprint` = `Dash`» | ✅ già fatto lato docs | — |

## 6. Rischio — **rivisto al ribasso**

Era dichiarato **medio-alto** e concentrato nelle fette 5 e 6, perché `Action.Sprint` è il più diffuso dei tre
e l'unico a cambiare famiglia. **Quella fetta non si esegue più** ([D-068]), e la 4 è cancellata da [D-025].

Il rischio residuo è **basso** e sta tutto nella fetta 6: dimostrare che un `.rttl` scritto quando
`Action.Activate` era nel catalogo si rilegge ancora. È il caso che il redirect esiste per servire, ed è
l'unico che i test attuali **non** coprono — la verifica in memoria non tocca la serializzazione.

## 7. La «verifica a due binari», e perché un binario solo è bastato

Il piano chiedeva: *«scrivere il log col binario **vecchio**, rileggerlo col **nuovo**, confrontare un digest
dei soli campi preesistenti»*. Il test scritto usa un binario solo. **Non è una scorciatoia**, ed è l'unico
punto del piano che vale la pena spiegare invece di dichiarare.

Il formato **non è cambiato per gli `ActionId`** da quando `Action.Activate` era a catalogo: dalla `v3` in poi
l'ID è lunghezza `uint16` + byte UTF-8, e nessuna versione successiva — `v4` FormatId, `v5` BaseActionId,
`v6` UnitId, `v7` Priority — ha toccato quella posizione o quella codifica. I byte che il serializzatore
di oggi produce per `Action.Activate` sono **gli stessi byte** che il binario di allora avrebbe scritto: non
una simulazione, la stessa sequenza. Ciò che il test verifica è il **lettore di oggi** su quei byte, che è
esattamente la domanda della fetta 6.

La verifica a due binari resta necessaria quando cambia **la forma** dei byte, non il loro contenuto — ed è il
motivo per cui la regola generale sta in piedi anche se qui non si applica.
