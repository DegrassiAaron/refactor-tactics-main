#include "Map/RTHexMapSummary.h"

#include "Map/RTHexMapAsset.h"

FRTHexMapSummary URTHexMapSummaryLibrary::Summarise(const URTHexMapAsset* Map, int32 ActiveLayer)
{
	FRTHexMapSummary S;
	S.ActiveLayer = ActiveLayer;

	if (Map == nullptr)
	{
		// ⛔ Si esce PRIMA di toccare qualunque conteggio: `bHasAsset = false` con `Cells = 0` e
		// `Layers` vuoto e' cio' che il chiamante deve poter distinguere da una mappa vuota.
		return S;
	}

	S.bHasAsset = true;
	S.AssetName = Map->GetName();
	S.Cells = Map->NumCells();

	// ⛔ `GetLayers()`, mai un ciclo su `Cells` che li raccolga di nuovo: e' il divieto esplicito della
	// issue, e il test `PanelAndLibraryCannotDiverge` lo presidia.
	S.Layers = Map->GetLayers();
	S.bActiveLayerExists = S.Layers.Contains(ActiveLayer);
	return S;
}

FString URTHexMapSummaryLibrary::DescriviAsset(const FRTHexMapSummary& S)
{
	// 🔴 Non una stringa vuota e non un trattino: la frase dice che cosa manca. La sandbox e' rimasta
	// senza `MapAsset` per un commit intero senza che nessuno se ne accorgesse (`7db11313`, #937).
	return S.bHasAsset ? S.AssetName : TEXT("(nessun asset collegato)");
}

FString URTHexMapSummaryLibrary::DescriviCelle(const FRTHexMapSummary& S)
{
	// ⚠️ **Senza asset NON si scrive `0`.** Uno zero e' una misura, e qui non c'e' niente da misurare:
	// mostrarlo farebbe leggere «mappa vuota» dove la verita' e' «nessuna mappa». E' il criterio 2 del DoD.
	if (!S.bHasAsset)
	{
		return TEXT("—");
	}
	return FString::FromInt(S.Cells);
}

FString URTHexMapSummaryLibrary::DescriviLayer(const FRTHexMapSummary& S)
{
	if (!S.bHasAsset)
	{
		return TEXT("—");
	}
	if (S.Layers.Num() == 0)
	{
		// Un asset senza celle: c'e' la mappa, non ci sono piani. Distinto dal caso sopra.
		return TEXT("nessuno (mappa senza celle)");
	}

	// 🔑 Il conteggio **e** l'elenco. `U21` aveva il conteggio delle celle e non sapeva i piani: un numero
	// solo e' precisamente cio' che non era bastato.
	TArray<FString> Nomi;
	Nomi.Reserve(S.Layers.Num());
	for (const int32 L : S.Layers)
	{
		Nomi.Add(FString::FromInt(L));
	}
	return FString::Printf(TEXT("%d — %s"), S.Layers.Num(), *FString::Join(Nomi, TEXT(", ")));
}

FString URTHexMapSummaryLibrary::DescriviLayerAttivo(const FRTHexMapSummary& S)
{
	if (!S.bHasAsset)
	{
		return TEXT("—");
	}

	// ⚠️ Un layer attivo che non esiste non e' un errore — si comincia un piano nuovo esattamente cosi' —
	// ma chi guarda deve distinguerlo da un piano gia' popolato, o crede di lavorare dove non c'e' niente.
	return S.bActiveLayerExists
		? FString::FromInt(S.ActiveLayer)
		: FString::Printf(TEXT("%d (nessuna cella su questo piano)"), S.ActiveLayer);
}

FString URTHexMapSummaryLibrary::DescriviValidazione(const URTHexMapAsset* Map)
{
	if (Map == nullptr)
	{
		// ⛔ Non «0 segnalazioni»: senza mappa la domanda non si pone, e uno zero si leggerebbe come
		// «va tutto bene» — la bugia che `#1186` ha tolto agli altri readout di questo pannello.
		return TEXT("nessuna mappa collegata: non c'e' niente da validare");
	}

	// ⚠️ `ValidateMap()` e non `ValidateMapDetailed()`: la prima porta le proprie regole **e** in coda
	// quelle della seconda, quindi e' il superset. Vedi l'header.
	const TArray<FString> Righe = Map->ValidateMap();

	int32 Errori = 0;
	int32 Avvisi = 0;
	for (const FString& Riga : Righe)
	{
		// Il prefisso e' la convenzione del validatore, non una lettura di questo file: ogni riga che
		// `ValidateMap` produce comincia per l'uno o per l'altro, comprese quelle che arrivano da
		// `ValidateMapDetailed` e che vengono formattate proprio cosi'.
		if (Riga.StartsWith(TEXT("Error:")))
		{
			++Errori;
		}
		else if (Riga.StartsWith(TEXT("Warning:")))
		{
			++Avvisi;
		}
	}

	// 🔑 Una riga senza prefisso non si perde in silenzio: sarebbe una regola nuova scritta fuori dalla
	// convenzione, e chi legge il pannello vedrebbe meno di cio' che il validatore dice.
	const int32 SenzaPrefisso = Righe.Num() - Errori - Avvisi;

	if (Righe.Num() == 0)
	{
		return TEXT("nessuna segnalazione");
	}

	FString Out;
	if (Errori > 0)
	{
		Out = FString::Printf(TEXT("%d error%s"), Errori, Errori == 1 ? TEXT("e") : TEXT("i"));
	}
	if (Avvisi > 0)
	{
		if (!Out.IsEmpty()) { Out += TEXT(", "); }
		Out += FString::Printf(TEXT("%d avvis%s"), Avvisi, Avvisi == 1 ? TEXT("o") : TEXT("i"));
	}
	if (SenzaPrefisso > 0)
	{
		if (!Out.IsEmpty()) { Out += TEXT(", "); }
		Out += FString::Printf(TEXT("%d senza prefisso"), SenzaPrefisso);
	}

	// La PRIMA riga per esteso: un conteggio dice quanto, non che cosa, e chi disegna deve sapere da dove
	// cominciare senza aprire un altro pannello.
	return FString::Printf(TEXT("%s — %s"), *Out, *Righe[0]);
}
