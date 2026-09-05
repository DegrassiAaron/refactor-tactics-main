// Cosa `WBP_RT_UnitOverlay` porta DAVVERO nel suo albero (`#2455`, `#2288`, `D-320`).
//
// 🔴 **Il difetto che questi test rendono impossibile e' un'assenza che non fa rumore.**
// `URTUnitOverlayWidget` dichiara i propri elementi con `BindWidgetOptional`, e l'opzionalita' e'
// deliberata: un `WBP` incompleto **degrada** — mostra il resto — invece di rifiutarsi di compilare. La
// contropartita e' che un elemento mancante non produce **niente**: nessun errore, nessun test rosso,
// nessun pixel. Il C++ compone il valore, lo posa su un puntatore nullo, e la funzione ritorna.
//
// ⚠️ **E' successo davvero, ed e' la ragione per cui questo file esiste.** `#2455` ha consegnato il token
// di danno con la suite verde e a schermo nulla, perche' l'elemento nel `.uasset` non c'era ancora. La
// verifica a occhio l'avrebbe trovato; nessuna macchina no.
//
// 🔑 **Sta nel modulo Editor perche' `UWidgetBlueprint` e' editor-only**: il runtime vede la classe
// generata, non l'albero autorato. E' la stessa sede, e la stessa ragione, di `RTPlaygroundPanelTests`.

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "WidgetBlueprint.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const TCHAR* OverlayAsset = TEXT("/Game/RT/UI/Match/WBP_RT_UnitOverlay");

	UWidgetBlueprint* LoadOverlay()
	{
		return LoadObject<UWidgetBlueprint>(nullptr, OverlayAsset);
	}
}

