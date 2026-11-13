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
// Tiratore (Riktor) pianifica l'indice 0, `Hero.Riktor.ImpactShot`: una variante dell'attacco base. Con [D-604] gridano
// entrambi; fino a #4059 gridava solo il primo.
// ⚠️ Altre scene, ciascuna col suo montaggio: la finestra di reazione (l'orologio del fumetto sopravvive alla ripresa,
// R4/R7), il loadout di Riktor (l'equipaggiamento grida), l'interazione (l'azione del Blast che non grida) e le reazioni
// del Blast (#4059: il fumetto all'arrivo del colpo, e niente per chi guarda senza vedere chi reagisce).

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/Format/RTMatchSetupLibrary.h"
#include "Turn/Reaction/RTReactionOpportunityTypes.h" // HoldResponse: la risposta alla finestra che non spara
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Ability/Catalog/RTCatalogLibrary.h" // DefaultLoadoutFor / FindEquipment / FindCoreAction: il loadout vero di Riktor
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
#include "Map/RTHexLibrary.h"                // #4059: la cella da cui il bersaglio dell'Overwatch entra nella linea
#include "Ability/RTEquipmentData.h"             // #4059: `Reaction.HazardEscape`, il modulo della fuga dal pericolo

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
		Out.Tiratore = SpawnUnitaFumetto(Out.World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0));
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
				Out.Scudo->SetPlannedMainAction(i);
			}
		}
		Out.Scudo->SetPlannedCell(Out.Scudo->Cell);
		Out.Tiratore->SetPlannedMainAction(0);
		Out.Tiratore->DeclareAttackOnUnit(Bersaglio);
		Out.Tiratore->SetPlannedCell(Out.Tiratore->Cell);

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
 * 🔑 Il Tiratore pianifica `Hero.Riktor.ImpactShot`, una variante dell'attacco base: fino a #4059 restava muto, e ora il
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
		B.Tiratore->LastSkillCalloutForTest().ActionId, FName(TEXT("Hero.Riktor.ImpactShot")));
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

		Out.Guardia->SetPlannedMainAction(RTAbilityFixtures::AddCoreAbilityInSlot(Out.Guardia, TEXT("Action.Overwatch"), 3));
		Out.Guardia->Facing = ERTHexDirection::W;
		Out.Guardia->SetPlannedCell(Out.Guardia->Cell);
		Out.Mover->SetPlannedCell(FRTCellId(2, 0));
		Out.Scudo->SetPlannedCell(Out.Scudo->Cell);
		for (int32 i = 0; i < Out.Scudo->NumAbilities(); ++i)
		{
			if (Out.Scudo->GetAbility(i) && Out.Scudo->GetAbility(i)->Def.ActionId == AzioneFirmaFumetto)
			{
				Out.Scudo->SetPlannedMainAction(i);
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
 * (`EquipLoadout(DefaultLoadoutFor(...))`, come `RTMatchBootstrapper`) Riktor grida il suo gadget.
 * ⛔ Nella stessa scena la cue Cast di Riktor e' suonata una volta: il fumetto e' quello del gadget.
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

	// Il viewer e' il ripiego a 0 (mondo non inizializzato), la squadra di Riktor.
	const URTHeroData* HeroRiktor = URTHeroCatalogLibrary::MakeRiktor();
	ARTUnit* Riktor = SpawnUnitaFumetto(World, 0, HeroRiktor, FRTCellId(0, 0));
	ARTUnit* Nemico = SpawnUnitaFumetto(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Riktor || !Nemico || !HeroRiktor) { return false; }

	// Il loadout di PRODUZIONE, non un'azione costruita a mano: e' quello con cui Riktor entra in partita.
	Riktor->EquipLoadout(URTCatalogLibrary::DefaultLoadoutFor(HeroRiktor->HeroId));
	const FName Gadget(TEXT("Gadget.PortableCover"));
	int32 Indice = INDEX_NONE;
	for (int32 i = 0; i < Riktor->NumAbilities(); ++i)
	{
		if (Riktor->GetAbility(i) && Riktor->GetAbility(i)->Def.ActionId == Gadget)
		{
			Indice = i;
		}
	}
	if (!TestTrue(TEXT("⛔ premessa: il loadout di default di Riktor concede Gadget.PortableCover"), Indice != INDEX_NONE))
	{
		return false;
	}
	// ⛔ Le premesse del caso: base vuota, fuori dal catalogo core, dentro quello d'equipaggiamento.
	if (!TestTrue(TEXT("⛔ premessa: l'azione del gadget non ha una base"),
			Riktor->GetAbility(Indice)->Def.BaseActionId.IsNone())
		|| !TestTrue(TEXT("⛔ premessa: e il catalogo core non la conosce"),
			URTCatalogLibrary::FindCoreAction(Gadget).ActionId.IsNone())
		|| !TestTrue(TEXT("⛔ premessa: e' il catalogo d'equipaggiamento a conoscerla"),
			URTCatalogLibrary::FindEquipment(Gadget) != nullptr))
	{
		return false;
	}

	// La copertura su (1,0), bordo W: la forma di `RefactorTactics.Equipment.PortableCover.CreatesCover`.
	Riktor->SetPlannedMainAction(Indice);
	Riktor->DeclareAttackOnCell(FRTCellId(1, 0));
	Riktor->DeclarePlannedCoverEdge(ERTHexDirection::W);
	Riktor->SetPlannedCell(Riktor->Cell);
	Nemico->SetPlannedCell(Nemico->Cell);

	TM->RefreshTeamKnowledgeNow();
	TM->LockInAndResolve();
	CorriFinoAllaFineDelFumetto(TM);

	TestEqual(TEXT("⛔ controllo positivo: Riktor ha attivato il gadget (la cue Cast e' suonata)"),
		Riktor->CastCuesPlayedForTest(), 1);
	TestEqual(TEXT("🔴 e l'azione d'equipaggiamento grida, col suo id"), Riktor->LastSkillCalloutForTest().ActionId, Gadget);
	TestEqual(TEXT("e il fumetto e' attribuito a Riktor"),
		Riktor->LastSkillCalloutForTest().SourceStableUnitId, Riktor->StableUnitId);
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
		U->SetPlannedCell(Cell);
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
		int32 TickPrimoVivo = INDEX_NONE;  // il primo tick in cui il fumetto di chi reagisce e' vivo
		int32 TickPrimaRiga = INDEX_NONE;  // il primo tick in cui c'e' la sua riga d'attivazione
	};

	/**
	 * Corre il playback fino alla fine guardando, a ogni tick, il fumetto di `Reattore` e l'arrivo del colpo `IdxColpo`.
	 * 🔑 L'arrivo si legge dalla traccia dei battiti (`bRecordAttackBeatsForTest`: `A<ordinale>`), e l'ordinale e' quello di
	 * `BlastAttackOrdinal`, cioe' i colpi che lo precedono nella sequenza. La sequenza la costruisce il playback, quindi la
	 * si legge al primo tick in cui contiene il colpo.
	 * #4059: e il primo tick del fumetto e quello della riga d'attivazione di `Azione`, che devono coincidere (R8).
	 */
	FRTFumettoAlColpo CorriGuardandoIlFumettoAlColpo(ARTTurnManager* TM, const ARTUnit* Reattore, int32 IdxColpo,
		const FName& Azione)
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
				if (Out.TickPrimoVivo == INDEX_NONE) { Out.TickPrimoVivo = I; }
			}
			if (Out.TickPrimaRiga == INDEX_NONE && FumettoRigheDi(TM, Reattore, Azione) > 0) { Out.TickPrimaRiga = I; }
		}
		return Out;
	}

	/** L'indice nella timeline del primo evento di quel tipo e quella fase, con quella sorgente (e quel bersaglio, se non `0`). */
	int32 FumettoIndiceDi(const ARTTurnManager* TM, ERTResolvedEventType Type, int32 Sorgente, int32 Bersaglio = 0,
		ERTMatchPhase Fase = ERTMatchPhase::Blast)
	{
		return TM->ResolvedTimelineForTest().IndexOfByPredicate([Type, Sorgente, Bersaglio, Fase](const FRTResolvedEvent& E)
		{
			return E.Type == Type && E.Phase == Fase && E.SourceStableUnitId == Sorgente
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
	Apre->SetPlannedMainAction(Indice);
	Apre->DeclareAttackOnCell(Oltre); // si mira a una CELLA: dietro la porta non c'e' nessuno
	Apre->SetPlannedCell(Apre->Cell);
	Nemico->SetPlannedCell(Nemico->Cell);

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
 * 🔴 A ogni tick: il fumetto del Reattore non e' mai vivo prima che la traccia registri l'arrivo del colpo, e si accende
 * nello stesso tick in cui nasce la sua riga d'attivazione (il criterio della issue).
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
	Reattore->SetPlannedReaction(RTAbilityFixtures::AddCoreAbilityInSlot(Reattore, TEXT("Action.Counter"), 3));
	Reattore->ClearPlannedMainAction();
	Attaccante->SetPlannedMainAction(0); // l'attacco base, colpo singolo
	Attaccante->DeclareAttackOnUnit(Reattore);
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

	const FRTFumettoAlColpo Visto = CorriGuardandoIlFumettoAlColpo(TM, Reattore, IdxColpo, Counter);
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
	// #4059, criterio della issue: fumetto e riga nascono nello STESSO tick (R8: li accende la stessa `ShowReaction`). Il
	// Reattore non ha altre attivazioni, quindi il suo primo fumetto vivo e' quello del Contrattacco.
	TestTrue(TEXT("⛔ premessa: il primo tick del fumetto e quello della riga sono stati osservati"),
		Visto.TickPrimoVivo != INDEX_NONE && Visto.TickPrimaRiga != INDEX_NONE);
	TestEqual(TEXT("🔴 (R8) il fumetto e la riga del Contrattacco nascono nello stesso tick"),
		Visto.TickPrimaRiga, Visto.TickPrimoVivo);
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
	Salvatore->SetPlannedReaction(RTAbilityFixtures::AddCoreAbilityInSlot(Salvatore, TEXT("Action.Intercept"), 3));
	Salvatore->ClearPlannedMainAction();
	Attaccante->SetPlannedMainAction(0); // l'attacco base, colpo singolo
	Attaccante->DeclareAttackOnUnit(Alleato);
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

	const FRTFumettoAlColpo Visto = CorriGuardandoIlFumettoAlColpo(TM, Salvatore, IdxColpo, Intercept);
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
	TestEqual(TEXT("🔴 (R8) e il fumetto e la riga dell'interposizione nascono nello stesso tick"),
		Visto.TickPrimaRiga, Visto.TickPrimoVivo);
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
		Attaccante->SetPlannedMainAction(0);     // l'attacco base
		Attaccante->DeclareAttackOnCell(Reattore->Cell); // una CELLA: un'unita' che la squadra non vede non si puo' mirare
		Reattore->Facing = ERTHexDirection::W;
		Reattore->SetPlannedReaction(RTAbilityFixtures::AddCoreAbilityInSlot(Reattore, TEXT("Action.Counter"), 3));
		Reattore->ClearPlannedMainAction();

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

/**
 * La risposta del Brace grida dopo la finestra, come primo elemento oltre il prefisso gia' mostrato — R5 e R4 della spec
 * di #4059, [D-605] (2).
 *
 * 🔑 La scena di `Reactions.Brace.WindowSuspendsBlast`: il Bracer (squadra 0, quella di chi guarda) col profilo
 * `Profile.Sidestep` e il Pusher (squadra 1) che lo spinge. La finestra si presenta a prefisso mostrato per intero
 * (`#3666`): la spinta e' gia' a schermo, e la risposta non e' ancora decisa. Alla ripresa nasce il `ReactionResolved`
 * del Brace, ancorato al colpo di chi spinge, che sta nel prefisso.
 * 🔴 Alla presentazione non c'e' nessuna riga del Brace in fase Blast (c'e' quella di Prep, dove il Brace si e' attivato);
 * dopo la ripresa la riga arriva quando il playback ha rivelato UN elemento oltre il prefisso, e il fumetto e' nuovo.
 * ⛔ E l'evento nasce una volta sola, anche se il Blast si e' sospeso e ripreso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackBraceAfterWindowTest,
	"RefactorTactics.Playback.SkillCallout.BraceResponseShoutsAfterTheWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackBraceAfterWindowTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 8);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = M;

	// Il viewer e' il ripiego a 0 (mondo non inizializzato): la squadra del Bracer.
	ARTUnit* Bracer = SpawnReattoreFumetto(World, 0, FRTCellId(0, 0));
	ARTUnit* Pusher = SpawnReattoreFumetto(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bracer || !Pusher) { return false; }

	const FName Brace(TEXT("Action.Brace"));
	Bracer->ReactionProfileId = TEXT("Profile.Sidestep"); // due risposte: la finestra si apre
	Bracer->SetPlannedMainAction(RTAbilityFixtures::AddCoreAbilityInSlot(Bracer, TEXT("Action.Brace"), 3));
	Pusher->SetPlannedMainAction(RTAbilityFixtures::AddCoreAbilityInSlot(Pusher, TEXT("Action.Push"), 3));
	Pusher->DeclareAttackOnUnit(Bracer);

	const auto RigheDelBraceNelBlast = [TM, Bracer, &Brace]()
	{
		int32 N = 0;
		for (const FRTActivationLine& Riga : TM->GetActivationLines())
		{
			if (Riga.SourceStableUnitId == Bracer->StableUnitId && Riga.ActionId == Brace
				&& Riga.Phase == ERTMatchPhase::Blast)
			{
				++N;
			}
		}
		return N;
	};

	int32 Presentazioni = 0;
	int32 RigheAllaPresentazione = INDEX_NONE;
	TM->OnReactionWindowOpened.BindLambda([&Presentazioni, &RigheAllaPresentazione, &RigheDelBraceNelBlast](
		const FRTReactionWindowView&, int32)
	{
		++Presentazioni;
		RigheAllaPresentazione = RigheDelBraceNelBlast();
	});
	ON_SCOPE_EXIT{ TM->OnReactionWindowOpened.Unbind(); };

	TM->RefreshTeamKnowledgeNow();
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ premessa: la resolution e' sospesa sulla finestra del Brace"), TM->IsResolutionSuspended())
		|| !TestTrue(TEXT("⛔ premessa: la finestra si presenta pompando il playback"),
			RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM))
		|| !TestEqual(TEXT("⛔ premessa: presentata una volta"), Presentazioni, 1))
	{
		return false;
	}
	TestEqual(TEXT("🔴 alla presentazione nessuna riga del Brace in fase Blast: la risposta non e' ancora decisa"),
		RigheAllaPresentazione, 0);
	const int32 Mostrati = TM->PlaybackBlastShownForTest();
	const float FumettoPrimaStartAt = Bracer->LastSkillCalloutForTest().StartAt;

	for (int32 Scadenze = 0; Scadenze < 8 && TM->IsResolutionSuspended(); ++Scadenze)
	{
		TM->ExpireReactionWindow();
	}
	if (!TestFalse(TEXT("⛔ premessa: la finestra e' chiusa e il turno ripreso"), TM->IsResolutionSuspended())) { return false; }

	const int32 IdxReazione = FumettoIndiceDi(TM, ERTResolvedEventType::ReactionResolved, Bracer->StableUnitId);
	if (!TestTrue(TEXT("⛔ premessa: alla ripresa e' nato il ReactionResolved del Brace"), IdxReazione != INDEX_NONE))
	{
		return false;
	}
	const FRTResolvedEvent Reazione = TM->ResolvedTimelineForTest()[IdxReazione];
	TestEqual(TEXT("🔴 (R5) l'azione e' il Brace"), Reazione.ActionId, Brace);
	TestEqual(TEXT("🔴 (R4) e nomina chi spinge"), Reazione.TriggerAttackerStableUnitId, Pusher->StableUnitId);
	TestTrue(TEXT("🔴 (R3) e porta un verdetto che la lascia vedere alla squadra del Bracer"),
		Reazione.SourceVerdict.AllowsTeam(0));

	int32 MostratiAllaRiga = INDEX_NONE;
	for (int32 I = 0; I < 400 && TM->IsResolving() && MostratiAllaRiga == INDEX_NONE; ++I)
	{
		TM->Tick(0.05f);
		if (RigheDelBraceNelBlast() > 0) { MostratiAllaRiga = TM->PlaybackBlastShownForTest(); }
	}
	if (!TestTrue(TEXT("🔴 dopo la ripresa la riga del Brace in fase Blast arriva durante il playback"),
			MostratiAllaRiga != INDEX_NONE))
	{
		return false;
	}
	TestEqual(TEXT("🔴 (R4) ed e' il primo elemento oltre il prefisso mostrato prima della finestra"),
		MostratiAllaRiga, Mostrati + 1);
	TestEqual(TEXT("e il fumetto del Bracer e' quello del Brace"), Bracer->LastSkillCalloutForTest().ActionId, Brace);
	TestTrue(TEXT("🔴 ed e' un fumetto NUOVO, non quello dell'attivazione in Prep"),
		Bracer->LastSkillCalloutForTest().StartAt > FumettoPrimaStartAt);

	CorriFinoAllaFineDelFumetto(TM);
	int32 Reazioni = 0;
	for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
	{
		if (Ev.Type == ERTResolvedEventType::ReactionResolved && Ev.SourceStableUnitId == Bracer->StableUnitId) { ++Reazioni; }
	}
	TestEqual(TEXT("⛔ l'evento del Brace nasce una volta sola, anche col Blast sospeso e ripreso"), Reazioni, 1);
	TestEqual(TEXT("e la riga del Brace in fase Blast e' una"), RigheDelBraceNelBlast(), 1);
	return true;
}

namespace
{
	/** La scena del colpo dell'Overwatch (#4059, R6): la geometria di `BuildRxCellStage` in `RTReactionMicroStepCellTests.cpp`. */
	struct FRTOverwatchDelFumetto
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* Watcher = nullptr; // squadra 1, quella di chi guarda: guarda a ovest da (3,0)
		ARTUnit* Mover = nullptr;   // squadra 0: entra nella linea in (1,0) con un passo
		ARTUnit* Runner = nullptr;  // squadra 1, lontano: allunga la fase Move, quando c'e'
	};

	bool MontaOverwatchDelFumetto(FAutomationTestBase& Test, int32 VistaDelWatcher, bool bConCorridore,
		FRTOverwatchDelFumetto& Out, int32 SquadraDelViewer = 1, ERTHexDirection FacciaDelMover = ERTHexDirection::E)
	{
		Out.World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), Out.World)) { return false; }
		Out.World->InitializeActorsForPlay(FURL()); // il viewer della squadra 1 deve esistere per il TurnManager
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 6);
		ARTHexMapActor* MapActor = Out.World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = M;

		const FRTCellId Destinazione(1, 0, 0);                                                  // dentro la linea guardata
		const FRTCellId Partenza = URTHexLibrary::Neighbor(Destinazione, ERTHexDirection::SW); // fuori, adiacente
		Out.Watcher = SpawnReattoreFumetto(Out.World, 1, FRTCellId(3, 0, 0));
		Out.Mover = SpawnReattoreFumetto(Out.World, 0, Partenza);
		if (bConCorridore) { Out.Runner = SpawnReattoreFumetto(Out.World, 1, FRTCellId(-3, -3, 0)); }
		Out.TM = Out.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTPlayerController* Viewer = RTWorldFixtures::MakePlayerOnTeam(Out.World, SquadraDelViewer);
		if (!Out.TM || !Out.Watcher || !Out.Mover || !Viewer || (bConCorridore && !Out.Runner)) { return false; }
		if (!Test.TestEqual(TEXT("⛔ premessa: il TurnManager legge la squadra di chi guarda"),
				ARTPlayerState::TeamIdOf(UGameplayStatics::GetPlayerController(Out.TM, 0)), SquadraDelViewer))
		{
			return false;
		}

		Out.Watcher->Facing = ERTHexDirection::W;
		Out.Watcher->VisionRange = VistaDelWatcher;
		Out.Watcher->SetPlannedMainAction(RTAbilityFixtures::AddCoreAbilityInSlot(Out.Watcher, TEXT("Action.Overwatch"), 3));
		Out.Mover->Facing = FacciaDelMover;
		Out.Mover->SetPlannedCell(Destinazione);
		if (Out.Runner) { Out.Runner->SetPlannedCell(FRTCellId(2, -3, 0)); }
		if (!Test.TestTrue(TEXT("⛔ premessa: il watcher arma l'Overwatch"), Out.Watcher->PlannedAbilityIndex != INDEX_NONE))
		{
			return false;
		}
		// Il watcher spara alla prima finestra che glielo consente: la forma di `BindRxCellFireDecider`.
		Out.TM->ReactionDecider.BindLambda([](const FRTReactionOpportunity& Opportunity, int32) -> FString
		{
			for (const FString& Response : Opportunity.AllowedResponses)
			{
				if (URTReactionOpportunityLibrary::FireResponseTarget(Response) != INDEX_NONE) { return Response; }
			}
			return FString();
		});
		Out.TM->RefreshTeamKnowledgeNow();
		return true;
	}

	/** Le righe del Registro del watcher per l'Overwatch nella fase Move: quella del COLPO (l'armamento e' in Prep). */
	int32 RigheDelColpoDiOverwatch(const ARTTurnManager* TM, const ARTUnit* Watcher)
	{
		int32 N = 0;
		for (const FRTActivationLine& Riga : TM->GetActivationLines())
		{
			if (Riga.SourceStableUnitId == Watcher->StableUnitId && Riga.ActionId == FName(TEXT("Action.Overwatch"))
				&& Riga.Phase == ERTMatchPhase::Move)
			{
				++N;
			}
		}
		return N;
	}
}

