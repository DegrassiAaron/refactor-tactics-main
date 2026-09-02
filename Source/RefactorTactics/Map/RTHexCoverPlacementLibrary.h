#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexOccupancyLibrary.h"
#include "Map/RTGeometryGrammar.h"
#include "RTHexCoverPlacementLibrary.generated.h"

/**
 * POSA E COPERTURA DENTRO L'ESAGONO — la geometria tattica intra-cella, senza sottocelle navigabili.
 *
 * Chiude il Decision Record *«Cover Placement & Intra-Hex Geometry»* del 2026-08-30, che supera due
 * scorciatoie precedenti e ne conserva un vincolo:
 *
 * ```text
 * SUPERATO   «geometria che tocca il centro  =>  cella bloccata»          (D-179 punto 3)
 * SUPERATO   «N settori occupati             =>  cella bloccata»          (FRTOccupancyThresholds::BlockedFrom)
 * CONSERVATO  FRTCellId resta l'UNICO nodo di navigazione e di occupancy
 * ```
 *
 * ⚠️ **Qui non nasce un secondo pathfinder.** Nessuna funzione di questo file restituisce una destinazione,
 * un costo o un vicino: A* continua a camminare da `FRTCellId` a `FRTCellId` in `URTHexPathLibrary`, e i
 * dodici settori restano quello che `#619` li ha fatti — un righello angolare, non dodici caselle.
 *
 * ⚠️ **E non nasce un secondo slot di occupancy.** `URTHexSimLibrary::MakeSnapshot` tiene UNA unita' per
 * cella e `ValidateSnapshot` dichiara errore strutturale la seconda: piu' regioni di posa nello stesso
 * esagono sono stati ALTERNATIVI dello stesso occupante, non posti in piu'. Il test
 * `CoverOptionsDoNotIncreaseCellCapacity` lo pinna sul resolver vero.
 *
 * Interamente PURA, headless e intera. Vive nel modulo runtime, dove stanno gia' `URTHexOccupancyLibrary`
 * e `URTGeometryGrammarLibrary`: cio' che e' una REGOLA — cosa e' legale, cosa e' calpestabile, cosa si
 * attraversa — sta qui e l'editor la CHIAMA. Li' resta cio' che e' davvero d'interfaccia: il ghost, lo
 * snap, la transaction.
 *
 * ⚠️ **La ragione storica di quella collocazione era «in `Source/RefactorTacticsEditor/` non esiste alcun
 * test», e dal 2026-08-16 e' falsa** (`#993`, `Private/Tests/`, sei file). La collocazione resta, e regge
 * su un argomento diverso e migliore: una funzione pura si prova headless e senza mondo, mentre un tool
 * d'editor si prova con un viewport. La ragione vecchia sopravvive scritta in piu' punti, e
 * `docs/CONTEXT_INDEX.md` la dichiara superata.
 */

/**
 * UNA REGIONE DI POSA: i settori liberi CONTIGUI in cui un'unita' puo' materialmente stare.
 *
 * E' il tipo che sostituisce il conteggio. La domanda vecchia era *«quanti settori sono liberi?»*, e
 * rispondeva male al caso che il Decision Record porta come esempio: rocce sui settori `1,2,3` piu' un
 * albero su `7,8,9` lasciano **sei** settori liberi — sopra la soglia `BlockedFrom` — ma li lasciano in
 * **due gruppi da tre**, che e' uno spazio utilizzabile completamente diverso da sei settori in fila.
 *
 * La domanda nuova e' *«esiste un gruppo contiguo abbastanza grande?»*, e la contiguita' e' CIRCOLARE:
 * i settori `11` e `0` sono adiacenti, quindi una regione puo' scavalcare lo zero.
 */
USTRUCT(BlueprintType)
struct FRTPlacementRegion
{
	GENERATED_BODY()

