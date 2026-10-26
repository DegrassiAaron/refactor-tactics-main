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
#include "Turn/RTTurnLog.h" // FRTTurnLogEntry: il feed si prova iniettando una voce nel log
#include "Unit/RTUnit.h" // ARTUnit: e' una delle classi AUTOREVOLI che nessun widget deve esporre
#include "Tests/RTAbilityFixtures.h" // AddCoreAbility: il chip REAZ. si prova su piani veri (#3618)
#include "UI/RTReactionWindowViewModel.h" // idem, ed e' quella che porterebbe `SubmitResponse` nel grafo
#include "UI/RTHudViewModel.h"           // il feed si prova anche SOTTO il widget: l'insieme vuoto (#2744)
#include "UI/RTPlayerEventProjector.h"   // IsAuthorized: il predicato si interroga da solo, ed e' il punto
#include "UI/RTHUD.h"                    // ComposeAbilityLine: l'oracolo dell'uguaglianza, non una seconda riga
#include "Misc/ScopeExit.h"              // ON_SCOPE_EXIT: il mondo si distrugge anche sui ritorni anticipati
#include "Kismet/GameplayStatics.h"      // GetActorOfClass: la premessa «nessun manager nel mondo» si asserisce (#2744)
#include "Components/Image.h"          // il brush dello slot si legge, non si deduce (#3178)
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "UObject/UObjectIterator.h" // TObjectIterator: le classi widget si interrogano, non si elencano
#include "Blueprint/WidgetTree.h"    // l'albero si COSTRUISCE qui: e' l'unico modo di provare la ricorsione
#include "Components/VerticalBox.h" // un pannello vero: serve per dare a UN widget DUE strade nell'albero
#include "UI/RTUIPalette.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h" // GLog->AddOutputDevice: l'assenza di avvisi si misura con un dispositivo di log
#include "Misc/ScopeLock.h"
#include "HAL/PlatformTLS.h"             // il rilevatore conta solo le righe del thread che lo crea

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
 * 🔴 **Lo slot dice il TASTO, lo stato armato e il MOTIVO — e li dice in TESTO**, che e' il canale non
 * cromatico che `#2826` chiede: *«uno slot in cooldown o non disponibile e' distinguibile **senza**
 * affidarsi al colore»*.
 *
 * L'oracolo non e' che la riga «ci sia»: sono le tre sottostringhe, ciascuna legata a un campo della vista.
 * Una riga che perdesse il numero passerebbe un test scritto come *«non e' vuota»* — e il giocatore non
 * saprebbe piu' quale tasto arma quello slot.
 *
 * ⚠️ **Il tasto si DICHIARA nella vista, e non si deduce piu' da `AbilityIndex`** (`#2987`). Fino a quel
 * punto `ComposeAbilityLine` scriveva `Index + 1`, e questo test lo confermava passando `AbilityIndex = 3`
 * e cercando `"4. "`: l'uguaglianza reggeva **per costruzione** su ogni posizione centrale, e nessuna delle
 * due che sbagliano — la decima, dove il tasto e' `0`, e l'undicesima, che nessun tasto raggiunge — era
 * provata. Ora `HotkeyLabel` viene da `AbilityHotkeys()` e le due sono coperte da
 * `HudViewModel.ShortcutComesFromTheBindingTableNotTheIndex`.
 *
 * ⛔ **Il tasto GENERICO resta fuori.** `Action.Wait` si arma anche con `Z` (`GenericHotkeys()`), e quale
 * dei due binding uno slot debba mostrare non e' deciso da nessuna fonte del progetto: e' `#2990`,
 * domanda 4. Questo test pinna il numero perche' e' cio' che la vista porta oggi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudActionLineTest,
	"RefactorTactics.ScreenHud.ActionSlotLineCarriesKeyArmedAndReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudActionLineTest::RunTest(const FString&)
{
	URTActionSlotWidget* Slot = NewObject<URTActionSlotWidget>();
	if (!TestNotNull(TEXT("widget"), Slot)) { return false; }

	FRTAbilityCooldownView Action;
	Action.ActionId = TEXT("Action.Overload");
	Action.DisplayName = FText::FromString(TEXT("Sovraccarico"));
	Action.AbilityIndex = 3;        // quarta posizione del kit
	Action.HotkeyLabel = FText::FromString(TEXT("4")); // il tasto, DICHIARATO: non piu' dedotto (#2987)
	Action.TurnsRemaining = 2;      // in ricarica
	Action.TotalTurns = 3;
	Action.bUsableNow = false;

	Slot->SetAction(Action, /*bArmed=*/ false);
	const FString Spenta = Slot->GetActionLine().ToString();

	TestTrue(TEXT("la riga nomina il tasto che arma lo slot"), Spenta.Contains(TEXT("4. ")));
	TestTrue(TEXT("e il nome dell'azione"), Spenta.Contains(TEXT("Sovraccarico")));
	TestTrue(TEXT("il motivo d'indisponibilita' e' leggibile senza aprire un log"),
		Spenta.Contains(TEXT("(ricarica 2)")));
	TestFalse(TEXT("non armata: nessun prefisso di selezione"), Spenta.StartsWith(TEXT("> ")));

	// Armata: il canale NON cromatico e' il prefisso. E' la meta' che il colore da solo non puo' dare.
	Slot->SetAction(Action, /*bArmed=*/ true);
	const FString Armata = Slot->GetActionLine().ToString();

	TestTrue(TEXT("armata: il prefisso lo dichiara in testo"), Armata.StartsWith(TEXT("> ")));
	TestNotEqual(TEXT("armata e non armata NON sono la stessa riga"), Armata, Spenta);

	// Pronta: il motivo sparisce invece di dire «ricarica 0», che sarebbe un motivo inventato.
	FRTAbilityCooldownView Pronta = Action;
	Pronta.TurnsRemaining = 0;
	Pronta.bUsableNow = true;
	Slot->SetAction(Pronta, /*bArmed=*/ false);

	TestFalse(TEXT("un'azione pronta non porta un motivo"),
		Slot->GetActionLine().ToString().Contains(TEXT("ricarica")));

	DestroyHudWidgetWorld(nullptr); // no-op: questo test non ha un mondo
	return true;
}

/**
 * 🔑 **L'oracolo e' l'UGUAGLIANZA con `ARTHUD::ComposeAbilityLine`, e serve a impedire un secondo
 * produttore** — la stessa disciplina che `#2826` impone al proprio percorso di armamento: *«stesso
 * percorso, non un secondo»*.
 *
 * ⚠️ **Detto onestamente: oggi questo test e' tautologico**, perche' `GetActionLine` inoltra a quella
 * funzione e a null'altro. Non e' un oracolo di contenuto — quello e'
 * `ActionSlotLineCarriesKeyArmedAndReason` — ed e' un **rilevatore di cambiamento**: il giorno in cui
 * qualcuno sostituisse l'inoltro con una composizione locale, o il grafo di `WBP_RT_ActionSlot`
 * concatenasse numero e nome per conto proprio, le due stringhe divergerebbero al primo cambio di formato e
 * questo test cadrebbe. E' l'unica cosa che promette, e la promette su tre forme di vista diverse perche'
 * una sola non distinguerebbe un inoltro da una copia fortunata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudActionLineSameComposerTest,
	"RefactorTactics.ScreenHud.ActionSlotLineIsTheSameComposerAsTheHud",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudActionLineSameComposerTest::RunTest(const FString&)
{
	URTActionSlotWidget* Slot = NewObject<URTActionSlotWidget>();
	if (!TestNotNull(TEXT("widget"), Slot)) { return false; }

	FRTAbilityCooldownView InRicarica;
	InRicarica.ActionId = TEXT("Action.Overload");
	InRicarica.DisplayName = FText::FromString(TEXT("Sovraccarico"));
	InRicarica.AbilityIndex = 3;
	InRicarica.TurnsRemaining = 2;
	InRicarica.TotalTurns = 3;

	FRTAbilityCooldownView Pronta;
	Pronta.ActionId = TEXT("Action.Move");
	Pronta.DisplayName = FText::FromString(TEXT("Muovi"));
	Pronta.AbilityIndex = 0;
	Pronta.bUsableNow = true;

	// Terza forma: la vista NUDA, quella che un grafo o un test possono costruire ai default. Se un ramo
	// locale comparisse, e' la forma su cui divergerebbe per prima.
	const FRTAbilityCooldownView Nuda;

	for (const FRTAbilityCooldownView& Vista : { InRicarica, Pronta, Nuda })
	{
		for (const bool bArmed : { false, true })
		{
			Slot->SetAction(Vista, bArmed);
			TestEqual(
				FString::Printf(TEXT("la riga dello slot e' quella dell'HUD (%s, armata=%d)"),
					*Vista.ActionId.ToString(), bArmed ? 1 : 0),
				Slot->GetActionLine().ToString(),
				ARTHUD::ComposeAbilityLine(Vista, bArmed).Text);
		}
	}

	DestroyHudWidgetWorld(nullptr); // no-op: questo test non ha un mondo
	return true;
}

/**
 * 🔴 **Nessuna superficie dei widget e' una texture.** E' la voce di DoD «nessun widget referenzia una
 * texture direttamente» (D-031, `#220`) resa verificabile invece che raccomandata.
 *
 * Il controllo gira sulla REFLECTION, non sul testo del sorgente: una `UPROPERTY` o una `UFUNCTION`
 * aggiunta domani viene vista anche se nessuno rilegge questo file.
 *
 * ⚠️ **Il limite va detto, perche' e' meta' della verita'.** Copre la superficie C++. Un Blueprint derivato
 * puo' sempre aggiungersi una variabile `Texture2D`, e nessun gate lo impedisce: i `.uasset` non sono
 * versionati in questo repository. La guida lo scrive come regola per chi costruisce i `WBP_RT_*`; questo
 * test tiene la parte che il codice controlla.
 *
 * 🔴 **Tre buchi chiusi il 2026-08-26**, e tutti e tre erano del genere che passa inosservato perche' il
 * test era verde:
 *
 *  1. **Le firme delle `UFUNCTION` non erano guardate.** Era il buco piu' grave, perche' l'header dichiara
 *     proprio del tipo di ritorno che *«e' la regola: un `FName` non si puo' collegare a un `Image` senza
 *     passare dal catalogo»*. Un `UFUNCTION(BlueprintPure) UTexture2D* GetIcon()` passava.
 *  2. **I contenitori non erano guardati.** Un `TArray<TObjectPtr<UTexture2D>>` e' un `FArrayProperty`, non
 *     un `FObjectPropertyBase`, e il cast falliva in silenzio.
 *  3. **L'elenco delle classi era scritto a mano**, con un `TestEqual(..., 7)` che sembrava una controprova
 *     e non lo era: fissava la lunghezza della lista letterale, non la copertura. Un ottavo widget non
 *     faceva cadere niente — semplicemente non veniva guardato.
 *
 * Ora le classi si INTERROGANO. La derivazione non puo' essere «cio' che eredita da
 * `URTScreenHudWidgetBase`»: `URTActionSlotWidget` deriva direttamente da `UUserWidget`, e una regola cosi'
 * lo perderebbe proprio mentre sembra piu' rigorosa di una lista.
 */
namespace
{
	/** Vero se il tipo di questa proprieta' e' — o contiene — una `UTexture2D`.
	 *
	 *  Ricorsiva perche' un contenitore nasconde il tipo: la texture puo' stare dentro un `TArray`, la
	 *  chiave o il valore di una `TMap`, un `TSet`. `FObjectPropertyBase` copre gia' hard, soft e weak. */
	bool HudWidgetCarriesTexture(const FProperty* Prop)
	{
		if (!Prop)
		{
			return false;
		}
		if (const FObjectPropertyBase* AsObject = CastField<FObjectPropertyBase>(Prop))
		{
			return AsObject->PropertyClass
				&& AsObject->PropertyClass->IsChildOf(UTexture2D::StaticClass());
		}
		if (const FArrayProperty* AsArray = CastField<FArrayProperty>(Prop))
		{
			return HudWidgetCarriesTexture(AsArray->Inner);
		}
		if (const FSetProperty* AsSet = CastField<FSetProperty>(Prop))
		{
			return HudWidgetCarriesTexture(AsSet->ElementProp);
		}
		if (const FMapProperty* AsMap = CastField<FMapProperty>(Prop))
		{
			return HudWidgetCarriesTexture(AsMap->KeyProp) || HudWidgetCarriesTexture(AsMap->ValueProp);
		}
		// 🔴 **E dentro una STRUCT, che e' il ramo che mancava** (`#3178`). Il commento qui sopra
		// diceva *«un contenitore nasconde il tipo»* ed elencava `TArray`, `TMap`, `TSet`: una `USTRUCT`
		// lo nasconde allo stesso modo, e senza questo ramo il gate era **cieco** proprio sulla forma che il
		// codice usa. `URTActionSlotWidget::GetResolvedIcon` restituiva un `FRTIconResolution`, che porta una
		// `TSoftObjectPtr<UTexture2D>`: la texture usciva dall'API del widget e nessuno la vedeva.
		//
		// ⚠️ **Che fosse un buco e non una deroga lo prova la data**: il test era VERDE con quella
		// firma in `main`. Aggiungere questo ramo lo rende rosso senza toccare una riga di produzione, ed e'
		// il gate che il DoD di `#3178` chiede.
		if (const FStructProperty* AsStruct = CastField<FStructProperty>(Prop))
		{
			if (AsStruct->Struct == nullptr)
			{
				return false;
			}
			for (TFieldIterator<FProperty> It(AsStruct->Struct); It; ++It)
			{
				if (HudWidgetCarriesTexture(*It))
				{
					return true;
				}
			}
			return false;
		}
		return false;
	}

	/**
	 * Il nome della classe AUTOREVOLE che questa proprieta' porta, o vuoto (CP 14.6, `#166`).
	 *
	 * Stessa forma ricorsiva di `HudWidgetCarriesTexture`, e per la stessa ragione: un contenitore nasconde
	 * il tipo. Cio' che cambia e' cosa si cerca — non un asset, ma una **porta**: un widget che raggiunge il
	 * `TurnManager`, un `ARTUnit` o il view model della finestra ha il modo di ricalcolare e di rispondere,
	 * che e' esattamente cio' che §4.1 chiude e che la DoD di CP 14.6 chiede al widget della finestra.
	 */
	FString HudWidgetAuthorityCarried(const FProperty* Prop)
	{
		if (!Prop)
		{
			return FString();
		}
		if (const FObjectPropertyBase* AsObject = CastField<FObjectPropertyBase>(Prop))
		{
			const UClass* Carried = AsObject->PropertyClass;
			if (!Carried)
			{
				return FString();
			}
			if (Carried->IsChildOf(ARTTurnManager::StaticClass())
				|| Carried->IsChildOf(ARTUnit::StaticClass())
				|| Carried->IsChildOf(URTReactionWindowViewModel::StaticClass()))
			{
				return Carried->GetName();
			}
			return FString();
		}
		if (const FArrayProperty* AsArray = CastField<FArrayProperty>(Prop))
		{
			return HudWidgetAuthorityCarried(AsArray->Inner);
		}
		if (const FSetProperty* AsSet = CastField<FSetProperty>(Prop))
		{
			return HudWidgetAuthorityCarried(AsSet->ElementProp);
		}
		if (const FMapProperty* AsMap = CastField<FMapProperty>(Prop))
		{
			const FString FromKey = HudWidgetAuthorityCarried(AsMap->KeyProp);
			return FromKey.IsEmpty() ? HudWidgetAuthorityCarried(AsMap->ValueProp) : FromKey;
		}
		return FString();
	}

