#include "Misc/AutomationTest.h"
#include "UObject/UObjectHash.h"
#include "Templates/Function.h"   // TFunctionRef, per il ciclo sui campi della chiave

#include "Components/InstancedStaticMeshComponent.h"
#include "Map/RTHexCellData.h"        // ERTHexSurface
#include "Map/RTHexLibrary.h"
#include "RTHexEditorModeSettings.h"
#include "RTHexWorkGrid.h"
#include "RTHexWorkGridActor.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * LA GRIGLIA DI LAVORO — dove le celle non esistono ancora (#622).
 *
 * 🔴 **L'ordine di questi test ripete la correzione di #921, e per la stessa ragione.** Il criterio ovvio —
 * *«nessuna cella della griglia coincide con una cella vera»* — e' un DIVIETO, ed e' soddisfatto alla
 * perfezione da una `BuildPlan` che non restituisce mai niente. L'obbligo positivo — *«ogni vicino LIBERO
 * di una cella esistente e' nel piano»* — sta quindi **prima**, e il divieto arriva dopo a chiudere la porta.
 *
 * ⛔ **Cio' che questi test NON coprono, dichiarato invece che lasciato scoprire:**
 *
 * 1. Che `URTHexEditorMode::Enter()` *chiami* `RefreshWorkGrid()`. Cancellando quella riga questa suite
 *    resta verde e la griglia non compare mai. Nessun automation test di questo repository apre un
 *    `UEdMode`, ed e' lo stesso limite che `RTHexOverlaySettingsTests.cpp:116-121` dichiara per la
 *    pubblicazione del settings.
 * 2. Il **valore** di default di `bShowWorkGrid`. `UEdMode::Exit()` chiama `SaveConfig()` con
 *    `bAllowCopyToDefaultObject = true`, che copia il valore dell'istanza DENTRO il CDO: un test sul
 *    default diventerebbe rosso per una preferenza utente di quel clone. Si verifica la FORMA (vedi AC-8),
 *    non il valore — il ragionamento per esteso sta a `RTHexOverlaySettingsTests.cpp:182-190`.
 * 3. Che la griglia si LEGGA come distinta da una cella vera. AC-5 misura la geometria che lo rende
 *    possibile; che l'occhio la usi davvero non lo dice nessun test.
 *
 * Tutti e tre sono a carico di **`PIE-MAPED-GRID`**, in `docs/technical/test-manuali-pie.md`.
 */

namespace
{
	// Nomi prefissati per file: namespace anonimo + unity build (vedi `SelStore*` in RTHexSelectionStoreTests).
	constexpr int32 WorkGridTestLayer = 3;

	RTHexWorkGrid::FInput WorkGridInputAround(const TArray<FRTCellId>& Existing, int32 Margin, int32 MaxCells)
	{
		RTHexWorkGrid::FInput In;
		In.bHasAsset = true;
		In.ExistingOnLayer = Existing;
		In.Layer = WorkGridTestLayer;
		In.Margin = Margin;
		In.SeedRadius = 3;
		In.MaxCells = MaxCells;
		return In;
	}

	/** Luminanza Rec.601, la stessa formula del cancello in `RTHexTests.cpp`. */
	float WorkGridLuma(const FColor& C)
	{
		return 0.299f * C.R + 0.587f * C.G + 0.114f * C.B;
	}

	int32 WorkGridManhattan(const FColor& A, const FColor& B)
	{
		return FMath::Abs(static_cast<int32>(A.R) - static_cast<int32>(B.R))
			+ FMath::Abs(static_cast<int32>(A.G) - static_cast<int32>(B.G))
			+ FMath::Abs(static_cast<int32>(A.B) - static_cast<int32>(B.B));
	}
}

