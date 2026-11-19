// La scarica a catena di LinearDischarge e l'ordine dei colpi lungo la linea ([D-621], #4136).
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/Format/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/Playback/RTPlaybackLibrary.h"
#include "Turn/Playback/RTPresentationBinding.h"
#include "Turn/Resolution/RTResolvedEvent.h"
#include "Perception/RTTeamKnowledge.h"
#include "Unit/RTUnit.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SplineComponent.h" // [D-621]: la scarica e' un attore con una spline, e i punti si leggono da li'
#include "Tests/Shared/RTWorldFixtures.h"
#include "Tests/Turn/Playback/Reveal/RTChainArcProbeForTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Nomi distinti per file: in unity build i test condividono la translation unit.

	/** Un evento di fase Blast della sorgente `Src`, noto a tutti. */
	FRTResolvedEvent CatenaEvento(ERTResolvedEventType Type, int32 Src, const TCHAR* Azione)
	{
		FRTResolvedEvent Ev;
		Ev.Phase = ERTMatchPhase::Blast;
		Ev.Type = Type;
		Ev.SourceStableUnitId = Src;
		Ev.ActionId = FName(Azione);
		Ev.SourceVerdict = FRTKnowledgeVerdict::Everyone();
		return Ev;
	}

	FRTResolvedEvent CatenaImpronta(int32 Src, const TCHAR* Azione, ERTAbilityShape Shape, const FRTCellId& Origine,
		const FRTCellId& Mira)
	{
		FRTResolvedEvent Ev = CatenaEvento(ERTResolvedEventType::AttackFootprint, Src, Azione);
		Ev.Shape = Shape;
		Ev.Origin = Origine;
		Ev.AimCell = Mira;
		return Ev;
	}

	/** Un colpo con la geometria risolta e i due estremi noti a tutti. */
	FRTResolvedEvent CatenaColpo(int32 Src, const TCHAR* Azione, int32 Bersaglio, const FRTCellId& Origine,
		const FRTCellId& Impatto)
	{
		FRTResolvedEvent Ev = CatenaEvento(ERTResolvedEventType::Attack, Src, Azione);
		Ev.Shape = ERTAbilityShape::Line;
		Ev.TargetStableUnitId = Bersaglio;
		Ev.HitGeometry.bResolved = true;
		Ev.HitGeometry.From = Origine;
		Ev.HitGeometry.Impact = Impatto;
		Ev.HitGeometry.FromVerdict = FRTKnowledgeVerdict::Everyone();
		Ev.HitGeometry.ImpactVerdict = FRTKnowledgeVerdict::Everyone();
		return Ev;
	}

	/**
	 * Una scarica da (0,0) verso Est con tre colpiti, emessi come li emette il resolver: per indice d'unita', non
	 * lungo la linea. Il 30 sta a 3 celle, il 10 a 1, il 20 a 2.
	 *   0 attivazione · 1 impronta · 2 colpo su 30 · 3 colpo su 10 · 4 colpo su 20
	 */
	TArray<FRTResolvedEvent> CatenaTimeline(ERTAbilityShape Shape = ERTAbilityShape::Line)
	{
		const FRTCellId O(0, 0);
		TArray<FRTResolvedEvent> T;
		T.Add(CatenaEvento(ERTResolvedEventType::AbilityActivated, 1, TEXT("L")));
		T.Add(CatenaImpronta(1, TEXT("L"), Shape, O, FRTCellId(4, 0)));
		T.Add(CatenaColpo(1, TEXT("L"), 30, O, FRTCellId(3, 0)));
		T.Add(CatenaColpo(1, TEXT("L"), 10, O, FRTCellId(1, 0)));
		T.Add(CatenaColpo(1, TEXT("L"), 20, O, FRTCellId(2, 0)));
		return T;
	}

	TArray<int32> CatenaIndici(const TArray<FRTBlastSequenceElement>& S)
	{
		TArray<int32> Out;
		for (const FRTBlastSequenceElement& E : S) { Out.Add(E.TimelineIndex); }
		return Out;
	}

	/** I salti validi della sequenza, come `<da>><a>`: lo stesso formato della traccia del playback. */
	TArray<FString> CatenaSalti(const TArray<FRTResolvedEvent>& T, const TArray<FRTBlastSequenceElement>& S, int32 Viewer)
	{
		TArray<FString> Out;
		for (int32 K = 0; K < S.Num(); ++K)
		{
			const FRTChainHop Hop = URTPlaybackLibrary::ChainHopFor(T, S, K, Viewer);
			if (Hop.bValid)
			{
				Out.Add(FString::Printf(TEXT("%d>%d"), Hop.FromTargetStableUnitId, Hop.ToTargetStableUnitId));
			}
		}
		return Out;
	}
}

/**
 * I colpi di un attacco a linea si mostrano dal piu' vicino al piu' lontano — [D-621] (4).
 *
 * 🔑 Il resolver li emette per `TargetId` (`Plan.Hits`), e la timeline qui fa lo stesso: 30, 10, 20.
 * ✅ Validato per mutazione: togliere la distanza dal confronto di `BuildBlastSequence` riporta `{0, 1, 2, 3, 4}`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackLineActHitsAlongTheLineTest,
	"RefactorTactics.Playback.LineActHitsAreShownAlongTheLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackLineActHitsAlongTheLineTest::RunTest(const FString&)
{
	const TArray<FRTResolvedEvent> T = CatenaTimeline();
	int32 Impronta = INDEX_NONE;
	if (!TestTrue(TEXT("⛔ premessa: l'atto e' a linea e noto a chi guarda"),
		URTPlaybackLibrary::IsLineActKnownTo(T, 1, FName(TEXT("L")), /*Viewer*/ 0, Impronta))) { return false; }
	TestEqual(TEXT("l'impronta trovata e' quella dell'atto"), Impronta, 1);

	const TArray<int32> Atteso = { 0, 1, 3, 4, 2 }; // attivazione, impronta, poi 10, 20, 30
	TestEqual(TEXT("🔴 i colpi vanno dal piu' vicino al piu' lontano"),
		CatenaIndici(URTPlaybackLibrary::BuildBlastSequence(T, {}, 0, /*Viewer*/ 0)), Atteso);
	return true;
}

