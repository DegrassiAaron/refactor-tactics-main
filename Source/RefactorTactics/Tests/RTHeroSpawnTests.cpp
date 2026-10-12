#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h" // GetGenericActionIds: il kit e' eroe + generiche (D-025)
#include "Ability/RTEquipmentData.h" // ERTEquipmentSlot: una variante MODIFICA l'attacco base, non lo accoda
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Frontend/RTStartupReport.h"
#include "RTGameMode.h"
#include "Unit/RTUnit.h"
#include "Match/RTMatchBootstrapper.h" // RTUnitClassForHero: la classe visiva per eroe (#3586)
#include "Tests/RTUnitClassProbeForTest.h"
#include "Turn/RTTurnManager.h"            // GetTurnLog: la traccia del primo turno, oracolo di «stessa partita» (#2748)
#include "Turn/RTTurnLogLibrary.h"         // SerializeTurnLog: byte e checksum, come il gate di determinismo
#include "RTWorldFixtures.h"               // PlayOneTurn: un turno giocato senza input
#include "RTConsoleVariableGuardForTest.h" // le due console variable dei roster tornano com'erano
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Lo spawn del roster attraverso `ARTGameMode::SetupHexMatch` — il percorso VERO di CP 6.6, non una sua
 * imitazione. Un test che ricostruisse a mano cio' che fa `SpawnHero` resterebbe verde anche togliendo la
 * guardia fail-closed dal GameMode: verificherebbe la propria copia, non il codice spedito. (E' successo:
 * la prima stesura di questo file faceva esattamente quell'errore, e la verifica di mutazione l'ha scoperto.)
 */
namespace
{
	UWorld* MakeRosterWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyRosterWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Mappa esagonale piena: abbastanza celle percorribili per le quattro posizioni di partenza. */
	ARTHexMapActor* SpawnRosterMap(UWorld* World, int32 Radius = 4)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	TArray<ARTUnit*> CollectRosterUnits(UWorld* World)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Found);

		TArray<ARTUnit*> Units;
		for (AActor* Actor : Found)
		{
			if (ARTUnit* Unit = Cast<ARTUnit>(Actor)) { Units.Add(Unit); }
		}
		return Units;
	}

	ARTUnit* FindByHeroId(const TArray<ARTUnit*>& Units, const TCHAR* HeroId)
	{
		for (ARTUnit* Unit : Units)
		{
			if (Unit->HeroId == FName(HeroId)) { return Unit; }
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroSpawnFromDataTest,
	"RefactorTactics.Heroes.SpawnFromData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroSpawnFromDataTest::RunTest(const FString&)
{
	// Nome vincolante della DoD (CP 6.6): il GameMode allestisce il 2v2 con i QUATTRO EROI del catalogo, non
	// piu' con due archetipi hard-coded.
	UWorld* World = MakeRosterWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = SpawnRosterMap(World);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		DestroyRosterWorld(World);
		return false;
	}

	GameMode->SetupHexMatch(HexMap);

	const TArray<ARTUnit*> Units = CollectRosterUnits(World);
	if (!TestEqual(TEXT("quattro unita' in campo"), Units.Num(), 4))
	{
		DestroyRosterWorld(World);
		return false;
	}

	// I quattro eroi del catalogo, ognuno una volta sola: nessun duplicato involontario.
	TSet<FName> InPlay;
	for (const ARTUnit* Unit : Units) { InPlay.Add(Unit->HeroId); }
	TestEqual(TEXT("quattro eroi distinti"), InPlay.Num(), 4);
	TestTrue(TEXT("c'e' Aevik"), InPlay.Contains(FName(TEXT("Hero.Aevik"))));
	TestTrue(TEXT("c'e' Muiren"), InPlay.Contains(FName(TEXT("Hero.Muiren"))));
	TestTrue(TEXT("c'e' Branth"), InPlay.Contains(FName(TEXT("Hero.Branth"))));
	TestTrue(TEXT("c'e' Ivrin"), InPlay.Contains(FName(TEXT("Hero.Ivrin"))));

	// Ogni unita' porta le statistiche del SUO eroe, sopravvissute a BeginPlay.
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		const FString Who = Hero->HeroId.ToString();
		ARTUnit* Unit = FindByHeroId(Units, *Who);
		if (!TestNotNull(FString::Printf(TEXT("%s in campo"), *Who), Unit)) { continue; }

		TestEqual(FString::Printf(TEXT("%s: salute"), *Who), Unit->MaxHealth, Hero->MaxHealth);
		TestEqual(FString::Printf(TEXT("%s: salute corrente = massima"), *Who), Unit->Health, Hero->MaxHealth);
		TestEqual(FString::Printf(TEXT("%s: movimento"), *Who), Unit->MoveRange, Hero->MovePoints);
		TestEqual(FString::Printf(TEXT("%s: vista"), *Who), Unit->VisionRange, Hero->VisionRange);
		TestEqual(FString::Printf(TEXT("%s: resistenza push"), *Who), Unit->PushResistance, Hero->PushResistance);
		TestEqual(FString::Printf(TEXT("%s: affinita'"), *Who), Unit->Affinity, Hero->Affinity);
		// Cinque dell'eroe piu' le generiche di D-025: ogni unita' in campo puo' dichiarare `Guard` e `Brace`,
		// e i quattro rami che li consumano nel TurnManager smettono di essere irraggiungibili.
		//
		// ⚠️ **Piu' i pezzi del LOADOUT dal 2026-08-16 (`#1054`)**, e il numero si DERIVA: gadget e modulo
		// concedono un'azione ciascuno, la variante d'arma no — modifica l'attacco base. Scrivere `+2` a
		// mano varrebbe finche' due eroi su quattro restano senza default (§4 assegna loro due gadget che
		// v0.1 non costruisce), e diventerebbe falso in silenzio il giorno in cui E36 o E13 li sbloccano.
		const int32 PezziCheConcedono = URTCatalogLibrary::DefaultLoadoutFor(Hero->HeroId).Num() > 0 ? 2 : 0;
		// ⚠️ **Anche il numero di azioni d'eroe si DERIVA**, per la stessa ragione del `+2` qui sopra: era
		// un `5` scritto a mano, ed e' diventato falso il giorno in cui [D-226] ha dato a Muiren e Ivrin
		// una sesta abilita'. Un conteggio esatto in un test che misura una SOMMA non aggiunge niente —
		// quanti kit abbia un eroe lo pinna `Heroes.RosterIsBalanced`, che esiste per quello.
		TestEqual(FString::Printf(TEXT("%s: azioni dell'eroe piu' generiche piu' il loadout"), *Who),
			Unit->NumAbilities(),
			Hero->Actions.Num() + URTCatalogLibrary::GetGenericActionIds().Num() + PezziCheConcedono);
		TestEqual(FString::Printf(TEXT("%s: l'indice 0 resta l'attacco base"), *Who),
			Unit->GetAbility(0)->Def.ActionId, Hero->Actions[0]->Def.ActionId);
	}

	// Formazione di default: Aevik+Muiren (giocatore) contro Branth+Ivrin (bot).
	ARTUnit* Aevik = FindByHeroId(Units, TEXT("Hero.Aevik"));
	ARTUnit* Muiren = FindByHeroId(Units, TEXT("Hero.Muiren"));
	ARTUnit* Branth = FindByHeroId(Units, TEXT("Hero.Branth"));
	ARTUnit* Ivrin = FindByHeroId(Units, TEXT("Hero.Ivrin"));
	if (Aevik && Muiren && Branth && Ivrin)
	{
		TestEqual(TEXT("Aevik e' del giocatore"), Aevik->TeamId, 0);
		TestEqual(TEXT("Muiren anche: la combo Wet e' giocabile"), Muiren->TeamId, 0);
		TestEqual(TEXT("Branth e' del bot"), Branth->TeamId, 1);
		TestEqual(TEXT("Ivrin anche"), Ivrin->TeamId, 1);
		TestFalse(TEXT("il giocatore comanda i suoi"), Aevik->bIsBotControlled);
		TestTrue(TEXT("il bot comanda i propri"), Branth->bIsBotControlled);

		// **Il punto della DoD sul bot**: MP diversi arrivano davvero in campo. Il budget dello snapshot viene
		// da `MoveRange`, quindi il bot non puo' proporre a Branth una mossa da 6 celle.
		TestEqual(TEXT("Branth: 4 MP"), Branth->MoveRange, 4);
		TestEqual(TEXT("Ivrin: 6 MP"), Ivrin->MoveRange, 6);
		// ⚠️ **La portata ora dipende dalla VARIANTE D'ARMA, e il valore atteso si deriva** (`#1054`).
		// Aevik non ha un loadout — §4 gli assegna `Gadget.Insulator`, che v0.1 non costruisce — quindi
		// resta a 4, il numero del catalogo. Branth monta `Weapon.Impact`, che toglie una cella: **3 → 2**.
		// Il `2` non e' scritto qui: lo produce `ApplyWeaponVariant`, cosi' un ribilanciamento della
		// variante fa cadere il catalogo e non questo file, e il giorno in cui Aevik avra' il suo gadget
		// questa riga comincera' a coprire anche lui senza che nessuno la aggiorni.
		auto PortataAttesa = [](const TCHAR* HeroId) -> int32
		{
			for (const URTHeroData* H : URTHeroCatalogLibrary::GetHeroRoster())
			{
				if (!H || H->HeroId != FName(HeroId) || H->Actions.Num() == 0) { continue; }
				const int32 Base = H->Actions[0]->Def.RangeCells;
				const TArray<FName> Loadout = URTCatalogLibrary::DefaultLoadoutFor(H->HeroId);
				if (Loadout.Num() == 0) { return Base; }
				const URTEquipmentData* V =
					URTCatalogLibrary::FindEquipment(URTCatalogLibrary::DefaultWeaponVariantFor(H->HeroId));
				return URTCatalogLibrary::ApplyWeaponVariant(H->Actions[0]->Def, V).RangeCells;
			}
			return INDEX_NONE;
		};
		TestEqual(TEXT("Aevik colpisce alla portata del catalogo (nessun loadout)"),
			Aevik->AttackRange, PortataAttesa(TEXT("Hero.Aevik")));
		TestEqual(TEXT("Branth colpisce alla portata ridotta dalla sua variante"),
			Branth->AttackRange, PortataAttesa(TEXT("Hero.Branth")));
		// E che le due NON siano lo stesso numero: senza questa riga il lambda potrebbe restituire sempre
		// la portata base e le due asserzioni sopra passerebbero senza dimostrare che la variante morde.
		TestNotEqual(TEXT("la variante d'arma cambia davvero la portata di Branth"),
			Branth->AttackRange, URTHeroCatalogLibrary::MakeBranth()->Actions[0]->Def.RangeCells);

		// Ogni unita' ha le PROPRIE istanze d'azione: due eroi che condividessero un `URTActionData`
		// ricaricherebbero insieme.
		TestTrue(TEXT("azioni non condivise"), Aevik->GetAbility(0) != Muiren->GetAbility(0));

		// Le unita' stanno su celle distinte della mappa.
		TSet<FRTCellId> Cells;
		for (const ARTUnit* Unit : Units) { Cells.Add(Unit->Cell); }
		TestEqual(TEXT("quattro celle di partenza distinte"), Cells.Num(), 4);
	}

	DestroyRosterWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroSpawnDuplicateTest,
	"RefactorTactics.Heroes.DuplicateHeroEntersOnlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroSpawnDuplicateTest::RunTest(const FString&)
{
	// ⚠️ **Questo test si chiamava `SpawnFailsClosedWithoutData` e copriva due difetti insieme**: un
	// `HeroId` inesistente e uno ripetuto. La prima meta' e' uscita con `#1069`, che ha cambiato la regola:
	// un eroe non risolto non lascia piu' entrare «solo gli eroi validi» — ferma l'allestimento, e ha i suoi
	// due test (`UnknownHeroInFormationAbortsSetup`, `UnknownHeroIsDeclaredFatal`). Resta la meta' che la
	// regola non ha toccato, e resta qui perche' nessun altro test la copre.
	UWorld* World = MakeRosterWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = SpawnRosterMap(World);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		DestroyRosterWorld(World);
		return false;
	}

	// Aevik in ENTRAMBE le squadre: la formazione dichiara quattro slot, ma i nomi distinti sono tre.
	// Due unita' che condividessero un `URTHeroData` ricaricherebbero insieme le stesse azioni.
	GameMode->Team0Heroes = { TEXT("Hero.Aevik"), TEXT("Hero.Muiren") };
	GameMode->Team1Heroes = { TEXT("Hero.Aevik"), TEXT("Hero.Ivrin") };

	GameMode->SetupHexMatch(HexMap);

	const TArray<ARTUnit*> Units = CollectRosterUnits(World);
	TestEqual(TEXT("tre unita' in campo: la copia non entra"), Units.Num(), 3);

	TSet<FName> InPlay;
	for (const ARTUnit* Unit : Units) { InPlay.Add(Unit->HeroId); }
	TestTrue(TEXT("Aevik, una volta sola"), InPlay.Contains(FName(TEXT("Hero.Aevik"))));
	TestTrue(TEXT("e Ivrin"), InPlay.Contains(FName(TEXT("Hero.Ivrin"))));
	TestEqual(TEXT("nessun duplicato in campo"), InPlay.Num(), Units.Num());

	// Nessuna unita' e' rimasta senza identita': se il fail-closed non funzionasse, qui ci sarebbe un'unita'
	// con `HeroId` vuoto e i valori di default di `ARTUnit`.
	for (const ARTUnit* Unit : Units)
	{
		TestFalse(TEXT("ogni unita' in campo ha un eroe"), Unit->HeroId.IsNone());
	}

	DestroyRosterWorld(World);
	return true;
}

