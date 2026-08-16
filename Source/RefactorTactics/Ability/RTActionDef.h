#pragma once

#include "CoreMinimal.h"
#include "Map/RTHexCellData.h"
#include "GameplayTagContainer.h"
#include "Turn/RTActionEvent.h"
#include "RTActionDef.generated.h"

/**
 * Fase di risoluzione dichiarata da un'azione, con i codici del catalogo v0.1 (0/10/20/30/40/50/60).
 *
 * NON e' la fase in cui l'azione risolve davvero: quella e' una macro-fase di Atlas (`ERTMatchPhase`), e la
 * conversione e' URTCatalogLibrary::MapResolutionPhase. I codici restano perche' sono la chiave di lettura dei
 * due PDF del catalogo, e perche' senza di essi non si potrebbe piu' dire da dove viene un valore.
 *
 * Il codice **20** del catalogo si SDOPPIA: `FastMovement` (Dash/Charge/Leap/Sprint/Reposition -> macro-fase
 * Dash, PRIMA del Blast) e `NormalMovement` (`Action.Move` -> macro-fase Move, DOPO il Blast). E' l'unica
 * divergenza strutturale dal catalogo, decisa in ADR-0003 §3.
 */
UENUM(BlueprintType)
enum class ERTResolutionPhase : uint8
{
	/** 0 — congelamento di stato, intenti e seed: nessuna azione risolve qui. */
	Snapshot,
	/** 10 — scudi, stance, trappole, reazioni preparate. */
	Preparation,
	/** 20 — mobilita' rapida (Dash, Charge, Leap, Sprint, Reposition). */
	FastMovement,
	/** 20 — percorso normale (Action.Move). */
	NormalMovement,
	/** 30 — root, push, interrupt, interposizione. */
	Control,
	/** 40 — attacchi, abilita', cure, interazioni. */
	Attack,
	/** 50 — fuoco, acqua, elettricita' e propagazione. */
	Environment,
	/** 60 — KO, obiettivi, cooldown, TurnLog. */
	Cleanup
};

/**
 * Comportamento dell'azione quando, al momento della risoluzione, non e' piu' eseguibile come pianificata
 * (il bersaglio si e' spostato, il percorso si e' chiuso, la cella e' occupata).
 *
 * Il fallback e' DICHIARATO nel catalogo, mai scelto a runtime: una scelta automatica (es. "il nemico piu'
 * vicino") produce esiti poco leggibili, e la leggibilita' tattica e' un pilastro di prodotto.
 */
UENUM(BlueprintType)
enum class ERTActionFallback : uint8
{
	/** Si ferma all'ultima posizione valida (regola standard del movimento). */
	Stop,
	/** Sostituisce l'azione con un'attesa. */
	Wait,
	/** Colpisce comunque la cella pianificata (AoE). */
	AttackCell,
	/** Segue il bersaglio, se ancora valido. */
	AttackTarget,
	/** Usa l'attacco base sul bersaglio valido piu' vicino. */
	BasicAttack,
	/** Non esegue nulla (attacchi diretti e cure). */
	Cancel
};

/**
 * Slot del turno che un'azione occupa. Il catalogo v0.1 (§«Slot per turno») ne dichiara uno per riga: senza
 * questo dato, «Sprint consuma movimento **e** azione principale» diventerebbe un `if` sull'ActionId dentro
 * l'orchestratore — cioe' il tipo di eccezione hard-coded che il motore azioni esiste per togliere.
 */
UENUM(BlueprintType)
enum class ERTActionSlot : uint8
{
	/** Non occupa slot: resta osservabile nel TurnLog ma non toglie nulla al piano (`Action.Wait`). */
	None,
	/** Slot movimento (`Action.Move`). */
	Movement,
	/** Azione principale: attacchi, scatti, guardia, cure. E' il caso comune. */
	Main,
	/** Consuma ENTRAMBI gli slot: chi la usa non si muove oltre e non agisce (`Action.Sprint`). */
	MovementAndMain,
	/**
	 * Slot reazione (CP 5.1, epic E5): 0-1 per turno, indipendente da Movimento e Principale — un eroe puo'
	 * muoversi, agire E tenere una reazione pronta nello stesso turno. Si dichiara in planning come le altre
	 * azioni; a differenza loro non ha una cella o un bersaglio scelti dal giocatore, ma un `ReactionTrigger`
	 * valutato deterministicamente sullo snapshot del Blast.
	 */
	Reaction
};

