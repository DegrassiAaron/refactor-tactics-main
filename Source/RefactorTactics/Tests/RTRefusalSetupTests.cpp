// L'allestimento che rende raggiungibile il rifiuto per COPERTURA — `#3085`.
//
// 🔑 **La domanda che questo file risponde, e perche' esiste.** La seduta PIE del 2026-09-12 ha provato
// quattro volte a produrre un rifiuto `Cover` in partita libera e non ci e' riuscita, ogni volta per una
// ragione diversa: i tasti, il click sulla cella, il velo, e infine la geometria che si dissolve mentre i
// bot avanzano. Il caso NON si lascia raggiungere a mano.
//
// ⛔ **E non si lascia raggiungere nemmeno con uno scenario dell'harness**: quello e' auto-run e scrive i
// campi `Planned*` direttamente, mentre un rifiuto nasce da un CLICK. Cio' che serve e' un allestimento —
// una mappa le cui posizioni di partenza producano il caso al turno 1 — e questo file misura quali delle
// fixture esistenti lo facciano, prima che se ne scriva una nuova (`SEARCH -> REUSE -> CREATE`).
//
// 🔴 **Il caso e' piu' stretto di quanto sembri, ed e' la ragione per cui la seduta manuale falliva.**
// `ARTHUD::ShouldDrawUnitOverlay` rende noto un avversario solo se `Visibility == Live`, cioe' visto ORA:
// un nemico dietro un muro di solito non e' nemmeno NOTO, e `RefusalForObserver` restituisce allora
// `Nothing`, non `Cover`. Il rifiuto per copertura vive quindi in una fessura precisa:
//
//     la conoscenza e' di SQUADRA   ->  basta che UN compagno veda il nemico
//     la linea di tiro e' per UNITA' ->  il tiratore puo' non averla
//
// ∴ serve **un compagno che veda** e **un altro che non abbia la linea**, nello stesso istante.

#include "Misc/AutomationTest.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexVisionLibrary.h"
#include "Debug/RTDebugReportLibrary.h" // la stessa funzione che rt.Debug.Refusal usa (#3107)
#include "Turn/RTMatchSetupLibrary.h"
#include "Terrain/RTTerrainLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTRefusalSetup
{
	/** Una fixture che produce il caso, e le tre celle che lo compongono. */
	struct FEsito
	{
		bool bTrovato = false;
		FRTCellId Vede;      // il compagno che VEDE il nemico
		FRTCellId Coperto;   // il compagno che NON ha la linea di tiro
		FRTCellId Nemico;    // il bersaglio
	};

	/**
	 * Cerca nella mappa una terna che produce il rifiuto per copertura, con le posizioni che
	 * `PickStartCells` assegna davvero — non con celle scelte a mano, che proverebbero una partita che
	 * nessuno gioca.
	 *
	 * ⚠️ `VisionRange` non e' un parametro: la vista si approssima con la sola LOS piu' la distanza, che e'
	 * la condizione NECESSARIA. Una terna trovata qui va comunque guardata in PIE — questo test dice dove
	 * cercare, non che si veda.
	 */
	FEsito CercaTerna(const URTHexMapAsset* Map, int32 UnitsPerTeam, int32 RaggioVisivo)
	{
		FEsito Out;
		if (!Map)
		{
			return Out;
		}

		const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Map, UnitsPerTeam, 0);
		if (Start.Num() != UnitsPerTeam * 2)
		{
			return Out;
		}

		// Contratto di `PickStartCells`: le prime `UnitsPerTeam` al team 0, le successive al team 1.
		for (int32 INemico = UnitsPerTeam; INemico < Start.Num(); ++INemico)
		{
			for (int32 IVede = 0; IVede < UnitsPerTeam; ++IVede)
			{
				const int32 DistVede = URTHexLibrary::HexDistance(Start[IVede], Start[INemico]);
				const bool bVede = DistVede <= RaggioVisivo
					&& URTHexVisionLibrary::HasLineOfSight(Map, Start[IVede], Start[INemico]);
				if (!bVede)
				{
					continue;
				}

				for (int32 ICoperto = 0; ICoperto < UnitsPerTeam; ++ICoperto)
				{
					if (ICoperto == IVede)
					{
						continue;
					}
					// 🔑 L'altra meta': il secondo compagno NON ha la linea sullo stesso nemico.
					if (!URTHexVisionLibrary::HasLineOfSight(Map, Start[ICoperto], Start[INemico]))
					{
						Out.bTrovato = true;
						Out.Vede = Start[IVede];
						Out.Coperto = Start[ICoperto];
						Out.Nemico = Start[INemico];
						return Out;
					}
				}
			}
		}
		return Out;
	}
}

