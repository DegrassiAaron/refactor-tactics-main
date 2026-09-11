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
#include "Components/CanvasPanel.h"
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
	// CP 14.6 (`#166`): la finestra di reazione. Il path entra QUI e non prima — questo file carica per
	// path, e un path senza asset e' un test rosso che aspetta un file che nessuno ha creato.
	const TCHAR* const FastDecisionPath = TEXT("/Game/RT/UI/Match/WBP_RT_FastDecision.WBP_RT_FastDecision_C");
	// Il bottone di UNA risposta. Entra col proprio asset, come il fratello qui sopra.
	const TCHAR* const FastDecisionOptionPath =
		TEXT("/Game/RT/UI/Match/WBP_RT_FastDecisionOption.WBP_RT_FastDecisionOption_C");
	// 🔴 **Il feed e la sua riga entrano qui, e in ritardo** (`#2697`): l'asset esiste dal 2026-09-09 e
	// fino al 2026-09-10 era l'unico `WBP_RT_*` di Match che nessun gate di questo file guardava — proprio
	// quello che #2697 stava montando. La regola scritta sopra per `FastDecision` — *il path entra QUI
	// quando l'asset c'e'* — valeva gia' e non era stata applicata.
	const TCHAR* const EventLogPath = TEXT("/Game/RT/UI/Match/WBP_RT_EventLog.WBP_RT_EventLog_C");
	// ⚠️ **`WBP_RT_EventLine` NON entra in `Expected[]`**, e non e' una svista: deriva da `UUserWidget`
	// nudo — misurato leggendo il pacchetto, che non nomina nessuna classe di `RefactorTactics`. E' un
	// contenitore passivo che il feed riempie, quindi il gate delle classi base non ha niente da chiedergli;
	// il caricamento e il divieto di texture, si'.
	const TCHAR* const EventLinePath = TEXT("/Game/RT/UI/Match/WBP_RT_EventLine.WBP_RT_EventLine_C");

	/**
	 * Vero se la proprieta' e' — o contiene — una `UTexture2D`.
	 *
	 * ⚠️ **Il nome non e' `HudWidgetCarriesTexture` di proposito, ed e' una precauzione di build.** Quella
	 * gemella vive nel namespace anonimo di `RTScreenHudWidgetTests.cpp`: sotto **unity build** i due file
	 * finiscono nella stessa unita' di traduzione e i due namespace anonimi si fondono, quindi due funzioni
	 * omonime sono una ridefinizione — un errore che compare a chi aggiunge il file DOPO, non a chi lo
	 * scrive. ⌨ ~~Duplicare quattro righe costa meno che esportare un helper di test in un header
	 * condiviso.~~ 🔴 **Superata da #2423, e questo file ora fa il contrario venti righe piu' su**:
	 * include `Tests/RTWidgetAssetTestHelpers.h`. La regola corretta non e' «duplicare» ne' «condividere»,
	 * e' **guardare i corpi**: identici → header condiviso con namespace nominato; diversi → rinominare,
	 * che e' il divieto di #2397. Le quattro omonime di questi due file erano due per tipo.
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
		ActionDockPath, ActionSlotPath, UnitCardPath, FastDecisionPath, FastDecisionOptionPath,
		EventLogPath, EventLinePath
	};
	const TCHAR* const Labels[] = {
		TEXT("TacticalHUD"), TEXT("TurnHeader"), TEXT("TeamRoster"), TEXT("SelectedUnitPanel"),
		TEXT("ActionDock"), TEXT("ActionSlot"), TEXT("UnitCard"), TEXT("FastDecision"),
		TEXT("FastDecisionOption"), TEXT("EventLog"), TEXT("EventLine")
	};
	static_assert(UE_ARRAY_COUNT(Paths) == UE_ARRAY_COUNT(Labels),
		"path ed etichette vanno a coppie: un'etichetta in meno sposta i nomi di tutti i successivi");

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
		{ FastDecisionPath, TEXT("FastDecision"),      URTFastDecisionWidget::StaticClass() },
		{ FastDecisionOptionPath, TEXT("FastDecisionOption"),
		                                               URTFastDecisionOptionWidget::StaticClass() },
		{ EventLogPath,     TEXT("EventLog"),          URTPlayerEventLogWidget::StaticClass() },
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
		ActionDockPath, ActionSlotPath, UnitCardPath, FastDecisionPath, FastDecisionOptionPath,
		EventLogPath, EventLinePath
	};
	const TCHAR* const Labels[] = {
		TEXT("TacticalHUD"), TEXT("TurnHeader"), TEXT("TeamRoster"), TEXT("SelectedUnitPanel"),
		TEXT("ActionDock"), TEXT("ActionSlot"), TEXT("UnitCard"), TEXT("FastDecision"),
		TEXT("FastDecisionOption"), TEXT("EventLog"), TEXT("EventLine")
	};
	static_assert(UE_ARRAY_COUNT(Paths) == UE_ARRAY_COUNT(Labels),
		"path ed etichette vanno a coppie: un'etichetta in meno sposta i nomi di tutti i successivi");

	int32 Caricate = 0;
	int32 Ispezionate = 0;

	for (int32 i = 0; i < UE_ARRAY_COUNT(Paths); ++i)
	{
		UWidgetBlueprintGeneratedClass* Class = RTWidgetAssetTest::LoadWidgetClass(Paths[i]);
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

/**
 * 🔴 **I TRE BINDING DELLA FINESTRA SONO COLLEGATI, E AI NODI GIUSTI** (CP 14.6, `#166`, voce 3).
 *
 * 🔑 **Esiste perche' il cablaggio e' lavoro a mano, e il difetto che teme e' SILENZIOSO.** I tre vestiti
 * (`GetWindowVisibility` · `GetCountdownText` · `GetPromptText`) vivono in C++ e sono coperti da test; ma
 * *quale* funzione finisce su *quale* proprieta' lo decide chi apre il Designer, in tre menu a tendina. Un
 * `Visibility` collegato al countdown invece che all'apertura e' **esattamente il difetto `F7`** — il
 * prompt sparisce mentre il gioco aspetta ancora, e la risposta mancata diventa un `HoldTimeout`
 * indistinguibile da una scelta. A schermo si vede come «a volte il bottone non c'e'»; nella suite, senza
 * questo test, non si vede affatto.
 *
 * ⚠️ **L'oracolo e' `Class->Bindings`, ed e' quello GIUSTO qui — a differenza del caso raccontato piu'
 * su.** `ActionDockConsumesArmedIndex` cercava property binding dove il dock usa il **grafo**, e sbagliava
 * meccanismo. Qui il meccanismo prescritto **e'** il property binding (`guida-screen-hud-umg.md` §4.3 e
 * §7-bis), quindi `FDelegateRuntimeBinding` e' la sede in cui la risposta deve comparire. La lezione di
 * quel caso resta applicata: prima di accusare l'asset, si controlla che l'oracolo misuri il meccanismo.
 *
 * ⛔ **Fallisce finche' i binding non ci sono, ed e' voluto**: e' il segnale di «fatto» per chi apre il
 * Designer. Per questo atterra INSIEME all'asset cablato, non prima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFastDecisionBindingsWiredTest,
	"RefactorTactics.ScreenHud.FastDecisionBindingsAreWiredToTheRightNodes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTFastDecisionBindingsWiredTest::RunTest(const FString&)
{
	UWidgetBlueprintGeneratedClass* Class = RTWidgetAssetTest::LoadWidgetClass(FastDecisionPath);
	if (!TestNotNull(TEXT("WBP_RT_FastDecision si carica"), Class))
	{
		return false;
	}

	struct FAtteso
	{
		const TCHAR* Widget;    // `ObjectName` del binding: il widget nell'albero
		const TCHAR* Proprieta; // la proprieta' legata
		const TCHAR* Funzione;  // il vestito C++ che la alimenta
		const TCHAR* Perche;
	};

	const FAtteso Attesi[] = {
		{ TEXT("WindowRoot"), TEXT("Visibility"), TEXT("GetWindowVisibility"),
		  TEXT("la visibilita' segue l'APERTURA; legata al countdown sarebbe il difetto F7") },
		{ TEXT("CountdownText"), TEXT("Text"), TEXT("GetCountdownText"),
		  TEXT("il countdown viene dall'orologio autorevole, non da un contatore del widget") },
		{ TEXT("PromptText"), TEXT("Text"), TEXT("GetPromptText"),
		  TEXT("l'etichetta viene dal C++, che sa di non poter nominare il bersaglio") },
	};

	for (const FAtteso& A : Attesi)
	{
		const FDelegateRuntimeBinding* Trovato = Class->Bindings.FindByPredicate(
			[&A](const FDelegateRuntimeBinding& B)
			{
				return B.ObjectName == A.Widget && B.PropertyName == FName(A.Proprieta);
			});

		if (!Trovato)
		{
			AddError(FString::Printf(
				TEXT("`%s` non ha un binding su `%s`. Va collegato a `%s` nel Designer — %s. ")
				TEXT("La ricetta e' in `docs/technical/runbooks/guida-screen-hud-umg.md` §7-bis."),
				A.Widget, A.Proprieta, A.Funzione, A.Perche));
			continue;
		}

		// 🔑 **La meta' che conta: non «c'e' un binding», ma «e' collegato alla funzione GIUSTA».** Un
		// binding presente e sbagliato e' peggio di uno assente — a schermo sembra funzionare.
		TestEqual(
			*FString::Printf(TEXT("`%s.%s` e' alimentata da `%s` (%s)"),
				A.Widget, A.Proprieta, A.Funzione, A.Perche),
			Trovato->FunctionName, FName(A.Funzione));
	}

	// Senza questa riga il ciclo sarebbe verde anche su un asset senza un solo binding, perche' `AddError`
	// non ferma l'iterazione e `TestEqual` non viene mai raggiunto.
	TestTrue(
		*FString::Printf(TEXT("l'asset dichiara almeno i tre binding attesi (ne ha %d)"),
			Class->Bindings.Num()),
		Class->Bindings.Num() >= UE_ARRAY_COUNT(Attesi));

	return true;
}

/**
 * 🔴 **IL CENTRO RESTA LIBERO, E LO DICE UN NUMERO** (CP 11.7, `#613`, voce «centro libero» del DoD).
 *
 * 🔑 **Esiste perche' il gate che lo copriva non era falsificabile.** `guida-screen-hud-umg.md` §3 scrive
 * che il centro libero e' *«un requisito, non un gusto»*, e poi lo affida a *«si verifica a occhio in
 * `PIE-V01-HUD`»*. Un occhio non ha una soglia: due persone guardano lo stesso fotogramma e passano
 * entrambe legittimamente, e nessuna delle due si accorge di una regressione da venti pixel.
 *
 * ⚠️ **Cio' che questo test NON toglie a PIE.** Comprensione, leggibilita', proporzioni fra i pannelli e il
 * sospetto delle barre duplicate restano di `PIE-V01-SCREENHUD`: nessuna aritmetica su rettangoli le vede.
 * Qui cade soltanto la meta' geometrica — ingombro, sovrapposizione, invasione del centro — che era
 * affidata all'occhio **pur essendo calcolabile**, ed e' anche l'unica meta' che regredisce in silenzio
 * quando qualcuno trascina una zona nel Designer.
 *
 * 🔑 **La soglia e' dichiarata qui e non altrove**: il riquadro centrale e' il **60% x 60% centrato** della
 * risoluzione di riferimento 1920x1080, cioe' `X 384..1536` e `Y 216..864`. Non e' un numero sacro — e' un
 * numero **scritto**, che si discute in una issue invece che in un playtest. Se la release lo vuole
 * diverso si cambia `CenterFraction`, e il gate resta ripetibile.
 *
 * ⚠️ **L'oracolo e' l'archetipo dell'albero, non un fotogramma renderizzato**: le zone si misurano da
 * `UCanvasPanelSlot` con la stessa formula che `SConstraintCanvas::OnArrangeChildren` applica a runtime.
 * Vale percio' per il layout **dichiarato nell'asset**; un widget spostato a runtime da Blueprint sfugge
 * di qui e resta di PIE.
 */
