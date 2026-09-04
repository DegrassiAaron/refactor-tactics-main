#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "RTHexCellData.generated.h"

/**
 * Superficie della cella. Le nove voci sono quelle che la simulazione ambientale **applica davvero**: costi e
 * stati stanno in `spec-terreni-e8.md` e nei test di `E8`, non qui — questo enum e' il vocabolario, non la
 * regola. `Void` non e' a catalogo e cade su `FRTTerrainDef()`.
 *
 * ⚠️ Diceva *«tipi iniziali … la simulazione ambientale arriva dopo»*, e l'epic **E8 e' chiusa dal
 * 2026-08-07**: terreni, stati temporanei, propagazione elettrica, terreno dinamico. Il futuro che quella
 * frase annunciava era gia' passato (#1320).
 */
UENUM(BlueprintType)
enum class ERTHexSurface : uint8
{
	Floor,
	ShallowWater,
	Rough,
	Fire,
	Conductive,
	Ice,
	Void,
	Smoke,
	HighGround
};

/**
 * Tipo di copertura su UN bordo di cella.
 *
 * ⚠️ **I valori nuovi vanno in CODA**, e la ragione vale ancora per il prossimo che ne aggiungera' uno:
 * estendere un enum non e' una migrazione di formato — riordinarlo si', perche' il valore serializzato e'
 * l'indice. E un valore che nessuna regola sa ancora applicare non si inventa: si aggiunge quando la regola
 * esiste.
 *
 * ⚠️ Questa frase diceva *«`High` (CP 9.2) si aggiungera' in coda»* **al futuro**, e `High` e' qui sotto —
 * in coda — dal 2026-08-07. L'argomento era giusto e il tempo verbale no (#1320).
 */
UENUM(BlueprintType)
enum class ERTHexCoverType : uint8
{
	None,  // nessuna copertura (una voce con questo tipo e' un dato incoerente: la valida ValidateMap)
	Low,   // copertura bassa: ripara dai colpi diretti che attraversano il bordo, non blocca ne' vista ne' passo
	High   // copertura alta (CP 9.2): NEGA l'attraversamento del bordo a vista, passo e proiettili
};

/**
 * Copertura su UNO dei sei bordi di una cella. L'array della cella e' SPARSO: solo i bordi riparati vengono
 * serializzati, quindi una mappa senza coperture pesa e hasha esattamente come prima.
 *
 * La direzionalita' e' del BORDO, non dell'unita': girarsi non sposta un muretto. Il facing (ADR-0005, epic
 * #175) e' una direzionalita' ORTOGONALE — un colpo fuori dall'arco frontale ANNULLA questa riduzione
 * (CP 16.2), ma e' una regola additiva, non la stessa regola.
 */
USTRUCT(BlueprintType)
struct FRTHexCover
{
	GENERATED_BODY()

