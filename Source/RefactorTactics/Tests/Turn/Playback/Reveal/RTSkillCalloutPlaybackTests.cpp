// IL FUMETTO DELLA SKILL NEL PLAYBACK (#4025, spec del fumetto R1b, R4, R7, R9).
//
// 🔑 **Il meccanismo, non il disegno**: headless l'overlay non ha un widget, quindi si legge il REGISTRO che
// `ARTUnit::ShowSkillCallout` scrive sempre (`LastSkillCalloutForTest`), la sua vita (`IsSkillCalloutLive`) e
// l'ultimo tempo che il playback gli ha portato (`SkillCalloutClockForTest`). Sono i tre fatti che il
// `TurnManager` e' l'unico a decidere: se accendere, quale orologio dare, quando spegnere.
// ⚠️ La fixture e' quella di `Tests/Turn/Playback/Reveal/RTPlaybackActivationTests.cpp` (stessa scena, stessa porta del viewer: vedi
// quel file per il perche' di `InitializeActorsForPlay` e di `MakePlayerOnTeam`), copiata con nomi locali
// DIVERSI perche' in unity build i namespace anonimi di due file condividono la translation unit.
// ⛔ Lo Scudo (Muiren) pianifica `Hero.Muiren.TideGuard` in Prep: una skill di firma, `BaseActionId` vuoto. Il
// Tiratore (Branth) pianifica l'indice 0, `Hero.Branth.ImpactShot`: una variante dell'attacco base. Con [D-604] gridano
// entrambi; fino a #4059 gridava solo il primo.
// ⚠️ Altre scene, ciascuna col suo montaggio: la finestra di reazione (l'orologio del fumetto sopravvive alla ripresa,
// R4/R7), il loadout di Branth (l'equipaggiamento grida), l'interazione (l'azione del Blast che non grida) e le reazioni
// del Blast (#4059: il fumetto all'arrivo del colpo, e niente per chi guarda senza vedere chi reagisce).

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/Format/RTMatchSetupLibrary.h"
#include "Turn/Reaction/RTReactionOpportunityTypes.h" // HoldResponse: la risposta alla finestra che non spara
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Ability/Catalog/RTCatalogLibrary.h" // DefaultLoadoutFor / FindEquipment / FindCoreAction: il loadout vero di Branth
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Player/RTPlayerController.h"
#include "Player/RTPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/Shared/RTAbilityFixtures.h"         // AddCoreAbilityInSlot: l'Overwatch della guardia
#include "Tests/Shared/RTReactionWindowFixtures.h"  // #3666: la finestra si presenta al trattenimento del playback
#include "UI/HUD/RTHudViewModel.h"               // SkillCalloutAlpha: l'opacita' che il widget ricaverebbe dall'orologio
#include "Tests/Shared/RTWorldFixtures.h"
#include "Map/RTHexCellData.h"            // #4059: la porta chiusa dell'interazione
#include "Map/Cover/RTHexCoverLibrary.h"  // #4059: la premessa che la porta si e' aperta
#include "UI/Feed/RTActivationLine.h"     // #4059: le righe del Registro delle reazioni

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Nomi distinti per file: in unity build i test condividono la translation unit.

	struct FRTFumettoDiProva
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* Scudo = nullptr;    // Prep: TideGuard, skill di firma
		ARTUnit* Tiratore = nullptr; // Blast: ImpactShot, attacco base
	};

	const FName AzioneFirmaFumetto(TEXT("Hero.Muiren.TideGuard"));

	ARTUnit* SpawnUnitaFumetto(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
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

	/** Due attivazioni della squadra 0 — una di Prep (firma), una di Blast (base) — e un viewer della squadra data. */
	bool CostruisciFumetto(FAutomationTestBase& Test, int32 SquadraDelViewer, FRTFumettoDiProva& Out)
	{
		Out.World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), Out.World)) { return false; }
		// 🔴 Senza, il viewer non esiste per il TurnManager (`GetPlayerController(this, 0)` risponde `nullptr` e
		// `TeamIdOf` ripiega su 0): vedi `CostruisciBeat` in `RTPlaybackActivationTests.cpp`.
		Out.World->InitializeActorsForPlay(FURL());
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 8);
		ARTHexMapActor* MapActor = Out.World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = M;

		Out.Scudo    = SpawnUnitaFumetto(Out.World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-2, 0));
		Out.Tiratore = SpawnUnitaFumetto(Out.World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
		ARTUnit* Bersaglio = SpawnUnitaFumetto(Out.World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(1, 0));
		Out.TM = Out.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Out.TM || !Out.Scudo || !Out.Tiratore || !Bersaglio) { return false; }
		ARTPlayerController* Viewer = RTWorldFixtures::MakePlayerOnTeam(Out.World, SquadraDelViewer);
		if (!Test.TestNotNull(TEXT("viewer"), Viewer)) { return false; }
		// ⛔ Premessa: il viewer che il TurnManager legge e' QUESTO, della squadra data. Senza, ogni asserto sotto
		// misurerebbe il ripiego sulla squadra 0.
		if (!Test.TestTrue(TEXT("⛔ premessa: il TurnManager vede il viewer della fixture"),
				UGameplayStatics::GetPlayerController(Out.Scudo, 0) == Viewer)
			|| !Test.TestEqual(TEXT("⛔ premessa: e la sua squadra e' quella data"),
				ARTPlayerState::TeamIdOf(UGameplayStatics::GetPlayerController(Out.Scudo, 0)), SquadraDelViewer))
		{
			return false;
		}

		for (int32 i = 0; i < Out.Scudo->NumAbilities(); ++i)
		{
			if (Out.Scudo->GetAbility(i) && Out.Scudo->GetAbility(i)->Def.ActionId == AzioneFirmaFumetto)
			{
				Out.Scudo->PlannedAbilityIndex = i;
			}
		}
		Out.Scudo->PlannedCell = Out.Scudo->Cell;
		Out.Tiratore->PlannedAbilityIndex = 0;
		Out.Tiratore->PlannedAttackTarget = Bersaglio;
		Out.Tiratore->PlannedCell = Out.Tiratore->Cell;

		Out.TM->RefreshTeamKnowledgeNow(); // la precondizione della Prep (spec §2.5)

		// ⛔ Premesse sulle FAMIGLIE, non sul playback: la scena vale solo se le due azioni sono davvero una skill di firma
		// (base vuota) e una variante dell'attacco base (con base), quella che fino a #4059 non gridava.
		const URTActionData* AbilityScudo = Out.Scudo->GetAbility(Out.Scudo->PlannedAbilityIndex);
		const URTActionData* AbilityTiratore = Out.Tiratore->GetAbility(0);
		if (!Test.TestTrue(TEXT("⛔ premessa: lo Scudo pianifica TideGuard"),
				AbilityScudo != nullptr && AbilityScudo->Def.ActionId == AzioneFirmaFumetto)
			|| !Test.TestTrue(TEXT("⛔ premessa: TideGuard non ha una base (e' una skill di firma)"),
				AbilityScudo->Def.BaseActionId.IsNone())
			|| !Test.TestTrue(TEXT("⛔ premessa: ImpactShot e' una variante di un'azione base"),
				AbilityTiratore != nullptr && !AbilityTiratore->Def.BaseActionId.IsNone()))
		{
			return false;
		}
		return true;
	}

	void CorriFinoAllaFineDelFumetto(ARTTurnManager* TM)
	{
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
	}

	/** Tick di 0,05 s, uno alla volta, finche' il fumetto dell'unita' non e' vivo. Vero se lo e' diventato. */
	bool CorriFinoAlFumettoVivo(ARTTurnManager* TM, const ARTUnit* Unit)
	{
		for (int32 I = 0; I < 400 && TM->IsResolving() && !Unit->IsSkillCalloutLive(); ++I) { TM->Tick(0.05f); }
		return Unit->IsSkillCalloutLive();
	}
}

