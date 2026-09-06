#include "RTAnimBrowserModel.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Unit/RTAnimCatalogLibrary.h"

FString FRTAnimBrowserModel::PackFromAssetPath(const FString& AssetPath)
{
	// `/Game/FabAsset/Paragon/ParagonGadget/Characters/Heroes/Gadget/Animations/Idle.Idle` -> `Gadget`.
	//
	// ⚠️ Si legge il segmento `Paragon<Pack>` e **non** la cartella `Heroes/<X>`: sono quasi sempre
	// uguali, ma il pack e' il nome che il path porta due volte, e prenderne uno solo rende il parser
	// muto se l'altro cambia. Se il segmento non c'e', la risposta e' vuota — non un pack inventato.
	const TCHAR* Marker = TEXT("/Paragon");
	int32 Start = INDEX_NONE;
	if (!AssetPath.FindLastChar(TEXT('/'), Start))
	{
		return FString();
	}

	int32 MarkerPos = INDEX_NONE;
	if (!AssetPath.Contains(Marker, ESearchCase::CaseSensitive))
	{
		return FString();
	}

	// L'ultima occorrenza di `/Paragon` che sia seguita da altro: `/Paragon/ParagonGadget/` ne ha due, e
	// quella buona e' la seconda.
	int32 Cursor = 0;
	while (true)
	{
		const int32 Found = AssetPath.Find(Marker, ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor);
		if (Found == INDEX_NONE)
		{
			break;
		}
		MarkerPos = Found;
		Cursor = Found + 1;
	}
	if (MarkerPos == INDEX_NONE)
	{
		return FString();
	}

	const int32 NameStart = MarkerPos + FCString::Strlen(Marker);
	int32 NameEnd = AssetPath.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, NameStart);
	if (NameEnd == INDEX_NONE)
	{
		NameEnd = AssetPath.Len();
	}
	return AssetPath.Mid(NameStart, NameEnd - NameStart);
}

bool FRTAnimBrowserModel::LoadFrom(const FString& CatalogPath, FString& OutError)
{
	// ⛔ Un catalogo illeggibile NON diventa un catalogo vuoto: un browser vuoto si legge come «non ci
	// sono clip», che e' un'affermazione, mentre qui il fatto e' «non ho potuto leggerle».
	Catalog = FRTAnimCatalog();
	return URTAnimCatalogLibrary::LoadFromFile(CatalogPath, Catalog, OutError);
}

bool FRTAnimBrowserModel::SaveTo(const FString& CatalogPath, FString& OutError) const
{
	FString Json;
	if (!URTAnimCatalogLibrary::SaveToString(Catalog, Json))
	{
		OutError = TEXT("serializzazione del catalogo fallita");
		return false;
	}
	if (!FFileHelper::SaveStringToFile(Json, *CatalogPath))
	{
		OutError = FString::Printf(TEXT("scrittura di '%s' fallita"), *CatalogPath);
		return false;
	}
	return true;
}

FRTAnimCatalogEntry* FRTAnimBrowserModel::FindEntry(const FName& Id)
{
	return Catalog.Entries.FindByPredicate(
		[&Id](const FRTAnimCatalogEntry& E) { return E.Id == Id; });
}

const FRTAnimCatalogEntry* FRTAnimBrowserModel::FindEntry(const FName& Id) const
{
	return Catalog.Entries.FindByPredicate(
		[&Id](const FRTAnimCatalogEntry& E) { return E.Id == Id; });
}

TArray<FRTAnimBrowserRow> FRTAnimBrowserModel::VisibleRows() const
{
	TArray<FRTAnimBrowserRow> Rows;
	Rows.Reserve(Catalog.Entries.Num());

	for (const FRTAnimCatalogEntry& Entry : Catalog.Entries)
	{
		const FString Pack = PackFromAssetPath(Entry.Derived.AssetPath);

		// I tre filtri sono in AND. Ognuno vuoto/non impostato non filtra: e' la differenza fra «nessun
		// filtro» e «filtro che non trova niente», e il pannello deve poterle distinguere.
		if (!PackFilter.IsEmpty() && !Pack.Equals(PackFilter, ESearchCase::IgnoreCase))
		{
			continue;
		}
		if (StatusFilter.IsSet() && Entry.Authored.Status != StatusFilter.GetValue())
		{
			continue;
		}
		if (!SearchText.IsEmpty())
		{
			// La ricerca guarda il nome dell'asset **e** l'`AV_ID`: si cerca tanto «Run» quanto «AV_0042».
			const bool bMatch =
				Entry.Derived.AssetName.Contains(SearchText, ESearchCase::IgnoreCase) ||
				Entry.Id.ToString().Contains(SearchText, ESearchCase::IgnoreCase);
			if (!bMatch)
			{
				continue;
			}
		}

		FRTAnimBrowserRow Row;
		Row.Id = Entry.Id;
		Row.AssetName = Entry.Derived.AssetName;
		Row.AssetPath = Entry.Derived.AssetPath;
		Row.Pack = Pack;
		Row.Status = Entry.Authored.Status;
		Row.Label = Entry.Authored.Label;
		Row.DurationSeconds = Entry.Derived.DurationSeconds;
		Row.bHasRootMotion = Entry.Derived.bHasRootMotion;
		Row.bIsAdditive = Entry.Derived.bIsAdditive;
		Row.NumBindings = Entry.Authored.Bindings.Num();
		Rows.Add(MoveTemp(Row));
	}

	// Ordine per `AV_ID`, stabile e indipendente dall'ordine del file: due caricamenti dello stesso
	// catalogo devono dare la stessa lista, altrimenti la selezione dell'autore salta sotto le dita.
	Rows.Sort([](const FRTAnimBrowserRow& A, const FRTAnimBrowserRow& B)
	{
		return A.Id.LexicalLess(B.Id);
	});
	return Rows;
}