	/** I widget NOSTRI: nativi, dentro questo modulo. Un Blueprint caricato non entra — il suo caso e'
	 *  dichiarato fuori scope sopra, e includerlo renderebbe l'esito dipendente da cosa e' in memoria. */
	TArray<UClass*> ModuleHudWidgetClasses()
	{
		TArray<UClass*> Out;
		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* Class = *It;
			if (Class == UUserWidget::StaticClass()
				|| !Class->IsChildOf(UUserWidget::StaticClass())
				|| !Class->HasAnyClassFlags(CLASS_Native))
			{
				continue;
			}
			if (Class->GetOutermost() == URTScreenHudWidgetBase::StaticClass()->GetOutermost())
			{
				Out.Add(Class);
			}
		}
		Out.Sort([](const UClass& A, const UClass& B) { return A.GetName() < B.GetName(); });
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudNoTextureTest,
	"RefactorTactics.ScreenHud.WidgetApiExposesNoTexture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudNoTextureTest::RunTest(const FString&)
{
	const TArray<UClass*> WidgetClasses = ModuleHudWidgetClasses();

	// Controprova della PREMESSA, e stavolta verifica la copertura invece della lunghezza di una lista:
	// i widget dell'HUD devono essere fra quelli trovati. Se l'enumerazione smettesse di funzionare, il
	// test cadrebbe qui invece di passare senza guardare niente.
	//
	// ⚠️ **Due voci mancavano e sono state aggiunte il 2026-09-09** (`#166`): `URTFastDecisionWidget`, che
	// nasce con questo checkpoint, e `URTPlayerEventLogWidget`, che era entrato con `#2697` senza passare di
	// qui. La lista e' scritta a mano e non si aggiorna da sola: chi aggiunge un widget alla famiglia
	// aggiunge una riga anche qui, o la «copertura» resta vera di una famiglia piu' piccola di quella reale.
	const TArray<UClass*> MustBeCovered = {
		URTScreenHudWidgetBase::StaticClass(),
		URTTurnHeaderWidget::StaticClass(),
		URTPlayerEventLogWidget::StaticClass(),
		URTTeamRosterWidget::StaticClass(),
		URTSelectedUnitPanelWidget::StaticClass(),
		URTActionDockWidget::StaticClass(),
		URTActionSlotWidget::StaticClass(),
		URTTacticalHUDWidget::StaticClass(),
		URTFastDecisionWidget::StaticClass(),
		URTUnitCardWidget::StaticClass(),          // #3636
		URTPlanWarningEntryWidget::StaticClass(),  // #3636
	};
	for (UClass* Class : MustBeCovered)
	{
		TestTrue(*FString::Printf(TEXT("l'enumerazione copre %s"), *GetNameSafe(Class)),
			WidgetClasses.Contains(Class));
	}

	int32 Inspected = 0;
	for (const UClass* Class : WidgetClasses)
	{
		// Solo cio' che la classe DICHIARA: `UUserWidget` porta proprieta' sue (brush di stile, cursori)
		// che non sono nostre e non sono il difetto che il DoD vieta.
		for (TFieldIterator<FProperty> It(Class, EFieldIterationFlags::None); It; ++It)
		{
			++Inspected;
			if (HudWidgetCarriesTexture(*It))
			{
				AddError(FString::Printf(
					TEXT("%s::%s e' — o contiene — una texture: le icone viaggiano per CHIAVE ")
					TEXT("(`UI.Icon.*`) e si risolvono dal catalogo (D-031). Un widget che riceve un ")
					TEXT("asset rende la regola una raccomandazione."),
					*Class->GetName(), *It->GetName()));
			}
		}

		// E le firme. Il tipo di ritorno E' la regola (§4.1): un `FName` non si puo' collegare a un
		// `Image` senza passare dal catalogo, un `UTexture2D*` si'.
		for (TFieldIterator<UFunction> Fn(Class, EFieldIterationFlags::None); Fn; ++Fn)
		{
			for (TFieldIterator<FProperty> Param(*Fn, EFieldIterationFlags::None); Param; ++Param)
			{
				++Inspected;
				if (HudWidgetCarriesTexture(*Param))
				{
					const bool bIsReturn = Param->HasAnyPropertyFlags(CPF_ReturnParm);
					AddError(FString::Printf(
						TEXT("%s::%s espone una texture (%s `%s`): il tipo di ritorno e' la regola — la ")
						TEXT("chiave si risolve dal catalogo, l'asset no."),
						*Class->GetName(), *Fn->GetName(),
						bIsReturn ? TEXT("valore di ritorno") : TEXT("parametro"), *Param->GetName()));
				}
			}
		}
	}

	// Senza questa riga il test sarebbe verde anche se l'iterazione non vedesse nulla.
	TestTrue(TEXT("l'iterazione ha davvero guardato delle superfici"), Inspected > 0);

	return true;
}

/**
 * 🔴 **«NESSUNA LOGICA DI GIOCO NEL WIDGET» SMETTE DI ESSERE UNA PROMESSA** (CP 14.6, `#166`, voce 2).
 *
 * Fino a qui quella riga di DoD si verificava **leggendo il diff**: nessun test poteva dire se un widget
 * avesse acquistato una porta sul core. Questo test la rende falsificabile, e in due modi che vanno tenuti
 * distinti perche' chiudono difetti diversi.
 *
 * **(1) Nessuna porta, su NESSUN widget della famiglia.** `URTReactionWindowViewModel::SubmitResponse` e'
 * `BlueprintCallable`: un accessore **pubblico** sulla base — la forma che questo checkpoint stava per
 * prendere — metterebbe un nodo che **spara un Overwatch** nel grafo di tutte e sei le classi derivate,
 * roster ed event log compresi. Percio' si guarda l'intera famiglia e non il solo widget nuovo:
 * `GetReactionWindow()` e' `protected` e non riflessa, e questa riga e' cio' che lo mantiene vero.
 *
 * **(2) La risposta non si NOMINA.** `ChooseOption` prende un `int32`. Se prendesse una `FString`, il
 * widget diventerebbe il nono produttore del letterale `FIRE`/`HOLD` — otto siti dello scenario harness lo
 * confrontano `CaseSensitive` — e il primo fuori dai test del core. E un `FRTReactionWindowOptionView` come
 * parametro non basterebbe: un Blueprint ne costruisce uno ai default, con `Response` **vuota**, che il
 * core legge come **scadenza**.
 *
 * ⚠️ **Il limite e' lo stesso di `WidgetApiExposesNoTexture` e va ripetuto**: si vede la superficie C++, non
 * il grafo di un `.uasset`. Un `WBP_RT_*` puo' sempre aggiungersi una variabile — nessun gate lo impedisce,
 * perche' i Blueprint non sono versionati qui. Cio' che questo test garantisce e' che il C++ non gliela
 * **offra**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudFastDecisionNoAuthorityTest,
	"RefactorTactics.ScreenHud.FastDecisionApiCarriesNoAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudFastDecisionNoAuthorityTest::RunTest(const FString&)
{
	// --- 1. NESSUNA PORTA, su tutta la famiglia -----------------------------------------------------
	const TArray<UClass*> WidgetClasses = ModuleHudWidgetClasses();
	if (!TestTrue(TEXT("premessa: l'enumerazione trova il widget della finestra"),
			WidgetClasses.Contains(URTFastDecisionWidget::StaticClass())))
	{
		return false;
	}

	// 🔴 **Si guarda cio' che i Blueprint VEDONO, non cio' che la riflessione elenca — e la distinzione e'
	// stata misurata, non prevista.** La prima stesura iterava tutte le `UPROPERTY` e segnalava tre campi
	// PRIVATI della base: `TurnManager`, `SelectedUnitForTest` e `ReactionWindow`. Sono `UPROPERTY(Transient)`
	// **senza** flag Blueprint, e la `UPROPERTY` li' serve al **GC** — un weak pointer al view model raccolto
	// spegnerebbe il ramo interattivo a partita in corso. Nessun grafo li raggiunge.
	//
	// ⚠️ **Il criterio sbagliato non era «troppo severo»: diceva una cosa falsa.** Il messaggio d'errore
	// affermava *«espone ai Blueprint»* di campi che ai Blueprint non arrivano — e chi lo avesse letto
	// avrebbe tolto la `UPROPERTY` giusta per il motivo sbagliato.
	//
	// ⚠️ Vale anche per le funzioni: solo quelle raggiungibili da un grafo (`BlueprintCallable`,
	// `BlueprintPure`, `BlueprintImplementableEvent`) sono una superficie. Una `UFUNCTION()` nuda esiste per
	// il delegate binding, non per l'autore di un `.uasset`.
	static constexpr uint64 BlueprintReachable =
		FUNC_BlueprintCallable | FUNC_BlueprintEvent;

	int32 Inspected = 0;
	for (const UClass* Class : WidgetClasses)
	{
		for (TFieldIterator<FProperty> It(Class, EFieldIterationFlags::None); It; ++It)
		{
			if (!It->HasAnyPropertyFlags(CPF_BlueprintVisible))
			{
				continue;
			}
			++Inspected;
			const FString Carried = HudWidgetAuthorityCarried(*It);
			if (!Carried.IsEmpty())
			{
				AddError(FString::Printf(
					TEXT("%s::%s espone `%s` ai Blueprint: e' una PORTA sul core. §4.1 chiude questa ")
					TEXT("strada — se non c'e' il puntatore, non c'e' il modo di ricalcolare — e per la ")
					TEXT("finestra di reazione significherebbe un nodo che risponde `FIRE` nel grafo di ")
					TEXT("un widget che non deve poterlo fare."),
					*Class->GetName(), *It->GetName(), *Carried));
			}
		}

		for (TFieldIterator<UFunction> Fn(Class, EFieldIterationFlags::None); Fn; ++Fn)
		{
			if ((Fn->FunctionFlags & BlueprintReachable) == 0)
			{
				continue;
			}
			for (TFieldIterator<FProperty> Param(*Fn, EFieldIterationFlags::None); Param; ++Param)
			{
				++Inspected;
				const FString Carried = HudWidgetAuthorityCarried(*Param);
				if (!Carried.IsEmpty())
				{
					AddError(FString::Printf(
						TEXT("%s::%s espone `%s` nella propria firma: la presentazione riceve VISTE, ")
						TEXT("non attori."),
						*Class->GetName(), *Fn->GetName(), *Carried));
				}
			}
		}
	}
	TestTrue(TEXT("l'iterazione ha davvero guardato delle superfici"), Inspected > 0);

	// ⛔ **Controprova della PREMESSA, e senza di essa il punto 1 sarebbe verde a vuoto.** I tre campi che
	// hanno fatto cadere la prima stesura devono esistere e **non** essere Blueprint-visibili: e' il fatto
	// su cui poggia tutto il filtro qui sopra, e un giorno qualcuno potrebbe marcarne uno `BlueprintReadOnly`
	// «per comodita' di debug».
	for (const TCHAR* Nome : { TEXT("TurnManager"), TEXT("SelectedUnitForTest"), TEXT("ReactionWindow") })
	{
		const FProperty* Prop =
			URTScreenHudWidgetBase::StaticClass()->FindPropertyByName(FName(Nome));
		if (TestNotNull(*FString::Printf(TEXT("la base dichiara `%s`"), Nome), Prop))
		{
			TestFalse(
				*FString::Printf(TEXT("`%s` NON e' visibile ai Blueprint: e' riflessa per il GC"), Nome),
				Prop->HasAnyPropertyFlags(CPF_BlueprintVisible));
		}
	}

	// --- 2. LA RISPOSTA NON SI NOMINA: `ChooseOption` prende un indice ------------------------------
	const UFunction* Choose =
		URTFastDecisionWidget::StaticClass()->FindFunctionByName(TEXT("ChooseOption"));
	if (!TestNotNull(TEXT("`ChooseOption` e' esposta ai Blueprint"), Choose))
	{
		return false;
	}

	int32 Parametri = 0;
	for (TFieldIterator<FProperty> Param(Choose, EFieldIterationFlags::None); Param; ++Param)
	{
		if (Param->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			continue;
		}
		++Parametri;
		TestTrue(
			*FString::Printf(TEXT("`ChooseOption::%s` e' un intero, non una risposta da comporre"),
				*Param->GetName()),
			CastField<FIntProperty>(*Param) != nullptr);
	}
	TestEqual(TEXT("`ChooseOption` prende un parametro solo: l'indice"), Parametri, 1);

	// --- 3. E nessuna funzione del widget accetta una stringa --------------------------------------
	// E' la meta' che chiude la porta di lato: un secondo mutatore che prendesse una `FString` renderebbe
	// il punto 2 vero e la regola falsa.
	for (TFieldIterator<UFunction> Fn(URTFastDecisionWidget::StaticClass(), EFieldIterationFlags::None);
		 Fn; ++Fn)
	{
		if ((Fn->FunctionFlags & BlueprintReachable) == 0)
		{
			continue;
		}
		for (TFieldIterator<FProperty> Param(*Fn, EFieldIterationFlags::None); Param; ++Param)
		{
			if (Param->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				continue;
			}
			if (CastField<FStrProperty>(*Param) || CastField<FNameProperty>(*Param))
			{
				AddError(FString::Printf(
					TEXT("`URTFastDecisionWidget::%s` accetta `%s` come testo: la risposta si INDICA per ")
					TEXT("indice, non si nomina. `FIRE:<indice>` e' un formato con un solo produttore ")
					TEXT("(`URTReactionOpportunityLibrary::FireResponse`)."),
					*Fn->GetName(), *Param->GetName()));
			}
		}
	}

	return true;
}

/**
 * ⛔ **SENZA VIEW MODEL IL WIDGET NON MOSTRA NIENTE, e non esplode** (CP 14.6, `#166`).
 *
 * 🔴 **Non e' un caso limite: e' il percorso normale per qualche frame.** `ARTGameMode::BeginPlay` presenta
 * l'HUD prima di spawnare il `TurnManager`, e `URTFrontendNavigator::PresentMatchHud` crea i widget con
 * `CreateWidget(GameInstance, ...)` — quindi puo' non esserci ancora un proprietario da cui risolvere la
 * finestra. Un widget che in quel momento chiamasse `GetWindow()` su un puntatore nullo chiuderebbe la
 * partita al primo frame.
 *
 * ⚠️ **Il residuo e' `-1`, non `0`, e la differenza e' semantica**: zero direbbe «scaduta adesso», che e' lo
 * stato in cui il countdown NON deve piu' accettare input. E' la convenzione di
 * `FRTMatchHeaderView::PlanningSecondsRemaining`, tenuta uguale di proposito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudFastDecisionEmptyTest,
	"RefactorTactics.ScreenHud.FastDecisionWithoutAWindowShowsNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudFastDecisionEmptyTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTFastDecisionWidget* Widget = NewObject<URTFastDecisionWidget>(World);
	if (!TestNotNull(TEXT("widget"), Widget)) { DestroyHudWidgetWorld(World); return false; }

	TestFalse(TEXT("nessuna finestra da disegnare"), Widget->IsWindowOpen());

	const FRTReactionWindowView Vista = Widget->GetWindow();
	TestFalse(TEXT("la vista e' ai default"), Vista.bOpen);
	TestEqual(TEXT("e non offre nessuna opzione da premere"), Vista.Options.Num(), 0);
	TestTrue(TEXT("il residuo dice «nessuna finestra», non «scaduta adesso»"),
		Widget->GetRemainingSeconds() < 0.f);

	// Un click senza finestra non deve fare nulla ne' esplodere: e' il ramo che un `.uasset` prende ogni
	// volta che un bottone sopravvive di un frame alla propria finestra. ⛔ Nessuna warning attesa qui: si
	// esce PRIMA di valutare l'indice, perche' senza view model non c'e' un elenco su cui giudicarlo.
	Widget->ChooseOption(0);
	TestFalse(TEXT("e dopo il click continua a non esserci nessuna finestra"), Widget->IsWindowOpen());

	DestroyHudWidgetWorld(World);
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

/**
 * Lo slot risolve la propria icona dal catalogo che ha RICEVUTO.
 *
 * 🔴 **La coppia e' il test, non la singola asserzione.** Senza catalogo `ResolveIcon` restituisce
 * `bResolved = false`; con il catalogo giusto `true`. Asserire solo il secondo caso lascerebbe passare
 * un'implementazione che risponde sempre «risolta» — e a schermo si vedrebbe il missing-icon con la
 * barra verde, cioe' il difetto piu' difficile da attribuire.
 *
 * ⚠️ Il test NON verifica che l'icona si veda: `Asset` e' una soft reference, e questa suite non carica
 * texture. La leggibilita' a schermo e' `PIE-ICON-01`, e nessun test la sostituisce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionSlotResolvesFromCatalogTest,
	"RefactorTactics.ScreenHud.ActionSlotResolvesFromCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionSlotResolvesFromCatalogTest::RunTest(const FString&)
{
	URTActionSlotWidget* Slot = NewObject<URTActionSlotWidget>();
	if (!TestNotNull(TEXT("slot"), Slot)) { return false; }

	FRTAbilityCooldownView Azione;
	Azione.ActionId = TEXT("Action.Move");
	const FName ChiaveAttesa = URTIconLibrary::MakeIconId(Azione.ActionId);

	// (1) Senza catalogo: il missing-icon, e `bResolved` lo DICHIARA. E' lo stato in cui lo slot vive
	// finche' qualcuno non gli passa il catalogo, ed e' voluto — non un errore da nascondere.
	Slot->SetAction(Azione, /*bArmed=*/ false);
	TestFalse(TEXT("senza catalogo l'icona non e' risolta"), Slot->GetResolvedIcon().bResolved);

	// (2) Con un catalogo che contiene la chiave: risolta.
	URTIconCatalogData* Catalogo = NewObject<URTIconCatalogData>();
	if (!TestNotNull(TEXT("catalogo"), Catalogo)) { return false; }
	Catalogo->Icons.Add(FRTIconDef(ChiaveAttesa, ERTIconCategory::Action,
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Prova/T_Prova.T_Prova")))));

	Slot->SetAction(Azione, /*bArmed=*/ false, Catalogo);
	const FRTIconResolution Risolta = Slot->GetResolvedIcon();
	TestTrue(TEXT("col catalogo l'icona e' risolta"), Risolta.bResolved);
	TestEqual(TEXT("ed e' l'asset dichiarato per quella chiave"),
		Risolta.Asset.ToString(), FString(TEXT("/Game/Prova/T_Prova.T_Prova")));

	// (3) La chiave che lo slot chiede e' quella dell'AZIONE, non una costante: cambiando azione cambia
	// chiave, e una chiave che il catalogo non ha torna non risolta. Senza questo caso, un'implementazione
	// che ignora `Action` e risolve sempre la stessa icona passerebbe i due sopra.
	FRTAbilityCooldownView Altra;
	Altra.ActionId = TEXT("Action.Guard");
	Slot->SetAction(Altra, /*bArmed=*/ false, Catalogo);
	TestFalse(TEXT("un'altra azione chiede un'altra chiave, che il catalogo non ha"),
		Slot->GetResolvedIcon().bResolved);

	return true;
}

/**
 * 🔴 **LO SLOT CARICA LA TEXTURE, INVECE DI SPERARE CHE QUALCUN ALTRO L'ABBIA CARICATA** (`#3178`).
 *
 * Il difetto che questo test rende falsificabile e' costato una seduta a schermo per essere visto, e
 * nessun gate poteva vederlo: `WBP_RT_ActionSlot` componeva
 * `SetBrushFromTexture(IconImage, ResolveSoftReference(Break(GetResolvedIcon)))`, e **`Resolve Soft
 * Reference` non carica** — *«If the object isn't already loaded in memory this will return none»*
 * (`K2Node_ConvertAsset.cpp`). A schermo restava un rettangolo bianco, e la diagnostica taceva perche' la
 * **chiave** si era risolta benissimo: `ResolveIcon` non aveva niente da segnalare.
 *
 * ⚠️ **La premessa e' meta' del test, e senza di lei sarebbe vacuo.** Se la texture fosse gia' in
 * memoria, anche un `Get()` la renderebbe e questo test passerebbe **sul codice difettoso**. Per questo la
 * prima asserzione e' che l'asset NON sia caricato: e' il controllo che rende la seconda capace di
 * fallire. Se un giorno qualcun altro caricasse quell'icona prima, la premessa cadrebbe e il test lo
 * direbbe, invece di diventare verde in silenzio.
 *
 * ⛔ **Non verifica il grafo del `.uasset`**, che nessun test raggiunge: verifica che il C++ offra la via
 * giusta. Che il Blueprint la usi lo dice
 * `RefactorTactics.Editor.ActionSlotAppliesTheIconThroughTheCppPort`, che cammina il grafo del `.uasset`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudSlotLoadsIconTest,
	"RefactorTactics.ScreenHud.ActionSlotLoadsTheIconItShows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudSlotLoadsIconTest::RunTest(const FString&)
{
	URTActionSlotWidget* Slot = NewObject<URTActionSlotWidget>();
	if (!TestNotNull(TEXT("slot"), Slot)) { return false; }

	// Un'icona VERA del catalogo spedito: una inventata non si caricherebbe mai, e il test non
	// distinguerebbe «non carica» da «non esiste».
	const TCHAR* const PathIcona = TEXT("/Game/RT/UI/Icons/RT_UI_Icon_Action_Move.RT_UI_Icon_Action_Move");

	FRTAbilityCooldownView Azione;
	Azione.ActionId = TEXT("Action.Move");

	URTIconCatalogData* Catalogo = NewObject<URTIconCatalogData>();
	if (!TestNotNull(TEXT("catalogo"), Catalogo)) { return false; }
	Catalogo->Icons.Add(FRTIconDef(URTIconLibrary::MakeIconId(Azione.ActionId), ERTIconCategory::Action,
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(PathIcona))));

	Slot->SetAction(Azione, /*bArmed=*/ false, Catalogo);

	const FRTIconResolution Risolta = Slot->GetResolvedIcon();
	if (!TestTrue(TEXT("premessa: la chiave si risolve"), Risolta.bResolved)) { return false; }

	// 🔴 **LA PREMESSA CHE RENDE IL TEST NON VACUO** (vedi il docstring): con l'asset gia' in
	// memoria, il codice difettoso passerebbe.
	if (!TestNull(TEXT("premessa: l'icona NON e' gia' in memoria"), Risolta.Asset.Get()))
	{
		AddWarning(TEXT("qualcuno ha caricato questa icona prima del test: l'asserzione sotto non ")
				   TEXT("distingue piu' LoadSynchronous da Get, e va scelta un'altra icona."));
		return false;
	}

	UImage* Immagine = NewObject<UImage>();
	if (!TestNotNull(TEXT("immagine"), Immagine)) { return false; }
	TestNull(TEXT("premessa: il brush nasce senza risorsa"), Immagine->GetBrush().GetResourceObject());

	Slot->ApplyResolvedIconTo(Immagine);

	// Il difetto in una riga: prima di `#3178` qui c'era `nullptr`, e a schermo un rettangolo bianco.
	TestNotNull(TEXT("dopo ApplyResolvedIconTo il brush porta una texture"),
		Immagine->GetBrush().GetResourceObject());
	TestEqual(TEXT("ed e' proprio l'icona che il catalogo dichiara per quella chiave"),
		GetNameSafe(Immagine->GetBrush().GetResourceObject()),
		FString(TEXT("RT_UI_Icon_Action_Move")));

	// ⚠️ **Un `Target` nullo non deve rompere niente**: il grafo puo' chiamarla prima che l'albero
	// sia costruito, e un crash li' sarebbe peggio del difetto che questo test chiude.
	Slot->ApplyResolvedIconTo(nullptr);

	return true;
}

