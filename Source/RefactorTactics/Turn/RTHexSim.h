#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "Perception/RTTeamKnowledge.h" // FRTTeamKnowledge: la conoscenza di squadra viaggia nello snapshot
#include "Turn/RTTurnLog.h"
#include "RTHexSim.generated.h"

class URTHexMapAsset;

/**
 * Stato minimo di un'unita' per la simulazione esagonale di un turno. L'identita' e' un INTERO STABILE
 * (UnitId), mai un pointer: e' la stessa disciplina del TurnLog (determinismo/replay).
 */
USTRUCT(BlueprintType)
struct FRTHexSimUnit
{
	GENERATED_BODY()

	/** Identita' stabile dell'unita' nel turno (chiave di occupazione e di risultato). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	int32 UnitId = 0;

	/** Posizione autorevole a inizio fase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	FRTCellId Cell;

	/** Le unita' non vive non occupano celle e non partecipano al movimento. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	bool bAlive = true;

	/** Costo massimo (intero) spendibile nel turno per il movimento; 0 = immobile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	int32 MoveBudget = 0;

	/**
	 * **Passi**: quante celle si possono attraversare nel turno, una per cella ([D-117] voce 1, `#653`).
	 *
	 * ⚠️ **Oggi vale sempre quanto `MoveBudget`, e nessun consumatore lo legge ancora.** Le due misure —
	 * quanto lontano arrivi, quanta asperita' assorbi — coincidono finche' ogni cella costa `1`, ed e' cio'
	 * che il pathfinding fa in questo istante. A separarle e' la funzione di costo di [D-117] voce 2
	 * (`max(0, MoveCost - 1 + MoveCostModifier)`), che e' [#666] e dipende da questo checkpoint.
	 *
	 * 🔑 **Il default lo popola il costruttore col valore di `MoveBudget`, e non e' una comodita'.** Un
	 * default `0` renderebbe immobile ogni unita' costruita da un chiamante che non conosce questo campo —
	 * test, harness, bot — nel momento stesso in cui [#666] gli dara' un lettore. Il valore che conserva il
	 * comportamento e' «le due misure coincidono», non «zero passi». Stessa disciplina con cui `TeamId`
	 * vale `INDEX_NONE` e non `0` qui sotto.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	int32 StepBudget = 0;

	/**
	 * Sovrapprezzo (intero, >= 0) aggiunto al costo di OGNI cella attraversata, per QUESTA unita' (`Action.Slow`,
	 * CP 4.7): 0 = nessun sovrapprezzo. E' un costo di pathfinding, non una riduzione del budget totale — la
	 * differenza conta perche' rende piu' cara la strada lunga senza rendere impossibile quella corta,
	 * mentre dimezzare il budget penalizza allo stesso modo un passo e dieci.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	int32 MoveCostModifier = 0;

	/**
	 * Celle di scivolamento AGGIUNTIVE per questa unita', oltre a quelle che il terreno dichiara
	 * (`FRTTerrainDef::SlideCells`). Oggi il solo produttore e' `Status.Unbalanced`, che vale `1`
	 * ([D-319]): chi scivola mentre e' gia' sbilanciato percorre due celle invece di una.
	 *
	 * 🔑 **Un numero e non un `bool bUnbalanced`, e il precedente e' `MoveCostModifier` due righe sopra.**
	 * Lo strato esagonale e' PURO: non conosce `ARTUnit`, non conosce i Gameplay Tag, e non deve
	 * cominciare — `Action.Slow` entra qui come sovrapprezzo, non come nome di stato, ed e' la stessa
	 * disciplina. Chi legge questo campo non ha bisogno di sapere *perche'* l'unita' scivoli di piu'.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	int32 ExtraSlideCells = 0;

	/**
	 * Orientamento AUTOREVOLE (CP 16.1): non lo yaw della mesh, ma il dato che decide da che lato si e'
	 * scoperti e dove punta un cono. Entra nello snapshot perche' e' stato di gioco, quindi entra anche
	 * nell'hash del replay attraverso le voci di TurnLog che lo registrano.
	 *
	 * Un solo campo, non sei: la timeline di D-020 e' la SEQUENZA delle scritture, e vive nel TurnLog. Tenere
	 * qui i sei momenti significherebbe conservare cinque valori che nessuno rilegge, e doverli invalidare a
	 * ogni cambio. Il valore qui e' sempre «il piu' recente», che e' esattamente cio' che D-020 chiede ai
	 * consumatori di leggere. Il facing di fine round persiste in quello dopo perche' l'unita' lo porta con se'.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	ERTHexDirection Facing = ERTHexDirection::E;

	/**
	 * LA SQUADRA, e serve a una domanda sola: chi si puo' attraversare — `#2984`, [D-396].
	 *
	 * 🔑 **Lo zero e' `INDEX_NONE` e non `0`, e la differenza e' il comportamento di chi non lo
	 * compila.** Con `0` come default ogni unita' costruita prima di questo campo diventerebbe compagna
	 * di tutte le altre, e l'attraversamento si aprirebbe ovunque in silenzio — nei test, negli harness,
	 * in ogni chiamante che non sa di doverlo dichiarare. Con `INDEX_NONE` nessuna coppia e' alleata
	 * finche' qualcuno non lo dice, quindi il comportamento resta **identico** a prima.
	 *
	 * ⚠️ **E' un NUMERO, non una fazione**, per la stessa disciplina di `MoveCostModifier` e
	 * `ExtraSlideCells` qui sopra: lo strato esagonale non conosce `ARTUnit`, e la traduzione avviene
	 * una volta sola, in `ARTTurnManager::MakeSimUnit`.
	 *
	 * ⛔ **NON entra in nessun hash, e non e' fortuna: e' il tipo.** `URTMatchStateHashLibrary::HashMatchState`
	 * legge `FRTUnitStateDigest`, che `BuildUnitDigests` costruisce da `ARTUnit` — questo struct non lo
	 * attraversa mai. E `FRTHexSnapshot` dichiara di se' che *«NON va conservata oltre la fase che la
	 * produce»*. 🔑 Il valore qui e' una COPIA transitoria di `ARTUnit::TeamId`, che esisteva gia': questo
	 * campo non aggiunge stato competitivo, quindi replay e `StateHash` non si muovono. Stessa disciplina
	 * con cui `FRTHexCover::bGenerated` dichiara di restare fuori dall'hash.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	int32 TeamId = INDEX_NONE;

	FRTHexSimUnit() = default;
	FRTHexSimUnit(int32 InUnitId, const FRTCellId& InCell, int32 InMoveBudget = 0, bool bInAlive = true)
		: UnitId(InUnitId), Cell(InCell), bAlive(bInAlive), MoveBudget(InMoveBudget),
		  StepBudget(InMoveBudget) {}
};

/** Cella raggiungibile entro il budget, col costo cumulato dalla partenza. */
USTRUCT(BlueprintType)
struct FRTHexReachableCell
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	FRTCellId Cell;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	int32 Cost = 0;