/**
 * Un'unità schierata in partita porta il proprio loadout di default (#1054, CP 7.4).
 *
 * ⚠️ **Sta qui e non fra i test d'equipaggiamento, e la differenza è tutto il punto di `#63`.**
 * `Equipment.DefaultLoadoutIsOnePerSlotForEveryHero` chiede al catalogo qual è il default e verifica che
 * sia legale: risponde a *«esiste un default?»*. Nessuno dei due, né quello né `LoadoutExactlyOneEach`,
 * cadrebbe se **nessuna unità venisse mai equipaggiata** — ed è precisamente ciò che succedeva.
 * Questo test passa da `SetupHexMatch`, cioè dal percorso che una partita esegue davvero, e interroga
 * l'**unità in campo**. È l'anello che mancava.
 *
 * I valori attesi si **derivano** dal catalogo, non si scrivono: `ApplyWeaponVariant` sul `Def` dell'eroe
 * dice quale portata deve avere l'attacco base dopo la variante. Trascrivere un numero qui creerebbe una
 * terza copia di ciò che il catalogo già decide, e cadrebbe al primo ribilanciamento senza dire perché.
 *
 * ⚠️ **Metà roster non ha un loadout, e non è un difetto di questo test.** §4 assegna a Aevik e Ivrin
 * due gadget che v0.1 non costruisce (`Gadget.Insulator` è un passivo che aspetta E36, `Gadget.Sensor`
 * aspetta E13), quindi `DefaultLoadoutFor` restituisce vuoto per loro — decisione presa nella fetta A.
 * Il test non elenca *quali*: chiede al catalogo, così quando E36 atterrerà comincerà a coprire anche
 * loro senza che nessuno lo aggiorni.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSpawnedUnitLoadoutTest,
	"RefactorTactics.Heroes.SpawnedUnitCarriesItsDefaultLoadout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSpawnedUnitLoadoutTest::RunTest(const FString&)
{
	UWorld* World = MakeRosterWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = SpawnRosterMap(World);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		DestroyRosterWorld(World);
		return false;
	}

	GameMode->SetupHexMatch(HexMap);
	const TArray<ARTUnit*> Units = CollectRosterUnits(World);

	int32 Equipaggiati = 0;
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		const FString Who = Hero->HeroId.ToString();
		ARTUnit* Unit = FindByHeroId(Units, *Who);
		if (!TestNotNull(*FString::Printf(TEXT("%s in campo"), *Who), Unit)) { continue; }

		const TArray<FName> Loadout = URTCatalogLibrary::DefaultLoadoutFor(Hero->HeroId);
		const int32 Generiche = URTCatalogLibrary::GetGenericActionIds().Num();

		if (Loadout.Num() == 0)
		{
			// Nessun default: l'unità entra ESATTAMENTE come prima. Un eroe senza equipaggiamento non deve
			// entrare a metà — `ValidateLoadout` non vedrà mai un insieme parziale, perché non ne esiste uno.
			TestEqual(*FString::Printf(TEXT("%s: senza default, azioni invariate"), *Who),
				Unit->NumAbilities(), Hero->Actions.Num() + Generiche);
			continue;
		}
		++Equipaggiati;

		// 1. I pezzi che CONCEDONO un'azione (gadget e modulo) sono in campo, per ActionId.
		for (const FName& PieceId : Loadout)
		{
			const URTEquipmentData* Piece = URTCatalogLibrary::FindEquipment(PieceId);
			if (!Piece || Piece->Slot == ERTEquipmentSlot::WeaponVariant) { continue; }

			bool bTrovata = false;
			for (int32 i = 0; i < Unit->NumAbilities(); ++i)
			{
				const URTActionData* A = Unit->GetAbility(i);
				if (A && A->Def.ActionId == PieceId) { bTrovata = true; break; }
			}
			TestTrue(*FString::Printf(TEXT("%s: '%s' e' in campo come azione"), *Who, *PieceId.ToString()),
				bTrovata);
		}

		// 2. La variante d'arma ha MODIFICATO l'attacco base — non è stata accodata come azione.
		//    Il valore atteso si deriva dal catalogo: è ciò che `ApplyWeaponVariant` produce.
		const FName VarianteId = URTCatalogLibrary::DefaultWeaponVariantFor(Hero->HeroId);
		const URTEquipmentData* Variante = URTCatalogLibrary::FindEquipment(VarianteId);
		const URTActionData* Base = Unit->GetAbility(0);
		if (Variante && TestNotNull(*FString::Printf(TEXT("%s: attacco base"), *Who), Base))
		{
			const FRTActionDef Atteso = URTCatalogLibrary::ApplyWeaponVariant(
				Hero->Actions[0]->Def, Variante);

			TestEqual(*FString::Printf(TEXT("%s: portata dopo la variante"), *Who),
				Base->Def.RangeCells, Atteso.RangeCells);
			TestEqual(*FString::Printf(TEXT("%s: l'ActionId resta quello dell'eroe"), *Who),
				Base->Def.ActionId, Hero->Actions[0]->Def.ActionId);

			// Lo specchio legacy segue il `Def`: `ARTTurnManager` legge `AttackRange` da qui, e un'unità
			// con `Def` aggiornato e specchio fermo colpirebbe alla portata vecchia in partita.
			TestEqual(*FString::Printf(TEXT("%s: lo specchio RangeCells segue il Def"), *Who),
				Base->RangeCells, Atteso.RangeCells);
		}

		// 3. Due azioni in più di quelle dell'eroe: gadget e modulo. La variante non conta, perché
		//    modifica invece di aggiungere — ed è la distinzione che l'harness sbagliava.
		TestEqual(*FString::Printf(TEXT("%s: azioni dell'eroe piu' generiche piu' i due pezzi"), *Who),
			Unit->NumAbilities(), Hero->Actions.Num() + Generiche + 2);
	}

	// Misura, non uguaglianza: sale da sé quando E36 ed E13 sbloccano i gadget mancanti. Zero significherebbe
	// che nessuno equipaggia, cioè che questa fetta non ha consegnato niente.
	TestTrue(*FString::Printf(TEXT("almeno un'unita' entra equipaggiata (oggi: %d)"), Equipaggiati),
		Equipaggiati > 0);

	// **Il bot come chiunque altro.** Non c'è un ramo per lui: `SpawnHero` è il punto unico, e questa
	// asserzione è ciò che impedisce a un `if (!bIsBotControlled)` di rientrare domani senza far rosso.
	for (const ARTUnit* Unit : Units)
	{
		if (!Unit || !Unit->bIsBotControlled) { continue; }
		const TArray<FName> Loadout = URTCatalogLibrary::DefaultLoadoutFor(Unit->HeroId);
		if (Loadout.Num() == 0) { continue; }

		// ⚠️ **Il kit si CHIEDE al catalogo, non si scrive qui.** Questa riga diceva `5`, cioe' la
		// dimensione che ogni kit aveva quando il test e' stato scritto; `ValidateHeroes` ammette pero'
		// *«1 attacco base + 4 fondamentali, piu' al massimo UNA generica portata nel kit»*, e il giorno in
		// cui un eroe ne ha sei — `Hero.Branth.MortarShot`, `#2890` — il letterale diventa rosso senza che
		// nulla sia rotto. L'asserzione sopra, sul giocatore, derivava gia' da `Hero->Actions.Num()`: le
		// due meta' della stessa domanda misuravano il roster in due modi diversi, e una sola invecchiava.
		const URTHeroData* const* HeroPtr = URTHeroCatalogLibrary::GetHeroRoster().FindByPredicate(
			[Unit](const URTHeroData* H) { return H && H->HeroId == Unit->HeroId; });
		if (!HeroPtr || !*HeroPtr) { continue; }

		TestEqual(*FString::Printf(TEXT("bot %s: equipaggiato quanto il giocatore"),
			*Unit->HeroId.ToString()),
			Unit->NumAbilities(),
			(*HeroPtr)->Actions.Num() + URTCatalogLibrary::GetGenericActionIds().Num() + 2);
	}

	DestroyRosterWorld(World);
	return true;
}


/**
 * #1069 criterio 3 — un eroe che il catalogo non conosce FERMA l'allestimento.
 *
 * Prima faceva `continue` con un Warning: la partita si allestiva **a meta'** e in campo restavano le
 * unita' risolte. E' lo stesso dato che il conteggio della formazione controlla venti righe sopra con
 * `Error` + partita non allestita — la formazione dichiara CHI gioca, e un CHI inesistente non e' una
 * partita piu' piccola: e' una formazione che nessuno onora.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnknownHeroAbortsSetupTest,
	"RefactorTactics.Heroes.UnknownHeroInFormationAbortsSetup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnknownHeroAbortsSetupTest::RunTest(const FString&)
{
	UWorld* World = MakeRosterWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = SpawnRosterMap(World);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		DestroyRosterWorld(World);
		return false;
	}

	// Il CONTEGGIO resta quello del formato — due per squadra — cosi' la guardia che si esercita e' quella
	// del nome. Con una formazione piu' corta si fermerebbe prima, sulla cardinalita', e questo test
	// direbbe verde senza aver mai toccato il ramo che copre.
	GameMode->Team0Heroes = { TEXT("Hero.Aevik"), TEXT("Hero.NonEsiste") };

	AddExpectedError(TEXT("non e' nel catalogo eroi"), EAutomationExpectedErrorFlags::Contains, 1);
	GameMode->SetupHexMatch(HexMap);

	// **Zero, non tre.** Aevik precede il nome ignoto e la squadra 1 e' intatta: senza la guardia in campo
	// resterebbero TRE unita', ed e' quel numero — una partita che parte e sembra normale — a rendere il
	// difetto invisibile a chi guarda lo schermo invece di contare i pezzi.
	TestEqual(TEXT("nessuna unita' in campo"), CollectRosterUnits(World).Num(), 0);

	DestroyRosterWorld(World);
	return true;
}

/**
 * #1069 criterio 3, seconda meta': fermarsi **e dirlo**.
 *
 * Un `return` muto lascerebbe il rapporto d'avvio senza fatale, quindi nessun modale: il giocatore
 * vedrebbe una schermata vuota senza una causa. `ERTStartupOutcome` esiste per questo, ed e' un elenco
 * chiuso — l'esito nuovo si dichiara li' prima di emetterlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnknownHeroIsDeclaredFatalTest,
	"RefactorTactics.Heroes.UnknownHeroIsDeclaredFatal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnknownHeroIsDeclaredFatalTest::RunTest(const FString&)
{
	UWorld* World = MakeRosterWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = SpawnRosterMap(World);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		DestroyRosterWorld(World);
		return false;
	}

	GameMode->Team1Heroes = { TEXT("Hero.Branth"), TEXT("Hero.NonEsiste") };

	AddExpectedError(TEXT("non e' nel catalogo eroi"), EAutomationExpectedErrorFlags::Contains, 1);
	GameMode->SetupHexMatch(HexMap);

	const FRTStartupReport& Report = GameMode->GetStartupReport();

	TestEqual(TEXT("l'avvio dichiara il fatale"),
		URTStartupReportLibrary::FindFatal(Report), ERTStartupOutcome::RosterHeroMissing);
	TestNotEqual(TEXT("e non arriva a Ready"), Report.Phase, ERTLoadPhase::Ready);

	// Il dettaglio NOMINA l'eroe: senza, il modale direbbe «un eroe non risolto» e chi legge dovrebbe
	// andare a cercare quale nei log. E' la stessa regola del `Detail` degli altri esiti.
	bool bNomeNelDettaglio = false;
	for (const FRTStartupNote& Note : Report.Notes)
	{
		if (Note.Outcome == ERTStartupOutcome::RosterHeroMissing
			&& Note.Detail.Contains(TEXT("Hero.NonEsiste")))
		{
			bNomeNelDettaglio = true;
		}
	}
	TestTrue(TEXT("il dettaglio nomina l'eroe non risolto"), bNomeNelDettaglio);

	DestroyRosterWorld(World);
	return true;
}


// ---------------------------------------------------------------------------------------------------------
// PRESENTAZIONE — CP E21.1 (`#287`): la classe visiva per eroe.
//
// ⚠️ **Il DoD di `#287` dichiarava il fallback «gia' coperto headless da `Heroes.SpawnFailsClosedWithoutData`»,
// e la premessa era falsa due volte**: quel test non si chiama piu' cosi' (vedi il commento a
// `FRTHeroSpawnDuplicateTest`), e nemmeno nella forma originale guardava `HeroUnitClasses` — copriva
// l'`HeroId` inesistente e quello ripetuto, che sono un'altra regola. Prima di questi tre test nessun test
// del repository nominava `HeroUnitClasses`: il ripiego al cilindro non era coperto da niente.
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroUnitClassesDefaultTest,
	"RefactorTactics.Heroes.HeroUnitClassesCoversTheRoster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroUnitClassesDefaultTest::RunTest(const FString&)
{
	// Il default vive nel costruttore di `ARTGameMode`, quindi si legge dal CDO: non serve un mondo.
	const ARTGameMode* Cdo = GetDefault<ARTGameMode>();
	if (!TestNotNull(TEXT("CDO del GameMode"), Cdo)) { return false; }

	// ⚠️ **Il percorso e' parte dell'asserto, non un dettaglio.** Un finder che non risolve lascia la voce
	// ASSENTE e in partita torna il cilindro senza dire niente: e' esattamente il difetto che `#287` ha
	// chiuso. Se un `BP_Unit_*` viene spostato o rinominato, qui si vede subito.
	//
	// ⚠️ **E il percorso e' per eroe**, perche' l'errore facile in quel costruttore e' lo scambio: quattro
	// `Assegna` in fila con quattro finder simili, e `Hero.Muiren` che riceve il Blueprint di Aevik passerebbe
	// un asserto scritto solo su «non e' il cilindro».
	// ✅ **L'asset si e' mosso, e questa tabella con lui** (`#2297` fetta E, 2026-09-10). Fino a qui i quattro
	// path dicevano `Gadget`, `Phase`, `Riktor` e `Wraith` — i nomi degli **slot asset Paragon**, che [D-321]
	// ha dichiarato non-definitivi — mentre le identita' erano gia' quelle canoniche.
	//
	// 🔑 **Questa riga e' l'ORACOLO del rename, non la sua copia.** Il ciclo qui sotto non confronta stringhe:
	// risolve `Trovata->Get()` e asserisce che la classe **esista**. ∴ se l'asset non fosse stato rinominato,
	// o lo fosse a un path diverso, il `FClassFinder` di `ARTGameMode` lascerebbe la voce assente e questo
	// test cadrebbe — che e' cio' che il commento a `:507` promette da sempre.
	const TMap<FName, FString> Attesi = {
		{ FName(TEXT("Hero.Aevik")), TEXT("/Game/RT/Characters/Aevik/Blueprints/BP_Unit_Aevik") },
		{ FName(TEXT("Hero.Muiren")),  TEXT("/Game/RT/Characters/Muiren/Blueprints/BP_Unit_Muiren")   },
		{ FName(TEXT("Hero.Branth")), TEXT("/Game/RT/Characters/Branth/Blueprints/BP_Unit_Branth") },
		{ FName(TEXT("Hero.Ivrin")), TEXT("/Game/RT/Characters/Ivrin/Blueprints/BP_Unit_Ivrin") },
	};

	TestEqual(TEXT("il default copre i quattro eroi del roster"), Cdo->HeroUnitClasses.Num(), Attesi.Num());

	for (const TPair<FName, FString>& Atteso : Attesi)
	{
		const TSubclassOf<ARTUnit>* Trovata = Cdo->HeroUnitClasses.Find(Atteso.Key);
		if (!TestNotNull(*FString::Printf(TEXT("voce per %s"), *Atteso.Key.ToString()), (const void*)Trovata))
		{
			continue;
		}

		UClass* Classe = Trovata->Get();
		if (!TestNotNull(*FString::Printf(TEXT("classe risolta per %s"), *Atteso.Key.ToString()), Classe))
		{
			continue;
		}

		TestTrue(*FString::Printf(TEXT("%s e' un'unita'"), *Atteso.Key.ToString()),
			Classe->IsChildOf(ARTUnit::StaticClass()));

		// Un Blueprint generato si chiama `<Nome>_C`: il confronto e' sul percorso dell'asset, che il suffisso
		// non tocca.
		TestTrue(*FString::Printf(TEXT("%s punta a %s"), *Atteso.Key.ToString(), *Atteso.Value),
			Classe->GetPathName().StartsWith(Atteso.Value));

		// E NON e' il cilindro: senza questa riga un default che risolvesse tutto su `ARTUnit` passerebbe le
		// due precedenti (`IsChildOf` di se' stessa e' vero) e la partita mostrerebbe quattro cilindri.
		TestTrue(*FString::Printf(TEXT("%s non ricade sul cilindro"), *Atteso.Key.ToString()),
			Classe != ARTUnit::StaticClass());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroUnitClassesEmptyFallbackTest,
	"RefactorTactics.Heroes.EmptyHeroUnitClassesFallsBackToCylinder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroUnitClassesEmptyFallbackTest::RunTest(const FString&)
{
	UWorld* World = MakeRosterWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = SpawnRosterMap(World);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		DestroyRosterWorld(World);
		return false;
	}

	// `HeroUnitClasses` resta `EditAnywhere`: e' un DEFAULT, e svuotarla e' una configurazione legittima —
	// la stessa che si aveva prima di `#287`, e quella di chi apre il progetto senza i pack Paragon.
	GameMode->HeroUnitClasses.Empty();

	GameMode->SetupHexMatch(HexMap);

	const TArray<ARTUnit*> Units = CollectRosterUnits(World);
	TestEqual(TEXT("le quattro unita' entrano lo stesso"), Units.Num(), 4);

	for (const ARTUnit* Unit : Units)
	{
		// Classe ESATTA, non `IsA`: il ripiego dichiarato e' `ARTUnit`, e un `IsA` resterebbe verde anche se
		// il ripiego diventasse un Blueprint qualunque.
		TestEqual(*FString::Printf(TEXT("%s e' il cilindro C++"), *Unit->HeroId.ToString()),
			Unit->GetClass(), ARTUnit::StaticClass());
	}

	// «La partita resta giocabile» non e' una frase: il ripiego tocca la CLASSE VISIVA, e i dati devono
	// restare quelli del catalogo. Senza questo confronto il test proverebbe solo che quattro attori sono
	// stati creati.
	//
	// ⚠️ **Contro il catalogo, non contro un numero**: `ARTUnit` nasce con `MaxHealth = 100`, quindi un
	// `> 0` scritto a mano resterebbe verde anche se `ConfigureFromHeroData` non girasse affatto.
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		const FString Who = Hero->HeroId.ToString();
		ARTUnit* Unit = FindByHeroId(Units, *Who);
		if (!TestNotNull(FString::Printf(TEXT("%s in campo col cilindro"), *Who), Unit)) { continue; }

		TestEqual(FString::Printf(TEXT("%s: salute dal catalogo"), *Who), Unit->MaxHealth, Hero->MaxHealth);
		TestEqual(FString::Printf(TEXT("%s: movimento dal catalogo"), *Who), Unit->MoveRange, Hero->MovePoints);

		if (TestNotNull(FString::Printf(TEXT("%s: il kit e' caricato"), *Who), Unit->GetAbility(0)))
		{
			TestEqual(FString::Printf(TEXT("%s: l'attacco base e' il suo"), *Who),
				Unit->GetAbility(0)->Def.ActionId, Hero->Actions[0]->Def.ActionId);
		}
	}

	// E l'avvio non dichiara un fatale: un asset visivo assente non e' un difetto di allestimento.
	TestEqual(TEXT("l'avvio arriva a Ready"), GameMode->GetStartupReport().Phase, ERTLoadPhase::Ready);

	DestroyRosterWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroUnitClassesPartialTest,
	"RefactorTactics.Heroes.PartialHeroUnitClassesMixesMeshAndCylinder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroUnitClassesPartialTest::RunTest(const FString&)
{
	UWorld* World = MakeRosterWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = SpawnRosterMap(World);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		DestroyRosterWorld(World);
		return false;
	}

	// Il caso che il DoD di `#287` chiede per nome: **mesh assegnata a meta' roster**. E' lo stato reale di
	// chi ha solo alcuni pack Paragon, e la domanda e' se le due meta' convivono nella stessa partita.
	const TSubclassOf<ARTUnit>* ClasseAevik = GameMode->HeroUnitClasses.Find(FName(TEXT("Hero.Aevik")));
	if (!TestNotNull(TEXT("il default porta la classe di Aevik"), (const void*)ClasseAevik))
	{
		DestroyRosterWorld(World);
		return false;
	}
	UClass* AttesaAevik = ClasseAevik->Get();

	GameMode->HeroUnitClasses.Remove(FName(TEXT("Hero.Branth")));
	GameMode->HeroUnitClasses.Remove(FName(TEXT("Hero.Ivrin")));

	GameMode->SetupHexMatch(HexMap);

	const TArray<ARTUnit*> Units = CollectRosterUnits(World);
	TestEqual(TEXT("le quattro unita' entrano"), Units.Num(), 4);

	const ARTUnit* Aevik = FindByHeroId(Units, TEXT("Hero.Aevik"));
	const ARTUnit* Branth = FindByHeroId(Units, TEXT("Hero.Branth"));
	if (TestNotNull(TEXT("Aevik in campo"), Aevik) && TestNotNull(TEXT("Branth in campo"), Branth))
	{
		TestEqual(TEXT("Aevik ha la sua classe visiva"), Aevik->GetClass(), AttesaAevik);
		TestEqual(TEXT("Branth, senza voce, ricade sul cilindro"), Branth->GetClass(), ARTUnit::StaticClass());

		// La meta' senza asset non degrada l'altra: e' il punto della domanda.
		TestNotEqual(TEXT("le due meta' restano distinte"), Aevik->GetClass(), Branth->GetClass());
	}

	DestroyRosterWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroMeshYawOffsetIsPinnedTest,
	"RefactorTactics.Heroes.MeshYawOffsetIsPinnedPerHero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroMeshYawOffsetIsPinnedTest::RunTest(const FString&)
{
	const ARTGameMode* Cdo = GetDefault<ARTGameMode>();
	if (!TestNotNull(TEXT("CDO del GameMode"), Cdo)) { return false; }

	// I quattro valori misurati sul CDO il 2026-09-10 (`origin/main` = `6c695731`). Le skeletal di
	// personaggio si modellano lungo +Y e il forward di un attore e' +X: `ACharacter` compensa i 90 gradi
	// nel costruttore, `ARTUnit` deriva da `AActor` e non li ha mai avuti — da cui il `-90`.
	//
	// 🔴 **`Hero.Muiren` e' atteso `-90` come gli altri, e `#1793` dice che per lui e' SBAGLIATO.** La sua
	// mesh e' `Phase_GDC`, l'unica del set che non segue la convenzione. Questa riga fotografa lo stato
	// **prima** della calibrazione: quando quel Blueprint ricevera' il suo valore, questo test diventa
	// ROSSO, e il rosso e' il punto — dice che l'asset e' cambiato davvero, invece di lasciare passare in
	// silenzio una modifica a un binario che nessun diff sa leggere.
	//
	// 🔤 **I quattro nomi sono quelli del roster corrente**, e non era cosi' quando il test e' stato
	// scritto: due delle identita' che attendeva sono state rinominate su `main` mentre il branch era
	// fermo. ⛔ Portarlo invariato avrebbe rotto DUE test: questo, che avrebbe cercato chiavi inesistenti,
	// e `RTLegacyIdentityRatchetTests.cpp`, che tiene un **tetto di file** per ogni identita' ritirata —
	// due di quei tetti valgono `1` ed erano gia' consumati, quindi anche solo NOMINARLE qui in un
	// commento li sfonderebbe. ⚠️ Per questo il commento non le nomina: la tabella dei rename sta in quel
	// file, ed e' li' che si guarda quando una di queste righe smette di risolvere — un `Hero.*` che non
	// esiste piu' fa fallire questo test per una ragione che non e' la sua.
	const TMap<FName, float> Attesi = {
		{ FName(TEXT("Hero.Aevik")),  -90.f },
		{ FName(TEXT("Hero.Muiren")), -90.f },
		{ FName(TEXT("Hero.Branth")), -90.f },
		{ FName(TEXT("Hero.Ivrin")),  -90.f },
	};

	// ⛔ L'anti-vacuita': se la mappa e' vuota o piu' corta, il ciclo sotto non asserisce niente.
	if (!TestEqual(TEXT("il roster risolve quattro classi"), Cdo->HeroUnitClasses.Num(), Attesi.Num()))
	{
		return false;
	}

	for (const TPair<FName, float>& Atteso : Attesi)
	{
		const TSubclassOf<ARTUnit>* Trovata = Cdo->HeroUnitClasses.Find(Atteso.Key);
		if (!TestNotNull(*FString::Printf(TEXT("voce per %s"), *Atteso.Key.ToString()), (const void*)Trovata))
		{
			continue;
		}

		UClass* Classe = Trovata->Get();
		if (!TestNotNull(*FString::Printf(TEXT("classe risolta per %s"), *Atteso.Key.ToString()), Classe))
		{
			continue;
		}

		// Il CDO della classe Blueprint: e' il valore che l'unita' avra' allo spawn, ed e' cio' che
		// `#1793` chiede di LEGGERE invece di dedurre dalla name table del `.uasset`.
		const ARTUnit* UnitCdo = Cast<ARTUnit>(Classe->GetDefaultObject());
		if (!TestNotNull(*FString::Printf(TEXT("CDO di %s"), *Atteso.Key.ToString()), UnitCdo))
		{
			continue;
		}

		// 🔑 **CONTROLLO POSITIVO, e senza di esso questo test sarebbe una decorazione.** La vacuita' da
		// escludere e' specifica: se `GetDefaultObject()` restituisse il CDO **nativo** invece di quello
		// della classe Blueprint, l'asserto qui sotto leggerebbe sempre il default C++ (`-90`) e passerebbe
		// **qualunque cosa dica il `.uasset`** — cioe' tacerebbe esattamente il giorno in cui deve parlare,
		// quando `BP_Unit_Phase` ricevera' il suo valore.
		//
		// `VisualZOffset` separa i due casi: il C++ lo mette a `UnitHalfHeight`, i quattro Blueprint lo
		// SOVRASCRIVONO (misurato sulla name table: compare in tutti e quattro). Se il valore letto qui
		// coincidesse col nativo, staremmo leggendo la classe sbagliata.
		const ARTUnit* NativeCdo = GetDefault<ARTUnit>();
		if (NativeCdo)
		{
			TestFalse(
				*FString::Printf(TEXT("%s: leggo il CDO del Blueprint, non quello nativo "
					"(VisualZOffset %.1f contro il nativo %.1f)"),
					*Atteso.Key.ToString(), UnitCdo->VisualZOffset, NativeCdo->VisualZOffset),
				FMath::IsNearlyEqual(UnitCdo->VisualZOffset, NativeCdo->VisualZOffset));
		}

		TestEqual(*FString::Printf(TEXT("%s: MeshYawOffset"), *Atteso.Key.ToString()),
			UnitCdo->MeshYawOffset, Atteso.Value);
	}

	return true;
}

// --- La classe visiva di un eroe (`#3586`) ----------------------------------------------------

/**
 * **Una regola sola per partita e scenari**: la classe configurata per l'eroe se c'e', il cilindro altrimenti.
 *
 * 🔑 Il ripiego vale per DUE casi distinti, e il test li separa: l'eroe assente dalla mappa e l'eroe presente
 * con una classe nulla (una voce di `HeroUnitClasses` lasciata vuota in un Blueprint). Entrambi devono dare il
 * cilindro, mai un `nullptr` che `SpawnActorDeferred` rifiuterebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitClassForHeroTest,
	"RefactorTactics.Heroes.UnitClassForHeroFallsBackToTheCylinder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitClassForHeroTest::RunTest(const FString&)
{
	TMap<FName, TSubclassOf<ARTUnit>> Classi;
	Classi.Add(FName(TEXT("Hero.Aevik")), ARTUnitClassProbeForTest::StaticClass());
	Classi.Add(FName(TEXT("Hero.Muiren")), nullptr);

	TestTrue(TEXT("un eroe con la sua classe riceve quella"),
		RTUnitClassForHero(Classi, FName(TEXT("Hero.Aevik"))) == ARTUnitClassProbeForTest::StaticClass());
	TestTrue(TEXT("un eroe assente dalla mappa riceve il cilindro"),
		RTUnitClassForHero(Classi, FName(TEXT("Hero.Branth"))) == ARTUnit::StaticClass());
	TestTrue(TEXT("una voce nulla riceve il cilindro, non nullptr"),
		RTUnitClassForHero(Classi, FName(TEXT("Hero.Muiren"))) == ARTUnit::StaticClass());
	TestTrue(TEXT("una mappa vuota da' sempre il cilindro"),
		RTUnitClassForHero({}, FName(TEXT("Hero.Aevik"))) == ARTUnit::StaticClass());

	// Una classe che `SpawnActorDeferred` rifiuterebbe ricade sul cilindro, non su un'unita' mancante.
	TMap<FName, TSubclassOf<ARTUnit>> Astratte;
	Astratte.Add(FName(TEXT("Hero.Ivrin")), ARTUnitAbstractProbeForTest::StaticClass());
	TestTrue(TEXT("una classe astratta riceve il cilindro"),
		RTUnitClassForHero(Astratte, FName(TEXT("Hero.Ivrin"))) == ARTUnit::StaticClass());
	return true;
}

// ======================================================================================================
// `#2748`, [D-507] — il roster ha tre sorgenti: proprieta' < riga di comando < console
// ======================================================================================================

extern TAutoConsoleVariable<FString> CVarRTTeam0Heroes;
extern TAutoConsoleVariable<FString> CVarRTTeam1Heroes;

namespace
{
	/**
	 * Console e riga di comando tornano com'erano. Sono stato di PROCESSO: un test che le lasciasse impostate
	 * cambierebbe il roster di ogni test successivo, e il rosso comparirebbe in un file che non c'entra.
	 */
	struct FRTScopedRosterSources
	{
		RTTestConsoleVariable::TGuardia<FString> Team0{ *CVarRTTeam0Heroes.AsVariable(), FString() };
		RTTestConsoleVariable::TGuardia<FString> Team1{ *CVarRTTeam1Heroes.AsVariable(), FString() };
		FString SavedCommandLine;

		FRTScopedRosterSources() : SavedCommandLine(FCommandLine::Get()) {}
		~FRTScopedRosterSources() { FCommandLine::Set(*SavedCommandLine); }

		void SetArgs(const TCHAR* Args) { FCommandLine::Set(*(SavedCommandLine + FString(TEXT(" ")) + Args)); }
		void ClearArgs() { FCommandLine::Set(*SavedCommandLine); }
	};

	/** `Hero.A,Hero.B` nell'ordine dato; una voce vuota si vede come `<vuota>`. */
	FString JoinRoster(const TArray<FName>& Roster)
	{
		return FString::JoinBy(Roster, TEXT(","), [](const FName& HeroId)
		{
			return HeroId.IsNone() ? FString(TEXT("<vuota>")) : HeroId.ToString();
		});
	}

	/** Gli eroi di una squadra IN CAMPO, ordinati: si confronta chi c'e', non quanti. */
	FString HeroIdsInPlay(const TArray<ARTUnit*>& Units, int32 TeamId)
	{
		TArray<FString> Ids;
		for (const ARTUnit* Unit : Units)
		{
			if (Unit && Unit->TeamId == TeamId) { Ids.Add(Unit->HeroId.ToString()); }
		}
		Ids.Sort();
		return FString::Join(Ids, TEXT(","));
	}

	/** Il dettaglio del fatale `RosterHeroMissing`, o la stringa vuota se il rapporto non lo porta. */
	FString MissingHeroDetail(const FRTStartupReport& Report)
	{
		for (const FRTStartupNote& Note : Report.Notes)
		{
			if (Note.Outcome == ERTStartupOutcome::RosterHeroMissing) { return Note.Detail; }
		}
		return FString();
	}
}

