# Matrice dei conflitti documentali

> `CURRENT` · **Stato**: vivo · **Ultimo aggiornamento**: 2026-08-08 · **Owner**: questo file
> **Scopo**: registrare dove due documenti dicono cose diverse, e cosa vale oggi.
> **Regola**: un conflitto non si risolve in silenzio. O si registra `SUPERSEDED` con la fonte che prevale,
> o diventa una voce di [`OPEN_DECISIONS.md`](OPEN_DECISIONS.md). Mai una scelta implicita.

## Stati

| Stato | Significato |
|---|---|
| `CONFIRMED` | La specifica corrente è l'unica; nessuna azione |
| `SUPERSEDED` | Una specifica precedente è stata sostituita, e la sostituzione è registrata |
| `CONFLICT` | Due fonti si contraddicono e **nessuna prevale**: serve una decisione |
| `OPEN` | Tema deciso in un sorgente, **mai recepito** in un documento normativo |
| `DUPLICATE` | La stessa regola definita in più posti → consolidare su un solo owner |

---

## Matrice

| # | Area | Specifica precedente | Specifica corrente | Fonte che prevale | Stato | Azione |
|---|---|---|---|---|---|---|
| 1 | Griglia | quadrata 10×10, 4-way, Manhattan | esagonale assiale `FRTCellId{q,r,Layer}`, 6 vicini + archi fra layer | [ADR-0002](decisions/adr-0002-griglia-esagonale.md) | `SUPERSEDED` | ✅ documenti superati marcati il 2026-08-07 |
| 2 | Ordine fasi | `Movement + Action`; `Preparation→Movement→Actions` | `Planning → Prep → Dash → Blast → Move → Cleanup` | [ADR-0003](decisions/adr-0003-modello-azioni-v01.md) | `CONFIRMED` | — |
| 3 | Move ultima fase volontaria | implicito | esplicito: il Move resta **dopo** il Blast | [ADR-0003](decisions/adr-0003-modello-azioni-v01.md) §3 | `CONFIRMED` | — |
| 4 | Finestra di reazione | interrupt 5 s · 7–8 s · `Reaction Charge` | **3,0 s**, `Timeout → HOLD` | [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) §8 · D-010 | `SUPERSEDED` | ✅ |
| 5 | Nome del parametro | `FastDecisionDuration` | `FastReactionDuration` | [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) | `DUPLICATE` | ✅ un solo nome nel codice |
| 6 | Modello delle reazioni | deterministiche, senza finestre | unico `opportunity → commit`; l'attuale è `AllowedResponses ≤ 1` | [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) (D17) | `CONFIRMED` | — |
| 7 | Overwatch | skill del singolo eroe | caso concreto del modello generale di reazione | [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) · E14 | `CONFIRMED` | — |
| 8 | **Overwatch universale** | — | azione di Planning **per tutti**, effetto dal profilo di eroe/equipaggiamento; **compete** con l'azione offensiva | [D-012](decisions/RT_PDR_00_Decision_Log.md) | `CONFIRMED` | ✅ decisione chiusa 2026-08-07 · owner [`gameplay/brief-azioni-generiche-overwatch.md`](gameplay/brief-azioni-generiche-overwatch.md) · ⏳ **propagazione incompleta**, distinta dalla decisione: al 2026-08-08 `Overwatch` compariva **zero volte** in `balance/` — riga aggiunta al catalogo azioni, profilo per eroe ancora assente |
| 9 | Action Ghosts | — | Ghost Timeline per fase, presentation-only | [`technical/brief-planning-visuale.md`](technical/brief-planning-visuale.md) | `CONFIRMED` | CP 11.5/11.6 |
| 10 | Delayed Actions | — | dichiarate in Planning, risolvono a un boundary nominato | [`gameplay/brief-delayed-actions.md`](gameplay/brief-delayed-actions.md) | `CONFIRMED` | nessuna epic, deliberato |
| 11 | **Trigger su transizione** | «gli archi portano trigger?» — domanda mal posta: gli adiacenti **non sono dati** | la **trap possiede** la coppia `(From→To)`; `FRTHexEdge` resta per i soli salti di layer | [D-013](decisions/RT_PDR_00_Decision_Log.md) | `CONFIRMED` | ✅ chiuso 2026-08-07 · mappa invariata, nessun vincolo su E9 |
| 12 | Roster | Aegis/Nyx/Drift/Vex · Mara/Ivo/Nyx/Sol · Steel/Aurora/Murdock/Kwang | **Flux · Riva · Bastion · Vektor** | [`balance/RT_HeroCatalog_v0.1.md`](balance/RT_HeroCatalog_v0.1.md) + codice | `SUPERSEDED` | ✅ i nomi vecchi restano solo in righe che li dichiarano storici |
| 13 | Fog of War | north-star P1 | **non** è FoW: conoscenza parziale a 3 livelli, mappa statica nota | [`gameplay/brief-conoscenza-parziale.md`](gameplay/brief-conoscenza-parziale.md) D1 | `CONFIRMED` | E13 |
| 14 | Team Knowledge | — | unione per squadra + `UltimoContatto` (1 turno) | [`gameplay/brief-conoscenza-parziale.md`](gameplay/brief-conoscenza-parziale.md) D3/D5 | `CONFIRMED` | CP 13.1/13.2 |
| 15 | Rumore | debuff | secondo canale percettivo, flood fill intero sul grafo | [`gameplay/brief-conoscenza-parziale.md`](gameplay/brief-conoscenza-parziale.md) §12 | `CONFIRMED` | CP 13.3/13.4 |
| 16 | **Unità ausiliarie** | — | concetto unico `AuxiliaryUnit`; in v0.1 entrano **solo i vincoli architetturali**, nessun gameplay | [`gameplay/brief-unita-ausiliarie.md`](gameplay/brief-unita-ausiliarie.md) | `CONFIRMED` | ✅ chiuso 2026-08-07 |
| 17 | Durata match | 20–30 min come invariante | 3v3 Standard **25–30 min**, tetto ~45; parametro di formato | D-010 | `SUPERSEDED` | ✅ D-002 barrata, non cancellata |
| 18 | Turn cap | «12 turni» costante | `RoundLimit` da `URTMatchFormatData`: 10–14 in 2v2, 16–20 in 3v3 | D-010 · CP 10.3 | `SUPERSEDED` | ✅ implementato e testato |
| 19 | Planning duration | 30 s universale | 30 s nel 2v2 corrente, **40–45 s** baseline 3v3 | D-010 §7 | `SUPERSEDED` | ✅ |
| 20 | **Formato principale** | D-001: **3v3** principale, *Consolidata* | **non deciso**: D-001 declassata ad *Assunzione da bloccare*; 3v3 resta baseline, 4v4 solo stress (E17) | [D-011](decisions/RT_PDR_00_Decision_Log.md) | `CONFIRMED` | ✅ chiuso 2026-08-07 — si consolida con la **prima misura** su una partita ≥3v3 |
| 21 | GAS | previsto in F2 dal PDR | **no-GAS**: `URTActionData : UPrimaryDataAsset` | canone | `CONFIRMED` | divergenza dichiarata |
| 22 | Multilivello | 2D piatto | `Layer` in `FRTCellId`, A\* multilivello | [ADR-0002](decisions/adr-0002-griglia-esagonale.md) · PF.4 | `CONFIRMED` | — |
| 23 | Testing automatico | test unitari + Automation | **RT Scenario Test Harness**: scenari JSON → percorso di gioco reale → `result.json` | [`technical/test-automatico-unreal.md`](technical/test-automatico-unreal.md) | `CONFIRMED` | ✅ harness consegnato: 5 scenari `Movement.*`, console `rt.Test.*`, auto-run via CVar + GameMode, 13 test `Scenario.*` |
| 23-bis | Forma dell'harness | il prompt proponeva un `ARTTestDirector` come Actor obbligatorio | CVar + GameMode + **stesso runner**: nessun Actor di test nel codice | codice (`ScenarioHarness/RTTestConsole.cpp`) | `SUPERSEDED` | ✅ il documento owner era rimasto il **prompt di implementazione**, non la spec di ciò che fu costruito — riscritto il 2026-08-08 |
| 24 | Numerazione roadmap | F0–F6 del PDR | M6–M11 (esecuzione) + E1–E17 (release), **mappate** sulle F | [`roadmap/roadmap-checkpoint.md`](roadmap/roadmap-checkpoint.md) | `CONFIRMED` | non rinumerare |
| 25 | Determinismo | — | snapshot + RulesVersion + seed ⇒ stesso `StateHash`/`LogHash`; le Fast Decision entrano nel TurnLog **come dato** | invariante #4 · [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) | `CONFIRMED` | — |
| 26 | Privacy dell'intento | — | `FilterForTeam → FRTIntentView`; canary a M10 | invariante #6 | `CONFIRMED` | — |
| 27 | **Azioni generiche** | catalogo E4: `Wait · Move · BasicAttack · Guard · Activate · Interact` | **sette** voci: `Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch`; `Activate`→`Interact`; `Guard` **universale** | [D-025](decisions/RT_PDR_00_Decision_Log.md) *(emenda [D-014](decisions/RT_PDR_00_Decision_Log.md))* | `SUPERSEDED` | ✅ semantica decisa · ⏳ **migrazione Stable ID** aperta (9 file usano `Action.Sprint`) · *questa riga elencava ancora le **sei** voci di D-014: corretta il 2026-08-08, vedi riga 42* |
| 28 | **`Sprint`** | azione a budget in fase Dash | **profilo della famiglia `Move`**; `Sprint` ≠ Dash | [D-015](decisions/RT_PDR_00_Decision_Log.md) | `SUPERSEDED` | ✅ nessun documento insegna più «Sprint = Dash» · ⏳ migrazione |
| 29 | **Predictive Action in v0.1** | fuori scope, nessuna epic | **thin slice**: una sola, `Vektor.InterceptShot` | [D-016](decisions/RT_PDR_00_Decision_Log.md) | `CONFIRMED` | ⏳ da inserire in roadmap; **sgancia** l'azione da E14 |
| 30 | **`Intercept` e copertura** | il colpo conserva la copertura del bersaglio **originale** | geometria **rivalidata sul bersaglio effettivo**, senza nuova opportunity | [D-017](decisions/RT_PDR_00_Decision_Log.md) | `SUPERSEDED` | ⏳ serve test discriminante A/B a copertura diversa |
| 31 | **`HighGround` e vista** | «bonus visuale» non quantificato · poi `Sight_Mod = +1/+2/−1` (workbook) | **nessun bonus numerico** in v0.1 | [D-018](decisions/RT_PDR_00_Decision_Log.md) | `SUPERSEDED` | ✅ chiusa nel verso opposto: il numero veniva dal workbook, non da un playtest |
| 32 | **«Fast Action»** | usato per l'azione dichiarata in Planning che risolve dopo | quello è **Delayed/Predictive**; `Fast Action` è una scelta **live** | [D-019](decisions/RT_PDR_00_Decision_Log.md) | `DUPLICATE` | ✅ glossario corretto in `spec-durata-partita-e-scala-mappe.md` |
| 33 | **Finestre live nell'MVP** | `spec-sequenza-turno.md` §4/§5: «non implementare, serve il multiplayer» | **in scope** (E14); il gate a due condizioni è caduto | [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) | `SUPERSEDED` | ✅ il divieto **rimosso**: il documento diceva sì in §2 e no in §4 |
| 34 | **APNAP a sei gruppi** | canone §5.1: ordine totale per effetti simultanei, `FR-RESOLVE-01` | **non implementato**: l'ordine reale è a 5 chiavi sulle *azioni* | codice (`RTActionQueueLibrary`) | **`OPEN`** | ⚠️ regola normativa **senza consumer** — decidere se costruirla o riscrivere §5.1 |

