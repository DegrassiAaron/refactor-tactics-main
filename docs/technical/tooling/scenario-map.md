# Scenario Map — cosa si verifica da solo, cosa richiede una persona

> `CURRENT` · **Creato**: 2026-08-09 · **Owner** di **una sola domanda**: per ogni cosa da verificare,
> *chi la verifica* — una macchina, un occhio umano, o nessuno dei due perché non esiste ancora.
>
> Non duplica nessuno dei tre documenti che già esistono, e il confine è netto:
>
> | Documento | Risponde a |
> |---|---|
> | [`scenario-index-e-tag.md`](scenario-index-e-tag.md) | **Come si identifica e si trova** uno scenario |
> | [`scenari-validazione-visiva.md`](../runbooks/scenari-validazione-visiva.md) | **Cosa si guarda** nel corpus `Visual.*`, e cosa oggi non è guardabile |
> | [`test-manuali-pie.md`](../test-manuali-pie.md) | Il **registro** delle verifiche interattive: esito atteso e stato |
> | **questo file** | **Chi esegue cosa** — la ripartizione fra automatico e umano, e il subset che gate la release |
>
> Come si scrive ed esegue uno scenario: [`test-e-diagnosi.md`](../runbooks/test-e-diagnosi.md).

## 1. Perché esiste

Il gate **G9** della [Definition of Done](../../roadmap/v0.1-definition-of-done.md) chiede che «il subset
`RELEASE-V01` delle verifiche manuali sia eseguito», e dichiara che la marcatura vive nel registro PIE.
**Quel marcatore non esisteva nel registro** — misurato con `git grep -c RELEASE-V01 HEAD -- docs/` il
2026-08-09: **sei righe in tre file**, quattro nei sorgenti archiviati e due nella DoD stessa. **Zero in
`test-manuali-pie.md`**, che è l'unico posto in cui il gate le cerca. Un gate che nomina un insieme vuoto non è
verificabile, ed è la seconda volta che succede a G9: la formulazione precedente citava «le 12 verifiche
`PIE-V01-*`» quando il registro ne contava 74.

La causa non è la distrazione: è che **la ripartizione fra automatico e umano non stava scritta da nessuna
parte**. Vive implicita in quattro posti diversi — il corpus `Scenarios/`, le 135 voci del registro PIE, il
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
| **A** — automatico | **46** scenari | `Scenarios/Combat/` · `Scenarios/Movement/` · `Scenarios/Spec/Facing/` · `Spec.Cover.TemporaryCoverExpires` · `Spec.Predictive.WhiffOnEmptyCell` · i tre `EnvironmentalActionOwner` · `RT_Showcase_Relay_v01` |
| **B** — automatico + occhio | **21** scenari ↔ **21** voci `PIE-VIS-*` | `Scenarios/Visual/` |
| **C** — solo umano | **112** voci PIE | tutte le sezioni di `test-manuali-pie.md` tranne l'ultima, **meno** le `PIE-MUT-*` |
| **D** — dichiarato | **11** scenari `Spec.*` ancora `BLOCKED` · **66** dichiarati `planned` nel registry e senza file *(⏱️ **rimisurati il 2026-08-17**, ed erano `12 · 56`: la riga era già stantia prima di questo giro e questo giro l'ha resa più stantia, aggiungendo tre `planned` al registry. Riletti da `scenariomap.shortlist.md`, che è **generato** — «78 scenari versionati · 67 eseguibili · 11 `BLOCKED` · 66 dichiarati `planned`» — e la scomposizione A/B/D è stata rifatta **classificando ogni file** con la regola dichiarata, non incrementata: `A 46 + B 21 + D 11 = 78`, che è il totale del generato. **Due metodi indipendenti che concordano**, come la volta precedente chiedeva. ⚠️ E la lezione è ancora quella: `feature-registry.yaml` è una **terza sorgente** di questa vista accanto a `Scenarios/` e `RTScenarioSession.cpp` — chi tocca solo il registry sposta questo numero senza toccare un solo file di scenario, ed è il motivo per cui `parallel-batch.yaml` l'ha aggiunta al `derives_from` nello stesso commit)* *(letti dal generato il 2026-08-13; questa riga diceva «**50** pianificati · **4** mai scritti», due sottoinsiemi che il generato **non distingue** — somma 54 contro 56, quindi la ripartizione era già inconciliabile con la sua fonte e si è tenuto il numero generato)* *(i pianificati rimisurati il 2026-08-12 **sull'albero mergiato**, su `scenariomap.shortlist.md`, che è generato; questa riga diceva **38** e §6.2 diceva **47** — due numeri vecchi in modi diversi nello stesso documento. ⚠️ Rimisurati **tre volte** il 2026-08-12, e ogni volta il numero era gia' cambiato sotto: **52** con `Spec.Map.ConstrainedCellCostsMore` ancora `planned`, **51** quando quel piano è diventato un file, **50** sull'albero unito perché `#659` ne ha acceso un altro nel frattempo. È il meccanismo del corpus che funziona — un `planned` che si accende esce da qui — ma dice anche che questo numero non si incrementa a mano: si rilegge da `scenariomap.shortlist.md` **dopo** il merge. ✅ **Le righe A/B e il totale sono state rimisurate il 2026-08-13, voce per voce come questa nota chiedeva.** Dicevano `A 27 + B 21 + D 12 = 60` e non si riconciliavano col generato: la scomposizione era ferma al **2026-08-09** e sbagliava di **13** scenari, tutti in classe **A**. Rifatta classificando **ogni file** con la regola dichiarata — `BLOCKED` se un `requires` chiede una capability fuori dall'allowlist di `RTScenarioSession.cpp`, altrimenti **B** se sta in `Scenarios/Visual/`, altrimenti **A** — il conto è `A 40 + B 21 + D 12 = 73`. ⚠️ **Misurato con due metodi indipendenti che concordano**: la classificazione voce per voce e il generato di `scenariomap.shortlist.md`, che conta **73** versionati e **61** non bloccati (= `A 40 + B 21`). La regola resta quella scritta qui — questo numero **non si incrementa a mano** quando nasce uno scenario: si rilegge dal generato dopo il merge)* | `Scenarios/Spec/` · `feature-registry.yaml` · fascia D di `scenari-validazione-visiva.md` |

Totale corpus versionato: **78** scenari (`A 46 + B 21 + D-bloccati 11`) — *rimisurato il 2026-08-17 con due metodi che concordano, vedi la nota della riga **D**; diceva `73` = `A 40 + B 21 + D 12`, fermo al 2026-08-13*. Totale registro PIE: **135** voci
(`B 21 + C 112 + 2 fuori classe`).

> 🔴 **Rimisurato il 2026-08-13: il totale PIE diceva 117 e ne erano 135.** Il numero fu scritto il
> 2026-08-09 (`362b717a`) e da allora **non è più stato toccato**, mentre il registro cresceva di diciotto
> voci. Lo stesso documento lo diceva in due modi diversi: la §1 parlava di **116** voci e questa riga di
> **117** — due numeri vecchi in modi diversi nello stesso file, che è la firma esatta del difetto già
> registrato per i `planned` nella riga `D`.
>
> ⚠️ **Non era un difetto di merge**: `135` era già il valore su `origin/main` (`6e109776`) prima che questa
> sessione iniziasse. Nessun gate confronta questi totali col registro, ed è il motivo per cui sono
> sopravvissuti quattro giorni a ogni `validate` verde.
>
> I tre comandi, che sono quelli della §7 e vanno rieseguiti invece che ricordati:
>
> ```bash
> grep -c '^| \*\*PIE-'     docs/technical/test-manuali-pie.md   # 135  totale
> grep -c '^| \*\*PIE-VIS-' docs/technical/test-manuali-pie.md   #  21  classe B
> grep -c '^| \*\*PIE-MUT-' docs/technical/test-manuali-pie.md   #   2  fuori classe
> ```
>
> `C` **non** è stata contata a mano: è `135 − 21 − 2 = 112`, cioè la definizione della sua riga resa
> aritmetica. La partizione ora è esaustiva e la somma si verifica da sé — prima non lo era, perché
> `21 + 95 + 1` faceva `117` e nessuna delle tre parti veniva dal registro.
>
> ⚠️ **Restano NON rimisurate le righe `A` e `D` e il totale del corpus**: hanno per soggetto gli scenari di
> `Scenarios/`, non il registro PIE, e la loro scomposizione per classe è **umana** — l'avvertenza del
> riquadro del 2026-08-10 vale ancora e non è stata aggirata da questa passata.

> ⚠️ **Rimisurato il 2026-08-10, e il conteggio era rotto in due punti diversi.** La riga `A` diceva già
> **27** mentre il totale sotto continuava a sommare `A 26`; la riga `D` diceva **9** bloccati e **21**
> pianificati quando erano **12** e **29**. Le tre misure sopra — corpus, `Visual`, `Spec` — vengono dai
> comandi della §7; il numero dei `planned` e la ripartizione eseguibili/bloccati vengono da
> `feature_registry.py shortlist`, che li calcola dalla stessa sorgente invece di ricopiarli.
> **Gli elenchi di dettaglio delle §3, §4 e §6.1 non sono stati rimisurati in questa sessione**: le tre PR
> mergiate il 2026-08-10 hanno aggiunto scenari di movimento e predittivi che vanno ancora attribuiti alla
> loro classe voce per voce. Il totale è vero, la sua scomposizione per file no.

