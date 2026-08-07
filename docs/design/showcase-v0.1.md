# RefactorTactics — Showcase v0.1 «Il Relè»

> **Stato**: scenario definito, non implementato · **Ultimo aggiornamento**: 2026-08-07
> **Epic**: **E15** di [`roadmap-v0.1.md`](roadmap-v0.1.md) §5 · **CP 15.1–15.5**
> **Sorgente**: [`../src/CLAUDE_Showcase_v0.1_Integration_CurrentCode.md`](../src/CLAUDE_Showcase_v0.1_Integration_CurrentCode.md)
> (handoff del 2026-08-07, consolidato qui — in caso di conflitto prevale questo file)

Questo documento tiene separate tre cose che è facile confondere: **cosa il codice fa già**, **cosa la
showcase vuole mostrare**, **cosa non esiste e chi lo costruirà**. La terza sezione è la più importante:
serve a impedire che una demo si costruisca del codice speciale per sé.

**Regola dell'epic**:

```text
la showcase espone il gap
  → il sistema generale si costruisce nella SUA epic
    → il sistema ha i SUOI test
      → lo scenario lo consuma
        → golden replay
```

Non:

```text
showcase → codice speciale → demo che funziona una volta sola
```

---

## 0. Identità dello scenario

| | |
|---|---|
| Nome di lavoro | `RT_Showcase_Relay_v01` |
| Arena | `L_Showcase_Relay` (asset d'autore) · fixture generata equivalente per i test |
| Squadre | **Team 0**: Flux + Riva — **Team 1**: Bastion + Vektor (bot) |
| Durata | 8 turni scriptati |
| Obiettivo mostrato | il controllo di un relè contestabile decide la partita, **non** l'eliminazione |

### Roster vigente — e cosa è storico

Il roster canonico della v0.1 è **Flux · Riva · Bastion · Vektor**
([`balance/RT_HeroCatalog_v0.1.md`](balance/RT_HeroCatalog_v0.1.md)).

Materiale **storico, non canone**, presente in PDF e documenti precedenti — da non reintrodurre:

| Elemento superato | Valore vigente |
|---|---|
| Aegis · Nyx · Drift · Vex | Flux · Riva · Bastion · Vektor |
| 100 HP per tutti | 90 / 95 / 120 / 100 (per eroe, dal catalogo) |
| Finestra di interrupt da 5 s | **3.0 s** ([ADR-0004](adr-0004-finestre-di-reazione.md) §8) |
| Mappa piatta, griglia quadrata | esagonale assiale multilivello, `FRTCellId{X,Y,Layer}` |
| GAS come motore delle abilità | resolver C++ + `URTActionData`; GAS resta north-star |

---

## 1. Canone corrente — cosa il codice fa **già**

Misurato sul repository il 2026-08-07 (HEAD `ea9009a`, **325 test unici**). La showcase può appoggiarsi a
tutto ciò che segue **senza costruire nulla**.

| Area | Disponibile | Dove |
|---|---|---|
| Allestimento | `ARTGameMode` cerca/crea `ARTHexMapActor`, crea `ARTTurnManager`, chiama `SetupHexMatch`; supporta `LevelAsset`, `GeneratedDemoArena`, `GeneratedTestArena`; roster dal catalogo eroi; Team 1 marcato bot | `RTGameMode.{h,cpp}`, `Turn/RTMatchSetupLibrary.*` |
| Substrato | griglia esagonale multilivello, A\* a costi interi, transizioni (`Stair`, `Ramp`, `Bridge`, `Tunnel`, `Elevator`, `Jump`) | `Map/`, `Pathfinding/` |
| Turno | `Prep → Dash → Blast → Move → Cleanup`; snapshot, collisioni simultanee, micro-step, TurnLog con hash e serializzazione versionata | `Turn/RTHexSimLibrary.*`, `Turn/RTTurnLog*` |
| Azioni | `FRTActionDef` con `ActionId`, fase, priorità, range, costo, cooldown, `Fallback`, `Slot`, `MovementStyle`, `Effects`, `bAllowsReaction`, `bCanBeInterrupted` | `Ability/`, `Turn/RTActionQueue*` |
| Reazioni | Counter, Deflect, Brace, Shield, Cleanse, **Intercept** (con pass dedicato **prima** delle altre: cambia il bersaglio del colpo) | `Turn/RTReactionLibrary.*` |
| Terreni | 8 superfici; **Rough** (costo alto, blocca Dash/Charge), **Fire** (10 danni on-enter + `Burning`), **Smoke** (cap targeting a 2), **ShallowWater**/**Conductive** (conducibilità dichiarata), **Ice** (scivolamento di 1 cella nel Move con ≥ 2 MP), **HighGround** (dato) | `Map/RTHexCellData.h`, `Turn/RTHexSimLibrary.*` |
| Zone controllate | `FRTSuppressiveZone`, `FRTSuppressionMover`, `ResolveSuppression`: celle controllate, path a micro-step, primo nemico che entra, ordine totale `StepIndex → UnitId`, una sola attivazione, stop del movimento | `Combat/RTOffensiveActionLibrary.*` |
| Privacy | `FRTPlannedIntent → FilterForTeam → FRTIntentView`: l'HUD **non riceve** il piano avversario, non lo nasconde | `Turn/RTIntentPrivacyLibrary.*` |
| Bot | utility su hex, candidate da `ReachableCells`, pesi tunabili | `Bot/RTHexBotLibrary.*` |

**Limiti noti del canone corrente**, da non scambiare per bug:

- il **Dash lineare che termina sul ghiaccio non scivola** (lo scivolamento è nel Move normale);
- `HighGround` esiste come dato ma **nessuna regola lo consuma**;
- `Wet`/`Obscured` sono dichiarati ma le **durate e la scadenza** arrivano con CP 8.2;
- le reazioni sono **pianificate e automatiche**: non chiedono una scelta live e non sospendono la simulazione;
- cinque **reazioni d'eroe non sono cablate** (CP 5.5 + CP 6.7).

---

## 2. Target showcase — gli 8 turni

Questa è la **sequenza finale**, non ciò che è giocabile oggi. Ogni turno dichiara l'epic che lo abilita.

| # | Cosa mostra | Contenuto | Abilitato da |
|---|---|---|---|
| **1** | mappa, planning simultaneo, terreno modificato | Riva `FluidTrail` verso il centro; Flux prende posizione; Bastion crea `KineticPanel`; Vektor prende una linea utile | ✅ oggi · acqua da `FluidTrail` → **CP 8.5** · pannello → **CP 9.5** |
| **2** | preparazione leggibile di una combo | Riva `PressureJet` su Vektor (16 danni + `Wet` + Push 1); Flux `ConductiveNode` | **CP 8.2** (stati) · **CP 8.5** (mutazione cella) |
| **3** | previsione e fallback | Flux dichiara `LinearDischarge` su Vektor; Vektor si sposta prima del Blast; il log mostra il fallback dichiarato | ✅ oggi (**CP 4.3**) |
| **4** | reazione interattiva, bait e commitment | Flux entra nella zona di Vektor → `FIRE`/`HOLD` → **HOLD**; poi entra Riva → **FIRE**: danno e movimento troncato nella cella raggiunta | **CP 14.5** — **opzionale** |
| **5** | informazione incompleta | Riva `MistVeil`; ciò che dipende dalla posizione nemica passa da *confermato* a *incerto* | cap targeting ✅ (**CP 8.1**) · etichette UI → **CP 11.2** · conoscenza reale → **E13** |
| **6** | protezione e identità dei ruoli | `PressureJet` su Bastion mostra la Push Resistance; Flux attacca Vektor; `Bastion.Interposition` redirige il colpo | **CP 5.5 + 6.7** |
| **7** | payoff ambientale | la rete acqua/conduttivo è pronta: danno sorgente, danno propagato, propagazione ordinata, ogni unità colpita una volta sola | **CP 8.3** |
| **8** | obiettivo > deathmatch | Flux va KO, Riva resiste sul relè, il punto è assegnato **dopo** ambiente e KO: la squadra vince con un eroe a terra | **CP 10.2** |

### Mappa target

Relay A centrale · Relay B su settore secondario · `ShallowWater` · `Conductive` · fumo producibile · un
tratto `Ice` · un tratto `Rough` · almeno un percorso alternativo · (E9) cover direzionale bassa,
`KineticPanel`, una porta o un ponte · (E10) obiettivo contestabile.

**Per il primo test headless non si dipende da un `.umap`**: prima la fixture riproducibile in codice/dati
(CP 15.2), poi l'asset d'autore equivalente per la presentazione.

---

## 3. Delta di scope — cosa **non** esiste e chi lo costruisce

Nessuna riga di questa tabella si costruisce dentro E15.

| Delta | Stato | Epic / CP proprietario |
|---|---|---|
| Stati temporanei con durata e scadenza deterministica (`Wet`, `Burning`, `Electrified`, `Obscured`, `Rooted`, `Exposed`, `Marked`, `Slow`) | ⏳ | **CP 8.2** — issue `#65` |
| Propagazione elettrica (≤ 3 celle, 20/12 danni, una volta per unità, ordine `distanza → CellId → UnitId`) | ⏳ | **CP 8.3** — issue `#66` |
| Acqua spegne il fuoco, `Wet` cancella `Burning` | ⏳ | **CP 8.4** — issue `#67` |
| Azioni che **creano** terreno (`CreateWater`, `Ignite`, `Electrify`, fumo di `MistVeil`, `ConductiveNode`, acqua di `FluidTrail`) | ⏳ | **CP 8.5** — issue `#68` |
| Cover direzionale sui 6 bordi, integrità, distruzione | ⏳ (`FRTHexCellData` non ha il campo) | **CP 9.1/9.2** |
| Strutture: porte, ponti, `GraphRevision`, invalidazione cache | ⏳ | **CP 9.3/9.4** |
| `KineticPanel` / `Reconfigure` come struttura, non come mesh spostata | ⏳ | **CP 9.5** |
| Obiettivo contestabile verificato nel Cleanup, dopo ambiente e KO | ⏳ | **CP 10.2** — issue `#75` |
| Reazioni d'eroe cablate al motore (`Interposition`, `Deflection`, `ReactiveCapacitor`) | ⏳ | **CP 5.5 + 6.7** |
| `Riva.FlowReaction` (riposizionamento **dentro** un boundary) | ⏳ rinviata | **E14** |
| Micro-step del movimento sospendibile | ⏳ | **CP 14.2** |
| Finestra `FIRE`/`HOLD` da 3 s, `Vektor.InterceptShot` interattivo | ⏳ | **CP 14.5** |
| Conoscenza parziale reale (vista, rumore, tre livelli) | ⏳ | **E13** |
| Etichette *confermato / previsto / incerto* nell'HUD | ⏳ | **CP 11.2** |

### Design **della showcase**, non del gioco

Queste regole valgono nello scenario e vivono nei **dati**, mai nel codice delle regole:

- la **relocation del Relay** (l'obiettivo si sposta durante la partita);
- il punteggio **«primo a 4 punti»**;
- la durata di **8 turni** (il canone v0.1 prevede il limite a 12).

> Vietato: `if (Turn == 4) { MoveRelay(); }` in `ARTTurnManager`. Quando la relocation arriverà, sarà uno
> schedule di scenario (`ObjectivePhase[]` con `StartTurn`, `ActiveCells`, `Duration`) sopra il sistema
> obiettivi di E10 — e si progetta **dopo** aver letto l'implementazione reale di E10.

---

## 4. Showcase **Lite** — la prima fetta, senza sistemi nuovi

Costruibile **oggi** (CP 15.2). Usa solo regole atterrate e serve da fixture d'integrazione:

1. spawn Flux/Riva vs Bastion/Vektor su arena generata deterministica;
2. percorsi e collisioni simultanee;
3. `Rough` nega un Dash;
4. `Ice` fa scivolare nel Move;
5. `Fire` applica l'effetto on-enter;
6. `Smoke` limita il targeting;
7. `Riva.PressureJet` applica danno (+ `Wet`/Push per quanto rappresentabile);
8. `Bastion.Ram` usa `LinearCharge` e si ferma all'impatto;
9. Counter / Deflect / Intercept **generici**;
10. fallback su bersaglio che si sposta prima del Blast;
11. TurnLog leggibile con reason code;
12. ripetizione a parità di input ⇒ stesso log e stesso hash.

**Gate**: build Editor + Game verdi, suite verde, scenario ripetuto N volte con log/hash identico.

---

## 5. Determinismo e golden replay

La formula di determinismo cambia quando entrano le finestre di reazione ([ADR-0004](adr-0004-finestre-di-reazione.md) §3):

```text
stessa snapshot + stessi intenti di planning + stesse decisioni di reazione
+ stesse regole/versione + stesso seed  =  stesso risultato
```

Ogni scelta durante la resolution è un **comando autorevole append-only** della stessa esecuzione.

**Nel replay canonico si registrano**: `OpportunityId`, `ReactionInstanceId`, `DecisionBoundary`, `Response`,
`SelectedTargetId`.
**Non entrano nell'hash**: millisecondi di wall-clock, durata dei VFX, frame di presentazione, slow-motion.
Il timeout è una risposta canonica: `Response = Hold, Reason = Timeout`.

Gli input della partita golden sono un **dato**, non un click:

```text
Turn 1
  Flux:    MoveIntent … / MainAction … / Reaction …
  Riva:    …
  Bastion: …
  Vektor:  …
ReactionDecisions:
  Boundary X -> HOLD
  Boundary Y -> FIRE target Riva
```

I file golden della showcase vivono con quelli del **CP 12.6**, stesso meccanismo e stessa cartella
(`Source/RefactorTactics/Tests/Golden/`). **Rigenerazione solo con flag esplicito**: ogni epic che atterra
cambia legittimamente l'esito, e una rigenerazione automatica trasformerebbe il golden in una firma vuota.
La PR che rigenera dichiara *perché* l'esito è cambiato.

---

## 6. Obiettivi di prodotto — registrati, non usati come gate

La showcase serve a sei scopi con criteri diversi: demo interna, scenario di smoke test, fixture di
integrazione, golden replay, benchmark del resolver, base per il tutorial e per i playtest di leggibilità.

Solo i criteri **verificabili in automatico** sono DoD di checkpoint (CP 15.2–15.4). Gli obiettivi di prodotto
— «si capisce senza spiegazione lunga», «la tensione del turno 4 si legge» — restano qui, e si valutano nel
playtest di CP 15.5: sono ragioni per iterare, non condizioni di chiusura.

---

## 7. Errori da evitare *(dalla §53 della sorgente, ridotti a quelli attivi)*

1. reintrodurre Aegis/Nyx/Drift/Vex o qualunque valore della tabella §0;
2. creare un secondo `ARTGameMode` «showcase» con regole duplicate;
3. cablare gli 8 turni o la relocation dentro `ARTTurnManager`;
4. scrivere `if (HeroId == …)` o `if (ActionId == "Vektor.InterceptShot")` nel resolver;
5. duplicare la geometria di `FRTSuppressiveZone` per l'Overwatch;
6. valutare l'Overwatch **dopo** che il movimento è già risolto;
7. usare `Delay`, Timeline, montage o frame rate come logica;
8. fare `timeout = FIRE`;
9. dire al giocatore che «arriveranno altri trigger»;
10. sequenziare due trigger simultanei con l'ordine di un array;
11. far leggere al bot il percorso futuro o all'HUD il piano avversario;
12. usare `TMap`/`TSet` come ordine competitivo, o un GUID casuale dentro il replay;
13. anticipare E9 in un `KineticPanel` «showcase-only» o E10 in un `if (Turn == 4)`;
14. una sola PR con E8 + E9 + E10 + Fast Reaction.
