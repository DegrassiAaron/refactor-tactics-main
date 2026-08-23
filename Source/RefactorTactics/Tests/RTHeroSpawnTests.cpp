#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h" // GetGenericActionIds: il kit e' eroe + generiche (D-025)
#include "Ability/RTEquipmentData.h" // ERTEquipmentSlot: una variante MODIFICA l'attacco base, non lo accoda
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Frontend/RTStartupReport.h"
#include "RTGameMode.h"
#include "Unit/RTUnit.h"

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
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();

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
	TestTrue(TEXT("c'e' Gadget"), InPlay.Contains(FName(TEXT("Hero.Gadget"))));
	TestTrue(TEXT("c'e' Phase"), InPlay.Contains(FName(TEXT("Hero.Phase"))));
	TestTrue(TEXT("c'e' Riktor"), InPlay.Contains(FName(TEXT("Hero.Riktor"))));
	TestTrue(TEXT("c'e' Wraith"), InPlay.Contains(FName(TEXT("Hero.Wraith"))));

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
		TestEqual(FString::Printf(TEXT("%s: azioni dell'eroe piu' generiche piu' il loadout"), *Who),
			Unit->NumAbilities(),
			5 + URTCatalogLibrary::GetGenericActionIds().Num() + PezziCheConcedono);
		TestEqual(FString::Printf(TEXT("%s: l'indice 0 resta l'attacco base"), *Who),
			Unit->GetAbility(0)->Def.ActionId, Hero->Actions[0]->Def.ActionId);
	}

	// Formazione di default: Gadget+Phase (giocatore) contro Riktor+Wraith (bot).
	ARTUnit* Gadget = FindByHeroId(Units, TEXT("Hero.Gadget"));
	ARTUnit* Phase = FindByHeroId(Units, TEXT("Hero.Phase"));
	ARTUnit* Riktor = FindByHeroId(Units, TEXT("Hero.Riktor"));
	ARTUnit* Wraith = FindByHeroId(Units, TEXT("Hero.Wraith"));
	if (Gadget && Phase && Riktor && Wraith)
	{
		TestEqual(TEXT("Gadget e' del giocatore"), Gadget->TeamId, 0);
		TestEqual(TEXT("Phase anche: la combo Wet e' giocabile"), Phase->TeamId, 0);
		TestEqual(TEXT("Riktor e' del bot"), Riktor->TeamId, 1);
		TestEqual(TEXT("Wraith anche"), Wraith->TeamId, 1);
		TestFalse(TEXT("il giocatore comanda i suoi"), Gadget->bIsBotControlled);
		TestTrue(TEXT("il bot comanda i propri"), Riktor->bIsBotControlled);

		// **Il punto della DoD sul bot**: MP diversi arrivano davvero in campo. Il budget dello snapshot viene
		// da `MoveRange`, quindi il bot non puo' proporre a Riktor una mossa da 6 celle.
		TestEqual(TEXT("Riktor: 4 MP"), Riktor->MoveRange, 4);
		TestEqual(TEXT("Wraith: 6 MP"), Wraith->MoveRange, 6);
		// ⚠️ **La portata ora dipende dalla VARIANTE D'ARMA, e il valore atteso si deriva** (`#1054`).
		// Gadget non ha un loadout — §4 gli assegna `Gadget.Insulator`, che v0.1 non costruisce — quindi
		// resta a 4, il numero del catalogo. Riktor monta `Weapon.Impact`, che toglie una cella: **3 → 2**.
		// Il `2` non e' scritto qui: lo produce `ApplyWeaponVariant`, cosi' un ribilanciamento della
		// variante fa cadere il catalogo e non questo file, e il giorno in cui Gadget avra' il suo gadget
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
		TestEqual(TEXT("Gadget colpisce alla portata del catalogo (nessun loadout)"),
			Gadget->AttackRange, PortataAttesa(TEXT("Hero.Gadget")));
		TestEqual(TEXT("Riktor colpisce alla portata ridotta dalla sua variante"),
			Riktor->AttackRange, PortataAttesa(TEXT("Hero.Riktor")));
		// E che le due NON siano lo stesso numero: senza questa riga il lambda potrebbe restituire sempre
		// la portata base e le due asserzioni sopra passerebbero senza dimostrare che la variante morde.
		TestNotEqual(TEXT("la variante d'arma cambia davvero la portata di Riktor"),
			Riktor->AttackRange, URTHeroCatalogLibrary::MakeRiktor()->Actions[0]->Def.RangeCells);

		// Ogni unita' ha le PROPRIE istanze d'azione: due eroi che condividessero un `URTActionData`
		// ricaricherebbero insieme.
		TestTrue(TEXT("azioni non condivise"), Gadget->GetAbility(0) != Phase->GetAbility(0));

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

	// Gadget in ENTRAMBE le squadre: la formazione dichiara quattro slot, ma i nomi distinti sono tre.
	// Due unita' che condividessero un `URTHeroData` ricaricherebbero insieme le stesse azioni.
	GameMode->Team0Heroes = { TEXT("Hero.Gadget"), TEXT("Hero.Phase") };
	GameMode->Team1Heroes = { TEXT("Hero.Gadget"), TEXT("Hero.Wraith") };

	GameMode->SetupHexMatch(HexMap);

	const TArray<ARTUnit*> Units = CollectRosterUnits(World);
	TestEqual(TEXT("tre unita' in campo: la copia non entra"), Units.Num(), 3);

	TSet<FName> InPlay;
	for (const ARTUnit* Unit : Units) { InPlay.Add(Unit->HeroId); }
	TestTrue(TEXT("Gadget, una volta sola"), InPlay.Contains(FName(TEXT("Hero.Gadget"))));
	TestTrue(TEXT("e Wraith"), InPlay.Contains(FName(TEXT("Hero.Wraith"))));
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
 * ⚠️ **Metà roster non ha un loadout, e non è un difetto di questo test.** §4 assegna a Gadget e Wraith
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
				Unit->NumAbilities(), 5 + Generiche);
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

		// 3. Due azioni in più delle cinque dell'eroe: gadget e modulo. La variante non conta, perché
		//    modifica invece di aggiungere — ed è la distinzione che l'harness sbagliava.
		TestEqual(*FString::Printf(TEXT("%s: cinque azioni piu' generiche piu' i due pezzi"), *Who),
			Unit->NumAbilities(), 5 + Generiche + 2);
	}

	// Misura, non uguaglianza: sale da sé quando E36 ed E13 sbloccano i gadget mancanti. Zero significherebbe
	// che nessuno equipaggia, cioè che questa fetta non ha consegnato niente.
	TestTrue(*FString::Printf(TEXT("almeno un'unita' entra equipaggiata (oggi: %d)"), Equipaggiati),
		Equipaggiati > 0);

	// **Il bot come chiunque altro.** Non c'è un ramo per lui: `SpawnUnitForHero` è il punto unico, e questa
	// asserzione è ciò che impedisce a un `if (!bIsBotControlled)` di rientrare domani senza far rosso.
	for (const ARTUnit* Unit : Units)
	{
		if (!Unit || !Unit->bIsBotControlled) { continue; }
		const TArray<FName> Loadout = URTCatalogLibrary::DefaultLoadoutFor(Unit->HeroId);
		if (Loadout.Num() == 0) { continue; }
		TestEqual(*FString::Printf(TEXT("bot %s: equipaggiato quanto il giocatore"),
			*Unit->HeroId.ToString()),
			Unit->NumAbilities(), 5 + URTCatalogLibrary::GetGenericActionIds().Num() + 2);
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
	GameMode->Team0Heroes = { TEXT("Hero.Gadget"), TEXT("Hero.NonEsiste") };

	AddExpectedError(TEXT("non e' nel catalogo eroi"), EAutomationExpectedErrorFlags::Contains, 1);
	GameMode->SetupHexMatch(HexMap);

	// **Zero, non tre.** Gadget precede il nome ignoto e la squadra 1 e' intatta: senza la guardia in campo
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

	GameMode->Team1Heroes = { TEXT("Hero.Riktor"), TEXT("Hero.NonEsiste") };

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

#endif // WITH_DEV_AUTOMATION_TESTS
