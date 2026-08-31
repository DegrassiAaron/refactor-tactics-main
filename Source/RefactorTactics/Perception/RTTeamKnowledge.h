#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Perception/RTPerceptionLibrary.h" // ERTAwareness, FRTPerceiver
#include "RTTeamKnowledge.generated.h"

class URTHexMapAsset;
struct FRTKnowledgeSubject;   // `RTKnowledgeView.h`: il soggetto di cui si valuta la conoscibilita'

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
 * Chi puo' leggere un fatto gia' accaduto: un bit per squadra, deciso QUANDO il fatto e' accaduto.
 *
 * E' l'artefatto di [D-223]. Un canale che racconta il passato — il combat log, la traccia post-lock — non
 * puo' essere filtrato al momento della lettura: la conoscenza di allora non esiste piu', e il soggetto
 * potrebbe essere stato distrutto. Porta quindi con se' la risposta, calcolata mentre era ancora
 * calcolabile.
 *
 * 🔴 **Il default e' fail-closed, ed e' il punto.** `Mask = 0` significa «nessuno lo vede», non «non ci ho
 * pensato»: un verdetto dimenticato nasconde una riga invece di regalarla. E' l'opposto del default di
 * `AddLogEvent` che `#1499` rimuove, e la direzione e' deliberata — un filtro di privacy sbaglia dalla parte
 * del silenzio.
 *
 * ⚠️ **Un fatto di MONDO non ha maschera vuota: ha `Everyone()`.** Le due cose non vanno confuse — una
 * superficie che scade riguarda tutti, e dichiararlo e' diverso dal non dichiarare nulla.
 */
USTRUCT()
struct FRTKnowledgeVerdict
{
	GENERATED_BODY()

	/**
	 * Un bit per `TeamId`: il bit `n` acceso significa «la squadra `n` puo' leggere questo fatto».
	 *
	 * ⚠️ **La larghezza non e' una costante di gioco.** Il formato di v0.1 ha due squadre, ma
	 * `TeamKnowledgeState` si dimensiona sui `TeamId` VIVI e il 4v4 e' un cambio di DATO, non di codice:
	 * scrivere `2` da qualche parte sarebbe un numero che invecchia da solo. Il bound e' quello del tipo, e
	 * `MaxTeamId` lo dichiara.
	 */
	UPROPERTY()
	uint32 Mask = 0;

	/** Il `TeamId` piu' alto rappresentabile. Oltre, `AllowsTeam` risponde `false` invece di leggere fuori. */
	static constexpr int32 MaxTeamId = 31;

	/** Un fatto che riguarda tutti: superfici, ponti, marker di turno, fine partita. */
	static FRTKnowledgeVerdict Everyone()
	{
		FRTKnowledgeVerdict V;
		V.Mask = ~0u;
		return V;
	}

	/** Nessuno: e' anche il default, e serve nominarlo dove la scelta e' deliberata. */
	static FRTKnowledgeVerdict NoOne()
	{
		return FRTKnowledgeVerdict();
	}

	void AllowTeam(int32 TeamId)
	{
		if (TeamId >= 0 && TeamId <= MaxTeamId)
		{
			Mask |= (1u << TeamId);
		}
	}

	/**
	 * ⚠️ Fail-closed su un `TeamId` fuori intervallo: un osservatore che il verdetto non sa rappresentare
	 * non legge, invece di leggere il bit di qualcun altro.
	 */
	bool AllowsTeam(int32 TeamId) const
	{
		if (TeamId < 0 || TeamId > MaxTeamId)
		{
			return false;
		}
		return (Mask & (1u << TeamId)) != 0;
	}

	bool operator==(const FRTKnowledgeVerdict& Other) const { return Mask == Other.Mask; }
	bool operator!=(const FRTKnowledgeVerdict& Other) const { return !(*this == Other); }
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
	 * Il contatto COMPLETO (cella e turno) per `StableUnitId`; `nullptr` se non ce n'e' uno vivo.
	 *
	 * Non e' un `UFUNCTION` (ritorna un puntatore, come `URTKnowledgeViewLibrary::FindEntry`): e' l'unica
	 * ricerca nell'array `Contacts`, e `LastKnownCell` la chiama invece di ripetere il proprio ciclo — cosi'
	 * un consumatore che ha bisogno anche del `TurnNumber` (la sagoma del ricordo, CP 13.5) non deve
	 * duplicare la logica di ricerca.
	 */
	static const FRTLastKnownContact* FindContact(const FRTTeamKnowledge& Knowledge, int32 StableUnitId);