/**
 * Il colpo dell'Overwatch grida quando il bersaglio arriva sulla cella del colpo — R6 della spec di #4059, [D-605] (2).
 *
 * 🔑 La geometria di `Reactions.VerdictFreezesOnTheImpactCell`, con la vista piena: il Mover (squadra 0) entra con un passo
 * nella linea del watcher (squadra 1, quella di chi guarda) e il watcher spara. Un Runner della squadra 1 corre lontano e
 * allunga la fase Move, cosi' il fumetto nasce DENTRO la fase e non dalla rete di fine fase.
 * 🔴 La riga del colpo (fase Move) arriva nel tick in cui l'animazione del Mover raggiunge la cella del colpo, e il
 * fumetto del watcher e' nuovo: l'Overwatch aveva gia' gridato in Prep, quando si e' armato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackOverwatchAtTheCellTest,
	"RefactorTactics.Playback.SkillCallout.OverwatchShotShoutsAtTheTargetCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackOverwatchAtTheCellTest::RunTest(const FString&)
{
	FRTOverwatchDelFumetto S;
	const bool bOk = MontaOverwatchDelFumetto(*this, /*Vista*/ 6, /*bConCorridore*/ true, S);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
	if (!bOk) { return false; }

	S.TM->LockInAndResolve();
	const FName Overwatch(TEXT("Action.Overwatch"));
	const int32 IdxReazione = FumettoIndiceDi(S.TM, ERTResolvedEventType::ReactionResolved, S.Watcher->StableUnitId,
		S.Mover->StableUnitId, ERTMatchPhase::Move);
	if (!TestTrue(TEXT("⛔ premessa: l'Overwatch ha sparato (c'e' il suo ReactionResolved, in fase Move)"),
			IdxReazione != INDEX_NONE))
	{
		return false;
	}
	const FRTResolvedEvent Reazione = S.TM->ResolvedTimelineForTest()[IdxReazione];
	TestEqual(TEXT("🔴 (R6) l'evento porta la cella del colpo nella rotta del bersaglio: dopo un passo"),
		Reazione.ReactionRouteCellIndex, 1);
	TestEqual(TEXT("(R6) e l'azione armata"), Reazione.ActionId, Overwatch);
	TestTrue(TEXT("🔴 (R3) e un verdetto che lascia vedere il colpo alla squadra del watcher"),
		Reazione.SourceVerdict.AllowsTeam(1));

	bool bAnimVista = false;
	int32 CellaAllaRiga = INDEX_NONE;
	ERTMatchPhase FaseAllaRiga = ERTMatchPhase::Planning;
	float StartAtPrima = -1.f;
	for (int32 I = 0; I < 400 && S.TM->IsResolving(); ++I)
	{
		S.TM->Tick(0.05f);
		const int32 Cella = S.TM->PlaybackAnimCellIndexForTest(S.Mover);
		if (S.TM->CurrentPlaybackPhaseForTest() == ERTMatchPhase::Move && Cella != INDEX_NONE) { bAnimVista = true; }
		if (RigheDelColpoDiOverwatch(S.TM, S.Watcher) == 0)
		{
			StartAtPrima = S.Watcher->LastSkillCalloutForTest().StartAt;
		}
		else if (FaseAllaRiga == ERTMatchPhase::Planning)
		{
			CellaAllaRiga = Cella;
			FaseAllaRiga = S.TM->CurrentPlaybackPhaseForTest();
		}
	}
	if (!TestTrue(TEXT("⛔ premessa: il Mover ha un'animazione visibile nella fase Move"), bAnimVista)
		|| !TestEqual(TEXT("⛔ premessa: la riga del colpo e' nata dentro la fase Move, non dalla rete o dal recupero"),
			FaseAllaRiga, ERTMatchPhase::Move))
	{
		return false;
	}
	TestEqual(TEXT("🔴 (R6) la riga del colpo nasce quando il Mover arriva sulla cella del colpo"),
		CellaAllaRiga, Reazione.ReactionRouteCellIndex);
	TestEqual(TEXT("🔴 e il fumetto del watcher e' quello dell'Overwatch"), S.Watcher->LastSkillCalloutForTest().ActionId,
		Overwatch);
	TestTrue(TEXT("🔴 ed e' NUOVO: non quello dell'armamento in Prep"),
		S.Watcher->LastSkillCalloutForTest().StartAt > StartAtPrima);
	TestEqual(TEXT("e la riga del colpo e' una"), RigheDelColpoDiOverwatch(S.TM, S.Watcher), 1);
	return true;
}

