#include "RTHexProbeReadout.h"

#include "RTHexHoverGate.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Turn/RTHexSimLibrary.h"

namespace RTHexProbe
{
	namespace
	{
		/** Il trattino che il pannello mostra quando non c'e' un valore. Uno solo, per non averne due forme. */
		const FString Dash = TEXT("—");
	}

	FBudget ResolveBudget(FName HeroId)
	{
		// 🔑 La stessa fonte da cui `ARTUnit` prende `MoveRange` e da cui il Composer prende il budget della
		// sua anteprima: `URTHeroData::MovePoints`. Non e' una stima, e' il valore.
		for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (Hero && Hero->HeroId == HeroId)
			{
				return FBudget{ /*bKnown=*/ true, Hero->MovePoints };
			}
		}
		return FBudget();
	}

	FReadout Describe(bool bHasUnit, ERTHexProbeExclusion Exclusion, int32 Cost, int32 Budget, int32 PathCells,
		bool bKnownHero)
	{
		FReadout Out;
		Out.Cost = Dash;

		if (!bHasUnit)
		{
			// Non e' un'esclusione: e' l'assenza della domanda. Dirlo con la frase di `NoRoute` sarebbe un
			// verdetto sulla mappa dove non c'e' ancora nessuno a cui applicarlo.
			Out.Reason = TEXT("nessuna unita' selezionata: scegli un profilo e clicca una cella di partenza");
			return Out;
		}

		switch (Exclusion)
		{
		case ERTHexProbeExclusion::Reachable:
			Out.Cost = FString::Printf(TEXT("%d / %d MP"), Cost, Budget);
			// I passi sono le celle meno la partenza. Con zero celle non c'e' un percorso da contare: il
			// `Max` evita il `-1` che si vedrebbe come «un passo indietro».
			Out.Steps = FMath::Max(0, PathCells - 1);
			Out.Reason = Dash;
			break;

		case ERTHexProbeExclusion::NotOnMap:
			Out.Reason = TEXT("fuori dalla mappa");
			break;

		case ERTHexProbeExclusion::BlocksMovement:
			Out.Reason = TEXT("ostacolo: la cella non si attraversa");
			break;

		case ERTHexProbeExclusion::Occupied:
			Out.Reason = TEXT("occupata da un'altra unita'");
			break;

		case ERTHexProbeExclusion::OutOfBudget:
			// 🔴 **Tre destinazioni, non due.** Senza eroe noto il budget e' `0` per assenza di dato, e
			// chiamarlo «fuori budget» manderebbe a cambiare il NUMERO invece del NOME — il motivo giusto
			// detto della cosa sbagliata, che e' il difetto per cui #711 esiste.
			Out.Reason = bKnownHero
				? FString::Printf(TEXT("fuori budget: %d MP non bastano"), Budget)
				: TEXT("nessun eroe con questo id nel catalogo: correggi HeroId");
			break;

		case ERTHexProbeExclusion::NoRoute:
			Out.Reason = TEXT("nessuna strada: non ci si arriva a nessun budget");
			break;
		}

		return Out;
	}

	bool ShouldRequery(bool bLastValid, const FRTCellId& Last, bool bNowValid, const FRTCellId& Now)
	{
		// La regola e' una sola e vive in `RTHexHover`: qui si CHIAMA. Vedi il commento sull'estrazione.
		return RTHexHover::ShouldRequery(bLastValid, Last, bNowValid, Now);
	}
}