	/** I settori della regione, un bit per settore, stessa ancora di `FRTOccupancyMask::Sectors`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 WedgeMask = 0;

	/**
	 * Il settore da cui la regione COMINCIA girando in senso crescente: quello il cui precedente e'
	 * occupato. E' la chiave d'ordinamento canonica, ed e' un dato e non un indice d'array — due
	 * enumerazioni della stessa maschera producono le stesse regioni con gli stessi `FirstWedge`.
	 *
	 * ⚠️ Con la cella interamente libera non esiste un settore «il cui precedente e' occupato»: la regione
	 * e' unica, copre l'intero anello, e `FirstWedge` vale `0` per convenzione dichiarata.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 FirstWedge = 0;

	/** Quanti settori contiene. Fra `1` e `RT_OccupancySectorCount`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 Size = 0;

	/** Vero se il settore appartiene a questa regione. */
	bool Contains(int32 Wedge) const
	{
		return Wedge >= 0 && Wedge < RT_OccupancySectorCount && ((WedgeMask >> Wedge) & 1) != 0;
	}
};

/**
 * QUANTO SPAZIO CHIEDE UN'UNITA' PER STARE IN UNA CELLA.
 *
 * ✅ **I valori di catalogo sono decisi, e questo tipo continua a non inventarli** (`D-307`, 2026-08-31):
 * `Small` **2** · `Medium` **3** · `Large` **4** settori contigui. Vivono in
 * `docs/balance/RT_FootprintCatalog_v0.1.md`, che ne e' l'owner; qui c'e' il PARAMETRO, non il numero, ed e'
 * la ragione per cui il default non e' cambiato.
 *
 * 🔑 **Due sono determinati, il terzo e' una scelta.** `Small` e `Medium` sono l'unica coppia che
 * soddisfa insieme *«>= 2»* (con `1` basta un settore, e un profilo a `1` non distingue nulla), *«<= 3»*
 * (`D-289`: nel gruppo da tre del suo esempio l'unita' standard *«ci sta benissimo»*) e `Small < Medium`.
 * `Large = 4` e' il minimo disponibile sopra `Medium`: nulla nella geometria lo obbliga a fermarsi li'.
 *
 * ⚠️ **Nessuna unita' dichiara ancora un profilo**, quindi oggi userebbero tutte `Medium`. Il default
 * resta l'IDENTITA' — «basta un settore libero» —, che non e' una quarta taglia ma il valore di chi non
 * ha dichiarato niente: la scelta piu' debole possibile, e quindi l'unica che non decide al posto d'altri.
 *
 * Un default piu' alto sarebbe un numero di bilanciamento travestito da costante, ed e' esattamente la
 * classe di difetto che `FRTOccupancyThresholds::BlockedFrom` ha portato dentro questo repository: `6` non
 * l'ha mai scelto nessuno per una regola di gioco, e per un anno ha significato «cella non calpestabile».
 */
USTRUCT(BlueprintType)
struct FRTFootprintProfile
{
	GENERATED_BODY()

	/** Quanti settori liberi CONTIGUI servono. `1` = l'identita': serve un posto qualsiasi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 MinContiguousWedges = 1;

	/**
	 * Il CENTRO della cella deve essere libero.
	 *
	 * 🔑 **Default `false`, ed e' il punto in cui il Decision Record supera `D-179`.** Prima il centro
	 * occupato bloccava la cella da solo e per tutti; ora e' un requisito che un profilo PUO' dichiarare —
	 * un'unita' grande che deve stare a cavallo del centro — e che nessuno paga per conto d'altri. Un muro
	 * che attraversa il centro lasciando sei settori liberi in fila non rende piu' la cella inagibile: la
	 * divide in due lati, che e' un'altra cosa e ha un altro nome (`ERTCoverSide`).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	bool bRequiresFreeCore = false;
};

/** Che cosa produce una copertura: un bordo della cella, o un segmento che le passa dentro. */
UENUM(BlueprintType)
enum class ERTCoverSourceKind : uint8
{
	/** Una `FRTHexCover` su uno dei sei bordi. E' la sorgente che il repository ha da `#621`. */
	Edge,
	/** Un `FRTGeometrySegment` interno alla cella (`URTHexMapAsset::InteriorWalls`, formato v10). */
	InteriorSegment
};

/**
 * LA FACCIA di una sorgente a due lati.
 *
 * ⚠️ `None` non significa «senza copertura»: significa che per QUELLA regione di posa la sorgente non ha
 * due facce distinguibili — o perche' e' un bordo, che dall'interno della cella si usa da un lato solo, o
 * perche' il segmento non separa quella regione e l'unita' puo' girargli intorno restando dov'e'.
 */
UENUM(BlueprintType)
enum class ERTCoverSide : uint8
{
	None,
	/** Il semipiano dei settori `[b .. b+5]`, con `b = URTGeometryGrammarLibrary::AxisBoundaryIndex(Axis)`. */
	A,
	/** Il semipiano opposto, `[b+6 .. b+11]`. */
	B
};

/**
 * IDENTITA' STABILE DI UNA SORGENTE DI COPERTURA.
 *
 * ⚠️ **Non e' un indice d'array, e la differenza e' la ragione per cui questo tipo esiste.** Un indice
 * dentro `Covers` o `InteriorWalls` cambia appena qualcuno cancella la voce precedente, quindi non puo'
 * viaggiare in un intento, in un TurnLog o in un replay. Questa chiave e' fatta SOLO di dato d'autore
 * discreto — lo stesso da cui `FRTGeometrySegment::operator==` deriva l'uguaglianza — e sopravvive a un
 * riordino della collezione.
 *
 * Interamente intera: nessun float attraversa un identificatore che prima o poi finira' in un hash.
 */
USTRUCT(BlueprintType)
struct FRTCoverSourceId
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	ERTCoverSourceKind Kind = ERTCoverSourceKind::Edge;

