#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

const TCHAR* URTScenarioIndex::RedirectsFileName = TEXT("_redirects.json");

namespace
{
	/** Interpreta un testo JSON in un oggetto radice. Nullptr se non è JSON o se la radice non è un oggetto. */
	TSharedPtr<FJsonObject> ParseObject(const FString& JsonText)
	{
		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			return nullptr;
		}
		return Root;
	}
}

FString URTScenarioIndex::NormalizeTag(const FString& Tag)
{
	return Tag.TrimStartAndEnd().ToLower();
}

bool URTScenarioIndex::ReadHeader(const FString& JsonText, FString& OutId, TArray<FString>& OutTags, FString& OutError)
{
	OutId.Reset();
	OutTags.Reset();
	OutError.Reset();

	const TSharedPtr<FJsonObject> Root = ParseObject(JsonText);
	if (!Root.IsValid())
	{
		OutError = TEXT("JSON non interpretabile");
		return false;
	}

	if (!Root->TryGetStringField(TEXT("scenarioId"), OutId) || OutId.IsEmpty())
	{
		OutError = TEXT("campo obbligatorio mancante: scenarioId");
		return false;
	}

	// I tag sono opzionali: uno scenario senza tag esiste, si esegue, e compare solo nell'elenco non
	// filtrato. Renderli obbligatori significherebbe bloccare la scrittura di uno scenario nuovo su una
	// decisione di catalogazione, che è esattamente l'ordine sbagliato.
	const TArray<TSharedPtr<FJsonValue>>* TagsJson = nullptr;
	if (Root->TryGetArrayField(TEXT("tags"), TagsJson))
	{
		for (const TSharedPtr<FJsonValue>& Value : *TagsJson)
		{
			FString Tag;
			if (!Value.IsValid() || !Value->TryGetString(Tag))
			{
				OutError = TEXT("tags: ogni voce deve essere una stringa");
				return false;
			}
			const FString Normalized = NormalizeTag(Tag);
			if (!Normalized.IsEmpty())
			{
				OutTags.AddUnique(Normalized);
			}
		}
	}
	OutTags.Sort();
	return true;
}

TArray<FRTScenarioEntry> URTScenarioIndex::BuildFrom(const TArray<TPair<FString, FString>>& Files, TArray<FString>& OutProblems)
{
	TArray<FRTScenarioEntry> Entries;
	OutProblems.Reset();

	for (const TPair<FString, FString>& File : Files)
	{
		FRTScenarioEntry Entry;
		FString Error;
		if (!ReadHeader(File.Value, Entry.ScenarioId, Entry.Tags, Error))
		{
			OutProblems.Add(FString::Printf(TEXT("%s: %s"), *File.Key, *Error));
			continue;
		}
		Entry.Path = File.Key;
		Entries.Add(MoveTemp(Entry));
	}

	// Ordine totale sul percorso PRIMA di cercare i duplicati: l'ordine in cui il filesystem restituisce i
	// file non è garantito, e un messaggio d'errore che cambia formulazione fra due esecuzioni è un
	// messaggio di cui non ci si fida.
	Entries.Sort([](const FRTScenarioEntry& A, const FRTScenarioEntry& B) { return A.Path < B.Path; });

	// Un ID dichiarato da due file è la classe di errore che questo indice introduce: finché l'ID era il
	// percorso il filesystem la rendeva impossibile. Va segnalata qui, dove si vedono tutti i file insieme.
	TMap<FString, TArray<FString>> PathsById;
	for (const FRTScenarioEntry& Entry : Entries)
	{
		PathsById.FindOrAdd(Entry.ScenarioId).Add(Entry.Path);
	}
	TArray<FString> DuplicatedIds;
	for (const TPair<FString, TArray<FString>>& Pair : PathsById)
	{
		if (Pair.Value.Num() > 1)
		{
			DuplicatedIds.Add(Pair.Key);
		}
	}
	DuplicatedIds.Sort();
	for (const FString& Id : DuplicatedIds)
	{
		OutProblems.Add(FString::Printf(TEXT("scenarioId '%s' dichiarato da %d file: %s"),
			*Id, PathsById[Id].Num(), *FString::Join(PathsById[Id], TEXT(", "))));
	}

	return Entries;
}

