// LA RIGA D'ATTIVAZIONE NEL PLAYBACK (#4033, D-600 (1); spec `2026-11-10-registro-attivazione-design.md`, R1-R10 e R7b).
//
// 🔑 **Il meccanismo, non il disegno**: si legge la lista che il TurnManager scrive (`GetActivationLines`) e il feed che il
// Registro rilegge (`URTHudViewModel::BuildPlayerEventFeed(TurnManager, ...)`). La cue `Cast` (`CastCuesPlayedForTest`) e'
// l'orologio di controllo: suona nella stessa `ShowActivation`, quindi dice QUANDO la riga deve nascere.
// ⚠️ Le scene sono quelle di `Tests/Turn/Playback/Reveal/RTSkillCalloutPlaybackTests.cpp` (#4025: Scudo e Tiratore, la finestra di reazione,
// il gadget di Branth) e di `Tests/Turn/Playback/Reveal/RTPlaybackActivationTests.cpp` (il muro alto), copiate con nomi `RigaAtt*`: in unity
// build i namespace anonimi di due file condividono la translation unit. Vedi quei file per il perche' di
// `InitializeActorsForPlay` e di `MakePlayerOnTeam`.
// ⚠️ Il testo atteso si compone con `URTHudViewModel::ComposeActivationLineText` dai campi dell'unita' (`RigaAttTestoAtteso`):
// il formato lo pinna `ActivationLine.TextNamesUnitAndAction`; qui si misura che `ShowActivation` nomini l'unita' e
// l'azione GIUSTE, con le premesse che rendono distinguibili l'etichetta dell'eroe dal nome dell'Actor.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/Format/RTMatchSetupLibrary.h"
#include "Turn/Reaction/RTReactionOpportunityTypes.h" // HoldResponse: la risposta alla finestra che non spara
#include "Turn/RTTurnManager.h"
#include "Turn/Resolution/RTResolvedEvent.h"
#include "Unit/RTUnit.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/Cover/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Ability/Catalog/RTCatalogLibrary.h" // DefaultLoadoutFor / FindEquipment / FindCoreAction
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Camera/RTCameraPawn.h" // la vista strategica (R10)
#include "Perception/RTTeamKnowledge.h"
#include "Player/RTPlayerController.h"
#include "Player/RTPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/Shared/RTAbilityFixtures.h"         // AddCoreAbilityInSlot: l'Overwatch
#include "Tests/Shared/RTReactionWindowFixtures.h"  // la finestra si presenta al trattenimento del playback (#3666)
#include "UI/Feed/RTActivationLine.h"
#include "UI/HUD/RTHudViewModel.h"
#include "UI/HUD/RTScreenHudWidgets.h" // URTPlayerEventLogWidget: il Registro che rilegge il feed (#4052)
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Tests/Shared/RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Nomi distinti per file: in unity build i test condividono la translation unit.

	struct FRigaAttScena
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTPlayerController* Viewer = nullptr;
		ARTUnit* Scudo = nullptr;     // Prep: TideGuard, la skill di firma del kit
		ARTUnit* Tiratore = nullptr;  // Blast: ImpactShot, una variante dell'attacco base
		ARTUnit* Bersaglio = nullptr; // squadra 1, adiacente al Tiratore
	};

	const FName RigaAttTideGuard(TEXT("Hero.Muiren.TideGuard"));
	const FName RigaAttOverwatch(TEXT("Action.Overwatch"));
	const FName RigaAttPortableCover(TEXT("Gadget.PortableCover"));
	const FName RigaAttPulseShot(TEXT("Hero.Ivrin.PulseShot"));

	ARTUnit* RigaAttSpawn(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
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

	int32 RigaAttIndice(const ARTUnit* U, FName ActionId)
	{
		for (int32 i = 0; U && i < U->NumAbilities(); ++i)
		{
			if (U->GetAbility(i) && U->GetAbility(i)->Def.ActionId == ActionId) { return i; }
		}
		return INDEX_NONE;
	}

	URTHexMapAsset* RigaAttMappa(UWorld* World, int32 Raggio)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Raggio);
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = M;
		return M;
	}

	/**
	 * Lo Scudo (TideGuard, Prep) e il Tiratore (ImpactShot sul Bersaglio adiacente, Blast) della squadra 0, e un viewer della
	 * squadra data — la scena di `CostruisciFumetto`. ⚠️ La conoscenza NON e' rinfrescata qui: lo fa il test, dopo aver
	 * aggiunto le sue unita' (`RefreshTeamKnowledgeNow`, la precondizione della Prep).
	 */
	bool RigaAttCostruisci(FAutomationTestBase& Test, int32 SquadraDelViewer, FRigaAttScena& Out)
	{
		Out.World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), Out.World)) { return false; }
		Out.World->InitializeActorsForPlay(FURL()); // senza, il viewer non esiste per il TurnManager
		RigaAttMappa(Out.World, 8);

		Out.Scudo     = RigaAttSpawn(Out.World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-2, 0));
		Out.Tiratore  = RigaAttSpawn(Out.World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
		Out.Bersaglio = RigaAttSpawn(Out.World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(1, 0));
		Out.TM = Out.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Out.TM || !Out.Scudo || !Out.Tiratore || !Out.Bersaglio) { return false; }
		Out.Viewer = RTWorldFixtures::MakePlayerOnTeam(Out.World, SquadraDelViewer);
		if (!Test.TestNotNull(TEXT("viewer"), Out.Viewer)) { return false; }
		// ⛔ Premessa: il viewer che il TurnManager legge (la porta di `BeginPlayback`) e' QUESTO, della squadra data.
		if (!Test.TestTrue(TEXT("⛔ premessa: il TurnManager vede il viewer della fixture"),
				UGameplayStatics::GetPlayerController(Out.TM, 0) == Out.Viewer)
			|| !Test.TestEqual(TEXT("⛔ premessa: e la sua squadra e' quella data"),
				ARTPlayerState::TeamIdOf(UGameplayStatics::GetPlayerController(Out.TM, 0)), SquadraDelViewer))
		{
			return false;
		}

		Out.Scudo->PlannedAbilityIndex = RigaAttIndice(Out.Scudo, RigaAttTideGuard);
		Out.Scudo->PlannedCell = Out.Scudo->Cell;
		Out.Tiratore->PlannedAbilityIndex = 0;
		Out.Tiratore->PlannedAttackTarget = Out.Bersaglio;
		Out.Tiratore->PlannedCell = Out.Tiratore->Cell;
		Out.Bersaglio->PlannedCell = Out.Bersaglio->Cell;

		const URTActionData* AbilityTiratore = Out.Tiratore->GetAbility(0);
		return Test.TestTrue(TEXT("⛔ premessa: lo Scudo pianifica TideGuard"), Out.Scudo->PlannedAbilityIndex != INDEX_NONE)
			&& Test.TestTrue(TEXT("⛔ premessa: il Tiratore ha un'azione nell'indice 0"), AbilityTiratore != nullptr);
	}

	/** L'azione del Tiratore: l'indice 0 del kit di Branth. */
	FName RigaAttAzioneTiratore(const FRigaAttScena& S)
	{
		const URTActionData* A = S.Tiratore ? S.Tiratore->GetAbility(0) : nullptr;
		return A ? A->Def.ActionId : NAME_None;
	}

	void RigaAttFinoAllaFine(ARTTurnManager* TM)
	{
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
	}

	/** Il testo che R5 vuole per (unita', azione): l'etichetta dell'eroe e il nome del kit, composti dalla funzione pura. */
	FString RigaAttTestoAtteso(const ARTUnit* U, FName ActionId)
	{
		return URTHudViewModel::ComposeActivationLineText(
			ARTUnit::DisplayLabel(U->HeroDisplayName, U->HeroId, U->GetName()), U->KitDisplayNameFor(ActionId), ActionId)
			.ToString();
	}

	/** Le righe della lista del manager per (sorgente, azione). Da leggere DOPO `LockInAndResolve` (prima l'id vale 0). */
	int32 RigaAttConta(const ARTTurnManager* TM, const ARTUnit* U, FName ActionId)
	{
		int32 N = 0;
		for (const FRTActivationLine& Riga : TM->GetActivationLines())
		{
			if (Riga.SourceStableUnitId == U->StableUnitId && Riga.ActionId == ActionId) { ++N; }
		}
		return N;
	}

	const FRTActivationLine* RigaAttTrova(const ARTTurnManager* TM, const ARTUnit* U, FName ActionId)
	{
		return TM->GetActivationLines().FindByPredicate([U, ActionId](const FRTActivationLine& Riga)
		{
			return Riga.SourceStableUnitId == U->StableUnitId && Riga.ActionId == ActionId;
		});
	}

	FString RigaAttChiave(ERTMatchPhase Fase, int32 Sorgente, FName Azione)
	{
		return FString::Printf(TEXT("%d:%d:%s"), static_cast<int32>(Fase), Sorgente, *Azione.ToString());
	}

	/** Le righe della lista in ordine, «fase:sorgente:azione»: per l'ordine, per i doppioni e per il messaggio di un rosso. */
	FString RigaAttElenco(const TArray<FRTActivationLine>& Righe)
	{
		TArray<FString> Chiavi;
		for (const FRTActivationLine& Riga : Righe)
		{
			Chiavi.Add(RigaAttChiave(Riga.Phase, Riga.SourceStableUnitId, Riga.ActionId));
		}
		return FString::Join(Chiavi, TEXT(" | "));
	}

	/** Il feed come lo rilegge il Registro, per la squadra data. */
	TArray<FRTPlayerEventLineView> RigaAttFeed(const ARTTurnManager* TM, int32 Squadra)
	{
		return URTHudViewModel::BuildPlayerEventFeed(TM, TArray<int32>{ Squadra });
	}

	int32 RigaAttNelFeed(const TArray<FRTPlayerEventLineView>& Feed, const FString& Testo)
	{
		return Feed.IndexOfByPredicate([&Testo](const FRTPlayerEventLineView& Riga) { return Riga.Text.ToString() == Testo; });
	}

	/** Il feed come testo confrontabile: testo, fase, numero e turno di ogni riga, in ordine. */
	FString RigaAttFeedComeTesto(const TArray<FRTPlayerEventLineView>& Feed)
	{
		TArray<FString> Righe;
		for (const FRTPlayerEventLineView& Riga : Feed)
		{
			Righe.Add(FString::Printf(TEXT("%s [f%d s%d t%d]"), *Riga.Text.ToString(), static_cast<int32>(Riga.Phase),
				Riga.Sequence, Riga.TurnNumber));
		}
		return FString::Join(Righe, TEXT(" | "));
	}

	/** L'Ivrin della squadra 0 che arma l'Overwatch verso il vuoto: l'azione GENERICA (D-025) di una Prep. */
	ARTUnit* RigaAttGuardiaNelVuoto(FAutomationTestBase& Test, UWorld* World, const FRTCellId& Cell)
	{
		ARTUnit* Guardia = RigaAttSpawn(World, 0, URTHeroCatalogLibrary::MakeIvrin(), Cell);
		if (!Guardia) { return nullptr; }
		Guardia->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Guardia, TEXT("Action.Overwatch"), 3);
		Guardia->Facing = ERTHexDirection::W; // nessuno a ovest: niente da intercettare
		Guardia->PlannedCell = Guardia->Cell;
		return Test.TestTrue(TEXT("⛔ premessa: la Guardia pianifica l'Overwatch"), Guardia->PlannedAbilityIndex != INDEX_NONE)
			? Guardia : nullptr;
	}

	/**
	 * Branth col loadout di DEFAULT (`DefaultLoadoutFor`, come `RTMatchBootstrapper`) che erige `Gadget.PortableCover` sul
	 * bordo W della cella a est: la forma di `RefactorTactics.Equipment.PortableCover.CreatesCover`. Il piano si scrive su
	 * un'unita' gia' spawnata.
	 */
	bool RigaAttPianificaCopertura(FAutomationTestBase& Test, ARTUnit* Geniere, const URTHeroData* HeroBranth,
		const FRTCellId& CellaCopertura)
	{
		Geniere->EquipLoadout(URTCatalogLibrary::DefaultLoadoutFor(HeroBranth->HeroId));
		Geniere->PlannedAbilityIndex = RigaAttIndice(Geniere, RigaAttPortableCover);
		Geniere->PlannedAttackTarget = nullptr;
		Geniere->PlannedAttackCell = CellaCopertura;
		Geniere->bAttackTargetsCell = true;
		Geniere->PlannedCoverEdge = ERTHexDirection::W;
		Geniere->bHasPlannedCoverEdge = true;
		Geniere->PlannedCell = Geniere->Cell;
		return Test.TestTrue(TEXT("⛔ premessa: il loadout di default di Branth concede Gadget.PortableCover"),
			Geniere->PlannedAbilityIndex != INDEX_NONE);
	}
}