namespace RTCenterFree
{
	/** La risoluzione a cui il DoD di `#613` chiede coerenza. Cambiarla cambia il significato del gate. */
	constexpr float RefWidth = 1920.f;
	constexpr float RefHeight = 1080.f;

	/** Il lato del riquadro centrale che nessuna zona puo' toccare, in frazione dello schermo. */
	constexpr float CenterFraction = 0.6f;

	struct FRect
	{
		float Left = 0.f;
		float Top = 0.f;
		float Right = 0.f;
		float Bottom = 0.f;

		float Width() const { return Right - Left; }
		float Height() const { return Bottom - Top; }
	};

	/** Il riquadro che deve restare sgombro, in pixel di riferimento. */
	FRect CenterKeepOut()
	{
		const float MargineX = RefWidth * (1.f - CenterFraction) * 0.5f;
		const float MargineY = RefHeight * (1.f - CenterFraction) * 0.5f;
		return FRect{ MargineX, MargineY, RefWidth - MargineX, RefHeight - MargineY };
	}

	/** Intersezione: larghezza o altezza <= 0 significa che i due riquadri non si toccano. */
	FRect Intersezione(const FRect& A, const FRect& B)
	{
		return FRect{
			FMath::Max(A.Left, B.Left),
			FMath::Max(A.Top, B.Top),
			FMath::Min(A.Right, B.Right),
			FMath::Min(A.Bottom, B.Bottom) };
	}

