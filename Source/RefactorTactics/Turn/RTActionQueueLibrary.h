#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTActionQueue.h"
#include "Turn/RTTurnRules.h" // ERTMatchPhase: le chiavi d'ordine partono dalla macro-fase
#include "RTActionQueueLibrary.generated.h"

class ARTUnit;

/**
 * Le chiavi con cui UN'UNITA' entra nell'ordine di risoluzione — cella, identita' stabile, nome dell'Actor.
 *
 * 🔑 **E' una struct e non tre parametri sciolti** perche' la chiave e' cio' che va tenuto insieme: un
 * comparatore che ne prende sei allineati e' il posto in cui prima o poi si invertono due argomenti dello
 * stesso tipo. E perche' un test possa costruirla **senza un mondo**: `ARTUnit` e' un Actor, e la regola
 * d'ordine non ha bisogno di uno per essere provata.
 *
 * ⛔ Nessun puntatore, nessun ordine di spawn, nessun indice di registrazione: sono i tre input che
 * `URTActionQueueLibrary::UnitOrderLess` esiste per tenere fuori dalla decisione.
 */
struct FRTUnitOrderKey
{
	/** Prima chiave: la cella logica, confrontata con `URTHexLibrary::StableLess`. */
	FRTCellId Cell;

	/** Identita' di partita ([D-063]). `0` significa «non ancora assegnata», non «unita' zero». */
	int32 StableUnitId = 0;

	/**
	 * Ultimo spareggio, per il solo caso in cui `StableUnitId` non esista ancora.
	 *
	 * 🔴 **Questa chiave separa due unita' solo se i loro NOMI sono diversi**, e la premessa che lo
	 * garantisce va detta con il modo di verificarla — perche' e' esattamente cio' che manco' a `#990`.
	 *
	 * L'unicita' dei nomi `UObject` vale **dentro un solo Outer**, e per un Actor l'Outer e' la `ULevel`. ∴ la
	 * chiave e' un ordine totale finche' **tutte le unita' vivono in un solo livello**. Oggi e' vero: sono
	 * spawnate nel livello persistente (`FRTMatchBootstrapper`, `FRTScenarioSession`) e il progetto non carica
	 * sublevel. La premessa si ricontrolla cosi':
	 *
	 *     git grep -n "SpawnActor.*ARTUnit" -- Source/RefactorTactics    # da dove nascono
	 *     git grep -n "LoadStreamLevel\|ULevelStreaming" -- Source/RefactorTactics   # deve essere vuoto
	 *
	 * ⛔ **Il giorno in cui cadesse, questa chiave non basta piu' e nessuna sua variante aiuta**: due Actor in
	 * livelli diversi possono avere nomi identici byte per byte, e li' non c'e' confronto di stringhe che
	 * separi. Servirebbe una quarta chiave davvero unica — che oggi non esiste e **non va inventata qui**.
	 *
	 * ⚠️ **E nei target Runtime la coppia `Unit_Alpha`/`UNIT_ALPHA` non e' nemmeno rappresentabile come due
	 * nomi**: `WITH_CASE_PRESERVING_NAME` vale `WITH_EDITORONLY_DATA`, e l'engine lo documenta —
	 * *«enabled for the Editor and any Programs (such as UHT), but not the Runtime»* (`NameTypes.h:25-30`).
	 * Senza preservazione del caso i due registrano sullo **stesso** `FNameEntry`, e `GetName()` restituisce la
	 * stessa stringa a entrambi. ∴ il case-sensitive di questa chiave conta **in Editor**, dove gira
	 * l'automation; in packaged e' la premessa dell'Outer unico a reggere tutto. Una stesura precedente
	 * affermava che il case-sensitive chiudesse il buco *e basta*: falso fuori dall'Editor. Trovato in code
	 * review, dopo il merge.
	 *
	 * ⛔ `operator<` resta comunque escluso: `FString::UEOpLessThan` e' `Stricmp(...) < 0`, quindi non e' un
	 * ordine totale sui byte nemmeno dove il caso e' preservato. E' lo stesso difetto che
	 * `URTTurnLogLibrary::EntryLess` ha gia' pagato sulla v10 del TurnLog.
	 *
	 * 🔑 Il costo di `AActor::GetName()` — che passa da `FName::ToString()` e alloca — si paga **una volta
	 * per unita'**, non a ogni confronto: `SortUnitsForResolution` costruisce le chiavi prima di ordinare.
	 */
	FString ActorName;

