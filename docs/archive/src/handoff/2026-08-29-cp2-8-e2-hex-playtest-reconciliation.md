# Claude Task Brief — RefactorTactics
## Allineamento documentazione + GitHub issue per CP 2.8 / E2 — Playtest partita hex

**Data del brief:** 2026-08-29  
**Repository:** `DegrassiAaron/refactor-tactics-main`  
**Scope:** solo documentazione, checklist PIE e issue GitHub.  
**NON modificare:** C++, Blueprint, `.uasset`, `.umap`, gameplay, bilanciamento o asset.

---

# 1. Obiettivo

Implementare nel repository e nelle issue GitHub quanto deciso durante la revisione guidata della seduta manuale Unreal relativa a:

- **Issue #38** — `CP 2.8 — Playtest della partita hex (sessione D)`
- **Epic #16** — `[EPIC v0.1] E2 — Parità hex del substrato`

La finalità è avere **una sola procedura manuale coerente, aggiornata e realmente eseguibile** per chiudere E2, evitando:

- cinque aperture separate dell'Editor;
- riesecuzione inutile di test già verdi;
- vecchi riferimenti a “sessione D” come unità indipendente;
- ambiguità `9 / 14 / 15` voci `PIE-HEXPLAY`;
- vecchio requisito “vittoria obbligatoria”;
- criteri che consentono 🟡 mentre il gate E2 corrente richiede il completamento del perimetro hex;
- istruzioni che richiedono di costruire a mano `L_HexArena` per questo playtest.

Il risultato deve permettere a un umano di aprire **UE 5.8 una sola volta** e completare i residui del playtest tramite più cicli Play/Stop all'interno della stessa apertura.

---

# 2. Gerarchia delle sorgenti

Prima di modificare qualunque cosa, rileggi lo stato corrente di:

1. GitHub **#16**
2. GitHub **#38**
3. `docs/technical/test-manuali-pie.md`
4. `docs/roadmap/editor-sessions.yaml`
5. eventuali riferimenti a `PIE-HEXPLAY` in:
   - `docs/roadmap/roadmap-checkpoint.md`
   - `docs/roadmap/roadmap-v0.1.md`
   - `docs/roadmap/v0.1-definition-of-done.md`
   - `docs/tooling/scenario-map.md`, se esiste ancora con questo path

Non assumere che il testo riportato in questo brief sia più recente del repository.

**Regola:** se c'è conflitto, privilegiare la decisione esplicita più recente già registrata nel repository/GitHub e documentare l'allineamento.

---

# 3. Decisioni da rendere canoniche

## 3.1 Il perimetro E2 è di 14 `PIE-HEXPLAY`

Il gate aggiornato di **#16** definisce come perimetro E2 queste 14 verifiche:

- `PIE-HEXPLAY-1`
- `PIE-HEXPLAY-2`
- `PIE-HEXPLAY-3`
- `PIE-HEXPLAY-3b`
- `PIE-HEXPLAY-4`
- `PIE-HEXPLAY-4b`
- `PIE-HEXPLAY-5`
- `PIE-HEXPLAY-6`
- `PIE-HEXPLAY-6b`
- `PIE-HEXPLAY-6c`
- `PIE-HEXPLAY-7`
- `PIE-HEXPLAY-8`
- `PIE-HEXPLAY-9`
- `PIE-HEXPLAY-10`

### Fuori perimetro E2

`PIE-HEXPLAY-11` **non appartiene a E2**.

Motivo:
- verifica la presentazione temporale degli attacchi Blast;
- appartiene a **E21 / presentazione**;
- non è parità del substrato hex.

Correggere ogni `15` residuo che pretenda di essere il numero delle voci E2.

Non correggere automaticamente occorrenze storiche o note retrospettive che descrivono correttamente un vecchio stato: distinguere **dato storico** da **regola corrente**.

---

## 3.2 Una sola apertura Editor per U2 → U6

Le sedute:

- `U2`
- `U3`
- `U4`
- `U5`
- `U6`

condividono il setup.

Devono essere documentate come:

> **una sola apertura di Unreal Editor**, con più Play/Stop PIE se necessario.

Non significa un singolo avvio PIE.

La struttura consigliata è:

1. apertura UE;
2. setup comune;
3. Run 1;
4. Stop;
5. Run 2;
6. Stop;
7. Run 3;
8. Stop;
9. Run 4;
10. rilettura finale del gate;
11. controllo log;
12. registrazione degli esiti.

