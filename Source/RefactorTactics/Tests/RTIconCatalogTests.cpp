#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Core/RTGameplayTags.h"
#include "UI/RTIconCatalogData.h"
#include "UI/RTIconLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Soft reference non nulla ma volutamente non risolvibile: al CP 20.1 conta che la voce DICHIARI un
	 *  asset, non che l'asset esista (la risolvibilita' al cook e' CP 25.2). Nome distinto per file (unity). */
	TSoftObjectPtr<UTexture2D> MakeIconAssetRef(const FName& IconId)
	{
		// I punti dell'IconId diventano underscore: in un `FSoftObjectPath` il punto separa pacchetto e
		// oggetto, e lasciarli produrrebbe un percorso che non significa quello che sembra.
		const FString Leaf = IconId.ToString().Replace(TEXT("."), TEXT("_"));
		return TSoftObjectPtr<UTexture2D>(FSoftObjectPath(
			FString::Printf(TEXT("/Game/RT/UI/Icons/T_%s.T_%s"), *Leaf, *Leaf)));
	}

	/** Categoria dedotta dal segmento dentro l'ID, cosi' il catalogo di prova e' coerente per costruzione. */
	ERTIconCategory IconCategoryFromId(const FName& IconId)
	{
		const FString Id = IconId.ToString();
		const UEnum* Enum = StaticEnum<ERTIconCategory>();
		for (int32 i = 0; Enum && i < Enum->NumEnums() - 1; ++i)
		{
			const FString Name = Enum->GetNameStringByIndex(i);
			if (Id.StartsWith(FString::Printf(TEXT("UI.Icon.%s."), *Name)))
			{
				return static_cast<ERTIconCategory>(Enum->GetValueByIndex(i));
			}
		}
		return ERTIconCategory::Identity;
	}

	/** Catalogo che copre esattamente l'insieme richiesto. */
	URTIconCatalogData* MakeCoveringIconCatalog()
	{
		URTIconCatalogData* Catalog = NewObject<URTIconCatalogData>();
		Catalog->MissingIcon = MakeIconAssetRef(TEXT("Missing"));
		for (const FName& Id : URTIconLibrary::RequiredIconIds())
		{
			Catalog->Icons.Add(FRTIconDef(Id, IconCategoryFromId(Id), MakeIconAssetRef(Id)));
		}
		return Catalog;
	}
}