/**
 * Chi grida: la skill di firma e l'attacco base, tutti e due — [D-604] (4), R1 della spec di #4059.
 *
 * 🔑 Il Tiratore pianifica `Hero.Branth.ImpactShot`, una variante dell'attacco base: fino a #4059 restava muto, e ora il
 * suo fumetto porta la SUA azione, attribuita a lui. ⛔ La sua cue Cast e' suonata una volta: il fumetto e' quello della
 * sua attivazione.
 * ⏱️ *Fino a #4059 questo test si chiamava `SignatureSkillShoutsAndBaseAttackDoesNot` e asseriva che il Tiratore tacesse.*
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackSignatureShoutsTest,
	"RefactorTactics.Playback.SkillCallout.SignatureSkillAndBaseAttackBothShout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackSignatureShoutsTest::RunTest(const FString&)
{
	FRTFumettoDiProva B;
	const bool bOk = CostruisciFumetto(*this, /*Viewer*/ 0, B);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
	if (!bOk) { return false; }

	B.TM->LockInAndResolve();
	CorriFinoAllaFineDelFumetto(B.TM);

	TestEqual(TEXT("🔴 lo Scudo ha gridato TideGuard"), B.Scudo->LastSkillCalloutForTest().ActionId, AzioneFirmaFumetto);
	TestEqual(TEXT("e il fumetto e' attribuito allo Scudo"),
		B.Scudo->LastSkillCalloutForTest().SourceStableUnitId, B.Scudo->StableUnitId);
	TestTrue(TEXT("⛔ il Tiratore ha attivato una volta (la cue Cast e' suonata)"),
		B.Tiratore->CastCuesPlayedForTest() == 1);
	TestEqual(TEXT("🔴 e ImpactShot, con la sua base, grida anche lui"),
		B.Tiratore->LastSkillCalloutForTest().ActionId, FName(TEXT("Hero.Branth.ImpactShot")));
	TestEqual(TEXT("e il fumetto e' attribuito al Tiratore"),
		B.Tiratore->LastSkillCalloutForTest().SourceStableUnitId, B.Tiratore->StableUnitId);
	return true;
}

/**
 * La vita del fumetto e' il playback: acceso con l'orologio del playback, spento a fine turno — R4, R7.
 *
 * 🔑 `StartAt == SkillCalloutClock` del manager (e `> 0`) dice che l'origine e' l'orologio del fumetto nell'istante
 * della rivelazione (non un contatore dell'unita', non uno zero), e il clock dell'unita' `>= StartAt` che gli e'
 * arrivato. A fine playback la vita e' chiusa, la lista del manager vuota e il suo orologio a zero, ma il registro
 * resta: e' lo scopo del campo farsi leggere dopo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackLiveThenClearedTest,
	"RefactorTactics.Playback.SkillCallout.LiveDuringPlaybackClearedWhenPlaybackEnds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackLiveThenClearedTest::RunTest(const FString&)
{
	FRTFumettoDiProva B;
	const bool bOk = CostruisciFumetto(*this, /*Viewer*/ 0, B);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
	if (!bOk) { return false; }

	B.TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ premessa: il fumetto dello Scudo diventa vivo durante il playback"),
			CorriFinoAlFumettoVivo(B.TM, B.Scudo)))
	{
		return false;
	}

	const FRTSkillCalloutView Acceso = B.Scudo->LastSkillCalloutForTest();
	// 🔴 **UGUALE, non `<=`**: `CorriFinoAlFumettoVivo` esce nel tick stesso della rivelazione, e dopo `ShowActivation`
	// `SkillCalloutClock` non cambia in quel tick, quindi l'origine del fumetto E' l'orologio di adesso. Con `<=` un
	// `ShowSkillCallout(Ev.ActionId, 0.f)` restava verde (0 e' sempre minore o uguale), e nessun altro test legge `StartAt`.
	// `> 0` perche' la rivelazione cade dopo il primo `Dt` (`TickPlayback` lo somma prima di rivelare): un `StartAt`
	// a zero e' un'origine non portata, e `Equal` da solo non la distinguerebbe se l'orologio fosse a zero.
	TestEqual(TEXT("🔴 l'origine del fumetto e' ESATTAMENTE l'orologio del fumetto (StartAt == SkillCalloutClock)"),
		Acceso.StartAt, B.TM->PlaybackSkillCalloutClockForTest());
	TestTrue(TEXT("🔴 e non e' zero: la rivelazione cade dopo il primo tick (StartAt > 0)"), Acceso.StartAt > 0.f);
	TestTrue(TEXT("🔴 e quell'orologio gli e' arrivato (clock >= StartAt)"),
		B.Scudo->SkillCalloutClockForTest() >= Acceso.StartAt);
	TestEqual(TEXT("il manager tiene l'unita' accesa nella sua lista"), B.TM->SkillCalloutUnitsForTest(), 1);

	CorriFinoAllaFineDelFumetto(B.TM);

	TestFalse(TEXT("premessa: il playback e' finito"), B.TM->IsResolving());
	TestFalse(TEXT("🔴 a fine playback il fumetto e' spento"), B.Scudo->IsSkillCalloutLive());
	TestEqual(TEXT("e la lista del manager e' vuota"), B.TM->SkillCalloutUnitsForTest(), 0);
	// Il turno dopo deve partire da zero: l'orologio del fumetto si azzera alla fine VERA, non in `ConcludeResolution`.
	TestEqual(TEXT("e l'orologio del fumetto e' tornato a zero"), B.TM->PlaybackSkillCalloutClockForTest(), 0.f);
	TestEqual(TEXT("ma il registro resta leggibile"), B.Scudo->LastSkillCalloutForTest().ActionId, AzioneFirmaFumetto);
	return true;
}

/**
 * Una sorgente che il viewer non osserva non grida — R9.
 *
 * 🔑 Nessun secondo giudizio di visibilita' nel fumetto: il filtro `SourceVerdict` di `BeginPlayback` non fa
 * arrivare l'attivazione a `ShowActivation`, e senza `ShowActivation` non esiste ne' registro ne' vita.
 * ✅ Il controllo positivo e' `SignatureSkillAndBaseAttackBothShout`: stessa scena, viewer della squadra 0.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackHiddenSourceTest,
	"RefactorTactics.Playback.SkillCallout.HiddenSourceDoesNotShout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackHiddenSourceTest::RunTest(const FString&)
{
	FRTFumettoDiProva B;
	const bool bOk = CostruisciFumetto(*this, /*Viewer*/ 7, B);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
	if (!bOk) { return false; }

	B.TM->LockInAndResolve();
	CorriFinoAllaFineDelFumetto(B.TM);

	TestTrue(TEXT("⛔ sorgente non osservata: nessun fumetto registrato"),
		B.Scudo->LastSkillCalloutForTest().ActionId.IsNone());
	TestTrue(TEXT("⛔ e nemmeno per il Tiratore, che con [D-604] griderebbe"),
		B.Tiratore->LastSkillCalloutForTest().ActionId.IsNone());
	TestFalse(TEXT("e nessuna vita"), B.Scudo->IsSkillCalloutLive());
	TestEqual(TEXT("e nessuna unita' accesa nel manager"), B.TM->SkillCalloutUnitsForTest(), 0);
	return true;
}

