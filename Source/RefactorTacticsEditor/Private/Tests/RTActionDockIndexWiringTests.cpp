// #2989 — quale valore il `ForEach` del dock passa: la POSIZIONE del ciclo o `AbilityIndex`.
//
// 🔑 **E' la prima domanda della tabella di #2989, e la sua risposta era «nessuno».** Il dock arma — la
// porta e' provata da `PlayerInput.TheDockPortArmsAndDisarms`, e lo stato osservabile da
// `ScreenHud.DockArmsOnlyTheSelectedAction` — ma fra `GetActions()` e `SetAction(..., bInArmed)` c'e' un
// tratto che vive interamente dentro `WBP_RT_ActionDock.uasset`, e su quel tratto la suite non ha nulla.
//
// 🔴 **Il difetto che questo gate esiste per trovare non rompe niente OGGI, ed e' il punto.** Il grafo
// confronta l'indice del ciclo con `GetArmedActionIndex()`, e funziona solo perche' `#2987` ha reso
// `Cooldowns[i].AbilityIndex == i` un invariante di costruzione. Il giorno in cui quell'array venisse
// riordinato, filtrato o paginato, il dock accenderebbe il riquadro sbagliato — e **nulla smetterebbe di
// compilare**. `FRTAbilityCooldownView::AbilityIndex` esiste proprio per non dipendere da quella
// coincidenza: il suo commento lo dichiara, *«e' cio' che l'hotkey arma, quindi il widget ne ha bisogno»*.
//
// ✅ **La forma REALE del grafo, misurata il 2026-09-11** (non quella che lo Step 7.3 descrive). Risalendo
// i dati di `bInArmed` si attraversano **7** pin:
//
//     K2Node_PromotableOperator::ReturnValue          <- il confronto
//       K2Node_MacroInstance[ForLoop]::Index          <- la POSIZIONE del ciclo
//       K2Node_PromotableOperator::ReturnValue
//         K2Node_CallArrayFunction::ReturnValue
//         K2Node_CallFunction::ReturnValue  (x3)
//
// ⚠️ Due scostamenti dalla descrizione del piano, e contano entrambi: il ciclo e' un **`ForLoop`** e non un
// `ForEachLoop` — quindi il pin si chiama `Index`, non `Array Index` — e il confronto e' un
// `K2Node_PromotableOperator`, non il `CallFunc_EqualEqual_IntInt` che i nomi dentro il `.uasset`
// suggeriscono. Un oracolo scritto sulla descrizione, e non sulla misura, guarda nel posto sbagliato.
//
// ⚠️ **Perche' NON con `Class->Bindings`**: `RTMatchWidgetAssetTests.cpp` documenta
// `ActionDockConsumesArmedIndex` caduto esattamente cosi' — *«cercava property binding dove il dock usa il
// grafo, e sbagliava meccanismo»*. Qui si seguono i **pin**, che e' l'oracolo giusto per un event graph, e
// per farlo serve `BlueprintGraph`: e' la ragione per cui questo file vive nel modulo EDITOR.
//
// ⛔ **Cosa NON copre**: che il riquadro acceso sia quello che il giocatore vede accendersi. Questo gate
// legge la PROVENIENZA del valore, non la resa a schermo, e un dock corretto su un widget alto zero pixel
// passa di qui. La resa resta di `PIE-V01-HUD`.
//
// 🔑 **Perche' serve un gate STRUTTURALE e non basta un test di comportamento** — la domanda e' arrivata da
// una code review, e la risposta e' il senso di questa issue. Finche' `#2987` tiene
// `Cooldowns[i].AbilityIndex == i` — e lo tiene per costruzione: `BuildAbilityCooldowns` scrive
// `Empty.AbilityIndex = Index` anche per una posizione vuota, il `continue` che saltava e' stato rimosso —
// le due letture **coincidono su ogni kit**, buchi compresi. ∴ nessun test di comportamento puo' distinguere
// il prima dal dopo di questa correzione: non cambia cio' che il dock fa, cambia da cosa **dipende**. Il
// complemento sta in `RefactorTactics.HudViewModel.KitHoleDoesNotRenumberTheSlots`, che presidia
// l'invariante; questo presidia il fatto che il dock non ne abbia bisogno.
//
// ⚠️ **Duplicazione dichiarata**: `RaccogliGrafi` e `ChiamateA` ripetono helper che
// `RTActionSlotClickWiringTests.cpp` porta su `issue/2826-slot-cliccabile` (PR #3007), non ancora in `main`.
// Quando quella atterra, i tre file di wiring vanno fattorizzati su un helper condiviso — FOLLOW-UP, non
// lavoro di questa issue. Il namespace e' NOMINATO di proposito: sotto unity build un namespace anonimo
// collide con gli omonimi di `RTMainMenuEntryWiringTests.cpp`.