	bool SiToccano(const FRect& A, const FRect& B)
	{
		const FRect I = Intersezione(A, B);
		return I.Width() > 0.f && I.Height() > 0.f;
	}

	/**
	 * Il rettangolo che il Canvas assegnera' alla zona a `RefWidth x RefHeight`.
	 *
	 * ⚠️ **E' la formula di `SConstraintCanvas::OnArrangeChildren`, non una sua semplificazione**: gli
	 * offset di un `UCanvasPanelSlot` NON sono un rettangolo — cambiano significato con gli anchor. Con
	 * anchor «stiracchiati» su un asse (`Minimum != Maximum`) `Left`/`Right` sono **margini** dai bordi
	 * dell'ancora; con anchor a punto, `Right` e' la **larghezza** e l'allineamento sposta l'origine.
	 * Leggerli come un rettangolo produce un oracolo che sbaglia proprio sulle zone ancorate a destra e in
	 * basso — quelle che invadono il centro nel modo piu' comune.
	 */
	FRect RettangoloDellaZona(const FAnchorData& Layout)
	{
		const float AncoraSinistra = static_cast<float>(Layout.Anchors.Minimum.X) * RefWidth;
		const float AncoraDestra = static_cast<float>(Layout.Anchors.Maximum.X) * RefWidth;
		const float AncoraAlto = static_cast<float>(Layout.Anchors.Minimum.Y) * RefHeight;
		const float AncoraBasso = static_cast<float>(Layout.Anchors.Maximum.Y) * RefHeight;

		const bool bStiracchiatoX = Layout.Anchors.Minimum.X != Layout.Anchors.Maximum.X;
		const bool bStiracchiatoY = Layout.Anchors.Minimum.Y != Layout.Anchors.Maximum.Y;

		FRect R;

		if (bStiracchiatoX)
		{
			R.Left = AncoraSinistra + Layout.Offsets.Left;
			R.Right = AncoraDestra - Layout.Offsets.Right;
		}
		else
		{
			const float Larghezza = Layout.Offsets.Right;
			R.Left = AncoraSinistra + Layout.Offsets.Left - static_cast<float>(Layout.Alignment.X) * Larghezza;
			R.Right = R.Left + Larghezza;
		}

		if (bStiracchiatoY)
		{
			R.Top = AncoraAlto + Layout.Offsets.Top;
			R.Bottom = AncoraBasso - Layout.Offsets.Bottom;
		}
		else
		{
			const float Altezza = Layout.Offsets.Bottom;
			R.Top = AncoraAlto + Layout.Offsets.Top - static_cast<float>(Layout.Alignment.Y) * Altezza;
			R.Bottom = R.Top + Altezza;
		}

		return R;
	}

