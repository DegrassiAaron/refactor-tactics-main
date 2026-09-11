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
	 * 🔴 **`FString` confrontata CASE-SENSITIVE, e non un `FName`.** Una stesura intermedia di questa PR
	 * era passata a `FName` + `LexicalLess` per non allocare: e' stata **ritirata**, perche' toglieva uno
	 * spareggio che funzionava. `FName::Compare` e' case-INSENSITIVE, quindi due Actor chiamati `Unit_Alpha` e
	 * `UNIT_ALPHA` — rappresentabili in due sublevel diversi, perche' l'unicita' degli `UObject` vale dentro
	 * un solo Outer — pareggerebbero su tutte e tre le chiavi, e a decidere tornerebbe `GetAllActorsOfClass`.
	 * Cioe' `#990`, di nuovo.
	 *
	 * ⚠️ **E non e' teorico nella build che conta**: `WITH_CASE_PRESERVING_NAME` vale `WITH_EDITORONLY_DATA`
	 * (`NameTypes.h:33`), quindi negli Editor target — dove gira l'automation — il caso e' preservato e le due
	 * stringhe sono davvero diverse. Trovato in code review.
	 *
	 * ⛔ `operator<` no: `FString::UEOpLessThan` e' `Stricmp(...) < 0`, case-insensitive, quindi non e' un
	 * ordine totale sui byte. E' lo stesso difetto che `URTTurnLogLibrary::EntryLess` ha gia' pagato sulla
	 * v10 del TurnLog, con la ragione scritta li'.
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
	 * Ordine TOTALE fra due azioni pianificate. Vero se A risolve prima di B.
	 *
	 * Chiavi, in ordine: **macro-fase di Atlas** (`Prep -> Dash -> Blast -> Move -> Cleanup`) -> `Priority`
	 * intera crescente -> `ActionId` -> `SourceUnitId` -> `EventSequence`.
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