> **Una voce non sta in nessuna delle quattro classi**, ed è meglio dirlo che forzarla dentro.
> `PIE-MUT-BASTION-SLOW` (2026-08-09) è una **verifica di mutazione**: rompere il codice di proposito e
> controllare che cada *esattamente* il test atteso. Non è C — non chiede mouse, editor né giudizio umano, e
> una macchina la eseguirebbe da sola. Non è A — perché la macchina, da sola, **non può garantirsi la
> precondizione**: che nessun altro processo UE tenga `UnrealEditor-RefactorTactics.dll`. Con la DLL bloccata
> il link fallisce (`LNK1104`) e il test gira contro il binario vecchio, cioè **passa per il motivo
> sbagliato**. Serve una persona che scelga il momento, non che guardi lo schermo.
>
> Il modello a quattro classi divide per *dove sta l'oracolo*. Questa voce dice che esiste un secondo asse —
> **chi controlla le precondizioni** — e che finora coincideva col primo. Se ne arriva una seconda, vale la
> pena farne una classe.
>
> 🔴 **La seconda è arrivata, e nessuno se n'era accorto — 2026-08-13.** `PIE-MUT-ACTIONS-ZERO` è nel
> registro accanto a `PIE-MUT-BASTION-SLOW`: le voci di mutazione sono **due**, non una, e la riga qui sopra
> aveva dichiarato in anticipo cosa fare in quel caso.
>
> È il caso più insidioso di numero stantio, perché **non è un numero**: è una **condizione d'innesco**
> scritta correttamente, verificabile con un `grep`, e scattata in silenzio. Un totale sbagliato prima o poi
> stona; una condizione già soddisfatta non stona mai — resta lì a sembrare futura. Nessun gate la controlla,
> per la stessa ragione per cui nessuno controlla i totali di questa tabella.
>
> ➡️ **Aperta, non decisa qui**: se `PIE-MUT-*` diventi una classe **E** — *oracolo automatico, precondizione
> umana* — è una scelta sul modello, e questa passata è documentale. Il conteggio nel frattempo le tiene
> **fuori da `C`**, che è ciò che la riga originale prescriveva; la classe le darebbe un nome, non un numero
> diverso.

> ⚠️ **Corretto il 2026-08-09, e non a occhio.** Questa tabella diceva `A 21` · `D-bloccati 12` ·
> `13 pianificati`. Due numeri erano sbagliati e uno era invecchiato nello stesso giorno in cui è stato
> scritto:
>
> - i **tre** scenari che chiedevano `EnvironmentalActionOwner` non sono più bloccati — la capability è
>   atterrata con `#282` (`75b8264`), poche ore dopo questa pagina. Sono passati in classe **A** da soli,
>   che è il meccanismo del corpus che funziona (§6.1);
> - i **pianificati erano ventuno**, non tredici: §6.2 ne elencava ventuno sotto un totale di tredici.
>   Nessuno li aveva contati, perché contarli a mano è esattamente ciò che nessuno fa due volte.
>
> Nel frattempo `#346` ne ha aggiunti **due** in `Scenarios/Combat/` (`RiktorImpactShotSlows` e il suo
> gemello di controllo `MoveIsFullWithoutSlow`): la classe A arriva a **26** e il corpus a **56**. Le due
> correzioni sono state calcolate su rami diversi nello stesso pomeriggio e riconciliate al merge — che è
> il motivo per cui il totale ora si misura invece di sommarlo.
>
> Da qui in avanti i tre numeri si **misurano**: `python scripts/feature_registry.py shortlist` li scrive
> in `../../roadmap/scenariomap.shortlist.md` §1 leggendo `Scenarios/`
> e le capability dichiarate in `RTScenarioSession.cpp`, e `shortlist --check` fallisce se divergono.
> **La ripartizione A/B/C resta umana** — dipende da dove sta l'oracolo, non dai file — ed è il motivo per
> cui questa pagina continua a esistere.

> ⚠️ **La cartella non è la classe.** **Dieci** scenari di `Scenarios/Spec/` sono di **classe A**: i sei
> `Spec.Facing.*`, `Spec.Cover.TemporaryCoverExpires` e i tre che chiedono `EnvironmentalActionOwner`
> (`Spec.Environment.ElectricPropagation`, `Spec.Environment.WaterQuenchesFire`,
> `Spec.Map.BridgeBreaksThePath`). I primi hanno
> `requires` vuoto perché **E16 è chiusa**; il settimo dichiara ancora `CreateCover`, ma quella capability è
> **disponibile** da E9.5, quindi il runner non lo blocca più; gli ultimi tre si sono accesi con `#282`.
> È il meccanismo del corpus che funziona come
> previsto — «si accendono da soli quando la capability atterra» — osservato **tre** volte il 2026-08-09.
> ⚠️ «Non più bloccato» **non vuol dire «verde»**: che le assertion tengano lo dice la suite, non questa
> tabella. Un file si classifica leggendo il suo `requires` e la disponibilità della capability, mai il
> percorso: le cartelle sono storage e non promettono nulla ([`scenario-index-e-tag.md`](scenario-index-e-tag.md) §2).

> 🔁 **Il corpus si rimisura quando cambia un numero di bilanciamento — 2026-08-10, `#131`.**
> Wraith è sceso da 100 a 90 HP ([D-069](../../decisions/RT_PDR_00_Decision_Log.md)) e **11 scenari** sono
> diventati rossi in blocco, tutti con lo **stesso** delta di −10 su un'unità Wraith: `Combat.LineHitsThrough`,
> `Combat.SplashHitsAlliesNotSelf`, `Spec.Environment.{ElectricPropagation, WaterQuenchesFire}`,
> `Visual.Combat.{PushResistance, WaterElectric, WaterElectricCoordinated}`,
> `Visual.Environment.{FireOnEnter, WetExtinguishesFire}`, `Visual.Reaction.{Deflection, Interposition}`.
>
> **Non è un difetto e non è una classe nuova**: è il corpus che fa il suo mestiere. Vale però registrare
> due cose per la prossima volta.
>
> 1. **La forma del rosso è la diagnosi.** Undici fallimenti con delta identico su un'unica unità dicono
>    «una statistica è cambiata», non «undici regole si sono rotte». Un delta *disomogeneo* avrebbe voluto
>    dire l'opposto, e sarebbe stato il momento di fermarsi.
> 2. **La prosa scade insieme ai numeri.** Sei di quegli undici file spiegano l'aritmetica a parole
>    (*«100 − 10 − 8 = 82»*, *«`Wraith` resta a 100 pieni»*). Correggere solo le `expect` avrebbe lasciato una
>    spiegazione che contraddice l'assertion accanto — e la spiegazione è **metà** del valore di uno scenario
>    `Visual.*`, che esiste per dire a una persona cosa deve vedere. Sono state corrette entrambe.

> ⚠️ **Non tutto ciò che sta sotto test è uno scenario, e non basta scriverlo in `Scenarios/` perché lo
> diventi.** Le quattro classi ripartiscono le **verifiche**; una *fixture* è un **ingresso**, e il suo posto
> lo decide ciò che il formato sa esprimere. `FRTScenarioCell` — l'unico modo che uno scenario ha di parlare
> della mappa — porta `Cell`, `bBlocksMovement`, `bBlocksLineOfSight` e `MoveCost`: niente segmenti, niente
> bordi disegnati, niente footprint. Uno scenario è una **partita**, con `scenarioId`, `fixture`, unità,
> intent e turni.
>
> Il caso concreto è [`#619`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/619): chiedeva
> quattro fixture di geometria («segmento solido, angolo, footprint solido, footprint void») in
> `Scenarios/Spec/Map/`. Non sono partite: sono input di una funzione pura, e vanno con i dati di test del
> modulo runtime. Ciò che merita davvero uno scenario è **uno** — la cella stretta che fa fermare
> `ReachableCells` una casella prima — ed è di **classe A**, perché l'oracolo è l'assertion. È
> `Spec.Map.ConstrainedCellCostsMore`, e sta in classe **D** finché non è un file: dichiarato `planned` sotto
> `RT-FEAT-TOOL-MAP-GEOMETRY`, così il warning di `validate` lo tiene visibile come vuole §6.2.
>
> ➕ **Dal 2026-08-13 quella regola ha un owner che la enuncia**:
> [`spec-hex-geometry-authoring.md`](../systems/spec-hex-geometry-authoring.md) §14 dice quali verifiche di quest'area
> vanno in automation e quali in seduta PIE, e ripete che le fixture geometriche vivono in
> `RTOccupancyFixtures.h`. La stessa revisione ha respinto sette scenari `Spec.Environment.*` proposti da un
> handoff: descrivono capability che non esistono, e uno scenario scritto prima della capability verifica il
> nulla — è la classe **D** usata come promessa invece che come inventario.
>
> Regola generale, che questo caso rende esplicita: *se il formato dello scenario non può esprimere
> l'ingresso, il file non appartiene a `Scenarios/`*. Metterlo lì prometterebbe copertura automatica e
> consegnerebbe file che nessun runner esegue — la stessa forma di errore dell'ultima riga di §5, dove dieci
> voci sono **classe D travestita da C**: la collocazione dichiara una promessa che il contenuto non mantiene.

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
| `Combat.RiktorImpactShotSlows` | r4 | 4 | lo `Slow` applicato nel **Blast** agisce sul **Move** dello stesso turno: il bersaglio si ferma due celle prima |
| `Combat.MoveIsFullWithoutSlow` | r4 | 3 | il gemello di controllo: senza il colpo, le stesse quattro celle si percorrono tutte |
| `Combat.SplashHitsAlliesNotSelf` | r4 | 5 | l'area prende gli alleati ma **non** chi la lancia |
| `Movement.Basic` | r3 | 2 | il passo singolo arriva sulla cella pianificata |
| `Movement.BasicFailsOnPurpose` | r3 | 1 | **`expected-fail`**: è l'unica prova che l'harness sappia dire «rosso» |
| `Movement.Blocked` | r3 | 3 | una destinazione bloccata non produce un percorso |
| `Movement.Collision` | r3 | 3 | chi cede la cella contesa, e con quale reason |
| `Movement.LongWalk` | r5 | 3 | due unità attraversano l'arena su due turni |
| `Movement.SwapRejectedByPlanning` | r3 | 3 | lo scambio diretto A↔B è rifiutato **in pianificazione**, non dal resolver |
| `Spec.Facing.DerivesFromMove` | — | — | il `Move` fissa `FacingFinalAfterMove`, che **persiste** nel round dopo |
| `Spec.Facing.TurningPathUsesLastCompletedStep` | r4 | 2 | un percorso che **svolta**: il facing è quello dell'**ultimo passo compiuto** (ADR-0008 §2), non del primo né della direzione del viaggio. ⚠️ Scritto il 2026-08-13 perché `DerivesFromMove` è **rettilineo**, e su una linea dritta le tre letture coincidono — non distingueva la regola dalla sua approssimazione |
| `Spec.Facing.DashReorients` | — | — | il Dash riscrive l'orientamento **prima** del Blast |
| `Spec.Facing.TargetingReorients` | — | — | un'azione con bersaglio orienta l'unità *prima* di risolvere (D-020) |
| `Spec.Facing.FrontAttackKeepsGuard` | — | — | `Guard` riduce dentro l'arco frontale |
| `Spec.Facing.BackAttackIgnoresGuard` | — | — | e **non** riduce da dietro: l'emisfero posteriore è scoperto (CP 16.2) |
| `Spec.Facing.BraceHoldsFromBehind` | — | — | `Brace` invece tiene da ogni lato — è ciò che lo distingue da `Guard` |
| `Spec.Cover.TemporaryCoverExpires` | r4 | 3 | una copertura temporanea **scade** — il terzo momento, quello che si dimentica. Acceso da E9.5 |
| `Spec.Environment.ElectricPropagation` | — | — | la scarica corre sul grafo dell'acqua **perché un eroe la innesca** — acceso da `#282` |
| `Spec.Environment.WaterQuenchesFire` | — | — | l'acqua spegne le fiamme, e la fonte è un'azione di un eroe — idem `#282` |
| `Spec.Map.BridgeBreaksThePath` | — | — | rompere un arco **annulla** il percorso invece di allungarlo — idem `#282` |
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
| `Visual.Combat.PushResistance` | r4 | `PIE-VIS-PUSH` | Riktor incassa e **non arretra**, Wraith arretra |
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