/**
 * La riga nasce quando il playback MOSTRA l'attivazione, non prima (R4, R1).
 *
 * 🔑 L'istante si misura con la cue `Cast`, che suona nella stessa `ShowActivation`: a ogni tick prima della cue la riga non
 * c'e', e nel tick della cue c'e'. ⛔ Una riga scritta all'inizio del playback (in `BeginPlayback`, da tutte le attivazioni
 * ammesse) renderebbe rossi i primi asserti; una scritta solo a fine playback (il recupero di R8 da solo), quelli dopo la cue.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLinePlaybackAppearsTest,
	"RefactorTactics.Playback.ActivationLine.AppearsWhenShownNotBefore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLinePlaybackAppearsTest::RunTest(const FString&)
{
	FRigaAttScena S;
	const bool bOk = RigaAttCostruisci(*this, /*Viewer*/ 0, S);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
	if (!bOk) { return false; }
	S.TM->RefreshTeamKnowledgeNow();

	// ⛔ Premesse sul TESTO: se l'etichetta dell'eroe coincidesse col nome dell'Actor, o il kit non avesse un nome, un
	// `ShowActivation` che componesse con `GetName()` o con l'id resterebbe verde.
	const FString Etichetta = ARTUnit::DisplayLabel(S.Scudo->HeroDisplayName, S.Scudo->HeroId, S.Scudo->GetName());
	const FText NomeDelKit = S.Scudo->KitDisplayNameFor(RigaAttTideGuard);
	if (!TestFalse(TEXT("⛔ premessa: l'etichetta dell'eroe non e' il nome dell'Actor"), Etichetta == S.Scudo->GetName())
		|| !TestFalse(TEXT("⛔ premessa: TideGuard ha un nome nel kit"), NomeDelKit.IsEmptyOrWhitespace()))
	{
		return false;
	}
	const FString Atteso = RigaAttTestoAtteso(S.Scudo, RigaAttTideGuard);
	AddInfo(FString::Printf(TEXT("riga attesa: %s"), *Atteso));

	S.TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ premessa: il playback e' partito"), S.TM->IsResolving())
		|| !TestEqual(TEXT("⛔ premessa: TideGuard non e' ancora stata mostrata (nessuna cue Cast)"),
			S.Scudo->CastCuesPlayedForTest(), 0)
		|| !TestTrue(TEXT("⛔ premessa: il turno ha voci di TurnLog (il colpo del Tiratore)"), S.TM->GetTurnLog().Num() > 0))
	{
		return false;
	}

	TestEqual(TEXT("🔴 all'inizio del playback nessuna riga d'attivazione"), S.TM->GetActivationLines().Num(), 0);
	TestEqual(TEXT("🔴 e il feed non ha la riga di TideGuard"), RigaAttNelFeed(RigaAttFeed(S.TM, 0), Atteso), INDEX_NONE);

	bool bGiaDetto = false;
	for (int32 I = 0; I < 400 && S.TM->IsResolving() && S.Scudo->CastCuesPlayedForTest() == 0; ++I)
	{
		S.TM->Tick(0.05f);
		if (!bGiaDetto && S.Scudo->CastCuesPlayedForTest() == 0 && S.TM->GetActivationLines().Num() > 0)
		{
			bGiaDetto = true;
			AddError(FString::Printf(TEXT("🔴 al tick %d c'e' una riga e la cue di TideGuard no: e' nata prima che il "
				"playback mostrasse l'attivazione (%s)"), I + 1, *RigaAttElenco(S.TM->GetActivationLines())));
		}
	}
	if (!TestEqual(TEXT("⛔ premessa: il playback ha mostrato TideGuard (la cue Cast e' suonata)"),
			S.Scudo->CastCuesPlayedForTest(), 1)
		|| !TestEqual(TEXT("⛔ premessa: il Blast non e' ancora arrivato (il Tiratore non ha attivato)"),
			S.Tiratore->CastCuesPlayedForTest(), 0))
	{
		return false;
	}

	TestEqual(TEXT("🔴 nel tick della cue la riga di TideGuard c'e', una"), RigaAttConta(S.TM, S.Scudo, RigaAttTideGuard), 1);
	TestEqual(TEXT("e quella del Tiratore, che il playback non ha ancora mostrato, no"),
		RigaAttConta(S.TM, S.Tiratore, RigaAttAzioneTiratore(S)), 0);
	if (const FRTActivationLine* Riga = RigaAttTrova(S.TM, S.Scudo, RigaAttTideGuard))
	{
		TestEqual(TEXT("il testo nomina l'eroe e la skill del kit (R5)"), Riga->Text.ToString(), Atteso);
		TestTrue(TEXT("e contiene il nome del kit"), Riga->Text.ToString().Contains(NomeDelKit.ToString()));
		TestTrue(TEXT("la fase e' la Prep"), Riga->Phase == ERTMatchPhase::Prep);
		TestEqual(TEXT("la chiave del pubblico e' la squadra del viewer (R7b)"), Riga->ViewerTeamId, 0);
		TestEqual(TEXT("il turno e' quello delle voci di TurnLog del turno mostrato"), Riga->TurnNumber,
			S.TM->GetTurnLog()[0].TurnNumber);
	}
	TestTrue(TEXT("🔴 e il feed del Registro la contiene"), RigaAttNelFeed(RigaAttFeed(S.TM, 0), Atteso) != INDEX_NONE);
	return true;
}