#include "Misc/AutomationTest.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "K2Node_MacroInstance.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTActionDockIndexWiring
{
	const TCHAR* ActionDockBlueprintPath = TEXT("/Game/RT/UI/Match/WBP_RT_ActionDock.WBP_RT_ActionDock");

	/**
	 * Lo SLOT serve da pietra di taratura, non come soggetto: il suo grafo porta `Action_AbilityIndex`, ed e'
	 * l'unico posto del progetto dove la convenzione di nome che questo gate riconosce esiste davvero.
	 */
	const TCHAR* ActionSlotBlueprintPath = TEXT("/Game/RT/UI/Match/WBP_RT_ActionSlot.WBP_RT_ActionSlot");

	/**
	 * Il pin porta `AbilityIndex` — la provenienza CORRETTA.
	 *
	 * 🔴 **Due forme, e riconoscerne una sola renderebbe questo gate INSODDISFACIBILE.** Un `Break` della
	 * vista da' un pin chiamato `AbilityIndex`; uno **split struct pin** — trascinare il campo direttamente
	 * dal pin della struct, che e' il gesto piu' naturale nel Designer — lo chiama `<NomeStruct>_AbilityIndex`.
	 * ⚠️ E la seconda forma non e' un'ipotesi: `WBP_RT_ActionSlot` porta **`Action_AbilityIndex`**, misurato
	 * nella name table del suo `.uasset`. Un predicato che accettasse solo `AbilityIndex` sarebbe rimasto
	 * rosso su un dock cablato correttamente, e il rosso avrebbe accusato l'asset invece dell'oracolo.
	 */
	bool ELAbilityIndex(const FString& NomePin)
	{
		return NomePin == TEXT("AbilityIndex") || NomePin.EndsWith(TEXT("_AbilityIndex"));
	}

	/**
	 * Il pin porta la POSIZIONE del ciclo.
	 *
	 * 🔴 **Il nome del pin da solo non basta, e la prima stesura ci e' cascata.** Cercava `Array Index`
	 * perche' lo Step 7.3 di #613 descrive un `ForEachLoop`, ma il dock usa un **`ForLoop`**, il cui pin si
	 * chiama `Index`. Il gate e' finito nel ramo indeterminato — misurato, `7` pin risaliti — invece di
	 * nominare il difetto. ⚠️ E la lezione non e' «aggiungi `Index` alla lista»: un pin chiamato `Index` lo
	 * ha anche un `Array Get`, e accettarlo nudo trasformerebbe questo predicato in un falso positivo. Il
	 * criterio guarda **il nodo**: quale macro e', e poi quale pin.
	 */
	bool EPosizioneDelCiclo(const UEdGraphNode* Nodo, const FString& NomePin)
	{
		if (const UK2Node_MacroInstance* Macro = Cast<UK2Node_MacroInstance>(Nodo))
		{
			const UEdGraph* MacroGraph = Macro->GetMacroGraph();
			const FString NomeMacro = MacroGraph ? MacroGraph->GetName() : FString();

			const bool bEUnCiclo = NomeMacro == TEXT("ForLoop")
				|| NomeMacro == TEXT("ForLoopWithBreak")
				|| NomeMacro == TEXT("ForEachLoop")
				|| NomeMacro == TEXT("ForEachLoopWithBreak")
				|| NomeMacro == TEXT("ReverseForEachLoop");

			if (bEUnCiclo && (NomePin == TEXT("Index") || NomePin == TEXT("Array Index")))
			{
				return true;
			}
		}

		// Le `Temp_int_*` sono le variabili che l'espansione della macro genera: si incontrano quando il
		// grafo che si sta leggendo e' gia' stato espanso, e li' il `MacroInstance` non c'e' piu'.
		return NomePin == TEXT("Temp_int_Array_Index_Variable")
			|| NomePin == TEXT("Temp_int_Loop_Counter_Variable");
	}

	/** Tutti i grafi in cui puo' vivere logica: ubergraph, funzioni, macro locali. */
	void RaccogliGrafi(const UBlueprint* Blueprint, TArray<UEdGraph*>& Fuori)
	{
		if (!Blueprint)
		{
			return;
		}
		for (const TObjectPtr<UEdGraph>& Graph : Blueprint->UbergraphPages)
		{
			if (Graph) { Fuori.Add(Graph); }
		}
		for (const TObjectPtr<UEdGraph>& Graph : Blueprint->FunctionGraphs)
		{
			if (Graph) { Fuori.Add(Graph); }
		}
		for (const TObjectPtr<UEdGraph>& Graph : Blueprint->MacroGraphs)
		{
			if (Graph) { Fuori.Add(Graph); }
		}
	}

	/** Le chiamate a `NomeFunzione`, in qualunque grafo del Blueprint. */
	TArray<UK2Node_CallFunction*> ChiamateA(const UBlueprint* Blueprint, const TCHAR* NomeFunzione)
	{
		TArray<UK2Node_CallFunction*> Trovate;
		TArray<UEdGraph*> Grafi;
		RaccogliGrafi(Blueprint, Grafi);

		for (const UEdGraph* Graph : Grafi)
		{
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node);
				if (!Call)
				{
					continue;
				}
				const UFunction* Target = Call->GetTargetFunction();
				const FString Nome = Target
					? Target->GetName()
					: Call->FunctionReference.GetMemberName().ToString();
				if (Nome == NomeFunzione)
				{
					Trovate.Add(Call);
				}
			}
		}
		return Trovate;
	}

	/** Quanti pin, in tutto il Blueprint, soddisfano `ELAbilityIndex`. Serve alla taratura. */
	int32 ContaPinAbilityIndex(const UBlueprint* Blueprint)
	{
		int32 Quanti = 0;
		TArray<UEdGraph*> Grafi;
		RaccogliGrafi(Blueprint, Grafi);

		for (const UEdGraph* Graph : Grafi)
		{
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (!Node)
				{
					continue;
				}
				for (const UEdGraphPin* Pin : Node->Pins)
				{
					if (Pin && ELAbilityIndex(Pin->PinName.ToString()))
					{
						++Quanti;
					}
				}
			}
		}
		return Quanti;
	}

	/** Che cosa la risalita ha incontrato. Nessun campo e' ridondante: vedi il verdetto. */
	struct FEsitoRisalita
	{
		/** Il valore viene da `AbilityIndex` — la provenienza che non dipende dall'invariante. */
		bool bLeggeAbilityIndex = false;

		/** Il valore viene dalla posizione del ciclo — il difetto. */
		bool bLeggePosizioneDelCiclo = false;

		/** Quanti pin la risalita ha davvero attraversato. Zero = il gate non ha misurato niente. */
		int32 PinVisitati = 0;

		/** Il cammino, per il log: un fallimento deve dire DOVE guardare. */
		TArray<FString> Tracce;
	};

	/**
	 * Risale il grafo dei DATI da `Ingresso` verso le sorgenti, e registra cosa incontra.
	 *
	 * ⚠️ **Attraversa solo nodi PURI** — quelli senza pin di esecuzione. Senza questo limite la risalita
	 * scavalcherebbe i confini fra istruzioni e finirebbe per raccogliere mezzo grafo, dichiarando «legge
	 * AbilityIndex» perche' quel nome compare *da qualche parte*. Il pin che conta e' quello che alimenta
	 * QUESTO argomento.
	 *
	 * `Visti` non e' prudenza generica: un grafo con un ciclo fra nodi puri farebbe girare la funzione per
	 * sempre, e un test che non termina non e' rosso — e' una suite che non finisce.
	 */
	void RisaliDati(const UEdGraphPin* Ingresso, FEsitoRisalita& Esito, TSet<const UEdGraphNode*>& Visti,
		int32 Profondita = 0)
	{
		if (!Ingresso || Profondita > 24)
		{
			return;
		}

		for (const UEdGraphPin* Sorgente : Ingresso->LinkedTo)
		{
			if (!Sorgente)
			{
				continue;
			}

			const UEdGraphNode* Nodo = Sorgente->GetOwningNode();
			if (!Nodo)
			{
				continue;
			}

			++Esito.PinVisitati;

			const FString NomePin = Sorgente->PinName.ToString();
			FString Descrizione = FString::Printf(TEXT("%s::%s"), *Nodo->GetClass()->GetName(), *NomePin);

			// Il `ForEachLoop` e' un `MacroInstance`: dire QUALE macro rende il log diagnostico invece di
			// generico, e distingue un ciclo da una macro qualsiasi.
			if (const UK2Node_MacroInstance* Macro = Cast<UK2Node_MacroInstance>(Nodo))
			{
				const UEdGraph* MacroGraph = Macro->GetMacroGraph();
				Descrizione = FString::Printf(TEXT("%s[%s]::%s"),
					*Nodo->GetClass()->GetName(),
					MacroGraph ? *MacroGraph->GetName() : TEXT("<macro ignota>"),
					*NomePin);
			}

			if (ELAbilityIndex(NomePin))
			{
				Esito.bLeggeAbilityIndex = true;
				Descrizione += TEXT("  <== AbilityIndex  (la risalita si ferma qui)");
				Esito.Tracce.Add(Descrizione);

				// 🔴 **POTATURA, e senza di essa questo gate e' INSODDISFACIBILE.** Oltre `AbilityIndex` c'e'
				// la provenienza dell'ELEMENTO — `Array|Get(acopy)` sull'array, indicizzato con la posizione
				// del ciclo — e quella posizione e' **legittima**: serve a prendere l'i-esimo elemento, non
				// e' il valore confrontato. Senza fermarsi qui, la risalita la incontrava e il gate
				// dichiarava il difetto su un dock corretto: misurato, `10` pin e un rosso con
				// `ForLoop::Index` sotto il `Break`.
				//
				// ⚠️ La distinzione che questo gate presidia e' esattamente quella: «l'indice serve a
				// PRENDERE l'elemento» (giusto) contro «l'indice E' il valore confrontato» (il difetto).
				continue;
			}

			if (EPosizioneDelCiclo(Nodo, NomePin))
			{
				Esito.bLeggePosizioneDelCiclo = true;
				Descrizione += TEXT("  <== POSIZIONE DEL CICLO");
			}

			Esito.Tracce.Add(Descrizione);

			if (Visti.Contains(Nodo))
			{
				continue;
			}
			Visti.Add(Nodo);

			// Un nodo con esecuzione e' un'istruzione, non un calcolo: la catena dei dati si ferma qui.
			bool bHaEsecuzione = false;
			for (const UEdGraphPin* Pin : Nodo->Pins)
			{
				if (Pin && Pin->PinType.PinCategory == TEXT("exec"))
				{
					bHaEsecuzione = true;
					break;
				}
			}
			// ⚠️ **Vale anche per i `MacroInstance`**, e la prima stesura li eccettuava per poter riconoscere
			// il pin `Index` del `ForLoop`. Non serviva: il riconoscimento avviene **sopra**, sul pin
			// sorgente, prima di questa ricorsione. Attraversarli faceva risalire *dentro* il ciclo — fino a
			// `LastIndex` e `Array|Length` — e portava nella traccia nodi che con lo stato armato non
			// c'entrano niente.
			if (bHaEsecuzione)
			{
				continue;
			}

			for (const UEdGraphPin* Pin : Nodo->Pins)
			{
				if (Pin && Pin->Direction == EGPD_Input
					&& Pin->PinType.PinCategory != TEXT("exec"))
				{
					RisaliDati(Pin, Esito, Visti, Profondita + 1);
				}
			}
		}
	}
}

