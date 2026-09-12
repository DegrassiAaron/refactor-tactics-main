#pragma once

#include "CoreMinimal.h"
// ⚠️ Header intero e non una forward declaration: `FRTLauncherTransportStatus` tiene una
// `FRTReplayPosition` **per valore**, e un tipo incompleto non ha dimensione. Passarla per riferimento
// avrebbe evitato l'include e imposto al chiamante di tenerla viva: per un aggregato che si costruisce
// sullo stack a ogni ridisegno, e' il baratto sbagliato.
//
// ⛔ **Ed e' la ragione per cui questo header sta in `Private/` dal 2026-09-12** (#2836). `RefactorTactics`
// e' dichiarato `PrivateDependencyModuleName` in `RefactorTacticsEditor.Build.cs`: un header PUBBLICO che
// include `Replay/RTReplayViewModel.h` promette a un modulo terzo un percorso di include che quella
// dichiarazione non gli concede. Qui il costo e' zero — misurato, gli unici quattro file che lo includono
// stanno tutti sotto `Private/`:
//
//     git grep -l 'RTLauncherScenarioBrowser.h' -- Source/
//
// e nessuno di loro e' fuori dal modulo. ⚠️ **Non e' un difetto isolato**: altri header di
// `RefactorTacticsEditor/Public/` includono header di `RefactorTactics` allo stesso modo. Quelli hanno un
// owner diverso e restano dove sono; spostare questo non li corregge e non pretende di farlo.
#include "Replay/RTReplayViewModel.h" // FRTReplayPosition

struct FRTScenarioRunReport;
struct FRTScenarioSummary;
struct FRTScenarioUnitView;

/**
 * Perche' l'elenco del launcher e' vuoto (#1705).
 *
 * ⚠️ **Le cause si somigliano sullo schermo e non sono la stessa cosa.** «Nessuno scenario porta i tag
 * scelti» e «la ricerca non trova niente fra quelli che li portano» chiedono due gesti opposti: la prima
 * si cura allargando i filtri, la seconda cancellando la ricerca. Un elenco che dicesse soltanto «vuoto»
 * manderebbe a smontare i filtri chi doveva solo svuotare la casella di testo.
 */
enum class ERTLauncherListState : uint8
{
	/** Ci sono voci da mostrare. */
	Populated,

	/**
	 * L'indice non ha restituito niente **senza che nessun filtro stesse restringendo**.
	 *
	 * ⚠️ Non e' un caso teorico e non e' un doppione di `NoTagMatches`: `Scenarios/` rinominata, un
	 * percorso di progetto diverso o header tutti illeggibili producono un indice vuoto a filtri aperti.
	 * Dire li' «allarga i filtri» manda a cercare una via d'uscita che non esiste — non c'e' niente da
	 * allargare, e la causa e' fuori dal pannello.
	 */
	EmptyCorpus,

	/** I tag scelti non lasciano passare nessuno scenario: la ricerca non c'entra. */
	NoTagMatches,

	/** I tag lasciavano passare qualcosa, e la ricerca testuale l'ha azzerato. */
	NoSearchMatches,
};

/**
 * Cosa il pulsante `Esegui` ha prodotto l'ultima volta (#2788).
 *
 * ⚠️ **Non e' «il playback e' aperto».** Quella domanda la risponde gia' il sottosistema d'anteprima a ogni
 * frame, e la riga di stato del trasporto la leggeva da sola: e' precisamente il motivo per cui `Esegui`
 * risultava muto. *«Il playback e' aperto»* e *«una corsa e' avvenuta»* sono due domande, e la prima non
 * risponde alla seconda — una corsa che non produce turni non apre nessun playback, e la riga diceva
 * «esegui uno scenario» a chi lo aveva appena eseguito.
 *
 * ⛔ **Nessun valore qui significa PASS o FAIL.** L'esito delle assertion sta in
 * `FRTScenarioRunReport::Outcome` e non e' cio' che il trasporto descrive: uno scenario che fallisce le
 * sue attese ha comunque prodotto una traccia da riprodurre, e sono due letture diverse dello stesso
 * gesto.
 *
 * 🔴 **Ma `Blocked` e `Error` SI'** (#2836), ed e' la distinzione che mancava. `Ran` copriva tutti e
 * quattro gli esiti del referto, e con `TurnsPlayed == 0` la riga diceva *«Corsa eseguita: nessun turno da
 * riprodurre.»* — cioe' affermava un successo — su uno scenario **bloccato** da una capability mancante o
 * andato in **errore**. Il difetto che #2788 correggeva era muto; questo era assertivo, che e' peggio: una
 * riga che non dice niente lascia a cercare, una che dichiara il falso manda altrove con una certezza.
 */
