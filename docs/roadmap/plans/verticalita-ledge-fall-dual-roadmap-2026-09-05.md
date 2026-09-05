# Verticalità tattica — Ledge, Fall e Forced Movement · doppia roadmap Codice / Editor

> `CURRENT` · **Stato**: audit chiuso, capability aperta, implementazione dietro una decisione di release
> **Data**: 2026-09-05
> **HEAD dell'audit**: `026850c0` (= `origin/main` dopo `fetch`, 2026-09-05)
> **Oggetto**: l'handoff esterno *Dual Roadmap: Code/Architecture + Editor/MCP/User — Verticality, Ledge,
> Fall, Forced Movement* (2026-09-04), letto come mandato di pianificazione e non come specifica di gameplay.
> **Owner di capability**: [#2388](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2388)
> ⚠️ **Il documento sorgente non è nel repository e non entra**: questo referto è l'unico posto in cui il suo
> contenuto resta citabile.

---

## 1. Il verdetto in una riga

Il mandato descrive una capability che il repository **non possiede davvero** — `Fall`, `OpenLedge`,
`Railing`, `LastStable`, `FallEffects`, `ImpactEffects` hanno **zero occorrenze** in `Source/` — ma chiede di
consegnarla come **backlog v0.1**, mentre la sede canonica la colloca su release `future` con owner
*«nessuno»*. Il gap è reale, la release no.

| | Rilievo | Gravità |
|---|---|:--:|
| **R1** | il mandato chiede issue **v0.1**; `roadmap-post-v0.1.md` colloca la verticalità su `future`/`DEFER` | 🔴 |
| **R2** | §2.1 indirizza `OpenLedge`/`Railing` su `FRTHexEdge`, che è **additivo** e non può negare un'adiacenza | 🔴 |
| **R3** | §2.9 presenta `LastStableCell` come lavoro nuovo: è **il comportamento di oggi** | 🟠 |
| **R4** | §11.H chiede di non decidere `MOV-4`, **chiusa il 2026-09-04** da `D-325` | 🟡 |
| **R5** | «caduta» è un **omonimo**: nel repository è `Prone` (`D-319`), non la gravità | 🟠 |
| **R6** | §2.6 vieta *«automatic Stagger/Prone/Stun»*; `D-319` ha appena introdotto una regola che li applica | 🟠 |
| **R7** | il mandato non nomina `ERTDisplacementBlockReason`, che è il vocabolario d'esito già serializzato | 🟡 |

**Del mandato regge la struttura** — due lane, un solo grafo di dipendenze, Editor tardivo e batch — e regge
la decisione di design §2.9. **Non regge la collocazione di release**, ed è ciò che questo referto instrada
invece di risolvere.

---

## 2. Come è stata misurata

Ogni citazione è letta con `git show origin/main:<path>` dopo `fetch`, non dal working tree: il checkout
principale era **7 commit indietro** a inizio audit e portava due `.uasset` sporchi di un'altra sessione.

```bash
git fetch --prune origin                       # origin/main = 026850c0
git grep -ilE '\bledge\b' origin/main -- docs Source Scenarios      # 1 file, in docs/archive/
git grep -il 'OpenLedge|Railing|LastStable|FallEffect' origin/main  # 0
git grep -inE '\bFall\b' origin/main -- Source                      # 0
gh search issues --repo <repo> --match title verticalit             # 0
gh search issues --repo <repo> --match title caduta                 # 0
```

⚠️ **Il primo conteggio di `ledge` in questo audit è stato sbagliato**, e vale la pena scriverlo perché la
forma dell'errore si ripete: `git grep -il ledge` restituisce **233 file** — è `knowledge` che contiene
`ledge`. Con `\bledge\b` è **1**, in `docs/archive/`, che non è autorità. Il numero pubblicato è il secondo.

---

## 3. Che cosa esiste già, e dove

Il mandato tratta la capability come greenfield. Metà del substrato esiste.

| Elemento | Dove | Stato |
|---|---|:--:|
| coordinate multilivello | `FRTCellId{X, Y, Layer}` | ✅ |
| celle su layer diversi **non** adiacenti senza arco | `FRTHexEdge`, `Map->Transitions` | ✅ |
| bordi qualificati (coperture, porte) | `FRTHexCellData::Covers` · `::Doors` · `CoverOn(Edge)` | ✅ |
| terreno `Void` | `ERTHexSurface::Void` | ✅ |
| spinta / trazione | `ARTTurnManager::ApplyDisplacements` · ramo `ERTActionEffect::Push` | ✅ |
| destinazione della spinta | `URTHexCombatLibrary::HexKnockbackDestination` | ✅ |
| vocabolario d'esito bloccato | `ERTDisplacementBlockReason` — 6 valori, serializzati nel TurnLog **v7** | ✅ |
| causa dello spostamento | `ERTDisplacementCause { Forced, Environmental }` | ✅ |
| divieto di auto-reroute | `spec-tassonomia-movimento.md` §2 — già canone su tutte le famiglie | ✅ |
| un arco per unità per micro-step | `D-305`, pinnato da [#2000](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2000) | ✅ |
| **caduta gravitazionale** | — | ❌ |
| **bordo aperto / parapetto** | — | ❌ |
| **relazione di atterraggio** | — | ❌ |

### 3.1 Il punto d'innesto è una funzione sola

```cpp
// Source/RefactorTactics/Combat/RTHexCombatLibrary.cpp:540-554
const FRTCellId Next(Current.X + StepX, Current.Y + StepY, Target.Layer);
const FRTHexCellData* Data = Map->FindCell(Next);
if (Data == nullptr || Data->bBlocksMovement || Occupied.Contains(Next))
{
    break; // bordo della mappa, ostacolo o unita': ci si ferma sulla cella libera precedente
}
```

Due fatti che il mandato non aveva misurato:

🔑 **`Target.Layer` è costante nel ciclo.** Una spinta non cambia layer, mai. La caduta non è un caso limite
del knockback: è il ramo che oggi **non esiste** dietro `Data == nullptr`.

🔑 **«ci si ferma sulla cella libera precedente» *è* `LastStableCell`.** Il fallback saturo del §2.9 non è
codice da scrivere ma comportamento da **qualificare**: oggi l'unità resta lì e non riceve nulla; il mandato
vuole che resti lì **e riceva gli effetti della caduta**. La differenza è negli effetti e nella traccia, non
nella posizione.

### 3.2 L'omonimo

Nel repository *«la caduta»* è già una cosa, e non è questa. `ApplyDisplacements` porta il commento
*«LA CADUTA (`D-319`, `#2253`): chi subisce uno spostamento forzato mentre è `Unbalanced` finisce `Prone`»*.

⛔ **Le due regole si incontrano e vanno conciliate esplicitamente**: il §2.6 del mandato vieta
*«no automatic Stagger/Prone/Stun»* da ogni caduta, ma `D-319` ha appena stabilito che uno spostamento
forzato **su unità già sbilanciata** applica `Prone`. Non è una contraddizione — la condizione è diversa — ma
è esattamente il tipo di collisione che diventa un difetto se nessuno la scrive.

---

## 4. R1 — perché non ci sono issue v0.1 in questo passaggio

Il mandato §7 dice *«only v0.1 gets granular issues in this pass»*. Presuppone che il lavoro **sia** v0.1.

Misurato in [`roadmap-post-v0.1.md`](../roadmap-post-v0.1.md), tabella del Graybox Kit:

| Cluster del kit | Proposta | Release **canonica** | Owner reale | Azione |
|---|:--:|:--:|---|---|
| 3D map / verticalità | v0.3 | **`future`** | nessuno — `RT-FEAT-MAP-VERTICALITY` è `IDEA` | `DEFER` |

e, poco sotto, esplicito: *«Due cluster non hanno una release che li possieda […] La verticalità resta senza
owner»*.

`roadmap-v0.1.md` conferma dall'altro lato: la v0.1 è un vertical slice su griglia esagonale **multilivello**
— il *substrato* c'è, con pathfinding A\* multilivello e LOS che attraversa i layer — ma nessuna delle
ventitré epic possiede la caduta, e la sezione *«Fuori dalla v0.1»* non la nomina né per includerla né per
escluderla.

∴ Creare sette issue `v0.1` avrebbe **espanso lo scope della release** contro un `DEFER` registrato, che è
precisamente ciò che il mandato §2 vieta: *«do not silently overwrite a stronger existing owner […] if
conflict exists, record it and route it through the repository decision process»*.

➡️ **La domanda è aperta come `REL-3`** in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).

⚠️ **Il lavoro non è perso, è solo non ancora numerato.** Le due roadmap sotto sono specificate al livello di
dettaglio che il mandato §7 chiede per una issue — owner, lane, dipendenze, criteri, test, evidenza,
non-scope. Quando `REL-3` si chiude, le voci diventano issue **senza rifare l'audit**.

---

## 5. ROADMAP A — Codice / Architettura

Lane `CODE`. Nessuna voce apre l'Editor.

| # | Lavoro | Dipende da | Gate di test | Editor | Stato |
|---|---|---|---|:--:|:--:|
| **A0** | riconciliazione di ownership: chi possiede bordo, resolver, traccia, validazione | — | nessuno (documentale) | no | ✅ **questo referto** |
| **A1** | modello dati: qualificazione del bordo, relazione di atterraggio, esito | A0 · `REL-3` | round-trip · ordinamento deterministico | no¹ | ⏳ |
| **A2** | runtime: attraversamento, terminazione, effetti, alternativa, fallback | A1 | Automation deterministica · invarianza per permutazione · una unità per cella | no | ⏳ |
| **A3** | traccia: catena causale in TurnLog, replay, seek | A2 | replay = esito risolto · seek al confine | no | ⏳ |
| **A4** | validazione statica dell'atterraggio isolato | A1 | atterraggio valido/isolato · muro · `Void` · occupazione runtime ininfluente | no | ⏳ |
| **A5** | dati di preview ed explainability | A2 · A3 | nessuna lettura di intento avversario | no | ⏳ |
| **A6** | regressione, mutazione, suite | A2 · A3 · A4 | suite **VALIDA** · cinque falsificazioni | no | ⏳ |
| **A7** | evidenza packaged / non visuale | A6 | secondo l'owner di release | no² | ⏳ |

¹ diventa `EARLY SMOKE` **solo** se la qualificazione del bordo richiede una forma serializzata che nessuna
fixture sa produrre. Misurato oggi: le fixture di scenario producono già celle, bordi e coperture, quindi la
condizione **non è soddisfatta**.
² `NOT RUN` finché l'owner di release non lo richiede.

### A1 — modello dati

**Owner candidato**: E23 ([#324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324)) — è
l'epic dei bordi, e `FRTHexCellData` è dove vivono coperture e porte.

⛔ **Non `FRTHexEdge`**, contro §2.1 del mandato. La struttura dichiara di sé: *«L'arco è ADDITIVO: crea un
collegamento dove non c'era. È la ragione per cui le PORTE non stanno qui ma sui bordi (CP 9.3) — negare
un'adiacenza planare richiede un oggetto sottrattivo, e questo non lo è»*. Un `OpenLedge` **qualifica** un
bordo planare esistente; un `Railing` lo **nega** in una direzione. Nessuno dei due è additivo.

Vocabolario minimo, da aggiungere **in coda** agli enum esistenti — la regola è già scritta due volte in
`RTHexCellData.h` e in `RTTurnLog.h`: *«estendere un enum non è una migrazione di formato — riordinarlo sì,
perché il valore serializzato è l'indice»*.

Criteri:
- il bordo distingue *aperto* da *parapetto* senza un secondo modello spaziale;
- la relazione di atterraggio è derivabile o dichiarata, mai dedotta da `FVector`;
- `LastStableCell` è un **risultato**, non uno stato persistito.

Non-scope: resistenza/distruttibilità del parapetto · `Hanging` · `BalconyCell`.

### A2 — runtime

**Owner**: il movement resolver — `ARTTurnManager::ApplyDisplacements` +
`URTHexCombatLibrary::HexKnockbackDestination`.

Sequenza, nell'ordine — l'ordine è il punto:

```text
spostamento forzato
  → attraversamento di bordo aperto
       → lo spostamento orizzontale TERMINA (i passi residui sono persi)
            → risoluzione della caduta
                 → [impatto sull'occupante]
                      → esito di atterraggio → posizione finale
```

Criteri, uno per uno falsificabile:
1. i passi di spinta residui **non** sopravvivono alla caduta;
2. il percorso volontario pianificato è **annullato**, senza reinterpretazione dalla nuova posizione;
3. atterraggio primario libero → effetti + collocazione;
4. atterraggio occupato con alternativa → chi cade prende gli effetti di caduta, l'occupante quelli
   d'impatto, l'occupante **resta**, chi cade va nella prima alternativa legale in ordine **canonico
   dichiarato** a partire dal Facing dell'occupante;
5. atterraggio saturo → stessi effetti, chi cade resta su `LastStableCell`;
6. nessuna sovrapposizione, in nessuno dei tre esiti.

⚠️ **Il Facing è un tie-break locale, non una fonte di ordine.** L'anello canonico esiste già —
`E → NE → NW → W → SW → SE` di `ERTHexDirection` — e non va ri-derivato.

Non-scope: collisione ricorsiva · spinta a catena · pathfinding di atterraggio · evitamento hazard · RNG.

### A3 — traccia

**Owner**: CR-REPLAY ([#1881](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881)).

L'esito di atterraggio deve distinguere almeno **primario**, **alternativa adiacente** e **fallback su
`LastStableCell`**.

🔴 **Costo da dichiarare prima di scrivere**: `ERTDisplacementBlockReason` è serializzato nel TurnLog
**formato v7**, e `ERTMoveOutcome` pure. Aggiungere valori in coda **non** è una migrazione; cambiare esiti
già prodotti sì, e porta con sé la rigenerazione del corpus golden — la conseguenza già pagata da `D-245`.
Il ramo `NoDestination` che oggi ferma la spinta al bordo porta già il commento che dice dove il valore nuovo
va scritto.

### A4 — validazione statica

**Owner**: Map Editor 0.1 ([#1861](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1861)).

L'authoring rifiuta un atterraggio staticamente isolato, con **reason code** e senza correzione automatica.
Un'alternativa valida esiste, è occupabile, non è `Void`, è topologicamente raggiungibile, e **non dipende
dall'occupazione runtime** — ragione per cui questa validazione non rimuove il fallback di A2: muri e unità
creati in partita possono chiudere un'area nata valida.

### A5 — preview

**Owner**: Player Event Log ([#1937](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1937)).

La preview consuma **solo** stato pubblico, Team Knowledge e intenti della propria squadra. Nessun intento
avversario, nessun trigger futuro. Nessun prompt nuovo durante la risoluzione.

### A6 — falsificazione

Prima di qualunque accettazione in Editor, ognuna di queste mutazioni deve far **cadere** un test:

| # | Mutazione | Cosa prova |
|---|---|---|
| 1 | conservare i passi di spinta residui dopo la caduta | A2.1 |
| 2 | sopprimere gli effetti di caduta nel fallback saturo | A2.5 |
| 3 | cambiare l'ordine adiacente canonico | A2.4 |
| 4 | permettere la sovrapposizione | A2.6 |
| 5 | non annullare il percorso volontario | A2.2 |

⚠️ **La mutazione più vera è il file prima del fix**, non una riscritta a mano: una mutazione scritta a
mano spesso non compila, e una che compila può **appendere** invece di far cadere. Si guarda il **verso**:
se la quantità misurata sale invece di scendere, si sta misurando il complemento.

### Matrice dei test richiesti

I nomi si adattano alle convenzioni del repository; il mandato lo consente esplicitamente. Ogni voce ha un
caso negativo.

`ForcedMovement.OpenLedgeStartsFall` · `...FallConsumesRemainingDisplacement` ·
`...CancelsRemainingVoluntaryPath` · `Fall.PrimaryLandingFree` · `Fall.OccupiedLandingAppliesImpact` ·
`Fall.AdjacentLandingUsesFacingCanonicalOrder` · `Fall.SaturatedLandingAppliesFallEffects` ·
`...AppliesImpactEffects` · `...ReturnsToLastStableCell` · `...NeverOverlaps` ·
`Fall.DynamicBlockerCanSaturateLanding` · `Fall.StaticValidatorRejectsIsolatedLanding` ·
`...IgnoresRuntimeOccupancy` · `Fall.TracePreservesCauseChain` · `Fall.ReplayMatchesResolvedOutcome` ·
`Fall.SeekMatchesPlaybackAtBoundary`

---

## 6. ROADMAP B — Editor / MCP / Utente

Lane `EDITOR`. È una roadmap di **accettazione**, non lo specchio di quella di codice: non esiste una voce
Editor per ogni voce di codice.

| # | Seduta | Valida | Azioni MCP | Giudizio umano | Prerequisito | Evidenza | Differibile |
|---|---|---|---|:--:|---|---|:--:|
| **B0** | smoke di authoring | A1 · A4 | creare/salvare il dato di bordo, rileggerlo | no | A1 in corso | verdetto | ⛔ **SKIP** |
| **B1** | allestimento | — | mappa/fixture, bordi, blockers, salvataggio | no | A1 chiusa | asset committato | — |
| **B2** | mega-seduta di accettazione | A2 · A3 · A5 | posa, Facing, scenari, log | **sì** | A1–A6 · suite VALIDA | voci `PIE-*` | ✅ |
| **B3** | rilettura da processo pulito | B1 | riapertura, load, validazione | no | B1 ha scritto asset | verdetto | fondibile in B2 |
| **B4** | referto di accettazione | B2 | — | **sì** | B2 | referto + artefatti | — |

### B0 è `SKIP`, e la ragione è misurata

Il mandato ammette lo smoke anticipato solo se una di cinque condizioni è vera. Nessuna lo è oggi:

1. *asset binario che sblocca il runtime* — no: le fixture di scenario producono già celle e bordi;
2. *fattibilità MCP incerta* — no per A1/A4, che sono C++ e dati;
3. *schema Save/Reload da provare* — no: il formato asset esiste e ha già le sue migrazioni;
4. *rappresentazione d'authoring non provata* — no: coperture e porte usano già la stessa sede;
5. *nessuna fixture non-Editor per lo stato richiesto* — no.

∴ **zero aperture anticipate**. Se A1 dovesse smentire il punto 5, B0 si apre con la ragione scritta.

### Gli scenari di B2

Batch in **una** apertura: cambiare mappa, fixture, Facing o riavviare PIE **non** sono precondizioni
diverse, e `AGENTS.md` §*Authoring e acceptance* lo dice esplicitamente.

| ID | Scenario | Cosa giudica una persona |
|---|---|---|
| S1 | caduta libera | la caduta **si legge** come caduta, non come teletrasporto |
| S2 | atterraggio occupato + alternativa | la collocazione finale non appare arbitraria |
| S3 | atterraggio saturo | si capisce che la caduta **è avvenuta** benché la posizione finale sia in alto |
| S4 | blockers dinamici | il cambiamento è leggibile |
| S5 | adiacenza a hazard | la preview non nasconde la conseguenza tattica |
| S6 | interazione con obiettivo | nessuna conseguenza di punteggio invisibile |
| S7 | interruzione del percorso volontario | si capisce **perché** il movimento è stato annullato |
| S8 | leggibilità multilivello | si vede chi è caduto, da dove, a dove; i layer non collassano |
| S9 | preview | l'incertezza non è presentata come certezza |
| S10 | log degli eventi | la domanda bersaglio: **«capisco perché è successo?»** |

⛔ **Non si chiede a una persona** ciò che l'Automation dimostra: coordinate finali, numeri di danno, esito
serializzato, equivalenza di replay, una unità per cella, ordine canonico. Il mandato §0 RISK 3 lo vieta e
la ripartizione esecutore/oracolo è già in
[`scenario-map.md`](../../technical/tooling/scenario-map.md) §2.

### Dove finisce il verdetto

`editor-sessions.yaml` possiede i **dati** delle sedute e cita gli **ID** delle voci PIE, mai il loro esito
atteso. L'esito vive in [`test-manuali-pie.md`](../../technical/test-manuali-pie.md).

⛔ **Nessuna seduta e nessuna voce `PIE-*` è stata aggiunta in questo passaggio**, e non è una dimenticanza:
B2 ha come prerequisito `A1–A6 complete`, e A1 è dietro `REL-3`. Registrare oggi dieci voci PIE
significherebbe scrivere in un catalogo — che *«non è un backlog»* — la specifica di un comportamento che non
esiste, cioè il difetto che il mandato stesso chiama RISK 5.

---

## 7. Piano di apertura dell'Editor

**Atteso: 1 apertura**, oppure 2 se l'authoring scrive asset binari.

| Apertura | Quando | Ragione tecnica |
|---|---|---|
| — | ora | **nessuna**: nessuna delle cinque condizioni di B0 è vera |
| E-A *(condizionale)* | dopo A1 | scrivere e salvare il dato di bordo su asset |
| E-B | dopo A6 | accettazione da **processo pulito**: il giudizio non vale nel processo che ha scritto gli asset |

Se E-A non serve — cioè se le fixture bastano — **B2 e B3 collassano in una sola apertura**.

⛔ **Non giustificano un'apertura**: un numero di issue nuovo · una fixture diversa · una mappa diversa · un
Facing diverso · il riavvio di PIE · *«è più comodo così»*.

---

## 8. v0.2 → v1.0 — nessuna issue, solo collocazione

Il mandato §8 vieta le cascate post-v0.1, e il repository pure: *«Non si aprono epic per ciò che non ha una
release»*.

| Release | Tema canonico | Cosa la capability ci porterebbe | Azione |
|:--:|---|---|---|
| v0.2 | Struttura e finestre | bordi dinamici, invalidazione dell'atterraggio | link a **E51** ([#1848](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1848)) |
| v0.3 | Informazione | conoscenza parziale della minaccia di caduta, certezza della preview | link a **E27** |
| v0.4 | Operations | mappe multilivello grandi, ispezione consapevole dei layer | già nel tema |
| v0.5 | Online Foundation | autorità sull'atterraggio, determinismo in rete | link a **E40** ([#773](https://github.com/DegrassiAaron/refactor-tactics-main/issues/773)) |
| v0.6 | Ability Runtime | effetti Push/Pull/Fall componibili senza fondere caduta e trasferimento | link a **E41** ([#774](https://github.com/DegrassiAaron/refactor-tactics-main/issues/774)) |
| v0.7 | Competitive Alpha | aggiudicazione server, replay spettatore | nel tema |
| v0.8 | Beta e bilanciamento | frequenza di caduta, valore della spinta, dipendenza dalla mappa | nel tema |
| v0.9 | Release Candidate | regressione, migrazione, golden replay | nel tema |
| v1.0 | Launch | deterministico, spiegabile, authoring validato | nel tema |

⛔ **Nessuna release cambia tema** per far posto a questa capability, e nessun `E<n>` nuovo viene allocato.

---

## 9. Non-scope della v0.1, se `REL-3` la portasse lì

Collisione ricorsiva · spinta a catena · Momentum generico · scivolamento casuale · nuovo modello di ghiaccio
· resistenza o distruttibilità del parapetto · `Hanging` · ring-out universale · `Stagger`/`Prone`/`Stun`
automatici da **ogni** caduta · input del giocatore durante la risoluzione · nuova policy `MOV-4` ·
auto-reroute · pathfinding tattico di atterraggio · evitamento hazard · fisica continua come autorità ·
seconda rappresentazione topologica · secondo resolver.

---

## 10. Riconciliazione

| Voce | Esito |
|---|---|
| Epic di capability | **creata**: [#2388](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2388), senza `E<n>`, senza milestone |
| `capability-roadmaps.md` | **aggiornato**: nuova capability `CR-VERT` |
| `OPEN_DECISIONS.md` | **aggiornato**: `REL-3` aperta |
| `roadmap-post-v0.1.md` | **aggiornato**: la riga della verticalità non dice più «nessuno» |
| issue v0.1 | **non create** — vedi §4 |
| sedute editor / voci PIE | **non create** — vedi §6 |
| `E52` | ⛔ **non allocato** |
| collisione ricorsiva | ⛔ **non introdotta** |
| esplosione post-v0.1 | ⛔ **non prodotta** |
| conflitti con `main` | nessuno: il write-set è documentale e disgiunto |

**Decisioni non prese**, come il mandato §11.H richiede: `MOV-4` (già chiusa), `MOV-3`, `STA-4`, `PER-5` e
ogni altro gate esistente restano intatti. La decisione di design del mandato §2.9 — fallback su
`LastStableCell` — è **accettata come input** e non riaperta; ciò che resta aperto è **quando** si implementa,
non **come**.