/**
 * Ogni tipo di azione che il playback mostra ha la sua riga, e grida (R1, D-600 (1); [D-604] (1), (4)).
 *
 * 🔑 Quattro attivazioni della squadra 0 nello stesso turno: TideGuard (skill di firma), ImpactShot (variante dell'attacco
 * base), l'Overwatch (azione generica del catalogo core) e `Gadget.PortableCover` (equipaggiamento). Le righe si leggono
 * mentre il playback e' ANCORA in corso, appena l'ultima cue `Cast` e' suonata: cosi' le scrive `ShowActivation` e non il
 * recupero di fine playback, che le darebbe anche se `ShowActivation` le scartasse.
 * ⛔ Con [D-604] gridano tutte e quattro, e il contrasto col fumetto non e' piu' in questa scena: la riga di un'azione che
 * non grida e' in `Playback.SkillCallout.InteractDoesNotShout`. ⏱️ *Fino a #4059 qui gridava la sola skill del kit.*
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLinePlaybackEveryKindTest,
	"RefactorTactics.Playback.ActivationLine.EveryActionKindHasALine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLinePlaybackEveryKindTest::RunTest(const FString&)
{
	FRigaAttScena S;
	const bool bOk = RigaAttCostruisci(*this, /*Viewer*/ 0, S);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
	if (!bOk) { return false; }

	ARTUnit* const Guardia = RigaAttGuardiaNelVuoto(*this, S.World, FRTCellId(-3, 3));
	const URTHeroData* HeroBranth = URTHeroCatalogLibrary::MakeBranth();
	ARTUnit* const Geniere = RigaAttSpawn(S.World, 0, HeroBranth, FRTCellId(2, 3));
	if (!Guardia || !Geniere || !HeroBranth
		|| !RigaAttPianificaCopertura(*this, Geniere, HeroBranth, FRTCellId(3, 3)))
	{
		return false;
	}

	// ⛔ Premesse sui TIPI: le quattro azioni stanno davvero in quattro famiglie diverse.
	const FName AzioneBase = RigaAttAzioneTiratore(S);
	const URTActionData* Firma = S.Scudo->GetAbility(S.Scudo->PlannedAbilityIndex);
	if (!TestTrue(TEXT("⛔ premessa: TideGuard e' una skill del kit (nessuna base, ne' core ne' equipaggiamento)"),
			Firma && Firma->Def.BaseActionId.IsNone() && URTCatalogLibrary::FindCoreAction(RigaAttTideGuard).ActionId.IsNone()
			&& URTCatalogLibrary::FindEquipment(RigaAttTideGuard) == nullptr)
		|| !TestTrue(TEXT("⛔ premessa: l'azione del Tiratore e' una variante di un'azione base"),
			!S.Tiratore->GetAbility(0)->Def.BaseActionId.IsNone())
		|| !TestFalse(TEXT("⛔ premessa: l'Overwatch e' un'azione generica del catalogo core"),
			URTCatalogLibrary::FindCoreAction(RigaAttOverwatch).ActionId.IsNone())
		|| !TestTrue(TEXT("⛔ premessa: il gadget sta nel catalogo d'equipaggiamento"),
			URTCatalogLibrary::FindEquipment(RigaAttPortableCover) != nullptr))
	{
		return false;
	}

	S.TM->RefreshTeamKnowledgeNow();
	S.TM->LockInAndResolve();
	const auto TutteMostrate = [&S, Guardia, Geniere]()
	{
		return S.Scudo->CastCuesPlayedForTest() > 0 && S.Tiratore->CastCuesPlayedForTest() > 0
			&& Guardia->CastCuesPlayedForTest() > 0 && Geniere->CastCuesPlayedForTest() > 0;
	};
	for (int32 I = 0; I < 400 && S.TM->IsResolving() && !TutteMostrate(); ++I) { S.TM->Tick(0.05f); }

	if (!TestTrue(TEXT("⛔ premessa: le quattro attivazioni sono state mostrate (una cue Cast ciascuna)"), TutteMostrate())
		|| !TestTrue(TEXT("⛔ premessa: e il playback e' ancora in corso (nessun recupero di fine playback)"),
			S.TM->IsResolving()))
	{
		return false;
	}
	AddInfo(FString::Printf(TEXT("righe: %s"), *RigaAttElenco(S.TM->GetActivationLines())));

	TestEqual(TEXT("🔴 la skill del kit ha la sua riga"), RigaAttConta(S.TM, S.Scudo, RigaAttTideGuard), 1);
	TestEqual(TEXT("🔴 l'attacco base ha la sua riga"), RigaAttConta(S.TM, S.Tiratore, AzioneBase), 1);
	TestEqual(TEXT("🔴 l'azione generica ha la sua riga"), RigaAttConta(S.TM, Guardia, RigaAttOverwatch), 1);
	TestEqual(TEXT("🔴 il gadget ha la sua riga"), RigaAttConta(S.TM, Geniere, RigaAttPortableCover), 1);
	TestEqual(TEXT("e nient'altro"), S.TM->GetActivationLines().Num(), 4);

	TestEqual(TEXT("✅ il fumetto grida la skill del kit"), S.Scudo->LastSkillCalloutForTest().ActionId, RigaAttTideGuard);
	TestEqual(TEXT("🔴 grida l'attacco base ([D-604] (4))"), S.Tiratore->LastSkillCalloutForTest().ActionId, AzioneBase);
	TestEqual(TEXT("🔴 grida l'azione generica"), Guardia->LastSkillCalloutForTest().ActionId, RigaAttOverwatch);
	TestEqual(TEXT("🔴 e grida il gadget"), Geniere->LastSkillCalloutForTest().ActionId, RigaAttPortableCover);
	return true;
}