	/**
	 * Cella da cui si ARRIVA su questa: il predecessore nel percorso piu' economico. Per la cella di partenza
	 * vale la cella stessa.
	 *
	 * Esiste per il FACING (CP 13.5): l'orientamento deriva dall'ultimo passo, e chi valuta una destinazione
	 * deve poter sapere da che parte ci arrivera' senza rifare il pathfinding per ogni candidata. La ricerca
	 * che produce queste celle il predecessore lo conosce gia' mentre espande — qui si smette di buttarlo via.
	 *
	 * ⚠️ E' il predecessore del percorso che la ricerca ha scelto, non «un» percorso qualsiasi: se un giorno
	 * la ricerca cambiasse criterio a parita' di costo, questo campo cambierebbe con lei. E' voluto — il facing
	 * deve seguire il percorso che il resolver poi costruira', non uno alternativo.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	FRTCellId FromCell;

	FRTHexReachableCell() = default;
	FRTHexReachableCell(const FRTCellId& InCell, int32 InCost) : Cell(InCell), Cost(InCost), FromCell(InCell) {}
	FRTHexReachableCell(const FRTCellId& InCell, int32 InCost, const FRTCellId& InFrom)
		: Cell(InCell), Cost(InCost), FromCell(InFrom) {}
};

/**
 * **La cadenza di un profilo nel calendario dei sotto-passi** ([D-428], che chiude `SKB-2`).
 *
 * Un tick contiene `FRTMovementResolutionState::SubStepsPerTick` sotto-passi — **derivati dai
 * partecipanti**, non una costante. Con `k` l'indice di calendario,
 * `s = k % SubStepsPerTick` e `t = k / SubStepsPerTick`, un'unita' e' eleggibile quando:
 *
 * ```text
 * s = k % SubStepsPerTick,  t = k / SubStepsPerTick
 * eleggibile  ⇔  s < StepsPerTick  &&  (t % TickPeriod) == 0
 * ```
 *
 * 🔑 **E' una funzione PURA di `(cadenza, indice)`**: nessuno stato per unita', nessun contatore che
 * dipenda da quando l'unita' entra nella risoluzione, nessuna iterazione di Actor o di `TMap`. E' cio'
 * che rende l'ordine deterministico senza violare [D-293].
 *
 * ⛔ **Un record e non due array paralleli.** I due numeri descrivono UNA regola, e tenerli separati
 * sarebbe un'occasione di scriverne uno solo — la stessa classe di difetto che `FRTCounterAttack` e
 * `FRTDisplacementCause` esistono per togliere di mezzo.
 *
 * ⚠️ **`StepsPerTick = 0` significa «non avanza mai»**, ed e' rappresentabile ma **nessun profilo lo
 * dichiara**: l'immobilita' di `MovementProfile.Still` viene dal non avere un percorso, non dalla cadenza.
 * Darglielo congelava un'unita' che il percorso ce l'ha — `ProfileForPlan` ripiega su `Still` quando il
 * catalogo non contiene il profilo nominato, e quel ripiego e' permissivo di proposito. Trovato in code
 * review.
 */
struct FRTMovementCadence
{
	/**
	 * In quanti sotto-passi del tick il profilo puo' avanzare. `Sprint` **2**, `Move` **1**.
	 *
	 * ⛔ **Il minimo e' `1`, e lo `0` NON e' rappresentabile.** Era ammesso — «non avanza mai» — e produceva
	 * un difetto: un'unita' con un percorso vero non diventava mai eleggibile, non raggiungeva mai `Done`,
	 * e usciva col `BlockReason` di default, cioe' *«cella occupata»* per un'unita' che nessuna cella aveva
	 * bloccato. Teneva inoltre `AnyLive` vero per sempre e impediva a chi la seguiva di fissare il proprio
	 * motivo. L'immobilita' si esprime **non avendo un percorso**, non con una cadenza nulla.
	 * Trovato in code review.
	 */
	int32 StepsPerTick = 1;