	FRTUnitOrderKey() = default;
	FRTUnitOrderKey(const FRTCellId& InCell, int32 InStableUnitId, FString InActorName)
		: Cell(InCell), StableUnitId(InStableUnitId), ActorName(MoveTemp(InActorName)) {}
};

/**
 * Ordine di risoluzione di un turno: quello delle **azioni** e quello delle **unita'**, deterministici
 * entrambi.
 *
 * ⚠️ **La regola e' pura; due funzioni di comodo no.** `MakeUnitOrderKey` e `SortUnitsForResolution`
 * prendono `ARTUnit`, e l'header lo dichiara in avanti invece di includerlo. Fino a #2922 questa classe
 * prometteva *«senza Actor»*: la promessa e' stata **ritirata**, non violata in silenzio. Cio' che resta
 * vero, ed e' la parte che conta, e' che `InstanceLess` e `UnitOrderLess` — le due regole — non toccano
 * nessun Actor e si provano senza un mondo.
 *
 * Una sola sede per la regola d'ordine, come `URTTurnLogLibrary` lo e' per il TurnLog. Se la scelta di
 * "chi risolve prima" fosse sparsa fra le fasi, due punti del codice potrebbero divergere e l'esito di un
 * turno dipenderebbe da quale ha ragione.
 */
