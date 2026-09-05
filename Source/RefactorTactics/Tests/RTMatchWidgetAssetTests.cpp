// CP 11.7 (#613) — i binding dichiarati DENTRO i `WBP_RT_*` di **Match**.
//
// ⚠️ **Gemello di `RTFrontendWidgetAssetTests.cpp`, per la meta' che quello non copre.** Quel file prova
// tre widget del Framework (`ErrorModal`, `FallbackBanner`, `LoadingScreen`) ed esiste per un difetto
// misurato: il modale si armava e non compariva, e fra le due cose c'era un `FDelegateRuntimeBinding`
// serializzato nel binario. I **sette** widget di `Content/RT/UI/Match/` non avevano nulla di equivalente:
// stessa classe di difetto, stessi gesti d'editor, meta' del perimetro scoperta.
//
// 🔴 **La domanda che questi test pongono e' una sola**: una funzione che il C++ espone al Blueprint e che
// **nessun binding consuma** e' un dato dichiarato, trasportato e mai letto — cioe' una regola che non
// arriva a schermo. Il C++ puo' essere verde e la partita illeggibile.
//
// ⚠️ Cio' che NON coprono: l'aspetto. Colori, font, posizione e leggibilita' restano di `PIE-V01-HUD`. Un
// binding corretto su un widget largo zero pixel passerebbe di qui — per questo il report stampa anche la
// geometria, come fa il gemello.

