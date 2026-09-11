#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Perception/RTKnowledgeView.h" // FRTKnowledgeView, FRTKnowledgeEntry
#include "RTEnemyTacticalQuery.generated.h"

class URTHexMapAsset;
class URTHeroData;

/**
 * Le regioni tattiche di un soggetto OSSERVATO: dove arriva camminando, dove arriva soltanto scattando,
 * dove puo' colpire adesso, dove potrebbe colpire dopo una mobilita' rapida (`#2596`, `#2632`).
 *
 * 🔴 **E' un DTO di privacy, non un risultato di simulazione.** Non decide nulla di competitivo: non
 * muove, non colpisce, non produce eventi. Esiste perche' il giocatore possa interrogare un nemico che
 * vede senza che la risposta sia costruita dal dato pieno — cio' che `#1805` riassume in *«la privacy non
 * e' non disegnare, e' non costruire la vista»*.
 *
 * ⚠️ **Ogni campo e' Public per costruzione**, ed e' l'unica classe ammessa qui: la struttura E' la
 * risposta autorizzata. Un campo che non lo fosse non andrebbe nascosto — non andrebbe calcolato.
 * `RefactorTactics.Perception.EnemyTacticalRegionsFieldsAreClassified` lo verifica sulla reflection,
 * con la disciplina che `#2331` ha introdotto per `FRTIntentView`: il gate misura i CAMPI, non i valori,
 * perche' un quinto campo aggiunto domani passerebbe ogni assert scritto sui quattro di oggi.
 */
USTRUCT(BlueprintType)
struct FRTEnemyTacticalRegions
{
	GENERATED_BODY()

