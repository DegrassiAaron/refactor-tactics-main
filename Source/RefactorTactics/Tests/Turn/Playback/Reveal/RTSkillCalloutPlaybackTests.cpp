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
// Tiratore (Branth) pianifica l'indice 0, `Hero.Branth.ImpactShot`: una variante dell'attacco base. Il primo grida,
// il secondo no, e il test di controllo e' la stessa scena: stesso turno, stesso playback, due esiti.
// ⚠️ Due scene in piu', ciascuna col suo montaggio: la finestra di reazione (l'orologio del fumetto sopravvive alla
// ripresa, R4/R7) e il loadout di Branth (l'equipaggiamento non grida, A7/R1b).

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
#include "Tests/Turn/Reaction/RTReactionWindowFixtures.h"  // #3666: la finestra si presenta al trattenimento del playback
#include "UI/HUD/RTHudViewModel.h"               // SkillCalloutAlpha: l'opacita' che il widget ricaverebbe dall'orologio
#include "Tests/Shared/RTWorldFixtures.h"

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

		// ⛔ Premesse sul GATE, non sul playback: il test positivo e il suo controllo valgono solo se le due azioni
		// stanno davvero dalle due parti di `ActionShouts` (base vuota e non core / con base).
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
 * Chi grida: la skill di firma si, l'attacco base no — R1b.
 *
 * ⛔ Controllo positivo nella stessa scena: il Tiratore ATTIVA davvero (la sua cue Cast e' suonata), quindi la sua
 * assenza di fumetto e' il gate di `ActionShouts` e non un'attivazione che non e' mai arrivata a `ShowActivation`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackSignatureShoutsTest,
	"RefactorTactics.Playback.SkillCallout.SignatureSkillShoutsAndBaseAttackDoesNot",
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
	TestTrue(TEXT("⛔ controllo positivo: il Tiratore ha attivato (la cue Cast e' suonata)"),
		B.Tiratore->CastCuesPlayedForTest() == 1);
	TestTrue(TEXT("🔴 ma ImpactShot ha una base, quindi non grida"),
		B.Tiratore->LastSkillCalloutForTest().ActionId.IsNone());
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
 * ✅ Il controllo positivo e' `SignatureSkillShoutsAndBaseAttackDoesNot`: stessa scena, viewer della squadra 0.
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
 * L'azione di un pezzo d'equipaggiamento non grida — A7, R1b (final review di #4025, IMPORTANT 1).
 *
 * 🔴 **Il difetto**: `MakeEquipmentAction` scrive `ActionId = EquipmentId` e copia dal core un `BaseActionId` vuoto,
 * e `FindCoreAction` non conosce `Gadget.PortableCover`: per base e catalogo core il gadget era una skill di firma, e
 * con il loadout di DEFAULT (`EquipLoadout(DefaultLoadoutFor(...))`, come `RTMatchBootstrapper`) Branth gridava
 * «COPERTURA PORTATILE». Le tre premesse sotto misurano che il gate di prima l'avrebbe fatto gridare.
 * ⛔ Controllo positivo nella stessa scena: la cue Cast di Branth e' suonata, quindi l'attivazione e' arrivata a
 * `ShowActivation` e l'assenza di fumetto e' il gate, non un'attivazione persa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutPlaybackEquipmentIsSilentTest,
	"RefactorTactics.Playback.SkillCallout.EquipmentActionDoesNotShout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutPlaybackEquipmentIsSilentTest::RunTest(const FString&)
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
	// ⛔ Le premesse del DIFETTO: con base e catalogo core soltanto, questa azione griderebbe.
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
	TestTrue(TEXT("🔴 ma l'azione d'equipaggiamento non grida"), Branth->LastSkillCalloutForTest().ActionId.IsNone());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