	/**
	 * La regola del targeting (CP 13.2), in una funzione pura e in un posto solo.
	 *
	 * Un alleato non e' mai «ignoto»: si cura e si protegge chi si conosce per appartenenza, non per
	 * avvistamento — e la conoscenza di squadra non contiene se stessa.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Perception")
	static ERTTargetKnowledge ClassifyTarget(const FRTTeamKnowledge& Knowledge, int32 TargetStableUnitId,
		int32 TargetTeamId, const FRTCellId& TargetCurrentCell);

	/**
	 * Il verdetto di [D-223]: chi, fra le squadre che conoscono qualcosa, puo' leggere un fatto su questo
	 * soggetto — deciso ADESSO, mentre il soggetto e' ancora osservabile.
	 *
	 * 🔴 **E' la prima definizione della regola, non una seconda.** Chiama `ClassifyTarget`, la stessa
	 * funzione da cui `ViewForTeam` ricava `Live`: `Allowed` e `Live` coincidono per costruzione, non per
	 * somiglianza. Riscrivere qui il confronto sarebbe la terza via che [D-223] vieta.
	 *
	 * ⚠️ **Il ramo alleato passa da `ClassifyTarget`, non da un `if` qui**: una squadra conosce sempre i
	 * propri, e quella regola vive gia' dentro la funzione che classifica.
	 *
	 * ⚠️ **Fail-closed per costruzione**: una squadra assente da `AllKnowledge` non riceve il bit. Se la
	 * lista arriva vuota il verdetto e' `NoOne()`, e la riga non si legge — mai il contrario.
	 */
	static FRTKnowledgeVerdict FreezeVerdict(const TArray<FRTTeamKnowledge>& AllKnowledge,
		const FRTKnowledgeSubject& Subject);

	/**
	 * Quante celle iniziali di una rotta l'osservatore ha il diritto di vedere: la regola di troncamento
	 * di [D-223], in **una** funzione pura.
	 *
	 * 🔴 **Tronca, non filtra: si ferma alla prima cella negata e non riprende.** Riprendere dopo un buco
	 * unirebbe due celle NON adiacenti — per la traccia con un segmento dritto sopra il tratto da
	 * nascondere, per il modello con uno scatto attraverso di esso. In entrambi i casi e' **peggio che non
	 * filtrare affatto**, perche' il salto disegna proprio cio' che il verdetto negava.
	 *
	 * 🔴 **Esiste per essere chiamata da DUE consumatori, ed e' questo il punto.**
	 * `ARTTurnManager::VisibleTrailFor` la usa per la traccia post-lock (`#1497`) e `BeginPlayback` per il
	 * modello animato (`#1525`): sono le due meta' degli errori speculari che [D-223] nomina, e finche' la
	 * regola stava in un solo posto la seconda meta' e' rimasta aperta. Due copie della stessa condizione
	 * divergerebbero, e la contraddizione — traccia troncata mentre il modello prosegue — tornerebbe.
	 *
	 * ⚠️ **Fail-closed sul disallineamento**: se i verdetti non sono tanti quante le celle risponde `0`.
	 * Una rotta malformata non si mostra, invece di leggere fuori dall'array o di indovinare.
	 *
	 * ⚠️ **Non e' una `UFUNCTION`**, come `FreezeVerdict` e per la stessa ragione: `FRTKnowledgeVerdict`
	 * non e' un tipo Blueprint. Una regola di privacy raggiungibile da Blueprint sarebbe una regola di
	 * privacy **aggirabile** da Blueprint.
	 *
	 * @return il numero di celle iniziali consentite: `0` = niente, `Cells.Num()` = tutta la rotta.
	 */
	static int32 ObservedPrefixLength(const TArray<FRTCellId>& Cells,
		const TArray<FRTKnowledgeVerdict>& CellVerdicts, int32 ObserverTeamId);
};