enum class ERTLauncherRunState : uint8
{
	/** Nessuna corsa da quando il pannello ha posato questo scenario. */
	NotRun,

	/**
	 * La facade ha rifiutato: scenario non apribile, non valido, o corsa non eseguibile.
	 *
	 * ⚠️ Distinto da `NotRun` e non fuso con esso: dopo un rifiuto la riga direbbe altrimenti «esegui uno
	 * scenario» a chi lo ha appena eseguito senza successo, cioe' inviterebbe a ripetere il gesto invece di
	 * mandare a leggere il perche'.
	 *
	 * ⛔ **Non e' un esito del gioco**: qui la corsa non e' proprio avvenuta. `Blocked` e `Errored` sono
	 * l'opposto — l'esecuzione c'e' stata e il REFERTO dice com'e' finita.
	 */
	Failed,

	/**
	 * La corsa e' avvenuta e il referto la conferma: `Pass` oppure `Fail`.
	 *
	 * ⚠️ **`Fail` sta qui, e non e' una svista.** `RTScenarioAuthoring.h` lo dichiara per la facade —
	 * *«un `FAIL` e' l'informazione piu' preziosa che questo strumento produce, e farlo apparire come un
	 * guasto dello strumento la butterebbe via»* — e vale identico per la riga di trasporto: uno scenario
	 * che fallisce le sue attese ha giocato i suoi turni e ha una traccia da riprodurre. L'esito lo dice il
	 * referto, non il trasporto.
	 */
	Ran,

	/**
	 * L'esecuzione e' avvenuta e si e' fermata su una **capability non ancora costruita** (`ERTTestOutcome::Blocked`).
	 *
	 * 🔴 Aggiunto da #2836. Prima cadeva in `Ran`, e a zero turni — che e' il caso in cui
	 * `RTScenarioSession` lo definisce — diventava indistinguibile da uno scenario legittimamente senza
	 * turni. `FRTScenarioRunReport::BlockedReason` dice quale capability manca, e senza questo valore
	 * nessuno lo andava a leggere.
	 */
	Blocked,

	/**
	 * L'esecuzione e' avvenuta e lo scenario non ha potuto giocare (`ERTTestOutcome::Error`).
	 *
	 * 🔴 Aggiunto da #2836, per la stessa ragione di `Blocked`: e' un difetto **del test**, non del gioco,
	 * e `FRTScenarioRunReport::ErrorMessage` dice quale. Presentarlo come una corsa riuscita manda a
	 * cercare un bug di gioco che non c'e'.
	 */
	Errored,
};

/**
 * I fatti da cui la riga di stato del trasporto si scrive (#2788).
 *
 * 🔑 **Due sorgenti, e nessuna delle due e' il widget.** `Run` e `TurnsPlayed` vengono dal referto che la
 * facade restituisce a `URTScenarioAuthoring::Run`; `bPlaybackOpen` e `Position` dal sottosistema
 * d'anteprima, che li possiede. Il pannello li mette in questo aggregato e non ne deriva nessuno: se un
 * giorno il conteggio dei turni comparisse calcolato qui, sarebbe una seconda autorita' su un numero che
 * il runner ha gia' misurato.
 */
struct FRTLauncherTransportStatus
{
	ERTLauncherRunState Run = ERTLauncherRunState::NotRun;

	/** I turni che la corsa ha GIOCATO — `FRTScenarioRunReport::TurnsPlayed`, non i turni authorati. */
	int32 TurnsPlayed = 0;

	/**
	 * C'e' una traccia confrontabile — `FRTScenarioRunReport::bHasTrace`, cioe' cio' che il RUNNER dichiara.
	 *
	 * 🔴 **Esiste perche' `!bPlaybackOpen` non e' un sinonimo di «traccia assente»** (#2836).
	 * `URTScenarioPreviewSubsystem::OpenPlayback` rifiuta per cause distinte — nessuna anteprima viva,
	 * nessuna unita' posata, tracce vuote, una traccia che non si deserializza, la navigazione che non si
	 * apre — e dedurne *«traccia non riproducibile»* accusa la traccia per un'anteprima morta. Il segnale
	 * autorevole e' questo, e va passato invece che indovinato.
	 */
	bool bHasTrace = false;