	/** Bordo riparato, visto DALLA cella: `W` protegge dai colpi che entrano dal vicino a ovest. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTHexDirection Edge = ERTHexDirection::E;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTHexCoverType Type = ERTHexCoverType::Low;

	/**
	 * Punti struttura del riparo. Il valore di catalogo per tipo lo da' `DefaultIntegrity` qui sotto — non si
	 * ripete qui, perche' un numero scritto due volte diverge (#1194).
	 *
	 * **Chi lo scala e' `URTHexCoverLibrary::ApplyStructureDamage`**, attraverso `DamageFace`
	 * ([`RTHexCoverLibrary.cpp`](RTHexCoverLibrary.cpp)): sottrae, e **quando arriva a zero rimuove l'entry**
	 * invece di lasciarla a `0` — quindi una copertura a zero non sopravvive a un danno, e il caso esiste solo
	 * nel dato AUTORATO. Ogni passaggio produce un `FRTCoverDamageResult{RemainingIntegrity, bDestroyed}`, ed
	 * e' quello che entra nel TurnLog: l'esito osservabile e' l'evento, non questo campo.
	 *
	 * I consumatori sono TRE, non uno:
	 * 1. `ApplyStructureDamage` — lo scala, ed e' il produttore dell'esito;
	 * 2. `URTHexMapAsset::ValidateMap` — rifiuta `Integrity <= 0`, cioe' il riparo che non ripara;
	 * 3. `URTHexMapAsset::ComputeHash` e `RTMatchStateHash` — **il campo entra nell'hash di stato**, quindi e'
	 *    dato deterministico e non decorazione. ⚠️ Da non confondere con `D-172`/`D-186`, dove a restare fuori
	 *    dall'hash e' la LETTURA di presentazione (`critico · ridotto · intatto`), non il numero.
	 *
	 * 🔴 **Questo commento prometteva la distruzione «con CP 9.2, che lo scalera'» — al futuro, e CP 9.2 era
	 * chiuso dal 2026-08-07** ([#70](https://github.com/DegrassiAaron/refactor-tactics-main/issues/70)). La
	 * frase e' stata trascritta in quattro documenti prima che qualcuno la verificasse (#1099, corretti in
	 * #1104), e il generatore e' rimasto qui fino a #1107: un commento invecchia come la prosa e **non
	 * fallisce mai** — nessun gate lo vede, nessun test lo contraddice. Per questo ora nomina i **simboli**
	 * invece di un checkpoint: un simbolo si cerca, e se sparisce il compilatore lo dice.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 Integrity = 30;
	// ⚠️ Il default del campo vale per la struct costruita di default e per cio' che si rilegge da un asset
	// vecchio, dove `Type` e' `Low`: deve restare lo stesso numero del catalogo, e a dirlo e' il compilatore
	// invece di due letterali che si somigliano (#1194). Non si scrive `DefaultIntegrity(Low)` qui perche'
	// la funzione e' dichiarata piu' sotto.

	/**
	 * PROVENIENZA: questa copertura l'ha prodotta la cottura della geometria (`#621`), non la mano di un
	 * designer. Default `false` = dipinta a mano — che e' anche cio' che ogni copertura scritta prima di
	 * questo campo diventa rileggendosi, ed e' esattamente cio' che quelle coperture gia' erano.
	 *
	 * ⚠️ **Il rebake tocca SOLO le proprie** (`D-131`): rimuove le `bGenerated`, riscrive quelle derivate dai
	 * segmenti correnti, e non tocca mai le altre. Senza questo campo un rebake non saprebbe distinguere «la
	 * copertura che avevo prodotto io e ora va tolta» da «quella dipinta a mano che va preservata», quindi
	 * TOGLIERE un segmento non potrebbe togliere la sua copertura. E' cio' che rende il bake idempotente.
	 *
	 * 🔑 **NON entra in `ComputeHash`, ed e' la parte da non sbagliare.** Le coperture ci entrano perche' sono
	 * dato autorevole — cambiano il danno subito — ma la PROVENIENZA non cambia una partita: se entrasse, due
	 * mappe che si giocano in modo identico avrebbero hash diversi solo perche' una copertura e' stata
	 * disegnata invece che dipinta, cioe' un falso positivo contro il KPI `replay divergence = 0`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	bool bGenerated = false;

	/**
	 * Integrita' di catalogo per tipo: 30 la bassa, 50 l'alta (v0.1). Sta qui e non in `URTCombatLibrary`
	 * perche' `Map/` non puo' dipendere da `Combat/`; la RIDUZIONE del danno, che e' della stessa famiglia dei
	 * numeri di `Guard`/`Deflect`/`Brace`, resta invece di la'.
	 */
	static constexpr int32 DefaultIntegrity(ERTHexCoverType Type)
	{
		return Type == ERTHexCoverType::High ? 50 : 30;
	}