/**
 * IL WIDGET DEL FEED LEGGE IL CANALE FILTRATO, E NON NE HA UN ALTRO — `#2697`.
 *
 * 🔴 **E' l'assertion che il difetto originale non aveva.** `GetRecentEventsForTeam` era corretto, testato
 * e verde — e senza chiamanti: un canale che nessuno legge non produce nessun rosso, ed e' la forma di
 * `#2549` e `#2492`. Questo test lega il **consumatore** al filtro: cancellare la chiamata nel widget lo
 * rende rosso, che e' l'unica cosa che il verde precedente non poteva fare.
 *
 * ⛔ **E copre la regressione che sarebbe un LEAK, non un difetto di UI.** Un widget che leggesse il canale
 * non filtrato mostrerebbe fatti che l'osservatore non ha diritto di conoscere. La difesa e' strutturale —
 * `URTScreenHudWidgetBase` non espone il `TurnManager` ai Blueprint, quindi non c'e' una seconda porta da
 * cui passare — ma la struttura si puo' cambiare, e l'assertion no.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudEventLogReadsFilteredFeedTest,
	"RefactorTactics.ScreenHud.EventLogWidgetReadsTheFilteredFeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudEventLogReadsFilteredFeedTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTPlayerEventLogWidget* Feed = NewObject<URTPlayerEventLogWidget>(World);
	if (!TestNotNull(TEXT("widget"), Feed)) { DestroyHudWidgetWorld(World); return false; }

	// 1. Senza contesto: nessuna riga, e nessun crash. Un widget costruito prima del manager e' il percorso
	//    normale, non un caso limite — `NativeConstruct` gira prima che l'orchestratore esista.
	TestEqual(TEXT("senza contesto il feed e' vuoto"), Feed->GetFeed().Num(), 0);

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHudWidgetWorld(World); return false; }

	// Il tiro rifiutato di un'unita' della squadra 1, con un muro che la SUA squadra conosce.
	FRTTurnLogEntry Rifiutato;
	Rifiutato.Category = ERTLogCategory::Combat;
	Rifiutato.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
	Rifiutato.UnitId = 9;
	Rifiutato.SrcCell = FRTCellId(-1, 0, 0);
	Rifiutato.TgtCell = FRTCellId(1, 0, 0);
	Rifiutato.SightBlockerCell = FRTCellId(0, 0, 0);
	Rifiutato.Verdict.AllowTeam(1);
	TM->AppendTurnLogEntryForTest(Rifiutato);

	// 2. L'osservatore AUTORIZZATO legge la riga, e il riferimento nel mondo per trovarla.
	Feed->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 1);
	const TArray<FRTPlayerEventLineView> Autorizzato = Feed->GetFeed();
	if (!TestEqual(TEXT("chi e' autorizzato legge la riga dal widget"), Autorizzato.Num(), 1))
	{
		DestroyHudWidgetWorld(World);
		return false;
	}
	TestFalse(TEXT("e la riga non e' vuota"), Autorizzato[0].Text.IsEmpty());
	TestTrue(TEXT("e porta l'ostacolo da marcare"), Autorizzato[0].bHasBlocker);
	TestTrue(TEXT("nella cella che la voce portava"), Autorizzato[0].BlockerCell == FRTCellId(0, 0, 0));

	// 3. L'altro osservatore, STESSO widget e stesso log: niente. E' la meta' che rende il test una prova
	//    del filtro invece che una prova dell'esistenza.
	Feed->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);
	TestEqual(TEXT("chi non e' autorizzato non legge nulla"), Feed->GetFeed().Num(), 0);

	DestroyHudWidgetWorld(World);
	return true;
}

namespace
{
	/** Un'unita' in campo, con l'identita' minima che il roster legge. */
	ARTUnit* SpawnRosterUnit(UWorld* World, int32 TeamId, const TCHAR* HeroId)
	{
		ARTUnit* Unit = World->SpawnActor<ARTUnit>();
		if (Unit)
		{
			Unit->TeamId = TeamId;
			Unit->HeroId = FName(HeroId);
			Unit->MaxHealth = 10;
			Unit->Health = 10;
		}
		return Unit;
	}

	/**
	 * Gli `HeroId` di una lista di carte, come stringa unica.
	 *
	 * ⚠️ **Si confronta CHI c'e', non quanti**: un roster rotto che restituisse la squadra 0 due volte
	 * conterebbe anche lui quattro carte. La forma a stringa esiste perche' `TestEqual` non confronta
	 * `TArray`, e perche' il messaggio di fallimento dice subito **quale** elenco e' arrivato.
	 */
	FString HeroIdsOf(const TArray<FRTUnitCardView>& Cards)
	{
		TArray<FString> Ids;
		Ids.Reserve(Cards.Num());
		for (const FRTUnitCardView& Card : Cards) { Ids.Add(Card.HeroId.ToString()); }
		return FString::Join(Ids, TEXT(","));
	}
}

/**
 * La GEMELLA non presidiata di `RosterShowsOnlyOwnTeamAndKeepsTheFallen` (`RTHudScenarioTests.cpp`), e il
 * nome le accoppia di proposito.
 *
 * 🔴 **Senza questa, quel test resta verde e il suo NOME diventa una mezza verita'.** Afferma una regola
 * incondizionata — *«solo la propria squadra»* — che `#2744` rende condizionata alla sessione presidiata; e
 * il suo scenario non arma mai una sessione non presidiata, quindi nessun rosso avviserebbe. Il verde di la'
 * e' il controllo positivo di qui: senza, un roster che mostrasse **sempre** tutto passerebbe.
 *
 * ⚠️ **Si guarda CHI c'e', non quanti.** Un roster rotto che restituisse la squadra 0 due volte conterebbe
 * anche lui quattro carte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudRosterUnattendedTest,
	"RefactorTactics.ScreenHud.RosterSplitsTheTeamsOnlyInAnUnattendedSession",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudRosterUnattendedTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ DestroyHudWidgetWorld(World); };

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("turn manager"), TM)) { return false; }

	SpawnRosterUnit(World, /*TeamId=*/ 0, TEXT("Alfa"));
	SpawnRosterUnit(World, /*TeamId=*/ 0, TEXT("Bravo"));
	SpawnRosterUnit(World, /*TeamId=*/ 1, TEXT("Charlie"));
	SpawnRosterUnit(World, /*TeamId=*/ 1, TEXT("Delta"));

	URTTeamRosterWidget* Roster = NewObject<URTTeamRosterWidget>(World);
	if (!TestNotNull(TEXT("widget"), Roster)) { return false; }
	Roster->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);

	// ── 1. CONTROLLO POSITIVO: sessione presidiata. Il comportamento di oggi, bit per bit.
	if (!TestFalse(TEXT("premessa: la sessione e' presidiata"), TM->IsUnattendedSession())) { return false; }

	TestEqual(TEXT("presidiata: il roster e' la propria squadra"),
		HeroIdsOf(Roster->GetRoster()), FString(TEXT("Alfa,Bravo")));

	// 🔑 **La riga che vale il test.** Se questa lista non fosse vuota, la lista avversaria esisterebbe
	// anche per chi sta giocando — cioe' il difetto sarebbe stato spostato, non chiuso.
	TestEqual(TEXT("presidiata: non c'e' NESSUNA lista avversaria"), Roster->GetOpposingRoster().Num(), 0);

	// ── 2. Sessione non presidiata: chi guarda non gioca in nessuna delle due.
	TM->SetUnattendedSession(true);

	const FString Propria = HeroIdsOf(Roster->GetRoster());
	const FString Altra = HeroIdsOf(Roster->GetOpposingRoster());

	TestEqual(TEXT("non presidiata: la propria lista non cambia"), Propria, FString(TEXT("Alfa,Bravo")));
	TestEqual(TEXT("non presidiata: l'altra squadra ha la sua lista"), Altra, FString(TEXT("Charlie,Delta")));

	// ── 3. Le due liste sono DISGIUNTE: e' la proprieta' per cui due chiamate a `BuildTeamRoster` possono
	//      comporre senza doppioni, e senza di essa il disegno sarebbe sbagliato invece che solo il codice.
	for (const FRTUnitCardView& Card : Roster->GetOpposingRoster())
	{
		TestFalse(FString::Printf(TEXT("'%s' non compare in entrambe le liste"), *Card.HeroId.ToString()),
			Propria.Contains(Card.HeroId.ToString()));
	}

	return true;
}

