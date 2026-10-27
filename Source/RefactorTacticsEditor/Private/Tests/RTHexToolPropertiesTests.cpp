#include "Tools/RTHexPaintTool.h"
#include "Misc/AutomationTest.h"

// `UInteractiveToolPropertySet` vive in `InteractiveTool.h`, non in un header col proprio nome: quello non
// esiste, e il primo tentativo l'ha inventato. Verificato con `grep -l` sui Public/ del modulo engine.
#include "InteractiveTool.h"
#include "UObject/UObjectIterator.h"

#include "Map/RTHexCellData.h" // ERTHexSurface
#include "Map/RTHexLibrary.h"    // SurfaceVolumeFor: il fumo e' l'unica superficie con un volume
#include "Terrain/RTTerrainLibrary.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTMapVisuals.h"            // RTCellPrismRadius: la scala Z del corpo si legge contro la mesh
#include "Map/RTStructuralBodyLibrary.h" // il corpo lo calcola il derivatore, il test non lo ricalcola

#if WITH_DEV_AUTOMATION_TESTS

/**
 * I PRIMI test del modulo editor, e la ragione per cui esistono.
 *
 * Tre issue di fila (#871, #921, #931) hanno dichiarato «`RefactorTacticsEditor/` non ha test, quindi la
 * verifica e' manuale» trattandolo come un dato di fatto. Non lo era: il `Build.cs` ha gia' `Core` — dove
 * vive `Misc/AutomationTest.h` — e il modulo runtime, e `WITH_DEV_AUTOMATION_TESTS` e' definito sui target
 * Editor. I test non erano impossibili: non erano stati scritti.
 *
 * Quello che segue e' il difetto #871 promosso a invariante. Il pennello Fill scriveva una regione intera
 * con un `MoveCost` che non corrispondeva alla superficie, in silenzio, perche' gli mancava il
 * `PostEditChangeProperty` che il Paint aveva. E' stato trovato davanti all'editor, per caso, durante una
 * verifica che riguardava altro.
 */
namespace
{
	/** `Rough` costa 2 nel catalogo: e' il valore che un pennello corretto deve scrivere da solo. */
	constexpr ERTHexSurface ProbeSurface = ERTHexSurface::Rough;

	/**
	 * Un property set e' un PENNELLO se ha `Surface` e `MoveCost` **entrambi editabili**.
	 *
	 * La distinzione non e' cosmetica: `URTHexSelectToolProperties` ha gli stessi due campi ma marcati
	 * `VisibleAnywhere` — sono un readout della cella selezionata, non un pennello, e pretendere che si
	 * auto-aggiornino sarebbe sbagliato.
	 *
	 * 🔴 **`CPF_Edit` da solo NON separa i due casi**, ed e' l'errore che la prima stesura di questo test ha
	 * commesso: entrambe le macro lo impostano. Quella che discrimina e' `CPF_EditConst`, che
	 * `VisibleAnywhere` aggiunge e `EditAnywhere` no:
	 *
	 *     EditAnywhere      CPF_Edit
	 *     VisibleAnywhere   CPF_Edit | CPF_EditConst
	 *
	 * Il test e' nato rosso proprio qui, su tutti e due i casi, e la semantica del flag era stata dedotta
	 * dal nome della macro invece che verificata.
	 */
	bool EditabileDavvero(const FProperty* P)
	{
		return P->HasAnyPropertyFlags(CPF_Edit) && !P->HasAnyPropertyFlags(CPF_EditConst);
	}

	bool IsBrushLike(const UClass* Class, FProperty*& OutSurface, FProperty*& OutMoveCost)
	{
		OutSurface = Class->FindPropertyByName(TEXT("Surface"));
		OutMoveCost = Class->FindPropertyByName(TEXT("MoveCost"));
		if (!OutSurface || !OutMoveCost)
		{
			return false;
		}
		return EditabileDavvero(OutSurface) && EditabileDavvero(OutMoveCost);
	}
}

