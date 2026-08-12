# Referto — Reconciliation di roadmap e tracking v0.1 (2026-08-12)

> **Partial reconciliation**, non full audit. `meta.last_full_audit` del Feature Registry **resta al
> 2026-08-08 / `2094b86`**: questo lavoro ha corretto contraddizioni misurate, non ha riconfrontato
> `Source/` · `Tests/` · `Scenarios/` · `docs/` · Wiki riga per riga.

**Sorgente**: [`../../archive/src/handoff/2026-08-12-roadmap-reconciliation.md`](../../archive/src/handoff/2026-08-12-roadmap-reconciliation.md)
**Audit rifatto su**: `ee0da4b3` (`origin/main`) — il sorgente dichiarava `dda87f1a`, due merge indietro
**Esito**: applicato in parte · una tesi del sorgente **respinta** · una **corretta** · tre difetti trovati che il sorgente non vedeva

---

## 1. Il sorgente chiedeva di non fidarsi di sé stesso, e aveva ragione

La sua §0.1 dice: *«Non fidarti delle checklist storiche se il codice o le issue live le smentiscono.
Misura su `origin/main`.»* Rimisurando, **quattro** delle sue affermazioni sono cambiate di segno.

| Affermazione del sorgente | Verifica su `ee0da4b3` | Esito |
|---|---|---|
| `#22` e `#175` risultano aperte in `#14` ma sono chiuse | ✅ vero — `gh issue view` conferma `CLOSED` per entrambe | **applicato** |
| `#25` afferma che nessun `FAutoConsoleCommand` esiste in `Source/` | ✅ vero: ne esistono **5** (`RTPacingConsole`, `RTTestConsole`, `RTHexOverlayConsole`, `RTArenaCriteriaConsole`, `RTReactionConditionConsole`) | **applicato** |
| `#152` elenca CP 14.1–14.6, mancano 14.7 e 14.8 | ✅ vero **della issue** — ⚠️ ma **non** della roadmap: `roadmap-v0.1.md` §5 documenta 14.7 e 14.8 per esteso dal 2026-08-09 | **corretto e applicato** |
| `roadmap-v0.1.md` descrive il bot partial-knowledge come mancante | 🟡 **parzialmente**: la §2.1 e la §5 dicono già ✅ dal 2026-08-11; a dirlo era la sola riga §2 | **corretto e applicato** |
| «17 categorie icone fuori enum» ridotte a **10** | ✅ verificato **contro l'enum**, non contro il sorgente: `Information`, `MapInteraction`, `Environment`, `Identity`, `Phase` esistono in `ERTIconCategory` e i loro commenti nominano proprio i concetti da mappare | **applicato in `#637`** |
| CP 11.8 va creato come contratto del puntatore | ✅ la lacuna è reale — ⚠️ ma **non quella descritta**: vedi §3 | **applicato, riformulato** |

## 2. La tesi respinta: «`#159 → #165` è una catena»

Il sorgente §1 la classifica come «troppo rigida» e propone lane parallele. **Confermato, e con la prova.**

`roadmap-v0.1.md` §3 dichiarava *«E14 non parte prima di E13»*. È **falso** sul repository di oggi, e la
smentita non è un'opinione:

- la dipendenza vera è sul **livello di rilevamento**, che esiste da **CP 13.2**;
- **tre** checkpoint di E14 (`#161`, `#162`, `#164`) sono stati chiusi **mentre** E13 era aperta, e `#163`
  (CP 14.3) è chiusa dal 2026-08-12;
- `#160` lo dichiara in proprio: *«Restano di E14 i checkpoint 14.5–14.8. Nessuno di essi aspetta questa
  issue.»*;
- la forma vincolante veniva da `v0.1-issue-plan.md`, che è **`HISTORICAL`** e non normativo.

Le quattro lane sono ora in `roadmap-v0.1.md` §3, con il vincolo interno di ciascuna — perché «parallele»
non significa «senza ordine»: `#314`/`#319` restano **dopo** `#166`, che è dove la durata della resolution
si misura per la prima volta.

## 3. CP 11.8 non è la lacuna che il sorgente descriveva

Il sorgente §7 propone il contratto come se l'interazione col puntatore fosse da progettare. **Non lo è.**
Misurato su `ee0da4b3`:

| Il sorgente implica | Il codice dice |
|---|---|
| `RMB` da assegnare a cancel/context | `RMB` è **già** `UndoAction`, insieme a `BackSpace` (`RTPlayerController.cpp:246-247`) — cioè già un `Cancel` |
| «Hover non committa mai» da imporre | l'hover è **già** sola presentazione (`:298-321`), e la cella viene dalla **quota** del punto colpito |
| contratto da costruire | nove regole su dieci **descrivono ciò che il codice fa già** |

I **tre delta reali** sono assenze, e nessuna è nel testo del sorgente:

1. **Nessuno stato esplicito** — la modalità di targeting vive come `SelectedAbilityIndex` **sull'unità**
   (`:504`, `:606`), non nel controller;
2. **L'input non conosce la fase** — `:767` è l'**unico** punto che la legge, e serve al fine partita;
3. **Nessuna precedenza HUD → mondo** — il Canvas HUD non registra hitbox (`AddHitBox` non compare in
   `Source/`), quindi oggi **ogni** click passa al mondo. Con i widget UMG di CP 11.7 il problema si
   **inverte**, ed è la ragione dell'ordine fra i due checkpoint.

Le sole regole **nuove** sono di privacy e ownership: nemico non rilevato fuori dall'hover, ghost alleato
in sola lettura. Owner: [`../../technical/spec-pointer-interaction.md`](../../technical/spec-pointer-interaction.md).

Il contratto **eredita** anche una domanda rimasta orfana: [`../../technical/spec-hover-cella.md`](../../technical/spec-hover-cella.md)
è `HISTORICAL` dal pivot esagonale e nessun owner aveva raccolto *cosa succede sotto il cursore*.

## 4. Tre difetti che il sorgente non vedeva

Trovati rimisurando, non erano nell'handoff.

### 4.1 La colonna `CP` della tabella §3 era ferma su **tre** epic

Il totale «95» era la somma **corretta** di una colonna **sbagliata**:

| Epic | §3 dichiarava | §5 conteneva | Da quando |
|---|---:|---:|---|
| **E7** | 4 | **5** (`7.1`–`7.5`) | CP 7.5, moduli reazione (`#505`) |
| **E11** | 6 | **7** (`11.1`–`11.7`) | CP 11.7, 2026-08-12 |
| **E14** | 6 | **8** (`14.1`–`14.8`) | CP 14.7 e 14.8, 2026-08-09 |

Il valore vero **prima** di questo lavoro era **99**; con CP 11.8 è **100**. Il difetto è di **direzione**:
chi aggiunge un checkpoint scrive la riga di dettaglio in §5 — dove serve al lavoro — e la tabella
riassuntiva resta indietro **in silenzio**, perché nessun gate la confronta con le sezioni che riassume.
Lo script di rimisura è ora **dentro** §3, accanto al totale, ed è stato eseguito prima di scriverlo.

> 🔴 **E il totale era copiato in cinque posti, non uno.** Corretto §3 e fermatosi lì, questo lavoro avrebbe
> lasciato quattro copie a dire «95» — cioè avrebbe **riprodotto** il difetto che stava correggendo. La
> ricerca (`grep -rn "21 epic" docs/`) le ha trovate tutte:
>
> | Copia | Esito |
> |---|---|
> | `roadmap-v0.1.md` §3 | **fonte** — corretta |
> | `docs/README.md` | copia viva — corretta |
> | `roadmap-checkpoint.md` ×2 | copie vive — corrette, e una ora dichiara di **essere** una copia |
> | `roadmap.shortlist.md` | prosa **fuori** dal blocco `RT_SHORTLIST_EPICS` generato — corretta a mano |
> | `v0.1-issue-plan.md` | **`HISTORICAL`**, snapshot dichiarato non riscritto — **lasciata** |
> | `plans/consolidamento-roadmap-2026-08-10.md` | referto **datato**: registra una misura del 2026-08-10 — **lasciato** |
>
> La lezione non è «cercare meglio»: è che un totale senza un gate che lo verifichi si moltiplica. Le copie
> vive sopravvivono perché nessuno le confronta con la fonte, ed è **esattamente** il motivo per cui il
> registro dei test e il registro PIE hanno smesso di tenere numeri scritti a mano.

### 4.2 `PIE-V01-GHOSTS` era citata da tre documenti e **non esisteva**

