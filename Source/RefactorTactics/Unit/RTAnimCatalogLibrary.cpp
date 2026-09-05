#include "Unit/RTAnimCatalogLibrary.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

const TCHAR* URTAnimCatalogLibrary::IdPrefix = TEXT("AV_");
const TCHAR* URTAnimCatalogLibrary::CatalogRelativePath = TEXT("Data/Anim/AnimCatalog.json");

namespace
{
	// Nomi delle chiavi JSON in un posto solo: chi legge e chi scrive non possono divergere su una stringa
	// scritta due volte.
	const TCHAR* KeyFormatVersion = TEXT("formatVersion");
	const TCHAR* KeyNextId = TEXT("nextId");
	const TCHAR* KeyEntries = TEXT("entries");
	const TCHAR* KeyId = TEXT("id");
	const TCHAR* KeyDerived = TEXT("derived");
	const TCHAR* KeyAuthored = TEXT("authored");

	const TCHAR* KeyAssetPath = TEXT("assetPath");
	const TCHAR* KeyAssetName = TEXT("assetName");
	const TCHAR* KeySkeleton = TEXT("skeleton");
	const TCHAR* KeyDuration = TEXT("durationSeconds");
	const TCHAR* KeyFrameCount = TEXT("frameCount");
	const TCHAR* KeyRootMotion = TEXT("bHasRootMotion");
	const TCHAR* KeyAdditive = TEXT("bIsAdditive");
	const TCHAR* KeyFingerprint = TEXT("fingerprint");

	const TCHAR* KeyStatus = TEXT("status");
	const TCHAR* KeyLabel = TEXT("label");
	const TCHAR* KeyNotes = TEXT("notes");

	/**
	 * Lo `Status` si serializza per NOME, non per numero.
	 *
	 * ⚠️ Un authoring source e' un file che una persona legge in un diff: `"status": 2` costringerebbe a
	 * ricordare a memoria quale valore e' `Promoted`, ed e' esattamente il punto in cui un riordino dell'enum
	 * cambierebbe in silenzio il giudizio scritto in un commit vecchio.
	 */
	FString StatusToString(ERTAnimClipStatus Status)
	{
		const UEnum* Enum = StaticEnum<ERTAnimClipStatus>();
		return Enum ? Enum->GetNameStringByValue(static_cast<int64>(Status)) : FString();
	}

	/** Falso quando il nome non e' uno dei quattro: uno `Status` sconosciuto e' un errore, non un default. */
	bool StatusFromString(const FString& Name, ERTAnimClipStatus& OutStatus)
	{
		const UEnum* Enum = StaticEnum<ERTAnimClipStatus>();
		if (!Enum)
		{
			return false;
		}

		const int64 Value = Enum->GetValueByNameString(Name);
		if (Value == INDEX_NONE)
		{
			return false;
		}

		OutStatus = static_cast<ERTAnimClipStatus>(Value);
		return true;
	}

	/**
	 * I tre lettori di campo OPZIONALE distinguono «assente» da «presente ma del tipo sbagliato».
	 *
	 * ⚠️ Un `TryGetStringField` nudo restituisce `false` in entrambi i casi, e chi lo chiama non puo' sapere
	 * se il campo mancava o se qualcuno ci ha scritto un numero. La differenza conta: la prima e' legale
	 * (i campi `derived` restano vuoti finche' lo scanner non gira), la seconda e' un file rotto che deve
	 * dirlo invece di lasciare il valore di default e tacere.
	 *
	 * Tre funzioni quasi identiche invece di un template su un puntatore-a-membro: `TryGetNumberField` e'
	 * **sovraccaricato** (e in parte templato) su `FJsonObject`, e un puntatore-a-membro verso un nome
	 * sovraccaricato non si risolve — l'errore che ne uscirebbe parlerebbe di deduzione, non di JSON.
	 */
	bool ReadOptionalString(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Key, FString& OutValue,
		FString& OutError)
	{
		if (!Obj->HasField(Key))
		{
			return true;
		}
		if (!Obj->TryGetStringField(Key, OutValue))
		{
			OutError = FString::Printf(TEXT("'%s': attesa una stringa"), Key);
			return false;
		}
		return true;
	}

