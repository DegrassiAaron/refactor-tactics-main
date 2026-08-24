#include "Debug/RTDebugReportLibrary.h"

#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTIntentPrivacyLibrary.h"
#include "Turn/RTTurnLog.h"

namespace
{
	/** Il nome di un valore di enum riflesso, senza il prefisso del tipo. Vuoto se l'enum non e' UENUM. */
	template <typename TEnum>
	FString EnumName(TEnum Value)
	{
		if (const UEnum* Meta = StaticEnum<TEnum>())
		{
			return Meta->GetNameStringByValue(static_cast<int64>(Value));
		}
		return FString::FromInt(static_cast<int64>(Value));
	}

	/** Il livello di certezza in una parola, per la riga di `DrawIntent`. */
	const TCHAR* CertaintyLabel(ERTIntentCertainty Certainty)
	{
		switch (Certainty)
		{
		case ERTIntentCertainty::Confirmed: return TEXT("confermato");
		case ERTIntentCertainty::Predicted: return TEXT("previsto");
		case ERTIntentCertainty::Uncertain: return TEXT("incerto");
		default:                            return TEXT("non calcolato");
		}
	}
}

TArray<FString> URTDebugReportLibrary::DescribeIntents(int32 ObserverTeamId, const TArray<FRTPlannedIntent>& Intents)
{
	// 🔴 La riga sotto e' il checkpoint intero: si compone dalla VISTA FILTRATA, mai da `Intents`. Chi
	// leggesse `Intents` direttamente avrebbe un comando che stampa i piani avversari — e nessun test sul
	// DTO se ne accorgerebbe, perche' il DTO resterebbe corretto.
	const TArray<FRTIntentView> Views = URTIntentPrivacyLibrary::FilterForTeam(ObserverTeamId, Intents);

	TArray<FString> Lines;
	Lines.Reserve(Views.Num());
	for (const FRTIntentView& V : Views)
	{
		FString Line = FString::Printf(TEXT("[RT] %s %s"),
			V.bIsAlly ? TEXT("alleata") : TEXT("avversaria"), *V.OwnerCell.ToString());

		if (V.bMoving)   { Line += FString::Printf(TEXT(" -> %s"), *V.PlannedCell.ToString()); }
		if (V.bDashing)  { Line += FString::Printf(TEXT(" scatto %s"), *V.DashCell.ToString()); }
		if (!V.ActionName.IsEmpty())
		{
			Line += FString::Printf(TEXT(" | %s"), *V.ActionName.ToString());
			if (V.bHasTarget) { Line += FString::Printf(TEXT(" su %s"), *V.TargetCell.ToString()); }
		}
		// `ReactionName` e' vuota per costruzione quando l'osservatore e' un avversario: qui non serve
		// nessuna guardia, ed e' il punto — la regola sta in `FilterForTeam`, non ricopiata in un `if`.
		if (!V.ReactionName.IsEmpty())
		{
			Line += FString::Printf(TEXT(" | reazione: %s"), *V.ReactionName.ToString());
		}
		Line += FString::Printf(TEXT(" | %s"), CertaintyLabel(V.Certainty));

		Lines.Add(MoveTemp(Line));
	}
	return Lines;
}

