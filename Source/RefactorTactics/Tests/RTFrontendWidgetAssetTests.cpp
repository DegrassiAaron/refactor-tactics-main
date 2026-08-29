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
	 */
	TArray<UFunction*> OwnFunctions(UClass* Class)
	{
		TArray<UFunction*> Own;
		for (TFieldIterator<UFunction> It(Class); It; ++It)
		{
			if (It->GetOuter() == Class)
			{
				Own.Add(*It);
			}
		}
		return Own;
	}

	/**
	 * I nomi delle `UFunction` che il bytecode di `Fn` chiama.
	 *
	 * ⚠️ **Legge `ScriptAndPropertyObjectReferences`, non il bytecode.** UE tiene li' gli oggetti
	 * referenziati dallo script perche' il garbage collector li veda, ed e' l'unico accesso pubblico a
	 * «cosa chiama questo grafo» senza disassemblare — che e' cio' che serve quando la logica sta in un
	 * `.uasset` e nessun grep la raggiunge.
	 *
	 * ⛔ **Il limite, dichiarato invece che scoperto**: una chiamata *virtuale* si compila con il solo
	 * nome e non lascia un puntatore, quindi non comparirebbe qui. Non e' il caso di `StartMatch`, che e'
	 * una `UFUNCTION` non virtuale su un subsystem concreto e si compila come `EX_FinalFunction`.
	 */
	TArray<FString> FunctionsCalledBy(const UFunction* Fn)
	{
		TArray<FString> Called;
		for (const TObjectPtr<UObject>& Ref : Fn->ScriptAndPropertyObjectReferences)
		{
			if (const UFunction* AsFunction = Cast<UFunction>(Ref.Get()))
			{
				Called.AddUnique(AsFunction->GetName());
			}
		}
		return Called;
	}
}

/**
 * CP 46.3 (`#938`) — `PLAY` chiede una partita al navigatore.
 *
 * ⚠️ **Questo test e' l'oracolo di un lavoro che si fa in editor, e nasce rosso di proposito.** Il fix di
 * `#938` non ha una riga di C++: `URTFrontendNavigator::StartMatch()` e' gia' `BlueprintCallable`, e cio'
 * che manca e' il cablaggio di `EntryPlay.OnEntryClicked` dentro `WBP_RT_MainMenu.uasset`. Senza un test,
 * la sola prova che quel cablaggio esista sarebbe una sessione PIE — e la diagnosi di `#938` e' costata
 * tre evidenze indipendenti (comportamento, name table, log vuoto) proprio perche' nessuno guardava
 * dentro il binario.
 *
 * Cosa verifica, dal fatto piu' generale al piu' specifico:
 *
 *  1. **L'entry esiste** e si chiama `EntryPlay`. Un rename in editor rompe il binding senza dirlo, ed e'
 *     l'unica riga del test che nomina il widget: se fallisce lei, le altre due non significherebbero
 *     niente.
 *  2. **Esiste una funzione di binding** per il suo `OnEntryClicked` — cio' che UMG genera quando si
 *     collega un event dispatcher. La sua assenza e' esattamente la misura registrata dalla diagnosi:
 *     *«nessun `BndEvt` per `EntryPlay`»*.
 *  3. **Il grafo chiama `StartMatch`**, e non l'altra via. `EnterMatch` compila, si arma e porta in
 *     partita — ma salta la richiesta pendente, che e' il contratto di CP 46.4: il navigatore non ha un
 *     mondo, quindi *chiede* invece di aprire. Un `PLAY` cablato su `EnterMatch` funzionerebbe in PIE e
 *     lascerebbe `G13` dov'e'.
 *
 * ⛔ **Cio' che NON copre, e che resta di `PIE-V01-FRONTEND-MAIN`**: che il pulsante si veda, che il focus
 * sia distinguibile senza il colore, che la tastiera lo raggiunga. Un binding corretto su un widget largo
 * zero pixel passa di qui — e' il limite dichiarato in testa a questo file, e vale per gli eventi come
 * vale per le proprieta'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMainMenuPlayAsksTheNavigatorToStartTest,
	"RefactorTactics.Frontend.MainMenuPlayAsksTheNavigatorToStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMainMenuPlayAsksTheNavigatorToStartTest::RunTest(const FString&)
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

	// ── l'evidenza, prima delle asserzioni: cosa chiama ogni funzione del menu ───────────────────────
	const TArray<UFunction*> Own = OwnFunctions(Class);
	AddInfo(FString::Printf(TEXT("=== WBP_RT_MainMenu: %d funzioni proprie ==="), Own.Num()));
	for (const UFunction* Fn : Own)
	{
		const TArray<FString> Called = FunctionsCalledBy(Fn);
		AddInfo(FString::Printf(TEXT("  %s -> [%s]"),
			*Fn->GetName(),
			Called.Num() > 0 ? *FString::Join(Called, TEXT(", ")) : TEXT("nessuna chiamata tracciata")));
	}

	bool bOk = true;

	// ── 2. il binding dell'evento esiste ─────────────────────────────────────────────────────────────
	const bool bHasPlayBinding = Own.ContainsByPredicate([](const UFunction* Fn)
	{
		const FString Name = Fn->GetName();
		return Name.Contains(TEXT("EntryPlay")) && Name.Contains(TEXT("OnEntryClicked"));
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

	// ── 3. e chiama StartMatch, non l'altra via d'avvio ──────────────────────────────────────────────
	bool bCallsStartMatch = false;
	bool bCallsEnterMatch = false;
	for (const UFunction* Fn : Own)
	{
		for (const FString& Called : FunctionsCalledBy(Fn))
		{
			bCallsStartMatch = bCallsStartMatch || (Called == TEXT("StartMatch"));
			bCallsEnterMatch = bCallsEnterMatch || (Called == TEXT("EnterMatch"));
		}
	}

	if (!bCallsStartMatch)
	{
		AddError(TEXT(
			"nessuna funzione di WBP_RT_MainMenu chiama StartMatch(): la catena "
			"menu -> StartMatch -> ConsumePendingMatchLevel -> ARTGameMode non viene mai imboccata. Un "
			"pacchetto che avvia sul menu e da li' non puo' iniziare una partita sposta il dead-end di "
			"G13 invece di rimuoverlo."));
		bOk = false;
	}

	if (bCallsEnterMatch && !bCallsStartMatch)
	{
		AddError(TEXT(
			"il menu chiama EnterMatch() al posto di StartMatch(): porta in partita saltando la richiesta "
			"pendente. EnterMatch serve al caso opposto — PIE lanciato direttamente su L_HexArena, dove il "
			"frontend non e' mai partito."));
		bOk = false;
	}

	return bOk;
}

#endif // WITH_DEV_AUTOMATION_TESTS