	/** Ogni quanti tick e' eleggibile. `Sneak` **2** (un passo ogni due tick), gli altri **1**. */
	int32 TickPeriod = 1;

	/**
	 * Il massimo `StepsPerTick` che la risoluzione accetta.
	 *
	 * ⛔ **Sta QUI e non sul profilo di movimento**, perche' il resolver non conosce i profili e non deve
	 * includerne l'header: e' l'intera ragione per cui `Cadences` porta numeri invece di `FRTMovementProfile`.
	 * Il gemello di catalogo e' `FRTMovementProfile::MaxStepsPerTick`, e vincola cosa un profilo puo'
	 * DICHIARARE; questo vincola cosa la risoluzione ONORA. Trovato in code review.
	 */
	static constexpr int32 MaxStepsPerTick = 2;

	/**
	 * Il massimo `TickPeriod` che la risoluzione accetta.
	 *
	 * 🔴 **Serve perche' il periodo del calendario e' un minimo comune multiplo**, e senza tetto cresce col
	 * prodotto dei periodi: quattro unita' con `3, 5, 7, 11` danno `1155`, e il ciclo che aspetta un giro
	 * completo prima di dichiarare finita la risoluzione ne consumerebbe 2310 — oltre il budget di
	 * micro-step del chiamante. Con periodi grandi e coprimi il prodotto **trabocca** e il periodo diventa
	 * negativo, cioe' la risoluzione si dichiara finita al primo sotto-passo inerte e tronca ogni percorso
	 * a meta'. Il commento che diceva *«i periodi sono pochi e piccoli»* era un'assunzione che niente
	 * faceva rispettare. Trovato in code review.
	 */
	static constexpr int32 MaxTickPeriod = 4;
};

/**
 * Cosa il GIOCATORE aveva chiesto, separato da cio' che il TERRENO ha aggiunto al percorso (`#2314`).
 *
 * 🔑 **E' la distinzione che il resolver non poteva fare.** Riceve un `Paths[i]` gia' esteso dallo
 * scivolamento e ne considera l'ultima cella la destinazione: se quell'ultima cella non viene raggiunta
 * scrive un `BlockReason`, cioe' *«fermo: cella occupata»* a un'unita' arrivata **esattamente dove il
 * giocatore l'aveva mandata**. I due campi qui sotto sono l'informazione mancante, e stanno insieme perche'
 * rispondono insieme: separati sarebbero un intero e un booleano scollegati, e nessuno dei due basta.
 *
 * ⚠️ **Dato PER UNITA', come `Paths`.** `FinalizeHexMovementOutcomes` dichiara che l'esito non dipende
 * dall'ordine di iterazione; un dato per-unita' nello stato non viola quel contratto, uno globale mutabile
 * si'. Misurato da `HexSim.PlannedLengthOutcomesAreOrderIndependent`.
 */
struct FRTPlannedMovement
{
	/**
	 * Quante celle INIZIALI di `Paths[i]` appartengono al piano del giocatore, cella di partenza inclusa.
	 * Tutto cio' che sta oltre e' estensione ambientale.
	 *
	 * `0` significa «non dichiarato»: `BeginHexMovement` lo porta a `Paths[i].Num()`, cioe' «il percorso e'
	 * tutto pianificato», che e' il comportamento di ogni chiamante che non conosce i terreni.
	 */
	int32 PlannedLength = 0;