/**
 * In autobattle il feed si allarga a entrambe le squadre — e una voce autorizzata a TUTTE E DUE resta UNA.
 *
 * 🔴 **E' il doppione che due proiezioni concatenate produrrebbero**, ed e' il motivo per cui il feed non
 * ha la stessa forma del roster: `BuildTeamRoster` filtra per `TeamId ==` e da' insiemi disgiunti, mentre
 * `AllowsTeam` autorizza per voce e gli insiemi si sovrappongono. La stessa trappola che `ARTHUD::DrawHUD`
 * documenta per gli intenti.
 *
 * ⚠️ Senza la voce pubblica il test sarebbe vacuo: due voci private, una per squadra, passerebbero anche
 * con l'implementazione sbagliata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudFeedUnattendedTest,
	"RefactorTactics.ScreenHud.EventFeedWidensToBothTeamsWithoutDuplicatingAnEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudFeedUnattendedTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ DestroyHudWidgetWorld(World); };

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("turn manager"), TM)) { return false; }

	SpawnRosterUnit(World, /*TeamId=*/ 0, TEXT("Alfa"));
	SpawnRosterUnit(World, /*TeamId=*/ 1, TEXT("Charlie"));

	// Un tiro rifiutato che solo la squadra 1 puo' conoscere.
	FRTTurnLogEntry SoloUno;
	SoloUno.Category = ERTLogCategory::Combat;
	SoloUno.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
	SoloUno.UnitId = 9;
	SoloUno.SrcCell = FRTCellId(-1, 0, 0);
	SoloUno.TgtCell = FRTCellId(1, 0, 0);
	SoloUno.Verdict.AllowTeam(1);
	TM->AppendTurnLogEntryForTest(SoloUno);

	// Un fatto PUBBLICO: autorizzato a entrambe. E' la voce su cui il doppione si manifesterebbe.
	FRTTurnLogEntry Pubblico;
	Pubblico.Category = ERTLogCategory::Combat;
	Pubblico.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
	Pubblico.UnitId = 7;
	Pubblico.SrcCell = FRTCellId(-2, 0, 0);
	Pubblico.TgtCell = FRTCellId(2, 0, 0);
	Pubblico.Verdict.AllowTeam(0);
	Pubblico.Verdict.AllowTeam(1);
	TM->AppendTurnLogEntryForTest(Pubblico);

	URTPlayerEventLogWidget* Feed = NewObject<URTPlayerEventLogWidget>(World);
	if (!TestNotNull(TEXT("widget"), Feed)) { return false; }
	Feed->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);

	// ── 1. CONTROLLO POSITIVO: presidiata. La squadra 0 vede solo il fatto pubblico.
	if (!TestFalse(TEXT("premessa: la sessione e' presidiata"), TM->IsUnattendedSession())) { return false; }
	TestEqual(TEXT("presidiata: la squadra 0 legge solo il fatto pubblico"), Feed->GetFeed().Num(), 1);

	// ── 2. Non presidiata: si aggiunge la voce dell'altra squadra, e il pubblico NON si sdoppia.
	TM->SetUnattendedSession(true);
	TestEqual(TEXT("non presidiata: due righe, non tre"), Feed->GetFeed().Num(), 2);

	return true;
}

/**
 * L'insieme vuoto e' «nessuno guarda», mai «guardano tutti».
 *
 * 🔴 **Il fail-closed di `AllowsTeam` poteva perdersi passando per un contenitore.** Un
 * `ContainsByPredicate` invertito, o un `if (Ids.IsEmpty()) return true` scritto per «comodita'», darebbe
 * un proiettore che pubblica tutto — e nessun altro test lo direbbe, perche' tutti gli altri passano un
 * insieme non vuoto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudEmptyObserverSetTest,
	"RefactorTactics.ScreenHud.AnEmptyObserverSetAuthorizesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudEmptyObserverSetTest::RunTest(const FString&)
{
	FRTTurnLogEntry Pubblico;
	Pubblico.Category = ERTLogCategory::Combat;
	Pubblico.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
	Pubblico.UnitId = 7;
	Pubblico.SrcCell = FRTCellId(-2, 0, 0);
	Pubblico.TgtCell = FRTCellId(2, 0, 0);
	Pubblico.Verdict.AllowTeam(0);
	Pubblico.Verdict.AllowTeam(1);

	const TArray<FRTTurnLogEntry> Log{ Pubblico };

	// Controllo positivo: con un osservatore autorizzato la riga c'e'. Senza, «zero righe» sarebbe vero
	// anche per un log vuoto o per una voce che nessuno classifica.
	TestEqual(TEXT("con un osservatore autorizzato la riga esiste"),
		URTHudViewModel::BuildPlayerEventFeed(Log, TArray<int32>{ 0 }).Num(), 1);

	TestEqual(TEXT("insieme VUOTO: nessuna riga"),
		URTHudViewModel::BuildPlayerEventFeed(Log, TArray<int32>{}).Num(), 0);

	TestFalse(TEXT("e il predicato dice di no anche da solo"),
		URTPlayerEventProjector::IsAuthorized(Pubblico, TArray<int32>{}));

	return true;
}

/**
 * `#2744` — SENZA MANAGER chi guarda e' trattato come in una sessione PRESIDIATA: la vista piu' stretta.
 *
 * 🔴 **Il ramo era dichiarato e non presidiato.** `ResolveObserverTeamIds` lo scrive nel proprio commento — un
 * widget nato prima del manager mostrerebbe altrimenti entrambe le squadre per qualche frame, «un leak temporale
 * invece di un difetto di layout» — ma la misura di privacy del 2026-10-12 l'ha trovato scoperto: invertendo il
 * ramo (senza manager -> la vista piu' LARGA) la suite restava verde. Nessun altro test fa attraversare a
 * `ResolveObserverTeamIds` il ramo senza manager con unita' di due squadre in campo.
 *
 * 🔑 **Due livelli, perche' il difetto puo' nascere in due posti**: la funzione che decide l'insieme, e il
 * roster che lo consuma. Il controllo positivo in coda dice che la fixture SA produrre una lista avversaria:
 * senza, «nessuna carta avversaria» sarebbe vero anche per un mondo con una squadra sola.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudNoManagerNarrowestViewTest,
	"RefactorTactics.ScreenHud.WithoutAManagerTheObserverSetIsTheNarrowest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudNoManagerNarrowestViewTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ DestroyHudWidgetWorld(World); };

	TArray<ARTUnit*> Units;
	Units.Add(SpawnRosterUnit(World, /*TeamId=*/ 0, TEXT("Alfa")));
	Units.Add(SpawnRosterUnit(World, /*TeamId=*/ 0, TEXT("Bravo")));
	Units.Add(SpawnRosterUnit(World, /*TeamId=*/ 1, TEXT("Charlie")));
	Units.Add(SpawnRosterUnit(World, /*TeamId=*/ 1, TEXT("Delta")));
	if (!TestFalse(TEXT("le quattro unita' esistono"), Units.Contains(nullptr))) { return false; }

	// ── 1. La funzione che decide: senza manager l'insieme e' la sola squadra di chi guarda.
	const TArray<int32> SenzaManager = URTHudViewModel::ResolveObserverTeamIds(nullptr, /*PlayerTeamId=*/ 0, Units);
	TestEqual(TEXT("senza manager l'insieme ha una squadra sola"), SenzaManager.Num(), 1);
	TestTrue(TEXT("ed e' quella di chi guarda"), SenzaManager.Contains(0));
	TestFalse(TEXT("la squadra avversaria NON e' autorizzata"), SenzaManager.Contains(1));

	// ── 2. Il roster che lo consuma: un widget senza manager non ha una lista avversaria.
	// ⚠️ Nel mondo non esiste ancora nessun `ARTTurnManager`: il widget non ne puo' trovare uno da se'. E' una
	// premessa, quindi si asserisce: un refactor della fixture che ne spawnasse uno renderebbe questo passo
	// un doppione del controllo presidiato, e nessuno lo direbbe.
	if (!TestNull(TEXT("premessa: nel mondo non esiste ancora nessun TurnManager"),
			UGameplayStatics::GetActorOfClass(World, ARTTurnManager::StaticClass())))
	{
		return false;
	}
	URTTeamRosterWidget* Roster = NewObject<URTTeamRosterWidget>(World);
	if (!TestNotNull(TEXT("widget"), Roster)) { return false; }
	Roster->SetMatchContextForTest(nullptr, /*PlayerTeamId=*/ 0);
	TestEqual(TEXT("senza manager il roster e' la propria squadra"),
		HeroIdsOf(Roster->GetRoster()), FString(TEXT("Alfa,Bravo")));
	TestEqual(TEXT("senza manager non c'e' NESSUNA carta avversaria"), Roster->GetOpposingRoster().Num(), 0);

	// ── 3. CONTROLLO POSITIVO: la stessa fixture, con un manager non presidiato, la lista avversaria la produce.
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("turn manager"), TM)) { return false; }
	TM->SetUnattendedSession(true);
	Roster->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);
	TestEqual(TEXT("controllo: non presidiata, l'altra squadra ha la sua lista"),
		HeroIdsOf(Roster->GetOpposingRoster()), FString(TEXT("Charlie,Delta")));
	TestEqual(TEXT("controllo: e l'insieme le porta entrambe"),
		URTHudViewModel::ResolveObserverTeamIds(TM, /*PlayerTeamId=*/ 0, Units).Num(), 2);

	return true;
}

/**
 * `ComposeMountReport`: il dump dice chi del §4.1 e' stato costruito **e chi manca**.
 *
 * 🔑 **Quello che questo test protegge e' la RICORSIONE**, ed e' la ragione per cui la funzione esiste:
 * `UWidgetTree::ForEachWidget` cammina l'albero di UN Blueprint e si ferma sui `UUserWidget` innestati,
 * che hanno un albero loro — limite dichiarato in `RTMatchWidgetAssetTests.cpp`. Qui l'`ActionSlot` sta
 * DENTRO l'`ActionDock`: se la discesa sparisse, `ActionSlot` conterebbe `0` e questo test diventerebbe
 * rosso. Un dump fermo al primo livello direbbe «manca» di un widget presente, che e' peggio del
 * silenzio da cui veniamo.
 *
 * ⚠️ **Prova il camminatore, non il montaggio reale**: l'albero qui e' costruito a mano. Che una
 * PARTITA monti i sei widget resta `PIE-V01-SCREENHUD`, ed e' precisamente cio' che il dump serve a
 * leggere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudMountReportTest,
	"RefactorTactics.ScreenHud.MountReportNamesWhoIsMissing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudMountReportTest::RunTest(const FString&)
{
	// Senza radice il report PARLA, invece di rendere una lista vuota: una lista vuota si leggerebbe
	// come «nessun problema», che e' il contrario di cio' che sarebbe successo.
	const TArray<FString> SenzaRadice = URTTacticalHUDWidget::ComposeMountReport(nullptr);
	TestTrue(TEXT("senza radice il report dice qualcosa"), SenzaRadice.Num() > 0);

	URTTacticalHUDWidget* Radice = NewObject<URTTacticalHUDWidget>(GetTransientPackage());
	if (!TestNotNull(TEXT("radice di prova"), Radice)) { return false; }

	// Un dock, e DENTRO il dock uno slot: due livelli, che e' il minimo per distinguere una discesa
	// ricorsiva da una passata sola.
	Radice->WidgetTree = NewObject<UWidgetTree>(Radice);
	URTActionDockWidget* Dock = Radice->WidgetTree->ConstructWidget<URTActionDockWidget>(
		URTActionDockWidget::StaticClass(), TEXT("DockDiProva"));
	if (!TestNotNull(TEXT("dock di prova"), Dock)) { return false; }
	Radice->WidgetTree->RootWidget = Dock;

	Dock->WidgetTree = NewObject<UWidgetTree>(Dock);
	URTActionSlotWidget* Slot = Dock->WidgetTree->ConstructWidget<URTActionSlotWidget>(
		URTActionSlotWidget::StaticClass(), TEXT("SlotDiProva"));
	if (!TestNotNull(TEXT("slot di prova"), Slot)) { return false; }
	Dock->WidgetTree->RootWidget = Slot;

	const TArray<FString> Report = URTTacticalHUDWidget::ComposeMountReport(Radice);
	const FString Testo = FString::Join(Report, TEXT("|"));

	TestTrue(TEXT("il dock, che sta al primo livello, e' contato"),
		Testo.Contains(TEXT("[ok]    ActionDock: 1")));

	// 🔴 LA RIGA CHE PROTEGGE LA RICORSIONE: lo slot sta nell'albero del dock, non della radice.
	TestTrue(TEXT("lo slot INNESTATO nel dock e' contato: la discesa ricorsiva regge"),
		Testo.Contains(TEXT("[ok]    ActionSlot: 1")));

	// E chi non c'e' viene NOMINATO. E' la meta' del report che il log non sapeva dare: di un widget
	// mancante non si sapeva niente, nemmeno che fosse atteso.
	TestTrue(TEXT("il feed assente e' nominato, non taciuto"),
		Testo.Contains(TEXT("[MANCA] EventLog: 0")));
	TestTrue(TEXT("l'header assente e' nominato"),
		Testo.Contains(TEXT("[MANCA] TurnHeader: 0")));

	return true;
}

/**
 * Lo stesso widget raggiunto da DUE strade si conta UNA volta.
 *
 * 🔴 **Il rapporto contava doppio, e lo si e' visto solo in PIE** (2026-09-16, `#2964`): `ActionSlot: 20`
 * con dieci slot in campo — `_23`..`_32`, elencati due volte ciascuno — e `EventLog: 2` con un feed solo.
 * Il test che gia' esiste non poteva coglierlo: il suo albero da' a ogni widget **una** strada sola.
 *
 * La causa in partita e' `UWidgetTree::ForWidgetAndChildren`, che scende gia' da se' nei named slot del
 * widget che incontra: una `WBP_RT_HudZone` e' un `UUserWidget` con un `NamedSlot Content`, quindi la
 * camminata della radice emette gia' il suo inquilino, e la ricorsione dentro l'albero della zona lo emette
 * di nuovo.
 *
 * ⚠️ **Questa fixture riproduce la FORMA, non il meccanismo**, e va detto: costruire un named slot vivo
 * fuori da una classe generata da Blueprint dipende da come `GetSlotNames` e `GetContentForSlot` si
 * comportano su un `UUserWidget` istanziato a mano — cioe' da un dettaglio del motore che un test del
 * progetto non dovrebbe fissare. Qui le due strade si danno esplicitamente: lo slot e' figlio del pannello
 * della radice **e** radice dell'albero del dock. Cio' che il codice promette — *«una volta, comunque lo si
 * raggiunga»* — e' lo stesso, e questa forma lo misura senza dipendere dal motore.
 *
 * 🔑 **E il numero e' asseribile nei due versi**: `ActionSlot: 1` passerebbe anche con la deduplica rotta se
 * lo slot fosse raggiungibile una volta sola, quindi il test verifica PRIMA che le due strade esistano
 * davvero — il dock e' contato, e lo slot sta nel suo albero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudMountReportNoDoubleCountTest,
	"RefactorTactics.ScreenHud.MountReportCountsEachWidgetOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudMountReportNoDoubleCountTest::RunTest(const FString&)
{
	URTTacticalHUDWidget* Radice = NewObject<URTTacticalHUDWidget>(GetTransientPackage());
	if (!TestNotNull(TEXT("radice di prova"), Radice)) { return false; }

	Radice->WidgetTree = NewObject<UWidgetTree>(Radice);
	UVerticalBox* Pannello = Radice->WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("PannelloDiProva"));
	if (!TestNotNull(TEXT("pannello di prova"), Pannello)) { return false; }
	Radice->WidgetTree->RootWidget = Pannello;

	URTActionDockWidget* Dock = Radice->WidgetTree->ConstructWidget<URTActionDockWidget>(
		URTActionDockWidget::StaticClass(), TEXT("DockDiProva"));
	URTActionSlotWidget* Slot = Radice->WidgetTree->ConstructWidget<URTActionSlotWidget>(
		URTActionSlotWidget::StaticClass(), TEXT("SlotDiProva"));
	if (!TestNotNull(TEXT("dock di prova"), Dock)) { return false; }
	if (!TestNotNull(TEXT("slot di prova"), Slot)) { return false; }

	// Strada 1: lo slot e' figlio del pannello della radice, come un inquilino di named slot e' autorato
	// nell'albero ESTERNO.
	Pannello->AddChild(Dock);
	Pannello->AddChild(Slot);

	// Strada 2: lo stesso slot e' anche cio' che la ricorsione trova scendendo nell'albero del dock —
	// `RootWidget` e' un puntatore, non uno slot di parentela, quindi le due strade convivono.
	Dock->WidgetTree = NewObject<UWidgetTree>(Dock);
	Dock->WidgetTree->RootWidget = Slot;

	const TArray<FString> Report = URTTacticalHUDWidget::ComposeMountReport(Radice);
	const FString Testo = FString::Join(Report, TEXT("|"));

	// Controllo che le due strade ESISTANO: senza, l'asserzione sul conteggio sarebbe vacua.
	TestTrue(TEXT("il dock e' nell'albero della radice"), Testo.Contains(TEXT("[ok]    ActionDock: 1")));
	TestTrue(TEXT("e lo slot e' la radice dell'albero del dock"), Dock->WidgetTree->RootWidget == Slot);
	TestTrue(TEXT("e lo slot e' anche figlio del pannello della radice"), Slot->GetParent() == Pannello);

	// 🔴 LA RIGA: due strade, un conteggio.
	TestTrue(TEXT("lo slot raggiungibile da due strade e' contato UNA volta"),
		Testo.Contains(TEXT("[ok]    ActionSlot: 1")));

	// E l'elenco non lo nomina due volte, che e' la meta' che si legge a occhio in un log.
	int32 Occorrenze = 0;
	for (const FString& Riga : Report)
	{
		if (Riga.Contains(TEXT("SlotDiProva")))
		{
			++Occorrenze;
		}
	}
	TestEqual(TEXT("e l'elenco lo nomina una volta sola"), Occorrenze, 1);

	return true;
}

/**
 * La chiave si risolve UNA VOLTA per cambio azione, non a ogni lettura — e il test lo prova contando le
 * warning, non ispezionando la cache.
 *
 * 🔴 **Il difetto che ferma e' misurato**: la seduta PIE del 2026-09-11 ha prodotto **16 388** righe
 * `Icona non risolta` per **quattro** chiavi distinte. `ResolveIcon` logga — e' il suo scopo — ma il
 * Blueprint chiama `GetResolvedIcon` da un property binding, cioe' a ogni frame. Il docstring del getter
 * prescriveva gia' «un evento, una volta per cambio azione»; nulla lo rendeva vero.
 *
 * ⚠️ **Si contano le warning e non si guarda `CachedResolvedIcon`**: un test sul campo privato
 * passerebbe anche se il getter continuasse a loggare, che e' esattamente il difetto. Il soggetto e' il
 * RUMORE, quindi si misura quello.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionSlotResolvesOncePerActionChangeTest,
	"RefactorTactics.ScreenHud.ActionSlotResolvesOncePerActionChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTActionSlotResolvesOncePerActionChangeTest::RunTest(const FString&)
{
	URTActionSlotWidget* Slot = NewObject<URTActionSlotWidget>();
	if (!TestNotNull(TEXT("slot"), Slot)) { return false; }

	URTIconCatalogData* Catalogo = NewObject<URTIconCatalogData>();
	if (!TestNotNull(TEXT("catalogo"), Catalogo)) { return false; }
	Catalogo->MissingIcon = TSoftObjectPtr<UTexture2D>(
		FSoftObjectPath(TEXT("/Game/Prova/T_Missing.T_Missing")));

	// Un'abilita' PROPRIA: nessun ripiego, quindi la chiave non si risolve e `ResolveIcon` logga.
	// E' il caso reale di `Hero.Aevik.Overload` — uno dei quattro che la seduta ha visto ripetersi.
	FRTAbilityCooldownView Azione;
	Azione.ActionId = TEXT("Hero.Aevik.Overload");

	// 🔑 **Esattamente UNA**, non «almeno una»: `Occurrences 1` e' l'asserto di questo test. Con `0` —
	// la forma usata altrove per dire «almeno una» — il difetto delle 16 388 righe passerebbe.
	AddExpectedMessage(TEXT("Icona non risolta"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ 1, /*IsRegex*/ false);

	Slot->SetAction(Azione, /*bArmed=*/ false, Catalogo);

	// Venti letture: e' cio' che un property binding fa in un terzo di secondo.
	for (int32 i = 0; i < 20; ++i)
	{
		Slot->GetResolvedIcon();
	}

	// ⚠️ Il controllo che rende il test non vacuo: la risoluzione DEVE essere avvenuta. Senza, «una sola
	// warning» sarebbe soddisfatto anche da un getter che non risolve mai.
	const FRTIconResolution Esito = Slot->GetResolvedIcon();
	TestFalse(TEXT("un'abilita' propria senza asset non si risolve"), Esito.bResolved);
	TestFalse(TEXT("ma l'esito e' popolato: e' il missing-icon, non un valore vuoto"),
		Esito.Asset.IsNull());

	return true;
}

