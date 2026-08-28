#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Perception/RTPerceptionLibrary.h" // ERTAwareness, FRTPerceiver
#include "RTTeamKnowledge.generated.h"

class URTHexMapAsset;

/**
 * Il ricordo di un contatto: DOVE una squadra ha visto un'unita' avversaria l'ultima volta, e QUANDO.
 *
 * ⚠️ **L'identita' e' `ARTUnit::StableUnitId`, non l'indice di fase.** Gli indici che il resolver usa dentro
 * un turno (`FRTHexCombatUnit::UnitId`, `FRTHexSimUnit::UnitId`) sono posizioni in un array **ordinato per
 * cella**: cambiano appena un'unita' si muove, e differiscono anche fra due fasi dello stesso turno, perche'
 * lo snapshot del movimento scarta i morti e quello del Blast no. Un ricordo indicizzato cosi' punterebbe,
 * il turno dopo, a un'unita' diversa — e nessun test lo direbbe, perche' l'indice resterebbe valido.
 * La memoria attraversa i turni: solo un'identita' che li attraversa anche lei puo' esserne la chiave.
 *
 * ⚠️ `Cell` e' la cella del CONTATTO, non la posizione attuale. E' il punto di tutta la struttura: un ricordo
 * che si aggiornasse da solo non sarebbe un ricordo, sarebbe la vista con un altro nome.
 */
USTRUCT(BlueprintType)
struct FRTLastKnownContact
{
	GENERATED_BODY()

	/** Di CHI e' il ricordo: `ARTUnit::StableUnitId`, che sopravvive a movimento, fasi e turni. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	int32 StableUnitId = INDEX_NONE;

	/** Dove e' stata vista l'ultima volta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	FRTCellId Cell;

	/** In quale turno. E' il campo che fa scadere il ricordo: senza, «un turno» non sarebbe misurabile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	int32 TurnNumber = 0;

	FRTLastKnownContact() = default;
	FRTLastKnownContact(int32 InStableUnitId, const FRTCellId& InCell, int32 InTurn)
		: StableUnitId(InStableUnitId), Cell(InCell), TurnNumber(InTurn) {}
};

/**
 * Cosa una squadra sa, a un dato turno (CP 13.2). Formato **versionato**, come il TurnLog e per la stessa
 * ragione: e' stato di gioco che entra nello snapshot, quindi un replay lo rilegge, e un campo aggiunto senza
 * versione renderebbe illeggibili le tracce gia' scritte.
 *
 * La conoscenza e' di SQUADRA ([D-043]): non esiste una relazione «chi vede chi» unita' per unita'. Questo e'
 * anche il motivo per cui l'avvistamento di un alleato estende il targeting di tutti — non e' una regola in
 * piu', e' una conseguenza di dove vive il dato.
 */
USTRUCT(BlueprintType)
struct FRTTeamKnowledge
{
	GENERATED_BODY()

	/**
	 * Versione del FORMATO, non del contenuto. Si incrementa quando cambia la forma della struttura, e chi
	 * rilegge una traccia con una versione che non conosce deve rifiutarla, non indovinarla.
	 */
	static constexpr int32 CurrentVersion = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	int32 Version = CurrentVersion;