/**
 * Condizione che fa scattare una reazione (CP 5.1), valutata su cio' che la fase ha GIA' raccolto o prodotto:
 * mai un `Delay`, una timeline o un montage nel resolver (invariante #3) — e' per questo che il trigger e' un
 * dato puro da confrontare con un array, non un evento a cui reagire mentre il turno gira.
 *
 * **I trigger non si valutano tutti nello stesso momento** (D-092): quelli che nascono da un colpo si leggono
 * su `FRTHexBlastPlan::Hits` dopo il filtro di `Action.Interrupt`, quelli che nascono da un evento della fase
 * (una spinta, uno stato, una cella diventata pericolosa) si leggono dove quell'evento e' deciso. A dire quale
 * momento spetta a ciascuno e' `URTReactionLibrary::PassPointFor`, funzione pura: un trigger che non
 * dichiarasse il proprio punto non verrebbe valutato da nessuno e sarebbe un dato senza consumatore.
 */
UENUM(BlueprintType)
enum class ERTReactionTrigger : uint8
{
	/** Nessun trigger dichiarato: non e' una reazione, o non ne ha ancora uno (dato incompleto). */
	None,
	/**
	 * L'unita' e' bersaglio di almeno un colpo DIRETTO (`ERTAbilityShape::Single`) andato a segno in questo
	 * Blast (`Action.Counter`, `Action.Deflect` — CP 5.2 aggiunge i loro effetti sulla stessa valutazione).
	 */
	HitByDirectAttack,

	/**
	 * Un ALLEATO entro la portata dell'azione e' bersaglio di un colpo diretto (`Action.Intercept`, CP 5.3).
	 *
	 * A differenza di `HitByDirectAttack` non basta guardare i colpi: servono squadre, posizioni e mappa (la
	 * traiettoria dall'attaccante all'intercettore dev'essere libera). Per questo ha un punto d'ingresso suo,
	 * `URTReactionLibrary::FindInterceptableHit`, invece di passare da `FindTriggeringAttacker`.
	 */
	AllyHitByDirectAttack,

	/**
	 * L'unita' sta per essere SPOSTATA da una spinta o da una trazione (`Reaction.Anchor`, CP 7.5 `#505`).
	 *
	 * Non nasce da un colpo ma da un EVENTO gia' prodotto dalla fase, e si valuta nel momento in cui lo
	 * spostamento e' deciso e non ancora applicato: dopo, annullarlo vorrebbe dire rimettere indietro
	 * un'unita' gia' mossa, e il TurnLog avrebbe due voci contraddittorie per lo stesso passo.
	 *
	 * Aggiunto in CODA all'enum: i valori precedenti non si rinumerano, e il trigger finisce nel TurnLog
	 * serializzato — la stessa disciplina di `ERTActionEffect::SelfReposition`.
	 */
	AboutToBeDisplaced,

	/**
	 * L'unita' sta per ricevere uno stato di CONTROLLO (`Reaction.Cleanse`, CP 7.5 `#505`).
	 *
	 * Come `AboutToBeDisplaced` nasce da un evento gia' prodotto dalla fase e si valuta prima che venga
	 * applicato: annullare dopo vorrebbe dire togliere uno stato appena messo, e il TurnLog racconterebbe due
	 * volte lo stesso turno.
	 *
	 * ⚠️ Scatta solo per un controllo che **cambierebbe qualcosa**: chi e' gia' sotto quello stato non spende
	 * l'attivazione per un rinnovo. Il limite dichiarato che ne segue e' che il prolungamento di un controllo
	 * gia' attivo non e' intercettabile — deciso in sessione il 2026-08-12, insieme al criterio di scelta.
	 *
	 * Quali stati siano «di controllo», e in che ordine di gravita', lo dice
	 * `URTReactionLibrary::ControlStatusesBySeverity`.
	 */
	AboutToReceiveControl,