/**
 * Una sorgente nemica che chi guarda non osserva non ha riga: ne' durante il playback, ne' a fine playback, ne' dopo un
 * salto (R2, R8).
 *
 * 🔑 La scena di `Playback.EnemyBehindHighCoverHasNoActivationBeat`: l'Ivrin della squadra 1 spara dietro un muro alto, a
 * distanza 3, al Branth della squadra 0, che vede la propria cella ma non la sua. In piu', Branth erige il suo gadget in
 * Prep: e' il CONTROLLO POSITIVO nella stessa scena (la sua riga c'e', anche dopo il salto, dove la recupera R8) e da' al
 * playback qualcosa da mostrare, cosi' che il salto avvenga davvero a playback in corso.
 * ⛔ Il recupero di R8 legge le stesse liste gia' filtrate del playback: un recupero dalla timeline intera darebbe la riga
 * dell'Ivrin proprio nel ramo del salto. ✅ Lo stesso turno visto dalla squadra 1 ha la riga dell'Ivrin.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLinePlaybackHiddenSourceTest,
	"RefactorTactics.Playback.ActivationLine.HiddenSourceHasNoLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLinePlaybackHiddenSourceTest::RunTest(const FString&)
{
	struct FRigaAttGiro
	{
		int32 RigheDelNemico = INDEX_NONE;
		int32 RigheDelGadget = INDEX_NONE;
		bool bNemicoVisto = true;
		bool bNemicoHaAttivato = false;
		bool bInCorsoAlSalto = false;
		bool bFeedSenzaNemico = false;
	};

	const auto Gira = [this](int32 SquadraDelViewer, bool bSalta, FRigaAttGiro& Out) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		World->InitializeActorsForPlay(FURL()); // il viewer deve esistere per il TurnManager
		URTHexMapAsset* Map = RigaAttMappa(World, 6);

		const URTHeroData* HeroBranth = URTHeroCatalogLibrary::MakeBranth();
		ARTUnit* Nemico = RigaAttSpawn(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0));
		ARTUnit* Branth = RigaAttSpawn(World, 0, HeroBranth, FRTCellId(3, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTPlayerController* Viewer = RTWorldFixtures::MakePlayerOnTeam(World, SquadraDelViewer);
		if (!TM || !Map || !Nemico || !Branth || !Viewer || !HeroBranth) { return false; }
		if (!TestEqual(TEXT("⛔ premessa: il TurnManager legge la squadra del viewer della fixture"),
				ARTPlayerState::TeamIdOf(UGameplayStatics::GetPlayerController(TM, 0)), SquadraDelViewer))
		{
			return false;
		}
		// Il muro alto sul bordo fra (0,0) e (1,0): la convenzione `Ring[EdgeIndex]` di `ResolveCoverStructures`.
		const int32 Bordo = URTHexLibrary::Neighbors(FRTCellId(0, 0)).IndexOfByKey(FRTCellId(1, 0));
		if (!TestTrue(TEXT("⛔ premessa: (1,0) e' un vicino di (0,0)"), Bordo != INDEX_NONE)) { return false; }
		URTHexCoverLibrary::AddCover(Map, FRTCellId(0, 0), static_cast<ERTHexDirection>(Bordo), ERTHexCoverType::High, 100);

		Nemico->PlannedAbilityIndex = RigaAttIndice(Nemico, RigaAttPulseShot);
		Nemico->PlannedAttackTarget = Branth;
		Nemico->PlannedCell = Nemico->Cell;
		if (!TestTrue(TEXT("⛔ premessa: l'Ivrin pianifica PulseShot"), Nemico->PlannedAbilityIndex != INDEX_NONE)
			|| !RigaAttPianificaCopertura(*this, Branth, HeroBranth, FRTCellId(4, 0)))
		{
			return false;
		}

		TM->RefreshTeamKnowledgeNow();
		// ⛔ La squadra 0 E' nella conoscenza (vede la propria cella): senza, il verdetto sarebbe il fail-closed della
		// squadra assente, cioe' il filtro e non la percezione.
		if (!TestTrue(TEXT("⛔ premessa: la squadra 0 e' in TeamKnowledgeState"),
				TM->KnowledgeForTeamPublic(0).VisibleCells.Contains(Branth->Cell)))
		{
			return false;
		}

		TM->LockInAndResolve();
		// Il verdetto dell'attivazione si congela all'emissione, nel Blast; in questo turno nessuno si muove e il muro
		// resta, quindi la lettura a fine risoluzione e' quella dell'emissione (vedi il test del muro alto).
		Out.bNemicoVisto = TM->KnowledgeForTeamPublic(0).VisibleCells.Contains(Nemico->Cell);
		Out.bNemicoHaAttivato = TM->ResolvedTimelineForTest().ContainsByPredicate([Nemico](const FRTResolvedEvent& Ev)
		{
			return Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.SourceStableUnitId == Nemico->StableUnitId;
		});
		if (bSalta)
		{
			Out.bInCorsoAlSalto = TM->IsResolving();
			TM->SkipPlayback();
		}
		RigaAttFinoAllaFine(TM);
		// ⛔ Le righe si contano a fine VERA, dove gira il recupero di R8: un playback ancora in corso le conterebbe a meta'.
		if (!TestFalse(TEXT("⛔ premessa: il playback e' finito"), TM->IsResolving())) { return false; }

		Out.RigheDelNemico = RigaAttConta(TM, Nemico, RigaAttPulseShot);
		Out.RigheDelGadget = RigaAttConta(TM, Branth, RigaAttPortableCover);
		Out.bFeedSenzaNemico =
			RigaAttNelFeed(RigaAttFeed(TM, SquadraDelViewer), RigaAttTestoAtteso(Nemico, RigaAttPulseShot)) == INDEX_NONE;
		AddInfo(FString::Printf(TEXT("viewer %d%s: righe %s"), SquadraDelViewer, bSalta ? TEXT(" (salto)") : TEXT(""),
			*RigaAttElenco(TM->GetActivationLines())));
		return true;
	};

	// --- Fino alla fine del playback ----------------------------------------------------------------------------------
	FRigaAttGiro Fine;
	if (!Gira(/*Viewer*/ 0, /*bSalta*/ false, Fine)) { return false; }
	if (!TestFalse(TEXT("⛔ premessa: la squadra 0 non vede la cella dell'Ivrin"), Fine.bNemicoVisto)
		|| !TestTrue(TEXT("⛔ premessa: l'Ivrin si e' attivato (la timeline canonica lo conserva)"), Fine.bNemicoHaAttivato))
	{
		return false;
	}
	TestEqual(TEXT("🔴 sorgente non osservata: nessuna riga a fine playback"), Fine.RigheDelNemico, 0);
	TestTrue(TEXT("🔴 e il feed non la nomina"), Fine.bFeedSenzaNemico);
	TestEqual(TEXT("✅ controllo positivo nella stessa scena: il gadget della squadra 0 ha la sua riga"),
		Fine.RigheDelGadget, 1);

	// --- Dopo un salto ------------------------------------------------------------------------------------------------
	FRigaAttGiro Salto;
	if (!Gira(/*Viewer*/ 0, /*bSalta*/ true, Salto)) { return false; }
	if (!TestTrue(TEXT("⛔ premessa: il salto e' avvenuto a playback in corso"), Salto.bInCorsoAlSalto)
		|| !TestTrue(TEXT("⛔ premessa: l'Ivrin si e' attivato anche qui"), Salto.bNemicoHaAttivato))
	{
		return false;
	}
	TestEqual(TEXT("🔴 dopo un salto il recupero non da' la riga della sorgente non osservata"), Salto.RigheDelNemico, 0);
	TestTrue(TEXT("🔴 e il feed non la nomina"), Salto.bFeedSenzaNemico);
	TestEqual(TEXT("✅ mentre recupera quella del gadget, che il salto non aveva lasciato mostrare"), Salto.RigheDelGadget, 1);

	// --- Lo stesso turno, visto dalla squadra della sorgente ----------------------------------------------------------
	FRigaAttGiro Proprio;
	if (!Gira(/*Viewer*/ 1, /*bSalta*/ false, Proprio)) { return false; }
	TestEqual(TEXT("✅ vista dalla squadra della sorgente, la riga c'e'"), Proprio.RigheDelNemico, 1);
	return true;
}

/**
 * Fuori dalla dominanza: l'unita' che colpisce ha nel feed sia «Colpita — N» sia la sua riga d'attivazione, quest'ultima
 * SUBITO SOPRA, e nessuna delle due sostituisce l'altra (R3, R6).
 *
 * 🔑 Tolte dal feed le righe d'attivazione restano, nell'ordine, esattamente le righe d'esito che il TurnLog da' oggi
 * (`BuildPlayerEventFeed` sul solo TurnLog): la catena di D-299 (2) non cambia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLinePlaybackOutsideDominanceTest,
	"RefactorTactics.Playback.ActivationLine.OutsideDominance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLinePlaybackOutsideDominanceTest::RunTest(const FString&)
{
	FRigaAttScena S;
	const bool bOk = RigaAttCostruisci(*this, /*Viewer*/ 0, S);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
	if (!bOk) { return false; }
	S.TM->RefreshTeamKnowledgeNow();
	S.TM->LockInAndResolve();
	RigaAttFinoAllaFine(S.TM);
	if (!TestFalse(TEXT("⛔ premessa: il playback e' finito"), S.TM->IsResolving())) { return false; }

	const TArray<FRTPlayerEventLineView> Feed = RigaAttFeed(S.TM, 0);
	AddInfo(FString::Printf(TEXT("feed: %s"), *RigaAttFeedComeTesto(Feed)));
	const int32 Colpo = Feed.IndexOfByPredicate([&S](const FRTPlayerEventLineView& Riga)
	{
		return Riga.PrimaryStableUnitId == S.Tiratore->StableUnitId && Riga.Text.ToString().StartsWith(TEXT("Colpita"));
	});
	if (!TestTrue(TEXT("⛔ premessa: il Tiratore ha la sua riga d'esito «Colpita» nel feed"), Colpo != INDEX_NONE))
	{
		return false;
	}

	const FString Attivazione = RigaAttTestoAtteso(S.Tiratore, RigaAttAzioneTiratore(S));
	TestTrue(TEXT("🔴 subito sopra la riga del colpo c'e' la riga d'attivazione del Tiratore"),
		Colpo > 0 && Feed[Colpo - 1].Text.ToString() == Attivazione);
	if (Colpo > 0)
	{
		TestEqual(TEXT("e appartiene al Tiratore"), Feed[Colpo - 1].PrimaryStableUnitId, S.Tiratore->StableUnitId);
	}
	TestEqual(TEXT("una sola riga d'attivazione del Tiratore"),
		Feed.FilterByPredicate([&Attivazione](const FRTPlayerEventLineView& R) { return R.Text.ToString() == Attivazione; }).Num(), 1);

	// Le righe d'esito restano quelle di oggi: tolte le attivazioni, il feed e' il TurnLog proiettato come prima.
	const TArray<int32> Osservatori = { 0 };
	const TArray<FRTPlayerEventLineView> Esiti = URTHudViewModel::BuildPlayerEventFeed(
		S.TM->GetTurnLog(), Osservatori, URTHudViewModel::OwnTeamForFeed(S.TM, Osservatori));
	TSet<FString> TestiDelleAttivazioni;
	for (const FRTActivationLine& Riga : S.TM->GetActivationLines()) { TestiDelleAttivazioni.Add(Riga.Text.ToString()); }
	TArray<FString> SenzaAttivazioni;
	for (const FRTPlayerEventLineView& Riga : Feed)
	{
		if (!TestiDelleAttivazioni.Contains(Riga.Text.ToString())) { SenzaAttivazioni.Add(Riga.Text.ToString()); }
	}
	TArray<FString> TestiDegliEsiti;
	for (const FRTPlayerEventLineView& Riga : Esiti) { TestiDegliEsiti.Add(Riga.Text.ToString()); }
	TestEqual(TEXT("🔴 nessuna riga d'esito e' stata sostituita o tolta"),
		FString::Join(SenzaAttivazioni, TEXT(" | ")), FString::Join(TestiDegliEsiti, TEXT(" | ")));
	TestEqual(TEXT("e nessuna riga d'attivazione e' stata assorbita da un esito"),
		Feed.Num(), Esiti.Num() + S.TM->GetActivationLines().Num());
	return true;
}