| 35 | **Gerarchia Canone/ADR** | «il canone prevale su tutto», ma ADR-0004/0005 *correggevano* il canone | un ADR accettato è **recepito nel canone nello stesso commit**: nessuno stato «canone + emendamenti» | [`README.md`](README.md) §gerarchia | `CONFIRMED` | ✅ chiuso 2026-08-08 — era un paradosso di governance, non un conflitto di contenuto |
| 36 | **Quando cambia il facing** | facing = presentazione; oppure «cambia solo dopo il Move» | l'unità **si orienta verso il target/direzione prima che l'azione risolva**; il `Move`, ultimo, fissa il facing finale, che persiste nel round dopo | [D-020](decisions/RT_PDR_00_Decision_Log.md) · [ADR-0005](decisions/adr-0005-orientamento.md) | `SUPERSEDED` | ⏳ E16 · servono i test di sequenza `Dash → Blast`, cambio bersaglio, cono Overwatch, `Move` finale |
| 37 | **Privacy della finestra di reazione** | privacy = «payload visibile solo alla propria squadra» | anche il **tempo** è un canale: l'avversario non deve poter dedurre che una finestra è stata aperta | [D-021](decisions/RT_PDR_00_Decision_Log.md) · [ADR-0004](decisions/adr-0004-finestre-di-reazione.md) | `CONFIRMED` | ⏳ E14/M10 · requisito di privacy, **non** rifinitura UI: niente pausa osservabile correlata alla scelta altrui |
| 38 | **Baseline del motore** | D-007: UE **5.8** come *assunzione da bloccare* | **5.8.1** consolidata: upgrade solo fra milestone e con migrazione esplicita | [D-022](decisions/RT_PDR_00_Decision_Log.md) | `SUPERSEDED` | ✅ chiuso 2026-08-08 — il repo la bloccava già di fatto; restava aperta solo sulla carta |
| 39 | **Workbook di bilanciamento** | `RefactorTactics_Balance_Matrices_v0.1.xlsx` letto come fonte di numeri | **`RESEARCH`**: i canonici sono i cataloghi `balance/RT_*Catalog_v0.1.md` | [D-023](decisions/RT_PDR_00_Decision_Log.md) | `SUPERSEDED` | ✅ il workbook non risolve più conflitti · un futuro v0.2 andrà **derivato** dai cataloghi |
| 40 | **Bonus danno da altura nel codice** | il terreno alto concede `+Damage` all'occupante | `OccupantDamageBonus` è un parametro **generico** di `EffectiveAttackPower`: ogni call site runtime passa `0` | codice (`RTCombatLibrary`, `RTTurnManager`) · [D-024](decisions/RT_PDR_00_Decision_Log.md) | `SUPERSEDED` | ⚠️ il meccanismo **resta** (serve ad altri effetti), la semantica «altura» no · il test `Combat.EffectiveAttackPowerWithTerrainBonus` insegna ancora il contrario nel nome → issue di rinomina |
| 41 | **`Sprint`: fase e slot nel codice** | — | D-015 lo vuole profilo del `Move`, **solo** slot movimento, fase `Move` | [D-015](decisions/RT_PDR_00_Decision_Log.md) | `SUPERSEDED` | ⏳ **divergenza misurata il 2026-08-08**: nel codice `Action.Sprint` è in `ERTResolutionPhase::FastMovement` e consuma movimento **e** azione principale → issue di refactor, non correzione silenziosa |

