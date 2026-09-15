#include "RTScenarioViewportModel.h"

#include "Map/RTHexLibrary.h"
#include "Map/RTMapVisuals.h"
#include "ScenarioHarness/RTScenarioDraft.h" // FRTScenarioUnitView
#include "Unit/RTUnit.h"                     // ARTUnit::TeamColorFor — la regola id -> colore vive li'

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

		/**
		 * Il pannello che segna un lato del confine visibile: le stesse proporzioni dei pannelli di copertura
		 * e porta di `ARTHexMapActor` (`RTEdgePanelThickness` 0,10 e `RTEdgePanelWidth` 0,92), perche' il
		 * confine e' un'altra cosa disegnata sullo stesso vocabolario di lati e non un secondo linguaggio.
		 *
		 * ⚠️ **`0.92` e non `1.0`**: un pannello largo quanto il lato intero si incastrerebbe negli angoli
		 * con quello adiacente, e su un confine concavo — dove due lati esposti della stessa cella si
		 * incontrano — l'incastro si vede.
		 */
		constexpr float BorderPanelThickness = 0.10f;
		constexpr float BorderPanelWidth = 0.92f;

		/**
		 * Altezza del pannello, in unita' mondo. Basso apposta: il confine deve **accompagnare** la lettura
		 * del velo, non sostituirla — e soprattutto non coprire percorso, AoE o marker di bersaglio, che
		 * stanno sopra la faccia della cella.
		 */
		constexpr float BorderPanelHeight = 18.f;
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

	FLinearColor TeamBodyColor(int32 TeamId)
	{
		// `Clamp` e non `%`, per la ragione gia' scritta su `TeamRingScale`: la squadra che tornasse al colore
		// della `0` sarebbe una coppia indistinguibile, e riavvolgersi e' peggio che fermarsi.
		const int32 Step = FMath::Clamp(TeamId, 0, MaxDistinctTeams - 1);

		// 🔑 **Le prime due sono le stesse del HUD**, verbatim da `RTHudViewModel.cpp:235`: un designer che
		// guarda l'anteprima e poi la partita deve vedere lo stesso azzurro e lo stesso rosso, altrimenti i
		// due schermi si contraddicono su una cosa che entrambi dichiarano.
		//
		// ⚠️ **La terza e la quarta non hanno un owner altrove, e non ne inventano uno**: sono le uniche due
		// aggiunte qui, per le squadre che il HUD non ha mai dovuto mostrare — in partita chi guarda vede
		// alleato o nemico, e la distinzione fra tre avversari non gli e' mai stata posta.
		static const FLinearColor Palette[MaxDistinctTeams] = {
			FLinearColor(0.55f, 0.75f, 1.f, 1.f),   // 0 — l'azzurro del HUD
			FLinearColor(1.f, 0.62f, 0.55f, 1.f),   // 1 — il rosso del HUD
			FLinearColor(0.62f, 1.f, 0.60f, 1.f),   // 2
			FLinearColor(1.f, 0.92f, 0.55f, 1.f),   // 3
		};

		// ⛔ La scelta fra i primi due passa da `ARTUnit::TeamColorFor` e non da un ternario scritto qui:
		// quella funzione possiede la regola ed e' dichiarata pura proprio per poter essere riusata.
		return Step <= 1 ? ARTUnit::TeamColorFor(Step, Palette[0], Palette[1]) : Palette[Step];
	}

	FTransform BorderEdgeTransform(const FRTCellId& Cell, ERTHexDirection Dir,
		const FVector& Origin, float HexSize, float LayerHeight)
	{
		// Il punto e' quello che la libreria deriva dai due centri di cella: lo stesso che `EdgeMidpointWorld`
		// da' guardando dall'altra cella, e per questo un lato posato una volta sola non lascia buchi.
		FVector Center = URTHexLibrary::EdgeMidpointWorld(Cell, Dir, Origin, HexSize, LayerHeight);
		Center.Z += RTCellTopZ + BorderPanelHeight * 0.5f;

		// Il cubo engine e' 100 uu per lato: X sottile (spessore), Y lungo il bordo, Z l'altezza. La
		// larghezza segue `HexSize` perche' il lato di un esagono cresce con esso: una larghezza fissa
		// lascerebbe fessure fra un pannello e il successivo su mappe a passo largo.
		return FTransform(URTHexLibrary::EdgeRotation(Cell, Dir), Center,
			FVector(BorderPanelThickness,
				HexSize / 100.f * BorderPanelWidth,
				BorderPanelHeight / 100.f));
	}
}
