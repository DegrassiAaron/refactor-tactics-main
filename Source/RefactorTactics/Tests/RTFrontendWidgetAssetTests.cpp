// CP 46.2 (#937) — i binding dichiarati DENTRO i `WBP_RT_*`.
//
// ⚠️ **Questi test provano il `.uasset`, non il C++.** `RTStartupReportTests.cpp` prova gia' che
// `GetModalVisibility()` restituisce `Visible` quando il modale e' armato; questo file prova la meta'
// che mancava — che quel valore **arrivi a un widget**, e al widget giusto.
//
// Esistono per un difetto misurato, non per completezza: il modale si armava (`IsArmed() == true`,
// verificato in PIE) e non compariva a schermo. Fra le due cose c'e' un `FDelegateRuntimeBinding`
// serializzato nel binario, che nessun test guardava e che nessuno puo' leggere a occhio — un `.uasset`
// non si diffa e non si grep-pa. La differenza fra «il codice e' giusto» e «la schermata si vede» stava
// tutta li'.
//
// Il banner e' nel file come **caso di controllo**: e' l'unico dei due che funziona in PIE, quindi la sua
// forma e' l'unica evidenza disponibile di come debba essere fatto un binding che arriva a schermo. Non
// asserisce, stampa: cio' che il banner fa e' un fatto da leggere, non ancora una regola da imporre.
//
// ⚠️ Cio' che questi test **non** coprono: l'aspetto. Colori, font, posizione e leggibilita' restano di
// `PIE-V01-FRONTEND-ERROR`. Un binding corretto su un widget largo zero pixel passerebbe di qui — ed e'
// esattamente perche' quel caso e' possibile che il test stampa anche la geometria dello slot.

