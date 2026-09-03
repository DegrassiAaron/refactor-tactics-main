#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "RTBuildGrayboxFixturesCommandlet.generated.h"

/**
 * Genera `BP_Graybox_UnitFacingFixture` e, se richiesto, lo posa nella Station 01 di
 * `L_GrayKitPlayground` (#1992, Epic #1990, `D-304`).
 *
 * 🔑 **Perche' un commandlet e non quattro clic**, con le parole che `RTSetObjectiveCell` usa gia': *«un
 * `.uasset` non e' diffabile, quindi il diff di una PR non puo' mostrare cosa e' cambiato dentro. Qui il
 * cambiamento e' un comando scritto — si rilegge, si ripete, e se qualcuno lo disfa per sbaglio si
 * riapplica identico.»* Il fixture della Station 01 e' esattamente quel caso: componenti, cinque
 * parametri e una posa, che a mano sono cinque occasioni di sbagliare in silenzio.
 *
 * ⚠️ **Il Blueprint non contiene geometria.** E' una sottoclasse senza grafo di
 * `ARTGrayboxUnitFacingFixture`, dove la geometria vive in C++ ed e' verificata da
 * `RefactorTactics.Graybox.FixtureMarkerComesFromTheLibrary` — che spawna l'attore e confronta il
 * componente posato con `URTHexLibrary::FacingMarkerOrigin`. E' cio' che chiude il buco dichiarato dalla
 * spec di #1992: *«sei test coprono la formula, non che il Blueprint la chiami»*.
 *
 * ⛔ **Non decide DOVE va la Station 01**: la posizione viene da `RTPlayground::FindStation(1)`, cioe'
 * dalla stessa planimetria che il pannello e i test consumano. Incidere qui `(-1500, 700)` sarebbe la
 * seconda copia di un numero che ha gia' un owner.
 *
 * Uso:
 *
 * ```
 * UnrealEditor-Cmd.exe <progetto> -run=RTBuildGrayboxFixtures [-Place] [-Force]
 * ```
 *
 * - senza argomenti genera il solo Blueprint e rifiuta se esiste gia';
 * - `-Place` posa anche l'istanza nella mappa e la salva;
 * - `-Force` sovrascrive un Blueprint esistente e sostituisce un'istanza gia' posata.
 */
UCLASS()
class URTBuildGrayboxFixturesCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