	/**
	 * C'e' uno scenario selezionato nel pannello.
	 *
	 * 🔴 **Senza, la riga descriveva la corsa di uno scenario che non e' piu' a schermo** (#2836).
	 * `ClearSelection()` dimenticava il readout e non la corsa: dopo `Esegui` su
	 * `Visual.Map.TwoLayersSameColumn` e un clic nel vuoto, la riga continuava a dire *«Corsa eseguita»*
	 * sopra un readout vuoto e senza nessuna selezione. `false` e' il default apposta: uno stato che non
	 * dichiara una selezione non ne ha una, e sbagliare in questo verso tace invece di mentire.
	 */
	bool bScenarioSelected = false;

	bool bPlaybackOpen = false;

	/** Ha senso solo con `bPlaybackOpen`: a playback chiuso resta il default e non va letta. */
	FRTReplayPosition Position;
};

/**
 * La parte di #1705 che si puo' misurare senza un editor vivo.
 *
 * Stessa scelta di `URTDevSandboxLauncherSubsystem::ShouldOpenFor` (#1680), e per la stessa ragione: di
 * una slice fatta di Slate, cio' che un automation test vede e' solo la decisione. Tenendo qui la ricerca,
 * la classificazione del vuoto e la formattazione del readout, il pannello resta un guscio che dispone
 * widget — e cio' che puo' sbagliare ha un test.
 *
 * ⛔ **Niente qui tocca il disco.** L'elenco filtrato arriva gia' fatto da `URTScenarioIndex::ListIds`,
 * che e' la stessa funzione del Details Panel di `ARTGameMode`: un secondo catalogo sarebbe la seconda
 * autorita' che la roadmap vieta, e queste funzioni non sono in condizione di crearne uno nemmeno per
 * sbaglio — non sanno da dove vengano gli id che ricevono.
 */
class FRTLauncherScenarioBrowser
{
public:
	/**
	 * Restringe per sottostringa l'elenco **gia' filtrato per tag**, senza riordinarlo.
	 *
	 * ⚠️ **Additiva per costruzione, non per disciplina.** Il risultato e' sempre un sottoinsieme di cio'
	 * che entra, perche' la funzione non sa da nessuna parte dove pescare altri id: e' il modo piu' solido
	 * di rispettare l'AC «la ricerca restringe l'elenco filtrato». Una ricerca che ripartisse dall'indice
	 * potrebbe far ricomparire scenari che i tag avevano escluso, e sembrerebbe funzionare.
	 *
	 * Ricerca vuota = identita', quindi «cercare a filtri vuoti cerca su tutti» non e' un caso speciale ma
	 * la composizione di due identita'. Confronto senza distinzione di maiuscole: gli id sono scritti in
	 * `Camel.Case` (`Movement.Basic`) e chi cerca digita `movement`.
	 */
	static TArray<FString> ApplySearch(const TArray<FString>& FilteredIds, const FString& Search);

	/**
	 * Quale causa ha svuotato l'elenco.
	 *
	 * ⚠️ **Prende due conteggi e non gli elenchi**, perche' la distinzione fra le due cause interne sta nel
	 * confronto fra il prima e il dopo della ricerca: `VisibleCount` e' quanto resta, `FilteredCount`
	 * quanto c'era prima di cercare. Con il solo elenco finale la domanda non e' rispondibile.
	 *
	 * ⚠️ **E `bAnyTagFilter` non e' deducibile dai conteggi**: zero id a filtri aperti e zero id perche' i
	 * filtri hanno escluso tutto sono lo stesso numero e due situazioni diverse. Senza questo terzo dato la
	 * funzione attribuirebbe ai filtri un vuoto che i filtri non hanno causato — cioe' proprio l'errore che
	 * l'enum esiste per impedire.
	 */
	static ERTLauncherListState Classify(int32 FilteredCount, int32 VisibleCount, bool bAnyTagFilter);

	/** Il messaggio che il pannello mostra al posto della lista. Vuoto quando `Populated`. */
	static FText DescribeEmptyState(ERTLauncherListState State);