#include "Misc/AutomationTest.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelSlot.h"
#include "Components/Widget.h"
#include "UI/RTScreenHudWidgets.h"
#include "Engine/Texture2D.h" // UTexture2D: il tipo che il gate rifiuta, esplicito e non ereditato
#include "Tests/RTWidgetAssetTestHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const TCHAR* const TacticalHudPath = TEXT("/Game/RT/UI/Match/WBP_RT_TacticalHUD.WBP_RT_TacticalHUD_C");
	const TCHAR* const TurnHeaderPath = TEXT("/Game/RT/UI/Match/WBP_RT_TurnHeader.WBP_RT_TurnHeader_C");
	const TCHAR* const TeamRosterPath = TEXT("/Game/RT/UI/Match/WBP_RT_TeamRoster.WBP_RT_TeamRoster_C");
	const TCHAR* const SelectedUnitPath = TEXT("/Game/RT/UI/Match/WBP_RT_SelectedUnitPanel.WBP_RT_SelectedUnitPanel_C");
	const TCHAR* const ActionDockPath = TEXT("/Game/RT/UI/Match/WBP_RT_ActionDock.WBP_RT_ActionDock_C");
	const TCHAR* const ActionSlotPath = TEXT("/Game/RT/UI/Match/WBP_RT_ActionSlot.WBP_RT_ActionSlot_C");
	const TCHAR* const UnitCardPath = TEXT("/Game/RT/UI/Match/WBP_RT_UnitCard.WBP_RT_UnitCard_C");

	/**
	 * Vero se la proprieta' e' — o contiene — una `UTexture2D`.
	 *
	 * ⚠️ **Il nome non e' `HudWidgetCarriesTexture` di proposito, ed e' una precauzione di build.** Quella
	 * gemella vive nel namespace anonimo di `RTScreenHudWidgetTests.cpp`: sotto **unity build** i due file
	 * finiscono nella stessa unita' di traduzione e i due namespace anonimi si fondono, quindi due funzioni
	 * omonime sono una ridefinizione — un errore che compare a chi aggiunge il file DOPO, non a chi lo
	 * scrive. Duplicare quattro righe costa meno che esportare un helper di test in un header condiviso.
	 */
	bool BlueprintPropertyCarriesTexture(const FProperty* Prop)
	{
		if (!Prop)
		{
			return false;
		}
		// `FObjectPropertyBase` e non `FObjectProperty`: copre anche `TSoftObjectPtr<UTexture2D>`, che nel
		// designer si sceglie con lo stesso menu e ha lo stesso effetto sulla regola.
		if (const FObjectPropertyBase* AsObject = CastField<FObjectPropertyBase>(Prop))
		{
			return AsObject->PropertyClass
				&& AsObject->PropertyClass->IsChildOf(UTexture2D::StaticClass());
		}
		if (const FArrayProperty* AsArray = CastField<FArrayProperty>(Prop))
		{
			return BlueprintPropertyCarriesTexture(AsArray->Inner);
		}
		if (const FSetProperty* AsSet = CastField<FSetProperty>(Prop))
		{
			return BlueprintPropertyCarriesTexture(AsSet->ElementProp);
		}
		if (const FMapProperty* AsMap = CastField<FMapProperty>(Prop))
		{
			return BlueprintPropertyCarriesTexture(AsMap->KeyProp)
				|| BlueprintPropertyCarriesTexture(AsMap->ValueProp);
		}
		return false;
	}

	/** I nomi delle funzioni consumate dai binding dell'asset. */
	TSet<FName> BoundFunctionNames(const UWidgetBlueprintGeneratedClass* Class)
	{
		TSet<FName> Names;
		for (const FDelegateRuntimeBinding& Binding : Class->Bindings)
		{
			Names.Add(Binding.FunctionName);
		}
		return Names;
	}

	FString DescribeWidgetSlotOffsets(const UWidget* Widget)
	{
		if (!Widget)
		{
			return TEXT("  <nullptr>");
		}

		FString Line = FString::Printf(TEXT("  %-22s %-24s Visibility=%s"),
			*Widget->GetName(),
			*Widget->GetClass()->GetName(),
			*StaticEnum<ESlateVisibility>()->GetNameStringByValue(static_cast<int64>(Widget->GetVisibility())));

		if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
		{
			const FAnchorData Layout = CanvasSlot->GetLayout();
			Line += FString::Printf(TEXT("  Offsets=(L%.0f T%.0f R%.0f B%.0f)"),
				Layout.Offsets.Left, Layout.Offsets.Top, Layout.Offsets.Right, Layout.Offsets.Bottom);
		}

		return Line;
	}

	/**
	 * Versa nel log binding, albero e — la parte che conta — le `BlueprintPure` della **classe base C++**
	 * che nessun binding consuma.
	 *
	 * L'elenco si ricava per reflection dalla base, non da una lista scritta a mano: una funzione aggiunta
	 * domani entra nel report senza che nessuno aggiorni questo file. E' la differenza fra un oracolo e un
	 * promemoria.
	 */
	void ReportBindingsAndUnconsumedPure(FAutomationTestBase& Test, const UWidgetBlueprintGeneratedClass* Class, const TCHAR* Label)
	{
		Test.AddInfo(FString::Printf(TEXT("=== %s: %d binding, base = %s ==="),
			Label, Class->Bindings.Num(),
			Class->GetSuperClass() ? *Class->GetSuperClass()->GetName() : TEXT("<nessuna>")));

		for (const FDelegateRuntimeBinding& Binding : Class->Bindings)
		{
			Test.AddInfo(RTWidgetAssetTest::DescribeBinding(Binding));
		}

		const TSet<FName> Bound = BoundFunctionNames(Class);
		TArray<FString> Unconsumed;
		for (UClass* Base = Class->GetSuperClass(); Base && Base != UUserWidget::StaticClass(); Base = Base->GetSuperClass())
		{
			for (TFieldIterator<UFunction> It(Base, EFieldIteratorFlags::ExcludeSuper); It; ++It)
			{
				const UFunction* Fn = *It;
				// Solo cio' che e' DESTINATO a un binding: puro, senza parametri d'ingresso, con un valore.
				const bool bPure = Fn->HasAnyFunctionFlags(FUNC_BlueprintPure);
				const bool bHasReturn = Fn->GetReturnProperty() != nullptr;
				if (bPure && bHasReturn && !Bound.Contains(Fn->GetFName()))
				{
					Unconsumed.Add(Fn->GetName());
				}
			}
		}

		if (Unconsumed.Num() > 0)
		{
			Test.AddInfo(FString::Printf(TEXT("  ⚠️ BlueprintPure della base NON consumate: %s"),
				*FString::Join(Unconsumed, TEXT(", "))));
		}
		else
		{
			Test.AddInfo(TEXT("  ✅ ogni BlueprintPure della base e' consumata da un binding"));
		}

		if (const UWidgetTree* Tree = Class->GetWidgetTreeArchetype())
		{
			Test.AddInfo(FString::Printf(TEXT("=== %s: albero (radice = %s) ==="),
				Label, Tree->RootWidget ? *Tree->RootWidget->GetName() : TEXT("<nessuna>")));
			Tree->ForEachWidget([&Test](UWidget* Widget)
			{
				Test.AddInfo(DescribeWidgetSlotOffsets(Widget));
			});
		}
	}

	/** Carica e riporta. `nullptr` se l'asset non c'e': il chiamante lo dichiara fallimento col path. */
	UWidgetBlueprintGeneratedClass* LoadAndReport(FAutomationTestBase& Test, const TCHAR* Path, const TCHAR* Label)
	{
		UWidgetBlueprintGeneratedClass* Class = RTWidgetAssetTest::LoadWidgetClass(Path);
		if (!Class)
		{
			Test.AddError(FString::Printf(TEXT("%s: l'asset non si carica — %s"), Label, Path));
			return nullptr;
		}
		ReportBindingsAndUnconsumedPure(Test, Class, Label);
		return Class;
	}
}