	bool ReadOptionalBool(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Key, bool& OutValue,
		FString& OutError)
	{
		if (!Obj->HasField(Key))
		{
			return true;
		}
		if (!Obj->TryGetBoolField(Key, OutValue))
		{
			OutError = FString::Printf(TEXT("'%s': atteso un booleano"), Key);
			return false;
		}
		return true;
	}

	bool ReadOptionalNumber(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Key, double& OutValue,
		FString& OutError)
	{
		if (!Obj->HasField(Key))
		{
			return true;
		}
		if (!Obj->TryGetNumberField(Key, OutValue))
		{
			OutError = FString::Printf(TEXT("'%s': atteso un numero"), Key);
			return false;
		}
		return true;
	}
}

FString URTAnimCatalogLibrary::DefaultCatalogPath()
{
	return FPaths::Combine(FPaths::ProjectDir(), CatalogRelativePath);
}

FString URTAnimCatalogLibrary::MakeId(int32 Number)
{
	// `%04d` NON tronca oltre le quattro cifre: 10000 esce `AV_10000`. Lo zero-padding e' una comodita' di
	// lettura per i primi diecimila ID, non un limite di capienza.
	return FString::Printf(TEXT("%s%04d"), IdPrefix, Number);
}

bool URTAnimCatalogLibrary::ParseId(const FName& Id, int32& OutNumber)
{
	OutNumber = INDEX_NONE;

	if (Id.IsNone())
	{
		return false;
	}

	const FString Text = Id.ToString();
	const FString Prefix(IdPrefix);
	if (!Text.StartsWith(Prefix, ESearchCase::CaseSensitive))
	{
		return false;
	}

	const FString Digits = Text.RightChop(Prefix.Len());
	if (Digits.IsEmpty())
	{
		return false;
	}

	// Ogni carattere deve essere una cifra: `IsNumeric` da solo accetterebbe segno e separatore decimale, e
	// `AV_-1` o `AV_1.5` diventerebbero ID plausibili che nessun formatter produce.
	for (const TCHAR Ch : Digits)
	{
		if (!FChar::IsDigit(Ch))
		{
			return false;
		}
	}

	OutNumber = FCString::Atoi(*Digits);
	return true;
}

int32 URTAnimCatalogLibrary::AllocateIds(FRTAnimCatalog& Catalog, const TArray<FString>& NewAssetPaths)
{
	// I path gia' a catalogo, per rendere la chiamata idempotente. `TSet` e' lecito qui: serve solo per
	// rispondere «c'e' gia'?», e non produce ne' ordine ne' output.
	TSet<FString> KnownPaths;
	KnownPaths.Reserve(Catalog.Entries.Num());
	for (const FRTAnimCatalogEntry& Entry : Catalog.Entries)
	{
		KnownPaths.Add(Entry.Derived.AssetPath);
	}

	int32 Added = 0;
	for (const FString& Path : NewAssetPaths)
	{
		if (Path.IsEmpty())
		{
			continue;
		}

		bool bAlreadyKnown = false;
		KnownPaths.Add(Path, &bAlreadyKnown);
		if (bAlreadyKnown)
		{
			continue;
		}

		FRTAnimCatalogEntry Entry;

		// 🔴 L'ID viene dall'high-water mark, e da NIENT'ALTRO. Questa funzione non legge gli ID gia'
		// assegnati: e' cio' che le rende impossibile riciclarne uno dopo una rimozione.
		Entry.Id = FName(*MakeId(Catalog.NextId));
		++Catalog.NextId;

		// Si scrive il path e basta. ⛔ Nemmeno `AssetName` viene dedotto dalla stringa: derivare un dato dal
		// nome del file produce qualcosa che sembra misurato e non lo e'. Lo riempie lo scanner (#2446),
		// leggendo l'asset.
		Entry.Derived.AssetPath = Path;

		// `Authored` resta al default, cioe' `Unreviewed`. ⛔ Non esiste un percorso automatico verso
		// `Promoted`: non e' una promessa, e' che nessuna riga qui sotto puo' scriverlo.
		Catalog.Entries.Add(Entry);
		++Added;
	}

	return Added;
}

