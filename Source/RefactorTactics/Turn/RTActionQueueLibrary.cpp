#include "Turn/RTActionQueueLibrary.h"
#include "Ability/RTCatalogLibrary.h"
#include "Map/RTHexLibrary.h" // StableLess: la prima chiave dell'ordine delle unita' e' la cella
#include "Unit/RTUnit.h"      // ARTUnit: solo qui, perche' l'header resti senza un Actor dentro

bool URTActionQueueLibrary::InstanceLess(const FRTActionInstance& A, const FRTActionInstance& B)
{
	// 1) Macro-fase: e' l'ordine del turno, e viene prima di tutto il resto.
	const ERTMatchPhase PhaseA = URTCatalogLibrary::MapResolutionPhase(A.Def.ResolutionPhase);
	const ERTMatchPhase PhaseB = URTCatalogLibrary::MapResolutionPhase(B.Def.ResolutionPhase);
	if (PhaseA != PhaseB)
	{
		return static_cast<uint8>(PhaseA) < static_cast<uint8>(PhaseB); // enum confrontato per valore (no float)
	}

	// 2) Priorita' intera: valore minore risolve prima (regola del catalogo v0.1).
	if (A.Def.Priority != B.Def.Priority)
	{
		return A.Def.Priority < B.Def.Priority;
	}

	// 3..5) Tie-break assoluti: due azioni non restano mai indistinguibili, altrimenti a deciderle
	// resterebbe l'ordine di arrivo nel container — cioe' il caso.
	if (A.Def.ActionId != B.Def.ActionId)
	{
		return A.Def.ActionId.LexicalLess(B.Def.ActionId); // confronto lessicale: stabile fra esecuzioni
	}
	if (A.SourceUnitId != B.SourceUnitId)
	{
		return A.SourceUnitId < B.SourceUnitId;
	}
	if (A.EventSequence != B.EventSequence)
	{
		return A.EventSequence < B.EventSequence;
	}

	// 6..8) I campi che restavano fuori (#2970). Fino a qui l'ordine era PARZIALE: due istanze identiche su
	// tutte e cinque le chiavi sopra ma con bersaglio diverso pareggiavano nei due versi, e a deciderle
	// restava `TArray::Sort` — che inoltra ad `Algo::Sort`, introsort e NON stabile, cioe' l'ordine d'arrivo
	// nel container. E' la stessa classe di difetto che `URTTurnLogLibrary::EntryLess` ha gia' pagato due
	// volte sui campi serializzati, e che `TurnLog.CanonicalOrderCoversSerializedFields` esiste per fermare;
	// qui il gate gemello e' `Actions.CanonicalOrderCoversInstanceFields`, che enumera i campi per reflection.
	//
	// 🔑 **Sono spareggi TECNICI, e stanno in CODA a tutte le chiavi di gioco.** Non decidono chi meriti di
	// risolvere prima — quella sarebbe una regola di gameplay, e #2970 dichiara di non inventarne. Decidono
	// soltanto che l'esito non dipenda dall'ordine d'inserimento (`CLAUDE.md` §11), che e' la stessa
	// distinzione fatta da `UnitOrderLess` fra la cella e i due confronti che la seguono. A tenerle in coda
	// e' `Actions.KeyPrecedenceIsPinned`: senza quel test, spostare `TargetUnitId` davanti ad `ActionId`
	// lascerebbe l'intera suite verde e cambierebbe una regola di gioco.
	//
	// ⚠️ **`TargetUnitId` e `SourceUnitId` sono indici di SNAPSHOT, non identita' stabili** ([D-063]):
	// `RTActionQueue.h` lo dichiara — *«l'intero e' l'indice nello snapshot»* — e quella numerazione si
	// rifa' a ogni risoluzione. Regge come spareggio perche' lo snapshot e' congelato DENTRO la risoluzione
	// che lo usa, che e' tutto cio' che serve a non dipendere dall'ordine d'inserimento; ⛔ non regge come
	// identita' fra turni o fra processi, e non va usata per quello. `UnitOrderLess` puo' permettersi
	// `StableUnitId` perche' lavora su `ARTUnit`; qui un campo d'identita' stabile non esiste, e #2970
	// dichiara di non introdurlo.
	if (A.TargetUnitId != B.TargetUnitId)
	{
		return A.TargetUnitId < B.TargetUnitId;
	}
	if (A.TargetCell != B.TargetCell)
	{
		return URTHexLibrary::StableLess(A.TargetCell, B.TargetCell); // stessa primitiva di `UnitOrderLess`
	}
	// `bInterrupted` chiude: falso prima di vero.
	//
	// ⛔ **Non e' un ordine ESAUSTIVO su `FRTActionInstance`, e una stesura precedente scriveva qui che lo
	// fosse.** `Def` e' un campo come gli altri e ne porta molti: di quelli il confronto guarda
	// `ResolutionPhase`, `Priority` e `ActionId`, non il resto. Due istanze che differiscono solo per
	// `Def.Effects` (`URTReactionLibrary::BuildReactionEvents`) o `Def.RangeCells`
	// (`ARTTurnManager::CollectAttackIntents`) restano a pari merito, e
	// `Actions.CanonicalOrderCoversInstanceFields` lo ASSERISCE invece di lasciarlo a un commento.
	//
	// 🔑 **La premessa che rende il limite accettabile**: le istanze che entrano in uno stesso sort vengono
	// da UN solo produttore, e li' `EventSequence` e' distinto per costruzione. Si ricontrolla cosi':
	//
	//     git grep -n "SortActionInstances" -- Source/RefactorTactics ":(exclude)Source/RefactorTactics/Tests"
	//
	// ⚠️ **Quel comando risponde piu' righe che chiamate**: fra i match ci sono la dichiarazione, la
	// definizione e i commenti — questo incluso. Le chiamate sono DUE, e si chiamano
	// `ARTTurnManager::ResolvePrep` e `URTActionQueueLibrary::InstancesForPhase` (venti righe sotto); cercale
	// per nome, non contare i match. Una stesura precedente dichiarava «un solo chiamante» (#3004), cosi' chi
	// eseguiva il controllo prescritto trovava una smentita e nessuna spiegazione.
	//
	// La premessa regge lo stesso, ma per una ragione sola: **`InstancesForPhase` non ha chiamanti**. ⛔ Non
	// perche' non sia `UFUNCTION` — in questo header non lo e' nessuno, nemmeno `SortActionInstances`.
	//
	// ⛔ Il giorno in cui due produttori confluissero nello stesso array — la direzione di #1818 — la
	// premessa cade, e **aggiungere `Def` al confronto non e' la risposta**: appartiene al catalogo, e
	// confrontarla a fondo qui ne creerebbe una seconda verita'. Servirebbe un'identita' d'istanza unica per
	// turno, che oggi non esiste. Trovato in code review.
	return !A.bInterrupted && B.bInterrupted;
}

