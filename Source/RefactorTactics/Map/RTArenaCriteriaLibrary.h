#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "RTArenaCriteriaLibrary.generated.h"

class URTHexMapAsset;

/**
 * Esito di UN criterio: passa o no, **e i numeri con cui e' stato deciso**.
 *
 * Il booleano da solo non basta: chi costruisce la mappa deve sapere *di quanto* ha mancato il bersaglio, o
 * correggere diventa tentativo ed errore. «Non passa» non dice se togliere una cella di fango o dieci.
 */
USTRUCT(BlueprintType)
struct FRTArenaCriterionResult
{
	GENERATED_BODY()

	/** Vero se il criterio e' soddisfatto. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Arena")
	bool bPassed = false;

	/** Perche' si', o perche' no — in una riga leggibile, coi numeri dentro. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Arena")
	FString Detail;

	/**
	 * Vero se il criterio non e' stato **valutabile** (mappa nulla, spawn assenti, nessun percorso): distinto
	 * da `bPassed == false`, che invece e' un verdetto. Una mappa senza percorso fra gli spawn non «fallisce il
	 * criterio delle rotte»: e' rotta prima, e dirlo con lo stesso simbolo nasconderebbe il difetto vero.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Arena")
	bool bNotEvaluable = false;

	static FRTArenaCriterionResult Pass(const FString& InDetail)
	{
		FRTArenaCriterionResult R; R.bPassed = true; R.Detail = InDetail; return R;
	}
	static FRTArenaCriterionResult Fail(const FString& InDetail)
	{
		FRTArenaCriterionResult R; R.bPassed = false; R.Detail = InDetail; return R;
	}
	static FRTArenaCriterionResult NotEvaluable(const FString& InDetail)
	{
		FRTArenaCriterionResult R; R.bPassed = false; R.bNotEvaluable = true; R.Detail = InDetail; return R;
	}
};

/** Verdetto sui tre criteri dell'arena (DoD di U1), piu' gli spawn su cui e' stato dato. */
USTRUCT(BlueprintType)
struct FRTArenaCriteriaReport
{
	GENERATED_BODY()

	/** Passo 3: copertura sulla linea fra gli spawn. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Arena")
	FRTArenaCriterionResult Coverage;

	/** Passo 4: la zona costosa morde il budget. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Arena")
	FRTArenaCriterionResult CostBite;

	/** Passo 7: due rotte con trade-off diverso. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Arena")
	FRTArenaCriterionResult TwoRoutes;

	/** Passo 5: ogni cella percorribile e' raggiungibile dagli spawn. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Arena")
	FRTArenaCriterionResult Reachability;

	/** Spawn su cui i criteri sono stati valutati (derivati dalla mappa, non inventati). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Arena")
	FRTCellId SpawnA;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Arena")
	FRTCellId SpawnB;

	/** Vero se tutti e tre passano. Un criterio non valutabile NON conta come passato. */
	bool AllPassed() const
	{
		return Coverage.bPassed && CostBite.bPassed && TwoRoutes.bPassed && Reachability.bPassed;
	}
};

/**
 * Misura i **tre criteri dell'arena** che il `done_when` di U1 dichiara (`docs/roadmap/editor-sessions.yaml`,
 * passi 3, 4 e 7). Pura, deterministica, headless: nessun Actor, nessun World, nessun editor.
 *
 * Esiste perche' quei criteri erano scritti in modo misurabile ma **non misurati**: restavano un giudizio di
 * chi costruiva, applicabile solo con l'editor aperto. U13 estendera' quell'arena e U19 la misurera': un
 * criterio che nessuno riapplica quando la mappa cambia non e' un criterio, e' un ricordo.
 *
 * Non decide cosa sia una buona mappa — decide se una mappa rispetti cio' che U1 ha dichiarato di volere.
 */
UCLASS()
class REFACTORTACTICS_API URTArenaCriteriaLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Budget Move di un turno usato come riferimento quando il chiamante non ne passa uno (`MovePoints` di catalogo). */
	static constexpr int32 DefaultMoveBudget = 5;

	/** Fattore massimo fra il costo della rotta lunga e quello della corta (U1 passo 7). Proposto, non misurato. */
	static constexpr float DefaultMaxCostRatio = 1.5f;

	/**
	 * Scarto minimo di esposizione fra le due rotte perche' il trade-off conti (0..1).
	 *
	 * Serve perche' senza soglia bastava **una cella**: `GeneratedTestArena` passava con 57% contro 56%, che
	 * non e' una scelta ma rumore. Come il fattore 1,5: proposto, non ricavato da una misura.
	 */
	static constexpr float DefaultMinExposureGap = 0.15f;

	/**
	 * **Passo 3 — copertura.** Vero se almeno **due** celle che bloccano la vista stanno sul segmento fra gli
	 * spawn, e la linea di tiro fra i due e' effettivamente interrotta.
	 *
	 * Le due condizioni non sono ridondanti: due celle `bBlocksLineOfSight` possono stare sul segmento senza
	 * interromperlo (gli estremi non bloccano mai, e la regola di elevazione lascia sparare da un layer
	 * superiore). Contare le celle senza chiedere a `HasLineOfSight` misurerebbe l'intenzione invece del
	 * risultato.
	 */
	static FRTArenaCriterionResult CheckCoverage(const URTHexMapAsset* Map, const FRTCellId& SpawnA,
		const FRTCellId& SpawnB);

	/**
	 * **Passo 4 — il costo morde.** Vero se il percorso a costo minimo fra gli spawn attraversa almeno una cella
	 * a costo > 1 **e** costa piu' di `MoveBudget`.
	 *
	 * Le due condizioni insieme dicono «la zona costosa e' una scelta»: un percorso caro perche' semplicemente
	 * lungo non mette davanti a nulla, e una zona cara che si aggira senza rinunciare a niente nemmeno.
	 */
	static FRTArenaCriterionResult CheckCostBite(const URTHexMapAsset* Map, const FRTCellId& SpawnA,
		const FRTCellId& SpawnB, int32 MoveBudget = DefaultMoveBudget);