/**
 * Saltare il playback spegne ogni fumetto — R7.
 *
 * 🔑 `SkipPlayback` passa da `FinishPlayback`, e li' sta lo spegnimento: nessun fumetto sopravvive a un turno
 * che chi guarda ha scelto di non vedere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackSkipClearsTest,
	"RefactorTactics.Playback.SkillCallout.SkipClearsEveryCallout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackSkipClearsTest::RunTest(const FString&)
{
	FRTFumettoDiProva B;
	const bool bOk = CostruisciFumetto(*this, /*Viewer*/ 0, B);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
	if (!bOk) { return false; }

	B.TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ premessa: il fumetto dello Scudo e' vivo prima dello Skip"),
			CorriFinoAlFumettoVivo(B.TM, B.Scudo))
		|| !TestEqual(TEXT("⛔ premessa: ed e' nella lista del manager"), B.TM->SkillCalloutUnitsForTest(), 1))
	{
		return false;
	}

	B.TM->SkipPlayback();
	CorriFinoAllaFineDelFumetto(B.TM);

	TestFalse(TEXT("🔴 dopo lo Skip il fumetto e' spento"), B.Scudo->IsSkillCalloutLive());
	TestEqual(TEXT("e la lista del manager e' vuota"), B.TM->SkillCalloutUnitsForTest(), 0);
	return true;
}

/**
 * In pausa il fumetto non invecchia — R4, A8.
 *
 * 🔑 Il tempo del fumetto e' `SkillCalloutClock`, e `TickPlayback` esce prima di toccarlo quando il playback e'
 * in pausa: non c'e' un ramo da scrivere per la pausa, c'e' un orologio che non viene portato. Un `DeltaTime`
 * locale nel fumetto lo farebbe scorrere comunque, ed e' quello che questo test pinna.
 * ⛔ Seconda meta': alla ripresa l'orologio avanza, o un orologio inerte passerebbe la prima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackPausedClockTest,
	"RefactorTactics.Playback.SkillCallout.PausedClockDoesNotAdvanceTheCallout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackPausedClockTest::RunTest(const FString&)
{
	FRTFumettoDiProva B;
	const bool bOk = CostruisciFumetto(*this, /*Viewer*/ 0, B);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
	if (!bOk) { return false; }

	B.TM->SetPlaybackControlsEnabled(true); // senza, `PausePlayback` e' inerte (fail-closed)
	B.TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ premessa: il fumetto dello Scudo diventa vivo"), CorriFinoAlFumettoVivo(B.TM, B.Scudo)))
	{
		return false;
	}

	const float OrologioPrima = B.Scudo->SkillCalloutClockForTest();
	const float TotalePrima = B.TM->PlaybackSkillCalloutClockForTest();
	B.TM->PausePlayback();
	if (!TestTrue(TEXT("⛔ premessa: il playback e' in pausa"), B.TM->IsPlaybackPaused())) { return false; }

	for (int32 I = 0; I < 10; ++I) { B.TM->Tick(0.05f); }

	TestEqual(TEXT("⛔ premessa: l'orologio del fumetto del manager e' fermo"), B.TM->PlaybackSkillCalloutClockForTest(),
		TotalePrima);
	TestEqual(TEXT("🔴 in pausa l'orologio del fumetto non avanza"), B.Scudo->SkillCalloutClockForTest(), OrologioPrima);
	TestTrue(TEXT("e il fumetto e' ancora vivo: la pausa non lo spegne"), B.Scudo->IsSkillCalloutLive());

	B.TM->ResumePlayback();
	B.TM->Tick(0.05f);

	TestTrue(TEXT("✅ alla ripresa l'orologio del fumetto avanza"), B.Scudo->SkillCalloutClockForTest() > OrologioPrima);
	return true;
}

namespace
{
	/**
	 * La scena della finestra di reazione (modello `RTPresentazioneFinestra::Monta`, `RTMovementResumeTests.cpp`): la
	 * Guardia (squadra 1) in `Action.Overwatch` verso ovest da (3,0), il Mover (squadra 0) che da (0,0) va a (2,0) ed
	 * entra nella sua linea in (1,0), e lo Scudo (squadra 0, lontano dalla linea) che in Prep grida TideGuard.
	 *
	 * ⚠️ Il mondo NON e' inizializzato, come in `ActivationPrefixSurvivesASuspendedBlast`: il viewer e' il ripiego a 0,
	 * la squadra di Scudo e Mover, quindi la loro attivazione e il loro passo arrivano al playback.
	 */
	struct FRTFinestraDelFumetto
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* Scudo = nullptr;
		ARTUnit* Mover = nullptr;
		ARTUnit* Guardia = nullptr;
	};

	bool MontaFinestraDelFumetto(FAutomationTestBase& Test, FRTFinestraDelFumetto& Out)
	{
		Out.World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), Out.World)) { return false; }
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 8);
		ARTHexMapActor* MapActor = Out.World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = M;

		Out.Mover   = SpawnUnitaFumetto(Out.World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0));
		Out.Guardia = SpawnUnitaFumetto(Out.World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(3, 0));
		Out.Scudo   = SpawnUnitaFumetto(Out.World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-3, 3));
		Out.TM = Out.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Out.TM || !Out.Mover || !Out.Guardia || !Out.Scudo) { return false; }

		Out.Guardia->PlannedAbilityIndex =
			RTAbilityFixtures::AddCoreAbilityInSlot(Out.Guardia, TEXT("Action.Overwatch"), 3);
		Out.Guardia->Facing = ERTHexDirection::W;
		Out.Guardia->PlannedCell = Out.Guardia->Cell;
		Out.Mover->PlannedCell = FRTCellId(2, 0);
		Out.Scudo->PlannedCell = Out.Scudo->Cell;
		for (int32 i = 0; i < Out.Scudo->NumAbilities(); ++i)
		{
			if (Out.Scudo->GetAbility(i) && Out.Scudo->GetAbility(i)->Def.ActionId == AzioneFirmaFumetto)
			{
				Out.Scudo->PlannedAbilityIndex = i;
			}
		}
		if (!Test.TestTrue(TEXT("⛔ premessa: la Guardia pianifica l'Overwatch"), Out.Guardia->PlannedAbilityIndex != INDEX_NONE)
			|| !Test.TestTrue(TEXT("⛔ premessa: lo Scudo pianifica TideGuard"),
				Out.Scudo->GetAbility(Out.Scudo->PlannedAbilityIndex) != nullptr
				&& Out.Scudo->GetAbility(Out.Scudo->PlannedAbilityIndex)->Def.ActionId == AzioneFirmaFumetto))
		{
			return false;
		}

		Out.TM->RefreshTeamKnowledgeNow(); // la precondizione della Prep (spec §2.5)
		return true;
	}
}