FRTDebugReplayVerdict URTDebugReportLibrary::VerifyReplay(const TArray<uint8>& GoldenBytes,
	const TArray<FRTTurnLogEntry>& GoldenEntries, const TArray<FRTTurnLogEntry>& Actual,
	ERTLogTopology Topology, FName FormatId)
{
	FRTDebugReplayVerdict Verdict;

	const TArray<uint8> ActualBytes = URTTurnLogLibrary::SerializeTurnLog(Actual, Topology, FormatId);
	Verdict.Comparison = URTTurnLogLibrary::CompareSerializedTraces(GoldenBytes, ActualBytes);

	switch (Verdict.Comparison)
	{
	case ERTTraceComparison::Identical:
		Verdict.Lines.Add(FString::Printf(TEXT("[RT] Replay: nessuna divergenza su %d voci."), Actual.Num()));
		break;

	case ERTTraceComparison::Divergence:
		Verdict.Lines.Add(FString::Printf(TEXT("[RT] Replay: DIVERGENZA su %d voci contro %d di riferimento."),
			Actual.Num(), GoldenEntries.Num()));
		// `DescribeFirstDivergence` confronta VOCI, non byte: senza il riferimento in forma di voci la
		// divergenza resta rilevata ma non localizzata, e il verdetto lo dice invece di tacerlo.
		if (GoldenEntries.Num() > 0)
		{
			// ⚠️ Il turno da riportare e' quello della **voce che diverge**, non `GoldenEntries[0]`: una
			// traccia copre piu' round, e un'etichetta presa dalla prima voce manderebbe a guardare il
			// round sbagliato — proprio dalla funzione il cui compito e' dire DOVE.
			int32 TurnNumber = GoldenEntries[0].TurnNumber;
			for (int32 i = 0; i < GoldenEntries.Num(); ++i)
			{
				if (!Actual.IsValidIndex(i) ||
					!URTTurnLogLibrary::GoldenEntriesMatch(GoldenEntries[i], Actual[i]))
				{
					TurnNumber = GoldenEntries[i].TurnNumber;
					break;
				}
			}
			Verdict.FirstDivergence = URTTurnLogLibrary::DescribeFirstDivergence(TurnNumber, GoldenEntries, Actual);
			Verdict.Lines.Add(FString::Printf(TEXT("[RT]   prima divergenza: %s"), *Verdict.FirstDivergence));
		}
		else
		{
			Verdict.Lines.Add(TEXT("[RT]   non localizzabile: il riferimento e' stato passato solo in byte."));
		}
		break;

	case ERTTraceComparison::FormatMismatch:
		// ⚠️ Non e' una divergenza, ed e' la distinzione che rende utile lo strumento: due tracce di
		// formati diversi non sono in disaccordo, sono incommensurabili. Dichiararle «divergenti»
		// manderebbe a cercare un difetto di simulazione che non c'e'.
		Verdict.Lines.Add(TEXT("[RT] Replay: formati diversi — le due tracce NON sono confrontabili."));
		break;

	case ERTTraceComparison::TopologyMismatch:
		Verdict.Lines.Add(TEXT("[RT] Replay: topologie diverse — le celle non significano la stessa cosa."));
		break;

	case ERTTraceComparison::Unreadable:
	default:
		Verdict.Lines.Add(TEXT("[RT] Replay: almeno una delle due tracce non e' leggibile "
			"(magic, versione, troncamento o checksum)."));
		break;
	}
	return Verdict;
}

FString URTDebugReportLibrary::DescribeCell(const FRTHexCellData& Cell, int32 OccupantUnitId, int32 Revision)
{
	FString Line = FString::Printf(TEXT("%s %s cost=%d"),
		*Cell.Id.ToString(), *EnumName(Cell.Surface), Cell.TotalMoveCost());

	// `INDEX_NONE` = libera. Si OMETTE il campo invece di stamparne uno vuoto: una riga che dice
	// `occupante=` invita a chiedersi di chi sia quella stringa vuota.
	if (OccupantUnitId != INDEX_NONE)
	{
		Line += FString::Printf(TEXT(" occupante=%d"), OccupantUnitId);
	}
	if (Cell.bBlocksMovement)     { Line += TEXT(" blocca-passo"); }
	if (Cell.bBlocksLineOfSight)  { Line += TEXT(" blocca-vista"); }

	for (const FRTHexCover& Cover : Cell.Covers)
	{
		Line += FString::Printf(TEXT(" cover[%s:%s/%d]"),
			*EnumName(Cover.Edge), *EnumName(Cover.Type), Cover.Integrity);
	}

	// Gli stati che la superficie impone a chi ci sta sopra. Derivati, non memorizzati — ed e' la
	// ragione per cui questa riga risponde a «perche' quell'unita' e' bagnata?», che il solo nome
	// della superficie non spiega a chi non conosce il catalogo terreni a memoria.
	//
	// 🔴 **`CellBoundStatusesFor` da sola non basta, e l'header di `RTTerrainLibrary` lo dice per esteso**:
	// restituisce i soli stati SOSTENUTI (durata 0), mentre `Burning` ha durata 2 e non compare — *«una
	// cella in fiamme risulterebbe innocua»*. Una stesura precedente di questa funzione faceva esattamente
	// l'errore che quel commento avverte di non fare, sulla superficie piu' pericolosa del catalogo.
	// `IsHazardousSurface` esiste per questa domanda e chiede al catalogo, non a un elenco scritto qui.
	TArray<FString> Tags;
	for (const FGameplayTag& Tag : URTTerrainLibrary::CellBoundStatusesFor(Cell.Surface))
	{
		Tags.Add(Tag.ToString());
	}
	if (URTTerrainLibrary::IsHazardousSurface(Cell.Surface))
	{
		Line += TEXT(" PERICOLOSA");
	}
	if (Tags.Num() > 0)
	{
		// Ordinati: l'iterazione di un `TSet` non ha ordine garantito, e un dump che cambia riga fra due
		// esecuzioni identiche non e' confrontabile — che e' l'unica cosa per cui serve un dump.
		Tags.Sort();
		Line += FString::Printf(TEXT(" stati=[%s]"), *FString::Join(Tags, TEXT(",")));
	}

	Line += FString::Printf(TEXT(" rev=%d"), Revision);
	return Line;
}