/**
 * 🔴 **Il pannello non porta MAI il piano di un'unita' ispezionata, e il nemico di questo test UN PIANO CE
 * L'HA.**
 *
 * ⚠️ **La premessa e' la meta' che conta.** Un test che ispezionasse un nemico senza piano passerebbe anche
 * con un `GetSlots()` che consegna tutto: non ci sarebbe niente da consegnare. Qui l'avversaria ha un
 * waypoint — quindi `BuildUnitSlots` la direbbe occupata — e il pannello continua a non dirlo.
 *
 * 🔑 **Il controllo positivo non e' cortesia**: senza il ramo sull'unita' comandata, un `GetSlots()` che
 * tornasse sempre il default passerebbe questo test mentre rompe il pannello per tutti.
 *
 * ⛔ E `bAuthorized` distingue il caso dal piano vuoto: un'area slot mostrata vuota per un'avversaria
 * direbbe *«non ha pianificato»*, che e' una lettura del suo piano — vietata da `#2757` in forma piu' forte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudInspectedSlotsTest,
	"RefactorTactics.ScreenHud.InspectedEnemyNeverCarriesItsPlannedSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudInspectedSlotsTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	ON_SCOPE_EXIT { DestroyHudWidgetWorld(World); };

	ARTUnit* Mine  = SpawnRosterUnit(World, 0, TEXT("Hero.Aevik"));
	ARTUnit* Enemy = SpawnRosterUnit(World, 1, TEXT("Hero.Ivrin"));
	URTSelectedUnitPanelWidget* Panel = NewObject<URTSelectedUnitPanelWidget>(World);
	if (!TestNotNull(TEXT("pannello"), Panel) || !Mine || !Enemy) { return false; }

	// **Entrambe** pianificano un movimento: senza il piano dell'avversaria non ci sarebbe nulla da NON
	// mostrare, e il test sarebbe vacuo.
	Mine->PlannedWaypoints.Add(FRTCellId(1, 0, 0));
	Enemy->PlannedWaypoints.Add(FRTCellId(2, 0, 0));

	// (1) Controllo POSITIVO — l'unita' comandata porta i suoi slot, autorizzati.
	Panel->SetSelectedUnitForTest(Mine);
	const FRTUnitSlotsView Comandata = Panel->GetSlots();
	TestTrue(TEXT("la comandata e' autorizzata"), Comandata.bAuthorized);
	TestTrue(TEXT("e il suo movimento risulta occupato"), Comandata.Movement.bOccupied);

	// (2) Ispezionata — stesso pannello, soggetto avversario: niente slot, e NON perche' non ne ha.
	Panel->SetSelectedUnitForTest(nullptr);
	Panel->SetInspectedUnitForTest(Enemy);

	TestFalse(TEXT("non si sta comandando nulla"), Panel->HasSelection());
	TestTrue(TEXT("ma il pannello ha un soggetto"), Panel->HasSubject());

	const FRTUnitSlotsView Ispezionata = Panel->GetSlots();
	TestFalse(TEXT("gli slot NON sono autorizzati"), Ispezionata.bAuthorized);
	TestFalse(TEXT("e il movimento dell'avversaria non trapela, benche' pianificato"),
		Ispezionata.Movement.bOccupied);
	TestFalse(TEXT("ne' la principale"), Ispezionata.Main.bOccupied);
	TestFalse(TEXT("ne' la reazione"), Ispezionata.Reaction.bOccupied);

	// La carta invece segue il soggetto: identita' e salute sono cio' che il velo gia' autorizza sopra la
	// testa di un'unita' osservata. Senza questa riga il pannello potrebbe essere vuoto e il test passerebbe.
	TestEqual(TEXT("la carta e' quella dell'avversaria che si sta guardando"),
		Panel->GetCard().HeroId, Enemy->HeroId);

	return true;
}

/**
 * PRIVACY: lo slot movimento di un soggetto NON comandato non dice «libero».
 *
 * 🔴 **«libero» sarebbe una lettura del piano avversario**: direbbe *«quell'unita' non ha pianificato
 * movimento»*. Il docstring di `GetSlots()` lo dichiara come obbligo di chi disegna — *«chi disegna deve
 * distinguere `bAuthorized == false` da un piano vuoto»* — e [#2757] lo vieta in forma piu' forte:
 * *«nessun conteggio o metadato da cui dedurre che un dato privato esiste»*.
 *
 * 🔑 **Il test esiste perche' l'obbligo e' stato spostato dal GRAFO del widget al C++.** Il binding di
 * `MovementText` componeva un `Select` sull'autorizzazione dentro il Blueprint, dove nessun test lo
 * raggiunge: ora e' una riga di `GetMovementSlotText()`, e questa la guarda.
 *
 * ⚠️ **Un widget appena costruito NON ha un'unita' comandata**, quindi `GetSlots()` rende il default con
 * `bAuthorized` falso: e' esattamente lo stato in esame, e non serve montare una partita per ottenerlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudMovementSlotUnauthorizedTest,
	"RefactorTactics.ScreenHud.MovementSlotSaysNothingWhenUnauthorized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudMovementSlotUnauthorizedTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTSelectedUnitPanelWidget* Panel = NewObject<URTSelectedUnitPanelWidget>(World);
	if (!TestNotNull(TEXT("pannello"), Panel)) { DestroyHudWidgetWorld(World); return false; }

	// Premessa: senza un'unita' comandata la vista non e' autorizzata. Si asserisce invece di assumerla —
	// se un giorno `GetSlots()` cambiasse, il test misurerebbe un altro stato senza accorgersene.
	if (!TestFalse(TEXT("premessa: la vista non e' autorizzata"), Panel->GetSlots().bAuthorized))
	{
		DestroyHudWidgetWorld(World); return false;
	}

	const FString Reso = Panel->GetMovementSlotText().ToString();

	// 🔴 Il cuore: non deve dire niente del piano, e «libero» ne direbbe.
	TestFalse(*FString::Printf(TEXT("non dice «libero» a chi non si comanda (reso: \"%s\")"), *Reso),
		Reso.Contains(TEXT("libero")));
	TestFalse(TEXT("ne' nomina un'andatura"), Reso.Contains(TEXT("percorso")));
	TestFalse(TEXT("ne' il ripiegamento"), Reso.Contains(TEXT("ripiegamento")));

	// ⛔ **ANTI-VACUITA'**: un reso vuoto passerebbe tutti i `TestFalse` qui sopra senza dire niente al
	// giocatore, e a schermo sarebbe un buco invece di un segnaposto. Il trattino e' quello che il binding
	// mostrava prima, e resta.
	TestFalse(TEXT("e non e' vuoto: a schermo resta un segnaposto"), Reso.IsEmpty());
	TestEqual(TEXT("ed e' il trattino, come prima"), Reso, FString(TEXT("-")));

	// 🔑 **La controprova che lo rende non vacuo dall'altro verso**: la stessa funzione, su una vista
	// AUTORIZZATA, dice «libero». Senza, un `GetMovementSlotText()` che rendesse sempre «-» passerebbe.
	FRTUnitSlotsView Autorizzata;
	Autorizzata.bAuthorized = true;
	TestEqual(TEXT("controllo positivo: autorizzata e senza piano dice «libero»"),
		ARTHUD::DescribeMovementSlot(Autorizzata).ToString(), FString(TEXT("libero")));

	DestroyHudWidgetWorld(World);
	return true;
}

/**
 * 🔑 **IL CHIP `REAZ.` SI ACCENDE SOLO PER UN'ALLEATA COMANDATA CON UNA REAZIONE ARMATA** ([D-478], #3618).
 *
 * Due accese: l'Overwatch come principale (arma una finestra pur occupando lo slot `Main`) e un'azione nello slot
 * `Reaction`. Quattro spente, e per ragioni diverse:
 * - `Brace` come principale: apre una finestra, ma D-478 nomina solo l'Overwatch (decisione del 2026-10-10);
 * - un'avversaria, un'alleata del bot, un'alleata di un altro gruppo di controllo: per loro il piano non si legge.
 *
 * ⚠️ **Lo spento delle ultime tre non deve venire da un piano vuoto.** Il test lo prova: per ciascuna
 * `BuildUnitSlots` direbbe vero. Se il roster saltasse il controllo «comandata», si accenderebbero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudRosterReactionChipTest,
	"RefactorTactics.ScreenHud.RosterReactionChipOnlyForCommandedAllies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudRosterReactionChipTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ DestroyHudWidgetWorld(World); };

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	URTTeamRosterWidget* Roster = NewObject<URTTeamRosterWidget>(World);
	if (!TestNotNull(TEXT("turn manager"), TM) || !TestNotNull(TEXT("widget"), Roster)) { return false; }
	Roster->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);
	Roster->SetControlGroupForTest(0);

	// Un'unita' con un'azione pianificata: come principale o nello slot della reazione.
	const auto Pianifica = [this, World](int32 TeamId, const TCHAR* HeroId, const TCHAR* ActionId, bool bReazione)
	{
		ARTUnit* Unit = SpawnRosterUnit(World, TeamId, HeroId);
		const int32 Indice = Unit ? RTAbilityFixtures::AddCoreAbility(Unit, ActionId) : INDEX_NONE;
		TestTrue(*FString::Printf(TEXT("premessa: %s ha %s nel kit"), HeroId, ActionId), Indice != INDEX_NONE);
		if (Unit)
		{
			(bReazione ? Unit->PlannedReactionAbility : Unit->PlannedAbilityIndex) = Indice;
		}
		return Unit;
	};

	ARTUnit* Guardia = Pianifica(0, TEXT("Guardia"), TEXT("Action.Overwatch"), /*bReazione=*/ false);
	ARTUnit* Contro = Pianifica(0, TEXT("Contro"), TEXT("Action.Counter"), /*bReazione=*/ true);
	ARTUnit* Saldo = Pianifica(0, TEXT("Saldo"), TEXT("Action.Brace"), /*bReazione=*/ false);
	ARTUnit* Nemica = Pianifica(1, TEXT("Nemica"), TEXT("Action.Overwatch"), /*bReazione=*/ false);
	ARTUnit* DelBot = Pianifica(0, TEXT("DelBot"), TEXT("Action.Overwatch"), /*bReazione=*/ false);
	ARTUnit* AltroGruppo = Pianifica(0, TEXT("AltroGruppo"), TEXT("Action.Overwatch"), /*bReazione=*/ false);
	if (!Guardia || !Contro || !Saldo || !Nemica || !DelBot || !AltroGruppo) { return false; }
	DelBot->bIsBotControlled = true;
	AltroGruppo->ControlGroup = 1;

	// --- 1. Le due accese ------------------------------------------------------------------------------------
	TestTrue(TEXT("l'Overwatch come principale accende il chip"), Roster->IsReactionArmed(TEXT("Guardia")));
	TestTrue(TEXT("un'azione nello slot della reazione accende il chip"), Roster->IsReactionArmed(TEXT("Contro")));

	// --- 2. Brace: il piano c'e', e non e' una reazione armata --------------------------------------------------
	TestFalse(TEXT("Brace come principale non accende il chip"), Roster->IsReactionArmed(TEXT("Saldo")));
	TestFalse(TEXT("e la vista lo dice anche senza il roster"), URTHudViewModel::BuildUnitSlots(Saldo).bReactionArmed);

	// --- 3. Le tre non comandate: il dato esisterebbe, e il roster non lo legge ----------------------------------
	for (const ARTUnit* NonComandata : { Nemica, DelBot, AltroGruppo })
	{
		const FString Nome = NonComandata->HeroId.ToString();
		if (!TestTrue(*FString::Printf(TEXT("premessa: il piano di %s arma una reazione"), *Nome),
				URTHudViewModel::BuildUnitSlots(NonComandata).bReactionArmed))
		{
			continue;
		}
		TestFalse(*FString::Printf(TEXT("%s non e' comandata: il chip resta spento"), *Nome),
			Roster->IsReactionArmed(NonComandata->HeroId));
	}

	TestFalse(TEXT("un HeroId che non e' in campo non accende niente"), Roster->IsReactionArmed(TEXT("Assente")));

	// --- 4. Lo stesso eroe nelle due squadre (un mirror): decide l'alleata, in qualunque ordine --------------------
	// L'ordinamento del roster per `HeroId` non e' stabile, quindi due coppie con spawn opposto: in almeno una
	// l'avversaria viene prima. La review di #3618 ha trovato che li' il chip dell'alleata si spegneva.
	Pianifica(1, TEXT("GemellaA"), TEXT("Action.Overwatch"), /*bReazione=*/ false);
	Pianifica(0, TEXT("GemellaA"), TEXT("Action.Overwatch"), /*bReazione=*/ false);
	Pianifica(0, TEXT("GemellaB"), TEXT("Action.Overwatch"), /*bReazione=*/ false);
	Pianifica(1, TEXT("GemellaB"), TEXT("Action.Overwatch"), /*bReazione=*/ false);
	TestTrue(TEXT("mirror, avversaria spawnata prima: l'alleata ha il chip"), Roster->IsReactionArmed(TEXT("GemellaA")));
	TestTrue(TEXT("mirror, alleata spawnata prima: l'alleata ha il chip"), Roster->IsReactionArmed(TEXT("GemellaB")));

	// --- 5. Senza contesto nessun chip: i default (squadra 0, gruppo 0) non sono chi guarda ----------------------
	URTTeamRosterWidget* SenzaContesto = NewObject<URTTeamRosterWidget>(World);
	TestFalse(TEXT("un roster senza contesto non accende il chip"), SenzaContesto->IsReactionArmed(TEXT("Guardia")));
	return true;
}