/**
 * Lo stato armato che il dock passa a ciascuno slot deriva da `AbilityIndex`, non dalla posizione del
 * `ForEach` — `#2989`.
 *
 * Dal fatto piu' generale al piu' specifico:
 *
 *  1. `WBP_RT_ActionDock` si carica come Blueprint;
 *  2. il suo grafo chiama `SetAction` — se non lo chiamasse, il dock non popolerebbe nulla;
 *  3. quella chiamata ha il pin `bInArmed`, ed e' **COLLEGATO**;
 *  4. risalendo i dati di quel pin, il valore viene da `AbilityIndex` e **non** dalla posizione del ciclo.
 *
 * 🔑 **Il punto 3 non e' cerimoniale, ed e' un difetto gia' accaduto**: `RTScreenHudWidgets.h` lo scrive
 * due volte — *«il Blueprint passava `false` fisso, e `ActionDockShowsTheNeutralState` era verde»*. Un pin
 * non collegato porta il proprio default, quindi nessuno slot risulterebbe mai armato e il gate al punto 4
 * non avrebbe niente da risalire: sarebbe verde misurando zero, *«che e' il modo in cui un gate diventa
 * decorativo»* (`BlueprintPropertiesExposeNoTexture`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionDockArmedStateReadsAbilityIndexTest,
	"RefactorTactics.Editor.DockArmedStateReadsTheAbilityIndexNotTheLoopPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTActionDockArmedStateReadsAbilityIndexTest::RunTest(const FString&)
{
	using namespace RTActionDockIndexWiring;

	// `UBlueprint` e non `UWidgetBlueprint`: la classe base basta per i grafi, e cosi' l'asserzione non
	// dipende da `UMGEditor`.
	UBlueprint* Blueprint = Cast<UBlueprint>(
		StaticLoadObject(UBlueprint::StaticClass(), nullptr, ActionDockBlueprintPath));

	if (!TestNotNull(TEXT("1: WBP_RT_ActionDock si carica come Blueprint"), Blueprint))
	{
		return false;
	}

	// ⚠️ **Taratura dell'oracolo, prima del verdetto sul dock.** Senza, un rosso non distingue «il dock legge
	// la posizione del ciclo» da «il riconoscitore di `AbilityIndex` non riconosce piu' niente», e la seconda
	// accuserebbe l'asset di un difetto dell'oracolo.
	//
	// 🔑 **L'asserzione DURA sta sul predicato, non su un asset.** Le due forme sono documentate — `Break` da'
	// `AbilityIndex`, split pin da' `<Struct>_AbilityIndex` — e verificarle qui non dipende da nessun altro
	// file: e' cio' che rende questo gate insensibile al cablaggio di widget che non sono il suo soggetto.
	TestTrue(TEXT("0: l'oracolo riconosce la forma da nodo `Break` — `AbilityIndex`"),
		ELAbilityIndex(TEXT("AbilityIndex")));
	TestTrue(TEXT("0: l'oracolo riconosce la forma da split struct pin — `Action_AbilityIndex`"),
		ELAbilityIndex(TEXT("Action_AbilityIndex")));
	TestFalse(TEXT("0: e NON scambia per AbilityIndex un pin che si chiama soltanto `Index`"),
		ELAbilityIndex(TEXT("Index")));

	// ⚠️ Il conteggio sullo SLOT resta **diagnostico e non bloccante**, ed e' una correzione da code review:
	// legge `WBP_RT_ActionSlot`, che non e' il soggetto di questo gate. Legarci un'asserzione lo avrebbe fatto
	// diventare rosso il giorno in cui lo slot cambia cablaggio — per una ragione che col dock non c'entra.
	if (const UBlueprint* Slot = Cast<UBlueprint>(
		StaticLoadObject(UBlueprint::StaticClass(), nullptr, ActionSlotBlueprintPath)))
	{
		AddInfo(FString::Printf(
			TEXT("taratura (informativa): in WBP_RT_ActionSlot i pin riconosciuti come AbilityIndex sono %d"),
			ContaPinAbilityIndex(Slot)));
	}

	const TArray<UK2Node_CallFunction*> Chiamate = ChiamateA(Blueprint, TEXT("SetAction"));

	if (!TestTrue(
		FString::Printf(
			TEXT("2: il grafo del dock chiama SetAction (ne trova %d). Senza quella chiamata il dock non ")
			TEXT("popola nessuno slot, e lo Step 7.3 di #613 la prescrive"),
			Chiamate.Num()),
		Chiamate.Num() > 0))
	{
		return false;
	}

	int32 ChiamateConPinCollegato = 0;
	int32 ChiamateCheLeggonoAbilityIndex = 0;
	int32 ChiamateCheLeggonoLaPosizione = 0;
	int32 ChiamateIndeterminate = 0;

	for (int32 I = 0; I < Chiamate.Num(); ++I)
	{
		UK2Node_CallFunction* Call = Chiamate[I];
		UEdGraphPin* Armato = Call->FindPin(TEXT("bInArmed"), EGPD_Input);

		if (!Armato)
		{
			AddError(FString::Printf(
				TEXT("3: la chiamata a SetAction #%d non ha il pin bInArmed. La firma di ")
				TEXT("`URTActionSlotWidget::SetAction` lo dichiara: se il nome e' cambiato, questo gate va ")
				TEXT("aggiornato insieme alla firma."),
				I));
			continue;
		}

		if (Armato->LinkedTo.Num() == 0)
		{
			AddError(FString::Printf(
				TEXT("3: il pin bInArmed della chiamata #%d NON e' collegato: porta il default '%s', ")
				TEXT("quindi lo stato armato non arriva dal gioco. E' il difetto che `RTScreenHudWidgets.h` ")
				TEXT("dichiara gia' accaduto — «il Blueprint passava `false` fisso, e ")
				TEXT("`ActionDockShowsTheNeutralState` era verde»."),
				I, *Armato->DefaultValue));
			continue;
		}

		++ChiamateConPinCollegato;

		FEsitoRisalita Esito;
		TSet<const UEdGraphNode*> Visti;
		RisaliDati(Armato, Esito, Visti);

		AddInfo(FString::Printf(TEXT("=== SetAction #%d: risalita di bInArmed (%d pin) ==="),
			I, Esito.PinVisitati));
		for (const FString& Traccia : Esito.Tracce)
		{
			AddInfo(FString::Printf(TEXT("    %s"), *Traccia));
		}

		// Controprova della premessa, per chiamata: una risalita che non attraversa nulla non ha misurato
		// il grafo, e un verdetto costruito su di essa sarebbe un'invenzione.
		if (!TestTrue(
			FString::Printf(
				TEXT("4: la risalita di bInArmed #%d ha attraversato dei pin (ne ha %d)"),
				I, Esito.PinVisitati),
			Esito.PinVisitati > 0))
		{
			continue;
		}

		if (Esito.bLeggeAbilityIndex)
		{
			++ChiamateCheLeggonoAbilityIndex;
		}
		if (Esito.bLeggePosizioneDelCiclo)
		{
			++ChiamateCheLeggonoLaPosizione;
		}
		if (!Esito.bLeggeAbilityIndex && !Esito.bLeggePosizioneDelCiclo)
		{
			++ChiamateIndeterminate;
		}
	}

	if (!TestTrue(
		FString::Printf(
			TEXT("3: almeno una chiamata a SetAction riceve bInArmed da un collegamento (ne ha %d su %d)"),
			ChiamateConPinCollegato, Chiamate.Num()),
		ChiamateConPinCollegato > 0))
	{
		return false;
	}

	// ⚠️ **Il caso indeterminato si DICHIARA, non si arrotonda.** Se il valore non viene ne' da
	// `AbilityIndex` ne' dalla posizione del ciclo, questo oracolo non sa dire quale sia — e chiamarlo
	// verde significherebbe presidiare una cosa che non ha visto. `RTMainMenuEntryWiringTests.cpp` fa la
	// stessa scelta quando un pin risulta calcolato: sposta il verdetto invece di inventarlo.
	if (ChiamateIndeterminate > 0 && ChiamateCheLeggonoAbilityIndex == 0
		&& ChiamateCheLeggonoLaPosizione == 0)
	{
		AddError(FString::Printf(
			TEXT("4: per %d chiamate lo stato armato non risale ne' ad `AbilityIndex` ne' alla posizione ")
			TEXT("del ciclo: il grafo lo calcola per una via che questo gate non riconosce. Leggi la ")
			TEXT("risalita nel log qui sopra e aggiorna l'oracolo, oppure sposta il verdetto su ")
			TEXT("`PIE-V01-HUD`. NON e' un PASS: il tratto resta non misurato."),
			ChiamateIndeterminate));
		return false;
	}

	// Il verdetto. `AbilityIndex` deve esserci; la posizione del ciclo non deve.
	TestTrue(
		FString::Printf(
			TEXT("5: lo stato armato risale ad `AbilityIndex` (%d chiamate su %d con pin collegato). ")
			TEXT("`FRTAbilityCooldownView::AbilityIndex` esiste per questo: e' cio' che l'hotkey arma, e ")
			TEXT("leggerlo rende il dock indipendente dall'invariante `Cooldowns[i].AbilityIndex == i`"),
			ChiamateCheLeggonoAbilityIndex, ChiamateConPinCollegato),
		ChiamateCheLeggonoAbilityIndex == ChiamateConPinCollegato);

	TestTrue(
		FString::Printf(
			TEXT("5: lo stato armato NON risale alla posizione del `ForEach` (%d chiamate la usano). Un ")
			TEXT("confronto sulla posizione del ciclo e' corretto solo finche' `#2987` tiene ")
			TEXT("`Cooldowns[i].AbilityIndex == i`: al primo riordino il dock accende il riquadro ")
			TEXT("sbagliato, e nulla smette di compilare"),
			ChiamateCheLeggonoLaPosizione),
		ChiamateCheLeggonoLaPosizione == 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
