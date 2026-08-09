# Scenario Map — cosa si verifica da solo, cosa richiede una persona

> `CURRENT` · **Creato**: 2026-08-09 · **Owner** di **una sola domanda**: per ogni cosa da verificare,
> *chi la verifica* — una macchina, un occhio umano, o nessuno dei due perché non esiste ancora.
>
> Non duplica nessuno dei tre documenti che già esistono, e il confine è netto:
>
> | Documento | Risponde a |
> |---|---|
> | [`scenario-index-e-tag.md`](scenario-index-e-tag.md) | **Come si identifica e si trova** uno scenario |
> | [`scenari-validazione-visiva.md`](scenari-validazione-visiva.md) | **Cosa si guarda** nel corpus `Visual.*`, e cosa oggi non è guardabile |
> | [`test-manuali-pie.md`](test-manuali-pie.md) | Il **registro** delle verifiche interattive: esito atteso e stato |
> | **questo file** | **Chi esegue cosa** — la ripartizione fra automatico e umano, e il subset che gate la release |
>
> Come si scrive ed esegue uno scenario: [`test-e-diagnosi.md`](test-e-diagnosi.md).

## 1. Perché esiste

Il gate **G9** della [Definition of Done](../roadmap/v0.1-definition-of-done.md) chiede che «il subset
`RELEASE-V01` delle verifiche manuali sia eseguito», e dichiara che la marcatura vive nel registro PIE.
**Quel marcatore non esisteva nel registro** — misurato con `git grep -c RELEASE-V01 HEAD -- docs/` il
2026-08-09: **sei righe in tre file**, quattro nei sorgenti archiviati e due nella DoD stessa. **Zero in
`test-manuali-pie.md`**, che è l'unico posto in cui il gate le cerca. Un gate che nomina un insieme vuoto non è
verificabile, ed è la seconda volta che succede a G9: la formulazione precedente citava «le 12 verifiche
`PIE-V01-*`» quando il registro ne contava 74.

La causa non è la distrazione: è che **la ripartizione fra automatico e umano non stava scritta da nessuna
parte**. Vive implicita in quattro posti diversi — il corpus `Scenarios/`, le 115 voci del registro PIE, il
gate `scenario` del Feature Registry, e le fasce di `scenari-validazione-visiva.md` — e ricomporla a memoria
produce ogni volta un numero diverso. Questo file la scrive una volta, con i comandi per rimisurarla.

## 2. Le quattro classi

```
A  automatico            la macchina esegue e giudica         nessun umano
B  automatico + occhio   la macchina esegue, l'umano giudica  l'oracolo è una persona
C  solo umano            nessuno scenario può sostituirlo     mouse, editor, giudizio, cronometro
D  dichiarato            esiste come specifica eseguibile     esce BLOCKED, o non esiste ancora
```

La distinzione fra **A** e **B** non è il tipo di file — sono tutti scenari `.json` che passano dallo stesso
runner — ma **dove sta l'oracolo**. In A l'oracolo è l'assertion: se il gioco devia, lo scenario è rosso e
nessuno deve guardare. In B l'assertion esiste comunque, e non è un residuo: garantisce che *ciò che stai
guardando sia lo stato giusto*, perché uno scenario visivo senza assertion può mostrarti una bellissima
animazione di un colpo che ha mancato.

La distinzione fra **B** e **C** è la sola che decide il carico di lavoro dell'autore: una voce B si esegue
scegliendo lo scenario e premendo Play, una voce C richiede di allestire, cliccare, o cronometrare.

**Conteggio misurato il 2026-08-09** (comandi in §7):

| Classe | Quanti | Dove |
|---|---:|---|
| **A** — automatico | **14** scenari | `Scenarios/Combat/` · `Scenarios/Movement/` · `RT_Showcase_Relay_v01` |
| **B** — automatico + occhio | **21** scenari ↔ **21** voci `PIE-VIS-*` | `Scenarios/Visual/` |
| **C** — solo umano | **94** voci PIE | tutte le sezioni di `test-manuali-pie.md` tranne l'ultima |
| **D** — dichiarato | **8** scenari `Spec.*` scritti · **10** pianificati · **8** mai scritti | `Scenarios/Spec/` · `feature-registry.yaml` · fascia D di `scenari-validazione-visiva.md` |

