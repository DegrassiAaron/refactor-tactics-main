// #2826 — il tratto fra il puntatore e la porta di armamento, che vive interamente nel `.uasset`.
//
// 🔑 **Questo file chiude la meta' che un altro gate dichiara di non coprire.**
// `Source/RefactorTactics/Tests/RTMatchWidgetAssetTests.cpp`, nel commento di
// `ScreenHud.ActionSlotIsNotTransparentToThePointer`, scrive che quel gate *«copre una meta' sola»* —
// misura che un click **possa arrivare** allo slot, non che ci sia qualcosa che lo **riceva** — e dichiara
// quando l'altra meta' debba atterrare: *«quello arrivera' col cablaggio»*. E' questo.
//
// ⚠️ **La tecnica runtime non basta, e il fratello di `#2326` lo ha gia' misurato.** UMG compila ogni
// evento del widget in un unico `ExecuteUbergraph_...`, e `UWidgetBlueprintGeneratedClass::Bindings` vede
// i **property binding**, non i cablaggi di grafo: `RTMatchWidgetAssetTests.cpp` porta scritta la storia
// di un test caduto proprio per quell'oracolo — *«cercava la cosa sbagliata»*. Qui si seguono i **pin**,
// dal nodo evento del bottone fino alla chiamata.
//
// ⛔ **Cosa NON copre**: l'aspetto, la dimensione e il fatto che il click arrivi davvero a schermo. Un
// bottone cablato benissimo dentro un antenato `Hit Test Invisible` passa di qui e non si preme — la meta'
// strutturale e' di `ActionSlotIsNotTransparentToThePointer`, e il verdetto umano di `PIE-V01-DOCKCLICK`.

#include "Misc/AutomationTest.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ComponentBoundEvent.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * ⚠️ **Namespace NOMINATO, non anonimo, e non e' uno stile.** `RTMainMenuEntryWiringTests.cpp` tiene tre
 * helper omonimi in un namespace anonimo: sotto **unity build** i due file finiscono nella stessa unita' di
 * traduzione, i due namespace anonimi si fondono, e due funzioni con la stessa firma sono una
 * ridefinizione. E' il difetto che `RTMatchWidgetAssetTests.cpp` documenta per `HudWidgetCarriesTexture`,
 * con la regola che ne e' uscita: *«identici → header condiviso; diversi → rinominare»*. Qui i corpi
 * divergono — questo segue **tutti** gli eventi di un delegate, non uno nominato — quindi si separa.
 */
namespace RTActionSlotClickWiring
{
	const TCHAR* ActionSlotBlueprintPath = TEXT("/Game/RT/UI/Match/WBP_RT_ActionSlot.WBP_RT_ActionSlot");

	/** Tutti i grafi in cui un widget puo' aver messo un nodo: ubergraph, event graph e funzioni. */
	void RaccogliGrafi(const UBlueprint* Blueprint, TArray<UEdGraph*>& Fuori)
	{
		if (!Blueprint)
		{
			return;
		}
		for (const TObjectPtr<UEdGraph>& Graph : Blueprint->UbergraphPages)
		{
			if (Graph) { Fuori.AddUnique(Graph); }
		}
		for (const TObjectPtr<UEdGraph>& Graph : Blueprint->EventGraphs)
		{
			if (Graph) { Fuori.AddUnique(Graph); }
		}
		// ⚠️ Le funzioni non sono un di piu': un cablaggio puo' vivere in una funzione del widget invece che
		// nell'evento, e cercare nei soli ubergraph darebbe rosso su un widget cablato bene.
		for (const TObjectPtr<UEdGraph>& Graph : Blueprint->FunctionGraphs)
		{
			if (Graph) { Fuori.AddUnique(Graph); }
		}
	}

