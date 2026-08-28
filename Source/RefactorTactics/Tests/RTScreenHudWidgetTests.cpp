// Le classi BASE dei widget dello Screen HUD (§4.1, CP 11.7 / #613).
//
// Cio' che questi test possono provare e' la SUPERFICIE: cosa un Blueprint puo' leggere, e cosa non trova
// perche' non esiste. Il layout, l'aspetto e il «centro libero» stanno nel `.uasset` e restano a
// `PIE-V01-HUD` — per costruzione, non per rinuncia.

#include "Misc/AutomationTest.h"
#include "UI/RTScreenHudWidgets.h"
#include "UI/RTIconLibrary.h"
#include "UI/RTIconCatalogData.h" // URTIconCatalogData: esplicito, non ereditato da RTIconLibrary.h
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnRules.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"

#if WITH_DEV_AUTOMATION_TESTS

// ⚠️ Le basi NON sono `Abstract`, e questa e' la ragione: `UCLASS()` non si puo' dichiarare in un `.cpp`
// (UHT processa solo gli header), quindi delle sottoclassi concrete di comodo non sono scrivibili qui — e
// un header di test dentro il modulo di gioco costerebbe piu' di quanto valga. Istanziabili direttamente,
// le classi si guidano dal test; in gioco restano comunque da derivare, perche' senza layout non disegnano
// nulla.

namespace
{
	UWorld* MakeHudWidgetWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyHudWidgetWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}
}

/**
 * Il contatore di round distingue TRE stati, e i due che si confondono sono quelli che contano.
 *
 * `RoundLimit == 0` non e' «su zero»: una partita senza formato non e' una partita gia' scaduta. E senza
 * contesto non si mostra `Round 0`, che sembra un dato, ma un trattino.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudRoundTextTest,
	"RefactorTactics.ScreenHud.RoundCounterDistinguishesNoLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudRoundTextTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTTurnHeaderWidget* Header = NewObject<URTTurnHeaderWidget>(World);
	if (!TestNotNull(TEXT("widget"), Header)) { DestroyHudWidgetWorld(World); return false; }

	// 1. Senza contesto: un trattino, non un numero.
	TestEqual(TEXT("senza contesto mostra un trattino"),
		Header->GetRoundCounterText().ToString(), FString(TEXT("—")));
	TestFalse(TEXT("e dichiara di non avere contesto"), Header->HasMatchContext());

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHudWidgetWorld(World); return false; }
	Header->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);

	// 2. Con un limite dichiarato: `Round N/Limite`.
	{
		FRTMatchRules Rules;
		Rules.RoundLimit = 9;
		TM->SetMatchRules(Rules);
		TestTrue(TEXT("col limite, il testo lo mostra"),
			Header->GetRoundCounterText().ToString().Contains(TEXT("/9")));
	}

	// 3. 🔴 Senza limite: NIENTE «/0». E' il caso che un binding ingenuo sbaglia, e si legge come una
	//    partita gia' finita.
	{
		FRTMatchRules Rules;
		Rules.RoundLimit = 0;
		TM->SetMatchRules(Rules);
		const FString Text = Header->GetRoundCounterText().ToString();
		TestFalse(TEXT("senza limite non compare uno «/0»"), Text.Contains(TEXT("/")));
		TestTrue(TEXT("ma il round c'e' comunque"), Text.Contains(TEXT("Round")));
	}

	DestroyHudWidgetWorld(World);
	return true;
}

/**
 * Lo slot azione porta la CHIAVE dell'icona, mai un asset — ed e' la chiave che il catalogo si aspetta.
 *
 * Il caso `None` non e' difensivo: le azioni create in codice prima del motore azioni non hanno `ActionId`,
 * e comporre `UI.Icon.` a vuoto produrrebbe una risoluzione che nomina una chiave mai dichiarata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudIconKeyTest,
	"RefactorTactics.ScreenHud.ActionSlotCarriesAnIconKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudIconKeyTest::RunTest(const FString&)
{
	URTActionSlotWidget* Slot = NewObject<URTActionSlotWidget>();
	if (!TestNotNull(TEXT("widget"), Slot)) { return false; }

	// Senza azione: nessuna chiave.
	TestEqual(TEXT("uno slot vuoto non inventa una chiave"), Slot->GetIconId(), FName(NAME_None));

	FRTAbilityCooldownView Action;
	Action.ActionId = TEXT("Action.Move");
	Slot->SetAction(Action, /*bArmed=*/ true);

	TestTrue(TEXT("lo slot registra l'azione"), Slot->Action.ActionId == FName(TEXT("Action.Move")));
	TestTrue(TEXT("e lo stato armato"), Slot->bArmed);

	// La chiave e' quella che il catalogo si aspetta: si CHIEDE a `MakeIconId`, non si compone qui.
	TestEqual(TEXT("la chiave e' quella del catalogo"),
		Slot->GetIconId(), URTIconLibrary::MakeIconId(TEXT("Action.Move")));

	DestroyHudWidgetWorld(nullptr); // no-op: questo test non ha un mondo
	return true;
}