namespace
{
	/**
	 * La scena della finestra di reazione (`MontaFinestraDelFumetto` di #4025): la Guardia (squadra 1) in Overwatch verso
	 * ovest da (3,0), il Mover (squadra 0) che da (0,0) va a (2,0) ed entra nella sua linea in (1,0), lo Scudo (squadra 0,
	 * lontano) che in Prep attiva TideGuard. ⚠️ Mondo NON inizializzato: il viewer e' il ripiego a 0, la squadra di Scudo e
	 * Mover.
	 */
	struct FRigaAttFinestra
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* Scudo = nullptr;
		ARTUnit* Mover = nullptr;
		ARTUnit* Guardia = nullptr;
	};

	bool RigaAttMontaFinestra(FAutomationTestBase& Test, FRigaAttFinestra& Out)
	{
		Out.World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), Out.World)) { return false; }
		RigaAttMappa(Out.World, 8);

		Out.Mover   = RigaAttSpawn(Out.World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0));
		Out.Guardia = RigaAttSpawn(Out.World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(3, 0));
		Out.Scudo   = RigaAttSpawn(Out.World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-3, 3));
		Out.TM = Out.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Out.TM || !Out.Mover || !Out.Guardia || !Out.Scudo) { return false; }

		Out.Guardia->PlannedAbilityIndex =
			RTAbilityFixtures::AddCoreAbilityInSlot(Out.Guardia, TEXT("Action.Overwatch"), 3);
		Out.Guardia->Facing = ERTHexDirection::W;
		Out.Guardia->PlannedCell = Out.Guardia->Cell;
		Out.Mover->PlannedCell = FRTCellId(2, 0);
		Out.Scudo->PlannedCell = Out.Scudo->Cell;
		Out.Scudo->PlannedAbilityIndex = RigaAttIndice(Out.Scudo, RigaAttTideGuard);
		if (!Test.TestTrue(TEXT("⛔ premessa: la Guardia pianifica l'Overwatch"), Out.Guardia->PlannedAbilityIndex != INDEX_NONE)
			|| !Test.TestTrue(TEXT("⛔ premessa: lo Scudo pianifica TideGuard"), Out.Scudo->PlannedAbilityIndex != INDEX_NONE))
		{
			return false;
		}

		Out.TM->RefreshTeamKnowledgeNow();
		return true;
	}
}

/**
 * Le righe nate prima di una finestra di reazione ci sono ancora alla ripresa, senza doppioni; il turno dopo parte vuoto
 * (R7, R8; Review Focus 2).
 *
 * 🔑 La ripresa passa da `ConcludeResolution` e da `BeginPlayback(true)`, che ricostruisce `PlaybackActivationsPrep`: un
 * azzeramento in uno dei due toglierebbe le righe sotto chi ha appena deciso, e un recupero di fine playback senza chiave
 * d'identita' ridarebbe le righe gia' nate. ⛔ La finestra si apre solo con un ascoltatore legato a
 * `OnReactionWindowOpened`, e il fotogramma della presentazione e' quello del delegate: le righe si contano li' dentro.
 * ⛔ Il turno dopo: `LockInAndResolve` azzera la lista accanto a `TurnLog.Reset()`, prima di ogni playback.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLinePlaybackReactionWindowTest,
	"RefactorTactics.Playback.ActivationLine.SurvivesTheReactionWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLinePlaybackReactionWindowTest::RunTest(const FString&)
{
	FRigaAttFinestra S;
	const bool bOk = RigaAttMontaFinestra(*this, S);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
	if (!bOk) { return false; }

	// ⚠️ Lo slegamento e' dichiarato DOPO la distruzione del mondo, quindi gira PRIMA (ordine inverso degli scope exit).
	int32 Presentazioni = 0;
	FString RigheAllaPresentazione;
	ARTTurnManager* const TM = S.TM;
	S.TM->OnReactionWindowOpened.BindLambda(
		[&Presentazioni, &RigheAllaPresentazione, TM](const FRTReactionWindowView&, int32)
		{
			++Presentazioni;
			RigheAllaPresentazione = RigaAttElenco(TM->GetActivationLines());
		});
	ON_SCOPE_EXIT{ TM->OnReactionWindowOpened.Unbind(); };

	S.TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ premessa: la resolution si e' sospesa su una finestra"), S.TM->IsResolutionSuspended())
		|| !TestTrue(TEXT("⛔ premessa: la finestra si presenta pompando il playback"),
			RTReactionWindowFixtures::PompaFinoAllaPresentazione(S.TM))
		|| !TestTrue(TEXT("⛔ premessa: alla presentazione il playback e' trattenuto"), S.TM->IsPlaybackHeldByWindow())
		|| !TestEqual(TEXT("⛔ premessa: la finestra e' stata presentata una volta"), Presentazioni, 1))
	{
		return false;
	}
	AddInfo(FString::Printf(TEXT("righe alla presentazione: %s"), *RigheAllaPresentazione));

	// --- (1) prima della finestra la riga e' nata, e la finestra la trova -------------------------------------------
	const FString Prima = RigaAttElenco(S.TM->GetActivationLines());
	TestEqual(TEXT("🔴 TideGuard, mostrata prima della finestra, ha la sua riga nel fotogramma della presentazione"),
		RigaAttConta(S.TM, S.Scudo, RigaAttTideGuard), 1);
	TestEqual(TEXT("e la lista e' quella del fotogramma della presentazione"), Prima, RigheAllaPresentazione);
	for (int32 I = 0; I < 5; ++I) { S.TM->Tick(0.05f); }
	if (!TestTrue(TEXT("⛔ premessa: la finestra attende ancora"), S.TM->IsResolutionSuspended())) { return false; }
	TestEqual(TEXT("mentre la finestra attende la lista non cambia"), RigaAttElenco(S.TM->GetActivationLines()), Prima);

	// --- (2) la risposta: `ConcludeResolution` e `BeginPlayback(true)` ----------------------------------------------
	S.TM->SubmitReactionResponse(S.TM->GetOpenReactionWindowId(), URTReactionOpportunityLibrary::HoldResponse());
	if (!TestFalse(TEXT("⛔ premessa: la finestra e' chiusa e il turno ripreso"), S.TM->IsResolutionSuspended())
		|| !TestTrue(TEXT("⛔ premessa: dopo la risposta c'e' un playback residuo (BeginPlayback(true))"), S.TM->IsResolving()))
	{
		return false;
	}
	TestEqual(TEXT("🔴 alla ripresa le righe nate prima della finestra ci sono ancora, uguali"),
		RigaAttElenco(S.TM->GetActivationLines()), Prima);

	RigaAttFinoAllaFine(S.TM);
	if (!TestFalse(TEXT("⛔ premessa: il playback e' finito (fine VERA, col recupero di R8)"), S.TM->IsResolving()))
	{
		return false;
	}
	const FString Dopo = RigaAttElenco(S.TM->GetActivationLines());
	AddInfo(FString::Printf(TEXT("righe a fine turno: %s"), *Dopo));
	TestEqual(TEXT("🔴 nessun doppione: TideGuard ha ancora una riga sola"), RigaAttConta(S.TM, S.Scudo, RigaAttTideGuard), 1);
	TSet<FString> Chiavi;
	for (const FRTActivationLine& Riga : S.TM->GetActivationLines())
	{
		bool bGiaVista = false;
		Chiavi.Add(RigaAttChiave(Riga.Phase, Riga.SourceStableUnitId, Riga.ActionId), &bGiaVista);
		TestFalse(FString::Printf(TEXT("🔴 la riga %s compare una volta sola"),
			*RigaAttChiave(Riga.Phase, Riga.SourceStableUnitId, Riga.ActionId)), bGiaVista);
	}
	TestTrue(TEXT("le righe di prima della finestra restano in testa, nell'ordine"), Dopo.StartsWith(Prima));

	// --- (3) il Planning che segue le mostra ancora, il turno dopo parte vuoto ----------------------------------------
	TestTrue(TEXT("nel Planning che segue le righe del turno restano (R7)"), S.TM->GetActivationLines().Num() > 0);
	const int32 TurnoPrima = S.TM->GetTurnNumber();
	S.TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ premessa: il turno dopo e' stato bloccato e risolto"), S.TM->GetTurnNumber() > TurnoPrima
			|| S.TM->IsResolving()))
	{
		return false;
	}
	TestEqual(TEXT("🔴 il turno dopo parte senza le righe del turno prima"), RigaAttConta(S.TM, S.Scudo, RigaAttTideGuard), 0);
	TestEqual(TEXT("e senza nessun'altra"), S.TM->GetActivationLines().Num(), 0);
	return true;
}

/**
 * Saltare il playback a meta' lascia una riga per ogni attivazione visibile del turno, ciascuna una volta, nell'ordine del
 * playback (R8; Review Focus 1).
 *
 * 🔑 Tre attivazioni della squadra 0: due di Prep (TideGuard, l'Overwatch della Guardia) e una di Blast (il Tiratore). Il
 * salto cade dopo la PRIMA riga mostrata: il recupero deve dare le altre due, senza ridare la prima, e nell'ordine in cui il
 * playback le avrebbe mostrate — Prep nell'ordine della timeline, poi Dash, poi Blast. ⛔ Le mutazioni che questo rende
 * rosse: nessun recupero (una riga sola), recupero senza chiave d'identita' (la prima due volte), recupero col Blast prima
 * della Prep (l'Overwatch dopo il Tiratore).
 * ⚠️ Una sola attivazione di Blast: l'ordine della sequenza di Blast e quello della timeline qui coincidono per costruzione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLinePlaybackSkipTest,
	"RefactorTactics.Playback.ActivationLine.SkipRevealsTheRest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLinePlaybackSkipTest::RunTest(const FString&)
{
	FRigaAttScena S;
	const bool bOk = RigaAttCostruisci(*this, /*Viewer*/ 0, S);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
	if (!bOk) { return false; }
	if (!RigaAttGuardiaNelVuoto(*this, S.World, FRTCellId(-3, 3))) { return false; }

	S.TM->RefreshTeamKnowledgeNow();
	S.TM->LockInAndResolve();

	// L'ordine del playback, dalla timeline: le attivazioni di Prep e di Dash nell'ordine in cui sono state emesse (cosi'
	// le accoda `BeginPlayback`), poi quella del Blast. Sono tutte della squadra di chi guarda: il playback le ammette tutte.
	TArray<FString> Attese;
	for (const ERTMatchPhase Fase : { ERTMatchPhase::Prep, ERTMatchPhase::Dash, ERTMatchPhase::Blast })
	{
		for (const FRTResolvedEvent& Ev : S.TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.Phase == Fase)
			{
				Attese.Add(RigaAttChiave(Ev.Phase, Ev.SourceStableUnitId, Ev.ActionId));
			}
		}
	}
	if (!TestEqual(TEXT("⛔ premessa: tre attivazioni nel turno (due di Prep, una di Blast)"), Attese.Num(), 3)
		|| !TestTrue(TEXT("⛔ premessa: il playback e' partito"), S.TM->IsResolving()))
	{
		return false;
	}

	for (int32 I = 0; I < 400 && S.TM->IsResolving() && S.TM->GetActivationLines().Num() == 0; ++I) { S.TM->Tick(0.05f); }
	if (!TestEqual(TEXT("⛔ premessa: il playback ha mostrato UNA attivazione"), S.TM->GetActivationLines().Num(), 1)
		|| !TestTrue(TEXT("⛔ premessa: e il playback e' ancora in corso (il salto cade a meta')"), S.TM->IsResolving())
		|| !TestEqual(TEXT("⛔ premessa: la mostrata e' la prima dell'ordine del playback"),
			RigaAttElenco(S.TM->GetActivationLines()), Attese[0]))
	{
		return false;
	}

	S.TM->SkipPlayback();
	if (!TestFalse(TEXT("⛔ premessa: il salto ha chiuso il playback"), S.TM->IsResolving())) { return false; }

	TestEqual(TEXT("🔴 dopo il salto: una riga per attivazione, ciascuna una volta, nell'ordine del playback"),
		RigaAttElenco(S.TM->GetActivationLines()), FString::Join(Attese, TEXT(" | ")));
	return true;
}

