#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionData.h"
#include "Map/RTCellId.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTReactionOpportunityTypes.h" // FRTReactionOpportunity: l'UNICO ingresso della decisione del bot
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

	/**
	 * Forma dell'attacco pianificato, copiata dal contesto da BuildCandidates come gia' avviene per
	 * AttackDamage. Sta QUI e non nel contesto perche' `ChooseBestPlan` confronta in una sola lista le
	 * candidate di abilita' DIVERSE: un solo contesto descriverebbe la forma di una sola di esse.
	 */
	UPROPERTY() ERTAbilityShape Shape = ERTAbilityShape::Single;

	/** Raggio dell'area, letto solo quando Shape e' Area. */
	UPROPERTY() int32 AreaRadius = 0;

	/** Gittata dichiarata dall'azione: serve alla profondita' del Cone. */
	UPROPERTY() int32 RangeCells = 0;

	/** Vero se l'attacco colpisce anche gli alleati (`FRTActionDef::bFriendlyFire`). */
	UPROPERTY() bool bFriendlyFire = false;

	/**
	 * Cella da cui il bot ARRIVA su `DestCell` — il predecessore che `ReachableCells` ora conserva.
	 *
	 * Serve al facing (CP 13.5, ADR-0005): l'orientamento deriva dall'ultimo passo, e senza questo campo
	 * valutare l'esposizione di una candidata richiederebbe un pathfinding per candidata dentro il ciclo di
	 * scoring. Per chi resta fermo vale `DestCell`, e il facing non cambia.
	 */
	UPROPERTY() FRTCellId FromCell;
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

	/**
	 * Orientamento di ciascun nemico (parallelo a Enemies), come e' ALL'INIZIO del turno.
	 *
	 * ⚠️ **Non e' una fuga d'informazione**: il facing corrente e' osservabile — `ARTUnit::Facing` e' cio' che
	 * le regole leggono e che la mesh mostra a fine playback, quindi il giocatore umano vede lo stesso dato.
	 * A restare privato e' l'INTENTO di rotazione, che [ADR-0005] filtra per squadra
	 * (`Facing.IntentIsTeamFiltered`) e che questo campo non contiene.
	 *
	 * ⚠️ **E' una previsione destinata a sbagliare, a volte**: chi attacca ruota verso il proprio bersaglio
	 * PRIMA che si valuti l'arco (`RTTurnManager.cpp`, `TargetingReoriented` precede il controllo direzionale).
	 * Un nemico che attacca il bot gli si gira contro, e il fianco che il bot aveva visto non c'e' piu'. Il
	 * bonus sopravvive quando il bersaglio e' impegnato con qualcun ALTRO — il fuoco incrociato, non il duello.
	 * La sovrastima che ne deriva si misura sul TurnLog (`RearHitBypassedCover`), non si stima a priori.
	 */
	UPROPERTY() TArray<ERTHexDirection> EnemyFacings;

	/** Orientamento attuale del bot: da qui parte la stima di come sara' orientato a fine turno. */
	UPROPERTY() ERTHexDirection SelfFacing = ERTHexDirection::E;

	/**
	 * Posizioni degli alleati vivi, ESCLUSO il bot che sta pianificando: `CollectHexAttacks` salta sempre
	 * l'attaccante (`u == Intent.AttackerId`), quindi contarsi qui renderebbe il bot timido su un danno che
	 * non subirebbe mai.
	 */
	UPROPERTY() TArray<FRTCellId> Allies;

	/** HP+scudo di ciascun alleato (parallelo a Allies): serve a riconoscere il collaterale letale. */
	UPROPERTY() TArray<int32> AllyHealth;

	/** Gittata dell'attacco del bot. */
	UPROPERTY() int32 AttackRange = 0;

	/** Danno dell'attacco del bot. */
	UPROPERTY() int32 AttackDamage = 0;

	/**
	 * Forma dell'attacco valutato: decide QUANTE celle prende, quindi quanti nemici in piu' e quanti alleati.
	 * Con `Single` il calcolo resta quello di prima (una cella, un bersaglio).
	 */
	UPROPERTY() ERTAbilityShape AttackShape = ERTAbilityShape::Single;

	/** Raggio dell'area, letto solo quando AttackShape e' Area. */
	UPROPERTY() int32 AttackAreaRadius = 0;

	/**
	 * Vero se l'attacco valutato colpisce anche gli alleati (`FRTActionDef::bFriendlyFire`). Il bot deve
	 * modellare il resolver, non un'idea del resolver: senza fuoco amico l'alleato nell'area non subisce nulla.
	 */
	UPROPERTY() bool bAttackFriendlyFire = false;

	/** >0 = kiter (mantiene la distanza di sicurezza); 0 = mischia (chiude la distanza). */
	UPROPERTY() int32 KiteStandoff = 0;

	// Pesi interi (bilanciabili senza toccare la logica; invariante #4: niente float). Default: il kill domina.
	UPROPERTY() int32 WKill = 10000;
	UPROPERTY() int32 WDamage = 10;
	/**
	 * Peso del danno inflitto a un ALLEATO dal collaterale di un'area. Pari a WDamage per default: un punto
	 * di danno al compagno annulla esattamente un punto di danno al nemico, quindi prendere due nemici e un
	 * alleato resta conveniente e prenderne uno solo non lo e'. E' un peso, non un veto: si tara senza
	 * toccare la logica.
	 */
	UPROPERTY() int32 WAllyDamage = 10;
	UPROPERTY() int32 WThreat = 100;
	UPROPERTY() int32 WKiteViolation = 50;
	UPROPERTY() int32 WApproach = 10;
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
 * illegali. Vedi docs/technical/h6-5-hex-bot-spec.md.
 */
