#include "Misc/AutomationTest.h"

#include "RTPlaygroundLayout.h"

#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * **La planimetria del Gray Kit Playground** (#1991, Epic #1990, `D-304`): otto pad in `40 m x 24 m`.
 *
 * 🔑 **Perche' questi test esistono, quando la scena e' un binario che nessun test apre.** Precisamente
 * per quello: un pad spostato di mezzo metro dentro un `.umap` non produce nessun segnale. Tenendo la
 * planimetria in una tabella si puo' almeno provare che la tabella e' coerente — che le otto scatole non
 * si sovrappongono, che stanno dentro il floor, che il corridoio non ne tocca nessuna. La posa in scena
 * resta un giudizio d'occhio; la geometria che la scena deve rispettare no.
 *
 * ⚠️ **Un test che confronta 8 con 8 non prova nulla**, e nessuno di questi lo fa: le asserzioni sono
 * relazioni fra i rettangoli, e cadono se qualcuno ne muove uno.
 *
 * ⛔ Nessuna regola di gioco: qui non ci sono celle, costi o occupancy. Il solo punto in cui questo file
 * tocca il modello tattico e' `MetreGuideIsNotTheHexPitch`, e lo fa per **misurare una distanza fra i due
 * mondi**, non per legarli.
 */

namespace
{
	/** Nomi distinti per file: namespace anonimo + unity build (vedi `KitTolerance` in RTGrayboxMeshTests). */
	constexpr double PlaygroundEps = 1.e-6;

	/**
	 * Area della sovrapposizione fra due rettangoli, in metri quadri.
	 *
	 * 🔴 **Non si usa `FBox2D::Intersect`, e la ragione e' nella planimetria stessa.** Quella funzione
	 * considera intersecanti due scatole che si toccano su un bordo, e qui alcune si toccano **per
	 * costruzione**: la service strip nord finisce a `Y = 3` dove le station nord cominciano. Con
	 * `Intersect` il test sarebbe rosso su un layout corretto. Cio' che va vietato e' la sovrapposizione
	 * con **area positiva**, che e' un'altra cosa.
	 */
	double PlaygroundOverlapArea(const FBox2D& A, const FBox2D& B)
	{
		const double W = FMath::Min(A.Max.X, B.Max.X) - FMath::Max(A.Min.X, B.Min.X);
		const double H = FMath::Min(A.Max.Y, B.Max.Y) - FMath::Max(A.Min.Y, B.Min.Y);
		return (W > 0.0 && H > 0.0) ? W * H : 0.0;
	}

	bool PlaygroundContains(const FBox2D& Outer, const FBox2D& Inner)
	{
		return Inner.Min.X >= Outer.Min.X - PlaygroundEps && Inner.Max.X <= Outer.Max.X + PlaygroundEps
		    && Inner.Min.Y >= Outer.Min.Y - PlaygroundEps && Inner.Max.Y <= Outer.Max.Y + PlaygroundEps;
	}
}

