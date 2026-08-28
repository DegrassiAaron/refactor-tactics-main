# CLAUDE TASK — RefactorTactics GitHub Issues v0.1 → v1.0

> `HISTORICAL` · **Kit d'autore consumato**, non una fonte. · **Consumato**: 2026-08-28 · **Base**: `2809a377`
>
> Archiviato da [`docs/archive/`](../../README.md): vale per la **provenienza** e il rationale, mai per la regola.
> Il file stava alla radice del repository come `CLAUDE_GitHub_Issues_Roadmap_v0.1_to_v1.0.md`.
>
> **Cosa possiede**: la richiesta d'autore del 2026-08-28 e le sue dieci fasi, verbatim.
> **Cosa non possiede**: nessuna autorità. Tre delle sue premesse sono state **falsificate** dalla misura, e
> ciò che è stato eseguito lo è stato in forma diversa da quella che chiedeva.

## Cosa è stato eseguito, e cosa è stato corretto

⛔ **FASE 2 non è stata eseguita come scritta, e la ragione è misurata.** Chiedeva un `CP E13.8` in **v0.1** per
far «attraversare il boundary dati → unità → percezione» a `VisionRange`. Il boundary **è già chiuso**:
`URTHeroData::VisionRange` (`RTHeroData.h:48`) → `ARTUnit::VisionRange` (`RTUnit.cpp:876`) →
`FRTPerceiver::VisionRange` in **tutti e quattro** i siti di costruzione (`RTTurnManager.cpp:250,505,5541`,
`RTTurnManager_Blast.cpp:186`) → `VisibleCells` → `FRTTeamKnowledge`. **Nove dei dieci criteri** del suo DoD
erano già verdi, e **cinque dei sei test** che proponeva esistono già con altro nome
(`FRTVisionVisibleCellsRespectsSightTest`, `FRTVisionConeUsesHexConePrimitiveTest`,
`FRTVisionAwarenessWithinTwoCellsIgnoresFacingTest`, `FRTVisionTeamKnowledgeIsUnionTest`,
`FRTVisionSmokeCapsContactAtTwoTest`): crearli avrebbe prodotto duplicati.