/**
 * 🔑 **LA CARTA DICE IL NOME, NON L'ID; IL CHIP SEGUE LA REAZIONE ARMATA** (#3636, T8).
 *
 * `WBP_RT_UnitCard` legava `Conv_NameToText(HeroId)`, e a schermo usciva «Hero.Aevik». La vista ora porta il nome di
 * `ARTUnit::DisplayLabel`, e la carta lo scrive. ⚠️ I testi si confrontano CaseSensitive: `TestEqual` su `FString`
 * ignora le maiuscole.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudUnitCardTest,
	"RefactorTactics.ScreenHud.UnitCardShowsTheReadableNameAndTheReactionChip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudUnitCardTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ DestroyHudWidgetWorld(World); };

	ARTUnit* Mia = SpawnRosterUnit(World, 0, TEXT("Hero.Aevik"));
	ARTUnit* Sua = SpawnRosterUnit(World, 1, TEXT("Hero.Ivrin"));
	URTUnitCardWidget* Carta = NewObject<URTUnitCardWidget>(World);
	if (!Mia || !Sua || !TestNotNull(TEXT("carta"), Carta)) { return false; }

	const auto Uguale = [](const FText& Testo, const FString& Atteso) { return Testo.ToString().Equals(Atteso, ESearchCase::CaseSensitive); };
	const auto Visibile = [](const UWidget* W) { return W && W->GetVisibility() != ESlateVisibility::Collapsed; };
	const auto Colore = [](ERTUIToken Token) { return URTUIPalette::ColorFor(Token); };

	// --- La vista: il nome leggibile ---------------------------------------------------------------------------------------
	const FRTUnitCardView Vista = URTHudViewModel::BuildUnitCard(Mia, /*PlayerTeamId=*/ 0);
	const FString Atteso = ARTUnit::DisplayLabel(Mia->HeroDisplayName, Mia->HeroId, Mia->GetName());
	TestTrue(TEXT("la vista porta il nome di DisplayLabel"), Uguale(Vista.DisplayName, Atteso));
	TestFalse(*FString::Printf(TEXT("e non l'id grezzo (reso: «%s»)"), *Vista.DisplayName.ToString()),
		Vista.DisplayName.ToString().Equals(Mia->HeroId.ToString(), ESearchCase::CaseSensitive));

	// --- La carta: le porte ------------------------------------------------------------------------------------------------
	Carta->CardFrame = NewObject<UBorder>(World);
	Carta->TeamDot = NewObject<UBorder>(World);
	Carta->NameText = NewObject<UTextBlock>(World);
	Carta->ReactionChip = NewObject<UBorder>(World);
	Carta->HealthBar = NewObject<UProgressBar>(World);
	Carta->HealthText = NewObject<UTextBlock>(World);
	Carta->ShieldText = NewObject<UTextBlock>(World);

	Carta->SetCard(Vista, /*bReactionArmed=*/ true);
	TestTrue(TEXT("la carta scrive il nome leggibile"), Uguale(Carta->NameText->GetText(), Atteso));
	TestTrue(TEXT("con la reazione armata il chip REAZ. c'e'"), Visibile(Carta->ReactionChip));
	TestTrue(TEXT("il pallino di un'alleata e' Cyan"), Carta->TeamDot->GetBrushColor().Equals(Colore(ERTUIToken::Cyan)));
	TestTrue(TEXT("i PV dicono 10/10"), Uguale(Carta->HealthText->GetText(), TEXT("10/10")));
	TestEqual(TEXT("e la barra e' piena"), Carta->HealthBar->GetPercent(), 1.f);
	TestEqual(TEXT("una carta viva e' opaca"), Carta->CardFrame->GetRenderOpacity(), 1.f);

	Carta->SetCard(Vista, /*bReactionArmed=*/ false);
	TestFalse(TEXT("senza reazione armata il chip si chiude"), Visibile(Carta->ReactionChip));

	Mia->Health = 4;
	Carta->SetCard(URTHudViewModel::BuildUnitCard(Mia, 0), false);
	TestTrue(TEXT("i PV seguono la vista: 4/10"), Uguale(Carta->HealthText->GetText(), TEXT("4/10")));
	TestTrue(TEXT("e la barra e' al 40%"), FMath::IsNearlyEqual(Carta->HealthBar->GetPercent(), 0.4f, 1e-4f));

	Mia->Health = 0;
	Carta->SetCard(URTHudViewModel::BuildUnitCard(Mia, 0), false);
	TestTrue(TEXT("una carta caduta si attenua"), Carta->CardFrame->GetRenderOpacity() < 1.f);

	Carta->SetCard(URTHudViewModel::BuildUnitCard(Sua, /*PlayerTeamId=*/ 0), false);
	TestTrue(TEXT("il pallino di un'avversaria e' Red"), Carta->TeamDot->GetBrushColor().Equals(Colore(ERTUIToken::Red)));
	return true;
}

namespace
{
	/**
	 * Raccoglie gli avvisi e gli errori che il THREAD CHE LO CREA scrive nel log, finche' vive.
	 *
	 * Esiste perche' un `Warning` non dichiarato non fa fallire un test (`bElevateLogWarningsToErrors` e' falso): una
	 * carta che scrivesse un avviso alla prima disegnata passerebbe da sola. Senza buffer (`CanBeUsedOnMultipleThreads`)
	 * per la ragione di `FRTScopedLogLines` in RTHeroSpawnTests.cpp: un dispositivo bufferizzato riceve le righe in
	 * ritardo, e l'asserto subito dopo la chiamata leggerebbe zero. Il filtro per thread tiene fuori cio' che scrive un
	 * altro thread mentre la carta disegna.
	 */
	struct FRTCardLogWatch : public FOutputDevice
	{
		FRTCardLogWatch() : OwnerThread(FPlatformTLS::GetCurrentThreadId()) { GLog->AddOutputDevice(this); }
		virtual ~FRTCardLogWatch() override { GLog->RemoveOutputDevice(this); }

		virtual bool CanBeUsedOnAnyThread() const override { return true; }
		virtual bool CanBeUsedOnMultipleThreads() const override { return true; }

		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if ((Verbosity & ELogVerbosity::VerbosityMask) <= ELogVerbosity::Warning
				&& FPlatformTLS::GetCurrentThreadId() == OwnerThread)
			{
				FScopeLock Lock(&Guardia);
				Righe.Add(FString::Printf(TEXT("%s: %s"), *Category.ToString(), V ? V : TEXT("")));
			}
		}

		TArray<FString> Righe;

	private:
		const uint32 OwnerThread;
		FCriticalSection Guardia;
	};
}

/**
 * LA BARRA DEI PV DELLA CARTA LEGGE LA FRAZIONE DELLA VISTA GIA' ALLA PRIMA DISEGNATA (#1896).
 *
 * Il difetto: alla PRIMA selezione la barra della carta restava vuota. Il grafo `Get_HealthBar_Percent` di
 * `WBP_RT_UnitCard` divideva per `FRTUnitCardView::MaxHealth`, che in una vista mai popolata vale 0 (`Divide by zero:
 * Divide_DoubleDouble`, seduta U49). Riparentata la carta su `URTUnitCardWidget` (#3658) la scrittura e' una riga di
 * `SetCard`: `HealthBar->SetPercent(Card.HealthFraction)`. La divisione protetta sta nella vista
 * (`HudViewModel.HealthFractionNeedsNoDivisionInTheWidget`); qui si prova l'altra meta': che il WIDGET applichi il
 * campo alla barra, e che lo faccia anche alla prima chiamata.
 *
 * Questo test e' una GUARDIA e non la prova di una correzione: il comportamento esiste gia' e il test nasce verde. La sua
 * validita' sta nelle mutazioni: barra non scritta, barra ricalcolata nel widget senza guardia sul denominatore, scrittura
 * saltata alla prima chiamata.
 *
 * Le gambe: (a) prima carta a PV pieni; (b) carta ferita; (c) carta mai popolata, il caso del difetto; (d) dal CHIAMANTE,
 * cioe' da un'unita' vera attraverso `BuildUnitCard` e attraverso il pannello dell'unita' selezionata, che e' dove la
 * prima selezione avviene davvero; (e) il controllo positivo: due carte che devono dare due letture diverse; (f) la barra
 * applica il campo e non lo ricalcola (la riga del DoD di #613: i widget non ricalcolano formule); (f2) e non lo
 * deforma: una frazione piccola e una scomoda arrivano alla barra tali e quali (gamba nata dalla revisione indipendente
 * di PR #3826); (g) nessun avviso nel
 * log durante le scritture, con una sonda che prova che il rilevatore ne vede uno.
 *
 * Cosa NON prova: il disegno della barra a schermo (e' PIE), ne' il grafo Blueprint che non esiste piu'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudUnitCardBarTest,
	"RefactorTactics.ScreenHud.UnitCardBarFollowsHealthFractionFromTheFirstDraw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudUnitCardBarTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ DestroyHudWidgetWorld(World); };

	// Una carta NUOVA, che non ha mai ricevuto una vista: e' lo stato della prima selezione. Ha la sola barra, e anche la
	// barra e' nuova.
	const auto CartaNuova = [World]() -> URTUnitCardWidget*
	{
		URTUnitCardWidget* Carta = NewObject<URTUnitCardWidget>(World);
		if (Carta) { Carta->HealthBar = NewObject<UProgressBar>(World); }
		return Carta;
	};
	const auto Barra = [](const URTUnitCardWidget* Carta) { return Carta->HealthBar->GetPercent(); };
	const auto Vicina = [](float Letta, float Attesa) { return FMath::IsFinite(Letta) && FMath::IsNearlyEqual(Letta, Attesa, 1e-4f); };

	// Una vista scritta a mano: qui si prova il WIDGET, non il view model che la produce.
	const auto Vista = [](int32 Health, int32 Max, float Frazione)
	{
		FRTUnitCardView V;
		V.Health = Health;
		V.MaxHealth = Max;
		V.HealthFraction = Frazione;
		V.bAlive = Health > 0;
		V.bIsAlly = true;
		return V;
	};

	ARTUnit* Unita = SpawnRosterUnit(World, 0, TEXT("Hero.Aevik"));
	URTSelectedUnitPanelWidget* Pannello = NewObject<URTSelectedUnitPanelWidget>(World);
	if (!TestNotNull(TEXT("unita'"), Unita) || !TestNotNull(TEXT("pannello"), Pannello)) { return false; }
	Pannello->SubjectCard = CartaNuova();
	if (!TestNotNull(TEXT("carta del pannello"), Pannello->SubjectCard.Get())) { return false; }

	// L'assenza di avvisi si misura da qui in poi: ogni scrittura della barra sta sotto lo sguardo del log.
	FRTCardLogWatch Log;

	// --- a. PRIMA DISEGNATA, ILLESA ---------------------------------------------------------------------------------------
	URTUnitCardWidget* Prima = CartaNuova();
	if (!TestNotNull(TEXT("a: carta"), Prima)) { return false; }
	TestEqual(TEXT("a: premessa: una barra mai scritta legge 0, quindi un pieno puo' venire solo da SetCard"), Barra(Prima), 0.f);
	Prima->SetCard(Vista(10, 10, 1.f), /*bInReactionArmed=*/ false);
	const float LettaPiena = Barra(Prima);
	TestEqual(TEXT("a: alla prima carta, a PV pieni, la barra e' piena"), LettaPiena, 1.f);

	// --- b. FERITA ----------------------------------------------------------------------------------------------------------
	// Su una carta NUOVA, non su quella di (a): una frazione intermedia che non viene da un 'pieno' precedente.
	URTUnitCardWidget* Ferita = CartaNuova();
	if (!TestNotNull(TEXT("b: carta"), Ferita)) { return false; }
	Ferita->SetCard(Vista(4, 10, 0.4f), false);
	const float LettaFerita = Barra(Ferita);
	TestTrue(*FString::Printf(TEXT("b: la barra e' al 40%% (letta %f)"), LettaFerita), Vicina(LettaFerita, 0.4f));
	TestTrue(TEXT("b: e sta sotto il pieno"), LettaFerita < 1.f);
	// Sulla stessa carta la barra segue la vista nei due versi: cura fino al pieno, poi di nuovo il danno.
	Ferita->SetCard(Vista(10, 10, 1.f), false);
	TestEqual(TEXT("b: curata fino al pieno, la barra risale"), Barra(Ferita), 1.f);
	Ferita->SetCard(Vista(4, 10, 0.4f), false);
	TestTrue(TEXT("b: ferita di nuovo, la barra riscende"), Vicina(Barra(Ferita), 0.4f));

	// --- c. CARTA MAI POPOLATA: il caso del difetto ----------------------------------------------------------------------
	// Il denominatore del grafo era `MaxHealth`, e qui vale 0. Due forme dello stesso stato: la vista di default (quella che
	// una carta tiene prima di ricevere qualunque cosa) e la vista che `BuildUnitCard` rende senza unita'.
	TArray<FRTUnitCardView> Vuote;
	Vuote.Add(FRTUnitCardView());
	Vuote.Add(URTHudViewModel::BuildUnitCard(nullptr, 0));
	for (int32 i = 0; i < Vuote.Num(); ++i)
	{
		const FRTUnitCardView& Vuota = Vuote[i];
		if (!TestEqual(*FString::Printf(TEXT("c%d: premessa: il denominatore del difetto, MaxHealth, vale 0"), i), Vuota.MaxHealth, 0))
		{
			continue;
		}

		// Barra nuova: e' la prima selezione cosi' com'era.
		URTUnitCardWidget* Mai = CartaNuova();
		// Barra con un valore SENTINELLA: uno 0 letto dopo SetCard deve essere stato scritto, non essere il residuo
		// dell'inizializzazione. Senza questa carta, togliere la scrittura della barra lascerebbe questa gamba verde.
		URTUnitCardWidget* Sentinella = CartaNuova();
		if (!TestNotNull(TEXT("c: carte"), Mai) || !TestNotNull(TEXT("c: carta sentinella"), Sentinella)) { return false; }
		Sentinella->HealthBar->SetPercent(0.7f);

		Mai->SetCard(Vuota, false);
		Sentinella->SetCard(Vuota, false);
		const float LettaMai = Barra(Mai);
		const float LettaSentinella = Barra(Sentinella);
		TestTrue(*FString::Printf(TEXT("c%d: la barra di una carta vuota e' un numero finito (letta %f)"), i, LettaMai), FMath::IsFinite(LettaMai));
		TestEqual(*FString::Printf(TEXT("c%d: e a zero"), i), LettaMai, 0.f);
		TestTrue(*FString::Printf(TEXT("c%d: la barra con la sentinella e' finita (letta %f)"), i, LettaSentinella), FMath::IsFinite(LettaSentinella));
		TestEqual(*FString::Printf(TEXT("c%d: e la sentinella e' stata riscritta a zero"), i), LettaSentinella, 0.f);
	}

	// --- d. DAL CHIAMANTE ---------------------------------------------------------------------------------------------------
	// Un test sulla vista a mano passa anche se il chiamante passa il valore sbagliato: qui il dato viene da un'unita' vera.
	{
		const FRTUnitCardView VistaReale = URTHudViewModel::BuildUnitCard(Unita, /*PlayerTeamId=*/ 0);
		if (!TestTrue(TEXT("d: premessa: un'unita' viva ha un massimo positivo"), VistaReale.MaxHealth > 0)) { return false; }
		URTUnitCardWidget* DaUnita = CartaNuova();
		if (!TestNotNull(TEXT("d: carta"), DaUnita)) { return false; }
		DaUnita->SetCard(VistaReale, false);
		TestEqual(TEXT("d: unita' vera a PV pieni -> BuildUnitCard -> SetCard: la barra e' piena"), Barra(DaUnita), 1.f);

		Unita->Health = 4;
		URTUnitCardWidget* DaFerita = CartaNuova();
		if (!TestNotNull(TEXT("d: carta della ferita"), DaFerita)) { return false; }
		DaFerita->SetCard(URTHudViewModel::BuildUnitCard(Unita, 0), false);
		TestTrue(TEXT("d: la stessa unita' a 4/10: la barra e' al 40%"), Vicina(Barra(DaFerita), 0.4f));
		Unita->Health = Unita->MaxHealth;
	}

	// Il pannello dell'unita' selezionata e' il chiamante che ha vissuto il difetto: la carta nasce vuota, e la PRIMA
	// selezione la riempie.
	TestEqual(TEXT("d: premessa: prima della selezione la barra del pannello legge 0"), Barra(Pannello->SubjectCard), 0.f);
	Pannello->SetSelectedUnitForTest(Unita);
	Pannello->RefreshLook();
	TestEqual(TEXT("d: alla PRIMA selezione la barra del pannello e' piena"), Barra(Pannello->SubjectCard), 1.f);
	Pannello->SetSelectedUnitForTest(nullptr);
	Pannello->RefreshLook();
	const float LettaDeselezionata = Barra(Pannello->SubjectCard);
	TestTrue(*FString::Printf(TEXT("d: deselezionata la carta si svuota e la barra e' un numero finito (letta %f)"), LettaDeselezionata),
		FMath::IsFinite(LettaDeselezionata));
	TestEqual(TEXT("d: e torna a zero"), LettaDeselezionata, 0.f);

	// --- e. CONTROLLO POSITIVO ---------------------------------------------------------------------------------------------
	// Due carte che DEVONO dare due letture diverse: un widget che rendesse sempre lo stesso numero non le passa.
	TestTrue(*FString::Printf(TEXT("e: la carta piena (%f) e la carta ferita (%f) danno due letture diverse"), LettaPiena, LettaFerita),
		LettaPiena > LettaFerita);

	// --- f. LA BARRA APPLICA IL CAMPO, NON LO RICALCOLA ----------------------------------------------------------------------
	// Una vista incoerente, scritta a mano APPOSTA: 3/10 ricalcolato darebbe 0.3, il campo dice 0.9. Se la barra legge 0.9
	// il widget applica `HealthFraction`; se legge 0.3 divide da se', ed e' la forma del difetto.
	URTUnitCardWidget* Applica = CartaNuova();
	if (!TestNotNull(TEXT("f: carta"), Applica)) { return false; }
	Applica->SetCard(Vista(3, 10, 0.9f), false);
	const float LettaCampo = Barra(Applica);
	TestTrue(*FString::Printf(TEXT("f: la barra legge il campo della vista, 0.9, e non Health/MaxHealth (letta %f)"), LettaCampo),
		Vicina(LettaCampo, 0.9f));

	// --- f2. LA BARRA NON DEFORMA LA FRAZIONE ----------------------------------------------------------------------------
	// Nata dalla revisione indipendente: i valori delle gambe precedenti (1, 0.4, 0.9 e lo 0 della carta vuota) non vedono una
	// RIMAPPATURA della barra - una soglia minima visibile per le unita' vive, una quantizzazione, una curva - e il test
	// restava verde. Una frazione piccola e una scomoda devono arrivare alla barra tali e quali: sotto una soglia minima di
	// 0.05 la piccola legge 0.05.
	URTUnitCardWidget* Piccola = CartaNuova();
	URTUnitCardWidget* Scomoda = CartaNuova();
	if (!TestNotNull(TEXT("f2: carta della frazione piccola"), Piccola) || !TestNotNull(TEXT("f2: carta della frazione scomoda"), Scomoda)) { return false; }
	Piccola->SetCard(Vista(1, 100, 0.01f), false);
	Scomoda->SetCard(Vista(37, 100, 0.37f), false);
	const float LettaPiccola = Barra(Piccola);
	const float LettaScomoda = Barra(Scomoda);
	TestTrue(*FString::Printf(TEXT("f2: la frazione piccola arriva alla barra tale e quale, 0.01 (letta %f)"), LettaPiccola),
		Vicina(LettaPiccola, 0.01f));
	TestTrue(*FString::Printf(TEXT("f2: la frazione scomoda arriva alla barra tale e quale, 0.37 (letta %f)"), LettaScomoda),
		Vicina(LettaScomoda, 0.37f));

	// --- g. NESSUN AVVISO NEL LOG, E IL RILEVATORE SA VEDERNE UNO ----------------------------------------------------------
	// La sonda dichiarata va DOPO l'asserto sullo zero, e il suo testo non compare in nessuna descrizione (un asserto che
	// citasse la riga attesa verrebbe inghiottito come errore atteso).
	// ⚠️ Va scritta su `GLog` e non con `UE_LOG`: il framework sostituisce `GWarn` con un filtro che declassa a `Verbose`
	// i messaggi attesi PRIMA che arrivino al log, e il rilevatore leggerebbe zero anche a funzionare (misurato: la prima
	// stesura, con `UE_LOG(LogTemp, Warning, ...)`, vedeva la riga come `Verbose`). Un avviso VERO non dichiarato attraversa
	// lo stesso filtro senza essere declassato, quindi arriva al rilevatore come `Warning`.
	TestEqual(*FString::Printf(TEXT("g: nessuna scrittura della barra ha lasciato un avviso nel log (righe: %s)"), *FString::Join(Log.Righe, TEXT(" | "))),
		Log.Righe.Num(), 0);
	AddExpectedMessage(TEXT("sonda del controllo positivo"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ 1, /*IsRegex*/ false);
	GLog->Serialize(TEXT("sonda del controllo positivo della barra PV"), ELogVerbosity::Warning, FName(TEXT("LogRTCardBarProbe")));
	TestEqual(TEXT("g: il rilevatore degli avvisi ne vede uno vero"), Log.Righe.Num(), 1);

	return true;
}

