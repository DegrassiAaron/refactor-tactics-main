# Catalogo iconografico — sette categorie, lista consolidata

> ⚠️ **Nota di consegna (2026-08-26).** Le **40 chiavi provate** di questo censimento sono
> **disegnate** e vivono in `Content/RT/UI/_Generated/Icons/`. Sono **asset, non chiavi
> richieste**: `RefactorTactics.IconCatalog.V01CategoriesPopulated` fallisce se una di loro
> comparisse in `RequiredIconIds()`, e la roadmap le assegna a **E25**. Chi aprira' la lista
> dovra' scrivere il ramo nello stesso commit in cui le innesta.
>
> `Objective` e `Coordination` restano vuote, ed e' un esito dichiarato.


**Che cos'è questa lista.** Il piano di disegno per le sette categorie non popolate, ricavato da sette censimenti separati e ripulito dai doppioni. **Non è un innesto da fare oggi nel catalogo v0.1**: `RefactorTactics.IconCatalog.V01CategoriesPopulated` (`Source/RefactorTactics/Tests/RTIconCatalogTests.cpp:142-151`) fallisce se una chiave di queste sette compare in `RequiredIconIds()`, e `roadmap-post-v0.1.md:378-384` le assegna a **E25**. Aprire la lista richiede una voce nel Decision Log, non una decisione di illustrazione.

**Misura di partenza, rifatta su questo branch** (non copiata dalla roadmap): `URTIconLibrary::RequiredIconIds()` produce oggi **61** chiavi — 4 Phase + 37 azioni di `GetCoreActionCatalog()` + 11 tag `Status.*` + 3 Certainty + 6 Identity. Le cinque abilità di Gadget non sono in quell'elenco: `RequiredIconIds()` deriva le azioni dal catalogo core, non dai kit d'eroe.

---

## 1. Arbitrati fra categorie

Ogni riga toglie una doppia verità. Sono decisioni prese qui, non proposte.