	/**
	 * **Passo 7 — due rotte con trade-off diverso.** Vero se esistono due percorsi fra gli spawn che:
	 * non condividono celle oltre agli estremi, hanno costo entro `MaxCostRatio` l'uno dall'altro, e la rotta
	 * **piu' cara e' proporzionalmente meno esposta** alla vista dello spawn avversario.
	 *
	 * La seconda rotta si cerca escludendo le celle interne della prima (`FindPathAvoiding`): e' il modo di
	 * chiedere «esiste un'alternativa che non ricalca la strada principale».
	 *
	 * L'esposizione si misura in **frazione** (celle viste / celle interne), non in numero assoluto, e la
	 * ragione e' un difetto vero trovato dai test: contando le celle, la rotta piu' lunga ne ha di piu' e
	 * quindi ne ha quasi sempre un numero *diverso* — su un campo completamente aperto e simmetrico il
	 * criterio passava. Il numero assoluto misura la lunghezza; la frazione misura la copertura.
	 *
	 * La direzione conta: U1 chiede «una piu' corta ed esposta, una piu' lunga e **coperta**». Se paghi di
	 * piu' in movimento e resti esposto uguale o di piu', non e' un trade-off — e' una rotta peggiore, che
	 * nessuno sceglierebbe.
	 *
	 * Nessuna seconda rotta -> `bPassed == false` con il motivo, non `bNotEvaluable`: e' un verdetto sulla
	 * mappa, ed e' esattamente il difetto che il criterio cerca.
	 */
	static FRTArenaCriterionResult CheckTwoRoutes(const URTHexMapAsset* Map, const FRTCellId& SpawnA,
		const FRTCellId& SpawnB, float MaxCostRatio = DefaultMaxCostRatio,
		float MinExposureGap = DefaultMinExposureGap);

	/**
	 * **Passo 5 — tutto e' raggiungibile.** Vero se ogni cella percorribile della mappa e' raggiungibile da uno
	 * spawn, attraversando il grafo tattico (quindi passando dalle transizioni esplicite per cambiare layer).
	 *
	 * E' il criterio che mancava: il `done_when` di U1 verificava i passi 3, 4 e 7 e **saltava il 5**, quindi
	 * un'arena poteva passare ogni controllo senza avere una piattaforma — o averla scollegata, che e' peggio,
	 * perche' la si vede e non ci si arriva.
	 *
	 * Una zona irraggiungibile non e' una scelta di design: e' una mappa rotta, e oggi la si scopre a runtime
	 * quando un'unita' non ci arriva. Riporta **quante** celle e le prime che trova, cosi' correggere non e'
	 * una caccia.
	 */
	static FRTArenaCriterionResult CheckReachability(const URTHexMapAsset* Map, const FRTCellId& SpawnA);

	/**
	 * Celle percorribili che il grafo tattico NON raggiunge partendo da `From`, in ordine stabile.
	 *
	 * Estratta da `CheckReachability` perche' ha due consumatori: il criterio, che ne conta quante sono, e
	 * l'overlay dell'editor, che le mostra. Una seconda visita scritta a parte avrebbe finito per rispondere
	 * diversamente — il numero e le celle marcate in scena devono venire dallo stesso calcolo.
	 *
	 * Visita il GRAFO, non i sei vicini planari: una cella sovrapposta e' vicina nello spazio e lontana nel
	 * grafo, e confonderle direbbe «ci si arriva» di una zona dove non ci si arriva.
	 */
	static TArray<FRTCellId> FindUnreachableCells(const URTHexMapAsset* Map, const FRTCellId& From);

	/**
	 * Valuta i tre criteri sugli spawn indicati.
	 * Mappa nulla o spawn assenti dalla mappa -> tutti e tre `bNotEvaluable`.
	 */
	static FRTArenaCriteriaReport Evaluate(const URTHexMapAsset* Map, const FRTCellId& SpawnA,
		const FRTCellId& SpawnB, int32 MoveBudget = DefaultMoveBudget,
		float MaxCostRatio = DefaultMaxCostRatio, float MinExposureGap = DefaultMinExposureGap);

	/**
	 * Come `Evaluate`, ma gli spawn li **deriva dalla mappa** con `URTMatchSetupLibrary::PickStartCells`, cioe'
	 * la stessa funzione che usa l'allestimento della partita.
	 *
	 * E' la forma da preferire: spawn scelti a mano misurerebbero una partita che non si gioca. Celle di
	 * partenza insufficienti -> tutti e tre `bNotEvaluable`.
	 */
	static FRTArenaCriteriaReport EvaluateWithDerivedSpawns(const URTHexMapAsset* Map,
		int32 NumPerTeam = 2, int32 Layer = 0, int32 MoveBudget = DefaultMoveBudget,
		float MaxCostRatio = DefaultMaxCostRatio, float MinExposureGap = DefaultMinExposureGap);

	/** Riga leggibile per il log/console: un simbolo e il dettaglio per ciascun criterio. */
	static FString FormatReport(const FRTArenaCriteriaReport& Report);

private:
	/**
	 * Frazione (0..1) di celle interne del percorso viste da `Observer`. Estremi esclusi: sono gli spawn, e si
	 * vedono sempre da se' — includerli diluirebbe la misura di una quantita' che dipende dalla lunghezza.
	 */
	static float ExposureRatio(const URTHexMapAsset* Map, const TArray<FRTCellId>& Path,
		const FRTCellId& Observer);
};
