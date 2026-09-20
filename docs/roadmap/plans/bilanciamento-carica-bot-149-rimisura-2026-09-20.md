# La carica del bot, rimisurata — cosa di #149 è scaduto, cosa altri hanno chiuso, cosa resta vivo

> `SNAPSHOT` · **Stato**: terza rimisura di #149 · **Data**: 2026-09-20
> **Base di misura**: la ricognizione su #149 è stata fatta su `origin/main` @ `a5b119d5`; i gate di §6
> sono girati su `a04c644e`, il commit di questa fetta, con albero pulito prima e dopo.
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
| La partita 2v2 si decide entro il limite di 12 turni (oggi: 10) | ⛔ **insoddisfacibile come scritta** | il «10» **è** una misura, ma del 2026-08-06 e ormai scaduta — veniva da `HexMatch.PlaysToCompletion`, che allestisce l'arena nel test, ed è anteriore alla correzione del deadlock di `#1088`; coincide con l'`ExpectedRounds` del formato spedito. Il dato di oggi è **21**, e [`D-184`] decide di **non** ritarare su di esso e di accettare il pareggio allo scadere come esito legittimo |
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

⌫ **Una prima stesura di questo paragrafo diceva che «il roster ha due sole mobilità … ed entrambe portano
il danno con sé, non esiste uno scatto puro da comporre con l'attacco base». È falso**, e la code review
l'ha preso. Le mobilità rapide sono **tre**, e la terza è uno scatto puro:

| azione | fase | effetti | slot |
|---|---|---|---|
| `Hero.Branth.Ram` (da `Action.Charge`) | `FastMovement` | `Damage 20` + `Push 1` | `Movement` |
| `Hero.Ivrin.PassingBlade` | `FastMovement` | porta danno | `Movement` |
| **`Hero.Muiren.FluidTrail`** (da `Action.Dodge`) | `FastMovement` | **`{}` — nessuno** | `Movement` |

∴ «scatto puro + attacco base» **esiste**, ed è la composizione di Muiren: `FluidTrail` sullo slot
`Movement` più `PressureJet` sullo slot `Main`, che è esattamente il piano della famiglia 4 del planner.

🔑 **Ma il Fatto 1 resta superato lo stesso, per un'altra ragione**: la carica che quel Fatto dichiarava
«dominata» è di **Branth**, e l'unica mobilità rapida di Branth *è* la carica. Non esiste nessun eroe che
abbia insieme una carica e uno scatto puro con cui dominarla. Muiren ha lo scatto puro e non ha nessuna
carica da dominare.

⚠️ La differenza fra le due formulazioni conta per chi rimisura: la prima chiude anche la domanda «il
bot compone scatto puro e attacco base?», che è **aperta** e ha un soggetto — `bot-competence.yaml` dà
`Dash` `PASS` a Muiren, e la composizione è ciò che `HexBotPlay.DashPlanIsExecutableOnCostlyTerrain`
esercita.

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
- **Otto token `Phase` su sei righe**, nomi d'eroe di due generazioni fa, invisibili al ratchet per costruzione (conta il token
  **prefissato** `Hero.Phase`; queste erano la parola nuda). Cambiate una per una leggendo la frase.
- **Due gate che il codice dichiarava sorvegliati e che non esistono** — `Bot.PlannerOutputCoversPlanFields`
  e `Bot.PlannerAppliesAttackThroughDeclare`. I nomi sono stati **tolti e non sostituiti**: mettere lì un
  gate che copre altro nasconderebbe un'assenza invece di dichiararla.
- **Il commento della famiglia 4** misurava un roster che non esiste e rinviava a `BAL-1`, che è la domanda
  Guard-contro-Brace.

### 🔴 Un reperto trovato in code review: in partita il ramo del kiting non entra mai

`RTHexBotLibrary.h` dichiarava *«sul roster v0.1 l'unica kiter è Muiren (`PressureJet`, portata 5 →
standoff 3)»*. Sono le portate **nude del catalogo**, e non sono quelle che il bot legge. Catena
verificata:

- `ARTTurnManager::PlanBots` prende `F.AttackRange = U->AttackRange`;
- `ARTUnit::EquipLoadout` risincronizza quel campo **dopo** aver applicato la variante d'arma — il suo
  commento lo dichiara: *«senza questa riga un'unità con la variante applicata continuerebbe a colpire
  alla portata VECCHIA in partita»*;
- `FRTMatchBootstrapper` chiama `EquipLoadout(DefaultLoadoutFor(HeroId))` sullo spawn di partita, **per
  entrambe le squadre** (nessun ramo per il bot, e il commento dice perché);
- `Weapon.Impact` porta `RangeDeltaCells = -1` ed è il default di Muiren **e** di Branth;
  `DefaultLoadoutFor` risponde vuoto per Aevik e Ivrin, i cui gadget la v0.1 non costruisce.

∴ le portate che il bot vede in partita sono **Muiren 4, Branth 2, Aevik 4, Ivrin 4**, e `KiterMinRange`
è 5: `DeriveKiteStandoff` restituisce **0 per tutto il roster spedito**.

⚠️ **E il ramo è verde nei test**, che è la ragione per cui nessuno se n'era accorto:
`HexBotPlay.KiterFleesWhenThreatened` costruisce Muiren dal catalogo **senza** il loadout di produzione,
quindi la vede a portata 5 e il kiting lo esercita davvero — un gate verde su una configurazione che la
partita non produce.

⛔ **Non corretto qui**: alzare `KiterMinRange`, togliere il `-1` a `Weapon.Impact` o dare uno standoff a
portata 4 sono tre decisioni di bilanciamento diverse, e `D-102` chiede il banco prima del numero. Il
fatto è dichiarato in `RTHexBotLibrary.h` perché chi legge «l'unica kiter è Muiren» non concluda che il
ramo gira.

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