	/**
	 * Sentinella del costruttore: «prendi il valore di catalogo del tipo» (#1194, `D-186`).
	 *
	 * ⚠️ **Negativa, e non zero**: `0` e' un'integrita' legittima — `RTHexCoverTests` costruisce
	 * deliberatamente una copertura a zero per verificare che `ValidateMap` la rifiuti — quindi usarlo come
	 * sentinella renderebbe impossibile scrivere quel caso.
	 *
	 * ⚠️ **Ma e' PUBBLICA, e attraversa `URTHexCoverLibrary::AddCover`**, che inoltra il proprio
	 * `Integrity` senza validarlo (`RTHexCoverLibrary.cpp`). Un `-1` che arrivi da li' non e' piu' un dato
	 * invalido che `ValidateMap` respinge (`Cover.Integrity <= 0`, `RTHexMapAsset.cpp`): e' un **comando**, e
	 * produce una copertura a catalogo in silenzio. Oggi l'unico chiamante e' `ARTTurnManager` con
	 * `Op.Integrity` d'autore, dove il valore mancante e' `0` e non `-1` — quindi il rischio e' dichiarato,
	 * non corso.
	 */
	static constexpr int32 UseCatalogIntegrity = -1;

	FRTHexCover() = default;

	/**
	 * 🔴 **Il default DERIVA dal tipo, e prima non lo faceva.** Il costruttore dichiarava
	 * `InIntegrity = 30` fisso: `FRTHexCover(Edge, ERTHexCoverType::High)` costruiva una copertura alta a
	 * **30**, cioe' il **60%** del suo valore di catalogo, senza che nulla l'avesse colpita — accettava il
	 * `Type` e **ignorava la funzione che sa cosa quel tipo vale**.
	 *
	 * ⚠️ **Misurato prima di cambiarlo** (#1194): degli **undici** siti che omettono `InIntegrity`,
	 * **uno solo** cambia valore — `RTHexMapActorTests.cpp:457`, l'unico che passa `High` — e quel test conta
	 * pannelli per bordo, non integrita'. Gli altri dieci passano `Low`, `None` o niente, per cui il catalogo
	 * vale 30 come prima. *(Sono siti di CHIAMATA: `RTHexMapTests.cpp:427` sta in un ciclo e ne produce sei.)*
	 *
	 * 🔴 **Questo NON copre l'autoraggio dall'EDITOR, che e' il percorso piu' battuto** (#1317). Chi
	 * aggiunge una entry `Covers` nel dettaglio di un `URTHexMapAsset` non passa di qui: la struct nasce da
	 * `FRTHexCover()` — `Low`/30 — e cambiare `Type` in `High` non ricalcola niente, perche' l'asset non ha
	 * un `PostEditChangeProperty`. `ValidateMap` non lo vede: la sua guardia e' `Integrity <= 0`.
	 *
	 * ⛔ **Le coperture gia' scritte in un `.uasset` non si toccano**: sono byte su disco, e una `High`
	 * autorata a 30 resta a 30. Sotto il vocabolario di `D-186` si legge **«ridotta»**, che e' vero — e' piu'
	 * debole di una `High` di catalogo — invece di «danneggiata», che direbbe che l'ha colpita qualcuno.
	 */
	explicit FRTHexCover(ERTHexDirection InEdge, ERTHexCoverType InType = ERTHexCoverType::Low,
		int32 InIntegrity = UseCatalogIntegrity)
		: Edge(InEdge), Type(InType),
		  Integrity(InIntegrity == UseCatalogIntegrity ? DefaultIntegrity(InType) : InIntegrity) {}
};

/**
 * Il default del CAMPO `Integrity` e quello di catalogo della `Low` devono restare lo stesso numero (#1194):
 * il campo vale per la struct costruita di default e per cio' che si rilegge da un asset vecchio, dove `Type`
 * e' `Low`. A dirlo e' il compilatore invece di due letterali che si somigliano.
 *
 * ⚠️ **Sta FUORI dalla struct e non dentro**, ed e' una correzione: uno `static_assert` a scope di classe si
 * valuta prima che la classe sia completa, quindi non puo' chiamare una `constexpr` membro della classe
 * stessa — `error C2131`. Qui la classe c'e' tutta.
 */
