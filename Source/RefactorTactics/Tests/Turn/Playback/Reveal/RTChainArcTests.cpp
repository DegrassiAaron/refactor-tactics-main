// La scarica a catena di LinearDischarge e l'ordine dei colpi lungo la linea ([D-620], #4136).
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
#include "Tests/Shared/RTWorldFixtures.h"

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
 * I colpi di un attacco a linea si mostrano dal piu' vicino al piu' lontano — [D-620] (4).
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
 * La catena: sorgente → piu' vicino → successivo — [D-620] (3).
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

/** Senza colpiti, un solo arco dalla sorgente all'esagono mirato, al battito dell'impronta — [D-620] (3). */
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
	TestEqual(TEXT("il sistema e' l'_Arc della guida"), Righe[0].Value.System.ToString(),
		FString(TEXT("/Game/RT/Characters/Gadget/FX/NS_Gadget_LinearDischarge_Arc.NS_Gadget_LinearDischarge_Arc")));
	TestFalse(TEXT("il primo salto parte da un socket nominato"), Righe[0].Value.SourceSocket.IsNone());

	TestNotNull(TEXT("ChainFxFor trova la riga dall'ActionId"), URTPresentationBindingLibrary::ChainFxFor(Righe[0].Key, NAME_None));
	TestNotNull(TEXT("e dal BaseActionId"), URTPresentationBindingLibrary::ChainFxFor(NAME_None, Righe[0].Key));
	TestNull(TEXT("un'azione senza riga non ha scarica"),
		URTPresentationBindingLibrary::ChainFxFor(FName(TEXT("Hero.Muiren.PressureJet")), NAME_None));
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

#endif // WITH_DEV_AUTOMATION_TESTS
