#include "Misc/AutomationTest.h"

#include "RTHexTransitionGlyph.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * COME SI LEGGE UN ARCO DI TRANSIZIONE (#1768).
 *
 * ⛔ **La leggibilità non diventa un test logico, ed è un divieto della issue**: l'oracolo di *«si
 * distingue»* non esiste nell'harness e simularlo produrrebbe un verde su un'altra domanda. Quella metà
 * è della seduta, e la voce PIE la nomina.
 *
 * 🔑 **Ciò che un test PUÒ vedere è il confine, e sono tre cose**: che i canali siano **due** e non uno,
 * che restino distinti dopo una conversione a scala di grigi — che è il modo in cui `D-146` intende la
 * ridondanza — e che nessuno stato si distingua per sola opacità.
 */

namespace
{
	/** Nomi prefissati per file: namespace anonimo + unity build. */
	const ERTHexTransitionKind GlyphAllKinds[] = {
		ERTHexTransitionKind::Stair, ERTHexTransitionKind::Ramp, ERTHexTransitionKind::Bridge,
		ERTHexTransitionKind::Tunnel, ERTHexTransitionKind::Elevator, ERTHexTransitionKind::Jump };

	const ERTHexArcState GlyphAllStates[] = {
		ERTHexArcState::Active, ERTHexArcState::Inactive, ERTHexArcState::Destroyed };

	/** Contati dagli array qui sopra, non riscritti: aggiungere un valore all'enum non deve lasciare
	 *  indietro un numero scritto a mano. `UE_ARRAY_COUNT` da' un `SIZE_T`, che `TestEqual` non sa
	 *  disambiguare fra i suoi overload — il cast e' qui una volta invece che a ogni chiamata. */
	constexpr int32 GlyphNumKinds = static_cast<int32>(UE_ARRAY_COUNT(GlyphAllKinds));
	constexpr int32 GlyphNumStates = static_cast<int32>(UE_ARRAY_COUNT(GlyphAllStates));

	const TCHAR* GlyphKindName(ERTHexTransitionKind K)
	{
		switch (K)
		{
		case ERTHexTransitionKind::Stair:    return TEXT("Stair");
		case ERTHexTransitionKind::Ramp:     return TEXT("Ramp");
		case ERTHexTransitionKind::Bridge:   return TEXT("Bridge");
		case ERTHexTransitionKind::Tunnel:   return TEXT("Tunnel");
		case ERTHexTransitionKind::Elevator: return TEXT("Elevator");
		case ERTHexTransitionKind::Jump:     return TEXT("Jump");
		default:                             return TEXT("?");
		}
	}

	FRTHexEdge GlyphEdge(ERTHexTransitionKind Kind, ERTHexArcState State)
	{
		FRTHexEdge E;
		E.From = FRTCellId(0, 0, 0);
		E.To = FRTCellId(0, 0, 1);
		E.Kind = Kind;
		E.State = State;
		return E;
	}

	/**
	 * Quanto due grigi devono distare per dirsi diversi, su `0`…`255`.
	 *
	 * ⚠️ **Il valore è una scelta dichiarata, non una costante universale.** `8/255` è circa il `3%`: sotto
	 * quella soglia due tratti sottili su un viewport d'editor non si separano. La soglia sta qui e non nel
	 * codice di produzione perché è la definizione dell'**oracolo del test**, non una regola di disegno.
	 */
	constexpr float GlyphGrayEpsilon = 8.0f;
}

