#include "Player/Planning/RTPlanningSnapshot.h"
#include "Ability/RTMovementProfileLibrary.h"
#include "Core/RTGameplayTags.h" // Unbalanced: [D-319] abbassa il TETTO, e il tetto si applica qui
#include "Kismet/GameplayStatics.h"
#include "Turn/Actions/RTPlanValidationLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/Resolution/RTHexSim.h"
#include "Unit/RTUnit.h"

bool RTPlanningSnapshot::For(const UObject* WorldContext, const ARTUnit* Unit, FRTHexSnapshot& OutSnapshot,
	int32& OutUnitId, TArray<ARTUnit*>* OutUnits)
{
	OutUnitId = INDEX_NONE;
	ARTTurnManager* TurnManager = Cast<ARTTurnManager>(
		UGameplayStatics::GetActorOfClass(const_cast<UObject*>(WorldContext), ARTTurnManager::StaticClass()));
	if (!TurnManager || !Unit)
	{
		return false;
	}
	// 🔴 **La fotografia della PIANIFICAZIONE si chiede dalla posizione di chi pianifica** ([D-371]).
	//
	// Fino al 2026-09-21 qui si chiedeva quella onnisciente, e l'anteprima ne ereditava l'occupazione:
	// il ventaglio perdeva **esattamente** le celle dei nemici ignoti — 61 celle, 60 con un nascosto,
	// 59 con due — e il costo del percorso passava da 4 a 5 deviando attorno a un corpo che il
	// giocatore non poteva vedere. Non «un'anteprima imprecisa»: una lettura di POSIZIONE.
	//
	// ⚠️ **Il client continua a NON costruirsi uno stato parallelo** (invariante #5): lo snapshot resta
	// chiesto all'autorita', che ora lo compone per l'osservatore invece di consegnarlo intero.
	TArray<ARTUnit*> Units;
	OutSnapshot = TurnManager->MakeCurrentSnapshot(Units, Unit->TeamId);
	// 🔴 **Il Move del giocatore si pianifica sulla mappa che il giocatore conosce** ([D-372], `#2794`): una cella
	// mai osservata e' libera, non un muro che la sua squadra non ha mai visto. Solo qui: la Resolution resta
	// onnisciente e i bot restano sulla vista autorevole.
	TurnManager->AttachPlanningMap(OutSnapshot, Unit->TeamId);
	// L'UnitId e' l'INDICE nell'array delle unita' vive: va ricalcolato a ogni interazione, non memorizzato.
	OutUnitId = Units.IndexOfByKey(const_cast<ARTUnit*>(Unit));

	// 🔑 **Il TETTO di [D-425], e vive QUI perche' qui si pianifica.** Lo snapshot che il turno usa per
	// risolvere porta la **banda** — quanto l'unita' ha chiesto — e non potrebbe portare altro: e' la
	// misura che descrive un piano gia' scritto. Questo snapshot invece serve a **scrivere** quel piano,
	// e la domanda che gli si fa e' l'altra: *fin dove ho il permesso di arrivare*.
	//
	// ⛔ **Senza questa riga la banda `Sprint` sarebbe irraggiungibile.** Chi non ha ancora pianificato
	// niente ha banda `Still`, cioe' `1×`: il primo clic oltre la portata verrebbe rifiutato, e nessuno
	// potrebbe mai superare `1×` — *«una banda che non e' una banda»*, l'alternativa che [D-425] punto
	// (9) scarta. ⚠️ E si alza il budget della SOLA unita' che sta pianificando: gli altri restano come
	// il turno li vede, perche' e' contro la loro occupazione reale che questo percorso va validato.
	if (OutSnapshot.Units.IsValidIndex(OutUnitId))
	{
		const FRTMovementProfile Ceiling = URTMovementProfileLibrary::CeilingProfile(
			Unit->PlannedMovementProfileId,
			URTMovementProfileLibrary::ReservedProfileForPlan(
				URTPlanValidationLibrary::MakePlanFor(Unit)),
			Unit->HasStatus(TAG_Status_Unbalanced));
		const int32 Base = Unit->GetEffectiveMoveRange();

		// 🔴 **Il tetto SOSTITUISCE il budget, non lo alza soltanto.**
		//
		// ⏱️ *Fino al 2026-09-17 questa riga era `FMath::Max(Planner.MoveBudget, ...)`*, scritta per
		// non scavalcare i modificatori. Era sbagliata, e il difetto e' arrivato da PIE: con `Max` il
		// tetto diventa un **pavimento**, quindi dichiarare `Sneak` — che lo DIMEZZA — non toglieva
		// niente all'anteprima. Le celle verdi restavano quelle del movimento pieno mentre il piano si
		// troncava davvero: l'anteprima mentiva sul budget con cui il turno si sarebbe risolto.
		//
		// 🔑 **E il timore che aveva motivato il `Max` non regge, misurato**: `Base` e'
		// `GetEffectiveMoveRange()`, cioe' lo STESSO valore da cui `MakeSimUnit` deriva il budget —
		// `Root` e `Prone` sono gia' dentro. Cio' che il tetto potrebbe scavalcare non esiste: `Slow`
		// non e' una riduzione di budget ma un costo PER CELLA (`MoveCostModifier`), e questa riga non
		// lo tocca.
		FRTHexSimUnit& Planner = OutSnapshot.Units[OutUnitId];
		Planner.MoveBudget = Ceiling.ResolveMoveBudget(Base);
		Planner.StepBudget = Ceiling.ResolveStepBudget(Base);
	}
	if (OutUnits)
	{
		// Gli stessi indici dello snapshot: servono a distinguere i NEMICI (che una carica colpisce) dagli
		// ostacoli (che la fermano). ⚠️ La si legge dagli Actor anche ora che `FRTHexSimUnit::TeamId`
		// esiste (`#2984`): quel campo serve al resolver e sta FUORI da ogni hash, e leggerlo qui
		// legherebbe una decisione di presentazione a un dato che lo snapshot non promette di portare.
		*OutUnits = MoveTemp(Units);
	}
	return OutUnitId != INDEX_NONE;
}
