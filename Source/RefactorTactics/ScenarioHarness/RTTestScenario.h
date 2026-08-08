#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "RTTestScenario.generated.h"

/**
 * Modello dati di uno SCENARIO di test automatico (RT Scenario Harness).
 *
 * Uno scenario descrive: chi c'e' in campo, cosa ciascuno fa turno per turno, e cosa deve risultare vero
 * alla fine. Il runner lo esegue attraverso il **percorso di gioco reale** (piani sulle unita' ->
 * `LockInAndResolve` -> resolver -> TurnLog), mai con scorciatoie tipo `SetActorLocation`: un test che non
 * passa dal codice vero non prova niente sul codice vero.
 *
 * Formato: JSON versionabile in `Scenarios/<Categoria>/<Nome>.json`. Non sono `.uasset` perche' devono
 * essere leggibili e diffabili in una PR.
 */

/** Esito di una esecuzione. `Error` NON e' un `Fail`: distinguere i due e' cio' che impedisce a uno scenario rotto di travestirsi da regressione di gioco. */
UENUM()
enum class ERTTestOutcome : uint8
{
	/** Simulazione completata, tutte le assertion soddisfatte. */
	Pass,
	/** Simulazione completata, almeno una assertion non soddisfatta. E' un difetto del GIOCO. */
	Fail,
	/** Impossibile eseguire: scenario invalido, eroe sconosciuto, mappa mancante. E' un difetto del TEST. */
	Error
};

/** Tipo di assertion. Prima iterazione: solo cio' che serve a `Movement.Basic` (§ASSERTION SYSTEM: «non creare un mega-framework prematuramente»). */
UENUM()
enum class ERTAssertionKind : uint8
{
	/** L'unita' indicata si trova sulla cella attesa a fine scenario. */
	UnitAtCell,
	/** Sono stati completati almeno N turni senza che la partita si interrompesse. */
	TurnsCompleted,
	/** La salute dell'unita' e' esattamente il valore atteso: e' cosi' che si verifica un danno. */
	UnitHpEquals,
	/** L'unita' e' viva (`value` != 0) oppure abbattuta (`value` == 0). */
	UnitAlive
};

/**
 * Modifica di una cella dell'arena generata: ostacoli, muri, terreno costoso.
 *
 * Serve a scrivere scenari come `Movement.Blocked` senza versionare un `.umap`: l'arena resta generata da
 * codice e lo scenario ne cambia le poche celle che gli interessano. Le celle non elencate restano pavimento
 * a costo 1.
 */
USTRUCT()
struct FRTScenarioCell
{
	GENERATED_BODY()

	UPROPERTY()
	FRTCellId Cell;

	/** Ostacolo: non ci si passa. Il pathfinding la aggira o rifiuta il percorso. */
	UPROPERTY()
	bool bBlocksMovement = false;

	/** Muro: non ci si vede attraverso. Non impedisce il passaggio. */
	UPROPERTY()
	bool bBlocksLineOfSight = false;

	/** Costo di traversata. 0 = non specificato, resta il default della cella (1). */
	UPROPERTY()
	int32 MoveCost = 0;
};

/** Un'unita' schierata dallo scenario. */
USTRUCT()
struct FRTScenarioUnit
{
	GENERATED_BODY()

	/** ID locale allo scenario (`A1`), usato dagli intent e dalle assertion. Non e' l'ID di gioco. */
	UPROPERTY()
	FString Id;

	/** ID stabile dell'eroe dal catalogo: `Hero.Flux`, `Hero.Riva`, `Hero.Bastion`, `Hero.Vektor`. */
	UPROPERTY()
	FName HeroId;

	UPROPERTY()
	int32 TeamId = 0;

	UPROPERTY()
	FRTCellId Cell;
};

/**
 * L'intento di una unita' in un turno: movimento, abilita', o entrambi.
 *
 * Movimento e abilita' convivono perche' convivono nel gioco — un'unita' ha uno slot movimento e uno slot
 * principale, e usarli insieme e' la norma, non un caso limite.
 */
USTRUCT()
struct FRTScenarioIntent
{
	GENERATED_BODY()

	UPROPERTY()
	FString UnitId;

	/** Waypoint del movimento, come li produrrebbe il giocatore cliccando. Vuoto = l'unita' non si sposta. */
	UPROPERTY()
	TArray<FRTCellId> Move;

	/**
	 * `ActionId` dell'abilita' da usare (`Flux.ArcPulse`). Vuoto = nessun attacco.
	 *
	 * Per **ID** e non per indice: l'indice di un'abilita' nel kit si sposta appena qualcuno ne aggiunge una,
	 * e lo scenario continuerebbe a passare verificando l'abilita' sbagliata — il tipo di test che mente.
	 */
	UPROPERTY()
	FName Ability;

	/** ID di scenario del bersaglio. Obbligatorio quando `Ability` e' valorizzata. */
	UPROPERTY()
	FString Target;
};

/** Un turno dello scenario. */
USTRUCT()
struct FRTScenarioTurn
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FRTScenarioIntent> Intents;
};

/** Una condizione da verificare a fine scenario. */
USTRUCT()
struct FRTTestExpectation
{
	GENERATED_BODY()

	UPROPERTY()
	ERTAssertionKind Kind = ERTAssertionKind::UnitAtCell;

	/** Unita' coinvolta (`UnitAtCell`). */
	UPROPERTY()
	FString UnitId;

	/** Cella attesa (`UnitAtCell`). */
	UPROPERTY()
	FRTCellId Cell;

	/** Valore intero atteso (`TurnsCompleted`, `UnitHpEquals`, `UnitAlive`). */
	UPROPERTY()
	int32 Value = 0;
};

/** Scenario completo, come letto dal file. */
USTRUCT()
struct FRTTestScenario
{
	GENERATED_BODY()

	/** ID stabile e gerarchico: `Movement.Basic`. Da' il nome alla cartella di output. */
	UPROPERTY()
	FString ScenarioId;

	/** Versione del FORMATO dello scenario, non del contenuto. Un loader piu' nuovo deve poter rifiutare un formato che non conosce. */
	UPROPERTY()
	int32 Version = 1;

	/**
	 * Seed dichiarato ma **non consumato**: oggi il progetto non ha alcun RNG e il determinismo viene da
	 * coordinate intere e ordinamenti totali (`HexSim.ReplayDivergenceZero`). Il campo esiste perche' il
	 * giorno in cui un RNG entrera' nel resolver lo scenario debba gia' saperlo dichiarare — non perche'
	 * faccia qualcosa adesso. Impostarlo a un valore diverso da 0 e' lecito e viene registrato nel report.
	 */
	UPROPERTY()
	int32 Seed = 0;

	/** Raggio dell'arena esagonale generata per lo scenario. Mappa da codice: nessun `.umap` da versionare. */
	UPROPERTY()
	int32 MapRadius = 3;

	/** Celle da modificare nell'arena generata (ostacoli, muri, terreno costoso). Vuoto = arena liscia. */
	UPROPERTY()
	TArray<FRTScenarioCell> Cells;

	UPROPERTY()
	TArray<FRTScenarioUnit> Units;

	UPROPERTY()
	TArray<FRTScenarioTurn> Turns;

	UPROPERTY()
	TArray<FRTTestExpectation> Expect;

	/** Trova un'unita' per ID di scenario. Nullptr se assente. */
	const FRTScenarioUnit* FindUnit(const FString& InId) const
	{
		return Units.FindByPredicate([&InId](const FRTScenarioUnit& U) { return U.Id == InId; });
	}
};