/**
 * Ogni pennello del mode Hex Map allinea `MoveCost` al catalogo quando cambia `Surface`.
 *
 * L'elenco dei pennelli NON e' trascritto: si itera la reflection su tutte le sottoclassi di
 * `UInteractiveToolPropertySet`, cosi' un tool aggiunto domani entra nel test da solo. Una lista scritta a
 * mano resterebbe verde mentre il tool nuovo nasce col difetto — che e' esattamente come #871 e' arrivato
 * fino all'editor.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBrushMoveCostFollowsSurfaceTest,
	"RefactorTactics.HexEditor.BrushMoveCostFollowsSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBrushMoveCostFollowsSurfaceTest::RunTest(const FString&)
{
	const FRTTerrainDef Def = URTTerrainLibrary::FindTerrainDef(ProbeSurface);
	TestTrue(TEXT("il catalogo da' a Rough un costo diverso dal default 1"), Def.MoveCost != 1);

	int32 Esaminati = 0;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class->IsChildOf(UInteractiveToolPropertySet::StaticClass())
			|| Class->HasAnyClassFlags(CLASS_Abstract)
			|| !Class->GetName().StartsWith(TEXT("RTHex")))
		{
			continue;
		}

		FProperty* SurfaceProp = nullptr;
		FProperty* MoveCostProp = nullptr;
		if (!IsBrushLike(Class, SurfaceProp, MoveCostProp))
		{
			continue; // readout o tool senza pennello: non deve auto-aggiornare nulla
		}

		++Esaminati;
		const FString Nome = Class->GetName();

		UObject* Props = NewObject<UObject>(GetTransientPackage(), Class);
		if (!TestNotNull(*FString::Printf(TEXT("%s: istanza creata"), *Nome), Props))
		{
			continue;
		}

		// Si scrive la superficie e si notifica come farebbe il pannello dei dettagli. Il costo di partenza
		// e' forzato a un valore SBAGLIATO e diverso dal default, cosi' un property set che non fa nulla
		// non puo' passare per coincidenza.
		MoveCostProp->ContainerPtrToValuePtr<int32>(Props)[0] = 99;
		SurfaceProp->ContainerPtrToValuePtr<uint8>(Props)[0] = static_cast<uint8>(ProbeSurface);

		FPropertyChangedEvent Evento(SurfaceProp);
		Props->PostEditChangeProperty(Evento);

		const int32 Ottenuto = MoveCostProp->ContainerPtrToValuePtr<int32>(Props)[0];
		TestEqual(*FString::Printf(
			TEXT("%s: cambiando Surface a Rough, MoveCost segue il catalogo"), *Nome),
			Ottenuto, Def.MoveCost);
	}

	// Se il filtro smettesse di riconoscere i pennelli, il ciclo girerebbe a vuoto e il test passerebbe
	// senza aver verificato niente: e' il modo in cui un test smette di dire qualcosa in silenzio.
	TestTrue(TEXT("almeno due pennelli esaminati (Paint e Fill)"), Esaminati >= 2);
	return true;
}

/**
 * Il pennello NON blocca il movimento quando allinea il costo — la **seconda meta'** del primo criterio di
 * #871, che il test qui sopra non guardava.
 *
 * 🔴 **Non e' pignoleria, ed e' la stessa forma di difetto della issue.** `bBlocksMovement` e' cio' che
 * rende una cella **impassabile**: se l'allineamento del costo lo accendesse per errore, il Fill scriverebbe
 * una regione intera di celle invalicabili — e il pannello mostrerebbe un costo corretto accanto a un blocco
 * che nessuno ha chiesto. Il criterio esiste perche' nessun terreno del catalogo v0.1 blocca il movimento
 * normale: il costo lo esprime il numero, non il flag.
 *
 * ⚠️ Il valore di partenza e' forzato a `true`, cioe' SBAGLIATO: un override che non toccasse il campo
 * passerebbe se il default fosse gia' `false`, e il test non direbbe niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBrushSurfaceDoesNotBlockMovementTest,
	"RefactorTactics.HexEditor.BrushSurfaceDoesNotBlockMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBrushSurfaceDoesNotBlockMovementTest::RunTest(const FString&)
{
	int32 Esaminati = 0;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class->IsChildOf(UInteractiveToolPropertySet::StaticClass())
			|| Class->HasAnyClassFlags(CLASS_Abstract)
			|| !Class->GetName().StartsWith(TEXT("RTHex")))
		{
			continue;
		}

		FProperty* SurfaceProp = nullptr;
		FProperty* MoveCostProp = nullptr;
		if (!IsBrushLike(Class, SurfaceProp, MoveCostProp))
		{
			continue;
		}

		FBoolProperty* BlocksProp = CastField<FBoolProperty>(Class->FindPropertyByName(TEXT("bBlocksMovement")));
		if (!BlocksProp || !EditabileDavvero(BlocksProp))
		{
			continue; // un pennello senza quel campo non ha nulla da spegnere
		}

		++Esaminati;
		const FString Nome = Class->GetName();

		UObject* Props = NewObject<UObject>(GetTransientPackage(), Class);
		if (!TestNotNull(*FString::Printf(TEXT("%s: istanza creata"), *Nome), Props))
		{
			continue;
		}

		BlocksProp->SetPropertyValue_InContainer(Props, true); // deliberatamente sbagliato
		SurfaceProp->ContainerPtrToValuePtr<uint8>(Props)[0] = static_cast<uint8>(ProbeSurface);

		FPropertyChangedEvent Evento(SurfaceProp);
		Props->PostEditChangeProperty(Evento);

		TestFalse(*FString::Printf(
			TEXT("%s: allineando il costo, bBlocksMovement resta spento"), *Nome),
			BlocksProp->GetPropertyValue_InContainer(Props));
	}

	TestTrue(TEXT("almeno due pennelli esaminati"), Esaminati >= 2);
	return true;
}

/**
 * L'override scatta sul cambio di `Surface`, **non a ogni edit del pannello** — il terzo criterio di #871,
 * ed e' quello che si dimentica.
 *
 * 🔴 **Il sintomo che questo test previene e' l'OPPOSTO di quello della issue, ed e' peggiore.** #871
 * riportava un `MoveCost` che *non* si aggiornava; un override scritto senza la guardia sul nome della
 * property lo riscriverebbe a **ogni** notifica del pannello dei dettagli — e allora il campo diventerebbe
 * **non modificabile a mano**: l'autore scrive `5`, tocca qualunque altro campo, e ritrova `2`. Un pennello
 * che rifiuta il valore d'autore e' un difetto piu' grave di uno che non lo suggerisce.
 *
 * 🔑 **La notifica di controllo e' su un'ALTRA property**, non su `Surface`: e' l'unico modo di distinguere
 * «reagisce al cambio di superficie» da «reagisce a tutto». Un test che notificasse di nuovo `Surface`
 * misurerebbe la stessa cosa del test precedente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBrushKeepsHandWrittenMoveCostTest,
	"RefactorTactics.HexEditor.BrushKeepsHandWrittenMoveCost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBrushKeepsHandWrittenMoveCostTest::RunTest(const FString&)
{
	constexpr int32 ScrittoAMano = 5;

	int32 Esaminati = 0;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class->IsChildOf(UInteractiveToolPropertySet::StaticClass())
			|| Class->HasAnyClassFlags(CLASS_Abstract)
			|| !Class->GetName().StartsWith(TEXT("RTHex")))
		{
			continue;
		}

		FProperty* SurfaceProp = nullptr;
		FProperty* MoveCostProp = nullptr;
		if (!IsBrushLike(Class, SurfaceProp, MoveCostProp))
		{
			continue;
		}

		// Serve una property DIVERSA da `Surface` su cui notificare. `bBlocksMovement` c'e' in entrambi i
		// pennelli; se un domani non ci fosse, il pennello esce dal conteggio invece di falsare il test.
		FProperty* AltraProp = Class->FindPropertyByName(TEXT("bBlocksMovement"));
		if (!AltraProp || !EditabileDavvero(AltraProp))
		{
			continue;
		}

		++Esaminati;
		const FString Nome = Class->GetName();

		UObject* Props = NewObject<UObject>(GetTransientPackage(), Class);
		if (!TestNotNull(*FString::Printf(TEXT("%s: istanza creata"), *Nome), Props))
		{
			continue;
		}

		// 1. L'autore sceglie la superficie: il costo si allinea da solo (gia' pinnato altrove, qui e' il
		//    presupposto del passo 2 e si verifica per non costruire sul vuoto).
		SurfaceProp->ContainerPtrToValuePtr<uint8>(Props)[0] = static_cast<uint8>(ProbeSurface);
		FPropertyChangedEvent CambioSuperficie(SurfaceProp);
		Props->PostEditChangeProperty(CambioSuperficie);

		const FRTTerrainDef Def = URTTerrainLibrary::FindTerrainDef(ProbeSurface);
		TestEqual(*FString::Printf(TEXT("%s: il costo si e' allineato al catalogo"), *Nome),
			MoveCostProp->ContainerPtrToValuePtr<int32>(Props)[0], Def.MoveCost);

		// 2. POI lo corregge a mano, e tocca un altro campo del pannello.
		MoveCostProp->ContainerPtrToValuePtr<int32>(Props)[0] = ScrittoAMano;
		FPropertyChangedEvent CambioAltro(AltraProp);
		Props->PostEditChangeProperty(CambioAltro);

		TestEqual(*FString::Printf(
			TEXT("%s: il costo scritto a mano sopravvive a un edit di un'altra property"), *Nome),
			MoveCostProp->ContainerPtrToValuePtr<int32>(Props)[0], ScrittoAMano);
	}

	TestTrue(TEXT("almeno due pennelli esaminati"), Esaminati >= 2);
	return true;
}

/**
 * I readout NON si auto-aggiornano, ed e' il rovescio della stessa invariante.
 *
 * Senza questo, la correzione di #871 potrebbe essere applicata «per sicurezza» anche a
 * `URTHexSelectToolProperties`, dove sovrascriverebbe il dato letto dalla cella selezionata con un valore
 * di catalogo — un readout che mente invece di uno che tace.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexReadoutDoesNotAutoUpdateTest,
	"RefactorTactics.HexEditor.ReadoutDoesNotAutoUpdate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexReadoutDoesNotAutoUpdateTest::RunTest(const FString&)
{
	UClass* Select = FindObject<UClass>(nullptr, TEXT("/Script/RefactorTacticsEditor.RTHexSelectToolProperties"));
	if (!TestNotNull(TEXT("URTHexSelectToolProperties e' riflessa"), Select))
	{
		return false;
	}

	FProperty* SurfaceProp = Select->FindPropertyByName(TEXT("Surface"));
	FProperty* MoveCostProp = Select->FindPropertyByName(TEXT("MoveCost"));
	if (!TestNotNull(TEXT("Surface esiste"), SurfaceProp) || !TestNotNull(TEXT("MoveCost esiste"), MoveCostProp))
	{
		return false;
	}

	// E' questa la riga che rende il readout un readout, e il test la pinna: se qualcuno le rendesse
	// editabili, il tool di selezione diventerebbe un pennello per sbaglio.
	//
	// ⚠️ Si asserisce `CPF_EditConst`, non l'assenza di `CPF_Edit`: `VisibleAnywhere` imposta ENTRAMBI, e
	// la prima stesura di questo test chiedeva che `CPF_Edit` fosse falso — fallendo su un codice corretto.
	TestTrue(TEXT("Surface del readout e' esposta nei dettagli"), SurfaceProp->HasAnyPropertyFlags(CPF_Edit));
	TestTrue(TEXT("Surface del readout e' di sola lettura"), SurfaceProp->HasAnyPropertyFlags(CPF_EditConst));
	TestTrue(TEXT("MoveCost del readout e' di sola lettura"), MoveCostProp->HasAnyPropertyFlags(CPF_EditConst));
	TestFalse(TEXT("il readout non e' un pennello"), EditabileDavvero(SurfaceProp));
	return true;
}

/**
 * 🔑 **La maschera del pennello esclude cio' che il pennello non scrive, e l'erase non la usa** — `#1865`.
 *
 * Il pennello scrive quattro campi: `Surface`, `MoveCost`, `bBlocksMovement`, `bBlocksLineOfSight`. La
 * matrice campo→famiglia dice che toccano `Cells`, `Glyphs`, `Relief`, `Blockers` — piu' `Borders`, perche'
 * una pennellata puo' creare una cella e ogni cella nuova porta il suo contorno.
 *
 * ⛔ **`EdgeFeatures` resta fuori**, e questa e' la meta' che fa risparmiare: dipende da `Covers`, `Doors`
 * e muri interni, che il pennello non scrive mai.
 *
 * 🔴 **`Bodies` NON resta fuori** (`#2732`): il corpo di una cella dipende da cio' che le sta sotto, e una
 * pennellata puo' creare quella cella. **Nemmeno `SurfaceVolumes`**: il volume lo decide `Surface`, che il
 * pennello scrive. I due test di comportamento che lo dimostrano sono piu' sotto.
 *
 * 🔴 **E l'erase ricostruisce TUTTO**, che e' la riga capace di cadere: cancellare una cella la toglie da
 * ogni famiglia, comprese quelle escluse. Con la maschera ridotta, una cella con coperture lascerebbe
 * pannelli orfani su un pavimento che non c'e' piu'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexPaintToolRebuildMaskTest,
	"RefactorTactics.HexTools.BrushRebuildMaskExcludesWhatTheBrushNeverWrites",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexPaintToolRebuildMaskTest::RunTest(const FString&)
{
	const ERTRebuildFamily Paint = URTHexPaintTool::BrushRebuildMask(/*bPaint=*/ true);

	// Cio' che il pennello scrive, ci deve essere.
	TestTrue(TEXT("il pavimento e' nella maschera"), EnumHasAnyFlags(Paint, ERTRebuildFamily::Cells));
	TestTrue(TEXT("i glifi anche: la superficie decide quanti anelli"),
		EnumHasAnyFlags(Paint, ERTRebuildFamily::Glyphs));
	TestTrue(TEXT("il rilievo anche: il costo decide se esiste"),
		EnumHasAnyFlags(Paint, ERTRebuildFamily::Relief));
	TestTrue(TEXT("i blocchi anche"), EnumHasAnyFlags(Paint, ERTRebuildFamily::Blockers));
	TestTrue(TEXT("e i contorni, perche' una pennellata puo' creare una cella"),
		EnumHasAnyFlags(Paint, ERTRebuildFamily::Borders));

	// 🔑 Cio' che NON scrive, non ci deve essere: e' il risparmio, e senza questa riga la maschera
	// potrebbe essere `All` e il test passerebbe lo stesso.
	TestFalse(TEXT("i pannelli di bordo NO: il pennello non scrive Covers ne' Doors"),
		EnumHasAnyFlags(Paint, ERTRebuildFamily::EdgeFeatures));

	// 🔴 I corpi SI', pur non scrivendo il pennello `BodyFill`: dipendono da cio' che sta sotto (`#2732`).
	TestTrue(TEXT("i corpi strutturali SI': un corpo dipende dalla cella sotto, e il pennello puo' crearla"),
		EnumHasAnyFlags(Paint, ERTRebuildFamily::Bodies));

	// 🔴 E il volume della superficie SI': lo decide `Surface`, che il pennello scrive (`#2732`).
	TestTrue(TEXT("il volume della superficie SI': lo decide Surface, che e' un campo del pennello"),
		EnumHasAnyFlags(Paint, ERTRebuildFamily::SurfaceVolumes));

	// 🔴 L'erase e' l'eccezione, e ricostruisce tutto.
	const ERTRebuildFamily Erase = URTHexPaintTool::BrushRebuildMask(/*bPaint=*/ false);
	TestTrue(TEXT("cancellare ricostruisce anche i pannelli di bordo"),
		EnumHasAnyFlags(Erase, ERTRebuildFamily::EdgeFeatures));
	TestTrue(TEXT("e anche i corpi: la cella sparisce da ogni famiglia"),
		EnumHasAnyFlags(Erase, ERTRebuildFamily::Bodies));
	return true;
}

