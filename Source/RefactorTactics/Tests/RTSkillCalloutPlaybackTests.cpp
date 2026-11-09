// IL FUMETTO DELLA SKILL NEL PLAYBACK (#4025, spec del fumetto R1b, R4, R7, R9).
//
// 🔑 **Il meccanismo, non il disegno**: headless l'overlay non ha un widget, quindi si legge il REGISTRO che
// `ARTUnit::ShowSkillCallout` scrive sempre (`LastSkillCalloutForTest`), la sua vita (`IsSkillCalloutLive`) e
// l'ultimo tempo che il playback gli ha portato (`SkillCalloutClockForTest`). Sono i tre fatti che il
// `TurnManager` e' l'unico a decidere: se accendere, quale orologio dare, quando spegnere.
// ⚠️ La fixture e' quella di `Tests/RTPlaybackActivationTests.cpp` (stessa scena, stessa porta del viewer: vedi
// quel file per il perche' di `InitializeActorsForPlay` e di `MakePlayerOnTeam`), copiata con nomi locali
// DIVERSI perche' in unity build i namespace anonimi di due file condividono la translation unit.
// ⛔ Lo Scudo (Muiren) pianifica `Hero.Muiren.TideGuard` in Prep: una skill di firma, `BaseActionId` vuoto. Il
// Tiratore (Branth) pianifica l'indice 0, `Hero.Branth.ImpactShot`: una variante dell'attacco base. Il primo grida,
// il secondo no, e il test di controllo e' la stessa scena: stesso turno, stesso playback, due esiti.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Player/RTPlayerController.h"
#include "Player/RTPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "RTWorldFixtures.h"

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
 * 🔑 `StartAt == PlaybackElapsedTotal` (e `> 0`) dice che l'origine e' l'orologio del playback nell'istante della
 * rivelazione (non un contatore dell'unita', non uno zero), e `SkillCalloutClock >= StartAt` che gli e' arrivato. A fine playback la vita e' chiusa e la lista del manager
 * vuota, ma il registro resta: e' lo scopo del campo farsi leggere dopo.
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
	// `PlaybackElapsedTotal` non cambia in quel tick, quindi l'origine del fumetto E' il totale di adesso. Con `<=` un
	// `ShowSkillCallout(Ev.ActionId, 0.f)` restava verde (0 e' sempre minore o uguale), e nessun altro test legge `StartAt`.
	// `> 0` perche' la rivelazione cade dopo il primo `Dt` (`TickPlayback` lo somma prima di rivelare): un `StartAt`
	// a zero e' un'origine non portata, e `Equal` da solo non la distinguerebbe se il totale fosse a zero.
	TestEqual(TEXT("🔴 l'origine del fumetto e' ESATTAMENTE l'orologio del playback (StartAt == PlaybackElapsedTotal)"),
		Acceso.StartAt, B.TM->PlaybackElapsedTotalForTest());
	TestTrue(TEXT("🔴 e non e' zero: la rivelazione cade dopo il primo tick (StartAt > 0)"), Acceso.StartAt > 0.f);
	TestTrue(TEXT("🔴 e quell'orologio gli e' arrivato (clock >= StartAt)"),
		B.Scudo->SkillCalloutClockForTest() >= Acceso.StartAt);
	TestEqual(TEXT("il manager tiene l'unita' accesa nella sua lista"), B.TM->SkillCalloutUnitsForTest(), 1);

	CorriFinoAllaFineDelFumetto(B.TM);

	TestFalse(TEXT("premessa: il playback e' finito"), B.TM->IsResolving());
	TestFalse(TEXT("🔴 a fine playback il fumetto e' spento"), B.Scudo->IsSkillCalloutLive());
	TestEqual(TEXT("e la lista del manager e' vuota"), B.TM->SkillCalloutUnitsForTest(), 0);
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
 * 🔑 Il tempo del fumetto e' `PlaybackElapsedTotal`, e `TickPlayback` esce prima di toccarlo quando il playback e'
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
	const float TotalePrima = B.TM->PlaybackElapsedTotalForTest();
	B.TM->PausePlayback();
	if (!TestTrue(TEXT("⛔ premessa: il playback e' in pausa"), B.TM->IsPlaybackPaused())) { return false; }

	for (int32 I = 0; I < 10; ++I) { B.TM->Tick(0.05f); }

	TestEqual(TEXT("⛔ premessa: l'orologio del playback e' fermo"), B.TM->PlaybackElapsedTotalForTest(), TotalePrima);
	TestEqual(TEXT("🔴 in pausa l'orologio del fumetto non avanza"), B.Scudo->SkillCalloutClockForTest(), OrologioPrima);
	TestTrue(TEXT("e il fumetto e' ancora vivo: la pausa non lo spegne"), B.Scudo->IsSkillCalloutLive());

	B.TM->ResumePlayback();
	B.TM->Tick(0.05f);

	TestTrue(TEXT("✅ alla ripresa l'orologio del fumetto avanza"), B.Scudo->SkillCalloutClockForTest() > OrologioPrima);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
