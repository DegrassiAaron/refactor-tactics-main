#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"

#include "RTPlaygroundPanelLibrary.generated.h"

class ARTGrayboxUnitFacingFixture;

/**
 * Una stazione, nella forma che un **Blueprint** puo' leggere.
 *
 * 🔴 **Esiste perche' `RTPlayground::FStation` NON e' raggiungibile da un Blueprint** — e' una `struct` C++
 * nuda in un namespace, e `RTPlaygroundLayout.h` dichiarava comunque, nella propria doc, che *«e' anche
 * cio' che il pannello (#1993) consuma»*. Quella frase descriveva un'intenzione, non una capacita':
 * `grep -cE "UCLASS|UFUNCTION|USTRUCT"` su quell'header dava **0**.
 *
 * ⚠️ E' lo stesso difetto di #1992, dove `EdgeRotation` esisteva ma era una `static` nuda. Li' il Blueprint
 * avrebbe dovuto incidersi sei angoli; qui avrebbe dovuto ricopiare **otto rettangoli** nel grafo — cioe'
 * `#1459`, *«tre posti che elencavano le fixture e nessuno dei tre coincideva col codice»*.
 *
 * ⛔ Questa struct **non e' una seconda planimetria**: e' una vista. I valori vengono da
 * `RTPlayground::Stations()`, e `StationsMatchTheLayout` lo verifica confrontandoli uno per uno.
 */
USTRUCT(BlueprintType)
struct FRTPlaygroundStationInfo
{
	GENERATED_BODY()

	/** `1`..`8`, come sul signage e sulle chip. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playground")
	int32 Number = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playground")
	FString Name;

	/**
	 * Bounds in **unita' Unreal**, non in metri.
	 *
	 * ⚠️ La planimetria e' in metri perche' e' la lingua della roadmap; un pannello che pilota una camera
	 * ha bisogno di unita' mondo. La conversione avviene **qui una volta**, con `WorldFromMetres`, invece
	 * che in ogni nodo del grafo — dove diventerebbe un `* 100` da ricordare.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playground")
	FVector2D MinWorld = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playground")
	FVector2D MaxWorld = FVector2D::ZeroVector;

	/** Il centro, gia' come `FVector`: e' cio' che `Focus` passa alla camera. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playground")
	FVector CentreWorld = FVector::ZeroVector;

	/**
	 * `true` solo per la Station 01 in `GKP 0.1`.
	 *
	 * ⚠️ Le chip `PLANNED` **non devono sembrare funzionanti**: una station non viva esiste come pad e
	 * signage e non finanzia il proprio sistema.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playground")
	bool bLive = false;
};

/** Lo stato che l'`HEADER` mostra. Due valori: o si lavora, o si dice perche' no. */
UENUM(BlueprintType)
enum class ERTPlaygroundReadiness : uint8
{
	Ready,
	Error
};

/**
 * Il verdetto sulla mappa aperta.
 *
 * ⚠️ `Reason` e' **non vuota quando `Error`**, ed e' un requisito non un'abitudine: la DoD chiede
 * *«`Error` con una ragione leggibile, e non un pannello vuoto che sembra rotto»*. Un pannello muto e un
 * pannello guasto hanno lo stesso aspetto.
 */
USTRUCT(BlueprintType)
struct FRTPlaygroundMapState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playground")
	ERTPlaygroundReadiness State = ERTPlaygroundReadiness::Error;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playground")
	FString MapName;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playground")
	FString Reason;
};

/**
 * Il **modello** del Playground Panel (#1993, Epic #1990, `D-304`): tutto cio' che il pannello deve sapere,
 * in una forma che un `EditorUtilityWidget` puo' chiamare e un test puo' interrogare **senza aprire un
 * viewport**.
 *
 * 🔑 **Perche' un modello e non nodi nel grafo.** Un Blueprint non si diffa e non si esercita headless:
 * cio' che vive li' dentro non ha oracolo. Qui invece la sezione Automation della issue diventa
 * eseguibile — le otto station, le sei direzioni e il verdetto sulla mappa sono funzioni pure con i loro
 * test, e il widget si limita a mostrarle.
 *
 * ⛔ **Nessuna autorita' di gioco.** Non legge `MapState`, non scrive snapshot, non tocca `TurnLog` ne'
 * `StateHash`, non decide LOS/percorsi/bersagli. L'unica scrittura ammessa e' sui cinque parametri di
 * **presentazione** del fixture.
 *
 * ⛔ **Nessun `Tick`**: sono funzioni pure, e chi le chiama decide quando.
 */
UCLASS()
class REFACTORTACTICSEDITOR_API URTPlaygroundPanelLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Le otto station, nella forma leggibile da Blueprint. **Delega** a `RTPlayground::Stations()`. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playground")
	static TArray<FRTPlaygroundStationInfo> GetStations();

	/** La station con quel numero. `false` fuori da `1..8`, e `OutStation` resta vuota — mai una finta. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playground")
	static bool FindStation(int32 Number, FRTPlaygroundStationInfo& OutStation);

	/**
	 * Le voci del dropdown `Facing`.
	 *
	 * 🔑 **Derivate da `StaticEnum<ERTHexDirection>()`, mai incise.** Una direzione aggiunta domani
	 * comparirebbe per costruzione; un elenco scritto a mano no, ed e' il difetto di `#1459`.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playground")
	static TArray<FString> GetFacingOptions();

	/** La direzione corrispondente a una voce del dropdown. `false` se la stringa non e' del set. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playground")
	static bool ParseFacingOption(const FString& Option, ERTHexDirection& OutFacing);

	/**
	 * `Ready` / `Error` a partire dal nome della mappa aperta. **Pura**: nessun `UWorld`, nessun viewport.
	 *
	 * ⚠️ Accetta sia il nome nudo sia un percorso lungo: chi chiama non deve sapere quale dei due ha in
	 * mano, e la differenza fra `L_GrayKitPlayground` e `/Game/.../L_GrayKitPlayground` non e' una
	 * decisione del pannello.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playground")
	static FRTPlaygroundMapState EvaluateMapState(const FString& OpenMapName);

	/**
	 * 🔴 **Scrive il `Facing` E ricostruisce, in un gesto solo.**
	 *
	 * ⚠️ **Senza questa funzione il pannello riprodurrebbe la trappola per cui la issue esiste.** La sua
	 * sezione *Why* apre proprio su quella: *«si scriveva a mano nel Details, e poi andava forzato il
	 * ridisegno toccando `ActiveLayer 0 -> 1 -> 0`: senza quel gesto si guardava la geometria vecchia»*.
	 *
	 * Il fixture posiziona il marker in `OnConstruction`, e `AActor::RerunConstructionScripts` **non e' una
	 * `UFUNCTION`** (`Actor.h:3417`): un Blueprint puo' scrivere i cinque `UPROPERTY` — sono
	 * `BlueprintReadWrite` — ma **non puo' forzare il ricalcolo**. Cambierebbe il dato e il marker
	 * resterebbe fermo, e chi guarda cercherebbe il difetto nel fixture, che e' corretto.
	 *
	 * `false` su attore nullo: rifiuto, non crash.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Playground")
	static bool ApplyFixtureFacing(ARTGrayboxUnitFacingFixture* Fixture, ERTHexDirection Facing);

	/** Gli altri quattro parametri, con la stessa disciplina: scrive e ricostruisce insieme. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Playground")
	static bool ApplyFixtureParameters(ARTGrayboxUnitFacingFixture* Fixture,
		float BodyRadius, float BodyHeight, float FaceHeight, float MarkerLength);

	/**
	 * Rimette i **default dichiarati**.
	 *
	 * ⚠️ Li legge dal **CDO**, non da letterali: incidere `60 / 180 / 120 / 70` qui significherebbe che il
	 * giorno in cui il fixture cambia default il `Reset` riporta a valori che non lo sono piu' — e nessuno
	 * se ne accorge, perche' il pannello resta coerente con se stesso.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Playground")
	static bool ResetFixture(ARTGrayboxUnitFacingFixture* Fixture);

	/**
	 * Le tre righe di `DIAGNOSTICS`.
	 *
	 * 🔑 **Non sono decorazione**, e stanno qui e non nel widget perche' un refuso — `NONE.` con un punto,
	 * o `MINIMAL` al posto di `NONE` — passerebbe qualunque verifica se l'unico oracolo fosse l'occhio.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playground")
	static TArray<FString> DiagnosticsLines();

	/**
	 * I tre `ArmLength` dei preset di camera: `Close` · `Tactical` · `Overview`.
	 *
	 * ⚠️ **Dichiarati, non impressioni**, come la DoD chiede. I valori sono quelli della seduta `U25`.
	 * ⛔ **Il legame con `U25` e' una CITAZIONE, non un controllo**: quei numeri vivono in prosa
	 * (`editor-sessions.yaml`, la guida di seduta) e nessun test puo' leggerli da li'. Cio' che il test
	 * verifica e' che siano tre e strettamente crescenti — se un giorno divergono da `U25`, va detto qui.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playground")
	static TArray<float> CameraPresetArmLengths();
};