	/**
	 * Il terreno imponeva uno spostamento di slide oltre la destinazione pianificata — **anche quando
	 * nessuna cella e' finita nel percorso**.
	 *
	 * Lo produce `FRTIceSlideResult::bSlideRequested`, dove sta il perche' per esteso; qui viaggia come
	 * dato dello stato. Il chiamante puo' spegnerlo — la topologia che taglia dentro al piano toglie allo
	 * scivolamento la sua precedenza — quindi i due campi non sono sempre lo stesso valore.
	 */
	bool bSlideRequested = false;
};

/**
 * Cosa `ApplyIceSliding` ha fatto al percorso, e cosa il terreno aveva CHIESTO (`#2314`).
 *
 * 🔴 **Esiste perche' le uscite negative erano indistinguibili.** La funzione restituiva il `Path` immutato
 * in sei casi diversi, e dall'esterno erano lo stesso valore: chi chiamava non poteva sapere se il terreno
 * non avesse chiesto nulla — nessun `SlideCells`, budget sotto soglia, nessuna direzione da cui scivolare —
 * oppure avesse chiesto uno scivolamento **impedito da un muro**. I due fatti hanno esiti diversi nel
 * TurnLog, e la differenza non e' ricostruibile a valle: `#2290` ci ha provato e ha raccolto tre difetti di
 * correttezza dalla stessa radice.
 */
struct FRTIceSlideResult
{
	/** Il percorso: esteso di una cella se lo scivolamento e' avvenuto, altrimenti quello ricevuto. */
	TArray<FRTCellId> Path;

	/**
	 * Il terreno imponeva uno scivolamento da quella cella d'arrivo — superficie scivolosa, budget residuo
	 * sufficiente, e una direzione che esiste.
	 *
	 * ⚠️ **Non dice che sia avvenuto.** «E' stato accodato» si legge dalla lunghezza di `Path` contro quella
	 * ricevuta; questo campo dice che la domanda era stata posta. Vero + percorso invariato = **impedito**.
	 */
	bool bSlideRequested = false;
};

/** Esito del movimento simultaneo di un'unita': cella finale, celle attraversate (partenza esclusa), reason code. */
USTRUCT(BlueprintType)
struct FRTHexMoveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	FRTCellId Final;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	TArray<FRTCellId> Entered;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	ERTMoveOutcome Outcome = ERTMoveOutcome::Stayed;
};

/**
 * Stato di una risoluzione di movimento SOSPENDIBILE fra un microstep e l'altro (CP 14.2).
 *
 * Esiste per una ragione sola: un Overwatch interattivo deve poter fermare il movimento **dentro** il calcolo.
 * Se la finestra si aprisse a movimento concluso, il futuro sarebbe gia' stato calcolato con uno stato che il
 * colpo avrebbe dovuto cambiare — e il prompt mostrerebbe una scelta gia' irrilevante.
 *
 * NON e' una USTRUCT, come `FRTHexSnapshot` qui sotto e per una ragione affine: `TArray<TArray<>>` non e'
 * esponibile a UPROPERTY, e questo e' stato interno di un calcolo, non un dato di gioco che qualcuno debba
 * ispezionare da Blueprint.
 *
 * **Gli input sono COPIATI, non referenziati.** E' il punto della struttura: un resolver che restituisce il
 * controllo non puo' dipendere dalla vita di variabili locali del chiamante, che fra un microstep e il
 * successivo puo' essere uscito dal proprio scope — o aver aperto una finestra di reazione durata un turno di
 * orologio. I percorsi di una partita 2v2 sono poche decine di celle: la copia costa meno di un dangling.
 */