---

## 3.3 Non costruire `L_HexArena` per CP 2.8

Per il playtest E2 usare:

`MapSource = GeneratedTestArena`

La fixture deve già fornire:

- esagono r=4;
- ostacoli;
- blocco LOS;
- fango costo 3;
- layer 0;
- piattaforma layer 1;
- una transizione fra layer.

Per CP 2.8:

- **nessun `.uasset` da creare**;
- **nessun `.umap` da creare**;
- **nessuna Binary Asset Lease necessaria**;
- `L_HexArena` resta lavoro separato legato alle sedute/issue che producono l'asset autore.

---

## 3.4 UE bloccata: 5.8

La procedura deve dichiarare:

- **Unreal Engine 5.8**
- progetto `RefactorTactics.uproject`
- se l'Editor chiede la versione engine, scegliere la 5.8 configurata localmente;
- eventuale GUID locale riscritto in `EngineAssociation` non va committato.

Non introdurre altre versioni UE.

---

## 3.5 Il gate non richiede una vittoria nel free-run

Il vecchio requisito:

> partita completa fino alla vittoria

è superato.

Il criterio corrente per `PIE-HEXPLAY-10` / E2 è:

> partita completa fino a un **esito dichiarato**.

Sono validi per il substrato E2:

- eliminazione;
- pareggio allo scadere del `RoundLimit`.

Il free-run default con `RoundLimit = 12` può finire pari.

Non riaprire `PIE-HEXPLAY-10` solo perché il free-run non produce eliminazione.

La vittoria per eliminazione, quando serve come evidenza di formato/autobattle, appartiene agli scenari dedicati e non deve essere usata come requisito impossibile del gate E2.

---

# 4. Stato da cui partire

L'ultimo stato consolidato discusso è:

### Già verdi

- `PIE-HEXPLAY-1`
- `PIE-HEXPLAY-2`
- `PIE-HEXPLAY-3`
- `PIE-HEXPLAY-5`
- `PIE-HEXPLAY-9`
- `PIE-HEXPLAY-10`

### Residui manuali E2

- `PIE-HEXPLAY-3b`
- `PIE-HEXPLAY-4`
- `PIE-HEXPLAY-4b`
- `PIE-HEXPLAY-6`
- `PIE-HEXPLAY-6b`
- `PIE-HEXPLAY-6c`
- `PIE-HEXPLAY-7`
- `PIE-HEXPLAY-8`

**ATTENZIONE:** prima di scrivere il nuovo stato nei documenti, rimisurarlo sul branch corrente.  
Non copiare il numero `6/14` o `8 residui` se nel frattempo alcune voci sono state eseguite.

---

# 5. Procedura canonica della seduta

Inserire nella documentazione una procedura equivalente a questa.

## Preparazione comune

1. Aprire `RefactorTactics.uproject` in **UE 5.8**.
2. Aprire il livello di prova adatto (`L_DevSandbox` o quello dichiarato dal registro corrente).
3. Verificare `RTGameMode` come GameMode Override.
4. Verificare:
   - `MapSource = GeneratedTestArena`
   - `RoundLimit = 12`
5. Aprire **Output Log**.
6. Non costruire né modificare `L_HexArena`.
7. Non salvare asset binari per questa seduta.
8. Mantenere una sola apertura dell'Editor fino alla fine di U6.

---

# 6. Run PIE da documentare

## RUN 1 — Movimento, target rejection, Dash

### `PIE-HEXPLAY-4` — playback movimento

Verificare:

- percorso di più celle;
- lock-in con **Spazio**;
- movimento visivo cella per cella;
- nessun teleport;
- nessuna deriva accumulata;
- posizione finale visiva centrata sulla cella logica finale.

La logica è già coperta headless: il PIE giudica soprattutto la **fluidità del playback**.

---

### `PIE-HEXPLAY-3b` — motivo corretto del rifiuto

Verificare almeno:

1. bersaglio fuori portata:
   - messaggio equivalente a `fuori portata (max N)`;
   - non deve essere classificato come coperto;

2. bersaglio dietro una cella `bBlocksLineOfSight`:
   - messaggio equivalente a `coperto / nessuna linea di tiro`;

3. se facilmente osservabile:
   - abilità non pronta / cooldown / energia insufficiente deve fallire col reason corretto prima di classificazioni spaziali.