#include "Misc/AutomationTest.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelSlot.h"
#include "Components/Widget.h"
#include "Frontend/RTFrontendNavigator.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const TCHAR* const ErrorModalPath = TEXT("/Game/RT/UI/Framework/WBP_RT_ErrorModal.WBP_RT_ErrorModal_C");
	const TCHAR* const FallbackBannerPath = TEXT("/Game/RT/UI/Framework/WBP_RT_FallbackBanner.WBP_RT_FallbackBanner_C");
	const TCHAR* const LoadingScreenPath = TEXT("/Game/RT/UI/Framework/WBP_RT_LoadingScreen.WBP_RT_LoadingScreen_C");
	const TCHAR* const MainMenuPath = TEXT("/Game/RT/UI/Framework/WBP_RT_MainMenu.WBP_RT_MainMenu_C");

	/**
	 * Carica la generated class di un `WBP_*`. `nullptr` se l'asset non c'e': il chiamante lo dichiara
	 * fallimento con un messaggio che nomina il path, perche' «cast fallito» non direbbe quale asset.
	 */
	UWidgetBlueprintGeneratedClass* LoadWidgetClass(const TCHAR* Path)
	{
		return Cast<UWidgetBlueprintGeneratedClass>(
			StaticLoadObject(UWidgetBlueprintGeneratedClass::StaticClass(), nullptr, Path));
	}

	/** Il testo di un binding, nella forma in cui serve leggerlo in un log di automation. */
	FString DescribeBinding(const FDelegateRuntimeBinding& Binding)
	{
		return FString::Printf(TEXT("  %s.%s <- %s()  [Kind=%s]"),
			*Binding.ObjectName,
			*Binding.PropertyName.ToString(),
			*Binding.FunctionName.ToString(),
			Binding.Kind == EBindingKind::Function ? TEXT("Function") : TEXT("Property"));
	}

	/**
	 * Una riga per widget: nome, classe, visibilita' di design e — se il widget vive in un Canvas — il
	 * rettangolo che occupa.
	 *
	 * ⚠️ La geometria e' qui perche' **`Visible` e area nulla producono lo stesso sintomo di `Collapsed`**:
	 * il binding e' corretto, la funzione restituisce il valore giusto, e a schermo non appare niente. Senza
	 * questi numeri i due casi non si distinguono se non aprendo l'editor.
	 */
	FString DescribeWidget(const UWidget* Widget)
	{
		if (!Widget)
		{
			return TEXT("  <nullptr>");
		}

		FString Line = FString::Printf(TEXT("  %-20s %-22s Visibility=%s"),
			*Widget->GetName(),
			*Widget->GetClass()->GetName(),
			*StaticEnum<ESlateVisibility>()->GetNameStringByValue(static_cast<int64>(Widget->GetVisibility())));

		if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
		{
			const FAnchorData Layout = CanvasSlot->GetLayout();
			Line += FString::Printf(
				TEXT("  Anchors=(%.2f,%.2f)-(%.2f,%.2f) Offsets=(L%.1f T%.1f R%.1f B%.1f) Alignment=(%.2f,%.2f)"),
				Layout.Anchors.Minimum.X, Layout.Anchors.Minimum.Y,
				Layout.Anchors.Maximum.X, Layout.Anchors.Maximum.Y,
				Layout.Offsets.Left, Layout.Offsets.Top, Layout.Offsets.Right, Layout.Offsets.Bottom,
				Layout.Alignment.X, Layout.Alignment.Y);
		}
		else if (Widget->Slot)
		{
			Line += FString::Printf(TEXT("  Slot=%s"), *Widget->Slot->GetClass()->GetName());
		}
		else
		{
			Line += TEXT("  Slot=<radice>");
		}

		return Line;
	}

	/**
	 * I widget che **restano a schermo** quando il widget bindato viene spento.
	 *
	 * Spegnere un widget spegne lui e i suoi discendenti. Tutto il resto dell'albero non lo sente: i suoi
	 * antenati (che lo contengono, e devono restare per contenerlo) e chiunque altro — fratelli, e
	 * discendenti dei fratelli. Sono questi ultimi la risposta che serve: se non e' vuota, il binding di
	 * visibilita' governa **una parte** della schermata invece della schermata.
	 *
	 * Gli antenati sono esclusi di proposito: il `CanvasPanel` radice di un `UserWidget` e' un contenitore
	 * senza pixel propri, e pretenderlo spento vieterebbe la forma che il banner usa e che funziona.
	 */
	TArray<FString> WidgetsLeftBehind(const UWidgetTree* Tree, UWidget* Bound)
	{
		TSet<const UWidget*> Governed;
		UWidgetTree::ForWidgetAndChildren(Bound, [&Governed](UWidget* Widget)
		{
			Governed.Add(Widget);
		});
		Governed.Add(Bound);

		for (const UWidget* Ancestor = Bound->GetParent(); Ancestor; Ancestor = Ancestor->GetParent())
		{
			Governed.Add(Ancestor);
		}

		TArray<FString> LeftBehind;
		Tree->ForEachWidget([&Governed, &LeftBehind](UWidget* Widget)
		{
			if (!Governed.Contains(Widget))
			{
				LeftBehind.Add(FString::Printf(TEXT("%s (%s)"),
					*Widget->GetName(), *Widget->GetClass()->GetName()));
			}
		});

		return LeftBehind;
	}

	/** Versa nel log del test i binding e l'albero. E' l'evidenza; le asserzioni vengono dopo. */
	void ReportAsset(FAutomationTestBase& Test, const UWidgetBlueprintGeneratedClass* Class, const TCHAR* Label)
	{
		Test.AddInfo(FString::Printf(TEXT("=== %s: %d binding ==="), Label, Class->Bindings.Num()));
		for (const FDelegateRuntimeBinding& Binding : Class->Bindings)
		{
			Test.AddInfo(DescribeBinding(Binding));
		}

		const UWidgetTree* Tree = Class->GetWidgetTreeArchetype();
		if (!Tree)
		{
			Test.AddInfo(TEXT("=== albero: assente ==="));
			return;
		}

		Test.AddInfo(FString::Printf(TEXT("=== %s: albero (radice = %s) ==="),
			Label, Tree->RootWidget ? *Tree->RootWidget->GetName() : TEXT("<nessuna>")));

		Tree->ForEachWidget([&Test](UWidget* Widget)
		{
			Test.AddInfo(DescribeWidget(Widget));
		});
	}

	/**
	 * L'invariante che le tre schermate del frontend condividono: **spegnere il widget bindato spegne la
	 * schermata**.
	 *
	 * ⚠️ **Sta in una funzione sola perche' il difetto e' ripetibile, non perche' il codice si ripeteva.**
	 * `WBP_RT_ErrorModal` ha sbagliato esattamente qui, e il terzo widget si costruisce con gli stessi gesti
	 * in editor — trascinare nel posto sbagliato costa un secondo e non produce nessun errore. Una regola
	 * scritta tre volte e' una regola che la terza volta si dimentica.
	 *
	 * Carica, versa l'evidenza nel log, e riporta se l'invariante regge. `false` anche quando l'asset non
	 * esiste: un widget assente non e' «niente da verificare», e' la verifica che fallisce.
	 */
	bool VisibilityGovernsWholeWidget(
		FAutomationTestBase& Test, const TCHAR* AssetPath, const TCHAR* Label, const TCHAR* VisibilityFunction)
	{
		UWidgetBlueprintGeneratedClass* Class = LoadWidgetClass(AssetPath);
		if (!Class)
		{
			Test.AddError(FString::Printf(TEXT("%s non si carica da %s"), Label, AssetPath));
			return false;
		}

		ReportAsset(Test, Class, Label);

		const UWidgetTree* Tree = Class->GetWidgetTreeArchetype();
		if (!Tree || !Tree->RootWidget)
		{
			Test.AddError(FString::Printf(
				TEXT("%s non ha un widget radice: non c'e' niente su cui il binding possa stare"), Label));
			return false;
		}

		const FName Wanted(VisibilityFunction);
		const FDelegateRuntimeBinding* Binding = Class->Bindings.FindByPredicate(
			[&Wanted](const FDelegateRuntimeBinding& Candidate)
			{
				return Candidate.PropertyName == TEXT("Visibility") && Candidate.FunctionName == Wanted;
			});

		if (!Binding)
		{
			Test.AddError(FString::Printf(
				TEXT("%s: nessun binding Visibility <- %s(): la schermata non puo' mostrarsi da se'"),
				Label, VisibilityFunction));
			return false;
		}

		UWidget* Bound = Tree->FindWidget(FName(*Binding->ObjectName));
		if (!Bound)
		{
			Test.AddError(FString::Printf(
				TEXT("%s: il binding punta a '%s', che non e' nell'albero"), Label, *Binding->ObjectName));
			return false;
		}

		const TArray<FString> LeftBehind = WidgetsLeftBehind(Tree, Bound);
		if (LeftBehind.Num() > 0)
		{
			Test.AddError(FString::Printf(
				TEXT("%s: %s governa solo '%s' — spegnerlo lascia a schermo %d widget: %s. ")
				TEXT("Il binding va spostato sul widget che CONTIENE il contenuto."),
				Label, VisibilityFunction, *Bound->GetName(),
				LeftBehind.Num(), *FString::Join(LeftBehind, TEXT(", "))));
			return false;
		}

		return true;
	}
}