	/**
	 * `Edge`: il valore di `ERTHexDirection`. `InteriorSegment`: il valore di `ERTTacticalAxis`.
	 *
	 * I due vocabolari non si mescolano perche' `Kind` li separa: leggere questo campo senza aver letto
	 * quello e' l'unico modo di sbagliarsi, ed e' il motivo per cui il campo non si chiama `Direction`.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	uint8 AxisOrEdge = 0;

	/** `InteriorSegment`: `FRTGeometrySegment::Offset` in quanti. `Edge`: sempre `0`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 Offset = 0;

	/** `InteriorSegment`: il minore dei due estremi. `Edge`: sempre `0`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 AlongMin = 0;

	/** `InteriorSegment`: il maggiore dei due estremi. `Edge`: sempre `0`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 AlongMax = 0;

	bool operator==(const FRTCoverSourceId& O) const
	{
		return Kind == O.Kind && AxisOrEdge == O.AxisOrEdge && Offset == O.Offset
			&& AlongMin == O.AlongMin && AlongMax == O.AlongMax;
	}
	bool operator!=(const FRTCoverSourceId& O) const { return !(*this == O); }

	FString ToString() const
	{
		return Kind == ERTCoverSourceKind::Edge
			? FString::Printf(TEXT("Edge(%u)"), AxisOrEdge)
			: FString::Printf(TEXT("Seg(ax=%u,off=%d,%d..%d)"), AxisOrEdge, Offset, AlongMin, AlongMax);
	}
};

FORCEINLINE uint32 GetTypeHash(const FRTCoverSourceId& S)
{
	uint32 H = GetTypeHash(static_cast<uint8>(S.Kind));
	H = HashCombine(H, GetTypeHash(S.AxisOrEdge));
	H = HashCombine(H, GetTypeHash(S.Offset));
	H = HashCombine(H, GetTypeHash(S.AlongMin));
	return HashCombine(H, GetTypeHash(S.AlongMax));
}

/**
 * UN MODO LEGALE, PER UN'UNITA' IN QUESTA CELLA, DI USARE UNA SORGENTE.
 *
 * 🔑 **`OccupiedWedge != Cover`**, ed e' l'invariante che questa struct esiste per tenere separata: la
 * maschera dei settori solidi dice DOVE la geometria sta, questa dice CHE COSA se ne puo' fare. Una roccia
 * puo' occupare tre settori senza offrire riparo, e un muretto puo' riparare un arco piu' largo del suo
 * ingombro.
 *
 * Una cella espone zero, una o piu' opzioni. Sceglierne una NON aggiunge un occupante.
 */
USTRUCT(BlueprintType)
struct FRTCoverOption
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	FRTCoverSourceId Source;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	ERTCoverSide Side = ERTCoverSide::None;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	ERTHexCoverType Type = ERTHexCoverType::Low;

