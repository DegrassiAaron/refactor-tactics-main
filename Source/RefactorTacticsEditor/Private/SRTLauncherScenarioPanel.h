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
class STableViewBase;
template <typename OptionType> class SComboBox;

/**
 * Il contenuto del tab `RTDevSandboxLauncher` (#1705): due tendine di tag in intersezione, una ricerca
 * testuale, l'elenco che ne risulta e il readout di cio' che lo scenario selezionato contiene.
 *
 * ⚠️ **E' un guscio.** Cio' che puo' sbagliare — restringere, classificare il vuoto, formattare il
 * readout — sta in `FRTLauncherScenarioBrowser`, che un automation test esamina. Qui restano solo la
 * disposizione dei widget, la cache dell'indice e le chiamate all'indice: se una regola compare in questo
 * file invece che li', e' una regola che nessun test vede.
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
	/**
	 * Rilegge l'INDICE: vocabolario dei tag e id che passano i due filtri. Poi ricalcola cio' che si vede.
	 *
	 * ⚠️ **Si chiama solo quando cambia un filtro, mai mentre si digita.** `URTScenarioIndex::ListIds`
	 * passa da `Scan`, che percorre `Scenarios/` e fa il parse di ogni file: novanta letture e novanta
	 * parse, sul game thread. Rifarle a ogni battuta di tasto e' precisamente il guardrail che #1705 vieta
	 * — «nessun asse che richieda di aprire tutti gli scenari a ogni ridisegno» — e nemmeno il ritardo di
	 * 0,25 s di `SSearchBox` lo rende accettabile: rallenta la frequenza, non elimina il costo.
	 *
	 * Il vocabolario si rilegge qui insieme agli id: erano gia' due letture separate, e tenerne una sola
	 * ferma al `Construct` faceva offrire alle tendine dei tag che l'elenco non conosceva piu'.
	 */
	void RefreshFilters();

	/** Applica la sola ricerca alla cache e riclassifica. Nessun accesso al disco: e' cio' che consente di chiamarla a ogni tasto. */
	void RefreshVisible();

	/** Apre lo scenario selezionato **da solo** e ne costruisce il readout. Un'apertura, non novanta. */
	void RefreshReadout();

	/** Dimentica la selezione e il readout insieme. Sempre insieme: vedi `SelectedId`. */
	void ClearSelection();

	/**
	 * Ricostruisce le posizioni del selettore di prospettiva dalle squadre che l'anteprima sta mostrando
	 * (#1754).
	 *
	 * ⚠️ **Dal DATO, e a ogni scenario.** Le squadre non sono `{0, 1}`: cambiano col file, e un elenco
	 * costruito una volta al `Construct` offrirebbe la squadra di uno scenario precedente. Senza anteprima
	 * resta la sola posizione `Omniscient`, che e' anche cio' che il viewport sta mostrando.
	 */
	void RefreshPerspectiveOptions();

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

	/** Gli id che passano i due tag. **Cache**: costa una scansione del corpus, e si rifa' solo sui filtri. */
	TArray<FString> FilteredIds;

	/**
	 * Un `TSharedPtr` **stabile** per ogni id gia' visto.
	 *
	 * ⚠️ Esiste per la selezione, non per risparmiare allocazioni. `SListView` tiene i selezionati in un
	 * `TSet<TSharedPtr<...>>` e li confronta per **identita' di puntatore**: rigenerando le voci a ogni
	 * refresh, nessun puntatore vecchio si ritrova nella sorgente nuova, e la lista svuota la selezione da
	 * sola — la riga resta elencata ma smette di essere evidenziata, mentre il readout continua a mostrarla.
	 */
	TMap<FString, TSharedPtr<FString>> ItemById;

	TArray<TSharedPtr<FString>> VisibleItems;

	ERTLauncherListState ListState = ERTLauncherListState::Populated;

	/**
	 * ⚠️ **Sopravvive ai filtri.** I filtri sono una lente: restringere l'elenco non deve deselezionare
	 * cio' che si stava guardando, altrimenti cercare un secondo scenario per confronto farebbe perdere
	 * il primo. Cede solo a un gesto esplicito — scegliere un'altra riga, o cliccare nel vuoto — e in quel
	 * caso se ne va **insieme al readout**: una selezione vuota con un readout pieno e' una schermata che
	 * si contraddice.
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

	/**
	 * Le posizioni del selettore di prospettiva: `Omniscient` piu' una per squadra schierata.
	 *
	 * `TSharedPtr<int32>` e non `int32`: `SComboBox` tiene la selezione per **identita' di puntatore**, ed
	 * e' la stessa ragione per cui gli id degli scenari passano da `ItemById`. Il valore e' il `TeamId`,
	 * oppure `RTScenarioKnowledge::OmniscientTeamId`.
	 */
	TArray<TSharedPtr<int32>> PerspectiveOptions;

	TSharedPtr<SComboBox<TSharedPtr<int32>>> PerspectiveCombo;

	TSharedPtr<SListView<TSharedPtr<FString>>> ListView;
};