/**
 * 🔴 **Nessuna proprieta' dei widget e' una texture.** E' la voce di DoD «nessun widget referenzia una
 * texture direttamente» (D-031, `#220`) resa verificabile invece che raccomandata.
 *
 * Il controllo gira sulla REFLECTION, non sul testo del sorgente: una `UPROPERTY` aggiunta domani viene
 * vista anche se nessuno rilegge questo file.
 *
 * ⚠️ **Il limite va detto, perche' e' meta' della verita'.** Copre la superficie C++. Un Blueprint derivato
 * puo' sempre aggiungersi una variabile `Texture2D`, e nessun gate lo impedisce: i `.uasset` non sono
 * versionati in questo repository. La guida lo scrive come regola per chi costruisce i `WBP_RT_*`; questo
 * test tiene la parte che il codice controlla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudNoTextureTest,
	"RefactorTactics.ScreenHud.WidgetApiExposesNoTexture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudNoTextureTest::RunTest(const FString&)
{
	const TArray<UClass*> WidgetClasses = {
		URTScreenHudWidgetBase::StaticClass(),
		URTTurnHeaderWidget::StaticClass(),
		URTTeamRosterWidget::StaticClass(),
		URTSelectedUnitPanelWidget::StaticClass(),
		URTActionDockWidget::StaticClass(),
		URTActionSlotWidget::StaticClass(),
		URTTacticalHUDWidget::StaticClass(),
	};

	// Controprova della premessa: se l'elenco fosse vuoto o le classi non si risolvessero, il test passerebbe
	// senza guardare niente — il difetto che questo repository chiama «test che smette di verificare».
	TestEqual(TEXT("le sette classi si risolvono"), WidgetClasses.Num(), 7);
	for (const UClass* Class : WidgetClasses)
	{
		if (!TestNotNull(TEXT("classe risolta"), Class)) { return false; }
	}

	int32 Inspected = 0;
	for (const UClass* Class : WidgetClasses)
	{
		// Solo le proprieta' DICHIARATE da queste classi: `UUserWidget` ne porta di sue (brush di stile,
		// cursori) che non sono nostre e non sono il difetto che il DoD vieta.
		for (TFieldIterator<FProperty> It(Class, EFieldIterationFlags::None); It; ++It)
		{
			const FProperty* Prop = *It;
			++Inspected;

			const FObjectPropertyBase* AsObject = CastField<FObjectPropertyBase>(Prop);
			if (AsObject && AsObject->PropertyClass
				&& AsObject->PropertyClass->IsChildOf(UTexture2D::StaticClass()))
			{
				AddError(FString::Printf(
					TEXT("%s::%s e' una texture: le icone viaggiano per CHIAVE (`UI.Icon.*`) e si risolvono ")
					TEXT("dal catalogo (D-031). Un widget che riceve un asset rende la regola una ")
					TEXT("raccomandazione."),
					*Class->GetName(), *Prop->GetName()));
			}
		}
	}

	// Senza questa riga il test sarebbe verde anche se l'iterazione non vedesse nulla.
	TestTrue(TEXT("l'iterazione ha davvero guardato delle proprieta'"), Inspected > 0);

	return true;
}

/**
 * Il dock spento e' uno stato REALE, non un caso limite: e' il neutro di D-128, quello in cui nessuna azione
 * e' armata e un click su un nemico ispeziona. Senza selezione vale lo stesso, e le due cose coincidono di
 * proposito — a schermo il giocatore deve vedere la stessa cosa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudDockNeutralTest,
	"RefactorTactics.ScreenHud.ActionDockShowsTheNeutralState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudDockNeutralTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTActionDockWidget* Dock = NewObject<URTActionDockWidget>(World);
	if (!TestNotNull(TEXT("widget"), Dock)) { DestroyHudWidgetWorld(World); return false; }

	TestEqual(TEXT("senza selezione nessuna azione e' armata"),
		Dock->GetArmedActionIndex(), (int32)INDEX_NONE);
	TestEqual(TEXT("e il dock e' vuoto"), Dock->GetActions().Num(), 0);

	DestroyHudWidgetWorld(World);
	return true;
}

/**
 * `GetIconCatalog` risale alla radice, e la radice legge SE STESSA.
 *
 * 🔴 **I tre rami sono asseriti insieme perche' due di loro si coprono a vicenda in modo ingannevole.**
 * Un'implementazione col solo `GetTypedOuter` passerebbe il caso del dock e fallirebbe **solo** sul
 * `TacticalHUD` — cioe' l'unico widget che il catalogo ce l'ha davvero. Un'implementazione col solo
 * `Cast<>(this)` farebbe l'opposto. Testarne uno alla volta lascerebbe verde meta' del difetto.
 *
 * ⚠️ Il terzo caso — outer senza HUD — non e' un contorno: e' la condizione in cui il widget vive in un
 * test o in un'anteprima d'editor, e la risposta corretta e' `nullptr` **senza crash**. Chi consuma passa
 * da `ResolveIcon`, che con catalogo nullo da' il missing-icon e logga la chiave.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudIconCatalogReachTest,
	"RefactorTactics.ScreenHud.IconCatalogReachesTheRoot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudIconCatalogReachTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTIconCatalogData* Catalog = NewObject<URTIconCatalogData>(World);
	URTTacticalHUDWidget* Root = NewObject<URTTacticalHUDWidget>(World);
	if (!TestNotNull(TEXT("catalogo"), Catalog) || !TestNotNull(TEXT("radice"), Root))
	{
		DestroyHudWidgetWorld(World);
		return false;
	}
	Root->IconCatalog = Catalog;

	// (1) La radice legge il PROPRIO catalogo. `GetTypedOuter` cerca fra gli outer e non guarda `this`:
	// senza il ramo dedicato, il widget su cui il dato vive sarebbe l'unico a non vederlo.
	TestEqual(TEXT("la radice legge il proprio catalogo"),
		Root->GetIconCatalog(), (const URTIconCatalogData*)Catalog);

	// (2) Un figlio che ha la radice per outer lo raggiunge risalendo. E' il caso reale del dock innestato
	// nel `WBP_RT_TacticalHUD`.
	const URTActionDockWidget* Figlio = NewObject<URTActionDockWidget>(Root);
	if (TestNotNull(TEXT("figlio della radice"), Figlio))
	{
		TestEqual(TEXT("il figlio risale alla radice"),
			Figlio->GetIconCatalog(), (const URTIconCatalogData*)Catalog);
	}

	// (3) Fuori dall'HUD: `nullptr`, e nessun crash. Controprova della premessa — se questo caso desse il
	// catalogo, i due sopra sarebbero veri per costruzione invece che per meccanismo.
	const URTActionDockWidget* Orfano = NewObject<URTActionDockWidget>(World);
	if (TestNotNull(TEXT("widget fuori dall'HUD"), Orfano))
	{
		TestNull(TEXT("fuori dall'HUD il catalogo non si raggiunge"), Orfano->GetIconCatalog());
	}

	DestroyHudWidgetWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