	/**
	 * Ogni nodo evento legato al delegate `NomeDelegate`, su **qualunque** widget.
	 *
	 * 🔑 **Si cercano tutti e non uno nominato, ed e' la differenza con `#2326`.** Li' il difetto era una
	 * voce di menu mancante fra quattro esistenti, quindi il nome del pulsante era l'oracolo. Qui il difetto
	 * da escludere e' l'opposto: **un** `OnClicked` che va altrove — o due superfici che si contendono il
	 * click — e un test che ne nominasse uno solo lascerebbe l'altro invisibile.
	 */
	TArray<UK2Node_ComponentBoundEvent*> EventiDelDelegate(const UBlueprint* Blueprint, FName NomeDelegate)
	{
		TArray<UK2Node_ComponentBoundEvent*> Trovati;
		TArray<UEdGraph*> Grafi;
		RaccogliGrafi(Blueprint, Grafi);

		for (const UEdGraph* Graph : Grafi)
		{
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				UK2Node_ComponentBoundEvent* Bound = Cast<UK2Node_ComponentBoundEvent>(Node);
				if (Bound && Bound->DelegatePropertyName == NomeDelegate)
				{
					Trovati.AddUnique(Bound);
				}
			}
		}
		return Trovati;
	}

	/** Il pin di esecuzione in uscita di un nodo, se ne ha uno. */
	UEdGraphPin* EsecuzioneInUscita(UEdGraphNode* Node)
	{
		if (!Node)
		{
			return nullptr;
		}
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == TEXT("exec"))
			{
				return Pin;
			}
		}
		return nullptr;
	}

	/**
	 * Segue la catena di esecuzione da `Partenza` e rende la prima chiamata a `NomeFunzione`.
	 *
	 * `Visitati` non e' prudenza generica: un grafo con un ciclo di esecuzione farebbe girare la funzione per
	 * sempre, e un test che non termina non e' un test rosso — e' una suite che non finisce.
	 */
	UK2Node_CallFunction* SegueFinoAllaChiamata(
		UEdGraphNode* Partenza, const FString& NomeFunzione, TArray<FString>& Passi)
	{
		TSet<UEdGraphNode*> Visitati;
		UEdGraphNode* Corrente = Partenza;

		while (Corrente && !Visitati.Contains(Corrente))
		{
			Visitati.Add(Corrente);

			UEdGraphPin* Uscita = EsecuzioneInUscita(Corrente);
			if (!Uscita || Uscita->LinkedTo.Num() == 0)
			{
				return nullptr;
			}

			UEdGraphNode* Prossimo = Uscita->LinkedTo[0] ? Uscita->LinkedTo[0]->GetOwningNode() : nullptr;
			if (!Prossimo)
			{
				return nullptr;
			}

			if (UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Prossimo))
			{
				const UFunction* Bersaglio = Call->GetTargetFunction();
				const FString Nome = Bersaglio
					? Bersaglio->GetName()
					: Call->FunctionReference.GetMemberName().ToString();
				Passi.Add(FString::Printf(TEXT("chiama %s"), *Nome));
				if (Nome == NomeFunzione)
				{
					return Call;
				}
			}
			else
			{
				Passi.Add(Prossimo->GetClass()->GetName());
			}

			Corrente = Prossimo;
		}
		return nullptr;
	}

	/** `Classe::Funzione` per ogni chiamata del Blueprint — l'evidenza, e l'ingresso del divieto. */
	TArray<FString> TutteLeChiamate(const UBlueprint* Blueprint)
	{
		TArray<FString> Fuori;
		TArray<UEdGraph*> Grafi;
		RaccogliGrafi(Blueprint, Grafi);

		for (const UEdGraph* Graph : Grafi)
		{
			for (const UEdGraphNode* Node : Graph->Nodes)
			{
				const UK2Node_CallFunction* Call = Cast<const UK2Node_CallFunction>(Node);
				if (!Call)
				{
					continue;
				}
				if (const UFunction* Bersaglio = Call->GetTargetFunction())
				{
					const UClass* Proprietaria = Bersaglio->GetOwnerClass();
					Fuori.AddUnique(FString::Printf(TEXT("%s::%s"),
						Proprietaria ? *Proprietaria->GetName() : TEXT("?"), *Bersaglio->GetName()));
				}
				else
				{
					Fuori.AddUnique(FString::Printf(TEXT("?::%s"),
						*Call->FunctionReference.GetMemberName().ToString()));
				}
			}
		}
		return Fuori;
	}

	/** `true` se una delle chiamate si chiama `Funzione`, su qualunque classe. */
	bool Chiama(const TArray<FString>& Chiamate, const TCHAR* Funzione)
	{
		for (const FString& Chiamata : Chiamate)
		{
			FString Classe, Nome;
			if (Chiamata.Split(TEXT("::"), &Classe, &Nome) && Nome == Funzione)
			{
				return true;
			}
		}
		return false;
	}
}

