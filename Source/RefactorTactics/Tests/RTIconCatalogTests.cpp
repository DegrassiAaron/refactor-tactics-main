#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
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

	// I quattro eroi del roster, con il prefisso TRADOTTO: `Hero.Aevik` -> `UI.Icon.Identity.Aevik`. Senza la
	// traduzione la chiave sarebbe `UI.Icon.Hero.Aevik`, che il validator rifiuta perche' il segmento non
	// combacia con la categoria dichiarata.
	//
	// ⚠️ **La fonte qui e' `GetHeroRoster()`, non `GetHeroIds()`, e la differenza e' tutto il valore del
	// test.** `RequiredIconIds()` deriva le chiavi da `GetHeroIds()`: confrontarle con la stessa lista
	// renderebbe questo controllo vero per costruzione — un `Hero.Ivrinr` scritto per sbaglio produrrebbe
	// `UI.Icon.Identity.Ivrinr` e il test lo troverebbe, contento. Misurato: con quella mutazione attiva il
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
		Required.Contains(FName(TEXT("UI.Icon.Hero.Aevik"))));

	// Relazione di squadra: il consumatore esiste gia' (`ARTHUD` legge `View.bIsAlly`).
	TestTrue(TEXT("Identity.Ally"), Required.Contains(FName(TEXT("UI.Icon.Identity.Ally"))));
	TestTrue(TEXT("Identity.Enemy"), Required.Contains(FName(TEXT("UI.Icon.Identity.Enemy"))));

	// I tre livelli di certezza di CP 11.2.
	TestTrue(TEXT("Certainty.Confirmed"), Required.Contains(FName(TEXT("UI.Icon.Certainty.Confirmed"))));
	TestTrue(TEXT("Certainty.Predicted"), Required.Contains(FName(TEXT("UI.Icon.Certainty.Predicted"))));
	TestTrue(TEXT("Certainty.Uncertain"), Required.Contains(FName(TEXT("UI.Icon.Certainty.Uncertain"))));

	// Le chiavi nuove sono davvero PRETESE, non solo presenti: un catalogo che copre tutto **tranne**
	// `Certainty` deve avere esattamente quelle tre mancanze.
	//
	// ⚠️ **Qui c'era un'assertion che non poteva fallire**, e la sua storia vale il commento: diceva «un
	// catalogo che copre l'insieme richiesto non ha mancanze», costruendo il catalogo con
	// `MakeCoveringIconCatalog()` — che itera `RequiredIconIds()` — e confrontandolo con
	// `FindMissingRequiredIcons()`, che itera `RequiredIconIds()`. Vero per costruzione, qualunque cosa
	// facesse la derivazione. E' lo stesso difetto che questo checkpoint denuncia, lasciato dentro il test
	// che lo denuncia; l'ha trovato la code review. La forma sotto invece cade in due modi: se `Certainty`
	// sparisce dall'insieme richiesto le mancanze diventano 0, se qualcuno ne aggiunge una quarta diventano 4.
	URTIconCatalogData* WithoutCertainty = NewObject<URTIconCatalogData>();
	WithoutCertainty->MissingIcon = MakeIconAssetRef(TEXT("Missing"));
	for (const FName& Id : Required)
	{
		if (!Id.ToString().StartsWith(TEXT("UI.Icon.Certainty.")))
		{
			WithoutCertainty->Icons.Add(FRTIconDef(Id, IconCategoryFromId(Id), MakeIconAssetRef(Id)));
		}
	}

	const TArray<FName> Missing = URTIconLibrary::FindMissingRequiredIcons(WithoutCertainty);
	TestEqual(TEXT("togliere Certainty dal catalogo produce esattamente tre mancanze"), Missing.Num(), 3);
	for (const TCHAR* Level : { TEXT("Confirmed"), TEXT("Predicted"), TEXT("Uncertain") })
	{
		TestTrue(*FString::Printf(TEXT("Certainty.%s risulta mancante"), Level),
			Missing.Contains(FName(*FString::Printf(TEXT("UI.Icon.Certainty.%s"), Level))));
	}

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