Il DoD di **CP 11.6** la nomina dal 2026-08-07 in `roadmap-v0.1.md`, `brief-planning-visuale.md` e
`v0.1-issue-plan.md`, ma il registro di `test-manuali-pie.md` **non aveva la riga**. Il link checker non
poteva vederlo: `PIE-V01-GHOSTS` è un **identificatore**, non un link. È lo stesso difetto dei due nomi di
test inesistenti che il gate `G3` citava prima di CP 14.3. Registrata ⏳.

### 4.3 `roadmap-v0.1.md` si contraddiceva sul bot

La §2 diceva *«bot/HUD che non la consumano»*; la §2.1 e la §5 dicevano ✅ **dal 2026-08-11**. Vince il
codice: `PlanBots` filtra `Ctx.Enemies` sulla conoscenza di squadra — *«ignoto alla squadra: per il bot
quella cella e' vuota»* (`RTTurnManager.cpp:374`) — con `HexBotPlay.HiddenEnemyFairness` a dimostrarlo.

## 5. Cosa è stato deliberatamente **non** fatto

| Non fatto | Perché |
|---|---|
| ribilanciare `Brace` / `Hold Ground` | `BAL-1` (`#403`) decide per playtest e `#404` applica: un reconciliation documentale non muove numeri di combat |
| decidere la soluzione di `#687` (`FormatVersion` non serializzato) | il sorgente stesso lo vieta finché il meccanismo non è verificato su asset serializzato con binario vecchio/nuovo — ed è ciò che fa la **PR #688**, aperta |
| toccare file delle PR aperte **#688** e **#694** | #694 è *stacked* su #688: nessun file di quelle aree è stato modificato |
| aggiornare `meta.last_full_audit` | non è stato fatto un full audit — vedi il banner in testa |
| dichiarare i test `RefactorTactics.PlayerInput.*` nel registry | `validate` tratta un pattern senza corrispondenza come **errore** (`feature_registry.py:372`), ed è la protezione giusta: i nomi entrano coi test |
| aggiungere `PIE-V01-POINTER` al subset `RELEASE-V01` | il criterio del §8 di `scenario-map.md` nomina tre cose e la tabella «non ne aggiunge una quarta»: allargherebbe `G9` di due voci aperte senza che la DoD lo chieda |
| aprire le epic proposte in §10 del sorgente (Super Actions, Modular Effects, Seeded Map, Level Designer, Networking) | il sorgente le marca **PROPOSTE**, non roadmap canonica, e `docs/OPEN_DECISIONS.md` è il posto di una proposta senza decisione |

## 6. Numeri rimisurati, non incrementati

| Misura | Prima | Dopo | Comando |
|---|---:|---:|---|
| epic v0.1 | 21 | 21 | script in `roadmap-v0.1.md` §3 |
| checkpoint v0.1 | 95 *(dichiarato)* · 99 *(reale)* | **100** | idem |
| voci registro PIE | 132 | **134** | `awk` in `scenario-map.md` §7 |
| di cui ⏳ aperte | 79 | **81** | idem |
| subset `RELEASE-V01` | 17 | **17** | `grep -c` in `scenario-map.md` §7 |
| `Visual.*` ↔ `PIE-VIS-*` | 21 = 21 | **21 = 21** | idem |
| scenari `planned` nel registry | 50 | **56** | `feature-registry.yaml` |
| feature nel registry | 103 | **104** | `grep -c "^  - feature_id:"` |
| copie vive del totale epic/CP | 5 *(una sola aggiornata)* | **5 allineate** | `grep -rn "21 epic" docs/` |
| `validate` | 0 errori · 32 warning | **0 errori · 33 warning** | `feature_registry.py validate` |

Il warning in più è **atteso e voluto**: i sei scenari `planned` di `RT-FEAT-UI-POINTER-INTERACTION`.
Il meccanismo è quello dichiarato in `scenario-map.md` §6.2 — *un piano che non diventa un file resta
visibile invece di sparire*.

## 7. Next action

Le lane di `roadmap-v0.1.md` §3 non si aspettano fra loro:

```text
Lane A — Reactions:   #165 → #166        (poi, e solo poi, #314 → #319)
Lane B — Perception:  #690 + #686 → #159 → #160
Lane C — UI:          #219/#637 → #220 → #77/#613 → CP 11.8 → #291
Lane D — Consistency: #625 + #687 + #649 → #512 → #170  (prima del golden)
```