	FString Descrivi(const FRect& R)
	{
		return FString::Printf(TEXT("X %.0f..%.0f  Y %.0f..%.0f  (%.0fx%.0f)"),
			R.Left, R.Right, R.Top, R.Bottom, R.Width(), R.Height());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelsLeaveTheCenterFreeTest,
	"RefactorTactics.ScreenHud.PanelsLeaveTheCenterFree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPanelsLeaveTheCenterFreeTest::RunTest(const FString&)
{
	const UWidgetTree* Tree = RTWidgetAssetTest::LoadWidgetTree(*this, TacticalHudPath,
		TEXT("WBP_RT_TacticalHUD"));
	if (Tree == nullptr)
	{
		return false;
	}

	// 🔑 La radice DEVE essere un Canvas: `guida-screen-hud-umg.md` §3 lo prescrive perche' una `Vertical
	// Box` a schermo pieno «non lascia un centro davvero libero». Se la radice cambia, questo gate non ha
	// piu' niente da misurare — e deve dirlo, non passare.
	const UCanvasPanel* Radice = Cast<UCanvasPanel>(Tree->RootWidget);
	if (!TestNotNull(
		TEXT("la radice di WBP_RT_TacticalHUD e' un Canvas Panel (guida-screen-hud-umg.md §3)"),
		Radice))
	{
		return false;
	}

	const RTCenterFree::FRect Centro = RTCenterFree::CenterKeepOut();
	AddInfo(FString::Printf(TEXT("=== centro da lasciare libero a %.0fx%.0f: %s ==="),
		RTCenterFree::RefWidth, RTCenterFree::RefHeight, *RTCenterFree::Descrivi(Centro)));

	// Le geometrie sono float: mezzo pixel di tolleranza evita che un arrotondamento diventi un difetto.
	constexpr float Tolleranza = 0.5f;

	int32 ZoneMisurate = 0;

	Tree->ForEachWidget([this, &Centro, &ZoneMisurate, Radice, Tolleranza](UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}

		const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot);
		if (!Slot || Slot->Parent != Radice)
		{
			return; // non e' una zona di primo livello: il suo posto lo decide la zona che lo contiene
		}

		const FAnchorData Layout = Slot->GetLayout();

		// ⚠️ **`AutoSize` rende la zona non misurabile QUI, e non e' un limite del test: e' il difetto.**
		// Una zona che si dimensiona sul contenuto puo' invadere il centro quando il contenuto cresce — un
		// roster con piu' unita', un nome piu' lungo — e nessun numero nell'asset la trattiene. Il centro
		// libero smette di essere una proprieta' del layout e diventa una coincidenza dei dati.
		if (Slot->GetAutoSize())
		{
			AddError(FString::Printf(
				TEXT("la zona `%s` e' AutoSize: la sua dimensione dipende dal contenuto, quindi il centro ")
				TEXT("libero non e' garantito dal layout. Dalle una dimensione esplicita nel Canvas."),
				*Widget->GetName()));
			return;
		}

		const RTCenterFree::FRect Zona = RTCenterFree::RettangoloDellaZona(Layout);
		++ZoneMisurate;

		AddInfo(FString::Printf(
			TEXT("  %-22s %-24s anchors=(%.2f,%.2f)-(%.2f,%.2f) align=(%.2f,%.2f)  ->  %s"),
			*Widget->GetName(), *Widget->GetClass()->GetName(),
			Layout.Anchors.Minimum.X, Layout.Anchors.Minimum.Y,
			Layout.Anchors.Maximum.X, Layout.Anchors.Maximum.Y,
			Layout.Alignment.X, Layout.Alignment.Y,
			*RTCenterFree::Descrivi(Zona)));

		// 🔴 **Una zona FUORI dallo schermo non invade il centro — e senza questo controllo passerebbe.**
		// E' il difetto che questo test ha trovato alla prima esecuzione: `ZoneBottom` risultava
		// `Y 1080..1280`, cioe' duecento pixel sotto il bordo inferiore, perche' con anchor in basso e
		// `Alignment.Y = 0` l'origine resta sul bordo invece di risalire dell'altezza. Il criterio del
		// centro libero da solo la dichiarava a posto: il centro lo lascia libero **non esistendo**.
		if (Zona.Left < -Tolleranza || Zona.Top < -Tolleranza
			|| Zona.Right > RTCenterFree::RefWidth + Tolleranza
			|| Zona.Bottom > RTCenterFree::RefHeight + Tolleranza)
		{
			AddError(FString::Printf(
				TEXT("la zona `%s` cade fuori dallo schermo di riferimento: %s contro %.0fx%.0f. ")
				TEXT("Una zona fuori viewport non si vede in partita, e non invade il centro solo perche' ")
				TEXT("non c'e'. Se e' ancorata a un bordo, l'Alignment deve riportarla dentro."),
				*Widget->GetName(),
				*RTCenterFree::Descrivi(Zona),
				RTCenterFree::RefWidth, RTCenterFree::RefHeight));
		}

		if (RTCenterFree::SiToccano(Zona, Centro))
		{
			const RTCenterFree::FRect Invasione = RTCenterFree::Intersezione(Zona, Centro);
			AddError(FString::Printf(
				TEXT("la zona `%s` invade il centro tattico: occupa %s del riquadro %s. ")
				TEXT("Il layer §4.2 (`ARTHUD::DrawHUD`) disegna li' path, AoE e fuoco amico — ")
				TEXT("un pannello al centro glieli copre (guida-screen-hud-umg.md §3)."),
				*Widget->GetName(),
				*RTCenterFree::Descrivi(Invasione),
				*RTCenterFree::Descrivi(Centro)));
		}
	});