bool URTAnimCatalogLibrary::LoadFromString(const FString& JsonText, FRTAnimCatalog& OutCatalog,
	FString& OutError)
{
	OutCatalog = FRTAnimCatalog();
	OutError.Reset();

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutError = TEXT("JSON illeggibile");
		return false;
	}

	// --- formatVersion ------------------------------------------------------------------------------------
	if (!Root->HasField(KeyFormatVersion))
	{
		OutError = FString::Printf(TEXT("'%s' assente: un catalogo senza versione non e' leggibile in sicurezza"),
			KeyFormatVersion);
		return false;
	}

	double VersionNumber = 0.0;
	if (!Root->TryGetNumberField(KeyFormatVersion, VersionNumber))
	{
		OutError = FString::Printf(TEXT("'%s': atteso un intero"), KeyFormatVersion);
		return false;
	}
	OutCatalog.FormatVersion = static_cast<int32>(VersionNumber);

	if (OutCatalog.FormatVersion > FRTAnimCatalog::CurrentFormatVersion)
	{
		// Il messaggio accusa la BUILD, non il file: il file e' nuovo, e' questo codice a essere vecchio.
		OutError = FString::Printf(
			TEXT("formato %d: questa build ne legge al massimo %d — aggiorna la build, non il catalogo"),
			OutCatalog.FormatVersion, FRTAnimCatalog::CurrentFormatVersion);
		return false;
	}

	// --- nextId -------------------------------------------------------------------------------------------
	if (!Root->HasField(KeyNextId))
	{
		OutError = FString::Printf(
			TEXT("'%s' assente: senza high-water mark un ID rimosso verrebbe riassegnato"), KeyNextId);
		return false;
	}

	double NextIdNumber = 0.0;
	if (!Root->TryGetNumberField(KeyNextId, NextIdNumber))
	{
		OutError = FString::Printf(TEXT("'%s': atteso un intero"), KeyNextId);
		return false;
	}
	OutCatalog.NextId = static_cast<int32>(NextIdNumber);

	// --- entries ------------------------------------------------------------------------------------------
	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	if (!Root->TryGetArrayField(KeyEntries, Entries) || Entries == nullptr)
	{
		OutError = FString::Printf(TEXT("'%s': atteso un array"), KeyEntries);
		return false;
	}

	OutCatalog.Entries.Reserve(Entries->Num());
	for (int32 Index = 0; Index < Entries->Num(); ++Index)
	{
		const TSharedPtr<FJsonObject>* EntryObj = nullptr;
		if (!(*Entries)[Index]->TryGetObject(EntryObj) || EntryObj == nullptr)
		{
			OutError = FString::Printf(TEXT("voce #%d: attesa un oggetto"), Index);
			return false;
		}

		FRTAnimCatalogEntry Entry;

		// 🔴 L'ID si LEGGE. Non si conia, non si deduce dall'indice, non si ripara. Una voce senza `id` e' un
		// errore che il chiamante deve vedere: un loader che assegna al volo e' il modo in cui un `AV_ID`
		// cambia significato fra due letture senza che nessuno se ne accorga.
		FString IdText;
		if (!(*EntryObj)->TryGetStringField(KeyId, IdText) || IdText.IsEmpty())
		{
			OutError = FString::Printf(TEXT("voce #%d: '%s' assente"), Index, KeyId);
			return false;
		}
		Entry.Id = FName(*IdText);

		const TSharedPtr<FJsonObject>* DerivedObj = nullptr;
		if ((*EntryObj)->TryGetObjectField(KeyDerived, DerivedObj) && DerivedObj != nullptr)
		{
			FString FieldError;

			// `assetPath` e' l'unico obbligatorio: e' l'identita' del referente.
			(*DerivedObj)->TryGetStringField(KeyAssetPath, Entry.Derived.AssetPath);

			double Duration = 0.0;
			double Frames = 0.0;

			if (!ReadOptionalString(*DerivedObj, KeyAssetName, Entry.Derived.AssetName, FieldError)
				|| !ReadOptionalString(*DerivedObj, KeySkeleton, Entry.Derived.Skeleton, FieldError)
				|| !ReadOptionalString(*DerivedObj, KeyFingerprint, Entry.Derived.Fingerprint, FieldError)
				|| !ReadOptionalBool(*DerivedObj, KeyRootMotion, Entry.Derived.bHasRootMotion, FieldError)
				|| !ReadOptionalBool(*DerivedObj, KeyAdditive, Entry.Derived.bIsAdditive, FieldError)
				|| !ReadOptionalNumber(*DerivedObj, KeyDuration, Duration, FieldError)
				|| !ReadOptionalNumber(*DerivedObj, KeyFrameCount, Frames, FieldError))
			{
				OutError = FString::Printf(TEXT("voce #%d ('%s'): %s"), Index, *IdText, *FieldError);
				return false;
			}

			Entry.Derived.DurationSeconds = static_cast<float>(Duration);
			Entry.Derived.FrameCount = static_cast<int32>(Frames);
		}

		const TSharedPtr<FJsonObject>* AuthoredObj = nullptr;
		if ((*EntryObj)->TryGetObjectField(KeyAuthored, AuthoredObj) && AuthoredObj != nullptr)
		{
			FString StatusText;
			if ((*AuthoredObj)->TryGetStringField(KeyStatus, StatusText))
			{
				// ⚠️ Uno `Status` sconosciuto NON ricade su `Unreviewed`. Ricadere significherebbe
				// declassare in silenzio un `Promoted` scritto da una build piu' nuova — cioe' cancellare un
				// giudizio umano per un errore di battitura.
				if (!StatusFromString(StatusText, Entry.Authored.Status))
				{
					OutError = FString::Printf(TEXT("voce #%d ('%s'): status '%s' sconosciuto"),
						Index, *IdText, *StatusText);
					return false;
				}
			}

			(*AuthoredObj)->TryGetStringField(KeyLabel, Entry.Authored.Label);
			(*AuthoredObj)->TryGetStringField(KeyNotes, Entry.Authored.Notes);
		}

		OutCatalog.Entries.Add(MoveTemp(Entry));
	}

	return true;
}