	/**
	 * I SETTORI LIBERI DA CUI QUESTA OPZIONE SI USA — non l'intera regione, ma la parte di regione che sta
	 * dalla parte giusta della sorgente.
	 *
	 * ⚠️ **E' la meta' che rende l'opzione non teletrasportabile.** Le due facce di un muro continuo hanno
	 * `AccessMask` disgiunte **e in regioni diverse**: chi sta sulla prima non arriva alla seconda
	 * scegliendola, e a dirlo e' `ClassifyIntraCellTraversal` invece della speranza.
	 *
	 * ⚠️ **Disgiunte non significa irraggiungibili**, e la distinzione e' esattamente quella che il
	 * Decision Record chiede all'editor di mostrare separata. Un muro dal centro a un vertice espone
	 * anch'esso `A` e `B` con maschere disgiunte, ma le due stanno nella **stessa** regione libera: li'
	 * l'unita' passa dall'una all'altra girando attorno all'estremo, ed e' un percorso reale, non una
	 * scorciatoia.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 AccessMask = 0;
};

/**
 * PASSARE DA UNA POSA ALL'ALTRA DENTRO LA STESSA CELLA.
 *
 * ⚠️ **Due valori, entrambi raggiungibili.** Il Decision Record prevede anche la traversata AUTORATA —
 * porta, apertura, vault, reposition autorizzato — ma in v0.1 nessun vocabolario la esprime: `FRTHexDoor`
 * sta sui BORDI, e non esiste un dato per «apertura dentro la cella». Un terzo valore che nessun produttore
 * puo' emettere sarebbe un campo che nessuno legge, ed e' il difetto che questo repository ha gia' pagato
 * quattro volte. Va aggiunto **in coda** insieme al suo produttore.
 */
UENUM(BlueprintType)
enum class ERTIntraCellTraversal : uint8
{
	/** Le due pose stanno nella STESSA regione libera: ci si sposta senza attraversare geometria. */
	SameRegion,
	/** Regioni diverse, e nessuna traversata le collega: la transizione e' INVALIDA. */
	Blocked,

	/**
	 * Regioni diverse, ma un muro **scavalcabile** le separa: la transizione e' valida e **paga** — `E23.7`,
	 * [D-308], `#1828`.
	 *
	 * 🔑 **E' il terzo valore che questo enum aspettava, e arriva col suo produttore nello stesso commit.**
	 * La riga qui sopra prometteva *«va aggiunto in coda insieme al suo produttore»*, e il produttore e'
	 * `FRTHexInteriorWall::bTraversable`: senza di lui il valore sarebbe un'etichetta che nessun ramo emette,
	 * cioe' il difetto che questo repository ha gia' pagato quattro volte.
	 *
	 * ⚠️ **Non e' `SameRegion` con un altro nome**, e la distinzione e' cio' che il chiamante deve vedere: li'
	 * non si attraversa niente e non si paga; qui si scavalca, e `D-308` fissa il costo del vault a *«costo
	 * d'arco normale + 1 MP»*. Schiacciare i due valori renderebbe gratuito cio' che ha un prezzo.
	 *
	 * ⛔ **Non introduce un secondo slot di occupancy**: la capacita' della cella resta **una** unita'
	 * ([D-289]). Scavalcare porta all'altra faccia, non in due posti.
	 */
	AuthoredTraversal
};

UCLASS()
class REFACTORTACTICS_API URTHexCoverPlacementLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * LE REGIONI DI POSA della cella: i gruppi massimali di settori liberi CONTIGUI, in ordine canonico di
	 * `FirstWedge` crescente.
	 *
	 * Deterministica e indipendente dall'ordine per costruzione: legge una maschera di dodici bit e la
	 * percorre da `0` a `11`, quindi non c'e' input da permutare. Cella interamente libera -> una regione
	 * da dodici; cella interamente occupata -> nessuna regione.
	 *
	 * ⚠️ `bCoreBlocked` **non** entra qui: il centro non e' un settore, e la sua richiesta appartiene al
	 * profilo di footprint (`bRequiresFreeCore`), non alla forma dello spazio libero.
	 */
	static void ComputeFreeRegions(const FRTOccupancyMask& Mask, TArray<FRTPlacementRegion>& OutRegions);