struct FRTMovementResolutionState
{
	/** Percorsi pianificati, uno per unita'. Indice = identita' dell'unita' per tutta la risoluzione. */
	TArray<TArray<FRTCellId>> Paths;

	/**
	 * Quanto di `Paths[i]` il giocatore ha davvero chiesto, e se il terreno voleva portare l'unita' oltre
	 * (`#2314`). Sempre della stessa lunghezza di `Paths`: `BeginHexMovement` completa cio' che manca con
	 * «tutto pianificato, nessuno scivolamento», che e' il caso di ogni chiamante che non tocca i terreni.
	 */
	TArray<FRTPlannedMovement> Planned;

	/** Precedenza sulla cella contesa (CP 4.8). Vuoto = tutti a parita', com'era prima delle priorita'. */
	TArray<int32> Priorities;

	/** Chi si muove in linea (`Action.Charge` e affini): due lineari non si attraversano, si scontrano. */
	TArray<bool> bLinearMovers;

	/** Chi ATTRAVERSA le unita' ferme, tranne che sulla propria cella finale. */
	TArray<bool> bPassThrough;

	/**
	 * La squadra di ogni unita', parallela a `Paths` — `#2984`, [D-396]. Vuoto o `INDEX_NONE` = non
	 * dichiarata, e chi non la dichiara non attraversa nessuno: il comportamento di prima.
	 *
	 * ⚠️ **Un array parallelo e non le unita'**, come `Priorities` e `bPassThrough`: questo resolver
	 * riceve percorsi e dati per indice, mai `FRTHexSimUnit`. Passargli le unita' per un solo campo
	 * allargherebbe il suo ingresso a tutto lo stato di gioco.
	 */
	TArray<int32> Teams;

	/** Posizione corrente di ogni unita'. */
	TArray<FRTCellId> Pos;

	/** Indice raggiunto dentro il proprio `Paths[i]`. */
	TArray<int32> Prog;

	/**
	 * Quanti microstep dura ogni ARCO del percorso ([D-381](../../../docs/decisions/RT_PDR_00_Decision_Log.md)).
	 * `StepDurations[i][k]` e' la durata dell'arco che porta a `Paths[i][k + 1]`, quindi ha un elemento in
	 * meno del percorso.
	 *
	 * 🔑 **E' un DATO del passo, non una lettura del costo**, ed e' la ragione per cui esiste come array
	 * invece che come una chiamata alla mappa qui dentro. Se la durata fosse *per contratto* il costo,
	 * l'unico modo di rendere un profilo piu' RAPIDO sarebbe fargli costare MENO per cella — e costare meno
	 * significa andare piu' LONTANO: velocita' e portata sarebbero la stessa manopola per sempre, e `MOV-3`
	 * si chiuderebbe per inerzia invece che per playtest.
	 *
	 * ⛔ **Il resolver NON riceve la mappa**, e questo array e' il modo di rispettarlo: la durata si deriva a
	 * monte, dove costo, modificatore e percorso sono tutti visibili — `ARTTurnManager::ResolveMovement` — e
	 * viaggia qui gia' calcolata. Passare lo snapshot al resolver renderebbe rosso
	 * `Movement.BlockedPath_DoesNotAutoReroute`, che esiste per impedirlo.
	 *
	 * Vuoto, piu' corto di `Paths`, o con valori `<= 0` -> **un microstep per arco**, cioe' il comportamento
	 * che il resolver aveva prima di `#2914`: con l'array vuoto l'esito e' identico per costruzione.
	 */
	TArray<TArray<int32>> StepDurations;