	/** La squadra che POSSIEDE questa conoscenza. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	int32 TeamId = 0;

	/** Il turno a cui questa fotografia si riferisce. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	int32 TurnNumber = 0;

	/** Cio' che la squadra VEDE ora, in ordine stabile: l'unione dei suoi osservatori. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	TArray<FRTCellId> VisibleCells;

	/**
	 * Il terreno che la squadra ha GIA' VISTO almeno una volta, ordinato con `URTHexLibrary::StableLess`
	 * ([D-227]). E' un sovrainsieme di `VisibleCells`, e la differenza fra i due e' esattamente cio' che il
	 * velo di `#1467` disegna in penombra invece che nascondere.
	 *
	 * ⚠️ **Non scade, e l'asimmetria con `Contacts` e' deliberata.** Un contatto vive
	 * `ContactLifetimeTurns` turni perche' **l'unita' si muove**: un ricordo che gli sopravvivesse sarebbe la
	 * vista sotto mentite spoglie. Il terreno non si muove — dimenticarlo sarebbe una regola nuova, non una
	 * conseguenza. Dare a questo campo la scadenza dei contatti e' la simmetria che viene naturale scrivere,
	 * ed e' il difetto che `Perception.KnowledgeRemembersExploredCells` esiste per prendere.
	 *
	 * ⛔ **Cresce in modo monotono**, fino alle 7 651 celle dell'arena di raggio 50 e per squadra, e
	 * viaggia in ogni snapshot perche' `FRTTeamKnowledge` sta in `FRTHexSim`. Il costo e' **accettato** in
	 * [D-227]: se diventasse misurabile la risposta e' una rappresentazione compressa, **mai** una scadenza.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	TArray<FRTCellId> ExploredCells;

	/** I ricordi vivi, ordinati per `StableUnitId` (ordine stabile, mai quello di scoperta). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	TArray<FRTLastKnownContact> Contacts;

	FRTTeamKnowledge() = default;
};

/**
 * Cosa un attaccante puo' fare del proprio bersaglio, dato cio' che la sua squadra sa.
 *
 * Tre esiti e non due, perche' `Uncertain` non e' «mezza ignoranza»: e' un'informazione posizionale senza
 * identita', e produce un tipo di attacco diverso — sulla CELLA. Comprimerlo su `Rejected` cancellerebbe il
 * canale acustico prima ancora che nasca (CP 13.3/13.4); comprimerlo su `Allowed` permetterebbe di inseguire
 * per identita' un'unita' che non si vede.
 */
UENUM(BlueprintType)
enum class ERTTargetKnowledge : uint8
{
	/** La squadra vede il bersaglio: si puo' mirare all'UNITA'. */
	Allowed,
	/** Solo un ricordo o un contatto incerto: si puo' mirare alla CELLA, mai all'unita'. */
	CellOnly,
	/** Ignoto alla squadra: non e' un bersaglio. Decide il fallback DICHIARATO dall'azione. */
	Rejected
};

/**
 * La conoscenza di squadra come DATO, e la regola che il targeting ne ricava (CP 13.2).
 *
 * Tutto puro e headless: nessun Actor, nessun `UWorld`. E' la condizione perche' la regola sia verificabile
 * senza montare una partita — e perche' il server possa applicarla senza che il client abbia voce.
 */
UCLASS()
class REFACTORTACTICS_API URTTeamKnowledgeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Per quanti turni un contatto sopravvive senza essere rinfrescato. **1**, come la DoD di CP 13.2: il
	 * ricordo dura il turno successivo a quello dell'avvistamento, poi scade.
	 *
	 * Un numero e non una costante per eroe: la memoria di squadra e' una regola del turno, e differenziarla
	 * per personaggio sarebbe un branch per eroe nel core (invariante #7).
	 */
	static constexpr int32 ContactLifetimeTurns = 1;

	/**
	 * La conoscenza della squadra a questo turno, a partire da quella del turno precedente.
	 *
	 * `Observers` sono i membri VIVI della squadra. `EnemiesNow` sono le unita' avversarie con la loro cella
	 * ATTUALE: la struttura e' riusata per comodita' e il suo `TurnNumber` in ingresso e' ignorato — lo scrive
	 * questa funzione, che e' l'unica a sapere quando l'avvistamento avviene.
	 *
	 * Chi e' visto ora produce un contatto FRESCO (che sovrascrive il ricordo); chi non lo e' conserva il
	 * proprio ricordo finche' non scade.
	 *
	 * `Previous` con una `Version` diversa da `CurrentVersion` viene **ignorata**, non reinterpretata: una
	 * memoria letta male e' peggio di nessuna memoria, perche' nessuno se ne accorge.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Perception")
	static FRTTeamKnowledge Observe(const URTHexMapAsset* Map, int32 TeamId, int32 TurnNumber,
		const TArray<FRTPerceiver>& Observers, const TArray<FRTLastKnownContact>& EnemiesNow,
		const FRTTeamKnowledge& Previous);

	/**
	 * Quanto la squadra sa di una certa unita', che si trova ORA in `CurrentCell`.
	 *
	 * `Detected` se la sua cella attuale e' vista; `Uncertain` se ne resta solo un ricordo vivo; `Hidden`
	 * altrimenti. Non si consulta la posizione reale per decidere `Uncertain`: e' proprio il punto.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Perception")
	static ERTAwareness AwarenessOfUnit(const FRTTeamKnowledge& Knowledge, int32 StableUnitId,
		const FRTCellId& CurrentCell);

	/** La cella dell'ultimo contatto con `StableUnitId`; `false` se non ce n'e' uno vivo. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Perception")
	static bool LastKnownCell(const FRTTeamKnowledge& Knowledge, int32 StableUnitId, FRTCellId& OutCell);

	/**
	 * La regola del targeting (CP 13.2), in una funzione pura e in un posto solo.
	 *
	 * Un alleato non e' mai «ignoto»: si cura e si protegge chi si conosce per appartenenza, non per
	 * avvistamento — e la conoscenza di squadra non contiene se stessa.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Perception")
	static ERTTargetKnowledge ClassifyTarget(const FRTTeamKnowledge& Knowledge, int32 TargetStableUnitId,
		int32 TargetTeamId, const FRTCellId& TargetCurrentCell);
};