	/**
	 * ESISTE UNA POSA LEGALE per questo footprint? E' il predicato che sostituisce
	 * `Classify(...) != Blocked` come autorita' sulla calpestabilita'.
	 *
	 * ⚠️ **Non e' l'ultima parola sull'ingresso in cella**, ed e' voluto: `FRTHexCellData::bBlocksMovement`
	 * resta la scelta d'autore diretta e vince comunque, e l'occupazione da parte di un'altra unita' e' di
	 * `URTHexSimLibrary`. Questa funzione risponde a *«ci sta?»*, non a *«e' libera?»* — che sono le due
	 * domande che `ERTHexWaypointReason` gia' distingue con `BlocksMovement` e `Occupied`.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static bool HasLegalPlacement(const FRTOccupancyMask& Mask, const FRTFootprintProfile& Footprint);

	/**
	 * L'indice della regione che accoglie il footprint, o `INDEX_NONE`.
	 *
	 * A parita' di idoneita' vince la regione con `FirstWedge` minore — dichiarato, arbitrario per chi
	 * gioca, e stabile: non «la prima che capita», che dipenderebbe da come e' costruito l'array.
	 */
	static int32 FindPlacementRegion(const TArray<FRTPlacementRegion>& Regions,
		const FRTFootprintProfile& Footprint);

	/** L'indice della regione che contiene quel settore, o `INDEX_NONE` se il settore e' occupato. */
	static int32 RegionIndexForWedge(const TArray<FRTPlacementRegion>& Regions, int32 Wedge);

	/**
	 * I DUE SEMIPIANI di un asse tattico, in settori.
	 *
	 * Il semipiano `A` sono i sei settori `[b .. b+5]` con `b = AxisBoundaryIndex(Axis)`, `B` i sei
	 * opposti. La corrispondenza fra assi e confini non e' riscritta qui: si chiede a
	 * `URTGeometryGrammarLibrary`, che ne e' l'unico posto — un angolo trascritto a mano mentirebbe in
	 * silenzio, ed e' il difetto che `#588` ha pagato.
	 */
	static void AxisHalfPlanes(ERTTacticalAxis Axis, int32& OutMaskA, int32& OutMaskB);

	/**
	 * IL SEMIPIANO IN CUI CADE UN SETTORE, rispetto all'asse. `A`, `B`, e mai `None`: un settore sta da
	 * una parte o dall'altra, ed e' la regione — che di settori ne ha molti — a poter stare a cavallo.
	 */
	static ERTCoverSide SideOfWedge(int32 Wedge, ERTTacticalAxis Axis);

	/**
	 * TUTTE LE OPZIONI DI COPERTURA della cella: una per ogni modo legale di usare una sorgente.
	 *
	 * ```text
	 * Cell.Covers[]      ->  ERTCoverSourceKind::Edge             Side = None    una per regione ADIACENTE al bordo
	 * InteriorWalls[]    ->  ERTCoverSourceKind::InteriorSegment  Side = A | B   una per faccia e per regione
	 * ```
	 *
	 * 🔑 **Le due facce restano due opzioni anche quando l'unita' puo' girare attorno al muro**, ed e' la
	 * correzione che il Decision Record chiede: *«valida»* e *«raggiungibile dal lato in cui mi trovo»* sono
	 * due domande, e schiacciarle in una sola nasconde proprio il caso da mostrare. La prima e' questa
	 * funzione, la seconda e' `ClassifyIntraCellTraversal`.
	 *
	 * ⚠️ **Un bordo produce un'opzione solo per le regioni che lo toccano**, dove «toccare» include
	 * l'adiacenza: le rocce che occupano i due settori del bordo non isolano da esso chi sta accanto — un
	 * muro continuo che taglia la cella si'. E' il caso che tiene la regola stretta senza renderla falsa.
	 *
	 * Ordine canonico e stabile: prima i bordi in ordine di `ERTHexDirection`, poi i segmenti nell'ordine
	 * in cui `Segments` li porta, e per ciascuna sorgente le regioni per `FirstWedge` crescente e le facce
	 * `A` prima di `B`. Nessun `TMap` attraversa questa funzione.
	 *
	 * ⚠️ **Le coperture con `Type == None` non producono opzioni**: una voce cosi' e' dato incoerente, e
	 * chi la rifiuta e' `URTHexMapAsset::ValidateMap`. Qui viene ignorata invece di essere corretta — non
	 * e' compito di un enumeratore riparare l'asset.
	 *
	 * ⚠️ **Non tocca ne' legge `bBlocksLineOfSight`.** Che una geometria fermi vista o proiettili e' una
	 * domanda separata con un altro owner (`D-269`), e la separazione e' il punto: scegliere la roccia a
	 * nord come riparo attivo non rende l'albero a sud intangibile.
	 *
	 * ⛔ **Approssimazione DICHIARATA, e non e' una svista.** Un segmento produce un'opzione per OGNI
	 * regione, senza chiedersi se quella regione gli sia abbastanza vicina da potercisi appoggiare. La
	 * distanza fra posa e sorgente e' esattamente cio' che un `CoverAnchor` misurerebbe, e la politica
	 * degli anchor — autorati, generati o ibridi — e' una **decisione aperta** del Decision Record: sceglierla
	 * qui significherebbe deciderla per inerzia. Il verso dell'approssimazione e' quello sicuro per un
	 * enumeratore — offre in piu', mai in meno — e chi accetta resta la validazione autoritativa.
	 */
	static void EnumerateCoverOptions(const FRTHexCellData& Cell, const TArray<FRTGeometrySegment>& Segments,
		const FRTOccupancyMask& Mask, TArray<FRTCoverOption>& OutOptions);