/**
 * **AC 1** — l'obbligo POSITIVO, e viene prima di tutto: ogni vicino libero di una cella esistente e'
 * marcato. Senza questo, «non marcare mai niente» sarebbe una soluzione valida.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkGridMarksEveryFreeNeighbourTest,
	"RefactorTactics.HexEditor.WorkGridMarksEveryFreeNeighbourOfTheMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkGridMarksEveryFreeNeighbourTest::RunTest(const FString&)
{
	const TArray<FRTCellId> Existing = URTHexLibrary::HexArea(FRTCellId(0, 0, WorkGridTestLayer), 2);
	const RTHexWorkGrid::FPlan Plan = RTHexWorkGrid::BuildPlan(WorkGridInputAround(Existing, 1, 4096));

	const TSet<FRTCellId> Marcate(Plan.Cells);
	const TSet<FRTCellId> Esistenti(Existing);

	int32 Attesi = 0;
	int32 Mancanti = 0;
	for (const FRTCellId& Cella : Existing)
	{
		for (const FRTCellId& Vicina : URTHexLibrary::Neighbors(Cella))
		{
			if (Esistenti.Contains(Vicina))
			{
				continue;
			}
			++Attesi;
			if (!Marcate.Contains(Vicina))
			{
				++Mancanti;
			}
		}
	}

	// Guardia anti-vacuita': se il filtro smettesse di riconoscere i vicini liberi, il ciclo girerebbe a
	// vuoto e il test passerebbe senza aver verificato niente. La soglia e' deliberatamente sotto il numero
	// reale — e' una guardia contro lo zero, non un conteggio da mantenere.
	TestTrue(TEXT("almeno sei vicini liberi esaminati"), Attesi >= 6);
	TestEqual(TEXT("nessun vicino libero resta senza il proprio esagono di lavoro"), Mancanti, 0);
	TestEqual(TEXT("il ramo e' la dilatazione delle celle esistenti"),
		static_cast<int32>(Plan.Source), static_cast<int32>(RTHexWorkGrid::ESource::Dilated));

	return true;
}

/**
 * **AC 2** — il DIVIETO, e i due invarianti che lo accompagnano: niente sopra una cella vera, niente su un
 * altro piano, ordine stabile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkGridNeverOverlapsARealCellTest,
	"RefactorTactics.HexEditor.WorkGridNeverOverlapsARealCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkGridNeverOverlapsARealCellTest::RunTest(const FString&)
{
	const TArray<FRTCellId> Existing = URTHexLibrary::HexArea(FRTCellId(0, 0, WorkGridTestLayer), 2);
	const RTHexWorkGrid::FPlan Plan = RTHexWorkGrid::BuildPlan(WorkGridInputAround(Existing, 2, 4096));
	const TSet<FRTCellId> Esistenti(Existing);

	if (!TestTrue(TEXT("il piano non e' vuoto, o il resto non verifica niente"), Plan.Cells.Num() > 0))
	{
		return false;
	}

	int32 Sovrapposte = 0;
	int32 FuoriPiano = 0;
	int32 FuoriOrdine = 0;
	for (int32 I = 0; I < Plan.Cells.Num(); ++I)
	{
		if (Esistenti.Contains(Plan.Cells[I])) { ++Sovrapposte; }
		if (Plan.Cells[I].Layer != WorkGridTestLayer) { ++FuoriPiano; }
		if (I > 0 && !URTHexLibrary::StableLess(Plan.Cells[I - 1], Plan.Cells[I])) { ++FuoriOrdine; }
	}

	TestEqual(TEXT("nessun esagono di lavoro cade su una cella che esiste gia'"), Sovrapposte, 0);
	TestEqual(TEXT("la griglia vive solo sul layer di lavoro, dove i pennelli scrivono"), FuoriPiano, 0);
	TestEqual(TEXT("l'ordine e' stabile: due esecuzioni danno lo stesso elenco"), FuoriOrdine, 0);

	return true;
}

/**
 * **AC 3** — senza asset non si inventa una griglia, e il motivo del vuoto e' LEGGIBILE.
 *
 * 🔴 E' il divieto centrale della issue, provato **per tipo**: `BuildPlan` riceve coordinate e numeri, non
 * un `AActor`, quindi non puo' leggere `DemoRadius` nemmeno volendo. Questo test fissa la conseguenza
 * visibile — nessun esagono — e che `ESource::None` la distingua da un layer semplicemente saturo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkGridWithoutAssetDrawsNothingTest,
	"RefactorTactics.HexEditor.WorkGridWithoutAnAssetDrawsNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkGridWithoutAssetDrawsNothingTest::RunTest(const FString&)
{
	RTHexWorkGrid::FInput In = WorkGridInputAround(TArray<FRTCellId>(), 2, 4096);
	In.bHasAsset = false;

	const RTHexWorkGrid::FPlan Senza = RTHexWorkGrid::BuildPlan(In);
	TestEqual(TEXT("senza asset non si posa nessun esagono"), Senza.Cells.Num(), 0);
	TestEqual(TEXT("e il vuoto dichiara la propria causa"),
		static_cast<int32>(Senza.Source), static_cast<int32>(RTHexWorkGrid::ESource::None));

	// Controllo POSITIVO: con le stesse identiche impostazioni, il solo `bHasAsset` cambia l'esito. Senza
	// questo, un `BuildPlan` che non restituisce mai niente passerebbe l'asserzione qui sopra.
	In.bHasAsset = true;
	const RTHexWorkGrid::FPlan Con = RTHexWorkGrid::BuildPlan(In);
	TestTrue(TEXT("con un asset, la stessa chiamata posa qualcosa"), Con.Cells.Num() > 0);
	TestNotEqual(TEXT("e non e' piu' il ramo «nessun asset»"),
		static_cast<int32>(Con.Source), static_cast<int32>(RTHexWorkGrid::ESource::None));

	return true;
}

/**
 * **AC 4** — layer attivo vuoto: si semina, con il proprio raggio e non col margine.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkGridSeedsAnEmptyLayerTest,
	"RefactorTactics.HexEditor.WorkGridSeedsAnEmptyLayerWithItsOwnRadius",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkGridSeedsAnEmptyLayerTest::RunTest(const FString&)
{
	RTHexWorkGrid::FInput In = WorkGridInputAround(TArray<FRTCellId>(), /*Margin=*/ 0, 4096);
	In.SeedRadius = 2;

	const RTHexWorkGrid::FPlan Plan = RTHexWorkGrid::BuildPlan(In);

	TestEqual(TEXT("il ramo e' il seme"),
		static_cast<int32>(Plan.Source), static_cast<int32>(RTHexWorkGrid::ESource::Seeded));
	// 3·R·(R+1)+1 con R = 2: un esagono pieno di raggio due.
	TestEqual(TEXT("il seme e' l'esagono pieno del proprio raggio"), Plan.Cells.Num(), 19);
	TestEqual(TEXT("e il raggio applicato e' quello chiesto"), Plan.AppliedReach, 2);

	// 🔑 Il margine vale ZERO in questa chiamata: se il seme lo leggesse, il piano sarebbe vuoto. E' cio'
	// che rende i due parametri davvero distinti invece che due nomi per la stessa manopola.
	TestFalse(TEXT("il seme non ha bisogno del margine"), Plan.Cells.Num() == 0);

	int32 FuoriPiano = 0;
	for (const FRTCellId& Cella : Plan.Cells)
	{
		if (Cella.Layer != WorkGridTestLayer) { ++FuoriPiano; }
	}
	TestEqual(TEXT("anche il seme nasce sul layer di lavoro"), FuoriPiano, 0);

	return true;
}