| Cosa era contesa | Assegnata a | Perché |
|---|---|---|
| `Cover.Low` · `Cover.High` (+ `Temporary`, `Destroyed`) | **MapInteraction** | `Environment` è dichiarata «superfici e terreni» (`RTIconCatalogData.h:28`) e una superficie **riempie la primitiva `Cell`**; una copertura vive su un **bordo**, ha integrità, e la producono/spostano/abbattono azioni via `ERTStructureOp` (`RTActionDef.h:178-196`), esattamente come porte e archi. Decisivo: `Cover.High` e `Door.Closed` condividono lo **stesso slot geometrico** (il pannello di bordo, `RTHexMapActor.cpp:982-993) e promettono la stessa cosa — separarle in due categorie renderebbe invisibile a chi disegna la collisione più pericolosa del set. Traslocano **in blocco**, mai una sola. |
| Porte e ponti (`Environment.Door` / `.Bridge`) | **MapInteraction** | Definizione dell'enum alla lettera: «porte, leve, ponti, ascensori» (`RTIconCatalogData.h:31`). |
| Nome del collegamento fra celle: `Arc` o `Bridge` | **`Arc.*`** | Il codice dice arco (`ERTHexArcState`, `URTHexArcLibrary`, `FRTArcChange`); `Bridge` è già un valore di `ERTHexTransitionKind` e userebbe lo stesso nome per il genere e per una sua specie. Il TurnLog che dice `BridgeDamaged` resta com'è: è il log, non la chiave. |
| `TargetUnknown` (Information ↔ Warning) | **Warning** | Compare in un contesto di **rifiuto**, e i rifiuti sono `Warning` per definizione dell'enum (`RTIconCatalogData.h:42`). Entrambi i censimenti convergono. Vincolo per chi disegna: non chiamarla `InvalidTarget` — il codice distingue `TargetGone` da `TargetUnknown` e comprimerle perde l'unica cosa che il giocatore deve capire. |
| Visibilità (`Information.Detected` ↔ `Certainty.Confirmed`) | **nessuna delle due: `Detected` non si disegna** | `progettazione-hud.md` §25 assegna a `Detected` «rappresentazione normale autorizzata», cioè **nessun marker**. La sua sola superficie possibile è un chip in lista che non esiste, e in monocromia sarebbe indistinguibile da `Certainty.Confirmed` (bordo solido + fill pieno) e da `Identity.Enemy`. Va in §4. |
| `UncertainContact` ↔ `LastContact` ↔ `CellOnly` | **`LastContact` + `CellOnly`** | Misurato in codice: l'unico produttore di `ERTAwareness::Uncertain` in v0.1 è la **memoria** — `AwarenessOfUnit` restituisce `Uncertain` quando esiste un `FRTLastKnownContact` e la cella non è visibile (`RTTeamKnowledge.cpp:70-79`), e quel contatto ha una **cella esatta**. L'«area senza cella esatta» nasce con il rumore, che non emette. Oggi `UncertainContact` sarebbe lo stesso fatto detto una terza volta, e il `?` finirebbe su tre glifi sovrapposti. |
| Ricarica (`Warning.Cooldown` ↔ `Reaction.Unavailable`) | **Warning** per la v0.1 | `Cooldown` è rilevata **in pianificazione** (`RTPlanValidationLibrary.cpp:136`, primo motivo per precedenza) e ha già una resa a schermo («(ricarica N)», `RTHUD.cpp:853`): il giocatore può correggere. `ERTReactionOutcome::Unavailable` è un esito **a consuntivo** senza campo HUD che lo trasporti. Tenuta in sospeso in §4. |
| Moduli di reazione ↔ `Action.*` (`GrantedActionId`) | **chiavi proprie in `Reaction`** | Risolvere l'icona dal `GrantedActionId` risparmierebbe sette disegni ma farebbe collassare `ReactiveShield` e `CounterShot`, che condividono `Action.Counter` con effetti **opposti** (15 scudo contro 14 danni, `RTCatalogLibrary.cpp:523-541`). Si disegnano però solo i **quattro** portati di default dal roster v0.1. |
| `Hazard` (Environment ↔ Warning) | **nessuna** | Non esiste un controllo in pianificazione che dica «il tuo percorso attraversa una cella pericolosa»: `IsHazardousSurface` è consumata dalla reazione e dal Cleanup. E come ombrello sopra `Fire` e `Conductive` collide con entrambe. |
| «Alleato» (`Coordination.AllyIntent` ↔ `Identity.Ally`) | **Identity**, già spedita | `bIsAlly` è già letto da `ARTHUD` e CP 11.2 prescrive «previsto = tratteggiata + icona di squadra», che è `Identity.Ally`. Un secondo glifo «alleato» in un'altra categoria è indistinguibile in monocromia. |
| `Exposed` · `Marked` · `Obscured` · `Reveal` (Information ↔ Status) | **Status**, già nelle 61 | Sono Gameplay Tag registrati: riproporli darebbe due icone per lo stesso tag. |
| Obiettivo come elemento di mappa (Objective ↔ MapInteraction) | **Objective** — che però resta vuota (§3) | Un obiettivo ha stati propri (neutro/cattura/conteso/posseduto/bloccato); metterlo in MapInteraction creerebbe due verità sulla stessa entità. |

Regola trasversale che nessuna tabella copre e che chi disegna deve applicare prima di ogni glifo: **azione, superficie e stato sono tre livelli con tre ancoraggi diversi** — l'azione è un gesto con una sorgente, la superficie riempie la primitiva `Cell` (§2, esagono 2D, nessuna prospettiva), lo stato si attacca a un unit marker. Le terne complete sono acqua (`CreateWater` · `ShallowWater` · `Wet`), fuoco (`Ignite` · `Fire` · `Burning`) ed elettricità (`Electrify` · `Conductive` · `Electrified`), **con l'avvertenza che la terza non è simmetrica**: `Conductive` non è elettricità sulla cella, è un materiale che la porterebbe.

---

## 2. Chiavi provate — il set da disegnare

Ordine: per categoria; dentro la categoria, per frequenza a schermo (prima ciò che è autorato nelle arene spedite e visibile a ogni partita, poi ciò che nasce in partita). La riga **G:** è la sola che serve a chi disegna: primitiva di `03-forme-e-primitive.md` + da cosa deve restare distinta.

### Environment — 7 chiavi
Tutte e sette sono autorate cella per cella nelle arene spedite (`RTMatchSetupLibrary.cpp:181-198`, `:246-270`), quindi si vedono tutte a ogni partita.

1. **`UI.Icon.Environment.ShallowWater`** — costa 2 MP, bagna finché ci resti, conduce. `RTTerrainLibrary.cpp:44`; spegne il fuoco in `RTTurnManager.cpp:1665`.
 **G:** primitiva `Cell` (§2) come **riempimento**. Distinta da `Action.CreateWater` (gesto con sorgente) e da `Status.Wet` (unit marker, §7). **Nessun fulmine**: la conduttività è del payload, non dell'acqua.
2. **`UI.Icon.Environment.Fire`** — 2 MP, 10 danni, `Burning` 2 turni. `RTTerrainLibrary.cpp:49`.
 **G:** `Cell` riempita. **Vietata la primitiva `Damage`**: §4 esclude la fiamma da `Damage`, e la reciproca vale — il fuoco non prende in prestito impact/crack, o si legge come un allarme di UI invece che come una proprietà della mappa. Distinta da `Action.Ignite` e `Status.Burning` (§7).
3. **`UI.Icon.Environment.Rough`** — 2 MP, blocca Dash/Charge. `RTTerrainLibrary.cpp:40`.
 **G:** `Cell`. Distinta dal **marcatore di cella bloccata** — D-183 misura il collasso in scala di grigi ed è «la peggiore delle nove»: il glifo deve chiudere quella confusione, non aggravarla. Vietato il «Dash sbarrato»: un'azione negata appartiene a `Warning`.
4. **`UI.Icon.Environment.Ice`** — chi finisce il movimento qui scivola di una cella. `RTTerrainLibrary.cpp:59`, consumata da `ApplyIceSliding`.
 **G:** `Cell`. **Vietata la grammatica `Push`/`Forced Movement`** (§5): quella dice «qualcuno ti ha spinto», qui il soggetto è il pavimento — se leggono uguale il giocatore non capisce chi ha deciso. Distinta da `ShallowWater` (tinte adiacenti, `RTHexLibrary.cpp:271/275`).
5. **`UI.Icon.Environment.Conductive`** — non fa nulla da sola; una scarica si propaga sulle celle collegate. `RTTerrainLibrary.cpp:53`.
 **G:** `Cell`, ma il soggetto è il **materiale** (griglia/piastra), non l'energia. Vietati il fulmine e la primitiva `Chain` (§3: la catena è la propagazione, e §7 pinna già `Electric ↔ Reaction`). Distinta da `Status.Electrified` e `Action.Electrify`: è la differenza fra un cavo e una scossa.
6. **`UI.Icon.Environment.Smoke`** — offusca chi ci sta dentro, tappa il targeting a 2 celle. `RTTerrainLibrary.cpp:56`, cap in `EffectiveTargetingRange`.
 **G:** `Cell`, disegnata come **oggetto**. Vietati fade e opacità ghost: sono i modificatori `Uncertain`/`Predicted` (§6) e li userebbe per un'altra cosa. Distinta da `Status.Obscured` — la superficie applica letteralmente quel tag, e i due glifi si vedranno a un centimetro l'uno dall'altro: la differenza è cella-contro-unità, non densità di tratto.
7. **`UI.Icon.Environment.HighGround`** — voce del catalogo terreni, si vede sulla board; in v0.1 **non cambia nessun numero**. `RTTerrainLibrary.cpp:61`, autorata in `RTMatchSetupLibrary.cpp:265-266`, due anelli di secondo canale (`RTHexLibrary.cpp:233`).
 **G:** `Cell`. ⚠️ **Nessun occhio, nessun mirino, nessun cono di vista**: D-018 e il pin di `CLAUDE.md` §2 tolgono ogni bonus alla quota, e `spec-terreni-e8.md:120` registra il bonus visuale come non implementato — un glifo che lo promette mente. Distinta dall'indicatore Layer/Elevation della HUD §20, che è un'altra cosa (`Height`/`Layer` della cella).

### MapInteraction — 8 chiavi
Regola di categoria: **tutto è ancorato al bordo**, mai al riempimento dell'esagono. Un glifo che riempie la cella dice una cosa falsa (`RTHexCellData.h:50-56`).

1. **`UI.Icon.MapInteraction.Cover.Low`** — barriera su un bordo: −10 al danno che entra da lì, non ferma vista né passo. `RTHexCellData.h:45`, `RTCombatLibrary.h:125`; autorata in `RTMatchSetupLibrary.cpp:286-290`.
 **G:** primitiva `Cover` (§4: segmento/barriera tattica con cue di protezione laterale), ancorata al terreno. Distinta dalle altre tre difese di §4.1 — `Guard` e `Brace` sono **azioni** (postura), `Shield` è una **risorsa** che assorbe. Da `Cover.High` la separa l'**altezza della barriera**, non il colore né lo spessore del tratto.
2. **`UI.Icon.MapInteraction.Cover.High`** — muro su un bordo: nega vista, passo e proiettili. `RTHexCellData.h:46`, blocco LOS in `RTHexCoverLibrary.cpp:66`; autorata in `RTMatchSetupLibrary.cpp:371`.
 **G:** `Cover` piena fino al bordo superiore. Distinta da `Door.Closed` **per che cosa è, non per che cosa fa**: producono lo stesso effetto sul bordo, ma la porta ha uno stato azionabile (un'anta) e il muro no. Sbagliarlo significa credere aggirabile ciò che è abbattibile.
3. **`UI.Icon.MapInteraction.Door.Closed`** — non ci si passa e non si vede, ma un `Interact` adiacente apre. `RTHexCellData.h:193`, `StateBlocks` in `RTHexDoorLibrary.h:97-99`; autorata come gate della lane sud del Relè, `RTMatchSetupLibrary.cpp:301`.
 **G:** primitiva `Object` (§2: blocco geometrico con base/anchor) sul bordo, con **anta/cardine visibile** = lo stato azionabile. Distinta da `Cover.High` (vedi sopra).
4. **`UI.Icon.MapInteraction.Arc.Active`** — il collegamento fra due celle (tipicamente fra due layer) esiste ed è percorribile; senza di lui le due celle **non** sono adiacenti. `RTHexCellData.h:372-376`, `IsArcTraversable`; unica transizione terra→piattaforma delle arene, `RTMatchSetupLibrary.cpp:151` e `:501`.
 **G:** primitiva `Line` (§3: origine, segmento, punta, **senza nodi intermedi**) fra due ancoraggi. Distinta da `Move` — §5 dice che i **nodi intermedi** definiscono `Move`, e §7 elenca già `Line ↔ Move` — e da `Direction`. Qui la linea è un **oggetto della mappa**, non un'intenzione dell'unità.
5. **`UI.Icon.MapInteraction.Door.Open`** — ci si passa e si vede; è lo stato che `Action.Interact` chiede, l'unico verbo di mappa che la v0.1 spedisce. `RTHexCellData.h:192`, `RTCatalogLibrary.cpp:1102-1111`.
 **G:** stessa `Object` di `Door.Closed` con anta ruotata e varco libero. Distinta da `Action.Interact` (un **comando**, non uno stato) e da `Door.Destroyed`, che il mondo 3D disegna con lo stesso pannello: la differenza da portare è la **richiudibilità**.
6. **`UI.Icon.MapInteraction.Cover.Temporary`** — copertura eretta in partita, integrità 30, **scade in 2 turni** nel Cleanup. `RTCatalogLibrary.cpp:1504-1521`, `TickDynamicCovers`, esiti `CoverCreated`/`CoverExpired`.
 **G:** `Cover` bassa + marca di **durata** (un conteggio). ⚠️ **Vietati tratteggio e punteggiato**: quel vocabolario è già speso dalla HUD §17 per l'incertezza degli intenti, e direbbe «forse c'è una copertura» invece di «c'è, e sta per finire».
7. **`UI.Icon.MapInteraction.Cover.Destroyed`** — abbattuta: da qui in poi quel bordo si attraversa e si tira. `RTTurnLog.h:230-235`, prodotta da `DamageStructure` (`Action.HeavyAttack` 20, `Gadget.BreachCharge` 35).
 **G:** `Cover` + `Damage` (§4: impact/crack astratto, **3–4 diramazioni max**, niente spada/proiettile/teschio/fiamma) e moncone alla base. Distinta da `Arc.Destroyed`: se la distruzione è un overlay generico i due collassano e il replay diventa illeggibile.
8. **`UI.Icon.MapInteraction.Arc.Destroyed`** — crollo **terminale**: i due layer tornano irraggiungibili e le macerie restano (l'arco non viene rimosso, `RTTurnManager.cpp:1727-1729`). Integrità 40, prodotto da `DamageArc` (`RTTurnManager_Blast.cpp:876-898`).
 **G:** `Line` spezzata da un `Interrupt` (§4: break/notch che spezza una linea) con i **monconi che restano**. Distinta dall'**assenza** di collegamento (nessun segno): «il ponte è crollato» e «qui non c'è mai stato un ponte» producono lo stesso fallimento di percorso e sono due letture diverse della mappa.

### Information — 2 chiavi
Le altre quattro candidate sono in §4. Nessuno stato mostrato qui passa da un filtro di Team Knowledge diverso da quello che le produce: sono entrambe già filtrate per squadra.

1. **`UI.Icon.Information.LastContact`** — dove quell'unità è stata vista l'ultima volta, e che è passato del tempo. `FRTLastKnownContact` (`RTTeamKnowledge.h:25-39`); marker già **presente** secondo `progettazione-hud.md` §40; scadenza a fine turno successivo.
 **G:** primitiva `Cell` (la cella è **esatta**) + una marca di **tempo** — scia, quadrante, non la sola opacità. La bugia da evitare è temporale, non spaziale: una silhouette piena direbbe «è lì adesso», falso appena il bersaglio si muove (`RTTeamKnowledge.h:20` lo mette in guardia per iscritto). Distinta dal modificatore `Predicted` (§6): il ricordo è passato, la predizione è futuro — se entrambi sono ghost, il giocatore non sa se inseguire o aspettare.
2. **`UI.Icon.Information.CellOnly`** — di quel bersaglio puoi colpire la **cella**, mai l'unità. `ERTTargetKnowledge::CellOnly` (`RTTeamKnowledge.h:102`), applicato in partita dal Blast (`RTTurnManager_Blast.cpp:413-423`).
 **G:** `Cell` (§2, esagono 2D pulito) **e nient'altro**: qualunque reticolo, bracket o silhouette su unità qui mente, perché il gioco rifiuterebbe quel bersaglio. Distinta da `LastContact`, che punta alla stessa identica cella e dice un'altra frase («era qui» contro «qui puoi ancora sparare»), e dai `Warning` di bersaglio invalido: **non è un rifiuto, è un'opzione degradata**.

### Reaction — 10 chiavi
⚠️ Vincolo di categoria non grafico: `Armed` è **ally-only** per privacy (`RTIntentPrivacyLibrary.h:177-179`, D-021). Nessun glifo di questa categoria ha una variante «nemico armato», in nessuna granularità.

1. **`UI.Icon.Reaction.Armed`** — quest'unità tiene pronta una reazione; il trigger lo decide l'avversario. `FRTArmedOverwatch::bCharged`, `FRTOverwatchWatcher::bArmed`, e un campo HUD **vivo**: `FRTIntentCertaintyStyle::bReactionArmed` (`RTHUD.h:159`).
 **G:** primitiva `Arc` (§3: sector boundary + anchor di facing) sul portrait, resa **obbligatoriamente** con la grammatica `Uncertain` (tratto discontinuo + `?`, imposto da `RTHUD.h:150-159`). Distinta da `Action.Overwatch`/`Action.Brace` — quelle sono le azioni che **armano**, questa è lo stato armato — e da `Certainty.Uncertain`, che è un asse indipendente e non un livello di piano.
2. **`UI.Icon.Reaction.Opportunity`** — una finestra di decisione è aperta adesso: 3,0 s, poi vale il default. `FRTReactionOpportunity`, `ERTClashLogEvent::OpportunityCreated`; la finestra del `Brace` si apre già in partita (`RTTurnManager_Blast.cpp:1202-1217`).
 **G:** marca di **finestra** (boundary che ospita il countdown). 🔴 **Vietata la famiglia `Phase`**: «una Reaction non è una quinta fase» è scritto due volte nell'owner e una terza in `RTIconCatalogData.h:26` — se il glifo eredita la famiglia di Prep/Dash/Blast/Move la regola cade a schermo. Vietato anche il ⚡ nudo: §7 pinna `Electric ↔ Reaction`. Distinta dal ⏱ della Delayed Action (semantica opposta: decisione live contro precommit).
3. **`UI.Icon.Reaction.Hold`** — non sparare: la reazione resta armata, la charge non si spende. È anche ciò che vale allo scadere. `HoldChosen`/`HoldTimeout`, `HoldResponse() == "HOLD"`.
 **G:** `Interrupt` trattenuto / stop netto. Distinta da `Action.Wait` (§7 la elenca testualmente: `Wait ↔ Hold`), da `Reaction.Armed` (tenere significa **restare** armati, ma uno è un pulsante da premere in 3 s e l'altro uno stato passivo su un portrait) e da `HoldGround`.
4. **`UI.Icon.Reaction.Fire`** — spara ora sul bersaglio nominato: spende la charge e tronca il movimento di chi hai colpito. `FireChosen`, token `FIRE:<UnitId>`, `RTTurnManager.cpp:4841-4847`.
 **G:** `Line` (origine, segmento, punta) verso un bersaglio nominato. ⚠️ Deve funzionare **ripetuta e affiancata** a un identificatore di bersaglio: un solo prompt può portare `FIRE A` / `FIRE B` / `HOLD`. Distinta da `Action.BasicAttack` e `Action.Overwatch` — §7 elenca già quella coppia, e questa è la terza punta dello stesso triangolo.
5. **`UI.Icon.Reaction.HoldGround`** — la risposta universale del `Brace`: tieni la cella e assorbi. `RTCatalogLibrary.cpp:87` e `:103`, finestra aperta in partita, D-047.
 **G:** primitiva `Brace` (§4.1: body/anchor + cuneo di contrasto). 🔴 Distinta da `Reaction.Hold`, che è la **stessa parola per un'altra cosa** (trattenere un colpo, non tenere la cella): D-049 registra la collisione, e due glifi simili producono un giocatore che preme la cosa sbagliata sotto pressione. Non riusa il glifo di `Shield` (§4.1 lo vieta esplicitamente) né quello di `Status.Braced`.
6. **`UI.Icon.Reaction.Sidestep`** — l'altra risposta del `Brace` di Phase: esci dalla linea di un passo. `RTCatalogLibrary.cpp:53-54` (`SelfReposition 1`), D-132.
 **G:** primitiva `Reposition` (§4: freccia breve origine→destinazione, **distinta dal path di `Move`**), orientata **trasversalmente** alla linea di tiro. ⚠️ Dal 2026-08-19 lo scarto **esce** dalla linea, non arretra lungo di essa (`RTCatalogLibrary.cpp:42-47`): un glifo di arretramento racconta la versione superata. Distinta da `Move`/`Sprint`/`Dash` e da `Push` (§7).
7. **`UI.Icon.Reaction.ReactiveShield`** — modulo di Gadget: 15 punti scudo quando incassi un colpo diretto. `RTCatalogLibrary.cpp:535-541`, default di `Hero.Gadget`.
 **G:** primitiva `Shield` (§4: scudo semplice, ampio negative space) con innesco. Distinta da `Action.Shield` (è in Preparation, **non** è una reazione, scudo 25 contro 15) e — 🔴 — da `Reaction.CounterShot`, che condivide lo stesso `Action.Counter` e lo stesso trigger con mestiere **opposto**: se qualcuno risolve i moduli riusando l'azione concessa, questi due diventano lo stesso glifo.
8. **`UI.Icon.Reaction.HazardEscape`** — modulo di Phase: quando la cella sotto di te diventa pericolosa, scappi di una verso dove guardi. `RTCatalogLibrary.cpp:586-590`, `ERTReactionTrigger::CellBecameHazardous`.
 **G:** `Reposition` che **esce** da una `Cell` marcata pericolosa; il soggetto è la fuga, non la fiamma. Distinta da `Environment.Fire` e `Status.Burning` (§7). ⚠️ È l'unica reazione che vive nel **Cleanup**: se il glifo eredita la famiglia Blast racconta il momento sbagliato.
9. **`UI.Icon.Reaction.AllyIntercept`** — modulo di Riktor: ti interponi e prendi al posto di un alleato entro 2 celle il colpo che era per lui. `RTCatalogLibrary.cpp:544-550`, `AllyHitByDirectAttack`, ciclo proprio in `ResolveCombat`.
 **G:** primitiva `Ally` (§2: unit marker rounded + connection tabs, **nessun `+`**) interposto su una `Line` in arrivo, con `Interrupt` nel punto d'incontro. Distinta da `Action.Intercept` (il comando) e — la più insidiosa — da `Hero.Wraith.InterceptShot`, che è la thin slice **Predictive** e non questa meccanica: due nomi quasi identici per un'interposizione e una previsione.
10. **`UI.Icon.Reaction.EmergencyDash`** — modulo di Wraith: quando sei bersagliato ti sposti di una cella **restando fronte alla minaccia**. `RTCatalogLibrary.cpp:556-565`, D-093 + D-104.
 **G:** `Reposition` + `Arc` di **facing conservato** — è l'unico tratto che lo separa da `Sidestep` e `HazardEscape`, che applicano lo stesso `SelfReposition 1`. 🔴 **Vietata la grammatica `Dash`** (§5, §7): la parola nel nome porta dritto al glifo della **fase** Dash, con cui non ha nulla a che vedere.

### Warning — 13 chiavi
Regola di categoria: il modificatore `Invalid` (§6: slash / cross-hatch / `⊘`, neutro muted) si applica sopra una primitiva che nomina la **causa**; il colore non porta il significato da solo (HUD §47-bis.1). Nessuna di queste ha oggi un widget: la resa attuale è testo su `ARTHUD` o riga di combat log — le chiavi sono corrette e disegnabili, il consumatore va scritto.

1. **`UI.Icon.Warning.Cooldown`** — mancano N turni di ricarica. `ERTActionInvalidReason::OnCooldown`, primo motivo per precedenza in `ValidatePlan`; già a schermo come «(ricarica N)».
 **G:** quadrante/arco di **tempo**. Distinta dall'overlay numerico che lo slot già disegna (se ripete il numero è la stessa cosa due volte) e da `SlotOccupied`: qui è **tempo** (aspetta), là è **economia** (scegli).
2. **`UI.Icon.Warning.OutOfMoveBudget`** — la cella va bene, i passi che restano no. `RTPlayerController.cpp:889-899`. ⚠️ Non usa `InsufficientMovementPoints`, che D-190 ha lasciato senza produttore.
 **G:** `Move` (§5, con i nodi intermedi) **troncato** + il residuo mancante. Distinta da `OutOfRange` (distanza di **tiro**, primitiva `Circle`) e da `PathBlocked` (togli un waypoint contro cambia strada). Se non porta il «quanto ti manca», l'overlay delle celle raggiungibili risponde già alla stessa domanda e l'avviso è rumore.
3. **`UI.Icon.Warning.PathBlocked`** — cella non percorribile: ostacolo, o non esiste sulla mappa. `ERTHexWaypointReason::NotOnMap`/`BlocksMovement`.
 **G:** `Cell` + `Invalid`. Distinta da `CellOccupied` (un occupante può spostarsi, un ostacolo no — il codice le tiene apposta separate: «il motivo GIUSTO, non un elenco di tre») e da `PathInvalidated` (stesso aspetto, momento diverso).
4. **`UI.Icon.Warning.OutOfRange`** — oltre la portata dichiarata: avvicinati. `ERTActionInvalidReason::OutOfRange`, `ERTHexTargetReason::OutOfRange`.
 **G:** `Circle` (§3: anello esterno + punto centrale) col bersaglio fuori. 🔴 Distinta da `NoLineOfSight`: è la coppia più pericolosa della categoria perché suggeriscono correzioni **opposte** — avvicinarsi contro spostarsi di lato. Il commento del codice lo dice già: confonderle «rende il log una bugia».
5. **`UI.Icon.Warning.NoLineOfSight`** — sei in portata, ma c'è qualcosa in mezzo. `ERTActionInvalidReason::NoLineOfSight`, `ERTCombatOutcome::NoLineOfSight`.
 **G:** `Line` (§3) interrotta a metà da un `Interrupt` (§4). L'avviso è la **linea spezzata**, non l'ostacolo: se sembra l'ostacolo, duplica le icone `Environment`/`MapInteraction` che ne sono la causa.
6. **`UI.Icon.Warning.SlotOccupied`** — due voci del tuo piano vogliono lo stesso slot. `ERTActionInvalidReason::SlotOccupied`, prodotto in tre punti di `ValidatePlan` e cablato al lock-in; faccia in risoluzione `SupersededByDash`. Cuore di CP 38.2.
 **G:** ⚠️ **l'unica warning che parla di una relazione fra due voci del piano**, non di un'azione difettosa: un glifo «azione barrata» direbbe la cosa sbagliata. Serve una marca di **collisione fra due contenitori**. Distinta dalle icone `Action.*` (non è una terza azione) e da `Cooldown`.
7. **`UI.Icon.Warning.CellOccupied`** — c'è già qualcun altro lì. `ERTHexWaypointReason::Occupied`; in risoluzione `BlockedByUnit`.
 **G:** `Cell` **presa**. Distinta da `Identity.Ally`/`Enemy`: l'occupante è già disegnato su quella cella, quindi un glifo «c'è un'unità» è ridondante — l'avviso dice che **la cella** è presa, non che c'è qualcuno.
8. **`UI.Icon.Warning.FriendlyFire`** — un alleato è dentro l'area del colpo che stai pianificando. `FRTActionDef::bFriendlyFire`, `SetPreviewHitCells(Hit, Allies)`, riga «N ALLEATO NELLA ZONA» già a schermo.
 **G:** `Ally` (§2, rounded + connection tabs) dentro un'area, con `Invalid` sull'**area**, non sull'alleato. Distinta da `TargetFriendly`: qui il bersaglio è legittimo e l'alleato è un danno collaterale **che avverrà**; là l'alleato **è** il bersaglio e l'azione non parte. Se si somigliano, il giocatore corregge la cosa sbagliata.
9. **`UI.Icon.Warning.Collision`** — due movimenti simultanei si sono contesi la stessa cosa. `BlockedContested`, `BlockedByPriority`, `BlockedByImpact`, più `OpposingForces` e `ContestedDestination`.
 **G:** due `Move` che convergono su una `Cell`, con `Interrupt` nel punto d'incontro. Distinta da `CellOccupied` (là c'era qualcuno **fermo**; qui due unità sono arrivate insieme — la differenza è la simultaneità, cioè il pilastro del gioco) e da `Phase.Move` se usa frecce.
10. **`UI.Icon.Warning.TargetLost`** — al momento della risoluzione il bersaglio non c'era più. `TargetGone` + `TargetDead`.
 **G:** `Enemy` (§2, angular + reticolo minimo) svuotato: **assenza avvenuta**, non incertezza. Distinta da `TargetUnknown` («per te non c'è mai stato») e dai marker `Information` (che dicono dov'era, non che non c'è più).
11. **`UI.Icon.Warning.PathInvalidated`** — il percorso non esiste più: la mappa è cambiata dopo il commit. `ERTMoveOutcome::BlockedByTopology` (CP 9.3), scritto dal chiamante.
 **G:** `Move` + `Interrupt` con una marca di **tempo** («era valido, non lo è più»). Distinta da `PathBlocked` (prima contro dopo il commit) e dalle icone `MapInteraction.Door`/`Arc`, che ne sono la **causa** e non l'avviso.
12. **`UI.Icon.Warning.TargetFriendly`** — l'azione offensiva punta a un membro della tua squadra: non parte. `ERTActionInvalidReason::TargetFriendly`.
 **G:** `Ally` + `Invalid`. Deve contenere il **segno di alleato**, non un divieto generico: senza, in monocromia è indistinguibile da `TargetLost` e `OutOfRange`.
13. **`UI.Icon.Warning.TargetUnknown`** — non è geometria: quel bersaglio la tua squadra non lo conosce. `RTActionFallbackLibrary.h:43`, prodotto in `RTTurnManager_Blast.cpp:430` e `:436` (CP 13.2); escluso dal degrado ad `AttackCell`.
 **G:** `Enemy` mai risolto + `?` **come marca, non come stile**. Distinta da `Certainty.Uncertain`, che è un modificatore di stile (§6) e non un'icona, e da `Information.CellOnly`, che è un'opzione degradata e non un rifiuto.

---

## 3. Categorie che restano vuote, dichiarate

- **Coordination — zero chiavi.** In v0.1 `Format.Skirmish2v2` è offline contro bot, il team 0 è interamente umano e D-155 fissa `UnitsPerTeam = UnitsPerPlayer = 2`: **una sola persona per squadra**. La categoria è «comunicazione fra alleati» (`RTIconCatalogData.h:37`): manca il secondo interlocutore, quindi manca il soggetto, non l'icona. Le due candidate (`Ready`/`Unready`) sono in §4.
- **Objective — zero chiavi.** In partita non esiste nessun obiettivo: zero entità, zero stato di possesso, e l'unico chiamante di `AddTeamScore` in tutto il repository è un test (`RTMatchFormatWorldTests.cpp:196`). L'harness lo dichiara già capability **non disponibile**, owner `#75` (`RTScenarioSession.cpp:258`), e due scenari versionati escono `BLOCKED`. I sei stati censiti sono in §4, e si sbloccano con **CP 10.2**, non con CP 10.1.

---

## 4. Non provate o bloccate — non si disegnano, e la domanda da porre

Venti chiavi. Nessuna entra nel set di §2 finché una persona non risponde.

**MapInteraction (2)**
- `Door.Locked` — la regola esiste davvero (`Locked → Open` rifiutata, `RTHexDoorLibrary.cpp:47`) e il mondo 3D **non** la distingue da `Closed`, quindi l'icona sarebbe l'unico canale fra «apri» e «non ci riuscirai». Ma nessun produttore runtime e **nessuna arena v0.1 la autora**. → *Domanda all'owner delle mappe: un'arena v0.1 scrive `Locked`? Se sì rientra subito; se no aspetta CP 10.1.* Nota per il committente: il lucchetto è rivendicato anche da `Objective.Locked` e da un futuro rifiuto `Warning` — tre lucchetti in tre categorie sono un lucchetto solo agli occhi del giocatore, e vanno arbitrati **prima** del disegno.
- `Door.Destroyed` — nessuna azione spedita dichiara `SetDoorState(Destroyed)` e le porte non hanno integrità, quindi `Gadget.BreachCharge` **non** le sfonda, malgrado la sua descrizione dica «apre coperture e porte» (`RTCatalogLibrary.cpp:463`). → *Domanda all'owner del catalogo: ha ragione la descrizione (manca l'integrità sulle porte) o il codice (va corretta la stringa)? La risposta decide se si disegna adesso o dopo.*