/**
 * Il token di danno ha un posto dove atterrare (`#2455`).
 *
 * ⛔ **Non verifica che si VEDA** — nessun test headless puo'. Verifica le quattro condizioni senza le
 * quali non potrebbe vedersi in nessun caso: l'elemento esiste, e' del tipo giusto, e' una **variabile**
 * (senza cui il `BindWidgetOptional` non lo trova) e nasce **nascosto**. Il giudizio a schermo resta
 * `PIE-HUD-DAMAGE-TOKEN`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverlayDamageTokenBindingTest,
	"RefactorTactics.Overlay.UnitOverlayCarriesTheDamageTokenBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverlayDamageTokenBindingTest::RunTest(const FString&)
{
	UWidgetBlueprint* Overlay = LoadOverlay();
	if (!TestNotNull(TEXT("l'overlay e' caricabile dal suo percorso"), Overlay))
	{
		return false;
	}
	UWidgetTree* Tree = Overlay->WidgetTree;
	if (!TestNotNull(TEXT("l'overlay ha un widget tree"), Tree))
	{
		return false;
	}

	UWidget* Raw = Tree->FindWidget(TEXT("DamageTokenText"));
	if (!TestNotNull(TEXT("'DamageTokenText' esiste nell'albero"), Raw))
	{
		// ⚠️ `return false` e non un proseguimento: senza l'elemento le righe sotto misurerebbero un
		// nullptr, e il referto direbbe quattro cose invece di quella che conta.
		return false;
	}

	UTextBlock* Token = Cast<UTextBlock>(Raw);
	if (!TestNotNull(TEXT("ed e' un UTextBlock, come il binding del C++ pretende"), Token))
	{
		return false;
	}

	// 🔑 **Senza `bIsVariable` il binding non lo trova.** `BindWidgetOptional` cerca una VARIABILE del
	// Blueprint: un widget presente nell'albero ma non-variabile e' invisibile al C++, che continuerebbe a
	// posare il token su un puntatore nullo — il difetto di partenza, intatto e con l'elemento in vista
	// nell'Editor. E' il modo piu' insidioso in cui questo cablaggio puo' sembrare fatto.
	TestTrue(TEXT("e' una variabile, o il BindWidgetOptional non lo aggancia"), Token->bIsVariable);

	// 🔴 **Il GUID, o la compilazione del `WBP` ASSERISCE.** `ValidateAndFixUpVariableGuids` pretende che
	// ogni widget-variabile stia in `WidgetVariableNameToGuidMap` — *«was added but did not get a GUID»*.
	// Un asset salvato senza sopravvive finche' nessuno lo ricompila, e rompe il primo che lo fa.
	TestTrue(TEXT("porta un GUID in WidgetVariableNameToGuidMap"),
		Overlay->WidgetVariableNameToGuidMap.Contains(Token->GetFName()));

	// ⚠️ **Nasce nascosto.** `SetOverlayView` riafferma il riposo a ogni fotogramma, ma il primo disegno
	// avviene prima di quel giro: un elemento che nascesse visibile mostrerebbe una riga sopra ogni unita'
	// per un fotogramma. E `Collapsed`, non `Hidden`: in una `UVerticalBox` un `Hidden` occuperebbe
	// comunque il proprio spazio, e la sovrapposizione a riposo sarebbe piu' alta di com'e' oggi.
	TestEqual(TEXT("nasce Collapsed: a riposo non occupa spazio"),
		static_cast<int32>(Token->GetVisibility()), static_cast<int32>(ESlateVisibility::Collapsed));

	// 🔑 **In cima, non in coda.** Il numero di un colpo si legge sopra l'unita'; in fondo finirebbe sotto
	// le icone di stato, cioe' dove si guarda per sapere *cosa* si ha addosso e non *cosa e' appena
	// successo*. Il posto e' una decisione, quindi si pinna.
	if (UPanelWidget* Parent = Token->GetParent())
	{
		TestEqual(TEXT("il contenitore e' RootBox"),
			Parent->GetFName().ToString(), FString(TEXT("RootBox")));
		TestEqual(TEXT("ed e' il primo figlio: il colpo si legge sopra, non sotto gli stati"),
			Parent->GetChildIndex(Token), 0);
	}
	else
	{
		AddError(TEXT("'DamageTokenText' non ha un contenitore: e' orfano nell'albero"));
	}

	return true;
}

/**
 * **Ogni `BindWidgetOptional` di `URTUnitOverlayWidget` ha una controparte nell'asset.**
 *
 * 🔴 **Generalizza il difetto invece di guardarne un'istanza.** Il token e' solo l'ultimo di sei elementi
 * legati per nome; gli altri cinque non erano presidiati da nulla, e ognuno puo' sparire dall'asset — per
 * un rename, per un `-Force` che rigenera, per un merge — **senza che niente diventi rosso**. Chi lo
 * scopre e' chi guarda lo schermo, e solo se sa cosa cercare.
 *
 * ⚠️ **Pinna una MISURA, non un'aspettativa.** I sei nomi sono quelli che il `.uasset` porta oggi
 * (verificati il 2026-09-06) e che il C++ dichiara. Se un elemento viene tolto per una ragione legittima —
 * com'e' successo a `EnergyBar` con `#2372` — si toglie **anche** la sua riga qui, e il diff mostra la
 * decisione. ⛔ Non si aggiorna il test per farlo tornare verde senza sapere chi l'ha tolto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverlayBoundWidgetsExistTest,
	"RefactorTactics.Overlay.EveryBoundWidgetExistsInTheAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverlayBoundWidgetsExistTest::RunTest(const FString&)
{
	UWidgetBlueprint* Overlay = LoadOverlay();
	UWidgetTree* Tree = Overlay ? ToRawPtr(Overlay->WidgetTree) : nullptr;
	if (!TestNotNull(TEXT("l'overlay e' caricabile"), Overlay) ||
		!TestNotNull(TEXT("l'overlay ha un widget tree"), Tree))
	{
		return false;
	}

	// Nome nell'asset -> classe che il C++ dichiara per quel binding.
	const TArray<TPair<const TCHAR*, UClass*>> Attesi =
	{
		{ TEXT("NameText"),        UTextBlock::StaticClass()   },
		{ TEXT("HealthBar"),       UProgressBar::StaticClass() },
		{ TEXT("HealthText"),      UTextBlock::StaticClass()   },
		{ TEXT("ShieldBar"),       UProgressBar::StaticClass() },
		{ TEXT("DamageTokenText"), UTextBlock::StaticClass()   },
	};

	for (const TPair<const TCHAR*, UClass*>& Voce : Attesi)
	{
		UWidget* W = Tree->FindWidget(FName(Voce.Key));
		if (!TestNotNull(*FString::Printf(TEXT("'%s' esiste nell'asset"), Voce.Key), W))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("'%s' e' una %s"), Voce.Key, *Voce.Value->GetName()),
			W->IsA(Voce.Value));
		TestTrue(*FString::Printf(TEXT("'%s' e' una variabile: il binding lo aggancia"), Voce.Key),
			W->bIsVariable);
	}

	// `StatusBox` a parte: il C++ lo dichiara `UHorizontalBox`, e qui basta che sia un contenitore — il
	// tipo esatto e' gia' preteso dal binding, che fallirebbe l'aggancio con una classe diversa. ⚠️ Non si
	// include nel ciclo sopra per non tirare dentro un altro include solo per una riga.
	UWidget* StatusBox = Tree->FindWidget(TEXT("StatusBox"));
	if (TestNotNull(TEXT("'StatusBox' esiste nell'asset"), StatusBox))
	{
		TestTrue(TEXT("'StatusBox' e' un contenitore"), StatusBox->IsA<UPanelWidget>());
		TestTrue(TEXT("'StatusBox' e' una variabile"), StatusBox->bIsVariable);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