static_assert(FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low) == 30,
	"Il default del campo Integrity e quello di catalogo della Low devono restare lo stesso numero (#1194).");

/**
 * Stato di una porta su un bordo (CP 9.3). `Closed` e `Locked` bloccano allo stesso modo passo e vista: la
 * differenza e' CHI puo' riaprirle — `SetDoorState` non apre una `Locked`, serve l'apertura autorizzata di
 * CP 10.1. `Destroyed` e' TERMINALE: una porta sfondata non si richiude.
 */
UENUM(BlueprintType)
enum class ERTHexDoorState : uint8
{
	Open,      // si passa e si vede
	Closed,    // nega passo e vista; riapribile
	Locked,    // come Closed, ma non si apre da sola
	Destroyed  // aperta per sempre (stato terminale)
};

/**
 * Porta su UNO dei sei bordi di una cella. Come le coperture, l'array della cella e' SPARSO e la direzionalita'
 * e' del BORDO: una mappa senza porte pesa e hasha esattamente come prima.
 *
 * Una porta e' un bordo e non un arco (`FRTHexEdge`) perche' e' SOTTRATTIVA — nega un'adiacenza che esiste —
 * mentre l'arco e' additivo: `GraphNeighbors` aggiunge i sei vicini planari PRIMA e indipendentemente dagli
 * archi, quindi togliere un arco fra celle adiacenti non chiuderebbe nulla. Vedi
 * docs/gameplay/spec-porte-cp93.md §1.
 */
USTRUCT(BlueprintType)
struct FRTHexDoor
{
	GENERATED_BODY()

	/** Bordo occupato dalla porta, visto DALLA cella (stessa convenzione di `FRTHexCover`). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTHexDirection Edge = ERTHexDirection::E;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTHexDoorState State = ERTHexDoorState::Closed;

	/**
	 * Gruppo di appartenenza: i bordi che lo condividono formano UNA porta larga e si commutano insieme, con
	 * un solo incremento di revisione (un portone e' un evento, non tre). `INDEX_NONE` = porta singola.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 DoorId = INDEX_NONE;

	/**
	 * Nome PUBBLICO della struttura (CP 23.3, #832), stabile attraverso salvataggio, ricarica e cottura.
	 *
	 * `DoorId` sopra resta l'indice interno del gruppo e non cambia mestiere: distingue i gruppi dentro
	 * l'asset, e chi rieditasse la mappa potrebbe riassegnarlo. Questo invece e' il nome che uno scenario
	 * puo' citare e che un replay puo' risolvere — l'anello che a `DoorId` manca.
	 *
	 * I bordi che lo condividono sono la STESSA struttura, con la stessa forma con cui `DoorId` gia'
	 * definisce il gruppo. `NAME_None` = struttura senza nome pubblico, che e' cio' che ogni porta scritta
	 * prima di questo campo diventa rileggendosi — ed e' esattamente cio' che quelle porte gia' erano.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	FName StableId;

	FRTHexDoor() = default;
	explicit FRTHexDoor(ERTHexDirection InEdge, ERTHexDoorState InState = ERTHexDoorState::Closed,
		int32 InDoorId = INDEX_NONE)
		: Edge(InEdge), State(InState), DoorId(InDoorId) {}
};

/**
 * Quanto del volume-cella e' PIENO sotto la superficie, in terzi — `#1865`.
 *
 * 🔑 **E' un dato d'AUTORE e non una deduzione**, ed e' la ragione per cui e' un enum e non un `float`: la
 * regola del corpo non e' derivabile dal contesto senza ambiguita'. Un ponte e una collina hanno entrambi
 * il vuoto sotto di se' — il primo deve restare attraversabile, la seconda no — e nessun segnale
 * geometrico li distingue. Lo dichiara chi disegna.
 *
 * ⛔ **Non entra in `ComputeHash`**: e' presentazione. Due mappe che differiscono solo qui si giocano
 * identiche, ed e' il non-goal che `#1865` dichiara per prima cosa.
 *
 * ⚠️ `None` e' il default e significa «nessun corpo», che e' cio' che ogni mappa scritta prima del formato
 * v15 gia' era: le superfici elevate erano dischi sospesi. Una ricarica non inventa un volume.
 */
UENUM(BlueprintType)
enum class ERTHexBodyFill : uint8
{
	/** Nessun corpo: la superficie resta un disco. */
	None = 0,
	/** Un terzo del volume-cella. Il caso del ponte e del tunnel: sotto resta spazio. */
	Third,
	/** Due terzi. */
	TwoThirds,
	/** Volume pieno: il corpo occupa l'intero `LayerHeight` sotto la superficie. */
	Full
};

/**
 * Dato compatto e AUTOREVOLE di una cella esagonale (serializzato nell'asset mappa). Nessun Actor per cella.
 * Estendibile in milestone successive (hazard, Gameplay Tags, interazioni) senza rompere il formato.
 */
USTRUCT(BlueprintType)
struct FRTHexCellData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	FRTCellId Id;