**Information (4)**
- `Detected` — §25 le assegna «nessun marker». → *Domanda all'owner della HUD: esiste un chip in lista/inspector che la porti? Se no, non si disegna mai — e resta il caso in cui `Certainty.Confirmed` la coprirebbe comunque.*
- `UncertainContact` — oggi l'unico `Uncertain` prodotto è la memoria, con cella esatta: l'area senza cella nasce col rumore. → *Domanda: si aspetta l'emettitore acustico, o si vuole la variante ad area prima che qualcosa la produca?* Se rientra, va deciso **chi porta il `?`** fra questa, `SoundContact` e il renderer `Uncertain`: se lo portano tutti e tre, non distinguono niente.
- `SoundContact` — `FRTNoiseEvent`, `IsAudible` e `PlausibleOriginCells` esistono, ma «nessun sistema di gioco chiama la propagazione e in partita nessuno produce ancora un evento sonoro» (roadmap §E13). → *Domanda: CP 13.5 tira l'HUD acustico dentro la v0.1, o slitta con E25?* ⚠️ Se rientra: l'area è centrata sull'**ascoltatore**, non sulla sorgente — un glifo posato «al centro del rumore» codificherebbe una posizione che nessuno ha.
- `ContactDirection` — DoD di CP 13.4 («l'attacco rivela almeno la direzione») ma non ancora in codice. → *Stessa domanda di sopra.* ⚠️ Vale **solo per l'attacco**: riusare quel chevron per un rumore concederebbe informazione che il gioco rifiuta di dare (`RTAcousticPropagationLibrary.h:154-157`).

**Reaction (6)**
- `Spent` · `NotTriggered` · `Unavailable` — tutte e tre hanno produttori reali e `URTTurnLogLibrary` le distingue in tre stringhe, ma **nessun campo HUD le trasporta**: `FRTPlannedSlotView` ha solo `bOccupied`/`ActionId`/`DisplayName`. → *Due domande in una: (a) il Combat Log porta icone per esito, o resta testo? (b) sono chiavi, o modificatori `Invalid`/`Disabled` (§6) applicati sopra `Armed`?* In più `Unavailable` è per metà `Warning.Cooldown` vista dal lato reazione: se `Warning` riceve un glifo generico di impedimento, questa lo duplica.
- `CounterShot` · `Anchor` · `Cleanse` — moduli veri del catalogo equipaggiamento, ma **nessun eroe del roster v0.1 li porta di default**. → *Domanda: esiste una schermata di loadout in v0.1? Senza, sono tre asset che non possono comparire.* Due trappole di nome da portarsi dietro il giorno che rientrano: `Reaction.Anchor` e `Gadget.Anchor` sono cose diverse con lo stesso nome, e il modulo `Reaction.Cleanse` concede `Action.Purge` perché «`Action.Cleanse` è già un'altra cosa».

**Objective (6)** — `Neutral` · `Capturing` · `Contested` · `Owned.Ally` · `Owned.Enemy` · `Locked`. Materiale per **#75**, non ordine di lavoro. Tre decisioni pendenti: (a) **relativo o assoluto** — `Owned.Ally`/`Enemy` (coerente col catalogo spedito) contro `TeamA`/`TeamB` (coerente con la spec): si sceglie **una volta per entrambe le famiglie**, o l'HUD dirà «Ally» per le unità e «Team0» per gli obiettivi nello stesso pannello; (b) **`Locked` contro `Disabled`** — la spec usa `Disabled` due volte, come stato di possesso e come reason code: prima si sceglie quale; (c) **`Capturing` con quale dato** — il progresso oggi è un intero **per squadra** (`Team0Score`/`Team1Score`), non per obiettivo: o è un glifo binario, o CP 10.2 si porta dietro un campo di progresso per elemento, e quella è una scelta di modello. 🔴 Vincolo che sopravvive a tutte e tre: la parola `Contested` è **già occupata** da `ERTMoveOutcome::BlockedContested`, che nel TurnLog si rende «contesa».

**Coordination (2)** — `Ready` · `Unready`. Il comportamento è specificato (countdown di 3 s annullabile, `spec-durata-partita-e-scala-mappe.md` §7.2) e D-155 nomina `Ready` come stato per persona, ma il lock-in in v0.1 è **immediato**: non esiste un frame in cui la HUD possa dire «pronto», e il test manuale `PIE-V01-READY` è marcato ⏳. → *Domanda all'owner di `spec-durata-partita-e-scala-mappe.md` per il comportamento e all'owner di E25 per il catalogo: si disegnano in anticipo?* Se sì, ne servono **due**, non una — uno stato che si può disfare senza il suo contrario è metà informazione — e il committente accetta un asset senza consumatore più il test da aggiornare nello stesso commit. Vietato in ogni caso il contatore `TEAM READY 1/2`: l'owner lo proibisce e con `UnitsPerPlayer = 2` avrebbe sempre denominatore 1.

---

## 5. Conteggio

| Categoria | Nuove chiavi da disegnare | In sospeso (§4) |
|---|---:|---:|
| Environment | 7 | 0 |
| MapInteraction | 8 | 2 |
| Information | 2 | 4 |
| Reaction | 10 | 6 |
| Coordination | **0** (vuota, dichiarata) | 2 |
| Warning | 13 | 0 |
| Objective | **0** (vuota, dichiarata) | 6 |
| **Totale** | **40** | **20** |

Catalogo a valle: **61 + 40 = 101 chiavi**, su 12 categorie di cui **10 popolate** e 2 dichiarate vuote.

---

## 6. Due cose da fare prima del primo disegno, non dopo

1. **Nessuna di queste 40 è oggi protetta da un gate.** `RequiredIconIds()` deriva solo Phase, Action, Status, Certainty e Identity: una chiave scritta a mano nel data asset con un typo passa il validator (che controlla prefisso, unicità e segmento di categoria) e fallisce solo a runtime. Quando la lista si apre, il ramo va scritto **nello stesso commit** degli asset — o `FindMissingRequiredIcons` cade, o le chiavi restano fuori dal gate. Dove si può, si **deriva** invece di elencare: `ERTHexSurface`, `ERTHexCoverType`, `ERTHexDoorState`, `ERTHexArcState`, `ERTActionInvalidReason` sono sorgenti vive, e una decima superficie chiederebbe la propria icona al compilatore come già fa `SurfaceRingCount` con il suo switch senza default. Ma la derivazione va **filtrata**: `Floor`, `Void`, `Cover.None`, `Arc.Inactive` e `ERTAwareness::Hidden` sono valori che **non si disegnano**, ed è la stessa trappola già documentata per `ERTIntentCertainty::Unknown`. Il ciclo di vita di `Reaction` non ha alcun tipo da cui derivare: sarebbe la **seconda** eccezione a mano dopo `Certainty`, e va dichiarata come tale.
2. **Verifica non fatta: nessuna sessione Unreal, nessuna misura a schermo.** Tutte le collisioni di questo documento sono ricavate da enum, catalogo, test e documenti owner — non da un rendering. La sede della verifica in scala di grigi a 24 px esiste già ed è **PIE-V01-BOARD (#1262)**, su fixture RelayBasin. Le coppie da portarci per prime, in ordine di rischio misurato: `Cover.High ↔ Door.Closed`, `Rough ↔ cella bloccata` (D-183 la misura come la peggiore delle nove), `Reaction.Hold ↔ Reaction.HoldGround`, `Warning.OutOfRange ↔ Warning.NoLineOfSight`, `Conductive ↔ Status.Electrified`.