bool URTAnimCatalogLibrary::LoadFromFile(const FString& FilePath, FRTAnimCatalog& OutCatalog,
	FString& OutError)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FilePath))
	{
		// «assente» e «illeggibile» sono la stessa riga solo perche' `LoadFileToString` non li distingue;
		// il path nel messaggio e' cio' che permette di distinguerli a mano in un secondo.
		OutError = FString::Printf(TEXT("catalogo non leggibile: %s"), *FilePath);
		return false;
	}
	return LoadFromString(Text, OutCatalog, OutError);
}

bool URTAnimCatalogLibrary::SaveToString(const FRTAnimCatalog& Catalog, FString& OutJsonText)
{
	OutJsonText.Reset();

	// ⚠️ Si scrive con il WRITER, campo per campo, e non costruendo un `FJsonObject` da serializzare: un
	// `FJsonObject` e' una `TMap`, e l'ordine delle sue chiavi in uscita non e' garantito. Un authoring
	// source che riordina le proprie chiavi a ogni salvataggio produce diff di puro rumore, e il primo
	// effetto e' che nessuno li legge piu'.
	const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutJsonText);

	Writer->WriteObjectStart();
	Writer->WriteValue(KeyFormatVersion, Catalog.FormatVersion);
	Writer->WriteValue(KeyNextId, Catalog.NextId);

	Writer->WriteArrayStart(KeyEntries);
	for (const FRTAnimCatalogEntry& Entry : Catalog.Entries)
	{
		Writer->WriteObjectStart();
		Writer->WriteValue(KeyId, Entry.Id.ToString());

		Writer->WriteObjectStart(KeyDerived);
		Writer->WriteValue(KeyAssetPath, Entry.Derived.AssetPath);
		Writer->WriteValue(KeyAssetName, Entry.Derived.AssetName);
		Writer->WriteValue(KeySkeleton, Entry.Derived.Skeleton);
		Writer->WriteValue(KeyDuration, Entry.Derived.DurationSeconds);
		Writer->WriteValue(KeyFrameCount, Entry.Derived.FrameCount);
		Writer->WriteValue(KeyRootMotion, Entry.Derived.bHasRootMotion);
		Writer->WriteValue(KeyAdditive, Entry.Derived.bIsAdditive);
		Writer->WriteValue(KeyFingerprint, Entry.Derived.Fingerprint);
		Writer->WriteObjectEnd();

		Writer->WriteObjectStart(KeyAuthored);
		Writer->WriteValue(KeyStatus, StatusToString(Entry.Authored.Status));
		Writer->WriteValue(KeyLabel, Entry.Authored.Label);
		Writer->WriteValue(KeyNotes, Entry.Authored.Notes);
		Writer->WriteObjectEnd();

		Writer->WriteObjectEnd();
	}
	Writer->WriteArrayEnd();

	Writer->WriteObjectEnd();
	return Writer->Close();
}