TArray<FRTScenarioEntry> URTScenarioIndex::Scan(TArray<FString>& OutProblems)
{
	const FString Root = URTScenarioLoader::ScenariosRoot();

	TArray<FString> FoundFiles;
	IFileManager::Get().FindFilesRecursive(FoundFiles, *Root, TEXT("*.json"),
		/*Files=*/ true, /*Directories=*/ false);

	TArray<TPair<FString, FString>> Loaded;
	TArray<FString> ReadProblems;
	for (const FString& File : FoundFiles)
	{
		// I file che cominciano per `_` non sono scenari: è così che la tabella di redirect vive accanto
		// agli scenari senza comparire nella tendina.
		if (FPaths::GetCleanFilename(File).StartsWith(TEXT("_")))
		{
			continue;
		}

		FString Text;
		const FString FullPath = FPaths::ConvertRelativePathToFull(File);
		if (!FFileHelper::LoadFileToString(Text, *FullPath))
		{
			ReadProblems.Add(FString::Printf(TEXT("%s: file non leggibile"), *FullPath));
			continue;
		}
		Loaded.Emplace(FullPath, MoveTemp(Text));
	}

	TArray<FRTScenarioEntry> Entries = BuildFrom(Loaded, OutProblems);
	OutProblems.Append(ReadProblems);
	return Entries;
}

TMap<FString, FString> URTScenarioIndex::LoadRedirects()
{
	TMap<FString, FString> Redirects;

	const FString Path = FPaths::Combine(URTScenarioLoader::ScenariosRoot(), RedirectsFileName);
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		// Tabella assente: è il caso normale finché nessuno ha rinominato niente.
		return Redirects;
	}

	const TSharedPtr<FJsonObject> Root = ParseObject(Text);
	if (!Root.IsValid())
	{
		return Redirects;
	}
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
	{
		// Le chiavi che cominciano per `_` non sono redirect: e' come il file resta commentabile senza che
		// il commento diventi un ID. Stessa convenzione dei file `_*.json` saltati dalla scansione.
		if (Pair.Key.StartsWith(TEXT("_")))
		{
			continue;
		}
		FString NewId;
		if (Pair.Value.IsValid() && Pair.Value->TryGetString(NewId) && !NewId.IsEmpty())
		{
			Redirects.Add(Pair.Key, NewId);
		}
	}
	return Redirects;
}

FString URTScenarioIndex::ApplyRedirects(const TMap<FString, FString>& Redirects, const FString& ScenarioId)
{
	FString Current = ScenarioId;
	for (int32 Hop = 0; Hop < MaxRedirectHops; ++Hop)
	{
		const FString* Next = Redirects.Find(Current);
		if (!Next)
		{
			return Current;
		}
		Current = *Next;
	}
	// Catena troppo lunga o ciclica: si restituisce l'ultimo ID raggiunto e il chiamante fallirà il lookup
	// con un messaggio che nomina l'ID. Girare all'infinito sarebbe l'unico esito peggiore.
	return Current;
}

bool URTScenarioIndex::IsSegmentSuffix(const FString& Full, const FString& Abbrev)
{
	TArray<FString> A, B;
	Full.ParseIntoArray(A, TEXT("."));
	Abbrev.ParseIntoArray(B, TEXT("."));
	if (B.Num() == 0 || B.Num() > A.Num())
	{
		return false;
	}
	// Dal FONDO: e' il verso in cui un ID gerarchico si abbrevia. Confrontare dall'inizio farebbe
	// risolvere `Visual` a uno qualunque dei suoi quaranta scenari.
	for (int32 i = 0; i < B.Num(); ++i)
	{
		if (!A[A.Num() - 1 - i].Equals(B[B.Num() - 1 - i], ESearchCase::IgnoreCase))
		{
			return false;
		}
	}
	return true;
}