// ---------------------------------------------------------------------------------------------------------
// L'insieme richiesto viene dai DATI DI GIOCO, non dal catalogo: e' cio' che impedisce a EveryKeyResolves di
// essere vero per costruzione (un catalogo vuoto non deve poterlo passare)
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIconRequiredIdsTest,
	"RefactorTactics.IconCatalog.RequiredIdsFollowGameData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIconRequiredIdsTest::RunTest(const FString&)
{
	const TArray<FName> Required = URTIconLibrary::RequiredIconIds();

	// Ogni azione del catalogo generico ha la sua chiave: se domani ne nasce una nuova, questo confronto la
	// pretende senza che nessuno debba ricordarsi di aggiornare una lista.
	const TArray<FRTActionDef> CoreActions = URTCatalogLibrary::GetCoreActionCatalog();
	TestTrue(TEXT("il catalogo generico non e' vuoto"), CoreActions.Num() > 0);
	for (const FRTActionDef& Def : CoreActions)
	{
		TestTrue(*FString::Printf(TEXT("chiave richiesta per %s"), *Def.ActionId.ToString()),
			Required.Contains(URTIconLibrary::MakeIconId(Def.ActionId)));
	}

	// Ogni stato che il gioco sa applicare ha la sua chiave.
	TestTrue(TEXT("Status.Wet e' richiesto"), Required.Contains(FName(TEXT("UI.Icon.Status.Wet"))));
	TestTrue(TEXT("Status.Burning e' richiesto"), Required.Contains(FName(TEXT("UI.Icon.Status.Burning"))));
	TestTrue(TEXT("Status.Electrified e' richiesto"), Required.Contains(FName(TEXT("UI.Icon.Status.Electrified"))));

	// Le quattro fasi volontarie, e SOLO quelle.
	TestTrue(TEXT("Phase.Prep"), Required.Contains(FName(TEXT("UI.Icon.Phase.Prep"))));
	TestTrue(TEXT("Phase.Dash"), Required.Contains(FName(TEXT("UI.Icon.Phase.Dash"))));
	TestTrue(TEXT("Phase.Blast"), Required.Contains(FName(TEXT("UI.Icon.Phase.Blast"))));
	TestTrue(TEXT("Phase.Move"), Required.Contains(FName(TEXT("UI.Icon.Phase.Move"))));
	TestFalse(TEXT("Planning non e' una fase in cui si agisce"),
		Required.Contains(FName(TEXT("UI.Icon.Phase.Planning"))));
	TestFalse(TEXT("Cleanup non e' una fase in cui si agisce"),
		Required.Contains(FName(TEXT("UI.Icon.Phase.Cleanup"))));

	// Il prefisso tiene separate icone e Gameplay Tag: nessuna chiave e' il nome nudo di un tag.
	for (const FName& Id : Required)
	{
		TestTrue(*FString::Printf(TEXT("'%s' ha il prefisso UI.Icon."), *Id.ToString()),
			Id.ToString().StartsWith(TEXT("UI.Icon.")));
	}

	// Deterministico: due chiamate danno la stessa lista nello stesso ordine (i tag arrivano da un manager
	// che non garantisce l'ordine, ed e' per questo che vengono ordinati).
	TestTrue(TEXT("due chiamate danno la stessa lista"), Required == URTIconLibrary::RequiredIconIds());

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// CP 20.2: le cinque categorie della v0.1 sono POPOLATE, le altre sette restano dichiarate e VUOTE
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIconV01CategoriesTest,
	"RefactorTactics.IconCatalog.V01CategoriesPopulated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIconV01CategoriesTest::RunTest(const FString&)
{
	const TArray<FName> Required = URTIconLibrary::RequiredIconIds();

	// Quali categorie compaiono davvero nell'insieme richiesto, dedotte dal segmento dentro ogni chiave.
	TSet<ERTIconCategory> Populated;
	for (const FName& Id : Required)
	{
		const FString Text = Id.ToString();
		const UEnum* Enum = StaticEnum<ERTIconCategory>();
		for (int32 i = 0; Enum && i < Enum->NumEnums() - 1; ++i)
		{
			if (Text.StartsWith(FString::Printf(TEXT("UI.Icon.%s."), *Enum->GetNameStringByIndex(i))))
			{
				Populated.Add(static_cast<ERTIconCategory>(Enum->GetValueByIndex(i)));
				break;
			}
		}
	}

	// Le cinque della v0.1 (#219). `Identity` e `Certainty` sono qui perche' CP 20.2 le ha rese DERIVATE —
	// dal roster eroi la prima, dai tre livelli di CP 11.2 la seconda. Scritte a mano nel data asset
	// sarebbero state chiavi che nessuna macchina pretende, cioe' il difetto che CP 20.1 evita per le altre.
	for (const ERTIconCategory Category : { ERTIconCategory::Identity, ERTIconCategory::Action,
		ERTIconCategory::Phase, ERTIconCategory::Status, ERTIconCategory::Certainty })
	{
		TestTrue(*FString::Printf(TEXT("la categoria %s della v0.1 e' popolata"),
			*URTIconLibrary::CategoryName(Category)), Populated.Contains(Category));
	}

	// Le altre sette restano VUOTE di proposito: la tassonomia e' dichiarata, gli asset no. Una chiave che
	// comparisse qui chiederebbe un disegno per un sistema che ancora non la consuma — ed e' esattamente
	// quello che #219 dice di non fare.
	for (const ERTIconCategory Category : { ERTIconCategory::Environment, ERTIconCategory::MapInteraction,
		ERTIconCategory::Information, ERTIconCategory::Reaction, ERTIconCategory::Coordination,
		ERTIconCategory::Warning, ERTIconCategory::Objective })
	{
		TestFalse(*FString::Printf(TEXT("la categoria %s non e' ancora popolata"),
			*URTIconLibrary::CategoryName(Category)), Populated.Contains(Category));
	}

	// I quattro eroi del roster, con il prefisso TRADOTTO: `Hero.Flux` -> `UI.Icon.Identity.Flux`. Senza la
	// traduzione la chiave sarebbe `UI.Icon.Hero.Flux`, che il validator rifiuta perche' il segmento non
	// combacia con la categoria dichiarata.
	//
	// ⚠️ **La fonte qui e' `GetHeroRoster()`, non `GetHeroIds()`, e la differenza e' tutto il valore del
	// test.** `RequiredIconIds()` deriva le chiavi da `GetHeroIds()`: confrontarle con la stessa lista
	// renderebbe questo controllo vero per costruzione — un `Hero.Vektorr` scritto per sbaglio produrrebbe
	// `UI.Icon.Identity.Vektorr` e il test lo troverebbe, contento. Misurato: con quella mutazione attiva il
	// test passava. Il roster e' la fonte che il gioco spedisce davvero, ed e' l'unica contro cui il
	// confronto significa qualcosa.
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Hero) { continue; }

		FString Name = Hero->HeroId.ToString();
		int32 Dot = INDEX_NONE;
		if (Name.FindLastChar(TEXT('.'), Dot)) { Name = Name.RightChop(Dot + 1); }

		TestTrue(*FString::Printf(TEXT("l'eroe %s ha la sua chiave Identity"), *Name),
			Required.Contains(FName(*FString::Printf(TEXT("UI.Icon.Identity.%s"), *Name))));
	}

	// E nessuna chiave Identity di troppo: le sei attese sono quattro eroi + due relazioni. Senza questo
	// conteggio un eroe fantasma aggiunto a `GetHeroIds()` passerebbe inosservato — il controllo sopra
	// verifica che ogni eroe REALE abbia la sua chiave, non che ogni chiave abbia il suo eroe.
	int32 IdentityKeys = 0;
	for (const FName& Id : Required)
	{
		if (Id.ToString().StartsWith(TEXT("UI.Icon.Identity."))) { ++IdentityKeys; }
	}
	TestEqual(TEXT("le chiavi Identity sono quattro eroi piu' due relazioni"),
		IdentityKeys, URTHeroCatalogLibrary::GetHeroRoster().Num() + 2);
	TestFalse(TEXT("nessuna chiave conserva il prefisso Hero."),
		Required.Contains(FName(TEXT("UI.Icon.Hero.Flux"))));

	// Relazione di squadra: il consumatore esiste gia' (`ARTHUD` legge `View.bIsAlly`).
	TestTrue(TEXT("Identity.Ally"), Required.Contains(FName(TEXT("UI.Icon.Identity.Ally"))));
	TestTrue(TEXT("Identity.Enemy"), Required.Contains(FName(TEXT("UI.Icon.Identity.Enemy"))));

	// I tre livelli di certezza di CP 11.2.
	TestTrue(TEXT("Certainty.Confirmed"), Required.Contains(FName(TEXT("UI.Icon.Certainty.Confirmed"))));
	TestTrue(TEXT("Certainty.Predicted"), Required.Contains(FName(TEXT("UI.Icon.Certainty.Predicted"))));
	TestTrue(TEXT("Certainty.Uncertain"), Required.Contains(FName(TEXT("UI.Icon.Certainty.Uncertain"))));

	// Un catalogo che copre l'insieme richiesto non ha piu' mancanze: e' la forma che il catalogo SPEDITO
	// dovra' avere. ⚠️ Questo test non tocca l'asset — `DA_IconCatalog` non esiste ancora e verificarlo e'
	// un passo Editor, non una funzione pura.
	TestEqual(TEXT("un catalogo che copre l'insieme richiesto non ha mancanze"),
		URTIconLibrary::FindMissingRequiredIcons(MakeCoveringIconCatalog()).Num(), 0);

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Copertura: un catalogo che copre l'insieme richiesto risolve ogni chiave
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIconEveryKeyResolvesTest,
	"RefactorTactics.IconCatalog.EveryKeyResolves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIconEveryKeyResolvesTest::RunTest(const FString&)
{
	URTIconCatalogData* Catalog = MakeCoveringIconCatalog();

	TestEqual(TEXT("catalogo coerente: nessun errore strutturale"),
		URTIconLibrary::ValidateIconCatalog(Catalog).Num(), 0);
	TestEqual(TEXT("nessuna chiave richiesta manca"),
		URTIconLibrary::FindMissingRequiredIcons(Catalog).Num(), 0);

	for (const FName& Id : URTIconLibrary::RequiredIconIds())
	{
		const FRTIconResolution Resolution = URTIconLibrary::ResolveIcon(Catalog, Id, TEXT("Test"));
		TestTrue(*FString::Printf(TEXT("'%s' risolta"), *Id.ToString()), Resolution.bResolved);
		TestFalse(*FString::Printf(TEXT("'%s' ha un asset"), *Id.ToString()), Resolution.Asset.IsNull());
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Una chiave richiesta senza icona e' un errore di copertura, non un widget vuoto
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIconMissingKeyTest,
	"RefactorTactics.IconCatalog.MissingKeyIsValidationError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIconMissingKeyTest::RunTest(const FString&)
{
	// Una chiave richiesta QUALUNQUE: questo test misura la meccanica della copertura, non quali chiavi siano
	// richieste (quello e' `RequiredIdsFollowGameData`). Sceglierne una per nome accoppierebbe due test alla
	// stessa sorgente, e toglierla dall'insieme li farebbe cadere entrambi dicendo una cosa sola.
	const TArray<FName> Required = URTIconLibrary::RequiredIconIds();
	if (!TestTrue(TEXT("l'insieme richiesto non e' vuoto"), Required.Num() > 0))
	{
		return false;
	}
	const FName Sacrificed = Required[0];

	{
		// Chiave assente del tutto.
		URTIconCatalogData* Catalog = MakeCoveringIconCatalog();
		Catalog->Icons.RemoveAll([&Sacrificed](const FRTIconDef& Icon) { return Icon.IconId == Sacrificed; });

		const TArray<FName> Missing = URTIconLibrary::FindMissingRequiredIcons(Catalog);
		TestEqual(TEXT("manca esattamente una chiave"), Missing.Num(), 1);
		TestTrue(TEXT("e' quella tolta"), Missing.Contains(Sacrificed));
	}

	{
		// Chiave DICHIARATA ma senza asset: e' il caso che il checkpoint esiste per impedire, e non deve
		// passare solo perche' la riga c'e'.
		URTIconCatalogData* Catalog = MakeCoveringIconCatalog();
		for (FRTIconDef& Icon : Catalog->Icons)
		{
			if (Icon.IconId == Sacrificed) { Icon.Asset.Reset(); }
		}

		TestTrue(TEXT("dichiarata senza asset: manca comunque"),
			URTIconLibrary::FindMissingRequiredIcons(Catalog).Contains(Sacrificed));

		const TArray<FString> Errors = URTIconLibrary::ValidateIconCatalog(Catalog);
		bool bNamesTheKey = false;
		for (const FString& E : Errors) { bNamesTheKey |= E.Contains(Sacrificed.ToString()); }
		TestTrue(TEXT("l'errore dice QUALE chiave e' rotta"), bNamesTheKey);
	}

	{
		// Nessun catalogo non e' «zero mancanze».
		TestEqual(TEXT("catalogo nullo: mancano tutte"),
			URTIconLibrary::FindMissingRequiredIcons(nullptr).Num(), URTIconLibrary::RequiredIconIds().Num());
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// A runtime una chiave sconosciuta non restituisce mai il vuoto
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIconUnknownKeyTest,
	"RefactorTactics.IconCatalog.UnknownKeyReturnsFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIconUnknownKeyTest::RunTest(const FString&)
{
	URTIconCatalogData* Catalog = MakeCoveringIconCatalog();
	const FName Unknown(TEXT("UI.Icon.Status.DoesNotExist"));

	// La warning e' il comportamento richiesto, non un difetto: il test la PRETENDE (Occurrences 0 = almeno
	// una). Se un giorno `ResolveIcon` smettesse di loggare, questo test cadrebbe.
	AddExpectedMessage(TEXT("Icona non risolta"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ 0, /*IsRegex*/ false);

	const FRTIconResolution Resolution = URTIconLibrary::ResolveIcon(Catalog, Unknown, TEXT("UnitHUD"));
	TestFalse(TEXT("la chiave non e' risolta"), Resolution.bResolved);
	TestFalse(TEXT("l'asset restituito non e' nullo: e' il missing-icon"), Resolution.Asset.IsNull());
	TestEqual(TEXT("ed e' proprio il missing-icon del catalogo"),
		Resolution.Asset.ToSoftObjectPath(), Catalog->MissingIcon.ToSoftObjectPath());

	// Senza catalogo non c'e' nulla da mostrare, ma non c'e' nemmeno nulla da dereferenziare: niente crash.
	const FRTIconResolution NoCatalog = URTIconLibrary::ResolveIcon(nullptr, Unknown, TEXT("UnitHUD"));
	TestFalse(TEXT("senza catalogo: non risolta"), NoCatalog.bResolved);

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Validazione strutturale: ogni caso da solo, perche' un validator che segnalasse sempre lo stesso errore
// passerebbe un test cumulativo senza distinguere i casi
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIconValidatorTest,
	"RefactorTactics.IconCatalog.DuplicateIdIsValidationError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIconValidatorTest::RunTest(const FString&)
{
	const FName Wet(TEXT("UI.Icon.Status.Wet"));

	{
		// Catalogo minimo costruito qui, non quello di copertura: la validazione strutturale non deve dipendere
		// da quali chiavi il gioco richieda oggi.
		URTIconCatalogData* Catalog = NewObject<URTIconCatalogData>();
		Catalog->MissingIcon = MakeIconAssetRef(TEXT("Missing"));
		Catalog->Icons.Add(FRTIconDef(Wet, ERTIconCategory::Status, MakeIconAssetRef(Wet)));
		Catalog->Icons.Add(FRTIconDef(Wet, ERTIconCategory::Status, MakeIconAssetRef(Wet)));

		const TArray<FString> Errors = URTIconLibrary::ValidateIconCatalog(Catalog);
		TestTrue(TEXT("id duplicato: almeno un errore"), Errors.Num() > 0);
		bool bNamesTheId = false;
		for (const FString& E : Errors) { bNamesTheId |= E.Contains(Wet.ToString()); }
		TestTrue(TEXT("l'errore dice QUALE id e' duplicato"), bNamesTheId);
	}

	{
		URTIconCatalogData* Catalog = NewObject<URTIconCatalogData>();
		Catalog->MissingIcon = MakeIconAssetRef(TEXT("Missing"));
		Catalog->Icons.Add(FRTIconDef(FName(TEXT("Status.Wet")), ERTIconCategory::Status, MakeIconAssetRef(Wet)));
		TestTrue(TEXT("senza prefisso UI.Icon.: rifiutato"),
			URTIconLibrary::ValidateIconCatalog(Catalog).Num() > 0);
	}

	{
		URTIconCatalogData* Catalog = NewObject<URTIconCatalogData>();
		Catalog->MissingIcon = MakeIconAssetRef(TEXT("Missing"));
		// L'ID dice Status, la categoria dichiara Warning: incoerenza che nessun filtro per categoria
		// scoprirebbe da solo.
		Catalog->Icons.Add(FRTIconDef(Wet, ERTIconCategory::Warning, MakeIconAssetRef(Wet)));
		TestTrue(TEXT("categoria incoerente con l'ID: rifiutata"),
			URTIconLibrary::ValidateIconCatalog(Catalog).Num() > 0);
	}

	{
		URTIconCatalogData* Catalog = NewObject<URTIconCatalogData>();
		Catalog->MissingIcon = MakeIconAssetRef(TEXT("Missing"));
		Catalog->Icons.Add(FRTIconDef(NAME_None, ERTIconCategory::Status, MakeIconAssetRef(Wet)));
		TestTrue(TEXT("IconId assente: rifiutato"), URTIconLibrary::ValidateIconCatalog(Catalog).Num() > 0);
	}

	{
		// Senza missing-icon, `ResolveIcon` non avrebbe nulla da restituire a una chiave sconosciuta.
		URTIconCatalogData* Catalog = NewObject<URTIconCatalogData>();
		Catalog->Icons.Add(FRTIconDef(Wet, ERTIconCategory::Status, MakeIconAssetRef(Wet)));
		TestTrue(TEXT("MissingIcon non impostata: rifiutata"),
			URTIconLibrary::ValidateIconCatalog(Catalog).Num() > 0);
	}

	{
		TestTrue(TEXT("catalogo nullo: rifiutato"), URTIconLibrary::ValidateIconCatalog(nullptr).Num() > 0);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
