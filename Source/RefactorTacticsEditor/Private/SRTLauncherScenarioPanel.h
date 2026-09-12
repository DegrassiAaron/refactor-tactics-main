#pragma once

#include "CoreMinimal.h"
#include "RTLauncherScenarioBrowser.h"
// ⚠️ Header intero e non una forward declaration: `TStrongObjectPtr<T>` fa uno `static_assert` che T derivi
// da `UObject`, e su un tipo incompleto quel controllo non si puo' fare — misurato, `C2027` piu' due
// asserzioni fallite. E' la stessa ragione per cui un membro `TStrongObjectPtr` costa un include e un
// puntatore nudo no.
#include "Replay/RTPlaybackSpeed.h" // ERTPlaybackSpeed
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

	/**
	 * Dimentica la selezione, il readout, **la corsa e l'anteprima** insieme. Sempre insieme: vedi `SelectedId`.
	 *
	 * 🔴 La corsa e' entrata in questo elenco con #2836: prima restava, e la riga di trasporto continuava a
	 * descriverla sopra un readout vuoto e senza nessuno scenario a schermo.
	 *
	 * 🔴 **E l'anteprima subito dopo, nello stesso giro.** Questa funzione non la chiudeva e non chiamava
	 * `RefreshReadout()`, che e' l'unica sede che lo fa: dopo `Esegui` e un clic nel vuoto il playback
	 * restava APERTO — i pulsanti di trasporto abilitati, i marcatori che si muovono — sotto un pannello che
	 * dichiarava nessuna selezione. La prima stesura di #2836 rendeva la contraddizione peggiore invece di
	 * toglierla: la riga smetteva di dire dove si era, e i comandi continuavano a funzionare.
	 */
	void ClearSelection();

	/**
	 * Dimentica la corsa e basta: i quattro campi `LastRun*` tornano al loro default.
	 *
	 * 🔑 **Esiste perche' il difetto di #2836 era esattamente «una sede se n'e' dimenticata».** Con
	 * l'azzeramento scritto a mano in ogni punto, aggiungere un quinto campo domani significherebbe
	 * ricordarselo in tutti — ed e' la stessa forma di errore, un giro dopo. Qui c'e' un posto solo.
	 */
	void ForgetLastRun();

	/**
	 * Ricostruisce le posizioni del selettore di prospettiva dalle squadre che l'anteprima sta mostrando
	 * (#1754).
	 *
	 * ⚠️ **Dal DATO, e a ogni scenario.** Le squadre non sono `{0, 1}`: cambiano col file, e un elenco
	 * costruito una volta al `Construct` offrirebbe la squadra di uno scenario precedente. Senza anteprima
	 * resta la sola posizione `Omniscient`, che e' anche cio' che il viewport sta mostrando.
	 */
	void RefreshPerspectiveOptions();

	/**
	 * `Start Session` sullo scenario selezionato (#1682).
	 *
	 * ⚠️ **Non apre niente da solo**: chiede al subsystem, che possiede la sessione, e si limita a mostrare
	 * la frase che torna indietro. La decisione sta in `FRTLauncherWorkspace::DecideStart`, dove un test la
	 * vede; qui resta la disposizione dei widget.
	 */
	FReply OnStartSessionClicked();

	/** Crea uno scenario NUOVO dalla facade e ci apre la sessione. L'id arriva da `NewScenarioId`. */
	FReply OnNewScenarioClicked();

	/**
	 * Esegue lo scenario selezionato e **apre il playback** sulla corsa appena fatta (`#1625`).
	 *
	 * 🔑 E' il soggetto che ai controlli di trasporto mancava: prima di questo pulsante nessuna via
	 * umana eseguiva uno scenario dall'editor — `Run()` viveva solo negli automation test — e un playback
	 * senza una corsa da mostrare non si apre mai.
	 *
	 * ⚠️ **Il draft si riapre e si richiude qui**, come fa gia' la selezione: `OpenPlayback` copia cio' che
	 * gli serve — tracce decodificate e ponte degli id — e non tiene un riferimento alla facade. Il pannello
	 * continua a non possedere una sessione.
	 */
	FReply OnRunScenarioClicked();

	/** La riga di trasporto: esecuzione, passi, riavvolgimento e velocita'. Solo disposizione e chiamate. */
	TSharedRef<SWidget> BuildTransportRow();

	// --- piazzamento delle unita' (#2786) ---------------------------------------------------------------

	/**
	 * La riga di piazzamento: eroe, squadra, cella, facing e i quattro gesti. Solo disposizione.
	 *
	 * ⚠️ **La cella si digita**, e non e' una rinuncia: `DEC-2` della issue tiene il click sul viewport per
	 * #2802, che riusa questo stesso `AddUnit` da un altro ingresso. Due ingressi, un solo mutatore.
	 */
	TSharedRef<SWidget> BuildPlacementRow();

	/**
	 * Il ciclo comune a ogni gesto di piazzamento: apri, muta, **salva**, chiudi, rileggi.
	 *
	 * 🔑 **`SaveInPlace` prima di `Close` non e' prudenza, e' la condizione di esistenza del gesto.** Il
	 * pannello non possiede una sessione — `RefreshReadout` e `OnRunScenarioClicked` aprono e chiudono il
	 * draft ogni volta — quindi una mutazione non salvata muore con la `Close()` che la segue, e il gesto
	 * sembrerebbe non aver fatto niente. E' `DEC-1` della issue, dichiarata invece che dedotta.
	 *
	 * ⚠️ **Il prezzo e' scritto**: ogni gesto tocca il disco, e non c'e' annulla. L'alternativa era tenere
	 * il draft aperto fra un gesto e l'altro, cioe' dare al pannello lo stato che l'invariante gli nega.
	 *
	 * `Mutate` riceve la facade **gia' aperta** sullo scenario selezionato e ne restituisce l'esito; il
	 * messaggio d'errore lo scrive in `OutError`.
	 */
	FReply RunPlacementGesture(TFunctionRef<ERTScenarioAuthoringResult(URTScenarioAuthoring&, FString&)> Mutate);

	/** Schiera una unita' nuova: id coniato, eroe, squadra, cella e facing correnti. */
	FReply OnAddUnitClicked();

	/** Sposta l'unita' selezionata sulla cella corrente. */
	FReply OnMoveUnitClicked();

	/** Ruota l'unita' selezionata sul facing corrente. */
	FReply OnFaceUnitClicked();

	/** Ritira l'unita' selezionata. */
	FReply OnRemoveUnitClicked();

	/** Rilegge eroi e unita' schierate per le due tendine. Chiamata dopo ogni gesto e a ogni selezione. */
	void RefreshPlacementOptions();

	/**
	 * Fa scorrere la riproduzione automatica.
	 *
	 * ⛔ **E' l'unico punto in cui il tempo entra**, e non decide niente: passa il delta al sottosistema, che
	 * lo passa al view model. Nessun esito dipende da quanti frame sono passati — il che e' il guardrail
	 * di sempre, qui reso vero per costruzione invece che per disciplina.
	 */
	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	/** Attiva una superficie del registro. Se non ci riesce, lo scrive invece di non fare niente. */
	FReply OnSurfaceClicked(FName SurfaceKey);

	/**
	 * La riga delle superfici, costruita **iterando il registro**.
	 *
	 * ⛔ Nessun elenco di pulsanti scritto a mano: e' cio' che rende vero l'AC di #1682 — *«aggiungere una
	 * superficie non richiede di modificare nessun criterio»*. Una voce nuova compare qui da sola, e una
	 * pendente compare come etichetta che nomina la issue invece che come pulsante inerte.
	 */
	TSharedRef<SWidget> BuildSurfaceRow();

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

	/**
	 * L'esito dell'ultimo `Start Session`: vuoto finche' nessuno l'ha premuto.
	 *
	 * ⚠️ Tiene sia il rifiuto sia la conferma, e non solo l'errore: dopo un rifiuto corretto e una seconda
	 * pressione riuscita, una casella che mostra solo gli errori resterebbe ferma sull'ultimo e direbbe che
	 * la sessione non e' partita mentre e' aperta.
	 */
	FString SessionMessage;

	/** L'id proposto per uno scenario nuovo. Vuoto = `New Scenario` rifiuta, dicendolo. */
	FString NewScenarioId;

	// --- piazzamento delle unita' (#2786) ---------------------------------------------------------------

	/**
	 * Gli `HeroId` del roster, da `URTScenarioAuthoring::ListHeroIds()`.
	 *
	 * ⛔ **Non un elenco scritto qui.** Un roster copiato nel pannello diverge dal catalogo il giorno che
	 * un eroe entra o cambia nome, e lo fa in silenzio: la tendina offrirebbe un eroe che `AddUnit`
	 * rifiuta, o ne nasconderebbe uno valido.
	 */
	TArray<TSharedPtr<FName>> HeroOptions;

	/** L'eroe scelto. `NAME_None` finche' il roster non e' stato letto: `AddUnit` lo rifiuta, dicendolo. */
	FName PlacementHeroId;

	/**
	 * La squadra su cui schierare. Default **1**, cioe' l'avversario.
	 *
	 * 🔑 Il default non e' `0` per la ragione per cui questa issue esiste: mettere un nemico e' il gesto che
	 * mancava, e farlo costare un cambio di tendina in piu' rispetto a schierare un alleato lo tratterebbe
	 * come il caso raro. Non lo e'.
	 */
	int32 PlacementTeamId = 1;

	/** La cella su cui schierare, in coordinate assiali piu' layer. `DEC-2`: si digita, per ora. */
	FRTCellId PlacementCell = FRTCellId();

	/** L'orientamento iniziale. `ERTHexDirection`, mai un angolo libero. */
	ERTHexDirection PlacementFacing = ERTHexDirection::E;

	/**
	 * Le sei direzioni esagonali, per la tendina.
	 *
	 * ⛔ **Sei, e non un angolo in gradi**: `AddUnit` e `SetUnitFacing` prendono `ERTHexDirection`, e un
	 * campo libero costringerebbe il pannello a decidere a quale direzione arrotondare — una regola di
	 * geometria di lato-strumento, che e' esattamente cio' che `spec-tactical-designer.md` §3 vieta.
	 */
	TArray<TSharedPtr<ERTHexDirection>> FacingOptions;

    /**
	 * Le unita' gia' schierate, per le tendine di sposta/ruota/ritira.
	 *
	 * ⚠️ Si rilegge da `ListUnits()` dopo ogni gesto: e' una **fotografia**, e tenerla ferma la farebbe
	 * puntare a un'unita' che il gesto precedente ha ritirato.
	 */
	TArray<TSharedPtr<FString>> PlacedUnitOptions;

	/** L'unita' bersaglio dei gesti che ne richiedono una. Vuota = quei gesti rifiutano, dicendolo. */
	FString SelectedUnitId;

	/** L'errore dell'ultima apertura, quando c'e'. Uno scenario illeggibile resta elencato e lo dice. */
	FString ReadoutError;

	/**
	 * Cosa ha prodotto l'ultima pressione di `Esegui` sullo scenario ora a schermo (#2788).
	 *
	 * 🔴 **E' l'unica cosa che il sottosistema d'anteprima non puo' dire.** Lui sa se un playback e'
	 * aperto; non sa se qualcuno ha eseguito. Le due domande divergono nel caso che ha ingannato un
	 * lettore: una corsa che non produce turni non apre nessun playback, e la riga di stato invitava a
	 * eseguire uno scenario a chi lo aveva appena eseguito.
	 *
	 * ⚠️ **Vale per la posa corrente e non oltre.** `RefreshReadout()` la dimentica, perche' quella
	 * funzione richiude il playback e rimette a schermo lo schieramento d'authoring: la memoria di una corsa
	 * non deve sopravvivere alla traccia che descriveva.
	 *
	 * 🔴 **E `ClearSelection()` la dimentica dal 2026-09-12** (#2836). Non lo faceva, e `RefreshReadout()`
	 * — l'unica sede che azzerava questi campi — da li' non viene chiamata: dopo `Esegui` e un clic nel
	 * vuoto la riga annunciava una corsa sopra un readout vuoto, senza nessuno scenario selezionato.
	 *
	 * ⛔ **Porta l'esito del REFERTO, non un successo dedotto.** `Blocked` ed `Errored` esistono perche'
	 * `URTScenarioAuthoring::Run` restituisce `Success` anche quando lo scenario si e' fermato su una
	 * capability mancante: quel `Success` dice che l'esecuzione e' avvenuta, e basta.
	 */
	ERTLauncherRunState LastRunState = ERTLauncherRunState::NotRun;

	/** I turni che quella corsa ha GIOCATO, dal referto della facade. Senza significato con `NotRun`. */
	int32 LastRunTurns = 0;

	/**
	 * `FRTScenarioRunReport::bHasTrace` dell'ultima corsa (#2836).
	 *
	 * ⚠️ **E' il solo modo di dire «traccia non riproducibile» senza indovinarlo.** Il playback chiuso ha
	 * cause distinte — anteprima non viva, nessuna unita' posata, traccia illeggibile — e dedurne l'assenza
	 * della traccia accusa la traccia per un difetto del viewport.
	 */
	bool LastRunHasTrace = false;

	/**
	 * Il motivo che il referto porta: `BlockedReason` oppure `ErrorMessage` (#2836).
	 *
	 * ⚠️ **Non finisce in `ReadoutError`**, che si presenta come *«non leggibile: …»* e sostituisce il
	 * readout: uno scenario bloccato si legge benissimo, e nasconderne terreno e squadre toglierebbe di
	 * mezzo proprio cio' che serve a capire dove si e' fermato. Ha la sua riga, sotto il readout.
	 *
	 * ⚠️ `FText` e non `FString`: la compone `FRTLauncherScenarioBrowser`, dove un automation test la vede,
	 * e il pannello la incornicia in un `LOCTEXT`. Testo grezzo qui darebbe una frase mezza tradotta.
	 */
	FText LastRunDetail;

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

	/** Le sei velocita' di `#2095`, nell'ordine che `URTPlaybackSpeedLibrary::AllSpeeds` dichiara. */
	TArray<TSharedPtr<ERTPlaybackSpeed>> SpeedOptions;

	TSharedPtr<SComboBox<TSharedPtr<ERTPlaybackSpeed>>> SpeedCombo;

	TSharedPtr<SListView<TSharedPtr<FString>>> ListView;
};