bool FRTAnimBrowserModel::ApplyUserStatus(const FName& Id, ERTAnimClipStatus NewStatus)
{
	FRTAnimCatalogEntry* Entry = FindEntry(Id);
	if (Entry == nullptr)
	{
		return false;
	}

	// ⛔ Si tocca SOLO `Status`. `Derived` resta intatto — il rescan riscrive quello, e mescolare le due
	// meta' qui vanificherebbe la separazione che il formato porta nella sua struttura.
	Entry->Authored.Status = NewStatus;
	return true;
}

bool FRTAnimBrowserModel::BindToRole(const FName& Id, const FName& HeroId, ERTPresentationRole Role)
{
	FRTAnimCatalogEntry* Entry = FindEntry(Id);
	if (Entry == nullptr || HeroId.IsNone())
	{
		return false;
	}

	// ⛔ **Solo da `Promoted`.** Legare una clip che nessuno ha guardato e' il salto che questo strumento
	// esiste per impedire: il legame significherebbe «questa suonera' in partita» senza che nessuno abbia
	// detto che si guarda bene.
	if (Entry->Authored.Status != ERTAnimClipStatus::Promoted)
	{
		return false;
	}

	const bool bGia = Entry->Authored.Bindings.ContainsByPredicate(
		[&HeroId, Role](const FRTAnimBinding& B) { return B.HeroId == HeroId && B.Role == Role; });
	if (bGia)
	{
		return false;   // idempotente e non duplicante
	}

	FRTAnimBinding Binding;
	Binding.HeroId = HeroId;
	Binding.Role = Role;
	Binding.bActive = false;   // ⛔ SEMPRE inattiva, anche se e' la prima del ruolo
	Entry->Authored.Bindings.Add(MoveTemp(Binding));
	return true;
}

bool FRTAnimBrowserModel::MakeActive(const FName& Id, const FName& HeroId, ERTPresentationRole Role)
{
	// Prima si verifica che il bersaglio esista: un `Make Active` fallito non deve poter disattivare
	// quella che c'era, che sarebbe una disattivazione travestita da errore.
	const FRTAnimCatalogEntry* Target = FindEntry(Id);
	if (Target == nullptr)
	{
		return false;
	}
	const bool bLegata = Target->Authored.Bindings.ContainsByPredicate(
		[&HeroId, Role](const FRTAnimBinding& B) { return B.HeroId == HeroId && B.Role == Role; });
	if (!bLegata)
	{
		return false;
	}

	// 🔑 L'atomicita': in un solo passaggio si spegne ogni altra attiva di QUESTO ruolo e si accende la
	// scelta. Non esiste un istante intermedio con due attive, che e' cio' che `ValidateCatalog` rifiuta.
	for (FRTAnimCatalogEntry& Entry : Catalog.Entries)
	{
		for (FRTAnimBinding& Binding : Entry.Authored.Bindings)
		{
			if (Binding.HeroId == HeroId && Binding.Role == Role)
			{
				Binding.bActive = (Entry.Id == Id);
			}
		}
	}
	return true;
}

bool FRTAnimBrowserModel::Unbind(const FName& Id, const FName& HeroId, ERTPresentationRole Role)
{
	FRTAnimCatalogEntry* Entry = FindEntry(Id);
	if (Entry == nullptr)
	{
		return false;
	}
	const int32 Rimossi = Entry->Authored.Bindings.RemoveAll(
		[&HeroId, Role](const FRTAnimBinding& B) { return B.HeroId == HeroId && B.Role == Role; });

	// ⚠️ Nessuna elezione di una sostituta. Se quella rimossa era l'attiva, il ruolo resta senza attiva e
	// l'unita' torna in posa di riferimento: si vede, ed e' la scelta dell'autore da rifare.
	return Rimossi > 0;
}