/**
 * **AC 5** — il tetto tronca per ANELLI INTERI, e la riduzione esce dal tipo invece di restare in un log.
 *
 * ⚠️ Un semplice `Num() <= Cap` resterebbe verde anche su una `BuildPlan` che tronca a meta' anello: qui si
 * asserisce il numero **esatto** e il raggio applicato, che e' l'unica forma che distingue le due cose.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkGridTruncatesByWholeRingsTest,
	"RefactorTactics.HexEditor.WorkGridTruncatesByWholeRings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkGridTruncatesByWholeRingsTest::RunTest(const FString&)
{
	const TArray<FRTCellId> UnaSola = { FRTCellId(0, 0, WorkGridTestLayer) };

	// Controllo POSITIVO, prima del taglio: con un tetto largo escono tutti e due gli anelli.
	const RTHexWorkGrid::FPlan Intera = RTHexWorkGrid::BuildPlan(WorkGridInputAround(UnaSola, 2, 4096));
	TestEqual(TEXT("due anelli attorno a una cella sola fanno diciotto esagoni"), Intera.Cells.Num(), 18);
	TestEqual(TEXT("e il raggio applicato e' quello chiesto"), Intera.AppliedReach, 2);
	TestFalse(TEXT("senza pressione il tetto non morde"), Intera.bClamped);

	// Il taglio: un tetto fra il primo anello (6) e i due insieme (18).
	const RTHexWorkGrid::FPlan Tagliata = RTHexWorkGrid::BuildPlan(WorkGridInputAround(UnaSola, 2, 10));
	TestEqual(TEXT("entra il primo anello INTERO, non dieci esagoni su diciotto"), Tagliata.Cells.Num(), 6);
	TestEqual(TEXT("il raggio applicato scende di uno"), Tagliata.AppliedReach, 1);
	TestEqual(TEXT("e quello chiesto resta leggibile"), Tagliata.RequestedReach, 2);
	TestTrue(TEXT("la riduzione e' dichiarata, non silenziosa"), Tagliata.bClamped);

	// Monotonia: cio' che esce con margine 1 e' contenuto in cio' che esce con margine 2.
	const RTHexWorkGrid::FPlan UnAnello = RTHexWorkGrid::BuildPlan(WorkGridInputAround(UnaSola, 1, 4096));
	const TSet<FRTCellId> Due(Intera.Cells);
	int32 Perse = 0;
	for (const FRTCellId& Cella : UnAnello.Cells)
	{
		if (!Due.Contains(Cella)) { ++Perse; }
	}
	TestTrue(TEXT("almeno un esagono esaminato per la monotonia"), UnAnello.Cells.Num() >= 6);
	TestEqual(TEXT("allargare il margine non toglie mai un esagono"), Perse, 0);

	return true;
}

/**
 * **AC 6** — 🔴 **il canale distintivo, misurato.** E' il criterio centrale del DoD di #622, ed e' l'unico
 * pezzo di esso che una macchina possa guardare.
 *
 * Le celle vere TASSELLANO: i loro anelli di bordo condividono i lati e formano un alveare continuo. La
 * griglia di lavoro deve essere fatta di ISOLE — e «isola» qui non e' una parola, e' la distanza di fondo
 * scoperto fra due esagoni vicini, misurata con le stesse conversioni che li posano.
 *
 * ⚠️ **Nessun numero della board e' ricopiato qui.** Il fattore di pianta con cui `ARTHexMapActor` disegna
 * le celle (`0.95`) e' un letterale non esportato, ripetuto in tre punti di `RTHexMapActor.cpp` (`:616`,
 * `:1298`, `:1884`): copiarlo in un test del modulo editor sarebbe il difetto di #983 al quarto giro. Cio'
 * che si asserisce riguarda il fantasma, che e' cio' che questa issue possiede.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkGridGhostsAreIslandsTest,
	"RefactorTactics.HexEditor.WorkGridGhostsAreIslandsAndNeverATiling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkGridGhostsAreIslandsTest::RunTest(const FString&)
{
	constexpr float HexSize = 150.f;
	constexpr float LayerHeight = 250.f;
	const FVector Origin = FVector::ZeroVector;

	// Due celle adiacenti, posate dalla funzione vera: la distanza fra i centri non si scrive, si misura.
	const FVector A = URTHexLibrary::AxialToWorld(FRTCellId(0, 0, 0), Origin, HexSize, LayerHeight);
	const FVector B = URTHexLibrary::AxialToWorld(FRTCellId(1, 0, 0), Origin, HexSize, LayerHeight);
	const double Passo = FVector::Dist2D(A, B);

	if (!TestTrue(TEXT("due celle adiacenti distano qualcosa"), Passo > 1.0))
	{
		return false;
	}

	// L'apotema dell'anello fantasma, DERIVATA da `HexCorners` invece che da un √3/2 riscritto qui.
	const float RaggioFantasma = RTHexWorkGrid::OuterScaleInHexSize * HexSize;
	const TArray<FVector> Vertici = URTHexLibrary::HexCorners(FVector::ZeroVector, RaggioFantasma);
	if (!TestEqual(TEXT("l'esagono ha sei vertici"), Vertici.Num(), 6))
	{
		return false;
	}
	const double Apotema = ((Vertici[0] + Vertici[1]) * 0.5).Size2D();

	const double FondoScoperto = Passo - 2.0 * Apotema;

	// 🔑 Piu' di mezza cella di fondo fra due fantasmi vicini. E' il gemello a runtime dello `static_assert`
	// di `RTHexWorkGrid.h`, ma passa dalle conversioni vere invece che dalla formula.
	TestTrue(*FString::Printf(
		TEXT("fra due esagoni di lavoro adiacenti resta piu' di mezza cella di fondo (misurato %.1f uu su %.1f)"),
		FondoScoperto, 0.5 * HexSize),
		FondoScoperto >= 0.5 * HexSize);

	// 🔑 E il fantasma non si avvicina mai al confine fra le due celle: resta ben dentro l'esagono che
	// marca, dove `CellBorders` — che sta SUL confine — non puo' essere confuso con lui.
	TestTrue(*FString::Printf(
		TEXT("l'anello di lavoro resta ben dentro la cella (apotema %.1f su meta' passo %.1f)"),
		Apotema, 0.5 * Passo),
		Apotema <= 0.6 * (0.5 * Passo));

	// Controllo NEGATIVO: alla scala a cui le celle vere sono disegnate, lo stesso calcolo NON passerebbe.
	// Serve a provare che l'asserzione qui sopra discrimina davvero, invece di essere vera per ogni numero.
	const TArray<FVector> VerticiPieni = URTHexLibrary::HexCorners(FVector::ZeroVector, HexSize);
	const double ApotemaPiena = ((VerticiPieni[0] + VerticiPieni[1]) * 0.5).Size2D();
	TestFalse(TEXT("un esagono a grandezza di cella TASSELLA, e infatti non passerebbe il criterio"),
		(Passo - 2.0 * ApotemaPiena) >= 0.5 * HexSize);

	return true;
}

/**
 * **AC 7** — la tinta del fantasma non e' la tinta di nessuna superficie, e passa i due cancelli di casa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkGridColourIsNotASurfaceColourTest,
	"RefactorTactics.HexEditor.WorkGridColourIsNotASurfaceColour",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkGridColourIsNotASurfaceColourTest::RunTest(const FString&)
{
	const FColor Fantasma = RTHexWorkGrid::GhostColour();

	const UEnum* Superfici = StaticEnum<ERTHexSurface>();
	if (!TestNotNull(TEXT("l'enum delle superfici e' raggiungibile per reflection"), Superfici))
	{
		return false;
	}

	int32 Esaminate = 0;
	int32 TroppoVicine = 0;
	int32 TroppoSimiliInGrigio = 0;
	for (int32 I = 0; I < Superfici->NumEnums() - 1; ++I)
	{
		const ERTHexSurface Superficie = static_cast<ERTHexSurface>(Superfici->GetValueByIndex(I));
		const FColor Tinta = URTHexLibrary::SurfaceColor(Superficie);
		++Esaminate;

		if (WorkGridManhattan(Fantasma, Tinta) < 60)
		{
			++TroppoVicine;
			AddError(FString::Printf(TEXT("la tinta della griglia e' a %d da %s"),
				WorkGridManhattan(Fantasma, Tinta), *Superfici->GetNameStringByIndex(I)));
		}
		if (FMath::Abs(WorkGridLuma(Fantasma) - WorkGridLuma(Tinta)) < 20.f)
		{
			++TroppoSimiliInGrigio;
			AddError(FString::Printf(TEXT("in scala di grigi la griglia e' a %.1f da %s"),
				FMath::Abs(WorkGridLuma(Fantasma) - WorkGridLuma(Tinta)), *Superfici->GetNameStringByIndex(I)));
		}
	}

	// Guardia anti-vacuita': senza, un enum che smettesse di enumerare renderebbe questo test verde a vuoto.
	TestTrue(TEXT("almeno cinque superfici esaminate"), Esaminate >= 5);
	TestEqual(TEXT("nessuna superficie e' confondibile con la griglia di lavoro"), TroppoVicine, 0);
	TestEqual(TEXT("e nemmeno in scala di grigi"), TroppoSimiliInGrigio, 0);

	// L'anello di bordo delle celle vere e' l'altro vicino da cui stare lontani: e' il segno che la griglia
	// di lavoro rischia di imitare.
	//
	// ⚠️ **Letto da `URTHexLibrary::CellBorderColor()`, non ricopiato.** Fino al 2026-09-23 questa
	// riga portava un `FColor(25, 25, 25)` scritto a mano, cioe' esattamente il difetto di #983 che AC-6
	// dichiara di evitare: schiarendo il bordo della board per leggibilita', il fantasma sarebbe diventato
	// confondibile col segno da cui esiste per distinguersi, e questo test sarebbe restato verde contro un
	// letterale stantio.
	TestTrue(TEXT("e neppure con l'anello di bordo delle celle vere"),
		WorkGridManhattan(Fantasma, URTHexLibrary::CellBorderColor()) >= 60);

	return true;
}

/**
 * **AC 8** — il portatore non puo' raggiungere il livello salvato, e non puo' rubare un click.
 *
 * 🔑 Provato per FORMA e non per disciplina: si leggono i flag della `UClass` e lo stato di collisione dei
 * componenti di default, non la buona volonta' di chi ha scritto il costruttore. E' il gemello di
 * `RefactorTactics.HexMap.OnlyTheCellsComponentIsClickable`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkGridCarrierCannotReachTheLevelTest,
	"RefactorTactics.HexEditor.WorkGridCarrierCannotReachTheLevelNorStealAClick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkGridCarrierCannotReachTheLevelTest::RunTest(const FString&)
{
	UClass* Classe = ARTHexWorkGridActor::StaticClass();
	if (!TestNotNull(TEXT("la classe del portatore esiste"), Classe))
	{
		return false;
	}

	TestTrue(TEXT("il portatore e' Transient: le sue istanze non si serializzano nel .umap"),
		Classe->HasAnyClassFlags(CLASS_Transient));
	TestTrue(TEXT("e non e' piazzabile a mano nel livello"),
		Classe->HasAnyClassFlags(CLASS_NotPlaceable));

	UObject* CDO = Classe->GetDefaultObject();
	if (!TestNotNull(TEXT("il CDO del portatore esiste"), CDO))
	{
		return false;
	}

	TArray<UObject*> Sottooggetti;
	// ⚠️ `EGetObjectsFlags::None` e non il `bool`: l'overload booleano e' deprecato in UE 5.8 e smettera' di
	// compilare. `None` significa «solo i figli diretti», cioe' i default subobject del portatore.
	GetObjectsWithOuter(CDO, Sottooggetti, EGetObjectsFlags::None);

	int32 Istanziati = 0;
	int32 Collidibili = 0;
	int32 ConOmbra = 0;
	for (const UObject* Oggetto : Sottooggetti)
	{
		if (Oggetto->IsA<UInstancedStaticMeshComponent>())
		{
			++Istanziati;
		}
		if (const UPrimitiveComponent* Primitiva = Cast<UPrimitiveComponent>(Oggetto))
		{
			if (Primitiva->GetCollisionEnabled() != ECollisionEnabled::NoCollision) { ++Collidibili; }
			if (Primitiva->CastShadow) { ++ConOmbra; }
		}
	}

	// Guardia anti-vacuita': senza, un portatore senza componenti passerebbe entrambi i divieti.
	TestTrue(TEXT("il portatore ha almeno un componente istanziato, o non disegna niente"), Istanziati >= 1);
	TestEqual(TEXT("nessun componente collidibile: il raycast dei tool non deve ripiegare"), Collidibili, 0);
	TestEqual(TEXT("e nessuna ombra: la griglia di lavoro non illumina la scena"), ConOmbra, 0);

	// ⛔ **Un Actor per esagono e' vietato dalla issue**, e l'instancing e' il modo in cui il divieto e'
	// rispettato: il portatore e' UNO e porta N istanze.
	TestEqual(TEXT("un solo componente istanziato, non uno per esagono"), Istanziati, 1);

	return true;
}

/**
 * **AC 9** — le tre impostazioni della griglia sono stato del MODE e sopravvivono al cambio di strumento.
 *
 * ⚠️ **Si verifica la FORMA e non il valore**, per la ragione dichiarata in cima al file: `SaveConfig()`
 * copia il valore dell'istanza dentro il CDO, e un test sul default diventerebbe rosso per una preferenza
 * utente. ⚠️ E `CPF_Edit` da solo non distingue `EditAnywhere` da `VisibleAnywhere` — entrambe lo
 * impostano; quella che discrimina e' `CPF_EditConst`, che solo la seconda aggiunge
 * (`RTHexToolPropertiesTests.cpp:39-52`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkGridSettingsAreModeStateTest,
	"RefactorTactics.HexEditor.WorkGridSettingsArePersistedModeState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkGridSettingsAreModeStateTest::RunTest(const FString&)
{
	UClass* Classe = URTHexEditorModeSettings::StaticClass();
	if (!TestNotNull(TEXT("la classe del settings esiste"), Classe))
	{
		return false;
	}

	TestTrue(TEXT("il settings ha un file di config in cui scrivere"),
		Classe->ClassConfigName != NAME_None);

	const TArray<FName> Attese = {
		GET_MEMBER_NAME_CHECKED(URTHexEditorModeSettings, bShowWorkGrid),
		GET_MEMBER_NAME_CHECKED(URTHexEditorModeSettings, WorkGridMargin),
		GET_MEMBER_NAME_CHECKED(URTHexEditorModeSettings, WorkGridSeedRadius),
	};

	int32 Esaminate = 0;
	for (const FName& Nome : Attese)
	{
		FProperty* Property = Classe->FindPropertyByName(Nome);
		if (!TestNotNull(*FString::Printf(TEXT("%s esiste sul settings del mode"), *Nome.ToString()), Property))
		{
			continue;
		}
		++Esaminate;
		TestTrue(*FString::Printf(TEXT("%s e' persistita (CPF_Config)"), *Nome.ToString()),
			Property->HasAnyPropertyFlags(CPF_Config));
		TestTrue(*FString::Printf(TEXT("%s si vede nel pannello del mode"), *Nome.ToString()),
			Property->HasAnyPropertyFlags(CPF_Edit));
		TestFalse(*FString::Printf(TEXT("%s si puo' CAMBIARE, non solo leggere"), *Nome.ToString()),
			Property->HasAnyPropertyFlags(CPF_EditConst));
	}

	TestEqual(TEXT("tutte e tre le impostazioni della griglia sono state esaminate"), Esaminate, Attese.Num());

	return true;
}

/**
 * **AC 10** — 🔴 **la chiave d'invalidazione si accorge di OGNI campo che porta.**
 *
 * `FWatch` e' l'intero meccanismo per cui la griglia si rifa': se un campo smettesse di partecipare al
 * confronto, la vista resterebbe ferma su un dato che e' cambiato — la **vista che mente** di #622 — e
 * **nessuno degli altri test se ne accorgerebbe**, perche' esercitano `BuildPlan` e non la chiave.
 *
 * ⚠️ Con `operator==` dichiarato `= default` il rischio e' chiuso per costruzione. Questo test esiste
 * perche' il `= default` possa essere tolto solo di proposito: chi lo sostituisse con un confronto scritto
 * a mano, dimenticando un campo, lo troverebbe rosso qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkGridWatchNoticesEveryFieldTest,
	"RefactorTactics.HexEditor.WorkGridWatchNoticesEveryFieldItCarries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkGridWatchNoticesEveryFieldTest::RunTest(const FString&)
{
	// Due `UObject` distinti e sicuramente vivi, per le due chiavi d'oggetto.
	UObject* Primo = GetTransientPackage();
	UObject* Secondo = URTHexEditorModeSettings::StaticClass()->GetDefaultObject();
	if (!TestNotNull(TEXT("primo oggetto"), Primo) || !TestNotNull(TEXT("secondo oggetto"), Secondo))
	{
		return false;
	}

	{
		const RTHexWorkGrid::FWatch A;
		const RTHexWorkGrid::FWatch B;
		TestTrue(TEXT("due chiavi appena costruite sono uguali, o il resto non significa niente"), A == B);
	}

	int32 Campi = 0;
	auto Discrimina = [this, &Campi](const TCHAR* Nome, TFunctionRef<void(RTHexWorkGrid::FWatch&)> Muta)
	{
		const RTHexWorkGrid::FWatch Base;
		RTHexWorkGrid::FWatch Mutata;
		Muta(Mutata);
		++Campi;
		TestFalse(*FString::Printf(TEXT("cambiare %s deve rompere l'uguaglianza"), Nome), Base == Mutata);
	};

	Discrimina(TEXT("MapActor"), [Primo](RTHexWorkGrid::FWatch& W) { W.MapActor = FObjectKey(Primo); });
	Discrimina(TEXT("MapAsset"), [Secondo](RTHexWorkGrid::FWatch& W) { W.MapAsset = FObjectKey(Secondo); });
	Discrimina(TEXT("Revision"), [](RTHexWorkGrid::FWatch& W) { W.Revision += 1; });
	Discrimina(TEXT("NumCells"), [](RTHexWorkGrid::FWatch& W) { W.NumCells += 1; });
	Discrimina(TEXT("ActiveLayer"), [](RTHexWorkGrid::FWatch& W) { W.ActiveLayer += 1; });
	Discrimina(TEXT("bShow"), [](RTHexWorkGrid::FWatch& W) { W.bShow = !W.bShow; });
	Discrimina(TEXT("Margin"), [](RTHexWorkGrid::FWatch& W) { W.Margin += 1; });
	Discrimina(TEXT("SeedRadius"), [](RTHexWorkGrid::FWatch& W) { W.SeedRadius += 1; });

	// Guardia anti-vacuita' e, insieme, la rete sul campo AGGIUNTO: la chiave ha otto campi, e chi ne
	// aggiunge un nono trova questo numero fermo e deve decidere che farne.
	TestEqual(TEXT("tutti e otto i campi della chiave sono stati esercitati"), Campi, 8);

	// 🔑 La posa e' una chiave a parte, e discrimina anch'essa: trascinare l'actor non cambia l'insieme,
	// ma deve far ri-posare.
	int32 CampiPosa = 0;
	auto DiscriminaPosa = [this, &CampiPosa](const TCHAR* Nome, TFunctionRef<void(RTHexWorkGrid::FPlacement&)> Muta)
	{
		const RTHexWorkGrid::FPlacement Base;
		RTHexWorkGrid::FPlacement Mutata;
		Muta(Mutata);
		++CampiPosa;
		TestFalse(*FString::Printf(TEXT("cambiare %s deve rompere l'uguaglianza della posa"), Nome), Base == Mutata);
	};
	DiscriminaPosa(TEXT("Origin"), [](RTHexWorkGrid::FPlacement& P) { P.Origin.X += 1.0; });
	DiscriminaPosa(TEXT("HexSize"), [](RTHexWorkGrid::FPlacement& P) { P.HexSize += 1.f; });
	DiscriminaPosa(TEXT("LayerHeight"), [](RTHexWorkGrid::FPlacement& P) { P.LayerHeight += 1.f; });
	TestEqual(TEXT("tutti e tre i campi della posa sono stati esercitati"), CampiPosa, 3);

	return true;
}

/**
 * **AC 11** — 🔴 **il seme nasce dove si sta lavorando, non sull'origine.**
 *
 * Su una mappa autorata lontano dall'origine — la precondizione che `PIE-MAPED-FRAME` richiede gia' a
 * questo progetto — passare a un layer vuoto seminava la griglia a migliaia di unita' dalle celle, e
 * lasciava senza fantasmi proprio le coordinate sopra di esse. Cioe' l'opposto di *«vedere dove cadra' la
 * prossima cella»*.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkGridSeedFollowsTheMapTest,
	"RefactorTactics.HexEditor.WorkGridSeedFollowsTheMapInsteadOfTheOrigin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkGridSeedFollowsTheMapTest::RunTest(const FString&)
{
	// L'ancora, da sola: media assiale, riportata sul layer chiesto.
	const TArray<FRTCellId> Lontane = {
		FRTCellId(30, 30, 0), FRTCellId(32, 30, 0), FRTCellId(30, 32, 0), FRTCellId(32, 32, 0)
	};
	const FRTCellId Ancora = RTHexWorkGrid::AnchorFor(Lontane, 1);
	TestEqual(TEXT("l'ancora e' la media assiale, X"), Ancora.X, 31);
	TestEqual(TEXT("l'ancora e' la media assiale, Y"), Ancora.Y, 31);
	TestEqual(TEXT("e vive sul layer chiesto"), Ancora.Layer, 1);

	// Insieme vuoto: l'origine e' l'unica risposta onesta, e va detta.
	const FRTCellId Vuota = RTHexWorkGrid::AnchorFor(TArray<FRTCellId>(), 5);
	TestEqual(TEXT("senza celle l'ancora e' l'origine, X"), Vuota.X, 0);
	TestEqual(TEXT("senza celle l'ancora e' l'origine, Y"), Vuota.Y, 0);
	TestEqual(TEXT("sul layer chiesto"), Vuota.Layer, 5);

	// Il seme la usa davvero: nessuna cella sul layer di lavoro, ancora lontana dall'origine.
	RTHexWorkGrid::FInput In = WorkGridInputAround(TArray<FRTCellId>(), /*Margin=*/ 0, 4096);
	In.SeedRadius = 1;
	In.SeedAnchor = FRTCellId(31, 31, WorkGridTestLayer);

	const RTHexWorkGrid::FPlan Plan = RTHexWorkGrid::BuildPlan(In);
	TestEqual(TEXT("il ramo e' il seme"),
		static_cast<int32>(Plan.Source), static_cast<int32>(RTHexWorkGrid::ESource::Seeded));
	TestEqual(TEXT("un esagono di raggio uno attorno all'ancora"), Plan.Cells.Num(), 7);

	int32 VicineAllAncora = 0;
	int32 SullOrigine = 0;
	for (const FRTCellId& Cella : Plan.Cells)
	{
		if (URTHexLibrary::HexDistance(Cella, FRTCellId(31, 31, WorkGridTestLayer)) <= 1) { ++VicineAllAncora; }
		if (URTHexLibrary::HexDistance(Cella, FRTCellId(0, 0, WorkGridTestLayer)) <= 1) { ++SullOrigine; }
	}
	TestEqual(TEXT("tutti e sette gli esagoni stanno attorno all'ancora"), VicineAllAncora, 7);
	// Controllo NEGATIVO: e nessuno sull'origine, che e' dove finivano prima.
	TestEqual(TEXT("nessuno e' rimasto sull'origine"), SullOrigine, 0);

	return true;
}