/**
 * Il binding di `GetModalVisibility` governa **tutto** il modale, non una parte.
 *
 * ⚠️ **Non basta che il binding esista, e non deve stare per forza sulla radice.** Il banner — l'unico dei
 * due verificato a schermo — lo porta su un figlio del proprio `CanvasPanel`, e funziona: la forma «sulla
 * radice» era una previsione sbagliata, confutata dal caso buono prima che diventasse una regola.
 *
 * Cio' che conta e' un'altra cosa: che spegnere il widget bindato spenga la schermata. Un binding su un
 * ramo **laterale** compila, si arma, e lascia a schermo tutto il resto dell'albero — e nessun errore lo
 * dice, perche' dal punto di vista di UMG e' un binding perfettamente valido. E' il difetto che ha bloccato
 * CP 46.2: `Border_0` portava `GetModalVisibility` ed era **fratello** del `VerticalBox` con il testo e i
 * bottoni, non suo genitore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTErrorModalVisibilityGovernsWholeModalTest,
	"RefactorTactics.Frontend.ErrorModalVisibilityGovernsWholeModal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTErrorModalVisibilityGovernsWholeModalTest::RunTest(const FString&)
{
	VisibilityGovernsWholeWidget(
		*this, ErrorModalPath, TEXT("WBP_RT_ErrorModal"), TEXT("GetModalVisibility"));

	// I due controlli qui sotto sono **specifici del modale** e continuano anche se l'invariante e' caduta:
	// un asset rotto in due modi deve dirli entrambi in una run, non uno per volta.
	UWidgetBlueprintGeneratedClass* ModalClass = LoadWidgetClass(ErrorModalPath);
	if (!ModalClass)
	{
		return false;
	}

	const UWidgetTree* Tree = ModalClass->GetWidgetTreeArchetype();
	if (!Tree)
	{
		return false;
	}

	// La causa e' la ragione per cui il modale esiste: un modale che compare senza dire perche' e' una
	// schermata vuota che blocca i click.
	const bool bHasReason = ModalClass->Bindings.ContainsByPredicate(
		[](const FDelegateRuntimeBinding& Binding)
		{
			return Binding.PropertyName == TEXT("Text") && Binding.FunctionName == TEXT("GetReasonText");
		});

	TestTrue(TEXT("esiste il binding Text <- GetReasonText()"), bHasReason);

	// ⚠️ `GetDetailsVisibility` deve spegnere il PULSANTE, non la sua etichetta. Agganciato al `TextBlock`
	// interno lascia in Shipping un bottone vuoto e **cliccabile**: l'utente preme qualcosa che non ha piu'
	// un contenuto da aprire. E' lo stesso difetto di `GetModalVisibility` su un ramo laterale, un livello
	// piu' in basso — ed e' per questo che si prova con la stessa domanda invece che con un `TestEqual` sul
	// nome, che direbbe solo *dove* sta il binding e non *cosa* governa.
	if (const FDelegateRuntimeBinding* DetailsBinding = ModalClass->Bindings.FindByPredicate(
		[](const FDelegateRuntimeBinding& Binding)
		{
			return Binding.PropertyName == TEXT("Visibility")
				&& Binding.FunctionName == TEXT("GetDetailsVisibility");
		}))
	{
		UWidget* DetailsWidget = Tree->FindWidget(FName(*DetailsBinding->ObjectName));
		if (DetailsWidget && !DetailsWidget->IsA<UButton>())
		{
			AddError(FString::Printf(
				TEXT("GetDetailsVisibility governa '%s' (%s): spegnerlo lascia il pulsante DETAILS a schermo, ")
				TEXT("vuoto e cliccabile. Il binding va sul Button, non sulla sua etichetta."),
				*DetailsWidget->GetName(), *DetailsWidget->GetClass()->GetName()));
		}
	}
	else
	{
		AddError(TEXT("nessun binding Visibility <- GetDetailsVisibility(): il pulsante DETAILS esisterebbe "
			"anche dove non c'e' un dettaglio da mostrare"));
	}

	return true;
}

/**
 * Il banner soddisfa la **stessa** invariante del modale: `GetBannerVisibility` governa tutto il banner.
 *
 * ⚠️ **Serve a provare che l'invariante e' soddisfacibile, non solo violabile.** Un controllo che esiste
 * unicamente per fallire su un asset rotto non distingue «regola giusta» da «regola impossibile»: il
 * banner e' l'unico dei due verificato a schermo in PIE (`rt.Map.Source GeneratedTestArena`), quindi e'
 * l'unica prova disponibile che la forma richiesta sia quella che davvero compare.
 *
 * E' anche la ragione per cui l'invariante non nomina la radice: qui il widget bindato e' `ReadLines`, un
 * figlio del `CanvasPanel`, e il banner funziona lo stesso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallbackBannerVisibilityGovernsWholeBannerTest,
	"RefactorTactics.Frontend.FallbackBannerVisibilityGovernsWholeBanner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallbackBannerVisibilityGovernsWholeBannerTest::RunTest(const FString&)
{
	VisibilityGovernsWholeWidget(
		*this, FallbackBannerPath, TEXT("WBP_RT_FallbackBanner"), TEXT("GetBannerVisibility"));

	return true;
}

/**
 * La schermata d'attesa: stessa invariante, **scritta prima che il widget esista**.
 *
 * ⚠️ **Questo test nasce rosso, ed e' voluto.** Per il modale l'ordine e' stato l'inverso — widget
 * costruito, difetto invisibile, test scritto dopo per capire perche' — ed e' costato una diagnosi. Il
 * terzo widget si costruisce con gli stessi gesti e puo' sbagliare nello stesso punto: trascinare un
 * elemento accanto invece che dentro non produce nessun errore in editor.
 *
 * Cio' che il test **non** copre, e va detto perche' e' la parte piu' facile da credere coperta: che la
 * schermata compaia davvero durante un'attesa vera. L'allestimento e' istantaneo, quindi in PIE la fase va
 * forzata con `SetPhase`; e la fase la produce `ARTGameMode`, non il widget. Qui si prova solo che, quando
 * `GetLoadingVisibility()` dice `Collapsed`, non resti niente a schermo.
 *
 * ⚠️ **Nessun controllo sul testo della fase.** `GetPhaseText()` e' gia' provato da
 * `Frontend.EveryWaitingPhaseHasText` sul C++, e ripeterlo qui direbbe due volte la stessa cosa; ma il
 * binding `Text <- GetPhaseText` si controlla, perche' un `TextBlock` con una stringa scritta a mano nel
 * `.uasset` passerebbe entrambi e sarebbe la seconda autorita' sulle fasi — cioe' il difetto che
 * `ERTLoadPhase` esiste per impedire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLoadingScreenVisibilityGovernsWholeScreenTest,
	"RefactorTactics.Frontend.LoadingScreenVisibilityGovernsWholeScreen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLoadingScreenVisibilityGovernsWholeScreenTest::RunTest(const FString&)
{
	if (!VisibilityGovernsWholeWidget(
		*this, LoadingScreenPath, TEXT("WBP_RT_LoadingScreen"), TEXT("GetLoadingVisibility")))
	{
		return false;
	}

	UWidgetBlueprintGeneratedClass* LoadingClass = LoadWidgetClass(LoadingScreenPath);
	if (!LoadingClass)
	{
		return false;
	}

	const bool bReadsPhaseText = LoadingClass->Bindings.ContainsByPredicate(
		[](const FDelegateRuntimeBinding& Binding)
		{
			return Binding.PropertyName == TEXT("Text") && Binding.FunctionName == TEXT("GetPhaseText");
		});

	TestTrue(
		TEXT("la riga della fase si legge da GetPhaseText(), non e' scritta nel widget"),
		bReadsPhaseText);

	return true;
}

namespace
{
	/**
	 * Le `UFunction` dichiarate DA questa classe, non quelle ereditate.
	 *
	 * Il grafo di un `WBP_*` vive qui: l'ubergraph, le funzioni utente, e le `BndEvt__…` che UMG genera
	 * per ogni event dispatcher cablato. Le ereditate da `UUserWidget` sono centinaia e non dicono nulla
	 * su cosa il menu faccia.
	 *
	 * `ExcludeSuper` e non un filtro a valle: e' la forma che il repository usa gia' in
	 * `RTMatchWidgetAssetTests.cpp:110`, e chiedere all'iteratore di non salire costa meno che iterare
	 * l'intera catena per scartarla.
	 */
	TArray<UFunction*> OwnFunctions(UClass* Class)
	{
		TArray<UFunction*> Own;
		for (TFieldIterator<UFunction> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			Own.Add(*It);
		}
		return Own;
	}

	/**
	 * Una chiamata letta dal bytecode: il nome **e la classe che la possiede**.
	 *
	 * ⚠️ **La classe non e' un dettaglio.** Senza, `StartMatch` sarebbe un nome qualunque: un grafo
	 * ricablato su un altro subsystem che esponga una `UFUNCTION` omonima passerebbe il controllo mentre
	 * la catena `menu → StartMatch → ConsumePendingMatchLevel → ARTGameMode` non viene imboccata.
	 */
	struct FCalledFunction
	{
		FString Name;
		const UClass* Owner = nullptr;
	};

	/**
	 * Le funzioni che il bytecode di `Fn` chiama.
	 *
	 * ⚠️ **Legge `ScriptAndPropertyObjectReferences`, non il bytecode.** UE tiene li' gli oggetti
	 * referenziati dallo script perche' il garbage collector li veda, ed e' l'unico accesso pubblico a
	 * «cosa chiama questo grafo» da un modulo **Runtime** — che e' cio' che serve quando la logica sta in
	 * un `.uasset` e nessun grep la raggiunge.
	 *
	 * ⛔ **Il limite, dichiarato invece che scoperto**: una chiamata *virtuale* si compila con il solo
	 * nome e non lascia un puntatore, quindi non comparirebbe qui. Non e' il caso di `StartMatch`, che e'
	 * una `UFUNCTION` non virtuale su un subsystem concreto e si compila come `EX_FinalFunction`.
	 */
	TArray<FCalledFunction> FunctionsCalledBy(const UFunction* Fn)
	{
		TArray<FCalledFunction> Called;
		for (const TObjectPtr<UObject>& Ref : Fn->ScriptAndPropertyObjectReferences)
		{
			if (const UFunction* AsFunction = Cast<UFunction>(Ref.Get()))
			{
				Called.Add({ AsFunction->GetName(), AsFunction->GetOwnerClass() });
			}
		}
		return Called;
	}

	/** `Classe::Funzione`, nella forma in cui serve leggerla in un log di automation. */
	FString DescribeCall(const FCalledFunction& Call)
	{
		return FString::Printf(TEXT("%s::%s"),
			Call.Owner ? *Call.Owner->GetName() : TEXT("<senza classe>"), *Call.Name);
	}
}