void URTActionQueueLibrary::SortActionInstances(TArray<FRTActionInstance>& Instances)
{
	Instances.Sort([](const FRTActionInstance& A, const FRTActionInstance& B) { return InstanceLess(A, B); });
}

TArray<FRTActionInstance> URTActionQueueLibrary::InstancesForPhase(const TArray<FRTActionInstance>& Instances,
	ERTMatchPhase Phase)
{
	TArray<FRTActionInstance> Out;
	for (const FRTActionInstance& Instance : Instances)
	{
		if (URTCatalogLibrary::MapResolutionPhase(Instance.Def.ResolutionPhase) == Phase)
		{
			Out.Add(Instance);
		}
	}
	SortActionInstances(Out);
	return Out;
}

// --- Ordine delle UNITA' (#2922) ------------------------------------------------------------------------

bool URTActionQueueLibrary::UnitOrderLess(const FRTUnitOrderKey& A, const FRTUnitOrderKey& B)
{
	// 1) La cella: e' la chiave che c'era prima di questa funzione, e resta la prima. Dove le celle
	// differiscono l'ordine e' identico a quello di ieri, e non si sposta niente.
	if (!(A.Cell == B.Cell))
	{
		return URTHexLibrary::StableLess(A.Cell, B.Cell);
	}

	// 2) L'identita' di partita ([D-063]). Due unita' sulla stessa cella esistono — `FRTHexSnapshot::Overlaps`
	// le registra — e senza questa riga a ordinarle resterebbe `GetAllActorsOfClass`.
	//
	// 🔴 **A difenderla e' `Actions.UnitOrderStableIdBeatsActorName`**, e quel test esiste perche' quelli
	// che c'erano prima non ci riuscivano: in ogni loro fixture l'ordine dei nomi CONCORDAVA con quello degli id,
	// quindi togliendo questa riga restavano tutti verdi — il nome decideva allo stesso modo. Una chiave
	// difesa solo da casi in cui la chiave successiva direbbe lo stesso non e' difesa. Trovato in code review.
	if (A.StableUnitId != B.StableUnitId)
	{
		return A.StableUnitId < B.StableUnitId;
	}

	// 3) Il nome dell'Actor, per il solo caso in cui l'identita' non sia ancora stata assegnata (vale `0` per
	// entrambe): la pianificazione prima del primo lock-in, e le unita' entrate dopo il congelamento del
	// roster, che `EnsureMatchRoster` non rinumera.
	//
	// 🔴 `Compare(..., CaseSensitive)` e NON `operator<`, e non un `FName`: il perche' di entrambi sta su
	// `FRTUnitOrderKey::ActorName`. In breve: `operator<` e `FName::Compare` sono case-INSENSITIVE, quindi
	// nessuno dei due e' un ordine totale, e il pareggio tornerebbe a `GetAllActorsOfClass`.
	return A.ActorName.Compare(B.ActorName, ESearchCase::CaseSensitive) < 0;
}