/**
 * **AC 1** — 🔴 **il tipo si legge da DUE canali, e sono entrambi completi**, ed è il criterio 2 del DoD.
 *
 * Il difetto misurato: `TransitionKindColor` mappa sei `FColor` *«e nient'altro»*, mentre `D-146` scrive
 * *«l'encoding è ridondante: mai solo il colore»*.
 *
 * ⛔ **Il test conta gli usciti distinti, non li confronta con una tabella.** Ripetere qui i numeri di
 * `TicksFor` proverebbe solo che so copiare: ciò che deve valere è che siano **sei**, comunque li si
 * scelga.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlyphKindHasTwoChannelsTest,
	"RefactorTactics.Editor.TransitionGlyph.KindIsReadableOnTwoChannels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlyphKindHasTwoChannelsTest::RunTest(const FString&)
{
	TSet<int32> Ticks;
	TSet<uint32> Tints;

	for (const ERTHexTransitionKind K : GlyphAllKinds)
	{
		const RTHexTransition::FGlyph G = RTHexTransition::Describe(GlyphEdge(K, ERTHexArcState::Active));
		Ticks.Add(G.Ticks);
		Tints.Add(G.Tint.ToPackedRGBA());

		TestTrue(FString::Printf(TEXT("%s ha delle tacche, non zero"), GlyphKindName(K)), G.Ticks > 0);
	}

	TestEqual(TEXT("sei tipi, sei conteggi di tacche DISTINTI"), Ticks.Num(), GlyphNumKinds);
	TestEqual(TEXT("e sei tinte distinte: il primo canale resta intero"), Tints.Num(), GlyphNumKinds);

	return true;
}

/**
 * **AC 2** — 🔴 **due tipi restano distinguibili in SCALA DI GRIGI**, ed è la verifica che il DoD nomina.
 *
 * 🔑 **È qui che il secondo canale smette di essere decorativo.** Misurato: `Tunnel` (200,120,255) e
 * `Elevator` (255,120,120) hanno luminanza Rec.709 **146.8** e **148.7** — `1.9` su `255`. Senza le
 * tacche, quei due archi sono **lo stesso arco** per chiunque non distingua il viola dal rosso, e per
 * qualunque cattura in bianco e nero.
 *
 * ⚠️ **L'invariante è «distinguibili», non «esiste una coppia che collassa».** Se un domani le sei tinte
 * venissero riscelte e si separassero anche in grigio, quello sarebbe un **miglioramento**: un test che
 * asserisse il collasso lo leggerebbe come una rottura. Le coppie che oggi dipendono dalle tacche
 * vengono **registrate** con `AddInfo`, che è evidenza senza essere una regola.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlyphKindSurvivesGrayscaleTest,
	"RefactorTactics.Editor.TransitionGlyph.KindSurvivesGrayscale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlyphKindSurvivesGrayscaleTest::RunTest(const FString&)
{
	int32 SoloGrazieAlleTacche = 0;

	for (int32 I = 0; I < GlyphNumKinds; ++I)
	{
		for (int32 J = I + 1; J < GlyphNumKinds; ++J)
		{
			const RTHexTransition::FGlyph A =
				RTHexTransition::Describe(GlyphEdge(GlyphAllKinds[I], ERTHexArcState::Active));
			const RTHexTransition::FGlyph B =
				RTHexTransition::Describe(GlyphEdge(GlyphAllKinds[J], ERTHexArcState::Active));

			const float Delta = FMath::Abs(RTHexTransition::Luminance(A.Tint)
				- RTHexTransition::Luminance(B.Tint));
			const bool bGrigioSepara = Delta >= GlyphGrayEpsilon;
			const bool bTaccheSeparano = A.Ticks != B.Ticks;

			TestTrue(FString::Printf(
				TEXT("%s e %s si distinguono in scala di grigi (delta luminanza %.1f, tacche %d vs %d)"),
				GlyphKindName(GlyphAllKinds[I]), GlyphKindName(GlyphAllKinds[J]),
				Delta, A.Ticks, B.Ticks),
				bGrigioSepara || bTaccheSeparano);

			if (!bGrigioSepara)
			{
				++SoloGrazieAlleTacche;
				AddInfo(FString::Printf(
					TEXT("%s/%s collassano in grigio (delta %.1f < %.1f): li separano SOLO le tacche"),
					GlyphKindName(GlyphAllKinds[I]), GlyphKindName(GlyphAllKinds[J]),
					Delta, GlyphGrayEpsilon));
			}
		}
	}

	AddInfo(FString::Printf(TEXT("coppie che dipendono dal secondo canale: %d"), SoloGrazieAlleTacche));
	return true;
}

/**
 * **AC 3** — 🔴 **i tre stati si distinguono, e NON per opacità**, ed è il criterio 3 del DoD.
 *
 * ⛔ Il difetto che chiude: *«`Active`, `Inactive` e `Destroyed` producono la stessa freccia dello stesso
 * colore: un ponte abbattuto si disegna identico a uno percorribile»*.
 *
 * 🔑 **E `Inactive` non è un `Destroyed` più tenue.** I due sono indistinguibili per il grafo — nessuno dei
 * due si percorre — ma differiscono per la **reversibilità**: uno si riaccende, l'altro è terminale. È
 * l'unica differenza su cui chi costruisce può agire, e per questo ha un canale proprio invece di una
 * gradazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlyphStateIsVisibleTest,
	"RefactorTactics.Editor.TransitionGlyph.StateIsVisibleAndNotOnlyOpacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlyphStateIsVisibleTest::RunTest(const FString&)
{
	TSet<TPair<uint8, bool>> Combinazioni;

	for (const ERTHexArcState S : GlyphAllStates)
	{
		const RTHexTransition::FGlyph G =
			RTHexTransition::Describe(GlyphEdge(ERTHexTransitionKind::Bridge, S));
		Combinazioni.Add(TPair<uint8, bool>(static_cast<uint8>(G.Stroke), G.bCrossed));
	}

	TestEqual(TEXT("tre stati, tre rese DISTINTE"), Combinazioni.Num(), GlyphNumStates);

	const RTHexTransition::FGlyph Attivo =
		RTHexTransition::Describe(GlyphEdge(ERTHexTransitionKind::Bridge, ERTHexArcState::Active));
	const RTHexTransition::FGlyph Spento =
		RTHexTransition::Describe(GlyphEdge(ERTHexTransitionKind::Bridge, ERTHexArcState::Inactive));
	const RTHexTransition::FGlyph Abbattuto =
		RTHexTransition::Describe(GlyphEdge(ERTHexTransitionKind::Bridge, ERTHexArcState::Destroyed));

	// DoD 4: un arco abbattuto non si disegna come percorribile. È l'asserzione che il criterio nomina.
	TestTrue(TEXT("solo l'arco ATTIVO ha il tratto continuo"),
		Attivo.Stroke == RTHexTransition::EStroke::Solid
		&& Spento.Stroke == RTHexTransition::EStroke::Dashed
		&& Abbattuto.Stroke == RTHexTransition::EStroke::Dashed);

	TestTrue(TEXT("e SOLO l'abbattuto e' barrato: e' cio' che lo separa dallo spento"),
		Abbattuto.bCrossed && !Spento.bCrossed && !Attivo.bCrossed);

	// ⛔ Il controllo che il DoD chiede in negativo: la distinzione non passa da un'opacità. Non esiste un
	// campo da cui potrebbe passare, e questo asserto lo pinna — se qualcuno ne aggiungesse uno e ci
	// spostasse sopra la differenza, `Stroke` e `bCrossed` tornerebbero uguali fra Inactive e Destroyed.
	TestTrue(TEXT("spento e abbattuto NON differiscono solo per il tratto"),
		Spento.Stroke == Abbattuto.Stroke && Spento.bCrossed != Abbattuto.bCrossed);

	return true;
}

/**
 * **AC 4** — gli estremi si **copiano**, e non si scambiano.
 *
 * Il verso di un arco è un dato: `From` e `To` non sono intercambiabili, e una resa che li invertisse
 * racconterebbe una scala che sale al contrario. Nessun altro test di questo file guarderebbe lo scambio,
 * perché tinta, tacche, tratto e barra sono tutti simmetrici.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlyphKeepsEndpointsTest,
	"RefactorTactics.Editor.TransitionGlyph.EndpointsAreCopiedNotSwapped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlyphKeepsEndpointsTest::RunTest(const FString&)
{
	FRTHexEdge E = GlyphEdge(ERTHexTransitionKind::Stair, ERTHexArcState::Active);
	E.From = FRTCellId(3, -1, 0);
	E.To = FRTCellId(3, -1, 1);

	const RTHexTransition::FGlyph G = RTHexTransition::Describe(E);
	TestTrue(TEXT("`From` e' quello del dato"), G.From == E.From);
	TestTrue(TEXT("`To` e' quello del dato"), G.To == E.To);
	TestTrue(TEXT("e i due non sono stati scambiati"), G.From != G.To);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