/**
 * Il colpo dell'Overwatch grida anche quando il bersaglio non ha un'animazione visibile — R6 della spec di #4059.
 *
 * 🔑 La stessa scena con la vista del watcher ridotta a due celle (`Reactions.VerdictFreezesOnTheImpactCell`): il Mover
 * parte da una cella che la squadra 1 non vede, quindi il prefisso osservato della sua rotta e' vuoto e il playback non lo
 * anima. Il colpo, sulla cella in vista, la squadra 1 lo vede. Senza il Runner nessuna rotta visibile apre la fase Move:
 * la apre la reazione.
 * 🔴 Il fumetto del watcher nasce nel playback — e' nuovo rispetto a quello dell'armamento, e il recupero di fine playback
 * non accende fumetti — e la riga del colpo c'e', in fase Move.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackOverwatchUnseenTargetTest,
	"RefactorTactics.Playback.SkillCallout.OverwatchShotShoutsForAnUnseenTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackOverwatchUnseenTargetTest::RunTest(const FString&)
{
	FRTOverwatchDelFumetto S;
	const bool bOk = MontaOverwatchDelFumetto(*this, /*Vista*/ 2, /*bConCorridore*/ false, S);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
	if (!bOk) { return false; }

	S.TM->LockInAndResolve();
	const FName Overwatch(TEXT("Action.Overwatch"));
	const int32 IdxReazione = FumettoIndiceDi(S.TM, ERTResolvedEventType::ReactionResolved, S.Watcher->StableUnitId,
		S.Mover->StableUnitId, ERTMatchPhase::Move);
	if (!TestTrue(TEXT("⛔ premessa: l'Overwatch ha sparato (c'e' il suo ReactionResolved, in fase Move)"),
			IdxReazione != INDEX_NONE)
		|| !TestTrue(TEXT("⛔ premessa: e la squadra del watcher puo' vederlo"),
			S.TM->ResolvedTimelineForTest()[IdxReazione].SourceVerdict.AllowsTeam(1)))
	{
		return false;
	}

	bool bAnimVista = false;
	bool bFaseMoveVista = false;
	bool bVivoDopoLaRiga = false;
	int32 TickDellaRiga = INDEX_NONE;
	float StartAtPrima = -1.f;
	for (int32 I = 0; I < 400 && S.TM->IsResolving(); ++I)
	{
		S.TM->Tick(0.05f);
		if (S.TM->PlaybackAnimCellIndexForTest(S.Mover) != INDEX_NONE) { bAnimVista = true; }
		if (S.TM->CurrentPlaybackPhaseForTest() == ERTMatchPhase::Move) { bFaseMoveVista = true; }
		if (RigheDelColpoDiOverwatch(S.TM, S.Watcher) == 0)
		{
			StartAtPrima = S.Watcher->LastSkillCalloutForTest().StartAt;
		}
		else if (TickDellaRiga == INDEX_NONE)
		{
			TickDellaRiga = I;
		}
		else if (S.TM->IsResolving() && S.Watcher->IsSkillCalloutLive())
		{
			bVivoDopoLaRiga = true;
		}
	}
	if (!TestFalse(TEXT("⛔ premessa: il Mover non ha un'animazione visibile a chi guarda"), bAnimVista))
	{
		return false;
	}
	AddInfo(FString::Printf(TEXT("fase Move osservata a fine tick: %s"), bFaseMoveVista ? TEXT("si'") : TEXT("no")));
	TestEqual(TEXT("🔴 (R6) la riga del colpo c'e', in fase Move"), RigheDelColpoDiOverwatch(S.TM, S.Watcher), 1);
	TestEqual(TEXT("🔴 e il fumetto del watcher e' quello dell'Overwatch"), S.Watcher->LastSkillCalloutForTest().ActionId,
		Overwatch);
	TestTrue(TEXT("🔴 ed e' nato nel playback: e' nuovo rispetto all'armamento, e il recupero non accende fumetti"),
		S.Watcher->LastSkillCalloutForTest().StartAt > StartAtPrima);
	// Review di #4066: il colpo e' l'unico fatto della fase, e la fase non deve chiudersi nel tick in cui nasce.
	TestTrue(TEXT("🔴 e resta vivo nei tick dopo, a playback in corso: la fase Move gli lascia il tempo di lettura"),
		bVivoDopoLaRiga);
	return true;
}

