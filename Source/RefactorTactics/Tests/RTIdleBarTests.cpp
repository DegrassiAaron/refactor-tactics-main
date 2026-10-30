// La barra dei comandi SENZA un'unita' comandata ([D-460], #3494): la struttura invece del vuoto.
//
// 🔑 **L'oracolo delle Comuni e del Kit e' la barra di un'unita' VERA**, costruita dallo stesso ViewModel: la
// struttura deve riprodurne la forma — quante Comuni, quale Base, quanti vuoti — senza portarne nessuno stato.

#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTCellId.h"
#include "Player/RTPlayerController.h"
#include "Turn/RTActionFallbackLibrary.h" // ERTActionInvalidReason: il motivo di un piano illegale (#3748)
#include "UI/RTHudViewModel.h"
#include "UI/RTScreenHudWidgets.h"
#include "Unit/RTUnit.h"
#include "RTWorldFixtures.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	ARTUnit* SpawnIdleBarUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = false;
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Quante posizioni del kit di un'unita' si leggono col Kit (una vuota compresa), dalla sua barra vera. */
	int32 IdleBarKitLength(const ARTUnit* Unit)
	{
		int32 N = 0;
		for (const FRTAbilityCooldownView& R : URTHudViewModel::BuildAbilityCooldowns(Unit))
		{
			N += (R.Group == ERTActionGroup::Kit || R.Group == ERTActionGroup::None) ? 1 : 0;
		}
		return N;
	}

	/** Le Comuni di un'unita', nell'ordine di lettura. */
	TArray<FName> IdleBarCommons(const ARTUnit* Unit)
	{
		TArray<FName> Out;
		for (const FRTAbilityCooldownView& R :
			URTHudViewModel::OrderForReading(URTHudViewModel::BuildAbilityCooldowns(Unit)))
		{
			if (R.Group == ERTActionGroup::Common) { Out.Add(R.ActionId); }
		}
		return Out;
	}

	/** Lo stato che il grafo della dock chiederebbe: armato se l'indice e' quello armato, che senza unita' e' `INDEX_NONE`. */
	ERTActionSlotState IdleBarStateWithoutUnit(const FRTAbilityCooldownView& V)
	{
		return URTHudViewModel::ResolveSlotState(V, /*bArmed=*/ V.AbilityIndex == INDEX_NONE);
	}
}