FRTUnitOrderKey URTActionQueueLibrary::MakeUnitOrderKey(const ARTUnit& Unit)
{
	return FRTUnitOrderKey(Unit.Cell, Unit.StableUnitId, Unit.GetName());
}

void URTActionQueueLibrary::SortUnitsForResolution(TArray<ARTUnit*>& Units)
{
	const int32 Num = Units.Num();
	if (Num < 2)
	{
		return; // niente da ordinare, e nessuna chiave da costruire
	}

	// 🔑 **La chiave si costruisce UNA volta per unita'**, e viaggia ATTACCATA al suo puntatore.
	//
	// ⚠️ **Il compromesso vero non e' O(N) contro O(N log N): e' UNA REGOLA contro ZERO ALLOCAZIONI**, e le
	// stesure precedenti lo raccontavano male. Un comparatore pigro — cella, poi id, e il nome solo se
	// entrambi pareggiano — non allocherebbe **mai**, perche' quel ramo non si raggiunge quasi mai. Ma per
	// essere pigro dovrebbe confrontare cella e id **da se'**, cioe' riscrivere le prime due chiavi fuori da
	// `UnitOrderLess`: due copie della stessa regola, che e' il difetto che #2922 esiste per chiudere.
	// ∴ si decora, e si paga una `FString` per unita'.
	//
	// ⛔ E si decora in COPPIE, non ordinando un array di indici: la stesura intermedia riscriveva
	// `Units[i] = Originale[Order[i]]`, una permutazione a mano che nessun test copre e che il refuso naturale
	// — `Units[Order[i]] = Originale[i]` — avrebbe invertito in silenzio. Tenere chiave e puntatore insieme
	// toglie quella classe di difetto invece di difendersene. Trovato in code review.
	TArray<TPair<FRTUnitOrderKey, ARTUnit*>> Decorate;
	Decorate.Reserve(Num);
	for (ARTUnit* Unit : Units)
	{
		Decorate.Emplace(MakeUnitOrderKey(*Unit), Unit); // precondizione: nessun `nullptr` nell'array
	}

	// `Sort` e non `StableSort`: `UnitOrderLess` e' totale **sotto la premessa dell'Outer unico** scritta su
	// `FRTUnitOrderKey::ActorName`. Se quella premessa cadesse, nemmeno `StableSort` salverebbe l'ordine — a
	// deciderlo resterebbe comunque l'ingresso, cioe' `GetAllActorsOfClass`. La difesa e' la premessa, non la
	// scelta del sort.
	Decorate.Sort([](const TPair<FRTUnitOrderKey, ARTUnit*>& A, const TPair<FRTUnitOrderKey, ARTUnit*>& B)
	{
		return UnitOrderLess(A.Key, B.Key);
	});

	// Si riscrive DENTRO `Units`: il buffer del chiamante resta il suo. `CollectLivingUnits` lo riusa con
	// `Reset()`+`Reserve()`, e `FRTScenarioSession` lo tiene per tutta la partita.
	for (int32 i = 0; i < Num; ++i)
	{
		Units[i] = Decorate[i].Value;
	}
}