/**
 * Otto stazioni, numerate `1..8` senza buchi e senza ripetizioni.
 *
 * Il signage e le chip del pannello (#1993) leggono da qui: un numero doppio produrrebbe due chip che si
 * chiamano uguale e un `Focus` che porta a caso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaygroundStationCountTest,
	"RefactorTactics.Playground.LayoutHasEightNumberedStations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaygroundStationCountTest::RunTest(const FString&)
{
	const TArray<RTPlayground::FStation> S = RTPlayground::Stations();
	TestEqual(TEXT("le stazioni sono otto"), S.Num(), 8);

	TSet<int32> Seen;
	for (const RTPlayground::FStation& St : S)
	{
		TestTrue(TEXT("il numero sta in 1..8"), St.Number >= 1 && St.Number <= 8);
		bool bAlready = false;
		Seen.Add(St.Number, &bAlready);
		TestFalse(TEXT("nessun numero si ripete"), bAlready);
		TestTrue(TEXT("ogni stazione ha un nome"), FCString::Strlen(St.Name) > 0);
	}
	TestEqual(TEXT("gli otto numeri sono tutti distinti"), Seen.Num(), 8);

	// La ricerca per numero e' cio' che il pannello usa: fuori range deve tacere, non restituire una vuota.
	TestNotNull(TEXT("la 1 si trova"), RTPlayground::FindStation(1));
	TestNotNull(TEXT("la 8 si trova"), RTPlayground::FindStation(8));
	TestNull(TEXT("la 0 non esiste"), RTPlayground::FindStation(0));
	TestNull(TEXT("la 9 non esiste"), RTPlayground::FindStation(9));
	return true;
}

/**
 * 🔴 **Nessuna coppia di stazioni si sovrappone.** E' l'asserzione centrale di questo file: ventotto
 * coppie, e basta un pad spostato perche' una diventi rossa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaygroundStationsDoNotOverlapTest,
	"RefactorTactics.Playground.StationsDoNotOverlap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaygroundStationsDoNotOverlapTest::RunTest(const FString&)
{
	const TArray<RTPlayground::FStation> S = RTPlayground::Stations();

	int32 Pairs = 0;
	for (int32 i = 0; i < S.Num(); ++i)
	{
		for (int32 j = i + 1; j < S.Num(); ++j)
		{
			++Pairs;
			const double Area = PlaygroundOverlapArea(S[i].Bounds, S[j].Bounds);
			TestEqual(*FString::Printf(TEXT("station %d e %d non si sovrappongono"), S[i].Number, S[j].Number),
				Area, 0.0);
		}
	}
	// ⚠️ Senza questa riga il doppio ciclo potrebbe non confrontare nulla e il test resterebbe verde.
	TestEqual(TEXT("le coppie confrontate sono tutte"), Pairs, 28);
	return true;
}

/**
 * Ogni pad e' `8 x 8 m` e sta dentro il floor da `40 x 24`.
 *
 * Il contenimento e' cio' che impedisce a una stazione di finire fuori dal pavimento — dove sarebbe
 * invisibile alla camera Overview, che e' l'unico modo in cui la struttura si legge tutta insieme.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaygroundStationsFitTest,
	"RefactorTactics.Playground.StationsAreEightSquareAndFitTheFloor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaygroundStationsFitTest::RunTest(const FString&)
{
	const FBox2D Floor = RTPlayground::FloorBounds();
	TestEqual(TEXT("il floor e' largo 40 m"), Floor.Max.X - Floor.Min.X, 40.0);
	TestEqual(TEXT("il floor e' profondo 24 m"), Floor.Max.Y - Floor.Min.Y, 24.0);

	for (const RTPlayground::FStation& St : RTPlayground::Stations())
	{
		const FVector2D Size = St.Bounds.Max - St.Bounds.Min;
		TestEqual(*FString::Printf(TEXT("station %d larga 8 m"), St.Number), Size.X, 8.0);
		TestEqual(*FString::Printf(TEXT("station %d profonda 8 m"), St.Number), Size.Y, 8.0);
		TestTrue(*FString::Printf(TEXT("station %d sta dentro il floor"), St.Number),
			PlaygroundContains(Floor, St.Bounds));
	}
	return true;
}

/**
 * 🔴 **Il corridoio non tocca nessuna stazione**, e le service strip sono la ragione per cui.
 *
 * Un corridoio che sfiorasse un pad toglierebbe alla planimetria l'unica cosa che la rende leggibile
 * dall'alto: si passa **fra** le stazioni, non attraverso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaygroundCorridorIsClearTest,
	"RefactorTactics.Playground.CorridorTouchesNoStation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaygroundCorridorIsClearTest::RunTest(const FString&)
{
	const FBox2D Corridor = RTPlayground::CorridorBounds();
	TestEqual(TEXT("il corridoio e' largo 4 m"), Corridor.Max.Y - Corridor.Min.Y, 4.0);

	for (const RTPlayground::FStation& St : RTPlayground::Stations())
	{
		TestEqual(*FString::Printf(TEXT("il corridoio non invade la station %d"), St.Number),
			PlaygroundOverlapArea(Corridor, St.Bounds), 0.0);

		// E non la sfiora nemmeno: fra i due passa una service strip, cioe' almeno un metro.
		const double GapNorth = St.Bounds.Min.Y - Corridor.Max.Y;
		const double GapSouth = Corridor.Min.Y - St.Bounds.Max.Y;
		TestTrue(*FString::Printf(TEXT("fra corridoio e station %d resta almeno 1 m"), St.Number),
			FMath::Max(GapNorth, GapSouth) >= 1.0 - PlaygroundEps);
	}

	const TArray<FBox2D> Strips = RTPlayground::ServiceStrips();
	TestEqual(TEXT("le service strip sono due"), Strips.Num(), 2);
	for (const FBox2D& Strip : Strips)
	{
		TestEqual(TEXT("una strip e' profonda 1 m"), Strip.Max.Y - Strip.Min.Y, 1.0);
		TestEqual(TEXT("una strip non invade il corridoio"), PlaygroundOverlapArea(Strip, Corridor), 0.0);
	}
	return true;
}

/**
 * Le quattro colonne distano **2 m** l'una dall'altra.
 *
 * ⚠️ Non e' estetica: e' lo stacco che permette di distinguere due stazioni adiacenti senza contare sul
 * colore, che `D-146` vieta come canale unico.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaygroundColumnGapTest,
	"RefactorTactics.Playground.ColumnsAreTwoMetresApart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaygroundColumnGapTest::RunTest(const FString&)
{
	TArray<double> Lefts;
	for (const RTPlayground::FStation& St : RTPlayground::Stations())
	{
		Lefts.AddUnique(St.Bounds.Min.X);
	}
	Lefts.Sort();
	TestEqual(TEXT("le colonne sono quattro"), Lefts.Num(), 4);

	for (int32 i = 1; i < Lefts.Num(); ++i)
	{
		// I pad sono larghi 8 m: fra il bordo destro di una colonna e il sinistro della successiva
		// devono restare esattamente 2 m.
		TestEqual(TEXT("2 m fra una colonna e la successiva"), Lefts[i] - (Lefts[i - 1] + 8.0), 2.0);
	}
	return true;
}

/**
 * In `GKP 0.1` **una sola** stazione e' viva, ed e' la 01.
 *
 * 🔴 **Il test che difende lo scope.** Le altre sette esistono come pad e signage `PLANNED`, e non
 * finanziano i loro sistemi: il giorno in cui qualcuno accendesse la 03 «tanto e' solo un flag» questa
 * riga diventerebbe rossa, che e' l'unico posto in cui quella decisione e' falsificabile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaygroundOnlyOneLiveTest,
	"RefactorTactics.Playground.OnlyStationOneIsLive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaygroundOnlyOneLiveTest::RunTest(const FString&)
{
	int32 Live = 0;
	for (const RTPlayground::FStation& St : RTPlayground::Stations())
	{
		if (St.bLive)
		{
			++Live;
			TestEqual(TEXT("la sola stazione viva e' la 01"), St.Number, 1);
		}
	}
	TestEqual(TEXT("le stazioni vive sono una"), Live, 1);
	return true;
}

/**
 * 🔴 **La guida da 1 m NON e' il passo della griglia esagonale — e questo test MISURA la relazione
 * invece di affermarla, perche' la prima stesura di `D-304` l'aveva scritta sbagliata due volte.**
 *
 * Quella stesura diceva: *«nessun multiplo intero di una guida da 1 m cade sul passo esagonale, e chi la
 * scambiasse per una griglia vedrebbe subito che non si allinea a niente»*. Misurato chiamando
 * `URTHexLibrary::AxialToWorld` invece di rileggere la prosa:
 *
 * ```
 * Wx = HexSize * sqrt(3) * (q + r/2)     ->  passo X = 2,598076 m   (HexSize = 150)
 * Wy = HexSize * 1.5 * r                 ->  passo Y = 2,250000 m
 * ```
 *
 * - sull'asse **Y** guida e griglia coincidono **esattamente**, a `9 · 18 · 27 · 36 m`;
 * - sull'asse **X** non coincidono mai — `sqrt(3)` e' irrazionale — ma la coincidenza piu' vicina dista
 *   **0,96 cm** a `13 m`. A occhio e' allineata.
 *
 * 🔑 **Conclusione, e vale piu' del test**: l'aritmetica **non protegge** dalla confusione, in nessuno
 * dei due assi. Cio' che la impedisce e' la regola — `D-304` — e il fatto che la scena dichiari la guida
 * presentation-only, che e' un acceptance criterion di #1991 e non un corollario di un calcolo.
 *
 * Questo test resta utile per una ragione diversa da quella per cui era stato pensato: **pinna i due passi
 * ai valori derivati dal CDO**, cosi' che un cambio di scala si veda qui invece di scoprirsi a schermo, e
 * impedisce che qualcuno dichiari di nuovo l'impossibilita' della confusione.
 *
 * ⚠️ `HexSize` si legge dal **CDO**, non si scrive `150`: un test che confrontasse `150` con `150`
 * passerebbe anche dopo un cambio di scala, cioe' esattamente quando dovrebbe parlare. E' la stessa
 * disciplina dei cinque `RefactorTactics.Graybox.*`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaygroundGuideIsNotTheGridTest,
	"RefactorTactics.Playground.MetreGuideIsNotTheHexPitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaygroundGuideIsNotTheGridTest::RunTest(const FString&)
{
	const URTHexMapAsset* CDO = GetDefault<URTHexMapAsset>();
	if (!TestNotNull(TEXT("il CDO della mappa esiste"), CDO))
	{
		return false;
	}
	const float HexSize     = CDO->HexSize;
	const float LayerHeight = CDO->LayerHeight;

	// I due passi NON si riscrivono: si derivano chiamando la stessa funzione che il gioco usa. Una
	// formula ricopiata qui sarebbe la seconda risposta alla domanda «dove cade una cella».
	const FVector Origin = FVector::ZeroVector;
	const FVector C00 = URTHexLibrary::AxialToWorld(FRTCellId(0, 0, 0), Origin, HexSize, LayerHeight);
	const FVector C10 = URTHexLibrary::AxialToWorld(FRTCellId(1, 0, 0), Origin, HexSize, LayerHeight);
	const FVector C01 = URTHexLibrary::AxialToWorld(FRTCellId(0, 1, 0), Origin, HexSize, LayerHeight);

	const double PitchXMetres = FMath::Abs(C10.X - C00.X) / 100.0;
	const double PitchYMetres = FMath::Abs(C01.Y - C00.Y) / 100.0;
	const double Guide        = RTPlayground::GuideSpacingMetres;

	AddInfo(FString::Printf(TEXT("HexSize=%.1f uu -> passo X %.6f m, passo Y %.6f m, guida %.3f m"),
		HexSize, PitchXMetres, PitchYMetres, Guide));

	// 1. Nessuno dei due passi COINCIDE con la guida: se lo facesse, guida e griglia sarebbero la stessa
	//    cosa, e l'invariante di D-304 non avrebbe nemmeno un oggetto da distinguere.
	TestTrue(TEXT("il passo orizzontale non e' la guida"), FMath::Abs(PitchXMetres - Guide) > 0.1);
	TestTrue(TEXT("il passo verticale non e' la guida"),   FMath::Abs(PitchYMetres - Guide) > 0.1);

	// 2. I due passi si DICHIARANO, non si giudicano. Sull'asse X si riporta la distanza minima da una
	//    linea della guida entro il floor; sull'asse Y le coincidenze esatte. Sono i due numeri che la
	//    prima stesura di D-304 non aveva misurato, ed e' il motivo per cui questo test esiste.
	const int32 FloorSpanMetres = 40;

	// 🔴 **Guardia contro il passo NULLO, e non e' teorica: misurata il 2026-09-02.** I due cicli qui
	// sotto terminano solo se il passo e' positivo — con `Pitch == 0` la condizione `k * Pitch <= Span`
	// resta vera per sempre. Una mutazione di `AxialToWorld` (scambio di `Wx` con `Wy`) ha azzerato il
	// passo X e questo test ha **APPESO la suite per 2h24m** invece di fallire, tenendo occupato il mutex
	// globale del motore — cioe' bloccando anche gli altri checkout. Un rosso costa un secondo, un
	// appendimento costa la macchina a tutti.
	if (!TestTrue(TEXT("il passo orizzontale e' positivo"), PitchXMetres > PlaygroundEps) ||
		!TestTrue(TEXT("il passo verticale e' positivo"),   PitchYMetres > PlaygroundEps))
	{
		return false;
	}

	double ClosestGapMetres = TNumericLimits<double>::Max();
	int32  ClosestAtMetres  = 0;
	for (int32 k = 1; k * PitchXMetres <= FloorSpanMetres; ++k)
	{
		const double At  = k * PitchXMetres;
		const double Gap = FMath::Abs(At - FMath::RoundToDouble(At));
		if (Gap < ClosestGapMetres)
		{
			ClosestGapMetres = Gap;
			ClosestAtMetres  = FMath::RoundToInt(At);
		}
	}
	AddInfo(FString::Printf(
		TEXT("asse X: mai coincidente (sqrt(3) e' irrazionale), ma a %d m la distanza scende a %.2f cm"),
		ClosestAtMetres, ClosestGapMetres * 100.0));

	TArray<int32> VerticalCoincidences;
	for (int32 k = 1; k * PitchYMetres <= FloorSpanMetres; ++k)
	{
		const double At = k * PitchYMetres;
		if (FMath::Abs(At - FMath::RoundToDouble(At)) < 1.e-9)
		{
			VerticalCoincidences.Add(FMath::RoundToInt(At));
		}
	}
	AddInfo(FString::Printf(TEXT("asse Y: %d coincidenze esatte entro %d m%s"),
		VerticalCoincidences.Num(), FloorSpanMetres,
		VerticalCoincidences.Num() > 0
			? *FString::Printf(TEXT(" (la prima a %d m)"), VerticalCoincidences[0])
			: TEXT("")));

	// 3. 🔴 L'asserzione che conta, ed e' l'opposto di quella che la prosa suggeriva: la coincidenza
	//    ESISTE. Il test la pinna perche' nessuno torni a scrivere che e' impossibile — se un giorno un
	//    cambio di HexSize la facesse sparire, questa riga lo direbbe, e la prosa andrebbe riletta di
	//    nuovo invece di restare vera per caso.
	TestTrue(TEXT("guida e griglia SI incontrano sull'asse Y: l'aritmetica non protegge, la regola si'"),
		VerticalCoincidences.Num() > 0);

	return true;
}

/**
 * La conversione metri -> unita' Unreal ha un solo posto, e questo test lo pinna.
 *
 * ⚠️ Il caso `0` non e' cerimonia: una conversione scritta come `Metres * 100 + Offset` passerebbe su un
 * valore solo, e fallirebbe qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaygroundUnitsTest,
	"RefactorTactics.Playground.MetresConvertToUnrealUnits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaygroundUnitsTest::RunTest(const FString&)
{
	TestEqual(TEXT("zero resta zero"), RTPlayground::WorldFromMetres(0.0), 0.0);
	TestEqual(TEXT("un metro e' cento unita'"), RTPlayground::WorldFromMetres(1.0), 100.0);
	TestEqual(TEXT("il floor e' lungo 4000 unita'"), RTPlayground::WorldFromMetres(40.0), 4000.0);
	TestEqual(TEXT("e la conversione e' simmetrica"), RTPlayground::WorldFromMetres(-2.5), -250.0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