UCLASS()
class REFACTORTACTICS_API URTHexBotLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Distanza di sicurezza del kiting, DERIVATA dalla portata dell'attacco base. `0` = mischia.
	 *
	 * Il kiting e' un comportamento del BOT, non una caratteristica dell'eroe: un'unita' che muovi tu non lo
	 * consulta mai — dove andare lo decidi tu. Per questo il numero non sta piu' su `ARTUnit` (dove i due
	 * archetipi legacy lo scrivevano insieme alle statistiche) ne' su `URTHeroData`: un campo sull'eroe
	 * direbbe «Phase tiene le distanze» anche quando Phase la guidi tu, dove non significa niente.
	 *
	 * La regola riproduce i due archetipi che il comportamento lo producevano: `Ranger` aveva portata 6 e
	 * standoff 4, `Guardian` portata 3 e standoff 0. Chi colpisce da lontano ha qualcosa da guadagnare a
	 * restare lontano; chi colpisce da vicino no, e arretrare gli costerebbe soltanto il turno.
	 *
	 * Sul roster v0.1 l'unica kiter e' Phase (`PressureJet`, portata 5 -> standoff 3). Gadget e Wraith (4) e
	 * Riktor (3) chiudono la distanza. Se la soglia va spostata, e' questa riga: il resto del bot legge
	 * `FRTHexBotContext::KiteStandoff` e non sa da dove venga.
	 */
	static int32 DeriveKiteStandoff(int32 AttackRangeCells)
	{
		constexpr int32 KiterMinRange = 5;
		return AttackRangeCells >= KiterMinRange ? AttackRangeCells - 2 : 0;
	}

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
	 * Segna in `Snapshot` le celle che l'unita' PRENOTA per raggiungere `DestCell` — la rotta intera, non la
	 * sola destinazione — cosi' che chi pianifica dopo non le veda libere.
	 *
	 * ⚠️ Prenota con l'id dell'unita' stessa, e non e' un dettaglio: `ReachableCells` tratta come occupata
	 * solo una cella il cui occupante e' un ALTRO (`*Occupant != UnitId`), quindi l'unita' non blocca la
	 * propria rotta mentre la blocca a tutte le altre.
	 *
	 * ⚠️ **E' piu' restrittivo del necessario**, dichiarato: nella risoluzione a micro-step due unita'
	 * potrebbero attraversare la stessa cella in momenti diversi senza contendersela. Qui la seconda dovra'
	 * trovare una rotta disgiunta, e in un corridoio stretto potrebbe non trovarne. E' un prezzo scelto: lo
	 * stallo di #1088 costava ZERO mosse per dodici turni.
	 */
	static void ReservePlannedRoute(FRTHexSnapshot& Snapshot, int32 UnitId, const FRTCellId& DestCell);

	/**
	 * Pianifica un'INTERA squadra: ogni unita' decide sapendo cosa hanno gia' prenotato le precedenti.
	 * `Contexts` e' parallelo a `UnitIds`. Restituisce un piano per unita', nello stesso ordine.
	 *
	 * E' la correzione di **#1088**. `PlanUnit` da sola e' corretta e resta: il difetto nasceva dal
	 * chiamarla in un ciclo passando a tutte lo stesso snapshot congelato, cosi' che due compagne
	 * scegliessero la stessa cella e la risoluzione simultanea le fermasse entrambe — per dodici turni,
	 * perche' la pianificazione e' deterministica e ricreava la stessa contesa.
	 *
	 * ⚠️ **La pianificazione diventa dipendente dall'ORDINE di `UnitIds`**: chi decide prima ha piu' scelta.
	 * Non intacca il determinismo — l'ordine e' quello dello snapshot, stabile — ma e' una regola nuova, e
	 * chi chiama deve passare un ordine stabile, non l'ordine di enumerazione degli Actor.
	 *
	 * ⛔ **UNA CHIAMATA PER SQUADRA, e non e' un'ottimizzazione: e' fairness.** Le prenotazioni sono
	 * informazione sui piani, e i piani di una squadra sono privati (CP 13.5, `RT-FEAT-BOT-FAIRNESS`:
	 * *«il bot non vede piu' di te»*). Passando qui unita' di squadre diverse, un bot eviterebbe la cella
	 * dove sta per andare un AVVERSARIO — cioe' schiverebbe un intento che nessun giocatore puo' vedere.
	 * Due squadre che si contendono la stessa cella devono continuare a contendersela: quella e' una
	 * collisione legittima, che il resolver risolve, e non il difetto di #1088.
	 */
	static TArray<FRTHexBotPlan> PlanTeam(const FRTHexSnapshot& Snapshot, const TArray<int32>& UnitIds,
		const TArray<FRTHexBotContext>& Contexts);

	/**
	 * La risposta del bot a una finestra di reazione (CP 14.5). Restituisce una delle `AllowedResponses`.
	 *
	 * ⚠️ **La firma E' il requisito.** Il DoD chiede che il bot decida «con la sola opportunity sanitizzata e
	 * restituisca subito: nessuna attesa reale, nessun accesso a percorsi futuri, trigger futuri o esiti
	 * precalcolati». Questa funzione non ha modo di violarlo: non riceve la mappa, non riceve lo snapshot, non
	 * riceve i percorsi, e `FRTReactionOpportunity` ha un elenco CHIUSO di campi verificato per riflessione da
	 * `Overwatch.OpportunityLeaksNoFuture`. E' una garanzia strutturale, non una promessa da testare — un test
	 * che asserisse «non ha guardato il futuro» potrebbe solo verificare l'esito, mentre qui il futuro non e'
	 * raggiungibile nemmeno volendo.
	 *
	 * Pura anche nel senso stretto: nessun `UWorld`, nessun Actor, nessun timer. «Restituisce subito» non e'
	 * una qualita' dell'implementazione, e' l'unica cosa che puo' fare.
	 *
	 * **Politica della v0.1**: spara al primo bersaglio legale, che e' quello di `UnitId` minore perche'
	 * `AllowedResponses` e' ordinato cosi'. Deterministica e deliberatamente semplice: tenere il colpo in
	 * attesa di un bersaglio migliore e' il *bait*, e vale quanto la stima di cosa arrivera' dopo — che e'
	 * esattamente cio' che il bot non puo' sapere qui, e che appartiene al Tactical Bot v1 (E26) e alla
	 * taratura di CP 14.6. Una politica piu' ambiziosa oggi sarebbe indistinguibile da un'euristica inventata.
	 */
	static FString DecideReactionResponse(const FRTReactionOpportunity& Opportunity);

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