	/** Il soggetto a cui le regioni si riferiscono: `ARTUnit::StableUnitId`, la chiave che attraversa i turni. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	int32 StableUnitId = INDEX_NONE;

	/**
	 * Dove il soggetto puo' arrivare col movimento normale, ordinate `StableLess`. Include la cella di
	 * partenza, come `URTHexSimLibrary::ReachableCells` da cui provengono.
	 *
	 * ⚠️ E' PORTATA, non minaccia: il ruleset manda `NormalMovement` in `ERTMatchPhase::Move`, che risolve
	 * DOPO il Blast (`URTCatalogLibrary::MapResolutionPhase`). Arrivarci non abilita nessun attacco in
	 * questo turno.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	TArray<FRTCellId> ReachableCells;

	/**
	 * Celle che il soggetto raggiunge SOLO con una mobilita' rapida, ordinate `StableLess` e **al netto** di
	 * `ReachableCells` — la stessa disciplina con cui `PostDashThreat` esce al netto di `ImmediateThreat`.
	 *
	 * 🔑 **Non e' un cerchio grande attorno a uno piccolo, e la differenza e' tattica** (`#2632`). Il passo
	 * (`NormalMovement`) risolve in `ERTMatchPhase::Move`, cioe' DOPO il Blast; lo scatto (`FastMovement`)
	 * risolve in `Dash`, cioe' PRIMA. E `Action.Sprint` si paga con `Status.Exposed`. Fondere le due aree in
	 * una sola direbbe al giocatore una capacita' che il ruleset non sostiene.
	 *
	 * ⚠️ **Vuota non vuol dire assente.** Un soggetto privo di mobilita' rapida ha questa regione vuota e
	 * `RegionsFor` risponde comunque `true`; un soggetto non osservato non produce regioni del tutto, e
	 * `RegionsFor` risponde `false`. Sono due esiti diversi e un test li distingue.
	 *
	 * ⚠️ **Include ogni origine di mobilita' rapida, non solo quelle a budget.** Lo Scope di `#2632` nomina
	 * `ReachableWithBudget`, che copre `Action.Sprint`; le mobilita' rapide LINEARI (`Action.Leap`,
	 * `Action.Charge`, `Action.Dodge`, `Action.Reposition`) passano da `ResolveLinearMove`, che e' la
	 * primitiva canonica della linearita' e non un secondo pathfinder. Escluderle renderebbe il DTO
	 * auto-contraddittorio: `PostDashThreat` irradia dalle loro celle d'arrivo, che sarebbero minaccia
	 * post-scatto partita da celle fuori dalla regione dello scatto. Scostamento dichiarato, non dedotto.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	TArray<FRTCellId> DashOnlyCells;

	/**
	 * Celle che il soggetto puo' investire SENZA riposizionarsi, ordinate `StableLess`: l'unione delle
	 * impronte delle sue azioni di slot principale valutate dalla cella corrente, piu' l'impronta della
	 * carica (vedi la nota su `Action.Charge` in `RegionsFor`).
	 *
	 * Sono le celle INVESTITE, mai i centri bersagliabili: per una forma ad area i due insiemi
	 * differiscono, e mostrare i centri direbbe al giocatore una portata che non subira'.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	TArray<FRTCellId> ImmediateThreat;

	/**
	 * Celle che il soggetto potrebbe investire DOPO una mobilita' rapida, ordinate `StableLess`.
	 *
	 * 🔑 **`PostDash` e non `PostMove`, e il nome e' la regola.** Misurato su
	 * `URTCatalogLibrary::MapResolutionPhase`: `FastMovement` risolve in `Dash`, cioe' PRIMA del Blast,
	 * mentre `NormalMovement` risolve in `Move`, cioe' DOPO. Una «minaccia post-movimento» descriverebbe
	 * qualcosa che il ruleset non consente. Solo le origini di mobilita' rapida contano.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	TArray<FRTCellId> PostDashThreat;

	FRTEnemyTacticalRegions() = default;
};

/**
 * La porta fra la conoscenza autorizzata e l'anteprima tattica di un nemico osservato (`#2596`).
 *
 * Pura e headless: nessun Actor, nessun `UWorld`, nessuna allocazione di `UObject`. Consuma le primitive
 * canoniche — `ReachableCells`, `HexHitCells`, `HasLineOfSight`, `ResolveLinearMove` — e non ne riscrive
 * nessuna regola. E' la stessa disciplina di `URTDebugReportLibrary::DescribeIntents`, che passa da
 * `URTIntentPrivacyLibrary::FilterForTeam` invece di reimplementarlo.
 */
UCLASS()
class REFACTORTACTICS_API URTEnemyTacticalQueryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Le regioni del soggetto indicato, come l'osservatore di `View` ha diritto di vederle.
	 *
	 * Ritorna `false` — e lascia `OutRegions` al default — quando il soggetto non ha una voce nella vista.
	 *
	 * ## 🔴 Perche' la firma NON prende `FRTHexSnapshot`
	 *
	 * `FRTHexSnapshot` porta `Units` con la cella AUTOREVOLE di ogni unita' viva o morta, l'`Occupancy`
	 * completa, e `TeamKnowledge` — che e' `TArray<FRTTeamKnowledge>`, cioe' la conoscenza di TUTTE le
	 * squadre. Passarlo qui non sarebbe «dare troppo»: sarebbe consegnare all'osservatore anche la vista
	 * del suo avversario, e lasciare che sia questa funzione a decidere di non guardarla.
	 *
	 * La firma e' il meccanismo, non una formalita'. Il precedente vive gia' nel progetto:
	 * `URTIntentPrivacyLibrary::ClassifyPlan` prende UN intento e non la lista *«cosi' non PUO' guardare i
	 * piani avversari neanche per sbaglio»*.
	 *
	 * `Map` non e' lo snapshot ed e' ammesso: e' il terreno, che entrambi i giocatori possiedono. E' lo
	 * stesso ingresso che `HasLineOfSight` e `ResolveLinearMove` prendono gia'.
	 *
	 * ## 🔴 Perche' `Hero` e' un parametro, e perche' non e' una falla
	 *
	 * Il profilo tattico serve, e non puo' nascere qui: `URTHeroCatalogLibrary::MakeBranth` e sorelle fanno
	 * `NewObject`, e una query pura chiamata a ogni selezione non alloca `UObject`. Arriva percio' dal
	 * chiamante, con la stessa forma di `ARTUnit::ConfigureFromHeroData`.
	 *
	 * ⛔ **Ma non e' un canale per il dato autorevole, e due proprieta' lo impediscono.** `URTHeroData` e'
	 * la definizione PUBBLICA dell'eroe — identica per chiunque, priva di qualunque stato corrente: nessun
	 * HP, nessuno status, nessun budget speso. E il suo `HeroId` deve coincidere con quello della voce
	 * autorizzata, altrimenti la funzione rifiuta: un chiamante non puo' sostituire il profilo di un eroe
	 * con quello di un altro per allargare la regione.
	 *
	 * ∴ il budget usato e' `Hero->MovePoints`, la BASELINE di catalogo, mai `FRTHexSimUnit::MoveBudget`.
	 * Un `Action.Slow` che l'osservatore non vede non deve restringere la regione: se lo facesse, la
	 * regione stessa diventerebbe la spia dello status.
	 *
	 * ## 🔴 L'occupazione e' quella autorizzata, e un'unita' non vista NON blocca
	 *
	 * Gli ostacoli mobili sono le sole voci di `View`. Non e' un'approssimazione: se un'unita' che
	 * l'osservatore non vede bloccasse una cella, il buco nella regione disegnata sarebbe una deduzione
	 * affidabile sulla sua posizione — l'esposizione che l'invariante `#6` vieta anche senza mostrare un
	 * campo. `RefactorTactics.Perception.EnemyQueryHidesUnobservedState` esiste per prendere questa
	 * regressione.
	 *
	 * ## ⚠️ Un solo non-esito, e senza reason code
	 *
	 * Soggetto ignoto e soggetto inesistente danno lo STESSO `false`. Un reason code che li distinguesse
	 * direbbe all'osservatore che quell'unita' esiste, che e' esattamente cio' che la voce assente gli
	 * nasconde. L'indistinguibilita' e' il contratto, non un'omissione.
	 *
	 * ## Nota su `Action.Charge`
	 *
	 * La carica e' mobilita' rapida (`IsFastMovement`) **e** `bCountsAsAttack`, e occupa il solo slot
	 * Movimento ([D-191]). Non essendo un'azione di slot principale non entrerebbe in nessuna delle due
	 * unioni, e sparirebbe dall'anteprima senza che nulla se ne accorga. Classificata percio' due volte, e
	 * le due cose sono simultanee:
	 *
	 * - la sua impronta di IMPATTO entra in `ImmediateThreat` — parte dalla cella corrente, non segue
	 *   nessuno scatto, e risolve in `Dash`, cioe' prima del Blast;
	 * - le sue celle d'ARRIVO sono origini di `PostDashThreat` — lo slot principale resta libero dopo di
	 *   essa, quindi da li' puo' partire un attacco vero.
	 */
	static bool RegionsFor(const URTHexMapAsset* Map, const FRTKnowledgeView& View,
		int32 SubjectStableUnitId, const URTHeroData* Hero, FRTEnemyTacticalRegions& OutRegions);
};
