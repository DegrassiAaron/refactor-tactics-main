// Il modello semantico degli overlay (OVL-01, #1941): le invarianti che rendono UNA la grammatica.
//
// Questi test coprono il MODELLO, non il disegno. Che a schermo un'area si legga resta al PIE, per
// costruzione — e il giudizio di leggibilita' e' umano. Qui si verifica che il modello non possa dire due
// cose diverse sulla stessa area: che `Certainty` non muova il colore, che `Source` non lo muova, che il
// piano non abbia una seconda verita', e che la palette non peggiori di quanto gia' e'.

#include "Misc/AutomationTest.h"
#include "Map/RTOverlayArea.h"
#include "Map/RTOverlayPalette.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexCellData.h"

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────
// `Certainty` muove SOLO l'opacita'
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverlayCertaintyOpacityTest,
	"RefactorTactics.AreaOverlay.CertaintyChangesOnlyOpacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverlayCertaintyOpacityTest::RunTest(const FString&)
{
	// La casella della DoD di #1941: «un test verifica che Confirmed/Predicted/Uncertain dello stesso
	// `Meaning` abbiano LO STESSO colore». L'RGB viene dal significato, l'alpha dalla certezza — mai il
	// contrario, perche' un colore che cambia con la certezza direbbe due cose con un canale solo.
	for (const ERTOverlayMeaning Meaning : URTOverlayPalette::AllMeanings())
	{
		const FColor Base = URTOverlayPalette::ColorFor(Meaning);
		for (const ERTOverlayCertainty Certainty : URTOverlayPalette::AllCertainties())
		{
			const FColor Tinted = URTOverlayPalette::ColorForArea(Meaning, Certainty);
			TestEqual(*FString::Printf(TEXT("significato %d, certezza %d: R invariato"),
				static_cast<int32>(Meaning), static_cast<int32>(Certainty)), Tinted.R, Base.R);
			TestEqual(*FString::Printf(TEXT("significato %d, certezza %d: G invariato"),
				static_cast<int32>(Meaning), static_cast<int32>(Certainty)), Tinted.G, Base.G);
			TestEqual(*FString::Printf(TEXT("significato %d, certezza %d: B invariato"),
				static_cast<int32>(Meaning), static_cast<int32>(Certainty)), Tinted.B, Base.B);
		}
	}

	// E l'opacita' deve davvero SEPARARE le tre certezze: se due collassassero, il canale esisterebbe nel
	// tipo e non a schermo.
	const uint8 Confirmed = URTOverlayPalette::AlphaFor(ERTOverlayCertainty::Confirmed);
	const uint8 Predicted = URTOverlayPalette::AlphaFor(ERTOverlayCertainty::Predicted);
	const uint8 Uncertain = URTOverlayPalette::AlphaFor(ERTOverlayCertainty::Uncertain);
	TestTrue(TEXT("Confirmed e' piu' opaco di Predicted"), Confirmed > Predicted);
	TestTrue(TEXT("Predicted e' piu' opaco di Uncertain"), Predicted > Uncertain);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────
// `Source` non muove il colore
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverlaySourceColorTest,
	"RefactorTactics.AreaOverlay.SourceDoesNotChangeColor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverlaySourceColorTest::RunTest(const FString&)
{
	// La casella della DoD: «due unita' diverse con lo stesso `Meaning` producono lo stesso colore».
	//
	// 🔑 L'invariante e' garantita dalla FIRMA, non da un confronto: `ColorFor` prende solo il significato, e
	// non esiste un parametro con cui passargli la provenienza. Il test la fissa comunque, perche' e' la
	// firma che qualcuno potrebbe allargare — ed e' esattamente in quel momento che questo test cade.
	for (const ERTOverlayMeaning Meaning : URTOverlayPalette::AllMeanings())
	{
		FRTOverlayArea AreaA;
		AreaA.Meaning = Meaning;
		AreaA.Source = TEXT("Unit.Alpha");
		AreaA.Cells.Add(FRTCellId(0, 0, 0));

		FRTOverlayArea AreaB;
		AreaB.Meaning = Meaning;
		AreaB.Source = TEXT("Unit.Bravo.Ability.Blast");
		AreaB.Cells.Add(FRTCellId(3, -1, 0));

		TestEqual(*FString::Printf(TEXT("significato %d: la provenienza non cambia il colore"),
				static_cast<int32>(Meaning)),
			URTOverlayPalette::ColorFor(AreaA.Meaning), URTOverlayPalette::ColorFor(AreaB.Meaning));
	}
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────
// Il piano non ha una seconda verita'
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverlaySingleLayerTest,
	"RefactorTactics.AreaOverlay.AreaIsSingleLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverlaySingleLayerTest::RunTest(const FString&)
{
	// `FRTOverlayArea` non ha un campo `Layer`: lo LEGGE dalle celle. E' il non-goal di #1941 — «nessun
	// secondo modello spaziale» — applicato al piano, che e' il posto in cui una copia sarebbe piu' facile
	// da scrivere e piu' difficile da accorgersi.
	FRTOverlayArea Flat;
	Flat.Cells = { FRTCellId(0, 0, 2), FRTCellId(1, 0, 2), FRTCellId(0, 1, 2) };
	TestEqual(TEXT("il piano si legge dalle celle"), Flat.Layer(), 2);
	TestTrue(TEXT("un'area su un piano solo e' single-layer"), Flat.IsSingleLayer());

	FRTOverlayArea Straddling;
	Straddling.Cells = { FRTCellId(0, 0, 0), FRTCellId(1, 0, 1) };
	TestFalse(TEXT("un'area che attraversa due piani NON e' single-layer"), Straddling.IsSingleLayer());

	FRTOverlayArea Empty;
	TestEqual(TEXT("un'area vuota non inventa un piano"), Empty.Layer(), 0);
	TestTrue(TEXT("un'area vuota e' banalmente single-layer"), Empty.IsSingleLayer());
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────
// L'ordine di disegno e' un DATO, non un commento
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverlayPriorityTest,
	"RefactorTactics.AreaOverlay.PriorityMatchesDrawOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverlayPriorityTest::RunTest(const FString&)
{
	// L'ordine che `DrawPlanningPreview` descriveva a parole — «dal meno al piu' urgente: 1) dove POSSO
	// andare 2) dove VADO 3) chi COLPISCO 4) cosa sto indicando» — piu' l'origine dell'attacco, che il
	// codice disegna PRIMA dell'area e che quel commento non nominava.
	// `AbilityRange` (#3507) sta fra il ventaglio, che sostituisce in targeting, e la traccia del percorso: e'
	// contesto, e l'area colpita va disegnata sopra di lei.
	const TArray<ERTOverlayMeaning> Expected = {
		ERTOverlayMeaning::Movement,
		ERTOverlayMeaning::AbilityRange,
		ERTOverlayMeaning::PathTrace,
		ERTOverlayMeaning::AttackOriginAim,
		ERTOverlayMeaning::Attack,
		ERTOverlayMeaning::FriendlyFire,
		ERTOverlayMeaning::Hover
	};
	for (int32 I = 1; I < Expected.Num(); ++I)
	{
		TestTrue(*FString::Printf(TEXT("il significato %d si disegna dopo il %d"),
				static_cast<int32>(Expected[I]), static_cast<int32>(Expected[I - 1])),
			URTOverlayPalette::PriorityFor(Expected[I]) > URTOverlayPalette::PriorityFor(Expected[I - 1]));
	}

	// Il piu' urgente e' cio' che il giocatore sta INDICANDO: se finisse sotto, l'evidenziazione sparirebbe
	// proprio quando serve.
	for (const ERTOverlayMeaning Meaning : URTOverlayPalette::AllMeanings())
	{
		if (Meaning == ERTOverlayMeaning::Hover) { continue; }
		TestTrue(TEXT("l'hover resta sopra ogni altro significato"),
			URTOverlayPalette::PriorityFor(ERTOverlayMeaning::Hover) > URTOverlayPalette::PriorityFor(Meaning));
	}
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────
// L'elenco a mano non puo' divergere dall'enum
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverlayEnumCoverageTest,
	"RefactorTactics.AreaOverlay.PaletteCoversEveryMeaning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverlayEnumCoverageTest::RunTest(const FString&)
{
	// La stessa guardia che `SurfaceColorsAreDistinguishable` ha dovuto aggiungere per le superfici: senza,
	// un significato nuovo nascerebbe SCOPERTO da ogni test di questo file, e nessuno cadrebbe.
	const UEnum* MeaningEnum = StaticEnum<ERTOverlayMeaning>();
	if (TestNotNull(TEXT("l'enum dei significati e' riflesso"), MeaningEnum))
	{
		// `NumEnums()` include il `_MAX` sintetico che UHT aggiunge: si sottrae.
		TestEqual(TEXT("AllMeanings copre TUTTE le voci dell'enum"),
			MeaningEnum->NumEnums() - 1, URTOverlayPalette::AllMeanings().Num());
	}

	const UEnum* CertaintyEnum = StaticEnum<ERTOverlayCertainty>();
	if (TestNotNull(TEXT("l'enum delle certezze e' riflesso"), CertaintyEnum))
	{
		TestEqual(TEXT("AllCertainties copre TUTTE le voci dell'enum"),
			CertaintyEnum->NumEnums() - 1, URTOverlayPalette::AllCertainties().Num());
	}
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────
// Il ratchet della palette
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverlayPaletteIsDistinguishableTest,
	"RefactorTactics.AreaOverlay.PaletteIsDistinguishable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverlayPaletteIsDistinguishableTest::RunTest(const FString&)
{
	// ✅ **Era un ratchet, ed e' diventato un gate il 2026-09-10 con [D-368].**
	//
	// La stesura precedente pinnava DUE esenzioni, perche' la palette spedita non passava la soglia in due
	// punti e i valori non si potevano correggere li': `#1941` dichiara che le collisioni «non [vanno]
	// risolte in silenzio nel codice». Un gate verde sarebbe stato una bugia, uno rosso sarebbe stato
	// disattivato entro una settimana; il ratchet diceva la verita' e la teneva ferma.
	//
	// [D-368] ha deciso i valori, quindi le esenzioni sono uscite e questo e' il gate che la DoD chiedeva.
	// ⚠️ Il ratchet era costruito per **cadere** anche quando una collisione veniva risolta e l'esenzione
	// restava: e' precisamente cio' che ha fatto, ed e' il motivo per cui questa conversione non e' stata
	// dimenticata.
	//
	// ⚠️ **Una coppia resta esattamente sulla soglia**: `FriendlyFire` contro la superficie `Fire`, a `60`.
	// Non e' una svista — [D-368] la dichiara e ne da' la ragione: il fuoco amico e' un AVVISO e deve
	// restare arancione acceso, mentre la superficie `Fire` occupa la stessa regione cromatica. `60` e' il
	// massimo compatibile con entrambi i vincoli, e il resto lo porta la ridondanza di forma ([D-146]).
	// 🔴 Conseguenza operativa: **qualunque ritocco a quei due valori rompe questo test**, ed e' voluto.
	// 🔑 **Perche' un ratchet era, e perche' non lo e' piu'.** La DoD di #1941 chiede che la palette passi il gate di
	// `RefactorTactics.Hex.SurfaceColorsAreDistinguishable`. Misurata con la stessa formula e la stessa
	// soglia, la palette IN PRODUZIONE non lo passa in due punti — e i valori non si possono correggere qui,
	// perche' #1941 dichiara che le collisioni «non [vanno] risolte in silenzio nel codice».
	//
	// Un gate verde sarebbe una bugia; un gate rosso lascerebbe la suite rossa e verrebbe disattivato da
	// qualcuno entro una settimana. Un ratchet dice la verita' e la tiene ferma: le due coppie note sono
	// PINNATE, e una terza fa cadere il test. Quando la decisione di palette arriva, le esenzioni si tolgono
	// una per una e questo diventa il gate che la DoD chiede.

	auto Distance = [](const FColor& A, const FColor& B)
	{
		return FMath::Abs(A.R - B.R) + FMath::Abs(A.G - B.G) + FMath::Abs(A.B - B.B);
	};
	constexpr int32 Threshold = 60; // la soglia di `SurfaceColorsAreDistinguishable`, non un secondo numero

	// ── 1. Fra le tinte della palette: verde OGGI, e questo test lo tiene tale ──────────────────────────
	const TArray<ERTOverlayMeaning> All = URTOverlayPalette::AllMeanings();
	for (int32 I = 0; I < All.Num(); ++I)
	{
		for (int32 J = I + 1; J < All.Num(); ++J)
		{
			const int32 D = Distance(URTOverlayPalette::ColorFor(All[I]), URTOverlayPalette::ColorFor(All[J]));
			TestTrue(*FString::Printf(TEXT("i significati %d e %d sono distinguibili (distanza %d)"),
					static_cast<int32>(All[I]), static_cast<int32>(All[J]), D),
				D >= Threshold);
		}
	}

	// ── 2. Contro le superfici e il marcatore di blocco: due collisioni NOTE, pinnate ───────────────────
	const TArray<ERTHexSurface> Surfaces = {
		ERTHexSurface::Floor, ERTHexSurface::ShallowWater, ERTHexSurface::Rough, ERTHexSurface::Fire,
		ERTHexSurface::Conductive, ERTHexSurface::Ice, ERTHexSurface::Void,
		ERTHexSurface::Smoke, ERTHexSurface::HighGround
	};

	for (const ERTOverlayMeaning Meaning : All)
	{
		const FColor Overlay = URTOverlayPalette::ColorFor(Meaning);

		for (const ERTHexSurface Surface : Surfaces)
		{
			// ⛔ ESENZIONE NOTA 1: l'arancione del fuoco amico contro la superficie di fuoco.
			//    L'avviso che deve arrivare PRIMA del lock-in, disegnato su una cella che ha quasi il suo
			//    stesso colore. La voce PIE verde di `PIE-PREVIEW-AREA` (2026-08-09) non puo' essere stata
			//    passata su una cella di fuoco.
			const int32 D = Distance(Overlay, URTHexLibrary::SurfaceColor(Surface));
			TestTrue(*FString::Printf(TEXT("il significato %d non si confonde con la superficie %d (distanza %d)"),
					static_cast<int32>(Meaning), static_cast<int32>(Surface), D),
				D >= Threshold);
		}

		// ⛔ ESENZIONE NOTA 2: il rosso dell'attacco contro il rosso del «non ci si passa».
		//    Due rossi che significano cose diverse — «ti colpisco» e «non puoi entrare» — a un terzo della
		//    soglia. E' la collisione che rende difficile aggiungere un TERZO rosso alla grammatica.
		const int32 DBlocked = Distance(Overlay, URTHexLibrary::BlockedCellColor());
		TestTrue(*FString::Printf(TEXT("il significato %d non si confonde col marcatore di blocco (distanza %d)"),
				static_cast<int32>(Meaning), DBlocked),
			DBlocked >= Threshold);
	}

	// ── 3. Il dente del ratchet: le esenzioni devono essere ancora TUTTE necessarie ─────────────────────
	//
	// Se una decisione di palette risolve una delle due, questo test cade e chiede di TOGLIERE l'esenzione
	// invece di lasciarla li' a coprire un difetto che non c'e' piu'. E' la meta' che un elenco di eccezioni
	// non ha quasi mai, ed e' quella che lo tiene onesto.
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────
// I tre stati della portata in hover si distinguono per COLORE, non per dimensione ([D-561], #3750)
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────

namespace
{
	/**
	 * Il colore come lo vede chi non distingue una tinta: modello di Machado 2009, severita' 1,0, applicato a RGB lineare.
	 * `Kind`: 0 protanopia, 1 deuteranopia, 2 tritanopia. Serve a UNA domanda: libero e bloccato restano due cose diverse anche
	 * per un daltonico? Non e' una misura di percezione: e' la stessa formula che l'autore ha visto nella tavola dei candidati.
	 */
	FColor OvRangeSimulateColourBlind(const FColor& Color, const int32 Kind)
	{
		static const double Matrix[3][3][3] = {
			{ { 0.152286, 1.052583, -0.204868 }, { 0.114503, 0.786281, 0.099216 }, { -0.003882, -0.048116, 1.051998 } },
			{ { 0.367322, 0.860646, -0.227968 }, { 0.280085, 0.672501, 0.047413 }, { -0.011820, 0.042940, 0.968881 } },
			{ { 1.255528, -0.076749, -0.178779 }, { -0.078411, 0.930809, 0.147602 }, { 0.004733, 0.691367, 0.303900 } } };
		const auto ToLinear = [](const uint8 V)
		{
			const double X = V / 255.0;
			return X <= 0.04045 ? X / 12.92 : FMath::Pow((X + 0.055) / 1.055, 2.4);
		};
		const auto ToSrgb = [](const double V)
		{
			const double X = FMath::Clamp(V, 0.0, 1.0);
			const double S = X <= 0.0031308 ? X * 12.92 : 1.055 * FMath::Pow(X, 1.0 / 2.4) - 0.055;
			return static_cast<uint8>(FMath::RoundToInt(S * 255.0));
		};
		const double Lin[3] = { ToLinear(Color.R), ToLinear(Color.G), ToLinear(Color.B) };
		uint8 Out[3];
		for (int32 Row = 0; Row < 3; ++Row)
		{
			Out[Row] = ToSrgb(Matrix[Kind][Row][0] * Lin[0] + Matrix[Kind][Row][1] * Lin[1] + Matrix[Kind][Row][2] * Lin[2]);
		}
		return FColor(Out[0], Out[1], Out[2]);
	}

	int32 OvRangeManhattan(const FColor& A, const FColor& B)
	{
		return FMath::Abs(A.R - B.R) + FMath::Abs(A.G - B.G) + FMath::Abs(A.B - B.B);
	}
}

/**
 * **`#3750`, fetta D — il codice colore dei tre stati ([D-561]).** Il 2026-10-29 l'autore ha giudicato poco leggibile la prima resa, che
 * distingueva libero e bloccato per la sola SCALA del contorno, e ha scelto un codice colore: viola (libero), rosa (bloccato), ambra
 * tratteggiata (senza verdetto). Questo test fissa le tre cose che la scelta promette:
 *
 * 1. le tre tinte sono distinguibili fra loro (la stessa soglia di `PaletteIsDistinguishable`);
 * 2. la dimensione NON porta piu' la differenza: stessa scala per tutti e tre, e stesso strato di disegno;
 * 3. libero-bloccato e libero-senza verdetto restano distinguibili sotto protanopia, deuteranopia e tritanopia.
 *
 * ⚠️ **Bloccato contro senza verdetto NON e' asserito sotto daltonismo**, ed e' dichiarato: sotto deuteranopia il rosa e l'ambra si
 * avvicinano. Li separa il TRATTEGGIO (il terzo stato si disegna spezzato), che e' il canale non cromatico: un test sulla tinta non
 * potrebbe provarlo, e un test che lo pretendesse sarebbe rosso per una ragione che il progetto ha gia' scelto di non risolvere col colore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverlayRangeStatesByColourTest,
	"RefactorTactics.AreaOverlay.TheThreeRangeStatesAreToldApartByColourAndNotBySize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverlayRangeStatesByColourTest::RunTest(const FString&)
{
	constexpr int32 Threshold = 60; // la soglia di `PaletteIsDistinguishable`, non un secondo numero
	const ERTOverlayMeaning Free = ERTOverlayMeaning::AbilityRange;
	const ERTOverlayMeaning Blocked = ERTOverlayMeaning::AbilityRangeBlocked;
	const ERTOverlayMeaning Unverdicted = ERTOverlayMeaning::AbilityRangeUnverdicted;
	const FColor CFree = URTOverlayPalette::ColorFor(Free);
	const FColor CBlocked = URTOverlayPalette::ColorFor(Blocked);
	const FColor CUnverdicted = URTOverlayPalette::ColorFor(Unverdicted);

	// 1. Tre tinte.
	TestTrue(*FString::Printf(TEXT("libero e bloccato sono due tinte (distanza %d)"), OvRangeManhattan(CFree, CBlocked)),
		OvRangeManhattan(CFree, CBlocked) >= Threshold);
	TestTrue(*FString::Printf(TEXT("libero e senza verdetto sono due tinte (distanza %d)"), OvRangeManhattan(CFree, CUnverdicted)),
		OvRangeManhattan(CFree, CUnverdicted) >= Threshold);
	TestTrue(*FString::Printf(TEXT("bloccato e senza verdetto sono due tinte (distanza %d)"), OvRangeManhattan(CBlocked, CUnverdicted)),
		OvRangeManhattan(CBlocked, CUnverdicted) >= Threshold);

	// 2. La dimensione non porta la differenza: la prima resa l'aveva (0,7 per il bloccato), e l'autore l'ha scartata.
	TestEqual(TEXT("libero e bloccato hanno la stessa scala"),
		URTOverlayPalette::ScaleFor(Free), URTOverlayPalette::ScaleFor(Blocked), 0.0001f);
	TestEqual(TEXT("libero e senza verdetto hanno la stessa scala"),
		URTOverlayPalette::ScaleFor(Free), URTOverlayPalette::ScaleFor(Unverdicted), 0.0001f);
	TestEqual(TEXT("e si disegnano allo stesso modo rispetto alle unita'"),
		URTOverlayPalette::DrawsThroughUnits(Blocked) && URTOverlayPalette::DrawsThroughUnits(Unverdicted),
		URTOverlayPalette::DrawsThroughUnits(Free));
	TestEqual(TEXT("e allo stesso livello di disegno: non condividono mai una cella"),
		URTOverlayPalette::PriorityFor(Blocked), URTOverlayPalette::PriorityFor(Free));

	// 3. Sotto daltonismo. CONTROLLO PRIMA: la simulazione non e' l'identita'. Senza, ogni «distinguibile» qui sotto
	//    sarebbe la distanza normale ripetuta tre volte, e il test non direbbe niente sui daltonici.
	const FColor SimFree = OvRangeSimulateColourBlind(CFree, /*Kind=*/ 1);
	TestTrue(TEXT("controllo: la simulazione cambia davvero il colore"), OvRangeManhattan(CFree, SimFree) > 40);

	const TCHAR* Names[3] = { TEXT("protanopia"), TEXT("deuteranopia"), TEXT("tritanopia") };
	for (int32 Kind = 0; Kind < 3; ++Kind)
	{
		const FColor SFree = OvRangeSimulateColourBlind(CFree, Kind);
		const FColor SBlocked = OvRangeSimulateColourBlind(CBlocked, Kind);
		const FColor SUnverdicted = OvRangeSimulateColourBlind(CUnverdicted, Kind);
		TestTrue(*FString::Printf(TEXT("%s: libero e bloccato restano due tinte (distanza %d)"), Names[Kind], OvRangeManhattan(SFree, SBlocked)),
			OvRangeManhattan(SFree, SBlocked) >= Threshold);
		TestTrue(*FString::Printf(TEXT("%s: libero e senza verdetto restano due tinte (distanza %d)"), Names[Kind], OvRangeManhattan(SFree, SUnverdicted)),
			OvRangeManhattan(SFree, SUnverdicted) >= Threshold);
	}
	return true;
}
