#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Map/RTCellId.h"

#include "RTGrayboxUnitFacingFixture.generated.h"

/**
 * Un corpo graybox con il suo **facing visibile da fermo**: l'oggetto della Station 01 (#1992, Epic #1990,
 * `D-304`).
 *
 * 🔑 **Perche' esiste.** `Facing` e' stato logico autorevole dal 2026-08-09 (`E16`) — sta in snapshot,
 * `TurnLog` e hash del replay — ma non esisteva **nessun oggetto che lo mostrasse fermo**. Per vedere dove
 * guarda un'unita' bisognava far partire una partita, quindi ogni domanda sulla leggibilita' del facing si
 * rispondeva muovendo qualcosa in PIE e guardandolo per un istante. Un laboratorio serve a guardare le
 * cose **ferme**.
 *
 * ## 🔴 Perche' C++ e non un Blueprint puro, contro il `D003` che la issue dichiarava
 *
 * La spec di #1992 prescriveva un Blueprint puro, sul precedente di `BP_Graybox_CellPlacementVolume`, e
 * dichiarava un buco noto: *«sei test coprono la formula, non che il Blueprint la chiami; un fixture che
 * calcolasse l'origine per conto proprio li lascerebbe tutti verdi»*. Quel buco era la parte piu' fragile
 * dell'intera issue, e con la geometria qui **si chiude**: `MarkerTransform` e' pura e chiamabile headless,
 * e `RefactorTactics.Graybox.FixtureMarkerComesFromTheLibrary` spawna davvero l'attore e confronta il
 * componente posato con `URTHexLibrary::FacingMarkerOrigin`. Il legame non e' piu' una promessa d'authoring.
 *
 * Il Blueprint resta — `/Game/RT/World/Graybox/Fixtures/BP_Graybox_UnitFacingFixture` — ma e' una
 * sottoclasse **senza grafo**: serve a posare e a tarare i cinque parametri, non a fare geometria.
 *
 * ## Il contratto geometrico
 *
 * ```text
 * MarkerOrigin = UnitCenter + Forward(Facing) * BodyRadius + Up * FaceHeight
 * ```
 *
 * ⚠️ **Il marker NON nasce dal centro.** Un marker che parte dal centro attraversa il corpo: da vicino lo
 * si vede spuntare da dentro, e a camera tattica la sua lunghezza apparente include il raggio del corpo —
 * cioe' due corpi con la stessa `MarkerLength` ma raggio diverso sembrano guardare a distanze diverse.
 *
 * ⛔ **Nessuna trigonometria qui dentro.** Origine e orientamento vengono da `URTHexLibrary`; se la
 * convenzione dei sei lati cambiasse, questo la seguirebbe invece di mentire in silenzio.
 *
 * ## Confini
 *
 * ⛔ Nessuna regola di gioco: non legge `MapState`, non scrive snapshot, non tocca `TurnLog` ne'
 * `StateHash`. E' un oggetto di scena.
 * ⛔ Non e' un `ARTUnit` e non ne eredita: un'unita' vera passa da altro.
 * ⛔ Nessun free-angle: `Facing` e' una delle sei `ERTHexDirection`, e non esiste una rotazione libera che
 * lo sovrascriva.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Graybox Unit Facing Fixture"))
class REFACTORTACTICS_API ARTGrayboxUnitFacingFixture : public AActor
{
	GENERATED_BODY()

public:
	ARTGrayboxUnitFacingFixture();

	/** La direzione mostrata. Enum, non angolo: e' il vocabolario canonico e resta quello. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Graybox")
	ERTHexDirection Facing = ERTHexDirection::E;

	/**
	 * Raggio del corpo. Default `60` — **derivato, non scelto**: il cilindro engine ha raggio `50` a scala
	 * `1` e `ARTUnit::BaseMeshScale.X` vale `1.2`.
	 *
	 * ⛔ Non e' un numero canonico nuovo: `GBX-5` — l'ingombro dell'unita' rispetto alla cella — resta
	 * aperta e di #1094, e si chiude a `U25`. Questo e' la fotografia del presente.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Graybox")
	float BodyRadius = 60.f;

	/** Altezza del corpo. Default `180` = `ARTUnit::UnitHalfHeight * 2`, la sommita' del cilindro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Graybox")
	float BodyHeight = 180.f;

	/**
	 * Quota della faccia da cui parte il marker. Default `120` = **due terzi** di `BodyHeight`.
	 *
	 * 🔴 **Era `24`, e il numero veniva dal posto sbagliato.** L'avevo preso da `WedgeLocalZ` di
	 * `RTScenarioPreviewActor` chiamandolo «derivato» — ma li' il cuneo sta a `WedgeForward = 78`, cioe'
	 * FUORI dal corpo, in un'anteprima con un'altra camera. Trasferito qui, su un corpo alto `180`, quella
	 * quota cade **alla base**: il marker finiva schiacciato fra il disco a terra e il pavimento, dove
	 * nessun contrasto lo salva.
	 *
	 * ⚠️ **Un default «derivato» da un contesto diverso non e' derivato: e' copiato.** La quota giusta si
	 * misura sul corpo che il marker veste, non su un altro.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Graybox")
	float FaceHeight = 120.f;

	/**
	 * Lunghezza del marker. Default `40` = il cuneo del preview (`WedgeScale.X * 100`).
	 *
	 * ⚠️ **Non entra in `MarkerTransform` come origine**: l'origine sta sulla superficie del corpo e non
	 * dipende da questa. E' cio' che rende la lunghezza una **misura** invece della somma di due cose.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Graybox")
	float MarkerLength = 70.f;

	/** Mostra il nome della direzione. ⚠️ **Secondo canale, non il primo**: la geometria deve bastare. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Graybox")
	bool bShowLabel = true;

	/**
	 * I tre materiali, **soft** e risolti in `OnConstruction`.
	 *
	 * 🔴 **Non un `ConstructorHelpers::FObjectFinder`, e la ragione e' misurata**: il CDO si costruisce al
	 * caricamento del modulo, cioe' PRIMA che `RTBuildGrayboxFixtures` crei i materiali. Alla prima
	 * esecuzione il finder falliva — *«Failed to find MI_Graybox_Fixture_Body»* — e il fixture nasceva col
	 * grigio di default: esattamente il difetto che questi materiali esistono per chiudere.
	 *
	 * ➕ Effetto collaterale che vale: essendo `UPROPERTY` si vedono e si sostituiscono dal Details, quindi
	 * chi vuole provare un contrasto diverso non tocca il codice.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Graybox|Materiali")
	TSoftObjectPtr<class UMaterialInterface> BodyMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Graybox|Materiali")
	TSoftObjectPtr<class UMaterialInterface> MarkerMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Graybox|Materiali")
	TSoftObjectPtr<class UMaterialInterface> AnchorMaterial;

	/**
	 * ⛔ **Il root e' neutro e resta neutro.** Una scala non uniforme qui non fallisce: **deforma in
	 * silenzio** tutto cio' che le sta sotto — e' #593, e il guardiano e' `FixtureRootIsNeutral`.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Graybox")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Graybox")
	TObjectPtr<class UStaticMeshComponent> UnitBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Graybox")
	TObjectPtr<class UStaticMeshComponent> FacingMarker;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Graybox")
	TObjectPtr<class UStaticMeshComponent> GroundAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Graybox")
	TObjectPtr<class UTextRenderComponent> OptionalLabel;

	/**
	 * 🔑 **Il riferimento contro cui il facing si giudica.** Punta sempre a `ERTHexDirection::E`, che in
	 * coordinate-mondo e' **+X** — e resta fermo mentre `Facing` gira.
	 *
	 * 🔴 **Perche' esiste**: nella seduta `U41` il marker seguiva la direzione scelta, ma il verdetto e'
	 * stato *«non so se e' corretto perche' non so qual e' il nord»*. Senza un riferimento **indipendente**
	 * nella scena, «il marker guarda dove dico io» non e' falsificabile: ogni orientamento sembra giusto
	 * se si sceglie il nord dopo averlo visto. E' lo stesso difetto che `PIE-FACING` ha risolto sulle
	 * unita' con `FacingArrow`, e per la stessa ragione: separare *«sbagliato»* da *«non so»*.
	 *
	 * ⚠️ **Rotazione ASSOLUTA**: se ereditasse quella dell'attore ruoterebbe con lui e smetterebbe di
	 * essere un riferimento nell'istante in cui servirebbe di piu'.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Graybox")
	TObjectPtr<class UArrowComponent> EastReference;

	/** L'etichetta del riferimento: una freccia senza nome sposta il dubbio, non lo toglie. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Graybox")
	TObjectPtr<class UTextRenderComponent> EastReferenceLabel;

	/**
	 * La posa del marker nello spazio dell'attore: **pura, e chiamabile headless**.
	 *
	 * 🔑 E' il punto in cui questa classe smette di essere un asset e diventa qualcosa che un test puo'
	 * interrogare. `OnConstruction` non fa altro che applicarla.
	 *
	 * ⚠️ La **traslazione** e' il centro della mesh, non l'origine del contratto: il cubo engine e'
	 * centrato, quindi il suo centro sta mezza lunghezza piu' avanti dell'origine. Chi verifica il
	 * contratto guardi `URTHexLibrary::FacingMarkerOrigin`, che e' cio' che questa funzione consuma.
	 */
	static FTransform MarkerTransform(ERTHexDirection Facing, float BodyRadius, float FaceHeight, float MarkerLength);

	/** La posa del corpo: cilindro engine portato a raggio e altezza, appoggiato sul piano del root. */
	static FTransform BodyTransform(float BodyRadius, float BodyHeight);

	virtual void OnConstruction(const FTransform& Transform) override;
};