	/**
	 * Microstep ancora da pagare per l'arco IN CORSO, per unita'. `1` significa «l'arco si completa in questo
	 * microstep»; un valore maggiore significa che l'unita' e' **in transito**.
	 *
	 * ⚠️ **Un'unita' in transito resta sulla propria cella d'origine** ([D-382](../../../docs/decisions/RT_PDR_00_Decision_Log.md)):
	 * `Pos[i]` non cambia finche' l'arco non e' completo, e non esiste un istante in cui sia «fra due celle».
	 * Ne segue, dichiarato: un'unita' lenta **tappa il corridoio** per l'intera durata del passo, e incassa
	 * nella copertura di PARTENZA fino all'ingresso — perche' `Pos` e' il soggetto autorevole, durante il
	 * ciclo, del colpo di Overwatch e della sua copertura, del facing d'impatto, del verdetto di visibilita'
	 * delle voci, della conoscenza di squadra, della geometria della voce e del `MoveLog`; non solo
	 * dell'occupancy. L'elenco governante e' in `spec-tassonomia-movimento.md` §2.0-ter.
	 *
	 * ⚠️ **E il ritardo si propaga lungo una catena**: un inseguitore bloccato non paga il proprio arco
	 * mentre aspetta, quindi un convoglio su terreno costoso **si serializza** invece di avanzare in blocco.
	 * E' la conseguenza diretta di [D-382], pinnata da `Movement.ConvoyOnCostlyTerrainSerializes`.
	 */
	TArray<int32> StepRemaining;

	/**
	 * **La cadenza di ciascuna unita' nel calendario dei sotto-passi** ([D-428], che chiude `SKB-2`).
	 *
	 * ⛔ **Il resolver non conosce i profili di movimento**, e questo array e' il modo di rispettarlo: la
	 * cadenza si deriva a monte — `ARTTurnManager::ResolveMovement`, dove il profilo e' visibile — e viaggia
	 * qui gia' tradotta in numeri. E' la stessa disciplina di `StepDurations`, e per la stessa ragione.
	 *
	 * Vuoto, piu' corto di `Paths`, o con valori `<= 0` -> **cadenza neutra**, cioe' `{1, 1}`: un passo per
	 * tick, che e' il comportamento di prima di `SKB-2`. Con l'array vuoto la sequenza di micro-step emessi
	 * e' identica **per costruzione**.
	 */
	TArray<FRTMovementCadence> Cadences;

	/**
	 * L'indice del **CALENDARIO**: avanza sempre, e decide chi e' eleggibile in questo sotto-passo.
	 *
	 * 🔴 **E' distinto da `MicroStepIndex`, e la distinzione e' l'intera ragione per cui il corpus golden
	 * non si muove.** Il calendario scorre anche sui sotto-passi in cui nessuno puo' avanzare; quelli **non
	 * si eseguono e non emettono** un micro-step. In una partita in cui tutti hanno cadenza neutra, il
	 * secondo sotto-passo di ogni tick non si materializza mai, e la sequenza di `MicroStepIndex` — che e'
	 * cio' che il TurnLog porta — resta quella di sempre.
	 *
	 * ⚠️ Se i due contatori tornassero a essere uno solo, ogni voce di movimento del corpus cambierebbe
	 * `MicroStepIndex` per il solo fatto che il calendario esiste.
	 */
	int32 CalendarIndex = 0;

	/**
	 * **Quanti sotto-passi contiene un tick in QUESTA risoluzione** ([D-428]).
	 *
	 * 🔑 **E' derivato dai partecipanti, non una costante**: vale il massimo `StepsPerTick` fra le cadenze
	 * dichiarate, e almeno `1`. Con soli profili neutri vale **1**, quindi `s` e' sempre `0`, tutti sono
	 * eleggibili a ogni micro-step e la risoluzione e' identica a prima del calendario — per costruzione.
	 *
	 * ⚠️ **Una costante globale sarebbe stata sbagliata, ed e' stato misurato**: fissandolo a `2` il `Move`
	 * poteva aprire un arco solo nei sotto-passi pari, e il suo percorso costava il doppio dei micro-step
	 * appena un'altra unita' teneva vivo un sotto-passo dispari.
	 */
	int32 SubStepsPerTick = 1;

	/**
	 * Sotto-passi consecutivi in cui **nessuno** ha progredito ([D-428]).
	 *
	 * 🔑 **Serve perche' «nessuno si e' mosso» ha smesso di significare «e' finita».** Con una cadenza non
	 * neutra esistono sotto-passi legittimamente inerti — quello in cui uno `Sneak` salta il proprio turno —
	 * e dichiarare finita la risoluzione al primo di essi troncherebbe il movimento a meta'.
	 *
	 * ⚠️ **E quei sotto-passi si EMETTONO**, non si saltano: i confini di reazione e
	 * `FRTTurnLogEntry::MicroStepIndex` sono chiavati sui micro-step emessi, quindi saltarli renderebbe la
	 * cadenza inosservabile — uno `Sneak` da solo avanzerebbe a ogni micro-step come un `Move`, e l'indice
	 * d'ingresso in una zona sorvegliata cambierebbe quando un'unita' ESTRANEA finisce il proprio percorso.
	 * `SKB-2` nominava proprio quell'esito come competitivo. Trovato in code review.
	 *
	 * La risoluzione finisce quando questo contatore raggiunge un giro completo di calendario.
	 */
	int32 IdleSubSteps = 0;