/**
 * I sette widget di Match esistono e si caricano.
 *
 * E' la verifica piu' debole del file ed e' deliberato che sia separata: se un asset viene rinominato o
 * spostato, questo test dice **quale**, mentre gli altri direbbero soltanto che un binding manca.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchWidgetsLoadTest,
	"RefactorTactics.ScreenHud.MatchWidgetsLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTMatchWidgetsLoadTest::RunTest(const FString&)
{
	const TCHAR* const Paths[] = {
		TacticalHudPath, TurnHeaderPath, TeamRosterPath, SelectedUnitPath,
		ActionDockPath, ActionSlotPath, UnitCardPath
	};
	const TCHAR* const Labels[] = {
		TEXT("TacticalHUD"), TEXT("TurnHeader"), TEXT("TeamRoster"), TEXT("SelectedUnitPanel"),
		TEXT("ActionDock"), TEXT("ActionSlot"), TEXT("UnitCard")
	};

	for (int32 i = 0; i < UE_ARRAY_COUNT(Paths); ++i)
	{
		LoadAndReport(*this, Paths[i], Labels[i]);
	}
	return true;
}

// ⛔ **QUI C'ERA UN TEST SBAGLIATO, e la sua storia vale piu' del test.**
//
// `ActionDockConsumesArmedIndex` asseriva che `WBP_RT_ActionDock` avesse un `FDelegateRuntimeBinding` su
// `GetActions()` e `GetArmedActionIndex()`. Falliva su **entrambe**, e sembrava aver trovato un difetto
// grosso. Non ne aveva trovato nessuno: cercava la cosa sbagliata.
//
// Lo Step 7.3 del piano di #613 prescrive che il dock consumi quelle funzioni **su `Event Construct`** —
// `GetActions()` → `ForEachLoop` → `SetAction(Element, Index == GetArmedActionIndex())`. Un consumo dentro
// il grafo **non produce un property binding**, quindi `Class->Bindings` non lo vedra' mai. L'albero del
// dock e' infatti un solo widget (`SlotBox`), esattamente come lo Step 7.2 lo descrive: e' un contenitore
// che si popola a runtime, non un pannello di campi legati.
//
// 🔴 **Cosa insegna, ed e' il motivo per cui la nota resta**: l'asserzione era caduta perche' il mio
// oracolo misurava UN meccanismo di consumo (il property binding) e lo trattava come se fosse l'unico. Un
// test che fallisce non prova un difetto — prova che l'oracolo e il codice non sono d'accordo, e prima di
// accusare il codice va escluso l'oracolo. Il controllo di sanita' su `GetActions` e' cio' che l'ha
// rivelato: se anche il fratello «che deve esserci» manca, il sospetto giusto e' sul metodo.
//
// ⚠️ Lo stato armato **resta non verificato da qui**, e non e' un buco che si chiude con un property
// binding. Serve istanziare il dock e osservare che con `INDEX_NONE` nessuno slot risulti acceso — un test
// di comportamento, con un mondo, oppure la voce PIE. `PIE-V01-HUD` e' la sede.

/**
 * 🔴 **Lo slot ha la superficie su cui l'icona atterra.**
 *
 * ✅ **Questo difetto e' reale, e lo dichiara il piano stesso.** Lo Step 6.2 di #613 prescrive per
 * `WBP_RT_ActionSlot` un *«`Overlay` con: un `Image` (`IconImage`), un `Text Block` per il cooldown
 * (`CooldownText`), e un `Border` per lo stato armato (`ArmedBorder`)»*. L'albero costruito contiene
 * `ArmedBorder`, `NomeText` e `CooldownText`: **`IconImage` non e' mai stato aggiunto.**
 *
 * ⚠️ **Il test chiede la SUPERFICIE, non il binding** — ed e' per questo che sopravvive all'errore
 * dell'altro: una `UImage` nell'albero e' un fatto strutturale, non un'inferenza su come il valore ci
 * arriva. Finche' il catalogo di #220 non esiste, `ResolveIcon` restituisce il missing-icon con
 * `bResolved = false`, e il comportamento voluto e' che **a schermo si veda che manca**: senza una
 * `UImage` non si vede nemmeno quello.
 *
 * `RefactorTactics.ScreenHud.WidgetApiExposesNoTexture` pinna la superficie C++ via reflection e **non
 * vede i Blueprint**: questa meta' non era coperta da nulla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionSlotHasIconSurfaceTest,
	"RefactorTactics.ScreenHud.ActionSlotHasIconSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTActionSlotHasIconSurfaceTest::RunTest(const FString&)
{
	UWidgetBlueprintGeneratedClass* Class = LoadAndReport(*this, ActionSlotPath, TEXT("ActionSlot"));
	if (!Class)
	{
		return false;
	}

	const UWidgetTree* Tree = Class->GetWidgetTreeArchetype();
	if (!Tree)
	{
		AddError(TEXT("WBP_RT_ActionSlot non ha un albero di widget"));
		return false;
	}

	int32 ImageCount = 0;
	TArray<FString> Nomi;
	Tree->ForEachWidget([&ImageCount, &Nomi](UWidget* Widget)
	{
		if (Widget)
		{
			Nomi.Add(Widget->GetName());
			if (Cast<UImage>(Widget))
			{
				++ImageCount;
			}
		}
	});

	// L'albero per intero nel messaggio: senza, il fallimento dice «manca una UImage» e chi apre l'editor
	// non sa se il widget e' stato dimenticato o solo chiamato in un altro modo.
	AddInfo(FString::Printf(TEXT("ActionSlot contiene: %s"), *FString::Join(Nomi, TEXT(", "))));

	TestTrue(
		FString::Printf(TEXT("WBP_RT_ActionSlot ha una UImage per l'icona (Step 6.2 la chiama `IconImage`) — contiene invece: %s"),
			*FString::Join(Nomi, TEXT(", "))),
		ImageCount > 0);

	// I due che il piano nomina insieme all'icona: se ci sono, l'assenza del terzo e' una dimenticanza
	// isolata e non un asset costruito su un altro disegno.
	TestTrue(TEXT("WBP_RT_ActionSlot ha `CooldownText` (Step 6.2)"), Nomi.Contains(TEXT("CooldownText")));
	TestTrue(TEXT("WBP_RT_ActionSlot ha `ArmedBorder` (Step 6.2)"), Nomi.Contains(TEXT("ArmedBorder")));

	return true;
}

/**
 * Ogni widget di Match che ha una classe base RT la dichiara davvero.
 *
 * ⚠️ **`UnitCard` e' escluso di proposito**: non esiste una `URTUnitCardWidget` in `Source/`, quindi il
 * Blueprint deriva da `UUserWidget` ed e' una scelta, non un difetto. Il test lo dice invece di lasciarlo
 * dedurre da un'assenza — chi aggiungesse quella classe domani troverebbe qui la riga da cambiare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchWidgetsDeriveFromCppBaseTest,
	"RefactorTactics.ScreenHud.MatchWidgetsDeriveFromCppBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTMatchWidgetsDeriveFromCppBaseTest::RunTest(const FString&)
{
	struct FExpected
	{
		const TCHAR* Path;
		const TCHAR* Label;
		UClass* Base;
	};

	const FExpected Expected[] = {
		{ TacticalHudPath,  TEXT("TacticalHUD"),       URTTacticalHUDWidget::StaticClass() },
		{ TurnHeaderPath,   TEXT("TurnHeader"),        URTTurnHeaderWidget::StaticClass() },
		{ TeamRosterPath,   TEXT("TeamRoster"),        URTTeamRosterWidget::StaticClass() },
		{ SelectedUnitPath, TEXT("SelectedUnitPanel"), URTSelectedUnitPanelWidget::StaticClass() },
		{ ActionDockPath,   TEXT("ActionDock"),        URTActionDockWidget::StaticClass() },
		{ ActionSlotPath,   TEXT("ActionSlot"),        URTActionSlotWidget::StaticClass() },
	};

	for (const FExpected& E : Expected)
	{
		UWidgetBlueprintGeneratedClass* Class = RTWidgetAssetTest::LoadWidgetClass(E.Path);
		if (!Class)
		{
			AddError(FString::Printf(TEXT("%s: l'asset non si carica — %s"), E.Label, E.Path));
			continue;
		}

		TestTrue(
			FString::Printf(TEXT("%s deriva da %s"), E.Label, *E.Base->GetName()),
			Class->IsChildOf(E.Base));
	}

	return true;
}

/**
 * 🔴 **La meta' Blueprint della regola D-031, che nessun gate vedeva.**
 *
 * `RefactorTactics.ScreenHud.WidgetApiExposesNoTexture` itera per reflection le proprieta' dichiarate dalle
 * **classi C++** dei widget e fallisce se una e' — o contiene — una `UTexture2D`. Il corpo di `#220` lo
 * dichiara da se':
 *
 * > 🔴 **Non vede i Blueprint.** Una variabile `Texture2D` aggiunta dentro un `WBP_RT_*` — la scorciatoia
 * > «per comodita'» che D-031 vieta e che il runbook §4.1 nomina — passerebbe questo gate.
 *
 * ⚠️ **E la scorciatoia non e' teorica**: e' esattamente il gesto che un autore fa quando l'icona «non si
 * vede» e vuole chiudere la questione in trenta secondi. Il costo non e' il pixel — e' che da quel momento
 * il Blueprint decide quale file grafico rappresenta un'azione, e il catalogo smette di essere l'unica
 * fonte. Una regola che vale solo dove il codice guarda e' una raccomandazione.
 *
 * Qui si guarda l'altra meta': le proprieta' **dichiarate dalla `UWidgetBlueprintGeneratedClass`**, cioe' le
 * variabili aggiunte dentro il `.uasset`. L'infrastruttura c'era gia' tutta in questo file — le sette classi
 * si caricano per `PIE-ICON-01` e per `ActionSlotHasIconSurface` — e mancava soltanto l'asserzione.
 *
 * ⛔ **Cosa NON copre, e va detto perche' il verde non prometta piu' di quanto misura:**
 *
 *  1. una texture nascosta **dentro una struct** — `FSlateBrush::ResourceObject` e' un `UObject*`, non un
 *     `UTexture2D*`, quindi un `Brush` impostato a mano nel designer passa di qui. Il difetto che questo
 *     test chiude e' la **variabile** dichiarata, non il valore di default di un widget dell'albero;
 *  2. i widget del **Framework** (`WBP_RT_ErrorModal` e compagni): li presidia
 *     `RTFrontendWidgetAssetTests.cpp`, e questa asserzione non ci e' stata estesa perche' D-031 parla
 *     dell'iconografia di partita;
 *  3. che l'icona **si veda**. Resta `PIE-ICON-01`, e nessun test la sostituisce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchWidgetsDeclareNoTextureTest,
	"RefactorTactics.ScreenHud.BlueprintPropertiesExposeNoTexture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTMatchWidgetsDeclareNoTextureTest::RunTest(const FString&)
{
	const TCHAR* const Paths[] = {
		TacticalHudPath, TurnHeaderPath, TeamRosterPath, SelectedUnitPath,
		ActionDockPath, ActionSlotPath, UnitCardPath
	};
	const TCHAR* const Labels[] = {
		TEXT("TacticalHUD"), TEXT("TurnHeader"), TEXT("TeamRoster"), TEXT("SelectedUnitPanel"),
		TEXT("ActionDock"), TEXT("ActionSlot"), TEXT("UnitCard")
	};

	int32 Caricate = 0;
	int32 Ispezionate = 0;

	for (int32 i = 0; i < UE_ARRAY_COUNT(Paths); ++i)
	{
		UWidgetBlueprintGeneratedClass* Class = LoadWidgetClass(Paths[i]);
		if (!Class)
		{
			AddError(FString::Printf(TEXT("%s: l'asset non si carica — %s"), Labels[i], Paths[i]));
			continue;
		}
		++Caricate;

		// `EFieldIterationFlags::None` e non `IncludeSuper`: la base C++ e' gia' presidiata da
		// `WidgetApiExposesNoTexture`, e includerla qui farebbe fallire **due** test per lo stesso difetto
		// senza aggiungere copertura. Qui interessa solo cio' che il **Blueprint** dichiara.
		for (TFieldIterator<FProperty> It(Class, EFieldIterationFlags::None); It; ++It)
		{
			++Ispezionate;
			if (BlueprintPropertyCarriesTexture(*It))
			{
				AddError(FString::Printf(
					TEXT("%s dichiara `%s`, che e' — o contiene — una `UTexture2D`: nel Blueprint l'icona ")
					TEXT("deve restare una CHIAVE (`UI.Icon.*`) risolta dal catalogo (D-031, #220). Una ")
					TEXT("variabile texture nel `WBP_` sposta nel `.uasset` la decisione su quale file ")
					TEXT("grafico rappresenta l'azione."),
					Labels[i], *It->GetName()));
			}
		}
	}

	// Le due controprove della premessa. Senza, questo test sarebbe verde anche se gli asset non si
	// caricassero affatto o se l'iterazione non guardasse niente — cioe' passerebbe **misurando zero**, che
	// e' il modo in cui un gate diventa decorativo.
	TestEqual(TEXT("i sette widget di Match si caricano"), Caricate, static_cast<int32>(UE_ARRAY_COUNT(Paths)));
	TestTrue(
		FString::Printf(TEXT("l'iterazione ha guardato delle proprieta' dichiarate dai Blueprint (ne ha viste %d)"),
			Ispezionate),
		Ispezionate > 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