	/**
	 * La cella sotto l'unita' e' appena diventata PERICOLOSA (`Reaction.HazardEscape`, CP 7.5 `#505`).
	 *
	 * L'ultimo dei tre trigger su evento, e l'unico che non nasce nel Blast: le superfici nascono nel Cleanup,
	 * e si valuta fra la loro nascita e il momento in cui i loro effetti toccano chi c'e' sopra. Un istante
	 * prima non c'e' ancora niente di pericoloso; uno dopo l'unita' ha gia' `Burning` addosso e fuggire non
	 * glielo toglie — la fuga sarebbe teatro.
	 *
	 * Ha potuto esistere solo dopo [#570]: finche' una superficie che nasceva sotto un'unita' ferma non le
	 * faceva niente, non c'era alcun danno imminente da evitare e il modulo sarebbe stato inerte.
	 */
	CellBecameHazardous
};

/**
 * Che cosa un'azione fa a una copertura di bordo (CP 9.5). Dato del catalogo, non un ramo nell'orchestratore:
 * la stessa operazione appartiene all'azione core, a un'abilita' d'eroe e a un gadget, e i tre non devono
 * diventare tre `if` sull'ActionId.
 */
UENUM(BlueprintType)
enum class ERTStructureOp : uint8
{
	/** L'azione non tocca le strutture di bordo. */
	None,
	/** Erige una copertura bassa temporanea sul bordo dichiarato dal piano. */
	CreateCover,
	/** Sposta una copertura gia' esistente su un altro bordo, conservandone integrita' e durata residua. */
	MoveCover
};

/**
 * COME si sposta un'azione di mobilita'. E' un dato del catalogo, non un ramo nell'orchestratore: `Dash` e
 * `Sprint` risolvono nella stessa macro-fase ma si muovono in due modi diversi, e senza questo campo la
 * differenza finirebbe in un `if` sull'ActionId.
 */
UENUM(BlueprintType)
enum class ERTMovementStyle : uint8
{
	/** L'azione non sposta chi la usa (attacchi, guardia, interazioni). */
	None,
	/** Percorso a costi interi dentro un budget di MP, ostacoli aggirati in pianificazione (`Action.Sprint`). */
	Budget,
	/** Linea retta su una delle sei direzioni: si ferma davanti a muri e unita' (`Dash`, `Reposition`). */
	LinearDash,
	/** Come `LinearDash`, ma si ferma SUL primo nemico incontrato e lo colpisce (`Charge`). */
	LinearCharge,
	/** Salto: ignora unita' e celle intermedie, conta solo dove si atterra (`Leap`). */
	LinearLeap,
	/**
	 * ATTRAVERSA le unita' sulla traiettoria e le colpisce, poi prosegue (`Wraith.PassingBlade`).
	 *
	 * La differenza con `LinearLeap` non e' il danno ma cosa si tocca: il salto **scavalca** e non incontra
	 * nessuno, la lama passa **in mezzo** e applica a ognuno gli effetti dell'azione. Con `LinearCharge`
	 * condivide il colpire, ma la carica si ferma sul primo bersaglio mentre questa tira dritto.
	 *
	 * Aggiunto IN CODA: i valori precedenti non cambiano numero, e gli asset che li hanno serializzati
	 * continuano a rileggersi.
	 */
	LinearPass
};

/**
 * COME un'azione **predittiva** sceglie il proprio bersaglio (E18 CP 18.1, [D-016]). E' un dato del catalogo
 * per la stessa ragione di `MovementStyle`: la thin slice ne usa una sola forma, ma il giorno in cui una
 * seconda azione prevede un BORDO invece di una cella, la differenza dev'essere un valore e non un `if`
 * sull'ActionId dentro il resolver.
 */
UENUM(BlueprintType)
enum class ERTPredictiveTargeting : uint8
{
	/** L'azione non e' predittiva: risolve su cio' che trova, come tutte le altre. */
	None,
	/**
	 * Una CELLA dichiarata in Planning e **bloccata**: non viene mai rivalutata al momento della risoluzione.
	 *
	 * Il blocco e' l'intera meccanica, non un dettaglio d'implementazione. Un'azione che al boundary cercasse
	 * il bersaglio piu' vicino colpirebbe sempre, e non sarebbe piu' una previsione ma un ordine: la
	 * scommessa — e quindi il counterplay — sparirebbe insieme alla possibilita' di sbagliarla.
	 */
	LockCell
};