	/** Quota/offset verticale della cella (per il rendering; la logica usa Layer + archi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 Height = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTHexSurface Surface = ERTHexSurface::Floor;

	/**
	 * Quanto volume si vede SOTTO questa superficie (`#1865`). Presentazione: non entra in `ComputeHash`.
	 *
	 * La quota della base la calcola `URTStructuralBodyLibrary::DeriveBodies`, che tronca il corpo sulla
	 * faccia superiore della prima cella sottostante: la frazione dichiara l'INTENZIONE, il derivatore la
	 * concilia con cio' che c'e' sotto.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTHexBodyFill BodyFill = ERTHexBodyFill::None;

	/** Costo di movimento base della cella (intero: no float nel pathfinding). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 MoveCost = 1;

	/**
	 * Sovrapprezzo di traversata dovuto alla GEOMETRIA che invade la cella (#619, formato v7): quanto costa in
	 * piu' passare da una cella stretta. Si somma a `MoveCost` — vedi `TotalMoveCost()`, che e' l'unico punto
	 * in cui i due si sommano.
	 *
	 * ⚠️ **Perche' non e' dentro `MoveCost`, che sarebbe stato piu' semplice**: quel campo ha gia' un
	 * produttore che lo RICALCOLA dalla sola `Surface` a ogni turno — `ARTTurnManager::ApplyDynamicSurface`
	 * quando una superficie cambia, `TickDynamicSurfaces` quando scade. Un corridoio stretto su cui
	 * un'abilita' mettesse dell'acqua tornerebbe, al ripristino, al costo del pavimento: il sovrapprezzo
	 * sparirebbe per il resto della partita e nessun test se ne accorgerebbe. Con due campi i due produttori
	 * non si toccano, ed e' la stessa forma di risposta che `MSE-1` cerca per i bordi.
	 *
	 * Non e' «un terzo booleano»: e' un intero, e i booleani accanto dicono un'altra cosa.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 OccupancySurcharge = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	bool bBlocksMovement = false;

	/**
	 * IL BLOCCO AL MOVIMENTO E' STATO DERIVATO DALLA COTTURA, o l'ha scritto un autore? — `E23.6`, `#1827`.
	 *
	 * 🔑 **Stessa disciplina di `FRTHexCover::bGenerated` (`D-131`), e per la stessa ragione.** Il rebake
	 * deve poter TOGLIERE il blocco che ha prodotto lui — altrimenti rimuovere il muro che rendeva la cella
	 * impraticabile non la restituirebbe al gioco — e non deve MAI toccare un `bBlocksMovement` dipinto a
	 * mano. Senza questo campo le due cose sono indistinguibili, e il bake dovrebbe scegliere fra due difetti:
	 * non essere idempotente, oppure cancellare la scelta dell'autore.
	 *
	 * Default `false` = **d'autore**, che e' anche cio' che ogni cella scritta prima di questo campo diventa
	 * rileggendosi — ed e' esattamente cio' che quelle celle gia' erano.
	 *
	 * ⛔ **NON entra in `ComputeHash`, e il criterio e' quello di `bGenerated`**: ci entra cio' che puo'
	 * cambiare un esito. `bBlocksMovement` ci entra — cambia dove si puo' andare — ma la sua PROVENIENZA no:
	 * due mappe che si giocano in modo identico avrebbero hash diversi solo perche' in una il blocco e' stato
	 * cotto invece che dipinto, che e' il falso positivo contro `replay divergence = 0`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	bool bMovementBlockGenerated = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	bool bBlocksLineOfSight = false;

	/**
	 * La cella e' un OBIETTIVO contendibile (CP 10.2, formato v11): chi la occupa alla fine del Cleanup fa
	 * punto, e due squadre presenti in pari numero non lo fanno.
	 *
	 * ⚠️ **Entra in `ComputeHash` insieme ai due booleani qui sopra**, e il criterio e' il loro stesso: ci
	 * entra cio' che puo' cambiare un esito. Questo campo cambia **chi vince** — due mappe identiche in
	 * tutto tranne dove sta l'obiettivo non si giocano allo stesso modo — quindi non possono condividere
	 * l'hash. E' il verso opposto di `MapClass` e `bGenerated`, che ne restano fuori perche' non toccano
	 * nessun esito di turno.
	 *
	 * ⚠️ **E' un `bool` e non un `TeamId`**, ed e' la regola di CP 10.2: l'obiettivo e' CONTENDIBILE, cioe'
	 * non appartiene a nessuno finche' qualcuno non lo occupa. Un campo di proprieta' dichiarerebbe una
	 * regola che questo checkpoint non ha; piu' obiettivi distinti e simultanei sono `CP 31.1`, post-v0.1.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	bool bIsObjective = false;

	/**
	 * Coperture per bordo (0..6 voci, al massimo una per bordo). Sparso: le celle scoperte — la quasi
	 * totalita' di una mappa — non pagano nulla. Formato v3.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	TArray<FRTHexCover> Covers;

	/**
	 * Porte per bordo (0..6 voci, al massimo una per bordo). Sparso come `Covers`. Formato v4.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	TArray<FRTHexDoor> Doors;

	/**
	 * Costo TOTALE di entrare in questa cella: il terreno piu' il sovrapprezzo della geometria.
	 *
	 * Esiste come accessore, e non come somma ripetuta nei chiamanti, perche' i lettori del costo sono
	 * CINQUE — il costruttore di vicini del pathfinding, i due accumuli della simulazione e i due
	 * `MaxCellCost`. Cinque copie della stessa somma sono cinque posti da cui puo' sparire.
	 */
	int32 TotalMoveCost() const
	{
		return FMath::Max(0, MoveCost) + FMath::Max(0, OccupancySurcharge);
	}