| 42 | **Composizione delle azioni generiche** | due decisioni chiuse lo **stesso giorno** si contraddicevano: D-014 (passaggio Gameplay) ne canonizzava **sei** togliendo `Guard`; l'handoff non-Gameplay ne elencava **otto** con `Guard` e `Activate` distinte | **sette**: `Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch` — `Guard` universale, `Activate` assorbita da `Interact` | [D-025](decisions/RT_PDR_00_Decision_Log.md) | `CONFLICT` → **risolto** | ✅ deciso dall'autore il 2026-08-08. Unico `CONFLICT` vero di questo passaggio: due fonti pari grado, nessuna gerarchia fra loro · ✅ **propagazione chiusa il 2026-08-08** in [`gameplay/brief-azioni-generiche-overwatch.md`](gameplay/brief-azioni-generiche-overwatch.md), `AGENTS.md`, `CLAUDE.md` e riga 27 di questa matrice — tutti elencavano ancora sei voci |

| 43 | **Slot per turno: regola senza esecutore** | il catalogo dichiara uno slot per azione, e `URTCatalogLibrary::ValidateActionSlots` la implementa | **nessuno la fa rispettare in partita**: `Action.Dash` e `Action.BasicAttack` prendono entrambi `Main`, e sia il controller sia il bot li pianificano **insieme** | codice (`RTPlayerController`, `RTTurnManager` nota `#145`) | **`OPEN`** | ⚠️ verificato il 2026-08-08: la funzione è chiamata **solo da due test** (`RTOffensiveActionTests`). Il controller imposta `PlannedDashAbility` e `PlannedAbilityIndex` senza azzerare l'altro — quindi vale anche per il **giocatore**, non solo per il bot. Decidere se **applicarla** o **cambiare gli slot del catalogo**: oggi il documento dice una cosa e la partita ne fa un'altra |