namespace
{
	const FName RigaAttBrace(TEXT("Action.Brace"));
	const FName RigaAttPush(TEXT("Action.Push"));
	const FName RigaAttFluidTrail(TEXT("Hero.Muiren.FluidTrail"));

	/**
	 * La scena di `Reactions.Brace.WindowSuspendsBlast` (`Tests/Turn/Reaction/RTDefensiveReactionTests.cpp`), nella forma da playback di
	 * `Playback.ActivationPrefixSurvivesASuspendedBlast`: l'Ivrin della squadra 0 si irrigidisce in Prep (`Action.Brace`, con
	 * `Profile.Sidestep`) e il Branth della squadra 1, adiacente, lo spinge nel Blast (`Action.Push`): la spinta apre la
	 * finestra e sospende il Blast. In piu', lontana, una Muiren della squadra 0 scatta di due celle con la scia: e'
	 * l'attivazione di Dash. ⚠️ Mondo NON inizializzato: il viewer e' il ripiego a 0, la squadra di Bracer e Scattista.
	 */
	struct FRigaAttBrace
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* Bracer = nullptr;
		ARTUnit* Pusher = nullptr;
		ARTUnit* Scattista = nullptr;
	};

	bool RigaAttMontaBrace(FAutomationTestBase& Test, FRigaAttBrace& Out)
	{
		Out.World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), Out.World)) { return false; }
		RigaAttMappa(Out.World, 8);

		Out.Bracer    = RigaAttSpawn(Out.World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0));
		Out.Pusher    = RigaAttSpawn(Out.World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0));
		Out.Scattista = RigaAttSpawn(Out.World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-3, 3));
		Out.TM = Out.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Out.TM || !Out.Bracer || !Out.Pusher || !Out.Scattista) { return false; }

		Out.Bracer->ReactionProfileId = TEXT("Profile.Sidestep"); // senza, la cardinalita' e' 1 e nessuna finestra e' dovuta
		Out.Bracer->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Out.Bracer, TEXT("Action.Brace"), 3);
		Out.Bracer->PlannedCell = Out.Bracer->Cell;
		Out.Pusher->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Out.Pusher, TEXT("Action.Push"), 3);
		Out.Pusher->PlannedAttackTarget = Out.Bracer;
		Out.Pusher->PlannedCell = Out.Pusher->Cell;
		Out.Scattista->PlannedDashAbility = RigaAttIndice(Out.Scattista, RigaAttFluidTrail);
		Out.Scattista->PlannedDashCell = FRTCellId(-1, 3);
		Out.Scattista->PlannedCell = Out.Scattista->Cell;
		if (!Test.TestTrue(TEXT("⛔ premessa: il Bracer pianifica Action.Brace"), Out.Bracer->PlannedAbilityIndex != INDEX_NONE)
			|| !Test.TestTrue(TEXT("⛔ premessa: il Pusher pianifica Action.Push"), Out.Pusher->PlannedAbilityIndex != INDEX_NONE)
			|| !Test.TestTrue(TEXT("⛔ premessa: la Scattista pianifica la scia"), Out.Scattista->PlannedDashAbility != INDEX_NONE))
		{
			return false;
		}

		Out.TM->RefreshTeamKnowledgeNow();
		return true;
	}

	/** Se la timeline ha l'attivazione di (fase, unita', azione). Da leggere DOPO `LockInAndResolve`. */
	bool RigaAttAttivataIn(const ARTTurnManager* TM, ERTMatchPhase Fase, const ARTUnit* U, FName ActionId)
	{
		return TM->ResolvedTimelineForTest().ContainsByPredicate([Fase, U, ActionId](const FRTResolvedEvent& Ev)
		{
			return Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.Phase == Fase
				&& Ev.SourceStableUnitId == U->StableUnitId && Ev.ActionId == ActionId;
		});
	}
}