/**
 * CP 46.3 (`#938`) — il grafo del menu chiede la partita al navigatore, e non con l'altra via.
 *
 * ⚠️ **Questo test e' l'oracolo di un lavoro che si fa in editor.** Il fix di `#938` non ha una riga di
 * C++: `URTFrontendNavigator::StartMatch()` e' gia' `BlueprintCallable`, e cio' che mancava era il
 * cablaggio di `EntryPlay.OnEntryClicked` dentro `WBP_RT_MainMenu.uasset`. Senza un test, la sola prova
 * che quel cablaggio esista sarebbe una sessione PIE — e la diagnosi di `#938` e' costata tre evidenze
 * indipendenti (comportamento, name table, log vuoto) proprio perche' nessuno guardava dentro il binario.
 *
 * Cosa verifica, dal fatto piu' generale al piu' specifico:
 *
 *  1. **L'entry esiste** e si chiama `EntryPlay`. Un rename in editor rompe il binding senza dirlo.
 *  2. **Esiste la funzione di binding** del suo `OnEntryClicked` — cio' che UMG genera quando si collega
 *     un event dispatcher. La sua assenza e' la misura registrata dalla diagnosi: *«nessun `BndEvt` per
 *     `EntryPlay`»*.
 *  3. **Il grafo chiama `URTFrontendNavigator::StartMatch`**, con la classe verificata e non il solo nome.
 *  4. **Il grafo NON chiama `EnterMatch`.** E' un guardiano **indipendente** dal punto 3, non il suo
 *     complemento: `EnterMatch` porta in partita saltando la richiesta pendente, e serve al caso opposto —
 *     PIE lanciato dritto su `L_HexArena`. Un menu che la chiamasse *mentre* qualcos'altro chiama ancora
 *     `StartMatch` lascerebbe `G13` dov'e' senza che nessuna delle altre asserzioni se ne accorga.
 *
 * ⛔ **CIO' CHE QUESTO TEST NON PUO' FARE, e va saputo prima di fidarsene: non attribuisce la chiamata al
 * PULSANTE.** UMG compila **ogni** evento del widget in un unico `ExecuteUbergraph_WBP_RT_MainMenu`, la
 * cui lista di referenze e' l'unione di PLAY, SETTINGS e QUIT — lo si legge nell'evidenza che questo test
 * stesso versa nel log. Quindi il punto 3 dice *«qualcosa in questo grafo chiama `StartMatch`»*, non
 * *«`EntryPlay` chiama `StartMatch`»*: con un secondo chiamante nel grafo, `EntryPlay` potrebbe essere
 * ricablato altrove — o avere il pin exec scollegato, che lascia lo stub `BndEvt__` in piedi — e le
 * asserzioni resterebbero verdi. E' la ragione per cui il test si chiama `MainMenuGraphAsks…`: il
 * soggetto e' il grafo, non il pulsante.
 *
 * ⚠️ **La sede per chiudere quel buco esiste, e non e' questa.** Serve seguire i pin dal
 * `K2Node_ComponentBoundEvent` di `EntryPlay` fino al `K2Node_CallFunction` di `StartMatch`, cioe'
 * `BlueprintGraph` e `UMGEditor` — moduli **Editor**, da cui un modulo Runtime non puo' dipendere senza
 * rompere Shipping. Il posto giusto e' `Source/RefactorTacticsEditor/Private/Tests/`, che ha gia'
 * `UnrealEd` e due file di test. Finche' quel test non esiste, **`PIE-V01-FRONTEND-PLAY` resta l'oracolo
 * che dice se PLAY avvia davvero la partita**, e questo ne copre soltanto il presupposto.
 *
 * ⛔ E come il resto del file: non copre l'aspetto. Un binding corretto su un widget largo zero pixel
 * passa di qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMainMenuGraphAsksTheNavigatorToStartTest,
	"RefactorTactics.Frontend.MainMenuGraphAsksTheNavigatorToStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMainMenuGraphAsksTheNavigatorToStartTest::RunTest(const FString&)
{
	UWidgetBlueprintGeneratedClass* Class = LoadWidgetClass(MainMenuPath);
	if (!TestNotNull(TEXT("WBP_RT_MainMenu si carica"), Class))
	{
		return false;
	}

	ReportAsset(*this, Class, TEXT("WBP_RT_MainMenu"));

	// ── 1. l'entry PLAY esiste, e si chiama cosi' ────────────────────────────────────────────────────
	const UWidgetTree* Tree = Class->GetWidgetTreeArchetype();
	if (!TestNotNull(TEXT("l'albero del menu esiste"), Tree))
	{
		return false;
	}

	if (!Tree->FindWidget(TEXT("EntryPlay")))
	{
		TArray<FString> Names;
		Tree->ForEachWidget([&Names](UWidget* Widget) { Names.Add(Widget->GetName()); });
		AddError(FString::Printf(
			TEXT("WBP_RT_MainMenu non ha un widget 'EntryPlay'. L'albero porta: %s. ")
			TEXT("Un rename dell'entry rompe il binding di PLAY senza che nulla lo dica."),
			*FString::Join(Names, TEXT(", "))));
		return false;
	}

	// ── l'evidenza, prima delle asserzioni ───────────────────────────────────────────────────────────
	// ⚠️ **Una sola scansione, e le asserzioni decidono su QUESTA.** Rileggere le referenze una seconda
	// volta piu' in basso costerebbe il doppio e, peggio, permetterebbe alle due letture di divergere:
	// l'evidenza versata nel log smetterebbe di essere l'evidenza su cui il verdetto si forma.
	const TArray<UFunction*> Own = OwnFunctions(Class);
	TArray<FCalledFunction> AllCalls;

	AddInfo(FString::Printf(TEXT("=== WBP_RT_MainMenu: %d funzioni proprie ==="), Own.Num()));
	for (const UFunction* Fn : Own)
	{
		const TArray<FCalledFunction> Called = FunctionsCalledBy(Fn);

		TArray<FString> Described;
		for (const FCalledFunction& Call : Called)
		{
			Described.Add(DescribeCall(Call));
		}

		AddInfo(FString::Printf(TEXT("  %s -> [%s]"),
			*Fn->GetName(),
			Described.Num() > 0 ? *FString::Join(Described, TEXT(", ")) : TEXT("nessuna chiamata tracciata")));

		AllCalls.Append(Called);
	}

	bool bOk = true;

	// ── 2. la funzione di binding dell'evento esiste ─────────────────────────────────────────────────
	const bool bHasPlayBinding = Own.ContainsByPredicate([](const UFunction* Fn)
	{
		// ⚠️ **`_EntryPlay_K2Node_ComponentBoundEvent_` e non il solo `EntryPlay`.** UMG compone il nome
		// come `BndEvt__<widget>_<entry>_K2Node_ComponentBoundEvent_<n>_<evento>__DelegateSignature`:
		// un futuro `EntryPlayAgain` produrrebbe `_EntryPlayAgain_K2Node_…`, che contiene la sottostringa
		// `EntryPlay` e soddisferebbe un match piu' largo **mentre il binding di PLAY e' stato cancellato**.
		return Fn->GetName().Contains(TEXT("_EntryPlay_K2Node_ComponentBoundEvent_"));
	});

	if (!bHasPlayBinding)
	{
		AddError(TEXT(
			"WBP_RT_MainMenu non ha alcuna funzione di binding per EntryPlay.OnEntryClicked: PLAY non "
			"chiama niente. E' la misura registrata dalla diagnosi di #938 — il pulsante e' disegnato e "
			"inerte. Si cabla in editor, e non in C++: OnEntryClicked di EntryPlay -> Get Game Instance "
			"Subsystem (RTFrontendNavigator) -> StartMatch."));
		bOk = false;
	}

	// ── 3. il grafo chiama StartMatch, e sul navigatore ──────────────────────────────────────────────
	bool bCallsStartMatch = false;
	bool bCallsEnterMatch = false;
	for (const FCalledFunction& Call : AllCalls)
	{
		if (Call.Owner != URTFrontendNavigator::StaticClass())
		{
			continue;
		}

		bCallsStartMatch = bCallsStartMatch || (Call.Name == TEXT("StartMatch"));
		bCallsEnterMatch = bCallsEnterMatch || (Call.Name == TEXT("EnterMatch"));
	}

	if (!bCallsStartMatch)
	{
		AddError(TEXT(
			"nessuna funzione di WBP_RT_MainMenu chiama URTFrontendNavigator::StartMatch(): la catena "
			"menu -> StartMatch -> ConsumePendingMatchLevel -> ARTGameMode non viene mai imboccata. Un "
			"pacchetto che avvia sul menu e da li' non puo' iniziare una partita sposta il dead-end di "
			"G13 invece di rimuoverlo."));
		bOk = false;
	}

	// ── 4. e NON chiama EnterMatch — guardiano indipendente, non complemento del punto 3 ─────────────
	if (bCallsEnterMatch)
	{
		AddError(TEXT(
			"WBP_RT_MainMenu chiama URTFrontendNavigator::EnterMatch(): porta in partita saltando la "
			"richiesta pendente. EnterMatch serve al caso opposto — PIE lanciato dritto su L_HexArena, "
			"dove il frontend non e' mai partito — e dal menu lascia G13 dov'e'. Il menu passa da "
			"StartMatch, che CHIEDE la partita e lascia l'apertura a chi ha il mondo."));
		bOk = false;
	}

	return bOk;
}

namespace
{
	const TCHAR* const MenuEntryPath = TEXT("/Game/RT/UI/Framework/WBP_RT_MenuEntry.WBP_RT_MenuEntry_C");

	/**
	 * Due brush differiscono per qualcosa che **non** sia il colore?
	 *
	 * ⚠️ `FSlateBrush::operator==` non serve qui: confronta anche `TintColor`, quindi due stati che
	 * cambiano **solo** tinta risulterebbero «diversi» — che e' esattamente il caso da bocciare. Questa
	 * funzione guarda i canali che restano leggibili quando il colore sparisce: la risorsa disegnata, il
	 * modo di disegnarla, il margine e lo spessore dell'outline.
	 *
	 * ⛔ `OutlineSettings.Color` e' deliberatamente escluso: un outline che cambia **colore** e non
	 * spessore e' ancora un segnale cromatico, e passerebbe un test che lo contasse.
	 */
	bool DiffersBeyondTint(const FSlateBrush& A, const FSlateBrush& B)
	{
		return A.GetResourceObject() != B.GetResourceObject()
			|| A.DrawAs != B.DrawAs
			|| A.Margin != B.Margin
			|| A.OutlineSettings.Width != B.OutlineSettings.Width;
	}

	/** I `UButton` dell'albero, in ordine di visita. */
	TArray<UButton*> ButtonsOf(const UWidgetTree* Tree)
	{
		TArray<UButton*> Found;
		if (Tree)
		{
			const_cast<UWidgetTree*>(Tree)->ForEachWidget([&Found](UWidget* Widget)
			{
				if (UButton* Button = Cast<UButton>(Widget))
				{
					Found.Add(Button);
				}
			});
		}
		return Found;
	}
}