namespace
{
	/** La scena della fuga dal pericolo (#4059, R7): quella di `Equipment.HazardEscape.FleesBeforeDamage`. */
	struct FRTFugaDelFumetto
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* Fuggitivo = nullptr; // squadra 1, quella di chi guarda: il fuoco nasce sotto di lui
		ARTUnit* Corridore = nullptr; // squadra 1, lontano: c'e' un movimento, e la fuga viene dopo
	};

	const FName FugaDelFumetto(TEXT("Reaction.HazardEscape"));

	bool MontaFugaDelFumetto(FAutomationTestBase& Test, FRTFugaDelFumetto& Out, int32 SquadraDelViewer = 1)
	{
		const URTEquipmentData* Modulo = nullptr;
		for (const URTEquipmentData* M : URTCatalogLibrary::MakeReactionModules())
		{
			if (M && M->EquipmentId == FugaDelFumetto) { Modulo = M; break; }
		}
		if (!Test.TestNotNull(TEXT("⛔ premessa: `Reaction.HazardEscape` e' nel catalogo dei moduli"), Modulo)) { return false; }

		Out.World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), Out.World)) { return false; }
		Out.World->InitializeActorsForPlay(FURL()); // il viewer della squadra 1 deve esistere per il TurnManager
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 6);
		ARTHexMapActor* MapActor = Out.World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = M;

		ARTUnit* Incendiario = SpawnReattoreFumetto(Out.World, 0, FRTCellId(0, 0));
		Out.Fuggitivo = SpawnReattoreFumetto(Out.World, 1, FRTCellId(2, 0));
		Out.Corridore = SpawnReattoreFumetto(Out.World, 1, FRTCellId(-3, -3, 0));
		Out.TM = Out.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTPlayerController* Viewer = RTWorldFixtures::MakePlayerOnTeam(Out.World, SquadraDelViewer);
		if (!Out.TM || !Incendiario || !Out.Fuggitivo || !Out.Corridore || !Viewer) { return false; }
		if (!Test.TestEqual(TEXT("⛔ premessa: il TurnManager legge la squadra di chi guarda"),
				ARTPlayerState::TeamIdOf(UGameplayStatics::GetPlayerController(Out.TM, 0)), SquadraDelViewer))
		{
			return false;
		}

		URTActionData* Reazione = URTCatalogLibrary::MakeEquipmentAction(Modulo, Out.Fuggitivo);
		if (!Test.TestNotNull(TEXT("⛔ premessa: il modulo concede un'azione"), Reazione)) { return false; }
		Out.Fuggitivo->Abilities.Add(Reazione);
		Out.Fuggitivo->SetPlannedReaction(Out.Fuggitivo->Abilities.Num() - 1);
		Out.Fuggitivo->Facing = ERTHexDirection::W; // fugge verso (1,0), la cella che ha davanti
		Incendiario->SetPlannedMainAction(RTAbilityFixtures::AddCoreAbilityInSlot(Incendiario, TEXT("Action.Ignite"), 3));
		Incendiario->DeclareAttackOnUnit(Out.Fuggitivo);
		Out.Corridore->SetPlannedCell(FRTCellId(-1, -3, 0));
		Out.TM->RefreshTeamKnowledgeNow();
		return true;
	}

	/** Le righe del Registro del Fuggitivo per la fuga nella fase Cleanup. */
	int32 RigheDellaFuga(const ARTTurnManager* TM, const ARTUnit* Fuggitivo)
	{
		int32 N = 0;
		for (const FRTActivationLine& Riga : TM->GetActivationLines())
		{
			if (Riga.SourceStableUnitId == Fuggitivo->StableUnitId && Riga.ActionId == FugaDelFumetto
				&& Riga.Phase == ERTMatchPhase::Cleanup)
			{
				++N;
			}
		}
		return N;
	}
}