/**
 * Un salto durante Prep/Dash di un playback PARZIALE, con la finestra di reazione nel Blast, lascia le righe nell'ordine
 * delle fasi del playback: Prep, Dash, Blast (R8; review finale di #4033).
 *
 * 🔑 Il salto cade prima che il playback abbia mostrato niente: passa dal ramo sospeso di `FinishPlayback`, che presenta la
 * finestra e non da' righe (non e' una fine vera). Alla ripresa il playback mostra il Blast, quindi la riga della spinta
 * NASCE per prima; quelle di Prep e Dash le da' il recupero alla fine vera, dopo. ⛔ Senza l'ordinamento per fase in
 * `FinishPlayback` la lista finisce Blast -> Prep -> Dash, e questo test e' rosso: misurato togliendo la riga.
 * ⛔ La finestra si apre solo con un ascoltatore legato a `OnReactionWindowOpened`, come in `WindowSuspendsBlast`.
 * ➕ #4059 (R5, [D-604] (3)): dopo la spinta, la riga della RISPOSTA del Brace, in fase Blast. La mostra la ripresa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLinePlaybackSkipBeforeBraceTest,
	"RefactorTactics.Playback.ActivationLine.SkipBeforeABraceWindowKeepsPlaybackOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLinePlaybackSkipBeforeBraceTest::RunTest(const FString&)
{
	FRigaAttBrace S;
	const bool bOk = RigaAttMontaBrace(*this, S);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
	if (!bOk) { return false; }

	// ⚠️ Lo slegamento e' dichiarato DOPO la distruzione del mondo, quindi gira PRIMA (ordine inverso degli scope exit).
	int32 Presentazioni = 0;
	FString RigheAllaPresentazione;
	ARTTurnManager* const TM = S.TM;
	S.TM->OnReactionWindowOpened.BindLambda(
		[&Presentazioni, &RigheAllaPresentazione, TM](const FRTReactionWindowView&, int32)
		{
			++Presentazioni;
			RigheAllaPresentazione = RigaAttElenco(TM->GetActivationLines());
		});
	ON_SCOPE_EXIT{ TM->OnReactionWindowOpened.Unbind(); };

	S.TM->LockInAndResolve();

	// ⛔ Le tre attivazioni, una per fase, tutte di squadre che chi guarda vede: l'ordine del playback e' questo.
	const FString Attese = FString::Join(TArray<FString>{
		RigaAttChiave(ERTMatchPhase::Prep, S.Bracer->StableUnitId, RigaAttBrace),
		RigaAttChiave(ERTMatchPhase::Dash, S.Scattista->StableUnitId, RigaAttFluidTrail),
		RigaAttChiave(ERTMatchPhase::Blast, S.Pusher->StableUnitId, RigaAttPush),
		RigaAttChiave(ERTMatchPhase::Blast, S.Bracer->StableUnitId, RigaAttBrace) }, TEXT(" | ")); // #4059: la risposta
	if (!TestTrue(TEXT("⛔ premessa: la timeline ha l'attivazione di Prep (Action.Brace)"),
			RigaAttAttivataIn(S.TM, ERTMatchPhase::Prep, S.Bracer, RigaAttBrace))
		|| !TestTrue(TEXT("⛔ premessa: la timeline ha l'attivazione di Dash (la scia)"),
			RigaAttAttivataIn(S.TM, ERTMatchPhase::Dash, S.Scattista, RigaAttFluidTrail))
		|| !TestTrue(TEXT("⛔ premessa: la timeline ha l'attivazione di Blast (Action.Push)"),
			RigaAttAttivataIn(S.TM, ERTMatchPhase::Blast, S.Pusher, RigaAttPush))
		|| !TestTrue(TEXT("⛔ premessa: la resolution e' sospesa su una finestra"), S.TM->IsResolutionSuspended())
		|| !TestTrue(TEXT("⛔ premessa: ed e' il Blast che si e' fermato"), S.TM->GetPhase() == ERTMatchPhase::Blast)
		|| !TestTrue(TEXT("⛔ premessa: il playback parziale e' partito"), S.TM->IsResolving()))
	{
		return false;
	}

	// --- (1) il salto, prima che Prep e Dash siano mostrate -------------------------------------------------------------
	const ERTMatchPhase FaseAlSalto = S.TM->GetPlaybackPhase();
	if (!TestTrue(TEXT("⛔ premessa: il salto cade in Prep o in Dash"),
			FaseAlSalto == ERTMatchPhase::Prep || FaseAlSalto == ERTMatchPhase::Dash)
		|| !TestEqual(TEXT("⛔ premessa: al salto nessuna riga"), S.TM->GetActivationLines().Num(), 0)
		|| !TestEqual(TEXT("⛔ premessa: al salto l'irrigidimento non e' stato mostrato (nessuna cue Cast)"),
			S.Bracer->CastCuesPlayedForTest(), 0)
		|| !TestEqual(TEXT("⛔ premessa: e nemmeno la scia"), S.Scattista->CastCuesPlayedForTest(), 0)
		|| !TestEqual(TEXT("⛔ premessa: la finestra non e' ancora presentata"), Presentazioni, 0))
	{
		return false;
	}
	S.TM->SkipPlayback();
	if (!TestEqual(TEXT("⛔ premessa: il salto ha presentato la finestra, una volta"), Presentazioni, 1)
		|| !TestTrue(TEXT("⛔ premessa: e il playback e' trattenuto dalla finestra"), S.TM->IsPlaybackHeldByWindow())
		|| !TestTrue(TEXT("⛔ premessa: la finestra attende"), S.TM->IsResolutionSuspended())
		|| !TestEqual(TEXT("⛔ premessa: il ramo sospeso non e' una fine vera, nessuna riga alla presentazione (R8)"),
			RigheAllaPresentazione, FString()))
	{
		return false;
	}

	// --- (2) la ripresa mostra il Blast, la fine vera recupera Prep e Dash ---------------------------------------------
	for (int32 Scadenze = 0; Scadenze < 8 && S.TM->IsResolutionSuspended(); ++Scadenze)
	{
		S.TM->ExpireReactionWindow();
	}
	if (!TestFalse(TEXT("⛔ premessa: la finestra e' chiusa e il turno ripreso"), S.TM->IsResolutionSuspended())) { return false; }
	RigaAttFinoAllaFine(S.TM);
	if (!TestFalse(TEXT("⛔ premessa: il playback e' finito (fine VERA, col recupero di R8)"), S.TM->IsResolving())
		|| !TestEqual(TEXT("⛔ premessa: la ripresa ha MOSTRATO la spinta: la sua riga e' nata da ShowActivation"),
			S.Pusher->CastCuesPlayedForTest(), 1)
		|| !TestEqual(TEXT("⛔ premessa: l'irrigidimento non e' mai stato mostrato: la sua riga e' del recupero, nato dopo"),
			S.Bracer->CastCuesPlayedForTest(), 0)
		|| !TestEqual(TEXT("⛔ premessa: e nemmeno la scia"), S.Scattista->CastCuesPlayedForTest(), 0))
	{
		return false;
	}

	TestEqual(TEXT("🔴 a fine vera le righe sono nell'ordine delle fasi del playback (Prep, Dash, Blast con la risposta), una volta ciascuna"),
		RigaAttElenco(S.TM->GetActivationLines()), Attese);
	return true;
}

/**
 * Un turno SENZA playback (`bEnablePlayback` falso: `ConcludeResolution` passa a `ConcludeTurn` senza `BeginPlayback`) non
 * mostra le righe del turno prima (R7).
 *
 * 🔑 E' la ragione per cui l'azzeramento sta in `LockInAndResolve` accanto a `TurnLog.Reset()` e non in `BeginPlayback`: li'
 * questo turno non passa. ✅ Prima del secondo turno le righe del primo ci sono (restano nel Planning).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLinePlaybackNoPlaybackTest,
	"RefactorTactics.Playback.ActivationLine.TurnWithoutPlaybackStartsEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLinePlaybackNoPlaybackTest::RunTest(const FString&)
{
	FRigaAttScena S;
	const bool bOk = RigaAttCostruisci(*this, /*Viewer*/ 0, S);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
	if (!bOk) { return false; }
	S.TM->RefreshTeamKnowledgeNow();
	S.TM->LockInAndResolve();
	RigaAttFinoAllaFine(S.TM);

	if (!TestFalse(TEXT("⛔ premessa: il primo turno e' finito"), S.TM->IsResolving())
		|| !TestEqual(TEXT("⛔ premessa: e nel Planning che segue le sue due righe ci sono"),
			S.TM->GetActivationLines().Num(), 2))
	{
		return false;
	}
	TArray<FString> TestiDelPrimo;
	for (const FRTActivationLine& Riga : S.TM->GetActivationLines()) { TestiDelPrimo.Add(Riga.Text.ToString()); }

	const int32 TurnoPrima = S.TM->GetTurnNumber();
	S.TM->bEnablePlayback = false;
	S.TM->LockInAndResolve();
	if (!TestFalse(TEXT("⛔ premessa: il secondo turno non ha playback"), S.TM->IsResolving())
		|| !TestTrue(TEXT("⛔ premessa: e si e' concluso"), S.TM->GetTurnNumber() > TurnoPrima))
	{
		return false;
	}

	TestEqual(TEXT("🔴 un turno senza playback parte senza le righe del turno prima"), S.TM->GetActivationLines().Num(), 0);
	const TArray<FRTPlayerEventLineView> Feed = RigaAttFeed(S.TM, 0);
	for (const FString& Testo : TestiDelPrimo)
	{
		TestEqual(FString::Printf(TEXT("e il feed non nomina «%s»"), *Testo), RigaAttNelFeed(Feed, Testo), INDEX_NONE);
	}
	return true;
}