Totale corpus versionato: **43** scenari (`A 14 + B 21 + D-scritti 8`). Totale registro PIE: **115** voci
(`B 21 + C 94`).

---

## 3. Classe A — automatico, nessun umano

Eseguiti in blocco da `RefactorTactics.Scenario.EveryShippedScenarioRuns`, che **scopre il corpus
dall'indice**: aggiungere un file basta perché venga eseguito, non c'è un secondo passo da ricordare.
Il verdetto è l'assertion. Nessuna di queste righe compare nel registro PIE, ed è corretto che non ci compaia.

| ScenarioId | Arena | Ass. | Cosa fissa |
|---|---|---:|---|
| `Combat.BasicAttack` | r4 | 4 | il colpo diretto arriva e toglie il danno del catalogo |
| `Combat.BlockedByWall` | r4 | 3 | il muro ferma la **vista**, non il passaggio |
| `Combat.CounterStrikesBack` | r4 | 4 | lo scudo assorbe **e** restituisce danno |
| `Combat.FriendlyFire` | r3 | 5 | l'AoE colpisce anche l'alleato — e porta `previewUnit` per il banco dell'anteprima |
| `Combat.LineHitsThrough` | r4 | 4 | la linea prende chi sta sulla traiettoria prima del bersaglio |
| `Combat.NoCounterWhenUnarmed` | r4 | 3 | il contrattacco richiede un'arma: niente reazione implicita |
| `Combat.SplashHitsAlliesNotSelf` | r4 | 5 | l'area prende gli alleati ma **non** chi la lancia |
| `Movement.Basic` | r3 | 2 | il passo singolo arriva sulla cella pianificata |
| `Movement.BasicFailsOnPurpose` | r3 | 1 | **`expected-fail`**: è l'unica prova che l'harness sappia dire «rosso» |
| `Movement.Blocked` | r3 | 3 | una destinazione bloccata non produce un percorso |
| `Movement.Collision` | r3 | 3 | chi cede la cella contesa, e con quale reason |
| `Movement.LongWalk` | r5 | 3 | due unità attraversano l'arena su due turni |
| `Movement.SwapRejectedByPlanning` | r3 | 3 | lo scambio diretto A↔B è rifiutato **in pianificazione**, non dal resolver |
| `RT_Showcase_Relay_v01` | RelayBasin | 5 | gli 8 turni della showcase — oggi **BLOCKED** su 5 capability (§6) |

> ⚠️ `Movement.BasicFailsOnPurpose` è escluso da `EveryShippedScenarioRuns` e verificato **al contrario** da
> `Scenario.ExpectedFailScenariosReallyFail`: deve fallire davvero, e con `FAIL`, non `ERROR`. Se il gioco
> cambiasse in modo da farlo passare, l'unica prova che l'harness sa dire «rosso» smetterebbe di provarlo
> **senza che nulla diventi rosso**. È il caso in cui il verde è il difetto.

---

## 4. Classe B — automatico + occhio

Corpus `Visual.*`: la macchina esegue e verifica lo stato, **la persona giudica se si vede**. Una voce di
questa classe non chiede «lo scenario passa» — quella parte la dicono le assertion, headless, senza aprire
l'editor. Chiede se l'effetto è **leggibile**, **distinguibile da quelli vicini**, e se non suggerisce una
regola diversa da quella che il resolver ha applicato.

**Come si eseguono**: `BP_GameMode` → `Scenario Filter A = animation`, `Scenario To Run = <id>`, Play.
Oppure `rt.Test.Scenario <Id>`. Nessun'altra preparazione: gli scenari portano arena, unità e piani.