	// Senza questa riga il test sarebbe verde su un albero senza zone — cioe' misurando zero, che e' il
	// modo in cui un gate diventa decorativo. Questo file lo ha gia' imparato una volta, piu' su.
	TestTrue(
		*FString::Printf(TEXT("il Canvas radice dichiara delle zone da misurare (ne ha %d)"), ZoneMisurate),
		ZoneMisurate > 0);

	return true;
}

// =====================================================================================================
// Chi e' MONTATO nell'albero della HUD (#2697, #2760)
// =====================================================================================================
//
// 🔴 **La domanda che i due test qui sotto pongono e' diversa da tutte quelle sopra**, e la differenza e'
// esattamente il difetto che #2697 insegue da tre stesure: quelli sopra chiedono *«questo widget e' fatto
// bene?»*, e possono essere tutti verdi mentre il widget **non e' nell'albero di nessuno**. Un canale che
// nessuno monta non produce nessun rosso, come un canale che nessuno legge.
//
// ⚠️ **Non e' un'ipotesi: e' successo due volte sullo stesso asset.** `cc5ca967` — `feat(2697): la destra
// ospita il feed invece di un secondo pannello unita'` — dichiara un montaggio che nel `.uasset` non c'e',
// e nello stesso salvataggio ha riportato l'albero allo stato **precedente** al fix di #2760. Misura, sulla
// tabella dei nomi del pacchetto: `WBP_RT_TacticalHUD` a `cc5ca967` e' nome-per-nome **identico** a
// `bbca9a36`, il commit prima di quel fix. Nessun gate se n'e' accorto perche' nessun gate guardava.

