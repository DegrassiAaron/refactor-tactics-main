# Piano di migrazione degli Stable ID legacy — `Action.Guard`, `Action.Activate`, `Action.Sprint`

> `CURRENT` · **Data**: 2026-08-10 · **Owner del piano**: questo documento
> **Consegue da**: [D-014 e D-015](../decisions/RT_PDR_00_Decision_Log.md) — la **tassonomia** è decisa, la **migrazione** no
> **Issue**: [#199](https://github.com/DegrassiAaron/refactor-tactics-main/issues/199) · **Milestone**: v0.1 · Gate di release
>
> ⚠️ **Questo documento pianifica una migrazione, non la esegue.** Alla data in cui è scritto nessun ID è stato
> rinominato o deprecato. Chi lo legge cercando lo stato del codice deve fidarsi della §1, che è misurata, non
> della §3, che è un piano.

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

| # | Fetta | Tocca la serializzazione? | Dipendenze |
|---:|---|---|---|
| 1 | **Tabella di redirect** `legacy → canonico`, in una sede unica, con i tre ID mappati e nessun consumatore ancora cambiato | no | — |
| 2 | **Validator**: un ID legacy usato in un contesto nuovo produce un errore diagnostico che *nomina* l'ID e il sostituto | no | 1 |
| 3 | `Action.Activate` → assorbita da `Interact` (2 file di codice, 1 di test: la fetta più piccola, e per questo la prima delle tre) | sì, in lettura | 1, 2 |
| 4 | `Action.Guard` → capacità/stance specifica, non fondamentale universale | sì, in lettura | 1, 2 |
| 5 | `Action.Sprint` → `MoveProfile.Sprint`: cambia **famiglia e nome**, non la meccanica (vedi §2) | sì, in lettura | 1, 2 |
| 6 | **Test di migrazione**: un TurnLog scritto col vocabolario vecchio si rilegge col nuovo e produce lo stesso stato | sì | 3, 4, 5 |

**La regola che tiene insieme le fette 3–5**: il redirect si applica in **lettura**, mai in scrittura. Un
replay registrato prima della migrazione continua a valere; uno registrato dopo usa il vocabolario nuovo. Non
esiste un momento in cui due ID sono entrambi autorevoli.

**Verifica a due binari** (obbligatoria per la fetta 6): scrivere il log col binario **vecchio**, rileggerlo
col **nuovo**, confrontare un digest dei soli campi preesistenti. Un test in memoria non tocca la
serializzazione e non dimostra nulla di ciò che questa migrazione rischia.

## 4. La finestra che conviene non sprecare

**Il corpus golden non esiste ancora.** `Source/RefactorTactics/Tests/Golden/` non è presente nel repository, e
la issue che lo crea — [#178](https://github.com/DegrassiAaron/refactor-tactics-main/issues/178), CP 12.6 — è
**aperta e in lavorazione**.

Questo inverte il verso del rischio dichiarato dalla issue #199:

- **oggi** la voce di DoD «i golden restano validi, oppure la PR dichiara perché cambiano» è **vacua**: non c'è
  nulla da invalidare, e le fette 3–5 non hanno un corpus da migrare;
- **dopo la chiusura di #178** quella voce diventa vincolante, e ogni fetta che tocchi un ID dovrà rigenerare
  il corpus dichiarando *perché* l'esito è cambiato — che è precisamente il lavoro che #178 vuole rendere
  difficile, e giustamente.

**Raccomandazione operativa**: chi lavora #178 e chi lavora #199 dovrebbero concordare l'ordine. Le due vie
sensate sono *migrare prima e congelare dopo*, oppure *congelare ora e mettere in conto una rigenerazione
motivata*. La via che costa di più è la terza, cioè non decidere: il corpus nasce con gli ID vecchi mentre la
migrazione è in corso, e si paga due volte.

Nessuna delle due scelte è presa qui: questo documento la segnala a chi ha il contesto.

## 5. Definition of Done della issue — stato al 2026-08-10

| Voce | Stato | Dove |
|---|---|---|
| Nessuna doppia verità runtime: un solo ID autorevole per azione | ⏳ da fare | fette 3–5 |
| `Sprint` non risolve più in `ResolveDash`, **oppure** è documentato perché lo fa | ✅ **fatto** | §2 di questo documento |
| Validator aggiornato | ⏳ da fare | fetta 2 |
| Test di migrazione o redirect per la serializzazione persistente | ⏳ da fare | fetta 6 |
| Golden TurnLog: o restano validi, o la PR dichiara perché cambiano | ⚠️ **vacuo oggi** | §4 — il corpus non esiste ancora |
| Nessun documento insegna «`Sprint` = `Dash`» | ✅ già fatto lato docs | — |

## 6. Rischio

**Medio-alto**, come dichiarato dalla issue, e concentrato nelle fette 5 e 6: `Action.Sprint` è il più diffuso
dei tre (9 file di codice) ed è l'unico la cui migrazione cambia **famiglia**, non solo nome.

Attenuanti reali: la meccanica non cambia (§2), il corpus golden non esiste ancora (§4), e le fette 3 e 4 sono
piccole abbastanza da esercitare il meccanismo di redirect prima che tocchi l'ID che conta.