FString URTDebugReportLibrary::DescribeLogEntry(const FRTTurnLogEntry& Entry, int32 SequenceIndex)
{
	// I campi STRUTTURALI in forma chiave=valore, poi la frase tradotta. L'ordine non e' estetico: chi
	// legge un dump cerca prima l'identita' della voce, e la prosa serve a capire cosa e' successo.
	FString Line = FString::Printf(TEXT("#%d T%d %s %s %s unita=%d p%d %s -> %s"),
		SequenceIndex,
		Entry.TurnNumber,
		*EnumName(Entry.Phase),
		*EnumName(Entry.Category),
		Entry.ActionId.IsNone() ? TEXT("(senza azione)") : *Entry.ActionId.ToString(),
		Entry.UnitId,
		Entry.Priority,
		*Entry.SrcCell.ToString(),
		*Entry.TgtCell.ToString());

	if (Entry.SelectedTargetUnitId != INDEX_NONE)
	{
		Line += FString::Printf(TEXT(" bersaglio=%d"), Entry.SelectedTargetUnitId);
	}

	// `ValidationResult` del DoD: l'esito tradotto. `DescribeEntry` sa quale enum leggere per ciascuna
	// categoria — qui non si duplica quella tabella, che e' precisamente il punto in cui due verita'
	// diverse si formerebbero senza che nessun test le confronti.
	Line += FString::Printf(TEXT(" | %s"), *URTTurnLogLibrary::DescribeEntry(Entry));
	return Line;
}

TArray<FString> URTDebugReportLibrary::DescribeSnapshot(const FRTHexSnapshot& Snapshot)
{
	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("[RT] Snapshot: %d unita', %d celle occupate, rev=%d, MapHash=0x%08x"),
		Snapshot.Units.Num(), Snapshot.Occupancy.Num(), Snapshot.Revision, Snapshot.MapHash));

	if (URTHexSimLibrary::IsSnapshotStale(Snapshot))
	{
		// Uno snapshot stantio e' la causa piu' comune di «il gioco ha deciso una cosa che non vedo»:
		// dirlo qui evita di cercare il difetto nel resolver.
		Lines.Add(TEXT("[RT]   ⚠ STANTIO: la mappa e' cambiata dopo la costruzione di questo snapshot."));
	}
	for (const FString& Problem : URTHexSimLibrary::ValidateSnapshot(Snapshot))
	{
		Lines.Add(FString::Printf(TEXT("[RT]   ⚠ %s"), *Problem));
	}

	// Le unita' in ordine di `UnitId`: `Units` e' un array e l'ordine e' gia' deterministico, ma
	// dichiararlo evita che un domani un `TMap` ci finisca dentro senza che nessuno se ne accorga.
	TArray<FRTHexSimUnit> Ordered = Snapshot.Units;
	Ordered.Sort([](const FRTHexSimUnit& A, const FRTHexSimUnit& B) { return A.UnitId < B.UnitId; });
	for (const FRTHexSimUnit& U : Ordered)
	{
		Lines.Add(FString::Printf(TEXT("[RT]   unita %d %s %s mp=%d facing=%s"),
			U.UnitId, *U.Cell.ToString(), U.bAlive ? TEXT("viva") : TEXT("ELIMINATA"),
			U.MoveBudget, *EnumName(U.Facing)));
	}

	if (const URTHexMapAsset* Map = Snapshot.Map)
	{
		Lines.Add(FString::Printf(TEXT("[RT]   mappa: %d celle"), Map->NumCells()));
	}
	else
	{
		Lines.Add(TEXT("[RT]   mappa: assente (snapshot senza asset)"));
	}
	return Lines;
}

TArray<FString> URTDebugReportLibrary::DescribeTurnLogEntries(const TArray<FRTTurnLogEntry>& Entries)
{
	TArray<FString> Lines;
	// L'hash in testa: e' cio' che si confronta fra due esecuzioni, e averlo accanto alle voci evita di
	// doverlo ricavare altrove quando si sta gia' guardando il log.
	Lines.Add(FString::Printf(TEXT("[RT] TurnLog: %d voci, hash=0x%08x (ordinato 0x%08x)"),
		Entries.Num(),
		URTTurnLogLibrary::HashTurnLog(Entries),
		URTTurnLogLibrary::HashTurnLogOrdered(Entries)));

	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		Lines.Add(FString::Printf(TEXT("[RT]   %s"), *DescribeLogEntry(Entries[i], i)));
	}
	return Lines;
}