	/**
	 * Il periodo del calendario, calcolato **una volta** in `BeginHexMovement`.
	 *
	 * ⚠️ E' interamente determinato da `Cadences` e `SubStepsPerTick`, che il ciclo non tocca. Ricalcolarlo
	 * a ogni micro-step costava una passata su tutte le unita' con un massimo comune divisore ciascuna, per
	 * riottenere una costante. Trovato in code review.
	 */
	int32 CalendarPeriodCached = 1;

	/**
	 * L'indice, dentro `Paths[i]`, della cella su cui l'ARCO in corso termina — `#3012`, [D-398].
	 *
	 * 🔑 **Un arco puo' coprire piu' di una cella**, ed e' cio' che rende l'attraversamento sicuro:
	 * copre le celle occupate consecutive piu' la prima LIBERA, quindi non esiste un micro-step in cui
	 * l'unita' sia *sopra* qualcun altro. Sotto [D-382] l'unita' resta sulla propria origine per tutta la
	 * durata dell'arco e compare direttamente sulla cella d'arrivo.
	 *
	 * ⚠️ **`ArcEnd <= Prog` significa «nessun arco aperto»**, ed e' il segnale con cui il micro-step
	 * successivo sa di doverne calcolare uno. L'arco si apre in un passaggio DEDICATO, prima del punto
	 * fisso: calcolarlo dentro il ciclo d'avanzamento leggerebbe un `Pos` aggiornato a meta', e l'esito
	 * dipenderebbe dall'ordine delle unita' — che e' precisamente cio' che questo resolver non ammette.
	 */
	TArray<int32> ArcEnd;

	/** Percorso esaurito, o nessun movimento da fare. */
	TArray<bool> Done;

	/**
	 * Motivo del PRIMO congelamento, e solo di quello: un microstep successivo bloccherebbe la stessa unita'
	 * per un motivo diverso e piu' recente, ma meno vero. La memoria vive nello stato perche' e' proprio cio'
	 * che un resolver a passi non puo' ricalcolare guardando l'ultimo passo.
	 */
	TArray<ERTMoveOutcome> BlockReason;

	/** Il motivo di `BlockReason[i]` e' stato fissato e non si sovrascrive. */
	TArray<bool> ReasonLocked;

	/** Risultato in costruzione: `Entered` cresce a ogni microstep, `Outcome` si scrive alla fine. */
	TArray<FRTHexMoveResult> Results;

	/**
	 * Quanti microstep sono stati **eseguiti**. Diagnostico: nessuna regola lo legge.
	 *
	 * 🔴 **E la riga qui sopra E' GIA' FALSA, da prima di [D-428].** `RTReactionOpportunityTypes.h` lo
	 * dichiara per esteso: *«e' oggi documentato come "diagnostico: nessuna regola lo legge". Entrando qui
	 * smette di esserlo: diventa parte di un identificatore che finisce nell'hash»*. E
	 * `ARTTurnManager::AdvanceMovementResolution` lo travasa in `CurrentMicroStepIndex`, che chiava ogni
	 * confine di reazione e ogni voce di TurnLog — rinumerarlo **rompe il replay**.
	 *
	 * ⚠️ **La prima stesura di [D-428] ci aveva aggiunto un «✅ e resta vero», che era falso due volte**:
	 * la riga non era vera nemmeno prima, e la decisione stessa dichiara al punto (3) che questo campo *«e'
	 * quello che il TurnLog porta»*. Trovato in code review. Cio' che [D-428] garantisce e' un'altra cosa,
	 * piu' stretta e utile: il calendario ha un contatore PROPRIO (`CalendarIndex`), quindi **non tocca**
	 * la numerazione che replay e reazioni leggono.
	 */
	int32 MicroStepIndex = 0;

	/** Vero quando l'ultimo microstep non ha mosso nessuno: da qui in poi `Results` non cambia piu'. */
	bool bFinished = false;