/**
 * 🔑 **IL ROSTER COSTRUISCE LE CARTE IN C++, DALLA CLASSE DEI DEFAULT** (#3636). Una carta per voce del roster,
 * nell'ordine del roster, col chip di `IsReactionArmed`; e le carte che ci sono si riusano.
 *
 * ⚠️ La classe e' quella C++: in una run headless non c'e' l'albero del `.uasset`, quindi le porte delle carte restano
 * nulle e qui si misura cio' che il roster consegna, non cio' che la carta disegna (lo misura il test qui sopra).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudRosterCardsTest,
	"RefactorTactics.ScreenHud.RosterBuildsItsCardsFromTheClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudRosterCardsTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ DestroyHudWidgetWorld(World); };

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	URTTeamRosterWidget* Roster = NewObject<URTTeamRosterWidget>(World);
	if (!TestNotNull(TEXT("turn manager"), TM) || !TestNotNull(TEXT("widget"), Roster)) { return false; }
	Roster->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);
	Roster->SetControlGroupForTest(0);

	ARTUnit* Guardia = SpawnRosterUnit(World, 0, TEXT("Guardia"));
	ARTUnit* Quieta = SpawnRosterUnit(World, 0, TEXT("Quieta"));
	ARTUnit* Nemica = SpawnRosterUnit(World, 1, TEXT("Nemica"));
	if (!Guardia || !Quieta || !Nemica) { return false; }
	const int32 Overwatch = RTAbilityFixtures::AddCoreAbility(Guardia, TEXT("Action.Overwatch"));
	if (!TestTrue(TEXT("premessa: la Guardia ha l'Overwatch"), Overwatch != INDEX_NONE)) { return false; }
	Guardia->PlannedAbilityIndex = Overwatch;

	// --- Senza pannello o senza classe: niente --------------------------------------------------------------------------
	Roster->RefreshCards();
	TestEqual(TEXT("senza pannello e classe il roster non costruisce carte"), Roster->GetCardWidgets().Num(), 0);
	UVerticalBox* Pannello = NewObject<UVerticalBox>(Roster);
	Roster->CardsBox = Pannello;
	Roster->RefreshCards();
	TestEqual(TEXT("senza classe nemmeno"), Roster->GetCardWidgets().Num(), 0);

	// --- Con pannello e classe: una carta per voce, nell'ordine del roster --------------------------------------------------
	Roster->CardClass = URTUnitCardWidget::StaticClass();
	Roster->RefreshCards();
	const TArray<FRTUnitCardView> Voci = Roster->GetRoster();
	if (!TestEqual(TEXT("una carta per voce del roster"), Roster->GetCardWidgets().Num(), Voci.Num())
		|| !TestEqual(TEXT("e tutte nel pannello"), Pannello->GetChildrenCount(), Voci.Num())
		|| !TestTrue(TEXT("premessa: il roster ha due voci, le alleate"), Voci.Num() == 2))
	{
		return false;
	}
	int32 Armate = 0;
	for (int32 i = 0; i < Voci.Num(); ++i)
	{
		const URTUnitCardWidget* Carta = Roster->GetCardWidgets()[i];
		TestEqual(*FString::Printf(TEXT("la carta %d e' la voce %d"), i, i), Carta->GetCard().HeroId, Voci[i].HeroId);
		TestEqual(*FString::Printf(TEXT("e il suo chip segue IsReactionArmed (%s)"), *Voci[i].HeroId.ToString()),
			Carta->IsReactionArmed(), Roster->IsReactionArmed(Voci[i].HeroId));
		Armate += Carta->IsReactionArmed() ? 1 : 0;
	}
	TestEqual(TEXT("una sola carta ha il chip"), Armate, 1);
	// Non circolare: la carta col chip e' proprio quella della Guardia, e quella della Quieta no.
	for (const URTUnitCardWidget* Carta : Roster->GetCardWidgets())
	{
		if (Carta->GetCard().HeroId == FName(TEXT("Guardia")))
		{
			TestTrue(TEXT("la carta della Guardia ha il chip"), Carta->IsReactionArmed());
		}
		if (Carta->GetCard().HeroId == FName(TEXT("Quieta")))
		{
			TestFalse(TEXT("la carta della Quieta no"), Carta->IsReactionArmed());
		}
	}

	// --- Un secondo giro riusa le stesse carte ----------------------------------------------------------------------------
	const URTUnitCardWidget* Prima = Roster->GetCardWidgets()[0];
	Roster->RefreshCards();
	TestTrue(TEXT("le carte si riusano: stessa istanza al secondo giro"), Roster->GetCardWidgets()[0] == Prima);
	TestEqual(TEXT("e il pannello non raddoppia"), Pannello->GetChildrenCount(), Voci.Num());

	// --- Una voce in meno: la carta in eccesso si toglie -------------------------------------------------------------------
	Quieta->TeamId = 1; // non e' piu' della squadra di chi guarda
	Roster->RefreshCards();
	TestEqual(TEXT("una voce in meno, una carta in meno"), Roster->GetCardWidgets().Num(), Voci.Num() - 1);
	TestEqual(TEXT("e il pannello la toglie"), Pannello->GetChildrenCount(), Voci.Num() - 1);

	// --- Il chip si riconta a 5 Hz, i PV no: senza riconteggio il chip resta, il resto segue -------------------------------
	Guardia->PlannedAbilityIndex = INDEX_NONE;
	Guardia->Health = 3;
	const auto CartaDi = [Roster](const TCHAR* Eroe) -> const URTUnitCardWidget*
	{
		for (const URTUnitCardWidget* Carta : Roster->GetCardWidgets())
		{
			if (Carta->GetCard().HeroId == FName(Eroe)) { return Carta; }
		}
		return nullptr;
	};
	Roster->RefreshCards(/*bRecountReactions=*/ false);
	const URTUnitCardWidget* DellaGuardia = CartaDi(TEXT("Guardia"));
	if (!TestNotNull(TEXT("premessa: la carta della Guardia c'e'"), DellaGuardia)) { return false; }
	TestTrue(TEXT("senza riconteggio il chip della Guardia resta quello di prima"), DellaGuardia->IsReactionArmed());
	TestEqual(TEXT("ma i PV seguono subito"), DellaGuardia->GetCard().Health, 3);
	Roster->RefreshCards(/*bRecountReactions=*/ true);
	TestFalse(TEXT("al riconteggio il chip si spegne"), CartaDi(TEXT("Guardia"))->IsReactionArmed());
	return true;
}