/**
 * La fuga dal pericolo grida nel Cleanup, dopo il movimento, e la sua rotta parte dopo il fumetto — R7 della spec di #4059,
 * [D-605] (3) «Al momento giusto, dopo».
 *
 * 🔑 La scena di `Equipment.HazardEscape.FleesBeforeDamage`: l'Incendiario (squadra 0) accende il fuoco sotto il Fuggitivo
 * (squadra 1, quella di chi guarda), che col modulo fugge nel Cleanup. Il Corridore della squadra 1 si muove: la fase Move
 * c'e', e la fuga deve venire DOPO.
 * 🔴 Gli eventi della fuga sono in fase Cleanup, la voce di TurnLog no (resta Blast: la fase entra nell'hash). La riga della
 * fuga nasce nella fase Cleanup, che il playback mostra dopo il Move, e il Fuggitivo comincia a correre solo dopo la riga:
 * prima le reazioni, poi le rotte, come nel Dash.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackHazardEscapeTest,
	"RefactorTactics.Playback.SkillCallout.HazardEscapeShoutsAfterTheMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackHazardEscapeTest::RunTest(const FString&)
{
	FRTFugaDelFumetto S;
	const bool bOk = MontaFugaDelFumetto(*this, S);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
	if (!bOk) { return false; }

	S.TM->LockInAndResolve();
	const int32 IdxReazione = FumettoIndiceDi(S.TM, ERTResolvedEventType::ReactionResolved, S.Fuggitivo->StableUnitId, 0,
		ERTMatchPhase::Cleanup);
	const int32 IdxRotta = FumettoIndiceDi(S.TM, ERTResolvedEventType::Move, S.Fuggitivo->StableUnitId, 0,
		ERTMatchPhase::Cleanup);
	TestTrue(TEXT("🔴 (R7) l'evento della reazione della fuga e' in fase Cleanup"), IdxReazione != INDEX_NONE);
	TestTrue(TEXT("🔴 (R7) e anche quello della sua rotta"), IdxRotta != INDEX_NONE);
	if (IdxReazione == INDEX_NONE || IdxRotta == INDEX_NONE) { return false; }
	TestEqual(TEXT("e la reazione e' la fuga"), S.TM->ResolvedTimelineForTest()[IdxReazione].ActionId, FugaDelFumetto);
	const FRTTurnLogEntry* Voce = S.TM->GetTurnLog().FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Reaction && E.ActionId == FugaDelFumetto
			&& E.Outcome == static_cast<uint8>(ERTReactionOutcome::Activated);
	});
	if (TestNotNull(TEXT("⛔ premessa: la voce di TurnLog della fuga c'e'"), Voce))
	{
		TestEqual(TEXT("⛔ ([D-605] (4)(b)) e resta in fase Blast: la fase entra nell'hash del TurnLog"),
			Voce->Phase, ERTMatchPhase::Blast);
	}

	TArray<ERTMatchPhase> Fasi;
	ERTMatchPhase FaseAllaRiga = ERTMatchPhase::Planning;
	bool bCorsaDopoLaRiga = false;
	bool bCorsaPrimaDellaRiga = false;
	for (int32 I = 0; I < 400 && S.TM->IsResolving(); ++I)
	{
		S.TM->Tick(0.05f);
		const ERTMatchPhase Fase = S.TM->CurrentPlaybackPhaseForTest();
		if (Fasi.Num() == 0 || Fasi.Last() != Fase) { Fasi.Add(Fase); }
		const bool bRiga = RigheDellaFuga(S.TM, S.Fuggitivo) > 0;
		if (bRiga && FaseAllaRiga == ERTMatchPhase::Planning) { FaseAllaRiga = Fase; }
		if (Fase == ERTMatchPhase::Cleanup && S.Fuggitivo->bIsMovingVisually)
		{
			(bRiga ? bCorsaDopoLaRiga : bCorsaPrimaDellaRiga) = true;
		}
	}
	const int32 PosMove = Fasi.IndexOfByKey(ERTMatchPhase::Move);
	const int32 PosCleanup = Fasi.IndexOfByKey(ERTMatchPhase::Cleanup);
	if (!TestTrue(TEXT("⛔ premessa: il playback ha mostrato il Move"), PosMove != INDEX_NONE)) { return false; }
	TestTrue(TEXT("🔴 (R7) e poi il Cleanup, dopo il Move"), PosCleanup != INDEX_NONE && PosCleanup > PosMove);
	TestEqual(TEXT("🔴 la riga della fuga nasce nella fase Cleanup"), FaseAllaRiga, ERTMatchPhase::Cleanup);
	TestTrue(TEXT("🔴 il Fuggitivo corre nel Cleanup"), bCorsaDopoLaRiga);
	TestFalse(TEXT("🔴 e non prima del fumetto: prima le reazioni, poi le rotte"), bCorsaPrimaDellaRiga);
	TestEqual(TEXT("e il fumetto e' quello della fuga"), S.Fuggitivo->LastSkillCalloutForTest().ActionId, FugaDelFumetto);
	TestEqual(TEXT("e la riga della fuga e' una"), RigheDellaFuga(S.TM, S.Fuggitivo), 1);
	return true;
}

/**
 * Il salto lascia la riga di ogni reazione scattata, in ogni fase e una volta sola — R9 della spec di #4059.
 *
 * 🔑 Tre scene, una per coda: il Contrattacco (Blast), il colpo dell'Overwatch (Move), la fuga dal pericolo (Cleanup). Il
 * Contrattacco e la fuga si saltano subito: la riga la da' il recupero di fine playback, che non accende fumetti. L'Overwatch
 * si salta DOPO che il colpo e' stato mostrato: il recupero non deve dare un doppione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackSkipKeepsReactionLinesTest,
	"RefactorTactics.Playback.SkillCallout.SkipKeepsTheReactionLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackSkipKeepsReactionLinesTest::RunTest(const FString&)
{
	// --- (1) Blast: il Contrattacco, saltato subito ---------------------------------------------------------------------
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
		Reattore->SetPlannedReaction(RTAbilityFixtures::AddCoreAbilityInSlot(Reattore, TEXT("Action.Counter"), 3));
		Attaccante->SetPlannedMainAction(0);
		Attaccante->DeclareAttackOnUnit(Reattore);
		TM->RefreshTeamKnowledgeNow();
		TM->LockInAndResolve();
		if (!TestTrue(TEXT("⛔ premessa (Blast): il Contrattacco e' scattato"),
				FumettoIndiceDi(TM, ERTResolvedEventType::ReactionResolved, Reattore->StableUnitId) != INDEX_NONE)
			|| !TestTrue(TEXT("⛔ premessa (Blast): il playback e' in corso"), TM->IsResolving()))
		{
			return false;
		}
		TM->SkipPlayback();
		CorriFinoAllaFineDelFumetto(TM);
		TestEqual(TEXT("🔴 (Blast) dopo il salto la riga del Contrattacco c'e', una"), FumettoRigheDi(TM, Reattore, Counter), 1);
		TestTrue(TEXT("e il salto non accende il fumetto"), Reattore->LastSkillCalloutForTest().ActionId.IsNone());
	}

	// --- (2) Move: l'Overwatch, saltato dopo che il colpo e' stato mostrato --------------------------------------------
	{
		FRTOverwatchDelFumetto S;
		const bool bOk = MontaOverwatchDelFumetto(*this, /*Vista*/ 6, /*bConCorridore*/ true, S);
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
		if (!bOk) { return false; }
		S.TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && S.TM->IsResolving() && RigheDelColpoDiOverwatch(S.TM, S.Watcher) == 0; ++I)
		{
			S.TM->Tick(0.05f);
		}
		if (!TestEqual(TEXT("⛔ premessa (Move): il colpo e' stato mostrato"), RigheDelColpoDiOverwatch(S.TM, S.Watcher), 1)
			|| !TestTrue(TEXT("⛔ premessa (Move): e il playback e' ancora in corso"), S.TM->IsResolving()))
		{
			return false;
		}
		S.TM->SkipPlayback();
		CorriFinoAllaFineDelFumetto(S.TM);
		TestEqual(TEXT("🔴 (Move) dopo il salto la riga del colpo e' ancora una: niente doppioni"),
			RigheDelColpoDiOverwatch(S.TM, S.Watcher), 1);
	}

	// --- (3) Cleanup: la fuga, saltata subito ---------------------------------------------------------------------------
	{
		FRTFugaDelFumetto S;
		const bool bOk = MontaFugaDelFumetto(*this, S);
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
		if (!bOk) { return false; }
		S.TM->LockInAndResolve();
		if (!TestTrue(TEXT("⛔ premessa (Cleanup): il playback e' in corso"), S.TM->IsResolving())) { return false; }
		S.TM->SkipPlayback();
		CorriFinoAllaFineDelFumetto(S.TM);
		TestEqual(TEXT("🔴 (Cleanup) dopo il salto la riga della fuga c'e', una"), RigheDellaFuga(S.TM, S.Fuggitivo), 1);
	}
	return true;
}

