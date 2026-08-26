#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexMapCustomVersion.h"
#include "Map/RTStructureIdentityLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Identita' stabile delle strutture (CP 23.3, #832).
 *
 * `DoorId` distingue i gruppi DENTRO un asset: e' un indice, e chi rieditasse la mappa potrebbe
 * riassegnarlo. Un nome invece sopravvive a salvataggio, ricarica e cottura, ed e' cio' che permette a uno
 * scenario di citare una struttura e a un replay di risolverla. Vedi #832.
 *
 * Il nome vive sul BORDO e sull'ARCO, accanto a `DoorId`: i bordi che lo condividono sono la stessa
 * struttura, con la stessa forma con cui `DoorId` gia' definisce il gruppo.
 */
namespace
{
	/** Esagono pieno di raggio N sul layer 0. Nome prefissato per dominio: unity build (vedi gotcha #8). */
	URTHexMapAsset* MakeIdentityMap(int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		return M;
	}

	/** Porta con nome pubblico sul bordo indicato. `DoorId` resta l'indice interno del gruppo. */
	void PutIdentityDoor(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge,
		int32 DoorId, FName StableId)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		FRTHexDoor Door(Edge, ERTHexDoorState::Closed, DoorId);
		Door.StableId = StableId;
		Data.Doors.Add(Door);
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}
}

/**
 * Il nome risolve il GRUPPO, non il singolo bordo.
 *
 * E' la proprieta' che rende il nome utile a #833: «apri `D1`» deve muovere l'intero portone, esattamente
 * come `SetDoorState` fa oggi partendo da un bordo qualsiasi del gruppo. Se risolvesse un bordo solo, il
 * bersaglio di un interaction graph sarebbe meta' porta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStructureIdentityResolvesGroupTest,
	"RefactorTactics.Structures.Identity.ResolvesDoorGroupByStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStructureIdentityResolvesGroupTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeIdentityMap(2);

	// Un portone di due bordi su celle diverse, come `Structures.Door.GroupClosesTogether`.
	PutIdentityDoor(Map, FRTCellId(0, 0), ERTHexDirection::E, /*DoorId*/ 7, TEXT("D1"));
	PutIdentityDoor(Map, FRTCellId(0, 1), ERTHexDirection::E, /*DoorId*/ 7, TEXT("D1"));

	// Una porta estranea, con nome e gruppo propri: la risoluzione non deve rastrellare la mappa.
	PutIdentityDoor(Map, FRTCellId(1, 0), ERTHexDirection::W, /*DoorId*/ 9, TEXT("D2"));

	const TArray<FRTStructureEdgeRef> Group =
		URTStructureIdentityLibrary::FindDoorEdges(Map, TEXT("D1"));

	if (!TestEqual(TEXT("il nome risolve i due bordi del portone"), Group.Num(), 2))
	{
		return false;
	}

	// L'ordine e' quello canonico delle celle (`SortCells`), non quello di inserimento: un bersaglio
	// risolto due volte deve dare la stessa sequenza, o l'ordine di applicazione di #833 non sarebbe
	// deterministico (invariante n. 3).
	TestEqual(TEXT("primo bordo: la cella"), Group[0].Cell, FRTCellId(0, 0));
	TestEqual(TEXT("primo bordo: la direzione"), Group[0].Edge, ERTHexDirection::E);
	TestEqual(TEXT("secondo bordo: la cella"), Group[1].Cell, FRTCellId(0, 1));

	// La porta estranea resta fuori.
	const TArray<FRTStructureEdgeRef> Other =
		URTStructureIdentityLibrary::FindDoorEdges(Map, TEXT("D2"));
	TestEqual(TEXT("l'altro nome risolve la propria porta, e una sola"), Other.Num(), 1);

	// Un nome che nessuno porta non risolve nulla: e' il caso su cui poggia la validazione dei
	// riferimenti pendenti.
	const TArray<FRTStructureEdgeRef> Missing =
		URTStructureIdentityLibrary::FindDoorEdges(Map, TEXT("D404"));
	TestEqual(TEXT("un nome inesistente non risolve nulla"), Missing.Num(), 0);

	return true;
}

/**
 * Il nome sopravvive a un round-trip di SERIALIZZAZIONE, non solo a una dichiarazione in memoria.
 *
 * E' la distinzione che #687 ha gia' fatto pagare al progetto: tutti i test di migrazione facevano
 * `NewObject` -> impostavano i campi -> verificavano la funzione, e la serializzazione restava in un punto
 * cieco durato otto versioni di formato. Un nome che non viaggia nei byte non e' un'identita' stabile: e'
 * una property che l'editor mostra.
 *
 * ⚠️ Questo test verifica un comportamento che UE fornisce (la serializzazione di un `UPROPERTY`), non
 * codice scritto qui: e' un GATE DI REGRESSIONE, e come tale vale solo se sa diventare rosso. La verifica
 * di mutazione e' dichiarata nella PR — tolto `UPROPERTY` da `StableId`, deve cadere questo e non altri.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStructureIdentitySurvivesReloadTest,
	"RefactorTactics.Structures.Identity.StableIdSurvivesReload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStructureIdentitySurvivesReloadTest::RunTest(const FString&)
{
	URTHexMapAsset* Source = MakeIdentityMap(2);
	PutIdentityDoor(Source, FRTCellId(0, 0), ERTHexDirection::E, /*DoorId*/ 7, TEXT("D1"));
	PutIdentityDoor(Source, FRTCellId(0, 1), ERTHexDirection::E, /*DoorId*/ 7, TEXT("D1"));

	// --- Scrittura ---------------------------------------------------------------------------------
	TArray<uint8> Bytes;
	FCustomVersionContainer WrittenVersions;
	{
		FMemoryWriter Writer(Bytes);
		Source->Serialize(Writer);
		WrittenVersions = Writer.GetCustomVersions();
	}

	// --- Rilettura ---------------------------------------------------------------------------------
	URTHexMapAsset* Reloaded = NewObject<URTHexMapAsset>();
	{
		FMemoryReader Reader(Bytes);
		Reader.SetCustomVersions(WrittenVersions);
		Reloaded->Serialize(Reader);
	}

	// Il gruppo si risolve dall'asset RILETTO, con lo stesso nome e la stessa cardinalita'.
	const TArray<FRTStructureEdgeRef> Group =
		URTStructureIdentityLibrary::FindDoorEdges(Reloaded, TEXT("D1"));
	if (!TestEqual(TEXT("dopo il round-trip il nome risolve ancora i due bordi"), Group.Num(), 2))
	{
		return false;
	}
	TestTrue(TEXT("ed e' lo stesso primo bordo"), Group[0] == FRTStructureEdgeRef(FRTCellId(0, 0),
		ERTHexDirection::E));
	TestTrue(TEXT("e lo stesso secondo bordo"), Group[1] == FRTStructureEdgeRef(FRTCellId(0, 1),
		ERTHexDirection::E));

	// `DoorId` resta l'indice interno e non viene sostituito dal nome: sono due anelli diversi, e la
	// issue mette fuori scope rimuoverlo.
	const FRTHexCellData* First = Reloaded->FindCell(FRTCellId(0, 0));
	if (TestNotNull(TEXT("la cella e' sopravvissuta"), First))
	{
		const FRTHexDoor* Door = First->DoorOn(ERTHexDirection::E);
		if (TestNotNull(TEXT("la porta e' sopravvissuta"), Door))
		{
			TestEqual(TEXT("il gruppo interno e' intatto"), Door->DoorId, 7);
			TestEqual(TEXT("e il nome pubblico anche"), Door->StableId, FName(TEXT("D1")));
		}
	}

	// Una porta SENZA nome resta senza nome: il campo nuovo nasce vuoto, non si inventa un'identita'.
	// E' la stessa asserzione che #619 ha dovuto fare per il sovrapprezzo di occupazione.
	PutIdentityDoor(Source, FRTCellId(1, 0), ERTHexDirection::W, /*DoorId*/ INDEX_NONE, NAME_None);
	TArray<uint8> BytesWithAnonymous;
	{
		FMemoryWriter Writer(BytesWithAnonymous);
		Source->Serialize(Writer);
	}
	URTHexMapAsset* WithAnonymous = NewObject<URTHexMapAsset>();
	{
		FMemoryReader Reader(BytesWithAnonymous);
		Reader.SetCustomVersions(WrittenVersions);
		WithAnonymous->Serialize(Reader);
	}
	const FRTHexCellData* Anonymous = WithAnonymous->FindCell(FRTCellId(1, 0));
	if (TestNotNull(TEXT("la porta anonima e' sopravvissuta"), Anonymous))
	{
		const FRTHexDoor* Door = Anonymous->DoorOn(ERTHexDirection::W);
		if (TestNotNull(TEXT("ed e' ancora una porta"), Door))
		{
			TestTrue(TEXT("una porta senza nome non ne guadagna uno ricaricandosi"),
				Door->StableId.IsNone());
		}
	}
	return true;
}

/**
 * Due strutture non possono chiamarsi allo stesso modo — ma i bordi di UNA struttura si', ed e' la
 * distinzione su cui questa validazione vive o muore.
 *
 * Un portone e' gia' oggi piu' bordi che condividono `DoorId`: se la regola fosse «un nome, un bordo»,
 * validare una porta larga diventerebbe impossibile. La regola giusta e' quindi «un nome, un GRUPPO».
 *
 * ⚠️ E il gruppo non e' sempre `DoorId`: una porta singola porta `INDEX_NONE`, che **tutte** le porte
 * singole condividono. Prendere `INDEX_NONE` per un gruppo farebbe passare due porte scollegate con lo
 * stesso nome — cioe' esattamente il difetto che questa issue esiste per chiudere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStructureIdentityDuplicateTest,
	"RefactorTactics.Structures.Identity.DuplicateIdFailsValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStructureIdentityDuplicateTest::RunTest(const FString&)
{
	// 1. Condivisione LEGITTIMA: due bordi dello stesso portone. Deve restare valida.
	URTHexMapAsset* Legit = MakeIdentityMap(2);
	PutIdentityDoor(Legit, FRTCellId(0, 0), ERTHexDirection::E, /*DoorId*/ 7, TEXT("D1"));
	PutIdentityDoor(Legit, FRTCellId(0, 1), ERTHexDirection::E, /*DoorId*/ 7, TEXT("D1"));
	TestEqual(TEXT("i bordi di uno stesso portone condividono il nome senza errori"),
		Legit->ValidateMap().Num(), 0);

	// 2. Collisione fra GRUPPI diversi: stesso nome, `DoorId` diverso.
	URTHexMapAsset* TwoGroups = MakeIdentityMap(2);
	PutIdentityDoor(TwoGroups, FRTCellId(0, 0), ERTHexDirection::E, /*DoorId*/ 7, TEXT("D1"));
	PutIdentityDoor(TwoGroups, FRTCellId(1, 0), ERTHexDirection::W, /*DoorId*/ 9, TEXT("D1"));
	TestTrue(TEXT("due portoni diversi non possono chiamarsi entrambi D1"),
		TwoGroups->ValidateMap().Num() > 0);

	// 3. Il caso che `DoorId` da solo non sa vedere: due porte SINGOLE, entrambe `INDEX_NONE`, stesso nome.
	//    Non sono un gruppo — sono due strutture — e devono collidere.
	URTHexMapAsset* TwoSingles = MakeIdentityMap(2);
	PutIdentityDoor(TwoSingles, FRTCellId(0, 0), ERTHexDirection::E, /*DoorId*/ INDEX_NONE, TEXT("D1"));
	PutIdentityDoor(TwoSingles, FRTCellId(1, 0), ERTHexDirection::W, /*DoorId*/ INDEX_NONE, TEXT("D1"));
	TestTrue(TEXT("due porte singole con lo stesso nome collidono"),
		TwoSingles->ValidateMap().Num() > 0);

	// 4. Una porta singola con nome proprio resta valida: la regola non punisce l'assenza di gruppo.
	URTHexMapAsset* OneSingle = MakeIdentityMap(2);
	PutIdentityDoor(OneSingle, FRTCellId(0, 0), ERTHexDirection::E, /*DoorId*/ INDEX_NONE, TEXT("D1"));
	TestEqual(TEXT("una porta singola con nome proprio e' valida"), OneSingle->ValidateMap().Num(), 0);

	// 5. Il nome vuoto non e' un nome: molte porte anonime non collidono fra loro.
	URTHexMapAsset* Anonymous = MakeIdentityMap(2);
	PutIdentityDoor(Anonymous, FRTCellId(0, 0), ERTHexDirection::E, /*DoorId*/ INDEX_NONE, NAME_None);
	PutIdentityDoor(Anonymous, FRTCellId(1, 0), ERTHexDirection::W, /*DoorId*/ INDEX_NONE, NAME_None);
	TestEqual(TEXT("le porte senza nome non collidono fra loro"), Anonymous->ValidateMap().Num(), 0);

	return true;
}

/**
 * Gli ARCHI hanno lo stesso problema di identita' delle porte, e una difficolta' in piu': non hanno un
 * `DoorId` su cui appoggiare il gruppo. Un `FRTHexEdge` e' identificato dalla coppia `(From, To)` e basta.
 *
 * Il gruppo pero' esiste gia' nella semantica del repository — `UpdateTransitions`: *«bidirezionale sono
 * due archi ma un evento solo, come il portone di CP 9.3»*. Due archi RECIPROCI sono una struttura sola,
 * ed e' l'unica condivisione di nome che va ammessa fra archi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStructureIdentityArcTest,
	"RefactorTactics.Structures.Identity.ArcsCarryStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStructureIdentityArcTest::RunTest(const FString&)
{
	const FRTCellId Low(0, 0, 0);
	const FRTCellId High(0, 0, 1);

	URTHexMapAsset* Map = MakeIdentityMap(2);
	Map->AddOrUpdateCell(FRTHexCellData(High));
	Map->SortCells();

	// Un ponte bidirezionale: due archi reciproci, un nome solo.
	FRTHexEdge Up(Low, High, /*Cost*/ 2, ERTHexTransitionKind::Bridge);
	Up.StableId = TEXT("B1");
	FRTHexEdge Down(High, Low, /*Cost*/ 2, ERTHexTransitionKind::Bridge);
	Down.StableId = TEXT("B1");
	Map->Transitions.Add(Up);
	Map->Transitions.Add(Down);

	const TArray<FRTStructureArcRef> Bridge = URTStructureIdentityLibrary::FindArcs(Map, TEXT("B1"));
	if (!TestEqual(TEXT("il nome risolve i due versi del ponte"), Bridge.Num(), 2))
	{
		return false;
	}
	TestTrue(TEXT("primo verso"), Bridge[0] == FRTStructureArcRef(Low, High));
	TestTrue(TEXT("secondo verso"), Bridge[1] == FRTStructureArcRef(High, Low));

	// Due archi reciproci con lo stesso nome sono UNA struttura: valido.
	TestEqual(TEXT("un ponte bidirezionale con un nome solo e' valido"), Map->ValidateMap().Num(), 0);

	// Un nome non attraversa i domini: una porta e un arco sono due strutture, e non possono chiamarsi
	// allo stesso modo — o `Interact B1` di #833 non saprebbe quale bersaglio ha davanti.
	URTHexMapAsset* Crossed = MakeIdentityMap(2);
	Crossed->AddOrUpdateCell(FRTHexCellData(High));
	Crossed->SortCells();
	PutIdentityDoor(Crossed, FRTCellId(0, 0), ERTHexDirection::E, /*DoorId*/ 7, TEXT("X1"));
	FRTHexEdge Clash(Low, High, /*Cost*/ 2, ERTHexTransitionKind::Stair);
	Clash.StableId = TEXT("X1");
	Crossed->Transitions.Add(Clash);
	TestTrue(TEXT("una porta e un arco non possono chiamarsi entrambi X1"),
		Crossed->ValidateMap().Num() > 0);

	// Due archi NON reciproci con lo stesso nome sono due strutture: collidono.
	URTHexMapAsset* TwoArcs = MakeIdentityMap(2);
	TwoArcs->AddOrUpdateCell(FRTHexCellData(High));
	TwoArcs->SortCells();
	FRTHexEdge A(FRTCellId(0, 0), High, /*Cost*/ 2, ERTHexTransitionKind::Stair);
	A.StableId = TEXT("S1");
	FRTHexEdge B(FRTCellId(1, 0), High, /*Cost*/ 2, ERTHexTransitionKind::Stair);
	B.StableId = TEXT("S1");
	TwoArcs->Transitions.Add(A);
	TwoArcs->Transitions.Add(B);
	TestTrue(TEXT("due scale distinte non possono chiamarsi entrambe S1"),
		TwoArcs->ValidateMap().Num() > 0);

	return true;
}

/**
 * Un riferimento verso il nulla non arriva a runtime: fallisce in validazione.
 *
 * ⚠️ **Dichiarazione onesta di cio' che questo test NON prova.** In #832 nessun dato di produzione cita
 * ancora uno `StableId`: l'unico consumatore previsto e' l'interaction graph di CP 23.4 (#833), e lo
 * scenario e' fuori scope per dichiarazione della issue stessa. Quindi qui la funzione viene esercitata su
 * una lista costruita dal test, non su un referente reale — e' un validator **pronto per #833**, non una
 * catena dichiarato/trasportato/letto gia' chiusa.
 *
 * Serve comunque, e non e' un test vacuo: e' la funzione che #833 chiamera' sui propri binding invece di
 * scriverne una propria, ed e' il punto in cui la regola «un bersaglio fantasma non passa» vive una volta
 * sola. Il giorno in cui #833 la collega, questo test smette di essere l'unico chiamante.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStructureIdentityUnknownRefTest,
	"RefactorTactics.Structures.Identity.UnknownIdFailsValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStructureIdentityUnknownRefTest::RunTest(const FString&)
{
	const FRTCellId Low(0, 0, 0);
	const FRTCellId High(0, 0, 1);

	URTHexMapAsset* Map = MakeIdentityMap(2);
	Map->AddOrUpdateCell(FRTHexCellData(High));
	Map->SortCells();
	PutIdentityDoor(Map, FRTCellId(0, 0), ERTHexDirection::E, /*DoorId*/ 7, TEXT("D1"));
	FRTHexEdge Arc(Low, High, /*Cost*/ 2, ERTHexTransitionKind::Stair);
	Arc.StableId = TEXT("S1");
	Map->Transitions.Add(Arc);

	TestEqual(TEXT("un riferimento a una porta esistente non produce errori"),
		URTStructureIdentityLibrary::ValidateReferences(Map, { FName(TEXT("D1")) }).Num(), 0);

	// La risoluzione non e' solo delle porte: un bersaglio puo' essere un ponte.
	TestEqual(TEXT("un riferimento a un arco esistente non produce errori"),
		URTStructureIdentityLibrary::ValidateReferences(Map, { FName(TEXT("S1")) }).Num(), 0);

	TestTrue(TEXT("un riferimento verso il nulla fallisce in validazione"),
		URTStructureIdentityLibrary::ValidateReferences(Map, { FName(TEXT("D404")) }).Num() > 0);

	// Il nome vuoto non e' un riferimento: chi lo scrive ha lasciato il campo indietro, e va detto invece
	// di risolverlo silenziosamente in «nessun bersaglio».
	TestTrue(TEXT("un riferimento vuoto fallisce in validazione"),
		URTStructureIdentityLibrary::ValidateReferences(Map, { NAME_None }).Num() > 0);

	// Piu' riferimenti insieme: si segnala ogni nome rotto, non solo il primo — chi corregge un asset
	// vuole la lista, non un errore per volta.
	const TArray<FString> Mixed = URTStructureIdentityLibrary::ValidateReferences(Map,
		{ FName(TEXT("D1")), FName(TEXT("D404")), FName(TEXT("S404")) });
	TestEqual(TEXT("due riferimenti rotti su tre producono due errori"), Mixed.Num(), 2);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
