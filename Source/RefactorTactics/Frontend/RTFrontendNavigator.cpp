#include "Frontend/RTFrontendNavigator.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"

namespace
{
	/**
	 * Le schermate stanno sotto, i modali sopra. Due bande separate invece di un contatore crescente:
	 * un modale aperto su una schermata profonda deve restare sopra **qualunque** schermata, anche dopo un
	 * `ReturnMain`, e con uno Z progressivo finirebbe sotto alla prima navigazione.
	 */
	constexpr int32 ScreenZBase = 0;
	constexpr int32 ModalZBase = 1000;
}

ERTNavResult URTFrontendNavigator::InitializeFrontend(FName RootScreenId)
{
	if (RootScreenId.IsNone())
	{
		// Non si inizializza niente: uno stack con radice vuota accetterebbe transizioni che non
		// producono mai un widget, cioe' fallirebbe in silenzio a ogni chiamata successiva.
		return ERTNavResult::InvalidScreen;
	}

	Stack = FRTScreenStack(RootScreenId);
	bInitialized = true;
	SyncPresentation();
	return ERTNavResult::Ok;
}

void URTFrontendNavigator::RegisterScreen(FName ScreenId, TSoftClassPtr<UUserWidget> WidgetClass)
{
	if (!ScreenId.IsNone())
	{
		Bindings.Add(ScreenId, WidgetClass);
	}
}

ERTNavResult URTFrontendNavigator::PushScreen(FName ScreenId)
{
	const ERTNavResult Result = Stack.PushScreen(ScreenId);
	if (Result == ERTNavResult::Ok)
	{
		SyncPresentation();
	}
	return Result;
}

ERTNavResult URTFrontendNavigator::PopScreen()
{
	const ERTNavResult Result = Stack.PopScreen();
	if (Result == ERTNavResult::Ok)
	{
		SyncPresentation();
	}
	return Result;
}

ERTNavResult URTFrontendNavigator::ShowModal(FName ModalId)
{
	const ERTNavResult Result = Stack.ShowModal(ModalId);
	if (Result == ERTNavResult::Ok)
	{
		SyncPresentation();
	}
	return Result;
}

ERTNavResult URTFrontendNavigator::CloseModal()
{
	const ERTNavResult Result = Stack.CloseModal();
	if (Result == ERTNavResult::Ok)
	{
		SyncPresentation();
	}
	return Result;
}

ERTNavResult URTFrontendNavigator::ReturnMain()
{
	const ERTNavResult Result = Stack.ReturnMain();
	SyncPresentation();
	return Result;
}

UUserWidget* URTFrontendNavigator::FindLiveWidget(FName ScreenId) const
{
	const TObjectPtr<UUserWidget>* Found = LiveWidgets.Find(ScreenId);
	return Found ? Found->Get() : nullptr;
}

void URTFrontendNavigator::PresentWidget(FName ScreenId, int32 ZOrder)
{
	// Una schermata senza binding NON e' un errore: lo stack si e' gia' mosso e la navigazione e' valida.
	// Manca solo cosa disegnare — il caso normale di un test headless.
	const TSoftClassPtr<UUserWidget>* Binding = Bindings.Find(ScreenId);
	if (!Binding || Binding->IsNull())
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	// 🔴 **Un puntatore locale, non un riferimento dentro la `TMap`.** `FindOrAdd` restituisce un
	// riferimento al valore, e sia `CreateWidget` (che esegue `NativeOnInitialized`) sia `AddToViewport`
	// (che esegue `NativeConstruct` e l'evento Blueprint `Construct`) possono richiamare il navigatore —
	// tutte le sue funzioni sono `BlueprintCallable`. Una rientranza in `PresentWidget` fa `FindOrAdd` su
	// `LiveWidgets`, il rehash sposta i valori, e il riferimento tenuto qui punta a memoria liberata.
	// Trovato in code review; il costo della correzione e' zero e il difetto sarebbe stato intermittente.
	UUserWidget* Widget = FindLiveWidget(ScreenId);
	if (!Widget)
	{
		UClass* WidgetClass = Binding->LoadSynchronous();
		if (!WidgetClass)
		{
			return;
		}

		// ⚠️ **L'unico `CreateWidget` del codebase, insieme a quelli di `DismissWidget`.** Se ne compare uno
		// dentro un widget, il DoD di CP 46.1 e' violato e il `grep` lo dice.
		Widget = CreateWidget<UUserWidget>(GI, WidgetClass);
		if (!Widget)
		{
			return;
		}

		// La mappa si scrive **dopo** la costruzione, e da qui in poi non si tengono riferimenti al suo
		// interno: qualunque rientranza trova una mappa coerente.
		LiveWidgets.Add(ScreenId, Widget);
	}

	if (!Widget->IsInViewport())
	{
		Widget->AddToViewport(ZOrder);
	}
}