Il residuo noto era soprattutto la metà **copertura/LOS**.

---

### `PIE-HEXPLAY-4b` — Dash

Verificare:

- Dash risolto nella fase corretta;
- Dash prima del Blast;
- movimento visivo lungo la traiettoria;
- nessun teleport alla destinazione;
- destinazioni invalide/bloccate/occupate gestite senza movimento illegale;
- compatibilità con il normale movimento del turno, se la configurazione lo consente.

La sequenza di fase era già osservata nel log; il residuo manuale era il playback visivo.

---

## RUN 2 — Combat / LOS / forme / spinta

### `PIE-HEXPLAY-6` — LOS esagonale

Non ricostruire manualmente la geometria se esistono scenari.

Usare, se ancora presenti:

- `Combat.BlockedByWall`
- `Combat.LineHitsThrough`
- `Visual.Map.HighGroundNoBonus`

Verificare:

1. muro blocca il tiro;
2. spostandosi lateralmente / nello scenario positivo il tiro passa;
3. ostacolo su layer differente segue la regola di elevazione prevista;
4. il motivo è **comprensibile a schermo**, non solo corretto nel log.

La verifica manuale deve rispondere alla domanda:

> Il giocatore capisce perché il colpo non parte?

Se il log dice “nessuna linea di tiro” ma il muro/ostacolo non si legge visivamente, la parte UX della voce non è verde.

---

### `PIE-HEXPLAY-6b` — forme d'attacco

Roster v0.1:

**Gadget**
- `LinearDischarge` → Line
- `Overload` → Circular AoE

**Phase**
- `PressureJet` → Line
- `CircularTide` → Circular AoE

Verificare:

- linea;
- area r1;
- preview leggibile prima del lock-in;
- celle colpite coerenti con l'esito;
- combat log con un esito per bersaglio.

`CircularTide`:
- cura alleati;
- bagna nemici.

Per friendly fire usare `Overload`, non `CircularTide`.

### Cono

Non inventare una prova Cone.

Il roster v0.1 non possiede un'abilità Cone.

Se questa condizione è ancora vera nel codice corrente:
- documentarla come **non verificabile in partita**;
- lasciare la copertura automatica come rete del sistema `HexCone`.

---

### `PIE-HEXPLAY-6c` — Push

Usare:

- Phase — `PressureJet`
- oppure Riktor — `Ram`

Verificare:

- direzione su uno dei 6 assi hex;
- Push 1 visivamente corretto;
- cella dietro libera → spinta;
- cella dietro occupata → nessun movimento illegale;
- ostacolo → nessun movimento illegale;
- bordo mappa → nessun movimento fuori board;
- playback come spostamento leggibile, non teleport/sobbalzo;
- se facilmente riproducibile: spinte opposte / contese seguono la regola corrente.

Non pretendere un test `Push 2`: non è più disponibile nel roster v0.1.

---

## RUN 3 — Bot

### `PIE-HEXPLAY-7`

Con almeno una unità bot-controlled verificare:

- nessuna mossa illegale;
- nessuna destinazione occupata proposta come valida;
- rispetto budget;
- coordinate del log in `(q,r,L)`;
- comportamento sensato osservando la partita.

Il residuo manuale non è soltanto:

> il bot stampa coordinate assiali.

Deve rispondere anche a:

> gli score e le scelte sembrano coerenti guardando la situazione?

Osservare, quando applicabile:

- melee chiude distanza;
- kiter conserva distanza;
- preferenza per riparo;
- uso sensato di Dash/attacco.

Non modificare i pesi bot durante questa task a meno che una issue separata lo richieda esplicitamente.

---

## RUN 4 — Multilivello

### `PIE-HEXPLAY-8`

Usare la transizione già presente nella `GeneratedTestArena`.

Verificare:

- path che attraversa L0 → L1;
- unità cambia realmente quota;
- playback segue la transizione;
- posizione finale alla quota `LayerHeight` corretta;
- niente teleport;
- niente unità che scorre sul piano basso sotto il layer alto.

### Non fare in questa seduta

Non rimuovere l'arco dalla `GeneratedTestArena`.

La transizione è creata da codice; la rimozione della struttura appartiene a test/scenari separati ed è già coperta headless.

---

# 7. Verifiche collaterali da sfruttare nella stessa apertura

Poiché U2–U6 condividono il setup, durante la stessa apertura conviene consuntivare eventuali voci ancora aperte già assegnate a quelle sedute.

