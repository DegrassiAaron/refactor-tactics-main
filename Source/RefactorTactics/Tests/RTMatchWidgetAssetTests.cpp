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
		ActionDockPath, ActionSlotPath, UnitCardPath, FastDecisionPath
	};
	const TCHAR* const Labels[] = {
		TEXT("TacticalHUD"), TEXT("TurnHeader"), TEXT("TeamRoster"), TEXT("SelectedUnitPanel"),
		TEXT("ActionDock"), TEXT("ActionSlot"), TEXT("UnitCard"), TEXT("FastDecision")
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
		{ FastDecisionPath, TEXT("FastDecision"),      URTFastDecisionWidget::StaticClass() },
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
		ActionDockPath, ActionSlotPath, UnitCardPath, FastDecisionPath
	};
	const TCHAR* const Labels[] = {
		TEXT("TacticalHUD"), TEXT("TurnHeader"), TEXT("TeamRoster"), TEXT("SelectedUnitPanel"),
		TEXT("ActionDock"), TEXT("ActionSlot"), TEXT("UnitCard"), TEXT("FastDecision")
	};

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
	UWidgetBlueprintGeneratedClass* Class = RTWidgetAssetTest::LoadWidgetClass(TacticalHudPath);
	if (!TestNotNull(TEXT("WBP_RT_TacticalHUD si carica"), Class))
	{
		return false;
	}

	const UWidgetTree* Tree = Class->GetWidgetTreeArchetype();
	if (!TestNotNull(TEXT("WBP_RT_TacticalHUD ha un albero di widget"), Tree))
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

#endif // WITH_DEV_AUTOMATION_TESTS