void URTFrontendNavigator::DismissWidget(FName ScreenId)
{
	TObjectPtr<UUserWidget>* Slot = LiveWidgets.Find(ScreenId);
	if (Slot && *Slot && (*Slot)->IsInViewport())
	{
		(*Slot)->RemoveFromParent();
	}
}

void URTFrontendNavigator::SyncPresentation()
{
	if (!bInitialized)
	{
		return;
	}

	// Cio' che deve stare a schermo: la sola schermata in cima, piu' tutti i modali aperti.
	//
	// ⚠️ **Solo la cima, non l'intero stack.** Le schermate sotto restano nello stack — sono il percorso di
	// ritorno — ma non si disegnano: tenerle a schermo significherebbe che un `Back` cambia lo Z-order
	// invece del contenuto, e che il giocatore vede due schermate sovrapposte.
	TSet<FName> ShouldBeVisible;
	ShouldBeVisible.Add(Stack.CurrentScreen());
	for (const FName& Modal : Stack.GetModals())
	{
		ShouldBeVisible.Add(Modal);
	}

	// Prima si smonta cio' che non serve piu', poi si monta: l'ordine inverso lascerebbe due schermate a
	// schermo per un frame.
	TArray<FName> Live;
	LiveWidgets.GetKeys(Live);
	for (const FName& Id : Live)
	{
		if (!ShouldBeVisible.Contains(Id))
		{
			DismissWidget(Id);
		}
	}

	PresentWidget(Stack.CurrentScreen(), ScreenZBase);

	int32 ModalZ = ModalZBase;
	for (const FName& Modal : Stack.GetModals())
	{
		PresentWidget(Modal, ModalZ++);
	}

	// Solo cio' che sta in CIMA riceve input: il modale piu' recente se ce n'e' uno, altrimenti la
	// schermata corrente. Tutto il resto resta **visibile e inerte**.
	//
	// E' `bIsEnabled` e non `Visibility`: nasconderlo lo farebbe sparire da sotto il modale, mentre deve
	// restare leggibile — un modale che oscura il contesto che sta interrompendo e' peggio di nessun modale.
	//
	// 🔴 **Questa parte disabilitava la sola `CurrentScreen()`**, quindi con due modali impilati quello
	// sotto restava **cliccabile** attraverso quello sopra. Trovato in code review: il DoD dice «il modale
	// disabilita cio' che sta sotto», e un modale sotto un altro modale *sta sotto*.
	const FName InteractiveId = Stack.IsModalOpen() ? Stack.TopModal() : Stack.CurrentScreen();

	for (const TPair<FName, TObjectPtr<UUserWidget>>& Pair : LiveWidgets)
	{
		if (UUserWidget* Widget = Pair.Value)
		{
			if (Widget->IsInViewport())
			{
				Widget->SetIsEnabled(Pair.Key == InteractiveId);
			}
		}
	}
}

void URTFrontendNavigator::Deinitialize()
{
	TArray<FName> Live;
	LiveWidgets.GetKeys(Live);
	for (const FName& Id : Live)
	{
		DismissWidget(Id);
	}
	LiveWidgets.Reset();
	Bindings.Reset();
	bInitialized = false;

	Super::Deinitialize();
}