/**
 * Il colpo di un watcher che chi guarda non vede non grida e non ha riga — R3 per la coda del Move (review di #4066).
 *
 * 🔑 La scena di `OverwatchShotShoutsAtTheTargetCell`, vista dalla squadra del Mover (0), che guarda a ovest: il watcher
 * sta a tre celle alle sue spalle, oltre la consapevolezza ravvicinata, e la squadra 0 non lo vede. Il colpo c'e' lo
 * stesso, ma il suo verdetto non ammette la squadra 0. ✅ Il controllo positivo e' quel test, con la squadra del watcher.
 * ⛔ Il verdetto della percezione, misurato sull'evento, e non il ripiego di una squadra assente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackHiddenWatcherTest,
	"RefactorTactics.Playback.SkillCallout.HiddenWatcherDoesNotShout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackHiddenWatcherTest::RunTest(const FString&)
{
	FRTOverwatchDelFumetto S;
	const bool bOk = MontaOverwatchDelFumetto(*this, /*Vista*/ 6, /*bConCorridore*/ false, S, /*Viewer*/ 0,
		/*Mover*/ ERTHexDirection::W);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
	if (!bOk) { return false; }

	S.TM->LockInAndResolve();
	const int32 IdxReazione = FumettoIndiceDi(S.TM, ERTResolvedEventType::ReactionResolved, S.Watcher->StableUnitId,
		S.Mover->StableUnitId, ERTMatchPhase::Move);
	if (!TestTrue(TEXT("⛔ premessa: l'Overwatch ha sparato"), IdxReazione != INDEX_NONE)) { return false; }
	const FRTResolvedEvent Reazione = S.TM->ResolvedTimelineForTest()[IdxReazione];
	if (!TestFalse(TEXT("⛔ premessa: il verdetto del colpo non lo lascia vedere alla squadra del Mover"),
			Reazione.SourceVerdict.AllowsTeam(0))
		|| !TestTrue(TEXT("⛔ premessa: e lo lascia vedere alla squadra del watcher"), Reazione.SourceVerdict.AllowsTeam(1)))
	{
		return false;
	}

	bool bFumettoNelMove = false;
	float StartAtPrimaDelMove = S.Watcher->LastSkillCalloutForTest().StartAt;
	for (int32 I = 0; I < 400 && S.TM->IsResolving(); ++I)
	{
		S.TM->Tick(0.05f);
		if (S.TM->CurrentPlaybackPhaseForTest() != ERTMatchPhase::Move)
		{
			StartAtPrimaDelMove = S.Watcher->LastSkillCalloutForTest().StartAt;
		}
		else if (S.Watcher->LastSkillCalloutForTest().StartAt != StartAtPrimaDelMove)
		{
			bFumettoNelMove = true;
		}
	}
	TestEqual(TEXT("🔴 il colpo del watcher non visto non ha riga"), RigheDelColpoDiOverwatch(S.TM, S.Watcher), 0);
	TestFalse(TEXT("🔴 e nella fase Move non nasce nessun suo fumetto"), bFumettoNelMove);
	return true;
}