/**
 * L'orologio del fumetto sopravvive alla finestra di reazione — R4, R7 (final review di #4025, CRITICAL).
 *
 * 🔴 **Il difetto**: `ConcludeResolution` azzera `PlaybackElapsedTotal` anche quando ci si arriva dalla RIPRESA dopo
 * una finestra, e `BeginPlayback(true)` «conserva» un orologio gia' azzerato. Quando il fumetto viveva di quel totale,
 * alla ripresa `Now - StartAt` diventava negativo: opacita' 1 e scala 1.15, cioe' ogni fumetto del turno — anche uno
 * gia' dissolto — tornava pieno e ingrandito sopra la sua unita'. Dedotto dal codice, e nessun test passava di li'.
 *
 * 🔑 Qui: (1) alla presentazione della finestra il fumetto e' vivo e il manager lo tiene (R7: lo spegnimento sta DOPO
 * il ramo sospeso di `FinishPlayback`; spostarlo sopra rende rosso questo punto); durante la finestra l'orologio e'
 * fermo (R4, trattenimento); (2) dopo la risposta, a ogni tick del playback residuo, l'orologio del manager e la vita
 * del fumetto (`clock dell'unita' - StartAt`) non scendono mai sotto il loro valore alla presentazione — che e' dire
 * che l'opacita' non risale e il pop non riparte.
 * ✅ Verificato per mutazione (2026-11-09, una per build): azzerare `SkillCalloutClock` in `ConcludeResolution` rende
 * rosso il punto (2) al primo campione dopo la ripresa (vita -0,458 contro 1,505: opacita' 1, scala 1,15, il difetto
 * della review); spegnere i fumetti in cima a `FinishPlayback`, sopra il ramo sospeso, rende rosso il punto (1).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackClockSurvivesTheWindowTest,
	"RefactorTactics.Playback.SkillCallout.CalloutClockSurvivesTheReactionWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackClockSurvivesTheWindowTest::RunTest(const FString&)
{
	FRTFinestraDelFumetto S;
	const bool bOk = MontaFinestraDelFumetto(*this, S);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
	if (!bOk) { return false; }

	// 🔴 **Senza un ascoltatore la finestra non si apre**: `PumpReactionTriggers` sospende solo se
	// `OnReactionWindowOpened.IsBound()`, altrimenti decide da se' e il turno non si ferma (misurato: la prima stesura
	// di questo test non lo legava, e il Move si risolveva senza finestra). Il delegate e' anche il FOTOGRAMMA della
	// presentazione (`FinishPlayback`, ramo sospeso → `PresentPendingReactionWindow`): lo stato si legge li' dentro.
	// ⚠️ Lo slegamento e' dichiarato DOPO la distruzione del mondo, quindi gira PRIMA (ordine inverso degli scope exit).
	int32 Presentazioni = 0;
	bool bVivoAllaPresentazione = false;
	int32 ListaAllaPresentazione = 0;
	ARTUnit* const Scudo = S.Scudo;
	ARTTurnManager* const TM = S.TM;
	S.TM->OnReactionWindowOpened.BindLambda(
		[&Presentazioni, &bVivoAllaPresentazione, &ListaAllaPresentazione, Scudo, TM](const FRTReactionWindowView&, int32)
		{
			++Presentazioni;
			bVivoAllaPresentazione = Scudo->IsSkillCalloutLive();
			ListaAllaPresentazione = TM->SkillCalloutUnitsForTest();
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

	// --- (1) la presentazione: il fumetto c'e', fermo ---------------------------------------------------------------
	TestTrue(TEXT("🔴 nel fotogramma della presentazione il fumetto dello Scudo e' vivo (R7)"), bVivoAllaPresentazione);
	TestTrue(TEXT("🔴 e il manager lo tiene nella sua lista (R7)"), ListaAllaPresentazione >= 1);
	TestTrue(TEXT("e resta vivo mentre la finestra attende"), S.Scudo->IsSkillCalloutLive());
	const FRTSkillCalloutView Acceso = S.Scudo->LastSkillCalloutForTest();
	if (!TestEqual(TEXT("⛔ premessa: il fumetto e' quello di TideGuard"), Acceso.ActionId, AzioneFirmaFumetto))
	{
		return false;
	}
	const float OrologioAllaFinestra = S.TM->PlaybackSkillCalloutClockForTest();
	const float VitaAllaFinestra = S.Scudo->SkillCalloutClockForTest() - Acceso.StartAt;
	TestEqual(TEXT("l'unita' ha l'orologio del manager"), S.Scudo->SkillCalloutClockForTest(), OrologioAllaFinestra);
	if (!TestTrue(FString::Printf(TEXT("⛔ premessa: il fumetto e' in corsa da prima della finestra (vita %.3f s)"),
			VitaAllaFinestra), VitaAllaFinestra > 0.f))
	{
		return false;
	}
	AddInfo(FString::Printf(TEXT("alla presentazione: orologio %.3f, StartAt %.3f, vita %.3f, opacita' %.2f"),
		OrologioAllaFinestra, Acceso.StartAt, VitaAllaFinestra, URTHudViewModel::SkillCalloutAlpha(VitaAllaFinestra, 3.f)));

	// Il trattenimento ferma l'orologio come la pausa (R4): il giocatore decide su un fumetto fermo.
	for (int32 I = 0; I < 5; ++I) { S.TM->Tick(0.05f); }
	if (!TestTrue(TEXT("⛔ premessa: la finestra attende ancora"), S.TM->IsResolutionSuspended())) { return false; }
	TestEqual(TEXT("durante la finestra l'orologio del fumetto non avanza (R4)"),
		S.TM->PlaybackSkillCalloutClockForTest(), OrologioAllaFinestra);

	// --- (2) la risposta e il playback residuo --------------------------------------------------------------------
	S.TM->SubmitReactionResponse(S.TM->GetOpenReactionWindowId(), URTReactionOpportunityLibrary::HoldResponse());
	if (!TestFalse(TEXT("⛔ premessa: la finestra e' chiusa e il turno ripreso"), S.TM->IsResolutionSuspended())
		|| !TestTrue(TEXT("⛔ premessa: dopo la risposta c'e' un playback residuo (il Mover arriva in (2,0))"),
			S.TM->IsResolving()))
	{
		return false;
	}
	// La misura che rende questo test non vacuo: il totale condiviso e' ripartito da zero alla ripresa.
	AddInfo(FString::Printf(TEXT("alla ripresa: PlaybackElapsedTotal %.3f, orologio del fumetto %.3f"),
		S.TM->PlaybackElapsedTotalForTest(), S.TM->PlaybackSkillCalloutClockForTest()));

	int32 Campioni = 0;
	bool bGiaDetto = false;
	for (int32 I = 0; I < 400 && S.TM->IsResolving(); ++I)
	{
		S.TM->Tick(0.05f);
		++Campioni;
		// L'unita' tiene l'ultimo valore portato anche nel tick che chiude il turno; il manager no — `FinishPlayback`
		// azzera il suo orologio — quindi il suo si legge solo a playback ancora in corso.
		const float Vita = S.Scudo->SkillCalloutClockForTest() - Acceso.StartAt;
		const bool bManagerInCorsa = S.TM->IsResolving();
		const float Orologio = S.TM->PlaybackSkillCalloutClockForTest();
		// Si dice UNA volta e il ciclo continua: un `break` lascerebbe il playback a meta', e gli asserti di fine turno
		// qui sotto cadrebbero per quello invece che per il proprio motivo.
		if (!bGiaDetto && (Vita < VitaAllaFinestra || (bManagerInCorsa && Orologio < OrologioAllaFinestra)))
		{
			bGiaDetto = true;
			AddError(FString::Printf(TEXT("🔴 al campione %d dopo la ripresa il fumetto e' tornato indietro: vita %.3f "
				"(alla finestra %.3f), orologio del manager %.3f (alla finestra %.3f), opacita' %.2f, scala %.2f"),
				Campioni, Vita, VitaAllaFinestra, Orologio, OrologioAllaFinestra,
				URTHudViewModel::SkillCalloutAlpha(Vita, 3.f), URTHudViewModel::SkillCalloutScale(Vita)));
		}
	}
	TestTrue(FString::Printf(TEXT("⛔ premessa: il playback residuo e' stato osservato (%d campioni)"), Campioni),
		Campioni > 0);
	TestFalse(TEXT("a fine turno il fumetto e' spento"), S.Scudo->IsSkillCalloutLive());
	TestEqual(TEXT("e l'orologio del fumetto e' tornato a zero"), S.TM->PlaybackSkillCalloutClockForTest(), 0.f);
	return true;
}

/**
 * L'azione di un pezzo d'equipaggiamento grida — [D-604] (1), R1 della spec di #4059.
 *
 * 🔑 **E' il caso che la regola vecchia escludeva con una seconda domanda**: `MakeEquipmentAction` scrive
 * `ActionId = EquipmentId` e copia dal core un `BaseActionId` vuoto, e `FindCoreAction` non conosce `Gadget.PortableCover`.
 * Le tre premesse sotto lo misurano. Con [D-604] la domanda non serve piu': col loadout di DEFAULT
 * (`EquipLoadout(DefaultLoadoutFor(...))`, come `RTMatchBootstrapper`) Branth grida il suo gadget.
 * ⛔ Nella stessa scena la cue Cast di Branth e' suonata una volta: il fumetto e' quello del gadget.
 * ⏱️ *Fino a #4059 questo test si chiamava `EquipmentActionDoesNotShout` e asseriva il contrario (A7 della spec del
 * fumetto, final review di #4025).*
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackEquipmentShoutsTest,
	"RefactorTactics.Playback.SkillCallout.EquipmentActionShouts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackEquipmentShoutsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 8);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = M;

	// Il viewer e' il ripiego a 0 (mondo non inizializzato), la squadra di Branth.
	const URTHeroData* HeroBranth = URTHeroCatalogLibrary::MakeBranth();
	ARTUnit* Branth = SpawnUnitaFumetto(World, 0, HeroBranth, FRTCellId(0, 0));
	ARTUnit* Nemico = SpawnUnitaFumetto(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Branth || !Nemico || !HeroBranth) { return false; }

	// Il loadout di PRODUZIONE, non un'azione costruita a mano: e' quello con cui Branth entra in partita.
	Branth->EquipLoadout(URTCatalogLibrary::DefaultLoadoutFor(HeroBranth->HeroId));
	const FName Gadget(TEXT("Gadget.PortableCover"));
	int32 Indice = INDEX_NONE;
	for (int32 i = 0; i < Branth->NumAbilities(); ++i)
	{
		if (Branth->GetAbility(i) && Branth->GetAbility(i)->Def.ActionId == Gadget)
		{
			Indice = i;
		}
	}
	if (!TestTrue(TEXT("⛔ premessa: il loadout di default di Branth concede Gadget.PortableCover"), Indice != INDEX_NONE))
	{
		return false;
	}
	// ⛔ Le premesse del caso: base vuota, fuori dal catalogo core, dentro quello d'equipaggiamento.
	if (!TestTrue(TEXT("⛔ premessa: l'azione del gadget non ha una base"),
			Branth->GetAbility(Indice)->Def.BaseActionId.IsNone())
		|| !TestTrue(TEXT("⛔ premessa: e il catalogo core non la conosce"),
			URTCatalogLibrary::FindCoreAction(Gadget).ActionId.IsNone())
		|| !TestTrue(TEXT("⛔ premessa: e' il catalogo d'equipaggiamento a conoscerla"),
			URTCatalogLibrary::FindEquipment(Gadget) != nullptr))
	{
		return false;
	}

	// La copertura su (1,0), bordo W: la forma di `RefactorTactics.Equipment.PortableCover.CreatesCover`.
	Branth->PlannedAbilityIndex = Indice;
	Branth->PlannedAttackTarget = nullptr;
	Branth->PlannedAttackCell = FRTCellId(1, 0);
	Branth->bAttackTargetsCell = true;
	Branth->PlannedCoverEdge = ERTHexDirection::W;
	Branth->bHasPlannedCoverEdge = true;
	Branth->PlannedCell = Branth->Cell;
	Nemico->PlannedCell = Nemico->Cell;

	TM->RefreshTeamKnowledgeNow();
	TM->LockInAndResolve();
	CorriFinoAllaFineDelFumetto(TM);

	TestEqual(TEXT("⛔ controllo positivo: Branth ha attivato il gadget (la cue Cast e' suonata)"),
		Branth->CastCuesPlayedForTest(), 1);
	TestEqual(TEXT("🔴 e l'azione d'equipaggiamento grida, col suo id"), Branth->LastSkillCalloutForTest().ActionId, Gadget);
	TestEqual(TEXT("e il fumetto e' attribuito a Branth"),
		Branth->LastSkillCalloutForTest().SourceStableUnitId, Branth->StableUnitId);
	return true;
}

namespace
{
	/** Le righe d'attivazione del Registro di un'unita' per un'azione. Nome distinto per l'unity build. */
	int32 FumettoRigheDi(const ARTTurnManager* TM, const ARTUnit* U, const FName& Azione)
	{
		int32 N = 0;
		for (const FRTActivationLine& Riga : TM->GetActivationLines())
		{
			if (Riga.SourceStableUnitId == U->StableUnitId && Riga.ActionId == Azione) { ++N; }
		}
		return N;
	}

	/** Un Ivrin senza difese fisse e fermo: la forma di `SpawnDefUnit` in `Tests/Turn/Reaction/RTDefensiveReactionTests.cpp`. */
	ARTUnit* SpawnReattoreFumetto(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeIvrin());
		U->Defense = 0;
		U->RearBonus = 0;
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell;
		U->Shield = 0;
		return U;
	}

	/** Cio' che il playback ha mostrato di una reazione del Blast, tick per tick (#4059, R4). */
	struct FRTFumettoAlColpo
	{
		TArray<int32> Sequenza;            // i `TimelineIndex` della sequenza del Blast, letti appena contiene il colpo
		bool bArrivoVisto = false;         // la traccia dei battiti ha registrato l'arrivo del colpo d'ancora
		bool bVistoVivo = false;           // il fumetto di chi reagisce e' stato vivo
		bool bVivoPrimaDellArrivo = false; // ...in un tick in cui il colpo non era ancora arrivato
	};

	/**
	 * Corre il playback fino alla fine guardando, a ogni tick, il fumetto di `Reattore` e l'arrivo del colpo `IdxColpo`.
	 * 🔑 L'arrivo si legge dalla traccia dei battiti (`bRecordAttackBeatsForTest`: `A<ordinale>`), e l'ordinale e' quello di
	 * `BlastAttackOrdinal`, cioe' i colpi che lo precedono nella sequenza. La sequenza la costruisce il playback, quindi la
	 * si legge al primo tick in cui contiene il colpo.
	 */
	FRTFumettoAlColpo CorriGuardandoIlFumettoAlColpo(ARTTurnManager* TM, const ARTUnit* Reattore, int32 IdxColpo)
	{
		FRTFumettoAlColpo Out;
		FString Arrivo;
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
			if (Arrivo.IsEmpty())
			{
				const TArray<int32> Sequenza = TM->PlaybackBlastSequenceIndicesForTest();
				const int32 Pos = Sequenza.IndexOfByKey(IdxColpo);
				if (Pos != INDEX_NONE)
				{
					Out.Sequenza = Sequenza;
					const TArray<FRTResolvedEvent>& T = TM->ResolvedTimelineForTest();
					int32 Ordinale = 0;
					for (int32 K = 0; K < Pos; ++K)
					{
						if (T.IsValidIndex(Sequenza[K]) && T[Sequenza[K]].Type == ERTResolvedEventType::Attack) { ++Ordinale; }
					}
					Arrivo = FString::Printf(TEXT("A%d"), Ordinale);
				}
			}
			const bool bArrivato = !Arrivo.IsEmpty() && TM->AttackBeatTraceForTest().Contains(Arrivo);
			Out.bArrivoVisto |= bArrivato;
			if (Reattore->IsSkillCalloutLive())
			{
				Out.bVistoVivo = true;
				Out.bVivoPrimaDellArrivo |= !bArrivato;
			}
		}
		return Out;
	}

	/** L'indice nella timeline del primo evento di quel tipo con quella sorgente (e quel bersaglio, se non `0`). */
	int32 FumettoIndiceDi(const ARTTurnManager* TM, ERTResolvedEventType Type, int32 Sorgente, int32 Bersaglio = 0)
	{
		return TM->ResolvedTimelineForTest().IndexOfByPredicate([Type, Sorgente, Bersaglio](const FRTResolvedEvent& E)
		{
			return E.Type == Type && E.Phase == ERTMatchPhase::Blast && E.SourceStableUnitId == Sorgente
				&& (Bersaglio == 0 || E.TargetStableUnitId == Bersaglio);
		});
	}
}