In particolare rileggere lo stato corrente di:

- `PIE-PREVIEW-PERSIST`
- `PIE-FACING-1`
- `PIE-AI-01`
- `PIE-AI-02`
- `PIE-AI-03`
- `PIE-AI-04`
- `PIE-AI-05`
- eventuali voci camera U2 non ancora verdi

## `PIE-FACING-1`

Il blocker storico dei cilindri dovrebbe essere superato se `BP_Unit_*` sono correttamente agganciati.

Se la precondizione è soddisfatta, verificare:

- mesh orientata come il facing logico;
- movimento → facing sull'ultimo passo;
- attacco → facing verso bersaglio;
- log coerente con la direzione;
- premere **Spazio** per saltare il playback e verificare che lo snap finale mantenga lo stesso facing.

Non dichiarare verde `PIE-FACING-1` solo perché il log è corretto: la voce esiste proprio per confrontare **mesh visiva e facing logico**.

---

# 8. Chiusura della seduta

Prima di chiudere Unreal:

## 8.1 Rilettura completa del gate

Rileggere tutte le 14 voci E2 insieme:

- `-1`
- `-2`
- `-3`
- `-3b`
- `-4`
- `-4b`
- `-5`
- `-6`
- `-6b`
- `-6c`
- `-7`
- `-8`
- `-9`
- `-10`

Non usare `PIE-HEXPLAY-11` per determinare lo stato E2.

---

## 8.2 Log

Verificare l'assenza di:

- `ensure`
- `check`
- crash

Non allegare l'intero `Saved/Logs/*.log` al repository.

Per ogni esito manuale, riportare nel registro:

- cosa è stato osservato;
- data;
- scenario/run usato;
- eventuale reason code;
- righe pertinenti del log.

---

## 8.3 Stato

Non promuovere una voce a ✅ perché:

- “coperta headless”;
- “dovrebbe funzionare”;
- “il log sembra giusto” quando il criterio è visivo.

Una voce manuale è ✅ solo dopo l'osservazione umana richiesta dalla sua definizione.

---

# 9. File repository da modificare

## Obbligatorio

### `docs/technical/test-manuali-pie.md`

Aggiornare:

- stati reali dopo la seduta;
- note obsolete;
- istruzioni residue incoerenti;
- `PIE-HEXPLAY-10` se contiene ancora riferimenti contraddittori alla vittoria obbligatoria;
- eventuali note `15` relative al gate E2;
- eventuale testo che chiama “sessione D” una singola entità invece del gruppo U2–U6;
- eventuale stato di `PIE-FACING-1` non più coerente con la presenza delle skeletal.

Preservare la storia utile:
- non cancellare reperti passati;
- marcarli chiaramente come `stato precedente` quando necessario.

---

### `docs/roadmap/editor-sessions.yaml`

Allineare U2–U6.

Punti da controllare:

- `shares_setup_with`;
- `done_when`;
- numero `14`, non `15`, per il gate E2;
- distinzione fra:
  - una apertura Editor;
  - più run PIE;
- U6 deve chiudere la seduta tramite rilettura complessiva del gate E2.

Non introdurre una nuova sessione se U2–U6 descrivono già correttamente la sequenza.

---

# 10. Issue GitHub da aggiornare

## #38 — CP 2.8

Aggiornare il corpo o aggiungere un commento di reconciliation, secondo lo stile attuale del repository.

Deve risultare inequivocabile che:

1. il gate E2 usa **14** `PIE-HEXPLAY`;
2. `-11` è fuori da E2;
3. U2 → U6 condividono una sola apertura Editor;
4. la sessione usa `GeneratedTestArena`;
5. non si costruisce `L_HexArena`;
6. `RoundLimit = 12`;
7. `PIE-HEXPLAY-10` accetta un esito dichiarato;
8. lo stato corrente viene rimisurato, non copiato;
9. i residui manuali sono raccolti per run/setup e non per voce;
10. nessun `ensure/check/crash`.

### Importante

Il corpo storico di #38 consentiva 🟡 per alcune voci.

Il gate E2 di #16 è stato successivamente riscritto.

Non lasciare due Definition of Done concorrenti.

Se il criterio corrente è **14/14 ✅**, dichiararlo esplicitamente in #38 oppure spiegare formalmente perché #38 conserva una semantica differente.

La preferenza di questa task è **allineare #38 a #16**.

---

## #16 — E2