| 44 | **Ownership di abilità e sinergie** | Wiki, handoff fazioni e showcase descrivevano `Water-Electric` come **«combo Flux + Riva»**: una combinazione stava diventando una seconda fonte di abilità/numeri | tre livelli distinti: **Ability/Action Definition → singolo owner**; **interazione → sistema**; **sinergia/fazione/scenario → esempio**. Nel codice il `+8` legge già `Status.Wet` e non conosce l'eroe che l'ha applicato | [D-028](decisions/RT_PDR_00_Decision_Log.md) · [ADR-0006](decisions/adr-0006-ownership-abilita-sinergie.md) | `SUPERSEDED` | ✅ chiuso 2026-08-08 — conflitto **fra documento e documento**, non con il codice: l'audit di `Source/` non ha trovato branch `if HeroA && HeroB` né `PairBonus`/`FactionSetBonus`. Owner normativo: [`gameplay/spec-ownership-abilita-interazioni-sinergie.md`](gameplay/spec-ownership-abilita-interazioni-sinergie.md) |

**Riepilogo al 2026-08-08** (secondo passaggio, documenti non-Gameplay): **40 risolti**
(`CONFIRMED`/`SUPERSEDED`) · 2 `DUPLICATE` chiusi · **2 `OPEN`** (riga 34 APNAP, riga 43 slot per turno)
· **0 `CONFLICT`**.