/**
 * ⛔ Fail-closed: l'ordine lungo la linea non vale se chi guarda non conosce l'origine o un colpito, ne' se l'atto ha due
 * impronte. In ognuno dei tre casi resta l'ordine di timeline.
 * ✅ Validato per mutazione: togliere da `IsLineActKnownTo` il controllo dei colpiti fa cadere (a), e con lui
 * `Playback.ChainHopNeedsEveryEndKnownToTheViewer`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackLineActKeepsTimelineOrderTest,
	"RefactorTactics.Playback.LineActKeepsTimelineOrderWhenNotFullyKnown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackLineActKeepsTimelineOrderTest::RunTest(const FString&)
{
	// (a) un colpito che chi guarda non conosce
	{
		TArray<FRTResolvedEvent> T = CatenaTimeline();
		T[3].HitGeometry.ImpactVerdict = FRTKnowledgeVerdict::NoOne();
		int32 Impronta = INDEX_NONE;
		TestFalse(TEXT("(a) l'atto non e' noto per intero"),
			URTPlaybackLibrary::IsLineActKnownTo(T, 1, FName(TEXT("L")), 0, Impronta));
		const TArray<int32> Atteso = { 0, 1, 2, 3, 4 };
		TestEqual(TEXT("🔴 (a) ordine di timeline"), CatenaIndici(URTPlaybackLibrary::BuildBlastSequence(T, {}, 0, 0)), Atteso);
	}
	// (b) l'origine che chi guarda non conosce: l'impronta resta fuori dalla sequenza (#3551), i colpi in timeline
	{
		TArray<FRTResolvedEvent> T = CatenaTimeline();
		T[1].SourceVerdict = FRTKnowledgeVerdict::NoOne();
		const TArray<int32> Atteso = { 0, 2, 3, 4 };
		TestEqual(TEXT("🔴 (b) ordine di timeline, impronta esclusa"),
			CatenaIndici(URTPlaybackLibrary::BuildBlastSequence(T, {}, 0, 0)), Atteso);
	}
	// (c) due impronte nello stesso atto: la stessa azione due volte, e i colpi non dicono di quale linea siano
	{
		TArray<FRTResolvedEvent> T = CatenaTimeline();
		T.Insert(CatenaImpronta(1, TEXT("L"), ERTAbilityShape::Line, FRTCellId(0, 0), FRTCellId(0, 4)), 2);
		int32 Impronta = INDEX_NONE;
		TestFalse(TEXT("(c) due impronte: nessuna linea"),
			URTPlaybackLibrary::IsLineActKnownTo(T, 1, FName(TEXT("L")), 0, Impronta));
		const TArray<int32> Atteso = { 0, 1, 2, 3, 4, 5 };
		TestEqual(TEXT("🔴 (c) ordine di timeline"), CatenaIndici(URTPlaybackLibrary::BuildBlastSequence(T, {}, 0, 0)), Atteso);
	}
	return true;
}

/** Un atto non a linea non cambia ordine: il cono, a pari dati, resta in ordine di timeline. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackNonLineActKeepsTimelineOrderTest,
	"RefactorTactics.Playback.NonLineActKeepsTimelineOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackNonLineActKeepsTimelineOrderTest::RunTest(const FString&)
{
	const TArray<FRTResolvedEvent> T = CatenaTimeline(ERTAbilityShape::Cone);
	const TArray<int32> Atteso = { 0, 1, 2, 3, 4 };
	TestEqual(TEXT("🔴 il cono resta in ordine di timeline"),
		CatenaIndici(URTPlaybackLibrary::BuildBlastSequence(T, {}, 0, 0)), Atteso);
	TestEqual(TEXT("e non porta salti di scarica"), CatenaSalti(T, URTPlaybackLibrary::BuildBlastSequence(T, {}, 0, 0), 0).Num(), 0);
	return true;
}

/**
 * Il prefisso congelato di D-355 resta verbatim anche per un atto a linea interrotto a meta': `Build(T, S, k) == S` per
 * ogni `k`, perche' il riordino e' funzione della sola timeline e di chi guarda.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackLineActFrozenPrefixTest,
	"RefactorTactics.Playback.LineActOrderKeepsTheFrozenPrefixVerbatim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackLineActFrozenPrefixTest::RunTest(const FString&)
{
	const TArray<FRTResolvedEvent> T = CatenaTimeline();
	const TArray<FRTBlastSequenceElement> S = URTPlaybackLibrary::BuildBlastSequence(T, {}, 0, 0);
	if (!TestEqual(TEXT("⛔ premessa: la sequenza ha i cinque elementi"), S.Num(), 5)) { return false; }
	for (int32 k = 0; k <= S.Num(); ++k)
	{
		TestTrue(*FString::Printf(TEXT("🔴 Build(T, S, %d) == S"), k), URTPlaybackLibrary::BuildBlastSequence(T, S, k, 0) == S);
	}

	// 🔑 **Il caso di D-355, che a timeline fissa nessuna funzione deterministica sbaglia**: l'atto e' mostrato fino al
	// primo colpito (attivazione, impronta, 10) e la timeline CRESCE con un colpo dello stesso atto sul 40, a 4 celle. Il
	// prefisso resta verbatim, il resto segue la linea, e la catena riprende dal colpito gia' mostrato.
	TArray<FRTResolvedEvent> T2 = T;
	T2.Add(CatenaColpo(1, TEXT("L"), 40, FRTCellId(0, 0), FRTCellId(4, 0))); // 5
	const TArray<FRTBlastSequenceElement> S2 = URTPlaybackLibrary::BuildBlastSequence(T2, S, 3, 0);
	const TArray<int32> Atteso = { 0, 1, 3, 4, 2, 5 };
	TestEqual(TEXT("🔴 prefisso verbatim, poi 20, 30 e 40 lungo la linea"), CatenaIndici(S2), Atteso);
	const TArray<FString> Salti = { TEXT("0>10"), TEXT("10>20"), TEXT("20>30"), TEXT("30>40") };
	TestEqual(TEXT("🔴 la catena riprende dal colpito gia' mostrato"),
		FString::Join(CatenaSalti(T2, S2, 0), TEXT(",")), FString::Join(Salti, TEXT(",")));
	return true;
}

/**
 * La catena: sorgente → piu' vicino → successivo — [D-621] (3).
 *
 * 🔑 Il precedente si cerca nella SEQUENZA: con la sequenza in ordine di linea i salti sono 0>10, 10>20, 20>30.
 * ✅ Validato per mutazione: cercare il precedente nella timeline invece che nella sequenza da' 0>30, 30>10, 10>20.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackChainHopSourceThenVictimsTest,
	"RefactorTactics.Playback.ChainHopGoesFromTheSourceThenVictimToVictim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackChainHopSourceThenVictimsTest::RunTest(const FString&)
{
	const TArray<FRTResolvedEvent> T = CatenaTimeline();
	const TArray<FRTBlastSequenceElement> S = URTPlaybackLibrary::BuildBlastSequence(T, {}, 0, 0);
	const TArray<FString> Atteso = { TEXT("0>10"), TEXT("10>20"), TEXT("20>30") };
	TestEqual(TEXT("🔴 i salti, uno per colpo, lungo la linea"),
		FString::Join(CatenaSalti(T, S, 0), TEXT(",")), FString::Join(Atteso, TEXT(",")));

	// L'impronta di un atto CON colpi non porta l'arco a vuoto.
	TestFalse(TEXT("l'impronta di un atto con colpi non salta"), URTPlaybackLibrary::ChainHopFor(T, S, 1, 0).bValid);

	const FRTChainHop Ultimo = URTPlaybackLibrary::ChainHopFor(T, S, 4, 0);
	TestTrue(TEXT("l'ultimo salto arriva sulla cella del colpito"), Ultimo.ToCell == FRTCellId(3, 0));
	TestEqual(TEXT("e porta l'azione del colpo"), Ultimo.ActionId, FName(TEXT("L")));
	TestEqual(TEXT("e chi lancia"), Ultimo.SourceStableUnitId, 1);
	return true;
}

/** Senza colpiti, un solo arco dalla sorgente all'esagono mirato, al battito dell'impronta — [D-621] (3). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackChainHopMissTest,
	"RefactorTactics.Playback.ChainHopWithoutVictimsArcsToTheAimCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackChainHopMissTest::RunTest(const FString&)
{
	TArray<FRTResolvedEvent> T;
	T.Add(CatenaEvento(ERTResolvedEventType::AbilityActivated, 1, TEXT("L")));
	T.Add(CatenaImpronta(1, TEXT("L"), ERTAbilityShape::Line, FRTCellId(0, 0), FRTCellId(4, 0)));
	const TArray<FRTBlastSequenceElement> S = URTPlaybackLibrary::BuildBlastSequence(T, {}, 0, 0);
	if (!TestEqual(TEXT("⛔ premessa: attivazione e impronta in sequenza"), S.Num(), 2)) { return false; }

	const FRTChainHop Hop = URTPlaybackLibrary::ChainHopFor(T, S, 1, 0);
	if (!TestTrue(TEXT("🔴 l'impronta senza colpi porta l'arco a vuoto"), Hop.bValid)) { return false; }
	TestEqual(TEXT("parte dalla sorgente"), Hop.FromTargetStableUnitId, 0);
	TestEqual(TEXT("non arriva su un colpito"), Hop.ToTargetStableUnitId, 0);
	TestTrue(TEXT("arriva sull'esagono mirato"), Hop.ToCell == FRTCellId(4, 0));
	TestFalse(TEXT("l'attivazione non salta"), URTPlaybackLibrary::ChainHopFor(T, S, 0, 0).bValid);
	return true;
}

/** ⛔ Nessun salto quando chi guarda non conosce un estremo: la catena e' tutto o niente. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackChainHopNeedsKnownEndsTest,
	"RefactorTactics.Playback.ChainHopNeedsEveryEndKnownToTheViewer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackChainHopNeedsKnownEndsTest::RunTest(const FString&)
{
	TArray<FRTResolvedEvent> T = CatenaTimeline();
	TestEqual(TEXT("premessa: con tutto noto i salti ci sono"),
		CatenaSalti(T, URTPlaybackLibrary::BuildBlastSequence(T, {}, 0, 0), 0).Num(), 3);

	T[4].HitGeometry.ImpactVerdict = FRTKnowledgeVerdict::NoOne();
	T[4].HitGeometry.ImpactVerdict.AllowTeam(1); // il colpito 20 lo conosce solo la squadra 1
	TestEqual(TEXT("🔴 la squadra 0 non conosce il 20: nessun salto"),
		CatenaSalti(T, URTPlaybackLibrary::BuildBlastSequence(T, {}, 0, 0), 0).Num(), 0);
	TestEqual(TEXT("la squadra 1 conosce tutto: tre salti"),
		CatenaSalti(T, URTPlaybackLibrary::BuildBlastSequence(T, {}, 0, 1), 1).Num(), 3);
	return true;
}

/**
 * La tabella delle scariche: una riga, LinearDischarge, su un'azione VERA di forma `Line`.
 *
 * 🔑 L'id si legge dal catalogo, non si confronta solo col testo della riga: il rename di #4011 ha mostrato che una riga
 * con l'id vecchio resta verde ovunque e non lancia piu' niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPresentationChainFxDeclaredTest,
	"RefactorTactics.Presentation.ChainFxIsDeclaredOnlyForLinearDischarge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPresentationChainFxDeclaredTest::RunTest(const FString&)
{
	const TArray<TPair<FName, FRTChainFxBinding>>& Righe = URTPresentationBindingLibrary::DeclaredChainFxRows();
	if (!TestEqual(TEXT("una riga"), Righe.Num(), 1)) { return false; }

	const URTHeroData* Gadget = URTHeroCatalogLibrary::MakeGadget();
	const URTActionData* Azione = nullptr;
	for (int32 i = 0; Gadget && i < Gadget->Actions.Num(); ++i)
	{
		if (Gadget->Actions[i] && Gadget->Actions[i]->Def.ActionId == Righe[0].Key) { Azione = Gadget->Actions[i]; }
	}
	if (!TestNotNull(*FString::Printf(TEXT("🔴 la riga '%s' e' un'azione del kit di Gadget"), *Righe[0].Key.ToString()), Azione))
	{
		return false;
	}
	TestTrue(TEXT("ed e' a linea: la catena segue la linea"), Azione->Shape == ERTAbilityShape::Line);
	TestEqual(TEXT("la classe e' la copia del Blueprint del pack"), Righe[0].Value.ArcClass.ToString(),
		FString(TEXT("/Game/RT/Characters/Gadget/FX/BP_Gadget_LinearDischarge_Arc.BP_Gadget_LinearDischarge_Arc_C")));
	TestEqual(TEXT("il primo salto parte dalla sfera"), Righe[0].Value.SourceSocket, FName(TEXT("canister_main")));
	TestTrue(TEXT("l'attore ha una vita"), Righe[0].Value.LifeSeconds > 0.f);

	TestNotNull(TEXT("ChainFxFor trova la riga dall'ActionId"), URTPresentationBindingLibrary::ChainFxFor(Righe[0].Key, NAME_None));
	TestNotNull(TEXT("e dal BaseActionId"), URTPresentationBindingLibrary::ChainFxFor(NAME_None, Righe[0].Key));
	TestNull(TEXT("un'azione senza riga non ha scarica"),
		URTPresentationBindingLibrary::ChainFxFor(FName(TEXT("Hero.Muiren.PressureJet")), NAME_None));
	return true;
}

/**
 * Il punto di mezzo della scarica sta SOPRA il tratto, di una frazione fissa della sua lunghezza — [D-621].
 *
 * 🔑 Una spline a due punti e' una retta, e l'arco non si vedrebbe: il terzo punto la piega. Alzarlo in proporzione alla
 * lunghezza fa si' che un salto corto e uno lungo abbiano la stessa curvatura, invece di un salto lungo piatto.
 * ⚠️ `Start == End` e' il caso limite: la lunghezza e' zero, il punto di mezzo e' `Start`, e nessuna divisione la
 * normalizza — un NaN qui sposterebbe la spline, e con lei l'effetto, dove nessuno lo cerca.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackChainArcMidpointTest,
	"RefactorTactics.Playback.ChainArcMidpointLiftsTheMiddle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackChainArcMidpointTest::RunTest(const FString&)
{
	const float Tol = UE_KINDA_SMALL_NUMBER;
	TestEqual(TEXT("🔴 un tratto di 1000 si alza di 200 al centro"),
		URTPlaybackLibrary::ChainArcMidpoint(FVector(0, 0, 0), FVector(1000, 0, 0)), FVector(500, 0, 200), Tol);
	TestEqual(TEXT("🔴 l'alzata si somma alla quota del tratto, non la sostituisce: 100 + 120"),
		URTPlaybackLibrary::ChainArcMidpoint(FVector(0, 0, 100), FVector(0, 600, 100)), FVector(0, 300, 220), Tol);

	const FVector Fermo(120, -40, 75);
	const FVector Medio = URTPlaybackLibrary::ChainArcMidpoint(Fermo, Fermo);
	TestFalse(TEXT("⛔ con Start == End nessuna componente e' NaN"), Medio.ContainsNaN());
	TestEqual(TEXT("e il centro e' proprio Start"), Medio, Fermo, Tol);
	return true;
}

/**
 * I tre punti della scarica vanno sulla spline dell'attore, in spazio mondo, e la riscrittura SOSTITUISCE — [D-621].
 *
 * 🔑 Il data interface `Spline` del Niagara rilegge i punti al cambio di versione: la spline va riscritta, non
 * allungata. Un'`AddSplinePoint` al posto di `SetSplinePoints` accoderebbe tre punti a ogni chiamata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackChainArcPointsTest,
	"RefactorTactics.Playback.ChainArcPointsGoOnTheSpline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackChainArcPointsTest::RunTest(const FString&)
{
	USplineComponent* Spline = NewObject<USplineComponent>(GetTransientPackage());
	if (!TestNotNull(TEXT("⛔ premessa: la spline di prova"), Spline)) { return false; }

	const float Tol = UE_KINDA_SMALL_NUMBER;
	const FVector Partenza(0, 0, 0);
	const FVector Arrivo(1000, 0, 0);
	URTPlaybackLibrary::SetChainArcPoints(*Spline, Partenza, Arrivo);
	if (!TestEqual(TEXT("🔴 tre punti: partenza, mezzo, arrivo"), Spline->GetNumberOfSplinePoints(), 3)) { return false; }
	TestEqual(TEXT("il primo punto e' la partenza"),
		Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World), Partenza, Tol);
	TestEqual(TEXT("il secondo e' il punto di mezzo, alzato"),
		Spline->GetLocationAtSplinePoint(1, ESplineCoordinateSpace::World),
		URTPlaybackLibrary::ChainArcMidpoint(Partenza, Arrivo), Tol);
	TestEqual(TEXT("il terzo e' l'arrivo"),
		Spline->GetLocationAtSplinePoint(2, ESplineCoordinateSpace::World), Arrivo, Tol);

	// Una seconda scrittura con altri estremi: i punti restano tre, e sono quelli nuovi.
	const FVector Partenza2(-300, 200, 50);
	const FVector Arrivo2(100, 800, 50);
	URTPlaybackLibrary::SetChainArcPoints(*Spline, Partenza2, Arrivo2);
	if (!TestEqual(TEXT("🔴 riscrivere sostituisce, non accoda: ancora tre punti"),
		Spline->GetNumberOfSplinePoints(), 3)) { return false; }
	TestEqual(TEXT("la nuova partenza"),
		Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World), Partenza2, Tol);
	TestEqual(TEXT("il nuovo mezzo"),
		Spline->GetLocationAtSplinePoint(1, ESplineCoordinateSpace::World),
		URTPlaybackLibrary::ChainArcMidpoint(Partenza2, Arrivo2), Tol);
	TestEqual(TEXT("il nuovo arrivo"),
		Spline->GetLocationAtSplinePoint(2, ESplineCoordinateSpace::World), Arrivo2, Tol);
	return true;
}

namespace
{
	/** Gli attori vivi del mondo: validi e non in distruzione. Un attore appena distrutto non conta. */
	int32 ContaAttoriViviCatena(UWorld* World)
	{
		int32 N = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (IsValid(*It) && !It->IsActorBeingDestroyed())
			{
				++N;
			}
		}
		return N;
	}
}