Ciò che manca davvero — il **budget a punti** — in v0.1 avrebbe tutti i modificatori a `0`, riducendo la formula
all'identità `Distance <= VisionRange`. Aprirlo in v0.1 avrebbe violato la **regola operativa 8 di questo stesso
documento**: *«Non spostare lavoro futuro nella v0.1 solo per completezza teorica»*. È diventato **CP 27.4**
([#1569](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1569)) sotto E27/v0.3, dove
l'illuminazione è un valore reale.

⛔ **La premessa di FASE 3 era invertita.** Il documento assumeva che il difetto fosse l'assenza di un
checkpoint; il difetto era una **frase falsa in #151**, che descriveva la vista come *«oggi inerte»*. Corretta
con la misura, non con una issue nuova.

✅ **FASE 5, 6, 7 eseguite**: contratto dell'illuminazione registrato in
[#327](https://github.com/DegrassiAaron/refactor-tactics-main/issues/327) senza aprire checkpoint prematuri;
[#784](https://github.com/DegrassiAaron/refactor-tactics-main/issues/784) esteso da «piano avversario» a ogni
dato privato, con dodici voci e il vincolo che il test misuri la **ricezione**, non il rendering; le epic
#773–#778 verificate — tutte OPEN, milestone corrette, **40 checkpoint** già presenti: la ladder v0.5 → v1.0 era
**già completa** e non è stata toccata.

➕ **Ciò che il documento non chiedeva, e che l'audit ha trovato.** Il buco non era in fondo alla roadmap ma nel
mezzo: **dieci epic fra v0.2 e v0.4 non possedevano alcun checkpoint**. Su richiesta esplicita d'autore sono
state aperte **31 issue** ([#1557](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1557) →
[#1587](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1587)).

🔴 **E quell'apertura anticipa un vincolo che nessuno ha revocato.** Tutte e dieci le epic dichiarano *«non si
apre prima dei 15 gate della v0.1»*, e i gate sono stati misurati lo stesso giorno: **7 ✅ · 4 🟡 · 3 ⏳** —
`G10`, `G11` e `G14` mai iniziati, 63 issue v0.1 aperte. Quattro epic dicevano di più: **E28** *«non ha
checkpoint, e non è una dimenticanza»*, **E29** *«si apre solo se il thin slice E18 ha retto al playtest»*,
**E31** *«va specificata prima di essere aperta»*, **E32** *«aprirla prima significherebbe decidere il formato
competitivo per inerzia di roadmap»*. Ogni issue creata porta il vincolo scritto nel proprio corpo: sono
**tracciate, non pronte**.

---

## Obiettivo

Lavora sul repository GitHub:

`DegrassiAaron/refactor-tactics-main`

Devi **verificare, creare e aggiornare le issue GitHub** necessarie per mantenere una roadmap coerente da **v0.1 fino a v1.0**, con un focus operativo molto forte sulla **v0.1**.

Non creare una seconda roadmap parallela. Prima misura lo stato reale delle issue e dei documenti correnti, poi modifica solo ciò che serve.

---

## Regole operative obbligatorie

1. Usa lo stato GitHub reale come autorità per `OPEN/CLOSED`, label, milestone e relazioni.
2. Leggi prima le issue esistenti e i documenti vivi del repository.
3. Non fidarti di conteggi o stati copiati dentro descrizioni vecchie: rimisurali.
4. Non creare issue duplicate per feature già coperte.
5. Se una epic esistente copre già il tema, **aggiornala** invece di crearne una nuova.
6. Crea una nuova issue solo quando esiste un lavoro chiaramente non posseduto da nessuna issue corrente.
7. Non riaprire decisioni consolidate senza un conflitto misurato.
8. Non spostare lavoro futuro nella v0.1 solo per completezza teorica.
9. Ogni nuova issue deve avere:
   - Why / problema misurato;
   - Scope;
   - Out of scope;
   - dipendenze;
   - Definition of Done falsificabile;
   - test automatici;
   - eventuale verifica PIE/packaged;
   - privacy impact;
   - replay/determinismo impact;
   - riferimenti alle epic correlate.
10. Nessun dato privato avversario deve essere inviato a un client per poi essere semplicemente nascosto dalla UI.

---

# FASE 1 — Audit prima di scrivere

Esegui almeno:

```bash
gh repo view DegrassiAaron/refactor-tactics-main

gh issue list \
  --repo DegrassiAaron/refactor-tactics-main \
  --state all \
  --limit 2000
```

Controlla in particolare:

- #14 — epic principale v0.1
- #26 — E12 determinismo/QA/release
- #151 — E13 conoscenza parziale
- #159 — CP 13.4 rumore
- #160 — CP 13.5 bot + HUD
- #1496 — regola temporale della conoscenza
- #1497 — leak tracce movimento
- #1525 — leak playback movimento
- #327 — E27 percezione completa v0.3
- #773 — E40 networking v0.5
- #784 — canary anti-leak
- #774 — E41 GAS v0.6
- #775 — E42 dedicated v0.7
- #776 — E43 balance/soak v0.8
- #777 — E44 freeze v0.9
- #778 — E45 production gate v1.0

Leggi anche i documenti vivi rilevanti, se presenti:

```text
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-post-v0.1.md
docs/roadmap/roadmap-checkpoint.md
docs/gameplay/brief-conoscenza-parziale.md
docs/technical/systems/conoscenza-parziale-visibile-spec.md
docs/technical/systems/progettazione-hud.md
docs/decisions/RT_PDR_00_Decision_Log.md
docs/OPEN_DECISIONS.md
```

Se un path è cambiato, cercalo. Non usare documenti archiviati come autorità corrente.

---

# FASE 2 — Nuova issue v0.1: VisionRange + SightBudget

## Prima verifica se esiste già

Cerca issue che coprano esplicitamente:

- `VisionRange`
- `SightBudget`
- range visivo deterministico
- distanza visiva per personaggio
- close awareness 360°
- cono visivo 120°
- integrazione VisionRange → TeamKnowledge

Se esiste già una issue equivalente, **aggiornala** e NON crearne una seconda.

Se non esiste, crea una issue sotto E13 (#151).

### Titolo consigliato

`CP E13.8 · VisionRange e SightBudget deterministico: la vista aumenta e diminuisce senza RNG`

### Target

- Release: `v0.1`
- Epic: `#151`
- Tipo: checkpoint
- Priorità: `P2` salvo diversa classificazione già consolidata nel repository
- Milestone: quella della v0.1

### Obiettivo

Rendere `VisionRange` una statistica realmente consumata dalla percezione, mantenendo LOS e orientamento come vincoli separati e deterministici.

### Formula baseline

Usare costi interi/fixed-point, non float competitivi:

```text
SightBudget =
    VisionRange * 100
  + ObserverModifiers
  + TargetCellIllumination
  + TemporaryEffects
  - DarknessPenalty
  - PathObscurity
  - TargetConcealment

Visible =
    DistanceCost <= SightBudget
    AND InVisionArc
    AND GeometricLOS == Clear
```

Baseline v0.1:

```text
FrontVisionArc = 120°
CloseAwareness = 2 celle / 360°
1 cella = 100 punti
TargetCellIllumination = 0
DarknessPenalty = 0
RNG = 0
```

La v0.1 prepara gli extension point ma **NON implementa ancora illuminazione gameplay dinamica**.

### Vincoli

- `VisionRange` modifica la distanza massima percepibile.
- `VisionRange` non deve bypassare muri o LOS bloccata.
- `VisionRange` non deve bypassare l'arco frontale oltre il Close Awareness.
- La percezione base non usa RNG.
- Non duplicare il LOS dentro il renderer/UI.
- Il risultato confluisce nella `TeamKnowledge` autorevole.
- UI e bot consumano la stessa conoscenza filtrata.

### DoD minimo

- [ ] `VisionRange` dell'eroe attraversa il boundary dati → unità → percezione.
- [ ] Il budget visivo usa interi/fixed-point.
- [ ] Un eroe con `VisionRange` maggiore vede una cella che uno con valore minore non vede, a parità di tutto.
- [ ] Una parete opaca blocca entrambi indipendentemente da `VisionRange`.
- [ ] Una cella dietro l'osservatore oltre Close Awareness non è visibile solo perché è nel range.
- [ ] Close Awareness 360° funziona entro il raggio deciso.
- [ ] Nessun RNG influenza la classificazione base.
- [ ] Il risultato alimenta `FRTTeamKnowledge`/struttura equivalente corrente, non un secondo stato parallelo.
- [ ] Bot e HUD ricevono lo stesso risultato autorizzato.
- [ ] Nessun planned intent avversario viene consultato per determinare la visibilità.

### Test minimi

Nomi indicativi, adattali alla tassonomia corrente senza duplicare test equivalenti:

```text
RefactorTactics.Perception.VisionRangeChangesDetectionDistance
RefactorTactics.Perception.WallBlocksRegardlessOfVisionRange
RefactorTactics.Perception.FrontArcConstrainsLongRangeVision
RefactorTactics.Perception.CloseAwarenessIsOmnidirectional
RefactorTactics.Perception.BaseVisionUsesNoRandomness
RefactorTactics.Perception.VisionFeedsTeamKnowledge
```

Aggiungere verifica di mutazione almeno su:

- bypass LOS;
- bypass facing;
- ignorare `VisionRange`.

---

# FASE 3 — Aggiorna E13 #151

Aggiorna **#151** senza distruggerne la cronologia utile.

Deve risultare chiaro che E13 possiede la pipeline:

```text
Hero VisionRange
      ↓
SightBudget deterministico
      ↓
Facing / Vision Arc
      ↓
Geometric LOS
      ↓
TeamKnowledge
      ↓
Targeting / Bot / HUD sanitizzati
```

Aggiungi E13.8 alla lista dei checkpoint solo se la issue è realmente nuova.

Non dichiarare E13 chiusa finché i residui reali non sono chiusi.

Segnala nel corpo, in modo sintetico, che la v0.1 prepara ma non implementa ancora:

- luce dinamica;
- blackout;
- torce/lampade;
- propagazione luminosa;
- stealth avanzato basato sull'illuminazione.

---

# FASE 4 — Focus v0.1: ordina il lavoro reale

Rileggi #1496, #1497 e #1525.

La priorità operativa è:

```text
A. Leak di conoscenza/presentazione vivi
   #1496 → #1497 → #1525

B. Percezione v0.1
   VisionRange/SightBudget
   #159 rumore → contatto incerto
   #160 HUD/bot sulla conoscenza parziale

C. Release gates
   playtest hex
   KPI
   packaged Development/Shipping
   determinismo/replay
   chiusura E12 / v0.1
```

Non cambiare automaticamente priorità/label senza verificare le dipendenze correnti.

Se #1496 è già sostanzialmente risolta da una decisione atterrata, aggiorna il corpo invece di duplicare la decisione.

## Regola fondamentale

Un leak di presentazione che mostra:

- posizione nemica;
- percorso nemico;
- playback nemico;
- facing privato;
- planned intent;

è un blocker di correttezza della conoscenza, non semplice polish.

---

# FASE 5 — Aggiorna E27 #327, ma NON aprire checkpoint prematuri

E27 v0.3 esiste già. Non creare una seconda epic.

Aggiungi una sezione futura tipo:

## Illuminazione gameplay dinamica — dopo la baseline v0.1

```text
Cell Illumination
      ↓
SightBudget modifier
      ↓
TeamKnowledge refresh ai boundary deterministici
```

Scope futuro E27:

- valore illuminazione per cella;
- sorgenti statiche e dinamiche;
- lampade;
- fuoco come possibile sorgente luminosa;
- blackout;
- effetti temporanei di luce/oscurità;
- modificatori positivi/negativi al `SightBudget`;
- stato luminoso rappresentato in snapshot/hash/TurnLog se competitivo;
- refresh solo a boundary deterministici, non dipendente dal frame;
- nessun RNG nascosto nella percezione base.

### NON fare ora

Non aprire automaticamente CP E27.x dedicati se E27 stessa dichiara che i checkpoint di percezione pura si aprono solo dopo i gate v0.1.

Registra il contratto nell'epic, poi lascia l'apertura dei checkpoint al momento corretto.

---

# FASE 6 — Aggiorna #784 Canary anti-leak

La issue attuale verifica soprattutto il planning avversario.

Estendi il concetto di canary a **qualsiasi informazione privata che il client non è autorizzato a ricevere**.

Il gate deve coprire almeno:

```text
- planned intents avversari
- path pianificati avversari
- facing pianificato avversario
- posizione reale di unità non conosciute
- rotte reali di movimento non osservate
- payload di playback non autorizzato
- TeamKnowledge canonica della squadra avversaria
- eventuali target/trigger futuri privati
```

### Regola

Il test deve fallire se il client **riceve** il dato proibito, anche se la UI non lo mostra.

Non è sufficiente:

```text
server replica tutto → client nasconde
```

Deve essere:

```text
server canonical state
      ↓ filtro/relay autorizzato
client riceve solo dati consentiti
```

Mantieni il requisito di mutazione: rimuovendo il filtro, il canary deve diventare rosso.

---

# FASE 7 — Verifica roadmap v0.5 → v1.0

Non creare nuove epic se queste esistono ancora e coprono la release:

```text
v0.5  #773  E40 · networking / authoritative simultaneous turn
v0.6  #774  E41 · GAS runtime, non authority
v0.7  #775  E42 · dedicated server
v0.8  #776  E43 · batch / balance / soak
v0.9  #777  E44 · feature freeze / hardening
v1.0  #778  E45 · production gate
```

Per ciascuna verifica:

- stato OPEN/CLOSED;
- milestone corretta;
- dipendenze;
- eventuali riferimenti a sistemi rimossi;
- privacy/perception coerenti con il modello corrente.

Non riscrivere completamente i corpi solo per pulizia cosmetica.

Aggiungi note solo quando correggono una prescrizione operativa falsa o un buco reale.

---

# FASE 8 — Roadmap finale da ottenere

La forma concettuale deve restare:

```text
v0.1
├─ vertical slice 2v2
├─ VisionRange + LOS + TeamKnowledge
├─ conoscenza parziale / rumore
├─ HUD e playback senza leak
├─ determinismo
└─ packaged internal release
        │
        ▼
v0.2
├─ mappe/interazioni/porte
└─ fondazioni content/data ulteriori
        │
        ▼
v0.3
├─ percezione completa
├─ memoria/belief
├─ stealth
└─ illuminazione gameplay dinamica
        │
        ▼
v0.4
├─ scala/Operations
└─ formato competitivo da validare
        │
        ▼
v0.5
└─ networking authoritative + privacy canary
        │
        ▼
v0.6
└─ GAS runtime sopra autorità già verificata
        │
        ▼
v0.7
└─ dedicated server + reconnect/resync
        │
        ▼
v0.8
└─ balance batch + performance + soak
        │
        ▼
v0.9
└─ freeze + hardening + migration
        │
        ▼
v1.0
└─ production readiness gate
```

---

# FASE 9 — Comandi GitHub

Preferisci `gh`.

## Lettura

```bash
gh issue view 151 --repo DegrassiAaron/refactor-tactics-main
gh issue view 327 --repo DegrassiAaron/refactor-tactics-main
gh issue view 784 --repo DegrassiAaron/refactor-tactics-main
```

## Creazione

Solo se E13.8 non esiste già:

```bash
gh issue create \
  --repo DegrassiAaron/refactor-tactics-main \
  --title "CP E13.8 · VisionRange e SightBudget deterministico: la vista aumenta e diminuisce senza RNG" \
  --label "v0.1" \
  --label "checkpoint" \
  --label "P2" \
  --body-file <FILE_TEMPORANEO_MD>
```

Se il repository usa una milestone GitHub v0.1, assegnala dopo averne verificato il numero/nome reale.

## Aggiornamento

Usa `gh issue edit` dopo aver salvato il corpo corrente e aver prodotto un diff leggibile.

Non cancellare note storiche utili solo per rendere il testo più corto.

---

# FASE 10 — Verifica dopo le modifiche

Dopo ogni write:

```bash
gh issue view <N> --repo DegrassiAaron/refactor-tactics-main
```

Alla fine controlla che:

- non esista una seconda E13.8 equivalente;
- #151 citi correttamente VisionRange/SightBudget;
- #327 contenga l'estensione di illuminazione futura senza checkpoint prematuri;
- #784 protegga anche perception/knowledge leak;
- le epic #773–#778 siano ancora la ladder principale fino alla 1.0;
- nessuna issue futura sia stata spostata dentro la v0.1 per errore.

---

# OUTPUT FINALE RICHIESTO A CLAUDE

Restituisci un report sintetico:

```text
ISSUE CREATE
- #NNNN — titolo

ISSUE UPDATE
- #151 — cosa è cambiato
- #327 — cosa è cambiato
- #784 — cosa è cambiato

NON MODIFICATE
- issue controllate ma già corrette

FOCUS v0.1
1. prossimo blocker
2. secondo blocker
3. terzo blocker

ROADMAP v0.1 → v1.0
- conferma delle epic/release

RISCHI / CONTRADDIZIONI TROVATE
- elenco breve con riferimenti alle issue
```

Non dichiarare una modifica riuscita se `gh` ha fallito.

