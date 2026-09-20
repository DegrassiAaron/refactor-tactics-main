# La carica del bot, rimisurata — cosa di #149 è scaduto, cosa altri hanno chiuso, cosa resta vivo

> `SNAPSHOT` · **Stato**: terza rimisura di #149 · **Data**: 2026-09-20
> **Base di misura**: `origin/main` @ `a5b119d5`, albero pulito.
> **Owner**: [#149](https://github.com/DegrassiAaron/refactor-tactics-main/issues/149) — milestone `v0.1 — Offline Vertical Slice`
> **Cosa non è**: non decide nessun peso, non apre nessuna voce in `OPEN_DECISIONS`, non tocca
> `CR-BALANCE`. Non sostituisce il corpo della issue: lo misura.

---

## In una riga

**Tre delle cinque voci di DoD di #149 non hanno più un soggetto o sono già soddisfatte da lavoro di
altri; la quarta è insoddisfacibile come scritta; la quinta — la sola viva — non è lavorabile come
«ritarare i pesi», perché l'evidenza che servirebbe a giustificare un numero nuovo è quella che `D-102`
dichiara inammissibile.**

Il corpo della issue è già stato riallineato due volte — il 2026-08-12 per togliere `Guardian`, il
2026-08-22 per `D-130` — e **ha ripreso a invecchiare**: i nomi del 2026-08-22 sono a loro volta scaduti
(`Phase` → `Muiren`, `D-334`/`D-341`). Questo referto esiste perché la quarta rimisura non ricominci da zero.

---

## 1. Voce per voce, col comando

| voce di DoD | stato | misura |
|---|---|---|
| Deciso se lo slot Principale va fatto valere in partita | ✅ **già deciso e implementato da altri** | `ARTTurnManager::ValidatePlansAtLockIn` giudica ogni piano al lock-in con `URTPlanValidationLibrary::ValidatePlan`, che legge lo `Slot` dal catalogo e produce `SlotOccupied`. **Registra e non blocca.** I bot ci passano: `Bot.LockInValidatesBotPlansToo`, `Bot.LockInStaysSilentOnALegalBotPlan` |
| Se sì: pesi ritarati **con misura** | 🔴 **viva, e non lavorabile così** | vedi §3 |
| La partita 2v2 si decide entro il limite di 12 turni (oggi: 10) | ⛔ **insoddisfacibile come scritta** | il «10» non è una misura ma la costante di design; il dato reale è **21**, e [`D-184`] decide di **non** ritarare su di esso e di accettare il pareggio allo scadere come esito legittimo |
| Deciso se `Guardian.Charge` a 20 danni ha senso accanto a una Spazzata da 30 cd 0 | ⛔ **senza soggetto** | `Guardian.*` è fuori dal gioco dal CP 6.6 e non esiste nessuna «Spazzata» con cui confrontarla |
| Un test che dimostri il bot scegliere la carica quando è la mossa migliore — «oggi non è scrivibile onestamente» | ✅ **è scritto** | `RefactorTactics.HexBotPlay.ChargeItPlannedActuallyLands`: il bot sceglie **da solo** una `LinearCharge`, punta la cella del bersaglio, e l'impatto atterra col danno e la spinta dichiarati |

⚠️ **`ValidateActionSlots` continua a non essere chiamata in partita** — `git grep -n ValidateActionSlots -- Source/`
dà solo la definizione, la dichiarazione, i test e la HUD. Ma la domanda della DoD è risolta **altrove**:
è `ValidatePlan` a leggere lo `Slot`, non `ValidateActionSlots`. Chi rileggesse solo il nome citato dalla
issue concluderebbe che la voce è aperta.

## 2. La carica reale, coi numeri di oggi

`Hero.Branth.Ram` eredita `Action.Charge`: **20 danni + `Push 1`, portata 3, cooldown 2, slot `Movement`,
`LinearCharge`**. Il corpo della issue scrive «cooldown 2 (non 3), slot `Main`»: il cooldown è corretto,
lo **slot no** — [`D-191`] ha messo ogni mobilità rapida su `Movement`.

E il «piano che la dominava» resta non componibile: il roster ha due sole mobilità — `Hero.Branth.Ram` e
`Hero.Ivrin.PassingBlade` — ed entrambe portano il danno con sé. Non esiste uno scatto puro da comporre
con l'attacco base.

## 3. Ciò che resta vivo, e perché non è «ritarare»

Il **Fatto 3** della issue è l'unico ancora in piedi: la scala di `WThreat` (100) contro `WDamage` (10) ×
danno rende impossibile tarare un premio di posizionamento con una costante — sotto 200 non batte due
nemici, sopra 200 batte un attacco vero.

🔴 **Ma non è lavorabile come la DoD lo scrive.** «Pesi ritarati con misura — es. N partite headless»
chiede esattamente l'evidenza che [`D-102`] dichiara inammissibile: un risultato bot-contro-bot non è
evidenza di bilanciamento finché il bot non è certificato sulle capability che quel risultato produce. E
lo stato di competenza, dal 2026-09-20, si legge:
[`../bot-competence.yaml`](../bot-competence.yaml) — `BasicAttack` `PASS` per il solo Ivrin, `MapControl`
`PASS` per nessuno, `AdvancedCounter` `FAIL` per tutti e quattro.

∴ ritarare oggi produrrebbe una decisione motivata da dati veri e conclusioni false, che è precisamente
il difetto che `D-102` esiste per impedire. Il banco di prova è `CR-BALANCE`
([`../roadmap-balance.md`](../roadmap-balance.md)), oggi `out_of_release_scope`.

⛔ **E toccare un peso ha un costo dichiarato**: `docs/OPEN_DECISIONS.md` registra `#149` come l'evento
che «ritarando i pesi del bot consuma il margine» di `BOT-STALL-1`. Ogni valore di `FRTHexBotContext` è
sorvegliato da un oracolo d'esito — `ElevationNeverOutweighsClosingOneCell`,
`EngageBonusFadesWithIdleTurns`, `ObjectiveNeverOutweighsAKill`, `ObjectivePullBeatsClosingOneCell`.

## 4. Ciò che questa fetta ha fatto invece

Non un peso, ma la **difesa** dei pesi che ci sono, più la pulizia dei riferimenti scaduti dentro `Bot/`.

- **La seconda invariante dei pesi ha un presidio.** `WObjectiveFalloff > WApproach` era dichiarata «l'invariante
  che PUÒ fallire» e la pinnava **solo** un test che legge il CDO — lo stesso buco che `#1276` ha chiuso per
  `WElevation`, perché un'istanza piazzata nel livello serializza i propri `UPROPERTY` nel `.umap`. Ora è a
  runtime accanto al suo gemello, e `Bot.ObjectiveWeightInvariantIsCheckedOnTheLiveInstance` verifica che
  urli quando deve e taccia quando non deve.
- **Sei nomi d'eroe di due generazioni fa**, invisibili al ratchet per costruzione (conta il token
  **prefissato** `Hero.Phase`; queste erano la parola nuda). Cambiate una per una leggendo la frase.