/**
 * Quali fixture producono il caso al turno 1 — la misura che decide se serve scriverne una nuova.
 *
 * ⛔ **Non asserisce che una fixture lo produca**: e' una SONDA, e il suo valore e' l'elenco che stampa.
 * Un'asserzione qui fisserebbe come contratto una proprieta' che nessuna fixture dichiara di avere, e
 * cadrebbe al primo ritocco di geometria fatto per un'altra ragione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRefusalSetupProbeTest,
	"RefactorTactics.Setup.WhichFixturesAllowACoverRefusal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRefusalSetupProbeTest::RunTest(const FString&)
{
	// Il raggio visivo piu' lungo del roster: e' la condizione piu' PERMISSIVA, quindi una fixture che non
	// produce il caso nemmeno qui non lo produce con nessun eroe.
	constexpr int32 RaggioPiuLungo = 7;
	constexpr int32 UnitaPerSquadra = 2;

	int32 Trovate = 0;
	for (const FString& Nome : URTMatchSetupLibrary::KnownFixtureIds())
	{
		const URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFixtureArena(GetTransientPackage(), Nome);
		if (!Map)
		{
			AddInfo(FString::Printf(TEXT("  %-14s  (non costruita)"), *Nome));
			continue;
		}

		const RTRefusalSetup::FEsito E =
			RTRefusalSetup::CercaTerna(Map, UnitaPerSquadra, RaggioPiuLungo);

		if (E.bTrovato)
		{
			++Trovate;
			AddInfo(FString::Printf(
				TEXT("  %-14s  SI — vede %s · coperto %s · nemico %s"),
				*Nome, *E.Vede.ToString(), *E.Coperto.ToString(), *E.Nemico.ToString()));
		}
		else
		{
			AddInfo(FString::Printf(TEXT("  %-14s  no"), *Nome));
		}
	}

	AddInfo(FString::Printf(TEXT("fixture che producono il caso: %d"), Trovate));

	// L'unica asserzione: la sonda ha davvero guardato qualcosa. Senza, un elenco vuoto — nessuna fixture
	// costruita, o `KnownFixtureIds` svuotato da un refactor — sarebbe indistinguibile da «nessuna lo
	// produce», che e' una risposta diversa e porterebbe a scrivere una fixture che forse gia' esiste.
	TestTrue(TEXT("la sonda ha esaminato almeno una fixture"),
		URTMatchSetupLibrary::KnownFixtureIds().Num() > 0);
	return true;
}

/**
 * La sonda distingue i tre casi, e non ne confonde due — `#3107`.
 *
 * 🔑 **La stessa funzione che `rt.Debug.Refusal` usa a runtime.** Prima di questo test la logica viveva
 * solo qui dentro, in `Tests/`, cioe' non era raggiungibile da una partita: e' esattamente la ragione per
 * cui la seduta del 2026-09-12 ha dovuto indovinare per cinque tentativi.
 *
 * ⛔ **Le tre risposte negative NON sono intercambiabili**, ed e' il punto del test. «Nessun nemico noto» e
 * «tutti in vista» portano a due gesti opposti — avvicinare un compagno, oppure spostare il tiratore — e un
 * comando che rispondesse «no» senza distinguerli lascerebbe chi legge davanti alle stesse ipotesi che
 * hanno bruciato la seduta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRefusalReachabilityTellsTheCasesApartTest,
	"RefactorTactics.Setup.RefusalReachabilityTellsTheCasesApart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRefusalReachabilityTellsTheCasesApartTest::RunTest(const FString&)
{
	const URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFixtureArena(GetTransientPackage(), TEXT("CoverYard"));
	if (!TestNotNull(TEXT("la fixture CoverYard si costruisce"), Map))
	{
		return false;
	}

	// Le celle che la sonda di questo stesso file misura su `CoverYard`: il tiratore non vede il nemico,
	// il compagno si'.
	const FRTCellId CellaCoperto(-3, 0, 0);
	const FRTCellId CellaVede(-3, 1, 0);
	const FRTCellId CellaNemico(3, 0, 0);

	auto Unita = [](int32 Id, int32 Team, const FRTCellId& Cella, bool bNoto)
	{
		FRTRefusalProbeUnit U;
		U.UnitId = Id;
		U.TeamId = Team;
		U.Cell = Cella;
		U.bKnownToObserver = bNoto;
		return U;
	};

	// --- il caso che il comando esiste per trovare ---
	{
		const TArray<FRTRefusalProbeUnit> Campo = {
			Unita(1, 0, CellaCoperto, false),
			Unita(2, 0, CellaVede,    false),
			Unita(3, 1, CellaNemico,  true) };

		const FRTRefusalReachability R =
			URTDebugReportLibrary::DescribeRefusalReachability(Map, 0, Campo);

		TestTrue(TEXT("il caso e' raggiungibile"), R.bReachable);
		TestEqual(TEXT("indica come tiratore quello SENZA linea di tiro"), R.ShooterUnitId, 1);
		TestEqual(TEXT("indica come bersaglio il nemico noto"), R.TargetUnitId, 3);
		// 🔑 Il testimone e' la meta' del caso che non si vede guardando il campo.
		TestEqual(TEXT("nomina il compagno che lo rende noto"), R.WitnessUnitId, 2);
	}

	// --- negativo 1: il nemico NON e' noto -> il rifiuto sarebbe `Nothing`, non `Cover` ---
	{
		const TArray<FRTRefusalProbeUnit> Campo = {
			Unita(1, 0, CellaCoperto, false),
			Unita(3, 1, CellaNemico,  false) };

		const FRTRefusalReachability R =
			URTDebugReportLibrary::DescribeRefusalReachability(Map, 0, Campo);

		TestFalse(TEXT("senza un nemico noto il caso non c'e'"), R.bReachable);
		// ⚠️ Il discriminante e' `Nothing`, il nome dell'esito, e non la parola «noto»: quella compare in
		// ENTRAMBE le ragioni, e `FString::Contains` ignora le maiuscole di default — un'asserzione su
		// «NOTO» sarebbe verde su tutte e due, cioe' non distinguerebbe i due casi che questo test separa.
		TestTrue(TEXT("e la ragione nomina l'esito che ne seguirebbe"),
			R.Reason.Contains(TEXT("Nothing")));
	}

	// --- negativo 2: tutti hanno la linea -> niente da rifiutare, ed e' una ragione DIVERSA ---
	{
		const TArray<FRTRefusalProbeUnit> Campo = {
			Unita(2, 0, CellaVede,   false),
			Unita(3, 1, CellaNemico, true) };

		const FRTRefusalReachability R =
			URTDebugReportLibrary::DescribeRefusalReachability(Map, 0, Campo);

		TestFalse(TEXT("se l'unica unita' propria ha la linea, non c'e' copertura da rifiutare"), R.bReachable);
		// ⛔ L'asserzione che impedisce di confondere i due negativi: la ragione dev'essere un'ALTRA.
		TestFalse(TEXT("e la ragione NON e' quella della conoscenza"),
			R.Reason.Contains(TEXT("Nothing")));
		TestTrue(TEXT("la ragione nomina la linea di tiro"),
			R.Reason.Contains(TEXT("linea di tiro")));
	}

	// --- negativo 3: senza mappa non si finge una risposta ---
	{
		const FRTRefusalReachability R =
			URTDebugReportLibrary::DescribeRefusalReachability(nullptr, 0, {});
		TestFalse(TEXT("senza mappa il caso non e' raggiungibile"), R.bReachable);
		TestFalse(TEXT("e la ragione non e' vuota"), R.Reason.IsEmpty());
	}

	return true;
}

/**
 * Quali fixture hanno un Fumo FUORI dalla vista della squadra al turno 1 — la misura di `#3818`.
 *
 * Sonda, non contratto: stampa per ogni fixture e per ogni squadra le celle di Fumo la cui distanza dalla
 * unita' della squadra supera il raggio visivo piu' lungo del roster (7) o sta dietro una cella che nega la vista,
 * cioe' che nessun eroe puo' aver osservato in partenza. Serve a scegliere dove allestire il click su Fumo mai visto (`#3270`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnexploredSmokeSetupProbeTest,
	"RefactorTactics.Setup.WhichFixturesHideSmokeFromATeamAtTurnOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnexploredSmokeSetupProbeTest::RunTest(const FString&)
{
	constexpr int32 RaggioPiuLungo = 7;
	constexpr int32 UnitaPerSquadra = 2;

	for (const FString& Nome : URTMatchSetupLibrary::KnownFixtureIds())
	{
		const URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFixtureArena(GetTransientPackage(), Nome);
		if (!Map)
		{
			continue;
		}
		const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Map, UnitaPerSquadra, 0);
		if (Start.Num() != UnitaPerSquadra * 2)
		{
			continue;
		}

		for (const FRTCellId& Id : Map->CellsInLayer(0))
		{
			const FRTHexCellData* Data = Map->FindCell(Id);
			if (!Data || Data->Surface != ERTHexSurface::Smoke)
			{
				continue;
			}
			for (int32 Team = 0; Team < 2; ++Team)
			{
				int32 Min = TNumericLimits<int32>::Max();
				bool bVisto = false;
				for (int32 i = 0; i < UnitaPerSquadra; ++i)
				{
					const FRTCellId& Da = Start[Team * UnitaPerSquadra + i];
					const int32 Dist = URTHexLibrary::HexDistance(Da, Id);
					Min = FMath::Min(Min, Dist);
					// Al buio per DISTANZA oltre il raggio, oppure per VISTA negata da un muro sulla linea.
					bVisto |= Dist <= RaggioPiuLungo && URTHexVisionLibrary::HasLineOfSight(Map, Da, Id);
				}
				AddInfo(FString::Printf(TEXT("  %-12s fumo %s  squadra %d  distanza minima %d  %s  (partenze %s %s)"),
					*Nome, *Id.ToString(), Team, Min, !bVisto ? TEXT("AL BUIO") : TEXT("in vista possibile"),
					*Start[Team * UnitaPerSquadra].ToString(), *Start[Team * UnitaPerSquadra + 1].ToString()));
			}
		}
	}

	TestTrue(TEXT("la sonda ha esaminato almeno una fixture"), URTMatchSetupLibrary::KnownFixtureIds().Num() > 0);
	return true;
}

/**
 * `SmokeDark` produce il caso: alla partenza il Fumo non e' in vista, dall'arrivo dello scatto si' e cappa — `#3818`.
 *
 * Le tre meta' sono insieme perche' ciascuna da sola e' vera anche su una fixture sbagliata: un Fumo non
 * visto ma che non cappa non e' il caso, e un Fumo che cappa ma e' visto dalla partenza non e' «mai visto».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSmokeDarkHidesTheSmokeUntilTheDashTest,
	"RefactorTactics.Setup.SmokeDarkHidesTheSmokeUntilTheDash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSmokeDarkHidesTheSmokeUntilTheDashTest::RunTest(const FString&)
{
	const URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFixtureArena(GetTransientPackage(), TEXT("SmokeDark"));
	if (!TestNotNull(TEXT("la fixture SmokeDark si costruisce"), Map))
	{
		return false;
	}

	const FRTCellId Fumo(-2, 0, 0);
	const FRTCellId Arrivo(-3, 0, 0);
	const FRTCellId Bersaglio(1, 0, 0);

	const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Map, 2, 0);
	if (!TestEqual(TEXT("le partenze sono quattro"), Start.Num(), 4))
	{
		return false;
	}
	TestEqual(TEXT("la prima partenza della squadra 0"), Start[0], FRTCellId(-6, 0, 0));
	TestEqual(TEXT("la seconda partenza della squadra 0"), Start[1], FRTCellId(-6, 1, 0));

	// Il Fumo c'e'...
	const FRTHexCellData* Cella = Map->FindCell(Fumo);
	TestTrue(TEXT("la cella (-2,0) e' Fumo"), Cella && Cella->Surface == ERTHexSurface::Smoke);

	// ...e nessuna partenza della squadra 0 lo vede, ne' alla portata del roster (7).
	for (int32 i = 0; i < 2; ++i)
	{
		TestTrue(TEXT("il Fumo e' entro 7 celle: e' il muro, non la distanza, a nasconderlo"),
			URTHexLibrary::HexDistance(Start[i], Fumo) <= 7);
		TestFalse(*FString::Printf(TEXT("la partenza %s non ha la linea sul Fumo"), *Start[i].ToString()),
			URTHexVisionLibrary::HasLineOfSight(Map, Start[i], Fumo));
	}

	// Controlli positivi. Arrivo e Fumo sono adiacenti, quindi quella linea ha solo gli estremi e passerebbe con
	// qualunque muro: il controllo che discrimina e' una linea LUNGA a est della colonna, che deve esistere,
	// mentre la stessa distanza attraverso la colonna no. E la colonna blocca la vista, non il passo.
	TestTrue(TEXT("dall'arrivo dello scatto una cella lontana a est e' in vista"),
		URTHexVisionLibrary::HasLineOfSight(Map, Arrivo, Bersaglio));
	TestFalse(TEXT("la stessa distanza attraverso la colonna q=-5 non lo e'"),
		URTHexVisionLibrary::HasLineOfSight(Map, FRTCellId(-6, 0, 0), FRTCellId(-3, 0, 0)));
	const FRTHexCellData* Colonna = Map->FindCell(FRTCellId(-5, 0, 0));
	TestTrue(TEXT("la colonna blocca la vista e non il passo"),
		Colonna && Colonna->bBlocksLineOfSight && !Colonna->bBlocksMovement);

	// E cappa: bersaglio a 4 celle, portata 5 -> 2 attraverso il Fumo; la stessa distanza su una riga senza Fumo no.
	TestEqual(TEXT("attraverso il Fumo la portata e' il cap"),
		URTTerrainLibrary::EffectiveTargetingRange(Map, Arrivo, Bersaglio, 5), 2);
	TestEqual(TEXT("senza Fumo sulla riga la portata resta intera"),
		URTTerrainLibrary::EffectiveTargetingRange(Map, FRTCellId(-3, 1, 0), FRTCellId(1, 1, 0), 5), 5);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