/**
 * ⛔ Un asset senza spline non lascia un attore a mezz'aria: nasce, si vede che non serve, e si distrugge — [D-621].
 *
 * 🔑 Il contratto con l'asset e' una `USplineComponent` sull'attore. Un Blueprint copiato male senza spline
 * non deve restare vivo per `LifeSeconds` nel mondo, fermo e invisibile, a ogni salto della catena.
 * ✅ Validato per mutazione: togliere `Destroy()` da `SpawnChainArcActor` lascia un attore in piu' nel mondo, e cade
 * l'asserto sul conteggio — non quello sulla nascita, che fa da controllo positivo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackChainArcActorNeedsASplineTest,
	"RefactorTactics.Playback.ChainArcActorNeedsASpline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackChainArcActorNeedsASplineTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	// 🔑 Controllo positivo: senza sapere che l'attore e' NATO, «non resta vivo» sarebbe vero anche se lo spawn fallisse.
	int32 Nati = 0;
	const FDelegateHandle Ascolto = World->AddOnActorSpawnedHandler(
		FOnActorSpawned::FDelegate::CreateLambda([&Nati](AActor*) { ++Nati; }));

	const int32 Prima = ContaAttoriViviCatena(World);
	AActor* const Arco = URTPlaybackLibrary::SpawnChainArcActor(*World, *AActor::StaticClass(),
		FVector(0, 0, 0), FVector(1000, 0, 0), 1.5f);
	World->RemoveOnActorSpawnedHandler(Ascolto);

	TestEqual(TEXT("⛔ premessa: l'attore e' stato creato"), Nati, 1);
	TestNull(TEXT("🔴 un attore senza spline non restituisce niente"), Arco);
	TestEqual(TEXT("🔴 e non resta vivo: gli attori sono quelli di prima"), ContaAttoriViviCatena(World), Prima);
	return true;
}

/**
 * Il ramo felice: un attore CON spline nasce in `Start`, riceve i tre punti IN SPAZIO MONDO e una vita — [D-621].
 *
 * 🔴 **Il contratto "spazio mondo" si misura qui, e non in `ChainArcPointsGoOnTheSpline`.** Quella prova lavora su una
 * componente non registrata, con la trasformata identita': li' `Local` e `World` coincidono, e scambiarli non farebbe
 * cadere niente. Qui l'attore nasce lontano dall'origine e la spline e' la sua radice: con punti locali l'intero arco
 * slitterebbe di `Start` rispetto al bersaglio.
 * ✅ Validato per mutazione: `ESplineCoordinateSpace::World` -> `Local` in `SetChainArcPoints` fa cadere i tre punti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackChainArcActorWritesWorldPointsTest,
	"RefactorTactics.Playback.ChainArcActorWritesTheWorldPoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackChainArcActorWritesWorldPointsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	const float Tol = UE_KINDA_SMALL_NUMBER;
	const FVector Partenza(1000, 500, 100); // lontano dall'origine: e' cio' che separa `World` da `Local`
	const FVector Arrivo(1600, 500, 100);
	const float Vita = 2.25f;               // diversa dal default di `FRTChainFxBinding`: un default non la indovina per caso

	AActor* const Arco = URTPlaybackLibrary::SpawnChainArcActor(*World, *ARTChainArcProbeForTest::StaticClass(),
		Partenza, Arrivo, Vita);
	if (!TestNotNull(TEXT("🔴 un attore con spline torna dalla funzione"), Arco)) { return false; }
	ON_SCOPE_EXIT{ if (IsValid(Arco)) { Arco->Destroy(); } };

	TestEqual(TEXT("⛔ premessa: l'attore e' nato in Start, e cosi' la sua spline"), Arco->GetActorLocation(), Partenza, Tol);
	const USplineComponent* const Spline = Arco->FindComponentByClass<USplineComponent>();
	if (!TestNotNull(TEXT("⛔ premessa: l'attore ha la sua spline"), Spline)) { return false; }
	if (!TestEqual(TEXT("🔴 tre punti sulla spline"), Spline->GetNumberOfSplinePoints(), 3)) { return false; }

	TestEqual(TEXT("🔴 il primo punto e' Start, in spazio mondo"),
		Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World), Partenza, Tol);
	TestEqual(TEXT("🔴 il secondo e' il punto di mezzo alzato, in spazio mondo"),
		Spline->GetLocationAtSplinePoint(1, ESplineCoordinateSpace::World),
		URTPlaybackLibrary::ChainArcMidpoint(Partenza, Arrivo), Tol);
	TestEqual(TEXT("🔴 il terzo e' End, in spazio mondo"),
		Spline->GetLocationAtSplinePoint(2, ESplineCoordinateSpace::World), Arrivo, Tol);

	TestTrue(TEXT("🔴 l'attore ha la vita chiesta"), FMath::IsNearlyEqual(Arco->GetLifeSpan(), Vita, 0.01f));
	TestTrue(TEXT("e non e' in distruzione"), !Arco->IsActorBeingDestroyed());
	return true;
}

/**
 * ⛔ L'arco non si replica, anche se la sua classe replica — [D-621], #4136.
 *
 * 🔴 La classe e' un Blueprint di terzi, fuori da git: su un listen server un arco replicato arriverebbe ai client, e sarebbe
 * un arco verso un colpito che chi guarda non conosce — proprio cio' che `ChainHopFor` esiste per non disegnare. Qui la
 * sonda replica nel CDO, e `SpawnChainArcActor` deve spegnerlo comunque.
 * 🔑 La premessa sul CDO e' il controllo positivo: senza, «non si replica» sarebbe vero anche per una sonda che non replicava mai.
 * ✅ Validato per mutazione: togliere `SetReplicates(false)` da `SpawnChainArcActor` fa cadere l'asserto sull'attore, non la premessa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackChainArcActorStaysLocalTest,
	"RefactorTactics.Playback.ChainArcActorDoesNotReplicate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackChainArcActorStaysLocalTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	if (!TestTrue(TEXT("⛔ premessa: la sonda replica di suo, nel CDO"),
		GetDefault<ARTChainArcReplicatingProbeForTest>()->GetIsReplicated())) { return false; }

	AActor* const Arco = URTPlaybackLibrary::SpawnChainArcActor(*World, *ARTChainArcReplicatingProbeForTest::StaticClass(),
		FVector(0, 0, 0), FVector(1000, 0, 0), 1.5f);
	if (!TestNotNull(TEXT("⛔ premessa: l'attore e' nato"), Arco)) { return false; }
	ON_SCOPE_EXIT{ if (IsValid(Arco)) { Arco->Destroy(); } };

	TestFalse(TEXT("🔴 l'arco non si replica"), Arco->GetIsReplicated());
	TestTrue(TEXT("e non ha un lato remoto"), Arco->GetRemoteRole() == ROLE_None);
	return true;
}

/**
 * ⛔ Un arco ha sempre una vita: `LifeSeconds` non positivo vale il default della riga, non «per sempre» — [D-621], #4136.
 *
 * 🔴 `SetLifeSpan(0)` toglie il timer di distruzione: l'attore resterebbe nel mondo per sempre. `SpawnChainArcActor` e'
 * pubblica, e un chiamante che passi 0 (o un valore negativo) non deve lasciare un arco immortale.
 * 🔑 Il controllo positivo e' `ChainArcActorWritesTheWorldPoints`, che con 2,25 misura la vita chiesta: qui conta che lo 0 e il
 * negativo NON arrivino al timer, e valgano `FRTChainFxBinding().LifeSeconds`.
 * ✅ Validato per mutazione: passare `LifeSeconds` cosi' com'e' a `SetLifeSpan` lascia `GetLifeSpan()` a zero e cade l'asserto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackChainArcActorNeverLivesForeverTest,
	"RefactorTactics.Playback.ChainArcActorNeverLivesForever",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackChainArcActorNeverLivesForeverTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	const float VitaDellaRiga = FRTChainFxBinding().LifeSeconds;
	if (!TestTrue(TEXT("⛔ premessa: il default della riga e' una vita positiva"), VitaDellaRiga > 0.f)) { return false; }

	for (const float Vita : { 0.f, -3.f })
	{
		AActor* const Arco = URTPlaybackLibrary::SpawnChainArcActor(*World, *ARTChainArcProbeForTest::StaticClass(),
			FVector(0, 0, 0), FVector(1000, 0, 0), Vita);
		if (!TestNotNull(*FString::Printf(TEXT("⛔ premessa (%.1f): l'attore e' nato"), Vita), Arco)) { continue; }
		ON_SCOPE_EXIT{ if (IsValid(Arco)) { Arco->Destroy(); } };

		TestTrue(*FString::Printf(TEXT("🔴 con LifeSeconds %.1f l'attore ha comunque una vita positiva (e' %.2f)"), Vita, Arco->GetLifeSpan()),
			Arco->GetLifeSpan() > 0.f);
		TestTrue(*FString::Printf(TEXT("🔴 con LifeSeconds %.1f la vita e' quella della riga (%.2f, e' %.2f)"), Vita, VitaDellaRiga, Arco->GetLifeSpan()),
			FMath::IsNearlyEqual(Arco->GetLifeSpan(), VitaDellaRiga, 0.01f));
	}
	return true;
}

namespace
{
	ARTUnit* SpawnCatenaUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	int32 IndiceCatenaAzione(const ARTUnit* U, const TCHAR* ActionId)
	{
		for (int32 i = 0; U && i < U->NumAbilities(); ++i)
		{
			const URTActionData* A = U->GetAbility(i);
			if (A && A->Def.ActionId == FName(ActionId)) { return i; }
		}
		return INDEX_NONE;
	}
}

/**
 * Nel playback vero: Gadget scarica verso OVEST su tre nemici in fila.
 *
 * 🔑 **Verso Ovest, e non verso Est, e' la meta' del montaggio.** Il roster si ordina per squadra e poi per cella
 * (`MatchRosterLess`), quindi l'indice d'unita' — e con lui `Plan.Hits` e la timeline — cresce con `X`: verso Est i
 * colpiti escono gia' dal piu' vicino, verso Ovest dal piu' lontano. Misurato: la prima stesura sparava a Est, e la
 * premessa qui sotto e' caduta.
 * 🔑 La traccia dei salti si scrive PRIMA di caricare l'asset: misura la decisione anche su un clone senza `Content/`.
 * ✅ Validato per mutazione: togliere la chiamata a `SpawnChainArc` dal ramo `Attack` del playback svuota la traccia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackChainArcHopsFollowTheLineTest,
	"RefactorTactics.Playback.ChainArcHopsFollowTheLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackChainArcHopsFollowTheLineTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 8);
	ARTUnit* Lontano = SpawnCatenaUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, 2));
	ARTUnit* Mezzo   = SpawnCatenaUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(3, 2));
	ARTUnit* Vicino  = SpawnCatenaUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(4, 2));
	ARTUnit* Gadget  = SpawnCatenaUnit(World, 0, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(5, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Lontano || !Vicino || !Mezzo || !Gadget) { return false; }
	Gadget->Facing = ERTHexDirection::W;
	Gadget->SetPlannedMainAction(IndiceCatenaAzione(Gadget, TEXT("Hero.Gadget.LinearDischarge")));
	Gadget->DeclareAttackOnUnit(Lontano);
	Gadget->SetPlannedCell(Gadget->Cell);
	if (!TestTrue(TEXT("⛔ premessa: Gadget ha LinearDischarge pianificata"),
		Gadget->GetPlan().PlannedAbilityIndex() != INDEX_NONE)) { return false; }
	TM->bRecordAttackBeatsForTest = true;

	TM->LockInAndResolve();

	// ⛔ Premessa: la timeline NON mette gia' i colpi lungo la linea, altrimenti il test non distinguerebbe il riordino.
	TArray<int32> ColpitiInTimeline;
	for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
	{
		if (Ev.Type == ERTResolvedEventType::Attack && Ev.SourceStableUnitId == Gadget->StableUnitId)
		{
			ColpitiInTimeline.Add(Ev.TargetStableUnitId);
		}
	}
	const TArray<int32> LungoLaLinea = { Vicino->StableUnitId, Mezzo->StableUnitId, Lontano->StableUnitId };
	if (!TestEqual(TEXT("⛔ premessa: tre colpiti"), ColpitiInTimeline.Num(), 3)) { return false; }
	if (!TestTrue(TEXT("⛔ premessa: in timeline i colpiti non sono gia' in ordine di linea"), ColpitiInTimeline != LungoLaLinea))
	{
		return false;
	}

	// La traccia si azzera a ogni fase: si tiene l'ultima vista nel Blast.
	TArray<FString> Salti;
	for (int32 I = 0; I < 2000 && TM->IsResolving(); ++I)
	{
		TM->Tick(0.02f);
		if (TM->CurrentPlaybackPhaseForTest() == ERTMatchPhase::Blast)
		{
			Salti = TM->ChainHopTraceForTest();
		}
	}

	const TArray<FString> Atteso = {
		FString::Printf(TEXT("0>%d"), Vicino->StableUnitId),
		FString::Printf(TEXT("%d>%d"), Vicino->StableUnitId, Mezzo->StableUnitId),
		FString::Printf(TEXT("%d>%d"), Mezzo->StableUnitId, Lontano->StableUnitId) };
	TestEqual(TEXT("🔴 la scarica salta dalla sfera al piu' vicino, poi lungo la linea"),
		FString::Join(Salti, TEXT(",")), FString::Join(Atteso, TEXT(",")));
	return true;
}

namespace
{
	/** La traccia dei salti del Blast, come la lascia l'ultimo tick nella fase: si azzera a ogni fase, quindi si tiene l'ultima vista. */
	TArray<FString> CatenaTracciaDelBlast(ARTTurnManager* TM, bool* OutLanciato = nullptr)
	{
		TArray<FString> Salti;
		for (int32 I = 0; I < 2000 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.02f);
			if (OutLanciato) { *OutLanciato |= TM->AttackBeatTraceForTest().Contains(TEXT("L0")); }
			if (TM->CurrentPlaybackPhaseForTest() == ERTMatchPhase::Blast)
			{
				Salti = TM->ChainHopTraceForTest();
			}
		}
		return Salti;
	}

	/** Gli eventi di fase Blast di `Tipo` emessi da `Sorgente`. */
	int32 CatenaContaEventi(const TArray<FRTResolvedEvent>& Timeline, ERTResolvedEventType Tipo, int32 Sorgente)
	{
		int32 N = 0;
		for (const FRTResolvedEvent& Ev : Timeline)
		{
			N += (Ev.Type == Tipo && Ev.SourceStableUnitId == Sorgente) ? 1 : 0;
		}
		return N;
	}

	/**
	 * Gadget in (5,2) a Ovest, squadra 0 — quella di chi guarda in un mondo senza player controller — mira una cella VUOTA a
	 * portata, (2,2). Un nemico sta fuori dalla linea: tiene viva la seconda squadra, e la scarica non lo tocca.
	 */
	ARTTurnManager* SetUpCatenaAVuoto(UWorld* World, ARTUnit*& OutGadget)
	{
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 8);
		OutGadget = SpawnCatenaUnit(World, 0, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(5, 2));
		ARTUnit* const Fuori = SpawnCatenaUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(3, 5));
		ARTTurnManager* const TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !OutGadget || !Fuori) { return nullptr; }
		OutGadget->Facing = ERTHexDirection::W;
		OutGadget->SetPlannedMainAction(IndiceCatenaAzione(OutGadget, TEXT("Hero.Gadget.LinearDischarge")));
		OutGadget->DeclareAttackOnCell(FRTCellId(2, 2));
		OutGadget->SetPlannedCell(OutGadget->Cell);
		TM->bRecordAttackBeatsForTest = true;
		return TM;
	}

	/**
	 * Gadget in (7,0) a Ovest scarica su tre Ivrin in fila, in (4,0), (3,0) e (2,0), che guardano a Ovest: la sfera sta alle loro
	 * spalle, a 3 celle dal piu' vicino, fuori dalla consapevolezza ravvicinata e dall'arco frontale. `TeamGadget` decide chi guarda:
	 * lo spettatore di un mondo di prova e' la squadra 0, quindi con `0` guarda chi spara, con `1` guarda chi e' colpito.
	 */
	ARTTurnManager* SetUpCatenaAlleSpalle(UWorld* World, int32 TeamGadget, ARTUnit*& OutGadget)
	{
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 8);
		const int32 TeamColpiti = 1 - TeamGadget;
		ARTUnit* const Lontano = SpawnCatenaUnit(World, TeamColpiti, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, 0));
		ARTUnit* const Mezzo   = SpawnCatenaUnit(World, TeamColpiti, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(3, 0));
		ARTUnit* const Vicino  = SpawnCatenaUnit(World, TeamColpiti, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(4, 0));
		OutGadget = SpawnCatenaUnit(World, TeamGadget, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(7, 0));
		ARTTurnManager* const TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Lontano || !Mezzo || !Vicino || !OutGadget) { return nullptr; }
		for (ARTUnit* const Colpito : { Lontano, Mezzo, Vicino }) { Colpito->Facing = ERTHexDirection::W; }
		OutGadget->Facing = ERTHexDirection::W;
		OutGadget->SetPlannedMainAction(IndiceCatenaAzione(OutGadget, TEXT("Hero.Gadget.LinearDischarge")));
		OutGadget->DeclareAttackOnUnit(Lontano);
		OutGadget->SetPlannedCell(OutGadget->Cell);
		TM->bRecordAttackBeatsForTest = true;
		return TM;
	}
}