/**
 * 🔴 **LA STRUTTURA: COMUNI SPENTE, UNA BASE VUOTA, TANTI VUOTI QUANTO IL KIT PIU' LUNGO** ([D-460]).
 *
 * ⚠️ **Il controllo sugli armati e' la ragione di `IdleSlotIndex`**: il grafo accende lo slot il cui indice e'
 * uguale a quello armato, che senza unita' vale `INDEX_NONE`. Una struttura con quell'indice sarebbe tutta
 * accesa come `Selected`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIdleBarShapeTest,
	"RefactorTactics.HudViewModel.IdleBarShowsTheCommonsOffAndTheKitEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIdleBarShapeTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	ARTUnit* Aevik = SpawnIdleBarUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(0, 0, 0));
	ARTUnit* Muiren = SpawnIdleBarUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(1, 0, 0));
	if (!Aevik || !Muiren)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("le due unita' esistono"), false);
	}

	const TArray<FName> Comuni = IdleBarCommons(Aevik); // la prima dell'elenco e' la fonte delle Comuni
	const int32 Kit = FMath::Max(IdleBarKitLength(Aevik), IdleBarKitLength(Muiren));
	if (!TestTrue(TEXT("premessa: ci sono Comuni"), Comuni.Num() > 0)
		|| !TestTrue(TEXT("premessa: c'e' un Kit"), Kit > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const TArray<FRTAbilityCooldownView> Barra = URTHudViewModel::BuildIdleBar({ Aevik, Muiren });
	if (!TestEqual(TEXT("Comuni + una Base + i vuoti del kit piu' lungo"), Barra.Num(), Comuni.Num() + 1 + Kit))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	for (int32 i = 0; i < Barra.Num(); ++i)
	{
		const FRTAbilityCooldownView& V = Barra[i];
		TestEqual(*FString::Printf(TEXT("voce %d: nessun indice di kit"), i), V.AbilityIndex, URTHudViewModel::IdleSlotIndex);
		TestFalse(*FString::Printf(TEXT("voce %d: niente di pianificato"), i), V.bPlanned);
		if (i < Comuni.Num())
		{
			TestEqual(*FString::Printf(TEXT("Comune %d: e' l'azione vera"), i), V.ActionId, Comuni[i]);
			TestEqual(*FString::Printf(TEXT("Comune %d: e' del gruppo Comuni"), i), V.Group, ERTActionGroup::Common);
			TestEqual(*FString::Printf(TEXT("Comune %d: spenta, cioe' Unavailable — non armata"), i),
				IdleBarStateWithoutUnit(V), ERTActionSlotState::Unavailable);
		}
		else
		{
			const bool bBase = i == Comuni.Num();
			TestEqual(*FString::Printf(TEXT("voce %d: il gruppo"), i), V.Group,
				bBase ? ERTActionGroup::Base : ERTActionGroup::Kit);
			TestEqual(*FString::Printf(TEXT("voce %d: vuota — Empty, non armata"), i),
				IdleBarStateWithoutUnit(V), ERTActionSlotState::Empty);
		}
	}
	TestTrue(TEXT("la Base apre un gruppo"), Barra[Comuni.Num()].bGroupBreakBefore);
	TestTrue(TEXT("il Kit apre un gruppo"), Barra[Comuni.Num() + 1].bGroupBreakBefore);

	TestEqual(TEXT("senza unita' nella squadra la barra resta vuota"),
		URTHudViewModel::BuildIdleBar({}).Num(), 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **LA STRUTTURA NON PORTA NIENTE DEL PIANO DELL'UNITA' DA CUI COPIA LE COMUNI** ([D-460]).
 *
 * 🔑 **E' il controllo dell'elenco positivo dei campi**: l'unita' fonte ha una Comune pianificata e un'altra in
 * ricarica, e il piano e' illegale. Una copia per intero della riga — invece che campo per campo — porterebbe
 * nella barra senza unita' l'impegno, il numero di turni e lo stato `Invalid` di un'unita' che nessuno comanda.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIdleBarCarriesNoPlanTest,
	"RefactorTactics.HudViewModel.IdleBarCarriesNothingOfThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIdleBarCarriesNoPlanTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ARTUnit* Fonte = SpawnIdleBarUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(0, 0, 0));
	if (!TestNotNull(TEXT("l'unita' fonte esiste"), Fonte)) { RTWorldFixtures::DestroyWorld(World); return false; }

	// Una Comune con ricarica: la si pianifica e la si consuma, cosi' il piano e' anche illegale (OnCooldown).
	int32 Comune = INDEX_NONE;
	const TArray<FRTAbilityCooldownView> Righe = URTHudViewModel::BuildAbilityCooldowns(Fonte);
	for (const FRTAbilityCooldownView& R : Righe)
	{
		const URTActionData* A = Fonte->GetAbility(R.AbilityIndex);
		if (R.Group == ERTActionGroup::Common && A && A->CooldownTurns > 0)
		{
			Comune = R.AbilityIndex;
			break;
		}
	}
	if (!TestTrue(TEXT("premessa: una Comune ha una ricarica"), Comune != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	Fonte->PlannedAbilityIndex = Comune;
	Fonte->ConsumeAbility(Comune);
	const TArray<FRTAbilityCooldownView> Prima = URTHudViewModel::BuildAbilityCooldowns(Fonte);
	if (!TestTrue(TEXT("premessa: nella barra dell'unita' la Comune e' pianificata, in ricarica e colpevole"),
		Prima[Comune].bPlanned && Prima[Comune].TurnsRemaining > 0 && Prima[Comune].bPlanInvalid))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	for (const FRTAbilityCooldownView& V : URTHudViewModel::BuildIdleBar({ Fonte }))
	{
		const FString Nome = V.ActionId.IsNone() ? TEXT("(vuota)") : V.ActionId.ToString();
		TestFalse(*FString::Printf(TEXT("%s: non pianificata"), *Nome), V.bPlanned);
		TestEqual(*FString::Printf(TEXT("%s: nessuna ricarica"), *Nome), V.TurnsRemaining, 0);
		TestEqual(*FString::Printf(TEXT("%s: nessun totale di ricarica"), *Nome), V.TotalTurns, 0);
		TestEqual(*FString::Printf(TEXT("%s: la carica al default"), *Nome), V.ChargeFraction, 1.f);
		TestFalse(*FString::Printf(TEXT("%s: nessuno stato di D-459"), *Nome),
			V.bPlanInvalid || V.bPlanDegraded || V.bTargetRefused);
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **LA BARRA DI CHI SI OSSERVA PORTA IL CATALOGO E NIENTE DELLA PARTITA** (#3748).
 *
 * 🔑 **Il soggetto ha TUTTO cio' che la partita sa di un'unita'**: un'azione armata, un piano sulla posizione 1 e
 * una ricarica vera sulla posizione 2, e una posizione di kit vuota. Il kit e l'equipaggiamento di un nemico
 * osservato sono pubblici; piano, azione armata e ricariche vere no — e una barra che li portasse direbbe a
 * chi guarda cio' che l'avversario ha deciso o speso.
 *
 * ⚠️ **Il controllo positivo e' la meta' del test**: la STESSA unita', letta da `BuildAbilityCooldowns`, porta il
 * piano, la ricarica e il tasto. Senza queste tre premesse le asserzioni sulla barra pubblica sarebbero vere
 * anche per una funzione che copia ogni campo dalla partita, perche' non ci sarebbe niente da copiare.
 *
 * ⛔ **Un secondo stato per il piano illegale**: `bPlanInvalid` dice di un avversario che il suo piano e' illegale e
 * quale azione e' la colpevole, cioe' dice il piano. Nel primo stato il piano e' legale, e `false` letto da `false`
 * non prova niente: nel secondo il piano va sulla posizione in ricarica, la barra della partita accende
 * `bPlanInvalid` (premessa) e la pubblica no.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPublicBarCarriesNoMatchTest,
	"RefactorTactics.HudViewModel.PublicBarCarriesTheCatalogAndNothingOfTheMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPublicBarCarriesNoMatchTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ARTUnit* Soggetto = SpawnIdleBarUnit(World, 1, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(0, 0, 0));
	if (!TestNotNull(TEXT("l'unita' osservata esiste"), Soggetto)) { RTWorldFixtures::DestroyWorld(World); return false; }

	// --- il soggetto: kit con una posizione 1 e una 2 piene, la 2 con una ricarica; l'ultima posizione svuotata ---
	const int32 Ultima = Soggetto->NumAbilities() - 1;
	if (!TestTrue(TEXT("premessa: le posizioni 1 e 2 portano un'azione, e la 2 ha una ricarica di catalogo"),
			Soggetto->GetAbility(1) != nullptr && Soggetto->GetAbility(2) != nullptr
			&& Soggetto->GetAbility(2)->CooldownTurns > 0)
		|| !TestTrue(TEXT("premessa: l'ultima posizione e' oltre la 2, cosi' lo svuotarla non tocca le altre"), Ultima > 2))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	Soggetto->Abilities[Ultima] = nullptr;
	Soggetto->SelectedAbilityIndex = 0;   // l'azione armata
	Soggetto->PlannedAbilityIndex = 1;    // il piano
	Soggetto->ConsumeAbility(2);          // la ricarica vera

	// --- CONTROLLO POSITIVO: la barra della partita le porta tutte e tre ---------------------------------------
	const TArray<FRTAbilityCooldownView> Partita = URTHudViewModel::BuildAbilityCooldowns(Soggetto);
	if (!TestEqual(TEXT("premessa: la barra della partita ha una riga per posizione"), Partita.Num(), Soggetto->NumAbilities())
		|| !TestTrue(TEXT("premessa: nella barra della partita la posizione 1 e' pianificata"), Partita[1].bPlanned)
		|| !TestTrue(TEXT("premessa: nella barra della partita la posizione 2 e' in ricarica vera, e non usabile"),
			Partita[2].TurnsRemaining > 0 && !Partita[2].bUsableNow && Partita[2].ChargeFraction < 1.f)
		|| !TestTrue(TEXT("premessa: nella barra della partita la posizione 2 porta il totale di ricarica, il denominatore dell'anello"),
			Partita[2].TotalTurns > 0)
		|| !TestFalse(TEXT("premessa: nella barra della partita la posizione 0 porta il tasto"), Partita[0].HotkeyLabel.IsEmpty())
		|| !TestTrue(TEXT("premessa: l'ultima posizione e' una riga vuota"), Partita[Ultima].ActionId.IsNone()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- LA BARRA PUBBLICA ------------------------------------------------------------------------------------
	const TArray<FRTAbilityCooldownView> Pubblica = URTHudViewModel::BuildPublicBar(Soggetto);
	if (!TestEqual(TEXT("una riga per posizione di kit, la vuota compresa"), Pubblica.Num(), Partita.Num()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	for (int32 i = 0; i < Pubblica.Num(); ++i)
	{
		const FRTAbilityCooldownView& R = Pubblica[i];
		const FRTAbilityCooldownView& P = Partita[i];
		const FString Dove = FString::Printf(TEXT("posizione %d"), i);

		TestEqual(*FString::Printf(TEXT("%s: l'indice e' quello del kit"), *Dove), R.AbilityIndex, i);

		// Il CATALOGO: uguale a quello che la barra della partita dice della stessa azione.
		TestEqual(*FString::Printf(TEXT("%s: l'azione"), *Dove), R.ActionId, P.ActionId);
		TestEqual(*FString::Printf(TEXT("%s: l'icona"), *Dove), R.IconId, P.IconId);
		TestEqual(*FString::Printf(TEXT("%s: il ripiego dell'icona"), *Dove), R.FallbackIconId, P.FallbackIconId);
		TestTrue(*FString::Printf(TEXT("%s: il nome"), *Dove), R.DisplayName.EqualTo(P.DisplayName));
		TestTrue(*FString::Printf(TEXT("%s: la frase"), *Dove), R.Description.EqualTo(P.Description));
		TestEqual(*FString::Printf(TEXT("%s: la portata"), *Dove), R.RangeCells, P.RangeCells);
		TestEqual(*FString::Printf(TEXT("%s: l'azione su di se'"), *Dove), R.bSelfTarget, P.bSelfTarget);
		TestEqual(*FString::Printf(TEXT("%s: il danno"), *Dove), R.Damage, P.Damage);
		TestEqual(*FString::Printf(TEXT("%s: lo slot del turno"), *Dove), R.Slot, P.Slot);
		TestEqual(*FString::Printf(TEXT("%s: la macro-fase"), *Dove), R.Phase, P.Phase);
		TestEqual(*FString::Printf(TEXT("%s: il segno di fase"), *Dove), R.PhaseMark, P.PhaseMark);
		TestTrue(*FString::Printf(TEXT("%s: l'etichetta di fase"), *Dove), R.PhaseLabel.EqualTo(P.PhaseLabel));
		TestEqual(*FString::Printf(TEXT("%s: il gruppo"), *Dove), R.Group, P.Group);
		TestEqual(*FString::Printf(TEXT("%s: la forma"), *Dove), R.Shape, P.Shape);
		TestEqual(*FString::Printf(TEXT("%s: il raggio dell'area"), *Dove), R.AreaRadius, P.AreaRadius);
		TestEqual(*FString::Printf(TEXT("%s: la ricarica di catalogo"), *Dove), R.CooldownTurns, P.CooldownTurns);
		TestTrue(*FString::Printf(TEXT("%s: la variante"), *Dove),
			R.VariantName.EqualTo(P.VariantName) && R.VariantTradeoff.EqualTo(P.VariantTradeoff));

		// ⛔ La PARTITA: niente, in ogni riga.
		TestTrue(*FString::Printf(TEXT("%s: nessun tasto"), *Dove), R.HotkeyLabel.IsEmpty());
		TestFalse(*FString::Printf(TEXT("%s: non pianificata"), *Dove), R.bPlanned);
		TestEqual(*FString::Printf(TEXT("%s: nessuna ricarica vera"), *Dove), R.TurnsRemaining, 0);
		TestEqual(*FString::Printf(TEXT("%s: la carica al default"), *Dove), R.ChargeFraction, 1.f);
		// R7: `TotalTurns` e' il denominatore dell'anello di carica, e senza questa riga resterebbe quello di catalogo. Un anello disegnato da
		// `TotalTurns > 0` con `ChargeFraction` a 1 direbbe «carica» di una skill nemica la cui ricarica vera e' privata. Al default, come in
		// `BuildIdleBar` ([D-460]): il tooltip legge `CooldownTurns` (catalogo, sopra), non questo campo. Il controllo positivo e' la premessa.
		TestEqual(*FString::Printf(TEXT("%s: nessun totale di ricarica"), *Dove), R.TotalTurns, 0);
		// `bPlanInvalid` ha il suo controllo positivo nel secondo stato, piu' sotto: qui il piano e' legale.
		TestFalse(*FString::Printf(TEXT("%s: nessun piano illegale"), *Dove), R.bPlanInvalid);
		// ⚠️ `bPlanDegraded` e `PlanDegradedRange` NON hanno un controllo positivo a questo livello: la barra della
		// partita li accende solo con una mappa (`ARTHexMapActor`), un bersaglio pianificato e un rifiuto che il click
		// darebbe, e quel banco esiste solo in `RTActionSlotRefusalTests.cpp` (`FSlotRefusalBench`). Qui provano che
		// restano al default, non che un leak del rifiuto sarebbe visto.
		TestFalse(*FString::Printf(TEXT("%s: nessun piano degradato"), *Dove), R.bPlanDegraded);
		// `bTargetRefused` lo scrive la dock, non `BuildAbilityCooldowns`: a questo livello non si puo' accendere,
		// ne' controllare. Che la dock in sola lettura lo tenga falso lo prova `ScreenHud.DockShowsTheBarOfAReadOnlySubject`, con un rifiuto vero sotto il puntatore.
		TestFalse(*FString::Printf(TEXT("%s: nessun bersaglio rifiutato"), *Dove), R.bTargetRefused);
		TestEqual(*FString::Printf(TEXT("%s: nessuna portata di ripiego"), *Dove), R.PlanDegradedRange, (int32)INDEX_NONE);

		if (!R.ActionId.IsNone())
		{
			// `bUsableNow` vero vuol dire «niente stato», non «pronta»: e' cio' che porta lo slot a `Available`
			// invece che a `Unavailable`. Anche per la posizione che la partita dice in ricarica.
			TestTrue(*FString::Printf(TEXT("%s: usabile = nessuno stato"), *Dove), R.bUsableNow);
			TestEqual(*FString::Printf(TEXT("%s: lo slot si legge Available, anche senza essere armato"), *Dove),
				URTHudViewModel::ResolveSlotState(R, /*bArmed=*/ false), ERTActionSlotState::Available);
		}
	}

	// La posizione vuota resta una RIGA al suo posto (#2987), e si legge Empty.
	TestTrue(TEXT("l'ultima posizione e' un segnaposto senza azione"), Pubblica[Ultima].ActionId.IsNone());
	TestEqual(TEXT("e il segnaposto si legge Empty"),
		URTHudViewModel::ResolveSlotState(Pubblica[Ultima], /*bArmed=*/ false), ERTActionSlotState::Empty);

	TestTrue(TEXT("senza unita' la barra pubblica e' vuota"), URTHudViewModel::BuildPublicBar(nullptr).IsEmpty());

	// --- SECONDO STATO: un piano che il validatore RIFIUTA ---------------------------------------------------------
	// ⛔ Il piano va sulla posizione 2, che e' in ricarica vera: `ValidatePlan` lo rifiuta (`OnCooldown`) e la barra
	// della partita accende `bPlanInvalid` con il suo motivo. E' il controllo positivo che il primo stato non ha:
	// un `BuildPublicBar` che rieseguisse il validatore lo copierebbe da qui.
	Soggetto->PlannedAbilityIndex = 2;
	const TArray<FRTAbilityCooldownView> PartitaIllegale = URTHudViewModel::BuildAbilityCooldowns(Soggetto);
	if (!TestTrue(TEXT("premessa: nella barra della partita la posizione 2 e' la colpevole di un piano illegale, col suo motivo"),
		PartitaIllegale.IsValidIndex(2) && PartitaIllegale[2].bPlanInvalid
			&& PartitaIllegale[2].PlanInvalidReason != ERTActionInvalidReason::None))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	const TArray<FRTAbilityCooldownView> PubblicaIllegale = URTHudViewModel::BuildPublicBar(Soggetto);
	if (!TestEqual(TEXT("piano illegale: una riga per posizione di kit"), PubblicaIllegale.Num(), PartitaIllegale.Num()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	for (int32 i = 0; i < PubblicaIllegale.Num(); ++i)
	{
		const FRTAbilityCooldownView& R = PubblicaIllegale[i];
		const FString Dove = FString::Printf(TEXT("piano illegale, posizione %d"), i);
		TestFalse(*FString::Printf(TEXT("%s: nessun piano illegale"), *Dove), R.bPlanInvalid);
		TestTrue(*FString::Printf(TEXT("%s: nessun motivo del validatore"), *Dove),
			R.PlanInvalidReason == ERTActionInvalidReason::None);
		TestFalse(*FString::Printf(TEXT("%s: non pianificata"), *Dove), R.bPlanned);
		if (!R.ActionId.IsNone())
		{
			TestEqual(*FString::Printf(TEXT("%s: lo slot si legge Available"), *Dove),
				URTHudViewModel::ResolveSlotState(R, /*bArmed=*/ false), ERTActionSlotState::Available);
		}
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **LA DOCK CONSEGNA LA STRUTTURA SOLO SENZA UN'UNITA' COMANDATA, E LA CONTA SULLA PROPRIA SQUADRA** ([D-460]).
 *
 * ⛔ **Il controllo B e' quello di privacy**: in campo c'e' un avversario con un kit PIU' LUNGO del proprio. Se
 * entrasse nel conteggio, il numero dei vuoti direbbe al giocatore quante azioni ha un'unita' nemica.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIdleBarDockTest,
	"RefactorTactics.ScreenHud.DockShowsTheIdleBarOnlyWithoutACommandedUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIdleBarDockTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	ARTUnit* A = SpawnIdleBarUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(0, 0, 0));
	ARTUnit* B = SpawnIdleBarUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(1, 0, 0));
	URTActionDockWidget* Dock = NewObject<URTActionDockWidget>(World);
	if (!A || !B || !Dock)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita' e dock esistono"), false);
	}
	// La propria e' quella dal kit piu' corto; l'avversaria, quella dal piu' lungo.
	ARTUnit* Propria = IdleBarKitLength(A) <= IdleBarKitLength(B) ? A : B;
	ARTUnit* Avversaria = Propria == A ? B : A;
	Avversaria->TeamId = 1;
	if (!TestTrue(TEXT("premessa: il kit avversario e' piu' lungo del proprio"),
		IdleBarKitLength(Avversaria) > IdleBarKitLength(Propria)))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	Dock->SetMatchContextForTest(nullptr, /*InPlayerTeamId=*/ 0);

	// --- A. senza unita': GetActions resta l'identita', la lettura consegna la struttura ------------------
	TestEqual(TEXT("A: GetActions senza unita' e' vuota"), Dock->GetActions().Num(), 0);
	const TArray<FRTAbilityCooldownView> Struttura = Dock->GetActionsInReadingOrder();
	TestTrue(TEXT("A: la lettura senza unita' non e' vuota"), Struttura.Num() > 0);

	// --- B. ⛔ i vuoti del Kit sono quelli della PROPRIA squadra ----------------------------------------------
	int32 VuotiKit = 0;
	for (const FRTAbilityCooldownView& V : Struttura)
	{
		VuotiKit += V.Group == ERTActionGroup::Kit ? 1 : 0;
		TestEqual(TEXT("A: ogni voce porta l'indice della struttura"), V.AbilityIndex, URTHudViewModel::IdleSlotIndex);
	}
	TestEqual(TEXT("B: i vuoti del Kit contano solo la propria squadra"), VuotiKit, IdleBarKitLength(Propria));

	// --- D. ⛔ la stessa domanda dalla squadra 1: il default del campo e' 0, e il controllo B da solo non
	//    distinguerebbe un filtro scritto `TeamId == 0` da quello vero (dalla revisione di privacy di #3494) ----
	Propria->TeamId = 1;
	Avversaria->TeamId = 0;
	Dock->SetMatchContextForTest(nullptr, /*InPlayerTeamId=*/ 1);
	int32 VuotiDaUno = 0;
	for (const FRTAbilityCooldownView& V : Dock->GetActionsInReadingOrder())
	{
		VuotiDaUno += V.Group == ERTActionGroup::Kit ? 1 : 0;
	}
	TestEqual(TEXT("D: dalla squadra 1 i vuoti contano la squadra 1"), VuotiDaUno, IdleBarKitLength(Propria));

	// --- C. con un'unita' comandata, la barra e' la sua --------------------------------------------------------
	Dock->SetSelectedUnitForTest(Propria);
	const TArray<FRTAbilityCooldownView> Sua = Dock->GetActionsInReadingOrder();
	TestEqual(TEXT("C: con l'unita' la lettura ha le sue voci"), Sua.Num(), Dock->GetActions().Num());
	for (const FRTAbilityCooldownView& V : Sua)
	{
		TestTrue(TEXT("C: e ogni voce porta un indice di kit vero"), V.AbilityIndex >= 0);
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **UNO SLOT DELLA STRUTTURA NON ARMA NIENTE** ([D-460]).
 *
 * 🔑 Il controllore ha un'unita' selezionata, cosi' il click arriverebbe davvero al kit: e' il caso del frame in
 * cui la selezione arriva mentre la barra mostra ancora la struttura. Il controllo positivo — lo stesso click
 * con un indice vero arma — impedisce che il verde venga da una porta che non arma mai.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIdleBarSlotArmsNothingTest,
	"RefactorTactics.ScreenHud.IdleBarSlotArmsNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIdleBarSlotArmsNothingTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ARTUnit* Unit = SpawnIdleBarUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	URTActionSlotWidget* Slot = NewObject<URTActionSlotWidget>(World);
	if (!Unit || !PC || !Slot)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita', controller e slot esistono"), false);
	}
	PC->SelectActorForTest(Unit);
	Slot->SetArmingControllerForTest(PC);

	FRTAbilityCooldownView Idle;
	Idle.AbilityIndex = URTHudViewModel::IdleSlotIndex;
	Idle.Group = ERTActionGroup::Kit;
	Slot->SetAction(Idle, false);
	Slot->Activate();
	TestEqual(TEXT("il click su uno slot della struttura non arma niente"), Unit->SelectedAbilityIndex, (int32)INDEX_NONE);

	// Controllo positivo: lo stesso slot con un indice vero arma.
	FRTAbilityCooldownView Vera = URTHudViewModel::BuildAbilityCooldowns(Unit)[0];
	Slot->SetAction(Vera, false);
	Slot->Activate();
	TestEqual(TEXT("controllo: con un indice di kit lo stesso click arma"), Unit->SelectedAbilityIndex, Vera.AbilityIndex);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