/**
 * CP 46.3 (`#938`) — **lo stato di un pulsante non si distingue dal solo colore.**
 *
 * La regola non e' nuova e non e' locale a questo widget: il DoD di #938 la scrive come *«il focus non e'
 * distinguibile dal solo colore: serve un secondo segnale (bordo, offset, icona)»*, e la stessa frase
 * ricorre in **otto** voci di `test-manuali-pie.md`, dove oggi si verifica **a occhio, in scala di grigi**.
 * Questo test ne prende la meta' meccanica.
 *
 * ⚠️ **Nasce rosso, ed e' il punto.** Misurato il 2026-08-29 sulla build packaged: la voce con il focus
 * porta `RGB(167,167,167)` e le altre due `RGB(187,187,187)` — nessun bordo, nessun offset (949 px contro
 * 946), nessuna icona. Il motivo e' che `WBP_RT_MenuEntry.uasset` **non contiene** `WidgetStyle`: il
 * pulsante usa lo stile di default di UMG, che distingue gli stati per la sola tinta. Non e' una scelta
 * sbagliata, e' una scelta mai fatta — e nessun gate la vedeva.
 *
 * ⛔ **Cosa questo test NON prova**: che il secondo segnale sia *leggibile*. Un outline da 0.01 px lo
 * farebbe passare. La leggibilita' resta di `PIE-V01-FRONTEND-MAIN`, che rilegge tutto in scala di grigi;
 * qui si accerta soltanto che un secondo canale **esista**, che e' la condizione senza la quale quella
 * rilettura non puo' che fallire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTButtonStatesAreNotColorOnlyTest,
	"RefactorTactics.Frontend.ButtonStatesAreNotColorOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTButtonStatesAreNotColorOnlyTest::RunTest(const FString&)
{
	// ⚠️ La lista e' esplicita e non una scansione della cartella: un widget nuovo con un pulsante NON
	// entra qui da solo, e va aggiunto a mano. E' un limite dichiarato — misurato il 2026-08-29, i soli
	// due `WBP_RT_*` che contengono un `UButton` sono questi — e la scansione automatica costerebbe un
	// caricamento di tutti gli asset UI a ogni run per coprire un caso che oggi non esiste.
	struct FSubject { const TCHAR* Path; const TCHAR* Label; };
	const FSubject Subjects[] =
	{
		{ MenuEntryPath,  TEXT("WBP_RT_MenuEntry")  },
		{ ErrorModalPath, TEXT("WBP_RT_ErrorModal") },
	};

	bool bOk = true;
	int32 ButtonsSeen = 0;

	for (const FSubject& Subject : Subjects)
	{
		UWidgetBlueprintGeneratedClass* Class = LoadWidgetClass(Subject.Path);
		if (!Class)
		{
			AddError(FString::Printf(TEXT("%s: asset non caricabile (%s)"), Subject.Label, Subject.Path));
			bOk = false;
			continue;
		}

		const TArray<UButton*> Buttons = ButtonsOf(Class->GetWidgetTreeArchetype());
		if (Buttons.Num() == 0)
		{
			AddError(FString::Printf(TEXT(
				"%s non contiene alcun UButton: il test non ha soggetto qui. Se il widget e' stato rifatto "
				"con un altro componente, questa riga va ripuntata invece che tolta."), Subject.Label));
			bOk = false;
			continue;
		}

		for (const UButton* Button : Buttons)
		{
			++ButtonsSeen;
			const FButtonStyle& Style = Button->GetStyle();

			// Normal contro Hovered e Normal contro Pressed: sono i due stati con cui il giocatore sa
			// «dove sono» e «cosa ho premuto».
			const bool bHoveredDiffers = DiffersBeyondTint(Style.Normal, Style.Hovered);
			const bool bPressedDiffers = DiffersBeyondTint(Style.Normal, Style.Pressed);

			AddInfo(FString::Printf(
				TEXT("%s / %s: hovered oltre-il-colore=%s  pressed oltre-il-colore=%s"),
				Subject.Label, *Button->GetName(),
				bHoveredDiffers ? TEXT("si") : TEXT("NO"),
				bPressedDiffers ? TEXT("si") : TEXT("NO")));

			if (!bHoveredDiffers)
			{
				AddError(FString::Printf(TEXT(
					"%s / %s: lo stato HOVERED si distingue dal Normal per il solo colore. Il DoD di #938 "
					"chiede un secondo segnale — bordo (OutlineSettings.Width), offset (Margin), o una "
					"risorsa diversa. Con la sola tinta, in scala di grigi la voce sotto il puntatore non "
					"si riconosce."),
					Subject.Label, *Button->GetName()));
				bOk = false;
			}

			if (!bPressedDiffers)
			{
				AddError(FString::Printf(TEXT(
					"%s / %s: lo stato PRESSED si distingue dal Normal per il solo colore. Stessa regola e "
					"stesso rimedio dello stato hovered."),
					Subject.Label, *Button->GetName()));
				bOk = false;
			}
		}
	}

	// 🔴 La guardia che rende il test non-vacuo: se nessun pulsante e' stato ispezionato, un `return true`
	// sarebbe un verde per ASSENZA di soggetto — la forma di falso verde piu' difficile da notare, perche'
	// non produce nessun messaggio.
	if (ButtonsSeen == 0)
	{
		AddError(TEXT(
			"nessun UButton ispezionato in alcun soggetto: il test non ha misurato niente. Un verde qui "
			"non avrebbe significato «gli stati sono distinguibili», ma «non ho guardato»."));
		bOk = false;
	}

	AddInfo(FString::Printf(TEXT("pulsanti ispezionati: %d in %d widget"), ButtonsSeen, UE_ARRAY_COUNT(Subjects)));
	return bOk;
}

#endif // WITH_DEV_AUTOMATION_TESTS