/**
 * **AC 12** — 🔴 **quando nemmeno il primo anello entra nel tetto, la griglia si riduce invece di sparire.**
 *
 * La regola degli anelli interi, applicata alla lettera, azzerava la griglia su una mappa enorme o
 * frammentata: cioe' proprio dove vedere il bordo serve di piu', il primo criterio del DoD non era
 * soddisfatto. Un pezzo di anello dichiarato e' una vista che manca a meta'; nessun fantasma era una vista
 * che manca e basta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkGridDegradesInsteadOfVanishingTest,
	"RefactorTactics.HexEditor.WorkGridDegradesInsteadOfVanishingUnderTheCap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkGridDegradesInsteadOfVanishingTest::RunTest(const FString&)
{
	// Un esagono pieno di raggio 3: il suo primo anello di dilatazione ha 24 celle.
	const TArray<FRTCellId> Existing = URTHexLibrary::HexArea(FRTCellId(0, 0, WorkGridTestLayer), 3);

	// Controllo POSITIVO: con un tetto che lo contiene, l'anello esce INTERO e non e' parziale.
	const RTHexWorkGrid::FPlan Intero = RTHexWorkGrid::BuildPlan(WorkGridInputAround(Existing, 1, 24));
	TestEqual(TEXT("il primo anello di un esagono di raggio tre ha ventiquattro celle"), Intero.Cells.Num(), 24);
	TestFalse(TEXT("e non e' parziale"), Intero.bPartialRing);
	TestFalse(TEXT("ne' ridotto"), Intero.bClamped);
	TestEqual(TEXT("il raggio applicato e' uno"), Intero.AppliedReach, 1);

	// Il caso che morde: il tetto sta SOTTO il primo anello.
	const RTHexWorkGrid::FPlan Ridotta = RTHexWorkGrid::BuildPlan(WorkGridInputAround(Existing, 1, 10));
	TestEqual(TEXT("si posa esattamente quanto il tetto concede, non zero"), Ridotta.Cells.Num(), 10);
	TestTrue(TEXT("e il pezzo di anello e' DICHIARATO"), Ridotta.bPartialRing);
	TestTrue(TEXT("come lo e' la riduzione"), Ridotta.bClamped);

	// Il pezzo e' preso in ordine stabile, quindi due esecuzioni danno lo stesso pezzo.
	const RTHexWorkGrid::FPlan Ancora = RTHexWorkGrid::BuildPlan(WorkGridInputAround(Existing, 1, 10));
	bool bIdentiche = Ancora.Cells.Num() == Ridotta.Cells.Num();
	for (int32 I = 0; bIdentiche && I < Ridotta.Cells.Num(); ++I)
	{
		bIdentiche = Ancora.Cells[I] == Ridotta.Cells[I];
	}
	TestTrue(TEXT("il pezzo e' deterministico: due esecuzioni danno lo stesso elenco"), bIdentiche);

	// ⛔ E il pezzo resta fuori dalle celle vere: una riduzione non e' una licenza a sovrapporsi.
	const TSet<FRTCellId> Esistenti(Existing);
	int32 Sovrapposte = 0;
	for (const FRTCellId& Cella : Ridotta.Cells)
	{
		if (Esistenti.Contains(Cella)) { ++Sovrapposte; }
	}
	TestEqual(TEXT("nemmeno ridotta la griglia cade su una cella vera"), Sovrapposte, 0);

	// Dal SECONDO anello in poi la regola resta intera: un tetto fra i due anelli non produce un pezzo.
	const RTHexWorkGrid::FPlan DueAnelli = RTHexWorkGrid::BuildPlan(WorkGridInputAround(Existing, 2, 40));
	TestEqual(TEXT("il secondo anello non entra, e non se ne prende un pezzo"), DueAnelli.Cells.Num(), 24);
	TestFalse(TEXT("nessun anello parziale oltre il primo"), DueAnelli.bPartialRing);
	TestTrue(TEXT("ma la riduzione e' dichiarata"), DueAnelli.bClamped);
	TestEqual(TEXT("e il raggio applicato si ferma a uno"), DueAnelli.AppliedReach, 1);

	return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS
