#include "Misc/AutomationTest.h"
#include "Unit/RTAnimCatalogLibrary.h"
#include "Unit/RTAnimCatalogTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Path plausibile e distinto per indice. Non deve esistere sul disco: questi test sono STRUTTURALI. */
	FString AnimCatalogTestPath(int32 Index)
	{
		return FString::Printf(
			TEXT("/Game/FabAsset/Paragon/ParagonGadget/Characters/Heroes/Gadget/Animations/Clip%02d.Clip%02d"),
			Index, Index);
	}

	FRTAnimCatalogEntry MakeAnimCatalogEntry(const TCHAR* Id, int32 PathIndex)
	{
		FRTAnimCatalogEntry Entry;
		Entry.Id = FName(Id);
		Entry.Derived.AssetPath = AnimCatalogTestPath(PathIndex);
		return Entry;
	}

	/** Catalogo valido minimo: tre voci, high-water mark che le domina. */
	FRTAnimCatalog MakeValidAnimCatalog()
	{
		FRTAnimCatalog Catalog;
		Catalog.NextId = 4;
		Catalog.Entries.Add(MakeAnimCatalogEntry(TEXT("AV_0001"), 1));
		Catalog.Entries.Add(MakeAnimCatalogEntry(TEXT("AV_0002"), 2));
		Catalog.Entries.Add(MakeAnimCatalogEntry(TEXT("AV_0003"), 3));
		return Catalog;
	}

	/**
	 * Testo JSON in cui gli ID **non** seguono l'indice e **non** partono da uno.
	 *
	 * 🔑 E' cio' che rende `IdsAreStableAcrossReads` falsificabile: un loader che assegnasse gli ID per
	 * posizione produrrebbe `AV_0001`/`AV_0002` e il confronto cadrebbe. Con ID 1-based e ordinati, lo stesso
	 * test sarebbe vero per costruzione — verde su un difetto.
	 */
	FString AnimCatalogJsonWithScrambledIds()
	{
		return TEXT(R"({
			"formatVersion": 1,
			"nextId": 8,
			"entries": [
				{ "id": "AV_0007", "derived": { "assetPath": "/Game/FabAsset/A.A" }, "authored": { "status": "Candidate", "label": "Heavy", "notes": "scritta a mano" } },
				{ "id": "AV_0003", "derived": { "assetPath": "/Game/FabAsset/B.B" }, "authored": { "status": "Unreviewed" } }
			]
		})");
	}

	/** Vero se almeno una riga contiene il frammento. Le righe del validator devono NOMINARE il colpevole. */
	bool AnyLineContains(const TArray<FString>& Lines, const TCHAR* Fragment)
	{
		for (const FString& Line : Lines)
		{
			if (Line.Contains(Fragment))
			{
				return true;
			}
		}
		return false;
	}
}

// ---------------------------------------------------------------------------------------------------------
// Un AV_ID duplicato fa fallire il validator, con una riga che LO NOMINA
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogDuplicateIdFailsTest,
	"RefactorTactics.Anim.Catalog.DuplicateIdFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogDuplicateIdFailsTest::RunTest(const FString&)
{
	// CONTROLLO POSITIVO. Senza, un validator che restituisse errori SEMPRE passerebbe la meta' che conta di
	// questo test: «rosso quando serve» non significa niente se non e' anche «verde quando serve».
	const FRTAnimCatalog Valid = MakeValidAnimCatalog();
	TestEqual(TEXT("un catalogo sano non produce errori"),
		URTAnimCatalogLibrary::ValidateCatalog(&Valid).Num(), 0);

	FRTAnimCatalog Duplicated = Valid;
	Duplicated.Entries[2].Id = FName(TEXT("AV_0001"));   // stesso ID della prima voce, path diverso

	const TArray<FString> Errors = URTAnimCatalogLibrary::ValidateCatalog(&Duplicated);
	TestTrue(TEXT("un AV_ID duplicato fa fallire il validator"), Errors.Num() > 0);
	TestTrue(TEXT("la riga nomina l'ID colpevole"), AnyLineContains(Errors, TEXT("AV_0001")));
	TestTrue(TEXT("la riga dice che e' un duplicato"), AnyLineContains(Errors, TEXT("duplicato")));

	// Lo stesso ASSET sotto due ID e' l'altra meta' dell'ambiguita': quale voce porti il giudizio umano
	// sarebbe indecidibile, e l'indecidibilita' non e' un giudizio.
	FRTAnimCatalog SamePath = Valid;
	SamePath.Entries[2].Derived.AssetPath = SamePath.Entries[0].Derived.AssetPath;
	const TArray<FString> PathErrors = URTAnimCatalogLibrary::ValidateCatalog(&SamePath);
	TestTrue(TEXT("due ID sullo stesso asset sono un errore"), PathErrors.Num() > 0);
	TestTrue(TEXT("la riga nomina entrambi gli ID"),
		AnyLineContains(PathErrors, TEXT("AV_0001")) && AnyLineContains(PathErrors, TEXT("AV_0003")));

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Due letture dello stesso catalogo danno gli stessi ID — e sono quelli SCRITTI NEL FILE, non l'indice
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogIdsAreStableAcrossReadsTest,
	"RefactorTactics.Anim.Catalog.IdsAreStableAcrossReads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogIdsAreStableAcrossReadsTest::RunTest(const FString&)
{
	const FString Json = AnimCatalogJsonWithScrambledIds();

	FRTAnimCatalog First;
	FRTAnimCatalog Second;
	FString Error;

	TestTrue(TEXT("prima lettura riuscita"), URTAnimCatalogLibrary::LoadFromString(Json, First, Error));
	TestTrue(TEXT("seconda lettura riuscita"), URTAnimCatalogLibrary::LoadFromString(Json, Second, Error));

	if (First.Entries.Num() != 2 || Second.Entries.Num() != 2)
	{
		AddError(TEXT("il catalogo di prova deve avere due voci"));
		return false;
	}

	// 🔴 L'asserto che conta: gli ID sono quelli del FILE, non `AV_0001`/`AV_0002` dedotti dalla posizione.
	// Un loader che coniasse per indice supererebbe «due letture uguali» — due letture sbagliate allo stesso
	// modo sono comunque uguali — e cadrebbe solo qui.
	TestEqual(TEXT("la prima voce conserva l'ID scritto nel file"),
		First.Entries[0].Id, FName(TEXT("AV_0007")));
	TestEqual(TEXT("la seconda voce conserva l'ID scritto nel file"),
		First.Entries[1].Id, FName(TEXT("AV_0003")));

	TestEqual(TEXT("due letture danno lo stesso primo ID"), First.Entries[0].Id, Second.Entries[0].Id);
	TestEqual(TEXT("due letture danno lo stesso secondo ID"), First.Entries[1].Id, Second.Entries[1].Id);
	TestEqual(TEXT("due letture danno lo stesso high-water mark"), First.NextId, Second.NextId);

	// Round-trip: scrivere e rileggere non riordina, non rinumera e non perde il giudizio umano.
	FString Written;
	TestTrue(TEXT("scrittura riuscita"), URTAnimCatalogLibrary::SaveToString(First, Written));

	FRTAnimCatalog Reloaded;
	TestTrue(TEXT("rilettura del testo scritto riuscita"),
		URTAnimCatalogLibrary::LoadFromString(Written, Reloaded, Error));

	if (Reloaded.Entries.Num() == 2)
	{
		TestEqual(TEXT("round-trip: primo ID invariato"), Reloaded.Entries[0].Id, First.Entries[0].Id);
		TestEqual(TEXT("round-trip: secondo ID invariato"), Reloaded.Entries[1].Id, First.Entries[1].Id);
		TestEqual(TEXT("round-trip: l'ordine delle voci non cambia"),
			Reloaded.Entries[0].Derived.AssetPath, First.Entries[0].Derived.AssetPath);
		TestEqual(TEXT("round-trip: il giudizio umano sopravvive"),
			Reloaded.Entries[0].Authored.Status, ERTAnimClipStatus::Candidate);
		TestEqual(TEXT("round-trip: l'etichetta umana sopravvive"),
			Reloaded.Entries[0].Authored.Label, FString(TEXT("Heavy")));
	}
	else
	{
		AddError(TEXT("il round-trip ha perso delle voci"));
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Una clip nuova riceve un ID nuovo, che viene dall'high-water mark
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogNewClipGetsNewIdTest,
	"RefactorTactics.Anim.Catalog.NewClipGetsNewId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogNewClipGetsNewIdTest::RunTest(const FString&)
{
	FRTAnimCatalog Catalog;
	FString Error;
	TestTrue(TEXT("lettura riuscita"),
		URTAnimCatalogLibrary::LoadFromString(AnimCatalogJsonWithScrambledIds(), Catalog, Error));

	const int32 Before = Catalog.Entries.Num();
	const int32 NextBefore = Catalog.NextId;

	const int32 Added = URTAnimCatalogLibrary::AllocateIds(Catalog, { AnimCatalogTestPath(42) });

	TestEqual(TEXT("una sola voce aggiunta"), Added, 1);
	TestEqual(TEXT("il catalogo cresce di una voce"), Catalog.Entries.Num(), Before + 1);

	// L'ID viene da `nextId`, non da `max(esistenti) + 1`: con gli ID `AV_0007`/`AV_0003` del file, un
	// allocatore che guardasse il massimo darebbe `AV_0008` per caso. Qui `nextId` vale 8 e il caso coincide,
	// quindi il difetto lo separa `RemovedIdIsNeverReused` — questo test misura che l'high-water mark AVANZA.
	TestEqual(TEXT("l'ID nuovo viene dall'high-water mark"),
		Catalog.Entries.Last().Id, FName(*URTAnimCatalogLibrary::MakeId(NextBefore)));
	TestEqual(TEXT("l'high-water mark e' avanzato di uno"), Catalog.NextId, NextBefore + 1);

	// L'ID nuovo non collide con nessuno dei precedenti.
	for (int32 i = 0; i < Before; ++i)
	{
		TestNotEqual(TEXT("l'ID nuovo non riusa un ID gia' in uso"),
			Catalog.Entries.Last().Id, Catalog.Entries[i].Id);
	}

	// Il catalogo risultante resta valido: l'high-water mark domina ancora tutto.
	TestEqual(TEXT("il catalogo resta valido dopo l'allocazione"),
		URTAnimCatalogLibrary::ValidateCatalog(&Catalog).Num(), 0);

	// Idempotenza: lo stesso path non produce una seconda voce e non consuma un ID. Senza, ogni passata dello
	// scanner brucerebbe ID e riempirebbe il catalogo di doppioni.
	const int32 NextAfter = Catalog.NextId;
	const int32 AddedAgain = URTAnimCatalogLibrary::AllocateIds(Catalog, { AnimCatalogTestPath(42) });
	TestEqual(TEXT("un path gia' a catalogo non aggiunge nulla"), AddedAgain, 0);
	TestEqual(TEXT("un path gia' a catalogo non consuma un ID"), Catalog.NextId, NextAfter);

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Un ID rimosso non torna MAI: e' il caso che l'high-water mark esiste per coprire
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogRemovedIdIsNeverReusedTest,
	"RefactorTactics.Anim.Catalog.RemovedIdIsNeverReused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogRemovedIdIsNeverReusedTest::RunTest(const FString&)
{
	FRTAnimCatalog Catalog = MakeValidAnimCatalog();   // AV_0001..AV_0003, nextId = 4

	// Si rimuove la voce con l'ID PIU' ALTO: e' l'unico caso in cui `max(esistenti) + 1` e `nextId`
	// divergono, quindi e' l'unico che separa le due implementazioni.
	const FName RemovedId = Catalog.Entries.Last().Id;
	Catalog.Entries.Pop();

	TestEqual(TEXT("l'high-water mark non torna indietro con la rimozione"), Catalog.NextId, 4);
	TestEqual(TEXT("il catalogo rimane valido dopo la rimozione"),
		URTAnimCatalogLibrary::ValidateCatalog(&Catalog).Num(), 0);

	URTAnimCatalogLibrary::AllocateIds(Catalog, { AnimCatalogTestPath(99) });
	const FName NewId = Catalog.Entries.Last().Id;

	// 🔴 L'asserto centrale. Con `max(esistenti) + 1` il massimo e' ora `AV_0002`, quindi la clip nuova
	// riceverebbe `AV_0003` — cioe' l'ID che in un commit precedente significava un'altra clip.
	TestNotEqual(TEXT("l'ID rimosso non viene riassegnato"), NewId, RemovedId);
	TestEqual(TEXT("la clip nuova riceve l'ID successivo all'high-water mark"),
		NewId, FName(TEXT("AV_0004")));

	// L'altra meta' della garanzia: il riciclo resta RAPPRESENTABILE (qualcuno puo' abbassare `nextId` a
	// mano in un file di testo) ma NON VALIDO — il gate lo ferma invece della memoria di chi rilegge il diff.
	FRTAnimCatalog Tampered = MakeValidAnimCatalog();
	Tampered.NextId = 3;                                // <= AV_0003, gia' assegnato
	const TArray<FString> Errors = URTAnimCatalogLibrary::ValidateCatalog(&Tampered);
	TestTrue(TEXT("un nextId che non domina gli ID in uso e' un errore"), Errors.Num() > 0);
	TestTrue(TEXT("la riga nomina l'ID che verrebbe riciclato"), AnyLineContains(Errors, TEXT("AV_0003")));

	// Il confronto e' NUMERICO sul suffisso, non lessicale: lessicalmente "AV_10000" < "AV_9999", e un
	// high-water mark confrontato per stringa comincerebbe a riciclare al superamento delle quattro cifre.
	FRTAnimCatalog Wide;
	Wide.NextId = 10000;
	Wide.Entries.Add(MakeAnimCatalogEntry(TEXT("AV_9999"), 1));
	TestEqual(TEXT("10000 domina 9999 anche se lessicalmente sembra di no"),
		URTAnimCatalogLibrary::ValidateCatalog(&Wide).Num(), 0);

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Un catalogo vuoto NON e' verde: e' la mancanza totale
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogEmptyIsNotCoverageTest,
	"RefactorTactics.Anim.Catalog.EmptyCatalogIsNotCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogEmptyIsNotCoverageTest::RunTest(const FString&)
{
	// Catalogo ASSENTE.
	TestTrue(TEXT("un catalogo assente non e' valido"),
		URTAnimCatalogLibrary::ValidateCatalog(nullptr).Num() > 0);

	// Catalogo PRESENTE ma SENZA VOCI. E' il caso che conta: e' lo stato del progetto prima di questa issue —
	// 85 clip per un solo eroe e nessun dato che dica quale sia stata guardata.
	const FRTAnimCatalog Empty;
	const TArray<FString> Errors = URTAnimCatalogLibrary::ValidateCatalog(&Empty);
	TestTrue(TEXT("un catalogo senza voci non e' valido"), Errors.Num() > 0);
	TestTrue(TEXT("la riga dice che il catalogo e' vuoto"), AnyLineContains(Errors, TEXT("vuoto")));

	// «Leggibile» e «valido» sono due domande diverse: un file con `entries: []` si LEGGE senza errori, ed e'
	// il validator — non il parser — a rifiutarlo. Confonderle nasconderebbe il caso peggiore dietro un
	// errore di sintassi che non c'e'.
	FRTAnimCatalog Parsed;
	FString Error;
	const bool bParsed = URTAnimCatalogLibrary::LoadFromString(
		TEXT(R"({ "formatVersion": 1, "nextId": 1, "entries": [] })"), Parsed, Error);
	TestTrue(TEXT("un catalogo vuoto e' LEGGIBILE"), bParsed);
	TestTrue(TEXT("...e comunque NON valido"), URTAnimCatalogLibrary::ValidateCatalog(&Parsed).Num() > 0);

	// CONTROLLO POSITIVO: senza, un validator che gridasse sempre passerebbe questo test senza misurare nulla.
	const FRTAnimCatalog Valid = MakeValidAnimCatalog();
	TestEqual(TEXT("un catalogo con voci sane e' valido"),
		URTAnimCatalogLibrary::ValidateCatalog(&Valid).Num(), 0);

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Nessun percorso automatico raggiunge `Promoted`, e nessuno declassa un giudizio gia' dato
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogAutomationNeverPromotesTest,
	"RefactorTactics.Anim.Catalog.AutomationNeverPromotes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogAutomationNeverPromotesTest::RunTest(const FString&)
{
	// `AllocateIds` e' l'UNICO ingresso automatico al catalogo. Tutto cio' che conia esce `Unreviewed`.
	FRTAnimCatalog Catalog;
	Catalog.NextId = 1;
	URTAnimCatalogLibrary::AllocateIds(Catalog,
		{ AnimCatalogTestPath(1), AnimCatalogTestPath(2), AnimCatalogTestPath(3) });

	TestEqual(TEXT("tre voci coniate"), Catalog.Entries.Num(), 3);
	for (const FRTAnimCatalogEntry& Entry : Catalog.Entries)
	{
		TestEqual(TEXT("una voce coniata automaticamente e' Unreviewed"),
			Entry.Authored.Status, ERTAnimClipStatus::Unreviewed);
		TestTrue(TEXT("nessuna etichetta artistica viene dedotta dal nome del file"),
			Entry.Authored.Label.IsEmpty());
		TestTrue(TEXT("nemmeno l'assetName viene dedotto dalla stringa del path"),
			Entry.Derived.AssetName.IsEmpty());
	}

	// Il default della struct e' `Unreviewed`, non `Promoted`: una voce dimenticata non si promuove da sola.
	const FRTAnimClipAuthored Fresh;
	TestEqual(TEXT("il default di Authored e' Unreviewed"), Fresh.Status, ERTAnimClipStatus::Unreviewed);

	// Un giudizio umano gia' dato sopravvive a un'allocazione successiva: `AllocateIds` non tocca `Authored`.
	Catalog.Entries[0].Authored.Status = ERTAnimClipStatus::Promoted;
	Catalog.Entries[0].Authored.Notes = TEXT("scelta da una persona");
	URTAnimCatalogLibrary::AllocateIds(Catalog, { AnimCatalogTestPath(4) });

	TestEqual(TEXT("una promozione umana sopravvive a una nuova allocazione"),
		Catalog.Entries[0].Authored.Status, ERTAnimClipStatus::Promoted);
	TestEqual(TEXT("la motivazione umana non viene toccata"),
		Catalog.Entries[0].Authored.Notes, FString(TEXT("scelta da una persona")));
	TestEqual(TEXT("la voce appena coniata resta Unreviewed"),
		Catalog.Entries.Last().Authored.Status, ERTAnimClipStatus::Unreviewed);

	// Uno `Status` sconosciuto NON ricade su `Unreviewed`: ricadere declasserebbe in silenzio un `Promoted`
	// scritto da una build piu' nuova, cioe' cancellerebbe un giudizio umano per un errore di battitura.
	FRTAnimCatalog Bogus;
	FString Error;
	const bool bLoaded = URTAnimCatalogLibrary::LoadFromString(
		TEXT(R"({ "formatVersion": 1, "nextId": 2, "entries": [ { "id": "AV_0001", "derived": { "assetPath": "/Game/A.A" }, "authored": { "status": "Approvato" } } ] })"),
		Bogus, Error);
	TestFalse(TEXT("uno status sconosciuto fa fallire la lettura"), bLoaded);
	TestTrue(TEXT("l'errore nomina lo status colpevole"), Error.Contains(TEXT("Approvato")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