112 voci del registro PIE che **nessuno scenario può sostituire**. La ripartizione qui sotto è per sezione del
registro — che è verificabile — con la ragione per cui serve una persona. Il dettaglio di ogni voce (esito
atteso, stato, copertura headless già esistente) resta in [`test-manuali-pie.md`](../test-manuali-pie.md), che
ne è l'owner: qui non se ne ricopia nessuno, perché è esattamente la duplicazione che questo repository ha già
pagato quattro volte.

| Sezione del registro | Voci | Perché serve una persona |
|---|---:|---|
| Checklist principale (materiali, editor mode, bot) | 43 | **Gesto e asset**: drag, gizmo, Undo, materiali da creare in editor. Un test può dire che `HandleClickOnCell` ha scelto la cella giusta, non che il mouse ci arrivi |
| Partita su griglia esagonale (M6) | 16 | **Ciò che solo l'occhio vede**: unità centrate sui centri-cella, fluidità del playback, nessun residuo di griglia quadrata. La logica è coperta headless per 5 voci su 15 |
| Contenuto della v0.1 | 22 | **Leggibilità e asset**: che il giocatore *capisca* dal log, che l'arancione del fuoco amico si **noti**, che l'asset mappa si editi a mano |
| Strumenti di leggibilità | 2 | **Giudizio a schermo puro**: `PIE-PREVIEW-AREA` ha già trovato due difetti che nessun test poteva vedere — un contorno disegnato sotto il cilindro, e un linguaggio che parlava di celle mentre la domanda era sulle unità |
| Scenario Test Harness | 6 | **Ciò che l'utente non deve dover fare**: «premo Play e parte da solo», e un esito che si legga **senza aprire l'Output Log** |
| Verifiche di mutazione — **solo `PIE-V01-ARENA`** | 1 | La sezione ne contiene **tre**, ma le due `PIE-MUT-*` sono *fuori classe* e vanno contate a parte. `PIE-V01-ARENA` resta C a pieno titolo: chiede l'editor |
| Durata, ritmo e scala | 5 | **Cronometro**: producono numeri di playtest, non superano gate. Si misura il **2v2** e lo si registra come tale |
| Bot — leggibilità delle decisioni | 5 | **Il perché, non il cosa**: un test può dire che lo score era il più alto, non che la scelta sembrasse sensata a chi guarda |
| Formato e icone | 2 | **Riconoscibilità alla dimensione reale** dell'HUD, e il caso di **errore** del formato |
| Stati del personaggio (E34) | 10 | Nessuna: **verificano un sistema che non esiste**. Sono classe D travestita da C — vedi §6 |

**Somma: 112** — e la somma è il punto. `43 + 16 + 22 + 2 + 6 + 1 + 5 + 5 + 2 + 10` va confrontata con la
riga `C` della §2 **prima** di toccare l'una o l'altra.

> 🔴 **Rimisurata il 2026-08-13, e il difetto non era in un numero: era in una sezione mancante.** Questa
> tabella dichiarava **95** e ne elencava nove sezioni; il registro ne ha **dieci** di classe C. Mancava
> *«Verifiche di mutazione»* per intero — comprese le due `PIE-MUT-*` che la §2 conta come fuori classe, e
> `PIE-V01-ARENA` che non lo è. Due sezioni erano inoltre indietro: **Checklist** da `31` a `43`,
> **Contenuto della v0.1** da `18` a `22`.
>
> ⚠️ **La prima passata di oggi aveva corretto la §2 e si era fermata lì**, lasciando questa tabella a
> dichiarare `95`. Il documento sarebbe uscito dalla correzione **contraddicendosi ancora**, che è esattamente
> il difetto che quella correzione dichiarava di aver chiuso. Il confine «la scomposizione per classe è
> umana, non la tocco» vale per le righe `A` e `D` — che hanno per soggetto `Scenarios/` — e **non** per
> questa, che ha per soggetto il registro PIE ed è misurabile con lo stesso comando.
>
> ```bash
> awk '/^#{2,3} /{s=$0} /^\| \*\*PIE-/{c[s]++} END{for(k in c) print c[k], k}' \
>     docs/technical/test-manuali-pie.md | sort -rn
> ```

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

> ✅ **Il meccanismo ha funzionato, e si è visto tre volte il 2026-08-09.** I sei `Spec.Facing.*` scritti per
> E16 hanno `requires` **vuoto** da quando l'epic è chiusa; `Spec.Cover.TemporaryCoverExpires` si è acceso con
> E9.5; e i tre `EnvironmentalActionOwner` con `#282`. Girano, giudicano, e sono di classe **A**. Nessuno ha
> dovuto ricordarsi di «promuoverli» — è la differenza fra uno scenario-spec e una nota in un documento.

I **nove** rimasti (l'elenco misurato è in
`../../roadmap/scenariomap.shortlist.md` §1, generato):

| ScenarioId | `requires` | Epic che lo accende |
|---|---|---|
| `Spec.Objective.PointSurvivesKO` | `Objective` | E10 · CP 10.2 (`#75`) |
| `Spec.Overwatch.HoldThenFire` | `DecisionBoundary` `Facing` | E14 (`#152`) + E16 (`#175`) |
| `Spec.Perception.HeardNotSeen` | `Perception` | E13 (`#151`) |
| ~~`Spec.Predictive.WhiffOnEmptyCell`~~ | ~~`PredictiveAction`~~ | ✅ **acceso il 2026-08-10** da E18 (`#225`): `PASS`, 4/4 assertion su 2 turni |
| `Spec.Brace.ProfileChangesResponse` | `DecisionBoundary` `ReactionClash` | E14 · CP 14.7 |
| `Spec.Clash.ReadBeatsStand` | `DecisionBoundary` `ReactionClash` | E14 · CP 14.7 |
| `Spec.Clash.StandBeatsShift` | `DecisionBoundary` `ReactionClash` | E14 · CP 14.7 |
| `Spec.Clash.ShiftBeatsRead` | `DecisionBoundary` `ReactionClash` | E14 · CP 14.7 |
| `Spec.Clash.TieAppliesOnce` | `DecisionBoundary` `ReactionClash` | E14 · CP 14.7 |

> **`EnvironmentalActionOwner` era diversa dalle altre**, e la differenza si è vista nell'esito: il sistema
> **esisteva ed era chiuso** (CP 8.3, 8.5, 9.4 verdi), mancava solo **chi possiede** le azioni. Non era
> un'epic da costruire ma una issue di cablaggio — `#282` — e infatti è l'unica delle cinque capability
> mancanti che sia stata chiusa in giornata. Il confine resta dichiarato nel codice: la capability **non**
> copre `Action.Ignite` né `Action.ModifyArc`, che per [D-046](../../decisions/RT_PDR_00_Decision_Log.md)
> restano senza owner in v0.1.

### 6.2 Pianificati nel Feature Registry — non ancora scritti · **63**

Dichiarati in `feature-registry.yaml` sotto `scenarios: {planned: [...]}`, il che li fa comparire come
**warning** in `feature_registry.py validate`. Il warning è il meccanismo: un piano che non diventa un file
resta visibile invece di sparire.

> 🔴 **Questa cella diceva `56` ed era ferma di sette, misurato il 2026-08-16.** Non è una svista di
> battitura: il numero non aveva un comando in §7 — la sezione ne elenca otto, e nessuno contava i
> `planned`. La shortlist **generata** diceva già `59` nello stesso repository, quindi due viste
> divergevano e solo una si aggiornava da sola. Ora il comando c'è (§7, ultimo blocco) e il valore si
> rimisura invece di ricordarlo.