	/**
	 * Il terreno **come lo scenario lo dichiara**: `fixture <nome>` oppure `radius <n>`.
	 *
	 * ⛔ **Nessuna traduzione dei due in un terzo vocabolario**, che e' l'AC esplicito di #1705: un
	 * allestimento salvato e un raggio da cui generare l'arena non sono due modi di dire la stessa cosa, e
	 * un readout che li appiattisse su «mappa: media» toglierebbe di mezzo proprio l'informazione per cui
	 * si guarda il readout prima di aprire.
	 *
	 * ⚠️ Il corpus li ha entrambi — **misurato il 2026-08-30**: 90 scenari, 21 con una fixture e 69 con un
	 * raggio — quindi nessuno dei due rami e' teorico. Se un giorno uno scenario non dichiarasse nessuno
	 * dei due, questa funzione lo dice invece di scegliere un default: un raggio `0` inventato sarebbe
	 * indistinguibile da un raggio `0` vero.
	 */
	static FString DescribeTerrain(const FRTScenarioSummary& Summary);

	/**
	 * La composizione per squadra, nell'ordine dei `TeamId`: `team 0: 2 · team 1: 2`.
	 *
	 * ⚠️ **E' un readout, non una colonna della lista.** Si legge dopo l'apertura di UNO scenario perche'
	 * `FRTScenarioUnitView` esiste solo a scenario aperto: farne un asse di filtro imporrebbe di aprirli
	 * tutti a ogni ridisegno, che e' il guardrail per cui `ReadHeader` esiste.
	 */
	static FString DescribeComposition(const TArray<FRTScenarioUnitView>& Units);

	/**
	 * Le righe del readout, nell'ordine in cui il pannello le mostra.
	 *
	 * Tutto viene da `FRTScenarioSummary` e dalle viste: nessuna aritmetica propria, nessun conteggio
	 * ricavato altrove. Se un numero e' sbagliato, e' sbagliato a monte — e questo e' voluto.
	 */
	static TArray<FString> BuildReadout(const FRTScenarioSummary& Summary, const TArray<FRTScenarioUnitView>& Units);

	/**
	 * Un `UnitId` che non collide con nessuno di quelli gia' schierati (#2786).
	 *
	 * ⚠️ **Esiste perche' `AddUnit` prende l'id in INGRESSO e non lo restituisce** (#1115): il conio e' del
	 * chiamante, e se il chiamante e' una schermata il difetto diventa suo. Un id che collide torna come
	 * `Invalid` con «id gia' preso» — un messaggio che accusa lo SCENARIO per un errore della SCHERMATA, e
	 * manda chi legge a cercare nel posto sbagliato.
	 *
	 * Forma `U<n>`, con `n` il primo intero libero da `1`. **Riempie i buchi**: rimossa `U2`, la prossima
	 * unita' torna `U2`. E' voluto — un contatore monotono richiederebbe di ricordare quante unita' sono
	 * passate, cioe' uno stato che ne' il draft ne' il pannello possiedono, e che dopo un salva/riapri
	 * ripartirebbe da capo producendo la collisione che questa funzione esiste per evitare.
	 *
	 * ⛔ **Non guarda la forma degli id esistenti.** Uno scenario scritto a mano puo' chiamarle `attaccante`
	 * o `hero_a`: cercare il primo `U<n>` libero non collide con nessuna di quelle, e non prova a
	 * indovinare una convenzione che il formato non impone.
	 */
	static FString CoinUnitId(const TArray<FRTScenarioUnitView>& Existing);

	/**
	 * L'etichetta di una posizione del selettore di prospettiva (#1754): `Omniscient` oppure `Team N`.
	 *
	 * ⚠️ **`N` e' l'id della squadra, non la sua posizione nel selettore.** Uno scenario che schiera le
	 * squadre `0` e `3` mostra `Team 0` e `Team 3`: numerarle `1` e `2` perche' sono la prima e la seconda
	 * voce le renderebbe irriconoscibili accanto a `rt.Debug.Knowledge <team>`, che e' l'oracolo con cui il
	 * designer le confronta.
	 *
	 * `Omniscient` e' una posizione **nominata**, e la sua etichetta lo dice: non «nessun filtro», non
	 * «tutti», che leggerebbero come l'assenza di una scelta invece che come una scelta.
	 */
	static FText DescribePerspective(int32 TeamId);