/**
 * QUANDO un'azione predittiva verifica la propria previsione (E18 CP 18.1).
 *
 * Il boundary e' **deterministico e senza attesa**: non e' una finestra in cui qualcuno decide, e' un punto
 * dell'ordine di risoluzione in cui si guarda cos'e' successo. E' la linea che separa E18 da E14 — la Fast
 * Reaction chiede, la Predictive Action ha gia' chiesto tutto in Planning.
 */
UENUM(BlueprintType)
enum class ERTPredictionBoundary : uint8
{
	/** Nessun boundary: l'azione non e' predittiva. */
	None,
	/**
	 * Fine del calcolo delle rotte del Move, prima che le posizioni finali vengano applicate: si guarda chi ha
	 * ATTRAVERSATO la cella bloccata, non chi ci e' rimasto sopra.
	 *
	 * Attraversare e restare sono due cose diverse, e la differenza e' osservabile: un'unita' che passa sulla
	 * cella e prosegue verrebbe mancata da un controllo sulla sola posizione finale. Il boundary sta **dopo**
	 * `ResolveHexPaths` e **prima** del piazzamento proprio perche' li' esistono entrambe le informazioni.
	 */
	MovementEntry
};

/**
 * Definizione di un'azione del catalogo v0.1: la parte di dati comune a ogni azione, indipendente dai suoi
 * effetti. Solo INTERI (invariante #4): niente float in costi, priorita', portata o cooldown — il test
 * `RefactorTactics.Catalog.NoFloatInIntegerFields` lo verifica per reflection, non a occhio.
 *
 * Riferimento: docs/balance/RT_ActionCatalog_v0.1.md
 */
USTRUCT(BlueprintType)
struct FRTActionDef
{
	GENERATED_BODY()