	/** Tipo di copertura sul bordo indicato (`None` se il bordo e' scoperto). */
	ERTHexCoverType CoverOn(ERTHexDirection Edge) const
	{
		for (const FRTHexCover& Cover : Covers)
		{
			if (Cover.Edge == Edge) { return Cover.Type; }
		}
		return ERTHexCoverType::None;
	}

	/**
	 * Voce di copertura su quel bordo, o nullptr se il bordo e' scoperto. `CoverOn` risponde «che tipo», questa
	 * serve a chi deve leggere anche l'INTEGRITA' — spostare una copertura conservandola (CP 9.5) o scalarla.
	 */
	const FRTHexCover* CoverEntryOn(ERTHexDirection Edge) const
	{
		for (const FRTHexCover& Cover : Covers)
		{
			if (Cover.Edge == Edge) { return &Cover; }
		}
		return nullptr;
	}

	/** Porta dichiarata su quel bordo, o nullptr se il bordo non ne ha. */
	const FRTHexDoor* DoorOn(ERTHexDirection Edge) const
	{
		for (const FRTHexDoor& Door : Doors)
		{
			if (Door.Edge == Edge) { return &Door; }
		}
		return nullptr;
	}

	FRTHexCellData() = default;
	explicit FRTHexCellData(const FRTCellId& InId) : Id(InId) {}
};