- **Due gate che il codice dichiarava sorvegliati e che non esistono** — `Bot.PlannerOutputCoversPlanFields`
  e `Bot.PlannerAppliesAttackThroughDeclare`. I nomi sono stati **tolti e non sostituiti**: mettere lì un
  gate che copre altro nasconderebbe un'assenza invece di dichiararla.
- **Il commento della famiglia 4** misurava un roster che non esiste e rinviava a `BAL-1`, che è la domanda
  Guard-contro-Brace.

## 5. Cosa questo giro NON ha fatto, e perché

| non fatto | perché |
|---|---|
| Ritarare un peso | `D-102` + `D-184`; il banco è `CR-BALANCE`, fuori release |
| Chiamare `ValidateActionSlots` in partita | la proprietà è già presidiata da `ValidatePlan` al lock-in; una seconda sede sarebbe una seconda verità |
| Scrivere i due gate mancanti | è lavoro di chi possiede `#3013`, che ha introdotto le due righe. Qui sono **dichiarati assenti**, che è lo stato leggibile |
| Misurare di nuovo i 21 round | `NOT RUN` — richiede il motore per una partita `-game -RTAutobattle`, e il numero non è in discussione: `D-184` ha già deciso di non ritarare su di esso |
| Il termine `WInRange` / `PositioningRange` del WIP `a8a42c8` | il branch non esiste più su `origin` (verificato il 2026-08-12), e il commit è raggiungibile solo dal clone locale su cui fu scritto |

## 6. Verifica

| Gate | Esito |
|---|---|
| Compile (Editor) | ✅ **PASS** |
| Tests | ✅ **PASS** — `Bot+HexBot+Match`, 175 test, 175 `Result={Success}` |
| Verifica di mutazione | ✅ spegnendo il presidio nuovo cade **esattamente un test** e è il suo (1 rosso, 36 verdi); ripristinato e **ricostruito**, 37 verdi |
| Determinism · Privacy | `N/A` — le due guardie sono diagnostica: nessun peso toccato, nessun esito di `ChooseBestPlan` spostato |
| PIE · Packaged | `NOT RUN` — mandato di sessione |