	/** ID stabile del catalogo (es. `Action.Move`). Chiave del data asset e del TurnLog: non cambia mai. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FName ActionId;

	/**
	 * Di quale AZIONE GENERICA questa e' un profilo (es. `Riktor.ImpactShot` -> `Action.BasicAttack`).
	 * `None` per le azioni che non sono profilo di niente — le generiche stesse, e le abilita' firma.
	 *
	 * Esiste perche' [D-033](../../../docs/decisions/RT_PDR_00_Decision_Log.md) chiede che un'azione generica
	 * con profilo sia spiegabile **nel TurnLog** come *azione base + profilo*, e senza questo campo la
	 * relazione non e' scritta da nessuna parte: `Riktor.ImpactShot` e' un'azione d'EROE, quindi un lettore
	 * della traccia non la risolve nemmeno consultando il catalogo core.
	 *
	 * E' un dato e non una deduzione: dedurlo dal nome (`Hero.Qualcosa` = attacco base?) funzionerebbe finche'
	 * un eroe non chiama diversamente la sua azione, cioe' fino al primo eroe nuovo.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FName BaseActionId;

	/** Fase dichiarata (codice del catalogo); la macro-fase reale viene da URTCatalogLibrary::MapResolutionPhase. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	ERTResolutionPhase ResolutionPhase = ERTResolutionPhase::Attack;

	/** Priorita' intera intra-fase: valore MINORE risolve prima. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 Priority = 50;

	/** Portata in celle esagonali (0 = su se stessi). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 RangeCells = 0;

	/** Costo in punti movimento (0 = nessun costo di movimento). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 CostMP = 0;

	/** Ricarica in turni completi. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 CooldownTurns = 0;

	/** Cosa succede se l'azione non e' piu' eseguibile come pianificata. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	ERTActionFallback Fallback = ERTActionFallback::Cancel;

	/**
	 * Slot del turno consumato dall'azione. Default `Main`: e' lo slot della maggior parte delle azioni
	 * (attacchi, scatti, guardia) e coincide con quello delle abilita' gia' spedite.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	ERTActionSlot Slot = ERTActionSlot::Main;

	/** Come l'azione sposta chi la usa. `None` per tutto cio' che non e' mobilita'. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	ERTMovementStyle MovementStyle = ERTMovementStyle::None;

	/**
	 * Che cosa l'azione fa a una STRUTTURA di bordo (CP 9.5). `None` per tutto il resto.
	 *
	 * Esiste per la stessa ragione di `MovementStyle`, e la sua assenza si sarebbe pagata subito: erigere una
	 * copertura e' semantica di **tre** identita' diverse — l'azione core, l'abilita' di Riktor e il gadget
	 * portatile — e senza un dato il resolver avrebbe tre `if` sull'ActionId, cioe' un ramo per eroe nel core.
	 * `Ignite`, `CreateWater` ed `Electrify` sono ancora riconosciute per ActionId: la' i produttori sono uno
	 * ciascuno, e il campo si aggiungera' quando smetteranno di esserlo.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	ERTStructureOp StructureOp = ERTStructureOp::None;

	/**
	 * Limite di propagazione ambientale in celle: **0 = non propaga**, N > 0 = si ferma a N celle.
	 * Un valore negativo significherebbe "senza limite" ed e' rifiutato dal validator: una propagazione
	 * illimitata su una mappa d'acqua colpisce tutti e rende il turno impredicibile (errore da evitare
	 * elencato dal catalogo).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 PropagationLimit = 0;

	/**
	 * L'azione TRASFORMA la superficie della cella bersaglio (`Action.Ignite` -> Fire,
	 * `Action.CreateWater` -> ShallowWater).
	 *
	 * Coppia flag+valore come `bTargetsCell`/`TargetCell`: l'enum delle superfici non ha un valore «nessuna»,
	 * e `Floor` e' una superficie legittima, quindi non puo' fare da «non dichiarato».
	 *
	 * Prima il resolver sceglieva la superficie confrontando l'ActionId letterale
	 * (`if (Id == "Action.CreateWater")`). Il commento di allora lo ammetteva e rimandava: «inventare un campo
	 * SurfaceCreated per due sole azioni sarebbe un dato che nessun'altra azione userebbe; quando le azioni
	 * ambientali saranno molte, il posto giusto e' quel campo». La condizione e' arrivata da un'altra
	 * direzione: con D-046 un EROE possiede un'azione ambientale, e `Phase.FluidTrail` non puo' chiamarsi
	 * `Action.CreateWater`. Un confronto per nome non sa esprimere «e' quell'azione con un nome d'eroe» —
	 * un campo si', ed e' la stessa strada di `PropagationLimit`, che infatti funzionava gia'.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	bool bCreatesSurface = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	ERTHexSurface SurfaceCreated = ERTHexSurface::Floor;

	/**
	 * Raggio dell'area DIPINTA attorno alla cella bersaglio: 0 = la sola cella, 1 = l'esagono pieno di raggio 1.
	 *
	 * E' un asse diverso da `URTActionData::AreaRadius`, che e' l'area degli EFFETTI sulle unita': `CreateWater`
	 * non colpisce nessuno — forma `Single` — e allaga comunque sette celle. Tenerli separati costa un campo;
	 * unificarli costerebbe la prima azione che danneggia un raggio e ne allaga un altro.
	 *
	 * Nasce con lo stesso argomento di `SurfaceCreated` (D-046): il resolver cablava
	 * `(Created == ShallowWater) ? 1 : 0`, cioe' proprio il ramo che il commento accanto dichiarava di voler
	 * evitare. Finche' i produttori erano due il ramo reggeva; con `Phase.MistVeil` (issue #353) i raggi
	 * dichiarati diventano tre e il ramo dovrebbe indovinare quale superficie vuole quale area.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 SurfaceRadius = 0;

	/**
	 * Effetti prodotti dall'azione, nell'ordine in cui si applicano. E' il campo che il registry traduce in
	 * eventi: cambiare cosa fa un'azione significa cambiare QUESTO, non aggiungere un ramo nell'orchestratore.
	 *
	 * E' una LISTA perche' molte azioni combinano piu' effetti (la Spazzata infligge danno **e** respinge).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	TArray<FRTActionEffectSpec> Effects;

	/**
	 * Se falso, chi usa questa azione NON puo' tenere pronta una reazione in questo turno (`Action.Sprint`:
	 * chi corre a perdifiato non para). Fatta valere da `ARTTurnManager::ResolveCombat` (CP 5.1): una reazione
	 * pianificata insieme a un'azione con questo campo a falso finisce nel TurnLog come non disponibile.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	bool bAllowsReaction = true;

	/**
	 * Condizione che fa scattare QUESTA azione, se `Slot == Reaction` (CP 5.1). `None` per tutto cio' che
	 * reazione non e'.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	ERTReactionTrigger ReactionTrigger = ERTReactionTrigger::None;

	/** Se falso, `Action.Interrupt` non ha effetto su questa azione. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	bool bCanBeInterrupted = true;

	/**
	 * L'azione si applica a CHI LA USA: nessun bersaglio da scegliere, nessuna portata da validare.
	 *
	 * Lo dichiara il catalogo perche' e' una proprieta' dell'azione, non una deduzione: `Action.Guard` e
	 * `Action.Brace` hanno gia' `Range (self) 0` e applicano il proprio stato a chi le pianifica, ma finche'
	 * il flag non esisteva nel `Def` nessuno dei tre consumatori lo sapeva — il giocatore chiedeva un
	 * bersaglio per un'azione che non ne ha, il bot le valutava fra le candidate d'ATTACCO (con `Power` 0),
	 * e il ramo «se ferito usa un supporto» non trovava mai niente da usare.
	 *
	 * Dedurlo da `RangeCells == 0` sarebbe stato sbagliato: anche `Action.Wait` ha portata 0 e non e' un
	 * supporto su di se'.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	bool bSelfTarget = false;

	/**
	 * L'azione colpisce anche gli ALLEATI di chi la usa. **Default vero** (decisione dell'autore, 2026-08-08):
	 * il fuoco amico e' attivo di base, non l'eccezione di una singola azione. Chi piazza un'area lo fa
	 * sapendo dove sono i suoi.
	 *
	 * Il default era falso e nessun eroe lo ribaltava — `MakeHeroAction` non aveva nemmeno il parametro —
	 * quindi il `bFriendlyFire = true` di `Action.CircularAoE` non raggiungeva il roster: in partita non si
	 * attivava mai. Invertire il default e' il modo che NON lascia scoperto un eroe futuro; una lista di
	 * azioni da marcare a mano si dimentica, un default no.
	 *
	 * Resta un `UPROPERTY` per azione perche' una singola azione possa dichiarare il contrario (una cura ad
	 * area che non deve colpire i nemici, o viceversa), non perche' sia normale doverlo impostare.
	 *
	 * Sta nei DATI e non nel codice che costruisce l'intento: altrimenti servirebbe un
	 * `if (ActionId == ...)`, cioe' l'eccezione hard-coded che il motore azioni esiste per togliere.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	bool bFriendlyFire = true;

	/**
	 * Come l'azione sceglie il bersaglio della propria PREVISIONE (E18 CP 18.1). `None` per tutto il resto,
	 * che e' quasi tutto il catalogo.
	 *
	 * Sta qui e non in un secondo sistema perche' una Predictive Action non ha bisogno di un motore proprio:
	 * la coda azioni esistente la ordina come qualunque altra, e il fallback usa `ERTActionFallback`, che c'e'
	 * gia'. Un sottosistema parallelo avrebbe dovuto reimplementare priorita', slot e interruzioni — e sarebbe
	 * divergito al primo cambiamento.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	ERTPredictiveTargeting PredictiveTargeting = ERTPredictiveTargeting::None;

	/**
	 * A quale boundary la previsione viene verificata. `None` quando l'azione non e' predittiva.
	 *
	 * Due campi e non uno perche' sono due assi indipendenti: COSA si blocca e QUANDO si guarda. Un'azione
	 * futura potra' bloccare una cella e risolvere al Blast invece che al Move, e la coppia lo esprime senza
	 * aggiungere un valore combinato per ogni incrocio.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	ERTPredictionBoundary PredictionBoundary = ERTPredictionBoundary::None;

	FRTActionDef() = default;
};
