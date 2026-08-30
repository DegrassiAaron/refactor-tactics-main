#pragma once

#include "CoreMinimal.h"
#include "RTLauncherScenarioBrowser.h"
// ⚠️ Header intero e non una forward declaration: `TStrongObjectPtr<T>` fa uno `static_assert` che T derivi
// da `UObject`, e su un tipo incompleto quel controllo non si puo' fare — misurato, `C2027` piu' due
// asserzioni fallite. E' la stessa ragione per cui un membro `TStrongObjectPtr` costa un include e un
// puntatore nudo no.
#include "ScenarioHarness/RTScenarioAuthoring.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class ITableRow;
class SSearchBox;
class STableViewBase;
template <typename OptionType> class SComboBox;

/**
 * Il contenuto del tab `RTDevSandboxLauncher` (#1705): due tendine di tag in intersezione, una ricerca
 * testuale, l'elenco che ne risulta e il readout di cio' che lo scenario selezionato contiene.
 *
 * ⚠️ **E' un guscio.** Cio' che puo' sbagliare — restringere, classificare il vuoto, formattare il
 * readout — sta in `FRTLauncherScenarioBrowser`, che un automation test esamina. Qui restano solo la
 * disposizione dei widget e le chiamate all'indice: se una regola compare in questo file invece che li',
 * e' una regola che nessun test vede.
 *
 * ⛔ Nessuna scansione di directory propria: l'elenco viene da `URTScenarioIndex::ListIds`, la stessa
 * funzione del Details Panel di `ARTGameMode`. Un secondo catalogo sarebbe la seconda autorita' che la
 * roadmap vieta, e sarebbe anche il modo piu' facile di farla divergere senza accorgersene.
 */
class SRTLauncherScenarioPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRTLauncherScenarioPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	/** Ricalcola elenco filtrato, elenco visibile e stato vuoto. Non tocca la selezione: vedi `SelectedId`. */
	void RefreshList();

	/** Apre lo scenario selezionato **da solo** e ne costruisce il readout. Un'apertura, non ottantotto. */
	void RefreshReadout();

	TSharedRef<ITableRow> OnGenerateScenarioRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<SWidget> OnGenerateTagOption(TSharedPtr<FString> Option) const;
	void OnScenarioSelected(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	void OnSearchTextChanged(const FText& NewText);

	/** Etichetta di una voce di tendina: la stringa vuota e' il «nessun filtro», e va detto a parole. */
	static FText TagOptionLabel(const TSharedPtr<FString>& Option);

	/** Vocabolario reale dei tag, preceduto dalla voce «tutti». Mai un elenco scritto a mano (#1705 AC). */
	TArray<TSharedPtr<FString>> TagOptions;

	/** I due filtri in intersezione. Stringa vuota = non restringe, ed e' il contratto di `ListIds`. */
	FString FilterA;
	FString FilterB;

	FString SearchText;

	/** Quanti id passavano i soli tag: serve a `Classify` per distinguere le due cause del vuoto. */
	int32 FilteredCount = 0;

	TArray<TSharedPtr<FString>> VisibleItems;

	ERTLauncherListState ListState = ERTLauncherListState::Populated;

	/**
	 * ⚠️ **Sopravvive ai filtri.** I filtri sono una lente: restringere l'elenco non deve deselezionare
	 * cio' che si stava guardando, altrimenti cercare un secondo scenario per confronto farebbe perdere
	 * il primo. Il readout resta finche' non si sceglie qualcos'altro.
	 */
	FString SelectedId;

	TArray<FString> ReadoutLines;

	/** L'errore dell'ultima apertura, quando c'e'. Uno scenario illeggibile resta elencato e lo dice. */
	FString ReadoutError;

	/**
	 * La facade d'authoring usata per leggere il readout.
	 *
	 * ⚠️ `TStrongObjectPtr` e non un puntatore nudo: e' un `UObject` creato su `GetTransientPackage()`, e
	 * senza radice il GC lo raccoglierebbe fra un ridisegno e l'altro — un crash che si manifesta solo
	 * dopo una pausa, cioe' il piu' difficile da attribuire.
	 */
	TStrongObjectPtr<URTScenarioAuthoring> Authoring;

	TSharedPtr<SListView<TSharedPtr<FString>>> ListView;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> FilterABox;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> FilterBBox;
	TSharedPtr<SSearchBox> SearchBox;
};