**Terzo passaggio, 2026-08-08 — ownership dei contenuti**: **+1 riga risolta** (riga 44, `SUPERSEDED`) e la
propagazione ⏳ della riga 42 chiusa. I due `OPEN` e gli zero `CONFLICT` **non cambiano**: D-028 era un conflitto
fra documenti, non fra documento e codice.

> Le due righe `OPEN` hanno la **stessa forma**, ed è il difetto ricorrente di questo repository: una regola
> normativa scritta, corretta, e che **nessun consumatore runtime applica**. L'APNAP a sei gruppi vive nel
> canone e l'ordine reale è un altro; gli slot per turno vivono nel catalogo e nessuno li fa rispettare.
> In entrambi i casi la scelta è la stessa: **costruire l'esecutore o riscrivere la regola** — non lasciarla
> leggibile e falsa.

> Le righe 35–41 nascono dall'audit del 2026-08-08 sui documenti non-Gameplay. Tre di esse (40, 41 e la parte
> ⏳ della 8) non sono divergenze *fra documenti* ma **fra documento e codice**: la decisione è presa, la
> migrazione no. Restano registrate finché la issue corrispondente non è chiusa — è esattamente il caso in cui
> marcare «✅ fatto» renderebbe la matrice leggibile e falsa.

> Le quattro voci aperte dalla revisione documentale sono state chiuse dalla sessione `/sc:brainstorm` del
> 2026-08-07 (`D-011`, `D-012`, `D-013` + due brief). Due delle domande erano **mal poste**, e lo si è scoperto
> guardando il codice: vedi la nota di metodo in [`OPEN_DECISIONS.md`](OPEN_DECISIONS.md).

---

## Come si aggiorna

1. Quando una decisione ne supera un'altra, aggiungi o modifica una riga qui **e** una voce nel
   [Decision Log](decisions/RT_PDR_00_Decision_Log.md). Barra la vecchia, non cancellarla.
2. Se non è chiaro quale fonte prevalga, lo stato è `CONFLICT` e la riga rimanda a
   [`OPEN_DECISIONS.md`](OPEN_DECISIONS.md). Non scegliere per plausibilità.
3. Un tema deciso in `src/` ma mai recepito in un documento normativo è `OPEN`, non `CONFIRMED`: un
   sorgente non è una specifica finché qualcuno non ne è owner.