/**
 * 🔴 **IL CLICK SU UNO SLOT RAGGIUNGE `Activate()`, E IL GRAFO NON COMPONE LA CHIAMATA DA SE'** (`#2826`).
 *
 * Dal fatto piu' generale al piu' specifico:
 *
 *  1. `WBP_RT_ActionSlot` si carica come Blueprint;
 *  2. esiste **almeno un** nodo evento `OnClicked` — cioe' una superficie che riceve il click;
 *  3. da **ognuno** di quei nodi la catena di esecuzione raggiunge `Activate`;
 *  4. la chiamata e' su **se stesso**: il pin `self` non e' collegato;
 *  5. il grafo **non** chiama `ArmKitAbility`, ne' cerca un `PlayerController` per conto proprio.
 *
 * 🔑 **Il punto 5 e' quello che questo test aggiunge, ed e' il difetto piu' facile da introdurre.** Un
 * `Get Player Controller` + `Cast` + `ArmKitAbility` dentro il grafo **funzionerebbe a schermo** e
 * passerebbe i punti 1-4 se qualcuno lasciasse anche `Activate` collegata. Sarebbe una seconda porta con la
 * propria risoluzione del proprietario: sei slot potrebbero risolverne sei diversi, e la ragione per cui
 * `Activate()` esiste — *«il grafo non deve comporre la chiamata da se'»* — resterebbe una promessa.
 *
 * ⚠️ **Il punto 3 dice ognuno e non uno**, e la differenza e' un difetto vero: due superfici cliccabili di
 * cui una cablata altrove — o non cablata affatto — darebbero al giocatore due meta' di riquadro che si
 * comportano in modo diverso, e un test che si fermasse al primo evento trovato non lo vedrebbe.
 *
 * ⛔ **Cosa questo test NON prova**: che il click arrivi. La `Visibility` degli antenati e' di
 * `ScreenHud.ActionSlotIsNotTransparentToThePointer`, e cio' che si vede a schermo di `PIE-V01-DOCKCLICK`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionSlotClickReachesArmingPortTest,
	"RefactorTactics.Editor.ActionSlotClickReachesTheArmingPort",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionSlotClickReachesArmingPortTest::RunTest(const FString&)
{
	using namespace RTActionSlotClickWiring;

	// `UBlueprint` e non `UWidgetBlueprint`: la classe base basta per i grafi, e cosi' questo file non deve
	// dipendere da `UMGEditor`.
	UBlueprint* Blueprint = Cast<UBlueprint>(
		StaticLoadObject(UBlueprint::StaticClass(), nullptr, ActionSlotBlueprintPath));

	if (!TestNotNull(TEXT("1: WBP_RT_ActionSlot si carica come Blueprint"), Blueprint))
	{
		return false;
	}

	const TArray<FString> Chiamate = TutteLeChiamate(Blueprint);
	AddInfo(FString::Printf(TEXT("il grafo dello slot chiama: %s"),
		Chiamate.Num() > 0 ? *FString::Join(Chiamate, TEXT(", ")) : TEXT("nessuna funzione")));

	// --- 2. Una superficie che riceve il click --------------------------------------------------------
	const TArray<UK2Node_ComponentBoundEvent*> Click = EventiDelDelegate(Blueprint, TEXT("OnClicked"));
	if (!TestTrue(
			FString::Printf(
				TEXT("2: lo slot ha almeno un evento `OnClicked` (ne ha %d). Senza, il dock resta a schermo ")
				TEXT("e cliccarlo non fa niente: e' il difetto che `#2826` esiste per chiudere, e la porta ")
				TEXT("C++ `ArmKitAbility` non aveva nessun chiamante in `Content/`"),
				Click.Num()),
			Click.Num() > 0))
	{
		return false;
	}

	// --- 3 e 4. OGNI click arriva ad `Activate`, e su se stesso ---------------------------------------
	for (UK2Node_ComponentBoundEvent* Evento : Click)
	{
		const FString Superficie = Evento->GetComponentPropertyName().ToString();

		TArray<FString> Passi;
		UK2Node_CallFunction* Attiva = SegueFinoAllaChiamata(Evento, TEXT("Activate"), Passi);

		AddInfo(FString::Printf(TEXT("cammino da %s.OnClicked: %s"),
			*Superficie, Passi.Num() > 0 ? *FString::Join(Passi, TEXT(" | ")) : TEXT("nessun passo")));

		if (!TestNotNull(
				*FString::Printf(
					TEXT("3: da `%s.OnClicked` la catena raggiunge `Activate` (passi: %s)"),
					*Superficie,
					Passi.Num() > 0 ? *FString::Join(Passi, TEXT(" | ")) : TEXT("nessuno")),
				Attiva))
		{
			continue;
		}

		// ⚠️ **Il pin `self` COLLEGATO sarebbe un difetto vero, non un dettaglio**: significherebbe che
		// questo riquadro attiva un widget che ha ricevuto da qualcun altro — cioe' arma la posizione di un
		// altro slot. Non collegato, il nodo chiama il proprio.
		if (UEdGraphPin* Self = Attiva->FindPin(TEXT("self"), EGPD_Input))
		{
			TestEqual(
				*FString::Printf(
					TEXT("4: `%s.OnClicked` attiva SE STESSO — il pin `self` di `Activate` non e' collegato"),
					*Superficie),
				Self->LinkedTo.Num(), 0);
		}
	}

	// --- 5. IL DIVIETO: il grafo non si costruisce una seconda porta -----------------------------------
	// ⛔ `ArmKitAbility` e' `BlueprintCallable` e resta raggiungibile dal grafo: e' proprio per questo che
	// il divieto va scritto. Chiamarla da qui salterebbe la guardia `INDEX_NONE` di `Activate()` e
	// spedirebbe una posizione che il grafo ha scelto, invece di quella che lo slot tiene.
	TestFalse(
		FString::Printf(
			TEXT("5: il grafo NON chiama `ArmKitAbility` da se' — la compone `Activate()`, in C++. Chiama: %s"),
			*FString::Join(Chiamate, TEXT(", "))),
		Chiama(Chiamate, TEXT("ArmKitAbility")));

	// ⛔ E non si cerca un proprietario: `Activate()` lo risolve una volta, in un posto solo. Sei slot che
	// lo risolvono ciascuno per conto proprio possono trovarne sei diversi — la stessa disciplina che
	// `ReceivedCatalog` porta scritta per il catalogo iconografico.
	for (const TCHAR* Vietata : { TEXT("GetPlayerController"), TEXT("GetOwningPlayer"),
			TEXT("SelectAbilityForCurrent") })
	{
		TestFalse(
			FString::Printf(
				TEXT("5: il grafo NON chiama `%s`: la risoluzione del proprietario resta in `Activate()`"),
				Vietata),
			Chiama(Chiamate, Vietata));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