/**
 * LE TRE SORGENTI e il loro ordine: proprieta' < `-RTTeam0=` < `rt.Match.Team0`, una coppia per squadra.
 *
 * 🔑 **Il caso che vale il test e' la VIRGOLA.** `FParse::Value` termina il valore su `,` se non glielo si
 * vieta: una lista da riga di comando arriverebbe troncata alla prima voce, e l'errore a valle parlerebbe di
 * cardinalita'. Qui si guarda la SECONDA voce.
 *
 * ⚠️ **E i casi sbagliati NON ricadono sulla proprieta'**: `-RTTeam0=` senza valore e una virgola in coda
 * restano cio' che sono — una lista vuota, una voce vuota. Il resolver sceglie chi ha parlato, non se ha
 * detto il vero; a rifiutare e' il bootstrapper (test qui sotto).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRosterSourcePrecedenceTest,
	"RefactorTactics.Match.Roster.SourcePrecedence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRosterSourcePrecedenceTest::RunTest(const FString&)
{
	UWorld* World = MakeRosterWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode)) { DestroyRosterWorld(World); return false; }

	FRTScopedRosterSources Sorgenti;
	const FString Proprieta0 = JoinRoster(GameMode->Team0Heroes);
	const FString Proprieta1 = JoinRoster(GameMode->Team1Heroes);
	if (!TestTrue(TEXT("premessa: le due proprieta' sono formazioni diverse e non vuote"),
			!Proprieta0.IsEmpty() && !Proprieta1.IsEmpty() && Proprieta0 != Proprieta1))
	{
		DestroyRosterWorld(World);
		return false;
	}

	// Niente di niente: decide la proprieta'.
	TestEqual(TEXT("senza flag e senza console decide la proprieta' (squadra 0)"),
		JoinRoster(GameMode->ResolveTeamHeroes(0)), Proprieta0);
	TestEqual(TEXT("senza flag e senza console decide la proprieta' (squadra 1)"),
		JoinRoster(GameMode->ResolveTeamHeroes(1)), Proprieta1);

	// La riga di comando, con la VIRGOLA: la lista arriva intera.
	Sorgenti.SetArgs(TEXT("-RTTeam0=Hero.Branth,Hero.Ivrin"));
	TestEqual(TEXT("la lista da riga di comando arriva INTERA, virgola compresa"),
		JoinRoster(GameMode->ResolveTeamHeroes(0)), FString(TEXT("Hero.Branth,Hero.Ivrin")));
	TestEqual(TEXT("e scavalcare la squadra 0 non tocca la squadra 1"),
		JoinRoster(GameMode->ResolveTeamHeroes(1)), Proprieta1);

	// Con gli spazi vogliono le virgolette, e le voci si rifilano.
	Sorgenti.SetArgs(TEXT("-RTTeam0=\"Hero.Branth, Hero.Ivrin\""));
	TestEqual(TEXT("fra virgolette gli spazi attorno alle voci non contano"),
		JoinRoster(GameMode->ResolveTeamHeroes(0)), FString(TEXT("Hero.Branth,Hero.Ivrin")));

	// L'altra squadra ha il suo token.
	Sorgenti.SetArgs(TEXT("-RTTeam1=Hero.Aevik,Hero.Muiren"));
	TestEqual(TEXT("-RTTeam1= sceglie la squadra 1"),
		JoinRoster(GameMode->ResolveTeamHeroes(1)), FString(TEXT("Hero.Aevik,Hero.Muiren")));
	TestEqual(TEXT("e non tocca la squadra 0"), JoinRoster(GameMode->ResolveTeamHeroes(0)), Proprieta0);

	// Console contro riga di comando: vince la console, che e' l'intento di ADESSO.
	Sorgenti.SetArgs(TEXT("-RTTeam0=Hero.Branth,Hero.Ivrin"));
	Sorgenti.Team0.Imposta(FString(TEXT("Hero.Ivrin,Hero.Branth")));
	TestEqual(TEXT("console + flag -> vince la console"),
		JoinRoster(GameMode->ResolveTeamHeroes(0)), FString(TEXT("Hero.Ivrin,Hero.Branth")));

	// Tolta la console torna il flag; tolto il flag torna la proprieta': la precedenza non consuma niente.
	Sorgenti.Team0.Imposta(FString());
	TestEqual(TEXT("tolta la console, vale di nuovo il flag"),
		JoinRoster(GameMode->ResolveTeamHeroes(0)), FString(TEXT("Hero.Branth,Hero.Ivrin")));
	Sorgenti.ClearArgs();
	TestEqual(TEXT("tolto il flag, vale di nuovo la proprieta'"),
		JoinRoster(GameMode->ResolveTeamHeroes(0)), Proprieta0);

	// ⛔ I casi sbagliati restano sbagliati: non ricadono sulla proprieta'.
	Sorgenti.SetArgs(TEXT("-RTTeam0="));
	TestEqual(TEXT("-RTTeam0= senza valore e' una sorgente presente con una lista VUOTA, non la proprieta'"),
		GameMode->ResolveTeamHeroes(0).Num(), 0);
	Sorgenti.SetArgs(TEXT("-RTTeam0=Hero.Branth,"));
	TestEqual(TEXT("una virgola in coda lascia una voce vuota: la lista non si accorcia da sola"),
		JoinRoster(GameMode->ResolveTeamHeroes(0)), FString(TEXT("Hero.Branth,<vuota>")));
	Sorgenti.ClearArgs();

	TestEqual(TEXT("una squadra che non esiste non ha un roster"), GameMode->ResolveTeamHeroes(2).Num(), 0);

	DestroyRosterWorld(World);
	return true;
}

/**
 * Il roster chiesto dalla console ENTRA IN CAMPO: la catena vera, da `SetupHexMatch` alle unita'.
 *
 * 🔑 **Le due squadre si SCAMBIANO gli eroi.** Un roster qualunque diverso dal default basterebbe a vedere
 * «qualcosa e' cambiato»; lo scambio dice anche che ogni lista e' arrivata alla PROPRIA squadra — due liste
 * incrociate in `SetupHexMatch` darebbero gli stessi quattro eroi in campo e il test cadrebbe lo stesso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRosterFromConsoleEntersTheFieldTest,
	"RefactorTactics.Match.Roster.ConsoleRosterEntersTheField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRosterFromConsoleEntersTheFieldTest::RunTest(const FString&)
{
	FRTScopedRosterSources Sorgenti;

	// CONTROLLO: senza sorgenti in campo c'e' il default. Senza, «Branth in squadra 0» sarebbe vero anche se
	// il default fosse cambiato.
	FString Default0;
	{
		UWorld* World = MakeRosterWorld();
		if (!TestNotNull(TEXT("mondo di controllo"), World)) { return false; }
		ARTHexMapActor* HexMap = SpawnRosterMap(World);
		ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
		if (!HexMap || !GameMode) { DestroyRosterWorld(World); return TestTrue(TEXT("mappa e GameMode"), false); }
		GameMode->SetupHexMatch(HexMap);
		Default0 = HeroIdsInPlay(CollectRosterUnits(World), 0);
		DestroyRosterWorld(World);
	}
	if (!TestEqual(TEXT("controllo: di default la squadra 0 e' Aevik e Muiren"),
			Default0, FString(TEXT("Hero.Aevik,Hero.Muiren"))))
	{
		return false;
	}

	UWorld* World = MakeRosterWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ARTHexMapActor* HexMap = SpawnRosterMap(World);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!HexMap || !GameMode) { DestroyRosterWorld(World); return TestTrue(TEXT("mappa e GameMode"), false); }

	Sorgenti.Team0.Imposta(FString(TEXT("Hero.Branth,Hero.Ivrin")));
	Sorgenti.Team1.Imposta(FString(TEXT("Hero.Aevik,Hero.Muiren")));
	GameMode->SetupHexMatch(HexMap);

	const TArray<ARTUnit*> Units = CollectRosterUnits(World);
	TestEqual(TEXT("quattro unita' in campo"), Units.Num(), 4);
	TestEqual(TEXT("la squadra 0 e' quella chiesta dalla console"),
		HeroIdsInPlay(Units, 0), FString(TEXT("Hero.Branth,Hero.Ivrin")));
	TestEqual(TEXT("e la squadra 1 pure: ogni lista e' arrivata alla PROPRIA squadra"),
		HeroIdsInPlay(Units, 1), FString(TEXT("Hero.Aevik,Hero.Muiren")));

	DestroyRosterWorld(World);
	return true;
}

/**
 * Un `HeroId` sconosciuto ferma l'allestimento DA QUALUNQUE SORGENTE, e l'errore dice quale.
 *
 * 🔴 **Prima il roster aveva una sorgente sola, e «correggi `Team0Heroes`» era sempre il consiglio giusto.**
 * Con tre, manderebbe ad aprire un Blueprint chi ha scritto male una riga di comando. Il dettaglio del
 * fatale porta ora la sorgente — ed e' cio' che questo test misura, tre volte: proprieta', flag, console.
 *
 * ⚠️ **Il conteggio resta quello del formato** in tutti e tre i casi (due voci): cosi' la guardia che si
 * esercita e' quella del NOME, non quella della cardinalita' che sta venti righe sopra.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRosterUnknownHeroNamesItsSourceTest,
	"RefactorTactics.Match.Roster.UnknownHeroAbortsAndNamesItsSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRosterUnknownHeroNamesItsSourceTest::RunTest(const FString&)
{
	struct FCaso
	{
		const TCHAR* Nome;
		const TCHAR* Args;       // riga di comando, o nullptr
		const TCHAR* Console;    // rt.Match.Team0, o nullptr
		const TCHAR* SorgenteAttesa;
	};
	const FCaso Casi[] = {
		{ TEXT("proprieta'"),      nullptr,                                   nullptr,                           TEXT("Team0Heroes") },
		{ TEXT("riga di comando"), TEXT("-RTTeam0=Hero.Aevik,Hero.NonEsiste"), nullptr,                           TEXT("-RTTeam0=") },
		{ TEXT("console"),         nullptr,                                   TEXT("Hero.Aevik,Hero.NonEsiste"), TEXT("rt.Match.Team0") },
	};

	AddExpectedError(TEXT("non e' nel catalogo eroi"), EAutomationExpectedErrorFlags::Contains, UE_ARRAY_COUNT(Casi));

	for (const FCaso& Caso : Casi)
	{
		FRTScopedRosterSources Sorgenti;
		UWorld* World = MakeRosterWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ARTHexMapActor* HexMap = SpawnRosterMap(World);
		ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
		if (!HexMap || !GameMode) { DestroyRosterWorld(World); return TestTrue(TEXT("mappa e GameMode"), false); }

		if (Caso.Args) { Sorgenti.SetArgs(Caso.Args); }
		if (Caso.Console) { Sorgenti.Team0.Imposta(FString(Caso.Console)); }
		if (!Caso.Args && !Caso.Console) { GameMode->Team0Heroes = { TEXT("Hero.Aevik"), TEXT("Hero.NonEsiste") }; }

		GameMode->SetupHexMatch(HexMap);

		TestEqual(FString::Printf(TEXT("%s: nessuna unita' in campo"), Caso.Nome),
			CollectRosterUnits(World).Num(), 0);
		const FRTStartupReport& Report = GameMode->GetStartupReport();
		TestEqual(FString::Printf(TEXT("%s: l'avvio dichiara il fatale"), Caso.Nome),
			URTStartupReportLibrary::FindFatal(Report), ERTStartupOutcome::RosterHeroMissing);
		const FString Dettaglio = MissingHeroDetail(Report);
		TestTrue(FString::Printf(TEXT("%s: il dettaglio nomina l'eroe (%s)"), Caso.Nome, *Dettaglio),
			Dettaglio.Contains(TEXT("Hero.NonEsiste")));
		TestTrue(FString::Printf(TEXT("%s: e nomina la SORGENTE '%s' (%s)"), Caso.Nome, Caso.SorgenteAttesa, *Dettaglio),
			Dettaglio.Contains(Caso.SorgenteAttesa));

		DestroyRosterWorld(World);
	}
	return true;
}

/**
 * Una lista scritta male FERMA l'allestimento: non riparte la partita di default al posto di quella chiesta.
 *
 * 🔴 **E' la meta' di [D-507] che un resolver «gentile» romperebbe.** La proprieta' sotto e' valida — Aevik e
 * Muiren — quindi ignorare una sorgente malformata e ripiegare darebbe una partita che parte e SEMBRA
 * normale, con gli eroi sbagliati: il difetto piu' difficile da vedere a schermo. Tre forme, tutte con la
 * proprieta' intatta sotto, e in campo non deve entrare nessuno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRosterMalformedListAbortsTest,
	"RefactorTactics.Match.Roster.MalformedListAbortsInsteadOfFallingBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRosterMalformedListAbortsTest::RunTest(const FString&)
{
	struct FCaso
	{
		const TCHAR* Nome;
		const TCHAR* Args;
		const TCHAR* ErroreAtteso;
	};
	const FCaso Casi[] = {
		{ TEXT("una voce sola, il formato ne schiera due"), TEXT("-RTTeam0=Hero.Aevik"),  TEXT("ne dichiara 1") },
		{ TEXT("lista vuota"),                              TEXT("-RTTeam0="),            TEXT("ne dichiara 0") },
		{ TEXT("virgola in coda: una voce vuota"),          TEXT("-RTTeam0=Hero.Aevik,"), TEXT("<voce vuota>") },
	};

	for (const FCaso& Caso : Casi)
	{
		FRTScopedRosterSources Sorgenti;
		UWorld* World = MakeRosterWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ARTHexMapActor* HexMap = SpawnRosterMap(World);
		ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
		if (!HexMap || !GameMode) { DestroyRosterWorld(World); return TestTrue(TEXT("mappa e GameMode"), false); }

		// La proprieta' sotto e' il default VALIDO: e' cio' su cui un ripiego silenzioso atterrerebbe.
		Sorgenti.SetArgs(Caso.Args);
		AddExpectedError(Caso.ErroreAtteso, EAutomationExpectedErrorFlags::Contains, 1);
		GameMode->SetupHexMatch(HexMap);

		TestEqual(FString::Printf(TEXT("%s: nessuna unita' in campo — nemmeno quelle della proprieta'"), Caso.Nome),
			CollectRosterUnits(World).Num(), 0);

		DestroyRosterWorld(World);
	}
	return true;
}

/**
 * A parita' di roster la partita e' LA STESSA da qualunque sorgente arrivi.
 *
 * 🔑 **L'oracolo e' il TurnLog serializzato del primo turno, checksum compreso — non la lista in campo**:
 * due allestimenti con gli stessi eroi ma in ordine diverso avrebbero la stessa lista e partite diverse,
 * perche' l'ordine decide le celle di partenza. E' lo stesso oracolo del gate di determinismo
 * dell'autobattle. Il controllo in coda (un roster DIVERSO, traccia diversa) dice che la traccia guarda
 * davvero chi gioca: senza, due tracce uguali sarebbero vere anche per due partite che il roster non tocca.
 *
 * ⚠️ Non `GetPendingFinalStateHash()`: quello si cattura solo mentre un replay registra, e qui vale zero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRosterSameMatchWhateverTheSourceTest,
	"RefactorTactics.Match.Roster.SameRosterSameMatchWhateverTheSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRosterSameMatchWhateverTheSourceTest::RunTest(const FString&)
{
	enum class ESorgente : uint8 { Proprieta, Console, RigaDiComando, DefaultDiControllo };

	auto GiocaUnTurno = [this](ESorgente Sorgente, TArray<uint8>& OutTraccia) -> bool
	{
		FRTScopedRosterSources Sorgenti;
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!World) { return false; }
		ARTHexMapActor* HexMap = SpawnRosterMap(World, /*Radius=*/ 5);
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
		if (!HexMap || !TM || !GameMode) { RTWorldFixtures::DestroyWorld(World); return false; }
		TM->DispatchBeginPlay();

		// Nessuno gioca: i quattro eroi sono dei bot, e un turno si chiude senza input.
		GameMode->bAutobattle = true;
		switch (Sorgente)
		{
		case ESorgente::Proprieta:
			GameMode->Team0Heroes = { TEXT("Hero.Branth"), TEXT("Hero.Ivrin") };
			GameMode->Team1Heroes = { TEXT("Hero.Aevik"), TEXT("Hero.Muiren") };
			break;
		case ESorgente::Console:
			Sorgenti.Team0.Imposta(FString(TEXT("Hero.Branth,Hero.Ivrin")));
			Sorgenti.Team1.Imposta(FString(TEXT("Hero.Aevik,Hero.Muiren")));
			break;
		case ESorgente::RigaDiComando:
			Sorgenti.SetArgs(TEXT("-RTTeam0=Hero.Branth,Hero.Ivrin -RTTeam1=Hero.Aevik,Hero.Muiren"));
			break;
		case ESorgente::DefaultDiControllo:
			break;
		}

		GameMode->SetupHexMatch(HexMap);
		const bool bInCampo = CollectRosterUnits(World).Num() == 4;
		if (bInCampo)
		{
			RTWorldFixtures::PlayOneTurn(TM);
			OutTraccia = URTTurnLogLibrary::SerializeTurnLog(TM->GetTurnLog(), ERTLogTopology::Hex);
		}
		RTWorldFixtures::DestroyWorld(World);
		return bInCampo;
	};

	TArray<uint8> DaProprieta, DaConsole, DaRigaDiComando, DiControllo;
	if (!TestTrue(TEXT("premessa: la partita dalla proprieta' si allestisce"), GiocaUnTurno(ESorgente::Proprieta, DaProprieta))
		|| !TestTrue(TEXT("premessa: la partita dalla console si allestisce"), GiocaUnTurno(ESorgente::Console, DaConsole))
		|| !TestTrue(TEXT("premessa: la partita dalla riga di comando si allestisce"), GiocaUnTurno(ESorgente::RigaDiComando, DaRigaDiComando))
		|| !TestTrue(TEXT("premessa: la partita di controllo si allestisce"), GiocaUnTurno(ESorgente::DefaultDiControllo, DiControllo)))
	{
		return false;
	}

	if (!TestTrue(TEXT("premessa: il primo turno ha lasciato una traccia"), DaProprieta.Num() > 0)) { return false; }
	TestTrue(TEXT("stesso roster dalla console -> stessa traccia del primo turno, byte per byte"),
		DaConsole == DaProprieta);
	TestTrue(TEXT("stesso roster dalla riga di comando -> stessa traccia"), DaRigaDiComando == DaProprieta);
	// ⛔ Il controllo che rende vere le due righe qui sopra: un roster diverso da' una partita diversa.
	TestFalse(TEXT("controllo: il roster di default da' una traccia DIVERSA"), DiControllo == DaProprieta);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