/**
 * Nel playback vero: una scarica che non colpisce nessuno apre UN arco, dalla sfera alla cella mirata — [D-621] (3), #4136.
 *
 * 🔴 `ChainArcHopsFollowTheLine` esercita solo il ramo `Attack` del director. L'arco a vuoto passa da un'altra chiamata, nel ramo
 * `AttackFootprint`, e fino a qui toglierla non faceva cadere niente: la funzione pura `ChainHopFor` era provata, il suo uso no.
 * La traccia `0>0` dice «dalla sorgente, verso nessun colpito»: `0` non e' mai un'unita'.
 * 🔑 Come `ChainArcHopsFollowTheLine`, la traccia si scrive PRIMA di caricare l'asset: misura la decisione anche su un clone senza
 * `Content/`. La premessa sulla timeline e' il controllo positivo: un'impronta SENZA colpi, altrimenti `0>0` non direbbe l'arco a vuoto.
 * ✅ Validato per mutazione: togliere `SpawnChainArc(Elemento)` dal ramo `AttackFootprint` di `AdvanceBlastSequence` svuota la traccia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackChainArcMissOpensOneHopTest,
	"RefactorTactics.Playback.ChainArcMissOpensOneHopToTheAimCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackChainArcMissOpensOneHopTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTUnit* Gadget = nullptr;
	ARTTurnManager* const TM = SetUpCatenaAVuoto(World, Gadget);
	if (!TestTrue(TEXT("⛔ premessa: il turno si costruisce"), TM != nullptr && Gadget != nullptr)) { return false; }
	if (!TestTrue(TEXT("⛔ premessa: Gadget ha LinearDischarge pianificata"), Gadget->GetPlan().PlannedAbilityIndex() != INDEX_NONE)) { return false; }

	TM->LockInAndResolve();

	// ⛔ Premessa: l'atto c'e' (un'impronta) e non ha colpito nessuno. Senza, `0>0` non direbbe l'arco a vuoto.
	const TArray<FRTResolvedEvent>& Timeline = TM->ResolvedTimelineForTest();
	if (!TestEqual(TEXT("⛔ premessa: una sola impronta della scarica"),
		CatenaContaEventi(Timeline, ERTResolvedEventType::AttackFootprint, Gadget->StableUnitId), 1)) { return false; }
	if (!TestEqual(TEXT("⛔ premessa: nessun colpito"),
		CatenaContaEventi(Timeline, ERTResolvedEventType::Attack, Gadget->StableUnitId), 0)) { return false; }

	const TArray<FString> Salti = CatenaTracciaDelBlast(TM);
	TestEqual(TEXT("🔴 un solo arco, dalla sfera (0) alla cella mirata, senza colpito"),
		FString::Join(Salti, TEXT(",")), FString(TEXT("0>0")));
	return true;
}

/**
 * Nel playback vero: chi guarda non vede la linea, e la scarica non salta — [D-621] (3), #4136.
 *
 * ⛔ Fail-closed passando dal director: lo stesso tiro con tre colpiti, guardato dalla squadra colpita, che non conosce l'origine
 * del colpo (la sfera sta alle sue spalle, a 3 celle). `ChainHopFor` e' gia' provata da sola; qui si misura che il director le dia
 * la squadra di chi guarda (`PlaybackViewerTeamId`) e non quella di chi lancia: un arco sulla linea direbbe dove sta il tiratore.
 * 🔴 **Il controllo positivo regge il test**: lo stesso tiro, con le squadre nell'altro verso, deve dare i tre salti — senza, una
 * traccia vuota sarebbe vera anche per una scarica mai lanciata. La premessa `L0` dice che il colpo e' partito, quindi `SpawnChainArc`
 * e' stata chiesta e non ha prodotto niente.
 * ⚠️ Lo spettatore del mondo di prova e' la squadra 0 (`TeamIdOf(nullptr)`): sono le squadre a scambiarsi, come in
 * `Privacy.UnseenAttackerTracerIsNotDelivered`. Qui l'estremo che chi guarda non conosce e' l'ORIGINE (la sfera), non un colpito: i
 * colpiti sono unita' sue, e un colpito ignoto richiederebbe di cambiare chi guarda, che da qui non si puo'.
 * ✅ Validato per mutazione: passare a `ChainHopFor`, nel director, la squadra di chi lancia al posto di `PlaybackViewerTeamId`
 * fa comparire i tre salti a chi non vedeva, e cade il caso `colpito`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackChainArcClosedToTheBlindViewerTest,
	"RefactorTactics.Playback.ChainArcStaysClosedWhenTheViewerCannotSeeTheLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackChainArcClosedToTheBlindViewerTest::RunTest(const FString&)
{
	for (const bool bSpettatoreEColpito : { false, true })
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		ARTUnit* Gadget = nullptr;
		ARTTurnManager* const TM = SetUpCatenaAlleSpalle(World, /*TeamGadget=*/ bSpettatoreEColpito ? 1 : 0, Gadget);
		if (!TestTrue(TEXT("⛔ premessa: il turno si costruisce"), TM != nullptr && Gadget != nullptr)) { return false; }
		if (!TestTrue(TEXT("⛔ premessa: Gadget ha LinearDischarge pianificata"), Gadget->GetPlan().PlannedAbilityIndex() != INDEX_NONE)) { return false; }

		TM->LockInAndResolve();

		// ⛔ Premessa: tre colpiti, e (con lo spettatore colpito) almeno un estremo che la squadra 0 non conosce.
		const TArray<FRTResolvedEvent>& Timeline = TM->ResolvedTimelineForTest();
		if (!TestEqual(TEXT("⛔ premessa: tre colpiti"),
			CatenaContaEventi(Timeline, ERTResolvedEventType::Attack, Gadget->StableUnitId), 3)) { return false; }
		int32 ColpitiNonNoti = 0;
		for (const FRTResolvedEvent& Ev : Timeline)
		{
			if (Ev.Type == ERTResolvedEventType::Attack && Ev.SourceStableUnitId == Gadget->StableUnitId
				&& !(Ev.HitGeometry.bResolved && Ev.HitGeometry.FromVerdict.AllowsTeam(0) && Ev.HitGeometry.ImpactVerdict.AllowsTeam(0)))
			{
				++ColpitiNonNoti;
			}
		}
		if (bSpettatoreEColpito)
		{
			if (!TestTrue(TEXT("⛔ premessa (colpito): la squadra 0 non conosce un estremo di almeno un colpo"), ColpitiNonNoti > 0)) { return false; }
		}
		else if (!TestEqual(TEXT("⛔ premessa (controllo): la squadra 0 conosce gli estremi di tutti i colpi"), ColpitiNonNoti, 0)) { return false; }

		bool bLanciato = false;
		const TArray<FString> Salti = CatenaTracciaDelBlast(TM, &bLanciato);
		if (!TestTrue(TEXT("⛔ premessa: il colpo e' partito, quindi `SpawnChainArc` e' stata chiesta"), bLanciato)) { return false; }

		if (bSpettatoreEColpito)
		{
			TestEqual(TEXT("🔴 chi non vede la linea non riceve nessun salto"), FString::Join(Salti, TEXT(",")), FString());
		}
		else
		{
			TestEqual(TEXT("controllo positivo: chi spara riceve i tre salti dello stesso tiro"), Salti.Num(), 3);
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