/**
 * L'interazione non grida, e ha la sua riga — [D-604] (1), R1 della spec di #4059.
 *
 * 🔑 E' l'unica azione muta che arriva al playback come attivazione: `Action.Interact` risolve nel Blast
 * (`ERTResolutionPhase::Attack`), mentre `Move`, `Sprint` e `Withdraw` risolvono nel Move, ed `EmitAttackIntentActivation`
 * salta quella fase. La scena e' quella di `Structures.Door.InteractFromKitOpensDoor`: una porta chiusa sul bordo E di (0,0).
 * ⛔ Due controlli nella stessa scena: la cue Cast e' suonata e la riga d'attivazione c'e', quindi l'attivazione e' arrivata
 * a `ShowActivation` e l'assenza di fumetto e' `ActionShouts`, non un'attivazione persa. E' anche il contrasto che
 * `Playback.ActivationLine.EveryActionKindHasALine` non ha piu': la riga non dipende dal fumetto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackInteractIsSilentTest,
	"RefactorTactics.Playback.SkillCallout.InteractDoesNotShout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackInteractIsSilentTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 8);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = M;

	// Porta CHIUSA sul bordo E di (0,0): la portata di `Interact` e' 1 ([D-149]), e (1,0) e' la cella oltre la porta.
	const FRTCellId Cardine(0, 0);
	const FRTCellId Oltre(1, 0);
	const FRTHexCellData* Dati = M->FindCell(Cardine);
	if (!TestNotNull(TEXT("⛔ premessa: la cella della porta esiste"), Dati)) { return false; }
	FRTHexCellData ConPorta = *Dati;
	ConPorta.Doors.Add(FRTHexDoor(ERTHexDirection::E, ERTHexDoorState::Closed));
	M->AddOrUpdateCell(ConPorta);
	M->SortCells();
	if (!TestTrue(TEXT("⛔ premessa: all'inizio la porta blocca il passaggio"),
			URTHexCoverLibrary::BlocksTraversal(M, Cardine, Oltre)))
	{
		return false;
	}

	// Il viewer e' il ripiego a 0 (mondo non inizializzato), la squadra di chi apre.
	ARTUnit* Apre = SpawnUnitaFumetto(World, 0, URTHeroCatalogLibrary::MakeAevik(), Cardine);
	ARTUnit* Nemico = SpawnUnitaFumetto(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Apre || !Nemico) { return false; }

	const FName Interact(TEXT("Action.Interact"));
	int32 Indice = INDEX_NONE;
	for (int32 i = 0; i < Apre->NumAbilities(); ++i)
	{
		if (Apre->GetAbility(i) && Apre->GetAbility(i)->Def.ActionId == Interact) { Indice = i; }
	}
	if (!TestTrue(TEXT("⛔ premessa: Aevik porta `Action.Interact` nel kit"), Indice != INDEX_NONE)) { return false; }
	Apre->PlannedAbilityIndex = Indice;
	Apre->PlannedAttackTarget = nullptr;
	Apre->PlannedAttackCell = Oltre;
	Apre->bAttackTargetsCell = true; // si mira a una CELLA: dietro la porta non c'e' nessuno
	Apre->PlannedCell = Apre->Cell;
	Nemico->PlannedCell = Nemico->Cell;

	TM->RefreshTeamKnowledgeNow();
	TM->LockInAndResolve();
	CorriFinoAllaFineDelFumetto(TM);

	if (!TestFalse(TEXT("⛔ premessa: la porta si e' aperta, quindi l'interazione e' avvenuta"),
			URTHexCoverLibrary::BlocksTraversal(M, Cardine, Oltre)))
	{
		return false;
	}
	TestEqual(TEXT("⛔ controllo: l'interazione si e' attivata nel playback (la cue Cast e' suonata)"),
		Apre->CastCuesPlayedForTest(), 1);
	TestEqual(TEXT("⛔ controllo: e ha la sua riga d'attivazione"), FumettoRigheDi(TM, Apre, Interact), 1);
	TestTrue(TEXT("🔴 ma l'interazione non grida"), Apre->LastSkillCalloutForTest().ActionId.IsNone());
	return true;
}

/**
 * Una reazione del Blast grida all'arrivo del colpo che l'ha innescata — R4 della spec di #4059, [D-605] (1).
 *
 * 🔑 La scena di `Reactions.Counter.DealsDamageToAttacker`: il Reattore (squadra 0, quella di chi guarda) arma
 * `Action.Counter`, e l'Attaccante (squadra 1) gli spara col suo attacco base. Nella timeline il `ReactionResolved` nasce
 * PRIMA del colpo, perche' il passo delle reazioni gira sui colpi raccolti; nella sequenza del Blast sta subito DOPO.
 * 🔴 A ogni tick: il fumetto del Reattore non e' mai vivo prima che la traccia registri l'arrivo del colpo, e si accende.
 * ⛔ Le premesse misurano i due campi che fino a #4059 l'evento non portava: chi ha innescato (R4) e il verdetto (R3). Il
 * caso in cui chi ha innescato non e' il bersaglio dell'evento e' `InterposeShoutsWhenTheRedirectedHitArrives`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackReactionAtTheHitTest,
	"RefactorTactics.Playback.SkillCallout.ReactionShoutsWhenTheHitArrives",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackReactionAtTheHitTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 6);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = M;

	// Il viewer e' il ripiego a 0 (mondo non inizializzato): la squadra del Reattore.
	ARTUnit* Reattore = SpawnReattoreFumetto(World, 0, FRTCellId(0, 0));
	ARTUnit* Attaccante = SpawnReattoreFumetto(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Reattore || !Attaccante) { return false; }

	const FName Counter(TEXT("Action.Counter"));
	Reattore->PlannedReactionAbility = RTAbilityFixtures::AddCoreAbilityInSlot(Reattore, TEXT("Action.Counter"), 3);
	Reattore->PlannedAbilityIndex = INDEX_NONE;
	Attaccante->PlannedAbilityIndex = 0; // l'attacco base, colpo singolo
	Attaccante->PlannedAttackTarget = Reattore;
	TM->bRecordAttackBeatsForTest = true;

	TM->RefreshTeamKnowledgeNow();
	TM->LockInAndResolve();

	const int32 IdxReazione = FumettoIndiceDi(TM, ERTResolvedEventType::ReactionResolved, Reattore->StableUnitId);
	const int32 IdxColpo = FumettoIndiceDi(TM, ERTResolvedEventType::Attack, Attaccante->StableUnitId, Reattore->StableUnitId);
	if (!TestTrue(TEXT("⛔ premessa: il Contrattacco e' scattato (c'e' il suo ReactionResolved)"), IdxReazione != INDEX_NONE)
		|| !TestTrue(TEXT("⛔ premessa: e c'e' il colpo dell'Attaccante sul Reattore"), IdxColpo != INDEX_NONE))
	{
		return false;
	}
	const FRTResolvedEvent Reazione = TM->ResolvedTimelineForTest()[IdxReazione];
	TestEqual(TEXT("🔴 (R4) l'evento nomina chi ha innescato"), Reazione.TriggerAttackerStableUnitId, Attaccante->StableUnitId);
	TestTrue(TEXT("🔴 (R3) e porta un verdetto che lascia vedere la reazione alla squadra del Reattore"),
		Reazione.SourceVerdict.AllowsTeam(0));
	AddInfo(FString::Printf(TEXT("timeline: reazione %d, colpo %d"), IdxReazione, IdxColpo));

	const FRTFumettoAlColpo Visto = CorriGuardandoIlFumettoAlColpo(TM, Reattore, IdxColpo);
	const int32 PosColpo = Visto.Sequenza.IndexOfByKey(IdxColpo);
	if (!TestTrue(TEXT("⛔ premessa: la sequenza del Blast contiene il colpo, e il suo arrivo e' stato visto"),
			PosColpo != INDEX_NONE && Visto.bArrivoVisto))
	{
		return false;
	}
	TestEqual(TEXT("🔴 (R4) nella sequenza la reazione segue SUBITO il colpo che l'ha innescata"),
		Visto.Sequenza.IndexOfByKey(IdxReazione), PosColpo + 1);
	TestTrue(TEXT("⛔ premessa: il fumetto del Reattore si e' acceso durante il playback"), Visto.bVistoVivo);
	TestFalse(TEXT("🔴 e mai prima che il colpo arrivasse"), Visto.bVivoPrimaDellArrivo);
	TestEqual(TEXT("🔴 il fumetto e' quello del Contrattacco"), Reattore->LastSkillCalloutForTest().ActionId, Counter);
	TestEqual(TEXT("e il Reattore ha la sua riga d'attivazione, una"), FumettoRigheDi(TM, Reattore, Counter), 1);
	return true;
}

/**
 * Un'interposizione grida all'arrivo del colpo deviato, e chi ha innescato NON e' il bersaglio dell'evento — R4.
 *
 * 🔑 La scena di `Reactions.Intercept`: il Salvatore (squadra 0) si mette davanti all'Alleato e prende il colpo
 * dell'Attaccante. L'evento nasce in `ResolveInterceptions`, non nel passo delle reazioni, e il suo `TargetStableUnitId` e'
 * l'Alleato protetto: e' per questo che chi ha innescato ha un campo suo. L'ancora e' il colpo dell'Attaccante sul
 * Salvatore, il bersaglio FINALE.
 * ⚠️ In questa scena un'ancora sbagliata ripiegherebbe sul primo colpo sul Salvatore, che e' lo stesso: a distinguere i due
 * casi e' `Playback.BlastSequenceReactionFollowsTheHitOfItsTrigger`, con due attaccanti. Qui si misura l'evento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackInterposeAtTheHitTest,
	"RefactorTactics.Playback.SkillCallout.InterposeShoutsWhenTheRedirectedHitArrives",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackInterposeAtTheHitTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 6);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = M;

	// Il viewer e' il ripiego a 0 (mondo non inizializzato): la squadra del Salvatore.
	ARTUnit* Attaccante = SpawnReattoreFumetto(World, 1, FRTCellId(4, 0));
	ARTUnit* Alleato = SpawnReattoreFumetto(World, 0, FRTCellId(0, 0));
	ARTUnit* Salvatore = SpawnReattoreFumetto(World, 0, FRTCellId(1, 0)); // adiacente all'Alleato: entro le 2 celle
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Alleato || !Salvatore) { return false; }
	Attaccante->Facing = ERTHexDirection::W; // si spara a cio' che si vede (CP 13.2)

	const FName Intercept(TEXT("Action.Intercept"));
	Salvatore->PlannedReactionAbility = RTAbilityFixtures::AddCoreAbilityInSlot(Salvatore, TEXT("Action.Intercept"), 3);
	Salvatore->PlannedAbilityIndex = INDEX_NONE;
	Attaccante->PlannedAbilityIndex = 0; // l'attacco base, colpo singolo
	Attaccante->PlannedAttackTarget = Alleato;
	TM->bRecordAttackBeatsForTest = true;

	TM->RefreshTeamKnowledgeNow();
	TM->LockInAndResolve();

	const int32 IdxReazione = FumettoIndiceDi(TM, ERTResolvedEventType::ReactionResolved, Salvatore->StableUnitId);
	const int32 IdxColpo = FumettoIndiceDi(TM, ERTResolvedEventType::Attack, Attaccante->StableUnitId, Salvatore->StableUnitId);
	if (!TestTrue(TEXT("⛔ premessa: l'interposizione e' scattata (c'e' il suo ReactionResolved)"), IdxReazione != INDEX_NONE)
		|| !TestTrue(TEXT("⛔ premessa: e c'e' il colpo dell'Attaccante sul Salvatore, il bersaglio finale"),
			IdxColpo != INDEX_NONE))
	{
		return false;
	}
	const FRTResolvedEvent Reazione = TM->ResolvedTimelineForTest()[IdxReazione];
	TestEqual(TEXT("🔴 (R4) l'evento nomina chi ha innescato: l'Attaccante"),
		Reazione.TriggerAttackerStableUnitId, Attaccante->StableUnitId);
	TestEqual(TEXT("⛔ e il suo bersaglio resta l'Alleato protetto"), Reazione.TargetStableUnitId, Alleato->StableUnitId);
	TestTrue(TEXT("🔴 (R3) e porta un verdetto che lascia vedere la reazione alla squadra del Salvatore"),
		Reazione.SourceVerdict.AllowsTeam(0));

	const FRTFumettoAlColpo Visto = CorriGuardandoIlFumettoAlColpo(TM, Salvatore, IdxColpo);
	const int32 PosColpo = Visto.Sequenza.IndexOfByKey(IdxColpo);
	if (!TestTrue(TEXT("⛔ premessa: la sequenza del Blast contiene il colpo, e il suo arrivo e' stato visto"),
			PosColpo != INDEX_NONE && Visto.bArrivoVisto))
	{
		return false;
	}
	TestEqual(TEXT("🔴 (R4) nella sequenza l'interposizione segue SUBITO il colpo deviato"),
		Visto.Sequenza.IndexOfByKey(IdxReazione), PosColpo + 1);
	TestTrue(TEXT("⛔ premessa: il fumetto del Salvatore si e' acceso durante il playback"), Visto.bVistoVivo);
	TestFalse(TEXT("🔴 e mai prima che il colpo arrivasse"), Visto.bVivoPrimaDellArrivo);
	TestEqual(TEXT("🔴 il fumetto e' quello dell'interposizione"), Salvatore->LastSkillCalloutForTest().ActionId, Intercept);
	TestEqual(TEXT("e il Salvatore ha la sua riga d'attivazione, una"), FumettoRigheDi(TM, Salvatore, Intercept), 1);
	return true;
}

/**
 * La reazione di un nemico che chi guarda non vede non grida e non ha riga — R3 della spec di #4059.
 *
 * 🔑 L'Attaccante (squadra 0) guarda a ovest e spara col suo attacco base a una CELLA alle sue spalle, a tre celle: fuori
 * dal suo arco frontale e oltre la consapevolezza ravvicinata (`URTPerceptionLibrary::CloseAwarenessRange`). Li' c'e' il
 * Reattore (squadra 1) col Contrattacco armato: la squadra 0 non lo vede, il colpo lo raggiunge e la reazione scatta.
 * ⛔ Il verdetto e' quello della PERCEZIONE, non il fail-closed di una squadra assente: la squadra 0 e' in campo, e la
 * premessa lo misura sull'evento. ✅ Controllo positivo: lo stesso turno visto dalla squadra 1 ha fumetto e riga.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackHiddenReactorTest,
	"RefactorTactics.Playback.SkillCallout.HiddenReactorDoesNotShout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackHiddenReactorTest::RunTest(const FString&)
{
	struct FRTGiroDelReattore
	{
		bool bScattata = false;
		bool bVerdettoSquadra0 = true;
		bool bVerdettoSquadra1 = false;
		bool bNellaSequenza = false;
		FName Fumetto;
		int32 Righe = INDEX_NONE;
	};
	const FName Counter(TEXT("Action.Counter"));

	const auto Gira = [this, &Counter](int32 SquadraDelViewer, FRTGiroDelReattore& Out) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		World->InitializeActorsForPlay(FURL()); // il viewer deve esistere per il TurnManager
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 6);
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = M;

		ARTUnit* Attaccante = SpawnReattoreFumetto(World, 0, FRTCellId(0, 0));
		ARTUnit* Reattore = SpawnReattoreFumetto(World, 1, FRTCellId(3, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTPlayerController* Viewer = RTWorldFixtures::MakePlayerOnTeam(World, SquadraDelViewer);
		if (!TM || !Attaccante || !Reattore || !Viewer) { return false; }
		if (!TestEqual(TEXT("⛔ premessa: il TurnManager legge la squadra del viewer della fixture"),
				ARTPlayerState::TeamIdOf(UGameplayStatics::GetPlayerController(TM, 0)), SquadraDelViewer))
		{
			return false;
		}

		Attaccante->Facing = ERTHexDirection::W; // le spalle al Reattore, che sta a est
		Attaccante->PlannedAbilityIndex = 0;     // l'attacco base
		Attaccante->PlannedAttackTarget = nullptr;
		Attaccante->PlannedAttackCell = Reattore->Cell;
		Attaccante->bAttackTargetsCell = true;   // una CELLA: un'unita' che la squadra non vede non si puo' mirare
		Reattore->Facing = ERTHexDirection::W;
		Reattore->PlannedReactionAbility = RTAbilityFixtures::AddCoreAbilityInSlot(Reattore, TEXT("Action.Counter"), 3);
		Reattore->PlannedAbilityIndex = INDEX_NONE;

		TM->RefreshTeamKnowledgeNow();
		TM->LockInAndResolve();
		const int32 IdxReazione = FumettoIndiceDi(TM, ERTResolvedEventType::ReactionResolved, Reattore->StableUnitId);
		Out.bScattata = (IdxReazione != INDEX_NONE);
		if (Out.bScattata)
		{
			const FRTResolvedEvent Reazione = TM->ResolvedTimelineForTest()[IdxReazione];
			Out.bVerdettoSquadra0 = Reazione.SourceVerdict.AllowsTeam(0);
			Out.bVerdettoSquadra1 = Reazione.SourceVerdict.AllowsTeam(1);
		}
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
			if (Out.bScattata && TM->PlaybackBlastSequenceIndicesForTest().Contains(IdxReazione)) { Out.bNellaSequenza = true; }
		}
		if (!TestFalse(TEXT("⛔ premessa: il playback e' finito"), TM->IsResolving())) { return false; }
		Out.Fumetto = Reattore->LastSkillCalloutForTest().ActionId;
		Out.Righe = FumettoRigheDi(TM, Reattore, Counter);
		return true;
	};

	FRTGiroDelReattore Squadra0;
	FRTGiroDelReattore Squadra1;
	if (!Gira(0, Squadra0) || !Gira(1, Squadra1)) { return false; }
	if (!TestTrue(TEXT("⛔ premessa: il Contrattacco e' scattato (il colpo alla cella lo ha raggiunto)"),
			Squadra0.bScattata && Squadra1.bScattata)
		|| !TestFalse(TEXT("⛔ premessa: il verdetto della reazione non la lascia vedere alla squadra 0"),
			Squadra0.bVerdettoSquadra0)
		|| !TestTrue(TEXT("⛔ premessa: e la lascia vedere alla squadra 1, quella di chi reagisce"),
			Squadra1.bVerdettoSquadra1))
	{
		return false;
	}
	TestFalse(TEXT("🔴 per la squadra 0 la reazione non entra nella sequenza del Blast"), Squadra0.bNellaSequenza);
	TestTrue(TEXT("🔴 non grida"), Squadra0.Fumetto.IsNone());
	TestEqual(TEXT("🔴 e non ha riga"), Squadra0.Righe, 0);
	TestTrue(TEXT("✅ per la squadra 1 la reazione e' nella sequenza"), Squadra1.bNellaSequenza);
	TestEqual(TEXT("✅ grida il Contrattacco"), Squadra1.Fumetto, Counter);
	TestEqual(TEXT("✅ e ha la sua riga"), Squadra1.Righe, 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