FString URTScenarioIndex::ResolvePath(const FString& ScenarioId, FString& OutError)
{
	OutError.Reset();

	if (ScenarioId.IsEmpty())
	{
		OutError = TEXT("scenarioId vuoto");
		return FString();
	}

	TArray<FString> Problems;
	const TArray<FRTScenarioEntry> Entries = Scan(Problems);

	// Cerca prima l'ID così com'è: un ID **vivo** deve vincere su una voce di redirect rimasta indietro,
	// altrimenti riusare un nome liberato porterebbe al file sbagliato.
	auto MatchesOf = [&Entries](const FString& Id)
	{
		TArray<FString> Paths;
		for (const FRTScenarioEntry& Entry : Entries)
		{
			if (Entry.ScenarioId == Id)
			{
				Paths.Add(Entry.Path);
			}
		}
		return Paths;
	};

	FString Effective = ScenarioId;
	TArray<FString> Matches = MatchesOf(Effective);
	if (Matches.Num() == 0)
	{
		const FString Redirected = ApplyRedirects(LoadRedirects(), ScenarioId);
		if (Redirected != ScenarioId)
		{
			Effective = Redirected;
			Matches = MatchesOf(Effective);
		}
	}

	// 🔑 **Terzo tentativo: l'ABBREVIAZIONE, e solo dopo che gli altri due hanno fallito** (#2257).
	//
	// `Visual.Environment.IceSlide` si scrive `IceSlide`, o `Environment.IceSlide` se il solo nome finale
	// non basta. Sui 118 scenari del corpus l'ultimo segmento e' univoco per **115**: l'unico ambiguo e'
	// `Acceptance`, che tre scenari dichiarano.
	//
	// 🔴 **L'ordine dei tre tentativi e' la garanzia, non una comodita'.** Un ID **vivo** vince su tutto, e
	// un redirect vince su un'abbreviazione: cosi' riusare un nome liberato porta al file giusto, e nessuna
	// scorciatoia puo' dirottare un ID che esiste davvero. Invertire l'ordine sarebbe il difetto che questa
	// funzione evitava gia' per i redirect.
	//
	// ⛔ **Il confine e' il SEGMENTO, non il carattere.** `ceSlide` non risolve, e non e' pignoleria: un
	// match per sottostringa sembra comodo finche' non nasce `IceBridge`, e da quel giorno `Ice` sceglie in
	// silenzio. Qui si confrontano segmenti interi dal fondo, che e' il modo in cui un ID gerarchico si
	// abbrevia davvero.
	if (Matches.Num() == 0)
	{
		TArray<FString> Candidati;
		for (const FRTScenarioEntry& Entry : Entries)
		{
			if (IsSegmentSuffix(Entry.ScenarioId, ScenarioId))
			{
				Candidati.AddUnique(Entry.ScenarioId);
			}
		}

		if (Candidati.Num() == 1)
		{
			Effective = Candidati[0];
			Matches = MatchesOf(Effective);
		}
		else if (Candidati.Num() > 1)
		{
			// ✅ **Validato per mutazione**: fatto scegliere «il primo che trova»,
			// `ScenarioIndex.AbbreviationResolvesOrRefuses` cade con *«un'abbreviazione ambigua NON risolve»*.
			// ⛔ **Ambiguo = NON parte, e il messaggio elenca i candidati.** Sceglierne uno «il primo che
			// trova» farebbe girare lo scenario sbagliato **senza dirlo** — peggio del typo che questa
			// funzione chiude, perche' quello almeno non fa nulla. Chi legge deve poter disambiguare
			// aggiungendo un segmento, e per farlo deve vedere fra cosa sta scegliendo.
			Candidati.Sort();
			OutError = FString::Printf(
				TEXT("abbreviazione '%s' ambigua, %d scenari la soddisfano: %s · aggiungi un segmento"),
				*ScenarioId, Candidati.Num(), *FString::Join(Candidati, TEXT(", ")));
			return FString();
		}
	}

	if (Matches.Num() == 1)
	{
		return Matches[0];
	}

	if (Matches.Num() > 1)
	{
		OutError = FString::Printf(TEXT("scenarioId '%s' ambiguo, dichiarato da %d file: %s"),
			*Effective, Matches.Num(), *FString::Join(Matches, TEXT(", ")));
		return FString();
	}

	// Nessuna corrispondenza: il messaggio dice DOVE si è cercato e quanti scenari c'erano, così chi legge
	// distingue «ho sbagliato l'ID» da «la cartella degli scenari non è quella che credevo».
	OutError = FString::Printf(TEXT("scenario '%s' non trovato nell'indice (%d scenari sotto %s)"),
		*ScenarioId, Entries.Num(), *URTScenarioLoader::ScenariosRoot());
	if (Problems.Num() > 0)
	{
		OutError += FString::Printf(TEXT(" · %d file con problemi: %s"), Problems.Num(), *FString::Join(Problems, TEXT(" · ")));
	}
	return FString();
}

TArray<FString> URTScenarioIndex::ListIds(const FString& FilterA, const FString& FilterB)
{
	TArray<FString> Problems;
	const TArray<FRTScenarioEntry> Entries = Scan(Problems);

	const FString A = NormalizeTag(FilterA);
	const FString B = NormalizeTag(FilterB);

	TArray<FString> Ids;
	for (const FRTScenarioEntry& Entry : Entries)
	{
		// Filtro vuoto = nessuna restrizione. Entrambi devono passare: intersezione, non unione.
		if (!A.IsEmpty() && !Entry.Tags.Contains(A)) { continue; }
		if (!B.IsEmpty() && !Entry.Tags.Contains(B)) { continue; }
		Ids.AddUnique(Entry.ScenarioId);
	}
	Ids.Sort();
	return Ids;
}

TArray<FString> URTScenarioIndex::ListTags()
{
	TArray<FString> Problems;
	const TArray<FRTScenarioEntry> Entries = Scan(Problems);

	TArray<FString> Tags;
	for (const FRTScenarioEntry& Entry : Entries)
	{
		for (const FString& Tag : Entry.Tags)
		{
			Tags.AddUnique(Tag);
		}
	}
	Tags.Sort();
	return Tags;
}