Verificare che il testo sia già coerente.

Se necessario aggiornare solo:

- stato misurato;
- link alla procedura canonica;
- eventuali riferimenti stantii.

Non riscrivere inutilmente la storia dell'epic.

---

# 11. Documenti secondari da cercare

Eseguire una ricerca per:

- `PIE-HEXPLAY`
- `15`
- `sessione D`
- `fino alla vittoria`
- `vittoria`
- `GeneratedTestArena`
- `U2`
- `U6`
- `M6.8`
- `CP 2.8`

Correggere solo i riferimenti **normativi correnti**.

Non alterare senza motivo:

- decision log storico;
- changelog;
- citazioni che spiegano un vecchio errore;
- commenti retrospettivi.

---

# 12. Acceptance Criteria

La task è completata quando:

- [ ] esiste un solo perimetro E2: **14 voci**;
- [ ] `PIE-HEXPLAY-11` è chiaramente fuori E2;
- [ ] #16 e #38 non hanno DoD incompatibili;
- [ ] U2–U6 sono descritti come una sola apertura Editor;
- [ ] la procedura distingue apertura Editor da singolo PIE;
- [ ] `GeneratedTestArena` è il setup di CP 2.8;
- [ ] nessuna istruzione chiede di creare `L_HexArena` per #38;
- [ ] `RoundLimit = 12`;
- [ ] il free-run non richiede eliminazione;
- [ ] `PIE-HEXPLAY-10` usa “esito dichiarato”;
- [ ] i residui manuali sono organizzati in run riutilizzabili;
- [ ] `test-manuali-pie.md` resta il source of truth degli esiti;
- [ ] `editor-sessions.yaml` resta il source of truth della sequenza di lavoro;
- [ ] nessuno stato manuale viene promosso senza evidenza PIE;
- [ ] i conteggi vengono rimisurati con il metodo canonico del repository;
- [ ] nessun asset binario viene modificato;
- [ ] nessun file C++ viene modificato;
- [ ] nessun Blueprint viene modificato.

---

# 13. Validazione finale

Prima del commit:

1. controllare il diff;
2. verificare che non siano presenti:
   - `.uasset`
   - `.umap`
   - `RefactorTactics.uproject` modificato col GUID locale;
3. cercare incoerenze rimaste:
   ```bash
   git grep -n "PIE-HEXPLAY" -- docs/
   git grep -n "sessione D" -- docs/
   git grep -n "fino alla vittoria" -- docs/
   ```
4. rimisurare gli stati dal registro con il comando canonico già presente in `test-manuali-pie.md`;
5. verificare link Markdown;
6. verificare YAML valido per `editor-sessions.yaml`;
7. controllare `git diff --check`.

Se nel repository esistono validator documentali correnti, eseguirli.

Non reintrodurre script rimossi dal repository.

---

# 14. Commit suggerito

Se la modifica è solo documentazione:

```text
docs(playtest): reconcile CP 2.8 with E2 hex gate
```

Se vengono aggiornati anche gli esiti reali dopo una seduta PIE:

```text
docs(playtest): record remaining E2 hex verification results
```

Separare reconciliation documentale ed esiti osservati in due commit se il repository preferisce tracciare distintamente:

1. regola/procedura;
2. evidenza della seduta.

---

# 15. Output richiesto a Claude

Alla fine Claude deve riportare:

## File modificati
Elenco esatto.

## Issue aggiornate
- #38
- #16, solo se necessario.

## Incoerenze trovate
Con indicazione:
- file/issue;
- testo precedente;
- correzione;
- motivo.

## Stato E2 misurato
Formato:

```text
E2 HEX gate: X/14 ✅
Residui:
- PIE-HEXPLAY-...
```

## Validazioni eseguite
Con risultato.

## Cose NON fatte
Esplicitare:
- nessun C++;
- nessun Blueprint;
- nessun `.uasset/.umap`;
- nessun tuning gameplay.

---

# 16. Vincolo finale

Non “semplificare” la documentazione cancellando la storia delle scoperte.

RefactorTactics usa il registro manuale anche come memoria dei difetti trovati durante i playtest.

L'obiettivo è:

> **una regola corrente non ambigua + storia conservata come storia.**

Se una vecchia nota è falsa oggi ma utile per spiegare perché il criterio è cambiato:
- conservarla;
- marcarla chiaramente come stato precedente;
- non lasciarla competere con il comportamento corrente.