TArray<FString> URTAnimCatalogLibrary::ValidateCatalog(const FRTAnimCatalog* Catalog)
{
	TArray<FString> Errors;

	if (!Catalog)
	{
		Errors.Add(TEXT("catalogo assente"));
		return Errors;
	}

	if (Catalog->FormatVersion > FRTAnimCatalog::CurrentFormatVersion)
	{
		Errors.Add(FString::Printf(
			TEXT("formato %d: questa build ne legge al massimo %d — aggiorna la build, non il catalogo"),
			Catalog->FormatVersion, FRTAnimCatalog::CurrentFormatVersion));
	}

	if (Catalog->Entries.Num() == 0)
	{
		// 🔴 Un catalogo vuoto non e' «zero mancanze»: e' la mancanza totale. E' lo stato del progetto PRIMA
		// di questa issue — 85 clip per un solo eroe e nessun dato che dica quale sia stata guardata — e un
		// gate che lo dichiarasse valido tacerebbe proprio nel caso peggiore.
		Errors.Add(TEXT("catalogo vuoto: nessuna clip e' mai stata guardata — non e' «zero mancanze», e' la mancanza totale"));
		return Errors;
	}

	// Gli insiemi servono solo a rispondere «l'ho gia' visto?»: non producono ne' ordine ne' righe. L'ordine
	// dell'output e' quello dell'array, quindi e' deterministico per costruzione.
	TSet<FName> SeenIds;
	TMap<FString, FName> PathOwner;
	int32 HighestAssignedId = INDEX_NONE;

	for (int32 Index = 0; Index < Catalog->Entries.Num(); ++Index)
	{
		const FRTAnimCatalogEntry& Entry = Catalog->Entries[Index];

		if (Entry.Id.IsNone())
		{
			Errors.Add(FString::Printf(TEXT("voce #%d: id assente"), Index));
			continue;
		}

		const FString IdText = Entry.Id.ToString();

		int32 Number = INDEX_NONE;
		if (!ParseId(Entry.Id, Number))
		{
			Errors.Add(FString::Printf(TEXT("'%s': formato non valido, atteso %s + cifre"), *IdText, IdPrefix));
		}
		else
		{
			HighestAssignedId = FMath::Max(HighestAssignedId, Number);
		}

		bool bDuplicateId = false;
		SeenIds.Add(Entry.Id, &bDuplicateId);
		if (bDuplicateId)
		{
			Errors.Add(FString::Printf(TEXT("'%s': AV_ID duplicato"), *IdText));
		}

		if (Entry.Derived.AssetPath.IsEmpty())
		{
			Errors.Add(FString::Printf(TEXT("'%s': assetPath vuoto"), *IdText));
		}
		else if (const FName* Owner = PathOwner.Find(Entry.Derived.AssetPath))
		{
			// Due ID sullo stesso asset significa che il giudizio umano si e' biforcato senza che nessuno lo
			// abbia deciso: quale delle due voci valga sarebbe ambiguo, e l'ambiguita' non e' un giudizio.
			Errors.Add(FString::Printf(TEXT("'%s': referenziato da '%s' e '%s'"),
				*Entry.Derived.AssetPath, *Owner->ToString(), *IdText));
		}
		else
		{
			PathOwner.Add(Entry.Derived.AssetPath, Entry.Id);
		}
	}

	// 🔴 L'high-water mark deve DOMINARE STRETTAMENTE ogni ID in uso. Senza questo controllo, un file
	// modificato a mano che abbassa `nextId` farebbe riassegnare al prossimo `AllocateIds` un ID che in un
	// commit precedente significava un'altra clip — e il difetto si vedrebbe mesi dopo, in un diff che nomina
	// due clip diverse con lo stesso `AV_ID`. Cosi' invece il riciclo resta *rappresentabile ma non valido*.
	if (HighestAssignedId != INDEX_NONE && Catalog->NextId <= HighestAssignedId)
	{
		Errors.Add(FString::Printf(
			TEXT("nextId %d non domina %s: un ID gia' assegnato verrebbe riciclato"),
			Catalog->NextId, *MakeId(HighestAssignedId)));
	}

	return Errors;
}