/**
 * ⛔ **Il feed del giocatore deve essere MONTATO, non solo esistere.**
 *
 * `URTPlayerEventLogWidget::GetFeed()` e' filtrato per osservatore, coperto da
 * `EventFeedShowsOnlyWhatTheObserverMaySee`, e `WBP_RT_EventLog` ha radice, contenitore e grafo (#2784).
 * Tutto verde, e a schermo **niente**: `WBP_RT_TacticalHUD` non lo referenziava.
 *
 * 🔑 **Il test chiede la PRESENZA nell'albero, non la zona.** Dove vada e' materia di
 * `guida-screen-hud-umg.md` §3 — che oggi dice `RIGHT` — e congelarla qui darebbe a un test di montaggio
 * un'opinione sul layout. La zona finisce nel report, cosi' il log dice **dove** e' atterrato senza che il
 * criterio dipenda dalla risposta.
 *
 * ⚠️ **ESATTAMENTE UNO, non «almeno uno»** — corretto in code review. Il contatore c'era gia' e il test
 * guardava solo lo zero: due feed montati sarebbero due cronache sovrapposte a schermo, e sarebbero
 * passati.
 *
 * ⚠️ **Limite dichiarato, e vale per tutti e tre i test di questo blocco**: `UWidgetTree::ForEachWidget`
 * cammina l'albero di QUESTO Blueprint e si ferma sui `UUserWidget` innestati, che hanno un albero loro.
 * Un segnaposto dentro `WBP_RT_ActionDock` o `WBP_RT_SelectedUnitPanel` non lo vede nessuno di qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudMountsTheFeedTest,
	"RefactorTactics.ScreenHud.TheHudMountsTheFeedThatExplainsTheTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudMountsTheFeedTest::RunTest(const FString&)
{
	const UWidgetTree* Tree = RTWidgetAssetTest::LoadWidgetTree(*this, TacticalHudPath,
		TEXT("WBP_RT_TacticalHUD"));
	if (Tree == nullptr)
	{
		return false;
	}

	int32 Trovati = 0;
	TArray<FString> Inquilini;

	Tree->ForEachWidget([this, &Trovati, &Inquilini](UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}

		// Solo i widget-blueprint innestati: i contenitori nudi (`Canvas`, `HorizontalBox`) sono struttura,
		// e l'elenco serve a dire CHI abita l'albero, non com'e' fatto.
		if (Widget->IsA<UUserWidget>())
		{
			Inquilini.Add(FString::Printf(TEXT("  %-32s %-34s in `%s`"),
				*Widget->GetName(),
				*Widget->GetClass()->GetName(),
				Widget->Slot && Widget->Slot->Parent ? *Widget->Slot->Parent->GetName() : TEXT("<radice>")));
		}

		if (Widget->IsA(URTPlayerEventLogWidget::StaticClass()))
		{
			++Trovati;
		}
	});

	AddInfo(FString::Printf(TEXT("=== inquilini di WBP_RT_TacticalHUD (%d) ==="), Inquilini.Num()));
	for (const FString& Riga : Inquilini)
	{
		AddInfo(Riga);
	}

	// Senza questa riga il test sarebbe verde su un albero vuoto — lo stesso modo in cui
	// `PanelsLeaveTheCenterFree` sarebbe diventato decorativo, e che quel test ha gia' imparato a evitare.
	TestTrue(
		*FString::Printf(TEXT("l'albero della HUD ospita dei widget-blueprint (ne ha %d)"), Inquilini.Num()),
		Inquilini.Num() > 0);

	if (Trovati == 0)
	{
		// ⚠️ `FString(TEXT(...))` e non `Printf`: il messaggio non ha segnaposto, e un `Printf` senza
		// argomenti diventa una trappola il giorno in cui qualcuno ci mette dentro un `%` letterale —
		// proprio nel ramo il cui mestiere e' spiegare cosa fare. Trovato in code review.
		AddError(FString(
			TEXT("`WBP_RT_TacticalHUD` non monta nessun `URTPlayerEventLogWidget`: il feed che spiega ")
			TEXT("perche' un'azione dichiarata non e' avvenuta esiste (`WBP_RT_EventLog`, con grafo da ")
			TEXT("#2784) e non e' nell'albero di nessuno, quindi in partita non disegna. ")
			TEXT("Monta un'istanza di `/Game/RT/UI/Match/WBP_RT_EventLog` nella zona che ")
			TEXT("`docs/technical/runbooks/guida-screen-hud-umg.md` §3 le assegna (#2697, #1936 fetta F).")));
	}
	else if (Trovati > 1)
	{
		AddError(FString::Printf(
			TEXT("`WBP_RT_TacticalHUD` monta %d `URTPlayerEventLogWidget`: il feed e' una cronaca sola, e ")
			TEXT("due istanze si sovrappongono a schermo leggendo lo stesso `GetTurnLog()`. ")
			TEXT("Tienine una."),
			Trovati));
	}

	return true;
}

/**
 * ⛔ **Ogni zona ospita l'inquilino che `guida-screen-hud-umg.md` §3 le assegna, verificato per CLASSE.**
 *
 * 🔴 **Esiste per un buco trovato in code review**, e il buco era nella pretesa dei due test fratelli:
 * dicevano di presidiare la regressione di #2760, e presidiavano **un solo sintomo** — un nodo che porta
 * il nome di un widget senza esserne un'istanza. Un risalvataggio stantio che lasciasse
 * `ZoneBottomContainer` semplicemente **vuoto** — nessun nodo, invece di un nodo omonimo — li avrebbe
 * lasciati entrambi verdi, col dock di nuovo impopolabile.
 *
 * 🔑 **La domanda e' «c'e' un'istanza di questa classe?», non «c'e' un nodo con questo nome».** Il nome e'
 * una convenzione e si puo' cambiare senza rompere niente; la classe e' il contratto — se manca, il
 * binding non ha nessuno da chiamare.
 *
 * ⚠️ **`WBP_RT_ActionSlot`, `WBP_RT_UnitCard` e `WBP_RT_FastDecision` NON sono qui**, e non e' una
 * dimenticanza: nascono a runtime dentro i rispettivi contenitori (`SetAction`, `AddChildToVerticalBox`),
 * quindi nell'archetipo dell'albero non ci sono e chiederli renderebbe il test rosso su un asset corretto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudMountsEveryZoneOwnerTest,
	"RefactorTactics.ScreenHud.EveryZoneOwnerIsMountedByClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudMountsEveryZoneOwnerTest::RunTest(const FString&)
{
	const UWidgetTree* Tree = RTWidgetAssetTest::LoadWidgetTree(*this, TacticalHudPath,
		TEXT("WBP_RT_TacticalHUD"));
	if (Tree == nullptr)
	{
		return false;
	}

	struct FInquilino
	{
		UClass* Classe;
		const TCHAR* Zona;   // solo per il messaggio: il criterio non guarda dove sia atterrato
		const TCHAR* Issue;
	};

	const FInquilino Attesi[] = {
		{ URTTurnHeaderWidget::StaticClass(),        TEXT("TOP"),    TEXT("#613") },
		{ URTTeamRosterWidget::StaticClass(),        TEXT("LEFT"),   TEXT("#613, #2744") },
		{ URTPlayerEventLogWidget::StaticClass(),    TEXT("RIGHT"),  TEXT("#2697, #1936 fetta F") },
		{ URTSelectedUnitPanelWidget::StaticClass(), TEXT("BOTTOM"), TEXT("#613, #2760") },
		{ URTActionDockWidget::StaticClass(),        TEXT("BOTTOM"), TEXT("#220, #2760") },
	};

	for (const FInquilino& Atteso : Attesi)
	{
		int32 Conta = 0;
		Tree->ForEachWidget([&Conta, &Atteso](UWidget* Widget)
		{
			if (Widget && Widget->IsA(Atteso.Classe))
			{
				++Conta;
			}
		});

		AddInfo(FString::Printf(TEXT("  %-34s zona %-6s -> %d istanza/e"),
			*Atteso.Classe->GetName(), Atteso.Zona, Conta));

		if (Conta == 0)
		{
			AddError(FString::Printf(
				TEXT("`WBP_RT_TacticalHUD` non monta nessun `%s`, che `guida-screen-hud-umg.md` §3 assegna ")
				TEXT("alla zona `%s` (%s). Un binding senza istanza non ha nessuno da chiamare: il pannello ")
				TEXT("non si popola per costruzione, non per una selezione mancante."),
				*Atteso.Classe->GetName(), Atteso.Zona, Atteso.Issue));
		}
	}

	return true;
}

/**
 * ⛔ **Un nodo che porta il nome di un widget deve esserne un'istanza.**
 *
 * E' il difetto di #2760, scritto come regola invece che come caso: `ZoneBottomContainer` conteneva due
 * `HorizontalBox` **vuoti** chiamati `WBP_RT_SelectedUnitPanel` e `WBP_RT_ActionDock`. Portavano il nome
 * senza esserne istanze, quindi nessun binding poteva popolarli e il dock non poteva riempirsi **per
 * costruzione** — non perche' mancasse una selezione.
 *
 * 🔴 **#2760 e' stata chiusa senza questo oracolo, e la correzione e' regredita in silenzio.** Il fix
 * (`9943dfda`) ha sostituito i due segnaposto con `WBP_RT_SelectedUnitPanelBottom` e
 * `WBP_RT_ActionDockBottom`; `cc5ca967` ha risalvato l'asset com'era prima, e i due segnaposto sono
 * tornati. Fra i due eventi la suite e' rimasta verde: **nessun test guardava l'albero**.
 *
 * ⚠️ **Questo test da solo NON presidia #2760**, e la prima stesura lo lasciava credere: vede un nodo
 * *omonimo*, non un nodo *mancante*. La meta' che manca la copre `EveryZoneOwnerIsMountedByClass`, ed e'
 * quella che risponde alla domanda «il dock c'e'?».
 *
 * ⚠️ **Nessun requisito di NOMENCLATURA, ed e' una correzione da code review.** La prima stesura pretendeva
 * almeno un nodo col prefisso `WBP_` — cioe' rendeva obbligatoria una convenzione che nessun documento
 * impone, e sarebbe diventata rossa su un albero corretto i cui nodi si chiamassero `EventLogRight`.
 * L'anti-vacuita' guarda che l'albero abbia dei widget, non come si chiamino.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoNodeWearsAWidgetNameTest,
	"RefactorTactics.ScreenHud.NoNodeWearsTheNameOfAWidgetWithoutBeingOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTNoNodeWearsAWidgetNameTest::RunTest(const FString&)
{
	const UWidgetTree* Tree = RTWidgetAssetTest::LoadWidgetTree(*this, TacticalHudPath,
		TEXT("WBP_RT_TacticalHUD"));
	if (Tree == nullptr)
	{
		return false;
	}

	int32 Nodi = 0;
	int32 Esaminati = 0;

	Tree->ForEachWidget([this, &Nodi, &Esaminati](UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}

		++Nodi;

		if (!Widget->GetName().StartsWith(TEXT("WBP_")))
		{
			return;
		}

		++Esaminati;

		if (Widget->IsA<UUserWidget>())
		{
			return;
		}

		AddError(FString::Printf(
			TEXT("il nodo `%s` porta il nome di un widget-blueprint ma e' un `%s`: un contenitore nudo che ")
			TEXT("ne indossa il nome non ha i suoi binding e non si popola per costruzione, e a chi legge ")
			TEXT("l'albero sembra montato. Sostituiscilo con un'istanza — quale, lo dice ")
			TEXT("`guida-screen-hud-umg.md` §3, e lo verifica `EveryZoneOwnerIsMountedByClass` (#2760, ")
			TEXT("regredito con `cc5ca967`)."),
			*Widget->GetName(),
			*Widget->GetClass()->GetName()));
	});

	AddInfo(FString::Printf(TEXT("=== %d nodi nell'albero, %d col prefisso `WBP_` ==="), Nodi, Esaminati));

	// ⚠️ L'anti-vacuita' guarda i NODI, non i nomi: un albero vuoto e' il caso da escludere, un albero con
	// altre convenzioni di nome no. Zero nodi col prefisso e' un esito legittimo, e il report lo dice.
	TestTrue(*FString::Printf(TEXT("l'albero contiene dei nodi da esaminare (ne ha %d)"), Nodi), Nodi > 0);

	return true;
}

/**
 * 🔴 **UNO SLOT DEVE POTER RICEVERE UN CLICK — cioe' non essere interamente trasparente al puntatore**
 * (`#2989`).
 *
 * 🔑 **E' il gate che manca alla catena, e il tratto che presidia non ne aveva nessuno.** La porta C++
 * esiste ed e' testata — `ArmKitAbility` e' `BlueprintCallable`, e
 * `PlayerInput.TheDockPortArmsAndDisarms` prova che armi e disarmi — ma fra il puntatore e quella porta
 * c'e' un tratto che vive interamente dentro il `.uasset`, e su quel tratto la suite non aveva niente.
 *
 * ⚠️ **Questo gate copre una meta' sola, e va detto quale.** Misura che un click *possa arrivare* allo
 * slot; **non** che ci sia qualcosa che lo riceva. Al 2026-09-11 l'albero di `WBP_RT_ActionSlot` contiene
 * `ArmedBorder` (`Overlay`), `IconImage` e due `TextBlock`, e **nessuno di essi e' un `Button`**: un gate
 * sull'altra meta' sarebbe rosso, e per la disciplina di questo file — *«fallisce finche' i binding non ci
 * sono, ed e' voluto: per questo atterra INSIEME all'asset cablato, non prima»* — quello arrivera' col
 * cablaggio. Qui si difende cio' che gia' vale.
 *
 * 🔴 **Il difetto che rende questo gate utile e' banale da introdurre e invisibile a occhio.** Chi cabla un
 * bottone deve scegliere la `Visibility` di ogni antenato: uno solo su `Hit Test Invisible` e il click
 * attraversa lo slot e finisce sulla mappa sotto — e a schermo lo slot sembra a posto, perche' la
 * `Visibility` che lo nasconde al puntatore non lo nasconde all'occhio.
 *
 * ⛔ `SelfHitTestInvisible` su un CONTENITORE non e' un difetto, ed e' la ragione per cui questo gate
 * guarda l'albero e non la radice: e' il valore corretto per un pannello che deve lasciar passare il
 * puntatore ai propri figli, ed e' quello che `ArmedBorder` porta oggi.
 *
 * ⌫ **Questa riga nominava anche `SlotBox`, e sbagliava albero.** `SlotBox` e' l'`HorizontalBox` del
 * **dock** — `RTHudScenarioTests.cpp` lo cerca in `Dock->WidgetTree`, e nell'albero dello slot ha **zero**
 * occorrenze. Attribuirlo qui suggeriva che il contenitore degli slot vivesse dentro uno slot, cioe'
 * esattamente la confusione fra i due `.uasset` che questo file esiste per tenere separati. Trovato da una
 * seduta Editor su `#2826` il 2026-09-11.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionSlotCanReceiveAClickTest,
	"RefactorTactics.ScreenHud.ActionSlotIsNotTransparentToThePointer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTActionSlotCanReceiveAClickTest::RunTest(const FString&)
{
	const UWidgetTree* Tree = RTWidgetAssetTest::LoadWidgetTree(*this, ActionSlotPath,
		TEXT("WBP_RT_ActionSlot"));
	if (Tree == nullptr)
	{
		return false;
	}

	int32 Esaminati = 0;
	int32 RaggiungibiliDalPuntatore = 0;

	Tree->ForEachWidget([this, &Esaminati, &RaggiungibiliDalPuntatore](UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}
		++Esaminati;

		const ESlateVisibility Vis = Widget->GetVisibility();

		// `Visible` e' l'unico valore che fa ricevere il click al widget STESSO.
		// ⛔ Gli altri quattro non lo sono, e per ragioni diverse: `Collapsed` e `Hidden` non disegnano
		// affatto; `HitTestInvisible` disegna e lascia passare il puntatore **anche ai figli**;
		// `SelfHitTestInvisible` disegna, non riceve, ma **i figli si'** — ed e' il valore giusto per un
		// contenitore.
		if (Vis == ESlateVisibility::Visible)
		{
			++RaggiungibiliDalPuntatore;
		}

		AddInfo(FString::Printf(TEXT("  %-24s %-20s visibility=%d"),
			*Widget->GetName(), *Widget->GetClass()->GetName(), static_cast<int32>(Vis)));
	});

	// Controprova della premessa: senza, il gate passerebbe su un albero vuoto — cioe' misurando zero.
	if (!TestTrue(
		FString::Printf(TEXT("l'albero di WBP_RT_ActionSlot porta dei widget da esaminare (ne ha %d)"),
			Esaminati),
		Esaminati > 0))
	{
		return false;
	}

	TestTrue(
		FString::Printf(
			TEXT("almeno un widget dello slot puo' ricevere il puntatore (ne ha %d su %d). Uno slot ")
			TEXT("interamente `Hit Test Invisible` lascia passare il click alla mappa sotto, e a schermo ")
			TEXT("sembra a posto: e' il difetto che questo gate esiste per trovare"),
			RaggiungibiliDalPuntatore, Esaminati),
		RaggiungibiliDalPuntatore > 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