	int32 Num() const { return Paths.Num(); }
};

/**
 * Due unita' VIVE trovate sulla stessa cella costruendo l'occupancy (`#1970`).
 *
 * 🔴 **E' un errore strutturale, e fino al 2026-08-31 accadeva in silenzio.** `MakeSnapshot` dichiarava
 * *«le sovrapposizioni sono un errore strutturale, segnalato da `ValidateSnapshot`»* — e `ValidateSnapshot`
 * in partita non lo chiama nessuno: cinque test e un report di debug su richiesta esplicita. L'invariante
 * era dichiarata e non la guardava nessuno.
 *
 * ⚠️ **Il fatto sta nel DATO e non in un log, e la ragione e' misurata**: `MakeSnapshot` sta anche sotto
 * `ARTPlayerController`, che ricostruisce lo snapshot a ogni interazione di pianificazione. Un `UE_LOG` qui
 * dentro produrrebbe centinaia di righe identiche al secondo per un singolo difetto — rendendo piu'
 * difficile la diagnosi che questo campo esiste per abilitare. Rilevare e segnalare sono due mestieri: qui
 * si rileva, e chi e' autoritativo decide se dirlo.
 */
struct FRTHexOverlap
{
	/** La cella contesa. */
	FRTCellId Cell;

	/** L'unita' che NON entra in `Occupancy`: a parita' di cella vince l'`UnitId` minore. */
	int32 DiscardedUnitId = INDEX_NONE;

	/** L'unita' che la occupa, cioe' quella arrivata prima nell'ordine stabile. */
	int32 KeptUnitId = INDEX_NONE;
};

/**
 * Stato CONGELATO a inizio fase per la risoluzione su griglia esagonale ("raccogli poi applica", invariante #3).
 *
 * NON e' una USTRUCT e NON va conservata oltre la fase che la produce: contiene un puntatore non-UPROPERTY
 * all'asset mappa (nessuna protezione dal GC). L'occupazione e' COPIATA (cambia durante la risoluzione);
 * la mappa e' solo referenziata insieme a hash/revisione, perche' e' immutabile per tutto il turno —
 * URTHexSimLibrary::IsSnapshotStale rileva la violazione di questa assunzione invece di ignorarla.
 */
struct FRTHexSnapshot
{
	/** Mappa autorevole (non posseduta): valida solo per la durata della fase. */
	const URTHexMapAsset* Map = nullptr;

	/** Hash del contenuto della mappa al momento della cattura. */
	uint32 MapHash = 0;

	/** Revisione della mappa al momento della cattura. */
	int32 Revision = 0;

	/** Unita' del turno, ordinate per UnitId (ordine stabile). */
	TArray<FRTHexSimUnit> Units;

	/** Cella -> UnitId dell'occupante (solo unita' vive). */
	TMap<FRTCellId, int32> Occupancy;

	/**
	 * Le sovrapposizioni fra unita' VIVE trovate costruendo `Occupancy`. Vuoto = snapshot sano (`#1970`).
	 *
	 * ⚠️ **Non entra in nessun hash e in nessuna serializzazione**, e non e' un'omissione: e' diagnostica
	 * della COSTRUZIONE, non stato di gioco. Se entrasse, due partite identiche con e senza il difetto
	 * darebbero hash diversi — e la sovrapposizione diventerebbe un fatto *di gioco*, che e' l'opposto
	 * dell'intento. `FRTHexSnapshot` non e' serializzato (nessun `operator<<`), quindi il campo e' inerte
	 * per replay e determinismo.
	 *
	 * ⛔ **L'esito NON cambia**: a parita' di cella vince l'`UnitId` minore, esattamente come prima. Questo
	 * campo rende la condizione visibile, non la risolve diversamente.
	 */
	TArray<FRTHexOverlap> Overlaps;

	/**
	 * Cosa sa ogni squadra (CP 13.2), ordinata per `TeamId` — un array e non una `TMap` perche' i consumatori
	 * la ITERANO, e l'ordine di una TMap non e' garantito (invariante #3).
	 *
	 * Sta nello snapshot e non in un servizio a parte perche' e' **stato del turno**: il bot che deciderà su
	 * conoscenza parziale (CP 13.5) e il resolver che rifiuta un bersaglio ignoto devono leggere la STESSA
	 * fotografia, altrimenti la partita si decide su due verita' diverse.
	 */
	TArray<FRTTeamKnowledge> TeamKnowledge;
};