TArray<FString> URTAnimCatalogLibrary::ValidateReferents(const FRTAnimCatalog* Catalog, bool& bOutRan)
{
	bOutRan = false;
	TArray<FString> Missing;

	if (!Catalog)
	{
		return Missing;
	}

	// I pack Paragon sono gitignorati (`.gitignore:105`, ~48 GB): su una macchina che non li ha questa meta'
	// del validator non ha nulla da misurare.
	//
	// 🔴 E in quel caso NON restituisce un array vuoto facendo finta di niente: `bOutRan` resta falso, e chi
	// chiama e' costretto a scrivere `NOT RUN`. Un `TArray` vuoto senza questo flag sarebbe indistinguibile
	// da «tutti i referenti esistono» — il verde per assenza, che e' il peggiore degli esiti perche' nessuno
	// va a guardarlo.
	const FString PackRoot = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("FabAsset"));
	if (!FPaths::DirectoryExists(PackRoot))
	{
		return Missing;
	}

	bOutRan = true;

	for (const FRTAnimCatalogEntry& Entry : Catalog->Entries)
	{
		if (Entry.Derived.AssetPath.IsEmpty())
		{
			continue;
		}

		// `/Game/FabAsset/.../Idle.Idle` -> `<Content>/FabAsset/.../Idle.uasset`. Si taglia l'`.Oggetto`
		// finale (che in un percorso oggetto Unreal separa pacchetto e oggetto) e si sostituisce `/Game/`.
		FString PackagePath = Entry.Derived.AssetPath;
		int32 DotIndex = INDEX_NONE;
		if (PackagePath.FindLastChar(TEXT('.'), DotIndex))
		{
			PackagePath = PackagePath.Left(DotIndex);
		}

		if (!PackagePath.StartsWith(TEXT("/Game/")))
		{
			Missing.Add(FString::Printf(TEXT("'%s': '%s' non e' un percorso /Game/"),
				*Entry.Id.ToString(), *Entry.Derived.AssetPath));
			continue;
		}

		const FString OnDisk = FPaths::Combine(FPaths::ProjectContentDir(),
			PackagePath.RightChop(6) + TEXT(".uasset"));

		if (!FPaths::FileExists(OnDisk))
		{
			Missing.Add(FString::Printf(TEXT("'%s': referente assente sul disco (%s)"),
				*Entry.Id.ToString(), *Entry.Derived.AssetPath));
		}
	}

	return Missing;
}