namespace
{
	/** Un mondo di gioco minimo per i test del pennello, distrutto all'uscita da QUALUNQUE ramo. */
	struct FRTPaintToolTestWorld
	{
		UWorld* World = nullptr;

		FRTPaintToolTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
			if (World && GEngine)
			{
				FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
				Ctx.SetCurrentWorld(World);
			}
		}

		~FRTPaintToolTestWorld()
		{
			if (World)
			{
				if (GEngine) { GEngine->DestroyWorldContext(World); }
				World->DestroyWorld(false);
			}
		}
	};

	/** L'actor con l'asset gia' assegnato, cosi' `OnConstruction` lo vede come al caricamento del livello. */
	ARTHexMapActor* SpawnPaintToolTestActor(UWorld* World, URTHexMapAsset* Asset)
	{
		ARTHexMapActor* Actor = World->SpawnActorDeferred<ARTHexMapActor>(
			ARTHexMapActor::StaticClass(), FTransform::Identity);
		if (Actor)
		{
			Actor->MapAsset = Asset;
			Actor->FinishSpawning(FTransform::Identity);
		}
		return Actor;
	}

	/** L'ISM dell'actor col nome dato, o `nullptr`. I nomi sono `CreateDefaultSubobject` distinti. */
	UInstancedStaticMeshComponent* FindPaintToolIsm(const ARTHexMapActor* Actor, const TCHAR* Name)
	{
		TArray<UInstancedStaticMeshComponent*> Isms;
		Actor->GetComponents(Isms);
		for (UInstancedStaticMeshComponent* Ism : Isms)
		{
			if (Ism && Ism->GetName() == Name) { return Ism; }
		}
		return nullptr;
	}
}

/**
 * 🔴 **Dipingere una cella SOTTO un corpo strutturale lo accorcia, e la maschera del pennello deve
 * saperlo** — `#2732`.
 *
 * `URTStructuralBodyLibrary::DeriveBodies` tronca il corpo di una cella sulla faccia superiore della cella
 * sottostante nella stessa colonna. Il pennello non scrive `BodyFill`, ma puo' CREARE quella cella: la
 * maschera che lo ignorava lasciava il corpo con la vecchia estensione, e lo mostrava attraversare il
 * pavimento appena dipinto per tutta la pennellata.
 *
 * ⚠️ **E' un'incoerenza del trascinamento, non un dato sbagliato**: al rilascio `EndStrokeIfActive`
 * ricostruisce tutto. Una stesura precedente di questo commento diceva «fino a una gomma o a un cambio
 * d'asset», ereditata dal testo della issue, ed era falsa.
 *
 * ⚠️ **Le premesse sono asserite, non assunte**: che prima della pennellata il corpo non sia troncato, e che
 * la cella nuova lo tronchi davvero spostandone la base. Senza, il test passerebbe anche con una maschera
 * che non ricostruisce i corpi, perche' non ci sarebbe niente da aggiornare.
 *
 * 🔑 **Si usa la maschera VERA del pennello**, `URTHexPaintTool::BrushRebuildMask`, e non un elenco copiato
 * qui: e' la riga che cambia il comportamento, e una copia resterebbe verde togliendola.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexPaintToolBrushKeepsBodiesInLineTest,
	"RefactorTactics.HexTools.PaintingBelowABodyShortensItWithTheBrushMask",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexPaintToolBrushKeepsBodiesInLineTest::RunTest(const FString&)
{
	FRTPaintToolTestWorld Mondo;
	if (!TestNotNull(TEXT("mondo di prova"), Mondo.World)) { return false; }

	// Una cella sola, a Layer 1, che dichiara un corpo pieno: sotto di lei la colonna e' vuota.
	URTHexMapAsset* Asset = NewObject<URTHexMapAsset>(GetTransientPackage());
	FRTHexCellData Sopra(FRTCellId(0, 0, 1));
	Sopra.BodyFill = ERTHexBodyFill::Full;
	Asset->AddOrUpdateCell(Sopra);
	Asset->SortCells();

	ARTHexMapActor* Actor = SpawnPaintToolTestActor(Mondo.World, Asset);
	if (!TestNotNull(TEXT("actor mappa"), Actor)) { return false; }

	const TArray<FRTStructuralBody> Prima = URTStructuralBodyLibrary::DeriveBodies(Asset);
	if (!TestEqual(TEXT("premessa: un corpo"), Prima.Num(), 1)
		|| !TestFalse(TEXT("premessa: con la colonna vuota sotto il corpo non e' troncato"), Prima[0].bTruncated))
	{
		return false;
	}

	// Il pennello crea la cella SOTTO il corpo, come fa `ApplyBrushAt` con la superficie di default.
	Asset->BeginStroke();
	const bool bDipinta = Asset->PaintCellInStroke(FRTCellId(0, 0, 0), ERTHexSurface::Floor, /*MoveCost=*/ 1,
		/*bBlocksMovement=*/ false, TOptional<bool>(false));
	Asset->EndStroke();
	if (!TestTrue(TEXT("il pennello crea la cella sotto il corpo"), bDipinta)) { return false; }

	const TArray<FRTStructuralBody> Dopo = URTStructuralBodyLibrary::DeriveBodies(Asset);
	if (!TestEqual(TEXT("il derivatore vede ancora un corpo"), Dopo.Num(), 1)
		|| !TestTrue(TEXT("premessa: la cella nuova TRONCA il corpo"), Dopo[0].bTruncated)
		|| !TestTrue(TEXT("premessa: e gli sposta la base"),
			!FMath::IsNearlyEqual(Dopo[0].BottomZ, Prima[0].BottomZ, 0.5f)))
	{
		return false;
	}

	// La ricostruzione che il pennello fa a ogni cella toccata, con la sua maschera.
	Actor->RebuildInstances(URTHexPaintTool::BrushRebuildMask(/*bPaint=*/ true));

	const UInstancedStaticMeshComponent* Corpi = FindPaintToolIsm(Actor, TEXT("StructuralBodies"));
	if (!TestNotNull(TEXT("l'ISM dei corpi"), Corpi) || !TestEqual(TEXT("un corpo posato"), Corpi->GetInstanceCount(), 1))
	{
		return false;
	}

	FTransform Xf;
	Corpi->GetInstanceTransform(0, Xf, /*bWorldSpace=*/ true);
	// La mesh nasce alta `2 * RTCellPrismRadius`: la scala Z e' il rapporto con l'altezza chiesta.
	const float AltezzaPosata = static_cast<float>(Xf.GetScale3D().Z) * 2.f * RTCellPrismRadius;
	TestTrue(*FString::Printf(TEXT("il corpo posato segue il troncamento: alto %.1f, atteso %.1f"),
			AltezzaPosata, Dopo[0].Height()),
		FMath::IsNearlyEqual(AltezzaPosata, Dopo[0].Height(), 0.5f));
	TestTrue(TEXT("e sta alla quota del corpo troncato"),
		FMath::IsNearlyEqual(static_cast<float>(Xf.GetLocation().Z), (Dopo[0].TopZ + Dopo[0].BottomZ) * 0.5f, 0.5f));
	return true;
}