/**
 * 🔑 **LE CARTE DELLE ALTRE SQUADRE LE COSTRUISCE IL C++, E SOLO FUORI DALLA SESSIONE PRESIDIATA** (#3636, #2744).
 *
 * 🔴 **Prima le costruiva il secondo ciclo del grafo di `WBP_RT_TeamRoster`**, e la seduta di #3636 toglie il grafo.
 * Senza `OpposingCardsBox` le carte avversarie sparivano dall'autobattle senza che niente cadesse: il gemello di
 * questo test, `RosterSplitsTheTeamsOnlyInAnUnattendedSession`, misura la LISTA, non le carte.
 *
 * ⚠️ **Il chip `REAZ.` di un'avversaria resta spento anche col suo piano armato**: il piano di chi non si comanda
 * non lascia il core (D-478 punto 4). Per questo la nemica qui ha un Overwatch pianificato: senza, il chip spento
 * sarebbe vero per qualunque ragione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudRosterOpposingCardsTest,
	"RefactorTactics.ScreenHud.RosterBuildsTheOpposingCardsOnlyInAnUnattendedSession",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudRosterOpposingCardsTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ DestroyHudWidgetWorld(World); };

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	URTTeamRosterWidget* Roster = NewObject<URTTeamRosterWidget>(World);
	if (!TestNotNull(TEXT("turn manager"), TM) || !TestNotNull(TEXT("widget"), Roster)) { return false; }
	Roster->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);

	SpawnRosterUnit(World, 0, TEXT("Alfa"));
	SpawnRosterUnit(World, 0, TEXT("Bravo"));
	ARTUnit* Charlie = SpawnRosterUnit(World, 1, TEXT("Charlie"));
	SpawnRosterUnit(World, 1, TEXT("Delta"));
	if (!Charlie) { return false; }
	const int32 Overwatch = RTAbilityFixtures::AddCoreAbility(Charlie, TEXT("Action.Overwatch"));
	if (!TestTrue(TEXT("premessa: la nemica ha l'Overwatch"), Overwatch != INDEX_NONE)) { return false; }
	Charlie->PlannedAbilityIndex = Overwatch;
	// Il suo piano ARMEREBBE il chip, se lo si leggesse: senza questa premessa il chip spento sarebbe vero per caso.
	if (!TestTrue(TEXT("premessa: il piano della nemica arma una reazione"), URTHudViewModel::BuildUnitSlots(Charlie).bReactionArmed))
	{
		return false;
	}

	UVerticalBox* Proprie = NewObject<UVerticalBox>(Roster);
	UVerticalBox* Altre = NewObject<UVerticalBox>(Roster);
	Roster->CardsBox = Proprie;
	Roster->OpposingCardsBox = Altre;
	Roster->CardClass = URTUnitCardWidget::StaticClass();
	const auto Ids = [](const TArray<TObjectPtr<URTUnitCardWidget>>& Carte)
	{
		TArray<FString> Out;
		for (const URTUnitCardWidget* Carta : Carte) { Out.Add(Carta->GetCard().HeroId.ToString()); }
		return FString::Join(Out, TEXT(","));
	};

	// --- 1. Presidiata: nessuna carta avversaria, le proprie si' ------------------------------------------------------
	if (!TestFalse(TEXT("premessa: la sessione e' presidiata"), TM->IsUnattendedSession())) { return false; }
	Roster->RefreshCards();
	TestEqual(TEXT("1: le carte proprie ci sono"), Ids(Roster->GetCardWidgets()), FString(TEXT("Alfa,Bravo")));
	TestEqual(TEXT("1: nessuna carta avversaria"), Roster->GetOpposingCardWidgets().Num(), 0);
	TestEqual(TEXT("1: e il pannello delle altre e' vuoto"), Altre->GetChildrenCount(), 0);

	// --- 2. Non presidiata: le carte dell'altra squadra, nel loro pannello -----------------------------------------------
	TM->SetUnattendedSession(true);
	Roster->RefreshCards();
	TestEqual(TEXT("2: una carta per voce di GetOpposingRoster, nel suo ordine"),
		Ids(Roster->GetOpposingCardWidgets()), FString(TEXT("Charlie,Delta")));
	TestEqual(TEXT("2: tutte nel pannello delle altre"), Altre->GetChildrenCount(), 2);
	TestEqual(TEXT("2: e nessuna fra le proprie"), Proprie->GetChildrenCount(), 2);
	for (const URTUnitCardWidget* Carta : Roster->GetOpposingCardWidgets())
	{
		TestFalse(*FString::Printf(TEXT("2: '%s' non porta il chip, anche col piano armato"), *Carta->GetCard().HeroId.ToString()),
			Carta->IsReactionArmed());
		TestFalse(*FString::Printf(TEXT("2: '%s' e' un'avversaria"), *Carta->GetCard().HeroId.ToString()), Carta->GetCard().bIsAlly);
	}

	// --- 3. Di nuovo presidiata: le carte avversarie si tolgono, le proprie restano ------------------------------------------
	TM->SetUnattendedSession(false);
	Roster->RefreshCards();
	TestEqual(TEXT("3: le carte avversarie si tolgono"), Roster->GetOpposingCardWidgets().Num(), 0);
	TestEqual(TEXT("3: anche dal pannello"), Altre->GetChildrenCount(), 0);
	TestEqual(TEXT("3: le proprie restano"), Ids(Roster->GetCardWidgets()), FString(TEXT("Alfa,Bravo")));
	return true;
}


/**
 * 🔑 **OGNI AVVISO A UN CHIP SOLO** (#3636, review): la principale se ne nomina l'azione, la reazione se nomina la sua,
 * altrimenti il movimento — comprese le voci che `MakePlanFor` aggiunge dal core e che lo slot non nomina
 * (`Action.Move` per un percorso). Funzione pura: le viste si costruiscono a mano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanWarningChipTest,
	"RefactorTactics.HudViewModel.PlanWarningBelongsToOneChip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanWarningChipTest::RunTest(const FString&)
{
	FRTUnitSlotsView Slots;
	Slots.bAuthorized = true;
	Slots.Main.bOccupied = true;
	Slots.Main.ActionId = TEXT("Hero.Aevik.Overload");
	Slots.Reaction.bOccupied = true;
	Slots.Reaction.ActionId = TEXT("Action.Counter");
	Slots.Movement.bOccupied = true; // un percorso: occupato, senza un'azione

	const auto Avviso = [](const TCHAR* Id)
	{
		FRTPlanWarningView W;
		W.SourceActionId = Id ? FName(Id) : NAME_None;
		return W;
	};
	TestTrue(TEXT("l'azione della principale va alla principale"),
		URTHudViewModel::ChipForWarning(Slots, Avviso(TEXT("Hero.Aevik.Overload"))) == ERTPlanChip::Main);
	TestTrue(TEXT("quella della reazione alla reazione"),
		URTHudViewModel::ChipForWarning(Slots, Avviso(TEXT("Action.Counter"))) == ERTPlanChip::Reaction);
	TestTrue(TEXT("la voce di movimento del core, che lo slot non nomina, al movimento"),
		URTHudViewModel::ChipForWarning(Slots, Avviso(TEXT("Action.Move"))) == ERTPlanChip::Movement);
	TestTrue(TEXT("un avviso senza azione a nessuno"),
		URTHudViewModel::ChipForWarning(Slots, Avviso(nullptr)) == ERTPlanChip::None);

	Slots.Movement.bOccupied = false;
	TestTrue(TEXT("senza movimento, un'azione che non e' di nessuno slot a nessuno"),
		URTHudViewModel::ChipForWarning(Slots, Avviso(TEXT("Action.Move"))) == ERTPlanChip::None);

	Slots.bAuthorized = false;
	TestTrue(TEXT("una vista non autorizzata non assegna niente"),
		URTHudViewModel::ChipForWarning(Slots, Avviso(TEXT("Hero.Aevik.Overload"))) == ERTPlanChip::None);
	return true;
}


namespace
{
	/** Quante righe `Icona non risolta` ha visto la sonda: e' la misura del RUMORE, che e' il soggetto di `#3861`. */
	int32 ContaIconeNonRisolte(const FRTCardLogWatch& Sonda)
	{
		int32 N = 0;
		for (const FString& Riga : Sonda.Righe)
		{
			if (Riga.Contains(TEXT("Icona non risolta"))) { ++N; }
		}
		return N;
	}
}

/**
 * 🔴 **IL DOCK CHIAMA `SetAction` A OGNI FRAME, E LA RISOLUZIONE DELL'ICONA NON PUO' SEGUIRLO** (`#3861`).
 *
 * Il difetto, misurato sul pacchetto del 2026-10-26: **otto** righe `Icona non risolta: 'None' richiesta da
 * 'ActionSlot'` per frame, quasi tutte le righe `LogRT` di una seduta di cinque minuti. La correzione di
 * `#2963` dichiarava «una volta per cambio azione» e spostava `ResolveIcon` in `SetAction`, ma `SetAction` e' chiamata
 * dal grafo del dock **a ogni frame**: la risoluzione — e il suo log — ripartiva ogni volta.
 *
 * ⚠️ **Perche' il test di `#2963` non lo vedeva**: `ActionSlotResolvesOncePerActionChange` chiama `SetAction` UNA volta
 * e poi legge il getter venti volte. Prova che il getter non risolve, non che il dock non faccia risolvere. Questo
 * chiama `SetAction` venti volte, che e' cio' che il dock fa in un terzo di secondo.
 *
 * Le gambe: (a) venti chiamate con la stessa azione e lo stesso catalogo → UNA warning (e la prima dev'esserci: e' la
 * sonda del rilevatore); (b) l'armamento cambia a ogni chiamata e non e' un cambio d'icona; (c) un'altra azione
 * scrive la sua warning; (d) un altro catalogo la scrive di nuovo; (e) **la risoluzione resta VIVA**: una voce del
 * catalogo senza asset che riceve l'asset SUL POSTO — senza cambiare il numero di icone, ne' il puntatore del
 * catalogo — si risolve alla chiamata successiva, e non scrive un'altra warning; (f) se l'asset sparisce di nuovo,
 * la mancanza e' di nuovo una novita' e la warning si riscrive.
 *
 * ⚠️ **(e) e' la gamba che la revisione di PR #3875 ha chiesto**: una prima stesura memorizzava la risoluzione e la
 * rifaceva solo se cambiava una "entrata" (azione, catalogo, numero di icone). Un'icona sostituita nel catalogo mentre
 * si gioca — il lavoro di chi disegna il catalogo — lasciava lo slot con l'icona vecchia fino al cambio d'azione, cosa
 * che prima della correzione non succedeva. Ora si ricalcola a ogni chiamata, senza log, e si LOGGA una volta per chiave.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionSlotResolvesOnceWhenTheDockRepeatsSetActionTest,
	"RefactorTactics.ScreenHud.ActionSlotResolvesOnceWhenTheDockRepeatsSetAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionSlotResolvesOnceWhenTheDockRepeatsSetActionTest::RunTest(const FString&)
{
	URTActionSlotWidget* Slot = NewObject<URTActionSlotWidget>();
	URTIconCatalogData* Catalogo = NewObject<URTIconCatalogData>();
	if (!TestNotNull(TEXT("slot"), Slot) || !TestNotNull(TEXT("catalogo"), Catalogo)) { return false; }
	Catalogo->MissingIcon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Prova/T_Missing.T_Missing")));

	// Un'abilita' PROPRIA, senza ripiego: la chiave non si risolve e `ResolveIcon` logga.
	FRTAbilityCooldownView Azione;
	Azione.ActionId = TEXT("Hero.Aevik.Overload");

	FRTCardLogWatch Sonda;

	// (a) La prima chiamata DEVE scrivere la warning: e' il controllo positivo del rilevatore. Senza, «una sola»
	// sarebbe soddisfatto anche da uno slot che non risolve mai.
	Slot->SetAction(Azione, /*bArmed=*/ false, Catalogo);
	TestEqual(TEXT("(a) la prima chiamata risolve e la sonda la vede"), ContaIconeNonRisolte(Sonda), 1);

	// Venti chiamate in tutto, come il dock in un terzo di secondo. (b) L'armamento alterna: e' un dato che cambia
	// a ogni click, e NON cambia l'icona.
	for (int32 i = 0; i < 19; ++i)
	{
		Slot->SetAction(Azione, /*bArmed=*/ (i % 2 == 0), Catalogo);
	}
	TestEqual(TEXT("(a)(b) venti chiamate con la stessa azione e lo stesso catalogo: una sola warning"),
		ContaIconeNonRisolte(Sonda), 1);

	// (c) Un'altra azione: un'altra chiave, e si risolve di nuovo.
	FRTAbilityCooldownView Altra;
	Altra.ActionId = TEXT("Hero.Aevik.Altra");
	Slot->SetAction(Altra, /*bArmed=*/ false, Catalogo);
	TestEqual(TEXT("(c) un'altra azione risolve di nuovo"), ContaIconeNonRisolte(Sonda), 2);

	// (d) Un altro catalogo, stessa azione.
	URTIconCatalogData* Altro = NewObject<URTIconCatalogData>();
	if (!TestNotNull(TEXT("secondo catalogo"), Altro)) { return false; }
	Altro->MissingIcon = Catalogo->MissingIcon;
	Slot->SetAction(Altra, /*bArmed=*/ false, Altro);
	TestEqual(TEXT("(d) un altro catalogo risolve di nuovo"), ContaIconeNonRisolte(Sonda), 3);

	// (e) Lo STESSO catalogo, con la voce della chiave che lo slot chiede ma SENZA asset (per il catalogo e' non
	// risolta: `ResolveIcon` salta le voci con asset nullo): la warning c'e' gia' stata (d), e non se ne scrive un'altra.
	const FName ChiaveAltra = URTIconLibrary::MakeActionIconId(Altra.ActionId);
	const int32 Voce = Altro->Icons.Add(FRTIconDef(ChiaveAltra, ERTIconCategory::Action, TSoftObjectPtr<UTexture2D>()));
	Slot->SetAction(Altra, /*bArmed=*/ false, Altro);
	TestFalse(TEXT("(e) la voce senza asset non risolve"), Slot->GetResolvedIcon().bResolved);
	TestEqual(TEXT("(e) e non scrive un'altra warning: e' la stessa chiave mancante"), ContaIconeNonRisolte(Sonda), 3);

	// ... poi l'asset viene assegnato SUL POSTO: stesso catalogo, stesso numero di icone. Lo slot deve accorgersene.
	Altro->Icons[Voce].Asset = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Prova/T_Altra.T_Altra")));
	Slot->SetAction(Altra, /*bArmed=*/ false, Altro);
	const FRTIconResolution Dopo = Slot->GetResolvedIcon();
	TestTrue(TEXT("(e) assegnato l'asset sul posto: ora e' risolta"), Dopo.bResolved);
	TestEqual(TEXT("(e) ed e' l'asset assegnato, non il missing-icon"), Dopo.Asset.ToString(),
		FString(TEXT("/Game/Prova/T_Altra.T_Altra")));
	TestEqual(TEXT("(e) senza un'altra warning"), ContaIconeNonRisolte(Sonda), 3);

	// (f) Se l'asset sparisce di nuovo — stessa chiave, stesso catalogo — la mancanza e' di nuovo una NOVITA': la warning
	// si riscrive. Senza questa gamba, un contatore che non si azzera mai quando la chiave si risolve passerebbe.
	Altro->Icons[Voce].Asset = TSoftObjectPtr<UTexture2D>();
	Slot->SetAction(Altra, /*bArmed=*/ false, Altro);
	TestFalse(TEXT("(f) senza asset non e' piu' risolta"), Slot->GetResolvedIcon().bResolved);
	TestEqual(TEXT("(f) la chiave torna mancante: la warning si riscrive"), ContaIconeNonRisolte(Sonda), 4);

	return true;
}

/**
 * 🔴 **UNO SLOT SENZA AZIONE NON CHIEDE UN'ICONA, E NON LO SCRIVE NEL LOG** (`#3861`).
 *
 * Gli otto avvisi per frame del pacchetto hanno la chiave `None`: sono gli slot che non portano un'azione. `GetIconId`
 * lo sa — *«un'azione senza `ActionId` non ha icona, e restituire `None` e' meglio di comporre `UI.Icon.` a vuoto: la
 * risoluzione direbbe "chiave sconosciuta"»* — ma poi `ResolveIcon` veniva chiamata lo stesso con `None`, e diceva
 * proprio quella cosa, nominando la chiave `None`.
 *
 * ⛔ **Non cambia cio' che si vede**: lo slot vuoto mostra il missing-icon, come prima. Il test lo asserisce, perche'
 * togliere l'avviso azzerando anche l'icona sarebbe un altro difetto, piu' visibile.
 *
 * Il controllo positivo: la stessa sonda DEVE vedere la warning di un'abilita' propria non risolta, poco dopo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionSlotWithoutAnActionDoesNotWarnTest,
	"RefactorTactics.ScreenHud.ActionSlotWithoutAnActionDoesNotWarn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionSlotWithoutAnActionDoesNotWarnTest::RunTest(const FString&)
{
	URTActionSlotWidget* Slot = NewObject<URTActionSlotWidget>();
	URTIconCatalogData* Catalogo = NewObject<URTIconCatalogData>();
	if (!TestNotNull(TEXT("slot"), Slot) || !TestNotNull(TEXT("catalogo"), Catalogo)) { return false; }
	Catalogo->MissingIcon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Prova/T_Missing.T_Missing")));

	FRTCardLogWatch Sonda;

	const FRTAbilityCooldownView Vuota;   // nessun `ActionId`: uno slot che non porta un'azione
	for (int32 i = 0; i < 20; ++i)
	{
		Slot->SetAction(Vuota, /*bArmed=*/ false, Catalogo);
	}
	TestEqual(TEXT("uno slot senza azione non scrive avvisi"), ContaIconeNonRisolte(Sonda), 0);

	const FRTIconResolution Esito = Slot->GetResolvedIcon();
	TestFalse(TEXT("non e' un'icona risolta"), Esito.bResolved);
	TestEqual(TEXT("ma si vede il missing-icon, come prima: l'avviso se ne va, l'icona no"),
		Esito.Asset.ToString(), FString(TEXT("/Game/Prova/T_Missing.T_Missing")));

	// Senza catalogo: nessuna icona da mostrare, e nessun avviso nemmeno qui.
	Slot->SetAction(Vuota, /*bArmed=*/ false, /*Catalog=*/ nullptr);
	TestEqual(TEXT("senza catalogo, senza azione: ancora nessun avviso"), ContaIconeNonRisolte(Sonda), 0);
	TestTrue(TEXT("e nessun asset da mostrare"), Slot->GetResolvedIcon().Asset.IsNull());

	// Controllo positivo: la sonda vede un'abilita' propria non risolta.
	FRTAbilityCooldownView Propria;
	Propria.ActionId = TEXT("Hero.Aevik.Overload");
	Slot->SetAction(Propria, /*bArmed=*/ false, Catalogo);
	TestEqual(TEXT("controllo positivo: una chiave vera non risolta scrive un avviso"), ContaIconeNonRisolte(Sonda), 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