| ScenarioId | Fixture | Voce PIE | Cosa deve vedersi |
|---|---|---|---|
| `Visual.Combat.BraceReducesEveryHit` | r4 | `PIE-VIS-BRACE` | `Brace` toglie 10 a **ogni** colpo e non finisce mai |
| `Visual.Combat.Defeat` | r4 | `PIE-VIS-KO` | barra che scende due volte, poi la rimozione — mai prima del colpo |
| `Visual.Combat.FallbackTargetMoved` | r5 | `PIE-VIS-FALLBACK` | il piano rivalidato, non un colpo a caso |
| `Visual.Combat.GuardReducesFirstHit` | r4 | `PIE-VIS-GUARD` | `Guard` toglie 15 al **primo** colpo e finisce lì |
| `Visual.Combat.PushResistance` | r4 | `PIE-VIS-PUSH` | Bastion incassa e **non arretra**, Vektor arretra |
| `Visual.Combat.SmokeCapsTargeting` | RelayLite | `PIE-VIS-SMOKE` | il bersaglio **si vede** e non si può colpire |
| `Visual.Combat.WaterElectric` | RelayLite | `PIE-VIS-COMBO` | il bonus viene dal **terreno**, non da chi bagna |
| `Visual.Combat.WaterElectricCoordinated` | r5 | `PIE-VIS-COORD` | la **coordinazione fra due eroi** dentro lo stesso Blast |
| `Visual.Core.PhaseOrder` | r4 | `PIE-VIS-PHASES` | `Dash → Blast → Move` in tre momenti separati |
| `Visual.Environment.FireOnEnter` | RelayLite | `PIE-VIS-FIRE` | **due** momenti distinti: ingresso e Cleanup |
| `Visual.Environment.IceSlide` | RelayLite | `PIE-VIS-ICE` | il terzo passo è **subìto**, non voluto |
| `Visual.Environment.WetExtinguishesFire` | RelayLite | `PIE-VIS-WETFIRE` | un'**assenza**: i danni del Cleanup che non arrivano |
| `Visual.Map.ClosedDoor` | RelayBasin | `PIE-VIS-DOOR` | il giro deve raccontare da sé perché è lungo |
| `Visual.Map.HighCoverBlocks` | **CoverYard** | `PIE-VIS-HIGHCOVER` | la barriera alta **nega**, la bassa una riga sotto **riduce** |
| `Visual.Map.HighGroundNoBonus` | RelayBasin | `PIE-VIS-HIGH` | due colpi **identici**: nessun vantaggio numerico (D-024) |
| `Visual.Map.LowCoverEdge` | RelayBasin | `PIE-VIS-COVER` | la copertura è di un **bordo**, e si vede quale |
| `Visual.Map.MultiLevel` | TestArena | `PIE-VIS-LEVEL` | la salita attraverso l'**unica** transizione |
| `Visual.Movement.Charge` | r4 | `PIE-VIS-CHARGE` | la carica si legge diversa dal passo |
| `Visual.Movement.RoughRefusesCharge` | RelayLite | `PIE-VIS-ROUGH` | un rifiuto è **muto**: non deve accadere niente |
| `Visual.Reaction.Deflection` | r4 | `PIE-VIS-DEFLECT` | 22 diventano 2 — una difesa, non un attacco debole |
| `Visual.Reaction.Interposition` | r4 | `PIE-VIS-INTERPOSE` | il colpo **cambia destinatario** a mezz'aria |

> ✅ **La corrispondenza è 1:1 dal 2026-08-09.** Non lo era: `GuardReducesFirstHit`, `BraceReducesEveryHit` e
> `WaterElectricCoordinated` erano nel corpus **senza una voce PIE**, e sarebbero rimasti eseguiti-e-mai-guardati.
> Il difetto viene dal modo in cui il corpus è cresciuto: le tre grammatiche difensive e il secondo scenario
> della combo sono arrivati **dopo** il blocco di 18 voci del 2026-08-08, e la convenzione «ogni scenario
> visivo porta una voce PIE» (§9 di `scenari-validazione-visiva.md`) non ha un controllo che la faccia
> rispettare. Il comando di §7 lo verifica ora, e va eseguito quando si aggiunge uno scenario `Visual.*`.

---

## 5. Classe C — solo input umano

94 voci del registro PIE che **nessuno scenario può sostituire**. La ripartizione qui sotto è per sezione del
registro — che è verificabile — con la ragione per cui serve una persona. Il dettaglio di ogni voce (esito
atteso, stato, copertura headless già esistente) resta in [`test-manuali-pie.md`](test-manuali-pie.md), che
ne è l'owner: qui non se ne ricopia nessuno, perché è esattamente la duplicazione che questo repository ha già
pagato quattro volte.

