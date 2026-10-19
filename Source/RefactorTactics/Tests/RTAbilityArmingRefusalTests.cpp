// #2986 — cosa resta scritto sul modello quando una richiesta di armamento viene RIFIUTATA.
//
// 🔑 **L'oracolo di questo file e' lo STATO, non il log**, e la distinzione e' il motivo per cui esiste.
// Il difetto che `#2986` nomina si vede nel log — una riga che dichiara «armata» un'azione che la funzione
// non ha armato — ma un test che leggesse quella riga misurerebbe la stringa, non la conseguenza. La
// conseguenza vera e' che `URTActionDockWidget::GetArmedActionIndex()` legge `SelectedAbilityIndex` e accende
// lo slot: se il rifiuto lascia quel campo scritto, il giocatore vede armato cio' che non lo e' e lo scopre
// a turno risolto.
//
// ⚠️ **Ogni test qui porta un CONTROLLO POSITIVO**, e non e' zelo: entrambe le asserzioni interessanti sono
// «il campo e' rimasto `INDEX_NONE`», cioe' passerebbero anche se `SelectAbilityForCurrent` non facesse
// assolutamente nulla — per una guardia scattata a monte, per un'unita' non selezionata, per un kit vuoto.
// Il controllo positivo arma un'azione che DEVE passare, cosi' un verde significa «rifiuta questo e accetta
// quello» invece di «non fa niente».

#include "Misc/AutomationTest.h"
#include "Player/RTPlayerController.h"
#include "Unit/RTUnit.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTCellId.h"
#include "RTAbilityFixtures.h"
#include "RTWorldFixtures.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/**
	 * Un'unita' col kit vero e i cooldown dimensionati.
	 *
	 * ⚠️ `DispatchBeginPlay` non e' decorativo: senza di esso `AbilityCooldowns` resta vuoto,
	 * `ConsumeAbility` trova `IsValidIndex` falso e non scrive, e `CanUseAbility` risponde `true` per
	 * sempre — un test che asserisse «in ricarica» su un'unita' cosi' sarebbe verde anche col difetto.
	 */
	ARTUnit* SpawnUnitConKit(UWorld* World)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U)
		{
			return nullptr;
		}
		U->TeamId = 0;
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeAevik());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = false;
		U->DispatchBeginPlay();
		U->PlaceOnCell(FRTCellId(0, 0, 0), FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** La prima posizione del kit che si possa armare senza essere rifiutata: serve al controllo positivo. */
	int32 TrovaPosizionePronta(const ARTUnit* Unit, int32 Escluso)
	{
		for (int32 i = 0; i < Unit->NumAbilities(); ++i)
		{
			const URTActionData* A = Unit->GetAbility(i);
			if (i != Escluso && A != nullptr && Unit->CanUseAbility(i))
			{
				return i;
			}
		}
		return INDEX_NONE;
	}
}