/** Tipo semantico di una transizione verticale/speciale (per rendering, validazione e futuri costi di profilo). */
UENUM(BlueprintType)
enum class ERTHexTransitionKind : uint8
{
	Stair,     // scala
	Ramp,      // rampa
	Bridge,    // ponte
	Tunnel,    // tunnel
	Elevator,  // ascensore
	Jump       // salto (predisposizione; teletrasporto resta futuro)
};

/**
 * Stato di un arco (CP 9.4). `Inactive` e `Destroyed` sono indistinguibili per il grafo — da nessuno dei due
 * si passa — e differiscono per la REVERSIBILITA': un ponte disattivato si riattiva, uno distrutto no. E' la
 * stessa distinzione che le porte fanno fra `Closed` e `Destroyed`.
 */
UENUM(BlueprintType)
enum class ERTHexArcState : uint8
{
	Active,    // il collegamento esiste e si percorre
	Inactive,  // spento: non si passa, ma si puo' riattivare
	Destroyed  // abbattuto: stato TERMINALE
};

/**
 * Arco di transizione ESPLICITA tra due celle esagonali (scale, rampe, ponti, tunnel, ascensori). Direzionale:
 * il bidirezionale richiede due archi. Le celle su layer diversi NON sono adiacenti senza un arco.
 *
 * L'arco e' ADDITIVO: crea un collegamento dove non c'era. E' la ragione per cui le PORTE non stanno qui ma
 * sui bordi (CP 9.3) — negare un'adiacenza planare richiede un oggetto sottrattivo, e questo non lo e'.
 */
USTRUCT(BlueprintType)
struct FRTHexEdge
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	FRTCellId From;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	FRTCellId To;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 Cost = 1;

	/** Tipo semantico dell'arco (informativo: non altera il pathfinding, che usa solo Cost). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTHexTransitionKind Kind = ERTHexTransitionKind::Stair;

	/** Stato del collegamento (CP 9.4). Solo `Active` e' percorribile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTHexArcState State = ERTHexArcState::Active;

	/** Punti struttura del ponte (catalogo v0.1: 40). A zero l'arco passa a `Destroyed`. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 Integrity = DefaultIntegrity;

	/**
	 * L'elettricita' RISALE questo collegamento (CP 9.4). Senza, la propagazione resta planare: il BFS di
	 * `URTTerrainLibrary::CollectElectricPropagation` cammina sui sei vicini e non sale mai di layer. Un ponte
	 * conduttivo e' quindi un rischio oltre che una scorciatoia.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	bool bConductsElectricity = false;

	/**
	 * Nome PUBBLICO della struttura (CP 23.3, #832), come per `FRTHexDoor::StableId`.
	 *
	 * ⚠️ Qui non c'e' un `DoorId` su cui appoggiare il gruppo: un arco e' identificato dalla coppia
	 * `(From, To)` e nient'altro. Il gruppo pero' la semantica del repository ce l'ha gia' — un ponte
	 * bidirezionale sono **due archi reciproci ma un evento solo** (`UpdateTransitions`, con lo stesso
	 * argomento del portone di CP 9.3) — ed e' l'unica condivisione di nome ammessa fra archi.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	FName StableId;

	/** Integrita' di catalogo di un ponte (v0.1). Sta qui per la stessa ragione di `FRTHexCover`. */
	static constexpr int32 DefaultIntegrity = 40;

	FRTHexEdge() = default;
	FRTHexEdge(const FRTCellId& InFrom, const FRTCellId& InTo, int32 InCost,
		ERTHexTransitionKind InKind = ERTHexTransitionKind::Stair)
		: From(InFrom), To(InTo), Cost(InCost), Kind(InKind) {}
};