| Sezione del registro | Voci | Perché serve una persona |
|---|---:|---|
| Checklist principale (materiali, editor mode, bot) | 31 | **Gesto e asset**: drag, gizmo, Undo, materiali da creare in editor. Un test può dire che `HandleClickOnCell` ha scelto la cella giusta, non che il mouse ci arrivi |
| Partita su griglia esagonale (M6) | 15 | **Ciò che solo l'occhio vede**: unità centrate sui centri-cella, fluidità del playback, nessun residuo di griglia quadrata. La logica è coperta headless per 5 voci su 15 |
| Contenuto della v0.1 | 18 | **Leggibilità e asset**: che il giocatore *capisca* dal log, che l'arancione del fuoco amico si **noti**, che l'asset mappa si editi a mano |
| Strumenti di leggibilità | 2 | **Giudizio a schermo puro**: `PIE-PREVIEW-AREA` ha già trovato due difetti che nessun test poteva vedere — un contorno disegnato sotto il cilindro, e un linguaggio che parlava di celle mentre la domanda era sulle unità |
| Scenario Test Harness | 6 | **Ciò che l'utente non deve dover fare**: «premo Play e parte da solo», e un esito che si legga **senza aprire l'Output Log** |
| Durata, ritmo e scala | 5 | **Cronometro**: producono numeri di playtest, non superano gate. Si misura il **2v2** e lo si registra come tale |
| Bot — leggibilità delle decisioni | 5 | **Il perché, non il cosa**: un test può dire che lo score era il più alto, non che la scelta sembrasse sensata a chi guarda |
| Formato e icone | 2 | **Riconoscibilità alla dimensione reale** dell'HUD, e il caso di **errore** del formato |
| Stati del personaggio (E34) | 10 | Nessuna: **verificano un sistema che non esiste**. Sono classe D travestita da C — vedi §6 |

**Una parte della classe C si riproduce con uno scenario, e vale la pena saperlo**: `PIE-PREVIEW-AREA` si
allestisce in un clic con `Scenario To Run = Combat.FriendlyFire` e `Scenario Turn Pause Seconds = 30`, grazie
al campo `previewUnit`. Non la rende automatica — l'oracolo resta l'occhio — ma toglie l'allestimento a mano,
che è la parte che fa saltare le sedute. Dove un allestimento del genere è possibile e non c'è, è lavoro
utile: **`previewUnit` è presentazione e non cambia l'esito**, verificato da
`Scenario.PreviewUnitDoesNotChangeTheOutcome`.

---

## 6. Classe D — dichiarato, non eseguibile

### 6.1 Scritti e `BLOCKED` — le specifiche eseguibili

`Scenarios/Spec/` contiene scenari che descrivono una feature **che non esiste**. Dichiarano la capability in
`requires`, escono `BLOCKED` nominandola, e si accendono da soli quando atterra. `BLOCKED` è trattato
**verde** da `EveryShippedScenarioRuns`: trattarlo come rosso renderebbe irrazionale scriverne in anticipo.