/**
 * La fuga di un'unita' che chi guarda non ha il diritto di vedere non grida, non ha riga e non apre il Cleanup — R3 per la
 * coda del Cleanup (review di #4066).
 *
 * 🔑 La scena di `HazardEscapeShoutsAfterTheMove`, vista da una squadra senza unita' in campo (7). ⚠️ Qui il verdetto e' il
 * ripiego di una squadra assente, non la percezione: e' il FILTRO della coda che si misura (`SourceVerdict.AllowsTeam` in
 * `BeginPlayback`), che senza questo test potrebbe sparire senza che niente diventi rosso. ✅ Il controllo positivo e'
 * `HazardEscapeShoutsAfterTheMove`, con la squadra del Fuggitivo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackHiddenFugitiveTest,
	"RefactorTactics.Playback.SkillCallout.HiddenFugitiveDoesNotShout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackHiddenFugitiveTest::RunTest(const FString&)
{
	FRTFugaDelFumetto S;
	const bool bOk = MontaFugaDelFumetto(*this, S, /*Viewer*/ 7);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(S.World); };
	if (!bOk) { return false; }

	S.TM->LockInAndResolve();
	const int32 IdxReazione = FumettoIndiceDi(S.TM, ERTResolvedEventType::ReactionResolved, S.Fuggitivo->StableUnitId, 0,
		ERTMatchPhase::Cleanup);
	if (!TestTrue(TEXT("⛔ premessa: la fuga e' scattata, in fase Cleanup"), IdxReazione != INDEX_NONE)
		|| !TestFalse(TEXT("⛔ premessa: e il suo verdetto non ammette la squadra 7"),
			S.TM->ResolvedTimelineForTest()[IdxReazione].SourceVerdict.AllowsTeam(7)))
	{
		return false;
	}
	bool bCleanupVisto = false;
	for (int32 I = 0; I < 400 && S.TM->IsResolving(); ++I)
	{
		S.TM->Tick(0.05f);
		if (S.TM->CurrentPlaybackPhaseForTest() == ERTMatchPhase::Cleanup) { bCleanupVisto = true; }
	}
	TestEqual(TEXT("🔴 la fuga non ha riga"), RigheDellaFuga(S.TM, S.Fuggitivo), 0);
	TestTrue(TEXT("🔴 e non grida"), S.Fuggitivo->LastSkillCalloutForTest().ActionId != FugaDelFumetto);
	TestFalse(TEXT("🔴 e la fase Cleanup del playback non si apre per lei"), bCleanupVisto);
	return true;
}