/**
 * 🔴 **UN RIFIUTO NON LASCIA `SelectedAbilityIndex` SCRITTO.**
 *
 * Il caso misurato da `#2986`: `Unit->SelectAbility(Index)` veniva chiamato **prima** del controllo di
 * ricarica sulla reazione, quindi il ramo che rifiutava usciva con `SelectedAbilityIndex == Index` e
 * `PlannedReactionAbility == INDEX_NONE`. Due letture dello stesso modello che si contraddicono: il dock
 * accende lo slot, il pass delle reazioni non trova niente.
 *
 * ⚠️ **Il rifiuto NON e' cio' che si misura qui, e non deve cambiare**: una reazione in ricarica non si
 * arma, e questo test non chiede che si armi. Chiede che il modello non affermi il contrario.
 *
 * ⛔ **Verifica di mutazione**: rimettere `Unit->SelectAbility(Index)` sopra il controllo di `CanUseAbility`
 * — cioe' ripristinare la sequenza di prima — fa cadere il punto B. Un test che passasse anche cosi' non
 * misurerebbe nulla di questa issue.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRejectedArmingLeavesNoSelectedAbilityTest,
	"RefactorTactics.PlayerInput.RejectedArmingLeavesNoSelectedAbility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTRejectedArmingLeavesNoSelectedAbilityTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World))
	{
		return false;
	}

	ARTUnit* Unit = SpawnUnitConKit(World);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!Unit || !PC)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita' e controller esistono"), false);
	}
	PC->SelectActorForTest(Unit);

	// Una REAZIONE nel kit, allo slot 0: `AddCoreAbilityInSlot` sostituisce invece di accodare proprio
	// perche' un'abilita' appesa in coda sta a un indice che `AbilityCooldowns` non copre.
	const int32 Reazione = RTAbilityFixtures::AddCoreAbilityInSlot(Unit, TEXT("Action.Counter"), 0);
	if (!TestTrue(TEXT("premessa: la reazione e' entrata nel kit"), Reazione != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	const URTActionData* Counter = Unit->GetAbility(Reazione);
	if (!TestNotNull(TEXT("premessa: la reazione si rilegge dal kit"), Counter))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	TestEqual(TEXT("premessa: e' davvero uno slot di reazione"),
		static_cast<int32>(Counter->Def.Slot), static_cast<int32>(ERTActionSlot::Reaction));

	// La si manda in ricarica pagandola, non scrivendo l'array a mano: `ConsumeAbility` e' la strada di
	// produzione, e se domani cambiasse regola questo test la seguirebbe invece di divergere.
	Unit->ConsumeAbility(Reazione);
	if (!TestFalse(TEXT("premessa ANTI-VACUITA': la reazione risulta in ricarica"),
			Unit->CanUseAbility(Reazione)))
	{
		// Senza ricarica non c'e' rifiuto da misurare, e il verde qui sotto non significherebbe niente.
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- A. CONTROLLO POSITIVO: la funzione arma, quando non c'e' motivo di rifiutare -------------
	// Senza questo punto, «il campo e' rimasto neutro» sarebbe soddisfatto anche da una funzione inerte.
	const int32 Pronta = TrovaPosizionePronta(Unit, Reazione);
	if (!TestTrue(TEXT("premessa: esiste una posizione pronta per il controllo positivo"),
			Pronta != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	PC->SelectAbilityForCurrentForTest(Pronta);
	TestEqual(TEXT("A: un'azione pronta viene armata — la funzione NON e' inerte"),
		Unit->SelectedAbilityIndex, Pronta);

	// Si torna al neutro passando dalla porta, non scrivendo il campo: il disarmo e' un ingresso
	// legittimo di `SelectAbility` ([D-128]), ed e' anche il ramo che `#2986` ha smesso di far cadere
	// in quello del kit vuoto.
	PC->SelectAbilityForCurrentForTest(INDEX_NONE);
	if (!TestEqual(TEXT("premessa: il disarmo riporta al neutro prima della misura"),
			Unit->SelectedAbilityIndex, INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- B. il RIFIUTO non lascia niente scritto --------------------------------------------------
	PC->SelectAbilityForCurrentForTest(Reazione);
	TestEqual(TEXT("B: una reazione in ricarica rifiutata NON resta selezionata sul modello"),
		Unit->SelectedAbilityIndex, INDEX_NONE);
	TestEqual(TEXT("B: e non risulta nemmeno pianificata come reazione"),
		Unit->PlannedReactionAbility, INDEX_NONE);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **UNA POSIZIONE DI KIT VUOTA NON SI DICHIARA ARMATA.**
 *
 * Il buco nel kit e' il caso in cui il difetto e' **osservabile dallo stato** e non solo dal log:
 * `ARTUnit::SelectAbility` scrive se `Abilities.IsValidIndex(Index)`, e un `nullptr` DENTRO l'array passa
 * quel controllo. Prima del riordino la sequenza era `SelectAbility` → riga di successo → `GetAbility` →
 * `return` muto: `SelectedAbilityIndex` restava puntato a una posizione senza azione, e il dock accendeva
 * uno slot vuoto.
 *
 * 🔑 **Per questo il banco e' un buco e non un indice oltre la fine.** Un `Index` fuori dal kit sarebbe
 * respinto da `IsValidIndex` dentro `SelectAbility`, quindi lo stato resterebbe neutro **anche col
 * difetto**: il test sarebbe verde prima e dopo la correzione, cioe' non misurerebbe niente.
 *
 * ⛔ **Verifica di mutazione**: far tornare muto il `return` del ramo `!Ability` **e** rimetterlo sotto
 * `SelectAbility` fa cadere il punto B.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEmptyKitPositionIsNotReportedAsArmedTest,
	"RefactorTactics.PlayerInput.AnEmptyKitPositionIsNotReportedAsArmed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTEmptyKitPositionIsNotReportedAsArmedTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World))
	{
		return false;
	}

	ARTUnit* Unit = SpawnUnitConKit(World);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!Unit || !PC)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita' e controller esistono"), false);
	}
	PC->SelectActorForTest(Unit);

	if (!TestTrue(TEXT("premessa: il kit ha almeno due posizioni"), Unit->NumAbilities() >= 2))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- A. CONTROLLO POSITIVO -------------------------------------------------------------------
	const int32 Pronta = TrovaPosizionePronta(Unit, /*Escluso*/ INDEX_NONE);
	if (!TestTrue(TEXT("premessa: esiste una posizione pronta"), Pronta != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	PC->SelectAbilityForCurrentForTest(Pronta);
	TestEqual(TEXT("A: una posizione piena viene armata — la funzione NON e' inerte"),
		Unit->SelectedAbilityIndex, Pronta);
	PC->SelectAbilityForCurrentForTest(INDEX_NONE);

	// --- B. il BUCO nel kit ----------------------------------------------------------------------
	// Si svuota una posizione DIVERSA da quella del controllo positivo, cosi' il punto A resta valido.
	const int32 Buco = (Pronta == 0) ? 1 : 0;
	Unit->Abilities[Buco] = nullptr;
	if (!TestTrue(TEXT("premessa: l'indice del buco e' valido per l'array"),
			Unit->Abilities.IsValidIndex(Buco)))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	if (!TestNull(TEXT("premessa ANTI-VACUITA': quella posizione non porta nessuna azione"),
			Unit->GetAbility(Buco)))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	PC->SelectAbilityForCurrentForTest(Buco);
	TestEqual(TEXT("B: una posizione di kit vuota non resta armata sul modello"),
		Unit->SelectedAbilityIndex, INDEX_NONE);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **UN'AZIONE ATTIVA IN RICARICA NON SI ARMA** — `#3828`, decisione d'autore del 2026-10-19.
 *
 * ⏱️ *Fino a questa issue* il rifiuto copriva solo reazioni e supporti (le due forme che gia' interrogavano
 * `CanUseAbility`), e un'azione attiva in ricarica si armava ancora e veniva fermata dal targeting (`#3517` lo
 * lasciava com'era e chiedeva una decisione propria). Misurato a schermo il 2026-10-19: `Scarica lineare` di
 * Aevik, con 1 turno di ricarica residuo, si armava dal tasto `2` e dal click sullo slot.
 *
 * 🔑 **I canali sono due, e il test li esercita entrambi** — il tasto (`SelectAbilityForCurrentForTest`, origine
 * `Hotkey`) e la porta dello slot (`ArmKitAbility`, origine `Dock`). Le generiche confluiscono nella stessa
 * funzione per indice (`PressGenericHotkeyForTest` le raggiunge), ma nessuna generica del catalogo ha una ricarica
 * che prima non fosse gia' rifiutata: `Guard`, `Overwatch`, `Wait` e `Interact` hanno ricarica 0, e `Brace` e' su
 * se' stesso. Un caso generico qui sarebbe verde anche con la guardia vecchia.
 *
 * ⚠️ **Il banco e' l'azione STESSA, prima pronta e poi in ricarica.** Il controllo positivo arma `Scarica lineare`
 * pronta da tutte e due le porte: cosi' un verde sul rifiuto non puo' venire da un'azione che non si arma mai (una
 * guardia che guardasse la ricarica DICHIARATA a catalogo e non quella residua cadrebbe qui, al punto A).
 *
 * ⛔ **Verifica di mutazione**: (1) tornare alla guardia di prima, `(bReazione || bSelfTarget) && !CanUseAbility`,
 * fa cadere B; (2) rifiutare solo con origine `Hotkey` fa cadere il punto B del dock; (3) rimettere
 * `Unit->SelectAbility(Index)` sopra la guardia fa cadere B e C, perche' il rifiuto lascerebbe lo stato scritto;
 * (4) rifiutare ogni azione col `CooldownTurns` di catalogo positivo fa cadere A; (5) rifiutare solo oltre un turno
 * residuo (`GetAbilityCooldown > 1`) fa cadere B2 — trovata dalla revisione indipendente: A, B, C e D la lasciavano
 * verde, e il confine di un turno e' proprio il caso che l'autore ha visto a schermo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCooldownActionIsNotArmedTest,
	"RefactorTactics.PlayerInput.AnActionOnCooldownIsNotArmed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCooldownActionIsNotArmedTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World))
	{
		return false;
	}

	ARTUnit* Unit = SpawnUnitConKit(World);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!Unit || !PC)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita' e controller esistono"), false);
	}
	PC->SelectActorForTest(Unit);

	// Un'azione ATTIVA con una ricarica a catalogo: ne' reazione ne' su se' stessi, che il rifiuto gia' copriva.
	int32 Attiva = INDEX_NONE;
	for (int32 i = 0; i < Unit->NumAbilities(); ++i)
	{
		const URTActionData* A = Unit->GetAbility(i);
		if (A != nullptr && A->Def.Slot != ERTActionSlot::Reaction && !A->bSelfTarget && A->CooldownTurns > 0)
		{
			Attiva = i;
			break;
		}
	}
	if (!TestTrue(TEXT("premessa: il kit ha un'azione attiva con ricarica (il caso che si armava)"),
			Attiva != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	const int32 Pronta = TrovaPosizionePronta(Unit, Attiva);
	if (!TestTrue(TEXT("premessa: esiste un'altra posizione pronta, per il caso C"), Pronta != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- A. CONTROLLO POSITIVO: la STESSA azione, pronta, si arma da tutte e due le porte ---------
	if (!TestTrue(TEXT("premessa: l'azione e' pronta prima di essere pagata"), Unit->CanUseAbility(Attiva)))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	PC->SelectAbilityForCurrentForTest(Attiva);
	TestEqual(TEXT("A: pronta, il tasto la arma"), Unit->SelectedAbilityIndex, Attiva);
	PC->SelectAbilityForCurrentForTest(INDEX_NONE);
	PC->ArmKitAbility(Attiva);
	TestEqual(TEXT("A: pronta, lo slot del dock la arma"), Unit->SelectedAbilityIndex, Attiva);
	PC->ArmKitAbility(Attiva); // ricliccare lo slot armato DISARMA: si torna al neutro dalla porta, non dal campo
	if (!TestEqual(TEXT("premessa: il disarmo riporta al neutro prima della misura"),
			Unit->SelectedAbilityIndex, INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- B. in ricarica, NESSUNA delle due porte la arma -----------------------------------------
	// Si paga con `ConsumeAbility`, la strada di produzione, e non scrivendo l'array a mano.
	Unit->ConsumeAbility(Attiva);
	if (!TestFalse(TEXT("premessa ANTI-VACUITA': l'azione risulta in ricarica"), Unit->CanUseAbility(Attiva)))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	PC->SelectAbilityForCurrentForTest(Attiva);
	TestEqual(TEXT("B: in ricarica, il tasto NON la arma"), Unit->SelectedAbilityIndex, INDEX_NONE);
	// ⚠️ Il dock si misura da neutro, e non dal residuo del tasto: se il tasto l'avesse armata, un click sullo slot
	// gia' armato la DISARMEREBBE e il punto qui sotto sarebbe verde per il motivo sbagliato.
	PC->SelectAbilityForCurrentForTest(INDEX_NONE);
	PC->ArmKitAbility(Attiva);
	TestEqual(TEXT("B: in ricarica, lo slot del dock NON la arma"), Unit->SelectedAbilityIndex, INDEX_NONE);
	TestEqual(TEXT("B: e non risulta pianificata"), Unit->PlannedAbilityIndex, INDEX_NONE);

	// --- C. il RIFIUTO lascia il modello com'era: l'azione gia' armata resta armata ---------------
	PC->SelectAbilityForCurrentForTest(Pronta);
	if (!TestEqual(TEXT("premessa: un'altra azione, pronta, e' armata"), Unit->SelectedAbilityIndex, Pronta))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	const int32 PianificataPrima = Unit->PlannedAbilityIndex;
	PC->SelectAbilityForCurrentForTest(Attiva);
	TestEqual(TEXT("C: il tasto rifiutato non toglie l'armo che c'era"), Unit->SelectedAbilityIndex, Pronta);
	// Si riparte dall'armo di prima, cosi' il dock non eredita l'esito del tasto (ne' il toggle dello slot armato).
	PC->SelectAbilityForCurrentForTest(Pronta);
	PC->ArmKitAbility(Attiva);
	TestEqual(TEXT("C: il dock rifiutato non toglie l'armo che c'era"), Unit->SelectedAbilityIndex, Pronta);
	TestEqual(TEXT("C: e non tocca il piano"), Unit->PlannedAbilityIndex, PianificataPrima);
	PC->SelectAbilityForCurrentForTest(INDEX_NONE);

	// --- B2. a UN turno dalla fine l'azione e' ancora in ricarica: il confine --------------------------
	// Il caso che l'autore ha misurato a schermo il 2026-10-19 (`Scarica lineare` con 1 turno residuo). Un rifiuto che
	// scattasse solo oltre una soglia passerebbe A, B, C e D: la ricarica appena pagata e quella finita non lo vedono.
	int32 Giri = 0;
	while (Unit->GetAbilityCooldown(Attiva) > 1 && Giri < 16)
	{
		Unit->TickCooldowns();
		++Giri;
	}
	if (!TestEqual(TEXT("premessa: manca esattamente un turno"), Unit->GetAbilityCooldown(Attiva), 1)
		|| !TestFalse(TEXT("premessa ANTI-VACUITA': a un turno dalla fine e' ancora in ricarica"),
			Unit->CanUseAbility(Attiva)))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	PC->SelectAbilityForCurrentForTest(Attiva);
	TestEqual(TEXT("B2: a un turno dalla fine il tasto NON la arma"), Unit->SelectedAbilityIndex, INDEX_NONE);
	PC->SelectAbilityForCurrentForTest(INDEX_NONE); // il dock si misura da neutro, come al punto B
	PC->ArmKitAbility(Attiva);
	TestEqual(TEXT("B2: a un turno dalla fine il dock NON la arma"), Unit->SelectedAbilityIndex, INDEX_NONE);
	PC->SelectAbilityForCurrentForTest(INDEX_NONE);

	// --- D. FINITA la ricarica si arma di nuovo: la regola e' sulla ricarica residua, non sull'azione ---
	int32 Turni = 0;
	while (!Unit->CanUseAbility(Attiva) && Turni < 16)
	{
		Unit->TickCooldowns();
		++Turni;
	}
	if (!TestTrue(TEXT("premessa: la ricarica finisce"), Unit->CanUseAbility(Attiva)))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	PC->SelectAbilityForCurrentForTest(Attiva);
	TestEqual(TEXT("D: finita la ricarica, il tasto la arma di nuovo"), Unit->SelectedAbilityIndex, Attiva);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