| ScenarioId | `requires` | Epic che lo accende |
|---|---|---|
| `Spec.Cover.TemporaryCoverExpires` | `CreateCover` | E9 · CP 9.5 (`#73`) |
| `Spec.Environment.ElectricPropagation` | `EnvironmentalActionOwner` | — · `#282` (le abilità ambientali d'eroe hanno `Effects` vuoti) |
| `Spec.Environment.WaterQuenchesFire` | `EnvironmentalActionOwner` | idem `#282` |
| `Spec.Map.BridgeBreaksThePath` | `EnvironmentalActionOwner` | idem `#282` |
| `Spec.Objective.PointSurvivesKO` | `Objective` | E10 · CP 10.2 (`#75`) |
| `Spec.Overwatch.HoldThenFire` | `DecisionBoundary` `Facing` | E14 (`#152`) + E16 (`#175`) |
| `Spec.Perception.HeardNotSeen` | `Perception` | E13 (`#151`) |
| `Spec.Predictive.WhiffOnEmptyCell` | `PredictiveAction` | E18 (`#225`) |

> **`EnvironmentalActionOwner` è diversa dalle altre**, e la differenza è il punto: il sistema **esiste ed è
> chiuso** (CP 8.3, 8.5, 9.4 sono verdi). Quello che manca è **chi possiede** le azioni. Tre scenari-spec su
> otto sono bloccati da una issue di cablaggio, non da un'epic da costruire.

### 6.2 Pianificati nel Feature Registry — non ancora scritti

Dichiarati in `feature-registry.yaml` sotto `scenarios: {planned: [...]}`, il che li fa comparire come
**warning** in `feature_registry.py validate`. Il warning è il meccanismo: un piano che non diventa un file
resta visibile invece di sparire.

| ScenarioId pianificato | Feature | Release |
|---|---|---|
| `Stress.4v4.CoreRoster` | `RT-FEAT-STRESS-4V4` | v0.1 · E17 (`#221`) |
| `Team.Conflux.FluxRiva.ConductiveFlood` | `RT-FEAT-FACTION-SCENARIOS` | v0.2 |
| `Team.Constrine.BastionVektor.OnlyExit` | idem | v0.2 |
| `Team.Sentinel.SteelMurdock.HoldTheLine` | idem | v0.2 · richiede E35 |
| `Team.Resonance.AuroraKwang.FrozenAnchor` | idem | v0.2 · richiede E35 |
| `State.Riva.Flow` · `State.Flux.Charged` · `State.Bastion.Bulwark` · `State.Howitzer.Siege` · `State.MultiState.Stress` | `RT-FEAT-CHARACTER-STATE` | v0.4 · E34 (`#244`) |

Le **10 voci `PIE-STATE-*`** del registro sono la controparte umana di questi cinque: nascono ⏳ e restano ⏳
finché E34 non esiste. Stanno nel registro perché il ciclo *docs → epic → scenario → PIE* resti chiuso, non
perché siano eseguibili.

### 6.3 Dichiarati e mai scritti — la fascia D che non è atterrata

La **fascia D** di [`scenari-validazione-visiva.md`](scenari-validazione-visiva.md) elenca 8 scenari `Visual.*`
descritti come «scritti adesso con `requires`». **Nessuno degli otto file esiste**, verificato il 2026-08-09:
nessun `Visual.*` del corpus dichiara un `requires` diverso da `Reaction`, che è disponibile.

Quattro dei temi sono stati poi scritti come **`Spec.*`** (Overwatch, Predictive, Objective, Perception), che
è la forma migliore — una specifica eseguibile invece di una vetrina cieca. I restanti quattro
(`Visual.Facing.Cone`, `Visual.Intercept.Revalidation`, `Visual.CoverWindow.OpenFireSeal`,
`Visual.Interaction.DoorGraph`) non esistono in nessuna forma.

Non è un difetto da correggere scrivendo otto file: è una **fascia da riscrivere** come intenzione, perché
oggi promette file che non ci sono. Registrato in §8.

---

## 7. Come si rimisura

Nessun numero di questo documento va aggiornato a memoria. Tutti si ricalcolano:

```bash
# Classe A + B + D-scritti — il corpus versionato
find Scenarios -name '*.json' ! -name '_*' | wc -l                        # 43
find Scenarios/Visual -name '*.json' | wc -l                              # 21  (B)
find Scenarios/Spec   -name '*.json' | wc -l                              # 8   (D)

# Registro PIE — verdi / parziali / aperte, e il totale
awk -F'|' '/^\| \*\*PIE-/ {s=$(NF-1);
  if (match(s, /✅|🟡|⏳/)) c[substr(s, RSTART, RLENGTH)]++; else c["nessuno"]++ }
  END {printf "verde=%d parziale=%d aperta=%d senza-marcatore=%d\n",
       c["✅"], c["🟡"], c["⏳"], c["nessuno"]}' docs/technical/test-manuali-pie.md

# Classe B — la corrispondenza scenario Visual ↔ voce PIE deve restare 1:1
echo "scenari: $(find Scenarios/Visual -name '*.json' | wc -l)  \
voci: $(grep -c '^| \*\*PIE-VIS-' docs/technical/test-manuali-pie.md)"

# Subset di release: deve valere quanto la tabella §8
grep -c '^| \*\*PIE-[A-Za-z0-9.-]*\*\* `RELEASE-V01`' docs/technical/test-manuali-pie.md    # 16

# ...e il suo stato, che è ciò che G9 deve poter leggere senza contare a mano
awk -F'|' '/RELEASE-V01/ && /^\| \*\*PIE-/ {s=$(NF-1);
  if (match(s, /✅|🟡|⏳/)) c[substr(s, RSTART, RLENGTH)]++ }
  END {printf "verde=%d parziale=%d aperta=%d\n", c["✅"], c["🟡"], c["⏳"]}' \
  docs/technical/test-manuali-pie.md
```

> ⚠️ Il conteggio del subset **non** si fa con `grep -c 'RELEASE-V01'` nudo: il marcatore compare anche nella
> prosa che lo spiega, e quella forma dà **23**. È lo stesso difetto del comando di conteggio del registro
> PIE, che per settimane ha cercato `✅` *ovunque* nella cella invece che come primo carattere — un comando
> approssimativo mente con l'autorevolezza di una misura.

Il terzo comando è quello che mancava: è il controllo che avrebbe trovato subito le tre voci `PIE-VIS-*`
assenti. Va eseguito **quando si aggiunge uno scenario `Visual.*`**, non quando si sospetta un buco.

---

## 8. Il subset `RELEASE-V01` — gate G9

**Il criterio, dichiarato**: una voce entra nel subset se **senza di essa la v0.1 non è consegnabile**. La DoD
nomina tre cose — *la partita completa su hex multilivello, la fine partita a tre vie, la leggibilità minima* —
e questa tabella non ne aggiunge una quarta.

**Cosa resta fuori, e perché**: gli strumenti dell'editor (non entrano nella build di gioco), le voci che
producono **numeri di playtest** (G11 chiede di *avere* i numeri, non di centrarli), il corpus `Visual.*`
(leggibilità, non consegnabilità — e la regola è già coperta dalle assertion), le voci di E34 e della v0.2.

| Voce | Cosa gate | Oggi |
|---|---|---|
| `PIE-HEXPLAY-1` | la partita si allestisce su esagoni, unità sui centri-cella | 🟡 |
| `PIE-HEXPLAY-2` | selezione e cella sotto il cursore, **layer giusto** su multilivello | 🟡 |
| `PIE-HEXPLAY-3` | pianificazione entro budget, con anteprima visibile | 🟡 |
| `PIE-HEXPLAY-4` | risoluzione e playback senza deriva | ⏳ |
| `PIE-HEXPLAY-5` | collisione simultanea, nessuna sovrapposizione | ⏳ |
| `PIE-HEXPLAY-6` | LOS esagonale, e che il giocatore capisca perché il colpo non parte | ⏳ |
| `PIE-HEXPLAY-8` | **multilivello**: il movimento via arco, esplicitamente nominato da G10 | 🟡 |
| `PIE-HEXPLAY-9` | HUD e anteprima piani sui centri esagonali | ⏳ |
| `PIE-HEXPLAY-10` | **partita completa fino alla vittoria** — è G10 | ⏳ |
| `PIE-CAM-START` | la partita si apre sulla propria squadra | ✅ |
| `PIE-V01-MATCHEND` | **fine partita a tre vie**, a schermo, e `R` riavvia | ⏳ |
| `PIE-V01-HUD` | HUD di partita completo, `Turno n/RoundLimit` dal formato | ⏳ |
| `PIE-V01-LOG` | combat log con reason code leggibili | 🟡 |
| `PIE-V01-INTENT` | intenti alleati e **nessun** intento avversario visibile | 🟡 |
| `PIE-V01-ROSTER` | i quattro eroi si sentono diversi da giocare | 🟡 |
| `PIE-PREVIEW-AREA` | **leggibilità minima**: si capisce cosa si sta per colpire, prima del lock-in | ✅ |

**16 voci: 2 verdi, 7 parziali, 7 aperte** — misurate col comando di §7, non contate a mano dalla tabella.

`PIE-PREVIEW-AREA` è passata a ✅ il 2026-08-09 dopo **tre** difetti, nessuno dei quali un test poteva vedere:
un contorno disegnato sotto il cilindro, un linguaggio che parlava di celle mentre la domanda era sulle unità,
e una selezione che non selezionava. Lascia dietro `PIE-PREVIEW-PERSIST`, ⏳: l'avviso di fuoco amico spariva
al cambio di selezione, cioè **proprio mentre** si finisce il turno. **Non l'ho messa nel subset** — è il
residuo di una voce che c'è già, e il criterio conta le capacità, non i difetti aperti su di esse. Se il
playtest dice che l'avviso che svanisce rende la voce padre inaffidabile, allora entra: sarebbe una revisione
del criterio, non una svista.

Sette delle sedici stanno già in una seduta dichiarata del registro (la **D**, partita su hex, e la **G**,
eseguibile subito). **Otto non stanno in nessuna**, e non è un dettaglio organizzativo: il registro lo dice da
sé — «una voce che non sta in una seduta non viene eseguita mai». Sono `PIE-HEXPLAY-1/2/3/8`, `PIE-V01-HUD`,
`PIE-V01-LOG`, `PIE-V01-INTENT`, `PIE-V01-ROSTER`: le prime quattro perché sono 🟡 e le sedute raggruppano le
⏳, le altre quattro perché appartengono a E11, che non ha una seduta propria. **Assegnarle a una seduta è il
prossimo passo naturale di G9**, e vale più di eseguirne una a caso.

> ⚠️ Il registro descrive la **sessione D** in due modi che non coincidono — `PIE-HEXPLAY-1..9` nel titolo
> della seduta, `HEXPLAY-4/4b/5/6/6b/6c/7/9/10` nella tabella dei gruppi. Finché divergono, «quante ne copre
> la seduta D» non è una domanda con una risposta: per questo la riga qui sopra conta **sette** senza
> ripartirle, invece di scegliere una delle due letture e farla sembrare misurata.

> Il subset è una **proposta motivata, non un decreto**: spostarne una voce è legittimo, purché la si sposti
> nel registro e qui insieme, e purché il motivo sia il criterio e non la comodità. Quello che non è
> legittimo è lasciarlo vuoto: era lo stato del 2026-08-08, e rendeva G9 non verificabile.

---

## 9. Buchi dichiarati

Nessuno di questi si chiude scrivendo un file: sono limiti del **formato** o del **canale di presentazione**,
e stanno qui perché «tutte le feature della v0.1» non diventi un traguardo che il corpus non può raggiungere.

| Buco | Effetto sulla mappa | Owner |
|---|---|---|
| **Effetti muti** — `Wet`, `Burning`, reazione armata e cella conduttiva non emettono alcun evento | Due scenari di classe B non si possono scrivere (`Visual.Water.Wet`, `Visual.Conductive.Network`): si aprirebbero mostrando il terreno che c'era già | `scenari-validazione-visiva.md` §8.1 |
| **Azioni core senza possessore** — `Electrify`, `Ignite`, `CreateWater`, `ModifyArc` non stanno nel kit di nessuno | Tre scenari di classe D restano `BLOCKED` su `EnvironmentalActionOwner` benché il sistema sia chiuso | issue `#282` |
| **Fascia D mai atterrata** — 8 `Visual.*` descritti come scritti, 0 file | Il catalogo promette vetrine che non esistono; 4 temi vivono come `Spec.*` | §6.3, da riscrivere in `scenari-validazione-visiva.md` |
| **`Visual.Reaction.*` esiste, `Spec` no** — il campo `reaction` nell'intent c'è ed è validato | Nessuno: è un buco **chiuso**, registrato perché la documentazione lo dichiarava aperto per una working copy indietro di qualche commit | `scenari-validazione-visiva.md` §8.2 |
| **Nessuna assertion su punteggio e conoscenza** — mancano `TeamScoreEquals` e un modo di asserire sulla conoscenza di squadra | Due scenari-spec non potranno diventare verdi anche quando la capability atterra | `_nota_da_completare` di `Spec.Objective.*` e `Spec.Perception.*` |

## 10. Rapporto con gli altri documenti

| Documento | Rapporto |
|---|---|
| [`test-manuali-pie.md`](test-manuali-pie.md) | **Owner** delle voci: stato ed esito atteso si scrivono lì. Qui se ne classifica l'esecutore e se ne dichiara il subset di release |
| [`scenari-validazione-visiva.md`](scenari-validazione-visiva.md) | **Owner** della classe B: cosa si guarda, con quale fixture e quali numeri |
| [`scenario-index-e-tag.md`](scenario-index-e-tag.md) | **Owner** dell'identità: `ScenarioId`, tag, redirect, indice |
| [`../roadmap/v0.1-definition-of-done.md`](../roadmap/v0.1-definition-of-done.md) | **Consumer**: G9 punta al subset di §8 |
| [`../roadmap/feature-registry.yaml`](../roadmap/feature-registry.yaml) | **Consumer e sorgente**: il gate `scenario` di una feature è `done` solo se uno scenario la **dimostra**; gli `planned` di §6.2 vengono da lì |
| [`test-e-diagnosi.md`](test-e-diagnosi.md) | Come si scrive ed esegue uno scenario, e come si legge un report fallito |
