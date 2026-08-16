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

void URTFrontendNavigator::InitializeFrontend(FName RootScreenId)
{
	Stack = FRTScreenStack(RootScreenId);
	bInitialized = !RootScreenId.IsNone();
	SyncPresentation();
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

	TObjectPtr<UUserWidget>& Slot = LiveWidgets.FindOrAdd(ScreenId);
	if (!Slot)
	{
		UClass* WidgetClass = Binding->LoadSynchronous();
		if (!WidgetClass)
		{
			return;
		}

		// ⚠️ **L'unico `CreateWidget` del codebase, insieme a quelli di `DismissWidget`.** Se ne compare uno
		// dentro un widget, il DoD di CP 46.1 e' violato e il `grep` lo dice.
		Slot = CreateWidget<UUserWidget>(GI, WidgetClass);
	}

	if (Slot && !Slot->IsInViewport())
	{
		Slot->AddToViewport(ZOrder);
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

	// La schermata sotto non riceve input mentre un modale la copre (DoD). E' `bIsEnabled` e non
	// `Visibility`: nasconderla la farebbe sparire da sotto il modale, mentre deve restare **visibile e
	// inerte** — un modale che oscura il contesto che sta interrompendo e' peggio di nessun modale.
	if (UUserWidget* Current = FindLiveWidget(Stack.CurrentScreen()))
	{
		Current->SetIsEnabled(Stack.IsScreenInteractive());
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