/**
 * La chiave icona di un'azione porta il NOME dell'abilita', e il ripiego sulla core e' una funzione a parte.
 *
 * 🔑 **Il dock risponde a «quale abilita' e' questa»**, quindi due voci dello stesso kit devono avere chiavi
 * diverse: tradurre verso la core le farebbe collassare sullo stesso disegno. La core resta come RIPIEGO,
 * finche' l'asset proprio non e' disegnato.
 *
 * ⛔ Il difetto che questo test ferma e' silenzioso a suite verde: `MakeIconId` prefissa e basta, quindi
 * `Hero.Muiren.TideGuard` diventerebbe `UI.Icon.Hero.…` — categoria che `D-031` non dichiara. E' cosi' che
 * e' arrivato in PIE (`#2963`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionIconIdKeepsTheAbilityNameTest,
	"RefactorTactics.Icons.ActionIconIdKeepsTheAbilityName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTActionIconIdKeepsTheAbilityNameTest::RunTest(const FString&)
{
	// ── Un'azione gia' in una categoria dichiarata non si tocca.
	{
		FRTActionDef Def;
		Def.ActionId = FName(TEXT("Action.Move"));
		TestEqual(TEXT("un'azione core attraversa entrambe le tassonomie e non si traduce"),
			URTIconLibrary::MakeActionIconId(Def.ActionId), FName(TEXT("UI.Icon.Action.Move")));
	}

	// ── `Reaction` E' fra le dodici di D-031: legittima anche se la v0.1 non la disegna.
	{
		FRTActionDef Def;
		Def.ActionId = FName(TEXT("Reaction.HazardEscape"));
		TestEqual(TEXT("Reaction e' dichiarata: nessuna traduzione"),
			URTIconLibrary::MakeActionIconId(Def.ActionId), FName(TEXT("UI.Icon.Reaction.HazardEscape")));
	}

	// ── 🔑 Il cuore: il PERCORSO sopravvive intero, il prefisso `Action.` lo qualifica.
	//
	// 🔴 Questo asserto diceva `UI.Icon.Action.Overload` — il solo nome — e passava. Passava contro una
	// chiave che **nessun asset porta**: il generatore disegna `Action.Hero.Aevik.Overload`, con dentro il
	// sigillo di Aevik, e il glifo restava irraggiungibile (`#2963`). Un test che pinna la regola sbagliata
	// non e' piu' debole di uno assente: e' peggio, perche' difende il difetto.
	{
		FRTActionDef Def;
		Def.ActionId = FName(TEXT("Hero.Aevik.Overload"));
		TestEqual(TEXT("un'abilita' d'eroe tiene eroe E nome sotto Action"),
			URTIconLibrary::MakeActionIconId(Def.ActionId), FName(TEXT("UI.Icon.Action.Hero.Aevik.Overload")));

		FRTActionDef Gadget;
		Gadget.ActionId = FName(TEXT("Gadget.Sprinkler"));
		TestEqual(TEXT("e vale anche per un gadget"),
			URTIconLibrary::MakeActionIconId(Gadget.ActionId), FName(TEXT("UI.Icon.Action.Gadget.Sprinkler")));
	}

	// ── ⚠️ **L'eroe e' la parte che distingue, e senza di lui due eroi collasserebbero.** Oggi nessuna
	// coppia del roster condivide il nome d'abilita', quindi la regola vecchia non sbagliava per collisione:
	// sbagliava perche' indirizzava un asset inesistente. Ma niente impone quell'unicita' — `ValidateHeroes`
	// controlla `HeroId` duplicato, non i nomi d'abilita' — e il giorno che due kit avessero entrambi un
	// `Overload`, la chiave corta li farebbe collassare in silenzio.
	{
		FRTActionDef A; FRTActionDef B;
		A.ActionId = FName(TEXT("Hero.Aevik.Overload"));
		B.ActionId = FName(TEXT("Hero.Branth.Overload"));
		TestNotEqual(TEXT("lo stesso nome su due eroi resta due chiavi"),
			URTIconLibrary::MakeActionIconId(A.ActionId), URTIconLibrary::MakeActionIconId(B.ActionId));
	}

	// ── ⚠️ Il controllo che rende il test NON vacuo: due abilita' dello stesso kit che derivano dalla STESSA
	// core restano distinte. Se la traduzione andasse alla core, questo asserto cadrebbe — ed e' il difetto
	// che la decisione del 2026-09-11 esiste per evitare.
	{
		FRTActionDef A; FRTActionDef B;
		A.ActionId = FName(TEXT("Hero.Muiren.TideGuard"));
		A.DerivedFromActionId = FName(TEXT("Action.Shield"));
		B.ActionId = FName(TEXT("Hero.Muiren.SecondoScudo"));
		B.DerivedFromActionId = FName(TEXT("Action.Shield"));
		TestNotEqual(TEXT("due abilita' con la STESSA core hanno chiavi preferite DIVERSE"),
			URTIconLibrary::MakeActionIconId(A.ActionId), URTIconLibrary::MakeActionIconId(B.ActionId));
		TestEqual(TEXT("ma lo stesso ripiego, che e' il punto del ripiego"),
			URTIconLibrary::MakeActionIconFallbackId(A), URTIconLibrary::MakeActionIconFallbackId(B));
	}

	// ── Il ripiego: `DerivedFromActionId` vince su `BaseActionId`, e si vede solo se DIVERGONO.
	{
		FRTActionDef Def;
		Def.ActionId = FName(TEXT("Hero.Test.Entrambi"));
		Def.BaseActionId = FName(TEXT("Action.BasicAttack"));
		Def.DerivedFromActionId = FName(TEXT("Action.Charge"));
		TestEqual(TEXT("il ripiego segue cio' che l'azione FA, non di che e' profilo"),
			URTIconLibrary::MakeActionIconFallbackId(Def), FName(TEXT("UI.Icon.Action.Charge")));
	}

	// ── ⛔ Un'abilita' PROPRIA non ha ripiego: l'asset va disegnato, e il vuoto resta visibile.
	{
		FRTActionDef Def;
		Def.ActionId = FName(TEXT("Hero.Aevik.Overload"));
		TestEqual(TEXT("nessun ripiego per un'abilita' propria"),
			URTIconLibrary::MakeActionIconFallbackId(Def), FName(NAME_None));
	}

	// ── Le categorie non si inventano.
	{
		TestFalse(TEXT("Hero non e' una delle dodici"),
			URTIconLibrary::IsDeclaredIconCategory(FName(TEXT("Hero.Aevik.Overload"))));
		TestTrue(TEXT("Action si'"), URTIconLibrary::IsDeclaredIconCategory(FName(TEXT("Action.Move"))));
		TestTrue(TEXT("Reaction si'"), URTIconLibrary::IsDeclaredIconCategory(FName(TEXT("Reaction.Counter"))));
		TestFalse(TEXT("un id senza punto non ha categoria"),
			URTIconLibrary::IsDeclaredIconCategory(FName(TEXT("Move"))));
	}

	return true;
}

/**
 * Ogni abilita' del roster o trova il proprio glifo nel catalogo REALE, o dichiara un ripiego.
 *
 * 🔑 **E' il gate che tiene insieme due produttori che non si parlano.** La chiave la compone
 * `MakeActionIconId` in C++; l'asset lo disegna `generate_hud_assets.py` in Python. Nessuno dei due
 * legge l'altro, e per questo sono divergiuti in silenzio (`#2963`): il generatore scriveva
 * `Action.Hero.Aevik.Overload`, il dock chiedeva `Action.Overload`, e venti glifi disegnati erano
 * irraggiungibili mentre ogni test unitario restava verde.
 *
 * ⚠️ **Non confronta due stringhe scritte a mano** — sarebbe la terza copia della stessa regola. Passa
 * dagli ASSET: se il glifo che il generatore ha prodotto non risponde alla chiave che il C++ compone,
 * questo test se ne accorge, qualunque delle due parti si sia mossa.
 *
 * ⛔ **Un'abilita' senza glifo NON e' un errore qui**: `TideGuard`, `MortarShot` e `PhaseGuard` derivano
 * da una core e mostrano la sua icona finche' la propria non e' disegnata. E' il ripiego che fa il suo
 * mestiere. Cio' che questo gate vieta e' il terzo caso: ne' glifo proprio, ne' ripiego — un'icona di
 * `MissingIcon` a schermo, che e' il difetto che ha aperto la issue.
 *
 * ⚠️ **Salta se il catalogo non e' caricabile**, come `RealCatalogCoversRequiredIds`: senza `Content/`
 * montato l'assenza dell'asset non e' un difetto di chiavi. La riga di premessa distingue i due verdi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroAbilityIconIdsReachTheirAssetTest,
	"RefactorTactics.IconCatalog.HeroAbilityIconIdsReachTheirAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroAbilityIconIdsReachTheirAssetTest::RunTest(const FString&)
{
	const URTIconCatalogData* Catalog = LoadObject<URTIconCatalogData>(
		nullptr, TEXT("/Game/RT/UI/DA_IconCatalog.DA_IconCatalog"));

	if (Catalog == nullptr)
	{
		AddWarning(TEXT("`/Game/RT/UI/DA_IconCatalog` non caricabile: chiavi d'eroe NON misurate"));
		return true;
	}

	int32 Esaminate = 0;
	int32 ConGlifoProprio = 0;

	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (Hero == nullptr)
		{
			continue;
		}

		for (const URTActionData* Action : Hero->Actions)
		{
			if (Action == nullptr || Action->Def.ActionId.IsNone())
			{
				continue;
			}

			++Esaminate;
			const FName Preferita = URTIconLibrary::MakeActionIconId(Action->Def.ActionId);

			if (URTIconLibrary::CatalogHasIcon(Catalog, Preferita))
			{
				++ConGlifoProprio;
				continue;
			}

			// Nessun glifo proprio: allora il ripiego deve esistere ED essere nel catalogo, o a schermo
			// comparirebbe `MissingIcon`.
			const FName Ripiego = URTIconLibrary::MakeActionIconFallbackId(Action->Def);
			if (Ripiego.IsNone())
			{
				AddError(FString::Printf(
					TEXT("`%s`: nessun glifo per `%s` e nessun ripiego dichiarato"),
					*Action->Def.ActionId.ToString(), *Preferita.ToString()));
			}
			else if (!URTIconLibrary::CatalogHasIcon(Catalog, Ripiego))
			{
				AddError(FString::Printf(
					TEXT("`%s`: nessun glifo per `%s`, e il ripiego `%s` non e' nel catalogo"),
					*Action->Def.ActionId.ToString(), *Preferita.ToString(), *Ripiego.ToString()));
			}
		}
	}

	// ── Anti-vacuita', due volte, perche' questo gate ha due modi di essere verde per niente.
	//
	// ⚠️ Un roster vuoto non esaminerebbe nulla e passerebbe. E un roster i cui glifi propri fossero TUTTI
	// spariti passerebbe lo stesso, per solo ripiego: sarebbe la regressione di `#2963` di nuovo, con ogni
	// abilita' che mostra l'icona della sua core. La soglia e' deliberatamente bassa — dice «il canale del
	// glifo proprio funziona», non «quante ne sono disegnate», che e' un totale e invecchia da solo.
	TestTrue(TEXT("anti-vacuita': il roster dichiara abilita'"), Esaminate > 0);
	TestTrue(TEXT("anti-vacuita': almeno un'abilita' raggiunge il PROPRIO glifo, non solo il ripiego"),
		ConGlifoProprio > 0);

	AddInfo(FString::Printf(TEXT("%d abilita' esaminate, %d con glifo proprio"),
		Esaminate, ConGlifoProprio));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
