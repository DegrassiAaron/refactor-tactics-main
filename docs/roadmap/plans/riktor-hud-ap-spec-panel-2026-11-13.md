# Spec panel — l'HUD sugli Action Point e il catalogo di Riktor

> `SNAPSHOT` · **Scritto**: 2026-11-13 · **Misurato su**: `origin/main` = `9560053a2`
> **Richiesta dell'autore** (2026-11-13, `/sc:spec-panel`), alla lettera: *«dobbiamo rifare la ui/hud per seguire il nuovo sistema
> fatto sui action point. ricataloghiamo le skill e focalizziamoci solo su un personaggio , Riktor.»* Segue un catalogo di Riktor,
> riportato **integralmente** in [Appendice A](#appendice-a--il-catalogo-dellautore-alla-lettera): è un ingresso esterno
> ([`AGENTS.md`](../../../AGENTS.md) §8 *«Un work order esterno»*), non un owner, e questo referto è l'unico posto in cui resta citabile.
> **Input visivo**: `docs/research/skill.af`, file Affinity **non tracciato** nel clone principale (81 MB, 2026-11-12), letto estraendone
> le immagini incorporate ([Appendice B](#appendice-b--la-grammatica-delle-icone-di-skillaf)).
> **Metodo**: ricognizione nella sessione principale e brief di fatti verificati; quattro lenti del panel, ognuna una coppia di esperti
> in un agente, ognuna seguita da un verificatore avversario indipendente; un critico di completezza finale; sintesi e controllo a
> campione nella sessione principale. Nessuna premessa è stata riconciliata per plausibilità ([`AGENTS.md`](../../../AGENTS.md) §2).
> **Esito**: le risposte dell'autore (§9–§11) sono registrate in [D-612](../../decisions/RT_PDR_00_Decision_Log.md) (il kit di Riktor) e
> [D-613](../../decisions/RT_PDR_00_Decision_Log.md) (budget AP dell'unità, ripetizione, ordine dentro la fase, costo delle Ultimate), e la spec
> dell'HUD ha una §19-bis. Niente è implementato. Restano aperte la struttura del kit (`KIT-1`), chi riempie la dock (`KIT-2`), la skill di
> overclock di Riktor (`KIT-3`) e i dettagli delle sedici voci (§6).

## 1. Verdetto in breve

Il catalogo è un **buon input di design ma non è ancora una specifica**. Ogni gruppo ha un'idea leggibile e quasi tutte le voci
hanno un pezzo di motore da riusare. Più della metà, però, dipende da una decisione d'autore o da una regola che il contratto
esclude o non ha. I problemi stanno in tre famiglie.

1. **Che cosa entra nel kit.** Le «Opzione 1/2/3» ammettono quattro letture, e ognuna tocca una regola in vigore: il testo
   scelto di [D-598](../../decisions/RT_PDR_00_Decision_Log.md), il tetto `MaxActionCount = 6`, la struttura «attacco base ·
   quattro abilità fondamentali» del catalogo eroi, o il vincolo di **esattamente una** fondamentale con varianti per eroe.
   Anche con un'opzione sola per gruppo Riktor porta **sei abilità d'eroe oltre la base, contro le quattro della struttura**.
   Il kit nuovo sostituisce il kit «Field Architecture» di oggi (strutture, tiro indiretto, Interposition), mentre
   l'equipaggiamento di default che il bootstrap monta su Riktor (arma, gadget, modulo di reazione) resta finché DR-4 non decide.
2. **Regole che il contratto esclude.** Spicchi e «macro-esagono condiviso» (D-583 (4)), combo come proprietà dell'eroe
   (invarianti 15 e 16), «+1 AP» sul costo del movimento (D-584 (2)), «Reaction Phase» (D-583 (1)). Le combo a tre elementi
   non sono state adottate: l'opzione scelta di D-591 (1) registra il GDD come superato su quel punto.
3. **Regole che il contratto non ha ancora.** Il budget AP come stato dell'unità (+1 e −1 AP), il blocco delle cure, la
   gabbia, l'«ancorato a terra», le trappole nascoste, lo spostamento dentro una reazione, una reazione con due trigger, il
   trigger «colpito in mischia», se il cooldown ferma una seconda copia della stessa azione nello stesso turno, e il criterio
   d'ordine a parità di priorità dentro la fase.

**L'HUD** ha due metà. La **forma** si può scrivere subito, perché regge sia con il budget costante sia con un budget per
unità, sia con gli slot sia con la lista: budget del turno − somma pianificata = residuo, letti da una funzione sola; costo
dello slot dal catalogo e mai disegnato nell'icona; rifiuto `OverBudget` dalla logica; nessun AP mostrato per le unità
avversarie. Il **contenuto** dipende invece da decisioni e da motore che non esistono ancora: quante voci ha la dock, la
Ghost Timeline a pile, la lista delle reazioni, la ripetizione. Un HUD che oggi mostrasse due Blast o due reazioni
mostrerebbe un piano che il resolver non sa eseguire, perché gli slot restano in vigore finché E38 non consegna il resolver
a lista ([ADR-0011](../../decisions/adr-0011-action-point.md), testata).

**Raccomandazione.** (1) Decidere DR-1…DR-4 e DR-8, cioè la composizione del kit. (2) Tradurre il catalogo con la tabella-modello
di §3.1 e accettarlo con una `D-nnn`, dopo il rename di [#4011](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4011).
(3) Nel frattempo, portare in [`progettazione-hud.md`](../../technical/systems/progettazione-hud.md) i requisiti di §4.1, che
valgono in tutti i modelli. (4) Lasciare §4.2 a E38 e alle decisioni DR-5…DR-7.

## 2. Mappa delle premesse

Legenda: **TRADUCIBILE** = si esprime riusando o estendendo pezzi che esistono · **CONTRACT CONFLICT** = una regola decisa
la esclude · **DA DECIDERE** = nessuna regola la copre, serve l'autore · **AMBIGUO** = il testo ammette più comportamenti ·
**STALE** = nomi o fatti superati. Gli id fra parentesi sono i rilievi di §5.

| Voce del catalogo | Esito | Che cosa decide l'esito |
|---|---|---|
| «Opzione 1/2/3» per gruppo | DA DECIDERE | Quattro letture. **Alternative di design** fra cui l'autore sceglie: il validatore chiede comunque **esattamente una** fondamentale con varianti per eroe (`RTHeroCatalogLibrary.cpp:347-358`). **Varianti di loadout** su `FRTAbilityVariant`: sostituiscono solo gli effetti «quando la variante è selezionata nel loadout» (`RTActionData.h:34-40`), non fase, forma, portata, movimento o elemento, e chi le sceglie resta a E7 (`RTUnit.h:366-377`): hanno la stessa tensione con D-598 del loadout. **Tutte nel kit**: 16 voci contro `MaxActionCount = 6` (`RTHeroCatalogLibrary.cpp:218-219`). **Loadout pre-partita**: supera il testo scelto di D-598, «lo snapshot non porta un loadout» (REQ-01, ARC-01, AP-10, HUD-01) |
| Un'opzione per gruppo | DA DECIDERE | Base, Prep, Dash, Blast, Ultimate e due reazioni fanno sette voci. La struttura del catalogo eroi è «attacco base · **quattro** abilità fondamentali», più al più una generica del catalogo core ([`RT_HeroCatalog_v0.1.md`](../../balance/RT_HeroCatalog_v0.1.md):72-79), e le reazioni sono fondamentali (`Interposition` lo è per Branth). Il validatore controlla solo l'intervallo 5–6 (`RTHeroCatalogLibrary.cpp:205-219`): togliere una voce lo soddisfa, ma lascia la struttura violata |
| Il kit nuovo al posto di quello di oggi | DA DECIDERE | «Stesso kit» per Branth → Riktor è una **lettura** dell'assistente (D-586 (5)) e una riga di piano tecnico ([`piano-rename-inverso-roster-2026-11-09.md`](../../technical/piano-rename-inverso-roster-2026-11-09.md):13). Ma il testo scelto di D-585 (1) dichiara che cambiano «mesh, animazioni, icone e kit di **quattro** eroi» (gli slot di Phase, Wraith, Aurora e Kwang), e Riktor non è fra questi. Il kit nuovo va quindi **accettato** con una `D-nnn` dopo la traduzione (D-591 (3)), che allarga il perimetro di D-585 (1) (ARC-02, AP-09) |
| «Costante: n AP» e «Costanti: 1–2 AP» | AMBIGUO | D-594 (3) vuole un intero per riga, con il default della classe di fase. Un intero unico per gruppo è compatibile; «1–2» non è un intero (REQ-02) |
| Costi 2 · 1 · 2 · 2 · 3 | TRADUCIBILE | Coincidono con le classi di D-594. Le reazioni a 1 sono uno **scostamento dichiarabile** dal default 2. Un'Ultimate fuori dal Blast è ambigua: D-594 (3), testo scelto, dà classe +1 (Prep = 2), la lettura D-594 (4) dà 3 (DR-8) |
| Shock Whip | TRADUCIBILE con correzioni | «Su singolo spicchio» urta D-583 (4): il bersaglio è una cella. Dei «terreni bagnati» esiste solo l'acqua: Fango è un candidato OPEN (Water+Earth), Vapore un input GDD su una coppia già CURRENT (Fire+Water). La catena riusa `Reaction.ElectricPropagatesThroughWater` (REQ-03, REQ-11, AP-12) |
| Static Sweep | AMBIGUO | «120 distribuito su 3 esagoni»: 120 per cella o 40 per cella? Forma `Cone` o arco di 3 celle frontali, sul facing dopo la Dash (REQ-09) |
| Taser Strike | AMBIGUO | «Per il turno successivo» contro lo `Status.Slow` di oggi, che agisce sul Move **dello stesso turno** (`MakeBranth`, test `Combat.BranthImpactShotSlows`) (REQ-13, ARC-13) |
| Electrocute | AMBIGUO + DA DECIDERE | Un'aura «sull'esagono di Riktor» non colpisce nessuno finché Riktor ci sta, perché la cella ha un solo occupante. Se è legata alla cella e Riktor si sposta, colpisce chi vi entra (l'«area denial» del catalogo); il GDD dice «agli adiacenti». Manca un effetto di cella persistente con durata (REQ-06, ARC-14) |
| Necrotic Anchor | DA DECIDERE | −1 AP a un piano già validato, a metà risoluzione; il budget è una costante (`RTPlanValidationLibrary.h:132`); una trappola nascosta è un nuovo confine di privacy (REQ-05, ARC-05, ARC-07, AP-04) |
| Magnetic Spikes | CONTRACT CONFLICT | «+1 AP al costo di movimento» contro D-584 (2): gli AP pagano il profilo, i MP la distanza col costo per cella del terreno (D-117). Si traduce in +1 MP per cella. «Ancorato a terra (Z=0)» è uno stato nuovo; «fisici puri» è un tipo di danno che D-589 non ha (ARC-06, AP-05) |
| Chain Hook / Chain Pull | TRADUCIBILE + AMBIGUO | Riusa `Action.Pull` (1 cella, fase Control, portata 2) con portata minima/massima, trazione e danno. Restano tre domande: 2 celle fisse o fino all'adiacente (il GDD dice «adiacente»)? chi sceglie il ramo «tira Riktor verso un muro»? «sradicando» vuol dire azzerare la `CoverSelection`? Il pack Paragon ha la famiglia di clip `Ability_Hook` (REQ-17) |
| Voltage Leap | TRADUCIBILE con correzioni | `Action.Leap` + area raggio 1 + `Push` di 1 **cella**: il «micro-spicchio» urta D-583 (4), e «Body Block» va legato alla contesa di cella (`BlockedContested`, D-590) (REQ-03) |
| Riot Charge | TRADUCIBILE con correzioni | «Spazza via» è `LinearPass` (`Ivrin.PassingBlade`, attraversa e colpisce); «inarrestabile» viene rifiutato dal validatore del catalogo per una mobilità senza `Stop` (`RTCatalogLibrary.cpp:316-320`). L'«armatura temporanea da 150 HP» è l'effetto `Shield` che esiste già (ARC-11, REQ-15) |
| Corrosive Shackles | AMBIGUO + DA DECIDERE | «In un raggio di 2» con «il bersaglio» al singolare: colpo singolo a portata 2, oppure area di raggio 2 attorno al bersaglio o a Riktor? Il blocco delle cure è uno stato nuovo (REQ-09) |
| Seismic Smash | AMBIGUO | «Se il bersaglio viene sbalzato contro un muro»: l'abilità non sbalza. Serve dire chi sbalza, e se il +50% entra in `Mod` (D-589) (REQ-17) |
| Arc Discharge | CONTRACT CONFLICT | «Condivide il macro-esagono con un alleato» non può mai essere vero con un occupante per cella. La lettura istanziabile è «un alleato del bersaglio adiacente» (REQ-03, AP-12) |
| Skewer | AMBIGUO | `Status.Electrified` è **istantaneo** e un'unità non lo porta (`RTElectricPropagationTests.cpp:169`, `RTTurnManager.h:3449`), quindi «già elettrificati» non ha un soggetto, e la priorità del catalogo è per azione, non per bersaglio. «Ignora l'armatura» va detto rispetto a quale termine: D-589 non dice se un colpo diretto che porta un elemento paga la DEF, la RES dell'elemento o entrambe, e oggi nessuna azione porta un elemento (`FRTActionDef` non ha il campo) (REQ-08, REQ-10, ARC-08) |
| Iron Maiden | DA DECIDERE | Fase: Blast per il catalogo, Prep per il GDD e per un'icona di `skill.af` (cornice Prep, glifo Terra, tre tacche). Lo stato «gabbia» non esiste; «esagono di atterraggio» non ha un atterraggio; il Fango è OPEN (REQ-16, AP-13) |
| Magnetic Bastion | TRADUCIBILE + DA DECIDERE | Il motore è `Action.Intercept` (trigger `AllyHitByDirectAttack`, portata 2, oggi `Interposition`). Il +1 AP è un budget come stato. Lo spostamento «nell'esagono pianificato» non ha una cella scelta dal giocatore, ma esiste un precedente: `Reaction.HazardEscape` sposta su una cella derivata. Rispetto al GDD («alleato adiacente», «fino a 300 danni») il catalogo ha perso portata e tetto. «Guardia» collide con `Action.Guard` (REQ-14, ARC-10) |
| Shock Armor | DA DECIDERE | «In mischia» non esiste nel codice. È una proprietà del **trigger** (`ERTReactionTrigger`, `RTActionDef.h:278`, un valore nuovo in coda), non della condizione dichiarata di D-109, che resta la soglia di salute del bersaglio. I trigger sono due in una sola reazione (`HitByDirectAttack` e `AboutToBeDisplaced`), mentre oggi una reazione ne ha uno. L'anti-spinta esiste già come modulo `Reaction.Anchor` (ARC-09, REQ-07) |
| «Reaction Phase», «Blast Phase nemica» | CONTRACT CONFLICT | Le fasi sono cinque e la reazione è un segmento che scatta sulla causa (D-583 (1)) (REQ-18) |
| «Energetico», «Fisico», «Oscuro», «fisici puri», «puro», «garantiti» | AMBIGUO | D-589 conosce due termini: danno diretto → DEF, danno elementale → RES dell'elemento. Un danno «puro» va deciso, dicendo se salta anche copertura e Guard (REQ-08, ARC-12, AP-07) |
| Combo a due elementi | CONTRACT CONFLICT (proprietà) | Una coppia di elementi ha **una** voce, nel [catalogo delle coppie](../../balance/RT_ElementPairCatalog_v0.1.md) (D-275), e nessun eroe la possiede (invarianti 15 e 16). Earth+Electric è OPEN con il candidato GDD «Faglia ionizzata»; Electric+Decay è OPEN senza input. «Fango Magnetico» collide col «Fango» di Water+Earth. «In Prep» e «in Dash» non hanno un posto nella riga di una coppia (REQ-04, ARC-04, AP-11) |
| Combo a tre elementi | NON ADOTTATE | L'opzione scelta di D-591 (1) registra il GDD come superato nei sette punti di divergenza, e la combo a tre cariche è il punto (a); nessuna decisione ha adottato una regola a tre elementi (D-583 (8) è la lettura che lo annota). L'invariante 16 vieta `PairBonus` e `ComboAbility`, non una reazione sistemica. Il catalogo è posteriore: la domanda è se l'autore vuole aprire una regola a tre elementi |
| Affinità e Matchup | STALE | Phase → Muriel (D-586 (1)), Flux → Warp (D-588, D-593); «Legame Cristallino» non esiste in nessun documento (`git grep` → 0), e somiglia al «Consecrated Ground» di Muriel nel GDD («rende immuni a Pull e Knockback»); lo stordimento di Kallari non viene da nessuna abilità. Il GDD si contraddice: la scheda di Riktor mette Muriel fra i suoi «Forte contro» (riga 222), quella di Muriel mette Riktor fra i propri (riga 317); il catalogo segue la seconda (REQ-19, AP-14) |
| HP, MP, DEF, RES, spalle | MANCANTE | Il catalogo non li dichiara. Oggi: 1200 · 4 · 45 · 35 · 30 (`RTHeroCatalogLibrary.cpp:780-804`). I 1750 HP del GDD stanno fuori dalla fascia di D-589 (AP-08) |
| Equipaggiamento di default | MANCANTE | Il bootstrap monta su Riktor `Weapon.Impact` (portata −1, `Push` sull'attacco base), `Gadget.PortableCover` e `Reaction.Cleanse` (`RTCatalogLibrary.cpp:783`, `:808`, `:832`; `RTMatchBootstrapper.cpp:353`). In partita l'attacco base di Riktor ha quindi portata **2**, non 3 (critico, DR-4) |

## 3. Il catalogo, tradotto: proposta di specifica

⚠️ Tutto questo paragrafo è **PROPOSTA**, subordinata a DR-1…DR-8. Nessun numero è accettato: i valori nuovi sono marcati, e
quelli del GDD sono il solo input esistente per i cooldown.

### 3.1 I campi che ogni voce dichiara

Una voce è specificata quando riempie ogni campo; `N/A` si scrive. Un campo vuoto è un errore del parser del catalogo
(`tools/radar/catalog-code.ts`, il resto di [#4012](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4012)), non un default.

| Campo | Valori ammessi | Regola che lo legge |
|---|---|---|
| `Id` e nome | `Hero.Riktor.<Nome>` + nome visibile | owner unico (invariante 15) |
| Ruolo nel kit | Base · Fondamentale · Ultimate · Reazione · Generica | `MaxActionCount`; Ultimate +1 (D-594) |
| Fase | Prep · Dash · Blast · Move · Cleanup · segmento di reazione | D-583 (1); cornice dell'icona |
| `ActionPointCost` | intero; lo scostamento dal default di fase è motivato | D-594 (3); budget e somma dell'HUD |
| `Cooldown` | intero ≥ 0, anche 0 dichiarato | D-584 (4) |
| `Priority` | intero dentro la fase | ordine dentro la fase: D-584 (1), «ordinati per priorità»; il criterio a parità è DR-7 |
| Portata | minima–massima in celle; linea di vista o percorso libero | validatore, anteprima |
| Forma e area | `Single` · `Area` · `Line` · `Cone`; raggio o lunghezza; centro (bersaglio o sé) | `ERTAbilityShape` |
| Bersaglio | cella o unità; nemici · alleati · tutti · sé | fuoco amico, privacy dell'anteprima |
| Elemento | un `Element.<X>` (D-593) o nessuno | RES, catalogo delle coppie |
| Tipo di danno | `Direct` (DEF) · `Elemental(X)` (RES di X) · `True` solo se deciso | D-589 |
| Danno base | numero sulla scala ×10, per cella colpita | D-589 |
| Spostamento | famiglia (`Dash` · `Forced` · Transfer), celle, direzione, che cosa lo ferma | `AGENTS.md` §5, D-590 |
| Stati applicati | tag esistente o nuovo; inizio (stessa risoluzione o turno dopo); durata | snapshot, REQ-13 |
| Oggetti persistenti | aura, trappola o zona: legata alla cella o all'unità, durata, fase del tick, chi la vede | privacy (`AGENTS.md` §4) |
| Condizioni | predicato su stato canonico esistente, mai su spicchi o stati istantanei | D-583 (4), REQ-10 |
| Reazione | trigger, fase, condizione dichiarata | D-109, ADR-0004 |
| Fallback | `Stop` · `Cancel` · … | `ERTActionFallback` |
| Icona | cornice = fase, glifo = elemento; il costo **non** è disegnato | `progettazione-hud.md` §7.1 |
| Esempio | almeno un Given/When/Then con celle, HP e AP | specification by example |

### 3.2 Voce per voce: riuso, estensione, creazione

Ordine del contratto: `SEARCH → REUSE → EXTEND → REFACTOR → CREATE` ([`CLAUDE.md`](../../../CLAUDE.md) §4). Ogni enum nuovo
(`ERTActionEffect`, `ERTActionInvalidReason`, stati) si aggiunge **in coda**, perché finisce nel TurnLog serializzato.

| Voce | Riusa | Estende | Crea (con la sede) | Punto aperto |
|---|---|---|---|---|
| Shock Whip | attacco base `Single`, fase Attack; `Reaction.ElectricPropagatesThroughWater` | condizione su `Status.Wet` o su cella d'acqua | — | che cosa vuol dire «si incatena» oltre la propagazione dell'acqua |
| Static Sweep | forma `Cone` | cono di 3 celle frontali | — | 120 per cella o diviso |
| Taser Strike | `Status.Slow` | inizio «turno successivo», se è voluto diverso dallo Slow di oggi | eventuale stato distinto | stesso turno o turno dopo |
| Electrocute | `Action.Electrify` (fase Environment, `PropagationLimit`) | fase Prep, raggio 1, durata 2 turni | effetto di cella persistente con durata (snapshot e hash) | celle adiacenti; legata alla cella o all'unità; colpisce gli alleati? |
| Necrotic Anchor | trigger di attraversamento (`AGENTS.md` §5, Traversal) | effetto di cella sul trigger | modificatore del budget (DR-5); visibilità per squadra se nascosta | visibile o nascosta; quando vale il −1 |
| Magnetic Spikes | costi interi del pathfinding (MP) | +1 MP per cella nell'area; 50 danni a chi scatta o salta nell'area | stato «ancorato al Layer 0» (PROPOSTA `Status.Grounded`: vieta il salto e il cambio di `Layer`) | durata e area |
| Chain Hook | `Action.Pull`; `SelfReposition` per il ramo «verso un muro» | portata 2–4, trazione, danno 140 | due modi = due voci, oppure un campo di modalità | fino all'adiacente o 2 celle; chi sceglie il ramo |
| Voltage Leap | `Action.Leap`; area raggio 1; `Push` 1 | portata 2; effetti all'atterraggio nel Blast, come la carica | — | contesa all'atterraggio |
| Riot Charge | `LinearPass`; `Shield` | portata 3; Shield 150 su sé a fine corsa | — | «inarrestabile» = non si ferma sulle unità; muri e contesa restano `Stop` |
| Corrosive Shackles | `Action.CircularAoE` o colpo singolo; elemento Decay | — | `Status.HealBlocked` (tag, lettura in Heal e Shield, glossario, icona) | singolo o area |
| Seismic Smash | `Action.HeavyAttack` (mischia) | — | effetto d'urto contro il muro, che richiede una spinta dichiarata | chi sbalza; +50% dentro `Mod` |
| Arc Discharge | colpo `Single` | rimbalzo all'alleato **adiacente** del bersaglio | flag «garantito» se deciso (tipo di danno) | conferma della lettura «adiacente» |
| Skewer | forma `Cone`; `Status.Stunned` e reason `Stunned` | condizione su uno stato persistente, come modificatore in `Mod` | — | niente priorità per bersaglio |
| Iron Maiden | `Action.Root`; `HealBlocked` | — | `Status.Caged` (movimento, LOS, cure, scudi): tocca il servizio LOS, che è separato (`CLAUDE.md` §11) | fase, durata, effetto sul Fango |
| Magnetic Bastion | `Action.Intercept` | — | modificatore del budget (DR-5); spostamento solo con una `D-nnn`, su cella derivata come `Reaction.HazardEscape` | portata e tetto del GDD |
| Shock Armor | `Action.Counter` (`HitByDirectAttack`); `Reaction.Anchor` (`CancelDisplacement`) | trigger nuovo «colpito da un'azione a portata 1» in coda a `ERTReactionTrigger` | una reazione a due trigger (oggi uno per reazione), oppure due voci separate | resta nel kit? (DR-3) |
| Combo a 2 | righe OPEN del catalogo delle coppie | candidato sulla riga | — | mai sulla scheda dell'eroe |
| Combo a 3 | — | — | — | non adottate: il GDD su quel punto è superato (D-591 (1)) |

### 3.3 Esempi che distinguono le letture

Ogni esempio ha due esiti; il primo che l'autore sceglie diventa il criterio d'accettazione della voce.

- **Corrosive Shackles.** Riktor in `(0,0,0)`; nemico A a 1 da Riktor; nemico B a 1 da A e a 2 da Riktor; alleato C a 2 da A
  e a 3 da Riktor; Shackles su A. *Colpo singolo*: A −200 e cure bloccate, B e C intatti. *Area raggio 2 attorno ad A*: A, B e
  C −200 (C solo se c'è fuoco amico). *Area raggio 2 attorno a Riktor*: A e B −200, C intatto.
- **Electrocute.** Nemico E adiacente a Riktor per due turni. *Solo la cella di Riktor*: E non subisce nulla in nessun turno.
  *Le sei adiacenti, come il GDD*: E −80 per turno, prima della RES Electric. Se Riktor si muove, l'aura resta o lo segue?
- **Static Sweep.** Tre nemici nelle tre celle frontali, DEF e RES a 0. *Diviso*: −40 ciascuno. *Per cella*: −120 ciascuno,
  360 in tutto. Con un solo nemico nell'arco, la lettura «per cella» dà 120: meno di Shock Whip sul singolo, coerente con
  «ottimo per formazioni compatte».
- **Taser Strike.** Nemico con 4 MP che pianifica un Move di 4. *Con la tempistica dello Slow di oggi*: arriva a 3 in questo
  turno, a 4 nel prossimo. *Turno successivo*: arriva a 4 ora, e nel prossimo turno la cella a 4 non compare fra quelle
  raggiungibili nell'anteprima del percorso. Non c'è un rifiuto da mostrare: `InsufficientMovementPoints` non ha produttore, e
  per D-190 il ramo dei Movement Point è uscito da `ValidatePlan`.
- **Necrotic Anchor.** Nemico con piano da 6 AP (Dash che attraversa la cella, Blast, Move), validato al Commit. *Dal turno
  dopo*: il turno risolve intero, il prossimo ha budget 5. *Subito, cade l'ultima azione*: il Move è annullato con un reason
  code nuovo. *Subito, solo AP non spesi*: effetto nullo su chi spende tutto.
- **Chain Hook.** Bersaglio in linea a 4, nessun ostacolo. *2 celle fisse*: arriva a 2, **non** adiacente. *Fino
  all'adiacente*: arriva a 1. Con il bersaglio a 2, la lettura «2 fisse» lo porta sulla cella di Riktor, che è occupata.

### 3.4 Economia degli AP di Riktor

Budget 6, costi del catalogo (reazioni a 1), Move 2, Guard, Brace e Overwatch a 1 (sono `Preparation` con slot `Main`:
`RTCatalogLibrary.cpp:1289`, `:1383`, `:1699`; è il «Prep economico» del testo scelto di D-594 (1)).

| Turno | Composizione | AP |
|---|---|---|
| Avanza e colpisci | Move + Shock Whip + Electrocute + Shock Armor | 2 + 2 + 1 + 1 = 6 |
| Aggancio da fermo | Chain Hook + Shock Whip + Electrocute + Magnetic Bastion | 2 + 2 + 1 + 1 = 6 |
| Raffica | Shock Whip ×3 (480 di base, nessuna reazione) | 6 |
| Ultimate da fermo | Skewer + Shock Whip + Electrocute (540 di base, Stun, cono) | 3 + 2 + 1 = 6 |
| Ultimate in avvicinamento | Move + Skewer + Electrocute | 2 + 3 + 1 = 6 |
| Muro | Move + Magnetic Bastion + Shock Armor + Electrocute (+1 libero) | 5 |
| Carica | Riot Charge + Corrosive Shackles + Shock Whip | 6 |
| Dopo un Bastion scattato, se il +1 resta | Skewer + Shock Whip + Move | 7 |

Che cosa domina, con i numeri del catalogo:

- **La ripetizione è legale.** Il testo scelto di D-584 (1) dice «Solo il budget conta», e il cooldown (D-584 (4)) oggi non
  ferma una seconda copia nello stesso turno: `ValidatePlan` legge `CooldownRemaining`, cioè lo stato **prima** del turno
  (`RTPlanValidationLibrary.h:38`, `.cpp:194-200`). Senza slot, quindi, Skewer due volte (6 AP) fa 760 di base e due Stun.
  Oggi la ripetizione la ferma solo lo slot, che ADR-0011 destina a cadere. Vietarla è un **emendamento** di D-584 (1), non
  un vuoto da colmare (AP-01, DR-6).
- **I cooldown mancano.** Il catalogo dichiara solo «Nessun Cooldown» sull'attacco base. Allo stesso costo la primaria di Blast
  rende 90–110 per AP contro gli 80 di Shock Whip: l'attacco base vale come riempitivo solo nei turni in cui la primaria è in
  ricarica. L'unico input sono i cooldown del GDD: Electrocute 1, Chain Pull 2, Corrosive Shackles 2, Skewer 3, Iron Maiden 3,
  Shock Armor 1, Magnetic Bastion 2 (AP-02).
- **Il camminare.** Il catalogo non dà una portata a Shock Whip (REQ-12). Il GDD dà «raggio 1–2 hex», e con `Weapon.Impact`
  (portata −1), se resta, diventerebbe 1: con 4 MP il raggio d'ingaggio scenderebbe a 5 celle. Oggi `ImpactShot` ingaggia a
  3, cioè a 2 in partita con `Weapon.Impact`. Ogni turno d'avvicinamento spende comunque un terzo del budget in Move, salvo
  un Dash che lo sostituisca. Il confronto fra Dash e Move va rifatto quando portata e DR-4 sono fissati.
- **Le reazioni a 1 AP.** Armarle entrambe costa quanto un Move. Il costo giusto dipende da DR-3 (quante ne restano) e da DR-5
  (se il +1 di Bastion esiste), e si misura poi con `tools/balance/ap_forms_sim.py`.

### 3.5 Le combo

Le combo **non** stanno nella scheda di Riktor. Una coppia di elementi produce un effetto per chiunque la crei, e la sua voce
sta nel [catalogo delle coppie](../../balance/RT_ElementPairCatalog_v0.1.md), che ne è l'owner (D-275). Le due combo a due
elementi si propongono come **candidati** sulle righe OPEN: Earth+Electric, dove l'autore sceglie fra «Fango Magnetico» (MP
dimezzati) e il candidato GDD «Faglia ionizzata» (attira i proiettili), ed Electric+Decay («Catene Spettrali», reazioni
disattivate). Il nome «Fango Magnetico» va cambiato, perché «Fango» è il candidato di Water+Earth. Una coppia oggi non ha
un effetto per fase. Water+Electric risolve nel Cleanup, e «in Prep» o «in Dash» chiederebbe una regola nuova del catalogo.
Le due combo a tre elementi non hanno una regola che le accolga: nessuna decisione ne ha adottata una, e su quel punto
l'opzione scelta di D-591 (1) registra il GDD come superato. Se l'autore le vuole, serve una decisione nuova sul catalogo
delle coppie, non una riga nella scheda dell'eroe.

## 4. L'HUD sugli Action Point: proposta di specifica

Owner: [`progettazione-hud.md`](../../technical/systems/progettazione-hud.md). Oggi non cita gli AP, e il codice HUD non ne
legge nessuno (`git grep -i -E "ActionPoint|OverBudget"` in `Source/RefactorTactics/UI` e `Source/RefactorTactics/Player` → 0).
Tutte le prove qui sotto sono `NOT RUN`: sono criteri, non esiti.

### 4.1 Che cosa si scrive subito

Questi requisiti reggono con il budget costante o per unità, e con gli slot o con la lista.

| Id | Requisito | Criterio d'accettazione | Prova |
|---|---|---|---|
| R1 | Il view-model dell'unità comandata porta `Budget`, `Committed`, `Preview` e `Residual`. `Committed` è la somma che `ValidatePlan` calcola; il widget non somma mai | Con piano vuoto: 6 · 0 · 0 · 6. Con Move + attacco base: `Committed` 4, `Residual` 2 | Automation su `URTHudViewModel`, con casi scritti a mano e non ricavati dal validatore |
| R2 | Il costo dello slot è un **layer a runtime** (§7.1, layer 4) letto da `FRTActionDef::ActionPointCost`, mai disegnato nel PNG | Per ogni voce di Riktor e per le generiche, le tacche del badge sono uguali al costo; `Wait` (0) non ha badge; sopra 3 si usa un numero | Automation sul catalogo |
| R3 | Due canali per il rosso. **Rifiuto del gesto**: lo slot armato diventa `Invalid(OverBudget)` quando `ValidatePlan(piano + armata)` rifiuta, e gli slot già pianificati non cambiano. **Colpevole del piano**: solo per un piano già impegnato che diventa illegale | Con 4 impegnati, armare un'azione da 3 accende lo slot armato e nient'altro; controllo positivo con un'azione da 2 | Automation |
| R4 | Il Move ha un'anteprima di costo all'hover sul percorso: −2 AP e residuo, dal validatore. La variazione per cella resta sui MP (D-117) | All'hover su una cella raggiungibile i pip mostrano −2 senza click | Automation sulla vista del percorso; PIE per l'aggiornamento all'hover |
| R5 | §37 riceve una famiglia «Action Point pips»: segmenti discreti con quattro stati (impegnato, anteprima, libero, sforato in rosso fuori cornice). Il numero di segmenti viene da `Budget` | Gli stati si enumerano e si provano uno per uno | Automation |
| R6 | **Privacy.** Le viste di un'unità non comandata (la barra dell'osservato di D-562, il roster avversario) non hanno **nessun** campo AP, in nessuna fase. La somma spesa rivelerebbe il piano, e un residuo dedotto dagli eventi sarebbe falso, perché le reazioni armate e non scattate hanno già speso. Gli AP degli **alleati** stanno nella Team intent card, da `FRTIntentView` filtrata per squadra | Per un'avversaria i campi sono vuoti in Planning e in Risoluzione; controllo positivo sull'unità comandata | Automation (sull'assenza del campo, non sul widget che lo nasconde) |
| R7 | **Grammatica delle icone.** La cornice dice la fase, il glifo l'elemento, il costo è R2. Il rango Ultimate ha un segno proprio, perché tre tacche non vogliono dire Ultimate. Manca il glifo di `Element.Decay`. «Guard» non nomina tre cose diverse nella stessa dock | Ogni icona del kit dichiara cornice e glifo coerenti con fase ed elemento della riga | Automation sul catalogo + revisione visiva |

### 4.2 Che cosa aspetta E38 e le decisioni

| Id | Requisito | Dipende da |
|---|---|---|
| R8 | La Ghost Timeline (§8) diventa una pila **verticale** per fase, con l'indice di risoluzione dato dallo stesso comparatore del resolver. Le fasi restano l'asse orizzontale, per non ricadere nella «coda arbitraria» che §6.6 vieta. La colonna Cleanup compare quando il piano ha un'azione di quella classe | DR-7, resolver a lista (E38) |
| R9 | Le reazioni sono N diramazioni (§11), ciascuna con nome, trigger, condizione e costo. Due reazioni sullo stesso micro-step stanno in **un** prompt, una riga ciascuna, con HOLD per ultimo | D-592 implementata |
| R10 | `Planned` diventa un contatore se la ripetizione è ammessa, e il rifiuto nomina l'occorrenza, non solo l'`ActionId` (oggi `bPlanned` è un booleano, `RTHudViewModel.h:661`) | DR-6 |
| R11 | Un modificatore di budget si vede come pip col tratto «modificato» e con la causa nel tooltip | DR-5 |
| R12 | La dock mostra K voci del kit, con K letto dalla stessa costante del validatore e nessun secondo numero | DR-2, DR-3 |

### 4.3 La grammatica di `skill.af` e §7.1

La grammatica dell'autore funziona: fase e costo si leggono di periferia, senza leggere testo. Il difetto è **dove** sta il
costo. Disegnato nel PNG è un numero statico (§7.1, riga 729) e una seconda sede del costo accanto a
`FRTActionDef::ActionPointCost`, e diventerebbe falso alla prima taratura di E38 senza che nessun test lo veda. La consegna
proposta: cornice e glifo **senza** badge, più uno sprite del badge a una, due e tre tacche, che il widget sceglie dal dato.
⚠️ Che le tacche siano il costo è una lettura: se indicano il rango o la potenza, il conflitto con §7.1 cade, ma il rango
ha bisogno di una definizione propria. Move ha già un'icona di fase nel gioco (`RT_UI_Icon_Phase_Move`); manca solo in
`skill.af`. [#4018](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4018) chiede la quinta forma, per la reazione,
e l'esagono viola dell'autore ne è il candidato. Il Cleanup non ha una cornice né una issue.

## 5. Il panel

| Lente | Esperti | Rilievi | Verifica avversaria |
|---|---|---|---|
| REQ — requisiti ed esempi | Karl Wiegers, Gojko Adzic | REQ-01…REQ-20 | un agente indipendente |
| ARC — contratto e modi di guasto | Martin Fowler, Michael Nygard | ARC-01…ARC-16 | un agente indipendente |
| ECO — economia AP e identità del kit | Alistair Cockburn, designer di sistemi tattici (lente di dominio) | AP-01…AP-14 | un agente indipendente |
| HUD — l'HUD sugli AP | Lisa Crispin, UX designer di tattici a turni (lente di dominio) | HUD-01…HUD-17 | un agente indipendente |
| Critico di completezza | — | lacune, contraddizioni, decisioni | — |

Esiti del passaggio: nove agenti, nessuno caduto. Il verificatore ha confermato o corretto in parte ogni rilievo; nessuno è stato
refutato per intero. Le correzioni sono incorporate in §2–§4.

### 5.1 I rilievi

Severità dopo la verifica. **C** = confermato, **P** = confermato in parte e corretto.

| Id | Sev. | Soggetto | Rilievo | Ver. |
|---|---|---|---|---|
| REQ-01 | BLOCKER | Opzioni | Tre letture delle «Opzioni», ognuna urta una regola diversa (la quarta, le varianti, l'ha aggiunta il verificatore) | P |
| REQ-02 | MAJOR | «Costante» | Il costo per gruppo è compatibile con D-594 solo se intero; «1–2 AP» non è un valore | P |
| REQ-03 | BLOCKER | Shock Whip · Voltage Leap · Arc Discharge | Spicchio, micro-spicchio e macro-esagono condiviso contro D-583 (4) | C |
| REQ-04 | BLOCKER | Combo | Combo a tre non adottate (GDD superato, D-591 (1)); combo a due di proprietà del catalogo coppie | P |
| REQ-05 | BLOCKER | Necrotic Anchor · Magnetic Spikes · Magnetic Bastion | AP come bersaglio di effetti con un budget costante | P |
| REQ-06 | MAJOR | Electrocute | Aura su una cella dove nessun nemico può stare | C |
| REQ-07 | MAJOR | Glossario | «In mischia» regge tre voci e nel codice non esiste | C |
| REQ-08 | MAJOR | Tipi di danno | Sei aggettivi contro i due termini di D-589 | C |
| REQ-09 | MAJOR | Corrosive Shackles · Static Sweep | «Raggio di 2» al singolare; «120 distribuito» | C |
| REQ-10 | MAJOR | Skewer | «Già elettrificati»: `Status.Electrified` non è portato | C |
| REQ-11 | MAJOR | Shock Whip | Fango e Vapore non esistono a runtime; il Vapore toccherebbe una coppia CURRENT | P |
| REQ-12 | MAJOR | Attributi | Fase dell'attacco base, portate, forme, cooldown e durate mancanti | C |
| REQ-13 | MAJOR | Taser Strike · Skewer | Stesso turno o turno dopo | C |
| REQ-14 | MAJOR | Magnetic Bastion | Intercept con spostamento; «Guardia» collide con `Action.Guard` | C |
| REQ-15 | MAJOR | Riot Charge | «Inarrestabile», «spazza via», «armatura» senza regola | C |
| REQ-16 | MAJOR | Iron Maiden | Fase incerta, «atterraggio» senza atterraggio, gabbia inesistente | C |
| REQ-17 | MAJOR | Seismic Smash · Chain Hook | Condizioni su cose che l'abilità non produce; un OR senza regola | C |
| REQ-18 | MAJOR | Shock Armor · «Reaction Phase» | Né «Reaction Phase» né «Blast Phase nemica» esistono | C |
| REQ-19 | MINOR | Affinità e Matchup | Effetti promessi che il kit non ha, nomi superati | C |
| REQ-20 | MINOR | Scala | Il catalogo non dice se i numeri sono già ×10 | C |
| ARC-01 | BLOCKER | Opzioni · tetto | Loadout contro D-598; anche le sole «Originali» sforano 6 | P |
| ARC-02 | BLOCKER | Kit nuovo | Non supera una decisione (D-586 (5) è una lettura), ma va accettato con una `D-nnn` | P |
| ARC-03 | BLOCKER | Posizione intra-cella | Un solo slot di occupancy per `FRTCellId` | C |
| ARC-04 | BLOCKER | Combo | Combo a tre non adottate (D-591 (1)); combo a due senza owner eroe (invarianti 15 e 16); rinominare «Fango Magnetico» | P |
| ARC-05 | BLOCKER | Budget come stato | Il verdetto dipende dal solo piano; +1/−1 AP chiede uno stato nuovo in snapshot, hash e replay | P |
| ARC-06 | MAJOR | Magnetic Spikes | Confonde le due valute di D-584 (2): si traduce in +1 MP per cella | P |
| ARC-07 | MAJOR | Trappole | Non esiste un effetto di cella nascosto: nuovo confine di privacy | C |
| ARC-08 | MAJOR | Skewer | La priorità è per azione; lo stato è istantaneo | C |
| ARC-09 | MAJOR | Shock Armor | Due trigger in una reazione; «in mischia» manca (è un trigger, non una condizione di D-109: §5.5) | P |
| ARC-10 | MAJOR | Magnetic Bastion | Intercept non sposta; precedente di cella derivata in `Reaction.HazardEscape` | P |
| ARC-11 | MAJOR | Riot Charge | Il validatore rifiuta una mobilità senza `Stop`; `LinearPass` esiste | C |
| ARC-12 | MAJOR | Tipi di danno | Sei vocaboli contro due | C |
| ARC-13 | MAJOR | Taser · Skewer | Due tempi per lo stesso stato | C |
| ARC-14 | MAJOR | Electrocute · Shock Whip · Iron Maiden | Aura su un solo occupante; Fango e Vapore non sono stati | C |
| ARC-15 | MAJOR | Reazioni a 1 AP | Legali per riga, ma il motore regge una reazione sola | C |
| ARC-16 | MINOR | Icone | Il badge del costo è una seconda sede, se è il costo | P |
| AP-01 | BLOCKER | Ripetizione | Quante volte la stessa abilità entra in un piano (la regola c'è: D-584 (1) la ammette, §5.5) | C |
| AP-02 | MAJOR | Cooldown | Senza cooldown la primaria di Blast domina l'attacco base | P |
| AP-03 | MAJOR | Move contro Dash | Il Dash sostituisce il Move solo con bersaglio a portata e destinazione allineata | P |
| AP-04 | BLOCKER | Modificatori di budget | Né modello né regola di risoluzione | P |
| AP-05 | MAJOR | Magnetic Spikes | Costo sul terreno in AP invece che in MP | C |
| AP-06 | MINOR | Reazioni a 1 AP | Armarle entrambe smette di essere una scelta: taratura, non ambiguità | P |
| AP-07 | MAJOR | Tipi di danno | Non si mappano su D-589 | C |
| AP-08 | MAJOR | Statistiche | HP, MP, DEF, RES e spalle non dichiarati | P |
| AP-09 | BLOCKER | Identità | Da Guardian/Field Architect a Disruptor: si perdono strutture e tiro indiretto. È una decisione da prendere, non un conflitto | P |
| AP-10 | BLOCKER | Opzioni | Loadout vietato o tetto sfondato | C |
| AP-11 | MAJOR | Combo | Il qualificatore di fase le rende combo di Riktor | C |
| AP-12 | MAJOR | Condizioni impossibili | Le opzioni che le portano valgono meno di quanto dicono | C |
| AP-13 | MAJOR | Iron Maiden | Fase divergente fra catalogo, GDD e icona | P |
| AP-14 | MINOR | Taser · Matchup | Turno dopo contro lo Slow di oggi; il GDD si contraddice fra le schede di Riktor e Muriel | P |
| HUD-01 | BLOCKER | Capienza della dock | Il catalogo non entra nel tetto di sei voci | P |
| HUD-02 | MAJOR | Slot `Invalid` | Il colpevole di `OverBudget` è scelto dall'ordine canonico, non dal gesto | C |
| HUD-03 | MAJOR | Ordine dentro la fase | Nessuna voce lo fissa; è un prerequisito di E38 | P |
| HUD-04 | MAJOR | Ghost Timeline | Quattro caselle da una voce contro fino a sei azioni in una fase | C |
| HUD-05 | MAJOR | Anteprima del Move | Il costo del Move non ha un punto di anteprima | P |
| HUD-06 | MAJOR | Necrotic Anchor | Una perdita di AP a metà risoluzione non ha regola | P |
| HUD-07 | MAJOR | Modificatori di budget | +1/−1 chiedono un budget per unità | C |
| HUD-08 | MAJOR | Privacy | Nessuna misura AP di un avversario, in nessuna fase | C |
| HUD-09 | MAJOR | AP alleati | La Team intent card non li prevede | C |
| HUD-10 | MAJOR | Reazioni | §11 e la vista hanno una reazione sola | C |
| HUD-11 | MAJOR | Badge del costo | Numero statico nel PNG, se è il costo | P |
| HUD-12 | MAJOR | Cornice = fase | Iron Maiden ha fasi diverse nelle fonti; Move ha già un'icona nel gioco | P |
| HUD-13 | MINOR | Rango Ultimate | Tre tacche non vogliono dire Ultimate | C |
| HUD-14 | MINOR | Glifi | Manca Decay; Iron Maiden dichiara due elementi | C |
| HUD-15 | MAJOR | Slot `Planned` | Booleano per slot contro azioni ripetute e `Wait` a costo 0 | P |
| HUD-16 | MINOR | Etichette | «Guard» indica tre cose nella stessa dock | C |
| HUD-17 | MINOR | Resource bar | Gli AP sono interi discreti, non una barra continua | C |

### 5.2 Le lacune del critico

- **Equipaggiamento di default** (BLOCKER): `Weapon.Impact`, `Gadget.PortableCover` e `Reaction.Cleanse` restano su Riktor
  anche con il kit nuovo, se nessuno decide altrimenti. Accorciano di uno la portata dell'attacco base e gli aggiungono una
  spinta, e cambiano la dock e il numero di reazioni (DR-4).
- **Ordine nel TurnLog** (BLOCKER per E38): `EntryLess` ordina le voci per chiave canonica (`RTTurnLogLibrary.cpp:14`), e
  `MicroStepIndex` è un gruppo simultaneo, `INDEX_NONE` per il Blast (`RTTurnLog.h:1308-1337`). Nessuna coordinata registra
  l'ordine in cui il resolver applica due azioni della stessa unità nella stessa fase, e il piano a lista ne ha bisogno (DR-7).
- **Il bot** (MAJOR): `FRTHexBotPlan` porta una cella di destinazione e un attacco (`RTHexBotLibrary.h:15`). Non esprime una
  lista, più reazioni né un budget.
- **Golden, scenari e id** (MAJOR): il TurnLog scrive l'id proprio della reazione (`Hero.Branth.Interposition`). Riscrivere il
  kit prima del rename di #4011 vuol dire migrare due volte gli stessi golden.
- **Clip e VFX** (MAJOR): nel pack Paragon di Riktor ci sono le famiglie `Ability_Hook` (con `Chain_Pull`), `Ability_Lockdown`,
  `Ability_ShockingPunch`, `Ability_Ultimate`, `PrimaryAttack`, `Jump`, `LaunchPad`, `Sprint`, `KnockBack` e `Stun`. Non c'è un
  fendente ad arco né una carica dedicata. Ogni azione nuova passa dal gate di cook delle clip
  (`RequiredActionClipsAreCooked`), e ognuna chiede i propri effetti visivi e il proprio suono.
- **Input** (MAJOR): il gate `PlayerInput.EveryKitEntryIsReachable` misura che ogni voce abbia un tasto, non i gesti di una
  lista (più reazioni, più movimenti, la ripetizione). [#1408](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1408) (E48) è aperta.
- **Nomi delle abilità** (MINOR): D-585 tratta come nomi di sviluppo i nomi degli **eroi**. Le voci marcate «(Originale)»
  (Shock Whip, Electrocute, Chain Hook, Corrosive Shackles, Skewer) seguono la stessa regola: vanno sostituite da nomi
  originali prima di pubblicare (LETTURA, da confermare).

### 5.3 Correzioni del panel al brief di questa sessione

Il brief consegnato agli esperti conteneva tre letture presentate come regole, e i verificatori le hanno corrette. Le
correzioni valgono anche per quello che la sessione aveva scritto all'autore prima del panel.

1. «Branth → Riktor, stesso kit» sta in D-586 **(5)**, fra le letture dell'assistente, ed è ribaltabile. Un kit nuovo non
   supera una decisione, ma esce dal perimetro che il testo scelto di D-585 (1) dichiara («kit di quattro eroi», e Riktor non
   è fra questi): ha bisogno di una `D-nnn` d'accettazione (D-591 (3)) e di una correzione del piano del rename.
2. Le combo a tre elementi non sono «escluse da una decisione»: l'opzione scelta di D-591 (1) registra il GDD come superato
   nei sette punti di divergenza, fra cui quello delle combo a tre, e nessuna decisione ne ha adottata una. D-583 (8) è la
   lettura che lo annota.
3. «Ultimate +1 (cioè 3)» vale per una Ultimate di Blast. Per una Ultimate di Prep il testo scelto di D-594 (3) dà 2, e la
   lettura D-594 (4) dà 3.

Una quarta incertezza, sollevata dal verificatore della lente HUD, si chiude col codice: Guard, Brace e Overwatch costano
1 AP. Risolvono in `Preparation` con slot `Main`, e il testo scelto di D-594 (1) parla di «Prep economico».

### 5.4 Voci del panel

- **Karl Wiegers**: «un requisito che ammette tre letture non è un requisito, è tre requisiti diversi travestiti da uno; e
  qui nessuna delle tre passa i vincoli già scritti.»
- **Gojko Adzic**: «basta un esempio — "il giocatore apre la selezione eroe: vede tre Shock Whip o una?" — e la discussione
  finisce in un minuto.»
- **Martin Fowler**: «"spazza via" è LinearPass, e "inarrestabile" va ristretto a "non si ferma sulle unità". Questo è un
  riuso, non una creazione.»
- **Michael Nygard**: «oggi un piano validato resta valido fino al Cleanup, ed è una proprietà preziosa. Un −1 AP durante il
  Dash produce un piano che era legale al commit e smette di esserlo a metà volo.»
- **Alistair Cockburn**: «il giocatore di Riktor oggi ha l'obiettivo "costruisco il campo e proteggo il carry", il catalogo gli
  dà "nego lo spazio e punisco chi entra". Sono due attori diversi.»
- **Designer di sistemi tattici**: «senza slot, Skewer due volte fa 760 danni con doppio Stun, e il cooldown non lo impedisce
  perché si applica solo dopo il turno.»
- **Lisa Crispin**: «il caso limite da scrivere per primo: il giocatore arma X e vede diventare rosso Y.»
- **UX designer di tattici a turni**: «la grammatica dell'autore è buona, perché il costo si legge di periferia senza leggere
  testo. Il difetto è dove sta, cioè cotto nell'immagine.»

### 5.5 La revisione avversaria del referto

Dopo la sintesi, un revisore a contesto fresco ha letto il referto contro il codice e il Decision Log. Ha confermato le
citazioni `file:riga` di §2–§5 e l'aritmetica di §3.3, §3.4 e §4.1, e ha trovato difetti che le lenti e i loro verificatori
non avevano visto. Sono corretti nel testo; qui restano i principali, perché cambiano due domande.

- **La ripetizione è già decisa** (BLOCKER): l'opzione scelta di D-584 (1) dice «Solo il budget conta». AP-01 e la prima
  stesura di DR-6 la presentavano come un vuoto; è invece lo stato in vigore, e vietarla emenda D-584 (1).
- **La struttura dell'eroe** (BLOCKER): il catalogo eroi vuole «attacco base · quattro abilità fondamentali», più al più una
  generica core. Togliere una reazione, come proponeva la prima stesura di DR-3, soddisfa il validatore ma non la struttura.
- **Le varianti** sostituiscono solo gli effetti, e il validatore chiede **esattamente una** fondamentale con varianti: anche
  la lettura «alternative scelte ora» tocca quella regola.
- **«In mischia» è un trigger**, non una condizione dichiarata: D-109 regola la condizione (la soglia di salute), e un trigger
  nuovo sta in `ERTReactionTrigger`.
- **`InsufficientMovementPoints` non ha produttore** e per D-190 non ne avrà: un effetto sui Movement Point si vede
  nell'anteprima del percorso, non come rifiuto.
- **Le conseguenze di ADR-0011 sono letture ribaltabili**, e la conseguenza 6 chiede asserti sull'ordine e sul budget, non
  sulla ripetizione.
- **L'equipaggiamento di default resta** finché DR-4 non decide. `Weapon.Impact` toglie portata e non danno.

## 6. Decisioni d'autore richieste

Le formulano il critico e la sintesi. Le premesse delle opzioni sono state rimisurate nella sessione principale. ➕ **Esiti in §9–§11**:
DR-1, DR-2 e DR-4…DR-8 sono decise e registrate in D-612 e D-613; DR-3 è rinviata dall'autore (`KIT-1`). La tabella qui sotto è lo stato
prima delle risposte.

| Id | Domanda | Opzioni | Raccomandazione | Blocca |
|---|---|---|---|---|
| DR-1 | Il catalogo del 2026-11-13 sostituisce il kit di oggi di Riktor? | sì, accettato con una `D-nnn` dopo il rename #4011 · sì, subito su `Hero.Branth` · no, resta input · ibrido (Bastion e Riot Charge al posto di Interposition e Ram, il resto resta) | sì, dopo #4011: una sola migrazione di golden e id. La `D-nnn` allarga il perimetro di D-585 (1), che nominava i kit di quattro eroi | scheda, catalogo, dock, clip |
| DR-2 | Che cosa sono le «Opzione 1/2/3»? | alternative fra cui l'autore sceglie ora · varianti di loadout su `FRTAbilityVariant`, da estendere (oggi sostituiscono solo gli effetti) e con la stessa tensione verso D-598 del loadout · tutte nel kit (tetto, struttura e input) · loadout pre-partita (supera D-598) | alternative scelte ora: non tocca D-598 e dà alla dock una cardinalità nota. Il validatore vuole comunque una fondamentale con varianti: si ricava da un'opzione scartata esprimibile con soli effetti, o si emenda la regola «esattamente una» (`RTHeroCatalogLibrary.cpp:355-358`) | dock, timeline, bot, numeri |
| DR-3 | La struttura vuole attacco base + quattro fondamentali (+ al più una generica core); con un'opzione per gruppo Riktor ne porta sei. Come entra? | struttura emendata a base + cinque (una voce per fase, l'Ultimate e una reazione): il tetto di sei regge · struttura invariata: escono due voci d'eroe · struttura e tetto a base + sei, con la rimisura dell'input in E48 | base + cinque con una reazione: segue la forma del catalogo e il tetto regge. Fra le due reazioni, Bastion senza +1 AP né spostamento è l'`Interposition` di oggi; con quelli chiede DR-5 e una `D-nnn` sullo spostamento. Shock Armor chiede un trigger nuovo e due trigger in una reazione, o due voci | catalogo eroi, `ValidateHeroes`, dock |
| DR-4 | L'equipaggiamento di default di Riktor resta col kit nuovo? | resta (`Weapon.Impact` aggiunge la spinta a Shock Whip) · si toglie · si ridisegna col kit | si ridisegna: `Reaction.Cleanse` fu scelto per non duplicare l'Intercept del kit e come risposta allo Slow di `ImpactShot` (#1403, D-218, #1479; `RTCatalogLibrary.cpp:824-832`). Col kit nuovo la prima ragione resta (Bastion è un Intercept), la seconda cade se non resta uno Slow | attacco base, reazioni, dock |
| DR-5 | Un effetto può cambiare il budget AP di un'unità? | no: Necrotic Anchor e Bastion si traducono su assi esistenti · sì, dal turno successivo (budget nello snapshot, nell'hash e nel replay, con un tetto) · sì, anche nel turno in corso (regola su quale azione cade, evento nel TurnLog) | no; se l'autore vuole la meccanica, «dal turno successivo» è la meno invasiva, ma cambia il «sfasando la pianificazione» del catalogo. La domanda vale per tutto il roster | Necrotic Anchor, Bastion, pip dell'HUD |
| DR-6 | Lo stato deciso (D-584 (1), «Solo il budget conta») ammette la stessa azione più volte nello stesso piano, e il cooldown oggi non ferma la seconda copia. Va emendato? | no: ripetizione libera fino al budget (lo stato deciso) · emendare: le azioni con cooldown > 0 al più una volta per turno, quelle a cooldown 0 ripetibili · emendare: ogni azione d'eroe al più una volta, ripetibili solo Move e Wait | la seconda: senza, Skewer due volte (760 e due Stun) domina il kit; tiene la raffica di base come scelta. È un emendamento di un testo scelto, e va detto così | `ValidatePlan`, slot `Planned`, bot |
| DR-7 | Dentro una fase, a parità di priorità, come si ordinano le azioni della stessa unità? | ordine di dichiarazione, in sequenza · id stabile, in sequenza · gruppo simultaneo sullo stesso stato | ordine di dichiarazione: il giocatore lo controlla, ed entra nel piano registrato | resolver a lista, TurnLog, timeline |
| DR-8 | Quanto costa un'Ultimate fuori dal Blast, e in che fase sta Iron Maiden? | sempre 3, Iron Maiden in Prep · classe +1 alla lettera (Prep = 2) · Iron Maiden in Blast a 3 | sempre 3, in Prep: lo dice il GDD (riga 231), e l'icona Prep + Terra con tre tacche lo conferma se le tacche sono il costo. Il 3 del catalogo è l'intestazione del gruppo Ultimate di Blast, dove il catalogo mette Iron Maiden | costo e cornice, Cost Preview |

Domande di dettaglio, che servono all'accettazione del kit ma non bloccano la forma dell'HUD: Static Sweep (per cella o
diviso); Corrosive Shackles (singolo o area); Electrocute (adiacenti, legata alla cella o all'unità, alleati); Taser e Skewer
(stesso turno o turno dopo); Chain Hook (fino all'adiacente o 2 celle; chi sceglie il ramo del muro); Skewer (quale stato
sostituisce «già elettrificati», quale termine ignora); Iron Maiden (che cosa blocca la gabbia, durata); i tipi di danno; la
voce Earth+Electric; la riapertura delle combo a tre; la visibilità delle trappole; il significato delle tacche nelle icone.

## 7. Provenienza, limiti e stati di verifica

- **Base misurata**: `origin/main` = `9560053a2` (merge della PR #4104), nel worktree `.claude/worktrees/riktor-hud-ap-panel`.
- **Fonti normative lette**: Decision Log (righe `D-583`, `D-584`, `D-585`, `D-586`, `D-588`, `D-589`, `D-591`, `D-592`, `D-594`,
  `D-598`, `D-608`, `D-609`), [ADR-0011](../../decisions/adr-0011-action-point.md),
  [`progettazione-hud.md`](../../technical/systems/progettazione-hud.md) (§6.7, §7, §7.1, §8, §11, §19, §22, §37),
  [`RT_ElementPairCatalog_v0.1.md`](../../balance/RT_ElementPairCatalog_v0.1.md),
  [`piano-rename-inverso-roster-2026-11-09.md`](../../technical/piano-rename-inverso-roster-2026-11-09.md),
  [`riktor.md`](../../characters/v0.1/riktor.md); il GDD del 2026-11-08 come input
  ([`master-game-design-document-input-2026-11-08.md`](../../research/handoff/master-game-design-document-input-2026-11-08.md) §2, §5, §6).
- **Codice letto**: `RTPlanValidationLibrary.h/.cpp` (budget e `OverBudget`), `RTHeroCatalogLibrary.cpp` (`ValidateHeroes`, `MakeBranth`),
  `RTCatalogLibrary.cpp` (azioni core, `DefaultActionPointCost`), gli enum `ERTAbilityShape`, `ERTResolutionPhase`, `ERTActionEffect`,
  `ERTActionInvalidReason`; ricerca di `ActionPoint`/`OverBudget` in `Source/RefactorTactics/UI` e `Source/RefactorTactics/Player`.
- **`skill.af`**: letto estraendo con Python le PNG e le JPEG incorporate (98 icone 512×512 più tavole di riferimento). Il significato di
  cornici, glifi e badge in Appendice B è una **lettura** della sessione, non una dichiarazione dell'autore.

| Gate | Esito |
|---|---|
| Compile · Tests · Determinism · Replay · Privacy · PIE · Packaged | `N/A`: il referto non cambia codice né asset |
| Prove di §4 (R1–R12) | `NOT RUN`: sono criteri d'accettazione proposti, non esiti |
| `node tools/radar/doc-links.ts --check` | `PASS` nel worktree, 2026-11-13: «tutti i percorsi citati risolvono» |

**Limiti del panel.** Gli esperti e i verificatori avevano un tetto di chiamate di tool (20 e 25) e hanno dichiarato ciò che non
hanno verificato. Fra i punti non verificati: l'esistenza di un campo di portata minima in `FRTActionDef` (serve a Chain Hook),
la cella finale di `LinearPass` quando l'ultima è occupata, l'esistenza di un effetto di cella persistente con durata, e se gli
effetti di cella sono replicati per intero ai client (decide il costo di una trappola nascosta). Restano aperti, e nessuna voce
di questo referto li presume. I verificatori hanno lavorato su `origin/main`, nella stessa base delle lenti.

## 8. Prossimi passi

Nessuna issue è stata aperta: le aprirà l'autore, o la sessione col suo via. Ordine proposto:

1. **Le decisioni sul kit**: DR-1…DR-4 e DR-8. Senza, la dock non ha una cardinalità.
2. **Il rename** [#4011](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4011) prima del kit nuovo, per migrare
   golden e id una volta sola.
3. **La spec HUD**: i requisiti R1–R7 di §4.1 entrano in [`progettazione-hud.md`](../../technical/systems/progettazione-hud.md)
   (§7.1, §19, §22, §37, §17) col via dell'autore. Valgono in tutti i modelli.
4. **La traduzione del kit di Riktor** con la tabella di §3.1 e le domande di dettaglio di §6, poi una `D-nnn` d'accettazione,
   la scheda [`riktor.md`](../../characters/v0.1/riktor.md) riscritta (oggi è anche STALE: elenca cinque abilità contro le sei del
   codice, e `ImpactShot` a 8 nel profilo d'attacco ma ancora a 24 nella sezione dell'abilità, il valore scartato da
   ADR-0007, contro gli 80 di `MakeBranth`) e le righe del catalogo eroi con la colonna del costo AP.
5. **E38**: l'ordine dentro la fase (DR-7) e, se l'autore emenda D-584 (1), la regola di ripetizione (DR-6), scritti come
   asserti **prima** del resolver a lista. Per l'ordine e per il budget lo propongono le conseguenze 1 e 6 di ADR-0011, che
   sono letture ribaltabili. Poi il resolver, il TurnLog con la coordinata d'ordine, il bot a lista.
6. **HUD a lista**: R8–R12 di §4.2, dopo E38.
7. **Contenuto**: le righe Earth+Electric ed Electric+Decay del catalogo delle coppie, gli stati nuovi che le decisioni
   lasciano in piedi, le clip, gli effetti e le icone riesportate senza il badge del costo.

Issue candidate, da aprire solo col via dell'autore: la traduzione del kit di Riktor (`MSP-5.d` esteso a Riktor); i pip AP e il
rifiuto `OverBudget` nel view-model dell'HUD (R1–R5); la privacy degli AP nelle viste delle unità non comandate (R6); la regola
di ripetizione in `ValidatePlan` (DR-6); la coordinata d'ordine nel TurnLog (DR-7); la riesportazione delle icone con il costo
come layer (R2, R7).

## 9. Risposte dell'autore, primo giro (2026-11-13)

Domande poste con `AskUserQuestion` dopo il referto; il testo dell'opzione scelta è riportato alla lettera, «(Recommended)» è
l'etichetta dell'assistente. Le risposte dei tre giri sono registrate in D-612 e D-613 (§11).

- **DR-1.** «Il catalogo di Riktor del 13 novembre sostituisce il kit di oggi (Field Architecture: ImpactShot, KineticPanel,
  Reconfigure, Ram, Interposition, MortarShot)?» Scelta: **«Sì, dopo il rename #4011 (Recommended)»**: «Il catalogo, tradotto
  nelle regole del contratto, diventa il kit di Riktor con una D-nnn d'accettazione. Prima si chiude il rename Branth → Riktor
  (#4011), così gli id delle abilità nuove nascono già come Hero.Riktor.*. Riktor perde KineticPanel, Reconfigure e
  MortarShot.» Le altre erano «Sì, subito su Hero.Branth», «No, resta input» e «Ibrido».
- **DR-2.** «Che cosa sono le «Opzione 1/2/3» di ogni gruppo del catalogo?» Scelta: **«Tutte nel kit»**: «Riktor porta tutte
  le sedici voci. Serve alzare il tetto di sei voci, cambiare la struttura a quattro fondamentali e ridisegnare l'input (E48).»
  Le altre erano «Alternative, ne scelgo una (Recommended)» e «Varianti scelte dal giocatore».
- **DR-3.** «Con un'opzione per gruppo Riktor porta sei abilità oltre l'attacco base, ma la struttura dell'eroe ne prevede
  quattro (più al massimo una generica del catalogo core). Come entra il kit?» Risposta libera: *«non decidiamolo ancora. per
  il momento base piu 6, 2 reaction e overclock»*. La struttura resta **aperta**.
- **DR-5.** «Un effetto di gioco può cambiare il budget AP di un'unità, come il +1 di Magnetic Bastion o il −1 di Necrotic
  Anchor?» Risposta: *«spiega meglio»*. La domanda è riformulata nel giro successivo.

**Letture dell'assistente, non decisioni:**

- Con DR-2 le «Opzioni» non sono più alternative ma **voci distinte**, ciascuna col proprio id: §3.2 e le domande di dettaglio
  di §6 valgono per tutte le sedici.
- «Tutte nel kit» e *«base piu 6, 2 reaction e overclock»* si leggono insieme così: le sedici voci stanno nel kit, e la dock
  provvisoria ha un attacco base, sei abilità, due reazioni e l'Overclock. Chi sceglie quali voci occupano quei posti resta
  aperto. Da confermare.
- «Overclock» è la meccanica del GDD §2.4: +2 AP nel turno in corso, pagati bloccando una fase nel turno dopo. L'opzione scelta
  di D-591 (1) registra il GDD come superato su quel punto (il punto (d)), e D-265 non ammette una risorsa universale. Va chiesto.
- Se l'Overclock entra, il budget deve poter variare per unità e per turno: è la domanda DR-5, e la raccomandazione di §6
  («no») va rivista.

## 10. Risposte dell'autore, secondo giro (2026-11-13)

Prima delle domande la sessione ha spiegato DR-5 con un esempio (un piano Dash + Blast + Move = 6 che attraversa
l'Anchor) e ha cambiato raccomandazione da «6 fisso» a «variabile al planning», perché l'autore vuole l'Overclock nella dock.

- **DR-5.** «Il budget di AP di un'unità può essere diverso da 6 in un certo turno?» Scelta: **«Variabile, fissato al
  planning (Recommended)»**: «Il budget del turno diventa un numero dell'unità, noto quando pianifichi: 7 dopo un Bastion
  scattato, 5 dopo aver attraversato un'Anchor, 8 con l'Overclock se entra. Un piano già validato non cambia mai, quindi
  l'Anchor colpisce il budget del turno dopo. Serve un campo nuovo nello snapshot, nell'hash e nel replay.» Le altre erano
  «No, 6 per tutti, sempre» e «Variabile anche in risoluzione».
- **Overclock.** «L'Overclock del GDD (+2 AP nel turno in corso, pagati con una fase bloccata nel turno dopo) entra nel
  gioco?» Risposta libera: *«l'overclock e' una skill che ti da un vantaggio per uno svantaggio successivo. un esempio e'
  quello che hai scritto»*. Le opzioni erano «Sì, per tutti come nel GDD», «Sì, come meccanica di kit» e «Non ora».
- **Dock.** «Leggo «base più 6, 2 reaction e overclock» come la dock provvisoria di Riktor: un attacco base, sei abilità, due
  reazioni e l'Overclock, mentre le sedici voci restano nel kit. È così?» Scelta: **«Sì, dock provvisoria»**: «La spec
  dell'HUD si progetta per quei posti. Chi sceglie quali delle sedici voci li occupano, e quando, si decide dopo, come hai
  chiesto.» L'altra era «No, la dock mostra tutte e sedici».

**Letture dell'assistente, non decisioni:**

- L'Overclock è una **categoria di skill**: un vantaggio subito, uno svantaggio in un turno successivo. Il +2 AP con una fase
  bloccata è *un* esempio, non la regola. Riktor non ha ancora la sua skill di Overclock: il catalogo non ne contiene una.
- La risposta non dice se ogni eroe ha un posto di Overclock. Il punto (d) di D-591 (1) riguardava l'Overclock **universale**
  del GDD, e resta superato finché una decisione non dice altrimenti. Una skill di kit con un contrappeso è ciò che D-265
  ammette («una risorsa è ammessa solo come meccanica specifica di un kit che la motivi»), ed è un drawback nel senso di
  D-584 (4).
- Il budget variabile copre i tre casi: Magnetic Bastion (+1 nel turno dopo), Necrotic Anchor (−1 nel turno dopo, non più
  «sfasando» il turno in corso) e un Overclock che dia AP. `RTPlanValidationLibrary.h:128-131` prevede già il passo: il valore
  passa sull'unità e la costante 6 diventa il default.
- Per l'HUD: i pip leggono il budget dell'unità per il turno (R1, R5) e mostrano la causa di un budget diverso da 6 (R11 non
  dipende più da una decisione). La dock provvisoria ha un posto per l'attacco base, sei per le abilità, due per le reazioni e
  uno per l'Overclock, più le generiche.

## 11. Risposte dell'autore, terzo giro, e registrazione (2026-11-13)

Dopo il secondo giro l'autore ha chiesto, scegliendo tutte e quattro le voci di una domanda a scelta multipla, di registrare le decisioni,
aggiornare la spec dell'HUD, porre le domande aperte e aprire una PR. Le quattro domande aperte, testo dell'opzione scelta alla lettera:

- **DR-4.** «Il bootstrap monta su Riktor un equipaggiamento di default: Weapon.Impact (portata −1 e una spinta sull'attacco base),
  Gadget.PortableCover (copertura bassa) e Reaction.Cleanse. Resta col kit nuovo?» Scelta: **«Si ridisegna col kit (Recommended)»**:
  «Arma, gadget e modulo di reazione si scelgono di nuovo, coerenti col catalogo elettrico, nella stessa D-nnn del kit. Il Cleanse era
  stato scelto anche come risposta allo Slow di ImpactShot, che esce dal kit.» Le altre erano «Resta com'è» e «Si toglie».
- **DR-6.** «Il testo scelto di D-584 (1) («Solo il budget conta») permette la stessa azione più volte nello stesso piano, e oggi il
  cooldown non ferma la seconda copia. Va cambiato?» Risposta libera: *«1, ma ci sono abilita' ripetibili, come muoversi.»*, dove «1» è
  la prima opzione, «Con cooldown: una volta (Recommended)»: «Un'azione con cooldown compare al più una volta nel piano; quelle senza
  cooldown (attacco base, Move, Wait) si ripetono finché c'è budget. Senza questa regola Skewer due volte fa 760 danni e due Stun. Emenda
  D-584 (1).» Le altre erano «Ogni azione d'eroe una volta» e «Nessun limite, come oggi».
- **DR-7.** «Dentro una fase, due azioni della stessa unità con la stessa priorità: in che ordine si risolvono?» Scelta: **«Come le
  dichiari (Recommended)»**: «La seconda vede lo stato lasciato dalla prima. L'ordine di dichiarazione entra nel piano, nello snapshot e
  nell'hash, e la timeline lo mostra e permette di cambiarlo.» Le altre erano «Per id stabile dell'azione» e «Insieme, sullo stesso stato».
- **DR-8.** «Quanto costa un'Ultimate che non risolve nel Blast, e in quale fase sta Iron Maiden?» Scelta: **«Sempre 3, Iron Maiden in
  Prep (Recommended)»**: «Ogni Ultimate costa 3 qualunque sia la sua fase; Iron Maiden risolve nel Prep, come nel GDD, prima del Dash e
  del Blast.» Le altre erano «Fase + 1, Iron Maiden in Prep a 2» e «Iron Maiden nel Blast, a 3».

**Registrazione.** I numeri sono stati misurati prima di scrivere: su `origin/main` = `9560053a2` il massimo è D-610, e una sessione
peer ha preso D-611 lo stesso giorno per un'altra decisione. Le risposte vanno in due voci:

- [D-612](../../decisions/RT_PDR_00_Decision_Log.md), **il kit di Riktor**: DR-1, DR-2, DR-3 (rinviata), la dock provvisoria,
  l'overclock come categoria di skill e DR-4;
- [D-613](../../decisions/RT_PDR_00_Decision_Log.md), **l'economia degli AP per tutto il roster**: DR-5, DR-6, DR-7 e DR-8.

Nello stesso giro sono stati annotati D-584, D-585, D-586, D-591, D-594, la testata e la conseguenza 1 di ADR-0011, `MSP-5.d` e le voci
nuove `KIT-1`…`KIT-3` di [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), la scheda di Riktor, la riga 13 del piano del rename e
[`progettazione-hud.md`](../../technical/systems/progettazione-hud.md), che ha la §19-bis e tre rimandi.

## Appendice A — Il catalogo dell'autore, alla lettera

Testo ricevuto nella richiesta del 2026-11-13, senza correzioni. Gli emoji e le maiuscole sono dell'autore.

> ⛓️ RIKTOR — Catalogo Ufficiale delle Abilità
>
> 1. Attacco Base (Costante: 2 AP | Nessun Cooldown)
> Opzione 1 (Originale) — Shock Whip [⚡ Fulmine]: Infligge 160 Danno Energetico su singolo spicchio. Se il bersaglio si trova su terreni bagnati (Acqua, Fango, Vapore), il danno si incatena ai nemici adiacenti.
> Opzione 2 (Cleave) — Static Sweep [⚡ Fulmine]: Fendente ad arco da 120 Danno Energetico distribuito su 3 esagoni frontali contigui. Ottimo per colpire formazioni compatte.
> Opzione 3 (Utility) — Taser Strike [⚡ Fulmine]: Colpo mirato da 100 Danno Energetico che riduce la capacità di movimento base del bersaglio di 1 esagono per il turno successivo.
>
> 2. Prep Phase (Costante: 1 AP)
> Opzione 1 (Originale) — Electrocute [⚡ Fulmine]: Genera un'aura da 80 Danno Energetico a turno per 2 turni sull'esagono di Riktor; agisce come area denial contro i nemici in mischia e innesca le reazioni a catena.
> Opzione 2 (Trappola Morte) — Necrotic Anchor [💀 Morte]: Impregna un esagono adiacente di energia oscura. Se un nemico lo attraversa durante la Dash Phase, perde 1 AP dal proprio pool, sfasando la sua pianificazione.
> Opzione 3 (Magnetismo Terra) — Magnetic Spikes [🪨 Terra]: Magnetizza il terreno (aumentando il costo di movimento di +1 AP). Se un nemico compie uno scatto o un salto nell'area, subisce 50 danni fisici puri e viene ancorato a terra ($Z=0$).
>
> 3. Dash Phase (Costante: 2 AP)
> Opzione 1 (Originale) — Chain Hook / Chain Pull [🪨 Terra]: Aggancia un bersaglio (gittata 2–4 esagoni) e lo trascina di 2 caselle (o tira Riktor verso un muro), infliggendo 140 Danno Fisico e sradicando i cecchini.
> Opzione 2 (Sbalzo Fulmine) — Voltage Leap [⚡Fulmine]: Un salto di 2 esagoni. All'atterraggio, sprigiona un'onda d'urto da 90 danni ad area che spinge i nemici adiacenti indietro di 1 micro-spicchio (provocando Body Block se collidono).
> Opzione 3 (Carica Scudo) — Riot Charge [🛡Scudo]: Una carica lineare e inarrestabile di 3 esagoni che spazza via chiunque si trovi sulla traiettoria e conferisce a Riktor un'armatura temporanea da 150 HP alla fine del movimento.
>
> 4. Blast Phase - Abilità Primaria (Costante: 2 AP)
> Opzione 1 (Originale) — Corrosive Shackles [💀 Morte]: Infligge 200 Danno Oscuro in un raggio di 2 esagoni e blocca completamente le cure ricevute dal bersaglio per 2 turni.
> Opzione 2 (Impatto Terra) — Seismic Smash [🪨 Terra]: Un colpo stritolante da 220 Danno Fisico in mischia. Se il bersaglio viene sbalzato contro un muro perimetrale solido, subisce il 50% di danni aggiuntivi per schiacciamento.
> Opzione 3 (Catena Fulmine) — Arc Discharge [⚡ Fulmine]: Spara un dardo da 180 Danno Energetico. Se il bersaglio condivide il macro-esagono con un alleato, la scossa rimbalza infliggendo 100 danni garantiti al compagno (punendo il posizionamento troppo stretto).
>
> 5. Blast Phase - Ultimate (Costante: 3 AP)
> Opzione 1 (Originale) — Skewer [⚡ Fulmine]: Eroga 380 Danno Energetico a cono frontale che ignora l'armatura e applica Stun per 1 turno, acquisendo priorità deterministica sui bersagli già elettrificati.
> Opzione 2 (Controllo Morte) — Iron Maiden [🪨 Terra / 💀 Morte]: Ingabbia un singolo bersaglio entro 3 esagoni bloccandone movimento, Linea di Vista, cure e scudi. Se l'esagono di atterraggio è Fango, la gabbia sprofonda azzerando l'armatura del bersaglio.
>
> 6. Reaction Phase (Costanti: 1–2 AP)
> Reaction Guard (Guardia / Protezione): Magnetic Bastion [🪨 Terra]: Guardia da 1 AP in cui Riktor intercetta un attacco destinato a un compagno subendolo al suo posto, guadagnando però +1 AP extra per il proprio turno successivo. Con le catene si lancia, spostandosi nell'esagono pianificato, bloccando e ricevendo il colpo.
> Reaction Counter (Contrattacco / Punizione): Shock Armor [⚡ Fulmine]: Reazione da 1 AP. Se Riktor viene colpito in mischia durante la Blast Phase nemica, l'armatura rilascia una scarica che applica Fulmine sull'esagono dell'attaccante e annulla qualsiasi Knockback subito.
>
> Combo Alchemiche (2 e 3 Elementi):
> Fango Magnetico (⚡ Fulmine + 🪨 Terra in Prep): Dimezza i Punti Movimento dei nemici nell'esagono.
> Catene Spettrali (⚡ Fulmine + 💀 Morte in Dash): Paralisi oscura che disattiva le Reaction nemiche.
> Sovraccarico Elettrolitico (⚡ Fulmine + 🪨 Terra + 💧 Acqua in Blast): Arco voltaico a catena da +450 Danni.
> Gabbia di Faraday Letale (⚡ Fulmine + 🪨 Terra + 💀 Morte in Blast): Infligge danno puro proporzionale agli HP mancanti e cura Riktor.
> Affinità (Sinergie): Murdock & Gadget (per tirare i nemici nelle zone modificate da Flux o trappole) e The Fey (per trascinare i bersagli dentro i rovi).
> Matchup e Debolezze:
> Forte contro: Kallari (l'aura pre-caricata la stordisce in Dash) e Murdock (sradicato dalle coperture).
> Debole contro: Phase (il Legame Cristallino rende l'alleato immune agli sbalzi) e Steel (le barriere bloccano le traiettorie di trazione).

## Appendice B — La grammatica delle icone di `skill.af`

⚠️ **Lettura della sessione, da confermare con l'autore.** Il file contiene tavole di schizzi e 98 icone 512×512 composte da tre strati:

| Strato | Che cosa porta | Valori osservati |
|---|---|---|
| **Cornice** | la **fase** in cui l'abilità risolve | cerchio blu con quattro tacche = Prep · chevron giallo = Dash · rombo rosso = Blast · **esagono viola = reazione**. Nessuna cornice per `Move` né per `Cleanup` |
| **Glifo centrale** | l'**elemento** | favo ciano (Scudo → `Element.Aegis`), fiamma (Fuoco), cristallo viola infranto (Disintegrazione), saetta (Fulmine → `Element.Electric`), montagne ocra (Terra), gocce (Acqua), vento (Aria), cristallo di neve (Freddo), gemma magenta (Flux → `Element.Warp`) |
| **Badge in alto a sinistra** | con ogni probabilità il **costo in AP** | esagonino con una, due o tre tacche (I · II · III). Esempio: un'icona Prep + Terra con III, che corrisponde a un'Ultimate di Prep come Iron Maiden del GDD |
| **Badge agli altri angoli** | effetti secondari | croce verde barrata (blocco cure), occhio barrato (vista bloccata), »» (spostamento), freccia in su o in giù (quota), rombo barrato (ignora armatura), stella (stordimento) e altri |

Le tavole contengono anche una barra a strisce rosse e nere, che corrisponde al *Phase Lock* dell'Overclock del GDD, e un arco
arancio-ciano. Overclock e Phase Lock restano fuori dal contratto: [D-584](../../decisions/RT_PDR_00_Decision_Log.md) (5) e
[D-265](../../decisions/RT_PDR_00_Decision_Log.md).

⚠️ **Il file non va versionato così com'è**: 81 MB di binario in un repository che non usa Git LFS ([`AGENTS.md`](../../../AGENTS.md) §7).
Se l'autore vuole che la grammatica resti citabile, la forma economica è una tavola PNG ridotta in `docs/research/` con la sua riga di
provenienza ([`docs/technical/asset-licenze.md`](../../technical/asset-licenze.md), `node tools/asset-provenance/check.ts`).
