// CP 11.7 (#613) — i binding dichiarati DENTRO i `WBP_RT_*` di **Match**.
//
// ⚠️ **Gemello di `RTFrontendWidgetAssetTests.cpp`, per la meta' che quello non copre.** Quel file prova
// tre widget del Framework (`ErrorModal`, `FallbackBanner`, `LoadingScreen`) ed esiste per un difetto
// misurato: il modale si armava e non compariva, e fra le due cose c'era un `FDelegateRuntimeBinding`
// serializzato nel binario. I widget di `Content/RT/UI/Match/` elencati in `Paths[]` non avevano nulla di
// equivalente:
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
#include "Tests/RTHudGeometryForTest.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h" // e' lo SLOT a decidere se la zona riempie la cella, non il widget
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h" // la barra del countdown della finestra di reazione (#3634)
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h" // CardsBox, PlanWarningsBox: le porte sono pannelli (#3636)
#include "Components/Widget.h"
#include "UI/RTScreenHudWidgets.h"
#include "UI/RTHudZoneWidget.h"
#include "Engine/Texture2D.h" // UTexture2D: il tipo che il gate rifiuta, esplicito e non ereditato
#include "Tests/RTWidgetAssetTestHelpers.h"
#include "Algo/Accumulate.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const TCHAR* const TacticalHudPath = TEXT("/Game/RT/UI/Match/WBP_RT_TacticalHUD.WBP_RT_TacticalHUD_C");
	const TCHAR* const TurnHeaderPath = TEXT("/Game/RT/UI/Match/WBP_RT_TurnHeader.WBP_RT_TurnHeader_C");
	const TCHAR* const TeamRosterPath = TEXT("/Game/RT/UI/Match/WBP_RT_TeamRoster.WBP_RT_TeamRoster_C");
	const TCHAR* const SelectedUnitPath = TEXT("/Game/RT/UI/Match/WBP_RT_SelectedUnitPanel.WBP_RT_SelectedUnitPanel_C");
	const TCHAR* const ActionDockPath = TEXT("/Game/RT/UI/Match/WBP_RT_ActionDock.WBP_RT_ActionDock_C");
	const TCHAR* const ActionSlotPath = TEXT("/Game/RT/UI/Match/WBP_RT_ActionSlot.WBP_RT_ActionSlot_C");
	const TCHAR* const ActionTooltipPath = TEXT("/Game/RT/UI/Match/WBP_RT_ActionTooltip.WBP_RT_ActionTooltip_C");
	const TCHAR* const UnitCardPath = TEXT("/Game/RT/UI/Match/WBP_RT_UnitCard.WBP_RT_UnitCard_C");
	// La voce di un avviso di piano nel pannello (#3636): entra con l'asset, dalla seduta `U76`.
	const TCHAR* const PlanWarningEntryPath = TEXT("/Game/RT/UI/Match/WBP_RT_PlanWarningEntry.WBP_RT_PlanWarningEntry_C");
	// Conferma e Annulla in `TopRight` ([D-458], #3471): entra con l'asset, dalla seduta `U64`.
	const TCHAR* const PlanCommitPath = TEXT("/Game/RT/UI/Match/WBP_RT_PlanCommit.WBP_RT_PlanCommit_C");
	// UNA zona: il contenitore che porta `ZoneId`, il bordo da cantiere e il `NamedSlot Content`.
	const TCHAR* const HudZonePath = TEXT("/Game/RT/UI/Match/WBP_RT_HudZone.WBP_RT_HudZone_C");
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
	// ⌫ *Fino alla seduta `U77` (#3637) `WBP_RT_EventLine` derivava da `UUserWidget` nudo e non entrava in
	// `Expected[]`. Riparentata su `URTPlayerEventLineWidget`, ora c'e'.*
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
 * I widget di Match elencati in `Paths[]` esistono e si caricano.
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
		EventLogPath, EventLinePath, PlanCommitPath
	};
	const TCHAR* const Labels[] = {
		TEXT("TacticalHUD"), TEXT("TurnHeader"), TEXT("TeamRoster"), TEXT("SelectedUnitPanel"),
		TEXT("ActionDock"), TEXT("ActionSlot"), TEXT("UnitCard"), TEXT("FastDecision"),
		TEXT("FastDecisionOption"), TEXT("EventLog"), TEXT("EventLine"), TEXT("PlanCommit")
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
 * ⌫ *Fino alla seduta `U76` `UnitCard` non era qui: `WBP_RT_UnitCard` derivava da `UUserWidget` nudo. Riparentata su
 * `URTUnitCardWidget` (#3636), entra insieme alla voce d'avviso nuova.*
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
		{ PlanCommitPath,   TEXT("PlanCommit"),        URTPlanCommitWidget::StaticClass() },
		{ UnitCardPath,     TEXT("UnitCard"),          URTUnitCardWidget::StaticClass() },
		{ PlanWarningEntryPath, TEXT("PlanWarningEntry"), URTPlanWarningEntryWidget::StaticClass() },
		{ EventLinePath,    TEXT("EventLine"),         URTPlayerEventLineWidget::StaticClass() },
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
 * variabili aggiunte dentro il `.uasset`. L'infrastruttura c'era gia' tutta in questo file — le classi di
 * `Paths[]` si caricano per `PIE-ICON-01` e per `ActionSlotHasIconSurface` — e mancava soltanto
 * l'asserzione.
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
		EventLogPath, EventLinePath, PlanCommitPath
	};
	const TCHAR* const Labels[] = {
		TEXT("TacticalHUD"), TEXT("TurnHeader"), TEXT("TeamRoster"), TEXT("SelectedUnitPanel"),
		TEXT("ActionDock"), TEXT("ActionSlot"), TEXT("UnitCard"), TEXT("FastDecision"),
		TEXT("FastDecisionOption"), TEXT("EventLog"), TEXT("EventLine"), TEXT("PlanCommit")
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
	TestEqual(TEXT("i widget di Match elencati in `Paths[]` si caricano"), Caricate,
		static_cast<int32>(UE_ARRAY_COUNT(Paths)));
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
 * 🔑 **Il riquadro e' la scena, e viene dal dato** (#3661, D-497): `RTHudLayout::Bands()`, le fasce in unita'
 * Slate. ⌫ *Fino al 2026-10-11 era il 60% x 60% centrato, `CenterFraction`, una costante dei test.*
 *
 * ⚠️ **L'oracolo e' l'archetipo dell'albero, non un fotogramma renderizzato**: le zone si misurano da
 * `UCanvasPanelSlot` con la stessa formula che `SConstraintCanvas::OnArrangeChildren` applica a runtime.
 * Vale percio' per il layout **dichiarato nell'asset**; un widget spostato a runtime da Blueprint sfugge
 * di qui e resta di PIE.
 */
namespace RTCenterFree
{
	// ⌫ **La meta' PURA di questo namespace e' uscita di qui il 2026-09-24** — costanti, `FRect`,
	// `CenterKeepOut` (da #3661 `SceneKeepOut`), `Intersezione`, `SiToccano`, `Descrivi` — e vive in
	// `Tests/RTHudGeometryForTest.h`. La ragione e' #3319: il Context Inspector ha bisogno dello stesso
	// keep-out e non e' una zona dell'HUD, e due copie dello stesso numero divergono in silenzio.
	// 🔑 Cio' che RESTA qui dipende da UMG — `RettangoloDellaZona` legge un `FAnchorData` — e un header
	// di `Tests/` e' compilato in ogni target: chi vuole un rettangolo non deve pagare UMG per averlo.


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
	FRect RettangoloDellaZona(const FAnchorData& Layout, float Larghezza = RefWidth, float Altezza = RefHeight)
	{
		const float AncoraSinistra = static_cast<float>(Layout.Anchors.Minimum.X) * Larghezza;
		const float AncoraDestra = static_cast<float>(Layout.Anchors.Maximum.X) * Larghezza;
		const float AncoraAlto = static_cast<float>(Layout.Anchors.Minimum.Y) * Altezza;
		const float AncoraBasso = static_cast<float>(Layout.Anchors.Maximum.Y) * Altezza;

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
			const float LarghezzaZona = Layout.Offsets.Right;
			R.Left = AncoraSinistra + Layout.Offsets.Left - static_cast<float>(Layout.Alignment.X) * LarghezzaZona;
			R.Right = R.Left + LarghezzaZona;
		}

		if (bStiracchiatoY)
		{
			R.Top = AncoraAlto + Layout.Offsets.Top;
			R.Bottom = AncoraBasso - Layout.Offsets.Bottom;
		}
		else
		{
			const float AltezzaZona = Layout.Offsets.Bottom;
			R.Top = AncoraAlto + Layout.Offsets.Top - static_cast<float>(Layout.Alignment.Y) * AltezzaZona;
			R.Bottom = R.Top + AltezzaZona;
		}

		return R;
	}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelsLeaveTheSceneFreeTest,
	"RefactorTactics.ScreenHud.PanelsLeaveTheSceneFree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPanelsLeaveTheSceneFreeTest::RunTest(const FString&)
{
	const UWidgetTree* Originale = RTWidgetAssetTest::LoadWidgetTree(*this, TacticalHudPath,
		TEXT("WBP_RT_TacticalHUD"));
	if (Originale == nullptr)
	{
		return false;
	}

	// 🔑 La radice DEVE essere un Canvas: `guida-screen-hud-umg.md` §3 lo prescrive, e le zone si posano come
	// slot di un Canvas. Se la radice cambia, questo gate non ha piu' niente da misurare — e deve dirlo.
	if (!TestNotNull(TEXT("la radice di WBP_RT_TacticalHUD e' un Canvas Panel (guida-screen-hud-umg.md §3)"),
		Cast<UCanvasPanel>(Originale->RootWidget)))
	{
		return false;
	}

	// 🔑 **Le zone si posano dal dato, come in partita** (`URTTacticalHUDWidget::ApplyBandLayout`, #3661): gli
	// anchor salvati nell'asset non contano piu'. ⚠️ Su una COPIA: l'albero caricato e' quello dell'asset, e
	// gli altri test di questo file lo leggono com'e' salvato.
	UWidgetTree* Tree = DuplicateObject<UWidgetTree>(const_cast<UWidgetTree*>(Originale), GetTransientPackage());
	const UCanvasPanel* Radice = Tree ? Cast<UCanvasPanel>(Tree->RootWidget) : nullptr;
	if (!TestNotNull(TEXT("la copia dell'albero ha la sua radice"), Radice))
	{
		return false;
	}
	const int32 Posate = URTTacticalHUDWidget::ApplyBandLayout(Tree, RTHudLayout::Bands());
	TestTrue(*FString::Printf(TEXT("ApplyBandLayout posa delle zone (%d)"), Posate), Posate > 0);

	// Le geometrie sono float: mezzo punto di tolleranza evita che un arrotondamento diventi un difetto.
	constexpr float Tolleranza = 0.5f;

	for (const RTHudRisoluzioni::FRisoluzione& Ris : RTHudRisoluzioni::Tutte())
	{
		const FVector2D Schermo = RTHudRisoluzioni::InSlate(Ris);
		const RTCenterFree::FRect Scena = RTCenterFree::SceneKeepOut(Schermo.X, Schermo.Y);
		AddInfo(FString::Printf(TEXT("=== %s: schermo %.0fx%.0f in unita' Slate, scena %s ==="),
			Ris.Nome, Schermo.X, Schermo.Y, *RTCenterFree::Descrivi(Scena)));

		int32 ZoneMisurate = 0;
		int32 FinestreEsentate = 0;

		Tree->ForEachWidget([this, &Ris, &Schermo, &Scena, &ZoneMisurate, &FinestreEsentate, Radice, Tolleranza](UWidget* Widget)
		{
			const UCanvasPanelSlot* Slot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
			if (!Slot || Slot->Parent != Radice)
			{
				return; // non e' un figlio di primo livello: il suo posto lo decide chi lo contiene
			}

			const FAnchorData Layout = Slot->GetLayout();

			// ⚠️ **`AutoSize` rende la zona non misurabile, e non e' un limite del test: e' il difetto.** Una
			// zona che si dimensiona sul contenuto puo' invadere la scena quando il contenuto cresce.
			if (Slot->GetAutoSize() && !Widget->IsA(URTFastDecisionWidget::StaticClass()))
			{
				AddError(FString::Printf(
					TEXT("`%s` e' AutoSize: la sua dimensione dipende dal contenuto, quindi la scena libera non ")
					TEXT("e' garantita dal layout."), *Widget->GetName()));
				return;
			}

			const RTCenterFree::FRect Zona = RTCenterFree::RettangoloDellaZona(Layout,
				static_cast<float>(Schermo.X), static_cast<float>(Schermo.Y));
			++ZoneMisurate;
			AddInfo(FString::Printf(TEXT("  %-22s -> %s"), *Widget->GetName(), *RTCenterFree::Descrivi(Zona)));

			// ⚖️ **La finestra di reazione e' l'UNICA esenzione** (D-497 (5), `#166`): sta sopra la scena finche'
			// la finestra e' aperta, ma CENTRATA SULLA SCENA, non sullo schermo.
			if (Widget->IsA(URTFastDecisionWidget::StaticClass()))
			{
				++FinestreEsentate;
				const float CentroX = (Zona.Left + Zona.Right) * 0.5f;
				const float CentroY = (Zona.Top + Zona.Bottom) * 0.5f;
				const float ScenaX = (Scena.Left + Scena.Right) * 0.5f;
				const float ScenaY = (Scena.Top + Scena.Bottom) * 0.5f;
				TestTrue(*FString::Printf(
					TEXT("%s: il centro della finestra di reazione (%.1f, %.1f) e' il centro della scena (%.1f, %.1f)"),
					Ris.Nome, CentroX, CentroY, ScenaX, ScenaY),
					FMath::IsNearlyEqual(CentroX, ScenaX, 1.f) && FMath::IsNearlyEqual(CentroY, ScenaY, 1.f));
				return;
			}

			// 🔴 **Una zona FUORI dallo schermo non invade la scena — e senza questo controllo passerebbe.**
			if (Zona.Left < -Tolleranza || Zona.Top < -Tolleranza
				|| Zona.Right > Schermo.X + Tolleranza || Zona.Bottom > Schermo.Y + Tolleranza)
			{
				AddError(FString::Printf(TEXT("%s: la zona `%s` cade fuori dallo schermo: %s contro %.0fx%.0f."),
					Ris.Nome, *Widget->GetName(), *RTCenterFree::Descrivi(Zona), Schermo.X, Schermo.Y));
			}

			if (RTCenterFree::SiToccano(Zona, Scena))
			{
				AddError(FString::Printf(
					TEXT("%s: la zona `%s` invade la scena: occupa %s del rettangolo %s. La 3D si rende solo li' ")
					TEXT("(D-497 (1)), e un pannello dentro la coprirebbe."),
					Ris.Nome, *Widget->GetName(),
					*RTCenterFree::Descrivi(RTCenterFree::Intersezione(Zona, Scena)),
					*RTCenterFree::Descrivi(Scena)));
			}
		});

		// ⚠️ **Le esenzioni si SOTTRAGGONO**: un albero con la sola finestra di reazione non misura niente.
		TestTrue(*FString::Printf(TEXT("%s: il Canvas radice ha delle zone da misurare (%d, oltre a %d esenti)"),
			Ris.Nome, ZoneMisurate - FinestreEsentate, FinestreEsentate), ZoneMisurate - FinestreEsentate > 0);
		// ⛔ **UNA finestra di reazione, non «almeno una»**: due istanze si sovrapporrebbero leggendo lo stesso view model.
		TestTrue(*FString::Printf(TEXT("%s: al piu' UNA finestra di reazione (ne ha %d)"), Ris.Nome, FinestreEsentate),
			FinestreEsentate <= 1);
	}

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
 *
 * ⚠️ **Precisazione dopo il rimontaggio a otto zone**: il caso normale diventera' il `NamedSlot Content` di
 * `WBP_RT_HudZone`, che NON e' un `UUserWidget` innestato con un albero separato — e' un punto d'innesto
 * dentro QUESTO albero. `ForEachWidget` lo attraversa, scendendo negli `INamedSlotInterface` (verificato nel
 * sorgente engine, `WidgetTree.cpp`): il limite sopra resta vero alla lettera, ma non si applica al
 * contenuto delle zone.
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
	// `PanelsLeaveTheSceneFree` sarebbe diventato decorativo, e che quel test ha gia' imparato a evitare.
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

	// ⚠️ Il vocabolario e' quello di `ERTHudZone`, non il TOP/LEFT/RIGHT/BOTTOM di prima del 2026-09-12 — e
	// dal 2026-10-04 la dock sta in `Bottom`, la fascia intera, non piu' in `BottomCenter` ([D-456], #3469). Il criterio non
	// cambia — la domanda resta «c'e' un'istanza di questa classe?», e il test era verde prima ed e' verde
	// dopo — ma l'etichetta finisce in un messaggio d'errore che rimanda a `guida-screen-hud-umg.md` §3, e
	// quella sezione ora descrive la griglia 3x3. Un'etichetta che rimanda a una sezione cambiata sotto di
	// lei manda chi legge a cercare una zona che non esiste piu'.
	const FInquilino Attesi[] = {
		{ URTTurnHeaderWidget::StaticClass(),        TEXT("TopCenter"),    TEXT("#613") },
		{ URTTeamRosterWidget::StaticClass(),        TEXT("TopLeft"),      TEXT("#613, #2744") },
		{ URTPlayerEventLogWidget::StaticClass(),    TEXT("MiddleRight"),  TEXT("#2697, #1936 fetta F") },
		// ⚠️ MiddleLeft e non BottomLeft: Selected Unit cambia fascia col rimontaggio delle otto zone.
		{ URTSelectedUnitPanelWidget::StaticClass(), TEXT("MiddleLeft"),   TEXT("#613, #2760") },
		{ URTActionDockWidget::StaticClass(),        TEXT("Bottom"),       TEXT("#220, #2760, #3469") },
		// Conferma e Annulla ([D-458]): l'asset e la riga entrano nello stesso commit, dalla seduta `U64`.
		{ URTPlanCommitWidget::StaticClass(),        TEXT("TopRight"),     TEXT("#3471") },
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

		// `%-12s` e non `%-6s`: i nomi delle zone arrivano a `MiddleRight`, e una colonna troppo stretta
		// non tronca ma sfalsa tutte le righe successive, rendendo il report peggiore di nessun report.
		AddInfo(FString::Printf(TEXT("  %-34s zona %-12s -> %d istanza/e"),
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
// =====================================================================================================
// Le zone: ci sono tutte, una volta ciascuna
// =====================================================================================================
//
// 🔴 **E' la domanda che prima di `URTHudZoneWidget` nessuno poteva porre**, e la ragione per cui quella
// classe esiste. Una zona era un `UCanvasPanelSlot` con un nome scelto nel Designer: indistinguibile, per
// un test, da qualunque altro nodo. E' anche la domanda che `cc5ca967` ha eluso — un salvataggio che ha
// riportato l'albero allo stato precedente al fix di `#2760`, con la suite verde perche' nessun gate
// guardava.

// ⌫ *Fino al 2026-10-04 il test si chiamava `TheEightZonesAreDeclaredExactlyOnce`*: con la fascia bassa unica
// ([D-456], #3469) le zone non sono piu' otto, e un nome che conta direbbe il falso alla prossima modifica.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTheZonesAreDeclaredExactlyOnceTest,
	"RefactorTactics.ScreenHud.TheZonesAreDeclaredExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTTheZonesAreDeclaredExactlyOnceTest::RunTest(const FString&)
{
	const UWidgetTree* Tree = RTWidgetAssetTest::LoadWidgetTree(*this, TacticalHudPath,
		TEXT("WBP_RT_TacticalHUD"));
	if (Tree == nullptr)
	{
		return false;
	}

	// Quante istanze per ogni valore dell'enum. Zero e due sono due difetti diversi, e il report li deve
	// distinguere: «manca» si corregge aggiungendo, «doppia» si corregge cambiando un `ZoneId`.
	TArray<int32> Conteggio;
	Conteggio.Init(0, static_cast<int32>(ERTHudZone::Count));

	int32 FuoriEnum = 0;

	Tree->ForEachWidget([&Conteggio, &FuoriEnum, this](UWidget* Widget)
	{
		const URTHudZoneWidget* Zona = Cast<URTHudZoneWidget>(Widget);
		if (!Zona)
		{
			return;
		}

		const int32 Indice = static_cast<int32>(Zona->ZoneId);
		if (Conteggio.IsValidIndex(Indice))
		{
			++Conteggio[Indice];
			AddInfo(FString::Printf(TEXT("  %-24s ZoneId=%s"),
				*Widget->GetName(), *URTHudZoneWidget::ZoneName(Zona->ZoneId)));
		}
		else
		{
			++FuoriEnum;
			AddError(FString::Printf(
				TEXT("la zona `%s` porta un `ZoneId` che non e' una zona (indice %d). ")
				TEXT("`ERTHudZone::Count` e' una sentinella per il conteggio, non un valore assegnabile."),
				*Widget->GetName(), Indice));
		}
	});

	for (int32 I = 0; I < Conteggio.Num(); ++I)
	{
		const FString Nome = URTHudZoneWidget::ZoneName(static_cast<ERTHudZone>(I));

		if (Conteggio[I] == 0)
		{
			AddError(FString::Printf(
				TEXT("`WBP_RT_TacticalHUD` non dichiara nessuna zona `%s`. Le zone sono la griglia ")
				TEXT("20/60/20 meno il centro, con la fascia bassa intera (`guida-screen-hud-umg.md` §3, ")
				TEXT("D-456): una che manca e' un buco nel layout, non uno spazio libero."), *Nome));
		}
		else if (Conteggio[I] > 1)
		{
			AddError(FString::Printf(
				TEXT("`WBP_RT_TacticalHUD` dichiara %d zone `%s`. Due istanze con lo stesso `ZoneId` si ")
				TEXT("sovrappongono a schermo, e il blockout smette di dire quale riquadro si guarda."),
				Conteggio[I], *Nome));
		}
	}

	// 🔴 **Senza questa riga il test sarebbe verde su un albero SENZA zone** — cioe' misurando zero, il modo
	// in cui un gate diventa decorativo. Questo file lo ha gia' imparato una volta, in
	// `PanelsLeaveTheSceneFree`.
	const int32 Totale = Algo::Accumulate(Conteggio, 0) + FuoriEnum;
	TestTrue(
		*FString::Printf(TEXT("l'albero contiene delle zone da misurare (ne ha %d)"), Totale),
		Totale > 0);

	return true;
}

// =====================================================================================================
// Le zone: stanno dove devono
// =====================================================================================================
//
// 🔑 **`PanelsLeaveTheSceneFree` e' un gate NEGATIVO**: dice che nessuna zona invade la scena, e passerebbe con
// tutte le zone schiacciate in un angolo. Questo dice dove sono.
//
// 🔑 **I tagli della griglia vengono dal dato delle fasce** (#3661, D-497): a sinistra `Left`, a destra
// `Right`, in alto `Top`, in basso `Bottom`, in unita' Slate. ⌫ *Fino al 2026-10-11 erano frazioni fisse,
// 0,2 e 0,8, e una zona doveva avere anchor stirati per scalare; ora una zona d'angolo e' un punto
// dell'angolo piu' una misura, e la scala dell'interfaccia la porta a ogni risoluzione.*

namespace RTGrigliaZone
{
	/** Il distacco di ogni zona dai bordi della sua fascia: e' quello del dato. */
	constexpr float Margine = RTHudLayout::ZoneMargin;

	/**
	 * La cella attesa di una zona, in unita' Slate, per uno schermo `Schermo`: l'ORACOLO, scritto qui con i tagli
	 * della griglia e non preso da `RTHudLayout::ZoneLayout`, che e' cio' che si misura.
	 */
	RTCenterFree::FRect CellaAttesa(ERTHudZone Zona, const FVector2D& Schermo, const FRTHudBands& Fasce)
	{
		const int32 I = static_cast<int32>(Zona);

		// Colonna: 0 = sinistra, 1 = centro, 2 = destra. Riga: 0 = alto, 1 = mezzo, 2 = basso. `Bottom` copre
		// tutte e tre le colonne ([D-456], #3469).
		static const int32 ColonnaDa[] = { 0, 1, 2,  0, 2,  0 };
		static const int32 ColonnaA[]  = { 0, 1, 2,  0, 2,  2 };
		static const int32 Righe[]     = { 0, 0, 0,  1, 1,  2 };
		static_assert(UE_ARRAY_COUNT(ColonnaDa) == static_cast<int32>(ERTHudZone::Count),
			"`ColonnaDa` deve avere una voce per ogni zona: se l'enum cambia, va aggiornata insieme.");
		static_assert(UE_ARRAY_COUNT(ColonnaA) == static_cast<int32>(ERTHudZone::Count),
			"`ColonnaA` deve avere una voce per ogni zona: se l'enum cambia, va aggiornata insieme.");
		static_assert(UE_ARRAY_COUNT(Righe) == static_cast<int32>(ERTHudZone::Count),
			"`Righe` deve avere una voce per ogni zona: se l'enum cambia, va aggiornata insieme.");

		if (I < 0 || I >= UE_ARRAY_COUNT(ColonnaDa))
		{
			return RTCenterFree::FRect{ -1.f, -1.f, -1.f, -1.f }; // un mismatch rumoroso, non una lettura fuori array
		}

		// 🔑 La colonna sinistra ha il suo taglio (`LeftTop`, #3661): una zona che sta SOLO nella colonna 0 lo usa
		// al posto della fascia alta. `Bottom`, che attraversa tutte le colonne, no.
		const bool bSoloSinistra = ColonnaDa[I] == 0 && ColonnaA[I] == 0;
		const float BordiX[] = { 0.f, Fasce.Left, static_cast<float>(Schermo.X) - Fasce.Right, static_cast<float>(Schermo.X) };
		const float BordiY[] = { 0.f, bSoloSinistra ? Fasce.LeftTop : Fasce.Top,
			static_cast<float>(Schermo.Y) - Fasce.Bottom, static_cast<float>(Schermo.Y) };
		return RTCenterFree::FRect{
			BordiX[ColonnaDa[I]] + Margine,
			BordiY[Righe[I]] + Margine,
			BordiX[ColonnaA[I] + 1] - Margine,
			BordiY[Righe[I] + 1] - Margine };
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTZoneRectanglesMatchTheBandsTest,
	"RefactorTactics.ScreenHud.ZoneRectanglesMatchTheBands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTZoneRectanglesMatchTheBandsTest::RunTest(const FString&)
{
	const UWidgetTree* Originale = RTWidgetAssetTest::LoadWidgetTree(*this, TacticalHudPath,
		TEXT("WBP_RT_TacticalHUD"));
	if (Originale == nullptr)
	{
		return false;
	}

	// Su una COPIA, posata come in partita: e' cio' che rende gli anchor delle zone «coincidenti col dato».
	UWidgetTree* Tree = DuplicateObject<UWidgetTree>(const_cast<UWidgetTree*>(Originale), GetTransientPackage());
	if (!TestNotNull(TEXT("la copia dell'albero"), Tree))
	{
		return false;
	}
	URTTacticalHUDWidget::ApplyBandLayout(Tree, RTHudLayout::Bands());

	// Un'unita' di tolleranza: le geometrie sono float, e un arrotondamento non e' un difetto di layout.
	constexpr float Tolleranza = 1.f;

	for (const RTHudRisoluzioni::FRisoluzione& Ris : RTHudRisoluzioni::Tutte())
	{
		const FVector2D Schermo = RTHudRisoluzioni::InSlate(Ris);
		int32 Misurate = 0;
		TArray<ERTHudZone> Viste;

		Tree->ForEachWidget([this, Tree, &Ris, &Schermo, &Misurate, &Viste, Tolleranza](UWidget* Widget)
		{
			const URTHudZoneWidget* Zona = Cast<URTHudZoneWidget>(Widget);
			if (!Zona)
			{
				return;
			}

			const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot);
			if (!Slot || Slot->Parent != Tree->RootWidget)
			{
				// ⚠️ Figlia DIRETTA del Canvas radice: annidata in un altro contenitore, la sua posizione reale
				// dipenderebbe anche da quello, e ne' questo gate ne' `ApplyBandLayout` lo vedono.
				AddError(FString::Printf(TEXT("la zona `%s` non e' figlia diretta del Canvas radice"), *Widget->GetName()));
				return;
			}

			const RTCenterFree::FRect Atteso = RTGrigliaZone::CellaAttesa(Zona->ZoneId, Schermo, RTHudLayout::Bands());
			const RTCenterFree::FRect Reale = RTCenterFree::RettangoloDellaZona(Slot->GetLayout(),
				static_cast<float>(Schermo.X), static_cast<float>(Schermo.Y));
			++Misurate;
			Viste.Add(Zona->ZoneId);

			AddInfo(FString::Printf(TEXT("  %s %-14s atteso %s   reale %s"), Ris.Nome,
				*URTHudZoneWidget::ZoneName(Zona->ZoneId), *RTCenterFree::Descrivi(Atteso), *RTCenterFree::Descrivi(Reale)));

			const bool bCombacia =
				FMath::IsNearlyEqual(Reale.Left, Atteso.Left, Tolleranza)
				&& FMath::IsNearlyEqual(Reale.Top, Atteso.Top, Tolleranza)
				&& FMath::IsNearlyEqual(Reale.Right, Atteso.Right, Tolleranza)
				&& FMath::IsNearlyEqual(Reale.Bottom, Atteso.Bottom, Tolleranza);
			if (!bCombacia)
			{
				AddError(FString::Printf(TEXT("%s: la zona `%s` non occupa la sua cella delle fasce: atteso %s, reale %s."),
					Ris.Nome, *URTHudZoneWidget::ZoneName(Zona->ZoneId),
					*RTCenterFree::Descrivi(Atteso), *RTCenterFree::Descrivi(Reale)));
			}
		});

		TestTrue(*FString::Printf(TEXT("%s: l'albero contiene delle zone da misurare (%d)"), Ris.Nome, Misurate), Misurate > 0);
		for (int32 I = 0; I < static_cast<int32>(ERTHudZone::Count); ++I)
		{
			TestEqual(*FString::Printf(TEXT("%s: la zona %s compare una volta"), Ris.Nome,
				*URTHudZoneWidget::ZoneName(static_cast<ERTHudZone>(I))),
				Viste.FilterByPredicate([I](ERTHudZone Z) { return static_cast<int32>(Z) == I; }).Num(), 1);
		}
	}

	return true;
}

// =====================================================================================================
// Dentro UNA zona: il bordo riempie la cella, non si dimensiona sul proprio contenuto
// =====================================================================================================
//
// 🔴 **Esiste per un difetto trovato in PIE il 2026-09-12, che nessun gate headless vedeva.** A schermo, al
// posto di otto cornici sulle celle della griglia, comparivano otto **quadratini colorati** negli angoli, e
// i pannelli sembravano «un vecchio HUD» rimasto fuori dalle zone. Non c'era nessun vecchio HUD: erano i
// widget nuovi, gia' nelle posizioni giuste, accanto a bordi che non li circondavano.
//
// 🔑 **La causa e' che a decidere il riempimento e' lo SLOT, non il widget.**
// `UBorder::Horizontal/VerticalAlignment` governa come il Border dispone il proprio **contenuto**; se il
// Border riempia il genitore lo dice `UOverlaySlot`, il cui default e' `HAlign_Left` / `VAlign_Top`. Senza
// `Fill` ogni figlio prende la propria *desired size* — per un `Border` vuoto con `padding 8` fanno
// **16 px**, la misura esatta del quadratino osservato.
//
// ⚠️ **L'ipotesi ovvia era il brush, ed era FALSA**: `Draw As: Border` non disegna il centro. Misurato sul
// sorgente dell'engine — `FSlateElementBatcher::AddBorderElement` costruisce `6 * (3 + 2 + 3)` indici, cioe'
// OTTO quad, e il Middle ne ha due (`SlateCore/Private/Rendering/ElementBatcher.cpp`). Questo gate guarda
// quindi lo slot, non il brush: presidiare il brush avrebbe presidiato un sintomo inesistente.
//
// ⛔ **Cosa NON copre, e va detto perche' il verde non prometta piu' di quanto misura**: che a schermo si
// **veda** una cornice. Questo test legge un allineamento dichiarato nel `.uasset`; la resa resta di
// `PIE-V01-SCREENHUD`. Un `Fill` corretto su un brush trasparente passerebbe di qui.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTZoneFillsItsCellTest,
	"RefactorTactics.ScreenHud.TheZoneFillsItsCellInsteadOfItsContent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTZoneFillsItsCellTest::RunTest(const FString&)
{
	const UWidgetTree* Tree = RTWidgetAssetTest::LoadWidgetTree(*this, HudZonePath, TEXT("WBP_RT_HudZone"));
	if (Tree == nullptr)
	{
		return false;
	}

	int32 Esaminati = 0;

	Tree->ForEachWidget([&Esaminati, this](UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}

		// Solo i figli dell'`Overlay` radice: sono quelli la cui geometria decide se la zona e' una cornice
		// sulla cella o un francobollo nell'angolo. Gli altri slot non rispondono a questa domanda.
		const UOverlaySlot* Slot = Cast<UOverlaySlot>(Widget->Slot);
		if (!Slot)
		{
			return;
		}

		++Esaminati;

		AddInfo(FString::Printf(TEXT("  %-12s HAlign=%d VAlign=%d"),
			*Widget->GetName(),
			static_cast<int32>(Slot->GetHorizontalAlignment()),
			static_cast<int32>(Slot->GetVerticalAlignment())));

		if (Slot->GetHorizontalAlignment() != HAlign_Fill)
		{
			AddError(FString::Printf(
				TEXT("`%s` non ha `HAlign_Fill` nel suo `UOverlaySlot`: prende la propria larghezza ")
				TEXT("desiderata invece della cella. E' il difetto visto in PIE il 2026-09-12 — un bordo ")
				TEXT("da `padding 8` diventa un quadratino di 16 px nell'angolo. L'allineamento va sullo ")
				TEXT("SLOT: quello sul `UBorder` governa il suo contenuto, non lui."),
				*Widget->GetName()));
		}

		if (Slot->GetVerticalAlignment() != VAlign_Fill)
		{
			AddError(FString::Printf(
				TEXT("`%s` non ha `VAlign_Fill` nel suo `UOverlaySlot`: stesso difetto dell'asse ")
				TEXT("orizzontale, sull'altro asse."),
				*Widget->GetName()));
		}
	});

	// 🔴 **Senza questa riga il test sarebbe verde su un albero SENZA `UOverlaySlot`** — cioe' misurando
	// zero, il modo in cui un gate diventa decorativo. E' la stessa controprova che gli altri tre gate
	// dell'albero portano, per la stessa ragione.
	TestTrue(
		*FString::Printf(TEXT("la zona contiene degli `UOverlaySlot` da misurare (ne ha %d)"), Esaminati),
		Esaminati > 0);

	return true;
}

/**
 * ⛔ **I WBP della barra dei comandi dichiarano i nomi che il C++ accende, con la classe giusta** (#3489). Dalle
 * sedute T8 ci sono anche `Conferma` (#3633), la finestra di reazione e la sua opzione (#3634), l'header (#3635), e
 * carta, roster, pannello e voce d'avviso (#3636), e il registro con la sua riga (#3637).
 *
 * 🔴 **Esiste perche' un nome sbagliato non da' errore.** I membri sono `BindWidgetOptional`: un widget
 * chiamato `InvalidMarker` invece di `InvalidMark` compila, si carica, e resta spento per sempre — lo slot non
 * direbbe mai «rifiutato», e nessun altro test lo vedrebbe, perche' i test headless iniettano i widget a mano.
 *
 * 🔑 **I nomi dello slot vengono dalla tabella che `RefreshLook` applica** — `IndicatorNameFor`, uno per stato —
 * e non da un elenco scritto qui: il gate e il codice leggono lo stesso fatto. Gli altri sono i membri
 * `BindWidgetOptional` dichiarati in `RTScreenHudWidgets.h`.
 *
 * ⚠️ **La classe e' parte del contratto**: un `PhaseStrip` che fosse un `Image` invece di un `Border` porta il
 * nome giusto e il binding lo scarta in silenzio, perche' il tipo del membro non combacia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionBarDeclaresNamedPortsTest,
	"RefactorTactics.ScreenHud.ActionBarDeclaresTheNamedPorts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTActionBarDeclaresNamedPortsTest::RunTest(const FString&)
{
	struct FPorta
	{
		FName Nome;
		UClass* Classe;
	};

	// Lo slot: gli indicatori dalla tabella di `RefreshLook`, piu' striscia, etichetta e bordo.
	TArray<FPorta> Slot = {
		{ TEXT("PhaseStrip"), UBorder::StaticClass() },
		{ TEXT("PhaseLabelText"), UTextBlock::StaticClass() },
		{ TEXT("StateFrame"), UBorder::StaticClass() },
		// #3498, seduta U65: la resa del mockup.
		{ TEXT("HotkeyBadge"), UWidget::StaticClass() },
		{ TEXT("HotkeyText"), UTextBlock::StaticClass() },
		{ TEXT("ActionNameText"), UTextBlock::StaticClass() },
		{ TEXT("GroupHeaderText"), UTextBlock::StaticClass() },
		{ TEXT("SelectedGlow"), UWidget::StaticClass() },
		{ TEXT("GroupDivider"), UWidget::StaticClass() },
	};
	const UEnum* Stati = StaticEnum<ERTActionSlotState>();
	for (int32 i = 0; i < Stati->NumEnums() - 1; ++i)
	{
		const FName Nome = URTActionSlotWidget::IndicatorNameFor(
			static_cast<ERTActionSlotState>(Stati->GetValueByIndex(i)));
		if (!Nome.IsNone())
		{
			Slot.Add({ Nome, UWidget::StaticClass() });
		}
	}

	const TArray<FPorta> Dock = {
		{ TEXT("MovementReadout"), UWidget::StaticClass() },
		{ TEXT("MovementReadoutText"), UTextBlock::StaticClass() },
		{ TEXT("SneakBadge"), UButton::StaticClass() },
		{ TEXT("SneakBadgeText"), UTextBlock::StaticClass() },
	};
	const TArray<FPorta> Commit = {
		{ TEXT("ConfirmButton"), UButton::StaticClass() },
		{ TEXT("ConfirmText"), UTextBlock::StaticClass() },
		{ TEXT("UndoButton"), UButton::StaticClass() },
		{ TEXT("UndoText"), UTextBlock::StaticClass() },
		// La resa della tavola (#3633, seduta U72): le porte di [D-496] e il contatore di [D-494].
		{ TEXT("CommitRoot"), UWidget::StaticClass() },
		{ TEXT("WarningCounter"), UWidget::StaticClass() },
		{ TEXT("CriticalBadge"), UWidget::StaticClass() },
		{ TEXT("CriticalCountText"), UTextBlock::StaticClass() },
		{ TEXT("WarningBadge"), UWidget::StaticClass() },
		{ TEXT("WarningCountText"), UTextBlock::StaticClass() },
		{ TEXT("InfoBadge"), UWidget::StaticClass() },
		{ TEXT("InfoCountText"), UTextBlock::StaticClass() },
		{ TEXT("ConfirmFrame"), UBorder::StaticClass() },
		{ TEXT("UndoFrame"), UBorder::StaticClass() },
		{ TEXT("ConfirmIcon"), UTextBlock::StaticClass() },
		{ TEXT("ConfirmKeyText"), UTextBlock::StaticClass() },
		{ TEXT("UndoKeyText"), UTextBlock::StaticClass() },
	};

	// La finestra di reazione come la tavola E (#3634, seduta U74). I tre binding obbligatori li controlla
	// `FastDecisionBindingsAreWiredToTheRightNodes`: qui solo le porte della resa.
	const TArray<FPorta> Finestra = {
		{ TEXT("WindowFrame"), UBorder::StaticClass() },
		{ TEXT("AccentStrip"), UBorder::StaticClass() },
		{ TEXT("CountdownBar"), UProgressBar::StaticClass() },
		{ TEXT("SafeOutcomeText"), UTextBlock::StaticClass() },
	};
	const TArray<FPorta> Opzione = {
		{ TEXT("OptionFrame"), UBorder::StaticClass() },
		{ TEXT("KeyText"), UTextBlock::StaticClass() },
		{ TEXT("DescriptionText"), UTextBlock::StaticClass() },
		{ TEXT("SafeChoiceBadge"), UWidget::StaticClass() },
	};

	// L'header e l'obiettivo come le tavole A e D (#3635, seduta U75). Il round non ha piu' un binding di proprieta':
	// lo scrive `RefreshLook` in queste porte, e `RoundLimitComesFromTheFormat` lo legge da li'.
	TArray<FPorta> Header = {
		{ TEXT("HeaderFrame"), UBorder::StaticClass() },
		{ TEXT("RoundValueText"), UTextBlock::StaticClass() },
		{ TEXT("RoundLimitText"), UTextBlock::StaticClass() },
		{ TEXT("PlanningGroup"), UWidget::StaticClass() },
		{ TEXT("ResolutionGroup"), UWidget::StaticClass() },
		{ TEXT("PhaseText"), UTextBlock::StaticClass() },
		{ TEXT("PhaseMark"), UBorder::StaticClass() },
		{ TEXT("TimerText"), UTextBlock::StaticClass() },
		{ TEXT("ObjectiveBox"), UWidget::StaticClass() },
		{ TEXT("ObjectiveScoreText"), UTextBlock::StaticClass() },
		{ TEXT("ObjectiveThresholdText"), UTextBlock::StaticClass() },
	};
	for (const TCHAR* Fase : { TEXT("Prep"), TEXT("Dash"), TEXT("Blast"), TEXT("Move") })
	{
		Header.Add({ FName(*FString::Printf(TEXT("PhaseCell_%s"), Fase)), UBorder::StaticClass() });
		Header.Add({ FName(*FString::Printf(TEXT("PhaseCellMark_%s"), Fase)), UBorder::StaticClass() });
		Header.Add({ FName(*FString::Printf(TEXT("PhaseCellText_%s"), Fase)), UTextBlock::StaticClass() });
	}

	// Carta, roster, pannello e voce d'avviso come la tavola (#3636, seduta U76).
	const TArray<FPorta> Carta = {
		{ TEXT("CardFrame"), UWidget::StaticClass() },
		{ TEXT("TeamDot"), UBorder::StaticClass() },
		{ TEXT("NameText"), UTextBlock::StaticClass() },
		{ TEXT("ReactionChip"), UWidget::StaticClass() },
		{ TEXT("HealthBar"), UProgressBar::StaticClass() },
		{ TEXT("HealthText"), UTextBlock::StaticClass() },
		{ TEXT("ShieldText"), UTextBlock::StaticClass() },
	};
	const TArray<FPorta> Roster = {
		{ TEXT("CardsBox"), UPanelWidget::StaticClass() },
		{ TEXT("OpposingCardsBox"), UPanelWidget::StaticClass() },
	};
	TArray<FPorta> Pannello = {
		{ TEXT("SubjectCard"), URTUnitCardWidget::StaticClass() },
		{ TEXT("PlanChipsBox"), UWidget::StaticClass() },
		{ TEXT("PlanWarningsBox"), UPanelWidget::StaticClass() },
	};
	for (const TCHAR* Chip : { TEXT("Main"), TEXT("Movement"), TEXT("Reaction") })
	{
		Pannello.Add({ FName(*FString::Printf(TEXT("%sChipFrame"), Chip)), UBorder::StaticClass() });
		Pannello.Add({ FName(*FString::Printf(TEXT("%sChipStrip"), Chip)), UBorder::StaticClass() });
		Pannello.Add({ FName(*FString::Printf(TEXT("%sChipValueText"), Chip)), UTextBlock::StaticClass() });
		Pannello.Add({ FName(*FString::Printf(TEXT("%sChipAlert"), Chip)), UWidget::StaticClass() });
	}
	const TArray<FPorta> VoceAvviso = {
		{ TEXT("EntryFrame"), UBorder::StaticClass() },
		{ TEXT("LevelGlyphText"), UTextBlock::StaticClass() },
		{ TEXT("LevelMarkText"), UTextBlock::StaticClass() },
		{ TEXT("LevelText"), UTextBlock::StaticClass() },
		{ TEXT("WhatText"), UTextBlock::StaticClass() },
		{ TEXT("WhyText"), UTextBlock::StaticClass() },
		{ TEXT("CostText"), UTextBlock::StaticClass() },
	};

	// Il registro e la sua riga come la tavola D (#3637, seduta U77).
	const TArray<FPorta> Registro = {
		{ TEXT("LogRoot"), UWidget::StaticClass() },
		{ TEXT("LogTurnText"), UTextBlock::StaticClass() },
		{ TEXT("LinesBox"), UPanelWidget::StaticClass() },
	};
	const TArray<FPorta> RigaRegistro = {
		{ TEXT("PhaseStrip"), UBorder::StaticClass() },
		{ TEXT("PhaseLabelText"), UTextBlock::StaticClass() },
		{ TEXT("SequenceText"), UTextBlock::StaticClass() },
		{ TEXT("ReactionBadge"), UWidget::StaticClass() },
		{ TEXT("LineText"), UTextBlock::StaticClass() },
	};

	struct FAsset
	{
		const TCHAR* Path;
		const TCHAR* Label;
		const TArray<FPorta>* Porte;
	};
	const FAsset Asset[] = {
		{ ActionSlotPath, TEXT("WBP_RT_ActionSlot"), &Slot },
		{ ActionDockPath, TEXT("WBP_RT_ActionDock"), &Dock },
		{ PlanCommitPath, TEXT("WBP_RT_PlanCommit"), &Commit },
		{ FastDecisionPath, TEXT("WBP_RT_FastDecision"), &Finestra },
		{ FastDecisionOptionPath, TEXT("WBP_RT_FastDecisionOption"), &Opzione },
		{ TurnHeaderPath, TEXT("WBP_RT_TurnHeader"), &Header },
		{ UnitCardPath, TEXT("WBP_RT_UnitCard"), &Carta },
		{ TeamRosterPath, TEXT("WBP_RT_TeamRoster"), &Roster },
		{ SelectedUnitPath, TEXT("WBP_RT_SelectedUnitPanel"), &Pannello },
		{ PlanWarningEntryPath, TEXT("WBP_RT_PlanWarningEntry"), &VoceAvviso },
		{ EventLogPath, TEXT("WBP_RT_EventLog"), &Registro },
		{ EventLinePath, TEXT("WBP_RT_EventLine"), &RigaRegistro },
	};

	for (const FAsset& A : Asset)
	{
		const UWidgetTree* Tree = RTWidgetAssetTest::LoadWidgetTree(*this, A.Path, A.Label);
		if (Tree == nullptr)
		{
			continue;
		}
		for (const FPorta& P : *A.Porte)
		{
			const UWidget* W = Tree->FindWidget(P.Nome);
			if (!TestNotNull(*FString::Printf(TEXT("%s dichiara '%s'"), A.Label, *P.Nome.ToString()), W))
			{
				continue;
			}
			TestTrue(*FString::Printf(TEXT("%s: '%s' e' un %s (e' un %s)"), A.Label, *P.Nome.ToString(),
				*P.Classe->GetName(), *W->GetClass()->GetName()), W->IsA(P.Classe));
		}
	}
	return true;
}

/**
 * **LO SLOT INDOSSA IL TOOLTIP, E IL TOOLTIP DICHIARA LE PORTE CHE IL C++ SCRIVE** — `#3499`, seduta `U66`.
 *
 * 🔑 **Due fatti che nessun test headless vede**: che i default di `WBP_RT_ActionSlot` puntino al tooltip — senza,
 * lo slot ripiega sul testo semplice e niente diventa rosso — e che `WBP_RT_ActionTooltip` porti i quattro testi
 * col nome giusto. Un nome sbagliato non da' errore: lascia la porta spenta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionSlotWearsTooltipTest,
	"RefactorTactics.ScreenHud.ActionSlotWearsTheTooltipAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTActionSlotWearsTooltipTest::RunTest(const FString&)
{
	if (const UWidgetTree* Tree = RTWidgetAssetTest::LoadWidgetTree(*this, ActionTooltipPath, TEXT("WBP_RT_ActionTooltip")))
	{
		for (const TCHAR* Nome : { TEXT("TitleText"), TEXT("DescriptionText"), TEXT("LinesText"), TEXT("ReasonText") })
		{
			const UWidget* W = Tree->FindWidget(Nome);
			if (TestNotNull(*FString::Printf(TEXT("WBP_RT_ActionTooltip dichiara '%s'"), Nome), W))
			{
				TestTrue(*FString::Printf(TEXT("'%s' e' un TextBlock"), Nome), W->IsA(UTextBlock::StaticClass()));
			}
		}
	}

	UWidgetBlueprintGeneratedClass* Tooltip = RTWidgetAssetTest::LoadWidgetClass(ActionTooltipPath);
	UWidgetBlueprintGeneratedClass* Slot = RTWidgetAssetTest::LoadWidgetClass(ActionSlotPath);
	if (!TestNotNull(TEXT("il tooltip si carica"), Tooltip) || !TestNotNull(TEXT("lo slot si carica"), Slot))
	{
		return false;
	}
	TestTrue(TEXT("il tooltip deriva dalla classe nativa che ne scrive le porte"),
		Tooltip->IsChildOf(URTActionTooltipWidget::StaticClass()));
	const URTActionSlotWidget* Default = Slot->GetDefaultObject<URTActionSlotWidget>();
	if (TestNotNull(TEXT("i default dello slot"), Default))
	{
		TestTrue(TEXT("i default di WBP_RT_ActionSlot puntano a WBP_RT_ActionTooltip"),
			Default->TooltipClass.Get() == Tooltip);
	}
	return true;
}

/**
 * ⛔ **Le zone il cui contenuto e' arrivato non mostrano piu' il bordo da cantiere** (#3494).
 *
 * 🔑 **E' il contratto di `bBlockoutVisible`, scritto sulla classe**: *«Si spegne quando il contenuto della zona
 * arriva»*. Per `Zone_Bottom` e' arrivato con la barra dei comandi (seduta `U63`), per `Zone_TopRight` con
 * Conferma e Annulla (`U64`), e i due bordi erano rimasti accesi: l'autore ha visto la fascia bassa col bordo viola.
 *
 * ⚠️ **Elenca solo le zone gia' arrivate**, non pretende che le altre restino accese: la prossima zona che riceve
 * il proprio contenuto si aggiunge qui, nella stessa PR dell'asset. Il bordo lo spegne il `PreConstruct` di
 * `WBP_RT_HudZone` leggendo questa proprieta' dell'istanza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArrivedZonesHideBlockoutTest,
	"RefactorTactics.ScreenHud.ZonesWithTheirContentHideTheBlockout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTArrivedZonesHideBlockoutTest::RunTest(const FString&)
{
	const UWidgetTree* Tree = RTWidgetAssetTest::LoadWidgetTree(*this, TacticalHudPath, TEXT("WBP_RT_TacticalHUD"));
	if (Tree == nullptr)
	{
		return false;
	}

	// `TopCenter` dalla seduta `U75` (#3635), che l'aveva dimenticata; `TopLeft` e `MiddleLeft` dalla `U76` (#3636);
	// `MiddleRight`, il registro, dalla `U77` (#3637). Ora ci sono tutte.
	const ERTHudZone Arrivate[] = { ERTHudZone::Bottom, ERTHudZone::TopRight, ERTHudZone::TopCenter,
		ERTHudZone::TopLeft, ERTHudZone::MiddleLeft, ERTHudZone::MiddleRight };
	for (const ERTHudZone Zona : Arrivate)
	{
		const FString Nome = URTHudZoneWidget::ZoneName(Zona);
		int32 Trovate = 0;
		Tree->ForEachWidget([this, Zona, &Nome, &Trovate](UWidget* Widget)
		{
			const URTHudZoneWidget* Istanza = Cast<URTHudZoneWidget>(Widget);
			if (Istanza && Istanza->ZoneId == Zona)
			{
				++Trovate;
				TestFalse(*FString::Printf(TEXT("%s ha il proprio contenuto: il bordo da cantiere e' spento"), *Nome),
					Istanza->bBlockoutVisible);
			}
		});
		TestEqual(*FString::Printf(TEXT("premessa: l'albero ha una zona %s"), *Nome), Trovate, 1);
	}
	return true;
}

/**
 * 🔑 **IL ROSTER E IL PANNELLO PORTANO LE CLASSI CHE IL C++ ISTANZIA, ANCHE DENTRO `WBP_RT_TacticalHUD`** (#3636, seduta `U76`).
 *
 * 🔴 **La seconda meta' e' quella che conta.** I default si scrivono sul CDO del Blueprint, ma il roster e il pannello che
 * si vedono sono le istanze dell'albero del TacticalHUD. Nella seduta, un TacticalHUD caricato PRIMA del cambio di default
 * teneva in memoria `CardClass` nullo, e il roster usciva vuoto. Salvarlo in quello stato avrebbe scritto il nullo come
 * override, per sempre. Questo test vede l'override SALVATO: in un processo fresco l'istanza deve risolvere alla stessa
 * classe del CDO. ⚠️ Lo stato solo in memoria non si riproduce qui, e si vede solo a schermo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRosterAndPanelWearTheirClassesTest,
	"RefactorTactics.ScreenHud.RosterAndPanelWearTheirClasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTRosterAndPanelWearTheirClassesTest::RunTest(const FString&)
{
	UWidgetBlueprintGeneratedClass* Carta = RTWidgetAssetTest::LoadWidgetClass(UnitCardPath);
	UWidgetBlueprintGeneratedClass* Voce = RTWidgetAssetTest::LoadWidgetClass(PlanWarningEntryPath);
	UWidgetBlueprintGeneratedClass* Roster = RTWidgetAssetTest::LoadWidgetClass(TeamRosterPath);
	UWidgetBlueprintGeneratedClass* Pannello = RTWidgetAssetTest::LoadWidgetClass(SelectedUnitPath);
	if (!TestNotNull(TEXT("la carta si carica"), Carta) || !TestNotNull(TEXT("la voce d'avviso si carica"), Voce)
		|| !TestNotNull(TEXT("il roster si carica"), Roster) || !TestNotNull(TEXT("il pannello si carica"), Pannello))
	{
		return false;
	}

	// --- 1. I default delle due classi -----------------------------------------------------------------------------------
	TestTrue(TEXT("1: i default di WBP_RT_TeamRoster puntano a WBP_RT_UnitCard"),
		Roster->GetDefaultObject<URTTeamRosterWidget>()->CardClass.Get() == Carta);
	TestTrue(TEXT("1: i default di WBP_RT_SelectedUnitPanel puntano a WBP_RT_PlanWarningEntry"),
		Pannello->GetDefaultObject<URTSelectedUnitPanelWidget>()->PlanWarningEntryClass.Get() == Voce);

	// --- 2. Le istanze dell'albero del TacticalHUD, che sono quelle a schermo ---------------------------------------------------
	const UWidgetTree* Tree = RTWidgetAssetTest::LoadWidgetTree(*this, TacticalHudPath, TEXT("WBP_RT_TacticalHUD"));
	if (Tree == nullptr)
	{
		return false;
	}
	int32 Rosters = 0;
	int32 Pannelli = 0;
	Tree->ForEachWidget([&](UWidget* Widget)
	{
		if (const URTTeamRosterWidget* R = Cast<URTTeamRosterWidget>(Widget))
		{
			++Rosters;
			TestTrue(*FString::Printf(TEXT("2: il roster '%s' del TacticalHUD costruisce WBP_RT_UnitCard"), *R->GetName()),
				R->CardClass.Get() == Carta);
		}
		if (const URTSelectedUnitPanelWidget* P = Cast<URTSelectedUnitPanelWidget>(Widget))
		{
			++Pannelli;
			TestTrue(*FString::Printf(TEXT("2: il pannello '%s' del TacticalHUD costruisce WBP_RT_PlanWarningEntry"), *P->GetName()),
				P->PlanWarningEntryClass.Get() == Voce);
		}
	});
	TestEqual(TEXT("2: premessa: il TacticalHUD monta un roster"), Rosters, 1);
	TestEqual(TEXT("2: premessa: il TacticalHUD monta un pannello"), Pannelli, 1);
	return true;
}

/**
 * 🔑 **IL REGISTRO PORTA LA CLASSE DELLE RIGHE, ANCHE DENTRO `WBP_RT_TacticalHUD`** (#3637, seduta `U77`).
 *
 * Il gemello di `RosterAndPanelWearTheirClasses`, per lo stesso motivo: `LineClass` sta sul CDO, ma il registro a
 * schermo e' l'istanza dell'albero del TacticalHUD, e un override nullo salvato li' lo lascerebbe senza righe. ⚠️ Vede
 * l'override SALVATO, non uno stato solo in memoria.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEventLogWearsItsLineClassTest,
	"RefactorTactics.ScreenHud.EventLogWearsItsLineClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTEventLogWearsItsLineClassTest::RunTest(const FString&)
{
	UWidgetBlueprintGeneratedClass* Riga = RTWidgetAssetTest::LoadWidgetClass(EventLinePath);
	UWidgetBlueprintGeneratedClass* Registro = RTWidgetAssetTest::LoadWidgetClass(EventLogPath);
	if (!TestNotNull(TEXT("la riga si carica"), Riga) || !TestNotNull(TEXT("il registro si carica"), Registro))
	{
		return false;
	}
	TestTrue(TEXT("i default di WBP_RT_EventLog puntano a WBP_RT_EventLine"),
		Registro->GetDefaultObject<URTPlayerEventLogWidget>()->LineClass.Get() == Riga);

	const UWidgetTree* Tree = RTWidgetAssetTest::LoadWidgetTree(*this, TacticalHudPath, TEXT("WBP_RT_TacticalHUD"));
	if (Tree == nullptr)
	{
		return false;
	}
	int32 Registri = 0;
	Tree->ForEachWidget([&](UWidget* Widget)
	{
		if (const URTPlayerEventLogWidget* L = Cast<URTPlayerEventLogWidget>(Widget))
		{
			++Registri;
			TestTrue(*FString::Printf(TEXT("il registro '%s' del TacticalHUD costruisce WBP_RT_EventLine"), *L->GetName()),
				L->LineClass.Get() == Riga);
		}
	});
	TestEqual(TEXT("premessa: il TacticalHUD monta un registro"), Registri, 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