UCLASS()
class REFACTORTACTICS_API URTActionQueueLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Ordine fra due azioni pianificate. Vero se A risolve prima di B.
	 *
	 * Chiavi, in ordine: **macro-fase di Atlas** (`Prep -> Dash -> Blast -> Move -> Cleanup`) -> `Priority`
	 * intera crescente -> `ActionId` -> `SourceUnitId` -> `EventSequence` -> `TargetUnitId` -> `TargetCell`
	 * (`URTHexLibrary::StableLess`) -> `bInterrupted`.
	 *
	 * ⚠️ **TOTALE sulle chiavi dichiarate, NON esaustivo su `FRTActionInstance`**, e la distinzione va tenuta
	 * perche' due stesure precedenti hanno scritto qui la parola «totale» senza qualificarla. Di `Def` il
	 * confronto guarda tre sottocampi — `ResolutionPhase`, `Priority`, `ActionId` — e non il resto: due
	 * istanze che differiscono solo per `Def.Effects` o `Def.RangeCells` restano a pari merito. La premessa
	 * che lo rende sufficiente, e il giorno in cui cade, stanno scritte in fondo a `InstanceLess`.
	 *
	 * 🔴 **Fino a #2970 le chiavi erano cinque, e non erano un ordine totale.** `FRTActionInstance` ne porta
	 * altri tre che DISTINGUONO due istanze — `TargetUnitId`, `TargetCell`, `bInterrupted` — e il confronto
	 * non li guardava: due azioni della stessa unita', stessa `ActionId`, stessa fase, pari priorita' e
	 * stesso `EventSequence` ma bersaglio diverso rispondevano `false` nei DUE versi. A deciderle restava
	 * `TArray::Sort`, che inoltra ad `Algo::Sort` — introsort, NON stabile — cioe' l'ordine d'arrivo nel
	 * container. E' la stessa classe di difetto che `EntryLess` ha gia' pagato due volte sui campi
	 * serializzati del TurnLog.
	 *
	 * 🔑 **Le tre chiavi in coda sono spareggi TECNICI, non una priorita' di gioco**, e stanno DOPO fase e
	 * priorita' apposta: dove il comparatore non pareggiava, l'ordine non si sposta di una posizione — ed e'
	 * `Actions.GameplayKeysStillBeatTechnicalTieBreaks` a dirlo, non questo commento. Cio' che stabiliscono e'
	 * soltanto che l'esito non dipenda dall'ordine d'inserimento (`CLAUDE.md` §11).
	 *
	 * ⚠️ **Il gate che impedisce il terzo giro e' `Actions.CanonicalOrderCoversInstanceFields`**, gemello di
	 * `TurnLog.CanonicalOrderCoversSerializedFields`: un campo discriminante lasciato fuori lo fa cadere.
	 *
	 * La macro-fase viene PRIMA della priorita': un `Action.Move` a priorita' 50 risolve dopo un attacco a
	 * priorita' 80, perche' il Move e' una fase successiva (ADR-0003 §1 — il catalogo v0.1 metteva invece il
	 * movimento prima dell'attacco). La priorita' ordina DENTRO la fase, non fra le fasi.
	 *
	 * E' l'UNICO ordine di risoluzione del gioco. Questo commento diceva di «estendere la regola APNAP di
	 * `piano-canonico-mvp.md §5.1`»: quella regola e' stata **ritirata** da D-293 (2026-08-31) perche'
	 * presupponeva un'unita' attiva che un gioco a turni simultanei non ha. Non esiste un secondo ordine da
	 * estendere, e non va costruito.
	 *
	 * Nessuna chiave e' un float; nessuna dipende dall'ordine di una `TMap`.
	 */
	static bool InstanceLess(const FRTActionInstance& A, const FRTActionInstance& B);

	/** Ordina in place con `InstanceLess`: permutare l'ingresso non cambia la sequenza risolta. */
	static void SortActionInstances(TArray<FRTActionInstance>& Instances);

	/**
	 * Le sole istanze che risolvono nella macro-fase indicata, nell'ordine di risoluzione.
	 * Il chiamante lavora una fase per volta senza rifiltrare a mano (e senza sbagliare la rimappatura).
	 */
	static TArray<FRTActionInstance> InstancesForPhase(const TArray<FRTActionInstance>& Instances,
		ERTMatchPhase Phase);

	// --- Ordine delle UNITA' (#2922) --------------------------------------------------------------------

	/**
	 * Ordine TOTALE fra due unita' nella risoluzione. Vero se A entra prima di B.
	 *
	 * Chiavi, in ordine: **cella** (`URTHexLibrary::StableLess`) -> **`StableUnitId`** -> **nome dell'Actor**.
	 *
	 * 🔴 **La cella da sola non e' un ordine totale, e per anni si e' creduto che lo fosse.** `#990` lo
	 * scrisse esplicitamente — *«`StableLess` sulla cella e' un ordine totale sulle unita' vive, perche' due
	 * unita' vive non condividono una cella»* — e `#1733`/`#1970` hanno misurato il contrario: la
	 * sovrapposizione esiste, `URTHexSimLibrary::MakeSnapshot` la **registra** in `FRTHexSnapshot::Overlaps`
	 * e `ARTTurnManager::ReportSnapshotOverlaps` la segnala a runtime. L'invariante e' un rilevatore di
	 * difetto, non una garanzia, e un comparatore non puo' poggiarci sopra.
	 *
	 * ⚠️ **Su un pareggio decideva `GetAllActorsOfClass`**: `TArray::Sort` inoltra ad `Algo::Sort` — introsort,
	 * NON stabile — quindi l'ordine d'ingresso sopravvive. Cioe' esattamente l'input che `#990` aveva tolto
	 * dalla decisione, rientrato da un anello piu' in la': l'indice in `CollectLivingUnits` **e'**
	 * `FRTHexSimUnit::UnitId`, e `MakeSnapshot` tiene come occupante l'`UnitId` minore.
	 *
	 * 🔑 **La cella resta la PRIMA chiave**: dove le celle differiscono l'ordine non si sposta di una
	 * posizione. Le due chiavi in coda sono spareggi TECNICI, non una priorita' di gioco: chi vince una
	 * sovrapposizione resta l'errore strutturale che `ReportSnapshotOverlaps` segnala, e qui si stabilisce
	 * soltanto che l'esito non dipenda dall'ordine di registrazione degli Actor (`CLAUDE.md` §11).
	 *
	 * ⚠️ **Dove le celle pareggiano, invece, l'ordine CAMBIA rispetto a prima** — da arbitrario a
	 * deterministico. Il caso e' raggiungibile senza patologie: una vittima uccisa nel Blast resta nel mondo
	 * fino a `DestroyDefeatedUnits`, e `MakeSnapshot` conta occupante solo chi e' VIVO, quindi nel Move
	 * un'altra unita' puo' salire sulla sua cella. La prima stesura di questo commento diceva *«il corpus
	 * golden non ha ragione di cambiare»*: la ragione formulata cosi' non regge, ed e' `GoldenCorpusMatches`
	 * verde a dirlo — una misura, non una deduzione. Trovato in code review.
	 *
	 * `StableUnitId` e' il tie-break **gia' disponibile e gia' corretto** ([D-063]): lo assegna
	 * `ARTTurnManager::EnsureMatchRoster()` una volta per partita, con `MatchRosterLess` che e' gia' un ordine
	 * totale. Il nome dell'Actor chiude il caso in cui vale ancora `0`, ed e' **lo stesso ultimo confronto**
	 * che `MatchRosterLess` usa gia' — `GetName().Compare(...)`, case-sensitive: i due restano allineati.
	 *
	 * ⚠️ **`EnsureMatchRoster` non garantisce che `StableUnitId` sia sempre assegnato**, e una stesura
	 * precedente lo dava per fatto. Esce subito quando `bMatchRosterBuilt`, e il campo vale `0` di default:
	 * ogni unita' che compare **dopo** il congelamento porta `0` nella risoluzione. ∴ il terzo confronto non
	 * serve solo alle anteprime di pianificazione. Trovato in code review.
	 *
	 * ⛔ **E il nome NON e' riproducibile fra processi diversi, dove decide.** Un'unita' spawnata senza nome
	 * esplicito lo riceve da `MakeUniqueObjectName`, che appende un contatore per-classe vivo quanto il
	 * processo: due partite nello stesso Editor danno `..._0..3` e `..._4..7`, e l'ordine lessicale mette
	 * `_10` prima di `_2`. Non e' un difetto introdotto qui — `MatchRosterLess` ha la stessa proprieta' — ma
	 * va detto che la terza chiave rende l'ordine totale e ripetibile **dentro una esecuzione**, non oltre.
	 */
	static bool UnitOrderLess(const FRTUnitOrderKey& A, const FRTUnitOrderKey& B);

	/**
	 * La chiave d'ordine di un'unita' viva o morta. L'unico punto che legge i tre campi dall'Actor.
	 *
	 * ⚠️ Alloca una `FString` per chiamata (`GetName()` passa da `FName::ToString()`), ed e' per questo che
	 * `SortUnitsForResolution` la chiama O(N) volte e **non** dentro il comparatore.
	 */
	static FRTUnitOrderKey MakeUnitOrderKey(const ARTUnit& Unit);

	/**
	 * Ordina in place con `UnitOrderLess`: permutare l'ingresso non cambia la sequenza risolta, nemmeno
	 * quando due unita' condividono una cella.
	 *
	 * Chiamanti, per nome invece che per conteggio (`AGENTS.md` §14): `ARTGameMode::AssignUnitControlGroups`,
	 * `ARTTurnManager::ConcludeResolution`, `::ResolveEnvironment`, `::ResolvePrep`, `::CollectLivingUnits`,
	 * `::GatherBlastUnits`. Prima di #2922 ognuno aveva la propria copia del comparatore.
	 *
	 * ⚠️ **Precondizione: nessun `nullptr` nell'array** — la stessa dei `Sort` scritti a mano che questa
	 * funzione sostituisce (`TDereferenceWrapper` dereferenzia comunque). I chiamanti riempiono l'array da un
	 * `Cast<ARTUnit>`, quindi la condizione e' vera per costruzione. Un ripiego silenzioso sarebbe peggio del
	 * crash: una chiave di default ordinerebbe il `nullptr` **in mezzo** alle unita' vere, come se stesse
	 * sulla cella `(0,0,0)`.
	 *
	 * ⛔ **Ordina IN PLACE, e non si riscrive l'array del chiamante.** Una stesura precedente decorava,
	 * ordinava gli indici e faceva `Units = MoveTemp(Sorted)`: buttava via il buffer che `CollectLivingUnits`
	 * riusa con `Reset()`+`Reserve()` — e che `FRTScenarioSession` tiene per tutta la partita — riallocandolo
	 * a ogni turno. Trovato in code review.
	 */
	static void SortUnitsForResolution(TArray<ARTUnit*>& Units);
};