/**
 * Il feed e' lo stesso con la vista strategica accesa e spenta (R10): nessun ramo sulla vista, ne' quando la riga nasce ne'
 * quando il Registro la rilegge.
 *
 * 🔑 Lo stesso turno due volte, la camera del viewer in vista tattica e in vista strategica (`ToggleStrategicView`, D-488)
 * per tutto il playback; nel secondo giro la vista torna tattica a fine turno e il feed si rilegge. ⛔ Controllo: il feed ha
 * le righe d'attivazione dei due eroi, quindi il confronto non e' fra due vuoti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLinePlaybackBothViewsTest,
	"RefactorTactics.Playback.ActivationLine.SameInBothViews",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLinePlaybackBothViewsTest::RunTest(const FString&)
{
	const auto Gira = [this](bool bStrategica, FString& OutFeed, FString& OutFeedRiletto, int32& OutRigheAttivazione) -> bool
	{
		FRigaAttScena S;
		const bool bOk = RigaAttCostruisci(*this, /*Viewer*/ 0, S);
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
		if (!bOk) { return false; }

		ARTCameraPawn* Cam = S.World->SpawnActor<ARTCameraPawn>();
		if (!TestNotNull(TEXT("camera"), Cam)) { return false; }
		S.Viewer->Possess(Cam);
		if (bStrategica) { Cam->ToggleStrategicView(); }
		if (!TestTrue(TEXT("⛔ premessa: la vista della camera del viewer e' quella del giro"), Cam->IsStrategicView() == bStrategica)
			|| !TestTrue(TEXT("⛔ premessa: e' la camera del viewer"), S.Viewer->GetPawn() == Cam))
		{
			return false;
		}

		S.TM->RefreshTeamKnowledgeNow();
		S.TM->LockInAndResolve();
		RigaAttFinoAllaFine(S.TM);
		if (!TestFalse(TEXT("⛔ premessa: il playback e' finito"), S.TM->IsResolving())) { return false; }

		const TArray<FRTPlayerEventLineView> Feed = RigaAttFeed(S.TM, 0);
		OutFeed = RigaAttFeedComeTesto(Feed);
		OutRigheAttivazione = 0;
		for (const FString& Attesa : { RigaAttTestoAtteso(S.Scudo, RigaAttTideGuard),
			RigaAttTestoAtteso(S.Tiratore, RigaAttAzioneTiratore(S)) })
		{
			OutRigheAttivazione += RigaAttNelFeed(Feed, Attesa) != INDEX_NONE ? 1 : 0;
		}
		if (bStrategica)
		{
			Cam->ToggleStrategicView();
			if (!TestFalse(TEXT("⛔ premessa: la vista e' tornata tattica"), Cam->IsStrategicView())) { return false; }
		}
		OutFeedRiletto = RigaAttFeedComeTesto(RigaAttFeed(S.TM, 0));
		return true;
	};

	FString Tattica, TatticaRiletta, Strategica, StrategicaRiletta;
	int32 RigheTattica = 0, RigheStrategica = 0;
	if (!Gira(/*bStrategica*/ false, Tattica, TatticaRiletta, RigheTattica)
		|| !Gira(/*bStrategica*/ true, Strategica, StrategicaRiletta, RigheStrategica))
	{
		return false;
	}
	AddInfo(FString::Printf(TEXT("feed in vista tattica: %s"), *Tattica));

	if (!TestEqual(TEXT("⛔ controllo: in vista tattica il feed ha le due righe d'attivazione"), RigheTattica, 2))
	{
		return false;
	}
	TestEqual(TEXT("🔴 in vista strategica le stesse due righe"), RigheStrategica, 2);
	TestEqual(TEXT("🔴 e lo stesso feed, riga per riga"), Strategica, Tattica);
	TestEqual(TEXT("e rileggerlo dopo aver cambiato vista non lo cambia"), StrategicaRiletta, Strategica);
	return true;
}

/**
 * 🔴 **IL REGISTRO MOSTRA LA RIGA NEL GIRO IN CUI NASCE, SENZA ASPETTARE I 5 HZ** ([D-603] (1), #4052).
 *
 * 🔑 Il widget gira con `TickFeed(0)`: il suo tempo non avanza, quindi dopo il primo giro la rilettura a 5 Hz non scatta
 * piu', e l'unica ragione per cui la riga puo' comparire e' che sia nata. ⛔ Prima di #4052 il Registro aspettava il giro
 * dei 5 Hz: col tempo fermo la riga non arrivava mai, e la prima gamba e' rossa.
 * 🔴 La seconda gamba e' il controllo che il resto del feed resta a 5 Hz ([D-603] (1)): una voce d'esito iniettata nel
 * TurnLog, senza nessuna riga d'attivazione nuova, non compare dopo 0,1 s e compare dopo 0,25. Un Registro che rileggesse
 * a ogni giro renderebbe rossa questa gamba.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActivationLinePlaybackLogSameFrameTest,
	"RefactorTactics.Playback.ActivationLine.LogShowsItInTheTickItIsBorn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActivationLinePlaybackLogSameFrameTest::RunTest(const FString&)
{
	FRigaAttScena S;
	const bool bOk = RigaAttCostruisci(*this, /*Viewer*/ 0, S);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
	if (!bOk) { return false; }
	S.TM->RefreshTeamKnowledgeNow();

	URTPlayerEventLogWidget* Log = NewObject<URTPlayerEventLogWidget>(S.World);
	if (!TestNotNull(TEXT("il widget del Registro"), Log)) { return false; }
	Log->SetMatchContextForTest(S.TM, /*PlayerTeamId=*/ 0);
	Log->LogRoot = NewObject<UBorder>(Log);
	Log->LogTurnText = NewObject<UTextBlock>(Log);
	Log->LinesBox = NewObject<UVerticalBox>(Log);
	Log->LineClass = URTPlayerEventLineWidget::StaticClass();

	const auto Testi = [Log]()
	{
		TArray<FString> Out;
		for (const URTPlayerEventLineWidget* Riga : Log->GetLineWidgets())
		{
			if (Riga) { Out.Add(Riga->GetLine().Text.ToString()); }
		}
		return Out;
	};
	const FString Atteso = RigaAttTestoAtteso(S.Scudo, RigaAttTideGuard);

	S.TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ premessa: il playback e' partito"), S.TM->IsResolving())) { return false; }
	Log->TickFeed(0.f); // il primo giro rilegge comunque: il Registro si apre in Risoluzione
	if (!TestTrue(TEXT("⛔ premessa: il Registro ha costruito le righe d'esito del turno"), Testi().Num() > 0)
		|| !TestFalse(TEXT("⛔ premessa: e non ha ancora la riga di TideGuard"), Testi().Contains(Atteso)))
	{
		return false;
	}

	// --- 1. Il tick in cui nasce la riga, con il tempo del widget fermo ------------------------------------------------------
	bool bNata = false;
	for (int32 I = 0; I < 400 && S.TM->IsResolving() && !bNata; ++I)
	{
		S.TM->Tick(0.05f);
		Log->TickFeed(0.f);
		bNata = S.TM->GetActivationLines().Num() > 0;
	}
	if (!TestTrue(TEXT("⛔ premessa: il playback ha mostrato TideGuard, e la sua riga e' nata"), bNata)
		|| !TestEqual(TEXT("⛔ premessa: ed e' l'unica riga nata"), S.TM->GetActivationLines().Num(), 1)
		|| !TestTrue(TEXT("⛔ premessa: il feed la contiene"), RigaAttNelFeed(RigaAttFeed(S.TM, 0), Atteso) != INDEX_NONE))
	{
		return false;
	}
	TestTrue(FString::Printf(TEXT("🔴 nel giro del widget dello stesso tick la riga e' a schermo, senza aspettare 0,2 s "
		"(righe: %s)"), *FString::Join(Testi(), TEXT(" | "))), Testi().Contains(Atteso));

	// --- 2. Il controllo: senza righe d'attivazione nuove il feed resta riletto a 5 Hz ---------------------------------------
	int32 Turno = 0;
	for (const FRTTurnLogEntry& Voce : S.TM->GetTurnLog()) { Turno = FMath::Max(Turno, Voce.TurnNumber); }
	const int32 UnitaSintetica = 9001; // non e' un'unita' in campo: il proiettore non la cerca
	FRTTurnLogEntry Iniettata;
	Iniettata.Category = ERTLogCategory::Combat;
	Iniettata.Outcome = static_cast<uint8>(ERTCombatOutcome::Hit);
	Iniettata.UnitId = UnitaSintetica;
	Iniettata.TurnNumber = Turno;
	Iniettata.Phase = ERTMatchPhase::Blast;
	Iniettata.Amount = 7;
	Iniettata.Verdict.AllowTeam(0);
	const int32 AttivazioniPrima = S.TM->GetActivationLines().Num();
	S.TM->AppendTurnLogEntryForTest(Iniettata);

	const auto HaLaIniettata = [Log, UnitaSintetica]()
	{
		for (const URTPlayerEventLineWidget* Riga : Log->GetLineWidgets())
		{
			if (Riga && Riga->GetLine().PrimaryStableUnitId == UnitaSintetica) { return true; }
		}
		return false;
	};
	const bool bNelFeed = RigaAttFeed(S.TM, 0).ContainsByPredicate([UnitaSintetica](const FRTPlayerEventLineView& Riga)
	{
		return Riga.PrimaryStableUnitId == UnitaSintetica;
	});
	if (!TestTrue(TEXT("⛔ premessa: il feed contiene la voce iniettata"), bNelFeed)
		|| !TestFalse(TEXT("⛔ premessa: il Registro non l'ha ancora riletta"), HaLaIniettata()))
	{
		return false;
	}
	Log->TickFeed(0.1f);
	TestFalse(TEXT("🔴 a 0,1 s, senza righe d'attivazione nuove, il Registro non rilegge"), HaLaIniettata());
	Log->TickFeed(0.15f);
	TestTrue(TEXT("controllo: passati 0,2 s il giro dei 5 Hz la porta"), HaLaIniettata());
	TestEqual(TEXT("⛔ premessa: in questi giri non e' nata nessuna riga d'attivazione"),
		S.TM->GetActivationLines().Num(), AttivazioniPrima);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