	/**
	 * SI PUO' PASSARE da un settore all'altro restando in questa cella?
	 *
	 * `SameRegion` se i due settori appartengono alla stessa regione libera, `Blocked` altrimenti — e
	 * `Blocked` include il settore occupato, dove non c'e' posa da cui partire o a cui arrivare.
	 *
	 * 🔑 **E' la funzione che impedisce alla scelta della faccia di diventare un teletrasporto.** Due
	 * opzioni sulle due facce di un muro continuo stanno in regioni diverse: chi e' su `SideA` non arriva a
	 * `SideB` scegliendolo, deve uscire dalla cella e rientrare da un percorso reale del grafo.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static ERTIntraCellTraversal ClassifyIntraCellTraversal(const FRTOccupancyMask& Mask,
		int32 FromWedge, int32 ToWedge);

	/**
	 * COME SOPRA, ma sapendo quali muri un autore ha reso **scavalcabili** — `E23.7`, [D-308], `#1828`.
	 *
	 * `Traversable` e' la maschera che la cella avrebbe **senza** i muri marcati `bTraversable`: se i due
	 * settori sono separati in `Mask` e uniti in `Traversable`, a separarli e' **solo** geometria che si puo'
	 * scavalcare, e la risposta e' `AuthoredTraversal`.
	 *
	 * 🔑 **Due maschere e non una lista di muri, ed e' la scelta che tiene questa libreria pura.** Qui non
	 * entra ne' l'asset ne' `FRTCellId`: chi chiama sa quali muri sono scavalcabili e costruisce le due
	 * maschere con `ComputeMask`, che e' gia' l'unico produttore. Passare i segmenti significherebbe
	 * ricalcolare qui dentro cio' che il chiamante ha gia' in mano, e dare a questa funzione una seconda
	 * ragione di esistere.
	 *
	 * ⛔ **L'AC 5 di `#1828` cade da se' con questa forma**: *«nessuna traversata autorata puo' collegare due
	 * regioni attraversando geometria bloccante»*. Se fra le due regioni ci fosse **anche** un muro non
	 * scavalcabile, toglierne uno solo non le unirebbe — `Traversable` resterebbe separata e la risposta
	 * `Blocked`. Non serve un validator che lo vieti: il modello non sa esprimerlo.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static ERTIntraCellTraversal ClassifyIntraCellTraversalWithAuthored(const FRTOccupancyMask& Mask,
		const FRTOccupancyMask& Traversable, int32 FromWedge, int32 ToWedge);

	/**
	 * L'opzione e' RAGGIUNGIBILE da chi e' posato su quel settore, senza uscire dalla cella?
	 *
	 * ⚠️ **Non e' l'appartenenza a `AccessMask`, ed e' la distinzione che l'editor deve mostrare.** Chi sta
	 * dietro un raggio centro-vertice non e' *sull*'altra faccia, ma ci **arriva** girando attorno
	 * all'estremo: stessa regione libera, percorso reale, opzione raggiungibile. Chi sta dietro un muro
	 * continuo no, ed e' la stessa funzione a dire di no — perche' li' le regioni sono due.
	 *
	 * Vero se `AccessMask` interseca la regione libera che contiene `FromWedge`.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static bool IsOptionReachableFromWedge(const FRTOccupancyMask& Mask, const FRTCoverOption& Option,
		int32 FromWedge);
};