/**
 * Un colpo dell'Overwatch mostrato PRIMA di una finestra di reazione non si rimostra alla ripresa — R6 (review di #4066).
 *
 * 🔑 Due watcher della squadra 1, quella di chi guarda. Il primo e' del bot e spara subito al Mover A, che entra nella sua
 * linea al primo passo. Il secondo e' umano: il Mover B arriva sulla sua linea non prima del secondo passo, e la finestra
 * sospende il Move. Il playback parziale mostra il colpo del primo; alla ripresa `BeginPlayback(true)` ricostruisce la coda
 * dalla timeline, e quel colpo c'e' ancora, gia' passato. ⛔ Lo tiene fuori la chiave del gia' mostrato
 * (`PlaybackReactionsMoveShown`), non l'indice di timeline, che `EmitMoveEvents` sposta a ogni ripresa.
 * 🔴 Dopo la ripresa il fumetto del primo watcher non rinasce: il suo `StartAt` non cambia piu'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackMoveReactionNotRepeatedTest,
	"RefactorTactics.Playback.SkillCallout.MoveReactionShownBeforeAWindowIsNotRepeated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackMoveReactionNotRepeatedTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	World->InitializeActorsForPlay(FURL()); // il viewer della squadra 1 deve esistere per il TurnManager
	URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 6);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = M;

	const FRTCellId DestinazioneA(1, 0, 0); // dentro la linea del primo watcher, al primo passo
	ARTUnit* Watcher1 = SpawnReattoreFumetto(World, 1, FRTCellId(3, 0, 0));
	ARTUnit* Watcher2 = SpawnReattoreFumetto(World, 1, FRTCellId(2, -3, 0));
	ARTUnit* MoverA = SpawnReattoreFumetto(World, 0, URTHexLibrary::Neighbor(DestinazioneA, ERTHexDirection::SW));
	ARTUnit* MoverB = SpawnReattoreFumetto(World, 0, FRTCellId(-3, -1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTPlayerController* Viewer = RTWorldFixtures::MakePlayerOnTeam(World, 1);
	if (!TM || !Watcher1 || !Watcher2 || !MoverA || !MoverB || !Viewer) { return false; }
	if (!TestEqual(TEXT("⛔ premessa: il TurnManager legge la squadra dei watcher"),
			ARTPlayerState::TeamIdOf(UGameplayStatics::GetPlayerController(TM, 0)), 1))
	{
		return false;
	}
	for (ARTUnit* W : { Watcher1, Watcher2 })
	{
		W->Facing = ERTHexDirection::W;
		W->SetPlannedMainAction(RTAbilityFixtures::AddCoreAbilityInSlot(W, TEXT("Action.Overwatch"), 3));
	}
	Watcher1->bIsBotControlled = true; // decide subito, e spara
	MoverA->Facing = ERTHexDirection::E;
	MoverA->SetPlannedCell(DestinazioneA);
	MoverB->Facing = ERTHexDirection::E;
	MoverB->SetPlannedCell(FRTCellId(0, -3, 0)); // la riga r = -3 dista almeno due passi da r = -1

	int32 Presentazioni = 0;
	int32 RigheAllaPresentazione = INDEX_NONE;
	TM->OnReactionWindowOpened.BindLambda([&Presentazioni, &RigheAllaPresentazione, TM, Watcher1](
		const FRTReactionWindowView&, int32)
	{
		++Presentazioni;
		RigheAllaPresentazione = RigheDelColpoDiOverwatch(TM, Watcher1);
	});
	ON_SCOPE_EXIT{ TM->OnReactionWindowOpened.Unbind(); };

	TM->RefreshTeamKnowledgeNow();
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ premessa: il primo watcher ha sparato"),
			FumettoIndiceDi(TM, ERTResolvedEventType::ReactionResolved, Watcher1->StableUnitId, MoverA->StableUnitId,
				ERTMatchPhase::Move) != INDEX_NONE)
		|| !TestTrue(TEXT("⛔ premessa: la resolution e' sospesa sulla finestra del secondo"), TM->IsResolutionSuspended())
		|| !TestTrue(TEXT("⛔ premessa: la finestra si presenta pompando il playback"),
			RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM))
		|| !TestEqual(TEXT("⛔ premessa: presentata una volta"), Presentazioni, 1)
		|| !TestEqual(TEXT("⛔ premessa: alla presentazione il colpo del primo watcher era gia' mostrato"),
			RigheAllaPresentazione, 1))
	{
		return false;
	}
	float Ultimo = Watcher1->LastSkillCalloutForTest().StartAt;
	if (!TestEqual(TEXT("⛔ premessa: il fumetto del primo watcher e' quello del colpo"),
			Watcher1->LastSkillCalloutForTest().ActionId, FName(TEXT("Action.Overwatch"))))
	{
		return false;
	}

	for (int32 Scadenze = 0; Scadenze < 8 && TM->IsResolutionSuspended(); ++Scadenze)
	{
		TM->ExpireReactionWindow();
	}
	if (!TestFalse(TEXT("⛔ premessa: la finestra e' chiusa e il turno ripreso"), TM->IsResolutionSuspended())) { return false; }
	int32 Rinascite = 0;
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
	{
		TM->Tick(0.05f);
		if (Watcher1->LastSkillCalloutForTest().StartAt != Ultimo)
		{
			++Rinascite;
			Ultimo = Watcher1->LastSkillCalloutForTest().StartAt;
		}
	}
	TestEqual(TEXT("🔴 dopo la ripresa il fumetto del primo watcher non rinasce"), Rinascite, 0);
	TestEqual(TEXT("e la riga del suo colpo e' una"), RigheDelColpoDiOverwatch(TM, Watcher1), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
