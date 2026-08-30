#include "RTScenarioViewportModel.h"

#include "Map/RTHexLibrary.h"
#include "Map/RTMapVisuals.h"
#include "ScenarioHarness/RTScenarioDraft.h" // FRTScenarioUnitView

namespace RTScenarioViewport
{
	namespace
	{
		/** Raggio della squadra `0`: lo stesso di `ARTUnit::TeamRing`, non un numero nuovo. */
		constexpr float BaseTeamRingScale = 1.6f;

		/**
		 * Passo fra una squadra e la successiva. Quattro squadre distinguibili, poi si ferma.
		 *
		 * 🔴 **Valeva `0.35` e il suo test l'ha bocciato**, che e' la ragione per cui quel test esiste: la quarta
		 * squadra otteneva un anello di raggio `2.65 x 50 = 132` uu, cioe' **265 uu di diametro** contro un
		 * passo di griglia di ~260 alla `HexSize` corrente. Due unita' adiacenti avrebbero avuto gli anelli
		 * sovrapposti — leggibile come una sola pedina larga, e nessun errore l'avrebbe detto.
		 *
		 * ⚠️ **Il margine e' misurato, non stimato**: `TeamsDifferByShape` ricava il passo di griglia dai due
		 * centri di cella invece di scrivere `sqrt(3) x HexSize` a mano, e confronta. Con `0.28` il diametro
		 * massimo e' 244 uu contro 260: **16 uu di margine**, il 6%.
		 *
		 * ⛔ **Il vincolo dipende da `HexSize`, e questo numero no.** E' la convenzione di `ARTUnit`, i cui
		 * anelli sono anch'essi in scala assoluta: su una mappa con `HexSize` sensibilmente minore di 150 il
		 * margine si consumerebbe. Non si corregge qui unilateralmente — sarebbe un secondo vocabolario
		 * accanto a quello dell'unita' in partita — ma il test lo pinna al valore corrente, quindi il giorno
		 * in cui `HexSize` scende diventa rosso invece che sbagliato in silenzio.
		 */
		constexpr float TeamRingStep = 0.28f;
		constexpr int32 MaxDistinctTeams = 4;
	}

	FTransform MarkerTransform(const FRTCellId& Cell, ERTHexDirection Facing,
		const FVector& Origin, float HexSize, float LayerHeight)
	{
		const FVector Center = URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerHeight);

		// Il facing E' «dove sta il vicino»: la direzione si prende fra i due centri invece che da una
		// tabella di angoli. Vedi il perche' esteso nell'header.
		const FRTCellId Ahead = URTHexLibrary::Neighbor(Cell, Facing);
		const FVector Toward = URTHexLibrary::AxialToWorld(Ahead, Origin, HexSize, LayerHeight) - Center;

		FRotator Rotation = FRotator::ZeroRotator;
		if (!Toward.IsNearlyZero())
		{
			// Solo lo yaw: un marcatore inclinato direbbe qualcosa che il facing non dice.
			Rotation = FRotator(0.f, Toward.Rotation().Yaw, 0.f);
		}

		return FTransform(Rotation, Center + FVector(0.f, 0.f, RTCellTopZ), FVector::OneVector);
	}

	TArray<int32> LayersInUse(const TArray<FRTScenarioUnitView>& Units)
	{
		TArray<int32> Layers;
		for (const FRTScenarioUnitView& Unit : Units)
		{
			Layers.AddUnique(Unit.Cell.Layer);
		}
		// Ordine crescente e non quello del file: e' una dichiarazione di cosa si sta mostrando, e un elenco
		// di piani che cambia ordine quando cambia l'ordine delle unita' si legge come se fosse cambiato
		// qualcosa.
		Layers.Sort();
		return Layers;
	}

	FString DescribeLayers(const TArray<int32>& Layers)
	{
		if (Layers.Num() == 0)
		{
			return TEXT("nessun layer");
		}

		TArray<FString> Parts;
		Parts.Reserve(Layers.Num());
		for (const int32 Layer : Layers)
		{
			Parts.Add(FString::Printf(TEXT("L%d"), Layer));
		}
		return FString::Join(Parts, TEXT(", "));
	}

	float TeamRingScale(int32 TeamId)
	{
		// `Clamp` e non `%`: con il modulo la squadra 4 tornerebbe al raggio della 0, cioe' due squadre
		// indistinguibili invece di due che condividono l'ultimo raggio disponibile. Fermarsi e' leggibile,
		// riavvolgersi no.
		const int32 Step = FMath::Clamp(TeamId, 0, MaxDistinctTeams - 1);
		return BaseTeamRingScale + TeamRingStep * static_cast<float>(Step);
	}

	float MaxTeamRingScale()
	{
		return TeamRingScale(MaxDistinctTeams - 1);
	}
}
