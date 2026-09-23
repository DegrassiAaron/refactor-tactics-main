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