	/**
	 * Dove il playback e' arrivato, in una riga.
	 *
	 * ⚠️ **Si guarda `State`, non i valori.** `TurnNumber` e `Phase` portano un default anche quando non
	 * significano niente — prima dell'inizio e a fine partita — e stamparli comunque direbbe «turno 0, fase
	 * Planning» come se fosse un istante della partita.
	 *
	 * 🔑 **Sta qui e non nel pannello dal 2026-09-10** (#2788). Ci viveva come funzione libera in un
	 * anonimo, dove nessun automation test la raggiungeva — e il commento nel suo corpo lo dichiarava,
	 * accanto a un difetto che solo una seduta in Editor aveva potuto vedere. La stessa frase, dallo stesso
	 * posto, ora ha un test.
	 */
	static FText DescribePlaybackPosition(const FRTReplayPosition& Position);

	/**
	 * Quale stato di corsa il REFERTO descrive (#2836).
	 *
	 * 🔴 **E' la traduzione che il pannello faceva per omissione**, e che sbagliava: qualunque
	 * `Run() == Success` diventava `Ran`, mentre `Success` significa soltanto che *«l'esecuzione e'
	 * avvenuta»* — `RTScenarioAuthoring.h` lo dichiara a parole. L'esito del gioco sta in
	 * `FRTScenarioRunReport::Outcome`, e a zero turni `Blocked` ed `Error` sono proprio i casi che
	 * `RTScenarioSession` produce.
	 *
	 * ⛔ **`Fail` torna `Ran`, deliberatamente.** Un `FAIL` e' un difetto del GIOCO su una partita che si e'
	 * giocata: ha i suoi turni e la sua traccia, e travestirlo da guasto dello strumento butterebbe via
	 * l'informazione piu' preziosa che questo strumento produce.
	 *
	 * ⚠️ Prende il referto per riferimento costante e non per valore: `FRTScenarioRunReport` porta gli array
	 * delle assertion e delle note, e qui non se ne legge nessuno.
	 */
	static ERTLauncherRunState ClassifyRun(const FRTScenarioRunReport& Report);

	/**
	 * Il motivo che il referto porta per gli esiti che ne hanno uno: `BlockedReason` oppure `ErrorMessage`.
	 * Stringa vuota per `Pass` e `Fail`, che non ne hanno (#2836).
	 *
	 * 🔑 **Serve a far arrivare al readout il campo che il pannello scartava.** Senza, il designer legge
	 * *«Corsa BLOCCATA»* e non sa **quale** capability manchi — che e' l'unica cosa da sapere per andare
	 * avanti, e che il referto gia' contiene.
	 *
	 * ⚠️ Un esito che dichiara il motivo e non lo porta lo DICE, invece di restituire una stringa vuota che
	 * a schermo diventa una riga assente: un referto incompleto e' un difetto, e va visto.
	 */
	static FString DescribeRunDetail(const FRTScenarioRunReport& Report);

	/**
	 * La riga di stato del trasporto: cosa il campo sta mostrando, e da quale gesto viene (#2788).
	 *
	 * 🔴 **La domanda a cui risponde non e' «dove sono nella traccia», e' «cosa sto guardando».** Prima
	 * rispondeva solo alla prima, e le tre risposte che contano collassavano: una posa d'authoring senza
	 * corsa, una corsa che non ha prodotto turni e una corsa la cui traccia comincia dalla posa si
	 * leggevano tutte come *«Posa iniziale.»* oppure come *«Nessun playback: esegui uno scenario.»* — cioe'
	 * come un pulsante che non fa niente. Misurato in Editor il 2026-09-09 su `AutoBattle.ArenaV01`: il
	 * click e' stato classificato non funzionante per due tentativi consecutivi.
	 *
	 * ⛔ **Non decide se il playback e' apribile.** Riceve `bPlaybackOpen` gia' misurato dal sottosistema:
	 * una funzione pura che provasse a dedurlo dai turni direbbe «aperto» per una traccia che non si e'
	 * decodificata.
	 *
	 * 🔴 **E non afferma un esito che il referto non conferma** (#2836). La frase si compone di due
	 * proposizioni — cosa il gesto ha prodotto, e cosa c'e' da guardare — e ogni valore di
	 * `ERTLauncherRunState` ha la sua: `Blocked` ed `Errored` si leggono come tali anche a zero turni,
	 * dove prima diventavano *«Corsa eseguita»*.
	 */
	static FText DescribeTransport(const FRTLauncherTransportStatus& Status);
};
