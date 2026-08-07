#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Turn/RTHexSim.h"
#include "RTHexBotLibrary.generated.h"

class URTHexMapAsset;

/** Mossa candidata del bot su griglia esagonale: dove finisce e, se serve, chi colpisce da lì. */
USTRUCT()
struct FRTHexBotPlan
{
	GENERATED_BODY()

	/** Cella in cui il bot finisce (o resta). */
	UPROPERTY() FRTCellId DestCell;

	/** Vero se il piano include un attacco sferrato da DestCell. */
	UPROPERTY() bool bHasAttack = false;

	/** Indice del bersaglio in FRTHexBotContext::Enemies (INDEX_NONE = nessuno). Puro: nessun Actor. */
	UPROPERTY() int32 TargetIndex = INDEX_NONE;

	/** Danno dell'attacco pianificato. */
	UPROPERTY() int32 AttackDamage = 0;

	/** HP+scudo del bersaglio (per riconoscere il colpo letale). */
	UPROPERTY() int32 TargetHealth = 0;
};

/**
 * Contesto puro per valutare una candidata su hex. Stessi pesi e stessa politica del bot quadrato
 * (FRTBotContext): cambiano solo la distanza (esagonale) e la copertura, che qui viene dai dati della mappa
 * invece che da una lista di ostacoli passata dal chiamante.
 */
USTRUCT()
struct FRTHexBotContext
{
	GENERATED_BODY()

	/** Posizione attuale del bot: tie-break di ChooseBestPlan (a parita' di punteggio, mossa minima da qui). */
	UPROPERTY() FRTCellId Origin;

	/** Posizioni dei nemici vivi. */
	UPROPERTY() TArray<FRTCellId> Enemies;

	/** Gittata di ciascun nemico (parallelo a Enemies): serve a stimare la minaccia sulla cella. */
	UPROPERTY() TArray<int32> EnemyRanges;

	/** HP+scudo di ciascun nemico (parallelo a Enemies): serve a riconoscere il colpo letale. */
	UPROPERTY() TArray<int32> EnemyHealth;

	/** Gittata dell'attacco del bot. */
	UPROPERTY() int32 AttackRange = 0;

	/** Danno dell'attacco del bot. */
	UPROPERTY() int32 AttackDamage = 0;

	/** >0 = kiter (mantiene la distanza di sicurezza); 0 = mischia (chiude la distanza). */
	UPROPERTY() int32 KiteStandoff = 0;

	// Pesi interi (bilanciabili senza toccare la logica; invariante #4: niente float). Default: il kill domina.
	UPROPERTY() int32 WKill = 10000;
	UPROPERTY() int32 WDamage = 10;
	UPROPERTY() int32 WThreat = 100;
	UPROPERTY() int32 WKiteViolation = 50;
	UPROPERTY() int32 WApproach = 10;

	/**
	 * Premio per una cella da cui il bot POTRA' colpire (distanza dal nemico piu' vicino entro `AttackRange`),
	 * quando la candidata non colpisce gia' in questo turno. E' il valore del posizionarsi per il turno
	 * SUCCESSIVO: senza, l'utility vede solo il turno corrente, e avvicinarsi ha solo costi — la minaccia
	 * cresce piu' in fretta di quanto `WApproach` premi la vicinanza, quindi una mischia si ferma appena
	 * FUORI dalla propria portata e la partita si blocca (stallo osservato in #145 facendo valere lo slot).
	 *
	 * Tarato sopra `WThreat` per due nemici (200): entrare a portata deve valere piu' del tiro che si incassa,
	 * altrimenti il termine non cambia nessuna decisione.
	 */
	UPROPERTY() int32 WInRange = 250;

	/**
	 * Portata d'attacco MIGLIORE del bot, usata solo per il posizionamento. Distinta da `AttackRange`, che il
	 * chiamante sovrascrive per ogni candidata (0 = «da questa cella non si colpisce»): senza un campo suo,
	 * il premio di posizionamento non si attiverebbe mai proprio sulle candidate di movimento, che sono le
	 * uniche a cui serve. 0 = nessun attacco -> nessun premio.
	 */
	UPROPERTY() int32 PositioningRange = 0;
	/** Bonus per la quota (Layer) della cella: premia l'alta quota. */
	UPROPERTY() int32 WElevation = 20;
};

/**
 * Decisioni del bot su griglia ESAGONALE: logica pura, nessun Actor, solo interi (invariante #4).
 * Politica ereditata dal bot quadrato che ha sostituito (rimosso al CP 7.2) — focus-fire, minaccia mitigata
 * dalla copertura, kiting o avvicinamento, bonus di elevazione — con distanza esagonale e linea di vista
 * letta dall'asset mappa.
 *
 * Le mosse candidate arrivano da URTHexSimLibrary::ReachableCells, che ha gia' applicato budget di movimento,
 * celle bloccate, unita' occupanti e archi verticali: il bot non rifa' pathfinding e non puo' proporre mosse
 * illegali. Vedi docs/design/h6-5-hex-bot-spec.md.
 */
UCLASS()
class REFACTORTACTICS_API URTHexBotLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Utility score (intero) di una candidata: focus-fire (danno + bonus se uccide), meno la minaccia subita
	 * nella cella di destinazione (solo dai nemici che hanno gittata E linea di vista: la copertura protegge),
	 * meno la penalita' di posizionamento (kiter sotto standoff / mischia lontana), piu' il bonus di quota.
	 */
	static int32 ScorePlan(const URTHexMapAsset* Map, const FRTHexBotPlan& Plan, const FRTHexBotContext& Context);

	/**
	 * Candidata a punteggio massimo. TIE-BREAK ASSOLUTO: a parita' di punteggio vince la MOSSA MINIMA da
	 * Context.Origin (restare vince), poi l'ordine stabile della cella (StableLess) -> permutare le candidate
	 * non cambia l'esito. Nessuna candidata -> resta a Origin.
	 */
	static FRTHexBotPlan ChooseBestPlan(const URTHexMapAsset* Map, const TArray<FRTHexBotPlan>& Candidates,
		const FRTHexBotContext& Context);

	/**
	 * Mosse candidate dell'unita': per ogni cella raggiungibile entro il budget, una candidata senza attacco e
	 * una per ciascun nemico entro gittata e in linea di vista DA QUELLA CELLA. Ordine deterministico.
	 */
	static TArray<FRTHexBotPlan> BuildCandidates(const FRTHexSnapshot& Snapshot, int32 UnitId,
		const FRTHexBotContext& Context);

	/** Piano scelto per l'unita': ChooseBestPlan sulle candidate generate. */
	static FRTHexBotPlan PlanUnit(const FRTHexSnapshot& Snapshot, int32 UnitId, const FRTHexBotContext& Context);

	/**
	 * Cella di FUGA: fra quelle raggiungibili entro il budget, quella piu' lontana (distanza esagonale) dalla
	 * minaccia. A parita' di distanza vince il percorso piu' economico, poi l'ordine stabile della cella —
	 * quindi l'esito non dipende dall'ordine di enumerazione. Nessuna via d'uscita (budget 0, celle bloccate
	 * od occupate) -> resta dov'e'.
	 *
	 * Serve alla guardia di "panico" del kiter, che rinuncia al tiro per non farsi raggiungere: e' una scelta
	 * di posizionamento pura, non un'utility, e resta separata da ScorePlan come nel bot quadrato.
	 */
	static FRTCellId BestKiteCell(const FRTHexSnapshot& Snapshot, int32 UnitId, const FRTCellId& Threat);
};