/**
 * 🔴 **Dipingere il fumo ne mostra il volume subito, non al rilascio** — `#2732`, il gemello di `Bodies`.
 *
 * Il volume con cui una superficie si dichiara nello spazio lo decide `URTHexLibrary::SurfaceVolumeFor(Surface)`,
 * e `Surface` e' uno dei quattro campi che il pennello scrive. La famiglia `SurfaceVolumes` e' arrivata con
 * `#2936`, dopo la maschera, e non c'era entrata: il fumo dipinto colorava la cella e posava il glifo, ma il
 * volume compariva solo al rilascio, quando `EndStrokeIfActive` ricostruisce tutto.
 *
 * ⚠️ **La premessa e' asserita**: il fumo ha davvero un volume, e prima della pennellata non ce n'e'. Senza,
 * il test passerebbe con una superficie che non ne produce mai.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexPaintToolBrushShowsSmokeVolumeTest,
	"RefactorTactics.HexTools.PaintingSmokeShowsItsVolumeWithTheBrushMask",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexPaintToolBrushShowsSmokeVolumeTest::RunTest(const FString&)
{
	FRTPaintToolTestWorld Mondo;
	if (!TestNotNull(TEXT("mondo di prova"), Mondo.World)) { return false; }

	// Un pavimento semplice: il pennello lo trasformera' in fumo.
	URTHexMapAsset* Asset = NewObject<URTHexMapAsset>(GetTransientPackage());
	Asset->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 0)));
	Asset->SortCells();

	ARTHexMapActor* Actor = SpawnPaintToolTestActor(Mondo.World, Asset);
	if (!TestNotNull(TEXT("actor mappa"), Actor)) { return false; }

	const UInstancedStaticMeshComponent* Volumi = FindPaintToolIsm(Actor, TEXT("SurfaceVolumes"));
	if (!TestNotNull(TEXT("l'ISM dei volumi"), Volumi)
		|| !TestTrue(TEXT("premessa: il fumo ha un volume"),
			URTHexLibrary::SurfaceVolumeFor(ERTHexSurface::Smoke).Y > 0.0)
		|| !TestEqual(TEXT("premessa: un pavimento semplice non ha volume"), Volumi->GetInstanceCount(), 0))
	{
		return false;
	}

	// Il pennello dipinge il fumo sulla cella.
	Asset->BeginStroke();
	const bool bDipinta = Asset->PaintCellInStroke(FRTCellId(0, 0, 0), ERTHexSurface::Smoke, /*MoveCost=*/ 1,
		/*bBlocksMovement=*/ false, TOptional<bool>(false));
	Asset->EndStroke();
	if (!TestTrue(TEXT("il pennello dipinge il fumo"), bDipinta)) { return false; }

	// La ricostruzione che il pennello fa a ogni cella toccata, con la sua maschera.
	Actor->RebuildInstances(URTHexPaintTool::BrushRebuildMask(/*bPaint=*/ true));

	TestEqual(TEXT("il fumo dipinto mostra subito il suo volume"), Volumi->GetInstanceCount(), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