> ➕ **+4 il 2026-08-16 dal consolidamento *Mini v0.1 Autobattle*** (`D-145`, epic
> [#952](https://github.com/DegrassiAaron/refactor-tactics-main/issues/952)): i quattro `AutoBattle.*`
> di `RT-FEAT-MATCH-AUTOBATTLE`.
>
> | Scenario | Cosa dimostrerà |
> |---|---|
> | `AutoBattle.OpenField` | La partita non presidiata si chiude su terreno neutro: nessun input, un vincitore |
> | `AutoBattle.Obstacles` | Il layout cambia le decisioni — è il gate che dice che la mappa è gameplay e non sfondo |
> | `AutoBattle.Hazard` | Una superficie pericolosa modifica l'esito in modo deterministico |
> | `AutoBattle.Objective` | La partita può finire per obiettivo, non solo per eliminazione |
>
> **Restano `planned` per l'oracolo, non per il tempo**, come i sei di CP 11.8: il formato di scenario
> enumera i turni uno per uno (`FRTScenarioTurn`) e una partita autobattle **non sa in anticipo quanti
> turni durerà**. Scriverli oggi produrrebbe un `ERROR` — difetto del test — non un `BLOCKED`, che è la
> forma legittima di una specifica anticipata. Li sblocca **E47.4**
> ([#957](https://github.com/DegrassiAaron/refactor-tactics-main/issues/957)), che **segue** — senza esserne
> bloccata — il seam `DecisionProvider` di [D-101](../../decisions/RT_PDR_00_Decision_Log.md)
> ([#542](https://github.com/DegrassiAaron/refactor-tactics-main/issues/542)).
> 🔴 **La riga diceva «non si apre prima», ed erano due errori in uno** (corretti il 2026-08-16):
> il free-run non è bloccato da nulla — `PlanBotsForTest()` esiste, serve un ciclo — e il numero era in
> conflitto col grafo, che scriveva `#512`. Sono issue diverse: `#512` è il `DecisionProvider` delle
> **finestre di reazione** (CP 15.3 metà B), `#542` è il seam **generale**.
>
> ⚠️ **Quattro scenari, una sola arena.** `FRTScenarioCell` permette già a uno scenario di modificare le
> celle che gli interessano sull'arena generata — ostacoli, costo, blocco della vista — quindi non servono
> quattro mappe versionate. È la clausola che la sorgente stessa chiedeva.

> ➕ **+6 il 2026-08-12 dal reconciliation di roadmap** — tutti e sei di
> `RT-FEAT-UI-POINTER-INTERACTION` (CP 11.8): cinque `Visual.UI.*` e uno `Spec.Privacy.*`.
> Restano `planned` per la ragione dichiarata in testa a questa sezione — **manca l'oracolo**, non il tempo:
> l'esito atteso di un click è la casella della matrice di
> [`spec-pointer-interaction.md`](../systems/spec-pointer-interaction.md) §5, e finché il `PlayerController` non ha un
> **contesto esplicito** quell'esito non è leggibile da uno scenario in nessun formato. `Spec.Privacy.HiddenEnemyHoverNoLeak`
> ha in più una dipendenza di contenuto: il filtro di rilevamento sull'hover è E13.
> Referto: [`../../archive/roadmap-plans/roadmap-reconciliation-2026-08-12.md`](../../archive/roadmap-plans/roadmap-reconciliation-2026-08-12.md).

> 🔴 **E un settimo non si aggiunge, il 2026-08-13.** Un sorgente di consolidamento proponeva di estendere la
> grammatica di scenario con `hover` / `lmb` / `rmb` / `pointerMode` per coprire il contratto del puntatore.
> **Rifiutato, e per la regola di questa cartella**: il formato non ha quelle chiavi, e aggiungerle prima che
> esista un produttore runtime renderebbe l'harness il **primo** produttore della capability — cioè più
> capace del gioco. È lo stesso criterio che tiene `DeclaredRotation` fuori dalle capability disponibili.
> I quattro nomi proposti (`Spec.UI.Mouse.EnemyInspectIsReadOnly`, `…TargetCellUnderUnit`,
> `…InteractionReasonPrivacy`, `…GhostFocusDoesNotEdit`) sono già coperti, per contenuto, dai sei `planned`
> qui sopra: non servono altri nomi, serve l'oracolo.
>
> ✅ **Lo stesso criterio era violato altrove, e si è chiuso in giornata.** La capability `PredictiveAction`
> era dichiarata disponibile in `RTScenarioSession.cpp` motivandolo con *«un canale che il gioco ha già»*:
> alla misura del 2026-08-13 **mattina** quel canale non esisteva — `PlannedAttackCell` non aveva alcun
> produttore in `Player/` o `UI/`. I quattro scenari che usano `targetCell`
> (`Spec/Cover/TemporaryCoverExpires`, `Spec/Facing/FrontHitOnCoverIsSilent`,
> `Spec/Facing/RearHitOnCoverIsTraced`, `Spec/Predictive/WhiffOnEmptyCell`) erano stati lasciati verdi per
> scelta, col debito su [`#737`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/737).
> **La sera `#737` è atterrata**: `ARTPlayerController::HandleTargetCell` scrive i due campi, la motivazione
> è tornata vera e quei verdi ora dicono qualcosa di vero sul giocatore.
>
> ✅ **E `DeclaredRotation` è entrata la sera stessa** — l'ultima delle tre asimmetrie storiche di
> quell'elenco, dopo `ReactionPlanning` (#601) e `PredictiveAction`. La chiave `facing` è in
> `FRTScenarioIntent`, la capability è disponibile, e i due scenari nuovi dimostrano la rotazione dichiarata
> sul percorso reale: `Spec.Facing.IllegalDeclaredRotationIsRejected` (rifiutata, non corretta) e
> `Spec.Facing.StationaryDeclaredRotationApplies` (da fermo si ruota liberi). Il secondo esiste perché il
> primo, da solo, passerebbe anche con un resolver che rifiuta **ogni** rotazione.
>
> 🔴 **E qui è emerso un buco che non riguarda solo il facing.** Togliendo `DeclaredRotation` dalle
> capability, i due scenari passano a `BLOCKED` e **l'intera suite resta verde**: `EveryShippedScenarioRuns`
> accetta `BLOCKED` per costruzione — giustamente, è il meccanismo che permette di versionare uno scenario
> prima della sua capability — ma per uno scenario che *oggi gira davvero* quell'accettazione lascia
> scoperta proprio la regressione che conta. Il rimedio esisteva già per la reazione (#601) e ora c'è anche
> qui: `RefactorTactics.Scenario.DeclaredRotationScenariosPass` pinna l'esito a `Pass`.
>
> ⚠️ **Vale per ogni scenario che passa da `BLOCKED` a verde**: quando una capability atterra, l'ancora va
> scritta **nello stesso commit**, o la capacità appena guadagnata può sparire in silenzio.
>
> 🔎 **E i sei `planned` qui sopra ora hanno l'oracolo.** Il contesto esplicito esiste
> (`ARTPlayerController::GetPointerContext`), quindi la ragione dichiarata in testa — *manca l'oracolo, non
> il tempo* — non vale più per tutti e sei. Restano bloccati da altro:
> `Visual.UI.DoorHoverAndInteract` da #74/#613, `Visual.UI.ReactionWindowPreemptsWorldInput` da E14,
> `Spec.Privacy.HiddenEnemyHoverNoLeak` da E13, e i due `Visual.UI.SelectMoveCancel`/`TargetEnemyConfirmCancel`
> dalla grammatica di scenario per il puntatore, che resta deliberatamente non aggiunta.

> 🔴 **Perché i `planned` non si scrivono tutti in una volta — misurato il 2026-08-12.** Tentando di
> scriverli fino alla v0.2 (**42**: 23 in v0.1, 19 in v0.2), l'ostacolo non è il tempo ma l'**oracolo**: uno
> scenario si scrive solo se il numero atteso esiste già nel repository. Il conto:
>
> | Motivo per cui non si scrive | Quanti |
> |---|---:|
> | i valori sono `PROPOSED FOR PLAYTEST`, non canone (`Spec.TimeBank.*`, `Spec.Clash.*`) | 13 |
> | dipende da una **decisione aperta** (`BAS-1`…`BAS-4` per i profili, `AE-2` per le soglie) | 10 |
> | la feature è `DESIGNED` e il comportamento non esiste (`Spec.Map.*`, `Spec.Bot.*` tattici) | 11 |
> | il roster non esiste ancora (`Team.Sentinel.*`, `Team.Resonance.*` sono E35/v0.2) | 2 |
> | ✅ **scritto e verde** | **1** |
>
> ⚠️ E una scoperta che vale oltre il conteggio: **`requires` copre il runtime mancante, non un `ActionId`
> inesistente**. `Spec.ActionEconomy.OverwatchReservesMovementSlot` è stato scritto, eseguito e **rimosso**:
> il loader rifiuta il file perché `Action.Overwatch` **non è nel catalogo core** (`grep` → zero occorrenze),
> e un file che non si carica fa fallire `ShippedScenariosAreValid` — cioè rompe il gate per tutti. Un
> `BLOCKED` dichiarato con `requires` presuppone che il **dato** esista e manchi solo il **codice**.
>
> ➕ **+6 il 2026-08-12 dal consolidamento dell'action economy** — quattro `RT-FEAT-ACTION-MOVEMENT-COMPAT`,
> uno `RT-FEAT-ACTION-PLAN-VALIDATION`, uno `RT-FEAT-ACTION-COOLDOWNS`. Il kit ne proponeva **dodici**
> (`AE-S01`…`AE-S12`), in una convenzione di nomi che il repository non usa. Sei sono caduti per ragioni
> diverse, e la differenza conta: due **contraddicono `D-028`** (`Dash + attacco + Move` e
> `Brace + attacco + movimento` non sono legali come regola generale — lo scatto occupa lo slot movimento, e
> `Brace` occupa la principale); due sono **bloccati su `FAC-12`** e si scrivono quando la decisione esiste,
> non prima; due sono **assorbiti** — «densità da fermo» non ha un fatto proprio, e la privacy del piano è
> già di `RT-FEAT-NET-PRIVATE-PLANNING`. Il sesto sopravvissuto è riformulato:
> `OverwatchReservesMovementSlot` asserisce la **causa** di [D-070](../../decisions/RT_PDR_00_Decision_Log.md)
> invece del divieto di `Dash`, che è una conseguenza.
>
> ⚠️ **Uno dei sei non si scriverà con questa epic**, e resta `planned` di proposito: `Spec.ActionEconomy.PathLengthChangesEffect` dipende dai **fatti del percorso** (`AE-3`), non dal profilo di movimento, ed è dichiarato *in prestito* sotto `RT-FEAT-ACTION-MOVEMENT-COMPAT` finché `AE-3` non ha una feature propria. Stesso motivo per cui `Spec.ActionEconomy.SprintEnhancesMomentum` è scrivibile solo a metà: la parte «lo Sprint potenzia» esiste, la parte «di quanto» dipende da quante celle hai percorso. Registrato in [`../../gameplay/spec-compatibilita-azioni-movimento.md`](../../gameplay/spec-compatibilita-azioni-movimento.md) §6. Referto:
> [`../../archive/roadmap-plans/action-economy-consolidamento-2026-08-12.md`](../../archive/roadmap-plans/action-economy-consolidamento-2026-08-12.md) §6.
>
> ➕ **+13 il 2026-08-11 dal consolidamento Bot/AI** — tre `RT-FEAT-BOT-FAIRNESS`, cinque
> `RT-FEAT-BOT-TACTICAL`, tre `RT-FEAT-BOT-BELIEF`, due `RT-FEAT-BOT-PREDICTIVE`. Il sorgente ne proponeva
> **33**, in una convenzione di nomi (`AI.<Area>.<Caso>`) che il repository non usa: sarebbe stato un secondo
> spazio di nomi accanto a quello che l'indice e l'harness già risolvono. Venti dei 33 descrivevano
> comportamenti di feature senza spec né gate — un nome pianificato per un sistema che non ha ancora una
> forma è un nome che verrà rinominato. Referto:
> [`../../archive/roadmap-plans/bot-ai-consolidamento-2026-08-11.md`](../../archive/roadmap-plans/bot-ai-consolidamento-2026-08-11.md) §4.4.
>
> ⚠️ **Il titolo di questa sezione diceva «38» mentre il suo primo paragrafo diceva «trentaquattro»**, ed
> erano entrambi vecchi. Rimisurato il 2026-08-11 su `scenariomap.shortlist.md`, che è **generato**: è la
> sola cifra di questa pagina che non si scrive a mano, e va letta di lì.

> ⚠️ **Erano dichiarati «13», poi ventuno, poi ventitré, poi trentotto, poi trentaquattro**: tredici
> fra `Clash` e `TimeBank`, dieci fra `State.*`/`Team.*`/`Stress.*`, sei fra `Brace` e `Overwatch`, cinque
> `Spec.Map.*`. **È la prima volta che il numero SCENDE**, e per due motivi diversi che vale la pena non
> confondere: dei **quattro di `BAL-1`**, tre sono stati **scritti** il 2026-08-10 (`#401`, verdi al primo
> run) e il quarto — `Spec.Brace.PushBeyondGuardThreshold` — è stato **cancellato**, non rinviato: chiedeva
> una spinta di 2 che [D-074](../../decisions/RT_PDR_00_Decision_Log.md) ha deciso di non introdurre, quindi
> non ha più un soggetto. Un piano che sparisce perché la decisione l'ha reso privo di oggetto non è un
> piano evaso.
> I `TimeBank` sono passati da 8 a 10 con la riconciliazione di `#361` (sotto); i sei di Brace/Overwatch e i
> cinque di `Spec.Map.*` arrivano dai triage del 2026-08-10, da **due rami diversi** — ed è così che il
> numero è andato fuori sincrono un'altra volta: ogni ramo aveva ricalcolato il totale sulla **propria** base,
> uno diceva 28 e l'altro 29, e nessuno dei due era giusto sull'unione.
>
> ⛔ **Il numero non si rimisura più con un generatore.** `feature_registry.py shortlist` e
> `scenariomap.shortlist.md` sono usciti il 2026-08-21 (**D-181**) col Feature Registry, e con loro il
> registro degli scenari pianificati da cui il conteggio si ricavava.
>
> ⚠️ **E un comando scritto a mano qui aveva già sbagliato**: il `python -c` che questa nota portava prima
> del generatore camminava solo le voci `- planned:` in forma di lista e non vedeva la forma `planned:` a
> chiave nuda — dava **24** dove il vero numero era **34**. Chi ne riscrive uno parta da lì, e conti
> **entrambe** le forme.

| ScenarioId pianificato | Feature | Release |
|---|---|---|
| `Stress.4v4.CoreRoster` | `RT-FEAT-STRESS-4V4` | v0.1 · E17 (`#221`) |
| `Team.Conflux.GadgetPhase.ConductiveFlood` | `RT-FEAT-FACTION-SCENARIOS` | v0.2 |
| `Team.Constrine.RiktorWraith.OnlyExit` | idem | v0.2 |
| `Team.Sentinel.SteelMurdock.HoldTheLine` | idem | v0.2 · richiede E35 |
| `Team.Resonance.AuroraKwang.FrozenAnchor` | idem | v0.2 · richiede E35 |
| `State.Phase.Flow` · `State.Gadget.Charged` · `State.Riktor.Bulwark` · `State.Howitzer.Siege` · `State.MultiState.Stress` | `RT-FEAT-CHARACTER-STATE` | v0.4 · E34 (`#244`) |
| `Spec.Clash.HiddenUntilReveal` · `Spec.Clash.RevealIsFixedDeadline` · `Spec.Clash.Determinism` | `RT-FEAT-REACTION-CLASH` | v0.1 · E14 · CP 14.7 |
| `Spec.Brace.AnchorResistsDisplacement` · `…FlowRedirectsToLegalHexOnly` · `…DeflectOffersOnlyLegalSides` | `RT-FEAT-REACTION-PROFILE` | v0.1 · E14 · CP 14.7 |
| `Spec.Overwatch.ConductiveDischargeUsesStandardConduction` · `…PressurePushChangesResolvedPath` · `…FrontlineFollowsFacing` | `RT-FEAT-REACTION-OVERWATCH` | v0.1 · E14 · CP 14.4 |
| `Spec.TimeBank.GraceDoesNotDrain` · `…DrainsAfterGrace` · `…NeverBelowZero` · `…TimeoutCostsFullWindow` · `…TimeoutSpendsNoCharge` · `…ClashCostsFullWindow` · `…BotDrainsLikePlayer` · `…ExhaustionKeepsResponsesLegal` · `…ReplayReadsRecordedBank` · `…PacketOrderInvariant` · `…ControlLoadScalesInitialBank` · `…ControlLoadNeverExtendsWindow` · `…TimeoutIgnoresPreferredResponse` | `RT-FEAT-CORE-DECISION-TIME-BANK` | v0.1 · E14 · CP 14.8 |
| `Spec.Map.WallCrossesCellStillStandable` · `…FootprintCollisionBlocksCell` · `…NinetyDegreeCornerBakesCorrectly` | `RT-FEAT-MAP-STANDABILITY` | v0.1 · E23 · CP 23.6 |
| `Spec.Map.ValidCellsBlockedTransition` · `…DoorOpensTransition` | `RT-FEAT-MAP-TRANSITION-CLEARANCE` | v0.1 · E23 · CP 23.7 |
| `Spec.Map.Interaction.SwitchOpensDoor` · `…SwitchControlsMultipleDoors` · `…OpenFailsDependentMoveBlocks` | `RT-FEAT-MAP-INTERACTION-GRAPH` | v0.1 · E23 · CP 23.4 (`#833`) |

> ➕ **I `Spec.TimeBank.*` passano da dieci a TREDICI il 2026-08-17** ([D-156](../../decisions/RT_PDR_00_Decision_Log.md),
> [D-157](../../decisions/RT_PDR_00_Decision_Log.md)): `ControlLoadScalesInitialBank`,
> `ControlLoadNeverExtendsWindow` e `TimeoutIgnoresPreferredResponse`. Il numero è la somma delle righe
> marcate `harness` nella tabella §13 di [`../../gameplay/spec-decision-time-bank.md`](../../gameplay/spec-decision-time-bank.md),
> **riparsata**, non incrementata a mano — ed è la stessa disciplina che questa pagina ha già dovuto imparare
> due volte qui sotto.
> ⚠️ **Quattro voci nate nello stesso giro NON entrano in questo conteggio, e la ragione è il livello, non
> l'importanza**: `PreferredResponseFallsBackWhenStale`, `PreselectionSpendsNoCharge` e
> `PreferredResponseKeepsAllowedResponses` sono `estende` — le ospita un test C++ che esiste già, e
> duplicarle come file darebbe due verità sullo stesso comportamento; `QuickConfirmReachableWithinGrace` è
> `funzionale · UI`, perché la **preselezione è uno stato di interfaccia** e il TurnLog registra la risposta
> committata, non ciò che era evidenziato. Un oracolo che l'harness non ha non diventa harness perché lo
> scriviamo nella colonna.
> ⚠️ **I conteggi delle note datate qui sotto non sono stati riscritti**: dicono ciò che era vero alla loro
> data, e correggerli li renderebbe falsi due volte.

> ✅ **I tre `Spec.Clash.*` e i dieci `Spec.TimeBank.*` sono scrivibili — rimisurato il 2026-08-13.** Questa
> sezione ha portato per giorni le due note qui sotto, che li dichiaravano *impossibili*: erano vere quando
> furono scritte e hanno smesso di esserlo il **2026-08-10**, senza che nessuno le riaprisse.
> `ERTAssertionKind` ha oggi **otto** assertion, non cinque — alle cinque di stato finale (`UnitAtCell`,
> `TurnsCompleted`, `UnitHpEquals`, `UnitAlive`, `UnitFacing`) si sono aggiunte `LogEventCount` e
> `LogEventOrder` (`#318`) e `LogEventAmount` (`#361`, commit `a7e4677b`, verificata su parser, valutatore e
> test). Le tre leggono il TurnLog, che è esattamente ciò che mancava. Restano **da scrivere**: li scrivono
> CP 14.7 e CP 14.8, e non aspettano più nulla dell'harness.

> ~~⚠️ **I tre `Spec.Clash.*` non sono «da scrivere»: oggi sono *impossibili*.**~~ *(superata il 2026-08-10)*
> Era la stessa situazione già incontrata dal facing prima di CP 16.1: le assertion leggevano **tutte lo stato
> finale**, e questi tre chiedono esattamente il contrario — che una scelta non sia visibile *prima* di un
> certo evento, che il reveal non anticipi, che due run producano lo stesso `LogHash`. Gli altri quattro
> `Spec.Clash.*` esistevano già perché il loro esito è osservabile nello stato finale.

> ~~⚠️ **Gli otto `Spec.TimeBank.*` hanno lo stesso problema, ma per intero.**~~ *(superata, e sbagliata due
> volte nel frattempo)* Sono **dieci** dalla riconciliazione di `#361`, non otto; e la frase «nessuno dei tre»
> con cui la nota si apriva non aveva alcun antecedente — nel blocco non c'era nessun «tre». Il contenuto che
> resta vero è il **perché**: riguardano quanto tempo è stato speso, cioè un valore che vive nel TurnLog e non
> nello stato finale della mappa, ed è per questo che l'assertion sul contatore era il blocco. Il conteggio di
> ciò che quella capability ha sbloccato è **tredici** — tre `Spec.Clash.*` più dieci `Spec.TimeBank.*` — mai
> undici.

> 🔴 **La lezione è la forma del difetto, non i numeri.** `#318` e `#361` sono state chiuse il 2026-08-09 e la
> terza assertion è atterrata poche ore dopo; le righe che le dichiaravano bloccanti sono rimaste in **quattro**
> derivati — questa sezione, il commento di `feature-registry.yaml`,
> `scenariomap.shortlist.md` §6.2 e il «prerequisito bloccante» di
> [`#319`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/319) — per **quattro giorni**, tutte
> con lo stesso numero sbagliato. Chiudere l'issue che porta una capability non chiude le righe che la
> dichiaravano assente: quelle si cercano per **numero di issue**, non per argomento.

> ✅ **I cinque `Spec.Map.*` sono il caso opposto, e vale la pena dirlo.** Non chiedono nessuna assertion
> nuova: «l'unità è arrivata» e «l'unità è rimasta dov'era» si scrivono con `UnitAtCell`, che c'è. Quello che
> manca è **il dato**, non l'oracolo — le celle cotte da geometria arrivano con CP 23.6/23.7, in v0.2. Sono
> quindi `planned` per una ragione diversa dai tredici sopra, e non entrano nel conteggio di ciò che `#318`
> e `#361` hanno sbloccato.
>
> Con un'eccezione da non nascondere: `Spec.Map.NinetyDegreeCornerBakesCorrectly` verifica anche LOS e
> traiettoria vicino all'angolo, e lì l'oracolo diretto non esiste — si osserverebbe **di rimbalzo**, dal
> danno andato a segno o no (`UnitHpEquals`). È un oracolo indiretto, e un test che misura la vista contando
> ferite è più fragile di quanto sembri. Va deciso quando `MAP-2` si chiude.

> ✅ **I tre `Spec.Map.Interaction.*` sono stati scelti, non trascritti — 2026-08-14, [D-138](../../decisions/RT_PDR_00_Decision_Log.md).**
> Il consolidamento del dominio muri/porte
> ([referto](../../roadmap/plans/walls-doors-interaction-spec-panel-2026-08-13.md)) ne proponeva **quattordici**,
> distribuiti su privacy, rete, Operations e authoring. Tre sono entrati, e il criterio è l'unico che conti:
> **hanno un oracolo oggi**. «L'unità è arrivata oltre la porta» e «l'unità è rimasta dov'era» si scrivono con
> `UnitAtCell`, che c'è.
>
> Gli altri undici sono rimasti fuori, e non per prudenza:
>
> - i cinque di **privacy** (`HiddenControllerRelation`, `NoHiddenRelationLeak`, `KnownToOneTeamOnly`…)
>   passerebbero **per assenza di rete**, non per correttezza — un test che non può fallire non è un test.
>   Il loro oracolo nasce con `E27` (v0.3, Team Knowledge) e diventa verificabile con `E40` (v0.5);
> - i tre di **identità e validazione** (`StableIdSurvivesCook`, `DuplicateBindingFailsValidation`…) non sono
>   scenari: nessuna delle otto `ERTAssertionKind` legge l'identità di una struttura o l'esito di una
>   validazione d'asset. Vivono in automation, e le issue `#832`/`#833` li elencano lì;
> - i tre di **scala** (`Gate.MultiTransitionLarge`, `Operations.InteractionScale`…) misurano performance su
>   mappe che `E30` (v0.4) non ha ancora prodotto.
>
> 🔴 È la stessa forma già pagata dai `Spec.Clash.*`, ruotata: **là** un oracolo mancante teneva bloccati
> scenari scrivibili, e le righe che li dichiaravano impossibili sono sopravvissute quattro giorni alla
> capability che li sbloccava. **Qui** il rischio è l'opposto — scrivere lo scenario prima dell'oracolo e
> lasciarlo verde per assenza. Un `BLOCKED` che scivola in verde non lo denuncia nessuno.

> ✅ **I `BAL-1` non sono più in questa lista: chiusi il 2026-08-10** (`#401`, `#402`). Tre scritti e verdi
> al primo run — `Spec.Brace.GuardAndBraceOnMixedHit`, `Spec.Brace.BraceWinsOnSecondHit` e
<!-- rename-exempt: la riga dichiara la rinomina: sostituirla la renderebbe muta -->
> `Spec.Combat.RiktorIsPushedLikeAnyone`, quest'ultimo **rinominato** da `…BastionIgnoresAllPushes` perché
> [D-075](../../decisions/RT_PDR_00_Decision_Log.md) ha deciso dall'altra parte e il nome previsto avrebbe
> significato il contrario del file. Il quarto, `Spec.Brace.PushBeyondGuardThreshold`, **non nasce**: chiedeva
> una spinta di 2 che [D-074](../../decisions/RT_PDR_00_Decision_Log.md) ha deciso di non introdurre, riscrivendo
> invece la clausola di `Brace`. Oracolo e fixture erano quelli previsti — `UnitHpEquals`, `UnitAtCell` e
> `Hero.Phase.PressureJet` (16 danni **e** `Push 1` nello stesso colpo) — e nessuna capability nuova è servita.
>
> Vale la pena notare **perché** questi quattro mancavano: `Guard` e `Brace` hanno ciascuno i propri scenari
> e nessuno li guarda **insieme**. È la stessa forma del *dato senza consumatore*, ruotata — qui i
> consumatori ci sono, manca il test che li confronta.

> ✅ **Aggiornato il 2026-08-09** (`#318`, PR di `feat/318-assertion-turnlog`). Le prime due primitive
> esistono: `LogEventCount` (con `value: 0` per l'assenza) e `LogEventOrder`. **I tre `Spec.Clash.*` sono
> quindi scrivibili** — restano da scrivere, ed è lavoro di CP 14.7, non di #318.

> ⚠️ **I sei `Spec.Brace.*` / `Spec.Overwatch.*` del 2026-08-10 sono bloccati da altro, e la differenza
> conta.** Non aspettano una capability dell'harness — un anti-displacement e una lista di risposte legali si
> osservano benissimo con `UnitAtCell` — aspettano una **decisione**: quali siano i quattro profili di `Brace`
> e di `Overwatch` è `BAS-1` e `BAS-2` in [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md). Scriverli prima
> significherebbe fissare in un file eseguibile un contenuto che nessuno ha approvato, ed è il modo più
> efficace di far sembrare decisa una proposta. Sono in **classe D per scelta**, non per debito tecnico.
> Origine e triage: [`../../roadmap/plans/baseaction-signatures-spec-panel-2026-08-10.md`](../../roadmap/plans/baseaction-signatures-spec-panel-2026-08-10.md).
>
> Per i `Spec.TimeBank.*` l'affermazione «una sola capability li sblocca tutti» **non reggeva alla verifica**.
> I tre punti sono stati riconciliati il 2026-08-09 con l'issue **`#361`**, e il risultato è questo:
>
> 1. **il contatore ha un formato deciso.** `spec-turnlog.md` §4.2 dichiara una categoria `Decision` in coda a
>    `ERTLogCategory` e **due** voci — `BankConsumed` e `BankAfter` — con il valore in `Amount`. Non tre:
>    `BankBeforeMs` è la somma delle due voci adiacenti della stessa decisione, e ciò che §6 della spec del
>    bank vieta è ricostruire il residuo sommando la *storia*, non leggere due numeri scritti accanto. Nessun
>    campo nuovo in `FRTTurnLogEntry`, quindi il formato serializzato non cambia versione;
> 2. **il livello è deciso e vale `harness`** per dieci scenari. §13 li classificava `golden` perché, quando
>    fu scritta, un file non poteva verificarli: con `LogEventCount`/`LogEventOrder` (`#318`) e il contatore
>    di cui sopra, possono — ed è la forma migliore, per la stessa ragione di §6.3. Due (`OverwatchTimeoutIsHold`,
>    `HoldKeepsReactionArmed`) **estendono un test C++ esistente** e non diventano file: duplicarli darebbe due
>    verità sullo stesso comportamento;
> 3. **le liste non divergono più, e la divergenza era descritta male.** Misurata: §13 elenca **20 nomi** in 18
>    righe (non 13), questa mappa ne elencava 8, e gli orfani erano **zero** — `ClashCostsFullWindow` e
>    `PrivacyNoBankLeak` esistono entrambi nella spec. Mancavano invece qui `NeverBelowZero`,
>    `ExhaustionKeepsResponsesLegal` e `PacketOrderInvariant`; ed era di troppo `PrivacyNoBankLeak`, che è
>    `funzionale · M10` — multiplayer, quindi fuori dalla v0.1 offline.
>
> Conteggio onesto dopo la riconciliazione: **tredici** scenari sbloccati — i tre `Spec.Clash.*` e i dieci
> `Spec.TimeBank.*` — e nessuno di essi è scritto. Li scrivono CP 14.7 e CP 14.8.
> ✅ **L'assertion sul contatore è fatta**, e questa riga diceva il contrario fino al 2026-08-13:
> `LogEventAmount` è atterrata il **2026-08-10** con `a7e4677b`, poche ore dopo la decisione di formato che la
> sbloccava. Dei tredici scenari non ne resta bloccato **nessuno** dall'harness — restano da scrivere, che è
> un'altra cosa.

Le **10 voci `PIE-STATE-*`** del registro sono la controparte umana di questi cinque: nascono ⏳ e restano ⏳
finché E34 non esiste. Stanno nel registro perché il ciclo *docs → epic → scenario → PIE* resti chiuso, non
perché siano eseguibili.

### 6.3 Dichiarati e mai scritti — la fascia D che non è atterrata

La **fascia D** di [`scenari-validazione-visiva.md`](../runbooks/scenari-validazione-visiva.md) elenca 8 scenari `Visual.*`
descritti come «scritti adesso con `requires`». **Nessuno degli otto file esiste**, verificato il 2026-08-09:
nessun `Visual.*` del corpus dichiara un `requires` diverso da `Reaction`, che è disponibile.

Quattro dei temi sono stati poi scritti come **`Spec.*`** (Overwatch, Predictive, Objective, Perception), che
è la forma migliore — una specifica eseguibile invece di una vetrina cieca. I restanti quattro
(`Visual.Facing.Cone`, `Visual.Intercept.Revalidation`, `Visual.CoverWindow.OpenFireSeal`,
`Visual.Interaction.DoorGraph`) non esistono in nessuna forma.

Non è un difetto da correggere scrivendo otto file: è una **fascia da riscrivere** come intenzione, perché
oggi promette file che non ci sono. Registrato in §8.

> 🔵 **La camera non è nemmeno in fascia D, ed è un caso diverso da quello sopra** *(2026-08-14)*.
> `RT-FEAT-UI-TACTICAL-CAMERA` ha `scenarios: []` e gate `scenario: todo`: **zero** scenari, né scritti né
> pianificati né promessi. `grep -ci camera docs/technical/scenario-map.md` rispondeva **0** prima di questa
> riga. Non è una promessa non mantenuta come i quattro `Visual.*` — è un'assenza che **nessun documento
> dichiarava**, e la differenza conta: una fascia D sbagliata si corregge riscrivendo l'intenzione, un vuoto
> non dichiarato non ha niente da correggere finché qualcuno non decide cosa vada verificato.
>
> Il consolidamento del *Camera Roadmap v1.0* ne ha proposti **34**
> ([triage](../../roadmap/plans/camera-roadmap-v1-triage-2026-08-14.md)), e **nessuno è stato aggiunto qui**:
> i loro nomi (`CAMERA-BASIC-PAN-ZOOM-ROTATE`, `CAMERA-CUTAWAY-ROOF`, …) non seguono la convenzione del
> corpus — `Visual.*`, `Spec.*`, `Movement.*` — e **la maggior parte** verifica sistemi che non esistono:
> Strategic View, Camera Director, `ActiveLayer`, cutaway, spectator, marker verticali. Uno ScenarioId
> scritto prima del suo soggetto è una voce che esce `BLOCKED` e resta a sembrare copertura.
> ⚠️ **«La maggior parte» è deliberatamente non un numero.** La prima stesura diceva «trenta dei
> trentaquattro», che nessuna partizione dei 34 nomi produce — e stava accanto a un `grep -ci camera → 0`
> misurato, quindi si leggeva come misurato anch'esso. Contarli richiederebbe decidere caso per caso se un
> nome descrive un sistema assente, e quella classificazione non è stata fatta. Trovato in code review.
>
> Quello che oggi **è** verificabile senza inventare sistemi sono `FocusOn` e `FrameOwnTeam`, e non serve
> uno scenario: sono automation test puri, aperti come
> [#865](https://github.com/DegrassiAaron/refactor-tactics-main/issues/865). La camera entra in questo
> documento quando avrà qualcosa che un occhio deve guardare — non prima. Registrato in §9.

---

## 7. Come si rimisura

Nessun numero di questo documento va aggiornato a memoria. Tutti si ricalcolano:

```bash
# Classe A + B + D-scritti — il corpus versionato
find Scenarios -name '*.json' ! -name '_*' | wc -l                        # 54
find Scenarios/Visual -name '*.json' | wc -l                              # 21  (B)
find Scenarios/Spec   -name '*.json' | wc -l                              # 19  (12 in D + 7 accesi in A)

# Registro PIE — i totali della tabella §2 e della §5
# ⚠️ Aggiunti il 2026-08-13: la §2 diceva 117 e la §5 diceva 95 perche' NESSUN comando qui sotto
#    produceva quei due numeri. Il blocco `awk` seguente conta i marcatori, non le voci.
grep -c '^| \*\*PIE-'     docs/technical/test-manuali-pie.md            # 135  totale
grep -c '^| \*\*PIE-VIS-' docs/technical/test-manuali-pie.md            #  21  classe B
grep -c '^| \*\*PIE-MUT-' docs/technical/test-manuali-pie.md            #   2  fuori classe
#                                                        classe C = 135 - 21 - 2 = 112

# La ripartizione per sezione della §5 — deve sommare a 112 una volta tolte le due PIE-MUT-*
awk '/^#{2,3} /{s=$0} /^\| \*\*PIE-/{c[s]++} END{for(k in c) print c[k], k}' \
    docs/technical/test-manuali-pie.md | sort -rn

# Registro PIE — verdi / parziali / aperte
awk -F'|' '/^\| \*\*PIE-/ {s=$(NF-1);
  if (match(s, /✅|🟡|⏳/)) c[substr(s, RSTART, RLENGTH)]++; else c["nessuno"]++ }
  END {printf "verde=%d parziale=%d aperta=%d senza-marcatore=%d\n",
       c["✅"], c["🟡"], c["⏳"], c["nessuno"]}' docs/technical/test-manuali-pie.md

# Classe B — la corrispondenza scenario Visual ↔ voce PIE deve restare 1:1
echo "scenari: $(find Scenarios/Visual -name '*.json' | wc -l)  \
voci: $(grep -c '^| \*\*PIE-VIS-' docs/technical/test-manuali-pie.md)"

# Subset di release: deve valere quanto la tabella §8
grep -c '^| \*\*PIE-[A-Za-z0-9.-]*\*\* `RELEASE-V01`' docs/technical/test-manuali-pie.md    # 17

# ...e il suo stato, che è ciò che G9 deve poter leggere senza contare a mano
awk -F'|' '/RELEASE-V01/ && /^\| \*\*PIE-/ {s=$(NF-1);
  if (match(s, /✅|🟡|⏳/)) c[substr(s, RSTART, RLENGTH)]++ }
  END {printf "verde=%d parziale=%d aperta=%d\n", c["✅"], c["🟡"], c["⏳"]}' \
  docs/technical/test-manuali-pie.md
```

```bash
# I `planned` della §6.2 — il numero che fino al 2026-08-16 non aveva un comando, ed era fermo di sette
python - <<'PY'
import yaml
d = yaml.safe_load(open('docs/roadmap/feature-registry.yaml', encoding='utf-8'))
n = []
for f in d['features']:
    raw = f.get('scenarios') or []
    if isinstance(raw, dict):
        n += list(raw.get('planned') or [])
    else:
        for it in raw:
            if isinstance(it, dict) and 'planned' in it:
                n += it['planned'] or []
print(len(n), 'planned ·', len(set(n)), 'unici')   # 63 · 63
PY
```

> ⚠️ **Un numero senza comando va fuori sincrono, e questa sezione ne è la prova al contrario**: gli otto
> comandi qui sopra tengono in riga i loro numeri, mentre l'unico che ne era privo — i `planned` della §6.2 —
> è l'unico che aveva divergito. Il rimedio non è ricordarsi di aggiornarlo: è il blocco qui sopra.

> ⚠️ Il conteggio del subset **non** si fa con `grep -c 'RELEASE-V01'` nudo: il marcatore compare anche nella
> prosa che lo spiega, e quella forma dà **24**. È lo stesso difetto del comando di conteggio del registro
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
| `PIE-FACING-1` | l'orientamento che si **vede** è quello che il resolver ha **usato** | ⏳ |
| `PIE-HEXPLAY-10` | **partita completa fino alla vittoria** — è G10 | ⏳ |
| `PIE-CAM-START` | la partita si apre sulla propria squadra | ✅ |
| `PIE-V01-MATCHEND` | **fine partita a tre vie**, a schermo, e `R` riavvia | ⏳ |
| `PIE-V01-HUD` | HUD di partita completo. Il **valore** del limite di round viene già dal formato (`RTHUD.cpp:403`), la **parola** no — `:405` stampa `"Turno"`, il DoD prescrive *round*. Resta sul Canvas: lo Screen HUD §4.1 di CP 11.7 (`#613`) avrà una voce propria | ⏳ |
| `PIE-V01-LOG` | combat log con reason code leggibili | 🟡 |
| `PIE-V01-INTENT` | intenti alleati e **nessun** intento avversario visibile | 🟡 |
| `PIE-V01-ROSTER` | i quattro eroi si sentono diversi da giocare | 🟡 |
| `PIE-PREVIEW-AREA` | **leggibilità minima**: si capisce cosa si sta per colpire, prima del lock-in | ✅ |

**17 voci: 2 verdi, 7 parziali, 8 aperte** — misurate col comando di §7, non contate a mano dalla tabella.

`PIE-PREVIEW-AREA` è passata a ✅ il 2026-08-09 dopo **tre** difetti, nessuno dei quali un test poteva vedere:
un contorno disegnato sotto il cilindro, un linguaggio che parlava di celle mentre la domanda era sulle unità,
e una selezione che non selezionava. Lascia dietro `PIE-PREVIEW-PERSIST`, ⏳: l'avviso di fuoco amico spariva
al cambio di selezione, cioè **proprio mentre** si finisce il turno. **Non l'ho messa nel subset** — è il
residuo di una voce che c'è già, e il criterio conta le capacità, non i difetti aperti su di esse. Se il
playtest dice che l'avviso che svanisce rende la voce padre inaffidabile, allora entra: sarebbe una revisione
del criterio, non una svista.

**Tutte e diciassette stanno in una seduta dichiarata del registro.** Le nove del subset `RELEASE-V01`:
`PIE-HEXPLAY-1` in **U2**, `-2` e `-3` in **U3**, `-8` e `PIE-FACING-1` in **U6**, `PIE-V01-ROSTER` in
**U11**, `PIE-V01-HUD`, `-INTENT` e `-LOG` in **U15**.

> ⚠️ **Corretto il 2026-08-12.** Questo paragrafo diceva «**nove non stanno in nessuna**» e concludeva che
> «assegnarle a una seduta è il prossimo passo naturale di G9». Misurando i campi `verifies:` di
> `editor-sessions.yaml`, l'assegnazione **c'è per tutte e nove**: l'affermazione era vera quando è stata
> scritta e nessuno l'ha rimisurata dopo che U11 e U15 sono nate. Il prossimo passo di G9 è **eseguirle**.
> Il conteggio delle voci davvero orfane — 55, nessuna nel subset — vive in
> `../../roadmap/editormap.shortlist.md`, che è coerente con questa misura.

`PIE-FACING-1` è entrata col merge di E16, e non per completezza: dal CP 16.2 l'emisfero posteriore è
**scoperto**, quindi il facing decide il danno. Un orientamento visibile diverso da quello che il resolver ha
usato rende impianificabile la difesa direzionale — è leggibilità minima nel senso stretto, non presentazione.

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
| ~~**Azioni core senza possessore**~~ — ✅ **chiuso il 2026-08-09** (`#282`, `75b8264`): `Flux.ConductiveNode` è `Action.Electrify` e `Riva.FluidTrail` è `Action.CreateWater` | I tre scenari sono passati in classe **A**. `Ignite` e `ModifyArc` restano **senza owner per decisione**, non per debito ([D-046](../../decisions/RT_PDR_00_Decision_Log.md)): nessun eroe del roster ha affinità col fuoco, i ponti non appartengono a nessun kit | — | <!-- rename-exempt: misura datata: riscriverla la renderebbe falsa -->
| **Fascia D mai atterrata** — 8 `Visual.*` descritti come scritti, 0 file | Il catalogo promette vetrine che non esistono; 4 temi vivono come `Spec.*` | §6.3, da riscrivere in `scenari-validazione-visiva.md` |
| **La camera non ha copertura, e non è un debito rinviato** — `RT-FEAT-UI-TACTICAL-CAMERA` ha `scenarios: []` e gate `scenario: todo`: zero scenari, né scritti né pianificati né promessi | Nessuno **oggi**: ciò che è verificabile della camera attuale sono automation test puri ([#865](https://github.com/DegrassiAaron/refactor-tactics-main/issues/865)), non scenari. Il buco si apre davvero quando esisterà qualcosa da *guardare* — marker verticali, cutaway, transizione Strategic — e allora servirà anche una convenzione di ScenarioId per la camera, che non esiste | §6.3 · [`../../roadmap/plans/camera-roadmap-v1-triage-2026-08-14.md`](../../roadmap/plans/camera-roadmap-v1-triage-2026-08-14.md) |
| **`Visual.Reaction.*` esiste, `Spec` no** — il campo `reaction` nell'intent c'è ed è validato | Nessuno: è un buco **chiuso**, registrato perché la documentazione lo dichiarava aperto per una working copy indietro di qualche commit | `scenari-validazione-visiva.md` §8.2 |
| **Nessuna assertion su punteggio e conoscenza** — mancano `TeamScoreEquals` e un modo di asserire sulla conoscenza di squadra | Due scenari-spec non potranno diventare verdi anche quando la capability atterra | `_nota_da_completare` di `Spec.Objective.*` e `Spec.Perception.*` |
| ~~**Nessuna assertion che legga il TurnLog**~~ — ✅ **chiuso il 2026-08-09** (`#318`): `LogEventCount` e `LogEventOrder` leggono il log accumulato dalla sessione, e l'evento si nomina per nome | I tre `Spec.Clash.*` sono **scrivibili** (li scrive CP 14.7). Gli otto `Spec.TimeBank.*` **no**: manca il contatore, e prima serve la decisione su come tre valori in millisecondi entrano in un `FRTTurnLogEntry` che ha un solo `Amount` — vedi §6.2 | `test-automatico-unreal.md` §5.1 |
| ~~**Il contatore del log, e le due liste `Spec.TimeBank.*` che divergono**~~ — ✅ **chiuso il 2026-08-09** (`#361`): `spec-turnlog.md` §4.2 dichiara la categoria `Decision` e due voci (`BankConsumed`, `BankAfter`) con il valore in `Amount`; le liste sono riconciliate a **dieci** scenari `harness`. I conteggi della riga precedente erano sbagliati in entrambi i sensi: §13 elencava **20** nomi, non 13, e gli orfani erano **zero** | I dieci `Spec.TimeBank.*` sono **scrivibili** appena esiste l'assertion sul contatore, che ora ha un formato da leggere. Li scrive CP 14.8 | §6.2 · `spec-turnlog.md` §4.2 |
| ~~**L'assertion sul contatore del log**~~ — ✅ **chiusa il 2026-08-09** (`#361`): `LogEventAmount` legge `Amount` della prima voce che corrisponde, in coda all'enum come `UnitFacing` e `LogEventCount` prima di lei | I dieci `Spec.TimeBank.*` non hanno più un ostacolo tecnico: restano **da scrivere**, ed è lavoro di CP 14.8 | `test-automatico-unreal.md` §5.1 |
| **Il frontend non ha copertura di nessuna classe, e le sue sei voci di classe C non sono scrivibili da chi le ha specificate** *(2026-08-16)* — le quattro feature `RT-FEAT-UI-FRONTEND-*` di **E46** nascono con `scenario: na` e `automation: todo`: ~~il repository non ha infrastruttura di test UI, quindi non esiste né uno scenario né un automation test che istanzi un widget e ne verifichi la navigazione~~ 🔴 **falso dal 2026-08-16**: CP 46.1 ([`#936`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/936)) ha prodotto **17 automation test** che fanno esattamente questo — istanziano un widget e verificano la navigazione, senza `.uasset`. Resta vero che **nessuno scenario** copre il frontend, ed è corretto: uno scenario descrive una partita, e il menu non è in partita (`scenario: na` nel registry) | Oggi **nessuno**, perché E46 è `SPECIFIED` e non c'è codice. Il buco si apre al primo checkpoint implementato: i sei DoD nominano `PIE-V01-FRONTEND-NAV`, `-ERROR`, `-MAIN`, `-PLAY`, `-RESULT`, `-PAUSE`, e **nessuna di quelle voci esiste nel registro**. ⚠️ Non sono state create per un vincolo di processo, non per dimenticanza: [`test-manuali-pie.md`](../test-manuali-pie.md) appartiene dal 2026-08-16 alla track **`playtest`** e D-139 dice che un file non assegnato è uno **stop**. La procedura non è aspettare: quella track dichiara che *«chi finisce una feature che ha una voce `PIE-*` non scrive il proprio esito qui: lo **propone** in handoff»*. Le sei voci si propongono quando il primo checkpoint di E46 produce qualcosa da guardare. Finché non succede i conteggi della §2 e della §5 **non** includono E46, il che è corretto: contarle sarebbe dichiarare un registro che non le contiene | [`spec-frontend-navigazione.md`](../architecture/spec-frontend-navigazione.md) §8 · [D-144](../../decisions/RT_PDR_00_Decision_Log.md) |
| **Nessuna assertion sul determinismo** — e non deve esserci: `HashTurnLog` **ordina** prima di mescolare, quindi è invariante per permutazione e non vede l'ordine; un hash letterale in un JSON si romperebbe alla prima voce nuova del log | `Spec.Clash.Determinism` va scritto sull'ordine (`LogEventOrder`), non su un checksum. Il determinismo vero si verifica **eseguendo due volte**: è una proprietà del runner | `test-automatico-unreal.md` §5.1 |
| **Un'aspettativa non dichiara la propria natura, e questo è ciò che manca al Tactical Designer** *(2026-08-17, [D-154](../../decisions/RT_PDR_00_Decision_Log.md))* — `ERTAssertionKind` dice **cosa** si verifica (`UnitAtCell`, `LogEventCount`, …), non **di che natura** è l'aspettativa. Le quattro nature sono distinte in [`spec-tactical-designer.md`](spec-tactical-designer.md) §7 — invariante forte · aspettativa di design · soglia di bilanciamento · osservazione di telemetria — e nel formato non esistono | Oggi **nessuno**: con un corpus intero scritto a mano da chi conosce il proprio caso, la distinzione vive nella testa dell'autore. Il buco si apre al primo confronto `baseline ↔ variante`, quando qualcosa dovrà **classificare** una differenza: una soglia di bilanciamento superata è un'informazione, un'invariante caduta è un difetto, e trattarle uguali produce l'errore in entrambi i versi — un nerf che sembra una regressione, o una regressione archiviata come nerf. ⚠️ **Non si chiude con una convenzione sui nomi**: dedurre la natura dal prefisso dello `ScenarioId` sarebbe lo string matching fragile proprio dove serve robustezza. È un DoD di `RT-FEAT-TOOL-SKILL-WORKBENCH`, non un lavoro separato | [`spec-tactical-designer.md`](spec-tactical-designer.md) §7 · `RTTestScenario.h` |
| **Le due feature del Tactical Designer nascono con `scenario: todo` e zero scenari, ed è corretto** *(2026-08-17)* — `RT-FEAT-TOOL-SCENARIO-COMPOSER` e `RT-FEAT-TOOL-SKILL-WORKBENCH` sono `DESIGNED`, senza codice e senza issue aperte | Nessuno **oggi**, e il conteggio della §2 non le include — contarle dichiarerebbe un corpus che non esiste. ⚠️ Quando atterreranno, la loro copertura è di **classe A** e non B: *«l'authoring visuale produce lo stesso `FRTTestScenario` di una fixture scritta a mano»* è una proprietà di **round-trip**, che una macchina giudica meglio di un occhio. La classe C resta per la sola **leggibilità** della timeline, e vive nella seduta **U26** | [`spec-tactical-designer.md`](spec-tactical-designer.md) §9 · `../../roadmap/feature-registry.yaml` |

## 10. Rapporto con gli altri documenti

| Documento | Rapporto |
|---|---|
| [`test-manuali-pie.md`](../test-manuali-pie.md) | **Owner** delle voci: stato ed esito atteso si scrivono lì. Qui se ne classifica l'esecutore e se ne dichiara il subset di release |
| [`scenari-validazione-visiva.md`](../runbooks/scenari-validazione-visiva.md) | **Owner** della classe B: cosa si guarda, con quale fixture e quali numeri |
| [`scenario-index-e-tag.md`](scenario-index-e-tag.md) | **Owner** dell'identità: `ScenarioId`, tag, redirect, indice |
| [`../../roadmap/v0.1-definition-of-done.md`](../../roadmap/v0.1-definition-of-done.md) | **Consumer**: G9 punta al subset di §8 |
| `../../roadmap/feature-registry.yaml` | **Consumer e sorgente**: il gate `scenario` di una feature è `done` solo se uno scenario la **dimostra**; gli `planned` di §6.2 vengono da lì |
| [`test-e-diagnosi.md`](../runbooks/test-e-diagnosi.md) | Come si scrive ed esegue uno scenario, e come si legge un report fallito |
